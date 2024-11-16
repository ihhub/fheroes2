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

int M82::getFromString(const std::string& id){
    if(id == "AELMATTK"){ return M82::AELMATTK; }else
    if(id == "AELMKILL"){ return M82::AELMKILL; }else
    if(id == "AELMMOVE"){ return M82::AELMMOVE; }else
    if(id == "AELMWNCE"){ return M82::AELMWNCE; }else
    if(id == "ANTIMAGK"){ return M82::ANTIMAGK; }else
    if(id == "ARCHATTK"){ return M82::ARCHATTK; }else
    if(id == "ARCHKILL"){ return M82::ARCHKILL; }else
    if(id == "ARCHMOVE"){ return M82::ARCHMOVE; }else
    if(id == "ARCHSHOT"){ return M82::ARCHSHOT; }else
    if(id == "ARCHWNCE"){ return M82::ARCHWNCE; }else
    if(id == "ARMGEDN"){ return M82::ARMGEDN; }else
    if(id == "BADLUCK"){ return M82::BADLUCK; }else
    if(id == "BADMRLE"){ return M82::BADMRLE; }else
    if(id == "BERZERK"){ return M82::BERZERK; }else
    if(id == "BLESS"){ return M82::BLESS; }else
    if(id == "BLIND"){ return M82::BLIND; }else
    if(id == "BLOODLUS"){ return M82::BLOODLUS; }else
    if(id == "BOARATTK"){ return M82::BOARATTK; }else
    if(id == "BOARKILL"){ return M82::BOARKILL; }else
    if(id == "BOARMOVE"){ return M82::BOARMOVE; }else
    if(id == "BOARWNCE"){ return M82::BOARWNCE; }else
    if(id == "BONEATTK"){ return M82::BONEATTK; }else
    if(id == "BONEKILL"){ return M82::BONEKILL; }else
    if(id == "BONEMOVE"){ return M82::BONEMOVE; }else
    if(id == "BONEWNCE"){ return M82::BONEWNCE; }else
    if(id == "BUILDTWN"){ return M82::BUILDTWN; }else
    if(id == "CATSND00"){ return M82::CATSND00; }else
    if(id == "CATSND02"){ return M82::CATSND02; }else
    if(id == "CAVLATTK"){ return M82::CAVLATTK; }else
    if(id == "CAVLKILL"){ return M82::CAVLKILL; }else
    if(id == "CAVLMOVE"){ return M82::CAVLMOVE; }else
    if(id == "CAVLWNCE"){ return M82::CAVLWNCE; }else
    if(id == "CHAINLTE"){ return M82::CHAINLTE; }else
    if(id == "CNTRATTK"){ return M82::CNTRATTK; }else
    if(id == "CNTRKILL"){ return M82::CNTRKILL; }else
    if(id == "CNTRMOVE"){ return M82::CNTRMOVE; }else
    if(id == "CNTRSHOT"){ return M82::CNTRSHOT; }else
    if(id == "CNTRWNCE"){ return M82::CNTRWNCE; }else
    if(id == "COLDRAY"){ return M82::COLDRAY; }else
    if(id == "COLDRING"){ return M82::COLDRING; }else
    if(id == "CURE"){ return M82::CURE; }else
    if(id == "CURSE"){ return M82::CURSE; }else
    if(id == "CYCLATTK"){ return M82::CYCLATTK; }else
    if(id == "CYCLKILL"){ return M82::CYCLKILL; }else
    if(id == "CYCLMOVE"){ return M82::CYCLMOVE; }else
    if(id == "CYCLWNCE"){ return M82::CYCLWNCE; }else
    if(id == "DIGSOUND"){ return M82::DIGSOUND; }else
    if(id == "DIPMAGK"){ return M82::DIPMAGK; }else
    if(id == "DISRUPTR"){ return M82::DISRUPTR; }else
    if(id == "DRAWBRG"){ return M82::DRAWBRG; }else
    if(id == "DRGNATTK"){ return M82::DRGNATTK; }else
    if(id == "DRGNKILL"){ return M82::DRGNKILL; }else
    if(id == "DRGNMOVE"){ return M82::DRGNMOVE; }else
    if(id == "DRGNSLAY"){ return M82::DRGNSLAY; }else
    if(id == "DRGNWNCE"){ return M82::DRGNWNCE; }else
    if(id == "DRUIATTK"){ return M82::DRUIATTK; }else
    if(id == "DRUIKILL"){ return M82::DRUIKILL; }else
    if(id == "DRUIMOVE"){ return M82::DRUIMOVE; }else
    if(id == "DRUISHOT"){ return M82::DRUISHOT; }else
    if(id == "DRUIWNCE"){ return M82::DRUIWNCE; }else
    if(id == "DWRFATTK"){ return M82::DWRFATTK; }else
    if(id == "DWRFKILL"){ return M82::DWRFKILL; }else
    if(id == "DWRFMOVE"){ return M82::DWRFMOVE; }else
    if(id == "DWRFWNCE"){ return M82::DWRFWNCE; }else
    if(id == "EELMATTK"){ return M82::EELMATTK; }else
    if(id == "EELMKILL"){ return M82::EELMKILL; }else
    if(id == "EELMMOVE"){ return M82::EELMMOVE; }else
    if(id == "EELMWNCE"){ return M82::EELMWNCE; }else
    if(id == "ELF_ATTK"){ return M82::ELF_ATTK; }else
    if(id == "ELF_KILL"){ return M82::ELF_KILL; }else
    if(id == "ELF_MOVE"){ return M82::ELF_MOVE; }else
    if(id == "ELF_SHOT"){ return M82::ELF_SHOT; }else
    if(id == "ELF_WNCE"){ return M82::ELF_WNCE; }else
    if(id == "ERTHQUAK"){ return M82::ERTHQUAK; }else
    if(id == "EXPERNCE"){ return M82::EXPERNCE; }else
    if(id == "FELMATTK"){ return M82::FELMATTK; }else
    if(id == "FELMKILL"){ return M82::FELMKILL; }else
    if(id == "FELMMOVE"){ return M82::FELMMOVE; }else
    if(id == "FELMWNCE"){ return M82::FELMWNCE; }else
    if(id == "FIREBALL"){ return M82::FIREBALL; }else
    if(id == "GARGATTK"){ return M82::GARGATTK; }else
    if(id == "GARGKILL"){ return M82::GARGKILL; }else
    if(id == "GARGMOVE"){ return M82::GARGMOVE; }else
    if(id == "GARGWNCE"){ return M82::GARGWNCE; }else
    if(id == "GBLNATTK"){ return M82::GBLNATTK; }else
    if(id == "GBLNKILL"){ return M82::GBLNKILL; }else
    if(id == "GBLNMOVE"){ return M82::GBLNMOVE; }else
    if(id == "GBLNWNCE"){ return M82::GBLNWNCE; }else
    if(id == "GENIATTK"){ return M82::GENIATTK; }else
    if(id == "GENIKILL"){ return M82::GENIKILL; }else
    if(id == "GENIMOVE"){ return M82::GENIMOVE; }else
    if(id == "GENIWNCE"){ return M82::GENIWNCE; }else
    if(id == "GHSTATTK"){ return M82::GHSTATTK; }else
    if(id == "GHSTKILL"){ return M82::GHSTKILL; }else
    if(id == "GHSTMOVE"){ return M82::GHSTMOVE; }else
    if(id == "GHSTWNCE"){ return M82::GHSTWNCE; }else
    if(id == "GOLMATTK"){ return M82::GOLMATTK; }else
    if(id == "GOLMKILL"){ return M82::GOLMKILL; }else
    if(id == "GOLMMOVE"){ return M82::GOLMMOVE; }else
    if(id == "GOLMWNCE"){ return M82::GOLMWNCE; }else
    if(id == "GOODLUCK"){ return M82::GOODLUCK; }else
    if(id == "GOODMRLE"){ return M82::GOODMRLE; }else
    if(id == "GRIFATTK"){ return M82::GRIFATTK; }else
    if(id == "GRIFKILL"){ return M82::GRIFKILL; }else
    if(id == "GRIFMOVE"){ return M82::GRIFMOVE; }else
    if(id == "GRIFWNCE"){ return M82::GRIFWNCE; }else
    if(id == "H2MINE"){ return M82::H2MINE; }else
    if(id == "HALFATTK"){ return M82::HALFATTK; }else
    if(id == "HALFKILL"){ return M82::HALFKILL; }else
    if(id == "HALFMOVE"){ return M82::HALFMOVE; }else
    if(id == "HALFSHOT"){ return M82::HALFSHOT; }else
    if(id == "HALFWNCE"){ return M82::HALFWNCE; }else
    if(id == "HASTE"){ return M82::HASTE; }else
    if(id == "HYDRATTK"){ return M82::HYDRATTK; }else
    if(id == "HYDRKILL"){ return M82::HYDRKILL; }else
    if(id == "HYDRMOVE"){ return M82::HYDRMOVE; }else
    if(id == "HYDRWNCE"){ return M82::HYDRWNCE; }else
    if(id == "HYPNOTIZ"){ return M82::HYPNOTIZ; }else
    if(id == "KEEPSHOT"){ return M82::KEEPSHOT; }else
    if(id == "KILLFADE"){ return M82::KILLFADE; }else
    if(id == "LICHATTK"){ return M82::LICHATTK; }else
    if(id == "LICHEXPL"){ return M82::LICHEXPL; }else
    if(id == "LICHKILL"){ return M82::LICHKILL; }else
    if(id == "LICHMOVE"){ return M82::LICHMOVE; }else
    if(id == "LICHSHOT"){ return M82::LICHSHOT; }else
    if(id == "LICHWNCE"){ return M82::LICHWNCE; }else
    if(id == "LIGHTBLT"){ return M82::LIGHTBLT; }else
    if(id == "LOOP0000"){ return M82::LOOP0000; }else
    if(id == "LOOP0001"){ return M82::LOOP0001; }else
    if(id == "LOOP0002"){ return M82::LOOP0002; }else
    if(id == "LOOP0003"){ return M82::LOOP0003; }else
    if(id == "LOOP0004"){ return M82::LOOP0004; }else
    if(id == "LOOP0005"){ return M82::LOOP0005; }else
    if(id == "LOOP0006"){ return M82::LOOP0006; }else
    if(id == "LOOP0007"){ return M82::LOOP0007; }else
    if(id == "LOOP0008"){ return M82::LOOP0008; }else
    if(id == "LOOP0009"){ return M82::LOOP0009; }else
    if(id == "LOOP0010"){ return M82::LOOP0010; }else
    if(id == "LOOP0011"){ return M82::LOOP0011; }else
    if(id == "LOOP0012"){ return M82::LOOP0012; }else
    if(id == "LOOP0013"){ return M82::LOOP0013; }else
    if(id == "LOOP0014"){ return M82::LOOP0014; }else
    if(id == "LOOP0015"){ return M82::LOOP0015; }else
    if(id == "LOOP0016"){ return M82::LOOP0016; }else
    if(id == "LOOP0017"){ return M82::LOOP0017; }else
    if(id == "LOOP0018"){ return M82::LOOP0018; }else
    if(id == "LOOP0019"){ return M82::LOOP0019; }else
    if(id == "LOOP0020"){ return M82::LOOP0020; }else
    if(id == "LOOP0021"){ return M82::LOOP0021; }else
    if(id == "LOOP0022"){ return M82::LOOP0022; }else
    if(id == "LOOP0023"){ return M82::LOOP0023; }else
    if(id == "LOOP0024"){ return M82::LOOP0024; }else
    if(id == "LOOP0025"){ return M82::LOOP0025; }else
    if(id == "LOOP0026"){ return M82::LOOP0026; }else
    if(id == "LOOP0027"){ return M82::LOOP0027; }else
    if(id == "MAGCAROW"){ return M82::MAGCAROW; }else
    if(id == "MAGEATTK"){ return M82::MAGEATTK; }else
    if(id == "MAGEKILL"){ return M82::MAGEKILL; }else
    if(id == "MAGEMOVE"){ return M82::MAGEMOVE; }else
    if(id == "MAGESHOT"){ return M82::MAGESHOT; }else
    if(id == "MAGEWNCE"){ return M82::MAGEWNCE; }else
    if(id == "MASSBLES"){ return M82::MASSBLES; }else
    if(id == "MASSCURE"){ return M82::MASSCURE; }else
    if(id == "MASSCURS"){ return M82::MASSCURS; }else
    if(id == "MASSHAST"){ return M82::MASSHAST; }else
    if(id == "MASSSHIE"){ return M82::MASSSHIE; }else
    if(id == "MASSSLOW"){ return M82::MASSSLOW; }else
    if(id == "MEDSATTK"){ return M82::MEDSATTK; }else
    if(id == "MEDSKILL"){ return M82::MEDSKILL; }else
    if(id == "MEDSMOVE"){ return M82::MEDSMOVE; }else
    if(id == "MEDSWNCE"){ return M82::MEDSWNCE; }else
    if(id == "METEOR"){ return M82::METEOR; }else
    if(id == "MINOATTK"){ return M82::MINOATTK; }else
    if(id == "MINOKILL"){ return M82::MINOKILL; }else
    if(id == "MINOMOVE"){ return M82::MINOMOVE; }else
    if(id == "MINOWNCE"){ return M82::MINOWNCE; }else
    if(id == "MIRRORIM"){ return M82::MIRRORIM; }else
    if(id == "MNRDEATH"){ return M82::MNRDEATH; }else
    if(id == "MUMYATTK"){ return M82::MUMYATTK; }else
    if(id == "MUMYKILL"){ return M82::MUMYKILL; }else
    if(id == "MUMYMOVE"){ return M82::MUMYMOVE; }else
    if(id == "MUMYWNCE"){ return M82::MUMYWNCE; }else
    if(id == "NMADATTK"){ return M82::NMADATTK; }else
    if(id == "NMADKILL"){ return M82::NMADKILL; }else
    if(id == "NMADMOVE"){ return M82::NMADMOVE; }else
    if(id == "NMADWNCE"){ return M82::NMADWNCE; }else
    if(id == "NWHEROLV"){ return M82::NWHEROLV; }else
    if(id == "OGREATTK"){ return M82::OGREATTK; }else
    if(id == "OGREKILL"){ return M82::OGREKILL; }else
    if(id == "OGREMOVE"){ return M82::OGREMOVE; }else
    if(id == "OGREWNCE"){ return M82::OGREWNCE; }else
    if(id == "ORC_ATTK"){ return M82::ORC_ATTK; }else
    if(id == "ORC_KILL"){ return M82::ORC_KILL; }else
    if(id == "ORC_MOVE"){ return M82::ORC_MOVE; }else
    if(id == "ORC_SHOT"){ return M82::ORC_SHOT; }else
    if(id == "ORC_WNCE"){ return M82::ORC_WNCE; }else
    if(id == "PARALIZE"){ return M82::PARALIZE; }else
    if(id == "PHOEATTK"){ return M82::PHOEATTK; }else
    if(id == "PHOEKILL"){ return M82::PHOEKILL; }else
    if(id == "PHOEMOVE"){ return M82::PHOEMOVE; }else
    if(id == "PHOEWNCE"){ return M82::PHOEWNCE; }else
    if(id == "PICKUP01"){ return M82::PICKUP01; }else
    if(id == "PICKUP02"){ return M82::PICKUP02; }else
    if(id == "PICKUP03"){ return M82::PICKUP03; }else
    if(id == "PICKUP04"){ return M82::PICKUP04; }else
    if(id == "PICKUP05"){ return M82::PICKUP05; }else
    if(id == "PICKUP06"){ return M82::PICKUP06; }else
    if(id == "PICKUP07"){ return M82::PICKUP07; }else
    if(id == "PIKEATTK"){ return M82::PIKEATTK; }else
    if(id == "PIKEKILL"){ return M82::PIKEKILL; }else
    if(id == "PIKEMOVE"){ return M82::PIKEMOVE; }else
    if(id == "PIKEWNCE"){ return M82::PIKEWNCE; }else
    if(id == "PLDNATTK"){ return M82::PLDNATTK; }else
    if(id == "PLDNKILL"){ return M82::PLDNKILL; }else
    if(id == "PLDNMOVE"){ return M82::PLDNMOVE; }else
    if(id == "PLDNWNCE"){ return M82::PLDNWNCE; }else
    if(id == "PREBATTL"){ return M82::PREBATTL; }else
    if(id == "PROTECT"){ return M82::PROTECT; }else
    if(id == "PSNTATTK"){ return M82::PSNTATTK; }else
    if(id == "PSNTKILL"){ return M82::PSNTKILL; }else
    if(id == "PSNTMOVE"){ return M82::PSNTMOVE; }else
    if(id == "PSNTWNCE"){ return M82::PSNTWNCE; }else
    if(id == "RESURECT"){ return M82::RESURECT; }else
    if(id == "RESURTRU"){ return M82::RESURTRU; }else
    if(id == "ROC_ATTK"){ return M82::ROC_ATTK; }else
    if(id == "ROC_KILL"){ return M82::ROC_KILL; }else
    if(id == "ROC_MOVE"){ return M82::ROC_MOVE; }else
    if(id == "ROC_WNCE"){ return M82::ROC_WNCE; }else
    if(id == "ROGUATTK"){ return M82::ROGUATTK; }else
    if(id == "ROGUKILL"){ return M82::ROGUKILL; }else
    if(id == "ROGUMOVE"){ return M82::ROGUMOVE; }else
    if(id == "ROGUWNCE"){ return M82::ROGUWNCE; }else
    if(id == "RSBRYFZL"){ return M82::RSBRYFZL; }else
    if(id == "SHIELD"){ return M82::SHIELD; }else
    if(id == "SKELATTK"){ return M82::SKELATTK; }else
    if(id == "SKELKILL"){ return M82::SKELKILL; }else
    if(id == "SKELMOVE"){ return M82::SKELMOVE; }else
    if(id == "SKELWNCE"){ return M82::SKELWNCE; }else
    if(id == "SLOW"){ return M82::SLOW; }else
    if(id == "SPRTATTK"){ return M82::SPRTATTK; }else
    if(id == "SPRTKILL"){ return M82::SPRTKILL; }else
    if(id == "SPRTMOVE"){ return M82::SPRTMOVE; }else
    if(id == "SPRTWNCE"){ return M82::SPRTWNCE; }else
    if(id == "STELSKIN"){ return M82::STELSKIN; }else
    if(id == "STONESKI"){ return M82::STONESKI; }else
    if(id == "STONSKIN"){ return M82::STONSKIN; }else
    if(id == "STORM"){ return M82::STORM; }else
    if(id == "SUMNELM"){ return M82::SUMNELM; }else
    if(id == "SWDMATTK"){ return M82::SWDMATTK; }else
    if(id == "SWDMKILL"){ return M82::SWDMKILL; }else
    if(id == "SWDMMOVE"){ return M82::SWDMMOVE; }else
    if(id == "SWDMWNCE"){ return M82::SWDMWNCE; }else
    if(id == "TELEIN"){ return M82::TELEIN; }else
    if(id == "TELPTIN"){ return M82::TELPTIN; }else
    if(id == "TELPTOUT"){ return M82::TELPTOUT; }else
    if(id == "TITNATTK"){ return M82::TITNATTK; }else
    if(id == "TITNKILL"){ return M82::TITNKILL; }else
    if(id == "TITNMOVE"){ return M82::TITNMOVE; }else
    if(id == "TITNSHOT"){ return M82::TITNSHOT; }else
    if(id == "TITNWNCE"){ return M82::TITNWNCE; }else
    if(id == "TREASURE"){ return M82::TREASURE; }else
    if(id == "TRLLATTK"){ return M82::TRLLATTK; }else
    if(id == "TRLLKILL"){ return M82::TRLLKILL; }else
    if(id == "TRLLMOVE"){ return M82::TRLLMOVE; }else
    if(id == "TRLLSHOT"){ return M82::TRLLSHOT; }else
    if(id == "TRLLWNCE"){ return M82::TRLLWNCE; }else
    if(id == "UNICATTK"){ return M82::UNICATTK; }else
    if(id == "UNICKILL"){ return M82::UNICKILL; }else
    if(id == "UNICMOVE"){ return M82::UNICMOVE; }else
    if(id == "UNICWNCE"){ return M82::UNICWNCE; }else
    if(id == "VAMPATTK"){ return M82::VAMPATTK; }else
    if(id == "VAMPEXT1"){ return M82::VAMPEXT1; }else
    if(id == "VAMPEXT2"){ return M82::VAMPEXT2; }else
    if(id == "VAMPKILL"){ return M82::VAMPKILL; }else
    if(id == "VAMPMOVE"){ return M82::VAMPMOVE; }else
    if(id == "VAMPWNCE"){ return M82::VAMPWNCE; }else
    if(id == "WELMATTK"){ return M82::WELMATTK; }else
    if(id == "WELMKILL"){ return M82::WELMKILL; }else
    if(id == "WELMMOVE"){ return M82::WELMMOVE; }else
    if(id == "WELMWNCE"){ return M82::WELMWNCE; }else
    if(id == "WOLFATTK"){ return M82::WOLFATTK; }else
    if(id == "WOLFKILL"){ return M82::WOLFKILL; }else
    if(id == "WOLFMOVE"){ return M82::WOLFMOVE; }else
    if(id == "WOLFWNCE"){ return M82::WOLFWNCE; }else
    if(id == "WSND00"){ return M82::WSND00; }else
    if(id == "WSND01"){ return M82::WSND01; }else
    if(id == "WSND02"){ return M82::WSND02; }else
    if(id == "WSND03"){ return M82::WSND03; }else
    if(id == "WSND04"){ return M82::WSND04; }else
    if(id == "WSND05"){ return M82::WSND05; }else
    if(id == "WSND06"){ return M82::WSND06; }else
    if(id == "WSND10"){ return M82::WSND10; }else
    if(id == "WSND11"){ return M82::WSND11; }else
    if(id == "WSND12"){ return M82::WSND12; }else
    if(id == "WSND13"){ return M82::WSND13; }else
    if(id == "WSND14"){ return M82::WSND14; }else
    if(id == "WSND15"){ return M82::WSND15; }else
    if(id == "WSND16"){ return M82::WSND16; }else
    if(id == "WSND20"){ return M82::WSND20; }else
    if(id == "WSND21"){ return M82::WSND21; }else
    if(id == "WSND22"){ return M82::WSND22; }else
    if(id == "WSND23"){ return M82::WSND23; }else
    if(id == "WSND24"){ return M82::WSND24; }else
    if(id == "WSND25"){ return M82::WSND25; }else
    if(id == "WSND26"){ return M82::WSND26; }else
    if(id == "ZOMBATTK"){ return M82::ZOMBATTK; }else
    if(id == "ZOMBKILL"){ return M82::ZOMBKILL; }else
    if(id == "ZOMBMOVE"){ return M82::ZOMBMOVE; }else
    if(id == "ZOMBWNCE"){ return M82::ZOMBWNCE; }
    return M82::UNKNOWN;
}
