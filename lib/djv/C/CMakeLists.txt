set(HEADERS
    Models.h)
set(SOURCE
    Models.cpp)

# The C interface to the models, over tlRender's C interface to the
# engine; for Swift. Static: it is linked into one application, so its
# functions need no export macro (DJV_C_API is empty).
add_library(djvC STATIC ${HEADERS} ${SOURCE})
target_link_libraries(djvC PUBLIC djvModels tlRender::tlC)
set_target_properties(djvC PROPERTIES FOLDER lib)
set_target_properties(djvC PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvC
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/djv/C)
install(
    FILES module.modulemap
    DESTINATION include/djv/C)
