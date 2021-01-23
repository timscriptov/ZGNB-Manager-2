#include <jni.h>

#ifndef _Included_com_perez_qrcode_ZbarManager
#define _Included_com_perez_qrcode_ZbarManager
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jstring JNICALL Java_com_perez_qrcode_ZbarManager_decode
  (JNIEnv *, jobject, jbyteArray, jint, jint, jboolean, jint, jint, jint, jint);

#ifdef __cplusplus
}
#endif
#endif
