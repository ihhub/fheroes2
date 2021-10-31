/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include <array>
#include <cassert>
#include <string>

#include "hero_info.h"
#include "race.h"
#include "translations.h"

namespace HeroInfo
{
    struct HeroType
    {
        const char * name;
        int race;
    };

    constexpr std::array<HeroType, 73> heroTypes = {
        { { "Lord Kilburn", Race::KNGT }, { "Sir Gallant", Race::KNGT },   { "Ector", Race::KNGT },        { "Gwenneth", Race::KNGT },   { "Tyro", Race::KNGT },
          { "Ambrose", Race::KNGT },      { "Ruby", Race::KNGT },          { "Maximus", Race::KNGT },      { "Dimitry", Race::KNGT },

          { "Thundax", Race::BARB },      { "Fineous", Race::BARB },       { "Jojosh", Race::BARB },       { "Crag Hack", Race::BARB },  { "Jezebel", Race::BARB },
          { "Jaclyn", Race::BARB },       { "Ergon", Race::BARB },         { "Tsabu", Race::BARB },        { "Atlas", Race::BARB },

          { "Astra", Race::SORC },        { "Natasha", Race::SORC },       { "Troyan", Race::SORC },       { "Vatawna", Race::SORC },    { "Rebecca", Race::SORC },
          { "Gem", Race::SORC },          { "Ariel", Race::SORC },         { "Carlawn", Race::SORC },      { "Luna", Race::SORC },

          { "Arie", Race::WRLK },         { "Alamar", Race::WRLK },        { "Vesper", Race::WRLK },       { "Crodo", Race::WRLK },      { "Barok", Race::WRLK },
          { "Kastore", Race::WRLK },      { "Agar", Race::WRLK },          { "Falagar", Race::WRLK },      { "Wrathmont", Race::WRLK },

          { "Myra", Race::WZRD },         { "Flint", Race::WZRD },         { "Dawn", Race::WZRD },         { "Halon", Race::WZRD },      { "Myrini", Race::WZRD },
          { "Wilfrey", Race::WZRD },      { "Sarakin", Race::WZRD },       { "Kalindra", Race::WZRD },     { "Mandigal", Race::WZRD },

          { "Zom", Race::NECR },          { "Darlana", Race::NECR },       { "Zam", Race::NECR },          { "Ranloo", Race::NECR },     { "Charity", Race::NECR },
          { "Rialdo", Race::NECR },       { "Roxana", Race::NECR },        { "Sandro", Race::NECR },       { "Celia", Race::NECR },

          { "Roland", Race::WZRD },       { "Lord Corlagon", Race::KNGT }, { "Sister Eliza", Race::SORC }, { "Archibald", Race::WRLK },  { "Lord Halton", Race::KNGT },
          { "Brother Brax", Race::NECR },

          { "Solmyr", Race::WZRD },       { "Dainwin", Race::WRLK },       { "Mog", Race::NECR },          { "Uncle Ivan", Race::BARB }, { "Joseph", Race::KNGT },
          { "Gallavant", Race::KNGT },    { "Elderian", Race::WRLK },      { "Ceallach", Race::KNGT },     { "Drakonia", Race::WZRD },   { "Martine", Race::SORC },
          { "Jarkonas", Race::BARB },     { "Debug Hero", Race::WRLK },    { "Unknown", Race::KNGT }

        } };

}

const char * HeroInfo::getHeroName( const HeroInfo::Id & heroId )
{
    return _( heroTypes[static_cast<int>( heroId )].name );
}

int HeroInfo::getHeroRace( const HeroInfo::Id & heroId )
{
    return heroTypes[static_cast<int>( heroId )].race;
}