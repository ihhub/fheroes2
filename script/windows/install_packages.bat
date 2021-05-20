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

echo [1/7] Unpacking packages
powershell -Command "Expand-Archive -LiteralPath 'zlib\zlib1.2.11.zip' -DestinationPath 'zlib' -Force"

echo [2/7] Unpacking packages
powershell -Command "Expand-Archive -LiteralPath 'sdl\sdl.zip' -DestinationPath 'sdl' -Force"

echo [3/7] Unpacking packages
powershell -Command "Expand-Archive -LiteralPath 'sdl\sdl2.zip' -DestinationPath 'sdl' -Force"

echo [4/7] Unpacking packages
powershell -Command "Expand-Archive -LiteralPath 'sdl_mixer\sdl_mixer.zip' -DestinationPath 'sdl_mixer' -Force"

echo [5/7] Unpacking packages
powershell -Command "Expand-Archive -LiteralPath 'sdl_mixer\sdl_mixer2.zip' -DestinationPath 'sdl_mixer' -Force"

echo [6/7] Unpacking packages
powershell -Command "Expand-Archive -LiteralPath 'sdl_ttf\sdl_ttf2.zip' -DestinationPath 'sdl_ttf' -Force"

echo [7/7] Unpacking packages
powershell -Command "Expand-Archive -LiteralPath 'sdl_image\sdl_image2.zip' -DestinationPath 'sdl_image' -Force"

cd ..

call "setup_packages.bat"
del "setup_packages.bat"

echo "SUCCESS! Installation is completed"

if not "%APPVEYOR_REPO_PROVIDER%" == "gitHub" (
    echo Press any key to exit...
    pause >nul
)
