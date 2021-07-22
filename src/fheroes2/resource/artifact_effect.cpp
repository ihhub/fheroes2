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

#include <cassert>
#include <vector>

#include "artifact_effect.h"

using namespace ArtifactEffects;

namespace
{
    static std::vector<std::vector<ArtifactEffect>> allArtifactsEffects;

    // Defines all the artifact effects for all the game's artifact.
    // This should probably be added to each artifact definition instead but it needs more refactoring before this is possible.
    const std::vector<std::vector<ArtifactEffect>> & getAllArtifactEffects()
    {
        if ( allArtifactsEffects.empty() ) {
            allArtifactsEffects.resize( Artifact::UNKNOWN + 1 );

            // TODO list all effects for all artifacts. For now this only contains the list of all morale effects and all luck effects

            allArtifactsEffects[Artifact::MEDAL_VALOR] = { ArtifactEffect::moraleEffect( +1 ) };
            allArtifactsEffects[Artifact::MEDAL_COURAGE] = { ArtifactEffect::moraleEffect( +1 ) };
            allArtifactsEffects[Artifact::MEDAL_HONOR] = { ArtifactEffect::moraleEffect( +1 ) };
            allArtifactsEffects[Artifact::MEDAL_DISTINCTION] = { ArtifactEffect::moraleEffect( +1 ) };
            allArtifactsEffects[Artifact::FIZBIN_MISFORTUNE] = { ArtifactEffect::moraleEffect( -2 ) };
            allArtifactsEffects[Artifact::RABBIT_FOOT] = { ArtifactEffect::luckEffect( +1 ) };
            allArtifactsEffects[Artifact::GOLDEN_HORSESHOE] = { ArtifactEffect::luckEffect( +1 ) };
            allArtifactsEffects[Artifact::GAMBLER_LUCKY_COIN] = { ArtifactEffect::luckEffect( +1 ) };
            allArtifactsEffects[Artifact::FOUR_LEAF_CLOVER] = { ArtifactEffect::luckEffect( +1 ) };
            allArtifactsEffects[Artifact::BATTLE_GARB]
                = { ArtifactEffect::moraleEffect( +10 ), ArtifactEffect::luckEffect( +10 ) }; // TODO : add in other effects for this artifact
            allArtifactsEffects[Artifact::MASTHEAD]
                = { ArtifactEffect::moraleEffect( +1, /*onlyAppliesOnBoat*/ true ), ArtifactEffect::luckEffect( +1, /*onlyAppliesOnBoat*/ true ) };

            // use this to explicitely mark we have an empty vector of effects for UNKNOWN artifact (or, no artifact)
            allArtifactsEffects[Artifact::UNKNOWN] = {};
        }

        return allArtifactsEffects;
    }

    const std::vector<ArtifactEffect> & getEffectsForArtifact( const Artifact & artifact )
    {
        const auto & effects = getAllArtifactEffects();
        return effects[artifact.GetID()]; // if artifact is unknown this will still work and return an empty vector
    }

    // Defines if a given instance of an artifact effect stack with itself.
    // This does NOT define if several effects of the same type stack with themselves if they come from different artifacts: they all do.
    bool doesArtifactEffectStackWithItself( const ArtifactEffectType effectType )
    {
        switch ( effectType ) {
        case ArtifactEffectType::CHANGE_MORALE:
        case ArtifactEffectType::CHANGE_LUCK:
            return false;
        default:
            return true;
        }
    }
}

ArtifactEffect::ArtifactEffect( ArtifactEffectType type, ArtifactEffectParam params, bool isCursed )
    : _type( type )
    , _parameters( params )
    , _isCursed( isCursed )
{}

bool ArtifactEffect::isCursed() const
{
    return _isCursed;
}

ArtifactEffectType ArtifactEffect::getType() const
{
    return _type;
}

// Morale effect

const MoraleModifierParams & ArtifactEffect::getMoraleModifiersParams() const
{
    assert( _type == ArtifactEffectType::CHANGE_MORALE );
    return _parameters.moraleModifierParams;
}

ArtifactEffect ArtifactEffect::moraleEffect( int modifier, bool onlyAppliesOnBoat )
{
    ArtifactEffectParam params;
    params.moraleModifierParams.modifier = modifier;
    params.moraleModifierParams.onlyAppliesOnBoat = onlyAppliesOnBoat;
    return ArtifactEffect( ArtifactEffectType::CHANGE_MORALE, params, modifier < 0 );
}

// Luck effect

const LuckModifierParams & ArtifactEffect::getLuckModifiersParams() const
{
    assert( _type == ArtifactEffectType::CHANGE_LUCK );
    return _parameters.luckModifierParams;
}

ArtifactEffect ArtifactEffect::luckEffect( int modifier, bool onlyAppliesOnBoat )
{
    ArtifactEffectParam params;
    params.luckModifierParams.modifier = modifier;
    params.luckModifierParams.onlyAppliesOnBoat = onlyAppliesOnBoat;
    return ArtifactEffect( ArtifactEffectType::CHANGE_LUCK, params, modifier < 0 );
}

// Change primary stats effect

const PrimarySkillModifierParams & ArtifactEffect::getPrimarySkillModifiersParams() const
{
    assert( _type == ArtifactEffectType::CHANGE_PRIMARY_STAT );
    return _parameters.primarySkillsModifierParams;
}

ArtifactEffect ArtifactEffect::primaryStatModifierEffect( int primarySkill, int modifier )
{
    ArtifactEffectParam params;
    params.primarySkillsModifierParams.primarySkill = primarySkill;
    params.primarySkillsModifierParams.modifier = modifier;
    return ArtifactEffect( ArtifactEffectType::CHANGE_PRIMARY_STAT, params, modifier < 0 );
}

// Add resource effect

const ResourceModifierParams & ArtifactEffect::getResourceModifierParams() const
{
    assert( _type == ArtifactEffectType::ADD_RESOURCE_PER_TURN );
    return _parameters.resourceModifierParams;
}

ArtifactEffect ArtifactEffect::resourceModifierEffect( int resource, int modifier )
{
    ArtifactEffectParam params;
    params.resourceModifierParams.modifier = modifier;
    params.resourceModifierParams.resource = resource;
    return ArtifactEffect( ArtifactEffectType::ADD_RESOURCE_PER_TURN, params, modifier < 0 );
}

// Add move points effect

const MovePointsModifierParams & ArtifactEffect::getMovePointsModifierParams() const
{
    assert( _type == ArtifactEffectType::ADD_MOVEMENT_POINTS );
    return _parameters.movePointsModifierParams;
}

ArtifactEffect ArtifactEffect::movePointsModifierEffect( int modifier, bool appliesOnLand, bool appliesOnBoat )
{
    ArtifactEffectParam params;
    params.movePointsModifierParams.modifier = modifier;
    params.movePointsModifierParams.appliesOnLand = appliesOnLand;
    params.movePointsModifierParams.appliesOnBoat = appliesOnBoat;
    return ArtifactEffect( ArtifactEffectType::ADD_MOVEMENT_POINTS, params, modifier < 0 );
}

namespace ArtifactEffects
{
    bool HasEffectTypeFromArtifacts( const BagArtifacts & bagArtifacts, const ArtifactEffectType type,
                                     const std::function<bool( const ArtifactEffect & )> & predicate /* = nullptr*/ )
    {
        for ( const auto & artifact : bagArtifacts ) {
            for ( const auto & effect : getEffectsForArtifact( artifact ) ) {
                if ( effect.getType() == type && ( !predicate || predicate( effect ) ) ) {
                    return true;
                }
            }
        }
        return false;
    }

    struct SortEffectsByAdress
    {
        inline bool operator()( const ArtifactEffectAndSource & struct1, const ArtifactEffectAndSource & struct2 )
        {
            return struct1.effect < struct2.effect;
        }
    };

    std::vector<ArtifactEffectAndSource> GetEffectTypesFromArtifacts( const BagArtifacts & bagArtifacts, const ArtifactEffectType type,
                                                                      const std::function<bool( const ArtifactEffect & )> & predicate /*= nullptr*/ )
    {
        // store all relevant effects
        std::vector<ArtifactEffectAndSource> effectsWithDuplicates;
        for ( const auto & artifact : bagArtifacts ) {
            for ( const auto & effect : getEffectsForArtifact( artifact ) ) {
                if ( effect.getType() == type && ( !predicate || predicate( effect ) ) ) {
                    effectsWithDuplicates.emplace_back( ArtifactEffectAndSource( effect, artifact ) );
                }
            }
        }

        // De-duplicate effects that don't stack:

        // nothing to do if the effects stack or if we have too few effects
        if ( doesArtifactEffectStackWithItself( type ) || effectsWithDuplicates.size() < 2 ) {
            return effectsWithDuplicates;
        }

        // Sort effects by adress: same effects coming from having same artifact several times will be contiguous
        std::sort( effectsWithDuplicates.begin(), effectsWithDuplicates.end(), SortEffectsByAdress() );

        // Check once if we really have duplicates : usually it's not the case. In that case, no need to create a new vector
        const ArtifactEffect * previousEffectWithNoStacking = nullptr;
        bool hasDuplicates = false;
        for ( const auto & effect : effectsWithDuplicates ) {
            if ( effect.effect == previousEffectWithNoStacking ) {
                hasDuplicates = true;
                break;
            }
            previousEffectWithNoStacking = effect.effect;
        }

        if ( !hasDuplicates ) {
            return effectsWithDuplicates;
        }

        // We really have duplicates that don't stack: create a new list that removes duplicate effects

        std::vector<ArtifactEffectAndSource> effects;
        effects.reserve( effectsWithDuplicates.size() - 1 );

        previousEffectWithNoStacking = nullptr;
        for ( const auto & effect : effectsWithDuplicates ) {
            if ( effect.effect != previousEffectWithNoStacking ) {
                effects.emplace_back( effect );
            }
            previousEffectWithNoStacking = effect.effect;
        }

        return effects;
    }
}