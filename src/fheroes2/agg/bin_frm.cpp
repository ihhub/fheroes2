#include "bin_frm.h"
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
}
/*
ARCHER, BOAR, CENTAUR, CHAMPION, CAVALRY, CYCLOPS, BONE_DRAGON, BLACK_DRAGON, GREEN_DRAGON, RED_DRAGON, DRUID, DWARF, ELF, FIRE_ELEMENT, GARGOYLE, GENIE, GHOST, GOBLIN,
    IRON_GOLEM, GRIFFIN, HALFLING, HYDRA, LICH, MAGE, MEDUSA, MINOTAUR, MUMMY, NOMAD, OGRE, ORC, PALADIN, PEASANT, PHOENIX, PIKEMAN, ROC, ROGUE, SKELETON, SPRITE,
    SWORDSMAN, TITAN, GIANT, TROLL, UNICORN, VAMPIRE, WOLF, ZOMBIE,
    */

const char * BIN::GetFilename( int bin_frm )
{
    return UNKNOWN < bin_frm && LAST_BIN_FRM >= bin_frm ? bin_file_map[bin_frm].string : bin_file_map[UNKNOWN].string;
}

std::map<int, std::vector<int> > BIN::convertBinToMap( const std::vector<u8>& data ) {
    std::map<int, std::vector<int> > animationMap;
    const char invalidFrameId = static_cast<char>( 0xFF );

    if ( data.size() != BIN::correctLength ) {
        std::cout << "OH NO!!";
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

        if ( frameCount != static_cast<int>( data[243 + setId] ) ) {
            std::cout << "WARNING: In "
                      << " file number of for animation frames for animation " << setId + 1 << " should be " << static_cast<int>( data[243 + setId] )
                      << " while found number is " << frameCount << std::endl;
        }
        else {
            animationMap.emplace( setId, frameSequence );
        }
    }

    return animationMap;
}