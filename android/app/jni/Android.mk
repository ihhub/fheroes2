#include $(call all-subdir-makefiles)
LOCAL_PATH := $(call my-dir)

# Common for both C and C++
FHEROES2_C_WARN_OPTIONS := \
    -pedantic \
    -Wall \
    -Wextra \
    -Wcast-align \
    -Wextra-semi \
    -Wfloat-conversion \
    -Wfloat-equal \
    -Winit-self \
    -Wredundant-decls \
    -Wshadow \
    -Wundef \
    -Wuninitialized \
    -Wunused

# C++ only
FHEROES2_CPP_WARN_OPTIONS := \
    -Wctor-dtor-privacy \
    -Woverloaded-virtual

include $(call all-subdir-makefiles)
