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

#include "dialog_troop_setup.h"

#include <cassert>
#include <optional>
#include <string>

#include "army_troop.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "icn.h"
#include "localevent.h"
#include "math_base.h"
#include "monster.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"

namespace fheroes2
{
    class Image;
}

namespace
{
    class TroopSelectionDialogElement final : public fheroes2::TroopDialogElement
    {
    public:
        explicit TroopSelectionDialogElement( const int32_t monster, int32_t & troopCount )
            : TroopDialogElement( monster, troopCount )
            , _troopCount( troopCount )
        {
            _updatedMonster = monster;
        }

        ~TroopSelectionDialogElement() override = default;

        void showPopup( const int buttons ) const override
        {
            if ( buttons != 0 ) {
                _updatedMonster = Dialog::selectMonster( _monster.GetID() );
            }
            else {
                fheroes2::TroopDialogElement::showPopup( buttons );
            }
        }

        bool update( fheroes2::Image & output, const fheroes2::Point & offset ) const override
        {
            if ( ( _updatedMonster != _monster || _troopCount != static_cast<int32_t>( _count ) ) && _updatedMonster != Monster::UNKNOWN ) {
                _monster = _updatedMonster;
                _count = static_cast<uint32_t>( _troopCount );
                draw( output, offset );
                return true;
            }

            return false;
        }

        int32_t getMonsterType() const
        {
            return _monster.GetID();
        }

    private:
        int32_t & _troopCount;
        mutable Monster _updatedMonster;
    };

    void switchMaxMinButtons( fheroes2::ButtonBase & minButton, fheroes2::ButtonBase & maxButton, const int32_t currentValue, const int32_t minimumValue )
    {
        const bool isMinValue = ( currentValue <= minimumValue );

        if ( ( isMinValue && minButton.isHidden() && maxButton.isVisible() ) || ( !isMinValue && minButton.isVisible() && maxButton.isHidden() ) ) {
            // The MIN/MAX buttons switch is not needed.
            return;
        }

        if ( isMinValue ) {
            minButton.hide();
            maxButton.show();
            maxButton.draw();
        }
        else {
            maxButton.hide();
            minButton.show();
            minButton.draw();
        }
    }
}

namespace Dialog
{
    bool selectTroop( Troop & troop, const int32_t min, const int32_t max )
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        int32_t monsterCount = std::max<int32_t>( static_cast<int32_t>( troop.GetCount() ), 1 );

        const TroopSelectionDialogElement monsterUiElement{ troop.GetID(), monsterCount };

        const fheroes2::Text headerText( _( "Select monster:" ), fheroes2::FontType::normalWhite() );
        constexpr int32_t elementOffset{ 10 };
        constexpr int32_t selectionAreaHeight{ 30 };
        const int32_t headerHeight = headerText.height( fheroes2::boxAreaWidthPx );

        int32_t offsetY = headerHeight + elementOffset;

        const FrameBox box( offsetY + monsterUiElement.area().height + selectionAreaHeight + elementOffset, true );

        const fheroes2::Rect & windowArea = box.GetArea();

        fheroes2::Display & display = fheroes2::Display::instance();
        headerText.draw( windowArea.x, windowArea.y, fheroes2::boxAreaWidthPx, display );

        offsetY += windowArea.y;
        const fheroes2::Point monsterUiOffset{ windowArea.x + ( windowArea.width - monsterUiElement.area().width ) / 2, offsetY };
        monsterUiElement.draw( display, monsterUiOffset );
        offsetY += elementOffset + monsterUiElement.area().height;

        const fheroes2::Size valueSelectionSize{ fheroes2::ValueSelectionDialogElement::getArea() };
        const fheroes2::Rect selectionBoxArea{ windowArea.x + 38, offsetY, valueSelectionSize.width, valueSelectionSize.height };

        fheroes2::ValueSelectionDialogElement valueSelectionElement( min, max, monsterCount, 1, selectionBoxArea.getPosition() );
        valueSelectionElement.ignoreMouseWheelEventRoiCheck();
        valueSelectionElement.draw( display );

        fheroes2::ButtonGroup btnGroups( box.GetArea(), Dialog::OK | Dialog::CANCEL );
        assert( btnGroups.getButtonsCount() == 2 );

        const auto & buttonOkay = btnGroups.button( 0 );
        const auto & buttonCancel = btnGroups.button( 1 );
        btnGroups.draw();

        const fheroes2::Point minMaxButtonOffset( selectionBoxArea.x + selectionBoxArea.width + 6, selectionBoxArea.y );
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        fheroes2::Button buttonMax( minMaxButtonOffset.x, minMaxButtonOffset.y, isEvilInterface ? ICN::UNIFORM_EVIL_MAX_BUTTON : ICN::UNIFORM_GOOD_MAX_BUTTON, 0, 1 );
        fheroes2::Button buttonMin( minMaxButtonOffset.x, minMaxButtonOffset.y, isEvilInterface ? ICN::UNIFORM_EVIL_MIN_BUTTON : ICN::UNIFORM_GOOD_MIN_BUTTON, 0, 1 );

        switchMaxMinButtons( buttonMin, buttonMax, monsterCount, min );

        display.render();

        const fheroes2::Rect monsterUiRect{ monsterUiOffset, monsterUiElement.area() };

        int result = Dialog::ZERO;
        std::string typedValueBuf;
        LocalEvent & le = LocalEvent::Get();

        while ( result == Dialog::ZERO && le.HandleEvents() ) {
            bool needRedraw = false;

            if ( buttonMax.isVisible() ) {
                buttonMax.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonMax.area() ) );
            }

            if ( buttonMin.isVisible() ) {
                buttonMin.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonMin.area() ) );
            }

            if ( const auto value = fheroes2::processIntegerValueTyping( min, max, typedValueBuf ); value ) {
                valueSelectionElement.setValue( *value );

                monsterCount = valueSelectionElement.getValue();

                needRedraw = true;
            }
            else if ( buttonMax.isVisible() && le.MouseClickLeft( buttonMax.area() ) ) {
                valueSelectionElement.setValue( max );
                typedValueBuf.clear();

                monsterCount = max;

                needRedraw = true;
            }
            else if ( buttonMax.isVisible() && le.isMouseRightButtonPressedInArea( buttonMax.area() ) ) {
                fheroes2::showStandardTextMessage( _( "MAX" ), _( "Click to select the maximum amount." ), Dialog::ZERO );
            }
            else if ( buttonMin.isVisible() && le.MouseClickLeft( buttonMin.area() ) ) {
                valueSelectionElement.setValue( min );
                typedValueBuf.clear();

                monsterCount = min;

                needRedraw = true;
            }
            else if ( buttonMin.isVisible() && le.isMouseRightButtonPressedInArea( buttonMin.area() ) ) {
                fheroes2::showStandardTextMessage( _( "MIN" ), _( "Click to select the minimum amount." ), Dialog::ZERO );
            }
            else if ( valueSelectionElement.processEvents() ) {
                typedValueBuf.clear();

                monsterCount = valueSelectionElement.getValue();

                needRedraw = true;
            }
            else if ( le.isMouseLeftButtonReleasedInArea( monsterUiRect ) || le.isMouseRightButtonPressedInArea( monsterUiRect ) ) {
                monsterUiElement.processEvents( monsterUiOffset );
                needRedraw = true;
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOkay.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to apply the entered number." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else {
                result = btnGroups.processEvents();
            }

            if ( needRedraw ) {
                switchMaxMinButtons( buttonMin, buttonMax, valueSelectionElement.getValue(), min );

                valueSelectionElement.draw( display );
                monsterUiElement.update( display, monsterUiOffset );

                display.render( windowArea );
            }
        }

        if ( result != Dialog::OK ) {
            return false;
        }

        troop = Troop( monsterUiElement.getMonsterType(), monsterCount );

        return true;
    }
}
