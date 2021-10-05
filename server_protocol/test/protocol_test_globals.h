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
#ifndef PROTOCOL_TEST_GLOBALS_H_
#define PROTOCOL_TEST_GLOBALS_H_

#include <engine.h>

//defined in protocol_test.c
extern int          g_entered_send_cb;
extern int          g_entered_close_cb;
extern int          g_entered_closed_cb;
extern int          g_entered_createSession;
extern int          g_entered_createConsumer;
extern int          g_entered_assocMsg_cb;
extern int          g_entered_createMsg;
extern int          g_entered_putMsg;
extern int          g_entered_releaseMsg;
extern int          g_entered_destroyConsumer;
extern int          g_entered_createClient;
extern int          g_entered_destroyClient;
extern int          g_entered_confirmDelivery;
extern int          g_entered_authenticate_user;
extern int          g_entered_authorize_user;
extern const char * g_cmd;
extern int          g_cmd_result;
extern char         g_retbuf [2048];
extern int          g_retlen;
extern int          g_ret_cmd;
extern int          g_qos;
extern uint8_t      g_authentication_fails;
extern uint8_t      g_authorization_fails;
extern int          g_entered_cancel_timer;
extern int          g_entered_startMessageDelivery;
extern int          g_entered_stopMessageDelivery;
extern int          g_entered_destroySession;
extern int          g_entered_createLocalTransaction;
extern int          g_entered_destroyProducer;
extern int          g_entered_putMessageWithDeliveryId;

extern int          g_allowdurable;
extern int          g_allowpersistentmessages;

typedef int32_t (*genericEngineAPI_CB_t)(
    const char *functionName,
    void *context,
    size_t contextLength,
    void *parm1,
    void *parm2,
    void **retValue);

extern genericEngineAPI_CB_t g_genericEngineAPI_CB;

typedef int32_t (*ism_engine_putMessageOnDestinationAPI_CB_t)(
    ismEngine_SessionHandle_t hSession,
    ismDestinationType_t destinationType,
    const char *pDestinationName,
    ismEngine_TransactionHandle_t hTran,
    ismEngine_MessageHandle_t pMessage,
    void * pContext,
    size_t contextLength,
    ismEngine_CompletionCallback_t cb);

extern ism_engine_putMessageOnDestinationAPI_CB_t g_ism_engine_putMessageOnDestinationAPI_CB;

typedef int32_t (*ism_engine_createMessageAPI_CB_t)(
    ismMessageHeader_t *pHeader,
    uint8_t areaCount,
    ismMessageAreaType_t areaTypes[areaCount],
    size_t areaLengths[areaCount],
    void * pAreaData[areaCount],
    ismEngine_MessageHandle_t * phMessage);

extern ism_engine_createMessageAPI_CB_t g_ism_engine_createMessageAPI_CB;

typedef int32_t (*ism_engine_getRetainedMessageAPI_CB_t)(
    ismEngine_SessionHandle_t       hSession,
    const char *                    topicString,
    void *                          pMessageContext,
    size_t                          messageContextLength,
    ismEngine_MessageCallback_t     pMessageCallbackFn,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

extern ism_engine_getRetainedMessageAPI_CB_t g_ism_engine_getRetainedMessageAPI_CB;

#endif /* PROTOCOL_TEST_GLOBALS_H_ */
