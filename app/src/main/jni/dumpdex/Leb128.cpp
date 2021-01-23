#include "Leb128.h"

int readAndVerifyUnsignedLeb128(const u1 **pStream, const u1 *limit,
                                bool *okay) {
    const u1 *ptr = *pStream;
    int result = readUnsignedLeb128(pStream);
    if (((limit != NULL) && (*pStream > limit))
            || (((*pStream - ptr) == 5) && (ptr[4] > 0x0f)))
        *okay = false;
    return result;
}

int readAndVerifySignedLeb128(const u1 **pStream, const u1 *limit,
                              bool *okay) {
    const u1 *ptr = *pStream;
    int result = readSignedLeb128(pStream);
    if (((limit != NULL) && (*pStream > limit))
            || (((*pStream - ptr) == 5) && (ptr[4] > 0x0f)))
        *okay = false;
    return result;
}
