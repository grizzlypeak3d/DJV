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

# The USD notices ride only when USD does: what ships needs a
# notice, and only what ships.
if(TLRENDER_USD)
    list(APPEND LEGAL
        LICENSE_boost.txt
        LICENSE_MaterialX.txt
        LICENSE_OpenSubdiv.txt
        LICENSE_OpenUSD.txt
        LICENSE_oneTBB.txt)
endif()

# Not installed. Every notice reaches the install inside the licenses page,
# which is documentation, goes wherever the documentation goes, and reads as
# one document rather than a directory of text files nobody opens.

# The licenses page in the documentation, written from the list above: a
# notice cannot reach a package without reaching the page.
#
# At configure time, into the build tree. The page is a description of one
# build -- these notices, because this configuration -- so it is not a file
# to keep in the repository, where it could only describe whoever generated
# it last. The list is handed over as a file rather than a command line, the
# way the navigation is, so a name the shell would take an interest in stays
# one name.
set(LEGAL_LIST ${CMAKE_CURRENT_BINARY_DIR}/legal.txt)
string(REPLACE ";" "\n" LEGAL_LINES "${LEGAL}")
file(WRITE ${LEGAL_LIST} "${LEGAL_LINES}\n")

set(DJV_LICENSES_PAGE ${CMAKE_CURRENT_BINARY_DIR}/licenses.html PARENT_SCOPE)
execute_process(
    COMMAND ${CMAKE_COMMAND}
        -DLICENSES_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DLICENSES_LIST=${LEGAL_LIST}
        -DLICENSES_OUT=${CMAKE_CURRENT_BINARY_DIR}/licenses.html
        -P ${PROJECT_SOURCE_DIR}/docs/Licenses.cmake
    COMMAND_ERROR_IS_FATAL ANY)
