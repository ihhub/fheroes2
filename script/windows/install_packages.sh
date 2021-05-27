#!/usr/bin/bash

if ! [[ $1 = 'dev' ]]; then
	v=2
	prj=_mixer
else
	v={,2}
	prj={_mixer,_image,_ttf}
fi

echo "Installing missing packages"
eval pacman -S --noconfirm --needed base-devel p7zip ${MINGW_PACKAGE_PREFIX}-{toolchain,SDL${v}${prj}}
