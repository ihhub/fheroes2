#ifndef H2BIN_FRM_H
#define H2BIN_FRM_H

#include "settings.h"

#include <map>
#include <vector>

namespace BIN
{

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
    
    const size_t CORRECT_FRM_LENGTH = 821;

    const char * GetFilename( int );
    std::map<int, std::vector<int> > convertBinToMap( const std::vector<u8> & );
    bool animationExists( const std::map<int, std::vector<int> > & animMap, H2_FRAME_SEQUENCE id );
    std::vector<int> analyzeGetUnitsWithoutAnim( const std::map<int, std::map<int, std::vector<int> > > & allAnimMap, H2_FRAME_SEQUENCE id, bool inverse = false );
    std::vector<int> analyzeGetUnitsWithoutAnim( const std::map<int, std::map<int, std::vector<int> > > & allAnimMap, H2_FRAME_SEQUENCE one, H2_FRAME_SEQUENCE two, bool inverse = false );
    // int FromString( const char * );
}


#endif