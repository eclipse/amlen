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
#ifndef __ISM_TESTPUBSUBRESTART_H_DEFINED
#define __ISM_TESTPUBSUBRESTART_H_DEFINED

#include <stdint.h>
#include <stdbool.h>

#include "engine.h"

#define TPR_RESOURCE_SETID "tprres"

//Max string length in bytes is BUFSIZE-1
#define TEST_TOPICSTR_BUFSIZE 128
#define TEST_CLIENTID_BUFSIZE 64
#define TEST_SUBNAME_BUFSIZE (TEST_TOPICSTR_BUFSIZE+TEST_CLIENTID_BUFSIZE+1)
#define TEST_MAXINFLIGHT 32

typedef struct tag_testMessage_t
{
    char StructId[4]; ///< TMSG
    uint32_t pubberNum;
    uint32_t msgNum;
} testMessage_t;
#define TESTMSG_STRUCTID "TMSG"
#define TESTMSG_STRUCTID_ARRAY {'T', 'M', 'S', 'G'}


//When persisting data this is data used by the persistence layer
typedef struct tag_persistenceLayerPrivateData_t {
    ismEngine_QManagerRecordHandle_t persDataContainerHandle;  //When not using shared memory we have a qmgr record for each subber/pubber
    ismEngine_QMgrXidRecordHandle_t  persDataRecordHandle; //...and store the state in an XID record
} persistenceLayerPrivateData_t;

typedef struct tag_subInfoPerPublisher_t
{
    uint32_t msgExpected;
    uint32_t msgsArrived;
} subInfoPerPublisher_t;

typedef struct tag_subMsgInFlight_t
{
    uint32_t deliveryId;  ///< If /this/ field is 0, this entry is not in use
    testMessage_t msg;
} subMsgInFlight_t;


typedef struct tag_persistableSubscriberStateShm_t
{
    char StructId[4];///< PTSS
    uint32_t subId; //< Must be unique and in range 0-num subbers
    char clientId[TEST_CLIENTID_BUFSIZE];
    char subname[TEST_SUBNAME_BUFSIZE];
    char topicstring[TEST_TOPICSTR_BUFSIZE];
    subMsgInFlight_t msgsInFlight[TEST_MAXINFLIGHT];
    subInfoPerPublisher_t pubState[];
 } persistableSubscriberState_t;

#define PERSSUB_STRUCTID "PTSS"

typedef struct tag_subscriberStateComplete_t
{
    ismEngine_ClientStateHandle_t hClient; ///< client handle so we can destroy cleanly
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    persistableSubscriberState_t *persState; ///< Ptr to shared mem for persistent state
    bool recoveryComplete;
    persistenceLayerPrivateData_t persLayerData;
} subscriberStateComplete_t;

typedef enum tag_pubMsgInFlightState_t
{
  pMIFS_Publishing,
  pMIFS_Completing
} pubMsgInFlightState_t;

typedef struct tag_pubMsgInFlight_t
{
    uint32_t msgNum;
    pubMsgInFlightState_t state;
    ismEngine_UnreleasedHandle_t  hUnrel;
} pubMsgInFlight_t;

typedef struct tag_persistablePublisherState_t
{
    char StructId[4];///< TPPS
    char topicstring[TEST_TOPICSTR_BUFSIZE];
    char clientId[TEST_CLIENTID_BUFSIZE];
    uint32_t pubId; //< Must be unique and in range 0-num pubbers
    uint32_t msgsTotal;
    uint32_t msgsCompleted;
    pubMsgInFlight_t msgsInFlight[TEST_MAXINFLIGHT];
} persistablePublisherState_t;

#define PERSPUB_STRUCTID "TPPS"

typedef enum tag_recvdMsgStatus_t
{
    rmsFIRSTSEEN,
    rmsSEENBEFORE
} recvdMsgStatus_t;

typedef struct tag_queryDeliveryId_t
{
  uint32_t pubId; //< Must be unique and in range 0-num pubbers
  uint32_t                        numIds; ///< Ids we have been told about so far
  uint32_t                       *deliveryId;
  ismEngine_UnreleasedHandle_t   *hUnrel;
} queryDeliveryId_t;



/*Persistent State load/Save functions: testPubSubRestartPersistentState.c */
persistableSubscriberState_t *getFirstSubberPersState(void);
persistableSubscriberState_t *getNextSubberPersState(persistableSubscriberState_t *curr);
persistableSubscriberState_t *getSubberPersState(uint32_t subberNum);

persistablePublisherState_t *getFirstPubberPersState(void);
persistablePublisherState_t *getNextPubberPersState(persistablePublisherState_t *curr);

void persistSubberState(subscriberStateComplete_t *subber);

void persistPubberState( ismEngine_SessionHandle_t hSession
                       , persistenceLayerPrivateData_t *persLayerData
                       , persistablePublisherState_t *pubState);

void initialiseTestState( bool keepMemAfterExit
                        , uint64_t numPublishers
                        , uint64_t numSubscribers);

void restoreTestState( bool keepMemAfterExit
                     ,  uint64_t numPublishers
                     , uint64_t numSubscribers);

void useSharedMemoryForPersistence(bool shmAllowed);

void backupPersistedState(void);

#endif // __ISM_TESTPUBSUBRESTART_H_DEFINED
