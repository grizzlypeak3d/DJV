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
    
    if(TLRENDER_USD)
        set(MATERIALX_DLLS
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXCore.dll
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXFormat.dll
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXGenGlsl.dll
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXGenMdl.dll
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXGenMsl.dll
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXGenOsl.dll
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXGenShader.dll
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXRender.dll
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXRenderGlsl.dll
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXRenderHw.dll
            ${CMAKE_INSTALL_PREFIX}/bin/MaterialXRenderOsl.dll)
        set(TBB_DLLS
            ${CMAKE_INSTALL_PREFIX}/bin/tbb12.dll
            ${CMAKE_INSTALL_PREFIX}/bin/tbbmalloc.dll
            ${CMAKE_INSTALL_PREFIX}/bin/tbbmalloc_proxy.dll)
        set(USD_DLLS
            ${CMAKE_INSTALL_PREFIX}/bin/usd_ar.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_arch.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_cameraUtil.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_ef.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_esf.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_esfUsd.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_exec.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_execGeom.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_execUsd.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_garch.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_geomUtil.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_gf.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_glf.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hd.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hdGp.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hdMtlx.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hdSt.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hdar.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hdsi.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hdx.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hf.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hgi.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hgiGL.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hgiInterop.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_hio.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_js.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_kind.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_pcp.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_pegtl.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_plug.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_pxOsd.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_sdf.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_sdr.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_tf.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_trace.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_ts.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usd.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdAppUtils.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdBakeMtlx.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdGeom.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdGeomValidators.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdHydra.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdImaging.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdImagingGL.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdLux.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdMedia.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdMtlx.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdPhysics.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdPhysicsValidators.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdProc.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdProcImaging.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdRender.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdRi.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdRiPxrImaging.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdSemantics.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdShade.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdShadeValidators.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdSkel.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdSkelImaging.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdSkelValidators.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdUI.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdUtils.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdUtilsValidators.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdValidation.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdVol.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_usdVolImaging.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_vdf.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_vt.dll
            ${CMAKE_INSTALL_PREFIX}/bin/usd_work.dll)
        list(APPEND INSTALL_DLLS ${MATERIALX_DLLS} ${TBB_DLLS} ${USD_DLLS})

        install(
            DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/usd
            DESTINATION bin)
        install(
            DIRECTORY ${CMAKE_INSTALL_PREFIX}/plugin
            DESTINATION ".")
    endif()
    
    install(
        FILES ${INSTALL_DLLS}
        DESTINATION bin)

    set(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/etc/Windows/DJV_Icon.ico)
    set(CPACK_NSIS_MUI_UNIICON ${PROJECT_SOURCE_DIR}/etc/Windows/DJV_Icon.ico)
    set(CPACK_NSIS_INSTALLED_ICON_NAME etc/Icons/DJV_Icon.ico)

elseif(APPLE AND DJV_MACOS_PACKAGE)

    set(CPACK_GENERATOR Bundle)

    set(INSTALL_DYLIBS)
    
    if(TLRENDER_FFMPEG)
        set(FFMPEG_DYLIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.62.11.100.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.62.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.62.1.100.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.62.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.62.3.100.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.62.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.60.8.100.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.60.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.6.1.100.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.6.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.9.1.100.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.9.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.dylib)
        list(APPEND INSTALL_DYLIBS ${FFMPEG_DYLIBS})
    endif()
    
    if(TLRENDER_USD)
        set(MATERIALX_DYLIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXCore.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXCore.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXCore.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXFormat.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXFormat.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXFormat.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenGlsl.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenGlsl.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenGlsl.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMdl.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMdl.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMdl.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMsl.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMsl.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMsl.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenOsl.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenOsl.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenOsl.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenShader.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenShader.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenShader.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRender.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRender.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRender.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderGlsl.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderGlsl.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderGlsl.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderHw.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderHw.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderHw.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderMsl.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderMsl.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderMsl.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderOsl.1.39.3.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderOsl.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderOsl.dylib)
        set(TBB_DYLIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libtbb.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libtbb.12.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libtbb.12.12.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libtbbmalloc.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libtbbmalloc.2.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libtbbmalloc.2.12.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libtbbmalloc_proxy.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libtbbmalloc_proxy.2.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libtbbmalloc_proxy.2.12.dylib)
        set(OSD_DYLIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libosdCPU.3.6.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libosdCPU.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libosdGPU.3.6.1.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libosdGPU.dylib)
        set(USD_DYLIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_ar.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_arch.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_cameraUtil.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_ef.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_esf.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_esfUsd.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_exec.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_execGeom.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_execUsd.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_garch.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_geomUtil.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_gf.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_glf.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hd.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdar.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdGp.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdMtlx.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdsi.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdSt.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdx.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hf.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hgi.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hgiGL.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hgiInterop.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hgiMetal.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hio.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_js.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_kind.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_pcp.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_pegtl.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_plug.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_pxOsd.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_sdf.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_sdr.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_tf.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_trace.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_ts.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usd.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdAppUtils.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdBakeMtlx.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdGeom.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdGeomValidators.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdHydra.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdImaging.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdImagingGL.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdLux.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdMedia.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdMtlx.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdPhysics.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdPhysicsValidators.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdProc.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdProcImaging.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdRender.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdRi.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdRiPxrImaging.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdSemantics.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdShade.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdShadeValidators.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdSkel.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdSkelImaging.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdSkelValidators.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdUI.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdUtils.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdUtilsValidators.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdValidation.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdVol.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdVolImaging.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_vdf.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_vt.dylib
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_work.dylib)
        list(APPEND INSTALL_DYLIBS ${MATERIALX_DYLIBS} ${TBB_DYLIBS} ${OSD_DYLIBS} ${USD_DYLIBS})

        # \bug Why do we need to use ".." to avoid installing into the
        # "Resources" directory in the bundle?
        install(
            DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib/usd
            DESTINATION ../Frameworks)
        install(
            DIRECTORY ${CMAKE_INSTALL_PREFIX}/plugin/usd
            DESTINATION ../PlugIns)
    endif()

    # \bug Why do we need to use ".." to avoid installing into the
    # "Resources" directory in the bundle?
    install(FILES ${INSTALL_DYLIBS} DESTINATION ../Frameworks)

    set(CPACK_BUNDLE_NAME DJV)
    configure_file(
        ${PROJECT_SOURCE_DIR}/etc/macOS/Info.plist.in
        ${PROJECT_BINARY_DIR}/Info.plist)
    set(CPACK_BUNDLE_PLIST ${PROJECT_BINARY_DIR}/Info.plist)
    set(CPACK_BUNDLE_ICON ${PROJECT_SOURCE_DIR}/etc/macOS/DJV.icns)
    install(FILES ${PROJECT_BINARY_DIR}/Info.plist DESTINATION "..")
    install(FILES ${PROJECT_SOURCE_DIR}/etc/macOS/DJV.icns DESTINATION ".")

    set(PRE_BUILD_SCRIPTS "${PROJECT_SOURCE_DIR}/cmake/Modules/usdPluginsSymlink.cmake")
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

else()

    set(CPACK_GENERATOR TGZ)

    set(INSTALL_LIBS)
    
    if(TLRENDER_FFMPEG)
        set(FFMPEG_LIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.so
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.so.62
            ${CMAKE_INSTALL_PREFIX}/lib/libavcodec.so.62.11.100
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.so
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.so.62
            ${CMAKE_INSTALL_PREFIX}/lib/libavdevice.so.62.1.100
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.so
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.so.62
            ${CMAKE_INSTALL_PREFIX}/lib/libavformat.so.62.3.100
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.so
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.so.60
            ${CMAKE_INSTALL_PREFIX}/lib/libavutil.so.60.8.100
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.so
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.so.6
            ${CMAKE_INSTALL_PREFIX}/lib/libswresample.so.6.1.100
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.so
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.so.9
            ${CMAKE_INSTALL_PREFIX}/lib/libswscale.so.9.1.100)
        list(APPEND INSTALL_LIBS ${FFMPEG_LIBS})
    endif()

    if(TLRENDER_USD)
        set(MATERIALX_LIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXCore.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXCore.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXCore.so.1.39.3
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXFormat.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXFormat.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXFormat.so.1.39.3
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenGlsl.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenGlsl.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenGlsl.so.1.39.3
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMdl.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMdl.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMdl.so.1.39.3
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMsl.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMsl.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenMsl.so.1.39.3
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenOsl.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenOsl.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenOsl.so.1.39.3
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenShader.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenShader.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXGenShader.so.1.39.3
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderGlsl.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderGlsl.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderGlsl.so.1.39.3
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderHw.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderHw.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderHw.so.1.39.3
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderOsl.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderOsl.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRenderOsl.so.1.39.3
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRender.so
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRender.so.1
            ${CMAKE_INSTALL_PREFIX}/lib/libMaterialXRender.so.1.39.3)
        include(GNUInstallDirs)
        set(TBB_LIBS
            ${CMAKE_INSTALL_FULL_LIBDIR}/libtbbmalloc_proxy.so
            ${CMAKE_INSTALL_FULL_LIBDIR}/libtbbmalloc_proxy.so.2
            ${CMAKE_INSTALL_FULL_LIBDIR}/libtbbmalloc_proxy.so.2.12
            ${CMAKE_INSTALL_FULL_LIBDIR}/libtbbmalloc.so
            ${CMAKE_INSTALL_FULL_LIBDIR}/libtbbmalloc.so.2
            ${CMAKE_INSTALL_FULL_LIBDIR}/libtbbmalloc.so.2.12
            ${CMAKE_INSTALL_FULL_LIBDIR}/libtbb.so
            ${CMAKE_INSTALL_FULL_LIBDIR}/libtbb.so.12
            ${CMAKE_INSTALL_FULL_LIBDIR}/libtbb.so.12.12)
        set(OSD_LIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libosdCPU.so
            ${CMAKE_INSTALL_PREFIX}/lib/libosdCPU.so.3.6.1
            ${CMAKE_INSTALL_PREFIX}/lib/libosdGPU.so
            ${CMAKE_INSTALL_PREFIX}/lib/libosdGPU.so.3.6.1)
        set(USD_LIBS
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_arch.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_ar.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_cameraUtil.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_ef.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_esf.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_esfUsd.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_execGeom.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_exec.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_execUsd.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_garch.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_geomUtil.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_gf.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_glf.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdar.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdGp.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdMtlx.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdsi.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hd.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdSt.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hdx.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hf.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hgiGL.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hgiInterop.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hgi.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_hio.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_js.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_kind.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_pcp.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_pegtl.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_plug.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_pxOsd.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_sdf.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_sdr.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_tf.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_trace.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_ts.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdAppUtils.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdBakeMtlx.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdGeom.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdGeomValidators.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdHydra.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdImagingGL.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdImaging.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdLux.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdMedia.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdMtlx.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdPhysics.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdPhysicsValidators.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdProcImaging.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdProc.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdRender.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdRiPxrImaging.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdRi.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdSemantics.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdShade.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdShadeValidators.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdSkelImaging.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdSkel.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdSkelValidators.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usd.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdUI.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdUtils.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdUtilsValidators.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdValidation.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdVolImaging.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_usdVol.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_vdf.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_vt.so
            ${CMAKE_INSTALL_PREFIX}/lib/libusd_work.so)
        list(APPEND INSTALL_LIBS ${MATERIALX_LIBS} ${TBB_LIBS} ${OSD_LIBS} ${USD_LIBS})

        install(
            DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib/usd
            DESTINATION lib)
        install(
            DIRECTORY ${CMAKE_INSTALL_PREFIX}/plugin
            DESTINATION ".")
    endif()
    
    install(
        FILES ${INSTALL_LIBS}
        DESTINATION lib)

endif()

set(DEST etc/DJV/Legal)
if(DJV_MACOS_PACKAGE)
    set(DEST Legal)
endif()
install(
    FILES
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_BlackmagicDesign.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_boost.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_CMake.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_CURL.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_expat.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_FFmpeg.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_feather-tk.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_libjpeg-turbo.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_libjpeg.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_libpng.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_Libssh2.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_libtiff.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_MaterialX.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_minizip-ng.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_nlohmann_json.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_OFL.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_oneTBB.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_OpenColorIO.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_OpenEXR.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_OpenImageIO.md
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_OpenSSL.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_OpenSubdiv.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_OpenTimelineIO.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_OpenUSD.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_pystring.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_tlRender.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_yaml-cpp.txt
    ${CMAKE_INSTALL_PREFIX}/etc/tlRender/LICENSE_zlib.txt
    DESTINATION ${DEST})
