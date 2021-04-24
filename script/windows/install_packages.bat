@echo off

set tempPath="..\..\VisualStudio\packages\installed\temp"

if not exist "%tempPath%\zlib"      mkdir "%tempPath%\zlib"
if not exist "%tempPath%\sdl"       mkdir "%tempPath%\sdl"
if not exist "%tempPath%\sdl_mixer" mkdir "%tempPath%\sdl_mixer"
if not exist "%tempPath%\sdl_ttf"   mkdir "%tempPath%\sdl_ttf"
if not exist "%tempPath%\sdl_image" mkdir "%tempPath%\sdl_image"

echo [1/7] Copying packages
xcopy /Y /Q "..\..\VisualStudio\packages\zlib1.2.11.zip" "%tempPath%\zlib\"
echo [2/7] Downloading packages
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/release/SDL-devel-1.2.15-VC.zip', '%tempPath%\sdl\sdl.zip')"
echo [3/7] Downloading packages
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/release/SDL2-devel-2.0.12-VC.zip', '%tempPath%\sdl\sdl2.zip')"
echo [4/7] Downloading packages
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-devel-1.2.12-VC.zip', '%tempPath%\sdl_mixer\sdl_mixer.zip')"
echo [5/7] Downloading packages
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-VC.zip', '%tempPath%\sdl_mixer\sdl_mixer2.zip')"
echo [6/7] Downloading packages
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-2.0.15-VC.zip', '%tempPath%\sdl_ttf\sdl_ttf2.zip')"
echo [7/7] Downloading packages
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-2.0.5-VC.zip', '%tempPath%\sdl_image\sdl_image2.zip')"

xcopy /Y /s /Q "setup_packages.bat" "..\..\VisualStudio\packages\installed"
cd "%tempPath%"

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
    echo [1/7] Unpacking packages
    "%sevenZipPath%" x zlib\zlib1.2.11.zip -aoa -ozlib > nul

    echo [2/7] Unpacking packages
    "%sevenZipPath%" x sdl\sdl.zip -aoa -osdl > nul

    echo [3/7] Unpacking packages
    "%sevenZipPath%" x sdl\sdl2.zip -aoa -osdl > nul

    echo [4/7] Unpacking packages
    "%sevenZipPath%" x sdl_mixer\sdl_mixer.zip -aoa -osdl_mixer > nul

    echo [5/7] Unpacking packages
    "%sevenZipPath%" x sdl_mixer\sdl_mixer2.zip -aoa -osdl_mixer > nul

    echo [6/7] Unpacking packages
    "%sevenZipPath%" x sdl_ttf\sdl_ttf2.zip -aoa -osdl_ttf > nul

    echo [7/7] Unpacking packages
    "%sevenZipPath%" x sdl_image\sdl_image2.zip -aoa -osdl_image > nul

    cd ..
    call "setup_packages.bat"
    del "setup_packages.bat"

    echo "SUCCESS! Installation is completed"
) else (
    echo "Failed to unzip archives because 7-zip is not installed in system. Please unpack all archives in packages internal folders and manually run setup_packages.bat file after."
)

if not "%APPVEYOR_REPO_PROVIDER%" == "gitHub" (
    echo Press any key to exit...
    pause >nul
)
