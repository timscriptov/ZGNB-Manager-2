

#include "window.h"
#include "image.h"
#include "win.h"

static int dib_cleanup (zbar_window_t *w)
{
    return(0);
}

static int dib_init (zbar_window_t *w,
                     zbar_image_t *img,
                     int new_format)
{
    if(new_format)
        _zbar_window_bih_init(w, img);

    window_state_t *win = w->state;
    w->dst_width = win->bih.biWidth = (img->width + 3) & ~3;
    w->dst_height = win->bih.biHeight = img->height;
    return(0);
}

static int dib_draw (zbar_window_t *w,
                     zbar_image_t *img)
{
    StretchDIBits(w->state->hdc,
                  w->scaled_offset.x, w->scaled_offset.y + w->scaled_size.y - 1,
                  w->scaled_size.x, -w->scaled_size.y,
                  0, 0, w->src_width, w->src_height,
                  (void*)img->data, (BITMAPINFO*)&w->state->bih,
                  DIB_RGB_COLORS, SRCCOPY);
    return(0);
}

static uint32_t dib_formats[] = {
    fourcc('B','G','R','3'),
    fourcc('B','G','R','4'),
    fourcc('J','P','E','G'),
    0
};

int _zbar_window_dib_init (zbar_window_t *w)
{
    uint32_t *fmt;
    for(fmt = dib_formats; *fmt; fmt++)
        _zbar_window_add_format(w, *fmt);

    w->init = dib_init;
    w->draw_image = dib_draw;
    w->cleanup = dib_cleanup;
    return(0);
}
