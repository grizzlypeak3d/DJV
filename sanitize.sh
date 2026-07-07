#!/bin/sh

# Build the whole DJV stack (DJV + tlRender + feather-tk, all from source) with
# sanitizers, run the test suites, and generate the documentation screenshots
# (which drives the application UI through many states for extra coverage).
#
# Prerequisites:
#   Run a normal super-build first so the deps are installed, e.g.
#       sh sbuild-macos.sh DJV Release
#   This reuses those deps (install-Release) and rebuilds only our code with
#   instrumentation, so it is faster than a full rebuild.
#
# Usage:
#   sh sanitize.sh [SOURCE_DIR] [PREFIX] [SANITIZERS]
#     SOURCE_DIR   DJV source root    (default: DJV)
#     PREFIX       Deps install dir   (default: $PWD/install-Release)
#     SANITIZERS   Sanitizer list     (default: address,undefined)

set -x

SOURCE_DIR=${1:-DJV}
PREFIX=${2:-$PWD/install-Release}
SANITIZERS=${3:-address,undefined}
BUILD_DIR=sanitize-build
JOBS=${JOBS:-4}

cmake \
    -S "$SOURCE_DIR" \
    -B "$BUILD_DIR" \
    -DDJV_SANITIZE="$SANITIZERS" \
    -DDJV_TESTS=ON \
    -Dftk_TESTS=ON \
    -Dftk_EXAMPLES=OFF \
    -DTLRENDER_TESTS=ON \
    -DTLRENDER_EXAMPLES=OFF \
    -DTLRENDER_PROGRAMS=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" || exit 1

cmake --build "$BUILD_DIR" -j "$JOBS" || exit 1

export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=0"
LEAKS=0
if [ "$(uname)" = "Linux" ]; then
    LEAKS=1
fi
export ASAN_OPTIONS="halt_on_error=0:detect_leaks=$LEAKS:detect_container_overflow=0"
SUPPRESSIONS=$(cd "$SOURCE_DIR" && pwd)/etc/sanitizer-suppressions.txt
if [ -f "$SUPPRESSIONS" ]; then
    export LSAN_OPTIONS="suppressions=$SUPPRESSIONS"
fi

for name in ftk-test tl-test djv-test; do
    exe=$(find "$BUILD_DIR" -type f -name "$name" 2>/dev/null | head -1)
    if [ -x "$exe" ]; then
        printf '\n======== Running %s ========\n' "$name"
        "$exe" -log
    else
        printf '\n======== %s not found, skipping ========\n' "$name"
    fi
done

# Exercise the application UI by generating the documentation screenshots under
# the sanitizer. Each shot is a separate djv process that sets up a distinct UI
# state (files, tools, compare modes, ...), renders offscreen, and exits, so
# LeakSanitizer gets a clean check across many configurations. This needs a GL
# context, so run it on a workstation with a display.
djv=$(find "$BUILD_DIR" -type f -name djv -perm -u+x 2>/dev/null | head -1)
manifest="$SOURCE_DIR/etc/Screenshots/screenshots.json"
driver="$SOURCE_DIR/etc/Screenshots/build_screenshots.py"
if [ -x "$djv" ] && [ -f "$driver" ]; then
    printf '\n======== Running documentation screenshots ========\n'
    shots=$(mktemp -d)
    python3 "$driver" "$manifest" --djv "$djv" --shots-dir "$shots"
    rm -rf "$shots"
else
    printf '\n======== djv or screenshot driver not found, skipping ========\n'
fi
