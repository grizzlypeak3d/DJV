# Write the licenses page from the notices that ship with the build.
#
# The notices are plain text and there are dozens of them, so the page is
# generated rather than kept by hand. Each notice lives in the repository whose
# build ships the component -- feather-tk, tlRender, DJV, or a project built on
# DJV -- and each of those adds its own to the list this reads; see
# etc/Legal/CMakeLists.txt. A notice cannot arrive in a package without
# arriving here.
#
# The navigation and the table of contents are left as one empty item each,
# for Nav.cmake to fill the way it fills every other page.
#
#   LICENSES_LIST   a file naming the notices, one per line: a path, whose
#                   file name says what it is ("LICENSE_OpenColorIO.txt" is
#                   OpenColorIO's), or "Name=path" for one whose file name
#                   does not (a project's own LICENSE.txt)
#   LICENSES_OUT    the page to write

if(NOT LICENSES_LIST OR NOT LICENSES_OUT)
    message(FATAL_ERROR "LICENSES_LIST and LICENSES_OUT are required")
endif()

file(STRINGS "${LICENSES_LIST}" LINES)

# Name each notice, and keep the first of a name. A package built before its
# notices moved to one repository each still installs copies of the ones below
# it, and the page lists a component once whoever else carried it.
set(NAMES)
foreach(LINE ${LINES})
    if(LINE MATCHES "^([^=/\\]+)=(.+)$")
        set(NAME "${CMAKE_MATCH_1}")
        set(PATH "${CMAKE_MATCH_2}")
    else()
        set(PATH "${LINE}")
        # "LICENSE_OpenColorIO.txt" is the notice for OpenColorIO, and
        # "LICENSE_DJV Studio.txt" the notice for DJV Studio: the file name
        # is the component's name, spaces and all, rather than a name with a
        # rule to undo. What a component calls itself is not this script's
        # to guess.
        get_filename_component(NAME "${PATH}" NAME_WE)
        string(REGEX REPLACE "^LICENSE_" "" NAME "${NAME}")
    endif()
    if(NOT EXISTS "${PATH}")
        message(FATAL_ERROR "No such notice: ${PATH}")
    endif()
    string(TOLOWER "${NAME}" LOWER)
    string(MAKE_C_IDENTIFIER "${LOWER}" KEY)
    if(NOT DEFINED NOTICE_${KEY})
        set(NOTICE_${KEY} "${PATH}")
        set(NOTICE_NAME_${KEY} "${NAME}")
        set(NOTICE_ID_${KEY} "${LOWER}")
        list(APPEND NAMES "${KEY}")
    endif()
endforeach()

# Alphabetical, whatever order the list is in: the page is a reference to
# look a component up in, and the list it comes from is a build's shipping
# manifest, which has its own reasons for the order it is in.
list(SORT NAMES CASE INSENSITIVE)

set(BODY)
foreach(KEY ${NAMES})
    set(NAME "${NOTICE_NAME_${KEY}}")
    string(REPLACE " " "-" ID "${NOTICE_ID_${KEY}}")

    file(READ "${NOTICE_${KEY}}" TEXT)
    # Ampersand first, or the escapes introduced below are escaped again.
    string(REPLACE "&" "&amp;" TEXT "${TEXT}")
    string(REPLACE "<" "&lt;" TEXT "${TEXT}")
    string(REPLACE ">" "&gt;" TEXT "${TEXT}")
    string(REPLACE "\r\n" "\n" TEXT "${TEXT}")
    string(REGEX REPLACE "[ \t\r\n]+$" "" TEXT "${TEXT}")

    string(APPEND BODY
        "<h2 id=\"${ID}\">${NAME}</h2>\n<pre>${TEXT}</pre>\n")
endforeach()

set(PAGE "<!DOCTYPE html>
<html lang=\"en\">
<head>
<meta charset=\"utf-8\">
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
<title>Licenses - DJV</title>
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
<h1>Licenses</h1>
<div class=\"toc\">
<ul>
<li></li>
</ul>
</div>
<p>This build of the application includes the components below, and
reproduces each of their notices in full.</p>
${BODY}</main>
</body>
</html>
")

file(WRITE "${LICENSES_OUT}" "${PAGE}")
