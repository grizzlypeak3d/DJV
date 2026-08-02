@echo off
setlocal

if not defined DJV_EXE set "DJV_EXE=djv"

"%DJV_EXE%" -playlistFolder "%CD%" %*
exit /b %ERRORLEVEL%
