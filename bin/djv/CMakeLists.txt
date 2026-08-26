set(HEADERS)
set(SOURCE main.cpp)
if (WIN32)
    list(APPEND HEADERS winrc.h)
    list(APPEND SOURCE win.rc)
endif()

set(DJV_BIN djv)

add_executable(${DJV_BIN} WIN32 ${HEADERS} ${SOURCE})
target_link_libraries(${DJV_BIN} djvApp)
set_target_properties(${DJV_BIN} PROPERTIES FOLDER bin)

if(APPLE AND DJV_MACOS_PACKAGE)
    # A real application bundle, built here rather than assembled by the
    # packaging: CMake lays out "djv.app/Contents" itself, so nothing
    # downstream installs into a directory above the prefix to reach it, and
    # the executable gets its install rpath rewritten on the way in.
    configure_file(
        ${PROJECT_SOURCE_DIR}/etc/macOS/Info.plist.in
        ${CMAKE_CURRENT_BINARY_DIR}/djv.plist)
    set_target_properties(
        ${DJV_BIN} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/djv.plist)

    install(
        TARGETS ${DJV_BIN}
        BUNDLE DESTINATION .
        COMPONENT bundle
        EXCLUDE_FROM_ALL)
else()
    install(TARGETS ${DJV_BIN} DESTINATION bin)
endif()

if(WIN32)
    add_executable(djv. ${HEADERS} ${SOURCE})
    target_link_libraries(djv. djvApp)
    set_target_properties(
        djv. PROPERTIES
        SUFFIX com
        FOLDER bin)
    install(TARGETS djv. RUNTIME DESTINATION bin)
endif()
