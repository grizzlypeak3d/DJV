rem Usage: package-win.bat [source directory] [build type]
rem
rem Builds with the "package-windows" config and then makes the package. What
rem to build is in etc/Config/package-windows.cmake, not here.

set SOURCE_DIR=%1
set BUILD_TYPE=%2
IF "%SOURCE_DIR%"=="" set SOURCE_DIR=DJV
IF "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

call %SOURCE_DIR%\etc\Windows\sbuild.bat %SOURCE_DIR% %BUILD_TYPE% package-windows
cmake --build build-%BUILD_TYPE% --config %BUILD_TYPE% --target package
