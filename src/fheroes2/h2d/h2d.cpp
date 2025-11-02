/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
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

#include "h2d.h"

#include <functional>
#include <set>
#include <stdexcept>

#include "h2d_file.h"
#include "logging.h"
#include "settings.h"
#include "system.h"

namespace
{
    const std::set<std::string, std::less<>> resurrectionH2DFileListSample{
        "adventure-map-grass-cave-diff-01.image",
        "adventure-map-grass-cave-diff-02.image",
        "barbarian_castle_captain_quarter_left_side.image",
        "barrel_animation_0.image",
        "barrel_animation_1.image",
        "barrel_animation_2.image",
        "barrel_animation_3.image",
        "barrel_animation_4.image",
        "barrel_animation_5.image",
        "black_cat.image",
        "black_cat_animation_0.image",
        "black_cat_animation_1.image",
        "black_cat_animation_2.image",
        "black_cat_animation_3.image",
        "black_cat_animation_4.image",
        "black_cat_animation_5.image",
        "book_corner.image",
        "circular_stone_liths_center.image",
        "circular_stone_liths_left.image",
        "circular_stone_liths_top.image",
        "graphics_icon.image",
        "hotkeys_icon.image",
        "keyboard_button_pressed_evil.image",
        "keyboard_button_pressed_good.image",
        "keyboard_button_released_evil.image",
        "keyboard_button_released_good.image",
        "knight_castle_left_farm.image",
        "lean-to-diff-part1.image",
        "lean-to-diff-part2.image",
        "main_menu_editor_highlighted_button.image",
        "main_menu_editor_icon.image",
        "main_menu_editor_pressed_button.image",
        "main_menu_editor_released_button.image",
        "mass-dispel-contour.image",
        "missing_sphinx_part.image",
        "observation_tower_desert_bottom_part.image",
        "observation_tower_desert_right_part.image",
        "observation_tower_generic_bottom_part.image",
        "observation_tower_snow_bottom_part.image",
        "observation_tower_snow_right_part.image",
        "observation_tower_snow_top_part.image",
        "petrification_spell_icon.image",
        "petrification_spell_icon_mini.image",
        "resolution_icon.image",
        "sorceress_castle_captain_quarter_left_side.image",
        "swordsman_walking_frame_extra_part.image",
        "swordsman_walking_frame_extra_part_mask.image",
        "toggle-all-off-pressed.image",
        "toggle-all-off-released.image",
        "toggle-all-on-pressed.image",
        "toggle-all-on-released.image",
        "townbkg2_fix.image",
        "twnsspec_fix.image",
        "wizard_bay_diff_to_twnzdock.image",
        "wizard_bay_diff_to_twnzdock_animation_0.image",
        "wizard_bay_diff_to_twnzdock_animation_1.image",
        "wizard_bay_diff_to_twnzdock_animation_2.image",
        "wizard_bay_diff_to_twnzdock_animation_3.image",
        "wizard_bay_diff_to_twnzdock_animation_4.image",
    };

    fheroes2::H2DReader reader;

    bool getH2DFilePath( const std::string & fileName, std::string & path )
    {
#if defined( MACOS_APP_BUNDLE )
        return Settings::findFile( "h2d", fileName, path );
#else
        return Settings::findFile( System::concatPath( "files", "data" ), fileName, path );
#endif
    }
}

namespace fheroes2::h2d
{
    H2DInitializer::H2DInitializer()
    {
        const std::string fileName{ "resurrection.h2d" };

        std::string filePath;
        if ( !getH2DFilePath( fileName, filePath ) ) {
            const std::string errorMessage{ "The '" + fileName + "' file was not found." };

            VERBOSE_LOG( errorMessage )
            throw std::logic_error( errorMessage );
        }

        if ( !reader.open( filePath ) ) {
            const std::string errorMessage{ "The '" + filePath + "' file cannot be opened." };

            VERBOSE_LOG( errorMessage )
            throw std::logic_error( errorMessage );
        }

        if ( reader.getAllFileNames() != resurrectionH2DFileListSample ) {
            const std::string errorMessage{ "The list of files contained in '" + filePath
                                            + "' does not match the sample. Make sure that you are using the latest version of the '" + fileName + "' file." };

            VERBOSE_LOG( errorMessage )
            throw std::logic_error( errorMessage );
        }
    }

    bool readImage( const std::string & name, Sprite & image )
    {
        return readImageFromH2D( reader, name, image );
    }
}
