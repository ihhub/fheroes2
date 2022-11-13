@echo off

set DST_DIR=%~dp0\..\..\VisualStudio\packages

set PKG_FILE=windows.zip
set PKG_FILE_SHA256=139382B653B2C38A7D523184ADA2C651AE79D6D692A2673FDBFCF1D4098FBF54
set PKG_URL=https://github.com/fheroes2/fheroes2-prebuilt-deps/releases/download/windows-deps/%PKG_FILE%
set PKG_TLS=[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

set EXIT_CODE=0

call :install_packages || ^
set EXIT_CODE=1

if not "%CI%" == "true" (
    pause
)

exit /B %EXIT_CODE%

:install_packages

if not exist "%DST_DIR%" ( mkdir "%DST_DIR%" || exit /B 1 )

echo Downloading %PKG_URL%                                                                                        && ^
powershell -Command "%PKG_TLS%; (New-Object System.Net.WebClient).DownloadFile('%PKG_URL%', '%TEMP%\%PKG_FILE%')" || ^
exit /B 1

set HASH_CALCULATED=false

for /F "usebackq delims=" %%H in (`powershell -Command "(certutil.exe -hashfile '%TEMP%\%PKG_FILE%' sha256 | Select-String -Pattern '^[0-9A-Fa-f]{64}$').Line.ToUpper()"`) do (
    set HASH_CALCULATED=true

    if not "%%H" == "%PKG_FILE_SHA256%" (
        echo Invalid hash for %PKG_FILE%: expected %PKG_FILE_SHA256%, got %%H
        exit /B 1
    )
)

if not "%HASH_CALCULATED%" == "true" (
    echo Failed to calculate hash for %PKG_FILE%
    exit /B 1
)

call :unpack_archive "%TEMP%\%PKG_FILE%" "%DST_DIR%" || ^
exit /B 1

exit /B

:unpack_archive

echo Unpacking %~n1%~x1

if "%CI%" == "true" (
    powershell -Command "Expand-Archive -LiteralPath '%~1' -DestinationPath '%~2' -Force" || exit /B 1
) else (
    powershell -Command "$shell = New-Object -ComObject 'Shell.Application'; $zip = $shell.NameSpace((Resolve-Path '%~1').Path); foreach ($item in $zip.items()) { $shell.Namespace((Resolve-Path '%~2').Path).CopyHere($item, 0x14) }" || exit /B 1
)

exit /B
