set(LEGAL
    LICENSE_BlackmagicDesign.txt
    LICENSE_boost.txt
    LICENSE_CMake.txt
    LICENSE_CURL.txt
    LICENSE_DJV.txt
    LICENSE_expat.txt
    LICENSE_FFmpeg.txt
    LICENSE_FreeType.txt
    LICENSE_feather-tk.txt
    LICENSE_glad.txt
    LICENSE_Libssh2.txt
    LICENSE_libjpeg-turbo.txt
    LICENSE_libjpeg.txt
    LICENSE_libpng.txt
    LICENSE_libtiff.txt
    LICENSE_lunasvg.txt
    LICENSE_MaterialX.txt
    LICENSE_minizip-ng.txt
    LICENSE_nlohmann_json.txt
    LICENSE_OFL.txt
    LICENSE_OpenColorIO.txt
    LICENSE_OpenEXR.txt
    LICENSE_OpenImageIO.md
    LICENSE_OpenSSL.txt
    LICENSE_OpenSubdiv.txt
    LICENSE_OpenTimelineIO.txt
    LICENSE_OpenUSD.txt
    LICENSE_oneTBB.txt
    LICENSE_plutovg.txt
    LICENSE_pystring.txt
    LICENSE_SDL2.txt
    LICENSE_subprocess.txt
    LICENSE_tlRender.txt
    LICENSE_yaml-cpp.txt
    LICENSE_zlib.txt)

set(DEST share/djv/Legal)
if(DJV_MACOS_PACKAGE OR DJV_WINDOWS_PACKAGE)
    set(DEST Legal)
endif()
install(FILES ${LEGAL} DESTINATION ${DEST})
