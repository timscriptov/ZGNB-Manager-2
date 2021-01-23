#include <jni.h>

#ifndef _Included_com_zengge_nbmanager_Features
#define _Included_com_zengge_nbmanager_Features
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL Java_com_zengge_nbmanager_Features_ExtractAllRAR
(JNIEnv *, jclass, jstring, jstring);

JNIEXPORT jboolean JNICALL Java_com_zengge_nbmanager_Features_Oat2Dex
(JNIEnv *, jclass, jstring);

JNIEXPORT jboolean JNICALL Java_com_zengge_nbmanager_Features_Odex2Dex
(JNIEnv *, jclass, jstring, jstring);

JNIEXPORT jboolean JNICALL Java_com_zengge_nbmanager_Features_ZipAlign
(JNIEnv *, jclass, jstring, jstring);

JNIEXPORT jboolean JNICALL Java_com_zengge_nbmanager_Features_isZipAligned
(JNIEnv *, jclass, jstring);

JNIEXPORT jboolean JNICALL Java_com_zengge_nbmanager_Features_isValidElf
(JNIEnv *, jclass, jstring);

JNIEXPORT jstring JNICALL Java_com_zengge_nbmanager_Features_compressStrToInt
(JNIEnv *, jclass, jstring);

JNIEXPORT jlong JNICALL Java_com_zengge_nbmanager_Features_ELFHash
(JNIEnv *, jclass, jstring);

JNIEXPORT jint JNICALL Java_com_zengge_nbmanager_Features_dumpDex
(JNIEnv *, jclass, jint, jstring);

#ifdef __cplusplus
}
#endif
#endif