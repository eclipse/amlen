/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
#define TRACE_COMP Engine

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <sys/prctl.h>

#include "engineInternal.h"
#include "fakeAsyncStore.h"

#ifdef USEFAKE_ASYNC_COMMIT

typedef struct
{
    char StructId[4];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    size_t origCapacity;        // original number of items
    size_t capacity;            // maximum number of items in the buffer
    size_t count;               // number of items in the buffer
    bool warnTriggered;
    callbackEntry_t *head;      // pointer to head
    callbackEntry_t *tail;      // pointer to tail
    callbackEntry_t  *entries; //pointer to array of entries of size: capacity
} callbackArray_t;

#define CALLBACKARRAY_STRUCTID "FSCA"

#define HWM_FRACTION 0.8
#define LWM_FRACTION 0.4

#define NUM_CALLBACK_THREADS 1

static uint64_t nextThreadId = 0;
static bool callbacksPaused = false;
static bool keepOnCallBacking = true;
static callbackArray_t *cbs = NULL;
static pthread_t cbThreads[NUM_CALLBACK_THREADS] = {0};

static callbackArray_t *callbackAray_create(size_t capacity)
{
    callbackArray_t *pArray = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,25), sizeof(callbackArray_t));

    if (pArray != NULL)
    {
        pArray->entries = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,26),capacity * sizeof(callbackEntry_t));

        if (pArray->entries == NULL)
        {
            ism_common_free(ism_memory_engine_misc,pArray);
            pArray = NULL;
        }
    }

    if (pArray != NULL)
    {
        ismEngine_SetStructId(pArray->StructId, CALLBACKARRAY_STRUCTID);
        pArray->capacity    = capacity;
        pArray->origCapacity = capacity;
        pArray->count = 0;
        pArray->head = &(pArray->entries[0]);
        pArray->tail = &(pArray->entries[0]);
        pArray->warnTriggered = false;

        DEBUG_ONLY int osrc = pthread_mutex_init(&(pArray->mutex), NULL);
        assert(osrc == 0);

        osrc = pthread_cond_init(&(pArray->cond), NULL);
        assert(osrc == 0);
    }

    return pArray;
}

static void callbackArray_free(callbackArray_t *pArray)
{
    ismEngine_InvalidateStructId(pArray->StructId);

    DEBUG_ONLY int osrc = pthread_mutex_destroy(&(pArray->mutex));
    assert(osrc == 0);

    osrc = pthread_cond_destroy(&(pArray->cond));
    assert(osrc == 0);

    ism_common_free(ism_memory_engine_misc,pArray->entries);
    ism_common_free(ism_memory_engine_misc,pArray);
}

static void callbackArray_add(callbackArray_t *pArray, callbackEntry_t *pEntry)
{
    bool added = false;

    do
    {
        DEBUG_ONLY int osrc = pthread_mutex_lock(&(pArray->mutex));
        assert(osrc == 0);

        if(pArray->count < pArray->capacity)
        {
            *(pArray->head) = *pEntry;
            added = true;
            pArray->head++;

            if(pArray->head >= &(pArray->entries[pArray->capacity]))
            {
                pArray->head = &(pArray->entries[0]);
            }

            if (pArray->count == 0)
            {
                pthread_cond_broadcast(&(pArray->cond));
            }
            pArray->count++;

            if (   !(pArray->warnTriggered)
                 && (pArray->count >= (pArray->origCapacity * HWM_FRACTION)))
            {
                pArray->warnTriggered = true;
                ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE] = StatusWarning;
            }
        }
        else
        {
            assert(pArray->tail == pArray->head);
            size_t newCapacity  = pArray->capacity * 2;
            callbackEntry_t  *newEntries = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,30),newCapacity * sizeof(callbackEntry_t));

            if (newEntries != NULL)
            {
                //We won't wrap around at the point in the entries we do at the current size, so we need to unwrap them
                size_t numEntriesAfterTail = ((void *)(pArray->entries + pArray->capacity)-(void *)(pArray->tail))/sizeof(callbackEntry_t);
                memcpy(newEntries, pArray->tail, sizeof(callbackEntry_t)*numEntriesAfterTail);
                memcpy(newEntries+numEntriesAfterTail, pArray->entries,  sizeof(callbackEntry_t)*(pArray->capacity - numEntriesAfterTail));
                ism_common_free(ism_memory_engine_misc,pArray->entries);
                pArray->entries = newEntries;
                pArray->capacity = newCapacity;
                pArray->tail = &(pArray->entries[0]);
                pArray->head = &(pArray->entries[pArray->count]);
            }
        }

        osrc = pthread_mutex_unlock(&(pArray->mutex));
        assert(osrc == 0);
    }
    while(!added);
}

static bool callbackArray_get(callbackArray_t *pArray, callbackEntry_t *pEntry)
{
    bool gotEntry = false;

    do
    {
        DEBUG_ONLY int osrc = pthread_mutex_lock(&(pArray->mutex));
        assert(osrc == 0);

        if (pArray->warnTriggered && pArray->count < (pArray->origCapacity * LWM_FRACTION))
        {
            ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE] = StatusOk;
        }

        while(callbacksPaused || (keepOnCallBacking && pArray->count == 0))
        {
            pthread_cond_wait(&(pArray->cond), &(pArray->mutex));
        }

        if (keepOnCallBacking && pArray->count >0)
        {
            *pEntry = *pArray->tail;
            gotEntry = true;

            pArray->tail++;

            if(pArray->tail >= &(pArray->entries[pArray->capacity]))
            {
                pArray->tail = &(pArray->entries[0]);
            }
            pArray->count--;
        }
        osrc = pthread_mutex_unlock(&(pArray->mutex));
        assert(osrc == 0);
    }
    while(!gotEntry && keepOnCallBacking);

    return gotEntry;
}

void fake_addCallback(callbackEntry_t *pEntry)
{
    callbackArray_add(cbs, pEntry);
}

static void *callBackThread(void *_context, void* _data, int _value)
{
    char tname[64];
    uint64_t threadNumber = __sync_add_and_fetch(&nextThreadId, 1);
    sprintf(tname, "FakeCBThread-%lu", threadNumber);
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    // Make sure we call ism_engine_threadInit with value >1 to indicate this is an
    // async callback thread.
    ism_engine_threadInit(((uint8_t)threadNumber)+1);

    while (keepOnCallBacking)
    {
        callbackEntry_t entry;

        if(callbackArray_get(cbs, &entry))
        {
            entry.pCallback(entry.rc, entry.context);
        }
    }

    ism_engine_threadTerm(1);
    ism_common_freeTLS();
    return NULL;
}


void initFakeCallBacks(size_t capacity)
{
    assert(cbs == NULL);
    cbs = callbackAray_create(capacity);
    assert(cbs != NULL);

    nextThreadId = 0;
    keepOnCallBacking = true;

    for (uint32_t i = 0; i < NUM_CALLBACK_THREADS; i++)
    {
        DEBUG_ONLY int osrc = ism_common_startThread( &(cbThreads[i]),callBackThread, NULL, NULL, 0,
                0, 0, "callBackThread", "callBackThread");
        assert(osrc == 0);
    }
}

void pauseFakeCallbacks(void)
{
    assert(cbs != NULL);

    DEBUG_ONLY int osrc = pthread_mutex_lock(&(cbs->mutex));
    assert(osrc == 0);

    callbacksPaused = true;

    pthread_cond_broadcast(&(cbs->cond));

    osrc = pthread_mutex_unlock(&(cbs->mutex));
    assert(osrc == 0);
}

void unpauseFakeCallbacks(void)
{
    assert(cbs != NULL);

    DEBUG_ONLY int osrc = pthread_mutex_lock(&(cbs->mutex));
    assert(osrc == 0);

    callbacksPaused = false;

    pthread_cond_broadcast(&(cbs->cond));

    osrc = pthread_mutex_unlock(&(cbs->mutex));
    assert(osrc == 0);
}

void stopFakeCallBacks(void)
{
    if (cbs != NULL)
    {
        DEBUG_ONLY int osrc = pthread_mutex_lock(&(cbs->mutex));
        assert(osrc == 0);

        keepOnCallBacking = false;
        callbacksPaused = false;

        pthread_cond_broadcast(&(cbs->cond));

        osrc = pthread_mutex_unlock(&(cbs->mutex));
        assert(osrc == 0);

        for (uint32_t i = 0; i < NUM_CALLBACK_THREADS; i++)
        {
            pthread_join(cbThreads[i], NULL);
        }

        callbackArray_free(cbs);
        cbs = NULL;
    }
}
#endif //USEFAKE_ASYNC_COMMIT
