###########################################################################
#   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  #
#   Copyright (C) 2021 - 2022                                             #
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
# FHEROES2_WITH_DEBUG: build in debug mode
# FHEROES2_WITH_IMAGE: build with SDL image support
# FHEROES2_WITH_TOOLS: build tools
# FHEROES2_STRICT_COMPILATION: build with strict compilation option (turns warnings into errors)
# FHEROES2_MACOS_APP_BUNDLE: create a Mac app bundle (only valid when building on macOS)
#
# -DFHEROES2_DATA: fheroes2 data directory

PROJECT_VERSION := 0.9.13

TARGET	:= fheroes2

.PHONY: all bundle clean

all:
	$(MAKE) -C src
ifndef FHEROES2_MACOS_APP_BUNDLE
	@cp src/dist/$(TARGET) .
endif

bundle:
ifdef FHEROES2_MACOS_APP_BUNDLE
	@mkdir -p "src/dist/${TARGET}.app/Contents/Resources/translations"
	@mkdir -p "src/dist/${TARGET}.app/Contents/Resources/h2d"
	@mkdir -p "src/dist/${TARGET}.app/Contents/MacOS"
	@cp ./fheroes2.key "src/dist/${TARGET}.app/Contents/Resources"
	@cp ./src/resources/fheroes2.icns "src/dist/${TARGET}.app/Contents/Resources"
	@cp ./files/lang/*.mo "src/dist/${TARGET}.app/Contents/Resources/translations"
	@cp ./files/data/*.h2d "src/dist/${TARGET}.app/Contents/Resources/h2d"
	@sed -e "s/\$${MACOSX\_BUNDLE\_EXECUTABLE\_NAME}/${TARGET}/" -e "s/\$${MACOSX\_BUNDLE\_ICON\_FILE}/fheroes2.icns/" -e "s/\$${MACOSX\_BUNDLE\_GUI\_IDENTIFIER}/com.fheroes2.${TARGET}/" -e "s/\$${MACOSX\_BUNDLE\_BUNDLE\_NAME}/${TARGET}/" -e "s/\$${MACOSX\_BUNDLE\_BUNDLE\_VERSION}/${PROJECT_VERSION}/" -e "s/\$${MACOSX\_BUNDLE\_SHORT\_VERSION\_STRING}/${PROJECT_VERSION}/" ./src/resources/Info.plist.in > "src/dist/${TARGET}.app/Contents/Info.plist"
	@mv "src/dist/${TARGET}" "src/dist/${TARGET}.app/Contents/MacOS"
	@dylibbundler -od -b -x "src/dist/${TARGET}.app/Contents/MacOS/${TARGET}" -d "src/dist/${TARGET}.app/Contents/libs"
	@cp -R "src/dist/${TARGET}.app" .
endif

clean:
	$(MAKE) -C src clean
	@rm -f ./$(TARGET)
	@rm -rf ./${TARGET}.app
