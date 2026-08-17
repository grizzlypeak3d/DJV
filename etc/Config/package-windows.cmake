# A Windows package.

# The sample data and legal documents go at the top level of the installer
# rather than under a Unix style share directory. The NSIS generator runs
# either way, so nothing fails without this: the installer is simply built
# around the generic layout instead.
set(DJV_WINDOWS_PACKAGE ON CACHE BOOL "" FORCE)

include("${CMAKE_CURRENT_LIST_DIR}/package.cmake")
