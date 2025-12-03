set(DJV_MACOS_TEAM_ID $ENV{DJV_MACOS_TEAM_ID})
execute_process(
    COMMAND codesign --sign "${DJV_MACOS_TEAM_ID}"
    --timestamp --force
    --options runtime
    --identifier com.grizzlypeak3d.djv
    --deep
    ${CPACK_TEMPORARY_INSTALL_DIRECTORY}/DJV.app)
