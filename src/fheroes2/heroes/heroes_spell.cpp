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

#include "agg.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_interface.h"
#include "heroes.h"
#include "interface_list.h"
#include "kingdom.h"
#include "m82.h"
#include "monster.h"
#include "settings.h"
#include "spell.h"
#include "world.h"

namespace
{
    // Values are extracted from Heroes2 executable
    const uint32_t dimensionDoorPenalty = 225;
    const uint32_t townGatePenalty = 225;
}

void DialogSpellFailed( const Spell & );
void DialogNotAvailable( void );

bool ActionSpellViewMines( Heroes & );
bool ActionSpellViewResources( Heroes & );
bool ActionSpellViewArtifacts( Heroes & );
bool ActionSpellViewTowns( Heroes & );
bool ActionSpellViewHeroes( Heroes & );
bool ActionSpellViewAll( Heroes & );
bool ActionSpellIdentifyHero( Heroes & );
bool ActionSpellSummonBoat( Heroes & );
bool ActionSpellDimensionDoor( Heroes & );
bool ActionSpellTownGate( Heroes & );
bool ActionSpellTownPortal( Heroes & );
bool ActionSpellVisions( Heroes & );
bool ActionSpellSetGuardian( Heroes &, const Spell & );

class CastleIndexListBox : public Interface::ListBox<s32>
{
public:
    CastleIndexListBox( const Point & pt, int & res )
        : Interface::ListBox<s32>( pt )
        , result( res ){};

    void RedrawItem( const s32 &, s32, s32, bool );
    void RedrawBackground( const Point & );

    void ActionCurrentUp( void ){};
    void ActionCurrentDn( void ){};
    void ActionListDoubleClick( s32 & )
    {
        result = Dialog::OK;
    };
    void ActionListSingleClick( s32 & ){};
    void ActionListPressRight( s32 & ){};

    int & result;
};

void CastleIndexListBox::RedrawItem( const s32 & index, s32 dstx, s32 dsty, bool current )
{
    const Castle * castle = world.GetCastle( Maps::GetPoint( index ) );

    if ( castle ) {
        Text text( castle->GetName(), ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 10, dsty );
    }
}

void CastleIndexListBox::RedrawBackground( const Point & dst )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    Text text( _( "Town Portal" ), Font::YELLOW_BIG );
    text.Blit( dst.x + 140 - text.w() / 2, dst.y + 6 );

    text.Set( _( "Select town to port to." ), Font::BIG );
    text.Blit( dst.x + 140 - text.w() / 2, dst.y + 30 );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 0 ), display, dst.x + 2, dst.y + 55 );
    for ( u32 ii = 1; ii < 5; ++ii )
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 1 ), display, dst.x + 2, dst.y + 55 + ( ii * 19 ) );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 2 ), display, dst.x + 2, dst.y + 145 );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 7 ), display, dst.x + 256, dst.y + 75 );
    for ( u32 ii = 1; ii < 3; ++ii )
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 8 ), display, dst.x + 256, dst.y + 74 + ( ii * 19 ) );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 9 ), display, dst.x + 256, dst.y + 126 );
}

bool Heroes::ActionSpellCast( const Spell & spell )
{
    std::string error;

    if ( !CanMove() && ( spell == Spell::DIMENSIONDOOR || spell == Spell::TOWNGATE || spell == Spell::TOWNPORTAL ) ) {
        Dialog::Message( "", _( "Your hero is too tired to cast this spell today. Try again tomorrow." ), Font::BIG, Dialog::OK );
        return false;
    }
    else if ( spell == Spell::NONE || spell.isCombat() || !CanCastSpell( spell, &error ) ) {
        if ( error.size() )
            Dialog::Message( "Error", error, Font::BIG, Dialog::OK );
        return false;
    }

    bool apply = false;

    switch ( spell() ) {
    case Spell::VIEWMINES:
        apply = ActionSpellViewMines( *this );
        break;
    case Spell::VIEWRESOURCES:
        apply = ActionSpellViewResources( *this );
        break;
    case Spell::VIEWARTIFACTS:
        apply = ActionSpellViewArtifacts( *this );
        break;
    case Spell::VIEWTOWNS:
        apply = ActionSpellViewTowns( *this );
        break;
    case Spell::VIEWHEROES:
        apply = ActionSpellViewHeroes( *this );
        break;
    case Spell::VIEWALL:
        apply = ActionSpellViewAll( *this );
        break;
    case Spell::IDENTIFYHERO:
        apply = ActionSpellIdentifyHero( *this );
        break;
    case Spell::SUMMONBOAT:
        apply = ActionSpellSummonBoat( *this );
        break;
    case Spell::DIMENSIONDOOR:
        apply = ActionSpellDimensionDoor( *this );
        break;
    case Spell::TOWNGATE:
        apply = isShipMaster() ? false : ActionSpellTownGate( *this );
        break;
    case Spell::TOWNPORTAL:
        apply = isShipMaster() ? false : ActionSpellTownPortal( *this );
        break;
    case Spell::VISIONS:
        apply = ActionSpellVisions( *this );
        break;
    case Spell::HAUNT:
        apply = ActionSpellSetGuardian( *this, spell );
        break;
    case Spell::SETEGUARDIAN:
        apply = ActionSpellSetGuardian( *this, spell );
        break;
    case Spell::SETAGUARDIAN:
        apply = ActionSpellSetGuardian( *this, spell );
        break;
    case Spell::SETFGUARDIAN:
        apply = ActionSpellSetGuardian( *this, spell );
        break;
    case Spell::SETWGUARDIAN:
        apply = ActionSpellSetGuardian( *this, spell );
        break;
    default:
        break;
    }

    if ( apply ) {
        DEBUG( DBG_GAME, DBG_INFO, GetName() << " cast spell: " << spell.GetName() );
        SpellCasted( spell );
        return true;
    }
    return false;
}

bool HeroesTownGate( Heroes & hero, const Castle * castle )
{
    if ( castle ) {
        Interface::Basic & I = Interface::Basic::Get();

        const s32 src = hero.GetIndex();
        const s32 dst = castle->GetIndex();

        if ( !Maps::isValidAbsIndex( src ) || !Maps::isValidAbsIndex( dst ) )
            return false;

        AGG::PlaySound( M82::KILLFADE );
        hero.GetPath().Hide();
        hero.FadeOut();

        Cursor::Get().Hide();
        hero.ApplyPenaltyMovement( townGatePenalty );
        hero.Move2Dest( dst );

        I.GetGameArea().SetCenter( hero.GetCenter() );
        I.RedrawFocus();
        I.Redraw();

        AGG::PlaySound( M82::KILLFADE );
        hero.FadeIn();
        hero.GetPath().Reset();
        hero.GetPath().Show(); // Reset method sets Hero's path to hidden mode with non empty path, we have to set it back

        I.SetFocus( &hero );

        // educate spells
        castle->MageGuildEducateHero( hero );

        return true;
    }
    return false;
}

void DialogSpellFailed( const Spell & spell )
{
    // failed
    std::string str = _( "%{spell} failed!!!" );
    StringReplace( str, "%{spell}", spell.GetName() );
    Dialog::Message( "", str, Font::BIG, Dialog::OK );
}

void DialogNotAvailable( void )
{
    Dialog::Message( "", "Not available for current version", Font::BIG, Dialog::OK );
}

bool ActionSpellViewMines( Heroes & )
{
    DialogNotAvailable();
    return false;
}

bool ActionSpellViewResources( Heroes & )
{
    DialogNotAvailable();
    return false;
}

bool ActionSpellViewArtifacts( Heroes & )
{
    DialogNotAvailable();
    return false;
}

bool ActionSpellViewTowns( Heroes & )
{
    DialogNotAvailable();
    return false;
}

bool ActionSpellViewHeroes( Heroes & )
{
    DialogNotAvailable();
    return false;
}

bool ActionSpellViewAll( Heroes & )
{
    DialogNotAvailable();
    return false;
}

bool ActionSpellIdentifyHero( Heroes & hero )
{
    if ( hero.GetKingdom().Modes( Kingdom::IDENTIFYHERO ) ) {
        Message( "", _( "This spell is already in use." ), Font::BIG, Dialog::OK );
        return false;
    }

    hero.GetKingdom().SetModes( Kingdom::IDENTIFYHERO );
    Message( "", _( "Enemy heroes are now fully identifiable." ), Font::BIG, Dialog::OK );

    return true;
}

bool ActionSpellSummonBoat( Heroes & hero )
{
    if ( hero.isShipMaster() ) {
        Dialog::Message( "", _( "This spell cannot be used on a boat." ), Font::BIG, Dialog::OK );
        return false;
    }

    const s32 center = hero.GetIndex();

    // find water
    s32 dst_water = -1;
    const MapsIndexes & v = Maps::ScanAroundObject( center, MP2::OBJ_ZERO );
    for ( MapsIndexes::const_iterator it = v.begin(); it != v.end(); ++it ) {
        if ( world.GetTiles( *it ).isWater() ) {
            dst_water = *it;
            break;
        }
    }

    if ( !Maps::isValidAbsIndex( dst_water ) ) {
        Dialog::Message( "", _( "This spell can be casted only nearby water." ), Font::BIG, Dialog::OK );
        return false;
    }

    u32 chance = 0;

    switch ( hero.GetLevelSkill( Skill::Secondary::WISDOM ) ) {
    case Skill::Level::BASIC:
        chance = 50;
        break;
    case Skill::Level::ADVANCED:
        chance = 75;
        break;
    case Skill::Level::EXPERT:
        chance = 100;
        break;
    default:
        chance = 30;
        break;
    }

    const MapsIndexes & boats = Maps::GetObjectPositions( center, MP2::OBJ_BOAT, false );
    for ( size_t i = 0; i < boats.size(); ++i ) {
        const s32 boat = boats[i];
        if ( Maps::isValidAbsIndex( boat ) ) {
            if ( Rand::Get( 1, 100 ) <= chance ) {
                world.GetTiles( boat ).SetObject( MP2::OBJ_ZERO );
                Game::ObjectFadeAnimation::Set( Game::ObjectFadeAnimation::Info( 27, 18, dst_water, 0, false ) );
                return true;
            }
            break;
        }
    }

    DialogSpellFailed( Spell::SUMMONBOAT );
    return true;
}

bool ActionSpellDimensionDoor( Heroes & hero )
{
    const u32 distance = Spell::CalculateDimensionDoorDistance( hero.GetPower(), hero.GetArmy().GetHitPoints() );

    Interface::Basic & I = Interface::Basic::Get();
    Cursor & cursor = Cursor::Get();

    // center hero
    cursor.Hide();
    I.GetGameArea().SetCenter( hero.GetCenter() );
    I.RedrawFocus();
    I.Redraw();

    const s32 src = hero.GetIndex();
    // get destination
    const s32 dst = I.GetDimensionDoorDestination( src, distance, hero.isShipMaster() );

    if ( Maps::isValidAbsIndex( src ) && Maps::isValidAbsIndex( dst ) ) {
        AGG::PlaySound( M82::KILLFADE );
        hero.FadeOut();

        hero.SpellCasted( Spell::DIMENSIONDOOR );

        cursor.Hide();
        hero.ApplyPenaltyMovement( dimensionDoorPenalty );
        hero.Move2Dest( dst );

        I.GetGameArea().SetCenter( hero.GetCenter() );
        I.RedrawFocus();
        I.Redraw();

        AGG::PlaySound( M82::KILLFADE );
        hero.FadeIn();
        hero.GetPath().Reset();
        hero.GetPath().Show(); // Reset method sets Hero's path to hidden mode with non empty path, we have to set it back

        hero.ActionNewPosition();

        Interface::Basic::Get().ResetFocus( GameFocus::HEROES );

        return false; /* SpellCasted apply */
    }

    return false;
}

bool ActionSpellTownGate( Heroes & hero )
{
    const Kingdom & kingdom = hero.GetKingdom();
    const KingdomCastles & castles = kingdom.GetCastles();
    KingdomCastles::const_iterator it;

    const Castle * castle = NULL;
    const s32 center = hero.GetIndex();
    s32 min = -1;

    // find the nearest castle
    for ( it = castles.begin(); it != castles.end(); ++it )
        if ( *it ) {
            int min2 = Maps::GetApproximateDistance( center, ( *it )->GetIndex() );
            if ( 0 > min || min2 < min ) {
                min = min2;
                castle = *it;
            }
        }

    Interface::Basic & I = Interface::Basic::Get();
    Cursor & cursor = Cursor::Get();

    // center hero
    cursor.Hide();
    I.GetGameArea().SetCenter( hero.GetCenter() );
    I.RedrawFocus();
    I.Redraw();

    if ( !castle ) {
        Dialog::Message( "", _( "No available towns.\nSpell Failed!!!" ), Font::BIG, Dialog::OK );
        return false;
    }
    else if ( castle->GetHeroes().Guest() && castle->GetHeroes().Guest() != &hero ) {
        Dialog::Message( "", _( "Nearest town occupied.\nSpell Failed!!!" ), Font::BIG, Dialog::OK );
        return false;
    }

    return HeroesTownGate( hero, castle );
}

bool ActionSpellTownPortal( Heroes & hero )
{
    const Kingdom & kingdom = hero.GetKingdom();
    std::vector<s32> castles;

    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    for ( KingdomCastles::const_iterator it = kingdom.GetCastles().begin(); it != kingdom.GetCastles().end(); ++it )
        if ( *it && !( *it )->GetHeroes().Guest() )
            castles.push_back( ( **it ).GetIndex() );

    if ( castles.empty() ) {
        Dialog::Message( "", _( "No available towns.\nSpell Failed!!!" ), Font::BIG, Dialog::OK );
        return false;
    }

    Dialog::FrameBorder * frameborder = new Dialog::FrameBorder( Size( 280, 200 ) );

    const Rect & area = frameborder->GetArea();
    int result = Dialog::ZERO;

    CastleIndexListBox listbox( area, result );

    listbox.RedrawBackground( area );
    listbox.SetScrollButtonUp( ICN::LISTBOX, 3, 4, fheroes2::Point( area.x + 256, area.y + 55 ) );
    listbox.SetScrollButtonDn( ICN::LISTBOX, 5, 6, fheroes2::Point( area.x + 256, area.y + 145 ) );
    listbox.SetScrollSplitter( fheroes2::AGG::GetICN( ICN::LISTBOX, 10 ), fheroes2::Rect( area.x + 260, area.y + 78, 14, 64 ) );
    listbox.SetAreaMaxItems( 5 );
    listbox.SetAreaItems( fheroes2::Rect( area.x + 10, area.y + 60, 250, 100 ) );
    listbox.SetListContent( castles );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( fheroes2::Rect( area.x, area.y, area.w, area.h ), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    cursor.Show();
    display.render();

    while ( result == Dialog::ZERO && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !cursor.isVisible() ) {
            listbox.Redraw();
            cursor.Show();
            display.render();
        }
    }

    delete frameborder;

    // store
    if ( result == Dialog::OK )
        return HeroesTownGate( hero, world.GetCastle( Maps::GetPoint( listbox.GetCurrent() ) ) );

    return false;
}

bool ActionSpellVisions( Heroes & hero )
{
    const u32 dist = hero.GetVisionsDistance();
    const MapsIndexes & monsters = Maps::ScanAroundObject( hero.GetIndex(), dist, MP2::OBJ_MONSTER );

    if ( monsters.size() ) {
        for ( MapsIndexes::const_iterator it = monsters.begin(); it != monsters.end(); ++it ) {
            const Maps::Tiles & tile = world.GetTiles( *it );
            MapMonster * map_troop = NULL;
            if ( tile.GetObject() == MP2::OBJ_MONSTER )
                map_troop = dynamic_cast<MapMonster *>( world.GetMapObject( tile.GetObjectUID() ) );

            Troop troop = map_troop ? map_troop->QuantityTroop() : tile.QuantityTroop();
            JoinCount join = Army::GetJoinSolution( hero, tile, troop );

            Funds cost;
            std::string hdr, msg;

            hdr = std::string( "%{count} " ) + StringLower( troop.GetPluralName( join.second ) );
            StringReplace( hdr, "%{count}", troop.GetCount() );

            switch ( join.first ) {
            default:
                msg = _( "I fear these creatures are in the mood for a fight." );
                break;

            case JOIN_FREE:
                msg = _( "The creatures are willing to join us!" );
                break;

            case JOIN_COST:
                if ( join.second == troop.GetCount() )
                    msg = _( "All the creatures will join us..." );
                else {
                    msg = _n( "The creature will join us...", "%{count} of the creatures will join us...", join.second );
                    StringReplace( msg, "%{count}", join.second );
                }
                msg.append( "\n" );
                msg.append( "\n for a fee of %{gold} gold." );
                StringReplace( msg, "%{gold}", troop.GetCost().gold );
                break;

            case JOIN_FLEE:
                msg = _( "These weak creatures will surely flee before us." );
                break;
            }

            Dialog::Message( hdr, msg, Font::BIG, Dialog::OK );
        }
    }
    else {
        std::string msg = _( "You must be within %{count} spaces of a monster for the Visions spell to work." );
        StringReplace( msg, "%{count}", dist );
        Dialog::Message( "", msg, Font::BIG, Dialog::OK );
        return false;
    }

    hero.SetModes( Heroes::VISIONS );

    return true;
}

bool ActionSpellSetGuardian( Heroes & hero, const Spell & spell )
{
    Maps::Tiles & tile = world.GetTiles( hero.GetIndex() );

    if ( MP2::OBJ_MINES != tile.GetObject( false ) ) {
        Dialog::Message( "", _( "You must be standing on the entrance to a mine (sawmills and alchemists don't count) to cast this spell." ), Font::BIG, Dialog::OK );
        return false;
    }

    const u32 count = hero.GetPower() * spell.ExtraValue();

    if ( count ) {
        tile.SetQuantity3( spell() );

        if ( spell == Spell::HAUNT ) {
            world.CaptureObject( tile.GetIndex(), Color::UNUSED );
            hero.SetMapsObject( MP2::OBJ_ABANDONEDMINE );
        }

        world.GetCapturedObject( tile.GetIndex() ).GetTroop().Set( Monster( spell ), count );
        return true;
    }

    return false;
}
