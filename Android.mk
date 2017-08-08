LOCAL_PATH := $(call my-dir)

gg_iconv_c_includes := $(LOCAL_PATH)/include
gg_iconv_sources := src/gg_iconv.c src/gg_wchar_ex.c

# This is only available as a static library for now.
include $(CLEAR_VARS)
LOCAL_MODULE := gg_iconv
LOCAL_SRC_FILES := $(gg_iconv_sources)
LOCAL_C_INCLUDES := $(gg_iconv_c_includes)
LOCAL_CFLAGS += -Drestrict=__restrict__ -ffunction-sections -fdata-sections

# These Clang warnings are triggered by the Musl sources. The code is fine,
# but we don't want to modify it. TODO(digit): This is potentially dangerous,
# see if there is a way to build the Musl sources in a separate static library
# and have the main one depend on it, or include its object files.
ifneq ($(TARGET_TOOLCHAIN),$(subst clang,,$(TARGET_TOOLCHAIN)))
	LOCAL_CFLAGS += -Wno-shift-op-parentheses \
					-Wno-string-plus-int \
					-Wno-dangling-else \
					-Wno-bitwise-op-parentheses
endif

LOCAL_EXPORT_C_INCLUDES := $(gg_iconv_c_includes)
include $(BUILD_STATIC_LIBRARY)
