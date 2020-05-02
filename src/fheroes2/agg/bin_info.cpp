#include "agg.h"
#include "bin_info.h"
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

    std::map<int, std::map<int, std::vector<int> > > bin_frm_cache;
    std::map<int, AnimationReference> animRefs;


    bool InitBinInfo()
    {
        std::map<int, std::vector<int> > binFrameDefault = {{Bin_Info::ORIGINAL_ANIMATION::STATIC, {1}}};
        bin_frm_cache.emplace( 0, binFrameDefault );

        for ( int i = Monster::UNKNOWN; i < Monster::LAST_VALID_MONSTER; i++ )
            animRefs[i] = AnimationReference( LookupBINCache( i ), i );

        return true;
    }

    void Cleanup()
    {
        for ( std::map<int, std::map<int, std::vector<int> > >::iterator it = bin_frm_cache.begin(); it != bin_frm_cache.end(); ++it ) {
            for ( std::map<int, std::vector<int> >::iterator secIt = it->second.begin(); secIt != it->second.end(); ++secIt )
                secIt->second.clear();
            it->second.clear();
        }
    }

    bool LoadBINFRM( int bin_frm )
    {
        std::vector<u8> body = AGG::LoadBINFRM( Bin_Info::GetFilename( bin_frm ) );

        if ( body.size() ) {
            const std::map<int, std::vector<int> > animMap = Bin_Info::buildMonsterAnimInfo( body );
            if ( !animMap.empty() ) {
                bin_frm_cache[bin_frm] = animMap;
                return true;
            }
        }
        return false;
    }

    AnimationReference GetAnimationSet( int monsterID )
    {
        std::map<int, AnimationReference>::const_iterator it = animRefs.find( monsterID );
        if ( it != animRefs.end() )
            return it->second;

        return AnimationReference( LookupBINCache( Monster::UNKNOWN ), Monster::UNKNOWN );
    }

    const std::map<int, std::vector<int> > & LookupBINCache( int bin_frm )
    {
        std::map<int, std::map<int, std::vector<int> > >::iterator mapIterator = bin_frm_cache.find( bin_frm );
        if ( mapIterator == bin_frm_cache.end() ) {
            if ( LoadBINFRM( bin_frm ) ) {
                mapIterator = bin_frm_cache.find( bin_frm );
            }
            else {
                // fall back to unknown if missing data
                DEBUG( DBG_ENGINE, DBG_WARN, "missing BIN FRM data: " << Bin_Info::GetFilename( bin_frm ) << ", index: " << bin_frm );
                mapIterator = bin_frm_cache.find( 0 );
            }
        }
        return mapIterator->second;
    }

    const std::map<int, std::map<int, std::vector<int> > > & LookupBINCache()
    {
        return bin_frm_cache;
    }


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

    std::map<int, std::vector<int> > buildMonsterAnimInfo( const std::vector<u8> & data )
    {
        MonsterAnimInfo monster_info;

        std::map<int, std::vector<int> > animationMap;
        const char invalidFrameId = static_cast<char>( 0xFF );

        // sanity check
        if ( sizeof( MonsterAnimInfo ) != Bin_Info::CORRECT_FRM_LENGTH ) {
            DEBUG( DBG_ENGINE, DBG_WARN, "Size of MonsterAnimInfo does not match expected length: " << sizeof( MonsterAnimInfo ) );
        }

        if ( data.size() != sizeof( MonsterAnimInfo ) ) {
            DEBUG( DBG_ENGINE, DBG_WARN, " wrong/corrupted BIN FRM FILE: " << data.size() << " length" );
        }

        // populate fields via direct copy, make sure size aligns
        std::memcpy( &monster_info, data.data(), sizeof( MonsterAnimInfo ) );

        //return monster_info;
        return std::map<int, std::vector<int> >();
    }

    bool animationExists( const std::map<int, std::vector<int> > & animMap, ORIGINAL_ANIMATION id )
    {
        return animMap.find( id ) != animMap.end();
    }
}
