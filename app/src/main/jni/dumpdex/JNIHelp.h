#ifndef NATIVEHELPER_JNIHELP_H_
#define NATIVEHELPER_JNIHELP_H_

#include <jni.h>
#include <unistd.h>

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#ifdef LIBCORE_CPP_JNI_HELPERS
#include <string>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int jniRegisterNativeMethods(C_JNIEnv *env, const char *className, const JNINativeMethod *gMethods, int numMethods);

int jniThrowException(C_JNIEnv *env, const char *className, const char *msg);

int jniThrowNullPointerException(C_JNIEnv *env, const char *msg);

int jniThrowRuntimeException(C_JNIEnv *env, const char *msg);

int jniThrowIOException(C_JNIEnv *env, int errnum);

const char *jniStrError(int errnum, char *buf, size_t buflen);

jobject jniCreateFileDescriptor(C_JNIEnv *env, int fd);

int jniGetFDFromFileDescriptor(C_JNIEnv *env, jobject fileDescriptor);

void jniSetFileDescriptorOfFD(C_JNIEnv *env, jobject fileDescriptor, int value);

jobject jniGetReferent(C_JNIEnv *env, jobject ref);

void jniLogException(C_JNIEnv *env, int priority, const char *tag, jthrowable exception);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
inline int jniRegisterNativeMethods(JNIEnv *env, const char *className, const JNINativeMethod *gMethods, int numMethods) {
    return jniRegisterNativeMethods(&env->functions, className, gMethods, numMethods);
}

inline int jniThrowException(JNIEnv *env, const char *className, const char *msg) {
    return jniThrowException(&env->functions, className, msg);
}

extern "C" int jniThrowExceptionFmt(C_JNIEnv *env, const char *className, const char *fmt, va_list args);

inline int jniThrowExceptionFmt(JNIEnv *env, const char *className, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    return jniThrowExceptionFmt(&env->functions, className, fmt, args);
    va_end(args);
}

inline int jniThrowNullPointerException(JNIEnv *env, const char *msg) {
    return jniThrowNullPointerException(&env->functions, msg);
}

inline int jniThrowRuntimeException(JNIEnv *env, const char *msg) {
    return jniThrowRuntimeException(&env->functions, msg);
}

inline int jniThrowIOException(JNIEnv *env, int errnum) {
    return jniThrowIOException(&env->functions, errnum);
}

inline jobject jniCreateFileDescriptor(JNIEnv *env, int fd) {
    return jniCreateFileDescriptor(&env->functions, fd);
}

inline int jniGetFDFromFileDescriptor(JNIEnv *env, jobject fileDescriptor) {
    return jniGetFDFromFileDescriptor(&env->functions, fileDescriptor);
}

inline void jniSetFileDescriptorOfFD(JNIEnv *env, jobject fileDescriptor, int value) {
    jniSetFileDescriptorOfFD(&env->functions, fileDescriptor, value);
}

inline jobject jniGetReferent(JNIEnv *env, jobject ref) {
    return jniGetReferent(&env->functions, ref);
}

inline void jniLogException(JNIEnv *env, int priority, const char *tag, jthrowable exception = NULL) {
    jniLogException(&env->functions, priority, tag, exception);
}

#endif

#ifndef TEMP_FAILURE_RETRY

#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })
#endif

#endif
