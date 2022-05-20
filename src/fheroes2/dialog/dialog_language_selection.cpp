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
#include "game_hotkeys.h"
#include "icn.h"
#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_text.h"
#include "ui_window.h"

#include <algorithm>
#include <cassert>

namespace
{
    bool getLanguage( const std::vector<fheroes2::SupportedLanguage> & languages, fheroes2::SupportedLanguage & chosenLanguage )
    {
        size_t selectionId = 0;
        auto currentLanguageIt = std::find( languages.begin(), languages.end(), chosenLanguage );
        if ( currentLanguageIt != languages.end() ) {
            selectionId = static_cast<size_t>( currentLanguageIt - languages.begin() );
        }

        const int32_t languageAreaWidth = 200;
        const int32_t languageAreaHeight = 25;
        const int32_t offsetFromBorders = 10;
        const int32_t offsetFromButton = 15;

        const int32_t languageCount = static_cast<int32_t>( languages.size() );

        fheroes2::Display & display = fheroes2::Display::instance();

        const int okIcnId = Settings::Get().ExtGameEvilInterface() ? ICN::NON_UNIFORM_EVIL_OKAY_BUTTON : ICN::NON_UNIFORM_GOOD_OKAY_BUTTON;
        const fheroes2::Sprite & buttonOkayImage = fheroes2::AGG::GetICN( okIcnId, 0 );

        fheroes2::StandardWindow window( languageAreaWidth + 2 * offsetFromBorders, 90 + languageAreaHeight * languageCount, display );
        const fheroes2::Rect windowRoi = window.activeArea();

        fheroes2::ButtonSprite okayButton
            = makeButtonWithShadow( windowRoi.x + ( windowRoi.width - buttonOkayImage.width() ) / 2, windowRoi.y + windowRoi.height - 10 - buttonOkayImage.height(),
                                    buttonOkayImage, fheroes2::AGG::GetICN( okIcnId, 1 ), display );

        const fheroes2::Sprite & unselectedButtonSprite = fheroes2::AGG::GetICN( ICN::CELLWIN, 4 );
        const fheroes2::Sprite & selectionSprite = fheroes2::AGG::GetICN( ICN::CELLWIN, 5 );

        fheroes2::Sprite selectedButtonSprite = unselectedButtonSprite;
        Blit( selectionSprite, 0, 0, selectedButtonSprite, selectionSprite.x(), selectionSprite.y(), selectionSprite.width(), selectionSprite.height() );

        fheroes2::ButtonGroup buttonGroup;
        for ( int32_t i = 0; i < languageCount; ++i ) {
            buttonGroup.createButton( windowRoi.x + offsetFromBorders + unselectedButtonSprite.width() / 2, windowRoi.y + 40 + languageAreaHeight * i,
                                      unselectedButtonSprite, selectedButtonSprite, i );
        }

        fheroes2::OptionButtonGroup optionButtonGroup;
        for ( size_t i = 0; i < languages.size(); ++i ) {
            optionButtonGroup.addButton( &buttonGroup.button( i ) );
        }

        window.render();
        okayButton.draw();

        buttonGroup.button( selectionId ).press();
        optionButtonGroup.draw();

        const fheroes2::Text title( _( "Select Game Language:" ), fheroes2::FontType::normalYellow() );
        title.draw( windowRoi.x + ( windowRoi.width - title.width() ) / 2, windowRoi.y + 10, display );

        for ( int32_t i = 0; i < languageCount; ++i ) {
            fheroes2::LanguageSwitcher languageSwitcher( languages[i] );
            const fheroes2::Text languageName( getLanguageName( languages[i] ), fheroes2::FontType::normalWhite() );
            languageName.draw( windowRoi.x + offsetFromBorders + selectedButtonSprite.width() + offsetFromButton,
                               windowRoi.y + 40 + languageAreaHeight * i + 2 + ( selectedButtonSprite.height() - languageName.height() ) / 2, display );
        }

        display.render();

        chosenLanguage = languages[selectionId];

        std::vector<fheroes2::Rect> languageArea( languages.size() );
        for ( size_t i = 0; i < languages.size(); ++i ) {
            languageArea[i] = buttonGroup.button( i ).area();
            languageArea[i].width += languageAreaWidth + offsetFromButton;
        }

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            if ( le.MousePressLeft( okayButton.area() ) ) {
                okayButton.drawOnPress();
            }
            else {
                okayButton.drawOnRelease();
            }

            if ( le.MouseClickLeft( okayButton.area() ) || Game::HotKeyCloseWindow() ) {
                return false;
            }

            for ( size_t i = 0; i < languages.size(); ++i ) {
                if ( le.MousePressLeft( languageArea[i] ) ) {
                    buttonGroup.button( i ).press();
                    optionButtonGroup.draw();
                    chosenLanguage = languages[i];
                    return true;
                }
            }
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

        size_t selectionId = 0;
        auto currentLanguageIt = std::find( languages.begin(), languages.end(), currentLanguage );
        if ( currentLanguageIt != languages.end() ) {
            selectionId = static_cast<size_t>( currentLanguageIt - languages.begin() );
        }

        SupportedLanguage chosenLanguage = languages[selectionId];

        while ( getLanguage( languages, chosenLanguage ) ) {
            Settings::Get().setGameLanguage( fheroes2::getLanguageAbbreviation( chosenLanguage ) );
        }
    }
}
