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

# The licenses page, written into the build tree by etc/Legal/CMakeLists.txt
# because it describes this configuration rather than the repository. Before
# the navigation below, which has to find it there to write the list into it.
install(
    FILES ${DJV_LICENSES_PAGE}
    DESTINATION ${DEST}
    COMPONENT docs)

# The change log page, written from ChangeLog.md at configure time so the
# Markdown stays the one copy. Into the build tree, like the licenses:
# generated pages are not kept in the repository. Configuring again when the
# change log changes keeps the page current.
set(DJV_CHANGELOG_PAGE ${CMAKE_CURRENT_BINARY_DIR}/changelog.html)
execute_process(
    COMMAND ${CMAKE_COMMAND}
        -DCHANGELOG_IN=${PROJECT_SOURCE_DIR}/ChangeLog.md
        -DCHANGELOG_OUT=${DJV_CHANGELOG_PAGE}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/Changelog.cmake
    COMMAND_ERROR_IS_FATAL ANY)
set_property(
    DIRECTORY APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/ChangeLog.md)
install(
    FILES ${DJV_CHANGELOG_PAGE}
    DESTINATION ${DEST}
    COMPONENT docs)

# The navigation, written into the installed copies, which have a page the
# repository does not: the licenses. The repository's own pages cannot carry
# that link, since the page it points at is made by a build.
install(
    CODE "
        set(NAV_DIR \"\${CMAKE_INSTALL_PREFIX}/${DEST}\")
        set(NAV_LIST \"${CMAKE_CURRENT_SOURCE_DIR}/nav.txt\")
        set(NAV_EXTRA \"${CMAKE_CURRENT_SOURCE_DIR}/nav-install.txt\")"
    COMPONENT docs)
install(
    SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/Nav.cmake
    COMPONENT docs)

# The same, written into the pages in the repository, along with the table of
# contents on the pages that carry one. Not part of the build: it edits the
# sources, and what is in the repository is what is published.
add_custom_target(djvDocsNav
    COMMAND ${CMAKE_COMMAND}
        -DNAV_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DNAV_LIST=${CMAKE_CURRENT_SOURCE_DIR}/nav.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/Nav.cmake
    COMMENT "Documentation navigation")
set_target_properties(djvDocsNav PROPERTIES FOLDER docs)

# The command line page, from the application's own help, into the
# repository: written here, then the navigation over every page so the new
# one is listed and has its table of contents. Not part of the build: it
# edits the sources. The check is what continuous integration runs, so an
# option added without regenerating the page is a failed build rather than
# a page that quietly went stale.
if(DJV_PROGRAMS)
    add_custom_target(djvDocsHelp
        COMMAND ${CMAKE_COMMAND}
            -DHELP_EXE=$<TARGET_FILE:djv>
            -DHELP_OUT=${CMAKE_CURRENT_SOURCE_DIR}/command-line.html
            -DHELP_TMP=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/Help.cmake
        COMMAND ${CMAKE_COMMAND}
            -DNAV_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -DNAV_LIST=${CMAKE_CURRENT_SOURCE_DIR}/nav.txt
            -P ${CMAKE_CURRENT_SOURCE_DIR}/Nav.cmake
        DEPENDS djv
        COMMENT "Documentation command line page")
    set_target_properties(djvDocsHelp PROPERTIES FOLDER docs)
    add_custom_target(djvDocsHelpCheck
        COMMAND ${CMAKE_COMMAND}
            -DHELP_EXE=$<TARGET_FILE:djv>
            -DHELP_OUT=${CMAKE_CURRENT_SOURCE_DIR}/command-line.html
            -DHELP_TMP=${CMAKE_CURRENT_BINARY_DIR}
            -DHELP_CHECK=ON
            -P ${CMAKE_CURRENT_SOURCE_DIR}/Help.cmake
        DEPENDS djv
        COMMENT "Documentation command line page check")
    set_target_properties(djvDocsHelpCheck PROPERTIES FOLDER docs)
endif()

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
