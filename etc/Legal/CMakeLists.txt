set(LEGAL
    LICENSE_DJV.txt
    LICENSE_tlRender.txt)

set(DEST etc/DJV/Legal)
if(DJV_MACOS_PACKAGE)
    set(DEST Legal)
endif()
install(
    FILES ${LEGAL}
    DESTINATION ${DEST})
