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
#include <string>
#include <vector>

#include "agg.h"
#include "artifact.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game.h"
#include "heroes.h"
#include "settings.h"
#include "spell.h"
#include "text.h"
#include "world.h"

enum
{
    ART_DISABLED = 0x01,
    ART_RNDUSED = 0x02
};
enum
{
    TYPE0,
    TYPE1,
    TYPE2,
    TYPE3,
    TYPE4
}; /*TYPE0: unique, TYPE1: morale/luck, TYPE2: resource, TYPE3: primary/mp/sp, TYPE4: secskills */

struct artifactstats_t
{
    u8 bits;
    u8 extra;
    u8 type;
    const char * name;
    const char * description;
};

artifactstats_t artifacts[] = {
    {0, 12, TYPE3, _( "Ultimate Book of Knowledge" ), _( "The %{name} increases your knowledge by %{count}." )},
    {0, 12, TYPE3, _( "Ultimate Sword of Dominion" ), _( "The %{name} increases your attack skill by %{count}." )},
    {0, 12, TYPE3, _( "Ultimate Cloak of Protection" ), _( "The %{name} increases your defense skill by %{count}." )},
    {0, 12, TYPE3, _( "Ultimate Wand of Magic" ), _( "The %{name} increases your spell power by %{count}." )},
    {0, 6, TYPE3, _( "Ultimate Shield" ), _( "The %{name} increases your attack and defense skills by %{count} each." )},
    {0, 6, TYPE3, _( "Ultimate Staff" ), _( "The %{name} increases your spell power and knowledge by %{count} each." )},
    {0, 4, TYPE3, _( "Ultimate Crown" ), _( "The %{name} increases each of your basic skills by %{count} points." )},
    {0, 10, TYPE2, _( "Golden Goose" ), _( "The %{name} brings in an income of %{count} gold per turn." )},
    {0, 4, TYPE3, _( "Arcane Necklace of Magic" ), _( "The %{name} increases your spell power by %{count}." )},
    {0, 2, TYPE3, _( "Caster's Bracelet of Magic" ), _( "The %{name} increases your spell power by %{count}." )},
    {0, 2, TYPE3, _( "Mage's Ring of Power" ), _( "The %{name} increases your spell power by %{count}." )},
    {0, 3, TYPE3, _( "Witch's Broach of Magic" ), _( "The %{name} increases your spell power by %{count}." )},
    {0, 1, TYPE1, _( "Medal of Valor" ), _( "The %{name} increases your morale." )},
    {0, 1, TYPE1, _( "Medal of Courage" ), _( "The %{name} increases your morale." )},
    {0, 1, TYPE1, _( "Medal of Honor" ), _( "The %{name} increases your morale." )},
    {0, 1, TYPE1, _( "Medal of Distinction" ), _( "The %{name} increases your morale." )},
    {0, 2, TYPE1, _( "Fizbin of Misfortune" ), _( "The %{name} greatly decreases your morale by %{count}." )},
    {0, 1, TYPE3, _( "Thunder Mace of Dominion" ), _( "The %{name} increases your attack skill by %{count}." )},
    {0, 1, TYPE3, _( "Armored Gauntlets of Protection" ), _( "The %{name} increase your defense skill by %{count}." )},
    {0, 1, TYPE3, _( "Defender Helm of Protection" ), _( "The %{name} increases your defense skill by %{count}." )},
    {0, 1, TYPE3, _( "Giant Flail of Dominion" ), _( "The %{name} increases your attack skill by %{count}." )},
    {0, 2, TYPE0, _( "Ballista of Quickness" ), _( "The %{name} lets your catapult fire twice per combat round." )},
    {0, 2, TYPE3, _( "Stealth Shield of Protection" ), _( "The %{name} increases your defense skill by %{count}." )},
    {0, 3, TYPE3, _( "Dragon Sword of Dominion" ), _( "The %{name} increases your attack skill by %{count}." )},
    {0, 2, TYPE3, _( "Power Axe of Dominion" ), _( "The %{name} increases your attack skill by %{count}." )},
    {0, 3, TYPE3, _( "Divine Breastplate of Protection" ), _( "The %{name} increases your defense skill by %{count}." )},
    {0, 2, TYPE3, _( "Minor Scroll of Knowledge" ), _( "The %{name} increases your knowledge by %{count}." )},
    {0, 3, TYPE3, _( "Major Scroll of Knowledge" ), _( "The %{name} increases your knowledge by %{count}." )},
    {0, 4, TYPE3, _( "Superior Scroll of Knowledge" ), _( "The %{name} increases your knowledge by %{count}." )},
    {0, 5, TYPE3, _( "Foremost Scroll of Knowledge" ), _( "The %{name} increases your knowledge by %{count}." )},
    {0, 100, TYPE2, _( "Endless Sack of Gold" ), _( "The %{name} provides you with %{count} gold per day." )},
    {0, 75, TYPE2, _( "Endless Bag of Gold" ), _( "The %{name} provides you with %{count} gold per day." )},
    {0, 50, TYPE2, _( "Endless Purse of Gold" ), _( "The %{name} provides you with %{count} gold per day." )},
    {0, 0, TYPE3, _( "Nomad Boots of Mobility" ), _( "The %{name} increase your movement on land." )},
    {0, 0, TYPE3, _( "Traveler's Boots of Mobility" ), _( "The %{name} increase your movement on land." )},
    {0, 1, TYPE1, _( "Lucky Rabbit's Foot" ), _( "The %{name} increases your luck in combat." )},
    {0, 1, TYPE1, _( "Golden Horseshoe" ), _( "The %{name} increases your luck in combat." )},
    {0, 1, TYPE1, _( "Gambler's Lucky Coin" ), _( "The %{name} increases your luck in combat." )},
    {0, 1, TYPE1, _( "Four-Leaf Clover" ), _( "The %{name} increases your luck in combat." )},
    {0, 0, TYPE3, _( "True Compass of Mobility" ), _( "The %{name} increases your movement on land and sea." )},
    {0, 0, TYPE3, _( "Sailor's Astrolabe of Mobility" ), _( "The %{name} increases your movement on sea." )},
    {0, 0, TYPE0, _( "Evil Eye" ), _( "The %{name} reduces the casting cost of curse spells by half." )},
    {0, 2, TYPE0, _( "Enchanted Hourglass" ), _( "The %{name} extends the duration of all your spells by %{count} turns." )},
    {0, 0, TYPE0, _( "Gold Watch" ), _( "The %{name} doubles the effectiveness of your hypnotize spells." )},
    {0, 0, TYPE0, _( "Skullcap" ), _( "The %{name} halves the casting cost of all mind influencing spells." )},
    {0, 50, TYPE0, _( "Ice Cloak" ), _( "The %{name} halves all damage your troops take from cold spells." )},
    {0, 50, TYPE0, _( "Fire Cloak" ), _( "The %{name} halves all damage your troops take from fire spells." )},
    {0, 50, TYPE0, _( "Lightning Helm" ), _( "The %{name} halves all damage your troops take from lightning spells." )},
    {0, 50, TYPE0, _( "Evercold Icicle" ), _( "The %{name} causes your cold spells to do %{count} percent more damage to enemy troops." )},
    {0, 50, TYPE0, _( "Everhot Lava Rock" ), _( "The %{name} causes your fire spells to do %{count} percent more damage to enemy troops." )},
    {0, 50, TYPE0, _( "Lightning Rod" ), _( "The %{name} causes your lightning spells to do %{count} percent more damage to enemy troops." )},
    {0, 0, TYPE0, _( "Snake-Ring" ), _( "The %{name} halves the casting cost of all your bless spells." )},
    {0, 0, TYPE0, _( "Ankh" ), _( "The %{name} doubles the effectiveness of all your resurrect and animate spells." )},
    {0, 0, TYPE0, _( "Book of Elements" ), _( "The %{name} doubles the effectiveness of all your summoning spells." )},
    {0, 0, TYPE0, _( "Elemental Ring" ), _( "The %{name} halves the casting cost of all summoning spells." )},
    {0, 0, TYPE0, _( "Holy Pendant" ), _( "The %{name} makes all your troops immune to curse spells." )},
    {0, 0, TYPE0, _( "Pendant of Free Will" ), _( "The %{name} makes all your troops immune to hypnotize spells." )},
    {0, 0, TYPE0, _( "Pendant of Life" ), _( "The %{name} makes all your troops immune to death spells." )},
    {0, 0, TYPE0, _( "Serenity Pendant" ), _( "The %{name} makes all your troops immune to berserk spells." )},
    {0, 0, TYPE0, _( "Seeing-eye Pendant" ), _( "The %{name} makes all your troops immune to blindness spells." )},
    {0, 0, TYPE0, _( "Kinetic Pendant" ), _( "The %{name} makes all your troops immune to paralyze spells." )},
    {0, 0, TYPE0, _( "Pendant of Death" ), _( "The %{name} makes all your troops immune to holy spells." )},
    {0, 0, TYPE0, _( "Wand of Negation" ), _( "The %{name} protects your troops from the Dispel Magic spell." )},
    {0, 50, TYPE0, _( "Golden Bow" ), _( "The %{name} eliminates the %{count} percent penalty for your troops shooting past obstacles (e.g. castle walls)." )},
    {0, 1, TYPE4, _( "Telescope" ), _( "The %{name} increases the amount of terrain your hero reveals when adventuring by %{count} extra square." )},
    {0, 10, TYPE0, _( "Statesman's Quill" ), _( "The %{name} reduces the cost of surrender to %{count} percent of the total cost of troops you have in your army." )},
    {0, 10, TYPE0, _( "Wizard's Hat" ), _( "The %{name} increases the duration of your spells by %{count} turns." )},
    {0, 2, TYPE4, _( "Power Ring" ), _( "The %{name} returns %{count} extra power points/turn to your hero." )},
    {0, 0, TYPE0, _( "Ammo Cart" ), _( "The %{name} provides endless ammunition for all your troops that shoot." )},
    {0, 25, TYPE2, _( "Tax Lien" ), _( "The %{name} costs you %{count} gold pieces/turn." )},
    {0, 0, TYPE0, _( "Hideous Mask" ), _( "The %{name} prevents all 'wandering' armies from joining your hero." )},
    {0, 1, TYPE2, _( "Endless Pouch of Sulfur" ), _( "The %{name} provides %{count} unit of sulfur per day." )},
    {0, 1, TYPE2, _( "Endless Vial of Mercury" ), _( "The %{name} provides %{count} unit of mercury per day." )},
    {0, 1, TYPE2, _( "Endless Pouch of Gems" ), _( "The %{name} provides %{count} unit of gems per day." )},
    {0, 1, TYPE2, _( "Endless Cord of Wood" ), _( "The %{name} provides %{count} unit of wood per day." )},
    {0, 1, TYPE2, _( "Endless Cart of Ore" ), _( "The %{name} provides %{count} unit of ore per day." )},
    {0, 1, TYPE2, _( "Endless Pouch of Crystal" ), _( "The %{name} provides %{count} unit of crystal/day." )},
    {0, 1, TYPE3, _( "Spiked Helm" ), _( "The %{name} increases your attack and defense skills by %{count} each." )},
    {0, 2, TYPE3, _( "Spiked Shield" ), _( "The %{name} increases your attack and defense skills by %{count} each." )},
    {0, 1, TYPE3, _( "White Pearl" ), _( "The %{name} increases your spell power and knowledge by %{count} each." )},
    {0, 2, TYPE3, _( "Black Pearl" ), _( "The %{name} increases your spell power and knowledge by %{count} each." )},

    {0, 0, TYPE0, _( "Magic Book" ), _( "The %{name} enables you to cast spells." )},

    {0, 0, TYPE0, "Dummy 1", "The reserved artifact."},
    {0, 0, TYPE0, "Dummy 2", "The reserved artifact."},
    {0, 0, TYPE0, "Dummy 3", "The reserved artifact."},
    {0, 0, TYPE0, "Dummy 4", "The reserved artifact."},

    {0, 0, TYPE0, _( "Spell Scroll" ), _( "This %{name} gives your hero the ability to cast the %{spell} spell." )},
    {0, 3, TYPE3, _( "Arm of the Martyr" ), _( "The %{name} increases your spell power by %{count} but adds the undead morale penalty." )},
    {0, 5, TYPE3, _( "Breastplate of Anduran" ), _( "The %{name} increases your defense by %{count}." )},
    {0, 50, TYPE3, _( "Broach of Shielding" ),
     _( "The %{name} provides %{count} percent protection from Armageddon and Elemental Storm, but decreases spell power by 2." )},
    {0, 5, TYPE0, _( "Battle Garb of Anduran" ),
     _( "The %{name} combines the powers of the three Anduran artifacts.  It provides maximum luck and morale for your troops and gives you the Town Portal spell." )},
    {0, 0, TYPE0, _( "Crystal Ball" ), _( "The %{name} lets you get more specific information about monsters, enemy heroes, and castles nearby the hero who holds it." )},
    {0, 50, TYPE0, _( "Heart of Fire" ), _( "The %{name} provides %{count} percent protection from fire, but doubles the damage taken from cold." )},
    {0, 50, TYPE0, _( "Heart of Ice" ), _( "The %{name} provides %{count} percent protection from cold, but doubles the damage taken from fire." )},
    {0, 5, TYPE3, _( "Helmet of Anduran" ), _( "The %{name} increases your spell power by %{count}." )},
    {0, 5, TYPE3, _( "Holy Hammer" ), _( "The %{name} increases your attack skill by %{count}." )},
    {0, 2, TYPE3, _( "Legendary Scepter" ), _( "The %{name} adds %{count} points to all attributes." )},
    {0, 1, TYPE1, _( "Masthead" ), _( "The %{name} boosts your luck and morale by %{count} each in sea combat." )},
    {0, 0, TYPE0, _( "Sphere of Negation" ), _( "The %{name} disables all spell casting, for both sides, in combat." )},
    {0, 5, TYPE3, _( "Staff of Wizardry" ), _( "The %{name} boosts your spell power by %{count}." )},
    {0, 4, TYPE3, _( "Sword Breaker" ), _( "The %{name} increases your defense by %{count} and attack by 1." )},
    {0, 5, TYPE3, _( "Sword of Anduran" ), _( "The %{name} increases your attack skill by %{count}." )},
    {0, 0, TYPE4, _( "Spade of Necromancy" ), _( "The %{name} gives you increased necromancy skill." )},

    {0, 0, TYPE0, "Unknown", "Unknown"},
};

const char * GetPluralDescription( const Artifact & art, u32 count )
{
    switch ( art() ) {
    case Artifact::ENCHANTED_HOURGLASS:
        return _n( "The %{name} extends the duration of all your spells by %{count} turn.", "The %{name} extends the duration of all your spells by %{count} turns.",
                   count );
    case Artifact::WIZARD_HAT:
        return _n( "The %{name} increases the duration of your spells by %{count} turn.", "The %{name} increases the duration of your spells by %{count} turns.", count );
    case Artifact::POWER_RING:
        return _n( "The %{name} returns %{count} extra power point/turn to your hero.", "The %{name} returns %{count} extra power points/turn to your hero.", count );
    case Artifact::ENDLESS_POUCH_SULFUR:
        return _n( "The %{name} provides %{count} unit of sulfur per day.", "The %{name} provides %{count} units of sulfur per day.", count );
    case Artifact::ENDLESS_VIAL_MERCURY:
        return _n( "The %{name} provides %{count} unit of mercury per day.", "The %{name} provides %{count} units of mercury per day.", count );
    case Artifact::ENDLESS_POUCH_GEMS:
        return _n( "The %{name} provides %{count} unit of gems per day.", "The %{name} provides %{count} units of gems per day.", count );
    case Artifact::ENDLESS_CORD_WOOD:
        return _n( "The %{name} provides %{count} unit of wood per day.", "The %{name} provides %{count} units of wood per day.", count );
    case Artifact::ENDLESS_CART_ORE:
        return _n( "The %{name} provides %{count} unit of ore per day.", "The %{name} provides %{count} units of ore per day.", count );
    case Artifact::ENDLESS_POUCH_CRYSTAL:
        return _n( "The %{name} provides %{count} unit of crystal per day.", "The %{name} provides %{count} units of crystal per day.", count );
    default:
        break;
    }
    return _( artifacts[art()].description );
}

bool SkipExtra( int art )
{
    switch ( art ) {
    case Artifact::BALLISTA:
    case Artifact::NOMAD_BOOTS_MOBILITY:
    case Artifact::TRAVELER_BOOTS_MOBILITY:
    case Artifact::RABBIT_FOOT:
    case Artifact::GOLDEN_HORSESHOE:
    case Artifact::GAMBLER_LUCKY_COIN:
    case Artifact::FOUR_LEAF_CLOVER:
    case Artifact::TRUE_COMPASS_MOBILITY:
    case Artifact::SAILORS_ASTROLABE_MOBILITY:
    case Artifact::EVIL_EYE:
    case Artifact::GOLD_WATCH:
    case Artifact::SKULLCAP:
    case Artifact::ICE_CLOAK:
    case Artifact::FIRE_CLOAK:
    case Artifact::LIGHTNING_HELM:
    case Artifact::GOLDEN_BOW:
    case Artifact::TELESCOPE:

        return true;

    default:
        break;
    }

    return false;
}

void Artifact::UpdateStats( const std::string & spec )
{
#ifdef WITH_XML
    // parse artifacts.xml
    TiXmlDocument doc;
    const TiXmlElement * xml_artifacts = NULL;

    if ( doc.LoadFile( spec.c_str() ) && NULL != ( xml_artifacts = doc.FirstChildElement( "artifacts" ) ) ) {
        size_t index = 0;
        const TiXmlElement * xml_artifact = xml_artifacts->FirstChildElement( "artifact" );
        for ( ; xml_artifact && index < UNKNOWN; xml_artifact = xml_artifact->NextSiblingElement( "artifact" ), ++index ) {
            int value;
            artifactstats_t * ptr = &artifacts[index];

            xml_artifact->Attribute( "disable", &value );
            if ( value )
                ptr->bits |= ART_DISABLED;

            xml_artifact->Attribute( "extra", &value );
            if ( value && !SkipExtra( index ) )
                ptr->extra = value;

            Artifact art( index );
        }
    }
    else
        VERBOSE( spec << ": " << doc.ErrorDesc() );
#else
    (void)spec;
#endif
}

Artifact::Artifact( int art )
    : id( art < UNKNOWN ? art : UNKNOWN )
    , ext( 0 )
{}

bool Artifact::operator==( const Spell & spell ) const
{
    switch ( id ) {
    case SPELL_SCROLL:
        return ext == spell();

    case Artifact::CRYSTAL_BALL:
        return spell == Spell::IDENTIFYHERO || spell == Spell::VISIONS;

    case Artifact::BATTLE_GARB:
        return spell == Spell::TOWNPORTAL;

    default:
        break;
    }

    return false;
}

bool Artifact::operator==( const Artifact & art ) const
{
    return id == art.id;
}

bool Artifact::operator!=( const Artifact & art ) const
{
    return id != art.id;
}

int Artifact::operator()( void ) const
{
    return id;
}

int Artifact::GetID( void ) const
{
    return id;
}

const char * Artifact::GetName( void ) const
{
    return _( artifacts[id].name );
}

int Artifact::Type( void ) const
{
    return artifacts[id].type;
}

std::string Artifact::GetDescription( void ) const
{
    u32 count = ExtraValue();
    std::string str = GetPluralDescription( *this, count );

    StringReplace( str, "%{name}", GetName() );

    if ( id == Artifact::SPELL_SCROLL )
        StringReplace( str, "%{spell}", Spell( ext ).GetName() );
    else
        StringReplace( str, "%{count}", count );

    return str;
}

u32 Artifact::ExtraValue( void ) const
{
    switch ( id ) {
    case GOLDEN_GOOSE:
        return 1000 * artifacts[id].extra;

    case ENDLESS_SACK_GOLD:
    case ENDLESS_BAG_GOLD:
    case ENDLESS_PURSE_GOLD:
    case TAX_LIEN:
        return 10 * artifacts[id].extra;

    default:
        break;
    }

    return artifacts[id].extra;
}

bool Artifact::isAlchemistRemove( void ) const
{
    switch ( id ) {
    case TAX_LIEN:
    case FIZBIN_MISFORTUNE:
    case HIDEOUS_MASK:
    case ARM_MARTYR:
    case HEART_FIRE:
    case HEART_ICE:
    case BROACH_SHIELDING:
    case SPHERE_NEGATION:
        return true;
    }

    return false;
}

bool Artifact::isUltimate( void ) const
{
    switch ( id ) {
    case ULTIMATE_BOOK:
    case ULTIMATE_SWORD:
    case ULTIMATE_CLOAK:
    case ULTIMATE_WAND:
    case ULTIMATE_SHIELD:
    case ULTIMATE_STAFF:
    case ULTIMATE_CROWN:
    case GOLDEN_GOOSE:
        return true;
    default:
        break;
    }

    return false;
}

bool Artifact::isValid( void ) const
{
    return id != UNKNOWN;
}

int Artifact::LoyaltyLevel( void ) const
{
    switch ( id ) {
    case MASTHEAD:
    case SPADE_NECROMANCY:
    case HEART_FIRE:
    case HEART_ICE:
        return ART_LEVEL2;

    case ARM_MARTYR:
    case HOLY_HAMMER:
    case LEGENDARY_SCEPTER:
    case STAFF_WIZARDRY:
    case SWORD_BREAKER:
    case CRYSTAL_BALL:
        return ART_LEVEL3;

    case SPELL_SCROLL:
    case BROACH_SHIELDING:
    case SWORD_ANDURAN:
    case BREASTPLATE_ANDURAN:
    case BATTLE_GARB:
    case HELMET_ANDURAN:
    case SPHERE_NEGATION:
        return ART_NORANDOM;

    default:
        break;
    }

    return ART_NONE;
}

int Artifact::Level( void ) const
{
    switch ( id ) {
    case MEDAL_VALOR:
    case MEDAL_COURAGE:
    case MEDAL_HONOR:
    case MEDAL_DISTINCTION:
    case THUNDER_MACE:
    case ARMORED_GAUNTLETS:
    case DEFENDER_HELM:
    case GIANT_FLAIL:
    case RABBIT_FOOT:
    case GOLDEN_HORSESHOE:
    case GAMBLER_LUCKY_COIN:
    case FOUR_LEAF_CLOVER:
    case ENCHANTED_HOURGLASS:
    case ICE_CLOAK:
    case FIRE_CLOAK:
    case LIGHTNING_HELM:
    case SNAKE_RING:
    case HOLY_PENDANT:
    case PENDANT_FREE_WILL:
    case PENDANT_LIFE:
    case PENDANT_DEATH:
    case GOLDEN_BOW:
    case TELESCOPE:
    case SERENITY_PENDANT:
    case STATESMAN_QUILL:
    case KINETIC_PENDANT:
    case SEEING_EYE_PENDANT:
        return ART_LEVEL1;

    case CASTER_BRACELET:
    case MAGE_RING:
    case STEALTH_SHIELD:
    case POWER_AXE:
    case MINOR_SCROLL:
    case ENDLESS_PURSE_GOLD:
    case SAILORS_ASTROLABE_MOBILITY:
    case ENDLESS_CORD_WOOD:
    case ENDLESS_CART_ORE:
    case SPIKED_HELM:
    case WHITE_PEARL:
    case EVIL_EYE:
    case GOLD_WATCH:
    case ANKH:
    case BOOK_ELEMENTS:
    case ELEMENTAL_RING:
    case SKULLCAP:
    case EVERCOLD_ICICLE:
    case POWER_RING:
    case AMMO_CART:
    case EVERHOT_LAVA_ROCK:
        return ART_LEVEL2;

    case ARCANE_NECKLACE:
    case WITCHES_BROACH:
    case BALLISTA:
    case DRAGON_SWORD:
    case DIVINE_BREASTPLATE:
    case MAJOR_SCROLL:
    case SUPERIOR_SCROLL:
    case FOREMOST_SCROLL:
    case ENDLESS_SACK_GOLD:
    case ENDLESS_BAG_GOLD:
    case NOMAD_BOOTS_MOBILITY:
    case TRAVELER_BOOTS_MOBILITY:
    case TRUE_COMPASS_MOBILITY:
    case ENDLESS_POUCH_SULFUR:
    case ENDLESS_POUCH_GEMS:
    case ENDLESS_POUCH_CRYSTAL:
    case ENDLESS_VIAL_MERCURY:
    case SPIKED_SHIELD:
    case BLACK_PEARL:
    case LIGHTNING_ROD:
    case WAND_NEGATION:
    case WIZARD_HAT:
        return ART_LEVEL3;

    case ULTIMATE_BOOK:
    case ULTIMATE_SWORD:
    case ULTIMATE_CLOAK:
    case ULTIMATE_WAND:
    case ULTIMATE_SHIELD:
    case ULTIMATE_STAFF:
    case ULTIMATE_CROWN:
    case GOLDEN_GOOSE:
        return ART_ULTIMATE;

    // no random
    case MAGIC_BOOK:
    case FIZBIN_MISFORTUNE:
    case TAX_LIEN:
    case HIDEOUS_MASK:
        return ART_NORANDOM;

    // price loyalty
    case SPELL_SCROLL:
    case ARM_MARTYR:
    case BREASTPLATE_ANDURAN:
    case BROACH_SHIELDING:
    case BATTLE_GARB:
    case CRYSTAL_BALL:
    case HELMET_ANDURAN:
    case HOLY_HAMMER:
    case LEGENDARY_SCEPTER:
    case MASTHEAD:
    case SPHERE_NEGATION:
    case STAFF_WIZARDRY:
    case SWORD_BREAKER:
    case SWORD_ANDURAN:
    case SPADE_NECROMANCY:
    case HEART_FIRE:
    case HEART_ICE:
        return Settings::Get().PriceLoyaltyVersion() ? ART_LOYALTY | LoyaltyLevel() : ART_LOYALTY;

    default:
        break;
    }

    return ART_NONE;
}

// Convert artifact flags into simple usable level value
int Artifact::getArtifactValue() const
{
    const int level = Level();

    if ( level & ART_LEVEL1 ) {
        return 1;
    }
    else if ( level & ART_LEVEL2 ) {
        return 2;
    }
    else if ( level & ART_LEVEL3 ) {
        return 3;
    }
    else if ( level & ART_ULTIMATE ) {
        return 5;
    }

    return 0;
}

/* return index sprite objnarti.icn */
u32 Artifact::IndexSprite( void ) const
{
    return id < UNKNOWN ? id * 2 + 1 : 0;
}

u32 Artifact::IndexSprite32( void ) const
{
    return id;
}

u32 Artifact::IndexSprite64( void ) const
{
    return id + 1;
}

int Artifact::GetSpell( void ) const
{
    return id == SPELL_SCROLL ? ext : Spell::NONE;
}

void Artifact::SetSpell( int v )
{
    bool adv = Rand::Get( 1 );

    switch ( v ) {
    case Spell::RANDOM:
        ext = Spell::Rand( Rand::Get( 1, 5 ), adv ).GetID();
        break;
    case Spell::RANDOM1:
        ext = Spell::Rand( 1, adv ).GetID();
        break;
    case Spell::RANDOM2:
        ext = Spell::Rand( 2, adv ).GetID();
        break;
    case Spell::RANDOM3:
        ext = Spell::Rand( 3, adv ).GetID();
        break;
    case Spell::RANDOM4:
        ext = Spell::Rand( 4, adv ).GetID();
        break;
    case Spell::RANDOM5:
        ext = Spell::Rand( 5, adv ).GetID();
        break;
    default:
        ext = v;
        break;
    }
}

void Artifact::Reset( void )
{
    id = UNKNOWN;
    ext = 0;
}

/* get rand all artifact */
int Artifact::Rand( level_t lvl )
{
    std::vector<int> v;
    v.reserve( 25 );

    // if possibly: make unique on map
    for ( u32 art = ULTIMATE_BOOK; art < UNKNOWN; ++art )
        if ( ( lvl & Artifact( art ).Level() ) && !( artifacts[art].bits & ART_DISABLED ) && !( artifacts[art].bits & ART_RNDUSED ) )
            v.push_back( art );

    //
    if ( v.empty() ) {
        for ( u32 art = ULTIMATE_BOOK; art < UNKNOWN; ++art )
            if ( ( lvl & Artifact( art ).Level() ) && !( artifacts[art].bits & ART_DISABLED ) )
                v.push_back( art );
    }

    int res = v.size() ? *Rand::Get( v ) : Artifact::UNKNOWN;
    artifacts[res].bits |= ART_RNDUSED;

    return res;
}

Artifact Artifact::FromMP2IndexSprite( u32 index )
{
    if ( 0xA2 > index )
        return Artifact( ( index - 1 ) / 2 );
    else if ( Settings::Get().PriceLoyaltyVersion() && 0xAB < index && 0xCE > index )
        return Artifact( ( index - 1 ) / 2 );
    else if ( 0xA3 == index )
        return Artifact( Rand( ART_LEVEL123 ) );
    else if ( 0xA4 == index )
        return Artifact( Rand( ART_ULTIMATE ) );
    else if ( 0xA7 == index )
        return Artifact( Rand( ART_LEVEL1 ) );
    else if ( 0xA9 == index )
        return Artifact( Rand( ART_LEVEL2 ) );
    else if ( 0xAB == index )
        return Rand( ART_LEVEL3 );

    DEBUG( DBG_GAME, DBG_WARN, "unknown index: " << static_cast<int>( index ) );

    return Artifact( UNKNOWN );
}

const char * Artifact::GetScenario( const Artifact & art )
{
    switch ( art() ) {
    case SPELL_SCROLL:
        return _(
            "You find an elaborate aontainer which housesan old vellum scroll. The runes on the container are very old, and the artistry with whitch it was put together is stunning. As you pull the scroll out, you feel imbued with magical power." );
    case ARM_MARTYR:
        return _(
            "One of the less intelligent members of your party picks up an arm off of the ground. Despite its missing a body, it is still moving. Your troops find the dismembered arm repulsive, but you cannot bring yourself to drop it: it seems to hold some sort of magical power that influences your decision making." );
    case BREASTPLATE_ANDURAN:
        return _(
            "You come upon a sign. It reads: \"Here lies the body of Anduran. Bow and swear fealty, and you shall be rewarded.\" You decide to do as it says. As you stand up, you feel a coldness against your skin. Looking down, you find that you are suddenly wearing a gleaming, ornate breastplate." );
    case BROACH_SHIELDING:
        return _(
            "A kindly Sorceress thinks that your army's defenses could use a magical boost. She offers to enchant the Broach that you wear on your cloak, and you accept." );
    case BATTLE_GARB:
        return _(
            "Out of pity for a poor peasant, you purchase a chest of old junk they are hawking for too much gold. Later, as you search through it, you find it contains the 3 pieces of the legendary battle garb of Anduran!" );
    case CRYSTAL_BALL:
        return _(
            "You come upon a caravan of gypsies who are feasting and fortifying their bodies with mead. They call you forward and say \"If you prove that you can dance the Rama-Buta, we will reward you.\" You don't know it, but try anyway. They laugh hysterically, but admire your bravery, giving you a Crystal Ball." );
    case HEART_FIRE:
        return _(
            "You enter a recently burned glade and come upon a Fire Elemental sitting atop a rock. It looks up, its flaming face contorted in a look of severe pain. It then tosses a glowing object at you. You put up your hands to block it, but it passes right through them and sears itself into your chest." );
    case HEART_ICE:
        return _(
            "Suddenly, a biting coldness engulfs your body. You seize up, falling from your horse. The pain subsides, but you still feel as if your chest is frozen.  As you pick yourself up off of the ground, you hear hearty laughter. You turn around just in time to see a Frost Giant run off into the woods and disappear." );
    case HELMET_ANDURAN:
        return _(
            "You spy a gleaming object poking up out of the ground. You send a member of your party over to investigate. He comes back with a golden helmet in his hands. You realize that it must be the helmet of the legendary Anduran, the only man who was known to wear solid gold armor." );
    case HOLY_HAMMER:
        return _(
            "You come upon a battle where a Paladin has been mortally wounded by a group of Zombies. He asks you to take his hammer and finish what he started.  As you pick it up, it begins to hum, and then everything becomes a blur. The Zombies lie dead, the hammer dripping with blood. You strap it to your belt." );
    case LEGENDARY_SCEPTER:
        return _(
            "Upon cresting a small hill, you come upon a ridiculous looking sight. A Sprite is attempting to carry a Scepter that is almost as big as it is. Trying not to laugh, you ask, \"Need help?\" The Sprite glares at you and answers: \"You think this is funny? Fine. You can carry it. I much prefer flying anyway.\"" );
    case MASTHEAD:
        return _(
            "An old seaman tells you a tale of an enchanted masthead that he used in his youth to rally his crew during times of trouble. He then hands you a faded map that shows where he hid it. After much exploring, you find it stashed underneath a nearby dock." );
    case SPHERE_NEGATION:
        return _(
            "You stop to help a Peasant catch a runaway mare. To show his gratitude, he hands you a tiny sphere. As soon as you grasp it, you feel the magical energy drain from your limbs..." );
    case STAFF_WIZARDRY:
        return _(
            "While out scaring up game, your troops find a mysterious staff levitating about three feet off of the ground. They hand it to you, and you notice an inscription. It reads: \"Brains best brawn and magic beats might. Heed my words, and you'll win every fight.\"" );
    case SWORD_BREAKER:
        return _( "A former Captain of the Guard admires your quest and gives you the enchanted Sword Breaker that he relied on during his tour of duty." );
    case SWORD_ANDURAN:
        return _(
            "A Troll stops you and says: \"Pay me 5,000 gold, or the Sword of Anduran will slay you where you stand.\" You refuse. The troll grabs the sword hanging from its belt, screams in pain, and runs away. Picking up the fabled sword, you give thanks that half-witted Trolls tend to grab the wrong end of sharp objects." );
    case SPADE_NECROMANCY:
        return _(
            "A dirty shovel has been thrust into a dirt mound nearby. Upon investigation, you discover it to be the enchanted shovel of the Gravediggers, long thought lost by mortals." );

    default:
        break;
    }

    return NULL;
}

StreamBase & operator<<( StreamBase & msg, const Artifact & art )
{
    return msg << art.id << art.ext;
}

StreamBase & operator>>( StreamBase & msg, Artifact & art )
{
    return msg >> art.id >> art.ext;
}

BagArtifacts::BagArtifacts()
    : std::vector<Artifact>( HEROESMAXARTIFACT, Artifact::UNKNOWN )
{}

bool BagArtifacts::ContainSpell( const Spell & spell ) const
{
    return end() != std::find( begin(), end(), spell );
}

bool BagArtifacts::isPresentArtifact( const Artifact & art ) const
{
    return end() != std::find( begin(), end(), art );
}

bool BagArtifacts::PushArtifact( const Artifact & art )
{
    if ( art.isValid() ) {
        if ( art() == Artifact::MAGIC_BOOK && isPresentArtifact( art ) )
            return false;

        iterator it = std::find( begin(), end(), Artifact( Artifact::UNKNOWN ) );
        if ( it == end() )
            return false;

        *it = art;

        // book insert first
        if ( art() == Artifact::MAGIC_BOOK )
            std::swap( *it, front() );

        return true;
    }

    return false;
}

void BagArtifacts::RemoveArtifact( const Artifact & art )
{
    iterator it = std::find( begin(), end(), art );
    if ( it != end() )
        ( *it ).Reset();
}

bool BagArtifacts::isFull( void ) const
{
    return end() == std::find( begin(), end(), Artifact( Artifact::UNKNOWN ) );
}

bool BagArtifacts::MakeBattleGarb( void )
{
    iterator it1, it2, it3;
    it1 = std::find( begin(), end(), Artifact( Artifact::BREASTPLATE_ANDURAN ) );
    it2 = std::find( begin(), end(), Artifact( Artifact::HELMET_ANDURAN ) );
    it3 = std::find( begin(), end(), Artifact( Artifact::SWORD_ANDURAN ) );
    if ( it1 == end() || it2 == end() || it3 == end() )
        return false;

    *it1 = Artifact::UNKNOWN;
    *it2 = Artifact::UNKNOWN;
    *it3 = Artifact::UNKNOWN;

    PushArtifact( Artifact::BATTLE_GARB );

    return true;
}

u32 BagArtifacts::CountArtifacts( void ) const
{
    // no way that we have more than 4 billion artifacts so static_cast is totally valid
    return static_cast<uint32_t>( std::count_if( begin(), end(), []( const Artifact & art ) { return art.isValid(); } ) );
}

int BagArtifacts::getArtifactValue() const
{
    int result = 0;
    for ( const Artifact & art : *this ) {
        if ( art.isValid() )
            result += art.getArtifactValue();
    }

    return result;
}

void BagArtifacts::exchangeArtifacts( BagArtifacts & giftBag )
{
    std::vector<Artifact> combined;
    for ( auto it = begin(); it != end(); ++it ) {
        if ( it->isValid() && it->GetID() != Artifact::MAGIC_BOOK ) {
            combined.push_back( *it );
            it->Reset();
        }
    }

    for ( auto it = giftBag.begin(); it != giftBag.end(); ++it ) {
        if ( it->isValid() && it->GetID() != Artifact::MAGIC_BOOK ) {
            combined.push_back( *it );
            it->Reset();
        }
    }

    // better artifacts at the end
    std::sort( combined.begin(), combined.end(), []( const Artifact & left, const Artifact & right ) { return left.getArtifactValue() < right.getArtifactValue(); } );

    // reset and clear all current artifacts, put back the best
    while ( combined.size() && PushArtifact( combined.back() ) ) {
        combined.pop_back();
    }

    while ( combined.size() && giftBag.PushArtifact( combined.back() ) ) {
        combined.pop_back();
    }
}

bool BagArtifacts::ContainUltimateArtifact( void ) const
{
    return end() != std::find_if( begin(), end(), []( const Artifact & art ) { return art.isUltimate(); } );
}

void BagArtifacts::RemoveScroll( const Artifact & art )
{
    Spell spell( art.GetSpell() );
    if ( spell.isValid() ) {
        iterator it = std::find( begin(), end(), spell );
        if ( it != end() )
            ( *it ).Reset();
    }
}

std::string BagArtifacts::String( void ) const
{
    std::ostringstream os;

    for ( const_iterator it = begin(); it != end(); ++it )
        os << ( *it ).GetName() << ", ";

    return os.str();
}

u32 BagArtifacts::Count( const Artifact & art ) const
{
    return static_cast<uint32_t>( std::count( begin(), end(), art ) ); // no way that we have more than 4 billion artifacts
}

u32 GoldInsteadArtifact( int obj )
{
    switch ( obj ) {
    case MP2::OBJ_SKELETON:
        return 1000;
    case MP2::OBJ_SHIPWRECKSURVIROR:
        return 1000;
    case MP2::OBJ_WATERCHEST:
        return 1500;
    case MP2::OBJ_TREASURECHEST:
        return 1000;
    case MP2::OBJ_SHIPWRECK:
        return 5000;
    case MP2::OBJ_GRAVEYARD:
        return 2000;
    default:
        break;
    }
    return 0;
}

ArtifactsBar::ArtifactsBar( const Heroes * ptr, bool mini, bool ro, bool change /* false */ )
    : hero( ptr )
    , use_mini_sprite( mini )
    , read_only( ro )
    , can_change( change )
{
    if ( use_mini_sprite ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::HSICONS, 0 );
        const Rect rt( 26, 21, 32, 32 );

        backsf.resize( rt.w + 2, rt.h + 2 );
        backsf.reset();

        fheroes2::DrawBorder( backsf, fheroes2::GetColorId( 0xD0, 0xC0, 0x48 ) );
        fheroes2::Blit( sprite, rt.x, rt.y, backsf, 1, 1, rt.w, rt.h );

        SetItemSize( backsf.width(), backsf.height() );

        spcursor.resize( backsf.width(), backsf.height() );
        spcursor.reset();
        fheroes2::DrawBorder( spcursor, 214 );
    }
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::ARTIFACT, 0 );
        SetItemSize( sprite.width(), sprite.height() );
        spcursor = fheroes2::AGG::GetICN( ICN::NGEXTRA, 62 );
    }
}

void ArtifactsBar::ResetSelected( void )
{
    Cursor::Get().Hide();
    spcursor.hide();
    Interface::ItemsActionBar<Artifact>::ResetSelected();
}

void ArtifactsBar::Redraw( fheroes2::Image & dstsf )
{
    Cursor::Get().Hide();
    spcursor.hide();
    Interface::ItemsActionBar<Artifact>::Redraw( dstsf );
}

void ArtifactsBar::RedrawBackground( const Rect & pos, fheroes2::Image & dstsf )
{
    if ( use_mini_sprite )
        fheroes2::Blit( backsf, dstsf, pos.x, pos.y );
    else
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::ARTIFACT, 0 ), dstsf, pos.x, pos.y );
}

void ArtifactsBar::RedrawItem( Artifact & art, const Rect & pos, bool selected, fheroes2::Image & dstsf )
{
    if ( art.isValid() ) {
        Cursor::Get().Hide();

        if ( use_mini_sprite )
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::ARTFX, art.IndexSprite32() ), dstsf, pos.x + 1, pos.y + 1 );
        else
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::ARTIFACT, art.IndexSprite64() ), dstsf, pos.x, pos.y );

        if ( selected ) {
            if ( use_mini_sprite )
                spcursor.setPosition( pos.x, pos.y );
            else
                spcursor.setPosition( pos.x - 3, pos.y - 3 );

            spcursor.show();
        }
    }
}

bool ArtifactsBar::ActionBarLeftMouseSingleClick( Artifact & art )
{
    if ( isSelected() ) {
        if ( !read_only )
            std::swap( art, *GetSelectedItem() );
        return false;
    }
    else if ( art.isValid() ) {
        if ( !read_only ) {
            Cursor::Get().Hide();
            spcursor.hide();
        }
    }
    else {
        if ( can_change )
            art = Dialog::SelectArtifact();

        return false;
    }

    return true;
}

bool ArtifactsBar::ActionBarLeftMouseDoubleClick( Artifact & art )
{
    if ( art() == Artifact::MAGIC_BOOK ) {
        if ( can_change )
            const_cast<Heroes *>( hero )->EditSpellBook();
        else
            hero->OpenSpellBook( SpellBook::ALL, false );
    }
    else if ( art() == Artifact::SPELL_SCROLL && Settings::Get().ExtHeroAllowTranscribingScroll() && hero->CanTranscribeScroll( art ) ) {
        Spell spell = art.GetSpell();

        if ( !spell.isValid() ) {
            DEBUG( DBG_GAME, DBG_WARN, "invalid spell" );
        }
        else if ( hero->CanLearnSpell( spell ) ) {
            payment_t cost = spell.GetCost();
            u32 answer = 0;
            std::string text = _(
                "Do you want to use your knowledge of magical secrets to transcribe the %{spell} Scroll into your Magic Book?\nThe Spell Scroll will be consumed.\n Cost in spell point: %{sp}" );

            StringReplace( text, "%{spell}", spell.GetName() );
            StringReplace( text, "%{sp}", spell.SpellPoint() );

            if ( spell.MovePoint() ) {
                text.append( "\n" );
                text.append( "Move point: %{mp}" );
                StringReplace( text, "%{mp}", spell.MovePoint() );
            }

            const std::string title = _( "Transcribe Spell Scroll" );

            if ( cost.GetValidItemsCount() )
                answer = Dialog::ResourceInfo( title, text, cost, Dialog::YES | Dialog::NO );
            else
                answer = Dialog::Message( title, text, Font::BIG, Dialog::YES | Dialog::NO );

            if ( answer == Dialog::YES )
                const_cast<Heroes *>( hero )->TranscribeScroll( art );
        }
    }
    else if ( art.isValid() ) {
        Dialog::ArtifactInfo( art.GetName(), "", art );
    }

    ResetSelected();

    return true;
}

bool ArtifactsBar::ActionBarRightMouseHold( Artifact & art )
{
    ResetSelected();

    if ( art.isValid() ) {
        if ( can_change )
            art.Reset();
        else
            Dialog::ArtifactInfo( art.GetName(), "", art, 0 );
    }

    return true;
}

bool ArtifactsBar::ActionBarLeftMouseSingleClick( Artifact & art1, Artifact & art2 )
{
    if ( art1() != Artifact::MAGIC_BOOK && art2() != Artifact::MAGIC_BOOK ) {
        std::swap( art1, art2 );
        return false;
    }

    return true;
}

bool ArtifactsBar::ActionBarCursor( Artifact & art )
{
    if ( isSelected() ) {
        Artifact * art2 = GetSelectedItem();

        if ( &art == art2 ) {
            if ( art() == Artifact::MAGIC_BOOK )
                msg = _( "View Spells" );
            else if ( art() == Artifact::SPELL_SCROLL && Settings::Get().ExtHeroAllowTranscribingScroll() && hero->CanTranscribeScroll( art ) )
                msg = _( "Transcribe Spell Scroll" );
            else {
                msg = _( "View %{name} Info" );
                StringReplace( msg, "%{name}", art.GetName() );
            }
        }
        else if ( !art.isValid() ) {
            if ( !read_only ) {
                msg = _( "Move %{name}" );
                StringReplace( msg, "%{name}", art2->GetName() );
            }
        }
        else {
            msg = _( "Exchange %{name2} with %{name}" );
            StringReplace( msg, "%{name}", art.GetName() );
            StringReplace( msg, "%{name2}", art2->GetName() );
        }
    }
    else if ( art.isValid() ) {
        msg = _( "Select %{name}" );
        StringReplace( msg, "%{name}", art.GetName() );
    }

    return false;
}

bool ArtifactsBar::ActionBarCursor( Artifact & art1, Artifact & art2 /* selected */ )
{
    if ( art2() == Artifact::MAGIC_BOOK || art1() == Artifact::MAGIC_BOOK )
        msg = _( "Cannot move artifact" );
    else if ( art1.isValid() ) {
        msg = _( "Exchange %{name2} with %{name}" );
        StringReplace( msg, "%{name}", art1.GetName() );
        StringReplace( msg, "%{name2}", art2.GetName() );
    }
    else {
        msg = _( "Move %{name}" );
        StringReplace( msg, "%{name}", art2.GetName() );
    }

    return false;
}

bool ArtifactsBar::QueueEventProcessing( std::string * str )
{
    msg.clear();
    bool res = Interface::ItemsActionBar<Artifact>::QueueEventProcessing();
    if ( str )
        *str = msg;
    return res;
}

bool ArtifactsBar::QueueEventProcessing( ArtifactsBar & bar, std::string * str )
{
    msg.clear();
    bool res = Interface::ItemsActionBar<Artifact>::QueueEventProcessing( bar );
    if ( str )
        *str = msg;
    return res;
}
