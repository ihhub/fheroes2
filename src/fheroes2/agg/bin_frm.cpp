#include "bin_frm.h"
#include "settings.h"
#include "monster.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

namespace BIN
{
    const struct
    {
        int type;
        const char * string;
    } bin_file_map[] = {
        {UNKNOWN, "UNKNOWN"},     {ARCHER, "ARCHRFRM.BIN"},     {RANGER, "ARCHRFRM.BIN"},      {BOAR, "BOAR_FRM.BIN"},        {CENTAUR, "CENTRFRM.BIN"},    {CHAMPION, "CVLR2FRM.BIN"},  {CAVALRY, "CVLRYFRM.BIN"}, 
        {CYCLOPS, "CYCLOFRM.BIN"},{BONE_DRAGON, "DRABNFRM.BIN"},{BLACK_DRAGON, "DRAGBFRM.BIN"},{GREEN_DRAGON, "DRAGGFRM.BIN"},{RED_DRAGON, "DRAGRFRM.BIN"}, {DRUID, "DRUIDFRM.BIN"},   {GREATER_DRUID, "DRUIDFRM.BIN"},
        {DWARF, "DWARFFRM.BIN"},  {BATTLE_DWARF, "DWARFFRM.BIN"},{ELF, "ELF__FRM.BIN"},        {GRAND_ELF, "ELF__FRM.BIN"},   {FIRE_ELEMENT, "FELEMFRM.BIN"},{EARTH_ELEMENT, "FELEMFRM.BIN"}, {AIR_ELEMENT, "FELEMFRM.BIN"}, 
        {WATER_ELEMENT, "FELEMFRM.BIN"},{GARGOYLE, "GARGLFRM.BIN"},{GENIE, "GENIEFRM.BIN"},    {GHOST, "GHOSTFRM.BIN"},       {GOBLIN, "GOBLNFRM.BIN"},     {IRON_GOLEM, "GOLEMFRM.BIN"}, {STEEL_GOLEM, "GOLEMFRM.BIN"},
        {GRIFFIN, "GRIFFFRM.BIN"},   {HALFLING, "HALFLFRM.BIN"},{HYDRA, "HYDRAFRM.BIN"},       {LICH, "LICH_FRM.BIN"},        {POWER_LICH, "LICH_FRM.BIN"}, {MAGE, "MAGE1FRM.BIN"},    {ARCHMAGE, "MAGE1FRM.BIN"},
        {MEDUSA, "MEDUSFRM.BIN"},    {MINOTAUR, "MINOTFRM.BIN"},{MINOTAUR_KING, "MINOTFRM.BIN"},{MUMMY, "MUMMYFRM.BIN"},      {ROYAL_MUMMY, "MUMMYFRM.BIN"},{NOMAD, "NOMADFRM.BIN"},   {OGRE, "OGRE_FRM.BIN"}, 
        {OGRE_LORD, "OGRE_FRM.BIN"}, {ORC, "ORC__FRM.BIN"},     {ORC_CHIEF, "ORC__FRM.BIN"},   {PALADIN, "PALADFRM.BIN"},     {CRUSADER, "PALADFRM.BIN"},   {PEASANT, "PEAS_FRM.BIN"}, {PHOENIX, "PHOENFRM.BIN"}, 
        {PIKEMAN, "PIKMNFRM.BIN"}, {VETERAN_PIKEMAN, "PIKMNFRM.BIN"},{ROC, "ROC__FRM.BIN"},    {ROGUE, "ROGUEFRM.BIN"},       {SKELETON, "SKEL_FRM.BIN"},   {SPRITE, "SPRITFRM.BIN"},  {SWORDSMAN, "SWRDSFRM.BIN"}, 
        {MASTER_SWORDSMAN, "SWRDSFRM.BIN"},{TITAN, "TITA2FRM.BIN"},{GIANT, "TITANFRM.BIN"},    {TROLL, "TROLLFRM.BIN"},       {WAR_TROLL, "TROLLFRM.BIN"},  {UNICORN, "UNICOFRM.BIN"}, {VAMPIRE, "VAMPIFRM.BIN"}, 
        {VAMPIRE_LORD, "VAMPIFRM.BIN"}, {WOLF, "WOLF_FRM.BIN"}, {ZOMBIE, "ZOMB_FRM.BIN"},     {MUTANT_ZOMBIE, "ZOMB_FRM.BIN"}
    };

    const char * GetFilename( int bin_frm )
    {
        return Monster::monster_t::UNKNOWN < bin_frm && Monster::monster_t::LAST_VALID_MONSTER >= bin_frm ? bin_file_map[bin_frm].string : bin_file_map[Monster::monster_t::UNKNOWN].string;
    }

    std::map<int, std::vector<int> > convertBinToMap( const std::vector<u8>& data ) 
    {
        std::map<int, std::vector<int> > animationMap;
        const char invalidFrameId = static_cast<char>( 0xFF );

        if ( data.size() != BIN::CORRECT_FRM_LENGTH ) {
            DEBUG( DBG_ENGINE, DBG_WARN, " wrong/corrupted BIN FRM FILE: " << data.size() << " length" );
        }

       for ( size_t setId = 0u; setId < BIN_FRAME_SEQUENCE_END; ++setId ) {
            std::vector<int> frameSequence;
            int frameCount = 0;

            for ( size_t frameId = 0; frameId < 16; ++frameId ) {
                const char frameValue = data[277 + setId * 16 + frameId];
                if ( frameValue == invalidFrameId )
                    break;

                frameSequence.push_back( frameValue );
                ++frameCount;
            }

            int expectedFrames = static_cast<int>( data[243 + setId] ); 
            if ( frameCount != expectedFrames )
            {
                DEBUG( DBG_ENGINE, DBG_WARN, " BIN FRM wrong amount of frames for animation: " << setId + 1 << " should be " << expectedFrames << " got " << frameCount );
            }
            else if (frameCount > 0) {
                animationMap.emplace( setId, frameSequence );
            }
        }

        return animationMap;
    }

    bool animationExists( const std::map<int, std::vector<int> > & animMap, H2_FRAME_SEQUENCE id )
    {
        return animMap.find( id ) != animMap.end();
    }

    std::vector<int> analyzeGetUnitsWithoutAnim( const std::map<int, std::map<int, std::vector<int>>> & allAnimMap, H2_FRAME_SEQUENCE id, bool inverse )
    {
        std::vector<int> unitIDs;

        for ( auto it = allAnimMap.begin(); it != allAnimMap.end(); ++it ) {
            if (animationExists(it->second, id) == inverse) {
                unitIDs.push_back( it->first );
            }
        }
        return unitIDs;
    }

    std::vector<int> analyzeGetUnitsWithoutAnim( const std::map<int, std::map<int, std::vector<int> > > & allAnimMap, H2_FRAME_SEQUENCE one, H2_FRAME_SEQUENCE two, bool inverse )
    {
        std::vector<int> unitIDs;

        for ( auto it = allAnimMap.begin(); it != allAnimMap.end(); ++it ) {
            if ( (animationExists( it->second, one ) && animationExists( it->second, two )) == inverse ) {
                unitIDs.push_back( it->first );
            }
        }
        return unitIDs;
    }
}