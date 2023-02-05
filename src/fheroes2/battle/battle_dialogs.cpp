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

#include <cassert>
#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "artifact.h"
#include "audio.h"
#include "audio_manager.h"
#include "battle.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_interface.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_audio.h"
#include "dialog_hotkeys.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "luck.h"
#include "math_base.h"
#include "monster.h"
#include "morale.h"
#include "mus.h"
#include "payment.h"
#include "players.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_option_item.h"

namespace
{
    // DialogBattleSummary text related values
    const int bsTextWidth = 270;
    const int bsTextXOffset = 25;
    const int bsTextYOffset = 175;
    const int bsTextIndent = 30;

    class LoopedAnimation
    {
    public:
        explicit LoopedAnimation( int icnId = 0, bool loop = false )
            : _icnId( icnId )
            , _frameId( 0 )
            , _counter( 0 )
            , _finished( false )
            , _loop( loop )
        {
            _frameId = ICN::AnimationFrame( _icnId, 1, _counter );
        }

        uint32_t frameId()
        {
            if ( _finished )
                return _frameId;

            ++_counter;
            uint32_t nextId = ICN::AnimationFrame( _icnId, 1, _counter );
            if ( nextId < _frameId ) {
                if ( _loop ) {
                    _counter = 0;
                    nextId = ICN::AnimationFrame( _icnId, 1, _counter );
                    std::swap( nextId, _frameId );
                    return nextId;
                }
                else {
                    _finished = true;
                }
            }
            else {
                std::swap( nextId, _frameId );
                return nextId;
            }

            return _frameId;
        }

        bool isFinished() const
        {
            return _finished;
        }

        int id() const
        {
            return _icnId;
        }

    private:
        int _icnId;
        uint32_t _frameId;
        uint32_t _counter;
        bool _finished;
        bool _loop;
    };

    class LoopedAnimationSequence
    {
    public:
        void push( int icnId, bool loop )
        {
            _queue.push( LoopedAnimation( icnId, loop ) );
        }

        uint32_t frameId()
        {
            if ( isFinished() )
                return 0;

            return _queue.front().frameId();
        }

        bool nextFrame() // returns true only if there is some frames left
        {
            if ( !_queue.empty() && _queue.front().isFinished() )
                _queue.pop();

            return _queue.empty();
        }

        bool isFinished() const
        {
            return _queue.empty();
        }

        int id() const
        {
            if ( isFinished() )
                return 0;

            return _queue.front().id();
        }

    private:
        std::queue<LoopedAnimation> _queue;
    };

    enum class DialogAction : int
    {
        Open,
        AudioSettings,
        HotKeys,
        Close
    };

    void RedrawBattleSettings( const std::vector<fheroes2::Rect> & areas )
    {
        assert( areas.size() == 9 );

        const Settings & conf = Settings::Get();

        int speed = conf.BattleSpeed();
        std::string str = _( "Speed: %{speed}" );
        StringReplace( str, "%{speed}", speed );
        uint32_t speedIcnIndex = 0;
        if ( speed >= 8 ) {
            speedIcnIndex = 2;
        }
        else if ( speed >= 5 ) {
            speedIcnIndex = 1;
        }

        const fheroes2::Sprite & speedIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, speedIcnIndex );
        fheroes2::drawOption( areas[0], speedIcon, _( "Speed" ), str, fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );

        const bool isShowArmyOrderEnabled = conf.BattleShowArmyOrder();
        const fheroes2::Sprite & armyOrderIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowArmyOrderEnabled ? 4 : 3 );
        fheroes2::drawOption( areas[1], armyOrderIcon, _( "Army Order" ), isShowArmyOrderEnabled ? _( "On" ) : _( "Off" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );

        const bool isBattleAudoSpellCastEnabled = conf.BattleAutoSpellcast();
        const fheroes2::Sprite & battleAutoSpellCastIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isBattleAudoSpellCastEnabled ? 7 : 6 );
        fheroes2::drawOption( areas[2], battleAutoSpellCastIcon, _( "Auto Spell Casting" ), isBattleAudoSpellCastEnabled ? _( "On" ) : _( "Off" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );

        const bool isShowBattleGridEnabled = conf.BattleShowGrid();
        const fheroes2::Sprite & battleGridIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowBattleGridEnabled ? 9 : 8 );
        fheroes2::drawOption( areas[3], battleGridIcon, _( "Grid" ), isShowBattleGridEnabled ? _( "On" ) : _( "Off" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );

        const bool isShowMoveShadowEnabled = conf.BattleShowMoveShadow();
        const fheroes2::Sprite & moveShadowIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowMoveShadowEnabled ? 11 : 10 );
        fheroes2::drawOption( areas[4], moveShadowIcon, _( "Shadow Movement" ), isShowMoveShadowEnabled ? _( "On" ) : _( "Off" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );

        const bool isShowMouseShadowEnabled = conf.BattleShowMouseShadow();
        const fheroes2::Sprite & mouseShadowIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowMouseShadowEnabled ? 13 : 12 );
        fheroes2::drawOption( areas[5], mouseShadowIcon, _( "Shadow Cursor" ), isShowMouseShadowEnabled ? _( "On" ) : _( "Off" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );

        const fheroes2::Sprite & audioSettingsIcon = fheroes2::AGG::GetICN( ICN::SPANEL, 1 );
        fheroes2::drawOption( areas[6], audioSettingsIcon, _( "Audio" ), _( "Settings" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );

        const fheroes2::Sprite & hotkeysIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, 5 );
        fheroes2::drawOption( areas[7], hotkeysIcon, _( "Hot Keys" ), _( "Configure" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );

        const bool isShowBattleDamageInfoEnabled = conf.isBattleShowDamageInfoEnabled();
        const fheroes2::Sprite & damageInfoIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowBattleDamageInfoEnabled ? 4 : 3 );
        fheroes2::drawOption( areas[8], damageInfoIcon, _( "Damage Info" ), isShowBattleDamageInfoEnabled ? _( "On" ) : _( "Off" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    DialogAction openBattleOptionDialog( bool & saveConfiguration )
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        LocalEvent & le = LocalEvent::Get();
        Settings & conf = Settings::Get();

        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::SPANBKGE : ICN::SPANBKG ), 0 );
        const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::SPANBKGE : ICN::SPANBKG ), 1 );

        const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
        const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

        fheroes2::ImageRestorer back( display, shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH );
        const fheroes2::Rect pos_rt( dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() );

        fheroes2::Fill( display, pos_rt.x, pos_rt.y, pos_rt.width, pos_rt.height, 0 );
        fheroes2::Blit( dialogShadow, display, pos_rt.x - BORDERWIDTH, pos_rt.y + BORDERWIDTH );
        fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

        const fheroes2::Sprite & panelSprite = fheroes2::AGG::GetICN( ICN::CSPANEL, 0 );
        const int32_t panelWidth = panelSprite.width();
        const int32_t panelHeight = panelSprite.height();

        const fheroes2::Point optionOffset( 36 + pos_rt.x, 47 + pos_rt.y );
        const fheroes2::Point optionStep( 92, 110 );

        std::vector<fheroes2::Rect> optionAreas;
        optionAreas.reserve( 9 );

        for ( int32_t y = 0; y < 3; ++y ) {
            for ( int32_t x = 0; x < 3; ++x ) {
                optionAreas.emplace_back( optionOffset.x + x * optionStep.x, optionOffset.y + y * optionStep.y, panelWidth, panelHeight );
            }
        }

        const fheroes2::Point buttonOffset( 112 + pos_rt.x, 362 + pos_rt.y );
        fheroes2::Button buttonOkay( buttonOffset.x, buttonOffset.y, isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD, 0, 1 );
        buttonOkay.draw();

        RedrawBattleSettings( optionAreas );

        display.render();

        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonOkay.area() ) ? buttonOkay.drawOnPress() : buttonOkay.drawOnRelease();

            bool redrawScreen = false;

            if ( le.MouseWheelUp( optionAreas[0] ) ) {
                conf.SetBattleSpeed( conf.BattleSpeed() + 1 );
                Game::UpdateGameSpeed();
                redrawScreen = true;
            }
            else if ( le.MouseWheelDn( optionAreas[0] ) ) {
                conf.SetBattleSpeed( conf.BattleSpeed() - 1 );
                Game::UpdateGameSpeed();
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( optionAreas[0] ) ) {
                conf.SetBattleSpeed( conf.BattleSpeed() % 10 + 1 );
                Game::UpdateGameSpeed();
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( optionAreas[1] ) ) {
                conf.setBattleShowArmyOrder( !conf.BattleShowArmyOrder() );
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( optionAreas[2] ) ) {
                conf.setBattleAutoSpellcast( !conf.BattleAutoSpellcast() );
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( optionAreas[3] ) ) {
                conf.SetBattleGrid( !conf.BattleShowGrid() );
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( optionAreas[4] ) ) {
                conf.SetBattleMovementShaded( !conf.BattleShowMoveShadow() );
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( optionAreas[5] ) ) {
                conf.SetBattleMouseShaded( !conf.BattleShowMouseShadow() );
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( optionAreas[6] ) ) {
                return DialogAction::AudioSettings;
            }
            if ( le.MouseClickLeft( optionAreas[7] ) ) {
                return DialogAction::HotKeys;
            }
            if ( le.MouseClickLeft( optionAreas[8] ) ) {
                conf.setBattleDamageInfo( !conf.isBattleShowDamageInfoEnabled() );
                redrawScreen = true;
            }

            if ( le.MousePressRight( optionAreas[0] ) ) {
                fheroes2::showStandardTextMessage( _( "Speed" ), _( "Set the speed of combat actions and animations." ), 0 );
            }
            else if ( le.MousePressRight( optionAreas[1] ) ) {
                fheroes2::showStandardTextMessage( _( "Army Order" ), _( "Toggle to display army order during the battle." ), 0 );
            }
            else if ( le.MousePressRight( optionAreas[2] ) ) {
                fheroes2::showStandardTextMessage(
                    _( "Auto Spell Casting" ),
                    _( "Toggle whether or not the computer will cast spells for you when auto combat is on. (Note: This does not affect spell casting for computer players in any way, nor does it affect quick combat.)" ),
                    0 );
            }
            else if ( le.MousePressRight( optionAreas[3] ) ) {
                fheroes2::showStandardTextMessage(
                    _( "Grid" ),
                    _( "Toggle the hex grid on or off. The hex grid always underlies movement, even if turned off. This switch only determines if the grid is visible." ),
                    0 );
            }
            else if ( le.MousePressRight( optionAreas[4] ) ) {
                fheroes2::showStandardTextMessage( _( "Shadow Movement" ), _( "Toggle on or off shadows showing where your creatures can move and attack." ), 0 );
            }
            else if ( le.MousePressRight( optionAreas[5] ) ) {
                fheroes2::showStandardTextMessage( _( "Shadow Cursor" ), _( "Toggle on or off a shadow showing the current hex location of the mouse cursor." ), 0 );
            }
            else if ( le.MousePressRight( optionAreas[6] ) ) {
                fheroes2::showStandardTextMessage( _( "Audio" ), _( "Change the audio settings of the game." ), 0 );
            }
            else if ( le.MousePressRight( optionAreas[7] ) ) {
                fheroes2::showStandardTextMessage( _( "Hot Keys" ), _( "Check and configure all the hot keys present in the game." ), 0 );
            }
            else if ( le.MousePressRight( optionAreas[8] ) ) {
                Dialog::Message( _( "Damage Info" ), _( "Toggle to display damage information during the battle." ), Font::BIG );
            }
            else if ( le.MousePressRight( buttonOkay.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Exit this menu." ), 0 );
            }

            if ( Game::HotKeyCloseWindow() || le.MouseClickLeft( buttonOkay.area() ) ) {
                break;
            }

            if ( redrawScreen ) {
                fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );
                RedrawBattleSettings( optionAreas );
                display.render();

                saveConfiguration = true;
            }
        }

        return DialogAction::Close;
    }
}

namespace Battle
{
    void GetSummaryParams( uint32_t res1, uint32_t res2, const HeroBase * hero, uint32_t exp, LoopedAnimationSequence & sequence, std::string & title,
                           std::string & msg );
}

void Battle::DialogBattleSettings()
{
    // We should make file writing only once.
    bool saveConfiguration = false;
    const Settings & conf = Settings::Get();

    DialogAction action = DialogAction::Open;

    while ( action != DialogAction::Close ) {
        switch ( action ) {
        case DialogAction::Open:
            action = openBattleOptionDialog( saveConfiguration );
            break;
        case DialogAction::AudioSettings:
            Dialog::openAudioSettingsDialog( false );
            action = DialogAction::Open;
            break;
        case DialogAction::HotKeys:
            fheroes2::openHotkeysDialog();
            action = DialogAction::Open;
            break;
        default:
            break;
        }
    }

    if ( saveConfiguration ) {
        conf.Save( Settings::configFileName );
    }
}

void Battle::GetSummaryParams( uint32_t res1, uint32_t res2, const HeroBase * hero, uint32_t exp, LoopedAnimationSequence & sequence, std::string & title,
                               std::string & msg )
{
    if ( res1 & RESULT_WINS ) {
        sequence.push( ICN::WINCMBT, true );

        if ( res2 & RESULT_SURRENDER ) {
            title.append( _( "The enemy has surrendered!" ) );
        }
        else if ( res2 & RESULT_RETREAT ) {
            title.append( _( "The enemy has fled!" ) );
        }
        else {
            title.append( _( "A glorious victory!" ) );
        }

        if ( hero && hero->isHeroes() ) {
            msg.append( _( "For valor in combat, %{name} receives %{exp} experience." ) );
            StringReplace( msg, "%{name}", hero->GetName() );
            StringReplace( msg, "%{exp}", exp );
        }
    }
    else if ( res1 & RESULT_RETREAT ) {
        assert( hero != nullptr );

        sequence.push( ICN::CMBTFLE1, false );
        sequence.push( ICN::CMBTFLE2, false );
        sequence.push( ICN::CMBTFLE3, false );

        msg.append( _( "The cowardly %{name} flees from battle." ) );
        StringReplace( msg, "%{name}", hero->GetName() );
    }
    else if ( res1 & RESULT_SURRENDER ) {
        assert( hero != nullptr );

        sequence.push( ICN::CMBTSURR, true );

        msg.append( _( "%{name} surrenders to the enemy, and departs in shame." ) );
        StringReplace( msg, "%{name}", hero->GetName() );
    }
    else {
        sequence.push( ICN::CMBTLOS1, false );
        sequence.push( ICN::CMBTLOS2, false );
        sequence.push( ICN::CMBTLOS3, true );

        if ( hero && hero->isHeroes() ) {
            msg.append( _( "Your force suffer a bitter defeat, and %{name} abandons your cause." ) );
            StringReplace( msg, "%{name}", hero->GetName() );
        }
        else {
            msg.append( _( "Your force suffer a bitter defeat." ) );
        }
    }
}

// Returns true if player want to restart the battle
bool Battle::Arena::DialogBattleSummary( const Result & res, const std::vector<Artifact> & artifacts, bool allowToCancel ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();
    const Settings & conf = Settings::Get();

    const Troops killed1 = _army1->GetKilledTroops();
    const Troops killed2 = _army2->GetKilledTroops();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::string msg;
    std::string title;
    LoopedAnimationSequence sequence;

    if ( ( res.army1 & RESULT_WINS ) && ( _army1->GetControl() & CONTROL_HUMAN ) ) {
        GetSummaryParams( res.army1, res.army2, _army1->GetCommander(), res.exp1, sequence, title, msg );
        AudioManager::PlayMusic( MUS::BATTLEWIN, Music::PlaybackMode::PLAY_ONCE );
    }
    else if ( ( res.army2 & RESULT_WINS ) && ( _army2->GetControl() & CONTROL_HUMAN ) ) {
        GetSummaryParams( res.army2, res.army1, _army2->GetCommander(), res.exp2, sequence, title, msg );
        AudioManager::PlayMusic( MUS::BATTLEWIN, Music::PlaybackMode::PLAY_ONCE );
    }
    else if ( _army1->GetControl() & CONTROL_HUMAN ) {
        GetSummaryParams( res.army1, res.army2, _army1->GetCommander(), res.exp1, sequence, title, msg );
        AudioManager::PlayMusic( MUS::BATTLELOSE, Music::PlaybackMode::PLAY_ONCE );
    }
    else if ( _army2->GetControl() & CONTROL_HUMAN ) {
        GetSummaryParams( res.army2, res.army1, _army2->GetCommander(), res.exp2, sequence, title, msg );
        AudioManager::PlayMusic( MUS::BATTLELOSE, Music::PlaybackMode::PLAY_ONCE );
    }
    else {
        // AI vs AI battle, this dialog should not be shown at all
        assert( 0 );

        return false;
    }

    if ( sequence.isFinished() ) {
        // This shouldn't happen
        assert( 0 );

        sequence.push( ICN::UNKNOWN, false );
    }

    const bool isEvilInterface = conf.isEvilInterfaceEnabled();
    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::WINLOSEE : ICN::WINLOSE ), 0 );
    const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::WINLOSEE : ICN::WINLOSE ), 1 );

    const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
    const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

    fheroes2::ImageRestorer back( display, shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH - 1 );
    const fheroes2::Rect pos_rt( dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() );

    fheroes2::Blit( dialogShadow, display, pos_rt.x - BORDERWIDTH, pos_rt.y + BORDERWIDTH - 1 );
    fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

    const int anime_ox = 47;
    const int anime_oy = 36;

    const fheroes2::Sprite & sequenceBase = fheroes2::AGG::GetICN( sequence.id(), 0 );
    const fheroes2::Sprite & sequenceStart = fheroes2::AGG::GetICN( sequence.id(), 1 );

    fheroes2::Blit( sequenceBase, display, pos_rt.x + anime_ox + sequenceBase.x(), pos_rt.y + anime_oy + sequenceBase.y() );
    fheroes2::Blit( sequenceStart, display, pos_rt.x + anime_ox + sequenceStart.x(), pos_rt.y + anime_oy + sequenceStart.y() );

    int32_t messageYOffset = 0;
    if ( !title.empty() ) {
        TextBox box( title, Font::YELLOW_BIG, bsTextWidth );
        box.Blit( pos_rt.x + bsTextXOffset, pos_rt.y + bsTextYOffset );
        messageYOffset = bsTextIndent;
    }

    if ( !msg.empty() ) {
        TextBox box( msg, Font::BIG, bsTextWidth );
        box.Blit( pos_rt.x + bsTextXOffset, pos_rt.y + bsTextYOffset + messageYOffset );
    }

    // battlefield casualties
    Text text( _( "Battlefield Casualties" ), Font::SMALL );
    text.Blit( pos_rt.x + ( pos_rt.width - text.w() ) / 2, pos_rt.y + 270 );

    // attacker
    text.Set( _( "Attacker" ), Font::SMALL );
    text.Blit( pos_rt.x + ( pos_rt.width - text.w() ) / 2, pos_rt.y + 285 );

    if ( killed1.isValid() )
        Army::drawSingleDetailedMonsterLine( killed1, pos_rt.x + 25, pos_rt.y + 308, 270 );
    else {
        text.Set( _( "None" ), Font::SMALL );
        text.Blit( pos_rt.x + ( pos_rt.width - text.w() ) / 2, pos_rt.y + 300 );
    }

    // defender
    text.Set( _( "Defender" ), Font::SMALL );
    text.Blit( pos_rt.x + ( pos_rt.width - text.w() ) / 2, pos_rt.y + 345 );

    if ( killed2.isValid() )
        Army::drawSingleDetailedMonsterLine( killed2, pos_rt.x + 25, pos_rt.y + 368, 270 );
    else {
        text.Set( _( "None" ), Font::SMALL );
        text.Blit( pos_rt.x + ( pos_rt.width - text.w() ) / 2, pos_rt.y + 360 );
    }

    if ( allowToCancel ) {
        const fheroes2::Sprite & buttonOverride = fheroes2::Crop( dialog, 20, 410, 84, 32 );
        fheroes2::Blit( buttonOverride, display, pos_rt.x + 116, pos_rt.y + 410 );
    }

    const int buttonOffset = allowToCancel ? 39 : 120;
    const int buttonOkICN = isEvilInterface ? ( allowToCancel ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALLER_OKAY_EVIL )
                                            : ( allowToCancel ? ICN::BUTTON_SMALL_OKAY_GOOD : ICN::BUTTON_SMALLER_OKAY_GOOD );
    const int buttonCancelICN = isEvilInterface ? ICN::BUTTON_SMALL_RESTART_EVIL : ICN::BUTTON_SMALL_RESTART_GOOD;

    std::unique_ptr<fheroes2::ButtonBase> btnOk;
    fheroes2::ButtonSprite btnCancel = fheroes2::makeButtonWithShadow( pos_rt.x + buttonOffset + 129, pos_rt.y + 410, fheroes2::AGG::GetICN( buttonCancelICN, 0 ),
                                                                       fheroes2::AGG::GetICN( buttonCancelICN, 1 ), display );

    if ( allowToCancel ) {
        btnCancel.draw();
        btnOk.reset( new fheroes2::ButtonSprite( fheroes2::makeButtonWithShadow( pos_rt.x + buttonOffset, pos_rt.y + 410, fheroes2::AGG::GetICN( buttonOkICN, 0 ),
                                                                                 fheroes2::AGG::GetICN( buttonOkICN, 1 ), display ) ) );
    }
    else {
        btnOk.reset( new fheroes2::Button( pos_rt.x + buttonOffset, pos_rt.y + 410, buttonOkICN, 0, 1 ) );
    }
    btnOk->draw();

    display.render();

    while ( le.HandleEvents() ) {
        le.MousePressLeft( btnOk->area() ) ? btnOk->drawOnPress() : btnOk->drawOnRelease();
        if ( allowToCancel ) {
            le.MousePressLeft( btnCancel.area() ) ? btnCancel.drawOnPress() : btnCancel.drawOnRelease();
        }

        // exit
        if ( Game::HotKeyCloseWindow() || le.MouseClickLeft( btnOk->area() ) )
            break;

        if ( allowToCancel && le.MouseClickLeft( btnCancel.area() ) ) {
            // Skip artifact transfer and return to restart battle in manual mode
            return true;
        }

        // animation
        if ( Game::validateAnimationDelay( Game::BATTLE_DIALOG_DELAY ) && !sequence.nextFrame() ) {
            const fheroes2::Sprite & base = fheroes2::AGG::GetICN( sequence.id(), 0 );
            const fheroes2::Sprite & sequenceCurrent = fheroes2::AGG::GetICN( sequence.id(), sequence.frameId() );

            fheroes2::Blit( base, display, pos_rt.x + anime_ox + sequenceBase.x(), pos_rt.y + anime_oy + sequenceBase.y() );
            fheroes2::Blit( sequenceCurrent, display, pos_rt.x + anime_ox + sequenceCurrent.x(), pos_rt.y + anime_oy + sequenceCurrent.y() );
            display.render();
        }
    }

    if ( !artifacts.empty() ) {
        const HeroBase * winner = ( res.army1 & RESULT_WINS ? _army1->GetCommander() : ( res.army2 & RESULT_WINS ? _army2->GetCommander() : nullptr ) );
        const HeroBase * loser = ( res.army1 & RESULT_LOSS ? _army1->GetCommander() : ( res.army2 & RESULT_LOSS ? _army2->GetCommander() : nullptr ) );

        // Can't transfer artifacts
        if ( winner == nullptr || loser == nullptr )
            return false;

        const bool isWinnerHuman = winner && winner->isControlHuman();

        btnOk.reset( new fheroes2::Button( pos_rt.x + 120, pos_rt.y + 410, isEvilInterface ? ICN::WINCMBBE : ICN::WINCMBTB, 0, 1 ) );

        for ( const Artifact & art : artifacts ) {
            if ( isWinnerHuman || art.isUltimate() ) { // always show the message for ultimate artifacts
                back.restore();
                back.update( shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH - 1 );
                fheroes2::Blit( dialogShadow, display, pos_rt.x - BORDERWIDTH, pos_rt.y + BORDERWIDTH - 1 );
                fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

                btnOk->draw();

                std::string artMsg;
                if ( art.isUltimate() ) {
                    if ( isWinnerHuman ) {
                        artMsg = _( "As you reach for the %{name}, it mysteriously disappears." );
                    }
                    else {
                        artMsg = _( "As your enemy reaches for the %{name}, it mysteriously disappears." );
                    }
                    StringReplace( artMsg, "%{name}", art.GetName() );
                }
                else {
                    artMsg = _( "You have captured an enemy artifact!" );
                    Game::PlayPickupSound();
                }

                TextBox box( artMsg, Font::YELLOW_BIG, bsTextWidth );
                box.Blit( pos_rt.x + bsTextXOffset, pos_rt.y + bsTextYOffset );

                const fheroes2::Sprite & border = fheroes2::AGG::GetICN( ICN::WINLOSEB, 0 );
                const fheroes2::Sprite & artifact = fheroes2::AGG::GetICN( ICN::ARTIFACT, art.IndexSprite64() );
                const fheroes2::Point artifactOffset( pos_rt.x + 119, pos_rt.y + 310 );

                fheroes2::Blit( border, display, artifactOffset.x, artifactOffset.y );
                fheroes2::Blit( artifact, display, artifactOffset.x + 8, artifactOffset.y + 8 );

                TextBox artName( art.GetName(), Font::SMALL, bsTextWidth );
                artName.Blit( pos_rt.x + bsTextXOffset, artifactOffset.y + border.height() + 5 );

                const fheroes2::Rect artifactArea( artifactOffset.x, artifactOffset.y, border.width(), border.height() );

                while ( le.HandleEvents() ) {
                    le.MousePressLeft( btnOk->area() ) ? btnOk->drawOnPress() : btnOk->drawOnRelease();

                    // display captured artifact info on right click
                    if ( le.MousePressRight( artifactArea ) ) {
                        fheroes2::ArtifactDialogElement( art ).showPopup( Dialog::ZERO );
                    }

                    // exit
                    if ( Game::HotKeyCloseWindow() || le.MouseClickLeft( btnOk->area() ) )
                        break;

                    // animation
                    if ( Game::validateAnimationDelay( Game::BATTLE_DIALOG_DELAY ) && !sequence.nextFrame() ) {
                        const fheroes2::Sprite & base = fheroes2::AGG::GetICN( sequence.id(), 0 );
                        const fheroes2::Sprite & sequenceCurrent = fheroes2::AGG::GetICN( sequence.id(), sequence.frameId() );

                        fheroes2::Blit( base, display, pos_rt.x + anime_ox + sequenceBase.x(), pos_rt.y + anime_oy + sequenceBase.y() );
                        fheroes2::Blit( sequenceCurrent, display, pos_rt.x + anime_ox + sequenceCurrent.x(), pos_rt.y + anime_oy + sequenceCurrent.y() );
                        display.render();
                    }
                }
            }
        }
    }
    return false;
}

void Battle::Arena::DialogBattleNecromancy( const uint32_t raiseCount, const uint32_t raisedMonsterType ) const
{
    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::WINLOSEE : ICN::WINLOSE ), 0 );
    const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::WINLOSEE : ICN::WINLOSE ), 1 );

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
    const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

    fheroes2::ImageRestorer back( display, shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH - 1 );
    const fheroes2::Rect renderArea( dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() );

    fheroes2::Blit( dialogShadow, display, renderArea.x - BORDERWIDTH, renderArea.y + BORDERWIDTH - 1 );
    fheroes2::Blit( dialog, display, renderArea.x, renderArea.y );

    LoopedAnimationSequence sequence;
    sequence.push( ICN::WINCMBT, true );

    if ( sequence.isFinished() ) // Cannot be!
        sequence.push( ICN::UNKNOWN, false );

    const fheroes2::Sprite & sequenceBase = fheroes2::AGG::GetICN( sequence.id(), 0 );
    const fheroes2::Sprite & sequenceStart = fheroes2::AGG::GetICN( sequence.id(), 1 );

    const fheroes2::Point sequenceRenderAreaOffset( 47, 36 );

    fheroes2::Blit( sequenceBase, display, renderArea.x + sequenceRenderAreaOffset.x + sequenceBase.x(), renderArea.y + sequenceRenderAreaOffset.y + sequenceBase.y() );
    fheroes2::Blit( sequenceStart, display, renderArea.x + sequenceRenderAreaOffset.x + sequenceStart.x(),
                    renderArea.y + sequenceRenderAreaOffset.y + sequenceStart.y() );

    int xOffset = renderArea.x + bsTextXOffset;
    int yOffset = renderArea.y + bsTextYOffset;

    TextBox titleBox( _( "Necromancy!" ), Font::YELLOW_BIG, bsTextWidth );
    titleBox.Blit( xOffset, yOffset );

    const Monster mons( raisedMonsterType );
    std::string msg = _( "Practicing the dark arts of necromancy, you are able to raise %{count} of the enemy's dead to return under your service as %{monster}." );
    StringReplace( msg, "%{count}", raiseCount );
    StringReplace( msg, "%{monster}", mons.GetPluralName( raiseCount ) );

    TextBox messageBox( msg, Font::BIG, bsTextWidth );
    yOffset += bsTextIndent;
    messageBox.Blit( xOffset, yOffset );

    const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( ICN::MONS32, mons.GetSpriteIndex() );
    yOffset += messageBox.h() + monsterSprite.height();
    fheroes2::Blit( monsterSprite, display, ( display.width() - monsterSprite.width() ) / 2, yOffset );

    const Text raiseCountText( std::to_string( raiseCount ), Font::SMALL );
    yOffset += 30;
    raiseCountText.Blit( ( display.width() - raiseCountText.w() ) / 2, yOffset, bsTextWidth );
    Game::PlayPickupSound();

    const int buttonOffset = 121;
    const int buttonICN = isEvilInterface ? ICN::WINCMBBE : ICN::WINCMBTB;
    fheroes2::Button buttonOk( renderArea.x + buttonOffset, renderArea.y + 410, buttonICN, 0, 1 );
    buttonOk.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

        // exit
        if ( Game::HotKeyCloseWindow() || le.MouseClickLeft( buttonOk.area() ) )
            break;

        // animation
        if ( Game::validateAnimationDelay( Game::BATTLE_DIALOG_DELAY ) && !sequence.nextFrame() ) {
            const fheroes2::Sprite & base = fheroes2::AGG::GetICN( sequence.id(), 0 );
            const fheroes2::Sprite & sequenceCurrent = fheroes2::AGG::GetICN( sequence.id(), sequence.frameId() );

            fheroes2::Blit( base, display, renderArea.x + sequenceRenderAreaOffset.x + sequenceBase.x(), renderArea.y + sequenceRenderAreaOffset.y + sequenceBase.y() );
            fheroes2::Blit( sequenceCurrent, display, renderArea.x + sequenceRenderAreaOffset.x + sequenceCurrent.x(),
                            renderArea.y + sequenceRenderAreaOffset.y + sequenceCurrent.y() );
            display.render();
        }
    }
}

int Battle::Arena::DialogBattleHero( const HeroBase & hero, const bool buttons, Status & status ) const
{
    const Settings & conf = Settings::Get();

    Cursor & cursor = Cursor::Get();
    cursor.SetThemes( Cursor::POINTER );

    const bool readonly = current_color != hero.GetColor() || !buttons;
    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( conf.isEvilInterfaceEnabled() ? ICN::VGENBKGE : ICN::VGENBKG ), 0 );

    const fheroes2::Point dialogShadow( 15, 15 );

    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Rect pos_rt( ( display.width() - dialog.width() - dialogShadow.x ) / 2, ( display.height() - dialog.height() - dialogShadow.y ) / 2, dialog.width(),
                           dialog.height() );

    fheroes2::ImageRestorer back( display, pos_rt.x, pos_rt.y, pos_rt.width, pos_rt.height );

    fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

    // first 15 pixels in the dialog is left shadow, skip
    pos_rt.x += 15;
    pos_rt.width -= 15;

    const fheroes2::Rect portraitArea( pos_rt.x + 7, pos_rt.y + 35, 113, 108 );
    const Heroes * actionHero = ( current_color == hero.GetColor() ) ? dynamic_cast<const Heroes *>( &hero ) : nullptr;

    hero.PortraitRedraw( pos_rt.x + 12, pos_rt.y + 42, PORT_BIG, display );
    int col = ( Color::NONE == hero.GetColor() ? 1 : Color::GetIndex( hero.GetColor() ) + 1 );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::VIEWGEN, col ), display, pos_rt.x + 133, pos_rt.y + 36 );

    fheroes2::Point tp( pos_rt.x, pos_rt.y );

    std::string str;
    Text text;
    text.Set( Font::SMALL );
    str = hero.isCaptain() ? _( "Captain of %{name}" ) : _( "%{name} the %{race}" );
    StringReplace( str, "%{name}", hero.GetName() );
    StringReplace( str, "%{race}", Race::String( hero.GetRace() ) );
    text.Set( str );
    tp.x = pos_rt.x + ( pos_rt.width - text.w() ) / 2;
    tp.y = pos_rt.y + 11;
    text.Blit( tp.x, tp.y );
    str = _( "Attack" ) + std::string( ": " ) + std::to_string( hero.GetAttack() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 40;
    text.Blit( tp.x, tp.y );
    str = _( "Defense" ) + std::string( ": " ) + std::to_string( hero.GetDefense() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 51;
    text.Blit( tp.x, tp.y );
    str = _( "Spell Power" ) + std::string( ": " ) + std::to_string( hero.GetPower() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 62;
    text.Blit( tp.x, tp.y );
    str = _( "Knowledge" ) + std::string( ": " ) + std::to_string( hero.GetKnowledge() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 73;
    text.Blit( tp.x, tp.y );
    str = _( "Morale" ) + std::string( ": " ) + Morale::String( hero.GetMorale() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 84;
    text.Blit( tp.x, tp.y );
    str = _( "Luck" ) + std::string( ": " ) + Luck::String( hero.GetLuck() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 95;
    text.Blit( tp.x, tp.y );
    str = _( "Spell Points" ) + std::string( ": " ) + std::to_string( hero.GetSpellPoints() ) + "/" + std::to_string( hero.GetMaxSpellPoints() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 117;
    text.Blit( tp.x, tp.y );

    fheroes2::Button btnCast( pos_rt.x + 15, pos_rt.y + 148, ICN::VIEWGEN, 9, 10 );
    fheroes2::Button btnRetreat( pos_rt.x + 74, pos_rt.y + 148, ICN::VIEWGEN, 11, 12 );
    fheroes2::Button btnSurrender( pos_rt.x + 133, pos_rt.y + 148, ICN::VIEWGEN, 13, 14 );
    fheroes2::Button btnClose( pos_rt.x + 192, pos_rt.y + 148, ICN::VIEWGEN, 15, 16 );

    if ( readonly || !hero.HaveSpellBook() || hero.Modes( Heroes::SPELLCASTED ) )
        btnCast.disable();

    if ( readonly || !CanRetreatOpponent( hero.GetColor() ) )
        btnRetreat.disable();

    if ( readonly || !CanSurrenderOpponent( hero.GetColor() ) )
        btnSurrender.disable();

    btnCast.draw();
    btnRetreat.draw();
    btnSurrender.draw();
    btnClose.draw();

    int result = 0;

    display.render();

    std::string statusMessage = _( "Hero's Options" );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() && !result ) {
        btnCast.isEnabled() && le.MousePressLeft( btnCast.area() ) ? btnCast.drawOnPress() : btnCast.drawOnRelease();
        btnRetreat.isEnabled() && le.MousePressLeft( btnRetreat.area() ) ? btnRetreat.drawOnPress() : btnRetreat.drawOnRelease();
        btnSurrender.isEnabled() && le.MousePressLeft( btnSurrender.area() ) ? btnSurrender.drawOnPress() : btnSurrender.drawOnRelease();
        le.MousePressLeft( btnClose.area() ) ? btnClose.drawOnPress() : btnClose.drawOnRelease();

        if ( buttons ) {
            // The Cast Spell is available for a hero and a captain.
            if ( le.MouseCursor( btnCast.area() ) && current_color == hero.GetColor() ) {
                statusMessage = _( "Cast Spell" );
            }
            // The retreat is available during a player's turn only. A captain cannot retreat.
            else if ( le.MouseCursor( btnRetreat.area() ) && current_color == hero.GetColor() && !hero.isCaptain() ) {
                statusMessage = _( "Retreat" );
            }
            // The surrender is available during a player's turn only. A captain cannot surrender.
            else if ( le.MouseCursor( btnSurrender.area() ) && current_color == hero.GetColor() && !hero.isCaptain() ) {
                statusMessage = _( "Surrender" );
            }
            else if ( le.MouseCursor( btnClose.area() ) ) {
                statusMessage = _( "Cancel" );
            }
            // The Hero Screen is available for a Hero only (not Captain) and when UI is not read-only.
            else if ( le.MouseCursor( portraitArea ) && actionHero != nullptr && actionHero->isHeroes() && !readonly ) {
                statusMessage = _( "Hero Screen" );
            }
            else if ( hero.isCaptain() ) {
                statusMessage = _( "Captain's Options" );
            }
            else {
                statusMessage = _( "Hero's Options" );
            }
        }

        if ( !buttons && !le.MousePressRight() )
            break;

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_CAST_SPELL ) || ( btnCast.isEnabled() && le.MouseClickLeft( btnCast.area() ) ) )
            result = 1;

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_RETREAT ) || ( btnRetreat.isEnabled() && le.MouseClickLeft( btnRetreat.area() ) ) )
            result = 2;

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_SURRENDER ) || ( btnSurrender.isEnabled() && le.MouseClickLeft( btnSurrender.area() ) ) )
            result = 3;

        if ( le.MouseClickLeft( portraitArea ) && actionHero != nullptr ) {
            // IMPORTANT!!! This is extremely dangerous but we have no choice with current code. Make sure that this trick doesn't allow user to modify the hero.
            LocalEvent::GetClean();
            const_cast<Heroes *>( actionHero )->OpenDialog( true, false, true, true );
        }

        if ( le.MousePressRight( btnCast.area() ) && current_color == hero.GetColor() ) {
            Dialog::Message( _( "Cast Spell" ),
                             _( "Cast a magical spell. You may only cast one spell per combat round. The round is reset when every creature has had a turn." ),
                             Font::BIG );
        }
        else if ( le.MousePressRight( btnRetreat.area() ) && current_color == hero.GetColor() && !hero.isCaptain() ) {
            Dialog::Message(
                _( "Retreat" ),
                _( "Retreat your hero, abandoning your creatures. Your hero will be available for you to recruit again, however, the hero will have only a novice hero's forces." ),
                Font::BIG );
        }
        else if ( le.MousePressRight( btnSurrender.area() ) && current_color == hero.GetColor() && !hero.isCaptain() ) {
            Dialog::Message(
                _( "Surrender" ),
                _( "Surrendering costs gold. However if you pay the ransom, the hero and all of his or her surviving creatures will be available to recruit again." ),
                Font::BIG );
        }
        else if ( le.MousePressRight( portraitArea ) && actionHero != nullptr ) {
            Dialog::Message( _( "Hero Screen" ), _( "Open Hero Screen to view full information about the hero." ), Font::BIG );
        }
        else if ( le.MousePressRight( btnClose.area() ) ) {
            Dialog::Message( _( "Cancel" ), _( "Return to the battle." ), Font::BIG );
        }

        // exit
        if ( Game::HotKeyCloseWindow() || le.MouseClickLeft( btnClose.area() ) )
            break;

        if ( statusMessage != status.GetMessage() ) {
            status.SetMessage( statusMessage );
            status.Redraw( display );
            display.render( status );
        }
    }

    return result;
}

bool Battle::DialogBattleSurrender( const HeroBase & hero, uint32_t cost, Kingdom & kingdom )
{
    if ( kingdom.GetColor() == hero.GetColor() ) // this is weird. You're surrending to yourself!
        return false;

    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();
    const Settings & conf = Settings::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const bool isEvilInterface = conf.isEvilInterfaceEnabled();

    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( isEvilInterface ? ICN::SURDRBKE : ICN::SURDRBKG, 0 );

    fheroes2::Rect pos_rt( ( display.width() - dialog.width() + 16 ) / 2, ( display.height() - dialog.height() + 16 ) / 2, dialog.width(), dialog.height() );

    fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

    const int icnAccept = isEvilInterface ? ICN::BUTTON_SMALL_ACCEPT_EVIL : ICN::BUTTON_SMALL_ACCEPT_GOOD;
    const int icnDecline = isEvilInterface ? ICN::BUTTON_SMALL_DECLINE_EVIL : ICN::BUTTON_SMALL_DECLINE_GOOD;
    const int icnMarket = isEvilInterface ? ICN::EVIL_MARKET_BUTTON : ICN::GOOD_MARKET_BUTTON;

    fheroes2::ButtonSprite btnAccept
        = fheroes2::makeButtonWithShadow( pos_rt.x + 91, pos_rt.y + 152, fheroes2::AGG::GetICN( icnAccept, 0 ), fheroes2::AGG::GetICN( icnAccept, 1 ), display );

    fheroes2::ButtonSprite btnDecline
        = fheroes2::makeButtonWithShadow( pos_rt.x + 295, pos_rt.y + 152, fheroes2::AGG::GetICN( icnDecline, 0 ), fheroes2::AGG::GetICN( icnDecline, 1 ), display );

    fheroes2::ButtonSprite btnMarket = fheroes2::makeButtonWithShadow( pos_rt.x + ( pos_rt.width - 16 ) / 2, pos_rt.y + 145, fheroes2::AGG::GetICN( icnMarket, 0 ),
                                                                       fheroes2::AGG::GetICN( icnMarket, 1 ), display );

    if ( !kingdom.AllowPayment( payment_t( Resource::GOLD, cost ) ) ) {
        btnAccept.disable();
    }

    if ( kingdom.GetCountMarketplace() ) {
        if ( kingdom.AllowPayment( payment_t( Resource::GOLD, cost ) ) ) {
            btnMarket.disable();
        }
        else {
            btnMarket.draw();
        }
    }
    else {
        btnMarket.disable();
    }

    btnAccept.draw();
    btnDecline.draw();

    auto drawGoldMsg = [cost, &kingdom, &btnAccept]() {
        std::string str = _( "Not enough gold (%{gold})" );

        StringReplace( str, "%{gold}", cost - kingdom.GetFunds().Get( Resource::GOLD ) );

        const Text text( str, Font::SMALL );
        const fheroes2::Rect rect = btnAccept.area();

        text.Blit( rect.x + ( rect.width - text.w() ) / 2, rect.y - 15 );
    };

    const int icn = isEvilInterface ? ICN::SURRENDE : ICN::SURRENDR;
    const fheroes2::Sprite & window = fheroes2::AGG::GetICN( icn, 4 );
    fheroes2::Blit( window, display, pos_rt.x + 55, pos_rt.y + 32 );
    hero.PortraitRedraw( pos_rt.x + 60, pos_rt.y + 38, PORT_BIG, display );

    std::string str = hero.isCaptain() ? _( "Captain of %{name} states:" ) : _( "%{name} states:" );
    StringReplace( str, "%{name}", hero.GetName() );

    Text text( str, Font::BIG );
    text.Blit( pos_rt.x + 312 - text.w() / 2, pos_rt.y + 30 );

    str = _( "\"I will accept your surrender and grant you and your troops safe passage for the price of %{price} gold.\"" );
    StringReplace( str, "%{price}", cost );

    TextBox box( str, Font::BIG, 275 );
    box.Blit( pos_rt.x + 175, pos_rt.y + 50 );

    fheroes2::ImageRestorer back( display, pos_rt.x, pos_rt.y, pos_rt.width, pos_rt.height );

    if ( !kingdom.AllowPayment( payment_t( Resource::GOLD, cost ) ) ) {
        drawGoldMsg();
    }

    display.render();

    bool result = false;

    while ( le.HandleEvents() && !result ) {
        if ( btnAccept.isEnabled() )
            le.MousePressLeft( btnAccept.area() ) ? btnAccept.drawOnPress() : btnAccept.drawOnRelease();
        le.MousePressLeft( btnDecline.area() ) ? btnDecline.drawOnPress() : btnDecline.drawOnRelease();

        if ( btnMarket.isEnabled() )
            le.MousePressLeft( btnMarket.area() ) ? btnMarket.drawOnPress() : btnMarket.drawOnRelease();

        if ( btnAccept.isEnabled() && le.MouseClickLeft( btnAccept.area() ) )
            result = true;

        if ( btnMarket.isEnabled() && le.MouseClickLeft( btnMarket.area() ) ) {
            Dialog::Marketplace( kingdom, false );

            back.restore();

            if ( kingdom.AllowPayment( payment_t( Resource::GOLD, cost ) ) ) {
                btnAccept.enable();
            }
            else {
                btnAccept.disable();

                drawGoldMsg();
            }

            btnAccept.draw();
            display.render();
        }

        // exit
        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( btnDecline.area() ) )
            break;
    }

    return result;
}
