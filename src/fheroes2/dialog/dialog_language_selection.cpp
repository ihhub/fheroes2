/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2022                                             *
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
#include "agg_image.h"
#include "cursor.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "interface_list.h"
#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"

#include <algorithm>
#include <cassert>

namespace
{
    const int editBoxLength = 266;

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
            languageName.draw( ( editBoxLength - languageName.width() ) / 2 + offsetX, offsetY, fheroes2::Display::instance() );
        }

        void RedrawBackground( const fheroes2::Point & dst ) override
        {
            const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
            fheroes2::Blit( panel, fheroes2::Display::instance(), dst.x, dst.y );
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

    private:
        bool _isDoubleClicked;
    };

    void redrawDialogInfo( const fheroes2::Rect & windowRoi, const fheroes2::SupportedLanguage & language )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Text title( _( "Select Game Language:" ), fheroes2::FontType::normalYellow() );
        title.draw( windowRoi.x + ( windowRoi.width - title.width() ) / 2, windowRoi.y + 30, display );

        const fheroes2::Text selectedLanguage( fheroes2::getLanguageName( language ), fheroes2::FontType::normalYellow() );
        selectedLanguage.draw( windowRoi.x + ( editBoxLength - selectedLanguage.width() ) / 2 + 41, windowRoi.y + 287 + ( 19 - selectedLanguage.height() + 2 ) / 2,
                               display );
    }

    bool getLanguage( const std::vector<fheroes2::SupportedLanguage> & languages, fheroes2::SupportedLanguage chosenLanguage )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        const fheroes2::Sprite & window = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
        const fheroes2::Sprite & windowShadow = fheroes2::AGG::GetICN( ICN::REQBKG, 1 );

        const fheroes2::Point dialogOffset( ( display.width() - window.width() ) / 2, ( display.height() - window.height() ) / 2 );
        const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

        fheroes2::ImageRestorer restorer( display, shadowOffset.x, shadowOffset.y, window.width() + BORDERWIDTH, window.height() + BORDERWIDTH );
        const fheroes2::Rect roi( dialogOffset.x, dialogOffset.y, window.width(), window.height() );

        fheroes2::Blit( windowShadow, display, roi.x - BORDERWIDTH, roi.y + BORDERWIDTH );

        fheroes2::Button buttonOk( roi.x + 34, roi.y + 315, ICN::REQUEST, 1, 2 );
        fheroes2::Button buttonCancel( roi.x + 244, roi.y + 315, ICN::REQUEST, 3, 4 );

        LanguageList items( roi.getPosition() );

        items.RedrawBackground( roi.getPosition() );
        items.SetScrollButtonUp( ICN::REQUESTS, 5, 6, { roi.x + 327, roi.y + 55 } );
        items.SetScrollButtonDn( ICN::REQUESTS, 7, 8, { roi.x + 327, roi.y + 257 } );

        const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::ESCROLL, 3 );
        const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, 180, 11, static_cast<int32_t>( languages.size() ),
                                                                                   { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );
        items.setScrollBarArea( { roi.x + 328, roi.y + 73, 12, 180 } );
        items.setScrollBarImage( scrollbarSlider );
        items.SetAreaMaxItems( 11 );
        items.SetAreaItems( { roi.x + 41, roi.y + 55 + 3, editBoxLength, 215 } );

        std::vector<fheroes2::SupportedLanguage> temp = languages;
        items.SetListContent( temp );

        const fheroes2::Size currentResolution( display.width(), display.height() );

        fheroes2::Size selectedResolution;
        for ( size_t i = 0; i < languages.size(); ++i ) {
            if ( languages[i] == chosenLanguage ) {
                items.SetCurrent( i );
                break;
            }
        }

        items.Redraw();

        buttonOk.draw();
        buttonCancel.draw();

        redrawDialogInfo( roi, chosenLanguage );

        display.render();

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonOk.area() ) && buttonOk.isEnabled() ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
            le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            items.QueueEventProcessing();

            if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY )
                 || items.isDoubleClicked() ) {
                return true;
            }

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return false;
            }

            if ( le.MousePressRight( buttonCancel.area() ) ) {
                fheroes2::Text header( _( "Cancel" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Exit this menu without doing anything." ), fheroes2::FontType::normalWhite() );
                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( buttonOk.area() ) ) {
                fheroes2::Text header( _( "Okay" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Click to choose the selected language." ), fheroes2::FontType::normalWhite() );
                fheroes2::showMessage( header, body, 0 );
            }

            if ( items.isSelected() ) {
                chosenLanguage = items.GetCurrent();
                Settings::Get().setGameLanguage( fheroes2::getLanguageAbbreviation( chosenLanguage ) );
            }

            if ( !items.IsNeedRedraw() ) {
                continue;
            }

            items.Redraw();
            buttonOk.draw();
            buttonCancel.draw();
            redrawDialogInfo( roi, chosenLanguage );
            display.render();
        }

        return false;
    }
}

namespace fheroes2
{
    void selectLanguage( const std::vector<SupportedLanguage> & languages, const SupportedLanguage currentLanguage )
    {
        if ( languages.empty() ) {
            // Why do you even call this function having 0 languages?
            assert( 0 );
            Settings::Get().setGameLanguage( fheroes2::getLanguageAbbreviation( SupportedLanguage::English ) );
            return;
        }

        if ( languages.size() == 1 ) {
            Settings::Get().setGameLanguage( fheroes2::getLanguageAbbreviation( languages.front() ) );
            return;
        }

        SupportedLanguage chosenLanguage = languages.front();
        for ( const SupportedLanguage language : languages ) {
            if ( currentLanguage == language ) {
                chosenLanguage = currentLanguage;
                break;
            }
        }

        if ( !getLanguage( languages, chosenLanguage ) ) {
            Settings::Get().setGameLanguage( fheroes2::getLanguageAbbreviation( chosenLanguage ) );
        }
    }
}
