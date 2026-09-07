# \bug How do we point USD at the "PlugIns" directory?
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CPACK_TEMPORARY_INSTALL_DIRECTORY}/DJV.app/Contents/plugin)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
    ../PlugIns/usd
    ${CPACK_TEMPORARY_INSTALL_DIRECTORY}/DJV.app/Contents/plugin/usd)
