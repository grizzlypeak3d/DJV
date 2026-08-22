set(SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
if(Darwin STREQUAL SYSTEM_NAME)
    set(SYSTEM_NAME macos)
endif()
string(TOLOWER
    djv-${DJV_VERSION_FULL}-${SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}
    CPACK_PACKAGE_FILE_NAME)
set(CPACK_PACKAGE_DESCRIPTION "DJV is an open source application for media playback and review.")
set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE.txt)
set(CPACK_PACKAGE_EXECUTABLES djv "DJV ${DJV_VERSION_FULL}")
set(CPACK_PACKAGE_VENDOR "Grizzly Peak 3D")
set(CPACK_VERBATIM_VARIABLES YES)

if(WIN32)
    set(CPACK_GENERATOR ZIP NSIS)
    
    set(INSTALL_DLLS)

    if(TLRENDER_FFMPEG)
        set(FFMPEG_DLLS
            ${CMAKE_INSTALL_PREFIX}/bin/avcodec-62.dll
            ${CMAKE_INSTALL_PREFIX}/bin/avdevice-62.dll
            ${CMAKE_INSTALL_PREFIX}/bin/avformat-62.dll
            ${CMAKE_INSTALL_PREFIX}/bin/avutil-60.dll
            ${CMAKE_INSTALL_PREFIX}/bin/swresample-6.dll
            ${CMAKE_INSTALL_PREFIX}/bin/swscale-9.dll)
        list(APPEND INSTALL_DLLS ${FFMPEG_DLLS})
    endif()

    # LibRaw arrives through OpenImageIO rather than being asked for here,
    # and whether the super build made it is not something this build is
    # told: TLRENDER_LIBRAW is a super build option and does not exist in
    # this scope, so testing it here was always false and the libraries were
    # never packaged at all. What the prefix holds is the answer, and it
    # does not go stale when LibRaw changes its version.
    file(GLOB LIBRAW_DLLS "${CMAKE_INSTALL_PREFIX}/bin/raw*.dll")
    list(APPEND INSTALL_DLLS ${LIBRAW_DLLS})
    
    
    install(FILES ${INSTALL_DLLS} DESTINATION bin)

    set(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/etc/Windows/DJV_Icon.ico)
    set(CPACK_NSIS_MUI_UNIICON ${PROJECT_SOURCE_DIR}/etc/Windows/DJV_Icon.ico)
    set(CPACK_NSIS_INSTALLED_ICON_NAME bin/djv.exe)

elseif(APPLE)

    if(DJV_MACOS_PACKAGE)
        set(CPACK_GENERATOR Bundle)
    else()
        set(CPACK_GENERATOR ZIP)
    endif()

    set(INSTALL_DYLIBS)
    
    if(TLRENDER_FFMPEG)
        set(FFMPEG_DYLIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.62.28.102.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.62.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.62.3.102.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.62.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.62.12.102.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.62.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.60.26.102.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.60.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.6.3.102.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.6.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.9.5.102.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.9.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.dylib)
        list(APPEND INSTALL_DYLIBS ${FFMPEG_DYLIBS})
    endif()

    # LibRaw arrives through OpenImageIO rather than being asked for here,
    # and whether the super build made it is not something this build is
    # told: TLRENDER_LIBRAW is a super build option and does not exist in
    # this scope, so testing it here was always false and the libraries were
    # never packaged at all. What the prefix holds is the answer, and it
    # does not go stale when LibRaw changes its version.
    file(GLOB LIBRAW_DYLIBS "${CMAKE_INSTALL_PREFIX}/lib/libraw*.dylib")
    list(APPEND INSTALL_DYLIBS ${LIBRAW_DYLIBS})
    

    if(DJV_MACOS_PACKAGE)
        # \bug Why do we need to use ".." to avoid installing into the
        # "Resources" directory in the bundle?
        install(FILES ${INSTALL_DYLIBS} DESTINATION ../Frameworks)
    else()
        install(FILES ${INSTALL_DYLIBS} DESTINATION lib)
    endif()

    if(DJV_MACOS_PACKAGE)
        set(CPACK_BUNDLE_NAME DJV)
        configure_file(
            ${PROJECT_SOURCE_DIR}/etc/macOS/Info.plist.in
            ${PROJECT_BINARY_DIR}/Info.plist)
        set(CPACK_BUNDLE_PLIST ${PROJECT_BINARY_DIR}/Info.plist)
        set(CPACK_BUNDLE_ICON ${PROJECT_SOURCE_DIR}/etc/macOS/DJV.icns)
        install(FILES ${PROJECT_BINARY_DIR}/Info.plist DESTINATION "..")
        install(FILES ${PROJECT_SOURCE_DIR}/etc/macOS/DJV.icns DESTINATION ".")

        set(POST_BUILD_SCRIPTS)
        set(DJV_MACOS_TEAM_ID $ENV{DJV_MACOS_TEAM_ID})
        if(DJV_MACOS_TEAM_ID)
            list(APPEND PRE_BUILD_SCRIPTS
                "${PROJECT_SOURCE_DIR}/cmake/Modules/macOSAppSign.cmake")
            list(APPEND POST_BUILD_SCRIPTS
                "${PROJECT_SOURCE_DIR}/cmake/Modules/macOSPackageSign.cmake")
        endif()
        set(CPACK_PRE_BUILD_SCRIPTS ${PRE_BUILD_SCRIPTS})
        set(CPACK_POST_BUILD_SCRIPTS ${POST_BUILD_SCRIPTS})
    endif()

else()

    set(CPACK_GENERATOR TGZ)

    set(INSTALL_LIBS)
    
    if(TLRENDER_FFMPEG)
        set(FFMPEG_LIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.so
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.so.62
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.so.62.28.102
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.so
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.so.62
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.so.62.3.102
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.so
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.so.62
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.so.62.12.102
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.so
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.so.60
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.so.60.26.102
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.so
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.so.6
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.so.6.3.102
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.so
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.so.9
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.so.9.5.102)
        list(APPEND INSTALL_LIBS ${FFMPEG_LIBS})
    endif()

    # LibRaw arrives through OpenImageIO rather than being asked for here,
    # and whether the super build made it is not something this build is
    # told: TLRENDER_LIBRAW is a super build option and does not exist in
    # this scope, so testing it here was always false and the libraries were
    # never packaged at all. What the prefix holds is the answer, and it
    # does not go stale when LibRaw changes its version.
    file(GLOB LIBRAW_LIBS "${CMAKE_INSTALL_PREFIX}/lib/libraw*.so*")
    list(APPEND INSTALL_LIBS ${LIBRAW_LIBS})

    
    install(FILES ${INSTALL_LIBS} DESTINATION lib)

endif()

# Stage the runtime component alone, leaving out what the install rules mark
# as "dev": the development files that are around two fifths of the download,
# and that nothing running the application reads.
set(CPACK_INSTALL_CMAKE_PROJECTS
    "${CMAKE_BINARY_DIR};${PROJECT_NAME};runtime;/")
