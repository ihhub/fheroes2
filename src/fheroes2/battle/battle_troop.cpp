/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cstring>
#include <functional>

#include "agg.h"
#include "battle_bridge.h"
#include "battle_cell.h"
#include "battle_interface.h"
#include "battle_troop.h"
#include "bin_info.h"
#include "engine.h"
#include "game.h"
#include "game_static.h"
#include "heroes.h"
#include "luck.h"
#include "morale.h"
#include "settings.h"
#include "speed.h"
#include "world.h"

Battle::ModeDuration::ModeDuration()
    : std::pair<u32, u32>( 0, 0 )
{}

Battle::ModeDuration::ModeDuration( u32 mode, u32 duration )
    : std::pair<u32, u32>( mode, duration )
{}

bool Battle::ModeDuration::isMode( u32 mode ) const
{
    return ( first & mode ) != 0;
}

bool Battle::ModeDuration::isZeroDuration( void ) const
{
    return 0 == second;
}

void Battle::ModeDuration::DecreaseDuration( void )
{
    if ( second )
        --second;
}

Battle::ModesAffected::ModesAffected()
{
    reserve( 3 );
}

u32 Battle::ModesAffected::GetMode( u32 mode ) const
{
    const_iterator it = std::find_if( begin(), end(), [mode]( const Battle::ModeDuration & v ) { return v.isMode( mode ); } );
    return it == end() ? 0 : ( *it ).second;
}

void Battle::ModesAffected::AddMode( u32 mode, u32 duration )
{
    iterator it = std::find_if( begin(), end(), [mode]( const Battle::ModeDuration & v ) { return v.isMode( mode ); } );
    if ( it == end() )
        push_back( ModeDuration( mode, duration ) );
    else
        ( *it ).second = duration;
}

void Battle::ModesAffected::RemoveMode( u32 mode )
{
    iterator it = std::find_if( begin(), end(), [mode]( const Battle::ModeDuration & v ) { return v.isMode( mode ); } );
    if ( it != end() ) {
        if ( it + 1 != end() )
            std::swap( *it, back() );
        pop_back();
    }
}

void Battle::ModesAffected::DecreaseDuration( void )
{
    std::for_each( begin(), end(), []( Battle::ModeDuration & v ) { v.DecreaseDuration(); } );
}

u32 Battle::ModesAffected::FindZeroDuration( void ) const
{
    const_iterator it = std::find_if( begin(), end(), []( const Battle::ModeDuration & v ) { return v.isZeroDuration(); } );
    return it == end() ? 0 : ( *it ).first;
}

Battle::Unit::Unit( const Troop & t, s32 pos, bool ref )
    : ArmyTroop( NULL, t )
    , animation( id )
    , uid( World::GetUniq() )
    , hp( t.GetHitPoints() )
    , count0( t.GetCount() )
    , dead( 0 )
    , shots( t.GetShots() )
    , disruptingray( 0 )
    , reflect( ref )
    , mirror( NULL )
    , idleTimer( animation.getIdleDelay() )
    , blindanswer( false )
    , customAlphaMask( 255 )
{
    // set position
    if ( Board::isValidIndex( pos ) ) {
        if ( t.isWide() )
            pos += ( reflect ? -1 : 1 );
        SetPosition( pos );
    }
}

Battle::Unit::~Unit()
{
    // reset summon elemental and mirror image
    if ( Modes( CAP_SUMMONELEM ) || Modes( CAP_MIRRORIMAGE ) ) {
        SetCount( 0 );
    }
}

void Battle::Unit::SetPosition( s32 pos )
{
    if ( position.GetHead() )
        position.GetHead()->SetUnit( NULL );
    if ( position.GetTail() )
        position.GetTail()->SetUnit( NULL );

    position.Set( pos, isWide(), reflect );

    if ( position.GetHead() )
        position.GetHead()->SetUnit( this );
    if ( position.GetTail() )
        position.GetTail()->SetUnit( this );
}

void Battle::Unit::SetPosition( const Position & pos )
{
    if ( position.GetHead() )
        position.GetHead()->SetUnit( NULL );
    if ( position.GetTail() )
        position.GetTail()->SetUnit( NULL );

    position = pos;

    if ( position.GetHead() )
        position.GetHead()->SetUnit( this );
    if ( position.GetTail() )
        position.GetTail()->SetUnit( this );

    if ( isWide() ) {
        reflect = GetHeadIndex() < GetTailIndex();
    }
}

void Battle::Unit::SetReflection( bool r )
{
    if ( reflect != r )
        position.Swap();

    reflect = r;
}

void Battle::Unit::UpdateDirection( void )
{
    // set auto reflect
    SetReflection( GetArena()->GetArmyColor1() != GetArmyColor() );
}

bool Battle::Unit::UpdateDirection( const Rect & pos )
{
    bool need = position.GetRect().x > pos.x;

    if ( need != reflect ) {
        SetReflection( need );
        return true;
    }
    return false;
}

bool Battle::Unit::isBattle( void ) const
{
    return true;
}

bool Battle::Unit::isModes( u32 v ) const
{
    return Modes( v );
}

std::string Battle::Unit::GetShotString( void ) const
{
    if ( Troop::GetShots() == GetShots() )
        return GetString( Troop::GetShots() );

    std::ostringstream os;
    os << Troop::GetShots() << " (" << GetShots() << ")";
    return os.str();
}

std::string Battle::Unit::GetSpeedString( void ) const
{
    std::ostringstream os;
    os << Speed::String( GetSpeed() ) << " (" << GetSpeed() << ")";
    return os.str();
}

u32 Battle::Unit::GetDead( void ) const
{
    return dead;
}

u32 Battle::Unit::GetHitPointsLeft( void ) const
{
    return GetHitPoints() - ( GetCount() - 1 ) * Monster::GetHitPoints();
}

u32 Battle::Unit::GetAffectedDuration( u32 mod ) const
{
    return affected.GetMode( mod );
}

u32 Battle::Unit::GetSpeed( void ) const
{
    return GetSpeed( false );
}

bool Battle::Unit::isUID( u32 v ) const
{
    return uid == v;
}

u32 Battle::Unit::GetUID( void ) const
{
    return uid;
}

Battle::Unit * Battle::Unit::GetMirror()
{
    return mirror;
}

void Battle::Unit::SetMirror( Unit * ptr )
{
    mirror = ptr;
}

u32 Battle::Unit::GetShots( void ) const
{
    return shots;
}

const Battle::Position & Battle::Unit::GetPosition( void ) const
{
    return position;
}

s32 Battle::Unit::GetHeadIndex( void ) const
{
    return position.GetHead() ? position.GetHead()->GetIndex() : -1;
}

s32 Battle::Unit::GetTailIndex( void ) const
{
    return position.GetTail() ? position.GetTail()->GetIndex() : -1;
}

void Battle::Unit::SetRandomMorale( void )
{
    s32 morale = GetMorale();

    // Bone dragon affects morale, not luck
    if ( GetArena()->GetForce( GetArmyColor(), true ).HasMonster( Monster::BONE_DRAGON ) && morale > Morale::TREASON )
        --morale;

    if ( morale > 0 && static_cast<int32_t>( Rand::Get( 1, 24 ) ) <= morale ) {
        SetModes( MORALE_GOOD );
    }
    else if ( morale < 0 && static_cast<int32_t>( Rand::Get( 1, 12 ) ) <= -morale ) {
        if ( isControlHuman() ) {
            SetModes( MORALE_BAD );
        }
        // AI is given a cheeky 25% chance to avoid it - because they build armies from random troops
        else if ( Rand::Get( 1, 4 ) != 1 ) {
            SetModes( MORALE_BAD );
        }
    }
}

void Battle::Unit::SetRandomLuck( void )
{
    const int32_t luck = GetLuck();
    const int32_t chance = static_cast<int32_t>( Rand::Get( 1, 24 ) );

    if ( luck > 0 && chance <= luck ) {
        SetModes( LUCK_GOOD );
    }
    else if ( luck < 0 && chance <= -luck ) {
        SetModes( LUCK_BAD );
    }

    // Bless, Curse and Luck do stack
}

bool Battle::Unit::isFlying( void ) const
{
    return ArmyTroop::isFlying() && !Modes( SP_SLOW );
}

bool Battle::Unit::isValid( void ) const
{
    return GetCount() != 0;
}

bool Battle::Unit::isReflect( void ) const
{
    return reflect;
}

bool Battle::Unit::OutOfWalls( void ) const
{
    return Board::isOutOfWallsIndex( GetHeadIndex() ) || ( isWide() && Board::isOutOfWallsIndex( GetTailIndex() ) );
}

bool Battle::Unit::canReach( int index ) const
{
    if ( !Board::isValidIndex( index ) )
        return false;

    if ( isFlying() || ( isArchers() && !isHandFighting() ) )
        return true;

    const bool isIndirectAttack = isReflect() == Board::isNegativeDistance( GetHeadIndex(), index );
    const int from = ( isWide() && isIndirectAttack ) ? GetTailIndex() : GetHeadIndex();
    return static_cast<uint32_t>( Board::GetDistance( from, index ) ) <= GetSpeed( true );
}

bool Battle::Unit::canReach( const Unit & unit ) const
{
    if ( unit.Modes( CAP_TOWER ) )
        return false;

    const bool isIndirectAttack = isReflect() == Board::isNegativeDistance( GetHeadIndex(), unit.GetHeadIndex() );
    const int target = ( unit.isWide() && isIndirectAttack ) ? unit.GetTailIndex() : unit.GetHeadIndex();
    return canReach( target );
}

bool Battle::Unit::isHandFighting( void ) const
{
    if ( GetCount() && !Modes( CAP_TOWER ) ) {
        const Indexes around = Board::GetAroundIndexes( *this );

        for ( Indexes::const_iterator it = around.begin(); it != around.end(); ++it ) {
            const Unit * enemy = Board::GetCell( *it )->GetUnit();
            if ( enemy && enemy->GetColor() != GetColor() )
                return true;
        }
    }

    return false;
}

bool Battle::Unit::isHandFighting( const Unit & a, const Unit & b )
{
    return a.isValid() && !a.Modes( CAP_TOWER ) && b.isValid() && b.GetColor() != a.GetCurrentColor()
           && ( Board::isNearIndexes( a.GetHeadIndex(), b.GetHeadIndex() ) || ( b.isWide() && Board::isNearIndexes( a.GetHeadIndex(), b.GetTailIndex() ) )
                || ( a.isWide()
                     && ( Board::isNearIndexes( a.GetTailIndex(), b.GetHeadIndex() )
                          || ( b.isWide() && Board::isNearIndexes( a.GetTailIndex(), b.GetTailIndex() ) ) ) ) );
}

int Battle::Unit::GetAnimationState() const
{
    return animation.getCurrentState();
}

bool Battle::Unit::isIdling() const
{
    return GetAnimationState() == Monster_Info::IDLE;
}

bool Battle::Unit::checkIdleDelay()
{
    return idleTimer.checkDelay();
}

void Battle::Unit::NewTurn( void )
{
    if ( isRegenerating() )
        hp = ArmyTroop::GetHitPoints();

    ResetModes( TR_RESPONSED );
    ResetModes( TR_MOVED );
    ResetModes( TR_SKIPMOVE );
    ResetModes( TR_HARDSKIP );
    ResetModes( TR_DEFENSED );
    ResetModes( MORALE_BAD );
    ResetModes( MORALE_GOOD );
    ResetModes( LUCK_BAD );
    ResetModes( LUCK_GOOD );

    // decrease spell duration
    affected.DecreaseDuration();

    // remove spell duration
    u32 mode = 0;
    while ( 0 != ( mode = affected.FindZeroDuration() ) ) {
        affected.RemoveMode( mode );
        ResetModes( mode );

        // cancel mirror image
        if ( mode == CAP_MIRROROWNER && mirror ) {
            if ( Arena::GetInterface() ) {
                std::vector<Unit *> images;
                images.push_back( mirror );
                Arena::GetInterface()->RedrawActionRemoveMirrorImage( images );
            }

            mirror->SetCount( 0 );
            mirror = NULL;
        }
    }
}

u32 Battle::Unit::GetSpeed( bool skip_standing_check ) const
{
    if ( !skip_standing_check && ( !GetCount() || Modes( TR_MOVED | SP_BLIND | IS_PARALYZE_MAGIC ) ) )
        return Speed::STANDING;

    uint32_t speed = Monster::GetSpeed();
    Spell spell;

    if ( Modes( SP_HASTE ) ) {
        spell = Spell::HASTE;
        return spell.ExtraValue() ? speed + spell.ExtraValue() : Speed::GetOriginalFast( speed );
    }
    else if ( Modes( SP_SLOW ) ) {
        spell = Spell::SLOW;
        return spell.ExtraValue() ? speed - spell.ExtraValue() : Speed::GetOriginalSlow( speed );
    }

    return speed;
}

uint32_t Battle::Unit::CalculateRetaliationDamage( uint32_t damageTaken ) const
{
    // Check if there will be retaliation in the first place
    if ( damageTaken > hp || Modes( CAP_MIRRORIMAGE ) || !AllowResponse() )
        return 0;

    const uint32_t unitsLeft = ( hp - damageTaken ) / Monster::GetHitPoints();

    uint32_t damagePerUnit = 0;
    if ( Modes( SP_CURSE ) )
        damagePerUnit = Monster::GetDamageMin();
    else if ( Modes( SP_BLESS ) )
        damagePerUnit = Monster::GetDamageMax();
    else
        damagePerUnit = ( Monster::GetDamageMin() + Monster::GetDamageMax() ) / 2;

    return unitsLeft * damagePerUnit;
}

u32 Battle::Unit::CalculateMinDamage( const Unit & enemy ) const
{
    return CalculateDamageUnit( enemy, ArmyTroop::GetDamageMin() );
}

u32 Battle::Unit::CalculateMaxDamage( const Unit & enemy ) const
{
    return CalculateDamageUnit( enemy, ArmyTroop::GetDamageMax() );
}

u32 Battle::Unit::CalculateDamageUnit( const Unit & enemy, float dmg ) const
{
    if ( isArchers() ) {
        if ( !isHandFighting() ) {
            // check skill archery +%10, +%25, +%50
            if ( GetCommander() ) {
                dmg += ( dmg * GetCommander()->GetSecondaryValues( Skill::Secondary::ARCHERY ) / 100 );
            }

            // check castle defense
            if ( GetArena()->GetObstaclesPenalty( *this, enemy ) )
                dmg /= 2;

            // check spell shield
            if ( enemy.Modes( SP_SHIELD ) )
                dmg /= Spell( Spell::SHIELD ).ExtraValue();
        }
        else if ( hasMeleePenalty() ) {
            dmg /= 2;
        }
    }

    // after blind
    if ( Modes( SP_BLIND ) )
        dmg /= 2;

    // stone cap.
    if ( enemy.Modes( SP_STONE ) )
        dmg /= 2;

    // check monster capability
    switch ( GetID() ) {
    case Monster::CRUSADER:
        // double damage for undead
        if ( enemy.isUndead() )
            dmg *= 2;
        break;
    case Monster::FIRE_ELEMENT:
        if ( enemy.GetID() == Monster::WATER_ELEMENT )
            dmg *= 2;
        break;
    case Monster::WATER_ELEMENT:
        if ( enemy.GetID() == Monster::FIRE_ELEMENT )
            dmg *= 2;
        break;
    case Monster::AIR_ELEMENT:
        if ( enemy.GetID() == Monster::EARTH_ELEMENT )
            dmg *= 2;
        break;
    case Monster::EARTH_ELEMENT:
        if ( enemy.GetID() == Monster::AIR_ELEMENT )
            dmg *= 2;
        break;
    default:
        break;
    }

    int r = GetAttack() - enemy.GetDefense();
    if ( enemy.isDragons() && Modes( SP_DRAGONSLAYER ) )
        r += Spell( Spell::DRAGONSLAYER ).ExtraValue();

    // Attack bonus is 20% to 300%
    dmg *= 1 + ( 0 < r ? 0.1f * std::min( r, 20 ) : 0.05f * std::max( r, -16 ) );

    return static_cast<u32>( dmg ) < 1 ? 1 : static_cast<u32>( dmg );
}

u32 Battle::Unit::GetDamage( const Unit & enemy ) const
{
    u32 res = 0;

    if ( Modes( SP_BLESS ) )
        res = CalculateMaxDamage( enemy );
    else if ( Modes( SP_CURSE ) )
        res = CalculateMinDamage( enemy );
    else
        res = Rand::Get( CalculateMinDamage( enemy ), CalculateMaxDamage( enemy ) );

    if ( Modes( LUCK_GOOD ) )
        res <<= 1; // mul 2
    else if ( Modes( LUCK_BAD ) )
        res >>= 1; // div 2

    return res;
}

u32 Battle::Unit::HowManyCanKill( const Unit & b ) const
{
    return b.HowManyWillKilled( ( CalculateMinDamage( b ) + CalculateMaxDamage( b ) ) / 2 );
}

u32 Battle::Unit::HowManyWillKilled( u32 dmg ) const
{
    return dmg >= hp ? GetCount() : GetCount() - Monster::GetCountFromHitPoints( *this, hp - dmg );
}

u32 Battle::Unit::ApplyDamage( u32 dmg )
{
    if ( dmg && GetCount() ) {
        u32 killed = HowManyWillKilled( dmg );

        // mirror image dies if recieves any damage
        if ( Modes( CAP_MIRRORIMAGE ) ) {
            dmg = hp;
            killed = GetCount();
        }

        DEBUG( DBG_BATTLE, DBG_TRACE, dmg << " to " << String() << " and killed: " << killed );

        if ( killed >= GetCount() ) {
            dead += GetCount();
            SetCount( 0 );
        }
        else {
            dead += killed;
            SetCount( GetCount() - killed );
        }
        hp -= ( dmg >= hp ? hp : dmg );

        return killed;
    }

    return 0;
}

void Battle::Unit::PostKilledAction( void )
{
    // kill mirror image (master)
    if ( Modes( CAP_MIRROROWNER ) ) {
        modes = 0;
        mirror->hp = 0;
        mirror->SetCount( 0 );
        mirror->mirror = NULL;
        mirror = NULL;
        ResetModes( CAP_MIRROROWNER );
    }
    // kill mirror image (slave)
    if ( Modes( CAP_MIRRORIMAGE ) && mirror != NULL ) {
        mirror->ResetModes( CAP_MIRROROWNER );
        mirror = NULL;
    }

    ResetModes( IS_MAGIC );
    ResetModes( TR_RESPONSED );
    ResetModes( TR_SKIPMOVE );
    ResetModes( LUCK_GOOD );
    ResetModes( LUCK_BAD );
    ResetModes( MORALE_GOOD );
    ResetModes( MORALE_BAD );

    SetModes( TR_MOVED );

    // save troop to graveyard
    // skip mirror and summon
    if ( !Modes( CAP_MIRRORIMAGE ) && !Modes( CAP_SUMMONELEM ) )
        Arena::GetGraveyard()->AddTroop( *this );

    Cell * head = Board::GetCell( GetHeadIndex() );
    Cell * tail = Board::GetCell( GetTailIndex() );
    if ( head )
        head->SetUnit( NULL );
    if ( tail )
        tail->SetUnit( NULL );

    DEBUG( DBG_BATTLE, DBG_TRACE, String() << ", is dead..." );
    // possible also..
}

u32 Battle::Unit::Resurrect( u32 points, bool allow_overflow, bool skip_dead )
{
    u32 resurrect = Monster::GetCountFromHitPoints( *this, hp + points ) - GetCount();

    if ( hp == 0 ) // Skip turn if already dead
        SetModes( TR_MOVED );

    SetCount( GetCount() + resurrect );
    hp += points;

    if ( allow_overflow ) {
        if ( count0 < GetCount() )
            count0 = GetCount();
    }
    else if ( GetCount() > count0 ) {
        resurrect -= GetCount() - count0;
        SetCount( count0 );
        hp = ArmyTroop::GetHitPoints();
    }

    if ( !skip_dead )
        dead -= ( resurrect < dead ? resurrect : dead );

    return resurrect;
}

u32 Battle::Unit::ApplyDamage( Unit & enemy, u32 dmg )
{
    u32 killed = ApplyDamage( dmg );
    u32 resurrect;

    if ( killed )
        switch ( enemy.GetID() ) {
        case Monster::GHOST:
            resurrect = killed * static_cast<Monster &>( enemy ).GetHitPoints();
            DEBUG( DBG_BATTLE, DBG_TRACE, String() << ", enemy: " << enemy.String() << " resurrect: " << resurrect );
            // grow troop
            enemy.Resurrect( resurrect, true, false );
            break;

        case Monster::VAMPIRE_LORD:
            resurrect = killed * Monster::GetHitPoints();
            DEBUG( DBG_BATTLE, DBG_TRACE, String() << ", enemy: " << enemy.String() << " resurrect: " << resurrect );
            // restore hit points
            enemy.Resurrect( resurrect, false, false );
            break;

        default:
            break;
        }

    // clean paralyze or stone magic
    if ( Modes( IS_PARALYZE_MAGIC ) ) {
        SetModes( TR_RESPONSED );
        SetModes( TR_MOVED );
        ResetModes( IS_PARALYZE_MAGIC );
        affected.RemoveMode( IS_PARALYZE_MAGIC );
    }

    // blind
    if ( Modes( SP_BLIND ) ) {
        blindanswer = true;
    }

    return killed;
}

bool Battle::Unit::AllowApplySpell( const Spell & spell, const HeroBase * hero, std::string * msg, bool forceApplyToAlly ) const
{
    if ( Modes( SP_ANTIMAGIC ) )
        return false;

    if ( ( Modes( CAP_MIRRORIMAGE ) || Modes( CAP_MIRROROWNER ) ) && ( spell == Spell::ANTIMAGIC || spell == Spell::MIRRORIMAGE ) )
        return false;

    // check global
    // if(GetArena()->DisableCastSpell(spell, msg)) return false; // disable - recursion!

    if ( hero && spell.isApplyToFriends() && GetColor() != hero->GetColor() )
        return false;
    if ( hero && spell.isApplyToEnemies() && GetColor() == hero->GetColor() && !forceApplyToAlly )
        return false;
    if ( isMagicResist( spell, ( hero ? hero->GetPower() : 0 ) ) )
        return false;

    const HeroBase * myhero = GetCommander();
    if ( !myhero )
        return true;

    // check artifact
    Artifact guard_art( Artifact::UNKNOWN );
    switch ( spell() ) {
    case Spell::CURSE:
    case Spell::MASSCURSE:
        guard_art = Artifact::HOLY_PENDANT;
        break;
    case Spell::HYPNOTIZE:
        guard_art = Artifact::PENDANT_FREE_WILL;
        break;
    case Spell::DEATHRIPPLE:
    case Spell::DEATHWAVE:
        guard_art = Artifact::PENDANT_LIFE;
        break;
    case Spell::BERSERKER:
        guard_art = Artifact::SERENITY_PENDANT;
        break;
    case Spell::BLIND:
        guard_art = Artifact::SEEING_EYE_PENDANT;
        break;
    case Spell::PARALYZE:
        guard_art = Artifact::KINETIC_PENDANT;
        break;
    case Spell::HOLYWORD:
    case Spell::HOLYSHOUT:
        guard_art = Artifact::PENDANT_DEATH;
        break;
    case Spell::DISPEL:
        guard_art = Artifact::WAND_NEGATION;
        break;

    default:
        break;
    }

    if ( guard_art.isValid() && myhero->HasArtifact( guard_art ) ) {
        if ( msg ) {
            *msg = _( "The %{artifact} artifact is in effect for this battle, disabling %{spell} spell." );
            StringReplace( *msg, "%{artifact}", guard_art.GetName() );
            StringReplace( *msg, "%{spell}", spell.GetName() );
        }
        return false;
    }

    return true;
}

bool Battle::Unit::ApplySpell( const Spell & spell, const HeroBase * hero, TargetInfo & target )
{
    // HACK!!! Chain lightining is the only spell which can't be casted on allies but could be applied on them
    const bool isForceApply = ( spell() == Spell::CHAINLIGHTNING );

    if ( !AllowApplySpell( spell, hero, NULL, isForceApply ) )
        return false;

    DEBUG( DBG_BATTLE, DBG_TRACE, spell.GetName() << " to " << String() );

    const u32 spoint = hero ? hero->GetPower() : DEFAULT_SPELL_DURATION;

    if ( spell.isDamage() )
        SpellApplyDamage( spell, spoint, hero, target );
    else if ( spell.isRestore() )
        SpellRestoreAction( spell, spoint, hero );
    else {
        SpellModesAction( spell, spoint, hero );
    }

    return true;
}

std::string Battle::Unit::String( bool more ) const
{
    std::stringstream ss;

    ss << "Unit: "
       << "[ " <<
        // info
        GetCount() << " " << GetName() << ", " << Color::String( GetColor() ) << ", pos: " << GetHeadIndex() << ", " << GetTailIndex() << ( reflect ? ", reflect" : "" );

    if ( more )
        ss << ", mode("
           << "0x" << std::hex << modes << std::dec << ")"
           << ", uid("
           << "0x" << std::setw( 8 ) << std::setfill( '0' ) << std::hex << uid << std::dec << ")"
           << ", speed(" << Speed::String( GetSpeed() ) << ", " << static_cast<int>( GetSpeed() ) << ")"
           << ", hp(" << hp << ")"
           << ", die(" << dead << ")"
           << ")";

    ss << " ]";

    return ss.str();
}

StreamBase & Battle::operator<<( StreamBase & msg, const ModesAffected & v )
{
    msg << static_cast<u32>( v.size() );

    for ( size_t ii = 0; ii < v.size(); ++ii )
        msg << v[ii].first << v[ii].second;

    return msg;
}

StreamBase & Battle::operator>>( StreamBase & msg, ModesAffected & v )
{
    u32 size = 0;
    msg >> size;
    v.clear();

    for ( size_t ii = 0; ii < size; ++ii ) {
        ModeDuration md;
        msg >> md.first >> md.second;
        v.push_back( md );
    }

    return msg;
}

StreamBase & Battle::operator<<( StreamBase & msg, const Unit & b )
{
    return msg << b.modes << b.id << b.count << b.uid << b.hp << b.count0 << b.dead << b.shots << b.disruptingray << b.reflect << b.GetHeadIndex()
               << ( b.mirror ? b.mirror->GetUID() : static_cast<u32>( 0 ) ) << b.affected << b.blindanswer;
}

StreamBase & Battle::operator>>( StreamBase & msg, Unit & b )
{
    s32 head = -1;
    u32 uid = 0;

    msg >> b.modes >> b.id >> b.count >> b.uid >> b.hp >> b.count0 >> b.dead >> b.shots >> b.disruptingray >> b.reflect >> head >> uid >> b.affected >> b.blindanswer;

    b.position.Set( head, b.isWide(), b.isReflect() );
    b.mirror = GetArena()->GetTroopUID( uid );

    return msg;
}

bool Battle::Unit::AllowResponse( void ) const
{
    if ( !Modes( IS_PARALYZE_MAGIC ) ) {
        if ( Modes( SP_BLIND ) )
            return blindanswer;
        else if ( isAlwaysRetaliating() || !Modes( TR_RESPONSED ) )
            return true;
    }

    return false;
}

void Battle::Unit::SetResponse( void )
{
    SetModes( TR_RESPONSED );
}

void Battle::Unit::PostAttackAction( Unit & enemy )
{
    // decrease shots
    if ( isArchers() ) {
        // check ammo cart artifact
        const HeroBase * hero = GetCommander();
        if ( !hero || !hero->HasArtifact( Artifact::AMMO_CART ) )
            --shots;
    }

    // clean berserker spell
    if ( Modes( SP_BERSERKER ) ) {
        ResetModes( SP_BERSERKER );
        affected.RemoveMode( SP_BERSERKER );
    }

    // clean hypnotize spell
    if ( Modes( SP_HYPNOTIZE ) ) {
        ResetModes( SP_HYPNOTIZE );
        affected.RemoveMode( SP_HYPNOTIZE );
    }
    if ( enemy.Modes( SP_HYPNOTIZE ) ) {
        enemy.ResetModes( SP_HYPNOTIZE );
        enemy.affected.RemoveMode( SP_HYPNOTIZE );
    }

    // clean luck capability
    ResetModes( LUCK_GOOD );
    ResetModes( LUCK_BAD );
}

void Battle::Unit::ResetBlind( void )
{
    // remove blind action
    if ( Modes( SP_BLIND ) ) {
        SetModes( TR_MOVED );
        ResetModes( SP_BLIND );
        affected.RemoveMode( SP_BLIND );
    }
}

u32 Battle::Unit::GetAttack( void ) const
{
    u32 res = ArmyTroop::GetAttack();

    if ( Modes( SP_BLOODLUST ) )
        res += Spell( Spell::BLOODLUST ).ExtraValue();

    return res;
}

u32 Battle::Unit::GetDefense( void ) const
{
    u32 res = ArmyTroop::GetDefense();

    if ( Modes( SP_STONESKIN ) )
        res += Spell( Spell::STONESKIN ).ExtraValue();
    else if ( Modes( SP_STEELSKIN ) )
        res += Spell( Spell::STEELSKIN ).ExtraValue();

    // extra
    if ( Modes( TR_DEFENSED ) )
        res += 2;

    // disrupting ray accumulate effect
    if ( disruptingray ) {
        const u32 step = disruptingray * Spell( Spell::DISRUPTINGRAY ).ExtraValue();
        if ( step > res )
            res = 1;
        else
            res -= step;
    }

    // check moat
    const Castle * castle = Arena::GetCastle();
    if ( castle && castle->isBuild( BUILD_MOAT ) ) {
        const Bridge * bridge = Arena::GetBridge();
        const bool isOnBridgeCell = bridge && !bridge->isDown() && ( bridge->isMoatCell( GetHeadIndex() ) || bridge->isMoatCell( GetTailIndex() ) );
        if ( isOnBridgeCell || Board::isMoatIndex( GetHeadIndex() ) || Board::isMoatIndex( GetTailIndex() ) )
            res -= GameStatic::GetBattleMoatReduceDefense();
    }

    if ( res < 1 ) // cannot be less than 1
        res = 1;

    return res;
}

s32 Battle::Unit::GetScoreQuality( const Unit & defender ) const
{
    const Unit & attacker = *this;

    const double defendersDamage = CalculateDamageUnit( attacker, ( static_cast<double>( defender.GetDamageMin() ) + defender.GetDamageMax() ) / 2.0 );
    const double attackerPowerLost = ( attacker.Modes( CAP_MIRRORIMAGE ) || defendersDamage >= hp ) ? 1.0 : defendersDamage / hp;
    const bool attackerIsArchers = isArchers();

    double attackerThreat = CalculateDamageUnit( defender, ( static_cast<double>( GetDamageMin() ) + GetDamageMax() ) / 2.0 );

    if ( !canReach( defender ) && !( defender.Modes( CAP_TOWER ) && attackerIsArchers ) ) {
        // Can't reach, so unit is not dangerous to defender at the moment
        attackerThreat /= 2;
    }

    // Monster special abilities
    if ( isTwiceAttack() ) {
        if ( attackerIsArchers || ignoreRetaliation() || defender.Modes( TR_RESPONSED ) ) {
            attackerThreat *= 2;
        }
        else {
            // check how much we will lose to retaliation
            attackerThreat += attackerThreat * ( 1.0 - attackerPowerLost );
        }
    }

    switch ( id ) {
    case Monster::UNICORN:
        attackerThreat += defendersDamage * 0.2 * ( 100 - defender.GetMagicResist( Spell::BLIND, DEFAULT_SPELL_DURATION ) ) / 100.0;
        break;
    case Monster::CYCLOPS:
        attackerThreat += defendersDamage * 0.2 * ( 100 - defender.GetMagicResist( Spell::PARALYZE, DEFAULT_SPELL_DURATION ) ) / 100.0;
        break;
    case Monster::MEDUSA:
        attackerThreat += defendersDamage * 0.2 * ( 100 - defender.GetMagicResist( Spell::STONE, DEFAULT_SPELL_DURATION ) ) / 100.0;
        break;
    case Monster::VAMPIRE_LORD:
        // Lifesteal
        attackerThreat *= 1.3;
        break;
    case Monster::GENIE:
        // Genie's ability to half enemy troops
        attackerThreat *= 2;
        break;
    case Monster::GHOST:
        // Ghost's ability to increase the numbers
        attackerThreat *= 3;
        break;
    }

    // force big priority on mirror images as they get destroyed in 1 hit
    if ( attacker.Modes( CAP_MIRRORIMAGE ) )
        attackerThreat *= 10;
    // Ignore disabled units
    if ( attacker.Modes( SP_BLIND ) || attacker.Modes( IS_PARALYZE_MAGIC ) )
        attackerThreat = 0;
    // Negative value of units that changed the side
    if ( attacker.Modes( SP_BERSERKER ) || attacker.Modes( SP_HYPNOTIZE ) )
        attackerThreat *= -1;

    // Avoid effectiveness scaling if we're dealing with archers
    if ( !attackerIsArchers || defender.isArchers() )
        attackerThreat *= attackerPowerLost;

    const int score = static_cast<int>( attackerThreat * 10 );
    return ( score == 0 ) ? 1 : score;
}

u32 Battle::Unit::GetHitPoints( void ) const
{
    return hp;
}

int Battle::Unit::GetControl( void ) const
{
    return !GetArmy() ? CONTROL_AI : GetArmy()->GetControl();
}

bool Battle::Unit::isArchers( void ) const
{
    return ArmyTroop::isArchers() && shots;
}

void Battle::Unit::SpellModesAction( const Spell & spell, u32 duration, const HeroBase * hero )
{
    if ( hero ) {
        u32 acount = hero->HasArtifact( Artifact::WIZARD_HAT );
        if ( acount )
            duration += acount * Artifact( Artifact::WIZARD_HAT ).ExtraValue();
        acount = hero->HasArtifact( Artifact::ENCHANTED_HOURGLASS );
        if ( acount )
            duration += acount * Artifact( Artifact::ENCHANTED_HOURGLASS ).ExtraValue();
    }

    switch ( spell() ) {
    case Spell::BLESS:
    case Spell::MASSBLESS:
        if ( Modes( SP_CURSE ) ) {
            ResetModes( SP_CURSE );
            affected.RemoveMode( SP_CURSE );
        }
        SetModes( SP_BLESS );
        affected.AddMode( SP_BLESS, duration );
        break;

    case Spell::BLOODLUST:
        SetModes( SP_BLOODLUST );
        affected.AddMode( SP_BLOODLUST, 3 );
        break;

    case Spell::CURSE:
    case Spell::MASSCURSE:
        if ( Modes( SP_BLESS ) ) {
            ResetModes( SP_BLESS );
            affected.RemoveMode( SP_BLESS );
        }
        SetModes( SP_CURSE );
        affected.AddMode( SP_CURSE, duration );
        break;

    case Spell::HASTE:
    case Spell::MASSHASTE:
        if ( Modes( SP_SLOW ) ) {
            ResetModes( SP_SLOW );
            affected.RemoveMode( SP_SLOW );
        }
        SetModes( SP_HASTE );
        affected.AddMode( SP_HASTE, duration );
        break;

    case Spell::DISPEL:
    case Spell::MASSDISPEL:
        if ( Modes( IS_MAGIC ) ) {
            ResetModes( IS_MAGIC );
            affected.RemoveMode( IS_MAGIC );
        }
        break;

    case Spell::SHIELD:
    case Spell::MASSSHIELD:
        SetModes( SP_SHIELD );
        affected.AddMode( SP_SHIELD, duration );
        break;

    case Spell::SLOW:
    case Spell::MASSSLOW:
        if ( Modes( SP_HASTE ) ) {
            ResetModes( SP_HASTE );
            affected.RemoveMode( SP_HASTE );
        }
        SetModes( SP_SLOW );
        affected.AddMode( SP_SLOW, duration );
        break;

    case Spell::STONESKIN:
        if ( Modes( SP_STEELSKIN ) ) {
            ResetModes( SP_STEELSKIN );
            affected.RemoveMode( SP_STEELSKIN );
        }
        SetModes( SP_STONESKIN );
        affected.AddMode( SP_STONESKIN, duration );
        break;

    case Spell::BLIND:
        SetModes( SP_BLIND );
        blindanswer = false;
        affected.AddMode( SP_BLIND, duration );
        break;

    case Spell::DRAGONSLAYER:
        SetModes( SP_DRAGONSLAYER );
        affected.AddMode( SP_DRAGONSLAYER, duration );
        break;

    case Spell::STEELSKIN:
        if ( Modes( SP_STONESKIN ) ) {
            ResetModes( SP_STONESKIN );
            affected.RemoveMode( SP_STONESKIN );
        }
        SetModes( SP_STEELSKIN );
        affected.AddMode( SP_STEELSKIN, duration );
        break;

    case Spell::ANTIMAGIC:
        ResetModes( IS_MAGIC );
        SetModes( SP_ANTIMAGIC );
        affected.AddMode( SP_ANTIMAGIC, duration );
        break;

    case Spell::PARALYZE:
        SetModes( SP_PARALYZE );
        affected.AddMode( SP_PARALYZE, duration );
        break;

    case Spell::BERSERKER:
        SetModes( SP_BERSERKER );
        affected.AddMode( SP_BERSERKER, duration );
        break;

    case Spell::HYPNOTIZE: {
        SetModes( SP_HYPNOTIZE );
        u32 acount = hero ? hero->HasArtifact( Artifact::GOLD_WATCH ) : 0;
        affected.AddMode( SP_HYPNOTIZE, ( acount ? duration * acount * 2 : duration ) );
    } break;

    case Spell::STONE:
        SetModes( SP_STONE );
        affected.AddMode( SP_STONE, duration );
        break;

    case Spell::MIRRORIMAGE:
        affected.AddMode( CAP_MIRRORIMAGE, duration );
        break;

    case Spell::DISRUPTINGRAY:
        ++disruptingray;
        break;

    default:
        break;
    }
}

void Battle::Unit::SpellApplyDamage( const Spell & spell, u32 spoint, const HeroBase * hero, TargetInfo & target )
{
    u32 dmg = spell.Damage() * spoint;

    switch ( GetID() ) {
    case Monster::IRON_GOLEM:
    case Monster::STEEL_GOLEM:
        switch ( spell() ) {
            // 50% damage
        case Spell::COLDRAY:
        case Spell::COLDRING:
        case Spell::FIREBALL:
        case Spell::FIREBLAST:
        case Spell::LIGHTNINGBOLT:
        case Spell::CHAINLIGHTNING:
        case Spell::ELEMENTALSTORM:
        case Spell::ARMAGEDDON:
            dmg /= 2;
            break;
        default:
            break;
        }
        break;

    case Monster::WATER_ELEMENT:
        switch ( spell() ) {
            // 200% damage
        case Spell::FIREBALL:
        case Spell::FIREBLAST:
            dmg *= 2;
            break;
        default:
            break;
        }
        break;

    case Monster::AIR_ELEMENT:
        switch ( spell() ) {
            // 200% damage
        case Spell::ELEMENTALSTORM:
        case Spell::LIGHTNINGBOLT:
        case Spell::CHAINLIGHTNING:
            dmg *= 2;
            break;
        default:
            break;
        }
        break;

    case Monster::FIRE_ELEMENT:
        switch ( spell() ) {
            // 200% damage
        case Spell::COLDRAY:
        case Spell::COLDRING:
            dmg *= 2;
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    // check artifact
    if ( hero ) {
        const HeroBase * defendingHero = GetCommander();

        switch ( spell() ) {
        case Spell::COLDRAY:
        case Spell::COLDRING:
            // +50%
            if ( hero->HasArtifact( Artifact::EVERCOLD_ICICLE ) )
                dmg += dmg * Artifact( Artifact::EVERCOLD_ICICLE ).ExtraValue() / 100;

            if ( defendingHero ) {
                // -50%
                if ( defendingHero->HasArtifact( Artifact::ICE_CLOAK ) )
                    dmg -= dmg * Artifact( Artifact::ICE_CLOAK ).ExtraValue() / 100;

                if ( defendingHero->HasArtifact( Artifact::HEART_ICE ) )
                    dmg -= dmg * Artifact( Artifact::HEART_ICE ).ExtraValue() / 100;

                // 100%
                if ( defendingHero->HasArtifact( Artifact::HEART_FIRE ) )
                    dmg *= 2;
            }
            break;

        case Spell::FIREBALL:
        case Spell::FIREBLAST:
            // +50%
            if ( hero->HasArtifact( Artifact::EVERHOT_LAVA_ROCK ) )
                dmg += dmg * Artifact( Artifact::EVERHOT_LAVA_ROCK ).ExtraValue() / 100;

            if ( defendingHero ) {
                // -50%
                if ( defendingHero->HasArtifact( Artifact::FIRE_CLOAK ) )
                    dmg -= dmg * Artifact( Artifact::FIRE_CLOAK ).ExtraValue() / 100;

                if ( defendingHero->HasArtifact( Artifact::HEART_FIRE ) )
                    dmg -= dmg * Artifact( Artifact::HEART_FIRE ).ExtraValue() / 100;

                // 100%
                if ( defendingHero->HasArtifact( Artifact::HEART_ICE ) )
                    dmg *= 2;
            }
            break;

        case Spell::LIGHTNINGBOLT:
            // +50%
            if ( hero->HasArtifact( Artifact::LIGHTNING_ROD ) )
                dmg += dmg * Artifact( Artifact::LIGHTNING_ROD ).ExtraValue() / 100;
            // -50%
            if ( defendingHero && defendingHero->HasArtifact( Artifact::LIGHTNING_HELM ) )
                dmg -= dmg * Artifact( Artifact::LIGHTNING_HELM ).ExtraValue() / 100;
            break;

        case Spell::CHAINLIGHTNING:
            // +50%
            if ( hero->HasArtifact( Artifact::LIGHTNING_ROD ) )
                dmg += dmg * Artifact( Artifact::LIGHTNING_ROD ).ExtraValue() / 100;
            // -50%
            if ( defendingHero && defendingHero->HasArtifact( Artifact::LIGHTNING_HELM ) )
                dmg -= dmg * Artifact( Artifact::LIGHTNING_HELM ).ExtraValue() / 100;
            // update orders damage
            switch ( target.damage ) {
            case 0:
                break;
            case 1:
                dmg /= 2;
                break;
            case 2:
                dmg /= 4;
                break;
            case 3:
                dmg /= 8;
                break;
            default:
                break;
            }
            break;

        case Spell::ELEMENTALSTORM:
        case Spell::ARMAGEDDON:
            // -50%
            if ( defendingHero && defendingHero->HasArtifact( Artifact::BROACH_SHIELDING ) )
                dmg /= 2;
            break;

        default:
            break;
        }
    }

    // apply damage
    if ( dmg ) {
        target.damage = dmg;
        target.killed = ApplyDamage( dmg );
        if ( target.defender && target.defender->Modes( SP_BLIND ) )
            target.defender->ResetBlind();
    }
}

void Battle::Unit::SpellRestoreAction( const Spell & spell, u32 spoint, const HeroBase * hero )
{
    switch ( spell() ) {
    case Spell::CURE:
    case Spell::MASSCURE:
        // clear bad magic
        if ( Modes( IS_BAD_MAGIC ) ) {
            ResetModes( IS_BAD_MAGIC );
            affected.RemoveMode( IS_BAD_MAGIC );
        }
        // restore
        hp += ( spell.Restore() * spoint );
        if ( hp > ArmyTroop::GetHitPoints() )
            hp = ArmyTroop::GetHitPoints();
        break;

    case Spell::RESURRECT:
    case Spell::ANIMATEDEAD:
    case Spell::RESURRECTTRUE: {
        u32 restore = spell.Resurrect() * spoint;
        // remove from graveyard
        if ( !isValid() ) {
            // TODO: buggy behaviour
            Arena::GetGraveyard()->RemoveTroop( *this );
        }
        // restore hp
        u32 acount = hero ? hero->HasArtifact( Artifact::ANKH ) : 0;
        if ( acount )
            restore *= acount * 2;

        const u32 resurrect = Resurrect( restore, false, ( spell == Spell::RESURRECT ) );

        if ( Arena::GetInterface() ) {
            std::string str( _( "%{count} %{name} rise(s) from the dead!" ) );
            StringReplace( str, "%{count}", resurrect );
            StringReplace( str, "%{name}", GetName() );
            Arena::GetInterface()->SetStatus( str, true );
        }
    } break;

    default:
        break;
    }
}

bool Battle::Unit::isTwiceAttack( void ) const
{
    switch ( GetID() ) {
    case Monster::ELF:
    case Monster::GRAND_ELF:
    case Monster::RANGER:
        return !isHandFighting();

    default:
        break;
    }

    return ArmyTroop::isTwiceAttack();
}

bool Battle::Unit::isMagicResist( const Spell & spell, u32 spower ) const
{
    return 100 <= GetMagicResist( spell, spower );
}

u32 Battle::Unit::GetMagicResist( const Spell & spell, u32 spower ) const
{
    if ( spell.isMindInfluence() && ( isUndead() || isElemental() || GetID() == Monster::GIANT || GetID() == Monster::TITAN ) )
        return 100;

    if ( spell.isALiveOnly() && isUndead() )
        return 100;

    if ( spell.isUndeadOnly() && !isUndead() )
        return 100;

    switch ( GetID() ) {
    // 25% unfortunatly
    case Monster::DWARF:
    case Monster::BATTLE_DWARF:
        if ( spell.isDamage() || spell.isApplyToEnemies() )
            return 25;
        break;

    case Monster::GREEN_DRAGON:
    case Monster::RED_DRAGON:
    case Monster::BLACK_DRAGON:
        return 100;

    case Monster::PHOENIX:
        switch ( spell() ) {
        case Spell::COLDRAY:
        case Spell::COLDRING:
        case Spell::FIREBALL:
        case Spell::FIREBLAST:
        case Spell::LIGHTNINGBOLT:
        case Spell::CHAINLIGHTNING:
        case Spell::ELEMENTALSTORM:
            return 100;
        default:
            break;
        }
        break;

    case Monster::CRUSADER:
        switch ( spell() ) {
        case Spell::CURSE:
        case Spell::MASSCURSE:
            return 100;
        default:
            break;
        }
        break;

    case Monster::EARTH_ELEMENT:
        switch ( spell() ) {
        case Spell::METEORSHOWER:
        case Spell::LIGHTNINGBOLT:
        case Spell::CHAINLIGHTNING:
            return 100;
        default:
            break;
        }
        break;

    case Monster::AIR_ELEMENT:
        switch ( spell() ) {
        case Spell::METEORSHOWER:
            return 100;
        default:
            break;
        }
        break;

    case Monster::FIRE_ELEMENT:
        switch ( spell() ) {
        case Spell::FIREBALL:
        case Spell::FIREBLAST:
            return 100;
        default:
            break;
        }
        break;

    case Monster::WATER_ELEMENT:
        switch ( spell() ) {
        case Spell::COLDRAY:
        case Spell::COLDRING:
            return 100;
        default:
            break;
        }
        break;

    default:
        break;
    }

    switch ( spell() ) {
    case Spell::CURE:
    case Spell::MASSCURE:
        if ( !isHaveDamage() && !( modes & IS_MAGIC ) )
            return 100;
        break;

    case Spell::RESURRECT:
    case Spell::RESURRECTTRUE:
    case Spell::ANIMATEDEAD:
        if ( isElemental() || ( GetCount() == count0 ) )
            return 100;
        break;

    case Spell::DISPEL:
        if ( !( modes & IS_MAGIC ) )
            return 100;
        break;

    case Spell::HYPNOTIZE:
        if ( spell.ExtraValue() * spower < hp )
            return 100;
        break;

    default:
        break;
    }

    return 0;
}

int Battle::Unit::GetSpellMagic( bool force ) const
{
    switch ( GetID() ) {
    case Monster::UNICORN:
        // 20% blind
        if ( force || 3 > Rand::Get( 1, 10 ) )
            return Spell::BLIND;
        break;

    case Monster::CYCLOPS:
        // 20% paralyze
        if ( force || 3 > Rand::Get( 1, 10 ) )
            return Spell::PARALYZE;
        break;

    case Monster::MUMMY:
        // 20% curse
        if ( force || 3 > Rand::Get( 1, 10 ) )
            return Spell::CURSE;
        break;

    case Monster::ROYAL_MUMMY:
        // 30% curse
        if ( force || 4 > Rand::Get( 1, 10 ) )
            return Spell::CURSE;
        break;

    case Monster::ARCHMAGE:
        // 20% dispel
        if ( force || 3 > Rand::Get( 1, 10 ) )
            return Spell::DISPEL;
        break;

    case Monster::MEDUSA:
        // 20% stone
        if ( force || 3 > Rand::Get( 1, 10 ) )
            return Spell::STONE;
        break;

    default:
        break;
    }

    return Spell::NONE;
}

bool Battle::Unit::isHaveDamage( void ) const
{
    return hp < count0 * Monster::GetHitPoints();
}

int Battle::Unit::GetFrameStart( void ) const
{
    return animation.firstFrame();
}

int Battle::Unit::GetFrame( void ) const
{
    return animation.getFrame();
}

void Battle::Unit::SetDeathAnim()
{
    if ( animation.getCurrentState() != Monster_Info::KILL ) {
        SwitchAnimation( Monster_Info::KILL );
    }
    animation.setToLastFrame();
}

void Battle::Unit::SetCustomAlpha( uint32_t alpha )
{
    customAlphaMask = alpha;
}

uint32_t Battle::Unit::GetCustomAlpha() const
{
    return customAlphaMask;
}

int Battle::Unit::GetFrameCount( void ) const
{
    return animation.animationLength();
}

void Battle::Unit::IncreaseAnimFrame( bool loop )
{
    animation.playAnimation( loop );
}

bool Battle::Unit::isStartAnimFrame( void ) const
{
    return animation.isFirstFrame();
}

bool Battle::Unit::isFinishAnimFrame( void ) const
{
    return animation.isLastFrame();
}

AnimationSequence Battle::Unit::GetFrameState( int state ) const
{
    // Can't return a reference here - it will be destroyed
    return animation.getAnimationSequence( state );
}

const AnimationState & Battle::Unit::GetFrameState( void ) const
{
    return animation;
}

bool Battle::Unit::SwitchAnimation( int rule, bool reverse )
{
    animation.switchAnimation( rule, reverse );
    return animation.isValid();
}

bool Battle::Unit::SwitchAnimation( const std::vector<int> & animationList, bool reverse )
{
    animation.switchAnimation( animationList, reverse );
    return animation.isValid();
}

int Battle::Unit::M82Attk( void ) const
{
    if ( isArchers() && !isHandFighting() ) {
        switch ( GetID() ) {
        case Monster::ARCHER:
        case Monster::RANGER:
            return M82::ARCHSHOT;
        case Monster::ORC:
        case Monster::ORC_CHIEF:
            return M82::ORC_SHOT;
        case Monster::TROLL:
        case Monster::WAR_TROLL:
            return M82::TRLLSHOT;
        case Monster::ELF:
        case Monster::GRAND_ELF:
            return M82::ELF_SHOT;
        case Monster::DRUID:
        case Monster::GREATER_DRUID:
            return M82::DRUISHOT;
        case Monster::CENTAUR:
            return M82::CNTRSHOT;
        case Monster::HALFLING:
            return M82::HALFSHOT;
        case Monster::MAGE:
        case Monster::ARCHMAGE:
            return M82::MAGESHOT;
        case Monster::TITAN:
            return M82::TITNSHOT;
        case Monster::LICH:
        case Monster::POWER_LICH:
            return M82::LICHSHOT;
        default:
            break;
        }
    }

    return GetMonsterSprite().m82_attk;
}

int Battle::Unit::M82Kill( void ) const
{
    return GetMonsterSprite().m82_kill;
}

int Battle::Unit::M82Move( void ) const
{
    return GetMonsterSprite().m82_move;
}

int Battle::Unit::M82Wnce( void ) const
{
    return GetMonsterSprite().m82_wnce;
}

int Battle::Unit::M82Expl( void ) const
{
    switch ( GetID() ) {
    case Monster::VAMPIRE:
        return M82::VAMPEXT1;
    case Monster::VAMPIRE_LORD:
        return M82::VAMPEXT1;
    case Monster::LICH:
        return M82::LICHEXPL;
    case Monster::POWER_LICH:
        return M82::LICHEXPL;

    default:
        break;
    }

    return M82::UNKNOWN;
}

int Battle::Unit::ICNFile( void ) const
{
    return GetMonsterSprite().icn_file;
}

int Battle::Unit::ICNMiss( void ) const
{
    return Monster::GetMissileICN( GetID() );
}

Rect Battle::Unit::GetRectPosition( void ) const
{
    return position.GetRect();
}

Point Battle::Unit::GetBackPoint( void ) const
{
    const Rect & rt = position.GetRect();
    return reflect ? Point( rt.x + rt.w, rt.y + rt.h / 2 ) : Point( rt.x, rt.y + rt.h / 2 );
}

Point Battle::Unit::GetCenterPoint() const
{
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( GetMonsterSprite().icn_file, GetFrame() );

    const Rect & pos = position.GetRect();
    const s32 centerY = pos.y + pos.h + sprite.y() / 2 - 10;

    return Point( pos.x + pos.w / 2, centerY );
}

Point Battle::Unit::GetStartMissileOffset( size_t direction ) const
{
    return animation.getProjectileOffset( direction );
}

int Battle::Unit::GetArmyColor( void ) const
{
    return ArmyTroop::GetColor();
}

int Battle::Unit::GetColor( void ) const
{
    return GetArmyColor();
}

int Battle::Unit::GetCurrentColor() const
{
    if ( Modes( SP_BERSERKER ) )
        return -1; // be aware of unknown color
    else if ( Modes( SP_HYPNOTIZE ) )
        return GetArena()->GetOppositeColor( GetArmyColor() );

    // default
    return GetColor();
}

int Battle::Unit::GetCurrentControl() const
{
    if ( Modes( SP_BERSERKER ) )
        return CONTROL_AI; // let's say that it belongs to AI which is not present in the battle

    if ( Modes( SP_HYPNOTIZE ) ) {
        const int color = GetCurrentColor();
        if ( color == GetArena()->GetForce1().GetColor() )
            return GetArena()->GetForce1().GetControl();
        else
            return GetArena()->GetForce2().GetControl();
    }

    return GetControl();
}

const HeroBase * Battle::Unit::GetCommander( void ) const
{
    return GetArmy() ? GetArmy()->GetCommander() : NULL;
}
