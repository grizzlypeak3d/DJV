set(HEADERS
    Shortcuts.h)
set(HEADERS_PRIVATE)

set(SOURCE
    Shortcuts.cpp)

add_library(djvCore ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
target_link_libraries(djvCore djvResource tlRender::tlCore)
set_target_properties(djvCore PROPERTIES FOLDER lib)
set_target_properties(djvCore PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvCore
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djv/Core)
