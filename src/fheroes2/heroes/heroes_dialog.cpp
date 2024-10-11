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

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "agg_image.h"
#include "army.h"
#include "army_bar.h"
#include "artifact.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_language.h"
#include "heroes.h" // IWYU pragma: associated
#include "heroes_base.h"
#include "heroes_indicator.h"
#include "icn.h"
#include "image.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "pal.h"
#include "race.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "skill_bar.h"
#include "statusbar.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    const fheroes2::Size primarySkillIconSize{ 82, 93 };
    const uint32_t experienceMaxValue{ 2990600 };
    const uint32_t spellPointsMaxValue{ 999 };
}

int Heroes::OpenDialog( const bool readonly, const bool fade, const bool disableDismiss, const bool disableSwitch, const bool renderBackgroundDialog, const bool isEditor,
                        const fheroes2::SupportedLanguage language )
{
    // Set the cursor image.This dialog does not require a cursor restorer. It is called from other dialogs that have the same cursor
    // or from the Game Area that will set the appropriate cursor after this dialog is closed.
    Cursor::Get().SetThemes( Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Rect dialogRoi;
    fheroes2::Rect dialogWithShadowRoi;
    std::unique_ptr<fheroes2::StandardWindow> background;
    std::unique_ptr<fheroes2::ImageRestorer> restorer;

    if ( renderBackgroundDialog ) {
        background = std::make_unique<fheroes2::StandardWindow>( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, false );
        dialogRoi = background->activeArea();
        dialogWithShadowRoi = background->totalArea();
    }
    else {
        dialogRoi = { ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2, ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2,
                      fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT };
        dialogWithShadowRoi = { dialogRoi.x - 2 * fheroes2::borderWidthPx, dialogRoi.y - fheroes2::borderWidthPx, dialogRoi.width + 3 * fheroes2::borderWidthPx,
                                dialogRoi.height + 3 * fheroes2::borderWidthPx };
        restorer = std::make_unique<fheroes2::ImageRestorer>( display, dialogRoi.x, dialogRoi.y, dialogRoi.width, dialogRoi.height );
    }

    // Fade-out game screen only for 640x480 resolution and if 'renderBackgroundDialog' is false (we are replacing image in already opened dialog).
    const bool isDefaultScreenSize = display.isDefaultSize();
    if ( fade && ( isDefaultScreenSize || !renderBackgroundDialog ) ) {
        fheroes2::fadeOutDisplay( dialogRoi, !isDefaultScreenSize );
    }

    const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( ICN::HEROBKG, 0 );
    fheroes2::Blit( backgroundImage, display, dialogRoi.x, dialogRoi.y );
    fheroes2::Blit( fheroes2::AGG::GetICN( Settings::Get().isEvilInterfaceEnabled() ? ICN::HEROEXTE : ICN::HEROEXTG, 0 ), display, dialogRoi.x, dialogRoi.y );

    // Hero portrait.
    const fheroes2::Rect portPos( dialogRoi.x + 49, dialogRoi.y + 31, 101, 93 );
    if ( isEditor && !isValidId( portrait ) ) {
        fheroes2::renderHeroRacePortrait( _race, portPos, display );
    }
    else {
        PortraitRedraw( portPos.x, portPos.y, PORT_BIG, display );
    }

    // In Editor there could be set to use "default" experience for hero. This state is transferred by setting 'experience = UINT32_MAX'.
    bool useDefaultExperience = isEditor && ( experience == UINT32_MAX );

    if ( useDefaultExperience ) {
        experience = GetStartingXp();
    }

    // Dialog title.
    const fheroes2::Rect titleRoi( dialogRoi.x + 60, dialogRoi.y + 1, 519, 17 );

    auto drawTitleText = [&display, &titleRoi, &dialogRoi, &backgroundImage, this]( const std::string & heroName, const int heroRace, const bool restoreBackground ) {
        if ( restoreBackground ) {
            fheroes2::Copy( backgroundImage, titleRoi.x - dialogRoi.x, titleRoi.y - dialogRoi.y, display, titleRoi );
        }

        std::string titleText;
        if ( !heroName.empty() ) {
            titleText = _( "%{name} the %{race} (Level %{level})" );
            StringReplace( titleText, "%{name}", heroName );
        }
        else if ( heroRace == Race::RAND ) {
            // In Editor the empty name is a sign that the default random hero (with a random name) will be used on the game start.
            titleText = _( "Random hero (Level %{level})" );
        }
        else {
            titleText = _( "Random %{race} hero (Level %{level})" );
        }
        StringReplace( titleText, "%{race}", Race::String( heroRace ) );
        StringReplace( titleText, "%{level}", GetLevel() );

        const fheroes2::Text title( std::move( titleText ), fheroes2::FontType::normalWhite() );
        title.drawInRoi( titleRoi.x + ( titleRoi.width - title.width() ) / 2, titleRoi.y + 2, display, titleRoi );
    };

    drawTitleText( name, _race, false );

    fheroes2::Point dst_pt( dialogRoi.x + 156, dialogRoi.y + 31 );

    PrimarySkillsBar primarySkillsBar( this, false, isEditor, isEditor );
    primarySkillsBar.setTableSize( { 4, 1 } );
    primarySkillsBar.setInBetweenItemsOffset( { 6, 0 } );
    primarySkillsBar.setRenderingOffset( dst_pt );

    if ( isEditor ) {
        // In Editor '-1' means that the primary skill value is reset to its default state.
        // Here we consider any negative value as a default skill value state.
        if ( attack < 0 && defense < 0 && power < 0 && knowledge < 0 ) {
            primarySkillsBar.useDefaultValues();
        }

        if ( attack < 0 ) {
            attack = Heroes::getHeroDefaultSkillValue( Skill::Primary::ATTACK, _race );
        }
        if ( defense < 0 ) {
            defense = Heroes::getHeroDefaultSkillValue( Skill::Primary::DEFENSE, _race );
        }
        if ( power < 0 ) {
            power = Heroes::getHeroDefaultSkillValue( Skill::Primary::POWER, _race );
        }
        if ( knowledge < 0 ) {
            knowledge = Heroes::getHeroDefaultSkillValue( Skill::Primary::KNOWLEDGE, _race );
        }
    }

    primarySkillsBar.Redraw( display );

    // Morale indicator.
    dst_pt.x = dialogRoi.x + 514;
    dst_pt.y = dialogRoi.y + ( isEditor ? 44 : 34 );

    MoraleIndicator moraleIndicator( this );
    moraleIndicator.SetPos( dst_pt );
    moraleIndicator.Redraw();

    // Luck indicator.
    dst_pt.x += 36;

    LuckIndicator luckIndicator( this );
    luckIndicator.SetPos( dst_pt );
    luckIndicator.Redraw();

    // Army format spread icon.
    dst_pt.x = dialogRoi.x + 516;
    dst_pt.y = dialogRoi.y + 63;
    const fheroes2::Sprite & sprite1 = fheroes2::AGG::GetICN( ICN::HSICONS, 9 );
    const fheroes2::Rect rectSpreadArmyFormat( dst_pt.x, dst_pt.y, sprite1.width(), sprite1.height() );
    const fheroes2::Point army1_pt( dst_pt.x - 1, dst_pt.y - 1 );

    // Army format grouped icon.
    dst_pt.x = dialogRoi.x + 552;
    const fheroes2::Sprite & sprite2 = fheroes2::AGG::GetICN( ICN::HSICONS, 10 );
    const fheroes2::Rect rectGroupedArmyFormat( dst_pt.x, dst_pt.y, sprite2.width(), sprite2.height() );
    const fheroes2::Point army2_pt( dst_pt.x - 1, dst_pt.y - 1 );

    // Army format cursor.
    fheroes2::MovableSprite cursorFormat( fheroes2::AGG::GetICN( ICN::HSICONS, 11 ) );
    const fheroes2::Point cursorFormatPos = army.isSpreadFormation() ? army1_pt : army2_pt;

    // Do not show Army format in Editor.
    if ( !isEditor ) {
        fheroes2::Copy( sprite1, 0, 0, display, rectSpreadArmyFormat );
        fheroes2::Copy( sprite2, 0, 0, display, rectGroupedArmyFormat );
        cursorFormat.setPosition( cursorFormatPos.x, cursorFormatPos.y );
    }

    // Experience indicator.
    dst_pt.x = dialogRoi.x + 512;
    dst_pt.y = dialogRoi.y + ( isEditor ? 76 : 86 );
    ExperienceIndicator experienceInfo( this );
    experienceInfo.SetPos( dst_pt );
    if ( isEditor ) {
        experienceInfo.setDefaultState( useDefaultExperience );
    }
    experienceInfo.Redraw();

    // Spell points indicator.
    bool useDefaultSpellPoints = isEditor && ( GetSpellPoints() == UINT32_MAX );
    if ( useDefaultSpellPoints ) {
        SetSpellPoints( GetMaxSpellPoints() );
    }

    dst_pt.x += 38;
    dst_pt.y += 2;
    SpellPointsIndicator spellPointsInfo( this );
    spellPointsInfo.SetPos( dst_pt );
    if ( isEditor ) {
        spellPointsInfo.setDefaultState( useDefaultSpellPoints );
    }
    spellPointsInfo.Redraw();

    // Color icon or race change icon for jailed hero details edit.
    const fheroes2::Rect crestRect( portPos.x, portPos.y + 99, portPos.width, portPos.height );
    fheroes2::Rect raceRect;

    auto redrawRace = [&raceRect, &crestRect, &display]( const int race ) {
        const fheroes2::Sprite & raceSprite = fheroes2::AGG::GetICN( ICN::NGEXTRA, Race::getRaceIcnIndex( race, true ) );
        fheroes2::Copy( raceSprite, 0, 0, display, raceRect );

        // Update race text background.
        const int32_t offsetY = raceRect.y - crestRect.y + raceRect.height + 9;
        const int32_t posY = crestRect.y + offsetY;
        const int32_t sizeY = crestRect.height - offsetY;
        fheroes2::ApplyPalette( fheroes2::AGG::GetICN( ICN::STRIP, 3 ), 0, offsetY, display, crestRect.x, posY, crestRect.width, sizeY,
                                PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        const fheroes2::Text raceText( Race::String( race ), fheroes2::FontType::smallWhite() );
        raceText.drawInRoi( crestRect.x, posY, crestRect.width, display, { crestRect.x, posY, crestRect.width, sizeY } );
    };

    if ( isEditor && Modes( JAIL ) ) {
        assert( GetColor() == Color::NONE );
        fheroes2::ApplyPalette( fheroes2::AGG::GetICN( ICN::STRIP, 3 ), 0, 0, display, crestRect.x, crestRect.y, crestRect.width, crestRect.height,
                                PAL::GetPalette( PAL::PaletteType::DARKENING ) );

        const fheroes2::Text raceText( _( "Hero race:" ), fheroes2::FontType::normalWhite() );
        raceText.drawInRoi( crestRect.x, crestRect.y + 6, crestRect.width, display, crestRect );

        const fheroes2::Sprite & raceSprite = fheroes2::AGG::GetICN( ICN::NGEXTRA, Race::getRaceIcnIndex( _race, true ) );
        raceRect.width = raceSprite.width();
        raceRect.height = raceSprite.height();
        raceRect.x = crestRect.x + ( crestRect.width - raceRect.width ) / 2;
        raceRect.y = crestRect.y + ( crestRect.height - raceRect.height ) / 2;

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::NGEXTRA, 61U ), display, raceRect.x - 5, raceRect.y + 3 );

        redrawRace( _race );
    }
    else {
        // Color "crest" icon.
        fheroes2::Copy( fheroes2::AGG::GetICN( ICN::CREST,
                                               Color::NONE == GetColor() ? Color::GetIndex( Settings::Get().CurrentColor() ) : Color::GetIndex( GetColor() ) ),
                        0, 0, display, crestRect );
    }

    // Hero's army.
    dst_pt.x = dialogRoi.x + 156;
    dst_pt.y = dialogRoi.y + 130;

    // In Editor mode we allow to edit army and remove all customized troops from the army.
    ArmyBar selectArmy( &army, false, readonly, isEditor, !isEditor );
    selectArmy.setTableSize( { 5, 1 } );
    selectArmy.setRenderingOffset( dst_pt );
    selectArmy.setInBetweenItemsOffset( { 6, 0 } );
    selectArmy.Redraw( display );

    // Hero's secondary skills.
    SecondarySkillsBar secskill_bar( *this, false, isEditor, isEditor );
    secskill_bar.setTableSize( { 8, 1 } );
    secskill_bar.setInBetweenItemsOffset( { 5, 0 } );
    secskill_bar.SetContent( secondary_skills.ToVector() );
    secskill_bar.setRenderingOffset( { dialogRoi.x + 3, dialogRoi.y + 233 } );
    secskill_bar.Redraw( display );

    // Status bar.
    dst_pt.x = dialogRoi.x + 22;
    dst_pt.y = dialogRoi.y + 460;
    const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( ICN::HSBTNS, 8 );
    fheroes2::Copy( bar, 0, 0, display, dst_pt.x, dst_pt.y, bar.width(), bar.height() );

    StatusBar statusBar;
    // Status bar must be smaller due to extra art on both sides.
    statusBar.setRoi( { dst_pt.x + 16, dst_pt.y + 3, bar.width() - 16 * 2, 0 } );

    // Artifacts bar.
    dst_pt.x = dialogRoi.x + 51;
    dst_pt.y = dialogRoi.y + 308;

    ArtifactsBar selectArtifacts( this, false, readonly, isEditor, true, &statusBar );
    selectArtifacts.setTableSize( { 7, 2 } );
    selectArtifacts.setInBetweenItemsOffset( { 15, 15 } );
    selectArtifacts.SetContent( GetBagArtifacts() );
    selectArtifacts.setRenderingOffset( dst_pt );
    selectArtifacts.Redraw( display );

    // Previous hero button.
    dst_pt.x = dialogRoi.x;
    dst_pt.y = dialogRoi.y + fheroes2::Display::DEFAULT_HEIGHT - 20;
    fheroes2::Button buttonPrevHero( dst_pt.x, dst_pt.y, ICN::HSBTNS, 4, 5 );
    fheroes2::TimedEventValidator timedButtonPrevHero( [&buttonPrevHero]() { return buttonPrevHero.isPressed(); } );
    buttonPrevHero.subscribe( &timedButtonPrevHero );

    // Next hero button.
    dst_pt.x += fheroes2::Display::DEFAULT_WIDTH - 22;
    fheroes2::Button buttonNextHero( dst_pt.x, dst_pt.y, ICN::HSBTNS, 6, 7 );
    fheroes2::TimedEventValidator timedButtonNextHero( [&buttonNextHero]() { return buttonNextHero.isPressed(); } );
    buttonNextHero.subscribe( &timedButtonNextHero );

    // Hero dismiss button.
    dst_pt.x = dialogRoi.x + 9;
    dst_pt.y = dialogRoi.y + 378;
    const fheroes2::Sprite & dismissReleased = fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_DISMISS, 0 );
    fheroes2::ButtonSprite buttonDismiss( dst_pt.x, dst_pt.y - dismissReleased.height() / 2, dismissReleased, fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_DISMISS, 1 ),
                                          fheroes2::AGG::GetICN( ICN::DISMISS_HERO_DISABLED_BUTTON, 0 ) );

    if ( inCastle() || readonly || disableDismiss || Modes( NOTDISMISS ) ) {
        buttonDismiss.disable();
    }

    if ( isEditor || readonly || disableDismiss ) {
        buttonDismiss.hide();
    }
    else {
        fheroes2::addGradientShadow( dismissReleased, display, { dst_pt.x, dst_pt.y - dismissReleased.height() / 2 }, { -3, 5 } );
    }

    // Hero Patrol mode button (used in Editor).
    const fheroes2::Sprite & patrolReleased = fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_PATROL, 0 );
    fheroes2::ButtonSprite buttonPatrol( dst_pt.x, dst_pt.y - patrolReleased.height() / 2, patrolReleased, fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_PATROL, 1 ) );
    if ( isEditor ) {
        fheroes2::addGradientShadow( patrolReleased, display, { dialogRoi.x + 9, dst_pt.y - patrolReleased.height() / 2 }, { -3, 5 } );
        if ( Modes( PATROL ) ) {
            buttonPatrol.press();
        }
    }
    else {
        buttonPatrol.hide();
    }

    // Exit button.
    dst_pt.x = dialogRoi.x + 602;
    const fheroes2::Sprite & exitReleased = fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_EXIT, 0 );
    fheroes2::ButtonSprite buttonExit( dst_pt.x, dst_pt.y - exitReleased.height() / 2, exitReleased, fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_EXIT, 1 ) );

    LocalEvent & le = LocalEvent::Get();

    if ( readonly || disableSwitch || 2 > GetKingdom().GetHeroes().size() ) {
        buttonNextHero.disable();
        buttonPrevHero.disable();
    }

    buttonPrevHero.draw();
    buttonNextHero.draw();
    buttonDismiss.draw();
    buttonPatrol.draw();
    buttonExit.draw();

    // Fade-in hero dialog.
    if ( fade ) {
        if ( renderBackgroundDialog && !isDefaultScreenSize ) {
            // We need to expand the ROI for the next render to properly render window borders and shadow.
            display.updateNextRenderRoi( dialogWithShadowRoi );
        }

        // Use half fade if game resolution is not 640x480.
        fheroes2::fadeInDisplay( dialogRoi, !isDefaultScreenSize );
    }
    else {
        display.render( dialogWithShadowRoi );
    }

    bool needRedraw{ false };
    std::string message;

    // dialog menu loop
    while ( le.HandleEvents() ) {
        // Exit this dialog.
        le.isMouseLeftButtonPressedInArea( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            // Exit the dialog handling loop to close it.
            break;
        }

        // Manage hero's army.
        if ( le.isMouseCursorPosInArea( selectArmy.GetArea() ) && selectArmy.QueueEventProcessing( &message ) ) {
            if ( selectArtifacts.isSelected() ) {
                selectArtifacts.ResetSelected();
            }
            selectArmy.Redraw( display );
            moraleIndicator.Redraw();
            luckIndicator.Redraw();

            needRedraw = true;
        }

        // Manage hero's artifacts.
        else if ( le.isMouseCursorPosInArea( selectArtifacts.GetArea() ) && selectArtifacts.QueueEventProcessing( &message ) ) {
            if ( selectArmy.isSelected() ) {
                selectArmy.ResetSelected();
            }
            selectArtifacts.Redraw( display );

            if ( isEditor ) {
                // Artifacts affect many hero stats.
                if ( useDefaultSpellPoints ) {
                    SetSpellPoints( GetMaxSpellPoints() );
                }

                spellPointsInfo.Redraw();
                moraleIndicator.Redraw();
                luckIndicator.Redraw();
                primarySkillsBar.Redraw( display );
            }

            needRedraw = true;
        }

        // Dismiss hero.
        else if ( buttonDismiss.isEnabled() && buttonDismiss.isVisible() ) {
            if ( le.isMouseLeftButtonPressedInArea( buttonDismiss.area() ) || HotKeyPressEvent( Game::HotKeyEvent::ARMY_DISMISS ) ) {
                buttonDismiss.drawOnPress();
            }
            else {
                buttonDismiss.drawOnRelease();
            }

            if ( ( le.MouseClickLeft( buttonDismiss.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::ARMY_DISMISS ) )
                 && Dialog::YES == fheroes2::showStandardTextMessage( GetName(), _( "Are you sure you want to dismiss this Hero?" ), Dialog::YES | Dialog::NO ) ) {
                // Fade-out hero dialog.
                fheroes2::fadeOutDisplay( dialogRoi, !isDefaultScreenSize );

                return Dialog::DISMISS;
            }
        }

        // Previous hero.
        if ( buttonPrevHero.isEnabled() ) {
            le.isMouseLeftButtonPressedInArea( buttonPrevHero.area() ) ? buttonPrevHero.drawOnPress() : buttonPrevHero.drawOnRelease();
            if ( le.MouseClickLeft( buttonPrevHero.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_LEFT ) || timedButtonPrevHero.isDelayPassed() ) {
                return Dialog::PREV;
            }
        }

        // Next hero.
        if ( buttonNextHero.isEnabled() ) {
            le.isMouseLeftButtonPressedInArea( buttonNextHero.area() ) ? buttonNextHero.drawOnPress() : buttonNextHero.drawOnRelease();
            if ( le.MouseClickLeft( buttonNextHero.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) || timedButtonNextHero.isDelayPassed() ) {
                return Dialog::NEXT;
            }
        }

        if ( le.isMouseCursorPosInArea( moraleIndicator.GetArea() ) ) {
            MoraleIndicator::QueueEventProcessing( moraleIndicator );
            message = fheroes2::MoraleString( army.GetMorale() );
        }
        else if ( le.isMouseCursorPosInArea( luckIndicator.GetArea() ) ) {
            LuckIndicator::QueueEventProcessing( luckIndicator );
            message = fheroes2::LuckString( army.GetLuck() );
        }
        else if ( le.isMouseCursorPosInArea( experienceInfo.GetArea() ) ) {
            if ( isEditor ) {
                message = useDefaultExperience ? _( "Set custom Experience value. Current value is default." )
                                               : _( "Change Experience value. Right-click to reset to default value." );

                if ( le.MouseClickLeft() ) {
                    int32_t value = static_cast<int32_t>( experience );
                    if ( Dialog::SelectCount( _( "Set Experience value" ), 0, experienceMaxValue, value ) ) {
                        useDefaultExperience = false;
                        experience = static_cast<uint32_t>( value );
                        experienceInfo.setDefaultState( useDefaultExperience );
                        experienceInfo.Redraw();
                        drawTitleText( name, _race, true );
                        needRedraw = true;
                    }
                }
                else if ( le.MouseClickRight() ) {
                    useDefaultExperience = true;
                    experience = GetStartingXp();
                    experienceInfo.setDefaultState( useDefaultExperience );
                    experienceInfo.Redraw();
                    drawTitleText( name, _race, true );
                    needRedraw = true;
                }
            }
            else {
                message = _( "View Experience Info" );
                experienceInfo.QueueEventProcessing();
            }
        }
        else if ( le.isMouseCursorPosInArea( spellPointsInfo.GetArea() ) ) {
            if ( isEditor ) {
                message = useDefaultSpellPoints ? _( "Set custom Spell Points value. Current value is default." )
                                                : _( "Change Spell Points value. Right-click to reset to default value." );

                if ( le.MouseClickLeft() ) {
                    int32_t value = static_cast<int32_t>( GetSpellPoints() );
                    if ( Dialog::SelectCount( _( "Set Spell Points value" ), 0, spellPointsMaxValue, value ) ) {
                        useDefaultSpellPoints = false;
                        SetSpellPoints( static_cast<uint32_t>( value ) );
                        spellPointsInfo.setDefaultState( useDefaultSpellPoints );
                        spellPointsInfo.Redraw();
                        needRedraw = true;
                    }
                }
                else if ( le.MouseClickRight() ) {
                    // Reset spell points modification.
                    useDefaultSpellPoints = true;
                    SetSpellPoints( GetMaxSpellPoints() );
                    spellPointsInfo.setDefaultState( useDefaultSpellPoints );
                    spellPointsInfo.Redraw();
                    needRedraw = true;
                }
            }
            else {
                message = _( "View Spell Points Info" );
                spellPointsInfo.QueueEventProcessing();
            }
        }
        else if ( !readonly && !isEditor && le.MouseClickLeft( rectSpreadArmyFormat ) && !army.isSpreadFormation() ) {
            cursorFormat.setPosition( army1_pt.x, army1_pt.y );
            needRedraw = true;
            army.SetSpreadFormation( true );
        }
        else if ( !readonly && !isEditor && le.MouseClickLeft( rectGroupedArmyFormat ) && army.isSpreadFormation() ) {
            cursorFormat.setPosition( army2_pt.x, army2_pt.y );
            needRedraw = true;
            army.SetSpreadFormation( false );
        }
        else if ( le.isMouseCursorPosInArea( secskill_bar.GetArea() ) && secskill_bar.QueueEventProcessing( &message ) ) {
            if ( isEditor ) {
                // The change of secondary skills affects many hero stats.
                secskill_bar.Redraw( display );
                moraleIndicator.Redraw();
                luckIndicator.Redraw();
                needRedraw = true;
            }
        }
        else if ( le.isMouseCursorPosInArea( primarySkillsBar.GetArea() ) && primarySkillsBar.QueueEventProcessing( &message ) ) {
            if ( isEditor ) {
                primarySkillsBar.Redraw( display );

                // Hero's Knowledge may have changed.
                if ( useDefaultSpellPoints ) {
                    SetSpellPoints( GetMaxSpellPoints() );
                }

                spellPointsInfo.Redraw();
                needRedraw = true;
            }
        }
        else if ( isEditor && le.MouseClickLeft( portPos ) ) {
            const int newPortrait = Dialog::selectHeroes( portrait );
            if ( newPortrait != Heroes::UNKNOWN ) {
                portrait = newPortrait;
                PortraitRedraw( portPos.x, portPos.y, PORT_BIG, display );
                needRedraw = true;
            }
        }
        else if ( le.isMouseRightButtonPressedInArea( portPos ) ) {
            if ( isEditor ) {
                portrait = 0;
                fheroes2::renderHeroRacePortrait( _race, portPos, display );
                needRedraw = true;
            }
            else {
                Dialog::QuickInfo( *this, true );
            }
        }
        else if ( buttonPatrol.isVisible() && le.isMouseCursorPosInArea( buttonPatrol.area() ) ) {
            if ( le.isMouseLeftButtonPressed() && buttonPatrol.isReleased() && !Modes( PATROL ) ) {
                buttonPatrol.drawOnPress();
                int32_t value = static_cast<int32_t>( _patrolDistance );
                if ( Dialog::SelectCount( _( "Set patrol radius in tiles" ), 0, 255, value ) ) {
                    SetModes( PATROL );
                    _patrolDistance = static_cast<uint32_t>( value );
                }
                else {
                    buttonPatrol.drawOnRelease();
                    ResetModes( PATROL );
                }
            }
            else if ( le.MouseClickLeft() && buttonPatrol.isPressed() && Modes( PATROL ) ) {
                ResetModes( PATROL );
            }
            if ( !Modes( PATROL ) ) {
                buttonPatrol.drawOnRelease();
            }
        }
        else if ( isEditor ) {
            if ( le.MouseClickLeft( titleRoi ) ) {
                std::string res = name;

                // TODO: add support for languages. As of now we do not support any other language except English.
                (void)language;

                const fheroes2::Text body{ _( "Enter hero's name" ), fheroes2::FontType::normalWhite() };
                if ( Dialog::inputString( fheroes2::Text{}, body, res, 30, false, fheroes2::SupportedLanguage::English ) && !res.empty() ) {
                    name = std::move( res );
                    drawTitleText( name, _race, true );
                    needRedraw = true;
                }
            }
            else if ( le.MouseClickRight( titleRoi ) ) {
                name.clear();
                drawTitleText( name, _race, true );
                needRedraw = true;
            }
            else if ( le.isMouseCursorPosInArea( raceRect ) ) {
                assert( !needRedraw );
                message = _( "Click to change race." );
                if ( le.MouseClickLeft() || le.isMouseWheelDown() ) {
                    _race = Race::getNextRace( _race );
                    needRedraw = true;
                }
                else if ( le.MouseClickRight() || le.isMouseWheelUp() ) {
                    _race = Race::getPreviousRace( _race );
                    needRedraw = true;
                }

                if ( needRedraw ) {
                    drawTitleText( name, _race, true );
                    redrawRace( _race );

                    if ( portrait == 0 ) {
                        fheroes2::renderHeroRacePortrait( _race, portPos, display );
                    }

                    if ( primarySkillsBar.isDefaultValues() ) {
                        attack = getHeroDefaultSkillValue( Skill::Primary::ATTACK, _race );
                        defense = getHeroDefaultSkillValue( Skill::Primary::DEFENSE, _race );
                        power = getHeroDefaultSkillValue( Skill::Primary::POWER, _race );
                        knowledge = getHeroDefaultSkillValue( Skill::Primary::KNOWLEDGE, _race );
                        primarySkillsBar.Redraw( display );

                        if ( useDefaultSpellPoints ) {
                            SetSpellPoints( GetMaxSpellPoints() );
                        }

                        spellPointsInfo.Redraw();
                    }
                }
            }
        }
        else {
            // If dialog is opened not in Editor.
            if ( le.isMouseRightButtonPressedInArea( rectSpreadArmyFormat ) ) {
                fheroes2::showStandardTextMessage(
                    _( "Spread Formation" ),
                    _( "'Spread' combat formation spreads the hero's units from the top to the bottom of the battlefield, with at least one empty space between each unit." ),
                    Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( rectGroupedArmyFormat ) ) {
                fheroes2::showStandardTextMessage( _( "Grouped Formation" ),
                                                   _( "'Grouped' combat formation bunches the hero's army together in the center of their side of the battlefield." ),
                                                   Dialog::ZERO );
            }
        }

        // Status messages.
        if ( le.isMouseCursorPosInArea( buttonExit.area() ) ) {
            message = _( "Exit Hero Screen" );
        }
        else if ( buttonDismiss.isVisible() && le.isMouseCursorPosInArea( buttonDismiss.area() ) ) {
            if ( inCastle() ) {
                message = _( "You cannot dismiss a hero in a castle" );
            }
            else if ( Modes( NOTDISMISS ) ) {
                message = _( "Dismissal of %{name} the %{race} is prohibited by scenario" );
                StringReplace( message, "%{name}", name );
                StringReplace( message, "%{race}", Race::String( _race ) );
            }
            else if ( buttonDismiss.isEnabled() ) {
                message = _( "Dismiss %{name} the %{race}" );
                StringReplace( message, "%{name}", name );
                StringReplace( message, "%{race}", Race::String( _race ) );
            }
        }
        else if ( buttonPrevHero.isEnabled() && le.isMouseCursorPosInArea( buttonPrevHero.area() ) ) {
            message = _( "Show previous hero" );
        }
        else if ( buttonNextHero.isEnabled() && le.isMouseCursorPosInArea( buttonNextHero.area() ) ) {
            message = _( "Show next hero" );
        }
        else if ( !isEditor ) {
            // These messages are not shown in the Editor mode.
            if ( le.isMouseCursorPosInArea( rectSpreadArmyFormat ) ) {
                message = _( "Set army combat formation to 'Spread'" );
            }
            else if ( le.isMouseCursorPosInArea( rectGroupedArmyFormat ) ) {
                message = _( "Set army combat formation to 'Grouped'" );
            }
        }
        else if ( message.empty() ) {
            // Editor related status messages.
            if ( le.isMouseCursorPosInArea( selectArtifacts.GetArea() ) ) {
                message = _( "Set hero's Artifacts. Right-click to reset Artifact." );
            }
            else if ( le.isMouseCursorPosInArea( secskill_bar.GetArea() ) ) {
                message = _( "Set hero's Secondary Skills. Right-click to reset skill." );
            }
            else if ( le.isMouseCursorPosInArea( selectArmy.GetArea() ) ) {
                message = _( "Set hero's Army. Right-click to reset unit." );
            }
            else if ( le.isMouseCursorPosInArea( portPos ) ) {
                message = _( "Set hero's portrait. Right-click to reset to default." );
            }
            else if ( le.isMouseCursorPosInArea( titleRoi ) ) {
                message = _( "Click to change hero's name. Right-click to reset to default." );
            }
            else if ( le.isMouseCursorPosInArea( buttonPatrol.area() ) ) {
                if ( buttonPatrol.isPressed() ) {
                    message = _( "Hero is in patrol mode in %{tiles} tiles radius. Click to disable it." );
                    StringReplace( message, "%{tiles}", _patrolDistance );
                }
                else {
                    message = _( "Click to enable hero's patrol mode." );
                }
            }
        }

        if ( message.empty() ) {
            statusBar.ShowMessage( _( "Hero Screen" ) );
        }
        else {
            statusBar.ShowMessage( std::move( message ) );
            message.clear();
        }

        if ( needRedraw ) {
            needRedraw = false;
            display.render( dialogRoi );
        }
    }

    // Disable fast scroll for resolutions where the exit button is directly above the border.
    Interface::AdventureMap::Get().getGameArea().setFastScrollStatus( false );

    buttonExit.drawOnPress();

    // Fade-out hero dialog.
    fheroes2::fadeOutDisplay( dialogRoi, !isDefaultScreenSize );

    if ( isEditor ) {
        if ( useDefaultExperience ) {
            // Tell Editor that default experience value is set.
            experience = UINT32_MAX;
        }

        if ( useDefaultSpellPoints ) {
            // Tell Editor that default Spell Points value is set.
            SetSpellPoints( UINT32_MAX );
        }

        // For Editor '-1' means that the primary skill is reset to its default state.
        if ( primarySkillsBar.isDefaultValues() ) {
            attack = -1;
            defense = -1;
            power = -1;
            knowledge = -1;
        }
    }

    return Dialog::CANCEL;
}
