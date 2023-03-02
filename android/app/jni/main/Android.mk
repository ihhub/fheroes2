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

MAIN_SRC_DIR := $(LOCAL_PATH)/../../../../src/fheroes2

# SDL expects libmain.so as the main application module
LOCAL_MODULE := main
LOCAL_C_INCLUDES := \
    $(filter %/, $(wildcard $(MAIN_SRC_DIR)/*/)) \
    $(filter %/, $(wildcard $(MAIN_SRC_DIR)/*/*/))
LOCAL_SRC_FILES := \
    $(wildcard $(MAIN_SRC_DIR)/*/*.cpp) \
    $(wildcard $(MAIN_SRC_DIR)/*/*/*.cpp)
LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_STATIC_LIBRARIES := engine
LOCAL_CFLAGS := $(FHEROES2_C_WARN_OPTIONS)
LOCAL_CPP_FEATURES := exceptions rtti
LOCAL_CPPFLAGS := \
    -std=c++17 \
    $(FHEROES2_CPP_WARN_OPTIONS)

include $(BUILD_SHARED_LIBRARY)
