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
# Mitigate the issue with Windows command line size limit
LOCAL_SHORT_COMMANDS := true

FHEROES2_ROOT := $(LOCAL_PATH)/../../../../src

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

LOCAL_SRC_FILES :=                                   \
    $(wildcard $(FHEROES2_ROOT)/fheroes2/*/*.cpp)    \
    $(wildcard $(FHEROES2_ROOT)/fheroes2/*/*/*.cpp)  \
    $(wildcard $(FHEROES2_ROOT)/engine/*.cpp)        \
    $(FHEROES2_ROOT)/thirdparty/libsmacker/smacker.c

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer

# TODO: separate debug and release build flags
LOCAL_CPPFLAGS += -std=c++17 -frtti -fcxx-exceptions -DWITH_DEBUG
LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid -lz

include $(BUILD_SHARED_LIBRARY)
