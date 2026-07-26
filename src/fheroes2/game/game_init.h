/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include <memory>

#include "component_base.h"

namespace Game
{
    void initLogging();

    void initDataDir();

    void initConfigDir( const char * appPath );

    void initPalette();

    void initTranslations();

    void initEventHandler();

    void initAnimation();

    void initHotKeys();

    std::unique_ptr<ComponentBase> createHardwareComponent();

    std::unique_ptr<ComponentBase> createCoreComponent();

    std::unique_ptr<ComponentBase> createDisplayComponent();

    std::unique_ptr<ComponentBase> createDataComponent();

    std::unique_ptr<ComponentBase> createAudioComponent( const ComponentBase * dataComponent );
}
