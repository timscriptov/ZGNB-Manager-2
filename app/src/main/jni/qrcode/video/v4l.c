

#include <config.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "video.h"

extern int _zbar_v4l1_probe(zbar_video_t*);
extern int _zbar_v4l2_probe(zbar_video_t*);

int _zbar_video_open (zbar_video_t *vdo,
                      const char *dev)
{
    vdo->fd = open(dev, O_RDWR);
    if(vdo->fd < 0)
        return(err_capture_str(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                               "opening video device '%s'", dev));
    zprintf(1, "opened camera device %s (fd=%d)\n", dev, vdo->fd);

    int rc = -1;
#ifdef HAVE_LINUX_VIDEODEV2_H
    if(vdo->intf != VIDEO_V4L1)
        rc = _zbar_v4l2_probe(vdo);
#endif
#ifdef HAVE_LINUX_VIDEODEV_H
    if(rc && vdo->intf != VIDEO_V4L2)
        rc = _zbar_v4l1_probe(vdo);
#endif

    if(rc && vdo->fd >= 0) {
        close(vdo->fd);
        vdo->fd = -1;
    }
    return(rc);
}
