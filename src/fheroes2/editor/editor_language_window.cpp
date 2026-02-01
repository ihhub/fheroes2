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

#include "editor_language_window.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_language_selection.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "map_format_helper.h"
#include "map_format_info.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_text.h"
#include "ui_window.h"

namespace fheroes2
{
    enum class SupportedLanguage : uint8_t;
}

namespace
{
    const int32_t elementOffset{ 9 };

    const fheroes2::Size languageArea{ 300, 300 };

    const int32_t listAreaHeightDeduction{ 8 };

    class LanguageListBox final : public Interface::ListBox<fheroes2::SupportedLanguage>
    {
    public:
        using Interface::ListBox<fheroes2::SupportedLanguage>::ListBox;

        using Interface::ListBox<fheroes2::SupportedLanguage>::ActionListDoubleClick;
        using Interface::ListBox<fheroes2::SupportedLanguage>::ActionListSingleClick;
        using Interface::ListBox<fheroes2::SupportedLanguage>::ActionListPressRight;

        void RedrawItem( const fheroes2::SupportedLanguage & language, int32_t posX, int32_t posY, bool current ) override
        {
            const fheroes2::LanguageSwitcher languageSwitcher( language );
            fheroes2::Text text{ fheroes2::getLanguageName( language ), ( current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() ) };
            text.fitToOneRow( languageArea.width - 10 );
            text.draw( posX + 5, posY + 5, fheroes2::Display::instance() );
        }

        void RedrawBackground( const fheroes2::Point & /*unused*/ ) override
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

        void ActionListDoubleClick( fheroes2::SupportedLanguage & /*unused*/ ) override
        {
            _isDoubleClicked = true;
        }

        bool isDoubleClicked() const
        {
            return _isDoubleClicked;
        }

        void ActionListSingleClick( fheroes2::SupportedLanguage & /*unused*/ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( fheroes2::SupportedLanguage & /*unused*/ ) override
        {
            // Do nothing.
        }

        void initListBackgroundRestorer( fheroes2::Rect roi )
        {
            _listBackground = std::make_unique<fheroes2::ImageRestorer>( fheroes2::Display::instance(), roi.x, roi.y, roi.width, roi.height );
        }

    private:
        std::unique_ptr<fheroes2::ImageRestorer> _listBackground;

        bool _isDoubleClicked{ false };
    };
}

namespace Editor
{
    void openLanguageWindow( Maps::Map_Format::MapFormat & mapFormat )
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::StandardWindow background{ languageArea.width + 50, languageArea.height + 100, true, display };

        const fheroes2::Rect windowArea{ background.activeArea() };

        int32_t offsetY = windowArea.y + elementOffset;

        const fheroes2::Text title( _( "Supported languages" ), fheroes2::FontType::normalYellow() );
        title.draw( windowArea.x + ( windowArea.width - title.width() ) / 2, offsetY, display );

        offsetY += title.height() + elementOffset;

        const fheroes2::Rect rumorsRoi{ windowArea.x + elementOffset, offsetY, languageArea.width, languageArea.height };
        background.applyTextBackgroundShading( rumorsRoi );

        LanguageListBox languageList( rumorsRoi.getPosition() );
        languageList.initListBackgroundRestorer( rumorsRoi );

        languageList.SetAreaItems( { rumorsRoi.x, rumorsRoi.y, rumorsRoi.width, rumorsRoi.height - listAreaHeightDeduction } );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        int32_t scrollbarOffsetX = rumorsRoi.x + rumorsRoi.width + 5;
        background.renderScrollbarBackground( { scrollbarOffsetX, rumorsRoi.y, rumorsRoi.width, rumorsRoi.height }, isEvilInterface );

        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;
        const int32_t topPartHeight = 19;
        ++scrollbarOffsetX;

        languageList.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, rumorsRoi.y + 1 } );
        languageList.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, rumorsRoi.y + rumorsRoi.height - 15 } );
        languageList.setScrollBarArea( { scrollbarOffsetX + 2, rumorsRoi.y + topPartHeight, 10, rumorsRoi.height - 2 * topPartHeight } );
        languageList.setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );
        languageList.SetAreaMaxItems( 10 );

        std::vector<fheroes2::SupportedLanguage> languages;
        languages.emplace_back( mapFormat.mainLanguage );
        for ( const auto & [language, info] : mapFormat.translations ) {
            languages.emplace_back( language );
        }

        const std::vector<fheroes2::SupportedLanguage> originalSelection{ languages };

        languageList.SetListContent( languages );
        languageList.SetCurrent( 0 );
        languageList.updateScrollBarImage();

        languageList.Redraw();

        const int minibuttonIcnId = isEvilInterface ? ICN::CELLWIN_EVIL : ICN::CELLWIN;

        const fheroes2::Sprite & buttonImage = fheroes2::AGG::GetICN( minibuttonIcnId, 13 );
        const int32_t buttonWidth = buttonImage.width();

        fheroes2::Button buttonAdd( rumorsRoi.x, rumorsRoi.y + rumorsRoi.height + 5, minibuttonIcnId, 13, 14 );
        buttonAdd.draw();

        fheroes2::Button buttonDelete( rumorsRoi.x + languageArea.width - buttonWidth, rumorsRoi.y + rumorsRoi.height + 5, minibuttonIcnId, 17, 18 );
        buttonDelete.draw();

        // Prepare OKAY and CANCEL buttons and render their shadows.
        fheroes2::Button buttonOk;
        fheroes2::Button buttonCancel;

        background.renderOkayCancelButtons( buttonOk, buttonCancel );

        display.render( background.totalArea() );

        bool isRedrawNeeded = false;

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );
            buttonAdd.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonAdd.area() ) );
            buttonDelete.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonDelete.area() ) );

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                break;
            }

            if ( buttonOk.isEnabled()
                 && ( languageList.isDoubleClicked() || le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) ) {
                assert( !languages.empty() );

                if ( languages == originalSelection && languageList.GetCurrent() == mapFormat.mainLanguage ) {
                    // No changes have been made.
                    return;
                }

                for ( size_t i = 0; i < languages.size(); ++i ) {
                    if ( languageList.getCurrentId() != static_cast<int32_t>( i ) ) {
                        Maps::changeLanguage( mapFormat, languages[i] );
                    }
                }

                // Set the main language of the map.
                Maps::changeLanguage( mapFormat, languageList.GetCurrent() );

                // Remove all remaining languages.
                for ( const auto language : originalSelection ) {
                    if ( std::find( languages.begin(), languages.end(), language ) == languages.end() ) {
                        Maps::removeTranslation( mapFormat, language );
                    }
                }

                return;
            }

            languageList.QueueEventProcessing();

            if ( languageList.IsNeedRedraw() ) {
                languageList.Redraw();
                isRedrawNeeded = true;
            }

            if ( le.MouseClickLeft( buttonAdd.area() ) ) {
                const std::vector<fheroes2::SupportedLanguage> supportedLanguages = fheroes2::getSupportedLanguages();
                const fheroes2::SupportedLanguage language = fheroes2::selectLanguage( supportedLanguages, mapFormat.mainLanguage, false );
                if ( language == mapFormat.mainLanguage ) {
                    continue;
                }

                if ( std::find( languages.begin(), languages.end(), language ) != languages.end() ) {
                    fheroes2::showStandardTextMessage( _( "Language" ), _( "This language already exists in the list." ), Dialog::OK );
                    continue;
                }

                languages.emplace_back( language );

                languageList.updateScrollBarImage();
                languageList.Redraw();
                isRedrawNeeded = true;
            }
            else if ( le.MouseClickLeft( buttonDelete.area() ) ) {
                if ( languages.size() == 1 ) {
                    fheroes2::showStandardTextMessage( _( "Language" ), _( "A map should have at least one language." ), Dialog::OK );
                    continue;
                }

                if ( languageList.getCurrentId() < 0 ) {
                    continue;
                }

                languageList.RemoveSelected();
                languageList.SetCurrent( 0 );
                languageList.updateScrollBarImage();
                languageList.Redraw();
                isRedrawNeeded = true;
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to apply changes." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonAdd.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Add Language" ), _( "Add an additional language." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonDelete.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Delete Language" ), _( "Delete an existing language." ), Dialog::ZERO );
            }

            if ( isRedrawNeeded ) {
                isRedrawNeeded = false;

                display.render( windowArea );
            }
        }
    }
}
