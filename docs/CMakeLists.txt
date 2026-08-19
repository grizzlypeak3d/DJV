# The documentation is installed beside the application and opened from
# there, so that what it describes is the version that is running. It is
# plain HTML with relative links, so the directory works as it stands.

set(DEST share/djv/docs)
if(DJV_MACOS_PACKAGE OR DJV_WINDOWS_PACKAGE)
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
