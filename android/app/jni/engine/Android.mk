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
