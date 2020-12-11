/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <queue>

#include "agg.h"
#include "army.h"
#include "battle.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_interface.h"
#include "cursor.h"
#include "engine.h"
#include "game.h"
#include "heroes.h"
#include "luck.h"
#include "morale.h"
#include "mus.h"
#include "race.h"
#include "settings.h"
#include "text.h"
#include "world.h"

namespace
{
    // DialogBattleSummary text related values
    const int bsTextWidth = 270;
    const int bsTextXOffset = 25;
    const int bsTextYOffset = 175;
    const int bsTextIndent = 30;

    class LoopedAnimation
    {
    public:
        explicit LoopedAnimation( int icnId = 0, bool loop = false )
            : _icnId( icnId )
            , _frameId( 0 )
            , _counter( 0 )
            , _finished( false )
            , _loop( loop )
        {
            _frameId = ICN::AnimationFrame( _icnId, 1, _counter );
        }

        uint32_t frameId()
        {
            if ( _finished )
                return _frameId;

            ++_counter;
            uint32_t nextId = ICN::AnimationFrame( _icnId, 1, _counter );
            if ( nextId < _frameId ) {
                if ( _loop ) {
                    _counter = 0;
                    nextId = ICN::AnimationFrame( _icnId, 1, _counter );
                    std::swap( nextId, _frameId );
                    return nextId;
                }
                else {
                    _finished = true;
                }
            }
            else {
                std::swap( nextId, _frameId );
                return nextId;
            }

            return _frameId;
        }

        bool isFinished() const
        {
            return _finished;
        }

        int id() const
        {
            return _icnId;
        }

    private:
        int _icnId;
        uint32_t _frameId;
        uint32_t _counter;
        bool _finished;
        bool _loop;
    };

    class LoopedAnimationSequence
    {
    public:
        void push( int icnId, bool loop )
        {
            _queue.push( LoopedAnimation( icnId, loop ) );
        }

        uint32_t frameId()
        {
            if ( isFinished() )
                return 0;

            return _queue.front().frameId();
        }

        bool nextFrame() // returns true only if there is some frames left
        {
            if ( !_queue.empty() && _queue.front().isFinished() )
                _queue.pop();

            return _queue.empty();
        }

        bool isFinished() const
        {
            return _queue.empty();
        }

        int id() const
        {
            if ( isFinished() )
                return 0;

            return _queue.front().id();
        }

    private:
        std::queue<LoopedAnimation> _queue;
    };
}

namespace Battle
{
    void GetSummaryParams( int res1, int res2, const HeroBase & hero, u32 exp, LoopedAnimationSequence & sequence, std::string & title, std::string & msg );
    void RedrawBattleSettings( const std::vector<fheroes2::Rect> & areas );
    void RedrawOnOffSetting( const Rect & area, const std::string & name, uint32_t index, bool isSet );
}

void Battle::RedrawBattleSettings( const std::vector<fheroes2::Rect> & areas )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();

    // Speed setting
    const Text speedTitle( _( "Speed" ), Font::SMALL );
    speedTitle.Blit( areas[0].x + ( areas[0].width - speedTitle.w() ) / 2, areas[0].y - 13 );

    int speed = Settings::Get().BattleSpeed();
    std::string str = _( "Speed: %{speed}" );
    StringReplace( str, "%{speed}", speed );

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::CSPANEL, ( speed < 5 ? 0 : ( speed < 8 ? 1 : 2 ) ) );
    fheroes2::Blit( sprite, fheroes2::Display::instance(), areas[0].x, areas[0].y );
    Text text( str, Font::SMALL );
    text.Blit( areas[0].x + ( sprite.width() - text.w() ) / 2, areas[0].y + sprite.height() + 3 );

    RedrawOnOffSetting( areas[3], _( "Grid" ), 8, conf.BattleShowGrid() );
    RedrawOnOffSetting( areas[4], _( "Shadow Movement" ), 10, conf.BattleShowMoveShadow() );
    RedrawOnOffSetting( areas[5], _( "Shadow Cursor" ), 12, conf.BattleShowMouseShadow() );

    display.render();
}

void Battle::RedrawOnOffSetting( const Rect & area, const std::string & name, uint32_t index, bool isSet )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::CSPANEL, isSet ? index + 1 : index );
    const int textOffset = 2;

    TextBox upperText( name, Font::SMALL, area.w );
    upperText.Blit( area.x + ( area.w - upperText.w() ) / 2, area.y - upperText.h() - textOffset );

    fheroes2::Blit( sprite, display, area.x, area.y );

    const Text lowerText( isSet ? _( "On" ) : _( "Off" ), Font::SMALL );
    lowerText.Blit( area.x + ( area.w - lowerText.w() ) / 2, area.y + area.h + textOffset );
}

void Battle::DialogBattleSettings( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Settings & conf = Settings::Get();

    cursor.Hide();

    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( conf.ExtGameEvilInterface() ? ICN::CSPANBKE : ICN::CSPANBKG ), 0 );
    const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( ( conf.ExtGameEvilInterface() ? ICN::CSPANBKE : ICN::CSPANBKG ), 1 );

    const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
    const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

    fheroes2::ImageRestorer back( display, shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH );
    const fheroes2::Rect pos_rt( dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() );

    fheroes2::Fill( display, pos_rt.x, pos_rt.y, pos_rt.width, pos_rt.height, 0 );
    fheroes2::Blit( dialogShadow, display, pos_rt.x - BORDERWIDTH, pos_rt.y + BORDERWIDTH );
    fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

    const fheroes2::Sprite & panelSprite = fheroes2::AGG::GetICN( ICN::CSPANEL, 0 );
    const int32_t panelWidth = panelSprite.width();
    const int32_t panelHeight = panelSprite.height();

    std::vector<fheroes2::Rect> optionAreas;
    optionAreas.push_back( fheroes2::Rect( pos_rt.x + 36, pos_rt.y + 47, panelWidth, panelHeight ) ); // speed
    optionAreas.push_back( fheroes2::Rect( pos_rt.x + 128, pos_rt.y + 47, panelWidth, panelHeight ) ); // info
    optionAreas.push_back( fheroes2::Rect( pos_rt.x + 220, pos_rt.y + 47, panelWidth, panelHeight ) ); // auto spell cast
    optionAreas.push_back( fheroes2::Rect( pos_rt.x + 36, pos_rt.y + 157, panelWidth, panelHeight ) ); // grid
    optionAreas.push_back( fheroes2::Rect( pos_rt.x + 128, pos_rt.y + 157, panelWidth, panelHeight ) ); // move shadow
    optionAreas.push_back( fheroes2::Rect( pos_rt.x + 220, pos_rt.y + 157, panelWidth, panelHeight ) ); // cursor shadow

    fheroes2::Button btn_ok( pos_rt.x + 113, pos_rt.y + 252, ( conf.ExtGameEvilInterface() ? ICN::CSPANBTE : ICN::CSPANBTN ), 0, 1 );
    btn_ok.draw();

    RedrawBattleSettings( optionAreas );

    cursor.Show();
    display.render();

    bool saveConfiguration = false;

    while ( le.HandleEvents() ) {
        le.MousePressLeft( btn_ok.area() ) ? btn_ok.drawOnPress() : btn_ok.drawOnRelease();

        if ( le.MouseClickLeft( optionAreas[0] ) ) {
            conf.SetBattleSpeed( conf.BattleSpeed() % 10 + 1 );
            Game::UpdateGameSpeed();
            fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );
            RedrawBattleSettings( optionAreas );
            display.render();
            saveConfiguration = true;
        }
        else if ( le.MouseClickLeft( optionAreas[3] ) ) {
            conf.SetBattleGrid( !conf.BattleShowGrid() );
            fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );
            RedrawBattleSettings( optionAreas );
            saveConfiguration = true;
        }
        else if ( le.MouseClickLeft( optionAreas[4] ) ) {
            conf.SetBattleMovementShaded( !conf.BattleShowMoveShadow() );
            fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );
            RedrawBattleSettings( optionAreas );
            saveConfiguration = true;
        }
        else if ( le.MouseClickLeft( optionAreas[5] ) ) {
            conf.SetBattleMouseShaded( !conf.BattleShowMouseShadow() );
            fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );
            RedrawBattleSettings( optionAreas );
            saveConfiguration = true;
        }
        else if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( btn_ok.area() ) ) {
            break;
        }
    }

    if ( saveConfiguration ) {
        conf.Save( "fheroes2.cfg" );
    }
}

void Battle::GetSummaryParams( int res1, int res2, const HeroBase & hero, u32 exp, LoopedAnimationSequence & sequence, std::string & title, std::string & msg )
{
    if ( res1 & RESULT_WINS ) {
        sequence.push( ICN::WINCMBT, true );
        if ( res2 & RESULT_SURRENDER )
            title.append( _( "The enemy has surrendered!" ) );
        else if ( res2 & RESULT_RETREAT )
            title.append( _( "The enemy has fled!" ) );
        else
            title.append( _( "A glorious victory!" ) );

        if ( hero.isHeroes() ) {
            msg.append( _( "For valor in combat, %{name} receives %{exp} experience." ) );
            StringReplace( msg, "%{name}", hero.GetName() );
            StringReplace( msg, "%{exp}", exp );
        }
    }
    else if ( res1 & RESULT_RETREAT ) {
        sequence.push( ICN::CMBTFLE1, false );
        sequence.push( ICN::CMBTFLE2, false );
        sequence.push( ICN::CMBTFLE3, false );
        msg.append( _( "The cowardly %{name} flees from battle." ) );
        StringReplace( msg, "%{name}", hero.GetName() );
    }
    else if ( res1 & RESULT_SURRENDER ) {
        sequence.push( ICN::CMBTSURR, true );
        msg.append( _( "%{name} surrenders to the enemy, and departs in shame." ) );
        StringReplace( msg, "%{name}", hero.GetName() );
    }
    else {
        sequence.push( ICN::CMBTLOS1, false );
        sequence.push( ICN::CMBTLOS2, false );
        sequence.push( ICN::CMBTLOS3, true );
        msg.append( _( "Your force suffer a bitter defeat, and %{name} abandons your cause." ) );
        StringReplace( msg, "%{name}", hero.GetName() );
    }
}

void Battle::Arena::DialogBattleSummary( const Result & res, const bool transferArtifacts ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Settings & conf = Settings::Get();

    const Troops killed1 = army1->GetKilledTroops();
    const Troops killed2 = army2->GetKilledTroops();

    cursor.SetThemes( Cursor::POINTER );

    std::string msg;
    std::string title;
    LoopedAnimationSequence sequence;

    if ( ( res.army1 & RESULT_WINS ) && army1->GetCommander() && army1->GetCommander()->isControlHuman() ) {
        GetSummaryParams( res.army1, res.army2, *army1->GetCommander(), res.exp1, sequence, title, msg );
        if ( conf.Music() )
            AGG::PlayMusic( MUS::BATTLEWIN, false );
    }
    else if ( ( res.army2 & RESULT_WINS ) && army2->GetCommander() && army2->GetCommander()->isControlHuman() ) {
        GetSummaryParams( res.army2, res.army1, *army2->GetCommander(), res.exp2, sequence, title, msg );
        if ( conf.Music() )
            AGG::PlayMusic( MUS::BATTLEWIN, false );
    }
    else if ( army1->GetCommander() && army1->GetCommander()->isControlHuman() ) {
        GetSummaryParams( res.army1, res.army2, *army1->GetCommander(), res.exp1, sequence, title, msg );
        if ( conf.Music() )
            AGG::PlayMusic( MUS::BATTLELOSE, false );
    }
    else if ( army2->GetCommander() && army2->GetCommander()->isControlHuman() ) {
        GetSummaryParams( res.army2, res.army1, *army2->GetCommander(), res.exp2, sequence, title, msg );
        if ( conf.Music() )
            AGG::PlayMusic( MUS::BATTLELOSE, false );
    }
    else
        // AI move
        if ( army1->GetCommander() && army1->GetCommander()->isControlAI() ) {
        // AI wins
        if ( res.army1 & RESULT_WINS ) {
            sequence.push( ICN::CMBTLOS1, false );
            sequence.push( ICN::CMBTLOS2, false );
            sequence.push( ICN::CMBTLOS3, false );
            msg.append( _( "Your force suffer a bitter defeat." ) );
        }
        else
            // Human wins
            if ( res.army2 & RESULT_WINS ) {
            sequence.push( ICN::WINCMBT, true );
            msg.append( _( "A glorious victory!" ) );
        }
    }

    if ( sequence.isFinished() ) // Cannot be!
        sequence.push( ICN::UNKNOWN, false );

    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( conf.ExtGameEvilInterface() ? ICN::WINLOSEE : ICN::WINLOSE ), 0 );
    const fheroes2::Sprite & dialogShadow = fheroes2::AGG::GetICN( ( conf.ExtGameEvilInterface() ? ICN::WINLOSEE : ICN::WINLOSE ), 1 );

    const fheroes2::Point dialogOffset( ( display.width() - dialog.width() ) / 2, ( display.height() - dialog.height() ) / 2 );
    const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

    fheroes2::ImageRestorer back( display, shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH - 1 );
    const fheroes2::Rect pos_rt( dialogOffset.x, dialogOffset.y, dialog.width(), dialog.height() );

    fheroes2::Blit( dialogShadow, display, pos_rt.x - BORDERWIDTH, pos_rt.y + BORDERWIDTH - 1 );
    fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

    const int anime_ox = 47;
    const int anime_oy = 36;

    const fheroes2::Sprite & sequenceBase = fheroes2::AGG::GetICN( sequence.id(), 0 );
    const fheroes2::Sprite & sequenceStart = fheroes2::AGG::GetICN( sequence.id(), 1 );

    fheroes2::Blit( sequenceBase, display, pos_rt.x + anime_ox + sequenceBase.x(), pos_rt.y + anime_oy + sequenceBase.y() );
    fheroes2::Blit( sequenceStart, display, pos_rt.x + anime_ox + sequenceStart.x(), pos_rt.y + anime_oy + sequenceStart.y() );

    fheroes2::Button btn_ok( pos_rt.x + 121, pos_rt.y + 410, ( conf.ExtGameEvilInterface() ? ICN::WINCMBBE : ICN::WINCMBTB ), 0, 1 );

    int32_t messageYOffset = 0;
    if ( !title.empty() ) {
        TextBox box( title, Font::YELLOW_BIG, bsTextWidth );
        box.Blit( pos_rt.x + bsTextXOffset, pos_rt.y + bsTextYOffset );
        messageYOffset = bsTextIndent;
    }

    if ( !msg.empty() ) {
        TextBox box( msg, Font::BIG, bsTextWidth );
        box.Blit( pos_rt.x + bsTextXOffset, pos_rt.y + bsTextYOffset + messageYOffset );
    }

    // battlefield casualties
    Text text( _( "Battlefield Casualties" ), Font::SMALL );
    text.Blit( pos_rt.x + ( pos_rt.width - text.w() ) / 2, pos_rt.y + 270 );

    // attacker
    text.Set( _( "Attacker" ), Font::SMALL );
    text.Blit( pos_rt.x + ( pos_rt.width - text.w() ) / 2, pos_rt.y + 285 );

    if ( killed1.isValid() )
        Army::DrawMons32Line( killed1, pos_rt.x + 25, pos_rt.y + 303, 270 );
    else {
        text.Set( "None", Font::SMALL );
        text.Blit( pos_rt.x + ( pos_rt.width - text.w() ) / 2, pos_rt.y + 300 );
    }

    // defender
    text.Set( _( "Defender" ), Font::SMALL );
    text.Blit( pos_rt.x + ( pos_rt.width - text.w() ) / 2, pos_rt.y + 345 );

    if ( killed2.isValid() )
        Army::DrawMons32Line( killed2, pos_rt.x + 25, pos_rt.y + 363, 270 );
    else {
        text.Set( "None", Font::SMALL );
        text.Blit( pos_rt.x + ( pos_rt.width - text.w() ) / 2, pos_rt.y + 360 );
    }

    btn_ok.draw();

    cursor.Show();
    display.render();

    while ( le.HandleEvents() ) {
        le.MousePressLeft( btn_ok.area() ) ? btn_ok.drawOnPress() : btn_ok.drawOnRelease();

        // exit
        if ( HotKeyCloseWindow || le.MouseClickLeft( btn_ok.area() ) )
            break;

        // animation
        if ( Game::AnimateInfrequentDelay( Game::BATTLE_DIALOG_DELAY ) && !sequence.nextFrame() ) {
            const fheroes2::Sprite & base = fheroes2::AGG::GetICN( sequence.id(), 0 );
            const fheroes2::Sprite & sequenceCurrent = fheroes2::AGG::GetICN( sequence.id(), sequence.frameId() );

            fheroes2::Blit( base, display, pos_rt.x + anime_ox + sequenceBase.x(), pos_rt.y + anime_oy + sequenceBase.y() );
            fheroes2::Blit( sequenceCurrent, display, pos_rt.x + anime_ox + sequenceCurrent.x(), pos_rt.y + anime_oy + sequenceCurrent.y() );
            display.render();
        }
    }

    if ( transferArtifacts ) {
        HeroBase * hero1 = ( res.army1 & RESULT_WINS ? army1->GetCommander() : ( res.army2 & RESULT_WINS ? army2->GetCommander() : NULL ) );
        HeroBase * hero2 = ( res.army1 & RESULT_LOSS ? army1->GetCommander() : ( res.army2 & RESULT_LOSS ? army2->GetCommander() : NULL ) );

        BagArtifacts & bag1 = hero1->GetBagArtifacts();
        BagArtifacts & bag2 = hero2->GetBagArtifacts();

        for ( size_t i = 0; i < bag2.size(); ++i ) {
            Artifact & art = bag2[i];

            if ( art.isUltimate() ) {
                art = Artifact::UNKNOWN;
                continue;
            }

            if ( art() == Artifact::UNKNOWN || art() == Artifact::MAGIC_BOOK ) {
                continue;
            }

            BagArtifacts::iterator it = std::find( bag1.begin(), bag1.end(), Artifact( ( Artifact::UNKNOWN ) ) );
            if ( bag1.end() != it ) {
                *it = art;

                back.restore();
                back.update( shadowOffset.x, shadowOffset.y, dialog.width() + BORDERWIDTH, dialog.height() + BORDERWIDTH - 1 );
                fheroes2::Blit( dialogShadow, display, pos_rt.x - BORDERWIDTH, pos_rt.y + BORDERWIDTH - 1 );
                fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

                Game::PlayPickupSound();

                TextBox box( _( "You have captured an enemy artifact!" ), Font::YELLOW_BIG, bsTextWidth );
                box.Blit( pos_rt.x + bsTextXOffset, pos_rt.y + bsTextYOffset );
                messageYOffset = bsTextIndent;

                const fheroes2::Sprite & border = fheroes2::AGG::GetICN( ICN::RESOURCE, 7 );
                const fheroes2::Sprite & artifact = fheroes2::AGG::GetICN( ICN::ARTIFACT, art.IndexSprite64() );

                fheroes2::Image image = border;
                fheroes2::Blit( artifact, image, 5, 5 );

                fheroes2::Blit( image, display, pos_rt.x + 119, pos_rt.y + 310 );

                TextBox artName( art.GetName(), Font::SMALL, bsTextWidth );
                artName.Blit( pos_rt.x + bsTextXOffset, pos_rt.y + 310 + image.height() + 5 );

                while ( le.HandleEvents() ) {
                    le.MousePressLeft( btn_ok.area() ) ? btn_ok.drawOnPress() : btn_ok.drawOnRelease();

                    // exit
                    if ( HotKeyCloseWindow || le.MouseClickLeft( btn_ok.area() ) )
                        break;

                    // animation
                    if ( Game::AnimateInfrequentDelay( Game::BATTLE_DIALOG_DELAY ) && !sequence.nextFrame() ) {
                        const fheroes2::Sprite & base = fheroes2::AGG::GetICN( sequence.id(), 0 );
                        const fheroes2::Sprite & sequenceCurrent = fheroes2::AGG::GetICN( sequence.id(), sequence.frameId() );

                        fheroes2::Blit( base, display, pos_rt.x + anime_ox + sequenceBase.x(), pos_rt.y + anime_oy + sequenceBase.y() );
                        fheroes2::Blit( sequenceCurrent, display, pos_rt.x + anime_ox + sequenceCurrent.x(), pos_rt.y + anime_oy + sequenceCurrent.y() );
                        display.render();
                    }
                }
            }
            art = Artifact::UNKNOWN;
        }
    }
}

int Battle::Arena::DialogBattleHero( const HeroBase & hero, bool buttons ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Settings & conf = Settings::Get();

    cursor.SetThemes( Cursor::POINTER );

    const bool readonly = current_color != hero.GetColor() || !buttons;
    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( ( conf.ExtGameEvilInterface() ? ICN::VGENBKGE : ICN::VGENBKG ), 0 );

    const fheroes2::Point dialogShadow( 15, 15 );

    fheroes2::Rect pos_rt( ( display.width() - dialog.width() - dialogShadow.x ) / 2, ( display.height() - dialog.height() - dialogShadow.y ) / 2, dialog.width(),
                           dialog.height() );

    fheroes2::ImageRestorer back( display, pos_rt.x, pos_rt.y, pos_rt.width, pos_rt.height );

    fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

    // first 15 pixels in the dialog is left shadow, skip
    pos_rt.x += 15;
    pos_rt.width -= 15;

    hero.PortraitRedraw( pos_rt.x + 12, pos_rt.y + 42, PORT_BIG, display );
    int col = ( Color::NONE == hero.GetColor() ? 1 : Color::GetIndex( hero.GetColor() ) + 1 );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::VIEWGEN, col ), display, pos_rt.x + 133, pos_rt.y + 36 );

    fheroes2::Point tp( pos_rt.x, pos_rt.y );

    std::string str;
    Text text;
    text.Set( Font::SMALL );
    str = _( "%{name} the %{race}" );
    StringReplace( str, "%{name}", hero.GetName() );
    StringReplace( str, "%{race}", Race::String( hero.GetRace() ) );
    text.Set( str );
    tp.x = pos_rt.x + ( pos_rt.width - text.w() ) / 2;
    tp.y = pos_rt.y + 11;
    text.Blit( tp.x, tp.y );
    str = _( "Attack" ) + std::string( ": " ) + GetString( hero.GetAttack() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 40;
    text.Blit( tp.x, tp.y );
    str = _( "Defense" ) + std::string( ": " ) + GetString( hero.GetDefense() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 51;
    text.Blit( tp.x, tp.y );
    str = _( "Spell Power" ) + std::string( ": " ) + GetString( hero.GetPower() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 62;
    text.Blit( tp.x, tp.y );
    str = _( "Knowledge" ) + std::string( ": " ) + GetString( hero.GetKnowledge() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 73;
    text.Blit( tp.x, tp.y );
    str = _( "Morale" ) + std::string( ": " ) + Morale::String( hero.GetMorale() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 84;
    text.Blit( tp.x, tp.y );
    str = _( "Luck" ) + std::string( ": " ) + Luck::String( hero.GetLuck() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 95;
    text.Blit( tp.x, tp.y );
    str = _( "Spell Points" ) + std::string( ": " ) + GetString( hero.GetSpellPoints() ) + "/" + GetString( hero.GetMaxSpellPoints() );
    text.Set( str );
    tp.x = pos_rt.x + 190 - text.w() / 2;
    tp.y = pos_rt.y + 117;
    text.Blit( tp.x, tp.y );

    fheroes2::Button btnCast( pos_rt.x + 15, pos_rt.y + 148, ICN::VIEWGEN, 9, 10 );
    fheroes2::Button btnRetreat( pos_rt.x + 74, pos_rt.y + 148, ICN::VIEWGEN, 11, 12 );
    fheroes2::Button btnSurrender( pos_rt.x + 133, pos_rt.y + 148, ICN::VIEWGEN, 13, 14 );
    fheroes2::Button btnClose( pos_rt.x + 192, pos_rt.y + 148, ICN::VIEWGEN, 15, 16 );

    if ( readonly || !hero.HaveSpellBook() || hero.Modes( Heroes::SPELLCASTED ) )
        btnCast.disable();

    if ( readonly || !CanRetreatOpponent( hero.GetColor() ) )
        btnRetreat.disable();

    if ( readonly || !CanSurrenderOpponent( hero.GetColor() ) )
        btnSurrender.disable();

    btnCast.draw();
    btnRetreat.draw();
    btnSurrender.draw();
    btnClose.draw();

    int result = 0;

    display.render();

    while ( le.HandleEvents() && !result ) {
        btnCast.isEnabled() && le.MousePressLeft( btnCast.area() ) ? btnCast.drawOnPress() : btnCast.drawOnRelease();
        btnRetreat.isEnabled() && le.MousePressLeft( btnRetreat.area() ) ? btnRetreat.drawOnPress() : btnRetreat.drawOnRelease();
        btnSurrender.isEnabled() && le.MousePressLeft( btnSurrender.area() ) ? btnSurrender.drawOnPress() : btnSurrender.drawOnRelease();
        le.MousePressLeft( btnClose.area() ) ? btnClose.drawOnPress() : btnClose.drawOnRelease();

        if ( !buttons && !le.MousePressRight() )
            break;

        if ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_CASTSPELL ) || ( btnCast.isEnabled() && le.MouseClickLeft( btnCast.area() ) ) )
            result = 1;

        if ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_RETREAT ) || ( btnRetreat.isEnabled() && le.MouseClickLeft( btnRetreat.area() ) ) )
            result = 2;

        if ( Game::HotKeyPressEvent( Game::EVENT_BATTLE_SURRENDER ) || ( btnSurrender.isEnabled() && le.MouseClickLeft( btnSurrender.area() ) ) )
            result = 3;

        if ( le.MousePressRight( btnCast.area() ) )
            Dialog::Message( _( "Cast Spell" ),
                             _( "Cast a magical spell. You may only cast one spell per combat round. The round is reset when every creature has had a turn." ),
                             Font::BIG );
        else if ( le.MousePressRight( btnRetreat.area() ) )
            Dialog::Message(
                _( "Retreat" ),
                _( "Retreat your hero, abandoning your creatures. Your hero will be available for you to recruit again, however, the hero will have only a novice hero's forces." ),
                Font::BIG );
        else if ( le.MousePressRight( btnSurrender.area() ) )
            Dialog::Message(
                _( "Surrender" ),
                _( "Surrendering costs gold. However if you pay the ransom, the hero and all of his or her surviving creatures will be available to recruit again." ),
                Font::BIG );
        else if ( le.MousePressRight( btnClose.area() ) )
            Dialog::Message( _( "Cancel" ), _( "Return to the battle." ), Font::BIG );

        // exit
        if ( HotKeyCloseWindow || le.MouseClickLeft( btnClose.area() ) )
            break;
    }

    return result;
}

bool Battle::DialogBattleSurrender( const HeroBase & hero, u32 cost, const Kingdom & kingdom )
{
    if ( kingdom.GetColor() == hero.GetColor() ) // this is weird. You're surrending to yourself!
        return false;

    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    Settings & conf = Settings::Get();

    cursor.Hide();
    cursor.SetThemes( Cursor::POINTER );

    const fheroes2::Sprite & dialog = fheroes2::AGG::GetICN( conf.ExtGameEvilInterface() ? ICN::SURDRBKE : ICN::SURDRBKG, 0 );

    fheroes2::Rect pos_rt( ( display.width() - dialog.width() + 16 ) / 2, ( display.height() - dialog.height() + 16 ) / 2, dialog.width(), dialog.height() );

    fheroes2::ImageRestorer back( display, pos_rt.x, pos_rt.y, pos_rt.width, pos_rt.height );

    fheroes2::Blit( dialog, display, pos_rt.x, pos_rt.y );

    const int icn = conf.ExtGameEvilInterface() ? ICN::SURRENDE : ICN::SURRENDR;

    fheroes2::Button btnAccept( pos_rt.x + 91, pos_rt.y + 152, icn, 0, 1 );
    fheroes2::Button btnDecline( pos_rt.x + 295, pos_rt.y + 152, icn, 2, 3 );
    fheroes2::Button btnMarket( pos_rt.x + ( pos_rt.width - 16 ) / 2, pos_rt.y + 145, ( conf.ExtGameEvilInterface() ? ICN::ADVEBTNS : ICN::ADVBTNS ), 4, 5 );
    Rect marketRect = btnAccept.area();

    if ( !kingdom.AllowPayment( payment_t( Resource::GOLD, cost ) ) ) {
        btnAccept.disable();
    }

    if ( kingdom.GetCountMarketplace() ) {
        if ( kingdom.AllowPayment( payment_t( Resource::GOLD, cost ) ) )
            btnMarket.disable();
        else {
            std::string msg = _( "Not enough gold (%{gold})" );
            StringReplace( msg, "%{gold}", cost - kingdom.GetFunds().Get( Resource::GOLD ) );
            Text text( msg, Font::SMALL );
            text.Blit( marketRect.x + ( marketRect.w - text.w() ) / 2, marketRect.y - 15 );
            btnMarket.draw();
        }
    }

    btnAccept.draw();
    btnDecline.draw();

    const fheroes2::Sprite & window = fheroes2::AGG::GetICN( icn, 4 );
    fheroes2::Blit( window, display, pos_rt.x + 55, pos_rt.y + 32 );
    hero.PortraitRedraw( pos_rt.x + 60, pos_rt.y + 38, PORT_BIG, display );

    std::string str = _( "%{name} states:" );
    StringReplace( str, "%{name}", hero.GetName() );
    Text text( str, Font::BIG );
    text.Blit( pos_rt.x + 320 - text.w() / 2, pos_rt.y + 30 );

    str = _( "\"I will accept your surrender and grant you and your troops safe passage for the price of %{price} gold.\"" );
    StringReplace( str, "%{price}", cost );

    TextBox box( str, Font::BIG, 275 );
    box.Blit( pos_rt.x + 175, pos_rt.y + 50 );
    bool result = false;

    cursor.Show();
    display.render();

    while ( le.HandleEvents() && !result ) {
        if ( btnAccept.isEnabled() )
            le.MousePressLeft( btnAccept.area() ) ? btnAccept.drawOnPress() : btnAccept.drawOnRelease();
        le.MousePressLeft( btnDecline.area() ) ? btnDecline.drawOnPress() : btnDecline.drawOnRelease();

        if ( btnMarket.isEnabled() )
            le.MousePressLeft( marketRect ) ? btnMarket.drawOnPress() : btnMarket.drawOnRelease();

        if ( btnAccept.isEnabled() && le.MouseClickLeft( btnAccept.area() ) )
            result = true;

        if ( btnMarket.isEnabled() && le.MouseClickLeft( marketRect ) ) {
            Dialog::Marketplace( false );

            if ( kingdom.AllowPayment( payment_t( Resource::GOLD, cost ) ) ) {
                btnAccept.release();
                btnAccept.enable();
            }
        }

        // exit
        if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( btnDecline.area() ) )
            break;
    }

    return result;
}
