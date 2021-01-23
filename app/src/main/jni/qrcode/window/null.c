

#include "window.h"

static inline int null_error (void *m,
                              const char *func)
{
    return(err_capture(m, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, func,
                       "not compiled with output window support"));
}

int _zbar_window_attach (zbar_window_t *w,
                         void *display,
                         unsigned long win)
{
    return(null_error(w, __func__));
}

int _zbar_window_expose (zbar_window_t *w,
                         int x,
                         int y,
                         int width,
                         int height)
{
    return(null_error(w, __func__));
}

int _zbar_window_resize (zbar_window_t *w)
{
    return(0);
}

int _zbar_window_clear (zbar_window_t *w)
{
    return(null_error(w, __func__));
}

int _zbar_window_begin (zbar_window_t *w)
{
    return(null_error(w, __func__));
}

int _zbar_window_end (zbar_window_t *w)
{
    return(null_error(w, __func__));
}

int _zbar_window_draw_marker (zbar_window_t *w,
                              uint32_t rgb,
                              point_t p)
{
    return(null_error(w, __func__));
}

int _zbar_window_draw_polygon (zbar_window_t *w,
                               uint32_t rgb,
                               const point_t *pts,
                               int npts)
{
    return(null_error(w, __func__));
}

int _zbar_window_draw_text (zbar_window_t *w,
                            uint32_t rgb,
                            point_t p,
                            const char *text)
{
    return(null_error(w, __func__));
}

int _zbar_window_fill_rect (zbar_window_t *w,
                            uint32_t rgb,
                            point_t org,
                            point_t size)
{
    return(null_error(w, __func__));
}

int _zbar_window_draw_logo (zbar_window_t *w)
{
    return(null_error(w, __func__));
}
