/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_over.h"
#include "gamedefs.h"
#include "mus.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "ui_button.h"
#include "world.h"
#include "zzlib.h"

#define HGS_ID 0xF1F3
#define HGS_MAX 10

struct hgs_t
{
    hgs_t()
        : days( 0 )
        , rating( 0 ){};

    bool operator==( const hgs_t & ) const;

    std::string player;
    std::string land;
    u32 localtime;
    u32 days;
    u32 rating;
};

StreamBase & operator<<( StreamBase & msg, const hgs_t & hgs )
{
    return msg << hgs.player << hgs.land << hgs.localtime << hgs.days << hgs.rating;
}

StreamBase & operator>>( StreamBase & msg, hgs_t & hgs )
{
    return msg >> hgs.player >> hgs.land >> hgs.localtime >> hgs.days >> hgs.rating;
}

bool hgs_t::operator==( const hgs_t & h ) const
{
    return player == h.player && land == h.land && days == h.days;
}

bool RatingSort( const hgs_t & h1, const hgs_t & h2 )
{
    return h1.rating > h2.rating;
}

class HGSData
{
public:
    HGSData() {}

    bool Load( const std::string & );
    bool Save( const std::string & );
    void ScoreRegistry( const std::string &, const std::string &, u32, u32 );
    void RedrawList( int32_t ox, int32_t oy );

private:
    std::vector<hgs_t> list;
};

bool HGSData::Load( const std::string & fn )
{
    ZStreamFile hdata;
    if ( !hdata.read( fn ) )
        return false;

    hdata.setbigendian( true );
    u16 hgs_id = 0;

    hdata >> hgs_id;

    if ( hgs_id == HGS_ID ) {
        hdata >> list;
        return !hdata.fail();
    }

    return false;
}

bool HGSData::Save( const std::string & fn )
{
    ZStreamFile hdata;
    hdata.setbigendian( true );
    hdata << static_cast<u16>( HGS_ID ) << list;
    if ( hdata.fail() || !hdata.write( fn ) )
        return false;

    return true;
}

void HGSData::ScoreRegistry( const std::string & p, const std::string & m, u32 r, u32 s )
{
    hgs_t h;

    h.player = p;
    h.land = m;
    h.localtime = std::time( NULL );
    h.days = r;
    h.rating = s;

    if ( list.end() == std::find( list.begin(), list.end(), h ) ) {
        list.push_back( h );
        std::sort( list.begin(), list.end(), RatingSort );
        if ( list.size() > HGS_MAX )
            list.resize( HGS_MAX );
    }
}

void HGSData::RedrawList( int32_t ox, int32_t oy )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // image background
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HSBKG, 0 ), display, ox, oy );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HISCORE, 6 ), display, ox + 50, oy + 31 );

    std::sort( list.begin(), list.end(), RatingSort );

    std::vector<hgs_t>::const_iterator it1 = list.begin();
    std::vector<hgs_t>::const_iterator it2 = list.end();

    Text text;
    text.Set( Font::BIG );

    for ( ; it1 != it2 && ( it1 - list.begin() < HGS_MAX ); ++it1 ) {
        const hgs_t & hgs = *it1;

        text.Set( hgs.player );
        text.Blit( ox + 88, oy + 70 );

        text.Set( hgs.land );
        text.Blit( ox + 260, oy + 70 );

        text.Set( GetString( hgs.days ) );
        text.Blit( ox + 420, oy + 70 );

        text.Set( GetString( hgs.rating ) );
        text.Blit( ox + 480, oy + 70 );

        oy += 40;
    }
}

int Game::HighScores()
{
#ifdef WITH_DEBUG
    if ( IS_DEVEL() && world.CountDay() ) {
        std::string msg = std::string( "Developer mode, not save! \n \n Your result: " ) + GetString( GetGameOverScores() );
        Dialog::Message( "High Scores", msg, Font::BIG, Dialog::OK );
        return MAINMENU;
    }
#endif

    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    HGSData hgs;

    std::ostringstream stream;
    stream << System::ConcatePath( GetSaveDir(), "fheroes2.hgs" );

    cursor.SetThemes( cursor.POINTER );
    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU );
    hgs.Load( stream.str().c_str() );

    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HSBKG, 0 );

    cursor.Hide();

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point top( ( display.width() - back.width() ) / 2, ( display.height() - back.height() ) / 2 );
    Dialog::FrameBorder border( Size( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT ) );

    hgs.RedrawList( top.x, top.y );

    fheroes2::Button buttonCampain( top.x + 8, top.y + 315, ICN::HISCORE, 0, 1 );
    fheroes2::Button buttonExit( top.x + back.width() - 36, top.y + 315, ICN::HISCORE, 4, 5 );

    buttonCampain.disable(); // disable for now till full campaign support

    buttonCampain.draw();
    buttonExit.draw();

    cursor.Show();
    display.render();

    const u32 rating = GetGameOverScores();
    const u32 days = world.CountDay();
    GameOver::Result & gameResult = GameOver::Result::Get();

    if ( rating && ( gameResult.GetResult() & GameOver::WINS ) ) {
        std::string player( _( "Unknown Hero" ) );
        Dialog::InputString( _( "Your Name" ), player );
        cursor.Hide();
        if ( player.empty() )
            player = _( "Unknown Hero" );
        hgs.ScoreRegistry( player, Settings::Get().CurrentFileInfo().name, days, rating );
        hgs.Save( stream.str().c_str() );
        hgs.RedrawList( top.x, top.y );
        buttonCampain.draw();
        buttonExit.draw();
        cursor.Show();
        display.render();
        gameResult.Reset();
    }

    LocalEvent & le = LocalEvent::Get();

    // highscores loop
    while ( le.HandleEvents() ) {
        // key code info
        if ( Settings::Get().Debug() == 0x12 && le.KeyPress() )
            Dialog::Message( "Key Press:", GetString( le.KeyValue() ), Font::SMALL, Dialog::OK );
        if ( buttonCampain.isEnabled() ) {
            le.MousePressLeft( buttonCampain.area() ) ? buttonCampain.drawOnPress() : buttonCampain.drawOnRelease();
        }
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            return MAINMENU;
    }

    return QUITGAME;
}
