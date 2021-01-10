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
#include <functional>

#include "agg.h"
#include "ai.h"
#include "artifact.h"
#include "castle.h"
#include "difficulty.h"
#include "error.h"
#include "game.h"
#include "game_over.h"
#include "game_static.h"
#include "heroes.h"
#include "kingdom.h"
#include "maps_actions.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "pairs.h"
#include "race.h"
#include "resource.h"
#include "settings.h"
#include "text.h"
#include "world.h"

namespace GameStatic
{
    extern u32 uniq;
}

#ifdef WITH_ZLIB
#include "zzlib.h"
std::vector<u8> DecodeBase64AndUncomress( const std::string & base64 )
{
    std::vector<u8> zdata = decodeBase64( base64 );
    StreamBuf sb( zdata );
    sb.skip( 4 ); // editor: version
    u32 realsz = sb.getLE32();
    sb.skip( 4 ); // qt uncompress size
    return zlibDecompress( sb.data(), sb.size(), realsz + 1 );
}
#endif

#ifdef WITH_XML
namespace Maps
{
    TiXmlElement & operator>>( TiXmlElement & doc, Addons & levels )
    {
        TiXmlElement * xml_sprite = doc.FirstChildElement( "sprite" );
        for ( ; xml_sprite; xml_sprite = xml_sprite->NextSiblingElement( "sprite" ) ) {
            int uid, ext, index, level, icn;

            xml_sprite->Attribute( "uid", &uid );
            xml_sprite->Attribute( "ext", &ext );
            xml_sprite->Attribute( "index", &index );
            xml_sprite->Attribute( "level", &level );
            xml_sprite->Attribute( "icn", &icn );

            levels.push_back( TilesAddon( level, uid, icn, index ) );
        }

        return doc;
    }

    TiXmlElement & operator>>( TiXmlElement & doc, Tiles & tile )
    {
        int sprite, shape, object, base, local;

        doc.Attribute( "tileSprite", &sprite );
        doc.Attribute( "tileShape", &shape );
        doc.Attribute( "objectID", &object );
        doc.Attribute( "base", &base );
        doc.Attribute( "local", &local );

        tile.SetTile( sprite, shape );
        tile.SetObject( object );

        TiXmlElement * xml_levels1 = doc.FirstChildElement( "levels1" );
        if ( xml_levels1 )
            *xml_levels1 >> tile.addons_level1;

        TiXmlElement * xml_levels2 = doc.FirstChildElement( "levels2" );
        if ( xml_levels2 )
            *xml_levels2 >> tile.addons_level2;

        return doc;
    }
}

TiXmlElement & operator>>( TiXmlElement & doc, MapsTiles & /*tiles*/ )
{
    TiXmlElement * xml_tile = doc.FirstChildElement( "tile" );
    for ( ; xml_tile; xml_tile = xml_tile->NextSiblingElement( "tile" ) ) {
        int posx, posy;
        xml_tile->Attribute( "posx", &posx );
        xml_tile->Attribute( "posy", &posy );

        Maps::Tiles & tile = world.GetTiles( posx, posy );
        tile.SetIndex( Maps::GetIndexFromAbsPoint( posx, posy ) );
        *xml_tile >> tile;
    }

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, Army & army )
{
    army.Clean();
    int position = 0;

    TiXmlElement * xml_troop = doc.FirstChildElement( "troop" );
    for ( ; xml_troop; xml_troop = xml_troop->NextSiblingElement( "troop" ) ) {
        int type, count;
        xml_troop->Attribute( "type", &type );
        xml_troop->Attribute( "count", &count );
        Troop * troop = army.GetTroop( position++ );
        if ( troop )
            troop->Set( type, count );
    }

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, Castle & town )
{
    int posx, posy, race, color, build, dwell, custom1, custom2, custom3, forcetown, iscastle, captain;
    doc.Attribute( "posx", &posx );
    doc.Attribute( "posy", &posy );
    doc.Attribute( "race", &race );
    doc.Attribute( "color", &color );
    doc.Attribute( "buildings", &build );
    doc.Attribute( "dwellings", &dwell );
    doc.Attribute( "customTroops", &custom1 );
    doc.Attribute( "customDwellings", &custom2 );
    doc.Attribute( "customBuildings", &custom3 );
    doc.Attribute( "forceTown", &forcetown );
    doc.Attribute( "isCastle", &iscastle );
    doc.Attribute( "captainPresent", &captain );

    town.SetCenter( Point( posx, posy ) );
    town.SetColor( color );

    town.building = 0;
    if ( custom3 )
        town.building |= build;
    if ( custom2 )
        town.building |= dwell;
    town.name = doc.Attribute( "name" );

    if ( 1 != forcetown )
        town.SetModes( Castle::ALLOWCASTLE );

    town.building |= 1 == iscastle ? BUILD_CASTLE : BUILD_TENT;

    if ( 1 == captain )
        town.building |= BUILD_CAPTAIN;

    // default building
    if ( 1 != custom3 && 1 != custom2 ) {
        town.building |= DWELLING_MONSTER1;
        u32 dwelling2 = 0;
        switch ( Settings::Get().GameDifficulty() ) {
        case Difficulty::EASY:
            dwelling2 = 80;
            break;
        case Difficulty::NORMAL:
            dwelling2 = 65;
            break;
        case Difficulty::HARD:
            dwelling2 = 15;
            break;
        default:
            break;
        }
        if ( dwelling2 && dwelling2 >= Rand::Get( 1, 100 ) )
            town.building |= DWELLING_MONSTER2;
    }

    if ( race == Race::RAND ) {
        u32 kingdom_race = Players::GetPlayerRace( town.GetColor() );
        race = Color::NONE != town.GetColor() && ( Race::ALL & kingdom_race ) ? kingdom_race : Race::Rand();

        Maps::UpdateCastleSprite( town.GetCenter(), race, town.isCastle(), true );
    }
    town.race = race;

    Maps::MinimizeAreaForCastle( town.GetCenter() );
    world.CaptureObject( town.GetIndex(), town.GetColor() );

    TiXmlElement * xml_troops = doc.FirstChildElement( "troops" );
    if ( xml_troops )
        *xml_troops >> town.army;

    town.PostLoad();

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, AllCastles & castles )
{
    TiXmlElement * xml_town = doc.FirstChildElement( "town" );
    for ( ; xml_town; xml_town = xml_town->NextSiblingElement( "town" ) ) {
        Castle * town = new Castle();
        *xml_town >> *town;
        castles.push_back( town );
    }

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, Skill::SecSkills & skills )
{
    TiXmlElement * xml_skill = doc.FirstChildElement( "skill" );
    for ( ; xml_skill; xml_skill = xml_skill->NextSiblingElement( "skill" ) ) {
        int skill, level;
        xml_skill->Attribute( "id", &skill );
        xml_skill->Attribute( "level", &level );

        skills.AddSkill( Skill::Secondary( skill, level ) );
    }

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, BagArtifacts & bag )
{
    TiXmlElement * xml_art = doc.FirstChildElement( "artifact" );
    for ( ; xml_art; xml_art = xml_art->NextSiblingElement( "artifact" ) ) {
        int art = 0;
        xml_art->Attribute( "id", &art );

        if ( art )
            bag.PushArtifact( art - 1 );
    }
    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, SpellBook & book )
{
    book.clear();

    TiXmlElement * xml_spell = doc.FirstChildElement( "spell" );
    for ( ; xml_spell; xml_spell = xml_spell->NextSiblingElement( "spell" ) ) {
        int spell = 0;
        xml_spell->Attribute( "id", &spell );

        if ( spell )
            book.Append( Spell( spell ) );
    }
    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, Heroes & hero )
{
    int posx, posy, color, portrait, exp, patrol, square, attack, defense, power, knowledge, race, jail, book;

    doc.Attribute( "posx", &posx );
    doc.Attribute( "posy", &posy );
    hero.SetCenter( Point( posx, posy ) );

    doc.Attribute( "color", &color );
    hero.SetColor( color );

    doc.Attribute( "attack", &attack );
    hero.attack = attack;

    doc.Attribute( "defense", &defense );
    hero.defense = defense;

    doc.Attribute( "power", &power );
    hero.power = power;

    doc.Attribute( "knowledge", &knowledge );
    hero.knowledge = knowledge;

    doc.Attribute( "portrait", &portrait );
    if ( portrait )
        hero.portrait = portrait - 1;

    doc.Attribute( "race", &race );
    if ( race & Race::ALL )
        hero.race = race;

    doc.Attribute( "experience", &exp );
    hero.experience = exp;

    doc.Attribute( "patrolMode", &patrol );
    if ( patrol ) {
        hero.SetModes( Heroes::PATROL );
        doc.Attribute( "patrolSquare", &square );
        hero.patrol_center = Point( posx, posy );
        hero.patrol_square = square;
    }

    doc.Attribute( "jailMode", &jail );
    if ( jail ) {
        hero.SetModes( Heroes::JAIL );
        hero.SetColor( Color::NONE );
    }

    hero.name = doc.Attribute( "name" );
    if ( hero.name == "Random" || hero.name == "Unknown" )
        hero.name = Heroes::GetName( hero.GetID() );

    // skills
    Skill::SecSkills skills;

    TiXmlElement * xml_skills = doc.FirstChildElement( "skills" );
    if ( xml_skills )
        *xml_skills >> skills;

    if ( skills.Count() ) {
        hero.SetModes( Heroes::CUSTOMSKILLS );
        hero.secondary_skills = skills;
    }

    // artifacts
    doc.Attribute( "haveBook", &book );
    if ( book )
        hero.bag_artifacts.PushArtifact( Artifact::MAGIC_BOOK );
    else
        hero.bag_artifacts.RemoveArtifact( Artifact::MAGIC_BOOK );

    TiXmlElement * xml_artifacts = doc.FirstChildElement( "artifacts" );
    if ( xml_artifacts )
        *xml_artifacts >> hero.bag_artifacts;

    // troops
    TiXmlElement * xml_troops = doc.FirstChildElement( "troops" );
    if ( xml_troops )
        *xml_troops >> hero.army;

    // spells
    if ( book ) {
        TiXmlElement * xml_spells = doc.FirstChildElement( "spells" );
        if ( xml_spells )
            *xml_spells >> hero.spell_book;
    }

    hero.PostLoad();

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, AllHeroes & /*heroes*/ )
{
    TiXmlElement * xml_hero = doc.FirstChildElement( "hero" );
    for ( ; xml_hero; xml_hero = xml_hero->NextSiblingElement( "hero" ) ) {
        int posx, posy, portrait, race;
        xml_hero->Attribute( "posx", &posx );
        xml_hero->Attribute( "posy", &posy );
        xml_hero->Attribute( "portrait", &portrait );
        xml_hero->Attribute( "race", &race );

        const Maps::Tiles & tile = world.GetTiles( posx, posy );
        const uint8_t object = tile.GetObject();
        bool jail = false;

        if ( object != MP2::OBJ_HEROES ) {
            jail = object == MP2::OBJ_JAIL;

            if ( !jail ) {
                VERBOSE( "xml error: heroes not found"
                         << ", "
                         << "posx: " << posx << ", "
                         << "posy: " << posy );
                continue;
            }
        }

        std::pair<int, int> colorRace( Color::NONE, race );
        Heroes * hero = NULL;

        if ( !jail ) {
            colorRace = Maps::Tiles::ColorRaceFromHeroSprite( tile.GetObjectSpriteIndex() );
            Kingdom & kingdom = world.GetKingdom( colorRace.first );

            if ( colorRace.second == Race::RAND && colorRace.first != Color::NONE )
                colorRace.second = kingdom.GetRace();

            // check heroes max count
            if ( !kingdom.AllowRecruitHero( false, 0 ) ) {
                DEBUG( DBG_GAME, DBG_WARN,
                       "kingdom recruil full, skip hero"
                           << ", "
                           << "posx: " << posx << ", "
                           << "posy: " << posy );
                continue;
            }
        }

        if ( 0 < portrait )
            hero = world.GetHeroes( portrait );

        if ( !hero || !hero->isFreeman() )
            hero = world.GetFreemanHeroes( colorRace.second );

        if ( hero ) {
            *xml_hero >> *hero;

            if ( jail )
                hero->SetModes( Heroes::JAIL );
        }
    }

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, Funds & funds )
{
    int ore, mercury, wood, sulfur, crystal, gems, gold;
    doc.Attribute( "ore", &ore );
    doc.Attribute( "mercury", &mercury );
    doc.Attribute( "wood", &wood );
    doc.Attribute( "sulfur", &sulfur );
    doc.Attribute( "crytal", &crystal );
    doc.Attribute( "gems", &gems );
    doc.Attribute( "gold", &gold );

    funds.ore = ore;
    funds.mercury = mercury;
    funds.wood = wood;
    funds.sulfur = crystal;
    funds.gems = gems;
    funds.gold = gold;

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, MapSphinx & riddle )
{
    int posx, posy, uid, artifact;

    doc.Attribute( "posx", &posx );
    doc.Attribute( "posy", &posy );
    doc.Attribute( "uid", &uid );
    doc.Attribute( "artifact", &artifact );

    riddle.SetCenter( Point( posx, posy ) );
    riddle.SetUID( uid );
    riddle.artifact = artifact ? artifact - 1 : Artifact::UNKNOWN;
    riddle.valid = true;

    TiXmlElement * xml_answers = doc.FirstChildElement( "answers" );
    if ( xml_answers ) {
        TiXmlElement * xml_answer = doc.FirstChildElement( "answer" );
        for ( ; xml_answer; xml_answer = xml_answer->NextSiblingElement( "answer" ) )
            if ( xml_answer->GetText() )
                riddle.answers.push_back( xml_answer->GetText() );
    }

    TiXmlElement * xml_resources = doc.FirstChildElement( "resources" );
    if ( xml_resources )
        *xml_resources >> riddle.resources;

    TiXmlElement * xml_msg = doc.FirstChildElement( "msg" );
    if ( xml_msg && xml_msg->GetText() )
        riddle.message = xml_msg->GetText();

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, MapEvent & event )
{
    int posx, posy, uid, colors, allow, cancel, artifact;

    doc.Attribute( "posx", &posx );
    doc.Attribute( "posy", &posy );
    doc.Attribute( "uid", &uid );
    doc.Attribute( "cancelAfterFirstVisit", &cancel );
    doc.Attribute( "colors", &colors );
    doc.Attribute( "allowComputer", &allow );
    doc.Attribute( "artifact", &artifact );

    event.SetCenter( Point( posx, posy ) );
    event.SetUID( uid );
    event.computer = allow;
    event.colors = colors;
    event.artifact = artifact ? artifact - 1 : Artifact::UNKNOWN;
    event.cancel = cancel;

    TiXmlElement * xml_resources = doc.FirstChildElement( "resources" );
    if ( xml_resources )
        *xml_resources >> event.resources;

    TiXmlElement * xml_msg = doc.FirstChildElement( "msg" );
    if ( xml_msg && xml_msg->GetText() )
        event.message = xml_msg->GetText();

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, EventDate & event )
{
    int day1, day2, colors, allow;

    doc.Attribute( "dayFirst", &day1 );
    doc.Attribute( "daySubsequent", &day2 );
    doc.Attribute( "colors", &colors );
    doc.Attribute( "allowComputer", &allow );

    event.computer = allow;
    event.first = day1;
    event.subsequent = day2;
    event.colors = colors;

    TiXmlElement * xml_resources = doc.FirstChildElement( "resources" );
    if ( xml_resources )
        *xml_resources >> event.resource;

    TiXmlElement * xml_msg = doc.FirstChildElement( "msg" );
    if ( xml_msg && xml_msg->GetText() )
        event.message = xml_msg->GetText();

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, EventsDate & events )
{
    TiXmlElement * xml_event = doc.FirstChildElement( "event" );
    for ( ; xml_event; xml_event = xml_event->NextSiblingElement( "event" ) ) {
        events.push_back( EventDate() );
        *xml_event >> events.back();
    }

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, Rumors & rumors )
{
    TiXmlElement * xml_msg = doc.FirstChildElement( "msg" );
    for ( ; xml_msg; xml_msg = xml_msg->NextSiblingElement( "msg" ) )
        if ( xml_msg->GetText() )
            rumors.push_back( xml_msg->GetText() );

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, MapSign & obj )
{
    int posx, posy, uid;
    doc.Attribute( "posx", &posx );
    doc.Attribute( "posy", &posy );
    doc.Attribute( "uid", &uid );

    obj.SetCenter( Point( posx, posy ) );
    obj.SetUID( uid );
    if ( doc.GetText() )
        obj.message = doc.GetText();

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, MapResource & obj )
{
    int posx, posy, uid, type, count;
    doc.Attribute( "posx", &posx );
    doc.Attribute( "posy", &posy );
    doc.Attribute( "uid", &uid );
    doc.Attribute( "type", &type );
    doc.Attribute( "count", &count );

    obj.SetCenter( Point( posx, posy ) );
    obj.SetUID( uid );
    obj.resource = ResourceCount( type, count );

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, MapMonster & obj )
{
    int posx, posy, uid, type, cond, count;
    doc.Attribute( "posx", &posx );
    doc.Attribute( "posy", &posy );
    doc.Attribute( "uid", &uid );
    doc.Attribute( "type", &type );
    doc.Attribute( "condition", &cond );
    doc.Attribute( "count", &count );

    obj.SetCenter( Point( posx, posy ) );
    obj.SetUID( uid );
    obj.monster = Monster( type );

    /* join condition: random: -1, fight: 0, all join: 1, for money: 2 */
    if ( cond < 0 ) {
        cond = 0;

        // 20% chance for join
        if ( 3 > Rand::Get( 1, 10 ) ) {
            cond = 4 > Rand::Get( 1, 10 ) ? 1 : 2;
        }
    }

    if ( obj.monster.GetID() == Monster::GHOST || obj.monster.isElemental() )
        cond = 0;

    if ( count == 0 ) {
        int mul = 4;

        // set random count
        switch ( Settings::Get().GameDifficulty() ) {
        case Difficulty::EASY:
            mul = 3;
            break;
        case Difficulty::NORMAL:
            mul = 4;
            break;
        case Difficulty::HARD:
            mul = 4;
            break;
        case Difficulty::EXPERT:
            mul = 5;
            break;
        case Difficulty::IMPOSSIBLE:
            mul = 6;
            break;
        default:
            break;
        }

        count = mul * obj.monster.GetRNDSize( true );
    }

    obj.condition = cond;
    obj.count = count;

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, MapArtifact & obj )
{
    int posx, posy, uid, type, cond;
    doc.Attribute( "posx", &posx );
    doc.Attribute( "posy", &posy );
    doc.Attribute( "uid", &uid );
    doc.Attribute( "type", &type );
    doc.Attribute( "condition", &cond );

    obj.SetCenter( Point( posx, posy ) );
    obj.SetUID( uid );
    obj.artifact = Artifact( type );

    if ( 0 > cond ) {
        // 0: 70% none
        // 1,2,3 - 2000g, 2500g+3res, 3000g+5res,
        // 4,5 - need have skill wisard or leadership,
        // 6 - 50 rogues, 7 - 1 gin, 8,9,10,11,12,13 - 1 monster level4,
        // 15 - spell
        cond = Rand::Get( 1, 10 ) < 4 ? Rand::Get( 1, 13 ) : 0;
    }

    obj.condition = cond;

    if ( cond == 2 || cond == 3 )
        obj.extended = Resource::Rand();

    if ( type == Artifact::SPELL_SCROLL ) {
        int spell;
        doc.Attribute( "spell", &spell );
        obj.artifact.SetSpell( spell );
    }

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, MapObjects & objects )
{
    TiXmlElement * xml_objects = doc.FirstChildElement();
    for ( ; xml_objects; xml_objects = xml_objects->NextSiblingElement() ) {
        const std::string name = StringLower( xml_objects->Value() );
        int posx, posy;
        xml_objects->Attribute( "posx", &posx );
        xml_objects->Attribute( "posy", &posy );

        if ( name == "sign" ) {
            MapSign * ptr = new MapSign();
            *xml_objects >> *ptr;
            objects.add( ptr );
        }
        else if ( name == "sphinx" ) {
            MapSphinx * ptr = new MapSphinx();
            *xml_objects >> *ptr;
            objects.add( ptr );
        }
        else if ( name == "event" ) {
            MapEvent * ptr = new MapEvent();
            *xml_objects >> *ptr;
            objects.add( ptr );
        }
        else if ( name == "monster" ) {
            MapMonster * ptr = new MapMonster();
            *xml_objects >> *ptr;
            objects.add( ptr );
        }
        else if ( name == "artifact" ) {
            MapArtifact * ptr = new MapArtifact();
            *xml_objects >> *ptr;
            objects.add( ptr );
        }
        else if ( name == "resource" ) {
            MapResource * ptr = new MapResource();
            *xml_objects >> *ptr;
            objects.add( ptr );
        }
    }

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, ActionDefault & st )
{
    int enabled;
    doc.Attribute( "enabled", &enabled );
    st.enabled = enabled;
    if ( doc.GetText() )
        st.message = doc.GetText();
    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, ActionAccess & st )
{
    int colors, comp, cancel;
    doc.Attribute( "allowPlayers", &colors );
    doc.Attribute( "allowComputer", &comp );
    doc.Attribute( "cancelAfterFirstVisit", &cancel );
    st.allowPlayers = colors;
    st.allowComputer = comp;
    st.cancelAfterFirstVisit = cancel;
    if ( doc.GetText() )
        st.message = doc.GetText();
    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, ActionMessage & st )
{
    if ( doc.GetText() )
        st.message = doc.GetText();
    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, ActionResources & st )
{
    doc >> st.resources;
    if ( doc.GetText() )
        st.message = doc.GetText();
    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, ActionArtifact & st )
{
    int artifact;
    doc.Attribute( "artifact", &artifact );
    st.artifact = artifact ? artifact - 1 : Artifact::UNKNOWN;
    if ( st.artifact() == Artifact::SPELL_SCROLL ) {
        int spell = 0;
        doc.Attribute( "spell", &spell );
        if ( 0 == spell )
            spell = Spell::RANDOM;
        st.artifact.SetSpell( spell );
    }
    if ( doc.GetText() )
        st.message = doc.GetText();
    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, MapActions & objects )
{
    TiXmlElement * xml_actions = doc.FirstChildElement( "actions" );
    for ( ; xml_actions; xml_actions = xml_actions->NextSiblingElement( "actions" ) ) {
        int posx, posy, uid;

        xml_actions->Attribute( "posx", &posx );
        xml_actions->Attribute( "posy", &posy );
        xml_actions->Attribute( "uid", &uid );

        s32 index = Maps::GetIndexFromAbsPoint( posx, posy );

        if ( Maps::isValidAbsIndex( index ) ) {
            ListActions & list = objects[index];
            list.clear();

            TiXmlElement * xml_action = xml_actions->FirstChildElement();
            for ( ; xml_action; xml_action = xml_action->NextSiblingElement() ) {
                const std::string name = StringLower( xml_action->Value() );

                if ( name == "defaultaction" ) {
                    ActionDefault * ptr = new ActionDefault();
                    *xml_action >> *ptr;
                    list.push_back( ptr );
                }
                else if ( name == "access" ) {
                    ActionAccess * ptr = new ActionAccess();
                    *xml_action >> *ptr;
                    list.push_back( ptr );
                }
                else if ( name == "message" ) {
                    ActionMessage * ptr = new ActionMessage();
                    *xml_action >> *ptr;
                    list.push_back( ptr );
                }
                else if ( name == "resources" ) {
                    ActionResources * ptr = new ActionResources();
                    *xml_action >> *ptr;
                    list.push_back( ptr );
                }
                else if ( name == "artifact" ) {
                    ActionArtifact * ptr = new ActionArtifact();
                    *xml_action >> *ptr;
                    list.push_back( ptr );
                }
            }
        }
    }

    return doc;
}

TiXmlElement & operator>>( TiXmlElement & doc, World & w )
{
    TiXmlElement * xml_tiles = doc.FirstChildElement( "tiles" );
    int value;

    if ( !xml_tiles )
        return doc;

    Size & sw = w;

    xml_tiles->Attribute( "width", &value );
    sw.w = value;

    xml_tiles->Attribute( "height", &value );
    sw.h = value;

    w.vec_tiles.resize( sw.w * sw.h );

    *xml_tiles >> w.vec_tiles;

    TiXmlElement * xml_objects = doc.FirstChildElement( "objects" );
    if ( xml_objects ) {
        xml_objects->Attribute( "lastUID", &value );
        GameStatic::uniq = value;

        *xml_objects >> w.vec_castles >> w.vec_heroes >> w.map_objects >> w.map_actions;
    }

    TiXmlElement * xml_events = doc.FirstChildElement( "events" );
    if ( xml_events )
        *xml_events >> w.vec_eventsday;

    TiXmlElement * xml_rumors = doc.FirstChildElement( "rumors" );
    if ( xml_rumors )
        *xml_rumors >> w.vec_rumors;
    w.ProcessNewMap();

    return doc;
}

/* load maps */
bool World::LoadMapMAP( const std::string & filename )
{
#ifdef WITH_ZLIB
    Reset();
    Defaults();

    TiXmlDocument doc;
    TiXmlElement * xml_map = NULL;

    if ( doc.LoadFile( filename.c_str() ) && NULL != ( xml_map = doc.FirstChildElement( "map" ) ) ) {
        TiXmlElement * xml_data = xml_map->FirstChildElement( "data" );
        if ( !xml_data->Attribute( "compress" ) ) {
            *xml_data >> *this;
            return true;
        }
        else if ( xml_data->GetText() ) {
            std::vector<u8> raw_data = DecodeBase64AndUncomress( xml_data->GetText() );
            raw_data.push_back( 0 );
            doc.Parse( reinterpret_cast<const char *>( &raw_data[0] ) );
            if ( doc.Error() ) {
                VERBOSE( "parse error: " << doc.ErrorDesc() );
                return false;
            }
            // SaveMemToFile(raw_data, "raw.data");
            xml_data = doc.FirstChildElement( "data" );
            *xml_data >> *this;
            return true;
        }
    }
#endif
    return false;
}
#else
bool World::LoadMapMAP( const std::string & )
{
    return false;
}
#endif // WITH_XML

bool World::LoadMapMP2( const std::string & filename )
{
    Reset();
    Defaults();

    StreamFile fs;
    if ( !fs.open( filename, "rb" ) ) {
        DEBUG( DBG_GAME | DBG_ENGINE, DBG_WARN, "file not found " << filename.c_str() );
        return false;
    }

    MapsIndexes vec_object; // index maps for OBJ_CASTLE, OBJ_HEROES, OBJ_SIGN, OBJ_BOTTLE, OBJ_EVENT
    vec_object.reserve( 100 );

    // check (mp2, mx2) ID
    if ( fs.getBE32() != 0x5C000000 )
        return false;

    // endof
    const u32 endof_mp2 = fs.size();
    fs.seek( endof_mp2 - 4 );

    // read uniq
    GameStatic::uniq = fs.getLE32();

    // offset data
    fs.seek( MP2OFFSETDATA - 2 * 4 );

    // width
    switch ( fs.getLE32() ) {
    case Maps::SMALL:
        Size::w = Maps::SMALL;
        break;
    case Maps::MEDIUM:
        Size::w = Maps::MEDIUM;
        break;
    case Maps::LARGE:
        Size::w = Maps::LARGE;
        break;
    case Maps::XLARGE:
        Size::w = Maps::XLARGE;
        break;
    default:
        Size::w = 0;
        break;
    }

    // height
    switch ( fs.getLE32() ) {
    case Maps::SMALL:
        Size::h = Maps::SMALL;
        break;
    case Maps::MEDIUM:
        Size::h = Maps::MEDIUM;
        break;
    case Maps::LARGE:
        Size::h = Maps::LARGE;
        break;
    case Maps::XLARGE:
        Size::h = Maps::XLARGE;
        break;
    default:
        Size::h = 0;
        break;
    }

    if ( Size::w == 0 || Size::h == 0 || Size::w != Size::h ) {
        DEBUG( DBG_GAME, DBG_WARN, "incrrect maps size" );
        return false;
    }

    // seek to ADDONS block
    fs.skip( w() * h() * SIZEOFMP2TILE );

    // read all addons
    std::vector<MP2::mp2addon_t> vec_mp2addons( fs.getLE32() /* count mp2addon_t */ );

    for ( std::vector<MP2::mp2addon_t>::iterator it = vec_mp2addons.begin(); it != vec_mp2addons.end(); ++it ) {
        MP2::mp2addon_t & mp2addon = *it;

        mp2addon.indexAddon = fs.getLE16();
        mp2addon.objectNameN1 = fs.get() * 2;
        mp2addon.indexNameN1 = fs.get();
        mp2addon.quantityN = fs.get();
        mp2addon.objectNameN2 = fs.get();
        mp2addon.indexNameN2 = fs.get();

        mp2addon.editorObjectLink = fs.getLE32();
        mp2addon.editorObjectOverlay = fs.getLE32();
    }

    const u32 endof_addons = fs.tell();
    DEBUG( DBG_GAME, DBG_INFO, "read all tiles addons, tellg: " << endof_addons );

    // offset data
    fs.seek( MP2OFFSETDATA );

    vec_tiles.resize( w() * h() );

    // read all tiles
    for ( MapsTiles::iterator it = vec_tiles.begin(); it != vec_tiles.end(); ++it ) {
        const size_t index = std::distance( vec_tiles.begin(), it );
        Maps::Tiles & tile = *it;

        MP2::mp2tile_t mp2tile;

        mp2tile.tileIndex = fs.getLE16();
        mp2tile.objectName1 = fs.get();
        mp2tile.indexName1 = fs.get();
        mp2tile.quantity1 = fs.get();
        mp2tile.quantity2 = fs.get();
        mp2tile.objectName2 = fs.get();
        mp2tile.indexName2 = fs.get();
        mp2tile.flags = fs.get();
        mp2tile.mapObject = fs.get();

        switch ( mp2tile.mapObject ) {
        case MP2::OBJ_RNDTOWN:
        case MP2::OBJ_RNDCASTLE:
        case MP2::OBJ_CASTLE:
        case MP2::OBJ_HEROES:
        case MP2::OBJ_SIGN:
        case MP2::OBJ_BOTTLE:
        case MP2::OBJ_EVENT:
        case MP2::OBJ_SPHINX:
        case MP2::OBJ_JAIL:
            vec_object.push_back( index );
            break;
        default:
            break;
        }

        // offset first addon
        size_t offsetAddonsBlock = fs.getLE16();

        mp2tile.editorObjectLink = fs.getLE32();
        mp2tile.editorObjectOverlay = fs.getLE32();

        tile.Init( index, mp2tile );

        // load all addon for current tils
        while ( offsetAddonsBlock ) {
            if ( vec_mp2addons.size() <= offsetAddonsBlock ) {
                DEBUG( DBG_GAME, DBG_WARN, "index out of range" );
                break;
            }
            tile.AddonsPushLevel1( vec_mp2addons[offsetAddonsBlock] );
            tile.AddonsPushLevel2( vec_mp2addons[offsetAddonsBlock] );
            offsetAddonsBlock = vec_mp2addons[offsetAddonsBlock].indexAddon;
        }

        tile.AddonsSort();
    }

    DEBUG( DBG_GAME, DBG_INFO, "read all tiles, tellg: " << fs.tell() );

    // after addons
    fs.seek( endof_addons );

    // cood castles
    // 72 x 3 byte (cx, cy, id)
    for ( u32 ii = 0; ii < 72; ++ii ) {
        u32 cx = fs.get();
        u32 cy = fs.get();
        u32 id = fs.get();

        // empty block
        if ( 0xFF == cx && 0xFF == cy )
            continue;

        switch ( id ) {
        case 0x00: // tower: knight
        case 0x80: // castle: knight
            vec_castles.push_back( new Castle( cx, cy, Race::KNGT ) );
            break;

        case 0x01: // tower: barbarian
        case 0x81: // castle: barbarian
            vec_castles.push_back( new Castle( cx, cy, Race::BARB ) );
            break;

        case 0x02: // tower: sorceress
        case 0x82: // castle: sorceress
            vec_castles.push_back( new Castle( cx, cy, Race::SORC ) );
            break;

        case 0x03: // tower: warlock
        case 0x83: // castle: warlock
            vec_castles.push_back( new Castle( cx, cy, Race::WRLK ) );
            break;

        case 0x04: // tower: wizard
        case 0x84: // castle: wizard
            vec_castles.push_back( new Castle( cx, cy, Race::WZRD ) );
            break;

        case 0x05: // tower: necromancer
        case 0x85: // castle: necromancer
            vec_castles.push_back( new Castle( cx, cy, Race::NECR ) );
            break;

        case 0x06: // tower: random
        case 0x86: // castle: random
            vec_castles.push_back( new Castle( cx, cy, Race::NONE ) );
            break;

        default:
            DEBUG( DBG_GAME, DBG_WARN,
                   "castle block: "
                       << "unknown id: " << id << ", maps index: " << cx + cy * w() );
            break;
        }
        // preload in to capture objects cache
        map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_CASTLE, Color::NONE );
    }

    DEBUG( DBG_GAME, DBG_INFO, "read coord castles, tellg: " << fs.tell() );
    fs.seek( endof_addons + ( 72 * 3 ) );

    // cood resource kingdoms
    // 144 x 3 byte (cx, cy, id)
    for ( u32 ii = 0; ii < 144; ++ii ) {
        u32 cx = fs.get();
        u32 cy = fs.get();
        u32 id = fs.get();

        // empty block
        if ( 0xFF == cx && 0xFF == cy )
            continue;

        switch ( id ) {
        // mines: wood
        case 0x00:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_SAWMILL, Color::NONE );
            break;
        // mines: mercury
        case 0x01:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_ALCHEMYLAB, Color::NONE );
            break;
        // mines: ore
        case 0x02:
        // mines: sulfur
        case 0x03:
        // mines: crystal
        case 0x04:
        // mines: gems
        case 0x05:
        // mines: gold
        case 0x06:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_MINES, Color::NONE );
            break;
        // lighthouse
        case 0x64:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_LIGHTHOUSE, Color::NONE );
            break;
        // dragon city
        case 0x65:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_DRAGONCITY, Color::NONE );
            break;
        // abandoned mines
        case 0x67:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_ABANDONEDMINE, Color::NONE );
            break;
        default:
            DEBUG( DBG_GAME, DBG_WARN,
                   "kingdom block: "
                       << "unknown id: " << id << ", maps index: " << cx + cy * w() );
            break;
        }
    }

    DEBUG( DBG_GAME, DBG_INFO, "read coord other resource, tellg: " << fs.tell() );
    fs.seek( endof_addons + ( 72 * 3 ) + ( 144 * 3 ) );

    // byte: num obelisks (01 default)
    fs.skip( 1 );

    // count final mp2 blocks
    u32 countblock = 0;
    while ( 1 ) {
        u32 l = fs.get();
        u32 h = fs.get();

        // VERBOSE("dump block: 0x" << std::setw(2) << std::setfill('0') << std::hex << l <<
        //	std::setw(2) << std::setfill('0') << std::hex << h);

        if ( 0 == h && 0 == l )
            break;
        else {
            countblock = 256 * h + l - 1;
        }
    }

    // castle or heroes or (events, rumors, etc)
    for ( u32 ii = 0; ii < countblock; ++ii ) {
        s32 findobject = -1;

        // read block
        size_t sizeblock = fs.getLE16();
        std::vector<u8> pblock = fs.getRaw( sizeblock );

        for ( MapsIndexes::const_iterator it_index = vec_object.begin(); it_index != vec_object.end() && findobject < 0; ++it_index ) {
            const Maps::Tiles & tile = vec_tiles[*it_index];

            // orders(quantity2, quantity1)
            u32 orders = ( tile.GetQuantity2() ? tile.GetQuantity2() : 0 );
            orders <<= 8;
            orders |= tile.GetQuantity1();

            if ( orders && !( orders % 0x08 ) && ( ii + 1 == orders / 0x08 ) )
                findobject = *it_index;
        }

        if ( 0 <= findobject ) {
            const Maps::Tiles & tile = vec_tiles[findobject];

            switch ( tile.GetObject() ) {
            case MP2::OBJ_CASTLE:
                // add castle
                if ( SIZEOFMP2CASTLE != pblock.size() ) {
                    DEBUG( DBG_GAME, DBG_WARN,
                           "read castle: "
                               << "incorrect size block: " << pblock.size() );
                }
                else {
                    Castle * castle = GetCastle( Maps::GetPoint( findobject ) );
                    if ( castle ) {
                        castle->LoadFromMP2( StreamBuf( pblock ) );
                        Maps::MinimizeAreaForCastle( castle->GetCenter() );
                        map_captureobj.SetColor( tile.GetIndex(), castle->GetColor() );
                    }
                    else {
                        DEBUG( DBG_GAME, DBG_WARN,
                               "load castle: "
                                   << "not found, index: " << findobject );
                    }
                }
                break;
            case MP2::OBJ_RNDTOWN:
            case MP2::OBJ_RNDCASTLE:
                // add rnd castle
                if ( SIZEOFMP2CASTLE != pblock.size() ) {
                    DEBUG( DBG_GAME, DBG_WARN,
                           "read castle: "
                               << "incorrect size block: " << pblock.size() );
                }
                else {
                    Castle * castle = GetCastle( Maps::GetPoint( findobject ) );
                    if ( castle ) {
                        castle->LoadFromMP2( StreamBuf( pblock ) );
                        Maps::UpdateCastleSprite( castle->GetCenter(), castle->GetRace(), castle->isCastle(), true );
                        Maps::MinimizeAreaForCastle( castle->GetCenter() );
                        map_captureobj.SetColor( tile.GetIndex(), castle->GetColor() );
                    }
                    else {
                        DEBUG( DBG_GAME, DBG_WARN,
                               "load castle: "
                                   << "not found, index: " << findobject );
                    }
                }
                break;
            case MP2::OBJ_JAIL:
                // add jail
                if ( SIZEOFMP2HEROES != pblock.size() ) {
                    DEBUG( DBG_GAME, DBG_WARN,
                           "read heroes: "
                               << "incorrect size block: " << pblock.size() );
                }
                else {
                    int race = Race::KNGT;
                    switch ( pblock[0x3c] ) {
                    case 1:
                        race = Race::BARB;
                        break;
                    case 2:
                        race = Race::SORC;
                        break;
                    case 3:
                        race = Race::WRLK;
                        break;
                    case 4:
                        race = Race::WZRD;
                        break;
                    case 5:
                        race = Race::NECR;
                        break;
                    default:
                        break;
                    }

                    Heroes * hero = GetFreemanHeroes( race );

                    if ( hero ) {
                        hero->LoadFromMP2( findobject, Color::NONE, hero->GetRace(), StreamBuf( pblock ) );
                        hero->SetModes( Heroes::JAIL );
                    }
                }
                break;
            case MP2::OBJ_HEROES:
                // add heroes
                if ( SIZEOFMP2HEROES != pblock.size() ) {
                    DEBUG( DBG_GAME, DBG_WARN,
                           "read heroes: "
                               << "incorrect size block: " << pblock.size() );
                }
                else {
                    std::pair<int, int> colorRace = Maps::Tiles::ColorRaceFromHeroSprite( tile.GetObjectSpriteIndex() );
                    Kingdom & kingdom = GetKingdom( colorRace.first );

                    if ( colorRace.second == Race::RAND && colorRace.first != Color::NONE )
                        colorRace.second = kingdom.GetRace();

                    // check heroes max count
                    if ( kingdom.AllowRecruitHero( false, 0 ) ) {
                        Heroes * hero = NULL;

                        if ( pblock[17] && pblock[18] < Heroes::BAX )
                            hero = vec_heroes.Get( pblock[18] );

                        if ( !hero || !hero->isFreeman() )
                            hero = vec_heroes.GetFreeman( colorRace.second );

                        if ( hero )
                            hero->LoadFromMP2( findobject, colorRace.first, colorRace.second, StreamBuf( pblock ) );
                    }
                    else {
                        DEBUG( DBG_GAME, DBG_WARN, "load heroes maximum" );
                    }
                }
                break;
            case MP2::OBJ_SIGN:
            case MP2::OBJ_BOTTLE:
                // add sign or buttle
                if ( SIZEOFMP2SIGN - 1 < pblock.size() && 0x01 == pblock[0] ) {
                    MapSign * obj = new MapSign();
                    obj->LoadFromMP2( findobject, StreamBuf( pblock ) );
                    map_objects.add( obj );
                }
                break;
            case MP2::OBJ_EVENT:
                // add event maps
                if ( SIZEOFMP2EVENT - 1 < pblock.size() && 0x01 == pblock[0] ) {
                    MapEvent * obj = new MapEvent();
                    obj->LoadFromMP2( findobject, StreamBuf( pblock ) );
                    map_objects.add( obj );
                }
                break;
            case MP2::OBJ_SPHINX:
                // add riddle sphinx
                if ( SIZEOFMP2RIDDLE - 1 < pblock.size() && 0x00 == pblock[0] ) {
                    MapSphinx * obj = new MapSphinx();
                    obj->LoadFromMP2( findobject, StreamBuf( pblock ) );
                    map_objects.add( obj );
                }
                break;
            default:
                break;
            }
        }
        // other events
        else if ( 0x00 == pblock[0] ) {
            // add event day
            if ( SIZEOFMP2EVENT - 1 < pblock.size() && 1 == pblock[42] ) {
                vec_eventsday.push_back( EventDate() );
                vec_eventsday.back().LoadFromMP2( StreamBuf( pblock ) );
            }
            // add rumors
            else if ( SIZEOFMP2RUMOR - 1 < pblock.size() ) {
                if ( pblock[8] ) {
                    vec_rumors.push_back( Game::GetEncodeString( StreamBuf( &pblock[8], pblock.size() - 8 ).toString() ) );
                    DEBUG( DBG_GAME, DBG_INFO, "add rumors: " << vec_rumors.back() );
                }
            }
        }
        // debug
        else {
            DEBUG( DBG_GAME, DBG_WARN, "read maps: unknown block addons, size: " << pblock.size() );
        }
    }

    ProcessNewMap();

    DEBUG( DBG_GAME, DBG_INFO, "end load" );
    return true;
}

void World::ProcessNewMap()
{
    // modify other objects
    for ( size_t ii = 0; ii < vec_tiles.size(); ++ii ) {
        Maps::Tiles & tile = vec_tiles[ii];

        Maps::Tiles::FixedPreload( tile );

        //
        switch ( tile.GetObject() ) {
        case MP2::OBJ_WITCHSHUT:
        case MP2::OBJ_SHRINE1:
        case MP2::OBJ_SHRINE2:
        case MP2::OBJ_SHRINE3:
        case MP2::OBJ_STONELITHS:
        case MP2::OBJ_FOUNTAIN:
        case MP2::OBJ_EVENT:
        case MP2::OBJ_BOAT:
        case MP2::OBJ_RNDARTIFACT:
        case MP2::OBJ_RNDARTIFACT1:
        case MP2::OBJ_RNDARTIFACT2:
        case MP2::OBJ_RNDARTIFACT3:
        case MP2::OBJ_RNDRESOURCE:
        case MP2::OBJ_WATERCHEST:
        case MP2::OBJ_TREASURECHEST:
        case MP2::OBJ_ARTIFACT:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_MAGICGARDEN:
        case MP2::OBJ_WATERWHEEL:
        case MP2::OBJ_WINDMILL:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_LEANTO:
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_SHIPWRECKSURVIROR:
        case MP2::OBJ_DERELICTSHIP:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_PYRAMID:
        case MP2::OBJ_DAEMONCAVE:
        case MP2::OBJ_ABANDONEDMINE:
        case MP2::OBJ_ALCHEMYLAB:
        case MP2::OBJ_SAWMILL:
        case MP2::OBJ_MINES:
        case MP2::OBJ_TREEKNOWLEDGE:
        case MP2::OBJ_BARRIER:
        case MP2::OBJ_TRAVELLERTENT:
        case MP2::OBJ_MONSTER:
        case MP2::OBJ_RNDMONSTER:
        case MP2::OBJ_RNDMONSTER1:
        case MP2::OBJ_RNDMONSTER2:
        case MP2::OBJ_RNDMONSTER3:
        case MP2::OBJ_RNDMONSTER4:
        case MP2::OBJ_ANCIENTLAMP:
        case MP2::OBJ_WATCHTOWER:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_TREEHOUSE:
        case MP2::OBJ_ARCHERHOUSE:
        case MP2::OBJ_GOBLINHUT:
        case MP2::OBJ_DWARFCOTT:
        case MP2::OBJ_HALFLINGHOLE:
        case MP2::OBJ_PEASANTHUT:
        case MP2::OBJ_THATCHEDHUT:
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREECITY:
        case MP2::OBJ_WAGONCAMP:
        case MP2::OBJ_DESERTTENT:
        case MP2::OBJ_TROLLBRIDGE:
        case MP2::OBJ_DRAGONCITY:
        case MP2::OBJ_CITYDEAD:
            tile.QuantityUpdate();
            break;

        case MP2::OBJ_WATERALTAR:
        case MP2::OBJ_AIRALTAR:
        case MP2::OBJ_FIREALTAR:
        case MP2::OBJ_EARTHALTAR:
        case MP2::OBJ_BARROWMOUNDS:
            tile.QuantityReset();
            tile.QuantityUpdate();
            break;

        case MP2::OBJ_HEROES: {
            // remove map editor sprite
            if ( MP2::GetICNObject( tile.GetObjectTileset() ) == ICN::MINIHERO )
                tile.Remove( tile.GetObjectUID() );

            tile.SetHeroes( GetHeroes( Maps::GetPoint( ii ) ) );
        } break;

        default:
            break;
        }
    }

    // add heroes to kingdoms
    vec_kingdoms.AddHeroes( vec_heroes );

    // add castles to kingdoms
    vec_kingdoms.AddCastles( vec_castles );

    // update wins, loss conditions
    if ( GameOver::WINS_HERO & Settings::Get().ConditionWins() ) {
        Heroes * hero = GetHeroes( Settings::Get().WinsMapsPositionObject() );
        heroes_cond_wins = hero ? hero->GetID() : Heroes::UNKNOWN;
    }
    if ( GameOver::LOSS_HERO & Settings::Get().ConditionLoss() ) {
        Heroes * hero = GetHeroes( Settings::Get().LossMapsPositionObject() );
        if ( hero ) {
            heroes_cond_loss = hero->GetID();
            hero->SetModes( Heroes::NOTDISMISS | Heroes::NOTDEFAULTS );
        }
    }

    PostLoad();

    // play with hero
    vec_kingdoms.ApplyPlayWithStartingHero();

    if ( Settings::Get().ExtWorldStartHeroLossCond4Humans() )
        vec_kingdoms.AddCondLossHeroes( vec_heroes );

    // play with debug hero
    if ( IS_DEVEL() ) {
        // get first castle position
        Kingdom & kingdom = GetKingdom( Color::GetFirst( Players::HumanColors() ) );

        if ( kingdom.GetCastles().size() ) {
            const Castle * castle = kingdom.GetCastles().front();
            Heroes * hero = vec_heroes.Get( Heroes::SANDYSANDY );

            if ( hero ) {
                const Point & cp = castle->GetCenter();
                hero->Recruit( castle->GetColor(), Point( cp.x, cp.y + 1 ) );
            }
        }
    }

    // set ultimate
    MapsTiles::iterator it = std::find_if( vec_tiles.begin(), vec_tiles.end(),
                                           []( const Maps::Tiles & tile ) { return tile.isObject( static_cast<int>( MP2::OBJ_RNDULTIMATEARTIFACT ) ); } );
    Point ultimate_pos;

    // not found
    if ( vec_tiles.end() == it ) {
        // generate position for ultimate
        MapsIndexes pools;
        pools.reserve( vec_tiles.size() / 2 );

        for ( size_t ii = 0; ii < vec_tiles.size(); ++ii ) {
            const Maps::Tiles & tile = vec_tiles[ii];
            const s32 x = tile.GetIndex() % w();
            const s32 y = tile.GetIndex() / w();
            if ( tile.GoodForUltimateArtifact() && x > 5 && x < w() - 5 && y > 5 && y < h() - 5 )
                pools.push_back( tile.GetIndex() );
        }

        if ( pools.size() ) {
            const s32 pos = *Rand::Get( pools );
            ultimate_artifact.Set( pos, Artifact::Rand( Artifact::ART_ULTIMATE ) );
            ultimate_pos = Maps::GetPoint( pos );
        }
    }
    else {
        // remove ultimate artifact sprite
        const uint8_t objectIndex = it->GetObjectSpriteIndex();
        it->Remove( it->GetObjectUID() );
        it->SetObject( MP2::OBJ_ZERO );
        ultimate_artifact.Set( it->GetIndex(), Artifact::FromMP2IndexSprite( objectIndex ) );
        ultimate_pos = ( *it ).GetCenter();
    }

    std::string rumor = _( "The ultimate artifact is really the %{name}." );
    StringReplace( rumor, "%{name}", ultimate_artifact.GetName() );
    vec_rumors.push_back( rumor );

    rumor = _( "The ultimate artifact may be found in the %{name} regions of the world." );

    if ( world.h() / 3 > ultimate_pos.y ) {
        if ( world.w() / 3 > ultimate_pos.x )
            StringReplace( rumor, "%{name}", _( "north-west" ) );
        else if ( 2 * world.w() / 3 > ultimate_pos.x )
            StringReplace( rumor, "%{name}", _( "north" ) );
        else
            StringReplace( rumor, "%{name}", _( "north-east" ) );
    }
    else if ( 2 * world.h() / 3 > ultimate_pos.y ) {
        if ( world.w() / 3 > ultimate_pos.x )
            StringReplace( rumor, "%{name}", _( "west" ) );
        else if ( 2 * world.w() / 3 > ultimate_pos.x )
            StringReplace( rumor, "%{name}", _( "center" ) );
        else
            StringReplace( rumor, "%{name}", _( "east" ) );
    }
    else {
        if ( world.w() / 3 > ultimate_pos.x )
            StringReplace( rumor, "%{name}", _( "south-west" ) );
        else if ( 2 * world.w() / 3 > ultimate_pos.x )
            StringReplace( rumor, "%{name}", _( "south" ) );
        else
            StringReplace( rumor, "%{name}", _( "south-east" ) );
    }
    vec_rumors.push_back( rumor );

    vec_rumors.push_back( _( "The truth is out there." ) );
    vec_rumors.push_back( _( "The dark side is stronger." ) );
    vec_rumors.push_back( _( "The end of the world is near." ) );
    vec_rumors.push_back( _( "The bones of Lord Slayer are buried in the foundation of the arena." ) );
    vec_rumors.push_back( _( "A Black Dragon will take out a Titan any day of the week." ) );
    vec_rumors.push_back( _( "He told her: Yada yada yada...  and then she said: Blah, blah, blah..." ) );

    vec_rumors.push_back( _( "Check the newest version of game at\nhttps://github.com/ihhub/\nfheroes2/releases" ) );
}
