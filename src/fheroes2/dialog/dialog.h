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
#ifndef H2DIALOG_H
#define H2DIALOG_H

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "game_mode.h"
#include "gamedefs.h"
#include "image.h"

#define SHADOWWIDTH 16
#define BOXAREA_WIDTH 244

class Castle;
class Kingdom;
class HeroBase;
class Heroes;
class Monster;
class Troop;
struct CapturedObject;

namespace Skill
{
    class Secondary;
}

namespace Maps
{
    class Tiles;
}

namespace Dialog
{
    enum
    {
        ZERO = 0x0000,
        YES = 0x0001,
        OK = 0x0002,
        NO = 0x0004,
        CANCEL = 0x0008,
        DISMISS = 0x0010,
        UPGRADE = 0x0020,
        MAX = 0x0040,
        PREV = 0x0080,
        NEXT = 0x0100,

        WORLD = 0x0200,
        PUZZLE = 0x0400,
        INFO = 0x0800,
        DIG = 0x1000,

        UPGRADE_DISABLE = MAX,

        READONLY = 0x2000,
        BUTTONS = ( YES | OK | NO | CANCEL )
    };

    int AdventureOptions( bool enabledig );
    fheroes2::GameMode FileOptions();
    std::string SelectFileLoad();
    std::string SelectFileSave();
    // show info cell maps
    void QuickInfo( const Maps::Tiles & tile, const bool ignoreHeroOnTile = false );
    void QuickInfo( const Castle & castle, const fheroes2::Rect & activeArea, const fheroes2::Point & position = fheroes2::Point() );
    void QuickInfo( const HeroBase & hero, const fheroes2::Rect & activeArea, const fheroes2::Point & position = fheroes2::Point() );
    int Message( const std::string &, const std::string &, int ft, int buttons = 0 /* buttons: OK : CANCEL : OK|CANCEL : YES|NO */ );
    void ExtSettings( bool );
    int LevelUpSelectSkill( const std::string & name, const int primarySkillType, const Skill::Secondary & sec1, const Skill::Secondary & sec2, Heroes & hero );
    bool SelectGoldOrExp( const std::string &, const std::string &, uint32_t gold, uint32_t expr, const Heroes & );
    int SelectSkillFromArena();
    bool SelectCount( const std::string & header, uint32_t min, uint32_t max, uint32_t & cur, int step = 1 );
    bool InputString( const std::string & header, std::string & result, const std::string & title = std::string(), const size_t charLimit = 0 );
    Troop RecruitMonster( const Monster & monster0, uint32_t available, const bool allowDowngradedMonster, const int32_t windowOffsetY );
    void DwellingInfo( const Monster &, uint32_t available );
    bool SetGuardian( Heroes &, Troop &, CapturedObject &, bool readonly );
    int ArmyInfo( const Troop & troop, int flags, bool isReflected = false );
    int ArmyJoinFree( const std::string&, const std::string&, const Troop &, Heroes & );
    int ArmyJoinWithCost( const Troop &, uint32_t join, uint32_t gold, Heroes & );
    int ArmySplitTroop( uint32_t freeSlots, const uint32_t redistributeMax, uint32_t & redistributeCount, bool & useFastSplit );
    void Marketplace( Kingdom & kingdom, bool fromTradingPost );
    void MakeGiftResource( Kingdom & kingdom );
    int BuyBoat( bool enable );
    void ThievesGuild( bool oracle );
    void GameInfo();

    class NonFixedFrameBox
    {
    public:
        explicit NonFixedFrameBox( int height = 0, int startYPos = -1, bool showButtons = false );
        virtual ~NonFixedFrameBox();

        const fheroes2::Rect & GetArea() const
        {
            return area;
        }

        void redraw();

    protected:
        std::unique_ptr<fheroes2::ImageRestorer> _restorer;
        fheroes2::Rect area;

    private:
        fheroes2::Point _position;
        uint32_t _middleFragmentCount;
        int32_t _middleFragmentHeight;
    };

    class FrameBox : public NonFixedFrameBox
    {
    public:
        FrameBox( int height, bool buttons = false );
        ~FrameBox() override = default;
    };

    class FrameBorder
    {
    public:
        explicit FrameBorder( int v = BORDERWIDTH );
        explicit FrameBorder( const fheroes2::Size & );
        FrameBorder( const fheroes2::Size &, const fheroes2::Image & );

        int BorderWidth() const;
        int BorderHeight() const;
        void SetPosition( int32_t posx, int32_t posy, int32_t encw, int32_t ench );

        bool isValid() const;
        const fheroes2::Rect & GetRect() const;
        const fheroes2::Rect & GetArea() const;
        const fheroes2::Rect & GetTop() const;

        static void RenderRegular( const fheroes2::Rect & dstrt );
        static void RenderOther( const fheroes2::Image &, const fheroes2::Rect & );

    protected:
        fheroes2::ImageRestorer restorer;

    private:
        fheroes2::Rect rect;
        fheroes2::Rect area;
        fheroes2::Rect top;
        int border;
    };
}

#endif
