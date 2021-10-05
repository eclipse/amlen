/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define TRACE_COMP Util

#include <ismutil.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/*
 * This source file contains definitions of timer-related utility functions
 * that start a timer, schedule a one-time or recurring task and cancel a task.
 */

#define THOUSAND       1000
#define MILLION        1000000
#define BILLION        1000000000
typedef struct epoll_event epoll_event;

/*
 * A timer task structure.
 */
typedef struct TimerTask_t {
    ism_attime_t function; // A pointer to a scheduled function. If NULL, can be cleaned up
    void * userData; // A pointer to the user data to be passed to the scheduled function
    int timer_fd; // TimerTask file descriptor
    int isPeriodic; // Is this a periodic task
    pthread_spinlock_t stateLock;
    int state;
    int timer;
    struct TimerTask_t * prev;
    struct TimerTask_t * next;
} TimerTask_t;

typedef struct TimerThread_t {
    ism_threadh_t thread; //Timer thread handle
    pthread_spinlock_t lock; //Timer synchronization lock
    int efd; //epoll fd			//Timer epoll file descriptor
    int pipe_wfd; //Pipe  file descriptor (used to stop the timer)
    TimerTask_t * tasksListHead;//List of all timer tasks
    TimerTask_t * canceledTasks;//List of canceled timer tasks
} TimerThread_t;

static TimerThread_t timerThreads[2];

typedef struct handler_t {
    struct handler_t * next;
    ism_handler_f      callback;
    void *             userdata;
    uint8_t            inuse;
    uint8_t            toDelete;
} handler_t;

handler_t * handlers;

int ism_common_isServer(void);

static int activeTimersCount = 0;
static volatile int stoppedTimersCount = 0;

static pthread_mutex_t handlerlock = PTHREAD_MUTEX_INITIALIZER;

static void freeTimer(TimerTask_t *tt) {
    pthread_spin_destroy(&tt->stateLock);
    ism_common_free(ism_memory_utils_misc,tt);
}

static ism_timer_t addTimer(ism_priority_class_e timer, ism_attime_t attime, void * userdata, ism_time_t delay,
        ism_time_t interval, const char *file,  int line) {
    struct itimerspec tspec;
    epoll_event event;
    TimerThread_t * timerThread = &timerThreads[timer];
    TimerTask_t * tt = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,41),1, sizeof(TimerTask_t));
    int osrc = pthread_spin_init(&tt->stateLock, PTHREAD_PROCESS_PRIVATE);
    if (osrc != 0) {
        TRACE(3, "Failed to initialize spinlock for timerTask %p. rc=%d.\n", tt, osrc);
        ism_common_free(ism_memory_utils_misc,tt);
        return NULL;
    }
    tt->timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tt->timer_fd < 0) {
        freeTimer(tt);
        return NULL;
    }
    tt->function = attime;
    tt->userData = userdata;
    tt->isPeriodic = (interval > 0);
    tt->timer = timer;
    tspec.it_value.tv_sec = delay / 1000000000;
    tspec.it_value.tv_nsec = delay % 1000000000;
    tspec.it_interval.tv_sec = interval / 1000000000;
    tspec.it_interval.tv_nsec = interval % 1000000000;
    if (timerfd_settime(tt->timer_fd, 0, &tspec, NULL) < 0) {
        close(tt->timer_fd);
        freeTimer(tt);
        return NULL;
    }
    pthread_spin_lock(&timerThread->lock);
    if (timerThread->tasksListHead) {
        tt->next = timerThread->tasksListHead;
        timerThread->tasksListHead->prev = tt;
    }
    timerThread->tasksListHead = tt;
    activeTimersCount++;
    pthread_spin_unlock(&timerThread->lock);
    if(SHOULD_TRACE(9)) {
        traceFunction(9, 0, file, line, "addTimer(%s): timer=%p callback=%p userdata=%p delay=%lu interval=%lu fd=%d\n",
                ((timer) ? "LOW" : "HIGH"), tt, attime, userdata, delay, interval, tt->timer_fd);
    }
    memset(&event, 0, sizeof(epoll_event));
    event.data.ptr = tt;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    if (epoll_ctl(timerThread->efd, EPOLL_CTL_ADD, tt->timer_fd, &event) < 0) {
        memset(&tspec, 0, sizeof(tspec));
        timerfd_settime(tt->timer_fd, 0, &tspec, NULL);
        close(tt->timer_fd);
        pthread_spin_lock(&timerThread->lock);
        if (tt->prev) {
            tt->prev->next = tt->next;
        } else {
            timerThread->tasksListHead = tt->next;
        }
        if (tt->next) {
            tt->next->prev = tt->prev;
        }
        activeTimersCount--;
        pthread_spin_unlock(&timerThread->lock);
        freeTimer(tt);
        return NULL;
    }
    return tt;
}

static void stopTimerTask(TimerTask_t * tt, const char * file, int line) {
    pthread_spin_lock(&tt->stateLock);
    if(SHOULD_TRACE(9)) {
        traceFunction(9, 0, file, line, "stopTimerTask: timer=%p state=%d\n", tt, tt->state);
    }
    if (tt->state == 0) {
        tt->state = 1;
        pthread_spin_unlock(&tt->stateLock);
        struct itimerspec tspec;
        TimerThread_t * timerThread = &timerThreads[tt->timer];
        memset(&tspec, 0, sizeof(tspec));
        timerfd_settime(tt->timer_fd, 0, &tspec, NULL);
        epoll_ctl(timerThread->efd, EPOLL_CTL_DEL, tt->timer_fd, NULL);
        close(tt->timer_fd);
        __sync_fetch_and_add(&stoppedTimersCount, 1);
    } else {
        pthread_spin_unlock(&tt->stateLock);
    }
}

/*
 * Schedule a task to be executed once.
 * @param  timer    The timer type (ISM_TIMER_HIGH/ISM_TIMER_LOW).
 * @param  attime   The address of the scheduled function to execute.
 * @param  userdata The pointer to the user data to be passed to the scheduled function.
 * @param  delay     The delay (in nanoseconds) before the function has to be executed.
 * @return The identifier for the scheduled task.
 */
ism_timer_t ism_common_setTimerOnceInt(ism_priority_class_e timer, ism_attime_t attime, void * userdata,
        ism_time_t delay, const char * file, int line) {
    return addTimer(timer, attime, userdata, delay, 0, file, line);
}

/*
 * Schedule a task to be executed repeatedly.
 * @param  timer    The timer type (ISM_TIMER_HIGH/ISM_TIMER_LOW).
 * @param  attime   The address of the scheduled function to execute.
 * @param  userdata The pointer to the user data to be passed to the scheduled function.
 * @param  delay    The delay before the first execution of the function, in units as specified by units parameter.
 * @param  period   The length of the interval between successive executions of the scheduled function,
 *                  in units as specified by units parameter.
 * @param  units    Time units used for delay and period parameters.
 * @return The identifier for the scheduled task.
 */
ism_timer_t ism_common_setTimerRateInt(ism_priority_class_e timer, ism_attime_t attime, void * userdata, uint64_t delay,
        uint64_t period, enum ism_timer_e units, const char * file, int line) {
    ism_time_t delayNS;
    ism_time_t periodNS;
    switch (units) {
        case TS_SECONDS:
            delayNS = (delay * BILLION);
            periodNS = (period * BILLION);
            break;
        case TS_MILLISECONDS:
            delayNS = (delay * MILLION);
            periodNS = (period * MILLION);
            break;
        case TS_MICROSECONDS:
            delayNS = (delay * THOUSAND);
            periodNS = (period * THOUSAND);
            break;
        case TS_NANOSECONDS:
        default:
            delayNS = delay;
            periodNS = period;
            break;
    }
    return addTimer(timer, attime, userdata, delayNS, periodNS, file, line);
}

/*
 * Cancel a scheduled task.
 * @param  tt      An identifier for the task to be canceled.
 * @return A return code: 0=found and canceled, -1=not found.
 */
int ism_common_cancelTimerInt(TimerTask_t * tt, const char * file, int line) {
	if(tt == NULL)
		return -1;
    stopTimerTask(tt, file, line);
    pthread_spin_lock(&tt->stateLock);
    if (tt->state == 1) {
        tt->state = 2;
        pthread_spin_unlock(&tt->stateLock);
        int needWrite;
        TimerThread_t * timerThread = &timerThreads[tt->timer];
        pthread_spin_lock(&timerThread->lock);
        if (tt->prev) {
            tt->prev->next = tt->next;
        } else {
            timerThread->tasksListHead = tt->next;
        }
        if (tt->next) {
            tt->next->prev = tt->prev;
        }
        needWrite = (timerThread->canceledTasks == NULL);
        tt->next = timerThread->canceledTasks;
        timerThread->canceledTasks = tt;
        activeTimersCount--;
        pthread_spin_unlock(&timerThread->lock);
        __sync_fetch_and_sub(&stoppedTimersCount,1);
        if (needWrite) {
            char c = 'C';
            ssize_t bytes_written = write(timerThread->pipe_wfd, &c, 1);
            if (bytes_written != 1) {
                int err = errno;
                TRACE(1, "Error cancelling timer task: %s (%d) %s %d\n", strerror(err), err,
                                 file, line);
                assert(0);
            }
        }
        return 0;
    } else {
        pthread_spin_unlock(&tt->stateLock);
    }
    return -1;
}

int g_doUser2 = 0;

/* Main function for timer thread */
static void * timerThreadProc(void * param, void * context, int value) {
    TimerThread_t * thData = (TimerThread_t*) context;
    pthread_barrier_t * initBarrier = (pthread_barrier_t*) param;
    int eventSize = 64 * 1024;
    epoll_event * events;
    int pipefd[2];
    int run = 1;
    int efd;
    int checkUserSig = value==0 && ism_common_isServer();
    int rc = pipe2(pipefd, O_NONBLOCK | O_CLOEXEC);
    int err;

    if (rc) {
        err = errno;
        TRACE(1, "Error creating timer pipe: %s (%d)\n", strerror(errno), errno);
        pthread_barrier_wait(initBarrier);
        ism_common_endThread(NULL);
    }
    efd = epoll_create1(EPOLL_CLOEXEC);
    if (efd <= 0) {
        err = errno;
        TRACE(1, "Error creating timer epoll: %s (%d)\n", strerror(errno), errno);
        pthread_barrier_wait(initBarrier);
        ism_common_endThread(NULL);
    }
    events = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,43),eventSize, sizeof(epoll_event));
    events[0].data.fd = pipefd[0];
    events[0].events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, pipefd[0], &events[0]) == -1) {
        ism_common_free(ism_memory_utils_misc,events);
        err = errno;
        TRACE(1, "Error adding timer to epoll: %s (%d)\n", strerror(errno), errno);
        pthread_barrier_wait(initBarrier);
        ism_common_endThread(NULL);
    }
    pthread_spin_init(&thData->lock, 0);
    thData->efd = efd;
    thData->pipe_wfd = pipefd[1];
    pthread_barrier_wait(initBarrier);
    while (run) {
        int i;
        int count;
        TimerTask_t * canceledTasks;
        pthread_spin_lock(&thData->lock);
        canceledTasks = thData->canceledTasks;
        thData->canceledTasks = NULL;
        pthread_spin_unlock(&thData->lock);

        while (canceledTasks) {
            TimerTask_t * tt = canceledTasks;
            canceledTasks = tt->next;
            freeTimer(tt);
        }

        count = epoll_wait(efd, events, eventSize, -1); /* BEAM suppression: using deallocated */
        if (count <=  0) {
            if (count == -1 && errno != EINTR) {
                err = errno;
                TRACE(1, "Error in timer epoll_wait: %s (%d)\n", strerror(err), err);
                break;
            }
        }

        /* In the server, run the user signal handlers in the high timer loop */
        if (checkUserSig && __sync_add_and_fetch(&g_doUser2, 0)) {
            g_doUser2 = 0;
            ism_common_runUserHandlers();
        }

        /* Run each available timer */
        if (count > 0) {
            ism_time_t currentTime = ism_common_currentTimeNanos();
            for (i = 0; i < count; i++) {
                struct epoll_event * event = &events[i];
                if (event->data.fd != pipefd[0]) {
                    TimerTask_t * tt = event->data.ptr;
                    if (tt) {
                        pthread_spin_lock(&tt->stateLock);
                        if (tt->state == 0) {
                            uint64_t exp = 0;
                            // fprintf(stderr,"**** timerThreadProc: timer expired: %d\n",tt->timer_fd);
                            int n = read(tt->timer_fd, &exp, sizeof(uint64_t));
                            int read_errno = errno;
                            pthread_spin_unlock(&tt->stateLock);
                            if (n == sizeof(uint64_t)) {
                                // TRACE(9, "Timer %p expired: function=%p userData=%p\n", tt, tt->function, tt->userData);
                                rc = tt->function(tt, currentTime, tt->userData);
                                // TRACE(9, "After timer %p invocation rc = %d\n", tt, rc);
                                if (rc && tt->isPeriodic)
                                    continue;
                                stopTimerTask(tt, __FILE__, __LINE__);
                                continue;
                            } else {
                                if (((event->events & ~EPOLLIN) != 0) || (read_errno != EAGAIN)) {
                                    TRACE(7, "Timer %p run with errno %d (events: 0x%08x)\n", tt, read_errno, event->events);
                                }
                            }
                            continue;
                        } else {
                            pthread_spin_unlock(&tt->stateLock);
                        }
                    }
                } else {
                    char c;
                    while (read(pipefd[0], &c, 1) > 0) {
                        if (c == 'S') {
                            run = 0;
                            break;
                        }
                    }
                    if (run)
                        continue;
                    break;
                }
            }
            if (count == eventSize) {
                eventSize *= 2;
                events = ism_common_realloc(ISM_MEM_PROBE(ism_memory_utils_misc,45),events, eventSize * sizeof(epoll_event));
            }
        }
    }
    close(efd); /* BEAM suppression: violated property */
    pthread_spin_lock(&thData->lock);
    while (thData->tasksListHead) {
        TimerTask_t * tt = thData->tasksListHead;
        thData->tasksListHead = tt->next;
        close(tt->timer_fd);
        freeTimer(tt);
    }
    pthread_spin_unlock(&thData->lock);
    pthread_spin_destroy(&thData->lock);
    ism_common_free(ism_memory_utils_misc,events);
    ism_common_endThread(NULL);
    return NULL;
}

int ism_common_traceFlush(int millis);

/*
 * Initialize timers for high- and low-priority events.
 */
void ism_common_initTimers(void) {
    pthread_barrier_t initBarrier;
    memset(timerThreads, 0, 2 * sizeof(TimerThread_t));
    pthread_barrier_init(&initBarrier, NULL, 3);

    ism_common_startThread(&timerThreads[ISM_TIMER_HIGH].thread, timerThreadProc, &initBarrier,
            &timerThreads[ISM_TIMER_HIGH], ISM_TIMER_HIGH, ISM_TUSAGE_TIMER, 0, "timer0",
            "A timer for high-priority tasks");

    ism_common_startThread(&timerThreads[ISM_TIMER_LOW].thread, timerThreadProc, &initBarrier,
            &timerThreads[ISM_TIMER_LOW], ISM_TIMER_LOW, ISM_TUSAGE_CLEANUP, 0, "timer1",
            "A timer for low-priority tasks");

    pthread_barrier_wait(&initBarrier);

    pthread_barrier_destroy(&initBarrier);

    /* Set up trace flushing.  This can only be done after setting up the timers */
    ism_common_traceFlush(ism_common_getIntConfig("TraceFlush", 2000));
}

/*
 * Stop timers for high- and low-priority events.
 */
void ism_common_stopTimers(void) {
    void * result = NULL;
    char c = 'S';
    int rc0 = write(timerThreads[ISM_TIMER_LOW].pipe_wfd, &c, 1);
    int rc1 = write(timerThreads[ISM_TIMER_HIGH].pipe_wfd, &c, 1);
    if (rc0 > 0)
        pthread_join(timerThreads[ISM_TIMER_LOW].thread, &result);
    if (rc1 > 1)
        pthread_join(timerThreads[ISM_TIMER_HIGH].thread, &result);
}

/*
 * Add a handler for user signals.
 *
 * As there can be multiple users of the user signal, the handler must
 * check that the waited for event is complete.
 *
 * @param callback The callback method
 * @param userdata The userdata to send as data to the callback
 */
ism_handler_t ism_common_addUserHandler(ism_handler_f callback, void * userdata) {
    if (!callback)
        return NULL;
    pthread_mutex_lock(&handlerlock);
    handler_t * handler = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,47),1, sizeof(handler_t));
    handler->callback = callback;
    handler->userdata = userdata;
    handler->next = handlers;
    handlers = handler;
    TRACE(9, "addUserHandler: handler=%p handlers=%p next=%p\n", handler, handlers, handler->next);
    pthread_mutex_unlock(&handlerlock);
    return handler;
}

/*
 * Remove a user handler.
 *
 * This cannot be called from within a handler callback.  To remove a handler
 * while running a callback, return -1 from it.
 */
void ism_common_removeUserHandler(ism_handler_t rmhandler) {
    handler_t * prev = NULL;
    handler_t * handler;
    if (!rmhandler)
        return;
    pthread_mutex_lock(&handlerlock);
    handler = handlers;
    while (handler) {
        if (handler == rmhandler) {
            if (handler->inuse) {
                handler->toDelete = 1;
            } else {
                TRACE(9, "removeUserHandler: handler=%p prev=%p next=%p\n", handler, prev, handler->next);
                if (prev)
                    prev->next = handler->next;
                else
                    handlers = handler->next;
                ism_common_free(ism_memory_utils_misc,handler);
            }
            return;
        } else {
            prev = handler;
            handler = handler->next;
        }
    }
    pthread_mutex_unlock(&handlerlock);
}

/*
 * Get the signal number to use for the user signal
 */
int ism_common_userSignal(void) {
    return SIGUSR2;
}

/*
 * Run all handlers.
 * This is called when the signal is received
 */
void ism_common_runUserHandlers(void) {
    handler_t * handler;
    handler_t * prev = NULL;
    int count = 0;
    TRACE(9, ">>> enter runUserHandlers: handlers=%p\n", handlers);
    pthread_mutex_lock(&handlerlock);
    handler = handlers;
    while (handler) {
        int  rc;
        handler->inuse = 1;
        pthread_mutex_unlock(&handlerlock);
        TRACE(9, "runUserHandler: %p\n", handler);
        rc = handler->callback(handler->userdata);
        pthread_mutex_lock(&handlerlock);
        count++;
        handler->inuse = 0;

        /* Delete the handler if requested */
        if (rc == -1 || handler->toDelete) {
            handler_t * findhandler = handlers;
            handler_t * delhandler = handler;
            /* Re-calculate prev which could have changed while we released the handlerlock */
            prev = NULL;
            while (findhandler && findhandler != handler) {
                prev = findhandler;
                findhandler = findhandler->next;
            }
            if (!findhandler) {
                TRACE(1, "runUserHandle error: current handler not in list: %p\n", handler);
                LOG(ERROR, Server, 999, "%s", "{0}", "System error in user handler list.  Name resolution may be incorrect.");
            } else {
                TRACE(9, "removeHandler: rc=%d handler=%p prev=%p next=%p\n", rc, handler, prev, handler->next);
                if (prev)
                    prev->next = handler->next;
                else
                    handlers = handler->next;
                handler = handler->next;
                ism_common_free(ism_memory_utils_misc,delhandler);
            }
        } else {
            prev = handler;
            handler = handler->next;
        }
    }
    pthread_mutex_unlock(&handlerlock);
    TRACE(9, "<<< leave runUserHandlers: count=%u\n", count);
}
