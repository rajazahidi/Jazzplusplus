#!/bin/bash
# =============================================================================
# Build Jazz++ Standalone AppImage
# =============================================================================

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build}"
APPDIR="$ROOT_DIR/AppDir"

echo "=== Building Jazz++ AppImage ==="
echo "Project Root: $ROOT_DIR"
echo "Build Dir:    $BUILD_DIR"
echo "AppDir:       $APPDIR"

# 1. Ensure project is built
if [ ! -f "$BUILD_DIR/jazz" ]; then
    echo "Configuring and compiling Jazz++ in $BUILD_DIR..."
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" -j"$(nproc)"
fi

# 2. Stage installation into AppDir
rm -rf "$APPDIR"
mkdir -p "$APPDIR"
cmake --install "$BUILD_DIR" --prefix "$APPDIR/usr"

# 3. Copy top-level AppImage metadata
cp "$ROOT_DIR/jazz.desktop" "$APPDIR/jazz.desktop"
cp "$ROOT_DIR/resources/icons/jazz-256.png" "$APPDIR/jazz.png"
cp "$ROOT_DIR/resources/icons/jazz.svg" "$APPDIR/jazz.svg"
mkdir -p "$APPDIR/usr/share/metainfo"

# 4. Create AppRun entrypoint
cat << 'APPRUN' > "$APPDIR/AppRun"
#!/bin/sh
set -e

SELF=$(readlink -f "$0")
HERE=${SELF%/*}

export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${HERE}/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

if [ -x "${HERE}/usr/bin/jazz-launcher" ]; then
    exec "${HERE}/usr/bin/jazz-launcher" "$@"
else
    exec "${HERE}/usr/bin/jazz" "$@"
fi
APPRUN
chmod +x "$APPDIR/AppRun"

echo "[OK] Staged AppDir successfully at $APPDIR"

# 5. Pack with appimagetool if available
if command -v appimagetool >/dev/null 2>&1; then
    echo "Packing AppImage using appimagetool..."
    ARCH=x86_64 appimagetool "$APPDIR" "$ROOT_DIR/Jazz++-x86_64.AppImage"
    echo "[SUCCESS] Created $ROOT_DIR/Jazz++-x86_64.AppImage"
else
    echo "[INFO] appimagetool not found locally. AppDir is ready for linuxdeploy / appimagetool in CI."
fi
