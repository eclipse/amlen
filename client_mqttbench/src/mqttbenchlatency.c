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

#include <string.h>
#include <ctype.h>
#include <math.h>
#include <ismutil.h>

#include "mqttbench.h"
#include "mqttclient.h"

/* ******************************** GLOBAL VARIABLES ********************************** */
/* Externs */

extern int g_MinMsgRcvd;
extern int g_MaxMsgRcvd;
extern int g_Equiv1Sec_Conn;             /* conversion of g_UnitsConn to 1 sec. */
extern int g_Equiv1Sec_RTT;              /* conversion of g_UnitsRTT to 1 sec. */
extern int g_Equiv1Sec_Reconn_RTT;       /* conversion of g_UnitsReconnRTT to 1 sec. */

extern uint32_t g_TotalAttemptedRetries; /* DEV ONLY - declared in tcpclient.c - used for STATS to provide time needed for reconnect. */

extern uint64_t bfAddrInUse;             /* declared in tcpclient.c */
extern uint64_t bfAddrNotAvail;          /* declared in tcpclient.c */
extern uint64_t cfAddrNotAvail;          /* declared in tcpclient.c */

extern uint8_t  g_AlertNoMsgIds;
extern uint8_t  g_ResetHeaders;

extern double  g_UnitsConn;
extern double  g_UnitsRTT;
extern double  g_UnitsReconnRTT;

extern ioProcThread_t **ioProcThreads;   /* ioProcessor threads hold statistics for consumers */

#define APPEND_METRIC(gbuf, mbuf, tbuf, name, value) {																	 \
	snprintf(mbuf, MAX_METRIC_NAME_LEN, "%s.mqttbench.%u.%s", pSysEnvSet->GraphiteMetricRoot, mqttbenchInfo->instanceID, name); 	 \
	snprintf(tbuf, MAX_METRIC_TUPLE_LEN, "%s %s %d\n", mbuf, value, timeSecs); 											 \
	ism_common_allocBufferCopyLen(gbuf, metricTuple, strlen(metricTuple)); 												 \
}


/* Function prototypes for functions unique to this file. */
void addHistogram(uint32_t [], uint32_t [], int);
int  checkLatencyHistograms(int, int, uint8_t);
int  getLatencyPercentile(uint32_t [], double, int);


/* *************************************************************************************
 * provideConnStats
 *
 * Description:  Provide connection statistics
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *   @param[in]  clientType          = Client type (Consumer, Producer or Dual Client)
 *   @param[in]  useWS               = WebSockets are being used or not (1 = true)
 *   @param[in]  endTime4ConnTest    = x
 *   @param[in]  startTime4ConnTest  = x
 *   @param[in]  fpCSVLatFile        = CSV Latency File Descriptor
 *   @param[in]  fpHistogramFiles    = Histogram File Descriptors
 *
 *   @return 0   = Successful completion
 * *************************************************************************************/
int provideConnStats (mqttbenchInfo_t *mqttbenchInfo,
		              uint8_t clientType,
		              double endTime4ConnTest,
		              double startTime4ConnTest,
		              FILE *fpCSVLatFile,
		              FILE **fpHistogramFiles)
{
	int i = 0;
	int rc = 0;
	int ctr = 0;
	int mask = 0x1;

	int latencyMask = mqttbenchInfo->mbCmdLineArgs->latencyMask;

	double connTime = 0.0;

	/* Get the connection times for each type specified (-T <mask>) */
	for ( i = 0, mask = 0x1 ; i < LAT_MASK_SIZE ; mask <<= 1, i++ ) {
		if ((mask & latencyMask) > 0) {
			if ((mask & LAT_CONN_AND_SUBSCRIBE) > 0) {

				/* Get the Connection Times */
		    	rc = getConnectionTimes(mqttbenchInfo, mask, &endTime4ConnTest, clientType);
		    	if (rc == 0) {
		    		calculateLatency(mqttbenchInfo, mask, OUTPUT_CSVFILE, 0, fpCSVLatFile, fpHistogramFiles[ctr]);
		    		if (fpHistogramFiles[ctr++] != NULL)
		    			mqttbenchInfo->histogramCtr++;
		    	} else {
		    		doCleanUp();
		    		exit (rc);
		    	}
			}
		}
	} /* for ( i=0, mask=0x1 */

	if (endTime4ConnTest > 0.0) {
		connTime = endTime4ConnTest - startTime4ConnTest;
		MBTRACE(MBINFO, 1, "Total Connection Test Time:  %f (secs)\n\n", connTime);
		fprintf(stdout, "Total Connection Test Time:  %f (secs)\n\n", connTime);
	} else {
		MBTRACE(MBINFO, 1, "Total Connection Test Time:  0.0    ** NOT all the connections completed.\n\n");
		fprintf(stdout, "Total Connection Test Time:  0.0    ** NOT all the connections completed.\n\n");
	}

	fflush(stdout);

	return rc;
} /* provideConnStats */

/* *************************************************************************************
 * getStats
 *
 * Description:  Get the statistics for the consumer and producer.  getStats() is used
 *               in several different ways:
 *                                                                     Input Param
 *               Description                                            where
 *               ----------------------------------------------------   -----------
 *               1. Provide statistics to the console                   OUTPUT_CONSOLE
 *               2. Provide accumulative statistics to a csv file       OUTPUT_CSVFILE
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *   @param[in]  where               = Location to put data (CONSOLE or CSV File)
 *   @param[in]  startTime           = x
 *   @param[in]  fpCSV               = File to write the stats to.
 *
 *   @return 0   = Successful completion
 * *************************************************************************************/
int getStats (mqttbenchInfo_t *mqttbenchInfo,
		      uint8_t where,
		      double startTime,
		      FILE *fpCSV)
{
	static uint8_t hdrPrinted_Stats = 0;

	environmentSet_t *pSysEnvSet = mqttbenchInfo->mbSysEnvSet;
    mbThreadInfo_t *pThreadInfo = mqttbenchInfo->mbThreadInfo;
    double connRetryFactor = pSysEnvSet->connRetryFactor;

	int rc = 0;
   	int i = 0;
   	int numIOPThrds = (int)pSysEnvSet->numIOProcThreads;
   	int rxNumFreeBfr = 0, rxAllocatedBfr = 0, rxMaxSizeBfr = 0, rxMinSizeBfr = 0;
	int txNumFreeBfr = 0, txAllocatedBfr = 0, txMaxSizeBfr = 0, txMinSizeBfr = 0;
	int tmpTotalNumClients = 0;
	int clientType = 0;
	int numSubmitThrds = mqttbenchInfo->mbCmdLineArgs->numSubmitterThreads;

	/* Following variables are used for counts and rates. */
	int msgType;
	int qosType;

	/* Following variables provide snap information. */
   	uint8_t snapStatTypes = mqttbenchInfo->mbCmdLineArgs->snapStatTypes;
   	int snapInterval = mqttbenchInfo->mbCmdLineArgs->snapInterval;

	/* Following variables are used for connects. */
	int aggRetries = 0, aggConnectMsgCt = 0, aggConnAckMsgCt = 0;

	/* RX data (received messages) */
	uint64_t currRxMsgCounts[MQTT_NUM_MSG_TYPES]	= {0};
	uint64_t prevRxMsgCounts[MQTT_NUM_MSG_TYPES]	= {0};
	uint64_t initialRxMsgCounts[MQTT_NUM_MSG_TYPES] = {0};
	uint64_t currRxQoSPubMsgCounts[MAX_NUM_QOS]     = {0};
	uint64_t prevRxQoSPubMsgCounts[MAX_NUM_QOS]     = {0};
	uint64_t currRxDupMsgCount                      = 0;
	uint64_t currRxRetainMsgCount                   = 0;
	uint64_t currConsConnectCount                   = 0;
	uint64_t currConsConnAckCount                   = 0;

	/* TX data (sent messages) */
	uint64_t currTxMsgCounts[MQTT_NUM_MSG_TYPES]    = {0};
	uint64_t prevTxMsgCounts[MQTT_NUM_MSG_TYPES]    = {0};
	uint64_t initialTxMsgCounts[MQTT_NUM_MSG_TYPES] = {0};
	uint64_t currTxQoSPubMsgCounts[MAX_NUM_QOS]     = {0};
	uint64_t prevTxQoSPubMsgCounts[MAX_NUM_QOS]     = {0};

   	int aggRxMsgRates[MQTT_NUM_MSG_TYPES]  		= {0};	/* aggregate rx rate (all received messages) */
   	int aggTxMsgRates[MQTT_NUM_MSG_TYPES]  		= {0};	/* aggregate tx rate (all sent messages) */
   	int aggRxQoSPubMsgRates[MAX_NUM_QOS]        = {0};  /* aggregate rx QoS publish msg rates. */
   	int aggTxQoSPubMsgRates[MAX_NUM_QOS]        = {0};  /* aggregate tx QoS publish msg rates. */

   	int connStates[NUM_CONNECT_STATES]          = {0};  /* array of connection status. */

   	uint64_t aggRxMsgCounts[MQTT_NUM_MSG_TYPES]     = {0};	/* aggregate rx count (all received messages) */
   	uint64_t prevAggRxMsgCounts[MQTT_NUM_MSG_TYPES] = {0};	/* previous aggregate rx count (all received messages) */
   	uint64_t aggTxMsgCounts[MQTT_NUM_MSG_TYPES]     = {0};	/* aggregate tx count (all sent messages) */
   	uint64_t prevAggTxMsgCounts[MQTT_NUM_MSG_TYPES] = {0};	/* previous aggregate tx count (all sent messages) */
   	uint64_t aggRxQoSPubMsgCounts[MAX_NUM_QOS]      = {0};  /* aggregate rx QoS publish msg counts. */
   	uint64_t prevAggRxQoSPubMsgCounts[MAX_NUM_QOS]  = {0};  /* previous aggregate rx QoS publish msg counts. */
   	uint64_t aggTxQoSPubMsgCounts[MAX_NUM_QOS]      = {0};  /* aggregate tx QoS publish msg counts. */
   	uint64_t prevAggTxQoSPubMsgCounts[MAX_NUM_QOS]  = {0};  /* previous aggregate tx QoS publish msg counts. */
   	uint64_t aggRxDupMsgCount                   = 0;
   	uint64_t aggRxRetainMsgCount                = 0;
   	uint64_t aggConsConnectMsgCount             = 0;    /* aggregate Consumer CONNECT msg count. */
   	uint64_t aggConsConnAckMsgCount             = 0;    /* aggregate Consumer CONNACK msg count. */
   	uint64_t aggBadRxRCCount               		= 0;    /* aggregate IOP Client's Bad return code Rx Count. */
   	uint64_t aggBadTxRCCount               		= 0;    /* aggregate IOP Client's Bad return code Tx Count. */
   	uint64_t aggFailedSubscriptions        		= 0;    /* aggregate IOP Client's Failed Subscription Count. */
   	uint64_t aggOOOCount		                = 0;    /* aggregate IOP Client's Out of order message Count. */
   	uint64_t aggRedeliveredCount                = 0;    /* aggregate IOP Client's Redelivered message Count. */
   	uint64_t totalConnRetryTime                 = 0;    /* Total time needed using RetryDelay to successfully connect. */
   	uint64_t totalDisconnectCount               = 0;    /* Total number of disconnects that have occurred. */
   	uint64_t totalServerMQTTDisconnectCount     = 0;    /* aggregate count of MQTT DISCONNECT messages sent by the server */

   	char snapStatLine[MAX_SNAP_STATLINE] = {0};
   	char snapHdrLine[MAX_HDRLINE] = {0};
   	char snapCurrLine[MAX_SNAP_CURRLINE] = {0};
	char snapTime[MAX_TIME_STRING_LEN];        /* Used for holding current timestamp. */

	const char * snapHdrConnect = "TimeStamp,TC,THIP,TR,TE,TSIP,TD,TCR,CON,CA,PS,PC,PCIP,PDIP,PD,DISC,SRVDISC";
	const char * snapHdrRate = "TxQ0,TxQ1,TxQ2,RxRPA,RxRPRC,RxRPC,RxQ0,RxQ1,RxQ2,TxRPA,TxRPRC,TxRPC";
	const char * snapHdrCount = "TxPUB,TxPRL,RxPA,RxPRC,RxPC,RxSA,RxPUB,RxPRL,TxSUB,TxPA,TxPRC,TxPC,BADSubs,BADRxRC,BADTxRC,OOO,REDEL";

   	/* Variables pertaining to time used for rate calculations. */
	double prevTime;
   	double currTime;
   	double timeDiff = 1;

    mqttbench_t *mbinst = NULL;
    mqttbench_t **mbInstArray = mqttbenchInfo->mbInstArray;

    /* Flags */
	uint8_t commaNeeded = 0;      /* Flag to indicate whether a comma is needed in the stats. */
	uint8_t displayType = 0;

	/* Obtain the current logging time in format of:  HH:MM:SS */
	createLogTimeStamp(snapTime, MAX_TIME_STRING_LEN, LOGFORMAT_YY_MM_DD);

	/* Check if resetHeaders flag is set.  If so then reset the static. */
	if (g_ResetHeaders)
		hdrPrinted_Stats = 0;

	/* Calculate the total connection time needed when using RetryDelay. */
	if ((connRetryFactor > 0) && (g_TotalAttemptedRetries)) {
		uint64_t nextDelayTime = pSysEnvSet->initConnRetryDelayTime_ns;

		for ( i = 0 ; i < g_TotalAttemptedRetries ; i++ ) {
			if (connRetryFactor > 1) {
				nextDelayTime *= connRetryFactor;
			} else {
				nextDelayTime += (nextDelayTime * connRetryFactor);
			}
			totalConnRetryTime += nextDelayTime;
		}
	}

	/* Aggregate some key IOP counters */
	for ( i = 0 ; i < numIOPThrds ; i++ ) {
		ioProcThread_t *iop = ioProcThreads[i];
		aggBadRxRCCount += iop->currBadRxRCCount;
		aggBadTxRCCount += iop->currBadTxRCCount;
		aggOOOCount += iop->currOOOCount;
		aggRedeliveredCount += iop->currRedeliveredCount;
		aggFailedSubscriptions += iop->currFailedSubscriptions;
		totalDisconnectCount += iop->currDisconnectCounts;
		totalServerMQTTDisconnectCount += iop->currRxMsgCounts[DISCONNECT];
	}

	/* --------------------------------------------------------------------------------
	 * If the data is to be displayed to the console then put some stats in regards
	 * to Ports Skipped, InUse, Not Available, Attempted retries...etc
	 * -------------------------------------------------------------------------------- */
   	if (where == OUTPUT_CONSOLE) {
   		/* Display some general health status on top. */
   		fprintf(stdout, "\nSTATS: [Ports skipped - Bind: InUse=%llu NotAvail=%llu] [FailedSubs: %llu] [BadRC: Rx=%llu Tx=%llu] [OOO: %llu] [Redel: %llu]",
		                (ULL)bfAddrInUse,
			            (ULL)bfAddrNotAvail,
						(ULL)aggFailedSubscriptions,
						(ULL)aggBadRxRCCount,
						(ULL)aggBadTxRCCount,
						(ULL)aggOOOCount,
						(ULL)aggRedeliveredCount);

   		if (pSysEnvSet-> portRangeOverlap == 1)
   			fprintf(stdout, "[PortRangeOverlap] ");

   		if (totalConnRetryTime > 0)
   			fprintf(stdout, "[ConnRetry#: %lu (~%3.3f secs)] ",
   					        (unsigned long) g_TotalAttemptedRetries,
   					        (double)(totalConnRetryTime / NANO_PER_SECOND));

   		if (g_AlertNoMsgIds)
   			fprintf(stdout, "NoMsgIds ");

		if (cfAddrNotAvail)
			fprintf(stdout, "\n       [Port conflicts: %llu] ", (ULL)cfAddrNotAvail);

   		fprintf(stdout, "\n-----------------------------------------------------------------------------------------------------------\n");
   		fflush(stdout);
	}

	/* --------------------------------------------------------------------------------
	 * If performing snapshots then need to see if the header has been printed yet.  If
	 * not then create the header line and put in the CSV file.
	 * -------------------------------------------------------------------------------- */
   	if ((where == OUTPUT_CSVFILE) && (hdrPrinted_Stats == 0) && (snapInterval > 0)) {
		if ((snapStatTypes & SNAP_CONNECT) > 0) {
			strcpy(snapHdrLine, snapHdrConnect);
			commaNeeded = 1;
		}

		if ((snapStatTypes & SNAP_RATE) > 0) {
			if (commaNeeded == 1) {
				strcat(snapHdrLine, ",,");
			} else {
				strcat(snapHdrLine, "TimeStamp,");
			}
			strcat(snapHdrLine, snapHdrRate);

			commaNeeded = 1;
		}

		if ((snapStatTypes & SNAP_COUNT) > 0) {
			if (commaNeeded == 1) {
				strcat(snapHdrLine, ",,");
			} else {
				strcat(snapHdrLine, "TimeStamp,");
			}
			strcat(snapHdrLine, snapHdrCount);
		}

   		fprintf(fpCSV, "%s\n", snapHdrLine);
		hdrPrinted_Stats = 1;
   	}

   	commaNeeded = 0;  /* Reset command indicator */
   	i = 0;

	currTime = getCurrTime();  /* Get the current time. */

	/* See if there are any submitter threads. */
	if (mbInstArray) {
		/* ----------------------------------------------------------------------------
		 * Determine what needs to be displayed based on the type of clients associated
		 * with the submitter threads.
		 * ---------------------------------------------------------------------------- */
		if (pThreadInfo->numDualThreads == 0) {
			if ((pThreadInfo->numConsThreads > 0) && (pThreadInfo->numProdThreads == 0)) {
				displayType = DISPLAY_CONSUMER_ONLY;
			} else if ((pThreadInfo->numProdThreads > 0) && (pThreadInfo->numConsThreads == 0)) {
				displayType = DISPLAY_PRODUCER_ONLY;
			} else
				displayType = DISPLAY_BOTH;
		} else
	   		displayType = DISPLAY_BOTH;

		/* ----------------------------------------------------------------------------
		 * Obtain the connection stats if the contents are to be displayed to the
		 * console or to a CSV File while requesting:  connects.
		 * ---------------------------------------------------------------------------- */
	   	if ((where == OUTPUT_CONSOLE) ||
	   		(((snapStatTypes & SNAP_CONNECT) > 0) && (snapInterval > 0))) {
			for ( i = 0 ; i < numSubmitThrds ; i++ ) {
				if (mbInstArray[i]) {
					mbinst = mbInstArray[i];
					clientType = mbinst->clientType;

	   				/* ----------------------------------------------------------------
	   				 * If displaying connection stats to the console then memset to 0
	   				 * for each IOP.  If performing snapshots then need to continue to
	   				 * add the numbers across all IOPs.
	   				 * ---------------------------------------------------------------- */
	   				if (where == OUTPUT_CONSOLE)
	   					memset(connStates, 0, sizeof(int) * NUM_CONNECT_STATES);

	   				rc = countConnections(mbinst, clientType, ALL_CONNECT_STATES, STATE_TYPE_UNIQUE, connStates);

	   				if (where == OUTPUT_CONSOLE) {
	   					fprintf(stdout, "SUB%d: Clients [TC: %d THIP: %d TR: %llu TE: %d TSIP: %d TD: %d TCR: %d " \
   				                        "PS: %d PC: %d PCIP: %d PDIP: %d PD: %d]\n",
			                            i,
                                        connStates[CONN_SOCK_CONN],
                                        connStates[CONN_SOCK_HSIP],
                                        (ULL)mbinst->connRetries,
                                        connStates[CONN_SOCK_ERR],
                                        connStates[CONN_SOCK_SDIP],
                                        connStates[CONN_SOCK_DISC],
                                        connStates[CONN_SOCK_CREATE],
                                        connStates[CONN_MQTT_PS],
                                        connStates[CONN_MQTT_CONN],
                                        connStates[CONN_MQTT_CIP],
                                        connStates[CONN_MQTT_DIP],
                                        connStates[CONN_MQTT_DISC]);
	   					fflush(stdout);
	   				} else {
	   					aggRetries += mbinst->connRetries;
	   					aggConnectMsgCt += mbinst->currConnectMsgCount;
	   					aggConnAckMsgCt += mbinst->currConnAckMsgCount;
	   				} /* if (where == OUTPUT_CONSOLE) { */
				}
	   		}  /* for ( i = 0 ; i < numSubmitThrds ; i++ ) */

			if (where == OUTPUT_CONSOLE) {
				fprintf(stdout, "-----------------------------------------------------------------------------------------------------------\n");
				fflush(stdout);
			}
	   	} else {
	   		for ( i = 0 ; i < numSubmitThrds ; i++ ) {
	   			mbinst = mbInstArray[i];
   				tmpTotalNumClients += ism_common_list_size(mbinst->clientTuples);
   			}
	   	} /* if ((where == OUTPUT_CONSOLE) || ((snapStatTypes & SNAP_CONNECT) > 0)) */

		/* Check to see if doing snapshots and if so then check if collecting Connection stats. */
		if (where == OUTPUT_CSVFILE) {

			/* If snap stat types has CONNECTS set then update the snapStatLine. */
			if ((snapStatTypes & SNAP_CONNECT) > 0) {
				snprintf(snapStatLine, MAX_SNAP_STATLINE,
						               "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%llu,%llu",
						               snapTime,
						               connStates[CONN_SOCK_CONN],
						               connStates[CONN_SOCK_HSIP],
	                                   aggRetries,
	                                   connStates[CONN_SOCK_ERR],
	                                   connStates[CONN_SOCK_SDIP],
	                                   connStates[CONN_SOCK_DISC],
	                                   connStates[CONN_SOCK_CREATE],
	                                   aggConnectMsgCt,
	                                   aggConnAckMsgCt,
	                                   connStates[CONN_MQTT_PS],
	                                   connStates[CONN_MQTT_CONN],
	                                   connStates[CONN_MQTT_CIP],
	                                   connStates[CONN_MQTT_DIP],
	                                   connStates[CONN_MQTT_DISC],
	                                   (ULL) totalDisconnectCount,
									   (ULL) totalServerMQTTDisconnectCount);

				/* If the snap stat types = CONNECTS then print this data to the file. */
				if (snapStatTypes == SNAP_CONNECT) {
					fprintf(fpCSV, "%s\n", snapStatLine);
				} else {
					commaNeeded = 1;
				}
			} /* if (where == OUTPUT_CONSOLE) */
		} /* if (where == OUTPUT_CONSOLE) */
	} /* if (mbInstArray) */

	/* --------------------------------------------------------------------------------
	 * Obtain the msg counts and msg stats if the contents are to be displayed to the
	 * console or to a CSV File while requesting:   msgcounts and/or msgrates.
	 * -------------------------------------------------------------------------------- */
	if ((where == OUTPUT_CONSOLE) ||
		((snapStatTypes & SNAP_COUNT) || (snapStatTypes & SNAP_RATE)) ||
		((where == OUTPUT_CSVFILE) && (snapInterval == 0))) {

   		for ( i = 0 ; i < numIOPThrds ; i++ ) {
   			ioProcThread_t *iop = ioProcThreads[i];

   			/* Clear all the RX data (received messages) */
   			memset(currRxMsgCounts, 0, (sizeof(uint64_t) * MQTT_NUM_MSG_TYPES));
   			memset(prevRxMsgCounts, 0, (sizeof(uint64_t) * MQTT_NUM_MSG_TYPES));
   			memset(initialRxMsgCounts, 0, (sizeof(uint64_t) * MQTT_NUM_MSG_TYPES));
   			memset(currRxQoSPubMsgCounts, 0, (sizeof(uint64_t) * MAX_NUM_QOS));
   			memset(prevRxQoSPubMsgCounts, 0, (sizeof(uint64_t) * MAX_NUM_QOS));
   			currRxDupMsgCount = 0;
   			currRxRetainMsgCount = 0;
   			currConsConnectCount = 0;
   			currConsConnAckCount = 0;

   			/* Clear all the TX data (sent messages) */
   			memset(currTxMsgCounts, 0, (sizeof(uint64_t) * MQTT_NUM_MSG_TYPES));
   			memset(prevTxMsgCounts, 0, (sizeof(uint64_t) * MQTT_NUM_MSG_TYPES));
   			memset(initialTxMsgCounts, 0, (sizeof(uint64_t) * MQTT_NUM_MSG_TYPES));
   			memset(currTxQoSPubMsgCounts, 0, (sizeof(uint64_t) * MAX_NUM_QOS));
   			memset(prevTxQoSPubMsgCounts, 0, (sizeof(uint64_t) * MAX_NUM_QOS));

   			/* Retrieve the RX Duplicate Msg Count. */
   			currRxDupMsgCount = iop->currRxDupCount;
   			aggRxDupMsgCount += currRxDupMsgCount;

   			currRxRetainMsgCount = iop->currRxRetainCount;
   			aggRxRetainMsgCount += currRxRetainMsgCount;

   			/* Retrieve QoS Publish statistical data from I/O Processor Threads */
   			for ( qosType = 0 ; qosType < MAX_NUM_QOS ; qosType++ ) {
   				currRxQoSPubMsgCounts[qosType] = iop->currRxQoSPublishCounts[qosType];
   				prevRxQoSPubMsgCounts[qosType] = iop->prevRxQoSPublishCounts[qosType];
   				currTxQoSPubMsgCounts[qosType] = iop->currTxQoSPublishCounts[qosType];
   				prevTxQoSPubMsgCounts[qosType] = iop->prevTxQoSPublishCounts[qosType];

   				if (startTime == 0) {  /* for command line statistics (i.e. from the doCommand loop) */
	   				iop->prevRxQoSPublishCounts[qosType] = currRxQoSPubMsgCounts[qosType];
   					iop->prevTxQoSPublishCounts[qosType] = currTxQoSPubMsgCounts[qosType];
   				}

   				aggRxQoSPubMsgCounts[qosType]     += currRxQoSPubMsgCounts[qosType];
   				prevAggRxQoSPubMsgCounts[qosType] += prevRxQoSPubMsgCounts[qosType];
   				aggTxQoSPubMsgCounts[qosType]     += currTxQoSPubMsgCounts[qosType];
   				prevAggTxQoSPubMsgCounts[qosType] += prevTxQoSPubMsgCounts[qosType];
   			} /* for ( qosType = 0 ; qosType < MAX_NUM_QOS ; qosType++ ) */

   			/* Get the Consumer CONNECT and CONNACK message counts */
   			currConsConnectCount = iop->currConsConnectCounts;
   			currConsConnAckCount = iop->currConsConnAckCounts;

   			/* Retrieve message type statistical data from I/O Processor threads */
   			for ( msgType = 0 ; msgType < MQTT_NUM_MSG_TYPES ; msgType++ ) {
   				currRxMsgCounts[msgType] = iop->currRxMsgCounts[msgType];
   				prevRxMsgCounts[msgType] = iop->prevRxMsgCounts[msgType];
   				currTxMsgCounts[msgType] = iop->currTxMsgCounts[msgType];
   				prevTxMsgCounts[msgType] = iop->prevTxMsgCounts[msgType];

   				if (startTime == 0) {  /* for command line statistics (i.e. from the doCommand loop) */

   					iop->prevRxMsgCounts[msgType] = currRxMsgCounts[msgType];
   					iop->prevTxMsgCounts[msgType] = currTxMsgCounts[msgType];
   				}

   				aggRxMsgCounts[msgType]     += currRxMsgCounts[msgType];
   				prevAggRxMsgCounts[msgType] += prevRxMsgCounts[msgType];
   				aggTxMsgCounts[msgType]     += currTxMsgCounts[msgType];
   				prevAggTxMsgCounts[msgType] += prevTxMsgCounts[msgType];

   				if (msgType == CONNECT)
   					aggConsConnectMsgCount += currConsConnectCount;
   				else if (msgType == CONNACK)
   					aggConsConnAckMsgCount += currConsConnAckCount;
   			} /* for ( msgType = 0 ; msgType < MQTT_NUM_MSG_TYPES ; msgType++ ) */

   			prevTime = iop->prevtime;  /* Retrieve the previous time for the IOP and update to currTime. */
   			iop->prevtime = currTime;  /* the time is the same in all IOP threads */

   			/* Determine the time difference from previous or when started. */
   			if (startTime == 0)
   				timeDiff = currTime - prevTime;  /* Calculate the time difference. */
   			else
   				timeDiff = currTime - startTime;

   			/* ------------------------------------------------------------------------
   			 * If where == CONSOLE then the IOP Buffers and Jobs information will be
   			 * provided.
   			 * ------------------------------------------------------------------------ */
   			if (where == OUTPUT_CONSOLE) {
   				rxNumFreeBfr = 0, rxAllocatedBfr = 0, rxMaxSizeBfr = 0, rxMinSizeBfr = 0;
   				txNumFreeBfr = 0, txAllocatedBfr = 0, txMaxSizeBfr = 0, txMinSizeBfr = 0;

   				if (iop->recvBuffPool)
   					ism_common_getBufferPoolInfo(iop->recvBuffPool, &rxMinSizeBfr, &rxMaxSizeBfr, &rxAllocatedBfr, &rxNumFreeBfr);
   				if (iop->sendBuffPool)
   					ism_common_getBufferPoolInfo(iop->sendBuffPool, &txMinSizeBfr, &txMaxSizeBfr, &txAllocatedBfr, &txNumFreeBfr);

   				if ((iop->recvBuffPool) && (iop->sendBuffPool)) {
   					fprintf(stdout, "IOP%d: [Jobs: %d] Bufs [RX InUse: %d Alloc: %d Max: %d Min: %d] [TX InUse: %d Alloc: %d Max: %d Min: %d]\n",
		   				            i,
									iop->currentJobs->used,
		   		                    (rxAllocatedBfr - rxNumFreeBfr),
		   		                    rxAllocatedBfr,
		   		                    rxMaxSizeBfr,
		   		                    rxMinSizeBfr,
		                            (txAllocatedBfr - txNumFreeBfr),
		                            txAllocatedBfr,
		                            txMaxSizeBfr,
		                            txMinSizeBfr);
   				} else if ((iop->recvBuffPool) && (iop->sendBuffPool == NULL)) {
   					fprintf(stdout, "IOP%d: [Jobs: %d] Bufs [RX InUse: %d Alloc: %d Max: %d Min: %d]\n",
		   				            i,
									iop->currentJobs->used,
		   		                    (rxAllocatedBfr - rxNumFreeBfr),
		   		                    rxAllocatedBfr,
		   		                    rxMaxSizeBfr,
		   		                    rxMinSizeBfr);
   				} else {
   					fprintf(stdout, "IOP%d: [Jobs: %d] Bufs [TX InUse: %d Alloc: %d Max: %d Min: %d]\n",
		   				            i,
									iop->currentJobs->used,
		                            (txAllocatedBfr - txNumFreeBfr),
		                            txAllocatedBfr,
		                            txMaxSizeBfr,
		                            txMinSizeBfr);
   				}
   			}
   		} /* end for loop of I/O processor threads*/
	} /* if ((where == OUTPUT_CONSOLE) || ((snapStatTypes & SNAP_COUNT) || (snapStatTypes & SNAP_RATE))) */

	/* Calculate rates from aggregate IOP counters, which is more accurate than adding rates per IOP thread */
	for ( qosType = 0 ; qosType < MAX_NUM_QOS ; qosType++ ) {
		aggRxQoSPubMsgRates[qosType] = (int)((double) (aggRxQoSPubMsgCounts[qosType] - prevAggRxQoSPubMsgCounts[qosType]) / timeDiff);
		aggTxQoSPubMsgRates[qosType] = (int)((double) (aggTxQoSPubMsgCounts[qosType] - prevAggTxQoSPubMsgCounts[qosType]) / timeDiff);
	}

	for ( msgType = 0 ; msgType < MQTT_NUM_MSG_TYPES ; msgType++ ) {
		aggRxMsgRates[msgType] = (int)((double) (aggRxMsgCounts[msgType] - prevAggRxMsgCounts[msgType]) / timeDiff);
		aggTxMsgRates[msgType] = (int)((double) (aggTxMsgCounts[msgType] - prevAggTxMsgCounts[msgType]) / timeDiff);
	}

	/* --------------------------------------------------------------------------------
   	 * QoS 0: PUB (->)
   	 * QoS 1: PUB (->)  PA (<-)
   	 * QoS 2: PUB (->) PRC (<-) PRL (->)  PC (<-)
   	 * -------------------------------------------------------------------------------- */

	/* If the output is to go to console then display here. */
   	if (where == OUTPUT_CONSOLE) {
   		fprintf(stdout, "-----------------------------------------------------------------------------------------------------------\n");
   		fprintf(stdout, "Totals: [CON: %llu CA: %llu SRVDISC: %llu]\n",
   						(ULL)aggConsConnectMsgCount, (ULL)aggConsConnAckMsgCount, (ULL)totalServerMQTTDisconnectCount);

		if ((displayType == DISPLAY_CONSUMER_ONLY) || (displayType == DISPLAY_BOTH)) {
			fprintf(stdout, "   Cons RX: MsgRate(msgs/sec) [Q0: %d   Q1: %d  Q2: %d] Last [SA: %llu PUB: %llu (DUP: %llu RET: %llu) PRL: %llu]\n",
	                        aggRxQoSPubMsgRates[0], aggRxQoSPubMsgRates[1], aggRxQoSPubMsgRates[2],
                            (ULL) aggRxMsgCounts[SUBACK], (ULL) aggRxMsgCounts[PUBLISH],
                            (ULL) aggRxDupMsgCount, (ULL) aggRxRetainMsgCount, (ULL) aggRxMsgCounts[PUBREL]);
			fprintf(stdout, "        TX: QoSRate(msgs/sec) [PA: %d  PRC: %d  PC: %d] Last [SUB: %llu  PA: %llu  PRC: %llu   PC: %llu]\n",
			                aggTxMsgRates[PUBACK], aggTxMsgRates[PUBREC], aggTxMsgRates[PUBCOMP],
                            (ULL) aggTxMsgCounts[SUBSCRIBE], (ULL) aggTxMsgCounts[PUBACK],
                            (ULL) aggTxMsgCounts[PUBREC],  (ULL) aggTxMsgCounts[PUBCOMP]);
		}

		if ((displayType == DISPLAY_PRODUCER_ONLY) || (displayType == DISPLAY_BOTH)) {
			fprintf(stdout, "   Prod TX: MsgRate(msgs/sec) [Q0: %d   Q1: %d  Q2: %d] Last [PUB: %llu PRL: %llu]\n",
                            aggTxQoSPubMsgRates[0], aggTxQoSPubMsgRates[1], aggTxQoSPubMsgRates[2],
                            (ULL) aggTxMsgCounts[PUBLISH], (ULL) aggTxMsgCounts[PUBREL]);
			fprintf(stdout, "        RX: QoSRate(msgs/sec) [PA: %d  PRC: %d  PC: %d] Last [PA: %llu PRC: %llu  PC: %llu]\n",
                            aggRxMsgRates[PUBACK], aggRxMsgRates[PUBREC], aggRxMsgRates[PUBCOMP],
                            (ULL) aggRxMsgCounts[PUBACK],  (ULL) aggRxMsgCounts[PUBREC],
                            (ULL) aggRxMsgCounts[PUBCOMP]);
		}

		fflush(stdout);
   	} else {  /* Put data in CSV file. */
   		if (snapInterval > 0) {
   			if ((snapStatTypes & SNAP_RATE) > 0) {

   				if (commaNeeded == 1)
   					strcat(snapStatLine, ",,");
   				else {
   					snprintf(snapStatLine, MAX_SNAP_STATLINE, "%s,", snapTime);
   					commaNeeded = 1;
   				}

   				snprintf(snapCurrLine, MAX_SNAP_CURRLINE,
   						               "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
			                           aggTxQoSPubMsgRates[0], aggTxQoSPubMsgRates[1], aggTxQoSPubMsgRates[2],
                                       aggRxMsgRates[PUBACK], aggRxMsgRates[PUBREC], aggRxMsgRates[PUBCOMP],
                                       aggRxQoSPubMsgRates[0], aggRxQoSPubMsgRates[1], aggRxQoSPubMsgRates[2],
                                       aggTxMsgRates[PUBACK], aggTxMsgRates[PUBREC], aggTxMsgRates[PUBCOMP]);

   				strcat(snapStatLine, snapCurrLine);
   			}

   			if ((snapStatTypes & SNAP_COUNT) == 0) {
   				fprintf(fpCSV, "%s\n", snapStatLine);
   			} else {
   				if (commaNeeded == 1)
   					strcat(snapStatLine,",,");
   				else {
   					snprintf(snapStatLine, MAX_SNAP_STATLINE, "%s,", snapTime);
   					commaNeeded = 1;
   				}

   				snprintf(snapCurrLine, MAX_SNAP_CURRLINE,
   						               "%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu",
                                       (ULL) aggTxMsgCounts[PUBLISH], (ULL) aggTxMsgCounts[PUBREL],
                                       (ULL) aggRxMsgCounts[PUBACK], (ULL) aggRxMsgCounts[PUBREC], (ULL) aggRxMsgCounts[PUBCOMP],
                                       (ULL) aggRxMsgCounts[SUBACK], (ULL) aggRxMsgCounts[PUBLISH], (ULL) aggRxMsgCounts[PUBREL],
                                       (ULL) aggTxMsgCounts[SUBSCRIBE], (ULL) aggTxMsgCounts[PUBACK],
                                       (ULL) aggTxMsgCounts[PUBREC], (ULL) aggTxMsgCounts[PUBCOMP], (ULL) aggFailedSubscriptions,
									   (ULL) aggBadRxRCCount, (ULL) aggBadTxRCCount, (ULL) aggOOOCount, (ULL) aggRedeliveredCount);

   				strcat(snapStatLine, snapCurrLine);
   				fprintf(fpCSV, "%s\n", snapStatLine);
   			}
   		} else {
   			fprintf(fpCSV, "# Clients,RX Msg Rate (msgs/sec),TX Msg Rate (msgs/sec)\n");
   			fprintf(fpCSV, "%d,%d,%d\n", tmpTotalNumClients, aggRxMsgRates[PUBLISH], aggTxMsgRates[PUBLISH]);
   		} /* if (snapInterval > 0) */
   	} /* if (where == OUTPUT_CONSOLE) */

   	if (where == OUTPUT_CSVFILE)
   		fflush(fpCSV);

   	/* Send metrics to Graphite */
   	if(pSysEnvSet->GraphiteIP && where == OUTPUT_CSVFILE && snapInterval > 0) {
   		char xbuf[64 * 1024];
   		concat_alloc_t graphiteBuffer = {xbuf, sizeof xbuf};
   		char value[MAX_METRIC_VALUE_LEN] = {0};
   		char metricPath[MAX_METRIC_NAME_LEN] = {0};
   		char metricTuple[MAX_METRIC_TUPLE_LEN] = {0};
   		int timeSecs = time(NULL);

   		/* Collect CONNECT stats*/
   		if ((snapStatTypes & SNAP_CONNECT) > 0) {
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_SOCK_CONN]);
   			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TC", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_SOCK_HSIP]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "THIP", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggRetries);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TR", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_SOCK_ERR]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TE", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_SOCK_SDIP]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TSIP", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_SOCK_DISC]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TD", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_SOCK_CREATE]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TCR", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggConnectMsgCt);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "CON", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggConnAckMsgCt);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "CA", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_MQTT_PS]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "PS", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_MQTT_CONN]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "PC", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_MQTT_CIP]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "PCIP", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_MQTT_DIP]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "PDIP", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", connStates[CONN_MQTT_DISC]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "PD", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) totalDisconnectCount);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "DISC", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) totalServerMQTTDisconnectCount);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "SRVDISC", value);
   		}

   		/* Collect MESSAGE RATE stats */
   		if ((snapStatTypes & SNAP_RATE) > 0) {
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggTxQoSPubMsgRates[0]);
   			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxQ0", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggTxQoSPubMsgRates[1]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxQ1", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggTxQoSPubMsgRates[2]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxQ2", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggRxMsgRates[PUBACK]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxRPA", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggRxMsgRates[PUBREC]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxRPRC", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggRxMsgRates[PUBCOMP]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxRPC", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggRxQoSPubMsgRates[0]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxQ0", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggRxQoSPubMsgRates[1]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxQ1", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggRxQoSPubMsgRates[2]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxQ2", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggTxMsgRates[PUBACK]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxRPA", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggTxMsgRates[PUBREC]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxRPRC", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggTxMsgRates[PUBCOMP]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxRPC", value);
   		}

   		/* Collect MESSAGE COUNT stats */
   		if ((snapStatTypes & SNAP_COUNT) > 0) {
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggTxMsgCounts[PUBLISH]);
   			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxPUB", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggTxMsgCounts[PUBREL]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxPRL", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggRxMsgCounts[PUBACK]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxPA", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggRxMsgCounts[PUBREC]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxPRC", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggRxMsgCounts[PUBCOMP]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxPC", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggRxMsgCounts[SUBACK]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxSA", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggRxMsgCounts[PUBLISH]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxPUB", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggRxMsgCounts[PUBREL]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "RxPRL", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggTxMsgCounts[SUBSCRIBE]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxSUB", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggTxMsgCounts[PUBACK]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxPA", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggTxMsgCounts[PUBREC]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxPRC", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggTxMsgCounts[PUBCOMP]);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "TxPC", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggBadRxRCCount);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "BADSubs", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggFailedSubscriptions);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "BADRXRC", value);
			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggBadTxRCCount);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "BADTXRC", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggOOOCount);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "OOO", value);
   			snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggRedeliveredCount);
			APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "REDEL", value);
   		}


   		if(graphiteBuffer.used > 0) {
			rc = basicConnectAndSend(pSysEnvSet->GraphiteIP, pSysEnvSet->GraphitePort, graphiteBuffer.buf, graphiteBuffer.used);
			if(rc) {
				MBTRACE(MBERROR, 1, "Failed to send mqttbench metrics to Graphite server at %s:%d\n",
									pSysEnvSet->GraphiteIP, pSysEnvSet->GraphitePort);
			}
   		}
   	}

   	return rc;
} /* getStats */

/* *************************************************************************************
 * getCurrCounts
 *
 * Description:  Get the current IOP Message counts for TX and RX.
 *
 *   @param[in]  numIOPThrds         = Number of IOP Threads
 *   @param[in]  pCurrMsgCounts      = Message Counters to be used to collect the data.
 * *************************************************************************************/
void getCurrCounts (int numIOPThrds, currMessageCounts_t *pCurrMsgCounts)
{
   	int i;
   	int msgType = 0;

   	uint64_t currRxMsgCounts[8] = {0};
   	uint64_t currTxMsgCounts[8] = {0};

   	for ( i = 0 ; i < numIOPThrds ; i++ ) {
   		ioProcThread_t *iop = ioProcThreads[i];

   		/* Retrieve statistical data from I/O Processor threads */
   		for ( msgType = PUBLISH ; msgType < SUBSCRIBE ; msgType++ ) {
   			currRxMsgCounts[msgType] += iop->currRxMsgCounts[msgType];
   			currTxMsgCounts[msgType] += iop->currTxMsgCounts[msgType];
   		}
   	} /* end for loop of I/O processor threads*/

   	pCurrMsgCounts->cons_Rx_Curr_PUB = currRxMsgCounts[PUBLISH];
   	pCurrMsgCounts->cons_Tx_Curr_PUBACK = currTxMsgCounts[PUBACK];
   	pCurrMsgCounts->cons_Tx_Curr_PUBREC = currTxMsgCounts[PUBREC];
   	pCurrMsgCounts->cons_Rx_Curr_PUBREL = currRxMsgCounts[PUBREL];
   	pCurrMsgCounts->cons_Tx_Curr_PUBCOMP = currTxMsgCounts[PUBCOMP];
   	pCurrMsgCounts->prod_Tx_Curr_PUB = currTxMsgCounts[PUBLISH];
   	pCurrMsgCounts->prod_Rx_Curr_PUBACK = currRxMsgCounts[PUBACK];
   	pCurrMsgCounts->prod_Rx_Curr_PUBREC = currRxMsgCounts[PUBREC];
   	pCurrMsgCounts->prod_Tx_Curr_PUBREL = currTxMsgCounts[PUBREL];
   	pCurrMsgCounts->prod_Rx_Curr_PUBCOMP = currRxMsgCounts[PUBCOMP];
} /* getCurrCounts */

/* *************************************************************************************
 * getConnectionTimes
 *
 * Description:  Total up the TCP/SSL or MQTT Connection times for the consumer(s)
 *               and/or producer(s) and display.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *   @param[in]  mask                = x
 *   @param[in]  endTime             = x
 *   @param[in]  clientType          = Client type (Consumer, Producer or Dual Client)
 *
 *   @return 0   = Successful completion
 * *************************************************************************************/
int getConnectionTimes (mqttbenchInfo_t *mqttbenchInfo,
                        int mask,
						double *endTime,
		                uint8_t clientType)
{
	int i;
	int rc = 0;
	int numConns = 0;
	int useSSL = (int)mqttbenchInfo->useSecureConnections;
	int numSubmitThrds = mqttbenchInfo->mbCmdLineArgs->numSubmitterThreads;
	int useWS = mqttbenchInfo->useWebSockets;

	double connTime;

	mqttbench_t **mbInstArray = mqttbenchInfo->mbInstArray;

	if ((clientType == CONSUMER) ||
		(clientType == PRODUCER) ||
		(clientType == DUAL_CLIENT)) {
    } else {
       	MBTRACE(MBERROR, 1, "Unknown client type: %d\n", clientType);
       	return -1;
    }

	switch (mask) {
		case CHKTIMETCPCONN:
			if (clientType == CONSUMER)
				fprintf(stdout, "\nConsumer TCP%s%s Connection Times:\n",
					            (useSSL == 1 ? "/SSL" : ""),
					            (useWS == 1 ? "/WebSocket" : ""));
			else if (clientType == PRODUCER)
				fprintf(stdout, "\nProducer TCP%s%s Connection Times:\n",
					            (useSSL == 1 ? "/SSL" : ""),
					            (useWS == 1 ? "/WebSocket" : ""));
			else
				fprintf(stdout, "\nClient TCP%s%s Connection Times:\n",
					            (useSSL == 1 ? "/SSL" : ""),
					            (useWS == 1 ? "/WebSocket" : ""));
			break;
		case CHKTIMEMQTTCONN:
			if (clientType == CONSUMER)
				fprintf(stdout, "\nConsumer MQTT Connection Times:\n");
			else if (clientType == PRODUCER)
				fprintf(stdout, "\nProducer MQTT Connection Times:\n");
			else
				fprintf(stderr, "\nClient MQTT Connection Times:\n");
			break;
		case CHKTIMETCP2MQTTCONN:
			if (clientType == CONSUMER)
				fprintf(stdout, "\nConsumer TCP%s%s to MQTT Connection Times:\n",
					            (useSSL == 1 ? "/SSL" : ""),
					            (useWS == 1 ? "/WebSocket" : ""));
			else if (clientType == PRODUCER)
				fprintf(stdout, "\nProducer TCP%s%s to MQTT Connection Times:\n",
					            (useSSL == 1 ? "/SSL" : ""),
					            (useWS == 1 ? "/WebSocket" : ""));
			else
				fprintf(stdout, "\nClient TCP%s%s to MQTT Connection Times:\n",
					            (useSSL == 1 ? "/SSL" : ""),
					            (useWS == 1 ? "/WebSocket" : ""));
			break;
		case CHKTIMETCP2SUBSCRIBE:
			if (clientType == CONSUMER)
				fprintf(stdout, "\nConsumer TCP%s%s to Subscribe Times:\n",
					            (useSSL == 1 ? "/SSL" : ""),
					            (useWS == 1 ? "/WebSocket" : ""));
			else if (clientType == PRODUCER)
				fprintf(stdout, "\nProducer TCP%s%s to Subscribe Times:\n",
					            (useSSL == 1 ? "/SSL" : ""),
					            (useWS == 1 ? "/WebSocket" : ""));
			else
				fprintf(stdout, "\nClient TCP%s%s to Subscribe Times:\n",
					            (useSSL == 1 ? "/SSL" : ""),
					            (useWS == 1 ? "/WebSocket" : ""));
			break;
		default:
			if (clientType == CONSUMER)
				fprintf(stdout, "\nConsumer Subscription Times:\n");
			else if (clientType == PRODUCER)
				fprintf(stdout, "\nProducer Subscription Times:\n");
			else
				fprintf(stdout, "\nClient Subscription Times:\n");
			break;
	} /* switch (mask) */

	printf("\n  Thread\tCPT\t#Conns\tTime(secs)\n");

	if (mbInstArray) {
		/* Go through each thread and display the results. */
		for ( i = 0 ; i < numSubmitThrds ; i++ ) {
			if (mbInstArray[i]) {
				/* Determine how many clients have an established connection. */
				if ((mask & CHKTIMETCPCONN) > 0)
					numConns = countConnections(mbInstArray[i], clientType, SOCK_CONNECTED, STATE_TYPE_TCP, NULL);
				else if ((mask & (CHKTIMEMQTTCONN | CHKTIMETCP2MQTTCONN)) > 0)
					numConns = countConnections(mbInstArray[i],
    					                        clientType,
    					                        (MQTT_PUBSUB | MQTT_CONNECTED),
												STATE_TYPE_MQTT,
    					                        NULL);
				else
					numConns = countConnections(mbInstArray[i], clientType, MQTT_PUBSUB, STATE_TYPE_MQTT, NULL);

				if ((mask & CHKTIMETCPCONN) > 0) {
					connTime = mbInstArray[i]->tcpConnTime;
					if (connTime > 0) {
						if ((connTime + mbInstArray[i]->startTCPConnTime) > *endTime)
							*endTime = connTime + mbInstArray[i]->startTCPConnTime;
					}
				}
				else if ((mask & (CHKTIMEMQTTCONN | CHKTIMETCP2MQTTCONN)) > 0) {
					connTime = mbInstArray[i]->mqttConnTime;
					if (connTime > 0) {
						if ((connTime + mbInstArray[i]->startMQTTConnTime) > *endTime)
							*endTime = connTime + mbInstArray[i]->startMQTTConnTime;
					}
				} else {
					connTime = mbInstArray[i]->subscribeTime;
					if (connTime > 0) {
						if ((connTime + mbInstArray[i]->startSubscribeTime) > *endTime)
							*endTime = connTime + mbInstArray[i]->startSubscribeTime;
					}
				}

				fprintf(stdout, "  %s %d:\t%d\t%d\t%f\n",
       		                    "Sub",
       		                    i,
       		                    abs(mbInstArray[i]->numClients),
       		                    numConns,
       		                    connTime);
			}
		}

		printf("\n");
		fflush(stdout);
	}

    return rc;
} /* getConnectionTimes */

/* *************************************************************************************
 * checkLatencyHistograms
 *
 * Description:  Check to see that there are the required histograms for each of the
 *               types of latency that are being performed.
 *
 *
 *   @param[in]  latencyBitSet       = The particular bit in the latencyMask that is
 *                                     set.  This will then test for that histogram
 *   @param[in]  numIOPThrds         = The number of IOP Threads in mqttbench.
 * *************************************************************************************/
int checkLatencyHistograms (int latencyBitSet, int numIOPThrds, uint8_t mqttClientType)
{
	int rc = 0;
	int i;

	volatile ioProcThread_t *iop = NULL;

	for ( i = 0 ; (i < numIOPThrds) && (rc == 0) ; i++ ) {
		iop = ioProcThreads[i];
		if (iop == NULL) {
			rc = RC_IOP_NOT_FOUND;
			break;
		}

		switch (latencyBitSet) {
			case CHKTIMETCPCONN:
				if (iop->tcpConnHist == NULL) {
					MBTRACE(MBERROR, 1, "No Histogram found for TCP Connection Latency.\n");
					rc = RC_HISTOGRAM_NOT_FOUND;
				}
				break;
			case CHKTIMEMQTTCONN:
			case CHKTIMETCP2MQTTCONN:
				if (iop->mqttConnHist == NULL) {
					if (latencyBitSet == CHKTIMEMQTTCONN) {
						MBTRACE(MBERROR, 1, "No Histogram found for MQTT Connection Latency.\n");
					} else {
						MBTRACE(MBERROR, 1, "No Histogram found for TCP - MQTT Connection Latency.\n");
					}

					rc = RC_HISTOGRAM_NOT_FOUND;
				}
				break;
			case CHKTIMESUBSCRIBE:
			case CHKTIMETCP2SUBSCRIBE:
				if (iop->subscribeHist == NULL) {
					if ((mqttClientType & CONTAINS_CONSUMERS) > 0) {
						if (latencyBitSet == CHKTIMESUBSCRIBE) {
							MBTRACE(MBERROR, 1, "No Histogram found for Subscription Latency.\n");
						} else {
							MBTRACE(MBERROR, 1, "No Histogram found for TCP - Subscription Latency.\n");
						}

						rc = RC_HISTOGRAM_NOT_FOUND;
					} else {
						MBTRACE(MBERROR, 1, "No Subscribers for Subscription Latency.\n");
						rc = RC_SUBSCRIBE_LATENCY_NO_SUBS;
					}
				}
				break;
			case CHKTIMERTT:
				if (iop->rttHist == NULL) {
					if ((mqttClientType & CONTAINS_CONSUMERS) > 0) {
						MBTRACE(MBERROR, 1, "No Histogram found for Round Trip Latency.\n");
						rc = RC_HISTOGRAM_NOT_FOUND;
					} else {
						MBTRACE(MBWARN, 1, "No subscribers were found in the client list file, so there is no Round Trip Latency to measure.\n");
					}

				}

				if (iop->reconnectMsgHist == NULL) {
					if ((mqttClientType & CONTAINS_CONSUMERS) > 0) {
						MBTRACE(MBERROR, 1, "No Histogram found for Reconnect Round Trip Latency.\n");
						rc = RC_HISTOGRAM_NOT_FOUND;
					} else {
						MBTRACE(MBWARN, 1, "No subscribers were found in the client list file, so there is no Reconnect Round Trip Latency to measure.\n");
					}
				}
				break;
			case CHKTIMESEND:
				if (iop->sendHist == NULL) {
					if ((mqttClientType & CONTAINS_PRODUCERS) > 0) {
						MBTRACE(MBERROR, 1, "No Histogram found for Send Call Latency.\n");
						rc = RC_HISTOGRAM_NOT_FOUND;
					} else {
						MBTRACE(MBERROR, 1, "No Publishers for Send Call Latency.\n");
						rc = RC_SEND_LATENCY_NO_PRODUCERS;
					}
				}
				break;
			default:
				continue;
		} /* switch (latencyBitSet) */

		if (rc)
			break;
	}

	return rc;
} /* checkLatencyHistograms */

/* *************************************************************************************
 * calculateLatency
 *
 * Description:  Calculate the latency based on the mask provided.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *   @param[in]  latencyType         = The Latency Mask
 *   @param[in]  where               = A value which provides where the data should be
 *                                     placed.
 *
 *                                     Supported Values:
 *                                        OUTPUT_CONSOLE - Put the output to the console
 *                                                         and fpCSV == NULL.
 *                                        OUTPUT_CSVFILE - Put the output to a CSV file
 *                                                         and fpCSV contains the file
 *                                                         handle for the CSV file to
 *                                                         be written to.
 *   @param[in]  aggregateOnly       = If set then only perform the aggregate data.
 *                                     If not set then collects the data for each IOP
 *                                     as well as the aggregate data.
 *   @param[in]  fpCSV               = The file handle for the file to be written to.
 *   @param[in]  fpHgram             = The file handle for the Histogram to be written
 *                                     to.  Histogram is not created if NULL.
 * *************************************************************************************/
void calculateLatency (mqttbenchInfo_t *mqttbenchInfo,
		               int latencyType,
					   int where,
					   int aggregateOnly,
					   FILE *fpCSV,
					   FILE *fpHgram)
{
	static uint8_t hdrPrinted_Latency = 0;

	environmentSet_t *pSysEnvSet = mqttbenchInfo->mbSysEnvSet;

	int i, b;
	int entryCt;
	int numPasses;
	int passCtr;
	int numIOPThrds = 0;
	int numConnLatency = 0;
	int currCtr = 0;
	int equiv1Sec = 0;
	int equiv5Sec = 0;
	int maxNumEntries = 0;
	int maxAggrNumEntries = pSysEnvSet->maxHgramSize;
	int aggr50thLatency, aggr75thLatency;
	int aggr90thLatency, aggr95thLatency, aggr99thLatency;
	int aggr999thLatency, aggr9999thLatency, aggr99999thLatency;
	int mask;
	int currBitInMask;
	int currLatencyType;

	int hdrLoop[4] = {0};

	uint32_t aggr1SecCount;
	uint32_t aggr5SecCount;
	uint32_t aggrMinLatency;
	uint32_t aggrMaxLatency;
	uint32_t aggrHistogram[maxAggrNumEntries];

	uint64_t aggrMsgCount = 0;
	uint64_t aggrHistSampleCount = 0;
	uint64_t aggrTotalSampleCount = 0;
	uint64_t aggrSumLatency = 0;

	double aggrAvgLatency, aggrStdDev, sigma;
	double currUnits = 0.0;
	double testUnits = 0.0;
	double iopHistTimeStamp;

   	char hdrLine[MAX_HDRLINE] = {0};
   	char hgramTitle[MAX_HGRAM_TITLE] = {0};
   	char snapStatLine[MAX_SNAP_STATLINE] = {0};
   	char snapCurrLine[MAX_SNAP_CURRLINE] = {0};
   	char snapRTTLine[MAX_SNAP_RTTLINE] = {0};
   	char snapReconnRTTLine[MAX_SNAP_RECONN_RTTLINE] = {0};

   	const char *currMinMaxUnits = NULL;
   	const char *currPfx = NULL;
   	const char *currSfx = NULL;

   	/* Non-Snap Header information (Default Headers) */
	const char *lat_hdr_pfx = "MsgCt,TotalSampleCt,HistSampleCt,";
	const char *lat_conn_hdr_pfx = "TotalSampleCt,HistSampleCt,";
	const char *lat_hdr_sfx = "50th,StdDev,95th,99th,99.9th,99.99th,99.999th,1Sec,5Sec";

	const char *lat_hdr_micros = "Min(us),Max(us),Avg(us),";
	const char *lat_hdr_millis = "Min(ms),Max(ms),Avg(ms),";
	const char *lat_hdr_secs   = "Min(s),Max(s),Avg(s),";
	const char *lat_hdr_def    = "Min(xs),Max(xs),Avg(xs),";

	/* Snap - Connection Latency */
	const char *snap_conn_lat_hdr_pfx    = "conn_Units,conn_Total#Samples,conn_Hist#Samples,";
	const char *snap_conn_lat_hdr_sfx    = "conn_StdDev,conn_50th,conn_75th,conn_90th,conn_99th,conn_1Sec,conn_5Sec";

	const char *snap_conn_lat_hdr_micros = "conn_Min(us),conn_Max(us),conn_Avg(us),";
	const char *snap_conn_lat_hdr_millis = "conn_Min(ms),conn_Max(ms),conn_Avg(ms),";
	const char *snap_conn_lat_hdr_secs   = "conn_Min(s),conn_Max(s),conn_Avg(s),";
	const char *snap_conn_lat_hdr_def    = "conn_Min(xs),conn_Max(xs),conn_Avg(xs),";

	/* Snap - Message Latency  */
	const char *snap_msg_lat_hdr_pfx     = "msg_Units,msg_#Msgs,msg_Total#Samples,msg_Hist#Samples,";
	const char *snap_msg_lat_hdr_sfx     = "msg_StdDev,msg_50th,msg_75th,msg_90th,msg_99th,msg_1Sec,msg_5Sec";

	const char *snap_msg_lat_hdr_micros  = "msg_Min(us),msg_Max(us),msg_Avg(us),";
	const char *snap_msg_lat_hdr_millis  = "msg_Min(ms),msg_Max(ms),msg_Avg(ms),";
	const char *snap_msg_lat_hdr_secs    = "msg_Min(s),msg_Max(s),msg_Avg(s),";
	const char *snap_msg_lat_hdr_def     = "msg_Min(xs),msg_Max(xs),msg_Avg(xs),";

	/* Snap - Reconnect Message Latency Header information */
	const char *snap_rmsg_lat_hdr_pfx    = "rmsg_Units,rmsg_#Msgs,rmsg_Total#Samples,rmsg_Hist#Samples,";
	const char *snap_rmsg_lat_hdr_sfx    = "rmsg_StdDev,rmsg_50th,rmsg_75th,rmsg_90th,rmsg_99th,rmsg_1Sec,rmsg_5Sec";

	const char *snap_rmsg_lat_hdr_micros = "rmsg_Min(us),rmsg_Max(us),rmsg_Avg(us),";
	const char *snap_rmsg_lat_hdr_millis = "rmsg_Min(ms),rmsg_Max(ms),rmsg_Avg(ms),";
	const char *snap_rmsg_lat_hdr_secs   = "rmsg_Min(s),rmsg_Max(s),rmsg_Avg(s),";
	const char *snap_rmsg_lat_hdr_def    = "rmsg_Min(xs),rmsg_Max(xs),rmsg_Avg(xs),";

	/* Variables needed for getting the time for logging */
	char snapTime[MAX_TIME_STRING_LEN];

	volatile ioProcThread_t *iop = NULL;
	latencystat_t *latData = NULL;

	/* Flags and/or counters */
	uint8_t iopBasedLatency = 1;
	uint8_t rttLatency = 0;
	uint8_t useSSL = mqttbenchInfo->useSecureConnections;
	uint8_t snapShotTime = 0;

	/* Obtain the current logging time in format of:  HH:MM:SS */
	createLogTimeStamp(snapTime, MAX_TIME_STRING_LEN, LOGFORMAT_YY_MM_DD);

	/* Set the number of IOP Threads */
	numIOPThrds = (int)pSysEnvSet->numIOProcThreads;

	/* --------------------------------------------------------------------------------
	 * Start working......  This supports both individual latency and snapshots.
	 * Snapshots is determined by looking at the snapshot interval.
	 * -------------------------------------------------------------------------------- */

	/* Check if the resetHeaders flag is set if so then reset the static for headers. */
	if (g_ResetHeaders == 1)
		hdrPrinted_Latency = 0;

	/* If the File Pointer is NULL then set to stderr. */
	if (fpCSV == NULL)
		fpCSV = stderr;

	/* --------------------------------------------------------------------------------
	 * If specified -snap parameter then the CSV File format needs to be as follows:
	 *
	 * Timestamp,Units,Total#Samples,Hist#Samples,min,max,avg,stddev,50,75,90,99,>1,>5, ,
	 *           Units,#msgs,Total#Samples,Hist#Samples,min,max,avg,stddev,50,75,90,99,>1,>5,msgsizes
	 *           Units,#msgs,Total#Samples,Hist#Samples,min,max,avg,stddev,50,75,90,99,>1,>5
	 * -------------------------------------------------------------------------------- */
	if (mqttbenchInfo->mbCmdLineArgs->snapInterval)
		snapShotTime = 1;

	/* --------------------------------------------------------------------------------
	 * Determine the various types of latency needed to build the header correctly, and
	 * also count the number of bits set in the mask.
	 * -------------------------------------------------------------------------------- */
	for ( b = 0, mask = 0x1 ; b < LAT_MASK_SIZE ; mask <<= 1, b++ ) {
		int bitSetInMask = mask & latencyType;

		switch (bitSetInMask) {
			case CHKTIMERTT:
				rttLatency = 2;
				break;
			case CHKTIMETCPCONN:
			case CHKTIMEMQTTCONN:
			case CHKTIMESUBSCRIBE:
			case CHKTIMETCP2MQTTCONN:
			case CHKTIMETCP2SUBSCRIBE:
				numConnLatency++;
				break;
			case CHKTIMESEND:
				break;
			default:
				break;
		} /* switch (bitSetInMask) */
	}

	/* Build and print the header for the file if performing snapshots */
	if ((hdrPrinted_Latency == 0) && (snapShotTime == 1)) {
		int totalNumLoops = rttLatency + numConnLatency;

		/* Setup the hdrLoop in the order of the data to be displayed */
		if (totalNumLoops == 3) {
			hdrLoop[0] = LATENCY_CONN_SUB;
			hdrLoop[1] = LATENCY_RTT;
			hdrLoop[2] = LATENCY_RECONN_RTT;
		} else if (totalNumLoops == 2) {
			hdrLoop[0] = LATENCY_RTT;
			hdrLoop[1] = LATENCY_RECONN_RTT;
		} else
			hdrLoop[0] = LATENCY_CONN_SUB;

		/* Build the header for the snap latency file. */
		for ( i = 0 ; i < totalNumLoops ; i++ ) {
			currPfx = NULL;
			currSfx = NULL;

			if (i == 0) {
				strcpy(hdrLine, "TimeStamp,");
			} else
				strcat(hdrLine, ",,");

			if (hdrLoop[i] == LATENCY_CONN_SUB) {
				equiv1Sec = g_Equiv1Sec_Conn;
				currUnits = 1.0/g_UnitsConn;
				currPfx = snap_conn_lat_hdr_pfx;

				if (g_UnitsConn == MICROS_PER_SECOND)
					currMinMaxUnits = snap_conn_lat_hdr_micros;
				else if (g_UnitsConn == MILLI_PER_SECOND)
					currMinMaxUnits = snap_conn_lat_hdr_millis;
				else if (g_UnitsConn == 1.0)
					currMinMaxUnits = snap_conn_lat_hdr_secs;
				else
					currMinMaxUnits = snap_conn_lat_hdr_def;

				currSfx = snap_conn_lat_hdr_sfx;
			} else if (hdrLoop[i] == LATENCY_RTT) {
				equiv1Sec = g_Equiv1Sec_RTT;
				currUnits = 1.0/g_UnitsRTT;
				currPfx = snap_msg_lat_hdr_pfx;

				if (g_UnitsRTT == MICROS_PER_SECOND)
					currMinMaxUnits = snap_msg_lat_hdr_micros;
				else if (g_UnitsRTT == MILLI_PER_SECOND)
					currMinMaxUnits = snap_msg_lat_hdr_millis;
				else if (g_UnitsRTT == 1.0)
					currMinMaxUnits = snap_msg_lat_hdr_secs;
				else
					currMinMaxUnits = snap_msg_lat_hdr_def;

				currSfx = snap_msg_lat_hdr_sfx;
			} else { /* LATENCY_RECONN_RTT */
				equiv1Sec = g_Equiv1Sec_Reconn_RTT;
				currUnits = 1.0/g_UnitsReconnRTT;
				currPfx = snap_rmsg_lat_hdr_pfx;

				if (g_UnitsReconnRTT == MICROS_PER_SECOND)
					currMinMaxUnits = snap_rmsg_lat_hdr_micros;
				else if (g_UnitsReconnRTT == MILLI_PER_SECOND)
					currMinMaxUnits = snap_rmsg_lat_hdr_millis;
				else if (g_UnitsReconnRTT == 1.0)
					currMinMaxUnits = snap_rmsg_lat_hdr_secs;
				else
					currMinMaxUnits = snap_rmsg_lat_hdr_def;

				currSfx = snap_rmsg_lat_hdr_sfx;
			}

			strcat(hdrLine, currPfx);
			strcat(hdrLine, currMinMaxUnits);
			strcat(hdrLine, currSfx);

			if (hdrLoop[i] == LATENCY_RTT)
				strcat(hdrLine, ",MsgSize(bytes)");
		} /* for ( i = 0 ; i < numConnLatency ; i++ ) */

		fprintf(fpCSV, "%s\n", hdrLine);
		fflush(fpCSV);

		hdrPrinted_Latency = 1;
	}

	/* Prepare buffer for sending metrics to Graphite */
	char xbuf[64 * 1024];
	concat_alloc_t graphiteBuffer = {xbuf, sizeof xbuf};
	char value[MAX_METRIC_VALUE_LEN] = {0};
	char metricPath[MAX_METRIC_NAME_LEN] = {0};
	char metricTuple[MAX_METRIC_TUPLE_LEN] = {0};
	int timeSecs = time(NULL);

	/* --------------------------------------------------------------------------------
	 * If this is snapshots, then need to loop through the entire mask.  parseArgs()
	 * turns off the PRINTHISTOGRAM if it was turned on and allocates the necessary
	 * files to dump the histograms.
	 *
	 * The format of the csv file for snapshots is 1 line every x secs with the format
	 * being:
	 *
	 *   TimeStamp,Units,[Connection Latency],,[Message Latency (RTT)],,[Reconnect Message Latency (RTT)]
	 *
	 * Knowing that the message latency bit in the mask is: 0x1, which means it will
	 * be gathered first, all the data is stored in the rttLine.   This data will be
	 * appended to the connection latency at the end of the call.  In addition, the
	 * snapshots is aggregate data ONLY but the data for each IOP still needs to be
	 * collected to create the aggregate data.
	 * -------------------------------------------------------------------------------- */
	for ( b = 0, mask = 0x1 ; b < LAT_MASK_SIZE ; mask <<= 1, b++ ) {
		/* ----------------------------------------------------------------------------
		 * Ensure there is a bit set before doing any work.  If there is a bit set then
		 * need to reset all the aggregate counters to ensure that multiple requests
		 * don't get mixed together.
		 * ---------------------------------------------------------------------------- */
		if ((currBitInMask = (mask & latencyType)) > 0) {
			/* ------------------------------------------------------------------------
			 * Set the currLatencyType and the number of passes that will be needed.
			 * If requesting CHKTIMERTT then:
			 *    2 passes and the currLatencyType is set to LATENCY_RTT
			 * Else
			 *    1 pass and the currLatencyType is set to LATENCY_CONN which includes
			 *    all types of connection latencies supported.
			 * ------------------------------------------------------------------------ */
			switch (currBitInMask) {
				case CHKTIMERTT:
					numPasses = 2;
					currLatencyType = LATENCY_RTT;
					break;
				case CHKTIMETCPCONN:
				case CHKTIMEMQTTCONN:
				case CHKTIMESUBSCRIBE:
				case CHKTIMETCP2MQTTCONN:
				case CHKTIMETCP2SUBSCRIBE:
					numPasses = 1;
					currLatencyType = LATENCY_CONN_SUB;
					break;
				case CHKTIMESEND:
					numPasses = 1;
					currLatencyType = LATENCY_SEND;
					break;
				default:
					continue;
			} /* switch (bitSetInMask) */

			/* ------------------------------------------------------------------------
			 * Gather the aggregrate based on the number of passes.  The only time
			 * there will be a 2nd pass is if there was RTT latency collected since
			 * there could be Reconnect RTT Latency.
			 * ------------------------------------------------------------------------ */
			for ( passCtr = 0 ; passCtr < numPasses ; passCtr++ ) {
				/* Reset all the aggregate latency variables. */
				aggrMinLatency = MINLATENCY;
				aggrMaxLatency = 0;
				aggr1SecCount = 0;
				aggr5SecCount = 0;
				aggr50thLatency = 0;
				aggr75thLatency = 0;
				aggr90thLatency = 0;
				aggr95thLatency = 0;
				aggr99thLatency = 0;
				aggr999thLatency = 0;
				aggr9999thLatency = 0;
				aggr99999thLatency = 0;
				aggrSumLatency = 0;
				aggrHistSampleCount = 0;
				aggrTotalSampleCount = 0;
				aggrAvgLatency = 0.0;
				aggrStdDev = 0.0;
				sigma = 0.0;
				memset(snapCurrLine, 0, MAX_SNAP_CURRLINE);
				memset(hdrLine, 0, MAX_HDRLINE);

				/* Initialize the Aggregate Histogram to zeros. */
				memset(aggrHistogram, 0, (sizeof(uint32_t) * maxAggrNumEntries));

				/* --------------------------------------------------------------------
				 * Update currLatencyType if RTT Latency.  If passCtr = 1 then change
				 * to LATENCY_RECONN_RTT.
				 * -------------------------------------------------------------------- */
				if (currLatencyType == LATENCY_CONN_SUB) {
					equiv1Sec = g_Equiv1Sec_Conn;
					currUnits = 1.0/g_UnitsConn;
					testUnits = g_UnitsConn;
				} else if (currLatencyType == LATENCY_RTT) {
					/* (passCtr == 0 is for RTT) AND (passCtr == 1 is for Reconnect RTT) */
					if (passCtr == 1) {
						currLatencyType = LATENCY_RECONN_RTT;
						equiv1Sec = g_Equiv1Sec_Reconn_RTT;
						currUnits = 1.0/g_UnitsReconnRTT;
						testUnits = g_UnitsReconnRTT;
					} else {  /* LATENCY_RTT */
						equiv1Sec = g_Equiv1Sec_RTT;
						currUnits = 1.0/g_UnitsRTT;
						testUnits = g_UnitsRTT;
					}
				} else {  /* LATENCY_SEND */
					equiv1Sec = g_Equiv1Sec_RTT;
					currUnits = 1.0/g_UnitsRTT;
					testUnits = g_UnitsRTT;
				}

				equiv5Sec = equiv1Sec * 5;   /* Set the 5sec value. */

				/* --------------------------------------------------------------------
				 * If this is NOT snapshots then need to establish the correct Units of
				 * time and title that is needed.   This needs to be placed in the file
				 * prior to the data.
				 * -------------------------------------------------------------------- */
				if (snapShotTime == 0) {
					if (testUnits == MICROS_PER_SECOND)      /* micro-seconds */
						currMinMaxUnits = lat_hdr_micros;
					else if (testUnits == MILLI_PER_SECOND)  /* milli-seconds */
						currMinMaxUnits = lat_hdr_millis;
					else if (testUnits == 1.0)               /* seconds */
						currMinMaxUnits = lat_hdr_secs;
					else                                     /* other unit */
						currMinMaxUnits = lat_hdr_def;

					fprintf(fpCSV, "Units:  %.1e\n\n", currUnits);
					memset(hgramTitle, 0, MAX_HGRAM_TITLE);

					/* ----------------------------------------------------------------
					 * If not aggregrate then print the type of latency in the csv file
					 * and establish the histogram title.
					 * ---------------------------------------------------------------- */
					if (!aggregateOnly) {
						switch (currBitInMask) {
							case CHKTIMETCPCONN:
								fprintf(fpCSV, "Latency for TCP%s Connection creation\n", (useSSL > 0 ? "/SSL" : ""));
								memcpy(hgramTitle, HGRAM_TCP_CONN, LEN_HGRAM_TCP_CONN);
								break;
							case CHKTIMERTT:
								if (passCtr == 0) {
									fprintf(fpCSV, "Round Trip Latency\n");
									memcpy(hgramTitle, HGRAM_RTT, LEN_HGRAM_RTT);
								} else {
									fprintf(fpCSV, "Reconnect RTT Latency\n");
									memcpy(hgramTitle, HGRAM_RECONN_RTT, LEN_HGRAM_RECONN_RTT);
								}
								break;
							case CHKTIMEMQTTCONN:
								fprintf(fpCSV, "Latency for MQTT Connection creation\n");
								memcpy(hgramTitle, HGRAM_MQTT_CONN, LEN_HGRAM_MQTT_CONN);
								break;
							case CHKTIMESUBSCRIBE:
								fprintf(fpCSV, "Latency for Subscriptions\n");
								memcpy(hgramTitle, HGRAM_SUBS, LEN_HGRAM_SUBS);
								break;
							case CHKTIMETCP2MQTTCONN:
								fprintf(fpCSV, "Latency for TCP - MQTT Connection\n");
								memcpy(hgramTitle, HGRAM_TCP_MQTT_CONN, LEN_HGRAM_TCP_MQTT_CONN);
								break;
							case CHKTIMETCP2SUBSCRIBE:
								fprintf(fpCSV, "Latency for TCP - Subscriptions\n");
								memcpy(hgramTitle, HGRAM_TCP_SUBS, LEN_HGRAM_TCP_SUBS);
								break;
							case CHKTIMESEND:
								fprintf(fpCSV, "Latency for the send call\n");
								memcpy(hgramTitle, HGRAM_SEND, LEN_HGRAM_SEND);
								break;
							default:
								break;
						} /* switch (currBitInMask) */

						/* Print the appropriate latency headers to the csv file. */
						if (currLatencyType == LATENCY_CONN_SUB)
							fprintf(fpCSV, "Thread,%s", lat_conn_hdr_pfx);
						else
							fprintf(fpCSV, "Thread,%s", lat_hdr_pfx);

						fprintf(fpCSV, "%s%s\n", currMinMaxUnits, lat_hdr_sfx);
						fflush(fpCSV);
					} /* if (!aggregateOnly) */
				} /* if (snapShotTime == 0) */

				/* Latency is based on the IOP Threads so loop through all the IOPs */
				for ( i = 0 ; i < numIOPThrds ; i++ ) {
					int thrd50thLatency = 0, thrd95thLatency = 0, thrd99thLatency = 0;
					int thrd999thLatency = 0, thrd9999thLatency = 0, thrd99999thLatency = 0;

					uint32_t thrd1SecCount, thrd5SecCount;
					uint32_t thrdMinLatency = MINLATENCY, thrdMaxLatency = 0;
					uint64_t thrdSumLatency = 0;
					uint64_t thrdMsgCount = 0;
					uint64_t thrdHistSampleCount = 0;
					uint64_t thrdTotalSampleCount = 0;

					double thrdAvgLatency = 0.0, thrdStdDev = 0.0;

					sigma = 0.0; /* Reset the sigma value for this thread. */
					latData = NULL;
					iop = ioProcThreads[i];
					if (iop == NULL) {
						MBTRACE(MBERROR, 1, "IOP %d is NULL.\n", i);
						continue;
					}

					/* ----------------------------------------------------------------
					 * Initialize the Maximum # of entries for a histogram to the System
					 * Environment maxHistogramSize and it will be modified if it is
					 * RTT to maxRTTHistogramSize.
					 * ---------------------------------------------------------------- */
					maxNumEntries = pSysEnvSet->maxConnHistogramSize;

					/* ----------------------------------------------------------------
					 * Based on the bit set in the mask set the latData to corresponding
					 * IOP histogram.
					 * ---------------------------------------------------------------- */
					switch (currBitInMask) {
						case CHKTIMETCPCONN:
							latData = iop->tcpConnHist;
							break;
						case CHKTIMERTT:
							if (passCtr == 0)
								latData = iop->rttHist;
							else
								latData = iop->reconnectMsgHist;

							maxNumEntries = pSysEnvSet->maxMsgHistogramSize; /* Set Max to Messages Histogram size */
							break;
						case CHKTIMEMQTTCONN:
						case CHKTIMETCP2MQTTCONN:
							latData = iop->mqttConnHist;
							break;
						case CHKTIMESUBSCRIBE:
						case CHKTIMETCP2SUBSCRIBE:
							latData = iop->subscribeHist;
							break;
						case CHKTIMESEND:
							latData = iop->sendHist;
							break;
						default:
							continue;
					} /* switch (currBitInMask) */

					if (latData == NULL || latData->histogram == NULL)
						continue;

					/* Read the current timestamp for this IOP Histogram */
					iopHistTimeStamp = latData->latResetTime;

					/* ----------------------------------------------------------------
					 * Need to see if the histogram contains latency which is in the
					 * range of 1 & 5 secs so that the respective counters are updated.
					 * If the big variable for the histogram is set then there was data
					 * larger than the histogram and need to initialize the local 1sec
					 * and 5sec counters with the values in the histogram.
					 * ---------------------------------------------------------------- */
					if (latData->big) {
						thrd1SecCount = latData->count1Sec;
						thrd5SecCount = latData->count5Sec;
					} else {
						thrd1SecCount = 0;
						thrd5SecCount = 0;
					}

					/* ----------------------------------------------------------------
					 * Add up the total Sample Counts that were collected for each IOP
					 * whether or not it was in the histogram.
					 * ---------------------------------------------------------------- */
					thrdTotalSampleCount += latData->totalSampleCt;

					/* ----------------------------------------------------------------
					 * Go through the entire histogram and count how many actual samples
					 * were found in the histogram.
					 * ---------------------------------------------------------------- */
					for ( entryCt = 0 ; entryCt < maxNumEntries ; entryCt++ ) {
						if (latData->histogram[entryCt] > 0) {
							thrdHistSampleCount += latData->histogram[entryCt];
							thrdSumLatency += (entryCt * latData->histogram[entryCt]);

							/* Record the Min, Max, # > 1sec and # > 5secs */
							if (entryCt < thrdMinLatency)
								thrdMinLatency = entryCt;
							if (entryCt > latData->max)
								thrdMaxLatency = entryCt;
							if (entryCt > equiv1Sec) {
								thrd1SecCount += latData->histogram[entryCt];
								if (entryCt > equiv5Sec)
									thrd5SecCount += latData->histogram[entryCt];
							}
						}
					}

					/* Update the aggregate Total Sample Count. */
					aggrTotalSampleCount += thrdTotalSampleCount;

					/* ----------------------------------------------------------------
					 * If there are no samples for this IOP then don't display any rate
					 * for this thread.  Just show the message count and sample count = 0.
					 * ---------------------------------------------------------------- */
					if (thrdHistSampleCount) {
						/* For RTT latency and passCtr = 0, count the PUBLISH msg */
						if ((currBitInMask == CHKTIMERTT) && (passCtr == 0)) {
							thrdMsgCount = iop->currRxMsgCounts[PUBLISH];
							aggrMsgCount += thrdMsgCount;
						}

						if (latData->max)
							thrdMaxLatency = latData->max;

						if (thrdMinLatency < aggrMinLatency)
							aggrMinLatency = thrdMinLatency;
						if (thrdMaxLatency > aggrMaxLatency)
							aggrMaxLatency = thrdMaxLatency;

						/* Get the average latency for this thread. */
						thrdAvgLatency = (double)thrdSumLatency / (double)thrdHistSampleCount;

						/* Update the aggregate sample count and sum of latency */
						aggrHistSampleCount += thrdHistSampleCount;
						aggrSumLatency += thrdSumLatency;

						/* Now determine the thread's standard deviation. */
						for ( entryCt = 0 ; entryCt < maxNumEntries ; entryCt++ ) {
							if (latData->histogram[entryCt] > 0) {
								sigma += (double)(((double)(entryCt - thrdAvgLatency) * (double)(entryCt - thrdAvgLatency)) *
									latData->histogram[entryCt]);
							}
						}

						thrdStdDev = sqrt(sigma / (double)thrdHistSampleCount);

						/* Get the 50th, 70th, 90th, 95th, 99th, and 999th Percentile. */
						thrd50thLatency    = getLatencyPercentile(latData->histogram, 0.50, maxNumEntries);
						thrd95thLatency    = getLatencyPercentile(latData->histogram, 0.95, maxNumEntries);
						thrd99thLatency    = getLatencyPercentile(latData->histogram, 0.99, maxNumEntries);
						thrd999thLatency   = getLatencyPercentile(latData->histogram, 0.999, maxNumEntries);
						thrd9999thLatency  = getLatencyPercentile(latData->histogram, 0.9999, maxNumEntries);
						thrd99999thLatency = getLatencyPercentile(latData->histogram, 0.99999, maxNumEntries);

						aggr1SecCount += thrd1SecCount;
						aggr5SecCount += thrd5SecCount;
					} else
						thrdMinLatency = 0;

					/* ----------------------------------------------------------------
					 * Compare the original timestamp taken from the IOP Latency
					 * Structure with the current timestamp value.   If performing Reset
					 * In-Memory Latency (-rl option) there is a chance that there was
					 * a reset which means the data is not complete since the histogram
					 * gets cleared.   Hence, if they are different exit this routine
					 * and don't record anything.
					 * ---------------------------------------------------------------- */
					if (iopHistTimeStamp != latData->latResetTime)
						return;

					/* Write the thread statistics out to the data file or console. */
					if (!aggregateOnly) {
						if (currLatencyType == LATENCY_CONN_SUB) {
							fprintf(fpCSV, "%s%d,%llu,%llu,%lu,%lu,%.2f,%d,%.2f,%d,%d,%d,%d,%d,%lu,%lu\n",
									       "iop",
						                   i,
										   (ULL) thrdTotalSampleCount,
						                   (ULL) thrdHistSampleCount,
									       (unsigned long) thrdMinLatency,
									       (unsigned long) thrdMaxLatency,
						                   thrdAvgLatency,
						                   thrd50thLatency,
						                   thrdStdDev,
						                   thrd95thLatency,
						                   thrd99thLatency,
						                   thrd999thLatency,
						                   thrd9999thLatency,
						                   thrd99999thLatency,
									       (unsigned long) thrd1SecCount,
									       (unsigned long) thrd5SecCount);
						} else { /* RTT Message Latency or Reconnect RTT Latency */
							fprintf(fpCSV, "%s%d,%llu,%llu,%llu,%lu,%lu,%.2f,%d,%.2f,%d,%d,%d,%d,%d,%lu,%lu\n",
								           "iop",
						                   i,
						                   (ULL) thrdMsgCount,
						                   (ULL) thrdTotalSampleCount,
						                   (ULL) thrdHistSampleCount,
									       (unsigned long) thrdMinLatency,
									       (unsigned long) thrdMaxLatency,
						                   thrdAvgLatency,
						                   thrd50thLatency,
						                   thrdStdDev,
						                   thrd95thLatency,
						                   thrd99thLatency,
						                   thrd999thLatency,
						                   thrd9999thLatency,
						                   thrd99999thLatency,
									       (unsigned long) thrd1SecCount,
									       (unsigned long) thrd5SecCount);
						}

						fflush(fpCSV);
					}

					/* Update the aggregate histogram adding this thread's histogram. */
					if (thrdHistSampleCount) {
						addHistogram(latData->histogram, aggrHistogram, maxNumEntries);
					}
				} /* for ( i = 0 ; i < numIOPThrds ; i++ ) */

				/* --------------------------------------------------------------------
				 * See if the histogram needs to be printed out.  There are 2 ways to
				 * have this occur:
				 *
				 *   - fpHgram is valid, and PRINTHISTOGRAM is not set in the latencyType
				 *   - fpHgram is valid, and PRINTHISTOGRAM is set in the latencyType
				 *     with snapshots.
				 * -------------------------------------------------------------------- */
				if (fpHgram) {
					fprintf(fpHgram, "%s  %s\n", hgramTitle, "iop");
					fprintf(fpHgram, "Latency,Count\n");

					for ( entryCt = 0 ; entryCt < maxNumEntries ; entryCt++ ) {
						fprintf(fpHgram, "%d,%u\n", entryCt, aggrHistogram[entryCt]);
					}

					fprintf(fpHgram, "-------------------------------------\n");
					fflush(fpHgram);
				}

				/* --------------------------------------------------------------------
				 * See if there is any data for the aggregate.  If not put a message:
				 * "NO DATA AVAILABLE" in the csv file.
				 * -------------------------------------------------------------------- */
				if (aggrHistSampleCount > 0) {
					/* Determine the average for the aggregate. */
					aggrAvgLatency = (double)aggrSumLatency / (double)aggrHistSampleCount;

					/* Now determine the aggregate's standard deviation. */
					sigma = 0.0;    /* Reset the sigma since previously used for threads. */
					for ( i = 0 ; i < maxNumEntries ; i++ ) {
						if (aggrHistogram[i] > 0) {
							sigma += (double)(((double)(i - aggrAvgLatency) * (double)(i - aggrAvgLatency)) *
									aggrHistogram[i]);
						}
					}

					aggrStdDev = sqrt(sigma / (double)aggrHistSampleCount);

					/* Get the 50th, 75th, 90th, 95th, 99th, and 999th Percentile. */
					aggr50thLatency    = getLatencyPercentile(aggrHistogram, 0.50, maxNumEntries);
					aggr75thLatency    = getLatencyPercentile(aggrHistogram, 0.75, maxNumEntries);
					aggr90thLatency    = getLatencyPercentile(aggrHistogram, 0.90, maxNumEntries);
					aggr95thLatency    = getLatencyPercentile(aggrHistogram, 0.95, maxNumEntries);
					aggr99thLatency    = getLatencyPercentile(aggrHistogram, 0.99, maxNumEntries);
					aggr999thLatency   = getLatencyPercentile(aggrHistogram, 0.999, maxNumEntries);
					aggr9999thLatency  = getLatencyPercentile(aggrHistogram, 0.9999, maxNumEntries);
					aggr99999thLatency = getLatencyPercentile(aggrHistogram, 0.99999, maxNumEntries);

					/* Print the corresponding headers and data depending on whether
					 * snapshots are enabled or not.  */
					if (snapShotTime == 0) {
						/* ------------------------------------------------------------
						 * Print out the aggregate latency header.  Currently it is
						 * separated being either Connection Latency -OR- some kind
						 * of message latency
						 * ------------------------------------------------------------ */
						if (currLatencyType == LATENCY_CONN_SUB)
							fprintf(fpCSV, "\nAggr,%s", lat_conn_hdr_pfx);
						else /* Either RTT Latency or Reconnect RTT Latency */
							fprintf(fpCSV, "\nAggr,%s", lat_hdr_pfx);

						fprintf(fpCSV, "%s", currMinMaxUnits);   /* Add the units being used. */

						/* If this is RTT latency then print the msg sizes with the header
						 * suffix else just header suffix.  */
						if (currLatencyType == LATENCY_RTT)
							fprintf(fpCSV, "%s,MsgSize(bytes)\n", lat_hdr_sfx);
						else  /* Either Reconnect RTT Latency or a Connection Latency. */
							fprintf(fpCSV, "%s\n", lat_hdr_sfx);

						/* ------------------------------------------------------------
						 * Print out the aggregate data based on the type of latency.
						 * Currently separated as Connection Latency, RTT Latency and
						 * then all other message latencies.
						 * ------------------------------------------------------------ */
						if (currLatencyType == LATENCY_CONN_SUB) {
							fprintf(fpCSV, "all %s,%llu,%llu,%lu,%lu,%.2f,%d,%.2f,%d,%d,%d,%d,%d,%lu,%lu\n\n",
					                       (iopBasedLatency ? "iops" : "clients"),
					                       (ULL) aggrTotalSampleCount,
					                       (ULL) aggrHistSampleCount,
					                       (unsigned long) aggrMinLatency,
									       (unsigned long) aggrMaxLatency,
					                       aggrAvgLatency,
					                       aggr50thLatency,
					                       aggrStdDev,
					                       aggr95thLatency,
					                       aggr99thLatency,
					                       aggr999thLatency,
									       aggr9999thLatency,
					                       aggr99999thLatency,
									       (unsigned long) aggr1SecCount,
									       (unsigned long) aggr5SecCount);
						} else if (currLatencyType == LATENCY_RTT) {
							fprintf(fpCSV, "all %s,%llu,%llu,%llu,%lu,%lu,%.2f,%d,%.2f,%d,%d,%d,%d,%d,%lu,%lu,%d-%d\n\n",
				                           (iopBasedLatency ? "iops" : "clients"),
					                       (ULL) aggrMsgCount,
 					                       (ULL) aggrTotalSampleCount,
 					                       (ULL) aggrHistSampleCount,
									       (unsigned long) aggrMinLatency,
									       (unsigned long) aggrMaxLatency,
					                       aggrAvgLatency,
					                       aggr50thLatency,
					                       aggrStdDev,
					                       aggr95thLatency,
					                       aggr99thLatency,
					                       aggr999thLatency,
					                       aggr9999thLatency,
					                       aggr99999thLatency,
									       (unsigned long) aggr1SecCount,
									       (unsigned long) aggr5SecCount,
									       g_MinMsgRcvd,
									       g_MaxMsgRcvd);
						} else {  /* Reconnect RTT Latency */
							fprintf(fpCSV, "all %s,%llu,%llu,%llu,%lu,%lu,%.2f,%d,%.2f,%d,%d,%d,%d,%d,%lu,%lu\n\n",
				                           (iopBasedLatency ? "iops" : "clients"),
					                       (ULL) aggrMsgCount,
 					                       (ULL) aggrTotalSampleCount,
 					                       (ULL) aggrHistSampleCount,
									       (unsigned long) aggrMinLatency,
									       (unsigned long) aggrMaxLatency,
					                       aggrAvgLatency,
					                       aggr50thLatency,
					                       aggrStdDev,
					                       aggr95thLatency,
					                       aggr99thLatency,
					                       aggr999thLatency,
					                       aggr9999thLatency,
					                       aggr99999thLatency,
									       (unsigned long) aggr1SecCount,
									       (unsigned long) aggr5SecCount);
						}

						fflush(fpCSV);
					} else { /* snapshots */
						if (currCtr++ == 0)
							snprintf(snapStatLine, MAX_SNAP_STATLINE, "%s", snapTime);

						/* ------------------------------------------------------------
						 * Print out the snapshot data based on the type of latency.
						 * Currently separated as Connection Latency, RTT Latency and
						 * then all other message latencies.
						 * ------------------------------------------------------------ */
						if (currLatencyType == LATENCY_CONN_SUB) {
							snprintf(snapCurrLine, MAX_SNAP_CURRLINE,
								                   ",%1.e,%llu,%llu,%lu,%lu,%.2f,%.2f,%d,%d,%d,%d,%lu,%lu,",
							                       currUnits,
					                               (ULL) aggrTotalSampleCount,
					                               (ULL) aggrHistSampleCount,
											       (unsigned long) aggrMinLatency,
											       (unsigned long) aggrMaxLatency,
					                               aggrAvgLatency,
					                               aggrStdDev,
					                               aggr50thLatency,
					                               aggr75thLatency,
					                               aggr90thLatency,
					                               aggr99thLatency,
											       (unsigned long) aggr1SecCount,
											       (unsigned long) aggr5SecCount);

							strcat(snapStatLine, snapCurrLine);

							if(pSysEnvSet->GraphiteIP && where == OUTPUT_CSVFILE) {
								snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggrTotalSampleCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_TotalSampleCt", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggrHistSampleCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_HistSampleCt", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%.1e", currUnits);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_Units", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggrMinLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_Min", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggrMaxLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_Max", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%.2f", aggrAvgLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_Avg", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr50thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_50P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", (int) aggrStdDev);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_STDV", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr75thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_75P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr90thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_90P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr95thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_95P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr99thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_99P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr999thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_999P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr9999thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_9999P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggr1SecCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_1sec", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggr5SecCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "ConnLat_5sec", value);
							}
						} else if (currLatencyType == LATENCY_RTT) {
							snprintf(snapRTTLine, MAX_SNAP_RTTLINE,
									              ",%1.e,%llu,%llu,%llu,%lu,%lu,%.2f,%.2f,%d,%d,%d,%d,%lu,%lu,%d-%d,",
					                              currUnits,
						                          (ULL) aggrMsgCount,
	 					                          (ULL) aggrTotalSampleCount,
	 					                          (ULL) aggrHistSampleCount,
												  (unsigned long) aggrMinLatency,
												  (unsigned long) aggrMaxLatency,
						                          aggrAvgLatency,
						                          aggrStdDev,
						                          aggr50thLatency,
					                              aggr75thLatency,
					                              aggr90thLatency,
						                          aggr99thLatency,
												  (unsigned long) aggr1SecCount,
												  (unsigned long) aggr5SecCount,
						                          g_MinMsgRcvd,
						                          g_MaxMsgRcvd);

							if(pSysEnvSet->GraphiteIP && where == OUTPUT_CSVFILE) {
								snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggrMsgCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_TotalMsgCt", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggrTotalSampleCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_TotalSampleCt", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggrHistSampleCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_HistSampleCt", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%.1e", currUnits);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_Units", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggrMinLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_Min", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggrMaxLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_Max", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%.2f", aggrAvgLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_Avg", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr50thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_50P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", (int) aggrStdDev);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_STDV", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr75thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_75P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr90thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_90P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr95thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_95P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr99thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_99P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr999thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_999P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr9999thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_9999P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggr1SecCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_1sec", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggr5SecCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLat_5sec", value);
							}
						} else {  /* Reconnect RTT Latency */
							snprintf(snapReconnRTTLine, MAX_SNAP_RECONN_RTTLINE,
								                        ",%1.e,%llu,%llu,%llu,%lu,%lu,%.2f,%.2f,%d,%d,%d,%d,%lu,%lu,",
				                                        currUnits,
					                                    (ULL) aggrMsgCount,
 					                                    (ULL) aggrTotalSampleCount,
 					                                    (ULL) aggrHistSampleCount,
											            (unsigned long) aggrMinLatency,
											            (unsigned long) aggrMaxLatency,
					                                    aggrAvgLatency,
					                                    aggrStdDev,
					                                    aggr50thLatency,
							                            aggr75thLatency,
							                            aggr90thLatency,
					                                    aggr99thLatency,
											            (unsigned long) aggr1SecCount,
											            (unsigned long) aggr5SecCount);

							if(pSysEnvSet->GraphiteIP && where == OUTPUT_CSVFILE) {
								snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggrMsgCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_TotalMsgCt", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggrTotalSampleCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_TotalSampleCt", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%llu", (ULL) aggrHistSampleCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_HistSampleCt", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%.1e", currUnits);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_Units", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggrMinLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_Min", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggrMaxLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_Max", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%.2f", aggrAvgLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_Avg", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr50thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_50P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", (int) aggrStdDev);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_STDV", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr75thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_75P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr90thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_90P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr95thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_95P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr99thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_99P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr999thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_999P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%d", aggr9999thLatency);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_9999P", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggr1SecCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_1sec", value);
								snprintf(value, MAX_METRIC_VALUE_LEN, "%lu", (unsigned long) aggr5SecCount);
								APPEND_METRIC(&graphiteBuffer, metricPath, metricTuple, "MsgLatReconn_5sec", value);
							}
						}
					} /* if (snapShotTime == 0) */
				} else {
					if (snapShotTime == 0)
						fprintf(fpCSV, "all %s, NO DATA AVAILABLE.\n\n", "iops");
					else {
						if (currCtr++ == 0)
							snprintf(snapStatLine, MAX_SNAP_STATLINE, "%s", snapTime);

						if (currLatencyType == LATENCY_CONN_SUB) {
							snprintf(snapCurrLine, MAX_SNAP_CURRLINE, ",%1.e,0,0,0,0,0.0,0.0,0,0,0,0,0,0,", currUnits);
							strcat(snapStatLine, snapCurrLine);
						} else if (currLatencyType == LATENCY_RTT)
							snprintf(snapRTTLine, MAX_SNAP_RTTLINE, ",%1.e,0,0,0,0,0,0.0,0.0,0,0,0,0,0,0,%d-%d,",
								                  currUnits,
								                  g_MinMsgRcvd,
								                  g_MaxMsgRcvd);
						else  /* Reconnect RTT Latency */
							snprintf(snapReconnRTTLine, MAX_SNAP_RECONN_RTTLINE, ",%1.e,0,0,0,0,0,0.0,0.0,0,0,0,0,0,0,", currUnits);
					}
				}
			} /* for ( passCtr = 0 ; passCtr < numPasses ; passCtr++ ) */
		} /* if ((currBitInMask = (mask & latencyType)) > 0) */
	} /* for ( b = 0, mask = 0x1 ; b < LAT_MASK_SIZE ; mask <<= 1, b++ )  */

	/* If performing snapshots then need to flush the data to the file. */
   	if (snapShotTime) {
   		if (rttLatency) {
   			strcat(snapStatLine, snapRTTLine);
			strcat(snapStatLine, snapReconnRTTLine);
   		}

   		fprintf(fpCSV, "%s\n", snapStatLine);
   		fflush(fpCSV);
   	}

   	if(graphiteBuffer.used > 0 && where == OUTPUT_CSVFILE) {
		int rc = basicConnectAndSend(pSysEnvSet->GraphiteIP, pSysEnvSet->GraphitePort, graphiteBuffer.buf, graphiteBuffer.used);
		if(rc) {
			MBTRACE(MBERROR, 1, "Failed to send mqttbench metrics to Graphite server at %s:%d\n",
								pSysEnvSet->GraphiteIP, pSysEnvSet->GraphitePort);
		}
	}
}  /* calculateLatency */

/* *************************************************************************************
 * provideLatencyWarning
 *
 * Description:  Provide a warning if the latency is greater than the histogram.
 *
 *   @param[in]  pSysEnvSet          = Environment Variable structure
 *   @param[in]  latMask             = Latency Mask
 * *************************************************************************************/
void provideLatencyWarning (environmentSet_t *pSysEnvSet, int latMask)
{
	int i, k;
	int mask;

	uint8_t numIOPThrds = pSysEnvSet->numIOProcThreads;

	uint32_t bigCtr;

	volatile ioProcThread_t *iop;

	for ( i = 0, mask = 0x1 ; i < LAT_MASK_SIZE ; mask <<= 1, i++ ) {
		if ((mask & latMask) > 0) {
			bigCtr = 0;

			switch (mask) {
				case CHKTIMETCPCONN:
					for ( k = 0 ; k < numIOPThrds ; k++ ) {
						iop = ioProcThreads[k];

						if (iop->tcpConnHist)
							bigCtr += iop->tcpConnHist->big;
					}

					if (bigCtr) {
						MBTRACE(MBWARN, 1, "%d TCP Connection Latency entries are larger than the histogram.\n", bigCtr);
					}

					break;
				case CHKTIMEMQTTCONN:
				case CHKTIMETCP2MQTTCONN:
					for ( k = 0 ; k < numIOPThrds ; k++ ) {
						iop = ioProcThreads[k];

						if (iop->mqttConnHist)
							bigCtr += iop->mqttConnHist->big;
					}

					if (bigCtr) {
						MBTRACE(MBWARN, 1, "%d MQTT Connection Latency entries are larger than the histogram.\n", bigCtr);
					}

					break;
				case CHKTIMESUBSCRIBE:
				case CHKTIMETCP2SUBSCRIBE:
					for ( k = 0 ; k < numIOPThrds ; k++ ) {
						iop = ioProcThreads[k];

						if (iop->subscribeHist)
							bigCtr += iop->subscribeHist->big;
					}

					if (bigCtr) {
						MBTRACE(MBWARN, 1, "%d Subscription Latency entries are larger than the histogram.\n", bigCtr);
					}

					break;
				case CHKTIMERTT:
					for ( k = 0 ; k < numIOPThrds ; k++ ) {
						iop = ioProcThreads[k];

						if (iop->rttHist)
							bigCtr += iop->rttHist->big;
					}

					if (bigCtr) {
						MBTRACE(MBWARN, 1, "%d Message RTT Latency entries are larger than the histogram.\n", bigCtr);
					}

					break;
				case CHKTIMESEND:
					for ( k = 0 ; k < numIOPThrds ; k++ ) {
						iop = ioProcThreads[k];

						if (iop->sendHist)
							bigCtr += iop->sendHist->big;
					}

					if (bigCtr) {
						MBTRACE(MBWARN, 1, "%d Send Call Latency entries are larger than the histogram.\n", bigCtr);
					}

					break;
				default:
					continue;
			} /* switch (mask) */
		} /* if ((mask & lat_Mask) > 0) */
	} /* for ( i = 0, mask = 0x1 ; i < LAT_MASK_SIZE ; mask <<= 1, i++ ) */
} /* provideLatencyWarning */

/* *************************************************************************************
 * printHistogram
 *
 * Description: Print the aggregate histogram for the latency type requested.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *   @param[in]  latencyType         = Latency Type to be printed.
 *   @param[in]  maxNumEntries       = Size of the Histogram
 *   @param[in]  fpHgram             = File Descriptor for Histogram FIles
 * *************************************************************************************/
void printHistogram (mqttbenchInfo_t *mqttbenchInfo, int latencyType, int maxNumEntries, FILE *fpHgram)
{
	environmentSet_t *pSysEnvSet = mqttbenchInfo->mbSysEnvSet;

	int i,j;
	int numIOPThrds = 0;

   	char hgramTitle[128] = {0};

	uint32_t aggrHistogram[maxNumEntries];

	volatile ioProcThread_t *iop;

	latencystat_t *latData = NULL;

	/* Ensure there is a bit set before doing any work. */
	if (latencyType > 0) {
		/* Initialize the Aggregate Histogram to zeros. */
		memset(aggrHistogram, 0, (sizeof(uint32_t) * maxNumEntries));

		/* Initialize the Histogram Title to zeros. */
		memset(hgramTitle, 0, 128);

		numIOPThrds = (int)pSysEnvSet->numIOProcThreads;

		/* Latency is based on the IOP Threads so need to loop through all the IOPs */
		for ( i = 0 ; i < numIOPThrds ; i++ ) {
			latData = NULL;
			iop = ioProcThreads[i];

			switch (latencyType) {
				case CHKTIMETCPCONN:
					if (iop->tcpConnHist){
						latData = iop->tcpConnHist;
						memcpy(hgramTitle, HGRAM_TCP_CONN, LEN_HGRAM_TCP_CONN);
					} else {
						MBTRACE(MBERROR, 1, "No Histogram found for TCP Connection Latency.\n");
						continue;
					}
					break;
				case CHKTIMEMQTTCONN:
				case CHKTIMETCP2MQTTCONN:
					if (iop->mqttConnHist) {
						latData = iop->mqttConnHist;
						if (latencyType == CHKTIMEMQTTCONN)
							memcpy(hgramTitle, HGRAM_MQTT_CONN, LEN_HGRAM_MQTT_CONN);
						else
							memcpy(hgramTitle, HGRAM_TCP_MQTT_CONN, LEN_HGRAM_TCP_MQTT_CONN);
					} else {
						if (latencyType == CHKTIMEMQTTCONN) {
							MBTRACE(MBERROR, 1, "No Histogram found for MQTT Connection Latency.\n");
						} else {
							MBTRACE(MBERROR, 1, "No Histogram found for TCP - MQTT Connection Latency.\n");
						}

						continue;
					}
					break;
				case CHKTIMESUBSCRIBE:
				case CHKTIMETCP2SUBSCRIBE:
					if (iop->subscribeHist) {
						latData = iop->subscribeHist;
						if (latencyType == CHKTIMESUBSCRIBE)
							memcpy(hgramTitle, HGRAM_SUBS, LEN_HGRAM_SUBS);
						else
							memcpy(hgramTitle, HGRAM_TCP_SUBS, LEN_HGRAM_TCP_SUBS);
					} else {
						if (latencyType == CHKTIMESUBSCRIBE) {
							MBTRACE(MBERROR, 1, "No Histogram found for Subscription Latency.\n");
						} else {
							MBTRACE(MBERROR, 1, "No Histogram found for TCP - Subscription Latency.\n");
						}

						continue;
					}
					break;
				case CHKTIMERTT:
					if (iop->rttHist) {
						latData = iop->rttHist;
						memcpy(hgramTitle, HGRAM_RTT, LEN_HGRAM_RTT);
					} else {
						MBTRACE(MBERROR, 1, "No Histogram found for Round Trip Latency.\n");
						continue;
					}
					break;
				case CHKTIMESEND:
					if (iop->sendHist) {
						latData = iop->sendHist;
						memcpy(hgramTitle, HGRAM_SEND, LEN_HGRAM_SEND);
					} else {
						MBTRACE(MBERROR, 1, "No Histogram found for Send Call Latency.\n");
						continue;
					}
					break;
				default:
					continue;
			} /* switch (latencyType) */

			if (latData == NULL || latData->histogram == NULL)
				continue;

			/* Add the current histogram to the aggregate histogram */
			addHistogram(latData->histogram, aggrHistogram, maxNumEntries);
		} /* for ( i = 0 ; i < numIOPThrds ; i++ ) */

		/* Print the histogram to file. */
		if (fpHgram) {
			fprintf(fpHgram, "%s  %s\n", hgramTitle, "iop");
			fprintf(fpHgram, "Latency,Count\n");

			for ( j = 0 ; j < maxNumEntries ; j++ ) {
				fprintf(fpHgram, "%d,%u\n", j, aggrHistogram[j]);
			}

			fprintf(fpHgram, "-------------------------------------\n");
			fflush(fpHgram);
		}
	} /* if (latencyType > 0) */
}  /* printHistogram */

/* *************************************************************************************
 * addHistogram
 *
 * Description:  Add src histogram to the dst histogram.
 *
 *   @param[in]  src                 = Source Histogram to copy data from.
 *   @param[in]  dst                 = Destination Histogram to copy to.
 *   @param[in]  maxLatency          = Number of indices in the histogram.
 * *************************************************************************************/
void addHistogram (uint32_t src[], uint32_t dst[], int maxLatency)
{
	int j;

	for ( j = 0 ; j < maxLatency ; j++ ) {
		if (src[j] > 0)
			dst[j] += src[j];
	}
}  /* addHistogram( ) */

/* *************************************************************************************
 * getLatencyPercentile
 *
 * Description:  Calculates the latency percentiles from a histogram of latencies.
 *
 *   @param[in]  histogram           = Histogram that contains the latency entries.
 *   @param[in]  percentile          = The percentile requested.
 *   @param[in]  maxLatency          = Number of indices in the histogram.
 *
 *   @return x   = The requested percentile of the data provided.
 * *************************************************************************************/
int getLatencyPercentile (uint32_t histogram[], double percentile, int maxLatency)
{
	int i;
	int sampleCount = 0;     /* Number of samples in the histogram */
	int percentileCount = 0; /* The sample count that is at the target latency percentile */
	int latency = 0;         /* The latency found at the percentile of all latency in the histogram */

	/* Get number of samples in the histogram */
	for ( i = 0 ; i < maxLatency ; i++ ) {
		if (histogram[i] > 0)
			sampleCount += histogram[i];
	}

	/* Calculate at what number of samples the target latency percentile will be found */
	percentileCount = (int)(percentile * (double)sampleCount);

	/* --------------------------------------------------------------------------------
	 * Moving from lowest index(latency) to highest index(latency) in the histogram
	 * find the index at which point the number of samples reaches the target latency
	 * percentile
	 * -------------------------------------------------------------------------------- */
	for ( i = 0, sampleCount=0 ; i < maxLatency ; i++ ) {
		if ((sampleCount < percentileCount) && ((sampleCount + histogram[i]) >= percentileCount))
		{
			latency = i;
			break;
		}

		sampleCount += histogram[i];
	}

	return latency;
}
