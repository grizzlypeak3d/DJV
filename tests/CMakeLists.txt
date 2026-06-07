find_package(Python3 COMPONENTS Interpreter QUIET)
find_program(FFMPEG_EXECUTABLE NAMES ffmpeg)
find_program(FFPROBE_EXECUTABLE NAMES ffprobe)

if(Python3_Interpreter_FOUND AND FFMPEG_EXECUTABLE AND FFPROBE_EXECUTABLE)
    add_test(
        NAME media_compat_ffmpeg_smoke
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/media-compat/media_compat.py
            --ffmpeg ${FFMPEG_EXECUTABLE}
            --ffprobe ${FFPROBE_EXECUTABLE}
            --output-dir ${CMAKE_CURRENT_BINARY_DIR}/media_compat
            --json ${CMAKE_CURRENT_BINARY_DIR}/media_compat/results.json)
endif()
