set(LEGAL
    LICENSE_aom.txt
    LICENSE_CMake.txt
    LICENSE_DJV.txt
    LICENSE_coi-serviceworker.txt
    LICENSE_expat.txt
    LICENSE_FFmpeg.txt
    LICENSE_FreeType.txt
    LICENSE_feather-tk.txt
    LICENSE_glad.txt
    LICENSE_LibRaw.txt
    LICENSE_libjpeg-turbo.txt
    LICENSE_libjpeg.txt
    LICENSE_libpng.txt
    LICENSE_libtiff.txt
    LICENSE_lunasvg.txt
    LICENSE_minizip-ng.txt
    LICENSE_mp4box.txt
    LICENSE_nlohmann_json.txt
    LICENSE_nv-codec-headers.txt
    LICENSE_OFL.txt
    LICENSE_OpenColorIO.txt
    LICENSE_OpenEXR.txt
    LICENSE_OpenImageIO.md
    LICENSE_OpenJPH.txt
    LICENSE_OpenTimelineIO.txt
    LICENSE_plutovg.txt
    LICENSE_pystring.txt
    LICENSE_SDL2.txt
    LICENSE_subprocess.txt
    LICENSE_svt-av1.txt
    LICENSE_tlRender.txt
    LICENSE_yaml-cpp.txt
    LICENSE_zlib.txt)

# The macOS package is not here: the bundle carries its own copy,
# taken from the prefix by cmake/Modules/Package.cmake, so this stays
# where every other build puts it.
set(DEST share/djv/Legal)
if(DJV_WINDOWS_PACKAGE)
    set(DEST Legal)
endif()
install(FILES ${LEGAL} DESTINATION ${DEST})
