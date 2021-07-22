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
#pragma once
#include "artifact.h"
#include "resource.h"
#include "skill.h"
#include "spell.h"

namespace ArtifactEffects
{
    enum class ArtifactEffectType : int
    {
        CHANGE_MORALE,
        CHANGE_LUCK,
        CHANGE_PRIMARY_STAT,
        ADD_RESOURCE_PER_TURN,
        ADD_MOVEMENT_POINTS,
        REDUCE_SPELL_COST,
        GRANT_SPELL,
        INCREASE_ALL_SPELLS_DURATION,
        PREVENT_ALL_SPELLCASTING,
        INCREASE_SPELL_DURATION,
        INCREASE_SPELL_EFFECTIVENESS,
        INCREASE_SPELL_DAMAGE,
        INCREASE_SPELL_PROTECTION,
        MAKE_IMMUNE_TO_SPELL,
        REMOVE_SHOOTING_PENALTY,
        INCREASE_SCOUTING_RADIUS,
        REDUCE_SURRENDERING_COST,
        PREVENT_ARMIES_FROM_JOINING,
        ADD_UNDEAD_PENALTY,
        PROVIDE_ENDLESS_AMMO,
        CRYSTAL_BALL_EFFECT,
        INCREASE_NECROMANCY_SKILL
    };

    // Information that details each of the different Artifact Effects:

    struct MoraleModifierParams
    {
        int modifier;
        bool onlyAppliesOnBoat;
    };

    struct LuckModifierParams
    {
        int modifier;
        bool onlyAppliesOnBoat;
    };

    struct PrimarySkillModifierParams
    {
        int modifier;
        int primarySkill;
    };

    struct ResourceModifierParams
    {
        int modifier;
        int resource;
    };

    struct MovePointsModifierParams
    {
        int modifier;
        bool appliesOnLand;
        bool appliesOnBoat;
    };

    struct SpellCostModifierParams
    {
        Spell::type_t spell;
        float modifier;
    };

    struct AddSpellParams
    {
        Spell::type_t spell;
    };

    /*
    This defines a single effect of an artifacts.
    Artifact are composed of multiple artifact effects.
    Each effect defines if it cursed: having a single effect cursed will make the whole artifact item cursed
    */
    class ArtifactEffect
    {
    private:
        union ArtifactEffectParam
        {
            MoraleModifierParams moraleModifierParams;
            LuckModifierParams luckModifierParams;
            PrimarySkillModifierParams primarySkillsModifierParams;
            ResourceModifierParams resourceModifierParams;
            MovePointsModifierParams movePointsModifierParams;
            SpellCostModifierParams spellCostModifierParams;
        };

        ArtifactEffectType _type;
        ArtifactEffectParam _parameters;
        bool _isCursed;

        ArtifactEffect( ArtifactEffectType type, ArtifactEffectParam params, bool isCursed );

    public:
        bool isCursed() const;
        ArtifactEffectType getType() const;

        // Access the parameters related to the artifact type
        const MoraleModifierParams & getMoraleModifiersParams() const;
        const LuckModifierParams & getLuckModifiersParams() const;
        const PrimarySkillModifierParams & getPrimarySkillModifiersParams() const;
        const ResourceModifierParams & getResourceModifierParams() const;
        const MovePointsModifierParams & getMovePointsModifierParams() const;

        // Create different effects
        static ArtifactEffect moraleEffect( int modifier, bool onlyAppliesOnBoat = false );
        static ArtifactEffect luckEffect( int modifier, bool onlyAppliesOnBoat = false );
        static ArtifactEffect primaryStatModifierEffect( int primarySkill, int modifier );
        static ArtifactEffect resourceModifierEffect( int resource, int modifier );
        static ArtifactEffect movePointsModifierEffect( int modifier, bool worksOnLand = true, bool worksOnBoat = true );
    };

    // Check if the effect is present at least once in the bag
    bool HasEffectTypeFromArtifacts( const BagArtifacts & bagArtifact, const ArtifactEffectType type,
                                     const std::function<bool( const ArtifactEffect & )> & predicate = nullptr );

    struct ArtifactEffectAndSource
    {
        const ArtifactEffect * effect;
        const Artifact * sourceArtifact;

        ArtifactEffectAndSource( const ArtifactEffect & artifactEffect, const Artifact & source )
            : effect( &artifactEffect )
            , sourceArtifact( &source )
        {}
    };

    // Retrieves from bag the relevant artifact effects, and where they come from
    std::vector<ArtifactEffectAndSource> GetEffectTypesFromArtifacts( const BagArtifacts & bagArtifact, const ArtifactEffectType type,
                                                                      const std::function<bool( const ArtifactEffect & )> & predicate = nullptr );
}