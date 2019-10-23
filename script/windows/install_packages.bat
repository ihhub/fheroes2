@echo off

if not exist "..\..\..\packages"            mkdir "..\..\..\packages"
if not exist "..\..\..\packages\zlib_32bit" mkdir "..\..\..\packages\zlib_32bit"
if not exist "..\..\..\packages\zlib_64bit" mkdir "..\..\..\packages\zlib_64bit"
if not exist "..\..\..\packages\sdl"        mkdir "..\..\..\packages\sdl"
if not exist "..\..\..\packages\sdl_mixer"  mkdir "..\..\..\packages\sdl_mixer"

echo downloading packages [1/4]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libs4win.com/libzlib/libzlib-1.2.11-msvc2015-x86-release.zip', '..\..\..\packages\zlib_32bit\zlib_32bit.zip')"
echo downloading packages [2/4]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libs4win.com/libzlib/libzlib-1.2.11-msvc2015-amd64-release.zip', '..\..\..\packages\zlib_64bit\zlib_64bit.zip')"
echo downloading packages [3/4]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/release/SDL-devel-1.2.15-VC.zip', '..\..\..\packages\sdl\sdl.zip')"
echo downloading packages [4/4]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-devel-1.2.12-VC.zip', '..\..\..\packages\sdl_mixer\sdl_mixer.zip')"

xcopy /Y /s /Q "setup_packages.bat" "..\..\..\packages"
cd "..\..\..\packages"

set sevenZipPath=

where 7z.exe >nul 2>nul
if %errorlevel% == 0 (
    set sevenZipPath=7z.exe
) else (
    if exist "C:\Program Files\7-Zip\7z.exe" (
        set sevenZipPath=C:\Program Files\7-Zip\7z.exe
    )
)

if not sevenZipPath == "" (
    echo unpacking packages [1/4]
    cd zlib_32bit
    "%sevenZipPath%" x zlib_32bit.zip -aoa > nul

    echo unpacking packages [2/4]
    cd ..\zlib_64bit
    "%sevenZipPath%" x zlib_64bit.zip -aoa > nul

    echo unpacking packages [3/4]
    cd ..\sdl
    "%sevenZipPath%" x sdl.zip -aoa > nul

    echo unpacking packages [4/4]
    cd ..\sdl_mixer
    "%sevenZipPath%" x sdl_mixer.zip -aoa > nul

    cd ..
    call "setup_packages.bat"
    echo "SUCCESS! Installation is completed"
) else (
    echo "Failed to unzip archives because 7-zip is not installed in system. Please unpack all archives in packages internal folders and manually run setup_packages.bat file after"
)
echo Press any key to exit...
pause >nul