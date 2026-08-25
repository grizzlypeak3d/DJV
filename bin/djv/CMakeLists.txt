set(HEADERS)
set(SOURCE main.cpp)
if (WIN32)
    list(APPEND HEADERS winrc.h)
    list(APPEND SOURCE win.rc)
endif()

set(DJV_BIN djv)
set(INSTALL_DIR bin)
set(INSTALL_COMPONENT)
if(APPLE AND DJV_MACOS_PACKAGE)
    # The bundle names the executable in its Info.plist, so this keeps the
    # name it has everywhere else. CPack stages into
    # "DJV.app/Contents/Resources", so ".." is "Contents" and this is
    # "Contents/MacOS"; outside CPack it would land beside the install prefix,
    # which is why it belongs to the bundle component. See
    # cmake/Modules/Package.cmake.
    set(INSTALL_DIR ../MacOS)
    set(INSTALL_COMPONENT COMPONENT bundle EXCLUDE_FROM_ALL)
endif()

add_executable(${DJV_BIN} WIN32 ${HEADER} ${SOURCE})
target_link_libraries(${DJV_BIN} djvApp)
set_target_properties(${DJV_BIN} PROPERTIES FOLDER bin)

install(TARGETS ${DJV_BIN} DESTINATION ${INSTALL_DIR} ${INSTALL_COMPONENT})

if(WIN32)
    add_executable(djv. ${HEADER} ${SOURCE})
    target_link_libraries(djv. djvApp)
    set_target_properties(
        djv. PROPERTIES
        SUFFIX com
        FOLDER bin)
    install(TARGETS djv. RUNTIME DESTINATION bin)
endif()
