LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := AndroidSurfaceEUI
LOCAL_CPPFLAGS  := -std=c++17 -fexceptions -frtti \
                   -DANDROID \
                   -DGLFW_INCLUDE_NONE \
                   -w

LOCAL_CFLAGS   += -include $(LOCAL_PATH)/src/Android_EUI/eui_android_config.h
LOCAL_CPPFLAGS += -include $(LOCAL_PATH)/src/Android_EUI/eui_android_config.h

# 头文件
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/src/glfw_shim \
    $(LOCAL_PATH)/src/glad_gles \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/3rd \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/include/Android_EUI \
    $(LOCAL_PATH)/include/Android_touch \
    $(LOCAL_PATH)/include/native_surface \
    $(LOCAL_PATH)/src/Android_EUI

# 源文件
LOCAL_SRC_FILES := \
    src/main.cpp \
    src/glfw_shim/glfw_shim.cpp \
    src/Android_EUI/android_main.cpp \
    src/Android_EUI/android_app.cpp \
    src/Android_touch/TouchHelperA.cpp \
    core/async.cpp \
    core/image.cpp \
    core/ime_bridge.c \
    core/network.cpp \
    core/platform.cpp \
    core/text.cpp \
    core/tray_bridge.c


LOCAL_LDLIBS := \
    -llog \
    -landroid \
    -lEGL \
    -lGLESv3 \
    -lz \
    -ldl

include $(BUILD_EXECUTABLE)
