#!/usr/bin/env bash
# ============================================================
#  setup.sh  --  ObradexEngine Dependency Setup
#  Works on: Linux, macOS, Windows (Git Bash / MSYS2)
#  Usage:  bash setup.sh
# ============================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR"
TP="$ROOT/third_party"
mkdir -p "$TP"

ok()   { echo "  [OK] $1"; }
warn() { echo "  [!!] $1"; }
fail() { echo "  [XX] $1"; }

case "$(uname -s)" in
    Linux*)           OS=Linux   ;;
    Darwin*)          OS=macOS   ;;
    MINGW*|MSYS*|CYGWIN*) OS=Windows ;;
    *)                OS=Unknown ;;
esac

dl() {   # dl <url> <dest>
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL "$1" -o "$2"
    elif command -v wget >/dev/null 2>&1; then
        wget -q "$1" -O "$2"
    else
        fail "Neither curl nor wget found."; return 1
    fi
}

echo ""
echo "======================================================"
echo "   ObradexEngine - Dependency Setup  [$OS]"
echo "======================================================"

# ── [1/5] GLAD ───────────────────────────────────────────────
# vcxproj expects glad at \$(SolutionDir)glad\  (repo root, NOT third_party\).
# Setup places it at <repo_root>/glad/ to match the build system.
echo ""
echo "[1/5] GLAD (OpenGL 4.1 Core loader)"

GLAD_H="$ROOT/glad/include/glad/glad.h"
glad_real() { grep -q "GLAPI" "$GLAD_H" 2>/dev/null; }

if glad_real; then
    ok "Already present at glad/."
else
    mkdir -p "$ROOT/glad/include/glad" "$ROOT/glad/src"
    GLAD_OK=false

    # Strategy 1: Python glad package
    PY=$(command -v python3 2>/dev/null || command -v python 2>/dev/null || echo "")
    if [ -n "$PY" ]; then
        echo "  Python found - generating GLAD..."
        "$PY" -m pip install glad --quiet --break-system-packages 2>/dev/null \
            || "$PY" -m pip install glad --quiet
        "$PY" -m glad --generator c --profile core --api gl=4.1 \
            --out-path "$ROOT/glad" --reproducible 2>/dev/null
        glad_real && { ok "GLAD generated via Python."; GLAD_OK=true; }
    fi

    # Strategy 2: glad.dav1d.de download
    if [ "$GLAD_OK" = false ]; then
        TMP="$(mktemp -d)"
        URL="https://glad.dav1d.de/generate?language=c&specification=gl&api=gl%3D4.1&api=gles1%3Dnone&api=gles2%3Dnone&profile=core&loader=on"
        echo "  Downloading from glad.dav1d.de..."
        if dl "$URL" "$TMP/glad.zip" && unzip -t "$TMP/glad.zip" >/dev/null 2>&1; then
            unzip -q "$TMP/glad.zip" -d "$TMP/out"
            cp -r "$TMP/out/include/." "$ROOT/glad/include/"
            cp -r "$TMP/out/src/."     "$ROOT/glad/src/"
            glad_real && { ok "GLAD downloaded."; GLAD_OK=true; }
        fi
        rm -rf "$TMP"
    fi

    if [ "$GLAD_OK" = false ]; then
        fail "Could not obtain GLAD automatically."
        echo "  Manual: https://glad.dav1d.de/ -> C, OpenGL, gl=4.1, Core, loader -> Generate"
        echo "    glad.h -> glad/include/glad/glad.h"
        echo "    glad.c -> glad/src/glad.c"
    fi
fi

# ── [2/5] Dear ImGui v1.90.1 ─────────────────────────────────
# v1.90.1 ships GLFW headers+lib inside examples/libs/glfw/
# which is what the vcxproj links against.
echo ""
echo "[2/5] Dear ImGui v1.90.1"

if [ -f "$TP/imgui/imgui.h" ]; then
    ok "Already present."
else
    rm -rf "$TP/imgui"
    if git clone --depth 1 --branch v1.90.1 \
        https://github.com/ocornut/imgui.git "$TP/imgui" 2>&1; then
        ok "ImGui cloned."
    else
        fail "git clone failed. Is git installed?"
    fi
fi

# ── [3/5] stb_image ──────────────────────────────────────────
echo ""
echo "[3/5] stb_image"

mkdir -p "$TP/stb"

if [ -f "$TP/stb/stb_image.h" ]; then
    ok "stb_image.h present."
else
    dl "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" \
       "$TP/stb/stb_image.h" && ok "stb_image.h downloaded." || fail "Download failed."
fi

if [ ! -f "$TP/stb/stb_image_impl.cpp" ]; then
    cat > "$TP/stb/stb_image_impl.cpp" << 'STB'
// stb_image_impl.cpp - defines STB_IMAGE_IMPLEMENTATION in exactly one TU.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
STB
    ok "stb_image_impl.cpp created."
else
    ok "stb_image_impl.cpp present."
fi

# ── [4/5] GLM 0.9.9.8 ────────────────────────────────────────
echo ""
echo "[4/5] GLM 0.9.9.8"

if [ -f "$TP/glm/glm/glm.hpp" ]; then
    ok "Already present."
else
    TMP="$(mktemp -d)"
    dl "https://github.com/g-truc/glm/archive/refs/tags/0.9.9.8.zip" "$TMP/glm.zip" \
    && unzip -q "$TMP/glm.zip" -d "$TMP" \
    && mkdir -p "$TP/glm" \
    && cp -r "$TMP/glm-0.9.9.8/glm" "$TP/glm/" \
    && ok "GLM installed." \
    || fail "GLM download failed."
    rm -rf "$TMP"
fi

# ── [5/5] miniaudio ──────────────────────────────────────────
# Single-header. AudioSystem.h includes it from third_party/miniaudio/.
# miniaudio_impl.cpp defines MINIAUDIO_IMPLEMENTATION.
echo ""
echo "[5/5] miniaudio (audio backend)"

mkdir -p "$TP/miniaudio"
MA_H="$TP/miniaudio/miniaudio.h"

if [ -f "$MA_H" ] && [ "$(wc -c < "$MA_H")" -gt 1000000 ]; then
    ok "Already present."
else
    echo "  Downloading miniaudio.h (~2.5 MB)..."
    dl "https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h" "$MA_H" \
    && ok "miniaudio.h downloaded ($(wc -c < "$MA_H" | tr -d ' ') bytes)." \
    || { fail "Download failed."; echo "  Manual: third_party/miniaudio/miniaudio.h"; }
fi

# ── Done ─────────────────────────────────────────────────────
echo ""
echo "======================================================"
echo "   All dependencies ready."
echo "======================================================"
echo ""
echo "  Open bitEngine.sln in Visual Studio or Rider."
echo "  Build: Debug x64"
echo ""
[ -t 1 ] && { printf "Press ENTER to exit..."; read -r _; }
