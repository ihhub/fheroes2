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
#include <string>

#include "hero_info.h"
#include "race.h"
#include "translations.h"

const char * HeroInfo::getHeroName( const int heroId )
{
    const std::array<const char *, 73> names
        = { // knight
            _( "Lord Kilburn" ), _( "Sir Gallant" ), _( "Ector" ), _( "Gwenneth" ), _( "Tyro" ), _( "Ambrose" ), _( "Ruby" ), _( "Maximus" ), _( "Dimitry" ),
            // barbarian
            _( "Thundax" ), _( "Fineous" ), _( "Jojosh" ), _( "Crag Hack" ), _( "Jezebel" ), _( "Jaclyn" ), _( "Ergon" ), _( "Tsabu" ), _( "Atlas" ),
            // sorceress
            _( "Astra" ), _( "Natasha" ), _( "Troyan" ), _( "Vatawna" ), _( "Rebecca" ), _( "Gem" ), _( "Ariel" ), _( "Carlawn" ), _( "Luna" ),
            // warlock
            _( "Arie" ), _( "Alamar" ), _( "Vesper" ), _( "Crodo" ), _( "Barok" ), _( "Kastore" ), _( "Agar" ), _( "Falagar" ), _( "Wrathmont" ),
            // wizard
            _( "Myra" ), _( "Flint" ), _( "Dawn" ), _( "Halon" ), _( "Myrini" ), _( "Wilfrey" ), _( "Sarakin" ), _( "Kalindra" ), _( "Mandigal" ),
            // necromant
            _( "Zom" ), _( "Darlana" ), _( "Zam" ), _( "Ranloo" ), _( "Charity" ), _( "Rialdo" ), _( "Roxana" ), _( "Sandro" ), _( "Celia" ),
            // campains
            _( "Roland" ), _( "Lord Corlagon" ), _( "Sister Eliza" ), _( "Archibald" ), _( "Lord Halton" ), _( "Brother Brax" ),
            // loyalty version
            _( "Solmyr" ), _( "Dainwin" ), _( "Mog" ), _( "Uncle Ivan" ), _( "Joseph" ), _( "Gallavant" ), _( "Elderian" ), _( "Ceallach" ), _( "Drakonia" ),
            _( "Martine" ), _( "Jarkonas" ),
            // debug
            "Debug Hero", "Unknown" };

    return names[heroId];
}

int HeroInfo::getHeroRace( const int heroId )
{
    constexpr std::array<int, 73> races = {
        // knight
        Race::KNGT, // LORDKILBURN
        Race::KNGT, // SIRGALLANTH
        Race::KNGT, // ECTOR
        Race::KNGT, // GVENNETH
        Race::KNGT, // TYRO
        Race::KNGT, // AMBROSE
        Race::KNGT, // RUBY
        Race::KNGT, // MAXIMUS
        Race::KNGT, // DIMITRY

        // barbarian
        Race::BARB, // THUNDAX
        Race::BARB, // FINEOUS
        Race::BARB, // JOJOSH
        Race::BARB, // CRAGHACK
        Race::BARB, // JEZEBEL
        Race::BARB, // JACLYN
        Race::BARB, // ERGON
        Race::BARB, // TSABU
        Race::BARB, // ATLAS

        // sorceress
        Race::SORC, // ASTRA
        Race::SORC, // NATASHA
        Race::SORC, // TROYAN
        Race::SORC, // VATAWNA
        Race::SORC, // REBECCA
        Race::SORC, // GEM
        Race::SORC, // ARIEL
        Race::SORC, // CARLAWN
        Race::SORC, // LUNA

        // warlock
        Race::WRLK, // ARIE
        Race::WRLK, // ALAMAR
        Race::WRLK, // VESPER
        Race::WRLK, // CRODO
        Race::WRLK, // BAROK
        Race::WRLK, // KASTORE
        Race::WRLK, // AGAR
        Race::WRLK, // FALAGAR
        Race::WRLK, // WRATHMONT

        // wizard
        Race::WZRD, // MYRA
        Race::WZRD, // FLINT
        Race::WZRD, // DAWN
        Race::WZRD, // HALON
        Race::WZRD, // MYRINI
        Race::WZRD, // WILFREY
        Race::WZRD, // SARAKIN
        Race::WZRD, // KALINDRA
        Race::WZRD, // MANDIGAL

        // necromancer
        Race::NECR, // ZOM
        Race::NECR, // DARLANA
        Race::NECR, // ZAM
        Race::NECR, // RANLOO
        Race::NECR, // CHARITY
        Race::NECR, // RIALDO
        Race::NECR, // ROXANA
        Race::NECR, // SANDRO
        Race::NECR, // CELIA

        // from campain
        Race::WZRD, // ROLAND
        Race::KNGT, // CORLAGON
        Race::SORC, // ELIZA
        Race::WRLK, // ARCHIBALD
        Race::KNGT, // HALTON
        Race::NECR, // BAX

        // loyalty version
        Race::WZRD, // SOLMYR
        Race::WRLK, // DAINWIN
        Race::NECR, // MOG
        Race::BARB, // UNCLEIVAN
        Race::KNGT, // JOSEPH
        Race::KNGT, // GALLAVANT
        Race::WRLK, // ELDERIAN
        Race::KNGT, // CEALLACH
        Race::WZRD, // DRAKONIA
        Race::SORC, // MARTINE
        Race::BARB, // JARKONAS

        // devel
        Race::WRLK, // DEBUG_HERO
        Race::KNGT // UNKNOWN
    };

    return races[heroId];
}