#include "image.h"
#include "video.h"
#include "window.h"

#define RGB_BITS(off, size) ((((8 - (size)) & 0x7) << 5) | ((off) & 0x1f))

typedef void (conversion_handler_t)(zbar_image_t*,
                                    const zbar_format_def_t*,
                                    const zbar_image_t*,
                                    const zbar_format_def_t*);

typedef struct conversion_def_s {
    int cost;
    conversion_handler_t *func;
} conversion_def_t;

const uint32_t _zbar_formats[] = {

    fourcc('4','2','2','P'),
    fourcc('I','4','2','0'),
    fourcc('Y','U','1','2'),
    fourcc('Y','V','1','2'),
    fourcc('4','1','1','P'),

    fourcc('N','V','1','2'),
    fourcc('N','V','2','1'),

    fourcc('Y','U','Y','V'),
    fourcc('U','Y','V','Y'),
    fourcc('Y','U','Y','2'),
    fourcc('Y','U','V','4'),

    fourcc('R','G','B','3'),
    fourcc( 3 , 0 , 0 , 0 ),
    fourcc('B','G','R','3'),
    fourcc('R','G','B','4'),
    fourcc('B','G','R','4'),

    fourcc('R','G','B','P'),
    fourcc('R','G','B','O'),
    fourcc('R','G','B','R'),
    fourcc('R','G','B','Q'),

    fourcc('Y','U','V','9'),
    fourcc('Y','V','U','9'),

    fourcc('G','R','E','Y'),
    fourcc('Y','8','0','0'),
    fourcc('Y','8',' ',' '),
    fourcc('Y','8', 0 , 0 ),

    fourcc('R','G','B','1'),
    fourcc('R','4','4','4'),
    fourcc('B','A','8','1'),

    fourcc('Y','4','1','P'),
    fourcc('Y','4','4','4'),
    fourcc('Y','U','V','O'),
    fourcc('H','M','1','2'),

    fourcc('H','I','2','4'),

    fourcc('J','P','E','G'),
    fourcc('M','J','P','G'),
    fourcc('M','P','E','G'),

    0
};

const int _zbar_num_formats = sizeof(_zbar_formats) / sizeof(uint32_t);

static const zbar_format_def_t format_defs[] = {

    { fourcc('R','G','B','4'), ZBAR_FMT_RGB_PACKED,
        { { 4, RGB_BITS(8, 8), RGB_BITS(16, 8), RGB_BITS(24, 8) } } },
    { fourcc('B','G','R','1'), ZBAR_FMT_RGB_PACKED,
        { { 1, RGB_BITS(0, 3), RGB_BITS(3, 3), RGB_BITS(6, 2) } } },
    { fourcc('4','2','2','P'), ZBAR_FMT_YUV_PLANAR, { { 1, 0, 0  } } },
    { fourcc('Y','8','0','0'), ZBAR_FMT_GRAY, },
    { fourcc('Y','U','Y','2'), ZBAR_FMT_YUV_PACKED,
        { { 1, 0, 0,  } } },
    { fourcc('J','P','E','G'), ZBAR_FMT_JPEG, },
    { fourcc('Y','V','Y','U'), ZBAR_FMT_YUV_PACKED,
        { { 1, 0, 1,  } } },
    { fourcc('Y','8', 0 , 0 ), ZBAR_FMT_GRAY, },
    { fourcc('N','V','2','1'), ZBAR_FMT_YUV_NV,     { { 1, 1, 1  } } },
    { fourcc('N','V','1','2'), ZBAR_FMT_YUV_NV,     { { 1, 1, 0  } } },
    { fourcc('B','G','R','3'), ZBAR_FMT_RGB_PACKED,
        { { 3, RGB_BITS(16, 8), RGB_BITS(8, 8), RGB_BITS(0, 8) } } },
    { fourcc('Y','V','U','9'), ZBAR_FMT_YUV_PLANAR, { { 2, 2, 1  } } },
    { fourcc('R','G','B','O'), ZBAR_FMT_RGB_PACKED,
        { { 2, RGB_BITS(10, 5), RGB_BITS(5, 5), RGB_BITS(0, 5) } } },
    { fourcc('R','G','B','Q'), ZBAR_FMT_RGB_PACKED,
        { { 2, RGB_BITS(2, 5), RGB_BITS(13, 5), RGB_BITS(8, 5) } } },
    { fourcc('G','R','E','Y'), ZBAR_FMT_GRAY, },
    { fourcc( 3 , 0 , 0 , 0 ), ZBAR_FMT_RGB_PACKED,
        { { 4, RGB_BITS(16, 8), RGB_BITS(8, 8), RGB_BITS(0, 8) } } },
    { fourcc('Y','8',' ',' '), ZBAR_FMT_GRAY, },
    { fourcc('I','4','2','0'), ZBAR_FMT_YUV_PLANAR, { { 1, 1, 0  } } },
    { fourcc('R','G','B','1'), ZBAR_FMT_RGB_PACKED,
        { { 1, RGB_BITS(5, 3), RGB_BITS(2, 3), RGB_BITS(0, 2) } } },
    { fourcc('Y','U','1','2'), ZBAR_FMT_YUV_PLANAR, { { 1, 1, 0  } } },
    { fourcc('Y','V','1','2'), ZBAR_FMT_YUV_PLANAR, { { 1, 1, 1  } } },
    { fourcc('R','G','B','3'), ZBAR_FMT_RGB_PACKED,
        { { 3, RGB_BITS(0, 8), RGB_BITS(8, 8), RGB_BITS(16, 8) } } },
    { fourcc('R','4','4','4'), ZBAR_FMT_RGB_PACKED,
        { { 2, RGB_BITS(8, 4), RGB_BITS(4, 4), RGB_BITS(0, 4) } } },
    { fourcc('B','G','R','4'), ZBAR_FMT_RGB_PACKED,
        { { 4, RGB_BITS(16, 8), RGB_BITS(8, 8), RGB_BITS(0, 8) } } },
    { fourcc('Y','U','V','9'), ZBAR_FMT_YUV_PLANAR, { { 2, 2, 0  } } },
    { fourcc('M','J','P','G'), ZBAR_FMT_JPEG, },
    { fourcc('4','1','1','P'), ZBAR_FMT_YUV_PLANAR, { { 2, 0, 0  } } },
    { fourcc('R','G','B','P'), ZBAR_FMT_RGB_PACKED,
        { { 2, RGB_BITS(11, 5), RGB_BITS(5, 6), RGB_BITS(0, 5) } } },
    { fourcc('R','G','B','R'), ZBAR_FMT_RGB_PACKED,
        { { 2, RGB_BITS(3, 5), RGB_BITS(13, 6), RGB_BITS(8, 5) } } },
    { fourcc('Y','U','Y','V'), ZBAR_FMT_YUV_PACKED,
        { { 1, 0, 0,  } } },
    { fourcc('U','Y','V','Y'), ZBAR_FMT_YUV_PACKED,
        { { 1, 0, 2,  } } },
};

static const int num_format_defs =
    sizeof(format_defs) / sizeof(zbar_format_def_t);

#ifdef DEBUG_CONVERT
static int intsort (const void *a,
                    const void *b)
{
    return(*(uint32_t*)a - *(uint32_t*)b);
}
#endif

static inline int verify_format_sort (void)
{
    int i;
    for(i = 0; i < num_format_defs; i++) {
        int j = i * 2 + 1;
        if((j < num_format_defs &&
            format_defs[i].format < format_defs[j].format) ||
           (j + 1 < num_format_defs &&
            format_defs[j + 1].format < format_defs[i].format))
            break;
    }
    if(i == num_format_defs)
        return(0);

    fprintf(stderr, "ERROR: image format list is not sorted!?\n");

#ifdef DEBUG_CONVERT
    assert(num_format_defs);
    uint32_t sorted[num_format_defs];
    uint32_t ordered[num_format_defs];
    for(i = 0; i < num_format_defs; i++)
        sorted[i] = format_defs[i].format;
    qsort(sorted, num_format_defs, sizeof(uint32_t), intsort);
    for(i = 0; i < num_format_defs; i = i << 1 | 1);
    i = (i - 1) / 2;
    ordered[i] = sorted[0];
    int j, k;
    for(j = 1; j < num_format_defs; j++) {
        k = i * 2 + 2;
        if(k < num_format_defs) {
            i = k;
            for(k = k * 2 + 1; k < num_format_defs; k = k * 2 + 1)
                i = k;
        }
        else {
            for(k = (i - 1) / 2; i != k * 2 + 1; k = (i - 1) / 2) {
                assert(i);
                i = k;
            }
            i = k;
        }
        ordered[i] = sorted[j];
    }
    fprintf(stderr, "correct sort order is:");
    for(i = 0; i < num_format_defs; i++)
        fprintf(stderr, " %4.4s", (char*)&ordered[i]);
    fprintf(stderr, "\n");
#endif
    return(-1);
}

static inline void uv_round (zbar_image_t *img,
                             const zbar_format_def_t *fmt)
{
    img->width >>= fmt->p.yuv.xsub2;
    img->width <<= fmt->p.yuv.xsub2;
    img->height >>= fmt->p.yuv.ysub2;
    img->height <<= fmt->p.yuv.ysub2;
}

static inline void uv_roundup (zbar_image_t *img,
                               const zbar_format_def_t *fmt)
{
    unsigned xmask, ymask;
    if(fmt->group == ZBAR_FMT_GRAY)
        return;
    xmask = (1 << fmt->p.yuv.xsub2) - 1;
    if(img->width & xmask)
        img->width = (img->width + xmask) & ~xmask;
    ymask = (1 << fmt->p.yuv.ysub2) - 1;
    if(img->height & ymask)
        img->height = (img->height + ymask) & ~ymask;
}

static inline unsigned long uvp_size (const zbar_image_t *img,
                                      const zbar_format_def_t *fmt)
{
    if(fmt->group == ZBAR_FMT_GRAY)
        return(0);
    return((img->width >> fmt->p.yuv.xsub2) *
           (img->height >> fmt->p.yuv.ysub2));
}

static inline uint32_t convert_read_rgb (const uint8_t *srcp,
                                         int bpp)
{
    uint32_t p;
    if(bpp == 3) {
        p = *srcp;
        p |= *(srcp + 1) << 8;
        p |= *(srcp + 2) << 16;
    }
    else if(bpp == 4)
        p = *((uint32_t*)(srcp));
    else if(bpp == 2)
        p = *((uint16_t*)(srcp));
    else
        p = *srcp;
    return(p);
}

static inline void convert_write_rgb (uint8_t *dstp,
                                      uint32_t p,
                                      int bpp)
{
    if(bpp == 3) {
        *dstp = p & 0xff;
        *(dstp + 1) = (p >> 8) & 0xff;
        *(dstp + 2) = (p >> 16) & 0xff;
    }
    else if(bpp == 4)
        *((uint32_t*)dstp) = p;
    else if(bpp == 2)
        *((uint16_t*)dstp) = p;
    else
        *dstp = p;
}

static void cleanup_ref (zbar_image_t *img)
{
    if(img->next)
        _zbar_image_refcnt(img->next, -1);
}

static inline void convert_y_resize (zbar_image_t *dst,
                                     const zbar_format_def_t *dstfmt,
                                     const zbar_image_t *src,
                                     const zbar_format_def_t *srcfmt,
                                     size_t n)
{
    uint8_t *psrc, *pdst;
    unsigned width, height, xpad, y;

    if(dst->width == src->width && dst->height == src->height) {
        memcpy((void*)dst->data, src->data, n);
        return;
    }
    psrc = (void*)src->data;
    pdst = (void*)dst->data;
    width = (dst->width > src->width) ? src->width : dst->width;
    xpad = (dst->width > src->width) ? dst->width - src->width : 0;
    height = (dst->height > src->height) ? src->height : dst->height;
    for(y = 0; y < height; y++) {
        memcpy(pdst, psrc, width);
        pdst += width;
        psrc += src->width;
        if(xpad) {
            memset(pdst, *(psrc - 1), xpad);
            pdst += xpad;
        }
    }
    psrc -= src->width;
    for(; y < dst->height; y++) {
        memcpy(pdst, psrc, width);
        pdst += width;
        if(xpad) {
            memset(pdst, *(psrc - 1), xpad);
            pdst += xpad;
        }
    }
}

static void convert_copy (zbar_image_t *dst,
                          const zbar_format_def_t *dstfmt,
                          const zbar_image_t *src,
                          const zbar_format_def_t *srcfmt)
{
    if(src->width == dst->width &&
       src->height == dst->height) {
        zbar_image_t *s = (zbar_image_t*)src;
        dst->data = src->data;
        dst->datalen = src->datalen;
        dst->cleanup = cleanup_ref;
        dst->next = s;
        _zbar_image_refcnt(s, 1);
    }
    else

        convert_y_resize(dst, dstfmt, src, srcfmt, dst->width * dst->height);
}

static void convert_uvp_append (zbar_image_t *dst,
                                const zbar_format_def_t *dstfmt,
                                const zbar_image_t *src,
                                const zbar_format_def_t *srcfmt)
{
    unsigned long n;
    uv_roundup(dst, dstfmt);
    dst->datalen = uvp_size(dst, dstfmt) * 2;
    n = dst->width * dst->height;
    dst->datalen += n;
    assert(src->datalen >= src->width * src->height);
    zprintf(24, "dst=%dx%d (%lx) %lx src=%dx%d %lx\n",
            dst->width, dst->height, n, dst->datalen,
            src->width, src->height, src->datalen);
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    convert_y_resize(dst, dstfmt, src, srcfmt, n);
    memset((uint8_t*)dst->data + n, 0x80, dst->datalen - n);
}

static void convert_yuv_pack (zbar_image_t *dst,
                              const zbar_format_def_t *dstfmt,
                              const zbar_image_t *src,
                              const zbar_format_def_t *srcfmt)
{
    unsigned long srcm, srcn;
    uint8_t flags, *srcy, *dstp;
    const uint8_t *srcu, *srcv;
    unsigned srcl, xmask, ymask, x, y;
    uint8_t y0 = 0, y1 = 0, u = 0x80, v = 0x80;

    uv_roundup(dst, dstfmt);
    dst->datalen = dst->width * dst->height + uvp_size(dst, dstfmt) * 2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    dstp = (void*)dst->data;

    srcm = uvp_size(src, srcfmt);
    srcn = src->width * src->height;
    assert(src->datalen >= srcn + 2 * srcn);
    flags = dstfmt->p.yuv.packorder ^ srcfmt->p.yuv.packorder;
    srcy = (void*)src->data;
    if(flags & 1) {
        srcv = (uint8_t*)src->data + srcn;
        srcu = srcv + srcm;
    } else {
        srcu = (uint8_t*)src->data + srcn;
        srcv = srcu + srcm;
    }
    flags = dstfmt->p.yuv.packorder & 2;

    srcl = src->width >> srcfmt->p.yuv.xsub2;
    xmask = (1 << srcfmt->p.yuv.xsub2) - 1;
    ymask = (1 << srcfmt->p.yuv.ysub2) - 1;
    for(y = 0; y < dst->height; y++) {
        if(y >= src->height) {
            srcy -= src->width;
            srcu -= srcl;  srcv -= srcl;
        }
        else if(y & ymask) {
            srcu -= srcl;  srcv -= srcl;
        }
        for(x = 0; x < dst->width; x += 2) {
            if(x < src->width) {
                y0 = *(srcy++);  y1 = *(srcy++);
                if(!(x & xmask)) {
                    u = *(srcu++);  v = *(srcv++);
                }
            }
            if(flags) {
                *(dstp++) = u;  *(dstp++) = y0;
                *(dstp++) = v;  *(dstp++) = y1;
            } else {
                *(dstp++) = y0;  *(dstp++) = u;
                *(dstp++) = y1;  *(dstp++) = v;
            }
        }
        for(; x < src->width; x += 2) {
            srcy += 2;
            if(!(x & xmask)) {
                srcu++;  srcv++;
            }
        }
    }
}

static void convert_yuv_unpack (zbar_image_t *dst,
                                const zbar_format_def_t *dstfmt,
                                const zbar_image_t *src,
                                const zbar_format_def_t *srcfmt)
{
    unsigned long dstn, dstm2;
    uint8_t *dsty, flags;
    const uint8_t *srcp;
    unsigned srcl, x, y;
    uint8_t y0 = 0, y1 = 0;

    uv_roundup(dst, dstfmt);
    dstn = dst->width * dst->height;
    dstm2 = uvp_size(dst, dstfmt) * 2;
    dst->datalen = dstn + dstm2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    if(dstm2)
        memset((uint8_t*)dst->data + dstn, 0x80, dstm2);
    dsty = (uint8_t*)dst->data;

    flags = srcfmt->p.yuv.packorder ^ dstfmt->p.yuv.packorder;
    flags &= 2;
    srcp = src->data;
    if(flags)
        srcp++;

    srcl = src->width + (src->width >> srcfmt->p.yuv.xsub2);
    for(y = 0; y < dst->height; y++) {
        if(y >= src->height)
            srcp -= srcl;
        for(x = 0; x < dst->width; x += 2) {
            if(x < src->width) {
                y0 = *(srcp++);  srcp++;
                y1 = *(srcp++);  srcp++;
            }
            *(dsty++) = y0;
            *(dsty++) = y1;
        }
        if(x < src->width)
            srcp += (src->width - x) * 2;
    }
}

static void convert_uvp_resample (zbar_image_t *dst,
                                  const zbar_format_def_t *dstfmt,
                                  const zbar_image_t *src,
                                  const zbar_format_def_t *srcfmt)
{
    unsigned long dstn, dstm2;
    uv_roundup(dst, dstfmt);
    dstn = dst->width * dst->height;
    dstm2 = uvp_size(dst, dstfmt) * 2;
    dst->datalen = dstn + dstm2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    convert_y_resize(dst, dstfmt, src, srcfmt, dstn);
    if(dstm2)
        memset((uint8_t*)dst->data + dstn, 0x80, dstm2);
}

static void convert_uv_resample (zbar_image_t *dst,
                                 const zbar_format_def_t *dstfmt,
                                 const zbar_image_t *src,
                                 const zbar_format_def_t *srcfmt)
{
    unsigned long dstn;
    uint8_t *dstp, flags;
    const uint8_t *srcp;
    unsigned srcl, x, y;
    uint8_t y0 = 0, y1 = 0, u = 0x80, v = 0x80;

    uv_roundup(dst, dstfmt);
    dstn = dst->width * dst->height;
    dst->datalen = dstn + uvp_size(dst, dstfmt) * 2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    dstp = (void*)dst->data;

    flags = (srcfmt->p.yuv.packorder ^ dstfmt->p.yuv.packorder) & 1;
    srcp = src->data;

    srcl = src->width + (src->width >> srcfmt->p.yuv.xsub2);
    for(y = 0; y < dst->height; y++) {
        if(y >= src->height)
            srcp -= srcl;
        for(x = 0; x < dst->width; x += 2) {
            if(x < src->width) {
                if(!(srcfmt->p.yuv.packorder & 2)) {
                    y0 = *(srcp++);  u = *(srcp++);
                    y1 = *(srcp++);  v = *(srcp++);
                }
                else {
                    u = *(srcp++);  y0 = *(srcp++);
                    v = *(srcp++);  y1 = *(srcp++);
                }
                if(flags) {
                    uint8_t tmp = u;  u = v;  v = tmp;
                }
            }
            if(!(dstfmt->p.yuv.packorder & 2)) {
                *(dstp++) = y0;  *(dstp++) = u;
                *(dstp++) = y1;  *(dstp++) = v;
            }
            else {
                *(dstp++) = u;  *(dstp++) = y0;
                *(dstp++) = v;  *(dstp++) = y1;
            }
        }
        if(x < src->width)
            srcp += (src->width - x) * 2;
    }
}

static void convert_yuvp_to_rgb (zbar_image_t *dst,
                                 const zbar_format_def_t *dstfmt,
                                 const zbar_image_t *src,
                                 const zbar_format_def_t *srcfmt)
{
    uint8_t *dstp, *srcy;
    int drbits, drbit0, dgbits, dgbit0, dbbits, dbbit0;
    unsigned long srcm, srcn;
    unsigned x, y;
    uint32_t p = 0;

    dst->datalen = dst->width * dst->height * dstfmt->p.rgb.bpp;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    dstp = (void*)dst->data;

    drbits = RGB_SIZE(dstfmt->p.rgb.red);
    drbit0 = RGB_OFFSET(dstfmt->p.rgb.red);
    dgbits = RGB_SIZE(dstfmt->p.rgb.green);
    dgbit0 = RGB_OFFSET(dstfmt->p.rgb.green);
    dbbits = RGB_SIZE(dstfmt->p.rgb.blue);
    dbbit0 = RGB_OFFSET(dstfmt->p.rgb.blue);

    srcm = uvp_size(src, srcfmt);
    srcn = src->width * src->height;
    assert(src->datalen >= srcn + 2 * srcm);
    srcy = (void*)src->data;

    for(y = 0; y < dst->height; y++) {
        if(y >= src->height)
            srcy -= src->width;
        for(x = 0; x < dst->width; x++) {
            if(x < src->width) {

                unsigned y0 = *(srcy++);
                p = (((y0 >> drbits) << drbit0) |
                     ((y0 >> dgbits) << dgbit0) |
                     ((y0 >> dbbits) << dbbit0));
            }
            convert_write_rgb(dstp, p, dstfmt->p.rgb.bpp);
            dstp += dstfmt->p.rgb.bpp;
        }
        if(x < src->width)
            srcy += (src->width - x);
    }
}

static void convert_rgb_to_yuvp (zbar_image_t *dst,
                                 const zbar_format_def_t *dstfmt,
                                 const zbar_image_t *src,
                                 const zbar_format_def_t *srcfmt)
{
    unsigned long dstn, dstm2;
    uint8_t *dsty;
    const uint8_t *srcp;
    int rbits, rbit0, gbits, gbit0, bbits, bbit0;
    unsigned srcl, x, y;
    uint16_t y0 = 0;

    uv_roundup(dst, dstfmt);
    dstn = dst->width * dst->height;
    dstm2 = uvp_size(dst, dstfmt) * 2;
    dst->datalen = dstn + dstm2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    if(dstm2)
        memset((uint8_t*)dst->data + dstn, 0x80, dstm2);
    dsty = (void*)dst->data;

    assert(src->datalen >= (src->width * src->height * srcfmt->p.rgb.bpp));
    srcp = src->data;

    rbits = RGB_SIZE(srcfmt->p.rgb.red);
    rbit0 = RGB_OFFSET(srcfmt->p.rgb.red);
    gbits = RGB_SIZE(srcfmt->p.rgb.green);
    gbit0 = RGB_OFFSET(srcfmt->p.rgb.green);
    bbits = RGB_SIZE(srcfmt->p.rgb.blue);
    bbit0 = RGB_OFFSET(srcfmt->p.rgb.blue);

    srcl = src->width * srcfmt->p.rgb.bpp;
    for(y = 0; y < dst->height; y++) {
        if(y >= src->height)
            srcp -= srcl;
        for(x = 0; x < dst->width; x++) {
            if(x < src->width) {
                uint8_t r, g, b;
                uint32_t p = convert_read_rgb(srcp, srcfmt->p.rgb.bpp);
                srcp += srcfmt->p.rgb.bpp;

                r = ((p >> rbit0) << rbits) & 0xff;
                g = ((p >> gbit0) << gbits) & 0xff;
                b = ((p >> bbit0) << bbits) & 0xff;

                y0 = ((77 * r + 150 * g + 29 * b) + 0x80) >> 8;
            }
            *(dsty++) = y0;
        }
        if(x < src->width)
            srcp += (src->width - x) * srcfmt->p.rgb.bpp;
    }
}

static void convert_yuv_to_rgb (zbar_image_t *dst,
                                const zbar_format_def_t *dstfmt,
                                const zbar_image_t *src,
                                const zbar_format_def_t *srcfmt)
{
    uint8_t *dstp;
    unsigned long dstn = dst->width * dst->height;
    int drbits, drbit0, dgbits, dgbit0, dbbits, dbbit0;
    const uint8_t *srcp;
    unsigned srcl, x, y;
    uint32_t p = 0;

    dst->datalen = dstn * dstfmt->p.rgb.bpp;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    dstp = (void*)dst->data;

    drbits = RGB_SIZE(dstfmt->p.rgb.red);
    drbit0 = RGB_OFFSET(dstfmt->p.rgb.red);
    dgbits = RGB_SIZE(dstfmt->p.rgb.green);
    dgbit0 = RGB_OFFSET(dstfmt->p.rgb.green);
    dbbits = RGB_SIZE(dstfmt->p.rgb.blue);
    dbbit0 = RGB_OFFSET(dstfmt->p.rgb.blue);

    assert(src->datalen >= (src->width * src->height +
                            uvp_size(src, srcfmt) * 2));
    srcp = src->data;
    if(srcfmt->p.yuv.packorder & 2)
        srcp++;

    assert(srcfmt->p.yuv.xsub2 == 1);
    srcl = src->width + (src->width >> 1);
    for(y = 0; y < dst->height; y++) {
        if(y >= src->height)
            srcp -= srcl;
        for(x = 0; x < dst->width; x++) {
            if(x < src->width) {
                uint8_t y0 = *(srcp++);
                srcp++;

                if(y0 <= 16)
                    y0 = 0;
                else if(y0 >= 235)
                    y0 = 255;
                else
                    y0 = (uint16_t)(y0 - 16) * 255 / 219;

                p = (((y0 >> drbits) << drbit0) |
                     ((y0 >> dgbits) << dgbit0) |
                     ((y0 >> dbbits) << dbbit0));
            }
            convert_write_rgb(dstp, p, dstfmt->p.rgb.bpp);
            dstp += dstfmt->p.rgb.bpp;
        }
        if(x < src->width)
            srcp += (src->width - x) * 2;
    }
}

static void convert_rgb_to_yuv (zbar_image_t *dst,
                                const zbar_format_def_t *dstfmt,
                                const zbar_image_t *src,
                                const zbar_format_def_t *srcfmt)
{
    uint8_t *dstp, flags;
    const uint8_t *srcp;
    int rbits, rbit0, gbits, gbit0, bbits, bbit0;
    unsigned srcl, x, y;
    uint16_t y0 = 0;

    uv_roundup(dst, dstfmt);
    dst->datalen = dst->width * dst->height + uvp_size(dst, dstfmt) * 2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    dstp = (void*)dst->data;
    flags = dstfmt->p.yuv.packorder & 2;

    assert(src->datalen >= (src->width * src->height * srcfmt->p.rgb.bpp));
    srcp = src->data;

    rbits = RGB_SIZE(srcfmt->p.rgb.red);
    rbit0 = RGB_OFFSET(srcfmt->p.rgb.red);
    gbits = RGB_SIZE(srcfmt->p.rgb.green);
    gbit0 = RGB_OFFSET(srcfmt->p.rgb.green);
    bbits = RGB_SIZE(srcfmt->p.rgb.blue);
    bbit0 = RGB_OFFSET(srcfmt->p.rgb.blue);

    srcl = src->width * srcfmt->p.rgb.bpp;
    for(y = 0; y < dst->height; y++) {
        if(y >= src->height)
            srcp -= srcl;
        for(x = 0; x < dst->width; x++) {
            if(x < src->width) {
                uint8_t r, g, b;
                uint32_t p = convert_read_rgb(srcp, srcfmt->p.rgb.bpp);
                srcp += srcfmt->p.rgb.bpp;

                r = ((p >> rbit0) << rbits) & 0xff;
                g = ((p >> gbit0) << gbits) & 0xff;
                b = ((p >> bbit0) << bbits) & 0xff;

                y0 = ((77 * r + 150 * g + 29 * b) + 0x80) >> 8;
            }
            if(flags) {
                *(dstp++) = 0x80;  *(dstp++) = y0;
            }
            else {
                *(dstp++) = y0;  *(dstp++) = 0x80;
            }
        }
        if(x < src->width)
            srcp += (src->width - x) * srcfmt->p.rgb.bpp;
    }
}

static void convert_rgb_resample (zbar_image_t *dst,
                                  const zbar_format_def_t *dstfmt,
                                  const zbar_image_t *src,
                                  const zbar_format_def_t *srcfmt)
{
    unsigned long dstn = dst->width * dst->height;
    uint8_t *dstp;
    int drbits, drbit0, dgbits, dgbit0, dbbits, dbbit0;
    int srbits, srbit0, sgbits, sgbit0, sbbits, sbbit0;
    const uint8_t *srcp;
    unsigned srcl, x, y;
    uint32_t p = 0;

    dst->datalen = dstn * dstfmt->p.rgb.bpp;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    dstp = (void*)dst->data;

    drbits = RGB_SIZE(dstfmt->p.rgb.red);
    drbit0 = RGB_OFFSET(dstfmt->p.rgb.red);
    dgbits = RGB_SIZE(dstfmt->p.rgb.green);
    dgbit0 = RGB_OFFSET(dstfmt->p.rgb.green);
    dbbits = RGB_SIZE(dstfmt->p.rgb.blue);
    dbbit0 = RGB_OFFSET(dstfmt->p.rgb.blue);

    assert(src->datalen >= (src->width * src->height * srcfmt->p.rgb.bpp));
    srcp = src->data;

    srbits = RGB_SIZE(srcfmt->p.rgb.red);
    srbit0 = RGB_OFFSET(srcfmt->p.rgb.red);
    sgbits = RGB_SIZE(srcfmt->p.rgb.green);
    sgbit0 = RGB_OFFSET(srcfmt->p.rgb.green);
    sbbits = RGB_SIZE(srcfmt->p.rgb.blue);
    sbbit0 = RGB_OFFSET(srcfmt->p.rgb.blue);

    srcl = src->width * srcfmt->p.rgb.bpp;
    for(y = 0; y < dst->height; y++) {
        if(y >= src->height)
            y -= srcl;
        for(x = 0; x < dst->width; x++) {
            if(x < src->width) {
                uint8_t r, g, b;
                p = convert_read_rgb(srcp, srcfmt->p.rgb.bpp);
                srcp += srcfmt->p.rgb.bpp;

                r = (p >> srbit0) << srbits;
                g = (p >> sgbit0) << sgbits;
                b = (p >> sbbit0) << sbbits;

                p = (((r >> drbits) << drbit0) |
                     ((g >> dgbits) << dgbit0) |
                     ((b >> dbbits) << dbbit0));
            }
            convert_write_rgb(dstp, p, dstfmt->p.rgb.bpp);
            dstp += dstfmt->p.rgb.bpp;
        }
        if(x < src->width)
            srcp += (src->width - x) * srcfmt->p.rgb.bpp;
    }
}

#ifdef HAVE_LIBJPEG
void _zbar_convert_jpeg_to_y(zbar_image_t *dst,
                              const zbar_format_def_t *dstfmt,
                              const zbar_image_t *src,
                              const zbar_format_def_t *srcfmt);

static void convert_jpeg(zbar_image_t *dst,
                         const zbar_format_def_t *dstfmt,
                         const zbar_image_t *src,
                         const zbar_format_def_t *srcfmt);
#endif

static conversion_def_t conversions[][ZBAR_FMT_NUM] = {
    {
        {   0, convert_copy },
        {   8, convert_uvp_append },
        {  24, convert_yuv_pack },
        {  32, convert_yuvp_to_rgb },
        {   8, convert_uvp_append },
        {  -1, NULL },
    },
    {
        {   1, convert_copy },
        {  48, convert_uvp_resample },
        {  64, convert_yuv_pack },
        { 128, convert_yuvp_to_rgb },
        {  40, convert_uvp_append },
        {  -1, NULL },
    },
    {
        {  24, convert_yuv_unpack },
        {  52, convert_yuv_unpack },
        {  20, convert_uv_resample },
        { 144, convert_yuv_to_rgb },
        {  18, convert_yuv_unpack },
        {  -1, NULL },
    },
    {
        { 112, convert_rgb_to_yuvp },
        { 160, convert_rgb_to_yuvp },
        { 144, convert_rgb_to_yuv },
        { 120, convert_rgb_resample },
        { 152, convert_rgb_to_yuvp },
        {  -1, NULL },
    },
    {
        {   1, convert_copy },
        {   8, convert_uvp_append },
        {  24, convert_yuv_pack },
        {  32, convert_yuvp_to_rgb },
        {   8, convert_uvp_append },
        {  -1, NULL },
    },
#ifdef HAVE_LIBJPEG
    {
        {  96, _zbar_convert_jpeg_to_y },
        { 104, convert_jpeg },
        { 116, convert_jpeg },
        { 256, convert_jpeg },
        { 104, convert_jpeg },
        {  -1, NULL },
    },
#else
    {
        {  -1, NULL },
        {  -1, NULL },
        {  -1, NULL },
        {  -1, NULL },
        {  -1, NULL },
        {  -1, NULL },
    },
#endif
};

const zbar_format_def_t *_zbar_format_lookup (uint32_t fmt)
{
    const zbar_format_def_t *def = NULL;
    int i = 0;
    while(i < num_format_defs) {
        def = &format_defs[i];
        if(fmt == def->format)
            return(def);
        i = i * 2 + 1;
        if(fmt > def->format)
            i++;
    }
    return(NULL);
}

#ifdef HAVE_LIBJPEG

static void convert_jpeg (zbar_image_t *dst,
                          const zbar_format_def_t *dstfmt,
                          const zbar_image_t *src,
                          const zbar_format_def_t *srcfmt)
{

    zbar_image_t *tmp;
    if(!src->src) {
        tmp = zbar_image_create();
        tmp->format = fourcc('Y','8','0','0');
        _zbar_image_copy_size(tmp, dst);
    }
    else {
        tmp = src->src->jpeg_img;
        assert(tmp);
        _zbar_image_copy_size(dst, tmp);
    }

    const zbar_format_def_t *tmpfmt = _zbar_format_lookup(tmp->format);
    assert(tmpfmt);

    _zbar_convert_jpeg_to_y(tmp, tmpfmt, src, srcfmt);

    _zbar_image_copy_size(dst, tmp);

    conversion_handler_t *func =
        conversions[tmpfmt->group][dstfmt->group].func;

    func(dst, dstfmt, tmp, tmpfmt);

    if(!src->src)
        zbar_image_destroy(tmp);
}
#endif

zbar_image_t *zbar_image_convert_resize (const zbar_image_t *src,
                                         unsigned long fmt,
                                         unsigned width,
                                         unsigned height)
{
    const zbar_format_def_t *srcfmt, *dstfmt;
    conversion_handler_t *func;
    zbar_image_t *dst = zbar_image_create();
    dst->format = fmt;
    dst->width = width;
    dst->height = height;
    zbar_image_set_crop(dst, src->crop_x, src->crop_y,
                        src->crop_w, src->crop_h);
    if(src->format == fmt &&
       src->width == width &&
       src->height == height) {
        convert_copy(dst, NULL, src, NULL);
        return(dst);
    }

    srcfmt = _zbar_format_lookup(src->format);
    dstfmt = _zbar_format_lookup(dst->format);
    if(!srcfmt || !dstfmt)

        return(NULL);

    if(srcfmt->group == dstfmt->group &&
       srcfmt->p.cmp == dstfmt->p.cmp &&
       src->width == width &&
       src->height == height) {
        convert_copy(dst, NULL, src, NULL);
        return(dst);
    }

    func = conversions[srcfmt->group][dstfmt->group].func;

    dst->cleanup = zbar_image_free_data;
    func(dst, dstfmt, src, srcfmt);
    if(!dst->data) {

        zbar_image_destroy(dst);
        return(NULL);
    }
    return(dst);
}

zbar_image_t *zbar_image_convert (const zbar_image_t *src,
                                  unsigned long fmt)
{
    return(zbar_image_convert_resize(src, fmt, src->width, src->height));
}

static inline int has_format (uint32_t fmt,
                              const uint32_t *fmts)
{
    for(; *fmts; fmts++)
        if(*fmts == fmt)
            return(1);
    return(0);
}

int _zbar_best_format (uint32_t src,
                       uint32_t *dst,
                       const uint32_t *dsts)
{
    const zbar_format_def_t *srcfmt;
    unsigned min_cost = -1;

    if(dst)
        *dst = 0;
    if(!dsts)
        return(-1);
    if(has_format(src, dsts)) {
        zprintf(8, "shared format: %4.4s\n", (char*)&src);
        if(dst)
            *dst = src;
        return(0);
    }
    srcfmt = _zbar_format_lookup(src);
    if(!srcfmt)
        return(-1);

    zprintf(8, "from %.4s(%08" PRIx32 ") to", (char*)&src, src);
    for(; *dsts; dsts++) {
        const zbar_format_def_t *dstfmt = _zbar_format_lookup(*dsts);
        int cost;
        if(!dstfmt)
            continue;
        if(srcfmt->group == dstfmt->group &&
           srcfmt->p.cmp == dstfmt->p.cmp)
            cost = 0;
        else
            cost = conversions[srcfmt->group][dstfmt->group].cost;

        if(_zbar_verbosity >= 8)
            fprintf(stderr, " %.4s(%08" PRIx32 ")=%d",
                    (char*)dsts, *dsts, cost);
        if(cost >= 0 && min_cost > cost) {
            min_cost = cost;
            if(dst)
                *dst = *dsts;
        }
    }
    if(_zbar_verbosity >= 8)
        fprintf(stderr, "\n");
    return(min_cost);
}

int zbar_negotiate_format (zbar_video_t *vdo,
                           zbar_window_t *win)
{
    static const uint32_t y800[2] = { fourcc('Y','8','0','0'), 0 };
    errinfo_t *errdst;
    const uint32_t *srcs, *dsts;
    unsigned min_cost = -1;
    uint32_t min_fmt = 0;
    const uint32_t *fmt;

    if(!vdo && !win)
        return(0);

    if(win)
        (void)window_lock(win);

    errdst = (vdo) ? &vdo->err : &win->err;
    if(verify_format_sort()) {
        if(win)
            (void)window_unlock(win);
        return(err_capture(errdst, SEV_FATAL, ZBAR_ERR_INTERNAL, __func__,
                           "image format list is not sorted!?"));
    }

    if((vdo && !vdo->formats) || (win && !win->formats)) {
        if(win)
            (void)window_unlock(win);
        return(err_capture(errdst, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, __func__,
                           "no input or output formats available"));
    }

    srcs = (vdo) ? vdo->formats : y800;
    dsts = (win) ? win->formats : y800;

    for(fmt = _zbar_formats; *fmt; fmt++) {

        uint32_t win_fmt = 0;
        int cost;
        if(!has_format(*fmt, srcs))
            continue;
        cost = _zbar_best_format(*fmt, &win_fmt, dsts);
        if(cost < 0) {
            zprintf(4, "%.4s(%08" PRIx32 ") -> ? (unsupported)\n",
                    (char*)fmt, *fmt);
            continue;
        }
        zprintf(4, "%.4s(%08" PRIx32 ") -> %.4s(%08" PRIx32 ") (%d)\n",
                (char*)fmt, *fmt, (char*)&win_fmt, win_fmt, cost);
        if(min_cost > cost) {
            min_cost = cost;
            min_fmt = *fmt;
            if(!cost)
                break;
        }
    }
    if(win)
        (void)window_unlock(win);

    if(!min_fmt)
        return(err_capture(errdst, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, __func__,
                           "no supported image formats available"));
    if(!vdo)
        return(0);

    zprintf(2, "setting best format %.4s(%08" PRIx32 ") (%d)\n",
            (char*)&min_fmt, min_fmt, min_cost);
    return(zbar_video_init(vdo, min_fmt));
}
