#include <cstdio>
#include <cstdlib>
#include "com_zengge_nbmanager_Features.h"
#include <fstream>
#include <string>
#include <android/log.h>

using namespace std;

void mydebug(const char *msg) {
    __android_log_print(ANDROID_LOG_INFO, "Perez jni Main Function", "%s", msg);
}
