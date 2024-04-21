/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2024                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

#include <vector>

#include "game_language.h"

namespace fheroes2
{
    class Sprite;

    void generateAlphabet( const SupportedLanguage language, std::vector<std::vector<Sprite>> & icnVsSprite );

    bool isAlphabetSupported( const SupportedLanguage language );

    void generateBaseButtonFont( std::vector<Sprite> & goodReleased, std::vector<Sprite> & goodPressed, std::vector<Sprite> & evilReleased,
                                 std::vector<Sprite> & evilPressed );

    void generateButtonAlphabet( const SupportedLanguage language, std::vector<std::vector<Sprite>> & icnVsSprite );

    void modifyBaseNormalFont( std::vector<fheroes2::Sprite> & icnVsSprite );

    void modifyBaseSmallFont( std::vector<fheroes2::Sprite> & icnVsSprite );
}
