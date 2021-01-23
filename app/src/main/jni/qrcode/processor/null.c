

#include "processor.h"

static inline int null_error (void *m,
                              const char *func)
{
    return(err_capture(m, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, func,
                       "not compiled with output window support"));
}

int _zbar_processor_open (zbar_processor_t *proc,
                          char *name,
                          unsigned w,
                          unsigned h)
{
    return(null_error(proc, __func__));
}

int _zbar_processor_close (zbar_processor_t *proc)
{
    return(null_error(proc, __func__));
}

int _zbar_processor_set_visible (zbar_processor_t *proc,
                                 int vis)
{
    return(null_error(proc, __func__));
}

int _zbar_processor_set_size (zbar_processor_t *proc,
                              unsigned width,
                              unsigned height)
{
    return(null_error(proc, __func__));
}

int _zbar_processor_invalidate (zbar_processor_t *proc)
{
    return(null_error(proc, __func__));
}
