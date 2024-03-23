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

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
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
#include "gamedefs.h"
#include "heroes.h"
#include "heroes_base.h"
#include "heroes_indicator.h"
#include "icn.h"
#include "image.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "race.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "skill_bar.h"
#include "statusbar.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    const uint32_t primaryMaxValue{ 20 };
    const fheroes2::Size primarySkillIconSize{ 82, 93 };
    const uint32_t experienceMaxValue{ 2990600 };
    const uint32_t spellPointsMaxValue{ 999 };

    void renderRacePortrait( const int race, const fheroes2::Rect & portPos, fheroes2::Image & output )
    {
        fheroes2::Image racePortrait( portPos.width, portPos.height );

        auto preparePortrait = [&racePortrait, &portPos]( const int icnId, const int bkgIndex ) {
            fheroes2::SubpixelResize( fheroes2::AGG::GetICN( ICN::STRIP, bkgIndex ), racePortrait );
            const fheroes2::Sprite & heroSprite = fheroes2::AGG::GetICN( icnId, 1 );
            fheroes2::Blit( heroSprite, 0, std::max( 0, heroSprite.height() - portPos.height ), racePortrait, ( portPos.width - heroSprite.width() ) / 2,
                            std::max( 0, portPos.height - heroSprite.height() ), heroSprite.width(), portPos.height );
        };

        switch ( race ) {
        case Race::KNGT:
            preparePortrait( ICN::CMBTHROK, 4 );
            break;
        case Race::BARB:
            preparePortrait( ICN::CMBTHROB, 5 );
            break;
        case Race::SORC:
            preparePortrait( ICN::CMBTHROS, 6 );
            break;
        case Race::WRLK:
            preparePortrait( ICN::CMBTHROW, 7 );
            break;
        case Race::WZRD:
            preparePortrait( ICN::CMBTHROZ, 8 );
            break;
        case Race::NECR:
            preparePortrait( ICN::CMBTHRON, 9 );
            break;
        case Race::RAND:
            // TODO: Make a portrait for the random hero.
            preparePortrait( ICN::NOMAD, 10 );
            break;
        default:
            // Have you added a new race? Correct the logic above!
            assert( 0 );
        }
        fheroes2::Copy( racePortrait, 0, 0, output, portPos );
    }
}

int Heroes::OpenDialog( const bool readonly, const bool fade, const bool disableDismiss, const bool disableSwitch, const bool renderBackgroundDialog,
                        const bool isEditor )
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
        dialogWithShadowRoi = { dialogRoi.x - 2 * BORDERWIDTH, dialogRoi.y - BORDERWIDTH, dialogRoi.width + 3 * BORDERWIDTH, dialogRoi.height + 3 * BORDERWIDTH };
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
        renderRacePortrait( _race, portPos, display );
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
        std::string titleText = _( "%{name} the %{race} (Level %{level})" );
        StringReplace( titleText, "%{name}", heroName );
        StringReplace( titleText, "%{race}", Race::String( heroRace ) );
        StringReplace( titleText, "%{level}", GetLevel() );

        const fheroes2::Text title( std::move( titleText ), fheroes2::FontType::normalWhite() );
        title.drawInRoi( titleRoi.x + ( titleRoi.width - title.width() ) / 2, titleRoi.y + 2, display, titleRoi );
    };

    drawTitleText( name, _race, false );

    fheroes2::Point dst_pt( dialogRoi.x + 156, dialogRoi.y + 31 );

    PrimarySkillsBar primskill_bar( this, false );
    primskill_bar.setTableSize( { 4, 1 } );
    primskill_bar.setInBetweenItemsOffset( { 6, 0 } );
    primskill_bar.setRenderingOffset( dst_pt );
    primskill_bar.Redraw( display );

    const fheroes2::Rect attackRoi( dst_pt.x, dst_pt.y, primarySkillIconSize.width, primarySkillIconSize.height );
    const fheroes2::Rect defenseRoi( dst_pt.x + primarySkillIconSize.width + 6, dst_pt.y, primarySkillIconSize.width, primarySkillIconSize.height );
    const fheroes2::Rect powerRoi( dst_pt.x + 2 * ( primarySkillIconSize.width + 6 ), dst_pt.y, primarySkillIconSize.width, primarySkillIconSize.height );
    const fheroes2::Rect knowledgeRoi( dst_pt.x + 3 * ( primarySkillIconSize.width + 6 ), dst_pt.y, primarySkillIconSize.width, primarySkillIconSize.height );

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
    experienceInfo.Redraw();

    // Spell points indicator.
    dst_pt.x += 38;
    dst_pt.y += 2;
    SpellPointsIndicator spellPointsInfo( this );
    spellPointsInfo.SetPos( dst_pt );
    spellPointsInfo.Redraw();

    // Color "crest" icon.
    dst_pt.x = dialogRoi.x + 49;
    dst_pt.y = dialogRoi.y + 130;

    fheroes2::Copy( fheroes2::AGG::GetICN( ICN::CREST, Color::NONE == GetColor() ? Color::GetIndex( Settings::Get().CurrentColor() ) : Color::GetIndex( GetColor() ) ), 0,
                    0, display, dst_pt.x, dst_pt.y, portPos.width, portPos.height );

    // Hero's army.
    dst_pt.x = dialogRoi.x + 156;

    // In Editor mode we allow to edit army and remove all customized troops from the army.
    ArmyBar selectArmy( &army, false, readonly, isEditor, !isEditor );
    selectArmy.setTableSize( { 5, 1 } );
    selectArmy.setRenderingOffset( dst_pt );
    selectArmy.setInBetweenItemsOffset( { 6, 0 } );
    selectArmy.Redraw( display );

    // Hero's secondary skills.
    SecondarySkillsBar secskill_bar( *this, false, isEditor );
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
    dst_pt.y = dialogRoi.y + 318;
    fheroes2::ButtonSprite buttonDismiss( dst_pt.x, dst_pt.y, fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_DISMISS, 0 ),
                                          fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_DISMISS, 1 ), fheroes2::AGG::GetICN( ICN::DISMISS_HERO_DISABLED_BUTTON, 0 ) );

    if ( inCastle() || readonly || disableDismiss || Modes( NOTDISMISS ) ) {
        buttonDismiss.disable();
    }

    if ( isEditor || readonly || disableDismiss ) {
        buttonDismiss.hide();
    }
    else {
        fheroes2::addGradientShadow( fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_DISMISS, 0 ), display, dst_pt, { -3, 5 } );
    }

    // Hero Patrol mode button (used in Editor).
    fheroes2::ButtonSprite buttonPatrol( dst_pt.x, dst_pt.y, fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_PATROL, 0 ),
                                         fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_PATROL, 1 ) );
    if ( isEditor ) {
        fheroes2::addGradientShadow( fheroes2::AGG::GetICN( ICN::BUTTON_VERTICAL_PATROL, 0 ), display, { dialogRoi.x + 9, dialogRoi.y + 318 }, { -3, 5 } );
        if ( Modes( PATROL ) ) {
            buttonPatrol.press();
        }
    }
    else {
        buttonPatrol.hide();
    }

    // Exit button.
    dst_pt.x = dialogRoi.x + 602;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::BUTTON_VERTICAL_EXIT, 0, 1 );

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
    int32_t extraSpellPoints{ 0 };
    std::string message;

    // dialog menu loop
    while ( le.HandleEvents() ) {
        // Exit this dialog.
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            // Disable fast scroll for resolutions where the exit button is directly above the border.
            Interface::AdventureMap::Get().getGameArea().setFastScrollStatus( false );

            buttonExit.drawOnPress();

            // Fade-out hero dialog.
            fheroes2::fadeOutDisplay( dialogRoi, !isDefaultScreenSize );

            if ( isEditor && useDefaultExperience ) {
                // Tell Editor that default experience value is set.
                experience = UINT32_MAX;
            }

            return Dialog::CANCEL;
        }

        // Manage hero's army.
        if ( le.MouseCursor( selectArmy.GetArea() ) && selectArmy.QueueEventProcessing( &message ) ) {
            if ( selectArtifacts.isSelected() ) {
                selectArtifacts.ResetSelected();
            }
            selectArmy.Redraw( display );
            moraleIndicator.Redraw();
            luckIndicator.Redraw();

            needRedraw = true;
        }

        // Manage hero's artifacts.
        else if ( le.MouseCursor( selectArtifacts.GetArea() ) && selectArtifacts.QueueEventProcessing( &message ) ) {
            if ( selectArmy.isSelected() ) {
                selectArmy.ResetSelected();
            }
            selectArtifacts.Redraw( display );

            if ( isEditor ) {
                // Artifacts affect many hero stats.
                SetSpellPoints( static_cast<uint32_t>( std::max( 0, static_cast<int32_t>( GetMaxSpellPoints() ) + extraSpellPoints ) ) );
                spellPointsInfo.Redraw();
                moraleIndicator.Redraw();
                luckIndicator.Redraw();
                primskill_bar.Redraw( display );
            }

            needRedraw = true;
        }

        // Dismiss hero.
        else if ( buttonDismiss.isEnabled() && buttonDismiss.isVisible() ) {
            if ( le.MousePressLeft( buttonDismiss.area() ) || HotKeyPressEvent( Game::HotKeyEvent::ARMY_DISMISS ) ) {
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
            le.MousePressLeft( buttonPrevHero.area() ) ? buttonPrevHero.drawOnPress() : buttonPrevHero.drawOnRelease();
            if ( le.MouseClickLeft( buttonPrevHero.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_LEFT ) || timedButtonPrevHero.isDelayPassed() ) {
                return Dialog::PREV;
            }
        }

        // Next hero.
        if ( buttonNextHero.isEnabled() ) {
            le.MousePressLeft( buttonNextHero.area() ) ? buttonNextHero.drawOnPress() : buttonNextHero.drawOnRelease();
            if ( le.MouseClickLeft( buttonNextHero.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) || timedButtonNextHero.isDelayPassed() ) {
                return Dialog::NEXT;
            }
        }

        if ( le.MouseCursor( moraleIndicator.GetArea() ) ) {
            MoraleIndicator::QueueEventProcessing( moraleIndicator );
            message = fheroes2::MoraleString( army.GetMorale() );
        }
        else if ( le.MouseCursor( luckIndicator.GetArea() ) ) {
            LuckIndicator::QueueEventProcessing( luckIndicator );
            message = fheroes2::LuckString( army.GetLuck() );
        }
        else if ( le.MouseCursor( experienceInfo.GetArea() ) ) {
            if ( isEditor ) {
                message = useDefaultExperience ? _( "Set custom Experience value. Current value is default." )
                                               : _( "Change Experience value. Right-click to reset to default value." );

                if ( le.MouseClickLeft() ) {
                    uint32_t value = experience;
                    if ( Dialog::SelectCount( _( "Set Experience value" ), 0, experienceMaxValue, value ) ) {
                        useDefaultExperience = false;
                        experience = value;
                        experienceInfo.Redraw();
                        drawTitleText( name, _race, true );
                        needRedraw = true;
                    }
                }
                else if ( le.MouseClickRight() ) {
                    useDefaultExperience = true;
                    experience = GetStartingXp();
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
        else if ( le.MouseCursor( spellPointsInfo.GetArea() ) ) {
            if ( isEditor ) {
                message = _( "Set Spell Points value" );

                if ( le.MouseClickLeft() ) {
                    uint32_t value = GetSpellPoints();
                    if ( Dialog::SelectCount( message, 0, spellPointsMaxValue, value ) ) {
                        extraSpellPoints = static_cast<int32_t>( value ) - static_cast<int32_t>( GetMaxSpellPoints() );
                        SetSpellPoints( value );
                        spellPointsInfo.Redraw();
                        needRedraw = true;
                    }
                }
                else if ( le.MouseClickRight() ) {
                    // Reset spell points modification.
                    extraSpellPoints = 0;
                    SetSpellPoints( GetMaxSpellPoints() );
                    spellPointsInfo.Redraw();
                    needRedraw = true;
                }
            }
            else {
                message = _( "View Spell Points Info" );
                spellPointsInfo.QueueEventProcessing();
            }
        }
        else if ( isEditor ) {
            if ( le.MouseClickLeft( titleRoi ) ) {
                std::string res = name;
                if ( Dialog::InputString( _( "Enter hero's name" ), res, {}, 30 ) && !res.empty() ) {
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
        }

        // left click info
        if ( !readonly && !isEditor && le.MouseClickLeft( rectSpreadArmyFormat ) && !army.isSpreadFormation() ) {
            cursorFormat.setPosition( army1_pt.x, army1_pt.y );
            needRedraw = true;
            army.SetSpreadFormation( true );
        }
        else if ( !readonly && !isEditor && le.MouseClickLeft( rectGroupedArmyFormat ) && army.isSpreadFormation() ) {
            cursorFormat.setPosition( army2_pt.x, army2_pt.y );
            needRedraw = true;
            army.SetSpreadFormation( false );
        }
        else if ( le.MouseCursor( secskill_bar.GetArea() ) && secskill_bar.QueueEventProcessing( &message ) ) {
            if ( isEditor ) {
                // The change of secondary skills affects many hero stats.
                secskill_bar.Redraw( display );
                moraleIndicator.Redraw();
                luckIndicator.Redraw();
                needRedraw = true;
            }
        }
        else if ( le.MouseCursor( primskill_bar.GetArea() ) ) {
            if ( isEditor ) {
                // In Editor we need to override Primary Skills Bar's event processing except the right mouse button press.

                auto primarySkillEditHandler = [&le, &primskill_bar, &display]( int & skill, const std::string & text ) {
                    if ( le.MouseClickLeft() ) {
                        uint32_t value = skill;
                        if ( Dialog::SelectCount( text, 0, primaryMaxValue, value ) ) {
                            skill = static_cast<int>( value );
                            primskill_bar.Redraw( display );
                            return true;
                        }
                    }
                    return false;
                };

                if ( le.MouseCursor( attackRoi ) ) {
                    message = _( "Set Attack Skill" );
                    needRedraw |= primarySkillEditHandler( attack, message );
                }
                else if ( le.MouseCursor( defenseRoi ) ) {
                    message = _( "Set Defense Skill" );
                    needRedraw |= primarySkillEditHandler( defense, message );
                }
                else if ( le.MouseCursor( powerRoi ) ) {
                    message = _( "Set Power Skill" );
                    needRedraw |= primarySkillEditHandler( power, message );
                }
                else if ( le.MouseCursor( knowledgeRoi ) ) {
                    message = _( "Set Knowledge Skill" );
                    if ( primarySkillEditHandler( knowledge, message ) ) {
                        // Knowledge change affects hero's spell points.
                        SetSpellPoints( static_cast<uint32_t>( std::max( 0, static_cast<int32_t>( GetMaxSpellPoints() ) + extraSpellPoints ) ) );
                        spellPointsInfo.Redraw();
                        needRedraw = true;
                    }
                }

                if ( le.MousePressRight() ) {
                    // In Editor we need to suppress standard Primary Skills Bar's messages.
                    primskill_bar.QueueEventProcessing();
                }
            }
            else {
                primskill_bar.QueueEventProcessing( &message );
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
        else if ( le.MousePressRight( portPos ) ) {
            if ( isEditor ) {
                portrait = 0;
                renderRacePortrait( _race, portPos, display );
                needRedraw = true;
            }
            else {
                Dialog::QuickInfo( *this, true );
            }
        }
        else if ( buttonPatrol.isVisible() && le.MouseCursor( buttonPatrol.area() ) ) {
            if ( le.MousePressLeft() && buttonPatrol.isReleased() && !Modes( PATROL ) ) {
                buttonPatrol.drawOnPress();
                uint32_t value = _patrolDistance;
                if ( Dialog::SelectCount( _( "Set patrol radius in tiles" ), 0, 255, value ) ) {
                    SetModes( PATROL );
                    _patrolDistance = static_cast<int>( value );
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
        else if ( !isEditor ) {
            if ( le.MousePressRight( rectSpreadArmyFormat ) ) {
                fheroes2::showStandardTextMessage(
                    _( "Spread Formation" ),
                    _( "'Spread' combat formation spreads the hero's units from the top to the bottom of the battlefield, with at least one empty space between each unit." ),
                    Dialog::ZERO );
            }
            else if ( le.MousePressRight( rectGroupedArmyFormat ) ) {
                fheroes2::showStandardTextMessage( _( "Grouped Formation" ),
                                                   _( "'Grouped' combat formation bunches the hero's army together in the center of their side of the battlefield." ),
                                                   Dialog::ZERO );
            }
        }

        // Status messages.
        if ( le.MouseCursor( buttonExit.area() ) ) {
            message = _( "Exit Hero Screen" );
        }
        else if ( buttonDismiss.isVisible() && le.MouseCursor( buttonDismiss.area() ) ) {
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
        else if ( buttonPrevHero.isEnabled() && le.MouseCursor( buttonPrevHero.area() ) ) {
            message = _( "Show previous hero" );
        }
        else if ( buttonNextHero.isEnabled() && le.MouseCursor( buttonNextHero.area() ) ) {
            message = _( "Show next hero" );
        }
        else if ( !isEditor ) {
            // These messages are not shown in the Editor mode.
            if ( le.MouseCursor( rectSpreadArmyFormat ) ) {
                message = _( "Set army combat formation to 'Spread'" );
            }
            else if ( le.MouseCursor( rectGroupedArmyFormat ) ) {
                message = _( "Set army combat formation to 'Grouped'" );
            }
        }
        else if ( message.empty() ) {
            // Editor related status messages.
            if ( le.MouseCursor( selectArtifacts.GetArea() ) ) {
                message = _( "Set hero's Artifacts. Right-click to reset Artifact." );
            }
            else if ( le.MouseCursor( secskill_bar.GetArea() ) ) {
                message = _( "Set hero's Secondary Skills. Right-click to reset skill." );
            }
            else if ( le.MouseCursor( selectArmy.GetArea() ) ) {
                message = _( "Set hero's Army. Right-click to reset unit." );
            }
            else if ( le.MouseCursor( portPos ) ) {
                message = _( "Set hero's portrait. Right-click to reset to default." );
            }
            else if ( le.MouseCursor( titleRoi ) ) {
                message = _( "Click to change hero's name. Right-click to reset to default." );
            }
            else if ( le.MouseCursor( buttonPatrol.area() ) ) {
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

    return Dialog::CANCEL;
}
