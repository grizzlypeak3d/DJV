# The libraries of the super build, gathered out of the install prefix and
# added to the package. A build against the system's packages has none of
# them there -- DJV_PACKAGE_DEPS is off for it -- and the package then holds
# DJV alone, which is what the system it is installed on already expects.
# The package itself is made either way: see the packaging section of
# CMakeLists.txt.
# The libraries that travel with the application are found in the prefix
# rather than named here. A list written by hand goes stale every time a
# dependency changes its version -- silently, since a file that is not
# listed is simply not packaged -- and the version is in the file name of
# every one of them. The globs are read when this build is configured, by
# which point the super build has made what it is going to make.
macro(djv_install_dep)
    if(DJV_PACKAGE_DEPS)
        install(${ARGN})
    endif()
endmacro()

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
        file(GLOB FFMPEG_DLLS
            "${CMAKE_INSTALL_PREFIX}/bin/avcodec-*.dll"
            "${CMAKE_INSTALL_PREFIX}/bin/avdevice-*.dll"
            "${CMAKE_INSTALL_PREFIX}/bin/avformat-*.dll"
            "${CMAKE_INSTALL_PREFIX}/bin/avutil-*.dll"
            "${CMAKE_INSTALL_PREFIX}/bin/swresample-*.dll"
            "${CMAKE_INSTALL_PREFIX}/bin/swscale-*.dll")
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
    
    if(TLRENDER_USD)
        file(GLOB MATERIALX_DLLS
            "${CMAKE_INSTALL_PREFIX}/bin/MaterialX*.dll")
        file(GLOB TBB_DLLS
            "${CMAKE_INSTALL_PREFIX}/bin/tbb*.dll")
        file(GLOB USD_DLLS
            "${CMAKE_INSTALL_PREFIX}/bin/usd_*.dll"
            "${CMAKE_INSTALL_PREFIX}/bin/osd*.dll")
        list(APPEND INSTALL_DLLS ${MATERIALX_DLLS} ${TBB_DLLS} ${USD_DLLS})

        djv_install_dep(
            DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/usd
            DESTINATION bin)
        djv_install_dep(
            DIRECTORY ${CMAKE_INSTALL_PREFIX}/plugin
            DESTINATION ".")
    endif()
    
    djv_install_dep(FILES ${INSTALL_DLLS} DESTINATION bin)

    set(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/etc/Windows/DJV_Icon.ico)
    set(CPACK_NSIS_MUI_UNIICON ${PROJECT_SOURCE_DIR}/etc/Windows/DJV_Icon.ico)
    set(CPACK_NSIS_INSTALLED_ICON_NAME bin/djv.exe)

    # The file types DJV offers to open. The same set the macOS bundle claims,
    # kept in step with etc/macOS/Info.plist.in, with USD appearing only when
    # this build reads it for the reason given there. Windows offers an
    # application for a type it says it supports; without this DJV is reached
    # only by browsing for the executable.
    set(DJV_WINDOWS_SUPPORTED_TYPES
        aac aiff bmp bw cin djvr dpx exr flac jpeg jpg m4a m4v mov mp3 mp4
        mxf opus otio otioz png ppm psd rgb rgba sgi tga tif tiff wav webm
        y4m)
    if(TLRENDER_USD)
        list(APPEND DJV_WINDOWS_SUPPORTED_TYPES usd usda usdc usdz)
    endif()
    set(DJV_WINDOWS_SUPPORTED_TYPES_REG)
    foreach(extension ${DJV_WINDOWS_SUPPORTED_TYPES})
        string(APPEND DJV_WINDOWS_SUPPORTED_TYPES_REG
            "        WriteRegStr HKLM 'Software\\Classes\\Applications\\djv.exe\\SupportedTypes' '.${extension}' ''\n")
    endforeach()

    # Associate review files (".djvr") with DJV so double-clicking one opens it.
    # The application already routes a ".djvr" argument to the review (see
    # App::_inputFilesInit); these registry entries tell Windows which command to
    # run. SHChangeNotify refreshes the icon cache.
    #
    # HKLM\Software\Classes by name, not HKCR. A write to HKCR does not always
    # reach the machine: the rule is that it goes to HKCU\Software\Classes when
    # the key already exists there, and only otherwise to HKLM. So an installer
    # writing HKCR puts a key in one hive or the other depending on what the
    # user running it happens to have, which is how this was first found -- the
    # "Applications\djv.exe" command below went to HKCU, over the stale value
    # that was the whole problem, while the keys beside it that did not exist
    # in HKCU went to HKLM. It looked right until that user's HKCU was cleared
    # and the registration went with it. Naming the hive removes the choice.
    #
    # "Applications\djv.exe" is claimed for a different reason. It is the key
    # Windows uses for "Open with", and it is named after the executable, which
    # does not change between versions, while the install directory does ("DJV
    # 3.6.0"). Windows writes that key itself the first time a file is opened
    # with DJV, capturing whatever path was current, and never revisits it: after
    # an upgrade it names a directory that has been removed, the launch fails,
    # and Explorer falls back to the same "Open with" dialog the user just came
    # from. Writing it here means every install puts the path right.
    #
    # That does not rescue a user already in this state. The copy Windows wrote
    # is in HKCU\Software\Classes, which shadows the HKLM one this writes, and
    # the installer has no business editing another account's hive. Clearing it
    # is the user's to do, or the application's on startup.
    #
    # ".com" is the console build of the same program, sitting beside the ".exe"
    # so that "djv -h" can print. It is not something to offer in a file dialog:
    # both appear as "djv", and the one that flashes a console is the wrong
    # answer. NoOpenWith keeps it out of the list.
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        WriteRegStr HKLM 'Software\\Classes\\.djvr' '' 'DJV.Review'
        WriteRegStr HKLM 'Software\\Classes\\DJV.Review' '' 'DJV Review Session'
        WriteRegStr HKLM 'Software\\Classes\\DJV.Review\\DefaultIcon' '' '\"$INSTDIR\\bin\\djv.exe\",0'
        WriteRegStr HKLM 'Software\\Classes\\DJV.Review\\shell\\open\\command' '' '\"$INSTDIR\\bin\\djv.exe\" \"%1\"'
        WriteRegStr HKLM 'Software\\Classes\\Applications\\djv.exe' 'FriendlyAppName' 'DJV'
        WriteRegStr HKLM 'Software\\Classes\\Applications\\djv.exe\\DefaultIcon' '' '\"$INSTDIR\\bin\\djv.exe\",0'
        WriteRegStr HKLM 'Software\\Classes\\Applications\\djv.exe\\shell\\open\\command' '' '\"$INSTDIR\\bin\\djv.exe\" \"%1\"'
${DJV_WINDOWS_SUPPORTED_TYPES_REG}        WriteRegStr HKLM 'Software\\Classes\\Applications\\djv.com' 'NoOpenWith' ''
        System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        DeleteRegKey HKLM 'Software\\Classes\\DJV.Review'
        DeleteRegValue HKLM 'Software\\Classes\\.djvr' ''
        DeleteRegKey /ifempty HKLM 'Software\\Classes\\.djvr'
        DeleteRegKey HKLM 'Software\\Classes\\Applications\\djv.exe'
        DeleteRegKey HKLM 'Software\\Classes\\Applications\\djv.com'
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
        file(GLOB FFMPEG_DYLIBS
            "${CMAKE_INSTALL_PREFIX}/lib/libav*.dylib"
            "${CMAKE_INSTALL_PREFIX}/lib/libsw*.dylib")
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
    
    if(TLRENDER_USD)
        file(GLOB MATERIALX_DYLIBS
            "${CMAKE_INSTALL_PREFIX}/lib/libMaterialX*.dylib")
        file(GLOB TBB_DYLIBS
            "${CMAKE_INSTALL_PREFIX}/lib/libtbb*.dylib")
        file(GLOB OSD_DYLIBS
            "${CMAKE_INSTALL_PREFIX}/lib/libosd*.dylib")
        file(GLOB USD_DYLIBS
            "${CMAKE_INSTALL_PREFIX}/lib/libusd_*.dylib")
        list(APPEND INSTALL_DYLIBS ${MATERIALX_DYLIBS} ${TBB_DYLIBS} ${OSD_DYLIBS} ${USD_DYLIBS})

        if(DJV_MACOS_PACKAGE)
            # \bug Why do we need to use ".." to avoid installing into the
            # "Resources" directory in the bundle?
            djv_install_dep(
                DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib/usd
                DESTINATION ../Frameworks)
            djv_install_dep(
                DIRECTORY ${CMAKE_INSTALL_PREFIX}/plugin/usd
                DESTINATION ../PlugIns)
        else()
            djv_install_dep(
                DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib/usd
                DESTINATION lib)
            djv_install_dep(
                DIRECTORY ${CMAKE_INSTALL_PREFIX}/plugin
                DESTINATION ".")
        endif()
    endif()

    if(DJV_MACOS_PACKAGE)
        # Beside the executable in the bundle. Its own copy: a bundle that
        # reaches outside itself for its libraries is not one that can be
        # signed, moved or installed on its own.
        #
        # Every destination here is inside the install prefix. These rules are
        # their own component so an ordinary install stays an ordinary Unix
        # prefix, and packaging asks for the component by name below.
        djv_install_dep(FILES ${INSTALL_DYLIBS}
            DESTINATION "${DJV_BUNDLE_CONTENTS}/Frameworks"
            COMPONENT bundle
            EXCLUDE_FROM_ALL)
    else()
        djv_install_dep(FILES ${INSTALL_DYLIBS} DESTINATION lib)
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
        foreach(RESOURCE docs SampleData)
            install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/share/djv/${RESOURCE}/
                DESTINATION "${DJV_BUNDLE_CONTENTS}/Resources/${RESOURCE}"
                COMPONENT bundle
                EXCLUDE_FROM_ALL)
        endforeach()

        if(TLRENDER_USD)
            set(PRE_BUILD_SCRIPTS "${PROJECT_SOURCE_DIR}/cmake/Modules/usdPluginsSymlink.cmake")
        endif()
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
        file(GLOB FFMPEG_LIBS
            "${CMAKE_INSTALL_PREFIX}/lib/libav*.so*"
            "${CMAKE_INSTALL_PREFIX}/lib/libsw*.so*")
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

    if(TLRENDER_USD)
        file(GLOB MATERIALX_LIBS
            "${CMAKE_INSTALL_PREFIX}/lib/libMaterialX*.so*")
        include(GNUInstallDirs)
        file(GLOB TBB_LIBS
            "${CMAKE_INSTALL_FULL_LIBDIR}/libtbb*.so*"
            "${CMAKE_INSTALL_PREFIX}/lib/libtbb*.so*")
        file(GLOB OSD_LIBS
            "${CMAKE_INSTALL_PREFIX}/lib/libosd*.so*")
        file(GLOB USD_LIBS
            "${CMAKE_INSTALL_PREFIX}/lib/libusd_*.so*")
        list(APPEND INSTALL_LIBS ${MATERIALX_LIBS} ${TBB_LIBS} ${OSD_LIBS} ${USD_LIBS})

        djv_install_dep(
            DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib/usd
            DESTINATION lib)
        djv_install_dep(
            DIRECTORY ${CMAKE_INSTALL_PREFIX}/plugin
            DESTINATION ".")
    endif()
    
    djv_install_dep(FILES ${INSTALL_LIBS} DESTINATION lib)

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
