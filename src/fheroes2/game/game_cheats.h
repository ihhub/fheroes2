#pragma once

#include <string>
#include "localevent.h"

namespace GameCheats
{
    void enableCheats( bool enable );
    bool cheatsEnabled();

    // Append a key press to internal buffer and perform cheat detection.
    void onKeyPressed( const fheroes2::Key key, const int32_t modifier );

    // Reset internal typed buffer.
    void reset();
}

