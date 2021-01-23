

#include <config.h>
#include <string.h>     

#include <zbar.h>

#ifdef DEBUG_CODE39
# define DEBUG_LEVEL (DEBUG_CODE39)
#endif
#include "debug.h"
#include "decoder.h"

#define NUM_CHARS (0x2c)

static const unsigned char code39_hi[32] = {
    0x80 | 0x00,  
    0x40 | 0x02,  
    0x80 | 0x06,  
    0xc0 | 0x08,  
    0x40 | 0x0a,  
    0x80 | 0x0e,  
    0xc0 | 0x10,  
    0x00 | 0x12,  

    0x80 | 0x13,  
    0xc0 | 0x15,  
    0x80 | 0x17,  
    0xff,
    0xc0 | 0x19,  
    0x00 | 0x1b,  
    0xff,
    0xff,

    0x40 | 0x1c,  
    0x80 | 0x20,  
    0xc0 | 0x22,  
    0x00 | 0x24,  
    0x80 | 0x25,  
    0xff,
    0x00 | 0x27,  
    0xff,

    0xc0 | 0x28,  
    0x00 | 0x2a,  
    0xff,
    0xff,
    0x00 | 0x2b,  
    0xff,
    0xff,
    0xff,
};

typedef struct char39_s {
    unsigned char chk, rev, fwd;
} char39_t;

static const char39_t code39_encodings[NUM_CHARS] = {
    { 0x07, 0x1a, 0x20 }, 
    { 0x0d, 0x10, 0x03 }, 
    { 0x13, 0x17, 0x22 }, 
    { 0x16, 0x1d, 0x23 }, 
    { 0x19, 0x0d, 0x05 }, 
    { 0x1c, 0x13, 0x06 }, 
    { 0x25, 0x07, 0x0c }, 
    { 0x2a, 0x2a, 0x27 }, 
    { 0x31, 0x04, 0x0e }, 
    { 0x34, 0x00, 0x0f }, 
    { 0x43, 0x15, 0x25 }, 
    { 0x46, 0x1c, 0x26 }, 
    { 0x49, 0x0b, 0x08 }, 
    { 0x4c, 0x12, 0x09 }, 
    { 0x52, 0x19, 0x2b }, 
    { 0x58, 0x0f, 0x00 }, 
    { 0x61, 0x02, 0x11 }, 
    { 0x64, 0x09, 0x12 }, 
    { 0x70, 0x06, 0x13 }, 
    { 0x85, 0x24, 0x16 }, 
    { 0x8a, 0x29, 0x28 }, 
    { 0x91, 0x21, 0x18 }, 
    { 0x94, 0x2b, 0x19 }, 
    { 0xa2, 0x28, 0x29 }, 
    { 0xa8, 0x27, 0x2a }, 
    { 0xc1, 0x1f, 0x1b }, 
    { 0xc4, 0x26, 0x1c }, 
    { 0xd0, 0x23, 0x1d }, 
    { 0x03, 0x14, 0x1e }, 
    { 0x06, 0x1b, 0x1f }, 
    { 0x09, 0x0a, 0x01 }, 
    { 0x0c, 0x11, 0x02 }, 
    { 0x12, 0x18, 0x21 }, 
    { 0x18, 0x0e, 0x04 }, 
    { 0x21, 0x01, 0x0a }, 
    { 0x24, 0x08, 0x0b }, 
    { 0x30, 0x05, 0x0d }, 
    { 0x42, 0x16, 0x24 }, 
    { 0x48, 0x0c, 0x07 }, 
    { 0x60, 0x03, 0x10 }, 
    { 0x81, 0x1e, 0x14 }, 
    { 0x84, 0x25, 0x15 }, 
    { 0x90, 0x22, 0x17 }, 
    { 0xc0, 0x20, 0x1a }, 
};

static const unsigned char code39_characters[NUM_CHARS] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%*";

static inline unsigned char code39_decode1 (unsigned char enc,
                                            unsigned e,
                                            unsigned s)
{
    unsigned char E = decode_e(e, s, 72);
    if(E > 18)
        return(0xff);
    enc <<= 1;
    if(E > 6) {
        enc |= 1;
        dbprintf(2, "1");
    }
    else
        dbprintf(2, "0");
    return(enc);
}

static inline signed char code39_decode9 (zbar_decoder_t *dcode)
{
    code39_decoder_t *dcode39 = &dcode->code39;

    if(dcode39->s9 < 9)
        return(-1);

    
    unsigned char i, enc = 0;
    for(i = 0; i < 5; i++) {
        enc = code39_decode1(enc, get_width(dcode, i), dcode39->s9);
        if(enc == 0xff)
            return(-1);
    }
    zassert(enc < 0x20, -1, " enc=%x s9=%x\n", enc, dcode39->s9);

    
    unsigned char idx = code39_hi[enc];
    if(idx == 0xff)
        return(-1);

    
    for(; i < 9; i++) {
        enc = code39_decode1(enc, get_width(dcode, i), dcode39->s9);
        if(enc == 0xff)
            return(-1);
    }

    if((idx & 0xc0) == 0x80)
        idx = (idx & 0x3f) + ((enc >> 3) & 1);
    else if((idx & 0xc0) == 0xc0)
        idx = (idx & 0x3f) + ((enc >> 2) & 1);
    else if(idx & 0xc0)
        idx = (idx & 0x3f) + ((enc >> 2) & 3);
    zassert(idx < 0x2c, -1, " idx=%x enc=%x s9=%x\n", idx, enc, dcode39->s9);

    const char39_t *c = &code39_encodings[idx];
    dbprintf(2, " i=%02x chk=%02x c=%02x/%02x", idx, c->chk, c->fwd, c->rev);
    if(enc != c->chk)
        return(-1);

    dcode39->width = dcode39->s9;
    return((dcode39->direction) ? c->rev : c->fwd);
}

static inline signed char code39_decode_start (zbar_decoder_t *dcode)
{
    code39_decoder_t *dcode39 = &dcode->code39;
    dbprintf(2, " s=%d ", dcode39->s9);

    signed char c = code39_decode9(dcode);
    if(c != 0x19 && c != 0x2b) {
        dbprintf(2, "\n");
        return(ZBAR_NONE);
    }
    dcode39->direction ^= (c == 0x19);

    
    unsigned quiet = get_width(dcode, 9);
    if(quiet && quiet < dcode39->s9 / 2) {
        dbprintf(2, " [invalid quiet]\n");
        return(ZBAR_NONE);
    }

    dcode39->element = 9;
    dcode39->character = 0;
    dbprintf(1, " dir=%x [valid start]\n", dcode39->direction);
    return(ZBAR_PARTIAL);
}

static inline int code39_postprocess (zbar_decoder_t *dcode)
{
    code39_decoder_t *dcode39 = &dcode->code39;
    dcode->direction = 1 - 2 * dcode39->direction;
    int i;
    if(dcode39->direction) {
        
        dbprintf(2, " (rev)");
        for(i = 0; i < dcode39->character / 2; i++) {
            unsigned j = dcode39->character - 1 - i;
            char code = dcode->buf[i];
            dcode->buf[i] = dcode->buf[j];
            dcode->buf[j] = code;
        }
    }
    for(i = 0; i < dcode39->character; i++)
        dcode->buf[i] = ((dcode->buf[i] < 0x2b)
                         ? code39_characters[(unsigned)dcode->buf[i]]
                         : '?');
    zassert(i < dcode->buf_alloc, -1, "i=%02x %s\n", i,
            _zbar_decoder_buf_dump(dcode->buf, dcode39->character));
    dcode->buflen = i;
    dcode->buf[i] = '\0';
    dcode->modifiers = 0;
    return(0);
}

static inline int
check_width (unsigned ref,
             unsigned w)
{
    unsigned dref = ref;
    ref *= 4;
    w *= 4;
    return(ref - dref <= w && w <= ref + dref);
}

zbar_symbol_type_t _zbar_decode_code39 (zbar_decoder_t *dcode)
{
    code39_decoder_t *dcode39 = &dcode->code39;

    
    dcode39->s9 -= get_width(dcode, 9);
    dcode39->s9 += get_width(dcode, 0);

    if(dcode39->character < 0) {
        if(get_color(dcode) != ZBAR_BAR)
            return(ZBAR_NONE);
        dbprintf(2, "      code39:");
        return(code39_decode_start(dcode));
    }

    if(++dcode39->element < 9)
        return(ZBAR_NONE);

    dbprintf(2, "      code39[%c%02d+%x]",
             (dcode39->direction) ? '<' : '>',
             dcode39->character, dcode39->element);

    if(dcode39->element == 10) {
        unsigned space = get_width(dcode, 0);
        if(dcode39->character &&
           dcode->buf[dcode39->character - 1] == 0x2b) {  
            
            dcode39->character--;
            zbar_symbol_type_t sym = ZBAR_NONE;

            
            if(space && space < dcode39->width / 2)
                dbprintf(2, " [invalid qz]\n");
            else if(dcode39->character < CFG(*dcode39, ZBAR_CFG_MIN_LEN) ||
                    (CFG(*dcode39, ZBAR_CFG_MAX_LEN) > 0 &&
                     dcode39->character > CFG(*dcode39, ZBAR_CFG_MAX_LEN)))
                dbprintf(2, " [invalid len]\n");
            else if(!code39_postprocess(dcode)) {
                
                dbprintf(2, " [valid end]\n");
                sym = ZBAR_CODE39;
            }
            dcode39->character = -1;
            if(!sym)
                release_lock(dcode, ZBAR_CODE39);
            return(sym);
        }
        if(space > dcode39->width / 2) {
            
            dbprintf(2, " ics>%d [invalid ics]", dcode39->width);
            if(dcode39->character)
                release_lock(dcode, ZBAR_CODE39);
            dcode39->character = -1;
        }
        dcode39->element = 0;
        dbprintf(2, "\n");
        return(ZBAR_NONE);
    }

    dbprintf(2, " s=%d ", dcode39->s9);
    if(!check_width(dcode39->width, dcode39->s9)) {
        dbprintf(2, " [width]\n");
        if(dcode39->character)
            release_lock(dcode, ZBAR_CODE39);
        dcode39->character = -1;
        return(ZBAR_NONE);
    }

    signed char c = code39_decode9(dcode);
    dbprintf(2, " c=%d", c);

    
    if(!dcode39->character && acquire_lock(dcode, ZBAR_CODE39)) {
        dcode39->character = -1;
        return(ZBAR_PARTIAL);
    }

    if(c < 0 || size_buf(dcode, dcode39->character + 1)) {
        dbprintf(1, (c < 0) ? " [aborted]\n" : " [overflow]\n");
        release_lock(dcode, ZBAR_CODE39);
        dcode39->character = -1;
        return(ZBAR_NONE);
    }
    else {
        zassert(c < 0x2c, ZBAR_NONE, "c=%02x s9=%x\n", c, dcode39->s9);
        dbprintf(2, "\n");
    }

    dcode->buf[dcode39->character++] = c;

    return(ZBAR_NONE);
}
