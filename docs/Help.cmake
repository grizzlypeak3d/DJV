# Write the command line page from the application's own help.
#
# The options are declared in the application, which prints them as JSON
# with "-helpJSON", so the page is made from that rather than written by
# hand: a section per option group, and it cannot drift from the binary
# without the check below noticing. The page goes into the repository, unlike the
# licenses and the change log, so that the published site has it and no
# build has to run the application to install the documentation.
#
# The navigation and the table of contents are left as one empty item
# each, for Nav.cmake to fill the way it fills every other page.
#
#   HELP_EXE    the application
#   HELP_OUT    the page to write, or to compare against
#   HELP_TMP    a directory for the settings and log the run would write
#   HELP_CHECK  compare the page's contents with the help and fail on a
#               difference, writing nothing

# Run as a script, which has no project to take the policies from.
cmake_minimum_required(VERSION 3.19)

if(NOT HELP_EXE OR NOT HELP_OUT OR NOT HELP_TMP)
    message(FATAL_ERROR "HELP_EXE, HELP_OUT and HELP_TMP are required")
endif()

# The application would otherwise read and write the user's settings on
# the way to printing the help.
execute_process(
    COMMAND ${HELP_EXE} -helpJSON
        -settingsFile ${HELP_TMP}/help.json
        -logFile ${HELP_TMP}/help.log
    OUTPUT_VARIABLE TEXT
    RESULT_VARIABLE RESULT)
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "Cannot run ${HELP_EXE} for its help")
endif()

function(help_escape TEXT OUT)
    string(REPLACE "&" "&amp;" TEXT "${TEXT}")
    string(REPLACE "<" "&lt;" TEXT "${TEXT}")
    string(REPLACE ">" "&gt;" TEXT "${TEXT}")
    set(${OUT} "${TEXT}" PARENT_SCOPE)
endfunction()

set(BODY)

string(JSON USAGE GET "${TEXT}" usage)
help_escape("${USAGE}" ESCAPED)
string(APPEND BODY "<h2 id=\"usage\">Usage</h2>\n<pre><code>${ESCAPED}</code></pre>\n")

string(JSON COUNT LENGTH "${TEXT}" arguments)
if(COUNT GREATER 0)
    string(APPEND BODY "<h2 id=\"arguments\">Arguments</h2>\n<dl>\n")
    math(EXPR LAST "${COUNT} - 1")
    foreach(I RANGE ${LAST})
        string(JSON NAME GET "${TEXT}" arguments ${I} name)
        string(JSON HELP GET "${TEXT}" arguments ${I} help)
        help_escape("${NAME}" NAME)
        help_escape("${HELP}" HELP)
        string(APPEND BODY "<dt><code>${NAME}</code></dt>\n<dd>${HELP}</dd>\n")
    endforeach()
    string(APPEND BODY "</dl>\n")
endif()

# The groups in the order the options name them, as the help prints
# them, with the options that have no group last.
string(JSON COUNT LENGTH "${TEXT}" options)
set(GROUPS)
math(EXPR LAST "${COUNT} - 1")
foreach(I RANGE ${LAST})
    string(JSON GROUP GET "${TEXT}" options ${I} group)
    list(FIND GROUPS "${GROUP}" FOUND)
    if(NOT "${GROUP}" STREQUAL "" AND FOUND EQUAL -1)
        list(APPEND GROUPS "${GROUP}")
    endif()
endforeach()
# An empty group cannot ride in the list, so the ungrouped options are
# the last pass, named here.
foreach(GROUP ${GROUPS} "")
    if("${GROUP}" STREQUAL "")
        set(TITLE "Options")
        set(ID "options")
    else()
        set(TITLE "${GROUP} options")
        string(TOLOWER "${GROUP}" ID)
        string(REPLACE " " "-" ID "${ID}")
        string(APPEND ID "-options")
    endif()
    set(ENTRIES)
    foreach(I RANGE ${LAST})
        string(JSON OPTION_GROUP GET "${TEXT}" options ${I} group)
        if(NOT OPTION_GROUP STREQUAL GROUP)
            continue()
        endif()
        string(JSON NAMES_COUNT LENGTH "${TEXT}" options ${I} names)
        set(NAMES)
        math(EXPR NAMES_LAST "${NAMES_COUNT} - 1")
        foreach(J RANGE ${NAMES_LAST})
            string(JSON NAME GET "${TEXT}" options ${I} names ${J})
            list(APPEND NAMES "${NAME}")
        endforeach()
        list(JOIN NAMES ", " TERM)
        string(JSON VALUE GET "${TEXT}" options ${I} value)
        if(VALUE)
            string(APPEND TERM " (value)")
        endif()
        string(JSON HELP GET "${TEXT}" options ${I} help)
        help_escape("${TERM}" TERM)
        help_escape("${HELP}" HELP)
        string(APPEND ENTRIES "<dt><code>${TERM}</code></dt>\n<dd>${HELP}</dd>\n")
    endforeach()
    if(NOT ENTRIES STREQUAL "")
        help_escape("${TITLE}" ESCAPED)
        string(APPEND BODY "<h2 id=\"${ID}\">${ESCAPED}</h2>\n<dl>\n${ENTRIES}</dl>\n")
    endif()
endforeach()

set(PAGE "<!DOCTYPE html>
<html lang=\"en\">
<head>
<meta charset=\"utf-8\">
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
<title>Command line - DJV</title>
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
<h1>Command line</h1>
<div class=\"toc\">
<ul>
<li></li>
</ul>
</div>
<p>The options the application takes, as <code>djv -h</code> prints them. The Python version of DJV takes the same options. Commands, which can be given on the command line as well, are described under <a href=\"automation.html\">Automation</a>.</p>
${BODY}</main>
</body>
</html>
")

if(HELP_CHECK)
    # The navigation and the table of contents are written into the
    # committed page after this makes it, so the comparison starts after
    # the table of contents, where the two are made the same way.
    file(READ "${HELP_OUT}" CURRENT)
    foreach(VAR PAGE CURRENT)
        string(FIND "${${VAR}}" "</div>\n" TOC_END)
        if(TOC_END EQUAL -1)
            message(FATAL_ERROR "No table of contents in ${VAR}")
        endif()
        string(SUBSTRING "${${VAR}}" ${TOC_END} -1 ${VAR}_BODY)
    endforeach()
    if(NOT PAGE_BODY STREQUAL CURRENT_BODY)
        message(FATAL_ERROR
            "${HELP_OUT} does not match the application's help: build the "
            "djvDocsHelp target and commit the page")
    endif()
    message(STATUS "${HELP_OUT} matches the application's help")
else()
    file(WRITE "${HELP_OUT}" "${PAGE}")
endif()
