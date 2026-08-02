# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

# Write the build information header. Run with cmake -P, both when the project
# is configured and on every build, so that the commit does not go stale after
# a commit is made without reconfiguring. The header is only replaced when its
# contents change, or every build would relink.
#
# The date is the commit's, not the clock's: a date that changes between two
# builds of the same source would relink everything once a day and would make
# builds unreproducible, for a value that says no more than the commit does.
#
# Expects SRC_DIR, IN_FILE and OUT_FILE.

set(DJV_GIT_COMMIT "unknown")
set(DJV_COMMIT_DATE "unknown")
find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${SRC_DIR}
        OUTPUT_VARIABLE _hash
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _result)
    if(_result EQUAL 0)
        set(DJV_GIT_COMMIT "${_hash}")

        execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd --date=short
            WORKING_DIRECTORY ${SRC_DIR}
            OUTPUT_VARIABLE _date
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        if(_date)
            set(DJV_COMMIT_DATE "${_date}")
        endif()

        # Marked when anything is uncommitted, which includes a submodule whose
        # contents differ from the commit recorded for it: the hash names one
        # build only when everything below it is where the commit says.
        execute_process(
            COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
            WORKING_DIRECTORY ${SRC_DIR}
            RESULT_VARIABLE _dirty
            ERROR_QUIET)
        if(NOT _dirty EQUAL 0)
            set(DJV_GIT_COMMIT "${DJV_GIT_COMMIT}-dirty")
        endif()
    endif()
endif()

configure_file(${IN_FILE} ${OUT_FILE}.tmp @ONLY)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${OUT_FILE}.tmp ${OUT_FILE})
file(REMOVE ${OUT_FILE}.tmp)
