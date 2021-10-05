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

//****************************************************************************
/// @file  test_utils_memory.c
/// @brief Memory manipulation routines
//****************************************************************************
#include <unistd.h>
#include <stdint.h>
#include <dlfcn.h>
#include <assert.h>

#include "engineInternal.h"
#include "test_utils_memory.h"
#include "test_utils_log.h"

#ifndef NO_MALLOC_WRAPPER
pthread_mutex_t probeLock = PTHREAD_MUTEX_INITIALIZER;
pthread_t memHandlerThisThread = 0;
uint32_t *memHandlerFailProbes = NULL;
uint32_t *memHandlerProbeCounts = NULL;

int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr)
{
    static int (*real_pthread_mutex_init)(pthread_mutex_t *restrict, const pthread_mutexattr_t *restrict) = NULL;

    if (real_pthread_mutex_init == NULL)
    {
        real_pthread_mutex_init = dlsym(RTLD_NEXT, "pthread_mutex_init");
        assert(real_pthread_mutex_init != NULL);
    }

    if (memHandlerFailProbes != NULL)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        // Make sure there really are probes and that this is a thread we should play with
        pthread_mutex_lock(&probeLock);

        if (memHandlerFailProbes != NULL &&
            pThreadData != NULL && pThreadData->engineThreadId == IEMEM_FAILURE_TEST_THREADID)
        {
            uint32_t *failProbe = memHandlerFailProbes;
            uint32_t *probeCount = memHandlerProbeCounts;

            for(; *failProbe; failProbe++, probeCount++)
            {
                if (IEMEM_GET_MEMORY_TYPE(*failProbe) == iemem_numtypes)
                {
                    if (IEMEM_GET_MEMORY_PROBEID(*failProbe) ==
                        IEMEM_GET_MEMORY_PROBEID(TEST_IEMEM_PROBE_MUTEX_INIT))
                    {
                        if (memHandlerProbeCounts == NULL || *probeCount == 0)
                        {
                            pthread_mutex_unlock(&probeLock);
                            real_pthread_mutex_init(mutex, attr); // Do it but return error
                            return ENOMEM;
                        }

                        *probeCount = *probeCount - 1;
                        break;
                    }
                }
            }

            // All OK - call the real function
        }

        pthread_mutex_unlock(&probeLock);
    }

    return real_pthread_mutex_init(mutex, attr);
}

int pthread_condattr_init(pthread_condattr_t *restrict attr)
{
    static int (*real_pthread_condattr_init)(pthread_condattr_t *restrict) = NULL;

    if (real_pthread_condattr_init == NULL)
    {
        real_pthread_condattr_init = dlsym(RTLD_NEXT, "pthread_condattr_init");
        assert(real_pthread_condattr_init != NULL);
    }

    if (memHandlerFailProbes != NULL)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        // Make sure there really are probes and that this is a thread we should play with
        pthread_mutex_lock(&probeLock);

        if (memHandlerFailProbes != NULL && pThreadData != NULL && pThreadData->engineThreadId == UINT32_MAX)
        {
            uint32_t *failProbe = memHandlerFailProbes;
            uint32_t *probeCount = memHandlerProbeCounts;

            for(; *failProbe; failProbe++, probeCount++)
            {
                if (IEMEM_GET_MEMORY_TYPE(*failProbe) == iemem_numtypes)
                {
                    if (IEMEM_GET_MEMORY_PROBEID(*failProbe) ==
                        IEMEM_GET_MEMORY_PROBEID(TEST_IEMEM_PROBE_CONDATTR_INIT))
                    {
                        if (memHandlerProbeCounts == NULL || *probeCount == 0)
                        {
                            pthread_mutex_unlock(&probeLock);
                            real_pthread_condattr_init(attr); // Do it but return error
                            return ENOMEM;
                        }

                        *probeCount = *probeCount - 1;
                        break;
                    }
                }
            }

            // All OK - call the real function
        }

        pthread_mutex_unlock(&probeLock);
    }

    return real_pthread_condattr_init(attr);
}

int pthread_condattr_setclock(pthread_condattr_t *restrict attr, clockid_t clock_id)
{
    static int (*real_pthread_condattr_setclock)(pthread_condattr_t *restrict, clockid_t) = NULL;

    if (real_pthread_condattr_setclock == NULL)
    {
        real_pthread_condattr_setclock = dlsym(RTLD_NEXT, "pthread_condattr_setclock");
        assert(real_pthread_condattr_setclock != NULL);
    }

    if (memHandlerFailProbes != NULL)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        // Make sure there really are probes and that this is a thread we should play with
        pthread_mutex_lock(&probeLock);

        if (memHandlerFailProbes != NULL && pThreadData != NULL && pThreadData->engineThreadId == UINT32_MAX)
        {
            uint32_t *failProbe = memHandlerFailProbes;
            uint32_t *probeCount = memHandlerProbeCounts;

            for(; *failProbe; failProbe++, probeCount++)
            {
                if (IEMEM_GET_MEMORY_TYPE(*failProbe) == iemem_numtypes)
                {
                    if (IEMEM_GET_MEMORY_PROBEID(*failProbe) ==
                        IEMEM_GET_MEMORY_PROBEID(TEST_IEMEM_PROBE_CONDATTR_SETCLOCK))
                    {
                        if (memHandlerProbeCounts == NULL || *probeCount == 0)
                        {
                            pthread_mutex_unlock(&probeLock);
                            real_pthread_condattr_setclock(attr, clock_id); // Do it but return error
                            return EINVAL;
                        }

                        *probeCount = *probeCount - 1;
                        break;
                    }
                }
            }

            // All OK - call the real function
        }

        pthread_mutex_unlock(&probeLock);
    }

    return real_pthread_condattr_setclock(attr, clock_id);
}

int pthread_condattr_destroy(pthread_condattr_t *restrict attr)
{
    static int (*real_pthread_condattr_destroy)(pthread_condattr_t *restrict) = NULL;

    if (real_pthread_condattr_destroy == NULL)
    {
        real_pthread_condattr_destroy = dlsym(RTLD_NEXT, "pthread_condattr_destroy");
        assert(real_pthread_condattr_destroy != NULL);
    }

    if (memHandlerFailProbes != NULL)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        // Make sure there really are probes and that this is a thread we should play with
        pthread_mutex_lock(&probeLock);

        if (memHandlerFailProbes != NULL && pThreadData != NULL && pThreadData->engineThreadId == UINT32_MAX)
        {
            uint32_t *failProbe = memHandlerFailProbes;
            uint32_t *probeCount = memHandlerProbeCounts;

            for(; *failProbe; failProbe++, probeCount++)
            {
                if (IEMEM_GET_MEMORY_TYPE(*failProbe) == iemem_numtypes)
                {
                    if (IEMEM_GET_MEMORY_PROBEID(*failProbe) ==
                        IEMEM_GET_MEMORY_PROBEID(TEST_IEMEM_PROBE_CONDATTR_DESTROY))
                    {
                        if (memHandlerProbeCounts == NULL || *probeCount == 0)
                        {
                            pthread_mutex_unlock(&probeLock);
                            real_pthread_condattr_destroy(attr); // Do it but return error
                            return EINVAL;
                        }

                        *probeCount = *probeCount - 1;
                        break;
                    }
                }
            }

            // All OK - call the real function
        }

        pthread_mutex_unlock(&probeLock);
    }

    return real_pthread_condattr_destroy(attr);
}

#if 0
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    static int (*real_pthread_cond_init)(pthread_cond_t *, const pthread_condattr_t *) = NULL;

    if (real_pthread_cond_init == NULL)
    {
        real_pthread_cond_init = dlsym(RTLD_NEXT, "pthread_cond_init");
        assert(real_pthread_cond_init != NULL);
    }

    if (memHandlerFailProbes != NULL)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        // Make sure there really are probes and that this is a thread we should play with
        pthread_mutex_lock(&probeLock);

        if (memHandlerFailProbes != NULL && pThreadData != NULL && pThreadData->engineThreadId == UINT32_MAX)
        {
            uint32_t *failProbe = memHandlerFailProbes;
            uint32_t *probeCount = memHandlerProbeCounts;

            for(; *failProbe; failProbe++, probeCount++)
            {
                if (IEMEM_GET_MEMORY_TYPE(*failProbe) == iemem_numtypes)
                {
                    if (IEMEM_GET_MEMORY_PROBEID(*failProbe) ==
                        IEMEM_GET_MEMORY_PROBEID(TEST_IEMEM_PROBE_COND_INIT))
                    {
                        if (memHandlerProbeCounts == NULL || *probeCount == 0)
                        {
                            pthread_mutex_unlock(&probeLock);
                            real_pthread_cond_init(cond, attr); // Do it but return error
                            return ENOMEM;
                        }

                        *probeCount = *probeCount - 1;
                        break;
                    }
                }
            }

            // All OK - call the real function
        }

        pthread_mutex_unlock(&probeLock);
    }

    return real_pthread_cond_init(cond, attr);
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    static int (*real_pthread_cond_broadcast)(pthread_cond_t *) = NULL;

    if (real_pthread_cond_broadcast == NULL)
    {
        real_pthread_cond_broadcast = dlsym(RTLD_NEXT, "pthread_cond_broadcast");
        assert(real_pthread_cond_broadcast != NULL);
    }

    if (memHandlerFailProbes != NULL)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        // Make sure there really are probes and that this is a thread we should play with
        pthread_mutex_lock(&probeLock);

        if (memHandlerFailProbes != NULL && pThreadData != NULL && pThreadData->engineThreadId == UINT32_MAX)
        {
            uint32_t *failProbe = memHandlerFailProbes;
            uint32_t *probeCount = memHandlerProbeCounts;

            for(; *failProbe; failProbe++, probeCount++)
            {
                if (IEMEM_GET_MEMORY_TYPE(*failProbe) == iemem_numtypes)
                {
                    if (IEMEM_GET_MEMORY_PROBEID(*failProbe) ==
                        IEMEM_GET_MEMORY_PROBEID(TEST_IEMEM_PROBE_COND_BROADCAST))
                    {
                        if (memHandlerProbeCounts == NULL || *probeCount == 0)
                        {
                            pthread_mutex_unlock(&probeLock);
                            return EINVAL;
                        }

                        *probeCount = *probeCount - 1;
                        break;
                    }
                }
            }

            // All OK - call the real function
        }

        pthread_mutex_unlock(&probeLock);
    }

    return real_pthread_cond_broadcast(cond);
}
#endif

int pthread_spin_init(pthread_spinlock_t *spin, int pshared)
{
    static int (*real_pthread_spin_init)(pthread_spinlock_t *, int pshared) = NULL;

    if (real_pthread_spin_init == NULL)
    {
        real_pthread_spin_init = dlsym(RTLD_NEXT, "pthread_spin_init");
        assert(real_pthread_spin_init != NULL);
    }

    if (memHandlerFailProbes != NULL)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        // Make sure there really are probes and that this is a thread we should play with
        pthread_mutex_lock(&probeLock);

        if (memHandlerFailProbes != NULL && pThreadData != NULL && pThreadData->engineThreadId == UINT32_MAX)
        {
            uint32_t *failProbe = memHandlerFailProbes;
            uint32_t *probeCount = memHandlerProbeCounts;

            for(; *failProbe; failProbe++, probeCount++)
            {
                if (IEMEM_GET_MEMORY_TYPE(*failProbe) == iemem_numtypes)
                {
                    if (IEMEM_GET_MEMORY_PROBEID(*failProbe) ==
                        IEMEM_GET_MEMORY_PROBEID(TEST_IEMEM_PROBE_SPIN_INIT))
                    {
                        if (memHandlerProbeCounts == NULL || *probeCount == 0)
                        {
                            pthread_mutex_unlock(&probeLock);
                            return ENOMEM;
                        }

                        *probeCount = *probeCount - 1;
                        break;
                    }
                }
            }

            // All OK - call the real function
        }

        pthread_mutex_unlock(&probeLock);
    }

    return real_pthread_spin_init(spin, pshared);
}

void *iemem_malloc(ieutThreadData_t *pThreadData, uint32_t probe, size_t size)
{
    static void * (*real_iemem_malloc)(ieutThreadData_t *,uint32_t, size_t) = NULL;

    if (real_iemem_malloc == NULL)
    {
        real_iemem_malloc = dlsym(RTLD_NEXT, "iemem_malloc");
        assert(real_iemem_malloc != NULL);
    }

    if (memHandlerFailProbes != NULL)
    {
        // Make sure there really are probes
        pthread_mutex_lock(&probeLock);

        if (memHandlerFailProbes != NULL)
        {
            uint32_t *failProbe = memHandlerFailProbes;
            uint32_t *probeCount = memHandlerProbeCounts;

            if (*memHandlerFailProbes == TEST_ANALYSE_IEMEM_PROBE ||
                *memHandlerFailProbes == TEST_ANALYSE_IEMEM_PROBE_STACK)
            {
                printf("0x%08x: %30s IEMEM_PROBE(%2d, %d)\n",
                       probe, __func__, IEMEM_GET_MEMORY_TYPE(probe), IEMEM_GET_MEMORY_PROBEID(probe));

                if (*memHandlerFailProbes == TEST_ANALYSE_IEMEM_PROBE_STACK)
                {
                    test_log_backtrace();
                }
            }
            else
            {
                // If a special probe requested, move to the next one
                if (IEMEM_GET_MEMORY_TYPE(*memHandlerFailProbes) == iemem_numtypes) failProbe++;

                for(; *failProbe; failProbe++, probeCount++)
                {
                    if (*failProbe == probe)
                    {
                        if (memHandlerProbeCounts == NULL || *probeCount == 0)
                        {
                            if (*memHandlerFailProbes == TEST_ASSERT_IEMEM_PROBE)
                            {
                                assert(false);
                            }

                            pthread_mutex_unlock(&probeLock);
                            return NULL;
                        }

                        *probeCount = *probeCount - 1;
                        break;
                    }
                }
            }

            // All OK - call the real function
        }

        pthread_mutex_unlock(&probeLock);
    }

    return real_iemem_malloc(pThreadData, probe, size);
}

void *iemem_calloc(ieutThreadData_t *pThreadData, uint32_t probe, size_t nmemb, size_t size)
{
    static void * (*real_iemem_calloc)(ieutThreadData_t *, uint32_t, size_t, size_t) = NULL;

    if (real_iemem_calloc == NULL)
    {
        real_iemem_calloc = dlsym(RTLD_NEXT, "iemem_calloc");
        assert(real_iemem_calloc != NULL);
    }

    if (memHandlerFailProbes != NULL)
    {
        // Make sure there really are probes
        pthread_mutex_lock(&probeLock);

        if (memHandlerFailProbes != NULL)
        {
            uint32_t *failProbe = memHandlerFailProbes;
            uint32_t *probeCount = memHandlerProbeCounts;

            if (*memHandlerFailProbes == TEST_ANALYSE_IEMEM_PROBE ||
                *memHandlerFailProbes == TEST_ANALYSE_IEMEM_PROBE_STACK)
            {
                printf("0x%08x: %30s IEMEM_PROBE(%2d, %d)\n",
                       probe, __func__, IEMEM_GET_MEMORY_TYPE(probe), IEMEM_GET_MEMORY_PROBEID(probe));

                if (*memHandlerFailProbes == TEST_ANALYSE_IEMEM_PROBE_STACK)
                {
                    test_log_backtrace();
                }
            }
            else
            {
                // If a special probe requested, move to the next one
                if (IEMEM_GET_MEMORY_TYPE(*memHandlerFailProbes) == iemem_numtypes) failProbe++;

                for(; *failProbe; failProbe++, probeCount++)
                {
                    if (*failProbe == probe)
                    {
                        if (memHandlerProbeCounts == NULL || *probeCount == 0)
                        {
                            if (*memHandlerFailProbes == TEST_ASSERT_IEMEM_PROBE)
                            {
                                assert(false);
                            }

                            pthread_mutex_unlock(&probeLock);
                            return NULL;
                        }

                        *probeCount = *probeCount - 1;
                        break;
                    }
                }
            }

            // All OK - call the real function
        }

        pthread_mutex_unlock(&probeLock);
    }

    return real_iemem_calloc(pThreadData, probe, nmemb, size);
}

void *iemem_realloc(ieutThreadData_t *pThreadData, uint32_t probe, void *ptr, size_t size)
{
    static void * (*real_iemem_realloc)(ieutThreadData_t *, uint32_t, void *, size_t) = NULL;

    if (real_iemem_realloc == NULL)
    {
        real_iemem_realloc = dlsym(RTLD_NEXT, "iemem_realloc");
        assert(real_iemem_realloc != NULL);
    }

    if (memHandlerFailProbes != NULL)
    {
        // Make sure there really are probes
        pthread_mutex_lock(&probeLock);

        if (memHandlerFailProbes != NULL)
        {
            uint32_t *failProbe = memHandlerFailProbes;
            uint32_t *probeCount = memHandlerProbeCounts;

            if (*memHandlerFailProbes == TEST_ANALYSE_IEMEM_PROBE ||
                *memHandlerFailProbes == TEST_ANALYSE_IEMEM_PROBE_STACK)
            {
                printf("0x%08x: %30s IEMEM_PROBE(%2d, %d)\n",
                       probe, __func__, IEMEM_GET_MEMORY_TYPE(probe), IEMEM_GET_MEMORY_PROBEID(probe));

                if (*memHandlerFailProbes == TEST_ANALYSE_IEMEM_PROBE_STACK)
                {
                    test_log_backtrace();
                }
            }
            else
            {
                // If a special probe requested, move to the next one
                if (IEMEM_GET_MEMORY_TYPE(*memHandlerFailProbes) == iemem_numtypes) failProbe++;

                for(; *failProbe; failProbe++, probeCount++)
                {
                    if (*failProbe == probe)
                    {
                        if (memHandlerProbeCounts == NULL || *probeCount == 0)
                        {
                            if (*memHandlerFailProbes == TEST_ASSERT_IEMEM_PROBE)
                            {
                                assert(false);
                            }

                            pthread_mutex_unlock(&probeLock);
                            return NULL;
                        }

                        *probeCount = *probeCount - 1;
                        break;
                    }
                }
            }

            // All OK - call the real function
        }

        pthread_mutex_unlock(&probeLock);
    }

    return real_iemem_realloc(pThreadData, probe, ptr, size);
}

//****************************************************************************
/// @brief  Provide a list of memory allocation probes to fail and set / reset
///         calling thread's engineThreadId to IEMEM_FAILURE_TEST_THREADID
///
/// @param[in]     probesToFail  List of probes to fail.
/// @param[in]     probeCounts   Counts of each probe to allow before failing
///                              (or pass the original thread id if unsetting probes)
///
/// @remark The list is terminated with a 0, only the probes specified will fail
///         automatically, others will call the real underlying iemem_*alloc.
///         Setting NULL will turn calling iemem_*alloc back on.
///
///         A NULL probeCounts pointer will mean all of the specified probes
///         will fail - non null, we decrement the probe count each time the
///         probe is seen, once it hits zero, it starts failing.
///
/// @return The original threadId for the calling thread unless it has been
///         set to IEMEM_FAILURE_TEST_THREADID
//****************************************************************************
uint32_t test_memoryFailMemoryProbes(uint32_t *probesToFail, uint32_t *probeCounts)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t originalThreadId = 0;

    if (pThreadData)
    {
        if (probesToFail == NULL)
        {
            if (probeCounts != NULL) pThreadData->engineThreadId = *probeCounts;
        }
        else if (pThreadData->engineThreadId != IEMEM_FAILURE_TEST_THREADID)
        {
            originalThreadId = pThreadData->engineThreadId;
            pThreadData->engineThreadId = IEMEM_FAILURE_TEST_THREADID;
        }
    }

    pthread_mutex_lock(&probeLock);
    memHandlerFailProbes = probesToFail;
    memHandlerProbeCounts = probeCounts;
    pthread_mutex_unlock(&probeLock);

    return originalThreadId;
}
#else
void test_memoryFailMemoryProbes(uint32_t *probesToFail, uint32_t *probeCounts)
{
    printf("No Memory wrapper, ignoring request to fail alloc calls\n");
    return;
}
#endif
