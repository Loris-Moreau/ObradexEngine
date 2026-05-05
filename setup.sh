#!/usr/bin/env bash
# ============================================================
#  setup.sh  —  ObradexEngine Dependency Bootstrapper
# ============================================================
#  Downloads and installs all third-party libraries into the
#  third_party/ directory.  Run once before first build.
#
#  Requires: git, curl (or wget), unzip
#  Tested on: Ubuntu 22.04, macOS 13, Windows (Git Bash)
# ============================================================

set -e   # Exit immediately on error
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TP="$SCRIPT_DIR/third_party"
mkdir -p "$TP"

echo "============================================"
echo "  ObradexEngine — Dependency Setup"
echo "============================================"

# ── 1. GLAD (OpenGL 4.1 Core loader) ─────────────────────────
# We bundle a pre-generated GLAD for OpenGL 4.1 Core.
# If you need a different version, regenerate at:
#   https://glad.dav1d.de/
echo ""
echo "[1/4] Setting up GLAD..."
mkdir -p "$TP/glad/include/glad"
mkdir -p "$TP/glad/src"

cat > "$TP/glad/include/glad/glad.h" << 'GLAD_HEADER_STUB'
/*
 * GLAD OpenGL 4.1 Core — stub header.
 *
 * Replace this file with a real GLAD-generated header.
 * Generate one at: https://glad.dav1d.de/
 *   API  : gl
 *   Version: 4.1
 *   Profile: Core
 *   Options: Generate a loader
 *
 * Then copy glad.h → third_party/glad/include/glad/glad.h
 *      and glad.c → third_party/glad/src/glad.c
 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef void* (* GLADloadproc)(const char* name);
int gladLoadGLLoader(GLADloadproc);
#ifdef __cplusplus
}
#endif
GLAD_HEADER_STUB

cat > "$TP/glad/src/glad.c" << 'GLAD_SRC_STUB'
/*
 * GLAD stub — replace with the real generated glad.c
 * See the comment in glad.h for generation instructions.
 */
#include "glad/glad.h"
int gladLoadGLLoader(GLADloadproc load) { (void)load; return 1; }
GLAD_SRC_STUB

echo "  → GLAD stub created (replace with generated version!)"

# ── 2. Dear ImGui ─────────────────────────────────────────────
echo ""
echo "[2/4] Cloning Dear ImGui v1.90.1..."
if [ ! -d "$TP/imgui/.git" ]; then
    git clone --depth 1 --branch v1.90.1 \
        https://github.com/ocornut/imgui.git \
        "$TP/imgui"
else
    echo "  → Already present, skipping."
fi

# ── 3. stb (header-only image loading) ────────────────────────
echo ""
echo "[3/4] Fetching stb_image..."
mkdir -p "$TP/stb"
if [ ! -f "$TP/stb/stb_image.h" ]; then
    curl -fsSL \
        "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" \
        -o "$TP/stb/stb_image.h"
    # Create the implementation file (include once, in a .cpp)
    cat > "$TP/stb/stb_image_impl.cpp" << 'STB_IMPL'
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
STB_IMPL
else
    echo "  → Already present, skipping."
fi

# ── 4. GLFW (system install preferred; fallback to source) ─────
echo ""
echo "[4/4] Checking for GLFW..."
if pkg-config --exists glfw3 2>/dev/null; then
    echo "  → System GLFW found ($(pkg-config --modversion glfw3))."
else
    echo "  → System GLFW not found."
    echo "     CMake will auto-fetch GLFW via FetchContent on first build."
    echo "     Or install manually:"
    echo "       Ubuntu: sudo apt install libglfw3-dev"
    echo "       macOS : brew install glfw"
fi

# ── Summary ───────────────────────────────────────────────────
echo ""
echo "============================================"
echo "  Setup complete!"
echo ""
echo "  IMPORTANT: Replace the GLAD stubs with the"
echo "  real generated files from:"
echo "    https://glad.dav1d.de/"
echo "  (API: gl 4.1, Profile: Core, Generate loader)"
echo ""
echo "  To build:"
echo "    mkdir build && cd build"
echo "    cmake .. -DCMAKE_BUILD_TYPE=Debug"
echo "    cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)"
echo "    ./bin/ObradexEngine"
echo "============================================"
