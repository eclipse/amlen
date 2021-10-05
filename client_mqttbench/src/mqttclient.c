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

#include <sys/socket.h>
#include <pthread.h>
#include <ismutil.h>

#include "mqttclient.h"
#include "mqttbenchrate.h"

/* ******************************** GLOBAL VARIABLES ********************************** */
/* Externs */
extern uint8_t g_ClkSrc;
extern uint8_t g_RequestedQuit;
extern int g_LatencyMask;                /* declared in env.c */
extern int g_MBErrorRC;					 /* declared in env.c */
extern char **g_EnvSIPs;                 /* declared in env.c */
extern int g_MBTraceLevel;

extern mqttbenchInfo_t * g_MqttbenchInfo;

extern ismHashMap *g_dstIPMap;          /* declared in env.c */
extern ismHashMap *g_srcIPMap;               /* declared in env.c */

/* *************************************************************************************
 * A new (not found in the destination hashmap) server destination was provided,
 * perform the following functions:
 *
 *       1. DNS resolve the server destination to a list of one or more IPv4 addresses
 *       2. Add the dstIPList_t entry to the dstIPMap Hash Map
 *       3. For each destination IPv4 address in the dstIPList_t entry check if there
 *          is a srcIPList_t entry in the srcIPMap.
 *           a.  If not, then need to try and connect to the destination IP
 *               from each and every source IP.
 *           b.  If able to connect then add the srcIPList_t entry into the srcIPMap Hash Map
 *               using the destination IPv4 address as the key.
 *
 * @param[in]  dest			    = the server destination to be resolved and test connectivity to
 * @param[in]  destPort         = the server destination port to test connectivity to
 * @param[out] dstIPList		= the pointer used to return the allocated dstIPList_t object
 * @param[in]  line				= line number in the client list file where this client is found
 *
 *   @return 0   =
 * *************************************************************************************/
int processServerDestination(char *dest, int destPort, destIPList **dstIPList, int line) {
	int rc = 0;
	int numEnvSourceIPs = g_MqttbenchInfo->mbSysEnvSet->numEnvSourceIPs;

	/* --------------------------------------------------------------------------------
	 * Attempt to DNS resolve the server destination
	 * -------------------------------------------------------------------------------- */
	rc = resolveDNS2IPAddress(dest, dstIPList);
	if(rc){
		return rc;
	}

	/* Add the list of resolved numerical IP DNS records to the dstIPMap Hash Map */
	rc = ism_common_putHashMapElement(g_dstIPMap, dest, 0, *dstIPList, NULL);
	if (rc) {
		MBTRACE(MBERROR, 1, "Unable to add a destination IP list to the dstIPMap hash map.\n");
		return RC_UNABLE_TO_ADD_TO_HASHMAP;
	}

	/* Iterate through the dstIPList and for each resolved destination IP create a source IP list, if one
	 * does not already exist*/
	for(int i=0; i<((dstIPList_t *) *dstIPList)->numElements; i++){
		srcIPList_t *srcIPList = (srcIPList_t *) ism_common_getHashMapElement(g_srcIPMap, ((dstIPList_t *) *dstIPList)->dipArrayList[i], 0);
		if(srcIPList)
			continue;

		srcIPList = calloc(1, sizeof(srcIPList_t));
		if(srcIPList == NULL) {
			return provideAllocErrorMsg("srcIPList_t object", sizeof(srcIPList_t), __FILE__, __LINE__);
		}

		srcIPList->sipArrayList = (char **) calloc(numEnvSourceIPs, sizeof(char *));
		if (srcIPList->sipArrayList == NULL) {
			return provideAllocErrorMsg("srcIPList_t sipArrayList", sizeof(char *) * numEnvSourceIPs, __FILE__, __LINE__);
		}

		/* --------------------------------------------------------------------
		 * Create the SIP Map ArrayList by testing the connection from each src
		 * ip provided in the SIPList env variable to the destination IP and port
		 * -------------------------------------------------------------------- */
		rc = createSIPArrayList(((dstIPList_t *) *dstIPList)->dipArrayList[i], destPort, srcIPList, g_EnvSIPs, numEnvSourceIPs);
		if (rc) {
			return rc;
		}

		/* Add the srcIPMapEntry to the srcIPMap Hash Map regardless of any connections or not. */
		ism_common_putHashMapElement(g_srcIPMap, ((dstIPList_t *) *dstIPList)->dipArrayList[i], 0, srcIPList, NULL);
	}

	return rc;
} /* processServerDestination */

/* *************************************************************************************
 * setMQTTPreSharedKey
 *
 * Description:  Set the client PreShared Key fields required for using SSLCipher (e.g. PSK-AES256-CBC-SHA) .
 *
 *   @param[in]  client              = the client to be updated with a PSK ID/Key pair
 *   @param[in]  indx                = index into the pskArrayList of ID/Key pairs
 *   @param[in]  maxCt               = number of elements in the pskArrayList
 *   @param[in]  pskArrayList        = the list of PSK ID/Key pairs, read from the PSK file
 *
 *   @return 0   = Successful completion.
 *         <>0   = An error/failure occurred.
 * *************************************************************************************/
int setMQTTPreSharedKey (mqttclient_t *client, int indx, int maxCt, pskArray_t *pskArrayList)
{
	int rc = 0;
	int currLen = 0;

	char bitArray[MAX_BIT_ARRAY] = {'0'};

    /* Check to make sure the index isn't greater than the number of array elements. */
    if (indx < maxCt) {
    	/* ----------------------------------------------------------------------------
    	 * Get the current PSK ID length from array to be used for allocating memory
    	 * for the current PSK ID.
    	 * ---------------------------------------------------------------------------- */
    	currLen = strlen(pskArrayList->pskIDArray[indx]);

    	/* Allocate memory for the PSK ID. */
    	client->psk_id_len = currLen;
    	client->psk_id = (char *)calloc(1, (currLen + 1));
    	if (client->psk_id) {
    		strcpy(client->psk_id, pskArrayList->pskIDArray[indx]);

    		currLen = ism_common_fromHexString(pskArrayList->pskKeyArray[indx], bitArray);
    		if (currLen > 0) {
    	    	/* Allocate memory for the PSK Key. */
    	    	client->psk_key_len = currLen;
    	    	client->psk_key = (unsigned char *)calloc(1, currLen);
    	    	if (client->psk_key)
    	    		memcpy(client->psk_key, bitArray, currLen);
    	    	else
    	        	rc = provideAllocErrorMsg("for PreShared Key", currLen, __FILE__, __LINE__);
    		} else {
    			MBTRACE(MBERROR, 1, "the PreSharedKey is not a valid Hex String:  %s\n",
    			                    pskArrayList->pskKeyArray[indx]);
    			rc = RC_INVALID_PSK;
    		}
    	} else
        	rc = provideAllocErrorMsg("PreShared Key ID", (currLen + 1), __FILE__, __LINE__);
    } else {
    	MBTRACE(MBERROR, 1, "There aren't enough PreShared IDs and Keys in the PSK file (%d PSK IDs/Keys) required for this instance of mqttbench.\n", maxCt);
    	rc = RC_INSUFFICIENT_NUM_PSK_IDS;
    }

    return rc;
} /* setMQTTPreSharedKey */

/* *************************************************************************************
 * resetMQTTClient
 *
 * Description:  Depending on whether reconnecting or performing disconnect prior to
 *               reconnecting need to clear some registers and counters.
 *
 *   @param[in]  client              = Specific mqttclient to be use.
 * *************************************************************************************/
void resetMQTTClient (mqttclient_t *client)
{
	int rc = 0;
	char serverIPAddr[INET_ADDRSTRLEN] = {'0'};

	client->issuedDisconnect = 0;  /* Reset the issued Disconnect flag */
	client->subackCount = 0;       /* Reset the suback count */
	client->unsubackCount = 0;     /* Reset the unsuback count */
	client->allTopicsUnSubAck = 0; /* Reset the flag for all Topics received UNSUBACK. */

	/* --------------------------------------------------------------------------------
	 * Regardless of type of client just reset the partial message used indicator.
	 * The server would re-send the entire message.
	 * -------------------------------------------------------------------------------- */
	client->partialMsg->used = 0;  /* Reset the partial message used flag */
	client->msgBytesNeeded = 0;    /* Reset the number of bytes needed. */

	/* Reset the timestamp fields to recalculate latency if latency is being performed. */
	client->resetConnTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());  /* mark the reset time*/
	client->tcpConnReqSubmitTime = 0.0;        /* time CONNECT request message was submitted */
	client->mqttConnReqSubmitTime = 0.0;       /* time a MQTT Connect message was submitted. */
	client->mqttDisConnReqSubmitTime = 0.0;    /* time a MQTT DISCONNECT message was submitted. */
	client->subscribeConnReqSubmitTime = 0.0;  /* time a Subscribe message was submitted. */
	client->pubSubmitTime = 0.0;               /* time PUBLISH message was submitted, used to calculate Server PUBACK latency */
	client->nextPingReqTime = 0.0;             /* time the next PINGREQ message must be sent. */
	client->tcpLatency = 0;                    /* TCP Latency for TCP-MQTT or TCP-Subscribe Latency */
	client->mqttLatency = 0;                   /* MQTT Latency for TCP-Subscribe Latency */
	client->connRetries = 0;                   /* Reset the connection retry counter. */

	if(client->destTxAliasMapped) {
		memset(client->destTxAliasMapped, 0, client->topicalias_count_out * sizeof(uint8_t));  /* clear the topic alias mapping state table on reconnect */
	}

	if (client->inflightMsgIds) {
		/* ----------------------------------------------------------------------------
		 * Depending on the QoS and the type of client the message IDs will be
		 * clear or not.
		 *
		 *                                 Tasks Performed
		 *                     QoS     Clear    Reset    Reset
		 *   Client Type:              MsgIDs   Ptrs   # InFlight
		 *   -----------       ---     ------   -----  ----------
		 *   Consumer Only      0        Y        Y         Y
		 *   Consumer Only      1        Y        Y         Y
		 *   Consumer Only      2        Y        Y         Y
		 *
		 *   Dual Client        0        Y        Y         Y
		 *   Dual Client        1        Y        Y         Y
		 *   Dual Client        2        N        N         N
		 *
		 *   Producer Only      0        N        N         N
		 *   Producer Only      1        Y        Y         Y
		 *   Producer Only      2        N        N         N
		 * --------------------------------------------------------------------------- */
		if ((client->clientType == CONSUMER) ||
			((client->clientType == DUAL_CLIENT) && ((client->txQoSBitMap & MQTT_QOS_2_BITMASK) == 0)) ||
		    ((client->clientType == PRODUCER) && (client->txQoSBitMap == MQTT_QOS_1_BITMASK)))
		{
			memset(client->inflightMsgIds, 0, client->allocatedInflightMsgs * sizeof(msgid_info_t));
		} else {
			if ((client->txQoSBitMap & MQTT_QOS_2_BITMASK) != 0) {
				/* For QoS 2 publishers:
				 *
				 * Any MsgIDs in PUB_MSGID_PUBREC or PUB_MSGID_PUBCOMP states add the
				 * PUB_MSGID_UNKNOWN_STATE flag to indicate that the client should re-use the
				 * MsgID WITH the dup flag set */
				for (int i=0; i < client->allocatedInflightMsgs ; i++) {
					if (client->inflightMsgIds[i].state == PUB_MSGID_PUBREC ||
						client->inflightMsgIds[i].state == PUB_MSGID_PUBCOMP) {
						client->inflightMsgIds[i].state |= PUB_MSGID_UNKNOWN_STATE;
					}
				}
			}
		}

		client->availMsgId = 1;           /* Reset the pointer to the 1st msgID, which is 1 */
		client->currInflightMessages = 0; /* Reset the number of current inflight messages */
	}

	/* --------------------------------------------------------------------------------
	 * If the client's initRetryDelayTime_ns = 0 then set it to the default value.
	 * This eliminates the problem of a dead timer task.
	 * -------------------------------------------------------------------------------- */
	if (client->initRetryDelayTime_ns == 0) {
		client->initRetryDelayTime_ns = DEFAULT_RETRY_DELAY_USECS * NANO_PER_MICRO;
	}
	if (client->currRetryDelayTime_ns == 0) {
		client->currRetryDelayTime_ns = client->initRetryDelayTime_ns;
	}

	/* --------------------------------------------------------------------------------
	 * Update/ReResolve the DNS for a reconnect if --nodnscache specified.
	 * -------------------------------------------------------------------------------- */
	if (g_MqttbenchInfo->mbCmdLineArgs->reresolveDNS) {
		/* Resolve the DNS and if successful then update the transport Server IP. */
		rc = updateDNS2IPAddr(client->server, serverIPAddr);
		if (!rc) {
			free(client->trans->serverIP);
			client->trans->serverIP = strdup(serverIPAddr);
			client->trans->serverSockAddr.sin_addr.s_addr = inet_addr(serverIPAddr);
	    	MBTRACE(MBDEBUG, 9, "DNS resolved to %s\n", serverIPAddr);
		}
	}
} /* resetMQTTClient */

/* *************************************************************************************
 * disconnectMQTTClient
 *
 * Description:  Disconnect the MQTT Client via the MQTT DISCONNECT Message and then
 *               close the tcp socket.
 *
 *   @param[in]  client              = Specific mqttclient to be use.
 *   @param[in]  txBuf               = IOP TX buffer to be used for the MQTT DISCONNECT
 *                                     message.
 *
 *   @return 0   = Successful completion.
 *         <>0   = An error/failure occurred.
 * *************************************************************************************/
int disconnectMQTTClient (mqttclient_t *client, ism_byte_buffer_t *txBuf)
{
	int rc = RC_SUCCESSFUL;
	int transState = client->trans->state;

	if (client) {
		if ((transState & SOCK_ERROR) == 0) {
			if (((transState & SOCK_DISCONNECTED) == 0) &&
				((client->trans->socket > 0) && ((transState & SOCK_CONNECTED) > 0))) {

				/* --------------------------------------------------------------------
				 * Protocol State should have been set in the calling routine and set
				 * it to MQTT_DISCONNECT_IN_PROCESS.   If NOT, then need to determine
				 * the correct return code.
				 * -------------------------------------------------------------------- */
				if (client->protocolState == MQTT_DISCONNECT_IN_PROCESS) {
					if ((client->currInflightMessages == 0) || (g_RequestedQuit == 1)) {
						/* Close the connection since all in flight messages are done. */
						createMQTTMessageDisconnect(txBuf, client, (int) client->useWebSockets);

						if(client->traceEnabled || g_MBTraceLevel == 9) {
							char obuf[256] = {0};
							sprintf(obuf, "DEBUG: %10s client @ line=%d, ID=%s submitting an MQTT DISCONNECT packet",
										  "DISCONNECT:", client->line, client->clientID);
							traceDataFunction(obuf, 0, __FILE__, __LINE__, txBuf->buf, txBuf->used, 512);
						}

						/* Submit the job to the IO Thread. */
						rc = submitIOJob(client->trans, txBuf);
						if (rc == 0) {
							client->currTxMsgCounts[DISCONNECT]++;    /* per client disconnect count */
							client->trans->ioProcThread->currTxMsgCounts[DISCONNECT]++;   /* per I/O processor disconnect count */
							client->mqttDisConnReqSubmitTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());
						} else {
							MBTRACE(MBERROR, 1, "Unable to submit MQTT DISCONNECT to I/O Thread (rc: %d).\n", rc);
							rc = RC_UNABLE_TO_SUBMITJOB;
						}

						pthread_spin_lock(&(client->mqttclient_lock));       /* Lock the spinlock */

						client->protocolState = MQTT_DISCONNECTED;
						client->numPubRecs = 0;
						client->currInflightMessages = 0;

						/* Client is not going to reconnect, so we don't care about state, mqttbench does not persist state to disk. */
						if (client->trans->reconnect == 0) {
							client->availMsgId = 0;
							client->pubRecMsgId = 0;

							if (client->inflightMsgIds) {
								free(client->inflightMsgIds);
								client->inflightMsgIds = NULL;
							}
						}

						pthread_spin_unlock(&(client->mqttclient_lock));  /* Unlock the spinlock */
					} else
						rc = RC_MQTTCLIENT_DISCONNECT_IP;
				} else if (client->protocolState == MQTT_DISCONNECTED)
					rc = RC_MQTTCLIENT_DISCONNECTED;
				else {
					MBTRACE(MBERROR, 1, "Trying to disconnect but client has invalid protocol " \
							            "state: %x   Setting to MQTT_UNKNOWN.\n",
										client->protocolState);

					client->protocolState = MQTT_UNKNOWN;
				}
			} else
				rc = RC_TCP_SOCK_DISCONNECTED;
		} else
			rc = RC_TCP_SOCK_ERROR;
	} else
		rc = RC_MQTTCLIENT_FAILURE;

	return rc;
} /* disconnectMQTTClient( ) */

/* *************************************************************************************
 * startSubmitterThreads
 *
 * Description:  Start the Submitter threads which calls doHandleClients and starts
 *               connecting clients and publishing messages.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *   @param[in]  ioProcThreads       = Array of I/O Processor Thread structures.
 *
 *   @return 0   =
 * *************************************************************************************/
int startSubmitterThreads (mqttbenchInfo_t *mqttbenchInfo, ioProcThread_t **ioProcThreads, latencyUnits_t *pLatUnits)
{
	int rc = 0;
	int i, k;
	int b;
	int mask;
	int endType;
	int startType;
	int clientType;
	int numSubmitThrds = mqttbenchInfo->mbCmdLineArgs->numSubmitterThreads;

    char threadName[MAX_THREAD_NAME];

    environmentSet_t *pSysEnvSet = mqttbenchInfo->mbSysEnvSet;
    mqttbench_t **mbInstArray = mqttbenchInfo->mbInstArray;

    uint8_t instanceClientType = mqttbenchInfo->instanceClientType; /* bitmap of all client types for this instance of mqttbench (i.e. PROD, CONS, DUAL)*/

	/* Allocate per IOP thread histograms (rtt, send, and/or connections) */
	if ((g_LatencyMask & LAT_IOP_HISTOGRAMS) > 0) {
	   	/* Determine how many times it is necessary to create the histograms.*/
		if (instanceClientType == CONSUMERS_ONLY) {
			startType = CONSUMER;
			endType = startType + 1;
		} else if (instanceClientType == PRODUCERS_ONLY) {
			startType = PRODUCER;
			endType = startType + 1;
		} else {
			startType = CONSUMER;
			endType = DUAL_CLIENT;
		}

		for ( i = startType ; i < endType ; i++ ) {
			clientType = i;

			for ( k = 0 ; k < pSysEnvSet->numIOProcThreads ; k++ ) {
				rc = setupIOPThreadHistograms(ioProcThreads[k],
											  pSysEnvSet,
											  pLatUnits,
						                      clientType,
											  mqttbenchInfo->mbCmdLineArgs->latencyMask);
				if (rc != 0) {
					return rc;
				}
			}
		} /* for ( i = startType ; i < endType ; i++ ) */

		/* Free up the latency units structure. */
		if (pLatUnits) {
			free(pLatUnits);
			pLatUnits = NULL;
		}

		/* --------------------------------------------------------------------------------
		 * Test to see if the latency histograms were created for the bitmask provided.
		 * -------------------------------------------------------------------------------- */
		MBTRACE(MBINFO, 1, "Checking if IOP Histograms were created for the Latency Mask.\n");
		for ( b = 0, mask = 0x1 ; b < LAT_MASK_SIZE ; mask <<= 1, b++ ) {
			int bitSetInMask = mask & g_LatencyMask ;

			/* Check to ensure there are histograms for the specified latency. */
			rc = checkLatencyHistograms(bitSetInMask, pSysEnvSet->numIOProcThreads, instanceClientType);
			if (rc) {
				MBTRACE(MBERROR, 1, "There is an issue with the IOP Histograms (rc=%d), exiting...\n",rc);
				return rc;
			}
		}
	}

	/* Set up the prevTime for each IOP if either situation is true: */
	for ( i = 0 ; i < pSysEnvSet->numIOProcThreads ; i++ ) {
		ioProcThreads[i]->prevtime = getCurrTime();
	}

	/* Start the submitter threads and set thread affinity if specified by env variable */
	i = 0;   /* reset i */
	if ((g_MBErrorRC == 0) && (mbInstArray)) {
		for ( i = 0 ; i < numSubmitThrds ; i++ ) {
			if (mbInstArray[i]) {
				int   affMapLen;
				char  affMap[64];
				char  envAffName[64];
				char *envAffStr;

				snprintf(threadName, MAX_THREAD_NAME, "sub%d", i);
				snprintf(envAffName, 64, "Affinity_%s", threadName);
				envAffStr = getenv(envAffName);
				affMapLen = ism_common_parseThreadAffinity(envAffStr,affMap);

				/* Start the consumer thread(s). */
				rc = ism_common_startThread(&mbInstArray[i]->thrdHandle,
		                                    doHandleClients,
											mbInstArray[i],
		                                    NULL,     /* context */
		                                    0,        /* value   */
		                                    ISM_TUSAGE_NORMAL,
		                                    0,
		                                    threadName,
		                                    NULL);

				if ((g_MBErrorRC == 0) && (rc == 0)) {
					if (affMapLen)
						ism_common_setAffinity(mbInstArray[i]->thrdHandle,affMap,affMapLen);
				} else {
					if (g_MBErrorRC == 0)
						g_MBErrorRC = rc;

					break;
				}
			} /* if (mbInstArray[i]) */
		} /* for ( i = 0 ; i < numSubmitThrds ; i++ ) */
	} /* if ((g_MBErrorRC == 0) && (mbInstArray)) */

	/* --------------------------------------------------------------------------------
	 * If this thread is only consumers then check to see what type of latency testing
	 * is being performed.  If not performing TCP or MQTT Connection times then check
	 * to see if all the subscriptions have completed.
	 *
	 * This wait loop is really only needed when -d X, where X > 0
	 * -------------------------------------------------------------------------------- */
	if (instanceClientType == CONSUMERS_ONLY) {
		if (((g_LatencyMask & (LAT_TCP_COMBO | LAT_MQTT_COMBO)) == 0) && (g_MBErrorRC == 0)) {
			/* while loop, checking for all subscriptions complete */
			int numComplete;
			do {
				numComplete = 0;

				for ( i = 0 ; i < numSubmitThrds ; i++ ) {
					if (mbInstArray) {
						if ((mqttbench_t *)mbInstArray[i]) {
							if (mbInstArray[i]->numSubscribes == mbInstArray[i]->numSubsPerThread)
								numComplete++;
						}
					}
				}

				/* Check to see if there was a request to kill the process. */
				if ((g_RequestedQuit) || (g_MBErrorRC))
					break;
			} while (numComplete < numSubmitThrds);
		}
	}

	return rc;
} /* startSubmitterThreads */

/* *************************************************************************************
 *   Allocate the InFlight Message IDs for the particular client.
 *
 *   @param[in/out]  client              = the mqttbench client object for which the inflight message IDs are to be allocated
 *   @param[in]      mqttbenchInfo       = mqttbench user input object (command line params, env vars, etc.)
 *   @param[in]      tunedMsgIdCount     = calculated max message id count for QoS 1 and 2 publishers
 *
 * @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
int allocateInFlightMsgIDs(mqttclient_t * client, mqttbenchInfo_t * mqttbenchInfo, int tunedMsgIdCount) {
	int rc = 0;

	uint16_t numRxMsgIds = 0;
	uint16_t numTxMsgIds = 0;

	/* Calculate max number of message IDs required by this client */
	if(client->destRxListCount) {
		int rxMsgIds = client->destRxListCount + 1;                  /* +1 because msg ID 0 is not a valid msg id */
		numRxMsgIds = rxMsgIds > MAX_UINT16 ? MAX_UINT16 : rxMsgIds; /* if there is an integer overflow of UINT16, then assign to MAX_UINT16*/
	}

	if((client->txQoSBitMap & (MQTT_QOS_1_BITMASK | MQTT_QOS_2_BITMASK)) > 0) {
		int txMsgIds = tunedMsgIdCount;										/* default msg id count is based on the autotuned value */
		if(mqttbenchInfo->mbCmdLineArgs->maxInflightMsgs < MQTT_MAX_MSGID)  /* if user does not pass -mim, cmd line default is MQTT_MAX_MSGID */
			txMsgIds = mqttbenchInfo->mbCmdLineArgs->maxInflightMsgs + 1;  	/* if the user passed the -mim command line param, then use it in place of the autotuned value */
		numTxMsgIds = txMsgIds > MAX_UINT16 ? MAX_UINT16 : txMsgIds; 		/* if there is an integer overflow of UINT16, then assign to MAX_UINT16*/
	}

	client->maxInflightMsgs = client->allocatedInflightMsgs = MAX(numRxMsgIds, numTxMsgIds);   /* if a dual client (i.e. PRODUCER and CONSUMER), take the max */

	if (client->allocatedInflightMsgs > 0) {
		client->inflightMsgIds = calloc(client->allocatedInflightMsgs, sizeof(msgid_info_t));
		if(client->inflightMsgIds) {
			client->availMsgId = 1;
		} else {
			rc = provideAllocErrorMsg("the InFlight Message IDs", client->allocatedInflightMsgs * sizeof(msgid_info_t), __FILE__, __LINE__);
			return rc;
		}
	}

	return rc;
} /* allocateInFlightMsgIDs */

/* *************************************************************************************
 * read_psk_lists_from_file
 *
 * Description:  Read the id and PreShared key from the file.
 *
 *               Depending on the function type specified it will determine the work to
 *               be performed:
 *
 *               1. FIND_LONGEST_ENTRY - Obtain the longest entry and then create each
 *                                       array entry with the longest length
 *               2. LOAD_ARRAY         - Using the file provided copy each entry into
 *                                       the array that was initially created.
 *
 *   @param[in]  funcType            = x
 *   @param[in]  filename            = x
 *   @param[in]  pPSKArray           = x
 *
 *   @return 0   = Successful completion.
 *         <>0   = An error/failure occurred.
 * *************************************************************************************/
int read_psk_lists_from_file (int funcType, char * filename, pskArray_t * pPSKArray)
{
	int i, x;
	int rc = 0;
	int entrySize = 0;
	int numEntries = 0;
	int numLists = 2;
	int idCount = 0;
	int keyCount = 0;
	int longestID = 0;
	int longestKey = 0;

	char line[PSK_LINE_SIZE];
	char ** tempArray = NULL;

	FILE *fp;

	/* Check to see if a valid function type was specified and if not return -5 */
	if ((funcType == FIND_LONGEST_ENTRY) || (funcType == LOAD_ARRAY)) {
		/* ----------------------------------------------------------------------------
		 * If the requested function type = LOAD_ARRAY then calloc the memory for each
		 * pointer to contain the elements.  If the array is NULL then return with -2.
		 * ---------------------------------------------------------------------------- */
		if (funcType == LOAD_ARRAY) {
			if (((pPSKArray->id_Count > 0) && (pPSKArray->pskIDArray == NULL)) ||
				((pPSKArray->key_Count > 0) && (pPSKArray->pskKeyArray == NULL))) {
				return RC_INVALID_PSK;
			}

			for ( i = 0 ; i < numLists ; i++ ) {
				switch (i) {
					case 0:
						numEntries = pPSKArray->id_Count;
						tempArray = pPSKArray->pskIDArray;
						entrySize = pPSKArray->id_Longest + 1;
						break;
					case 1:
						numEntries = pPSKArray->key_Count;
						tempArray = pPSKArray->pskKeyArray;
						entrySize = pPSKArray->key_Longest + 1;
						break;
				} /* switch (i) */

				if (numEntries > 0) {
					for ( x = 0 ; x < numEntries ; x++ ) {
						tempArray[x] = calloc(1, entrySize);
						if (tempArray[x] == NULL) {
							rc = provideAllocErrorMsg("array elements", entrySize, __FILE__, __LINE__);
							return rc;
						}
					} /* for ( x = 0 ; x < numArrayElements ; x++ ) */
				}
			} /* for ( i = 0 ; i < numLists ; i++ ) */
		} /* if (funcType == LOAD_ARRAY) */

		/* Open the file provide */
		fp = fopen(filename, "r");

		/* ----------------------------------------------------------------------------
		 * Process the file provided by reading in the file and performing the function
		 * requested:  find longest entry or load array provided.
		 * ---------------------------------------------------------------------------- */
		if (fp) {
			int tmpLen = 0;

			numEntries = 0;     /* Reset the number of entries. */

			while ((fgets(line, PSK_LINE_SIZE, fp) != NULL)) {
				char *ip;
				char *startPos = NULL;

				do {
					/* Skip the line if either comment  '#' or blank line '\n' */
					if ((line[0] == '#') || (line[0]  == '\n')) break;

					line[PSK_LINE_SIZE-1] = 0 ;     /* Set the null terminator character
					                             * to the end of the line */
					/* ----------------------------------------------------------------
					 * Empty for loop body, move along the line until hitting an
					 * newline, ',' or the end of the string
					 * ---------------------------------------------------------------- */
					for ( ip = line ; *ip != 0 && *ip != ',' && *ip != '\n' ; ip++ ) ;

					/* If ip == line then is a blank line... go to next line. */
					if (ip == line) break;      /* Skip line if it is blank */

					tmpLen = (int)(ip - line);

					/* ----------------------------------------------------------------
					 * Depending on the function type the following is performed:
					 *
					 * 1. FIND_LONGEST_ENTRY - See if this is the longest entry and
					 *                         if so keep it as the newest longest.
					 * 2. LOAD_ARRAY - Copy current/1st element into the array.
					 * ---------------------------------------------------------------- */
					if (funcType == FIND_LONGEST_ENTRY) {
						if (tmpLen > longestID)
							longestID = tmpLen;
						idCount++;
					} else if (funcType == LOAD_ARRAY)
						strncat(pPSKArray->pskIDArray[numEntries], line, tmpLen);

					startPos = ip + 1;

					/* ----------------------------------------------------------------
					 * Empty for loop body, move along the line until hitting an
					 * newline or the end of the string
					 * ---------------------------------------------------------------- */
					for ( ip = startPos ; *ip != 0 && *ip != ',' && *ip != '\n' ; ip++ ) ;

					/* If ip == startPos then there is only a portion of the line. */
					if (ip != line) {
						/* Determine the length of the password element. */
						tmpLen = (int)(ip - startPos);

						if ((tmpLen % 2) != 0) {
							MBTRACE(MBERROR, 1, "PreShared Keys must be even number of Hex digits (entry #: %d is %d bytes).\n",
							                    (numEntries + 1),
							                    tmpLen);
							fclose(fp);  /* Close the file */
							return RC_INVALID_PSK;
						}

						/* ------------------------------------------------------------
						 * Depending on the function type the following is performed:
						 *
						 * 1. FIND_LONGEST_ENTRY - See if this is the longest entry_2
						 *                         and if so keep it as the newest
						 *                         longest_2.
						 * 2. LOAD_ARRAY - Copy 2nd element into the 2nd array.
						 * ------------------------------------------------------------ */
						if (funcType == FIND_LONGEST_ENTRY) {
							if (tmpLen > longestKey)
								longestKey = tmpLen;
							keyCount++;
						} else if (funcType == LOAD_ARRAY)
							strncat(pPSKArray->pskKeyArray[numEntries], startPos, tmpLen);
					} else {
						MBTRACE(MBERROR, 1, "Expecting other elements for this entry.\n");
						fclose(fp);  /* Close the file */
						return RC_INSUFFICIENT_INFO;
					}

					numEntries++;  /* Increment the number of entries */
				} while (0);
			} /* while ((fgets(line, LINE_SIZE, fp) != NULL)) */

			fclose(fp);  /* Close the file */

			/* ------------------------------------------------------------------------
			 * If determining the longest entry now time to set the registers provided
			 * by calling routine.
			 * ------------------------------------------------------------------------ */
			if (funcType == FIND_LONGEST_ENTRY) {
				pPSKArray->id_Count = idCount;
				pPSKArray->id_Longest = longestID;
				pPSKArray->key_Count = keyCount;
				pPSKArray->key_Longest = longestKey;
			}
		} else {
			MBTRACE(MBERROR, 1, "File specified:  %s  doesn't exist.\n", filename);
			rc = RC_FILE_NOT_FOUND;
		}
	} else {
		MBTRACE(MBERROR, 1, "Unsupported function type (%d) for reading file list.\n", funcType);
		rc = RC_UNSUPPORTED_FUNCTION;
	}

	return rc;
} /* read_psk_lists_from_file */
