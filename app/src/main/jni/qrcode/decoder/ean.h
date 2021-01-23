#ifndef _EAN_H_
#define _EAN_H_

typedef struct ean_pass_s {
    signed char state;
#define STATE_REV   0x80
#define STATE_ADDON 0x40
#define STATE_IDX   0x3f
    unsigned width;
    unsigned char raw[7];
} ean_pass_t;

typedef struct ean_decoder_s {
    ean_pass_t pass[4];
    zbar_symbol_type_t left;
    zbar_symbol_type_t right;
    int direction;
    unsigned s4, width;
    signed char buf[18];

    signed char enable;
    unsigned ean13_config;
    unsigned ean8_config;
    unsigned upca_config;
    unsigned upce_config;
    unsigned isbn10_config;
    unsigned isbn13_config;
    unsigned ean5_config;
    unsigned ean2_config;
} ean_decoder_t;

static inline void ean_new_scan (ean_decoder_t *ean)
{
    ean->pass[0].state = ean->pass[1].state = -1;
    ean->pass[2].state = ean->pass[3].state = -1;
    ean->s4 = 0;
}

static inline void ean_reset (ean_decoder_t *ean)
{
    ean_new_scan(ean);
    ean->left = ean->right = ZBAR_NONE;
}

static inline unsigned ean_get_config (ean_decoder_t *ean,
                                       zbar_symbol_type_t sym)
{
    switch(sym) {
    case ZBAR_EAN2:   return(ean->ean2_config);
    case ZBAR_EAN5:   return(ean->ean5_config);
    case ZBAR_EAN8:   return(ean->ean8_config);
    case ZBAR_UPCE:   return(ean->upce_config);
    case ZBAR_ISBN10: return(ean->isbn10_config);
    case ZBAR_UPCA:   return(ean->upca_config);
    case ZBAR_EAN13:  return(ean->ean13_config);
    case ZBAR_ISBN13: return(ean->isbn13_config);
    default:          return(0);
    }
}

zbar_symbol_type_t _zbar_decode_ean(zbar_decoder_t *dcode);

#endif
