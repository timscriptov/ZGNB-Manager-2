#ifndef _PDF417_H_
#define _PDF417_H_

typedef struct pdf417_decoder_s {
    unsigned direction : 1;
    unsigned element : 3;
    int character : 12;
    unsigned s8;

    unsigned config;
    int configs[NUM_CFGS];
} pdf417_decoder_t;

static inline void pdf417_reset (pdf417_decoder_t *pdf417)
{
    pdf417->direction = 0;
    pdf417->element = 0;
    pdf417->character = -1;
    pdf417->s8 = 0;
}

zbar_symbol_type_t _zbar_decode_pdf417(zbar_decoder_t *dcode);

#endif
