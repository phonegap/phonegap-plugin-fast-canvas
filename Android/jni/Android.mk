LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := FastCanvasJNI
LOCAL_SRC_FILES := FastCanvasJNI.cpp \
				   JNIHelper.cpp \
                   Canvas.cpp \
				   lodepng.c
				   

LOCAL_LDLIBS := -lGLESv1_CM -ldl -llog -landroid

include $(BUILD_SHARED_LIBRARY)