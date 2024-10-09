/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include "game_credits.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "audio.h"
#include "audio_manager.h"
#include "cursor.h"
#include "game_delays.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "mus.h"
#include "pal.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_text.h"
#include "ui_tool.h"

namespace
{
    void transformToBlack( fheroes2::Image & out )
    {
        if ( out.empty() ) {
            return;
        }

        assert( !out.singleLayer() );

        uint8_t * image = out.image();
        const uint8_t * transform = out.transform();
        const uint8_t * imageEnd = image + out.width() * out.height();

        for ( ; image != imageEnd; ++image, ++transform ) {
            if ( *transform == 0 ) {
                *image = 0;
            }
        }
    }

    enum class AnimationState : int
    {
        FADING_IN,
        NO_ACTION,
        FADING_OUT
    };

    class AnimationSequence
    {
    public:
        AnimationSequence() = delete;
        explicit AnimationSequence( const int32_t imageCount )
            : _imageCount( imageCount )
        {}

        int32_t pageId() const
        {
            return _pageId;
        }

        uint8_t alpha() const
        {
            return static_cast<uint8_t>( _alphaValue );
        }

        AnimationState state() const
        {
            return _animationState;
        }

        void increment()
        {
            switch ( _animationState ) {
            case AnimationState::FADING_IN:
                _alphaValue += ALPHA_VALUE_STEP;
                if ( _alphaValue > 255 ) {
                    _alphaValue = 255;
                    _animationState = AnimationState::NO_ACTION;
                    _noActionCounter = 0;
                }
                break;
            case AnimationState::NO_ACTION:
                ++_noActionCounter;
                if ( _noActionCounter > NO_ACTION_COUNTER_LIMIT ) {
                    _animationState = AnimationState::FADING_OUT;
                }
                break;
            case AnimationState::FADING_OUT:
                _alphaValue -= ALPHA_VALUE_STEP;
                if ( _alphaValue < 0 ) {
                    _alphaValue = 0;
                    _animationState = AnimationState::FADING_IN;

                    ++_pageId;
                    if ( _pageId >= _imageCount ) {
                        _pageId = 0;
                    }
                }

                break;
            default:
                assert( 0 );
                break;
            }
        }

    private:
        const int32_t _imageCount;
        int32_t _alphaValue = 0;
        int32_t _noActionCounter = 0;
        int32_t _pageId = 0;

        AnimationState _animationState = AnimationState::FADING_IN;

        enum : int32_t
        {
            ALPHA_VALUE_STEP = 5,
            NO_ACTION_COUNTER_LIMIT = 100
        };
    };

    fheroes2::Sprite generateHeader()
    {
        const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::CBKGLAVA, 0 );
        assert( background.height() < fheroes2::Display::DEFAULT_HEIGHT );

        fheroes2::Sprite output;
        output._disableTransformLayer();
        output.resize( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT - background.height() );
        output.fill( 0 );

        const fheroes2::Text caption( "fheroes2 engine (" + Settings::GetVersion() + ")", fheroes2::FontType::normalYellow() );
        caption.draw( output.width() / 2 - caption.width() / 2, 17, output );

        return output;
    }

    int32_t renderText( fheroes2::Image & output, const int32_t offsetX, const int32_t offsetY, const int32_t textWidth, const char * titleText, const char * bodyText )
    {
        const fheroes2::Text title( titleText, fheroes2::FontType::normalYellow() );
        const fheroes2::Text name( bodyText, fheroes2::FontType::normalWhite() );

        title.draw( offsetX, offsetY, textWidth, output );
        const int32_t titleHeight = title.height( textWidth );

        name.draw( offsetX, offsetY + titleHeight, textWidth, output );

        return name.height( textWidth ) + titleHeight;
    }

    fheroes2::Sprite generateResurrectionCreditsFirstPage()
    {
        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::CBKGLAVA, 0 );
        output._disableTransformLayer();

        const int32_t columnStep = 210;
        const int32_t textInitialOffsetY = 42;
        const int32_t textWidth = 200;

        int32_t offsetY = textInitialOffsetY;
        int32_t offsetX = 0;

        offsetY += renderText( output, offsetX, offsetY, textWidth, _( "Project Coordination and Core Development" ), "Ihar Hubchyk" );
        offsetY += 10;

        const fheroes2::Sprite & blackDragon = fheroes2::AGG::GetICN( ICN::DRAGBLAK, 5 );
        fheroes2::Blit( blackDragon, output, ( columnStep - blackDragon.width() ) / 2, offsetY );
        offsetY += blackDragon.height() + 50;

        const int32_t secondAuthorLayerY = offsetY;

        offsetY += renderText( output, offsetX, offsetY, textWidth, _( "Development" ), "Sergei Ivanov" );
        offsetY += 10;

        const fheroes2::Sprite & minotaur = fheroes2::AGG::GetICN( ICN::MINOTAUR, 14 );
        fheroes2::Blit( minotaur, output, ( columnStep - minotaur.width() ) / 2, offsetY );
        offsetY += minotaur.height();

        offsetY += 40;

        const fheroes2::Text websiteInto( _( "Visit us at " ), fheroes2::FontType::normalWhite() );
        const fheroes2::Text website( "https://github.com/ihhub/fheroes2", fheroes2::FontType::normalYellow() );

        const int32_t websiteIntoWidth = websiteInto.width();
        const int32_t websiteWidth = website.width();
        const int32_t websiteHeight = website.height();
        const int32_t websiteOffsetX = ( output.width() - websiteIntoWidth - websiteWidth ) / 2;
        websiteInto.draw( websiteOffsetX, offsetY, output );
        website.draw( websiteOffsetX + websiteIntoWidth, offsetY, output );

        const fheroes2::Sprite & missile = fheroes2::AGG::GetICN( ICN::ARCH_MSL, 4 );
        fheroes2::Blit( missile, output, websiteOffsetX - 10 - missile.width(), offsetY + websiteHeight / 2 - missile.height() / 2 );
        fheroes2::Blit( missile, output, websiteOffsetX + websiteIntoWidth + websiteWidth + 10, offsetY + websiteHeight / 2 - missile.height() / 2, true );

        offsetY = textInitialOffsetY;
        offsetX += columnStep;

        offsetY += renderText( output, offsetX, offsetY, textWidth, _( "QA and Support" ), "Igor Tsivilko" );
        offsetY += 10;

        const fheroes2::Sprite & cyclop = fheroes2::AGG::GetICN( ICN::CYCLOPS, 38 );
        fheroes2::Blit( cyclop, output, offsetX + ( columnStep - cyclop.width() ) / 2, offsetY );

        offsetY = secondAuthorLayerY;

        offsetY += renderText( output, offsetX, offsetY, textWidth, _( "Development" ), "Ivan Shibanov" );
        offsetY += 10;

        const fheroes2::Sprite & crusader = fheroes2::AGG::GetICN( ICN::PALADIN2, 23 );
        fheroes2::Blit( crusader, output, offsetX + ( columnStep - crusader.width() ) / 2, offsetY );

        offsetY = textInitialOffsetY;
        offsetX += columnStep;

        offsetY += renderText( output, offsetX, offsetY, textWidth, _( "Development" ), "Oleg Derevenetz" );
        offsetY += 10;

        const fheroes2::Sprite & mage = fheroes2::AGG::GetICN( ICN::MAGE1, 24 );
        fheroes2::Blit( mage, output, offsetX + ( columnStep - mage.width() ) / 2, offsetY );

        offsetY = secondAuthorLayerY;

        offsetY += renderText( output, offsetX, offsetY, textWidth, _( "Dev and Support" ), "Zense" );
        offsetY += 10;

        const fheroes2::Sprite & phoenix = fheroes2::AGG::GetICN( ICN::PHOENIX, 4 );
        fheroes2::Blit( phoenix, output, offsetX + ( columnStep - phoenix.width() ) / 2, offsetY - 10 );

        const fheroes2::Sprite & goblin = fheroes2::AGG::GetICN( ICN::GOBLIN, 27 );
        fheroes2::Blit( goblin, output, output.width() - goblin.width() * 2, output.height() - goblin.height() - 10, true );

        return output;
    }

    fheroes2::Sprite generateResurrectionCreditsSecondPage()
    {
        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::CBKGLAVA, 0 );
        output._disableTransformLayer();

        const int32_t columnStep = 210;
        const int32_t textInitialOffsetX = output.width() / 2;
        const int32_t textInitialOffsetY = 60;
        const int32_t textWidth = 300;

        int32_t offsetY = textInitialOffsetY;

        const fheroes2::Text title( _( "Special Thanks to" ), fheroes2::FontType::normalYellow() );
        title.draw( textInitialOffsetX - title.width() / 2, offsetY, output );
        offsetY += title.height() + 5;

        std::string contributors( "LeHerosInconnu\n"
                                  "undef21\n"
                                  "shprotru\n"
                                  "Arkadiy Illarionov\n"
                                  "a1exsh\n"
                                  "vincent-grosbois\n"
                                  "eos428\n"
                                  "Mr-Bajs\n"
                                  "Arthusppp\n"
                                  "felix642\n"
                                  "Vasilenko Alexey\n"
                                  "Andrii Kurdiumov\n"
                                  "Stisen1\n"
                                  "dimag0g\n"
                                  "Effektus\n"
                                  "Laserlicht\n"
                                  "Mauri Mustonen\n"
                                  "tau3\n" );

        fheroes2::Text name( std::move( contributors ), fheroes2::FontType::normalWhite() );
        const int32_t constributorsHeight = name.height( textWidth );
        name.draw( ( columnStep - textWidth ) / 2, offsetY, textWidth, output );

        std::string supporters( "Aimi Lindschouw\n"
                                "Aleksei Mazur\n"
                                "Alex Perry\n"
                                "Andrei Dyldin\n"
                                "Andrew Lasinskiy\n"
                                "Andrew Szucs\n"
                                "Benjamin Hughes\n"
                                "Bolsch\n"
                                "Brandon Wright\n"
                                "Connor Townsend\n"
                                "Christophe Didion\n"
                                "Christopher Elliott" );

        name.set( std::move( supporters ), fheroes2::FontType::normalWhite() );
        name.draw( columnStep + ( columnStep - textWidth ) / 2, offsetY, textWidth, output );

        supporters = "David C Jernberg\n"
                     "domoyega\n"
                     "Grigoris Papadourakis\n"
                     "Hajler\n"
                     "Hakon\n"
                     "hommaddict\n"
                     "Kiril Lipatov\n"
                     "Kresimir Condic\n"
                     "Kuza\n"
                     "Matt Taylor\n"
                     "Matthew Pfluger\n"
                     "Michael Van Wambeke\n"
                     "Siarzuk Piatrouski\n"
                     "slvclw\n"
                     "TechnoCore\n"
                     "William Hoskinson\n";

        name.set( std::move( supporters ), fheroes2::FontType::normalWhite() );
        name.draw( columnStep * 2 + ( columnStep - textWidth ) / 2, offsetY, textWidth, output );

        offsetY += constributorsHeight;

        name.set( _( "and many-many other contributors and supporters!" ), fheroes2::FontType::normalWhite() );
        name.draw( output.width() / 2 - name.width() / 2, offsetY, output );

        const fheroes2::Sprite & hydra = fheroes2::AGG::GetICN( ICN::HYDRA, 11 );

        offsetY = output.height() - hydra.height() - 40;

        fheroes2::Blit( hydra, output, textInitialOffsetX - hydra.width() / 2, offsetY );

        return output;
    }

    fheroes2::Sprite generateResurrectionCreditsThirdPage()
    {
        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::CBKGSWMP, 0 );
        output._disableTransformLayer();

        const int32_t textInitialOffsetX = output.width() / 2;
        const int32_t textInitialOffsetY = 80;
        const int32_t textWidth = 300;

        int32_t offsetX = ( textInitialOffsetX - textWidth ) / 2;
        int32_t offsetY = textInitialOffsetY;

        offsetY += renderText( output, offsetX, offsetY, textWidth, _( "Support us at" ), _( "local-donation-platform|https://www.patreon.com/fheroes2" ) );
        offsetY += 30;

        const fheroes2::Sprite & wizard = fheroes2::AGG::GetICN( ICN::CMBTCAPZ, 4 );
        fheroes2::Blit( wizard, output, ( textInitialOffsetX - wizard.width() ) / 2, offsetY );
        offsetY += wizard.height() + 20;

        offsetY
            += renderText( output, offsetX, offsetY, textWidth - 10, _( "Connect with us at" ), _( "local-social-network|https://www.facebook.com/groups/fheroes2" ) );
        offsetY += 20;

        const fheroes2::Sprite & vampireLord = fheroes2::AGG::GetICN( ICN::VAMPIRE2, 22 );
        fheroes2::Blit( vampireLord, output, ( textInitialOffsetX - vampireLord.width() ) / 2, offsetY );

        offsetY = textInitialOffsetY;
        offsetX += textInitialOffsetX;

        offsetY += renderText( output, offsetX, offsetY, textWidth - 10, _( "Need help with the game?" ), "https://discord.gg/xF85vbZ" );
        offsetY += 10;

        fheroes2::Sprite labyrinth = fheroes2::AGG::GetICN( ICN::TWNWUP_3, 0 );
        fheroes2::ApplyPalette( labyrinth, 2 );
        fheroes2::Blit( labyrinth, output, textInitialOffsetX + ( textInitialOffsetX - labyrinth.width() ) / 2, offsetY );

        offsetY += labyrinth.height() + 50;

        const int32_t miniMonsterXOffset = textInitialOffsetX * 3 / 2;

        fheroes2::Text name( _( "Original project before 0.7" ), fheroes2::FontType::smallYellow() );
        name.draw( offsetX, offsetY, textWidth - 10, output );
        offsetY += name.height( textWidth - 10 );
        name.set( "Andrey Afletdinov\nhttps://sourceforge.net/\nprojects/fheroes2/", fheroes2::FontType::smallWhite() );
        name.draw( offsetX, offsetY, textWidth - 10, output );

        fheroes2::Sprite creature = fheroes2::AGG::GetICN( ICN::MAGE2, 4 );
        transformToBlack( creature );

        const int32_t creatureOffsetY = output.height() - 95;
        fheroes2::Blit( creature, 0, 0, output, miniMonsterXOffset - creature.width() / 2, creatureOffsetY, creature.width(), creature.height() );
        name.set( "?", fheroes2::FontType::normalYellow() );
        name.draw( miniMonsterXOffset - name.width() / 2, creatureOffsetY + creature.height() / 2 - 5, output );

        return output;
    }

    fheroes2::Sprite generateSuccessionWarsCreditsFirstPage()
    {
        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::CBKGWATR, 0 );
        fheroes2::ApplyPalette( output, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        output._disableTransformLayer();

        const fheroes2::FontType nameFontType = fheroes2::FontType::normalWhite();

        const fheroes2::Text title( _( "Heroes of Might and Magic II: The Succession Wars team" ), nameFontType );
        title.draw( ( output.width() - title.width() ) / 2, 10, output );

        const int32_t textInitialOffsetY = 35;
        const int32_t textWidth = 320;
        const int32_t titleOffsetY = 7;

        int32_t offsetY = textInitialOffsetY;
        int32_t offsetX = 0;

        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Designed and Directed" ), "Jon Van Caneghem" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Programming and Design" ), "Phil Steinmeyer" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Executive Producer" ), "Mark Caldwell" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Producer" ), "Walt Hochbrueckner" );
        offsetY += titleOffsetY
                   + renderText( output, offsetX, offsetY, textWidth, _( "Additional Design" ),
                                 "Paul Rattner\n"
                                 "Debbie Van Caneghem" );
        offsetY += titleOffsetY
                   + renderText( output, offsetX, offsetY, textWidth, _( "Additional Programming" ),
                                 "George Ruof\n"
                                 "Todd Hendrix\n"
                                 "Mark Caldwell" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Musical Production" ), "Rob King" );
        renderText( output, offsetX, offsetY, textWidth, _( "Music and Sound Design" ),
                    "Rob King\n"
                    "Steve Baca\n"
                    "Paul Romero" );

        offsetY = textInitialOffsetY;
        offsetX += textWidth;

        offsetY += titleOffsetY
                   + renderText( output, offsetX, offsetY, textWidth, _( "Vocalists" ),
                                 "Grant Youngblood\n"
                                 "Kareen Meshagan" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Art Director" ), "Julia Ulano" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Assistant Art Director" ), "Bonita Long-Hemsath" );
        renderText( output, offsetX, offsetY, textWidth, _( "Artists" ),
                    "Julie Bateman\n"
                    "Rebecca Christel\n"
                    "Shelly Garcia\n"
                    "Sam Hasson\n"
                    "Louis Henderson\n"
                    "Tracy Iwata\n"
                    "Steve Jasper\n"
                    "April Lee\n"
                    "Lieu Pham\n"
                    "Phelan Sykes\n"
                    "Steve Wasaff\n"
                    "Scott White" );

        return output;
    }

    fheroes2::Sprite generateSuccessionWarsCreditsSecondPage()
    {
        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::CBKGWATR, 0 );
        fheroes2::ApplyPalette( output, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        output._disableTransformLayer();

        const fheroes2::FontType nameFontType = fheroes2::FontType::normalWhite();

        const fheroes2::Text title( _( "Heroes of Might and Magic II: The Succession Wars team" ), nameFontType );
        title.draw( ( output.width() - title.width() ) / 2, 10, output );

        const int32_t textInitialOffsetY = 35;
        const int32_t textWidth = 320;
        const int32_t titleOffsetY = 7;

        int32_t offsetY = textInitialOffsetY;
        int32_t offsetX = 0;

        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "QA Manager" ), "Peter Ryu" );
        offsetY += titleOffsetY
                   + renderText( output, offsetX, offsetY, textWidth, _( "QA" ),
                                 "David Botan\n"
                                 "David Fernandez\n"
                                 "Bill Nesemeier\n"
                                 "Walter Johnson\n"
                                 "Kate McClelland\n"
                                 "Timothy Lang\n"
                                 "Bryan Farina" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Writing" ), "Paul Rattner" );
        renderText( output, offsetX, offsetY, textWidth, _( "Manual and Helpfile" ),
                    "Bryan Farina\n"
                    "Rozita Tolouey\n"
                    "Bruce Schlickbernd" );

        offsetY = textInitialOffsetY;
        offsetX += textWidth;

        offsetY += titleOffsetY
                   + renderText( output, offsetX, offsetY, textWidth, _( "Scenarios" ),
                                 "Jon Van Caneghem\n"
                                 "Debbie Van Caneghem\n"
                                 "Clayton Retzer\n"
                                 "Christian Vanover\n"
                                 "Paul Rattner\n"
                                 "Benjamin Bent\n"
                                 "Bryan Farina\n"
                                 "Eric Heffron\n"
                                 "Mark Palczynski\n"
                                 "Walt Hochbrueckner\n"
                                 "Bruce Schlickbernd\n"
                                 "Craig Konas" );
        renderText( output, offsetX, offsetY, textWidth, _( "Special Thanks to" ),
                    "Scott McDaniel\n"
                    "Dean Rettig\n"
                    "Ted Chapman\n"
                    "Dean Frost" );

        return output;
    }

    fheroes2::Sprite generatePriceOfLoyaltyCreditsFirstPage()
    {
        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::CBKGGRAV, 0 );
        fheroes2::ApplyPalette( output, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        output._disableTransformLayer();

        const fheroes2::FontType titleFontType = fheroes2::FontType::normalYellow();
        const fheroes2::FontType nameFontType = fheroes2::FontType::normalWhite();

        fheroes2::Text title( _( "Heroes of Might and Magic II: The Price of Loyalty team" ), nameFontType );
        title.draw( ( output.width() - title.width() ) / 2, 10, output );

        title.set( "Cyberlore Studios", titleFontType );
        title.draw( ( output.width() - title.width() ) / 2, 10 + title.height() * 2, output );

        const int32_t textInitialOffsetY = 35 + title.height() * 4;
        const int32_t textWidth = 320;
        const int32_t titleOffsetY = 7;

        int32_t offsetY = textInitialOffsetY;
        int32_t offsetX = 0;

        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Executive Producer" ), "Lester Humphreys" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Producer" ), "Joe Minton" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Design Lead" ), "Jim DuBois" );
        offsetY += titleOffsetY
                   + renderText( output, offsetX, offsetY, textWidth, _( "Designers" ),
                                 "Jesse King\n"
                                 "Kris Greenia" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Programming Lead" ), "Mike White" );
        renderText( output, offsetX, offsetY, textWidth, _( "Art Director" ), "Seth Spaulding" );

        offsetY = textInitialOffsetY;
        offsetX += textWidth;

        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Art Lead" ), "Thomas Gale" );
        offsetY += titleOffsetY
                   + renderText( output, offsetX, offsetY, textWidth, _( "Artists" ),
                                 "Michael Clarke\n"
                                 "Michael Baker\n"
                                 "Julie Airoldi" );
        renderText( output, offsetX, offsetY, textWidth, _( "Playtesters" ),
                    "Bart Simon\n"
                    "Fred Fredette\n"
                    "Rendall Koski\n"
                    "T.J. Andrzejczyk\n"
                    "Joanne Delphia" );

        return output;
    }

    fheroes2::Sprite generatePriceOfLoyaltyCreditsSecondPage()
    {
        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::CBKGGRAV, 0 );
        fheroes2::ApplyPalette( output, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        output._disableTransformLayer();

        const fheroes2::FontType titleFontType = fheroes2::FontType::normalYellow();
        const fheroes2::FontType nameFontType = fheroes2::FontType::normalWhite();

        fheroes2::Text title( _( "Heroes of Might and Magic II: The Price of Loyalty team" ), nameFontType );
        title.draw( ( output.width() - title.width() ) / 2, 10, output );

        title.set( "New World Computing", titleFontType );
        title.draw( ( output.width() - title.width() ) / 2, 10 + title.height() * 2, output );

        const int32_t textInitialOffsetY = 35 + title.height() * 4;
        const int32_t textWidth = 320;
        const int32_t titleOffsetY = 7;

        int32_t offsetY = textInitialOffsetY;
        int32_t offsetX = 0;

        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Designer" ), "Jon Van Caneghem" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Executive Producer" ), "Mark Caldwell" );
        offsetY += titleOffsetY
                   + renderText( output, offsetX, offsetY, textWidth, _( "Producers" ),
                                 "Peter Ryu\n"
                                 "Deane Rettig" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Musical Production" ), "Rob King" );
        offsetY += titleOffsetY
                   + renderText( output, offsetX, offsetY, textWidth, _( "QA Managers" ),
                                 "Brian Gilmer\n"
                                 "Peter Ryu" );
        renderText( output, offsetX, offsetY, textWidth, _( "Music" ),
                    "Rob King\n"
                    "Paul Romero\n"
                    "Steve Baca" );

        offsetY = textInitialOffsetY;
        offsetX += textWidth;

        offsetY += titleOffsetY
                   + renderText( output, offsetX, offsetY, textWidth, _( "Sound Design" ),
                                 "Rob King\n"
                                 "Steve Baca" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Town Themes" ), "Paul Romero" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Alto Sax" ), "Brock \"Saxman\" Summers" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Harpsichord and Piano" ), "Paul Romero" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Basso Vocal" ), "Reid Bruton" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Soprano Vocal" ), "Karin Meshagin" );

        std::string recordedString = _( "Recorded at %{recordingStudio}" );
        StringReplace( recordedString, "%{recordingStudio}", "Green Street Studios" );

        title.set( recordedString, titleFontType );
        title.draw( offsetX, offsetY, textWidth, output );

        return output;
    }

    fheroes2::Sprite generatePriceOfLoyaltyCreditsThirdPage()
    {
        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::CBKGGRAV, 0 );
        fheroes2::ApplyPalette( output, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        output._disableTransformLayer();

        const fheroes2::FontType titleFontType = fheroes2::FontType::normalYellow();
        const fheroes2::FontType nameFontType = fheroes2::FontType::normalWhite();

        fheroes2::Text title( _( "Heroes of Might and Magic II: The Price of Loyalty team" ), nameFontType );
        title.draw( ( output.width() - title.width() ) / 2, 10, output );

        title.set( "New World Computing", titleFontType );
        title.draw( ( output.width() - title.width() ) / 2, 10 + title.height() * 2, output );

        const int32_t textInitialOffsetY = 35 + title.height() * 4;
        const int32_t textWidth = 320;
        const int32_t titleOffsetY = 7;

        int32_t offsetY = textInitialOffsetY;
        int32_t offsetX = 0;

        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "credits|Manual" ), "Bryan Farina" );
        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "German Consultant" ), "Dr. Brock H. Summers" );
        renderText( output, offsetX, offsetY, textWidth, _( "Map Designers" ),
                    "Christian Vanover\n"
                    "Ben Bent\n"
                    "Tracy Iwata\n"
                    "Clay Ratzner\n"
                    "Walter Hochbrueckner\n"
                    "Paul Ratner" );

        offsetY = textInitialOffsetY;
        offsetX += textWidth;

        offsetY += titleOffsetY + renderText( output, offsetX, offsetY, textWidth, _( "Package Design" ), "Rozita Tolouey" );
        renderText( output, offsetX, offsetY, textWidth, _( "Playtesters" ),
                    "Mikeael Herauf\n"
                    "Walter Johnson\n"
                    "David Botan\n"
                    "David Fernandez\n"
                    "Kate McClelland\n"
                    "William Nesemeier\n"
                    "Tim Lang\n"
                    "Pavel Vesely\n"
                    "John Lencioni\n"
                    "Jason Wildblood" );

        return output;
    }
}

void Game::ShowCredits( const bool keepMainMenuBorders )
{
    // Credits are shown in the place of Main Menu background which is correctly resized.
    // We get the Main Menu background ROI to use it for credits ROI and leave borders unchanged.
    const fheroes2::Sprite & mainMenuBackground = fheroes2::AGG::GetICN( ICN::HEROES, 0 );
    const fheroes2::Rect creditsRoi( mainMenuBackground.x(), mainMenuBackground.y(), mainMenuBackground.width(), mainMenuBackground.height() );

    // Hide mouse cursor.
    const CursorRestorer cursorRestorer( false );

    std::unique_ptr<fheroes2::ImageRestorer> restorer;

    fheroes2::Display & display = fheroes2::Display::instance();

    if ( keepMainMenuBorders ) {
        // Make a copy of background image to restore it during fade after the credits.
        restorer = std::make_unique<fheroes2::ImageRestorer>( display, creditsRoi.x, creditsRoi.y, creditsRoi.width, creditsRoi.height );

        fheroes2::fadeOutDisplay( creditsRoi, false );
    }

    AudioManager::PlayMusicAsync( MUS::VICTORY, Music::PlaybackMode::REWIND_AND_PLAY_INFINITE );

    const uint64_t animationDelay = 50;

    std::vector<fheroes2::Sprite> pages;
    pages.emplace_back( generateResurrectionCreditsFirstPage() );
    pages.emplace_back( generateResurrectionCreditsSecondPage() );
    pages.emplace_back( generateResurrectionCreditsThirdPage() );

    if ( Settings::Get().isPriceOfLoyaltySupported() ) {
        pages.emplace_back( generatePriceOfLoyaltyCreditsFirstPage() );
        pages.emplace_back( generatePriceOfLoyaltyCreditsSecondPage() );
        pages.emplace_back( generatePriceOfLoyaltyCreditsThirdPage() );
    }

    pages.emplace_back( generateSuccessionWarsCreditsFirstPage() );
    pages.emplace_back( generateSuccessionWarsCreditsSecondPage() );

    // Resize the credits pages. 'creditsRoi' is made using Main Menu background parameters that were already properly calculated for the current resolution.
    const int32_t resizedPageHeight = creditsRoi.width * pages.front().height() / pages.front().width();
    for ( fheroes2::Sprite & page : pages ) {
        fheroes2::Sprite resizedPage;
        resizedPage._disableTransformLayer();
        resizedPage.resize( creditsRoi.width, resizedPageHeight );
        fheroes2::Resize( page, resizedPage );
        page = std::move( resizedPage );
    }

    fheroes2::Sprite header;
    header._disableTransformLayer();
    header.resize( creditsRoi.width, creditsRoi.height - resizedPageHeight );
    fheroes2::Resize( generateHeader(), header );

    AnimationSequence sequence( static_cast<int32_t>( pages.size() ) );

    bool fadeInHeader = true;

    // Immediately indicate that the delay has passed to render first frame immediately.
    Game::passCustomAnimationDelay( animationDelay );
    // Make sure that the first run is passed immediately.
    assert( !Game::isCustomDelayNeeded( animationDelay ) );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents( Game::isCustomDelayNeeded( animationDelay ) ) ) {
        if ( le.isAnyKeyPressed() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() ) {
            break;
        }

        if ( Game::validateCustomAnimationDelay( animationDelay ) ) {
            if ( sequence.state() == AnimationState::NO_ACTION ) {
                if ( fadeInHeader ) {
                    fadeInHeader = false;
                }

                sequence.increment();
                continue;
            }

            const uint8_t alpha = sequence.alpha();

            if ( fadeInHeader ) {
                fheroes2::ApplyAlpha( header, 0, 0, display, creditsRoi.x, creditsRoi.y, header.width(), header.height(), alpha );
            }

            const fheroes2::Image & page = pages[sequence.pageId()];

            if ( alpha == 255 ) {
                // This alpha is for fully bright image so there is no need to apply alpha.
                Copy( page, 0, 0, display, creditsRoi.x, creditsRoi.y + header.height(), page.width(), page.height() );
            }
            else if ( alpha == 0 ) {
                // This alpha is for fully dark image so fill it with the black color.
                Fill( display, creditsRoi.x, creditsRoi.y + header.height(), page.width(), page.height(), 0 );
            }
            else {
                fheroes2::ApplyAlpha( page, 0, 0, display, creditsRoi.x, creditsRoi.y + header.height(), page.width(), page.height(), alpha );
            }

            display.render();

            sequence.increment();
        }
    }

    if ( keepMainMenuBorders ) {
        fheroes2::fadeOutDisplay( creditsRoi, false );

        // Restore a copy of background image to fade it in.
        restorer->restore();

        fheroes2::fadeInDisplay( creditsRoi, false );
    }
    else {
        fheroes2::fadeOutDisplay();
    }
}
