

#include "video.h"

static inline int null_error (void *m,
                              const char *func)
{
    return(err_capture(m, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, func,
                       "not compiled with video input support"));
}

int _zbar_video_open (zbar_video_t *vdo,
                      const char *device)
{
    return(null_error(vdo, __func__));
}
