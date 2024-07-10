/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include "editor_ui_helper.h"

#include <utility>

#include "agg_image.h"
#include "color.h"
#include "icn.h"
#include "image.h"
#include "resource.h"
#include "screen.h"
#include "tools.h"
#include "translations.h"
#include "ui_text.h"
#include "ui_tool.h"

namespace Editor
{
    Checkbox::Checkbox( const int32_t x, const int32_t y, const int boxColor, const bool checked, fheroes2::Image & output )
        : _color( boxColor )
        , _checkmark( fheroes2::AGG::GetICN( ICN::CELLWIN, 2 ) )
    {
        const int32_t icnIndex = Color::GetIndex( _color ) + 43;
        const fheroes2::Sprite & playerIcon = fheroes2::AGG::GetICN( ICN::CELLWIN, icnIndex );

        _area = { x, y, playerIcon.width(), playerIcon.height() };

        fheroes2::Copy( playerIcon, 0, 0, output, _area.x, _area.y, _area.width, _area.height );

        _checkmark.setPosition( _area.x + 2, _area.y + 2 );

        if ( checked ) {
            _checkmark.show();
        }
        else {
            _checkmark.hide();
        }
    }

    bool Checkbox::toggle()
    {
        _checkmark.isHidden() ? _checkmark.show() : _checkmark.hide();

        fheroes2::Display::instance().render( _area );

        return !_checkmark.isHidden();
    }

    fheroes2::Rect drawCheckboxWithText( fheroes2::MovableSprite & checkSprite, std::string str, fheroes2::Image & output, const int32_t posX, const int32_t posY,
                                         const bool isEvil )
    {
        const fheroes2::Sprite & checkboxBackground = fheroes2::AGG::GetICN( isEvil ? ICN::CELLWIN_EVIL : ICN::CELLWIN, 1 );
        fheroes2::Copy( checkboxBackground, 0, 0, output, posX, posY, checkboxBackground.width(), checkboxBackground.height() );

        fheroes2::addGradientShadow( checkboxBackground, output, { posX, posY }, { -4, 4 } );
        const fheroes2::Text checkboxText( std::move( str ), fheroes2::FontType::normalWhite() );
        checkboxText.draw( posX + 23, posY + 4, output );

        checkSprite = fheroes2::AGG::GetICN( ICN::CELLWIN, 2 );
        checkSprite.setPosition( posX + 2, posY + 2 );

        return { posX, posY, 23 + checkboxText.width(), checkboxBackground.height() };
    }

    void renderResources( const Funds & resources, const fheroes2::Rect & roi, fheroes2::Image & output, std::array<fheroes2::Rect, 7> & resourceRoi )
    {
        const int32_t offsetFromEdge{ 7 };

        // Maximum width is 39 pixels (except gold), maximum height is 32 pixels
        const int32_t maxWidth = 39;
        const int32_t maxHeight = 32;

        const int32_t midElementOffsetX = ( ( roi.width - 2 * offsetFromEdge ) - maxWidth * 4 ) / 3;

        const int32_t firstColumnOffset = roi.x + offsetFromEdge;
        const int32_t columnStep = maxWidth + midElementOffsetX;
        const int32_t secondColumnOffset = firstColumnOffset + columnStep;
        const int32_t thirdColumnOffset = secondColumnOffset + columnStep;
        const int32_t forthColumnOffset = thirdColumnOffset + columnStep;

        const fheroes2::FontType fontType( fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE );
        const int32_t fontHeight = fheroes2::Text( std::string(), fontType ).height();

        const std::array<int32_t, 2> offsetY = { roi.y + offsetFromEdge + maxHeight, roi.y + offsetFromEdge + 2 * maxHeight + fontHeight + 2 };

        const fheroes2::Sprite & woodImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 0 );
        const fheroes2::Sprite & mercuryImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 1 );
        const fheroes2::Sprite & oreImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 2 );
        const fheroes2::Sprite & sulfurImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 3 );
        const fheroes2::Sprite & crystalImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 4 );
        const fheroes2::Sprite & gemsImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 5 );
        const fheroes2::Sprite & goldImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 6 );

        resourceRoi[0] = { firstColumnOffset + ( maxWidth - woodImage.width() ) / 2, offsetY[0] - woodImage.height(), woodImage.width(), woodImage.height() };
        resourceRoi[1] = { secondColumnOffset + ( maxWidth - sulfurImage.width() ) / 2, offsetY[0] - sulfurImage.height(), sulfurImage.width(), sulfurImage.height() };
        resourceRoi[2] = { thirdColumnOffset + ( maxWidth - crystalImage.width() ) / 2, offsetY[0] - crystalImage.height(), crystalImage.width(), crystalImage.height() };
        resourceRoi[3] = { forthColumnOffset + ( maxWidth - mercuryImage.width() ) / 2, offsetY[0] - mercuryImage.height(), mercuryImage.width(), mercuryImage.height() };
        resourceRoi[4] = { firstColumnOffset + ( maxWidth - oreImage.width() ) / 2, offsetY[1] - oreImage.height(), oreImage.width(), oreImage.height() };
        resourceRoi[5] = { secondColumnOffset + ( maxWidth - gemsImage.width() ) / 2, offsetY[1] - gemsImage.height(), gemsImage.width(), gemsImage.height() };
        resourceRoi[6] = { thirdColumnOffset + ( maxWidth * 2 + midElementOffsetX - goldImage.width() ) / 2, offsetY[1] - goldImage.height(), goldImage.width(),
                           goldImage.height() };

        fheroes2::Blit( woodImage, output, resourceRoi[0].x, resourceRoi[0].y );
        fheroes2::Blit( sulfurImage, output, resourceRoi[1].x, resourceRoi[1].y );
        fheroes2::Blit( crystalImage, output, resourceRoi[2].x, resourceRoi[2].y );
        fheroes2::Blit( mercuryImage, output, resourceRoi[3].x, resourceRoi[3].y );
        fheroes2::Blit( oreImage, output, resourceRoi[4].x, resourceRoi[4].y );
        fheroes2::Blit( gemsImage, output, resourceRoi[5].x, resourceRoi[5].y );
        fheroes2::Blit( goldImage, output, resourceRoi[6].x, resourceRoi[6].y );

        fheroes2::Text text( std::to_string( resources.wood ), fontType );
        text.draw( firstColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[0] + 1, output );

        text.set( std::to_string( resources.sulfur ), fontType );
        text.draw( secondColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[0] + 1, output );

        text.set( std::to_string( resources.crystal ), fontType );
        text.draw( thirdColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[0] + 1, output );

        text.set( std::to_string( resources.mercury ), fontType );
        text.draw( forthColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[0] + 1, output );

        text.set( std::to_string( resources.ore ), fontType );
        text.draw( firstColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[1] + 1, output );

        text.set( std::to_string( resources.gems ), fontType );
        text.draw( secondColumnOffset + ( maxWidth - text.width() ) / 2, offsetY[1] + 1, output );

        text.set( std::to_string( resources.gold ), fontType );
        text.draw( thirdColumnOffset + ( maxWidth * 2 + midElementOffsetX - text.width() ) / 2, offsetY[1] + 1, output );
    }

    std::string getDateDescription( const int32_t day )
    {
        std::string message = _( "Day: %{day} Week: %{week} Month: %{month}" );
        int32_t days = day - 1;
        const int32_t month = days / ( 7 * 4 );
        days -= month * ( 7 * 4 );

        StringReplace( message, "%{day}", ( days % 7 ) + 1 );
        StringReplace( message, "%{week}", ( days / 7 ) + 1 );
        StringReplace( message, "%{month}", month + 1 );

        return message;
    }
}
