rem Usage: sbuild-win.bat [source directory] [build type] [config]
rem
rem The config names a file in etc/Config; "default" builds everything.
rem Personal settings go in etc/Config/local.cmake, which is not tracked. For
rem the number of build jobs, set CMAKE_BUILD_PARALLEL_LEVEL.

set SOURCE_DIR=%1
set BUILD_TYPE=%2
set CONFIG=%3
IF "%SOURCE_DIR%"=="" set SOURCE_DIR=DJV
IF "%BUILD_TYPE%"=="" set BUILD_TYPE=Release
IF "%CONFIG%"=="" set CONFIG=default

%SOURCE_DIR%\etc\Windows\sbuild.bat %SOURCE_DIR% %BUILD_TYPE% %CONFIG%
