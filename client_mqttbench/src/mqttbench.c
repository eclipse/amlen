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

#include <ismutil.h>
#include <protoex.h>

#include "mqttbench.h"
#include "mqttclient.h"
#include "mqttbenchrate.h"
#include "tcpclient.h"


/* ******************************** GLOBAL VARIABLES ********************************** */
/* Externs */
extern uint8_t g_RequestedQuit;
extern uint8_t g_StopIOPProcessing;
extern uint8_t g_StopProcessing;
extern uint8_t g_ClkSrc;

extern int16_t g_SubmitterExit;
extern int16_t g_WaitReady;

extern int g_AppSimLoop;
extern int g_ChkMsgMask;
extern int g_LatencyMask;
extern int g_MinMsgRcvd;
extern int g_MaxMsgRcvd;
extern int g_Equiv1Sec_Conn;
extern int g_Equiv1Sec_RTT;
extern int g_Equiv1Sec_Reconn_RTT;
extern int g_MBErrorRC;
extern int g_TxBfrSize;
extern int g_MBTraceLevel;

extern double g_EndTimeConnTest;

/* Rate related and/or Latency related */
extern double g_MsgRate;
extern double g_UnitsRTT;
extern double g_UnitsReconnRTT;

extern struct drand48_data randBuffer;
extern pthread_spinlock_t submitter_lock;
extern pthread_spinlock_t ready_lock;
extern pthread_spinlock_t mbinst_lock;
extern mqttbenchInfo_t *g_MqttbenchInfo;
extern const char *g_ReasonStrings[];

#define _INLINE    1
#define _UTF8      1

/* ******************************** GLOBAL VARIABLES ********************************** */

/* Globals - Initial declaration */
uint8_t g_AlertNoMsgIds   = 0;
int16_t g_TotalNumRetries_EXIT;
int g_SubmitThreadReady   = 0;
mqtt_prop_ctx_t *g_mqttCtx5;                  /* MQTTv5 Identifier/Value context */

/*
 * MQTT extended fields (CSD02)
 * This must match the table in pxmqtt.c
 */
mqtt_prop_field_t g_mqttPropFields[] = {
    { MPI_PayloadFormat,     MPT_Int1,     5, CPOI_PUBLISH | CPOI_WILLPROP,       "PayloadFormat"    },
    { MPI_MsgExpire,         MPT_Int4,     5, CPOI_PUBLISH | CPOI_WILLPROP,       "MessageExpire"    },
    { MPI_ContentType,       MPT_String,   5, CPOI_PUBLISH | CPOI_WILLPROP,       "ContentType"      },
    { MPI_ReplyTopic,        MPT_String,   5, CPOI_PUBLISH | CPOI_WILLPROP,       "ReplyTopic"       },
    { MPI_Correlation,       MPT_Bytes,    5, CPOI_PUBLISH | CPOI_WILLPROP,       "CorrelationData"  },
    { MPI_SubID,             MPT_VarInt,   5, CPOI_S_PUBLISH | CPOI_SUBSCRIBE | CPOI_ALT_MULTI,    "SubscriptionID"   },
    { MPI_SessionExpire,     MPT_Int4,     5, CPOI_CONNECT | CPOI_CONNACK | CPOI_C_DISCONNECT,     "SessionExpire"    },
    { MPI_ClientID,          MPT_String,   5, CPOI_CONNACK,                       "AssignedClientID" },
    { MPI_KeepAlive,         MPT_Int2,     5, CPOI_CONNACK,                       "ServerKeepAlive"  },
    { MPI_AuthMethod,        MPT_String,   5, CPOI_AUTH_CONN_ACK,                 "AuthenticationMethod" },
    { MPI_AuthData,          MPT_Bytes,    5, CPOI_AUTH_CONN_ACK,                 "AuthenticationData"   },
    { MPI_RequestReason,     MPT_Int1,     5, CPOI_CONNECT,                       "RequestReason"    },
    { MPI_WillDelay,         MPT_Int4,     5, CPOI_WILLPROP,                      "WillDelay"        },
    { MPI_RequestReplyInfo,  MPT_Int1,     5, CPOI_CONNECT,                       "RequestReplyInfo" },
    { MPI_ReplyInfo,         MPT_String,   5, CPOI_CONNACK,                       "ReplyInfo"        },
    { MPI_ServerReference,   MPT_String,   5, CPOI_CONNACK | CPOI_S_DISCONNECT,   "ServerReference" },
    { MPI_Reason,            MPT_String,   5, CPOI_ACK,                           "Reason"           },
    { MPI_MaxReceive,        MPT_Int2,     5, CPOI_CONNECT_CONNACK,               "MaxInflight"      },
    { MPI_MaxTopicAlias,     MPT_Int2,     5, CPOI_CONNECT_CONNACK,               "TopicAliasMaximum"},
    { MPI_TopicAlias,        MPT_Int2,     5, CPOI_PUBLISH,                       "TopicAlias"       },
    { MPI_MaxQoS,            MPT_Int1,     5, CPOI_CONNECT_CONNACK,               "MaximumQoS"       },
    { MPI_RetainAvailable,   MPT_Boolean,  5, CPOI_CONNACK,                       "RetainAvailable"  },
    { MPI_UserProperty,      MPT_NamePair, 5, CPOI_USERPROPS | CPOI_MULTI,        "UserProperty"     },
    { MPI_MaxPacketSize,     MPT_Int4,     5, CPOI_CONNECT_CONNACK,               "MaxPacketSize"    },
    { MPI_WildcardAvailable, MPT_Int1,     5, CPOI_CONNACK,                       "WildcardAvailable"},
    { MPI_SubIDAvailable,    MPT_Int1,     5, CPOI_CONNACK,                       "SubIDAvailable"   },
    { MPI_SharedAvailable,   MPT_Int1,     5, CPOI_CONNACK,                       "SharedAvailable"  },
    { 0, 0, 0, 0, NULL },
};

ism_enumList mb_MQTTMsgTypeEnum [] = {
	{ "Fields",			14,				},  // IMPORTANT: you must always update this field, when adding/removing items from enumList
    { "CONNECT",  		CONNECT,        },
    { "CONNACK",        CONNACK,		},
    { "PUBLISH",	    PUBLISH,		},
	{ "PUBACK",	        PUBACK,			},
	{ "PUBREC",	        PUBREC,			},
	{ "PUBREL",	        PUBREL,			},
	{ "PUBCOMP",	    PUBCOMP,		},
	{ "SUBSCRIBE",	    SUBSCRIBE,		},
	{ "SUBACK",	        SUBACK,			},
	{ "UNSUBSCRIBE",	UNSUBSCRIBE,	},
	{ "UNSUBACK",	    UNSUBACK,		},
	{ "PINGREQ",	    PINGREQ,		},
	{ "PINGRESP",	    PINGRESP, 		},
	{ "DISCONNECT",	   	DISCONNECT,		},
};

/*
 * Validate a message ID.
 * The message ID must be between 1 and 65535.
 * @param mmsg   A MQTT message object with the msdid field set
 * @return The mmsg return code which may now contain ISMRC_InvalidID
 */
/* *************************************************************************************
 * Verify that the message id is in the expected range
 *
 *   @param[in]  client              = mqtt client object to check the msg ID against
 *   @param[in]  msgID               = the message ID to check
 *   @param[in]  msgTypeStr          = the message Type that contained the message ID being checked
 *
 *   @return 0 if msg ID is within the acceptable range, non-zero if not a valid msg ID
 * *************************************************************************************/
static int chkmsgID(mqttclient_t *client, int msgId, const char *msgTypeStr) {
	int rc = 0;

	if(msgId < 1 || msgId > 65535) {
		MBTRACE(MBERROR, 1, "Received message type=%s, with invalid msg ID=%d, client %s (line=%d) has a MAX message ID=%d\n",
							msgTypeStr, msgId, client->clientID, client->line, client->maxInflightMsgs);
		rc = RC_MSGID_BAD;
	}

    return rc;
}

/* *************************************************************************************
 * decodeMQTTMessageLength
 *
 * Decodes the message length according to the MQTT algorithm
 *    @param buf    - a character buffer containing the remaining length.
 *    @param msglen - the decoded length returned
 *    @return       - the number of bytes read to determine the remaining length.
 * *************************************************************************************/
static inline int decodeMQTTMessageLength (char *buf, char *endBuf, int *msglen)
{
	int multiplier = 1;
	int ctr = 0;
	int len = 0;
	char *ptr = buf;
	char c;

	/* --------------------------------------------------------------------------------
	 * Decode the remaining length.
	 *
	 * NOTE: Will always read 1 byte.  If in the case where there is a partial packet
	 *       and it splits the remaining length then return PARTIAL_MQTT_MSG.
	 * -------------------------------------------------------------------------------*/
	while ((ptr < endBuf) && (++ctr < MQTT_MAX_NO_OF_REMAINING_LEN_BYTES)) {
		c = *ptr;
		ptr++;
		len += ((c & 0x7F) * multiplier);
		if ((c & 0x80) == 0) {
			*msglen = len;
			return (ctr);   /* Return the length of the remaining length */
		}

		multiplier = multiplier << 7;
	}

	/* --------------------------------------------------------------------------------
	 * Determine which return code to provide:
	 *    PARTIAL_MQTT_MSG = ptr has reached the end of the buffer, but the remaining
	 *                       length field is less than 4 bytes and unfinished
	 *    BAD_MQTT_MSG     = ptr has reached the end of the buffer, and the remaining
	 *                       length field is 4 bytes or greater.  This means we are
	 *                       totally lost since it can only be 4 bytes long.
	 * -------------------------------------------------------------------------------*/
	if ((ptr-buf) < MQTT_MAX_NO_OF_REMAINING_LEN_BYTES)
		return PARTIAL_MQTT_MSG;
	else
		return BAD_MQTT_MSG;

} /* decodeMQTTMessageLength */

/* *************************************************************************************
 * Prior to exiting a producer / consumer thread the following needs to be performed:
 * 	- Destroy the iterator structure
 *  - Free the memory used for the iterator
 *  - Destroy the buffer pool and set the mbinst value = NULL
 *  - Depending on the Consumer or Producer set the respective lock and
 *    increment the exit counter.
 *
 *   @param[in]  mbinst              = mqttbench_t structure for this thread.
 *   @param[in]  stopProcess         = Value to set g_StopProcessing flag (1 = Stop).
 *   @param[in]  errCleanUp          = RC reason for cleanup
 *   @param[in]  testDuration        = Duration (0 = Endless - Command Lane ; > 0 = num Secs)
 *   @param[in]  pClientIter         = Client Iterator to be free
 *
 * *************************************************************************************/
static inline void doThreadCleanUp (mqttbench_t *mbinst,
                                    int stopProcess,
                                    int errCleanUp,
									int testDuration,
                                    ism_common_listIterator *pClientIter)
{
	/* Destroy the iterator, free the memory, and lastly free the Transmit buffer Pool. */
	ism_common_list_iter_destroy(pClientIter);
	free(pClientIter);

	pthread_spin_lock(&submitter_lock);
	g_SubmitterExit++;
	pthread_spin_unlock(&submitter_lock);
	g_StopProcessing = (uint8_t)stopProcess;

	/* --------------------------------------------------------------------------------
	 * If doCommands is running (testDuration == 0) then sit here until the global
	 * flag ( ) is set which indicates the user typed in: quit.
	 * -------------------------------------------------------------------------------- */
	if (testDuration == 0) {
		while (g_RequestedQuit == 0) {
			ism_common_sleep(DISCONNECT_SLEEP_RETRY * SLEEP_1_USEC);
		}
	}

	mbinst->thrdHandle = 0;
	g_MBErrorRC = errCleanUp;
} /* doThreadCleanup */

/* *************************************************************************************
 * Search the client inflight message window for a message/packet IDs that matches the
 * desiredState.  Search up to maxSearch message IDs before giving up. Update the
 * client->availMsgId value while searching.  Caller must be holding the client lock
 *
 *   @param[in]  client              = mqttclient_t to search
 *   @param[in]  desiredState        = the message ID state that is being searched for
 *   @param[in]  maxSearch           = the max number of message IDs to search
 *
 *	@return RC_SUCCESSFUL			 = if an available message ID was found (or in an UNKNOWN state),
 *			RC_MSGID_NONE_AVAILABLE  = there are no available message IDs
 *			RC_MSGID_MAXSEARCH       = reached the max search limit
 * *************************************************************************************/
int findAvailableMsgId (mqttclient_t *client, int desiredState, int maxSearch){
	int rc = RC_MSGID_NONE_AVAILABLE;
	int searchLimit = MIN(maxSearch, client->allocatedInflightMsgs);		/* ensure that we don't check the same msg id more than once */

	if (client->currInflightMessages < maxSearch) {
		/* ----------------------------------------------------------------------------
		 * Check to see if the current available msgid is marked as being available.
		 * If not (and is not in UNKNOWN state), then search for one.
		 * ---------------------------------------------------------------------------- */
		msgid_info_t *msgIdInfo = &client->inflightMsgIds[client->availMsgId];
		if((msgIdInfo->state != desiredState) &&
		   ((msgIdInfo->state & PUB_MSGID_UNKNOWN_STATE) == 0)) {
			int searchCtr = 0;

			/* ------------------------------------------------------------------------
			 * If the available inFlightMsgIds = the 1st msgID + the maxInflightMsgs then
			 * reset the available msgID to the beginning and verify that is available.
			 * ------------------------------------------------------------------------ */
			while (1) {
				if(++client->availMsgId >= maxSearch) 	/* wrap back to msgID 1, if we reach the end of the window */
					client->availMsgId = 1;

				msgIdInfo = &client->inflightMsgIds[client->availMsgId];
				if((msgIdInfo->state == desiredState) ||
				   ((msgIdInfo->state & PUB_MSGID_UNKNOWN_STATE) > 0)) {
					g_AlertNoMsgIds = 0;
					return RC_SUCCESSFUL;
				}

				if (++searchCtr > searchLimit) {
					MBTRACE(MBDEBUG, 7, "Client %s at line %d has reached the max search limit for available msg IDs: (count=%d limit=%d)\n",
							            client->clientID, client->line, searchCtr, searchLimit);
					return RC_MSGID_MAXSEARCH;
				}
			} /* while (1) */
		} else {
			g_AlertNoMsgIds = 0;
			rc = RC_SUCCESSFUL;  /* found it on the first try, no search required */
		}
	} else {
		g_AlertNoMsgIds = 1;	/* Enable the global msgid alert flag, server cannot or will not ACK at this rate */
	}

	return rc;
}

/* *************************************************************************************
 * provideDecodeMessageData
 *
 * Description:  Provide data pertaining to the decode message error. Clear various data.
 *
 *   @param[in]  client              = Specific mqttclient to use.
 *   @param[in]  ptr                 = x
 *   @param[in]  pEndBuf             = x
 *   @param[in]  rxLen               = x
 *   @param[in]  header              = Header of the MQTT Message.
 * *************************************************************************************/
void provideDecodeMessageData (mqttclient_t *client,
		                       char *ptr,
		                       char *pEndBuf,
		                       int rxLen,
		                       Header *header)
{
	MBTRACE(MBINFO, 5, "ptr:  0x%p    pEndBuf: 0x%p\n" \
			           "partial bfr info:   bfr: 0x%p  bfr->used:  %d   bytesNeeded:  %d\n" \
					   "msg type: %d \n",
			    	   ptr,
					   pEndBuf,
                       client->partialMsg->buf,
                       client->partialMsg->used,
                       client->msgBytesNeeded,
	                   header->bits.type);

	/* Need to reset the partial message at this point and return. */
	client->msgBytesNeeded = 0;      /* Reset the # of bytes needed = 0 */
	client->partialMsg->used = 0;    /* Reset the partial length = 0 */

	/* Reset the putPt and getPtr to the beginning of the partial msg buffer. */
	client->partialMsg->putPtr = client->partialMsg->buf;
	client->partialMsg->getPtr = client->partialMsg->buf;
} /* provideDecodeMessageData */

/* *************************************************************************************
 * shutdownMQTTClients
 *
 * Description:  Perform a Shutdown on the MQTT Connections.  Performs either DISCONNECT
 *               only -OR- UNSUBSCRIBE to all connections with a subscription followed
 *               by a DISCONNECT.
 *
 *   @param[in]  mbinst              = Pointer to this thread's information (mqttbench_t)
 *   @param[in]  shutdownType        = Whether performing only DISCONNECT or both
 *                                     UNSUBSCRIBE and DISCONNECT.
 * *************************************************************************************/
int shutdownMQTTClients (mqttbench_t * mbinst, uint8_t shutdownType)
{
	int rc = 0;
	int count = 0;
	int retry = 0;
	int currNumSubscribers = 0;
	int numClients = mbinst->numClients;
	int reqState;
	int loopCt = 0;

	/* --------------------------------------------------------------------------------
	 * Check if there are any subscriptions associated with this thread and that this
	 * is NOT a DISCONNECT_ONLY.
	 * -------------------------------------------------------------------------------- */
	if ((mbinst->numSubsPerThread) && (shutdownType == UNSUBSCRIBE_W_DISCONNECT)) {
		MBTRACE(MBINFO, 1, "Submitter thread (#: %d) - Beginning MQTT UnSubscribe.\n", mbinst->id);

		/* Schedule an UNSUBSCRIBE for all clients with subscriptions. */
		doUnSubscribeAllTopics(mbinst, &currNumSubscribers);

		/* ----------------------------------------------------------------------------
		 * Will attempt to wait for all connections to have received an UNSUBACK for
		 * all subscriptions.   Currently there is up to 3 retries with 1 second wait.
		 * ---------------------------------------------------------------------------- */
		while ((count = countConnections(mbinst, CONSUMER, ALL_TOPICS_UNSUBACK, STATE_TYPE_UNIQUE, NULL)) < currNumSubscribers) {
			ism_common_sleep(SLEEP_1_SEC);

			if (retry++ > UNSUBSCRIBE_RETRY)
				break;

			count = 0;
		}
	}

	/* Perform a MQTT DISCONNECT for every client on this thread.  */
	MBTRACE(MBINFO, 1, "Submitter thread (#: %d) - Beginning MQTT Disconnection.\n", mbinst->id);

	/* Schedule jobs for all clients to be disconnected. */
	doDisconnectAllMQTTClients(mbinst, DUAL_CLIENT);

#ifdef _NEED_TIME
		MBTRACE(MBINFO, 7, "Additional Retries applied:  %d\n", pSysEnvSet->addRetry);

		g_TotalNumRetries_EXIT += pSysEnvSet->addRetry;

		MBTRACE(MBINFO, 7, "(%d) Curr Time:  %f\n", mbinst->id, getCurrTime());
#endif /* _NEED_TIME */

	ism_common_sleep(SLEEP_1_SEC);  /* Wait 1 second */
	retry = 0;    /* Reset the retry counter. */

	/* --------------------------------------------------------------------------------
	 * Call countCounnections to see how many clients have been disconnected with a
	 * requested state = MQTT_DISCONNECT_IN_PROCESS | MQTT_DISCONNECTED.  If all haven't
	 * been disconnected then sleep for 1 second and continue to retry until the retry
	 * count has been exceeded.  If retry count has been exceeded, then force a disconnect.
	 * -------------------------------------------------------------------------------- */
	reqState = (MQTT_DISCONNECT_IN_PROCESS | MQTT_DISCONNECTED );

	while ((count = countConnections(mbinst, DUAL_CLIENT, reqState, STATE_TYPE_MQTT, NULL)) < numClients) {
		ism_common_sleep(SLEEP_1_SEC);

		if (loopCt++ >= g_TotalNumRetries_EXIT) {
			if (retry++ > DISCONNECT_RETRY) {
				MBTRACE(MBWARN, 1, "Forcing disconnect on client(s): %d/%d (actual/expected).\n",
                                   count,
                                   numClients);
				fprintf(stdout, "(w) Forcing disconnect on client(s): %d/%d (actual/expected).\n",
                                count,
                                numClients);
				fflush(stdout);

				forceDisconnect(mbinst);
				break;
			}
		}
	}

	return rc;
} /* shutdownMQTTClients */

/* *************************************************************************************
 * doHandleClients
 *
 * Description:  Thread entry point for each instance of mqttbench (consumer and producer
 *               clients).
 *
 *   @param[in]  thread_parm         = Pointer to this thread's information (mqttbench_t)
 *   @param[in]  context             = x
 *   @param[in]  value               = x
 * *************************************************************************************/
void * doHandleClients (void *thread_parm, void *context, int value)
{
	mqttbench_t *mbinst            = (mqttbench_t *)thread_parm;
	mqttbenchInfo_t *mqttbenchInfo = (mqttbenchInfo_t *)mbinst->mqttbenchInfo;
	environmentSet_t *pSysEnvSet   = mqttbenchInfo->mbSysEnvSet;
	mbCmdLineArgs_t *pCmdLineArgs  = mqttbenchInfo->mbCmdLineArgs;
	mbThreadInfo_t * pThreadInfo   = mqttbenchInfo->mbThreadInfo;

	int rc = 0;
	int errRC = 0;
	int numClients = mbinst->numClients;
	int nrPubSubmitterThreads = 0;
	int discCount = 0;        /* Disconnect Count */
	int currState;            /* Local stack variable for trans->state */
	int idx;
	int sampleRatePerThread = 0;
	int messageRatePerThread = 0;
	int sendRate;
	int closeType = UNSUBSCRIBE_W_DISCONNECT;

	/* Local variables for command line parameters */
	int latencyMask       = pCmdLineArgs->latencyMask;
	int testDuration      = pCmdLineArgs->testDuration;

	/* Local variables for message size and rate related */
	int currMsgSize = 0;

	/* Local environment variables. */
	int envDelayCount = pSysEnvSet->delaySendCount;
	int localDelayCount = 0;
	int localSampleRate = pSysEnvSet->sampleRate;
	int iopIdx;
	int numBfrs = 0;
	int sendMsg = MSG_SKIP;   /* MSG_SEND == 0x1  and  MSG_SKIP == 0x0 */

	int8_t rateControl = pCmdLineArgs->rateControl;

	uint8_t qos = 0;
	uint8_t randClientAndTopicSelect = 0;
	uint8_t moreInFlight = 0;     /* Flag indicating more messages still in flight */
	uint8_t conditionMet = 0;
	uint8_t reconnectEnabled = pCmdLineArgs->reconnectEnabled;

	uint32_t txBufferPoolExhausted = 0;
	uint64_t expectedSubmitCnt = 0;

	long int randomNum;
	double startTime = 0;
	double endTime = 0;

	mqttclient_t *client = NULL;
	mqttmessage_t msg = {0};
	messagedir_object_t *currMsgDirObj = NULL;
	message_object_t *currMsgObj = NULL;

	ism_common_listIterator *pClientIter = NULL; /* iterator for list of clients for this thread (i.e. instance of mqttbench test) */
	ism_common_listIterator *pProdMsgInFlightIter = NULL;
	ism_common_list_node *currClient = NULL;
	ism_byte_buffer_t *txBuf = NULL;

	RateFilter filter;
	MicroWait timer;
	MicroWait connectTimer;
	double globalConnectRate = 1e6;   // if DelayTime environment variable is not set, then set default target global connect rate to 1M conn/sec (i.e. basically no delay)
	if (pSysEnvSet->delaySendTimeUsecs) {
		globalConnectRate = 1e6 / pSysEnvSet->delaySendTimeUsecs;  // convert microsecond delay to connection rate (conn/sec)
	}
	double perSubmitterThreadConnRate = (double) globalConnectRate / (double) pCmdLineArgs->numSubmitterThreads;

	/* Allocate the iterator for walking the list of MQTT clients for this instance of mqttbench test. */
	pClientIter = (ism_common_listIterator *)calloc(1, sizeof(ism_common_listIterator));
	if (pClientIter == NULL) {
    	rc = provideAllocErrorMsg ("an iterator", (int)sizeof(ism_common_listIterator), __FILE__, __LINE__);
		g_MBErrorRC = rc;
		return NULL;
	}

    /* Store the number of submitter threads which are assigned publishing clients in a local variable */
	nrPubSubmitterThreads = pThreadInfo->numProdThreads + pThreadInfo->numDualThreads;

	/* Initialize some basic variables. */
	ism_common_list_iter_init(pClientIter, mbinst->clientTuples);

	/* --------------------------------------------------------------------------------
	 * Retrieve TX buffers from each IOPs TX buffer pool, for this submitter thread
	 * -------------------------------------------------------------------------------- */
	numBfrs = pSysEnvSet->buffersPerReq;
	if (nrPubSubmitterThreads > 1) {
		numBfrs /= nrPubSubmitterThreads; /* for multiple submitter threads split the number of buffers amongst the threads.*/
	}

	/* Get TX buffers from each IOP for the producer(s). */
	rc = initIOPTxBuffers(mbinst, numBfrs);
	if (rc) {
		MBTRACE(MBERROR, 1, "Unable to initialize TX buffers for IOP (rc: %d).\n", rc);

		/* Destroy the iterator and free the memory. */
		ism_common_list_iter_destroy(pClientIter);
		g_MBErrorRC = RC_UNABLE_TO_INIT_BUFFERS;
		free(pClientIter);
		return NULL;
	}

	/* --------------------------------------------------------------------------------
	 * If there are any producers assigned to this submitter thread, then need to:
	 *
	 *  - allocate inflight msg counter if -c parameter used
	 *  - initialize the random generator for -t -2 ratecontrol mode
	 *  - initialize rate control timer
	 *  - initialize message selector for message latency
	 *  - calculate submit msg count
	 *  - If any QoS > 0 then need to wait for ACKs
	 *  - take start time
	 *  - check if performing sequence numbering of topics.
	 * -------------------------------------------------------------------------------- */
	if ((mbinst->clientType & PRODUCER) > 0) {
		if (pCmdLineArgs->globalMsgPubCount > 0) {
			pProdMsgInFlightIter = (ism_common_listIterator *)calloc(1, sizeof(ism_common_listIterator));
			if (pProdMsgInFlightIter) {
				ism_common_list_iter_init(pProdMsgInFlightIter, mbinst->clientTuples);
			} else {
				rc = provideAllocErrorMsg("an iterator", (int)sizeof(ism_common_listIterator), __FILE__, __LINE__);
				g_MBErrorRC = rc;
				free(pClientIter);
				return NULL;
			}
		}

		if (rateControl == -2)
			randClientAndTopicSelect = 1;

		/* Seed the random generator. */
		srand(time(NULL));

		/* ----------------------------------------------------------------------------
		 * Need to initialize the per thread MW_wait structure when each thread is
		 * controlling its own rate.
		 * ---------------------------------------------------------------------------- */
		if (rateControl == 0) {
			timer.valid = 1;
			MW_init(&timer, ((double)g_MsgRate / (double)nrPubSubmitterThreads));
			if (timer.valid == 0) {
				return (void *)NULL;
			}
		}

		/* If performing round trip latency then need to set the Selector up. */
		if ((latencyMask & CHKTIMERTT) > 0) {
			sampleRatePerThread = (int)((double)localSampleRate / (double)nrPubSubmitterThreads);

			if (rateControl == -1) /* No rate control, send as fast as possible */
				messageRatePerThread = (int)((double)MAXGLOBALRATE / (double)nrPubSubmitterThreads);
			else  /* With rate control */
				messageRatePerThread = (int)((double)g_MsgRate / (double)nrPubSubmitterThreads);

			/* Initialize the rate filter data structure for latency sampling */
			initMessageSampler(&filter, sampleRatePerThread, messageRatePerThread);
		}

		/* ----------------------------------------------------------------------------
		 * If sending a specific message count (-c option) then need to set up the
		 * appropriate fields.
		 * ---------------------------------------------------------------------------- */
		if (pCmdLineArgs->globalMsgPubCount > 0) {
			if (nrPubSubmitterThreads > 1) {
				int fraction = (pCmdLineArgs->globalMsgPubCount % nrPubSubmitterThreads);
				expectedSubmitCnt = (pCmdLineArgs->globalMsgPubCount / nrPubSubmitterThreads);

				if ((fraction > 0) && (mbinst->id < fraction))
					expectedSubmitCnt++;
			} else
				expectedSubmitCnt = pCmdLineArgs->globalMsgPubCount;
		}

		/* ----------------------------------------------------------------------------
		 * Check to see whether QoS 1 or 2 is set and if so then set QoS to the highest
		 * value.  This is needed for the case when using -c.
		 * ---------------------------------------------------------------------------- */
		if (mbinst->txQoSBitMap & MQTT_QOS_2_BITMASK) {
			qos = 2;
		} else {
			if (mbinst->txQoSBitMap & MQTT_QOS_1_BITMASK) {
				qos = 1;
			}
		}

		/* If using sequence numbers on the topic then set the flag in the msg. */
		msg.useseqnum = ((pCmdLineArgs->chkMsgMask & CHKMSGSEQNUM) > 0 ? 1 : 0);

		startTime = getCurrTime();
	} /* if (mbinst->clientType & PRODUCER) */

	/* --------------------------------------------------------------------------------
	 * Set the stack variable for # topics per client.
	 *
	 * roundRobinSendSub = 0
	 *    - Number of topics per client is the absolute value of the mqttbench->tpc
	 *
	 * roundRobinSendSub = 1
	 *    - Number of topics per client is the absolute value of the mqttbench->tpc
	 *      divided by the (absolute value of mqttbench->numClients * absolute value of
	 *      mqttbench->nThrds)
	 * -------------------------------------------------------------------------------- */
	MBTRACE(MBINFO, 1, "Submitter thread (#: %d) - Starting %d connection(s) @ %.0f conn/sec.\n",
			           mbinst->id, numClients, perSubmitterThreadConnRate);

	/* --------------------------------------------------------------------------------
	 * Set counters and time needed for when disconnecting.  This is based on number of
	 * clients.  Want to give time for IOPs to disconnect the clients before forcing
	 * the disconnect.
	 * -------------------------------------------------------------------------------- */
	g_TotalNumRetries_EXIT = numClients / NUM_CLIENTS_PER_CHUNK;
	if (g_TotalNumRetries_EXIT == 0)
		g_TotalNumRetries_EXIT = 1;
	else
		g_TotalNumRetries_EXIT = 3;

	/* Calculate per submitter thread connection rate (conn/sec/thread) and
	 * initialize MicroWait timer for the connection rate loop */
	MW_init(&connectTimer, perSubmitterThreadConnRate);

	/* --------------------------------------------------------------------------------
	 * Submit job to initiate connections for all assigned clients
	 *
	 * All the clients have been initialized (memory, structures...etc).  At this point
	 * to start the process a job needs to be scheduled on the IOP for creating the
	 * socket.  The IOPs will do the following:
	 *
	 *   - TCP Connection (includes SSL Connection)
	 *   - MQTT Connection
	 *   - Subscription (Consumers)
	 *
	 * Note:  The consumers and producers will enter a unique loop for that type.
	 * -------------------------------------------------------------------------------- */
	while (((currClient = ism_common_list_iter_next(pClientIter)) != NULL) &&
			(__builtin_expect((g_RequestedQuit == 0), 1)))
	{
		/* ----------------------------------------------------------------------------
		 * If user requested quit then perform the necessary cleanup prior to
		 * exiting the thread.
		 * ---------------------------------------------------------------------------- */
		if (g_RequestedQuit) {
			doThreadCleanUp(mbinst, 1, RC_REQUESTED_QUIT, testDuration, pClientIter);
			return NULL;
		}

		rc = submitCreateConnectionJob(((mqttclient_t *)(currClient->data))->trans);
		if (rc < 0) {
			MBTRACE(MBERROR, 1, "Failed to submit an IOP job for connection creation (rc: %d).\n", rc);

			/* Perform the necessary cleanup prior to exiting the thread due to an error. */
			doThreadCleanUp(mbinst, 1, rc, testDuration, pClientIter);
			return NULL;
		} else {
			if (envDelayCount > 0) {
				if (++localDelayCount == envDelayCount) {
					localDelayCount = 0;
					MW_wait(&connectTimer);   // rate control for connections
				}
			}
		}
	} /* while loop which iterates through list of clients assigned to this submitter thread and connects them */

	/* --------------------------------------------------------------------------------
	 * All jobs for connections have been submitted and now preparing to start
	 * publishing messages.
	 * -------------------------------------------------------------------------------- */
	MBTRACE(MBINFO, 1, "Submitter thread (#: %d) - Started %d connection(s).\n", mbinst->id, numClients);

	/* --------------------------------------------------------------------------------
	 * If performing latency testing specifically for TCPIP Connection, MQTT Connection,
	 * and Subscription times then collect these latencies before entering the publish
	 * loop.
	 *
	 * NOTE: This also requires that reconnect not be enabled.
	 * -------------------------------------------------------------------------------- */
	if ((reconnectEnabled == 0)     &&
		(latencyMask > CHKTIMESEND) &&
		((latencyMask & CHKTIMERTT) == 0))
	{
		/* ----------------------------------------------------------------------------
		 * Start the while loop that is waiting for either a requested quit,
		 * time ran out or the connection latency has completed.
		 * ---------------------------------------------------------------------------- */
		conditionMet = 0;

		do {
			ism_common_sleep(SLEEP_1_SEC);

			if (g_RequestedQuit || g_StopProcessing) {
				return NULL;
			}
			else if ((latencyMask & CHKTIMETCPCONN) > 0) {
				if (mbinst->numTCPConnects == numClients) {
					conditionMet = 1;
					MBTRACE(MBINFO, 5, "Completed expected number of TCP/TLS handshakes (%d) for this submitter thread\n",
							          mbinst->numTCPConnects);
				}
			} else if ((latencyMask & (CHKTIMEMQTTCONN | CHKTIMETCP2MQTTCONN)) > 0) {
				if (mbinst->numMQTTConnects == numClients) {
					conditionMet = 1;
					MBTRACE(MBINFO, 5, "Completed expected number of MQTT handshakes (%d) for this submitter thread\n",
									   mbinst->numMQTTConnects);
				}
			} else if ((latencyMask & (CHKTIMESUBSCRIBE | CHKTIMETCP2SUBSCRIBE)) > 0) {
				if (mbinst->numSubscribes == mbinst->numSubsPerThread) {
					conditionMet = 1;
					MBTRACE(MBINFO, 5, "Completed expected number of MQTT subscribes (%d) for this submitter thread\n",
									   mbinst->numSubscribes);
				}
			}
		} while (conditionMet == 0);
	}

	/* --------------------------------------------------------------------------------
	 * If the wait ready option (-wr) was specified then this thread will enter a
	 * while loop in order to wait for all CONNACKs (Consumers/Producers) and all
	 * s (Consumers) have been received prior to entering the hot loop
	 * (sending messages).
	 *
	 * Use the ready_lock, increment the g_SubmitThreadReady counter and decrement the
	 * waitReady counter.
	 *
	 * If the wait ready option	is specified then enter the loop and perform a 1 usec
	 * sleep checking the following after each sleep:
	 *
	 *   - stopProcessing  :: Time expired or failure
	 *     -- or --
	 *     requestedQuit   :: User requested a quit
	 *   - waitReady       :: All threads are ready to enter the hot loop.  If producers
	 *                        they will send messages.
	 * -------------------------------------------------------------------------------- */
	if (g_WaitReady) {
		int numMBInstClientsMQTTConnected = 0;

		while ((g_RequestedQuit == 0) && (g_StopProcessing == 0)) {  /* BEAM suppression:  infinite loop */
			/* Reset the iterator */
			ism_common_list_iter_reset(pClientIter);

			/* ------------------------------------------------------------------------
			 * Reset the counter for the clients in MQTT CONNECT state.  The producers
			 * will be in PUBSUB and the consumers will either be in PUBSUB or CONNECT.
			 * ------------------------------------------------------------------------ */
			numMBInstClientsMQTTConnected = 0;

			/* Go through each of the clients and count the number of clients in PUBSUB */
			while (((currClient = ism_common_list_iter_next(pClientIter)) != NULL) &&
					(__builtin_expect((g_RequestedQuit == 0), 1)))
			{
				if ((((mqttclient_t *)currClient->data)->protocolState & MQTT_CONNECT_OR_PUBSUB) > 0)
					numMBInstClientsMQTTConnected++;
			}

			if (numMBInstClientsMQTTConnected >= mbinst->numClients) {
				if ((mbinst->clientType & CONSUMER) > 0) {
					if (mbinst->numSubscribes < mbinst->numSubsPerThread) {
						ism_common_sleep(SLEEP_1_SEC);
						continue;
					}
				}

				pthread_spin_lock(&ready_lock);
				g_SubmitThreadReady++;
				g_WaitReady--;
				pthread_spin_unlock(&ready_lock);

				MBTRACE(MBINFO, 5, "All clients assigned to submitter thread %d, have completed the MQTT handshake "\
						           "and have completed their MQTT subscriptions\n", mbinst->id);

				break;
			}

			ism_common_sleep(SLEEP_1_SEC);
		};
	} else {
		pthread_spin_lock(&ready_lock);
		g_SubmitThreadReady++;
		pthread_spin_unlock(&ready_lock);
	}

	/* --------------------------------------------------------------------------------
	 * If the wait ready option was specified then need to ensure that all the threads
	 * are ready before entering the hot loop.
	 * -------------------------------------------------------------------------------- */
	while ((g_RequestedQuit == 0) && (g_WaitReady) && (g_StopProcessing == 0)) {
		ism_common_sleep(SLEEP_1_SEC);
	};

	static volatile int singletonWRFlag = 1;
	if(!pCmdLineArgs->noWaitFlag && singletonWRFlag){
		singletonWRFlag = 0;
		MBTRACE(MBINFO, 5, "All clients assigned to all submitter threads have completed the MQTT handshake "\
						   "and have completed their MQTT subscriptions.  Publishing may begin now.\n");
	}

	/* --------------------------------------------------------------------------------
	 * If user requested quit then perform the necessary cleanup prior to exiting
	 * the thread.
	 * -------------------------------------------------------------------------------- */
	if (g_RequestedQuit || g_StopProcessing) {
		if(!errRC && g_MBErrorRC) // if local errRC is 0, then check the global g_MBErrorRC
			errRC = g_MBErrorRC;
		doThreadCleanUp(mbinst, 1, errRC, testDuration, pClientIter);
		return NULL;
	}

	/* If requested to measure jitter than enable the timer task to do so. */
	if ((pCmdLineArgs->determineJitter) && (mbinst->id == 0)) {
		rc = addJitterTimerTask(g_MqttbenchInfo);
	}

	if ((mbinst->clientType & PRODUCER) > 0)
		MBTRACE(MBINFO, 1, "Submitter thread (#: %d) - Beginning to publish messages.\n", mbinst->id);

	/* --------------------------------------------------------------------------------
	 * All the clients have been initialized (memory, structures...etc).  At this
	 * point the client->trans->state and the client->protocolState variables are
	 * interrogated to determine what should be done.
	 * -------------------------------------------------------------------------------- */
	if ((mbinst->clientType & PRODUCER) > 0)
	{
		int sendLessThan1KMsgs = (pCmdLineArgs->globalMsgPubCount > 0) && (pCmdLineArgs->globalMsgPubCount < 1000); /* -c X, where X < 1000 */
		do // PUBLISH HOT LOOP
		{
			/* Reset the iterator, the tcpConnCount if testing for TCP Connection time */
			ism_common_list_iter_reset(pClientIter);

			/* Go through each of the clients and see what is the next processing step */
			while (((currClient = ism_common_list_iter_next(pClientIter)) != NULL) &&
					(__builtin_expect((g_RequestedQuit == 0), 1)))
			{
				client = (mqttclient_t *)(currClient->data);

				/* Check if the client has a publish (TX) list.  If not continue. */
				if (client->destTxListCount == 0)
					continue;

				currState = (client->trans->state & TRANS_CHECK_STATES);

				/* --------------------------------------------------------------------
				 * If there is no socket error and the sock hasn't been disconnected
				 * switch the client->protocolState in order to decide what to do next.
				 * -------------------------------------------------------------------- */
				if ((currState & (SHUTDOWN_IN_PROCESS | SOCK_DISCONNECTED | SOCK_ERROR)) == 0) {
					if ((currState & SOCK_CONNECTED) > 0) { /* socket connection finished */
						switch (client->protocolState) {
						case (MQTT_PUBSUB): /* Ready to send PUBLISH or SUBSCRIBE msg */
							/* --------------------------------------------------------
							 * At this point need to determine which option was used
							 * for sending messages (-c or -r).  The current logic:
							 *
							 * - If using -c then set the sendMsg = 1
							 * - If using -r then use the random() function to determine
							 *   whether to send a message.
							 * - After sending a message if this is QoS level 2 and
							 *   there are PUBREC's needing a PUBREL then allow the
							 *   client to send one.
							 * -------------------------------------------------------- */
							if ((pCmdLineArgs->globalMsgPubCount > 0) && (testDuration <= 0)) { /* -c <messages> */
								if (mbinst->submitCnt < expectedSubmitCnt) {
									sendMsg = MSG_SEND;
								} else {  /* Have completed sending the desired # of msgs */
									sendMsg = MSG_SKIP;

									/* ------------------------------------------------
									 * If QoS = 1 OR QoS = 2, then need to wait for all
									 * PUBRECS and/or PUBCOMPS to be processed.  Need to
									 * hang out until all clients currInFlightMessages
									 * have completed processing (0).
									 * ------------------------------------------------ */
									if (qos) {
										do {
											ism_common_list_iter_reset(pProdMsgInFlightIter);
											moreInFlight = 0;

											while ((currClient = ism_common_list_iter_next(pProdMsgInFlightIter)) != NULL) {
												if (((mqttclient_t *)(currClient->data))->currInflightMessages) {
													moreInFlight = 1;

													if (qos == 1)
														break;
												}
											}
										} while ((moreInFlight) && (g_RequestedQuit == 0));

										ism_common_list_iter_destroy(pProdMsgInFlightIter);
										free(pProdMsgInFlightIter);
									}

									endTime = getCurrTime();
									if ((endTime - startTime) < 1) {
										MBTRACE(MBWARN, 1, "Prod %d: the test did not run long enough (%d secs) " \
									                       "to provide meaningful throughput results.\n",
							                               mbinst->id,
							                               (int)(endTime - startTime));
										fprintf(stdout, "(w) Prod %d: the test did not run long enough (%d secs) " \
									    	            "to provide meaningful throughput results.\n",
							                            mbinst->id,
							                            (int)(endTime - startTime));
									} else {
										sendRate = (mbinst->submitCnt / (endTime - startTime));
										MBTRACE(MBINFO, 1, "Prod %d: avg send rate: %d\n", mbinst->id, sendRate);
										fprintf(stdout, "Prod %d: avg send rate: %d\n", mbinst->id, sendRate);
									}

									fflush(stdout);

									/* ------------------------------------------------
									 * If the test duration (-d option) is set to '0'
									 * then sit in a while loop waiting for the user to
									 * enter a quit or close.
									 * ------------------------------------------------ */
									if (testDuration == 0) {
										do {
											ism_common_sleep(SLEEP_1_SEC);
										} while (g_RequestedQuit == 0);  /* BEAM suppression:  infinite loop */
									}

									/* --------------------------------------------
									 * User requested to exit.  Check if the user
									 * requested a "close".
									 *
									 * If "close" then just perform an
									 * MQTT DISCONNECT for all clients.
									 *
									 * If "quit" then perform:
									 *    1. MQTT UNSUBSCRIBE for any connections
									 *       that contain subscriptions.
									 *    2. Wait a minimal amount of time to
									 *       handle UNSUBACKs from Server.
									 *    3. MQTT DISCONNECT for all clients.
									 * -------------------------------------------- */
									if (pCmdLineArgs->disconnectType == 1)
										closeType = DISCONNECT_ONLY;

									rc = shutdownMQTTClients(mbinst, closeType);

									/* Perform the necessary cleanup prior to exiting the thread. */
									doThreadCleanUp(mbinst, 0, RC_SUCCESSFUL, testDuration, pClientIter);
									return NULL;
								} /* if (mbinst->submitCnt < expectedSubmitCnt) */
							} else if (randClientAndTopicSelect == 0) {
								sendMsg = MSG_SEND;
							} else {  /* Timed duration */
								lrand48_r(&randBuffer, &randomNum);
								sendMsg = randomNum & 0x1;
							} /* if ((pCmdLineArgs->globalMsgPubCount != 0) && (testDuration <= 0)) */

							/* Check if a message should be sent for this client for this iteration */
							if (sendMsg) {
								/* ----------------------------------------------------
								 * A message is going to be sent and the client message
								 * payload needs to be set to the local stack variable
								 * in order to obtain what the next message size is to
								 * be sent along with information that needs to be place
								 * in the message structure.
								 * ---------------------------------------------------- */
				                currMsgDirObj = client->msgDirObj;
				                currMsgObj = &currMsgDirObj->msgObjArray[client->msgObjIndex];
	                            currMsgSize = currMsgObj->payloadBuf->used;
	                            msg.payloadlen = currMsgSize;
	                            msg.payload = currMsgObj->payloadBuf->buf;
	                            msg.dupflag = 0;

								/* ----------------------------------------------------
								 * When performing round trip latency:
								 *    - Check if the message is to be sampled and if so
								 *      set the msg.marked field to '1'.
								 *
								 * V3.1 and V3.1.1
								 *    - Latency can only be performed on the command
								 *      line size option (-s <min>-<max>).  Predefined
								 *      messages doesn't support latency since it would
								 *      overwrite the data.
								 *    - 1st 17 bytes in the payload is overlayed with
								 *      Performance Info which consists of 1 byte header,
								 *      8 byte Timestamp, and 8 byte sequence number.
								 *    - Header is cleared, If message is to be sampled
								 *      then msg.marked is set to 1 to ensure the header
								 *      in Performance Info is set and Timestamp is
								 *      filled in.
								 *    - Receiving client inspects the Performance Info
								 *      to see if header indicates that messages is to
								 *      be sampled.   So timestamp is extracted from
								 *      message in order to calculate round trip latency.
								 * ----------------------------------------------------
								 * V5
								 *    - If msg.marked field is set to 1 then User
								 *      Property 'TS' is used to contain the timestamp.
								 *    - The receiving client inspects the Properties
								 *      searching for the User Property marked 'TS'.
								 *      If found then extracts the timestamp from
								 *      the value portion of User Property in order
								 *      to calculate Round Trip Latency.
								 * ---------------------------------------------------- */
	                            if ((latencyMask & CHKTIMERTT) > 0) {
	                            	if ((selectForSampling(&filter)) || sendLessThan1KMsgs) { /* if we are sending less than 1K messages and message latency is enabled, sample 100% of messages for latency */
	                            		/* This message will be sampled for RTT latency */
	                            		if (client->mqttVersion >= MQTT_V5) {  	/* For MQTT v5 clients and above put send timestamp in an MQTT user property */
	                            			msg.marked = 1;
										} else {								/* For MQTT v3.1.1 or earlier we only measure message latency on generated msgs (i.e. -s <min>-<max>) */
											if(currMsgDirObj->generated) {		/* Binary generated message payloads (i.e. -s <min>-<max>) */
												msg.payload[0] = 1;
												msg.marked = 1;
											}
										}

	                            	} else {
	                            		/* This message will NOT be sampled for RTT latency, for MQTT v3.1.1 clients with
	                            		 * generate messages we need to mark the payload and msg object as "not sampled" */
	                            		if((client->mqttVersion < MQTT_V5) && currMsgDirObj->generated) {
											msg.marked = 0;
											msg.payload[0] = 0;
	                            		}
	                            	}
	                            }

	                        	/* ----------------------------------------------------
	                        	 * Increment the size index and if greater than the
	                        	 * total number of different messages sizes then reset
	                        	 * back to 0.
	                        	 * ---------------------------------------------------- */
	                            if (randClientAndTopicSelect == 0) { // select the next message, in order
	                            	if(++client->msgObjIndex >= currMsgDirObj->numMsgObj)
	                            		client->msgObjIndex = 0;
	                            } else {							 // randomly select next message
	                            	lrand48_r(&randBuffer, &randomNum);
	                            	client->msgObjIndex = randomNum % currMsgDirObj->numMsgObj;
	                            }
								/* ----------------------------------------------------
								 * If not using the burst mode then need to use round
								 * robin on the topic for the clients.  Else, randomly
								 * pick the topic with the lrand48_r function.
								 * ---------------------------------------------------- */
								if (randClientAndTopicSelect == 0) {
									if(currMsgObj->topic) { // grab the destTxList index from the MD-based list of destTuple indexes
										idx = client->destTxFromMD[client->topicIdxFromMD];
										if(++client->topicIdxFromMD >= client->destTxFromMDCount)
											client->topicIdxFromMD = 0;
									} else {				// grab the destTxList index from the CL-based list of destTuple indexes
										idx = client->destTxFromCL[client->topicIdxFromCL];
										if(++client->topicIdxFromCL >= client->destTxFromCLCount)
											client->topicIdxFromCL = 0;
									}
								} else {
									lrand48_r(&randBuffer, &randomNum);
									if(currMsgObj->topic) { // no random destTuple selection for topics that are defined within message files
										idx = client->destTxFromMD[client->topicIdxFromMD];
										if(++client->topicIdxFromMD >= client->destTxFromMDCount)
											client->topicIdxFromMD = 0;
									} else {				// randomly select next destTuple to publish message on
										idx = client->destTxFromCL[randomNum % client->destTxFromCLCount];
									}
								}

								iopIdx = (int)client->ioProcThreadIdx;
								txBuf = mbinst->txBfr[iopIdx];
								if (txBuf == NULL) {
									volatile ioProcThread_t * iop = client->trans->ioProcThread;
									mbinst->txBfr[iopIdx] = getIOPTxBuffers(iop->sendBuffPool, numBfrs, 0);

									/* ------------------------------------------------
									 * Check to see if the getIOPTxBuffers returned a
									 * NULL. If so then need to update the counter, and
									 * check to see if the counter has reached the
									 * specific max and then display a message that the
									 * pool is exhausted.
									 * ------------------------------------------------ */
									if (mbinst->txBfr[iopIdx]) {
										txBuf = mbinst->txBfr[iopIdx];
									} else {
										if (++txBufferPoolExhausted >= GETBUFFER_FAIL_CTR) {
											int txNumFreeBfr = 0, txAllocatedBfr = 0, txMaxSizeBfr = 0, txMinSizeBfr = 0;

											ism_common_getBufferPoolInfo(iop->sendBuffPool, &txMinSizeBfr, &txMaxSizeBfr, &txAllocatedBfr, &txNumFreeBfr);

						   					txBufferPoolExhausted = 0;  /* Reset the Counter */
							   				MBTRACE(MBWARN, 1, "TX buffer pool on IOP %d has reached max # of allocations (%d) " \
							   						           "with a request of %d buffers.\n",
							   					               iopIdx,
							   					               txAllocatedBfr,
													           numBfrs);
										}

										continue;  /* Move on to next client. */
									}
								}

								mbinst->txBfr[iopIdx] = txBuf->next;

								submitPublish(client, currMsgObj, &msg, idx, txBuf); // build and send PUBLISH message

								if (rateControl == 0) { /* Individual producer rate control */
									/* ------------------------------------------------
									 * sched_yield implemented "sleep" function.  Used
									 * to control the message transmission rate per
									 * publishing thread. This is not used in the case
									 * where the rate control thread is being used.
									 * ------------------------------------------------ */
									MW_wait(&timer);

									/* ------------------------------------------------
									 * Check if stopProcessing was requested.  This is
									 * needed for large fanin and very slow rates.
									 * ------------------------------------------------ */
									if (g_RequestedQuit) {
										/* --------------------------------------------
										 * User requested to exit.  Check if the user
										 * requested a "close".
										 *
										 * If "close" then just perform an
										 * MQTT DISCONNECT for all clients.
										 *
										 * If "quit" then perform:
										 *    1. MQTT UNSUBSCRIBE for any connections
										 *       that contain subscriptions.
										 *    2. Wait a minimal amount of time to
										 *       handle UNSUBACKs from Server.
										 *    3. MQTT DISCONNECT for all clients.
										 * -------------------------------------------- */
										if (pCmdLineArgs->disconnectType == 1)
											closeType = DISCONNECT_ONLY;

										rc = shutdownMQTTClients(mbinst, closeType);

										/* Perform the necessary cleanup prior to exiting the thread. */
										doThreadCleanUp(mbinst, 0, RC_REQUESTED_QUIT, testDuration, pClientIter);
										return NULL;
									}
								} /* if (rateControl == 0) */

								/* Check to see if this client has published the max number of messages per connection (i.e. lingerMsgsCount)
								 * if so, schedule a disconnect (and reconnect, if enabled for this client) */
								if(client->lingerMsgsCount && (client->currTxMsgCounts[PUBLISH] % client->lingerMsgsCount == 0)) {
									client->disconnectRC = MQTTRC_OK;
									submitMQTTDisconnectJob(client->trans, DEFERJOB);
								}
							} /* if (sendMsg) */

							/* --------------------------------------------------------
							 * Check to see if the message rate has been changed, which
							 * is highly unlikely.
							 * -------------------------------------------------------- */
							if (UNLIKELY(mbinst->rateChanged == 1)) {
								if (g_MsgRate) {
									MW_init(&timer, ((double)g_MsgRate / (double)nrPubSubmitterThreads)); /*BEAM suppression: division by 0*/
								} else {
									while ((g_RequestedQuit == 0) && (g_MsgRate == 0)) {
										sched_yield();
									}

									if (g_MsgRate)
										MW_init(&timer, ((double)g_MsgRate / (double)nrPubSubmitterThreads)); /*BEAM suppression: division by 0*/
								}

								mbinst->rateChanged = 0;
							}
							break;
						case MQTT_DISCONNECT_IN_PROCESS:
							break;
						default:
							break;
						} /* switch(client->protocolState) */
					} /* if ((currState & SOCK_CONNECTED) > 0) */
				} else { /* transport state not in state to send or receive */
					errRC = 1;

					if ((g_RequestedQuit == 0) && (g_StopProcessing == 0)) {
						if (reconnectEnabled)
							continue;
						else if (client->errorMsgDisplayed == 0) {
						    client->errorMsgDisplayed = 1;
						    MBTRACE(MBWARN, 1, "%s trans->state: 0x%X client->protocolState: 0x%X\n",
	                                           client->clientID,
	                                           currState,
			                                   client->protocolState);
							fprintf(stdout, "(w) %s trans->state: 0x%X client->protocolState: 0x%X\n",
	                                        client->clientID,
	                                        currState,
			                                client->protocolState);
							fflush(stdout);

						    errRC = RC_ERROR_PROCESS_FAILURE;
							if ((client->trans->state & SOCK_ERROR) > 0) {
								MBTRACE(MBERROR, 1, "Socket Error while publishing (rc: %d).\n", errRC);
							} else if ((client->trans->state & SOCK_DISCONNECTED) > 0)
								MBTRACE(MBERROR, 1, "Socket Disconnected while publishing.\n");
						}
					} else /* if ((g_RequestedQuit == 0) && (g_StopProcessing == 0)) */
						break;
				} /* if ((currState & (SOCK_ERROR | SOCK_DISCONNECTED | SHUTDOWN_IN_PROCESS)) == 0) */

				if (g_RequestedQuit)
					break;

			} /* while ((iter) && (stopProcessing == 0)) */
		} while ((g_RequestedQuit == 0) && (g_StopProcessing == 0));   /* BEAM suppression:  infinite loop */
	} else { // if (mbinst->clientType & PRODUCER) > 0
		/* ----------------------------------------------------------------------------
		 * All assigned clients are consumer only, so going to perform a sleep each time thru.
		 * ---------------------------------------------------------------------------- */
		do
		{
			/* Reset the iterator, the tcpConnCount if testing for TCP Connection time */
			ism_common_list_iter_reset(pClientIter);

			/* Go through each of the clients and see what is the next processing step */
			while (((currClient = ism_common_list_iter_next(pClientIter)) != NULL) &&
					(__builtin_expect((g_RequestedQuit == 0), 1)))
			{
				client = (mqttclient_t *)(currClient->data);
				currState = (client->trans->state & TRANS_CHECK_STATES);

				/* --------------------------------------------------------------------
				 * If there is no socket error and that the sock hasn't been disconnected
				 * switch the client->protocolState in order to decide what to do next.
				 * -------------------------------------------------------------------- */
				if ((currState & (SHUTDOWN_IN_PROCESS | SOCK_DISCONNECTED | SOCK_ERROR)) > 0) {
					errRC = 1;

					if ((g_RequestedQuit == 0) && (g_StopProcessing == 0)) {
						if (reconnectEnabled)
							continue;
						else if (client->errorMsgDisplayed == 0) {
						    client->errorMsgDisplayed = 1;
						    MBTRACE(MBWARN, 1, "%s trans->state: 0x%X client->protocolState: 0x%X\n",
	                                           client->clientID,
	                                           currState,
			                                   client->protocolState);
							fprintf(stdout, "(w) %s trans->state: 0x%X client->protocolState: 0x%X\n",
	                                        client->clientID,
	                                        currState,
			                                client->protocolState);
							fflush(stdout);

						    errRC = RC_ERROR_PROCESS_FAILURE;
							if ((client->trans->state & SOCK_ERROR) > 0) {
								MBTRACE(MBERROR, 1, "Socket Error while publishing (rc: %d).\n", errRC);
							} else if ((client->trans->state & SOCK_DISCONNECTED) > 0) {
								MBTRACE(MBERROR, 1, "Socket Disconnected while publishing.\n");
							}
						}
					} /* if ((g_RequestedQuit == 0) && (g_StopProcessing == 0)) */
				}

				ism_common_sleep(SLEEP_1_SEC);

				if (g_RequestedQuit)
					break;
			} /* while( ) */
		} while ((g_RequestedQuit == 0) && (g_StopProcessing == 0));   /* BEAM suppression:  infinite loop */
	}

	endTime = getCurrTime();

	/* --------------------------------------------------------------------------------
	 * Check the mbErrorRC to ensure that this wasn't due to an ABORT.   If not an
	 * ABORT then perform the necessary steps for shutting down.
	 * -------------------------------------------------------------------------------- */
	if (g_MBErrorRC == RC_SUCCESSFUL) {
		/* ----------------------------------------------------------------------------
		 * Exited the main loop and now looking at performing some kind of cleanup.  The
		 * user either specified "q", "quit", "close" or "closeconn".  The "close" and
		 * "closeconn" commands are purely for the consumers since this will not perform
		 * the UnSubscribe.  "Quit" command performs the UnSubscribe for the consumers.
		 * The following actions performed based on command and type of client:
		 *
		 * Command        Client           Description
		 * ============   ==============   ============================================
		 * "q"      OR    1) Consumer(s)   UnSubscribe, MQTT Disconnect and close TCP Connection
		 * "quit"         2) Producer(s)   MQTT Disconnect and close TCP Connection
		 *
		 * "close"  OR    1) Consumer(s)   MQTT Disconnect and close TCP Connection
		 * "closeconn"    2) Producer(s)   MQTT Disconnect and close TCP Connection
		 * ---------------------------------------------------------------------------- */
		if (pCmdLineArgs->disconnectType == 0) {
			/* ------------------------------------------------------------------------
			 * Process terminating for quit:
			 *    1. MQTT UNSUBSCRIBE for any connections that contain subscriptions.
			 *    2. Wait a minimal amount of time to handle UNSUBACKs from Server.
			 *    3. MQTT DISCONNECT for all clients.
			 * ------------------------------------------------------------------------ */
			closeType = UNSUBSCRIBE_W_DISCONNECT;
		} else {
			/* ------------------------------------------------------------------------
			 * Process terminating for close:
			 *    1. Wait 500 millisecs prior to calling disconnect in order to complete
			 *       processing of messages.
			 *    2. MQTT DISCONNECT for all clients.
			 * ------------------------------------------------------------------------ */
			ism_common_sleep(5 * SLEEP_100_MSEC);
			closeType = DISCONNECT_ONLY;
		}

		rc = shutdownMQTTClients(mbinst, closeType);

		MBTRACE(MBINFO, 6, "(%d) Completed Time:  %f\n", mbinst->id, getCurrTime());
	}

	MBTRACE(MBINFO, 1, "Submitter thread (#: %d) - %s (%d).\n",
                       mbinst->id,
                       (discCount == numClients ? "Successfully disconnected all" : "Only partial disconnection"),
                       discCount);

	MBTRACE(MBINFO, 1, "Submitter thread (#: %d) - Performing cleanup for all connections.\n", mbinst->id);

	/* Perform the necessary cleanup prior to exiting the thread. */
	if(!errRC && g_MBErrorRC) // if local errRC is 0, then check the global g_MBErrorRC
		errRC = g_MBErrorRC;

	doThreadCleanUp(mbinst, 1, errRC, testDuration, pClientIter);

	return NULL;
} /* doHandleClients */

/*
 * Validate MQTT Users Name/Pair property
 *
 * @param[in] ptr		= pointer to a pair of UTF-8 encoded MQTT property strings (i.e Name/Value pair)
 * @param[in] len		= length of the pair ot UTF-8 encoded MQTT property strings
 *
 * @return A return code: 0=good, otherwise an RC return code
 */
static int validateNamePair(const char * ptr, int len) {
    int namelen = READINT16(ptr);
    int valuelen = READINT16(ptr+2+namelen);
    char * outname;
    char * outval;
    if (ism_common_validUTF8Restrict(ptr+2, namelen, UR_NoNull)<0 ||
        ism_common_validUTF8(ptr+4+namelen, valuelen) < 0) {
        /* Make the error string valid */
        int shortname = namelen;
        if (shortname > 256)
            shortname = 256;
        outname = alloca(shortname+1);
        if (valuelen > 256)
            valuelen = 256;
        outval = alloca(valuelen+1);
        ism_common_validUTF8Replace(ptr+2, shortname, outname, UR_NoControl | UR_NoNonchar, '?');
        ism_common_validUTF8Replace(ptr+4+namelen, valuelen, outval, UR_NoControl | UR_NoNonchar, '?');
        MBTRACE(MBERROR, 5, "Invalid MQTT property name/value pair name=%s, value=%s\n", outname, outval);
        return MQTTRC_MalformedPacket;
    }
    return 0;
}

/*
 * Callback function (called from checkMqttPropFields) to further process MQTT properties
 *
 * @param[in] ctx     	= the MQTT property context
 * @param[in] userdata	= userdata to pass to the callback function
 * @param[in] fld       = the property field entry from the mqtt property context table (g_mqttPropFields)
 * @param[in] ptr		= pointer to a UTF-8 encoded MQTT property (i.e. string property)
 * @param[in] len		= length of the UTF-8 encoded MQTT property located at ptr
 * @param[in] value   	= the value of an integer MQTT property (i.e. integer property)
 *
 * @return A return code: 0=good, otherwise an RC return code
 */
static int mqttPropertyCheck(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld, const char * ptr, int len, uint32_t value) {
	int rc = 0;
	mqttmessage_t *mmsg = (mqttmessage_t *) userdata;
	mqttclient_t *client = mmsg->client;

    if (fld->type == MPT_String) {
        if (ism_common_validUTF8Restrict(ptr, len, UR_NoControl | UR_NoNonchar) < 0) {
            char * out;
            if (len > 256)
                len = 256;
            out = alloca(len+1);
            ism_common_validUTF8Replace(ptr, len, out, UR_NoControl | UR_NoNonchar, '?');
            MBTRACE(MBERROR, 5, "Invalid MQTT property name=%s, error=%s\n", fld->name, "UnicodeNotValid");
            return MQTTRC_MalformedPacket;
        }
    }
    if (fld->type == MPT_NamePair) {
    	rc = validateNamePair(ptr, len);
        if(rc) {
			return rc;
        }
    }

    switch (fld->id) {
    case MPI_UserProperty:
		{
			char *eos;
			int namelen = READINT16(ptr);
			int valuelen = READINT16(ptr + 2 + namelen);
			char userPropName[64] = {0};
			char userPropValue[64] = {0};
			READUTF(userPropName, ptr + 2,namelen);
			READUTF(userPropValue, ptr + 4 + namelen, valuelen);
			if(strncmp(userPropName, UPROP_TIMESTAMP, namelen) == 0){
				mmsg->senderTimestamp = strtod(userPropValue, &eos);
			} else if (strncmp(userPropName, UPROP_SEQNUM, namelen) == 0) {
				mmsg->seqnum = strtoull(userPropValue, &eos, 16);
			} else if (strncmp(userPropName, UPROP_STREAMID, namelen) == 0) {
				mmsg->streamID = strtoull(userPropValue, &eos, 10);
			}
		}
    	break;
    case MPI_SubID:
		if (value == 0) {
			MBTRACE(MBERROR, 5, "Bad subscription ID was received (value=%d) for client %s (line=%d)\n", value, client->clientID, client->line);
			return MQTTRC_ProtocolError;
		}
		mmsg->subID = value;
		break;
    case MPI_TopicAlias:
        mmsg->topic_alias = value;
        break;
	case MPI_Reason:
		{
			if(len > REASON_STR_LEN)
				len = REASON_STR_LEN;
			memset(mmsg->reasonMsg, 0, REASON_STR_LEN);
			READUTF(mmsg->reasonMsg, ptr, len);
			mmsg->reasonMsgLen = len;
		}
		break;
    case MPI_MsgExpire:
    	/* For now mqttbench will not do anything with the message expiry property from the server */
		break;
    case MPI_PayloadFormat:
        if (value>1) {
        	MBTRACE(MBERROR, 5, "Bad payload format indicator was received (value=%d) for client %s (line=%d)\n", value, client->clientID, client->line);
            return MQTTRC_ProtocolError;
        }
        break;
    case MPI_ContentType:
		/* For now mqttbench will not do anything with the message content type property from the server */
		break;
    case MPI_ReplyTopic:
    	/* For now mqttbench will not do anything with the message reply topic property from the server, TODO in the future
    	 * mqttbench could be made to republish to the received message on the replyTopic */
		break;
    case MPI_Correlation:
		/* For now mqttbench will not do anything with the message correlation date property from the server, TODO in the future
		 * mqttbench could be made to send the correlation data on the replyTopic */
		break;
    case MPI_SessionExpire:
        mmsg->sessionExpiryTTL = value;
        break;
    case MPI_MaxReceive:
		if (value == 0 || value > 65535) {
			MBTRACE(MBERROR, 5, "Max receive MQTT property is invalid (value=%d) for client %s (line=%d)\n", value, client->clientID, client->line);
			return MQTTRC_ProtocolError;
		}
		mmsg->serverMaxInflight = value;
		if (mmsg->serverMaxInflight < client->maxInflightMsgs) {
			MBTRACE(MBINFO, 6, "The max inflight window for client %s (line=%d) was adjusted based on the MQTT server limits (previous=%d, new=%d)\n",
					           client->clientID, client->line, client->maxInflightMsgs, mmsg->serverMaxInflight);
			client->maxInflightMsgs = mmsg->serverMaxInflight;    	// adjust the client max inflight limit based on the server advertised max inflight limit
		}
		break;
	case MPI_MaxTopicAlias:
		mmsg->topicAliasMaxOut = value;
		if (client->topicalias_count_out > 0 && client->destTxAliasMapped == NULL) {	// assuming that MaxTopicAlias will not change on a subsequent reconnect
			if (mmsg->topicAliasMaxOut < client->topicalias_count_out) {
				MBTRACE(MBWARN, 6, "The outbound topic alias count was decreased (based on MQTT server limits) for client %s (line=%d), previous=%d, new=%d\n",
									client->clientID, client->line, client->topicalias_count_out, mmsg->topicAliasMaxOut);

				client->topicalias_count_out = mmsg->topicAliasMaxOut;
			}
			client->destTxAliasMapped = calloc(client->topicalias_count_out, sizeof(uint8_t)); 	// allocate the topic alias mapping state table
		}
		break;
	case MPI_MaxPacketSize:
		if (value == 0) {
			MBTRACE(MBERROR, 5, "Max packet size MQTT property is invalid (value=%d) for client %s (line=%d)\n", value, client->clientID, client->line);
			return MQTTRC_ProtocolError;
		}
		if (value < g_TxBfrSize) {
			MBTRACE(MBERROR, 5, "Max packet size allowed by the MQTT server is smaller than the max TX packet size required by this test (serverMax=%d bytes, clientMax=%d bytes)\n",
								value, g_TxBfrSize);
			return MQTTRC_PacketTooLarge;
		}
		break;
	case MPI_ClientID:
		{
			if(len > MAX_CLIENTID_LEN) {
				MBTRACE(MBERROR, 5, "The generated client ID provided by the MQTT server is larger than MAX_CLIENTID_LEN(%d) (value=%d)\n",
									MAX_CLIENTID_LEN, len);
				return MQTTRC_ProtocolError;
			}
			char clientID[MAX_CLIENTID_LEN] = {0};
			client->clientIDLen = len;
			READUTF(clientID, ptr, len);
			free(client->clientID); 				// free the old client ID (most likely a zero length empty string)
			client->clientID = strdup(clientID);
		}
		break;
	case MPI_MaxQoS:
		/* messagesight supports all QoS so nothing to do here, TODO later we could check this against client->txQoSBitMap and client->rxQoSBitMap
		 * and close the connection if either is greater than MaxQoS from the server */
		mmsg->serverMaxQoS = value;
		break;
	case MPI_RetainAvailable:
		break;
    }

    return rc;
}

/* *************************************************************************************
 * submitPublish
 *
 * Description:  Send messages (Producer) for all QoS levels (0, 1 and 2)
 *
 * Note:         Only supporting Asynchronous publishing in this application.
 *
 *   @param[in]  client              = Specific mqttclient to use.
 *   @param[in]  msg                 = Message to be PUBLISHed
 *   @param[in]  topicIdx            = Topic Index for this publisher.
 *   @param[in]  txBuf               = TX Buffer that contains the current msg to send
 * *************************************************************************************/
void submitPublish(mqttclient_t *client, message_object_t *msgObj, mqttmessage_t *msg, int topicIdx, ism_byte_buffer_t *txBuf)
{
	int rc = 0;
	destTuple_t *pDestTuple;
	uint8_t qos;

    char xbuf[2048];                                /* Buffer used for holding properties. */
    concat_alloc_t propsBfr = {xbuf, sizeof xbuf};  /* Structure used with Server Utils. */

	mqttbench_t *mbinst = client->mbinst;
	if(topicIdx < client->destTxListCount) {
		pDestTuple = client->destTxList[topicIdx];

		msg->sendTopicName = 1;  // by default send the topic name
		if(pDestTuple->topicAlias > 0 && pDestTuple->topicAlias <= client->topicalias_count_out) {
			msg->sendTopicAlias = 1;

			if(client->destTxAliasMapped[topicIdx])
				msg->sendTopicName = 0;
		} else {
		    msg->sendTopicAlias = 0;
		}
	} else {
		MBTRACE(MBERROR, 5, "Internal Error: attempting to publish a message without having a topic to publish it to.  Exiting...\n");
		g_RequestedQuit = 1;
		g_MBErrorRC = RC_INTERNAL_ERROR;
		return;
	}

	qos = pDestTuple->topicQoS;

	/* If QoS is 1 or 2, then need to assign the msg id and update the currInFlightMessages */
	if (qos) {
		/* ----------------------------------------------------------------------------
		 * Check to see if reached max InFlight messages.  If not, then assign the
		 * message/packet ID for this publish.  If = to the number of max InFlight messages
		 * then there will be no Available message id for the next publish, and need to
		 * set flag that there are no msgIDs
		 * ---------------------------------------------------------------------------- */
		pthread_spin_lock(&(client->mqttclient_lock));
		rc = findAvailableMsgId(client, PUB_MSGID_AVAIL, client->maxInflightMsgs);
		if(rc) {										/* did not find an available message id within the search limit */
			pthread_spin_unlock(&(client->mqttclient_lock));
			ism_common_returnBuffer(txBuf,__FILE__,__LINE__);   			/* return the transmit buffer back to the pool. This publisher will have to wait until it's next turn */
			if(rc == RC_MSGID_NONE_AVAILABLE){
				MBTRACE(MBWARN, 7, "Client %s at line %d has no available msg IDs for publishing QoS %d message to topic %s, skipping this publishers turn\n",
									client->clientID, client->line, qos, pDestTuple->topicName);
			}
			return;
		} else {
			msgid_info_t *msgIdInfo = &client->inflightMsgIds[client->availMsgId];
			if (qos == 1) {
				msgIdInfo->state = PUB_MSGID_PUBACK;	/* set the MsgID status to PUBACK */
				msgIdInfo->destTuple = pDestTuple;		/* save a reference to the destTuple, used when the ACK is received */
			} else {
				if ((msgIdInfo->state & PUB_MSGID_UNKNOWN_STATE) > 0) {
					msg->dupflag = 1;
				}
				msgIdInfo->state = PUB_MSGID_PUBREC;	/* set the MsgID status to PUBREC */
				msgIdInfo->destTuple = pDestTuple;		/* save a reference to the destTuple, used when the ACK is received */
			}

			client->currInflightMessages++;         	/* increment InFlight # of msgs */
			msg->msgid = client->availMsgId; 			/* assign first available msg ID to this message */
		}
		pthread_spin_unlock(&(client->mqttclient_lock));
	} /* if (qos > 0) */

	/* Update the QoS and retain settings for this topic. */
	msg->qos = pDestTuple->topicQoS;
	msg->retain = pDestTuple->retain;
	msg->generatedMsg = msgObj->msgDirObj->generated;

	/* Create the PUBLISH message. */
	createMQTTMessagePublish(txBuf, pDestTuple, msgObj, msg, &propsBfr, g_ClkSrc, client->useWebSockets, client->mqttVersion);

	/* --------------------------------------------------------------------------------
	 * If requested PUBACK latency, then need to make sure the QoS is > 0 and that the
	 * client's published message ID = 0;  If so then take a timestamp and update the
	 * client's message ID with the current one.
	 * -------------------------------------------------------------------------------- */
	if (((g_LatencyMask & CHKTIMESEND) > 0) && qos && (client->pubMsgId == 0)) {
		client->pubMsgId = msg->msgid;		/* client->pubMsgId is set to 0 in doPubACK and doPubREC */
		client->pubSubmitTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());
	}

	/* --------------------------------------------------------------------------------
	 * Due to race condition found with reconnect, the client protocol state needs to
	 * be checked to ensure that the correct state is set to send.
	 * -------------------------------------------------------------------------------- */
	if (client->protocolState == MQTT_PUBSUB) {
	    if(client->traceEnabled || g_MBTraceLevel == 9) {
            char obuf[256] = {0};
            sprintf(obuf, "DEBUG: %10s client @ line=%d, ID=%s submitting an MQTT PUBLISH packet",
                          "PUBLISH:", client->line, client->clientID);
            traceDataFunction(obuf, 0, __FILE__, __LINE__, txBuf->buf, txBuf->used, 512);
        }
		/* Submit the job to the IO Thread and check return code. */
		rc = submitIOJob(client->trans, txBuf);
		if (rc == 0) {
			volatile ioProcThread_t * iop = client->trans->ioProcThread;
			client->currTxMsgCounts[PUBLISH]++;   /* per client publish count */
			__sync_add_and_fetch(&(iop->currTxMsgCounts[PUBLISH]),1);		/* MUST be atomically incremented because this is called from submitter threads */
			__sync_add_and_fetch(&(iop->currTxQoSPublishCounts[qos]),1);	/* MUST be atomically incremented because this is called from submitter threads */
			mbinst->submitCnt++;				/* per producer thread message submit count */

			if((pDestTuple->topicAlias > 0) && (pDestTuple->topicAlias <= client->topicalias_count_out)){
				client->destTxAliasMapped[topicIdx] = 1; /* update topic alias mapping state table */
			}

			/* ------------------------------------------------------------------------
			 * Since the current available message is valid then need to update the
			 * client->availMsgId to the next available one. Need to check to see if
			 * incrementing it by 1 will cause it to wrap.  If so reset = 1.
			 * ------------------------------------------------------------------------ */
			pthread_spin_lock(&(client->mqttclient_lock));
			if (++client->availMsgId >= client->maxInflightMsgs)
				client->availMsgId = 1;
			pthread_spin_unlock(&(client->mqttclient_lock));
		} else {
			/* ------------------------------------------------------------------------
			 * If the QoS is either 1 or 2 then need to reset the msgID as available
			 * since submitIOJob returned an error.  QoS 0 there is no msgID array.
			 * ------------------------------------------------------------------------ */
			if (qos) {
				pthread_spin_lock(&(client->mqttclient_lock));
				msgid_info_t *msgIdInfo = &client->inflightMsgIds[client->availMsgId];
				msgIdInfo->state = PUB_MSGID_AVAIL;			/* Set the msgid status to AVAIL */
				client->currInflightMessages--;          	/* Decrement InFlight # of msgs */
				pthread_spin_unlock(&(client->mqttclient_lock));
			}

			MBTRACE(MBERROR, 1, "Unable to submit PUBLISH message to I/O thread (rc: %d).\n", rc);
		}
	} else {
		ism_common_returnBuffer(txBuf,__FILE__,__LINE__);     /* Return the buffer back to the pool */
	}
} /* submitPublish */

/* *************************************************************************************
 * doCreatePubRel
 *
 * Description:  Handle any PUBREC messages with a PUBREL message.  This is only
 *               applicable to the producer at QoS level 2.
 *
 *   @param[in]  client              = Specific mqttclient to use.
 *   @param[in]  pDest               = Destuple for the topic.
 *   @param[in]  msgId               = Message ID associate with this PUBREL.
 * *************************************************************************************/
void doCreatePubRel(mqttclient_t *client, destTuple_t *pDest, uint16_t msgId)
{
    int rc = 0;
    ism_byte_buffer_t *txBuf;
    volatile ioProcThread_t *iop = client->trans->ioProcThread;

    /* Get an IOP tx buffer to send the PUBREL */
    txBuf = iop->txBuffer;
    if (txBuf == NULL) {
        iop->txBuffer = getIOPTxBuffers(iop->sendBuffPool, iop->numBfrs, 1);
        txBuf = iop->txBuffer;
    }
    iop->txBuffer = txBuf->next;

    /* Create the PUBREL message with a QoS = 1 per spec */
    createMQTTMessageAck(txBuf, PUBREL, 1, msgId, client->useWebSockets);

    /* Submit the job to the IO Thread and check return code. */
    rc = submitIOJob(client->trans, txBuf);
    if (rc == 0) {
        client->currTxMsgCounts[PUBREL]++;   /* per client PUBREL count */
        iop->currTxMsgCounts[PUBREL]++;
        client->numPubRecs--; 				 /* decrement number of msgs in PUBREC state, i.e. PUBREC received, but not yet transmitted PUBREL */
		pDest->currPUBRELCount++;    		 /* increment topic PUBREL count */
    } else
    	MBTRACE(MBERROR, 1, "Unable to submit PUBREL message to I/O thread (rc: %d).\n", rc);
} /* doCreatePubRel */

/* *************************************************************************************
 * Process a UNSUBACK message received from the MQTT server
 *
 *   @param[in]  client              = the mqtt client receiving the UNSUBACK
 *   @param[in]  buf              	 = pointer to the current offset into the UNSUBACK message
 *   @param[in]  msgLen           	 = length of the UNSUBACK message to be processed
 *   @param[in]  mmsg				 = object used to store data parsed from the received MQTT packet
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
static int doUnSubACK(mqttclient_t *client, char *buf, int msgLen, mqttmessage_t *mmsg) {
	int rc = 0;
	volatile ioProcThread_t *iop = client->trans->ioProcThread;

	uint16_t msgId = (uint16_t)READINT16(buf);             	/* Read the msg id in the message */
	rc = chkmsgID(client, msgId, "UNSUBACK");
	if(rc)
		return rc;

	pthread_spin_lock(&(client->mqttclient_lock));
	msgid_info_t msgIDInfo = client->inflightMsgIds[msgId];
	if(msgIDInfo.state == SUB_MSGID_UNSUBACK) {		/* Need to update the msgID that it received the UNSUBACK. */
		msgIDInfo.state = SUB_MSGID_AVAIL; 			/* mark this msg ID as available */
		client->currInflightMessages--;      		/* Decrement the number of messages in flight. */
	} else {
		MBTRACE(MBERROR, 1, "Received an unexpected UNSUBACK from a client which did not unsubscribe\n");
		return RC_MSGID_BAD;
	}
	pthread_spin_unlock(&(client->mqttclient_lock));

	int numMsgs = msgLen - MQTT_MSGID_SIZE; 	/* number of UnSubscriptions in the UNSUBACK message */
	if (client->mqttVersion == MQTT_V311 || client->mqttVersion == MQTT_V5) {
		/* ------------------------------------------------------------------------
		 * MQTT V5 requires the following:
		 *    a) Decode the length of the properties field.
		 *    b) Subtract the size of the MsgID, length of properties field, and
		 *       length of the properties in order to determine the number of msgs.
		 * ------------------------------------------------------------------------ */
		if (client->mqttVersion == MQTT_V5) {
			int numBytes = 0;
			int propsLen = 0;

			buf += MQTT_MSGID_SIZE;
			msgLen -= MQTT_MSGID_SIZE;
			numBytes = decodeMQTTMessageLength(buf, (buf + msgLen), &propsLen);
			if (numBytes < 0) {
				MBTRACE(MBERROR, 1, "Decode MQTT Message failed with rc: %d", numBytes);
			}

			numMsgs -= (propsLen + numBytes);

			/* -------------------------------------------------------------------
			 * If Property Length is > 0, then there are properties are present.
			 * Properties supported for UNSUBACK are: Reason String & User Property.
			 * ------------------------------------------------------------------- */
			if ((propsLen < 0) || (propsLen > msgLen)) {
				rc = checkMqttPropFields((char *) buf, propsLen, g_mqttCtx5, CPOI_UNSUBACK, mqttPropertyCheck, mmsg);
				if (rc) {
					MBTRACE(MBERROR, 5, "Client %s (line=%d) failed to validate the MQTT properties on a UNSUBACK message, initiating a disconnect\n", client->clientID, client->line);
					client->disconnectRC = rc;
					performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
					return rc;
				}
			}

			/* Add propsLen to buf, and Subtract propsLen from msgLen.*/
			buf += propsLen;
			msgLen -= propsLen;
		}

		client->unsubackCount += numMsgs;

		for ( int i = 0 ; i < numMsgs ; i++ ) {
			int unsubRC = READCHAR(buf++);
			if (unsubRC) {
				iop->currBadRxRCCount++;
				client->badRxRCCount++;

				if (MQTT_RC_CHECK(unsubRC)) {
					MBTRACE(MBERROR, 1, "Client %s (line=%d) received a UNSUBACK message from the MQTT server with an invalid reason code %d, sending DISCONNECT message\n",
										client->clientID, client->line, unsubRC);
					client->disconnectRC = MQTTRC_ProtocolError;
					performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
					return MQTTRC_ProtocolError;
				}

				const char *reason = g_ReasonStrings[unsubRC];
		        MBTRACE(MBERROR, 1, "Client %s (line=%d) received an UNSUBACK message from the MQTT server with reason code %d (%s) and server reason message \"%s\"\n",
		                            client->clientID, client->line, unsubRC, reason ? reason : "", mmsg->reasonMsgLen ? mmsg->reasonMsg : "");
			}
		}
	} else if (client->mqttVersion == MQTT_V3) {
		client->unsubackCount += numMsgs;
	}

	if (client->currInflightMessages == 0) {
		client->protocolState = MQTT_UNSUBSCRIBED;
		client->allTopicsUnSubAck = 1;    			/* Set flag that all topics have received UNSUBACK. */
		performMQTTDisconnect((ioProcThread_t *) iop, client->trans);
	}

	return rc;
} /* doUnSubACK */

/* *************************************************************************************
 * Process a SUBACK message received from the MQTT server
 *
 *   @param[in]  client              = the mqtt client receiving the SUBACK
 *   @param[in]  buf              	 = pointer to the current offset into the SUBACK message
 *   @param[in]  msgLen           	 = length of the SUBACK message to be processed
 *   @param[in]  mmsg				 = object used to store data parsed from the received MQTT packet
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
static int doSubACK(mqttclient_t *client, char *buf, int msgLen, mqttmessage_t *mmsg) {
	int rc = 0;
	mqttbench_t *mbinst = client->mbinst;
	volatile ioProcThread_t *iop = client->trans->ioProcThread;

	uint16_t msgId = (uint16_t)READINT16(buf);   /* Read the msgID from the message */
	rc = chkmsgID(client, msgId, "SUBACK");
	if(rc)
		return rc;

	if (client->destRxListCount == 0) {
		MBTRACE(MBERROR, 1, "Received an unexpected SUBACK for client @ line=%d, ID=%s, which has no subscriptions\n",
							client->line, client->clientID);
		return RC_UNEXPECTED_MSG;
	} else {
		pthread_spin_lock(&(client->mqttclient_lock));
		msgid_info_t *msgIdInfo = &client->inflightMsgIds[msgId];
        destTuple_t *subscription = msgIdInfo->destTuple;
		if (msgIdInfo->state == SUB_MSGID_SUBACK) {
			client->currInflightMessages--;    /* decrement the number of messages in flight. */
		} else {
			MBTRACE(MBERROR, 1, "Expected msg ID state %d, actual msg ID state %d. msg ID=%u received message type=SUBACK\n",
								SUB_MSGID_SUBACK, msgIdInfo->state, msgId);
			return RC_MSGID_BAD;
		}
		pthread_spin_unlock(&(client->mqttclient_lock));

		/* ----------------------------------------------------------------------------
		 * If measuring Subscriptions or TCP-Subscribe then time to get the t2 for the
		 * latency and update the histogram.
		 * ---------------------------------------------------------------------------- */
		if ((g_LatencyMask & (CHKTIMESUBSCRIBE | CHKTIMETCP2SUBSCRIBE)) > 0) {
			uint32_t latency = (uint32_t)(((g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime()) -
					client->subscribeConnReqSubmitTime) * iop->subscribeHist->histUnits);
			if ((g_LatencyMask & CHKTIMETCP2SUBSCRIBE) > 0) {
				latency += (client->tcpLatency + client->mqttLatency);
			}

			iop->subscribeHist->totalSampleCt++;   /* Update Total # of Samples Received. */

			if (latency > iop->subscribeHist->max)   /* Update max latency if applicable. */
				iop->subscribeHist->max = latency;

			/* ------------------------------------------------------------------------
			 * Check whether the latency falls within the scope of the histogram.  If
			 * not, then increment the counter for larger than histogram size and update
			 * the 1 sec and 5 sec counters if applicable.
			 * ------------------------------------------------------------------------ */
			if (latency < iop->subscribeHist->histSize)
				iop->subscribeHist->histogram[latency]++;  /* Update histogram since within scope. */
			else {
				iop->subscribeHist->big++;    /* Update larger than histogram. */

				if (latency >= g_Equiv1Sec_Conn) {  /* Update if larger than 1 sec. */
					iop->subscribeHist->count1Sec++;
					if (latency >= (g_Equiv1Sec_Conn * 5)) /* Update if larger than 5 sec. */
						iop->subscribeHist->count5Sec++;
				}
			}
		}

		int numMsgs = msgLen - MQTT_MSGID_SIZE;										/* number of subscriptions in the SUBACK message */
		if (client->mqttVersion == MQTT_V311 || client->mqttVersion == MQTT_V5) { 	/* for v3.1.1 and v5 client check the SUBACK rc codes */
			/* ------------------------------------------------------------------------
			 * MQTT V5 requires the following:
			 *    a) Decode the length of the properties field.
			 *    b) Subtract the size of the MsgID, length of properties field, and
			 *       length of the properties in order to determine the number of msgs.
			 * ------------------------------------------------------------------------ */
			if (client->mqttVersion == MQTT_V5) {
				int vilen = 0;
				int propsLen = 0;

				buf += MQTT_MSGID_SIZE;
				msgLen -= MQTT_MSGID_SIZE;
				vilen = decodeMQTTMessageLength(buf, (buf + msgLen), &propsLen);
				if (vilen < 0) {
					MBTRACE(MBERROR, 1, "Decode MQTT Message failed with rc: %d", vilen);
				}

				numMsgs -= (propsLen + vilen);

				/* -------------------------------------------------------------------
				 * If Property Length is > 0, then there are properties are present.
				 * Properties supported for SUBACK are: Reason String & User Property.
				 * ------------------------------------------------------------------- */
				if ((propsLen < 0) || (propsLen > msgLen)) {
					rc = checkMqttPropFields((char *) buf, propsLen, g_mqttCtx5, CPOI_SUBACK, mqttPropertyCheck, mmsg);
					if (rc) {
						MBTRACE(MBERROR, 5, "Client %s (line=%d) failed to validate the MQTT properties on a SUBACK message, initiating a disconnect\n", client->clientID, client->line);
						client->disconnectRC = rc;
						performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
						return rc;
					}
				}

				/* Add propsLen + vilen to buf, and Subtract propsLen + vilen from msgLen.*/
				buf += (propsLen + vilen);
				msgLen -= (propsLen + vilen);
			}

			for ( int i = 0 ; i < numMsgs ; i++ ) {
				int subRC = READCHAR(buf++);
				if (subRC >= 0x80) {   // only RC >= 0x80 are errors for a SUBACK
					iop->currBadRxRCCount++;
					iop->currFailedSubscriptions++;
					client->badRxRCCount++;
					client->failedSubscriptions++;

					if(subscription) {
					    subscription->badRCCount++;
					}

					if (MQTT_RC_CHECK(subRC)) {
						MBTRACE(MBERROR, 1, "Client %s (line=%d) received a SUBACK message from the MQTT server with an invalid reason code %d, sending DISCONNECT message\n",
											client->clientID, client->line, subRC);
						client->disconnectRC = MQTTRC_ProtocolError;
						performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
						return MQTTRC_ProtocolError;
					}

					const char *reason = g_ReasonStrings[subRC];
			        MBTRACE(MBERROR, 1, "Client %s (line=%d) received a SUBACK message from the MQTT server with reason code %d (%s) and server reason message \"%s\"\n",
			                            client->clientID, client->line, subRC, reason ? reason : "", mmsg->reasonMsgLen ? mmsg->reasonMsg : "");
				} else {
				    client->subackCount++; // do not count bad RC (i.e. subscriptions which were not successfully created)
				}
			}
		} else if (client->mqttVersion == MQTT_V3) {
			client->subackCount += numMsgs;
		}

		if (client->protocolState != MQTT_DISCONNECT_IN_PROCESS) {
			client->protocolState = MQTT_PUBSUB;
		}

		if (client->subackCount == client->destRxListCount) {							/* Check to see if this client has received all its SUBACKs */
			client->allSubscribesComplete = 1;

			__sync_add_and_fetch(&(mbinst->numSubscribes), client->subackCount);		/* increment subscription complete count for the submitter thread that this client is assigned to */
			if (mbinst->numSubscribes == mbinst->numSubsPerThread) {
				mbinst->subscribeTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime()) -
					mbinst->startSubscribeTime;  	/* Calculate the total subscription latency for all clients
													 * assigned to this submitter thread */
				MBTRACE(MBINFO, 1, "Successfully created all subscriptions for all clients assigned to submitter thread %d.\n", mbinst->id);
				fprintf(stdout, "Successfully created all subscriptions for all clients assigned to submitter thread %d.\n", mbinst->id);
				fflush(stdout);
			}

			/* Check to see  if lingerSecs is set and if so schedule timer. */
			if (client->lingerTime_ns > 0)
				addLingerTimerTask(client);
		}
	}

	return rc;
} /* doSubACK */

/* *************************************************************************************
 * Process a CONNACK message received from the MQTT server
 *
 *   @param[in]  client              = the mqtt client receiving the CONNACK
 *   @param[in]  buf              	 = pointer to the current offset into the CONNACK message
 *   @param[in]  msgLen              = length of the CONNACK message (including Fixed Header)
 *   @param[in]  mmsg				 = object used to store data parsed from the received MQTT packet
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
static int doConnACK(mqttclient_t *client, char *buf, int msgLen, mqttmessage_t *mmsg) {
	int rc = 0;
	mqttbench_t *mbinst = client->mbinst;
	volatile ioProcThread_t *iop = client->trans->ioProcThread;

	/* Update CONNACK message counters - per submitter thread, and per IOP thread */
	__sync_add_and_fetch(&(mbinst->currConnAckMsgCount),1);
	iop->currConsConnAckCounts++;

	/* Update the time for the last CONNACK for this client. */
	client->lastMqttConnAckTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());

	Connack *connack = (Connack *) buf;
	buf += 2; 		// advance the pointer beyond the CONNACK header and subtract from remaining length
	msgLen -= 2;

	/* Process MQTT v5 CONNACK Properties */
	if(client->mqttVersion >= MQTT_V5) {
		if(*buf != 0){ // this CONNACK has properties, so process them
			int vilen = 0;
			int mproplen = ism_common_getMqttVarIntExp((const char *)buf, msgLen, &vilen);
			buf += vilen;
			msgLen -= vilen;
			if (mproplen < 0 || mproplen > msgLen) {
				MBTRACE(MBERROR, 5, "Invalid MQTT property length was found in a CONNACK message destined for client %s (line=%d)\n",
									client->clientID, client->line);
				client->disconnectRC = MQTTRC_MalformedPacket;
				performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
				return MQTTRC_MalformedPacket;
			} else {
				rc = checkMqttPropFields(buf, mproplen, g_mqttCtx5, CPOI_CONNACK, mqttPropertyCheck, mmsg);
				if (rc) {
					iop->currBadRxRCCount++;
					client->badRxRCCount++;

					if (MQTT_RC_CHECK(rc)) {
						MBTRACE(MBERROR, 1, "Client %s (line=%d) failed to validate the MQTT properties on a CONNACK message, invalid reason code %d, sending DISCONNECT message\n",
											client->clientID, client->line, rc);
						client->disconnectRC = MQTTRC_ProtocolError;
						performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
						return MQTTRC_ProtocolError;
					}

					const char *reason = g_ReasonStrings[rc];
					MBTRACE(MBERROR, 5, "Client %s (line=%d) failed to validate the MQTT properties on a CONNACK message, rc=%d (%s), initiating a disconnect\n",
										client->clientID, client->line, rc, reason ? reason : "");
					client->disconnectRC = rc;
					performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
					return rc;
				}
			}

			/* Add propsLen to buf, and Subtract propsLen from msgLen.*/
			buf += mproplen;
			msgLen -= mproplen;
		}
	}

	int connackRC = connack->rc;
	if (connackRC) {
		iop->currBadRxRCCount++;
		client->badRxRCCount++;

		if (MQTT_RC_CHECK(connackRC)) {
			MBTRACE(MBERROR, 1, "Client %s (line=%d) received a CONNACK message from the MQTT server with an invalid reason code %d, sending DISCONNECT message\n",
								client->clientID, client->line, connackRC);
			client->disconnectRC = MQTTRC_ProtocolError;
			performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
			return MQTTRC_ProtocolError;
		}

		const char *reason = "Unknown Reason Code";
		if(client->mqttVersion >= MQTT_V5) {
			if(g_ReasonStrings[connackRC]) {
				reason = g_ReasonStrings[connackRC];
			}
		} else {
			if (connackRC == MQTTRC_V311NotAuthorized) {
				reason = "Not authorized";
			}
		}
		MBTRACE(MBERROR, 1, "Client %s (line=%d) received a CONNACK message from the MQTT server with reason code %d (%s) and server reason message \"%s\"\n",
							client->clientID, client->line, connackRC, reason ? reason : "", mmsg->reasonMsgLen ? mmsg->reasonMsg : "");
		performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
		return connackRC;
	} else {
		/* mqttbench does NOT persist client state, so all we can do is log whether the server found a session state for this client */
		uint8_t sessionPresent = connack->flags.sessPresent;
		client->sessionPresent = sessionPresent;
		MBTRACE(MBDEBUG, 7, "The MQTT server %s a session state for this client\n", sessionPresent ? "found" : "did NOT find");

		/* Check to see if doing latency first and if so then handle accordingly. */
		int tmpLatencyType;
		if (client->clientType == PRODUCER)
			tmpLatencyType = g_LatencyMask & (CHKTIMEMQTTCONN | CHKTIMETCP2MQTTCONN);
		else
			tmpLatencyType = g_LatencyMask & (CHKTIMEMQTTCONN | CHKTIMETCP2MQTTCONN | CHKTIMETCP2SUBSCRIBE);

		/* ----------------------------------------------------------------------------
		 * If measuring MQTT Connection then time to get the t2 for the latency and
		 * update the histogram.
		 * ---------------------------------------------------------------------------- */
		if (tmpLatencyType > 0) {
			uint32_t latency = (uint32_t)(((g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime()) -
					client->mqttConnReqSubmitTime) * iop->mqttConnHist->histUnits);
			if ((g_LatencyMask & CHKTIMETCP2SUBSCRIBE) == 0) {
				if (g_LatencyMask == CHKTIMETCP2MQTTCONN)
					latency += client->tcpLatency;

				iop->mqttConnHist->totalSampleCt++;   /* Update Total # of Samples Received. */

				if (latency > iop->mqttConnHist->max)   /* Update max latency if applicable. */
					iop->mqttConnHist->max = latency;

				/* --------------------------------------------------------------------
				 * Check whether the latency falls within the scope of the histogram.
				 * If not, then increment the counter for larger than histogram size and
				 * update the 1 sec and 5 sec counters if applicable.
				 * -------------------------------------------------------------------- */
				if (latency < iop->mqttConnHist->histSize)
					iop->mqttConnHist->histogram[latency]++;  /* Update histogram since within scope. */
				else {
					iop->mqttConnHist->big++;   /* Update larger than histogram. */

					if (latency >= g_Equiv1Sec_Conn) {  /* Update if larger than 1 sec. */
						iop->mqttConnHist->count1Sec++;
						if (latency >= (g_Equiv1Sec_Conn * 5))  /* Update if larger than 5 sec. */
							iop->mqttConnHist->count5Sec++;
					}
				}
			} else
				client->mqttLatency = latency;
		} /* if (tmpLatencyType > 0) */

		/* Update the counter for number of CONNECTS for this particular thread. */
		__sync_add_and_fetch(&(mbinst->numMQTTConnects),1);
		if (mbinst->numMQTTConnects == mbinst->numClients) {
			g_EndTimeConnTest = getCurrTime();
			mbinst->mqttConnTime = g_EndTimeConnTest - mbinst->startMQTTConnTime;
			MBTRACE(MBINFO, 1, "Successfully completed the MQTT handshake for all clients assigned to submitter thread %d.\n", mbinst->id);
			fprintf(stdout, "Successfully completed the MQTT handshake for all clients assigned to submitter thread %d.\n", mbinst->id);
			fflush(stdout);
		}

		if ((client->clientType & CONSUMER) > 0) { /* consumer or dual client */
			/* Update the protocolState */
			if ((client->protocolState != MQTT_DISCONNECT_IN_PROCESS)) {
				client->protocolState = MQTT_CONNECTED;
			}

			/* If requested Subscription latency then set the timer if not set. */
			if ((g_LatencyMask & (CHKTIMETCP2SUBSCRIBE | CHKTIMESUBSCRIBE)) > 0) {
				pthread_spin_lock(&mbinst_lock);
				if (mbinst->startSubscribeTime == 0)
					mbinst->startSubscribeTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());
				pthread_spin_unlock(&mbinst_lock);
			}

			rc = submitSubscribe(client);
			if(rc) {
				/* Failed to subscribe we should close the connection */
				MBTRACE(MBERROR, 5, "Client @ line=%d, ID=%s failed to subscribe, sending MQTT DISCONNECT and closing the connection.\n",
									client->line, client->clientID);
				performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
			}
		} else { /* pure producer or connect only*/
			/* Update the protocolState */
			if (client->protocolState != MQTT_DISCONNECT_IN_PROCESS) {
				if (client->clientType == PRODUCER) {
					client->protocolState = MQTT_PUBSUB;
				} else {
					client->protocolState = MQTT_CONNECTED;
				}
			}

			/* ----------------------------------------------------------------
			 * Check to see  if the clients linger time is set or not.  If so,
			 * then schedule the timer.
			 * ---------------------------------------------------------------- */
			if (client->lingerTime_ns > 0) {
				addLingerTimerTask(client);
			}
		}
	} /* end successful CONNACK (i.e good RC in CONNACK)*/

	return rc;
} /* doConnACK */

/* *************************************************************************************
 * Process a DISCONNECT message received from the MQTT server , only v5 client should
 * receive a DISCONNECT message from the MQTT server.
 *
 *   @param[in]  client              = the mqtt client receiving the DISCONNECT
 *   @param[in]  buf              	 = pointer to the current offset into the DISCONNECT message
 *   @param[in]  msgLen              = length of the DISCONNECT message (including Fixed Header)
 *   @param[in]  mmsg				 = object used to store data parsed from the received MQTT packet
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
static int doDisconnect(mqttclient_t *client, char *buf, int msgLen, mqttmessage_t *mmsg) {
	int rc = 0;
	mqttbench_t *mbinst = client->mbinst;
	volatile ioProcThread_t *iop = client->trans->ioProcThread;

	/* Update DISCONNECT message counters - per submitter thread, and per IOP thread */
	__sync_add_and_fetch(&(mbinst->currDisconnectMsgCount),1);
	iop->currDisconnectMsgCounts++;

	/* Process MQTT v5 DISCONNECT Properties */
	if(client->mqttVersion >= MQTT_V5) {
		int serverDisconnectRC = READCHAR(buf);
		buf++;
		msgLen--;

		if(*buf != 0){ // this DISCONNECT has properties, so process them
			int vilen = 0;
			int mproplen = ism_common_getMqttVarIntExp((const char *)buf, msgLen, &vilen);
			buf += vilen;
			msgLen -= vilen;
			if (mproplen < 0 || mproplen > msgLen) {
				MBTRACE(MBERROR, 5, "Invalid MQTT property length was found in a DISCONNECT message destined for client %s (line=%d)\n",
									client->clientID, client->line);
				return MQTTRC_MalformedPacket;
			} else {
				rc = checkMqttPropFields((char *) buf, mproplen, g_mqttCtx5, CPOI_S_DISCONNECT, mqttPropertyCheck, mmsg);
				if (rc) {
					MBTRACE(MBERROR, 5, "Client %s (line=%d) failed to validate the MQTT properties on a DISCONNECT message, initiating a disconnect\n", client->clientID, client->line);
					return rc;
				}
			}

			/* Add propsLen to buf, and Subtract propsLen from msgLen.*/
			buf += mproplen;
			msgLen -= mproplen;
		}

		if(serverDisconnectRC) {
			iop->currBadRxRCCount++;
			client->badRxRCCount++;

			if (MQTT_RC_CHECK(serverDisconnectRC)) {
				MBTRACE(MBERROR, 1, "Client %s (line=%d) received a DISCONNECT message from the MQTT server with an invalid reason code %d, sending DISCONNECT message\n",
									client->clientID, client->line, serverDisconnectRC);
				client->disconnectRC = MQTTRC_ProtocolError;
				performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
				return MQTTRC_ProtocolError;
			}

			const char *reason = g_ReasonStrings[serverDisconnectRC];
			MBTRACE(MBERROR, 5, "Client %s (line=%d) received a DISCONNECT message from the MQTT server with reason code %d (%s) and server reason string \"%s\"\n",
					            client->clientID, client->line, serverDisconnectRC, reason ? reason : "", mmsg->reasonMsgLen ? mmsg->reasonMsg : "");
		}
	} else {
		MBTRACE(MBERROR, 5, "Client %s (line=%d) is not an MQTT v5 client or later (version=%d), yet it received a DISCONNECT message from the server, this is a server error\n",
							client->clientID, client->line, client->mqttVersion);
	}

	return rc;
} /* doDisconnect */

/* *************************************************************************************
 * Process a PUBREL message received from the MQTT server
 *
 *   @param[in]  client              = the mqtt client receiving the PUBREL
 *   @param[in]  buf              	 = pointer to the current offset into the PUBREL message
 *   @param[out] txBuf				 = buffer to write the ACK message into, this buffer is
 *                                     submitted at the end of doData
 *   @param[in]  msgLen              = length of the DISCONNECT message (including Fixed Header)
 *   @param[in]  mmsg				 = object used to store data parsed from the received MQTT packet
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
static int doPubREL(mqttclient_t *client, char *buf, ism_byte_buffer_t *txBuf, int msgLen, mqttmessage_t *mmsg) {
	int rc = 0;
	volatile ioProcThread_t *iop = client->trans->ioProcThread;

	uint16_t msgId = (uint16_t)READINT16(buf);  /* Read the msgID from the message */
	rc = chkmsgID(client, msgId, "PUBREL");
	if(rc)
		return rc;

	/* Check for V5 Protocol for Properties. */
	if (client->mqttVersion >= MQTT_V5) {
		/* If msgLen = 2 then there is ReasonCode = 0 and there are no Properties. */
		if (msgLen > 2) {
			int numBytes = 0;
			int propsLen = 0;
			uint8_t reasonCode = (uint8_t)READCHAR(buf + 2);

			/* Add 3 to buf, and Subtract 3 from msgLen; 3 = (2byte msgID, 1 byte Reason code.*/
			buf += 3;
			msgLen -= 3;

			numBytes = decodeMQTTMessageLength(buf, (buf + msgLen), &propsLen);
			if (numBytes < 0) {
				MBTRACE(MBERROR, 1, "Decode MQTT Message failed with rc: %d", numBytes);
			}

			/* ------------------------------------------------------------------------
			 * If Property Length is > 0, then there are properties are present.
			 * Properties supported for SUBACK are: Reason String & User Property.
			 * ------------------------------------------------------------------------ */
			if ((propsLen < 0) || (propsLen > msgLen)) {
				rc = checkMqttPropFields(buf, propsLen, g_mqttCtx5, CPOI_PUBCOMP, mqttPropertyCheck, mmsg);
				if (rc) {
					MBTRACE(MBERROR, 5, "Client %s (line=%d) failed to validate the MQTT properties on a PUBREL message, initiating a disconnect\n", client->clientID, client->line);
					client->disconnectRC = rc;
					performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
					return rc;
				}
			}

			if (reasonCode) {  /* Check reasonCode and if true display the reason string. */
				iop->currBadRxRCCount++;
				client->badRxRCCount++;

				if (MQTT_RC_CHECK(reasonCode)) {
					MBTRACE(MBERROR, 1, "Client %s (line=%d) received a PUBREL message from the MQTT server with an invalid reason code %d, sending DISCONNECT message\n",
										client->clientID, client->line, reasonCode);
					client->disconnectRC = MQTTRC_ProtocolError;
					performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
					return MQTTRC_ProtocolError;
				}

				const char *reason = g_ReasonStrings[reasonCode];
				MBTRACE(MBERROR, 1, "Client %s (line=%d) received a PUBREL message from the MQTT server with reason code %d (%s) and server reason message \"%s\"\n",
	                            	client->clientID, client->line, reasonCode, reason ? reason : "", mmsg->reasonMsgLen ? mmsg->reasonMsg : "");
			}

			/* Add propsLen to buf, and Subtract propsLen from msgLen.*/
			buf += propsLen;
			msgLen -= propsLen;
		} else
			msgLen = 0;
	} /* if (client->mqttVersion >= MQTT_V5) */

	createMQTTMessageAck(txBuf, PUBCOMP, 0, msgId, client->useWebSockets);	/* Create the PUBCOMP message in response to the PUBREL */
	client->currTxMsgCounts[PUBCOMP]++;										/* NOTE - the send buffer is submitted at the end of doData */
	iop->currTxMsgCounts[PUBCOMP]++;										/*        so this might be a bit presumptuous */

	return rc;
} /* doPUBREL */

/* *************************************************************************************
 * Process a PUBCOMP message received from the MQTT server
 *
 *   @param[in]  client              = the mqtt client receiving the PUBCOMP
 *   @param[in]  buf              	 = pointer to the current offset into the PUBCOMP message
 *   @param[in]  msgLen              = length of the DISCONNECT message (including Fixed Header)
 *   @param[in]  mmsg				 = object used to store data parsed from the received MQTT packet
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
static int doPubCOMP(mqttclient_t *client, char *buf, int msgLen, mqttmessage_t *mmsg) {
	int rc = 0;
	volatile ioProcThread_t *iop = client->trans->ioProcThread;

	uint16_t msgId = (uint16_t)READINT16(buf);   /* Read the msgID from the message buffer */
	rc = chkmsgID(client, msgId, "PUBCOMP");
	if(rc)
		return rc;

	/* Check for V5 Protocol for Properties. */
	if (client->mqttVersion >= MQTT_V5) {
		/* If msgLen = 2 then there is ReasonCode = 0 and there are no Properties. */
		if (msgLen > 2) {
			int numBytes = 0;
			int propsLen = 0;
			uint8_t reasonCode = (uint8_t)READCHAR(buf + 2);

			/* Add 3 to buf, and Subtract 3 from msgLen; 3 = (2byte msgID, 1 byte Reason code.*/
			buf += 3;
			msgLen -= 3;

			numBytes = decodeMQTTMessageLength(buf, (buf + msgLen), &propsLen);
			if (numBytes < 0) {
				MBTRACE(MBERROR, 1, "Decode MQTT Message failed with rc: %d", numBytes);
			}

			/* ------------------------------------------------------------------------
			 * If Property Length is > 0, then there are properties are present.
			 * Properties supported for PUBCOMP are: Reason String & User Property.
			 * ------------------------------------------------------------------------ */
			if ((propsLen < 0) || (propsLen > msgLen)) {
				rc = checkMqttPropFields(buf, propsLen, g_mqttCtx5, CPOI_PUBCOMP, mqttPropertyCheck, mmsg);
				if (rc) {
					MBTRACE(MBERROR, 5, "Client %s (line=%d) failed to validate the MQTT properties on a PUBCOMP message, initiating a disconnect\n", client->clientID, client->line);
					client->disconnectRC = rc;
					performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
					return rc;
				}
			}

			if (reasonCode) {  /* Check reasonCode and if true display the reason string. */
				iop->currBadRxRCCount++;
				client->badRxRCCount++;

				if (MQTT_RC_CHECK(reasonCode)) {
					MBTRACE(MBERROR, 1, "Client %s (line=%d) received a PUBCOMP message from the MQTT server with an invalid reason code %d, sending DISCONNECT message\n",
										client->clientID, client->line, reasonCode);
					client->disconnectRC = MQTTRC_ProtocolError;
					performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
					return MQTTRC_ProtocolError;
				}

				const char *reason = g_ReasonStrings[reasonCode];
				MBTRACE(MBERROR, 1, "Client %s (line=%d) received a PUBCOMP message from the MQTT server with reason code %d (%s) and server reason message \"%s\"\n",
	                            	client->clientID, client->line, reasonCode, reason ? reason : "", mmsg->reasonMsgLen ? mmsg->reasonMsg : "");
			}

			/* Add propsLen to buf, and Subtract propsLen from msgLen.*/
			buf += propsLen;
			msgLen -= propsLen;
		} else
			msgLen = 0;
	} /* if (client->mqttVersion >= MQTT_V5) */

	if(client->inflightMsgIds) {						/* check the message ID state */
		pthread_spin_lock(&(client->mqttclient_lock));
		msgid_info_t *msgIdInfo = &client->inflightMsgIds[msgId];
		if(msgIdInfo->state == PUB_MSGID_PUBCOMP) {		/* check current msg ID state, should be PUB_MSGID_PUBCOMP*/
			msgIdInfo->state = PUB_MSGID_AVAIL;     	/* mark this message ID available for publish */
			msgIdInfo->destTuple->currPUBCOMPCount++;   /* increment topic PUBCOMP count */
			client->currInflightMessages--; 	   		/* decrement the number of messages in flight. */
		} else {
			MBTRACE(MBERROR, 1, "Expected msg ID state %d, actual msg ID state %d. msg ID=%u received message type=PUBCOMP\n",
								PUB_MSGID_PUBCOMP, msgIdInfo->state, msgId);
			rc=RC_MSGID_BAD;
		}
		pthread_spin_unlock(&(client->mqttclient_lock));
	} else {
		MBTRACE(MBERROR, 1, "Client %s (line=%d) received an unexpected PUBCOMP message, client max inflight message ID=%d\n",
							client->clientID, client->line, client->maxInflightMsgs);
		rc=RC_MSGID_NONE_AVAILABLE;
	}

	return rc;
} /* doPUBCOMP */

/* *************************************************************************************
 * Process a PUBREC message received from the MQTT server
 *
 *   @param[in]  client              = the mqtt client receiving the PUBREC
 *   @param[in]  buf              	 = pointer to the current offset into the PUBREC message
 *   @param[in]  msgLen              = length of the DISCONNECT message (including Fixed Header)
 *   @param[in]  mmsg				 = object used to store data parsed from the received MQTT packet
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
static int doPubREC(mqttclient_t *client, char *buf, int msgLen, mqttmessage_t *mmsg) {
	int rc = 0;
	volatile ioProcThread_t *iop = client->trans->ioProcThread;
	destTuple_t *pDest;
	double currTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());

	uint16_t msgId = (uint16_t)READINT16(buf);   /* Read the msgID from the message buffer */
	rc = chkmsgID(client, msgId, "PUBREC");
	if(rc)
		return rc;

	if(client->inflightMsgIds) {						/* check the message ID state */
		pthread_spin_lock(&(client->mqttclient_lock));
		msgid_info_t *msgIdInfo = &client->inflightMsgIds[msgId];
		pDest = msgIdInfo->destTuple;
		if(msgIdInfo->state == PUB_MSGID_PUBREC) {		/* check current msg ID state, should be PUB_MSGID_PUBREC*/
			msgIdInfo->state = PUB_MSGID_PUBCOMP;  		/* set next state for this msg ID to PUBCOMP */
			client->pubRecMsgId = msgId;				/* store the most recently received PUBREC message ID*/
			client->numPubRecs++;						/* increment client PUBREC count */
			pDest->currPUBRECCount++;    				/* increment topic PUBREC count */
			doCreatePubRel(client, pDest, msgId);		/* TODO :: Some day the code should be changed to reuse the buffer the message was sent in. */
		} else {
			MBTRACE(MBERROR, 1, "Expected msg ID state %d, actual msg ID state %d. msg ID=%u received message type=PUBREC\n",
								PUB_MSGID_PUBREC, msgIdInfo->state, msgId);
			return RC_MSGID_BAD;
		}
		pthread_spin_unlock(&(client->mqttclient_lock));
	} else {
		MBTRACE(MBERROR, 1, "Client %s (line=%d) received an unexpected PUBREC message, client max inflight message ID=%d\n",
							client->clientID, client->line, client->maxInflightMsgs);
		return RC_MSGID_NONE_AVAILABLE;
	}

	if((g_LatencyMask & CHKTIMESEND) > 0) { 			/* Check to see if user requested measuring PUB - PUBREC latency */
		if (msgId == (uint16_t) client->pubMsgId) {		/* We don't measure PUB-PUBREC latency on every PUBLISH, i.e. it is sampled */
			uint32_t latency = (uint32_t)((currTime - client->pubSubmitTime) * iop->sendHist->histUnits);

			iop->sendHist->totalSampleCt++;   /* Update Total # of Samples Received. */

			if (latency > iop->sendHist->max)   /* Update max latency if applicable. */
				iop->sendHist->max = latency;

			/* ------------------------------------------------------------------------
			 * Check whether the latency falls within the scope of the histogram.  If
			 * not, then increment the counter for larger than histogram size and update
			 * the 1 sec and 5 sec counters if applicable.
			 * ------------------------------------------------------------------------ */
			if (latency < iop->sendHist->histSize)
				iop->sendHist->histogram[latency]++;  /* Update histogram since within scope. */
			else {
				iop->sendHist->big++;   /* Update larger than histogram. */

				if (latency >= g_Equiv1Sec_RTT) {  /* Update if larger than 1 sec. */
					iop->sendHist->count1Sec++;
					if (latency >= (g_Equiv1Sec_RTT * 5))  /* Update if larger than 5 sec. */
						iop->sendHist->count5Sec++;
				}
			}

			client->pubMsgId = 0;  /* reset to 0, to indicate that the next PUB - PUBREC latency measurement can be taken for this client
			 	 	 	 	 	 	* pubMsgId is set during submit of the PUBLISH message */
		}
	}

	/* Check for V5 Protocol for Properties. */
	if (client->mqttVersion >= MQTT_V5) {
		/* If msgLen = 2 then there is ReasonCode = 0 and there are no Properties. */
		if (msgLen > 2) {
			int numBytes = 0;
			int propsLen = 0;
			uint8_t reasonCode = (uint8_t)READCHAR(buf + 2);

			/* Add 3 to buf, and Subtract 3 from msgLen; 3 = (2byte msgID, 1 byte Reason code.*/
			buf += 3;
			msgLen -= 3;

			numBytes = decodeMQTTMessageLength(buf, (buf + msgLen), &propsLen);
			if (numBytes < 0) {
				MBTRACE(MBERROR, 1, "Decode MQTT Message failed with rc: %d", numBytes);
			}

			/* ------------------------------------------------------------------------
			 * If Property Length is > 0, then there are properties are present.
			 * Properties supported for PUBREC are: Reason String & User Property.
			 * ------------------------------------------------------------------------ */
			if ((propsLen < 0) || (propsLen > msgLen)) {
				rc = checkMqttPropFields(buf, propsLen, g_mqttCtx5, CPOI_PUBREC, mqttPropertyCheck, mmsg);
				if (rc) {
					MBTRACE(MBERROR, 5, "Client %s (line=%d) failed to validate the MQTT properties on a PUBREC message, initiating a disconnect\n", client->clientID, client->line);
					client->disconnectRC = rc;
					performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
					return rc;
				}
			}

			if (reasonCode) {  /* Check reasonCode and if true display the reason string. */
				if(reasonCode == MQTTRC_NoSubscription) {
					if(pDest) {
						pDest->txNoSubCount++;
					}
				} else {
					if(pDest) {
						pDest->badRCCount++;
					}
					iop->currBadRxRCCount++;
					client->badRxRCCount++;

					if (MQTT_RC_CHECK(reasonCode)) {
						MBTRACE(MBERROR, 1, "Client %s (line=%d) received a PUBREC message from the MQTT server with an invalid reason code %d, sending DISCONNECT message\n",
											client->clientID, client->line, reasonCode);
						client->disconnectRC = MQTTRC_ProtocolError;
						performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
						return MQTTRC_ProtocolError;
					}

					const char *reason = g_ReasonStrings[reasonCode];
					MBTRACE(MBERROR, 1, "Client %s (line=%d) received a PUBREC message from the MQTT server with reason code %d (%s) and server reason message \"%s\"\n",
										client->clientID, client->line, reasonCode, reason ? reason : "", mmsg->reasonMsgLen ? mmsg->reasonMsg : "");
				}
			}

			/* Add propsLen to buf, and Subtract propsLen from msgLen.*/
			buf += propsLen;
			msgLen -= propsLen;
		} else {
			msgLen = 0;
		}
	} /* if (client->mqttVersion >= MQTT_V5) */

	return rc;
} /* doPUBREC */

/* *************************************************************************************
 * Process a PINGRESP message received from the MQTT server
 *
 *   @param[in]  client              = the mqtt client receiving the PINGRESP
 *   @param[in]  buf              	 = pointer to the current offset into the PINGRESP message
 *   @param[in]  msgLen              = length of the PINGRESP message (including Fixed Header)
 *   @param[in]  mmsg				 = object used to store data parsed from the received MQTT packet
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
static int doPingResp(mqttclient_t *client, char *buf, int msgLen, mqttmessage_t * mmsg) {
	int rc = 0;
	client->unackedPingReqs = 0;
	return rc;
}

/* *************************************************************************************
 * Process a PUBACK message received from the MQTT server
 *
 *   @param[in]  client              = the mqtt client receiving the PUBACK
 *   @param[in]  buf              	 = pointer to the current offset into the PUBACK message
 *   @param[in]  msgLen              = length of the PUBACK message (including Fixed Header)
 *   @param[in]  mmsg				 = object used to store data parsed from the received MQTT packet
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
static int doPubACK(mqttclient_t *client, char *buf, int msgLen, mqttmessage_t * mmsg) {
	int rc = 0;
	volatile ioProcThread_t *iop = client->trans->ioProcThread;
	destTuple_t *pDest;
	double currTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());

	uint16_t msgId = (uint16_t)READINT16(buf);   /* Read the msgID from the message buffer */
	rc = chkmsgID(client, msgId, "PUBACK");
	if(rc)
		return rc;

	if(client->inflightMsgIds) {						/* check the message ID state */
		pthread_spin_lock(&(client->mqttclient_lock));
		msgid_info_t *msgIdInfo = &client->inflightMsgIds[msgId];
		pDest = msgIdInfo->destTuple;
		if(msgIdInfo->state == PUB_MSGID_PUBACK) {		/* check current msg ID state, should be PUB_MSGID_PUBACK */
			msgIdInfo->state = PUB_MSGID_AVAIL;			/* mark this message ID available for publish */
			pDest->currPUBACKCount++;    				/* increment topic PUBACK count */
			client->currInflightMessages--; 			/* decrement the number of messages in flight. */
		} else {
			MBTRACE(MBERROR, 1, "Expected msg ID state %d, actual msg ID state %d. msg ID=%u received message type=PUBACK\n",
								PUB_MSGID_PUBACK, msgIdInfo->state, msgId);
			return RC_MSGID_BAD;
		}
		pthread_spin_unlock(&(client->mqttclient_lock));
	} else {
		MBTRACE(MBERROR, 1, "Client %s (line=%d) received an unexpected PUBACK message, client max inflight message ID=%d\n",
							client->clientID, client->line, client->maxInflightMsgs);
		return RC_MSGID_NONE_AVAILABLE;
	}

	if((g_LatencyMask & CHKTIMESEND) > 0) { 			/* Check to see if user requested measuring PUB - PUBACK latency */
		if (msgId == (uint16_t) client->pubMsgId) {		/* We don't measure PUB-PUBACK latency on every PUBLISH, i.e. it is sampled */
			uint32_t latency = (uint32_t)((currTime - client->pubSubmitTime) * iop->sendHist->histUnits);

			iop->sendHist->totalSampleCt++;   /* Update Total # of Samples Received. */

			if (latency > iop->sendHist->max)   /* Update max latency if applicable. */
				iop->sendHist->max = latency;

			/* ------------------------------------------------------------------------
			 * Check whether the latency falls within the scope of the histogram.  If
			 * not, then increment the counter for larger than histogram size and update
			 * the 1 sec and 5 sec counters if applicable.
			 * ------------------------------------------------------------------------ */
			if (latency < iop->sendHist->histSize)
				iop->sendHist->histogram[latency]++;  /* Update histogram since within scope. */
			else {
				iop->sendHist->big++;   /* Update larger than histogram. */

				if (latency >= g_Equiv1Sec_RTT) {  /* Update if larger than 1 sec. */
					iop->sendHist->count1Sec++;
					if (latency >= (g_Equiv1Sec_RTT * 5))  /* Update if larger than 5 sec. */
						iop->sendHist->count5Sec++;
				}
			}
			client->pubMsgId = 0;  /* reset to 0, to indicate that the next PUB - PUBACK latency measurement can be taken for this client
			 	 	 	 	 	 	* pubMsgId is set during submit of the PUBLISH message */
		}
	}

	/* Check for V5 Protocol for Properties. */
	if (client->mqttVersion >= MQTT_V5) {
		/* If msgLen = 2 then there is ReasonCode = 0 and there are no Properties. */
		if (msgLen > 2) {
			int numBytes = 0;
			int propsLen = 0;
			uint8_t reasonCode = (uint8_t)READCHAR(buf + 2);

			/* Add 3 to buf, and Subtract 3 from msgLen; 3 = (2byte msgID, 1 byte Reason code.*/
			buf += 3;
			msgLen -= 3;

			numBytes = decodeMQTTMessageLength(buf, (buf + msgLen), &propsLen);
			if (numBytes < 0) {
				MBTRACE(MBERROR, 1, "Decode MQTT Message failed with rc: %d", numBytes);
			}

			/* ------------------------------------------------------------------------
			 * If Property Length is > 0 then there are properties are present.
		 	 * Properties supported for PUBACK are: Reason String & User Property.
		 	 * ------------------------------------------------------------------------ */
			if ((propsLen < 0) || (propsLen > msgLen)) {
				rc = checkMqttPropFields(buf, propsLen, g_mqttCtx5, CPOI_PUBACK, mqttPropertyCheck, mmsg);
				if (rc) {
					MBTRACE(MBERROR, 5, "Client %s (line=%d) failed to validate the MQTT properties on a PUBACK message, initiating a disconnect\n", client->clientID, client->line);
					client->disconnectRC = rc;
					performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
					return rc;
				}
			}

			if (reasonCode) {  /* Check reasonCode and if true display the reason string. */
				if(reasonCode == MQTTRC_NoSubscription) {
					if(pDest) {
						pDest->txNoSubCount++;
					}
				} else {
					if(pDest) {
						pDest->badRCCount++;
					}
					iop->currBadRxRCCount++;
					client->badRxRCCount++;

					if (MQTT_RC_CHECK(reasonCode)) {
						MBTRACE(MBERROR, 1, "Client %s (line=%d) received a PUBACK message from the MQTT server with an invalid reason code %d, sending DISCONNECT message\n",
											client->clientID, client->line, reasonCode);
						client->disconnectRC = MQTTRC_ProtocolError;
						performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
						return MQTTRC_ProtocolError;
					}

					const char *reason = g_ReasonStrings[reasonCode];
					MBTRACE(MBERROR, 1, "Client %s (line=%d) received a PUBACK message from the MQTT server with reason code %d (%s) and server reason message \"%s\"\n",
										client->clientID, client->line, reasonCode, reason ? reason : "", mmsg->reasonMsgLen ? mmsg->reasonMsg : "");
				}
			}

			/* Add propsLen to buf, and Subtract propsLen from msgLen.*/
			buf += propsLen;
			msgLen -= propsLen;
		} else {
			msgLen = 0;
		}
	} /* if (client->mqttVersion >= MQTT_V5) */

	return rc;
} /* doPUBACK */

/* *************************************************************************************
 * doProcessMessage
 *
 * Description:  process the current MQTT message from the server
 *
 *   @param[in]  client              = the mqtt client that the message is being processed for
 *   @param[in]  ptr                 = current offset into the message buffer
 *   @param[in]  header              = Fixed Header of the MQTT Message received.
 *   @param[in]  msgLen              = remaining length of the MQTT message
 *   @param[in]  txBuf               = the TX buffer into which ACKs are written for subsequent submission (at the end of doData())
 * *************************************************************************************/
HOT static inline void doProcessMessage (mqttclient_t *client, char *ptr, Header *header, int msgLen, ism_byte_buffer_t *txBuf)
{
	xUNUSED int rc = 0;

	uint8_t msgType = (uint8_t)header->bits.type;
	volatile ioProcThread_t *iop = client->trans->ioProcThread;
	mqttmessage_t mmsg = {0};
	mmsg.client = client;

	if(msgType < MQTT_NUM_MSG_TYPES) {
		client->currRxMsgCounts[msgType]++;   /* per client message type count */
		iop->currRxMsgCounts[msgType]++;      /* per I/O processor message type count */
	} else {
		MBTRACE(MBERROR, 1, "Unknown MQTT message type: %d\n", msgType);
	}

	const char *msgTypeStr = ism_common_enumName(mb_MQTTMsgTypeEnum, msgType);
	if(!msgTypeStr){
		msgTypeStr = "UNKNOWN";
	}

	mmsg.msgType = (char *)msgTypeStr;   /* Put current message type in mmsg */

	if(client->traceEnabled || g_MBTraceLevel == 9) {
		char obuf[256] = {0};
		sprintf(obuf, "DEBUG: %10s client @ line=%d, ID=%s has received an MQTT %s packet",
					  msgTypeStr, client->line, client->clientID, msgTypeStr);
		traceDataFunction(obuf, 0, __FILE__, __LINE__, ptr, msgLen, 512);
	}

	/* Check that the MQTT server has complied with the client MaxPacketSize limit, specified in the CONNECT*/
	if(client->mqttVersion >= MQTT_V5 && msgLen > client->maxPacketSize) {
		MBTRACE(MBERROR, 1, "Client %s (line=%d) received a message from the MQTT server that exceeded the client's advertised limit (msg recvd=%d bytes, client limit=%d bytes)\n",
							client->clientID, client->line, msgLen, client->maxPacketSize);
		client->disconnectRC = MQTTRC_PacketTooLarge;
		performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
	}

	/* Process the received msg based on msg types */
	switch (msgType) {
		case PUBACK: 	/* Server to producer */
			rc = doPubACK(client, ptr, msgLen, &mmsg);
			break;
		case PUBREC: 	/* Server to producer */
			rc = doPubREC(client, ptr, msgLen, &mmsg);
			break;
		case PUBCOMP: 	/* Server to producer */
			rc = doPubCOMP(client, ptr, msgLen, &mmsg);
			break;
		case PUBLISH: 	/* Server to consumer */
			rc = onMessage(client, txBuf, ptr, msgLen, header, &mmsg);		/* process the received PUBLISH message */
			break;
		case PUBREL: 	/* Server to consumer */
			rc = doPubREL(client, ptr, txBuf, msgLen, &mmsg);
			break;
		case SUBACK: /* Server to consumer */
			rc = doSubACK(client, ptr, msgLen, &mmsg);
			break;
		case CONNACK: 	/* Server to consumer/producer */
			rc = doConnACK(client, ptr, msgLen, &mmsg);
			break;
		case DISCONNECT: /* Server to consumer/producer */
			rc = doDisconnect(client, ptr, msgLen, &mmsg);
			break;
		case UNSUBACK: /* Server to consumer */
			rc = doUnSubACK(client, ptr, msgLen, &mmsg);
			break;
		case PINGRESP: /* Server to client in response to PINGREQ */
			rc = doPingResp(client, ptr, msgLen, &mmsg);
			break;
		default:
			MBTRACE(MBWARN, 1, "Not handling this type of MQTT message: %d\n", msgType);
			break;
	} /* switch (msgType) */
} /* doProcessMessage */

/* *************************************************************************************
 * doData
 *
 * Description:  Called via epoll with the data read in from the socket.  Based on the
 *               type of message it will call the respective callback associated with it.
 *
 *
 *   @param[in]  client              = Specific mqttclient to use.
 *   @param[in]  rxBuf               = RX Buffer that contains the data read in.
 * *************************************************************************************/
HOT void doData (mqttclient_t *client, ism_byte_buffer_t *rxBuf)
{
	int rc = 0;
	int currMsgLen;
	int lenDiff;
	int msgLen;
	int numBytes;
	int numBytesNeeded = 0;
	int partialLen = 0;
	int totalMsgLen;
	int rxLen = rxBuf->used;
	int wsFrameSize = 0;

	char *ptr = NULL;
	char *pEndBuf;
	char *dataBuffer;

	Header header;

	uint64_t wsMsgLen = 0;
	WSFrame  wsframe;

	ism_byte_buffer_t replyMsgInfo;
	ism_byte_buffer_t *prevBuf = NULL;

	uint8_t isWebSocket = client->useWebSockets;

	/* --------------------------------------------------------------------------------
	 * Check to see if rxlen < 8 if so need to allocate more for tx buf since ACK's
	 * require 4 bytes and if WebSockets are being used they will require at minimum
	 * 4 bytes.
	 * -------------------------------------------------------------------------------- */
	if (rxLen < 8) {
		dataBuffer = alloca(128);
		replyMsgInfo.allocated = 128;
	} else {
		dataBuffer = alloca(rxLen);
		replyMsgInfo.allocated = rxLen;
	}

	pEndBuf = rxBuf->getPtr + rxLen;   /* Set the end of the buffer */

	/* Set up the reply message buffer for sending ACKs */
	replyMsgInfo.buf = dataBuffer;
	replyMsgInfo.putPtr = dataBuffer;
	replyMsgInfo.getPtr = dataBuffer;
	replyMsgInfo.used = 0;

	/* --------------------------------------------------------------------------------
	 * Check to see if user specified stop of the client's transport state is
	 * SOCK_DISCONNECTED or SOCK_ERROR. If true, then return the buffer and leave.
	 * -------------------------------------------------------------------------------- */
	if ((g_StopIOPProcessing) ||
		((client->trans->state & (SOCK_DISCONNECTED | SOCK_ERROR)) > 0)) {
		ism_common_returnBuffer(rxBuf,__FILE__,__LINE__);  /* Return the receive buffer back to the pool. */
		return;
	}

	/* Check to see if there is a partial message from the previous buffer. */
	if (client->partialMsg->used) {
		ptr = client->partialMsg->buf;

		/* Check if partial buffer contains a WebSocket message or not. */
		if (client->partialBufContainsWSFrame == 0) {
			numBytesNeeded = client->msgBytesNeeded;
		} else {
			int numAdditionalBytes = 0;

			if (client->partialMsg->used == 1) {
	            /* Copy 1 byte into the partial buffer */
	            memcpy(client->partialMsg->putPtr, (char *)rxBuf->getPtr, 1);
	            client->partialMsg->used = 2;
	            client->msgBytesNeeded -= 1;
	            rxBuf->getPtr += 1;
	            rxLen -= 1;
	            client->partialMsg->putPtr += 1;
	        }

	        /* read the frame (1st 2 bytes) and see if enough data was read. */
	        wsframe.all = READINT16(ptr);   /* Read the WebSocket Frame from the data. */

	        /* Determine whether there is enough data in the rxBuf to finalize the WS Frame. */
	        if (wsframe.bits.len < 126) {
	        	wsMsgLen = wsframe.bits.len;
				wsFrameSize = WS_FRAME_2BYTE;
	        } else if (wsframe.bits.len == WS_PAYLOAD_MASK_LESS_64K) {
	        	if (client->partialMsg->used >= WS_FRAME_4BYTE) {
	        		wsMsgLen = READINT16(ptr + 2);
	    			wsFrameSize = WS_FRAME_4BYTE;
	        	} else if ((client->partialMsg->used + rxLen) >= WS_FRAME_4BYTE) {
	        		numAdditionalBytes = WS_FRAME_4BYTE - client->partialMsg->used;
	        		memcpy(client->partialMsg->putPtr, (char *)rxBuf->getPtr, numAdditionalBytes);
	        		wsMsgLen = READINT16(ptr + 2);
	    			wsFrameSize = WS_FRAME_4BYTE;
	        	} else
	        		rc = -1;
	        } else {
	        	if (client->partialMsg->used >= WS_FRAME_10BYTE) {
	        		memcpy(&wsMsgLen, (ptr + WS_FRAME_SIZE), 8);
	        		wsMsgLen = ntohll(wsMsgLen);
	    			wsFrameSize = WS_FRAME_10BYTE;
	        	} else if ((client->partialMsg->used + rxLen) >= WS_FRAME_10BYTE) {
	        		numAdditionalBytes = WS_FRAME_10BYTE - client->partialMsg->used;
	        		memcpy(client->partialMsg->putPtr, (char *)rxBuf->getPtr, numAdditionalBytes);
	        		memcpy(&wsMsgLen, (ptr + WS_FRAME_SIZE), 8);
	        		wsMsgLen = ntohll(wsMsgLen);
	    			wsFrameSize = WS_FRAME_10BYTE;
	        	} else
	        		rc = -1;
	        } /* if (wsframe.bits.len < 126) */

	        /* ------------------------------------------------------------------------
	         * If there was a need for additional bytes (numAdditionalBytes in order to
	         * have a full WebSocket Frame then update the following variables:
	         *
	         * Actual buffer information:
	         *     rxLen, rxBuf->getPtr
	         *
	         * Client information pertaining to partial data:
	         *     client->partialMsg->putPtr, client->partialMsg->used
	         * ------------------------------------------------------------------------ */
			if (numAdditionalBytes > 0) {
				rxLen -= numAdditionalBytes;
				rxBuf->getPtr += numAdditionalBytes;
				client->partialMsg->putPtr += numAdditionalBytes;
				client->partialMsg->used += numAdditionalBytes;
			}

			ptr += wsFrameSize;
			numBytesNeeded = (wsMsgLen + wsFrameSize) - client->partialMsg->used;
		} /* if (client->partialBufContainsWSFrame == 0) */

		/* ----------------------------------------------------------------------------
	     * If the return code = 0 then either the partial WebSocket message is finished
	     * and the MQTT message is ready to be processed or the partial MQTT message is
	     * ready to be processed.
	     * ---------------------------------------------------------------------------- */
		if (rc == 0) {
			/* ------------------------------------------------------------------------
			 * Need to check if the # of bytes needed from previous buffer handled
			 * exists in the current buffer (rxBuf->used).
			 *
			 * If there are enough bytes then copy the amount of data needed from the
			 * client->msgBytesNeeded into the current partial msg buffer and start
			 * processing the data.
			 *
			 * If there aren't enough bytes then need to copy the amount of data available
			 * into the partial buffer, update the proper pointers and return.
			 * ------------------------------------------------------------------------ */
			if (numBytesNeeded <= rxLen) { /* enough data for required bytes. */
				/* Copy numBytesNeeded bytes into the partial buffer */
				memcpy(client->partialMsg->putPtr, (char *)rxBuf->getPtr, numBytesNeeded);

				client->partialMsg->putPtr += numBytesNeeded;
				client->partialMsg->used += numBytesNeeded;
				rxBuf->getPtr += numBytesNeeded;
				rxLen -= numBytesNeeded;

				currMsgLen = client->partialMsg->used;

				if (client->partialBufContainsWSFrame == 1) {
					currMsgLen -= wsFrameSize;
					client->partialBufContainsWSFrame = 0; /* Reset Partial Buffer contains WebSocket frame flag */
				}
			} else
				rc = -1;
		} /* if (rc == 0) */

		/* ----------------------------------------------------------------------------
		 * If the return code is -1 then only some of the data for the previous message
		 * is available. Copy the data that is available and update the info for the
		 * partial msg.
		 * ---------------------------------------------------------------------------- */
		if (rc == -1) {
			if (client->partialBufContainsWSFrame == 1)
				numBytesNeeded = (wsMsgLen + wsFrameSize) - client->partialMsg->used;

			/* ------------------------------------------------------------------------
			 * If the current msg size is greater than the allocated buffer than need
			 * to reallocate the buffer by 1024 (INC_BFR_BYTES_SIZE).  Then copy the
			 * data to the new buffer and finally free the buffer.
			 * ------------------------------------------------------------------------ */
			if ((numBytesNeeded + client->partialMsg->used) > client->partialMsg->allocated) {
				ism_byteBuffer tmpBuf = ism_allocateByteBuffer(numBytesNeeded + client->partialMsg->used + INC_BFR_BYTES_SIZE);
				memcpy(tmpBuf->buf, client->partialMsg->buf, client->partialMsg->used);
				tmpBuf->used = client->partialMsg->used;
				tmpBuf->putPtr = tmpBuf->buf + tmpBuf->used;
				ism_common_returnBuffer(client->partialMsg,__FILE__,__LINE__);
				client->partialMsg = tmpBuf;
				ptr = client->partialMsg->buf;
			} else {
				/* --------------------------------------------------------------------
				 * Only some of the data for the previous message is available.  Copy
				 * the data that is available and update the info for the partial msg.
				 * -------------------------------------------------------------------- */
				memcpy(client->partialMsg->putPtr, (char *)rxBuf->getPtr, rxLen);
				client->partialMsg->putPtr += rxLen;
				client->partialMsg->used += rxLen;
				client->msgBytesNeeded = client->msgBytesNeeded - rxLen;
			}

			ism_common_returnBuffer(rxBuf,__FILE__,__LINE__);  /* Return the receive buffer back to the pool. */
			return;
		} /* if (rc == -1) */

		/* ----------------------------------------------------------------------------
		 * Process the MQTT message by reading the header byte and then set the
		 * pointer at the byte after the header to decode the length of the message.
		 * ---------------------------------------------------------------------------- */
		header.byte = ptr[0];
		ptr++;         /* Set the ptr 1 byte past header to obtain the remaining length */

		/* ----------------------------------------------------------------------------
		 * Find out how many bytes are contained in the variable header portion
		 * of the message by calling decode.  Decode returns:
		 *
		 * BAD_MQTT_MSG     - Unable to determine the length of the remaining length
		 *                    field (didn't go past the end of buffer).
		 * PARTIAL_MQTT_MSG - The remaining length field is not completed due to
		 *                    the ptr exceeding the end of buffer
		 * 1 to 4           - Successful decode with the remaining length field
		 *                    being 1 to 4 bytes
		 * ---------------------------------------------------------------------------- */
		do {
			numBytes = decodeMQTTMessageLength(ptr, client->partialMsg->putPtr, &msgLen);

			/* Determine what decoding figured out about the message. */
			if (numBytes > 0) {
				currMsgLen = client->partialMsg->used;
				break;
			} else if (numBytes == PARTIAL_MQTT_MSG) { /* Partial remaining length field */
				/* --------------------------------------------------------------------
				 * If there are more bytes in the buffer than try copying 1 byte at a
				 * time to see if able to decipher the MQTT header and remaining length
				 * field otherwise still a partial message.
				 * -------------------------------------------------------------------- */
				if (rxLen > 0) {
					/* Copy 1 byte into the partial buffer and try again. */
					memcpy(client->partialMsg->putPtr, (char *)rxBuf->getPtr, 1);
					client->partialMsg->putPtr += 1;
					client->partialMsg->used += 1;
					rxBuf->getPtr += 1;
					rxLen -= 1;
				} else {
					client->msgBytesNeeded = 1;

					ism_common_returnBuffer(rxBuf,__FILE__,__LINE__);  /* Return the receive buffer back to the pool. */
					return;
				}
			} else {
				if (numBytes == BAD_MQTT_MSG) {   /* Invalid remaining length field */
					MBTRACE(MBERROR, 1, "Bad MQTT message!  Reconstructed message is bad,  exiting...\n");
				} else
					MBTRACE(MBERROR, 1, "Invalid state for numBytes: %d  (%s:%d).\n", numBytes, __FILE__, __LINE__);

				provideDecodeMessageData (client, ptr, pEndBuf, rxLen, &header);

				ism_common_returnBuffer(rxBuf,__FILE__,__LINE__);  /* Return the receive buffer back to the pool. */
				return;
			}
		} while (1);

		totalMsgLen = sizeof(char) + numBytes + msgLen;

		/* ----------------------------------------------------------------------------
		 * Check if the current partial msg buffer is large enough to hold the full
		 * message.  If not, then store the current partial msg buffer and allocate a
		 * new one.  After successful allocation then update the necessary pointers.
		 * ---------------------------------------------------------------------------- */
		if (totalMsgLen > client->partialMsg->allocated) {
			prevBuf = client->partialMsg;
			/* Reallocate buffer */
			client->partialMsg = ism_allocateByteBuffer(totalMsgLen + PARTIAL_BUF_PADDING);
			if (client->partialMsg == NULL) {
	        	rc = provideAllocErrorMsg("the Partial Message Buffer",
	        			                  (totalMsgLen + PARTIAL_BUF_PADDING),
										  __FILE__,
										  __LINE__);
				return;
			}

			client->partialMsg->putPtr += prevBuf->used;
			client->partialMsg->used = prevBuf->used;
			memcpy(client->partialMsg->buf, prevBuf->buf, prevBuf->used);
			ptr = client->partialMsg->buf;  /* Reset ptr since changed buffer. */
			ptr++; /* Increment ptr to be 1 byte past header. */

			/* Return the old/previous partial msg buffer to the pool. */
			ism_freeByteBuffer(prevBuf);
		} /* if (totalMsgLen > client->partialMsg->allocated) */

		/* ----------------------------------------------------------------------------
		 * Check if the total length of the message is greater than the current message
		 * length store in the partial msg buffer.
		 *
		 * If the total msg length > current stored length, then need to get the
		 * remaining data from the rxBuf.  Also, check whether the rxBuf contains all the
		 * data needed to complete the partial msg.  If not, then is just additional data.
		 * ---------------------------------------------------------------------------- */
		if (totalMsgLen > currMsgLen) {
			lenDiff = totalMsgLen - currMsgLen; /* Bytes need to read */

			if (lenDiff > rxLen) { /* Cannot read the whole msg, update the partial buffer */
				/* --------------------------------------------------------------------
				 * Only some of the data for the previous message is available.  Copy
				 * the data that is available and update the info for the partial msg.
				 * -------------------------------------------------------------------- */
				memcpy(client->partialMsg->putPtr, (char *)rxBuf->getPtr, rxLen);
				client->partialMsg->putPtr += rxLen;
				client->partialMsg->used += rxLen;
				client->msgBytesNeeded = lenDiff - rxLen;

				ism_common_returnBuffer(rxBuf,__FILE__,__LINE__);  /* Return the receive buffer back to the pool. */
				return;
			}

			memcpy(client->partialMsg->putPtr, (char *)rxBuf->getPtr, lenDiff);

			rxBuf->getPtr += lenDiff;
			/* Update rxLen since client->msgBytesNeeded was already read from it. */
			rxLen -= numBytesNeeded;

		} else if (totalMsgLen < currMsgLen) {
			/* Need to back up the rxBuf since too many bytes were read. */
			rxBuf->getPtr -= (currMsgLen - totalMsgLen);
		}

		ptr += numBytes;     /* Pass the remaining length field */

		doProcessMessage(client, ptr, &header, msgLen, &replyMsgInfo);

		/* In the case we got a whole message */
		client->partialMsg->used = 0;    /* Reset the partial length = 0 to indicate no partial. */

	} /* if (client->partialMsg->used) */

	/* --------------------------------------------------------------------------------
	 * Check to see if user specified stop of the client's transport state is
	 * SOCK_DISCONNECTED or SOCK_ERROR. If true, then return the buffer and leave.
	 * -------------------------------------------------------------------------------- */
	if ((g_StopIOPProcessing) ||
		((client->trans->state & (SOCK_DISCONNECTED | SOCK_ERROR)) > 0)) {
		ism_common_returnBuffer(rxBuf,__FILE__,__LINE__);  /* Return the receive buffer back to the pool. */
		return;
	}

	ptr = (char *)rxBuf->getPtr;  /* Set ptr to the rxBuf->getPtr */

	/* --------------------------------------------------------------------------------
	 * At this point either a partial message has been completed or there are no
	 * partial messages and need to process the rxBuf.
	 * -------------------------------------------------------------------------------- */
	while (ptr < pEndBuf) {
		/* ----------------------------------------------------------------------------
		 * If using WebSockets need to strip off the WebSocket Frame and test to see if
		 * the entire messages is contained in the bytes left.
		 *
		 * If unable to determine length of payload due to splitting of WebSocket Frame
		 * then set the special flag (specialWSPartial).
		 * ---------------------------------------------------------------------------- */
		if (isWebSocket) {
			if ((ptr + WS_FRAME_SIZE) <= pEndBuf) {
				wsMsgLen = 0;

				wsframe.all = READINT16(ptr);   /* Read the WebSocket Frame from the data. */

				/* Assert if either the bits mask is set or the final bit is not set */
				assert(wsframe.bits.mask == 0);
				assert(wsframe.bits.final == 1);

				if (wsframe.bits.len < 126) {
					wsMsgLen = wsframe.bits.len;
					if ((ptr + WS_FRAME_SIZE + wsMsgLen) <= pEndBuf) {
						ptr += WS_FRAME_SIZE;
					} else {
						partialLen = pEndBuf - ptr;
						numBytesNeeded = (wsMsgLen + WS_FRAME_2BYTE) - partialLen;
						client->partialBufContainsWSFrame = 1;
						break;
					}
				} else if (wsframe.bits.len == WS_PAYLOAD_MASK_LESS_64K) {
					if ((ptr + WS_FRAME_4BYTE) <= pEndBuf) {
						wsMsgLen = READINT16(ptr + WS_FRAME_SIZE);

						if ((ptr + WS_FRAME_4BYTE + wsMsgLen) <= pEndBuf) {
							ptr += WS_FRAME_4BYTE;
						} else {
							partialLen = pEndBuf - ptr;
							numBytesNeeded = (wsMsgLen + WS_FRAME_4BYTE) - partialLen;
							client->partialBufContainsWSFrame = 1;
							break;
						}
					} else {
						partialLen = pEndBuf - ptr;
						numBytesNeeded = WS_FRAME_4BYTE - partialLen;
						client->partialBufContainsWSFrame = 1;
						break;
					}
				} else {
					if ((ptr + WS_FRAME_10BYTE) <= pEndBuf) {
						memcpy(&wsMsgLen, (ptr + WS_FRAME_SIZE), 8);
						wsMsgLen = ntohll(wsMsgLen);

						if ((ptr + WS_FRAME_10BYTE + wsMsgLen) <= pEndBuf) {
							ptr += WS_FRAME_10BYTE;
						} else {
							partialLen = pEndBuf - ptr;
							numBytesNeeded = (wsMsgLen + WS_FRAME_10BYTE) - partialLen;
							client->partialBufContainsWSFrame = 1;
							break;
						}
					} else {
						partialLen = pEndBuf - ptr;
						numBytesNeeded = WS_FRAME_10BYTE - partialLen;
						client->partialBufContainsWSFrame = 1;
						break;
					}
				}
			} else { /* This is a partial WebSocket Frame (only 1 byte) */
				partialLen = 1;
				numBytesNeeded = 1;
				client->partialBufContainsWSFrame = 1; /* set flag that WebSocket Frame in partial buffer */
				break;
			}
		} /* if (isWebSocket) */

		header.byte = ptr[0];
		ptr++;        /* Set the ptr 1 byte past header to obtain length of payload */

		/* ----------------------------------------------------------------------------
		 * Check to see that there are more bytes still available since if there are
		 * none then this is a partial message.
		 * ---------------------------------------------------------------------------- */
		if (ptr < pEndBuf) {
			numBytes = decodeMQTTMessageLength(ptr, pEndBuf, &msgLen);

			/* Determine what decoding figured out about the message. */
			if (numBytes > 0) {
				if ((ptr + numBytes + msgLen) > pEndBuf) {
					ptr--;
					partialLen = pEndBuf - ptr;
					numBytesNeeded = (sizeof(char) + numBytes + msgLen) - partialLen;
					break;
				} else {
					if ((replyMsgInfo.used + 4) >= g_TxBfrSize)
						submitACK(client, &replyMsgInfo);

					ptr += numBytes;

					doProcessMessage(client, ptr, &header, msgLen, &replyMsgInfo);
				}
			} else if (numBytes == PARTIAL_MQTT_MSG) { /* Partial remaining length field */
				ptr--;
				partialLen = pEndBuf - ptr;
				numBytesNeeded = 1;
				break;
			} else {
				if (numBytes == BAD_MQTT_MSG) { /* Invalid remaining length field */
					MBTRACE(MBERROR, 1, "Bad MQTT message!  Error decoding...Abort processing!\n");
				} else {
					MBTRACE(MBERROR, 1, "Invalid state for numBytes: %d  (%s:%d).\n", numBytes, __FILE__, __LINE__);
				}

				provideDecodeMessageData (client, ptr, pEndBuf, rxLen, &header);

				assert(numBytes != BAD_MQTT_MSG);

				ism_common_returnBuffer(rxBuf,__FILE__,__LINE__);  /* Return the receive buffer back to the pool. */
				return;
			}

			/* Increment the pointer to the next message. */
			ptr += msgLen;

			/* ------------------------------------------------------------------------
			 * Check to see if user specified stop of the client's transport state is
			 * SOCK_DISCONNECTED or SOCK_ERROR. If true, then return the buffer and leave.
			 * ------------------------------------------------------------------------ */
			if ((g_StopIOPProcessing) &&
				((client->trans->state & (SOCK_DISCONNECTED | SOCK_ERROR)) > 0)) {
				ism_common_returnBuffer(rxBuf,__FILE__,__LINE__);  /* Return the receive buffer back to the pool. */
				return;
			}
		} else {
			/* There appears to be only 1 byte available so handle as a partial message. */
			ptr--;
			partialLen = pEndBuf - ptr;
			numBytesNeeded = 1;
			break;
		}
	}; /* while loop */

	/* --------------------------------------------------------------------------------
	 * Check to see if user specified stop of the client's transport state is
	 * SOCK_DISCONNECTED or SOCK_ERROR. If true, then return the buffer and leave.
	 * -------------------------------------------------------------------------------- */
	if ((g_StopIOPProcessing) &&
		((client->trans->state & (SOCK_DISCONNECTED | SOCK_ERROR)) > 0)) {
		ism_common_returnBuffer(rxBuf,__FILE__,__LINE__);  /* Return the receive buffer back to the pool. */
		return;
	}

	/* Copy any remaining data to the partial buffer for this client. */
	if (partialLen) {
		/* ----------------------------------------------------------------------------
		 * Need to check if the partial length + the # of bytes needed is greater than
		 * the current allocated buffer.  If so, then need to allocate a larger buffer
		 * and return the previously allocated buffer.
		 * ---------------------------------------------------------------------------- */
		if ((partialLen + numBytesNeeded) > client->partialMsg->allocated) {
			prevBuf = client->partialMsg;
			client->partialMsg = ism_allocateByteBuffer(partialLen + numBytesNeeded + PARTIAL_BUF_PADDING);
			if (client->partialMsg == NULL) {
				MBTRACE(MBERROR, 1, "Unable to get a buffer for the partialMsg buffer (%s:%d).\n", __FILE__, __LINE__);
				return;
			}

			/* Return old buffer */
			ism_freeByteBuffer(prevBuf);
		} else {
			client->partialMsg->putPtr = client->partialMsg->buf;
			client->partialMsg->getPtr = client->partialMsg->buf;
		}

		client->msgBytesNeeded = numBytesNeeded;
		memcpy(client->partialMsg->putPtr, ptr, partialLen);
		client->partialMsg->putPtr += partialLen;
		client->partialMsg->used = partialLen;
	} else {
		client->msgBytesNeeded = 0;
		client->partialMsg->putPtr = client->partialMsg->buf;
		client->partialMsg->getPtr = client->partialMsg->buf;
	}

	/* --------------------------------------------------------------------------------
	 * Check to see if there was any data written to the reply message data buffer for
	 * replies from the consumer.
	 * True  = Data exists and needs to be sent.
	 * False = Return the buffer back to the pool.
	 * -------------------------------------------------------------------------------- */
	if ((replyMsgInfo.used > 0) && (g_StopProcessing == 0)) {
		submitACK(client, &replyMsgInfo);
	}

	ism_common_returnBuffer(rxBuf,__FILE__,__LINE__);  /* Return the receive buffer back to the pool. */
} /* doData */

/* *************************************************************************************
 * submitACK
 *
 * Description:  This is used by doData to submit the reply buffer which contains
 *               ACK messages.   If there is no more room in the TX Buffer then
 *               doData will call this to send all these replies.
 *
 *   @param[in]  client              = Specific mqttclient to use.
 *   @param[in]  replyMsgInfo        = x
 * *************************************************************************************/
void submitACK (mqttclient_t *client, ism_byte_buffer_t *replyMsgInfo)
{
	int rc = 0;

	ism_byte_buffer_t *txBuf;
	volatile ioProcThread_t * iop = client->trans->ioProcThread;

	txBuf = iop->txBuffer;
	if (txBuf == NULL) {
		iop->txBuffer = getIOPTxBuffers(iop->sendBuffPool, iop->numBfrs, 1);
		txBuf = iop->txBuffer;
	}

	iop->txBuffer = txBuf->next;

	/* --------------------------------------------------------------------------------
	 * If the amount of data that needs to be transmitted is greater than the txBuf
	 * allocated then need to exit the program.  This should never occur consider the
	 * size of ACKs are only 4 bytes, which should be less than the message sent.
	 * -------------------------------------------------------------------------------- */
	assert(replyMsgInfo->used <= txBuf->allocated);

	memcpy(txBuf->buf, replyMsgInfo->buf, replyMsgInfo->used);
	txBuf->used = replyMsgInfo->used;
	txBuf->putPtr = txBuf->buf + txBuf->used;
	txBuf->getPtr = txBuf->buf;

	/* Reset the reply msg info */
	replyMsgInfo->used = 0;
	replyMsgInfo->getPtr = replyMsgInfo->buf;
	replyMsgInfo->putPtr = replyMsgInfo->buf;

	/* Submit job to I/O Thread */
	rc = submitIOJob(client->trans, txBuf);
	if (rc)
		MBTRACE(MBERROR, 1, "Unable to submit ACK message to I/O thread (rc: %d).\n", rc);
} /* submitACK */

/* *************************************************************************************
 * onMessage
 *
 * Description:  Asynchronous receipt for delivery of messages.
 *
 *   @param[in]  client              = Specific mqttclient to use.
 *   @param[in]  txBuf               = Buffer that contains data to be read.
 *   @param[in]  msg                 = x
 *   @param[in]  msgLen              = x
 *   @param[in]  header              = Header of the MQTT Message received.
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
HOT int onMessage (mqttclient_t *client,
		           ism_byte_buffer_t *txBuf,
				   char *msg,
				   int msgLen,
				   Header *header,
				   mqttmessage_t *mmsg)
{
	int rc = 0;
	int topicLen;
	uint8_t msgQoS = (uint8_t)header->bits.qos;
	uint8_t retain = (uint8_t)header->bits.retain;
	uint8_t dupFlag = (uint8_t)header->bits.dup;
	uint8_t sentACK = 0;
	uint16_t msgId = 0;
	uint32_t msgSize;

	char *ptr = msg;
	char *msgLenOff;
	char  topicName[MQTT_MAX_TOPIC_NAME_PAD128];

	if (g_StopIOPProcessing == 0) {
		volatile ioProcThread_t * iop = client->trans->ioProcThread;

		/* ----------------------------------------------------------------------------
		 * Need to get to the payload.  In order to do that the following must be
		 * performed:
		 *
		 * 1. Move the pointer past the topic length field + the topic name size.
		 * 2. If QoS 1 or 2 is set then need to get the message id from the message and
		 *    then increment the pointer by 2 more bytes.
		 * ---------------------------------------------------------------------------- */

		/* Read out the length of the topic name */
		topicLen = (int)((256 * (unsigned char)ptr[0]) + (unsigned char)ptr[1]);

		if (topicLen >= MQTT_MAX_TOPIC_NAME) {
			MBTRACE(MBERROR, 1, "Client %s (line=%d) received a PUBLISH message with a topic name length (%d), which is larger than max supported by mqttbench (%d)\n",
					            client->clientID, client->line, topicLen, (MQTT_MAX_TOPIC_NAME - 1));
			client->disconnectRC = MQTTRC_MalformedPacket;
			performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
			return MQTTRC_MalformedPacket;
		}

		ptr += 2;
		if(topicLen) {
			strncpy(topicName, ptr, topicLen);
			topicName[topicLen] = '\0';
		}
		ptr += topicLen;  /* Move past the topic name */

		/* If the QoS level is set then there will be a msg id. */
		if (msgQoS) {
			/* Ensure that msgQoS is either 0,1 or 2 */
			if ((msgQoS < 0) || (msgQoS > 2)) {
				MBTRACE(MBERROR, 1, "Client %s (line=%d) received a PUBLISH message with an invalid QoS (%d)",
									client->clientID, client->line, msgQoS);
				client->disconnectRC = MQTTRC_MalformedPacket;
				performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
				return MQTTRC_MalformedPacket;
			}

			msgId = (uint16_t)READINT16(ptr);
			ptr += MQTT_MSGID_SIZE;
		}

		/* ----------------------------------------------------------------------------
		 * Update msgLen subtracting start of message from the current pointer.  This
		 * is equivalent to:
		 *    - Subtract length of msgID, which is only present if QoS 1 or 2
		 *    - Subtract length of topic and length of topic len field
		 * ---------------------------------------------------------------------------- */
		msgLen -= (int)(ptr - msg);

		/* ----------------------------------------------------------------------------
		 * If using MQTT V5 then need to check for properties.
		 * Property length
		 *    = 0   :: Skip over obtaining the Properties information.
		 *    > 0   :: Obtain the properties which are copied into mmsg.
		 * ---------------------------------------------------------------------------- */
		if (client->mqttVersion >= MQTT_V5) {
			if (*ptr != 0) {  /* If > 0 then there are properties which need to be processed. */
				int propsLen = 0;
				int numBytes = 0;

				numBytes = decodeMQTTMessageLength (ptr, (ptr + msgLen), &propsLen);
				if (numBytes > 0) {
					ptr += numBytes;
					msgLen -= numBytes;
					if ((propsLen < 0) || (propsLen > msgLen)) {
						MBTRACE(MBERROR, 5, "Invalid MQTT property length was found in a PUBLISH message received by client %s (line=%d)\n",
											client->clientID, client->line);
						client->disconnectRC = MQTTRC_MalformedPacket;
						performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
						return MQTTRC_MalformedPacket;
					}

					rc = checkMqttPropFields((char *) ptr, propsLen, g_mqttCtx5, CPOI_S_PUBLISH, mqttPropertyCheck, mmsg);
					if (rc) {
						MBTRACE(MBERROR, 5, "Client %s (line=%d) failed to validate the MQTT properties on a PUBLISH message, initiating a disconnect\n", client->clientID, client->line);
						client->disconnectRC = rc;
						performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
						return rc;
					}
				} else {
					client->disconnectRC = MQTTRC_MalformedPacket;
					MBTRACE(MBERROR, 5, "Property Length > 0 but decoding of Variable Integer = 0\n");
					performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
					return MQTTRC_MalformedPacket;
				}

				ptr += propsLen;
				msgLen -= propsLen;

				/* Check the topic alias is valid, but we really use the SUB ID to keeping track of statistics */
				if(topicLen == 0) {
					if(mmsg->topic_alias) {
						if(mmsg->topic_alias > client->topicalias_count_in) {
							MBTRACE(MBERROR, 5, "Client %s (line=%d) received a PUBLISH message with a topic alias greater than the client specified limit(%d). This is a protocol error.\n",
												 client->clientID, client->line, client->topicalias_count_in);
							client->disconnectRC = MQTTRC_TopicAliasInvalid;
							performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
							return MQTTRC_TopicAliasInvalid;
						}
					} else {
						MBTRACE(MBERROR, 5, "Client %s (line=%d) received a PUBLISH message with a zero length topic name and no topic alias. This is a protocol error.\n",
								             client->clientID, client->line);
						client->disconnectRC = MQTTRC_ProtocolError;
						performMQTTDisconnect((void *) client->trans->ioProcThread, client->trans);
						return MQTTRC_ProtocolError;
					}
				}

				/* Message order checking (MQTT v5 clients only) */
				if ((g_ChkMsgMask & CHKMSGSEQNUM) > 0 && !retain && !dupFlag) {   /* don't check message order on retained or republished messages */
					uint64_t streamID = mmsg->streamID;
					uint32_t txInstanceID = streamID >> 32;
					if (streamID > 0) {  // a stream ID user property was found in the message, do sequence checking
						msgStreamObj * streamObj = ism_common_getHashMapElement(iop->streamIDHashMap, &streamID, sizeof(streamID));
						if (streamObj == NULL) { // we've not seen this stream before to allocate a new msgStreamObj for it
							streamObj = calloc(1, sizeof(msgStreamObj));
							if (UNLIKELY(streamObj == NULL)) {
					        	rc = provideAllocErrorMsg("a Stream Object", (int)sizeof(msgStreamObj), __FILE__, __LINE__);
								return rc;
							}
							streamObj->streamID = streamID;
							streamObj->txInstanceId = txInstanceID;
							streamObj->txTopicStr = strdup(topicName);  /* Copy topic name into the stream Obj., this only happens the first time the stream is seen by this consumer */
							streamObj->rxClient = client;
						}

						/* Get the last sequence number for QoS and then test for redelivery or out of order */
						uint64_t *lastSqn = 0;
						switch (msgQoS) {
						case 0:
							lastSqn = &streamObj->q0;
							break;
						case 1:
							lastSqn = &streamObj->q1;
							break;
						case 2:
							lastSqn = &streamObj->q2;
							break;
						default:
							break;
						}

						if (mmsg->seqnum != (*lastSqn + 1)) {
							if(*lastSqn >= mmsg->seqnum){
								streamObj->reDelivered++;
								iop->currRedeliveredCount++;
							}
							if(*lastSqn + 1 < mmsg->seqnum){
								streamObj->outOfOrder++;
								iop->currOOOCount++;
							}
						}

						*lastSqn = mmsg->seqnum;
						/* Update the streamObj in the hash map.*/
						ism_common_putHashMapElement(iop->streamIDHashMap, &streamID, sizeof(streamID), streamObj, NULL);
					}
				}

				/* Message payload length checking (MQTT v5 clients only) */
				if ((g_ChkMsgMask & CHKMSGLENGTH) > 0) {
					/* Check the message length is correct. */
					msgLenOff = ptr + PAYLOAD_MSGLEN_OFFSET;
					NgetInt(msgLenOff, msgSize);
					if (msgSize == msgLen) {
						/* if this is only a consumer then need to set the min/max message sizes received. */
						if (msgSize > g_MaxMsgRcvd)
							g_MaxMsgRcvd = msgSize;
						else if (msgSize < g_MinMsgRcvd)
							g_MinMsgRcvd = msgSize;
					} else	{
						MBTRACE(MBERROR, 1, "Message received is not the expect msg length: %d  actual: %d\n",
						                    msgSize,
						                    msgLen);
						rc = RC_BAD_MSGLEN;
					}
				}
			}
		} /* if (client->mqttVersion >= MQTT_V5) */

		iop = client->trans->ioProcThread;   /* Set the iop from the client structure */

		/* ----------------------------------------------------------------------------
		 * If calculating the Round Trip Time then need to get access the payload of
		 * the MQTT message in order to obtain the publish send time.
		 *
		 * If the receive time is before the last MQTT CONNACK time then need to store
		 * this latency into the Reconnect Histogram.
		 *
		 * NOTE: The msg that is delivered from doData should be at the field after the
		 *       remaining length.  Prior to MQTT V5 the timestamp was in the payload
		 *       but starting in MQTT v5 there is a separate User Property that contains
		 *       the timestamp.
		 * ---------------------------------------------------------------------------- */
		if ((g_LatencyMask & CHKTIMERTT) > 0 && !retain && !dupFlag) {  /* don't measure latency on retained or republished messages */
			int timeEquiv;
			int sampledMsg = 0;
			uint32_t latency;
			uli recvTimestamp = {0};
			uli sendTimestamp = {0};
			double timeUnits;
			latencystat_t * iopLatHist;

			/* ------------------------------------------------------------------------
			 * MQTT V5 & beyond
			 *    Read the Properties length field.  If Properties Length is greater
			 *    than 0, then need to get all the Properties.   Will need the check
			 *    to see if the User Property with the sendTimestamp is present.
			 *
			 * MQTT V3.1 & V3.1.1
			 *    Read the message and see if the 1st bytes is 1, which indicates a
			 *    sampled message.  Read the subsequent 8 bytes to obtain the timestamp.
			 * ------------------------------------------------------------------------ */
			if (client->mqttVersion >= MQTT_V5) {
				if (mmsg->senderTimestamp > 0) {
					sampledMsg = 1;
					sendTimestamp.d = mmsg->senderTimestamp;
				}
			} else if (ptr[0] == 1) {  /* Checking if 1st byte marked for sampling. */
				sampledMsg = 1;
				ptr += 1;   /* Move the pointer past the 1st byte flag */
				NgetLong(ptr, sendTimestamp);
			}

			/* ------------------------------------------------------------------------
			 * Check if this is a sampled message.  If so, then read the current time
			 * for the receive in order to determine the message latency and update
			 * the correct entry in the histogram.
			 * ------------------------------------------------------------------------ */
			if (sampledMsg == 1) {
				recvTimestamp.d = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());

				/* --------------------------------------------------------------------
				 * Check to see that the last MQTT CONNACK time was before the publish
				 * time of this message in order to determine which type of latency
				 * this message is for.
				 *
				 * Calculate the round trip latency by taking the current timestamp
				 * (recvTimestamp) and subtracting the timestamp stored in the message.
				 *
				 * Publish Time > last MQTT CONNACK:
				 *    - Multiply by the UNITS used for Round Trip Latency g_UnitsRTT
				 *
				 * Publish Time < last MQTT CONNACK:
				 *    - Multiply by the UNITS used for Reconnect Round Trip Latency
				 *      g_UnitsReconnectRTT.
				 * -------------------------------------------------------------------- */
				if (sendTimestamp.d > client->lastMqttConnAckTime) {
					timeUnits = g_UnitsRTT;
					iopLatHist = iop->rttHist;
					timeEquiv = g_Equiv1Sec_RTT;
				} else {
					timeUnits = g_UnitsReconnRTT;
					iopLatHist = iop->reconnectMsgHist;
					timeEquiv = g_Equiv1Sec_Reconn_RTT;
				}

				latency = (uint32_t)((recvTimestamp.d - sendTimestamp.d) * timeUnits);

				iopLatHist->totalSampleCt++;    /* Update Total # of Samples Received. */

				if (latency > iopLatHist->max)   /* Update max latency if applicable. */
					iopLatHist->max = latency;

				/* --------------------------------------------------------------------
				 * Check whether the latency falls within the scope of the histogram.
				 * If not, then increment the counter for larger than histogram size
				 * and update the 1 sec and 5 sec counters if applicable.
				 * -------------------------------------------------------------------- */
				if (latency < iopLatHist->histSize)
					iopLatHist->histogram[latency]++;  /* Update histogram since within scope. */
				else {
					iopLatHist->big++;   /* Update larger than histogram. */

					if (latency >= timeEquiv) {  /* Update if larger than 1 sec. */
						iopLatHist->count1Sec++;
						if (latency >= (timeEquiv * 5))  /* Update if larger than 5 sec. */
							iopLatHist->count5Sec++;
					}
				}
			}
		} /* if ((latencyMask & CHKTIMERTT) > 0) */

		/* ----------------------------------------------------------------------------
		 * Check to see if the -as option was specified.  If so, then perform a for
		 * loop which will just call sched_yield() for the specified number of loops.
		 * ---------------------------------------------------------------------------- */
		if (g_AppSimLoop) {
			int lpCtr;

			for ( lpCtr = 0 ; lpCtr <= g_AppSimLoop ; lpCtr++ ) {
				sched_yield(); //TODO need something compute intensive
			}
		}

		/* ----------------------------------------------------------------------------
		 * If the msg QoS > 0 then an ACK message (PUBACK or PUBREC) which is based on
		 * QoS 1 or 2 respectively.
		 * ---------------------------------------------------------------------------- */
		if (msgQoS) {
			/* Send either a PUBACK (QoS1) or PUBREC (QoS2) */
			createMQTTMessageAck(txBuf, (msgQoS == 1 ? PUBACK : PUBREC), 0, msgId, client->useWebSockets);
			client->currTxMsgCounts[(msgQoS == 1 ? PUBACK : PUBREC)]++;
			iop->currTxMsgCounts[(msgQoS == 1 ? PUBACK : PUBREC)]++;

			/* If this is a duplicate message then update counter. */
			if (dupFlag > 0) {
				iop->currRxDupCount++;
			}
			if (retain) {
				iop->currRxRetainCount++;
			}

			/* Set flag that sending PUBACK or PUBREC */
			sentACK = 1;
		}

		/* Update the IOP QoS Publish Message Count. */
		iop->currRxQoSPublishCounts[msgQoS]++;

		/* ----------------------------------------------------------------------------
		 * For MQTT v5 clients, we can now use the subscription ID to efficiently lookup
		 * the subscription that resulted in this client receiving this message. The
		 * MQTT server includes the subscription ID in the message
		 * ---------------------------------------------------------------------------- */
		destTuple_t *pDestTuple = NULL;
		if(client->mqttVersion >= MQTT_V5) {
			int subID = mmsg->subID;
			if (subID)
				pDestTuple = ism_common_getHashMapElement(client->destRxSubIDMap, &subID, sizeof(subID));
		} else if(!client->usesWildCardSubs) { // For MQTT v3.1.1 or earlier client only update per subscription stats if client uses no wildcard subscriptions
			pDestTuple = ism_common_getHashMapElement(client->destRxSubMap, topicName, 0);
		}

		if (pDestTuple) {
			switch(msgQoS){
			case 0:
				pDestTuple->currQoS0MsgCount++;
				break;
			case 1:
				pDestTuple->currQoS1MsgCount++;
				break;
			case 2:
				pDestTuple->currQoS2MsgCount++;
				break;
			}

			/* ------------------------------------------------------------------------
			 * If an ACK was sent then update the correct DestTuple counters for
			 * PUBACK or PUBREC based on QoS 1 or 2 respectively.
			 * ------------------------------------------------------------------------ */
			if (sentACK) {
				switch(msgQoS){
				case 1:
					pDestTuple->currPUBACKCount++;
					break;
				case 2:
					pDestTuple->currPUBRECCount++;
					break;
				}
			}

			if(retain) {
				pDestTuple->rxRetainCount++; /* increment number of retained messages received for this subscription */
			}

			if(dupFlag) {
				pDestTuple->rxDupCount++;
			}
		} /* if (pDestTuple) */
	} /* if (stopIOPProcessing == 0) */

	return rc;
} /*  */

/* *************************************************************************************
 * submitPing
 *
 * Description:  Send PINGREQ message to Server.
 *
 *   @param[in]  client              = Specific mqttclient to use.
 * *************************************************************************************/
void submitPing (mqttclient_t *client)
{
	int rc = 0;
	volatile ioProcThread_t *iop = client->trans->ioProcThread;
	
	/* Get a buffer from the TX buffer pool to send the PINGREQ message */
	ism_byte_buffer_t *txBuf = iop->txBuffer;
	if (txBuf == NULL) {
		iop->txBuffer = getIOPTxBuffers(iop->sendBuffPool, iop->numBfrs, 1);
		txBuf = iop->txBuffer;
	}
	iop->txBuffer = txBuf->next;

	/* Create the PING REQUEST message. */
	createMQTTMessagePingReq(txBuf, (int)client->useWebSockets);
	
	if(client->traceEnabled || g_MBTraceLevel == 9) {
		char obuf[256] = {0};
		sprintf(obuf, "DEBUG: %10s client @ line=%d, ID=%s submitting an MQTT PINGREQ packet",
						"PINGREQ:", client->line, client->clientID);
		traceDataFunction(obuf, 0, __FILE__, __LINE__, txBuf->buf, txBuf->used, 512);
	}

	/* Submit the job to the IO Thread and check return code. */
	rc = submitIOJob(client->trans, txBuf);
	if (rc == 0) {
		client->lastPingSubmitTime = g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime();
		if (client->unackedPingReqs++ == 0)
			client->pingWindowStartTime = client->lastPingSubmitTime;
		client->currTxMsgCounts[PINGREQ]++;   /* per client publish count */
		__sync_add_and_fetch(&(iop->currTxMsgCounts[PINGREQ]),1);
	} else {
		MBTRACE(MBERROR, 1, "Unable to submit PINGREQ message to I/O thread (rc: %d).\n", rc);
	}
} /* doPing */

/* *************************************************************************************
 * Consumer submits a SUBSCRIBE packet for each subscription it has
 *
 *   @param[in]  client              = Specific mqttclient to use.
 *
 *   @return 0 = OK, otherwise it is a failure
 * *************************************************************************************/
int submitSubscribe(mqttclient_t *client) {
	int rc = 0;
	mqttmessage_t msg;

	volatile ioProcThread_t *iop = client->trans->ioProcThread;

	/* Loop through all the subscriptions for this client, and submit a SUBSCRIBE message for each one */
	for(int i=0; i< client->destRxListCount; i++) {
		memset(&msg, 0, sizeof(mqttmessage_t));								/* clear out SUBSRIBE message */
		destTuple_t *pDestTuple = (destTuple_t *) client->destRxList[i];
		msg.qos = pDestTuple->topicQoS;   									/* The QoS for the topic. */

		/* ----------------------------------------------------------------------------
		 * Find an available message ID for this SUBSCRIBE message, we are in trouble if
		 * we can't find one, because we don't retry subscriptions
		 * ---------------------------------------------------------------------------- */
		pthread_spin_lock(&(client->mqttclient_lock));
		rc = findAvailableMsgId(client, SUB_MSGID_AVAIL, client->allocatedInflightMsgs);
		if(rc) {										/* did not find an available message id within the search limit */
			pthread_spin_unlock(&(client->mqttclient_lock));
			MBTRACE(MBERROR, 5, "Client %s at line %d has no available msg IDs for subscribing to topic %s at QoS %d (rc=%d)\n",
								client->clientID, client->line, pDestTuple->topicName, pDestTuple->topicQoS, rc);
			client->disconnectRC = MQTTRC_UnspecifiedError;
			return rc;
		} else {
			msgid_info_t *msgIdInfo = &client->inflightMsgIds[client->availMsgId];
			msgIdInfo->state = SUB_MSGID_SUBACK;		/* set the MsgID state to SUB_MSGID_SUBACK */
			msgIdInfo->destTuple = pDestTuple;			/* save a reference to the destTuple, used when the ACK is received */
			client->currInflightMessages++;         	/* increment inFlight # of msgs */
			msg.msgid = client->availMsgId; 			/* assign first available msg ID to this message */
		}
		pthread_spin_unlock(&(client->mqttclient_lock));

		/* Get a buffer from the TX buffer pool to send the SUBSCRIBE message */
		ism_byte_buffer_t *txBuf = iop->txBuffer;
		if (txBuf == NULL) {
			iop->txBuffer = getIOPTxBuffers(iop->sendBuffPool, iop->numBfrs, 1);
			txBuf = iop->txBuffer;
		}
		iop->txBuffer = txBuf->next;

		/* Generate SUBSCRIBE msg */
		createMQTTMessageSubscribe(txBuf, pDestTuple, &msg, (int)client->useWebSockets, client->mqttVersion);

		if(client->traceEnabled || g_MBTraceLevel == 9) {
			char obuf[256] = {0};
			sprintf(obuf, "DEBUG: %10s client @ line=%d, ID=%s submitting an MQTT SUBSCRIBE packet",
						  "SUBSCRIBE:", client->line, client->clientID);
			traceDataFunction(obuf, 0, __FILE__, __LINE__, txBuf->buf, txBuf->used, 512);
		}

		/* ----------------------------------------------------------------------------
		 * If performing Subscription latency then take t1 timestamp and store in the
		 * client. Note, we only record the submit time of the first subscription
		 * ---------------------------------------------------------------------------- */
		if ((i == 0) && (g_LatencyMask & (CHKTIMESUBSCRIBE | CHKTIMETCP2SUBSCRIBE)) > 0)
			client->subscribeConnReqSubmitTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());

		/* Submit job to I/O Thread */
		rc = submitIOJob(client->trans, txBuf);
		if (rc == 0) {
			client->currTxMsgCounts[SUBSCRIBE]++;
			iop->currTxMsgCounts[SUBSCRIBE]++;

			/* ------------------------------------------------------------------------
			 * The current message id was utilized for the subscribe message, now need
			 * to update the client->availMsgId to the next available one. Need to check
			 * to see if incrementing it by 1 will cause it to wrap.  If so reset = 1.
			 * ------------------------------------------------------------------------ */
			if (++client->availMsgId >= client->allocatedInflightMsgs)
				client->availMsgId = 1;
		} else {
			msgid_info_t *msgIdInfo = &client->inflightMsgIds[client->availMsgId];
			msgIdInfo->state = SUB_MSGID_AVAIL;      /* mark this message ID as available, since we failed to submit the SUBSCRIBE message */
			client->currInflightMessages--;          /* decrement inFlight # of msgs */
			MBTRACE(MBERROR, 1, "Unable to submit SUBSCRIBE message to I/O thread (rc: %d).\n", rc);
			client->disconnectRC = MQTTRC_UnspecifiedError;
			return rc;
		}

	} /* end loop through subscription list */

	return rc;
} /* submitSubscribe */

/* *************************************************************************************
 * doUnSubscribeAllTopics
 *
 * Description:  UnSubscribe to all the topic(s) for the specified client.
 *
 *   @param[in]  mbinst              = x
 *   @param[in]  numSubscribers      = x
 * *************************************************************************************/
void doUnSubscribeAllTopics (mqttbench_t *mbinst, int *numSubscribers)
{
	int rc = 0;
	int clientCtr = 0;
	mqttclient_t *client;
	ism_common_listIterator *pClientIter;
	ism_common_list_node *currClient;

	/* --------------------------------------------------------------------------------
	 * If the tuples is not NULL then start performing UnSubscribes for the clients
	 * associated with this instance of mqttbench_t that have performed a subscribe
	 * during the start up process.
	 * -------------------------------------------------------------------------------- */
	if (mbinst->clientTuples) {
		/* Allocate the iterator for walking the list of MQTT clients for this instance of mqttbench test. */
		pClientIter = (ism_common_listIterator *)calloc(1, sizeof(ism_common_listIterator));
		if (pClientIter) {
			ism_common_list_iter_init(pClientIter, mbinst->clientTuples);

			while ((currClient = ism_common_list_iter_next(pClientIter)) != NULL) {
				client = (mqttclient_t *)(currClient->data);

				if ((client->destRxListCount > 0) && (client->protocolState == MQTT_PUBSUB)) {
					/* Submit a job to UnSubscribe the current client. */
					rc = submitMQTTUnSubscribeJob(client->trans);
					if (rc < 0) {
						MBTRACE(MBERROR, 1, "Failed to submit UnSubscribe (rc: %d).\n", rc);
						*numSubscribers = clientCtr;
						return;
					} else
						clientCtr++;
				}
			} /* while loop - disconnect */

			/* Destroy the linked list used by the iterator */
			ism_common_list_iter_destroy(pClientIter);

			/* Free the memory used to create the iterator. */
			free(pClientIter);

			*numSubscribers = clientCtr;
		} else {
        	rc = provideAllocErrorMsg("an iterator", (int)sizeof(ism_common_listIterator), __FILE__, __LINE__);
			return;
		}
	}
} /* doUnSubscribeAllTopics */

/* *************************************************************************************
 * doUnSubscribe
 *
 * Description:  UnSubscribe from all topics that the connection Subscribed to.
 *
 *   @param[in]  client              = Specific mqttclient to use.
 * *************************************************************************************/
void submitUnSubscribe (mqttclient_t *client)
{
	int rc = 0;
	mqttmessage_t msg;

	volatile ioProcThread_t * iop;

	if (((client->trans->state & (SHUTDOWN_IN_PROCESS | SOCK_DISCONNECTED | SOCK_ERROR)) > 0) ||
		(client->unsubackCount > 0) ||
		(client->trans->ioProcThread == NULL)) {
		MBTRACE(MBERROR, 1, "Attempting to submit an UNSUBSCRIBE message, client %s is not in the process of shutting down, this is unexpected\n", client->clientID);
		return;
	}

	if (client->destRxListCount == 0) {
		MBTRACE(MBERROR, 1, "Attempting to submit an UNSUBSCRIBE message, but no subscriptions were found for client %s\n", client->clientID);
		return;
	}

	/* --------------------------------------------------------------------------------
	 * Since this client will no longer be submitting messages it is necessary to
	 * clear out all the message IDs and reset the pointer to 1.
	 * -------------------------------------------------------------------------------- */
	memset(client->inflightMsgIds, 0, client->maxInflightMsgs  * sizeof(msgid_info_t));
	client->availMsgId = 1;
	client->currInflightMessages = 0; /* Reset the number of current inflight messages */

	MBTRACE(MBINFO, 7, "Beginning to UnSubscribe all topics for Client %s.\n", client->clientID);

	/* Loop through all the subscriptions for this client, and submit an UNSUBSCRIBE message for each one */
	for(int i=0; i< client->destRxListCount; i++) {
		memset(&msg, 0, sizeof(mqttmessage_t));								/* clear out UNSUBSRIBE message */
		destTuple_t *pDestTuple = (destTuple_t *) client->destRxList[i];
		msg.qos = pDestTuple->topicQoS;   									/* The QoS for the topic. */

		/* ----------------------------------------------------------------------------
		 * Find an available message ID for this UNSUBSCRIBE message, we are in trouble if
		 * we can't find one, because we don't retry unsubscribing
		 * ---------------------------------------------------------------------------- */
		pthread_spin_lock(&(client->mqttclient_lock));
		rc = findAvailableMsgId(client, SUB_MSGID_AVAIL, client->allocatedInflightMsgs);
		if(rc) {										/* did not find an available message id within the search limit */
			pthread_spin_unlock(&(client->mqttclient_lock));
			MBTRACE(MBERROR, 5, "Client %s at line %d has no available msg IDs for subscribing to topic %s at QoS %d, we will not unsubscribe for this client.\n",
								client->clientID, client->line, pDestTuple->topicName, pDestTuple->topicQoS);
			return;
		} else {
			msgid_info_t *msgIdInfo = &client->inflightMsgIds[client->availMsgId];
			msgIdInfo->state = SUB_MSGID_UNSUBACK;		/* set the MsgID state to SUB_MSGID_UNSUBACK */
			msgIdInfo->destTuple = pDestTuple;			/* save a reference to the destTuple, used when the ACK is received */
			client->currInflightMessages++;         	/* increment inFlight # of msgs */
			msg.msgid = client->availMsgId; 			/* assign first available msg ID to this message */
		}
		pthread_spin_unlock(&(client->mqttclient_lock));

		iop = client->trans->ioProcThread;

		/* Get a buffer from the TX buffer pool to send the UNSUBSCRIBE message */
		ism_byte_buffer_t *txBuf = iop->txBuffer;
		if (txBuf == NULL) {
			iop->txBuffer = getIOPTxBuffers(iop->sendBuffPool, iop->numBfrs, 1);
			txBuf = iop->txBuffer;
		}
		iop->txBuffer = txBuf->next;

		/* Generate UNSUBSCRIBE msg */
		createMQTTMessageUnSubscribe(txBuf, pDestTuple, &msg, (int)client->useWebSockets, client->mqttVersion);

		if(client->traceEnabled || g_MBTraceLevel == 9) {
			char obuf[256] = {0};
			sprintf(obuf, "DEBUG: %10s client @ line=%d, ID=%s submitting an MQTT UNSUBSCRIBE packet",
						  "UNSUBSCRIBE:", client->line, client->clientID);
			traceDataFunction(obuf, 0, __FILE__, __LINE__, txBuf->buf, txBuf->used, 512);
		}

		/* Submit job to I/O Thread */
		rc = submitIOJob(client->trans, txBuf);
		if (rc == 0) {
			client->currTxMsgCounts[UNSUBSCRIBE]++;
			iop->currTxMsgCounts[UNSUBSCRIBE]++;

			/* ------------------------------------------------------------------------
			 * The current message id was utilized for the unsubscribe message, now need
			 * to update the client->availMsgId to the next available one. Need to check
			 * to see if incrementing it by 1 will cause it to wrap.  If so reset = 1.
			 * ------------------------------------------------------------------------ */
			if (++client->availMsgId >= client->allocatedInflightMsgs)
				client->availMsgId = 1;
		} else {
			msgid_info_t *msgIdInfo = &client->inflightMsgIds[client->availMsgId];
			msgIdInfo->state = SUB_MSGID_AVAIL;  	 /* mark this message ID as available, since we failed to submit the SUBSCRIBE message */
			client->currInflightMessages--;          /* decrement inFlight # of msgs */
			MBTRACE(MBERROR, 1, "Unable to submit UNSUBSCRIBE message to I/O thread (rc: %d).\n", rc);
		}

	} /* end loop through subscription list */
} /* doUnSubscribe */

/* *************************************************************************************
 * doDisconnectMQTTClient
 *
 * Description:  Disconnect the specific client for a particular thread.  This will
 *               initially set the protocolState = MQTT_DISCONNECT_IN_PROCESS, and if it
 *               successfully submits the MQTT Disconnect message it will set the
 *               protocolState = MQTT_DISCONNECTED.   It will also set the
 *               client->issuedDisconnect flag so that it doesn't try to submit the
 *               request more than once.
 *
 *   @param[in]  client              = Specific mqttclient to use.
 * *************************************************************************************/
void doDisconnectMQTTClient (mqttclient_t *client)
{
	int state = 0;

	ism_byte_buffer_t *txBuf = NULL;
	volatile ioProcThread_t *iop = NULL;

	if (client->trans) {
		int rc = 0;
		state = client->trans->state;
		iop = client->trans->ioProcThread;

		if ((state & SOCK_ERROR) || (state & SOCK_DISCONNECTED) || (state & SHUTDOWN_IN_PROCESS) ||
			(client->protocolState == MQTT_DISCONNECT_IN_PROCESS)) {
			return;
		} else {
			client->protocolState = MQTT_DISCONNECT_IN_PROCESS;
		}

		txBuf = iop->txBuffer;
		if (txBuf == NULL) {
			if ((g_StopIOPProcessing == 0) && (iop)) {
				iop->txBuffer = getIOPTxBuffers(iop->sendBuffPool, iop->numBfrs, 1);
				txBuf = iop->txBuffer;
			} else
				return;
		}

		iop->txBuffer = txBuf->next;

		rc = disconnectMQTTClient(client, txBuf);
		if (rc) {
			if (rc != RC_UNABLE_TO_SUBMITJOB)
				ism_common_returnBuffer(txBuf,__FILE__,__LINE__);   /* Return the transmit buffer back to the pool. */

			if (rc != RC_MQTTCLIENT_DISCONNECT_IP) {
				MBTRACE(MBERROR, 1, "%s didn't disconnect cleanly (rc: %d)\n",
				                    (client->clientType == CONSUMER ? "Consumer" : "Producer"),
				                    rc);
			}
		} else {
			client->issuedDisconnect = 1;
		}
	}
} /* doDisconnectMQTTClient */

/* *************************************************************************************
 * doDisconnectAllMQTTClients
 *
 * Description:  Disconnect the clients for a particular thread based on the client type
 *               requested.  This will send the MQTT Disconnect message and put the
 *               protocolState = MQTT_DISCONNECTED.
 *
 * Note:         Prior to 10/2017 this was only called with Producers, but now called
 *               for both types.
 *
 *   @param[in]  mbinst              = Thread's Credentials for this invocation.
 *   @param[in]  clientType          = Client type (Consumer, Producer or Dual Client)
 * *************************************************************************************/
void doDisconnectAllMQTTClients (mqttbench_t *mbinst, uint8_t clientType)
{
	int rc = 0;
	int numClients = ism_common_list_size(mbinst->clientTuples);
	int currNumDiscClients;
	int loopCtr = 0;

	mqttclient_t *client;

	ism_common_listIterator *pClientIter;
	ism_common_list_node *currClient;

	/* --------------------------------------------------------------------------------
	 * Allocate the iterator for walking the list of MQTT clients for this instance of
	 * mqttbench test.
	 * -------------------------------------------------------------------------------- */
	pClientIter = (ism_common_listIterator *)calloc(1, sizeof(ism_common_listIterator));
	if (pClientIter) {
		if (mbinst->clientTuples) {
			ism_common_list_iter_init(pClientIter, mbinst->clientTuples);

			while ((currClient = ism_common_list_iter_next(pClientIter)) != NULL) {
				client = (mqttclient_t *)(currClient->data);

				if ((client->trans) && ((client->trans->state & SOCK_ERROR) == 0)) {
					if (client->currInflightMessages == 0) {
						client->disconnectRC = MQTTRC_OK;
						rc = submitMQTTDisconnectJob(client->trans, NODEFERJOB);
						if (rc)
							MBTRACE(MBERROR, 1, "Failure submitting MQTT Disconnect job (rc: %d).\n", rc);

						client->issuedDisconnect = 1;
					}
				}
			} /* while () */

			/* Reset the iterator. */
			ism_common_list_iter_reset(pClientIter);

			/* ------------------------------------------------------------------------
		 	 * All client's transports that were NOT in SOCK_ERROR and there were no
		 	 * Messages In Flight have issued an MQTT Disconnect.   Now wait for
		 	 * a specific amount of time prior to forcing a Disconnect.
		 	 * ------------------------------------------------------------------------ */
			do {
				currNumDiscClients = 0;

				while ((currClient = ism_common_list_iter_next(pClientIter)) != NULL) {
					client = (mqttclient_t *)(currClient->data);

					/* Check to see if this client has issued disconnect message yet.*/
					if ((client->issuedDisconnect == 0) &&
						(client->trans) &&
						((client->trans->state & SOCK_ERROR) == 0)) {
						if (client->currInflightMessages == 0) {
							client->disconnectRC = MQTTRC_OK;
							rc = submitMQTTDisconnectJob(client->trans, NODEFERJOB);
							if (rc)
								MBTRACE(MBERROR, 1, "Failure submitting MQTT Disconnect job (rc: %d).\n", rc);

							client->issuedDisconnect = 1;
							currNumDiscClients++;
						} /* if (client->currInflightMessages == 0) */
					} else
						currNumDiscClients++;
				} /* while loop - disconnect */

				/* Reset the iterator based on client type. */
				ism_common_list_iter_reset(pClientIter);

				/* Check to see if all the clients have issued a disconnect. */
				if (currNumDiscClients < numClients)
					ism_common_sleep(SLEEP_10_MSEC);
				else
					break;
			} while (loopCtr++ <= DISCONNECT_ATTEMPTS);

			/* Free the memory used to create the iterator. */
			ism_common_list_iter_destroy(pClientIter);
		}

		free(pClientIter);
	} else
		rc = provideAllocErrorMsg("an iterator", (int)sizeof(ism_common_listIterator), __FILE__, __LINE__);

	return;
} /* doDisconnectAllMQTTClients */

/* *************************************************************************************
 * doCloseAllConnections
 *
 * Description:  Close all the connections for a particular thread
 *
 *   @param[in]  mbinst              = Thread's Credentials for this invocation.
 *   @param[in]  clientType          = Client type (Consumer, Producer or Dual Client)
 *
 *   @return 0   = Successful completion
 * *************************************************************************************/
int doCloseAllConnections (mqttbench_t *mbinst, int clientType)
{
	int rc = 0;

    ism_common_listIterator *pClientIter;
    ism_common_list_node *currNode;

    /* Create the iterator to walk each of the consumers and producers to get stats. */
    pClientIter = (ism_common_listIterator *)calloc(1, sizeof(ism_common_listIterator));
    if (pClientIter) {
		ism_common_list_iter_init(pClientIter, mbinst->clientTuples);

       	/* Close each client for this thread. */
        while ((currNode = ism_common_list_iter_next(pClientIter)) != NULL) {
    		rc = removeTransportFromIOThread(((mqttclient_t *)(currNode->data))->trans);
    		if (rc)
    			break;
        }  /* for loop (i) */

        ism_common_list_iter_destroy(pClientIter);
        free(pClientIter);
    } else   /* pClientIter == NULL */
    	rc = provideAllocErrorMsg("an iterator", (int)sizeof(ism_common_listIterator), __FILE__, __LINE__);

    return rc;
} /* doCloseAllConnections */

/* *************************************************************************************
 * Stop all the submitter threads.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 * *************************************************************************************/
void stopSubmitterThreads (mqttbenchInfo_t *mqttbenchInfo)
{
	int rc = 0;
	int i;

	if (mqttbenchInfo->mbInstArray) {
		for ( i = 0 ; i < mqttbenchInfo->mbCmdLineArgs->numSubmitterThreads ; i++ ) {
			if (mqttbenchInfo->mbInstArray[i]) {
				/* Join the submitter thread(s) */
				if (mqttbenchInfo->mbInstArray[i]->thrdHandle != 0) {
					ism_time_t timer = NANO_PER_SECOND * 2;  /* wait at most 2 seconds for the command thread to terminate */
					rc += ism_common_joinThreadTimed(mqttbenchInfo->mbInstArray[i]->thrdHandle, NULL, timer);
					if (rc)
						MBTRACE(MBERROR, 1, "Could not join submitter thread (idx=%d)\n", mqttbenchInfo->mbInstArray[i]->id);
				}
			}
		}
	}
} /* stopSubmitterThreads */

/* *************************************************************************************/
/* *************************************************************************************/
