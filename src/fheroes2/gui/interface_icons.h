/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

enum HeroesCastlesIcons : uint8_t
{
    ICON_HEROES = 0x01,
    ICON_CASTLES = 0x02,
    ICON_ANY = ICON_HEROES | ICON_CASTLES
};

namespace Interface
{
    class AdventureMap;

    using HEROES = Heroes *;
    using CASTLE = Castle *;

    class IconsBar
    {
    public:
        IconsBar( const int32_t count, const fheroes2::Image & markerImage )
            : _marker( markerImage )
            , _iconsCount( count )
        {
            assert( count >= 0 );
        }

        IconsBar( const IconsBar & ) = delete;

        virtual ~IconsBar() = default;

        IconsBar & operator=( const IconsBar & ) = delete;

        void setShow( bool show )
        {
            _show = show;
        }

        void redrawBackground( fheroes2::Image & output, const fheroes2::Point & offset, const int32_t validItemCount ) const;

        void setIconsCount( const int32_t count )
        {
            _iconsCount = count;
        }

        int32_t getIconsCount() const
        {
            return _iconsCount;
        }

        static int32_t getItemWidth()
        {
            return 46;
        }
        static int32_t getItemHeight()
        {
            return 22;
        }

        static bool isVisible();

    protected:
        const fheroes2::Image & _marker;
        int32_t _iconsCount;
        bool _show{ true };
    };

    void redrawHeroesIcon( const Heroes & hero, const int32_t posX, const int32_t posY );
    void redrawCastleIcon( const Castle & castle, const int32_t posX, const int32_t posY );

    class HeroesIcons final : public Interface::ListBox<HEROES>, public IconsBar
    {
    public:
        HeroesIcons( const int32_t count, const fheroes2::Image & markerImage )
            : IconsBar( count, markerImage )
        {
            // Do nothing.
        }

        HeroesIcons( const HeroesIcons & ) = delete;

        ~HeroesIcons() override = default;

        HeroesIcons & operator=( const HeroesIcons & ) = delete;

        void setPos( const int32_t px, const int32_t py );
        void showIcons( const bool show );

    private:
        using Interface::ListBox<HEROES>::ActionListDoubleClick;
        using Interface::ListBox<HEROES>::ActionListSingleClick;
        using Interface::ListBox<HEROES>::ActionListPressRight;

        void ActionCurrentUp() override;
        void ActionCurrentDn() override;
        void ActionListDoubleClick( HEROES & item ) override;
        void ActionListSingleClick( HEROES & item ) override;
        void ActionListPressRight( HEROES & item ) override;
        void RedrawItem( const HEROES & item, int32_t ox, int32_t oy, bool current ) override;
        void RedrawBackground( const fheroes2::Point & pos ) override;

        fheroes2::Point _topLeftCorner;
    };

    class CastleIcons final : public Interface::ListBox<CASTLE>, public IconsBar
    {
    public:
        CastleIcons( const int32_t count, const fheroes2::Image & markerImage )
            : IconsBar( count, markerImage )
        {
            // Do nothing.
        }

        CastleIcons( const CastleIcons & ) = delete;

        ~CastleIcons() override = default;

        CastleIcons & operator=( const CastleIcons & ) = delete;

        void setPos( const int32_t px, const int32_t py );
        void showIcons( const bool show );

    private:
        using Interface::ListBox<CASTLE>::ActionListDoubleClick;
        using Interface::ListBox<CASTLE>::ActionListSingleClick;
        using Interface::ListBox<CASTLE>::ActionListPressRight;

        void ActionCurrentUp() override;
        void ActionCurrentDn() override;
        void ActionListDoubleClick( CASTLE & item ) override;
        void ActionListSingleClick( CASTLE & item ) override;
        void ActionListPressRight( CASTLE & item ) override;
        void RedrawItem( const CASTLE & item, int32_t ox, int32_t oy, bool current ) override;
        void RedrawBackground( const fheroes2::Point & pos ) override;

        fheroes2::Point _topLeftCorner;
    };

    class IconsPanel final : public BorderWindow
    {
    public:
        explicit IconsPanel( AdventureMap & interface );
        IconsPanel( const IconsPanel & ) = delete;

        ~IconsPanel() override = default;

        IconsPanel & operator=( const IconsPanel & ) = delete;

        void SetPos( int32_t x, int32_t y ) override;
        void SavePosition() override;

        void setRedraw() const
        {
            setRedraw( ICON_ANY );
        }

        void setRedraw( const HeroesCastlesIcons type ) const;

        void queueEventProcessing();

        void select( Heroes * const hero );
        void select( Castle * const castle );

        bool isSelected( const HeroesCastlesIcons type ) const;
        void resetIcons( const HeroesCastlesIcons type );
        void hideIcons( const HeroesCastlesIcons type );
        void showIcons( const HeroesCastlesIcons type );

        // Do not call this method directly, use Interface::AdventureMap::redraw() instead to avoid issues in the "no interface" mode.
        // The name of this method starts from _ on purpose to do not mix with other public methods.
        void _redraw();
        // The name of this method starts from _ on purpose to do not mix with other public methods.
        void _redrawIcons( const HeroesCastlesIcons type );

    private:
        AdventureMap & _interface;

        fheroes2::Image _sfMarker;

        CastleIcons _castleIcons{ 4, _sfMarker };
        HeroesIcons _heroesIcons{ 4, _sfMarker };
    };
}

#endif
