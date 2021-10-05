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

//*********************************************************************
/// @file  ackList.h
/// @brief Header for management of message acknowledgement
//*********************************************************************
#ifndef ISM_ENGINE_ACKLIST_H_DEFINED
#define ISM_ENGINE_ACKLIST_H_DEFINED
#include <stdint.h>

struct ismEngine_Consumer_t; //forward declaration...really in engineInternal.h
struct tag_iemqQNode_t; //forward declaration...really in multiConsumerQ.h

typedef struct tag_iealAckEntry_t {
    struct ismEngine_Consumer_t *pConsumer;
    struct tag_iemqQNode_t *pPrev;
    struct tag_iemqQNode_t *pNext;
} iealAckDetails_t;

void ieal_addUnackedMessage( ieutThreadData_t *pThreadData
                           , ismEngine_Consumer_t *pConsumer
                           , struct tag_iemqQNode_t *qnode);

void ieal_removeUnackedMessage( ieutThreadData_t *pThreadData
                              , ismEngine_Session_t *pSession
                              , struct tag_iemqQNode_t *qnode
                              , ismEngine_Consumer_t **ppConsumer);

int32_t ieal_nackOutstandingMessages( ieutThreadData_t *pThreadData
                                    , ismEngine_Session_t *pSession);

void ieal_debugAckList( ieutThreadData_t *pThreadData
                      , ismEngine_Session_t *pSession);

#endif //ISM_ENGINE_ACKLIST_H_DEFINED
