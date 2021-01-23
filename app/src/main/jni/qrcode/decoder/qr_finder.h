#ifndef _DECODER_QR_FINDER_H_
#define _DECODER_QR_FINDER_H_

#include "qrcode.h"

typedef struct qr_finder_s {
    unsigned s5;
    qr_finder_line line;

    unsigned config;
} qr_finder_t;

static inline void qr_finder_reset (qr_finder_t *qrf)
{
    qrf->s5 = 0;
}

zbar_symbol_type_t _zbar_find_qr (zbar_decoder_t *dcode);

#endif
