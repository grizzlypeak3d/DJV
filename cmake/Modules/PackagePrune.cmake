# Take out of the staged tree what a package has no use for: the static
# libraries, headers and CMake configuration that every library in DJV,
# tlRender and feather-tk installs. Together they are around two fifths of
# the download, and nothing running the application reads any of them.
#
# Only packaging is affected. Installing to a prefix still gets everything,
# so building from source and installing is unchanged.
#
# Temporary. The libraries should not be installing these into a package
# build at all, which means install components across three projects; this
# reaches the same result from one file until that is done.

# Plain comparisons rather than IN_LIST: this is run on its own by CPack,
# where the policy that makes IN_LIST an operator is not set.
file(GLOB_RECURSE entries LIST_DIRECTORIES true
    "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/*")

set(pruneDirs)
set(pruneFiles)
foreach(entry ${entries})
    get_filename_component(name "${entry}" NAME)
    if(IS_DIRECTORY "${entry}")
        if(name STREQUAL "include" OR name STREQUAL "cmake")
            list(APPEND pruneDirs "${entry}")
        endif()
    elseif(entry MATCHES "\\.(a|lib)$")
        list(APPEND pruneFiles "${entry}")
    endif()
endforeach()

foreach(dir ${pruneDirs})
    file(RELATIVE_PATH rel "${CPACK_TEMPORARY_INSTALL_DIRECTORY}" "${dir}")
    message(STATUS "Package prune: ${rel}")
    file(REMOVE_RECURSE "${dir}")
endforeach()

# After the directories, so that anything inside one of them is already gone
# and is not reported twice.
set(pruneFileCount 0)
foreach(entry ${pruneFiles})
    if(EXISTS "${entry}")
        file(REMOVE "${entry}")
        math(EXPR pruneFileCount "${pruneFileCount} + 1")
    endif()
endforeach()
message(STATUS "Package prune: ${pruneFileCount} static libraries")
