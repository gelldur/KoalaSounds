LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := koala_sound_static

LOCAL_MODULE_FILENAME := libkoala_sound

LOCAL_SRC_FILES :=\
../src/OpenSL_ES/SoundPool.cpp \
../src/OpenSL_ES/OpenSLEngine.cpp\
../src/decoders/OggDecoder.cpp\
../src/Log.cpp\

# libogg
LOCAL_SRC_FILES += \
../libogg-1.3.1/src/framing.c\
../libogg-1.3.1/src/bitwise.c

# libvorbis
LOCAL_SRC_FILES += \
../libvorbis-1.3.4/lib/analysis.c\
../libvorbis-1.3.4/lib/envelope.c\
../libvorbis-1.3.4/lib/lpc.c\
../libvorbis-1.3.4/lib/synthesis.c\
../libvorbis-1.3.4/lib/floor0.c\
../libvorbis-1.3.4/lib/lsp.c\
../libvorbis-1.3.4/lib/registry.c\
../libvorbis-1.3.4/lib/bitrate.c\
../libvorbis-1.3.4/lib/floor1.c\
../libvorbis-1.3.4/lib/mapping0.c\
../libvorbis-1.3.4/lib/res0.c\
../libvorbis-1.3.4/lib/block.c\
../libvorbis-1.3.4/lib/info.c\
../libvorbis-1.3.4/lib/mdct.c\
../libvorbis-1.3.4/lib/sharedbook.c\
../libvorbis-1.3.4/lib/vorbisfile.c\
../libvorbis-1.3.4/lib/codebook.c\
../libvorbis-1.3.4/lib/lookup.c\
../libvorbis-1.3.4/lib/psy.c\
../libvorbis-1.3.4/lib/smallft.c\
../libvorbis-1.3.4/lib/window.c

LOCAL_EXPORT_C_INCLUDES := \
$(LOCAL_PATH)/../src \
$(LOCAL_PATH)/../libogg-1.3.1/include \
$(LOCAL_PATH)/../libvorbis-1.3.4/include\
$(LOCAL_PATH)/../libvorbis-1.3.4/lib
		

LOCAL_C_INCLUDES := \
$(LOCAL_PATH)/../src \
$(LOCAL_PATH)/../libogg-1.3.1/include \
$(LOCAL_PATH)/../libvorbis-1.3.4/include\
$(LOCAL_PATH)/../libvorbis-1.3.4/lib

# for native audio
LOCAL_LDLIBS    += -lOpenSLES
# for logging
LOCAL_LDLIBS    += -llog

#LOCAL_CFLAGS += -Wno-psabi -MMD -Wall -fPIC -Wno-unused-function -Wno-unused-result
#LOCAL_EXPORT_CFLAGS += -Wno-psabi -Wuninitialized -MMD -Wall -Werror -fPIC -std=c++11 -Wno-unused-function -Wno-unused-result

include $(BUILD_STATIC_LIBRARY)
