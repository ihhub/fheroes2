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

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer
LOCAL_STATIC_LIBRARIES := engine
LOCAL_CFLAGS := $(FHEROES2_C_WARN_OPTIONS)
LOCAL_CPP_FEATURES := exceptions rtti
LOCAL_CPPFLAGS := \
    -std=c++17 \
    $(FHEROES2_CPP_WARN_OPTIONS)

include $(BUILD_SHARED_LIBRARY)
