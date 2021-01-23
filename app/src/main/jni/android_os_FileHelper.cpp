#include <string.h>
#include <stdlib.h>

#include "jni.h"

#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include <unistd.h>
#include <assert.h>

#include <sys/ioctl.h>
#include <linux/msdos_fs.h>

#ifdef __cplusplus
extern "C" {
#endif

char *j2c(JNIEnv *env, jstring jstr) {
    char *rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char *)malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

namespace android {
    JNIEXPORT jint JNICALL Java_android_os_FileHelper_setPermissions(JNIEnv *env, jobject clazz, jstring file, jint mode,
            jint uid, jint gid) {
        char *str = j2c(env, file);
        if (strlen(str) <= 0 || !str)
            return ENOENT;
        if (uid >= 0 || gid >= 0) {
            int res = chown(str, uid, gid);
            if (res != 0)
                return errno;
        }
        return chmod(str, mode) == 0 ? 0 : errno;
    }

    JNIEXPORT jint JNICALL Java_android_os_FileHelper_getPermissions(JNIEnv *env, jobject clazz, jstring file,
            jintArray outArray) {
        char *str = j2c(env, file);
        if (strlen(str) <= 0 || !str)
            return ENOENT;
        struct stat st;
        if (stat(str, &st) != 0)
            return errno;
        jint *array = (jint *)env->GetPrimitiveArrayCritical(outArray, 0);
        if (array) {
            int len = env->GetArrayLength(outArray);
            if (len >= 1)
                array[0] = st.st_mode;
            if (len >= 2)
                array[1] = st.st_uid;
            if (len >= 3)
                array[2] = st.st_gid;
        }
        env->ReleasePrimitiveArrayCritical(outArray, array, 0);
        return 0;
    }

    //Landroid/os/FileHelper;.stat (Ljava/lang/String;Landroid/os/FileHelper$FileStatus;)Z
    JNIEXPORT jboolean JNICALL Java_android_os_FileHelper_stat(JNIEnv *env, jobject clazz, jstring path, jobject fileStatus) {
        //li("-------stat");
        static jclass gFileStatusClass = env->FindClass("android/os/FileHelper$FileStatus");
        if (gFileStatusClass == NULL)
            return false;
        jfieldID gFileStatusDevFieldID = env->GetFieldID(gFileStatusClass, "dev", "I");
        jfieldID gFileStatusInoFieldID = env->GetFieldID(gFileStatusClass, "ino", "I");
        jfieldID gFileStatusModeFieldID = env->GetFieldID(gFileStatusClass, "mode", "I");
        jfieldID gFileStatusNlinkFieldID = env->GetFieldID(gFileStatusClass, "nlink", "I");
        jfieldID gFileStatusUidFieldID = env->GetFieldID(gFileStatusClass, "uid", "I");
        jfieldID gFileStatusGidFieldID = env->GetFieldID(gFileStatusClass, "gid", "I");
        jfieldID gFileStatusSizeFieldID = env->GetFieldID(gFileStatusClass, "size", "J");
        jfieldID gFileStatusBlksizeFieldID = env->GetFieldID(gFileStatusClass, "blksize", "I");
        jfieldID gFileStatusBlocksFieldID = env->GetFieldID(gFileStatusClass, "blocks", "J");
        jfieldID gFileStatusAtimeFieldID = env->GetFieldID(gFileStatusClass, "atime", "J");
        jfieldID gFileStatusMtimeFieldID = env->GetFieldID(gFileStatusClass, "mtime", "J");
        jfieldID gFileStatusCtimeFieldID = env->GetFieldID(gFileStatusClass, "ctime", "J");
        const char *pathStr = env->GetStringUTFChars(path, NULL);
        jboolean ret = false;
        struct stat s;
        int res = stat(pathStr, &s);
        if (res == 0) {
            ret = true;
            if (fileStatus != NULL) {
                env->SetIntField(fileStatus, gFileStatusDevFieldID, s.st_dev);
                env->SetIntField(fileStatus, gFileStatusInoFieldID, s.st_ino);
                env->SetIntField(fileStatus, gFileStatusModeFieldID, s.st_mode);
                env->SetIntField(fileStatus, gFileStatusNlinkFieldID, s.st_nlink);
                env->SetIntField(fileStatus, gFileStatusUidFieldID, s.st_uid);
                env->SetIntField(fileStatus, gFileStatusGidFieldID, s.st_gid);
                env->SetLongField(fileStatus, gFileStatusSizeFieldID, s.st_size);
                env->SetIntField(fileStatus, gFileStatusBlksizeFieldID, s.st_blksize);
                env->SetLongField(fileStatus, gFileStatusBlocksFieldID, s.st_blocks);
                env->SetLongField(fileStatus, gFileStatusAtimeFieldID, s.st_atime);
                env->SetLongField(fileStatus, gFileStatusMtimeFieldID, s.st_mtime);
                env->SetLongField(fileStatus, gFileStatusCtimeFieldID, s.st_ctime);
            }
        }
        env->ReleaseStringUTFChars(path, pathStr);
        return ret;
    }

    JNIEXPORT jobject JNICALL Java_android_os_FileHelper_getpwuid(JNIEnv *env, jobject clazz, jint uid) {
        jclass cPassWord = env->FindClass("android/os/FileHelper$PassWord");
        if (cPassWord == NULL)
            return 0;
        jfieldID gPassWordNameFieldID = env->GetFieldID(cPassWord, "name", "Ljava/lang/String;");
        jfieldID gPassWordPasswdFieldID = env->GetFieldID(cPassWord, "passwd", "Ljava/lang/String;");
        jfieldID gPassWordUidFieldID = env->GetFieldID(cPassWord, "uid", "I");
        jfieldID gPassWordGidFieldID = env->GetFieldID(cPassWord, "gid", "I");
        //jfieldID gPassWordGecosFieldID = env->GetFieldID(cPassWord, "gecos", "Ljava/lang/String;");
        jfieldID gPassWordDirFieldID = env->GetFieldID(cPassWord, "dir", "Ljava/lang/String;");
        jfieldID gPassWordShellFieldID = env->GetFieldID(cPassWord, "shell", "Ljava/lang/String;");
        jobject passwd = (env)->AllocObject(cPassWord);
        struct passwd *pw;
        pw = getpwuid(uid);
        if (pw == 0)
            return 0;
        env->SetObjectField(passwd, gPassWordNameFieldID, env->NewStringUTF(pw->pw_name));
        env->SetObjectField(passwd, gPassWordPasswdFieldID, env->NewStringUTF(pw->pw_passwd));
        env->SetIntField(passwd, gPassWordUidFieldID, pw->pw_uid);
        env->SetIntField(passwd, gPassWordGidFieldID, pw->pw_gid);
        env->SetObjectField(passwd, gPassWordDirFieldID, env->NewStringUTF(pw->pw_dir));
        env->SetObjectField(passwd, gPassWordShellFieldID, env->NewStringUTF(pw->pw_shell));
        return passwd;
    }

    //#define SetObjectField(pEnv,pObj, pFieldID, pVal) pEnv->SetObjectField(pObj, pFieldID, pVal);DeleteLocalRef(pEnv, pVal)

    JNIEXPORT jobject JNICALL Java_android_os_FileHelper_getgrgid(JNIEnv *env, jobject clazz, jint gid) {

        jclass cGroup = env->FindClass("android/os/FileHelper$Group");
        if (cGroup == NULL) {
            return 0;
        }
        jfieldID gGroupNameFieldID = env->GetFieldID(cGroup, "name", "Ljava/lang/String;");
        jfieldID gGroupPasswdFieldID = env->GetFieldID(cGroup, "passwd", "Ljava/lang/String;");
        jfieldID gGroupGidFieldID = env->GetFieldID(cGroup, "gid", "I");
        jfieldID gGroupMemFieldID = env->GetFieldID(cGroup, "mem", "[Ljava/lang/String;");
        jobject group = (env)->AllocObject(cGroup);
        struct group *gr;
        gr = getgrgid(gid);
        if (gr == 0)
            return 0;
        jclass jcstring = env->FindClass("java/lang/String");
        jstring strTemp;
        strTemp = env->NewStringUTF(gr->gr_name);
        env->SetObjectField(group, gGroupNameFieldID, strTemp);
        env->SetObjectField(group, gGroupPasswdFieldID, env->NewStringUTF(gr->gr_passwd));
        env->SetIntField(group, gGroupGidFieldID, gr->gr_gid);
        int i = 0;
        while (gr->gr_mem[i++])
            ;
        jobjectArray mems;
         mems = (env)->NewObjectArray(i, jcstring, 0);
        //env->SetIntField(group, gGroupMemFieldID, gr->mem);
        for (--i; i >= 0; i--) {
            strTemp = env->NewStringUTF(gr->gr_mem[i]);
            (env)->SetObjectArrayElement(mems, i, strTemp);
            (env)->DeleteLocalRef(strTemp);
        }
        env->SetObjectField(group, gGroupMemFieldID, mems);
        (env)->DeleteLocalRef(mems);
        return group;
    }

    JNIEXPORT jint JNICALL Java_android_os_FileHelper_chown(JNIEnv *env, jobject clazz, jstring file, jint uid, jint gid) {
        int res;
        char *str = j2c(env, file);
        if (strlen(str) <= 0 || !str)
            return ENOENT;
        if (uid >= 0 || gid >= 0) {
            res = chown(str, uid, gid);
            if (res != 0)
                return errno;
        }
        return res;
    }

    JNIEXPORT jint JNICALL Java_android_os_FileHelper_chmod(JNIEnv *env, jobject clazz, jstring file, jint mode) {
        char *str = j2c(env, file);
        if (strlen(str) <= 0 || !str)
            return ENOENT;
        return chmod(str, mode) == 0 ? 0 : errno;
    }

#ifndef VFAT_IOCTL_GET_VOLUME_ID
#define VFAT_IOCTL_GET_VOLUME_ID _IOR('r', 0x12, __u32)
#endif

    JNIEXPORT jint JNICALL Java_android_os_FileHelper_getFatVolumeId(JNIEnv *env, jobject clazz, jstring path) {
        if (path == NULL) {
            exit(0);
            return -1;
        }
        const char *pathStr = env->GetStringUTFChars(path, NULL);
        int result = -1;
        int fd = open(pathStr, O_RDONLY);
        if (fd >= 0) {
            result = ioctl(fd, VFAT_IOCTL_GET_VOLUME_ID);
            close(fd);
        }
        env->ReleaseStringUTFChars(path, pathStr);
        return result;
    }

}
#ifdef __cplusplus
}
#endif
