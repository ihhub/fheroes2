/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "m82.h"

#include "maps_tiles.h"
#include "mp2.h"
#include "spell.h"

namespace M82
{
    const struct
    {
        int type;
        const char * string;
    } m82map[] = { { AELMATTK, "AELMATTK.82M" }, { AELMKILL, "AELMKILL.82M" }, { AELMMOVE, "AELMMOVE.82M" }, { AELMWNCE, "AELMWNCE.82M" }, { ANTIMAGK, "ANTIMAGK.82M" },
                   { ARCHATTK, "ARCHATTK.82M" }, { ARCHKILL, "ARCHKILL.82M" }, { ARCHMOVE, "ARCHMOVE.82M" }, { ARCHSHOT, "ARCHSHOT.82M" }, { ARCHWNCE, "ARCHWNCE.82M" },
                   { ARMGEDN, "ARMGEDN.82M" },   { BADLUCK, "BADLUCK.82M" },   { BADMRLE, "BADMRLE.82M" },   { BERZERK, "BERZERK.82M" },   { BLESS, "BLESS.82M" },
                   { BLIND, "BLIND.82M" },       { BLOODLUS, "BLOODLUS.82M" }, { BOARATTK, "BOARATTK.82M" }, { BOARKILL, "BOARKILL.82M" }, { BOARMOVE, "BOARMOVE.82M" },
                   { BOARWNCE, "BOARWNCE.82M" }, { BONEATTK, "BONEATTK.82M" }, { BONEKILL, "BONEKILL.82M" }, { BONEMOVE, "BONEMOVE.82M" }, { BONEWNCE, "BONEWNCE.82M" },
                   { BUILDTWN, "BUILDTWN.82M" }, { CATSND00, "CATSND00.82M" }, { CATSND02, "CATSND02.82M" }, { CAVLATTK, "CAVLATTK.82M" }, { CAVLKILL, "CAVLKILL.82M" },
                   { CAVLMOVE, "CAVLMOVE.82M" }, { CAVLWNCE, "CAVLWNCE.82M" }, { CHAINLTE, "CHAINLTE.82M" }, { CNTRATTK, "CNTRATTK.82M" }, { CNTRKILL, "CNTRKILL.82M" },
                   { CNTRMOVE, "CNTRMOVE.82M" }, { CNTRSHOT, "CNTRSHOT.82M" }, { CNTRWNCE, "CNTRWNCE.82M" }, { COLDRAY, "COLDRAY.82M" },   { COLDRING, "COLDRING.82M" },
                   { CURE, "CURE.82M" },         { CURSE, "CURSE.82M" },       { CYCLATTK, "CYCLATTK.82M" }, { CYCLKILL, "CYCLKILL.82M" }, { CYCLMOVE, "CYCLMOVE.82M" },
                   { CYCLWNCE, "CYCLWNCE.82M" }, { DIGSOUND, "DIGSOUND.82M" }, { DIPMAGK, "DIPMAGK.82M" },   { DISRUPTR, "DISRUPTR.82M" }, { DRAWBRG, "DRAWBRG.82M" },
                   { DRGNATTK, "DRGNATTK.82M" }, { DRGNKILL, "DRGNKILL.82M" }, { DRGNMOVE, "DRGNMOVE.82M" }, { DRGNSLAY, "DRGNSLAY.82M" }, { DRGNWNCE, "DRGNWNCE.82M" },
                   { DRUIATTK, "DRUIATTK.82M" }, { DRUIKILL, "DRUIKILL.82M" }, { DRUIMOVE, "DRUIMOVE.82M" }, { DRUISHOT, "DRUISHOT.82M" }, { DRUIWNCE, "DRUIWNCE.82M" },
                   { DWRFATTK, "DWRFATTK.82M" }, { DWRFKILL, "DWRFKILL.82M" }, { DWRFMOVE, "DWRFMOVE.82M" }, { DWRFWNCE, "DWRFWNCE.82M" }, { EELMATTK, "EELMATTK.82M" },
                   { EELMKILL, "EELMKILL.82M" }, { EELMMOVE, "EELMMOVE.82M" }, { EELMWNCE, "EELMWNCE.82M" }, { ELF_ATTK, "ELF_ATTK.82M" }, { ELF_KILL, "ELF_KILL.82M" },
                   { ELF_MOVE, "ELF_MOVE.82M" }, { ELF_SHOT, "ELF_SHOT.82M" }, { ELF_WNCE, "ELF_WNCE.82M" }, { ERTHQUAK, "ERTHQUAK.82M" }, { EXPERNCE, "EXPERNCE.82M" },
                   { FELMATTK, "FELMATTK.82M" }, { FELMKILL, "FELMKILL.82M" }, { FELMMOVE, "FELMMOVE.82M" }, { FELMWNCE, "FELMWNCE.82M" }, { FIREBALL, "FIREBALL.82M" },
                   { GARGATTK, "GARGATTK.82M" }, { GARGKILL, "GARGKILL.82M" }, { GARGMOVE, "GARGMOVE.82M" }, { GARGWNCE, "GARGWNCE.82M" }, { GBLNATTK, "GBLNATTK.82M" },
                   { GBLNKILL, "GBLNKILL.82M" }, { GBLNMOVE, "GBLNMOVE.82M" }, { GBLNWNCE, "GBLNWNCE.82M" }, { GENIATTK, "GENIATTK.82M" }, { GENIKILL, "GENIKILL.82M" },
                   { GENIMOVE, "GENIMOVE.82M" }, { GENIWNCE, "GENIWNCE.82M" }, { GHSTATTK, "GHSTATTK.82M" }, { GHSTKILL, "GHSTKILL.82M" }, { GHSTMOVE, "GHSTMOVE.82M" },
                   { GHSTWNCE, "GHSTWNCE.82M" }, { GOLMATTK, "GOLMATTK.82M" }, { GOLMKILL, "GOLMKILL.82M" }, { GOLMMOVE, "GOLMMOVE.82M" }, { GOLMWNCE, "GOLMWNCE.82M" },
                   { GOODLUCK, "GOODLUCK.82M" }, { GOODMRLE, "GOODMRLE.82M" }, { GRIFATTK, "GRIFATTK.82M" }, { GRIFKILL, "GRIFKILL.82M" }, { GRIFMOVE, "GRIFMOVE.82M" },
                   { GRIFWNCE, "GRIFWNCE.82M" }, { H2MINE, "H2MINE.82M" },     { HALFATTK, "HALFATTK.82M" }, { HALFKILL, "HALFKILL.82M" }, { HALFMOVE, "HALFMOVE.82M" },
                   { HALFSHOT, "HALFSHOT.82M" }, { HALFWNCE, "HALFWNCE.82M" }, { HASTE, "HASTE.82M" },       { HYDRATTK, "HYDRATTK.82M" }, { HYDRKILL, "HYDRKILL.82M" },
                   { HYDRMOVE, "HYDRMOVE.82M" }, { HYDRWNCE, "HYDRWNCE.82M" }, { HYPNOTIZ, "HYPNOTIZ.82M" }, { KEEPSHOT, "KEEPSHOT.82M" }, { KILLFADE, "KILLFADE.82M" },
                   { LICHATTK, "LICHATTK.82M" }, { LICHEXPL, "LICHEXPL.82M" }, { LICHKILL, "LICHKILL.82M" }, { LICHMOVE, "LICHMOVE.82M" }, { LICHSHOT, "LICHSHOT.82M" },
                   { LICHWNCE, "LICHWNCE.82M" }, { LIGHTBLT, "LIGHTBLT.82M" }, { LOOP0000, "LOOP0000.82M" }, { LOOP0001, "LOOP0001.82M" }, { LOOP0002, "LOOP0002.82M" },
                   { LOOP0003, "LOOP0003.82M" }, { LOOP0004, "LOOP0004.82M" }, { LOOP0005, "LOOP0005.82M" }, { LOOP0006, "LOOP0006.82M" }, { LOOP0007, "LOOP0007.82M" },
                   { LOOP0008, "LOOP0008.82M" }, { LOOP0009, "LOOP0009.82M" }, { LOOP0010, "LOOP0010.82M" }, { LOOP0011, "LOOP0011.82M" }, { LOOP0012, "LOOP0012.82M" },
                   { LOOP0013, "LOOP0013.82M" }, { LOOP0014, "LOOP0014.82M" }, { LOOP0015, "LOOP0015.82M" }, { LOOP0016, "LOOP0016.82M" }, { LOOP0017, "LOOP0017.82M" },
                   { LOOP0018, "LOOP0018.82M" }, { LOOP0019, "LOOP0019.82M" }, { LOOP0020, "LOOP0020.82M" }, { LOOP0021, "LOOP0021.82M" }, { LOOP0022, "LOOP0022.82M" },
                   { LOOP0023, "LOOP0023.82M" }, { LOOP0024, "LOOP0024.82M" }, { LOOP0025, "LOOP0025.82M" }, { LOOP0026, "LOOP0026.82M" }, { LOOP0027, "LOOP0027.82M" },
                   { MAGCAROW, "MAGCAROW.82M" }, { MAGEATTK, "MAGEATTK.82M" }, { MAGEKILL, "MAGEKILL.82M" }, { MAGEMOVE, "MAGEMOVE.82M" }, { MAGESHOT, "MAGESHOT.82M" },
                   { MAGEWNCE, "MAGEWNCE.82M" }, { MASSBLES, "MASSBLES.82M" }, { MASSCURE, "MASSCURE.82M" }, { MASSCURS, "MASSCURS.82M" }, { MASSHAST, "MASSHAST.82M" },
                   { MASSSHIE, "MASSSHIE.82M" }, { MASSSLOW, "MASSSLOW.82M" }, { MEDSATTK, "MEDSATTK.82M" }, { MEDSKILL, "MEDSKILL.82M" }, { MEDSMOVE, "MEDSMOVE.82M" },
                   { MEDSWNCE, "MEDSWNCE.82M" }, { METEOR, "METEOR~1.82M" },   { MINOATTK, "MINOATTK.82M" }, { MINOKILL, "MINOKILL.82M" }, { MINOMOVE, "MINOMOVE.82M" },
                   { MINOWNCE, "MINOWNCE.82M" }, { MIRRORIM, "MIRRORIM.82M" }, { MNRDEATH, "MNRDEATH.82M" }, { MUMYATTK, "MUMYATTK.82M" }, { MUMYKILL, "MUMYKILL.82M" },
                   { MUMYMOVE, "MUMYMOVE.82M" }, { MUMYWNCE, "MUMYWNCE.82M" }, { NMADATTK, "NMADATTK.82M" }, { NMADKILL, "NMADKILL.82M" }, { NMADMOVE, "NMADMOVE.82M" },
                   { NMADWNCE, "NMADWNCE.82M" }, { NWHEROLV, "NWHEROLV.82M" }, { OGREATTK, "OGREATTK.82M" }, { OGREKILL, "OGREKILL.82M" }, { OGREMOVE, "OGREMOVE.82M" },
                   { OGREWNCE, "OGREWNCE.82M" }, { ORC_ATTK, "ORC_ATTK.82M" }, { ORC_KILL, "ORC_KILL.82M" }, { ORC_MOVE, "ORC_MOVE.82M" }, { ORC_SHOT, "ORC_SHOT.82M" },
                   { ORC_WNCE, "ORC_WNCE.82M" }, { PARALIZE, "PARALIZE.82M" }, { PHOEATTK, "PHOEATTK.82M" }, { PHOEKILL, "PHOEKILL.82M" }, { PHOEMOVE, "PHOEMOVE.82M" },
                   { PHOEWNCE, "PHOEWNCE.82M" }, { PICKUP01, "PICKUP01.82M" }, { PICKUP02, "PICKUP02.82M" }, { PICKUP03, "PICKUP03.82M" }, { PICKUP04, "PICKUP04.82M" },
                   { PICKUP05, "PICKUP05.82M" }, { PICKUP06, "PICKUP06.82M" }, { PICKUP07, "PICKUP07.82M" }, { PIKEATTK, "PIKEATTK.82M" }, { PIKEKILL, "PIKEKILL.82M" },
                   { PIKEMOVE, "PIKEMOVE.82M" }, { PIKEWNCE, "PIKEWNCE.82M" }, { PLDNATTK, "PLDNATTK.82M" }, { PLDNKILL, "PLDNKILL.82M" }, { PLDNMOVE, "PLDNMOVE.82M" },
                   { PLDNWNCE, "PLDNWNCE.82M" }, { PREBATTL, "PREBATTL.82M" }, { PROTECT, "PROTECT.82M" },   { PSNTATTK, "PSNTATTK.82M" }, { PSNTKILL, "PSNTKILL.82M" },
                   { PSNTMOVE, "PSNTMOVE.82M" }, { PSNTWNCE, "PSNTWNCE.82M" }, { RESURECT, "RESURECT.82M" }, { RESURTRU, "RESURTRU.82M" }, { ROC_ATTK, "ROC_ATTK.82M" },
                   { ROC_KILL, "ROC_KILL.82M" }, { ROC_MOVE, "ROC_MOVE.82M" }, { ROC_WNCE, "ROC_WNCE.82M" }, { ROGUATTK, "ROGUATTK.82M" }, { ROGUKILL, "ROGUKILL.82M" },
                   { ROGUMOVE, "ROGUMOVE.82M" }, { ROGUWNCE, "ROGUWNCE.82M" }, { RSBRYFZL, "RSBRYFZL.82M" }, { SHIELD, "SHIELD.82M" },     { SKELATTK, "SKELATTK.82M" },
                   { SKELKILL, "SKELKILL.82M" }, { SKELMOVE, "SKELMOVE.82M" }, { SKELWNCE, "SKELWNCE.82M" }, { SLOW, "SLOW.82M" },         { SPRTATTK, "SPRTATTK.82M" },
                   { SPRTKILL, "SPRTKILL.82M" }, { SPRTMOVE, "SPRTMOVE.82M" }, { SPRTWNCE, "SPRTWNCE.82M" }, { STELSKIN, "STELSKIN.82M" }, { STONESKI, "STONESKI.82M" },
                   { STONSKIN, "STONSKIN.82M" }, { STORM, "STORM.82M" },       { SUMNELM, "SUMNELM.82M" },   { SWDMATTK, "SWDMATTK.82M" }, { SWDMKILL, "SWDMKILL.82M" },
                   { SWDMMOVE, "SWDMMOVE.82M" }, { SWDMWNCE, "SWDMWNCE.82M" }, { TELEIN, "TELEIN.82M" },     { TELPTIN, "TELPTIN.82M" },   { TELPTOUT, "TELPTOUT.82M" },
                   { TITNATTK, "TITNATTK.82M" }, { TITNKILL, "TITNKILL.82M" }, { TITNMOVE, "TITNMOVE.82M" }, { TITNSHOT, "TITNSHOT.82M" }, { TITNWNCE, "TITNWNCE.82M" },
                   { TREASURE, "TREASURE.82M" }, { TRLLATTK, "TRLLATTK.82M" }, { TRLLKILL, "TRLLKILL.82M" }, { TRLLMOVE, "TRLLMOVE.82M" }, { TRLLSHOT, "TRLLSHOT.82M" },
                   { TRLLWNCE, "TRLLWNCE.82M" }, { UNICATTK, "UNICATTK.82M" }, { UNICKILL, "UNICKILL.82M" }, { UNICMOVE, "UNICMOVE.82M" }, { UNICWNCE, "UNICWNCE.82M" },
                   { VAMPATTK, "VAMPATTK.82M" }, { VAMPEXT1, "VAMPEXT1.82M" }, { VAMPEXT2, "VAMPEXT2.82M" }, { VAMPKILL, "VAMPKILL.82M" }, { VAMPMOVE, "VAMPMOVE.82M" },
                   { VAMPWNCE, "VAMPWNCE.82M" }, { WELMATTK, "WELMATTK.82M" }, { WELMKILL, "WELMKILL.82M" }, { WELMMOVE, "WELMMOVE.82M" }, { WELMWNCE, "WELMWNCE.82M" },
                   { WOLFATTK, "WOLFATTK.82M" }, { WOLFKILL, "WOLFKILL.82M" }, { WOLFMOVE, "WOLFMOVE.82M" }, { WOLFWNCE, "WOLFWNCE.82M" }, { WSND00, "WSND00.82M" },
                   { WSND01, "WSND01.82M" },     { WSND02, "WSND02.82M" },     { WSND03, "WSND03.82M" },     { WSND04, "WSND04.82M" },     { WSND05, "WSND05.82M" },
                   { WSND06, "WSND06.82M" },     { WSND10, "WSND10.82M" },     { WSND11, "WSND11.82M" },     { WSND12, "WSND12.82M" },     { WSND13, "WSND13.82M" },
                   { WSND14, "WSND14.82M" },     { WSND15, "WSND15.82M" },     { WSND16, "WSND16.82M" },     { WSND20, "WSND20.82M" },     { WSND21, "WSND21.82M" },
                   { WSND22, "WSND22.82M" },     { WSND23, "WSND23.82M" },     { WSND24, "WSND24.82M" },     { WSND25, "WSND25.82M" },     { WSND26, "WSND26.82M" },
                   { ZOMBATTK, "ZOMBATTK.82M" }, { ZOMBKILL, "ZOMBKILL.82M" }, { ZOMBMOVE, "ZOMBMOVE.82M" }, { ZOMBWNCE, "ZOMBWNCE.82M" }, { UNKNOWN, "UNKNOWN" } };
}

const char * M82::GetString( int m82 )
{
    return AELMATTK <= m82 && UNKNOWN > m82 ? m82map[m82].string : m82map[UNKNOWN].string;
}

int M82::FromSpell( const int spellID )
{
    switch ( spellID ) {
    case Spell::FIREBALL:
    case Spell::FIREBLAST:
        return FIREBALL;
    case Spell::LIGHTNINGBOLT:
        return LIGHTBLT;
    case Spell::CHAINLIGHTNING:
        return CHAINLTE;
    case Spell::TELEPORT:
        return TELEIN;
    case Spell::CURE:
        return CURE;
    case Spell::MASSCURE:
        return MASSCURE;
    case Spell::RESURRECT:
        return RESURECT;
    case Spell::RESURRECTTRUE:
        return RESURTRU;
    case Spell::HASTE:
        return HASTE;
    case Spell::MASSHASTE:
        return MASSHAST;
    case Spell::SLOW:
        return SLOW;
    case Spell::MASSSLOW:
        return MASSSLOW;
    case Spell::BLIND:
        return BLIND;
    case Spell::BLESS:
        return BLESS;
    case Spell::MASSBLESS:
        return MASSBLES;
    case Spell::STONESKIN:
        return STONSKIN;
    case Spell::STEELSKIN:
        return STELSKIN;
    case Spell::CURSE:
        return CURSE;
    case Spell::MASSCURSE:
        return MASSCURS;
    case Spell::ANTIMAGIC:
        return ANTIMAGK;
    case Spell::DISPEL:
    case Spell::MASSDISPEL:
        return DIPMAGK;
    case Spell::ARROW:
        return MAGCAROW;
    case Spell::BERSERKER:
        return BERZERK;
    case Spell::ARMAGEDDON:
        return ARMGEDN;
    case Spell::ELEMENTALSTORM:
        return STORM;
    case Spell::METEORSHOWER:
        return METEOR;
    case Spell::PARALYZE:
        return PARALIZE;
    case Spell::HYPNOTIZE:
        return HYPNOTIZ;
    case Spell::COLDRAY:
        return COLDRAY;
    case Spell::COLDRING:
        return COLDRING;
    case Spell::DISRUPTINGRAY:
        return DISRUPTR;
    case Spell::DEATHRIPPLE:
    case Spell::DEATHWAVE:
        return MNRDEATH;
    case Spell::DRAGONSLAYER:
        return DRGNSLAY;
    case Spell::BLOODLUST:
        return BLOODLUS;
    case Spell::ANIMATEDEAD:
        return RESURECT;
    case Spell::MIRRORIMAGE:
        return MIRRORIM;
    case Spell::SHIELD:
        return SHIELD;
    case Spell::MASSSHIELD:
        return MASSSHIE;
    case Spell::SUMMONEELEMENT:
    case Spell::SUMMONAELEMENT:
    case Spell::SUMMONFELEMENT:
    case Spell::SUMMONWELEMENT:
        return SUMNELM;
    case Spell::EARTHQUAKE:
        return ERTHQUAK;
    case Spell::HAUNT:
        return H2MINE;
    case Spell::PETRIFY:
        return PARALIZE;
    default:
        break;
    }

    return UNKNOWN;
}

M82::SoundType M82::getAdventureMapTileSound( const Maps::Tile & tile )
{
    if ( tile.isStream() ) {
        return LOOP0014;
    }

    switch ( tile.getMainObjectType( false ) ) {
    case MP2::OBJ_BUOY:
        return LOOP0000;
    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_DERELICT_SHIP:
        return LOOP0001;
    case MP2::OBJ_COAST:
        return LOOP0002;
    case MP2::OBJ_ORACLE:
        return LOOP0003;
    case MP2::OBJ_STONE_LITHS:
        return LOOP0004;
    case MP2::OBJ_VOLCANO:
        switch ( tile.getMainObjectPart().icnType ) {
        // Tile with volcanic steam only
        case MP2::OBJ_ICN_TYPE_UNKNOWN:
            return UNKNOWN;
        // Small volcanoes
        case MP2::OBJ_ICN_TYPE_OBJNLAVA:
            return LOOP0005;
        default:
            break;
        }
        // Other volcanoes
        return LOOP0027;
    case MP2::OBJ_LAVAPOOL:
        return LOOP0006;
    case MP2::OBJ_ALCHEMIST_LAB:
        return LOOP0007;
    case MP2::OBJ_WATER_WHEEL:
        return LOOP0009;
    case MP2::OBJ_CAMPFIRE:
        return LOOP0010;
    case MP2::OBJ_WINDMILL:
        return LOOP0011;
    case MP2::OBJ_ARTESIAN_SPRING:
    case MP2::OBJ_FOUNTAIN:
        return LOOP0012;
    case MP2::OBJ_WATER_LAKE:
    case MP2::OBJ_WATERING_HOLE:
        return LOOP0013;
    case MP2::OBJ_MINE:
        return LOOP0015;
    case MP2::OBJ_SAWMILL:
        return LOOP0016;
    case MP2::OBJ_DAEMON_CAVE:
        return LOOP0017;
    case MP2::OBJ_SHRINE_FIRST_CIRCLE:
    case MP2::OBJ_SHRINE_SECOND_CIRCLE:
    case MP2::OBJ_SHRINE_THIRD_CIRCLE:
        return LOOP0018;
    case MP2::OBJ_ROCK:
        // This sound should only be played for a specific sprite belonging to Rock
        if ( tile.containsSprite( MP2::OBJ_ICN_TYPE_OBJNWATR, 183 ) ) {
            return LOOP0019;
        }
        break;
    case MP2::OBJ_TAR_PIT:
        return LOOP0021;
    case MP2::OBJ_TRADING_POST:
        return LOOP0022;
    case MP2::OBJ_RUINS:
        return LOOP0024;
    case MP2::OBJ_PEASANT_HUT:
    case MP2::OBJ_DWARF_COTTAGE:
    case MP2::OBJ_ARCHER_HOUSE:
        return LOOP0025;
    case MP2::OBJ_ABANDONED_MINE:
    case MP2::OBJ_FREEMANS_FOUNDRY:
        return LOOP0026;
    default:
        break;
    }

    return UNKNOWN;
}
