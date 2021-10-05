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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ismutil.h>

#include "mqttbench.h"
#include "mqttclient.h"

extern uint8_t g_ClkSrc;
extern ioProcThread **ioProcThreads;
extern int g_LongestClientIDLen;

static void add_cmdhistory(char * cmd);
static void init_readline(void);
typedef  char * (* readline_t)(const char *);
typedef  void   (* add_history_t)(const char *);
readline_t    ism_readline = NULL;
add_history_t ism_add_history = NULL;

/* Some external symbols for readline */
char BC = 0;
char PC = 0;
char UP = 0;

/* *************************************************************************************
 * Initialize for readline by loading the library and finding the symbols used.
 * If readline is not available, use the normal stdin processing.
 * *************************************************************************************/
static void init_readline(void) {
    char  errbuf [512];
    LIBTYPE  rlmod;

    /* We do not care what version of the editline library we have so use any version */
    rlmod = dlopen("libedit.so" DLLOAD_OPT);/* Use the base symbolic link if it exists*/
    if(!rlmod) {
        rlmod = dlopen("libedit.so.0" DLLOAD_OPT); /* Use editline version in CENTOS 7, and SLES 10,11 */
        if(!rlmod) {
        	rlmod = dlopen("libedit.so.2" DLLOAD_OPT); /* Use editline version in Ubuntu 14 */
        }
    }

    if (rlmod) {
        ism_readline = (readline_t) dlsym(rlmod, "readline");
        ism_add_history = (add_history_t) dlsym(rlmod, "add_history");
        if (!ism_readline || !ism_add_history) {
            dlerror_string(errbuf, sizeof(errbuf));
            MBTRACE(MBINFO, 6, "readline and add_history symbols not found in libedit.so (command history will not be available): %s\n", errbuf);
        } else {
        	MBTRACE(MBINFO, 5, "Loaded libedit.so functions (e.g. command history) for the interactive command line\n");
        }
    } else {
        dlerror_string(errbuf, sizeof(errbuf));
        MBTRACE(MBINFO, 5, "The libedit.so not available, falling back to fgets loop (i.e. no command history on prompt): %s\n", errbuf);
    }
}   /* BEAM suppression:  file leak */

/* *************************************************************************************
 *  Callback function for enumerating the entries in the message stream HashMap
 *
 *   @param[in]  entry         = hashmap entry
 *   @param[in]  context       = user data passed to the callback function
 *
 *   @return 0 = continue enumerating, 1 = stop enumerating the entries in the map
 * *************************************************************************************/
int streamMapEnumCB(ismHashMapEntry * entry, void * context){
	int stop = 0; // continue enumerating the map
	int *mincount = (int *) context;
	char sidStr[64] = {0};
	memcpy(sidStr, entry->key, entry->key_len);
	msgStreamObj *streamObj = (msgStreamObj *) entry->value;
	if(streamObj->outOfOrder >= *mincount || streamObj->reDelivered >= *mincount) {
		fprintf(stdout, "\tsid=%llu txInstanceId=%u q0=%llu q1=%llu q2=%llu ooo=%llu redel=%llu txTopicStr=%s rxClientId=%s\n",
						(ULL) streamObj->streamID,
						streamObj->txInstanceId,
						(ULL)streamObj->q0,
						(ULL)streamObj->q1,
						(ULL)streamObj->q2,
						(ULL)streamObj->outOfOrder,
						(ULL)streamObj->reDelivered,
						streamObj->txTopicStr,
						streamObj->rxClient->clientID);
	}

	return stop;
}


/* *************************************************************************************
 * Add the string to the history if it is not one of the last two commands used
 * *************************************************************************************/
static void add_cmdhistory (char *cmd)
{
    static char cmd_history1[MAX_CMDLINE_HISTORY_LEN];
    static char cmd_history2[MAX_CMDLINE_HISTORY_LEN];

    int len;

    if (!cmd)
        return;
    len = strlen(cmd);
    if (len < MAX_CMDLINE_HISTORY_LEN) {
        if (!strcmp(cmd_history1, cmd) || !strcmp(cmd_history2, cmd))
            return;
        strcpy(cmd_history2, cmd_history1);
        strcpy(cmd_history1, cmd);
    }

    ism_add_history(cmd);
}

/* ==================================================================================== */
/* ==================================================================================== */

/* ******************************** GLOBAL VARIABLES ********************************** */
/* Externs */
extern uint8_t g_JitterLock;
extern uint8_t g_RequestedQuit;
extern double g_MsgRate;
extern int g_MBErrorRC;

/* *************************************************************************************
 * doDumpClients
 *
 * Description:  Dump the current client information to a file for testing purposes.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *   @param[in]  fName               = Filename of the file to dump the client info to.
 * *************************************************************************************/
void doDumpClients (mqttbenchInfo_t *mqttbenchInfo, FILE *fName)
{
	int rc = 0;
	int i, k, m;
	int numTopics;

    ism_common_listIterator *pClientIter;
    ism_common_list_node *currClient;

    mqttbench_t **mbInstArray = mqttbenchInfo->mbInstArray;

    /* Create an iterator to go through the clients. */
    pClientIter = (ism_common_listIterator *)calloc(1, sizeof(ism_common_listIterator));
    if (pClientIter) {
    	if (mbInstArray) {
    		for ( m = 0 ; m < mqttbenchInfo->mbCmdLineArgs->numSubmitterThreads ; m++ ) {
    			if (mbInstArray[m]) {
    				/* Set the pClientIter */
    				ism_common_list_iter_init(pClientIter, mbInstArray[m]->clientTuples);

    				/* Go through each client associated with this thread and provide Topics */
    				while ((currClient = ism_common_list_iter_next(pClientIter)) != NULL) {
    					fprintf(fName, "ClientID: %s\n", ((mqttclient_t *)(currClient->data))->clientID);

    					for ( i = 0 ; i < 2 ; i++ ) {
    						if (i == 0) {
    							numTopics = ((mqttclient_t *)(currClient->data))->destRxListCount;
    							if (numTopics > 0)
    	     						fprintf(fName, "    Subscribed Topics:\n");
    							else
    								continue;
    						} else {
    							numTopics = ((mqttclient_t *)(currClient->data))->destTxListCount;
    							if (numTopics > 0)
    								fprintf(fName, "    Published Topics:\n");
    							else
    								continue;
    						}

    						for ( k = 0 ; k < numTopics ; k++ ) {
    							if (i == 0)
    								fprintf(fName, "        %s \n", ((mqttclient_t *)(currClient->data))->destRxList[k]->topicName);
    							else
    								fprintf(fName, "        %s \n", ((mqttclient_t *)(currClient->data))->destTxList[k]->topicName);
    						}
    					}

    					fprintf(fName, "\n");
    				}

    				/* Perform some cleanup. */
    				ism_common_list_iter_destroy(pClientIter);
    			} /* if (mbInstArray[m]) */
    		} /* for loop of the submitter threads */
    	}

   	    free(pClientIter);
	} else {
    	rc = provideAllocErrorMsg("an iterator", (int)sizeof(ism_common_listIterator), __FILE__, __LINE__);
    	exit (rc);
    }
} /* doDumpClients */

/* *************************************************************************************
 * doDumpClientInfo
 *
 * Description:  Dump the current client information to a file for testing purposes.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 * *************************************************************************************/
void doDumpClientInfo (mqttbenchInfo_t *mqttbenchInfo)
{
	int rc = 0;
	FILE *fp;

	rc = createFile(&fp, CLIENT_INFO_FNAME);
	if (rc == 0) {
		doDumpClients(mqttbenchInfo, fp);
		fclose(fp);
	} else
		MBTRACE(MBERROR, 1, "Failed to dump client information to %s.\n", CLIENT_INFO_FNAME);

} /* doDumpClientInfo */

/* *************************************************************************************
 * Display the information requested for the specified topic or subscription
 *
 *   @param[in]  client              = the mqttclient_t object to which this topic belongs
 *   @param[in]  dspTopicCt          = flag to indicate whether to display the header
 *   @param[in]  destTuple           = destionation object to display
 *   @param[in]  destType            = destination type (PUB = topic, SUB = subscription)
 *   @param[in]  detail              = If set = 0 then more spaces are used for formatting.
 * *************************************************************************************/
void doDisplayTopicInfo (mqttclient_t *client, int dspTopicCt, destTuple_t *dest, int destType, int detail)
{
	if (dest) {
		if (destType == SUB) {
			char stats[1024] = {0};
			if(client->mqttVersion >= MQTT_V5 || !client->usesWildCardSubs) {
				sprintf(stats,
						"RX (Q0=%llu Q1=%llu Q2=%llu PUBREL=%llu) TX (PUBACK=%llu PUBREC=%llu PUBCOMP=%llu) BADRC=%llu PUBREC-PUBREL=%llu RxRetain=%llu RxDup=%llu SubOpts: NL=%u, RAP=%u, RH=%u",
						(ULL)dest->currQoS0MsgCount,
						(ULL)dest->currQoS1MsgCount,
						(ULL)dest->currQoS2MsgCount,
						(ULL)dest->currPUBRELCount,
						(ULL)dest->currPUBACKCount,
						(ULL)dest->currPUBRECCount,
						(ULL)dest->currPUBCOMPCount,
						(ULL)dest->badRCCount,
						(ULL)dest->currPUBRECCount - dest->currPUBRELCount,
						(ULL)dest->rxRetainCount,
						(ULL)dest->rxDupCount,
						dest->noLocal,
						dest->retainAsPublished,
						dest->retainHandling);
			}
			fprintf(stdout, "%s%s  %s  SUBQoS=%d %s\n",
				            (detail == 0 ? "    " : ""),
                            (dspTopicCt == 0 ? "Subscribe:" : "          "),
							dest->topicName,
                            (int)dest->topicQoS,
                            stats);
		} else {
			fprintf(stdout, "%s%s  %s  Retain=%d TX (Q0=%llu Q1=%llu Q2=%llu PUBREL=%llu) RX (PUBACK=%llu PUBREC=%llu PUBCOMP=%llu) BADRC=%llu Q1-PUBACK=%llu PUBREL-PUBCOMP=%llu NoSub=%llu sid=%s\n",
				            (detail == 0 ? "    " : ""),
	                        (dspTopicCt == 0 ? "Publish:  " : "          "),
							dest->topicName,
							dest->retain,
                            (ULL)dest->currQoS0MsgCount,
							(ULL)dest->currQoS1MsgCount,
							(ULL)dest->currQoS2MsgCount,
							(ULL)dest->currPUBRELCount,
							(ULL)dest->currPUBACKCount,
							(ULL)dest->currPUBRECCount,
							(ULL)dest->currPUBCOMPCount,
							(ULL)dest->badRCCount,
							(ULL)dest->currQoS1MsgCount - dest->currPUBACKCount,
							(ULL)dest->currPUBRELCount - dest->currPUBCOMPCount,
							(ULL)dest->txNoSubCount,
							dest->streamIDStr);
		}

		fflush(stdout);
	}
} /* doDisplayTopicInfo */

/* *************************************************************************************
 * doDisplayClientInfo
 *
 * Description:  Display the information requested for the specified Client.
 *
 *   @param[in]  client              = Specific mqttclient to be use.
 *   @param[in]  flag                = Flag to print column titles (0 = print titles).
 *   @param[in]  numThrdsPub         = Number of submitter threads containing publishers.
 * *************************************************************************************/
void doDisplayClientInfo (mqttclient_t *client, int flag, int numThrdsPub)
{
    transport_t *trans = client->trans;
	int prefixLen = g_LongestClientIDLen;

	if (flag == 0) {
		fprintf(stdout, "%-*s  %-4s  %-13s  %-13s  %-15s  %-7s  %-15s  %-7s  %-7s\n",
                        prefixLen,
                        "ClientID",
						"IOP",
                        "MQTT Version",
						"Prev Session",
						"Src IP",
                        "Port",
                        "Dest IP",
                        "Port",
						"Line");
	}

	fprintf(stdout, "%-*s  %-4d  %-13s  %-13s  %-15s  %-7d  %-15s  %-7d  %-7d\n",
                    prefixLen,
                    client->clientID,
					client->ioProcThreadIdx,
					client->mqttVersion == MQTT_V5 ? "5" : (client->mqttVersion == MQTT_V311) ? "3.1.1" : "3.1",
					client->sessionPresent ? "yes" : "no",
                    inet_ntoa(trans->clientSockAddr.sin_addr),
                    trans->srcPort,
	                trans->serverIP,
	                trans->serverPort,
					client->line);

	fflush(stdout);
} /* doDisplayClientInfo */

/* *************************************************************************************
 * doDisplayClientDetail
 *
 * Description:  Display the information requested for the specified Client.
 *
 *   @param[in]  client              = Specific mqttclient to be use.
 *   @param[in]  numThrdsPub         = Number of submitter threads containing publishers,
 *                                     which is used to provide the correct RX & TX content.
 * *************************************************************************************/
void doDisplayClientDetail (mqttclient_t *client, int numThrdsPub)
{
	int stateIdx = 0;

    char *supportedMQTTStates[] = { NULL, \
    		                        "CONNECT", \
    							    "CONNACK", \
    							    "PUBLISH", \
    							    "PUBACK", \
    							    "PUBREC", \
    							    "PUBREL", \
    							    "PUBCOMP", \
    							    "SUBSCRIBE", \
    							    "SUBACK", \
    							    "UNSUBSCRIBE", \
    							    "UNSUBACK", \
    							    "PINGREQ", \
    							    "PINGRESP", \
    							    "DISCONNECT", \
                                    NULL};

    transport_t *trans = client->trans;

    double now = g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime();

    fprintf(stdout, "\n");
    fprintf(stdout, "ClientID: %s, line=%d\n",
    				client->clientID,
					client->line);
    fprintf(stdout, "States: transport=0x%X, protocol=0x%X, reconnectEnabled=%d sockfd=%d\n",
					trans->state,
					client->protocolState,
					trans->reconnect,
					trans->socket);
    fprintf(stdout, "Elapsed time (seconds) since: reset=%f tcpconnect=%f mqttconnect=%f mqttdisconnect=%f\n",
    				client->resetConnTime == 0 ? 0 : now - client->resetConnTime,
    				client->tcpConnReqSubmitTime == 0 ? 0 : now - client->tcpConnReqSubmitTime,
    				client->mqttConnReqSubmitTime == 0 ? 0 : now - client->mqttConnReqSubmitTime,
					client->mqttDisConnReqSubmitTime == 0 ? 0 : now - client->mqttDisConnReqSubmitTime);
	fprintf(stdout, "MsgIDs: available=%d, inuse=%d\n",
					client->maxInflightMsgs - (client->currInflightMessages),
					client->currInflightMessages);
	fprintf(stdout, "Assigned IOP: %d JobIdx: %d NumJobs: %llu\n", client->ioProcThreadIdx, trans->jobIdx, (ULL) trans->processCount);
	fprintf(stdout, "TraceEnabled: %s, Clean start/session: %u, Prev Session: %s\n",
					(client->traceEnabled) ? "yes" : "no",
					(client->mqttVersion >= MQTT_V5) ? client->cleanStart : client->cleansession,
					client->sessionPresent ? "yes" : "no");
	fprintf(stdout, "Connection Information:\n");
	fprintf(stdout, "\tSrcIP: %s   SrcPort: %d   DstIP: %s   DstPort: %d   Linger: %d secs\n",
                    inet_ntoa(trans->clientSockAddr.sin_addr),
					trans->srcPort ? trans->srcPort : ntohs(trans->clientSockAddr.sin_port),
                    trans->serverIP,
			        trans->serverPort,
					(int) (client->lingerTime_ns / NANO_PER_SECOND));
	fprintf(stdout, "\tTransport Addr:  %p\n", trans);
	fprintf(stdout, "\tTCP Conn Count:  %llu    TLS Conn Count:  %llu    TLS Error Count:  %llu\n",
			        (ULL)client->tcpConnCtr,
					(ULL)client->tlsConnCtr,
					(ULL)client->tlsErrorCtr);
    fprintf(stdout, "\tsocket error ct: %d   last errno: %d   bind failures: %llu\n\n",
	                client->socketErrorCt,
	                client->lastErrorNo,
					(ULL) trans->bindFailures);

    fprintf(stdout, "Failed subscriptions: %llu \nBad RC: Rx=%llu Tx=%llu\n",
    		        (ULL)client->failedSubscriptions,
    				(ULL)client->badRxRCCount,
					(ULL)client->badTxRCCount);

   	fprintf(stdout, "MSG counts:\t\t\t    RX\t\t      TX\n");

   	for ( stateIdx = CONNECT ; stateIdx < MQTT_NUM_MSG_TYPES ; stateIdx++ ) {
   		if (supportedMQTTStates[stateIdx] != NULL) {
   			fprintf(stdout, "\t%-12s:\t%14ld    %14ld\n",
		                    supportedMQTTStates[stateIdx],
			                client->currRxMsgCounts[stateIdx],
                            client->currTxMsgCounts[stateIdx]);
   		}
   	}

	fprintf(stdout, "\n");
	fflush(stdout);
} /* doDisplayClientDetail */

/* *************************************************************************************
 * doChangeMessageRate
 *
 * Description:  Change the message rate.   This is shared between the timer task
 *               onTimerBurstMode and from doCommands when user types in:  rate=x
 *
 *   @param[in]  newRate             = The new requested rate.
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *   @param[in]  whom                = The caller requesting the rate change.
 *
 *   @return 0   = Successfully changed the rate.
 * *************************************************************************************/
int doChangeMessageRate (int newRate, mqttbenchInfo_t *mqttbenchInfo, uint8_t whom)
{
	int rc = 0;
	int i;
	int currRate = (int)g_MsgRate;

	mqttbench_t * mbinst;

	/* --------------------------------------------------------------------------------
	 * Message rate to be changed:
	 *
	 * If invoked via TimerTask then the changes will be:
	 *    -  Base rate to Burst rate
	 *       -- OR --
	 *    -  Burst rate to Base rate
	 *
	 * If invoked via doCommands then the user requested a specific rate change.
	 * -------------------------------------------------------------------------------- */
	if (mqttbenchInfo->mbCmdLineArgs->rateControl == 0) { /* Self rate control */
		MBTRACE(MBINFO, 1, "%s the global message rate, prev=%d (msgs/sec) new=%d (msgs/sec).\n",
 					       (whom == TIMERTASK ? "Burst timer changed" : "User changed"),
					       currRate,
					       newRate);

		g_MsgRate = newRate;

		for ( i = 0 ; i < mqttbenchInfo->mbCmdLineArgs->numSubmitterThreads ; i++ ) {
			mbinst = mqttbenchInfo->mbInstArray[i];

			/* Set the rateChenged flag for each thread */
			mbinst->rateChanged = 1;
		}
	} else {  /* No rate control */
		if (whom == DOCOMMANDS) {
			MBTRACE(MBWARN, 6, "Rate change is not available when passing command line parameter -t with value -1. \n" \
                           "Please rerun with rate control: -t 0 or don't specify the -t parameter at all (default is 0)\n");
			fprintf(stdout, "Rate change is not available when passing command line parameter -t with value -1. \n" \
                     	    "Please rerun with rate control: -t 0 or don't specify the -t parameter at all (default is 0)\n");
		}
	}

	return rc;
} /* doChangeMessageRate */

/* *************************************************************************************
 * provideHelp
 *
 * Description:  Command prompt for the application.
 *
 *   @param[in]  thread_parm         = Contains the pointer to mqttbenchInfo_t
 *   @param[in]  context             = x
 *   @param[in]  value               = x
 * *************************************************************************************/
static void provideHelp (void)
{
	fprintf(stdout, "\npossible commands: \n");
	fprintf(stdout, "\thelp(h)           -- help information.\n");
	fprintf(stdout, "\tclose             -- close all connections without MQTT UnSubscribing.\n");
	fprintf(stdout, "\tquit(q)           -- close all connections with MQTT UnSubscribe and Disconnect.\n");
	fprintf(stdout, "\tstat              -- print current statistics -or- press the 'Enter' key.\n");
	fprintf(stdout, "\tal(l)             -- get latency.\n");
	fprintf(stdout, "\tenv               -- print the environment variable settings.\n");
	fprintf(stdout, "\tfreemem           -- print the amount of free memory on the system.\n");
	fprintf(stdout, "\tnt                -- print # topics.\n");
	fprintf(stdout, "\trate=[new rate]   -- set new rate.\n");
	fprintf(stdout, "\tresetlat          -- reset the latency statistics.\n");
	fprintf(stdout, "\tuptime            -- print mqttbench uptime.\n");
	fprintf(stdout, "\tver               -- print the mqttbench version.\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\tc *                       -- print client summary info for all clientIDs.\n");
	fprintf(stdout, "\tc [clientID]              -- print client summary info for matching: clientID regex.\n");
	fprintf(stdout, "\tc [clientID] dump         -- print client detailed info for matching: clientID regex.\n");
	fprintf(stdout, "\tc [clientID [topic]]      -- print client summary info for matching: clientID regex & topic regex.\n");
	fprintf(stdout, "\tc [clientID [topic]] dump -- print client detailed info for matching: clientID regex & topic regex.\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\tbadclient [count] -- print clients which have at least count of non-zero MQTT return code occurrences.\n");
	fprintf(stdout, "\tclientTrace [regex] <enable|disable>\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\tfindsid [sid]     -- print client ID of the publisher that owns the message stream identified by stream ID (sid).\n");
	fprintf(stdout, "\tfindtopic [topic] -- print client ID of the publishers or subscribers that publish/subscribe to the topic.\n");
	fprintf(stdout, "\tlse               -- print the last x (max: 10) socket errors per IOP\n");
	fprintf(stdout, "\tsidchk [mincount] -- print message streams which have at least a mincount of redelivered or outoforder messages.\n");
	fprintf(stdout, "\ttl [new level]    -- set a new trace level (valid range: 0-9).\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\tlstate            -- print the supported states for pstate and tstate commands.\n");
	fprintf(stdout, "\tpstate [state]    -- print clientIDs in specified protocol state.\n");
	fprintf(stdout, "\ttstate [state]    -- print clientIDs in specified transport state.\n");
	fprintf(stdout, "\tm [state]         -- print current connections in the state specified (only supports CONNACK)\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\tjitter [opt]\n");
	fprintf(stdout, "\t    results       -- print the current jitter information.\n");
	fprintf(stdout, "\t    reset         -- reset the max, min and array for jitter information.\n");
	fprintf(stdout, "\t    settings      -- print the current interval and sample count.\n");
	fprintf(stdout, "\t    [msecs] [cnt] -- enables jitter with sample interval <msecs> and sample count <cnt>).\n");
	fprintf(stdout, "\tslowacker [#acks] -- print connections that are slow ack'g based on supplied: #acks.\n");
	fprintf(stdout, "\n");

	fflush(stdout);
} /* provideHelp */

/* *************************************************************************************
 * doCommands
 *
 * Description:  Command prompt for the application.
 *
 *   @param[in]  thread_parm         = Contains the pointer to mqttbenchInfo_t
 *   @param[in]  context             = x
 *   @param[in]  value               = x
 * *************************************************************************************/
void * doCommands (void *thread_parm, void *context, int value)
{
	mqttbenchInfo_t *pMBInfo = (mqttbenchInfo_t *)thread_parm;

	environmentSet_t *pSysEnvSet = pMBInfo->mbSysEnvSet;
	mbCmdLineArgs_t *pCmdLineArgs = pMBInfo->mbCmdLineArgs;
	mbThreadInfo_t *pThrdInfoDoCmds = pMBInfo->mbThreadInfo;
	mbJitterInfo_t *pJitterInfo = pMBInfo->mbJitterInfo;

	int rc = 0;
	int i;
	int latencyMask = pCmdLineArgs->latencyMask;
    int linesize = MAX_DOCOMMANDS_CMDLINE_LEN;
    int numThreadsSub = 0;
    int numThreadsPub = 0;
    int initThreadType = 0;
    int mqttClientType = 0;
	int mask;
	int newRate;
	int numSubmitThrds = pMBInfo->mbCmdLineArgs->numSubmitterThreads;

	uint64_t *topicSendCount = NULL;
	uint64_t *topicRecvCount = NULL;

	char tmpTime[MAX_TIME_STRING_LEN];
	char xprompt[MAX_PROMPT_LEN];
	char commandline[MAX_DOCOMMANDS_CMDLINE_LEN];

	char *command = NULL;
	char *subCmdInfo = NULL;
	char *ptrSubCmd = NULL;
	char *tmp;
    const char *FIFONAME = "/tmp/mqttbench-np";

    struct timeb beginTime;     /* Timestamp structure for beginning time. */

	ism_common_list_node *currClient = NULL;
	ism_common_listIterator *pClientIter = NULL;

	mqttclient_t *client = NULL;
	mqttbench_t **mbInstArray = pMBInfo->mbInstArray;

	/* Variables needed for lstate, pstate and tstate. */
    char *supportedProtocolStates[] = { "MQTT_UNKNOWN", \
                                        "MQTT_CONNECT_IN_PROCESS", \
			                            "MQTT_CONNECTED", \
			                            "MQTT_PUBSUB", \
			                            "MQTT_DOUNSUBSCRIBE", \
			                            "MQTT_UNSUBSCRIBE_IN_PROCESS", \
			                            "MQTT_UNSUBSCRIBED", \
			                            "MQTT_DISCONNECT_IN_PROCESS", \
			                            "MQTT_DISCONNECTED", \
			                            "MQTT_RETRY_DISCONNECT", \
    								    NULL};

    uint16_t protocolStateValues[] = { MQTT_UNKNOWN, \
                                       MQTT_CONNECT_IN_PROCESS, \
			                           MQTT_CONNECTED, \
			                           MQTT_PUBSUB, \
							           MQTT_DOUNSUBSCRIBE, \
								       MQTT_UNSUBSCRIBE_IN_PROCESS, \
								       MQTT_UNSUBSCRIBED, \
			                           MQTT_DISCONNECT_IN_PROCESS, \
			                           MQTT_DISCONNECTED, \
                                       MQTT_RETRY_DISCONNECT };

    int numProtocolStates = sizeof(protocolStateValues) / sizeof(uint16_t);

    char *supportedTransportStates[] = { "READ_WANT_READ", \
    		                             "READ_WANT_WRITE", \
    		                             "WRITE_WANT_READ", \
    		                             "WRITE_WANT_WRITE", \
    		                             "SOCK_CAN_READ", \
    		                             "SOCK_CAN_WRITE", \
    		                             "HANDSHAKE_IN_PROCESS", \
			                             "SHUTDOWN_IN_PROCESS", \
			                             "SOCK_CONNECTED", \
			                             "SOCK_DISCONNECTED", \
			                             "SOCK_ERROR", \
    		                             "SOCK_NEED_CONNECT", \
    		                             "SOCK_NEED_CREATE", \
    		                             "SOCK_WS_IN_PROCESS", \
    								     NULL};

    uint16_t transportStateValues[] = { READ_WANT_READ, \
    							        READ_WANT_WRITE, \
    							        WRITE_WANT_READ, \
    							        WRITE_WANT_WRITE, \
    							        SOCK_CAN_READ, \
    							        SOCK_CAN_WRITE, \
    							        HANDSHAKE_IN_PROCESS, \
    							        SHUTDOWN_IN_PROCESS, \
    							        SOCK_CONNECTED, \
    							        SOCK_DISCONNECTED, \
    							        SOCK_ERROR, \
    							        SOCK_NEED_CONNECT, \
    							        SOCK_NEED_CREATE, \
    							        SOCK_WS_IN_PROCESS };

    int numTransportStates = sizeof(transportStateValues) / sizeof(uint16_t);

    /* Flags */
    uint8_t aggregateOnly = 0;
    uint8_t found;

    /* Initialize editline processing */
    init_readline();

	sleep(1); /* wait for other prints to be written out before presenting command line prompt */

	if (pMBInfo->instanceClientType == CONTAINS_DUALCLIENTS)
		initThreadType = INIT_STRUCTURE_ONLY;
	else
		initThreadType = PRELIM_THREADTYPE;

	pThrdInfoDoCmds = initMBThreadInfo(initThreadType, pMBInfo);
	if (pThrdInfoDoCmds) {
		if (pMBInfo->instanceClientType == CONTAINS_DUALCLIENTS) {
			pThrdInfoDoCmds->numDualThreads = pMBInfo->mbCmdLineArgs->numSubmitterThreads;
			mqttClientType = CONTAINS_DUALCLIENTS;
		} else {
			/* Set the mqttClientType up based on the number of threads. */
			if (pThrdInfoDoCmds->numConsThreads)
				mqttClientType |= CONSUMERS_ONLY;
			if (pThrdInfoDoCmds->numProdThreads)
				mqttClientType |= PRODUCERS_ONLY;
		}
	} else
		exit (1);

	numThreadsSub = (int)pThrdInfoDoCmds->numConsThreads + (int)pThrdInfoDoCmds->numDualThreads;
	numThreadsPub = (int)pThrdInfoDoCmds->numProdThreads + (int)pThrdInfoDoCmds->numDualThreads;

	fprintf(stdout, "Start command prompt.  Type: quit (exit)%s\n",
	                (numThreadsSub > 0 ? "  OR  close (close connections)" : ""));
	fflush(stdout);

	/* Get initial time */
	ftime(&beginTime);

	if (pSysEnvSet->pipeCommands) {
		int rcPipe = mkfifo(FIFONAME, S_IRUSR | S_IWUSR);
		if (rcPipe && errno != EEXIST) {
			MBTRACE(MBERROR, 1, "Failed to create named pipe (%s) for reading commands, errno=%d\n", FIFONAME, errno);
			g_MBErrorRC = RC_MKFIFO_ERR;
			goto END;
		}
		MBTRACE(MBINFO, 5, "Created named pipe %s to receive interactive commands\n", FIFONAME);
	}

	/* Create the iterator for the clients and threads. */
	pClientIter = (ism_common_listIterator *)calloc(1, sizeof(ism_common_listIterator));
	if (pClientIter == NULL) {
		g_MBErrorRC = provideAllocErrorMsg("an iterator", (int)sizeof(ism_common_listIterator), __FILE__, __LINE__);
		goto END;
	}

	/* Only break out of this infinite loop if the user types "quit" */
	while (!g_RequestedQuit) {
		char *xbuf;
        memset(commandline,'\0',linesize);

		/* Display general information in regards to duration and Current Time started */
        createLogTimeStamp(tmpTime, MAX_TIME_STRING_LEN, LOGFORMAT_HH_MM_SS);
		snprintf(xprompt, MAX_PROMPT_LEN, "\n%s mqttbench.%u> ", tmpTime, pMBInfo->instanceID);

		if (pSysEnvSet->pipeCommands) {
			int fd = open(FIFONAME,O_RDONLY);
			if (fd == -1) {
				MBTRACE(MBERROR, 1, "Failed to open named pipe (%s) for reading commands, errno=%d\n", FIFONAME, errno);
				break;
			}

			int bytesRead = read(fd, commandline, linesize);
			close(fd);
			if (bytesRead < 1)
				continue;
			else
				commandline[bytesRead-1]='\0';
		} else {
    		if (ism_readline) {
    			xbuf = ism_readline(xprompt);
				/* --------------------------------------------------------------------
				 * If xbuf is a non-null line and not the same as one of the last two
				 * entries in the history file, add it to the history file.
				 * -------------------------------------------------------------------- */
    			if (xbuf) {
    				add_cmdhistory(xbuf);
    				ism_common_strlcpy(commandline, xbuf, sizeof(commandline)-1);
    				free(xbuf);
    				xbuf = commandline;
    			}
    		} else {
    			fprintf(stdout, "%s", xprompt);
    			fflush(stdout);
    			while(1){ // infinite loop waiting for input from stdin
    				struct timeval timeout = {1, 0}; // wait up to 1 second for a command, before checking if we should be shutting down
    				fd_set selectset;
    				FD_ZERO(&selectset);
    				FD_SET(0, &selectset); // watch stdin (fd 0) for input
    				int ret = select(1, &selectset, NULL, NULL, &timeout);
    				if(ret == 0) {
    					if(g_RequestedQuit)
    						goto END;  // time to go, main thread is waiting on this thread to stop
    					else
    						continue; // timed out waiting for input from stdin, try again
    				} else if (ret == -1 ){
    					MBTRACE(MBERROR, 1, "Failed to poll stdin for command line input (errno=%d)\n", errno);
    					goto END;
    				} else {
    					if (fgets(commandline, sizeof(commandline), stdin) == NULL) {
    						MBTRACE(MBINFO, 5, "stdin has closed, can no longer read commands from stdin (errno=%d)\n", errno);
    						goto END;
    					} else {
    						break;
    					}
    				}

    			}
    		}
        }

		command = commandline + strlen(commandline);

		while (command > commandline && (command[-1] == '\n' || command[-1] == '\r'))
			command--;

		*command = 0; /* set the null terminator at the end of the command string */
		command = commandline;

		while (*command == ' ') /* make sure the first character of command is not whitespace */
			command++;

		if (*command != 0) {
			command = strtok(command, " ");
			if (command) {
				subCmdInfo = NULL;
				ptrSubCmd = strtok(NULL, "\n");

				if (ptrSubCmd) {
					int cmdLen = strlen(ptrSubCmd);
					i = 0;

					while ((ptrSubCmd[i] == ' ') && (i < cmdLen))
						i++;

					if (i < cmdLen)
						subCmdInfo = &ptrSubCmd[i];
				}
			}
		} else
			subCmdInfo = NULL;

		/* ----------------------------------------------------------------------------
		 * Reset the following variables since they are used throughout this routine.
		 * ---------------------------------------------------------------------------- */
		i = 0;

		/* ----------------------------------------------------------------------------
		 * If user does not enter a command or the command is "stat", print the
		 * current statistics
		 * ---------------------------------------------------------------------------- */
		if (!*command || !strcmp(command, "stat")) {
			getStats(pMBInfo, OUTPUT_CONSOLE, 0, NULL);
		}

		/* ----------------------------------------------------------------------------
		 * If user enter a command "quit", set g_RequestedQuit = 1
		 * ---------------------------------------------------------------------------- */
		else if ((!strcmp(command, "quit")) ||
				((strlen(command) == 1) && (!strncmp(command, "q", 1)))) {
			pCmdLineArgs->disconnectType = DISCONNECT_WITH_UNSUBSCRIBE;
			g_RequestedQuit = 1;
			MBTRACE(MBINFO, 1, "Quit requested.....preparing to exit.\n");
			fprintf(stdout, "\nQuit requested.....preparing to exit.\n");
			break;
		}

		/* ----------------------------------------------------------------------------
		 * If user enter the command "close" or 'closeconn', set g_RequestedQuit = 1 and
		 * the flag disconnectType = DISCONNECT_WITHOUT_UNSUBSCRIBE so that all the connections are closed and
		 * MQTT Disconnect are the only 2 things performed.  The MQTT UnSubscribe is
		 * performed with quit.
		 * ----------------------------------------------------------------------------- */
		else if ((!strcmp(command, "close")) || (!strcmp(command, "closeconn"))) {
			pCmdLineArgs->disconnectType = DISCONNECT_WITHOUT_UNSUBSCRIBE;
			g_RequestedQuit = 1;
			MBTRACE(MBINFO, 1, "Close requested.....preparing to close all the connections.\n");
			fprintf(stdout, "\nClose requested.....preparing to close all the connections.\n");
			break;
		}

		/* ----------------------------------------------------------------------------
		 * If user enters the command "help", give options of commands
		 * ----------------------------------------------------------------------------- */
		else if ((!strcmp(command, "help")) ||
				((!strncmp(command, "h", 1)) && (strlen(command) == 1)) ||
				(!strncmp(command, "?", 1))) {
			provideHelp();
		}

		/*-----------------------------------------------------------------------------
		 * If user enters the command "nt", print out the total number of publish topics
		 * and subscriptions.
		 *----------------------------------------------------------------------------- */
		else if ((!strcmp(command, "nt"))) {
			int numSubs = 0;
			int numPubTopics = 0;
			for ( i=0 ; i < pMBInfo->mbCmdLineArgs->numSubmitterThreads ; i++ ) {
				numSubs 		+= mbInstArray[i]->numSubsPerThread;
				numPubTopics 	+= mbInstArray[i]->numPubTopicsPerThread;
			}

			fprintf(stdout, "Total publish topic count: %d\n", numPubTopics);
			fprintf(stdout, "Total subscription count: %d\n",  numSubs);
		} /* n and nt command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "clientTrace" with a regular expression to match
		 * one or more client ID, and a "enable" or "disable" keyword to indicate whether
		 * to enable or disable client tracing for the matched clients
		 *----------------------------------------------------------------------------- */
		else if ((!strcmp(command, "clientTrace"))) {
			if (subCmdInfo == NULL) {
				/* Provide user with additional information by putting to stdout. */
				fprintf(stdout,	"Must specify a regular expression to match client IDs and \"enable\" or \"disable\" to "\
						        "indicate whether to enable or disable low level trace on the matching clients.  For example, "\
								"clientTrace ^d:.* enable\n");
				fflush(stdout);
				continue;
			} else {
				int enableTrace = 0, nrMatches = 0;
				ism_regex_t regex;
				char *savePtr = NULL, *flag = NULL;
				char *clientIDRegex = strtok_r(subCmdInfo, " ", &savePtr);
				if (clientIDRegex) {
					flag = strtok_r(NULL, " ", &savePtr);
				}

				if(clientIDRegex == NULL || flag == NULL || !(strcmp(flag,"enable") == 0 || strcmp(flag,"disable") == 0)) {
					fprintf(stdout,	"Must specify a regular expression to match client IDs and \"enable\" or \"disable\" to "\
									"indicate whether to enable or disable low level trace on the matching clients.  For example, "\
									"clientTrace ^d:.* enable\n");
					fflush(stdout);
					continue;
				}

				if(strcmp(flag, "enable") == 0){
					enableTrace = 1;
				}

				rc = ism_regex_compile(&regex, clientIDRegex);
				if(rc){
					fprintf(stdout, "The provided regex string %s, is not a valid regular expression (regex rc=%d)\n", clientIDRegex, rc);
					fflush(stdout);
					continue;
				}

				/* Go through the submitter threads attempting to match clients using regular expression */
				for (int thrdNum = 0; thrdNum < numSubmitThrds; thrdNum++) {
					ism_common_list_iter_init(pClientIter, mbInstArray[thrdNum]->clientTuples);
					while ((currClient = ism_common_list_iter_next(pClientIter)) != NULL) {
						client = (mqttclient_t *) (currClient->data);
						if(client->clientIDLen <= 0){
							continue; // no point in trying to match on a zero length client ID string
						}
						int match = ism_regex_match(regex, client->clientID);
						if(match == 0) { // found a match, update the traceEnabled flag
							client->traceEnabled = enableTrace;
							nrMatches++;
						}
					} /* while ((currClient */

					ism_common_list_iter_destroy(pClientIter);
				} /* for ( m = 0 ; m < numThrds ; m++ ) */

				if (nrMatches == 0)
					fprintf(stdout, "No clients found matching regular expression %s\n", clientIDRegex);
				else {
					fprintf(stdout, "The low level trace setting was set to %s on %d clients\n", flag, nrMatches);
					MBTRACE(MBINFO, 5, "The low level trace setting was set to %s on %d clients\n", flag, nrMatches);
				}
				fflush(stdout);
				ism_regex_free(regex); // free regex object
				continue;
			}
		} /* clientTrace command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "c" with client ID, display connection information,
		 * transport & protocol states and Msg counts for the client.
		 *----------------------------------------------------------------------------- */
		else if (!strcmp(command, "c")) {    /* show list of msg count for all msgTypes */
			int destIdx;
			int needDetail = 0;
			int showClientID = 0;
			int showClientDetail = 0;
			int showClientIDHdr = 0;
			int showTopicHdr = 0;
			int thrdNum;

			char *dumpParm = NULL;
			char *topicParm = NULL;
			char *savePtr;
			char *clientID;

			/* Flags */
			uint8_t clientMatch = 0;
			uint8_t clientWildcard = 0;
			uint8_t foundTopic = 0;

			client = NULL;   /* Reset the client to NULL */

			if (subCmdInfo) {
				ism_regex_t clientIDRegex = NULL;
				ism_regex_t topicNameRegex = NULL;
				/* --------------------------------------------------------------------
				 * Determine what is being requested:
				 *    - Display all the client IDs
				 *    - Dump a specific client information
				 * -------------------------------------------------------------------- */
				clientID = strtok_r(subCmdInfo, " ", &savePtr);
				if (clientID) {
					if (strcmp(clientID, "*") == 0) {
						clientID = subCmdInfo;
						clientWildcard = 1;
					} else {
						rc = ism_regex_compile(&clientIDRegex, clientID);
						if(rc){
							fprintf(stdout, "The provided client ID regex string %s, is not a valid regular expression (regex rc=%d)\n",
									        clientID, rc);
							fflush(stdout);
							continue;
						}
						tmp = strtok_r(NULL, " ", &savePtr);
						if (tmp) {
							if (strcmp(tmp, "dump") != 0) {
								topicParm = tmp;
								rc = ism_regex_compile(&topicNameRegex, topicParm);
								if (rc) {
									fprintf(stdout, "The provided topic name regex string %s, is not a valid regular expression (regex rc=%d)\n",
											         topicParm, rc);
									fflush(stdout);
									continue;
								}
								dumpParm = strtok_r(NULL, " ", &savePtr);
								if ((dumpParm) && (strcmp(dumpParm, "dump") == 0))
									needDetail = 1;
							} else
								needDetail = 1;
						}
					}
				} else {
					/* Provide user with additional information by putting to stdout. */
					fprintf(stdout, "Must specify a client ID (c <clientID>  -OR-  c *).\n");
					fflush(stdout);
					continue;
				}

				/* Go through the submitter threads searching for the specific connection. */
				for ( thrdNum = 0 ; thrdNum < numSubmitThrds ; thrdNum++ ) {
					if (mbInstArray[thrdNum]) {
						ism_common_list_iter_init(pClientIter, mbInstArray[thrdNum]->clientTuples);

						while ((currClient = ism_common_list_iter_next(pClientIter)) != NULL) {
							client = (mqttclient_t *)(currClient->data);

							if ((clientWildcard == 1) || (ism_regex_match(clientIDRegex, client->clientID) == 0)) {
								clientMatch = 1;

								if (needDetail == 0) {
									if (topicParm == NULL)
										doDisplayClientInfo(client, showClientIDHdr++, 0);
									else { /* topicParm != NULL */
										showTopicHdr = 0;
										showClientID = 0;

										/* Handle wildcard subscription stats */
										if (client->destRxListCount > 0) {
											for ( destIdx = 0, showTopicHdr = 0 ; destIdx < client->destRxListCount ; destIdx++ ) {
												if (ism_regex_match(topicNameRegex, client->destRxList[destIdx]->topicName) == 0) {
													foundTopic = 1;
													if (showClientID++ == 0)
														doDisplayClientInfo(client, showClientIDHdr++, 0);

													doDisplayTopicInfo (client, showTopicHdr++, client->destRxList[destIdx], SUB, needDetail);
												}
											} /* for ( destIdx = 0, topicHdr = 0 ; destIdx < client->destRxListCount ; destIdx++ ) */
										} /* if (client->destRxListCount > 0) */

										showTopicHdr = 0;

										/* See if there are any Published Topics (TX) and if so then display the msg counts. */
										if (client->destTxListCount > 0) {
											for ( destIdx = 0, showTopicHdr = 0 ; destIdx < client->destTxListCount ; destIdx++ ) {
												if (ism_regex_match(topicNameRegex, client->destTxList[destIdx]->topicName) == 0) {
													foundTopic = 1;
													if (showClientID++ == 0)
														doDisplayClientInfo(client, showClientIDHdr++, 0);

													doDisplayTopicInfo(client, showTopicHdr++, client->destTxList[destIdx], PUB, needDetail);
												}
											} /* for ( destIdx = 0, topicHdr = 0 ; destIdx < client->destTxListCount ; destIdx++ ) */
										} /* if (client->destTxListCount > 0) */

									}
								} else {
									showClientDetail = 0;
									showTopicHdr = 0;
									foundTopic = 0;

									/* See if there are any Subscriptions (RX) and if so then display the msg counts. */
									if (client->destRxListCount > 0) {
										for ( destIdx = 0, showTopicHdr = 0 ; destIdx < client->destRxListCount ; destIdx++ ) {
											if ((topicParm == NULL) ||
												(ism_regex_match(topicNameRegex, client->destRxList[destIdx]->topicName) == 0)) {
												foundTopic = 1;
												if (showClientDetail++ == 0)
													doDisplayClientDetail(client, numThreadsPub);

												doDisplayTopicInfo(client, showTopicHdr++, client->destRxList[destIdx], SUB, needDetail);
											}
										} /* for ( destIdx = 0, topicHdr = 0 ; destIdx < client->destRxListCount ; destIdx++ ) */
									} /* if (client->destRxListCount > 0) */

									showTopicHdr = 0;

									/* See if there are any Published Topics (TX) and if so then display the msg counts. */
									if (client->destTxListCount > 0) {
										for ( destIdx = 0, showTopicHdr = 0 ; destIdx < client->destTxListCount ; destIdx++ ) {
											if ((topicParm == NULL) ||
												(ism_regex_match(topicNameRegex, client->destTxList[destIdx]->topicName) == 0)) {
												foundTopic = 1;
												if (showClientDetail++ == 0)
													doDisplayClientDetail(client, numThreadsPub);

												doDisplayTopicInfo(client, showTopicHdr++, client->destTxList[destIdx], PUB, needDetail);
											}
										} /* for ( destIdx = 0, topicHdr = 0 ; destIdx < client->destTxListCount ; destIdx++ ) */
									} /* if (client->destTxListCount > 0) */

									fprintf(stdout, "\n");

									if (foundTopic == 0) {
										doDisplayClientDetail(client, numThreadsPub);
										fprintf(stdout, "There are no matching topics: %s\n", topicParm);
									}

									fprintf(stdout, "---------------------------------------\n");

								} /* if (detailReq == 0) */
							} /* if ((clientWildcard == 1) || (strstr(client->clientID, clientID) > 0)) */
						} /* while ((currClient */

						ism_common_list_iter_destroy(pClientIter);
					} /*  if (mbInstArray[thrdNum]) */
				} /* for ( m = 0 ; m < numThrds ; m++ ) */

				if (clientMatch == 0)
					fprintf(stdout, "No match for specified client ID: %s\n", clientID);
				else {
					if ((foundTopic == 0) && (topicParm) && (needDetail == 0))
						fprintf(stdout, "No match for specified client ID: %s with topic: %s\n",
								        clientID,
								        topicParm);
				}

				if(clientIDRegex != NULL)
					ism_regex_free(clientIDRegex);
				if(topicNameRegex != NULL)
					ism_regex_free(topicNameRegex);

			} else {
				fprintf(stdout, "Correct format for the 'c' command is one of the following:\n" \
						        "\tc *\n" \
						        "\tc [clientID]\n" \
						        "\tc [clientID] dump\n" \
						        "\tc [clientID [topic]]\n" \
						        "\tc [clientID [topic]] dump\n");
			}
		} /* c command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "badclient" then display the list of clients that match
		 * the badrccount threshold
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "badclient") == 0) {
			int mincount = 0;
			if (subCmdInfo != NULL) {
				mincount = atoi(subCmdInfo);
			}

			MBTRACE(MBINFO, 5, "The badclient command invoked with param: %s\n", subCmdInfo);
			fprintf(stdout,"Clients with at least %d occurrence(s) of non-zero MQTT return codes:\n", mincount);
			for (int j=0; j < pCmdLineArgs->numSubmitterThreads; j++){
				ism_common_listIterator iter;
				ism_common_list *clients = pMBInfo->mbInstArray[i]->clientTuples;
				ism_common_list_iter_init(&iter, clients);
				ism_common_list_node *currTuple;
				while ((currTuple = ism_common_list_iter_next(&iter)) != NULL) {
					mqttclient_t *clientObj = (mqttclient_t *) currTuple->data;
					if(clientObj->badRxRCCount >= mincount || clientObj->badTxRCCount >= mincount){
						fprintf(stdout, "client %s (line=%d) badRxRCCount=%llu badTxRCCount=%llu\n",
								        clientObj->clientID, clientObj->line, (ULL) clientObj->badRxRCCount, (ULL) clientObj->badTxRCCount);
					}
				}
			}
		} /* badclient command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "printjobs" then display the list of jobs for the
		 * specified IOP thread
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "printjobs") == 0) {
			if (subCmdInfo != NULL) {
				int iopidx = atoi(subCmdInfo);
				int numIOP = (int) pSysEnvSet->numIOProcThreads;
				if (iopidx >= numIOP) {
					fprintf(stdout, "The specified IOP thread %d does not exist, must specify a valid IOP thread number(max=%d)\n", iopidx, numIOP-1);
				} else {
					ioProcThread_t *iop = ioProcThreads[iopidx];
					ioProcJobsList *currentJobsList = iop->currentJobs;
					int numJobs = currentJobsList->used;
					fprintf(stdout, "job #   tstate  pstate  events  trans              Client ID(line)\n");
					for (int j = 0 ; j < numJobs ; j++ ) {
						const char *clientID = "N/A";
						int line = -1;
						int pstate = 0xDEAD;
						int tstate = 0xDEAD;
						ioProcJob * job = currentJobsList->jobs + j;
						uint32_t events = job->events;
						transport_t * trans = job->trans;
						if (trans) {
							mqttclient_t *clientObj = trans->client;
							tstate = trans->state;
							if (clientObj) {
								clientID = clientObj->clientID;
								line = clientObj->line;
								pstate = clientObj->protocolState;
							}
						}
						fprintf(stdout, "%-7d 0x%-4x  0x%-4x  0x%-4x  %-18p %s(%d)\n",
										j, tstate, pstate, events, trans, clientID, line);
					}
				}
			} else {
				fprintf(stdout, "No IOP thread number was specified, must specify an IOP thread number (e.g. printjobs 1)\n");
			}
		}

		/*-----------------------------------------------------------------------------
		 * If user enters the command "sidchk" then display the list of streams that match
		 * the mincount threshold
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "sidchk") == 0) {
			int mincount = 0;
			if (subCmdInfo != NULL) {
				mincount = atoi(subCmdInfo);
			}

			if((pCmdLineArgs->chkMsgMask & CHKMSGSEQNUM) > 0) {
				MBTRACE(MBINFO, 5, "The sidchk command invoked with param: %s\n", subCmdInfo);
				fprintf(stdout,"Matching Message Streams:\n");
				for (int j=0; j < pSysEnvSet->numIOProcThreads; j++){
					ism_common_enumerateHashMap(ioProcThreads[j]->streamIDHashMap, streamMapEnumCB, &mincount);
				}
			} else {
				fprintf(stdout, "The sidchk command is only valid when the '-V 0x2' command line parameter is specified\n");
				MBTRACE(MBINFO, 5, "The sidchk command is only valid when the '-V 0x2' command line parameter is specified\n");
			}

		} /* sidchk command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "findsid" then display the client ID of the publisher
		 * that owns the message stream identified by the stream ID
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "findsid") == 0) {
			if (subCmdInfo != NULL) {
				char *sid = subCmdInfo;
				MBTRACE(MBINFO, 5, "The findsid command invoked with param: %s\n", sid);
				int cont = 1; // continue searching
				for (int j=0; j < pCmdLineArgs->numSubmitterThreads && cont; j++){
					ism_common_listIterator iter;
					ism_common_list *clients = pMBInfo->mbInstArray[i]->clientTuples;
					ism_common_list_iter_init(&iter, clients);
					ism_common_list_node *currTuple;
					while ((currTuple = ism_common_list_iter_next(&iter)) != NULL && cont) {
						mqttclient_t *pubClient = (mqttclient_t *) currTuple->data;
						for(int k=0; k < pubClient->destTxListCount && cont ; k++) {
							destTuple_t *publishTopic = pubClient->destTxList[k];
							if(!strcmp(publishTopic->streamIDStr, sid)) {
								cont = 0;
								fprintf(stdout, "The message stream %s is owned by publishing client %s (line=%d)\n", sid, pubClient->clientID, pubClient->line);
							}
						}
					}
				}
			} else {
				fprintf(stdout, "The findsid command requires a stream ID as an argument (e.g. findsid 1 or findsid 2345521)\n");
				MBTRACE(MBERROR, 5, "The findsid command requires a stream ID as an argument (e.g. findsid 1 or findsid 2345521)\n");
			}
		} /* findsid command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "findtopic" then display the client ID of the publishers/subscribers
		 * that publish/subscribe to topicStr
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "findtopic") == 0) {
			if (subCmdInfo != NULL) {
				char *topicStr = subCmdInfo;
				MBTRACE(MBINFO, 5, "The findtopic command invoked with param: %s\n", topicStr);
				for (int j=0; j < pCmdLineArgs->numSubmitterThreads; j++){
					ism_common_listIterator iter;
					ism_common_list *clients = pMBInfo->mbInstArray[i]->clientTuples;
					ism_common_list_iter_init(&iter, clients);
					ism_common_list_node *currTuple;
					while ((currTuple = ism_common_list_iter_next(&iter)) != NULL) {
						mqttclient_t *clientObj = (mqttclient_t *) currTuple->data;
						for(int k=0; k < clientObj->destTxListCount; k++) {
							destTuple_t *publishTopic = clientObj->destTxList[k];
							if(!strcmp(publishTopic->topicName, topicStr)) {
								fprintf(stdout, "The topic %s is published to by client %s (line=%d)\n", topicStr, clientObj->clientID, clientObj->line);
							}
						}
						for(int k=0; k < clientObj->destRxListCount; k++) {
							destTuple_t *subscription = clientObj->destRxList[k];
							if(!strcmp(subscription->topicName, topicStr)) {
								fprintf(stdout, "The topic %s is subscribed to by client %s (line=%d)\n", topicStr, clientObj->clientID, clientObj->line);
							}
						}
					}
				}
			} else {
				fprintf(stdout, "The findtopic command requires a topic string as an argument (e.g. findtopic /this/is/my/topic or findtopic topic123)\n");
				MBTRACE(MBERROR, 5, "The findtopic command requires a topic string as an argument (e.g. findtopic /this/is/my/topic or findtopic topic123)\n");
			}
		} /* findtopic command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "lse" then display the last 10 socket errors for
		 * each IOP.
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "lse") == 0) {
			int sockIdx;
			int numMsgs;

			char *tmpErrorArray[MAX_NUM_SOCKET_ERRORS];

			if (subCmdInfo == NULL)
				numMsgs = MAX_NUM_SOCKET_ERRORS;
			else {
				numMsgs = atoi(subCmdInfo);
				if (numMsgs > MAX_NUM_SOCKET_ERRORS)
					numMsgs = MAX_NUM_SOCKET_ERRORS;
			}

			if (numMsgs == 1)
				fprintf(stdout, "\nLast Socket Error per IOP ::\n");
			else
				fprintf(stdout, "\nLast %d Socket Errors per IOP ::\n", numMsgs);

			for ( i = 0 ; i < pSysEnvSet->numIOProcThreads ; i++ ) {
				fprintf(stdout, "   IOP %d\n", i);

				rc = obtainSocketErrors(&tmpErrorArray[0], i);
				if(rc) {
					fprintf(stdout, "Unable to obtain socket errors\n");
				}
				for ( sockIdx=0 ; sockIdx < numMsgs ; sockIdx++ ) {
					if ((tmpErrorArray[sockIdx])[0] != '\0')
						fprintf(stdout, "\t%s\n", tmpErrorArray[sockIdx]);
				}
			}

		} /* lse command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "l" or "al", get latency for each type of latency
		 * (-T <mask>) by going through all the producers and consumers.
		 *----------------------------------------------------------------------------- */
		else if ((strcmp(command, "l") == 0) || (strcmp(command, "al") == 0))
		{
			mask = 0x1;

			if (strcmp(command, "al") == 0)
				aggregateOnly = 1;

			if (latencyMask) {
				for ( i = 0 ; i < LAT_MASK_SIZE ; mask <<= 1, i++ ) {
					if ((mask & latencyMask) > 0) {
						calculateLatency(pMBInfo, mask, OUTPUT_CONSOLE, aggregateOnly, NULL, NULL);
					}
				}
			} else
				MBTRACE(MBERROR, 1, "Must be performing latency testing to get latency statistics (-T not set).\n");
				fprintf(stdout, "\nMust be performing latency testing to get latency statistics (-T not set).\n");
		} /* l or al command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "env" they are requesting the environment settings
		 *----------------------------------------------------------------------------- */
		else if (!strcmp(command, "env")) {
			provideEnvInfo (INTERACTIVESHELL, DISPLAY_STDOUT, pMBInfo);
		} /* env command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "rate=", set the new publish rate.
		 *----------------------------------------------------------------------------- */
		else if ((!strncmp(command, "rate=", 5)) || (!strcmp(command, "r"))) {
			tmp = strtok(command, "=");
			tmp = strtok(NULL, "=");

			if (tmp) {
				if (mqttClientType == CONSUMERS_ONLY) {
					MBTRACE(MBWARN, 6, "Rate control is not available for consumers.\n");
					fprintf(stdout, "Rate control is not available for consumers.\n");
				} else if (pMBInfo->burstModeEnabled) {
					MBTRACE(MBWARN, 6, "Unable to change message rate when burst mode is enabled.\n");
					fprintf(stdout, "Unable to change message rate when burst mode is enabled.\n");
				} else {
					if (validateRate(tmp) == 1) {
						newRate = atoi(tmp);
						doChangeMessageRate(newRate, pMBInfo, DOCOMMANDS);
					} else {
						MBTRACE(MBWARN, 6, "The rate specified is not a valid number: %s\n", tmp);
						fprintf(stdout, "The rate specified is not a valid number: %s\n", tmp);
					}
				}
			} else
				fprintf(stdout, "The proper rate change invocation is:  rate=x\n");
		} /* rate command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "lstate" then it will provide the states available
		 * for "pstate" and "tstate"
		 *----------------------------------------------------------------------------- */
		else if (!strcmp(command, "lstate")) {         /* show list of states supported */
			int ptIdx;

			fprintf(stdout, "\nSupported states for 'tstate' and 'pstate' commands:\n\n");
			fprintf(stdout, "\t%-24s%-11s%-30s%s\n", "transport state", "hex val", "protocol state", "hex val");
			fprintf(stdout, "\t%-24s%-11s%-30s%s\n",
					        "--------------------",
					        "-------",
					        "--------------------------",
					        "-------");

			for ( ptIdx = 0 ; ptIdx < numTransportStates ; ptIdx++ ) {
				if (ptIdx < numProtocolStates) {
					fprintf(stdout, "\t%-24s%#7x %s%-30s%#7x\n",
						            supportedTransportStates[ptIdx],
						            transportStateValues[ptIdx],
						            "   ",
						            supportedProtocolStates[ptIdx],
						            protocolStateValues[ptIdx]);
				} else
					fprintf(stdout, "\t%-24s%#7x\n",
						            supportedTransportStates[ptIdx],
						            transportStateValues[ptIdx]);
			}

		} /* lstate command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "freemem" then display the amount of free memory.
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "freemem") == 0) {
			getAmountOfFreeMemory(DISPLAY_DATA);
		}

		/*-----------------------------------------------------------------------------
		 * If user enters the command "lse" then display the last 10 socket errors for
		 * each IOP.
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "jitter") == 0) {
			char *subCmd;

			/* Flags */
			uint8_t resizeArray = 0;
			uint8_t enablingJitter = 0;

			if (latencyMask != 0) {
				fprintf(stderr, "\nJitter can not be enabled while Latency testing is being performed.\n");
				continue;
			}

			rc = 0;

			if (pCmdLineArgs->determineJitter == 1) {
				if (subCmdInfo) {
					if ((strcmp(subCmdInfo, "results") == 0) ||
						(strcmp(subCmdInfo, "report") == 0)) {
						while (g_JitterLock) {
							ism_common_sleep(SLEEP_1_MSEC);
						}

						g_JitterLock = 1;
						showJitterInfo(mqttClientType, pJitterInfo);
						g_JitterLock = 0;
					} else if (strcmp(subCmdInfo, "reset") == 0) {
						while (g_JitterLock) {
							ism_common_sleep(SLEEP_1_MSEC);
						}

						g_JitterLock = 1;
						resetJitterInfo(mqttClientType, pJitterInfo);
						g_JitterLock = 0;
					} else if (strcmp(subCmdInfo, "settings") == 0) {
						fprintf(stdout, "Jitter Interval: %lu (ms)  Sample Count: %d\n",
								        pJitterInfo->interval,
								        pJitterInfo->sampleCt);
					} else
						resizeArray = 1;
       			} else
       				showJitterInfo(mqttClientType, pJitterInfo);
			} else {
				if (subCmdInfo == NULL) {
       				fprintf(stdout, "Jitter statistics are not enabled.\n");
				} else {
           			if ((strcmp(subCmdInfo, "results") == 0) ||
           				(strcmp(subCmdInfo, "report") == 0) ||
           				(strcmp(subCmdInfo, "reset") == 0) ||
           				(strcmp(subCmdInfo, "settings") == 0)) {
           				fprintf(stdout, "Jitter statistics are not enabled.\n");
	           		} else
						enablingJitter = 1;
				}
			}

			/* Check to see if enabling jitter or resizing the jitter array. */
			if ((enablingJitter == 1) || (resizeArray == 1)) {
	           	int jitterInterval = MB_JITTER_INTERVAL;
	           	int jitterSampleCt = MB_JITTER_SAMPLECT;

       			subCmd = strtok(subCmdInfo, " ");
				jitterInterval = atoi(subCmd);
				subCmd = strtok(NULL, " ");
				if (subCmd)
					jitterSampleCt = atoi(subCmd);

           		if (enablingJitter == 1) {
           			int tmpVal;

           			pJitterInfo = calloc(1, sizeof(mbJitterInfo_t));
           			if (pJitterInfo) {
           				pMBInfo->mbJitterInfo = pJitterInfo;
           				pJitterInfo->interval = jitterInterval;
           				pJitterInfo->sampleCt = jitterSampleCt;
           				pJitterInfo->rxQoSMask = -1;
           				pJitterInfo->txQoSMask = -1;
           				pCmdLineArgs->determineJitter = 1;

           				if ((mqttClientType & CONTAINS_CONSUMERS) > 0) {
           					tmpVal = getAllThreadsQoS(CONSUMER, pMBInfo);
           					if (tmpVal > 0)
           						pJitterInfo->rxQoSMask = (int8_t)tmpVal;
           				}
           				if ((mqttClientType & CONTAINS_PRODUCERS) > 0) {
           					tmpVal = getAllThreadsQoS(PRODUCER, pMBInfo);
           					if (tmpVal > 0)
           						pJitterInfo->txQoSMask = (int8_t)tmpVal;
           				}

           				rc = provisionJitterArrays(CREATION, mqttClientType, pJitterInfo, mbInstArray);
           				if (rc == 0) {
           					rc = addJitterTimerTask(pMBInfo);
           					fprintf(stdout, "Jitter statistics enabled.\n");
           				} else
           					fprintf(stdout, "Unable to create the jitter arrays (rc = %d).\n", rc);
           			} else
           		    	rc = provideAllocErrorMsg("the structure to measure jitter", (int)sizeof(mbJitterInfo_t), __FILE__, __LINE__);
            	} else if (resizeArray == 1) {
            		if (subCmd == NULL) {
               			fprintf(stdout, "To resize jitter histogram:  jitter [int] [sample #]\n");
            		} else {
            			/* ------------------------------------------------------------
            			 * This is a reallocation of the jitter array based on the users
            			 * input and requires the following to occur:
            			 *
            			 *  1. Cancel the current timer task
            			 *  2. Reallocate the current array
            			 *  3. Update the structure that contains the intervals and count
            			 *  4. Reset the array.
            			 * ------------------------------------------------------------ */
            			/* If the sample count is different then need to reallocate the array. */
            			if (pJitterInfo->sampleCt != jitterSampleCt) {
               				pJitterInfo->sampleCt = jitterSampleCt;
               				g_JitterLock = 1;

               				rc = provisionJitterArrays(REALLOCATION, mqttClientType, pJitterInfo, mbInstArray);
               				if (rc == 0) {
               					fprintf(stdout, "Successfully adjusted jitter sample count: %d\n", jitterSampleCt);
               				} else {
               					fprintf(stdout, "Unable to resize the jitter array.\n");
               				}
    						resetJitterInfo(mqttClientType, pJitterInfo);

               				g_JitterLock = 0;
            			}

            			/* If the interval is different then cancel the current timer task. */
            			if ((pJitterInfo->interval != jitterInterval) && (rc == 0)) {
           					pJitterInfo->interval = jitterInterval;

           					rc = ism_common_cancelTimer(pJitterInfo->timerTask);
           					if (rc)
           						fprintf(stdout, "Something wrong.....Jitter timer task not located.\n");

           					rc = addJitterTimerTask(pMBInfo);

           					fprintf(stdout, "Successfully adjusted Jitter interval %d (ms)\n", jitterInterval);
            			}
            		}
            	}
           	}
		} /* jitter command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "pstate" or "tstate" with valid option then
		 * provide the client IDs that have the client->protocolState or the
		 * trans->state set as requested respectively.
		 *----------------------------------------------------------------------------- */
		else if ((!strcmp(command, "pstate")) || (!strcmp(command, "tstate"))) {
			int ptIdx;
			int requestedState = -1;
			int type_Transport_Client = 0;
			int numThrd;

			char *mState = subCmdInfo;

			transport_t *trans = NULL;

			found = 0;  /* Reset the found flag. */

			client = NULL;   /* Reset the client to NULL */

			if (mState) {
				if (!strncmp(command, "pstate", 6)) {
					type_Transport_Client = CLIENT;

					for ( ptIdx = 0 ; ptIdx < numProtocolStates ; ptIdx++ ) {
						if (!strcmp(mState, supportedProtocolStates[ptIdx])) {
							requestedState = protocolStateValues[ptIdx];
							break;
						}
					}

					if (requestedState == -1) {
						fprintf(stdout, "Unsupported state for Client: %s\n", mState);
						fflush(stdout);
						continue;
					}
				} else {
					type_Transport_Client = TRANSPORT;

					for ( ptIdx = 0 ; ptIdx < numTransportStates ; ptIdx++ ) {
						if (!strcmp(mState, supportedTransportStates[ptIdx])) {
							requestedState = transportStateValues[ptIdx];
							break;
						}
					}

					if (requestedState == -1) {
						MBTRACE(MBERROR, 1, "Unsupported state for transport:  %s\n", mState);
						fprintf(stdout, "Unsupported state for transport:  %s\n", mState);
						fflush(stdout);
						continue;
					}
				}

				/* Go through the submitter threads checking the states of all connections. */
				if (mbInstArray) {
					for ( numThrd = 0 ; numThrd < numSubmitThrds ; numThrd++ ) {
						if (mbInstArray[numThrd]) {
							ism_common_list_iter_init(pClientIter, mbInstArray[numThrd]->clientTuples);

							while ((currClient = ism_common_list_iter_next(pClientIter)) != NULL) {
								if (type_Transport_Client == CLIENT) {
									client = (mqttclient_t *)(currClient->data);

									if (client->protocolState & requestedState) {
										if (found == 0) {
											found = 1;
											fprintf(stdout, "ClientID(s) with protocol state %s ::\n", mState);
										}

										fprintf(stdout, "\t%s\n", client->clientID);
									}
								} else {
									client = (mqttclient_t *)(currClient->data);
									trans = client->trans;

									if (trans->state & requestedState) {
										if (found == 0) {
											found = 1;
											fprintf(stdout, "ClientID(s) with transport state %s ::\n", mState);
										}

										fprintf(stdout, "\t%s\n", client->clientID);
									}
								}
							}  /* while currClient */

							ism_common_list_iter_destroy(pClientIter);
						} /* if (mbInstArray[thrdNum]) */
					}  /* for ( m = 0 ; m < numSubmitThrds ; m++ ) */
				} /* if (mbInstArray) */
			} else
				fprintf(stdout, "Must specify a valid state.\n");
		} /* pstate and tstate commands */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "m" with valid option then provide the client ID
		 * associated with it.  Currently only supporting CONNACK
		 *----------------------------------------------------------------------------- */
		else if (!strcmp(command, "m")) { /* show list of clients missing a message type. */
			int requestedState = -1;
			int numThrd;
			char *mType = subCmdInfo;

			found = 0;  /* Reset the found flag */

			client = NULL;   /* Reset the client to NULL */

			if (mType) {
				if (strcmp(mType, "CONNACK") == 0)
					requestedState = CONNACK;

				if (requestedState != -1) {
					/* Go through the connections searching for the desired state. */
					if (mbInstArray) {
						for ( numThrd = 0 ; numThrd < numSubmitThrds ; numThrd++ ) {
							if (mbInstArray[numThrd]) {
								ism_common_list_iter_init(pClientIter, mbInstArray[numThrd]->clientTuples);

								while ((currClient = ism_common_list_iter_next(pClientIter)) != NULL) {
									client = (mqttclient_t *)(currClient->data);

									if (client->currRxMsgCounts[requestedState] == 0) {
										if (found == 0) {
											found = 1;
											fprintf(stdout, "ClientID(s) with missing %s\n", mType);
										}

										fprintf(stdout, "   %s\n", client->clientID);
									}
								}  /* while currClient */

								ism_common_list_iter_destroy(pClientIter);
							} /* if (mbInstArray[thrdNum]) */
						}  /* for ( m = 0 ; m < numSubmitThrds ; m++ ) */
					} else
						fprintf(stderr, "(e) There are no submitter threads.\n");
				} else
					fprintf(stdout, "Supported type is: 'CONNACK'.\n");
			} else
				fprintf(stdout, "Must specify a valid type (m <type>). Currently only 'CONNACK' supported.\n");
		} /* m command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "slowacker" with a positive number of ACKS then
		 * provide the client ID which is not acking appropriately.
		 *----------------------------------------------------------------------------- */
		else if (!strcmp(command, "slowacker")) {
			/* show list of clients missing a message type. */
			int diffQoS1 = 0;
			int diffQoS2 = 0;
			int thrdNum;

			uint64_t numAcks = 0;
			char *cNumAcks;
			destTuple_t **pDestList = NULL;

			found = 0;   		/* Reset the found flag. */
			client = NULL;   	/* Reset the client to NULL */
			cNumAcks = subCmdInfo;

			if (cNumAcks) {
				char *endptr;
				numAcks = (uint64_t) strtoull(cNumAcks, &endptr, 10);
				if (numAcks > 0) {
					/* ----------------------------------------------------------------
					 * Go through the submitter threads and then through the consumer
					 * topics searching for cases where the number of ACKs is lagging
					 * ---------------------------------------------------------------- */
					if (mbInstArray) {
						for ( thrdNum = 0 ; thrdNum < numSubmitThrds ; thrdNum++ ) {
							if (mbInstArray[thrdNum]) {
								ism_common_list_iter_init(pClientIter, mbInstArray[thrdNum]->clientTuples);

								while ((currClient = ism_common_list_iter_next(pClientIter)) != NULL) {
									client = (mqttclient_t *)(currClient->data);
									pDestList = client->destRxList;
									found = 0;

									for ( i = 0 ; i < client->destRxListCount ; i++ ) {
										if (pDestList[i]->topicQoS == 0)
											continue;
										else {
											diffQoS1 = pDestList[i]->currQoS1MsgCount - pDestList[i]->currPUBACKCount;
											diffQoS2 = pDestList[i]->currQoS2MsgCount - pDestList[i]->currPUBRECCount;
										}

										if ((diffQoS1 >= numAcks) || (diffQoS2 >= numAcks)) {
											if (found++ == 0) {
												fprintf (stdout, "\nClientID: %s\n", client->clientID);
												fprintf (stdout, "   Topic:  ");
											} else
												fprintf (stdout, "           ");

											if ((diffQoS1 >= numAcks) && (diffQoS2 < numAcks)) {
												fprintf(stdout, "%s   # Pending Acks: %d (QoS1)\n",
																pDestList[i]->topicName,
											                    diffQoS1);
											} else if ((diffQoS1 < numAcks) && (diffQoS2 >= numAcks)) {
												fprintf(stdout, "%s   # Pending Acks: %d (QoS2)\n",
																pDestList[i]->topicName,
																diffQoS2);
											} else {
												fprintf(stdout, "%s   # Pending Acks: %d (QoS1)  %d (QoS2)\n",
																pDestList[i]->topicName,
											                    diffQoS1,
											                    diffQoS2);
											}
										}
									}
								} /* for ( i = 0 ; i < client->destRxListCount ; i++ ) */

								ism_common_list_iter_destroy(pClientIter);
							}  /* if (mbInstArray[thrdNum]) */
						}  /* for ( thrdNum = 0 ; thrdNum < numSubmitThrds ; thrdNum++ ) */
					} else
						fprintf(stdout, "slowacker is only available on consumers.\n");
				} else
					fprintf(stdout, "Must specify a valid number (> 0) of ACKS.\n");
			} else
				fprintf(stdout, "Must specify a # of ACKS (slowacker <# acks>), which is greater than 0.\n");
		} /* slowacker command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "resetlat" then schedule a job for each IOP to
		 * reset the latency statistics.
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "resetlat") == 0) {
			if (latencyMask > 0) {
				MBTRACE(MBINFO, 5, "Scheduled job to reset latency.\n");

				submitResetLatencyJob();

				fprintf(stdout, "\nResetting latency statistics.\n");
			} else {
				MBTRACE(MBERROR, 1, "Must be performing latency testing to reset the statistics.\n");
				fprintf(stdout, "\nMust be performing latency testing to reset the statistics.\n");
			}
		} /* resetlat command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "tl" or "tracelevel" then change the trace level
		 * to the requested level.
		 *----------------------------------------------------------------------------- */
		else if ((strcmp(command, "tracelevel") == 0) ||
			(strcmp(command, "tl") == 0)) {
			if (subCmdInfo != NULL) {
				int traceLevel = atoi(subCmdInfo);

				rc = setMBTraceLevel(traceLevel);
				if (rc) {
					MBTRACE(MBERROR, 1, "Invalid value specified for 'tracelevel' command.\n");
					fprintf(stdout, "Invalid value specified for 'tracelevel' command (valid values: 0-9).\n");
					fflush(stdout);
				} else /* Successfully set so modify the global */
					pCmdLineArgs->traceLevel = (uint8_t)traceLevel;
			} else {
				MBTRACE(MBERROR, 1, "No value specified for 'tracelevel' command.\n");
				fprintf(stdout, "No value specified for 'tracelevel' command (valid values: 0-9).\n");
				fflush(stdout);
			}
		} /* tracelevel or tl command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "uptime" then display the current runtime
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "uptime") == 0) {
			int secsDiff;
			time_t currTime;

			time(&currTime);

			secsDiff = currTime - beginTime.time;
			fprintf(stdout, " Days: %d  Hrs: %d  Mins: %d  Secs: %d\n\n",
					        ((secsDiff / 3600) / 24),
							((secsDiff / 3600) % 24),
							((secsDiff / 60) % 60),
							(secsDiff % 60));
		} /* uptime command */

		/*-----------------------------------------------------------------------------
		 * If user enters the command "ver" then display the version of mqttbench
		 *----------------------------------------------------------------------------- */
		else if (strcmp(command, "ver") == 0) {
			SHOWVERSION
		} /* ver command */

		/*-----------------------------------------------------------------------------
		 * Unknown command.
		 *----------------------------------------------------------------------------- */
		else {
			provideHelp();
			fprintf(stdout, "Unrecognized command.  See above for supported commands.\n\n");
		}

		fflush(stdout);
	} /* endless for loop */

END:
	/* Clean up memory */
	if (pClientIter)
		free(pClientIter);
	if (topicSendCount > 0)
		free(topicSendCount);
	if (topicRecvCount > 0)
		free(topicRecvCount);

	free(pThrdInfoDoCmds);

	return NULL;
} /* doCommands */
