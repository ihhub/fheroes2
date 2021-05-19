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
if not exist %pkgPath%\zlib mkdir %pkgPath%\zlib
powershell -Command "$shell = new-object -com shell.application; $zip = $shell.NameSpace((Resolve-Path '%pkgPath%\zlib1.2.11.zip').Path); foreach($item in $zip.items()) { $shell.Namespace((Resolve-Path '%pkgPath%\zlib').Path).copyhere($item, 0x14) }"
powershell -command "$shell = new-object -com shell.application; Get-ChildItem %pkgPath%\*.zip -exclude zlib*.zip | %%{$zip = $shell.NameSpace($_.FullName); foreach($item in $zip.items()) { $shell.Namespace((Resolve-Path '%pkgPath%').Path).copyhere($item, 0x14) }}"

echo Tidying up...
xcopy /Y /s /Q "%pkgPath%\SDL-1.2.15\"       "%pkgPath%\sdl\" >nul
xcopy /Y /s /Q "%pkgPath%\SDL_mixer-1.2.12\" "%pkgPath%\sdl\" >nul
xcopy /Y /s /Q "%pkgPath%\SDL2-2.0.14\"      "%pkgPath%\sdl2\" >nul
xcopy /Y /s /Q "%pkgPath%\SDL2_mixer-2.0.4\" "%pkgPath%\sdl2\" >nul
REM xcopy /Y /s /Q "%pkgPath%\SDL2_ttf-2.0.15\"  "%pkgPath%\sdl2\" >nul
REM xcopy /Y /s /Q "%pkgPath%\SDL2_image-2.0.5\" "%pkgPath%\sdl2\" >nul
rd /S /Q "%pkgPath%\SDL-1.2.15"
rd /S /Q "%pkgPath%\SDL_mixer-1.2.12"
rd /S /Q "%pkgPath%\SDL2-2.0.14"
rd /S /Q "%pkgPath%\SDL2_mixer-2.0.4"
REM rd /S /Q "%pkgPath%\SDL2_ttf-2.0.15"
REM rd /S /Q "%pkgPath%\SDL2_image-2.0.5"
del /F /Q %pkgPath%\sdl*.zip >nul

if not "%APPVEYOR_REPO_PROVIDER%" == "gitHub" (
    echo All done!
    pause
)
