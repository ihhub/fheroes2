/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2GAMEOVER_H
#define H2GAMEOVER_H

#include <string>

namespace GameOver
{
    enum conditions_t
    {
	COND_NONE	= 0x0000,

	WINS_ALL	= 0x0001,
	WINS_TOWN	= 0x0002,
	WINS_HERO	= 0x0004,
	WINS_ARTIFACT	= 0x0008,
	WINS_SIDE	= 0x0010,
	WINS_GOLD	= 0x0020,
	WINS            = WINS_ALL | WINS_TOWN | WINS_HERO | WINS_ARTIFACT | WINS_SIDE | WINS_GOLD,

	LOSS_ALL	= 0x0100,
	LOSS_TOWN	= 0x0200,
	LOSS_HERO	= 0x0400,
	LOSS_TIME	= 0x0800,
	LOSS_STARTHERO	= 0x1000,
	LOSS            = LOSS_ALL | LOSS_TOWN | LOSS_HERO | LOSS_TIME | LOSS_STARTHERO
    };

    const char*		GetString(int);
    std::string		GetActualDescription(int);
    void		DialogWins(int);
    void		DialogLoss(int);

    class Result
    {
    public:
	static Result & Get(void);

	void		Reset(void);
	void		SetResult(int);
	int		GetResult(void) const;
	int		LocalCheckGameOver(void);

    private:
	friend StreamBase & operator<< (StreamBase &, const Result &);
	friend StreamBase & operator>> (StreamBase &, Result &);

	Result();

	int	colors;
	int	result;
	bool	continue_game;
    };

    StreamBase & operator<< (StreamBase &, const Result &);
    StreamBase & operator>> (StreamBase &, Result &);
}

#endif
