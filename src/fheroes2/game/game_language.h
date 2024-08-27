/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include <cstdint>

namespace fheroes2
{
    enum class SupportedLanguage : uint8_t
    {
        English = 0, // default language for all versions of the game.
        French, // GOG version
        Polish, // GOG version
        German, // GOG version
        Russian, // Buka and XXI Vek versions
        Italian, // Rare version?
        Czech, // Local release occurred in 2002 by CD Projekt
        Spanish, // Published by Proein. Only Succession Wars

        // All languages listed below are original to fheroes2.
        Belarusian,
        Bulgarian,
        Danish,
        Dutch,
        Hungarian,
        Norwegian,
        Portuguese,
        Romanian,
        Slovak,
        Swedish,
        Turkish,
        Ukrainian,
        Vietnamese
    };
}
