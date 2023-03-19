#!/usr/bin/env bash

###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2023                                                    #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

set -e -o pipefail

PATH="$(dirname "$0"):$PATH"

extractor agg *.AGG *.agg
pal2img agg/*/kb.pal palette.png

for DIR in agg/*; do
    if [[ ! -d "$DIR" ]]; then
        continue
    fi

    82m2wav "wav/$(basename "$DIR")" "$DIR"/*.82m
    bin2txt "txt/$(basename "$DIR")" "$DIR"/*.bin
    icn2img "icn/$(basename "$DIR")" agg/*/kb.pal "$DIR"/*.icn
    til2img "til/$(basename "$DIR")" agg/*/kb.pal "$DIR"/*.til
    xmi2midi "midi/$(basename "$DIR")" "$DIR"/*.xmi
done

echo -e "\nAsset extraction completed successfully."
