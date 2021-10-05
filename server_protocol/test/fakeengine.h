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

/**
 * Perform engine functions required for JSON testing
 */
XAPI int ism_transport_registerProtocol(ism_transport_onStartMessaging_t onStart, ism_transport_onConnection_t onConnection);

XAPI int32_t ism_engine_createClientState(
    const char *                    pClientId,
    uint32_t                        protocolId,
    uint32_t                        options,
    void *                          pStealContext,
    ismEngine_StealCallback_t       pStealCallbackFn,
    ismSecurity_t *                 pSecContext,
    ismEngine_ClientStateHandle_t * phClient,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

XAPI int32_t ism_engine_putMessageOnDestination(ismEngine_SessionHandle_t hSession, ismDestinationType_t destinationType, const char *pDestinationName, ismEngine_TransactionHandle_t hTran, ismEngine_MessageHandle_t pMessage, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb);

XAPI int32_t ism_engine_putMessageWithDeliveryIdOnDestination(ismEngine_SessionHandle_t hSession, ismDestinationType_t destinationType, const char *pDestinationName, ismEngine_TransactionHandle_t hTran, ismEngine_MessageHandle_t pMessage, uint32_t unrelDeliveryId, ismEngine_UnreleasedHandle_t *  phUnrel, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb);

XAPI int32_t ism_engine_createConsumer(ismEngine_SessionHandle_t hSession, ismDestinationType_t destinationType, const char *pDestinationName, const ismEngine_SubscriptionAttributes_t *pSubAttributes, ismEngine_ClientStateHandle_t hOwningClient, void * pMessageContext, size_t messageContextLength, ismEngine_MessageCallback_t pMessageCallbackFn, const ism_prop_t * pConsumerProperties, uint32_t consumerOptions, ismEngine_ConsumerHandle_t * phConsumer, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb);

XAPI int32_t ism_engine_destroyConsumer(ismEngine_ConsumerHandle_t hConsumer, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb);

XAPI int32_t ism_engine_startMessageDelivery(ismEngine_SessionHandle_t hSession, uint32_t options, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb);

XAPI int32_t ism_engine_createMessage(ismMessageHeader_t *pHeader, uint8_t areaCount, ismMessageAreaType_t areaTypes[areaCount], size_t areaLengths[areaCount], void * pAreadData[areaCount], ismEngine_MessageHandle_t * phMessage);

XAPI int32_t ism_engine_createSession(ismEngine_ClientStateHandle_t hClient, uint32_t options, ismEngine_SessionHandle_t * phSession, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb);

XAPI int32_t ism_engine_destroySession(ismEngine_SessionHandle_t hSession, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb);

XAPI void ism_engine_releaseMessage(ismEngine_MessageHandle_t pMessage);

XAPI int32_t ism_engine_destroyClientState(ismEngine_ClientStateHandle_t hClient, uint32_t options, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb);

XAPI int32_t ism_engine_setWillMessage(
        ismEngine_ClientStateHandle_t   hClient,
        ismDestinationType_t            destinationType,
        const char *                    pDestinationName,
        ismEngine_MessageHandle_t       hMessage,
        uint32_t                        delayInterval,
        uint32_t                        timeToLive,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn);

XAPI int32_t ism_engine_unsetWillMessage(
        ismEngine_ClientStateHandle_t   hClient,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn);

XAPI int32_t ism_engine_addUnreleasedDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

XAPI int32_t ism_engine_removeUnreleasedDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_UnreleasedHandle_t    hUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

XAPI int32_t ism_engine_createSubscription(
    ismEngine_ClientStateHandle_t              hRequestingClient,
    const char *                               pSubName,
    const ism_prop_t *                         pSubProperties,
    uint8_t                                    destinationType,
    const char *                               pDestinationName,
    const ismEngine_SubscriptionAttributes_t * pSubAttributes,
    ismEngine_ClientStateHandle_t              hOwningClient,
    void *                                     pContext,
    size_t                                     contextLength,
    ismEngine_CompletionCallback_t             pCallbackFn);

XAPI int32_t ism_engine_destroySubscription(
    ismEngine_ClientStateHandle_t    hRequestingClient,
    const char *                     pSubName,
    ismEngine_ClientStateHandle_t    hOwningClient,
    void *                           pContext,
    size_t                           contextLength,
    ismEngine_CompletionCallback_t   pCallbackFn);

XAPI int32_t ism_engine_listSubscriptions(
    ismEngine_ClientStateHandle_t    hOwningClient,
    const char *                     pSubName,
    void *                           pContext,
    ismEngine_SubscriptionCallback_t pCallbackFn);

XAPI int32_t ism_engine_confirmMessageDelivery(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_DeliveryHandle_t      hDelivery,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

XAPI int32_t ism_engine_stopMessageDelivery(
    ismEngine_SessionHandle_t       hSession,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

XAPI int32_t ism_engine_setMessageDeliveryId(
    ismEngine_ConsumerHandle_t      hConsumer,
    ismEngine_DeliveryHandle_t      hDelivery,
    uint32_t                        deliveryId,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

XAPI int32_t ism_security_authenticate_user(ismSecurity_t *sContext, const char *username, int username_len, const char *password, int password_len);
XAPI void ism_security_authenticate_user_async(ismSecurity_t *sContext,const char *username, int username_len,
												const char *password, int password_len, 
												void *pContext,int pContext_size,
                                                ismSecurity_AuthenticationCallback_t pCallbackFn, int authnRequired,ism_time_t delay);

XAPI int32_t ism_security_validate_policy(ismSecurity_t *secContext,
		ismSecurityAuthObjectType_t objectType,
		const char * objectName,
		ismSecurityAuthActionType_t actionType,
		ism_ConfigComponentType_t compType,
		void **               context);

XAPI int32_t ism_engine_createLocalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t * phTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

XAPI int ism_security_context_getAllowDurable(ismSecurity_t *sContext);

XAPI int ism_security_context_getAllowPersistentMessages(ismSecurity_t *sContext);
