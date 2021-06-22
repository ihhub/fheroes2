/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#define FORMAT_VERSION_095_RELEASE 9501
#define FORMAT_VERSION_PRE_095_RELEASE 9500
#define FORMAT_VERSION_094_RELEASE 9400
#define FORMAT_VERSION_093_RELEASE 9300

// TODO: once FORMAT_VERSION_094_RELEASE version becomes minimal supported please remove game version handling in HeaderSAV class and FileInfo structure.
#define LAST_SUPPORTED_FORMAT_VERSION FORMAT_VERSION_093_RELEASE

#define CURRENT_FORMAT_VERSION FORMAT_VERSION_095_RELEASE // TODO: update this value for a new release
