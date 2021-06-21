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

#include "monster_info.h"
#include "icn.h"
#include "m82.h"
#include "monster.h"
#include "race.h"
#include "speed.h"
#include "spell.h"

#include <cassert>
#include <sstream>

namespace
{
    std::vector<fheroes2::MonsterData> monsterData;

    void populateMonsterData()
    {
        const int monsterIcnIds[Monster::MONSTER_COUNT]
            = { ICN::UNKNOWN,  ICN::PEASANT,  ICN::ARCHER,   ICN::ARCHER2,  ICN::PIKEMAN,  ICN::PIKEMAN2, ICN::SWORDSMN, ICN::SWORDSM2, ICN::CAVALRYR,
                ICN::CAVALRYB, ICN::PALADIN,  ICN::PALADIN2, ICN::GOBLIN,   ICN::ORC,      ICN::ORC2,     ICN::WOLF,     ICN::OGRE,     ICN::OGRE2,
                ICN::TROLL,    ICN::TROLL2,   ICN::CYCLOPS,  ICN::SPRITE,   ICN::DWARF,    ICN::DWARF2,   ICN::ELF,      ICN::ELF2,     ICN::DRUID,
                ICN::DRUID2,   ICN::UNICORN,  ICN::PHOENIX,  ICN::CENTAUR,  ICN::GARGOYLE, ICN::GRIFFIN,  ICN::MINOTAUR, ICN::MINOTAU2, ICN::HYDRA,
                ICN::DRAGGREE, ICN::DRAGRED,  ICN::DRAGBLAK, ICN::HALFLING, ICN::BOAR,     ICN::GOLEM,    ICN::GOLEM2,   ICN::ROC,      ICN::MAGE1,
                ICN::MAGE2,    ICN::TITANBLU, ICN::TITANBLA, ICN::SKELETON, ICN::ZOMBIE,   ICN::ZOMBIE2,  ICN::MUMMYW,   ICN::MUMMY2,   ICN::VAMPIRE,
                ICN::VAMPIRE2, ICN::LICH,     ICN::LICH2,    ICN::DRAGBONE, ICN::ROGUE,    ICN::NOMAD,    ICN::GHOST,    ICN::GENIE,    ICN::MEDUSA,
                ICN::EELEM,    ICN::AELEM,    ICN::FELEM,    ICN::WELEM,    ICN::UNKNOWN,  ICN::UNKNOWN,  ICN::UNKNOWN,  ICN::UNKNOWN,  ICN::UNKNOWN };

        const char * binFileName[Monster::MONSTER_COUNT]
            = { "UNKNOWN",      "PEAS_FRM.BIN", "ARCHRFRM.BIN", "ARCHRFRM.BIN", "PIKMNFRM.BIN", "PIKMNFRM.BIN", "SWRDSFRM.BIN", "SWRDSFRM.BIN", "CVLRYFRM.BIN",
                "CVLR2FRM.BIN", "PALADFRM.BIN", "PALADFRM.BIN", "GOBLNFRM.BIN", "ORC__FRM.BIN", "ORC__FRM.BIN", "WOLF_FRM.BIN", "OGRE_FRM.BIN", "OGRE_FRM.BIN",
                "TROLLFRM.BIN", "TROLLFRM.BIN", "CYCLOFRM.BIN", "SPRITFRM.BIN", "DWARFFRM.BIN", "DWARFFRM.BIN", "ELF__FRM.BIN", "ELF__FRM.BIN", "DRUIDFRM.BIN",
                "DRUIDFRM.BIN", "UNICOFRM.BIN", "PHOENFRM.BIN", "CENTRFRM.BIN", "GARGLFRM.BIN", "GRIFFFRM.BIN", "MINOTFRM.BIN", "MINOTFRM.BIN", "HYDRAFRM.BIN",
                "DRAGGFRM.BIN", "DRAGRFRM.BIN", "DRAGBFRM.BIN", "HALFLFRM.BIN", "BOAR_FRM.BIN", "GOLEMFRM.BIN", "GOLEMFRM.BIN", "ROC__FRM.BIN", "MAGE1FRM.BIN",
                "MAGE1FRM.BIN", "TITANFRM.BIN", "TITA2FRM.BIN", "SKEL_FRM.BIN", "ZOMB_FRM.BIN", "ZOMB_FRM.BIN", "MUMMYFRM.BIN", "MUMMYFRM.BIN", "VAMPIFRM.BIN",
                "VAMPIFRM.BIN", "LICH_FRM.BIN", "LICH_FRM.BIN", "DRABNFRM.BIN", "ROGUEFRM.BIN", "NOMADFRM.BIN", "GHOSTFRM.BIN", "GENIEFRM.BIN", "MEDUSFRM.BIN",
                "FELEMFRM.BIN", "FELEMFRM.BIN", "FELEMFRM.BIN", "FELEMFRM.BIN", "UNKNOWN",      "UNKNOWN",      "UNKNOWN",      "UNKNOWN",      "UNKNOWN" };

        const fheroes2::MonsterSound monsterSounds[Monster::MONSTER_COUNT] = {
            // melee attack | death | movement | wince |ranged attack
            { M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN }, // Unknown Monster
            { M82::PSNTATTK, M82::PSNTKILL, M82::PSNTMOVE, M82::PSNTWNCE, M82::UNKNOWN }, // Peasant
            { M82::ARCHATTK, M82::ARCHKILL, M82::ARCHMOVE, M82::ARCHWNCE, M82::ARCHSHOT }, // Archer
            { M82::ARCHATTK, M82::ARCHKILL, M82::ARCHMOVE, M82::ARCHWNCE, M82::ARCHSHOT }, // Ranger
            { M82::PIKEATTK, M82::PIKEKILL, M82::PIKEMOVE, M82::PIKEWNCE, M82::UNKNOWN }, // Pikeman
            { M82::PIKEATTK, M82::PIKEKILL, M82::PIKEMOVE, M82::PIKEWNCE, M82::UNKNOWN }, // Veteran Pikeman
            { M82::SWDMATTK, M82::SWDMKILL, M82::SWDMMOVE, M82::SWDMWNCE, M82::UNKNOWN }, // Swordsman
            { M82::SWDMATTK, M82::SWDMKILL, M82::SWDMMOVE, M82::SWDMWNCE, M82::UNKNOWN }, // Master Swordsman
            { M82::CAVLATTK, M82::CAVLKILL, M82::CAVLMOVE, M82::CAVLWNCE, M82::UNKNOWN }, // Cavalry
            { M82::CAVLATTK, M82::CAVLKILL, M82::CAVLMOVE, M82::CAVLWNCE, M82::UNKNOWN }, // Champion
            { M82::PLDNATTK, M82::PLDNKILL, M82::PLDNMOVE, M82::PLDNWNCE, M82::UNKNOWN }, // Paladin
            { M82::PLDNATTK, M82::PLDNKILL, M82::PLDNMOVE, M82::PLDNWNCE, M82::UNKNOWN }, // Crusader
            { M82::GBLNATTK, M82::GBLNKILL, M82::GBLNMOVE, M82::GBLNWNCE, M82::UNKNOWN }, // Goblin
            { M82::ORC_ATTK, M82::ORC_KILL, M82::ORC_MOVE, M82::ORC_WNCE, M82::ORC_SHOT }, // Orc
            { M82::ORC_ATTK, M82::ORC_KILL, M82::ORC_MOVE, M82::ORC_WNCE, M82::ORC_SHOT }, // Orc Chief
            { M82::WOLFATTK, M82::WOLFKILL, M82::WOLFMOVE, M82::WOLFWNCE, M82::UNKNOWN }, // Wolf
            { M82::OGREATTK, M82::OGREKILL, M82::OGREMOVE, M82::OGREWNCE, M82::UNKNOWN }, // Ogre
            { M82::OGREATTK, M82::OGREKILL, M82::OGREMOVE, M82::OGREWNCE, M82::UNKNOWN }, // Ogre Lord
            { M82::TRLLATTK, M82::TRLLKILL, M82::TRLLMOVE, M82::TRLLWNCE, M82::TRLLSHOT }, // Troll
            { M82::TRLLATTK, M82::TRLLKILL, M82::TRLLMOVE, M82::TRLLWNCE, M82::TRLLSHOT }, // War Troll
            { M82::CYCLATTK, M82::CYCLKILL, M82::CYCLMOVE, M82::CYCLWNCE, M82::UNKNOWN }, // Cyclops
            { M82::SPRTATTK, M82::SPRTKILL, M82::SPRTMOVE, M82::SPRTWNCE, M82::UNKNOWN }, // Sprite
            { M82::DWRFATTK, M82::DWRFKILL, M82::DWRFMOVE, M82::DWRFWNCE, M82::UNKNOWN }, // Dwarf
            { M82::DWRFATTK, M82::DWRFKILL, M82::DWRFMOVE, M82::DWRFWNCE, M82::UNKNOWN }, // Battle Dwarf
            { M82::ELF_ATTK, M82::ELF_KILL, M82::ELF_MOVE, M82::ELF_WNCE, M82::ELF_SHOT }, // Elf
            { M82::ELF_ATTK, M82::ELF_KILL, M82::ELF_MOVE, M82::ELF_WNCE, M82::ELF_SHOT }, // Grand Elf
            { M82::DRUIATTK, M82::DRUIKILL, M82::DRUIMOVE, M82::DRUIWNCE, M82::DRUISHOT }, // Druid
            { M82::DRUIATTK, M82::DRUIKILL, M82::DRUIMOVE, M82::DRUIWNCE, M82::DRUISHOT }, // Greater Druid
            { M82::UNICATTK, M82::UNICKILL, M82::UNICMOVE, M82::UNICWNCE, M82::UNKNOWN }, // Unicorn
            { M82::PHOEATTK, M82::PHOEKILL, M82::PHOEMOVE, M82::PHOEWNCE, M82::UNKNOWN }, // Phoenix
            { M82::CNTRATTK, M82::CNTRKILL, M82::CNTRMOVE, M82::CNTRWNCE, M82::CNTRSHOT }, // Centaur
            { M82::GARGATTK, M82::GARGKILL, M82::GARGMOVE, M82::GARGWNCE, M82::UNKNOWN }, // Gargoyle
            { M82::GRIFATTK, M82::GRIFKILL, M82::GRIFMOVE, M82::GRIFWNCE, M82::UNKNOWN }, // Griffin
            { M82::MINOATTK, M82::MINOKILL, M82::MINOMOVE, M82::MINOWNCE, M82::UNKNOWN }, // Minotaur
            { M82::MINOATTK, M82::MINOKILL, M82::MINOMOVE, M82::MINOWNCE, M82::UNKNOWN }, // Minotaur King
            { M82::HYDRATTK, M82::HYDRKILL, M82::HYDRMOVE, M82::HYDRWNCE, M82::UNKNOWN }, // Hydra
            { M82::DRGNATTK, M82::DRGNKILL, M82::DRGNMOVE, M82::DRGNWNCE, M82::UNKNOWN }, // Green Dragon
            { M82::DRGNATTK, M82::DRGNKILL, M82::DRGNMOVE, M82::DRGNWNCE, M82::UNKNOWN }, // Red Dragon
            { M82::DRGNATTK, M82::DRGNKILL, M82::DRGNMOVE, M82::DRGNWNCE, M82::UNKNOWN }, // Black Dragon
            { M82::HALFATTK, M82::HALFKILL, M82::HALFMOVE, M82::HALFWNCE, M82::HALFSHOT }, // Halfling
            { M82::BOARATTK, M82::BOARKILL, M82::BOARMOVE, M82::BOARWNCE, M82::UNKNOWN }, // Boar
            { M82::GOLMATTK, M82::GOLMKILL, M82::GOLMMOVE, M82::GOLMWNCE, M82::UNKNOWN }, // Iron Golem
            { M82::GOLMATTK, M82::GOLMKILL, M82::GOLMMOVE, M82::GOLMWNCE, M82::UNKNOWN }, // Steel Golem
            { M82::ROC_ATTK, M82::ROC_KILL, M82::ROC_MOVE, M82::ROC_WNCE, M82::UNKNOWN }, // Roc
            { M82::MAGEATTK, M82::MAGEKILL, M82::MAGEMOVE, M82::MAGEWNCE, M82::MAGESHOT }, // Mage
            { M82::MAGEATTK, M82::MAGEKILL, M82::MAGEMOVE, M82::MAGEWNCE, M82::MAGESHOT }, // Archmage
            { M82::TITNATTK, M82::TITNKILL, M82::TITNMOVE, M82::TITNWNCE, M82::UNKNOWN }, // Giant
            { M82::TITNATTK, M82::TITNKILL, M82::TITNMOVE, M82::TITNWNCE, M82::TITNSHOT }, // Titan
            { M82::SKELATTK, M82::SKELKILL, M82::SKELMOVE, M82::SKELWNCE, M82::UNKNOWN }, // Skeleton
            { M82::ZOMBATTK, M82::ZOMBKILL, M82::ZOMBMOVE, M82::ZOMBWNCE, M82::UNKNOWN }, // Zombie
            { M82::ZOMBATTK, M82::ZOMBKILL, M82::ZOMBMOVE, M82::ZOMBWNCE, M82::UNKNOWN }, // Mutant Zombie
            { M82::MUMYATTK, M82::MUMYKILL, M82::MUMYMOVE, M82::MUMYWNCE, M82::UNKNOWN }, // Mummy
            { M82::MUMYATTK, M82::MUMYKILL, M82::MUMYMOVE, M82::MUMYWNCE, M82::UNKNOWN }, // Royal Mummy
            { M82::VAMPATTK, M82::VAMPKILL, M82::VAMPMOVE, M82::VAMPWNCE, M82::UNKNOWN }, // Vampire
            { M82::VAMPATTK, M82::VAMPKILL, M82::VAMPMOVE, M82::VAMPWNCE, M82::UNKNOWN }, // Vampire Lord
            { M82::LICHATTK, M82::LICHKILL, M82::LICHMOVE, M82::LICHWNCE, M82::LICHSHOT }, // Lich
            { M82::LICHATTK, M82::LICHKILL, M82::LICHMOVE, M82::LICHWNCE, M82::LICHSHOT }, // Power Lich
            { M82::BONEATTK, M82::BONEKILL, M82::BONEMOVE, M82::BONEWNCE, M82::UNKNOWN }, // Bone Dragon
            { M82::ROGUATTK, M82::ROGUKILL, M82::ROGUMOVE, M82::ROGUWNCE, M82::UNKNOWN }, // Rogue
            { M82::NMADATTK, M82::NMADKILL, M82::NMADMOVE, M82::NMADWNCE, M82::UNKNOWN }, // Nomad
            { M82::GHSTATTK, M82::GHSTKILL, M82::GHSTMOVE, M82::GHSTWNCE, M82::UNKNOWN }, // Ghost
            { M82::GENIATTK, M82::GENIKILL, M82::GENIMOVE, M82::GENIWNCE, M82::UNKNOWN }, // Genie
            { M82::MEDSATTK, M82::MEDSKILL, M82::MEDSMOVE, M82::MEDSWNCE, M82::UNKNOWN }, // Medusa
            { M82::EELMATTK, M82::EELMKILL, M82::EELMMOVE, M82::EELMWNCE, M82::UNKNOWN }, // Earth Elemental
            { M82::AELMATTK, M82::AELMKILL, M82::AELMMOVE, M82::AELMWNCE, M82::UNKNOWN }, // Air Elemental
            { M82::FELMATTK, M82::FELMKILL, M82::FELMMOVE, M82::FELMWNCE, M82::UNKNOWN }, // Fire Elemental
            { M82::WELMATTK, M82::WELMKILL, M82::WELMMOVE, M82::WELMWNCE, M82::UNKNOWN }, // Water Elemental
            { M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN }, // Random Monster
            { M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN }, // Random Monster 1
            { M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN }, // Random Monster 2
            { M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN }, // Random Monster 3
            { M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN, M82::UNKNOWN }, // Random Monster 4
        };

        // Monster abilities and weaknesses will be added later.
        const fheroes2::MonsterBattleStats monsterBattleStats[Monster::MONSTER_COUNT] = {
            // attack | defence | damageMin | damageMax | hp | speed | shots | abilities | weaknesses
            { 0, 0, 0, 0, 0, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Unknown Monster
            { 1, 1, 1, 1, 1, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Peasant
            { 5, 3, 2, 3, 10, Speed::VERYSLOW, 12, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Archer
            { 5, 3, 2, 3, 10, Speed::AVERAGE, 24, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Ranger
            { 5, 9, 3, 4, 15, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Pikeman
            { 5, 9, 3, 4, 20, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Veteran Pikeman
            { 7, 9, 4, 6, 25, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Swordsman
            { 7, 9, 4, 6, 30, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Master Swordsman
            { 10, 9, 5, 10, 30, Speed::VERYFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Cavalry
            { 10, 9, 5, 10, 40, Speed::ULTRAFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Champion
            { 11, 12, 10, 20, 50, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Paladin
            { 11, 12, 10, 20, 65, Speed::VERYFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Crusader
            { 3, 1, 1, 2, 3, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Goblin
            { 3, 4, 2, 3, 10, Speed::VERYSLOW, 8, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Orc
            { 3, 4, 3, 4, 15, Speed::SLOW, 16, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Orc Chief
            { 6, 2, 3, 5, 20, Speed::VERYFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Wolf
            { 9, 5, 4, 6, 40, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Ogre
            { 9, 5, 5, 7, 60, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Ogre Lord
            { 10, 5, 5, 7, 40, Speed::AVERAGE, 8, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Troll
            { 10, 5, 7, 9, 40, Speed::FAST, 16, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // War Troll
            { 12, 9, 12, 24, 80, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Cyclops
            { 4, 2, 1, 2, 2, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Sprite
            { 6, 5, 2, 4, 20, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Dwarf
            { 6, 6, 2, 4, 20, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Battle Dwarf
            { 4, 3, 2, 3, 15, Speed::AVERAGE, 24, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Elf
            { 5, 5, 2, 3, 15, Speed::VERYFAST, 24, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Grand Elf
            { 7, 5, 5, 8, 25, Speed::FAST, 8, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Druid
            { 7, 7, 5, 8, 25, Speed::VERYFAST, 16, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Greater Druid
            { 10, 9, 7, 14, 40, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Unicorn
            { 12, 10, 20, 40, 100, Speed::ULTRAFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Phoenix
            { 3, 1, 1, 2, 5, Speed::AVERAGE, 8, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Centaur
            { 4, 7, 2, 3, 15, Speed::VERYFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Gargoyle
            { 6, 6, 3, 5, 25, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Griffin
            { 9, 8, 5, 10, 35, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Minotaur
            { 9, 8, 5, 10, 45, Speed::VERYFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Minotaur King
            { 8, 9, 6, 12, 75, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Hydra
            { 12, 12, 25, 50, 200, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Green Dragon
            { 13, 13, 25, 50, 250, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Red Dragon
            { 14, 14, 25, 50, 300, Speed::VERYFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Black Dragon
            { 2, 1, 1, 3, 3, Speed::SLOW, 12, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Halfling
            { 5, 4, 2, 3, 15, Speed::VERYFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Boar
            { 5, 10, 4, 5, 30, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Iron Golem
            { 7, 10, 4, 5, 35, Speed::SLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Steel Golem
            { 7, 7, 4, 8, 40, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Roc
            { 11, 7, 7, 9, 30, Speed::FAST, 12, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Mage
            { 12, 8, 7, 9, 35, Speed::VERYFAST, 24, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Archmage
            { 13, 10, 20, 30, 150, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Giant
            { 15, 15, 20, 30, 300, Speed::VERYFAST, 24, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Titan
            { 4, 3, 2, 3, 4, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Skeleton
            { 5, 2, 2, 3, 15, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Zombie
            { 5, 2, 2, 3, 20, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Mutant Zombie
            { 6, 6, 3, 4, 25, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Mummy
            { 6, 6, 3, 4, 30, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Royal Mummy
            { 8, 6, 5, 7, 30, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Vampire
            { 8, 6, 5, 7, 40, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Vampire Lord
            { 7, 12, 8, 10, 25, Speed::FAST, 12, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Lich
            { 7, 13, 8, 10, 35, Speed::VERYFAST, 24, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Power Lich
            { 11, 9, 25, 45, 150, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Bone Dragon
            { 6, 1, 1, 2, 4, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Rogue
            { 7, 6, 2, 5, 20, Speed::VERYFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Nomad
            { 8, 7, 4, 6, 20, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Ghost
            { 10, 9, 20, 30, 50, Speed::VERYFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Genie
            { 8, 9, 6, 10, 35, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Medusa
            { 8, 8, 4, 5, 50, Speed::SLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Earth Elemental
            { 7, 7, 2, 8, 35, Speed::VERYFAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Air Elemental
            { 8, 6, 4, 6, 40, Speed::FAST, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Fire Elemental
            { 6, 8, 3, 7, 45, Speed::AVERAGE, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Water Elemental
            { 0, 0, 0, 0, 0, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Random Monster
            { 0, 0, 0, 0, 0, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Random Monster 1
            { 0, 0, 0, 0, 0, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Random Monster 2
            { 0, 0, 0, 0, 0, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Random Monster 3
            { 0, 0, 0, 0, 0, Speed::VERYSLOW, 0, std::set<fheroes2::MonsterAbility>(), std::set<fheroes2::MonsterWeakness>() }, // Random Monster 4
        };

        const fheroes2::MonsterGeneralStats monsterGeneralStats[Monster::MONSTER_COUNT]
            = { // name | plural name | growth | race | level | cost
                { "Unknown Monster", "Unknown Monsters", 0, Race::NONE, 0, { 0, 0, 0, 0, 0, 0, 0 } },
                { "Peasant", "Peasants", 12, Race::KNGT, 1, { 20, 0, 0, 0, 0, 0, 0 } },
                { "Archer", "Archers", 8, Race::KNGT, 2, { 150, 0, 0, 0, 0, 0, 0 } },
                { "Ranger", "Rangers", 8, Race::KNGT, 2, { 200, 0, 0, 0, 0, 0, 0 } },
                { "Pikeman", "Pikemen", 5, Race::KNGT, 3, { 200, 0, 0, 0, 0, 0, 0 } },
                { "Veteran Pikeman", "Veteran Pikemen", 5, Race::KNGT, 3, { 250, 0, 0, 0, 0, 0, 0 } },
                { "Swordsman", "Swordsmen", 4, Race::KNGT, 4, { 250, 0, 0, 0, 0, 0, 0 } },
                { "Master Swordsman", "Master Swordsmen", 4, Race::KNGT, 4, { 300, 0, 0, 0, 0, 0, 0 } },
                { "Cavalry", "Cavalries", 3, Race::KNGT, 5, { 300, 0, 0, 0, 0, 0, 0 } },
                { "Champion", "Champions", 3, Race::KNGT, 5, { 375, 0, 0, 0, 0, 0, 0 } },
                { "Paladin", "Paladins", 2, Race::KNGT, 6, { 600, 0, 0, 0, 0, 0, 0 } },
                { "Crusader", "Crusaders", 2, Race::KNGT, 6, { 1000, 0, 0, 0, 0, 0, 0 } },
                { "Goblin", "Goblins", 10, Race::BARB, 1, { 40, 0, 0, 0, 0, 0, 0 } },
                { "Orc", "Orcs", 8, Race::BARB, 2, { 140, 0, 0, 0, 0, 0, 0 } },
                { "Orc Chief", "Orc Chiefs", 8, Race::BARB, 2, { 175, 0, 0, 0, 0, 0, 0 } },
                { "Wolf", "Wolves", 5, Race::BARB, 3, { 200, 0, 0, 0, 0, 0, 0 } },
                { "Ogre", "Ogres", 4, Race::BARB, 4, { 300, 0, 0, 0, 0, 0, 0 } },
                { "Ogre Lord", "Ogre Lords", 4, Race::BARB, 4, { 500, 0, 0, 0, 0, 0, 0 } },
                { "Troll", "Trolls", 3, Race::BARB, 5, { 600, 0, 0, 0, 0, 0, 0 } },
                { "War Troll", "War Trolls", 3, Race::BARB, 5, { 700, 0, 0, 0, 0, 0, 0 } },
                { "Cyclops", "Cyclopes", 2, Race::BARB, 6, { 750, 0, 0, 0, 0, 1, 0 } },
                { "Sprite", "Sprites", 8, Race::SORC, 1, { 50, 0, 0, 0, 0, 0, 0 } },
                { "Dwarf", "Dwarves", 6, Race::SORC, 2, { 200, 0, 0, 0, 0, 0, 0 } },
                { "Battle Dwarf", "Battle Dwarves", 6, Race::SORC, 2, { 250, 0, 0, 0, 0, 0, 0 } },
                { "Elf", "Elves", 4, Race::SORC, 3, { 250, 0, 0, 0, 0, 0, 0 } },
                { "Grand Elf", "Grand Elves", 4, Race::SORC, 3, { 300, 0, 0, 0, 0, 0, 0 } },
                { "Druid", "Druids", 3, Race::SORC, 4, { 350, 0, 0, 0, 0, 0, 0 } },
                { "Greater Druid", "Greater Druids", 3, Race::SORC, 4, { 400, 0, 0, 0, 0, 0, 0 } },
                { "Unicorn", "Unicorns", 2, Race::SORC, 5, { 500, 0, 0, 0, 0, 0, 0 } },
                { "Phoenix", "Phoenixes", 1, Race::SORC, 6, { 1500, 0, 1, 0, 0, 0, 0 } },
                { "Centaur", "Centaurs", 8, Race::WRLK, 1, { 60, 0, 0, 0, 0, 0, 0 } },
                { "Gargoyle", "Gargoyles", 6, Race::WRLK, 2, { 200, 0, 0, 0, 0, 0, 0 } },
                { "Griffin", "Griffins", 4, Race::WRLK, 3, { 300, 0, 0, 0, 0, 0, 0 } },
                { "Minotaur", "Minotaurs", 3, Race::WRLK, 4, { 400, 0, 0, 0, 0, 0, 0 } },
                { "Minotaur King", "Minotaur Kings", 3, Race::WRLK, 4, { 500, 0, 0, 0, 0, 0, 0 } },
                { "Hydra", "Hydras", 2, Race::WRLK, 5, { 800, 0, 0, 0, 0, 0, 0 } },
                { "Green Dragon", "Green Dragons", 1, Race::WRLK, 6, { 3000, 0, 0, 0, 1, 0, 0 } },
                { "Red Dragon", "Red Dragons", 1, Race::WRLK, 6, { 3500, 0, 0, 0, 1, 0, 0 } },
                { "Black Dragon", "Black Dragons", 1, Race::WRLK, 6, { 4000, 0, 0, 0, 2, 0, 0 } },
                { "Halfling", "Halflings", 8, Race::WZRD, 1, { 50, 0, 0, 0, 0, 0, 0 } },
                { "Boar", "Boars", 6, Race::WZRD, 2, { 150, 0, 0, 0, 0, 0, 0 } },
                { "Iron Golem", "Iron Golems", 4, Race::WZRD, 3, { 300, 0, 0, 0, 0, 0, 0 } },
                { "Steel Golem", "Steel Golems", 4, Race::WZRD, 3, { 350, 0, 0, 0, 0, 0, 0 } },
                { "Roc", "Rocs", 3, Race::WZRD, 4, { 400, 0, 0, 0, 0, 0, 0 } },
                { "Mage", "Magi", 2, Race::WZRD, 5, { 600, 0, 0, 0, 0, 0, 0 } },
                { "Archmage", "Archmagi", 2, Race::WZRD, 5, { 700, 0, 0, 0, 0, 0, 0 } },
                { "Giant", "Giants", 1, Race::WZRD, 6, { 2000, 0, 0, 0, 0, 0, 1 } },
                { "Titan", "Titans", 1, Race::WZRD, 6, { 5000, 0, 0, 0, 0, 0, 2 } },
                { "Skeleton", "Skeletons", 8, Race::NECR, 1, { 75, 0, 0, 0, 0, 0, 0 } },
                { "Zombie", "Zombies", 6, Race::NECR, 2, { 150, 0, 0, 0, 0, 0, 0 } },
                { "Mutant Zombie", "Mutant Zombies", 6, Race::NECR, 2, { 200, 0, 0, 0, 0, 0, 0 } },
                { "Mummy", "Mummies", 4, Race::NECR, 3, { 250, 0, 0, 0, 0, 0, 0 } },
                { "Royal Mummy", "Royal Mummies", 4, Race::NECR, 3, { 300, 0, 0, 0, 0, 0, 0 } },
                { "Vampire", "Vampires", 3, Race::NECR, 4, { 500, 0, 0, 0, 0, 0, 0 } },
                { "Vampire Lord", "Vampire Lords", 3, Race::NECR, 4, { 650, 0, 0, 0, 0, 0, 0 } },
                { "Lich", "Liches", 2, Race::NECR, 5, { 750, 0, 0, 0, 0, 0, 0 } },
                { "Power Lich", "Power Liches", 2, Race::NECR, 5, { 900, 0, 0, 0, 0, 0, 0 } },
                { "Bone Dragon", "Bone Dragons", 1, Race::NECR, 6, { 1500, 0, 0, 0, 0, 0, 0 } },
                { "Rogue", "Rogues", 8, Race::NONE, 1, { 50, 0, 0, 0, 0, 0, 0 } },
                { "Nomad", "Nomads", 4, Race::NONE, 2, { 200, 0, 0, 0, 0, 0, 0 } },
                { "Ghost", "Ghosts", 3, Race::NONE, 3, { 1000, 0, 0, 0, 0, 0, 0 } },
                { "Genie", "Genies", 2, Race::NONE, 6, { 650, 0, 0, 0, 0, 0, 1 } },
                { "Medusa", "Medusas", 5, Race::NONE, 4, { 500, 0, 0, 0, 0, 0, 0 } },
                { "Earth Elemental", "Earth Elementals", 4, Race::NONE, 4, { 500, 0, 0, 0, 0, 0, 0 } },
                { "Air Elemental", "Air Elementals", 4, Race::NONE, 4, { 500, 0, 0, 0, 0, 0, 0 } },
                { "Fire Elemental", "Fire Elementals", 4, Race::NONE, 4, { 500, 0, 0, 0, 0, 0, 0 } },
                { "Water Elemental", "Water Elementals", 4, Race::NONE, 4, { 500, 0, 0, 0, 0, 0, 0 } },
                { "Random Monster", "Random Monsters", 0, Race::NONE, 0, { 0, 0, 0, 0, 0, 0, 0 } },
                { "Random Monster 1", "Random Monsters 1", 0, Race::NONE, 1, { 0, 0, 0, 0, 0, 0, 0 } },
                { "Random Monster 2", "Random Monsters 2", 0, Race::NONE, 2, { 0, 0, 0, 0, 0, 0, 0 } },
                { "Random Monster 3", "Random Monsters 3", 0, Race::NONE, 3, { 0, 0, 0, 0, 0, 0, 0 } },
                { "Random Monster 4", "Random Monsters 4", 0, Race::NONE, 4, { 0, 0, 0, 0, 0, 0, 0 } } };

        monsterData.reserve( Monster::MONSTER_COUNT );

        for ( int i = 0; i < Monster::MONSTER_COUNT; ++i ) {
            monsterData.emplace_back( monsterIcnIds[i], binFileName[i], monsterSounds[i], monsterBattleStats[i], monsterGeneralStats[i] );
        }

        // Add monster abilities and weaknesses.
        monsterData[Monster::RANGER].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_SHOOTING );

        monsterData[Monster::CAVALRY].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );

        monsterData[Monster::CHAMPION].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );

        monsterData[Monster::PALADIN].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_MELEE_ATTACK );

        monsterData[Monster::CRUSADER].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_MELEE_ATTACK );
        monsterData[Monster::CRUSADER].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_DAMAGE_TO_UNDEAD );
        monsterData[Monster::CRUSADER].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::IMMUNE_TO_CERTAIN_SPELL, 100, Spell::CURSE );
        monsterData[Monster::CRUSADER].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::IMMUNE_TO_CERTAIN_SPELL, 100, Spell::MASSCURSE );

        monsterData[Monster::WOLF].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::WOLF].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_MELEE_ATTACK );

        monsterData[Monster::TROLL].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::HP_REGENERATION );

        monsterData[Monster::WAR_TROLL].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::HP_REGENERATION );

        monsterData[Monster::CYCLOPS].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::TWO_CELL_MELEE_ATTACK );
        monsterData[Monster::CYCLOPS].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::SPELL_CASTER, 20, Spell::PARALYZE );

        monsterData[Monster::SPRITE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::NO_ENEMY_RETALIATION );
        monsterData[Monster::SPRITE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );

        monsterData[Monster::DWARF].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::MAGIC_RESISTANCE, 25, 0 );

        monsterData[Monster::BATTLE_DWARF].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::MAGIC_RESISTANCE, 25, 0 );

        monsterData[Monster::ELF].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_SHOOTING );

        monsterData[Monster::GRAND_ELF].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_SHOOTING );

        monsterData[Monster::UNICORN].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::UNICORN].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::SPELL_CASTER, 20, Spell::BLIND );

        monsterData[Monster::PHOENIX].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::PHOENIX].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::TWO_CELL_MELEE_ATTACK );
        monsterData[Monster::PHOENIX].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );
        monsterData[Monster::PHOENIX].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::ELEMENTAL_SPELL_IMMUNITY );

        monsterData[Monster::CENTAUR].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );

        monsterData[Monster::GARGOYLE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );

        monsterData[Monster::GRIFFIN].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::GRIFFIN].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );
        monsterData[Monster::GRIFFIN].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::ALWAYS_RETALIATE );

        monsterData[Monster::HYDRA].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::HYDRA].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::ALL_ADJACENT_CELL_MELEE_ATTACK );
        monsterData[Monster::HYDRA].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::NO_ENEMY_RETALIATION );

        monsterData[Monster::GREEN_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DRAGON );
        monsterData[Monster::GREEN_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::GREEN_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );
        monsterData[Monster::GREEN_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::MAGIC_RESISTANCE, 100, 0 );
        monsterData[Monster::GREEN_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::TWO_CELL_MELEE_ATTACK );

        monsterData[Monster::RED_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DRAGON );
        monsterData[Monster::RED_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::RED_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );
        monsterData[Monster::RED_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::MAGIC_RESISTANCE, 100, 0 );
        monsterData[Monster::RED_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::TWO_CELL_MELEE_ATTACK );

        monsterData[Monster::BLACK_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DRAGON );
        monsterData[Monster::BLACK_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::BLACK_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );
        monsterData[Monster::BLACK_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::MAGIC_RESISTANCE, 100, 0 );
        monsterData[Monster::BLACK_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::TWO_CELL_MELEE_ATTACK );

        monsterData[Monster::BOAR].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );

        monsterData[Monster::IRON_GOLEM].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::ELEMENTAL_SPELL_DAMAGE_REDUCTION, 50, 0 );

        monsterData[Monster::STEEL_GOLEM].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::ELEMENTAL_SPELL_DAMAGE_REDUCTION, 50, 0 );

        monsterData[Monster::ROC].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::ROC].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );

        monsterData[Monster::MAGE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::NO_MELEE_PENALTY );

        monsterData[Monster::ARCHMAGE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::NO_MELEE_PENALTY );
        monsterData[Monster::ARCHMAGE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::SPELL_CASTER, 20, Spell::DISPEL );

        monsterData[Monster::GIANT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::MIND_SPELL_IMMUNITY );

        monsterData[Monster::TITAN].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::NO_MELEE_PENALTY );
        monsterData[Monster::TITAN].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::MIND_SPELL_IMMUNITY );

        monsterData[Monster::SKELETON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );

        monsterData[Monster::ZOMBIE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );

        monsterData[Monster::MUTANT_ZOMBIE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );

        monsterData[Monster::MUMMY].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );
        monsterData[Monster::MUMMY].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::SPELL_CASTER, 20, Spell::CURSE );

        monsterData[Monster::ROYAL_MUMMY].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );
        monsterData[Monster::ROYAL_MUMMY].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::SPELL_CASTER, 30, Spell::CURSE );

        monsterData[Monster::VAMPIRE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );
        monsterData[Monster::VAMPIRE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );
        monsterData[Monster::VAMPIRE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::NO_ENEMY_RETALIATION );

        monsterData[Monster::VAMPIRE_LORD].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );
        monsterData[Monster::VAMPIRE_LORD].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );
        monsterData[Monster::VAMPIRE_LORD].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::NO_ENEMY_RETALIATION );
        monsterData[Monster::VAMPIRE_LORD].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::HP_DRAIN );

        monsterData[Monster::LICH].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );
        monsterData[Monster::LICH].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::AREA_SHOT );

        monsterData[Monster::POWER_LICH].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );
        monsterData[Monster::POWER_LICH].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::AREA_SHOT );

        monsterData[Monster::BONE_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );
        monsterData[Monster::BONE_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DRAGON );
        monsterData[Monster::BONE_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::BONE_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );
        monsterData[Monster::BONE_DRAGON].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::MORAL_DECREMENT, 100, 1 );

        monsterData[Monster::ROGUE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::NO_ENEMY_RETALIATION );

        monsterData[Monster::GHOST].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::SOUL_EATER );
        monsterData[Monster::GHOST].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );
        monsterData[Monster::GHOST].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::UNDEAD );

        monsterData[Monster::GENIE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::ENEMY_HALFING, 10, 0 );
        monsterData[Monster::GENIE].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FLYING );

        monsterData[Monster::MEDUSA].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
        monsterData[Monster::MEDUSA].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::SPELL_CASTER, 20, Spell::STONE );

        monsterData[Monster::NOMAD].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );

        monsterData[Monster::AIR_ELEMENT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::ELEMENTAL );
        monsterData[Monster::AIR_ELEMENT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::IMMUNE_TO_CERTAIN_SPELL, 100, Spell::METEORSHOWER );
        monsterData[Monster::AIR_ELEMENT].battleStats.weaknesses.emplace( fheroes2::MonsterWeaknessType::EXTRA_DAMAGE_FROM_CERTAIN_SPELL, 100, Spell::LIGHTNINGBOLT );
        monsterData[Monster::AIR_ELEMENT].battleStats.weaknesses.emplace( fheroes2::MonsterWeaknessType::EXTRA_DAMAGE_FROM_CERTAIN_SPELL, 100, Spell::CHAINLIGHTNING );
        monsterData[Monster::AIR_ELEMENT].battleStats.weaknesses.emplace( fheroes2::MonsterWeaknessType::EXTRA_DAMAGE_FROM_CERTAIN_SPELL, 100, Spell::ELEMENTALSTORM );

        monsterData[Monster::EARTH_ELEMENT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::ELEMENTAL );
        monsterData[Monster::EARTH_ELEMENT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::IMMUNE_TO_CERTAIN_SPELL, 100, Spell::LIGHTNINGBOLT );
        monsterData[Monster::EARTH_ELEMENT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::IMMUNE_TO_CERTAIN_SPELL, 100, Spell::CHAINLIGHTNING );
        monsterData[Monster::EARTH_ELEMENT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::IMMUNE_TO_CERTAIN_SPELL, 100, Spell::ELEMENTALSTORM );
        monsterData[Monster::EARTH_ELEMENT].battleStats.weaknesses.emplace( fheroes2::MonsterWeaknessType::EXTRA_DAMAGE_FROM_CERTAIN_SPELL, 100, Spell::METEORSHOWER );

        monsterData[Monster::FIRE_ELEMENT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::ELEMENTAL );
        monsterData[Monster::FIRE_ELEMENT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::FIRE_SPELL_IMMUNITY );
        monsterData[Monster::FIRE_ELEMENT].battleStats.weaknesses.emplace( fheroes2::MonsterWeaknessType::EXTRA_DAMAGE_FROM_COLD_SPELL );

        monsterData[Monster::WATER_ELEMENT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::ELEMENTAL );
        monsterData[Monster::WATER_ELEMENT].battleStats.abilities.emplace( fheroes2::MonsterAbilityType::COLD_SPELL_IMMUNITY );
        monsterData[Monster::WATER_ELEMENT].battleStats.weaknesses.emplace( fheroes2::MonsterWeaknessType::EXTRA_DAMAGE_FROM_FIRE_SPELL );
    }

    void removeDuplicateSpell( std::set<int> & sortedSpellIds, const int massSpellId, const int spellId )
    {
        if ( sortedSpellIds.count( massSpellId ) > 0 && sortedSpellIds.count( spellId ) > 0 ) {
            sortedSpellIds.erase( massSpellId );
        }
    }

    std::vector<int> replaceMassSpells( const std::vector<int> & spellIds )
    {
        std::set<int> sortedSpellIds( spellIds.begin(), spellIds.end() );

        removeDuplicateSpell( sortedSpellIds, Spell::CHAINLIGHTNING, Spell::LIGHTNINGBOLT );
        removeDuplicateSpell( sortedSpellIds, Spell::MASSCURE, Spell::CURE );
        removeDuplicateSpell( sortedSpellIds, Spell::MASSCURSE, Spell::CURSE );
        removeDuplicateSpell( sortedSpellIds, Spell::MASSBLESS, Spell::BLESS );
        removeDuplicateSpell( sortedSpellIds, Spell::MASSHASTE, Spell::HASTE );
        removeDuplicateSpell( sortedSpellIds, Spell::MASSSHIELD, Spell::SHIELD );
        removeDuplicateSpell( sortedSpellIds, Spell::MASSDISPEL, Spell::DISPEL );
        removeDuplicateSpell( sortedSpellIds, Spell::MASSSLOW, Spell::SLOW );

        return std::vector<int>( sortedSpellIds.begin(), sortedSpellIds.end() );
    }
}

namespace fheroes2
{
    const MonsterData & getMonsterData( const int monsterId )
    {
        if ( monsterData.empty() ) {
            populateMonsterData();
        }

        assert( monsterId >= 0 && static_cast<size_t>( monsterId ) < monsterData.size() );
        if ( monsterId < 0 || static_cast<size_t>( monsterId ) >= monsterData.size() ) {
            return monsterData.front();
        }

        return monsterData[monsterId];
    }

    std::string getMonsterAbilityDescription( const MonsterAbility & ability, const bool ignoreBasicAbility )
    {
        switch ( ability.type ) {
        case MonsterAbilityType::NONE:
            return ignoreBasicAbility ? "" : "None";
        case MonsterAbilityType::DOUBLE_SHOOTING:
            return "Double shot";
        case MonsterAbilityType::DOUBLE_HEX_SIZE:
            return ignoreBasicAbility ? "" : "2-hex monster";
        case MonsterAbilityType::DOUBLE_MELEE_ATTACK:
            return "Double strike";
        case MonsterAbilityType::DOUBLE_DAMAGE_TO_UNDEAD:
            return "Double damage to Undead";
        case MonsterAbilityType::MAGIC_RESISTANCE:
            return std::to_string( ability.percentage ) + "% magic resistance";
        case MonsterAbilityType::MIND_SPELL_IMMUNITY:
            return "Immune to Mind spells";
        case MonsterAbilityType::ELEMENTAL_SPELL_IMMUNITY:
            return "Immune to Elemental spells";
        case MonsterAbilityType::FIRE_SPELL_IMMUNITY:
            return "Immune to Fire spells";
        case MonsterAbilityType::COLD_SPELL_IMMUNITY:
            return "Immune to Cold spells";
        case MonsterAbilityType::IMMUNE_TO_CERTAIN_SPELL:
            if ( ability.percentage == 100 ) {
                return std::string( "Immune to " ) + Spell( ability.value ).GetName();
            }
            else {
                return std::to_string( ability.percentage ) + "% immunity to " + Spell( ability.value ).GetName() + " spell";
            }
        case MonsterAbilityType::ELEMENTAL_SPELL_DAMAGE_REDUCTION:
            return std::to_string( ability.percentage ) + "% damage from Elemental spells";
        case MonsterAbilityType::SPELL_CASTER:
            if ( ability.value == Spell::DISPEL ) {
                return std::to_string( ability.percentage ) + "% chance to Dispel beneficial spells";
            }
            else if ( ability.value == Spell::PARALYZE ) {
                return std::to_string( ability.percentage ) + "% chance to Paralyze";
            }
            else if ( ability.value == Spell::STONE ) {
                return std::to_string( ability.percentage ) + "% chance to Petrify";
            }
            else if ( ability.value == Spell::BLIND ) {
                return std::to_string( ability.percentage ) + "% chance to Blind";
            }
            else if ( ability.value == Spell::CURSE ) {
                return std::to_string( ability.percentage ) + "% chance to Curse";
            }
            else {
                return std::to_string( ability.percentage ) + "% chance to cast " + Spell( ability.value ).GetName() + " spell";
            }
        case MonsterAbilityType::HP_REGENERATION:
            return "HP regeneration";
        case MonsterAbilityType::TWO_CELL_MELEE_ATTACK:
            return "Two hexes attack";
        case MonsterAbilityType::FLYING:
            return ignoreBasicAbility ? "" : "Flyer";
        case MonsterAbilityType::ALWAYS_RETALIATE:
            return "Always retaliates";
        case MonsterAbilityType::ALL_ADJACENT_CELL_MELEE_ATTACK:
            return "Attacks all adjacent enemies";
        case MonsterAbilityType::NO_MELEE_PENALTY:
            return "No melee penalty";
        case MonsterAbilityType::DRAGON:
            return ignoreBasicAbility ? "" : "Dragon";
        case MonsterAbilityType::UNDEAD:
            return "Undead";
        case MonsterAbilityType::NO_ENEMY_RETALIATION:
            return "No enemy retaliation";
        case MonsterAbilityType::HP_DRAIN:
            return "HP drain";
        case MonsterAbilityType::AREA_SHOT:
            return "Cloud attack";
        case MonsterAbilityType::MORAL_DECREMENT:
            return "Decreases enemy's morale by " + std::to_string( ability.value );
        case MonsterAbilityType::ENEMY_HALFING:
            return std::to_string( ability.percentage ) + "% chance to halve enemy";
        case MonsterAbilityType::SOUL_EATER:
            return "Soul Eater";
        case MonsterAbilityType::ELEMENTAL:
            return ignoreBasicAbility ? "No Morale" : "Elemental";
        default:
            break;
        }

        assert( 0 ); // Did you add a new ability? Please add the implementation!
        return "";
    }

    std::string getMonsterWeaknessDescription( const MonsterWeakness & weakness, const bool ignoreBasicAbility )
    {
        switch ( weakness.type ) {
        case MonsterWeaknessType::NONE:
            return ignoreBasicAbility ? "" : "None";
        case MonsterWeaknessType::EXTRA_DAMAGE_FROM_FIRE_SPELL:
            return "200% damage from Fire spells";
        case MonsterWeaknessType::EXTRA_DAMAGE_FROM_COLD_SPELL:
            return "200% damage from Cold spells";
        case MonsterWeaknessType::EXTRA_DAMAGE_FROM_CERTAIN_SPELL:
            return std::to_string( weakness.percentage + 100 ) + "% damage from " + Spell( weakness.value ).GetName() + " spell";
        default:
            break;
        }

        assert( 0 ); // Did you add a new weakness? Please add the implementation!
        return "";
    }

    std::string getMonsterDescription( const int monsterId )
    {
        if ( monsterData.empty() ) {
            populateMonsterData();
        }

        assert( monsterId >= 0 && static_cast<size_t>( monsterId ) < monsterData.size() );
        if ( monsterId < 0 || static_cast<size_t>( monsterId ) >= monsterData.size() ) {
            return "";
        }

        const MonsterData & monster = monsterData[monsterId];

        std::ostringstream os;
        os << "----------" << std::endl;
        os << "Name: " << monster.generalStats.name << std::endl;
        os << "Plural name: " << monster.generalStats.pluralName << std::endl;
        os << "Base growth: " << monster.generalStats.baseGrowth << std::endl;
        os << "Race: " << Race::String( monster.generalStats.race ) << std::endl;
        os << "Level: " << monster.generalStats.level << std::endl;
        os << "Cost: " << Funds( monster.generalStats.cost ).String() << std::endl;
        os << std::endl;
        os << "Attack: " << monster.battleStats.attack << std::endl;
        os << "Defense: " << monster.battleStats.defense << std::endl;
        os << "Min damage: " << monster.battleStats.damageMin << std::endl;
        os << "Max damage: " << monster.battleStats.damageMax << std::endl;
        os << "Hit Points: " << monster.battleStats.hp << std::endl;
        os << "Speed: " << Speed::String( monster.battleStats.speed ) << std::endl;
        os << "Number of shots: " << monster.battleStats.shots << std::endl;
        if ( !monster.battleStats.abilities.empty() ) {
            os << std::endl;
            os << "Abilities:" << std::endl;
            for ( const MonsterAbility & ability : monster.battleStats.abilities ) {
                os << "   " << getMonsterAbilityDescription( ability, false ) << std::endl;
            }
        }

        if ( !monster.battleStats.weaknesses.empty() ) {
            os << std::endl;
            os << "Weaknesses:" << std::endl;
            for ( const MonsterWeakness & weakness : monster.battleStats.weaknesses ) {
                os << "   " << getMonsterWeaknessDescription( weakness, false ) << std::endl;
            }
        }

        return os.str();
    }

    std::vector<std::string> getMonsterPropertiesDescription( const int monsterId )
    {
        std::vector<std::string> output;

        const MonsterBattleStats & battleStats = getMonsterData( monsterId ).battleStats;

        const std::set<MonsterAbility> & abilities = battleStats.abilities;

        std::map<uint32_t, std::vector<int>> immuneToSpells;
        for ( const MonsterAbility & ability : abilities ) {
            if ( ability.type == MonsterAbilityType::IMMUNE_TO_CERTAIN_SPELL ) {
                immuneToSpells[ability.percentage].emplace_back( ability.value );
                continue;
            }
            const std::string abilityDescription = getMonsterAbilityDescription( ability, true );
            if ( !abilityDescription.empty() ) {
                output.emplace_back( abilityDescription + '.' );
            }
        }

        for ( auto spellInfoIter = immuneToSpells.begin(); spellInfoIter != immuneToSpells.end(); ++spellInfoIter ) {
            assert( !spellInfoIter->second.empty() );

            std::string temp;

            if ( spellInfoIter->first == 100 ) {
                temp += "Immune to ";
            }
            else {
                temp += std::to_string( spellInfoIter->first ) + "% immunity to ";
            }

            const std::vector<int> sortedSpells = replaceMassSpells( spellInfoIter->second );

            for ( size_t i = 0; i < sortedSpells.size(); ++i ) {
                if ( i > 0 ) {
                    temp += ", ";
                }

                if ( sortedSpells[i] == Spell::LIGHTNINGBOLT ) {
                    temp += "Lightning";
                }
                else {
                    temp += Spell( sortedSpells[i] ).GetName();
                }
            }

            temp += '.';
            output.emplace_back( std::move( temp ) );
        }

        std::map<uint32_t, std::vector<int>> extraDamageSpells;
        const std::set<MonsterWeakness> & weaknesses = battleStats.weaknesses;
        for ( const MonsterWeakness & weakness : weaknesses ) {
            if ( weakness.type == MonsterWeaknessType::EXTRA_DAMAGE_FROM_CERTAIN_SPELL ) {
                extraDamageSpells[weakness.percentage].emplace_back( weakness.value );
                continue;
            }

            const std::string weaknessDescription = getMonsterWeaknessDescription( weakness, true );
            if ( !weaknessDescription.empty() ) {
                output.emplace_back( weaknessDescription + '.' );
            }
        }

        for ( auto spellInfoIter = extraDamageSpells.begin(); spellInfoIter != extraDamageSpells.end(); ++spellInfoIter ) {
            assert( !spellInfoIter->second.empty() );

            std::string temp;

            temp += std::to_string( spellInfoIter->first + 100 ) + "% damage from ";

            const std::vector<int> sortedSpells = replaceMassSpells( spellInfoIter->second );

            for ( size_t i = 0; i < sortedSpells.size(); ++i ) {
                if ( i > 0 ) {
                    temp += ", ";
                }

                if ( sortedSpells[i] == Spell::LIGHTNINGBOLT ) {
                    temp += "Lightning";
                }
                else {
                    temp += Spell( sortedSpells[i] ).GetName();
                }
            }

            temp += '.';
            output.emplace_back( std::move( temp ) );
        }

        return output;
    }

    uint32_t getSpellResistance( const int monsterId, const int spellId )
    {
        const std::set<MonsterAbility> & abilities = getMonsterData( monsterId ).battleStats.abilities;

        Spell spell( spellId );

        // Find magic immunity for every spell.
        auto foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::MAGIC_RESISTANCE ) );
        if ( foundAbility != abilities.end() ) {
            if ( foundAbility->percentage == 100 ) {
                // Immune to everything.
                return foundAbility->percentage;
            }
            if ( spell.isDamage() || spell.isApplyToEnemies() ) {
                return foundAbility->percentage;
            }
        }

        foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::IMMUNE_TO_CERTAIN_SPELL, 0, spellId ) );
        if ( foundAbility != abilities.end() ) {
            return foundAbility->percentage;
        }

        if ( spell.isMindInfluence() ) {
            foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::MIND_SPELL_IMMUNITY ) );
            if ( foundAbility != abilities.end() ) {
                return 100;
            }

            foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::UNDEAD ) );
            if ( foundAbility != abilities.end() ) {
                return 100;
            }

            foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::ELEMENTAL ) );
            if ( foundAbility != abilities.end() ) {
                return 100;
            }
        }

        if ( spell.isALiveOnly() ) {
            foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::UNDEAD ) );
            if ( foundAbility != abilities.end() ) {
                return 100;
            }
        }

        if ( spell.isUndeadOnly() ) {
            foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::UNDEAD ) );
            if ( foundAbility == abilities.end() ) {
                return 100;
            }
        }

        if ( spell == Spell::RESURRECT || spell == Spell::RESURRECTTRUE || spell == Spell::ANIMATEDEAD ) {
            foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::ELEMENTAL ) );
            if ( foundAbility != abilities.end() ) {
                return 100;
            }
        }

        if ( spell.isCold() ) {
            foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::COLD_SPELL_IMMUNITY ) );
            if ( foundAbility != abilities.end() ) {
                return 100;
            }
        }

        if ( spell.isFire() ) {
            foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::FIRE_SPELL_IMMUNITY ) );
            if ( foundAbility != abilities.end() ) {
                return 100;
            }
        }

        if ( spell == Spell::COLDRAY || spell == Spell::COLDRING || spell == Spell::FIREBALL || spell == Spell::FIREBLAST || spell == Spell::LIGHTNINGBOLT
             || spell == Spell::CHAINLIGHTNING || spell == Spell::ELEMENTALSTORM ) {
            foundAbility = abilities.find( MonsterAbility( MonsterAbilityType::ELEMENTAL_SPELL_IMMUNITY ) );
            if ( foundAbility != abilities.end() ) {
                return 100;
            }
        }

        return 0;
    }
}
