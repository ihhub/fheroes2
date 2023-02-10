:: ###########################################################################
:: #   fheroes2: https://github.com/ihhub/fheroes2                           #
:: #   Copyright (C) 2021 - 2023                                             #
:: #                                                                         #
:: #   This program is free software; you can redistribute it and/or modify  #
:: #   it under the terms of the GNU General Public License as published by  #
:: #   the Free Software Foundation; either version 2 of the License, or     #
:: #   (at your option) any later version.                                   #
:: #                                                                         #
:: #   This program is distributed in the hope that it will be useful,       #
:: #   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
:: #   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
:: #   GNU General Public License for more details.                          #
:: #                                                                         #
:: #   You should have received a copy of the GNU General Public License     #
:: #   along with this program; if not, write to the                         #
:: #   Free Software Foundation, Inc.,                                       #
:: #   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
:: ###########################################################################

@echo off

set PATH=%~dp0;%PATH%

set EXIT_CODE=0

call :extract_sprites || ^
set EXIT_CODE=1

if not "%CI%" == "true" (
    pause
)

exit /B %EXIT_CODE%

:extract_sprites

extractor agg *.AGG *.agg || exit /B 1
icn2img sprites agg\kb.pal agg\*.icn || exit /B 1

exit /B
