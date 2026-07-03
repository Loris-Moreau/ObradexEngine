# ============================================================
#  setup.ps1  --  ObradexEngine Dependency Setup (Windows)
# ============================================================
#  Run from the repository root:
#    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
#    .\setup.ps1
# ============================================================
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$tp   = Join-Path $root "third_party"

function ok($msg)   { Write-Host "  [OK] $msg" -ForegroundColor Green  }
function warn($msg) { Write-Host "  [!!] $msg" -ForegroundColor Yellow }
function fail($msg) { Write-Host "  [XX] $msg" -ForegroundColor Red    }

Write-Host ""
Write-Host "======================================================"
Write-Host "   ObradexEngine - Dependency Setup (Windows)"
Write-Host "======================================================"

# ── [1/5] GLAD (OpenGL 4.1 Core loader) ──────────────────────
# vcxproj expects glad at $(SolutionDir)glad\  (the repo root),
# NOT inside third_party\. Setup places it there directly.
Write-Host ""
Write-Host "[1/5] GLAD (OpenGL 4.1 Core loader)"

$gladH = Join-Path $root "glad\include\glad\glad.h"
$gladReal = (Test-Path $gladH) -and (Select-String -Path $gladH -Pattern "GLAPI" -Quiet)

if ($gladReal) { ok "Already present at glad\." }
else {
    $gladDir = Join-Path $root "glad"
    $hasPython = $false
    try { $null = & python --version 2>&1; $hasPython = $true } catch {}

    if ($hasPython) {
        Write-Host "  Generating via glad Python package..."
        & python -m pip install glad --quiet
        New-Item -ItemType Directory -Force -Path $gladDir | Out-Null
        & python -m glad --generator c --profile core --api "gl=4.1" --out-path $gladDir
        if ((Test-Path $gladH) -and (Select-String -Path $gladH -Pattern "GLAPI" -Quiet)) {
            ok "GLAD generated via Python."
        } else { warn "Python generation failed, trying download..." }
    }

    if (-not $gladReal) {
        $tmp = Join-Path $env:TEMP "glad_dl"
        New-Item -ItemType Directory -Force -Path $tmp | Out-Null
        $zip = Join-Path $tmp "glad.zip"
        $url = "https://glad.dav1d.de/generate?language=c&specification=gl" +
               "&api=gl%3D4.1&api=gles1%3Dnone&api=gles2%3Dnone&profile=core&loader=on"
        try {
            Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing
            $ext = Join-Path $tmp "out"
            Expand-Archive -Path $zip -DestinationPath $ext -Force
            New-Item -ItemType Directory -Force -Path $gladDir | Out-Null
            Copy-Item (Join-Path $ext "include") (Join-Path $gladDir "include") -Recurse -Force
            Copy-Item (Join-Path $ext "src")     (Join-Path $gladDir "src")     -Recurse -Force
            ok "GLAD downloaded from glad.dav1d.de."
        } catch {
            fail "GLAD download failed: $_"
            warn "Manual: https://glad.dav1d.de/ -> C, OpenGL, gl=4.1, Core, loader -> extract"
            warn "  glad.h  -> glad\include\glad\glad.h"
            warn "  glad.c  -> glad\src\glad.c"
        }
        Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# ── [2/5] Dear ImGui v1.90.1 ─────────────────────────────────
# v1.90.1 ships GLFW headers/lib inside examples\libs\glfw\
# which is what the vcxproj links against.
Write-Host ""
Write-Host "[2/5] Dear ImGui v1.90.1"

$imguiH = Join-Path $tp "imgui\imgui.h"
if (Test-Path $imguiH) { ok "Already present." }
else {
    $imguiDir = Join-Path $tp "imgui"
    if (Test-Path $imguiDir) { Remove-Item $imguiDir -Recurse -Force }
    git clone --depth 1 --branch v1.90.1 https://github.com/ocornut/imgui.git $imguiDir
    if (Test-Path $imguiH) { ok "ImGui cloned." } else { fail "Clone failed."; exit 1 }
}

# ── [3/5] stb_image ──────────────────────────────────────────
Write-Host ""
Write-Host "[3/5] stb_image"

$stbDir  = Join-Path $tp "stb"
$stbH    = Join-Path $stbDir "stb_image.h"
$stbImpl = Join-Path $stbDir "stb_image_impl.cpp"
New-Item -ItemType Directory -Force -Path $stbDir | Out-Null

if (Test-Path $stbH) { ok "stb_image.h present." }
else {
    Invoke-WebRequest `
        -Uri "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" `
        -OutFile $stbH -UseBasicParsing
    ok "stb_image.h downloaded."
}
if (-not (Test-Path $stbImpl)) {
    Set-Content $stbImpl @"
// stb_image_impl.cpp - single implementation unit; do not add STB_IMAGE_IMPLEMENTATION elsewhere.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
"@ -Encoding UTF8
    ok "stb_image_impl.cpp created."
} else { ok "stb_image_impl.cpp present." }

# ── [4/5] GLM 0.9.9.8 ────────────────────────────────────────
Write-Host ""
Write-Host "[4/5] GLM 0.9.9.8"

$glmH = Join-Path $tp "glm\glm\glm.hpp"
if (Test-Path $glmH) { ok "Already present." }
else {
    $tmp = Join-Path $env:TEMP "glm_dl"; New-Item -ItemType Directory -Force -Path $tmp | Out-Null
    $zip = Join-Path $tmp "glm.zip"
    Invoke-WebRequest -Uri "https://github.com/g-truc/glm/archive/refs/tags/0.9.9.8.zip" `
        -OutFile $zip -UseBasicParsing
    Expand-Archive -Path $zip -DestinationPath $tmp -Force
    $glmDst = Join-Path $tp "glm"
    New-Item -ItemType Directory -Force -Path $glmDst | Out-Null
    Copy-Item (Join-Path $tmp "glm-0.9.9.8\glm") (Join-Path $glmDst "glm") -Recurse -Force
    Remove-Item $tmp -Recurse -Force
    ok "GLM installed."
}

# ── [5/5] miniaudio ──────────────────────────────────────────
# Single-header library. AudioSystem.h includes it from third_party\miniaudio\.
# miniaudio_impl.cpp (in bitEngine\) defines MINIAUDIO_IMPLEMENTATION.
Write-Host ""
Write-Host "[5/5] miniaudio (audio backend)"

$maDir = Join-Path $tp "miniaudio"
$maH   = Join-Path $maDir "miniaudio.h"
New-Item -ItemType Directory -Force -Path $maDir | Out-Null

if ((Test-Path $maH) -and ((Get-Item $maH).Length -gt 1MB)) { ok "Already present." }
else {
    Write-Host "  Downloading miniaudio.h (~2.5 MB)..."
    try {
        Invoke-WebRequest `
            -Uri "https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h" `
            -OutFile $maH -UseBasicParsing
        ok "miniaudio.h downloaded ($([math]::Round((Get-Item $maH).Length/1MB,1)) MB)."
    } catch {
        fail "Download failed: $_"
        warn "Manual: https://github.com/mackron/miniaudio -> miniaudio.h"
        warn "  Place at: third_party\miniaudio\miniaudio.h"
    }
}

# ── Done ──────────────────────────────────────────────────────
Write-Host ""
Write-Host "======================================================"
Write-Host "   All dependencies ready." -ForegroundColor Green
Write-Host "======================================================"
Write-Host ""
Write-Host "  Open bitEngine.sln in Visual Studio or Rider."
Write-Host "  Build configuration: Debug x64"
Write-Host ""
Read-Host "Press ENTER to exit"
