###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2022                                                    #
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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

# TODO: this should be $(LOCAL_PATH)/../../../../src
# TODO: by changing it make sure that it is compilable on Windows due to command line length limitation.
FHEROES2_ROOT := $(LOCAL_PATH)

LOCAL_C_INCLUDES :=                        \
    $(FHEROES2_ROOT)/fheroes2/agg          \
    $(FHEROES2_ROOT)/fheroes2/ai           \
    $(FHEROES2_ROOT)/fheroes2/army         \
    $(FHEROES2_ROOT)/fheroes2/audio        \
    $(FHEROES2_ROOT)/fheroes2/battle       \
    $(FHEROES2_ROOT)/fheroes2/campaign     \
    $(FHEROES2_ROOT)/fheroes2/castle       \
    $(FHEROES2_ROOT)/fheroes2/dialog       \
    $(FHEROES2_ROOT)/fheroes2/game         \
    $(FHEROES2_ROOT)/fheroes2/gui          \
    $(FHEROES2_ROOT)/fheroes2/h2d          \
    $(FHEROES2_ROOT)/fheroes2/heroes       \
    $(FHEROES2_ROOT)/fheroes2/image        \
    $(FHEROES2_ROOT)/fheroes2/kingdom      \
    $(FHEROES2_ROOT)/fheroes2/maps         \
    $(FHEROES2_ROOT)/fheroes2/monster      \
    $(FHEROES2_ROOT)/fheroes2/objects      \
    $(FHEROES2_ROOT)/fheroes2/resource     \
    $(FHEROES2_ROOT)/fheroes2/spell        \
    $(FHEROES2_ROOT)/fheroes2/system       \
    $(FHEROES2_ROOT)/fheroes2/world        \
    $(FHEROES2_ROOT)/engine                \
    $(FHEROES2_ROOT)/thirdparty/libsmacker

# TODO: find a better solution for getting relative paths for source files.
define walk
  $(wildcard $(1)) $(foreach e, $(wildcard $(1)/*), $(call walk, $(e)))
endef

ALL_FILES = $(call walk, $(FHEROES2_ROOT))
SOURCE_FILE_LIST := $(filter %.cpp, $(ALL_FILES))

LOCAL_SRC_FILES :=                                   \
	$(SOURCE_FILE_LIST:$(LOCAL_PATH)/%=%)            \
	$(FHEROES2_ROOT)/thirdparty/libsmacker/smacker.c

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid -lz

# TODO: separate debug and release build flags
LOCAL_CPPFLAGS += -std=c++17 -frtti -fcxx-exceptions -DWITH_DEBUG

include $(BUILD_SHARED_LIBRARY)
