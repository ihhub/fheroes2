/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "battle_interface_settings.h"

#include <cstdint>
#include <string>

#include "agg_image.h"
#include "battle_cell.h"
#include "cursor.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_option_item.h"
#include "ui_window.h"

namespace
{
    const fheroes2::Rect turnOrderRoi{ fheroes2::threeOptionsOffsetX, fheroes2::optionsOffsetY, fheroes2::optionIconSize, fheroes2::optionIconSize };
    const fheroes2::Rect gridRoi{ fheroes2::threeOptionsOffsetX + fheroes2::threeOptionsStepX, fheroes2::optionsOffsetY, fheroes2::optionIconSize,
                                  fheroes2::optionIconSize };
    const fheroes2::Rect damageInfoRoi{ fheroes2::threeOptionsOffsetX + fheroes2::threeOptionsStepX * 2, fheroes2::optionsOffsetY, fheroes2::optionIconSize,
                                        fheroes2::optionIconSize };

    const fheroes2::Rect shadowMovementRoi{ fheroes2::threeOptionsOffsetX, fheroes2::optionsOffsetY + fheroes2::optionsStepY, fheroes2::optionIconSize,
                                            fheroes2::optionIconSize };
    const fheroes2::Rect shadowCursorRoi{ fheroes2::threeOptionsOffsetX + fheroes2::threeOptionsStepX, fheroes2::optionsOffsetY + fheroes2::optionsStepY,
                                          fheroes2::optionIconSize, fheroes2::optionIconSize };
    const fheroes2::Rect movementAreaRoi{ fheroes2::threeOptionsOffsetX + fheroes2::threeOptionsStepX * 2, fheroes2::optionsOffsetY + fheroes2::optionsStepY,
                                          fheroes2::optionIconSize, fheroes2::optionIconSize };

    void drawTurnOrder( const fheroes2::Rect & optionRoi )
    {
        const bool isShowTurnOrderEnabled = Settings::Get().BattleShowTurnOrder();
        const fheroes2::Sprite & turnOrderIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowTurnOrderEnabled ? 4 : 3 );
        fheroes2::drawOption( optionRoi, turnOrderIcon, _( "Turn Order" ), isShowTurnOrderEnabled ? _( "On" ) : _( "Off" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawGrid( const fheroes2::Rect & optionRoi )
    {
        const bool isShowBattleGridEnabled = Settings::Get().BattleShowGrid();
        const fheroes2::Sprite & battleGridIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowBattleGridEnabled ? 9 : 8 );
        fheroes2::drawOption( optionRoi, battleGridIcon, _( "Grid" ), isShowBattleGridEnabled ? _( "On" ) : _( "Off" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawDamageInfo( const fheroes2::Rect & optionRoi )
    {
        const bool isShowBattleDamageInfoEnabled = Settings::Get().isBattleShowDamageInfoEnabled();
        const fheroes2::Sprite & damageInfoIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowBattleDamageInfoEnabled ? 4 : 3 );
        fheroes2::drawOption( optionRoi, damageInfoIcon, _( "Damage Info" ), isShowBattleDamageInfoEnabled ? _( "On" ) : _( "Off" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawShadowMovement( const fheroes2::Rect & optionRoi )
    {
        const bool isShowMoveShadowEnabled = Settings::Get().BattleShowMoveShadow();
        const fheroes2::Sprite & moveShadowIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowMoveShadowEnabled ? 11 : 10 );
        fheroes2::drawOption( optionRoi, moveShadowIcon, _( "Shadow Movement" ), isShowMoveShadowEnabled ? _( "On" ) : _( "Off" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void drawShadowCursor( const fheroes2::Rect & optionRoi )
    {
        const bool isShowMouseShadowEnabled = Settings::Get().BattleShowMouseShadow();
        const fheroes2::Sprite & mouseShadowIcon = fheroes2::AGG::GetICN( ICN::CSPANEL, isShowMouseShadowEnabled ? 13 : 12 );
        fheroes2::drawOption( optionRoi, mouseShadowIcon, _( "Shadow Cursor" ), isShowMouseShadowEnabled ? _( "On" ) : _( "Off" ),
                              fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    fheroes2::Image createHexagonShadow()
    {
        const int32_t l = 13;
        const int32_t w = Battle::Cell::widthPx;
        const int32_t h = Battle::Cell::heightPx;

        fheroes2::Image sf( w, h );
        sf.reset();
        fheroes2::Rect rt( 1, l - 1, w - 1, 2 * l + 4 );
        for ( int32_t i = 0; i < w / 2; i += 2 ) {
            for ( int32_t x = 0; x < rt.width; ++x ) {
                for ( int32_t y = 0; y < rt.height; ++y ) {
                    fheroes2::SetTransformPixel( sf, rt.x + x, rt.y + y, 2 );
                }
            }
            --rt.y;
            rt.height += 2;
            rt.x += ( i == 0 ) ? 1 : 2;
            rt.width -= ( i == 0 ) ? 2 : 4;
        }

        return sf;
    }

    void drawMovementArea( const fheroes2::Rect & optionRoi )
    {
        const bool isMovementAreaEnabled = Settings::Get().isBattleMovementAreaHighlightEnabled();

        fheroes2::Sprite image = fheroes2::AGG::GetICN( ICN::EMPTY_OPTION_ICON_BACKGROUND, 0 );

        if ( isMovementAreaEnabled ) {
            // Render the shadow area.
            const auto shadow = createHexagonShadow();
            fheroes2::Blit( shadow, 0, 0, image, ( image.width() - shadow.width() ) / 2, ( image.height() - shadow.height() ) / 2, shadow.width(), shadow.height() );
        }

        // Draw Hydra.
        const auto & hydraIcon = fheroes2::AGG::GetICN( ICN::MONS32, 34 );
        fheroes2::Blit( hydraIcon, 0, 0, image, ( image.width() - hydraIcon.width() ) / 2, ( image.height() - hydraIcon.height() ) / 2, hydraIcon.width(),
                        hydraIcon.height() );

        if ( isMovementAreaEnabled ) {
            // Render a cursor.
            const auto & cursor = fheroes2::AGG::GetICN( ICN::ADVMCO, 0 );
            fheroes2::Blit( cursor, 0, 0, image, ( image.width() - cursor.width() ) / 2 + 10, ( image.height() - cursor.height() ) / 2 + 15, cursor.width(),
                            cursor.height() );
        }

        fheroes2::drawOption( optionRoi, image, _( "Movement Area" ), isMovementAreaEnabled ? _( "On" ) : _( "Off" ), fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
    }

    void openInterfaceBattleOptionDialog( bool & saveConfiguration )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        // Set the cursor image. This dialog is called from the battlefield and does not require a cursor restorer.
        // Battlefield event processor will set the appropriate cursor after this dialog is closed.
        Cursor::Get().SetThemes( Cursor::POINTER );

        fheroes2::StandardWindow background( 289, fheroes2::optionsStepY * 2 + 52, true, display );

        const fheroes2::Rect windowRoi = background.activeArea();

        Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        fheroes2::Button buttonOk;
        const int buttonOkIcnId = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        background.renderButton( buttonOk, buttonOkIcnId, 0, 1, { 0, 5 }, fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        fheroes2::ImageRestorer emptyDialogRestorer( display, windowRoi.x, windowRoi.y, windowRoi.width, windowRoi.height );

        const fheroes2::Rect windowTurnOrderRoi( turnOrderRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowGridRoi( gridRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowDamageInfoRoi( damageInfoRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowShadowMovementRoi( shadowMovementRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowShadowCursorRoi( shadowCursorRoi + windowRoi.getPosition() );
        const fheroes2::Rect windowMovementAreaRoi( movementAreaRoi + windowRoi.getPosition() );

        const auto drawOptions = [&windowTurnOrderRoi, &windowGridRoi, &windowDamageInfoRoi, &windowShadowMovementRoi, &windowShadowCursorRoi, &windowMovementAreaRoi]() {
            drawTurnOrder( windowTurnOrderRoi );
            drawGrid( windowGridRoi );
            drawDamageInfo( windowDamageInfoRoi );
            drawShadowMovement( windowShadowMovementRoi );
            drawShadowCursor( windowShadowCursorRoi );
            drawMovementArea( windowMovementAreaRoi );
        };

        drawOptions();

        display.render( background.totalArea() );

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );

            bool redrawScreen = false;

            if ( le.MouseClickLeft( windowTurnOrderRoi ) ) {
                conf.setBattleShowTurnOrder( !conf.BattleShowTurnOrder() );
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( windowGridRoi ) ) {
                conf.SetBattleGrid( !conf.BattleShowGrid() );
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( windowShadowMovementRoi ) ) {
                conf.SetBattleMovementShaded( !conf.BattleShowMoveShadow() );
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( windowShadowCursorRoi ) ) {
                conf.SetBattleMouseShaded( !conf.BattleShowMouseShadow() );
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( windowMovementAreaRoi ) ) {
                conf.setHighlightBattleMovementArea( !conf.isBattleMovementAreaHighlightEnabled() );
                redrawScreen = true;
            }
            else if ( le.MouseClickLeft( windowDamageInfoRoi ) ) {
                conf.setBattleDamageInfo( !conf.isBattleShowDamageInfoEnabled() );
                redrawScreen = true;
            }
            else if ( le.isMouseRightButtonPressedInArea( windowTurnOrderRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Turn Order" ), _( "Toggle to display the turn order during the battle." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowGridRoi ) ) {
                fheroes2::showStandardTextMessage(
                    _( "Grid" ),
                    _( "Toggle the hex grid on or off. The hex grid always underlies movement, even if turned off. This switch only determines if the grid is visible." ),
                    0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowDamageInfoRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Damage Info" ), _( "Toggle to display damage information during the battle." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowShadowMovementRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Shadow Movement" ), _( "Toggle on or off shadows showing where your creatures can move and attack." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowShadowCursorRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Shadow Cursor" ), _( "Toggle on or off a shadow showing the current hex location of the mouse cursor." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( windowMovementAreaRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Movement Area" ), _( "Toggle showing the movement area of a highlighted creature on or off ." ), 0 );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Exit this menu." ), 0 );
            }

            if ( Game::HotKeyCloseWindow() || le.MouseClickLeft( buttonOk.area() ) ) {
                break;
            }

            if ( redrawScreen ) {
                emptyDialogRestorer.restore();
                drawOptions();
                display.render( emptyDialogRestorer.rect() );

                saveConfiguration = true;
            }
        }
    }
}

namespace Battle
{
    bool showBattleInterfaceDialog()
    {
        bool isSaveConfiguration{ false };

        openInterfaceBattleOptionDialog( isSaveConfiguration );
        return isSaveConfiguration;
    }
}
