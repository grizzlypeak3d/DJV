# The desktop entry and icons, in the layout a desktop reads them from. The
# tar archive is not installed anywhere in particular, so the entry runs
# "djv" from the path; the install page says how to point it at the copy.
install(
    FILES djv.desktop
    DESTINATION share/applications)
foreach(SIZE 16 64 128 512)
    install(
        FILES ${PROJECT_SOURCE_DIR}/etc/Icons/DJV_Icon_${SIZE}.png
        DESTINATION share/icons/hicolor/${SIZE}x${SIZE}/apps
        RENAME djv.png)
endforeach()
install(
    FILES ${PROJECT_SOURCE_DIR}/etc/Icons/DJV_Icon.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME djv.svg)
