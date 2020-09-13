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
#include <cmath>
#include <map>

#include "agg.h"
#include "ai.h"
#include "battle.h"
#include "buildinginfo.h"
#include "castle.h"
#include "cursor.h"
#include "difficulty.h"
#include "game.h"
#include "game_interface.h"
#include "game_static.h"
#include "ground.h"
#include "kingdom.h"
#include "maps_tiles.h"
#include "monster.h"
#include "mp2.h"
#include "mus.h"
#include "payment.h"
#include "profit.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "system.h"
#include "test.h"
#include "tinyconfig.h"
#include "tools.h"
#include "world.h"

namespace Game
{
    u32 GetMixerChannelFromObject( const Maps::Tiles & );
    void AnimateDelaysInitialize( void );
    void KeyboardGlobalFilter( int, int );
    void UpdateGlobalDefines( const std::string & );
    void LoadExternalResource( const Settings & );

    void HotKeysDefaults( void );
    void HotKeysLoad( const std::string & );

    bool disable_change_music = false;
    int current_music = MUS::UNKNOWN;
    u32 castle_animation_frame = 0;
    u32 maps_animation_frame = 0;
    std::string last_name;
    int save_version = CURRENT_FORMAT_VERSION;
    std::vector<int> reserved_vols( LOOPXX_COUNT, 0 );

    namespace ObjectFadeAnimation
    {
        Info::Info()
            : object( MP2::OBJ_ZERO )
            , index( 0 )
            , tile( 0 )
            , alpha( 255 )
            , isFadeOut( true )
        {}

        Info::Info( u8 object_, u8 index_, s32 tile_, u32 alpha_, bool fadeOut )
            : object( object_ )
            , tile( tile_ )
            , alpha( alpha_ )
            , isFadeOut( fadeOut )
        {
            const fheroes2::Image & tileImage = world.GetTiles( tile_ ).GetTileSurface();
            surfaceSize.w = tileImage.width();
            surfaceSize.h = tileImage.height();

            index = ICN::AnimationFrame( MP2::GetICNObject( object ), index_, 0 );
            if ( 0 == index ) {
                index = index_;
            }
        }

        Info removeInfo;
    }
}

void Game::SetLoadVersion( int ver )
{
    save_version = ver;
}

int Game::GetLoadVersion( void )
{
    return save_version;
}

const std::string & Game::GetLastSavename( void )
{
    return last_name;
}

void Game::SetLastSavename( const std::string & name )
{
    last_name = name;
}

int Game::Testing( int t )
{
#ifndef BUILD_RELEASE
    Test::Run( t );
    return Game::QUITGAME;
#else
    return Game::MAINMENU;
#endif
}

int Game::Credits( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Dialog::FrameBorder border( Size( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT ) );

    const fheroes2::Point screenOffset( ( display.width() - display.DEFAULT_WIDTH ) / 2, ( display.height() - display.DEFAULT_HEIGHT ) / 2 );

    const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::CBKGLAVA, 0 );
    fheroes2::Blit( background, display, screenOffset.x, screenOffset.y + display.DEFAULT_HEIGHT - background.height() );

    fheroes2::Fill( display, screenOffset.x, screenOffset.y, display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT - background.height(), 0 );

    Text caption( "Free Heroes of Might and Magic II (" + Settings::Get().GetVersion() + ")", Font::YELLOW_BIG );
    caption.Blit( screenOffset.x + display.DEFAULT_WIDTH / 2 - caption.w() / 2, screenOffset.y + 15 );

    const int32_t columnStep = 210;
    const int32_t textInitialOffsetY = 40;
    const int32_t textWidth = 200;

    int32_t offsetY = screenOffset.y + textInitialOffsetY;

    TextBox title( "Project Coordination and Core Development", Font::YELLOW_BIG, textWidth );
    TextBox name( "Ihar Hubchyk", Font::BIG, textWidth );
    title.Blit( screenOffset.x + ( columnStep - title.w() ) / 2, offsetY );
    name.Blit( screenOffset.x + ( columnStep - name.w() ) / 2, offsetY + title.h() );
    offsetY += title.h() + name.h() + 10;

    const fheroes2::Sprite & blackDragon = fheroes2::AGG::GetICN( ICN::DRAGBLAK, 5 );
    fheroes2::Blit( blackDragon, display, screenOffset.x + ( columnStep - blackDragon.width() ) / 2, offsetY );
    offsetY += blackDragon.height();

    title.Set( "QA and Support", Font::YELLOW_BIG, textWidth );
    name.Set( "Ihar Tsyvilka", Font::BIG, textWidth );
    title.Blit( screenOffset.x + ( columnStep - title.w() ) / 2, offsetY );
    name.Blit( screenOffset.x + ( columnStep - name.w() ) / 2, offsetY + title.h() );
    offsetY += title.h() + name.h() + 10;

    const fheroes2::Sprite & cyclop = fheroes2::AGG::GetICN( ICN::CYCLOPS, 38 );
    fheroes2::Blit( cyclop, display, screenOffset.x + ( columnStep - cyclop.width() ) / 2, offsetY );
    offsetY += cyclop.height();

    title.Set( "Development", Font::YELLOW_BIG, textWidth );
    name.Set( "Ivan Shibanov", Font::BIG, textWidth );
    title.Blit( screenOffset.x + ( columnStep - title.w() ) / 2, offsetY );
    name.Blit( screenOffset.x + ( columnStep - name.w() ) / 2, offsetY + title.h() );
    offsetY += title.h() + name.h() + 10;

    const fheroes2::Sprite & crusader = fheroes2::AGG::GetICN( ICN::PALADIN2, 23 );
    fheroes2::Blit( crusader, display, screenOffset.x + ( columnStep - crusader.width() ) / 2, offsetY );
    offsetY += crusader.height();

    offsetY += 10;

    const Text websiteInto( "Visit us at ", Font::BIG );
    const Text website( "https://github.com/ihhub/fheroes2", Font::YELLOW_BIG );
    const int32_t websiteOffsetX = screenOffset.x + ( display.DEFAULT_WIDTH - websiteInto.w() - website.w() ) / 2;
    websiteInto.Blit( websiteOffsetX, offsetY );
    website.Blit( websiteOffsetX + websiteInto.w(), offsetY );

    const fheroes2::Sprite & missile = fheroes2::AGG::GetICN( ICN::ARCH_MSL, 4 );
    fheroes2::Blit( missile, display, websiteOffsetX - 10 - missile.width(), offsetY + website.h() / 2 - missile.height() / 2 );
    fheroes2::Blit( missile, display, websiteOffsetX + websiteInto.w() + website.w() + 10, offsetY + website.h() / 2 - missile.height() / 2, true );

    offsetY = screenOffset.y + textInitialOffsetY;

    title.Set( "Special Thanks to", Font::YELLOW_BIG, textWidth );
    title.Blit( screenOffset.x + 2 * columnStep + ( columnStep - title.w() ) / 2, offsetY );
    offsetY += title.h();

    const std::string contributors(
        "LeHerosInconnu\nPavel aka shprotru\nAndrey Starodubtsev\nVasilenko Alexey\nKrzysztof Gorecki\nghostBot\nPalash Bansal\nMaria Sopkova\nHarri Nieminen\n"
        "and many other contributors!" );

    name.Set( contributors, Font::BIG, textWidth );
    name.Blit( screenOffset.x + 2 * columnStep + ( columnStep - name.w() ) / 2, offsetY );
    offsetY += name.h() + 10;

    const fheroes2::Sprite & hydra = fheroes2::AGG::GetICN( ICN::HYDRA, 11 );
    fheroes2::Blit( hydra, display, screenOffset.x + 2 * columnStep + ( columnStep - hydra.width() ) / 2, offsetY );
    offsetY += hydra.height();

    title.Set( "Original project before 0.7", Font::YELLOW_SMALL, textWidth );
    title.Blit( screenOffset.x + 2 * columnStep + ( columnStep - title.w() ) / 2, offsetY );
    offsetY += title.h();

    name.Set( "Andrey Afletdinov\nhttps://sourceforge.net/\nprojects/fheroes2/", Font::SMALL, textWidth );
    name.Blit( screenOffset.x + 2 * columnStep + ( columnStep - name.w() ) / 2, offsetY );
    offsetY += name.h();

    const fheroes2::Sprite & goblin = fheroes2::AGG::GetICN( ICN::GOBLIN, 27 );
    fheroes2::Blit( goblin, display, screenOffset.x + ( display.DEFAULT_WIDTH - goblin.width() ) / 2, screenOffset.y + ( display.DEFAULT_HEIGHT - goblin.height() ) / 2 );

    AGG::PlayMusic( MUS::VICTORY );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() )
            break;
    }

    return Game::MAINMENU;
}

bool Game::ChangeMusicDisabled( void )
{
    return disable_change_music;
}

void Game::DisableChangeMusic( bool f )
{
    // disable_change_music = f;
}

void Game::Init( void )
{
    Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();

    // update all global defines
    if ( conf.UseAltResource() )
        LoadExternalResource( conf );

    // default events
    le.SetStateDefaults();

    // set global events
    le.SetGlobalFilterMouseEvents( Cursor::Redraw );
    le.SetGlobalFilterKeysEvents( Game::KeyboardGlobalFilter );
    le.SetGlobalFilter( true );

    le.SetTapMode( conf.ExtPocketTapMode() );

    Game::AnimateDelaysInitialize();

    HotKeysDefaults();

    const std::string hotkeys = Settings::GetLastFile( "", "fheroes2.key" );
    Game::HotKeysLoad( hotkeys );
}

int Game::CurrentMusic( void )
{
    return current_music;
}

void Game::SetCurrentMusic( int mus )
{
    current_music = mus;
}

void Game::ObjectFadeAnimation::Set( const Info & info )
{
    removeInfo = info;
}

Game::ObjectFadeAnimation::Info & Game::ObjectFadeAnimation::Get()
{
    return removeInfo;
}

u32 & Game::MapsAnimationFrame( void )
{
    return maps_animation_frame;
}

u32 & Game::CastleAnimationFrame( void )
{
    return castle_animation_frame;
}

/* play all sound from focus area game */
void Game::EnvironmentSoundMixer( void )
{
    const Point abs_pt( Interface::GetFocusCenter() );
    const Settings & conf = Settings::Get();

    if ( conf.Sound() ) {
        std::fill( reserved_vols.begin(), reserved_vols.end(), 0 );

        // scan 4x4 square from focus
        for ( s32 yy = abs_pt.y - 3; yy <= abs_pt.y + 3; ++yy ) {
            for ( s32 xx = abs_pt.x - 3; xx <= abs_pt.x + 3; ++xx ) {
                if ( Maps::isValidAbsPoint( xx, yy ) ) {
                    const u32 channel = GetMixerChannelFromObject( world.GetTiles( xx, yy ) );
                    if ( channel < reserved_vols.size() ) {
                        // calculation volume
                        const int length = std::max( std::abs( xx - abs_pt.x ), std::abs( yy - abs_pt.y ) );
                        const int volume = ( 2 < length ? 4 : ( 1 < length ? 8 : ( 0 < length ? 12 : 16 ) ) ) * Mixer::MaxVolume() / 16;

                        if ( volume > reserved_vols[channel] )
                            reserved_vols[channel] = volume;
                    }
                }
            }
        }

        AGG::LoadLOOPXXSounds( reserved_vols );
    }
}

u32 Game::GetMixerChannelFromObject( const Maps::Tiles & tile )
{
    // force: check stream
    if ( tile.isStream() )
        return 13;

    return M82::GetIndexLOOP00XXFromObject( tile.GetObject( false ) );
}

u32 Game::GetRating( void )
{
    Settings & conf = Settings::Get();
    u32 rating = 50;

    switch ( conf.MapsDifficulty() ) {
    case Difficulty::NORMAL:
        rating += 20;
        break;
    case Difficulty::HARD:
        rating += 40;
        break;
    case Difficulty::EXPERT:
    case Difficulty::IMPOSSIBLE:
        rating += 80;
        break;
    default:
        break;
    }

    switch ( conf.GameDifficulty() ) {
    case Difficulty::NORMAL:
        rating += 30;
        break;
    case Difficulty::HARD:
        rating += 50;
        break;
    case Difficulty::EXPERT:
        rating += 70;
        break;
    case Difficulty::IMPOSSIBLE:
        rating += 90;
        break;
    default:
        break;
    }

    return rating;
}

u32 Game::GetGameOverScores( void )
{
    Settings & conf = Settings::Get();

    u32 k_size = 0;

    switch ( conf.MapsSize().w ) {
    case Maps::SMALL:
        k_size = 140;
        break;
    case Maps::MEDIUM:
        k_size = 100;
        break;
    case Maps::LARGE:
        k_size = 80;
        break;
    case Maps::XLARGE:
        k_size = 60;
        break;
    default:
        break;
    }

    u32 flag = 0;
    u32 nk = 0;
    u32 end_days = world.CountDay();

    for ( u32 ii = 1; ii <= end_days; ++ii ) {
        nk = ii * k_size / 100;

        if ( 0 == flag && nk > 60 ) {
            end_days = ii + ( world.CountDay() - ii ) / 2;
            flag = 1;
        }
        else if ( 1 == flag && nk > 120 )
            end_days = ii + ( world.CountDay() - ii ) / 2;
        else if ( nk > 180 )
            break;
    }

    return GetRating() * ( 200 - nk ) / 100;
}

void Game::ShowLoadMapsText( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Rect pos( 0, display.height() / 2, display.width(), display.height() / 2 );
    TextBox text( _( "Maps Loading..." ), Font::BIG, pos.w );

    // blit test
    display.fill( 0 );
    text.Blit( pos );
    display.render();
}

u32 Game::GetLostTownDays( void )
{
    return GameStatic::GetGameOverLostDays();
}

u32 Game::GetViewDistance( u32 d )
{
    return GameStatic::GetOverViewDistance( d );
}

void Game::UpdateGlobalDefines( const std::string & spec )
{
#ifdef WITH_XML
    // parse profits.xml
    TiXmlDocument doc;
    const TiXmlElement * xml_globals = NULL;

    if ( doc.LoadFile( spec.c_str() ) && NULL != ( xml_globals = doc.FirstChildElement( "globals" ) ) ) {
        // starting_resource
        KingdomUpdateStartingResource( xml_globals->FirstChildElement( "starting_resource" ) );
        // view_distance
        OverViewUpdateStatic( xml_globals->FirstChildElement( "view_distance" ) );
        // kingdom
        KingdomUpdateStatic( xml_globals->FirstChildElement( "kingdom" ) );
        // game_over
        GameOverUpdateStatic( xml_globals->FirstChildElement( "game_over" ) );
        // whirlpool
        WhirlpoolUpdateStatic( xml_globals->FirstChildElement( "whirlpool" ) );
        // heroes
        HeroesUpdateStatic( xml_globals->FirstChildElement( "heroes" ) );
        // castle_extra_growth
        CastleUpdateGrowth( xml_globals->FirstChildElement( "castle_extra_growth" ) );
        // monster upgrade ratio
        MonsterUpdateStatic( xml_globals->FirstChildElement( "monster_upgrade" ) );
    }
    else
        VERBOSE( spec << ": " << doc.ErrorDesc() );
#endif
}

u32 Game::GetWhirlpoolPercent( void )
{
    return GameStatic::GetLostOnWhirlpoolPercent();
}

void Game::LoadExternalResource( const Settings & conf )
{
    std::string spec;
    const std::string prefix_stats = System::ConcatePath( "files", "stats" );

    // globals.xml
    spec = Settings::GetLastFile( prefix_stats, "globals.xml" );

    if ( System::IsFile( spec ) )
        Game::UpdateGlobalDefines( spec );

    // battle.xml
    spec = Settings::GetLastFile( prefix_stats, "battle.xml" );

    if ( System::IsFile( spec ) )
        Battle::UpdateMonsterAttributes( spec );

    // monsters.xml
    spec = Settings::GetLastFile( prefix_stats, "monsters.xml" );

    if ( System::IsFile( spec ) )
        Monster::UpdateStats( spec );

    // spells.xml
    spec = Settings::GetLastFile( prefix_stats, "spells.xml" );

    if ( System::IsFile( spec ) )
        Spell::UpdateStats( spec );

    // artifacts.xml
    spec = Settings::GetLastFile( prefix_stats, "artifacts.xml" );

    if ( System::IsFile( spec ) )
        Artifact::UpdateStats( spec );

    // buildings.xml
    spec = Settings::GetLastFile( prefix_stats, "buildings.xml" );

    if ( System::IsFile( spec ) )
        BuildingInfo::UpdateCosts( spec );

    // payments.xml
    spec = Settings::GetLastFile( prefix_stats, "payments.xml" );

    if ( System::IsFile( spec ) )
        PaymentConditions::UpdateCosts( spec );

    // profits.xml
    spec = Settings::GetLastFile( prefix_stats, "profits.xml" );

    if ( System::IsFile( spec ) )
        ProfitConditions::UpdateCosts( spec );

    // skills.xml
    spec = Settings::GetLastFile( prefix_stats, "skills.xml" );

    if ( System::IsFile( spec ) )
        Skill::UpdateStats( spec );
}

std::string Game::GetEncodeString( const std::string & str1 )
{
    const Settings & conf = Settings::Get();

    // encode name
    if ( conf.Unicode() && conf.MapsCharset().size() )
        return EncodeString( str1.c_str(), conf.MapsCharset().c_str() );

    return str1;
}

int Game::GetKingdomColors( void )
{
    return Settings::Get().GetPlayers().GetColors();
}

int Game::GetActualKingdomColors( void )
{
    return Settings::Get().GetPlayers().GetActualColors();
}

std::string Game::CountScoute( u32 count, int scoute, bool shorts )
{
    double infelicity = 0;
    std::string res;

    switch ( scoute ) {
    case Skill::Level::BASIC:
        infelicity = count * 30 / 100.0;
        break;

    case Skill::Level::ADVANCED:
        infelicity = count * 15 / 100.0;
        break;

    case Skill::Level::EXPERT:
        res = shorts ? GetStringShort( count ) : GetString( count );
        break;

    default:
        return Army::SizeString( count );
    }

    if ( res.empty() ) {
        u32 min = Rand::Get( static_cast<u32>( std::floor( count - infelicity + 0.5 ) ), static_cast<u32>( std::floor( count + infelicity + 0.5 ) ) );
        u32 max = 0;

        if ( min > count ) {
            max = min;
            min = static_cast<u32>( std::floor( count - infelicity + 0.5 ) );
        }
        else
            max = static_cast<u32>( std::floor( count + infelicity + 0.5 ) );

        res = GetString( min );

        if ( min != max ) {
            res.append( "-" );
            res.append( GetString( max ) );
        }
    }

    return res;
}

void Game::PlayPickupSound( void )
{
    int wav = M82::UNKNOWN;

    switch ( Rand::Get( 1, 7 ) ) {
    case 1:
        wav = M82::PICKUP01;
        break;
    case 2:
        wav = M82::PICKUP02;
        break;
    case 3:
        wav = M82::PICKUP03;
        break;
    case 4:
        wav = M82::PICKUP04;
        break;
    case 5:
        wav = M82::PICKUP05;
        break;
    case 6:
        wav = M82::PICKUP06;
        break;
    case 7:
        wav = M82::PICKUP07;
        break;

    default:
        return;
    }

    AGG::PlaySound( wav );
}
