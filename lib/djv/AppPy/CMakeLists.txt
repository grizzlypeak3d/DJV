set(HEADERS
    Bindings.h)
set(HEADERS_PRIVATE)
set(SOURCE
    App.cpp
    Bindings.cpp
    IToolWidget.cpp
    MainWindow.cpp)

# Static. The bindings are linked by the djv module and nothing else, so there is no
# API here for anything to import -- the module absorbs them.
add_library(djvAppPy STATIC ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
# Hidden, as the module is: the binding classes hold nanobind's types,
# which are hidden, and GCC warns about a class more visible than its
# members. Nothing here is exported anyway.
set_target_properties(djvAppPy PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)

target_include_directories(djvAppPy
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>)
target_include_directories(djvAppPy
    PRIVATE
        ${NB_DIR}/include
        ${NB_DIR}/ext/robin_map/include)

target_link_libraries(djvAppPy PUBLIC djvApp PRIVATE ${DJV_NANOBIND_LIBRARY})

target_compile_options(djvAppPy PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

set_target_properties(djvAppPy PROPERTIES FOLDER lib)
set_target_properties(djvAppPy PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(TARGETS djvAppPy
    ARCHIVE DESTINATION lib COMPONENT dev
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djv/AppPy COMPONENT dev)
