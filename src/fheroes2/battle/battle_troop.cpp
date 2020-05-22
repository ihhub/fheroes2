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

void Battle::UpdateMonsterAttributes( const std::string & spec )
{
#ifdef WITH_XML
    // parse battle.xml
    TiXmlDocument doc;
    const TiXmlElement * xml_battle = NULL;

    if ( doc.LoadFile( spec.c_str() ) && NULL != ( xml_battle = doc.FirstChildElement( "battle" ) ) ) {
        const TiXmlElement * xml_element;
        int value;
    }
    else
        VERBOSE( spec << ": " << doc.ErrorDesc() );
#endif
}

void Battle::UpdateMonsterSpriteAnimation( const std::string & spec )
{
#ifdef WITH_XML
    // parse battle.xml
    TiXmlDocument doc;
    const TiXmlElement * xml_animation = NULL;

    if ( doc.LoadFile( spec.c_str() ) && NULL != ( xml_animation = doc.FirstChildElement( "animations" ) ) ) {
        const TiXmlElement * xml_icn = xml_animation->FirstChildElement( "icn" );
        for ( ; xml_icn; xml_icn = xml_icn->NextSiblingElement( "icn" ) ) {
            std::string icn_name = StringUpper( xml_icn->Attribute( "name" ) );
            // find icn name
            int icn = ICN::FromString( icn_name.c_str() );
            if ( icn == ICN::UNKNOWN )
                continue;

            // find monster info position
            Monster::monstersprite_t * ptr = Monster::GetMonsterSpireByICN( icn );
            if ( ptr->icn_file == ICN::UNKNOWN )
                continue;

            const TiXmlElement * xml_anim = xml_icn->FirstChildElement( "animation" );
            int start, count;

            for ( ; xml_anim; xml_anim = xml_anim->NextSiblingElement( "animation" ) ) {
                const char * state = xml_anim->Attribute( "state" );
                xml_anim->Attribute( "start", &start );
                xml_anim->Attribute( "count", &count );
                Monster::animframe_t frm;
                frm.start = start;
                frm.count = count;

                if ( 0 == std::strcmp( "idle", state ) )
                    ptr->frm_idle = frm;
                else if ( 0 == std::strcmp( "move", state ) )
                    ptr->frm_move = frm;
                else if ( 0 == std::strcmp( "fly1", state ) )
                    ptr->frm_fly1 = frm;
                else if ( 0 == std::strcmp( "fly2", state ) )
                    ptr->frm_fly2 = frm;
                else if ( 0 == std::strcmp( "fly3", state ) )
                    ptr->frm_fly3 = frm;
                else if ( 0 == std::strcmp( "shot0", state ) )
                    ptr->frm_shot0 = frm;
                else if ( 0 == std::strcmp( "shot1", state ) )
                    ptr->frm_shot1 = frm;
                else if ( 0 == std::strcmp( "shot2", state ) )
                    ptr->frm_shot2 = frm;
                else if ( 0 == std::strcmp( "shot3", state ) )
                    ptr->frm_shot3 = frm;
                else if ( 0 == std::strcmp( "attk0", state ) )
                    ptr->frm_attk0 = frm;
                else if ( 0 == std::strcmp( "attk1", state ) )
                    ptr->frm_attk1 = frm;
                else if ( 0 == std::strcmp( "attk2", state ) )
                    ptr->frm_attk2 = frm;
                else if ( 0 == std::strcmp( "attk3", state ) )
                    ptr->frm_attk3 = frm;
                else if ( 0 == std::strcmp( "wnce", state ) )
                    ptr->frm_wnce = frm;
                else if ( 0 == std::strcmp( "kill", state ) )
                    ptr->frm_kill = frm;
            }
        }
    }
    else
        VERBOSE( spec << ": " << doc.ErrorDesc() );
#endif
}

Battle::ModeDuration::ModeDuration()
    : std::pair<u32, u32>( 0, 0 )
{}

Battle::ModeDuration::ModeDuration( u32 mode, u32 duration )
    : std::pair<u32, u32>( mode, duration )
{}

bool Battle::ModeDuration::isMode( u32 mode ) const
{
    return ( first & mode );
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
    const_iterator it = std::find_if( begin(), end(), std::bind2nd( std::mem_fun_ref( &ModeDuration::isMode ), mode ) );
    return it == end() ? 0 : ( *it ).second;
}

void Battle::ModesAffected::AddMode( u32 mode, u32 duration )
{
    iterator it = std::find_if( begin(), end(), std::bind2nd( std::mem_fun_ref( &ModeDuration::isMode ), mode ) );
    if ( it == end() )
        push_back( ModeDuration( mode, duration ) );
    else
        ( *it ).second = duration;
}

void Battle::ModesAffected::RemoveMode( u32 mode )
{
    iterator it = std::find_if( begin(), end(), std::bind2nd( std::mem_fun_ref( &ModeDuration::isMode ), mode ) );
    if ( it != end() ) {
        // erase(it)
        if ( it + 1 != end() )
            std::swap( *it, back() );
        pop_back();
    }
}

void Battle::ModesAffected::DecreaseDuration( void )
{
    std::for_each( begin(), end(), std::mem_fun_ref( &ModeDuration::DecreaseDuration ) );
}

u32 Battle::ModesAffected::FindZeroDuration( void ) const
{
    const_iterator it = std::find_if( begin(), end(), std::mem_fun_ref( &ModeDuration::isZeroDuration ) );
    return it == end() ? 0 : ( *it ).first;
}

Battle::Unit::Unit( const Troop & t, s32 pos, bool ref )
    : ArmyTroop( NULL, t )
    , uid( World::GetUniq() )
    , hp( t.GetHitPoints() )
    , count0( t.GetCount() )
    , dead( 0 )
    , shots( t.GetShots() )
    , disruptingray( 0 )
    , reflect( ref )
    , mirror( NULL )
    , blindanswer( false )
    , animation( id )
    , idleTimer( 0 )
    , idleTimerSet( false )
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

Surface Battle::Unit::GetContour( int val ) const
{
    const int frame = GetFrame();

    switch ( val ) {
    case CONTOUR_MAIN:
        return getContour( frame, contoursMain, false, false );
    case CONTOUR_REFLECT:
        return getContour( frame, contoursReflect, true, false );
    case CONTOUR_BLACK: // TODO: really get contour for stunned unit?
        return getContour( frame, contoursWB, false, true );
    case CONTOUR_BLACK | CONTOUR_REFLECT: // TODO: really get contour for stunned unit?
        return getContour( frame, contoursWBReflect, true, true );
    default:
        break;
    }

    return Surface();
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

const Surface & Battle::Unit::getContour( const int frameId, std::map<int, Surface> & contours, const bool isReflected, const bool isBlackWhite ) const
{
    std::map<int, Surface>::iterator iter = contours.find( frameId );
    if ( iter != contours.end() )
        return iter->second;

    const monstersprite_t & msi = GetMonsterSprite();
    const Sprite sprite = AGG::GetICN( msi.icn_file, frameId, isReflected );

    if ( !sprite.isValid() ) {
        iter = contours.insert( std::make_pair( frameId, Surface() ) ).first;
        return iter->second;
    }

    if ( isBlackWhite )
        iter = contours.insert( std::make_pair( frameId, sprite.RenderGrayScale() ) ).first;
    else
        iter = contours.insert( std::make_pair( frameId, sprite.RenderContour( RGBA( 0xe0, 0xe0, 0 ) ) ) ).first;

    return iter->second;
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

    if ( morale > 0 && Rand::Get( 1, 24 ) <= morale ) {
        SetModes( MORALE_GOOD );
    }
    else if ( morale < 0 && Rand::Get( 1, 12 ) <= -morale ) {
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
    s32 luck = GetLuck();
    u32 chance = Rand::Get( 1, 24 );

    if ( luck > 0 && chance <= luck ) {
        SetModes( LUCK_GOOD );
    }
    else if ( luck < 0 && chance <= -luck ) {
        SetModes( LUCK_BAD );
    }

    // Bless, Curse and Luck do stack
}

bool Battle::Unit::isFly( void ) const
{
    return ArmyTroop::isFly() && !Modes( SP_SLOW );
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
    return a.isValid() && !a.Modes( CAP_TOWER ) && b.isValid() && b.GetColor() != a.GetColor()
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
    if ( !idleTimerSet ) {
        const uint32_t halfDelay = animation.getIdleDelay() / 2;
        idleTimer.second = Rand::Get( 0, halfDelay / 2 ) + halfDelay * 3 / 2;
        idleTimerSet = true;
    }
    const bool res = idleTimer.Trigger();
    if ( res )
        idleTimerSet = false;
    return res;
}

void Battle::Unit::NewTurn( void )
{
    if ( isResurectLife() )
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
            if ( Arena::GetInterface() )
                Arena::GetInterface()->RedrawActionRemoveMirrorImage( *mirror );

            mirror->SetCount( 0 );
            mirror = NULL;
        }
    }
}

u32 Battle::Unit::GetSpeed( bool skip_standing_check ) const
{
    if ( !skip_standing_check && ( !GetCount() || Modes( TR_MOVED | SP_BLIND | IS_PARALYZE_MAGIC ) ) )
        return Speed::STANDING;

    int speed = Monster::GetSpeed();
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

u32 Battle::Unit::GetDamageMin( const Unit & enemy ) const
{
    return CalculateDamageUnit( enemy, ArmyTroop::GetDamageMin() );
}

u32 Battle::Unit::GetDamageMax( const Unit & enemy ) const
{
    return CalculateDamageUnit( enemy, ArmyTroop::GetDamageMax() );
}

u32 Battle::Unit::CalculateDamageUnit( const Unit & enemy, float dmg ) const
{
    if ( isArchers() ) {
        if ( isHandFighting() ) {
            switch ( GetID() ) {
            // skip
            case Monster::MAGE:
            case Monster::ARCHMAGE:
            case Monster::TITAN:
                break;

            default:
                dmg /= 2;
                break;
            }
        }
        else {
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
        res = GetDamageMax( enemy );
    else if ( Modes( SP_CURSE ) )
        res = GetDamageMin( enemy );
    else
        res = Rand::Get( GetDamageMin( enemy ), GetDamageMax( enemy ) );

    if ( Modes( LUCK_GOOD ) )
        res <<= 1; // mul 2
    else if ( Modes( LUCK_BAD ) )
        res >>= 1; // div 2

    return res;
}

u32 Battle::Unit::HowManyCanKill( const Unit & b ) const
{
    return b.HowManyWillKilled( ( GetDamageMin( b ) + GetDamageMax( b ) ) / 2 );
}

u32 Battle::Unit::HowManyWillKilled( u32 dmg ) const
{
    return dmg >= hp ? GetCount() : GetCount() - Monster::GetCountFromHitPoints( *this, hp - dmg );
}

u32 Battle::Unit::ApplyDamage( u32 dmg )
{
    if ( dmg && GetCount() ) {
        u32 killed = HowManyWillKilled( dmg );

        // kill mirror image (slave)
        if ( Modes( CAP_MIRRORIMAGE ) ) {
            if ( Arena::GetInterface() )
                Arena::GetInterface()->RedrawActionRemoveMirrorImage( *this );
            mirror->ResetModes( CAP_MIRROROWNER );
            dmg = hp;
            killed = GetCount();
            mirror = NULL;
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

        if ( !isValid() )
            PostKilledAction();

        return killed;
    }

    return 0;
}

void Battle::Unit::PostKilledAction( void )
{
    // kill mirror image (master)
    if ( Modes( CAP_MIRROROWNER ) ) {
        if ( Arena::GetInterface() )
            Arena::GetInterface()->RedrawActionRemoveMirrorImage( *mirror );
        modes = 0;
        mirror->hp = 0;
        mirror->SetCount( 0 );
        mirror->mirror = NULL;
        mirror = NULL;
        ResetModes( CAP_MIRROROWNER );
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
            resurrect = killed * static_cast<Monster>( enemy ).GetHitPoints();
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

bool Battle::Unit::AllowApplySpell( const Spell & spell, const HeroBase * hero, std::string * msg ) const
{
    if ( Modes( SP_ANTIMAGIC ) )
        return false;

    if ( ( Modes( CAP_MIRRORIMAGE ) || Modes( CAP_MIRROROWNER ) ) && ( spell == Spell::ANTIMAGIC || spell == Spell::MIRRORIMAGE ) )
        return false;

    // check global
    // if(GetArena()->DisableCastSpell(spell, msg)) return false; // disable - recursion!

    if ( hero && spell.isApplyToFriends() && GetColor() != hero->GetColor() )
        return false;
    if ( hero && spell.isApplyToEnemies() && GetColor() == hero->GetColor() )
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
    if ( !AllowApplySpell( spell, hero ) )
        return false;

    DEBUG( DBG_BATTLE, DBG_TRACE, spell.GetName() << " to " << String() );

    u32 spoint = hero ? hero->GetPower() : 3;

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
    if ( isAlwayResponse() )
        return true;

    if ( !Modes( TR_RESPONSED ) ) {
        if ( Modes( SP_BLIND ) )
            return blindanswer;
        else
            return !Modes( IS_PARALYZE_MAGIC );
    }

    return false;
}

void Battle::Unit::SetResponse( void )
{
    SetModes( TR_RESPONSED );
}

void Battle::Unit::PostAttackAction( Unit & enemy )
{
    switch ( GetID() ) {
    case Monster::ARCHMAGE:
        // 20% clean magic state
        if ( enemy.isValid() && enemy.Modes( IS_GOOD_MAGIC ) && 3 > Rand::Get( 1, 10 ) )
            enemy.ResetModes( IS_GOOD_MAGIC );
        break;

    default:
        break;
    }

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

    if ( GetArena()->GetArmyColor2() == GetColor() && GetArena()->GetForce2().Modes( ARMY_GUARDIANS_OBJECT ) )
        res += 2;

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
    if ( Board::isMoatIndex( GetHeadIndex() ) || Board::isMoatIndex( GetTailIndex() ) )
        res -= GameStatic::GetBattleMoatReduceDefense();

    return res;
}

s32 Battle::Unit::GetScoreQuality( const Unit & defender ) const
{
    const Unit & attacker = *this;

    // initial value: (hitpoints)
    const u32 & damage = ( attacker.GetDamageMin( defender ) + attacker.GetDamageMax( defender ) ) / 2;
    const u32 & kills = defender.HowManyWillKilled( attacker.isTwiceAttack() ? damage * 2 : damage );
    double res = kills * static_cast<Monster>( defender ).GetHitPoints();
    bool noscale = false;

    // attacker
    switch ( attacker.GetID() ) {
    case Monster::GHOST:
        // priority: from killed only
        noscale = true;
        break;

    case Monster::VAMPIRE_LORD:
        if ( attacker.isHaveDamage() ) {
            // alive priority
            if ( defender.isElemental() || defender.isUndead() )
                res /= 2;
        }
        break;

    default:
        break;
    }

    // scale on ability
    if ( !noscale ) {
        if ( defender.isArchers() )
            res += res * 0.7;
        if ( defender.isFly() )
            res += res * 0.6;
        if ( defender.isHideAttack() )
            res += res * 0.5;
        if ( defender.isTwiceAttack() )
            res += res * 0.4;
        if ( defender.isResurectLife() )
            res += res * 0.3;
        if ( defender.isDoubleCellAttack() )
            res += res * 0.3;
        if ( defender.isAlwayResponse() )
            res -= res * 0.5;
    }

    // extra
    if ( defender.Modes( CAP_MIRRORIMAGE ) )
        res += res * 0.7;
    if ( !attacker.isArchers() ) {
        if ( defender.Modes( TR_RESPONSED ) )
            res += res * 0.3;
        else {
            if ( defender.Modes( LUCK_BAD ) )
                res += res * 0.3;
            else if ( defender.Modes( LUCK_GOOD ) )
                res -= res * 0.3;
        }
    }

    return static_cast<s32>( res ) > 1 ? static_cast<u32>( res ) : 1;
}

u32 Battle::Unit::GetHitPoints( void ) const
{
    return hp;
}

int Battle::Unit::GetControl( void ) const
{
    return Modes( SP_BERSERKER ) || !GetArmy() ? CONTROL_AI : GetArmy()->GetControl();
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
        ResetModes( LUCK_GOOD );
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
        ResetModes( LUCK_BAD );
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
        const HeroBase * myhero = GetCommander();
        u32 acount = 0;

        switch ( spell() ) {
        case Spell::COLDRAY:
        case Spell::COLDRING:
            // +50%
            acount = hero->HasArtifact( Artifact::EVERCOLD_ICICLE );
            if ( acount )
                dmg += dmg * acount * Artifact( Artifact::EVERCOLD_ICICLE ).ExtraValue() / 100;
            acount = hero->HasArtifact( Artifact::EVERCOLD_ICICLE );
            if ( acount )
                dmg += dmg * acount * Artifact( Artifact::EVERCOLD_ICICLE ).ExtraValue() / 100;
            // -50%
            acount = myhero ? myhero->HasArtifact( Artifact::ICE_CLOAK ) : 0;
            if ( acount )
                dmg /= acount * 2;
            acount = myhero ? myhero->HasArtifact( Artifact::HEART_ICE ) : 0;
            if ( acount )
                dmg -= dmg * acount * Artifact( Artifact::HEART_ICE ).ExtraValue() / 100;
            // 100%
            acount = myhero ? myhero->HasArtifact( Artifact::HEART_FIRE ) : 0;
            if ( acount )
                dmg *= acount * 2;
            break;

        case Spell::FIREBALL:
        case Spell::FIREBLAST:
            // +50%
            acount = hero->HasArtifact( Artifact::EVERHOT_LAVA_ROCK );
            if ( acount )
                dmg += dmg * acount * Artifact( Artifact::EVERHOT_LAVA_ROCK ).ExtraValue() / 100;
            // -50%
            acount = myhero ? myhero->HasArtifact( Artifact::FIRE_CLOAK ) : 0;
            if ( acount )
                dmg /= acount * 2;
            acount = myhero ? myhero->HasArtifact( Artifact::HEART_FIRE ) : 0;
            if ( acount )
                dmg -= dmg * acount * Artifact( Artifact::HEART_FIRE ).ExtraValue() / 100;
            // 100%
            acount = myhero ? myhero->HasArtifact( Artifact::HEART_ICE ) : 0;
            if ( acount )
                dmg *= acount * 2;
            break;

        case Spell::LIGHTNINGBOLT:
            // +50%
            acount = hero->HasArtifact( Artifact::LIGHTNING_ROD );
            if ( acount )
                dmg += dmg * acount * Artifact( Artifact::LIGHTNING_ROD ).ExtraValue() / 100;
            // -50%
            acount = myhero ? myhero->HasArtifact( Artifact::LIGHTNING_HELM ) : 0;
            if ( acount )
                dmg /= acount * 2;
            break;

        case Spell::CHAINLIGHTNING:
            // +50%
            acount = hero->HasArtifact( Artifact::LIGHTNING_ROD );
            if ( acount )
                dmg += acount * dmg / 2;
            // -50%
            acount = myhero ? myhero->HasArtifact( Artifact::LIGHTNING_HELM ) : 0;
            if ( acount )
                dmg /= acount * 2;
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
            acount = myhero ? myhero->HasArtifact( Artifact::BROACH_SHIELDING ) : 0;
            if ( acount )
                dmg /= acount * 2;
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
            SwitchAnimation( Monster_Info::KILL, true );
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

    if ( Settings::Get().ExtBattleMagicTroopCanResist() && spell == GetSpellMagic( true ) )
        return 20;

    switch ( GetID() ) {
    case Monster::ARCHMAGE:
        if ( Settings::Get().ExtBattleArchmageCanResistBadMagic() && ( spell.isDamage() || spell.isApplyToEnemies() ) )
            return 20;
        break;

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

bool Battle::Unit::isMagicAttack( void ) const
{
    return GetSpellMagic( true ) != Spell::NONE;
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

        /* skip: see Unit::PostAttackAction
    case Monster::ARCHMAGE:
            // 20% dispel
            if(!force && 3 > Rand::Get(1, 10)) return Spell::DISPEL;
            break;
    */

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
    const monstersprite_t & msi = GetMonsterSprite();

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
    switch ( GetID() ) {
    case Monster::ARCHER:
        return ICN::ARCH_MSL;
    case Monster::RANGER:
        return ICN::ARCH_MSL;
    case Monster::ORC:
        return ICN::ORC__MSL;
    case Monster::ORC_CHIEF:
        return ICN::ORC__MSL;
    case Monster::TROLL:
        return ICN::TROLLMSL;
    case Monster::WAR_TROLL:
        return ICN::TROLLMSL;
    case Monster::ELF:
        return ICN::ELF__MSL;
    case Monster::GRAND_ELF:
        return ICN::ELF__MSL;
    case Monster::DRUID:
        return ICN::DRUIDMSL;
    case Monster::GREATER_DRUID:
        return ICN::DRUIDMSL;
    case Monster::CENTAUR:
        return ICN::ARCH_MSL;
    case Monster::HALFLING:
        return ICN::HALFLMSL;
    case Monster::MAGE:
        return ICN::DRUIDMSL;
    case Monster::ARCHMAGE:
        return ICN::DRUIDMSL;
    case Monster::TITAN:
        return ICN::TITANMSL;
    case Monster::LICH:
        return ICN::LICH_MSL;
    case Monster::POWER_LICH:
        return ICN::LICH_MSL;

    default:
        break;
    }

    return ICN::UNKNOWN;
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

int Battle::Unit::GetStartMissileOffset( int state ) const
{
    switch ( GetID() ) {
    case Monster::ARCHER:
    case Monster::RANGER:
        switch ( state ) {
        case Monster_Info::RANG_TOP:
            return -15;
        case Monster_Info::RANG_FRONT:
            return -3;
        case Monster_Info::RANG_BOT:
            return 10;
        default:
            break;
        }
        break;

    case Monster::ORC:
    case Monster::ORC_CHIEF:
        return 5;

    case Monster::TROLL:
    case Monster::WAR_TROLL:
        return -20;

    case Monster::LICH:
    case Monster::POWER_LICH:
        switch ( state ) {
        case Monster_Info::RANG_TOP:
            return -30;
        case Monster_Info::RANG_FRONT:
            return -20;
        case Monster_Info::RANG_BOT:
            return 0;
        default:
            break;
        }
        break;

    case Monster::ELF:
    case Monster::GRAND_ELF:
        switch ( state ) {
        case Monster_Info::RANG_TOP:
            return -5;
        case Monster_Info::RANG_FRONT:
            return 0;
        case Monster_Info::RANG_BOT:
            return 5;
        default:
            break;
        }
        break;

    case Monster::CENTAUR:
        switch ( state ) {
        case Monster_Info::RANG_TOP:
            return -20;
        case Monster_Info::RANG_FRONT:
            return -10;
        case Monster_Info::RANG_BOT:
            return 5;
        default:
            break;
        }
        break;

    case Monster::DRUID:
    case Monster::GREATER_DRUID:
        switch ( state ) {
        case Monster_Info::RANG_TOP:
            return -20;
        case Monster_Info::RANG_FRONT:
            return -5;
        case Monster_Info::RANG_BOT:
            return 15;
        default:
            break;
        }
        break;

    case Monster::HALFLING:
        switch ( state ) {
        case Monster_Info::RANG_TOP:
            return -20;
        case Monster_Info::RANG_FRONT:
            return 10;
        case Monster_Info::RANG_BOT:
            return 20;
        default:
            break;
        }
        break;

    case Monster::MAGE:
    case Monster::ARCHMAGE:
        switch ( state ) {
        case Monster_Info::RANG_TOP:
            return -40;
        case Monster_Info::RANG_FRONT:
            return -10;
        case Monster_Info::RANG_BOT:
            return 25;
        default:
            break;
        }
        break;

    case Monster::TITAN:
        switch ( state ) {
        case Monster_Info::RANG_TOP:
            return -80;
        case Monster_Info::RANG_FRONT:
            return -20;
        case Monster_Info::RANG_BOT:
            return 15;
        default:
            break;
        }
        break;

    default:
        break;
    }

    return 0;
}

int Battle::Unit::GetArmyColor( void ) const
{
    return ArmyTroop::GetColor();
}

int Battle::Unit::GetColor( void ) const
{
    if ( Modes( SP_BERSERKER ) )
        return 0;
    else if ( Modes( SP_HYPNOTIZE ) )
        return GetArena()->GetOppositeColor( GetArmyColor() );

    // default
    return GetArmyColor();
}

const HeroBase * Battle::Unit::GetCommander( void ) const
{
    return GetArmy() ? GetArmy()->GetCommander() : NULL;
}
