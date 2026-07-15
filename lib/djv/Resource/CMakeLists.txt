set(HEADERS)
set(SOURCE)
set(RESOURCES
    etc/Icons/DJV_Icon.svg
    etc/Icons/ViewTimelineRelativeZoom.svg)
foreach(RESOURCE ${RESOURCES})
    get_filename_component(RESOURCE_BASE ${RESOURCE} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_BASE}.cpp
        COMMAND ftk-resource ${PROJECT_SOURCE_DIR}/${RESOURCE} ${RESOURCE_BASE} djv_resource
        DEPENDS ${PROJECT_SOURCE_DIR}/${RESOURCE})
    list(APPEND SOURCE ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_BASE}.cpp)
endforeach()

add_library(djvResource ${HEADERS} ${SOURCE})

target_link_libraries(djvResource ZLIB::ZLIB)

set_target_properties(djvResource PROPERTIES FOLDER lib)
set_target_properties(djvResource PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvResource
    EXPORT djvResourceTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)
install(
    EXPORT djvResourceTargets
    FILE djvResourceTargets.cmake
    DESTINATION "lib/cmake/djv"
    NAMESPACE djv::)

