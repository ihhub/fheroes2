/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "ui_dialog.h"
#include "cursor.h"
#include "dialog.h"
#include "localevent.h"
#include "screen.h"
#include "ui_button.h"
#include "ui_text.h"

namespace
{
    const int32_t textOffsetY = 10;
}

namespace fheroes2
{
    int showMessage( const TextBase & header, const TextBase & body, const int buttons )
    {
        // setup cursor
        const CursorRestorer cursorRestorer( buttons != 0, ::Cursor::POINTER );

        const int32_t headerHeight = header.empty() ? 0 : header.height( BOXAREA_WIDTH ) + textOffsetY;

        Dialog::FrameBox box( textOffsetY + headerHeight + body.height( BOXAREA_WIDTH ), buttons != 0 );
        const Rect & pos = box.GetArea();

        Display & display = Display::instance();
        header.draw( pos.x, pos.y + textOffsetY, BOXAREA_WIDTH, display );
        body.draw( pos.x, pos.y + textOffsetY + headerHeight, BOXAREA_WIDTH, display );

        ButtonGroup group( pos, buttons );
        group.draw();

        display.render();

        int result = Dialog::ZERO;
        LocalEvent & le = LocalEvent::Get();

        while ( result == Dialog::ZERO && le.HandleEvents() ) {
            if ( !buttons && !le.MousePressRight() ) {
                break;
            }

            result = group.processEvents();
        }

        return result;
    }
}
