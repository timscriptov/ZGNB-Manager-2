#ifndef LIBDEX_LEB128_H_
#define LIBDEX_LEB128_H_

#include "Types.h"

#ifndef _DEX_GEN_INLINES
# define DEX_INLINE extern __inline__
#else
# define DEX_INLINE
#endif

DEX_INLINE int readUnsignedLeb128(const u1 **pStream) {
    const u1 *ptr = *pStream;
    int result = *(ptr++);
    if (result > 0x7f) {
        int cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur > 0x7f) {
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur > 0x7f) {
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur > 0x7f) {

                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }
    *pStream = ptr;
    return result;
}

DEX_INLINE int readSignedLeb128(const u1 **pStream) {
    const u1 *ptr = *pStream;
    int result = *(ptr++);
    if (result <= 0x7f)
        result = (result << 25) >> 25;
    else {
        int cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur <= 0x7f)
            result = (result << 18) >> 18;
        else {
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur <= 0x7f)
                result = (result << 11) >> 11;
            else {
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur <= 0x7f)
                    result = (result << 4) >> 4;
                else {

                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }
    *pStream = ptr;
    return result;
}

int readAndVerifyUnsignedLeb128(const u1 **pStream, const u1 *limit,
                                bool *okay);

int readAndVerifySignedLeb128(const u1 **pStream, const u1 *limit, bool *okay);

DEX_INLINE u1 *writeUnsignedLeb128(u1 *ptr, u4 data) {
    while (true) {
        u1 out = data & 0x7f;
        if (out != data) {
            *ptr++ = out | 0x80;
            data >>= 7;
        } else {
            *ptr++ = out;
            break;
        }
    }
    return ptr;
}

DEX_INLINE int unsignedLeb128Size(u4 data) {
    int count = 0;
    do {
        data >>= 7;
        count++;
    } while (data != 0);
    return count;
}

#endif
