:: ###########################################################################
:: #   fheroes2: https://github.com/ihhub/fheroes2                           #
:: #   Copyright (C) 2022 - 2026                                             #
:: #                                                                         #
:: #   This program is free software; you can redistribute it and/or modify  #
:: #   it under the terms of the GNU General Public License as published by  #
:: #   the Free Software Foundation; either version 2 of the License, or     #
:: #   (at your option) any later version.                                   #
:: #                                                                         #
:: #   This program is distributed in the hope that it will be useful,       #
:: #   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
:: #   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
:: #   GNU General Public License for more details.                          #
:: #                                                                         #
:: #   You should have received a copy of the GNU General Public License     #
:: #   along with this program; if not, write to the                         #
:: #   Free Software Foundation, Inc.,                                       #
:: #   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
:: ###########################################################################

@echo off

set DST_DIR=%~dp0\..\..\android

set PKG_FILE=android.zip
set PKG_FILE_SHA256=4A34D7CB11EEB31A16107B0D8E56EC2D487803EC618080BD5944834A4FE11C40
set PKG_URL=https://github.com/fheroes2/fheroes2-prebuilt-deps/releases/download/android-deps/%PKG_FILE%
set PKG_TLS=[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

:: Network games use the standalone header-only version of asio
set ASIO_VERSION=1-38-2
set ASIO_FILE=asio-%ASIO_VERSION%.zip
set ASIO_FILE_SHA256=1907E462B736F3B682E1539C6D7A8ECECDE66DCB25FE98C8386701C101A93388
set ASIO_URL=https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-%ASIO_VERSION%.zip
set ASIO_DST_DIR=%DST_DIR%\app\jni\asio

set EXIT_CODE=0

call :install_packages || ^
set EXIT_CODE=1

if not "%CI%" == "true" (
    pause
)

exit /B %EXIT_CODE%

:install_packages

if not exist "%DST_DIR%" ( mkdir "%DST_DIR%" || exit /B 1 )

call :download_file "%PKG_URL%" "%PKG_FILE%" "%PKG_FILE_SHA256%" || ^
exit /B 1

call :unpack_archive "%TEMP%\%PKG_FILE%" "%DST_DIR%" || ^
exit /B 1

call :download_file "%ASIO_URL%" "%ASIO_FILE%" "%ASIO_FILE_SHA256%" || ^
exit /B 1

call :install_asio || ^
exit /B 1

exit /B

:download_file

echo Downloading %~1                                                                                              && ^
powershell -Command "%PKG_TLS%; (New-Object System.Net.WebClient).DownloadFile('%~1', '%TEMP%\%~2')" || ^
exit /B 1

set HASH_CALCULATED=false

for /F "usebackq delims=" %%H in (`powershell -Command "(certutil.exe -hashfile '%TEMP%\%~2' sha256 | Select-String -Pattern '^[0-9A-Fa-f]{64}$').Line.ToUpper()"`) do (
    set HASH_CALCULATED=true

    if not "%%H" == "%~3" (
        echo Invalid hash for %~2: expected %~3, got %%H
        exit /B 1
    )
)

if not "%HASH_CALCULATED%" == "true" (
    echo Failed to calculate hash for %~2
    exit /B 1
)

exit /B

:install_asio

echo Unpacking %ASIO_FILE%

if exist "%TEMP%\asio-asio-%ASIO_VERSION%" ( rmdir /S /Q "%TEMP%\asio-asio-%ASIO_VERSION%" || exit /B 1 )
if exist "%ASIO_DST_DIR%" ( rmdir /S /Q "%ASIO_DST_DIR%" || exit /B 1 )

powershell -Command "Expand-Archive -LiteralPath '%TEMP%\%ASIO_FILE%' -DestinationPath '%TEMP%' -Force" || exit /B 1

xcopy /E /I /Q "%TEMP%\asio-asio-%ASIO_VERSION%\include" "%ASIO_DST_DIR%\include" > nul || exit /B 1
copy /Y "%TEMP%\asio-asio-%ASIO_VERSION%\LICENSE_1_0.txt" "%ASIO_DST_DIR%" > nul || exit /B 1

rmdir /S /Q "%TEMP%\asio-asio-%ASIO_VERSION%"

exit /B

:unpack_archive

echo Unpacking %~n1%~x1

if "%CI%" == "true" (
    powershell -Command "Expand-Archive -LiteralPath '%~1' -DestinationPath '%~2' -Force" || exit /B 1
) else (
    powershell -Command "$shell = New-Object -ComObject 'Shell.Application'; $zip = $shell.NameSpace((Resolve-Path '%~1').Path); foreach ($item in $zip.items()) { $shell.Namespace((Resolve-Path '%~2').Path).CopyHere($item, 0x14) }" || exit /B 1
)

exit /B
