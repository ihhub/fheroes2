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

    bool isBuildingConnectionNeeded( const Castle & castle, const uint32_t building )
    {
        const int race = castle.GetRace();

        if ( race == Race::BARB ) {
            if ( building & BUILD_MAGEGUILD ) {
                const int mageGuildLevel = castle.GetLevelMageGuild();
                assert( mageGuildLevel > 0 );

                return building == ( BUILD_MAGEGUILD1 << ( mageGuildLevel - 1 ) );
            }

            if ( building == BUILD_THIEVESGUILD ) {
                return true;
            }
        }
        else if ( race == Race::NECR && building == BUILD_CAPTAIN ) {
            return true;
        }

        return false;
    }

    bool isBuildingFullyBuilt( const Castle & castle, const uint32_t building, const uint32_t buildingCurrentlyUnderConstruction )
    {
        return castle.isBuild( building ) && building != buildingCurrentlyUnderConstruction;
    }

    void redrawBuildingConnection( const Castle & castle, const fheroes2::Point & position, const uint32_t building, const uint32_t buildingCurrentlyUnderConstruction,
                                   const uint8_t alpha = 255 )
    {
        const fheroes2::Rect & roi = CastleGetMaxArea( castle, position );

        const int race = castle.GetRace();

        if ( race == Race::BARB ) {
            if ( building & BUILD_MAGEGUILD || building == BUILD_SPEC ) {
                if ( building & BUILD_MAGEGUILD && ( !castle.isBuild( building ) || !isBuildingFullyBuilt( castle, BUILD_SPEC, buildingCurrentlyUnderConstruction ) ) ) {
                    return;
                }

                if ( building == BUILD_SPEC
                     && ( !castle.isBuild( building ) || !isBuildingFullyBuilt( castle, BUILD_MAGEGUILD1, buildingCurrentlyUnderConstruction ) ) ) {
                    return;
                }

                fheroes2::drawCastleDialogBuilding( ICN::TWNBEXT2, 0, castle, position, roi, alpha );
            }

            if ( building == DWELLING_MONSTER3 || building == BUILD_THIEVESGUILD ) {
                if ( building == DWELLING_MONSTER3
                     && ( !castle.isBuild( building ) || !isBuildingFullyBuilt( castle, BUILD_THIEVESGUILD, buildingCurrentlyUnderConstruction ) ) ) {
                    return;
                }

                if ( building == BUILD_THIEVESGUILD
                     && ( !castle.isBuild( building ) || !isBuildingFullyBuilt( castle, DWELLING_MONSTER3, buildingCurrentlyUnderConstruction ) ) ) {
                    return;
                }

                fheroes2::drawCastleDialogBuilding( ICN::TWNBEXT3, 0, castle, position, roi, alpha );
            }
        }
        else if ( race == Race::NECR ) {
            if ( building == BUILD_CAPTAIN && ( !castle.isBuild( building ) || !isBuildingFullyBuilt( castle, BUILD_CASTLE, buildingCurrentlyUnderConstruction ) ) ) {
                return;
            }

            if ( building == BUILD_CASTLE && ( !castle.isBuild( building ) || !isBuildingFullyBuilt( castle, BUILD_CAPTAIN, buildingCurrentlyUnderConstruction ) ) ) {
                return;
            }

            fheroes2::drawCastleDialogBuilding( ICN::NECROMANCER_CASTLE_CAPTAIN_QUARTERS_BRIDGE, 0, castle, position, roi, alpha );
        }
    }

    void redrawCastleBuilding( const Castle & castle, const fheroes2::Point & offset, const uint32_t building, const uint32_t frame,
                               const uint32_t buildingCurrentlyUnderConstruction, const uint8_t alpha = 255 )
    {
        if ( building == BUILD_TENT ) {
            // We don't need to draw a tent because it's on the background image.
            return;
        }

        if ( building & BUILD_MAGEGUILD ) {
            assert( ( [&castle, building]() {
                const int mageGuildLevel = castle.GetLevelMageGuild();

                // If we are going to draw a Mage Guild building, then the castle should have a Mage Guild
                if ( mageGuildLevel < 1 ) {
                    return false;
                }

                // The drawing of the Mage Guild building of the currently built level is allowed
                if ( building == ( BUILD_MAGEGUILD1 << ( mageGuildLevel - 1 ) ) ) {
                    return true;
                }

                // If we are going to draw a Mage Guild building not of the currently built level, then the castle should have at least a Level 2 Mage Guild
                if ( mageGuildLevel < 2 ) {
                    return false;
                }

                // The drawing of the Mage Guild building of the previous level is allowed as well, so that we can draw the "upgrade" animation
                if ( building == ( BUILD_MAGEGUILD1 << ( mageGuildLevel - 2 ) ) ) {
                    return true;
                }

                // In all other cases, we should never draw this Mage Guild building
                return false;
            }() ) );
        }

        const int race = castle.GetRace();

        // For the Sorceress town when Statue and Crystal Garden are both built they have an extra image ICN::TWNSEXT1
        // that should be used instead of separate Statue and Crystal Garden images render.
        // This extra image is rendered in redrawCastleBuildingExtended() function.
        // When Statue is constructing (fade-in animation) its render is also skipped here and is done in redrawCastleBuildingExtended().
        if ( race == Race::SORC && ( building == BUILD_STATUE || building == BUILD_WEL2 ) && castle.isBuild( BUILD_STATUE )
             && isBuildingFullyBuilt( castle, BUILD_WEL2, buildingCurrentlyUnderConstruction ) ) {
            return;
        }

        const uint32_t index = [building, race]() -> uint32_t {
            switch ( building ) {
            case BUILD_MAGEGUILD2:
                return ( race == Race::NECR ? 6 : 1 );
            case BUILD_MAGEGUILD3:
                return ( race == Race::NECR ? 12 : 2 );
            case BUILD_MAGEGUILD4:
                return ( race == Race::NECR ? 18 : 3 );
            case BUILD_MAGEGUILD5:
                return ( race == Race::NECR ? 24 : 4 );
            default:
                break;
            }

            return 0;
        }();

        const int icn = Castle::GetICNBuilding( building, race );
        if ( icn == ICN::UNKNOWN ) {
            // Have you added a new building? Add the logic for it!
            assert( 0 );
            return;
        }

        const fheroes2::Rect roi = CastleGetMaxArea( castle, offset );

        // Building main sprite.
        fheroes2::drawCastleDialogBuilding( icn, index, castle, offset, roi, alpha );

        // Special case: Knight castle's flags are overlapped by Right Turret so we need to draw flags after drawing the Turret.
        const bool knightCastleCase = ( race == Race::KNGT && isBuildingFullyBuilt( castle, BUILD_RIGHTTURRET, buildingCurrentlyUnderConstruction )
                                        && isBuildingFullyBuilt( castle, BUILD_CASTLE, buildingCurrentlyUnderConstruction ) );
        if ( knightCastleCase && building == BUILD_CASTLE ) {
            // Do not draw flags.
            return;
        }

        // Building animation sprite.
        if ( const uint32_t index2 = ICN::getAnimatedIcnIndex( icn, index, frame ); index2 != 0 ) {
            fheroes2::drawCastleDialogBuilding( icn, index2, castle, offset, roi, alpha );
        }

        if ( knightCastleCase && building == BUILD_RIGHTTURRET ) {
            // Draw Castle's flags after the Turret.
            const int castleIcn = Castle::GetICNBuilding( BUILD_CASTLE, race );
            const uint32_t flagAnimFrame = ICN::getAnimatedIcnIndex( castleIcn, index, frame );
            if ( flagAnimFrame > 0 ) {
                fheroes2::drawCastleDialogBuilding( castleIcn, flagAnimFrame, castle, offset, roi, alpha );
            }
        }
    }

    void redrawCastleBuildingExtended( const Castle & castle, const fheroes2::Point & offset, const uint32_t building, const uint32_t frame,
                                       const uint32_t buildingCurrentlyUnderConstruction, const uint8_t alpha = 255 )
    {
        if ( building == BUILD_TENT ) {
            // We don't need to draw a tent because it's on the background image.
            return;
        }

        const fheroes2::Rect roi = CastleGetMaxArea( castle, offset );
        const int castleRace = castle.GetRace();

        if ( building == BUILD_SHIPYARD ) {
            if ( castle.HasBoatNearby() ) {
                const int icn2 = Castle::GetICNBoat( castleRace );

                fheroes2::drawCastleDialogBuilding( icn2, 0, castle, offset, roi, alpha );

                if ( const uint32_t index2 = ICN::getAnimatedIcnIndex( icn2, 0, frame ) ) {
                    fheroes2::drawCastleDialogBuilding( icn2, index2, castle, offset, roi, alpha );
                }
            }
            else {
                const int icn = Castle::GetICNBuilding( building, castleRace );
                if ( const uint32_t index2 = ICN::getAnimatedIcnIndex( icn, 0, frame ) ) {
                    fheroes2::drawCastleDialogBuilding( icn, index2, castle, offset, roi, alpha );
                }
            }
        }
        else if ( building == BUILD_STATUE ) {
            if ( castleRace == Race::SORC && buildingCurrentlyUnderConstruction == BUILD_STATUE
                 && isBuildingFullyBuilt( castle, BUILD_WEL2, buildingCurrentlyUnderConstruction ) ) {
                // This is an extra case for the Sorceress castle Statue fade-in animation when Crystal Garden is already built.
                // It is done to swap render priority of these objects during this animation.

                const int gardenIcn = Castle::GetICNBuilding( BUILD_WEL2, castleRace );

                fheroes2::drawCastleDialogBuilding( gardenIcn, 0, castle, offset, roi );
                if ( const uint32_t index = ICN::getAnimatedIcnIndex( gardenIcn, 0, frame ) ) {
                    fheroes2::drawCastleDialogBuilding( gardenIcn, index, castle, offset, roi );
                }

                // Fade-in Statue.
                fheroes2::drawCastleDialogBuilding( ICN::TWNSEXT1, 0, castle, offset, roi, alpha );
            }
        }
        else if ( building == BUILD_WEL2 ) {
            if ( castleRace == Race::SORC ) {
                if ( buildingCurrentlyUnderConstruction != BUILD_STATUE && isBuildingFullyBuilt( castle, BUILD_STATUE, buildingCurrentlyUnderConstruction ) ) {
                    // This is an extra case for the Sorceress castle when Statue and Crystal Garden are built.
                    // They are both rendered using ICN::TWNSEXT1.

                    fheroes2::drawCastleDialogBuilding( ICN::TWNSEXT1, 0, castle, offset, roi, alpha );
                    if ( const uint32_t index2 = ICN::getAnimatedIcnIndex( ICN::TWNSEXT1, 0, frame ) ) {
                        fheroes2::drawCastleDialogBuilding( ICN::TWNSEXT1, index2, castle, offset, roi, alpha );
                    }
                }
            }
            else if ( castleRace == Race::KNGT && !isBuildingFullyBuilt( castle, BUILD_CASTLE, buildingCurrentlyUnderConstruction ) ) {
                const fheroes2::Sprite & rightFarm = fheroes2::AGG::GetICN( ICN::KNIGHT_CASTLE_RIGHT_FARM, 0 );
                const fheroes2::Sprite & leftFarm = fheroes2::AGG::GetICN( ICN::KNIGHT_CASTLE_LEFT_FARM, 0 );
                fheroes2::drawCastleDialogBuilding( ICN::KNIGHT_CASTLE_LEFT_FARM, 0, castle, { offset.x + rightFarm.x() - leftFarm.width(), offset.y + rightFarm.y() },
                                                    roi, alpha );
            }
        }
        else if ( building == BUILD_CAPTAIN ) {
            if ( castleRace == Race::BARB ) {
                if ( !isBuildingFullyBuilt( castle, BUILD_CASTLE, buildingCurrentlyUnderConstruction ) ) {
                    const fheroes2::Sprite & rightCaptainQuarters = fheroes2::AGG::GetICN( ICN::TWNBCAPT, 0 );
                    const fheroes2::Sprite & leftCaptainQuarters = fheroes2::AGG::GetICN( ICN::BARBARIAN_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE, 0 );
                    fheroes2::drawCastleDialogBuilding( ICN::BARBARIAN_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE, 0, castle,
                                                        { offset.x + rightCaptainQuarters.x() - leftCaptainQuarters.width(), offset.y + rightCaptainQuarters.y() }, roi,
                                                        alpha );
                }
            }
            else if ( castleRace == Race::SORC && !isBuildingFullyBuilt( castle, BUILD_CASTLE, buildingCurrentlyUnderConstruction ) ) {
                fheroes2::drawCastleDialogBuilding( ICN::SORCERESS_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE, 0, castle, offset, roi, alpha );
            }
        }
    }

    void redrawCastleBuildings( const Castle & castle, const fheroes2::Point & offset, const CastleDialog::BuildingsRenderQueue & buildings,
                                const CastleDialog::FadeBuilding & fadeBuilding, const uint32_t animationIndex )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const int castleRace = castle.GetRace();

        const fheroes2::Sprite & townbkg = fheroes2::AGG::GetICN( getTownIcnId( castleRace ), 0 );
        const fheroes2::Rect max( offset.x, offset.y, townbkg.width(), townbkg.height() );
        fheroes2::Copy( townbkg, 0, 0, display, offset.x, offset.y, max.width, max.height );

        if ( castleRace == Race::BARB ) {
            // Render the river animation for the Barbarian castle.
            const fheroes2::Sprite & sprite0 = fheroes2::AGG::GetICN( ICN::TWNBEXT1, 1 + animationIndex % 5 );
            fheroes2::Blit( sprite0, display, offset.x + sprite0.x(), offset.y + sprite0.y() );
        }

        const uint32_t fadingInBuildingId = fadeBuilding.getBuilding();

        // Bay animation. The Wizard's castle is "special": its "bay" is not actually a bay, but a river flowing through a gorge in the wastelands, which must be drawn
        // and animated, even if the castle itself is not located on the seashore.
        if ( castleRace == Race::WZRD || ( castle.HasSeaAccess() && ( !castle.isBuild( BUILD_SHIPYARD ) || fadingInBuildingId == BUILD_SHIPYARD ) ) ) {
            int bayIcnId = 0;
            const uint32_t bayExtraIndex = 1 + animationIndex % 5;

            switch ( castleRace ) {
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

            fheroes2::drawCastleDialogBuilding( bayIcnId, 0, castle, offset, max );
            fheroes2::drawCastleDialogBuilding( bayIcnId, bayExtraIndex, castle, offset, max );
        }

        if ( fadingInBuildingId == BUILD_NOTHING ) {
            for ( const CastleDialog::BuildingRenderInfo & currentBuild : buildings ) {
                if ( !castle.isBuild( currentBuild.id ) ) {
                    continue;
                }

                // Only draw this building if an upgraded version of this building has not yet been built
                const BuildingType upgradeForCurrentBuilding = fheroes2::getUpgradeForBuilding( castleRace, currentBuild.id );
                if ( upgradeForCurrentBuilding != currentBuild.id && castle.isBuild( upgradeForCurrentBuilding ) ) {
                    continue;
                }

                redrawCastleBuilding( castle, offset, currentBuild.id, animationIndex, fadingInBuildingId );
                redrawCastleBuildingExtended( castle, offset, currentBuild.id, animationIndex, fadingInBuildingId );

                if ( isBuildingConnectionNeeded( castle, currentBuild.id ) ) {
                    redrawBuildingConnection( castle, offset, currentBuild.id, fadingInBuildingId );
                }
            }

            return;
        }

        if ( std::find( buildings.cbegin(), buildings.cend(), fadingInBuildingId ) == buildings.cend() ) {
            // No building construction fade-in animation is needed.
            return;
        }

        const uint8_t fadeAlpha = fadeBuilding.getAlpha();

        for ( const CastleDialog::BuildingRenderInfo & currentBuild : buildings ) {
            if ( !castle.isBuild( currentBuild.id ) ) {
                continue;
            }

            // Only draw this building if an upgraded version of this building has either not been built yet, or is still under construction at the moment
            const BuildingType upgradeForCurrentBuilding = fheroes2::getUpgradeForBuilding( castleRace, currentBuild.id );
            if ( upgradeForCurrentBuilding != currentBuild.id && castle.isBuild( upgradeForCurrentBuilding ) && upgradeForCurrentBuilding != fadingInBuildingId ) {
                continue;
            }

            if ( currentBuild.id == fadingInBuildingId && !fadeBuilding.isOnlyBoat() ) {
                redrawCastleBuilding( castle, offset, currentBuild.id, animationIndex, fadingInBuildingId, fadeAlpha );
                redrawCastleBuildingExtended( castle, offset, currentBuild.id, animationIndex, fadingInBuildingId, fadeAlpha );

                if ( isBuildingConnectionNeeded( castle, currentBuild.id ) ) {
                    redrawBuildingConnection( castle, offset, currentBuild.id, fadingInBuildingId, fadeAlpha );
                }

                continue;
            }

            redrawCastleBuilding( castle, offset, currentBuild.id, animationIndex, fadingInBuildingId );

            if ( currentBuild.id == BUILD_SHIPYARD && fadingInBuildingId == BUILD_SHIPYARD ) {
                redrawCastleBuildingExtended( castle, offset, currentBuild.id, animationIndex, fadingInBuildingId, fadeAlpha );
            }
            else {
                redrawCastleBuildingExtended( castle, offset, currentBuild.id, animationIndex, fadingInBuildingId );
            }

            if ( isBuildingConnectionNeeded( castle, currentBuild.id ) ) {
                redrawBuildingConnection( castle, offset, fadingInBuildingId, fadingInBuildingId, fadeAlpha );
                redrawBuildingConnection( castle, offset, currentBuild.id, fadingInBuildingId );
            }
        }
    }
}

CastleDialog::BuildingsRenderQueue::BuildingsRenderQueue( const Castle & castle, const fheroes2::Point & top )
{
    const int castleRace = castle.GetRace();
    const std::vector<BuildingType> ordersBuildings = fheroes2::getBuildingDrawingPriorities( castleRace, Settings::Get().getCurrentMapInfo().version );

    for ( const BuildingType buildingId : ordersBuildings ) {
        emplace_back( buildingId, fheroes2::getCastleBuildingArea( castleRace, buildingId ) + top );
    }
}

bool CastleDialog::FadeBuilding::updateFadeAlpha()
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

void CastleDialog::redrawAllBuildings( const Castle & castle, const fheroes2::Point & offset, const BuildingsRenderQueue & buildings,
                                       const CastleDialog::FadeBuilding & alphaBuilding, const uint32_t animationIndex )
{
    redrawCastleBuildings( castle, offset, buildings, alphaBuilding, animationIndex );
    fheroes2::drawCastleName( castle, fheroes2::Display::instance(), offset );
}
