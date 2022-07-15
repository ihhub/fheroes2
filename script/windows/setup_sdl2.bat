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
if not exist "%PACKAGES_DIR%\include\SDL2"   ( mkdir "%PACKAGES_DIR%\include\SDL2"   || exit /B 1 )
if not exist "%PACKAGES_DIR%\lib\%PLATFORM%" ( mkdir "%PACKAGES_DIR%\lib\%PLATFORM%" || exit /B 1 )

xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\gio*.dll"           "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\glib*.dll"          "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\gmodule*.dll"       "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\gobject*.dll"       "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\gthread*.dll"       "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\iconv*.dll"         "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\intl*.dll"          "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\libffi*.dll"        "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\libfluidsynth*.dll" "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\libpng*.dll"        "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\pcre*.dll"          "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\SDL2*.dll"          "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\bin\zlib*.dll"          "%PACKAGES_DIR%\lib\%PLATFORM%" || ^
exit /B 1

xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\include\SDL2\*.*" "%PACKAGES_DIR%\include\SDL2" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\include\zconf.h"  "%PACKAGES_DIR%\include"      && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\include\zlib.h"   "%PACKAGES_DIR%\include"      || ^
exit /B 1

xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\lib\SDL2*.lib" "%PACKAGES_DIR%\lib\%PLATFORM%" && ^
xcopy /Y /Q "%VCPKG_ROOT_DIR%\installed\%TRIPLET%\lib\zlib*.lib" "%PACKAGES_DIR%\lib\%PLATFORM%" || ^
exit /B 1
