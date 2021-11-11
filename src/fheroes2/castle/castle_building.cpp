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
#include <cassert>

#include "agg_image.h"
#include "castle.h"
#include "castle_building_info.h"
#include "castle_ui.h"
#include "game.h"
#include "game_delays.h"
#include "icn.h"
#include "race.h"
#include "settings.h"
#include "text.h"
#include "tools.h"

void CastleRedrawCurrentBuilding( const Castle & castle, const fheroes2::Point & dst_pt, const CastleDialog::CacheBuildings & orders,
                                  const CastleDialog::FadeBuilding & alphaBuilding, const uint32_t animationIndex );
fheroes2::Rect CastleGetMaxArea( const Castle &, const fheroes2::Point & );

namespace
{
    bool isBuildingConnectionNeeded( const Castle & castle, const uint32_t buildId, const bool constructionInProgress )
    {
        const int race = castle.GetRace();

        if ( race == Race::BARB ) {
            if ( buildId & BUILD_MAGEGUILD ) {
                const int mageGuildLevel = castle.GetLevelMageGuild();
                if ( constructionInProgress ) {
                    return mageGuildLevel == 0 || buildId > ( BUILD_MAGEGUILD1 << ( mageGuildLevel - 1 ) );
                }

                assert( mageGuildLevel > 0 );
                return buildId == ( BUILD_MAGEGUILD1 << ( mageGuildLevel - 1 ) );
            }
            else if ( buildId == BUILD_THIEVESGUILD ) {
                return true;
            }
        }
        else if ( race == Race::NECR ) {
            if ( buildId == BUILD_CAPTAIN ) {
                return true;
            }
        }

        return false;
    }

    void redrawBuildingConnection( const Castle & castle, const fheroes2::Point & position, const uint32_t buildId, const uint8_t alpha = 255 )
    {
        const fheroes2::Rect & roi = CastleGetMaxArea( castle, position );
        const bool constructionInProgress = alpha < 255;

        const int race = castle.GetRace();

        if ( race == Race::BARB ) {
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
        else if ( race == Race::NECR ) {
            if ( buildId == BUILD_CAPTAIN ) {
                if ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( BUILD_CASTLE ) ) )
                    return;
            }
            else if ( buildId == BUILD_CASTLE ) {
                if ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( BUILD_CAPTAIN ) ) )
                    return;
            }

            const fheroes2::Sprite & bridge = fheroes2::AGG::GetICN( ICN::NECROMANCER_CASTLE_CAPTAIN_QUARTERS_BRIDGE, 0 );
            CastleDialog::RedrawBuildingSpriteToArea( bridge, position.x + bridge.x(), position.y + bridge.y(), roi, alpha );
        }
    }
}

void CastleDialog::RedrawBuildingSpriteToArea( const fheroes2::Sprite & sprite, s32 dst_x, s32 dst_y, const fheroes2::Rect & max, uint8_t alpha )
{
    std::pair<fheroes2::Rect, fheroes2::Point> res = Fixed4Blit( fheroes2::Rect( dst_x, dst_y, sprite.width(), sprite.height() ), max );
    fheroes2::AlphaBlit( sprite, res.first.x, res.first.y, fheroes2::Display::instance(), res.second.x, res.second.y, res.first.width, res.first.height, alpha );
}

CastleDialog::CacheBuildings::CacheBuildings( const Castle & castle, const fheroes2::Point & top )
{
    const std::vector<building_t> ordersBuildings = fheroes2::getBuildingDrawingPriorities( castle.GetRace(), Settings::Get().CurrentFileInfo()._version );

    for ( const building_t buildingId : ordersBuildings ) {
        emplace_back( buildingId, fheroes2::getCastleBuildingArea( castle.GetRace(), buildingId ) + top );
    }
}

void CastleDialog::FadeBuilding::StartFadeBuilding( const uint32_t build )
{
    _alpha = 0;
    _build = build;
}

bool CastleDialog::FadeBuilding::UpdateFadeBuilding()
{
    if ( _alpha < 255 && Game::validateAnimationDelay( Game::CASTLE_BUILD_DELAY ) ) {
        if ( _alpha < 255 - 15 ) {
            _alpha += 15;
        }
        else {
            _alpha = 255;
        }
        return true;
    }
    return false;
}

void CastleDialog::FadeBuilding::StopFadeBuilding()
{
    if ( _build != BUILD_NOTHING ) {
        _build = BUILD_NOTHING;
        _alpha = 255;
    }
}

void CastleDialog::RedrawAllBuilding( const Castle & castle, const fheroes2::Point & dst_pt, const CacheBuildings & orders,
                                      const CastleDialog::FadeBuilding & alphaBuilding, const uint32_t animationIndex )
{
    CastleRedrawCurrentBuilding( castle, dst_pt, orders, alphaBuilding, animationIndex );
    fheroes2::drawCastleName( castle, fheroes2::Display::instance(), dst_pt );
}

void CastleRedrawCurrentBuilding( const Castle & castle, const fheroes2::Point & dst_pt, const CastleDialog::CacheBuildings & orders,
                                  const CastleDialog::FadeBuilding & fadeBuilding, const uint32_t animationIndex )
{
    fheroes2::Display & display = fheroes2::Display::instance();

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

    const fheroes2::Rect max = CastleGetMaxArea( castle, dst_pt );

    if ( townIcnId != -1 ) {
        // We shouldn't Blit this image. This is a background image so a normal Copy is enough.
        const fheroes2::Sprite & townbkg = fheroes2::AGG::GetICN( townIcnId, 0 );
        fheroes2::Copy( townbkg, 0, 0, display, dst_pt.x, dst_pt.y, townbkg.width(), townbkg.height() );
    }

    if ( Race::BARB == castle.GetRace() ) {
        const fheroes2::Sprite & sprite0 = fheroes2::AGG::GetICN( ICN::TWNBEXT1, 1 + animationIndex % 5 );
        fheroes2::Blit( sprite0, display, dst_pt.x + sprite0.x(), dst_pt.y + sprite0.y() );
    }

    // Bay animation
    if ( Race::WZRD == castle.GetRace() || ( !castle.isBuild( BUILD_SHIPYARD ) && castle.HaveNearlySea() ) ) {
        fheroes2::Sprite sprite50;
        fheroes2::Sprite sprite51;

        switch ( castle.GetRace() ) {
        case Race::KNGT:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNKEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNKEXT0, 1 + animationIndex % 5 );
            break;
        case Race::BARB:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNBEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNBEXT0, 1 + animationIndex % 5 );
            break;
        case Race::SORC:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNSEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNSEXT0, 1 + animationIndex % 5 );
            break;
        case Race::NECR:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNNEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNNEXT0, 1 + animationIndex % 5 );
            break;
        case Race::WRLK:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNWEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNWEXT0, 1 + animationIndex % 5 );
            break;
        case Race::WZRD:
            sprite50 = fheroes2::AGG::GetICN( ICN::TWNZEXT0, 0 );
            sprite51 = fheroes2::AGG::GetICN( ICN::TWNZEXT0, 1 + animationIndex % 5 );
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
    if ( BUILD_NOTHING == fadeBuilding.GetBuild() ) {
        for ( auto it = orders.cbegin(); it != orders.cend(); ++it ) {
            const uint32_t currentBuildId = it->id;
            if ( castle.isBuild( currentBuildId ) ) {
                CastleDialog::CastleRedrawBuilding( castle, dst_pt, currentBuildId, animationIndex );
                CastleDialog::CastleRedrawBuildingExtended( castle, dst_pt, currentBuildId, animationIndex );
                if ( isBuildingConnectionNeeded( castle, currentBuildId, false ) ) {
                    redrawBuildingConnection( castle, dst_pt, currentBuildId );
                }
            }
        }
    }
    // redraw build with alpha
    else if ( orders.cend() != std::find( orders.cbegin(), orders.cend(), fadeBuilding.GetBuild() ) ) {
        for ( auto it = orders.cbegin(); it != orders.cend(); ++it ) {
            const uint32_t currentBuildId = it->id;

            if ( castle.isBuild( currentBuildId ) ) {
                CastleDialog::CastleRedrawBuilding( castle, dst_pt, currentBuildId, animationIndex );
                if ( currentBuildId == BUILD_SHIPYARD && fadeBuilding.GetBuild() == BUILD_SHIPYARD ) {
                    CastleDialog::CastleRedrawBuildingExtended( castle, dst_pt, currentBuildId, animationIndex, fadeBuilding.GetAlpha() );
                }
                else {
                    CastleDialog::CastleRedrawBuildingExtended( castle, dst_pt, currentBuildId, animationIndex );
                }
                if ( isBuildingConnectionNeeded( castle, currentBuildId, false ) ) {
                    redrawBuildingConnection( castle, dst_pt, fadeBuilding.GetBuild(), fadeBuilding.GetAlpha() );
                    redrawBuildingConnection( castle, dst_pt, currentBuildId );
                }
            }
            else if ( currentBuildId == fadeBuilding.GetBuild() ) {
                CastleDialog::CastleRedrawBuilding( castle, dst_pt, currentBuildId, animationIndex, fadeBuilding.GetAlpha() );
                CastleDialog::CastleRedrawBuildingExtended( castle, dst_pt, currentBuildId, animationIndex, fadeBuilding.GetAlpha() );
                if ( isBuildingConnectionNeeded( castle, currentBuildId, true ) ) {
                    redrawBuildingConnection( castle, dst_pt, currentBuildId, fadeBuilding.GetAlpha() );
                }
            }
        }
    }
}

void CastleDialog::CastleRedrawBuilding( const Castle & castle, const fheroes2::Point & dst_pt, u32 build, u32 frame, uint8_t alpha )
{
    if ( build == BUILD_TENT ) // we don't need to draw a tent as it's on the background image
        return;

    const fheroes2::Rect max = CastleGetMaxArea( castle, dst_pt );

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

        // Special case: Knight castle's flags are overlapped by Right Turret so we need to draw flags after drawing the Turret.
        const bool knightCastleCase = ( race == Race::KNGT && castle.isBuild( BUILD_RIGHTTURRET ) && castle.isBuild( BUILD_CASTLE ) );
        if ( knightCastleCase && build == BUILD_CASTLE ) {
            // Do not draw flags.
            return;
        }

        // second anime sprite
        if ( const u32 index2 = ICN::AnimationFrame( icn, index, frame ) ) {
            const fheroes2::Sprite & sprite2 = fheroes2::AGG::GetICN( icn, index2 );

            CastleDialog::RedrawBuildingSpriteToArea( sprite2, dst_pt.x + sprite2.x(), dst_pt.y + sprite2.y(), max, alpha );
        }

        if ( knightCastleCase && build == BUILD_RIGHTTURRET ) {
            // Draw Castle's flags after the Turret.
            const int castleIcn = Castle::GetICNBuilding( BUILD_CASTLE, race );
            const uint32_t flagAnimFrame = ICN::AnimationFrame( castleIcn, index, frame );
            if ( flagAnimFrame > 0 ) {
                const fheroes2::Sprite & castleFlagSprite = fheroes2::AGG::GetICN( castleIcn, flagAnimFrame );

                CastleDialog::RedrawBuildingSpriteToArea( castleFlagSprite, dst_pt.x + castleFlagSprite.x(), dst_pt.y + castleFlagSprite.y(), max, alpha );
            }
        }
    }
}

void CastleDialog::CastleRedrawBuildingExtended( const Castle & castle, const fheroes2::Point & dst_pt, u32 build, u32 frame, uint8_t alpha )
{
    if ( build == BUILD_TENT ) // we don't need to draw a tent as it's on the background image
        return;

    const fheroes2::Rect max = CastleGetMaxArea( castle, dst_pt );
    int icn = Castle::GetICNBuilding( build, castle.GetRace() );

    // shipyard
    if ( BUILD_SHIPYARD == build ) {
        // boat
        if ( castle.PresentBoat() ) {
            const int icn2 = Castle::GetICNBoat( castle.GetRace() );

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
    else if ( castle.GetRace() == Race::KNGT && BUILD_WEL2 == build && !castle.isBuild( BUILD_CASTLE ) ) {
        const fheroes2::Sprite & rightFarm = fheroes2::AGG::GetICN( ICN::KNIGHT_CASTLE_RIGHT_FARM, 0 );
        const fheroes2::Sprite & leftFarm = fheroes2::AGG::GetICN( ICN::KNIGHT_CASTLE_LEFT_FARM, 0 );
        CastleDialog::RedrawBuildingSpriteToArea( leftFarm, dst_pt.x + rightFarm.x() - leftFarm.width(), dst_pt.y + rightFarm.y(), max, alpha );
    }
    else if ( castle.GetRace() == Race::BARB && BUILD_CAPTAIN == build && !castle.isBuild( BUILD_CASTLE ) ) {
        const fheroes2::Sprite & rightCaptainQuarters = fheroes2::AGG::GetICN( ICN::TWNBCAPT, 0 );
        const fheroes2::Sprite & leftCaptainQuarters = fheroes2::AGG::GetICN( ICN::BARBARIAN_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE, 0 );
        CastleDialog::RedrawBuildingSpriteToArea( leftCaptainQuarters, dst_pt.x + rightCaptainQuarters.x() - leftCaptainQuarters.width() + leftCaptainQuarters.x(),
                                                  dst_pt.y + rightCaptainQuarters.y() + leftCaptainQuarters.y(), max, alpha );
    }
}

fheroes2::Rect CastleGetMaxArea( const Castle & castle, const fheroes2::Point & top )
{
    fheroes2::Rect res( top.x, top.y, 0, 0 );

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
    res.width = townbkg.width();
    res.height = townbkg.height();

    return res;
}
