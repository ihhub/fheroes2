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

#ifndef H2INTERFACE_BUTTONS_H
#define H2INTERFACE_BUTTONS_H

#include "button.h"
#include "interface_border.h"

namespace Interface
{
    class Basic;

    class ButtonsArea : public BorderWindow
    {
    public:
	ButtonsArea(Basic &);

	void		SetPos(s32, s32);
	void		SavePosition(void);
	void		SetRedraw(void) const;

	void		Redraw(void);
	int		QueueEventProcessing(void);

    private:
	Basic & interface;

	Button	buttonNextHero;
	Button	buttonMovement;
	Button	buttonKingdom;
	Button	buttonSpell;
	Button	buttonEndTur;
	Button	buttonAdventure;
	Button	buttonFile;
	Button	buttonSystem;
    };
}

#endif
