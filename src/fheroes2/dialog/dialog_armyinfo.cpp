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
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army_troop.h"
#include "battle.h"
#include "battle_cell.h"
#include "cursor.h"
#include "dialog.h" // IWYU pragma: associated
#include "game_delays.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "luck.h"
#include "math_base.h"
#include "monster.h"
#include "monster_anim.h"
#include "monster_info.h"
#include "morale.h"
#include "pal.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    const int offsetXAmountBox = 80;
    const int offsetYAmountBox = 223;
    const int widthAmountBox = 125;
    const int heightAmountBox = 23;

    struct SpellInfo
    {
        SpellInfo( const uint32_t mode_, const uint32_t duration_, const int32_t offset_, const int32_t space_ )
            : mode( mode_ )
            , duration( duration_ )
            , offset( offset_ )
            , space( space_ )
        {}

        uint32_t mode;
        uint32_t duration;
        int32_t offset;
        int32_t space;
        Spell spell;
    };

    Spell modeToSpell( const uint32_t modeId )
    {
        switch ( modeId ) {
        case Battle::SP_BLOODLUST:
            return Spell::BLOODLUST;
        case Battle::SP_BLESS:
            return Spell::BLESS;
        case Battle::SP_HASTE:
            return Spell::HASTE;
        case Battle::SP_SHIELD:
            return Spell::SHIELD;
        case Battle::SP_STONESKIN:
            return Spell::STONESKIN;
        case Battle::SP_DRAGONSLAYER:
            return Spell::DRAGONSLAYER;
        case Battle::SP_STEELSKIN:
            return Spell::STEELSKIN;
        case Battle::SP_ANTIMAGIC:
            return Spell::ANTIMAGIC;
        case Battle::SP_CURSE:
            return Spell::CURSE;
        case Battle::SP_SLOW:
            return Spell::SLOW;
        case Battle::SP_BERSERKER:
            return Spell::BERSERKER;
        case Battle::SP_HYPNOTIZE:
            return Spell::HYPNOTIZE;
        case Battle::SP_BLIND:
            return Spell::BLIND;
        case Battle::SP_PARALYZE:
            return Spell::PARALYZE;
        case Battle::SP_STONE:
            return Spell::PETRIFY;
        default:
            // Did you add another mode? Please add a corresponding spell.
            assert( 0 );
            break;
        }

        return Spell::NONE;
    }

    void DrawMonsterStats( const fheroes2::Point & dst, const Troop & troop )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        // attack
        fheroes2::Text text( std::string( _( "Attack Skill" ) ) + ":", fheroes2::FontType::normalWhite() );
        fheroes2::Point dst_pt{ dst.x - text.width(), dst.y };
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        const int offsetX = 6;
        const int offsetY = 16;

        text.set( troop.GetAttackString(), fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x + offsetX;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        // defense
        text.set( std::string( _( "Defense Skill" ) ) + ":", fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x - text.width();
        dst_pt.y += offsetY;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        text.set( troop.GetDefenseString(), fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x + offsetX;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        if ( fheroes2::getMonsterData( troop.GetID() ).battleStats.shots > 0 ) {
            std::string message = troop.isBattle() ? _( "Shots Left" ) : _( "Shots" );
            message += ':';
            text.set( std::move( message ), fheroes2::FontType::normalWhite() );
            dst_pt.x = dst.x - text.width();
            dst_pt.y += offsetY;
            text.draw( dst_pt.x, dst_pt.y + 2, display );

            text.set( troop.GetShotString(), fheroes2::FontType::normalWhite() );
            dst_pt.x = dst.x + offsetX;
            text.draw( dst_pt.x, dst_pt.y + 2, display );
        }

        // damage
        text.set( std::string( _( "Damage" ) ) + ":", fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x - text.width();
        dst_pt.y += offsetY;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        if ( troop.GetMonster().GetDamageMin() != troop.GetMonster().GetDamageMax() ) {
            text.set( std::to_string( troop.GetMonster().GetDamageMin() ) + "-" + std::to_string( troop.GetMonster().GetDamageMax() ),
                      fheroes2::FontType::normalWhite() );
        }
        else {
            text.set( std::to_string( troop.GetMonster().GetDamageMin() ), fheroes2::FontType::normalWhite() );
        }

        dst_pt.x = dst.x + offsetX;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        // hp
        text.set( std::string( _( "Hit Points" ) ) + ":", fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x - text.width();
        dst_pt.y += offsetY;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        text.set( std::to_string( troop.GetMonster().GetHitPoints() ), fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x + offsetX;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        if ( troop.isBattle() && troop.GetCount() != 0 ) {
            text.set( std::string( _( "Hit Points Left" ) ) + ":", fheroes2::FontType::normalWhite() );
            dst_pt.x = dst.x - text.width();
            dst_pt.y += offsetY;
            text.draw( dst_pt.x, dst_pt.y + 2, display );

            text.set( std::to_string( troop.GetHitPointsLeft() ), fheroes2::FontType::normalWhite() );
            dst_pt.x = dst.x + offsetX;
            text.draw( dst_pt.x, dst_pt.y + 2, display );
        }

        // speed
        text.set( std::string( _( "Speed" ) ) + ":", fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x - text.width();
        dst_pt.y += offsetY;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        text.set( troop.GetSpeedString(), fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x + offsetX;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        // morale
        text.set( std::string( _( "Morale" ) ) + ":", fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x - text.width();
        dst_pt.y += offsetY;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        text.set( Morale::String( troop.GetMorale() ), fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x + offsetX;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        // luck
        text.set( std::string( _( "Luck" ) ) + ":", fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x - text.width();
        dst_pt.y += offsetY;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        text.set( Luck::String( troop.GetLuck() ), fheroes2::FontType::normalWhite() );
        dst_pt.x = dst.x + offsetX;
        text.draw( dst_pt.x, dst_pt.y + 2, display );
    }

    const fheroes2::Sprite & GetModesSprite( const uint32_t mod )
    {
        switch ( mod ) {
        case Battle::SP_BLOODLUST:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 9 );
        case Battle::SP_BLESS:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 3 );
        case Battle::SP_HASTE:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 0 );
        case Battle::SP_SHIELD:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 10 );
        case Battle::SP_STONESKIN:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 13 );
        case Battle::SP_DRAGONSLAYER:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 8 );
        case Battle::SP_STEELSKIN:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 14 );
        case Battle::SP_ANTIMAGIC:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 12 );
        case Battle::SP_CURSE:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 4 );
        case Battle::SP_SLOW:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 1 );
        case Battle::SP_BERSERKER:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 5 );
        case Battle::SP_HYPNOTIZE:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 7 );
        case Battle::SP_BLIND:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 2 );
        case Battle::SP_PARALYZE:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 6 );
        case Battle::SP_STONE:
            return fheroes2::AGG::GetICN( ICN::SPELLINL, 11 );
        default:
            break;
        }

        return fheroes2::AGG::GetICN( ICN::UNKNOWN, 0 );
    }

    std::vector<std::pair<fheroes2::Rect, Spell>> DrawBattleStats( const fheroes2::Point & dst, const Troop & b )
    {
        std::vector<std::pair<fheroes2::Rect, Spell>> output;

        const std::array<uint32_t, 15> modes = { Battle::SP_BLOODLUST,    Battle::SP_BLESS,     Battle::SP_HASTE,     Battle::SP_SHIELD,   Battle::SP_STONESKIN,
                                                 Battle::SP_DRAGONSLAYER, Battle::SP_STEELSKIN, Battle::SP_ANTIMAGIC, Battle::SP_CURSE,    Battle::SP_SLOW,
                                                 Battle::SP_BERSERKER,    Battle::SP_HYPNOTIZE, Battle::SP_BLIND,     Battle::SP_PARALYZE, Battle::SP_STONE };

        int32_t ow = 0;
        int32_t spritesWidth = 0;

        std::vector<SpellInfo> spellsInfo;
        for ( const uint32_t mode : modes ) {
            if ( !b.isModes( mode ) )
                continue;

            const fheroes2::Sprite & sprite = GetModesSprite( mode );
            if ( sprite.empty() )
                continue;

            const uint32_t duration = b.GetAffectedDuration( mode );
            int offset = 0;
            if ( duration > 0 ) {
                offset = duration >= 10 ? 12 : 7;
                if ( mode >= Battle::SP_BLESS && mode <= Battle::SP_DRAGONSLAYER )
                    offset -= 5;
            }
            const int space = ( offset == 2 ) ? 10 : 5;

            spellsInfo.emplace_back( mode, duration, offset, space );
            ow += sprite.width() + offset + space;
            spritesWidth += sprite.width();
        }

        if ( spellsInfo.empty() )
            return output;

        std::sort( spellsInfo.begin(), spellsInfo.end(),
                   []( const SpellInfo & first, const SpellInfo & second ) { return first.duration > 0 && first.duration < second.duration; } );

        ow -= spellsInfo.back().space;

        const int maxSpritesWidth = 212;
        const int maxSpriteHeight = 32;

        fheroes2::Display & display = fheroes2::Display::instance();

        if ( ow <= maxSpritesWidth ) {
            ow = dst.x - ow / 2;
            for ( const auto & spell : spellsInfo ) {
                const fheroes2::Sprite & sprite = GetModesSprite( spell.mode );
                const fheroes2::Point imageOffset( ow, dst.y + maxSpriteHeight - sprite.height() );

                fheroes2::Blit( sprite, display, imageOffset.x, imageOffset.y );
                output.emplace_back( fheroes2::Rect( imageOffset.x, imageOffset.y, sprite.width(), sprite.height() ), modeToSpell( spell.mode ) );

                if ( spell.duration > 0 ) {
                    const fheroes2::Text text( std::to_string( spell.duration ), fheroes2::FontType::smallWhite() );
                    ow += sprite.width() + spell.offset;
                    text.draw( ow - text.width(), dst.y + maxSpriteHeight - text.height() + 3, display );
                }
                ow += spell.space;
            }
        }
        else {
            // Too many spells
            const int widthDiff = maxSpritesWidth - spritesWidth;
            int space = widthDiff / static_cast<int>( spellsInfo.size() - 1 );
            if ( widthDiff > 0 ) {
                if ( space > 10 )
                    space = 10;
                ow = dst.x + ( spritesWidth + space * static_cast<int>( spellsInfo.size() - 1 ) ) / 2;
            }
            else {
                ow = dst.x + maxSpritesWidth / 2;
            }

            for ( auto spellIt = spellsInfo.crbegin(); spellIt != spellsInfo.crend(); ++spellIt ) {
                const fheroes2::Sprite & sprite = GetModesSprite( spellIt->mode );
                const fheroes2::Point imageOffset( ow - sprite.width(), dst.y + maxSpriteHeight - sprite.height() );

                fheroes2::Blit( sprite, display, imageOffset.x, imageOffset.y );
                output.emplace_back( fheroes2::Rect( imageOffset.x, imageOffset.y, sprite.width(), sprite.height() ), modeToSpell( spellIt->mode ) );

                if ( spellIt->duration > 0 ) {
                    const fheroes2::Text text( std::to_string( spellIt->duration ), fheroes2::FontType::smallWhite() );
                    text.draw( ow - text.width(), dst.y + maxSpriteHeight - text.height() + 3, display );
                }
                ow -= sprite.width() + space;
            }
        }

        return output;
    }

    void DrawMonsterInfo( const fheroes2::Point & offset, const Troop & troop )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        // name
        fheroes2::Text text( troop.GetName(), fheroes2::FontType::normalYellow() );
        fheroes2::Point pos( offset.x + 29 + ( 227 - text.width() ) / 2, offset.y + 37 );
        text.draw( pos.x, pos.y + 2, display );

        // Description.
        const std::vector<std::string> descriptions = fheroes2::getMonsterPropertiesDescription( troop.GetID() );
        if ( !descriptions.empty() ) {
            const int32_t descriptionWidth = 210;
            const int32_t maximumRowCount = 3;
            const int32_t rowHeight = fheroes2::getFontHeight( fheroes2::FontSize::SMALL );

            bool asSolidText = true;
            if ( descriptions.size() <= static_cast<size_t>( maximumRowCount ) ) {
                asSolidText = false;
                for ( const std::string & sentence : descriptions ) {
                    if ( fheroes2::Text( sentence, fheroes2::FontType::smallWhite() ).width() > descriptionWidth ) {
                        asSolidText = true;
                        break;
                    }
                }
            }

            if ( asSolidText ) {
                std::string description;
                for ( const std::string & sentence : descriptions ) {
                    if ( !description.empty() ) {
                        description += ' ';
                    }

                    description += sentence;
                }

                const fheroes2::Text descriptionText( description, fheroes2::FontType::smallWhite() );
                const int32_t rowCount = descriptionText.rows( descriptionWidth );

                descriptionText.draw( offset.x + 37, offset.y + 185 + ( maximumRowCount - rowCount ) * rowHeight, descriptionWidth, display );
            }
            else {
                int32_t sentenceId = maximumRowCount - static_cast<int32_t>( descriptions.size() ); // safe to cast as we check the size before.
                for ( const std::string & sentence : descriptions ) {
                    const fheroes2::Text descriptionText( sentence, fheroes2::FontType::smallWhite() );

                    descriptionText.draw( offset.x + 37, offset.y + 185 + sentenceId * rowHeight, descriptionWidth, display );
                    ++sentenceId;
                }
            }
        }

        // amount
        if ( troop.GetCount() != 0 ) {
            text.set( std::to_string( troop.GetCount() ), fheroes2::FontType::normalWhite() );
            pos.x = offset.x + offsetXAmountBox + widthAmountBox / 2 - text.width() / 2;
            pos.y = offset.y + offsetYAmountBox + heightAmountBox / 2 - text.height() / 2;
            text.draw( pos.x, pos.y + 2, display );
        }
    }

    void DrawMonster( fheroes2::RandomMonsterAnimation & monsterAnimation, const Troop & troop, const fheroes2::Point & offset, bool isReflected, bool isAnimated,
                      const fheroes2::Rect & roi )
    {
        const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( monsterAnimation.icnFile(), monsterAnimation.frameId() );
        fheroes2::Point monsterPos( offset.x, offset.y + monsterSprite.y() );
        if ( isReflected )
            monsterPos.x -= monsterSprite.x() - ( troop.isWide() ? Battle::Cell::widthPx / 2 : 0 ) - monsterAnimation.offset() + monsterSprite.width();
        else
            monsterPos.x += monsterSprite.x() - ( troop.isWide() ? Battle::Cell::widthPx / 2 : 0 ) - monsterAnimation.offset();

        fheroes2::Point inPos( 0, 0 );
        fheroes2::Point outPos( monsterPos.x, monsterPos.y );
        fheroes2::Size inSize( monsterSprite.width(), monsterSprite.height() );

        fheroes2::Display & display = fheroes2::Display::instance();

        if ( fheroes2::FitToRoi( monsterSprite, inPos, display, outPos, inSize, roi ) ) {
            if ( troop.isModes( Battle::CAP_MIRRORIMAGE ) ) {
                fheroes2::Sprite outMonsterSprite = monsterSprite;
                fheroes2::ApplyPalette( outMonsterSprite, PAL::GetPalette( PAL::PaletteType::MIRROR_IMAGE ) );
                fheroes2::Blit( outMonsterSprite, inPos, display, outPos, inSize, isReflected );
            }
            else {
                fheroes2::Blit( monsterSprite, inPos, display, outPos, inSize, isReflected );
            }
        }

        if ( isAnimated )
            monsterAnimation.increment();
    }
}

int Dialog::ArmyInfo( const Troop & troop, int flags, bool isReflected, const int32_t windowOffsetY )
{
    // Unit cannot be dismissed or upgraded during combat
    assert( !troop.isBattle() || !( flags & BUTTONS ) || !( flags & ( UPGRADE | DISMISS ) ) );
    // This function should not be called with the UPGRADE flag for a non-upgradeable unit
    assert( !( flags & BUTTONS ) || !( flags & UPGRADE ) || troop.isAllowUpgrade() );

    // The active size of the window is 520 by 256 pixels
    fheroes2::Display & display = fheroes2::Display::instance();
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    const int viewarmy = isEvilInterface ? ICN::VIEWARME : ICN::VIEWARMY;
    const fheroes2::Sprite & sprite_dialog = fheroes2::AGG::GetICN( viewarmy, 0 );
    const fheroes2::Sprite & spriteDialogShadow = fheroes2::AGG::GetICN( viewarmy, 7 );

    // setup cursor
    const CursorRestorer cursorRestorer( ( flags & BUTTONS ) != 0, Cursor::POINTER );

    fheroes2::Point dialogOffset( ( display.width() - sprite_dialog.width() ) / 2, ( display.height() - sprite_dialog.height() ) / 2 );
    if ( isEvilInterface ) {
        dialogOffset.y += 3;
    }
    dialogOffset.y += windowOffsetY;

    const fheroes2::Point shadowShift( spriteDialogShadow.x() - sprite_dialog.x(), spriteDialogShadow.y() - sprite_dialog.y() );
    const fheroes2::Point shadowOffset( dialogOffset.x + shadowShift.x, dialogOffset.y + shadowShift.y );

    const fheroes2::ImageRestorer restorer( display, shadowOffset.x, dialogOffset.y, sprite_dialog.width() - shadowShift.x, sprite_dialog.height() + shadowShift.y );
    fheroes2::Blit( spriteDialogShadow, display, dialogOffset.x + shadowShift.x, dialogOffset.y + shadowShift.y );
    fheroes2::Blit( sprite_dialog, display, dialogOffset.x, dialogOffset.y );

    fheroes2::Rect pos_rt( dialogOffset.x, dialogOffset.y, sprite_dialog.width(), sprite_dialog.height() );
    if ( isEvilInterface ) {
        pos_rt.x += 9;
        pos_rt.y -= 1;
    }

    const fheroes2::Point monsterStatOffset( pos_rt.x + 400, pos_rt.y + 37 );
    DrawMonsterStats( monsterStatOffset, troop );

    std::vector<std::pair<fheroes2::Rect, Spell>> spellAreas;

    const fheroes2::Point battleStatOffset( pos_rt.x + 395, pos_rt.y + 184 );
    if ( troop.isBattle() ) {
        spellAreas = DrawBattleStats( battleStatOffset, troop );
    }

    DrawMonsterInfo( pos_rt.getPosition(), troop );

    const bool isAnimated = ( flags & BUTTONS ) != 0;
    fheroes2::RandomMonsterAnimation monsterAnimation( troop );
    const fheroes2::Point monsterOffset( pos_rt.x + 520 / 4 + 16, pos_rt.y + 175 );
    if ( !isAnimated ) {
        monsterAnimation.reset();
    }

    const fheroes2::Rect dialogRoi( pos_rt.x, pos_rt.y + fheroes2::shadowWidthPx, sprite_dialog.width(), sprite_dialog.height() - 2 * fheroes2::shadowWidthPx );
    DrawMonster( monsterAnimation, troop, monsterOffset, isReflected, isAnimated, dialogRoi );

    const int upgradeButtonIcnID = isEvilInterface ? ICN::BUTTON_SMALL_UPGRADE_EVIL : ICN::BUTTON_SMALL_UPGRADE_GOOD;
    fheroes2::Point dst_pt( pos_rt.x + 400, pos_rt.y + 40 );
    dst_pt.x = pos_rt.x + 280;
    dst_pt.y = pos_rt.y + 192;
    fheroes2::Button buttonUpgrade( dst_pt.x, dst_pt.y, upgradeButtonIcnID, 0, 1 );

    const int dismissButtonIcnID = isEvilInterface ? ICN::BUTTON_SMALL_DISMISS_EVIL : ICN::BUTTON_SMALL_DISMISS_GOOD;
    dst_pt.x = pos_rt.x + 280;
    dst_pt.y = pos_rt.y + 221;
    fheroes2::Button buttonDismiss( dst_pt.x, dst_pt.y, dismissButtonIcnID, 0, 1 );

    const int exitButtonIcnID = isEvilInterface ? ICN::BUTTON_SMALL_EXIT_EVIL : ICN::BUTTON_SMALL_EXIT_GOOD;
    const int32_t exitWidth = fheroes2::AGG::GetICN( exitButtonIcnID, 0 ).width();
    const int32_t interfaceAdjustment = isEvilInterface ? 0 : 18;
    dst_pt.x = pos_rt.x + sprite_dialog.width() - 58 - exitWidth + interfaceAdjustment;
    dst_pt.y = pos_rt.y + 221;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, exitButtonIcnID, 0, 1 );

    if ( ( flags & ( BUTTONS | UPGRADE ) ) == ( BUTTONS | UPGRADE ) ) {
        buttonUpgrade.enable();
        buttonUpgrade.draw();
    }
    else {
        buttonUpgrade.disable();
    }

    if ( ( flags & ( BUTTONS | DISMISS ) ) == ( BUTTONS | DISMISS ) ) {
        buttonDismiss.enable();
        buttonDismiss.draw();
    }
    else {
        buttonDismiss.disable();
    }

    if ( flags & BUTTONS ) {
        buttonExit.draw();
    }

    LocalEvent & le = LocalEvent::Get();
    int result = Dialog::ZERO;

    display.render( restorer.rect() );

    while ( le.HandleEvents( ( flags & BUTTONS ) ? Game::isDelayNeeded( { Game::CASTLE_UNIT_DELAY } ) : true ) ) {
        if ( !( flags & BUTTONS ) ) {
            if ( !le.isMouseRightButtonPressed() ) {
                break;
            }

            continue;
        }

        if ( buttonUpgrade.isEnabled() ) {
            le.isMouseLeftButtonPressedInArea( buttonUpgrade.area() ) ? buttonUpgrade.drawOnPress() : buttonUpgrade.drawOnRelease();
        }

        if ( buttonDismiss.isEnabled() ) {
            le.isMouseLeftButtonPressedInArea( buttonDismiss.area() ) ? buttonDismiss.drawOnPress() : buttonDismiss.drawOnRelease();
        }

        le.isMouseLeftButtonPressedInArea( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( buttonUpgrade.isEnabled() && ( le.MouseClickLeft( buttonUpgrade.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::ARMY_UPGRADE_TROOP ) ) ) {
            // If this assertion blows up then you are executing this code for a monster which has no upgrades.
            assert( troop.isAllowUpgrade() );

            if ( UPGRADE_DISABLE & flags ) {
                const fheroes2::Text description( _( "You can't afford to upgrade your troops!" ), fheroes2::FontType::normalWhite() );
                fheroes2::showResourceMessage( fheroes2::Text( "", {} ), description, Dialog::OK, troop.GetTotalUpgradeCost() );
            }
            else {
                const fheroes2::Text description( _( "Your troops can be upgraded, but it will cost you dearly. Do you wish to upgrade them?" ),
                                                  fheroes2::FontType::normalWhite() );

                if ( fheroes2::showResourceMessage( fheroes2::Text( "", {} ), description, Dialog::YES | Dialog::NO, troop.GetTotalUpgradeCost() ) == Dialog::YES ) {
                    result = Dialog::UPGRADE;
                    break;
                }
            }
        }

        if ( buttonDismiss.isEnabled() && ( le.MouseClickLeft( buttonDismiss.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::ARMY_DISMISS ) )
             && Dialog::YES
                    == fheroes2::showStandardTextMessage( troop.GetPluralName( troop.GetCount() ), _( "Are you sure you want to dismiss this army?" ),
                                                          Dialog::YES | Dialog::NO ) ) {
            result = Dialog::DISMISS;
            break;
        }

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            result = Dialog::CANCEL;
            break;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonExit.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit this menu." ), 0 );
        }
        else if ( buttonUpgrade.isEnabled() && le.isMouseRightButtonPressedInArea( buttonUpgrade.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Upgrade" ), _( "Upgrade your troops." ), 0 );
        }
        else if ( buttonDismiss.isEnabled() && le.isMouseRightButtonPressedInArea( buttonDismiss.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Dismiss" ), _( "Dismiss this army." ), 0 );
        }

        for ( const auto & spellInfo : spellAreas ) {
            if ( le.isMouseRightButtonPressedInArea( spellInfo.first ) ) {
                fheroes2::SpellDialogElement( spellInfo.second, nullptr ).showPopup( Dialog::ZERO );
                break;
            }
        }

        if ( Game::validateAnimationDelay( Game::CASTLE_UNIT_DELAY ) ) {
            fheroes2::Blit( sprite_dialog, display, dialogOffset.x, dialogOffset.y );

            DrawMonsterStats( monsterStatOffset, troop );

            if ( troop.isBattle() ) {
                spellAreas = DrawBattleStats( battleStatOffset, troop );
            }

            DrawMonsterInfo( pos_rt.getPosition(), troop );
            DrawMonster( monsterAnimation, troop, monsterOffset, isReflected, true, dialogRoi );

            if ( buttonUpgrade.isEnabled() ) {
                buttonUpgrade.draw();
            }

            if ( buttonDismiss.isEnabled() ) {
                buttonDismiss.draw();
            }

            if ( buttonExit.isEnabled() ) {
                buttonExit.draw();
            }

            display.render( restorer.rect() );
        }
    }

    return result;
}

int Dialog::ArmyJoinFree( const Troop & troop )
{
    std::string message = _( "A group of %{monster} with a desire for greater glory wish to join you.\nDo you accept?" );
    StringReplaceWithLowercase( message, "%{monster}", troop.GetMultiName() );

    return fheroes2::showStandardTextMessage( _( "Followers" ), std::move( message ), Dialog::YES | Dialog::NO );
}

int Dialog::ArmyJoinWithCost( const Troop & troop, const uint32_t join, const uint32_t gold )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::string message;

    if ( troop.GetCount() == 1 ) {
        message = _( "The %{monster} is swayed by your diplomatic tongue, and offers to join your army for the sum of %{gold} gold.\nDo you accept?" );
    }
    else {
        message = _( "The creatures are swayed by your diplomatic\ntongue, and make you an offer:\n\n" );

        if ( join != troop.GetCount() )
            message += _( "%{offer} of the %{total} %{monster} will join your army, and the rest will leave you alone, for the sum of %{gold} gold.\nDo you accept?" );
        else
            message += _( "All %{offer} of the %{monster} will join your army for the sum of %{gold} gold.\nDo you accept?" );
    }

    StringReplace( message, "%{offer}", join );
    StringReplace( message, "%{total}", troop.GetCount() );
    StringReplaceWithLowercase( message, "%{monster}", troop.GetPluralName( join ) );
    StringReplace( message, "%{gold}", gold );

    fheroes2::Text textbox( message, fheroes2::FontType::normalWhite() );
    const int buttons = Dialog::YES | Dialog::NO;
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::RESOURCE, 6 );
    int posy = 0;

    message = _( "(Rate: %{percent})" );
    StringReplace( message, "%{percent}", troop.GetMonster().GetCost().gold * join * 100 / gold );
    fheroes2::Text text( message, fheroes2::FontType::normalWhite() );

    const FrameBox box( 10 + textbox.height( fheroes2::boxAreaWidthPx ) + 10 + text.height() + 40 + sprite.height() + 10, true );
    const fheroes2::Rect & pos = box.GetArea();

    posy = pos.y + 10;
    textbox.draw( pos.x, posy + 2, fheroes2::boxAreaWidthPx, display );

    posy += textbox.height( fheroes2::boxAreaWidthPx ) + 10;
    text.draw( pos.x + ( pos.width - text.width() ) / 2, posy + 2, display );

    posy += text.height() + 40;
    fheroes2::Blit( sprite, display, pos.x + ( pos.width - sprite.width() ) / 2, posy );

    const fheroes2::Text goldText( std::to_string( gold ), fheroes2::FontType::smallWhite() );
    goldText.draw( pos.x + ( pos.width - goldText.width() ) / 2, posy + sprite.height() + 5, display );

    fheroes2::ButtonGroup btnGroup( pos, buttons );
    btnGroup.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    int result = Dialog::ZERO;

    while ( result == Dialog::ZERO && le.HandleEvents() ) {
        result = btnGroup.processEvents();
    }

    return result;
}
