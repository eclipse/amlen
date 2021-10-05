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

/*********************************************************************/
/*                                                                   */
/* Module Name: testPubSubThreads.h                                  */
/*                                                                   */
/* Description: Handy functions for doing pub/sub at multi point in  */
/* the topic tree. For examples of use see testPubSubCfgs.c          */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_TESTPUBSUBTHREADS_H_DEFINED
#define __ISM_TESTPUBSUBTHREADS_H_DEFINED

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef enum tag_psgc_phase_t {initing, settingup, running, finished} psgc_phase_t;

#define TPT_RESOURCE_SET_ID "tstres"
#define PUTTER_CLIENTID_FORMAT    "d:"TPT_RESOURCE_SET_ID":putter_%s"
#define SUBBER_CLIENTID_FORMAT    "a:"TPT_RESOURCE_SET_ID":subber_%u"
#define BGSUBBER_CLIENTID_FORMAT  "a:"TPT_RESOURCE_SET_ID":bgsubber_%u"

typedef struct tag_psgroupcontroller {
    int threadsControlled;
    int bgThreads;
    psgc_phase_t phase;
    pthread_cond_t cond_setup; //used to signal group should setup
    pthread_mutex_t mutex_setup; //used to protect cond_setup
    int threadsReadySetup; //incremented for each thread ready to setup
    pthread_cond_t cond_start; //used to signal group should start
    pthread_mutex_t mutex_start; //used to protect cond_start
    int threadsReadyStart; //incremented for each thread ready to start
    pthread_cond_t cond_finish; //used to broadcast finish
    pthread_mutex_t mutex_finish; //used to protect cond
    int threadsFinished; //incremented for each thread finished
    char *monitorTopic;
} psgroupcontroller;

//Must call initControllerConds() on the controller before use.
#define PSGROUPCONTROLLERDEFAULT {0,                              \
                                  0,                              \
                                  initing,                        \
                                  PTHREAD_COND_INITIALIZER,       \
                                  PTHREAD_MUTEX_INITIALIZER,      \
                                  0,                              \
                                  PTHREAD_COND_INITIALIZER,       \
                                  PTHREAD_MUTEX_INITIALIZER,      \
                                  0,                              \
                                  PTHREAD_COND_INITIALIZER,       \
                                  PTHREAD_MUTEX_INITIALIZER,      \
                                  0,                              \
                                  NULL }

#define PSGROUPCONTROLLER_CONDCLOCK CLOCK_MONOTONIC

// Should QoS1 select ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE (no, yes or randomly)
typedef enum tag_transactionCapableQoS1 {
    TxnCapable_False,
    TxnCapable_True,
    TxnCapable_Random,
}transactionCapableQoS1;

typedef struct tag_psgroupinfo {
    char *topicString;
    int32_t numPubbers;
    int64_t msgsPerPubber;
    int32_t pubSprayWidth;
    uint8_t pubMsgFlags;
    uint8_t pubMsgPersistence;
    int32_t numSubbers;
    int64_t msgsPerSubber;
    int32_t numBgSubThreads;
    int32_t bgSubberDelay;
    psgroupcontroller *controller;
    int64_t groupMsgsSent;
    int64_t groupMsgsRcvd;
    int64_t groupBgSubbersCreated;
    int64_t groupBgMsgsRcvd;
    int32_t groupRetCode;
    bool    subsWait;         //Should subs wait after they receive all there messages to check for extra msgs
    bool    cpuPin;           //Should publishers in this group be pinned to a cpu
    transactionCapableQoS1 txnCapable; // Should QoS1  select ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
    int8_t  QoS;
} psgroupinfo;

#define PSGROUPDEFAULT {"",                              \
                        0,0,0,0,0,0,0,0,0,               \
                        NULL,                            \
                        0,0,0,0,                         \
                        0,                               \
                        false, false, TxnCapable_False,  \
                        0}

//Initialise the contoller cond vars before use with either simple or complex API
void initControllerConds(psgroupcontroller *pGController);

//Simple API
int32_t doPubs(char *topicstring,
               int numpublishers,
               int msgsperpubber,
               int numsubscribers,
               bool subsWait,
               bool cpupin,
               bool memTest,
               transactionCapableQoS1 transactionCapable,
               int qos);

//More complex API
int32_t InitPubSubThreadGroup(psgroupinfo *psginfo);
int TriggerSetupThreadGroups(psgroupcontroller *gcontroller,
                             ism_time_t *setupTime,
                             bool memTest);
int32_t StartThreadGroups(psgroupcontroller *pgc,
                          ism_time_t *startTime);
int32_t WaitForGroupsFinish(psgroupcontroller *pgc,
                            ism_time_t *finishTime,
                            int numGroups,
                            psgroupinfo *groups[]);
void FreeGroups(psgroupinfo *groups, int groupCount);
void startMemoryTest(void);
void stopMemoryTest(void);

#endif /* __ISM_TESTPUBSUBTHREADS_H_DEFINED */

/*********************************************************************/
/* End of testPubSubThreads.h                                        */
/*********************************************************************/
