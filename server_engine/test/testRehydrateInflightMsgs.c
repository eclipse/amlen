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
/********************************************************************/
/*                                                                  */
/* Module Name: testRehydrateInflightMsgs                           */
/*                                                                  */
/* Description: This test attempts to create a subscription that    */
/*              has message references in various states and checks */
/*              they come back after restart ok                     */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"
#include "engineCommon.h"
#include "policyInfo.h"
#include "queueCommon.h"
#include "topicTree.h"
#include "multiConsumerQInternal.h"
#include "lockManager.h"

#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_log.h"
#include "test_utils_task.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

/********************************************************************/
/* Function prototypes                                              */
/********************************************************************/
int ProcessArgs( int argc
               , char **argv
               , int *phase
               , char **padminDir );

/********************************************************************/
/* Global data                                                      */
/********************************************************************/
static uint32_t logLevel = testLOGLEVEL_CHECK;

#define testClientId "RehydrateInflightTest"

#define createPhase          0 // Creation phase
#define RecoverPhase         1 // End phase


typedef struct tag_consumerContext_t {
    ismEngine_SessionHandle_t hSession;
    ismEngine_TransactionHandle_t hLocalTran;
    ismEngine_TransactionHandle_t hGlobalTran;
    volatile uint64_t messagesCompleted;
}consumerContext_t ;


void finishedAck(int32_t rc, void *handle, void *context)
{
    uint64_t *pMsgsCompleted = *(uint64_t **)context;

    if(pMsgsCompleted != NULL)
    {
        __sync_fetch_and_add(pMsgsCompleted, 1);
    }
}

bool consumerCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
                              void *                          pContext)
{
    consumerContext_t *context = *((consumerContext_t **)pContext);
    bool wantMoreMessages = true;
    uint32_t confirmOptions = ismENGINE_CONFIRM_OPTION_CONSUMED;
    bool doAck = true;
    volatile uint64_t *pMsgsCompleted = &(context->messagesCompleted);

    ismEngine_TransactionHandle_t hTranToUse = NULL;

    assert(ismENGINE_NULL_DELIVERY_HANDLE != hDelivery);

    switch(pMsgDetails->OrderId)
    {
        case 1:
        case 5:
            if(pMsgDetails->RedeliveryCount < 4)
            {
                confirmOptions = ismENGINE_CONFIRM_OPTION_NOT_RECEIVED;
                pMsgsCompleted = NULL; //Not finished with this message
            }
            else
            {
                doAck = false; // Now we've increased the delivery count - leave it unacked
            }
            break;

        case 2:
        case 6:
            hTranToUse = context->hLocalTran;
            break;

        case 3:
        case 7:
            hTranToUse = context->hGlobalTran;
            break;

        case 4:
        case 8:
            doAck = false;
            wantMoreMessages = false;
            break;

        default:
            TEST_ASSERT(false, ("Unexpected oId: %lu", pMsgDetails->OrderId));
            break;
    }


    if (doAck)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        hTranToUse,
                                                        hDelivery,
                                                        confirmOptions,
                                                        &pMsgsCompleted, sizeof(pMsgsCompleted), finishedAck);

        TEST_ASSERT((rc == OK || rc == ISMRC_AsyncCompletion), ("%d != %d", rc, OK));

        if (rc == OK)
        {
            finishedAck(rc, NULL, &pMsgsCompleted);
        }
    }
    else
    {
        __sync_fetch_and_add(&(context->messagesCompleted), 1);
    }

    ism_engine_releaseMessage(hMessage);

    return wantMoreMessages;
}

bool isNodeLocked(iemqQueue_t *Q,
                  iemqQNode_t *node)
{
    ielmLockName_t LockName = { .Msg = {
            ielmLOCK_TYPE_MESSAGE, Q->qId, node->orderId
        }
    };

    int32_t rc = ielm_instantLockWithPeek(&LockName, NULL,
                                          NULL);
    assert((rc == OK) || (rc == ISMRC_LockNotGranted));

    return (rc == ISMRC_LockNotGranted);
}

void publishMessages( ismEngine_SessionHandle_t hSession
                    , char *topicString
                    , uint32_t msgsToSend)
{
    uint32_t totalMsgs = msgsToSend;

    // Publish messages to the sub
    uint32_t *pMsgsToSend = &msgsToSend;
    int32_t rc;

    for(int32_t i=0; i<totalMsgs; i++)
    {
        ismEngine_MessageHandle_t hMsg;

        rc = test_createMessage(100,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMsg, NULL);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

        rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    topicString,
                                                    NULL,
                                                    hMsg,
                                                    &pMsgsToSend, sizeof(pMsgsToSend), test_decrementActionsRemaining);

        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("rc was  %d", rc));

        if (rc == OK)
        {
            test_decrementActionsRemaining(rc, NULL, &pMsgsToSend);
        }
    }
    test_waitForRemainingActions(pMsgsToSend);
}

void createSubAndGetMessages( ismEngine_ClientStateHandle_t hClient
                            , ismEngine_SessionHandle_t hSession
                            , char *subName
                            , ism_xid_t *pXID)
{
    char *topicString = "/test/rehydrateInflight";
    int32_t rc;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_SHARED };

    rc = sync_ism_engine_createSubscription( hClient
                                           , subName
                                           , NULL
                                           , ismDESTINATION_TOPIC
                                           , topicString
                                           , &subAttrs
                                           , NULL ); // Owning client same as requesting client
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));


    //Create the local and global trans we need in this test
    ismEngine_TransactionHandle_t hLocalTran = NULL;
    ismEngine_TransactionHandle_t hGlobalTran = NULL;

    rc = sync_ism_engine_createLocalTransaction(hSession, &hLocalTran);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));


    rc = sync_ism_engine_createGlobalTransaction( hSession
                                                , pXID
                                                , ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS
                                                , &hGlobalTran);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    //Send first message...
    publishMessages(hSession, topicString, 1);

    //Create a consumer and consume the first message
    ismEngine_ConsumerHandle_t hConsumer1;

    consumerContext_t consContext   = {hSession, hLocalTran, hGlobalTran, 0};
    consumerContext_t *pConsContext = &consContext;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   subName,
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &pConsContext,
                                   sizeof(pConsContext),
                                   consumerCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
    TEST_ASSERT(hConsumer1 != NULL, ("hConsumer1 is NULL"));

    //We nacked the first message... repeatedly but eventually leave it unnacked...
    test_waitForMessages64(&(consContext.messagesCompleted), NULL, 1, 10);
    
    //Deliver another 3 messages and check they are consumed..
    publishMessages(hSession, topicString, 3);
    test_waitForMessages64(&(consContext.messagesCompleted), NULL, 4, 10);

    //Now do it again with a consumer that does persist delivery counts

    publishMessages(hSession, topicString, 1);
    consContext.messagesCompleted = 0;
    ismEngine_ConsumerHandle_t hConsumer2;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   subName,
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &pConsContext,
                                   sizeof(pConsContext),
                                   consumerCallback,
                                   NULL,
                                     ismENGINE_CONSUMER_OPTION_ACK
                                   | ismENGINE_CONSUMER_OPTION_NONPERSISTENT_DELIVERYCOUNTING,
                                   &hConsumer2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
    TEST_ASSERT(hConsumer2 != NULL, ("hConsumer2 is NULL"));

    //We nacked the first message...repeatedly but eventually leave it unnacked...
    test_waitForMessages64(&(consContext.messagesCompleted), NULL, 1, 10);

    //Deliver another 3 messages and check they are consumed..
    publishMessages(hSession, topicString, 3);
    test_waitForMessages64(&(consContext.messagesCompleted), NULL, 4, 10);

    //Prepare the global tran
    rc = ism_engine_endTransaction(hSession,
                                   hGlobalTran,
                                   ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                   NULL, 0, NULL);
    TEST_ASSERT(rc == OK,("%d != %d", rc, OK));


    rc = sync_ism_engine_prepareGlobalTransaction(hSession, pXID);
    TEST_ASSERT(rc == OK,("%d != %d", rc, OK));

    //Ready for restart...
}

void checkRecoveredMessages(  ismEngine_ClientStateHandle_t hClient
                            , ismEngine_SessionHandle_t hSession
                            , char *subName
                            , ism_xid_t *pXID)
{
    int32_t rc;

    //Get the queueHandle...
    ismEngine_Subscription_t *pSub = NULL;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    rc = iett_findClientSubscription(pThreadData,
                                     testClientId,
                                     iett_generateClientIdHash(testClientId),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &pSub);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
    iemqQueue_t *Q = (iemqQueue_t *)(pSub->queueHandle);

    int os_rc = pthread_rwlock_rdlock(&(Q->headlock));
    TEST_ASSERT(os_rc == 0, ("os_rc was %d", os_rc));

    os_rc = pthread_mutex_lock(&(Q->getlock));
    TEST_ASSERT(os_rc == 0, ("os_rc was %d", os_rc));

    iemqQNodePage_t *curPage = Q->headPage;
    iemqQNode_t *pnode = &(curPage->nodes[0]);
    uint64_t expectedOId = 1;

    while(expectedOId <= 8)
    {
        TEST_ASSERT(pnode->orderId == expectedOId,
                    ("Expected oId %lu, got %lu",
                      expectedOId, pnode->orderId ));

        bool nodeLocked = isNodeLocked(Q, pnode);

        TEST_ASSERT(pnode->msg != NULL,
                     ("Node %lu had no msg"));

        switch (pnode->orderId)
        {
            case 1:
                TEST_ASSERT(!nodeLocked, ("node oId 1 was locked!"));
                TEST_ASSERT(pnode->msgState ==  ismMESSAGE_STATE_AVAILABLE,
                            ("node oId 1 had msgState %u (expected %u)",
                                    pnode->msgState, ismMESSAGE_STATE_AVAILABLE));
                TEST_ASSERT(pnode->deliveryCount == 5,
                              ("node 1 was delivered %u times, expected 5", pnode->deliveryCount));
                break;

            case 2:
            case 4:
            case 6:
                TEST_ASSERT(!nodeLocked, ("node oId %lu was locked!", pnode->orderId));
                TEST_ASSERT(pnode->msgState ==  ismMESSAGE_STATE_AVAILABLE,
                            ("node oId %lu had msgState %u (expected %u)",
                                       pnode->orderId, pnode->msgState, ismMESSAGE_STATE_AVAILABLE));
                TEST_ASSERT(pnode->deliveryCount == 1,
                              ("node %lu was delivered %u times, expected 1",
                                      pnode->orderId, pnode->deliveryCount));
                break;

            case 3:
            case 7:
                TEST_ASSERT(nodeLocked, ("node oId %lu was NOT locked!", pnode->orderId));
                TEST_ASSERT(pnode->deliveryCount == 1,
                              ("node %lu was delivered %u times, expected 1",
                                      pnode->orderId, pnode->deliveryCount));
                break;

            case 5:
            case 8:
                TEST_ASSERT(!nodeLocked, ("node oId %lu was locked!", pnode->orderId));
                TEST_ASSERT(pnode->msgState ==  ismMESSAGE_STATE_AVAILABLE,
                            ("node oId %lu had msgState %u (expected %u)",
                                       pnode->orderId, pnode->msgState, ismMESSAGE_STATE_AVAILABLE));
                TEST_ASSERT(pnode->deliveryCount == 0,
                              ("node %lu was delivered %u times, expected 0",
                                      pnode->orderId, pnode->deliveryCount));
                break;

            default:
                TEST_ASSERT(false, ("Unexpected node oId %lu", pnode->orderId));
                break;
        }

        pnode++;
        if (pnode->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            curPage = curPage->next;
            pnode = &(curPage->nodes[0]);
        }
        expectedOId++;
    }

    //Find and commit transaction....
    ism_xid_t XIDArray[5] = {{0}};

    uint32_t found = ism_engine_XARecover(hSession,
                                          XIDArray,
                                          sizeof(XIDArray)/sizeof(XIDArray[0]),
                                          0,
                                          ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS);
    TEST_ASSERT(found ==  1, ("On restart, found %lu XA transactions, expected 1"));

    // Check that the transaction can now be committed
    rc = sync_ism_engine_commitGlobalTransaction(hSession,
                                                 &XIDArray[0],
                                                 ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS);
    TEST_ASSERT(rc == ISMRC_OK, ("rc was %d", rc));


    curPage = Q->headPage;
    pnode = &(curPage->nodes[0]);
    expectedOId = 1;

       while(expectedOId <= 8)
       {
           TEST_ASSERT(pnode->orderId == expectedOId,
                       ("Expected oId %lu, got %lu",
                         expectedOId, pnode->orderId ));

           bool nodeLocked = isNodeLocked(Q, pnode);

           switch (pnode->orderId)
           {
               case 1:
               case 2:
               case 4:
               case 5:
               case 6:
               case 8:
                   //Already checked these
                   break;

               case 3:
               case 7:
                   TEST_ASSERT(!nodeLocked, ("node oId %lu was locked!", pnode->orderId));
                   TEST_ASSERT(pnode->msgState ==  ismMESSAGE_STATE_CONSUMED,
                               ("node oId %lu had msgState %u (expected %u)",
                                          pnode->orderId, pnode->msgState, ismMESSAGE_STATE_CONSUMED));
                   TEST_ASSERT(pnode->deliveryCount == 1,
                                 ("node %lu was delivered %u times, expected 1",
                                         pnode->orderId, pnode->deliveryCount));
                   break;

               default:
                   TEST_ASSERT(false, ("Unexpected node oId %lu", pnode->orderId));
                   break;
           }

           pnode++;
           if (pnode->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
           {
               curPage = curPage->next;
               pnode = &(curPage->nodes[0]);
           }
           expectedOId++;
       }

    os_rc = pthread_mutex_unlock(&(Q->getlock));
    TEST_ASSERT(os_rc == 0, ("os_rc was %d", os_rc));

    os_rc = pthread_rwlock_unlock(&(Q->headlock));
    TEST_ASSERT(os_rc == 0, ("os_rc was %d", os_rc));

    rc = iett_releaseSubscription(pThreadData, pSub, false);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    // Now destroy the subscription
    rc = ism_engine_destroySubscription( hClient
                                       , subName
                                       , hClient
                                       , NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    rc = sync_ism_engine_destroyClientState( hClient
                                           , ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

}

/********************************************************************/
/* MAIN                                                             */
/********************************************************************/


/* Create a subscription with messages then : */

//Deliver message 1 and nack it in a loop until it has a delivery count of 4 then leave it unnacked*
//Deliver message 2 to a normal consumer who acks it consumed in a local transaction and doesn't commit
//Deliver message 3 to a normal consumer who acks it consumed in a global transaction and prepares but doesn't commit
//Deliver message 4 to a normal consumer who doesn't ack or nack it
//Deliver message 5-8 as above but to a  nonpersistent delivery count consumer

//Restart with the consumers still connected

//On restart:
//    Message 1 should have deliverycount=4 and be available
//            2 should have deliverycount=1 and be available
//            3 should have deliverycount=1 and be locked in the transaction
//            4 should have deliverycount=1 and be available
//            5 should have deliverycount=0 and be available
//            6 should have deliverycount=1 and be available
//            7 should have deliverycount=1 and be locked in the transaction
//            8 should have deliverycount=0 and be available

//We then commit the global transactions then:
//                  3 should be consumed  delivery count = 1
//                  7 should be consumed  delivery count = 1


int main(int argc, char *argv[])
{
    int trclvl = 4;
    int rc;
    int phase;

    ismEngine_SessionHandle_t hSession;
    ismEngine_ClientStateHandle_t hClient;
    char *adminDir = NULL;

    /************************************************************************/
    /* Parse the command line arguments                                     */
    /************************************************************************/
    rc = ProcessArgs( argc
                    , argv
                    , &phase
                    , &adminDir );

    if (rc != OK)
    {
        return rc;
    }

    test_setLogLevel(logLevel);

    rc = test_processInit(trclvl, adminDir);
    if (rc != OK) return rc;
    
    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    rc =  test_engineInit(phase == createPhase,
                          true,
                          ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                          false, /*recovery should complete ok*/
                          ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                          50);  // 50mb Store
    if (rc != OK) return rc;

    rc = test_createClientAndSessionWithProtocol(testClientId,
                                                 PROTOCOL_ID_JMS,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                 ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                                 &hClient,
                                                 &hSession,
                                                 true);
    TEST_ASSERT(rc == OK || rc == ISMRC_ResumedClientState , ("%d != %d || %d", rc, OK, ISMRC_ResumedClientState));

    char *subName = "rehydrateSub";

    //Generate an XID:
    ism_xid_t XID;

    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x12C13311;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x0123456789ABCDEF;
    globalId.bqual = 0xABCDEF0123456789;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    if (phase == createPhase)
    {
        createSubAndGetMessages(hClient, hSession, subName, &XID);
    }
    else
    {
        checkRecoveredMessages( hClient, hSession, subName, &XID);

        rc = test_engineTerm(true);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        test_processTerm(false);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        test_removeAdminDir(adminDir);
    }

    if (phase < RecoverPhase)
    {
        phase++;

        fprintf(stdout, "== Restarting for phase %d\n", phase);

        // Re-execute ourselves for the next phase
        int32_t newargc = 0;

        char *newargv[argc+8];

        newargv[newargc++] = argv[0];

        for(int32_t i=newargc; i<argc; i++)
        {
            if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "-a") == 0))
            {
                i++;
            }
            else
            {
                newargv[newargc++] = argv[i];
            }
        }

        char phaseString[50];
        sprintf(phaseString, "%d", phase);

        newargv[newargc++] = "-p";
        newargv[newargc++] = phaseString;
        newargv[newargc++] = "-a";
        newargv[newargc++] = adminDir;
        newargv[newargc] = NULL;

        printf("== Command: ");
        for(int32_t i=0; i<newargc; i++)
        {
            printf("%s ", newargv[i]);
        }
        printf("\n");

        rc = test_execv(newargv[0], newargv);
        TEST_ASSERT(false, ("'test_execv' failed. rc = %d", rc));
    }

    return rc;
}

/****************************************************************************/
/* ProcessArgs                                                              */
/****************************************************************************/
int ProcessArgs( int argc
               , char **argv
               , int *phase
               , char **padminDir )
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { NULL, 1, NULL, 0 } };
    int long_index;

    *phase = 0;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, "v:p:a:", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'p':
                *phase = atoi(optarg);
                break;
            case 'v':
               logLevel = atoi(optarg);
               if (logLevel > testLOGLEVEL_VERBOSE)
                   logLevel = testLOGLEVEL_VERBOSE;
               break;
            case 'a':
                *padminDir = optarg;
                break;
            case '?':
                usage=1;
                break;
            default: 
                printf("%c\n", (char)opt);
                usage=1;
                break;
        }
    }

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-v verbose]\n", argv[0]);
        fprintf(stderr, "       -v - logLevel 0-5 [2]\n");
        fprintf(stderr, "       -a - Admin directory to use\n");
        fprintf(stderr, "\n");
    }

    return usage;
}
