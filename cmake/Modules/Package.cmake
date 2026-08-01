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

    # Associate review files (".djvr") with DJV so double-clicking one opens it.
    # The application already routes a ".djvr" argument to the review (see
    # App::_inputFilesInit); these registry entries tell Windows which command to
    # run. The installer is elevated (Program Files), so HKCR resolves to the
    # system-wide HKLM\Software\Classes. SHChangeNotify refreshes the icon cache.
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        WriteRegStr HKCR '.djvr' '' 'DJV.Review'
        WriteRegStr HKCR 'DJV.Review' '' 'DJV Review Session'
        WriteRegStr HKCR 'DJV.Review\\DefaultIcon' '' '\"$INSTDIR\\bin\\djv.exe\",0'
        WriteRegStr HKCR 'DJV.Review\\shell\\open\\command' '' '\"$INSTDIR\\bin\\djv.exe\" \"%1\"'
        System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        DeleteRegKey HKCR 'DJV.Review'
        DeleteRegValue HKCR '.djvr' ''
        DeleteRegKey /ifempty HKCR '.djvr'
        System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
    ")

elseif(APPLE)

    # A disk image holding the application bundle. The Bundle generator would
    # make the .app itself, from a plist and an icon; the bundle is built by
    # the target now, so this only has to carry it.
    if(DJV_MACOS_PACKAGE)
        set(CPACK_GENERATOR DragNDrop)
    else()
        set(CPACK_GENERATOR ZIP)
    endif()

    # Where the bundle's own directories live, inside the install prefix.
    set(DJV_BUNDLE_CONTENTS "djv.app/Contents")

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
        # Beside the executable in the bundle. Its own copy: a bundle that
        # reaches outside itself for its libraries is not one that can be
        # signed, moved or installed on its own.
        #
        # Every destination here is inside the install prefix. These rules are
        # their own component so an ordinary install stays an ordinary Unix
        # prefix, and packaging asks for the component by name below.
        install(FILES ${INSTALL_DYLIBS}
            DESTINATION "${DJV_BUNDLE_CONTENTS}/Frameworks"
            COMPONENT bundle
            EXCLUDE_FROM_ALL)
    else()
        install(FILES ${INSTALL_DYLIBS} DESTINATION lib)
    endif()

    if(DJV_MACOS_PACKAGE)
        set(CPACK_DMG_VOLUME_NAME "DJV ${DJV_VERSION_FULL}")

        install(FILES ${PROJECT_SOURCE_DIR}/etc/macOS/DJV.icns
            DESTINATION "${DJV_BUNDLE_CONTENTS}/Resources"
            COMPONENT bundle
            EXCLUDE_FROM_ALL)

        # Taken from the prefix rather than installed a second time: the build
        # installs before it packages, and this way the install rules stay as
        # they are for every other platform. The documentation is found
        # relative to the executable, at "../Resources/docs".
        foreach(RESOURCE docs Legal SampleData)
            install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/share/djv/${RESOURCE}/
                DESTINATION "${DJV_BUNDLE_CONTENTS}/Resources/${RESOURCE}"
                COMPONENT bundle
                EXCLUDE_FROM_ALL)
        endforeach()

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
# The documentation is its own component, so it has to be named: staging the
# runtime alone shipped packages with no documentation in them, and a Help
# menu that says the documentation is not installed because it is not.
set(CPACK_INSTALL_CMAKE_PROJECTS
    "${CMAKE_BINARY_DIR};${PROJECT_NAME};runtime;/"
    "${CMAKE_BINARY_DIR};${PROJECT_NAME};docs;/")
if(APPLE AND DJV_MACOS_PACKAGE)
    # The bundle alone. A full install is a Unix prefix with the application
    # sitting inside it, and a full install is what the bundle rules opt out
    # of, so it would arrive with no application in it at all.
    set(CPACK_INSTALL_CMAKE_PROJECTS
        "${CMAKE_BINARY_DIR};${PROJECT_NAME};bundle;/")
endif()
