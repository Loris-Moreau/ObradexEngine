#!/usr/bin/env bash
# ============================================================
#  setup.sh  —  ObradexEngine Dependency Bootstrapper
# ============================================================
#  Fetches ImGui, stb_image, and GLM.
#  GLAD is already pre-generated and bundled in third_party/glad/
#  — no Python required.
#
#  Requirements: git, curl or wget, unzip
#  Run from the repository root:  bash setup.sh
# ============================================================

set -e   # Exit on error (but not pipefail — Git Bash has quirks with it)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TP="$SCRIPT_DIR/third_party"
mkdir -p "$TP"

section() { printf "\n\033[1;36m[%s]\033[0m %s\n" "$1" "$2"; }
ok()      { printf "  \033[0;32mv\033[0m %s\n" "$1"; }
warn()    { printf "  \033[0;33m!\033[0m %s\n" "$1"; }

download() {
    local url="$1" dest="$2"
    if command -v curl &>/dev/null; then
        curl -fsSL "$url" -o "$dest"
    elif command -v wget &>/dev/null; then
        wget -q "$url" -O "$dest"
    else
        echo "ERROR: Neither curl nor wget found. Please install one."
        exit 1
    fi
}

echo ""
echo "======================================================"
echo "   ObradexEngine - Dependency Setup"
echo "======================================================"

# ── [1/4] GLAD — pre-generated, bundled in the repo ──────────
section "1/4" "GLAD (OpenGL 4.1 Core loader)"

GLAD_H="$TP/glad/include/glad/glad.h"

# Use grep to detect the real GLAD — "GLAPI" appears thousands
# of times in a real generated file, never in a stub.
if grep -q "GLAPI" "$GLAD_H" 2>/dev/null; then
    ok "Pre-generated GLAD found — no action needed."
else
    warn "GLAD not found in third_party/glad/ !"
    warn "It should have been bundled in the zip."
    warn "If it is missing, download a pre-generated GLAD from:"
    warn "   https://glad.dav1d.de/"
    warn "   Settings: Language=C, Spec=OpenGL, API gl=4.1, Profile=Core"
    warn "   Tick 'Generate a loader', click Generate, download the zip."
    warn "   Extract so that glad.h is at:"
    warn "     third_party/glad/include/glad/glad.h"
    warn "   and glad.c is at:"
    warn "     third_party/glad/src/glad.c"
fi


# ── [2/4] Dear ImGui ─────────────────────────────────────────
section "2/4" "Dear ImGui v1.90.1"

if [ -f "$TP/imgui/imgui.h" ]; then
    ok "Already present — skipping clone."
else
    echo "  Cloning Dear ImGui v1.90.1..."
    rm -rf "$TP/imgui"
    git clone --depth 1 --branch v1.90.1 \
        https://github.com/ocornut/imgui.git \
        "$TP/imgui"
    ok "ImGui cloned."
fi


# ── [3/4] stb_image ──────────────────────────────────────────
section "3/4" "stb_image"

mkdir -p "$TP/stb"

if [ -f "$TP/stb/stb_image.h" ]; then
    ok "stb_image.h already present."
else
    echo "  Downloading stb_image.h..."
    download \
        "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" \
        "$TP/stb/stb_image.h"
    ok "stb_image.h downloaded."
fi

if [ ! -f "$TP/stb/stb_image_impl.cpp" ]; then
    cat > "$TP/stb/stb_image_impl.cpp" << 'STB_EOF'
// stb_image_impl.cpp
// Defines STB_IMAGE_IMPLEMENTATION in exactly one translation unit.
// All other files simply #include "stb_image.h" without this define.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
STB_EOF
    ok "stb_image_impl.cpp created."
else
    ok "stb_image_impl.cpp already present."
fi


# ── [4/4] GLM ────────────────────────────────────────────────
section "4/4" "GLM 0.9.9.8 (math library)"

if [ -f "$TP/glm/glm/glm.hpp" ]; then
    ok "Already present — skipping download."
else
    echo "  Downloading GLM 0.9.9.8..."
    TMP_GLM="$(mktemp -d)"
    download \
        "https://github.com/g-truc/glm/archive/refs/tags/0.9.9.8.zip" \
        "$TMP_GLM/glm.zip"
    echo "  Extracting..."
    unzip -q "$TMP_GLM/glm.zip" -d "$TMP_GLM"
    mkdir -p "$TP/glm"
    cp -r "$TMP_GLM/glm-0.9.9.8/glm" "$TP/glm/"
    rm -rf "$TMP_GLM"
    ok "GLM installed to third_party/glm/glm/."
fi


# ── Summary ───────────────────────────────────────────────────
echo ""
echo "======================================================"
echo "   Setup complete!"
echo "======================================================"
echo ""
echo "  Build with:"
echo "    mkdir build"
echo "    cd build"
echo "    cmake .. -DCMAKE_BUILD_TYPE=Debug"
echo "    cmake --build ."
echo ""
echo "  Or open bitEngine.sln directly in Visual Studio."
echo ""
echo "  Controls:"
echo "    WASD / Mouse  - move and look"
echo "    Shift         - sprint"
echo "    Ctrl          - crouch / slide"
echo "    Space         - jump"
echo "    Q / E         - lean"
echo "    E (near obj)  - interact"
echo "    F1            - editor overlay"
echo "    Escape        - pause"
echo ""

# Keep terminal open when run by double-clicking on Windows
if [ -t 1 ]; then
    printf "Press ENTER to exit..."
    read -r _
fi
