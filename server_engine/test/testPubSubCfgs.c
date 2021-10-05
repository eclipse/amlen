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
/* Module Name: testPubSubCfgs                                       */
/*                                                                   */
/* Description: UnitTests that pub/sub with different configurations */
/* of publishers/subscribers at different points in the topic tree   */
/*                                                                   */
/*********************************************************************/
#include <stdio.h>
#include <math.h>

#include "ismutil.h"
#include "engineInternal.h"
#include "engine.h"
#include "testPubSubThreads.h"
#include "policyInfo.h"
#include "resourceSetStats.h"

#include "test_utils_initterm.h"

#define TOPIC_FORMAT "test/"TPT_RESOURCE_SET_ID"/%s"

//Use the simple API to put a few subscribers at a single point in the
//topic tree
int testSimple(void) {
    int rc = OK;
    printf("Starting testSimple...\n");

    char topicString[256];
    sprintf(topicString, TOPIC_FORMAT, "testPubSubThreads");

    rc = doPubs(topicString, 3, 100000, 2, true, false, false, TxnCapable_False, 0);
    if (rc != OK) {
        printf("...test failed (doPubs() returned %d\n)", rc);
        return rc;
    }
    printf("...test passed\n");
    return OK;
}

//Test fanning out from 1 to to lots of subs
int testFanOut(void) {
    int rc = OK;
    printf("Starting testFanOut...\n");

    char topicString[256];
    sprintf(topicString, TOPIC_FORMAT, "testFanOut");

    rc = doPubs(topicString, 1, 10000, 200, true, false, false, TxnCapable_False, 0);
    if (rc != OK) {
        printf("...test failed (doPubs() for fan out returned %d\n)", rc);
        return rc;
    }
    printf("...test passed\n");
    return OK;
}
//Test fanning in from lots of pubs to 1 sub.
int testFanIn(void) {
    int rc = OK;
    printf("Starting testFanIn...\n");

    char topicString[256];
    sprintf(topicString, TOPIC_FORMAT, "testFanIn");

    rc = doPubs(topicString, 200, 1000, 1, true, false, false, TxnCapable_False, 0);
    if (rc != OK) {
        printf("...test failed (doPubs() for fan in returned %d\n)", rc);
        return rc;
    }
    printf("...test passed\n");
    return OK;
}
//Test the intermediate queue
int testDurable(void) {
    int rc = OK;
    printf("Starting testDurable...\n");

    char topicString[256];
    sprintf(topicString, TOPIC_FORMAT, "testDurable");

    rc = doPubs(topicString, 3, 10000, 10, true, false, false, TxnCapable_False, 2);
    if (rc != OK) {
        printf("...test failed (doPubs() for testDurable returned %d\n)", rc);
        return rc;
    }
    printf("...test passed\n");
    return OK;
}

//Test the multiconsumer queue
int testTxnCapable(void) {
    int rc = OK;
    printf("Starting testTxnCapable...\n");

    char topicString[256];
    sprintf(topicString, TOPIC_FORMAT, "testTxnCapable");

    rc = doPubs(topicString, 3, 10000, 10, true, false, false,TxnCapable_True, 1);
    if (rc != OK) {
        printf("...test failed (doPubs() for testDurable returned %d\n)", rc);
        return rc;
    }
    printf("...test passed\n");
    return OK;
}

int doMultilevelPubs(int depth, int width,
                     int pubberspernode, int msgsperpubber, int subberspernode,
                     bool subWait, bool cpuPin)
{
    int32_t rc = OK;
    ism_time_t setupTime;
    ism_time_t startTime;
    ism_time_t finishTime;
    int totalnodes = 0;

    psgroupcontroller gcontroller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo *pginfo = NULL;
    psgroupinfo **groupArray = NULL;

    initControllerConds(&gcontroller);

    int i, j;
    
    for(i=depth; i>0; i--) {
        totalnodes += (int)pow(width,i);
    }

    pginfo = calloc(1, sizeof(psgroupinfo)*totalnodes);

    psgroupinfo *pparent_ginfo = NULL;

    int x = 0;
    for(i=1; i<=depth; i++) {
        int nodesthislevel = (int)pow(width,i);

        for(j=0; j<nodesthislevel; j++) {
            pginfo[x].topicString = calloc(1, (pparent_ginfo ? strlen(pparent_ginfo->topicString) : 0) + 11);
            sprintf(pginfo[x].topicString, "%s/Topic%04d",
                    pparent_ginfo ? pparent_ginfo->topicString : "", x%width);

            if (x%width == (width-1)) {
              if (x > width)
                pparent_ginfo++;
              else
                pparent_ginfo = pginfo;
            }

            pginfo[x].numPubbers = pubberspernode;
            pginfo[x].msgsPerPubber = msgsperpubber;
            pginfo[x].numSubbers = subberspernode;
            pginfo[x].msgsPerSubber = pubberspernode * msgsperpubber;
            pginfo[x].controller = &gcontroller;

            gcontroller.threadsControlled += pubberspernode + subberspernode;

            x++;
        }
    }

    groupArray = calloc(1, sizeof(psgroupinfo *)*totalnodes);
    
    for (i=0; i<totalnodes; i++) {
        rc = InitPubSubThreadGroup(&(pginfo[i]));
        groupArray[i] = &(pginfo[i]);
    }

    if ( rc == OK ) {
        rc = TriggerSetupThreadGroups(&gcontroller,
                                      &setupTime,
                                      false);
    }

    if ( rc == OK ) {
        rc = StartThreadGroups(&gcontroller,
                               &startTime);
    }

    if ( rc == OK ) {
        rc = WaitForGroupsFinish(&gcontroller,
                                 &finishTime,
                                 totalnodes, groupArray);
    }

    return rc;
}

//Put pubbers+subbers at a few points in a topic tree
int testMultiTopic(void) {
    printf("Starting testMultiTopic...\n");

    int32_t rc = OK;
    ism_time_t setupTime;
    ism_time_t startTime;
    ism_time_t finishTime;

    psgroupcontroller gcontroller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo ginfo_a = PSGROUPDEFAULT;
    psgroupinfo ginfo_a_a = PSGROUPDEFAULT;
    psgroupinfo ginfo_a_b = PSGROUPDEFAULT;
    psgroupinfo ginfo_b = PSGROUPDEFAULT;

    initControllerConds(&gcontroller);

    char topicStringA[256];
    sprintf(topicStringA, TOPIC_FORMAT, "testMultiTopic/a");
    char topicStringAA[256];
    sprintf(topicStringAA, TOPIC_FORMAT, "testMultiTopic/a/a");
    char topicStringAB[256];
    sprintf(topicStringAB, TOPIC_FORMAT, "testMultiTopic/a/b");
    char topicStringB[256];
    sprintf(topicStringB, TOPIC_FORMAT, "testMultiTopic/b");

    ginfo_a.topicString   = topicStringA;
    ginfo_a.numPubbers    = 2;
    ginfo_a.msgsPerPubber = 1000;
    ginfo_a.numSubbers    = 4;
    ginfo_a.msgsPerSubber = ginfo_a.numPubbers * ginfo_a.msgsPerPubber;
    ginfo_a.controller    = &gcontroller;

    memcpy(&ginfo_a_a, &ginfo_a, sizeof(psgroupinfo));
    ginfo_a_a.topicString   = topicStringAA;

    memcpy(&ginfo_a_b, &ginfo_a, sizeof(psgroupinfo));
    ginfo_a_b.topicString   = topicStringAB;

    memcpy(&ginfo_b, &ginfo_a, sizeof(psgroupinfo));
    ginfo_b.topicString   = topicStringB;

    gcontroller.threadsControlled = 4 * (ginfo_a.numPubbers + ginfo_a.numSubbers);

    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_a);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_a_a);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_a_b);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_b);

    if ( rc == OK ) {
        rc = TriggerSetupThreadGroups(&gcontroller,
                                      &setupTime,
                                      false);
    }

    if ( rc == OK ) {
        rc = StartThreadGroups(&gcontroller,
                               &startTime);
    }

    if ( rc == OK ) {
        psgroupinfo *groupArray[4];

        groupArray[0] = &ginfo_a;
        groupArray[1] = &ginfo_a_a;
        groupArray[2] = &ginfo_a_b;
        groupArray[3] = &ginfo_b;

        rc = WaitForGroupsFinish(&gcontroller,
                                 &finishTime,
                                 4, groupArray);
    }

    return rc;

    printf("...test passed\n");
    return OK;
}

//Put pubbers+subbers with wildcardat a few points in a topic tree
int testMultiWildCardTopic(void) {
    printf("Starting testMultiWildCardTopic...\n");

    int32_t rc = OK;
    ism_time_t setupTime;
    ism_time_t startTime;
    ism_time_t finishTime;

    psgroupcontroller gcontroller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo ginfo_publishers = PSGROUPDEFAULT;
    psgroupinfo ginfo_subscribers = PSGROUPDEFAULT;
    psgroupinfo ginfo_publishers2 = PSGROUPDEFAULT;
    psgroupinfo ginfo_subscribers2 = PSGROUPDEFAULT;
    psgroupinfo ginfo_publishers3 = PSGROUPDEFAULT;
    psgroupinfo ginfo_subscribers3 = PSGROUPDEFAULT;

    initControllerConds(&gcontroller);

    char topicStringA[256];
    sprintf(topicStringA, TOPIC_FORMAT, "testMultiTopic/level1");
    ginfo_publishers.topicString   = topicStringA;
    ginfo_publishers.numPubbers    = 2;
    ginfo_publishers.msgsPerPubber = 1000;
    ginfo_publishers.numSubbers    = 0;
    ginfo_publishers.msgsPerSubber = 0;
    ginfo_publishers.controller    = &gcontroller;

    char topicStringB[256];
    sprintf(topicStringB, TOPIC_FORMAT, "testMultiTopic/+");
    ginfo_subscribers.topicString   = topicStringB;
    ginfo_subscribers.numPubbers    = 0;
    ginfo_subscribers.msgsPerPubber = 0;
    ginfo_subscribers.numSubbers    = 2;
    ginfo_subscribers.msgsPerSubber = ginfo_publishers.numPubbers * ginfo_publishers.msgsPerPubber ;
    ginfo_subscribers.controller    = &gcontroller;

    char topicStringC[256];
    sprintf(topicStringC, TOPIC_FORMAT, "testMultiTopic/level1/level2");
    ginfo_publishers2.topicString   = topicStringC;
    ginfo_publishers2.numPubbers    = 2;
    ginfo_publishers2.msgsPerPubber = 1000;
    ginfo_publishers2.numSubbers    = 0;
    ginfo_publishers2.msgsPerSubber = 0;
    ginfo_publishers2.controller    = &gcontroller;

    char topicStringD[256];
    sprintf(topicStringD, TOPIC_FORMAT, "testMultiTopic/+/+");
    ginfo_subscribers2.topicString   = topicStringD;
    ginfo_subscribers2.numPubbers    = 0;
    ginfo_subscribers2.msgsPerPubber = 0;
    ginfo_subscribers2.numSubbers    = 2;
    ginfo_subscribers2.msgsPerSubber = ginfo_publishers2.numPubbers * ginfo_publishers2.msgsPerPubber ;
    ginfo_subscribers2.controller    = &gcontroller;
    

    char topicStringE[256];
    sprintf(topicStringE, TOPIC_FORMAT, "testMultiTopic/level1/level2/level3");
    ginfo_publishers3.topicString   = topicStringE;
    ginfo_publishers3.numPubbers    = 2;
    ginfo_publishers3.msgsPerPubber = 1000;
    ginfo_publishers3.numSubbers    = 0;
    ginfo_publishers3.msgsPerSubber = 0;
    ginfo_publishers3.controller    = &gcontroller;

    char topicStringF[256];
    sprintf(topicStringF, TOPIC_FORMAT, "testMultiTopic/+/+/+");
    ginfo_subscribers3.topicString   = topicStringF;
    ginfo_subscribers3.numPubbers    = 0;
    ginfo_subscribers3.msgsPerPubber = 0;
    ginfo_subscribers3.numSubbers    = 2;
    ginfo_subscribers3.msgsPerSubber = ginfo_publishers3.numPubbers * ginfo_publishers3.msgsPerPubber ;
    ginfo_subscribers3.controller    = &gcontroller;
	
  
    gcontroller.threadsControlled = 12;

    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_publishers);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_subscribers);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_publishers2);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_subscribers2);
	if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_publishers3);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_subscribers3);
    
    if ( rc == OK ) {
        rc = TriggerSetupThreadGroups(&gcontroller,
                                      &setupTime,
                                      false);
    }

    if ( rc == OK ) {
        rc = StartThreadGroups(&gcontroller,
                               &startTime);
    }

    if ( rc == OK ) {
        psgroupinfo *groupArray[6];

        groupArray[0] = &ginfo_publishers;
        groupArray[1] = &ginfo_subscribers;
        groupArray[2] = &ginfo_publishers2;
        groupArray[3] = &ginfo_subscribers2;
		groupArray[4] = &ginfo_publishers3;
        groupArray[5] = &ginfo_subscribers3;
        rc = WaitForGroupsFinish(&gcontroller,
                                 &finishTime,
                                 6, groupArray);
    }

    return rc;

}

//Put pubbers+subbers with wildcardat a few points in a topic tree
int testMultiWildCardTopic_multilevel(void) {
    printf("Starting testMultiWildCardTopic  multilevel...\n");

    int32_t rc = OK;
    ism_time_t setupTime;
    ism_time_t startTime;
    ism_time_t finishTime;

    psgroupcontroller gcontroller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo ginfo_publishers = PSGROUPDEFAULT;
    psgroupinfo ginfo_subscribers = PSGROUPDEFAULT;
    psgroupinfo ginfo_publishers2 = PSGROUPDEFAULT;
    psgroupinfo ginfo_subscribers2 = PSGROUPDEFAULT;
    psgroupinfo ginfo_publishers3 = PSGROUPDEFAULT;
    psgroupinfo ginfo_subscribers3 = PSGROUPDEFAULT;
    
    initControllerConds(&gcontroller);

    char topicStringA[256];
    sprintf(topicStringA, TOPIC_FORMAT, "testMultiTopic/level1/level2");
    ginfo_publishers.topicString   = topicStringA;
    ginfo_publishers.numPubbers    = 2;
    ginfo_publishers.msgsPerPubber = 1000;
    ginfo_publishers.numSubbers    = 0;
    ginfo_publishers.msgsPerSubber = 0;
    ginfo_publishers.controller    = &gcontroller;

    /*Should receive all the messages from topic : ...testMultiTopic/level1/level2*/
    char topicStringB[256];
    sprintf(topicStringB, TOPIC_FORMAT, "testMultiTopic/#/level2");
    ginfo_subscribers.topicString   = topicStringB;
    ginfo_subscribers.numPubbers    = 0;
    ginfo_subscribers.msgsPerPubber = 0;
    ginfo_subscribers.numSubbers    = 2;
    ginfo_subscribers.msgsPerSubber = ginfo_publishers.numPubbers * ginfo_publishers.msgsPerPubber ;
    ginfo_subscribers.controller    = &gcontroller;
    ginfo_subscribers.QoS           = 1;
    ginfo_subscribers.txnCapable    = TxnCapable_True;
    
    char topicStringC[256];
    sprintf(topicStringC, TOPIC_FORMAT, "testMultiTopic/level1/level2/level3");
    ginfo_publishers2.topicString   = topicStringC;
    ginfo_publishers2.numPubbers    = 2;
    ginfo_publishers2.msgsPerPubber = 1000;
    ginfo_publishers2.numSubbers    = 0;
    ginfo_publishers2.msgsPerSubber = 0;
    ginfo_publishers2.controller    = &gcontroller;

    /*Should receive all the messages from topic : ...testMultiTopic/level1/level2/level3*/
    char topicStringD[256];
    sprintf(topicStringD, TOPIC_FORMAT, "testMultiTopic/#/level3");
    ginfo_subscribers2.topicString   = topicStringD;
    ginfo_subscribers2.numPubbers    = 0;
    ginfo_subscribers2.msgsPerPubber = 0;
    ginfo_subscribers2.numSubbers    = 2;
    ginfo_subscribers2.msgsPerSubber = ginfo_publishers2.numPubbers * ginfo_publishers2.msgsPerPubber ;
    ginfo_subscribers2.controller    = &gcontroller;
    ginfo_subscribers2.QoS           = 1;
    ginfo_subscribers2.txnCapable    = TxnCapable_True;
    
    char topicStringE[256];
    sprintf(topicStringE, TOPIC_FORMAT, "testMultiTopic/level1/level2/level3/level4/level5");
    ginfo_publishers3.topicString   = topicStringE;
    ginfo_publishers3.numPubbers    = 2;
    ginfo_publishers3.msgsPerPubber = 1000;
    ginfo_publishers3.numSubbers    = 0;
    ginfo_publishers3.msgsPerSubber = 0;
    ginfo_publishers3.controller    = &gcontroller;

	/*Should receive all the messages from topic : ...testMultiTopic/level1/level2/level3/level4/level5
	                                           AND ...testMultiTopic/level1/level2/level3 */
    char topicStringF[256];
    sprintf(topicStringF, TOPIC_FORMAT, "testMultiTopic/#/level3/#");
    ginfo_subscribers3.topicString   = topicStringF;
    ginfo_subscribers3.numPubbers    = 0;
    ginfo_subscribers3.msgsPerPubber = 0;
    ginfo_subscribers3.numSubbers    = 2;
    ginfo_subscribers3.msgsPerSubber = (ginfo_publishers2.numPubbers * ginfo_publishers2.msgsPerPubber) +
                                       (ginfo_publishers3.numPubbers * ginfo_publishers3.msgsPerPubber) ;
    ginfo_subscribers3.controller    = &gcontroller;
    ginfo_subscribers3.QoS           = 1;
    ginfo_subscribers3.txnCapable    = TxnCapable_True;
    
    gcontroller.threadsControlled = 12;

    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_publishers);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_subscribers);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_publishers2);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_subscribers2);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_publishers3);
    if (rc == OK ) rc = InitPubSubThreadGroup(&ginfo_subscribers3);

    if ( rc == OK ) {
        rc = TriggerSetupThreadGroups(&gcontroller,
                                      &setupTime,
                                      false);
    }

    if ( rc == OK ) {
        rc = StartThreadGroups(&gcontroller,
                               &startTime);
    }

    if ( rc == OK ) {
        psgroupinfo *groupArray[6];

        groupArray[0] = &ginfo_publishers;
        groupArray[1] = &ginfo_subscribers;
        groupArray[2] = &ginfo_publishers2;
        groupArray[3] = &ginfo_subscribers2;
        groupArray[4] = &ginfo_publishers3;
        groupArray[5] = &ginfo_subscribers3;
        rc = WaitForGroupsFinish(&gcontroller,
                                 &finishTime,
                                 6, groupArray);
    }

    return rc;

  
}

int process_args(int argc, char *argv[],
                 int *depth, int *width,
                 int *pubbers, int *subbers, int *msgsperpubber,
                 bool *wait, bool *cpupin, transactionCapableQoS1 *transactionCapable, int *qos)
{
    int rc = OK;
    int validargs = 0;
    int invalidargs = 0;
    int i;

    for (i=1; i<argc; i++) {
        char * argp = argv[i];
        int oldinvalidargs = invalidargs;

        if (argp[0]=='-') {
            if (argp[1] == 'p') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    *pubbers = atoi(argp);
                    validargs++;
                } else {
                    invalidargs++;
                }
            } else if (argp[1] == 's') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    *subbers = atoi(argp);
                    validargs++;
                } else {
                    invalidargs++;
                }
            } else if (argp[1] == 'd') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    *depth = atoi(argp);
                    validargs++;
                } else {
                    invalidargs++;
                }
            } else if (argp[1] == 'w') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    *width = atoi(argp);
                    validargs++;
                } else {
                    invalidargs++;
                }
            } else if (strcmpi(argp,"-mp") == 0) {
                if (i+1 < argc) {
                    argp = argv[++i];
                    *msgsperpubber = atoi(argp);
                    validargs++;
                } else {
                    invalidargs++;
                }
            } else if (strcmpi(argp,"-q") == 0) {
                if (i+1 < argc) {
                    argp = argv[++i];
                    *qos = atoi(argp);
                    validargs++;
                } else {
                    invalidargs++;
                }
            } else if (strcmpi(argp,"--nowait") == 0) {
                *wait = false;
                validargs++;
            } else if (strcmpi(argp,"--pin") == 0) {
                *cpupin = true;
                validargs++;
            } else if (strcmpi(argp,"--txn") == 0) {
                *transactionCapable = TxnCapable_True;
                validargs++;
            } else {
                invalidargs++;
            }
        } else {
            invalidargs++;
        }

        if (invalidargs > oldinvalidargs) {
            printf("Invalid arg: %s\n", argp);
        }
    }

    if(validargs == 0 || invalidargs > 0) {
        printf("USAGE: There are two modes of operation.\n");
        printf("1) Run with args, this program creates a number of publisher threads\n");
        printf("and a set of subscribing threads and flows msgs between them...\n");
        printf(" %s -p <num publishers> -s <num subscribers> -mp <msgs per publisher> [-q <qos>] [--nowait] [--pin] [--txn] (-d <tree depth> -w <tree width>)\n", argv[0]);
        printf(" --pin means each publisher thread is pinned to a cpu\n");
        printf(" --txn means (for QoS1 only!) that the transaction capable flag should be set (=> subscribers use the multiconsumer queue)\n");
        printf(" --nowait means subscribers finish as soon as they can rather than waiting for a moment to see if any\n");
        printf("   extra publications turn up in error\n\n");
        printf("2) Run with no args, this program runs a suite of unit tests\n\n");
        rc = 1;
    }
    return rc;
}
extern void ism_common_traceSignal(int signo);

/*
 * Catch a terminating signal and try to flush the trace file.
 */
static void sig_handler(int signo) {
    static volatile int in_handler = 0;
    signal(signo, SIG_DFL);
    if (in_handler)
        raise(signo);
    in_handler = 1;
    ism_common_traceSignal(signo);
    raise(signo);
}

int main(int argc, char *argv[])
{
    int rc = OK;
    int failures=0;
    int trclvl = 0;
    bool resourceSetStatsEnabled = false;
    bool resourceSetStatsTrackUnmatched;

    ism_time_t seedVal = ism_common_currentTimeNanos();
    srand(seedVal);

    // 70% of the time, turn on resourceSetStats
    if ((rand()%100) >= 30)
    {
        setenv("IMA_TEST_RESOURCESET_CLIENTID_REGEX", "^[^:]+:([^:]+):", false);
        setenv("IMA_TEST_RESOURCESET_TOPICSTRING_REGEX", "^[^/]+/([^/]+)", false);
        setenv("IMA_TEST_PROTOCOL_ALLOW_MQTT_PROXY", "true", false);

        resourceSetStatsEnabled = true;
        resourceSetStatsTrackUnmatched = ((rand()%100) >= 20); // 80% of the time track unmatched
        setenv("IMA_TEST_RESOURCESET_TRACK_UNMATCHED", resourceSetStatsTrackUnmatched ? "true" : "false", false);
    }

    rc = test_processInit(trclvl, NULL);
    if (rc != OK) goto mod_exit;

    rc = test_engineInit_DEFAULT;
    if (rc != OK) goto mod_exit;

    signal(SIGINT, sig_handler);
    signal(SIGSEGV, sig_handler);

    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    if(argc > 1) {
        int depth = 1;
        int width = 1;
        int pubbers = 1;
        int subbers = 1 ;
        int msgsperpubber = 10000;
        bool subWait = true;
        bool cpuPin = false;
        transactionCapableQoS1 transactionCapable =TxnCapable_False;
        int qos = 0;

        rc = process_args(argc, argv,
                          &depth, &width,
                          &pubbers, &subbers, &msgsperpubber,
                          &subWait, &cpuPin, &transactionCapable, &qos);

        if(rc == OK) {
            if (depth == 1 && width == 1) {
                rc = doPubs("/PSCfgs/Manual",
                            pubbers, msgsperpubber, subbers,
                            subWait, cpuPin, false, transactionCapable, qos);
            } else {
                rc = doMultilevelPubs(depth, width, 
                                      pubbers, msgsperpubber, subbers,
                                      subWait, cpuPin);
            }

            if (rc != OK) {
                printf("...test failed (doPubs() returned %d\n)", rc);
                return rc;
            }
        } else {
            return rc;
        }
    } else {
        failures += testSimple();
        failures += testFanOut();
        failures += testFanIn();
        failures += testDurable();
        failures += testTxnCapable();
        failures += testMultiTopic();
        failures += testMultiWildCardTopic();
        failures += testMultiWildCardTopic_multilevel();
    }

    ismEngine_ResourceSetMonitor_t *results = NULL;
    uint32_t resultCount = 0;

    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 10, NULL);
    if (rc != OK) goto mod_exit;

    if (resourceSetStatsEnabled)
    {
        uint32_t expectResultCount = resourceSetStatsTrackUnmatched ? 2 : 1;
        if (resultCount != expectResultCount)
        {
            printf("ERROR: ResultCount %u should be %u\n", resultCount, expectResultCount);
            rc = ISMRC_Error;
            goto mod_exit;
        }

        for(int32_t r=0; r<resultCount; r++)
        {
            if (strcmp(results[r].resourceSetId, iereDEFAULT_RESOURCESET_ID) == 0)
            {
                iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();
                if (results[r].resourceSet != defaultResourceSet)
                {
                    printf("ERROR: Default resource set pointer %p doesn't match expected %p\n",
                           results[r].resourceSet, defaultResourceSet);
                }
            }
            else
            {
                if (strcmp(results[r].resourceSetId, TPT_RESOURCE_SET_ID) != 0)
                {
                    printf("ERROR: Unexpected resourceSetId '%s' should be '%s'\n",
                           results[r].resourceSetId, TPT_RESOURCE_SET_ID);
                    rc = ISMRC_Error;
                    goto mod_exit;
                }

                if (results[r].stats.Connections == 0)
                {
                    printf("ERROR: Connection count %ld should be >0\n", results[r].stats.Connections);
                    rc = ISMRC_Error;
                    goto mod_exit;
                }
            }

#if 1
            iereResourceSet_t *resourceSet = (iereResourceSet_t *)(results[r].resourceSet);
            printf("ResourceSetStats for setId '%s' {", resourceSet->stats.resourceSetIdentifier);
            for(int32_t ires=0; ires<ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS; ires++)
            {
                printf("%ld%c", resourceSet->stats.int64Stats[ires], ires == ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS-1 ? '}':',');
            }
            printf("\n");
#endif

        }
    }
    else if (resultCount != 0)
    {
        rc = ISMRC_Error;
        goto mod_exit;
    }

    ism_engine_freeResourceSetMonitor(results);

    rc = test_engineTerm(true);
    if (rc != OK) goto mod_exit;

    rc = test_processTerm(true);
    if (rc != OK) goto mod_exit;

mod_exit:

    if (rc != OK && failures == 0) failures = 1;

    if (failures == 0) {
        printf("SUCCESS: testPubSubCfgs: tests passed\n");
    } else {
        printf("FAILURE: testPubSubCfgs: %d tests failed\n", failures);
        if (rc == OK) rc = 10;
    }
    return rc;
}
