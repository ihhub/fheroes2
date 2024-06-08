/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "battle.h"
#include "battle_animation.h"
#include "battle_board.h"
#include "battle_troop.h"
#include "cursor.h"
#include "dialog.h"
#include "icn.h"
#include "image.h"
#include "math_base.h"
#include "spell.h"
#include "ui_button.h"
#include "ui_text.h"

class Castle;
class HeroBase;
class Kingdom;
class LocalEvent;

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
    class Units;

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

    // Sprite data to render over the unit (spell effect animation)
    struct UnitSpellEffectInfo
    {
        UnitSpellEffectInfo( const uint32_t setUnitId, const int32_t setIcnId, const bool setReflectedImage )
            : unitId( setUnitId )
            , icnId( setIcnId )
            , isReflectedImage( setReflectedImage )
        {}

        uint32_t unitId{ 0 };
        int32_t icnId{ ICN::UNKNOWN };
        uint32_t icnIndex{ 0 };
        fheroes2::Point position;
        bool isReflectedImage{ false };
    };

    class OpponentSprite
    {
    public:
        OpponentSprite( const fheroes2::Rect & area, const HeroBase * hero, const bool isReflect );
        OpponentSprite( const OpponentSprite & ) = delete;

        OpponentSprite & operator=( const OpponentSprite & ) = delete;

        const fheroes2::Rect & GetArea() const
        {
            return pos;
        }

        fheroes2::Point GetCastPosition() const;
        void Redraw( fheroes2::Image & dst ) const;

        // Return true is animation state was changed.
        bool updateAnimationState();

        void SetAnimation( const int rule );
        void IncreaseAnimFrame( const bool loop = false );

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

        void SetPosition( const int32_t cx, const int32_t cy );

        void SetLogs( StatusListBox * logs )
        {
            listlog = logs;
        }

        void SetMessage( const std::string & messageString, const bool top = false );
        void Redraw( fheroes2::Image & output ) const;

        const std::string & GetMessage() const
        {
            return _lastMessage;
        }

        void clear();

    private:
        fheroes2::Text _upperText;
        fheroes2::Text _lowerText;
        const fheroes2::Sprite & back1;
        const fheroes2::Sprite & back2;
        std::string _lastMessage;
        StatusListBox * listlog;
    };

    class TurnOrder : public fheroes2::Rect
    {
    public:
        TurnOrder();
        TurnOrder( const TurnOrder & ) = delete;

        TurnOrder & operator=( const TurnOrder & ) = delete;

        void Set( const fheroes2::Rect & rt, const std::shared_ptr<const Units> & units, const int army2Color );
        void Redraw( const Unit * current, const uint8_t currentUnitColor, fheroes2::Image & output );
        void QueueEventProcessing( std::string & msg, const fheroes2::Point & offset ) const;

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
        void SetAttackInfo( const Cell * cell, const Unit * attacker, const Unit * defender );
        void SetSpellAttackInfo( const Cell * cell, const HeroBase * hero, const Unit * defender, const Spell spell );
        void Reset();
        void Redraw() const;

    private:
        bool SetDamageInfoBase( const Cell * cell, const Unit * defender );

        fheroes2::Rect _battleUIRect;
        const Cell * _cell;
        const Unit * _defender;
        uint32_t _minDamage;
        uint32_t _maxDamage;
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
        void HumanTurn( const Unit & unit, Actions & actions );

        const fheroes2::Rect & GetArea() const
        {
            return _surfaceInnerArea;
        }

        // Battlefield interface ROI.
        const fheroes2::Rect & GetInterfaceRoi() const
        {
            return _interfacePosition;
        }

        fheroes2::Point getRelativeMouseCursorPos() const;

        void SetStatus( const std::string & message, const bool top = false );
        void SetOrderOfUnits( const std::shared_ptr<const Units> & units );
        void FadeArena( const bool clearMessageLog );

        void RedrawActionNewTurn() const;
        void RedrawActionAttackPart1( Unit & attacker, const Unit & defender, const TargetsInfo & targets );
        void RedrawActionAttackPart2( Unit & attacker, const Unit & defender, const TargetsInfo & targets, const uint32_t resurrects );
        void RedrawActionSpellCastStatus( const Spell & spell, int32_t dst, const std::string & name, const TargetsInfo & targets );
        void RedrawActionSpellCastPart1( const Spell & spell, int32_t dst, const HeroBase * caster, const TargetsInfo & targets );
        void RedrawActionSpellCastPart2( const Spell & spell, const TargetsInfo & targets );
        void RedrawActionResistSpell( const Unit & target, const bool playSound );
        void RedrawActionMonsterSpellCastStatus( const Spell & spell, const Unit & attacker, const TargetInfo & target );
        void RedrawActionMove( Unit & unit, const Indexes & path );
        void RedrawActionFly( Unit & unit, const Position & pos );
        void RedrawActionMorale( Unit & unit, const bool isGoodMorale );
        void RedrawActionLuck( const Unit & unit );
        void RedrawActionTowerPart1( const Tower & tower, const Unit & defender );
        void RedrawActionTowerPart2( const Tower & tower, const TargetInfo & target );
        void RedrawActionCatapultPart1( const CastleDefenseElement catapultTarget, const bool isHit );
        void RedrawActionCatapultPart2( const CastleDefenseElement catapultTarget );
        void RedrawActionTeleportSpell( Unit & target, const int32_t dst );
        void RedrawActionEarthQuakeSpell( const std::vector<CastleDefenseElement> & targets );
        void RedrawActionSummonElementalSpell( Unit & target );
        void RedrawActionMirrorImageSpell( const Unit & target, const Position & pos );
        void RedrawActionSkipStatus( const Unit & unit );
        void RedrawActionRemoveMirrorImage( const std::vector<Unit *> & mirrorImages );
        void RedrawBridgeAnimation( const bool bridgeDownAnimation );
        void RedrawMissileAnimation( const fheroes2::Point & startPos, const fheroes2::Point & endPos, const double angle, const uint32_t monsterID );

    private:
        enum CreatureSpellAnimation
        {
            NONE,
            WINCE,
            RESURRECT
        };

        void HumanBattleTurn( const Unit & unit, Actions & actions, std::string & msg );
        void HumanCastSpellTurn( const Unit & /* unused */, Actions & actions, std::string & msg );

        void RedrawCover();
        void _redrawBattleGround();
        void _redrawCoverStatic();
        void RedrawLowObjects( const int32_t cellId );
        void RedrawHighObjects( const int32_t cellId );
        void RedrawCastle( const Castle & castle, const int32_t cellId );
        void RedrawCastleMainTower( const Castle & castle );
        void RedrawKilled();
        void RedrawInterface();
        void RedrawOpponents();
        void RedrawOpponentsFlags();
        void redrawPreRender();
        void RedrawArmies();
        void RedrawTroopSprite( const Unit & unit );

        fheroes2::Point drawTroopSprite( const Unit & unit, const fheroes2::Sprite & troopSprite );

        void RedrawTroopCount( const Unit & unit );

        void RedrawActionWincesKills( const TargetsInfo & targets, Unit * attacker = nullptr, const Unit * defender = nullptr );
        void RedrawActionArrowSpell( const Unit & target );
        void RedrawActionColdRaySpell( Unit & target );
        void RedrawActionDisruptingRaySpell( const Unit & target );
        void RedrawActionBloodLustSpell( const Unit & target );
        void RedrawActionStoneSpell( const Unit & target );
        void RedrawActionColdRingSpell( const int32_t dst, const TargetsInfo & targets );
        void RedrawActionElementalStormSpell( const TargetsInfo & targets );
        void RedrawActionArmageddonSpell();
        void RedrawActionHolyShoutSpell( const uint8_t strength );
        void RedrawActionResurrectSpell( Unit & target, const Spell & spell );
        void RedrawActionDeathWaveSpell( const int32_t strength );
        void RedrawActionLightningBoltSpell( const Unit & target );
        void RedrawActionChainLightningSpell( const TargetsInfo & targets );
        void RedrawLightningOnTargets( const std::vector<fheroes2::Point> & points, const fheroes2::Rect & drawRoi ); // helper function
        void RedrawRaySpell( const Unit & target, const int spellICN, const int spellSound, const int32_t size );

        // Wait for all possible battlefield action delays that could be set in previous functions to pass.
        // Use this if a function may be called from other functions with different render delay types.
        void WaitForAllActionDelays();

        void AnimateOpponents( OpponentSprite * hero );
        void AnimateUnitWithDelay( Unit & unit, const bool skipLastFrameRender = false );
        void RedrawTroopDefaultDelay( Unit & unit );
        void RedrawTroopWithFrameAnimation( Unit & unit, const int icn, const int m82, const CreatureSpellAnimation animation );
        void RedrawTargetsWithFrameAnimation( const int32_t dst, const TargetsInfo & targets, const int icn, const int m82, int repeatCount = 0 );
        void RedrawTargetsWithFrameAnimation( const TargetsInfo & targets, const int icn, const int m82, const bool wnce );

        bool IdleTroopsAnimation() const;
        void ResetIdleTroopAnimation() const;
        void SwitchAllUnitsAnimation( const int32_t animationState ) const;
        void UpdateContourColor();
        void CheckGlobalEvents( LocalEvent & );
        void InterruptAutoBattleIfRequested( LocalEvent & le );
        void SetHeroAnimationReactionToTroopDeath( const int32_t deathColor ) const;

        void ProcessingHeroDialogResult( const int result, Actions & actions );

        void _openBattleSettingsDialog();
        void EventStartAutoBattle( const Unit & unit, Actions & actions );
        void EventAutoFinish( Actions & actions );
        void EventShowOptions();
        void ButtonAutoAction( const Unit & unit, Actions & actions );
        void ButtonSettingsAction();
        void ButtonSkipAction( Actions & actions );
        void MouseLeftClickBoardAction( const int themes, const Cell & cell, const bool isConfirmed, Actions & actions );
        void MousePressRightBoardAction( const Cell & cell ) const;

        int GetBattleCursor( std::string & statusMsg ) const;
        int GetBattleSpellCursor( std::string & statusMsg ) const;

        Arena & arena;
        Dialog::FrameBorder border;

        fheroes2::Rect _interfacePosition;
        fheroes2::Rect _surfaceInnerArea;
        fheroes2::Image _mainSurface;
        fheroes2::Image _battleGround;
        fheroes2::Image _hexagonGrid;
        fheroes2::Image _hexagonShadow;
        fheroes2::Image _hexagonGridShadow;
        fheroes2::Image _hexagonCursorShadow;

        int icn_cbkg;
        int icn_frng;

        fheroes2::Button btn_auto;
        fheroes2::Button btn_settings;
        fheroes2::Button btn_skip;
        Status status;

        std::unique_ptr<OpponentSprite> _opponent1;
        std::unique_ptr<OpponentSprite> _opponent2;

        Spell humanturn_spell;
        bool humanturn_exit;
        bool humanturn_redraw;
        uint32_t animation_flags_frame;
        int catapult_frame;

        int _interruptAutoBattleForColor;

        // The Channel ID of pre-battle sound. Used to check it is over to start the battle music.
        std::optional<int> _preBattleSoundChannelId{ -1 };

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
        // Index of the cell selected as the source for the Teleport spell
        int32_t _teleportSpellSrcIdx;
        fheroes2::Rect main_tower;

        std::unique_ptr<StatusListBox> listlog;

        PopupDamageInfo popup;
        TurnOrder _turnOrder;

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

        struct SwipeAttack
        {
            void setSrc( int theme, int32_t index, const Unit * unit )
            {
                currentUnit = unit;
                srcTheme = theme;
                srcCellIndex = index;
            }

            void setDst( int theme, int32_t index )
            {
                dstTheme = theme;
                dstCellIndex = index;
            }

            bool isValidDestination( int theme, int32_t index ) const
            {
                if ( !currentUnit ) {
                    return false;
                }

                if ( !Board::isNearIndexes( srcCellIndex, index ) ) {
                    return false;
                }

                if ( theme < Cursor::SWORD_TOPRIGHT || theme > Cursor::SWORD_BOTTOM ) {
                    return false;
                }

                if ( srcTheme != Cursor::WAR_MOVE && srcTheme != Cursor::WAR_FLY && srcCellIndex != currentUnit->GetHeadIndex()
                     && srcCellIndex != currentUnit->GetTailIndex() ) {
                    return false;
                }

                return true;
            }

            bool isValid() const
            {
                return isValidDestination( dstTheme, dstCellIndex );
            }

            const Unit * currentUnit{ nullptr };
            int32_t srcCellIndex{ -1 };
            int32_t dstCellIndex{ -1 };
            int srcTheme{ Cursor::NONE };
            int dstTheme{ Cursor::NONE };
        };

        SwipeAttack _swipeAttack;

        // TODO: While currently we don't need to persist 'UnitSpellEffectInfos' between render functions,
        // this may be needed in the future (for example, in expansion) to display some sprites over
        // troops for some time (e.g. long duration spell effects or other permanent effects).
        std::vector<UnitSpellEffectInfo> _unitSpellEffectInfos;

        struct BoardActionIntent
        {
            int cursorTheme = Cursor::NONE;
            int32_t cellIndex = -1;

            bool operator==( const BoardActionIntent & other ) const
            {
                return cursorTheme == other.cursorTheme && cellIndex == other.cellIndex;
            }
        };

        // Intents are used to confirm actions in combat performed using touch gestures
        BoardActionIntent _boardActionIntent;

        class BoardActionIntentUpdater
        {
        public:
            BoardActionIntentUpdater( BoardActionIntent & storedIntent, const bool isFromTouchpad )
                : _storedIntent( storedIntent )
                , _isFromTouchpad( isFromTouchpad )
            {}

            BoardActionIntentUpdater( const BoardActionIntentUpdater & ) = delete;

            ~BoardActionIntentUpdater()
            {
                // Do not remember intermediate touch gestures as intents
                if ( _isFromTouchpad ) {
                    return;
                }

                _storedIntent = _intent.value_or( BoardActionIntent{} );
            }

            BoardActionIntentUpdater & operator=( const BoardActionIntentUpdater & ) = delete;

            void setIntent( const BoardActionIntent & intent )
            {
                _intent = intent;
            }

            bool isConfirmed()
            {
                // If the mouse event has been triggered by the touchpad, it should be considered confirmed only if this event orders to
                // perform the same action that is already indicated on the battle board with the mouse cursor.
                return ( !_isFromTouchpad || _storedIntent == _intent );
            }

        private:
            BoardActionIntent & _storedIntent;
            const bool _isFromTouchpad;
            std::optional<BoardActionIntent> _intent;
        };
    };
}

#endif
