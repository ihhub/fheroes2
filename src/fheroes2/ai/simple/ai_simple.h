/********************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>               *
 *   All rights reserved.                                                       *
 *                                                                              *
 *   Part of the Free Heroes2 Engine:                                           *
 *   http://sourceforge.net/projects/fheroes2                                   *
 *                                                                              *
 *   Redistribution and use in source and binary forms, with or without         *
 *   modification, are permitted provided that the following conditions         *
 *   are met:                                                                   *
 *   - Redistributions may not be sold, nor may they be used in a               *
 *     commercial product or activity.                                          *
 *   - Redistributions of source code and/or in binary form must reproduce      *
 *     the above copyright notice, this list of conditions and the              *
 *     following disclaimer in the documentation and/or other materials         *
 *     provided with the distribution.                                          *
 *                                                                              *
 * THIS SOFTWARE IS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,   *
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS    *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT     *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,        *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;  *
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,     *
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE         *
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,            *
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           *
 *******************************************************************************/

#ifndef H2AI_SIMPLE_H
#define H2AI_SIMPLE_H

#include <map>
#include <list>
#include <vector>

#include "pairs.h"
#include "ai.h"

struct IndexObjectMap : public std::map<s32, int>
{
    void DumpObjects(const IndexDistance & id);
};

struct AIKingdom
{
    AIKingdom() : capital(NULL) {};
    void Reset(void);

    Castle*         capital;
    IndexObjectMap  scans;
};

class AIKingdoms : public std::vector<AIKingdom>
{
public:
    static AIKingdom & Get(int color);
    static void Reset(void);

private:
    static AIKingdoms & Get(void);
    AIKingdoms() : std::vector<AIKingdom>(KINGDOMMAX + 1) {};
};

struct Queue : public std::list<s32>
{
    bool isPresent(s32) const;
};

struct AIHero
{
    AIHero() : primary_target(-1), fix_loop(0) {};

    void ClearTasks(void) { sheduled_visit.clear(); }
    void Reset(void);

    Queue           sheduled_visit;
    s32             primary_target;
    u32             fix_loop;
};

struct AIHeroes : public std::vector<AIHero>
{
public:
    static AIHero & Get(const Heroes &);
    static void Reset(void);

private:
    static AIHeroes & Get(void);
    AIHeroes() : std::vector<AIHero>(HEROESMAXCOUNT + 2) {};
};

#endif
