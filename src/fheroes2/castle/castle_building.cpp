/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cstdint>
#include <vector>

#include "agg_image.h"
#include "castle.h" // IWYU pragma: associated
#include "castle_building_info.h"
#include "game_delays.h"
#include "icn.h"
#include "image.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "race.h"
#include "screen.h"
#include "settings.h"
#include "ui_castle.h"

namespace
{
    int getTownIcnId( const int race )
    {
        switch ( race ) {
        case Race::KNGT:
            return ICN::TOWNBKG0;
        case Race::BARB:
            return ICN::TOWNBKG1;
        case Race::SORC:
            return ICN::TOWNBKG2;
        case Race::WRLK:
            return ICN::TOWNBKG3;
        case Race::WZRD:
            return ICN::TOWNBKG4;
        case Race::NECR:
            return ICN::TOWNBKG5;
        default:
            // Have you added a new race? Add the logic for it!
            assert( 0 );
            break;
        }
        return ICN::UNKNOWN;
    }

    fheroes2::Rect CastleGetMaxArea( const Castle & castle, const fheroes2::Point & top )
    {
        const fheroes2::Sprite & townbkg = fheroes2::AGG::GetICN( getTownIcnId( castle.GetRace() ), 0 );

        return { top.x, top.y, townbkg.width(), townbkg.height() };
    }

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

            if ( buildId == BUILD_THIEVESGUILD ) {
                return true;
            }
        }
        else if ( race == Race::NECR && buildId == BUILD_CAPTAIN ) {
            return true;
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
                if ( buildId & BUILD_MAGEGUILD && ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( BUILD_SPEC ) ) ) ) {
                    return;
                }

                if ( buildId == BUILD_SPEC && ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( BUILD_MAGEGUILD1 ) ) ) ) {
                    return;
                }

                fheroes2::drawCastleDialogBuilding( ICN::TWNBEXT2, 0, castle, position, roi, alpha );
            }

            if ( buildId == DWELLING_MONSTER3 || buildId == BUILD_THIEVESGUILD ) {
                if ( buildId == DWELLING_MONSTER3 && ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( BUILD_THIEVESGUILD ) ) ) ) {
                    return;
                }

                if ( buildId == BUILD_THIEVESGUILD && ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( DWELLING_MONSTER3 ) ) ) ) {
                    return;
                }

                fheroes2::drawCastleDialogBuilding( ICN::TWNBEXT3, 0, castle, position, roi, alpha );
            }
        }
        else if ( race == Race::NECR ) {
            if ( buildId == BUILD_CAPTAIN && ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( BUILD_CASTLE ) ) ) ) {
                return;
            }

            if ( buildId == BUILD_CASTLE && ( ( !constructionInProgress && !castle.isBuild( buildId ) ) || ( !castle.isBuild( BUILD_CAPTAIN ) ) ) ) {
                return;
            }

            fheroes2::drawCastleDialogBuilding( ICN::NECROMANCER_CASTLE_CAPTAIN_QUARTERS_BRIDGE, 0, castle, position, roi, alpha );
        }
    }

    void redrawCastleBuilding( const Castle & castle, const fheroes2::Point & dst_pt, const uint32_t building, const uint32_t frame, const uint8_t alpha = 255 )
    {
        if ( building == BUILD_TENT ) {
            // We don't need to draw a tent because it's on the background image.
            return;
        }

        const int race = castle.GetRace();
        uint32_t index = 0;

        switch ( building ) {
        case BUILD_MAGEGUILD1:
            if ( castle.GetLevelMageGuild() > 1 ) {
                return;
            }
            break;
        case BUILD_MAGEGUILD2:
            if ( castle.GetLevelMageGuild() > 2 ) {
                return;
            }
            index = ( Race::NECR == race ) ? 6 : 1;
            break;
        case BUILD_MAGEGUILD3:
            if ( castle.GetLevelMageGuild() > 3 ) {
                return;
            }
            index = ( Race::NECR == race ) ? 12 : 2;
            break;
        case BUILD_MAGEGUILD4:
            if ( castle.GetLevelMageGuild() > 4 ) {
                return;
            }
            index = ( Race::NECR == race ) ? 18 : 3;
            break;
        case BUILD_MAGEGUILD5:
            index = ( Race::NECR == race ) ? 24 : 4;
            break;
        default:
            break;
        }

        const int icn = Castle::GetICNBuilding( building, race );
        if ( icn == ICN::UNKNOWN ) {
            // Have you added a new building? Add the logic for it!
            assert( 0 );
            return;
        }

        const fheroes2::Rect max = CastleGetMaxArea( castle, dst_pt );

        // Building main sprite.
        fheroes2::drawCastleDialogBuilding( icn, index, castle, dst_pt, max, alpha );

        // Special case: Knight castle's flags are overlapped by Right Turret so we need to draw flags after drawing the Turret.
        const bool knightCastleCase = ( race == Race::KNGT && castle.isBuild( BUILD_RIGHTTURRET ) && castle.isBuild( BUILD_CASTLE ) );
        if ( knightCastleCase && building == BUILD_CASTLE ) {
            // Do not draw flags.
            return;
        }

        // Building animation sprite.
        if ( const uint32_t index2 = ICN::AnimationFrame( icn, index, frame ) ) {
            fheroes2::drawCastleDialogBuilding( icn, index2, castle, dst_pt, max, alpha );
        }

        if ( knightCastleCase && building == BUILD_RIGHTTURRET ) {
            // Draw Castle's flags after the Turret.
            const int castleIcn = Castle::GetICNBuilding( BUILD_CASTLE, race );
            const uint32_t flagAnimFrame = ICN::AnimationFrame( castleIcn, index, frame );
            if ( flagAnimFrame > 0 ) {
                fheroes2::drawCastleDialogBuilding( castleIcn, flagAnimFrame, castle, dst_pt, max, alpha );
            }
        }
    }

    void redrawCastleBuildingExtended( const Castle & castle, const fheroes2::Point & dst_pt, const uint32_t building, const uint32_t frame, const uint8_t alpha = 255 )
    {
        if ( building == BUILD_TENT ) {
            // We don't need to draw a tent because it's on the background image.
            return;
        }

        const fheroes2::Rect max = CastleGetMaxArea( castle, dst_pt );
        const int icn = Castle::GetICNBuilding( building, castle.GetRace() );

        if ( building == BUILD_SHIPYARD ) {
            if ( castle.HasBoatNearby() ) {
                const int icn2 = Castle::GetICNBoat( castle.GetRace() );

                fheroes2::drawCastleDialogBuilding( icn2, 0, castle, dst_pt, max, alpha );

                if ( const uint32_t index2 = ICN::AnimationFrame( icn2, 0, frame ) ) {
                    fheroes2::drawCastleDialogBuilding( icn2, index2, castle, dst_pt, max, alpha );
                }
            }
            else {
                if ( const uint32_t index2 = ICN::AnimationFrame( icn, 0, frame ) ) {
                    fheroes2::drawCastleDialogBuilding( icn, index2, castle, dst_pt, max, alpha );
                }
            }
        }
        else if ( building == BUILD_WEL2 && Race::SORC == castle.GetRace() ) {
            const int icn2 = castle.isBuild( BUILD_STATUE ) ? ICN::TWNSEXT1 : icn;

            fheroes2::drawCastleDialogBuilding( icn2, 0, castle, dst_pt, max, alpha );

            if ( const uint32_t index2 = ICN::AnimationFrame( icn2, 0, frame ) ) {
                fheroes2::drawCastleDialogBuilding( icn2, index2, castle, dst_pt, max, alpha );
            }
        }
        else if ( building == BUILD_WEL2 && castle.GetRace() == Race::KNGT && !castle.isBuild( BUILD_CASTLE ) ) {
            const fheroes2::Sprite & rightFarm = fheroes2::AGG::GetICN( ICN::KNIGHT_CASTLE_RIGHT_FARM, 0 );
            const fheroes2::Sprite & leftFarm = fheroes2::AGG::GetICN( ICN::KNIGHT_CASTLE_LEFT_FARM, 0 );
            fheroes2::drawCastleDialogBuilding( ICN::KNIGHT_CASTLE_LEFT_FARM, 0, castle, { dst_pt.x + rightFarm.x() - leftFarm.width(), dst_pt.y + rightFarm.y() }, max,
                                                alpha );
        }
        else if ( building == BUILD_CAPTAIN && castle.GetRace() == Race::BARB && !castle.isBuild( BUILD_CASTLE ) ) {
            const fheroes2::Sprite & rightCaptainQuarters = fheroes2::AGG::GetICN( ICN::TWNBCAPT, 0 );
            const fheroes2::Sprite & leftCaptainQuarters = fheroes2::AGG::GetICN( ICN::BARBARIAN_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE, 0 );
            fheroes2::drawCastleDialogBuilding( ICN::BARBARIAN_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE, 0, castle,
                                                { dst_pt.x + rightCaptainQuarters.x() - leftCaptainQuarters.width(), dst_pt.y + rightCaptainQuarters.y() }, max, alpha );
        }
        else if ( building == BUILD_CAPTAIN && castle.GetRace() == Race::SORC && !castle.isBuild( BUILD_CASTLE ) ) {
            fheroes2::drawCastleDialogBuilding( ICN::SORCERESS_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE, 0, castle, dst_pt, max, alpha );
        }
    }

    void redrawCurrentCastleBuilding( const Castle & castle, const fheroes2::Point & dst_pt, const CastleDialog::CacheBuildings & orders,
                                      const CastleDialog::FadeBuilding & fadeBuilding, const uint32_t animationIndex )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Sprite & townbkg = fheroes2::AGG::GetICN( getTownIcnId( castle.GetRace() ), 0 );
        const fheroes2::Rect max( dst_pt.x, dst_pt.y, townbkg.width(), townbkg.height() );
        fheroes2::Copy( townbkg, 0, 0, display, dst_pt.x, dst_pt.y, max.width, max.height );

        if ( Race::BARB == castle.GetRace() ) {
            const fheroes2::Sprite & sprite0 = fheroes2::AGG::GetICN( ICN::TWNBEXT1, 1 + animationIndex % 5 );
            fheroes2::Blit( sprite0, display, dst_pt.x + sprite0.x(), dst_pt.y + sprite0.y() );
        }

        // Bay animation
        if ( Race::WZRD == castle.GetRace() || ( !castle.isBuild( BUILD_SHIPYARD ) && castle.HasSeaAccess() ) ) {
            int bayIcnId = 0;
            const uint32_t bayExtraIndex = 1 + animationIndex % 5;

            switch ( castle.GetRace() ) {
            case Race::KNGT:
                bayIcnId = ICN::TWNKEXT0;
                break;
            case Race::BARB:
                bayIcnId = ICN::TWNBEXT0;
                break;
            case Race::SORC:
                bayIcnId = ICN::TWNSEXT0;
                break;
            case Race::NECR:
                bayIcnId = ICN::TWNNEXT0;
                break;
            case Race::WRLK:
                bayIcnId = ICN::TWNWEXT0;
                break;
            case Race::WZRD:
                bayIcnId = ICN::TWNZEXT0;
                break;
            default:
                // Did you add a new race? Add the logic for it!
                assert( 0 );
                break;
            }

            fheroes2::drawCastleDialogBuilding( bayIcnId, 0, castle, dst_pt, max );
            fheroes2::drawCastleDialogBuilding( bayIcnId, bayExtraIndex, castle, dst_pt, max );
        }

        if ( fadeBuilding.GetBuilding() == BUILD_NOTHING ) {
            for ( const CastleDialog::BuildingRenderInfo & currentBuild : orders ) {
                if ( !castle.isBuild( currentBuild.id ) ) {
                    continue;
                }

                redrawCastleBuilding( castle, dst_pt, currentBuild.id, animationIndex );
                redrawCastleBuildingExtended( castle, dst_pt, currentBuild.id, animationIndex );

                if ( isBuildingConnectionNeeded( castle, currentBuild.id, false ) ) {
                    redrawBuildingConnection( castle, dst_pt, currentBuild.id );
                }
            }

            return;
        }

        if ( std::find( orders.cbegin(), orders.cend(), fadeBuilding.GetBuilding() ) == orders.cend() ) {
            return;
        }

        for ( const CastleDialog::BuildingRenderInfo & currentBuild : orders ) {
            if ( currentBuild.id == fadeBuilding.GetBuilding() && !fadeBuilding.isOnlyBoat() ) {
                redrawCastleBuilding( castle, dst_pt, currentBuild.id, animationIndex, fadeBuilding.GetAlpha() );
                redrawCastleBuildingExtended( castle, dst_pt, currentBuild.id, animationIndex, fadeBuilding.GetAlpha() );

                if ( isBuildingConnectionNeeded( castle, currentBuild.id, true ) ) {
                    redrawBuildingConnection( castle, dst_pt, currentBuild.id, fadeBuilding.GetAlpha() );
                }

                continue;
            }

            if ( castle.isBuild( currentBuild.id ) ) {
                redrawCastleBuilding( castle, dst_pt, currentBuild.id, animationIndex );

                if ( currentBuild.id == BUILD_SHIPYARD && fadeBuilding.GetBuilding() == BUILD_SHIPYARD ) {
                    redrawCastleBuildingExtended( castle, dst_pt, currentBuild.id, animationIndex, fadeBuilding.GetAlpha() );
                }
                else {
                    redrawCastleBuildingExtended( castle, dst_pt, currentBuild.id, animationIndex );
                }

                if ( isBuildingConnectionNeeded( castle, currentBuild.id, false ) ) {
                    redrawBuildingConnection( castle, dst_pt, fadeBuilding.GetBuilding(), fadeBuilding.GetAlpha() );
                    redrawBuildingConnection( castle, dst_pt, currentBuild.id );
                }
            }
        }
    }
}

CastleDialog::CacheBuildings::CacheBuildings( const Castle & castle, const fheroes2::Point & top )
{
    const std::vector<BuildingType> ordersBuildings = fheroes2::getBuildingDrawingPriorities( castle.GetRace(), Settings::Get().getCurrentMapInfo().version );

    for ( const BuildingType buildingId : ordersBuildings ) {
        emplace_back( buildingId, fheroes2::getCastleBuildingArea( castle.GetRace(), buildingId ) + top );
    }
}

bool CastleDialog::FadeBuilding::UpdateFade()
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

void CastleDialog::RedrawAllBuilding( const Castle & castle, const fheroes2::Point & dst_pt, const CacheBuildings & orders,
                                      const CastleDialog::FadeBuilding & alphaBuilding, const uint32_t animationIndex )
{
    redrawCurrentCastleBuilding( castle, dst_pt, orders, alphaBuilding, animationIndex );
    fheroes2::drawCastleName( castle, fheroes2::Display::instance(), dst_pt );
}
