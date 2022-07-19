@echo off

set DST_DIR=%~dp0\..\..\VisualStudio\packages\installed

set PKG_FILE=windows.zip
set PKG_URL=https://github.com/fheroes2/fheroes2-prebuilt-deps/releases/download/windows-deps/%PKG_FILE%
set PKG_TLS=[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

setlocal enableextensions

if not exist "%DST_DIR%" ( mkdir "%DST_DIR%" || exit /B 1 )

endlocal

echo Downloading %PKG_URL%                                                                                        && ^
powershell -Command "%PKG_TLS%; (New-Object System.Net.WebClient).DownloadFile('%PKG_URL%', '%TEMP%\%PKG_FILE%')" || ^
exit /B 1

call :unpack_archive "%TEMP%\%PKG_FILE%" "%DST_DIR%" || ^
exit /B 1

if not "%CI%" == "true" (
    pause
)

exit /B

:unpack_archive

echo Unpacking %~n1%~x1

if "%CI%" == "true" (
    powershell -Command "Expand-Archive -LiteralPath '%~1' -DestinationPath '%~2' -Force" || exit /B 1
) else (
    powershell -Command "$shell = New-Object -ComObject 'Shell.Application'; $zip = $shell.NameSpace((Resolve-Path '%~1').Path); foreach ($item in $zip.items()) { $shell.Namespace((Resolve-Path '%~2').Path).CopyHere($item, 0x14) }" || exit /B 1
)

exit /B
