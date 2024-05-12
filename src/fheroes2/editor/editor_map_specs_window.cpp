/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#include "editor_map_specs_window.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "difficulty.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "map_format_info.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

// TODO: Remove this when Victory and Loss conditions are fully implemented.
#define HIDE_VICTORY_LOSS_CONDITIONS

#ifndef HIDE_VICTORY_LOSS_CONDITIONS
#include <algorithm>

#include "game_over.h"
#include "interface_list.h"
#include "maps_fileinfo.h"
#include "tools.h"
#endif

namespace
{
    // In original Editor map name is limited to 17 characters. We keep this limit to fit the Select Scenario dialog.
    const int32_t maxMapNameLength = 17;

    const int32_t descriptionBoxWidth = 292;
    const int32_t descriptionBoxHeight = 90;

    const int32_t playerStepX = 80;
    const int32_t difficultyStepX = 77;

#ifndef HIDE_VICTORY_LOSS_CONDITIONS
    const char * getVictoryConditionText( const uint8_t victoryConditionType )
    {
        switch ( victoryConditionType ) {
        case Maps::FileInfo::VICTORY_DEFEAT_EVERYONE:
            return _( "None." );
        case Maps::FileInfo::VICTORY_CAPTURE_TOWN:
            return GameOver::GetString( GameOver::WINS_TOWN );
        case Maps::FileInfo::VICTORY_KILL_HERO:
            return GameOver::GetString( GameOver::WINS_HERO );
        case Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT:
            return GameOver::GetString( GameOver::WINS_ARTIFACT );
        case Maps::FileInfo::VICTORY_DEFEAT_OTHER_SIDE:
            return _( "One side defeats another." );
        case Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD:
            return _( "Accumulate gold." );
        default:
            // This is an unknown condition. Add the logic for it above!
            assert( 0 );
            break;
        }

        return {};
    }

    uint32_t getVictoryIcnIndex( const uint8_t victoryConditionType )
    {
        switch ( victoryConditionType ) {
        case Maps::FileInfo::VICTORY_DEFEAT_EVERYONE:
            return 30;
        case Maps::FileInfo::VICTORY_CAPTURE_TOWN:
            return 31;
        case Maps::FileInfo::VICTORY_KILL_HERO:
            return 32;
        case Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT:
            return 33;
        case Maps::FileInfo::VICTORY_DEFEAT_OTHER_SIDE:
            return 34;
        case Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD:
            return 35;
        default:
            // This is an unknown condition. Add the logic for it above!
            assert( 0 );
            break;
        }

        return 0;
    }

    const char * getLossConditionText( const uint8_t lossConditionType )
    {
        switch ( lossConditionType ) {
        case Maps::FileInfo::LOSS_EVERYTHING:
            return _( "None." );
        case Maps::FileInfo::LOSS_TOWN:
            return GameOver::GetString( GameOver::LOSS_TOWN );
        case Maps::FileInfo::LOSS_HERO:
            return GameOver::GetString( GameOver::LOSS_HERO );
        case Maps::FileInfo::LOSS_OUT_OF_TIME:
            return _( "Run out of time." );
        default:
            // This is an unknown condition. Add the logic for it above!
            assert( 0 );
            break;
        }

        return {};
    }

    uint32_t getLossIcnIndex( const uint8_t lossConditionType )
    {
        switch ( lossConditionType ) {
        case Maps::FileInfo::LOSS_EVERYTHING:
            return 36;
        case Maps::FileInfo::LOSS_TOWN:
            return 37;
        case Maps::FileInfo::LOSS_HERO:
            return 38;
        case Maps::FileInfo::LOSS_OUT_OF_TIME:
            return 39;
        default:
            // This is an unknown condition. Add the logic for it above!
            assert( 0 );
            break;
        }

        return 0;
    }

    void redrawVictoryCondition( const uint8_t condition, const fheroes2::Rect & roi, const bool yellowFont, fheroes2::Image & output )
    {
        const fheroes2::Sprite & winIcon = fheroes2::AGG::GetICN( ICN::REQUESTS, getVictoryIcnIndex( condition ) );
        fheroes2::Copy( winIcon, 0, 0, output, roi.x + 1, roi.y, winIcon.width(), winIcon.height() );
        const fheroes2::Text winText( getVictoryConditionText( condition ), yellowFont ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        winText.drawInRoi( roi.x + 20, roi.y + 2, output, roi );
    }

    void redrawLossCondition( const uint8_t condition, const fheroes2::Rect & roi, const bool yellowFont, fheroes2::Image & output )
    {
        const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::REQUESTS, getLossIcnIndex( condition ) );
        fheroes2::Copy( icon, 0, 0, output, roi.x + 1, roi.y, icon.width(), icon.height() );
        const fheroes2::Text winText( getLossConditionText( condition ), yellowFont ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        winText.drawInRoi( roi.x + 20, roi.y + 2, output, roi );
    }

    class DropBoxList final : public Interface::ListBox<uint8_t>
    {
    public:
        using Interface::ListBox<uint8_t>::ActionListDoubleClick;
        using Interface::ListBox<uint8_t>::ActionListSingleClick;
        using Interface::ListBox<uint8_t>::ActionListPressRight;

        DropBoxList( const DropBoxList & ) = delete;
        DropBoxList & operator=( const DropBoxList & ) = delete;

        explicit DropBoxList( const fheroes2::Point & pt, const int32_t itemsCount, const bool isLossList )
            : Interface::ListBox<uint8_t>( pt )
            , _isLossList( isLossList )
        {
            SetAreaMaxItems( itemsCount );

            fheroes2::Display & display = fheroes2::Display::instance();

            const fheroes2::Sprite & image = fheroes2::AGG::GetICN( ICN::DROPLISL, 0 );

            const int32_t topPartHeight = image.height() - 2;
            const int32_t listWidth = image.width();
            const int32_t middlePartHeight = topPartHeight - 2;
            const int32_t bottomPartHeight = topPartHeight;
            const int32_t listHeight = itemsCount * ( _itemHeight + 2 ) + 10;

            _itemWidth = listWidth - 6;

            _restorer = std::make_unique<fheroes2::ImageRestorer>( display, pt.x, pt.y, listWidth + 1, listHeight );

            // Top part of list background.
            fheroes2::Copy( image, 0, 0, display, pt.x, pt.y, listWidth, topPartHeight );
            const int32_t lineFixOffsetX = pt.x + listWidth;
            fheroes2::Copy( image, 0, 0, display, lineFixOffsetX, pt.y, 1, topPartHeight );

            // Middle part of list background.
            const int32_t middlePartCount = ( listHeight - topPartHeight - bottomPartHeight + middlePartHeight - 1 ) / middlePartHeight;
            int32_t offsetY = topPartHeight;

            for ( int32_t i = 0; i < middlePartCount; ++i ) {
                const int32_t copyHeight = std::min( middlePartHeight, listHeight - bottomPartHeight - offsetY );
                const int32_t posY = pt.y + offsetY;

                fheroes2::Copy( image, 0, 2, display, pt.x, posY, listWidth, copyHeight );
                fheroes2::Copy( image, 0, 2, display, lineFixOffsetX, posY, 1, copyHeight );

                offsetY += middlePartHeight;
            }

            // Bottom part of list background.
            offsetY = pt.y + listHeight - bottomPartHeight;
            fheroes2::Copy( image, 0, 2, display, pt.x, offsetY, listWidth, bottomPartHeight );
            fheroes2::Copy( image, 0, 2, display, lineFixOffsetX, offsetY, 1, bottomPartHeight );

            _background = std::make_unique<fheroes2::ImageRestorer>( display, pt.x + 2, pt.y + 2, listWidth - 3, listHeight - 4 );

            SetAreaItems( { pt.x + 5, pt.y + 5, listWidth - 10, listHeight - 10 } );
        }

        ~DropBoxList() override
        {
            // After closing the drop list we also need to render the restored background.
            _restorer->restore();
            fheroes2::Display::instance().render( _restorer->rect() );
        }

        void RedrawItem( const uint8_t & condition, int32_t dstX, int32_t dstY, bool current ) override
        {
            if ( _isLossList ) {
                redrawLossCondition( condition, { dstX, dstY, _itemWidth, _itemHeight }, current, fheroes2::Display::instance() );
            }
            else {
                redrawVictoryCondition( condition, { dstX, dstY, _itemWidth, _itemHeight }, current, fheroes2::Display::instance() );
            }
        }

        void RedrawBackground( const fheroes2::Point & /*dst*/ ) override
        {
            _background->restore();
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( uint8_t & /* item */ ) override
        {
            _isClicked = true;
        }

        void ActionListSingleClick( uint8_t & /* item */ ) override
        {
            _isClicked = true;
        }

        void ActionListPressRight( uint8_t & /* item */ ) override
        {
            // Do nothing.
        }

        fheroes2::Rect getArea() const
        {
            if ( _restorer == nullptr ) {
                return {};
            }

            return _restorer->rect();
        }

        bool isClicked() const
        {
            return _isClicked;
        }

    private:
        bool _isClicked{ false };
        bool _isLossList{ false };
        int32_t _itemWidth;
        int32_t _itemHeight{ fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) + 1 };
        std::unique_ptr<fheroes2::ImageRestorer> _restorer;
        std::unique_ptr<fheroes2::ImageRestorer> _background;
    };

    uint8_t showWinLoseList( const fheroes2::Point & pt, const uint8_t current, const bool isLossList )
    {
        std::vector<uint8_t> conditions;
        if ( isLossList ) {
            conditions = { Maps::FileInfo::LOSS_EVERYTHING, Maps::FileInfo::LOSS_TOWN, Maps::FileInfo::LOSS_HERO, Maps::FileInfo::LOSS_OUT_OF_TIME };
        }
        else {
            conditions = { Maps::FileInfo::VICTORY_DEFEAT_EVERYONE, Maps::FileInfo::VICTORY_CAPTURE_TOWN,      Maps::FileInfo::VICTORY_KILL_HERO,
                           Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT, Maps::FileInfo::VICTORY_DEFEAT_OTHER_SIDE, Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD };
        }

        DropBoxList victoryConditionsList( pt, static_cast<int32_t>( conditions.size() ), isLossList );
        victoryConditionsList.SetListContent( conditions );
        victoryConditionsList.SetCurrent( current );
        victoryConditionsList.Redraw();

        const fheroes2::Rect listArea( victoryConditionsList.getArea() );

        fheroes2::Display & display = fheroes2::Display::instance();
        display.render( listArea );

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            victoryConditionsList.QueueEventProcessing();

            if ( victoryConditionsList.isClicked() || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
                assert( victoryConditionsList.IsValid() );

                return victoryConditionsList.GetCurrent();
            }

            if ( le.MouseClickLeft() || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                break;
            }

            if ( victoryConditionsList.IsNeedRedraw() ) {
                victoryConditionsList.Redraw();
                display.render( listArea );
            }
        }

        return current;
    }
#endif // HIDE_VICTORY_LOSS_CONDITIONS

    uint32_t getPlayerIcnIndex( const Maps::Map_Format::MapFormat & mapFormat, const int currentColor )
    {
        if ( !( mapFormat.availablePlayerColors & currentColor ) ) {
            // This player is not available.
            assert( 0 );

            return 70;
        }

        if ( mapFormat.humanPlayerColors & currentColor ) {
            if ( mapFormat.computerPlayerColors & currentColor ) {
                // Both AI and human can choose this player color.
                return 82;
            }

            // Human only.
            return 9;
        }

        if ( mapFormat.computerPlayerColors & currentColor ) {
            // AI only.
            return 3;
        }

        // It is not possible to have available player which is not allowed to be controlled by AI or human.
        assert( 0 );

        return 70;
    }

    size_t getDifficultyIndex( const uint8_t difficulty )
    {
        switch ( difficulty ) {
        case Difficulty::EASY:
            return 0;
        case Difficulty::NORMAL:
            return 1;
        case Difficulty::HARD:
            return 2;
        case Difficulty::EXPERT:
            return 3;
        default:
            // Did you add a new difficulty mode? Add the corresponding case above!
            assert( 0 );
            break;
        }

        return 0;
    }

    uint8_t setDifficultyByIndex( const size_t difficultyIndex )
    {
        switch ( difficultyIndex ) {
        case 0:
            return Difficulty::EASY;
        case 1:
            return Difficulty::NORMAL;
        case 2:
            return Difficulty::HARD;
        case 3:
            return Difficulty::EXPERT;
        default:
            // Did you add a new difficulty mode? Add the corresponding case above!
            assert( 0 );
            break;
        }

        return Difficulty::EASY;
    }
}

namespace Editor
{
    bool mapSpecificationsDialog( Maps::Map_Format::MapFormat & mapFormat )
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        const bool isDefaultScreenSize = display.isDefaultSize();

        fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, !isDefaultScreenSize );
        const fheroes2::Rect activeArea( background.activeArea() );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        if ( isDefaultScreenSize ) {
            const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 );
            fheroes2::Copy( backgroundImage, 0, 0, display, activeArea );
        }

        if ( mapFormat.name.empty() ) {
            mapFormat.name = "My Map";
        }

        // Map name.
        const fheroes2::Sprite & scenarioBox = fheroes2::AGG::GetICN( isEvilInterface ? ICN::METALLIC_BORDERED_TEXTBOX_EVIL : ICN::METALLIC_BORDERED_TEXTBOX_GOOD, 0 );
        const fheroes2::Rect scenarioBoxRoi( activeArea.x + ( activeArea.width - scenarioBox.width() ) / 2, activeArea.y + 10, scenarioBox.width(),
                                             scenarioBox.height() );
        const fheroes2::Rect mapNameRoi( scenarioBoxRoi.x + 6, scenarioBoxRoi.y + 5, scenarioBoxRoi.width - 12, scenarioBoxRoi.height - 11 );

        fheroes2::Copy( scenarioBox, 0, 0, display, scenarioBoxRoi );
        fheroes2::addGradientShadow( scenarioBox, display, scenarioBoxRoi.getPosition(), { -5, 5 } );

        fheroes2::Text text( mapFormat.name, fheroes2::FontType::normalWhite() );
        text.drawInRoi( mapNameRoi.x, mapNameRoi.y + 3, mapNameRoi.width, display, mapNameRoi );

        // Players setting (AI or human).
        const int32_t availablePlayersCount = Color::Count( mapFormat.availablePlayerColors );
        int32_t offsetX = activeArea.x + ( activeArea.width - availablePlayersCount * playerStepX ) / 2;
        int32_t offsetY = scenarioBoxRoi.y + scenarioBoxRoi.height + 10;

        std::vector<fheroes2::Rect> playerRects( availablePlayersCount );
        const Colors availableColors( mapFormat.availablePlayerColors );

        const fheroes2::Sprite & playerIconShadow = fheroes2::AGG::GetICN( ICN::NGEXTRA, 61 );
        for ( int32_t i = 0; i < availablePlayersCount; ++i ) {
            playerRects[i].x = offsetX + i * playerStepX;
            playerRects[i].y = offsetY;

            fheroes2::Blit( playerIconShadow, display, playerRects[i].x - 5, playerRects[i].y + 3 );

            const uint32_t icnIndex = Color::GetIndex( availableColors[i] ) + getPlayerIcnIndex( mapFormat, availableColors[i] );

            const fheroes2::Sprite & playerIcon = fheroes2::AGG::GetICN( ICN::NGEXTRA, icnIndex );
            playerRects[i].width = playerIcon.width();
            playerRects[i].height = playerIcon.height();
            fheroes2::Copy( playerIcon, 0, 0, display, playerRects[i].x, playerRects[i].y, playerRects[i].width, playerRects[i].height );
        }

        // Draw difficulty icons.
        offsetX = activeArea.x + 15;
        offsetY = scenarioBoxRoi.y + scenarioBoxRoi.height + 65;

        text.set( _( "Map Difficulty" ), fheroes2::FontType::normalWhite() );
        text.draw( offsetX + ( difficultyStepX * 4 - text.width() ) / 2, scenarioBoxRoi.y + scenarioBoxRoi.height + 65, display );

        std::array<fheroes2::Rect, 4> difficultyRects;
        offsetY += 23;

        const fheroes2::Sprite & difficultyCursorSprite = fheroes2::AGG::GetICN( ICN::NGEXTRA, 62 );
        const int32_t difficultyIconSideLength = difficultyCursorSprite.width();
        const int difficultyIcnIndex = isEvilInterface ? 1 : 0;

        for ( int i = 0; i < 4; ++i ) {
            difficultyRects[i].x = offsetX + difficultyStepX * i;
            difficultyRects[i].y = offsetY;
            difficultyRects[i].width = difficultyIconSideLength;
            difficultyRects[i].height = difficultyIconSideLength;

            const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::DIFFICULTY_ICON_EASY + i, difficultyIcnIndex );
            fheroes2::Copy( icon, 0, 0, display, difficultyRects[i] );
            fheroes2::addGradientShadow( icon, display, { difficultyRects[i].x, difficultyRects[i].y }, { -5, 5 } );

            difficultyRects[i].x -= 3;
            difficultyRects[i].y -= 3;

            text.set( Difficulty::String( setDifficultyByIndex( i ) ), fheroes2::FontType::smallWhite() );
            text.draw( difficultyRects[i].x + ( difficultyRects[i].width - text.width() ) / 2, difficultyRects[i].y + difficultyRects[i].height + 7, display );
        }

        fheroes2::MovableSprite difficultyCursor( difficultyCursorSprite );

        size_t difficultyIndex = getDifficultyIndex( mapFormat.difficulty );
        difficultyCursor.setPosition( difficultyRects[difficultyIndex].x, difficultyRects[difficultyIndex].y );
        difficultyCursor.redraw();

        // Map description.
        offsetX = activeArea.x + activeArea.width - descriptionBoxWidth - 15;
        offsetY -= 23;

        text.set( _( "Map Description" ), fheroes2::FontType::normalWhite() );
        text.draw( offsetX + ( descriptionBoxWidth - text.width() ) / 2, offsetY, display );

        offsetY += 25;
        const fheroes2::Rect descriptionTextRoi( offsetX, offsetY, descriptionBoxWidth, descriptionBoxHeight );
        background.applyTextBackgroundShading( { descriptionTextRoi.x - 6, descriptionTextRoi.y - 6, descriptionTextRoi.width + 12, descriptionTextRoi.height + 12 } );
        fheroes2::ImageRestorer descriptionBackground( display, descriptionTextRoi.x, descriptionTextRoi.y, descriptionTextRoi.width, descriptionTextRoi.height );

        text.set( mapFormat.description, fheroes2::FontType::normalWhite() );
        text.drawInRoi( descriptionTextRoi.x, descriptionTextRoi.y, descriptionTextRoi.width, display, descriptionTextRoi );

#ifndef HIDE_VICTORY_LOSS_CONDITIONS
        // Victory conditions.
        offsetY += descriptionTextRoi.height + 20;

        text.set( _( "Special Victory Condition" ), fheroes2::FontType::normalWhite() );
        text.draw( activeArea.x + activeArea.width / 4 - text.width() / 2, offsetY, display );

        offsetY += 20;
        const fheroes2::Sprite & itemBackground = fheroes2::AGG::GetICN( ICN::DROPLISL, 0 );
        const int32_t itemBackgroundWidth = itemBackground.width();
        const int32_t itemBackgroundHeight = itemBackground.height();
        const int32_t itemBackgroundOffsetX = activeArea.width / 4 - itemBackgroundWidth / 2 - 11;

        offsetX = activeArea.x + itemBackgroundOffsetX;
        fheroes2::Copy( itemBackground, 0, 0, display, offsetX, offsetY, itemBackgroundWidth, itemBackgroundHeight );
        const fheroes2::Rect victoryTextRoi( offsetX + 2, offsetY + 3, itemBackgroundWidth - 4, itemBackgroundHeight - 5 );

        redrawVictoryCondition( mapFormat.victoryConditionType, victoryTextRoi, false, display );

        fheroes2::ButtonSprite victoryDroplistButton( offsetX + itemBackgroundWidth, offsetY, fheroes2::AGG::GetICN( ICN::DROPLISL, 1 ),
                                                      fheroes2::AGG::GetICN( ICN::DROPLISL, 2 ) );
        const fheroes2::Rect victoryDroplistButtonRoi( fheroes2::getBoundaryRect( victoryDroplistButton.area(), victoryTextRoi ) );
        victoryDroplistButton.draw();

        // Loss conditions.
        offsetY = descriptionTextRoi.y + descriptionTextRoi.height + 20;

        text.set( _( "Special Loss Condition" ), fheroes2::FontType::normalWhite() );
        text.draw( activeArea.x + 3 * activeArea.width / 4 - text.width() / 2, offsetY, display );

        offsetY += 20;
        offsetX = activeArea.x + activeArea.width / 2 + itemBackgroundOffsetX;
        fheroes2::Copy( itemBackground, 0, 0, display, offsetX, offsetY, itemBackgroundWidth, itemBackground.height() );
        const fheroes2::Rect lossTextRoi( offsetX + 2, offsetY + 3, victoryTextRoi.width, victoryTextRoi.height );

        redrawLossCondition( mapFormat.lossConditionType, lossTextRoi, false, display );

        fheroes2::ButtonSprite lossDroplistButton( offsetX + itemBackgroundWidth, offsetY, fheroes2::AGG::GetICN( ICN::DROPLISL, 1 ),
                                                   fheroes2::AGG::GetICN( ICN::DROPLISL, 2 ) );
        const fheroes2::Rect lossDroplistButtonRoi( fheroes2::getBoundaryRect( lossDroplistButton.area(), lossTextRoi ) );
        lossDroplistButton.draw();
#endif // HIDE_VICTORY_LOSS_CONDITIONS

        // Buttons.
        fheroes2::Button buttonCancel;
        const int buttonCancelIcn = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
        background.renderButton( buttonCancel, buttonCancelIcn, 0, 1, { 20, 6 }, fheroes2::StandardWindow::Padding::BOTTOM_RIGHT );
        const fheroes2::Rect buttonCancelRoi( buttonCancel.area() );

        fheroes2::Button buttonOk;
        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        background.renderButton( buttonOk, buttonOkIcn, 0, 1, { 20 + buttonCancelRoi.width + 10, 6 }, fheroes2::StandardWindow::Padding::BOTTOM_RIGHT );
        const fheroes2::Rect buttonOkRoi( buttonOk.area() );

        LocalEvent & le = LocalEvent::Get();

        display.render( background.totalArea() );

        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.MousePressLeft( buttonOkRoi ) );
            buttonCancel.drawOnState( le.MousePressLeft( buttonCancelRoi ) );
#ifndef HIDE_VICTORY_LOSS_CONDITIONS
            victoryDroplistButton.drawOnState( le.MousePressLeft( victoryDroplistButtonRoi ) );
            lossDroplistButton.drawOnState( le.MousePressLeft( lossDroplistButtonRoi ) );
#endif // HIDE_VICTORY_LOSS_CONDITIONS

            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancelRoi ) ) {
                return false;
            }

            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOkRoi ) ) {
                break;
            }

            if ( le.MouseClickLeft( mapNameRoi ) ) {
                // TODO: Edit texts directly in this dialog.

                std::string editableMapName = mapFormat.name;
                if ( Dialog::inputString( _( "Change Map Name" ), editableMapName, {}, maxMapNameLength, false ) ) {
                    mapFormat.name = std::move( editableMapName );
                    text.set( mapFormat.name, fheroes2::FontType::normalWhite() );
                    fheroes2::Copy( scenarioBox, 0, 0, display, scenarioBoxRoi );
                    text.drawInRoi( mapNameRoi.x, mapNameRoi.y + 3, mapNameRoi.width, display, mapNameRoi );

                    display.render( scenarioBoxRoi );
                }
            }
            else if ( le.MouseClickLeft( descriptionTextRoi ) ) {
                // TODO: Edit texts directly in this dialog.
                // TODO: Limit description to 5 text lines.

                std::string signText = mapFormat.description;
                if ( Dialog::inputString( _( "Change Map Description" ), signText, {}, 150, true ) ) {
                    mapFormat.description = std::move( signText );

                    text.set( mapFormat.description, fheroes2::FontType::normalWhite() );

                    // TODO: Remove this temporary fix when direct text edit with text length checks is implemented.
                    if ( text.rows( descriptionTextRoi.width ) > 5 ) {
                        fheroes2::showStandardTextMessage(
                            _( "Warning" ), _( "The entered map description exceeds the maximum allowed 5 rows. It will be shortened to fit the map description field." ),
                            Dialog::OK );

                        // As a temporary solution we cut the end of the text to fit 5 rows.
                        while ( text.rows( descriptionTextRoi.width ) > 5 ) {
                            mapFormat.description.pop_back();
                            text.set( mapFormat.description, fheroes2::FontType::normalWhite() );
                        }
                    }

                    descriptionBackground.restore();
                    text.drawInRoi( descriptionTextRoi.x, descriptionTextRoi.y, descriptionTextRoi.width, display, descriptionTextRoi );
                    display.render( descriptionTextRoi );
                }
            }
#ifndef HIDE_VICTORY_LOSS_CONDITIONS
            else if ( le.MouseClickLeft( victoryDroplistButtonRoi ) ) {
                const uint8_t result = showWinLoseList( { victoryTextRoi.x - 2, victoryTextRoi.y + victoryTextRoi.height }, mapFormat.victoryConditionType, false );

                if ( result != mapFormat.victoryConditionType ) {
                    mapFormat.victoryConditionType = result;

                    fheroes2::Copy( itemBackground, 2, 3, display, victoryTextRoi );
                    redrawVictoryCondition( mapFormat.victoryConditionType, victoryTextRoi, false, display );
                    display.render( victoryTextRoi );
                }
            }
            else if ( le.MouseClickLeft( lossDroplistButtonRoi ) ) {
                const uint8_t result = showWinLoseList( { lossTextRoi.x - 2, lossTextRoi.y + lossTextRoi.height }, mapFormat.lossConditionType, true );

                if ( result != mapFormat.lossConditionType ) {
                    mapFormat.lossConditionType = result;

                    fheroes2::Copy( itemBackground, 2, 3, display, lossTextRoi );
                    redrawLossCondition( mapFormat.lossConditionType, lossTextRoi, false, display );
                    display.render( lossTextRoi );
                }
            }
#endif // HIDE_VICTORY_LOSS_CONDITIONS
            else if ( le.MousePressRight( buttonCancelRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( buttonOkRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to accept the changes made." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( mapNameRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Map Name" ), _( "Click to change your map name." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( descriptionTextRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Map Description" ), _( "Click to change the description of the current map." ), Dialog::ZERO );
            }

            for ( int32_t i = 0; i < availablePlayersCount; ++i ) {
                if ( le.MouseClickLeft( playerRects[i] ) ) {
                    if ( !( mapFormat.availablePlayerColors & availableColors[i] ) ) {
                        break;
                    }

                    const bool allowAi = mapFormat.computerPlayerColors & availableColors[i];
                    const bool allowHuman = mapFormat.humanPlayerColors & availableColors[i];

                    if ( allowHuman ) {
                        if ( allowAi ) {
                            // Disable AI.
                            mapFormat.computerPlayerColors ^= availableColors[i];
                        }
                        else {
                            // Enable AI.
                            mapFormat.computerPlayerColors |= availableColors[i];
                            if ( Color::Count( mapFormat.humanPlayerColors ) > 1 ) {
                                // and disable human only if any other player can be controlled by human.
                                mapFormat.humanPlayerColors ^= availableColors[i];
                            }
                        }
                    }
                    else {
                        // Enable human.
                        mapFormat.humanPlayerColors |= availableColors[i];
                    }

                    // Update player icon.
                    const uint32_t icnIndex = Color::GetIndex( availableColors[i] ) + getPlayerIcnIndex( mapFormat, availableColors[i] );
                    const fheroes2::Sprite & playerIcon = fheroes2::AGG::GetICN( ICN::NGEXTRA, icnIndex );
                    fheroes2::Copy( playerIcon, 0, 0, display, playerRects[i].x, playerRects[i].y, playerRects[i].width, playerRects[i].height );
                    display.render( playerRects[i] );

                    break;
                }

                if ( le.MousePressRight( playerRects[i] ) ) {
                    fheroes2::showStandardTextMessage( _( "Player Type" ), _( "Indicates the player types in the scenario. Click to change." ), Dialog::ZERO );
                }
            }

            for ( size_t i = 0; i < 4; ++i ) {
                if ( le.MouseClickLeft( difficultyRects[i] ) ) {
                    if ( i == difficultyIndex ) {
                        // This difficulty is already selected.
                        break;
                    }

                    difficultyCursor.setPosition( difficultyRects[i].x, difficultyRects[i].y );
                    difficultyCursor.redraw();
                    mapFormat.difficulty = setDifficultyByIndex( i );

                    display.updateNextRenderRoi( difficultyRects[difficultyIndex] );

                    difficultyIndex = i;

                    display.render( difficultyRects[i] );

                    break;
                }

                if ( le.MousePressRight( difficultyRects[i] ) ) {
                    fheroes2::showStandardTextMessage(
                        _( "Map Difficulty" ),
                        _( "Click to set map difficulty. More difficult maps might include more or stronger enemies, fewer resources, or other special conditions making things tougher for the human player." ),
                        Dialog::ZERO );
                }
            }
        }

        return true;
    }
}
