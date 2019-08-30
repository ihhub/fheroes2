/***************************************************************************
 *   Copyright (C) 2008 by Josh Matthews  <josh@joshmatthews.net>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#ifdef WITH_AUDIOCD

#include "system.h"
#include "audio_mixer.h"
#include "audio_cdrom.h"

namespace Cdrom
{
    void Open(void);
    void Close(void);
    
    static SDL_CD *cd		  = NULL;
    static int currentTrack       = -1;
    static unsigned int startTime = 0;
    static unsigned int tickLength;
    static SDL_Thread *loopThread;
    static SDL_mutex *cdLock;
    
    int LoopCheck(void *data);
}

void Cdrom::Open(void)
{
    for(int i = 0; i < SDL_CDNumDrives(); i++)
    {
        SDL_CD *drive = SDL_CDOpen(i);

        if(drive)
        {
            if(!CD_INDRIVE(SDL_CDStatus(drive)))
            {
                SDL_CDClose(drive);
                continue;
            }
            else
	    if(drive->numtracks > 1 && drive->track[0].type == SDL_DATA_TRACK)
            {
                cd = drive;
                break;
            }
        }
    }
    
    if(cd)
    {
        loopThread = SDL_CreateThread(&LoopCheck, NULL);
        cdLock = SDL_CreateMutex();
    }

    ERROR(cd ? "found CD audio device." : "no CDROM devices available.");
}

void Cdrom::Close(void)
{
    if(cd)
    {
        SDL_CDStop(cd);
        SDL_KillThread(loopThread);
        SDL_DestroyMutex(cdLock);
	SDL_CDClose(cd);
	cd = NULL;
    }
}

bool Cdrom::isValid(void)
{
    return cd;
}

int Cdrom::LoopCheck(void *data)
{
    while(1)
    {
        SDL_Delay(5000);
        SDL_LockMutex(cdLock);
        if(startTime && SDL_GetTicks() - startTime > tickLength)
            Play(currentTrack, true, true);
        SDL_UnlockMutex(cdLock);
    }
    return 0;
}

void Cdrom::Play(const u8 track, bool loop, bool force)
{
    if(Mixer::isValid() && cd)
    {
        SDL_LockMutex(cdLock);
        
        if(currentTrack != track || force)
        {
            if(SDL_CDPlayTracks(cd, track, 0, 1, 0) < 0)
                ERROR("Couldn't play track " << static_cast<int>(track));
            
            currentTrack = track;
            if(loop)
            {
              tickLength = (unsigned int)((cd->track[track].length / CD_FPS) * 0.01f);
              startTime = SDL_GetTicks();
            }
            else startTime = 0;

            if(SDL_CDStatus(cd) != CD_PLAYING)
                ERROR("CD is not playing" << SDL_GetError());
        }
        
        SDL_UnlockMutex(cdLock);
    }
}

void Cdrom::Pause(void)
{
    if(cd) SDL_CDPause(cd);
}
#endif
