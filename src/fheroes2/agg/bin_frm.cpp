#include "bin_frm.h"
#include "monster.h"

#include <algorithm>
#include <iostream>

namespace Bin_Info
{
    const size_t CORRECT_FRM_LENGTH = 821;

    const struct
    {
        int type;
        const char * string;
    } bin_file_map[] = {
        {Monster::UNKNOWN, "UNKNOWN"},         {Monster::ARCHER, "ARCHRFRM.BIN"},       {Monster::RANGER, "ARCHRFRM.BIN"},      {Monster::BOAR, "BOAR_FRM.BIN"},        
        {Monster::CENTAUR, "CENTRFRM.BIN"},    {Monster::CHAMPION, "CVLR2FRM.BIN"},     {Monster::CAVALRY, "CVLRYFRM.BIN"},     {Monster::CYCLOPS, "CYCLOFRM.BIN"},
        {Monster::BONE_DRAGON, "DRABNFRM.BIN"},{Monster::BLACK_DRAGON, "DRAGBFRM.BIN"}, {Monster::GREEN_DRAGON, "DRAGGFRM.BIN"},{Monster::RED_DRAGON, "DRAGRFRM.BIN"}, 
        {Monster::DRUID, "DRUIDFRM.BIN"},      {Monster::GREATER_DRUID, "DRUIDFRM.BIN"},{Monster::DWARF, "DWARFFRM.BIN"},       {Monster::BATTLE_DWARF, "DWARFFRM.BIN"},
        {Monster::ELF, "ELF__FRM.BIN"},        {Monster::GRAND_ELF, "ELF__FRM.BIN"},    {Monster::FIRE_ELEMENT, "FELEMFRM.BIN"},{Monster::EARTH_ELEMENT, "FELEMFRM.BIN"}, 
        {Monster::AIR_ELEMENT, "FELEMFRM.BIN"},{Monster::WATER_ELEMENT, "FELEMFRM.BIN"},{Monster::GARGOYLE, "GARGLFRM.BIN"},    {Monster::GENIE, "GENIEFRM.BIN"},    
        {Monster::GHOST, "GHOSTFRM.BIN"},      {Monster::GOBLIN, "GOBLNFRM.BIN"},       {Monster::IRON_GOLEM, "GOLEMFRM.BIN"},  {Monster::STEEL_GOLEM, "GOLEMFRM.BIN"},
        {Monster::GRIFFIN, "GRIFFFRM.BIN"},    {Monster::HALFLING, "HALFLFRM.BIN"},     {Monster::HYDRA, "HYDRAFRM.BIN"},       {Monster::LICH, "LICH_FRM.BIN"},        
        {Monster::POWER_LICH, "LICH_FRM.BIN"}, {Monster::MAGE, "MAGE1FRM.BIN"},         {Monster::ARCHMAGE, "MAGE1FRM.BIN"},    {Monster::MEDUSA, "MEDUSFRM.BIN"},    
        {Monster::MINOTAUR, "MINOTFRM.BIN"},   {Monster::MINOTAUR_KING, "MINOTFRM.BIN"},{Monster::MUMMY, "MUMMYFRM.BIN"},       {Monster::ROYAL_MUMMY, "MUMMYFRM.BIN"},
        {Monster::NOMAD, "NOMADFRM.BIN"},      {Monster::OGRE, "OGRE_FRM.BIN"},         {Monster::OGRE_LORD, "OGRE_FRM.BIN"},   {Monster::ORC, "ORC__FRM.BIN"},     
        {Monster::ORC_CHIEF, "ORC__FRM.BIN"},  {Monster::PALADIN, "PALADFRM.BIN"},      {Monster::CRUSADER, "PALADFRM.BIN"},    {Monster::PEASANT, "PEAS_FRM.BIN"}, 
        {Monster::PHOENIX, "PHOENFRM.BIN"},    {Monster::PIKEMAN, "PIKMNFRM.BIN"},      {Monster::VETERAN_PIKEMAN, "PIKMNFRM.BIN"},{Monster::ROC, "ROC__FRM.BIN"},    
        {Monster::ROGUE, "ROGUEFRM.BIN"},      {Monster::SKELETON, "SKEL_FRM.BIN"},     {Monster::SPRITE, "SPRITFRM.BIN"},      {Monster::SWORDSMAN, "SWRDSFRM.BIN"}, 
        {Monster::MASTER_SWORDSMAN, "SWRDSFRM.BIN"},{Monster::TITAN, "TITA2FRM.BIN"},   {Monster::GIANT, "TITANFRM.BIN"},       {Monster::TROLL, "TROLLFRM.BIN"},       
        {Monster::WAR_TROLL, "TROLLFRM.BIN"},  {Monster::UNICORN, "UNICOFRM.BIN"},      {Monster::VAMPIRE, "VAMPIFRM.BIN"},     {Monster::VAMPIRE_LORD, "VAMPIFRM.BIN"}, 
        {Monster::WOLF, "WOLF_FRM.BIN"},       {Monster::ZOMBIE, "ZOMB_FRM.BIN"},       {Monster::MUTANT_ZOMBIE, "ZOMB_FRM.BIN"}
    };

    const char * GetFilename( int bin_frm )
    {
        int index = Monster::UNKNOWN;

        for ( int idx = Monster::UNKNOWN; idx < Monster::LAST_VALID_MONSTER; idx++ ) {
            if ( bin_file_map[idx].type == bin_frm ) {
                index = idx;
                break;
            }
        }
        return bin_file_map[index].string;
    }

    std::map<int, std::vector<int> > convertBinToMap( const std::vector<u8> & data ) 
    {
        std::map<int, std::vector<int> > animationMap;
        const char invalidFrameId = static_cast<char>( 0xFF );

        if ( data.size() != Bin_Info::CORRECT_FRM_LENGTH ) {
            DEBUG( DBG_ENGINE, DBG_WARN, " wrong/corrupted BIN FRM FILE: " << data.size() << " length" );
        }

       for ( size_t setId = 0u; setId < SHOOT3_END + 1; ++setId ) {
            std::vector<int> frameSequence;
            int frameCount = 0;

            for ( size_t frameId = 0; frameId < 16; ++frameId ) {
                const char frameValue = data[277 + setId * 16 + frameId];
                if ( frameValue == invalidFrameId )
                    break;

                frameSequence.push_back( frameValue );
                ++frameCount;
            }

            const int expectedFrames = static_cast<int>( data[243 + setId] ); 
            if ( frameCount != expectedFrames )
            {
                DEBUG( DBG_ENGINE, DBG_WARN, " BIN FRM wrong amount of frames for animation: " << setId + 1 << " should be " << expectedFrames << " got " << frameCount );
            }
            else if ( frameCount > 0 ) {
                animationMap.emplace( setId, frameSequence );
            }
        }

        return animationMap;
    }

    bool animationExists( const std::map<int, std::vector<int> > & animMap, ORIGINAL_ANIMATION id )
    {
        return animMap.find( id ) != animMap.end();
    }
}
