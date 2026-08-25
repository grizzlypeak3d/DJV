set(HEADERS)
set(SOURCE)

# The icons go through ftk_icon_resources(): adding an icon is adding a
# file to etc/Icons, and the main window registers the generated map.
ftk_icon_resources(SOURCE ${PROJECT_SOURCE_DIR}/etc/Icons djv_resource)

# Static, whatever the rest of the build is. The resources are data at
# namespace scope with no export macro on them, so a shared build on Windows
# produces a library that exports nothing and no import library beside it:
#
#     LINK : fatal error LNK1104: cannot open file 'djvResource.lib'
#
# CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS does not answer this the way it does for
# the dependencies: it writes a module definition file from the functions the
# objects hold, and these hold none. Marking the data instead would mean the
# generator and every extern declaration that names it.
#
# Static suits them anyway. They are an implementation detail rather than API,
# and each resource is its own object file, so a consumer links only the ones
# it names -- its consumers take the ones they use and nothing more.
add_library(djvResource STATIC ${HEADERS} ${SOURCE})

# The generated headers are found here; the build interface only, since
# the resources are an implementation detail rather than installed API.
target_include_directories(djvResource PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties(djvResource PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(djvResource ZLIB::ZLIB)

set_target_properties(djvResource PROPERTIES FOLDER lib)
set_target_properties(djvResource PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvResource
    EXPORT djvResourceTargets
    ARCHIVE DESTINATION lib COMPONENT dev
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)
install(
    EXPORT djvResourceTargets
    FILE djvResourceTargets.cmake
    DESTINATION "lib/cmake/djv"
    NAMESPACE djv::
    COMPONENT dev)

