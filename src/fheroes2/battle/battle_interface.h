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

#ifndef H2BATTLE_INTERFACE_H
#define H2BATTLE_INTERFACE_H

#include <string>

#include "battle_board.h"
#include "dialog.h"
#include "game_delays.h"
#include "gamedefs.h"
#include "icn.h"
#include "spell.h"
#include "statusbar.h"
#include "text.h"
#include "ui_button.h"

class Settings;

namespace Battle
{
    class Arena;
    class Unit;
    class Units;
    class Tower;
    class StatusListBox;
    class Cell;
    class Actions;
    struct TargetInfo;
    struct TargetsInfo;
    struct Result;

    void DialogBattleSettings( void );
    bool DialogBattleSurrender( const HeroBase & hero, u32 cost, const Kingdom & kingdom );

    enum HeroAnimation
    {
        OP_JOY,
        OP_CAST_MASS,
        OP_CAST_MASS_RETURN,
        OP_CAST_UP,
        OP_CAST_UP_RETURN,
        OP_CAST_DOWN,
        OP_CAST_DOWN_RETURN,
        OP_IDLE,
        OP_IDLE2,
        OP_STATIC,
        OP_SORROW
    };

    enum BattleHeroType
    {
        KNIGHT,
        BARBARIAN,
        SORCERESS,
        WARLOCK,
        WIZARD,
        NECROMANCER,
        CAPTAIN
    };

    class OpponentSprite
    {
    public:
        OpponentSprite( const Rect &, const HeroBase *, bool );

        const Rect & GetArea( void ) const;
        Point GetCastPosition() const;
        void Redraw( fheroes2::Image & dst ) const;
        void Update();
        void SetAnimation( int rule );
        void IncreaseAnimFrame( bool loop = false );
        bool isFinishFrame( void ) const;
        bool isStartFrame( void ) const;
        int GetColor( void ) const;
        const HeroBase * GetHero( void ) const;
        Point Offset() const;

        enum
        {
            HERO_X_OFFSET = 30,
            LEFT_HERO_Y_OFFSET = 183,
            RIGHT_HERO_Y_OFFSET = 148,
            CAPTAIN_X_OFFSET = 6,
            CAPTAIN_Y_OFFSET = -13
        };

    private:
        const HeroBase * base;
        AnimationSequence _currentAnim;
        int _animationType;
        RandomizedDelay _idleTimer;

        int icn;
        bool reflect;
        Rect pos;
        Point _offset;
    };

    class Status : public Rect
    {
    public:
        Status();

        void SetPosition( s32, s32 );
        void SetLogs( StatusListBox * logs )
        {
            listlog = logs;
        };
        void SetMessage( const std::string &, bool = false );
        void Redraw( void );
        const std::string & GetMessage( void ) const;

        void clear();

    private:
        Text bar1;
        Text bar2;
        fheroes2::Sprite back1;
        fheroes2::Sprite back2;
        std::string message;
        StatusListBox * listlog;
    };

    class ArmiesOrder : public Rect
    {
    public:
        ArmiesOrder();

        void Set( const Rect &, const Units *, int );
        void Redraw( const Unit * current, fheroes2::Image & output );
        void QueueEventProcessing( std::string & msg, const Point & offset );

    private:
        typedef std::pair<const Unit *, Rect> UnitPos;

        void RedrawUnit( const Rect & pos, const Battle::Unit & unit, bool revert, bool current, fheroes2::Image & output ) const;

        const Units * orders;
        int army_color2;
        Rect area;
        fheroes2::Image sf_color[3];
        std::vector<UnitPos> rects;
    };

    class PopupDamageInfo : public Dialog::FrameBorder
    {
    public:
        PopupDamageInfo();

        void SetInfo( const Cell * c, const Unit * a, const Unit * b, const Point & offset );
        void Reset();
        void Redraw( int, int );

    private:
        const Cell * cell;
        const Unit * attacker;
        const Unit * defender;
        bool redraw;
    };

    class Interface
    {
    public:
        Interface( Arena &, s32 );
        ~Interface();

        void Redraw();
        void RedrawPartialStart();
        void RedrawPartialFinish();
        void HumanTurn( const Unit &, Actions & );
        bool NetworkTurn( Result & );

        const Rect & GetArea( void ) const;
        Point GetMouseCursor() const;

        void SetStatus( const std::string &, bool = false );
        void SetArmiesOrder( const Units * );
        void FadeArena( bool clearMessageLog );

        void RedrawActionAttackPart1( Unit &, Unit &, const TargetsInfo & );
        void RedrawActionAttackPart2( Unit &, TargetsInfo & );
        void RedrawActionSpellCastPart1( const Spell &, s32, const HeroBase *, const std::string &, const TargetsInfo & );
        void RedrawActionSpellCastPart2( const Spell &, TargetsInfo & );
        void RedrawActionResistSpell( const Unit & );
        void RedrawActionMonsterSpellCastStatus( const Unit &, const TargetInfo & );
        void RedrawActionMove( Unit &, const Indexes & );
        void RedrawActionFly( Unit &, const Position & );
        void RedrawActionMorale( Unit &, bool );
        void RedrawActionLuck( Unit & );
        void RedrawActionTowerPart1( Tower &, Unit & );
        void RedrawActionTowerPart2( TargetInfo & );
        void RedrawActionCatapult( int );
        void RedrawActionTeleportSpell( Unit &, s32 );
        void RedrawActionEarthQuakeSpell( const std::vector<int> & );
        void RedrawActionSummonElementalSpell( Unit & target );
        void RedrawActionMirrorImageSpell( const Unit &, const Position & );
        void RedrawActionSkipStatus( const Unit & );
        void RedrawActionRemoveMirrorImage( const std::vector<Unit *> & mirrorImages );
        void RedrawBridgeAnimation( bool down );
        void RedrawMissileAnimation( const Point & startPos, const Point & endPos, double angle, uint32_t monsterID );

    private:
        enum CreatueSpellAnimation
        {
            NONE,
            WINCE,
            RESURRECT
        };

        void HumanBattleTurn( const Unit &, Actions &, std::string & );
        void HumanCastSpellTurn( const Unit &, Actions &, std::string & );

        void RedrawBorder( void );
        void RedrawCover( void );
        void RedrawCoverStatic();
        void RedrawCoverBoard( const Settings &, const Board & );
        void RedrawLowObjects( s32 );
        void RedrawHighObjects( s32 );
        void RedrawCastle1( const Castle & );
        void RedrawCastle2( const Castle &, s32 );
        void RedrawCastleMainTower( const Castle & );
        void RedrawKilled( void );
        void RedrawInterface( void );
        void RedrawOpponents( void );
        void RedrawOpponentsFlags( void );
        void RedrawArmies( void );
        void RedrawTroopSprite( const Unit & );
        void RedrawTroopCount( const Unit & unit );

        void RedrawActionWincesKills( TargetsInfo & targets, Unit * attacker = NULL );
        void RedrawActionArrowSpell( const Unit & );
        void RedrawActionColdRaySpell( Unit & );
        void RedrawActionDisruptingRaySpell( Unit & );
        void RedrawActionBloodLustSpell( Unit & );
        void RedrawActionStoneSpell( Unit & target );
        void RedrawActionColdRingSpell( s32, const TargetsInfo & );
        void RedrawActionElementalStormSpell( const TargetsInfo & );
        void RedrawActionArmageddonSpell();
        void RedrawActionHolyShoutSpell( const TargetsInfo & targets, int strength );
        void RedrawActionResurrectSpell( Unit &, const Spell & );
        void RedrawActionDeathWaveSpell( const TargetsInfo & targets, int strength );
        void RedrawActionLightningBoltSpell( Unit & );
        void RedrawActionChainLightningSpell( const TargetsInfo & );
        void RedrawLightningOnTargets( const std::vector<Point> & points, const Rect & drawRoi ); // helper function
        void RedrawRaySpell( const Unit & target, int spellICN, int spellSound, uint32_t size );

        void AnimateOpponents( OpponentSprite * target );
        void AnimateUnitWithDelay( Unit & unit, uint32_t delay );
        void RedrawTroopDefaultDelay( Unit & unit );
        void RedrawTroopWithFrameAnimation( Unit & b, int icn, int m82, CreatueSpellAnimation animation );
        void RedrawTargetsWithFrameAnimation( int32_t dst, const TargetsInfo & targets, int icn, int m82, int repeatCount = 0 );
        void RedrawTargetsWithFrameAnimation( const TargetsInfo &, int, int, bool );

        bool IdleTroopsAnimation( void );
        void ResetIdleTroopAnimation( void );
        void UpdateContourColor();
        void CheckGlobalEvents( LocalEvent & );

        void ProcessingHeroDialogResult( int, Actions & );

        void EventAutoSwitch( const Unit &, Actions & );
        void EventShowOptions( void );
        void ButtonAutoAction( const Unit &, Actions & );
        void ButtonSettingsAction( void );
        void ButtonSkipAction( Actions & );
        void ButtonWaitAction( Actions & );
        void MouseLeftClickBoardAction( u32, const Cell &, Actions & );
        void MousePressRightBoardAction( u32, const Cell &, Actions & );

        int GetBattleCursor( std::string & ) const;
        int GetBattleSpellCursor( std::string & ) const;
        int GetAllowSwordDirection( u32 );

        Arena & arena;
        Dialog::FrameBorder border;

        Rect _interfacePosition;
        Rect _surfaceInnerArea;
        fheroes2::Image _mainSurface;
        fheroes2::Image sf_hexagon;
        fheroes2::Image sf_shadow;
        fheroes2::Image sf_cursor;

        int icn_cbkg;
        int icn_frng;

        fheroes2::Button btn_auto;
        fheroes2::Button btn_settings;
        fheroes2::Button btn_skip;
        fheroes2::Button btn_wait;
        Status status;

        OpponentSprite * opponent1;
        OpponentSprite * opponent2;

        Spell humanturn_spell;
        bool humanturn_exit;
        bool humanturn_redraw;
        u32 animation_flags_frame;
        int catapult_frame;

        uint8_t _contourColor;
        bool _brightLandType; // used to determin current monster contour cycling colors
        uint32_t _contourCycle;

        const Unit * _currentUnit;
        const Unit * _movingUnit;
        const Unit * _flyingUnit;
        const fheroes2::Sprite * b_current_sprite;
        Point _movingPos;
        Point _flyingPos;

        s32 index_pos;
        s32 teleport_src;
        Rect pocket_book;
        Rect main_tower;

        StatusListBox * listlog;
        u32 turn;

        PopupDamageInfo popup;
        ArmiesOrder armies_order;
    };
}

#endif
