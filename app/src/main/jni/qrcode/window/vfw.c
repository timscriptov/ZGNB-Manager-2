

#include "window.h"
#include <vfw.h>

extern int _zbar_window_bih_init(zbar_window_t *w,
                                 zbar_image_t *img);

static int vfw_cleanup (zbar_window_t *w)
{
    if(w->hdd) {
        DrawDibClose(w->hdd);
        w->hdd = NULL;
    }
    return(0);
}

static int vfw_init (zbar_window_t *w,
                     zbar_image_t *img,
                     int new_format)
{
    if(new_format)
        _zbar_window_bih_init(w, img);

    w->dst_width = w->bih.biWidth = (img->width + 3) & ~3;
    w->dst_height = w->bih.biHeight = img->height;

    HDC hdc = GetDC(w->hwnd);
    if(!hdc)
        return(-1);

    if(!DrawDibBegin(w->hdd, hdc, w->width, w->height,
                     &w->bih, img->width, img->height, 0))
        return(-1);

    ReleaseDC(w->hwnd, hdc);
    return(0);
}

static int vfw_draw (zbar_window_t *w,
                     zbar_image_t *img)
{
    HDC hdc = GetDC(w->hwnd);
    if(!hdc)
        return(-1);

    zprintf(24, "DrawDibDraw(%dx%d -> %dx%d)\n",
            img->width, img->height, w->width, w->height);

    DrawDibDraw(w->hdd, hdc,
                0, 0, w->width, w->height,
                &w->bih, (void*)img->data,
                0, 0, w->src_width, w->src_height,
                DDF_SAME_DRAW);

    ValidateRect(w->hwnd, NULL);
    ReleaseDC(w->hwnd, hdc);
    return(0);
}

static uint32_t vfw_formats[] = {
    fourcc('B','G','R','3'),
    fourcc('B','G','R','4'),
    fourcc('J','P','E','G'),
    0
};

int _zbar_window_vfw_init (zbar_window_t *w)
{
    w->hdd = DrawDibOpen();
    if(!w->hdd)
        return(err_capture(w, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, __func__,
                           "unable to initialize DrawDib"));

    uint32_t *fmt;
    for(fmt = vfw_formats; *fmt; fmt++)
        _zbar_window_add_format(w, *fmt);

    w->init = vfw_init;
    w->draw_image = vfw_draw;
    w->cleanup = vfw_cleanup;
    return(0);
}
