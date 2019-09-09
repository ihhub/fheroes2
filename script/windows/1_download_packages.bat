@echo off

if not exist "..\..\..\packages" mkdir "..\..\..\packages"

echo downloading packages (1/4)
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libs4win.com/libzlib/libzlib-1.2.11-msvc2015-x86-release.zip', '..\..\..\packages\zlib_32bit.zip')"
echo downloading packages (2/4)
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libs4win.com/libzlib/libzlib-1.2.11-msvc2015-amd64-release.zip', '..\..\..\packages\zlib_64bit.zip')"
echo downloading packages (3/4)
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/release/SDL-devel-1.2.15-VC.zip', '..\..\..\packages\sdl.zip')"
echo downloading packages (4/4)
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-devel-1.2.12-VC.zip', '..\..\..\packages\sdl_mixer.zip')"
echo completed

if not exist "..\..\..\packages\zlib_32bit" mkdir "..\..\..\packages\zlib_32bit"
if not exist "..\..\..\packages\zlib_64bit" mkdir "..\..\..\packages\zlib_64bit"
if not exist "..\..\..\packages\sdl"        mkdir "..\..\..\packages\sdl"
if not exist "..\..\..\packages\sdl_mixer"  mkdir "..\..\..\packages\sdl_mixer"

xcopy /Y /s "2_setup_packages.bat" "..\..\..\packages"
