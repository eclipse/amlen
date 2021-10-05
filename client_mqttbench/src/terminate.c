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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ismutil.h>

#include "mqttbench.h"
#include "mqttclient.h"

/* ******************************** GLOBAL VARIABLES ********************************** */
/* Externs */
extern uint8_t g_RequestedQuit;
extern uint8_t g_StopIOPProcessing;      /* Specifically notify the IOP Threads to stop processing. */
extern uint8_t g_StopProcessing;         /* Stop processing messages and/or jobs */

extern int16_t g_SubmitterExit;
extern int16_t g_TotalNumRetries_EXIT;   /* DEV ONLY - # of times to retry before exiting */

extern int g_MBErrorRC;
extern int *g_srcPorts;                  /* Source port counters for clients spread across multiple */
extern int *serverPorts;                 /* multi-port server testing */
extern char **g_EnvSIPs;                 /* List of Source IP Addresses specified with the SIPList environment variable. */
extern char *csvLogFile;
extern char *csvLatencyFile;

extern mqttbenchInfo_t *g_MqttbenchInfo;
extern mbMsgConfigInfo_t *g_MsgCfgInfo;

extern pskArray_t *pIDSharedKeyArray;
extern ioProcThread **ioProcThreads;

/* External Hash Maps */
extern ismHashMap *qtMap;
extern ismHashMap *g_dstIPMap;
extern ismHashMap *g_srcIPMap;
extern ismHashMap *g_ccertsMap;
extern ismHashMap *g_cpkeysMap;

/* External Spinlocks */
extern pthread_spinlock_t mqttlatency_lock;
extern pthread_spinlock_t submitter_lock;
extern pthread_spinlock_t ready_lock;
extern pthread_spinlock_t connTime_lock;
extern pthread_spinlock_t mbinst_lock;

/* Extern File Descriptors */
extern FILE *g_fpCSVLogFile;
extern FILE *g_fpCSVLatencyFile;
extern FILE *g_fpDebugFile;
extern FILE *g_fpHgramFiles[MAX_NUM_HISTOGRAMS];


/* *************************************************************************************
 * doStartTermination
 *
 * Description:  Common routine to start the termination process.
 *
 *   @param[in]  error               = If != 0, then performs minimal steps.
 *                                     If 0, then will log statistics, and log latency if
 *                                     specified.
 * *************************************************************************************/
void doStartTermination (int error)
{
	int i;
	int numThrds = 0;
	int ctr = g_MqttbenchInfo->histogramCtr;
	int mask;
	int maxSize;
	int latencyMask = g_MqttbenchInfo->mbCmdLineArgs->latencyMask;

	/* Try to Quiesce the System at this point. */
	doQuiesceSystem();

	/* If there was an error then don't obtain latencies. */
	if (error == 0) {
		/* ----------------------------------------------------------------------------
		 * If snapshots were NOT enabled then obtain the latencies here otherwise the
		 * snapshots will have written the data to the file already.
		 * ----------------------------------------------------------------------------
		 * Get latency for each type of latency (-T <mask>) by going through each
		 * consumers histogram
		 * ---------------------------------------------------------------------------- */
		for ( i = 0, mask = 0x1 ; i < LAT_MASK_SIZE ; mask <<= 1, i++ ) {
			if ((mask & latencyMask) > 0) {
				/* --------------------------------------------------------------------
				 * If snapshots are NOT enabled, then obtain the latencies now.
				 * Otherwise the snapshots will have already written the data to file.
				 * -------------------------------------------------------------------- */
				if (g_MqttbenchInfo->mbCmdLineArgs->snapInterval == 0) {
					if ((mask & LAT_CONN_AND_SUBSCRIBE) == 0)
						calculateLatency(g_MqttbenchInfo, mask, OUTPUT_CSVFILE, 0, g_fpCSVLatencyFile, g_fpHgramFiles[ctr++]);
				} else {
					if ((mask & latencyMask) == CHKTIMERTT)
						maxSize = g_MqttbenchInfo->mbSysEnvSet->maxMsgHistogramSize;
					else
						maxSize = g_MqttbenchInfo->mbSysEnvSet->maxConnHistogramSize;

					printHistogram(g_MqttbenchInfo, mask, maxSize, g_fpHgramFiles[ctr++]);
				}
			}
		}
	}

	/* --------------------------------------------------------------------------------
	 * If there are submitter threads then need to stop the clients running on them.
	 * Stop the client threads before stopping the IOPs.
	 * -------------------------------------------------------------------------------- */
	numThrds = determineNumSubmitThreadsStarted();
	if (numThrds > 0)
		stopSubmitterThreads(g_MqttbenchInfo);

	/* Clean up the histograms created on the IOP threads. */
	removeIOPThreadHistograms(g_MqttbenchInfo->mbSysEnvSet, ioProcThreads);

	/* Stop I/O Listener and I/O Processor threads */
	stopIOThreads(g_MqttbenchInfo);

    /* Perform all the necessary cleanup of variables that were allocated on the heap. */
	if (error == 0)
		MBTRACE(MBINFO, 1, "Cleaning up....\n");

	doCleanUp();
} /* doStartTermination */

/* *************************************************************************************
 * doQuiesceSystem
 *
 * Description:  Attempt to do a graceful shutdown.
 * *************************************************************************************/
void doQuiesceSystem (void)
{
	int numThrds = 0;

	/* --------------------------------------------------------------------------------
	 * First stop all the producers running on the Submitter Threads by setting the
	 * the mb_StopProcessing flag.
	 * -------------------------------------------------------------------------------- */
	g_StopProcessing = 1;
	g_RequestedQuit = 1;

	/* --------------------------------------------------------------------------------
	 * Determine how many submitter threads have been started.
	 *
	 *   - If no submitter threads
	 *   - If there are submitter threads started then need to give them a chance to
	 *     close cleanly.  Will wait for up to 1 minute and if not able to close
	 *     cleanly then will force an exit.
	 * -------------------------------------------------------------------------------- */
	numThrds = determineNumSubmitThreadsStarted();
	if ((numThrds > 0) && (g_MqttbenchInfo->mbInstArray)) {
		int16_t waitCtr = 0;

		if (g_MBErrorRC == 0) {
			while (g_SubmitterExit < numThrds) { /*BEAM suppression: infinite loop*/
				ism_common_sleep(2 * SLEEP_1_SEC);

				if (waitCtr++ >= g_TotalNumRetries_EXIT)
					break;
			}
		}

		MBTRACE(MBINFO, 1, "Submitter thread(s) have exited: %d/%d (actual/expected).\n",
			               g_SubmitterExit,
						   numThrds);
	}

	g_StopIOPProcessing = 1;
} /* doQuiesceSystem */

/* *************************************************************************************
 * determineNumSubmitThreadsStarted
 *
 * Description:  Determine the number of submitter threads which have been started.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *
 *   @return 0   = Successful completion
 *          >0   = Number of submitter threads started.
 * *************************************************************************************/
int determineNumSubmitThreadsStarted (void)
{
	int thrdCtr = 0;     /* thread counter */
	int i;

	mqttbench_t **mbInstArray = g_MqttbenchInfo->mbInstArray;

	for ( i = 0 ; i < g_MqttbenchInfo->mbCmdLineArgs->numSubmitterThreads ; i++ ) {
		if ((mbInstArray[i])->thrdHandle)
			thrdCtr++;
	}

	return thrdCtr;
} /* determineNumSubmitThreadsStarted */

/* *************************************************************************************
 * doCleanUp
 *
 * Description:  Perform the necessary cleanup on variables created on the heap.
 * *************************************************************************************/
void doCleanUp (void)
{
	int i, j, k;
	int rc;
	int numElems;
	int numSubmitThrds = g_MqttbenchInfo->mbCmdLineArgs->numSubmitterThreads;
	int latencyMask = g_MqttbenchInfo->mbCmdLineArgs->latencyMask;

	char *cSSLClientMethod = NULL;
	char *cSSLCipher = NULL;

    ism_common_listIterator *pClientIter;
    ism_common_list_node *currClient;

    mqttbench_t **mbInstArray = g_MqttbenchInfo->mbInstArray;

    messagePayload_t  *currMsgPayload = NULL;
    mbMsgConfigInfo_t *pMsgCfgInfo  = g_MqttbenchInfo->mbMsgCfgInfo;
    mbJitterInfo_t    *pJitterInfo  = g_MqttbenchInfo->mbJitterInfo;
    mbCmdLineArgs_t   *pCmdLineArgs = g_MqttbenchInfo->mbCmdLineArgs;
    environmentSet_t  *pSysEnvSet   = g_MqttbenchInfo->mbSysEnvSet;

    /* Get the 2 SSL Environment variables if set in order to free up the memory later on. */
    cSSLClientMethod = getenv("SSLClientMeth");     /* Possible values: SSLv23, SSLv3, TLSv1, TLSv11, TLSv12 (default) */
    cSSLCipher       = getenv("SSLCipher");			/* Possible values: RSA (default) */

	/* If user requested quit then try to quiesce system before killing everything. */
	if (g_RequestedQuit)
		doQuiesceSystem();

    /* Close and remove the log and latency filenames variables if allocated. */
	if (g_fpCSVLogFile) {
		fclose (g_fpCSVLogFile);
		g_fpCSVLogFile = NULL;
	}
	if (g_fpCSVLatencyFile) {
		fclose(g_fpCSVLatencyFile);
		g_fpCSVLatencyFile = NULL;
	}
	if (g_fpDebugFile) {
		fclose(g_fpDebugFile);
		g_fpDebugFile = NULL;
	}

    /* --------------------------------------------------------------------------------
     * If the files created (mqttbench.csv, latency.csv and mqttbench.log) are empty
     * then remove them.
     * -------------------------------------------------------------------------------- */
   	removeFile(csvLogFile, REMOVE_EMPTY_FILE);
   	removeFile(csvLatencyFile, REMOVE_EMPTY_FILE);

	/* Free the memory for the csv filenames */
    if (csvLogFile)
    	free(csvLogFile);
    if (csvLatencyFile)
		free(csvLatencyFile);

    /* --------------------------------------------------------------------------------
     * Check to see if the arrays for the clientID, password and topic names was
     * deleted.  If it hasn't been deleted then clean up the memory used for it now.
     * -------------------------------------------------------------------------------- */
	doRemoveHashMaps();

	/* Removed the HashMaps that were needed if there were client certificates. */
	if (g_ccertsMap) {
		doCleanUpHashMap(g_ccertsMap);
		g_ccertsMap = NULL;
	}
	if (g_cpkeysMap) {
		doCleanUpHashMap(g_cpkeysMap);
		g_cpkeysMap = NULL;
	}
	if (g_MsgCfgInfo->msgDirMap) {
		doCleanUpHashMap(g_MsgCfgInfo->msgDirMap);
		g_MsgCfgInfo->msgDirMap = NULL;
	}

    /* --------------------------------------------------------------------------------
     * Check to see if the arrays for the PreSharedKeys was deleted. If it hasn't been
     * deleted then clean up the memory used for it now.
     * -------------------------------------------------------------------------------- */
#ifdef _NEED_TO_FIX
	if (pIDSharedKeyArray) {
		if (pIDSharedKeyArray->pskIDArray)
			doCleanUpArrayList(PSK_ID_ARRAY);
		if (pIDSharedKeyArray->pskKeyArray)
			doCleanUpArrayList(PSK_KEY_ARRAY);

    	free(pIDSharedKeyArray);
	}
#endif /* _NEED_TO_FIX */

	/* Remove the histograms */
	for ( i = 0 ; i < MAX_NUM_HISTOGRAMS ; i++ ) {
		if (g_fpHgramFiles[i]) {
			fclose(g_fpHgramFiles[i]);
			g_fpHgramFiles[i]= NULL;
		}
	}

	/* --------------------------------------------------------------------------------
	 * Free up the global message payloads array which contains the Message Size Array
	 * and Message Buffer Arrays.
	 * -------------------------------------------------------------------------------- */
	if (pMsgCfgInfo) {
		if (pMsgCfgInfo->pArrayMsg) {
			numElems = pMsgCfgInfo->pArrayMsg->numMsgPayloads;

			if (pMsgCfgInfo->pArrayMsg->srcOfAllMsgs > PARM_S_OPTION) {
				for ( i = 0 ; i < numElems ; i++ ) {
					currMsgPayload = pMsgCfgInfo->pArrayMsg->arrayMsgPtr[i];
					if (currMsgPayload) {
						for ( j = 0 ; j < currMsgPayload->numMsgSizes ; j++ ) {
							if (currMsgPayload->msgBfrArray) {
								if (currMsgPayload->msgBfrArray[j]) {
									free(currMsgPayload->msgBfrArray[j]);
									currMsgPayload->msgBfrArray[j] = NULL;
								}
							}
							if (currMsgPayload->msgFilenameArray) {
								if (currMsgPayload->msgFilenameArray[j]) {
									free(currMsgPayload->msgFilenameArray[j]);
									currMsgPayload->msgFilenameArray[j] = NULL;
								}
							}
						}
					}
				}
			} else {
				currMsgPayload = pMsgCfgInfo->pArrayMsg->arrayMsgPtr[0];
				free(currMsgPayload->msgBfrArray);
				currMsgPayload->msgBfrArray = NULL;
			}

			free(currMsgPayload);
			currMsgPayload = NULL;

			free(pMsgCfgInfo->pArrayMsg);
			pMsgCfgInfo->pArrayMsg = NULL;
		}

		free(pMsgCfgInfo);
		pMsgCfgInfo = NULL;
	}

    /* Create an iterator to cleanup the memory for clients. */
    pClientIter = (ism_common_listIterator *)calloc(1, sizeof(ism_common_listIterator));
    if (pClientIter == NULL) {
    	rc = provideAllocErrorMsg("an iterator", (int)sizeof(ism_common_listIterator), __FILE__, __LINE__);
    	exit (rc);
    }

    if ((mbInstArray) && (numSubmitThrds)) {
    	for ( k = 0 ; k < numSubmitThrds ; k++ ) {
    		if (mbInstArray[k]) {
    			if (mbInstArray[k]->clientTuples) {
    				ism_common_list_iter_init(pClientIter, mbInstArray[k]->clientTuples);

    				while ((currClient = ism_common_list_iter_next(pClientIter)) != NULL) {
    					mqttclient_t *client = (mqttclient_t *) currClient->data;
    					pthread_spin_lock(&client->mqttclient_lock);       /* Lock the spinlock */
    					if(client->destRxSubIDMap) {
    						ism_common_destroyHashMap(client->destRxSubIDMap);
    					}
    					if (client->destRxListCount > 0) {
    						for ( i = 0 ; i < client->destRxListCount ; i++ ) {
    							if (client->destRxList) {
    								if (client->destRxList[i]->topicName) {
    									free(client->destRxList[i]->topicName);
    									client->destRxList[i]->topicName = NULL;
    								}
    							}
    						}

    						free(client->destRxList);
    						client->destRxList = NULL;
    					}

    					if (((mqttclient_t *)(currClient->data))->destRxListCount > 0) {
    						for ( i = 0 ; i < ((mqttclient_t *)(currClient->data))->destTxListCount ; i++ ) {
    							if (((mqttclient_t *)(currClient->data))->destTxList) {
    								if (((mqttclient_t *)(currClient->data))->destTxList[i]->topicName) {
    									free(((mqttclient_t *)(currClient->data))->destTxList[i]->topicName);
    									((mqttclient_t *)(currClient->data))->destTxList[i]->topicName = NULL;
    								}
    							}
    						}

    						free(((mqttclient_t *)(currClient->data))->destTxList);
    						((mqttclient_t *)(currClient->data))->destTxList = NULL;
    					}

                        if (((mqttclient_t *)(currClient->data))->partialMsg) {
                            ism_common_free(ism_memory_utils_misc, ((mqttclient_t *)(currClient->data))->partialMsg);
                            ((mqttclient_t *)(currClient->data))->partialMsg = NULL;
                        }

    					if (((mqttclient_t *)(currClient->data))->clientID) {
    						free(((mqttclient_t *)(currClient->data))->clientID);
    						((mqttclient_t *)(currClient->data))->clientID = NULL;
    						((mqttclient_t *)(currClient->data))->clientIDLen = 0;
    					}

    					if (((mqttclient_t *)(currClient->data))->inflightMsgIds)
    						free(((mqttclient_t *)(currClient->data))->inflightMsgIds);

    					if (((mqttclient_t *)(currClient->data))->trans) {
    						if (((mqttclient_t *)(currClient->data))->trans->socket > 0)
    							close(((mqttclient_t *)(currClient->data))->trans->socket);
        					if (((mqttclient_t *)(currClient->data))->trans->serverIP) {
        						free(((mqttclient_t *)(currClient->data))->trans->serverIP);
        						((mqttclient_t *)(currClient->data))->trans->serverIP = NULL;
        					}
    						free(((mqttclient_t *)(currClient->data))->trans);
    						((mqttclient_t *)(currClient->data))->trans = NULL;
    					}

    					if (((mqttclient_t *)(currClient->data))->psk_id) {
    						free(((mqttclient_t *)(currClient->data))->psk_id);
    						((mqttclient_t *)(currClient->data))->psk_id = NULL;
    					}

    					if (((mqttclient_t *)(currClient->data))->psk_key) {
    						free(((mqttclient_t *)(currClient->data))->psk_key);
    						((mqttclient_t *)(currClient->data))->psk_key = NULL;
    					}

    					pthread_spin_unlock(&(((mqttclient_t *)(currClient->data))->mqttclient_lock));       /* Unlock the spinlock */
    					pthread_spin_destroy(&(((mqttclient_t *)(currClient->data))->mqttclient_lock));       /* Unlock the spinlock */
    				}

    				ism_common_list_iter_destroy(pClientIter);
    				ism_common_list_destroy(mbInstArray[k]->clientTuples);
    				free(mbInstArray[k]->clientTuples);
    			}
    		}
   		} /* for ( k = 0 ; k < numSubmitThrds ; k++ ) */

   		free(mbInstArray);
   		mbInstArray = NULL;
   		g_MqttbenchInfo->mbInstArray = NULL;
   	}

    /* Cleanup the memory for the Hash map values and entries. */
    if (qtMap)
    	doCleanUpHashMap(qtMap);

    /* Cleanup the Command Line Args structure. */

    /* If measuring jitter than ensure that the array are freed. */
    if (pCmdLineArgs) {
    	if (pCmdLineArgs->determineJitter) {
    		pCmdLineArgs->determineJitter = 0;  /* disable jitter */

    		if (pJitterInfo) {
    			if (pJitterInfo->rateArray_Cons_PUB)
    				free(pJitterInfo->rateArray_Cons_PUB);
    			if (pJitterInfo->rateArray_Cons_PUBACK)
    				free(pJitterInfo->rateArray_Cons_PUBACK);
    			if (pJitterInfo->rateArray_Cons_PUBREC)
    				free(pJitterInfo->rateArray_Cons_PUBREC);
    			if (pJitterInfo->rateArray_Cons_PUBCOMP)
    				free(pJitterInfo->rateArray_Cons_PUBCOMP);
    			if (pJitterInfo->rateArray_Prod_PUB)
    				free(pJitterInfo->rateArray_Prod_PUB);
    			if (pJitterInfo->rateArray_Prod_PUBACK)
    				free(pJitterInfo->rateArray_Prod_PUBACK);
    			if (pJitterInfo->rateArray_Prod_PUBREC)
    				free(pJitterInfo->rateArray_Prod_PUBREC);
    			if (pJitterInfo->rateArray_Prod_PUBCOMP)
    				free(pJitterInfo->rateArray_Prod_PUBCOMP);

    			free(pJitterInfo);
    			pJitterInfo = NULL;
    		}
    	}

    	if (pCmdLineArgs->stringCmdLineArgs) {
			free(pCmdLineArgs->stringCmdLineArgs);
			pCmdLineArgs->stringCmdLineArgs = NULL;
		}

    	if (pCmdLineArgs->clientListPath) {
    		free(pCmdLineArgs->clientListPath);
    		pCmdLineArgs->clientListPath = NULL;
    	}

    	if (pCmdLineArgs->crange) {
    		free(pCmdLineArgs->crange);
    		pCmdLineArgs->crange = NULL;
    	}

    	free(pCmdLineArgs);
    }

    free(pClientIter);

    /* Call the tcp cleanup function. */
    if (pSysEnvSet) {
    	tcpCleanup();

    	if (pSysEnvSet->numEnvSourceIPs > 0) {
    		for ( i = 0 ; i < pSysEnvSet->numEnvSourceIPs ; i++ ) {
    			free(g_EnvSIPs[i]);
    		}
		}
	}

	if (g_MqttbenchInfo->timerThreadsInit == 1)
		ism_common_stopTimers();

    /* Remove any simple linked lists and memory for the arrays. */
    if (g_srcPorts) {
        free(g_srcPorts);
    }
    if (g_EnvSIPs) {
    	free(g_EnvSIPs);
    }
	/* Cleanup all spinlocks */
	if (ready_lock > 0)
		pthread_spin_destroy(&ready_lock);
	if (submitter_lock > 0)
		pthread_spin_destroy(&submitter_lock);
	if (mbinst_lock > 0)
		pthread_spin_destroy(&mbinst_lock);

    /* If performing latency initialize the spinlock */
	if ((latencyMask & LAT_CONN_AND_SUBSCRIBE) > 0) {
		if (connTime_lock > 0)
			pthread_spin_destroy(&connTime_lock);

		if (((latencyMask & (CHKTIMETCPCONN | CHKTIMETCP2MQTTCONN | CHKTIMETCP2SUBSCRIBE)) > 0) &&
			(mqttlatency_lock > 0))
			pthread_spin_destroy(&mqttlatency_lock);
	}

    /* The pSysEnvSet must be freed after calling tcpCleanup() */
    if (pSysEnvSet) {
    	if ((cSSLClientMethod) && strcmp(cSSLClientMethod, "") > 0)
    		free(pSysEnvSet->sslClientMethod);
    	if ((cSSLCipher) && strcmp(cSSLCipher, "") > 0)
    		free(pSysEnvSet->sslCipher);
    	if (pSysEnvSet->negotiated_sslCipher)
        	free(pSysEnvSet->negotiated_sslCipher);
    	if (pSysEnvSet->strSIPList)
    		free(pSysEnvSet->strSIPList);

    	free(pSysEnvSet);
    	pSysEnvSet = NULL;
    }

    /* Free up the memory for the g_MqttbenchInfo->mbSSLBfrEnv */
    if (g_MqttbenchInfo->mbSSLBfrEnv) {
    	free(g_MqttbenchInfo->mbSSLBfrEnv);
    	g_MqttbenchInfo->mbSSLBfrEnv = NULL;
    }

    /* Free up the memory for the Burst Mode structure */
    if (g_MqttbenchInfo->mbBurstInfo) {
    	free(g_MqttbenchInfo->mbBurstInfo);
    	g_MqttbenchInfo->mbBurstInfo = NULL;
    }
} /* doCleanUp */

/* *************************************************************************************
 * cleanUpLogFiles
 *
 * Description:  Remove pre-exiting mqttbench log and stats files before
 * *************************************************************************************/
void resetLogFiles (void)
{
    /* --------------------------------------------------------------------------------
     * If any of the following files exist then erase them:
     *    - mqttbench_latstats.csv
     *    - mqttbench_stats.csv
     *    - mqttbench_clientdump.log
     * -------------------------------------------------------------------------------- */
   	removeFile(CLIENT_INFO_FNAME, REMOVE_FILE_EXISTS);
   	removeFile(CSV_STATS_FNAME, REMOVE_FILE_EXISTS);
   	removeFile(CSV_LATENCY_FNAME, REMOVE_FILE_EXISTS);
} /* cleanUpLogFiles */

/* *************************************************************************************
 * doRemoveHashMaps
 *
 * Description:  Remove the Hostname and SIP Hash Maps and the entries associated with it.
 * *************************************************************************************/
void doRemoveHashMaps (void)
{
	if (g_dstIPMap) {
		doCleanUpHashMap(g_dstIPMap);
		g_dstIPMap = NULL;
	}
	if (g_srcIPMap) {
		doCleanUpHashMap(g_srcIPMap);
		g_srcIPMap = NULL;
	}
	if (g_ccertsMap) {
		doCleanUpHashMap(g_ccertsMap);
		g_ccertsMap = NULL;
	}
	if (g_cpkeysMap) {
		doCleanUpHashMap(g_cpkeysMap);
		g_cpkeysMap = NULL;
	}

} /* doRemoveHashMaps */

/* *************************************************************************************
 * doCleanUpHashMap
 *
 * Description:  Cleanup the specified Hash Map values and entries.
 *
 *   @param[in]  hashMap             = HashMap to be freed
 * *************************************************************************************/
void doCleanUpHashMap (ismHashMap *hashMap)
{
    ismHashMapEntry **entries;

    if (hashMap) {
    	entries = ism_common_getHashMapEntriesArray(hashMap);
		/* Free up the process's Hash Table. */
    	if (entries)
    		ism_common_freeHashMapEntriesArray(entries);

    	ism_common_destroyHashMap(hashMap);
    }
} /* doCleanUpHashMap */
