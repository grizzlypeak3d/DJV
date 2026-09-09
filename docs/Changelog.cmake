# Write the change log page from ChangeLog.md.
#
# The change log is kept as Markdown at the top of the repository, where
# GitHub shows it; this puts the same text into the documentation, which
# is installed beside the application -- the only copy a DJV Studio user
# has -- and published with the rest of the pages. The Markdown is simple
# enough to convert here: version headings, "Changes:" and "Fixes:"
# labels, and bullet items that may wrap onto indented lines. Nothing
# else is expected, and a line that fits none of those is kept as it is.
#
# The navigation and the table of contents are left as one empty item
# each, for Nav.cmake to fill the way it fills every other page.
#
#   CHANGELOG_IN     ChangeLog.md
#   CHANGELOG_OUT    the page to write
#   CHANGELOG_TITLE  the page's title (optional; "Change log")

if(NOT CHANGELOG_IN OR NOT CHANGELOG_OUT)
    message(FATAL_ERROR "CHANGELOG_IN and CHANGELOG_OUT are required")
endif()
if(NOT CHANGELOG_TITLE)
    set(CHANGELOG_TITLE "Change log")
endif()

file(STRINGS "${CHANGELOG_IN}" LINES)

function(changelog_escape TEXT OUT)
    # Ampersand first, or the escapes introduced below are escaped again.
    string(REPLACE "&" "&amp;" TEXT "${TEXT}")
    string(REPLACE "<" "&lt;" TEXT "${TEXT}")
    string(REPLACE ">" "&gt;" TEXT "${TEXT}")
    set(${OUT} "${TEXT}" PARENT_SCOPE)
endfunction()

set(BODY)
set(ITEM)
set(IN_LIST OFF)

# The item being gathered is written when the next thing starts.
macro(changelog_flush_item)
    # Quoted: an unset variable in if() would otherwise be read as its name.
    if(NOT "${ITEM}" STREQUAL "")
        changelog_escape("${ITEM}" ESCAPED)
        string(APPEND BODY "<li>${ESCAPED}</li>\n")
        set(ITEM)
    endif()
endmacro()
macro(changelog_close_list)
    changelog_flush_item()
    if(IN_LIST)
        string(APPEND BODY "</ul>\n")
        set(IN_LIST OFF)
    endif()
endmacro()

foreach(LINE ${LINES})
    if(LINE MATCHES "^## (.+)$")
        changelog_close_list()
        set(VERSION "${CMAKE_MATCH_1}")
        changelog_escape("${VERSION}" ESCAPED)
        string(APPEND BODY "<h2 id=\"v${VERSION}\">${ESCAPED}</h2>\n")
    elseif(LINE MATCHES "^([A-Za-z]+):$")
        changelog_close_list()
        string(APPEND BODY "<h3>${CMAKE_MATCH_1}</h3>\n")
    elseif(LINE MATCHES "^\\* (.*)$")
        changelog_flush_item()
        if(NOT IN_LIST)
            string(APPEND BODY "<ul>\n")
            set(IN_LIST ON)
        endif()
        set(ITEM "${CMAKE_MATCH_1}")
    elseif(LINE MATCHES "^  +(.*)$" AND NOT "${ITEM}" STREQUAL "")
        string(APPEND ITEM " ${CMAKE_MATCH_1}")
    elseif(LINE STREQUAL "")
        changelog_close_list()
    else()
        changelog_close_list()
        changelog_escape("${LINE}" ESCAPED)
        string(APPEND BODY "<p>${ESCAPED}</p>\n")
    endif()
endforeach()
changelog_close_list()

set(PAGE "<!DOCTYPE html>
<html lang=\"en\">
<head>
<meta charset=\"utf-8\">
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
<title>${CHANGELOG_TITLE} - DJV</title>
<link rel=\"stylesheet\" href=\"assets/docs.css\">
<link rel=\"icon\" href=\"assets/DJV_Icon_32.svg\">
</head>
<body>
<input type=\"checkbox\" id=\"nav-toggle\">
<label for=\"nav-toggle\" id=\"nav-button\">Menu</label>
<nav>
<a class=\"logo\" href=\"index.html\"><img src=\"assets/DJV_Icon_32.svg\" alt=\"\">DJV</a>
<ul>
<li></li>
</ul>
</nav>
<main>
<h1>${CHANGELOG_TITLE}</h1>
<div class=\"toc\">
<ul>
<li></li>
</ul>
</div>
${BODY}</main>
</body>
</html>
")

file(WRITE "${CHANGELOG_OUT}" "${PAGE}")
