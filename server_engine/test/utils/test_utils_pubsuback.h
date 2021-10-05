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

//****************************************************************************
/// @file  test_utils_pubsuback.h
///
/// @brief   simple wrappers that allow pubbing, subbing, acking of messages
///
//****************************************************************************

#include "engine.h"

typedef struct tag_testMsgsCounts_t {
    uint64_t msgsArrived;         //Total number of messages arrived (sum of states below)
    uint32_t msgsArrivedDlvd;     //How many messages arrived in a delivered state
    uint32_t msgsArrivedRcvd;     //How many messages arrived in a recvd state
    uint32_t msgsArrivedConsumed; //How many messages arrived in a consumed state
} testMsgsCounts_t;

typedef struct tag_testGetMsgContext_t {
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    volatile testMsgsCounts_t *pMsgCounts;
    uint32_t consumerOptions;
    bool overrideConsumerOpts;
    bool consumeMessages;
    volatile uint64_t acksStarted;
    volatile uint64_t acksCompleted;
} testGetMsgContext_t;

typedef struct tag_testMarkMsgsContext_t {
    ismEngine_SessionHandle_t hSession;
    ismEngine_DeliveryHandle_t *pNackHandles;
    ismEngine_DeliveryHandle_t *pCallerHandles;
    uint64_t numToSkipBeforeAcks;
    uint64_t numToMarkConsumed;
    uint64_t numToMarkRecv;
    uint64_t numToSkipBeforeStore; //skipped before handles stored to return to caller or nack
    uint64_t numToStoreForCaller;
    uint64_t numToStoreForNack;
    uint64_t totalNumToHaveDelivered;
    volatile uint64_t consStarted;
    volatile uint64_t consCompleted;
    volatile uint64_t recvStarted;
    volatile uint64_t recvCompleted;
    volatile uint64_t numNackHandlesStored;
    volatile uint64_t numCallerHandlesStored;
    volatile uint64_t numSkippedBeforeAcks;
    volatile uint64_t numSkippedBeforeStores; //skipped before handles stored to return to caller or nack
    volatile uint64_t numDelivered;
    volatile uint64_t unmarkableMsgs;
} testMarkMsgsContext_t;

bool test_markMessagesCallback(
        ismEngine_ConsumerHandle_t      hConsumer,
        ismEngine_DeliveryHandle_t      hDelivery,
        ismEngine_MessageHandle_t       hMessage,
        uint32_t                        deliveryId,
        ismMessageState_t               state,
        uint32_t                        destinationOptions,
        ismMessageHeader_t *            pMsgDetails,
        uint8_t                         areaCount,
        ismMessageAreaType_t            areaTypes[areaCount],
        size_t                          areaLengths[areaCount],
        void *                          pAreaData[areaCount],
        void *                          pConsumerContext);

void test_pubMessages( const char *topicString
                     , uint8_t persistence
                     , uint8_t reliability
                     , uint32_t numMsgs);

void test_connectAndGetMsgs( const char *clientId
                           , uint32_t clientOptions
                           , bool consumeMsgs
                           , bool overrideConsumeOptions
                           , uint32_t consumerOptions
                           , uint64_t waitForNumMsgs
                           , uint64_t waitForNumMilliSeconds
                           , testMsgsCounts_t *pMsgDetails);

void test_receive( testGetMsgContext_t *pContext
                 , const char *subName
                 , uint32_t subOptions
                 , uint64_t waitForNumMsgs
                 , uint64_t waitForNumMilliSeconds);

void test_connectReuseReceive( const char *clientId
                             , uint32_t clientOptions
                             , ismEngine_ClientState_t *hOwningClient
                             , const char *subName
                             , bool doReuse
                             , uint32_t subOptions
                             , uint32_t consumerOptions
                             , bool consumeMessages
                             , uint64_t waitForNumMsgs
                             , uint64_t waitForNumMilliSeconds
                             , testMsgsCounts_t *pMsgDetails);

void test_makeSub( const char *subName
                 , const char *topicString
                 , uint32_t subOptions
                 , ismEngine_ClientStateHandle_t hAttachingClient
                 , ismEngine_ClientStateHandle_t hOwningClient);

void test_markMsgs(
               char *subName
             , ismEngine_SessionHandle_t hSession
             , ismEngine_ClientState_t *hOwningClient
             , uint32_t numToHaveDelivered    //Total messages we have delivered before disabling consumer
             , uint32_t numToSkipBeforeAcks   //Of the delivered messages, how many do we skip before we start acking
             , uint32_t numToCons             //Of the delivered messages, how many to consume
             , uint32_t numToMarkRecv         //Of the delivered messages, how many to mark received
             , uint32_t numToSkipBeforeStore  //Of the delivered messages, after we've done acks how many
                                              //    do we skip before we store handles to give to caller or nack
             , uint32_t numToGiveToCaller     //Of the delivered messages, how many handles do we give to the caller unnacked
             , uint32_t numToNackNotRecv      //Of the delivered messages, how many to nack (not received) after delivery stops
             , uint32_t numToNackNotSent      //Of the delivered messages, how many to nack (not sent) after delivery stops
             , ismEngine_DeliveryHandle_t ackHandles[numToGiveToCaller]);

uint32_t test_getBatchSize(ismEngine_SessionHandle_t hSession);

