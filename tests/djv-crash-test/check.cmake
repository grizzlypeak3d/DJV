# Run djv-crash-test one way and check what it left in the log.
#
#   cmake -DPROGRAM=... -DHOW=clean|error|crash|terminate -DLOG=... -P check.cmake

execute_process(
    COMMAND ${PROGRAM} ${HOW} ${LOG}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output)
if(output MATCHES "Not installed")
    message(STATUS "The crash log is not installed in this build (a sanitizer)")
    return()
endif()
file(READ ${LOG} text)

function(expect pattern)
    if(NOT text MATCHES "${pattern}")
        message(FATAL_ERROR "The log does not have \"${pattern}\":\n${text}")
    endif()
endfunction()
function(reject pattern)
    if(text MATCHES "${pattern}")
        message(FATAL_ERROR "The log has \"${pattern}\":\n${text}")
    endif()
endfunction()

# What was there before is kept.
expect("^The log before\n")

if(HOW STREQUAL "clean")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Exit code ${result}, not 0")
    endif()
    expect("djv::app::CrashLog: Clean exit\n$")
    reject(": Crash:")
elseif(HOW STREQUAL "error")
    if(NOT result EQUAL 1)
        message(FATAL_ERROR "Exit code ${result}, not 1")
    endif()
    expect("djv::app::CrashLog: Exit on an error: Cannot open the file\n$")
    reject("Clean exit")
elseif(HOW STREQUAL "crash")
    # The earlier handler ran after the line was written.
    if(NOT result EQUAL 42)
        message(FATAL_ERROR "Exit code ${result}, not 42 from the earlier handler")
    endif()
    expect("djv::app::CrashLog: Crash: ")
    reject("Clean exit")
elseif(HOW STREQUAL "terminate")
    if(NOT result EQUAL 43)
        message(FATAL_ERROR "Exit code ${result}, not 43 from the earlier terminate handler")
    endif()
    expect("djv::app::CrashLog: Crash: an exception nothing caught: Nothing caught this")
    reject("Clean exit")
endif()
