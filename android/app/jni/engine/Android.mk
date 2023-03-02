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

# Mitigate the issue with Windows command line size limit
LOCAL_SHORT_COMMANDS := true

ENGINE_SRC_DIR := $(LOCAL_PATH)/../../../../src/engine

LOCAL_MODULE := engine
LOCAL_SRC_FILES := $(wildcard $(ENGINE_SRC_DIR)/*.cpp)
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer
LOCAL_STATIC_LIBRARIES := smacker
LOCAL_CFLAGS := $(FHEROES2_C_WARN_OPTIONS)
LOCAL_CPP_FEATURES := exceptions rtti
LOCAL_CPPFLAGS := \
    -std=c++17 \
    $(FHEROES2_CPP_WARN_OPTIONS)
LOCAL_EXPORT_C_INCLUDES := $(ENGINE_SRC_DIR)
LOCAL_EXPORT_LDLIBS := -llog -lz

include $(BUILD_STATIC_LIBRARY)
