# The licenses page: DJV's own license, and the notices of everything the
# build ships.
#
# Each notice lives in one repository only, the one whose build ships the
# component -- tlRender's (FFmpeg, OpenColorIO, ...) and feather-tk's (zlib,
# FreeType, the fonts, ...) are theirs to keep -- and each adds its own to the
# LEGAL_NOTICES global property as it is built. So there are no copies here to
# drift, a component cannot be listed where it is not built, and one that is
# built cannot be left off. DJV brings nothing in itself beyond what tlRender
# and feather-tk do, so its own entry is its license.
set_property(GLOBAL APPEND PROPERTY LEGAL_NOTICES
    "DJV=${PROJECT_SOURCE_DIR}/LICENSE.txt")

# Built against installed packages -- the wheel -- the notices are the ones
# installed beside them. A package from before the notices moved has no
# *_LEGAL_DIR and installs them in the same place, copies and all; the page
# lists each component once whichever copy it reads.
if(DJV_TLRENDER_PACKAGE)
    if(NOT tlRender_LEGAL_DIR)
        get_filename_component(tlRender_LEGAL_DIR
            "${tlRender_DIR}/../../../share/tlRender/Legal" ABSOLUTE)
    endif()
    if(NOT ftk_LEGAL_DIR)
        get_filename_component(ftk_LEGAL_DIR
            "${ftk_DIR}/../../../share/ftk/Legal" ABSOLUTE)
    endif()
    # feather-tk's first, as a build from source collects them: where a
    # component is listed twice the page keeps the first, which is then the
    # copy of the repository that ships it.
    foreach(DIR ${ftk_LEGAL_DIR} ${tlRender_LEGAL_DIR})
        if(NOT IS_DIRECTORY "${DIR}")
            message(FATAL_ERROR "No notices where the package installs them: ${DIR}")
        endif()
        file(GLOB NOTICES LIST_DIRECTORIES false "${DIR}/LICENSE_*")
        list(SORT NOTICES)
        set_property(GLOBAL APPEND PROPERTY LEGAL_NOTICES "${NOTICES}")
    endforeach()
endif()

# Not installed as text files. Every notice reaches the install inside the
# licenses page, which is documentation, goes wherever the documentation goes,
# and reads as one document rather than a directory of text files nobody
# opens.
#
# At configure time, into the build tree. The page is a description of one
# build -- these notices, because this configuration -- so it is not a file
# to keep in the repository, where it could only describe whoever generated
# it last. The list is handed over as a file rather than a command line, the
# way the navigation is, so a name the shell would take an interest in stays
# one name.
get_property(LEGAL GLOBAL PROPERTY LEGAL_NOTICES)
set(LEGAL_LIST ${CMAKE_CURRENT_BINARY_DIR}/legal.txt)
string(REPLACE ";" "\n" LEGAL_LINES "${LEGAL}")
file(WRITE ${LEGAL_LIST} "${LEGAL_LINES}\n")

set(DJV_LICENSES_PAGE ${CMAKE_CURRENT_BINARY_DIR}/licenses.html PARENT_SCOPE)
execute_process(
    COMMAND ${CMAKE_COMMAND}
        -DLICENSES_LIST=${LEGAL_LIST}
        -DLICENSES_OUT=${CMAKE_CURRENT_BINARY_DIR}/licenses.html
        -P ${PROJECT_SOURCE_DIR}/docs/Licenses.cmake
    COMMAND_ERROR_IS_FATAL ANY)
