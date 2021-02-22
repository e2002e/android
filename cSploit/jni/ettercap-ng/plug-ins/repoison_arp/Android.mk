LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_CFLAGS := -DHAVE_CONFIG_H

LOCAL_C_INCLUDES := $(ettercap_pl_includes)

LOCAL_SRC_FILES := repoison_arp.c

LOCAL_MODULE := ec_repoison_arp

include $(BUILD_SHARED_LIBRARY)