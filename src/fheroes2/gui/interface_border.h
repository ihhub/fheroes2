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

#ifndef H2INTERFACE_BORDER_H
#define H2INTERFACE_BORDER_H

#include "gamedefs.h"
#include "dialog.h"

namespace Interface
{
    void GameBorderRedraw(void);

    class BorderWindow
    {
    public:
	BorderWindow(const Rect &);
	virtual ~BorderWindow(){}

        virtual void	SetPos(s32, s32) = 0;
	virtual void	SavePosition(void) = 0;

        void		Redraw(void);
        bool		QueueEventProcessing(void);

        const Rect &	GetArea(void) const;
	const Rect &	GetRect(void) const;

    protected:
        void		SetPosition(s32, s32, u32, u32);
        void		SetPosition(s32, s32);

	Rect			area;
        Dialog::FrameBorder	border;
    };
}

#endif
