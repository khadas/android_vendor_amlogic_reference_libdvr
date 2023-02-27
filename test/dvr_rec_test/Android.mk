LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true
#LOCAL_PRODUCT_MODULE = true


#for amstream.h
AMADEC_C_INCLUDES:=hardware/amlogic/media/amcodec/include \
ANDROID_LOG_INCLUDE:=system/core/liblog/include \

LOCAL_SRC_FILES:= dvr_rec_test.c

LOCAL_MODULE:= dvr_rec_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice

LOCAL_MODULE_TAGS := optional

#LOCAL_MULTILIB := 32

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
                    vendor/amlogic/common/mediahal_sdk/include \
                    $(LOCAL_PATH)/../../include/ \
                    $(ANDROID_LOG_INCLUDE)

LOCAL_SHARED_LIBRARIES := libamdvr
LOCAL_SHARED_LIBRARIES += libcutils liblog libdl libc

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
#LOCAL_VENDOR_MODULE := true
LOCAL_PRODUCT_MODULE = true


#for amstream.h
AMADEC_C_INCLUDES:=hardware/amlogic/media/amcodec/include \
ANDROID_LOG_INCLUDE:=system/core/liblog/include \

LOCAL_SRC_FILES:= dvr_rec_test.c

LOCAL_MODULE:= dvr_rec_test_product

LOCAL_MODULE_TAGS := optional

#LOCAL_MULTILIB := 32

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
                    vendor/amlogic/common/mediahal_sdk/include \
                    $(LOCAL_PATH)/../../include/ \
                    $(ANDROID_LOG_INCLUDE)

LOCAL_SHARED_LIBRARIES := libamdvr.product
LOCAL_SHARED_LIBRARIES += libcutils liblog libdl libc

include $(BUILD_EXECUTABLE)
