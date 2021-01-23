#ifndef ANDROID_BINDER_DEBUG_H
#define ANDROID_BINDER_DEBUG_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "alog.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *stringForIndent(int32_t indentLevel);

typedef void (*debugPrintFunc)(void *cookie, const char *txt);

void printTypeCode(uint32_t typeCode,
                   debugPrintFunc func = 0, void *cookie = 0);

void printHexData(int32_t indent, const void *buf, size_t length,
                  size_t bytesPerLine = 16, int32_t singleLineBytesCutoff = 16,
                  size_t alignment = 0, bool cArrayStyle = false,
                  debugPrintFunc func = 0, void *cookie = 0);

__inline__ void print_string_hex(char *comment, unsigned char *str, size_t len) {
    unsigned char *c;
    ALOGI("%s\n", comment);
    for (c = str; c < str + len; c++)
        ALOGI("0x%02x ", *c & 0xff);
    ALOGI("\n");
}

#ifdef __cplusplus
}
#endif

#endif
