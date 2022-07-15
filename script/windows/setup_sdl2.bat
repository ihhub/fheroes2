@echo off

if "%~3"=="" (
    echo Syntax: %~n0%~x0 VCPKG_ROOT_DIR PLATFORM PACKAGES_DIR
    exit /B 1
)

set VCPKG_ROOT_DIR=%~1
set PLATFORM=%~2
set PACKAGES_DIR=%~3

set TRIPLET=%PLATFORM%-windows

"%VCPKG_ROOT_DIR%\vcpkg.exe" --triplet "%TRIPLET%" install sdl2 sdl2-mixer[fluidsynth] sdl2-image zlib || ^
exit /B 1

if not exist "%PACKAGES_DIR%\include"        ( mkdir "%PACKAGES_DIR%\include"        || exit /B 1 )
if not exist "%PACKAGES_DIR%\lib\%PLATFORM%" ( mkdir "%PACKAGES_DIR%\lib\%PLATFORM%" || exit /B 1 )

xcopy /Y /Q    "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\*.dll" "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q /S "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\include"   "%PACKAGES_DIR%\include"        && ^
xcopy /Y /Q /S "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\lib"       "%PACKAGES_DIR%\lib\%PLATFORM%" || ^
exit /B 1
