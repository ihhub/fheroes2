/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>

#include "agg.h"
#include "battle_arena.h"
#include "battle_bridge.h"
#include "battle_catapult.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_interface.h"
#include "battle_tower.h"
#include "battle_troop.h"
#include "castle.h"
#include "cursor.h"
#include "engine.h"
#include "game.h"
#include "ground.h"
#include "interface_list.h"
#include "kingdom.h"
#include "pocketpc.h"
#include "race.h"
#include "settings.h"
#include "world.h"

#define ARMYORDERW 40

namespace
{
    struct LightningPoint
    {
        explicit LightningPoint( const Point & p = Point(), uint32_t thick = 1 )
            : point( p )
            , thickness( thick )
        {}

        Point point;
        uint32_t thickness;
    };

    std::vector<std::pair<LightningPoint, LightningPoint> > GenerateLightning( const Point & src, const Point & dst )
    {
        const int distance = static_cast<int>( src.distance( dst ) );
        const double angle = src.getAngle( dst );

        uint32_t iterationCount = ( distance + 50 ) / 100;
        if ( iterationCount < 3 )
            iterationCount = 3;
        if ( iterationCount > 5 )
            iterationCount = 5;

        std::vector<std::pair<LightningPoint, LightningPoint> > lines;
        lines.push_back( std::make_pair( LightningPoint( Point( 0, 0 ), 5 ), LightningPoint( Point( distance, 0 ), 3 ) ) );

        int maxOffset = distance;

        for ( int step = 0; step < iterationCount; ++step ) {
            std::vector<std::pair<LightningPoint, LightningPoint> > oldLines;
            std::swap( lines, oldLines );

            for ( size_t i = 0; i < oldLines.size(); ++i ) {
                const int diffX = ( oldLines[i].first.point.x - oldLines[i].second.point.x );
                Point middle( oldLines[i].first.point + oldLines[i].second.point );
                middle.x /= 2;
                middle.y /= 2;

                const bool isPositive = ( Rand::Get( 1, 2 ) == 1 );
                int offsetY = static_cast<int>( Rand::Get( 1, 10 ) ) * maxOffset / 100;
                if ( offsetY < 1 )
                    offsetY = 1;

                middle.y += isPositive ? offsetY : -offsetY;

                const uint32_t middleThickness = ( oldLines[i].first.thickness + oldLines[i].second.thickness ) / 2;

                const LightningPoint middlePoint( middle, middleThickness );

                lines.push_back( std::make_pair( oldLines[i].first, middlePoint ) );
                lines.push_back( std::make_pair( middlePoint, oldLines[i].second ) );

                if ( Rand::Get( 1, 4 ) == 1 ) { // 25%
                    offsetY = static_cast<int>( Rand::Get( 1, 10 ) ) * maxOffset / 100;
                    const int x = ( middle.x - oldLines[i].first.point.x ) * 0.7 + middle.x;
                    const int y = ( middle.y - oldLines[i].first.point.y ) * 0.7 + middle.y + ( isPositive ? offsetY : -offsetY );
                    lines.push_back( std::make_pair( middlePoint, LightningPoint( Point( x, y ), 1 ) ) );
                }
            }

            maxOffset /= 2;
        }

        for ( size_t i = 0; i < lines.size(); ++i ) {
            lines[i].first.point = lines[i].first.point.rotate( angle ) + src;
            lines[i].second.point = lines[i].second.point.rotate( angle ) + src;
        }

        return lines;
    }

    void RedrawLightning( const std::vector<std::pair<LightningPoint, LightningPoint> > & lightning, const RGBA & color, Surface & surface, const Rect & roi = Rect() )
    {
        for ( size_t i = 0; i < lightning.size(); ++i ) {
            const Point & first = lightning[i].first.point;
            const Point & second = lightning[i].second.point;
            const bool isHorizontal = std::abs( first.x - second.x ) >= std::abs( first.y - second.y );
            const int xOffset = isHorizontal ? 0 : 1;
            const int yOffset = isHorizontal ? 1 : 0;

            surface.DrawLine( first, second, color, roi );

            for ( uint32_t thickness = 1; thickness < lightning[i].second.thickness; ++thickness ) {
                const bool isUpper = ( ( thickness % 2 ) == 1 );
                const int offset = isUpper ? ( thickness + 1 ) / 2 : -static_cast<int>( ( thickness + 1 ) / 2 );
                const int x = xOffset * offset;
                const int y = yOffset * offset;

                surface.DrawLine( Point( first.x + x, first.y + y ), Point( second.x + x, second.y + y ), color, roi );
            }

            for ( uint32_t thickness = lightning[i].second.thickness; thickness < lightning[i].first.thickness; ++thickness ) {
                const bool isUpper = ( ( thickness % 2 ) == 1 );
                const int offset = isUpper ? ( thickness + 1 ) / 2 : -static_cast<int>( ( thickness + 1 ) / 2 );

                surface.DrawLine( Point( first.x + xOffset * offset, first.y + yOffset * offset ), second, color, roi );
            }
        }
    }
}

namespace Battle
{
    int GetIndexIndicator( const Unit & );
    int GetSwordCursorDirection( int );
    int GetDirectionFromCursorSword( u32 );
    int GetCursorFromSpell( int );

    struct CursorPosition
    {
        CursorPosition()
            : index( -1 )
        {}

        Point coord;
        s32 index;
    };

    class StatusListBox : public ::Interface::ListBox<std::string>
    {
    public:
        StatusListBox()
            : openlog( false )
        {}

        void SetPosition( u32 px, u32 py )
        {
            const u32 mx = 6;
            const u32 sw = 640;
            const u32 sh = mx * 20;
            border.SetPosition( px, py - sh - 2, sw - 30, sh - 30 );
            const Rect & area = border.GetArea();
            const u32 ax = area.x + area.w - 20;

            SetTopLeft( area );
            SetAreaMaxItems( mx );

            SetScrollButtonUp( ICN::DROPLISL, 6, 7, Point( ax, area.y ) );
            SetScrollButtonDn( ICN::DROPLISL, 8, 9, Point( ax, area.y + area.h - 20 ) );
            SetScrollSplitter( AGG::GetICN( ICN::DROPLISL, 13 ),
                               Rect( ax + 5, buttonPgUp.y + buttonPgUp.h + 3, 12, buttonPgDn.y - ( buttonPgUp.y + buttonPgUp.h ) - 6 ) );
            splitter.HideCursor();
            SetAreaItems( Rect( area.x, area.y, area.w - 10, area.h ) );
            SetListContent( messages );
            splitter.ShowCursor();
        }

        const Rect & GetArea( void ) const
        {
            return border.GetRect();
        }

        void AddMessage( const std::string & str )
        {
            messages.push_back( str );
            SetListContent( messages );
            SetCurrent( messages.size() - 1 );
        }

        void RedrawItem( const std::string & str, s32 px, s32 py, bool f )
        {
            Text text( str, Font::BIG );
            text.Blit( px, py );
        }

        void RedrawBackground( const Point & pt )
        {
            const Sprite & sp1 = AGG::GetICN( ICN::DROPLISL, 10 );
            const Sprite & sp2 = AGG::GetICN( ICN::DROPLISL, 12 );
            const Sprite & sp3 = AGG::GetICN( ICN::DROPLISL, 11 );
            const Sprite & sp4 = AGG::GetICN( ICN::TEXTBAK2, 0 );
            const u32 ax = buttonPgUp.x;
            const u32 ah = buttonPgDn.y - ( buttonPgUp.y + buttonPgUp.h );

            Dialog::FrameBorder::RenderOther( sp4, border.GetRect() );

            for ( u32 ii = 0; ii < ( ah / sp3.h() ); ++ii )
                sp3.Blit( ax, buttonPgUp.y + buttonPgUp.h + ( sp3.h() * ii ) );

            sp1.Blit( ax, buttonPgUp.y + buttonPgUp.h );
            sp2.Blit( ax, buttonPgDn.y - sp2.h() );
        }

        void ActionCurrentUp( void ) {}
        void ActionCurrentDn( void ) {}
        void ActionListDoubleClick( std::string & ) {}
        void ActionListSingleClick( std::string & ) {}
        void ActionListPressRight( std::string & ) {}

        void SetOpenLog( bool f )
        {
            openlog = f;
        }
        bool isOpenLog( void ) const
        {
            return openlog;
        }

    private:
        Dialog::FrameBorder border;
        std::vector<std::string> messages;
        bool openlog;
    };

    bool AnimateInfrequentDelay( int dl )
    {
        return Game::AnimateInfrequentDelay( dl );
    }

    int matchHeroType( const HeroBase * base )
    {
        if ( base->isCaptain() )
            return CAPTAIN;

        switch ( base->GetRace() ) {
        case Race::BARB:
            return BARBARIAN;
        case Race::SORC:
            return SORCERESS;
        case Race::WRLK:
            return WARLOCK;
        case Race::WZRD:
            return WIZARD;
        case Race::NECR:
            return NECROMANCER;
        default:
            break;
        }
        return KNIGHT;
    }

    const std::vector<int> & getHeroAnimation( const HeroBase * hero, int animation )
    {
        static std::vector<int> staticAnim;
        if ( staticAnim.empty() ) {
            staticAnim.push_back( 1 );
        }

        if ( !hero || animation == OP_STATIC )
            return staticAnim;

        const int heroType = matchHeroType( hero );
        static std::vector<int> sorrowAnim;
        if ( sorrowAnim.empty() ) {
            const int sorrowArray[9] = {2, 3, 4, 5, 4, 5, 4, 3, 2};
            sorrowAnim.insert( sorrowAnim.begin(), sorrowArray, sorrowArray + 9 );
        }

        if ( animation == OP_SORROW )
            return ( heroType == CAPTAIN ) ? staticAnim : sorrowAnim;

        static std::vector<int> heroTypeAnim[7][9];

        if ( heroTypeAnim[heroType][animation].empty() ) {
            const int sourceArray[7][9][9] = {
                //   JOY                CAST_MASS             CAST_UP               CAST_DOWN     IDLE
                {{6, 7, 8, 9, 8, 9, 8, 7, 6}, {10, 11}, {10}, {6, 12, 13}, {12, 6}, {2, 14}, {2}, {15, 16, 17}, {18, 19}}, // KNIGHT
                {{6, 7, 8, 9, 9, 8, 7, 6}, {6, 10, 11}, {10, 6}, {6, 12, 13}, {12, 6}, {6, 14}, {6}, {15, 16, 17}, {18}}, // BARBARIAN
                {{6, 7, 8, 7, 6}, {6, 7, 9}, {7, 6}, {6, 10, 11}, {10, 6}, {6, 12}, {6}, {13, 14, 15}, {16}}, // SORCERESS
                {{6, 7, 8, 9, 10, 9, 8, 7, 6}, {6, 7, 11, 12}, {11, 6}, {6, 7, 13}, {6}, {6, 14}, {6}, {15, 16}, {6}}, // WARLOCK
                {{6, 7, 8, 9, 8, 7, 6}, {6, 10, 11, 12, 13}, {12, 11, 10, 6}, {6, 14}, {6}, {6, 15}, {6}, {16, 17}, {18}}, // WIZARD
                {{6, 7, 6, 7, 6, 7}, {7, 8, 9, 10, 11}, {10, 9, 7}, {7, 12, 13, 14, 15}, {7}, {7, 12, 13, 14, 16}, {7}, {17}, {18, 19}}, // NECROMANCER
                {{1}, {2, 3, 4}, {3, 2}, {5, 6}, {5}, {5, 7}, {5}, {8, 9}, {10}} // CAPTAIN
            };

            for ( int frame = 0; frame < 9; ++frame ) {
                if ( sourceArray[heroType][animation][frame] != 0 ) {
                    heroTypeAnim[heroType][animation].push_back( sourceArray[heroType][animation][frame] );
                }
            }
        }

        return heroTypeAnim[heroType][animation];
    }
}

bool CursorAttack( u32 theme )
{
    switch ( theme ) {
    case Cursor::WAR_ARROW:
    case Cursor::WAR_BROKENARROW:
    case Cursor::SWORD_TOPRIGHT:
    case Cursor::SWORD_RIGHT:
    case Cursor::SWORD_BOTTOMRIGHT:
    case Cursor::SWORD_BOTTOMLEFT:
    case Cursor::SWORD_LEFT:
    case Cursor::SWORD_TOPLEFT:
    case Cursor::SWORD_TOP:
    case Cursor::SWORD_BOTTOM:
        return true;
    default:
        break;
    }

    return false;
}

Surface DrawHexagon( const RGBA & color )
{
    int r, l, w, h;

    if ( Settings::Get().QVGA() ) {
        r = 11;
        l = 7;
        w = CELLW2;
        h = CELLH2;
    }
    else {
        r = 22;
        l = 10;
        w = CELLW;
        h = CELLH;
    }

    Surface sf( Size( w + 1, h + 1 ), false );

    sf.DrawLine( Point( r, 0 ), Point( 0, l ), color );
    sf.DrawLine( Point( r, 0 ), Point( w, l ), color );

    sf.DrawLine( Point( 0, l + 1 ), Point( 0, h - l ), color );
    sf.DrawLine( Point( w, l + 1 ), Point( w, h - l ), color );

    sf.DrawLine( Point( r, h ), Point( 0, h - l ), color );
    sf.DrawLine( Point( r, h ), Point( w, h - l ), color );

    return sf;
}

Surface DrawHexagonShadow( int alphaValue )
{
    int l, w, h;

    if ( Settings::Get().QVGA() ) {
        l = 7;
        w = CELLW2;
        h = CELLH2;
    }
    else {
        l = 13;
        w = CELLW;
        h = CELLH;
    }

    Surface sf( Size( w, h ), 32, true );
    const RGBA shadow = RGBA( 0, 0, 0, alphaValue );
    Rect rt( 0, l - 1, w + 1, 2 * l + 3 );
    for ( int i = 0; i < w / 2; i += 2 ) {
        --rt.y;
        rt.h += 2;
        rt.x += 2;
        rt.w -= 4;
        sf.FillRect( rt, shadow );
    }

    return sf;
}

bool Battle::TargetInfo::isFinishAnimFrame( const TargetInfo & info )
{
    return info.defender && info.defender->isFinishAnimFrame();
}

int Battle::GetCursorFromSpell( int spell )
{
    switch ( spell ) {
    case Spell::SLOW:
        return Cursor::SP_SLOW;
    case Spell::CURSE:
        return Cursor::SP_CURSE;
    case Spell::CURE:
        return Cursor::SP_CURE;
    case Spell::BLESS:
        return Cursor::SP_BLESS;
    case Spell::FIREBALL:
        return Cursor::SP_FIREBALL;
    case Spell::FIREBLAST:
        return Cursor::SP_FIREBLAST;
    case Spell::TELEPORT:
        return Cursor::SP_TELEPORT;
    case Spell::RESURRECT:
        return Cursor::SP_RESURRECT;
    case Spell::HASTE:
        return Cursor::SP_HASTE;
    case Spell::SHIELD:
        return Cursor::SP_SHIELD;
    case Spell::ARMAGEDDON:
        return Cursor::SP_ARMAGEDDON;
    case Spell::ANTIMAGIC:
        return Cursor::SP_ANTIMAGIC;
    case Spell::BERSERKER:
        return Cursor::SP_BERSERKER;
    case Spell::PARALYZE:
        return Cursor::SP_PARALYZE;
    case Spell::BLIND:
        return Cursor::SP_BLIND;

    case Spell::LIGHTNINGBOLT:
        return Cursor::SP_LIGHTNINGBOLT;
    case Spell::CHAINLIGHTNING:
        return Cursor::SP_CHAINLIGHTNING;
    case Spell::ELEMENTALSTORM:
        return Cursor::SP_ELEMENTALSTORM;
    case Spell::RESURRECTTRUE:
        return Cursor::SP_RESURRECTTRUE;
    case Spell::DISPEL:
        return Cursor::SP_DISPEL;
    case Spell::HOLYWORD:
        return Cursor::SP_HOLYWORD;
    case Spell::HOLYSHOUT:
        return Cursor::SP_HOLYSHOUT;
    case Spell::METEORSHOWER:
        return Cursor::SP_METEORSHOWER;

    case Spell::ANIMATEDEAD:
        return Cursor::SP_ANIMATEDEAD;
    case Spell::MIRRORIMAGE:
        return Cursor::SP_MIRRORIMAGE;
    case Spell::BLOODLUST:
        return Cursor::SP_BLOODLUST;
    case Spell::DEATHRIPPLE:
        return Cursor::SP_DEATHRIPPLE;
    case Spell::DEATHWAVE:
        return Cursor::SP_DEATHWAVE;
    case Spell::STEELSKIN:
        return Cursor::SP_STEELSKIN;
    case Spell::STONESKIN:
        return Cursor::SP_STONESKIN;
    case Spell::DRAGONSLAYER:
        return Cursor::SP_DRAGONSLAYER;
    case Spell::EARTHQUAKE:
        return Cursor::SP_EARTHQUAKE;
    case Spell::DISRUPTINGRAY:
        return Cursor::SP_DISRUPTINGRAY;
    case Spell::COLDRING:
        return Cursor::SP_COLDRING;
    case Spell::COLDRAY:
        return Cursor::SP_COLDRAY;
    case Spell::HYPNOTIZE:
        return Cursor::SP_HYPNOTIZE;
    case Spell::ARROW:
        return Cursor::SP_ARROW;

    default:
        break;
    }
    return Cursor::WAR_NONE;
}

int Battle::GetSwordCursorDirection( int dir )
{
    switch ( dir ) {
    case BOTTOM_RIGHT:
        return Cursor::SWORD_TOPLEFT;
    case BOTTOM_LEFT:
        return Cursor::SWORD_TOPRIGHT;
    case RIGHT:
        return Cursor::SWORD_LEFT;
    case TOP_RIGHT:
        return Cursor::SWORD_BOTTOMLEFT;
    case TOP_LEFT:
        return Cursor::SWORD_BOTTOMRIGHT;
    case LEFT:
        return Cursor::SWORD_RIGHT;
    default:
        break;
    }
    return 0;
}

int Battle::GetDirectionFromCursorSword( u32 sword )
{
    switch ( sword ) {
    case Cursor::SWORD_TOPLEFT:
        return BOTTOM_RIGHT;
    case Cursor::SWORD_TOPRIGHT:
        return BOTTOM_LEFT;
    case Cursor::SWORD_LEFT:
        return RIGHT;
    case Cursor::SWORD_BOTTOMLEFT:
        return TOP_RIGHT;
    case Cursor::SWORD_BOTTOMRIGHT:
        return TOP_LEFT;
    case Cursor::SWORD_RIGHT:
        return LEFT;
    default:
        break;
    }

    return UNKNOWN;
}

Battle::OpponentSprite::OpponentSprite( const Rect & area, const HeroBase * b, bool r )
    : base( b )
    , icn( ICN::UNKNOWN )
    , reflect( r )
    , _offset( area.x, area.y )
    , _currentAnim( getHeroAnimation( b, OP_STATIC ) )
    , _animationType( OP_STATIC )
    , _idleTimer( 8000 )
{
    const bool isCaptain = b->isCaptain();
    switch ( b->GetRace() ) {
    case Race::KNGT:
        icn = isCaptain ? ICN::CMBTCAPK : ICN::CMBTHROK;
        break;
    case Race::BARB:
        icn = isCaptain ? ICN::CMBTCAPB : ICN::CMBTHROB;
        break;
    case Race::SORC:
        icn = isCaptain ? ICN::CMBTCAPS : ICN::CMBTHROS;
        break;
    case Race::WRLK:
        icn = isCaptain ? ICN::CMBTCAPW : ICN::CMBTHROW;
        break;
    case Race::WZRD:
        icn = isCaptain ? ICN::CMBTCAPZ : ICN::CMBTHROZ;
        break;
    case Race::NECR:
        icn = isCaptain ? ICN::CMBTCAPN : ICN::CMBTHRON;
        break;
    default:
        break;
    }

    const Sprite & sprite = AGG::GetICN( icn, _currentAnim.getFrame(), reflect );

    if ( reflect ) {
        pos.x = _offset.x + Display::DEFAULT_WIDTH - HERO_X_OFFSET - ( sprite.x() + sprite.w() );
        pos.y = _offset.y + RIGHT_HERO_Y_OFFSET + sprite.y();
    }
    else {
        pos.x = _offset.x + HERO_X_OFFSET + sprite.x();
        pos.y = _offset.y + LEFT_HERO_Y_OFFSET + sprite.y();
    }

    if ( isCaptain ) {
        if ( reflect )
            pos.x += CAPTAIN_X_OFFSET;
        else
            pos.x -= CAPTAIN_X_OFFSET;
        pos.y += CAPTAIN_Y_OFFSET;
    }

    pos.w = sprite.w();
    pos.h = sprite.h();
}

int Battle::OpponentSprite::GetColor( void ) const
{
    return base ? base->GetColor() : 0;
}

const HeroBase * Battle::OpponentSprite::GetHero( void ) const
{
    return base;
}

Point Battle::OpponentSprite::Offset() const
{
    return _offset;
}

void Battle::OpponentSprite::IncreaseAnimFrame( bool loop )
{
    _currentAnim.playAnimation( loop );
}

void Battle::OpponentSprite::SetAnimation( int rule )
{
    _animationType = rule;
    _currentAnim = getHeroAnimation( base, rule );
}

bool Battle::OpponentSprite::isFinishFrame( void ) const
{
    return _currentAnim.isLastFrame();
}

bool Battle::OpponentSprite::isStartFrame( void ) const
{
    return _currentAnim.isFirstFrame();
}

const Rect & Battle::OpponentSprite::GetArea( void ) const
{
    return pos;
}

Point Battle::OpponentSprite::GetCastPosition( void ) const
{
    const bool isCaptain = base->isCaptain();
    Point offset;
    switch ( base->GetRace() ) {
    case Race::KNGT:
        offset.x = isCaptain ? 0 : 13;
        offset.y = isCaptain ? 3 : -7;
        break;
    case Race::BARB:
        offset.x = isCaptain ? 0 : 16;
        offset.y = isCaptain ? 3 : -15;
        break;
    case Race::SORC:
        offset.x = isCaptain ? 0 : 11;
        offset.y = isCaptain ? 3 : -8;
        break;
    case Race::WRLK:
        offset.x = isCaptain ? 2 : 9;
        offset.y = isCaptain ? 5 : -11;
        break;
    case Race::WZRD:
        offset.x = isCaptain ? 5 : 1;
        offset.y = isCaptain ? 8 : -9;
        break;
    case Race::NECR:
        offset.x = isCaptain ? 5 : 13;
        offset.y = isCaptain ? 6 : -7;
        break;
    default:
        break;
    }

    return Point( pos.x + ( reflect ? offset.x : pos.w - offset.x ), pos.y + pos.h / 2 + offset.y );
}

void Battle::OpponentSprite::Redraw( Surface & dst, uint32_t cycleFrame ) const
{
    Sprite hero = AGG::GetICN( icn, _currentAnim.getFrame(), reflect );
    if ( !base->isCaptain() && base->GetRace() == Race::NECR ) {
        AGG::ReplaceColors( hero, PAL::GetCyclingPalette( cycleFrame ), icn, _currentAnim.getFrame(), reflect );
    }

    Point offset( _offset );
    if ( base->isCaptain() ) {
        if ( reflect )
            offset.x += CAPTAIN_X_OFFSET;
        else
            offset.x -= CAPTAIN_X_OFFSET;
        offset.y += CAPTAIN_Y_OFFSET;
    }

    if ( reflect )
        hero.Blit( offset.x + Display::DEFAULT_WIDTH - HERO_X_OFFSET - ( hero.x() + hero.w() ), offset.y + RIGHT_HERO_Y_OFFSET + hero.y(), dst );
    else
        hero.Blit( offset.x + HERO_X_OFFSET + hero.x(), offset.y + LEFT_HERO_Y_OFFSET + hero.y(), dst );
}

void Battle::OpponentSprite::Update()
{
    if ( _currentAnim.isLastFrame() ) {
        if ( _animationType != OP_STATIC ) {
            if ( _animationType != OP_CAST_MASS && _animationType != OP_CAST_UP && _animationType != OP_CAST_DOWN )
                SetAnimation( OP_STATIC );
        }
        else if ( _idleTimer.checkDelay() ) {
            SetAnimation( ( Rand::Get( 1, 3 ) < 2 ) ? OP_IDLE2 : OP_IDLE );
        }
    }
    else {
        _currentAnim.playAnimation();
    }
}

Battle::Status::Status()
    : back1( AGG::GetICN( ICN::TEXTBAR, 8 ) )
    , back2( AGG::GetICN( ICN::TEXTBAR, 9 ) )
    , listlog( NULL )
{
    Rect::w = back1.w();
    Rect::h = back1.h() + back2.h();

    bar1.Set( Settings::Get().QVGA() ? Font::SMALL : Font::BIG );
    bar2.Set( Settings::Get().QVGA() ? Font::SMALL : Font::BIG );
}

void Battle::Status::SetPosition( s32 cx, s32 cy )
{
    Rect::x = cx;
    Rect::y = cy;
}

void Battle::Status::SetMessage( const std::string & str, bool top )
{
    if ( top ) {
        bar1.Set( str );
        if ( listlog )
            listlog->AddMessage( str );
    }
    else if ( str != message ) {
        bar2.Set( str );
        message = str;
    }
}

void Battle::Status::Redraw( void )
{
    back1.Blit( x, y );
    back2.Blit( x, y + back1.h() );

    if ( bar1.Size() )
        bar1.Blit( x + ( back1.w() - bar1.w() ) / 2, y + ( Settings::Get().QVGA() ? -1 : 3 ) );
    if ( bar2.Size() )
        bar2.Blit( x + ( back2.w() - bar2.w() ) / 2, y + back1.h() + ( Settings::Get().QVGA() ? -3 : 0 ) );
}

const std::string & Battle::Status::GetMessage( void ) const
{
    return message;
}

Battle::ArmiesOrder::ArmiesOrder()
    : orders( NULL )
    , army_color2( 0 )
{
    const RGBA yellow = RGBA( 0xe0, 0xe0, 0 );
    const RGBA orange = RGBA( 0xe0, 0x48, 0 );
    const RGBA green = RGBA( 0x90, 0xc0, 0 );
    const Size sz( ARMYORDERW, ARMYORDERW );

    sf_color[0].Set( sz.w, sz.h, true );
    sf_color[0].DrawBorder( yellow );

    sf_color[1].Set( sz.w, sz.h, true );
    sf_color[1].DrawBorder( orange );

    sf_color[2].Set( sz.w, sz.h, true );
    sf_color[2].DrawBorder( green );
}

void Battle::ArmiesOrder::Set( const Rect & rt, const Units * units, int color )
{
    area = rt;
    orders = units;
    army_color2 = color;
    if ( units )
        rects.reserve( units->size() );
}

void Battle::ArmiesOrder::QueueEventProcessing( std::string & msg )
{
    LocalEvent & le = LocalEvent::Get();

    for ( std::vector<UnitPos>::const_iterator it = rects.begin(); it != rects.end(); ++it )
        if ( ( *it ).first ) {
            if ( le.MouseCursor( ( *it ).second ) ) {
                msg = _( "View %{monster} info." );
                StringReplace( msg, "%{monster}", ( *it ).first->GetName() );
            }

            const Unit & unit = *( *it ).first;

            if ( le.MouseClickLeft( ( *it ).second ) )
                Dialog::ArmyInfo( unit, Dialog::READONLY | Dialog::BUTTONS, unit.isReflect() );
            else if ( le.MousePressRight( ( *it ).second ) )
                Dialog::ArmyInfo( unit, Dialog::READONLY, unit.isReflect() );
        }
}

void Battle::ArmiesOrder::RedrawUnit( const Rect & pos, const Battle::Unit & unit, bool revert, bool current ) const
{
    Display & display = Display::Get();
    const Sprite & mons32 = AGG::GetICN( ICN::MONS32, unit.GetSpriteIndex(), revert );

    // background
    display.FillRect( pos, RGBA( 0x33, 0x33, 0x33 ) );
    // mons32 sprite
    mons32.Blit( pos.x + ( pos.w - mons32.w() ) / 2, pos.y + pos.h - mons32.h() - ( mons32.h() + 3 < pos.h ? 3 : 0 ), display );

    // window
    if ( current )
        sf_color[0].Blit( pos.x + 1, pos.y + 1, display );
    else if ( unit.Modes( Battle::TR_MOVED ) )
        sf_color[1].Blit( pos.x + 1, pos.y + 1, display );
    else
        sf_color[2].Blit( pos.x + 1, pos.y + 1, display );

    // number
    Text number( GetString( unit.GetCount() ), Font::SMALL );
    number.Blit( pos.x + 2, pos.y + 2 );
}

void Battle::ArmiesOrder::Redraw( const Unit * current )
{
    if ( orders ) {
        const u32 ow = ARMYORDERW + 2;

        u32 ox = area.x + ( area.w - ow * std::count_if( orders->begin(), orders->end(), std::mem_fun( &Unit::isValid ) ) ) / 2;
        u32 oy = area.y;

        Rect::x = ox;
        Rect::y = oy;
        Rect::h = ow;

        rects.clear();

        for ( Units::const_iterator it = orders->begin(); it != orders->end(); ++it )
            if ( *it && ( *it )->isValid() ) {
                rects.push_back( UnitPos( *it, Rect( ox, oy, ow, ow ) ) );
                RedrawUnit( rects.back().second, **it, ( **it ).GetColor() == army_color2, current == *it );
                ox += ow;
                Rect::w += ow;
            }
    }
}

Battle::Interface::Interface( Arena & a, s32 center )
    : arena( a )
    , _surfaceInnerArea( 0, 0, Display::DEFAULT_WIDTH, Display::DEFAULT_HEIGHT )
    , _mainSurface( Size( Display::DEFAULT_WIDTH, Display::DEFAULT_HEIGHT ), false )
    , icn_cbkg( ICN::UNKNOWN )
    , icn_frng( ICN::UNKNOWN )
    , humanturn_spell( Spell::NONE )
    , humanturn_exit( true )
    , humanturn_redraw( true )
    , animation_flags_frame( 0 )
    , catapult_frame( 0 )
    , _currentUnit( NULL )
    , _movingUnit( NULL )
    , _flyingUnit( NULL )
    , b_current_sprite( NULL )
    , index_pos( -1 )
    , teleport_src( -1 )
    , listlog( NULL )
    , turn( 0 )
    , _colorCycle( 0 )
    , _customPalette( PAL::GetPalette( PAL::STANDARD ) )
    , _contourColor( 110 )
    , _brightLandType( false )
    , _cycleBattlefield( false )
{
    const Settings & conf = Settings::Get();

    Cursor::Get().Hide();

    // border
    Display & display = Display::Get();

    _interfacePosition = Rect( ( display.w() - Display::DEFAULT_WIDTH ) / 2, ( display.h() - Display::DEFAULT_HEIGHT ) / 2, _surfaceInnerArea.w, _surfaceInnerArea.h );
    border.SetPosition( _interfacePosition.x - BORDERWIDTH, _interfacePosition.y - BORDERWIDTH, Display::DEFAULT_WIDTH, Display::DEFAULT_HEIGHT );

    // cover
    bool trees = Maps::ScanAroundObject( center, MP2::OBJ_TREES ).size();
    const Maps::Tiles & tile = world.GetTiles( center );
    bool grave = MP2::OBJ_GRAVEYARD == tile.GetObject( false );
    bool light = true;

    const int groundType = tile.GetGround();
    _brightLandType
        = ( groundType == Maps::Ground::SNOW || groundType == Maps::Ground::DESERT || groundType == Maps::Ground::WASTELAND || groundType == Maps::Ground::BEACH );
    if ( _brightLandType ) {
        _contourColor = 108;
    }

    switch ( groundType ) {
    case Maps::Ground::DESERT:
        icn_cbkg = ICN::CBKGDSRT;
        light = false;
        icn_frng = ICN::FRNG0004;
        break;
    case Maps::Ground::SNOW:
        icn_cbkg = trees ? ICN::CBKGSNTR : ICN::CBKGSNMT;
        light = false;
        icn_frng = trees ? ICN::FRNG0006 : ICN::FRNG0007;
        break;
    case Maps::Ground::SWAMP:
        icn_cbkg = ICN::CBKGSWMP;
        icn_frng = ICN::FRNG0008;
        break;
    case Maps::Ground::WASTELAND:
        icn_cbkg = ICN::CBKGCRCK;
        light = false;
        icn_frng = ICN::FRNG0003;
        break;
    case Maps::Ground::BEACH:
        icn_cbkg = ICN::CBKGBEAC;
        light = false;
        icn_frng = ICN::FRNG0002;
        break;
    case Maps::Ground::LAVA:
        icn_cbkg = ICN::CBKGLAVA;
        icn_frng = ICN::FRNG0005;
        break;
    case Maps::Ground::DIRT:
        icn_cbkg = trees ? ICN::CBKGDITR : ICN::CBKGDIMT;
        icn_frng = trees ? ICN::FRNG0010 : ICN::FRNG0009;
        break;
    case Maps::Ground::GRASS:
        icn_cbkg = trees ? ICN::CBKGGRTR : ICN::CBKGGRMT;
        icn_frng = trees ? ICN::FRNG0011 : ICN::FRNG0012;
        break;
    case Maps::Ground::WATER:
        icn_cbkg = ICN::CBKGWATR;
        icn_frng = ICN::FRNG0013;
        break;
    default:
        break;
    }

    if ( grave ) {
        icn_cbkg = ICN::CBKGGRAV;
        light = true;
        icn_frng = ICN::FRNG0001;
    }
    if ( conf.QVGA() || conf.ExtPocketLowMemory() )
        icn_frng = ICN::UNKNOWN;

    sf_cover.Set( _mainSurface.w(), _mainSurface.h(), false );

    // hexagon
    sf_hexagon = DrawHexagon( ( light ? RGBA( 0x78, 0x94, 0 ) : RGBA( 0x38, 0x48, 0 ) ) );
    sf_cursor = DrawHexagonShadow( 0x60 );
    sf_shadow = DrawHexagonShadow( 0x30 );

    btn_auto.SetSprite( ICN::TEXTBAR, 4, 5 );
    btn_settings.SetSprite( ICN::TEXTBAR, 6, 7 );

    // opponents
    opponent1 = arena.GetCommander1() ? new OpponentSprite( _surfaceInnerArea, arena.GetCommander1(), false ) : NULL;
    opponent2 = arena.GetCommander2() ? new OpponentSprite( _surfaceInnerArea, arena.GetCommander2(), true ) : NULL;

    if ( Arena::GetCastle() )
        main_tower = Rect( 570, 145, 70, 70 );

    const Rect & area = border.GetArea();

    btn_auto.SetPos( area.x, area.y + area.h - btn_settings.h - btn_auto.h );
    btn_settings.SetPos( area.x, area.y + area.h - btn_settings.h );

    if ( conf.ExtBattleSoftWait() ) {
        btn_wait.SetSprite( ICN::BATTLEWAIT, 0, 1 );
        btn_skip.SetSprite( ICN::BATTLESKIP, 0, 1 );

        btn_wait.SetPos( area.x + area.w - btn_wait.w, area.y + area.h - btn_skip.h - btn_wait.h );
        btn_skip.SetPos( area.x + area.w - btn_skip.w, area.y + area.h - btn_skip.h );
    }
    else {
        btn_skip.SetSprite( ICN::TEXTBAR, 0, 1 );
        btn_skip.SetPos( area.x + area.w - btn_skip.w, area.y + area.h - btn_skip.h );
    }

    status.SetPosition( area.x + btn_settings.w, btn_auto.y );

    listlog = new StatusListBox();

    if ( listlog )
        listlog->SetPosition( area.x, area.y + area.h - status.h );
    status.SetLogs( listlog );
}

Battle::Interface::~Interface()
{
    if ( listlog )
        delete listlog;
    if ( opponent1 )
        delete opponent1;
    if ( opponent2 )
        delete opponent2;
}

void Battle::Interface::SetArmiesOrder( const Units * units )
{
    armies_order.Set( GetArea(), units, arena.GetArmyColor2() );
}

const Rect & Battle::Interface::GetArea( void ) const
{
    return _surfaceInnerArea;
}

Point Battle::Interface::GetMouseCursor() const
{
    return LocalEvent::Get().GetMouseCursor() - _interfacePosition;
}

void Battle::Interface::SetStatus( const std::string & msg, bool top )
{
    if ( top ) {
        status.SetMessage( msg, true );
        status.SetMessage( "", false );
    }
    else {
        status.SetMessage( msg );
    }
    humanturn_redraw = true;
}

void Battle::Interface::CycleColors()
{
    ++_colorCycle;
    _customPalette = PAL::GetCyclingPalette( _colorCycle );
    _cycleBattlefield = true;
}

void Battle::Interface::UpdateContourColor()
{
    ++_contourCycle;

    if ( _brightLandType ) {
        static const uint8_t contourColorTable[] = {108, 115, 122, 129, 122, 115};
        _contourColor = contourColorTable[_contourCycle % sizeof( contourColorTable )];
    }
    else {
        static const uint8_t contourColorTable[] = {110, 114, 118, 122, 126, 122, 118, 114};
        _contourColor = contourColorTable[_contourCycle % sizeof( contourColorTable )];
    }
}

void Battle::Interface::Redraw( void )
{
    RedrawPartialStart();
    RedrawPartialFinish();
}

void Battle::Interface::RedrawPartialStart()
{
    const Castle * castle = Arena::GetCastle();

    Cursor::Get().Hide();
    RedrawBorder();
    RedrawCover();
    RedrawOpponents();
    if ( castle )
        RedrawCastle3( *castle );
    RedrawArmies();
}

void Battle::Interface::RedrawPartialFinish()
{
    Display & display = Display::Get();

    _mainSurface.Blit( _interfacePosition, display );
    RedrawInterface();
    if ( Settings::Get().ExtBattleShowBattleOrder() && !Settings::Get().QVGA() )
        armies_order.Redraw( _currentUnit );

    Cursor::Get().Show();
    display.Flip();
}

void Battle::Interface::RedrawInterface( void )
{
    const Settings & conf = Settings::Get();

    status.Redraw();

    btn_auto.Draw();
    btn_settings.Draw();

    if ( conf.ExtBattleSoftWait() )
        btn_wait.Draw();
    btn_skip.Draw();

    if ( !conf.QVGA() && !conf.ExtPocketLowMemory() )
        popup.Redraw( _interfacePosition.x + _interfacePosition.w + 60, _interfacePosition.y + _interfacePosition.h );

    if ( listlog && listlog->isOpenLog() )
        listlog->Redraw();
}

void Battle::Interface::RedrawArmies( void )
{
    const Castle * castle = Arena::GetCastle();

    for ( s32 ii = 0; ii < ARENASIZE; ++ii ) {
        RedrawHighObjects( ii );

        if ( castle )
            if ( 8 == ii || 19 == ii || 29 == ii || 40 == ii || 50 == ii || 62 == ii || 85 == ii || 73 == ii || 77 == ii )
                RedrawCastle2( *castle, ii );

        const Cell * cell = Board::GetCell( ii );
        const Unit * b = cell->GetUnit();

        if ( b && _flyingUnit != b && ii != b->GetTailIndex() ) {
            RedrawTroopSprite( *b );

            if ( _movingUnit != b && b->isValid() )
                RedrawTroopCount( *b );
        }
    }

    if ( castle ) {
        RedrawCastle2( *castle, 96 );
        const Unit * b = Board::GetCell( 96 )->GetUnit();
        if ( b )
            RedrawTroopSprite( *b );
    }

    if ( _flyingUnit ) {
        RedrawTroopSprite( *_flyingUnit );
    }
}

void Battle::Interface::RedrawOpponents( void )
{
    if ( opponent1 )
        opponent1->Redraw( _mainSurface, _colorCycle );
    if ( opponent2 )
        opponent2->Redraw( _mainSurface, _colorCycle );

    RedrawOpponentsFlags();
}

void Battle::Interface::RedrawOpponentsFlags( void )
{
    if ( !Settings::Get().QVGA() && opponent1 ) {
        int icn = ICN::UNKNOWN;

        switch ( arena.GetArmyColor1() ) {
        case Color::BLUE:
            icn = ICN::HEROFL00;
            break;
        case Color::GREEN:
            icn = ICN::HEROFL01;
            break;
        case Color::RED:
            icn = ICN::HEROFL02;
            break;
        case Color::YELLOW:
            icn = ICN::HEROFL03;
            break;
        case Color::ORANGE:
            icn = ICN::HEROFL04;
            break;
        case Color::PURPLE:
            icn = ICN::HEROFL05;
            break;
        default:
            icn = ICN::HEROFL06;
            break;
        }

        const Sprite & flag = AGG::GetICN( icn, ICN::AnimationFrame( icn, 0, animation_flags_frame ), false );
        flag.Blit( opponent1->Offset().x + OpponentSprite::HERO_X_OFFSET + flag.x(), opponent1->Offset().y + OpponentSprite::LEFT_HERO_Y_OFFSET + flag.y(),
                   _mainSurface );
    }

    if ( !Settings::Get().QVGA() && opponent2 ) {
        int icn = ICN::UNKNOWN;

        switch ( arena.GetForce2().GetColor() ) {
        case Color::BLUE:
            icn = ICN::HEROFL00;
            break;
        case Color::GREEN:
            icn = ICN::HEROFL01;
            break;
        case Color::RED:
            icn = ICN::HEROFL02;
            break;
        case Color::YELLOW:
            icn = ICN::HEROFL03;
            break;
        case Color::ORANGE:
            icn = ICN::HEROFL04;
            break;
        case Color::PURPLE:
            icn = ICN::HEROFL05;
            break;
        default:
            icn = ICN::HEROFL06;
            break;
        }

        const Sprite & flag = AGG::GetICN( icn, ICN::AnimationFrame( icn, 0, animation_flags_frame ), true );
        const Point offset = opponent2->Offset();
        flag.Blit( offset.x + Display::DEFAULT_WIDTH - OpponentSprite::HERO_X_OFFSET - ( flag.x() + flag.w() ), offset.y + OpponentSprite::RIGHT_HERO_Y_OFFSET + flag.y(),
                   _mainSurface );
    }
}

Point GetTroopPosition( const Battle::Unit & b, const Sprite & sprite )
{
    const Rect & rt = b.GetRectPosition();

    const s32 sx
        = b.isReflect() ? rt.x + ( b.isWide() ? rt.w / 2 + rt.w / 4 : rt.w / 2 ) - sprite.w() - sprite.x() : rt.x + ( b.isWide() ? rt.w / 4 : rt.w / 2 ) + sprite.x();
    const s32 sy = rt.y + rt.h + sprite.y() - 10;

    return Point( sx, sy );
}

void Battle::Interface::RedrawTroopSprite( const Unit & b )
{
    const Monster::monstersprite_t & msi = b.GetMonsterSprite();
    Sprite spmon1, spmon2;

    std::vector<std::vector<uint8_t> > applyPalettes;

    if ( b_current_sprite && _currentUnit == &b ) {
        spmon1 = *b_current_sprite;
        spmon2.Reset();
    }
    else if ( b.Modes( SP_STONE ) ) { // under medusa's stunning effect
        spmon1 = AGG::GetICN( msi.icn_file, b.GetFrame(), b.isReflect() );
        applyPalettes.push_back( PAL::GetPalette( PAL::GRAY ) );
    }
    else {
        // regular
        spmon1 = AGG::GetICN( msi.icn_file, b.GetFrame(), b.isReflect() );

        // this unit's turn, must be covered with contour
        if ( _currentUnit == &b ) {
            if ( b_current_sprite ) {
                spmon1 = *b_current_sprite;
                spmon2.Reset();
            }
            else {
                spmon2 = Sprite( b.GetContour( _contourColor ), 0, 0 );
            }
        }

        if ( b.hasColorCycling() && b.GetCustomAlpha() == 255 ) {
            applyPalettes.push_back( _customPalette );
        }

        if ( b.Modes( CAP_MIRRORIMAGE ) ) {
            applyPalettes.push_back( PAL::GetPalette( PAL::MIRROR_IMAGE ) );
        }
    }

    if ( spmon1.isValid() ) {
        const Rect & rt = b.GetRectPosition();
        Point sp = GetTroopPosition( b, spmon1 );

        // move offset
        if ( _movingUnit == &b ) {
            Sprite spmon0 = AGG::GetICN( msi.icn_file, _movingUnit->animation.firstFrame(), b.isReflect() );
            const s32 ox = spmon1.x() - spmon0.x();

            if ( _movingUnit->animation.animationLength() ) {
                const s32 cx = _movingPos.x - rt.x;
                const s32 cy = _movingPos.y - rt.y;

                // TODO: use offset X from bin file for ground movement
                // cx/cy is sprite size
                // Frame count: one tile of movement goes through all stages of animation
                // sp is sprite drawing offset
                sp.y += _movingUnit->animation.movementProgress() * cy;
                if ( 0 != Sign( cy ) )
                    sp.x -= Sign( cx ) * ox / 2;
            }
        }
        // fly offset
        else if ( _flyingUnit == &b ) {
            const s32 cx = _flyingPos.x - rt.x;
            const s32 cy = _flyingPos.y - rt.y;

            sp.x += cx + ( _movingPos.x - _flyingPos.x ) * _flyingUnit->animation.movementProgress();
            sp.y += cy + ( _movingPos.y - _flyingPos.y ) * _flyingUnit->animation.movementProgress();
        }

        if ( !applyPalettes.empty() ) {
            for ( size_t i = 1; i < applyPalettes.size(); ++i ) {
                applyPalettes[0] = PAL::CombinePalettes( applyPalettes[0], applyPalettes[i] );
            }
            spmon1 = Sprite( spmon1.GetSurface(), spmon1.x(), spmon1.y() );
            AGG::ReplaceColors( spmon1, applyPalettes[0], msi.icn_file, b.GetFrame(), b.isReflect() );
        }

        // sprite monster
        if ( b.GetCustomAlpha() < 255 ) {
            spmon1 = Sprite( spmon1.GetSurface(), spmon1.x(), spmon1.y() );
            spmon1.SetAlphaMod( b.GetCustomAlpha(), false );
        }
        spmon1.Blit( sp, _mainSurface );

        // contour
        if ( spmon2.isValid() )
            spmon2.Blit( sp.x, sp.y, _mainSurface );
    }
}

void Battle::Interface::RedrawTroopCount( const Unit & unit )
{
    const Rect & rt = unit.GetRectPosition();
    const Sprite & bar = AGG::GetICN( ICN::TEXTBAR, GetIndexIndicator( unit ) );
    const bool isReflected = unit.isReflect();

    const int tileInFront = Board::GetIndexDirection( unit.GetHeadIndex(), isReflected ? Battle::LEFT : Battle::RIGHT );

    s32 sx = rt.x + ( isReflected ? -7 : rt.w - 13 );
    const s32 sy = rt.y + rt.h - bar.h() - ( isReflected ? 23 : 11 );

    int xOffset = unit.animation.getTroopCountOffset( isReflected );
    // check if has unit standing in front
    if ( xOffset > 0 && Board::isValidIndex( tileInFront ) && Board::GetCell( tileInFront )->GetUnit() != NULL )
        xOffset = 0;

    sx += isReflected ? -xOffset : xOffset;

    bar.Blit( sx, sy, _mainSurface );

    Text text( GetStringShort( unit.GetCount() ), Font::SMALL );
    text.Blit( sx + ( bar.w() - text.w() ) / 2, sy, _mainSurface );
}

void Battle::Interface::RedrawCover( void )
{
    const Settings & conf = Settings::Get();

    RedrawCoverStatic( _mainSurface );

    const Board & board = *Arena::GetBoard();
    RedrawCoverBoard( conf, board );

    const Bridge * bridge = Arena::GetBridge();
    // bridge
    if ( bridge && bridge->isDown() ) {
        const Sprite & sprite3 = AGG::GetICN( ICN::Get4Castle( Arena::GetCastle()->GetRace() ), bridge->isDestroy() ? 24 : 21 );
        sprite3.Blit( sprite3.x(), sprite3.y(), _mainSurface );
    }

    // cursor
    const Cell * cell = Board::GetCell( index_pos );

    if ( cell && _currentUnit && conf.ExtBattleShowMouseShadow() && Cursor::Get().Themes() != Cursor::WAR_NONE )
        sf_cursor.Blit( cell->GetPos(), _mainSurface );

    RedrawKilled();

    _cycleBattlefield = false;
}

void Battle::Interface::RedrawCoverStatic( Surface & dst )
{
    if ( icn_cbkg != ICN::UNKNOWN ) {
        Sprite cbkg = AGG::GetICN( icn_cbkg, 0 );
        if ( _cycleBattlefield && ( icn_cbkg == ICN::CBKGLAVA || icn_cbkg == ICN::CBKGWATR || icn_cbkg == ICN::CBKGSWMP ) )
            AGG::ReplaceColors( cbkg, _customPalette, icn_cbkg, 0, false );
        cbkg.Blit( dst );
    }

    if ( icn_frng != ICN::UNKNOWN ) {
        const Sprite & frng = AGG::GetICN( icn_frng, 0 );
        frng.Blit( frng.x(), frng.y(), dst );
    }

    if ( arena.GetICNCovr() != ICN::UNKNOWN ) {
        const Sprite & cover = AGG::GetICN( arena.GetICNCovr(), 0 );
        cover.Blit( cover.x(), cover.y(), dst );
    }

    // ground obstacles
    for ( u32 ii = 0; ii < ARENASIZE; ++ii ) {
        RedrawLowObjects( ii, dst );
    }

    const Castle * castle = Arena::GetCastle();
    if ( castle )
        RedrawCastle1( *castle, dst );
}

void Battle::Interface::RedrawCoverBoard( const Settings & conf, const Board & board )
{
    if ( conf.ExtBattleShowGrid() ) { // grid
        for ( Board::const_iterator it = board.begin(); it != board.end(); ++it )
            if ( ( *it ).GetObject() == 0 )
                sf_hexagon.Blit( ( *it ).GetPos(), _mainSurface );
    }

    if ( !_movingUnit && conf.ExtBattleShowMoveShadow() && _currentUnit && !( _currentUnit->GetCurrentControl() & CONTROL_AI ) ) { // shadow
        for ( Board::const_iterator it = board.begin(); it != board.end(); ++it ) {
            if ( ( *it ).isPassable1( true ) && UNKNOWN != ( *it ).GetDirection() )
                sf_shadow.Blit( ( *it ).GetPos(), _mainSurface );
        }
    }
}

void Battle::Interface::RedrawCastle1( const Castle & castle, Surface & dst )
{
    const bool fortification = ( Race::KNGT == castle.GetRace() ) && castle.isBuild( BUILD_SPEC );

    int icn_castbkg = ICN::UNKNOWN;

    switch ( castle.GetRace() ) {
    default:
    case Race::BARB:
        icn_castbkg = ICN::CASTBKGB;
        break;
    case Race::KNGT:
        icn_castbkg = ICN::CASTBKGK;
        break;
    case Race::NECR:
        icn_castbkg = ICN::CASTBKGN;
        break;
    case Race::SORC:
        icn_castbkg = ICN::CASTBKGS;
        break;
    case Race::WRLK:
        icn_castbkg = ICN::CASTBKGW;
        break;
    case Race::WZRD:
        icn_castbkg = ICN::CASTBKGZ;
        break;
    }

    // castle cover
    const Sprite & sprite1 = AGG::GetICN( icn_castbkg, 1 );
    sprite1.Blit( sprite1.x(), sprite1.y(), dst );

    // moat
    if ( castle.isBuild( BUILD_MOAT ) ) {
        Sprite sprite = AGG::GetICN( ICN::MOATWHOL, 0 );
        if ( _cycleBattlefield )
            AGG::ReplaceColors( sprite, _customPalette, ICN::MOATWHOL, 0, false );
        sprite.Blit( sprite.x(), sprite.y(), dst );
    }

    // top wall
    const Sprite & sprite2 = AGG::GetICN( icn_castbkg, fortification ? 4 : 3 );
    sprite2.Blit( sprite2.x(), sprite2.y(), dst );
}

void Battle::Interface::RedrawCastle2( const Castle & castle, s32 cell_index )
{
    const int icn_castle = ICN::Get4Castle( castle.GetRace() );

    // catapult
    if ( 77 == cell_index ) {
        const Sprite & sprite = AGG::GetICN( ICN::CATAPULT, catapult_frame );
        const Rect & pos = Board::GetCell( cell_index )->GetPos();
        sprite.Blit( sprite.x() + pos.x - pos.w, sprite.y() + pos.y + pos.h - 10, _mainSurface );
    }
    else
        // castle gate
        if ( 50 == cell_index ) {
        const Sprite & sprite = AGG::GetICN( icn_castle, 4 );
        sprite.Blit( sprite.x(), sprite.y(), _mainSurface );
    }
    else
        // castle wall
        if ( 8 == cell_index || 29 == cell_index || 73 == cell_index || 96 == cell_index ) {
        u32 index = 0;
        const bool fortification = ( Race::KNGT == castle.GetRace() ) && castle.isBuild( BUILD_SPEC );

        switch ( cell_index ) {
        case 8:
            index = 5;
            break;
        case 29:
            index = 6;
            break;
        case 73:
            index = 7;
            break;
        case 96:
            index = 8;
            break;
        default:
            break;
        }

        if ( fortification ) {
            switch ( Board::GetCell( cell_index )->GetObject() ) {
            case 0:
                index += 31;
                break;
            case 1:
                index += 35;
                break;
            case 2:
                index += 27;
                break;
            case 3:
                index += 23;
                break;
            default:
                break;
            }
        }
        else {
            switch ( Board::GetCell( cell_index )->GetObject() ) {
            case 0:
                index += 8;
                break;
            case 1:
                index += 4;
                break;
            case 2:
                index += 0;
                break;
            default:
                break;
            }
        }

        const Sprite & sprite = AGG::GetICN( icn_castle, index );
        sprite.Blit( sprite.x(), sprite.y(), _mainSurface );
    }
    else
        // castle archer towers
        if ( 19 == cell_index ) {
        const Tower * ltower = Arena::GetTower( TWR_LEFT );
        u32 index = 17;

        if ( castle.isBuild( BUILD_LEFTTURRET ) && ltower )
            index = ltower->isValid() ? 18 : 19;

        AGG::GetICN( icn_castle, index ).Blit( 415, 40, _mainSurface );
    }
    else if ( 85 == cell_index ) {
        const Tower * rtower = Arena::GetTower( TWR_RIGHT );
        u32 index = 17;

        if ( castle.isBuild( BUILD_RIGHTTURRET ) && rtower )
            index = rtower->isValid() ? 18 : 19;

        AGG::GetICN( icn_castle, index ).Blit( 415, 290, _mainSurface );
    }
    else
        // castle towers
        if ( 40 == cell_index )
        AGG::GetICN( icn_castle, 17 ).Blit( 375, 120, _mainSurface );
    else
        // castle towers
        if ( 62 == cell_index )
        AGG::GetICN( icn_castle, 17 ).Blit( 375, 205, _mainSurface );
}

void Battle::Interface::RedrawCastle3( const Castle & castle )
{
    const Sprite & sprite = AGG::GetICN( ICN::Get4Castle( castle.GetRace() ), ( Arena::GetTower( TWR_CENTER )->isValid() ? 20 : 26 ) );

    sprite.Blit( sprite.x(), sprite.y(), _mainSurface );
}

void Battle::Interface::RedrawLowObjects( s32 cell_index, Surface & dst )
{
    const Cell * cell = Board::GetCell( cell_index );
    if ( cell == NULL )
        return;

    Sprite sprite;

    switch ( cell->GetObject() ) {
    case 0x84:
        sprite = AGG::GetICN( ICN::COBJ0004, 0 );
        break;
    case 0x87:
        sprite = AGG::GetICN( ICN::COBJ0007, 0 );
        break;
    case 0x90:
        sprite = AGG::GetICN( ICN::COBJ0016, 0 );
        break;
    case 0x9E:
        sprite = AGG::GetICN( ICN::COBJ0030, 0 );
        break;
    case 0x9F:
        sprite = AGG::GetICN( ICN::COBJ0031, 0 );
        break;
    default:
        break;
    }

    if ( sprite.isValid() ) {
        // const Point & topleft = border.GetArea();
        const Rect & pt = cell->GetPos();
        sprite.Blit( pt.x + pt.w / 2 + sprite.x(), pt.y + pt.h + sprite.y() - ( Settings::Get().QVGA() ? 5 : 10 ), dst );
    }
}

void Battle::Interface::RedrawHighObjects( s32 cell_index )
{
    const Cell * cell = Board::GetCell( cell_index );
    if ( cell == NULL )
        return;

    Sprite sprite;

    switch ( cell->GetObject() ) {
    case 0x80:
        sprite = AGG::GetICN( ICN::COBJ0000, 0 );
        break;
    case 0x81:
        sprite = AGG::GetICN( ICN::COBJ0001, 0 );
        break;
    case 0x82:
        sprite = AGG::GetICN( ICN::COBJ0002, 0 );
        break;
    case 0x83:
        sprite = AGG::GetICN( ICN::COBJ0003, 0 );
        break;
    case 0x85:
        sprite = AGG::GetICN( ICN::COBJ0005, 0 );
        break;
    case 0x86:
        sprite = AGG::GetICN( ICN::COBJ0006, 0 );
        break;
    case 0x88:
        sprite = AGG::GetICN( ICN::COBJ0008, 0 );
        break;
    case 0x89:
        sprite = AGG::GetICN( ICN::COBJ0009, 0 );
        break;
    case 0x8A:
        sprite = AGG::GetICN( ICN::COBJ0010, 0 );
        break;
    case 0x8B:
        sprite = AGG::GetICN( ICN::COBJ0011, 0 );
        break;
    case 0x8C:
        sprite = AGG::GetICN( ICN::COBJ0012, 0 );
        break;
    case 0x8D:
        sprite = AGG::GetICN( ICN::COBJ0013, 0 );
        break;
    case 0x8E:
        sprite = AGG::GetICN( ICN::COBJ0014, 0 );
        break;
    case 0x8F:
        sprite = AGG::GetICN( ICN::COBJ0015, 0 );
        break;
    case 0x91:
        sprite = AGG::GetICN( ICN::COBJ0017, 0 );
        break;
    case 0x92:
        sprite = AGG::GetICN( ICN::COBJ0018, 0 );
        break;
    case 0x93:
        sprite = AGG::GetICN( ICN::COBJ0019, 0 );
        break;
    case 0x94:
        sprite = AGG::GetICN( ICN::COBJ0020, 0 );
        break;
    case 0x95:
        sprite = AGG::GetICN( ICN::COBJ0021, 0 );
        break;
    case 0x96:
        sprite = AGG::GetICN( ICN::COBJ0022, 0 );
        break;
    case 0x97:
        sprite = AGG::GetICN( ICN::COBJ0023, 0 );
        break;
    case 0x98:
        sprite = AGG::GetICN( ICN::COBJ0024, 0 );
        break;
    case 0x99:
        sprite = AGG::GetICN( ICN::COBJ0025, 0 );
        break;
    case 0x9A:
        sprite = AGG::GetICN( ICN::COBJ0026, 0 );
        break;
    case 0x9B:
        sprite = AGG::GetICN( ICN::COBJ0027, 0 );
        break;
    case 0x9C:
        sprite = AGG::GetICN( ICN::COBJ0028, 0 );
        break;
    case 0x9D:
        sprite = AGG::GetICN( ICN::COBJ0029, 0 );
        break;
    default:
        break;
    }

    if ( sprite.isValid() ) {
        // const Point & topleft = border.GetArea();
        const Rect & pt = cell->GetPos();
        sprite.Blit( pt.x + pt.w / 2 + sprite.x(), pt.y + pt.h + sprite.y() - 10, _mainSurface );
    }
}

void Battle::Interface::RedrawKilled( void )
{
    // redraw killed troop
    const Indexes cells = arena.GraveyardClosedCells();

    for ( Indexes::const_iterator it = cells.begin(); it != cells.end(); ++it ) {
        const Unit * b = arena.GraveyardLastTroop( *it );

        if ( b && *it != b->GetTailIndex() ) {
            RedrawTroopSprite( *b );
        }
    }
}

void Battle::Interface::RedrawBorder( void )
{
    const Size displaySize = Display::Get().GetSize();

    if ( displaySize != Display::GetDefaultSize() )
        Dialog::FrameBorder::RenderRegular( border.GetRect() );
}

int Battle::Interface::GetBattleCursor( std::string & statusMsg ) const
{
    statusMsg.clear();

    const Cell * cell = Board::GetCell( index_pos );

    if ( cell && _currentUnit ) {
        const Unit * b_enemy = cell->GetUnit();

        if ( b_enemy ) {
            if ( _currentUnit->GetCurrentColor() == b_enemy->GetColor() || ( _currentUnit == b_enemy ) ) {
                statusMsg = _( "View %{monster} info." );
                StringReplace( statusMsg, "%{monster}", b_enemy->GetMultiName() );
                return Cursor::WAR_INFO;
            }
            else {
                if ( _currentUnit->isArchers() && !_currentUnit->isHandFighting() ) {
                    statusMsg = _( "Shoot %{monster}" );
                    statusMsg.append( " " );
                    statusMsg.append( _n( "(one shot left)", "(%{count} shots left)", _currentUnit->GetShots() ) );
                    StringReplace( statusMsg, "%{monster}", b_enemy->GetMultiName() );
                    StringReplace( statusMsg, "%{count}", _currentUnit->GetShots() );

                    return arena.GetObstaclesPenalty( *_currentUnit, *b_enemy ) ? Cursor::WAR_BROKENARROW : Cursor::WAR_ARROW;
                }
                else {
                    const int dir = cell->GetTriangleDirection( GetMouseCursor() );
                    const int cursor = GetSwordCursorDirection( dir );

                    if ( cursor && Board::isValidDirection( index_pos, dir ) ) {
                        const s32 from = Board::GetIndexDirection( index_pos, dir );

                        // if free cell or it is b_current
                        if ( UNKNOWN != Board::GetCell( from )->GetDirection() || from == _currentUnit->GetHeadIndex()
                             || ( _currentUnit->isWide() && from == _currentUnit->GetTailIndex() ) ) {
                            statusMsg = _( "Attack %{monster}" );
                            StringReplace( statusMsg, "%{monster}", b_enemy->GetName() );

                            return cursor;
                        }
                    }
                }
            }
        }
        else if ( cell->isPassable3( *_currentUnit, false ) && UNKNOWN != cell->GetDirection() ) {
            statusMsg = _currentUnit->isFlying() ? _( "Fly %{monster} here." ) : _( "Move %{monster} here." );
            StringReplace( statusMsg, "%{monster}", _currentUnit->GetName() );
            return _currentUnit->isFlying() ? Cursor::WAR_FLY : Cursor::WAR_MOVE;
        }
    }

    statusMsg = _( "Turn %{turn}" );
    StringReplace( statusMsg, "%{turn}", arena.GetCurrentTurn() );

    return Cursor::WAR_NONE;
}

int Battle::Interface::GetBattleSpellCursor( std::string & statusMsg ) const
{
    statusMsg.clear();

    const Cell * cell = Board::GetCell( index_pos );
    const Spell & spell = humanturn_spell;

    if ( cell && _currentUnit && spell.isValid() ) {
        const Unit * b_stats = cell->GetUnit();

        // over graveyard
        if ( !b_stats && arena.GraveyardAllowResurrect( index_pos, spell ) ) {
            b_stats = arena.GraveyardLastTroop( index_pos );
            if ( b_stats->isWide() ) { // we need to check tail and head positions
                const Cell * tailCell = Board::GetCell( b_stats->GetTailIndex() );
                const Cell * headCell = Board::GetCell( b_stats->GetHeadIndex() );
                if ( !tailCell || tailCell->GetUnit() || !headCell || headCell->GetUnit() )
                    b_stats = NULL;
            }
        }

        // teleport check first
        if ( Board::isValidIndex( teleport_src ) ) {
            if ( !b_stats && cell->isPassable3( *_currentUnit, false ) ) {
                statusMsg = _( "Teleport Here" );
                return Cursor::SP_TELEPORT;
            }

            statusMsg = _( "Invalid Teleport Destination" );
            return Cursor::WAR_NONE;
        }
        else if ( b_stats && b_stats->AllowApplySpell( spell, _currentUnit->GetCommander() ) ) {
            statusMsg = _( "Cast %{spell} on %{monster}" );
            StringReplace( statusMsg, "%{spell}", spell.GetName() );
            StringReplace( statusMsg, "%{monster}", b_stats->GetName() );
            return GetCursorFromSpell( spell() );
        }
        else if ( !spell.isApplyToFriends() && !spell.isApplyToEnemies() && !spell.isApplyToAnyTroops() ) {
            statusMsg = _( "Cast %{spell}" );
            StringReplace( statusMsg, "%{spell}", spell.GetName() );
            return GetCursorFromSpell( spell() );
        }
    }

    statusMsg = _( "Select Spell Target" );

    return Cursor::WAR_NONE;
}

void Battle::Interface::HumanTurn( const Unit & b, Actions & a )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.SetThemes( Cursor::WAR_NONE );
    _currentUnit = &b;
    humanturn_redraw = false;
    humanturn_exit = false;
    catapult_frame = 0;

    // in case we moved the window
    _interfacePosition = border.GetArea();

    Board & board = *Arena::GetBoard();
    board.Reset();
    board.SetScanPassability( b );

    if ( listlog && turn != arena.GetCurrentTurn() ) {
        turn = arena.GetCurrentTurn();
        std::string msg = _( "Turn %{turn}" );
        StringReplace( msg, "%{turn}", turn );
        listlog->AddMessage( msg );
    }

    popup.Reset();

    // safe position coord
    CursorPosition cursorPosition;

    Redraw();

    std::string msg;
    animation_flags_frame = 0;

    ResetIdleTroopAnimation();

    while ( !humanturn_exit && le.HandleEvents() ) {
        // move cursor
        const s32 index_new = board.GetIndexAbsPosition( GetMouseCursor() );
        if ( index_pos != index_new ) {
            index_pos = index_new;
            humanturn_redraw = true;
        }

        if ( humanturn_spell.isValid() )
            HumanCastSpellTurn( b, a, msg );
        else
            HumanBattleTurn( b, a, msg );

        if ( humanturn_exit )
            cursor.SetThemes( Cursor::WAIT );

        // update status
        if ( msg != status.GetMessage() ) {
            status.SetMessage( msg );
            humanturn_redraw = true;
        }

        // animation troops
        if ( IdleTroopsAnimation() )
            humanturn_redraw = true;

        CheckGlobalEvents( le );

        // redraw arena
        if ( humanturn_redraw ) {
            Redraw();
            humanturn_redraw = false;
        }

        if ( !cursor.isVisible() ) {
            cursor.Show();
        }
    }

    popup.Reset();
    _currentUnit = NULL;
}

void Battle::Interface::HumanBattleTurn( const Unit & b, Actions & a, std::string & msg )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Settings & conf = Settings::Get();

    if ( le.KeyPress() ) {
        // skip
        if ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_HARDSKIP ) ) {
            a.push_back( Command( MSG_BATTLE_SKIP, b.GetUID(), true ) );
            humanturn_exit = true;
        }
        else
            // soft skip
            if ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_SOFTSKIP ) ) {
            a.push_back( Command( MSG_BATTLE_SKIP, b.GetUID(), !conf.ExtBattleSoftWait() ) );
            humanturn_exit = true;
        }
        else
            // options
            if ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_OPTIONS ) )
            EventShowOptions();
        else
            // auto switch
            if ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_AUTOSWITCH ) )
            EventAutoSwitch( b, a );
        else
            // cast
            if ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_CASTSPELL ) )
            ProcessingHeroDialogResult( 1, a );
        else
            // retreat
            if ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_RETREAT ) )
            ProcessingHeroDialogResult( 2, a );
        else
            // surrender
            if ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_SURRENDER ) )
            ProcessingHeroDialogResult( 3, a );

            // debug only
#ifdef WITH_DEBUG
        if ( IS_DEVEL() )
            switch ( le.KeyValue() ) {
            case KEY_w:
                // fast wins game
                arena.GetResult().army1 = RESULT_WINS;
                humanturn_exit = true;
                a.push_back( Command( MSG_BATTLE_END_TURN, b.GetUID() ) );
                break;

            case KEY_l:
                // fast loss game
                arena.GetResult().army1 = RESULT_LOSS;
                humanturn_exit = true;
                a.push_back( Command( MSG_BATTLE_END_TURN, b.GetUID() ) );
                break;

            default:
                break;
            }
#endif
    }

    // Add offsets to inner objects
    const Rect mainTowerRect = main_tower + _interfacePosition.getPosition();
    const Rect armiesOrderRect = armies_order + _interfacePosition.getPosition();
    if ( pocket_book.w && le.MouseCursor( pocket_book ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );
        msg = _( "Spell cast" );

        if ( le.MouseClickLeft( pocket_book ) ) {
            ProcessingHeroDialogResult( 1, a );
            humanturn_redraw = true;
        }
    }
    else if ( Arena::GetTower( TWR_CENTER ) && le.MouseCursor( mainTowerRect ) ) {
        cursor.SetThemes( Cursor::WAR_INFO );
        msg = _( "View Ballista Info" );

        if ( le.MouseClickLeft( mainTowerRect ) || le.MousePressRight( mainTowerRect ) ) {
            const Castle * cstl = Arena::GetCastle();
            std::string ballistaMessage = Tower::GetInfo( *cstl );

            if ( cstl->isBuild( BUILD_MOAT ) ) {
                ballistaMessage.append( "\n \n" );
                ballistaMessage.append( Battle::Board::GetMoatInfo() );
            }

            Dialog::Message( _( "Ballista" ), ballistaMessage, Font::BIG, le.MousePressRight() ? 0 : Dialog::OK );
        }
    }
    else if ( conf.ExtBattleShowBattleOrder() && le.MouseCursor( armiesOrderRect ) ) {
        cursor.SetThemes( Cursor::POINTER );
        armies_order.QueueEventProcessing( msg );
    }
    else if ( le.MouseCursor( btn_auto ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );
        msg = _( "Auto combat" );
        ButtonAutoAction( b, a );
    }
    else if ( le.MouseCursor( btn_settings ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );
        msg = _( "Customize system options." );
        ButtonSettingsAction();
    }
    else if ( conf.ExtBattleSoftWait() && le.MouseCursor( btn_wait ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );
        msg = _( "Wait this unit" );
        ButtonWaitAction( a );
    }
    else if ( le.MouseCursor( btn_skip ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );
        msg = _( "Skip this unit" );
        ButtonSkipAction( a );
    }
    else if ( opponent1 && le.MouseCursor( opponent1->GetArea() + _interfacePosition.getPosition() ) ) {
        const Rect opponent1Area = opponent1->GetArea() + _interfacePosition.getPosition();
        if ( arena.GetCurrentColor() == arena.GetArmyColor1() ) {
            msg = _( "Hero's Options" );
            cursor.SetThemes( Cursor::WAR_HERO );

            if ( le.MouseClickLeft( opponent1Area ) ) {
                ProcessingHeroDialogResult( arena.DialogBattleHero( *opponent1->GetHero(), true ), a );
                humanturn_redraw = true;
            }
        }
        else {
            msg = _( "View Opposing Hero" );
            cursor.SetThemes( Cursor::WAR_INFO );

            if ( le.MouseClickLeft( opponent1Area ) ) {
                arena.DialogBattleHero( *opponent1->GetHero(), true );
                humanturn_redraw = true;
            }
        }

        if ( le.MousePressRight( opponent1Area ) ) {
            arena.DialogBattleHero( *opponent1->GetHero(), false );
            humanturn_redraw = true;
        }
    }
    else if ( opponent2 && le.MouseCursor( opponent2->GetArea() + _interfacePosition.getPosition() ) ) {
        const Rect opponent2Area = opponent2->GetArea() + _interfacePosition.getPosition();
        if ( arena.GetCurrentColor() == arena.GetForce2().GetColor() ) {
            msg = _( "Hero's Options" );
            cursor.SetThemes( Cursor::WAR_HERO );

            if ( le.MouseClickLeft( opponent2Area ) ) {
                ProcessingHeroDialogResult( arena.DialogBattleHero( *opponent2->GetHero(), true ), a );
                humanturn_redraw = true;
            }
        }
        else {
            msg = _( "View Opposing Hero" );
            cursor.SetThemes( Cursor::WAR_INFO );

            if ( le.MouseClickLeft( opponent2Area ) ) {
                arena.DialogBattleHero( *opponent2->GetHero(), true );
                humanturn_redraw = true;
            }
        }

        if ( le.MousePressRight( opponent2Area ) ) {
            arena.DialogBattleHero( *opponent2->GetHero(), false );
            humanturn_redraw = true;
        }
    }
    else if ( listlog && listlog->isOpenLog() && le.MouseCursor( listlog->GetArea() ) ) {
        cursor.SetThemes( Cursor::WAR_POINTER );
        listlog->QueueEventProcessing();
    }
    else if ( le.MouseCursor( Rect( _interfacePosition.x, _interfacePosition.y, _interfacePosition.w, _interfacePosition.h - status.h ) ) ) {
        const int themes = GetBattleCursor( msg );

        if ( cursor.Themes() != themes )
            cursor.SetThemes( themes );

        const Cell * cell = Board::GetCell( index_pos );

        if ( cell ) {
            if ( CursorAttack( themes ) ) {
                const Unit * b_enemy = cell->GetUnit();
                popup.SetInfo( cell, _currentUnit, b_enemy );
            }
            else
                popup.Reset();

            if ( le.MouseClickLeft() )
                MouseLeftClickBoardAction( themes, *cell, a );
            else if ( le.MousePressRight() )
                MousePressRightBoardAction( themes, *cell, a );
        }
        else {
            le.MouseClickLeft();
            le.MousePressRight();
        }
    }
    else if ( le.MouseCursor( status ) ) {
        if ( listlog ) {
            msg = ( listlog->isOpenLog() ? _( "Hide logs" ) : _( "Show logs" ) );
            if ( le.MouseClickLeft( status ) )
                listlog->SetOpenLog( !listlog->isOpenLog() );
        }
        cursor.SetThemes( Cursor::WAR_POINTER );
    }
    else {
        cursor.SetThemes( Cursor::WAR_NONE );
        le.MouseClickLeft();
        le.MousePressRight();
    }
}

void Battle::Interface::HumanCastSpellTurn( const Unit & b, Actions & a, std::string & msg )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    // reset cast
    if ( le.MousePressRight() ) {
        humanturn_spell = Spell::NONE;
        teleport_src = -1;
    }
    else if ( le.MouseCursor( _interfacePosition ) && humanturn_spell.isValid() ) {
        const int themes = GetBattleSpellCursor( msg );

        if ( cursor.Themes() != themes )
            cursor.SetThemes( themes );

        if ( le.MouseClickLeft() && Cursor::WAR_NONE != cursor.Themes() ) {
            if ( !Board::isValidIndex( index_pos ) ) {
                DEBUG( DBG_BATTLE, DBG_WARN,
                       "dst: "
                           << "out of range" );
                return;
            }

            if ( listlog ) {
                std::string str = _( "%{color} cast spell: %{spell}" );
                const HeroBase * current_commander = arena.GetCurrentCommander();
                if ( current_commander )
                    StringReplace( str, "%{color}", Color::String( current_commander->GetColor() ) );
                StringReplace( str, "%{spell}", humanturn_spell.GetName() );
                listlog->AddMessage( str );
            }

            DEBUG( DBG_BATTLE, DBG_TRACE, humanturn_spell.GetName() << ", dst: " << index_pos );

            if ( Cursor::SP_TELEPORT == cursor.Themes() ) {
                if ( 0 > teleport_src )
                    teleport_src = index_pos;
                else {
                    a.push_back( Command( MSG_BATTLE_CAST, Spell::TELEPORT, teleport_src, index_pos ) );
                    humanturn_spell = Spell::NONE;
                    humanturn_exit = true;
                    teleport_src = -1;
                }
            }
            else if ( Cursor::SP_MIRRORIMAGE == cursor.Themes() ) {
                a.push_back( Command( MSG_BATTLE_CAST, Spell::MIRRORIMAGE, index_pos ) );
                humanturn_spell = Spell::NONE;
                humanturn_exit = true;
            }
            else {
                a.push_back( Command( MSG_BATTLE_CAST, humanturn_spell(), index_pos ) );
                humanturn_spell = Spell::NONE;
                humanturn_exit = true;
            }
        }
    }
    else {
        cursor.SetThemes( Cursor::WAR_NONE );
    }
}

void Battle::Interface::FadeArena( void )
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    Settings & conf = Settings::Get();

    cursor.SetThemes( cursor.WAR_POINTER );
    Redraw();

    if ( !conf.QVGA() ) {
        cursor.Hide();
        Rect srt = border.GetArea();
        Surface top = display.GetSurface( srt );
        Surface back( top.GetSize(), false );
        back.Fill( ColorBlack );
        display.Fade( top, back, srt, 100, 300 );
        cursor.Show();
        display.Flip();
    }
}

int Battle::GetIndexIndicator( const Unit & b )
{
    // yellow
    if ( b.Modes( IS_GREEN_STATUS ) && b.Modes( IS_RED_STATUS ) )
        return 13;
    else
        // green
        if ( b.Modes( IS_GREEN_STATUS ) )
        return 12;
    else
        // red
        if ( b.Modes( IS_RED_STATUS ) )
        return 14;

    return 10;
}

void Battle::Interface::EventShowOptions( void )
{
    btn_settings.PressDraw();
    DialogBattleSettings();
    btn_settings.ReleaseDraw();
    humanturn_redraw = true;
}

void Battle::Interface::EventAutoSwitch( const Unit & b, Actions & a )
{
    btn_auto.PressDraw();

    a.push_back( Command( MSG_BATTLE_AUTO, b.GetColor() ) );

    Cursor::Get().SetThemes( Cursor::WAIT );
    humanturn_redraw = true;
    humanturn_exit = true;

    btn_auto.ReleaseDraw();
}

void Battle::Interface::ButtonAutoAction( const Unit & b, Actions & a )
{
    LocalEvent & le = LocalEvent::Get();

    le.MousePressLeft( btn_auto ) ? btn_auto.PressDraw() : btn_auto.ReleaseDraw();

    if ( le.MouseClickLeft( btn_auto ) )
        EventAutoSwitch( b, a );
}

void Battle::Interface::ButtonSettingsAction( void )
{
    LocalEvent & le = LocalEvent::Get();

    le.MousePressLeft( btn_settings ) ? btn_settings.PressDraw() : btn_settings.ReleaseDraw();

    if ( le.MouseClickLeft( btn_settings ) ) {
        DialogBattleSettings();
        humanturn_redraw = true;
    }
}

void Battle::Interface::ButtonWaitAction( Actions & a )
{
    LocalEvent & le = LocalEvent::Get();

    le.MousePressLeft( btn_wait ) ? btn_wait.PressDraw() : btn_wait.ReleaseDraw();

    if ( le.MouseClickLeft( btn_wait ) && _currentUnit ) {
        a.push_back( Command( MSG_BATTLE_SKIP, _currentUnit->GetUID(), false ) );
        humanturn_exit = true;
    }
}

void Battle::Interface::ButtonSkipAction( Actions & a )
{
    LocalEvent & le = LocalEvent::Get();

    le.MousePressLeft( btn_skip ) ? btn_skip.PressDraw() : btn_skip.ReleaseDraw();

    if ( le.MouseClickLeft( btn_skip ) && _currentUnit ) {
        a.push_back( Command( MSG_BATTLE_SKIP, _currentUnit->GetUID(), true ) );
        humanturn_exit = true;
    }
}

int Battle::Interface::GetAllowSwordDirection( u32 index )
{
    int res = 0;

    if ( _currentUnit ) {
        const Indexes around = Board::GetAroundIndexes( index );

        for ( Indexes::const_iterator it = around.begin(); it != around.end(); ++it ) {
            const s32 from = *it;

            if ( UNKNOWN != Board::GetCell( from )->GetDirection() || from == _currentUnit->GetHeadIndex()
                 || ( _currentUnit->isWide() && from == _currentUnit->GetTailIndex() ) ) {
                res |= Board::GetDirection( index, from );
            }
        }
    }

    return res;
}

void Battle::Interface::MousePressRightBoardAction( u32 themes, const Cell & cell, Actions & a )
{
    const s32 index = cell.GetIndex();
    const Unit * b = cell.GetUnit();

    if ( b ) {
        const Settings & conf = Settings::Get();
        const int allow = GetAllowSwordDirection( index );

        if ( arena.GetCurrentColor() == b->GetColor() || !conf.ExtPocketTapMode() || !allow )
            Dialog::ArmyInfo( *b, Dialog::READONLY, b->isReflect() );
        else {
            int res = PocketPC::GetCursorAttackDialog( cell.GetPos(), allow );

            switch ( res ) {
            case Cursor::SWORD_TOPLEFT:
            case Cursor::SWORD_TOPRIGHT:
            case Cursor::SWORD_RIGHT:
            case Cursor::SWORD_BOTTOMRIGHT:
            case Cursor::SWORD_BOTTOMLEFT:
            case Cursor::SWORD_LEFT:
                MouseLeftClickBoardAction( res, cell, a );
                break;

            default:
                Dialog::ArmyInfo( *b, Dialog::READONLY | Dialog::BUTTONS, b->isReflect() );
                break;
            }
        }
    }
}

void Battle::Interface::MouseLeftClickBoardAction( u32 themes, const Cell & cell, Actions & a )
{
    const s32 index = cell.GetIndex();
    const Unit * b = cell.GetUnit();

    if ( Settings::Get().ExtPocketTapMode() && !_currentUnit->isArchers() ) // archers always attack
    {
        // fast tap; attack
        if ( Board::isNearIndexes( index_pos, _currentUnit->GetHeadIndex() ) )
            themes = GetSwordCursorDirection( Board::GetDirection( index, _currentUnit->GetHeadIndex() ) );
        // or show direction attack
        else if ( b ) {
            int res = PocketPC::GetCursorAttackDialog( cell.GetPos(), GetAllowSwordDirection( index ) );

            switch ( res ) {
            case Cursor::SWORD_TOPLEFT:
            case Cursor::SWORD_TOPRIGHT:
            case Cursor::SWORD_RIGHT:
            case Cursor::SWORD_BOTTOMRIGHT:
            case Cursor::SWORD_BOTTOMLEFT:
            case Cursor::SWORD_LEFT:
                themes = res;
                break;

            default:
                Dialog::ArmyInfo( *b, Dialog::READONLY | Dialog::BUTTONS, b->isReflect() );
                break;
            }
        }
    }

    if ( _currentUnit )
        switch ( themes ) {
        case Cursor::WAR_FLY:
        case Cursor::WAR_MOVE:
            a.push_back( Command( MSG_BATTLE_MOVE, _currentUnit->GetUID(), index ) );
            a.push_back( Command( MSG_BATTLE_END_TURN, _currentUnit->GetUID() ) );
            humanturn_exit = true;
            break;

        case Cursor::SWORD_TOPLEFT:
        case Cursor::SWORD_TOPRIGHT:
        case Cursor::SWORD_RIGHT:
        case Cursor::SWORD_BOTTOMRIGHT:
        case Cursor::SWORD_BOTTOMLEFT:
        case Cursor::SWORD_LEFT: {
            const Unit * enemy = b;
            const int dir = GetDirectionFromCursorSword( themes );

            if ( enemy && Board::isValidDirection( index, dir ) ) {
                const s32 move = Board::GetIndexDirection( index, dir );

                if ( _currentUnit->GetHeadIndex() != move )
                    a.push_back( Command( MSG_BATTLE_MOVE, _currentUnit->GetUID(), move ) );
                a.push_back( Command( MSG_BATTLE_ATTACK, _currentUnit->GetUID(), enemy->GetUID(), index, Board::GetReflectDirection( dir ) ) );
                a.push_back( Command( MSG_BATTLE_END_TURN, _currentUnit->GetUID() ) );
                humanturn_exit = true;
            }
            break;
        }

        case Cursor::WAR_BROKENARROW:
        case Cursor::WAR_ARROW: {
            const Unit * enemy = b;

            if ( enemy ) {
                a.push_back( Command( MSG_BATTLE_ATTACK, _currentUnit->GetUID(), enemy->GetUID(), index, 0 ) );
                a.push_back( Command( MSG_BATTLE_END_TURN, _currentUnit->GetUID() ) );
                humanturn_exit = true;
            }
            break;
        }

        case Cursor::WAR_INFO: {
            if ( b ) {
                Dialog::ArmyInfo( *b, Dialog::BUTTONS | Dialog::READONLY, b->isReflect() );
                humanturn_redraw = true;
            }
            break;
        }

        default:
            break;
        }
}

void Battle::Interface::AnimateUnitWithDelay( Unit & unit, uint32_t delay )
{
    if ( unit.isFinishAnimFrame() ) // nothing to animate
        return;

    LocalEvent & le = LocalEvent::Get();
    const uint32_t frameDelay = ( unit.animation.animationLength() > 0 ) ? delay / unit.animation.animationLength() : 0;

    while ( le.HandleEvents( false ) ) {
        CheckGlobalEvents( le );

        if ( Game::AnimateCustomDelay( frameDelay ) ) {
            Redraw();
            if ( unit.isFinishAnimFrame() )
                break;
            unit.IncreaseAnimFrame();
        }
    }
}

void Battle::Interface::AnimateOpponents( OpponentSprite * target )
{
    if ( target == NULL ) // nothing to animate
        return;

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() && !target->isFinishFrame() ) {
        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_OPPONENTS_DELAY ) ) {
            target->IncreaseAnimFrame();
            Redraw();
        }
    }
}

void Battle::Interface::RedrawTroopDefaultDelay( Unit & unit )
{
    if ( unit.isFinishAnimFrame() ) // nothing to animate
        return;

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents( false ) ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_FRAME_DELAY ) ) {
            Redraw();
            if ( unit.isFinishAnimFrame() )
                break;
            unit.IncreaseAnimFrame();
        }
    }
}

void Battle::Interface::RedrawActionSkipStatus( const Unit & attacker )
{
    std::string msg;
    if ( attacker.Modes( TR_HARDSKIP ) ) {
        msg = _( "%{name} skipping turn" );
        if ( Settings::Get().ExtBattleSkipIncreaseDefense() )
            msg.append( _( ", and get +2 defense" ) );
    }
    else
        msg = _( "%{name} waiting turn" );

    StringReplace( msg, "%{name}", attacker.GetName() );
    status.SetMessage( msg, true );
}

void Battle::Interface::RedrawMissileAnimation( const Point & startPos, const Point & endPos, double angle, uint32_t monsterID )
{
    LocalEvent & le = LocalEvent::Get();
    Sprite missile;

    const bool reverse = startPos.x > endPos.x;
    const bool isMage = ( monsterID == Monster::MAGE || monsterID == Monster::ARCHMAGE );

    // Mage is channeling the bolt; doesn't have missile sprite
    if ( isMage )
        DELAY( Game::ApplyBattleSpeed( 115 ) );
    else
        missile = AGG::GetICN( Monster::GetMissileICN( monsterID ), Bin_Info::GetMonsterInfo( monsterID ).getProjectileID( angle ), reverse );

    // Lich/Power lich has projectile speed of 25
    const Points points = GetEuclideanLine( startPos, endPos, isMage ? 50 : std::max( missile.w(), 25 ) );
    Points::const_iterator pnt = points.begin();

    // convert the following code into a function/event service
    while ( le.HandleEvents( false ) && pnt != points.end() ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_MISSILE_DELAY ) ) {
            RedrawPartialStart();
            if ( isMage ) {
                _mainSurface.DrawLine( Point( startPos.x, startPos.y - 2 ), Point( pnt->x, pnt->y - 2 ), PAL::GetPaletteColor( 0x77 ) );
                _mainSurface.DrawLine( Point( startPos.x, startPos.y - 1 ), Point( pnt->x, pnt->y - 1 ), PAL::GetPaletteColor( 0xB5 ) );
                _mainSurface.DrawLine( Point( startPos.x, startPos.y ), Point( pnt->x, pnt->y ), PAL::GetPaletteColor( 0xBC ) );
                _mainSurface.DrawLine( Point( startPos.x, startPos.y + 1 ), Point( pnt->x, pnt->y + 1 ), PAL::GetPaletteColor( 0xB5 ) );
                _mainSurface.DrawLine( Point( startPos.x, startPos.y + 2 ), Point( pnt->x, pnt->y + 2 ), PAL::GetPaletteColor( 0x77 ) );
            }
            else {
                missile.Blit( reverse ? pnt->x - missile.w() : pnt->x, ( angle > 0 ) ? pnt->y - missile.h() : pnt->y, _mainSurface );
            }
            RedrawPartialFinish();
            ++pnt;
        }
    }
}

void Battle::Interface::RedrawActionAttackPart1( Unit & attacker, Unit & defender, const TargetsInfo & targets )
{
    Cursor::Get().SetThemes( Cursor::WAR_NONE );

    _currentUnit = NULL;
    _movingUnit = &attacker;
    _movingPos = attacker.GetRectPosition();

    // Unit 'Position' is position of the tile he's standing at
    const Rect & pos1 = attacker.GetRectPosition();
    const Rect & pos2 = defender.GetRectPosition();

    const bool archer = attacker.isArchers() && !attacker.isHandFighting();
    const bool isDoubleCell = attacker.isDoubleCellAttack() && 2 == targets.size();

    // redraw luck animation
    if ( attacker.Modes( LUCK_GOOD | LUCK_BAD ) )
        RedrawActionLuck( attacker );

    AGG::PlaySound( attacker.M82Attk() );

    // long distance attack animation
    if ( archer ) {
        const Sprite & attackerSprite = AGG::GetICN( attacker.GetMonsterSprite().icn_file, attacker.GetFrame(), attacker.isReflect() );
        const Point attackerPos = GetTroopPosition( attacker, attackerSprite );

        // For shooter position we need bottom center position of rear tile
        // Use cell coordinates for X because sprite width is very inconsistent (e.g. halfling)
        const int rearCenterX = ( attacker.isWide() && attacker.isReflect() ) ? pos1.w * 3 / 4 : CELLW / 2;
        const Point shooterPos( pos1.x + rearCenterX, attackerPos.y - attackerSprite.y() );

        // Use the front one to calculate the angle, then overwrite
        Point offset = attacker.GetStartMissileOffset( Monster_Info::FRONT );

        const Point targetPos = defender.GetCenterPoint();

        const double angle = GetAngle( Point( shooterPos.x + offset.x, shooterPos.y + offset.y ), targetPos );

        // Angles are used in Heroes2 as 90 (TOP) -> 0 (FRONT) -> -90 (BOT) degrees
        const int direction = angle >= 25.0 ? Monster_Info::TOP : ( angle <= -25.0 ) ? Monster_Info::BOTTOM : Monster_Info::FRONT;

        if ( direction != Monster_Info::FRONT )
            offset = attacker.GetStartMissileOffset( direction );

        // redraw archer attack animation
        if ( attacker.SwitchAnimation( Monster_Info::RANG_TOP + direction * 2 ) ) {
            AnimateUnitWithDelay( attacker, Game::ApplyBattleSpeed( attacker.animation.getShootingSpeed() ) );
        }

        const Point missileStart = Point( shooterPos.x + ( attacker.isReflect() ? -offset.x : offset.x ), shooterPos.y + offset.y );

        // draw missile animation
        RedrawMissileAnimation( missileStart, targetPos, angle, attacker.GetID() );
    }
    else {
        int attackAnim = isDoubleCell ? Monster_Info::RANG_FRONT : Monster_Info::MELEE_FRONT;
        if ( pos2.y < pos1.y ) {
            attackAnim -= 2;
        }
        else if ( pos2.y > pos1.y ) {
            attackAnim += 2;
        }

        // redraw melee attack animation
        if ( attacker.SwitchAnimation( attackAnim ) ) {
            RedrawTroopDefaultDelay( attacker );
        }
    }

    // post attack action
    switch ( attacker.GetID() ) {
    case Monster::VAMPIRE_LORD:
        // possible: vampire ressurect animation
        // RedrawTroopWithFrameAnimation(attacker, , );
        break;

    case Monster::LICH:
    case Monster::POWER_LICH:
        // lich clod animation
        if ( archer ) {
            RedrawTroopWithFrameAnimation( defender, ICN::LICHCLOD, attacker.M82Expl(), NONE );
        }
        break;

    default:
        break;
    }
}

void Battle::Interface::RedrawActionAttackPart2( Unit & attacker, TargetsInfo & targets )
{
    // post attack animation
    int attackStart = attacker.animation.getCurrentState();
    if ( attackStart >= Monster_Info::MELEE_TOP && attackStart <= Monster_Info::RANG_BOT ) {
        attacker.SwitchAnimation( ++attackStart );
    }

    // targets damage animation
    RedrawActionWincesKills( targets, &attacker );
    RedrawTroopDefaultDelay( attacker );

    attacker.SwitchAnimation( Monster_Info::STATIC );

    const bool isMirror = targets.size() == 1 && targets.front().defender->isModes( CAP_MIRRORIMAGE );
    // draw status for first defender
    if ( !isMirror && targets.size() ) {
        std::string msg = _( "%{attacker} do %{damage} damage." );
        StringReplace( msg, "%{attacker}", attacker.GetName() );

        if ( 1 < targets.size() ) {
            u32 killed = 0;
            u32 damage = 0;

            for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it ) {
                if ( !it->defender->isModes( CAP_MIRRORIMAGE ) ) {
                    killed += ( *it ).killed;
                    damage += ( *it ).damage;
                }
            }

            StringReplace( msg, "%{damage}", damage );

            if ( killed ) {
                msg.append( " " );
                msg.append( _n( "one creature perishes.", "%{count} creatures perish.", killed ) );
                StringReplace( msg, "%{count}", killed );
            }
        }
        else {
            TargetInfo & target = targets.front();
            StringReplace( msg, "%{damage}", target.damage );

            if ( target.killed ) {
                msg.append( " " );
                msg.append( _n( "one %{defender} perishes.", "%{count} %{defender} perish.", target.killed ) );
                StringReplace( msg, "%{count}", target.killed );
                StringReplace( msg, "%{defender}", target.defender->GetPluralName( target.killed ) );
            }
        }

        status.SetMessage( msg, true );
        status.SetMessage( "", false );
    }

    _movingUnit = NULL;
}

void Battle::Interface::RedrawActionWincesKills( TargetsInfo & targets, Unit * attacker )
{
    const Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();

    // targets damage animation
    int py = ( conf.QVGA() ? 20 : 50 );
    int finish = 0;
    int deathColor = Color::UNUSED;

    std::vector<Unit *> mirrorImages;

    for ( TargetsInfo::iterator it = targets.begin(); it != targets.end(); ++it ) {
        Unit * defender = it->defender;
        if ( defender ) {
            if ( defender->isModes( CAP_MIRRORIMAGE ) )
                mirrorImages.push_back( defender );

            // kill animation
            if ( !defender->isValid() ) {
                // destroy linked mirror
                if ( defender->isModes( CAP_MIRROROWNER ) )
                    mirrorImages.push_back( defender->GetMirror() );

                defender->SwitchAnimation( Monster_Info::KILL );
                AGG::PlaySound( defender->M82Kill() );
                ++finish;

                deathColor = defender->GetArmyColor();
            }
            else
                // wince animation
                if ( it->damage ) {
                // wnce animation
                defender->SwitchAnimation( Monster_Info::WNCE );
                AGG::PlaySound( defender->M82Wnce() );
                ++finish;
            }
            else
            // have immunity
            {
                AGG::PlaySound( M82::RSBRYFZL );
            }
        }
    }

    if ( deathColor != Color::UNUSED ) {
        const bool attackersTurn = deathColor == arena.GetArmyColor2();
        OpponentSprite * attackingHero = attackersTurn ? opponent1 : opponent2;
        OpponentSprite * defendingHero = attackersTurn ? opponent2 : opponent1;
        // 60% of joyful animation
        if ( attackingHero && Rand::Get( 1, 5 ) < 4 ) {
            attackingHero->SetAnimation( OP_JOY );
        }
        // 80% of sorrow animation otherwise
        else if ( defendingHero && Rand::Get( 1, 5 ) < 5 ) {
            defendingHero->SetAnimation( OP_SORROW );
        }
    }

    // targets damage animation loop
    while ( le.HandleEvents() && finish != std::count_if( targets.begin(), targets.end(), TargetInfo::isFinishAnimFrame ) ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_FRAME_DELAY ) ) {
            bool redrawBattleField = false;

            if ( attacker != NULL ) {
                if ( attacker->isFinishAnimFrame() ) {
                    attacker->SwitchAnimation( Monster_Info::STATIC );
                }
                else {
                    attacker->IncreaseAnimFrame();
                }

                redrawBattleField = true;
                RedrawPartialStart();
            }

            for ( TargetsInfo::iterator it = targets.begin(); it != targets.end(); ++it ) {
                if ( ( *it ).defender ) {
                    TargetInfo & target = *it;
                    const Rect & pos = target.defender->GetRectPosition();

                    if ( !redrawBattleField ) {
                        redrawBattleField = true;
                        RedrawPartialStart();
                    }

                    // extended damage info
                    if ( conf.ExtBattleShowDamage() && target.killed && ( pos.y - py ) > 0 ) {
                        std::string msg = "-" + GetString( target.killed );
                        Text txt( msg, Font::YELLOW_SMALL );
                        txt.Blit( pos.x + ( pos.w - txt.w() ) / 2, pos.y - py );
                    }
                }
            }

            if ( redrawBattleField ) {
                RedrawPartialFinish();
            }

            for ( TargetsInfo::iterator it = targets.begin(); it != targets.end(); ++it ) {
                if ( ( *it ).defender ) {
                    if ( it->defender->isFinishAnimFrame() && it->defender->GetAnimationState() == Monster_Info::WNCE ) {
                        it->defender->SwitchAnimation( Monster_Info::STATIC );
                    }
                    else {
                        it->defender->IncreaseAnimFrame();
                    }
                }
            }

            py += ( conf.QVGA() ? 5 : 10 );
        }
    }

    // Fade away animation for destroyed mirror images
    if ( mirrorImages.size() )
        RedrawActionRemoveMirrorImage( mirrorImages );

    // Set to static animation as attacker might still continue its animation
    for ( TargetsInfo::iterator it = targets.begin(); it != targets.end(); ++it ) {
        Unit * unit = ( *it ).defender;
        if ( unit ) {
            if ( unit->isFinishAnimFrame() && unit->GetAnimationState() == Monster_Info::WNCE ) {
                unit->SwitchAnimation( Monster_Info::STATIC );
            }

            if ( !unit->isValid() )
                unit->PostKilledAction();
        }
    }
}

void Battle::Interface::RedrawActionMove( Unit & unit, const Indexes & path )
{
    Indexes::const_iterator dst = path.begin();
    Bridge * bridge = Arena::GetBridge();

    const uint32_t frameDelay = Game::ApplyBattleSpeed( unit.animation.getMoveSpeed() );

    Cursor::Get().SetThemes( Cursor::WAR_NONE );

#ifdef DEBUG
    std::string msg = _( "Moved %{monster}: %{src}, %{dst}" );
    StringReplace( msg, "%{monster}", unit.GetName() );
    StringReplace( msg, "%{src}", unit.GetHeadIndex() );
#else
    std::string msg = _( "Moved %{monster}" );
    StringReplace( msg, "%{monster}", unit.GetName() );
#endif

    _currentUnit = NULL;
    _movingUnit = &unit;

    while ( dst != path.end() ) {
        const Cell * cell = Board::GetCell( *dst );
        _movingPos = cell->GetPos();
        bool show_anim = false;

        if ( bridge && bridge->NeedDown( unit, *dst ) ) {
            _movingUnit = NULL;
            unit.SwitchAnimation( Monster_Info::STATIC );
            bridge->Action( unit, *dst );
            _movingUnit = &unit;
        }

        if ( unit.isWide() ) {
            if ( unit.GetTailIndex() == *dst )
                unit.SetReflection( !unit.isReflect() );
            else
                show_anim = true;
        }
        else {
            unit.UpdateDirection( cell->GetPos() );
            show_anim = true;
        }

        if ( show_anim ) {
            AGG::PlaySound( unit.M82Move() );
            unit.SwitchAnimation( Monster_Info::MOVING );
            AnimateUnitWithDelay( unit, frameDelay );
            unit.SetPosition( *dst );
        }

        // check for possible bridge close action, after unit's end of movement
        if ( bridge && bridge->AllowUp() ) {
            _movingUnit = NULL;
            unit.SwitchAnimation( Monster_Info::STATIC );
            bridge->Action( unit, *dst );
            _movingUnit = &unit;
        }

        ++dst;
    }

    // restore
    _flyingUnit = NULL;
    _movingUnit = NULL;
    _currentUnit = NULL;
    unit.SwitchAnimation( Monster_Info::STATIC );

#ifdef DEBUG
    StringReplace( msg, "%{dst}", unit.GetHeadIndex() );
#endif
    status.SetMessage( msg, true );
}

void Battle::Interface::RedrawActionFly( Unit & unit, const Position & pos )
{
    const s32 destIndex = pos.GetHead()->GetIndex();
    const Rect & pos1 = unit.GetRectPosition();
    const Rect & pos2 = Board::GetCell( destIndex )->GetPos();

    Point destPos( pos1.x, pos1.y );
    Point targetPos( pos2.x, pos2.y );

    if ( unit.isWide() && targetPos.x > destPos.x ) {
        targetPos.x -= CELLW; // this is needed to avoid extra cell shifting upon landing when we move to right side
    }

    std::string msg = _( "Moved %{monster}: %{src}, %{dst}" );
    StringReplace( msg, "%{monster}", unit.GetName() );
    StringReplace( msg, "%{src}", unit.GetHeadIndex() );

    Cursor::Get().SetThemes( Cursor::WAR_NONE );
    const uint32_t step = unit.animation.getFlightSpeed();
    const uint32_t frameDelay = Game::ApplyBattleSpeed( unit.animation.getMoveSpeed() );

    const Points points = GetEuclideanLine( destPos, targetPos, Settings::Get().QVGA() ? step / 2 : step );
    Points::const_iterator currentPoint = points.begin();

    // jump up
    _currentUnit = NULL;
    _movingUnit = NULL;
    _movingPos = currentPoint != points.end() ? *currentPoint : destPos;
    _flyingUnit = NULL;
    _movingUnit = &unit;
    _flyingPos = destPos;

    unit.SwitchAnimation( Monster_Info::FLY_UP );
    // Take off animation is 30% length on average (original value)
    AnimateUnitWithDelay( unit, frameDelay * 0.3 );

    _movingUnit = NULL;
    _flyingUnit = &unit;
    _flyingPos = _movingPos;

    if ( currentPoint != points.end() )
        ++currentPoint;

    unit.SwitchAnimation( Monster_Info::MOVING );
    while ( currentPoint != points.end() ) {
        _movingPos = *currentPoint;

        AGG::PlaySound( unit.M82Move() );
        unit.animation.restartAnimation();
        AnimateUnitWithDelay( unit, frameDelay );

        _flyingPos = _movingPos;
        ++currentPoint;
    }

    unit.SetPosition( destIndex );

    // landing
    _flyingUnit = NULL;
    _movingUnit = &unit;
    _movingPos = targetPos;

    std::vector<int> landAnim;
    landAnim.push_back( Monster_Info::FLY_LAND );
    landAnim.push_back( Monster_Info::STATIC );
    unit.SwitchAnimation( landAnim );
    AnimateUnitWithDelay( unit, frameDelay );

    // restore
    _movingUnit = NULL;

    StringReplace( msg, "%{dst}", unit.GetHeadIndex() );
    status.SetMessage( msg, true );
}

void Battle::Interface::RedrawActionResistSpell( const Unit & target )
{
    AGG::PlaySound( M82::RSBRYFZL );
    std::string str( _( "The %{name} resist the spell!" ) );
    StringReplace( str, "%{name}", target.GetName() );
    status.SetMessage( str, true );
    status.SetMessage( "", false );
}

void Battle::Interface::RedrawActionSpellCastPart1( const Spell & spell, s32 dst, const HeroBase * caster, const std::string & name, const TargetsInfo & targets )
{
    std::string msg;
    Unit * target = targets.size() ? targets.front().defender : NULL;

    if ( target && target->GetHeadIndex() == dst ) {
        msg = _( "%{name} casts %{spell} on the %{troop}." );
        StringReplace( msg, "%{troop}", target->GetName() );
    }
    else if ( spell.isApplyWithoutFocusObject() )
        msg = _( "%{name} casts %{spell}." );

    if ( msg.size() ) {
        StringReplace( msg, "%{name}", name );
        StringReplace( msg, "%{spell}", spell.GetName() );
        status.SetMessage( msg, true );
        status.SetMessage( "", false );
    }

    // set spell cast animation
    if ( caster ) {
        OpponentSprite * opponent = caster->GetColor() == arena.GetArmyColor1() ? opponent1 : opponent2;
        if ( opponent ) {
            opponent->SetAnimation( spell.isApplyWithoutFocusObject() ? OP_CAST_MASS : OP_CAST_UP );
            AnimateOpponents( opponent );
        }
    }

    // without object
    switch ( spell() ) {
    case Spell::FIREBALL:
        RedrawTargetsWithFrameAnimation( dst, targets, ICN::FIREBALL, M82::FromSpell( spell() ) );
        break;
    case Spell::FIREBLAST:
        RedrawTargetsWithFrameAnimation( dst, targets, ICN::FIREBAL2, M82::FromSpell( spell() ) );
        break;
    case Spell::METEORSHOWER:
        RedrawTargetsWithFrameAnimation( dst, targets, ICN::METEOR, M82::FromSpell( spell() ) );
        break;
    case Spell::COLDRING:
        RedrawActionColdRingSpell( dst, targets );
        break;

    case Spell::MASSSHIELD:
        RedrawTargetsWithFrameAnimation( targets, ICN::SHIELD, M82::FromSpell( spell() ), false );
        break;
    case Spell::MASSCURE:
        RedrawTargetsWithFrameAnimation( targets, ICN::MAGIC01, M82::FromSpell( spell() ), false );
        break;
    case Spell::MASSHASTE:
        RedrawTargetsWithFrameAnimation( targets, ICN::HASTE, M82::FromSpell( spell() ), false );
        break;
    case Spell::MASSSLOW:
        RedrawTargetsWithFrameAnimation( targets, ICN::MAGIC02, M82::FromSpell( spell() ), false );
        break;
    case Spell::MASSBLESS:
        RedrawTargetsWithFrameAnimation( targets, ICN::BLESS, M82::FromSpell( spell() ), false );
        break;
    case Spell::MASSCURSE:
        RedrawTargetsWithFrameAnimation( targets, ICN::CURSE, M82::FromSpell( spell() ), false );
        break;
    case Spell::MASSDISPEL:
        RedrawTargetsWithFrameAnimation( targets, ICN::MAGIC07, M82::FromSpell( spell() ), false );
        break;

    case Spell::DEATHRIPPLE:
        RedrawActionDeathWaveSpell( targets, 10 );
        break;
    case Spell::DEATHWAVE:
        RedrawActionDeathWaveSpell( targets, 15 );
        break;

    case Spell::HOLYWORD:
        RedrawActionHolyShoutSpell( targets, 8 );
        break;
    case Spell::HOLYSHOUT:
        RedrawActionHolyShoutSpell( targets, 16 );
        break;

    case Spell::ELEMENTALSTORM:
        RedrawActionElementalStormSpell( targets );
        break;
    case Spell::ARMAGEDDON:
        RedrawActionArmageddonSpell( targets );
        break;

    default:
        break;
    }

    // with object
    if ( target ) {
        if ( spell.isResurrect() )
            RedrawActionResurrectSpell( *target, spell );
        else
            switch ( spell() ) {
            // simple spell animation
            case Spell::BLESS:
                RedrawTroopWithFrameAnimation( *target, ICN::BLESS, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::BLIND:
                RedrawTroopWithFrameAnimation( *target, ICN::BLIND, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::CURE:
                RedrawTroopWithFrameAnimation( *target, ICN::MAGIC01, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::SLOW:
                RedrawTroopWithFrameAnimation( *target, ICN::MAGIC02, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::SHIELD:
                RedrawTroopWithFrameAnimation( *target, ICN::SHIELD, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::HASTE:
                RedrawTroopWithFrameAnimation( *target, ICN::HASTE, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::CURSE:
                RedrawTroopWithFrameAnimation( *target, ICN::CURSE, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::ANTIMAGIC:
                RedrawTroopWithFrameAnimation( *target, ICN::MAGIC06, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::DISPEL:
                RedrawTroopWithFrameAnimation( *target, ICN::MAGIC07, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::STONESKIN:
                RedrawTroopWithFrameAnimation( *target, ICN::STONSKIN, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::STEELSKIN:
                RedrawTroopWithFrameAnimation( *target, ICN::STELSKIN, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::PARALYZE:
                RedrawTroopWithFrameAnimation( *target, ICN::PARALYZE, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::HYPNOTIZE:
                RedrawTroopWithFrameAnimation( *target, ICN::HYPNOTIZ, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::DRAGONSLAYER:
                RedrawTroopWithFrameAnimation( *target, ICN::DRAGSLAY, M82::FromSpell( spell() ), NONE );
                break;
            case Spell::BERSERKER:
                RedrawTroopWithFrameAnimation( *target, ICN::BERZERK, M82::FromSpell( spell() ), NONE );
                break;

            // uniq spell animation
            case Spell::LIGHTNINGBOLT:
                RedrawActionLightningBoltSpell( *target );
                break;
            case Spell::CHAINLIGHTNING:
                RedrawActionChainLightningSpell( targets );
                break;
            case Spell::ARROW:
                RedrawActionArrowSpell( *target );
                break;
            case Spell::COLDRAY:
                RedrawActionColdRaySpell( *target );
                break;
            case Spell::DISRUPTINGRAY:
                RedrawActionDisruptingRaySpell( *target );
                break;
            case Spell::BLOODLUST:
                RedrawActionBloodLustSpell( *target );
                break;
            case Spell::STONE:
                RedrawActionStoneSpell( *target );
                break;
            default:
                break;
            }
    }

    if ( caster ) {
        OpponentSprite * opponent = caster->GetColor() == arena.GetArmyColor1() ? opponent1 : opponent2;
        if ( opponent ) {
            opponent->SetAnimation( ( target ) ? OP_CAST_UP_RETURN : OP_CAST_MASS_RETURN );
            AnimateOpponents( opponent );
        }
    }
}

void Battle::Interface::RedrawActionSpellCastPart2( const Spell & spell, TargetsInfo & targets )
{
    if ( spell.isDamage() ) {
        u32 killed = 0;
        u32 damage = 0;

        for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it ) {
            if ( !it->defender->isModes( CAP_MIRRORIMAGE ) ) {
                killed += ( *it ).killed;
                damage += ( *it ).damage;
            }
        }

        // targets damage animation
        RedrawActionWincesKills( targets );

        if ( damage ) {
            std::string msg;
            if ( spell.isUndeadOnly() )
                msg = _( "The %{spell} spell does %{damage} damage to all undead creatures." );
            else if ( spell.isALiveOnly() )
                msg = _( "The %{spell} spell does %{damage} damage to all living creatures." );
            else
                msg = _( "The %{spell} does %{damage} damage." );
            StringReplace( msg, "%{spell}", spell.GetName() );
            StringReplace( msg, "%{damage}", damage );

            if ( killed ) {
                status.SetMessage( msg, true );
                msg = _n( "one creature perishes.", "%{count} creatures perish.", killed );
                StringReplace( msg, "%{count}", killed );
            }

            status.SetMessage( msg, true );
        }
    }

    status.SetMessage( " ", false );
    _movingUnit = NULL;
}

void Battle::Interface::RedrawActionMonsterSpellCastStatus( const Unit & attacker, const TargetInfo & target )
{
    const char * msg = NULL;

    switch ( attacker.GetID() ) {
    case Monster::UNICORN:
        msg = _( "The Unicorns attack blinds the %{name}!" );
        break;
    case Monster::MEDUSA:
        msg = _( "The Medusas gaze turns the %{name} to stone!" );
        break;
    case Monster::ROYAL_MUMMY:
    case Monster::MUMMY:
        msg = _( "The Mummies' curse falls upon the %{name}!" );
        break;
    case Monster::CYCLOPS:
        msg = _( "The %{name} are paralyzed by the Cyclopes!" );
        break;
    case Monster::ARCHMAGE:
        msg = _( "The Archmagi dispel all good spells on your %{name}!" );
        break;
    default:
        break;
    }

    if ( msg ) {
        std::string str( msg );
        StringReplace( str, "%{name}", target.defender->GetName() );

        status.SetMessage( str, true );
        status.SetMessage( "", false );
    }
}

void Battle::Interface::RedrawActionLuck( Unit & unit )
{
    LocalEvent & le = LocalEvent::Get();

    const bool isGoodLuck = unit.Modes( LUCK_GOOD );
    const Rect & pos = unit.GetRectPosition();

    std::string msg = isGoodLuck ? _( "Good luck shines on the %{attacker}" ) : _( "Bad luck descends on the %{attacker}" );
    StringReplace( msg, "%{attacker}", unit.GetName() );
    status.SetMessage( msg, true );

    Cursor::Get().SetThemes( Cursor::WAR_NONE );
    if ( isGoodLuck ) {
        const Sprite & luckSprite = AGG::GetICN( ICN::EXPMRL, 0 );
        const Sprite & unitSprite = AGG::GetICN( unit.GetMonsterSprite().icn_file, unit.GetFrame(), unit.isReflect() );

        int width = 2;
        Rect src( 0, 0, width, luckSprite.h() );
        src.x = ( luckSprite.w() - src.w ) / 2;
        int y = pos.y + pos.h - unitSprite.h() - src.h;
        if ( y < 0 )
            y = 0;

        AGG::PlaySound( M82::GOODLUCK );

        while ( le.HandleEvents() && Mixer::isPlaying( -1 ) ) {
            CheckGlobalEvents( le );

            if ( width < luckSprite.w() && Battle::AnimateInfrequentDelay( Game::BATTLE_MISSILE_DELAY ) ) {
                RedrawPartialStart();

                luckSprite.Blit( src, pos.x + ( pos.w - src.w ) / 2, y, _mainSurface );

                RedrawPartialFinish();

                src.w = width;
                src.x = ( luckSprite.w() - src.w ) / 2;

                width += 3;
            }
        }
    }
    else {
        const int maxHeight = AGG::GetAbsoluteICNHeight( ICN::CLOUDLUK );
        int y = pos.y + pos.h - 10;

        // move drawing position if it will clip outside of the battle window
        if ( y - maxHeight < 0 )
            y = maxHeight;

        AGG::PlaySound( M82::BADLUCK );

        int frameId = 0;
        while ( le.HandleEvents() && Mixer::isPlaying( -1 ) ) {
            CheckGlobalEvents( le );

            if ( frameId < 8 && Battle::AnimateInfrequentDelay( Game::BATTLE_MISSILE_DELAY ) ) {
                RedrawPartialStart();

                const Sprite & luckSprite = AGG::GetICN( ICN::CLOUDLUK, frameId );
                luckSprite.Blit( pos.x + pos.w / 2 + luckSprite.x(), y + luckSprite.y(), _mainSurface );

                RedrawPartialFinish();

                ++frameId;
            }
        }
    }
}

void Battle::Interface::RedrawActionMorale( Unit & b, bool good )
{
    std::string msg;

    if ( good ) {
        msg = _( "High morale enables the %{monster} to attack again." );
        StringReplace( msg, "%{monster}", b.GetName() );
        status.SetMessage( msg, true );
        RedrawTroopWithFrameAnimation( b, ICN::MORALEG, M82::GOODMRLE, NONE );
    }
    else {
        msg = _( "Low morale causes the %{monster} to freeze in panic." );
        StringReplace( msg, "%{monster}", b.GetName() );
        status.SetMessage( msg, true );
        RedrawTroopWithFrameAnimation( b, ICN::MORALEB, M82::BADMRLE, WINCE );
    }
}

void Battle::Interface::RedrawActionTowerPart1( Tower & tower, Unit & defender )
{
    Cursor::Get().SetThemes( Cursor::WAR_NONE );
    _currentUnit = NULL;

    const Point missileStart = tower.GetPortPosition();
    const Point targetPos = defender.GetCenterPoint();
    const double angle = GetAngle( missileStart, targetPos );

    AGG::PlaySound( M82::KEEPSHOT );

    // Keep missile == Orc missile
    RedrawMissileAnimation( missileStart, targetPos, angle, Monster::ORC );
}

void Battle::Interface::RedrawActionTowerPart2( Tower & tower, TargetInfo & target )
{
    TargetsInfo targets;
    targets.push_back( target );
    const bool isMirror = target.defender->isModes( CAP_MIRRORIMAGE );

    // targets damage animation
    RedrawActionWincesKills( targets );

    // draw status for first defender
    std::string msg = _( "Tower does %{damage} damage." );
    StringReplace( msg, "%{damage}", target.damage );
    if ( target.killed ) {
        msg.append( " " );
        msg.append( _n( "one %{defender} perishes.", "%{count} %{defender} perish.", target.killed ) );
        StringReplace( msg, "%{count}", target.killed );
        StringReplace( msg, "%{defender}", target.defender->GetName() );
    }

    if ( !isMirror ) {
        status.SetMessage( msg, true );
        status.SetMessage( "", false );
    }

    _movingUnit = NULL;
}

void Battle::Interface::RedrawActionCatapult( int target )
{
    LocalEvent & le = LocalEvent::Get();

    const Sprite & missile = AGG::GetICN( ICN::BOULDER, 0 );
    const Rect & area = GetArea();

    AGG::PlaySound( M82::CATSND00 );

    // catapult animation
    while ( le.HandleEvents( false ) && catapult_frame < 6 ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_CATAPULT_DELAY ) ) {
            Redraw();
            ++catapult_frame;
        }
    }

    // boulder animation
    Point pt1( 90, 220 );
    Point pt2 = Catapult::GetTargetPosition( target );
    Point max( 300, 20 );

    pt1.x += area.x;
    pt2.x += area.x;
    max.x += area.x;
    pt1.y += area.y;
    pt2.y += area.y;
    max.y += area.y;

    const Points points = GetArcPoints( pt1, pt2, max, missile.w() );
    Points::const_iterator pnt = points.begin();

    while ( le.HandleEvents( false ) && pnt != points.end() ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_CATAPULT2_DELAY ) ) {
            if ( catapult_frame < 9 )
                ++catapult_frame;

            RedrawPartialStart();
            missile.Blit( *pnt, _mainSurface );
            RedrawPartialFinish();
            ++pnt;
        }
    }

    // clod
    u32 frame = 0;
    int icn = target == CAT_MISS ? ICN::SMALCLOD : ICN::LICHCLOD;
    AGG::PlaySound( M82::CATSND02 );

    while ( le.HandleEvents() && frame < AGG::GetICNCount( icn ) ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_CATAPULT3_DELAY ) ) {
            if ( catapult_frame < 9 )
                ++catapult_frame;

            RedrawPartialStart();
            const Sprite & sprite = AGG::GetICN( icn, frame );
            sprite.Blit( pt2.x + sprite.x(), pt2.y + sprite.y(), _mainSurface );
            RedrawPartialFinish();

            ++frame;
        }
    }

    catapult_frame = 0;
}

void Battle::Interface::RedrawActionArrowSpell( const Unit & target )
{
    const HeroBase * caster = arena.GetCurrentCommander();

    if ( caster ) {
        const Point missileStart = caster == opponent1->GetHero() ? opponent1->GetCastPosition() : opponent2->GetCastPosition();

        const Point targetPos = target.GetCenterPoint();
        const double angle = GetAngle( missileStart, targetPos );

        Cursor::Get().SetThemes( Cursor::WAR_NONE );
        AGG::PlaySound( M82::MAGCAROW );

        // Magic arrow == Archer missile
        RedrawMissileAnimation( missileStart, targetPos, angle, Monster::ARCHER );
    }
}

void Battle::Interface::RedrawActionTeleportSpell( Unit & target, s32 dst )
{
    LocalEvent & le = LocalEvent::Get();

    const Monster::monstersprite_t & msi = target.GetMonsterSprite();
    Sprite sprite = AGG::GetICN( msi.icn_file, target.GetFrame(), target.isReflect() );
    sprite = Sprite( sprite.GetSurface(), sprite.x(), sprite.y() );

    if ( target.Modes( SP_STONE ) ) {
        AGG::ReplaceColors( sprite, PAL::GetPalette( PAL::GRAY ), msi.icn_file, target.GetFrame(), target.isReflect() );
    }
    else if ( target.Modes( CAP_MIRRORIMAGE ) ) {
        AGG::ReplaceColors( sprite, PAL::GetPalette( PAL::MIRROR_IMAGE ), msi.icn_file, target.GetFrame(), target.isReflect() );
    }

    Cursor::Get().SetThemes( Cursor::WAR_NONE );

    uint32_t currentAlpha = target.GetCustomAlpha();

    AGG::PlaySound( M82::TELPTOUT );

    while ( le.HandleEvents() && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( currentAlpha > 0 && Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            currentAlpha -= 15;
            target.SetCustomAlpha( currentAlpha );
            Redraw();
        }
    }

    currentAlpha = 0;
    Redraw();

    target.SetPosition( dst );
    AGG::PlaySound( M82::TELPTIN );

    while ( le.HandleEvents() && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( currentAlpha <= 240 && Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            currentAlpha += 15;
            target.SetCustomAlpha( currentAlpha );
            Redraw();
        }
    }

    target.SetCustomAlpha( 255 );
}

void Battle::Interface::RedrawActionSummonElementalSpell( Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    Cursor::Get().SetThemes( Cursor::WAR_NONE );

    uint32_t currentAlpha = 0;

    AGG::PlaySound( M82::SUMNELM );

    while ( le.HandleEvents() && currentAlpha < 220 ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            currentAlpha += 20;
            target.SetCustomAlpha( currentAlpha );
            Redraw();
        }
    }

    target.SetCustomAlpha( 255 );
}

void Battle::Interface::RedrawActionMirrorImageSpell( const Unit & target, const Position & pos )
{
    LocalEvent & le = LocalEvent::Get();

    const Monster::monstersprite_t & msi = target.GetMonsterSprite();
    Sprite sprite = AGG::GetICN( msi.icn_file, target.GetFrame(), target.isReflect() );
    sprite = Sprite( sprite.GetSurface(), sprite.x(), sprite.y() );
    AGG::ReplaceColors( sprite, PAL::GetPalette( PAL::MIRROR_IMAGE ), msi.icn_file, target.GetFrame(), target.isReflect() );

    const Rect & rt1 = target.GetRectPosition();
    const Rect & rt2 = pos.GetRect();

    const Points points = GetLinePoints( rt1, rt2, 5 );
    Points::const_iterator pnt = points.begin();

    Cursor::Get().SetThemes( Cursor::WAR_NONE );
    AGG::PlaySound( M82::MIRRORIM );

    while ( le.HandleEvents() && pnt != points.end() ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            const Point & sp = GetTroopPosition( target, sprite );

            RedrawPartialStart();
            sprite.Blit( sp.x - rt1.x + ( *pnt ).x, sp.y - rt1.y + ( *pnt ).y, _mainSurface );
            RedrawPartialFinish();

            ++pnt;
        }
    }

    status.SetMessage( _( "MirrorImage created" ), true );
}

void Battle::Interface::RedrawLightningOnTargets( const std::vector<Point> & points, const Rect & drawRoi )
{
    if ( points.size() < 2 )
        return;

    const Point roiOffset( drawRoi.x, drawRoi.y );

    LocalEvent & le = LocalEvent::Get();
    Cursor & cursor = Cursor::Get();
    cursor.SetThemes( Cursor::WAR_NONE );

    AGG::PlaySound( points.size() > 2 ? M82::CHAINLTE : M82::LIGHTBLT );

    Surface whiteSurface( _surfaceInnerArea, false );
    whiteSurface.Fill( RGBA( 0xFF, 0xFF, 0xFF ) );

    for ( size_t i = 1; i < points.size(); ++i ) {
        const Point & startingPos = points[i - 1];
        const Point & endPos = points[i];

        const std::vector<std::pair<LightningPoint, LightningPoint> > & lightningBolt = GenerateLightning( startingPos + roiOffset, endPos + roiOffset );
        Rect roi;
        const bool isHorizontalBolt = std::abs( startingPos.x - endPos.x ) > std::abs( startingPos.y - endPos.y );
        const bool isForwardDirection = isHorizontalBolt ? ( endPos.x > startingPos.x ) : ( endPos.y > startingPos.y );
        const int animationStep = 100;

        if ( isHorizontalBolt ) {
            roi.h = drawRoi.h;
            if ( isForwardDirection ) {
                roi.x = 0;
                roi.w = startingPos.x;
            }
            else {
                roi.x = startingPos.x;
                roi.w = drawRoi.w - startingPos.x;
            }
        }
        else {
            roi.w = drawRoi.w;
            if ( isForwardDirection ) {
                roi.y = 0;
                roi.h = startingPos.y;
            }
            else {
                roi.y = startingPos.y;
                roi.h = drawRoi.h - startingPos.y;
            }
        }

        while ( le.HandleEvents() && ( ( isHorizontalBolt && roi.w < drawRoi.w ) || ( !isHorizontalBolt && roi.h < drawRoi.h ) ) ) {
            if ( Battle::AnimateInfrequentDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
                if ( isHorizontalBolt ) {
                    if ( isForwardDirection ) {
                        roi.w += animationStep;
                    }
                    else {
                        roi.w += animationStep;
                        roi.x -= animationStep;
                    }

                    if ( roi.x < 0 )
                        roi.x = 0;
                    if ( roi.w > drawRoi.w )
                        roi.w = drawRoi.w;
                }
                else {
                    if ( isForwardDirection ) {
                        roi.h += animationStep;
                    }
                    else {
                        roi.h += animationStep;
                        roi.y -= animationStep;
                    }

                    if ( roi.y < 0 )
                        roi.y = 0;
                    if ( roi.h > drawRoi.h )
                        roi.h = drawRoi.h;
                }

                RedrawPartialStart();

                RedrawLightning( lightningBolt, RGBA( 0xff, 0xff, 0 ), _mainSurface, Rect( roi.x + roiOffset.x, roi.y + roiOffset.y, roi.w, roi.h ) );
                Surface::Blend( _mainSurface, whiteSurface, 80 ).Blit( _mainSurface );

                RedrawPartialFinish();
            }
        }
    }

    // small delay to display fully drawn lightning
    DELAY( 100 );

    uint32_t frame = 0;
    while ( le.HandleEvents() && frame < AGG::GetICNCount( ICN::SPARKS ) ) {
        CheckGlobalEvents( le );

        if ( ( frame == 0 ) || Battle::AnimateInfrequentDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            RedrawPartialStart();

            Sprite sprite = AGG::GetICN( ICN::SPARKS, frame, false );

            for ( size_t i = 1; i < points.size(); ++i ) {
                sprite.Blit( points[i] - Point( sprite.w() / 2, 0 ) + roiOffset, _mainSurface );
            }
            RedrawPartialFinish();

            ++frame;
        }
    }
}

void Battle::Interface::RedrawActionLightningBoltSpell( Unit & target )
{
    _currentUnit = NULL;

    const Point startingPos = arena.GetCurrentCommander() == opponent1->GetHero() ? opponent1->GetCastPosition() : opponent2->GetCastPosition();
    const Rect & pos = target.GetRectPosition();
    const Point endPos( pos.x + pos.w / 2, pos.y );

    std::vector<Point> points;
    points.push_back( startingPos );
    points.push_back( endPos );

    RedrawLightningOnTargets( points, _surfaceInnerArea );
}

void Battle::Interface::RedrawActionChainLightningSpell( const TargetsInfo & targets )
{
    const Point startingPos = arena.GetCurrentCommander() == opponent1->GetHero() ? opponent1->GetCastPosition() : opponent2->GetCastPosition();
    std::vector<Point> points;
    points.push_back( startingPos );

    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it ) {
        const Rect & pos = it->defender->GetRectPosition();
        points.push_back( Point( pos.x + pos.w / 2, pos.y ) );
    }

    RedrawLightningOnTargets( points, _surfaceInnerArea );
}

void Battle::Interface::RedrawActionBloodLustSpell( Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    const Monster::monstersprite_t & msi = target.GetMonsterSprite();
    Sprite unitSprite = AGG::GetICN( msi.icn_file, target.GetFrame(), target.isReflect() );
    unitSprite = Sprite( unitSprite.GetSurface(), unitSprite.x(), unitSprite.y() );

    std::vector<std::vector<uint8_t> > originalPalette;
    if ( target.Modes( SP_STONE ) ) {
        originalPalette.push_back( PAL::GetPalette( PAL::GRAY ) );
    }
    else if ( target.Modes( CAP_MIRRORIMAGE ) ) {
        originalPalette.push_back( PAL::GetPalette( PAL::MIRROR_IMAGE ) );
    }

    if ( !originalPalette.empty() ) {
        for ( size_t i = 1; i < originalPalette.size(); ++i ) {
            originalPalette[0] = PAL::CombinePalettes( originalPalette[0], originalPalette[i] );
        }
        AGG::ReplaceColors( unitSprite, originalPalette[0], msi.icn_file, target.GetFrame(), target.isReflect() );
    }

    std::vector<uint8_t> convert = PAL::GetPalette( PAL::RED );
    if ( !originalPalette.empty() ) {
        convert = PAL::CombinePalettes( PAL::GetPalette( PAL::GRAY ), convert );
    }

    Sprite bloodlustEffect( unitSprite.GetSurface(), unitSprite.x(), unitSprite.y() );
    AGG::ReplaceColors( bloodlustEffect, convert, msi.icn_file, target.GetFrame(), target.isReflect() );

    Sprite mixSprite( Surface( unitSprite.GetSize(), true ), unitSprite.x(), unitSprite.y() );

    Cursor::Get().SetThemes( Cursor::WAR_NONE );

    _currentUnit = &target;
    b_current_sprite = &mixSprite;

    const uint32_t bloodlustDelay = 1800 / 20;
    // duration is 1900ms
    AGG::PlaySound( M82::BLOODLUS );

    uint32_t alpha = 0;
    uint32_t frame = 0;
    while ( le.HandleEvents() && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( frame < 20 && Game::AnimateCustomDelay( bloodlustDelay ) ) {
            mixSprite = Sprite( Surface::Blend( unitSprite, bloodlustEffect, ( 255 - alpha ) * 100 / 255 ), unitSprite.x(), unitSprite.y() );
            Redraw();

            alpha += ( frame < 10 ) ? 20 : -20;
            ++frame;
        }
    }

    _currentUnit = NULL;
    b_current_sprite = NULL;
}

void Battle::Interface::RedrawActionStoneSpell( Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    const Monster::monstersprite_t & msi = target.GetMonsterSprite();
    Sprite unitSprite = AGG::GetICN( msi.icn_file, target.GetFrame(), target.isReflect() );
    unitSprite = Sprite( unitSprite.GetSurface(), unitSprite.x(), unitSprite.y() );

    Sprite stoneEffect( unitSprite.GetSurface(), unitSprite.x(), unitSprite.y() );
    AGG::ReplaceColors( stoneEffect, PAL::GetPalette( PAL::GRAY ), msi.icn_file, target.GetFrame(), target.isReflect() );

    Sprite mixSprite( Surface( unitSprite.GetSize(), true ), unitSprite.x(), unitSprite.y() );

    Cursor::Get().SetThemes( Cursor::WAR_NONE );

    _currentUnit = &target;
    b_current_sprite = &mixSprite;

    AGG::PlaySound( M82::PARALIZE );

    uint32_t alpha = 0;
    uint32_t frame = 0;
    while ( le.HandleEvents() && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( frame < 25 && Game::AnimateCustomDelay( Game::BATTLE_SPELL_DELAY ) ) {
            mixSprite = Sprite( Surface::Blend( unitSprite, stoneEffect, ( 255 - alpha ) * 100 / 255 ), unitSprite.x(), unitSprite.y() );
            Redraw();

            alpha += 10;
            ++frame;
        }
    }

    _currentUnit = NULL;
    b_current_sprite = NULL;
}

void Battle::Interface::RedrawActionResurrectSpell( Unit & target, const Spell & spell )
{
    LocalEvent & le = LocalEvent::Get();

    if ( !target.isValid() ) {
        while ( le.HandleEvents() && !target.isFinishAnimFrame() ) {
            CheckGlobalEvents( le );

            if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
                Redraw();
                target.IncreaseAnimFrame();
            }
        }
    }

    AGG::PlaySound( M82::FromSpell( spell() ) );

    RedrawTroopWithFrameAnimation( target, ICN::YINYANG, M82::UNKNOWN, target.GetHitPoints() == 0 ? RESURRECT : NONE );
}

void Battle::Interface::RedrawActionColdRaySpell( Unit & target )
{
    RedrawRaySpell( target, ICN::COLDRAY, M82::COLDRAY, 18 );
    RedrawTroopWithFrameAnimation( target, ICN::ICECLOUD, M82::UNKNOWN, WINCE );
}

void Battle::Interface::RedrawRaySpell( const Unit & target, int spellICN, int spellSound, uint32_t size )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    // Casting hero position
    const Point startingPos = arena.GetCurrentCommander() == opponent1->GetHero() ? opponent1->GetCastPosition() : opponent2->GetCastPosition();
    const Point targetPos = target.GetCenterPoint();

    const Points path = GetEuclideanLine( startingPos, targetPos, size );
    const uint32_t spriteCount = AGG::GetICNCount( spellICN );

    cursor.SetThemes( Cursor::WAR_NONE );
    AGG::PlaySound( spellSound );

    size_t i = 0;
    while ( le.HandleEvents() && i < path.size() ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            cursor.Hide();
            const uint32_t frame = i * spriteCount / path.size();
            const Sprite & sprite = AGG::GetICN( spellICN, frame );
            sprite.Blit( path[i].x - sprite.w() / 2, path[i].y - sprite.h() / 2, _mainSurface );
            RedrawPartialFinish();
            ++i;
        }
    }
}

void Battle::Interface::RedrawActionDisruptingRaySpell( Unit & target )
{
    LocalEvent & le = LocalEvent::Get();

    RedrawRaySpell( target, ICN::DISRRAY, M82::DISRUPTR, 24 );

    // Part 2 - ripple effect
    const Sprite & unitSprite = AGG::GetICN( target.GetMonsterSprite().icn_file, target.GetFrame(), target.isReflect() );
    Sprite rippleSprite;

    const Unit * old_current = _currentUnit;
    _currentUnit = &target;
    _movingPos = Point( 0, 0 );

    uint32_t frame = 0;
    while ( le.HandleEvents() && frame < 60 ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            rippleSprite = Sprite( unitSprite.RenderRippleEffect( frame ), unitSprite.GetPos().x, unitSprite.GetPos().y );
            b_current_sprite = &rippleSprite;
            Redraw();

            frame += 2;
        }
    }

    _currentUnit = old_current;
    b_current_sprite = NULL;
}

void Battle::Interface::RedrawActionDeathWaveSpell( const TargetsInfo & targets, int strength )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    _currentUnit = NULL;
    cursor.SetThemes( Cursor::WAR_NONE );

    Rect area = GetArea();
    area.h -= Settings::Get().QVGA() ? 18 : 36;

    Surface copy = _mainSurface.GetSurface( area );
    const int waveLength = strength * 2 + 10;

    AGG::PlaySound( M82::MNRDEATH );

    int position = 10;
    while ( le.HandleEvents() && position < area.w + waveLength ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_DISRUPTING_DELAY ) ) {
            cursor.Hide();
            copy.RenderDeathWave( position, waveLength, strength ).Blit( _mainSurface );
            cursor.Show();
            RedrawPartialFinish();

            position += 3;
        }
    }

    RedrawTargetsWithFrameAnimation( targets, ICN::REDDEATH, M82::UNKNOWN, true );
}

void Battle::Interface::RedrawActionColdRingSpell( s32 dst, const TargetsInfo & targets )
{
    LocalEvent & le = LocalEvent::Get();

    const int icn = ICN::COLDRING;
    const int m82 = M82::FromSpell( Spell::COLDRING );
    u32 frame = 0;
    const Rect & center = Board::GetCell( dst )->GetPos();

    Cursor::Get().SetThemes( Cursor::WAR_NONE );

    // set WNCE
    _currentUnit = NULL;
    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender && ( *it ).damage )
            ( *it ).defender->SwitchAnimation( Monster_Info::WNCE );

    if ( M82::UNKNOWN != m82 )
        AGG::PlaySound( m82 );

    while ( le.HandleEvents() && frame < AGG::GetICNCount( icn ) ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            RedrawPartialStart();

            const Sprite & sprite1 = AGG::GetICN( icn, frame );
            sprite1.Blit( center.x + center.w / 2 + sprite1.x(), center.y + center.h / 2 + sprite1.y(), _mainSurface );
            const Sprite & sprite2 = AGG::GetICN( icn, frame, true );
            sprite2.Blit( center.x + center.w / 2 - sprite2.w() - sprite2.x(), center.y + center.h / 2 + sprite2.y(), _mainSurface );
            RedrawPartialFinish();

            for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
                if ( ( *it ).defender && ( *it ).damage )
                    ( *it ).defender->IncreaseAnimFrame( false );
            ++frame;
        }
    }

    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender ) {
            ( *it ).defender->SwitchAnimation( Monster_Info::STATIC );
            _currentUnit = NULL;
        }
}

void Battle::Interface::RedrawActionHolyShoutSpell( const TargetsInfo & targets, int strength )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.SetThemes( Cursor::WAR_NONE );

    Surface original = _mainSurface.GetSurface();
    Surface blurred = _mainSurface.RenderBoxBlur( 2, -strength, true );

    _currentUnit = NULL;
    AGG::PlaySound( M82::MASSCURS );

    const uint32_t spellcastDelay = Game::ApplyBattleSpeed( 3000 ) / 20;
    uint32_t frame = 0;
    uint8_t alpha = 30;

    while ( le.HandleEvents() && frame < 20 ) {
        CheckGlobalEvents( le );

        if ( Game::AnimateCustomDelay( spellcastDelay ) ) {
            // stay at maximum blur for 2 frames
            if ( frame < 9 || frame > 10 ) {
                cursor.Hide();
                Surface::Blend( original, blurred, ( 255 - alpha ) * 100 / 255 ).Blit( _mainSurface );
                cursor.Show();
                RedrawPartialFinish();

                alpha += ( frame < 10 ) ? 25 : -25;
            }
            ++frame;
        }
    }

    RedrawTargetsWithFrameAnimation( targets, ICN::MAGIC08, M82::UNKNOWN, true );
}

void Battle::Interface::RedrawActionElementalStormSpell( const TargetsInfo & targets )
{
    LocalEvent & le = LocalEvent::Get();

    const int icn = ICN::STORM;
    const int m82 = M82::FromSpell( Spell::ELEMENTALSTORM );
    const int spriteSize = 54;
    const uint32_t icnCount = AGG::GetICNCount( icn );

    std::vector<Sprite> spriteCache;
    spriteCache.reserve( icnCount );
    for ( uint32_t i = 0; i < icnCount; ++i ) {
        spriteCache.push_back( AGG::GetICN( icn, i ) );
    }

    Cursor::Get().SetThemes( Cursor::WAR_NONE );

    _currentUnit = NULL;
    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender && ( *it ).damage )
            ( *it ).defender->SwitchAnimation( Monster_Info::WNCE );

    if ( M82::UNKNOWN != m82 )
        AGG::PlaySound( m82 );

    uint32_t frame = 0;
    while ( le.HandleEvents() && frame < 60 ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            RedrawPartialStart();

            for ( int x = 0; x * spriteSize < _surfaceInnerArea.w; ++x ) {
                const int idX = frame + x * 3;
                const int offsetX = x * spriteSize;
                for ( int y = 0; y * spriteSize < _surfaceInnerArea.h; ++y ) {
                    const Sprite & sprite = spriteCache[( idX + y ) % icnCount];
                    sprite.Blit( Point( offsetX + sprite.x(), y * spriteSize + sprite.y() ), _mainSurface );
                }
            }

            RedrawPartialFinish();

            for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
                if ( ( *it ).defender && ( *it ).damage )
                    ( *it ).defender->IncreaseAnimFrame( false );
            ++frame;
        }
    }

    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender ) {
            ( *it ).defender->SwitchAnimation( Monster_Info::STATIC );
            _currentUnit = NULL;
        }
}

void Battle::Interface::RedrawActionArmageddonSpell( const TargetsInfo & targets )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Rect area = GetArea();

    area.h -= Settings::Get().QVGA() ? 18 : 36;

    Surface sprite1( area, false );
    Surface sprite2( area, false );

    cursor.SetThemes( Cursor::WAR_NONE );

    _mainSurface.Blit( area, 0, 0, sprite1 );
    sprite2.Fill( RGBA( 0xFF, 0xFF, 0xFF ) );

    _currentUnit = NULL;
    AGG::PlaySound( M82::ARMGEDN );
    u32 alpha = 10;

    while ( le.HandleEvents() && alpha < 180 ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            cursor.Hide();
            Surface::Blend( sprite1, sprite2, ( 255 - alpha ) * 100 / 255 ).Blit( area.x, area.y, _mainSurface );
            cursor.Show();
            RedrawPartialFinish();

            alpha += 10;
        }
    }

    sprite2.Fill( RGBA( 0xb0, 0x0c, 0 ) );
    sprite1 = Surface::Blend( sprite1, sprite2, ( 255 - alpha ) * 100 / 255 );

    cursor.Hide();

    while ( le.HandleEvents() && Mixer::isPlaying( -1 ) ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            cursor.Hide();

            const int16_t offsetX = Rand::Get( -7, 7 );
            const int16_t offsetY = Rand::Get( -7, 7 );
            const Rect initialArea( area.x, area.y, area.w, area.h );
            const Rect original = initialArea ^ Rect( area.x + offsetX, area.y + offsetY, area.w, area.h );

            const Rect shifted( initialArea.x - original.x, initialArea.y - original.y, original.w, original.h );
            sprite1.Blit( shifted, original, _mainSurface );

            RedrawPartialFinish();
        }
    }
}

void Battle::Interface::RedrawActionEarthQuakeSpell( const std::vector<int> & targets )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Rect area = GetArea();

    u32 frame = 0;
    area.h -= Settings::Get().QVGA() ? 19 : 38;

    cursor.SetThemes( Cursor::WAR_NONE );

    Surface sprite = _mainSurface.GetSurface( area );

    _currentUnit = NULL;
    AGG::PlaySound( M82::ERTHQUAK );

    const u32 offset = Settings::Get().QVGA() ? 5 : 10;

    // draw earth quake
    while ( le.HandleEvents() && frame < 18 ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            cursor.Hide();

            const int16_t offsetX = Rand::Get( -7, 7 );
            const int16_t offsetY = Rand::Get( -7, 7 );
            const Rect initialArea( area.x, area.y, area.w, area.h );
            const Rect original = initialArea ^ Rect( area.x + offsetX, area.y + offsetY, area.w, area.h );

            const Rect shifted( initialArea.x - original.x, initialArea.y - original.y, original.w, original.h );
            sprite.Blit( shifted, original, _mainSurface );

            RedrawPartialFinish();
            ++frame;
        }
    }

    // draw cloud
    frame = 0;
    int icn = ICN::LICHCLOD;
    AGG::PlaySound( M82::CATSND02 );

    while ( le.HandleEvents() && frame < AGG::GetICNCount( icn ) ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            RedrawPartialStart();

            for ( std::vector<int>::const_iterator it = targets.begin(); it != targets.end(); ++it ) {
                Point pt2 = Catapult::GetTargetPosition( *it );

                if ( Settings::Get().QVGA() ) {
                    pt2.x /= 2;
                    pt2.y /= 2;
                }
                pt2.x += area.x;
                pt2.y += area.y;

                const Sprite & spriteCloud = AGG::GetICN( icn, frame );
                spriteCloud.Blit( pt2.x + spriteCloud.x(), pt2.y + spriteCloud.y(), _mainSurface );
            }

            RedrawPartialFinish();

            ++frame;
        }
    }
}

void Battle::Interface::RedrawActionRemoveMirrorImage( const std::vector<Unit *> & mirrorImages )
{
    if ( mirrorImages.empty() ) // nothing to animate
        return;

    LocalEvent & le = LocalEvent::Get();

    int frame = 10;
    while ( le.HandleEvents() && frame > 0 ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_FRAME_DELAY ) ) {
            const uint32_t alpha = static_cast<uint32_t>( frame ) * 25;
            for ( std::vector<Unit *>::const_iterator it = mirrorImages.begin(); it != mirrorImages.end(); ++it ) {
                if ( *it )
                    ( *it )->SetCustomAlpha( alpha );
            }

            Redraw();

            --frame;
        }
    }
    status.SetMessage( _( "The mirror image is destroyed!" ), true );
}

void Battle::Interface::RedrawTargetsWithFrameAnimation( s32 dst, const TargetsInfo & targets, int icn, int m82 )
{
    LocalEvent & le = LocalEvent::Get();

    u32 frame = 0;
    const Rect & center = Board::GetCell( dst )->GetPos();

    Cursor::Get().SetThemes( Cursor::WAR_NONE );

    _currentUnit = NULL;
    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender && ( *it ).damage )
            ( *it ).defender->SwitchAnimation( Monster_Info::WNCE );

    if ( M82::UNKNOWN != m82 )
        AGG::PlaySound( m82 );

    while ( le.HandleEvents() && frame < AGG::GetICNCount( icn ) ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            RedrawPartialStart();

            const Sprite & sprite = AGG::GetICN( icn, frame );
            sprite.Blit( center.x + center.w / 2 + sprite.x(), center.y + center.h / 2 + sprite.y(), _mainSurface );
            RedrawPartialFinish();

            for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
                if ( ( *it ).defender && ( *it ).damage )
                    ( *it ).defender->IncreaseAnimFrame( false );
            ++frame;
        }
    }

    for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
        if ( ( *it ).defender ) {
            ( *it ).defender->SwitchAnimation( Monster_Info::STATIC );
            _currentUnit = NULL;
        }
}

Point CalculateSpellPosition( const Battle::Unit & target, int spellICN, const Sprite & spellSprite )
{
    const Rect & pos = target.GetRectPosition();
    const Sprite & unitSprite = AGG::GetICN( target.GetMonsterSprite().icn_file, target.GetFrame(), target.isReflect() );

    // Bottom-left corner (default) position with spell offset applied
    Point result( pos.x + spellSprite.x(), pos.y + pos.h - 10 + spellSprite.y() );

    switch ( spellICN ) {
    case ICN::SHIELD:
        // in front of the unit
        result.x += target.isReflect() ? -pos.w / ( target.isWide() ? 2 : 1 ) : pos.w;
        result.y += unitSprite.y() / 2;
        break;
    case ICN::BLIND: {
        // unit's eyes
        const Point & offset = target.animation.getBlindOffset();

        // calculate OG Heroes2 unit position to apply offset to
        const int rearCenterX = ( target.isWide() && target.isReflect() ) ? pos.w * 3 / 4 : CELLW / 2;

        // Overwrite result with custom blind value
        result.x += rearCenterX + ( target.isReflect() ? -offset.x : offset.x );
        result.y += offset.y;
    } break;
    case ICN::STONSKIN:
    case ICN::STELSKIN:
        // bottom center point
        result.x += pos.w / 2;
        break;
    default:
        // center point of the unit
        result.x += pos.w / 2;
        result.y += unitSprite.y() / 2;
        break;
    }

    if ( result.y < 0 ) {
        const int maximumY = AGG::GetAbsoluteICNHeight( spellICN );
        result.y = maximumY + spellSprite.y();
    }

    return result;
}

void Battle::Interface::RedrawTargetsWithFrameAnimation( const TargetsInfo & targets, int icn, int m82, bool wnce )
{
    LocalEvent & le = LocalEvent::Get();

    u32 frame = 0;

    Cursor::Get().SetThemes( Cursor::WAR_NONE );

    _currentUnit = NULL;

    if ( wnce )
        for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
            if ( ( *it ).defender && ( *it ).damage )
                ( *it ).defender->SwitchAnimation( Monster_Info::WNCE );

    if ( M82::UNKNOWN != m82 )
        AGG::PlaySound( m82 );

    while ( le.HandleEvents() && frame < AGG::GetICNCount( icn ) ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            RedrawPartialStart();

            for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
                if ( ( *it ).defender ) {
                    const bool reflect = ( icn == ICN::SHIELD && it->defender->isReflect() );
                    const Sprite & spellSprite = AGG::GetICN( icn, frame, reflect );
                    spellSprite.Blit( CalculateSpellPosition( *it->defender, icn, spellSprite ), _mainSurface );
                }
            RedrawPartialFinish();

            if ( wnce )
                for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
                    if ( ( *it ).defender && ( *it ).damage )
                        ( *it ).defender->IncreaseAnimFrame( false );
            ++frame;
        }
    }

    if ( wnce )
        for ( TargetsInfo::const_iterator it = targets.begin(); it != targets.end(); ++it )
            if ( ( *it ).defender ) {
                ( *it ).defender->SwitchAnimation( Monster_Info::STATIC );
                _currentUnit = NULL;
            }
}

void Battle::Interface::RedrawTroopWithFrameAnimation( Unit & b, int icn, int m82, CreatueSpellAnimation animation )
{
    LocalEvent & le = LocalEvent::Get();

    u32 frame = 0;
    const bool reflect = ( icn == ICN::SHIELD && b.isReflect() );

    Cursor::Get().SetThemes( Cursor::WAR_NONE );

    if ( animation == WINCE ) {
        _currentUnit = NULL;
        b.SwitchAnimation( Monster_Info::WNCE );
    }
    else if ( animation == RESURRECT ) {
        _currentUnit = NULL;
        b.SwitchAnimation( Monster_Info::KILL, true );
    }

    if ( M82::UNKNOWN != m82 )
        AGG::PlaySound( m82 );

    while ( le.HandleEvents() && frame < AGG::GetICNCount( icn ) ) {
        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_SPELL_DELAY ) ) {
            RedrawPartialStart();

            const Sprite & spellSprite = AGG::GetICN( icn, frame, reflect );

            spellSprite.Blit( CalculateSpellPosition( b, icn, spellSprite ), _mainSurface );
            RedrawPartialFinish();

            if ( animation != NONE ) {
                if ( animation == RESURRECT ) {
                    if ( b.isFinishAnimFrame() )
                        b.SwitchAnimation( Monster_Info::STATIC );
                }
                b.IncreaseAnimFrame( false );
            }
            ++frame;
        }
    }

    if ( animation != NONE ) {
        if ( animation == RESURRECT ) {
            b.SetPosition( b.GetPosition() );
        }
        b.SwitchAnimation( Monster_Info::STATIC );
        _currentUnit = NULL;
    }
}

void Battle::Interface::RedrawBridgeAnimation( bool down )
{
    LocalEvent & le = LocalEvent::Get();

    u32 frame = down ? 23 : 21;

    if ( down )
        AGG::PlaySound( M82::DRAWBRG );

    while ( le.HandleEvents() ) {
        if ( down ) {
            if ( frame < 21 )
                break;
        }
        else {
            if ( frame > 23 )
                break;
        }

        CheckGlobalEvents( le );

        if ( Battle::AnimateInfrequentDelay( Game::BATTLE_BRIDGE_DELAY ) ) {
            RedrawPartialStart();
            const Sprite & sprite = AGG::GetICN( ICN::Get4Castle( Arena::GetCastle()->GetRace() ), frame );
            sprite.Blit( sprite.x(), sprite.y(), _mainSurface );
            RedrawPartialFinish();

            if ( down )
                --frame;
            else
                ++frame;
        }
    }

    if ( !down )
        AGG::PlaySound( M82::DRAWBRG );
}

bool Battle::Interface::IdleTroopsAnimation( void )
{
    if ( Battle::AnimateInfrequentDelay( Game::BATTLE_IDLE_DELAY ) ) {
        const bool redrawNeeded = arena.GetForce1().animateIdleUnits();
        return arena.GetForce2().animateIdleUnits() || redrawNeeded;
    }

    return false;
}

void Battle::Interface::ResetIdleTroopAnimation( void )
{
    arena.GetForce1().resetIdleAnimation();
    arena.GetForce2().resetIdleAnimation();
}

void Battle::Interface::CheckGlobalEvents( LocalEvent & le )
{
    if ( Game::AnimateInfrequentDelay( Game::BATTLE_COLOR_CYCLE_DELAY ) )
        CycleColors();

    if ( Game::AnimateInfrequentDelay( Game::BATTLE_SELECTED_UNIT_DELAY ) )
        UpdateContourColor();

    // animate heroes
    if ( Battle::AnimateInfrequentDelay( Game::BATTLE_OPPONENTS_DELAY ) ) {
        if ( opponent1 )
            opponent1->Update();

        if ( opponent2 )
            opponent2->Update();

        humanturn_redraw = true;
    }

    // flags animation
    if ( Battle::AnimateInfrequentDelay( Game::BATTLE_FLAGS_DELAY ) ) {
        ++animation_flags_frame;
        humanturn_redraw = true;
    }

    // break auto battle
    if ( arena.CanBreakAutoBattle()
         && ( le.MouseClickLeft( btn_auto )
              || ( le.KeyPress()
                   && ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_AUTOSWITCH )
                        || ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_RETREAT )
                             && Dialog::YES == Dialog::Message( "", _( "Break auto battle?" ), Font::BIG, Dialog::YES | Dialog::NO ) ) ) ) ) ) {
        arena.BreakAutoBattle();
    }
}

void Battle::Interface::ProcessingHeroDialogResult( int res, Actions & a )
{
    switch ( res ) {
    // cast
    case 1: {
        const HeroBase * hero = _currentUnit ? _currentUnit->GetCommander() : NULL;
        if ( hero ) {
            if ( hero->HaveSpellBook() ) {
                std::string msg;
                if ( arena.isDisableCastSpell( Spell::NONE, &msg ) )
                    Dialog::Message( "", msg, Font::BIG, Dialog::OK );
                else {
                    const Spell spell = hero->OpenSpellBook( SpellBook::CMBT, true );
                    if ( spell.isValid() ) {
                        std::string error;

                        if ( arena.isDisableCastSpell( spell, &msg ) )
                            Dialog::Message( "", msg, Font::BIG, Dialog::OK );
                        else if ( hero->CanCastSpell( spell, &error ) ) {
                            if ( spell.isApplyWithoutFocusObject() ) {
                                a.push_back( Command( MSG_BATTLE_CAST, spell(), -1 ) );
                                humanturn_redraw = true;
                                humanturn_exit = true;
                            }
                            else
                                humanturn_spell = spell;
                        }
                        else if ( error.size() )
                            Dialog::Message( "Error", error, Font::BIG, Dialog::OK );
                    }
                }
            }
            else
                Dialog::Message( "", _( "No spells to cast." ), Font::BIG, Dialog::OK );
        }
    } break;

    // retreat
    case 2:
        if ( _currentUnit->GetCommander() && arena.CanRetreatOpponent( _currentUnit->GetColor() ) ) {
            if ( Dialog::YES == Dialog::Message( "", _( "Are you sure you want to retreat?" ), Font::BIG, Dialog::YES | Dialog::NO ) ) {
                a.push_back( Command( MSG_BATTLE_RETREAT ) );
                a.push_back( Command( MSG_BATTLE_END_TURN, _currentUnit->GetUID() ) );
                humanturn_exit = true;
            }
        }
        else
            Dialog::Message( "", _( "Retreat disabled" ), Font::BIG, Dialog::OK );
        break;

    // surrender
    case 3:
        if ( arena.CanSurrenderOpponent( _currentUnit->GetColor() ) ) {
            const HeroBase * enemy = arena.GetCommander( arena.GetCurrentColor(), true );

            if ( enemy ) {
                const s32 cost = arena.GetCurrentForce().GetSurrenderCost();
                const Kingdom & kingdom = world.GetKingdom( arena.GetCurrentColor() );

                if ( !kingdom.AllowPayment( Funds( Resource::GOLD, cost ) ) )
                    Dialog::Message( "", _( "You don't have enough gold!" ), Font::BIG, Dialog::OK );
                else if ( DialogBattleSurrender( *enemy, cost, kingdom ) ) {
                    a.push_back( Command( MSG_BATTLE_SURRENDER ) );
                    a.push_back( Command( MSG_BATTLE_END_TURN, _currentUnit->GetUID() ) );
                    humanturn_exit = true;
                }
            }
        }
        else
            Dialog::Message( "", _( "Surrender disabled" ), Font::BIG, Dialog::OK );
        break;

    default:
        break;
    }
}

Battle::PopupDamageInfo::PopupDamageInfo()
    : Dialog::FrameBorder( 5 )
    , cell( NULL )
    , attacker( NULL )
    , defender( NULL )
    , redraw( false )
{}

void Battle::PopupDamageInfo::SetInfo( const Cell * c, const Unit * a, const Unit * b )
{
    if ( Settings::Get().ExtBattleShowDamage() && Battle::AnimateInfrequentDelay( Game::BATTLE_POPUP_DELAY )
         && ( !cell || ( c && cell != c ) || !attacker || ( a && attacker != a ) || !defender || ( b && defender != b ) ) ) {
        redraw = true;
        cell = c;
        attacker = a;
        defender = b;

        const Rect & rt = cell->GetPos();
        SetPosition( rt.x + rt.w, rt.y, 20, 20 );
    }
}

void Battle::PopupDamageInfo::Reset( void )
{
    if ( redraw ) {
        Cursor::Get().Hide();
        background.Restore();
        redraw = false;
        cell = NULL;
        attacker = NULL;
        defender = NULL;
    }
    Game::AnimateResetDelay( Game::BATTLE_POPUP_DELAY );
}

void Battle::PopupDamageInfo::Redraw( int maxw, int maxh )
{
    if ( redraw ) {
        Cursor::Get().Hide();

        Text text1, text2;
        std::string str;

        u32 tmp1 = attacker->CalculateMinDamage( *defender );
        u32 tmp2 = attacker->CalculateMaxDamage( *defender );

        str = tmp1 == tmp2 ? _( "Damage: %{max}" ) : _( "Damage: %{min} - %{max}" );

        StringReplace( str, "%{min}", tmp1 );
        StringReplace( str, "%{max}", tmp2 );

        text1.Set( str, Font::SMALL );

        tmp1 = defender->HowManyWillKilled( tmp1 );
        tmp2 = defender->HowManyWillKilled( tmp2 );

        if ( tmp1 > defender->GetCount() )
            tmp1 = defender->GetCount();
        if ( tmp2 > defender->GetCount() )
            tmp2 = defender->GetCount();

        str = tmp1 == tmp2 ? _( "Perish: %{max}" ) : _( "Perish: %{min} - %{max}" );

        StringReplace( str, "%{min}", tmp1 );
        StringReplace( str, "%{max}", tmp2 );

        text2.Set( str, Font::SMALL );

        int tw = 5 + ( text1.w() > text2.w() ? text1.w() : text2.w() );
        int th = ( text1.h() + text2.h() );

        const Rect & borderArea = GetArea();
        const Rect & borderRect = GetRect();
        const Rect & pos = cell->GetPos();

        int tx = borderRect.x;
        int ty = borderRect.y;

        if ( borderRect.x + borderRect.w > maxw ) {
            tx = maxw - borderRect.w - 5;
            ty = pos.y - pos.h;
        }

        if ( borderRect.x != tx || borderRect.y != ty || borderArea.w != tw || borderArea.h != th )
            SetPosition( tx, ty, tw, th );

        const Sprite & sf = AGG::GetICN( ICN::CELLWIN, 1 );
        Dialog::FrameBorder::RenderOther( sf, GetRect() );

        text1.Blit( borderArea.x, borderArea.y );
        text2.Blit( borderArea.x, borderArea.y + borderArea.h / 2 );
    }
}

bool Battle::Interface::NetworkTurn( Result & result )
{
    return false;
}
