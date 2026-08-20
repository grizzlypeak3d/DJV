pybind11_add_module(djvPy Bindings.cpp)

target_include_directories(djvPy PRIVATE ${PROJECT_SOURCE_DIR}/lib)

target_link_libraries(djvPy PRIVATE djvModelsPy)

set_target_properties(djvPy PROPERTIES FOLDER lib)

install(TARGETS djvPy
    ARCHIVE DESTINATION lib COMPONENT dev
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)
