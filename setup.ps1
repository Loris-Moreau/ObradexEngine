# ============================================================
#  setup.ps1  —  ObradexEngine Dependency Setup (Windows)
# ============================================================
#  Run from the repository root in PowerShell:
#    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
#    .\setup.ps1
# ============================================================

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$tp   = Join-Path $root "third_party"

function ok($msg)   { Write-Host "  [OK] $msg" -ForegroundColor Green  }
function warn($msg) { Write-Host "  [!!] $msg" -ForegroundColor Yellow }
function err($msg)  { Write-Host "  [XX] $msg" -ForegroundColor Red    }

Write-Host ""
Write-Host "======================================================"
Write-Host "   ObradexEngine - Dependency Setup (Windows)"
Write-Host "======================================================"

# ── [1] GLAD (OpenGL 4.1 Core loader) ────────────────────────
Write-Host ""
Write-Host "[1/4] GLAD (OpenGL 4.1 Core loader)"

$gladH    = Join-Path $tp "glad\include\glad\glad.h"
$gladC    = Join-Path $tp "glad\src\glad.c"
$gladReal = (Test-Path $gladH) -and (Select-String -Path $gladH -Pattern "GLAPI" -Quiet)

if ($gladReal) {
    ok "Already present."
} else {
    # Strategy 1: Python + pip (glad package)
    $hasPython = $false
    try {
        $null = & python --version 2>&1
        $hasPython = $true
    } catch {}

    if ($hasPython) {
        Write-Host "  Python found — generating via glad pip package..."
        & python -m pip install glad --quiet
        New-Item -ItemType Directory -Force -Path (Join-Path $tp "glad") | Out-Null
        & python -m glad --generator c --profile core --api "gl=4.1" `
            --out-path (Join-Path $tp "glad")
        if (Test-Path $gladH) { ok "GLAD generated via Python." }
        else { warn "Python generation failed — trying web download..." }
    }

    # Strategy 2: download pre-generated zip from glad.dav1d.de
    if (-not (Test-Path $gladH)) {
        Write-Host "  Downloading pre-generated GLAD from glad.dav1d.de..."
        $tmp = Join-Path $env:TEMP "glad_dl"
        New-Item -ItemType Directory -Force -Path $tmp | Out-Null
        $zip = Join-Path $tmp "glad.zip"

        try {
            # The glad.dav1d.de generate endpoint returns a ready-to-use zip
            $url = "https://glad.dav1d.de/generate?language=c&specification=gl" +
                   "&api=gl%3D4.1&api=gles1%3Dnone&api=gles2%3Dnone" +
                   "&profile=core&loader=on"
            Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing

            $ext = Join-Path $tmp "out"
            Expand-Archive -Path $zip -DestinationPath $ext -Force

            # Zip layout: include/glad/glad.h  include/KHR/khrplatform.h  src/glad.c
            New-Item -ItemType Directory -Force -Path (Join-Path $tp "glad") | Out-Null
            Copy-Item (Join-Path $ext "include") (Join-Path $tp "glad\include") -Recurse -Force
            Copy-Item (Join-Path $ext "src")     (Join-Path $tp "glad\src")     -Recurse -Force

            if (Test-Path $gladH) { ok "GLAD downloaded from glad.dav1d.de." }
            else { warn "Download succeeded but files not found in expected location." }
        } catch {
            warn "Web download failed: $_"
            warn "Manual steps:"
            warn "  1. Go to https://glad.dav1d.de/"
            warn "  2. Language=C, Specification=OpenGL, API gl=4.1, Profile=Core"
            warn "  3. Tick 'Generate a loader', click Generate, download zip"
            warn "  4. Extract so glad.h -> third_party\glad\include\glad\glad.h"
            warn "     and      glad.c -> third_party\glad\src\glad.c"
        }
        Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# ── [2] Dear ImGui v1.90.1 ────────────────────────────────────
Write-Host ""
Write-Host "[2/4] Dear ImGui v1.90.1"

$imguiH = Join-Path $tp "imgui\imgui.h"
if (Test-Path $imguiH) {
    ok "Already present."
} else {
    Write-Host "  Cloning Dear ImGui v1.90.1..."
    $imguiDir = Join-Path $tp "imgui"
    if (Test-Path $imguiDir) { Remove-Item $imguiDir -Recurse -Force }
    git clone --depth 1 --branch v1.90.1 https://github.com/ocornut/imgui.git $imguiDir
    if (Test-Path $imguiH) { ok "ImGui cloned." }
    else { err "Clone failed. Is git on PATH?"; exit 1 }
}

# ── [3] stb_image ─────────────────────────────────────────────
Write-Host ""
Write-Host "[3/4] stb_image"

$stbDir  = Join-Path $tp "stb"
$stbH    = Join-Path $stbDir "stb_image.h"
$stbImpl = Join-Path $stbDir "stb_image_impl.cpp"
New-Item -ItemType Directory -Force -Path $stbDir | Out-Null

if (Test-Path $stbH) { ok "stb_image.h already present." }
else {
    Write-Host "  Downloading stb_image.h..."
    Invoke-WebRequest `
        -Uri "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" `
        -OutFile $stbH -UseBasicParsing
    ok "stb_image.h downloaded."
}

if (-not (Test-Path $stbImpl)) {
    Set-Content $stbImpl @"
// stb_image_impl.cpp — defines STB_IMAGE_IMPLEMENTATION in one translation unit only.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
"@ -Encoding UTF8
    ok "stb_image_impl.cpp created."
} else { ok "stb_image_impl.cpp already present." }

# ── [4] GLM 0.9.9.8 ──────────────────────────────────────────
Write-Host ""
Write-Host "[4/4] GLM 0.9.9.8 (math library)"

$glmH = Join-Path $tp "glm\glm\glm.hpp"
if (Test-Path $glmH) { ok "Already present." }
else {
    Write-Host "  Downloading GLM 0.9.9.8..."
    $tmp = Join-Path $env:TEMP "glm_dl"
    New-Item -ItemType Directory -Force -Path $tmp | Out-Null
    $zip = Join-Path $tmp "glm.zip"
    Invoke-WebRequest `
        -Uri "https://github.com/g-truc/glm/archive/refs/tags/0.9.9.8.zip" `
        -OutFile $zip -UseBasicParsing
    Write-Host "  Extracting..."
    Expand-Archive -Path $zip -DestinationPath $tmp -Force
    $glmDir = Join-Path $tp "glm"
    New-Item -ItemType Directory -Force -Path $glmDir | Out-Null
    Copy-Item (Join-Path $tmp "glm-0.9.9.8\glm") (Join-Path $glmDir "glm") -Recurse -Force
    Remove-Item $tmp -Recurse -Force
    ok "GLM installed."
}

# ── Done ──────────────────────────────────────────────────────
Write-Host ""
Write-Host "======================================================"
Write-Host "   Setup complete!" -ForegroundColor Green
Write-Host "======================================================"
Write-Host ""
Write-Host "  Build options:"
Write-Host "    A) Open bitEngine.sln in Rider / Visual Studio"
Write-Host "    B) cmake .. -DCMAKE_BUILD_TYPE=Debug && cmake --build ."
Write-Host ""
Read-Host "Press ENTER to exit"
