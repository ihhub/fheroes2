:: ###########################################################################
:: #   fheroes2: https://github.com/ihhub/fheroes2                           #
:: #   Copyright (C) 2023                                                    #
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

call :extract_icn || ^
set EXIT_CODE=1

if not "%CI%" == "true" (
    pause
)

exit /B %EXIT_CODE%

:extract_icn

extractor agg *.AGG *.agg || exit /B 1
icn2img icn\HEROES2 agg\HEROES2\kb.pal agg\HEROES2\*.icn || exit /B 1
icn2img icn\HEROES2X agg\HEROES2\kb.pal agg\HEROES2X\*.icn || exit /B 1
82m2wav wav\HEROES2 agg\HEROES2\*.82m || exit /B 1
xmi2midi midi\HEROES2 agg\HEROES2\*.xmi || exit /B 1
xmi2midi midi\HEROES2X agg\HEROES2X\*.xmi || exit /B 1
pal2img agg\HEROES2\kb.pal palette.png || exit /B 1

exit /B
