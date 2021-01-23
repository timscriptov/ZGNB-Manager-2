

#include "video.h"
#include "thread.h"
#include <vfw.h>

#include <assert.h>

#define MAX_DRIVERS 10
#define MAX_NAME 128

#define BIH_FMT "%ldx%ld @%dbpp (%lx) cmp=%.4s(%08lx) res=%ldx%ld clr=%ld/%ld (%lx)"
#define BIH_FIELDS(bih)                                                 \
    (bih)->biWidth, (bih)->biHeight, (bih)->biBitCount, (bih)->biSizeImage, \
        (char*)&(bih)->biCompression, (bih)->biCompression,             \
        (bih)->biXPelsPerMeter, (bih)->biYPelsPerMeter,                 \
        (bih)->biClrImportant, (bih)->biClrUsed, (bih)->biSize

struct video_state_s {
    zbar_thread_t thread;       
    HANDLE captured;
    HWND hwnd;                  
    HANDLE notify;              
    int bi_size;                
    BITMAPINFOHEADER *bih;      
    zbar_image_t *image;        
};

static const uint32_t vfw_formats[] = {
    
    fourcc('I','4','2','0'),
    
    fourcc('Y','V','1','2'),
    

    
    fourcc('N','V','1','2'),

    
    fourcc('U','Y','V','Y'),
    fourcc('Y','U','Y','2'), 
    

    
    fourcc('B','G','R','3'),
    fourcc('B','G','R','4'),

    fourcc('Y','V','U','9'),

    
    fourcc('G','R','E','Y'),
    fourcc('Y','8','0','0'),

    
    fourcc('J','P','E','G'),

    
    0
};

#define VFW_NUM_FORMATS (sizeof(vfw_formats) / sizeof(uint32_t))

static ZTHREAD vfw_capture_thread (void *arg)
{
    zbar_video_t *vdo = arg;
    video_state_t *state = vdo->state;
    zbar_thread_t *thr = &state->thread;

    state->hwnd = capCreateCaptureWindow(NULL, WS_POPUP, 0, 0, 1, 1, NULL, 0);
    if(!state->hwnd)
        goto done;

    _zbar_mutex_lock(&vdo->qlock);
    _zbar_thread_init(thr);
    zprintf(4, "spawned vfw capture thread (thr=%04lx)\n",
            _zbar_thread_self());

    MSG msg;
    int rc = 0;
    while(thr->started && rc >= 0 && rc <= 1) {
        _zbar_mutex_unlock(&vdo->qlock);

        rc = MsgWaitForMultipleObjects(1, &thr->notify, 0,
                                       INFINITE, QS_ALLINPUT);
        if(rc == 1)
            while(PeekMessage(&msg, NULL, 0, 0, PM_NOYIELD | PM_REMOVE))
                if(rc > 0) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

        _zbar_mutex_lock(&vdo->qlock);
    }

 done:
    thr->running = 0;
    _zbar_event_trigger(&thr->activity);
    _zbar_mutex_unlock(&vdo->qlock);
    return(0);
}

static LRESULT CALLBACK vfw_stream_cb (HWND hwnd,
                                       VIDEOHDR *hdr)
{
    if(!hwnd || !hdr)
        return(0);
    zbar_video_t *vdo = (void*)capGetUserData(hwnd);

    _zbar_mutex_lock(&vdo->qlock);
    zbar_image_t *img = vdo->state->image;
    if(!img) {
        _zbar_mutex_lock(&vdo->qlock);
        img = video_dq_image(vdo);
    }
    if(img) {
        img->data = hdr->lpData;
        img->datalen = hdr->dwBufferLength;
        vdo->state->image = img;
        SetEvent(vdo->state->captured);
    }
    _zbar_mutex_unlock(&vdo->qlock);

    return(1);
}

static LRESULT CALLBACK vfw_error_cb (HWND hwnd,
                                      int errid,
                                      const char *errmsg)
{
    if(!hwnd)
        return(0);
    zbar_video_t *vdo = (void*)capGetUserData(hwnd);
    zprintf(2, "id=%d msg=%s\n", errid, errmsg);
    _zbar_mutex_lock(&vdo->qlock);
    vdo->state->image = NULL;
    SetEvent(vdo->state->captured);
    _zbar_mutex_unlock(&vdo->qlock);
    return(1);
}

static int vfw_nq (zbar_video_t *vdo,
                   zbar_image_t *img)
{
    img->data = NULL;
    img->datalen = 0;
    return(video_nq_image(vdo, img));
}

static zbar_image_t *vfw_dq (zbar_video_t *vdo)
{
    zbar_image_t *img = vdo->state->image;
    if(!img) {
        _zbar_mutex_unlock(&vdo->qlock);
        int rc = WaitForSingleObject(vdo->state->captured, INFINITE);
        _zbar_mutex_lock(&vdo->qlock);
        if(!rc)
            img = vdo->state->image;
        else
            img = NULL;
        
    }
    else
        ResetEvent(vdo->state->captured);
    if(img)
        vdo->state->image = NULL;

    video_unlock(vdo);
    return(img);
}

static int vfw_start (zbar_video_t *vdo)
{
    ResetEvent(vdo->state->captured);

    if(!capCaptureSequenceNoFile(vdo->state->hwnd))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "starting video stream"));
    return(0);
}

static int vfw_stop (zbar_video_t *vdo)
{
    video_state_t *state = vdo->state;
    if(!capCaptureAbort(state->hwnd))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "stopping video stream"));

    _zbar_mutex_lock(&vdo->qlock);
    if(state->image)
        state->image = NULL;
    SetEvent(state->captured);
    _zbar_mutex_unlock(&vdo->qlock);
    return(0);
}

static int vfw_set_format (zbar_video_t *vdo,
                           uint32_t fmt)
{
    const zbar_format_def_t *fmtdef = _zbar_format_lookup(fmt);
    if(!fmtdef->format)
        return(err_capture_int(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                               "unsupported vfw format: %x", fmt));

    BITMAPINFOHEADER *bih = vdo->state->bih;
    assert(bih);
    bih->biWidth = vdo->width;
    bih->biHeight = vdo->height;
    switch(fmtdef->group) {
    case ZBAR_FMT_GRAY:
        bih->biBitCount = 8;
        break;
    case ZBAR_FMT_YUV_PLANAR:
    case ZBAR_FMT_YUV_PACKED:
    case ZBAR_FMT_YUV_NV:
        bih->biBitCount = 8 + (16 >> (fmtdef->p.yuv.xsub2 + fmtdef->p.yuv.ysub2));
        break;
    case ZBAR_FMT_RGB_PACKED:
        bih->biBitCount = fmtdef->p.rgb.bpp * 8;
        break;
    default:
        bih->biBitCount = 0;
    }
    bih->biClrUsed = bih->biClrImportant = 0;
    bih->biCompression = fmt;

    zprintf(8, "seting format: %.4s(%08x) " BIH_FMT "\n",
            (char*)&fmt, fmt, BIH_FIELDS(bih));

    if(!capSetVideoFormat(vdo->state->hwnd, bih, vdo->state->bi_size))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "setting video format"));

    if(!capGetVideoFormat(vdo->state->hwnd, bih, vdo->state->bi_size))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "getting video format"));

    if(bih->biCompression != fmt)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "video format set ignored"));

    vdo->format = fmt;
    vdo->width = bih->biWidth;
    vdo->height = bih->biHeight;
    vdo->datalen = bih->biSizeImage;

    zprintf(4, "set new format: %.4s(%08x) " BIH_FMT "\n",
            (char*)&fmt, fmt, BIH_FIELDS(bih));
    return(0);
}

static int vfw_init (zbar_video_t *vdo,
                     uint32_t fmt)
{
    if(vfw_set_format(vdo, fmt))
        return(-1);

    HWND hwnd = vdo->state->hwnd;
    CAPTUREPARMS cp;
    if(!capCaptureGetSetup(hwnd, &cp, sizeof(cp)))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "retrieving capture parameters"));

    cp.dwRequestMicroSecPerFrame = 33333;
    cp.fMakeUserHitOKToCapture = 0;
    cp.wPercentDropForError = 90;
    cp.fYield = 1;
    cp.wNumVideoRequested = vdo->num_images;
    cp.fCaptureAudio = 0;
    cp.vKeyAbort = 0;
    cp.fAbortLeftMouse = 0;
    cp.fAbortRightMouse = 0;
    cp.fLimitEnabled = 0;

    if(!capCaptureSetSetup(hwnd, &cp, sizeof(cp)))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "setting capture parameters"));

    if(!capCaptureGetSetup(hwnd, &cp, sizeof(cp)))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "checking capture parameters"));

    
    capOverlay(hwnd, 0);

    if(!capPreview(hwnd, 0) ||
       !capPreviewScale(hwnd, 0))
        err_capture(vdo, SEV_WARNING, ZBAR_ERR_WINAPI, __func__,
                    "disabling preview");

    if(!capSetCallbackOnVideoStream(hwnd, vfw_stream_cb) ||
       !capSetCallbackOnError(hwnd, vfw_error_cb))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_BUSY, __func__,
                           "setting capture callbacks"));

    vdo->num_images = cp.wNumVideoRequested;
    vdo->iomode = VIDEO_MMAP; 

    zprintf(3, "initialized video capture: %d buffers %ldms/frame\n",
            vdo->num_images, cp.dwRequestMicroSecPerFrame);

    return(0);
}

static int vfw_cleanup (zbar_video_t *vdo)
{
    
    video_state_t *state = vdo->state;
    
    capDriverDisconnect(state->hwnd);
    DestroyWindow(state->hwnd);
    state->hwnd = NULL;
    _zbar_thread_stop(&state->thread, &vdo->qlock);

    if(state->captured) {
        CloseHandle(state->captured);
        state->captured = NULL;
    }
    return(0);
}

static int vfw_probe_format (zbar_video_t *vdo,
                             uint32_t fmt)
{
    const zbar_format_def_t *fmtdef = _zbar_format_lookup(fmt);
    if(!fmtdef)
        return(0);

    zprintf(4, "    trying %.4s(%08x)...\n", (char*)&fmt, fmt);
    BITMAPINFOHEADER *bih = vdo->state->bih;
    bih->biWidth = vdo->width;
    bih->biHeight = vdo->height;
    switch(fmtdef->group) {
    case ZBAR_FMT_GRAY:
        bih->biBitCount = 8;
        break;
    case ZBAR_FMT_YUV_PLANAR:
    case ZBAR_FMT_YUV_PACKED:
    case ZBAR_FMT_YUV_NV:
        bih->biBitCount = 8 + (16 >> (fmtdef->p.yuv.xsub2 + fmtdef->p.yuv.ysub2));
        break;
    case ZBAR_FMT_RGB_PACKED:
        bih->biBitCount = fmtdef->p.rgb.bpp * 8;
        break;
    default:
        bih->biBitCount = 0;
    }
    bih->biCompression = fmt;

    if(!capSetVideoFormat(vdo->state->hwnd, bih, vdo->state->bi_size)) {
        zprintf(4, "\tno (set fails)\n");
        return(0);
    }

    if(!capGetVideoFormat(vdo->state->hwnd, bih, vdo->state->bi_size))
        return(0);

    zprintf(6, "\tactual: " BIH_FMT "\n", BIH_FIELDS(bih));

    if(bih->biCompression != fmt) {
        zprintf(4, "\tno (set ignored)\n");
        return(0);
    }

    zprintf(4, "\tyes\n");
    return(1);
}

static int vfw_probe (zbar_video_t *vdo)
{
    video_state_t *state = vdo->state;
    state->bi_size = capGetVideoFormatSize(state->hwnd);
    BITMAPINFOHEADER *bih = state->bih = realloc(state->bih, state->bi_size);
    

    if(!capSetUserData(state->hwnd, (LONG)vdo) ||
       !state->bi_size || !bih ||
       !capGetVideoFormat(state->hwnd, bih, state->bi_size))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "setting up video capture"));

    zprintf(3, "initial format: " BIH_FMT " (bisz=%x)\n",
            BIH_FIELDS(bih), state->bi_size);

    if(!vdo->width || !vdo->height) {
        vdo->width = bih->biWidth;
        vdo->height = bih->biHeight;
    }
    vdo->datalen = bih->biSizeImage;

    zprintf(2, "probing supported formats:\n");
    vdo->formats = calloc(VFW_NUM_FORMATS, sizeof(uint32_t));

    int n = 0;
    const uint32_t *fmt;
    for(fmt = vfw_formats; *fmt; fmt++)
        if(vfw_probe_format(vdo, *fmt))
            vdo->formats[n++] = *fmt;

    vdo->formats = realloc(vdo->formats, (n + 1) * sizeof(uint32_t));

    vdo->width = bih->biWidth;
    vdo->height = bih->biHeight;
    vdo->intf = VIDEO_VFW;
    vdo->init = vfw_init;
    vdo->start = vfw_start;
    vdo->stop = vfw_stop;
    vdo->cleanup = vfw_cleanup;
    vdo->nq = vfw_nq;
    vdo->dq = vfw_dq;
    return(0);
}

int _zbar_video_open (zbar_video_t *vdo,
                      const char *dev)
{
    video_state_t *state = vdo->state;
    if(!state)
        state = vdo->state = calloc(1, sizeof(video_state_t));

    int reqid = -1;
    if((!strncmp(dev, "/dev/video", 10) ||
        !strncmp(dev, "\\dev\\video", 10)) &&
       dev[10] >= '0' && dev[10] <= '9' && !dev[11])
        reqid = dev[10] - '0';
    else if(strlen(dev) == 1 &&
            dev[0] >= '0' && dev[0] <= '9')
        reqid = dev[0] - '0';

    zprintf(6, "searching for camera: %s (%d)\n", dev, reqid);
    char name[MAX_NAME], desc[MAX_NAME];
    int devid;
    for(devid = 0; devid < MAX_DRIVERS; devid++) {
        if(!capGetDriverDescription(devid, name, MAX_NAME, desc, MAX_NAME)) {
            
            zprintf(6, "    [%d] not found...\n", devid);
            continue;
        }
        zprintf(6, "    [%d] %.100s - %.100s\n", devid, name, desc);
        if((reqid >= 0)
           ? devid == reqid
           : !strncmp(dev, name, MAX_NAME))
            break;
    }
    if(devid >= MAX_DRIVERS)
        return(err_capture_str(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                               "video device not found '%s'", dev));

    if(!state->captured)
        state->captured = CreateEvent(NULL, 0, 0, NULL);
    else
        ResetEvent(state->captured);

    if(_zbar_thread_start(&state->thread, vfw_capture_thread, vdo, NULL))
        return(-1);

    
    assert(state->hwnd);

    if(!capDriverConnect(state->hwnd, devid)) {
        _zbar_thread_stop(&state->thread, NULL);
        return(err_capture_str(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                               "failed to connect to camera '%s'", dev));
    }

    zprintf(1, "opened camera: %.60s (%d) (thr=%04lx)\n",
            name, devid, _zbar_thread_self());

    if(vfw_probe(vdo)) {
        _zbar_thread_stop(&state->thread, NULL);
        return(-1);
    }
    return(0);
}
