@echo off

echo Setting up SDL1... && ^
call %~dp0\setup_sdl1.bat %~dp0\..\..\VisualStudio/packages %~dp0\..\..\VisualStudio/packages/installed/sdl1 && ^
echo Setting up SDL2... && ^
call %~dp0\setup_sdl2.bat C:\vcpkg x86 %~dp0\..\..\VisualStudio/packages/installed/sdl2 && ^
call %~dp0\setup_sdl2.bat C:\vcpkg x64 %~dp0\..\..\VisualStudio/packages/installed/sdl2 && ^
echo "SUCCESS! Installation is completed"

echo Press any key to exit...
pause >nul
