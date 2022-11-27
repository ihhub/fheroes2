/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "battle_animation.h"
#include "battle_board.h"
#include "cursor.h"
#include "dialog.h"
#include "image.h"
#include "math_base.h"
#include "spell.h"
#include "text.h"
#include "ui_button.h"

class Castle;
class HeroBase;
class Kingdom;
class LocalEvent;
class Settings;

namespace fheroes2
{
    class StandardWindow;
}

namespace Battle
{
    class Actions;
    class Arena;
    class Cell;
    class Position;
    class StatusListBox;
    class Tower;
    class Unit;
    class Units;

    struct TargetInfo;
    struct TargetsInfo;

    void DialogBattleSettings();
    bool DialogBattleSurrender( const HeroBase & hero, uint32_t cost, Kingdom & kingdom );

    enum HeroAnimation : uint32_t
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
        OpponentSprite( const fheroes2::Rect &, const HeroBase *, bool );
        OpponentSprite( const OpponentSprite & ) = delete;

        OpponentSprite & operator=( const OpponentSprite & ) = delete;

        const fheroes2::Rect & GetArea() const
        {
            return pos;
        }

        fheroes2::Point GetCastPosition() const;
        void Redraw( fheroes2::Image & dst ) const;
        void Update();
        void SetAnimation( int rule );
        void IncreaseAnimFrame( bool loop = false );

        bool isFinishFrame() const
        {
            return _currentAnim.isLastFrame();
        }

        const HeroBase * GetHero() const
        {
            return base;
        }

        fheroes2::Point Offset() const
        {
            return _offset;
        }

        enum
        {
            RIGHT_HERO_X_OFFSET = 29,
            LEFT_HERO_X_OFFSET = 30,
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

        int _heroIcnId;
        bool reflect;
        fheroes2::Rect pos;
        fheroes2::Point _offset;
    };

    class Status : public fheroes2::Rect
    {
    public:
        Status();
        Status( const Status & ) = delete;

        Status & operator=( const Status & ) = delete;

        void SetPosition( int32_t, int32_t );

        void SetLogs( StatusListBox * logs )
        {
            listlog = logs;
        }

        void SetMessage( const std::string & message, bool top = false );
        void Redraw( fheroes2::Image & output ) const;

        const std::string & GetMessage() const
        {
            return message;
        }

        void clear();

    private:
        Text bar1;
        Text bar2;
        const fheroes2::Sprite & back1;
        const fheroes2::Sprite & back2;
        std::string message;
        StatusListBox * listlog;
    };

    class ArmiesOrder : public fheroes2::Rect
    {
    public:
        ArmiesOrder();
        ArmiesOrder( const ArmiesOrder & ) = delete;

        ArmiesOrder & operator=( const ArmiesOrder & ) = delete;

        void Set( const fheroes2::Rect & rt, const std::shared_ptr<const Units> & units, const int army2Color );
        void Redraw( const Unit * current, const uint8_t currentUnitColor, fheroes2::Image & output );
        void QueueEventProcessing( std::string & msg, const fheroes2::Point & offset );

    private:
        enum ArmyColor : uint8_t
        {
            ARMY_COLOR_BLACK = 0x00,
            ARMY_COLOR_BLUE = 0x47,
            ARMY_COLOR_GREEN = 0x67,
            ARMY_COLOR_RED = 0xbd,
            ARMY_COLOR_YELLOW = 0x70,
            ARMY_COLOR_ORANGE = 0xcd,
            ARMY_COLOR_PURPLE = 0x87,
            ARMY_COLOR_GRAY = 0x10
        };

        using UnitPos = std::pair<const Unit *, fheroes2::Rect>;

        void RedrawUnit( const fheroes2::Rect & pos, const Battle::Unit & unit, const bool revert, const bool isCurrentUnit, const uint8_t currentUnitColor,
                         fheroes2::Image & output ) const;

        std::weak_ptr<const Units> _orders;
        int _army2Color;
        fheroes2::Rect _area;
        std::vector<UnitPos> _rects;
    };

    class PopupDamageInfo : public Dialog::FrameBorder
    {
    public:
        PopupDamageInfo();
        PopupDamageInfo( const PopupDamageInfo & ) = delete;

        PopupDamageInfo & operator=( const PopupDamageInfo & ) = delete;

        void setBattleUIRect( const fheroes2::Rect & battleUIRect );
        void SetInfo( const Cell * cell, const Unit * attacker, const Unit * defender );
        void Reset();
        void Redraw();

    private:
        fheroes2::Rect _battleUIRect;
        const Cell * _cell;
        const Unit * _attacker;
        const Unit * _defender;
        bool _redraw;
    };

    class Interface
    {
    public:
        Interface( Arena & battleArena, const int32_t tileIndex );
        Interface( const Interface & ) = delete;

        ~Interface();

        Interface & operator=( const Interface & ) = delete;

        void fullRedraw(); // only at the start of the battle
        void Redraw();
        void RedrawPartialStart();
        void RedrawPartialFinish();

        void getPendingActions( Actions & actions );
        void HumanTurn( const Unit &, Actions & );

        const fheroes2::Rect & GetArea() const
        {
            return _surfaceInnerArea;
        }

        fheroes2::Point GetMouseCursor() const;

        void SetStatus( const std::string &, bool = false );
        void SetOrderOfUnits( const std::shared_ptr<const Units> & units );
        void FadeArena( bool clearMessageLog );

        void RedrawActionNewTurn() const;
        void RedrawActionAttackPart1( Unit &, Unit &, const TargetsInfo & );
        void RedrawActionAttackPart2( Unit & attacker, const TargetsInfo & targets );
        void RedrawActionSpellCastStatus( const Spell & spell, int32_t dst, const std::string & name, const TargetsInfo & targets );
        void RedrawActionSpellCastPart1( const Spell & spell, int32_t dst, const HeroBase * caster, const TargetsInfo & targets );
        void RedrawActionSpellCastPart2( const Spell & spell, const TargetsInfo & targets );
        void RedrawActionResistSpell( const Unit & target, bool playSound );
        void RedrawActionMonsterSpellCastStatus( const Spell & spell, const Unit & attacker, const TargetInfo & target );
        void RedrawActionMove( Unit &, const Indexes & );
        void RedrawActionFly( Unit &, const Position & );
        void RedrawActionMorale( Unit &, bool );
        void RedrawActionLuck( const Unit & );
        void RedrawActionTowerPart1( const Tower &, const Unit & );
        void RedrawActionTowerPart2( const Tower &, const TargetInfo & );
        void RedrawActionCatapult( int target, bool hit );
        void RedrawActionTeleportSpell( Unit &, int32_t );
        void RedrawActionEarthQuakeSpell( const std::vector<int> & );
        void RedrawActionSummonElementalSpell( Unit & target );
        void RedrawActionMirrorImageSpell( const Unit &, const Position & );
        void RedrawActionSkipStatus( const Unit & );
        void RedrawActionRemoveMirrorImage( const std::vector<Unit *> & mirrorImages );
        void RedrawBridgeAnimation( const bool bridgeDownAnimation );
        void RedrawMissileAnimation( const fheroes2::Point & startPos, const fheroes2::Point & endPos, double angle, uint32_t monsterID );

    private:
        enum CreatueSpellAnimation
        {
            NONE,
            WINCE,
            RESURRECT
        };

        void HumanBattleTurn( const Unit &, Actions &, std::string & );
        void HumanCastSpellTurn( const Unit &, Actions &, std::string & );

        void RedrawCover();
        void RedrawCoverStatic( const Settings & conf, const Board & board );
        void RedrawLowObjects( int32_t );
        void RedrawHighObjects( int32_t );
        void RedrawCastle( const Castle &, int32_t );
        void RedrawCastleMainTower( const Castle & );
        void RedrawKilled();
        void RedrawInterface();
        void RedrawOpponents();
        void RedrawOpponentsFlags();
        void RedrawArmies();
        void RedrawTroopSprite( const Unit & unit );

        fheroes2::Point drawTroopSprite( const Unit & unit, const fheroes2::Sprite & troopSprite );

        void RedrawTroopCount( const Unit & unit );

        void RedrawActionWincesKills( const TargetsInfo & targets, Unit * attacker = nullptr );
        void RedrawActionArrowSpell( const Unit & );
        void RedrawActionColdRaySpell( Unit & );
        void RedrawActionDisruptingRaySpell( const Unit & );
        void RedrawActionBloodLustSpell( const Unit & );
        void RedrawActionStoneSpell( const Unit & target );
        void RedrawActionColdRingSpell( int32_t, const TargetsInfo & );
        void RedrawActionElementalStormSpell( const TargetsInfo & );
        void RedrawActionArmageddonSpell();
        void RedrawActionHolyShoutSpell( const TargetsInfo & targets, int strength );
        void RedrawActionResurrectSpell( Unit &, const Spell & );
        void RedrawActionDeathWaveSpell( const TargetsInfo & targets, int strength );
        void RedrawActionLightningBoltSpell( const Unit & );
        void RedrawActionChainLightningSpell( const TargetsInfo & );
        void RedrawLightningOnTargets( const std::vector<fheroes2::Point> & points, const fheroes2::Rect & drawRoi ); // helper function
        void RedrawRaySpell( const Unit & target, int spellICN, int spellSound, int32_t size );

        void AnimateOpponents( OpponentSprite * target );
        void AnimateUnitWithDelay( Unit & unit, uint32_t delay );
        void RedrawTroopDefaultDelay( Unit & unit );
        void RedrawTroopWithFrameAnimation( Unit & b, int icn, int m82, CreatueSpellAnimation animation );
        void RedrawTargetsWithFrameAnimation( int32_t dst, const TargetsInfo & targets, int icn, int m82, int repeatCount = 0 );
        void RedrawTargetsWithFrameAnimation( const TargetsInfo &, int, int, bool );

        bool IdleTroopsAnimation() const;
        void ResetIdleTroopAnimation() const;
        void UpdateContourColor();
        void CheckGlobalEvents( LocalEvent & );

        void ProcessingHeroDialogResult( int, Actions & );

        void EventAutoSwitch( const Unit &, Actions & );
        void EventAutoFinish( Actions & a );
        void EventShowOptions();
        void ButtonAutoAction( const Unit &, Actions & );
        void ButtonSettingsAction();
        void ButtonSkipAction( Actions & );
        void MouseLeftClickBoardAction( int themes, const Cell & cell, Actions & a );
        void MousePressRightBoardAction( const Cell & cell ) const;

        int GetBattleCursor( std::string & ) const;
        int GetBattleSpellCursor( std::string & ) const;

        Arena & arena;
        Dialog::FrameBorder border;

        fheroes2::Rect _interfacePosition;
        fheroes2::Rect _surfaceInnerArea;
        fheroes2::Image _mainSurface;
        fheroes2::Image sf_hexagon;
        fheroes2::Image sf_shadow;
        fheroes2::Image sf_shadow_grid;
        fheroes2::Image sf_cursor;

        int icn_cbkg;
        int icn_frng;

        fheroes2::Button btn_auto;
        fheroes2::Button btn_settings;
        fheroes2::Button btn_skip;
        Status status;

        OpponentSprite * opponent1;
        OpponentSprite * opponent2;

        Spell humanturn_spell;
        bool humanturn_exit;
        bool humanturn_redraw;
        uint32_t animation_flags_frame;
        int catapult_frame;

        int _interruptAutoBattleForColor;

        uint8_t _contourColor;
        bool _brightLandType; // used to determine current monster contour cycling colors
        uint32_t _contourCycle;

        const Unit * _currentUnit;
        const Unit * _movingUnit;
        const Unit * _flyingUnit;
        const fheroes2::Sprite * b_current_sprite;
        fheroes2::Point _movingPos;
        fheroes2::Point _flyingPos;

        int32_t index_pos;
        int32_t teleport_src;
        fheroes2::Rect main_tower;

        std::unique_ptr<StatusListBox> listlog;

        PopupDamageInfo popup;
        ArmiesOrder armies_order;

        CursorRestorer _cursorRestorer;
        std::unique_ptr<fheroes2::StandardWindow> _background;

        struct BridgeMovementAnimation
        {
            enum AnimationStatusId : uint32_t
            {
                DOWN_POSITION = 21,
                UP_POSITION = 23,
                DESTROYED = 24
            };

            bool animationIsRequired;

            uint32_t currentFrameId;
        };

        BridgeMovementAnimation _bridgeAnimation;
    };
}

#endif
