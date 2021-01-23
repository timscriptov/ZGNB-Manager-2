#ifndef HELLO_DUMP_DEX_H
#define HELLO_DUMP_DEX_H

#include <jni.h>
#include <string>

#include <vector>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "alog.h"

#include "DexUtil.h"
#include "Debug.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline std::string get_main_classdef(const char *main_app_class) {
    std::string main_app_classdef = "L";
    main_app_classdef += main_app_class;
    main_app_classdef += ";";
    return main_app_classdef;
}

int dump_dex(JNIEnv *env, jint api_level, const char *MAIN_APP_CLASS);

typedef enum  {
    under_android_4 = 3, android4x, android5, android6, android7,
    android8, android9
} Android_Version;

typedef struct {
    Android_Version maj_version;
    std::string min_version;
} Version_Info;

static Version_Info minfo;

static std::string get_android_os_version() {
    std::string version = std::to_string(minfo.maj_version);
    version.append(".").append(minfo.min_version);
    return version;
}

static Android_Version get_os_major_version() {
    return minfo.maj_version;
}

static void set_apilevel(jint apil) {
    if(apil < 14) {
        minfo.maj_version = under_android_4;
        minfo.min_version = "0";
    } else if (apil == 14) {
        minfo.maj_version = android4x;
        minfo.min_version = "0.2" ;
    } else if(apil == 15) {
        minfo.maj_version = android4x;
        minfo.min_version = "0.4" ;
    } else if(apil == 16) {
        minfo.maj_version = android4x;
        minfo.min_version = "1" ;
    } else if(apil == 17) {
        minfo.maj_version = android4x;
        minfo.min_version = "2" ;
    } else if(apil == 18) {
        minfo.maj_version = android4x;
        minfo.min_version = "3" ;
    } else if(apil == 19) {
        minfo.maj_version = android4x;
        minfo.min_version = "4" ;
    } else if(apil == 21) {
        minfo.maj_version = android5;
        minfo.min_version = "0" ;
    } else if(apil == 22) {
        minfo.maj_version = android5;
        minfo.min_version = "1" ;
    } else if(apil == 23) {
        minfo.maj_version = android6;
        minfo.min_version = "0" ;
    } else if(apil == 24) {
        minfo.maj_version = android7;
        minfo.min_version = "0" ;
    } else if(apil == 25) {
        minfo.maj_version = android7;
        minfo.min_version = "1" ;
    } else if(apil == 26) {
        minfo.maj_version = android8;
        minfo.min_version = "0" ;
    } else if(apil == 27) {
        minfo.maj_version = android8;
        minfo.min_version = "1" ;
    } else if(apil == 28) {
        minfo.maj_version = android9;
        minfo.min_version = "0" ;
    }
}

static void  *convert_java_array_to_dexfiles(JNIEnv *env, jlong cookie1, jint cookie2) ;

#ifdef __cplusplus
}
#endif
#endif
