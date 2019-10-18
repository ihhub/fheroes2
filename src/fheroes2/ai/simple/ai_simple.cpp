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

#include <algorithm>
#include "heroes.h"
#include "castle.h"
#include "ai_simple.h"

const char* AI::Type(void)
{
    return "simple";
}

const char* AI::License(void)
{
    return "Non-Commercial";
}

void AI::HeroesAdd(const Heroes &)
{
}

void AI::HeroesRemove(const Heroes &)
{
}

void AI::CastleAdd(const Castle &)
{
}

void AI::CastleRemove(const Castle & castle)
{
    AIKingdom & ai = AIKingdoms::Get(castle.GetColor());

    if(ai.capital == &castle)
    {
        ai.capital->ResetModes(Castle::CAPITAL);
        ai.capital = NULL;
    }
}

void AI::Init(void)
{
    AIKingdoms::Reset();
    AIHeroes::Reset();
}

bool Queue::isPresent(s32 index) const
{
    return end() != std::find(begin(), end(), index);
}
