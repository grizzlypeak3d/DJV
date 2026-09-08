# Write the licenses page from the notice files that ship beside it.
#
# The notices are plain text and there are dozens of them, so the page is
# generated rather than kept by hand: the list that installs them is the list
# that writes this, and a notice cannot arrive in a package without arriving
# here. See etc/Legal/CMakeLists.txt.
#
# The navigation and the table of contents are left as one empty item each,
# for Nav.cmake to fill the way it fills every other page.
#
#   LICENSES_DIR    the directory holding the notice files
#   LICENSES_LIST   a file naming the notices, one per line
#   LICENSES_OUT    the page to write

if(NOT LICENSES_DIR OR NOT LICENSES_LIST OR NOT LICENSES_OUT)
    message(FATAL_ERROR "LICENSES_DIR, LICENSES_LIST and LICENSES_OUT are required")
endif()

# Alphabetical, whatever order the list is in: the page is a reference to
# look a component up in, and the list it comes from is a build's shipping
# manifest, which has its own reasons for the order it is in.
file(STRINGS "${LICENSES_LIST}" FILES)
list(SORT FILES CASE INSENSITIVE)

set(BODY)
foreach(FILE ${FILES})
    set(PATH "${LICENSES_DIR}/${FILE}")
    if(NOT EXISTS "${PATH}")
        message(FATAL_ERROR "No such notice: ${PATH}")
    endif()

    # "LICENSE_OpenColorIO.txt" is the notice for OpenColorIO. The file names
    # are the component names, which is why they are worth keeping tidy.
    get_filename_component(NAME "${FILE}" NAME_WE)
    string(REGEX REPLACE "^LICENSE_" "" NAME "${NAME}")
    string(TOLOWER "${NAME}" ID)

    file(READ "${PATH}" TEXT)
    # Ampersand first, or the escapes introduced below are escaped again.
    string(REPLACE "&" "&amp;" TEXT "${TEXT}")
    string(REPLACE "<" "&lt;" TEXT "${TEXT}")
    string(REPLACE ">" "&gt;" TEXT "${TEXT}")
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
