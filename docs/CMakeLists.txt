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

# Writing the navigation into the pages, from nav.txt, and the table of
# contents into the pages that carry one, from their own headings. Not part
# of the build: it edits the sources, and what is in the repository is what
# is published.
add_custom_target(djvDocsNav
    COMMAND ${CMAKE_COMMAND}
        -DNAV_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DNAV_LIST=${CMAKE_CURRENT_SOURCE_DIR}/nav.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/Nav.cmake
    COMMENT "Documentation navigation")
set_target_properties(djvDocsNav PROPERTIES FOLDER docs)

# The same, reporting rather than writing, for a build that wants to know
# whether anyone has edited a page's navigation or table of contents by
# hand.
add_custom_target(djvDocsNavCheck
    COMMAND ${CMAKE_COMMAND}
        -DNAV_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DNAV_LIST=${CMAKE_CURRENT_SOURCE_DIR}/nav.txt
        -DNAV_CHECK=ON
        -P ${CMAKE_CURRENT_SOURCE_DIR}/Nav.cmake
    COMMENT "Documentation navigation check")
set_target_properties(djvDocsNavCheck PROPERTIES FOLDER docs)
