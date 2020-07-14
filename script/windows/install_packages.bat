@echo off

if not exist "..\..\..\packages"            mkdir "..\..\..\packages"
if not exist "..\..\..\packages\zlib"       mkdir "..\..\..\packages\zlib"
if not exist "..\..\..\packages\sdl"        mkdir "..\..\..\packages\sdl"
if not exist "..\..\..\packages\sdl_mixer"  mkdir "..\..\..\packages\sdl_mixer"
if not exist "..\..\..\packages\sdl_net"    mkdir "..\..\..\packages\sdl_net"

echo copying packages [1/7]
xcopy /Y /s /Q "..\..\VisualStudio\packages\zlib1.2.11.zip"  "..\..\..\packages\zlib"
echo downloading packages [2/7]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/release/SDL-devel-1.2.15-VC.zip', '..\..\..\packages\sdl\sdl.zip')"
echo downloading packages [3/7]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-devel-1.2.12-VC.zip', '..\..\..\packages\sdl_mixer\sdl_mixer.zip')"
echo downloading packages [4/7]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/release/SDL2-devel-2.0.10-VC.zip', '..\..\..\packages\sdl\sdl2.zip')"
echo downloading packages [5/7]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-VC.zip', '..\..\..\packages\sdl_mixer\sdl_mixer2.zip')"
echo downloading packages [6/7]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_net/release/SDL_net-devel-1.2.8-VC.zip', '..\..\..\packages\sdl_net\sdl_net.zip')"
echo downloading packages [7/7]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_net/release/SDL2_net-devel-2.0.1-VC.zip', '..\..\..\packages\sdl_net\sdl_net2.zip')"

xcopy /Y /s /Q "setup_packages.bat" "..\..\..\packages"
cd "..\..\..\packages"

set sevenZipPath=

where 7z.exe >nul 2>nul
if %errorlevel% == 0 (
    set sevenZipPath=7z.exe
) else (
    if exist "%ProgramFiles%\7-Zip\7z.exe" (
        set sevenZipPath=%ProgramFiles%\7-Zip\7z.exe
    )
)

if not "%sevenZipPath%" == "" (
    echo unpacking packages [1/7]
    cd zlib
    "%sevenZipPath%" x zlib1.2.11.zip -aoa > nul

    echo unpacking packages [2/7]
    cd ..\sdl
    "%sevenZipPath%" x sdl.zip -aoa > nul

    echo unpacking packages [3/7]
    cd ..\sdl_mixer
    "%sevenZipPath%" x sdl_mixer.zip -aoa > nul

    echo unpacking packages [4/7]
    cd ..\sdl_net
    "%sevenZipPath%" x sdl_net.zip -aoa > nul

    echo unpacking packages [5/7]
    cd ..\sdl
    "%sevenZipPath%" x sdl2.zip -aoa > nul

    echo unpacking packages [6/7]
    cd ..\sdl_mixer
    "%sevenZipPath%" x sdl_mixer2.zip -aoa > nul

    echo unpacking packages [7/7]
    cd ..\sdl_net
    "%sevenZipPath%" x sdl_net2.zip -aoa > nul

    cd ..
    call "setup_packages.bat"
    echo "SUCCESS! Installation is completed"
) else (
    echo "Failed to unzip archives because 7-zip is not installed in system. Please unpack all archives in packages internal folders and manually run setup_packages.bat file after"
)

if not "%APPVEYOR_REPO_PROVIDER%" == "gitHub" (
    echo Press any key to exit...
    pause >nul
)
