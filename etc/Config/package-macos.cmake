# A macOS package, built back to the oldest supported system and for a
# named architecture rather than the one that happened to build it.

# An application bundle: a disk image rather than a zip, with the libraries,
# sample data and legal documents laid out for a bundle instead of a Unix
# prefix. Off is not a failure, it is the generic relocatable package a
# source build wants, so a release has to ask for this one by name.
set(DJV_MACOS_PACKAGE ON CACHE BOOL "" FORCE)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/package.cmake")
