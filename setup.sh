#!/usr/bin/env bash
# ============================================================
#  setup.sh  -  ObradexEngine dependency bootstrapper
#  Works on: Linux, macOS, Windows (Git Bash)
#
#  Usage: bash setup.sh
# ============================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TP="$SCRIPT_DIR/third_party"
mkdir -p "$TP"

ok()   { echo "  [OK] $1"; }
warn() { echo "  [!!] $1"; }
fail() { echo "  [XX] $1"; }

# Detect OS for messages
case "$(uname -s)" in
    Linux*)  OS=Linux  ;;
    Darwin*) OS=macOS  ;;
    MINGW*|MSYS*|CYGWIN*) OS=Windows ;;
    *)       OS=Unknown ;;
esac

# Download helper - uses curl or wget, whichever is available
download() {
    local url="$1" dest="$2"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL "$url" -o "$dest"
        return $?
    elif command -v wget >/dev/null 2>&1; then
        wget -q "$url" -O "$dest"
        return $?
    else
        fail "Neither curl nor wget found. Please install one."
        return 1
    fi
}

echo ""
echo "======================================================"
echo "   ObradexEngine - Dependency Setup  [$OS]"
echo "======================================================"


# ── [1/4] GLAD ───────────────────────────────────────────────
# GLAD is a generated OpenGL loader - we need either Python
# (to run the glad generator) or to download a pre-built zip
# from the official glad.dav1d.de generator web service.
echo ""
echo "[1/4] GLAD (OpenGL 4.1 Core loader)"

GLAD_H="$TP/glad/include/glad/glad.h"
GLAD_C="$TP/glad/src/glad.c"

glad_is_real() {
    # Returns 0 (true) only if glad.h exists and contains real generated code
    grep -q "GLAPI" "$GLAD_H" 2>/dev/null
}

if glad_is_real; then
    ok "Already present."
else
    mkdir -p "$TP/glad/include/glad" "$TP/glad/src"
    GLAD_OK=false

    # Strategy 1: Python + glad pip package (generates locally, no network for spec)
    if command -v python3 >/dev/null 2>&1 || command -v python >/dev/null 2>&1; then
        PY=$(command -v python3 2>/dev/null || command -v python)
        echo "  Python found ($PY) - installing glad generator..."
        "$PY" -m pip install glad --quiet --break-system-packages 2>/dev/null \
            || "$PY" -m pip install glad --quiet

        echo "  Generating OpenGL 4.1 Core loader..."
        "$PY" -m glad \
            --generator c \
            --profile core \
            --api gl=4.1 \
            --out-path "$TP/glad" \
            --reproducible

        if glad_is_real; then
            ok "GLAD generated via Python."
            GLAD_OK=true
        else
            warn "Python generation failed - trying web download..."
        fi
    else
        echo "  Python not found - downloading from glad.dav1d.de..."
    fi

    # Strategy 2: download pre-generated zip from the official glad generator API
    if [ "$GLAD_OK" = false ]; then
        TMP_GLAD="$(mktemp -d 2>/dev/null || echo "$TP/../_glad_tmp")"
        mkdir -p "$TMP_GLAD"

        GLAD_URL="https://glad.dav1d.de/generate?language=c&specification=gl&api=gl%3D4.1&api=gles1%3Dnone&api=gles2%3Dnone&profile=core&loader=on"
        echo "  Fetching from glad.dav1d.de..."

        if download "$GLAD_URL" "$TMP_GLAD/glad.zip"; then
            # Verify it is actually a zip (not an HTML error page)
            if unzip -t "$TMP_GLAD/glad.zip" >/dev/null 2>&1; then
                unzip -q "$TMP_GLAD/glad.zip" -d "$TMP_GLAD/out"
                # zip layout: include/glad/glad.h  include/KHR/khrplatform.h  src/glad.c
                cp -r "$TMP_GLAD/out/include/." "$TP/glad/include/"
                cp -r "$TMP_GLAD/out/src/."     "$TP/glad/src/"
                if glad_is_real; then
                    ok "GLAD downloaded from glad.dav1d.de."
                    GLAD_OK=true
                fi
            else
                warn "Downloaded file is not a valid zip (server may have returned an error page)."
            fi
        fi
        rm -rf "$TMP_GLAD"
    fi

    if [ "$GLAD_OK" = false ]; then
        fail "Could not obtain GLAD automatically."
        echo ""
        echo "  Manual steps:"
        echo "    1. Open https://glad.dav1d.de/ in a browser"
        echo "    2. Language: C  |  Specification: OpenGL"
        echo "    3. API gl: 4.1  |  Profile: Core"
        echo "    4. Tick 'Generate a loader'  ->  click Generate  ->  Download"
        echo "    5. Extract the zip:"
        echo "         glad.h  ->  third_party/glad/include/glad/glad.h"
        echo "         glad.c  ->  third_party/glad/src/glad.c"
        echo ""
    fi
fi


# ── [2/4] Dear ImGui v1.90.1 ─────────────────────────────────
echo ""
echo "[2/4] Dear ImGui v1.90.1"

if [ -f "$TP/imgui/imgui.h" ]; then
    ok "Already present."
else
    echo "  Cloning (depth 1)..."
    rm -rf "$TP/imgui"
    if git clone --depth 1 --branch v1.90.1 \
        https://github.com/ocornut/imgui.git \
        "$TP/imgui" 2>&1; then
        ok "ImGui cloned."
    else
        fail "git clone failed. Is git installed and on PATH?"
    fi
fi


# ── [3/4] stb_image ──────────────────────────────────────────
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
        fail "Download failed."
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


# ── [4/4] GLM 0.9.9.8 ────────────────────────────────────────
echo ""
echo "[4/4] GLM 0.9.9.8 (math library)"

if [ -f "$TP/glm/glm/glm.hpp" ]; then
    ok "Already present."
else
    echo "  Downloading GLM 0.9.9.8..."
    TMP_GLM="$(mktemp -d 2>/dev/null || echo "$TP/../_glm_tmp")"
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
        fail "GLM download failed. CMake will try to auto-fetch it at configure time."
        rm -rf "$TMP_GLM"
    fi
fi


# ── Summary ───────────────────────────────────────────────────
echo ""
echo "======================================================"
echo "   Setup complete!"
echo "======================================================"
echo ""
echo "  Build:"
echo "    mkdir build"
echo "    cd build"
echo "    cmake .. -DCMAKE_BUILD_TYPE=Debug"
echo "    cmake --build ."
echo ""
echo "  Or open bitEngine.sln in Rider / Visual Studio."
echo ""

if [ -t 1 ]; then
    printf "Press ENTER to exit..."
    read -r _
fi
