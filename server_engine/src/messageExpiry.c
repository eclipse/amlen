/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
/// @file  messageExpiry.c
/// @brief Functions for tracking and remove messages with expiry set
//****************************************************************************
#define TRACE_COMP Engine

#include <sys/time.h>


#include "engineInternal.h"
#include "engineUtils.h"
#include "messageExpiry.h"
#include "messageExpiryInternal.h"
#include "memHandler.h"
#include "engineSplitList.h"
#include "engineMonitoring.h"
#include "remoteServers.h"     // iers_ functions and constants

void *ieme_reaperThread(void *arg, void * context, int value);

//****************************************************************************
/// @brief Setup the locks/conds for waking up the expiry reaper thread(s)
///
/// @param[in]     pThreadData    Thread data to use
/// @param[in]     expiryControl  Expiry Control information
///
/// @return void
//****************************************************************************
static inline void ieme_initExpiryReaperWakeupMechanism( ieutThreadData_t *pThreadData
                                                       , iemeExpiryControl_t *expiryControl)
{
    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);
    assert (expiryControl != NULL);

    pthread_condattr_t attr;

    int os_rc = pthread_condattr_init(&attr);

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "pthread_condattr_init failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_002, true, "pthread_condattr_setclock failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_cond_init(&(expiryControl->cond_wakeup), &attr);

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_003, true, "pthread_cond_init failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_condattr_destroy(&attr);


    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_004, true, "pthread_condattr_destroy failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_mutex_init(&(expiryControl->mutex_wakeup), NULL);

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_005, true, "pthread_mutex_init failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

static inline void ieme_lockExpiryWakeupMutex(iemeExpiryControl_t *expiryControl)
{
    int os_rc = pthread_mutex_lock(&(expiryControl->mutex_wakeup));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "pthread_mutex_lock failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

}

static inline void ieme_unlockExpiryWakeupMutex(iemeExpiryControl_t *expiryControl)
{
    int os_rc = pthread_mutex_unlock(&(expiryControl->mutex_wakeup));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "pthread_mutex_unlock failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

}

//****************************************************************************
/// @brief Sleep for specified number of seconds unless woken up
///
/// @param[in]     pThreadData    Thread data to use
/// @param[in]     secs           Num seconds we intend to sleep
/// @param[inout]  numWakeups     On input: number of wakeups last time this
///                                         function was called
///                               On output: current number of wakeups
///
/// @return void
//****************************************************************************
void ieme_expiryReaperSleep( ieutThreadData_t *pThreadData
                           , uint32_t secs
                           , uint64_t *numWakeups)
{
    struct timespec now;
    struct timespec waituntil;


    ieutTRACEL(pThreadData, secs, ENGINE_FNC_TRACE, FUNCTION_ENTRY "secs: %u wakeups: %lu\n",
               __func__, secs, *numWakeups);

    iemeExpiryControl_t *expiryControl = ismEngine_serverGlobal.msgExpiryControl;
    assert (expiryControl != NULL);

    int os_rc = clock_gettime(CLOCK_MONOTONIC, &now);

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "gettimeofday failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "currtime", &now, sizeof(now)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    waituntil = now;
    waituntil.tv_sec  += secs;

    ieme_lockExpiryWakeupMutex(expiryControl);

    if (    (*numWakeups == expiryControl->numWakeups)
         && (expiryControl->reaperEndRequested == false))
    {
        os_rc = pthread_cond_timedwait( &(expiryControl->cond_wakeup)
                                      , &(expiryControl->mutex_wakeup)
                                      , &waituntil);

        if (UNLIKELY((os_rc != 0) && (os_rc != ETIMEDOUT)))
        {
            ieutTRACE_FFDC( ieutPROBE_001, true, "timedwait failed!", ISMRC_Error
                          , "expiryControl", expiryControl, sizeof(*expiryControl)
                          , "now", &now, sizeof(now)
                          , "waituntil", &waituntil, sizeof(waituntil)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }
    }

    *numWakeups = expiryControl->numWakeups;

    ieme_unlockExpiryWakeupMutex(expiryControl);

    ieutTRACEL(pThreadData, expiryControl->numWakeups, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief Cause the expiry reaper to start a scan of queues and topics
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return void
//****************************************************************************
void ieme_wakeMessageExpiryReaper(ieutThreadData_t *pThreadData)
{
    iemeExpiryControl_t *expiryControl = ismEngine_serverGlobal.msgExpiryControl;
    assert (expiryControl != NULL);

    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    //Broadcast that the reaper should wake up
    ieme_lockExpiryWakeupMutex(expiryControl);

    expiryControl->numWakeups++;

    int os_rc = pthread_cond_broadcast(&(expiryControl->cond_wakeup));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "broadcast failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }
    ieme_unlockExpiryWakeupMutex(expiryControl);

    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief destroy wake-up mechanism for expiry reaper
///
/// @param[in]     pThreadData      Thread data to use
/// @param[in]     expiryControl    Expiry Control structure
///
/// @return void
//****************************************************************************
static inline void ieme_destroyExpiryReaperWakeupMechanism( ieutThreadData_t *pThreadData
                                                          , iemeExpiryControl_t *expiryControl)
{
    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);
    assert(expiryControl != NULL);

    int os_rc = pthread_cond_destroy(&(expiryControl->cond_wakeup));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "cond_destroy!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_mutex_destroy(&(expiryControl->mutex_wakeup));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_002, true, "mutex_destroy!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }
    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}


//****************************************************************************
/// @brief Create and initialize the structures needed to perform at-rest
///        message expiry.
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieme_initMessageExpiry( ieutThreadData_t *pThreadData )
{
    int32_t rc = OK;
    iemeExpiryControl_t *expiryControl = ismEngine_serverGlobal.msgExpiryControl;

    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (expiryControl != NULL) goto mod_exit;

    expiryControl = iemem_calloc(pThreadData,
                                 IEMEM_PROBE(iemem_messageExpiryData, 1),
                                 1, sizeof(iemeExpiryControl_t));

    if (expiryControl == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    //Allow us to be woken-up
    ieme_initExpiryReaperWakeupMechanism(pThreadData, expiryControl);

    // Initialise the queue reaper list
    rc = ieut_createSplitList(pThreadData,
                              offsetof(ismEngine_Queue_t, expiryLink),
                              iemem_messageExpiryData,
                              &expiryControl->queueReaperList);

    if (rc != OK) goto mod_exit;

    // Initialize the topic reaper list
    rc = ieut_createSplitList(pThreadData,
                              offsetof(iettTopicNode_t, expiryLink),
                              iemem_messageExpiryData,
                              &expiryControl->topicReaperList);

    if (rc != OK) goto mod_exit;

mod_exit:

    ismEngine_serverGlobal.msgExpiryControl = expiryControl;

    if (rc != OK)
    {
        ieme_destroyMessageExpiry(pThreadData);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Start the thread(s) to perform at-rest message expiry
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieme_startMessageExpiry( ieutThreadData_t *pThreadData )
{
    int32_t rc = OK;

    iemeExpiryControl_t *expiryControl = ismEngine_serverGlobal.msgExpiryControl;

    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(expiryControl != NULL);
    assert(expiryControl->reaperThreadHandle == 0);
    assert(expiryControl->reaperEndRequested == false);

    int startRc = ism_common_startThread(&expiryControl->reaperThreadHandle,
                                         ieme_reaperThread,
                                         NULL, expiryControl, 0, // Pass expiryControl as context
                                         ISM_TUSAGE_NORMAL,
                                         0,
                                         "msgReaper",
                                         "Remove_Expired_Messages");

    if (startRc != 0)
    {
       ieutTRACEL(pThreadData, startRc, ENGINE_ERROR_TRACE, "ism_common_startThread for msgReaper failed with %d\n", startRc);
       rc = ISMRC_Error;
       ism_common_setError(rc);
       goto mod_exit;
    }

    assert(expiryControl->reaperThreadHandle != 0);

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Stop the threads performing message expiry
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void ieme_stopMessageExpiry( ieutThreadData_t *pThreadData )
{
    iemeExpiryControl_t *expiryControl = ismEngine_serverGlobal.msgExpiryControl;

    ieutTRACEL(pThreadData, expiryControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (expiryControl != NULL && expiryControl->reaperThreadHandle != 0)
    {
        void *retVal = NULL;

        // Request the reaper thread to end, and wait for it to do so
        expiryControl->reaperEndRequested = true;

        ieme_wakeMessageExpiryReaper(pThreadData);

        // Wait for the thread to actually end
        ieut_waitForThread(pThreadData,
                           expiryControl->reaperThreadHandle,
                           &retVal,
                           iettMAXIMUM_SHUTDOWN_TIMEOUT_SECONDS);

        // The reaper thread doesn't return anything but if it starts to
        // we ought to do something with it!
        assert(retVal == NULL);

        expiryControl->reaperThreadHandle = 0;
    }

    ieutTRACEL(pThreadData, expiryControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Clean up the expiry control information including lists of topics
///        and queues.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void ieme_destroyMessageExpiry( ieutThreadData_t *pThreadData )
{
    iemeExpiryControl_t *expiryControl = ismEngine_serverGlobal.msgExpiryControl;

    ieutTRACEL(pThreadData, expiryControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (expiryControl != NULL)
    {
        assert(expiryControl->reaperThreadHandle == 0);

        if (expiryControl->queueReaperList != NULL)
        {
            ieut_destroySplitList(pThreadData, expiryControl->queueReaperList);
        }

        if (expiryControl->topicReaperList != NULL)
        {
            ieut_destroySplitList(pThreadData, expiryControl->topicReaperList);
        }

        ieme_destroyExpiryReaperWakeupMechanism(pThreadData, expiryControl);

        iemem_free(pThreadData, iemem_messageExpiryData, expiryControl);

        ismEngine_serverGlobal.msgExpiryControl = NULL;
    }

    ieutTRACEL(pThreadData, expiryControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

static inline bool ieme_checkQExpiryDataExists( ieutThreadData_t *pThreadData
                                              , ismEngine_Queue_t *pQ)
{
    bool exists = false;

    while (pQ->QExpiryData == NULL)
    {
        iemeQueueExpiryData_t *newExpiryData = iemem_calloc(pThreadData,
                                                            IEMEM_PROBE(iemem_messageExpiryData, 2), 1,
                                                            sizeof(iemeQueueExpiryData_t));

        if (newExpiryData == NULL)
        {
            goto mod_exit;
        }

        int os_rc = pthread_mutex_init(&(newExpiryData->expiryLock), NULL);
        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_001, true, "pthread_mutexattr_init failed!", ISMRC_Error
                          , "Q", pQ, sizeof(*pQ)
                          , "newExpiryData", newExpiryData, sizeof(*newExpiryData)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }


        //try and CAS in our newly allocated expiry data area.
        bool CASworked = __sync_bool_compare_and_swap (&(pQ->QExpiryData), NULL, newExpiryData);

        if (!CASworked)
        {
            //Someone appears to have beaten us to it...
            os_rc = pthread_mutex_destroy(&(newExpiryData->expiryLock));

            if (UNLIKELY(os_rc != 0))
            {
                ieutTRACE_FFDC( ieutPROBE_002, true, "destroy expirylock failed!", ISMRC_Error
                        , "Q", pQ, sizeof(*pQ)
                        , "newExpiryData", newExpiryData, sizeof(*newExpiryData)
                        , "os_rc", &os_rc, sizeof(os_rc)
                        , NULL);
            }

            iemem_free(pThreadData, iemem_messageExpiryData, newExpiryData);
        }
    }

    //If we got here, we broke out of the loop, because the area now exists
    exists=true;

 mod_exit:
    return exists;
}

static inline void ieme_takeQExpiryLock( ismEngine_Queue_t *pQ
                                       , iemeQueueExpiryData_t *QExpiryData)
{
    int os_rc = pthread_mutex_lock(&(QExpiryData->expiryLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Taking expirylock failed.", ISMRC_Error
                      , "Queue", pQ, sizeof(ismEngine_Queue_t)
                      , "ExpiryData", QExpiryData, sizeof(*QExpiryData)
                      , NULL);
    }
}

static inline bool ieme_tryQExpiryLock( ismEngine_Queue_t *pQ
                                      , iemeQueueExpiryData_t *QExpiryData)
{
    bool gotlock = true;

    int os_rc = pthread_mutex_trylock(&(QExpiryData->expiryLock));

    if (os_rc == EBUSY)
    {
        gotlock = false;
    }
    else if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Try taking expirylock failed.", ISMRC_Error
                      , "Queue", pQ, sizeof(ismEngine_Queue_t)
                      , "ExpiryData", QExpiryData, sizeof(*QExpiryData)
                      , NULL);
    }

    return gotlock;
}

static inline void ieme_releaseQExpiryLock( ismEngine_Queue_t *pQ
                                          , iemeQueueExpiryData_t *QExpiryData)
{
    int os_rc = pthread_mutex_unlock(&(QExpiryData->expiryLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Releasing expirylock failed.", ISMRC_Error
                      , "Queue", pQ, sizeof(ismEngine_Queue_t)
                      , "ExpiryData", QExpiryData, sizeof(*QExpiryData)
                      , NULL);
    }
}

static inline void ieme_addQueueToExpiryReaperList( ieutThreadData_t *pThreadData
                                                  , ismEngine_Queue_t *pQ)
{
    assert(ismEngine_serverGlobal.msgExpiryControl != NULL);

    ieut_addObjectToSplitList(ismEngine_serverGlobal.msgExpiryControl->queueReaperList, pQ);
}

void ieme_removeQueueFromExpiryReaperList( ieutThreadData_t *pThreadData
                                         , ismEngine_Queue_t *pQ)
{
    assert(ismEngine_serverGlobal.msgExpiryControl != NULL);

    ieut_removeObjectFromSplitList(ismEngine_serverGlobal.msgExpiryControl->queueReaperList, pQ);
}

void ieme_addTopicToExpiryReaperList( ieutThreadData_t *pThreadData
                                    , iettTopicNode_t *pTopicNode)
{
    assert(ismEngine_serverGlobal.msgExpiryControl != NULL);

    ieut_addObjectToSplitList(ismEngine_serverGlobal.msgExpiryControl->topicReaperList, pTopicNode);
}

void ieme_removeTopicFromExpiryReaperList( ieutThreadData_t *pThreadData
                                         , iettTopicNode_t *pTopicNode)
{
    assert(ismEngine_serverGlobal.msgExpiryControl != NULL);

    ieut_removeObjectFromSplitList(ismEngine_serverGlobal.msgExpiryControl->topicReaperList, pTopicNode);
}

//Called as part of ieme_replaceQExpiryDataStart/End or from a standalone ieme_addMessageExpiryData
//
//assumes queue data has been created and we have the expiry lock
//
static inline void ieme_addMessageExpiryDataInternal( ieutThreadData_t *pThreadData
                                                    , ismEngine_Queue_t *pQ
                                                    , iemeBufferedMsgExpiryDetails_t *msgdata
                                                    , bool alreadyInExpiryList)
{
    iemeQueueExpiryData_t *QExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;
    iemeBufferedMsgExpiryDetails_t *earliestExpiryMessages
                                                    = QExpiryData->earliestExpiryMessages;

    bool insertedMsg = false;
    uint32_t i=0;

    //Adjust the global count
    pThreadData->stats.bufferedExpiryMsgCount++;

    //Update the per-queue array of earliest to expiry messages
    for (i=0; i < QExpiryData->messagesInArray; i++)
    {
        if (msgdata->Expiry < earliestExpiryMessages[i].Expiry)
        {
            if ( i < NUM_EARLIEST_MESSAGES - 1)
            {
                //We are inserting, with space for more entries after us, copy them down
                uint32_t lastCopyIdx = (QExpiryData->messagesInArray < NUM_EARLIEST_MESSAGES)
                                           ? QExpiryData->messagesInArray - 1
                                           : QExpiryData->messagesInArray - 2;

                uint32_t numToCopy = 1 + (lastCopyIdx - i);

                memmove( &(earliestExpiryMessages[i+1])
                       , &(earliestExpiryMessages[i])
                       , numToCopy * sizeof(iemeBufferedMsgExpiryDetails_t));
            }

            earliestExpiryMessages[i] = *msgdata;

            if (QExpiryData->messagesInArray < NUM_EARLIEST_MESSAGES)
            {
                QExpiryData->messagesInArray++;
            }
            insertedMsg = true;
            break;
        }
    }

    //If we haven't inserted the message and there is space and we know position of all
    //relevant messages are in earlier slots, then we can record expirydata in this slot.
    //(There are other messages with expiry on this queue that aren't in this array,
    //We don't know how the expiry of this message compares with those, we can't add
    //it to the array)
    if (    !insertedMsg
         && (QExpiryData->messagesInArray < NUM_EARLIEST_MESSAGES)
         && (i == QExpiryData->messagesWithExpiry))
    {
        earliestExpiryMessages[i] = *msgdata;
        QExpiryData->messagesInArray++;
        insertedMsg = true;
    }

    //Increase the per-queue count and if necessary add this queue to global list
    if (   (QExpiryData->messagesWithExpiry == 0)
        && (!alreadyInExpiryList))
    {
        ieme_addQueueToExpiryReaperList(pThreadData, pQ);
    }
    QExpiryData->messagesWithExpiry++;
}

//Called as part of ieme_replaceQExpiryDataStart/End or from a standalone ieme_removeMessageExpiryData
//
//assumes queue data has been created and we have the expiry lock
//
static inline void ieme_removeMessageExpiryDataInternal( ieutThreadData_t *pThreadData
                                                       , ismEngine_Queue_t *pQ
                                                       , uint64_t orderId)
{
    iemeQueueExpiryData_t *QExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;
    iemeBufferedMsgExpiryDetails_t *earliestExpiryMessages
                                                    = QExpiryData->earliestExpiryMessages;

    //Adjust the global count
    pThreadData->stats.bufferedExpiryMsgCount--;

    //Adjust the per-queue array of messages expiring soon
    for (uint32_t i=0; i < QExpiryData->messagesInArray; i++)
    {
        if (orderId == earliestExpiryMessages[i].orderId)
        {
            //Need to zero this entry out... we'll copy all the entries up by one and then
            //zero last entry
            uint32_t numEntriesToCopy = (QExpiryData->messagesInArray - i - 1);

            if (numEntriesToCopy > 0)
            {
                memmove( &(earliestExpiryMessages[i])
                       , &(earliestExpiryMessages[i+1])
                       , numEntriesToCopy * sizeof(iemeBufferedMsgExpiryDetails_t));
            }
            QExpiryData->messagesInArray--;

            earliestExpiryMessages[QExpiryData->messagesInArray].orderId = 0;
            earliestExpiryMessages[QExpiryData->messagesInArray].Expiry  = 0;
            earliestExpiryMessages[QExpiryData->messagesInArray].qnode   = NULL;
        }
    }

    //Decrease the per-queue count and if necessary remove this queue from global list
    if (QExpiryData->messagesWithExpiry == 1)
    {
        ieme_removeQueueFromExpiryReaperList(pThreadData, pQ);
    }
    QExpiryData->messagesWithExpiry--;
}

void ieme_addMessageExpiryData( ieutThreadData_t *pThreadData
                              , ismEngine_Queue_t *pQ
                              , iemeBufferedMsgExpiryDetails_t *msgdata)
{
    bool queueDataExists = ieme_checkQExpiryDataExists( pThreadData
                                                      , pQ);

    if (queueDataExists)
    {
        iemeQueueExpiryData_t *QExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;

        ieme_takeQExpiryLock(pQ, QExpiryData);

        ieme_addMessageExpiryDataInternal( pThreadData
                                         , pQ
                                         , msgdata
                                         , false);

        ieme_releaseQExpiryLock(pQ, QExpiryData);
    }
    else
    {
        //Can't allocate enough memory for queue data, see if we can get enough
        //to add to the reaper queue list to fix up later
        ieme_addQueueToExpiryReaperList(pThreadData, pQ);
    }
}

void ieme_addMessageExpiryPreLocked( ieutThreadData_t *pThreadData
                                   , ismEngine_Queue_t *pQ
                                   , iemeBufferedMsgExpiryDetails_t *msgdata
                                   , bool alreadyInExpiryList)
{
    ieme_addMessageExpiryDataInternal( pThreadData
                                     , pQ
                                     , msgdata
                                     , alreadyInExpiryList );
}

void ieme_removeMessageExpiryData( ieutThreadData_t *pThreadData
                                 , ismEngine_Queue_t *pQ
                                 , uint64_t orderId)
{
    bool queueDataExists = ieme_checkQExpiryDataExists( pThreadData
                                                      , pQ);

    if (queueDataExists)
    {
        iemeQueueExpiryData_t *QExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;

        ieme_takeQExpiryLock(pQ, QExpiryData);

        ieme_removeMessageExpiryDataInternal( pThreadData
                                            , pQ
                                            , orderId);

        ieme_releaseQExpiryLock(pQ, QExpiryData);
    }
}

void ieme_freeQExpiryData( ieutThreadData_t *pThreadData
                         , ismEngine_Queue_t *pQ )
{
    if (pQ->QExpiryData != NULL)
    {
        iemeQueueExpiryData_t *pQExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;

        ieme_takeQExpiryLock(pQ, pQExpiryData);

        if (pQExpiryData->messagesWithExpiry > 0)
        {
            ieme_removeQueueFromExpiryReaperList(pThreadData, pQ);

            pThreadData->stats.bufferedExpiryMsgCount -= pQExpiryData->messagesWithExpiry;
        }
        ieme_releaseQExpiryLock(pQ, pQExpiryData);

        int os_rc = pthread_mutex_destroy(&(pQExpiryData->expiryLock));
        if (UNLIKELY(os_rc != OK))
        {
            ieutTRACE_FFDC( ieutPROBE_006, true, "destroy expirylock failed!", ISMRC_Error
                          , "pQ", pQ, sizeof(*pQ)
                          , "pQExpiryData", pQExpiryData, sizeof(*pQExpiryData)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }
        iemem_free(pThreadData, iemem_messageExpiryData, pQExpiryData);

        pQ->QExpiryData = NULL;
    }
    else
    {
        //It's possible that we didn't have enough memory to allocate it,
        //but added it to the list to try again later... check by trying to remove it:
        ieme_removeQueueFromExpiryReaperList(pThreadData, pQ);
    }
}

bool ieme_startReaperQExpiryScan( ieutThreadData_t *pThreadData
                                , ismEngine_Queue_t *pQ)
{
    iemeQueueExpiryData_t *pQExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;

    bool gotLock = ieme_tryQExpiryLock(pQ, pQExpiryData);

    return gotLock;
}

void ieme_endReaperQExpiryScan( ieutThreadData_t *pThreadData
                              , ismEngine_Queue_t *pQ)
{
    iemeQueueExpiryData_t *pQExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;
    ieme_releaseQExpiryLock(pQ, pQExpiryData);
}

//Called between start & end QExpirySCan (so we have the expirylock)
void ieme_clearQExpiryDataPreLocked( ieutThreadData_t *pThreadData
                                   , ismEngine_Queue_t *pQ)
{
    iemeQueueExpiryData_t *QExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;
    pThreadData->stats.bufferedExpiryMsgCount -= QExpiryData->messagesWithExpiry;
    QExpiryData->messagesInArray    = 0;
    QExpiryData->messagesWithExpiry = 0;

    memset(QExpiryData->earliestExpiryMessages, 0,
                NUM_EARLIEST_MESSAGES*sizeof(iemeBufferedMsgExpiryDetails_t));
}

void ieme_reapQExpiredMessages( ieutThreadData_t *pThreadData
                              , ismEngine_Queue_t *pQ)
{
    if (pQ->QExpiryData != NULL)
    {
#if TRACETIMESTAMP_EXPIRY
    uint64_t TTS_Start_Expiry = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_Expiry);
#endif
       iemeQueueExpiryData_t *pQExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;
       uint32_t nowExpire = ism_common_nowExpire();

       //dirty read the expirydata without taking the lock, if we get it wrong, we'll
       //skip the queue on this scan, not a problem
       if (    (pQExpiryData->messagesWithExpiry > 0)
            && (   pQExpiryData->messagesInArray == 0
                || pQExpiryData->earliestExpiryMessages[0].Expiry < nowExpire))
       {
           ieq_reapExpiredMsgs(pThreadData, pQ, nowExpire, false, false);
       }
#if TRACETIMESTAMP_EXPIRY
    uint64_t TTS_Stop_Expiry = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_Expiry);
#endif
    }
}

ieutSplitListCallbackAction_t ieme_reapQExpiredMessagesCB( ieutThreadData_t *pThreadData
                                                         , void *object
                                                         , void *context)
{
    ismEngine_Queue_t *pQ = (ismEngine_Queue_t *)object;
    ieutSplitListCallbackAction_t rc = ieutSPLIT_LIST_CALLBACK_CONTINUE;
    uint32_t nowExpire;
    iemeExpiryReaperQContext_t *QContext = (iemeExpiryReaperQContext_t *)context;
    bool needScan  = false;
    bool forceFull = false;
    iemeQueueExpiryData_t *pQExpiryData = NULL;

    ieutTRACEL(pThreadData, pQ,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pQ=%p\n", __func__, pQ);

    // If the reaper is being ended, stop scanning
    if (*(QContext->reaperEndRequested) == true)
    {
        rc = ieutSPLIT_LIST_CALLBACK_STOP;
        goto mod_exit;
    }

    // Update the time we use to check expiry
    if ((QContext->callbackCount & 0x1F) == 0)
    {
        QContext->nowExpire = ism_common_nowExpire();
    }
    QContext->callbackCount += 1;

    nowExpire = QContext->nowExpire;


    if (pQ->QExpiryData != NULL)
    {
        pQExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;

        //dirty read the expirydata without taking the lock, if we get it wrong, we'll
        //skip the queue on this scan, not a problem
        if (    (pQExpiryData->messagesWithExpiry > 0)
             && (   pQExpiryData->messagesInArray == 0
                 || pQExpiryData->earliestExpiryMessages[0].Expiry < nowExpire))
        {
            needScan = true;
        }
        else
        {
            // Remember the earliest expiring message we didn't expire
            if (pQExpiryData->messagesInArray > 0)
            {
                uint32_t earliestExpiryTime = pQExpiryData->earliestExpiryMessages[0].Expiry;

                if (earliestExpiryTime < QContext->earliestObservedExpiry)
                {
                    QContext->earliestObservedExpiry = earliestExpiryTime;
                }
            }

            QContext->statQNoWorkRequired++;
        }

    }
    else
    {
        //We presumably were unable to allocate it, try now
        bool queueDataExists = ieme_checkQExpiryDataExists( pThreadData
                                                          , pQ);

        if (queueDataExists)
        {
            //It exists now...do full scan
            pQExpiryData = (iemeQueueExpiryData_t *)pQ->QExpiryData;
            needScan  = true;
            forceFull = true;
        }
        else
        {
            //If it doesn't exist, we leave it and will try again next time
            QContext->statQNoMem++;
        }
    }

    if (needScan)
    {
        ieqExpiryReapRC_t reaprc = ieq_reapExpiredMsgs(pThreadData, pQ, nowExpire, forceFull, true);

        if (reaprc == ieqExpiryReapRC_NoExpiryLock)
        {
            if (QContext != NULL)
            {
                QContext->statQNoLock++;
            }
        }
        else if (reaprc == ieqExpiryReapRC_RemoveQ)
        {
            rc = ieutSPLIT_LIST_CALLBACK_REMOVE_OBJECT;
        }

    }

    // Remember the earliest expiring message we didn't expire
    if (pQExpiryData && pQExpiryData->messagesInArray > 0)
    {
        uint32_t earliestExpiryTime = pQExpiryData->earliestExpiryMessages[0].Expiry;

        if (earliestExpiryTime < QContext->earliestObservedExpiry)
        {
            QContext->earliestObservedExpiry = earliestExpiryTime;
        }
    }

 mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

void *ieme_reaperThread(void *arg, void *context, int value)
{
    char threadName[16];
    ism_common_getThreadName(threadName, sizeof(threadName));

    iemeExpiryControl_t *expiryControl = (iemeExpiryControl_t *)context;

    // Make sure we're thread-initialised.
    ism_engine_threadInit(0); // TODO: Should we use isStoreCrit or not? We are NOT at the moment

    // Not working on behalf of a particular client
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    ieutTRACEL(pThreadData, expiryControl, ENGINE_CEI_TRACE, FUNCTION_ENTRY "Started thread %s with control %p\n",
               __func__, threadName, expiryControl);

    uint64_t numWakeups = 0;

    uint32_t lastTopicScanTime = 0;
    uint32_t lastQueueScanTime = 0;

    while(expiryControl->reaperEndRequested == false)
    {
        uint32_t nowTime = ism_common_nowExpire();

        iemnMessagingStatistics_t beforeStats;

        iemn_getMessagingStatistics(pThreadData, &beforeStats);

        expiryControl->scansStarted += 1;

        // Reap the topics for expired retained messages...
        iemeExpiryReaperTopicContext_t topicContext = {0};
        topicContext.earliestObservedExpiry = UINT32_MAX;

        // If we have expiring retained messages, or it has been 5 minutes since
        // the last topic scan, scan now.
        if (   (beforeStats.RetainedMessagesWithExpirySet > 0)
            || ((nowTime - lastTopicScanTime) > 300))
        {
            topicContext.remoteServerMemLimit = iers_queryRemoteServerMemLimit(pThreadData);
            topicContext.reaperEndRequested = &expiryControl->reaperEndRequested;

            ieut_traverseSplitList(pThreadData,
                                   expiryControl->topicReaperList,
                                   iett_reapTopicExpiredMessagesCB,
                                   &topicContext);

            iett_finishReapTopicExpiredMessages(pThreadData, &topicContext);

            if (expiryControl->reaperEndRequested) break;

            lastTopicScanTime = nowTime;
        }

        // Reap the queues for expired buffered messages...
        iemeExpiryReaperQContext_t queueContext = {0};
        queueContext.earliestObservedExpiry = UINT32_MAX;

        // If we have expiring buffered messages, or it has been 5 minutes since
        // the last queue scan, scan now.
        if (   (beforeStats.BufferedMessagesWithExpirySet > 0)
            || ((nowTime - lastQueueScanTime) > 300))
        {
            queueContext.reaperEndRequested = &expiryControl->reaperEndRequested;

            ieut_traverseSplitList(pThreadData,
                                   expiryControl->queueReaperList,
                                   ieme_reapQExpiredMessagesCB,
                                   &queueContext);

            if (expiryControl->reaperEndRequested) break;

            lastQueueScanTime = nowTime;
        }

        uint32_t sleepSeconds;

        // If we didn't do anything this time around, wait for 10 seconds and look again
        if (lastTopicScanTime != nowTime && lastQueueScanTime != nowTime)
        {
            sleepSeconds = 10;
        }
        // We did a scan, so we have more information to go on...
        //
        // If we saw a retained or buffered message due to expire, set the sleep time
        // to that interval.
        //
        // If the sleep time is less than 5 seconds or more than 30 seconds, we instead
        // sleep for 5 seconds or 30 seconds, respectively.
        else
        {
            sleepSeconds = UINT32_MAX;

            nowTime = ism_common_nowExpire();

            // There is a retained message due to expire sooner than current time
            if ((topicContext.earliestObservedExpiry < nowTime))
            {
                // Try again in 5 seconds
                sleepSeconds = 5;
            }
            // There is a retained message due to expire sooner than current sleep time
            else if ((topicContext.earliestObservedExpiry - nowTime) < sleepSeconds)
            {
                sleepSeconds = topicContext.earliestObservedExpiry - nowTime;
            }

            // There is a buffered message due to expire sooner than current time
            if ((queueContext.earliestObservedExpiry < nowTime))
            {
                // Try again in 5 seconds
                sleepSeconds = 5;
            }
            // There is a buffered message due to expire sooner than current sleep time
            else if ((queueContext.earliestObservedExpiry - nowTime) < sleepSeconds)
            {
                sleepSeconds = queueContext.earliestObservedExpiry - nowTime;
            }

            // Don't sleep for less than 5 seconds
            if (sleepSeconds < 5)
            {
                sleepSeconds = 5;
            }
            // Don't sleep for more than 30 seconds
            else if (sleepSeconds > 30)
            {
                sleepSeconds = 30;
            }
        }

        expiryControl->scansEnded += 1;

        // Ensure that anything waiting for this thread to free memory doesn't wait now
        ieut_leavingEngine(pThreadData);
        ieme_expiryReaperSleep( pThreadData, sleepSeconds, &numWakeups );
        ieut_enteringEngine(NULL);
    }

    ieutTRACEL(pThreadData, expiryControl, ENGINE_CEI_TRACE, FUNCTION_EXIT "Ending thread %s with control %p\n",
               __func__, threadName, expiryControl);
    ieut_leavingEngine(pThreadData);

    // No longer need the thread to be initialized
    ism_engine_threadTerm(1);

    return NULL;
}
