/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#pragma once

#ifdef TARGET_NINTENDO_3DS

#include <cstdio>
#include <cstdlib>

#include <3ds.h>

namespace CTR
{
    bool ctr_check_dsp();
    bool ctr_is_n3ds();
    void ctr_sys_init();

    bool ctr_check_dsp()
    {
        FILE * dsp = fopen( "sdmc:/3ds/dspfirm.cdc", "r" );
        if ( dsp == nullptr ) {
            gfxInitDefault();
            errorConf error;
            errorInit( &error, ERROR_TEXT, CFG_LANGUAGE_EN );
            errorText( &error, "Cannot find DSP firmware!\n\n\"sdmc:/3ds/dspfirm.cdc\"\n\nRun \'DSP1\' at least once to\ndump your DSP firmware." );
            errorDisp( &error );
            gfxExit();
            return false;
        }
        fclose( dsp );
        return true;
    }

    bool ctr_is_n3ds()
    {
        bool isN3DS;
        Result res = APT_CheckNew3DS( &isN3DS );
        return R_SUCCEEDED( res ) && isN3DS;
    }

    void ctr_sys_init()
    {
        if ( !ctr_check_dsp() ) {
            exit( 0 );
        }

        if ( ctr_is_n3ds() ) {
            osSetSpeedupEnable( true );
        }

        romfsInit();
        atexit( []() { romfsExit(); } );

        acInit();
        atexit( []() { acExit(); } );
    }
}
#endif
