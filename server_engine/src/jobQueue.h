/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
/// @file jobQueue.h
/// @brief API functions for using the jobQueues
///
/// Defines API functions for putting and getting to a
/// job queue
//*********************************************************************

#ifndef __ISM_ENGINE_JOBQUEUE_DEFINED
#define __ISM_ENGINE_JOBQUEUE_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "engineInternal.h"

int32_t iejq_createJobQueue(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t *pJQH);
void iejq_freeJobQueue(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh);


int32_t iejq_getJob( ieutThreadData_t *pThreadData
                   , iejqJobQueueHandle_t jqh
                   , void **pFunc
                   , void **pArgs
                   , bool takeLock);
void iejq_takeGetLock(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh);
void iejq_releaseGetLock(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh);
bool iejq_tryTakeGetLock(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh);
int32_t iejq_addJob( ieutThreadData_t *pThreadData
                   , iejqJobQueueHandle_t jqh
                   , void *func
                   , void *args
#ifdef IEJQ_JOBQUEUE_PUTLOCK
                   , bool takeLock);
#else
                   );
#endif

#ifdef IEJQ_JOBQUEUE_PUTLOCK
void iejq_takePutLock(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh);
void iejq_releasePutLock(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh);
#endif

bool iejq_ownerBlocked(iejqJobQueueHandle_t jqh, bool resetBlockedFlag);
void iejq_recordOwnerBlocked(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh);


//Can't actually use all jobs (1 reserved for separator)
#define IEJQ_JOB_MAX 32768


#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_JOBQUEUE_DEFINED */

/*********************************************************************/
/* End of intermediateQ.h                                            */
/*********************************************************************/
