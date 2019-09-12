@echo off

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
    cd zlib_32bit
    "%sevenZipPath%" 7z x zlib_32bit.zip -aoa > nul
    cd ..\zlib_64bit
    "%sevenZipPath%" 7z x zlib_64bit.zip -aoa > nul
    cd ..\sdl
    "%sevenZipPath%" 7z x sdl.zip -aoa > nul
    cd ..\sdl_mixer
    "%sevenZipPath%" 7z x sdl_mixer.zip -aoa > nul
    cd ..  
) else (
    @echo 7z.exe not found in path. Please unzip files manually.
	exit /b 1
)
