LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libiconv
LIBICONV := qrcode/libiconv
LOCAL_CFLAGS := -I$(LOCAL_PATH)/$(LIBICONV)
LOCAL_SRC_FILES := $(LIBICONV)/iconv.c
include $(BUILD_STATIC_LIBRARY) 


include $(CLEAR_VARS)  
LOCAL_MODULE := function 

QRCODE_FILES = qrcode/convert.c qrcode/decoder.c qrcode/error.c qrcode/image.c qrcode/img_scanner.c \
	qrcode/refcnt.c qrcode/scanner.c qrcode/symbol.c qrcode/video.c qrcode/window.c \
	qrcode/qrcode/bch15_5.c qrcode/qrcode/binarize.c qrcode/qrcode/isaac.c qrcode/qrcode/qrdec.c qrcode/qrcode/qrdectxt.c \
	qrcode/qrcode/rs.c qrcode/qrcode/util.c \
	qrcode/processor/null.c qrcode/video/null.c qrcode/window/null.c qrcode/decoder/qr_finder.c \
	qrcode/decoder/code128.c qrcode/decoder/code39.c qrcode/decoder/code93.c qrcode/decoder/codabar.c \
	qrcode/decoder/ean.c qrcode/decoder/databar.c qrcode/decoder/i25.c \
	qrcode/android_zbar.c

UNRAR_FILES = unrar/filestr.cpp unrar/recvol.cpp unrar/rs.cpp unrar/scantree.cpp
LIB_FILES = unrar/filestr.cpp unrar/scantree.cpp unrar/dll.cpp

RAR_FILES = unrar/consio.cpp unrar/rar.cpp unrar/strlist.cpp unrar/strfn.cpp unrar/pathfn.cpp unrar/savepos.cpp unrar/smallfn.cpp unrar/global.cpp unrar/file.cpp unrar/filefn.cpp unrar/filcreat.cpp \
	unrar/archive.cpp unrar/arcread.cpp unrar/unicode.cpp unrar/system.cpp unrar/isnt.cpp unrar/crypt.cpp unrar/crc.cpp unrar/rawread.cpp unrar/encname.cpp \
	unrar/resource.cpp unrar/match.cpp unrar/timefn.cpp unrar/rdwrfn.cpp unrar/options.cpp unrar/ulinks.cpp unrar/errhnd.cpp unrar/rarvm.cpp \
	unrar/rijndael.cpp unrar/getbits.cpp unrar/sha1.cpp unrar/extinfo.cpp unrar/extract.cpp unrar/volume.cpp unrar/list.cpp unrar/find.cpp unrar/unpack.cpp unrar/cmddata.cpp

DUMPDEX_FILES = dumpdex/Debug.cpp dumpdex/DexUtil.cpp dumpdex/dump_dex.cpp dumpdex/Leb128.cpp

ASTYLE_FILES = astyle/ASBeautifier.cpp astyle/ASEnhancer.cpp astyle/ASFormatter.cpp astyle/ASLocalizer.cpp astyle/ASResource.cpp astyle/astyle_main.cpp

ZIPA_FILES = zipalign/ZipAlign.cpp zipalign/ZipEntry.cpp zipalign/ZipFile.cpp zipalign/android/native/src/utils/SharedBuffer.cpp zipalign/android/native/src/utils/ZipUtils.cpp zipalign/android/native/src/utils/VectorImpl.cpp

LOCAL_SRC_FILES := tracepath.c miniTelnet.c Provider.c ELFHash.cpp mainfunction.cpp zlibmgr.cpp rarext.cpp oat2dex.cpp parse_elf.cpp odex2dex.cpp zipa.cpp dumpdex_main.cpp $(QRCODE_FILES) $(DUMPDEX_FILES) $(RAR_FILES) $(LIB_FILES) $(ZIPA_FILES) $(ASTYLE_FILES)

LOCAL_LDLIBS := -llog -lz

LOCAL_C_INCLUDES += $(LOCAL_PATH)/zipalign \
$(LOCAL_PATH)/zipalign/android/base/include \
$(LOCAL_PATH)/zipalign/android/core/include \
$(LOCAL_PATH)/zipalign/android/native/include \
$(LOCAL_PATH)/qrcode \
$(LOCAL_PATH)/qrcode/libiconv \
$(LOCAL_PATH)/dumpdex

LOCAL_CFLAGS := -DASTYLE_JNI -DSILENT -DRARDLL -I$(LOCAL_PATH)/rar -fexceptions -Os \
    -ffunction-sections -fdata-sections -fvisibility=hidden \
    -w -Wl,--gc-sections
LOCAL_STATIC_LIBRARIES := libiconv
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE    := FileHelper
LOCAL_SRC_FILES := android_os_FileHelper.cpp
include $(BUILD_SHARED_LIBRARY)
