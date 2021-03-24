#!/usr/bin/bash

if [ ! -d src/fheroes2 ]; then
    echo "Must run script from base directory"
    exit 1
fi

if [ ! -f fheroes2.exe ]; then
	read -p "fheroes2.exe not found! Would you like to build it now? (default=yes)" yn
        case $yn in
         ''|[Yy]* ) make -j$(nproc); script/msys/release.sh; exit 0;;
            [Nn]* ) echo "You must build fheroes2.exe prior to running this script!"; exit 1;;
        esac
fi

fh2arch=x$(objdump -h fheroes2.exe | grep format | tail -c3)
fh2sdl=$(ldd fheroes2.exe | grep SDL2)
version=$(cat src/fheroes2/system/version.h | grep define | cut -d" " -f3 | tr '\n' '.' | sed 's/.$//')

if [[ -z $fh2sdl ]]; then
    fh2sdl=SDL1
else
    fh2sdl=SDL2
fi

if   [ $fh2arch = x86 ]; then
    if [ $fh2sdl = SDL1 ]; then
        sdl="https://www.libsdl.org/release/SDL-1.2.15-win32.zip"
        mix="https://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-1.2.12-win32.zip"
        # ttf="https://www.libsdl.org/projects/SDL_ttf/release/SDL_ttf-2.0.11-win32.zip"
		# img="https://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.12-win32.zip"
    else
        sdl="https://www.libsdl.org/release/SDL2-2.0.14-win32-x86.zip"
        mix="https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.0.4-win32-x86.zip"
        # ttf="https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15-win32-x86.zip"
		# img="https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.5-win32-x86.zip"
    fi 
elif [ $fh2arch = x64 ]; then
    if [ $fh2sdl = SDL1 ]; then
        sdl="https://www.libsdl.org/release/SDL-1.2.15-win32-x64.zip"
        mix="https://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-1.2.12-win32-x64.zip"
        # ttf="https://www.libsdl.org/projects/SDL_ttf/release/SDL_ttf-2.0.11-win32-x64.zip"
		# img="https://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.12-win32-x64.zip"
    else
        sdl="https://www.libsdl.org/release/SDL2-2.0.14-win32-x64.zip"
        mix="https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.0.4-win32-x64.zip"
        # ttf="https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15-win32-x64.zip"
		# img="https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.5-win32-x64.zip"
    fi
else
    echo "Error! You're doing something wrong!"
    exit 1
fi

echo "Downloading $fh2sdl $fh2arch libraries"
wget -q -c -P package $sdl $mix $ttf $img
7z e package/\*.zip -aos -bs{o,p}0  -x{\!*FLAC*,\!*mikmod*,\!*modplug*,\!*mpg123*,\!*opus*,\!*smpeg*,\!*jpeg*,\!*libpng*,\!*tiff*,\!*webp*,!README.txt} -opackage
rm -f package/*.zip
rm -f fheroes2_${fh2arch}_$fh2sdl-$version.zip
echo "Packing fheroes2_${fh2arch}_$fh2sdl-$version.zip"
cp LICENSE package/License.txt
# 7z a -tzip fheroes2_${fh2arch}_$fh2sdl-$version.zip -stl -mx9 -bs{o,p}0 ./package/* ./doc/README.txt changelog.txt fheroes2.key fheroes2.exe files/fonts/* files/lang/*.po ./script/demo/download_demo.bat
7z a -tzip fheroes2_${fh2arch}_$fh2sdl-$version.zip -stl -mx9 -bs{o,p}0 ./package/* ./doc/README.txt changelog.txt fheroes2.key fheroes2.exe ./script/demo/download_demo.bat
rm -rf package
