/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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
#include "game.h"
#include "icn.h"
#include "localevent.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_text.h"
#include "ui_window.h"

#include <algorithm>
#include <cassert>

namespace fheroes2
{
    SupportedLanguage selectLanguage( const std::vector<SupportedLanguage> & languages, const SupportedLanguage currentLanguage )
    {
        if ( languages.empty() ) {
            // Why do you even call this function having 0 languages?
            assert( 0 );
            return SupportedLanguage::English;
        }

        if ( languages.size() == 1 ) {
            return languages.front();
        }

        size_t selectionId = 0;
        auto currentLanguageIt = std::find( languages.begin(), languages.end(), currentLanguage );
        if ( currentLanguageIt != languages.end() ) {
            selectionId = static_cast<size_t>( currentLanguageIt - languages.begin() );
        }

        const int32_t languageAreaWidth = 200;
        const int32_t languageAreaHeight = 25;
        const int32_t offsetFromBorders = 10;
        const int32_t offsetFromButton = 15;

        const int32_t languageCount = static_cast<int32_t>( languages.size() );

        Display & display = Display::instance();

        const int okIcnId = Settings::Get().ExtGameEvilInterface() ? ICN::NON_UNIFORM_EVIL_OKAY_BUTTON : ICN::NON_UNIFORM_GOOD_OKAY_BUTTON;
        const Sprite & buttonOkayImage = AGG::GetICN( okIcnId, 0 );

        StandardWindow window( languageAreaWidth + 2 * offsetFromBorders, 90 + languageAreaHeight * languageCount, display );
        const Rect windowRoi = window.activeArea();

        ButtonSprite okayButton
            = makeButtonWithShadow( windowRoi.x + ( windowRoi.width - buttonOkayImage.width() ) / 2, windowRoi.y + windowRoi.height - 10 - buttonOkayImage.height(),
                                    buttonOkayImage, AGG::GetICN( okIcnId, 1 ), display );

        const Sprite & unselectedButtonSprite = AGG::GetICN( ICN::CELLWIN, 4 );
        const Sprite & selectionSprite = AGG::GetICN( ICN::CELLWIN, 5 );

        Sprite selectedButtonSprite = unselectedButtonSprite;
        Blit( selectionSprite, 0, 0, selectedButtonSprite, selectionSprite.x(), selectionSprite.y(), selectionSprite.width(), selectionSprite.height() );

        ButtonGroup buttonGroup;
        for ( int32_t i = 0; i < languageCount; ++i ) {
            buttonGroup.createButton( windowRoi.x + offsetFromBorders + unselectedButtonSprite.width() / 2, windowRoi.y + 40 + languageAreaHeight * i,
                                      unselectedButtonSprite, selectedButtonSprite, i );
        }

        OptionButtonGroup optionButtonGroup;
        for ( size_t i = 0; i < languages.size(); ++i ) {
            optionButtonGroup.addButton( &buttonGroup.button( i ) );
        }

        window.render();
        okayButton.draw();

        buttonGroup.button( selectionId ).press();
        optionButtonGroup.draw();

        const Text title( _( "Choose game language:" ), FontType::normalYellow() );
        title.draw( windowRoi.x + ( windowRoi.width - title.width() ) / 2, windowRoi.y + 10, display );

        for ( int32_t i = 0; i < languageCount; ++i ) {
            fheroes2::LanguageSwitcher languageSwitcher( languages[i] );
            const Text languageName( getLanguageName( languages[i] ), FontType::normalWhite() );
            languageName.draw( windowRoi.x + offsetFromBorders + selectedButtonSprite.width() + offsetFromButton,
                               windowRoi.y + 40 + languageAreaHeight * i + 2 + ( selectedButtonSprite.height() - languageName.height() ) / 2, display );
        }

        display.render();

        SupportedLanguage chosenLanguage = languages[selectionId];

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

            if ( le.MouseClickLeft( okayButton.area() ) || HotKeyCloseWindow ) {
                break;
            }

            for ( size_t i = 0; i < languages.size(); ++i ) {
                if ( le.MousePressLeft( languageArea[i] ) ) {
                    buttonGroup.button( i ).press();
                    optionButtonGroup.draw();
                    chosenLanguage = languages[i];
                    break;
                }
            }
        }

        return chosenLanguage;
    }
}
