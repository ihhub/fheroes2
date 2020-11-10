/***************************************************************************
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *   Copyright (C) 2009 by Josh Matthews  <josh@joshmatthews.net>          *
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

#ifndef H2HEROESBASE_H
#define H2HEROESBASE_H

#include "artifact.h"
#include "bitmodes.h"
#include "players.h"
#include "position.h"
#include "skill.h"
#include "spell_book.h"

class Army;
class Castle;

enum
{
    MDF_NONE,
    MDF_ATTACK,
    MDF_DEFENSE,
    MDF_POWER,
    MDF_KNOWLEDGE,
    MDF_MORALE,
    MDF_LUCK
};
enum
{
    PORT_BIG = 1,
    PORT_MEDIUM = 2,
    PORT_SMALL = 3
};

class HeroBase : public Skill::Primary, public MapPosition, public BitModes, public Control
{
public:
    HeroBase( int type, int race );
    HeroBase();

    enum
    {
        UNDEFINED,
        CAPTAIN,
        HEROES
    };

    virtual const std::string & GetName( void ) const = 0;
    virtual int GetColor( void ) const = 0;
    virtual int GetControl( void ) const = 0;
    virtual bool isValid( void ) const = 0;

    virtual const Army & GetArmy( void ) const = 0;
    virtual Army & GetArmy( void ) = 0;

    virtual u32 GetMaxSpellPoints( void ) const = 0;

    virtual int GetLevelSkill( int skill ) const = 0;
    virtual u32 GetSecondaryValues( int skill ) const = 0;

    virtual void ActionAfterBattle( void ) = 0;
    virtual void ActionPreBattle( void ) = 0;

    virtual const Castle * inCastle( void ) const = 0;
    virtual void PortraitRedraw( s32, s32, int type, fheroes2::Image & ) const = 0;

    virtual int GetType( void ) const = 0;

    bool isCaptain( void ) const;
    bool isHeroes( void ) const;

    int GetAttackModificator( std::string * = NULL ) const;
    int GetDefenseModificator( std::string * = NULL ) const;
    int GetPowerModificator( std::string * = NULL ) const;
    int GetKnowledgeModificator( std::string * = NULL ) const;
    int GetMoraleModificator( std::string * = NULL ) const;
    int GetLuckModificator( std::string * = NULL ) const;

    u32 GetSpellPoints( void ) const;
    bool HaveSpellPoints( const Spell & ) const;
    bool CanCastSpell( const Spell &, std::string * = NULL ) const;
    bool CanTeachSpell( const Spell & ) const;
    bool CanLearnSpell( const Spell & ) const;
    bool CanTranscribeScroll( const Artifact & ) const;
    void TranscribeScroll( const Artifact & );
    void SpellCasted( const Spell & );
    void SetSpellPoints( u32 );

    std::vector<Spell> GetSpells( int lvl = -1 ) const;
    void EditSpellBook( void );
    Spell OpenSpellBook( int filter, bool ) const;
    bool HaveSpellBook( void ) const;
    bool HaveSpell( const Spell &, bool skip_bag = false ) const;
    void AppendSpellToBook( const Spell &, bool without_wisdom = false );
    void AppendSpellsToBook( const SpellStorage &, bool without_wisdom = false );
    bool SpellBookActivate( void );

    BagArtifacts & GetBagArtifacts( void );
    const BagArtifacts & GetBagArtifacts( void ) const;
    u32 HasArtifact( const Artifact & ) const;
    bool PickupArtifact( const Artifact & );

    void LoadDefaults( int type, int race );

protected:
    friend StreamBase & operator<<( StreamBase &, const HeroBase & );
    friend StreamBase & operator>>( StreamBase &, HeroBase & );
#ifdef WITH_XML
    friend TiXmlElement & operator>>( TiXmlElement &, HeroBase & );
#endif

    u32 magic_point;
    u32 move_point;

    SpellBook spell_book;
    BagArtifacts bag_artifacts;
};

StreamBase & operator<<( StreamBase &, const HeroBase & );
StreamBase & operator>>( StreamBase &, HeroBase & );

#endif
