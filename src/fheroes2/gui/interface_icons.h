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

#ifndef H2INTERFACE_ICONS_H
#define H2INTERFACE_ICONS_H

#include "interface_border.h"
#include "interface_list.h"

enum icons_t
{
    ICON_HEROES = 0x01,
    ICON_CASTLES = 0x02,
    ICON_ANY = ICON_HEROES | ICON_CASTLES
};

namespace Interface
{
    using HEROES = Heroes *;
    using CASTLE = Castle *;

    class IconsBar
    {
    public:
        IconsBar( u32 count, const fheroes2::Image & sf )
            : marker( sf )
            , iconsCount( count )
            , show( true )
        {}

        void SetShow( bool f )
        {
            show = f;
        }

        bool IsShow( void ) const
        {
            return show;
        }

        void RedrawBackground( const fheroes2::Point & ) const;

        void SetIconsCount( u32 c )
        {
            iconsCount = c;
        }

        static u32 GetItemWidth( void );
        static u32 GetItemHeight( void );
        static bool IsVisible( void );

    protected:
        const fheroes2::Image & marker;
        u32 iconsCount;
        bool show;
    };

    void RedrawHeroesIcon( const Heroes &, s32, s32 );
    void RedrawCastleIcon( const Castle &, s32, s32 );

    class HeroesIcons : public Interface::ListBox<HEROES>, public IconsBar
    {
    public:
        HeroesIcons( u32 count, const fheroes2::Image & sf )
            : IconsBar( count, sf )
        {}

        void SetPos( s32, s32 );
        void SetShow( bool );

    protected:
        void ActionCurrentUp( void ) override;
        void ActionCurrentDn( void ) override;
        void ActionListDoubleClick( HEROES & ) override;
        void ActionListSingleClick( HEROES & ) override;
        void ActionListPressRight( HEROES & ) override;
        void RedrawItem( const HEROES &, s32 ox, s32 oy, bool current ) override;
        void RedrawBackground( const fheroes2::Point & ) override;

    private:
        fheroes2::Point _topLeftCorner;
    };

    class CastleIcons : public Interface::ListBox<CASTLE>, public IconsBar
    {
    public:
        CastleIcons( u32 count, const fheroes2::Image & sf )
            : IconsBar( count, sf )
        {}

        void SetPos( s32, s32 );
        void SetShow( bool );

    protected:
        void ActionCurrentUp( void ) override;
        void ActionCurrentDn( void ) override;
        void ActionListDoubleClick( CASTLE & ) override;
        void ActionListSingleClick( CASTLE & ) override;
        void ActionListPressRight( CASTLE & ) override;
        void RedrawItem( const CASTLE &, s32 ox, s32 oy, bool current ) override;
        void RedrawBackground( const fheroes2::Point & ) override;

    private:
        fheroes2::Point _topLeftCorner;
    };

    class Basic;

    class IconsPanel : public BorderWindow
    {
    public:
        explicit IconsPanel( Basic & );

        void SetPos( s32, s32 ) override;
        void SavePosition( void ) override;
        void SetRedraw( void ) const;
        void SetRedraw( icons_t ) const;

        void Redraw( void );
        void QueueEventProcessing( void );

        void Select( Heroes * const );
        void Select( Castle * const );

        bool IsSelected( icons_t ) const;
        void ResetIcons( icons_t = ICON_ANY );
        void HideIcons( icons_t = ICON_ANY );
        void ShowIcons( icons_t = ICON_ANY );
        void RedrawIcons( icons_t = ICON_ANY );
        void SetCurrentVisible( void );

    private:
        Basic & interface;

        fheroes2::Image sfMarker;

        CastleIcons castleIcons;
        HeroesIcons heroesIcons;
    };
}

#endif
