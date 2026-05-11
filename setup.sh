#!/usr/bin/env bash
# ============================================================
#  setup.sh  —  ObradexEngine Dependency Setup (Linux / macOS)
# ============================================================
#  For Windows use setup.ps1 in PowerShell instead.
#  Run from the repo root:  bash setup.sh
# ============================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TP="$SCRIPT_DIR/third_party"
mkdir -p "$TP"

ok()   { echo "  [OK] $1"; }
warn() { echo "  [!!] $1"; }

download() {
    local url="$1" dest="$2"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL "$url" -o "$dest" && return 0
    elif command -v wget >/dev/null 2>&1; then
        wget -q "$url" -O "$dest" && return 0
    fi
    warn "Neither curl nor wget found."
    return 1
}

echo ""
echo "======================================================"
echo "   ObradexEngine - Dependency Setup"
echo "======================================================"

# ── [1] GLAD — pre-generated and bundled ─────────────────────
echo ""
echo "[1/4] GLAD (OpenGL 4.1 Core loader)"

GLAD_H="$TP/glad/include/glad/glad.h"
if grep -q "GLAPI" "$GLAD_H" 2>/dev/null; then
    ok "Pre-generated GLAD found -- no action needed."
else
    warn "GLAD not found at third_party/glad/include/glad/glad.h"
    warn "It should have been bundled in the zip."
    warn "If missing, visit https://glad.dav1d.de/"
    warn "  Language: C, API gl: 4.1, Profile: Core, Generate loader: yes"
    warn "  Copy glad.h -> third_party/glad/include/glad/glad.h"
    warn "  Copy glad.c -> third_party/glad/src/glad.c"
fi

# ── [2] Dear ImGui v1.90.1 ───────────────────────────────────
echo ""
echo "[2/4] Dear ImGui v1.90.1"

if [ -f "$TP/imgui/imgui.h" ]; then
    ok "Already present -- skipping clone."
else
    echo "  Cloning Dear ImGui v1.90.1..."
    rm -rf "$TP/imgui"
    if git clone --depth 1 --branch v1.90.1 \
        https://github.com/ocornut/imgui.git \
        "$TP/imgui"; then
        ok "ImGui cloned."
    else
        warn "git clone failed."
    fi
fi

# ── [3] stb_image ────────────────────────────────────────────
echo ""
echo "[3/4] stb_image"

mkdir -p "$TP/stb"

if [ -f "$TP/stb/stb_image.h" ]; then
    ok "stb_image.h already present."
else
    echo "  Downloading stb_image.h..."
    if download \
        "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" \
        "$TP/stb/stb_image.h"; then
        ok "stb_image.h downloaded."
    else
        warn "Download failed."
    fi
fi

if [ ! -f "$TP/stb/stb_image_impl.cpp" ]; then
    cat > "$TP/stb/stb_image_impl.cpp" << 'STB'
// stb_image_impl.cpp
// Defines STB_IMAGE_IMPLEMENTATION in exactly one translation unit.
// All other files simply #include "stb_image.h" without this define.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
STB
    ok "stb_image_impl.cpp created."
else
    ok "stb_image_impl.cpp already present."
fi

# ── [4] GLM 0.9.9.8 ─────────────────────────────────────────
echo ""
echo "[4/4] GLM 0.9.9.8 (math library)"

if [ -f "$TP/glm/glm/glm.hpp" ]; then
    ok "Already present -- skipping download."
else
    echo "  Downloading GLM 0.9.9.8..."
    TMP_GLM="$(mktemp -d 2>/dev/null || echo /tmp/glm_setup_$$)"
    mkdir -p "$TMP_GLM"
    if download \
        "https://github.com/g-truc/glm/archive/refs/tags/0.9.9.8.zip" \
        "$TMP_GLM/glm.zip"; then
        echo "  Extracting..."
        unzip -q "$TMP_GLM/glm.zip" -d "$TMP_GLM"
        mkdir -p "$TP/glm"
        cp -r "$TMP_GLM/glm-0.9.9.8/glm" "$TP/glm/"
        rm -rf "$TMP_GLM"
        ok "GLM installed to third_party/glm/"
    else
        warn "GLM download failed. CMake will auto-fetch it at build time."
        rm -rf "$TMP_GLM"
    fi
fi

echo ""
echo "======================================================"
echo "   Setup complete!"
echo "======================================================"
echo ""
echo "  Build:  mkdir build && cd build"
echo "          cmake .. -DCMAKE_BUILD_TYPE=Debug"
echo "          cmake --build ."
echo ""

if [ -t 1 ]; then
    printf "Press ENTER to exit..."
    read -r _
fi
