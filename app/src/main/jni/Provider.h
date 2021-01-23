#ifndef __PROVIDER_H_
#define __PROVIDER_H_

#include <string.h>
#include <jni.h>
#include <android/log.h>

extern JavaVM *gTelnetJvm;
extern JavaVM *gJvm;

void PrintTraceInfo(const char *aStrToPrint);

void PrintSocketInfo(const char *aStrToPrint);

#endif