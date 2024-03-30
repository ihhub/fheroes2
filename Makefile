###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2021 - 2024                                             #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

# Options:
#
# FHEROES2_STRICT_COMPILATION: build in strict compilation mode (turns warnings into errors)
# FHEROES2_WITH_DEBUG: build in debug mode
# FHEROES2_WITH_ASAN: build with UB Sanitizer and Address Sanitizer (small runtime overhead, incompatible with FHEROES2_WITH_TSAN)
# FHEROES2_WITH_TSAN: build with UB Sanitizer and Thread Sanitizer (large runtime overhead, incompatible with FHEROES2_WITH_ASAN)
# FHEROES2_WITH_IMAGE: build with SDL_image (requires libpng)
# FHEROES2_WITH_SYSTEM_SMACKER: build with an external libsmacker instead of the bundled one
# FHEROES2_WITH_TOOLS: build additional tools
# FHEROES2_MACOS_APP_BUNDLE: create a Mac app bundle (only valid when building on macOS)
# FHEROES2_DATA: set the built-in path to the fheroes2 data directory (e.g. /usr/share/fheroes2)

TARGET := fheroes2
PROJECT_VERSION := $(file < version.txt)

.PHONY: all bundle clean

all:
	$(MAKE) -C src
	$(MAKE) -C files/lang
ifndef FHEROES2_MACOS_APP_BUNDLE
	@cp src/dist/$(TARGET) .
endif

bundle:
ifdef FHEROES2_MACOS_APP_BUNDLE
	@mkdir -p "src/dist/$(TARGET).app/Contents/Resources/translations"
	@mkdir -p "src/dist/$(TARGET).app/Contents/Resources/h2d"
	@mkdir -p "src/dist/$(TARGET).app/Contents/MacOS"
	@cp ./src/resources/fheroes2.icns "src/dist/$(TARGET).app/Contents/Resources"
	@cp ./files/lang/*.mo "src/dist/$(TARGET).app/Contents/Resources/translations"
	@cp ./files/data/*.h2d "src/dist/$(TARGET).app/Contents/Resources/h2d"
	@sed -e "s/\$${MACOSX_BUNDLE_EXECUTABLE_NAME}/$(TARGET)/" -e "s/\$${MACOSX_BUNDLE_ICON_FILE}/fheroes2.icns/" -e "s/\$${MACOSX_BUNDLE_GUI_IDENTIFIER}/com.fheroes2.$(TARGET)/" -e "s/\$${MACOSX_BUNDLE_BUNDLE_NAME}/$(TARGET)/" -e "s/\$${MACOSX_BUNDLE_BUNDLE_VERSION}/$(PROJECT_VERSION)/" -e "s/\$${MACOSX_BUNDLE_SHORT_VERSION_STRING}/$(PROJECT_VERSION)/" ./src/resources/Info.plist.in > "src/dist/$(TARGET).app/Contents/Info.plist"
	@mv "src/dist/$(TARGET)" "src/dist/$(TARGET).app/Contents/MacOS"
	@dylibbundler -od -b -x "src/dist/$(TARGET).app/Contents/MacOS/$(TARGET)" -d "src/dist/$(TARGET).app/Contents/libs"
	@cp -R "src/dist/$(TARGET).app" .
endif

clean:
	$(MAKE) -C src clean
	$(MAKE) -C files/lang clean
	@rm -f ./$(TARGET)
	@rm -rf ./$(TARGET).app
