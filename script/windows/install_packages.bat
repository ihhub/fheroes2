@echo off

echo [1/4] Downloading packages
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/release/SDL-devel-1.2.15-VC.zip', '..\..\VisualStudio\packages\sdl.zip')"

echo [2/4] Downloading packages
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/release/SDL2-devel-2.0.14-VC.zip', '..\..\VisualStudio\packages\sdl2.zip')"

echo [3/4] Downloading packages
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-devel-1.2.12-VC.zip', '..\..\VisualStudio\packages\sdl_mixer.zip')"

echo [4/4] Downloading packages
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-VC.zip', '..\..\VisualStudio\packages\sdl_mixer2.zip')"

REM echo [5/6] Downloading packages
REM powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-2.0.15-VC.zip', '..\..\VisualStudio\packages\sdl_ttf2.zip')"

REM echo [6/6] Downloading packages
REM powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-2.0.5-VC.zip', '..\..\VisualStudio\packages\sdl_image2.zip')"

echo [1/5] Unpacking packages
powershell -command "Expand-Archive -Force '..\..\VisualStudio\packages\zlib1.2.11.zip' '..\..\VisualStudio\packages\zlib'"

echo [2/5] Unpacking packages
powershell -command "Expand-Archive -Force '..\..\VisualStudio\packages\sdl.zip' '..\..\VisualStudio\packages\sdl'"

echo [3/5] Unpacking packages
powershell -command "Expand-Archive -Force '..\..\VisualStudio\packages\sdl2.zip' '..\..\VisualStudio\packages\sdl2'"

echo [4/5] Unpacking packages
powershell -command "Expand-Archive -Force '..\..\VisualStudio\packages\sdl_mixer.zip' '..\..\VisualStudio\packages\sdl'"

echo [5/5] Unpacking packages
powershell -command "Expand-Archive -Force '..\..\VisualStudio\packages\sdl_mixer2.zip' '..\..\VisualStudio\packages\sdl2'"

REM echo [6/7] Unpacking packages
REM powershell -command "Expand-Archive -Force '..\..\VisualStudio\packages\sdl_ttf2.zip' '..\..\VisualStudio\packages\sdl2'"

REM echo [7/7] Unpacking packages
REM powershell -command "Expand-Archive -Force '..\..\VisualStudio\packages\sdl_image2.zip' '..\..\VisualStudio\packages\sdl2'"

echo Cleaning up...
powershell -command "Remove-Item '..\..\VisualStudio\packages\sdl*\*\docs' -Recurse -Force"
powershell -command "Copy-Item -Path '..\..\VisualStudio\packages\sdl\SDL*\*' -Exclude "*FLAC*", "*mikmod*", "*smpeg*" -Destination '..\..\VisualStudio\packages\sdl' -Recurse -Force"
powershell -command "Copy-Item -Path '..\..\VisualStudio\packages\sdl2\SDL*\*' -Exclude "*FLAC*", "*modplug*", "*mpg123*", "*opus*", "*jpeg*", "*tiff*", "*webp*", "*zlib*" -Destination '..\..\VisualStudio\packages\sdl2' -Recurse -Force"
powershell -command "Remove-Item '..\..\VisualStudio\packages\sdl*\*' -Exclude *LICENSE*, "README-SDL.txt" -Include *.txt,*.html -Recurse -Force"
powershell -command "Remove-Item '..\..\VisualStudio\packages\sdl*\SDL*' -Recurse -Force"
powershell -command "Remove-Item '..\..\VisualStudio\packages\*' -Exclude *zlib* -Include *.zip -Recurse -Force"


echo SUCCESS! Installation is complete.


if not "%APPVEYOR_REPO_PROVIDER%" == "gitHub" (
    echo Press any key to exit...
    pause >nul
)
