/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include "dialog_language_selection.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "game_language.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    const int32_t verticalPaddingAreasHight = 30;
    const int32_t textAreaWidth = 270;
    const int32_t scrollBarAreaWidth = 48;
    const int32_t paddingLeftSide = 24;

    class LanguageList : public Interface::ListBox<fheroes2::SupportedLanguage>
    {
    public:
        using Interface::ListBox<fheroes2::SupportedLanguage>::ActionListSingleClick;
        using Interface::ListBox<fheroes2::SupportedLanguage>::ActionListPressRight;
        using Interface::ListBox<fheroes2::SupportedLanguage>::ActionListDoubleClick;

        explicit LanguageList( const fheroes2::Point & offset )
            : Interface::ListBox<fheroes2::SupportedLanguage>( offset )
            , _isDoubleClicked( false )
        {
            // Do nothing.
        }

        void RedrawItem( const fheroes2::SupportedLanguage & language, int32_t offsetX, int32_t offsetY, bool isSelected ) override
        {
            fheroes2::LanguageSwitcher languageSwitcher( language );
            const fheroes2::Text languageName( fheroes2::getLanguageName( language ),
                                               isSelected ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
            languageName.draw( ( textAreaWidth - languageName.width() ) / 2 + offsetX, offsetY, fheroes2::Display::instance() );
        }

        void RedrawBackground( const fheroes2::Point & /* unused */ ) override
        {
            _listBackground->restore();
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListSingleClick( fheroes2::SupportedLanguage & /*language*/ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( fheroes2::SupportedLanguage & /*language*/ ) override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( fheroes2::SupportedLanguage & /*language*/ ) override
        {
            _isDoubleClicked = true;
        }

        bool isDoubleClicked() const
        {
            return _isDoubleClicked;
        }

        int getCurrentId() const
        {
            return _currentId;
        }

        void initListBackgroundRestorer( fheroes2::Rect roi )
        {
            _listBackground = std::make_unique<fheroes2::ImageRestorer>( fheroes2::Display::instance(), roi.x, roi.y, roi.width, roi.height );
        }

        void updateScrollBarImage()
        {
            const int32_t scrollBarWidth = _scrollbar.width();

            setScrollBarImage( fheroes2::generateScrollbarSlider( _scrollbar, false, _scrollbar.getArea().height, VisibleItemCount(), _size(),
                                                                  { 0, 0, scrollBarWidth, 8 }, { 0, 7, scrollBarWidth, 8 } ) );
            _scrollbar.moveToIndex( _topId );
        }

    private:
        bool _isDoubleClicked;
        std::unique_ptr<fheroes2::ImageRestorer> _listBackground;
    };

    void redrawDialogInfo( const fheroes2::Rect & listRoi, const fheroes2::SupportedLanguage language, const bool isGameLanguage )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::FontType fontType = fheroes2::FontType::normalYellow();

        const fheroes2::Text title( isGameLanguage ? _( "Select Game Language:" ) : _( "Select Language:" ), fontType );
        title.draw( listRoi.x + ( listRoi.width - title.width() ) / 2, listRoi.y - ( verticalPaddingAreasHight + title.height() + 2 ) / 2, display );

        const fheroes2::LanguageSwitcher languageSwitcher( language );

        const fheroes2::Text selectedLanguage( fheroes2::getLanguageName( language ), fontType );

        selectedLanguage.draw( listRoi.x + ( listRoi.width - selectedLanguage.width() ) / 2, listRoi.y + listRoi.height + 12 + ( 21 - selectedLanguage.height() ) / 2 + 2,
                               display );
    }

    bool getLanguage( const std::vector<fheroes2::SupportedLanguage> & languages, fheroes2::SupportedLanguage & chosenLanguage, const bool isGameLanguage )
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        const int32_t listHeightDeduction = 112;
        const int32_t listAreaOffsetY = 3;
        const int32_t listAreaHeightDeduction = 4;

        // If we don't have many languages, we reduce the maximum dialog height,
        // but not less than enough for 11 elements.
        // We also limit the maximum list height to 22 lines.
        const int32_t maxDialogHeight = fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) * std::clamp( static_cast<int32_t>( languages.size() ), 11, 22 )
                                        + listAreaOffsetY + listAreaHeightDeduction + listHeightDeduction;

        fheroes2::Display & display = fheroes2::Display::instance();

        // Dialog height is also capped with the current screen height.
        fheroes2::StandardWindow background( paddingLeftSide + textAreaWidth + scrollBarAreaWidth + 3, std::min( display.height() - 100, maxDialogHeight ), true,
                                             display );

        const fheroes2::Rect roi( background.activeArea() );
        const fheroes2::Rect listRoi( roi.x + paddingLeftSide, roi.y + 37, textAreaWidth, roi.height - listHeightDeduction );

        // We divide the list: language list and selected language.
        const fheroes2::Rect selectedLangRoi( listRoi.x, listRoi.y + listRoi.height + 12, listRoi.width, 21 );
        background.applyTextBackgroundShading( selectedLangRoi );
        background.applyTextBackgroundShading( { listRoi.x, listRoi.y, listRoi.width, listRoi.height } );

        fheroes2::ImageRestorer titleBackground( fheroes2::Display::instance(), roi.x, listRoi.y - verticalPaddingAreasHight, roi.width, verticalPaddingAreasHight );
        fheroes2::ImageRestorer selectedLangBackground( fheroes2::Display::instance(), selectedLangRoi.x, selectedLangRoi.y, listRoi.width, selectedLangRoi.height );
        fheroes2::ImageRestorer buttonsBackground( fheroes2::Display::instance(), roi.x, selectedLangRoi.y + selectedLangRoi.height + 10, roi.width,
                                                   verticalPaddingAreasHight );

        LanguageList listBox( roi.getPosition() );

        listBox.initListBackgroundRestorer( listRoi );

        Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        // Prepare OKAY and CANCEL buttons and render their shadows.
        fheroes2::Button buttonOk;
        fheroes2::Button buttonCancel;
        background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );

        listBox.SetAreaItems( { listRoi.x, listRoi.y + 3, listRoi.width - 3, listRoi.height - 4 } );

        int32_t scrollbarOffsetX = roi.x + roi.width - 35;
        background.renderScrollbarBackground( { scrollbarOffsetX, listRoi.y, listRoi.width, listRoi.height }, isEvilInterface );

        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;
        const int32_t topPartHeight = 19;
        ++scrollbarOffsetX;

        listBox.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, listRoi.y + 1 } );
        listBox.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, listRoi.y + listRoi.height - 15 } );
        listBox.setScrollBarArea( { scrollbarOffsetX + 2, listRoi.y + topPartHeight, 10, listRoi.height - 2 * topPartHeight } );
        listBox.setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );
        listBox.SetAreaMaxItems( ( listRoi.height - 7 ) / fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) );
        std::vector<fheroes2::SupportedLanguage> temp = languages;
        listBox.SetListContent( temp );
        listBox.updateScrollBarImage();

        for ( size_t i = 0; i < languages.size(); ++i ) {
            if ( languages[i] == chosenLanguage ) {
                listBox.SetCurrent( i );
                break;
            }
        }

        listBox.Redraw();

        redrawDialogInfo( listRoi, chosenLanguage, isGameLanguage );

        display.render( background.totalArea() );

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            le.isMouseLeftButtonPressedInArea( buttonOk.area() ) && buttonOk.isEnabled() ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
            le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            if ( le.isMouseRightButtonPressedInArea( listRoi ) ) {
                continue;
            }

            const int listId = listBox.getCurrentId();
            listBox.QueueEventProcessing();
            const bool needRedraw = listId != listBox.getCurrentId();

            if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY )
                 || listBox.isDoubleClicked() ) {
                return true;
            }

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return false;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to choose the selected language." ), Dialog::ZERO );
            }

            if ( !listBox.IsNeedRedraw() ) {
                continue;
            }

            if ( needRedraw ) {
                const fheroes2::SupportedLanguage newChosenLanguage = listBox.GetCurrent();
                if ( newChosenLanguage != chosenLanguage ) {
                    chosenLanguage = newChosenLanguage;

                    if ( isGameLanguage ) {
                        conf.setGameLanguage( fheroes2::getLanguageAbbreviation( chosenLanguage ) );
                    }

                    titleBackground.restore();
                    selectedLangBackground.restore();
                    redrawDialogInfo( listRoi, chosenLanguage, isGameLanguage );
                    buttonsBackground.restore();
                    background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );
                }
            }

            listBox.Redraw();
            display.render( roi );
        }

        return false;
    }
}

namespace fheroes2
{
    SupportedLanguage selectLanguage( const std::vector<SupportedLanguage> & languages, const SupportedLanguage currentLanguage, const bool isGameLanguage )
    {
        if ( languages.empty() ) {
            // Why do you even call this function having 0 languages?
            assert( 0 );
            Settings::Get().setGameLanguage( fheroes2::getLanguageAbbreviation( SupportedLanguage::English ) );
            return SupportedLanguage::English;
        }

        if ( languages.size() == 1 ) {
            Settings::Get().setGameLanguage( fheroes2::getLanguageAbbreviation( languages.front() ) );
            return languages.front();
        }

        SupportedLanguage chosenLanguage = languages.front();
        for ( const SupportedLanguage language : languages ) {
            if ( currentLanguage == language ) {
                chosenLanguage = currentLanguage;
                break;
            }
        }

        if ( !getLanguage( languages, chosenLanguage, isGameLanguage ) ) {
            if ( isGameLanguage ) {
                Settings::Get().setGameLanguage( fheroes2::getLanguageAbbreviation( currentLanguage ) );
            }

            return currentLanguage;
        }

        return chosenLanguage;
    }
}
