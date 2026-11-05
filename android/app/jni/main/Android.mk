###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2022 - 2026                                             #
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

# Only these files see asio. The identity of the game sources is written by app/build.gradle.
MULTI_NETWORK_SRC_FILES := \
    $(MAIN_SRC_DIR)/multi/multi_network.cpp \
    $(MAIN_SRC_DIR)/multi/multi_port_mapping.cpp
MULTI_ASIO_INCLUDE_DIR := $(LOCAL_PATH)/../asio/include
MULTI_GENERATED_DIR := $(LOCAL_PATH)/../../build/generated/multi

# SDL expects libmain.so as the main application module
LOCAL_MODULE := main
LOCAL_C_INCLUDES := \
    $(filter %/, $(wildcard $(MAIN_SRC_DIR)/*/)) \
    $(filter %/, $(wildcard $(MAIN_SRC_DIR)/*/*/)) \
    $(MULTI_GENERATED_DIR)
LOCAL_SRC_FILES := \
    $(filter-out $(MULTI_NETWORK_SRC_FILES), \
        $(wildcard $(MAIN_SRC_DIR)/*/*.cpp) \
        $(wildcard $(MAIN_SRC_DIR)/*/*/*.cpp))
LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_STATIC_LIBRARIES := engine multi_network
LOCAL_CFLAGS := $(FHEROES2_C_WARN_OPTIONS)
LOCAL_CPP_FEATURES := exceptions rtti
LOCAL_CPPFLAGS := \
    -std=c++17 \
    $(FHEROES2_CPP_WARN_OPTIONS) \
    -DWITH_NETWORK

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SHORT_COMMANDS := true

LOCAL_MODULE := multi_network
LOCAL_C_INCLUDES := \
    $(filter %/, $(wildcard $(MAIN_SRC_DIR)/*/)) \
    $(filter %/, $(wildcard $(MAIN_SRC_DIR)/*/*/))
LOCAL_SRC_FILES := $(MULTI_NETWORK_SRC_FILES)
LOCAL_STATIC_LIBRARIES := engine
LOCAL_CFLAGS := $(FHEROES2_C_WARN_OPTIONS)
LOCAL_CPP_FEATURES := exceptions rtti
LOCAL_CPPFLAGS := \
    -std=c++17 \
    $(FHEROES2_CPP_WARN_OPTIONS) \
    -DWITH_NETWORK \
    -isystem $(MULTI_ASIO_INCLUDE_DIR)

include $(BUILD_STATIC_LIBRARY)
