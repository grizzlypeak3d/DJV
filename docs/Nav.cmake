# Write the navigation into the documentation pages, and a table of
# contents into the pages that carry one.
#
# The pages have to be self-contained -- they are read from disk as often as
# from a server, and a file:// origin cannot fetch anything -- so each one
# carries the whole list. That is a copy per page, which is fine as long as
# nobody maintains them: the list lives in nav.txt and this puts it in. The
# table of contents is built the same way, from the page's own "h2" headings,
# on any page holding a <div class="toc"> block.
#
# Run over the sources to update the repository, and over an installed copy
# to add a section that only that install has. See docs/CMakeLists.txt.
#
#   NAV_DIR    the directory of pages to write
#   NAV_LIST   the list to write
#   NAV_EXTRA  a further list, appended (optional)
#   NAV_CHECK  report pages that would change and fail, writing nothing

if(NOT NAV_DIR OR NOT NAV_LIST)
    message(FATAL_ERROR "NAV_DIR and NAV_LIST are required")
endif()

# Read a list file into ENTRIES, as "page<TAB>title" with an empty page for a
# section heading.
function(nav_read PATH OUT)
    set(ENTRIES ${${OUT}})
    file(STRINGS "${PATH}" LINES)
    foreach(LINE ${LINES})
        if(LINE STREQUAL "" OR LINE MATCHES "^#")
            continue()
        endif()
        if(LINE MATCHES "^= (.+)$")
            list(APPEND ENTRIES "\t${CMAKE_MATCH_1}")
            continue()
        endif()
        string(FIND "${LINE}" " " SPACE)
        if(SPACE EQUAL -1)
            message(FATAL_ERROR "No title in ${PATH}: \"${LINE}\"")
        endif()
        string(SUBSTRING "${LINE}" 0 ${SPACE} PAGE)
        math(EXPR AFTER "${SPACE} + 1")
        string(SUBSTRING "${LINE}" ${AFTER} -1 TITLE)
        list(APPEND ENTRIES "${PAGE}\t${TITLE}")
    endforeach()
    set(${OUT} ${ENTRIES} PARENT_SCOPE)
endfunction()

set(ENTRIES)
nav_read("${NAV_LIST}" ENTRIES)
if(NAV_EXTRA)
    nav_read("${NAV_EXTRA}" ENTRIES)
endif()

# Between these the list is written; every page has them, and the first "<ul>"
# is the navigation's because the navigation comes before the content.
set(OPEN "<ul>\n")
set(CLOSE "\n</ul>\n</nav>")

set(CHANGED)
file(GLOB PAGES "${NAV_DIR}/*.html")
foreach(PAGE ${PAGES})
    get_filename_component(NAME "${PAGE}" NAME)

    set(LIST_HTML)
    foreach(ENTRY ${ENTRIES})
        string(FIND "${ENTRY}" "\t" TAB)
        string(SUBSTRING "${ENTRY}" 0 ${TAB} HREF)
        math(EXPR AFTER "${TAB} + 1")
        string(SUBSTRING "${ENTRY}" ${AFTER} -1 TITLE)
        if(HREF STREQUAL "")
            set(LINE "<li class=\"section\">${TITLE}</li>")
        elseif(HREF STREQUAL NAME)
            set(LINE "<li><a class=\"current\" href=\"${HREF}\">${TITLE}</a></li>")
        else()
            set(LINE "<li><a href=\"${HREF}\">${TITLE}</a></li>")
        endif()
        if(LIST_HTML)
            set(LIST_HTML "${LIST_HTML}\n${LINE}")
        else()
            set(LIST_HTML "${LINE}")
        endif()
    endforeach()

    file(READ "${PAGE}" TEXT)
    string(FIND "${TEXT}" "${OPEN}" START)
    string(FIND "${TEXT}" "${CLOSE}" END)
    if(START EQUAL -1 OR END EQUAL -1 OR END LESS START)
        message(WARNING "No navigation in ${NAME}")
        continue()
    endif()
    string(LENGTH "${OPEN}" OPEN_LENGTH)
    math(EXPR START "${START} + ${OPEN_LENGTH}")
    math(EXPR LENGTH "${END} - ${START}")
    string(SUBSTRING "${TEXT}" 0 ${START} BEFORE)
    string(SUBSTRING "${TEXT}" ${END} -1 AFTER)
    set(UPDATED "${BEFORE}${LIST_HTML}${AFTER}")

    # The table of contents, on the pages that carry the block.
    set(TOC_OPEN "<div class=\"toc\">\n<ul>\n")
    set(TOC_CLOSE "\n</ul>\n</div>")
    string(FIND "${UPDATED}" "${TOC_OPEN}" TOC_START)
    if(NOT TOC_START EQUAL -1)
        string(REGEX MATCHALL "<h2 id=\"[^\"]+\">[^<]*</h2>" HEADINGS "${UPDATED}")
        set(TOC_HTML)
        foreach(HEADING ${HEADINGS})
            string(REGEX REPLACE
                "<h2 id=\"([^\"]+)\">([^<]*)</h2>"
                "<li><a href=\"#\\1\">\\2</a></li>"
                LINE "${HEADING}")
            if(TOC_HTML)
                set(TOC_HTML "${TOC_HTML}\n${LINE}")
            else()
                set(TOC_HTML "${LINE}")
            endif()
        endforeach()
        string(LENGTH "${TOC_OPEN}" TOC_OPEN_LENGTH)
        math(EXPR TOC_CONTENT "${TOC_START} + ${TOC_OPEN_LENGTH}")
        string(SUBSTRING "${UPDATED}" ${TOC_CONTENT} -1 TOC_REST)
        string(FIND "${TOC_REST}" "${TOC_CLOSE}" TOC_END)
        if(TOC_END EQUAL -1)
            message(WARNING "No table of contents end in ${NAME}")
        else()
            string(SUBSTRING "${UPDATED}" 0 ${TOC_CONTENT} TOC_BEFORE)
            string(SUBSTRING "${TOC_REST}" ${TOC_END} -1 TOC_AFTER)
            set(UPDATED "${TOC_BEFORE}${TOC_HTML}${TOC_AFTER}")
        endif()
    endif()

    if(UPDATED STREQUAL TEXT)
        continue()
    endif()
    list(APPEND CHANGED "${NAME}")
    if(NOT NAV_CHECK)
        file(WRITE "${PAGE}" "${UPDATED}")
    endif()
endforeach()

if(NAV_CHECK AND CHANGED)
    string(REPLACE ";" ", " CHANGED "${CHANGED}")
    message(FATAL_ERROR
        "The navigation in these pages is not what nav.txt says: ${CHANGED}")
endif()
