#ifndef _I25_H_
#define _I25_H_

typedef struct i25_decoder_s {
    unsigned direction : 1;
    unsigned element : 4;
    int character : 12;
    unsigned s10;
    unsigned width;
    unsigned char buf[4];

    unsigned config;
    int configs[NUM_CFGS];
} i25_decoder_t;

static inline void i25_reset (i25_decoder_t *i25)
{
    i25->direction = 0;
    i25->element = 0;
    i25->character = -1;
    i25->s10 = 0;
}

zbar_symbol_type_t _zbar_decode_i25(zbar_decoder_t *dcode);

#endif
