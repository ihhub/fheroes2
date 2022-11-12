/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cassert>
#include <cstdint>

#include "image.h"
#include "interface_border.h"
#include "interface_list.h"
#include "math_base.h"

class Castle;
class Heroes;

enum icons_t
{
    ICON_HEROES = 0x01,
    ICON_CASTLES = 0x02,
    ICON_ANY = ICON_HEROES | ICON_CASTLES
};

namespace Interface
{
    class Basic;

    using HEROES = Heroes *;
    using CASTLE = Castle *;

    class IconsBar
    {
    public:
        IconsBar( const int32_t count, const fheroes2::Image & sf )
            : marker( sf )
            , iconsCount( count )
            , show( true )
        {
            assert( count >= 0 );
        }

        IconsBar( const IconsBar & ) = delete;

        virtual ~IconsBar() = default;

        IconsBar & operator=( const IconsBar & ) = delete;

        void SetShow( bool f )
        {
            show = f;
        }

        void redrawBackground( fheroes2::Image & output, const fheroes2::Point & offset, const int32_t validItemCount ) const;

        void SetIconsCount( const int32_t c )
        {
            iconsCount = c;
        }

        int32_t getIconsCount() const
        {
            return iconsCount;
        }

        static int32_t GetItemWidth();
        static int32_t GetItemHeight();
        static bool IsVisible();

    protected:
        const fheroes2::Image & marker;
        int32_t iconsCount;
        bool show;
    };

    void RedrawHeroesIcon( const Heroes &, int32_t, int32_t );
    void RedrawCastleIcon( const Castle &, int32_t, int32_t );

    class HeroesIcons final : public Interface::ListBox<HEROES>, public IconsBar
    {
    public:
        HeroesIcons( const int32_t count, const fheroes2::Image & sf )
            : IconsBar( count, sf )
        {}

        HeroesIcons( const HeroesIcons & ) = delete;

        ~HeroesIcons() override = default;

        HeroesIcons & operator=( const HeroesIcons & ) = delete;

        void SetPos( int32_t px, int32_t py );
        void SetShow( bool );

    private:
        using Interface::ListBox<HEROES>::ActionListDoubleClick;
        using Interface::ListBox<HEROES>::ActionListSingleClick;
        using Interface::ListBox<HEROES>::ActionListPressRight;

        void ActionCurrentUp() override;
        void ActionCurrentDn() override;
        void ActionListDoubleClick( HEROES & ) override;
        void ActionListSingleClick( HEROES & ) override;
        void ActionListPressRight( HEROES & ) override;
        void RedrawItem( const HEROES & item, int32_t ox, int32_t oy, bool current ) override;
        void RedrawBackground( const fheroes2::Point & ) override;

        fheroes2::Point _topLeftCorner;
    };

    class CastleIcons final : public Interface::ListBox<CASTLE>, public IconsBar
    {
    public:
        CastleIcons( const int32_t count, const fheroes2::Image & sf )
            : IconsBar( count, sf )
        {}

        CastleIcons( const CastleIcons & ) = delete;

        ~CastleIcons() override = default;

        CastleIcons & operator=( const CastleIcons & ) = delete;

        void SetPos( int32_t px, int32_t py );
        void SetShow( bool );

    private:
        using Interface::ListBox<CASTLE>::ActionListDoubleClick;
        using Interface::ListBox<CASTLE>::ActionListSingleClick;
        using Interface::ListBox<CASTLE>::ActionListPressRight;

        void ActionCurrentUp() override;
        void ActionCurrentDn() override;
        void ActionListDoubleClick( CASTLE & ) override;
        void ActionListSingleClick( CASTLE & ) override;
        void ActionListPressRight( CASTLE & ) override;
        void RedrawItem( const CASTLE & item, int32_t ox, int32_t oy, bool current ) override;
        void RedrawBackground( const fheroes2::Point & ) override;

        fheroes2::Point _topLeftCorner;
    };

    class IconsPanel final : public BorderWindow
    {
    public:
        explicit IconsPanel( Basic & basic );
        IconsPanel( const IconsPanel & ) = delete;

        ~IconsPanel() override = default;

        IconsPanel & operator=( const IconsPanel & ) = delete;

        void SetPos( int32_t ox, int32_t oy ) override;
        void SavePosition() override;
        void SetRedraw() const;
        void SetRedraw( const icons_t type ) const;

        void QueueEventProcessing();

        void Select( Heroes * const );
        void Select( Castle * const );

        bool IsSelected( const icons_t type ) const;
        void ResetIcons( const icons_t type );
        void HideIcons( const icons_t type );
        void ShowIcons( const icons_t type );

    private:
        friend Basic;

        // Do not call these methods directly, use Interface::Basic::Redraw() instead
        // to avoid issues in the "no interface" mode
        void Redraw();
        void RedrawIcons( const icons_t type );

        Basic & interface;

        fheroes2::Image sfMarker;

        CastleIcons castleIcons;
        HeroesIcons heroesIcons;
    };
}

#endif
