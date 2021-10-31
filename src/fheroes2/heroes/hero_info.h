/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#ifndef HERO_INFO_H
#define HERO_INFO_H

#include <array>

namespace HeroInfo
{
    enum class Id
    {
        // knight
        LORDKILBURN,
        SIRGALLANTH,
        ECTOR,
        GVENNETH,
        TYRO,
        AMBROSE,
        RUBY,
        MAXIMUS,
        DIMITRY,
        // barbarian
        THUNDAX,
        FINEOUS,
        JOJOSH,
        CRAGHACK,
        JEZEBEL,
        JACLYN,
        ERGON,
        TSABU,
        ATLAS,
        // sorceress
        ASTRA,
        NATASHA,
        TROYAN,
        VATAWNA,
        REBECCA,
        GEM,
        ARIEL,
        CARLAWN,
        LUNA,
        // warlock
        ARIE,
        ALAMAR,
        VESPER,
        CRODO,
        BAROK,
        KASTORE,
        AGAR,
        FALAGAR,
        WRATHMONT,
        // wizard
        MYRA,
        FLINT,
        DAWN,
        HALON,
        MYRINI,
        WILFREY,
        SARAKIN,
        KALINDRA,
        MANDIGAL,
        // necromancer
        ZOM,
        DARLANA,
        ZAM,
        RANLOO,
        CHARITY,
        RIALDO,
        ROXANA,
        SANDRO,
        CELIA,
        // From The Succession Wars campaign.
        ROLAND,
        CORLAGON,
        ELIZA,
        ARCHIBALD,
        HALTON,
        BAX,
        // From The Price of Loyalty expansion.
        SOLMYR,
        DAINWIN,
        MOG,
        UNCLEIVAN,
        JOSEPH,
        GALLAVANT,
        ELDERIAN,
        CEALLACH,
        DRAKONIA,
        MARTINE,
        JARKONAS,
        // debugger
        DEBUG_HERO,
        UNKNOWN
    };

    constexpr std::array<Id, 9> knight = { Id::LORDKILBURN, Id::SIRGALLANTH, Id::ECTOR, Id::GVENNETH, Id::TYRO, Id::AMBROSE, Id::RUBY, Id::MAXIMUS, Id::DIMITRY };

    constexpr std::array<Id, 9> barbarian = { Id::THUNDAX, Id::FINEOUS, Id::JOJOSH, Id::CRAGHACK, Id::JEZEBEL, Id::JACLYN, Id::ERGON, Id::TSABU, Id::ATLAS };

    constexpr std::array<Id, 9> sorceress = { Id::ASTRA, Id::NATASHA, Id::TROYAN, Id::VATAWNA, Id::REBECCA, Id::GEM, Id::ARIEL, Id::CARLAWN, Id::LUNA };

    constexpr std::array<Id, 9> warlock = { Id::ARIE, Id::ALAMAR, Id::VESPER, Id::CRODO, Id::BAROK, Id::KASTORE, Id::AGAR, Id::FALAGAR, Id::WRATHMONT };

    constexpr std::array<Id, 9> wizard = { Id::MYRA, Id::FLINT, Id::DAWN, Id::HALON, Id::MYRINI, Id::WILFREY, Id::SARAKIN, Id::KALINDRA, Id::MANDIGAL };

    constexpr std::array<Id, 9> necromancer = { Id::ZOM, Id::DARLANA, Id::ZAM, Id::RANLOO, Id::CHARITY, Id::RIALDO, Id::ROXANA, Id::SANDRO };

    constexpr std::array<Id, 60> successionWars
        = { Id::LORDKILBURN, Id::SIRGALLANTH, Id::ECTOR,    Id::GVENNETH, Id::TYRO,     Id::AMBROSE,   Id::RUBY,    Id::MAXIMUS,   Id::DIMITRY, Id::THUNDAX,
            Id::FINEOUS,     Id::JOJOSH,      Id::CRAGHACK, Id::JEZEBEL,  Id::JACLYN,   Id::ERGON,     Id::TSABU,   Id::ATLAS,     Id::ASTRA,   Id::NATASHA,
            Id::TROYAN,      Id::VATAWNA,     Id::REBECCA,  Id::GEM,      Id::ARIEL,    Id::CARLAWN,   Id::LUNA,    Id::ARIE,      Id::ALAMAR,  Id::VESPER,
            Id::CRODO,       Id::BAROK,       Id::KASTORE,  Id::AGAR,     Id::FALAGAR,  Id::WRATHMONT, Id::MYRA,    Id::FLINT,     Id::DAWN,    Id::HALON,
            Id::MYRINI,      Id::WILFREY,     Id::SARAKIN,  Id::KALINDRA, Id::MANDIGAL, Id::ZOM,       Id::DARLANA, Id::ZAM,       Id::RANLOO,  Id::CHARITY,
            Id::RIALDO,      Id::ROXANA,      Id::SANDRO,   Id::CELIA,    Id::ROLAND,   Id::CORLAGON,  Id::ELIZA,   Id::ARCHIBALD, Id::HALTON,  Id::BAX };

    constexpr std::array<Id, 71> priceOfLoyalty
        = { Id::LORDKILBURN, Id::SIRGALLANTH, Id::ECTOR,     Id::GVENNETH, Id::TYRO,    Id::AMBROSE, Id::RUBY,    Id::MAXIMUS, Id::DIMITRY,   Id::THUNDAX, Id::FINEOUS,
            Id::JOJOSH,      Id::CRAGHACK,    Id::JEZEBEL,   Id::JACLYN,   Id::ERGON,   Id::TSABU,   Id::ATLAS,   Id::ASTRA,   Id::NATASHA,   Id::TROYAN,  Id::VATAWNA,
            Id::REBECCA,     Id::GEM,         Id::ARIEL,     Id::CARLAWN,  Id::LUNA,    Id::ARIE,    Id::ALAMAR,  Id::VESPER,  Id::CRODO,     Id::BAROK,   Id::KASTORE,
            Id::AGAR,        Id::FALAGAR,     Id::WRATHMONT, Id::MYRA,     Id::FLINT,   Id::DAWN,    Id::HALON,   Id::MYRINI,  Id::WILFREY,   Id::SARAKIN, Id::KALINDRA,
            Id::MANDIGAL,    Id::ZOM,         Id::DARLANA,   Id::ZAM,      Id::RANLOO,  Id::CHARITY, Id::RIALDO,  Id::ROXANA,  Id::SANDRO,    Id::CELIA,   Id::ROLAND,
            Id::CORLAGON,    Id::ELIZA,       Id::ARCHIBALD, Id::HALTON,   Id::BAX,     Id::SOLMYR,  Id::DAINWIN, Id::MOG,     Id::UNCLEIVAN, Id::JOSEPH,  Id::GALLAVANT,
            Id::ELDERIAN,    Id::CEALLACH,    Id::DRAKONIA,  Id::MARTINE,  Id::JARKONAS };

    constexpr std::array<Id, 73> all
        = { Id::LORDKILBURN, Id::SIRGALLANTH, Id::ECTOR,    Id::GVENNETH,  Id::TYRO,     Id::AMBROSE,   Id::RUBY,     Id::MAXIMUS,   Id::DIMITRY,  Id::THUNDAX,
            Id::FINEOUS,     Id::JOJOSH,      Id::CRAGHACK, Id::JEZEBEL,   Id::JACLYN,   Id::ERGON,     Id::TSABU,    Id::ATLAS,     Id::ASTRA,    Id::NATASHA,
            Id::TROYAN,      Id::VATAWNA,     Id::REBECCA,  Id::GEM,       Id::ARIEL,    Id::CARLAWN,   Id::LUNA,     Id::ARIE,      Id::ALAMAR,   Id::VESPER,
            Id::CRODO,       Id::BAROK,       Id::KASTORE,  Id::AGAR,      Id::FALAGAR,  Id::WRATHMONT, Id::MYRA,     Id::FLINT,     Id::DAWN,     Id::HALON,
            Id::MYRINI,      Id::WILFREY,     Id::SARAKIN,  Id::KALINDRA,  Id::MANDIGAL, Id::ZOM,       Id::DARLANA,  Id::ZAM,       Id::RANLOO,   Id::CHARITY,
            Id::RIALDO,      Id::ROXANA,      Id::SANDRO,   Id::CELIA,     Id::ROLAND,   Id::CORLAGON,  Id::ELIZA,    Id::ARCHIBALD, Id::HALTON,   Id::BAX,
            Id::SOLMYR,      Id::DAINWIN,     Id::MOG,      Id::UNCLEIVAN, Id::JOSEPH,   Id::GALLAVANT, Id::ELDERIAN, Id::CEALLACH,  Id::DRAKONIA, Id::MARTINE,
            Id::JARKONAS,    Id::DEBUG_HERO,  Id::UNKNOWN };

    constexpr int lastId = static_cast<int>( Id::UNKNOWN );

    const char * getHeroName( const HeroInfo::Id & heroId );
    int getHeroRace( const HeroInfo::Id & heroId );
}

#endif
