LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Mitigate the issue with Windows command line size limit
LOCAL_SHORT_COMMANDS := true

SMACKER_SRC_DIR := $(LOCAL_PATH)/../../../../src/thirdparty/libsmacker

LOCAL_MODULE := smacker
LOCAL_SRC_FILES := $(SMACKER_SRC_DIR)/smacker.c
LOCAL_EXPORT_C_INCLUDES := $(SMACKER_SRC_DIR)

include $(BUILD_STATIC_LIBRARY)