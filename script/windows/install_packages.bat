@echo off

set pkgPath=..\..\VisualStudio\packages

echo [1/4] Downloading SDL...
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/release/SDL-devel-1.2.15-VC.zip', '%pkgPath%\sdl.zip')"
echo [2/4] Downloading SDL2...
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/release/SDL2-devel-2.0.14-VC.zip', '%pkgPath%\sdl2.zip')"
echo [3/4] Downloading SDL_Mixer...
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-devel-1.2.12-VC.zip', '%pkgPath%\sdl_mixer.zip')"
echo [4/4] Downloading SDL2_Mixer...
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-VC.zip', '%pkgPath%\sdl2_mixer.zip')"
REM echo [5/6] Downloading SDL2_TTF...
REM powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-2.0.15-VC.zip', '%pkgPath%\sdl2_ttf.zip')"
REM echo [6/6] Downloading SDL2_Image...
REM powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-2.0.5-VC.zip', '%pkgPath%\sdl2_image.zip')"

echo Extracting archives...
for /f usebackq %%g in (
`powershell -Command $PSVersionTable.PSVersion.Major`) do (
if %%g lss 4 (
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.willus.com/archive/zip64/unzip.exe','unzip.exe')"
unzip -o -qq -L %pkgPath%\zlib1.2.11.zip -d %pkgPath%\zlib
unzip -o %pkgPath%\sdl*.zip */lib/* */include/* */README*.* -d %pkgPath%
del unzip.exe
) else (
powershell -command "Expand-Archive -Force  %pkgPath%\zlib1.2.11.zip -DestinationPath %pkgPath%\zlib"
powershell -command "Get-ChildItem %pkgPath%\*.zip -exclude zlib*.zip | Expand-Archive -Force -DestinationPath %pkgPath%"
)
)
xcopy /Y /s /Q "%pkgPath%\SDL-1.2.15\"       "%pkgPath%\sdl\" >nul
xcopy /Y /s /Q "%pkgPath%\SDL_mixer-1.2.12\" "%pkgPath%\sdl\" >nul
xcopy /Y /s /Q "%pkgPath%\SDL2-2.0.14\"      "%pkgPath%\sdl2\" >nul
xcopy /Y /s /Q "%pkgPath%\SDL2_mixer-2.0.4\" "%pkgPath%\sdl2\" >nul
xcopy /Y /s /Q "%pkgPath%\SDL2_ttf-2.0.15\"  "%pkgPath%\sdl2\" >nul
xcopy /Y /s /Q "%pkgPath%\SDL2_image-2.0.5\" "%pkgPath%\sdl2\" >nul
rd /S /Q "%pkgPath%\SDL-1.2.15"
rd /S /Q "%pkgPath%\SDL_mixer-1.2.12"
rd /S /Q "%pkgPath%\SDL2-2.0.14"
rd /S /Q "%pkgPath%\SDL2_mixer-2.0.4"
rd /S /Q "%pkgPath%\SDL2_ttf-2.0.15"
rd /S /Q "%pkgPath%\SDL2_image-2.0.5"

if not "%APPVEYOR_REPO_PROVIDER%" == "gitHub" (
    echo All done!
    pause
)
