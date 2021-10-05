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
/// @file  clientStateExpiry.h
/// @brief Engine component header file with internals of tracking
///        expiry of client states.
//*********************************************************************
#ifndef __ISM_CLIENTSTATEEXPIRY_DEFINED
#define __ISM_CLIENTSTATEEXPIRY_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include "engineCommon.h"      /* Engine common internal header file */
#include "engineInternal.h"    /* Engine internal header file        */
#include "engineSplitList.h"   /* Engine Split list handling funcs   */
#include "remoteServers.h"     /* iers_ functions and constants      */
#include "queueCommon.h"
#include "topicTree.h"

/*********************************************************************/
/*                                                                   */
/* TYPE DEFINITIONS                                                  */
/*                                                                   */
/*********************************************************************/

typedef struct tag_ieceExpiryControl_t
{
    ism_threadh_t    reaperThreadHandle;  ///< The thread handle of the reaper thread
    volatile bool    reaperEndRequested;  ///< Whether the reaper thread has been asked to end
    pthread_cond_t   cond_wakeup;         ///< Used to wake up the reaper
    pthread_mutex_t  mutex_wakeup;        ///< Used to protect cond_wakeup
    ism_time_t       nextScheduledScan;   ///< When the next scheduled scan will be
    uint64_t         numWakeups;          ///< Count of times reaper has been woken
    uint64_t         scansStarted;        ///< Count of the number of scans that have been started
    uint64_t         scansEnded;          ///< Count of the number of scans that have ended
} ieceExpiryControl_t;

// Amount of time we're willing to wait for timers at shutdown.
#define ieceMAXIMUM_SHUTDOWN_TIMEOUT_SECONDS 60

/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/

//****************************************************************************
/// @brief Create and initialize the structures needed to perform clientState
///        expiry.
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iece_initClientStateExpiry( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief Start the thread(s) to perform clientState expiry
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iece_startClientStateExpiry( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief Stop the threads performing clientState expiry
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void iece_stopClientStateExpiry( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief End the threads and clean up.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void iece_destroyClientStateExpiry( ieutThreadData_t *pThreadData );

//Cause expiry reaper to scan clientStates
void iece_wakeClientStateExpiryReaper(ieutThreadData_t *pThreadData);

//Cause a re-assesment of the scheduled expiry scan for this time
void iece_checkTimeWithScheduledScan(ieutThreadData_t *pThreadData, ism_time_t time);

#endif /* __ISM_CLIENTSTATEEXPIRY_DEFINED */
