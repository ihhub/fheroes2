#ifndef H2BIN_FRM_H
#define H2BIN_FRM_H

#include "gamedefs.h"
#include "monster.h"

namespace BIN
{
    enum MonsterType
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

    enum H2_FRAME_SEQUENCE
    {
        MOVE_START,  // START & END is only used by flyers BUT also Skeleton & Swordsman, however MOVE_ONE matches them perfectly
        UNKNOWN_SEQ1,
        MOVE_MAIN,   // All units have this
        EXTRA_MOVE,  // Cavalry & wolf, ignore for now
        MOVE_END,    
        MOVE_ONE,    // LICH and POWER_LICH doesn't have this
        UNKNOWN_SEQ2,
        STATIC,      // Frame 1
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

    enum ATTACK_DIRECTION
    {
        TOP,
        FRONT,
        BOTTOM,
        DIRECTION_END
    };
    
    const size_t correctLength = 821;

    const char * GetFilename( int );
    std::map<int, std::vector<int> > convertBinToMap( const std::vector<u8> & );
    bool animationExists( const std::map<int, std::vector<int> > & animMap, H2_FRAME_SEQUENCE id );
    std::vector<int> analyzeGetUnitsWithoutAnim( const std::map<int, std::map<int, std::vector<int> > > & allAnimMap, H2_FRAME_SEQUENCE id, bool inverse = false );
    std::vector<int> analyzeGetUnitsWithoutAnim( const std::map<int, std::map<int, std::vector<int> > > & allAnimMap, H2_FRAME_SEQUENCE one, H2_FRAME_SEQUENCE two, bool inverse = false );
    // int FromString( const char * );

    struct startEndAnim_t
    {
        std::vector<int> start;
        std::vector<int> end;
    };

    class AnimationSequence
    {
    public:
        AnimationSequence( std::map<int, std::vector<int> > & animMap, MonsterType id );
        ~AnimationSequence();

    private:
        MonsterType _type;
        int _staticFrame;
        std::vector<int> _quickMove;
        std::vector<int> _loopMove;
        startEndAnim_t _moveModes;
        std::vector<int> _wince;
        std::vector<int> _death;
        startEndAnim_t _melee[ATTACK_DIRECTION::DIRECTION_END];
        startEndAnim_t _ranged[ATTACK_DIRECTION::DIRECTION_END];
        std::vector<std::vector<int> > _idle;

        int _frameDelay[4]; // TODO: extract and find if it's useful later

        bool appendFrames( std::map<int, std::vector<int> > & animMap, std::vector<int> & target, int animID, bool critical = false );
    };
}


#endif