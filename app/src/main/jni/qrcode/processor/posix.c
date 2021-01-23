

#include "processor.h"
#include "posix.h"
#include <unistd.h>
#include <assert.h>
#include <errno.h>


static inline int proc_sleep (int timeout)
{
    assert(timeout > 0);
    struct timespec sleepns, remns;
    sleepns.tv_sec = timeout / 1000;
    sleepns.tv_nsec = (timeout % 1000) * 1000000;
    while(nanosleep(&sleepns, &remns) && errno == EINTR)
        sleepns = remns;
    return(1);
}

int _zbar_event_init (zbar_event_t *event)
{
    event->state = 0;
    event->pollfd = -1;
#ifdef HAVE_LIBPTHREAD
    pthread_cond_init(&event->cond, NULL);
#endif
    return(0);
}

void _zbar_event_destroy (zbar_event_t *event)
{
    event->state = -1;
    event->pollfd = -1;
#ifdef HAVE_LIBPTHREAD
    pthread_cond_destroy(&event->cond);
#endif
}


void _zbar_event_trigger (zbar_event_t *event)
{
    event->state = 1;
#ifdef HAVE_LIBPTHREAD
    pthread_cond_broadcast(&event->cond);
#endif
    if(event->pollfd >= 0) {
        unsigned i = 0; 
        if(write(event->pollfd, &i, sizeof(unsigned)) < 0)
            perror("");
        event->pollfd = -1;
    }
}

#ifdef HAVE_LIBPTHREAD


int _zbar_event_wait (zbar_event_t *event,
                      zbar_mutex_t *lock,
                      zbar_timer_t *timeout)
{
    int rc = 0;
    while(!rc && !event->state) {
        if(!timeout)
            rc = pthread_cond_wait(&event->cond, lock);
        else {
            struct timespec *timer;
# if _POSIX_TIMERS > 0
            timer = timeout;
# else
            struct timespec tmp;
            tmp.tv_sec = timeout->tv_sec;
            tmp.tv_nsec = timeout->tv_usec * 1000;
            timer = &tmp;
# endif
            rc = pthread_cond_timedwait(&event->cond, lock, timer);
        }
    }

    
    event->state = 0;

    if(!rc)
        return(1); 
    if(rc == ETIMEDOUT)
        return(0); 
    return(-1); 
}

int _zbar_thread_start (zbar_thread_t *thr,
                        zbar_thread_proc_t *proc,
                        void *arg,
                        zbar_mutex_t *lock)
{
    if(thr->started || thr->running)
        return(-1);
    thr->started = 1;
    _zbar_event_init(&thr->notify);
    _zbar_event_init(&thr->activity);

    int rc = 0;
    _zbar_mutex_lock(lock);
    if(pthread_create(&thr->tid, NULL, proc, arg) ||
       _zbar_event_wait(&thr->activity, lock, NULL) < 0 ||
       !thr->running) {
        thr->started = 0;
        _zbar_event_destroy(&thr->notify);
        _zbar_event_destroy(&thr->activity);
        
        rc = -1;
    }
    _zbar_mutex_unlock(lock);
    return(rc);
}

int _zbar_thread_stop (zbar_thread_t *thr,
                       zbar_mutex_t *lock)
{
    if(thr->started) {
        thr->started = 0;
        _zbar_event_trigger(&thr->notify);
        while(thr->running)
            
            _zbar_event_wait(&thr->activity, lock, NULL);
        pthread_join(thr->tid, NULL);
        _zbar_event_destroy(&thr->notify);
        _zbar_event_destroy(&thr->activity);
    }
    return(0);
}

#else

int _zbar_event_wait (zbar_event_t *event,
                      zbar_mutex_t *lock,
                      zbar_timer_t *timeout)
{
    int rc = !event->state;
    if(rc) {
        if(!timeout)
            
            return(-1);

        int sleep = _zbar_timer_check(timeout);
        if(sleep)
            proc_sleep(sleep);
    }

    rc = !event->state;

    
    event->state = 0;

    return(rc);
}

#endif


static int proc_video_handler (zbar_processor_t *proc,
                               int i)
{
    _zbar_mutex_lock(&proc->mutex);
    _zbar_processor_lock(proc);
    _zbar_mutex_unlock(&proc->mutex);

    zbar_image_t *img = NULL;
    if(proc->streaming) {
        
        img = zbar_video_next_image(proc->video);
        if(img)
            _zbar_process_image(proc, img);
    }

    _zbar_mutex_lock(&proc->mutex);
    _zbar_processor_unlock(proc, 0);
    _zbar_mutex_unlock(&proc->mutex);
    if(img)
        zbar_image_destroy(img);
    return(0);
}

static inline void proc_cache_polling (processor_state_t *state)
{
    
    int n = state->thr_polling.num = state->polling.num;
    alloc_polls(&state->thr_polling);
    memcpy(state->thr_polling.fds, state->polling.fds,
           n * sizeof(struct pollfd));
    memcpy(state->thr_polling.handlers, state->polling.handlers,
           n * sizeof(poll_handler_t*));
}

static int proc_kick_handler (zbar_processor_t *proc,
                              int i)
{
    processor_state_t *state = proc->state;
    zprintf(5, "kicking %d fds\n", state->polling.num);

    unsigned junk[2];
    int rc = read(state->kick_fds[0], junk, 2 * sizeof(unsigned));

    assert(proc->threaded);
    _zbar_mutex_lock(&proc->mutex);
    proc_cache_polling(proc->state);
    _zbar_mutex_unlock(&proc->mutex);
    return(rc);
}

static inline int proc_poll_inputs (zbar_processor_t *proc,
                                    int timeout)
{
    processor_state_t *state = proc->state;
    if(state->pre_poll_handler)
        state->pre_poll_handler(proc, -1);

    poll_desc_t *p = &state->thr_polling;
    assert(p->num);
    int rc = poll(p->fds, p->num, timeout);
    if(rc <= 0)
        
        return(rc);
    int i;
    for(i = p->num - 1; i >= 0; i--)
        if(p->fds[i].revents) {
            if(p->handlers[i])
                p->handlers[i](proc, i);
            p->fds[i].revents = 0; 
            rc--;
        }
    assert(!rc);
    return(1);
}

int _zbar_processor_input_wait (zbar_processor_t *proc,
                                zbar_event_t *event,
                                int timeout)
{
    processor_state_t *state = proc->state;
    if(state->thr_polling.num) {
        if(event) {
            _zbar_mutex_lock(&proc->mutex);
            event->pollfd = state->kick_fds[1];
            _zbar_mutex_unlock(&proc->mutex);
        }
        return(proc_poll_inputs(proc, timeout));
    }
    else if(timeout)
        return(proc_sleep(timeout));
    return(-1);
}

int _zbar_processor_init (zbar_processor_t *proc)
{
    processor_state_t *state = proc->state =
        calloc(1, sizeof(processor_state_t));
    state->kick_fds[0] = state->kick_fds[1] = -1;

    if(proc->threaded) {
        
        if(pipe(state->kick_fds))
            return(err_capture(proc, SEV_FATAL, ZBAR_ERR_SYSTEM, __func__,
                               "failed to open pipe"));
        add_poll(proc, state->kick_fds[0], proc_kick_handler);
        proc_cache_polling(proc->state);
    }
    return(0);
}

int _zbar_processor_cleanup (zbar_processor_t *proc)
{
    processor_state_t *state = proc->state;
    if(proc->threaded) {
        close(state->kick_fds[0]);
        close(state->kick_fds[1]);
        state->kick_fds[0] = state->kick_fds[1] = -1;
    }
    if(state->polling.fds) {
        free(state->polling.fds);
        state->polling.fds = NULL;
        if(!proc->threaded)
            state->thr_polling.fds = NULL;
    }
    if(state->polling.handlers) {
        free(state->polling.handlers);
        state->polling.handlers = NULL;
        if(!proc->threaded)
            state->thr_polling.handlers = NULL;
    }
    if(state->thr_polling.fds) {
        free(state->thr_polling.fds);
        state->thr_polling.fds = NULL;
    }
    if(state->thr_polling.handlers) {
        free(state->thr_polling.handlers);
        state->thr_polling.handlers = NULL;
    }
    free(proc->state);
    proc->state = NULL;
    return(0);
}

int _zbar_processor_enable (zbar_processor_t *proc)
{
    int vid_fd = zbar_video_get_fd(proc->video);
    if(vid_fd < 0)
        return(0);

    if(proc->streaming)
        add_poll(proc, vid_fd, proc_video_handler);
    else
        remove_poll(proc, vid_fd);
    
    return(0);
}
