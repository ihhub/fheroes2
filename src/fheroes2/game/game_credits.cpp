/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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
#include "agg.h"
#include "agg_image.h"
#include "cursor.h"
#include "game_delays.h"
#include "icn.h"
#include "localevent.h"
#include "mus.h"
#include "screen.h"
#include "settings.h"
#include "text.h"

#include <cassert>

namespace
{
    void transformToBlack( fheroes2::Image & out )
    {
        if ( out.empty() )
            return;

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
                if ( _alphaValue >= 255 ) {
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
                    if ( _pageId >= _imageCount )
                        _pageId = 0;
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
            NO_ACTION_COUNTER_LIMIT = 80
        };
    };

    fheroes2::Sprite generateHeader()
    {
        const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::CBKGLAVA, 0 );
        assert( background.height() < fheroes2::Display::DEFAULT_HEIGHT );

        fheroes2::Sprite output( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT - background.height() );
        output.fill( 0 );
        output._disableTransformLayer();

        Text caption( "Free Heroes of Might and Magic II (" + Settings::GetVersion() + ")", Font::YELLOW_BIG );
        caption.Blit( output.width() / 2 - caption.w() / 2, 15, output );

        return output;
    }

    fheroes2::Sprite generateFirstPage()
    {
        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::CBKGLAVA, 0 );
        output._disableTransformLayer();

        const int32_t columnStep = 210;
        const int32_t textInitialOffsetY = 3;
        const int32_t textWidth = 200;

        int32_t offsetY = textInitialOffsetY;

        TextBox title( "Project Coordination and Core Development", Font::YELLOW_BIG, textWidth );
        TextBox name( "Ihar Hubchyk", Font::BIG, textWidth );
        title.Blit( ( columnStep - title.w() ) / 2, offsetY, output );
        name.Blit( ( columnStep - name.w() ) / 2, offsetY + title.h(), output );
        offsetY += title.h() + name.h() + 10;

        const fheroes2::Sprite & blackDragon = fheroes2::AGG::GetICN( ICN::DRAGBLAK, 5 );
        fheroes2::Blit( blackDragon, output, ( columnStep - blackDragon.width() ) / 2, offsetY );
        offsetY += blackDragon.height();

        title.Set( "QA and Support", Font::YELLOW_BIG, textWidth );
        name.Set( "Igor Tsivilko", Font::BIG, textWidth );
        title.Blit( ( columnStep - title.w() ) / 2, offsetY, output );
        name.Blit( ( columnStep - name.w() ) / 2, offsetY + title.h(), output );
        offsetY += title.h() + name.h() + 10;

        const fheroes2::Sprite & cyclop = fheroes2::AGG::GetICN( ICN::CYCLOPS, 38 );
        fheroes2::Blit( cyclop, output, ( columnStep - cyclop.width() ) / 2, offsetY );
        offsetY += cyclop.height();

        title.Set( "Development", Font::YELLOW_BIG, textWidth );
        name.Set( "Ivan Shibanov", Font::BIG, textWidth );
        title.Blit( ( columnStep - title.w() ) / 2, offsetY, output );
        name.Blit( ( columnStep - name.w() ) / 2, offsetY + title.h(), output );
        offsetY += title.h() + name.h() + 10;

        const fheroes2::Sprite & crusader = fheroes2::AGG::GetICN( ICN::PALADIN2, 23 );
        fheroes2::Blit( crusader, output, ( columnStep - crusader.width() ) / 2, offsetY );
        offsetY += crusader.height();

        const int32_t bottomOffset = offsetY;

        const fheroes2::Sprite & mage = fheroes2::AGG::GetICN( ICN::MAGE1, 24 );
        offsetY -= crusader.height();
        fheroes2::Blit( mage, output, columnStep + ( columnStep - mage.width() ) / 2, offsetY );

        name.Set( "Oleg Derevenetz", Font::BIG, textWidth );
        offsetY -= 10 + name.h();
        name.Blit( columnStep + ( columnStep - name.w() ) / 2, offsetY, output );
        offsetY -= title.h();
        title.Blit( columnStep + ( columnStep - title.w() ) / 2, offsetY, output );

        offsetY = bottomOffset + 10;

        const Text websiteInto( "Visit us at ", Font::BIG );
        const Text website( "https://github.com/ihhub/fheroes2", Font::YELLOW_BIG );
        const int32_t websiteOffsetX = ( output.width() - websiteInto.w() - website.w() ) / 2;
        websiteInto.Blit( websiteOffsetX, offsetY, output );
        website.Blit( websiteOffsetX + websiteInto.w(), offsetY, output );

        const fheroes2::Sprite & missile = fheroes2::AGG::GetICN( ICN::ARCH_MSL, 4 );
        fheroes2::Blit( missile, output, websiteOffsetX - 10 - missile.width(), offsetY + website.h() / 2 - missile.height() / 2 );
        fheroes2::Blit( missile, output, websiteOffsetX + websiteInto.w() + website.w() + 10, offsetY + website.h() / 2 - missile.height() / 2, true );

        offsetY = textInitialOffsetY;

        title.Set( "Special Thanks to", Font::YELLOW_BIG, textWidth );
        title.Blit( 2 * columnStep + ( columnStep - title.w() ) / 2, offsetY, output );
        offsetY += title.h();

        const std::string contributors( "LeHerosInconnu\n"
                                        "undef21\n"
                                        "shprotru\n"
                                        "eos428\n"
                                        "vincent-grosbois\n"
                                        "Andrii Kurdiumov\n"
                                        "Vasilenko Alexey\n"
                                        "Andrey Starodubtsev\n"
                                        "tau3\n"
                                        "and many other contributors!" );

        name.Set( contributors, Font::BIG, textWidth );
        name.Blit( 2 * columnStep + ( columnStep - name.w() ) / 2, offsetY, output );
        offsetY += name.h() + 10;

        const fheroes2::Sprite & hydra = fheroes2::AGG::GetICN( ICN::HYDRA, 11 );
        fheroes2::Blit( hydra, output, 2 * columnStep + ( columnStep - hydra.width() ) / 2, offsetY );
        offsetY += hydra.height();

        title.Set( "Original project before 0.7", Font::YELLOW_SMALL, textWidth );
        title.Blit( 2 * columnStep + ( columnStep - title.w() ) / 2, offsetY, output );
        offsetY += title.h();

        name.Set( "Andrey Afletdinov\nhttps://sourceforge.net/\nprojects/fheroes2/", Font::SMALL, textWidth );
        name.Blit( 2 * columnStep + ( columnStep - name.w() ) / 2, offsetY, output );

        const fheroes2::Sprite & goblin = fheroes2::AGG::GetICN( ICN::GOBLIN, 27 );
        fheroes2::Blit( goblin, output, ( output.width() - goblin.width() ) / 2, ( output.height() - goblin.height() ) / 2 );

        return output;
    }

    fheroes2::Sprite generateSecondPage()
    {
        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::CBKGSWMP, 0 );
        output._disableTransformLayer();

        const int32_t columnStep = 425;
        const int32_t textInitialOffsetY = 50;
        const int32_t textWidth = 300;

        int32_t offsetY = textInitialOffsetY;

        TextBox title( "Support us at", Font::BIG, textWidth );
        TextBox name( "https://www.patreon.com/fheroes2", Font::YELLOW_BIG, textWidth );
        title.Blit( ( columnStep - title.w() ) / 2, offsetY, output );
        name.Blit( ( columnStep - name.w() ) / 2, offsetY + title.h(), output );
        offsetY += title.h() + name.h() + 10;

        const fheroes2::Sprite & wizard = fheroes2::AGG::GetICN( ICN::CMBTCAPZ, 4 );
        fheroes2::Blit( wizard, output, ( columnStep - wizard.width() ) / 2, offsetY );
        offsetY += wizard.height();

        title.Set( "Connect with us at", Font::BIG, textWidth );
        name.Set( "https://www.facebook.com/groups/fheroes2", Font::YELLOW_BIG, textWidth - 10 ); // special case to properly split the string
        title.Blit( ( columnStep - title.w() ) / 2, offsetY, output );
        name.Blit( ( columnStep - name.w() ) / 2, offsetY + title.h(), output );
        offsetY += title.h() + name.h() + 10;

        const fheroes2::Sprite & vampireLord = fheroes2::AGG::GetICN( ICN::VAMPIRE2, 22 );
        fheroes2::Blit( vampireLord, output, ( columnStep - vampireLord.width() ) / 2, offsetY );
        offsetY += vampireLord.height();

        title.Set( "Need help with the game?", Font::BIG, textWidth );
        name.Set( "https://discord.gg/xF85vbZ", Font::YELLOW_BIG, textWidth );
        title.Blit( ( columnStep - title.w() ) / 2, offsetY, output );
        name.Blit( ( columnStep - name.w() ) / 2, offsetY + title.h(), output );
        offsetY += title.h() + name.h() + 10;

        fheroes2::Sprite labyrinth = fheroes2::AGG::GetICN( ICN::TWNWUP_3, 0 );
        fheroes2::ApplyPalette( labyrinth, 2 );
        fheroes2::Blit( labyrinth, output, ( columnStep - labyrinth.width() ) / 2, offsetY );

        offsetY = textInitialOffsetY + 70;

        title.Set( "Special Thanks to", Font::YELLOW_BIG, output.width() - columnStep );
        title.Blit( columnStep + ( output.width() - columnStep - title.w() ) / 2, offsetY, output );
        offsetY += title.h();

        const std::string contributors( "Matt Taylor\n"
                                        "Lipatov Kiril\n"
                                        "Aleksei Mazur\n"
                                        "William Hoskinson\n"
                                        "and many-many other supporters!" );

        name.Set( contributors, Font::BIG, output.width() - columnStep );
        name.Blit( columnStep + ( output.width() - columnStep - title.w() ) / 2, offsetY, output );
        offsetY += name.h() + 10;

        const fheroes2::Sprite & miniPeasant = fheroes2::AGG::GetICN( ICN::MONS32, 0 );
        const fheroes2::Sprite & miniSwordsman = fheroes2::AGG::GetICN( ICN::MONS32, 5 );
        const fheroes2::Sprite & miniCrusader = fheroes2::AGG::GetICN( ICN::MONS32, 10 );
        const fheroes2::Sprite & miniTitan = fheroes2::AGG::GetICN( ICN::MONS32, 46 );

        const int32_t miniMonsterXOffset = columnStep + ( output.width() - columnStep ) / 2;
        offsetY += miniTitan.height();

        fheroes2::Blit( miniPeasant, 0, 0, output, miniMonsterXOffset - miniPeasant.width() - miniSwordsman.width(), offsetY - miniPeasant.height(), miniPeasant.width(),
                        miniPeasant.height() );
        fheroes2::Blit( miniSwordsman, 0, 0, output, miniMonsterXOffset - miniSwordsman.width(), offsetY - miniSwordsman.height(), miniSwordsman.width(),
                        miniSwordsman.height() );
        fheroes2::Blit( miniCrusader, 0, 0, output, miniMonsterXOffset, offsetY - miniCrusader.height(), miniCrusader.width(), miniCrusader.height() );
        fheroes2::Blit( miniTitan, 0, 0, output, miniMonsterXOffset + miniCrusader.width(), offsetY - miniTitan.height(), miniTitan.width(), miniTitan.height() );

        fheroes2::Sprite creature = fheroes2::AGG::GetICN( ICN::MAGE2, 4 );
        transformToBlack( creature );

        const int32_t creatureOffsetY = output.height() - 100;
        fheroes2::Blit( creature, 0, 0, output, miniMonsterXOffset - creature.width() / 2, output.height() - 100, creature.width(), creature.height() );
        title.Set( "?", Font::YELLOW_BIG, 30 );
        title.Blit( miniMonsterXOffset - title.w() / 2, creatureOffsetY + creature.height() / 2, output );

        return output;
    }
}

void Game::ShowCredits()
{
    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    AGG::PlayMusic( MUS::VICTORY, true, true );

    LocalEvent & le = LocalEvent::Get();

    fheroes2::Image blackScreen( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );
    blackScreen.fill( 0 );
    blackScreen._disableTransformLayer();

    fheroes2::Image output( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );
    output.reset();
    output._disableTransformLayer();

    const uint64_t animationDelay = 50;

    std::vector<fheroes2::Sprite> pages;
    pages.emplace_back( generateFirstPage() );
    pages.emplace_back( generateSecondPage() );

    const fheroes2::Sprite header = generateHeader();

    AnimationSequence sequence( static_cast<int32_t>( pages.size() ) );

    bool fadeInHeader = true;

    while ( le.HandleEvents( Game::isCustomDelayNeeded( animationDelay ) ) ) {
        if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() )
            break;

        if ( Game::validateCustomAnimationDelay( animationDelay ) ) {
            if ( fadeInHeader && sequence.state() != AnimationState::FADING_IN ) {
                fadeInHeader = false;
                fheroes2::Copy( header, 0, 0, output, 0, 0, header.width(), header.height() );
            }

            if ( fadeInHeader ) {
                output = blackScreen;
                fheroes2::AlphaBlit( header, 0, 0, output, 0, 0, header.width(), header.height(), sequence.alpha() );

                const fheroes2::Image & page = pages[sequence.pageId()];
                fheroes2::AlphaBlit( page, 0, 0, output, 0, header.height(), page.width(), page.height(), sequence.alpha() );
            }
            else {
                const fheroes2::Image & page = pages[sequence.pageId()];
                fheroes2::Copy( blackScreen, 0, 0, output, 0, header.height(), page.width(), page.height() );
                fheroes2::AlphaBlit( page, 0, 0, output, 0, header.height(), page.width(), page.height(), sequence.alpha() );
            }

            fheroes2::Resize( output, display );
            display.render();

            sequence.increment();
        }
    }
}
