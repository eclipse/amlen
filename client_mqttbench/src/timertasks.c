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

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ismutil.h>

#include "mqttbench.h"
#include "tcpclient.h"
#include "timertasks.h"


/* ******************************** GLOBAL VARIABLES ********************************** */
/* Externs */
extern uint8_t  g_RequestedQuit;
extern uint8_t  g_StopProcessing;          /* Stop processing messages and/or jobs */
extern int      g_TotalNumClients;
extern uint32_t g_TotalAttemptedRetries;
extern double   g_ConnRetryFactor;
extern int      g_MBTraceLevel;
extern uint8_t  g_ClkSrc;
extern int      g_numClientsWithConnTimeout;

extern mqttbenchInfo_t *g_MqttbenchInfo;
extern ioProcThread_t **ioProcThreads;

extern char *csvLatencyFile;
extern char *csvLogFile;

extern FILE *g_fpCSVLogFile;
extern FILE *g_fpCSVLatencyFile;

/* Globals - Initial declaration */
uint8_t g_JitterLock = 0;
uint8_t g_RenamingFiles_lock = 0;
uint8_t g_ResetHeaders = 0;

static int onTimerClientScan (ism_timer_t, ism_time_t, void *);
static int onTimerDelayTimeout (ism_timer_t, ism_time_t, void *);
static int onTimerJitter (ism_timer_t, ism_time_t, void *);
static int onTimerLatencyStatsReset (ism_timer_t, ism_time_t, void *);
static int onTimerLinger (ism_timer_t, ism_time_t, void *);
static int onTimerMemoryCleanup (ism_timer_t, ism_time_t, void *);
static int onTimerSnapRequests (ism_timer_t, ism_time_t, void *);


/* ************************************************************************************ */
/* ************************************************************************************ */
/*                                 Timer Functions                                      */
/* ************************************************************************************ */
/* ************************************************************************************ */

/* *************************************************************************************
 * onTimerLatencyStatsReset
 *
 * Description:  This task will schedule a job for the IOP to reset the latency
 *               statistics if specified with the -rl option.
 *
 *   @param[in]  latencyKey          = Identifier for the scheduled Latency Statistics
 *                                     Reset task.
 *   @param[in]  timestamp           = x
 *   @param[in]  userdata            = The System Environment Settings structure.
 *
 *   @return 0   = Timer has been cancelled
 *           1   = Timer has been rescheduled
 * *************************************************************************************/
static int onTimerLatencyStatsReset (ism_timer_t latencyKey, ism_time_t timestamp, void *userdata)
{
	environmentSet_t *pSysEnvSet = (environmentSet_t *)userdata;

	int rc = 1;           /* Reschedule timer task by default */
	int i;

	if (g_RequestedQuit == 0) {
		if (g_RenamingFiles_lock == 0) {
			for ( i = 0 ; i < pSysEnvSet->numIOProcThreads ; i++ ) {
				addJob2IOP(ioProcThreads[i], NULL, 0, resetLatencyStats);
			}
		}
	} else {
		/* Shutting down so cancel the Latency Stats timer. */
		rc = ism_common_cancelTimer(latencyKey);
		rc = 0;  /* Cancel the timer since the user requested a quit. */
	}

	return rc;
} /* onTimerLatencyStatsReset */

/* *************************************************************************************
 * onTimerLinger
 *
 * Description:  Schedule a timer task for a client to linger prior to attempting
 *               a reconnect.
 *
 *   @param[in]  lingerKey           = Identifier for the scheduled Linger task, used to cancel the timer.
 *   @param[in]  timestamp           = timestamp when the timer expired
 *   @param[in]  userdata            = user data passed to the callback function.
 *
 *   @return 0   = Timer has been cancelled
 * *************************************************************************************/
static int onTimerLinger (ism_timer_t lingerKey, ism_time_t timestamp, void *userdata)
{
	int rc = 0;    /* Set return code = 0 to ensure the timer is cancelled. */
	int disconnectType = g_MqttbenchInfo->mbCmdLineArgs->disconnectType;
	mqttclient_t *client = (mqttclient_t *) userdata;
	transport_t *trans = client->trans;

	MBTRACE(MBDEBUG, 8, "Linger timer expired for client %s (line=%d), initiating disconnect sequence\n",
						  client->clientID, client->line);

	/* Since this is called only ONCE then need to cancel the timer. */
	rc = ism_common_cancelTimer(lingerKey);
	if (rc)
		MBTRACE(MBERROR, 6, "Unable to cancel the Linger Timer (rc = %d).\n", rc);

	/* --------------------------------------------------------------------------------
	 * Check that the transport is NOT NULL and that the transport state is NOT
	 * SOCK_ERROR.
	 *
	 * If it is a Consumer then need to submit a job to have an MQTT UNSUBSCRIBE job
	 * scheduled for each connection.   Once the UnSubscribe is completed the clients
	 * will schedule a MQTT DISCONNECT.
	 *
	 * If the reconnectEnabled bit is set to 1 then the TCP code reschedules a Reconnect
	 * for that client.
	 * -------------------------------------------------------------------------------- */
	if ((trans) && ((trans->state & SOCK_ERROR) == 0) && client) {
		client = trans->client;

		if ((client->issuedDisconnect == 0) &&
			(trans->ioProcThread) &&
			(trans->ioProcThread->isStopped == 0)) {

			/* ------------------------------------------------------------------------
			 * Check to see if this is a Consumer or not.  A Consumer must first schedule
			 * a MQTT UNSUBSCRIBE job to UnSubscribe from all the topics it has
			 * Subscribed to.   When the UNSUBACK is received it will then schedule an
			 * MQTT DISCONNECT job for itself.
			 *
			 * The Producer just needs to schedule a for MQTT DISCONNECT.
			 *
			 * Any of these jobs must be performed by the IOP and not the Consumer or
			 * Producer Thread.  This is due to the fact that the Protocol and Transport
			 * States need to be modified by the dedicated IOP so there isn't any
			 * inconsistencies on states.
			 * ------------------------------------------------------------------------ */
 			if (((client->clientType & CONSUMER) > 0) && (client->destRxListCount > 0) &&
				(disconnectType == DISCONNECT_WITH_UNSUBSCRIBE)) {
				rc = submitMQTTUnSubscribeJob(trans);
				if (rc)
					MBTRACE(MBERROR, 6, "Unable to perform MQTT Unsubscribe (rc: %d).\n", rc);
			} else {
				client->disconnectRC = MQTTRC_OK;
				rc = submitMQTTDisconnectJob(trans, NODEFERJOB);
				if (rc)
					MBTRACE(MBERROR, 6, "Unable to perform MQTT Disconnect (rc: %d).\n", rc);
			}
		}
	} /* if ((trans) && ((trans->state & SOCK_ERROR) == 0)) */

	return 0;  /* Return '0', which ensures the timer is cancelled. */
} /* onTimerLinger */

/* *************************************************************************************
 * onTimerMemoryCleanup
 *
 * Description:  Timer that will trim memory so that it can be returned to the system.
 *               If running SSL then there will be quite alot of memory being used by
 *               the Crypto package.
 *
 *   @param[in]  memCleanupKey       = Identifier for the scheduled Memory Cleanup task.
 *   @param[in]  timestamp           = x
 *   @param[in]  userdata            = NULL
 *
 *   @return 0   = Timer has been cancelled
 *           1   = Timer has been rescheduled
 * *************************************************************************************/
static int onTimerMemoryCleanup (ism_timer_t memCleanupKey, ism_time_t timestamp, void *userdata)
{
	int rc = 1;           /* Reschedule timer task by default */

	/* --------------------------------------------------------------------------------
	 * Check to see if the user didn't requested a quit.   If so, then return 0 so
	 * the timer will cancel itself.   Otherwise perform a memory trim.
	 * -------------------------------------------------------------------------------- */
	if (g_RequestedQuit == 0) {
		/* Log the fact that a malloc_trim was performed */
		MBTRACE(MBINFO, 5, "Performing a malloc_trim\n");

		malloc_trim(0);
	} else {
		/* Shutting down so cancel the malloc trim timer. */
		rc = ism_common_cancelTimer(memCleanupKey);
		rc = 0;   /* Cancel the timer since user requested to quit. */
	}

    return rc;
} /* onTimerMemoryCleanup */

/* *************************************************************************************
 * onTimerClientScan
 *
 * Description:  Timer task which is used to check client that have been in HANDSHAKE_IN_PROCESS
 *               or MQTT_CONNECT_IN_PROGRESS longer than the configurable timeout.
 *
 *   @param[in]  clientScanKey       = Identifier for the client scan timer task.
 *   @param[in]  timestamp           = x
 *   @param[in]  userdata            = The mqttbenchInfo structure.
 *
 *   @return 0   = Timer has been cancelled
 *           1   = Timer has been rescheduled
 * *************************************************************************************/
int onTimerClientScan (ism_timer_t clientScanKey, ism_time_t timestamp, void *userdata)
{
	mqttbenchInfo_t *mqttbenchInfo = (mqttbenchInfo_t *)userdata;
	mbCmdLineArgs_t *pCmdLineArgs = mqttbenchInfo->mbCmdLineArgs;
	mqttbench_t **mbInstArray = mqttbenchInfo->mbInstArray;

	int rc = 1;           /* Reschedule timer task by default */

	if (g_RequestedQuit == 0) {
		double now = g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime();

		/* Iterate through the list of submitter threads */
		for (int i=0 ; i < pCmdLineArgs->numSubmitterThreads ; i++) {
			/* Iterate through client list per submitter thread */
			ism_common_listIterator iter;
			ism_common_list *clients = mbInstArray[i]->clientTuples;
			ism_common_list_iter_init(&iter, clients);
			ism_common_list_node *currTuple;
			while ((currTuple = ism_common_list_iter_next(&iter)) != NULL) {
				mqttclient_t *client = (mqttclient_t *) currTuple->data;
				if (client->connectionTimeoutSecs == 0) {
					continue; // skip clients which explicitly disable connection timeout
				}
				transport_t *trans = client->trans;
				int pCInProcess = client->protocolState & MQTT_CONNECT_IN_PROCESS;
				int pDInProcess = client->protocolState & MQTT_DISCONNECTED;
				int tInProcess = trans->state & (HANDSHAKE_IN_PROCESS | SOCK_WS_IN_PROCESS);
				int rInProcess = (trans->state & SOCK_NEED_CREATE) && (client->resetConnTime != 0);
				if (pCInProcess || pDInProcess || tInProcess || rInProcess) {
					double pConnWaitTime = now - client->mqttConnReqSubmitTime;
					double pDisConnWaitTime = now - client->mqttDisConnReqSubmitTime;
					double tConnWaitTime = now - client->tcpConnReqSubmitTime;
					double rConnWaitTime = now - client->resetConnTime;
					if ((tInProcess && (tConnWaitTime > client->connectionTimeoutSecs)) ||
						(pDInProcess && (pDisConnWaitTime > client->connectionTimeoutSecs)) ||
						(pCInProcess && (pConnWaitTime > client->connectionTimeoutSecs)) ||
						(rInProcess && (rConnWaitTime > client->connectionTimeoutSecs)))
					{
						MBTRACE(MBWARN, 5, "%s client @ line %d, ID=%s was stuck (to=%d secs) for t=%.3f pc=%.3f pd=%.3f r=%.3f seconds (tip=%d pcip=%d pdip=%d rip=%d) attempting to connect or disconnect (tstate=0x%X pstate=0x%X), initiating reconnect\n",
											"TIMER: client scan:", client->line, client->clientID, client->connectionTimeoutSecs, tConnWaitTime, pConnWaitTime, pDisConnWaitTime, rConnWaitTime,
											tInProcess, pCInProcess, pDInProcess, rInProcess, trans->state, client->protocolState);

						addJob2IOP(trans->ioProcThread, trans, 0, scheduleReconnectCallback);
					}
				}
			}
		}
	} else {
		/* Shutting down so cancel the Latency Warning timer. */
		rc = ism_common_cancelTimer(clientScanKey);
		MBTRACE(MBINFO, 7, "Cancelled the Client Scan Timer Task due to requested quit.\n");
		rc = 0;  /* Cancel the timer since the user requested a quit. */
	}

	return rc;
} /* onTimerClientScan */

/* *************************************************************************************
 * onTimerSnapRequests
 *
 * Description:  This task will perform the requested snap options at the specified
 *               interval:
 *                  - Message Rates       (msgrates)
 *                  - Message Counts      (msgcount)
 *                  - Connections         (connects)
 *                  - Connection Latency  (connlatency)
 *                  - Message Latency     (msglatency)
 *
 *               The message rates, counts and connections utilize getStats() while
 *               latency utilizes calculateLatency().
 *
 *               Data is recorded in the following files based on the function utilized:
 *
 *               getStats()          recorded based on -csv [filename] option
 *                                   (default: mqttbench.csv)
 *               calculateLatency()  recorded based on -lcsv [filename] option
 *                                   (default: latency.csv)
 *
 *   @param[in]  snapKey             = x
 *   @param[in]  timestamp           = x
 *   @param[in]  userdata            = The mqttbenchInfo structure.
 *
 *   @return 0   = Timer has been cancelled
 *           1   = Timer has been rescheduled
 * *************************************************************************************/
static int onTimerSnapRequests (ism_timer_t snapKey, ism_time_t timestamp, void *userdata)
{
	mqttbenchInfo_t *mqttbenchInfo = (mqttbenchInfo_t *)userdata;

	int rc = 1;           /* Reschedule timer task by default */
	int tmpRC = 0;        /* Temporary Return code. 0 = OK */

	int latencyMask = mqttbenchInfo->mbCmdLineArgs->latencyMask;

	uint8_t localSnapStatTypes = mqttbenchInfo->mbCmdLineArgs->snapStatTypes;

	/* Check to make sure the user hasn't requested a quit and the indicating for
	 * renaming files must be equal to 0 . */
	if (g_RequestedQuit == 0) {
		if (g_RenamingFiles_lock == 0) {
			/* ------------------------------------------------------------------------
			 * Check if doing snaps for Connections, Msg Rates or Msg Counts and if so
			 * then need to call getStats( )
			 * ------------------------------------------------------------------------ */
			if ((localSnapStatTypes & (SNAP_CONNECT | SNAP_RATE | SNAP_COUNT)) > 0) {
				tmpRC = getStats(mqttbenchInfo, OUTPUT_CSVFILE, 0, g_fpCSVLogFile);

				/* --------------------------------------------------------------------
			 	 * If there is a non-zero rc from getStats then need to cancel the timer
			 	 * by returning rc = 0
			 	 * -------------------------------------------------------------------- */
				if (tmpRC) {
					MBTRACE(MBERROR, 5, "Failure obtaining Snap Statistics (rc = %d)\n", tmpRC);
					return rc;
				}
			}

			/* ----------------------------------------------------------------------------
			 * Check if doing snaps for Connection Latency or Message Latency.  If so then
			 * need to call calculateLatency( ) with the latencyMask.
			 * ---------------------------------------------------------------------------- */
			if ((localSnapStatTypes & (SNAP_LATENCY | SNAP_CONNECT_LATENCY)) > 0) {
				/* Get latency for all requested. */
				if (latencyMask)
					calculateLatency(mqttbenchInfo,
                                     latencyMask,
                                     OUTPUT_CSVFILE,
                                     1,
                                     g_fpCSVLatencyFile,
                                     NULL);
			}

			g_ResetHeaders = 0;  /* Set the reset headers flag equal to 0 */
		}
	} else {
		/* Shutting down so cancel the Snap timer. */
		rc = ism_common_cancelTimer(snapKey);
		MBTRACE(MBINFO, 7, "Cancelled Snap Timer Task due to requested quit.\n");
		rc = 0;  /* Cancel the timer since the user requested a quit. */
	}

	return rc;
} /* onTimerSnapRequests */

/* *************************************************************************************
 * onTimerDelayTimeout
 *
 * Description:  This is the Delay Timer, which is a set call timer once.  Need to
 *               ensure that the timer is explicitly cancelled and returns a rc = 0.
 *
 *               It performs a delay before attempting a TCP Connection or a reconnect.
 *
 *                  -  Based on the command line option:  -recon <secs>
 *                  -  Before submitting the job to create a socket it must reset the
 *                     client's reconnectStatus to 0.
 *
 *               2 environment variables that must be set to enable a longer delay between
 *               attempts:
 *
 *                  RetryDelay  - Number of microseconds to wait for the init delay if
 *                                there was a failure.
 *                  RetryFactor - The multiplier factor to sleep if there is a failure.
 *
 *               When the RetryFactor and RetryBackoff factor are set then the delay is
 *               based on multiplying the RetryFactor against the previous value to setup
 *               for the next time if there is a failure.
 *
 *               TCP Connection : Upon expiration of the timer, a job is submitted to
 *                                create a connection (submitCreateConnectionJob).
 *
 *               Reconnection :   Upon expiration of the timer, a job is submitted to
 *                                create a socket (submitCreateSocketJob).
 *
 *   @param[in]  delayKey            = timer key used for cancelling the timer
 *   @param[in]  timestamp           = timestamp when the timer expired
 *   @param[in]  userdata            = user data passed to callback function
 *
 *   @return 0   =
 * *************************************************************************************/
static int onTimerDelayTimeout (ism_timer_t delayTimeoutKey, ism_time_t timestamp, void *userdata)
{
	int rc = 0;
	static int numTimes = 1;

	transport_t *trans = (transport_t *) userdata;
	mqttclient_t *client = trans->client;
	uint8_t reconnectEnabled = trans->reconnect;

	MBTRACE(MBDEBUG, 8, "Delay timer expired for client %s (line=%d), initiating reconnect sequence\n",
						  client->clientID, client->line);

	/* Cancel the Delay Timer since it is a set call timer once. */
	rc = ism_common_cancelTimer(delayTimeoutKey);
	if (rc)
		MBTRACE(MBERROR, 6, "Unable to cancel the Delay Timer (rc = %d).\n", rc);

	/* First check to see if the user requested to quit and that trans is not NULL */
	if ((g_RequestedQuit == 0) && (trans)) {
		/* Check to see if the trans ioProcThread and client are not NULL. */
		if ((trans->ioProcThread) && client) {
			/* ------------------------------------------------------------------------
			 * If the transport state is either SOCK_DISCONNECTED or SOCK_ERROR and
			 * then submit a job to the IOP which will ultimately reschedule it.
			 * ------------------------------------------------------------------------ */
			if (((trans->state & (SOCK_DISCONNECTED | SOCK_ERROR)) > 0) && (reconnectEnabled)) {
				submitJob4IOP(trans);
				return 0;   /* Return '0', which ensures the timer is cancelled. */
			}

			client = trans->client;

			/* ------------------------------------------------------------------------
			 * If the connection retry factor is set then need to set the next delay
			 * amount.  Based on the value of the retry factor the increase for the
			 * next delay is calculated.
			 *
			 * if > 1 then just multiply the current value with the factor
			 * if < 1 then multiply the current value and to the previous value.
			 * ------------------------------------------------------------------------ */
			if (g_ConnRetryFactor > 0) {
				if (g_ConnRetryFactor > 1)
					client->currRetryDelayTime_ns *= g_ConnRetryFactor;  /* Set the next interval for a failure. */
				else
					client->currRetryDelayTime_ns += (client->currRetryDelayTime_ns * g_ConnRetryFactor);
			}

			/* ------------------------------------------------------------------------
			 * Test the trans->state:
			 *
			 *  - trans->state & SOCK_NEED_CONNECT  (brand new connection delay)
			 *  - trans->state & SOCK_NEED_CREATE   (reconnection delay)
			 * ------------------------------------------------------------------------ */
			if (trans->state & SOCK_NEED_CONNECT) {
				if (client->connRetries > g_TotalAttemptedRetries)
					g_TotalAttemptedRetries = client->connRetries;

				submitCreateConnectionJob(trans);
			} else if ((reconnectEnabled) && (trans->state & SOCK_NEED_CREATE)) {
				if ((++numTimes % g_TotalNumClients) == 0) {
					MBTRACE(MBINFO, 1, "Client %s (line=%d) is attempting to reconnect...\n", client->clientID, client->line);
					numTimes = 1;
				}

				submitCreateSocketJob(trans);
			}
		}
	}

	return 0;   /* Return '0', which ensures the timer is cancelled. */
} /* onTimerDelayTimeout */

/* *************************************************************************************
 * onTimerBurstMode
 *
 * Description:  Timer task which is used for the burst mode feature.  The timer is
 *               initially called by main, but subsequent changes will be performed by
 *               the timer task flipping between original message rate and the burst mode
 *               specified by the --burst parameter.
 *
 *   @param[in]  burstKey            = Identifier for the scheduled Burst Mode task.
 *   @param[in]  timestamp           = Current Timestamp
 *   @param[in]  userdata            = The mqttbenchInfo structure.
 *
 *   @return 0   = Timer has been cancelled
 * *************************************************************************************/
int onTimerBurstMode (ism_timer_t burstKey, ism_time_t timestamp, void *userdata)
{
	mqttbenchInfo_t *pMBInfo = (mqttbenchInfo_t *)userdata;

	int rc = 0;   /* Cancel the timer task. */
	int newRate = 0;
	uint64_t runInterval;

	mbBurstInfo_t *pBurstInfo = pMBInfo->mbBurstInfo;

	/* Cancel the timer prior to doing anything. */
	rc = ism_common_cancelTimer(burstKey);

	/* Ensure that the user hasn't requested a termination */
	if ((g_RequestedQuit == 0) && (rc == 0)) {
		if (pBurstInfo->currMode == BASE_MODE)
			newRate = pBurstInfo->burstMsgRate;
		else
			newRate = pBurstInfo->baseMsgRate;

		/* Call doChangeMessageRate function to perform the rate change. */
		rc = doChangeMessageRate(newRate, pMBInfo, TIMERTASK);
		if (rc == 0) {
			/* Update the currMode and interval since switching modes. */
			if (pBurstInfo->currMode == BASE_MODE) {
				pBurstInfo->currMode = BURST_MODE;
				runInterval = pBurstInfo->burstDuration;
			} else {
				pBurstInfo->currMode = BASE_MODE;
				runInterval =  pBurstInfo->burstInterval;
			}

			/* Reschedule the timer */
			pBurstInfo->timerTask = ism_common_setTimerOnce(ISM_TIMER_HIGH,
				                                            (ism_attime_t)onTimerBurstMode,
				                                            (void *)pMBInfo,
							                                runInterval);

			if (pBurstInfo->timerTask == NULL)
				MBTRACE(MBERROR, 6, "Unable to schedule timer for burst mode.\n");
		} else
			MBTRACE(MBWARN, 1, "Failed to change the message rate (rc=%d).\n", rc);
	} else {
		if (rc) {
			MBTRACE(MBERROR, 6, "Unable to cancel the timer for burst mode (r=%d)\n", rc);
		} else {
			MBTRACE(MBINFO, 7, "Cancelled Burst Timer Task due to requested quit.\n");
		}
	}

	return 0;  /* Return with '0' in order to cancel the timer. */
} /* onTimerBurstMode */

/* *************************************************************************************
 * onTimerLatencyWarning
 *
 * Description:  Timer task which is used to provide a warning message if the latency is
 *               larger than the size of the histogram.
 *
 *   @param[in]  latencyWarningKey   = Identifier for the scheduled Latency Warning task.
 *   @param[in]  timestamp           = x
 *   @param[in]  userdata            = The mqttbenchInfo structure.
 *
 *   @return 0   = Timer has been cancelled
 *           1   = Timer has been rescheduled
 * *************************************************************************************/
int onTimerLatencyWarning (ism_timer_t latencyWarningKey, ism_time_t timestamp, void *userdata)
{
	mqttbenchInfo_t *mqttbenchInfo = (mqttbenchInfo_t *)userdata;

	int rc = 1;           /* Reschedule timer task by default */

	if (g_RequestedQuit == 0) {
		/* ----------------------------------------------------------------------------
		 * Check to see if the latency that is being kept is larger than the size of
		 * the histogram.   If so then place a message in the mqttbench_trace.log, and
		 * display on stdout as well.
		 * ---------------------------------------------------------------------------- */
		provideLatencyWarning(mqttbenchInfo->mbSysEnvSet, mqttbenchInfo->mbCmdLineArgs->latencyMask);
	} else {
		/* Shutting down so cancel the Latency Warning timer. */
		rc = ism_common_cancelTimer(latencyWarningKey);
		MBTRACE(MBINFO, 7, "Cancelled the Latency Warning Timer Task due to requested quit.\n");
		rc = 0;  /* Cancel the timer since the user requested a quit. */
	}

	return rc;
} /* onTimerLatencyWarning */

/* *************************************************************************************
 * onTimerAutoRenameLogFiles
 *
 * Description:  This task will rename the csv and latency csv files every 24 hrs by
 *               appending a number suffix.
 *
 *   @param[in]  renameKey           = Identifier for the scheduled Rename task.
 *   @param[in]  timestamp           = x
 *   @param[in]  userdata            = The mqttbenchInfo structure.
 *
 *   @return 0   = Timer has been cancelled
 *           1   = Timer has been rescheduled
 * *************************************************************************************/
int onTimerAutoRenameLogFiles (ism_timer_t renameKey, ism_time_t timestamp, void *userdata)
{
	static int numSfx = 0;

	mqttbenchInfo_t *mqttbenchInfo = (mqttbenchInfo_t *)userdata;

	int rc = 1;           /* Reschedule timer task by default */
	int tmpRC = 0;

	uint8_t snapStatTypes = mqttbenchInfo->mbCmdLineArgs->snapStatTypes;

	/* Check to make sure the user hasn't requested a quit. */
	if (g_RequestedQuit == 0) {
		/* ----------------------------------------------------------------------------
		 * Set the renaming files flag so that the onSnap Timer doesn't trigger during
		 * the renaming of the current files.
		 * ---------------------------------------------------------------------------- */
		g_RenamingFiles_lock = 1;

		/* ----------------------------------------------------------------------------
		 * Check if the File Pointer for the CSV Log file is not NULL.  If so, then
		 * rename the file with an appended suffix.
		 * ---------------------------------------------------------------------------- */
		if ((g_fpCSVLogFile) &&
			((snapStatTypes & (SNAP_CONNECT | SNAP_RATE | SNAP_COUNT)) > 0))
			tmpRC = renameFile(csvLogFile, &g_fpCSVLogFile, numSfx);

		/* ----------------------------------------------------------------------------
		 * Check if the File Pointer for the CSV Latency file is not NULL.  If so, then
		 * rename the file with an appended suffix.
		 * ---------------------------------------------------------------------------- */
		if ((g_fpCSVLatencyFile) && (tmpRC == 0) &&
			((snapStatTypes & (SNAP_LATENCY | SNAP_CONNECT_LATENCY)) > 0))
			tmpRC = renameFile(csvLatencyFile, &g_fpCSVLatencyFile, numSfx);

		g_RenamingFiles_lock = 0;  /* reset the renaming files back to 0. */
		numSfx++;  /* Increment suffix for the next time through. */

		/* ----------------------------------------------------------------------------
		 * If there is a failure renaming the files, then set rc = 0.  Setting rc = 0
		 * will result in the timer task being cancelled.
		 *----------------------------------------------------------------------------- */
		if (tmpRC) {
			MBTRACE(MBWARN, 1, "Unable to rename files (rc = %d).\n", tmpRC);

			/* Cancel the Snap timer since it was unable to rename the files. */
			rc = ism_common_cancelTimer(renameKey);
			if (rc)
				MBTRACE(MBERROR, 6, "Unable to cancel the Rename Timer (rc: %d)\n", rc);

			rc = 0;  /* Reset the RC = 0 to ensure timer is not rescheduled. */
		}
	} else {
		/* Shutting down so cancel the Snap timer. */
		rc = ism_common_cancelTimer(renameKey);
		MBTRACE(MBINFO, 7, "Cancelled the Auto-renaming Timer Task due to requested quit.\n");
		rc = 0;  /* Cancel the timer since the user requested a quit. */
	}

	/* Set the flag for the snap function to reset the headers. */
	g_ResetHeaders = 1;

	return rc;
} /* onTimerAutoRenameLogFiles */

/* *************************************************************************************
 * onTimerJitter
 *
 * Description:  This timer routine gets the latest counters for publish messages to
 *               determine if there is any jitter in the current throughput.
 *
 *   @param[in]  jitterKey           = Identifier for the scheduled Jitter task.
 *   @param[in]  timestamp           = x
 *   @param[in]  userdata            = The mqttbenchInfo structure.
 *
 *   @return 0   =
 * *************************************************************************************/
static int onTimerJitter (ism_timer_t jitterKey, ism_time_t timestamp, void *userdata)
{
	/* Set the MqttbenchInfo structure pointer to the userdata. */
	mqttbenchInfo_t *mqttbenchInfo = (mqttbenchInfo_t *)userdata;

	environmentSet_t *pSysEnvSet = mqttbenchInfo->mbSysEnvSet;
	mbJitterInfo_t *pJitterInfo = mqttbenchInfo->mbJitterInfo;

	int rc = 1; /* Reschedule the timer task. */
	int currRate = 0;
	int currIndex = pJitterInfo->currIndex;

	double elapsedTime;
	double currTime = getCurrTime();

	currMessageCounts_t pCurrMsgCounts;

	int8_t rxQoSMask = pJitterInfo->rxQoSMask;
	int8_t txQoSMask = pJitterInfo->txQoSMask;

	uint8_t mbThreadClientType = mqttbenchInfo->instanceClientType;

	/* Ensure that the user hasn't requested a termination */
	if ((g_RequestedQuit == 0) && (g_StopProcessing == 0) && (g_JitterLock == 0)) {
		g_JitterLock = 1;

		/* Obtain the current counts from the IOPs */
		getCurrCounts((int)pSysEnvSet->numIOProcThreads, &pCurrMsgCounts);

		/* ----------------------------------------------------------------------------
		 * Calculate the rates for the different type of messages and store in the
		 * corresponding array.
		 * ---------------------------------------------------------------------------- */
		if (pJitterInfo->prevTime != 0) {
			elapsedTime = currTime - pJitterInfo->prevTime;

			if ((mbThreadClientType & CONTAINS_CONSUMERS) > 0) {
				currRate = (pCurrMsgCounts.cons_Rx_Curr_PUB - pJitterInfo->cons_Rx_Prev_PUB) / elapsedTime;
				pJitterInfo->rateArray_Cons_PUB[currIndex] = currRate;

				if (currRate > pJitterInfo->cons_PUBLISH_MaxRate)
					pJitterInfo->cons_PUBLISH_MaxRate = currRate;
				if (currRate < pJitterInfo->cons_PUBLISH_MinRate)
					pJitterInfo->cons_PUBLISH_MinRate = currRate;

				if (rxQoSMask & MQTT_QOS_1_BITMASK) {
					currRate = (pCurrMsgCounts.cons_Tx_Curr_PUBACK - pJitterInfo->cons_Tx_Prev_PUBACK) / elapsedTime;
					pJitterInfo->rateArray_Cons_PUBACK[currIndex] = currRate;

					if (currRate > pJitterInfo->cons_PUBACK_MaxRate)
						pJitterInfo->cons_PUBACK_MaxRate = currRate;
					if (currRate < pJitterInfo->cons_PUBACK_MinRate)
						pJitterInfo->cons_PUBACK_MinRate = currRate;
				}

				if (rxQoSMask & MQTT_QOS_2_BITMASK) {
					currRate = (pCurrMsgCounts.cons_Tx_Curr_PUBREC - pJitterInfo->cons_Tx_Prev_PUBREC) / elapsedTime;
					pJitterInfo->rateArray_Cons_PUBREC[currIndex] = currRate;

					if (currRate > pJitterInfo->cons_PUBREC_MaxRate)
						pJitterInfo->cons_PUBREC_MaxRate = currRate;
					if (currRate < pJitterInfo->cons_PUBREC_MinRate)
						pJitterInfo->cons_PUBREC_MinRate = currRate;

					currRate = (pCurrMsgCounts.cons_Tx_Curr_PUBCOMP - pJitterInfo->cons_Tx_Prev_PUBCOMP) / elapsedTime;
					pJitterInfo->rateArray_Cons_PUBCOMP[currIndex] = currRate;

					if (currRate > pJitterInfo->cons_PUBCOMP_MaxRate)
						pJitterInfo->cons_PUBCOMP_MaxRate = currRate;
					if (currRate < pJitterInfo->cons_PUBCOMP_MinRate)
						pJitterInfo->cons_PUBCOMP_MinRate = currRate;
				}

				/* Store the current results in the previous for the next sample */
				pJitterInfo->cons_Rx_Prev_PUB     = pCurrMsgCounts.cons_Rx_Curr_PUB;
				pJitterInfo->cons_Tx_Prev_PUBACK  = pCurrMsgCounts.cons_Tx_Curr_PUBACK;
				pJitterInfo->cons_Tx_Prev_PUBREC  = pCurrMsgCounts.cons_Tx_Curr_PUBREC;
				pJitterInfo->cons_Rx_Prev_PUBREL  = pCurrMsgCounts.cons_Rx_Curr_PUBREL;
				pJitterInfo->cons_Tx_Prev_PUBCOMP = pCurrMsgCounts.cons_Tx_Curr_PUBCOMP;
			}

			if ((mbThreadClientType & CONTAINS_PRODUCERS) > 0) {
				currRate = (pCurrMsgCounts.prod_Tx_Curr_PUB - pJitterInfo->prod_Tx_Prev_PUB) / elapsedTime;
				pJitterInfo->rateArray_Prod_PUB[currIndex] = currRate;

				if (currRate > pJitterInfo->prod_PUBLISH_MaxRate)
					pJitterInfo->prod_PUBLISH_MaxRate = currRate;
				if (currRate < pJitterInfo->prod_PUBLISH_MinRate)
					pJitterInfo->prod_PUBLISH_MinRate = currRate;

				if (txQoSMask & MQTT_QOS_1_BITMASK) {
					currRate = (pCurrMsgCounts.prod_Rx_Curr_PUBACK - pJitterInfo->prod_Rx_Prev_PUBACK) / elapsedTime;
					pJitterInfo->rateArray_Prod_PUBACK[currIndex] = currRate;

					if (currRate > pJitterInfo->prod_PUBACK_MaxRate)
						pJitterInfo->prod_PUBACK_MaxRate = currRate;
					if (currRate < pJitterInfo->prod_PUBACK_MinRate)
						pJitterInfo->prod_PUBACK_MinRate = currRate;
				}

				if (txQoSMask & MQTT_QOS_2_BITMASK) {
					currRate = (pCurrMsgCounts.prod_Rx_Curr_PUBREC - pJitterInfo->prod_Rx_Prev_PUBREC) / elapsedTime;
					pJitterInfo->rateArray_Prod_PUBREC[currIndex] = currRate;

					if (currRate > pJitterInfo->prod_PUBREC_MaxRate)
						pJitterInfo->prod_PUBREC_MaxRate = currRate;
					if (currRate < pJitterInfo->prod_PUBREC_MinRate)
						pJitterInfo->prod_PUBREC_MinRate = currRate;

					currRate = (pCurrMsgCounts.prod_Rx_Curr_PUBCOMP - pJitterInfo->prod_Rx_Prev_PUBCOMP) / elapsedTime;
					pJitterInfo->rateArray_Prod_PUBCOMP[currIndex] = currRate;

					if (currRate > pJitterInfo->prod_PUBCOMP_MaxRate)
						pJitterInfo->prod_PUBCOMP_MaxRate = currRate;
					if (currRate < pJitterInfo->prod_PUBCOMP_MinRate)
						pJitterInfo->prod_PUBCOMP_MinRate = currRate;
				}

				/* Store the current results in the previous for the next sample */
				pJitterInfo->prod_Tx_Prev_PUB     = pCurrMsgCounts.prod_Tx_Curr_PUB;
				pJitterInfo->prod_Rx_Prev_PUBACK  = pCurrMsgCounts.prod_Rx_Curr_PUBACK;
				pJitterInfo->prod_Rx_Prev_PUBREC  = pCurrMsgCounts.prod_Rx_Curr_PUBREC;
				pJitterInfo->prod_Tx_Prev_PUBREL  = pCurrMsgCounts.prod_Tx_Curr_PUBREL;
				pJitterInfo->prod_Rx_Prev_PUBCOMP = pCurrMsgCounts.prod_Rx_Curr_PUBCOMP;
			}

			/* Increment the index and handle when it needs to wrap. */
			if (++currIndex >= pJitterInfo->sampleCt) {
				currIndex = 0;
				pJitterInfo->wrapped = 1;
			}

			pJitterInfo->currIndex = currIndex;
		} else {

		}

		/* Store the current time as the previous time for next iteration */
		pJitterInfo->prevTime = currTime;

		/* Reset the jitter lock */
		g_JitterLock = 0;
	} else {
		/* Shutting down so cancel the timer for jitter. */
		rc = ism_common_cancelTimer(jitterKey);
		MBTRACE(MBINFO, 7, "Cancelled the Jitter Timer Task due to requested quit.\n");
		rc = 0;  /* Cancel the timer since the user requested to quit. */
	}

	return rc;
} /* onJitterTimeout */



/* ************************************************************************************ */
/* ************************************************************************************ */
/*                                Add Timer Requests                                    */
/* ************************************************************************************ */
/* ************************************************************************************ */

/* *************************************************************************************
 * addMqttbenchTimerTasks
 *
 * Description:  This task will rename the csv and latency csv files every 24 hrs by
 *               appending a number suffix.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int addMqttbenchTimerTasks (mqttbenchInfo_t *mqttbenchInfo)
{
	int rc = 0;

	mbCmdLineArgs_t  *pCmdLineArgs = mqttbenchInfo->mbCmdLineArgs;
	mbTimerInfo_t *pTimerInfo = mqttbenchInfo->mbTimerInfo;
	environmentSet_t *pSysEnvSet = mqttbenchInfo->mbSysEnvSet;

	/* Create a timer task for scanning for client which are stuck in transport or protocol connect. */
    if(g_numClientsWithConnTimeout > 0) {
		pTimerInfo->clientScanKey = ism_common_setTimerRate(ISM_TIMER_LOW,
														   (ism_attime_t) onTimerClientScan,
														   (void *) mqttbenchInfo,
														   10,
														   5,
														   TS_SECONDS);
		if (pTimerInfo->clientScanKey == NULL) {
			MBTRACE(MBERROR, 1, "Unable to schedule the Client Scan timer.\n");
			rc = RC_TIMER_KEY_NULL;
		}
    }

	/* Create a timer task if performing snapshots (-snap parameter). */
	if (pCmdLineArgs->snapInterval) {
		pTimerInfo->snapTimerKey = ism_common_setTimerRate(ISM_TIMER_LOW,
			                                               (ism_attime_t)onTimerSnapRequests,
				                                           (void *)mqttbenchInfo,
				                                           1,
														   pCmdLineArgs->snapInterval,
				                                           TS_SECONDS);
		if (pTimerInfo->snapTimerKey == NULL) {
			MBTRACE(MBERROR, 1, "Unable to schedule Snap timer.\n");
			rc = RC_TIMER_KEY_NULL;
		}
	}

	/* Create a timer task if performing latency reset (-rl option) */
	if (pCmdLineArgs->resetLatStatsSecs > 0) {
		pTimerInfo->resetLatTimerKey = ism_common_setTimerRate(ISM_TIMER_LOW,
		                                                       (ism_attime_t)onTimerLatencyStatsReset,
			                                                   (void *)pSysEnvSet,
												               pCmdLineArgs->resetLatStatsSecs,
												               pCmdLineArgs->resetLatStatsSecs,
			                                                   TS_SECONDS);
		if (pTimerInfo->resetLatTimerKey == NULL) {
			MBTRACE(MBERROR, 1, "Unable to schedule Latency Stats Reset timer.\n");
			rc = RC_TIMER_KEY_NULL;
		}
	}

	/* Create a timer task for performing memory cleanup (EnvVar MemTrimInterval). */
	if (pSysEnvSet->memTrimSecs > 0) {
		pTimerInfo->memTrimTimerKey = ism_common_setTimerRate(ISM_TIMER_LOW,
                                                              (ism_attime_t)onTimerMemoryCleanup,
                                                              (void *)NULL,
									 			              pSysEnvSet->memTrimSecs,
												              pSysEnvSet->memTrimSecs,
												              TS_SECONDS);
		if (pTimerInfo->memTrimTimerKey == NULL) {
			MBTRACE(MBERROR, 1, "Unable to schedule Memory Cleanup timer.\n");
			rc = RC_TIMER_KEY_NULL;
		}
	}

	return rc;
} /* addMqttbenchTimerTasks */

/* *************************************************************************************
 * addDelayTimerTask
 *
 * Description:  Schedule task to delay reconnection of a client.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures.
 *   @param[in]  trans               = Client's transport.
 *   @param[in]  delayTime           = Delay time prior to rescheduling a connection.
 * *************************************************************************************/
void addDelayTimerTask (mqttbenchInfo_t *mqttbenchInfo, transport_t * trans, uint64_t delayTime)
{
	mbTimerInfo_t *pTimerInfo = mqttbenchInfo->mbTimerInfo;
	mqttclient_t *client = trans->client;

	pTimerInfo->delayTimeoutTimerKey = ism_common_setTimerOnce(ISM_TIMER_HIGH,
															   (ism_attime_t) onTimerDelayTimeout,
															   (void *) trans,
															   delayTime);

	if (pTimerInfo->delayTimeoutTimerKey == NULL) {
		MBTRACE(MBERROR, 1, "Unable to schedule timer task for reconnect of client %s (line=%d).\n",
				            client->clientID, client->line);
	}
} /* addDelayTimerTask */

/* *************************************************************************************
 * addAutoRenameTimerTask
 *
 * Description:  Schedule task to rename the log file.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
void addAutoRenameTimerTask (mqttbenchInfo_t *mqttbenchInfo)
{
	mbTimerInfo_t *pTimerInfo = mqttbenchInfo->mbTimerInfo;

	pTimerInfo->autoRenameTimerKey = ism_common_setTimerRate(ISM_TIMER_LOW,
                                                             (ism_attime_t)onTimerAutoRenameLogFiles,
                                                             (void *)mqttbenchInfo,
											                 SECONDS_IN_24HRS,
											                 SECONDS_IN_24HRS,
                                                             TS_SECONDS);
	if (pTimerInfo->autoRenameTimerKey == NULL)
		MBTRACE(MBERROR, 1, "Unable to schedule timer task for renaming the Logfile.\n");
} /* addAutoRenameTimerTask */

/* *************************************************************************************
 * addBurstTimerTask
 *
 * Description:  Add a timer with the interval requested to schedule a burst in the rate.
 *
 *   @param[in]  pTimerInfo          = The TimerInfo structure.
 *   @param[in]  pBurstInfo          = The BurstInfo structure.
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int addBurstTimerTask (mbTimerInfo_t *pTimerInfo, mqttbenchInfo_t *pMBInfo)
{
	int rc = 0;

	pTimerInfo->burstModeTimerKey = ism_common_setTimerOnce(ISM_TIMER_LOW,
                                                            (ism_attime_t)onTimerBurstMode,
                                                            (void *)pMBInfo,
												            pMBInfo->mbBurstInfo->burstInterval);

	if (pTimerInfo->burstModeTimerKey == NULL) {
		rc = RC_TIMER_KEY_NULL;
		MBTRACE(MBERROR, 1, "Unable to schedule timer task for Burst Mode.\n");
	} else
		MBTRACE(MBINFO, 1, "Burst Mode timer task has been started.\n");

	return rc;
} /* addBurstTimerTask */

/* *************************************************************************************
 * addJitterTimerTask
 *
 * Description:  Schedule the jitter timer task.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int addJitterTimerTask (mqttbenchInfo_t *mqttbenchInfo)
{
	int rc = 0;

	mbJitterInfo_t *pJitterInfo = mqttbenchInfo->mbJitterInfo;
	mbTimerInfo_t *pTimerInfo = mqttbenchInfo->mbTimerInfo;

	uint64_t jitterInterval = pJitterInfo->interval;
	uint8_t QoSBitMask = 0;

	/* --------------------------------------------------------------------------------
	 * This is called to set up the timer task to capture the jitter information.  At
	 * this point the JitterInfo structure has NOT had the QoS's set up for RX and TX.
	 * First get the Consumer QoS in order to set the Jitter RX QoS.  Test the return
	 * value to between 0 and 2, since this is the only valid values.  If NOT, then set
	 * rc = to RC_UNABLE_DETERMINE_QOS.
	 * -------------------------------------------------------------------------------- */
	if (mqttbenchInfo->instanceClientType & CONTAINS_CONSUMERS) {
		QoSBitMask = getAllThreadsQoS (CONSUMER, mqttbenchInfo);
		if ((QoSBitMask & ~MQTT_VALID_QOS) == 0)
			pJitterInfo->rxQoSMask = QoSBitMask;
		else
			rc = RC_UNABLE_DETERMINE_QOS;
	}

	/* --------------------------------------------------------------------------------
	 * Obtain the Producers QoS to set the Jitter TX QoS. Test the return value to
	 * between 0 and 2, since this is the only valid values.  If NOT, then set rc
	 * equal to UNABLE_DETERMINE_QOS.
	 * -------------------------------------------------------------------------------- */
	if (mqttbenchInfo->instanceClientType & CONTAINS_PRODUCERS) {
		QoSBitMask = getAllThreadsQoS (PRODUCER, mqttbenchInfo);
		if ((QoSBitMask & ~MQTT_VALID_QOS) == 0)
			pJitterInfo->txQoSMask = QoSBitMask;
		else
			rc = RC_UNABLE_DETERMINE_QOS;
	}

	if (rc == 0) {
		pTimerInfo->jitterKey = ism_common_setTimerRate(ISM_TIMER_HIGH,
                                                        (ism_attime_t)onTimerJitter,
                                                        (void *)mqttbenchInfo,
	                                                    jitterInterval,
	                                                    jitterInterval,
	                                                    TS_MILLISECONDS);

		if (pTimerInfo->jitterKey == NULL) {
			rc = RC_TIMER_KEY_NULL;
			MBTRACE(MBERROR, 1, "Unable to schedule timer task for Jitter Statistics.\n");
		} else {
			MBTRACE(MBINFO, 1, "Jitter Statistics timer task has been started.\n");
		}
	} else {
		MBTRACE(MBERROR, 1, "Unable to determine QoS for Jitter Statistics.\n");
	}

	return rc;
} /* addJitterTimerTask */

/* *************************************************************************************
 * addLingerTimerTask
 *
 * Description:  Schedule the Linger timer task.
 *
 *   @param[in]  client              = Specific mqttclient to be use.
 *
 * *************************************************************************************/
void addLingerTimerTask (mqttclient *client)
{
	MBTRACE(MBDEBUG, 9 , "Setting linger timer for client %s (line=%d), set to expire in %.2f seconds\n",
						 client->clientID, client->line, client->lingerTime_ns * 1e-9);
	ism_timer_t lingerTimer = ism_common_setTimerOnce(ISM_TIMER_HIGH,
													  (ism_attime_t) onTimerLinger,
													  (void *) client,
													  client->lingerTime_ns);

	if (lingerTimer == NULL) {
			MBTRACE(MBERROR, 1, "Unable to schedule timer task for disconnect of client %s (line=%d).\n",
					            client->clientID, client->line);
	}
} /* addLingerTimerTask */

/* *************************************************************************************
 * addWarningTimerTask
 *
 * Description:  Add a timer to provide a warning when latency is greater than the
 *               histogram.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int addWarningTimerTask (mqttbenchInfo_t *mqttbenchInfo)
{
	int rc = 0;
	mbTimerInfo_t *pTimerInfo = mqttbenchInfo->mbTimerInfo;

	pTimerInfo->warningLatTimerKey = ism_common_setTimerRate(ISM_TIMER_LOW,
			                                                 (ism_attime_t)onTimerLatencyWarning,
												             (void *)mqttbenchInfo,
												             SECONDS_IN_30MINS,
												             SECONDS_IN_30MINS,
												             TS_SECONDS);
	if (pTimerInfo->warningLatTimerKey == NULL) {
		rc = RC_TIMER_KEY_NULL;
		MBTRACE(MBERROR, 1, "Unable to schedule timer task for Warning about Latency.\n");
	} else
		MBTRACE(MBINFO, 1, "Latency Warning timer has been started.\n");

	return rc;
} /* addWarningTimerTask */
