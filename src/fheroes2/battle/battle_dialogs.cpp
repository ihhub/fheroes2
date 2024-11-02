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
#include "battle_arena.h" // IWYU pragma: associated
#include "battle_army.h"
#include "battle_interface.h" // IWYU pragma: associated
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_audio.h"
#include "dialog_hotkeys.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
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
#include "players.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_option_item.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    // DialogBattleSummary text related values
    const int bsTextWidth = 303;
    const int bsTextXOffset = 25;
    const int bsTextYOffset = 160;
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
            _frameId = ICN::getAnimatedIcnIndex( _icnId, 1, _counter );
        }

        uint32_t frameId()
        {
            if ( _finished )
                return _frameId;

            ++_counter;
            uint32_t nextId = ICN::getAnimatedIcnIndex( _icnId, 1, _counter );
            if ( nextId < _frameId ) {
                if ( _loop ) {
                    _counter = 0;
                    nextId = ICN::getAnimatedIcnIndex( _icnId, 1, _counter );
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

        const bool isShowTurnOrderEnabled = conf.BattleShowTurnOrder();
        const fheroes2::Sprite & turnOrderIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowTurnOrderEnabled ? 4 : 3 );
        fheroes2::drawOption( areas[1], turnOrderIcon, _( "Turn Order" ), isShowTurnOrderEnabled ? _( "On" ) : _( "Off" ),
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

        const fheroes2::Sprite & hotkeysIcon = fheroes2::AGG::GetICN( ICN::GAME_OPTION_ICON, 0 );
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

        // Set the cursor image. This dialog is called from the battlefield and does not require a cursor restorer.
        // Battlefield event processor will set the appropriate cursor after this dialog is closed.
        Cursor::Get().SetThemes( Cursor::POINTER );

        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::SPANBKGE : ICN::SPANBKG ), 0 );
        const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::SPANBKGE : ICN::SPANBKG ), 1 );

        const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
        const fheroes2::Point shadowOffset( dialogOffset.x - fheroes2::borderWidthPx, dialogOffset.y );

        const fheroes2::ImageRestorer back( display, shadowOffset.x, shadowOffset.y, dialog.width() + fheroes2::borderWidthPx,
                                            dialog.height() + fheroes2::borderWidthPx );
        const fheroes2::Rect pos_rt( dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() );

        fheroes2::Fill( display, pos_rt.x, pos_rt.y, pos_rt.width, pos_rt.height, 0 );
        fheroes2::Blit( dialogShadow, display, pos_rt.x - fheroes2::borderWidthPx, pos_rt.y + fheroes2::borderWidthPx );
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
            le.isMouseLeftButtonPressedInArea( buttonOkay.area() ) ? buttonOkay.drawOnPress() : buttonOkay.drawOnRelease();

            bool redrawScreen = false;

            if ( le.isMouseWheelUpInArea( optionAreas[0] ) ) {
                conf.SetBattleSpeed( conf.BattleSpeed() + 1 );
                Game::UpdateGameSpeed();
                redrawScreen = true;
            }
            else if ( le.isMouseWheelDownInArea( optionAreas[0] ) ) {
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
                conf.setBattleShowTurnOrder( !conf.BattleShowTurnOrder() );
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

            if ( le.isMouseRightButtonPressedInArea( optionAreas[0] ) ) {
                fheroes2::showStandardTextMessage( _( "Speed" ), _( "Set the speed of combat actions and animations." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( optionAreas[1] ) ) {
                fheroes2::showStandardTextMessage( _( "Turn Order" ), _( "Toggle to display the turn order during the battle." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( optionAreas[2] ) ) {
                fheroes2::showStandardTextMessage(
                    _( "Auto Spell Casting" ),
                    _( "Toggle whether or not the computer will cast spells for you when auto combat is on. (Note: This does not affect spell casting for computer players in any way, nor does it affect quick combat.)" ),
                    0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( optionAreas[3] ) ) {
                fheroes2::showStandardTextMessage(
                    _( "Grid" ),
                    _( "Toggle the hex grid on or off. The hex grid always underlies movement, even if turned off. This switch only determines if the grid is visible." ),
                    0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( optionAreas[4] ) ) {
                fheroes2::showStandardTextMessage( _( "Shadow Movement" ), _( "Toggle on or off shadows showing where your creatures can move and attack." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( optionAreas[5] ) ) {
                fheroes2::showStandardTextMessage( _( "Shadow Cursor" ), _( "Toggle on or off a shadow showing the current hex location of the mouse cursor." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( optionAreas[6] ) ) {
                fheroes2::showStandardTextMessage( _( "Audio" ), _( "Change the audio settings of the game." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( optionAreas[7] ) ) {
                fheroes2::showStandardTextMessage( _( "Hot Keys" ), _( "Check and configure all the hot keys present in the game." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( optionAreas[8] ) ) {
                fheroes2::showStandardTextMessage( _( "Damage Info" ), _( "Toggle to display damage information during the battle." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOkay.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Exit this menu." ), 0 );
            }

            if ( Game::HotKeyCloseWindow() || le.MouseClickLeft( buttonOkay.area() ) ) {
                break;
            }

            if ( redrawScreen ) {
                fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );
                RedrawBattleSettings( optionAreas );
                buttonOkay.draw();
                display.render();

                saveConfiguration = true;
            }
        }

        return DialogAction::Close;
    }
}

namespace Battle
{
    void GetSummaryParams( const uint32_t res1, const uint32_t res2, const HeroBase * hero, const uint32_t exp, const uint32_t surrenderCost,
                           LoopedAnimationSequence & sequence, std::string & title, std::string & surrenderText, std::string & outcomeText );
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
            saveConfiguration |= Dialog::openAudioSettingsDialog( false );
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

void Battle::GetSummaryParams( const uint32_t res1, const uint32_t res2, const HeroBase * hero, const uint32_t exp, const uint32_t surrenderCost,
                               LoopedAnimationSequence & sequence, std::string & title, std::string & surrenderText, std::string & outcomeText )
{
    if ( res1 & RESULT_WINS ) {
        sequence.push( ICN::WINCMBT, true );

        if ( res2 & RESULT_SURRENDER ) {
            title.append( _( "The enemy has surrendered!" ) );
            surrenderText.append( _( "Their cowardice costs them %{gold} gold." ) );
            StringReplace( surrenderText, "%{gold}", surrenderCost );
        }
        else if ( res2 & RESULT_RETREAT ) {
            title.append( _( "The enemy has fled!" ) );
        }
        else {
            title.append( _( "A glorious victory!" ) );
        }

        if ( hero && hero->isHeroes() ) {
            outcomeText.append( _( "For valor in combat, %{name} receives %{exp} experience." ) );
            StringReplace( outcomeText, "%{name}", hero->GetName() );
            StringReplace( outcomeText, "%{exp}", exp );
        }
    }
    else if ( res1 & RESULT_RETREAT ) {
        assert( hero != nullptr );

        sequence.push( ICN::CMBTFLE1, false );
        sequence.push( ICN::CMBTFLE2, false );
        sequence.push( ICN::CMBTFLE3, false );

        title.append( _( "The cowardly %{name} flees from battle." ) );
        StringReplace( title, "%{name}", hero->GetName() );
    }
    else if ( res1 & RESULT_SURRENDER ) {
        assert( hero != nullptr );

        sequence.push( ICN::CMBTSURR, true );

        title.append( _( "%{name} surrenders to the enemy, and departs in shame." ) );
        StringReplace( title, "%{name}", hero->GetName() );
    }
    else {
        sequence.push( ICN::CMBTLOS1, false );
        sequence.push( ICN::CMBTLOS2, false );
        sequence.push( ICN::CMBTLOS3, true );

        if ( hero && hero->isHeroes() ) {
            title.append( _( "Your forces suffer a bitter defeat, and %{name} abandons your cause." ) );
            StringReplace( title, "%{name}", hero->GetName() );
        }
        else {
            title.append( _( "Your forces suffer a bitter defeat." ) );
        }
    }
}

// Returns true if player wants to restart the battle
bool Battle::Arena::DialogBattleSummary( const Result & res, const std::vector<Artifact> & artifacts, const bool allowToRestart ) const
{
    const bool attackerIsHuman = _army1->GetControl() & CONTROL_HUMAN;
    const bool defenderIsHuman = _army2->GetControl() & CONTROL_HUMAN;

    if ( !attackerIsHuman && !defenderIsHuman ) {
        // AI vs AI battle, this dialog should not be shown
        assert( 0 );
        return false;
    }

    fheroes2::Display & display = fheroes2::Display::instance();

    // Set the cursor image. After this dialog the Game Area or the Battlefield will be shown, so it does not require a cursor restorer.
    Cursor::Get().SetThemes( Cursor::POINTER );

    fheroes2::StandardWindow background( bsTextWidth + 32, 424, true, display );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    const fheroes2::Sprite & originalBorderImage = fheroes2::AGG::GetICN( isEvilInterface ? ICN::WINLOSEE : ICN::WINLOSE, 0 );
    const fheroes2::Rect animationBorderRoi{ 43, 32, 231, 133 };

    const fheroes2::Rect & roi( background.activeArea() );
    const fheroes2::Rect animationRoi( roi.x + ( ( roi.width - animationBorderRoi.width ) / 2 ) + 4, roi.y + 21, animationBorderRoi.width, animationBorderRoi.height );
    Copy( originalBorderImage, animationBorderRoi.x, animationBorderRoi.y, display, animationRoi.x - 4, animationRoi.y - 4, animationRoi.width, animationRoi.height );

    // Setup summary texts according to results and get the corresponding animation sequence.
    std::string surrenderText;
    std::string outcomeText;
    std::string title;
    LoopedAnimationSequence sequence;

    fheroes2::FontType summaryTitleFont = fheroes2::FontType::normalWhite();
    if ( ( res.army1 & RESULT_WINS ) && attackerIsHuman ) {
        GetSummaryParams( res.army1, res.army2, _army1->GetCommander(), res.exp1, _army2->GetSurrenderCost(), sequence, title, surrenderText, outcomeText );
        summaryTitleFont = fheroes2::FontType::normalYellow();
        AudioManager::PlayMusic( MUS::BATTLEWIN, Music::PlaybackMode::PLAY_ONCE );
    }
    else if ( ( res.army2 & RESULT_WINS ) && defenderIsHuman ) {
        GetSummaryParams( res.army2, res.army1, _army2->GetCommander(), res.exp2, _army1->GetSurrenderCost(), sequence, title, surrenderText, outcomeText );
        summaryTitleFont = fheroes2::FontType::normalYellow();
        AudioManager::PlayMusic( MUS::BATTLEWIN, Music::PlaybackMode::PLAY_ONCE );
    }
    else if ( attackerIsHuman ) {
        GetSummaryParams( res.army1, res.army2, _army1->GetCommander(), res.exp1, 0, sequence, title, surrenderText, outcomeText );
        AudioManager::PlayMusic( MUS::BATTLELOSE, Music::PlaybackMode::PLAY_ONCE );
    }
    else if ( defenderIsHuman ) {
        GetSummaryParams( res.army2, res.army1, _army2->GetCommander(), res.exp2, 0, sequence, title, surrenderText, outcomeText );
        AudioManager::PlayMusic( MUS::BATTLELOSE, Music::PlaybackMode::PLAY_ONCE );
    }

    if ( sequence.isFinished() ) {
        // This shouldn't happen
        assert( 0 );

        sequence.push( ICN::UNKNOWN, false );
    }

    // Setup animation
    const fheroes2::Sprite & sequenceBase = fheroes2::AGG::GetICN( sequence.id(), 0 );
    const fheroes2::Sprite & sequenceStart = fheroes2::AGG::GetICN( sequence.id(), 1 );
    Copy( sequenceBase, 0, 0, display, animationRoi.x, animationRoi.y, sequenceBase.width(), sequenceBase.height() );
    fheroes2::Blit( sequenceStart, display, animationRoi.x + sequenceStart.x(), animationRoi.y + sequenceStart.y() );

    const fheroes2::Rect summaryRoi( roi.x + 11, roi.y + bsTextYOffset, roi.width - 22, roi.height - bsTextYOffset );
    fheroes2::ImageRestorer summaryBackground( display, summaryRoi.x, summaryRoi.y, roi.width, summaryRoi.height );

    const int32_t casualtiesOffsetY = summaryRoi.y + 96;
    int32_t summaryBodyOffset = summaryRoi.y;
    int32_t remainingSummaryBodyHeight = casualtiesOffsetY - summaryBodyOffset;

    // Draw texts
    if ( !title.empty() ) {
        fheroes2::Text box( title, summaryTitleFont );
        box.setUniformVerticalAlignment( false );
        box.draw( summaryRoi.x, summaryBodyOffset, summaryRoi.width, display );
        summaryBodyOffset += box.height( summaryRoi.width );
        remainingSummaryBodyHeight -= box.height( summaryRoi.width );
    }

    const fheroes2::FontType bodyFont = fheroes2::FontType::normalWhite();
    if ( !outcomeText.empty() ) {
        if ( !surrenderText.empty() ) {
            // Divide the main text area evenly between the two texts bodies by splitting it into 3 equal parts.
            fheroes2::Text upperText( surrenderText, bodyFont );
            fheroes2::Text lowerText( outcomeText, bodyFont );
            const int32_t inbetweenSpace = ( remainingSummaryBodyHeight - upperText.height( summaryRoi.width ) - lowerText.height( summaryRoi.width ) ) / 3;
            upperText.setUniformVerticalAlignment( false );
            lowerText.setUniformVerticalAlignment( false );
            upperText.draw( summaryRoi.x, summaryBodyOffset + inbetweenSpace, summaryRoi.width, display );

            lowerText.draw( summaryRoi.x, summaryBodyOffset + upperText.height( summaryRoi.width ) + inbetweenSpace * 2, summaryRoi.width, display );
        }
        else {
            fheroes2::Text upperText( outcomeText, bodyFont );
            upperText.setUniformVerticalAlignment( false );
            upperText.draw( summaryRoi.x, summaryBodyOffset + remainingSummaryBodyHeight / 2 - ( upperText.height( summaryRoi.width ) / 2 ), summaryRoi.width, display );
        }
    }
    else if ( !surrenderText.empty() ) {
        const fheroes2::Text upperText( surrenderText, bodyFont );
        upperText.draw( summaryRoi.x, summaryBodyOffset + remainingSummaryBodyHeight / 2 - ( upperText.height( summaryRoi.width ) / 2 ), summaryRoi.width, display );
    }

    // Battlefield casualties
    const fheroes2::FontType casualtiesFont = fheroes2::FontType::smallWhite();
    fheroes2::Text text( _( "Battlefield Casualties" ), casualtiesFont );
    text.draw( summaryRoi.x + ( summaryRoi.width - text.width() ) / 2, casualtiesOffsetY, display );

    // Attacker
    text.set( _( "Attacker" ), casualtiesFont );
    text.draw( summaryRoi.x + ( summaryRoi.width - text.width() ) / 2, casualtiesOffsetY + 15, display );

    const Troops killed1 = _army1->GetKilledTroops();
    const Troops killed2 = _army2->GetKilledTroops();

    if ( killed1.isValid() ) {
        Army::drawSingleDetailedMonsterLine( killed1, summaryRoi.x + 13, casualtiesOffsetY + 36, roi.width - 47 );
    }
    else {
        text.set( _( "None" ), casualtiesFont );
        text.draw( summaryRoi.x + ( summaryRoi.width - text.width() ) / 2, casualtiesOffsetY + 30, display );
    }

    // defender
    text.set( _( "Defender" ), casualtiesFont );
    text.draw( summaryRoi.x + ( summaryRoi.width - text.width() ) / 2, casualtiesOffsetY + 75, display );

    if ( killed2.isValid() ) {
        Army::drawSingleDetailedMonsterLine( killed2, summaryRoi.x + 13, casualtiesOffsetY + 96, roi.width - 47 );
    }
    else {
        text.set( _( "None" ), casualtiesFont );
        text.draw( summaryRoi.x + ( summaryRoi.width - text.width() ) / 2, casualtiesOffsetY + 90, display );
    }

    int32_t buttonHorizontalMargin = 0;
    const int32_t buttonVerticalMargin = 5;
    std::unique_ptr<fheroes2::Button> buttonRestart;
    if ( allowToRestart ) {
        buttonRestart = std::make_unique<fheroes2::Button>();
        buttonHorizontalMargin = 23;
        background.renderButton( *buttonRestart, isEvilInterface ? ICN::BUTTON_SMALL_RESTART_EVIL : ICN::BUTTON_SMALL_RESTART_GOOD, 0, 1,
                                 { buttonHorizontalMargin, buttonVerticalMargin }, fheroes2::StandardWindow::Padding::BOTTOM_RIGHT );
    }

    const int buttonOkICN = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;

    fheroes2::Button buttonOk;
    const fheroes2::StandardWindow::Padding buttonOkPadding
        = allowToRestart ? fheroes2::StandardWindow::Padding::BOTTOM_LEFT : fheroes2::StandardWindow::Padding::BOTTOM_CENTER;
    background.renderButton( buttonOk, buttonOkICN, 0, 1, { buttonHorizontalMargin, buttonVerticalMargin }, buttonOkPadding );

    if ( Game::validateDisplayFadeIn() ) {
        fheroes2::fadeInDisplay();
    }
    else {
        display.render( background.totalArea() );
    }

    LocalEvent & le = LocalEvent::Get();

    int sequenceId = sequence.id();

    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        if ( allowToRestart ) {
            le.isMouseLeftButtonPressedInArea( buttonRestart->area() ) ? buttonRestart->drawOnPress() : buttonRestart->drawOnRelease();
        }

        if ( Game::HotKeyCloseWindow() || le.MouseClickLeft( buttonOk.area() ) ) {
            break;
        }
        if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to leave the battle results." ), Dialog::ZERO );
        }
        else if ( allowToRestart ) {
            if ( le.MouseClickLeft( buttonRestart->area() ) ) {
                // Skip artifact transfer and return to restart the battle in manual mode
                return true;
            }
            if ( le.isMouseRightButtonPressedInArea( buttonRestart->area() ) ) {
                fheroes2::showStandardTextMessage( _( "Restart" ), _( "Click to restart the battle in manual mode." ), Dialog::ZERO );
            }
        }

        // Animation
        if ( Game::validateAnimationDelay( Game::BATTLE_DIALOG_DELAY ) && !sequence.nextFrame() ) {
            if ( sequenceId != sequence.id() ) {
                sequenceId = sequence.id();
                const fheroes2::Sprite & base = fheroes2::AGG::GetICN( sequenceId, 0 );

                Copy( base, 0, 0, display, animationRoi.x + base.x(), animationRoi.y + base.y(), base.width(), base.height() );
            }
            const fheroes2::Sprite & sequenceCurrent = fheroes2::AGG::GetICN( sequence.id(), sequence.frameId() );

            fheroes2::Blit( sequenceCurrent, display, animationRoi.x + sequenceCurrent.x(), animationRoi.y + sequenceCurrent.y() );
            display.render( animationRoi );
        }
    }

    if ( !artifacts.empty() ) {
        const HeroBase * winner = ( res.army1 & RESULT_WINS ? _army1->GetCommander() : ( res.army2 & RESULT_WINS ? _army2->GetCommander() : nullptr ) );
        const HeroBase * loser = ( res.army1 & RESULT_LOSS ? _army1->GetCommander() : ( res.army2 & RESULT_LOSS ? _army2->GetCommander() : nullptr ) );

        // Cannot transfer artifacts
        if ( winner == nullptr || loser == nullptr ) {
            return false;
        }
        const bool isWinnerHuman = winner && winner->isControlHuman();

        // Nothing to do if the AI won and there are no Ultimate Artifacts.
        if ( !isWinnerHuman && !loser->GetBagArtifacts().ContainUltimateArtifact() ) {
            return false;
        }

        summaryBackground.restore();

        background.renderButton( buttonOk, buttonOkICN, 0, 1, { 0, buttonVerticalMargin }, fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        const fheroes2::Sprite & border = fheroes2::AGG::GetICN( ICN::WINLOSEB, 0 );
        const fheroes2::Rect artifactArea( summaryRoi.x + ( summaryRoi.width - border.width() ) / 2, casualtiesOffsetY + 38, border.width(), border.height() );
        Copy( border, 0, 0, display, artifactArea.x, artifactArea.y, artifactArea.width, artifactArea.height );

        fheroes2::ImageRestorer artifactHeader( display, summaryRoi.x, summaryRoi.y, summaryRoi.width, 66 );
        fheroes2::ImageRestorer artifactName( display, summaryRoi.x, artifactArea.y + artifactArea.height, summaryRoi.width, 18 );
        std::string artMsg;

        display.render( summaryRoi );

        bool needHeaderRedraw = false;
        int prevArtifactId = -1;

        for ( const Artifact & art : artifacts ) {
            // Only the Ultimate Artifacts are shown for both the winner and loser's dialogs. Skip if it is a regular artifact and the AI won.
            if ( !isWinnerHuman && !art.isUltimate() ) {
                continue;
            }

            // If two identical artifacts go in a row, then we do not need to redraw anything, but only play the sound if necessary.
            if ( prevArtifactId == art.GetID() ) {
                // Sound is never played for Ultimate Artifact messages.
                if ( isWinnerHuman && !art.isUltimate() ) {
                    Game::PlayPickupSound();
                }
            }
            else {
                const char * const artName = art.GetName();

                if ( !art.isUltimate() ) {
                    // Only draw the regular artifact header once.
                    if ( !needHeaderRedraw ) {
                        artMsg = _( "You have captured an enemy artifact!" );

                        const fheroes2::Text box( artMsg, fheroes2::FontType::normalYellow() );
                        box.draw( summaryRoi.x, summaryRoi.y, summaryRoi.width, display );

                        needHeaderRedraw = true;
                    }
                    Game::PlayPickupSound();
                }
                else {
                    // Ultimate artifacts are always displayed after all the regular artifacts.
                    if ( needHeaderRedraw ) {
                        artifactHeader.restore();
                    }
                    if ( isWinnerHuman ) {
                        artMsg = _( "As you reach for the %{name}, it mysteriously disappears." );
                    }
                    else {
                        artMsg = _( "As your enemy reaches for the %{name}, it mysteriously disappears." );
                    }
                    StringReplace( artMsg, "%{name}", artName );

                    const fheroes2::Text box( artMsg, fheroes2::FontType::normalYellow() );
                    box.draw( summaryRoi.x, summaryRoi.y, summaryRoi.width, display );

                    needHeaderRedraw = true;
                }

                const fheroes2::Sprite & artifact = fheroes2::AGG::GetICN( ICN::ARTIFACT, art.IndexSprite64() );
                Copy( artifact, 0, 0, display, artifactArea.x + 8, artifactArea.y + 8, artifact.width(), artifact.height() );

                artifactName.restore();

                const fheroes2::Text artNameText( artName, fheroes2::FontType::smallWhite() );
                artNameText.draw( summaryRoi.x, artifactArea.y + border.height() + 7, summaryRoi.width, display );

                prevArtifactId = art.GetID();

                display.render( summaryRoi );
            }

            while ( le.HandleEvents() ) {
                le.isMouseLeftButtonPressedInArea( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

                // Display captured artifact info on right click
                if ( le.isMouseRightButtonPressedInArea( artifactArea ) ) {
                    fheroes2::ArtifactDialogElement( art ).showPopup( Dialog::ZERO );
                }
                else if ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyCloseWindow() ) {
                    break;
                }
                else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "Okay" ), _( "Exit this menu." ), Dialog::ZERO );
                }
                // Animation
                if ( Game::validateAnimationDelay( Game::BATTLE_DIALOG_DELAY ) && !sequence.nextFrame() ) {
                    if ( sequenceId != sequence.id() ) {
                        sequenceId = sequence.id();
                        const fheroes2::Sprite & base = fheroes2::AGG::GetICN( sequenceId, 0 );

                        Copy( base, 0, 0, display, animationRoi.x + base.x(), animationRoi.y + base.y(), base.width(), base.height() );
                    }
                    const fheroes2::Sprite & sequenceCurrent = fheroes2::AGG::GetICN( sequence.id(), sequence.frameId() );

                    fheroes2::Blit( sequenceCurrent, display, animationRoi.x + sequenceCurrent.x(), animationRoi.y + sequenceCurrent.y() );
                    display.render( animationRoi );
                }
            }
        }
    }
    return false;
}

void Battle::Arena::DialogBattleNecromancy( const uint32_t raiseCount )
{
    // Set the cursor image. This dialog does not require a cursor restorer.
    Cursor::Get().SetThemes( Cursor::POINTER );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::WINLOSEE : ICN::WINLOSE ), 0 );
    const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::WINLOSEE : ICN::WINLOSE ), 1 );

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
    const fheroes2::Point shadowOffset( dialogOffset.x - fheroes2::borderWidthPx, dialogOffset.y );

    const fheroes2::ImageRestorer back( display, shadowOffset.x, shadowOffset.y, dialog.width() + fheroes2::borderWidthPx,
                                        dialog.height() + fheroes2::borderWidthPx - 1 );
    const fheroes2::Rect renderArea( dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() );

    fheroes2::Blit( dialogShadow, display, renderArea.x - fheroes2::borderWidthPx, renderArea.y + fheroes2::borderWidthPx - 1 );
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
    int yOffset = renderArea.y + bsTextYOffset + 15;

    const int adjustedTextWidth = bsTextWidth - 33;

    const fheroes2::Text titleBox( _( "Necromancy!" ), fheroes2::FontType::normalYellow() );
    titleBox.draw( xOffset, yOffset + 2, adjustedTextWidth, display );

    const Monster mons( Monster::SKELETON );
    std::string msg = _( "Practicing the dark arts of necromancy, you are able to raise %{count} of the enemy's dead to return under your service as %{monster}." );
    StringReplace( msg, "%{count}", raiseCount );
    StringReplace( msg, "%{monster}", mons.GetPluralName( raiseCount ) );

    const fheroes2::Text messageBox( msg, fheroes2::FontType::normalWhite() );
    yOffset += bsTextIndent;
    messageBox.draw( xOffset, yOffset + 2, adjustedTextWidth, display );

    const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( ICN::MONS32, mons.GetSpriteIndex() );
    yOffset += messageBox.height( adjustedTextWidth ) + monsterSprite.height();
    fheroes2::Blit( monsterSprite, display, ( display.width() - monsterSprite.width() ) / 2, yOffset );

    fheroes2::Text raiseCountText( std::to_string( raiseCount ), fheroes2::FontType::smallWhite() );
    raiseCountText.fitToOneRow( adjustedTextWidth );
    yOffset += 30;
    raiseCountText.draw( ( display.width() - raiseCountText.width() ) / 2, yOffset + 2, display );
    Game::PlayPickupSound();

    const int buttonOffset = 121;
    const int buttonICN = isEvilInterface ? ICN::BUTTON_SMALLER_OKAY_EVIL : ICN::BUTTON_SMALLER_OKAY_GOOD;
    fheroes2::Button buttonOk( renderArea.x + buttonOffset, renderArea.y + 410, buttonICN, 0, 1 );
    buttonOk.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

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

int Battle::Arena::DialogBattleHero( HeroBase & hero, const bool buttons, Status & status ) const
{
    const Settings & conf = Settings::Get();

    Cursor & cursor = Cursor::Get();
    cursor.SetThemes( Cursor::POINTER );

    const int currentColor = GetCurrentColor();
    const bool readonly = ( currentColor != hero.GetColor() || !buttons );
    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( conf.isEvilInterfaceEnabled() ? ICN::VGENBKGE : ICN::VGENBKG ), 0 );

    const fheroes2::Point dialogShadow( 15, 15 );

    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Rect pos_rt( ( display.width() - dialog.width() - dialogShadow.x ) / 2, ( display.height() - dialog.height() - dialogShadow.y ) / 2, dialog.width(),
                           dialog.height() );

    fheroes2::ImageRestorer back( display, pos_rt.x, pos_rt.y, pos_rt.width, pos_rt.height );

    fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

    // first 15 pixels in the dialog is left shadow, skip
    pos_rt.x += dialogShadow.x;
    pos_rt.width -= dialogShadow.x;

    const fheroes2::Rect portraitArea( pos_rt.x + 7, pos_rt.y + 35, 113, 108 );

    hero.PortraitRedraw( pos_rt.x + 12, pos_rt.y + 42, PORT_BIG, display );
    int col = ( Color::NONE == hero.GetColor() ? 1 : Color::GetIndex( hero.GetColor() ) + 1 );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::VIEWGEN, col ), display, pos_rt.x + 133, pos_rt.y + 36 );

    std::string str = hero.isCaptain() ? _( "Captain of %{name}" ) : _( "%{name} the %{race}" );
    StringReplace( str, "%{name}", hero.GetName() );
    StringReplace( str, "%{race}", Race::String( hero.GetRace() ) );
    fheroes2::Text text( str, fheroes2::FontType::smallWhite() );
    fheroes2::Point tp{ pos_rt.x + ( pos_rt.width - text.width() ) / 2, pos_rt.y + 11 };
    text.draw( tp.x, tp.y + 2, display );

    const int32_t letterShadowCompensation = 1;
    const fheroes2::Point statsTextOffset{ pos_rt.x + 133 + letterShadowCompensation, pos_rt.y + 40 };
    const int32_t maxStatsTextWidth{ 109 };
    const int32_t statsTextRowHeight{ 11 };

    str = _( "Attack" ) + std::string( ": " ) + std::to_string( hero.GetAttack() );
    text.set( str, fheroes2::FontType::smallWhite() );
    tp.x = statsTextOffset.x + ( maxStatsTextWidth - text.width() ) / 2;
    tp.y = statsTextOffset.y;
    text.draw( tp.x, tp.y + 2, display );
    str = _( "Defense" ) + std::string( ": " ) + std::to_string( hero.GetDefense() );
    text.set( str, fheroes2::FontType::smallWhite() );
    tp.x = statsTextOffset.x + ( maxStatsTextWidth - text.width() ) / 2;
    tp.y += statsTextRowHeight;
    text.draw( tp.x, tp.y + 2, display );
    str = _( "Spell Power" ) + std::string( ": " ) + std::to_string( hero.GetPower() );
    text.set( str, fheroes2::FontType::smallWhite() );
    tp.x = statsTextOffset.x + ( maxStatsTextWidth - text.width() ) / 2;
    tp.y += statsTextRowHeight;
    text.draw( tp.x, tp.y + 2, display );
    str = _( "Knowledge" ) + std::string( ": " ) + std::to_string( hero.GetKnowledge() );
    text.set( str, fheroes2::FontType::smallWhite() );
    tp.x = statsTextOffset.x + ( maxStatsTextWidth - text.width() ) / 2;
    tp.y += statsTextRowHeight;
    text.draw( tp.x, tp.y + 2, display );
    str = _( "Morale" ) + std::string( ": " ) + Morale::String( hero.GetMorale() );
    text.set( str, fheroes2::FontType::smallWhite() );
    tp.x = statsTextOffset.x;
    tp.y += statsTextRowHeight;
    text.setUniformVerticalAlignment( false );
    text.draw( tp.x, tp.y + 2, maxStatsTextWidth, display );
    tp.y += text.height( maxStatsTextWidth );
    str = _( "Luck" ) + std::string( ": " ) + Luck::String( hero.GetLuck() );
    text.set( str, fheroes2::FontType::smallWhite() );
    tp.x = statsTextOffset.x;
    text.draw( tp.x, tp.y + 2, maxStatsTextWidth, display );
    tp.y += text.height( maxStatsTextWidth );
    str = _( "Spell Points" ) + std::string( ": " ) + std::to_string( hero.GetSpellPoints() ) + "/" + std::to_string( hero.GetMaxSpellPoints() );
    text.set( str, fheroes2::FontType::smallWhite() );
    tp.x = statsTextOffset.x;
    // By default the spell points should have one line of space between it and the Luck, but if there isn't
    // any space due to morale and luck taking up four lines, then move it down to the lowest line.
    const int32_t compensation = ( tp.y - ( statsTextOffset.y + 88 ) ) == 0 ? 11 : 0;
    text.draw( tp.x, statsTextOffset.y + 79 + compensation, maxStatsTextWidth, display );

    fheroes2::Button btnCast( pos_rt.x + 15, pos_rt.y + 148, ICN::VIEWGEN, 9, 10 );
    fheroes2::Button btnRetreat( pos_rt.x + 74, pos_rt.y + 148, ICN::VIEWGEN, 11, 12 );
    fheroes2::Button btnSurrender( pos_rt.x + 133, pos_rt.y + 148, ICN::VIEWGEN, 13, 14 );
    fheroes2::Button btnClose( pos_rt.x + 192, pos_rt.y + 148, ICN::VIEWGEN, 15, 16 );

    if ( readonly || !hero.HaveSpellBook() || hero.Modes( Heroes::SPELLCASTED ) ) {
        btnCast.disable();
    }

    if ( readonly || !CanRetreatOpponent( hero.GetColor() ) ) {
        btnRetreat.disable();
    }

    if ( readonly || !CanSurrenderOpponent( hero.GetColor() ) ) {
        btnSurrender.disable();
    }

    if ( !buttons ) {
        btnClose.disable();
    }

    btnCast.draw();
    btnRetreat.draw();
    btnSurrender.draw();
    btnClose.draw();

    int result = 0;

    display.render( pos_rt );

    // The Hero Screen is available for a Hero only (not Captain) and only when the corresponding player has a turn.
    Heroes * heroForHeroScreen = ( currentColor == hero.GetColor() ) ? dynamic_cast<Heroes *>( &hero ) : nullptr;

    std::string statusMessage = _( "Hero's Options" );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() && !result ) {
        btnCast.isEnabled() && le.isMouseLeftButtonPressedInArea( btnCast.area() ) ? btnCast.drawOnPress() : btnCast.drawOnRelease();
        btnRetreat.isEnabled() && le.isMouseLeftButtonPressedInArea( btnRetreat.area() ) ? btnRetreat.drawOnPress() : btnRetreat.drawOnRelease();
        btnSurrender.isEnabled() && le.isMouseLeftButtonPressedInArea( btnSurrender.area() ) ? btnSurrender.drawOnPress() : btnSurrender.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( btnClose.area() ) ? btnClose.drawOnPress() : btnClose.drawOnRelease();

        if ( !buttons ) {
            if ( !le.isMouseRightButtonPressed() ) {
                break;
            }

            continue;
        }

        // The Cast Spell is available for a hero and a captain.
        if ( le.isMouseCursorPosInArea( btnCast.area() ) && currentColor == hero.GetColor() ) {
            statusMessage = _( "Cast Spell" );
        }
        // The retreat is available during a player's turn only. A captain cannot retreat.
        else if ( le.isMouseCursorPosInArea( btnRetreat.area() ) && currentColor == hero.GetColor() && !hero.isCaptain() ) {
            statusMessage = _( "Retreat" );
        }
        // The surrender is available during a player's turn only. A captain cannot surrender.
        else if ( le.isMouseCursorPosInArea( btnSurrender.area() ) && currentColor == hero.GetColor() && !hero.isCaptain() ) {
            statusMessage = _( "Surrender" );
        }
        else if ( le.isMouseCursorPosInArea( btnClose.area() ) ) {
            statusMessage = _( "Cancel" );
        }
        // The Hero Screen is available for a Hero only (not Captain) and only when the corresponding player has a turn.
        else if ( le.isMouseCursorPosInArea( portraitArea ) && heroForHeroScreen != nullptr ) {
            statusMessage = _( "Hero Screen" );
        }
        else if ( hero.isCaptain() ) {
            statusMessage = _( "Captain's Options" );
        }
        else {
            statusMessage = _( "Hero's Options" );
        }

        if ( Game::HotKeyCloseWindow() || le.MouseClickLeft( btnClose.area() ) ) {
            break;
        }

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_CAST_SPELL ) || ( btnCast.isEnabled() && le.MouseClickLeft( btnCast.area() ) ) ) {
            result = 1;
        }

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_RETREAT ) || ( btnRetreat.isEnabled() && le.MouseClickLeft( btnRetreat.area() ) ) ) {
            result = 2;
        }

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::BATTLE_SURRENDER ) || ( btnSurrender.isEnabled() && le.MouseClickLeft( btnSurrender.area() ) ) ) {
            result = 3;
        }

        if ( le.MouseClickLeft( portraitArea ) && heroForHeroScreen != nullptr ) {
            LocalEvent::Get().reset();

            heroForHeroScreen->OpenDialog( true, true, true, true, false, false, fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() ) );

            // Fade-in to restore the screen after closing the hero dialog.
            fheroes2::fadeInDisplay( _interface->GetInterfaceRoi(), !display.isDefaultSize() );
        }

        if ( le.isMouseRightButtonPressedInArea( btnCast.area() ) && currentColor == hero.GetColor() ) {
            fheroes2::showStandardTextMessage(
                _( "Cast Spell" ), _( "Cast a magical spell. You may only cast one spell per combat round. The round is reset when every creature has had a turn." ),
                Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( btnRetreat.area() ) && currentColor == hero.GetColor() && !hero.isCaptain() ) {
            fheroes2::showStandardTextMessage(
                _( "Retreat" ),
                _( "Retreat your hero, abandoning your creatures. Your hero will be available for you to recruit again, however, the hero will have only a novice hero's forces." ),
                Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( btnSurrender.area() ) && currentColor == hero.GetColor() && !hero.isCaptain() ) {
            fheroes2::showStandardTextMessage(
                _( "Surrender" ),
                _( "Surrendering costs gold. However if you pay the ransom, the hero and all of his or her surviving creatures will be available to recruit again. The cost of surrender is half of the total cost of the non-temporary troops remaining in the army." ),
                Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( portraitArea ) && heroForHeroScreen != nullptr ) {
            fheroes2::showStandardTextMessage( _( "Hero Screen" ), _( "Open Hero Screen to view full information about the hero." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( btnClose.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Return to the battle." ), Dialog::ZERO );
        }

        if ( statusMessage != status.getMessage() ) {
            status.setMessage( statusMessage, false );
            status.redraw( display );
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

    // Set the cursor image. After this dialog the Game Area or the Battlefield will be shown, so it does not require a cursor restorer.
    Cursor::Get().SetThemes( Cursor::POINTER );

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

    if ( !kingdom.AllowPayment( Funds( Resource::GOLD, cost ) ) ) {
        btnAccept.disable();
    }

    if ( kingdom.GetCountMarketplace() ) {
        if ( kingdom.AllowPayment( Funds( Resource::GOLD, cost ) ) ) {
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

    const auto drawGoldMsg = [cost, &kingdom, &btnAccept]() {
        std::string str = _( "Not enough gold (%{gold})" );

        StringReplace( str, "%{gold}", cost - kingdom.GetFunds().gold );

        const fheroes2::Text text( str, fheroes2::FontType::smallWhite() );
        const fheroes2::Rect rect = btnAccept.area();

        // Since button area includes 3D effect on the left side we need to shift the text by X axis to center it in relation to the button.
        text.draw( rect.x + ( rect.width - text.width() ) / 2 + 2, rect.y - 13, fheroes2::Display::instance() );
    };

    const int icn = isEvilInterface ? ICN::SURRENDE : ICN::SURRENDR;
    const fheroes2::Sprite & window = fheroes2::AGG::GetICN( icn, 4 );
    fheroes2::Blit( window, display, pos_rt.x + 55, pos_rt.y + 32 );
    hero.PortraitRedraw( pos_rt.x + 60, pos_rt.y + 38, PORT_BIG, display );

    std::string str = hero.isCaptain() ? _( "Captain of %{name} states:" ) : _( "%{name} states:" );
    StringReplace( str, "%{name}", hero.GetName() );

    const fheroes2::Text text( str, fheroes2::FontType::normalWhite() );
    text.draw( pos_rt.x + 312 - text.width() / 2, pos_rt.y + 32, display );

    str = _( "\"I will accept your surrender and grant you and your troops safe passage for the price of %{price} gold.\"" );
    StringReplace( str, "%{price}", cost );

    fheroes2::Text box( str, fheroes2::FontType::normalWhite() );
    box.draw( pos_rt.x + 175, pos_rt.y + 52, 275, display );

    fheroes2::ImageRestorer back( display, pos_rt.x, pos_rt.y, pos_rt.width, pos_rt.height );

    if ( !kingdom.AllowPayment( Funds( Resource::GOLD, cost ) ) ) {
        drawGoldMsg();
    }

    display.render();

    bool result = false;

    while ( le.HandleEvents() && !result ) {
        if ( btnAccept.isEnabled() )
            le.isMouseLeftButtonPressedInArea( btnAccept.area() ) ? btnAccept.drawOnPress() : btnAccept.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( btnDecline.area() ) ? btnDecline.drawOnPress() : btnDecline.drawOnRelease();

        if ( btnMarket.isEnabled() )
            le.isMouseLeftButtonPressedInArea( btnMarket.area() ) ? btnMarket.drawOnPress() : btnMarket.drawOnRelease();

        if ( btnAccept.isEnabled() && ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( btnAccept.area() ) ) ) {
            result = true;
        }

        if ( btnMarket.isEnabled() && le.MouseClickLeft( btnMarket.area() ) ) {
            Dialog::Marketplace( kingdom, false );

            back.restore();

            if ( kingdom.AllowPayment( Funds( Resource::GOLD, cost ) ) ) {
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
