@echo off

call :find_path vcpkg.exe

if "%VCPKG_DIR%"=="" (

echo vcpkg was not found in your system. Please install it, add its location to PATH and re-run this script.

) else (

echo Setting up SDL1... && ^
call "%~dp0\setup_sdl1.bat" "%~dp0\..\..\VisualStudio/packages" "%~dp0\..\..\VisualStudio/packages/installed/sdl1" && ^
echo Setting up SDL2... && ^
call "%~dp0\setup_sdl2.bat" "%VCPKG_DIR%" x86 "%~dp0\..\..\VisualStudio/packages/installed/sdl2" && ^
call "%~dp0\setup_sdl2.bat" "%VCPKG_DIR%" x64 "%~dp0\..\..\VisualStudio/packages/installed/sdl2" && ^
echo Installation is complete

)

pause

exit /B

:find_path

set VCPKG_DIR=%~dp$PATH:1

exit /B
