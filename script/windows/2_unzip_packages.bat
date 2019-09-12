@echo off

where 7z.exe >nul 2>nul
IF NOT ERRORLEVEL 0 (
    @echo 7z.exe not found in path. Please unzip files manually.
    exit 1
)

cd zlib_32bit
7z x zlib_32bit.zip > nul
cd ..\zlib_64bit
7z x zlib_64bit.zip > nul
cd ..\sdl
7z x sdl.zip > nul
cd ..\sdl_mixer
7z x sdl_mixer.zip > nul
cd ..
