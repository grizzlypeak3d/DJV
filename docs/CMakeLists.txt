# The documentation is installed beside the application and opened from
# there, so that what it describes is the version that is running. It is
# plain HTML with relative links, so the directory works as it stands.

# The macOS package is not here: the bundle carries its own copy,
# taken from the prefix by cmake/Modules/Package.cmake, so this stays
# where every other build puts it.
set(DEST share/djv/docs)
if(DJV_WINDOWS_PACKAGE)
    set(DEST docs)
endif()
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${DEST}
    COMPONENT docs
    FILES_MATCHING
        PATTERN "*.html"
        PATTERN "*.css"
        PATTERN "*.svg"
        PATTERN "*.png")
