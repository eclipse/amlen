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

//****************************************************************************
/// @file  expiringGetInternal.h
/// @brief Get a single message (without an external consumer) with a TimeOut
//****************************************************************************
#ifndef __ISM_EXPIRINGGETINTERNAL_DEFINED
#define __ISM_EXPIRINGGETINTERNAL_DEFINED

#include <stdint.h>
#include <stdbool.h>

#include "engineCommon.h"
#include "engineInternal.h"


//some enums used in debugging
typedef enum __attribute__ ((__packed__))
{
    iegiCANCELSTATE_NONE                = 0, ///< Not cancelled
    iegiCANCELSTATE_WHENFIRED           = 1, ///< Cancelled when the timer fired
    iegiCANCELSTATE_BYFUNC              = 2, ///< Cancelled by iegiCancelTimer
    iegiCANCELSTATE_CANCELFAILED        = 4, ///< Cancel didn't work
    iegiCANCELSTATE_ALREADYFINISHED     = 8, ///< Attempting to cancel when it was already finished
    iegiCANCELSTATE_NOKEY               = 16, ///< Attempting to cancel when no timerkey
    iegiCANCELSTATE_MSGARRIVDNOTIMER    = 32  ///< No Timer when the message arrived - no cancel
} iegiCancelState_t;

typedef enum __attribute__ ((__packed__))
{
    iegiFIREDSTATE_NONE           = 0, ///< Not Fired
    iegiFIREDSTATE_FIRED          = 1, ///< Fired
    iegiFIREDSTATE_TOOLATE        = 2, ///< When fired, we were no longer waiting
    iegiFIREDSTATE_NOKEY            = 4
} iegiFiredState_t;

typedef enum __attribute__ ((__packed__))
{
    iegiEVENTCOUNT_NONE                = 0, ///< Not Increase
    iegiEVENTCOUNT_INCREASED_CREATION  = 1,
    iegiEVENTCOUNT_DEC_FIRED           = 2, ///< Decreased when the timer fired
    iegiEVENTCOUNT_DEC_CANCELLED       = 4,
    iegiEVENTCOUNT_FREE_TIMER_STARTED  = 8, ///< Free is done by calling timer, so this is an increase
    iegiEVENTCOUNT_FREE_TIMER_FIRED    = 16
} iegiEventCountState_t;

typedef enum __attribute__ ((__packed__))
{
    iegiCONSSTATE_NONE                = 0, ///< Not Created
    iegiCONSSTATE_CREATED             = 1,
    iegiCONSSTATE_DESTROYEDASYNC      = 2,
    iegiCONSSTATE_DESTROYNOTIMER      = 4,
    iegiCONSSTATE_DESTROYTIMERFIRED   = 8,
    iegiCONSSTATE_DESTROYONCOMPLETE   = 16,
    iegiCONSSTATE_DESTROYCREATEFAILED = 32
} iegiConsState_t;

typedef struct tag_iegiExpiringGetInfo_t {
    char                       StrucId[4];            /* 'IEGI' */
    pthread_mutex_t            lock;
    ismEngine_SessionHandle_t  hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ism_timer_t                timerKey;
    uint64_t                   timeOutMillis;
    ieutThreadData_t          *pTimerFiredThread; //Allows us to check timer fired on
                                                  //same thread it was freed on

    bool             timerCreated;
    bool             consumerDestroyStarted;
    bool             messageDelivered;
    bool             completionCallbackFired;
    bool             doneCompletion;
    bool             recursivelyDestroyed; //Set if part of destroy of session/client
    bool             timerFinished; //Not usecount as timer will be cancelled before
                                    //consumer destroy completes

    /*** The following flags (altered under the lock) combine into a usecount,
     *   once all are set to true, this struct is finished with and should be freed by
     *   whoever sets the last flag to true
     */
    bool             constructionFinished;
    bool             consumerDestroyFinished;
    /* End usecount flags */

    //Some flags used in debugging
    iegiCancelState_t             timerCancelState;
    iegiFiredState_t              timerFiredState;
    iegiEventCountState_t         eventCountState;
    iegiConsState_t               consumerState;

    //Contexts and Callbacks
    void *                          pMessageContext;
    size_t                          messageContextLength;
    ismEngine_MessageCallback_t     pMessageCallbackFn;

    void *                          pCompletionContext;
    size_t                          completionContextLength;
    ismEngine_CompletionCallback_t  pCompletionCallbackFn;
} iegiExpiringGetInfo_t;

#define ismENGINE_EXPIRINGGETINFO_STRUCID "IEGI"

#endif
