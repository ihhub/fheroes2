@echo off

set VCPKG_ROOT=%~1
set PLATFORM=%~2
set PACKAGES=%~3

"%VCPKG_ROOT%\vcpkg.exe" --triplet "%PLATFORM%-windows" install sdl2 sdl2-mixer[fluidsynth,libflac,libvorbis,mpg123] sdl2-image zlib || ^
exit 1

if not exist "%PACKAGES%\zlib\include"          ( mkdir "%PACKAGES%\zlib\include"          || exit 1 )
if not exist "%PACKAGES%\zlib\lib\%PLATFORM%"   ( mkdir "%PACKAGES%\zlib\lib\%PLATFORM%"   || exit 1 )
if not exist "%PACKAGES%\sdl2\include"          ( mkdir "%PACKAGES%\sdl2\include"          || exit 1 )
if not exist "%PACKAGES%\sdl2\lib\%PLATFORM%"   ( mkdir "%PACKAGES%\sdl2\lib\%PLATFORM%"   || exit 1 )
if not exist "%PACKAGES%\extras\lib\%PLATFORM%" ( mkdir "%PACKAGES%\extras\lib\%PLATFORM%" || exit 1 )

xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2_%PLATFORM%-windows\bin\SDL2.dll"                  "%PACKAGES%\sdl2\lib\%PLATFORM%"   && ^
xcopy /Y /Q /S "%VCPKG_ROOT%\packages\sdl2_%PLATFORM%-windows\include\SDL2"                  "%PACKAGES%\sdl2\include"          && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2_%PLATFORM%-windows\lib\SDL2.lib"                  "%PACKAGES%\sdl2\lib\%PLATFORM%"   || ^
exit 1

xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2-image_%PLATFORM%-windows\bin\SDL2_image.dll"      "%PACKAGES%\sdl2\lib\%PLATFORM%"   && ^
xcopy /Y /Q /S "%VCPKG_ROOT%\packages\sdl2-image_%PLATFORM%-windows\include\SDL2"            "%PACKAGES%\sdl2\include"          && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2-image_%PLATFORM%-windows\lib\SDL2_image.lib"      "%PACKAGES%\sdl2\lib\%PLATFORM%"   || ^
exit 1

xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2-mixer_%PLATFORM%-windows\bin\SDL2_mixer.dll"      "%PACKAGES%\sdl2\lib\%PLATFORM%"   && ^
xcopy /Y /Q /S "%VCPKG_ROOT%\packages\sdl2-mixer_%PLATFORM%-windows\include\SDL2"            "%PACKAGES%\sdl2\include"          && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2-mixer_%PLATFORM%-windows\lib\SDL2_mixer.lib"      "%PACKAGES%\sdl2\lib\%PLATFORM%"   || ^
exit 1

xcopy /Y /Q    "%VCPKG_ROOT%\packages\zlib_%PLATFORM%-windows\bin\zlib1.dll"                 "%PACKAGES%\zlib\lib\%PLATFORM%"   && ^
xcopy /Y /Q /S "%VCPKG_ROOT%\packages\zlib_%PLATFORM%-windows\include"                       "%PACKAGES%\zlib\include"          && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\zlib_%PLATFORM%-windows\lib\zlib.lib"                  "%PACKAGES%\zlib\lib\%PLATFORM%"   || ^
exit 1

xcopy /Y /Q    "%VCPKG_ROOT%\packages\fluidsynth_%PLATFORM%-windows\bin\libfluidsynth-3.dll" "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\gettext_%PLATFORM%-windows\bin\intl-8.dll"             "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\glib_%PLATFORM%-windows\bin\gio-2.0-0.dll"             "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\glib_%PLATFORM%-windows\bin\glib-2.0-0.dll"            "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\glib_%PLATFORM%-windows\bin\gmodule-2.0-0.dll"         "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\glib_%PLATFORM%-windows\bin\gobject-2.0-0.dll"         "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\glib_%PLATFORM%-windows\bin\gthread-2.0-0.dll"         "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libflac_%PLATFORM%-windows\bin\FLAC.dll"               "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libiconv_%PLATFORM%-windows\bin\iconv-2.dll"           "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libogg_%PLATFORM%-windows\bin\ogg.dll"                 "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libpng_%PLATFORM%-windows\bin\libpng16.dll"            "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libvorbis_%PLATFORM%-windows\bin\vorbis.dll"           "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libvorbis_%PLATFORM%-windows\bin\vorbisenc.dll"        "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libvorbis_%PLATFORM%-windows\bin\vorbisfile.dll"       "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\mpg123_%PLATFORM%-windows\bin\mpg123.dll"              "%PACKAGES%\extras\lib\%PLATFORM%" && ^
xcopy /Y /Q    "%VCPKG_ROOT%\packages\pcre_%PLATFORM%-windows\bin\pcre.dll"                  "%PACKAGES%\extras\lib\%PLATFORM%" || ^
exit 1
