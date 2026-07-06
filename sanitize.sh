#!/bin/sh

# Build the whole DJV stack (DJV + tlRender + feather-tk, all from source) with
# sanitizers and run the test suites.
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

# Configure the whole stack with the sanitizer. DJV_SANITIZE propagates to the
# tlRender and ftk subdirectories via CMAKE_CXX_FLAGS. RelWithDebInfo keeps
# symbols so reports show file:line; examples/programs are skipped to keep the
# build quicker (the tests are what we run).
cmake \
    -S "$SOURCE_DIR" \
    -B "$BUILD_DIR" \
    -DDJV_SANITIZE="$SANITIZERS" \
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

# Run the test suites under the sanitizers.
#   halt_on_error=0  -> report every finding rather than stopping at the first
#   detect_leaks     -> LeakSanitizer is Linux-only; off on macOS
#   print_stacktrace -> give UndefinedBehaviorSanitizer a stack per report
# DJV enables testing at the top level, so ctest from the build root runs every
# registered test (DJV + tlRender + feather-tk).
LEAKS=0
if [ "$(uname)" = "Linux" ]; then
    LEAKS=1
fi

( cd "$BUILD_DIR" && \
  UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" \
  ASAN_OPTIONS="halt_on_error=1:detect_leaks=$LEAKS:detect_container_overflow=0" \
  ctest --output-on-failure )
