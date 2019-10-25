@echo off

if not exist "..\zlib"         mkdir "..\zlib"
if not exist "..\zlib\include" mkdir "..\zlib\include"
if not exist "..\zlib\lib"     mkdir "..\zlib\lib"
if not exist "..\zlib\lib\x86" mkdir "..\zlib\lib\x86"
if not exist "..\zlib\lib\x64" mkdir "..\zlib\lib\x64"
if not exist "..\sdl"          mkdir "..\sdl"
if not exist "..\sdl\include"  mkdir "..\sdl\include"
if not exist "..\sdl\lib"      mkdir "..\sdl\lib"

xcopy /Y /s /Q "sdl\SDL-1.2.15\include" "..\sdl\include"
xcopy /Y /s /Q "sdl\SDL-1.2.15\lib" "..\sdl\lib"

xcopy /Y /s /Q "sdl_mixer\SDL_mixer-1.2.12\include"               "..\sdl\include"
xcopy /Y /s /Q "sdl_mixer\SDL_mixer-1.2.12\lib\x86\SDL_mixer.dll" "..\sdl\lib\x86"
xcopy /Y /s /Q "sdl_mixer\SDL_mixer-1.2.12\lib\x86\SDL_mixer.lib" "..\sdl\lib\x86"
xcopy /Y /s /Q "sdl_mixer\SDL_mixer-1.2.12\lib\x64\SDL_mixer.dll" "..\sdl\lib\x64"
xcopy /Y /s /Q "sdl_mixer\SDL_mixer-1.2.12\lib\x64\SDL_mixer.lib" "..\sdl\lib\x64"

xcopy /Y /s /Q "zlib_32bit\include" "..\zlib\include"
xcopy /Y /s /Q "zlib_32bit\lib" "..\zlib\lib\x86"
xcopy /Y /s /Q "zlib_64bit\lib" "..\zlib\lib\x64"
