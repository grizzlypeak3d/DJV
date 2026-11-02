set(DJV_MACOS_TEAM_ID $ENV{DJV_MACOS_TEAM_ID})

# With no Developer ID, signed ad hoc: the linker signs the executable ad
# hoc by itself, and a bundle with only that is "damaged" to macOS once it
# has been downloaded, with no way to open it. Signed ad hoc as a whole, it
# is a build from an unidentified developer, which can be opened from
# System Settings > Privacy & Security.
if(DJV_MACOS_TEAM_ID)
    set(SIGN_ARGS --sign "${DJV_MACOS_TEAM_ID}" --timestamp --options runtime)
else()
    set(SIGN_ARGS --sign -)
endif()
execute_process(
    COMMAND codesign ${SIGN_ARGS}
    --force
    --identifier com.grizzlypeak3d.djv
    --deep
    ${CPACK_TEMPORARY_INSTALL_DIRECTORY}/djv.app)
