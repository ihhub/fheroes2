/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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
#include "math_base.h"
#include "ui_language.h"

namespace fheroes2
{
    class Sprite;

    // this class is used for a situations when we need to remove letter specific offsets, like when we display single letters in a row,
    // and then restore these offsets within the scope of the code
    class ButtonFontRestorer
    {
    public:
        ButtonFontRestorer( std::vector<fheroes2::Sprite> & font, const fheroes2::Point & newOffsets );
        ButtonFontRestorer( const ButtonFontRestorer & ) = delete;

        ~ButtonFontRestorer();

        ButtonFontRestorer & operator=( const ButtonFontRestorer & ) = delete;

    private:
        std::vector<fheroes2::Sprite> & _font;
        const std::vector<fheroes2::Sprite> _originalOffsets;
    };

    void generateAlphabet( const SupportedLanguage language, std::vector<std::vector<Sprite>> & icnVsSprite );

    bool isAlphabetSupported( const SupportedLanguage language );

    void generateBaseButtonFont( std::vector<Sprite> & goodReleased, std::vector<Sprite> & goodPressed, std::vector<Sprite> & evilReleased,
                                 std::vector<Sprite> & evilPressed );

    void generateButtonAlphabet( const SupportedLanguage language, std::vector<std::vector<Sprite>> & icnVsSprite );

    void modifyBaseNormalFont( std::vector<fheroes2::Sprite> & icnVsSprite );

    void modifyBaseSmallFont( std::vector<fheroes2::Sprite> & icnVsSprite );
}
