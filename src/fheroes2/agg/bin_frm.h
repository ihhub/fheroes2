#ifndef H2BIN_FRM_H
#define H2BIN_FRM_H

#include "gamedefs.h"
#include "monster.h"

namespace BIN
{
    enum
    {
        UNKNOWN,

        ARCHER,
        RANGER,
        BOAR,        
        CENTAUR,
        CHAMPION,
        CAVALRY,
        CYCLOPS,
        BONE_DRAGON,
        BLACK_DRAGON,
        GREEN_DRAGON,
        RED_DRAGON,
        DRUID,
        GREATER_DRUID,
        DWARF,
        BATTLE_DWARF,
        ELF,
        GRAND_ELF,
        FIRE_ELEMENT,
        EARTH_ELEMENT,
        AIR_ELEMENT,
        WATER_ELEMENT,
        GARGOYLE,
        GENIE,
        GHOST,
        GOBLIN,
        IRON_GOLEM,
        STEEL_GOLEM,
        GRIFFIN,
        HALFLING,
        HYDRA,
        LICH,
        POWER_LICH,
        MAGE,
        ARCHMAGE,
        MEDUSA,
        MINOTAUR,
        MINOTAUR_KING,
        MUMMY,
        ROYAL_MUMMY,
        NOMAD,
        OGRE,
        OGRE_LORD,
        ORC,
        ORC_CHIEF,
        PALADIN,
        CRUSADER,
        PEASANT,
        PHOENIX,
        PIKEMAN,
        VETERAN_PIKEMAN,
        ROC,
        ROGUE,
        SKELETON,
        SPRITE,
        SWORDSMAN,
        MASTER_SWORDSMAN,
        TITAN,
        GIANT,
        TROLL,
        WAR_TROLL,
        UNICORN,
        VAMPIRE,
        VAMPIRE_LORD,
        WOLF,
        ZOMBIE,
        MUTANT_ZOMBIE,

        LAST_BIN_FRM
        // INVALID
    };

    enum FRAME_SEQUENCE
    {
        MOVE_START,
        UNKNOWN1,
        MOVE_MAIN,
        EXTRA_MOVE,
        MOVE_END,
        MOVE_ONE,
        UNKNOWN2,
        STATIC,
        IDLE1,
        IDLE2,
        IDLE3,
        IDLE4,
        IDLE5,
        DEATH,
        WINCE_UP,
        WINCE_END,
        ATTACK1,
        ATTACK1_END,
        BREATH1,
        BREATH1_END,
        ATTACK2,
        ATTACK2_END,
        BREATH2,
        BREATH2_END,
        ATTACK3,
        ATTACK3_END,
        BREATH3,
        BREATH3_END,
        SHOOT1,
        SHOOT1_END,
        SHOOT2,
        SHOOT2_END,
        SHOOT3,
        SHOOT3_END,

        BIN_FRAME_SEQUENCE_END
    };

    const size_t correctLength = 821;

    const char * GetFilename( int );
    std::map<int, std::vector<int> > convertBinToMap( const std::vector<u8> & );
    //int FromString( const char * );
}


#endif