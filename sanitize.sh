#!/bin/sh

# Build feather-tk with sanitizers and run its test suite.
#
# Prerequisites:
#   Run a normal super-build first so the dependencies are installed, e.g.
#       sh sbuild-macos.sh DJV Release
#   This script reuses those installed deps and only rebuilds feather-tk itself
#   with instrumentation.
#
# Usage:
#   sh sanitize.sh <SOURCE_DIR> [PREFIX] [SANITIZERS]
#     SOURCE_DIR   DJV source root
#     PREFIX       Deps install dir   (default: $PWD/install-Release)
#     SANITIZERS   Sanitizer list     (default: address,undefined)

set -x

SOURCE_DIR=$1
PREFIX=${2:-$PWD/install-Release}
SANITIZERS=${3:-address,undefined}

FTK_SRC="$SOURCE_DIR/deps/tlRender/deps/ftk"
BUILD_DIR="sanitize-build"
JOBS=${JOBS:-4}

if [ -z "$SOURCE_DIR" ]; then
    echo "Usage: sh sanitize.sh <SOURCE_DIR> [PREFIX] [SANITIZERS]"
    exit 1
fi

# Configure feather-tk with the sanitizer flags. RelWithDebInfo keeps symbols
# so reports show file:line; examples are skipped to keep the build quick.
cmake \
    -S "$FTK_SRC" \
    -B "$BUILD_DIR" \
    -Dftk_SANITIZE="$SANITIZERS" \
    -Dftk_TESTS=ON \
    -Dftk_EXAMPLES=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" || exit 1

cmake --build "$BUILD_DIR" -j "$JOBS" || exit 1

# Run the tests under the sanitizers.
#   halt_on_error=0  -> report every finding rather than stopping at the first
#   detect_leaks     -> LeakSanitizer is Linux-only; off on macOS so it doesn't
#                       error out. Run on Linux to also catch leaks.
#   print_stacktrace -> give UndefinedBehaviorSanitizer a stack per report
LEAKS=0
if [ "$(uname)" = "Linux" ]; then
    LEAKS=1
fi

cd "$BUILD_DIR" || exit 1
UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=0" \
ASAN_OPTIONS="halt_on_error=0:detect_leaks=$LEAKS" \
ctest --output-on-failure
cd - >/dev/null 2>&1
