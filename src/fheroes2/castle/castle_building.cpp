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

#include "agg.h"
#include "castle.h"
#include "cursor.h"
#include "game.h"
#include "race.h"
#include "settings.h"
#include "text.h"

void CastleRedrawTownName( const Castle &, const Point & );
void CastleRedrawCurrentBuilding( const Castle &, const Point &, const CastleDialog::CacheBuildings &, u32 build );
fheroes2::Rect CastleGetCoordBuilding( int, building_t, const Point & );
void CastlePackOrdersBuildings( const Castle &, std::vector<building_t> & );
Rect CastleGetMaxArea( const Castle &, const Point & );

void CastleDialog::RedrawBuildingSpriteToArea( const fheroes2::Sprite & sprite, s32 dst_x, s32 dst_y, const Rect & max, uint8_t alpha )
{
    std::pair<Rect, Point> res = Rect::Fixed4Blit( Rect( dst_x, dst_y, sprite.width(), sprite.height() ), max );
    fheroes2::AlphaBlit( sprite, res.first.x, res.first.y, fheroes2::Display::instance(), res.second.x, res.second.y, res.first.w, res.first.h, alpha );
}

CastleDialog::CacheBuildings::CacheBuildings( const Castle & castle, const Point & top )
{
    std::vector<building_t> ordersBuildings;

    ordersBuildings.reserve( 25 );

    CastlePackOrdersBuildings( castle, ordersBuildings );

    for ( std::vector<building_t>::const_iterator it = ordersBuildings.begin(); it != ordersBuildings.end(); ++it ) {
        push_back( builds_t( *it, CastleGetCoordBuilding( castle.GetRace(), *it, top ) ) );
    }
}

const Rect & CastleDialog::CacheBuildings::GetRect( building_t b ) const
{
    const_iterator it = std::find( begin(), end(), b );
    return it != end() ? ( *it ).coord : back().coord;
}

void CastleDialog::RedrawAnimationBuilding( const Castle & castle, const Point & dst_pt, const CacheBuildings & orders, u32 build )
{
    Cursor::Get().Hide();
    CastleRedrawCurrentBuilding( castle, dst_pt, orders, build );
}

void CastleDialog::RedrawAllBuilding( const Castle & castle, const Point & dst_pt, const CacheBuildings & orders )
{
    CastleRedrawCurrentBuilding( castle, dst_pt, orders, BUILD_NOTHING );
    CastleRedrawTownName( castle, dst_pt );
}

void CastleRedrawTownName( const Castle & castle, const Point & dst )
{
    const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::TOWNNAME, 0 );
    Point dst_pt( dst.x + 320 - background.width() / 2, dst.y + 248 );
    fheroes2::Blit( background, fheroes2::Display::instance(), dst_pt.x, dst_pt.y );

    Text text( castle.GetName(), Font::SMALL );
    dst_pt.x = dst.x + 320 - text.w() / 2;
    dst_pt.y = dst.y + 248;
    text.Blit( dst_pt );
}

void CastleRedrawCurrentBuilding( const Castle & castle, const Point & dst_pt, const CastleDialog::CacheBuildings & orders, u32 build )
{
    const uint32_t frame = Game::CastleAnimationFrame();

    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();

    int townIcnId = -1;

    switch ( castle.GetRace() ) {
    case Race::KNGT:
        townIcnId = ICN::TOWNBKG0;
        break;
    case Race::BARB:
        townIcnId = ICN::TOWNBKG1;
        break;
    case Race::SORC:
        townIcnId = ICN::TOWNBKG2;
        break;
    case Race::WRLK:
        townIcnId = ICN::TOWNBKG3;
        break;
    case Race::WZRD:
        townIcnId = ICN::TOWNBKG4;
        break;
    case Race::NECR:
        townIcnId = ICN::TOWNBKG5;
        break;
    default:
        break;
    }

    const Rect max = CastleGetMaxArea( castle, dst_pt );

    if ( townIcnId != -1 ) {
        const fheroes2::Sprite & townbkg = fheroes2::AGG::GetICN( townIcnId, 0 );
        fheroes2::Copy( townbkg, 0, 0, display, dst_pt.x, dst_pt.y, townbkg.width(), townbkg.height() );
    }

    if ( Race::BARB == castle.GetRace() ) {
        const fheroes2::Sprite & sprite0 = fheroes2::AGG::GetICN( ICN::TWNBEXT1, 1 + frame % 5 );
        fheroes2::Blit( sprite0, display, dst_pt.x + sprite0.x(), dst_pt.y + sprite0.y() );
    }

    // Bay animation
    if ( Race::WZRD == castle.GetRace() || ( !castle.isBuild( BUILD_SHIPYARD ) && castle.HaveNearlySea() ) ) {
        fheroes2::Sprite sprite50;
        fheroes2::Sprite sprite51;

        switch ( castle.GetRace() ) {
        case Race::KNGT:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNKEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNKEXT0, 1 + frame % 5 );
            break;
        case Race::BARB:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNBEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNBEXT0, 1 + frame % 5 );
            break;
        case Race::SORC:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNSEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNSEXT0, 1 + frame % 5 );
            break;
        case Race::NECR:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNNEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNNEXT0, 1 + frame % 5 );
            break;
        case Race::WRLK:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNWEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNWEXT0, 1 + frame % 5 );
            break;
        case Race::WZRD:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNZEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNZEXT0, 1 + frame % 5 );
            break;
        default:
            break;
        }

        if ( !sprite50.empty() )
            CastleDialog::RedrawBuildingSpriteToArea( sprite50, dst_pt.x + sprite50.x(), dst_pt.y + sprite50.y(), max );

        if ( !sprite51.empty() )
            CastleDialog::RedrawBuildingSpriteToArea( sprite51, dst_pt.x + sprite51.x(), dst_pt.y + sprite51.y(), max );
    }

    // redraw all builds
    if ( BUILD_NOTHING == build ) {
        for ( CastleDialog::CacheBuildings::const_iterator it = orders.begin(); it != orders.end(); ++it ) {
            const uint32_t currentBuildId = it->id;
            if ( castle.isBuild( currentBuildId ) ) {
                CastleDialog::CastleRedrawBuilding( castle, dst_pt, currentBuildId, frame );
                CastleDialog::CastleRedrawBuildingExtended( castle, dst_pt, currentBuildId, frame );
                if ( CastleDialog::RoadConnectionNeeded( castle, currentBuildId, false ) ) {
                    CastleDialog::RedrawRoadConnection( castle, dst_pt, currentBuildId );
                }
            }
        }
    }
    // redraw build with alpha
    else if ( orders.end() != std::find( orders.begin(), orders.end(), build ) ) {
        LocalEvent & le = LocalEvent::Get();
        int alpha = 1;
        uint32_t buildFrame = 0;

        while ( le.HandleEvents() && alpha < 255 ) {
            if ( Game::AnimateInfrequentDelay( Game::CASTLE_BUILD_DELAY ) ) {
                cursor.Hide();

                for ( CastleDialog::CacheBuildings::const_iterator it = orders.begin(); it != orders.end(); ++it ) {
                    const uint32_t currentBuildId = it->id;

                    if ( castle.isBuild( currentBuildId ) ) {
                        CastleDialog::CastleRedrawBuilding( castle, dst_pt, currentBuildId, frame );
                        CastleDialog::CastleRedrawBuildingExtended( castle, dst_pt, currentBuildId, frame );
                        if ( CastleDialog::RoadConnectionNeeded( castle, currentBuildId, false ) ) {
                            CastleDialog::RedrawRoadConnection( castle, dst_pt, build, alpha );
                            CastleDialog::RedrawRoadConnection( castle, dst_pt, currentBuildId );
                        }
                    }
                    else if ( currentBuildId == build ) {
                        CastleDialog::CastleRedrawBuilding( castle, dst_pt, currentBuildId, buildFrame, alpha );
                        CastleDialog::CastleRedrawBuildingExtended( castle, dst_pt, currentBuildId, frame, alpha );
                        if ( CastleDialog::RoadConnectionNeeded( castle, currentBuildId, true ) ) {
                            CastleDialog::RedrawRoadConnection( castle, dst_pt, currentBuildId, alpha );
                        }
                    }
                }

                alpha += 15;

                CastleRedrawTownName( castle, dst_pt );

                cursor.Show();
                display.render();
            }
            ++buildFrame;
        }

        cursor.Hide();
    }
}

void CastleDialog::CastleRedrawBuilding( const Castle & castle, const Point & dst_pt, u32 build, u32 frame, uint8_t alpha )
{
    if ( build == BUILD_TENT ) // we don't need to draw a tent as it's on the background image
        return;

    const Rect max = CastleGetMaxArea( castle, dst_pt );

    // correct build
    switch ( build ) {
    case DWELLING_MONSTER2:
    case DWELLING_MONSTER3:
    case DWELLING_MONSTER4:
    case DWELLING_MONSTER5:
    case DWELLING_MONSTER6:
        build = castle.GetActualDwelling( build );
        break;

    default:
        break;
    }

    const int race = castle.GetRace();
    const int icn = Castle::GetICNBuilding( build, race );
    u32 index = 0;

    // correct index (mage guild)
    switch ( build ) {
    case BUILD_MAGEGUILD1:
        if ( castle.GetLevelMageGuild() > 1 )
            return;
        index = 0;
        break;
    case BUILD_MAGEGUILD2:
        if ( castle.GetLevelMageGuild() > 2 )
            return;
        index = Race::NECR == race ? 6 : 1;
        break;
    case BUILD_MAGEGUILD3:
        if ( castle.GetLevelMageGuild() > 3 )
            return;
        index = Race::NECR == race ? 12 : 2;
        break;
    case BUILD_MAGEGUILD4:
        if ( castle.GetLevelMageGuild() > 4 )
            return;
        index = Race::NECR == race ? 18 : 3;
        break;
    case BUILD_MAGEGUILD5:
        index = Race::NECR == race ? 24 : 4;
        break;
    default:
        break;
    }

    if ( icn != ICN::UNKNOWN ) {
        // simple first sprite
        const fheroes2::Sprite & sprite1 = fheroes2::AGG::GetICN( icn, index );

        CastleDialog::RedrawBuildingSpriteToArea( sprite1, dst_pt.x + sprite1.x(), dst_pt.y + sprite1.y(), max, alpha );

        // second anime sprite
        if ( const u32 index2 = ICN::AnimationFrame( icn, index, frame ) ) {
            const fheroes2::Sprite & sprite2 = fheroes2::AGG::GetICN( icn, index2 );

            CastleDialog::RedrawBuildingSpriteToArea( sprite2, dst_pt.x + sprite2.x(), dst_pt.y + sprite2.y(), max, alpha );
        }
    }
}

void CastleDialog::CastleRedrawBuildingExtended( const Castle & castle, const Point & dst_pt, u32 build, u32 frame, uint8_t alpha )
{
    if ( build == BUILD_TENT ) // we don't need to draw a tent as it's on the background image
        return;

    const Rect max = CastleGetMaxArea( castle, dst_pt );
    int icn = Castle::GetICNBuilding( build, castle.GetRace() );

    // shipyard
    if ( BUILD_SHIPYARD == build ) {
        // boat
        if ( castle.PresentBoat() ) {
            const int icn2 = castle.GetICNBoat( castle.GetRace() );

            const fheroes2::Sprite & sprite40 = fheroes2::AGG::GetICN( icn2, 0 );
            CastleDialog::RedrawBuildingSpriteToArea( sprite40, dst_pt.x + sprite40.x(), dst_pt.y + sprite40.y(), max, alpha );

            if ( const u32 index2 = ICN::AnimationFrame( icn2, 0, frame ) ) {
                const fheroes2::Sprite & sprite41 = fheroes2::AGG::GetICN( icn2, index2 );
                CastleDialog::RedrawBuildingSpriteToArea( sprite41, dst_pt.x + sprite41.x(), dst_pt.y + sprite41.y(), max, alpha );
            }
        }
        else {
            if ( const u32 index2 = ICN::AnimationFrame( icn, 0, frame ) ) {
                const fheroes2::Sprite & sprite3 = fheroes2::AGG::GetICN( icn, index2 );
                CastleDialog::RedrawBuildingSpriteToArea( sprite3, dst_pt.x + sprite3.x(), dst_pt.y + sprite3.y(), max, alpha );
            }
        }
    }
    else if ( Race::SORC == castle.GetRace() && BUILD_WEL2 == build ) { // sorc and anime wel2 or statue
        const int icn2 = castle.isBuild( BUILD_STATUE ) ? ICN::TWNSEXT1 : icn;

        const fheroes2::Sprite & sprite20 = fheroes2::AGG::GetICN( icn2, 0 );
        CastleDialog::RedrawBuildingSpriteToArea( sprite20, dst_pt.x + sprite20.x(), dst_pt.y + sprite20.y(), max, alpha );

        if ( const u32 index2 = ICN::AnimationFrame( icn2, 0, frame ) ) {
            const fheroes2::Sprite & sprite21 = fheroes2::AGG::GetICN( icn2, index2 );
            CastleDialog::RedrawBuildingSpriteToArea( sprite21, dst_pt.x + sprite21.x(), dst_pt.y + sprite21.y(), max, alpha );
        }
    }
}

bool CastleDialog::RoadConnectionNeeded( const Castle & castle, const uint32_t buildId, const bool constructionInProgress )
{
    if ( Race::BARB == castle.GetRace() ) {
        if ( buildId & BUILD_MAGEGUILD ) {
            const int mageGuildLevel = castle.GetLevelMageGuild();
            if ( constructionInProgress ) {
                return mageGuildLevel == 0 || buildId > ( BUILD_MAGEGUILD1 << ( mageGuildLevel - 1 ) );
            }
            else {
                return buildId == ( BUILD_MAGEGUILD1 << ( mageGuildLevel - 1 ) );
            }
        }
        else if ( buildId == BUILD_THIEVESGUILD ) {
            return true;
        }
    }

    return false;
}

void CastleDialog::RedrawRoadConnection( const Castle & castle, const Point & position, const uint32_t buildId, const uint8_t alpha )
{
    const Rect & roi = CastleGetMaxArea( castle, position );
    const bool constructionInProgress = alpha < 255;

    if ( Race::BARB == castle.GetRace() ) {
        if ( buildId & BUILD_MAGEGUILD || buildId == BUILD_SPEC ) {
            if ( buildId & BUILD_MAGEGUILD ) {
                if ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( BUILD_SPEC ) ) )
                    return;
            }
            else if ( buildId == BUILD_SPEC ) {
                if ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( BUILD_MAGEGUILD1 ) ) )
                    return;
            }

            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::TWNBEXT2, 0 );
            CastleDialog::RedrawBuildingSpriteToArea( sprite, position.x + sprite.x(), position.y + sprite.y(), roi, alpha );
        }

        if ( buildId == DWELLING_MONSTER3 || buildId == BUILD_THIEVESGUILD ) {
            if ( buildId == DWELLING_MONSTER3 ) {
                if ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( BUILD_THIEVESGUILD ) ) )
                    return;
            }
            else if ( buildId == BUILD_THIEVESGUILD ) {
                if ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( DWELLING_MONSTER3 ) ) )
                    return;
            }

            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::TWNBEXT3, 0 );
            CastleDialog::RedrawBuildingSpriteToArea( sprite, position.x + sprite.x(), position.y + sprite.y(), roi, alpha );
        }
    }
}

fheroes2::Rect CastleGetCoordBuilding( int race, building_t building, const Point & pt )
{
    switch ( building ) {
    case BUILD_THIEVESGUILD:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 0, pt.y + 130, 50, 60 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 478, pt.y + 100, 76, 42 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 423, pt.y + 165, 65, 49 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 525, pt.y + 109, 60, 48 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 507, pt.y + 55, 47, 42 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 291, pt.y + 134, 43, 59 );
        default:
            break;
        }
        break;

    case BUILD_TAVERN:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 350, pt.y + 110, 46, 56 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 0, pt.y + 205, 125, 60 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 494, pt.y + 140, 131, 87 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 479, pt.y + 100, 39, 52 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x, pt.y + 160, 118, 50 );
        default:
            break;
        }
        break;

    case BUILD_SHRINE:
        switch ( race ) {
        case Race::NECR:
            return fheroes2::Rect( pt.x + 453, pt.y + 36, 55, 96 );
        default:
            break;
        }
        break;

    case BUILD_SHIPYARD:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 537, pt.y + 221, 103, 33 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 535, pt.y + 210, 105, 45 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 0, pt.y + 220, 134, 35 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 520, pt.y + 206, 120, 47 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x, pt.y + 218, 185, 35 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 516, pt.y + 221, 124, 28 );
        default:
            break;
        }
        break;

    case BUILD_WELL:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 194, pt.y + 225, 29, 27 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 272, pt.y + 215, 44, 32 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 346, pt.y + 209, 43, 25 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 348, pt.y + 221, 63, 30 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 254, pt.y + 143, 19, 28 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 217, pt.y + 225, 23, 26 );
        default:
            break;
        }
        break;

    case BUILD_STATUE:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 480, pt.y + 205, 45, 50 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 470, pt.y + 180, 30, 58 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 158, pt.y + 173, 17, 58 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 473, pt.y + 172, 45, 51 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 464, pt.y + 58, 26, 62 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 374, pt.y + 174, 26, 70 );
        default:
            break;
        }
        break;

    case BUILD_MARKETPLACE:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 220, pt.y + 144, 115, 20 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 224, pt.y + 168, 52, 36 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 412, pt.y + 122, 56, 40 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 391, pt.y + 185, 70, 26 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 254, pt.y + 176, 105, 39 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 415, pt.y + 216, 85, 35 );
        default:
            break;
        }
        break;

    case BUILD_WEL2:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 288, pt.y + 97, 63, 18 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 252, pt.y + 120, 44, 16 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 135, pt.y + 200, 63, 31 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 69, pt.y + 46, 24, 124 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 234, pt.y + 225, 107, 22 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 275, pt.y + 206, 68, 39 );
        default:
            break;
        }
        break;

    case BUILD_MOAT:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 53, pt.y + 150, 93, 30 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 113, pt.y + 155, 106, 30 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 143, pt.y + 169, 98, 11 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 327, pt.y + 166, 66, 17 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x, pt.y + 91, 198, 11 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 336, pt.y + 169, 71, 15 );
        default:
            break;
        }
        break;

    case BUILD_SPEC:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 0, pt.y + 80, 250, 20 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 223, pt.y + 79, 124, 27 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 147, pt.y + 0, 252, 30 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x, pt.y + 162, 70, 77 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 304, pt.y + 111, 95, 50 );
        case Race::NECR:
            return fheroes2::Rect( pt.x, pt.y, 640, 54 );
        default:
            break;
        }
        break;

    case BUILD_CASTLE:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 0, pt.y + 55, 290, 85 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 0, pt.y + 88, 202, 62 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 0, pt.y + 67, 198, 100 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 268, pt.y + 35, 131, 129 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x, pt.y + 48, 187, 39 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 322, pt.y + 63, 73, 97 );
        default:
            break;
        }
        break;

    case BUILD_CAPTAIN:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 293, pt.y + 109, 48, 27 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 210, pt.y + 104, 40, 35 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 238, pt.y + 136, 32, 34 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 420, pt.y + 102, 52, 60 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 210, pt.y + 55, 28, 32 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 423, pt.y + 126, 41, 46 );
        default:
            break;
        }
        break;

    case BUILD_MAGEGUILD1:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 398, pt.y + 150, 58, 30 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 348, pt.y + 118, 50, 25 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 285, pt.y + 32, 55, 129 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 590, pt.y + 135, 50, 35 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 583, pt.y + 73, 57, 48 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 565, pt.y + 131, 73, 74 );
        default:
            break;
        }
        break;

    case BUILD_MAGEGUILD2:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 398, pt.y + 128, 58, 52 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 348, pt.y + 94, 50, 49 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 285, pt.y + 32, 55, 129 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 590, pt.y + 108, 50, 60 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 585, pt.y + 69, 55, 50 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 568, pt.y + 102, 62, 104 );
        default:
            break;
        }
        break;

    case BUILD_MAGEGUILD3:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 398, pt.y + 105, 58, 75 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 348, pt.y + 72, 50, 72 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 285, pt.y + 32, 55, 129 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 590, pt.y + 77, 50, 90 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 585, pt.y + 44, 55, 78 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 570, pt.y + 79, 56, 130 );
        default:
            break;
        }
        break;

    case BUILD_MAGEGUILD4:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 398, pt.y + 85, 58, 95 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 348, pt.y + 48, 50, 96 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 285, pt.y + 32, 55, 129 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 590, pt.y + 45, 50, 125 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 585, pt.y + 20, 54, 102 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 570, pt.y + 61, 60, 146 );
        default:
            break;
        }
        break;

    case BUILD_MAGEGUILD5:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 398, pt.y + 55, 58, 125 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 348, pt.y + 20, 50, 124 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 285, pt.y + 32, 55, 129 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 590, pt.y + 14, 50, 155 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 585, pt.y, 57, 122 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 570, pt.y + 45, 61, 162 );
        default:
            break;
        }
        break;

    case BUILD_TENT:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 82, pt.y + 132, 42, 30 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 53, pt.y + 119, 67, 35 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 88, pt.y + 145, 62, 36 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 308, pt.y + 140, 52, 28 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 60, pt.y + 68, 46, 33 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 333, pt.y + 131, 49, 51 );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER1:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 195, pt.y + 175, 50, 40 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 258, pt.y + 142, 71, 41 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 478, pt.y + 70, 92, 62 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x, pt.y + 63, 68, 53 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 459, pt.y + 181, 45, 32 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 404, pt.y + 181, 56, 25 );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER2:
    case DWELLING_UPGRADE2:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 145, pt.y + 155, 58, 20 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 152, pt.y + 190, 68, 50 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 345, pt.y + 149, 70, 56 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 248, pt.y + 192, 60, 55 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 253, pt.y + 69, 90, 29 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 147, pt.y + 184, 92, 32 );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER3:
    case DWELLING_UPGRADE3:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 250, pt.y + 177, 70, 50 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 582, pt.y + 81, 58, 40 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 90, pt.y + 180, 56, 21 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 504, pt.y + 53, 38, 30 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 156, pt.y + 139, 74, 51 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 108, pt.y + 69, 117, 91 );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER4:
    case DWELLING_UPGRADE4:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 328, pt.y + 195, 100, 50 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 509, pt.y + 148, 123, 57 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 208, pt.y + 182, 127, 55 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 154, pt.y + 168, 171, 76 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 593, pt.y + 187, 47, 28 );
        case Race::NECR:
            return fheroes2::Rect( pt.x, pt.y + 154, 140, 74 );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER5:
    case DWELLING_UPGRADE5:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 0, pt.y + 200, 150, 55 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 331, pt.y + 186, 121, 44 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 345, pt.y + 231, 165, 21 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 149, pt.y + 98, 95, 58 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 417, pt.y + 25, 35, 140 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 235, pt.y + 136, 53, 70 );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER6:
    case DWELLING_UPGRADE6:
    case DWELLING_UPGRADE7:
        switch ( race ) {
        case Race::KNGT:
            return fheroes2::Rect( pt.x + 465, pt.y + 85, 175, 110 );
        case Race::BARB:
            return fheroes2::Rect( pt.x + 407, pt.y + 13, 109, 80 );
        case Race::SORC:
            return fheroes2::Rect( pt.x + 202, pt.y + 38, 42, 65 );
        case Race::WRLK:
            return fheroes2::Rect( pt.x + 98, pt.y + 25, 55, 229 );
        case Race::WZRD:
            return fheroes2::Rect( pt.x + 196, pt.y + 7, 129, 38 );
        case Race::NECR:
            return fheroes2::Rect( pt.x + 468, pt.y + 112, 92, 78 );
        default:
            break;
        }
        break;

    default:
        break;
    }

    return fheroes2::Rect();
}

void CastlePackOrdersBuildings( const Castle & castle, std::vector<building_t> & ordersBuildings )
{
    ordersBuildings.reserve( 30 );

    switch ( castle.GetRace() ) {
    case Race::KNGT:
        ordersBuildings.push_back( BUILD_TENT );
        ordersBuildings.push_back( BUILD_CASTLE );
        ordersBuildings.push_back( BUILD_SPEC );
        ordersBuildings.push_back( BUILD_WEL2 );
        ordersBuildings.push_back( BUILD_CAPTAIN );
        ordersBuildings.push_back( BUILD_LEFTTURRET );
        ordersBuildings.push_back( BUILD_RIGHTTURRET );
        ordersBuildings.push_back( BUILD_MOAT );
        ordersBuildings.push_back( BUILD_MARKETPLACE );
        ordersBuildings.push_back( DWELLING_MONSTER2 );
        ordersBuildings.push_back( DWELLING_UPGRADE2 );
        ordersBuildings.push_back( BUILD_THIEVESGUILD );
        ordersBuildings.push_back( BUILD_TAVERN );
        ordersBuildings.push_back( BUILD_MAGEGUILD1 );
        ordersBuildings.push_back( BUILD_MAGEGUILD2 );
        ordersBuildings.push_back( BUILD_MAGEGUILD3 );
        ordersBuildings.push_back( BUILD_MAGEGUILD4 );
        ordersBuildings.push_back( BUILD_MAGEGUILD5 );
        ordersBuildings.push_back( DWELLING_MONSTER5 );
        ordersBuildings.push_back( DWELLING_UPGRADE5 );
        ordersBuildings.push_back( DWELLING_MONSTER6 );
        ordersBuildings.push_back( DWELLING_UPGRADE6 );
        ordersBuildings.push_back( DWELLING_MONSTER1 );
        ordersBuildings.push_back( DWELLING_MONSTER3 );
        ordersBuildings.push_back( DWELLING_UPGRADE3 );
        ordersBuildings.push_back( DWELLING_MONSTER4 );
        ordersBuildings.push_back( DWELLING_UPGRADE4 );
        ordersBuildings.push_back( BUILD_WELL );
        ordersBuildings.push_back( BUILD_SHIPYARD );
        ordersBuildings.push_back( BUILD_STATUE );
        break;
    case Race::BARB:
        ordersBuildings.push_back( BUILD_SPEC );
        ordersBuildings.push_back( BUILD_WEL2 );
        ordersBuildings.push_back( DWELLING_MONSTER6 );
        ordersBuildings.push_back( BUILD_MAGEGUILD1 );
        ordersBuildings.push_back( BUILD_MAGEGUILD2 );
        ordersBuildings.push_back( BUILD_MAGEGUILD3 );
        ordersBuildings.push_back( BUILD_MAGEGUILD4 );
        ordersBuildings.push_back( BUILD_MAGEGUILD5 );
        ordersBuildings.push_back( BUILD_CAPTAIN );
        ordersBuildings.push_back( BUILD_TENT );
        ordersBuildings.push_back( BUILD_CASTLE );
        ordersBuildings.push_back( BUILD_LEFTTURRET );
        ordersBuildings.push_back( BUILD_RIGHTTURRET );
        ordersBuildings.push_back( BUILD_MOAT );
        ordersBuildings.push_back( DWELLING_MONSTER3 );
        ordersBuildings.push_back( BUILD_THIEVESGUILD );
        ordersBuildings.push_back( DWELLING_MONSTER1 );
        ordersBuildings.push_back( BUILD_MARKETPLACE );
        ordersBuildings.push_back( DWELLING_MONSTER2 );
        ordersBuildings.push_back( DWELLING_UPGRADE2 );
        ordersBuildings.push_back( BUILD_TAVERN );
        ordersBuildings.push_back( DWELLING_MONSTER4 );
        ordersBuildings.push_back( DWELLING_UPGRADE4 );
        ordersBuildings.push_back( DWELLING_MONSTER5 );
        ordersBuildings.push_back( DWELLING_UPGRADE5 );
        ordersBuildings.push_back( BUILD_WELL );
        ordersBuildings.push_back( BUILD_STATUE );
        ordersBuildings.push_back( BUILD_SHIPYARD );
        break;
    case Race::SORC:
        ordersBuildings.push_back( BUILD_SPEC );
        ordersBuildings.push_back( DWELLING_MONSTER6 );
        ordersBuildings.push_back( BUILD_MAGEGUILD1 );
        ordersBuildings.push_back( BUILD_MAGEGUILD2 );
        ordersBuildings.push_back( BUILD_MAGEGUILD3 );
        ordersBuildings.push_back( BUILD_MAGEGUILD4 );
        ordersBuildings.push_back( BUILD_MAGEGUILD5 );
        ordersBuildings.push_back( BUILD_CAPTAIN );
        ordersBuildings.push_back( BUILD_TENT );
        ordersBuildings.push_back( BUILD_CASTLE );
        ordersBuildings.push_back( BUILD_LEFTTURRET );
        ordersBuildings.push_back( BUILD_RIGHTTURRET );
        ordersBuildings.push_back( BUILD_MOAT );
        ordersBuildings.push_back( DWELLING_MONSTER3 );
        ordersBuildings.push_back( DWELLING_UPGRADE3 );
        ordersBuildings.push_back( BUILD_SHIPYARD );
        ordersBuildings.push_back( BUILD_MARKETPLACE );
        ordersBuildings.push_back( DWELLING_MONSTER2 );
        ordersBuildings.push_back( DWELLING_UPGRADE2 );
        ordersBuildings.push_back( BUILD_THIEVESGUILD );
        ordersBuildings.push_back( DWELLING_MONSTER1 );
        ordersBuildings.push_back( BUILD_TAVERN );
        ordersBuildings.push_back( BUILD_STATUE );
        ordersBuildings.push_back( BUILD_WEL2 );
        ordersBuildings.push_back( DWELLING_MONSTER4 );
        ordersBuildings.push_back( DWELLING_UPGRADE4 );
        ordersBuildings.push_back( BUILD_WELL );
        ordersBuildings.push_back( DWELLING_MONSTER5 );
        break;
    case Race::WRLK:
        ordersBuildings.push_back( DWELLING_MONSTER5 );
        ordersBuildings.push_back( DWELLING_MONSTER3 );
        ordersBuildings.push_back( BUILD_TENT );
        ordersBuildings.push_back( BUILD_CASTLE );
        ordersBuildings.push_back( BUILD_LEFTTURRET );
        ordersBuildings.push_back( BUILD_RIGHTTURRET );
        ordersBuildings.push_back( BUILD_CAPTAIN );
        ordersBuildings.push_back( BUILD_MOAT );
        ordersBuildings.push_back( BUILD_SHIPYARD );
        ordersBuildings.push_back( BUILD_MAGEGUILD1 );
        ordersBuildings.push_back( BUILD_MAGEGUILD2 );
        ordersBuildings.push_back( BUILD_MAGEGUILD3 );
        ordersBuildings.push_back( BUILD_MAGEGUILD4 );
        ordersBuildings.push_back( BUILD_MAGEGUILD5 );
        ordersBuildings.push_back( BUILD_TAVERN );
        ordersBuildings.push_back( BUILD_THIEVESGUILD );
        ordersBuildings.push_back( BUILD_MARKETPLACE );
        ordersBuildings.push_back( BUILD_STATUE );
        ordersBuildings.push_back( DWELLING_MONSTER1 );
        ordersBuildings.push_back( BUILD_WEL2 );
        ordersBuildings.push_back( BUILD_SPEC );
        ordersBuildings.push_back( DWELLING_MONSTER4 );
        ordersBuildings.push_back( DWELLING_UPGRADE4 );
        ordersBuildings.push_back( DWELLING_MONSTER2 );
        ordersBuildings.push_back( DWELLING_MONSTER6 );
        ordersBuildings.push_back( DWELLING_UPGRADE6 );
        ordersBuildings.push_back( DWELLING_UPGRADE7 );
        ordersBuildings.push_back( BUILD_WELL );
        break;
    case Race::WZRD:
        ordersBuildings.push_back( DWELLING_MONSTER6 );
        ordersBuildings.push_back( DWELLING_UPGRADE6 );
        ordersBuildings.push_back( BUILD_TENT );
        ordersBuildings.push_back( BUILD_CASTLE );
        ordersBuildings.push_back( BUILD_LEFTTURRET );
        ordersBuildings.push_back( BUILD_RIGHTTURRET );
        ordersBuildings.push_back( BUILD_MOAT );
        ordersBuildings.push_back( BUILD_CAPTAIN );
        ordersBuildings.push_back( DWELLING_MONSTER2 );
        ordersBuildings.push_back( BUILD_THIEVESGUILD );
        ordersBuildings.push_back( BUILD_TAVERN );
        ordersBuildings.push_back( BUILD_SHIPYARD );
        ordersBuildings.push_back( BUILD_WELL );
        ordersBuildings.push_back( DWELLING_MONSTER3 );
        ordersBuildings.push_back( DWELLING_UPGRADE3 );
        ordersBuildings.push_back( DWELLING_MONSTER5 );
        ordersBuildings.push_back( DWELLING_UPGRADE5 );
        ordersBuildings.push_back( BUILD_MAGEGUILD1 );
        ordersBuildings.push_back( BUILD_MAGEGUILD2 );
        ordersBuildings.push_back( BUILD_MAGEGUILD3 );
        ordersBuildings.push_back( BUILD_MAGEGUILD4 );
        ordersBuildings.push_back( BUILD_MAGEGUILD5 );
        ordersBuildings.push_back( BUILD_SPEC );
        ordersBuildings.push_back( BUILD_STATUE );
        ordersBuildings.push_back( DWELLING_MONSTER1 );
        ordersBuildings.push_back( DWELLING_MONSTER4 );
        ordersBuildings.push_back( BUILD_MARKETPLACE );
        ordersBuildings.push_back( BUILD_WEL2 );
        break;
    case Race::NECR:
        ordersBuildings.push_back( BUILD_SPEC );
        if ( Settings::Get().PriceLoyaltyVersion() )
            ordersBuildings.push_back( BUILD_SHRINE );
        ordersBuildings.push_back( BUILD_TENT );
        ordersBuildings.push_back( BUILD_CASTLE );
        ordersBuildings.push_back( BUILD_LEFTTURRET );
        ordersBuildings.push_back( BUILD_RIGHTTURRET );
        ordersBuildings.push_back( BUILD_MOAT );
        ordersBuildings.push_back( BUILD_CAPTAIN );
        ordersBuildings.push_back( DWELLING_MONSTER6 );
        ordersBuildings.push_back( DWELLING_MONSTER1 );
        ordersBuildings.push_back( BUILD_THIEVESGUILD );
        ordersBuildings.push_back( DWELLING_MONSTER3 );
        ordersBuildings.push_back( DWELLING_UPGRADE3 );
        ordersBuildings.push_back( DWELLING_MONSTER5 );
        ordersBuildings.push_back( DWELLING_UPGRADE5 );
        ordersBuildings.push_back( DWELLING_MONSTER2 );
        ordersBuildings.push_back( DWELLING_UPGRADE2 );
        ordersBuildings.push_back( DWELLING_MONSTER4 );
        ordersBuildings.push_back( DWELLING_UPGRADE4 );
        ordersBuildings.push_back( BUILD_MAGEGUILD1 );
        ordersBuildings.push_back( BUILD_MAGEGUILD2 );
        ordersBuildings.push_back( BUILD_MAGEGUILD3 );
        ordersBuildings.push_back( BUILD_MAGEGUILD4 );
        ordersBuildings.push_back( BUILD_MAGEGUILD5 );
        ordersBuildings.push_back( BUILD_SHIPYARD );
        ordersBuildings.push_back( BUILD_WEL2 );
        ordersBuildings.push_back( BUILD_MARKETPLACE );
        ordersBuildings.push_back( BUILD_STATUE );
        ordersBuildings.push_back( BUILD_WELL );
        break;
    default:
        break;
    }

    ordersBuildings.push_back( BUILD_NOTHING );
}

Rect CastleGetMaxArea( const Castle & castle, const Point & top )
{
    Rect res( top, 0, 0 );

    int townIcnId = -1;

    switch ( castle.GetRace() ) {
    case Race::KNGT:
        townIcnId = ICN::TOWNBKG0;
        break;
    case Race::BARB:
        townIcnId = ICN::TOWNBKG1;
        break;
    case Race::SORC:
        townIcnId = ICN::TOWNBKG2;
        break;
    case Race::WRLK:
        townIcnId = ICN::TOWNBKG3;
        break;
    case Race::WZRD:
        townIcnId = ICN::TOWNBKG4;
        break;
    case Race::NECR:
        townIcnId = ICN::TOWNBKG5;
        break;
    default:
        return res;
    }

    const fheroes2::Sprite & townbkg = fheroes2::AGG::GetICN( townIcnId, 0 );
    res.w = townbkg.width();
    res.h = townbkg.height();

    return res;
}
