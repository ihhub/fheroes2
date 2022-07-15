@echo off

if "%~2"=="" (
    echo Syntax: %~n0%~x0 ZLIB_FILE_DIR PACKAGES_DIR
    exit /B 1
)

set ZLIB_FILE_DIR=%~1
set PACKAGES_DIR=%~2

set ZLIB_FILE=zlib1.2.11.zip
set SDL_FILE=SDL-devel-1.2.15-VC.zip
set SDL_MIXER_FILE=SDL_mixer-devel-1.2.12-VC.zip

set SDL_URL=https://www.libsdl.org/release/%SDL_FILE%
set SDL_MIXER_URL=https://www.libsdl.org/projects/SDL_mixer/release/%SDL_MIXER_FILE%

echo Downloading %SDL_URL%...                                                                                      && ^
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%SDL_URL%',       '%TEMP%\%SDL_FILE%')"       && ^
echo Downloading %SDL_MIXER_URL%...                                                                                && ^
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%SDL_MIXER_URL%', '%TEMP%\%SDL_MIXER_FILE%')" || ^
exit /B 1

if not exist "%PACKAGES_DIR%\include"        ( mkdir "%PACKAGES_DIR%\include"        || exit /B 1 )
if not exist "%PACKAGES_DIR%\include\SDL"    ( mkdir "%PACKAGES_DIR%\include\SDL"    || exit /B 1 )
if not exist "%PACKAGES_DIR%\lib\%PLATFORM%" ( mkdir "%PACKAGES_DIR%\lib\%PLATFORM%" || exit /B 1 )

call :unpack_archive "%ZLIB_FILE_DIR%\%ZLIB_FILE%" "%PACKAGES_DIR%" && ^
call :unpack_archive "%TEMP%\%SDL_FILE%"           "%TEMP%"         && ^
call :unpack_archive "%TEMP%\%SDL_MIXER_FILE%"     "%TEMP%"         || ^
exit /B 1

echo Copying files...

xcopy /Y /S /Q "%TEMP%\SDL-1.2.15\include"       "%PACKAGES_DIR%\include\SDL" && ^
xcopy /Y /S /Q "%TEMP%\SDL-1.2.15\lib"           "%PACKAGES_DIR%\lib"         && ^
xcopy /Y /S /Q "%TEMP%\SDL_mixer-1.2.12\include" "%PACKAGES_DIR%\include\SDL" && ^
xcopy /Y /S /Q "%TEMP%\SDL_mixer-1.2.12\lib"     "%PACKAGES_DIR%\lib"         || ^
exit /B 1

exit /B

:unpack_archive

echo Unpacking %~n1%~x1...

if "%CI%" == "true" (
    powershell -Command "Expand-Archive -LiteralPath '%~1' -DestinationPath '%~2' -Force" || exit /B 1
) else (
    powershell -Command "$shell = New-Object -ComObject 'Shell.Application'; $zip = $shell.NameSpace((Resolve-Path '%~1').Path); foreach ($item in $zip.items()) { $shell.Namespace((Resolve-Path '%~2').Path).CopyHere($item, 0x14) }" || exit /B 1
)

exit /B
