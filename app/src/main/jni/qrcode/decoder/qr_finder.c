#include <config.h>
#include <assert.h>

#include <zbar.h>

#ifdef DEBUG_QR_FINDER
# define DEBUG_LEVEL (DEBUG_QR_FINDER)
#endif
#include "debug.h"
#include "decoder.h"

qr_finder_line *_zbar_decoder_get_qr_finder_line (zbar_decoder_t *dcode)
{
    return(&dcode->qrf.line);
}

zbar_symbol_type_t _zbar_find_qr (zbar_decoder_t *dcode)
{
    qr_finder_t *qrf = &dcode->qrf;
    unsigned s, qz, w;
    int ei;

    qrf->s5 -= get_width(dcode, 6);
    qrf->s5 += get_width(dcode, 1);
    s = qrf->s5;

    if(get_color(dcode) != ZBAR_SPACE || s < 7)
        return(0);

    dbprintf(2, "    qrf: s=%d", s);

    ei = decode_e(pair_width(dcode, 1), s, 7);
    dbprintf(2, " %d", ei);
    if(ei)
        goto invalid;

    ei = decode_e(pair_width(dcode, 2), s, 7);
    dbprintf(2, "%d", ei);
    if(ei != 2)
        goto invalid;

    ei = decode_e(pair_width(dcode, 3), s, 7);
    dbprintf(2, "%d", ei);
    if(ei != 2)
        goto invalid;

    ei = decode_e(pair_width(dcode, 4), s, 7);
    dbprintf(2, "%d", ei);
    if(ei)
        goto invalid;

    qz = get_width(dcode, 0);
    w = get_width(dcode, 1);
    qrf->line.eoffs = qz + (w + 1) / 2;
    qrf->line.len = qz + w + get_width(dcode, 2);
    qrf->line.pos[0] = qrf->line.len + get_width(dcode, 3);
    qrf->line.pos[1] = qrf->line.pos[0];
    w = get_width(dcode, 5);
    qrf->line.boffs = qrf->line.pos[0] + get_width(dcode, 4) + (w + 1) / 2;

    dbprintf(2, " boff=%d pos=%d len=%d eoff=%d [valid]\n",
             qrf->line.boffs, qrf->line.pos[0], qrf->line.len,
             qrf->line.eoffs);

    dcode->direction = 0;
    dcode->buflen = 0;
    return(ZBAR_QRCODE);

invalid:
    dbprintf(2, " [invalid]\n");
    return(0);
}
