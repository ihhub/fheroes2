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

#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "artifact_info.h"
#include "image.h"
#include "interface_itemsbar.h"
#include "math_base.h"
#include "ui_tool.h"

class IStreamBase;
class OStreamBase;

class Heroes;
class StatusBar;

namespace MP2
{
    enum MapObjectType : uint16_t;
}

class Artifact
{
public:
    enum ArtLevel
    {
        ART_NONE = 0,
        ART_LEVEL_TREASURE = 0x01,
        ART_LEVEL_MINOR = 0x02,
        ART_LEVEL_MAJOR = 0x04,
        ART_LEVEL_ALL_NORMAL = ART_LEVEL_TREASURE | ART_LEVEL_MINOR | ART_LEVEL_MAJOR,
        ART_ULTIMATE = 0x08,
        ART_LOYALTY = 0x10,
        ART_NORANDOM = 0x20
    };

    // All artifact IDs are by value 1 bigger than in the original game.
    // This is done to support new artifact addition and also align with the rest of object types.
    enum : int
    {
        UNKNOWN = 0,

        // The Succession Wars artifacts.
        ULTIMATE_BOOK,
        ULTIMATE_SWORD,
        ULTIMATE_CLOAK,
        ULTIMATE_WAND,
        ULTIMATE_SHIELD,
        ULTIMATE_STAFF,
        ULTIMATE_CROWN,
        GOLDEN_GOOSE,
        ARCANE_NECKLACE,
        CASTER_BRACELET,
        MAGE_RING,
        WITCHES_BROACH,
        MEDAL_VALOR,
        MEDAL_COURAGE,
        MEDAL_HONOR,
        MEDAL_DISTINCTION,
        FIZBIN_MISFORTUNE,
        THUNDER_MACE,
        ARMORED_GAUNTLETS,
        DEFENDER_HELM,
        GIANT_FLAIL,
        BALLISTA,
        STEALTH_SHIELD,
        DRAGON_SWORD,
        POWER_AXE,
        DIVINE_BREASTPLATE,
        MINOR_SCROLL,
        MAJOR_SCROLL,
        SUPERIOR_SCROLL,
        FOREMOST_SCROLL,
        ENDLESS_SACK_GOLD,
        ENDLESS_BAG_GOLD,
        ENDLESS_PURSE_GOLD,
        NOMAD_BOOTS_MOBILITY,
        TRAVELER_BOOTS_MOBILITY,
        RABBIT_FOOT,
        GOLDEN_HORSESHOE,
        GAMBLER_LUCKY_COIN,
        FOUR_LEAF_CLOVER,
        TRUE_COMPASS_MOBILITY,
        SAILORS_ASTROLABE_MOBILITY,
        EVIL_EYE,
        ENCHANTED_HOURGLASS,
        GOLD_WATCH,
        SKULLCAP,
        ICE_CLOAK,
        FIRE_CLOAK,
        LIGHTNING_HELM,
        EVERCOLD_ICICLE,
        EVERHOT_LAVA_ROCK,
        LIGHTNING_ROD,
        SNAKE_RING,
        ANKH,
        BOOK_ELEMENTS,
        ELEMENTAL_RING,
        HOLY_PENDANT,
        PENDANT_FREE_WILL,
        PENDANT_LIFE,
        SERENITY_PENDANT,
        SEEING_EYE_PENDANT,
        KINETIC_PENDANT,
        PENDANT_DEATH,
        WAND_NEGATION,
        GOLDEN_BOW,
        TELESCOPE,
        STATESMAN_QUILL,
        WIZARD_HAT,
        POWER_RING,
        AMMO_CART,
        TAX_LIEN,
        HIDEOUS_MASK,
        ENDLESS_POUCH_SULFUR,
        ENDLESS_VIAL_MERCURY,
        ENDLESS_POUCH_GEMS,
        ENDLESS_CORD_WOOD,
        ENDLESS_CART_ORE,
        ENDLESS_POUCH_CRYSTAL,
        SPIKED_HELM,
        SPIKED_SHIELD,
        WHITE_PEARL,
        BLACK_PEARL,

        MAGIC_BOOK,

        // This entry is used in Editor for the special victory conditions.
        EDITOR_ANY_ULTIMATE_ARTIFACT,

        // These four artifacts are used in original editor for the Ultimate and Random artifacts.
        // They should not exist and used on the adventure map after the game is properly started.
        // We do not use these or create extra entries for the Ultimate and Random artifacts in editor.
        UNUSED_84,
        UNUSED_85,
        UNUSED_86,

        // The Price of Loyalty artifacts.
        SPELL_SCROLL,
        ARM_MARTYR,
        BREASTPLATE_ANDURAN,
        BROACH_SHIELDING,
        BATTLE_GARB,
        CRYSTAL_BALL,
        HEART_FIRE,
        HEART_ICE,
        HELMET_ANDURAN,
        HOLY_HAMMER,
        LEGENDARY_SCEPTER,
        MASTHEAD,
        SPHERE_NEGATION,
        STAFF_WIZARDRY,
        SWORD_BREAKER,
        SWORD_ANDURAN,
        SPADE_NECROMANCY,

        // Resurrection artifacts.

        // IMPORTANT! Put all new artifacts just above this line.
        ARTIFACT_COUNT
    };

    Artifact( int art = UNKNOWN )
        : id( art > UNKNOWN && art < ARTIFACT_COUNT ? art : UNKNOWN )
        , ext( 0 )
    {
        // Do nothing.
    }

    bool operator==( const Artifact & art ) const
    {
        return id == art.id;
    }

    bool operator!=( const Artifact & art ) const
    {
        return id != art.id;
    }

    int GetID() const
    {
        return id;
    }

    bool isUltimate() const;

    bool containsCurses() const
    {
        return !fheroes2::getArtifactData( id ).curses.empty();
    }

    bool isValid() const
    {
        return id != UNKNOWN && id != EDITOR_ANY_ULTIMATE_ARTIFACT && id < ARTIFACT_COUNT && ( id < UNUSED_84 || id > UNUSED_86 );
    }

    void Reset()
    {
        id = UNKNOWN;
        ext = 0;
    }

    int Level() const;
    int LoyaltyLevel() const;

    double getArtifactValue() const;

    // artfx.icn
    uint32_t IndexSprite32() const
    {
        if ( id == UNKNOWN ) {
            return 255;
        }

        return id - 1;
    }

    // return index from artifact.icn
    uint32_t IndexSprite64() const
    {
        return id;
    }

    void SetSpell( const int v );
    int32_t getSpellId() const;

    const char * GetName() const;

    std::string GetDescription() const
    {
        return fheroes2::getArtifactData( id ).getDescription( ext );
    }

    static int Rand( ArtLevel );
    static Artifact getArtifactFromMapSpriteIndex( const uint32_t index );
    static const char * getDiscoveryDescription( const Artifact & );

private:
    friend OStreamBase & operator<<( OStreamBase & stream, const Artifact & art );
    friend IStreamBase & operator>>( IStreamBase & stream, Artifact & art );

    int id;
    int ext;
};

uint32_t GoldInsteadArtifact( const MP2::MapObjectType objectType );

namespace fheroes2
{
    void ResetArtifactStats();
    void ExcludeArtifactFromRandom( const int artifactID );

    bool isPriceOfLoyaltyArtifact( const int artifactID );
}

struct ArtifactSetData
{
    ArtifactSetData( const int32_t artifactID, std::string assembleMessage )
        : _assembledArtifactID( artifactID )
        , _assembleMessage( std::move( assembleMessage ) )
    {
        // Do nothing.
    }

    int32_t _assembledArtifactID = Artifact::UNKNOWN;
    std::string _assembleMessage;

    bool operator<( const ArtifactSetData & other ) const;
};

class BagArtifacts : public std::vector<Artifact>
{
public:
    // Maximum number of artifacts that can be placed in an artifact bag
    static constexpr size_t maxCapacity{ 14 };

    BagArtifacts();

    bool ContainSpell( const int spellId ) const;
    bool isPresentArtifact( const Artifact & art ) const;

    bool isArtifactBonusPresent( const fheroes2::ArtifactBonusType type ) const;
    bool isArtifactCursePresent( const fheroes2::ArtifactCurseType type ) const;

    // These methods must be called only for bonuses with cumulative effect.
    int32_t getTotalArtifactEffectValue( const fheroes2::ArtifactBonusType bonus ) const;
    int32_t getTotalArtifactEffectValue( const fheroes2::ArtifactBonusType bonus, std::string & description ) const;

    int32_t getTotalArtifactEffectValue( const fheroes2::ArtifactCurseType curse ) const;
    int32_t getTotalArtifactEffectValue( const fheroes2::ArtifactCurseType curse, std::string & description ) const;

    // These methods must be called only for bonuses with multiplication effect.
    std::vector<int32_t> getTotalArtifactMultipliedPercent( const fheroes2::ArtifactBonusType bonus ) const;
    std::vector<int32_t> getTotalArtifactMultipliedPercent( const fheroes2::ArtifactCurseType curse ) const;

    // Ideally, these methods should be called only for unique bonuses. However, it can be called for other bonus types.
    Artifact getFirstArtifactWithBonus( const fheroes2::ArtifactBonusType bonus ) const;
    Artifact getFirstArtifactWithCurse( const fheroes2::ArtifactCurseType curse ) const;

    bool PushArtifact( const Artifact & art );

    // Removes the first found instance of the specified artifact from the artifact bag
    void RemoveArtifact( const Artifact & art );

    bool isFull() const;
    bool ContainUltimateArtifact() const;

    // Automatically exchange artifacts between two heroes. The taker should get the best possible artifacts.
    static void exchangeArtifacts( Heroes & taker, Heroes & giver );

    double getArtifactValue() const;
    uint32_t CountArtifacts() const;
    uint32_t Count( const Artifact & art ) const;

    std::set<ArtifactSetData> assembleArtifactSetIfPossible();

    std::string String() const;
};

class ArtifactsBar : public Interface::ItemsActionBar<Artifact>
{
public:
    using Interface::ItemsActionBar<Artifact>::RedrawItem;
    using Interface::ItemsActionBar<Artifact>::ActionBarRightMouseHold;

    ArtifactsBar( Heroes * hero, const bool mini, const bool ro, const bool change, const bool allowOpeningMagicBook, StatusBar * bar );

    void RedrawBackground( const fheroes2::Rect &, fheroes2::Image & ) override;
    void RedrawItem( Artifact &, const fheroes2::Rect &, bool, fheroes2::Image & ) override;

    void ResetSelected();
    void Redraw( fheroes2::Image & dstsf );

    bool ActionBarLeftMouseSingleClick( Artifact & artifact ) override;
    bool ActionBarLeftMouseSingleClick( Artifact & artifact1, Artifact & artifact2 ) override;
    bool ActionBarLeftMouseDoubleClick( Artifact & artifact ) override;
    bool ActionBarRightMouseHold( Artifact & artifact ) override;

    bool QueueEventProcessing( std::string * = nullptr );
    bool QueueEventProcessing( ArtifactsBar &, std::string * = nullptr );

    bool ActionBarCursor( Artifact & ) override;
    bool ActionBarCursor( Artifact &, Artifact & ) override;

protected:
    fheroes2::MovableSprite spcursor;

private:
    Heroes * _hero;
    fheroes2::Image backsf;
    const bool use_mini_sprite;
    const bool read_only;
    const bool can_change;
    const bool _allowOpeningMagicBook;
    StatusBar * _statusBar;
    std::string msg;

    static bool isMagicBook( const Artifact & artifact );

    void messageMagicBookAbortTrading() const;
};
