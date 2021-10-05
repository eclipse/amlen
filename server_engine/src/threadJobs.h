/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

//*********************************************************************
/// @file  threadJobs.h
/// @brief Engine component header file with internals of per-thread
///        job processing.
//*********************************************************************
#ifndef __ISM_THREADJOBS_DEFINED
#define __ISM_THREADJOBS_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include "jobQueue.h"

/*********************************************************************/
/*                                                                   */
/* TYPE DEFINITIONS                                                  */
/*                                                                   */
/*********************************************************************/

typedef struct tag_ietjScavengerEntry
{
    ieutThreadData_t *pThreadData;
    bool removalRequested;
    iejqJobQueueHandle_t jobQueue;
    uint64_t scavengedCount;
    uint64_t lastUpdated;
    uint64_t lastEntryCount;
    uint64_t lastProcessedJobs;
    uint64_t lastOwnerBlockedTime; //Last time owner tried to drain the queue but scavenger was doing it
    uint64_t scavengerWaitDelay; //How long scavenger thread waits in hope thread owner will drain the queue
} ietjScavengerEntry;

typedef struct tag_ietjThreadJobControl_t
{
    pthread_mutex_t scavengerListLock;   ///< Lock protecting scavenger list
    ietjScavengerEntry *scavengerList;
    uint32_t scavengerListCount;
    uint32_t scavengerListCapacity;
    ism_threadh_t scavengerThreadHandle; ///< Thread handle of the scavenger
    bool scavengerEndRequested;          ///< Set when the scavenger is asked to stop
} ietjThreadJobControl_t;

typedef void (*ietjCallback_t)(ieutThreadData_t *pThreadData,
                               void *pContext);

// NOTE: There was a popping sound when these numbers were coded... There was no research
// involved... If they need to be changed, change them...

// How long the scavenger will wait before processing a thread's job queue... if it gets in
// the way of the owner draining it, then the scavenger will wait longer next time

#define ietjSCAVENGER_THREAD_JOB_QUEUE_MIN_IDLE_NANOS    500000   // 0.5 millisecond(s)
#define ietjSCAVENGER_THREAD_JOB_QUEUE_MAX_IDLE_NANOS  50000000   // 50 millisecond(s)

//Each time the scavenger block the thread owner, it waits this much extra
#define IETJ_SCAVENGERWAITTIME_INCREMENT_NANOS           500000   // 0.5 millisecond(s)


//How many jobs the scavenger will do for a thread before it will move on and look
//at next job queue
#define ietjSCAVENGER_THREAD_JOB_BATCH_SIZE  4

//If the scavenger finds the thread owner is not picking up work, it waits less time in future,
//but only if it hasn't got in the way of the thread owner "recently" - this defines what recent means in nanos
#define ietjSCAVENGER_THREAD_JOB_FORGET_OWNER_BLOCK_TIME 10000000000   // 10 seconds


//If no engine threads have come in and the scavenger has done no work in a "while"
//then the scavenger sleeps "a bit longer"... So define  "a while" and "a bit longer":
#define ietjSCAVENGER_THREAD_INACTIVITY_THRESHOLD_NANOS   2000000000   // 2 seconds
#define ietjSCAVENGER_THREAD_INACTIVITY_LONG_SLEEP         100000000   // 100 millisecond(s)

//
//Numbers for the old thread jobs code that only kicks in above a certain number of clients:
//
// How long the scavenger will wait before processing a thread's job queue
#define ietjSCAVENGER_THREAD_JOB_QUEUE_IDLE_NANOS 50000000  // 50 millisecond(s)
// How long the scavenger thread will delay before looping
#define ietjSCAVENGER_THREAD_LOOP_DELAY_NANOS     2000000   // 2 millisecond(s)

// End of numbers for old thread Jobs code

// Amount of time we're willing to wait for timers at shutdown.
#define ietjMAXIMUM_SHUTDOWN_TIMEOUT_SECONDS 60

/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/

//****************************************************************************
/// @brief Create and initialize the structures needed to handle per-thread
///        job queues.
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ietj_initThreadJobs( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief Start the per-thread job queue scavenger
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ietj_startThreadJobScavenger( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief Stop the per-thread job queue scavenger
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void ietj_stopThreadJobScavenger( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief End the per-thread job queue scavenger and clean up.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void ietj_destroyThreadJobs( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief Add a jobQueue to the thread, and add it to the scavenger list.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
int32_t ietj_addThreadJobQueue( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief Request the removal of the thread's job queue, the actual removal
/// will happen when it can safely be removed.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void ietj_removeThreadJobQueue( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief Process the job queue for the specified thread
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return Whether any jobs were found
//****************************************************************************
bool ietj_processJobQueue(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief Update the threadData for a job callback
///
/// @param[in]     pThreadData      Thread data to update
/// @param[in]     pClient          The client to use to set trace level (may
///                                 be NULL).
//****************************************************************************
static inline void ietj_updateThreadData(ieutThreadData_t *pThreadData,
                                         ismEngine_ClientState_t *pClient)
{
    assert(pThreadData->callDepth >= 1);

    pThreadData->componentTrcLevel = ism_security_context_getTrcLevel(pClient ? pClient->pSecContext : NULL)->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)];
    pThreadData->memUpdateCount = ismEngine_serverGlobal.memUpdateCount;
}
#endif /* __ISM_CLIENTSTATEEXPIRY_DEFINED */
