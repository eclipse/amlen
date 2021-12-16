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
#include <ifaddrs.h>
#include <sys/sysinfo.h>

#include "mqttclient.h"

/* ==================================================================================== */
/* ==================================================================================== */

/* ******************************** GLOBAL VARIABLES ********************************** */
/* Externs */
extern mqttbenchInfo_t *g_MqttbenchInfo;
extern double g_MsgRate;
extern int g_MBTraceLevel;

/* ******************************** GLOBAL VARIABLES ********************************** */
/* Initial declaration */
double g_BaseTime = 0;            /* time in nanoseconds */
clockid_t clock_id;               /* clock id */
static double CLOCK_FREQ;


/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * Functions used during initialization.
 *
 * ==================================================================================== */

/* *************************************************************************************
 * initMBThreadInfo
 *
 * Description:  This will allocate a mbThreadInfo_t structure which will contain the
 *               number of thread types (e.g. Consumers Only, Producers Only or Dual
 *               Type Clients).  It will call the doDetermineNumClientTypeThreads to
 *               obtain the current type of thread.
 *
 * Note:         This can be invoked 2 ways in order to handle the race condition of
 *               where the actual distribution of clients could potentially only have
 *               consumers or producers even though it is declared a DUAL_CLIENT Thread.
 *
 *   @param[in]  obtainThreadType    = x
 *   @param[in]  mqttbenchInfo       = x
 *   @param[out] mbThreadInfo_t      = Pointer to a ThreadInfo Structure.
 * *************************************************************************************/
mbThreadInfo_t * initMBThreadInfo (int obtainThreadType, mqttbenchInfo_t *mqttbenchInfo)
{
	int rc;
	mbThreadInfo_t *pThreadInfo = NULL;

	pThreadInfo = (mbThreadInfo_t *)calloc(1, sizeof(mbThreadInfo_t));
	if (pThreadInfo) {
		if (obtainThreadType != INIT_STRUCTURE_ONLY) {
			rc = doDetermineNumClientTypeThreads(obtainThreadType,
					                             mqttbenchInfo->mbCmdLineArgs->numSubmitterThreads,
					                             mqttbenchInfo->mbInstArray,
												 pThreadInfo);
			if (rc) {
				free(pThreadInfo);
				pThreadInfo = NULL;
			}
		}
	} else
		rc = provideAllocErrorMsg("the thread info structure", (int)sizeof(mbThreadInfo_t), __FILE__, __LINE__);

	return pThreadInfo;
} /* initMBThreadInfo */

/* *************************************************************************************
 * setMBConfig
 *
 * Description:  This initializes the config file to be used for tracing.
 *
 *   @param[in]  prop                = x
 *   @param[in]  val                 = x
 * *************************************************************************************/
void setMBConfig (char *prop, char *val) {
	ism_field_t  f;

	f.type = VT_String;

	f.val.s = val;

	ism_common_setProperty(ism_common_getConfigProperties(), prop, &f);
} /* setMqttbenchConfig */

/* *************************************************************************************
 * createMQTTBench
 *
 * Description:  Allocate the memory for a mqttbench_t and initialize some fields.
 *
 *   @param[out] mqttbench_t         = An instance of mqttbench_t is created and returned
 *                                     to be populated with a thread's settings.
 * *************************************************************************************/
mqttbench_t * createMQTTBench (void)
{
	xUNUSED int rc = 0;
	mqttbench_t *mqttbench_new = NULL;

	mqttbench_new = (mqttbench_t *)calloc(1, sizeof(mqttbench_t));
	if (mqttbench_new == NULL) {
		rc = provideAllocErrorMsg("mqttbench", (int)sizeof(mqttbench_t), __FILE__, __LINE__);
	}
	return mqttbench_new;
} /* createMQTTBench */

/* *************************************************************************************
 * readLTPAToken
 *
 * Description:  Read in the LTPAToken into the LTPATokenData field of the System
 *               Environment structure.
 *
 *   @param[in]  pSysEnv             = Environment Variable structure
 *   @param[in]  ltpaTokenFile       = LTPA Token file to be used
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int readLTPAToken (environmentSet_t *pSysEnv, char *ltpaTokenFile)
{
	int rc = RC_FILEIO_ERR;
	int bytesRead;
	int fSize = 0;

	char *tokenData;
	struct stat fileInfo;

	FILE *fp;

	/* Read the size of the file. */
	rc = stat(ltpaTokenFile, &fileInfo);
	if (rc == 0) {
		fSize = fileInfo.st_size;
		/* Allocate a string to read in the token data */
		tokenData = calloc(fSize, sizeof(char));
		if (tokenData) {
			/* The password field will contain the token data so need to allocate
			 * memory for it here. */
			pSysEnv->LTPAToken = calloc(fSize, sizeof(char));
			if (pSysEnv->LTPAToken) {
				/* --------------------------------------------------------------------
				 * Open the token data file.  If successful then read the data into the
				 * tokenData variable and close the file. memcpy the data to the
				 * password field of the System Environment variables.
				 * -------------------------------------------------------------------- */
				fp = fopen(ltpaTokenFile, "r");
				if (fp) {
					bytesRead = fread(tokenData, sizeof(char), fSize, fp);
					fclose(fp);

					if (bytesRead > 0) {
						if (bytesRead == fSize)
							memcpy(pSysEnv->LTPAToken, tokenData, (int)fSize);
						else {
							MBTRACE(MBERROR, 1, "# of bytes read (%d) from file doesn't match expected size (%d).\n",
									            bytesRead,
								                fSize);
						}
					} else
						MBTRACE(MBERROR, 1, "Unable to read file: %s\n", ltpaTokenFile);
				} else
					MBTRACE(MBERROR, 1, "Unable to open LTPA Token file: %s\n", ltpaTokenFile);
			} else
		    	rc = provideAllocErrorMsg("client password", fSize, __FILE__, __LINE__);

			free(tokenData);
		} else
	    	rc = provideAllocErrorMsg("LTPA Token", fSize, __FILE__, __LINE__);
	} else
		MBTRACE(MBERROR, 1, "LTPA Token file doesn't exist (%s).\n", ltpaTokenFile);

	return rc;
} /* readLTPAToken */

/* *************************************************************************************
 * doDetermineNumClientTypeThreads
 *
 * Description:  Determine the number of threads which are just pure producer, consumer
 *               and/or dual client threads.
 *
 * Note:         This information can and will change once the clients have been assigned
 *               to the actual threads.  It is possible that 1 thread could become all
 *               producers or consumers.
 *
 *   @param[in]  obtainType          = x
 *   @param[in]  numSubmitThrds      = Number of submitter threads
 *   @param[in]  mbInstArray         = Array of mqttbench_t which contains 1 for each
 *                                     submitter thread.
 *   @param[in]  pThrdInfo           = Pointer to the ThreadInfo structure.
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int doDetermineNumClientTypeThreads (int obtainType,
		                             int numSubmitThrds,
		                             mqttbench_t **mbInstArray,
									 mbThreadInfo_t *pThrdInfo)
{
	int rc = 0;
	int i;

	int numConsThrds = 0;
	int numProdThrds = 0;
	int numDualThrds = 0;

	/* ----------------------------------------------------------------------------
	 * Loop through each of the submitter threads and determine if there are just
	 * consumers, producers or both type of clients in order to have the correct
	 * number of thread types.
	 * ---------------------------------------------------------------------------- */
	for (i = 0; i < numSubmitThrds; i++) {
		if (mbInstArray[i]->clientType == CONSUMERS_ONLY)
			numConsThrds++;
		if (mbInstArray[i]->clientType == PRODUCERS_ONLY)
			numProdThrds++;
		if (mbInstArray[i]->clientType == CONTAINS_DUALCLIENTS)
			numDualThrds++;
	}

	/* Need to ensure that the number of threads is less than the max size of uint8_t */
	if ((numConsThrds < (MAX_UINT8 + 1)) &&
		(numProdThrds < (MAX_UINT8 + 1)) &&
		(numDualThrds < (MAX_UINT8 + 1)))
	{
		pThrdInfo->numConsThreads = (uint8_t) numConsThrds;
		pThrdInfo->numProdThreads = (uint8_t) numProdThrds;
		pThrdInfo->numDualThreads = (uint8_t) numDualThrds;
	} else {
		if (numConsThrds > MAX_UINT8)
			MBTRACE(MBERROR, 1, "The number of threads is too large (%d)\n", numConsThrds);
		if (numProdThrds > MAX_UINT8)
			MBTRACE(MBERROR, 1, "The number of threads is too large (%d)\n", numProdThrds);
		if (numDualThrds > MAX_UINT8)
			MBTRACE(MBERROR, 1, "The number of threads is too large (%d)\n", numDualThrds);
	}

	return rc;
} /* doDetermineNumClientTypeThreads */

/* *************************************************************************************
 * getAllThreadsQoS
 *
 * Description:  Create a QoS Mask for either RX or TX based on the QoS bitmask for all
 *               submitter threads.
 *
 *   @param[in]  clientType          = Client type (Consumer, Producer or Dual Client)
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *
 *   @return -1  = Failed to obtain memory needed for test (provideAllocErrorMsg)
 * *************************************************************************************/
int getAllThreadsQoS (int clientType, mqttbenchInfo_t *mqttbenchInfo)
{
	int rc = -1;
	int i;
	int qosBitMap = 0;
	int numSubmitThrds = mqttbenchInfo->mbCmdLineArgs->numSubmitterThreads;
	mqttbench_t **mbInstArray = mqttbenchInfo->mbInstArray;

	/* Loop through all the threads. */
	for ( i = 0 ; i < numSubmitThrds ; i++ ) {
		if (clientType == CONSUMER)
			qosBitMap |= mbInstArray[i]->rxQoSBitMap;
		else
			qosBitMap |= mbInstArray[i]->txQoSBitMap;
	}

	if (qosBitMap == 0)
		return rc;
	else
		return qosBitMap;
} /* getAllThreadsQoS */

/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * Source and Interface addresses
 *
 * ==================================================================================== */

/* *************************************************************************************
 * getListOfInterfaceIPs
 *
 * Description:  Get the source IPs that are valid for the current machine.
 *
 *   @param[out] numElems            = Number of Source IPs for the current machine.
 *   @param[out] validIPAddrs        = Array of valid IP Addresses for the current machine.
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
char ** getListOfInterfaceIPs (int *numElems)
{
	int rc = 0;
	int ctr = 0;
	int ipLen = 0;
	int family;

	char **validIPAddrs = NULL;

	struct ifaddrs *interfaceIPs = NULL;
	struct ifaddrs *ifa = NULL;
	struct sockaddr_in *sa = NULL;

	/* Obtain all the Interface IPs associated with this machine. */
	rc = getifaddrs(&interfaceIPs);
	if (rc == 0) {
		ifa = interfaceIPs;

		/* First need to determine how many valid entries there are for this machine. */
		while (ifa) {
			if (ifa->ifa_addr) {
				sa = (struct sockaddr_in *)ifa->ifa_addr;
				family = sa->sin_family;
				if (family == AF_INET)
					ctr++;
			}

			ifa = ifa->ifa_next;
		}

		/* Check to see if there are any INET4 IP Addresses */
		if (ctr > 0) {
			/* Allocate memory for the structure needed to hold the number of IP Addresses. */
			validIPAddrs = (char **)calloc(ctr, sizeof(char *));
			if (validIPAddrs) {
				/* Reset the pointer to the list back to the beginning */
				ifa = interfaceIPs;
				ctr = 0;

				while (ifa) {
					if (ifa->ifa_addr) {
						sa = (struct sockaddr_in *)ifa->ifa_addr;
						family = sa->sin_family;
						if (family == AF_INET) {
							ipLen = strlen(inet_ntoa(sa->sin_addr)) + 1;
							validIPAddrs[ctr] = calloc(ipLen, sizeof(char));
							if (validIPAddrs[ctr])
								strcpy(validIPAddrs[ctr++], inet_ntoa(sa->sin_addr));
							else {
								rc = provideAllocErrorMsg("mqttclient_t ClientID", ipLen, __FILE__, __LINE__);
								break;
							}
						}
					}

					ifa = ifa->ifa_next;
				}
			}
		}

		free(interfaceIPs);
	} else
		MBTRACE(MBERROR, 1, "Unable to determine valid Interface Addresses.\n");

	if (ctr > 0)
		*numElems = ctr;

	return validIPAddrs;
} /* getListOfInterfaceIPs */

/* *************************************************************************************
 * validateSrcIPs
 *
 * Description:  Validate the Source IPs that were provided by the Environment variables
 *               IMAServer or IMAPort.  This is done by checking the valid ipv4 interfaces
 *               discovered for this machine.  If there is one IP that is not correct
 *               then abort.
 *
 *   @param[in]  machineIPs          = x
 *   @param[in]  numMachineIPs       = x
 *   @param[in]  ptrSrcIPs           = x
 *   @param[in]  numSrcIPs2Test      = x
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int validateSrcIPs (char **machineIPs,
		            int numMachineIPs,
					char **ptrSrcIPs,
					int numSrcIPs2Test)
{
	int rc = 0;
	int i, j;
	char *testIP;

	uint8_t goodIP;   /* Flag indicating that the status of the IP (0=bad 1=good) */

	/* --------------------------------------------------------------------------------
	 * The outer for loop is the Source IPs provided via Environment variables and the
	 * inner for loop are the valid discovered ipv4 interfaces for the machine running
	 * on.
	 * -------------------------------------------------------------------------------- */
	for ( i = 0 ; i < numSrcIPs2Test ; i++ ) {
		goodIP = 0;
		testIP = ptrSrcIPs[i];

		for ( j = 0 ; j < numMachineIPs ; j++ ) {
			if (strcmp(machineIPs[j], testIP) == 0) {
				goodIP = 1;
				break;
			}
		}

		/* ----------------------------------------------------------------------------
		 * Check to see if the goodIP flag was set.  If not then provide message and
		 * get out.
		 * ---------------------------------------------------------------------------- */
		if (goodIP == 0) {
			MBTRACE(MBERROR, 1, "Source IP: %s is not a valid IPv4 address on this machine.\n", testIP);
			rc = RC_INVALID_IPV4_ADDRESS;
		}
	}

	/* If rc == 0 then all the Source IPs are good.   Provide a message. */
	if (rc == 0)
		MBTRACE(MBINFO, 1, "All Source IPs have been successfully validated.\n");

	return rc;
} /* validateSrcIPs */



/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * Display information about environment and command line parameters
 *
 * ==================================================================================== */

/* *************************************************************************************
 * convertBitMaskToString
 *
 * Description:  Convert BitMask to a character string in order to log information
 *               pertaining to invocation of mqttbench.  Used for debugging purposes.
 *
 *   @param[out] strNum              = Character string representing the bitMask
 *   @param[in]  bitmask             = BitMask to be convered
 *   @param[in]  maskLen             = Length of bitMask in term of bits
 * *************************************************************************************/
void convertBitMaskToString (char *strNum, int bitMask, int maskLen)
{
	int i;
	int mask = 0x01;

	char tmp[16];

	/* Flags */
	uint8_t prev = 0;

	for ( i = 0 ; i < maskLen ; mask <<= 1, i++ ) {
		if ((mask & bitMask) > 0) {
			if (prev == 1) {
				sprintf(tmp, ", %d", i);
				strcat(strNum, tmp);
			} else {
				sprintf(strNum, "%d", i);
				prev = 1;
			}
		}
	}
} /* convertBitMaskToString */

/* *************************************************************************************
 * putDataLocation
 *
 * Description:  Put the data provided in the location specified (log file and/or to
 *               the Display = StdOut).
 *
 *   @param[in]  actionType			 = write to LOG, CONSOLE, or both
 *   @param[in]  dataString          = Contents of what to display
 *
 * *************************************************************************************/
void putDataLocation (int actionType, char * dataString)
{
	if ((actionType == PLACE_IN_TRACE) || (actionType == LOG2TRACE_AND_DISPLAY)) {
		MBTRACE(MBINFO, 1, "%s\n", dataString);
		if (actionType == LOG2TRACE_AND_DISPLAY) {
			fprintf(stdout, "%s\n", dataString);
			fflush(stdout);
		}
	} else if (actionType == DISPLAY_STDOUT) {
		fprintf(stdout, "%s\n", dataString);
		fflush(stdout);
	}

	dataString[0] = '\0';   /* Reset the string to NULL so that it can be reused. */
} /* putDataLocation */

/* *************************************************************************************
 * provideEnvInfo
 *
 * Description:  Log information pertaining to invocation into the log file for debugging
 *               purposes.
 *   @param[in]  runDataType         = Provide data based on passed in value.
 *   @param[in]  actionType			 = write to LOG, CONSOLE, or both
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 * *************************************************************************************/
void provideEnvInfo (int runDataType, int actionType, mqttbenchInfo_t *pMBInfo)
{
	int latencyMask = 0;
	int dataDestination = 0;

	char infoLine[MAX_LOGDATA_LINE] = {0};
	char tmpLine[MAX_LOGDATA_LINE/2] = {0};

    char *cCERTSIZE = NULL;

    environmentSet_t *pSysEnvSet = NULL;
    mbCmdLineArgs_t *pCmdLineArgs = NULL;
    mbSSLBufferInfo_t *pSSLBfr = NULL;
    mbMsgConfigInfo_t *pMsgCfg = NULL;
    mbBurstInfo_t *pBurstInfo = NULL;

	/* Flags */
	int8_t useSecure;
	int8_t useWebSockets;

	int8_t displayGenEnv = 1;
	int8_t displayCmdline = 0;

	uint8_t comma = 0;

	/* Ensure that mqttbenchInfo is not NULL. If not NULL then set other local pointers. */
	if (pMBInfo == NULL) {
		MBTRACE(MBERROR, 1, "Unable to determine information about this instance of mqttbench.\n");
		exit(RC_UNDETERMINISTIC_DATA);
	}

	pSysEnvSet = pMBInfo->mbSysEnvSet;
	pCmdLineArgs = pMBInfo->mbCmdLineArgs;
	pSSLBfr = pMBInfo->mbSSLBfrEnv;
	pMsgCfg = pMBInfo->mbMsgCfgInfo;
	useSecure = pMBInfo->useSecureConnections;
	useWebSockets = pMBInfo->useWebSockets;
	latencyMask = pCmdLineArgs->latencyMask;

	if (pMBInfo->burstModeEnabled)
		pBurstInfo = pMBInfo->mbBurstInfo;

    cCERTSIZE = getenv("CERTSIZE");

    /* --------------------------------------------------------------------------------
     * Determine what type of run data and format to display the data.  This is used
     * to provide some environment and startup information for debugging purposes.
     * The data will automatically be logged to the trace log (mqttbench_trace.log).
     * While using the mqttbench interactive command prompt this information can be
     * displayed by using the:  env  command.
     *
     * Set the destination locale based on the action type specified.
     *
     * RunDataType         ActionType       Output Destination
     * ----------------    -------------    --------------------------------------
     * GENERALENV          LOGONLY          trace log
     *                     LOGANDDISPLAY    trace log and stdout
     *
     * INTERACTIVESHELL    DISPLAYONLY      stdout ; env entered from doCommand
     * -------------------------------------------------------------------------------- */
    if (runDataType == GENERALENV) {
    	if (actionType == LOGANDDISPLAY)
    		dataDestination = LOG2TRACE_AND_DISPLAY;
    	else
    		dataDestination = PLACE_IN_TRACE;
    } else if (runDataType == INTERACTIVESHELL) {
    	dataDestination = DISPLAY_STDOUT;
    	displayCmdline = 1;
    }

    /* Starting here put everything into the Trace Log. */
   	strcpy(infoLine, "Command line invocation:");
   	putDataLocation(dataDestination, infoLine);

   	sprintf(infoLine, "\t%s", pCmdLineArgs->stringCmdLineArgs);
   	putDataLocation(dataDestination, infoLine);

   	if (displayCmdline)
   		fprintf(stdout, "\n");

   	/* ----------------------------------------------------------------------------
   	 * General Information which includes Max Inflight Msgs, Round Robin Info,
   	 * number of client threads, number of I/O threads and test duration.
   	 * ---------------------------------------------------------------------------- */
   	strcpy(infoLine, "General Information:");
   	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\tSystem Memory          : %llu (MB)", (ULL)pSysEnvSet->totalSystemMemoryMB);
   	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\tAssigned CPU           : %u", pSysEnvSet->assignedCPU);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tSMT                    : %u", pSysEnvSet->numSMT);
	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\tdestTuple_t size       : %0.3f (KB)", (double) sizeof(destTuple_t) / 1024);
   	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\tmqttclient_t size      : %0.3f (KB)", (double) sizeof(mqttclient_t) / 1024);
   	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\tMqttbench Instance ID  : %d ", pCmdLineArgs->mqttbenchID);
   	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\tMax Inflight Msgs      : %d", pCmdLineArgs->maxInflightMsgs);
   	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\tClient Processing      : %s First", (pCmdLineArgs->roundRobinSendSub ? "Breadth" : "Depth"));
   	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\t# Submitter Threads    : %d", pCmdLineArgs->numSubmitterThreads);
   	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\t# I/O Threads          : Proc: %u  Listener: %u",
				  	  pSysEnvSet->numIOProcThreads,
					  pSysEnvSet->numIOListenerThreads);
   	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\tTest Duration          : %d", pCmdLineArgs->testDuration);
   	putDataLocation(dataDestination, infoLine);

   	/* Check to see if using quit parameter with duration > 0 */
   	if (pCmdLineArgs->testDuration > 0) {
   	   	sprintf(infoLine, "\tClient Termination     : %s",
   	   		(pCmdLineArgs->disconnectType == 0 ? "UnSubscribe with Disconnect" : "Disconnect Only"));
   		putDataLocation(dataDestination, infoLine);
   	}

   	/* If displaying to command line provide a blank line. */
   	if (displayCmdline)
   		fprintf(stdout, "\n");

   	/* Client List information if available. */
   	strcpy(infoLine, "Client List Information:");
   	putDataLocation(dataDestination, infoLine);
   	sprintf(infoLine, "\t# Entries              : %d", pMBInfo->clientListNumEntries);
   	putDataLocation(dataDestination, infoLine);
   	if (pMBInfo->mbCmdLineArgs->crange != NULL) {
   		sprintf(infoLine, "\tClient Range           : %s", pMBInfo->mbCmdLineArgs->crange);
   		putDataLocation(dataDestination, infoLine);
   	}

   	/* If displaying to command line provide a blank line. */
   	if (displayCmdline)
   		fprintf(stdout, "\n");

   	/* ----------------------------------------------------------------------------
   	 * If there are Producers present then provide information about Message Size,
   	 * Message Rate, and Rate Control.
   	 * ---------------------------------------------------------------------------- */
	if ((pMBInfo->instanceClientType & CONTAINS_PRODUCERS) > 0) {
		strcpy(infoLine, "Producer Information:");
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tSend Msg Rate          : %d", (int)g_MsgRate);
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tMsg Size               : %d-%d (Bytes)", pMsgCfg->minMsg, pMsgCfg->maxMsg);
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tRateControl            : %d", pCmdLineArgs->rateControl);
		putDataLocation(dataDestination, infoLine);

		if (pMBInfo->mbCmdLineArgs->globalMsgPubCount > 0) {
			sprintf(infoLine, "\tSend Msg Count         : %d", pCmdLineArgs->globalMsgPubCount);
			putDataLocation(dataDestination, infoLine);
		}

		if (pBurstInfo) {
			sprintf(infoLine, "\tBurst Mode Information:");
			putDataLocation(dataDestination, infoLine);
			sprintf(infoLine, "\t   int,dur,rate        : %llu %llu %.1f",
							  (ULL)(pBurstInfo->burstInterval / NANO_PER_SECOND),
							  (ULL)(pBurstInfo->burstDuration /NANO_PER_SECOND),
							  pBurstInfo->burstMsgRate);
			putDataLocation(dataDestination, infoLine);
		}

		if (displayCmdline)
			fprintf(stdout, "\n");
	}

   	/* Snap Information if available */
   	if ((pMBInfo) && (pCmdLineArgs->snapInterval > 0)) {
   		tmpLine[0] = '\0';
   		if (displayGenEnv) {
   			strcpy(infoLine, "Snapshot Information:");
   			putDataLocation(dataDestination, infoLine);
   			sprintf(infoLine, "\tInterval               : %d (secs)", pCmdLineArgs->snapInterval);
   			putDataLocation(dataDestination, infoLine);
   		}

   		/* Create output line for snap statistics. */
   		if ((pCmdLineArgs->snapStatTypes & (SNAP_COUNT | SNAP_RATE | SNAP_CONNECT)) > 0) {
   			if ((pCmdLineArgs->snapStatTypes & SNAP_CONNECT) > 0) {
    			if ((pCmdLineArgs->snapStatTypes & SNAP_RATE) > 0) {
    				if ((pCmdLineArgs->snapStatTypes & SNAP_COUNT) > 0)
    					strcpy(tmpLine, "Connections, Message Rates, Message Counts");
    				else
    					strcpy(tmpLine, "Connections, Message Rates");
    			} else if ((pCmdLineArgs->snapStatTypes & SNAP_COUNT) > 0)
    				strcpy(tmpLine, "Connections, Message Counts");
    			else
    				strcpy(tmpLine, "Connections");
   			} else if ((pCmdLineArgs->snapStatTypes & SNAP_RATE) > 0) {
   				if ((pCmdLineArgs->snapStatTypes & SNAP_COUNT) > 0)
   					strcpy(tmpLine, "Message Rates, Message Counts");
   				else
   					strcpy(tmpLine, "Message Rates");
   			} else
   				strcpy(tmpLine, "Message Counts");

   			sprintf(infoLine, "\tStatistics             : %s", tmpLine);
			putDataLocation(dataDestination, infoLine);
   		} /* if ((pMBInfo->snapStatTypes & (SNAP_COUNT | SNAP_RATE | SNAP_CONNECT)) > 0) */

   		/* Create output line for snap latency. */
		if ((pCmdLineArgs->snapStatTypes & (SNAP_CONNECT_LATENCY | SNAP_LATENCY)) > 0) {
			strcpy(infoLine, "\tLatency                : ");
			if ((pCmdLineArgs->snapStatTypes & SNAP_LATENCY) > 0) {
				if ((pCmdLineArgs->snapStatTypes & SNAP_CONNECT_LATENCY) > 0)
					strcat(infoLine, "Connection, Message Latency");
				else
					strcat(infoLine, "Message");
			} else
				strcat(infoLine, "Connection");

			putDataLocation(dataDestination, infoLine);
   		} /* if ((pCmdLineArgs->snapStatTypes & (SNAP_CONNECT_LATENCY | SNAP_LATENCY)) > 0) */
	} /* if ((pMBInfo) && (pCmdLineArgs->snapInterval > 0)) */

	/* Latency Information if mask set. */
	if (latencyMask) {
		tmpLine[0] = '\0';
		strcpy(infoLine, "Latency Information:");
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tLatency Mask           : 0x%X", pCmdLineArgs->latencyMask);
		putDataLocation(dataDestination, infoLine);

		/* Provide more detail based on the Latency Mask. */
    	if ((latencyMask & CHKTIMERTT) > 0) {
        	if ((latencyMask & CHKTIMESEND) > 0)
        		strcat(tmpLine, " Round Trip, Send Call");
        	else
        		strcat(tmpLine, " Round Trip");

        	comma = 1;
        } else {
        	if ((latencyMask & CHKTIMESEND) > 0) {
        		strcat(tmpLine, " Send Call");
        		comma = 1;
        	}
    	}

    	if ((latencyMask & CHKTIMETCPCONN) > 0) {
    		if (comma == 1)
    			strcat(tmpLine, ",");
    		else
    			comma = 1;

    		if (useSecure) {
    			if (useWebSockets)
    				strcat(tmpLine, " TCP/SSL/WebSocket Connections");
    			else
    				strcat(tmpLine, " TCP/SSL Connections");
    		} else {
    			if (useWebSockets)
    				strcat(tmpLine, " TCP/WebSocket Connections");
    			else
    				strcat(tmpLine, " TCP Connections");
    		}
    	}

    	if ((latencyMask & CHKTIMEMQTTCONN) > 0) {
    		if (comma)
    			strcat(tmpLine, ", MQTT Connections");
    		else {
    			strcat(tmpLine, "MQTT Connections");
    			comma = 1;
    		}
    	}

    	if (((latencyMask & CHKTIMETCP2MQTTCONN) > 0) ||
    		((latencyMask & CHKTIMETCP2SUBSCRIBE) > 0)) {
    		if (comma)
    			strcat(tmpLine, ",");
    		else
    			comma = 1;

    		if (useSecure) {
    			if (useWebSockets) {
    				if ((latencyMask & CHKTIMETCP2MQTTCONN) > 0)
    					strcat(tmpLine, " TCP/SSL/WebSocket - MQTT Connections");
					else
    					strcat(tmpLine, " TCP/SSL/WebSocket - Subscribe");
    			} else {
    				if ((latencyMask & CHKTIMETCP2MQTTCONN) > 0)
    					strcat(tmpLine, " TCP/SSL - MQTT Connections");
					else
    					strcat(tmpLine, " TCP/SSL - Subscribe");
    			}
    		} else {
    			if (useWebSockets) {
    				if ((latencyMask & CHKTIMETCP2MQTTCONN) > 0)
    					strcat(tmpLine, " TCP/WebSocket - MQTT Connections");
					else
    					strcat(tmpLine, " TCP/WebSocket - Subscribe");
    			} else {
    				if ((latencyMask & CHKTIMETCP2MQTTCONN) > 0)
    					strcat(tmpLine, " TCP - MQTT Connections");
					else
    					strcat(tmpLine, " TCP - Subscribe");
    			}
    		}
    	}

    	if ((latencyMask & CHKTIMESUBSCRIBE) > 0) {
    		if (comma)
    			strcat(tmpLine, ", Subscriptions");
    		else
    			strcat(tmpLine, " Subscriptions");
    	}

    	sprintf(infoLine, "\t   Expanded Details    : (%s )", tmpLine);
    	putDataLocation(dataDestination, infoLine);

    	if (pSysEnvSet->clkSrc == GTOD) {
    		strcpy(infoLine, "Using the Time-Of-Day Clock for latency.");
    		putDataLocation(dataDestination, infoLine);
    	}

		sprintf(infoLine, "\tSampleRate             : %d", pSysEnvSet->sampleRate);
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tConnHistogramSize      : %d", pSysEnvSet->maxConnHistogramSize);
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tMsgHistogramSize       : %d", pSysEnvSet->maxMsgHistogramSize);
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tReconnHistogramSize    : %d", pSysEnvSet->maxReconnHistogramSize);
		putDataLocation(dataDestination, infoLine);

	   	/* If displaying to command line provide a blank line. */
		if (displayCmdline)
			fprintf(stdout, "\n");
	} /* if (latencyMask) */

	/* Provide Environment Information */
	strcat(infoLine, "Environment variables:");
	putDataLocation(dataDestination, infoLine);

	/* Display and log all the environment variables that are set. */
	sprintf(infoLine, "\tSIPList                : %s", pSysEnvSet->strSIPList);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tALPNList               : %s", pSysEnvSet->ALPNList);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tSourcePortLo           : %d", pSysEnvSet->sourcePortLo);
	putDataLocation(dataDestination, infoLine);

	sprintf(infoLine, "\tBatchingDelay          : %d", pSysEnvSet->batchingDelay);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tDelayTime              : %d (usecs)", pSysEnvSet->delaySendTimeUsecs);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tDelayCount             : %d", pSysEnvSet->delaySendCount);
	putDataLocation(dataDestination, infoLine);

   	/* If displaying to command line provide a blank line. */
	if (displayCmdline)
		fprintf(stdout, "\n");

	sprintf(infoLine, "\tMQTTKeepAlive          : %d (secs)", pSysEnvSet->mqttKeepAlive);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tPipeCommands           : %u", pSysEnvSet->pipeCommands);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tReconnect              : %s", (pCmdLineArgs->reconnectEnabled ? "Enabled" : "Disabled"));
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tRetryDelay             : %d (usecs)", (pSysEnvSet->initConnRetryDelayTime_ns / (int)NANO_PER_MICRO));
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tRetryBackoff           : %f", pSysEnvSet->connRetryFactor);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tMaxRetryAttempts       : %d", pSysEnvSet->maxRetryAttempts);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tExitOnDisconnect       : %d", pCmdLineArgs->exitOnFirstDisconnect);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tUseNagle               : %u", pSysEnvSet->useNagle);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tAutoLog                : %u", pSysEnvSet->autoRenameFiles);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tTraceLevel             : %u", pCmdLineArgs->traceLevel);
	putDataLocation(dataDestination, infoLine);

	if (pSysEnvSet->memTrimSecs > 0) {
		sprintf(infoLine, "\tMem Trim Interval      : %d (secs)", pSysEnvSet->memTrimSecs);
		putDataLocation(dataDestination, infoLine);
	}

	/* Display information about the clock source */
	if (pSysEnvSet->clkSrc == TSC)
		strcpy(infoLine, "\tClockSrc               : TSC");
	else if (pSysEnvSet->clkSrc == GTOD)
		strcpy(infoLine, "\tClockSrc               : GTOD");
	else
		strcpy(infoLine, "\tClockSrc               : Undefined");

	putDataLocation(dataDestination, infoLine);

	sprintf(infoLine, "\tUseEphemeralPorts      : %u", pSysEnvSet->useEphemeralPorts);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tGraphiteIP             : %s", pSysEnvSet->GraphiteIP);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tGraphitePort           : %d", pSysEnvSet->GraphitePort);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tGraphiteMetricRoot     : %s", pSysEnvSet->GraphiteMetricRoot);
	putDataLocation(dataDestination, infoLine);

	/* If interactive command line then add a space. */
	if (displayCmdline)
		fprintf(stdout, "\n");

	/* Provide Security Information if security enabled. */
	if (pMBInfo->useSecureConnections) {
		strcpy(infoLine, "Security Enabled (1 or more clients):");
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tSSLCipher");
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\t   Requested/Def       : %s", pSysEnvSet->sslCipher);
		putDataLocation(dataDestination, infoLine);

		sprintf(infoLine, "\t   Negotiated          : %s", pSysEnvSet->negotiated_sslCipher);
		putDataLocation(dataDestination, infoLine);


		sprintf(infoLine, "\tClient Method          : %s", pSysEnvSet->sslClientMethod);
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tCertSize               : %s", cCERTSIZE);
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tUse TLS Bfr Pool       : %s", (pSysEnvSet->mqttbenchTLSBfrPool == 0 ? "N" : "Y"));
		putDataLocation(dataDestination, infoLine);
		sprintf(infoLine, "\tTLS Bfr Pool Size      : %u %s",
				          (pSSLBfr->sslBufferPoolMemory == 0 ? pSSLBfr->pool64B_numBfrs : pSSLBfr->sslBufferPoolMemory),
					      (pSSLBfr->sslBufferPoolMemory == 0 ? "(Based on System Memory)"    : ""));
		putDataLocation(dataDestination, infoLine);

		/* Display information about the SSL Buffer Pool. */
		if (pSysEnvSet->mqttbenchTLSBfrPool > 0) {
			sprintf(infoLine, "\t     Buffer Sizes     # Bfrs     Total Bytes");
			putDataLocation(dataDestination, infoLine);
			sprintf(infoLine, "\t     ============     ======     ===========");
			putDataLocation(dataDestination, infoLine);
			sprintf(infoLine, "\t       o  64 Byte      %12d       %16llu", pSSLBfr->pool64B_numBfrs,  (unsigned long long) pSSLBfr->pool64B_totalSize);
			putDataLocation(dataDestination, infoLine);
			sprintf(infoLine, "\t       o 128 Byte      %12d       %16llu", pSSLBfr->pool128B_numBfrs, (unsigned long long) pSSLBfr->pool128B_totalSize);
			putDataLocation(dataDestination, infoLine);
			sprintf(infoLine, "\t       o 256 Byte      %12d       %16llu", pSSLBfr->pool256B_numBfrs, (unsigned long long) pSSLBfr->pool256B_totalSize);
			putDataLocation(dataDestination, infoLine);
			sprintf(infoLine, "\t       o 512 Byte      %12d       %16llu", pSSLBfr->pool512B_numBfrs, (unsigned long long) pSSLBfr->pool512B_totalSize);
			putDataLocation(dataDestination, infoLine);
			sprintf(infoLine, "\t       o  1K Byte      %21d       %16llu", pSSLBfr->pool1KB_numBfrs,  (unsigned long long) pSSLBfr->pool1KB_totalSize);
			putDataLocation(dataDestination, infoLine);
			sprintf(infoLine, "\t       o  2K Byte      %12d       %16llu", pSSLBfr->pool2KB_numBfrs,  (unsigned long long) pSSLBfr->pool2KB_totalSize);
			putDataLocation(dataDestination, infoLine);
		}

		/* If interactive command line then add a space. */
		if (displayCmdline)
			fprintf(stdout, "\n");
	}

	strcpy(infoLine, "Buffer Information:");
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tReceive Buffer Size    : %d", pSysEnvSet->recvBufferSize);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tSend Buffer Size       : %d", pSysEnvSet->sendBufferSize);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tReceive Socket Size    : %d", pSysEnvSet->recvSockBuffer);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tSend Socket Size       : %d", pSysEnvSet->sendSockBuffer);
	putDataLocation(dataDestination, infoLine);
	sprintf(infoLine, "\tBuffers/Request        : %d", pSysEnvSet->buffersPerReq);
	putDataLocation(dataDestination, infoLine);

	if (pSysEnvSet->maxNumTXBfrs > 0) {
		sprintf(infoLine, "\tMax # of Tx Buffers    : %d\n", (int)pSysEnvSet->maxNumTXBfrs);
		putDataLocation(dataDestination, infoLine);
	}

	/* Provide additional information on the current run. */
	if ((displayGenEnv == 1) && (displayCmdline == 0)) {
		if (pMBInfo->useWebSockets) {
			strcpy(infoLine, "Using WebSockets Protocols (1 or more clients)");
			putDataLocation(dataDestination, infoLine);
		}

		/* Provide information on -as option. */
		if (pCmdLineArgs->appSimLoop) {
			sprintf(infoLine, "Using Simulation Loop    : %d", pCmdLineArgs->appSimLoop);
			putDataLocation(dataDestination, infoLine);
		}

    	/* Display WebSocket Information if set in the client list. */
    	if (useWebSockets) {
    		strcpy(infoLine, "Using WebSockets Protocol (1 or more clients).");
			putDataLocation(dataDestination, infoLine);
    	}

    	/* Display message if using reconnection when disconnected. */
    	if (pCmdLineArgs->reconnectEnabled) {
    		if (pCmdLineArgs->reresolveDNS == 1)
    			strcpy(infoLine, "Client reconnect is enabled with re-resolve DNS (-nodnscache).");
    		else
    			strcpy(infoLine, "Client reconnect is enabled.");
    	} else
        	strcpy(infoLine, "Client reconnect is disabled.");

    	putDataLocation(dataDestination, infoLine);

    	/* Display message if exiting on first disconnect (-efd option). */
    	if (pCmdLineArgs->exitOnFirstDisconnect) {
    		strcpy(infoLine, "Exiting on first disconnect.");
			putDataLocation(dataDestination, infoLine);
    	}

    	/* Display message if performing malloc_trim. */
    	if (pSysEnvSet->memTrimSecs) {
    		sprintf(infoLine, "Performing memory cleanup at %d secs interval.", pSysEnvSet->memTrimSecs);
			putDataLocation(dataDestination, infoLine);
    	}

    	/* Display message if gathering Jitter Statistics. */
    	if (pCmdLineArgs->determineJitter) {
    		strcpy(infoLine, "Obtaining Jitter Statistics.");
    		putDataLocation(dataDestination, infoLine);
    	}

    	/* Display message if using A Simulation (-as) loop. */
    	if (pCmdLineArgs->appSimLoop) {
    		sprintf(infoLine, "Using simulation loop with count: %d.", pCmdLineArgs->appSimLoop);
			putDataLocation(dataDestination, infoLine);
    	}

        /* If the user requested -V then provide what checking is being performed. */
        if (pCmdLineArgs->chkMsgMask > 0) {
        	if (((pCmdLineArgs->chkMsgMask & CHKMSGLENGTH) > 0) &&
        		((pCmdLineArgs->chkMsgMask & CHKMSGSEQNUM) > 0))
        		sprintf(infoLine, "Performing Message Length checking and Sequence Number checking.");
        	else if ((pCmdLineArgs->chkMsgMask & CHKMSGLENGTH) > 0)
        		sprintf(infoLine, "Performing Message Length checking.");
        	else
        		sprintf(infoLine, "Performing Sequence Number checking.");
        }

        /* If the user requested -dcl (dump client list) now is the time to perform it. */
        if (pCmdLineArgs->dumpClientInfo == 1)
       		doDumpClientInfo(g_MqttbenchInfo);
	} /* if (displayGenEnv) */

	fflush(stdout);
} /* provideEnvInfo */

/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * Histogram Related - Latency
 *
 * ==================================================================================== */

/* *************************************************************************************
 * prep2PrintHistogram
 *
 * Description:  Prepare histogram file so it can be written out to.
 *
 *   @param[out] fpHgram             = Array of File Descriptors for the histogram files.
 *   @param[in]  latencyFile         = Name of file used for the latency output.
 *   @param[in]  latencyMask         = Latency Mask which provides what type of latency
 *                                     is being collected.
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int prep2PrintHistogram (FILE **fpHgram, char *latencyFile, int latencyMask)
{
	int i;
	int rc = 0;
	int ctr = 0;
	int loc = 0;

	char *ptr = NULL;
    char  hgramFile[MAX_HISTOGRAM_NAME_LEN] = {0};
	char  pfx[2] = {0};

    if (latencyFile) {
    	if ((latencyMask & LAT_CONN_AND_SUBSCRIBE) > 0) {
    		if ((latencyMask & CHKTIMETCPCONN) > 0)
    			ctr++;
    		if ((latencyMask & (CHKTIMEMQTTCONN | CHKTIMETCP2MQTTCONN)) > 0)
    			ctr++;
    		if ((latencyMask & (CHKTIMESUBSCRIBE | CHKTIMETCP2SUBSCRIBE)) > 0)
    			ctr++;
    	}

    	if (((latencyMask & CHKTIMERTT) > 0) || ((latencyMask & CHKTIMESEND) > 0))
    		ctr++;

    	/* Create the histogram file for writing */
    	for ( i = 0 ; i < ctr ; i++ ) {
    		loc = 0;
    		memset(hgramFile, 0, MAX_HISTOGRAM_NAME_LEN);

    		if ((strlen(latencyFile) + strlen("_hgram") + 1) < MAX_HISTOGRAM_NAME_LEN) {
    			ptr = strrchr(latencyFile, '.');
    			if (ptr) {
    				loc = ptr - latencyFile;
    				strncpy(hgramFile, latencyFile, loc);
    			} else
    				strcpy(hgramFile, latencyFile);

    			strcat(hgramFile, "_hgram");
    			if (i > 0) {
    				pfx[0] = (char)(i + 0x30);
    				pfx[1] = '\0';
    				strcat(hgramFile, pfx);
    			}

    			if (loc)
    				strcat(hgramFile, ptr);
    		} else {
    			MBTRACE(MBERROR, 1, "Histogram filename exceeds max supported length (%d).\n", MAX_HISTOGRAM_NAME_LEN);
    			rc = RC_FILENAME_ERR;
    			break;
    		}

			fpHgram[i] = fopen(hgramFile, "w");
			if (fpHgram[i] == NULL) {
				MBTRACE(MBERROR, 1, "Unable to open the file (%s) for writing, check file permissions.\n", hgramFile);
				rc = RC_FILEIO_ERR;
				break;
			}
		}
    } else {
		MBTRACE(MBERROR, 1, "Histogram filename is NULL.\n");
		rc = RC_FILENAME_ERR;
    }

	return rc;
} /* prep2PrintHistogram */

/* *************************************************************************************
 * setupIOPThreadHistograms
 *
 * Description:  Set up the IOP thread histograms based on the value of the -T command
 *               line parameter.
 *
 *   @param[in]  iop                 = Particular IOP thread to create histograms
 *   @param[in]  pSysEnvSet          = Pointer to the environment variable structure
 *   @param[in]  mqttClientType      = Type of clients for the latency
 *   @param[in]  latencyMask         = Latency Mask which provides what type of latency
 *                                     is being collected.
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int setupIOPThreadHistograms (ioProcThread_t *iop,
                              environmentSet_t *pSysEnvSet,
							  latencyUnits_t *pLatUnits,
		                      int mqttClientType,
							  int latencyMask)
{
	int rc = 0;
	int i, j;
	int mask = 0x1;
	int currLatency = UNKNOWN_LATENCY;
	int maxSize;

	char histType[MAX_HGRAM_TITLE] = {0};
	char errString[MAX_ERROR_STRING] = {0};

	uint8_t mqttConnLat = 0;
	uint8_t subConnLat = 0;
	uint8_t numPasses = 1;

	latencystat_t * latHist = NULL;

	for ( i = 0, mask = 0x1 ; i < LAT_MASK_SIZE ; mask <<= 1, i++ ) {
		if ((mask & latencyMask) > 0) {
			currLatency = UNKNOWN_LATENCY;
			numPasses = 1;
			histType[0] = 0;
			maxSize = pSysEnvSet->maxConnHistogramSize;

			switch(mask) {
				case CHKTIMERTT:
					if (mqttClientType == CONSUMER) {
						currLatency = RTT_LATENCY;
						maxSize = pSysEnvSet->maxMsgHistogramSize;
						strcpy(histType, "message");
						numPasses = 2;
					} else
						continue;
					break;
				case CHKTIMESEND:
					if (mqttClientType == PRODUCER) {
						currLatency = SEND_LATENCY;
						strcpy(histType, "send");
					} else
						continue;
					break;
				case CHKTIMETCPCONN:
					currLatency = TCPCONN_LATENCY;
					strcpy(histType, "TCP connection");
					break;
				case CHKTIMEMQTTCONN:
				case CHKTIMETCP2MQTTCONN:
					if (mqttConnLat++ == 0) {
						currLatency = MQTTCONN_LATENCY;
						strcpy(histType, "mqtt connection");
					} else
						continue;
					break;
				case CHKTIMESUBSCRIBE:
				case CHKTIMETCP2SUBSCRIBE:
					if (subConnLat++ == 0) {
						currLatency = SUB_LATENCY;
						strcpy(histType, "subscribe");
					} else
						continue;
					break;
				default:
					continue;
					break;
			} /* switch(mask)h */

			/* ------------------------------------------------------------------------
			 * Only if RTT message latency is being captured is there a 2nd pass needed
			 * in order to filter out the latency that is due to a reboot, deploy which
			 * results in the client trying to reconnect.
			 *
			 * Hence the 2nd pass for RTT is the creation of the Reconnect Histogram.
			 * ------------------------------------------------------------------------ */
			for ( j = 0 ; j < numPasses ; j++ ) {
				if ((currLatency == RTT_LATENCY) && (j == 1)) {
					latHist = iop->reconnectMsgHist;
					maxSize = pSysEnvSet->maxReconnHistogramSize;
					histType[0] = 0;
					strcpy(histType, "reconnect");
				}

				latHist = calloc(1, sizeof(latencystat_t));
				if (latHist) {
					latHist->histogram = calloc(maxSize, sizeof(uint32_t));
					if (latHist->histogram) {
						latHist->histSize = maxSize;

						/* ------------------------------------------------------------
						 * Since was successfully able to create the histogram now need
						 * to update the iop's ptr to this particular histogram.
						 * ------------------------------------------------------------ */
						switch (currLatency) {
							case RTT_LATENCY:
								if (j == 0) {
									iop->rttHist = latHist;
									latHist->histUnits = pLatUnits->RTTUnits;
								} else {
									iop->reconnectMsgHist = latHist;
									latHist->histUnits = pLatUnits->ReconnRTTUnits;
								}
								break;
							case SEND_LATENCY:
								iop->sendHist = latHist;
								latHist->histUnits = pLatUnits->RTTUnits;
								break;
							case TCPCONN_LATENCY:
								iop->tcpConnHist = latHist;
								latHist->histUnits = pLatUnits->ConnUnits;
								break;
							case MQTTCONN_LATENCY:
								iop->mqttConnHist = latHist;
								latHist->histUnits = pLatUnits->ConnUnits;
								break;
							case SUB_LATENCY:
								iop->subscribeHist = latHist;
								latHist->histUnits = pLatUnits->ConnUnits;
								break;
						} /* switch (currLatency) */
					} else {
						free(latHist);
						snprintf(errString, MAX_ERROR_STRING, "the %s histogram\n", histType);
						rc = provideAllocErrorMsg(errString, (int)(maxSize * sizeof(uint32_t)), __FILE__, __LINE__);
						return rc;
					}
				} else {
					snprintf(errString, MAX_ERROR_STRING, "a ptr for the %s histogram\n", histType);
					rc = provideAllocErrorMsg(errString, (int)(sizeof(latencystat_t)), __FILE__, __LINE__);
					return rc;
				}
			} /* for ( j = 0 ; j < numPasses ; j++ ) */
		} /* if ((mask & latencyMask) > 0) */
	} /* for ( i = 0, mask = 0x1 ; i < LAT_MASK_SIZE ; mask <<= 1, i++ ) */

	return 0;
} /* setupIOPThreadHistograms */

/* *************************************************************************************
 * removeIOPThreadHistograms
 *
 * Description:  Remove the IOP Histograms
 *
 *   @param[in]  pSysEnvSet          = Pointer to the environment variable structure.
 *   @param[in]  ioProcThrds         = Array of IOP threads.
 * *************************************************************************************/
void removeIOPThreadHistograms (environmentSet_t *pSysEnvSet, ioProcThread_t **ioProcThrds)
{
	int i;
	ioProcThread_t *iop = NULL;

	for ( i = 0 ; i < pSysEnvSet->numIOProcThreads ; i++ ) {
		iop = ioProcThrds[i];

		/* Check for Round Trip Histogram. */
		if (iop->rttHist) {
			if (iop->rttHist->histogram)
				free(iop->rttHist->histogram);

			free(iop->rttHist);
			iop->rttHist = NULL;
		}

		/* Check for Round Trip during Reconnects Histogram. */
		if (iop->reconnectMsgHist) {
			if (iop->reconnectMsgHist->histogram)
				free(iop->reconnectMsgHist->histogram);

			free(iop->reconnectMsgHist);
			iop->reconnectMsgHist = NULL;
		}

		/* Check for Send Call Histogram. */
		if (iop->sendHist) {
			if (iop->sendHist->histogram)
				free(iop->sendHist->histogram);

			free(iop->sendHist);
			iop->sendHist = NULL;
		}

		/* Check for TCP Connection Histogram. */
		if (iop->tcpConnHist) {
			if (iop->tcpConnHist->histogram)
				free(iop->tcpConnHist->histogram);

			free(iop->tcpConnHist);
			iop->tcpConnHist = NULL;
		}

		/* Check for MQTT Connection Histogram. */
		if (iop->mqttConnHist) {
			if (iop->mqttConnHist->histogram)
				free(iop->mqttConnHist->histogram);

			free(iop->mqttConnHist);
			iop->mqttConnHist = NULL;
		}

		/* Check for Subscribe Histogram. */
		if (iop->subscribeHist) {
			if (iop->subscribeHist->histogram)
				free(iop->subscribeHist->histogram);

			free(iop->subscribeHist);
			iop->subscribeHist = NULL;
		}
	}
} /* removeIOPThreadHistograms */



/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * Jitter Related
 *
 * ==================================================================================== */

/* *************************************************************************************
 * provisionJitterArrays
 *
 * Description:  Create or Re-allocate the Jitter arrays that are needed for holding the
 *               various throughput rates for the producers and consumers.   The arrays
 *               are based on the QoS.
 *
 *   @param[in]  allocType           = Type of allocation to make
 *                                        0 = CREATION
 *                                        1 = REALLOCATION
 *   @param[in]  mqttClientType      = Type of clients for the submitter threads.
 *   @param[in]  pJitterInfo         = Pointer to the JitterInfo structure.
 *   @param[in]  mbInstArray         = Array of all mqttbench_t instances.
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int provisionJitterArrays (int allocType, int mqttClientType, mbJitterInfo_t *pJitterInfo, mqttbench_t **mbInstArray)
{
	int rc = 0;
	int arraySize = pJitterInfo->sampleCt * sizeof(int);

	char errString[MAX_ERROR_STRING] = {0};

	uint8_t allocationFailed = 0;

	if (mbInstArray) {
		if ((mqttClientType & CONTAINS_CONSUMERS) > 0) {
			pJitterInfo->cons_PUBLISH_MinRate = 0x7FFFFFFF;
			pJitterInfo->cons_PUBACK_MinRate  = 0x7FFFFFFF;
			pJitterInfo->cons_PUBREC_MinRate  = 0x7FFFFFFF;
			pJitterInfo->cons_PUBREL_MinRate  = 0x7FFFFFFF;
			pJitterInfo->cons_PUBCOMP_MinRate = 0x7FFFFFFF;

			if (allocType == CREATION)
				pJitterInfo->rateArray_Cons_PUB = calloc(pJitterInfo->sampleCt, sizeof(int));
			else
				pJitterInfo->rateArray_Cons_PUB = realloc(pJitterInfo->rateArray_Cons_PUB, arraySize);

			if (pJitterInfo->rateArray_Cons_PUB) {
				if (allocType == CREATION)
					pJitterInfo->rateArray_Cons_PUBACK = calloc(pJitterInfo->sampleCt, sizeof(int));
				else
					pJitterInfo->rateArray_Cons_PUBACK = realloc(pJitterInfo->rateArray_Cons_PUBACK, arraySize);

				if (pJitterInfo->rateArray_Cons_PUBACK) {
					if (allocType == CREATION)
						pJitterInfo->rateArray_Cons_PUBREC = calloc(pJitterInfo->sampleCt, sizeof(int));
					else
						pJitterInfo->rateArray_Cons_PUBREC = realloc(pJitterInfo->rateArray_Cons_PUBREC, arraySize);

					if (pJitterInfo->rateArray_Cons_PUBREC) {
						if (allocType == CREATION)
							pJitterInfo->rateArray_Cons_PUBCOMP = calloc(pJitterInfo->sampleCt, sizeof(int));
						else
							pJitterInfo->rateArray_Cons_PUBCOMP = realloc(pJitterInfo->rateArray_Cons_PUBCOMP, arraySize);

						if (pJitterInfo->rateArray_Cons_PUBCOMP == NULL) {
							snprintf(errString, MAX_ERROR_STRING, "%sthe Consumer PUBCOMP Jitter Array",
									            (allocType == CREATION ? "" : "resizing "));
							allocationFailed = 1;
						}
					} else {
						snprintf(errString, MAX_ERROR_STRING, "%sthe Consumer PUBREC Jitter Array",
								            (allocType == CREATION ? "" : "resizing "));
						allocationFailed = 1;
					}
				} else {
					snprintf(errString, MAX_ERROR_STRING, "%sthe Consumer PUBACK Jitter Array",
							            (allocType == CREATION ? "" : "resizing "));
					allocationFailed = 1;
				}
			} else {
				snprintf(errString, MAX_ERROR_STRING, "%sthe Consumer PUBLISH Jitter Array",
						            (allocType == CREATION ? "" : "resizing "));
				allocationFailed = 1;
			}
		} /* if ((mqttClientType & CONSUMERS_ONLY) > 0) */

		if (allocationFailed)
			rc = provideAllocErrorMsg(errString, arraySize, __FILE__, __LINE__);

		if ((rc == 0) && ((mqttClientType & CONTAINS_PRODUCERS) > 0)) {
			pJitterInfo->prod_PUBLISH_MinRate = 0x7FFFFFFF;
			pJitterInfo->prod_PUBACK_MinRate  = 0x7FFFFFFF;
			pJitterInfo->prod_PUBREC_MinRate  = 0x7FFFFFFF;
			pJitterInfo->prod_PUBREL_MinRate  = 0x7FFFFFFF;
			pJitterInfo->prod_PUBCOMP_MinRate = 0x7FFFFFFF;

			if (allocType == CREATION)
				pJitterInfo->rateArray_Prod_PUB = calloc(pJitterInfo->sampleCt, sizeof(int));
			else
				pJitterInfo->rateArray_Prod_PUB = realloc(pJitterInfo->rateArray_Prod_PUB, arraySize);

			if (pJitterInfo->rateArray_Prod_PUB) {
				if (allocType == CREATION)
					pJitterInfo->rateArray_Prod_PUBACK = calloc(pJitterInfo->sampleCt, sizeof(int));
				else
					pJitterInfo->rateArray_Prod_PUBACK = realloc(pJitterInfo->rateArray_Prod_PUBACK, arraySize);

				if (pJitterInfo->rateArray_Prod_PUBACK) {
					if (allocType == CREATION)
						pJitterInfo->rateArray_Prod_PUBREC = calloc(pJitterInfo->sampleCt, sizeof(int));
					else
						pJitterInfo->rateArray_Prod_PUBREC = realloc(pJitterInfo->rateArray_Prod_PUBREC, arraySize);

					if (pJitterInfo->rateArray_Prod_PUBREC) {
						if (allocType == CREATION)
							pJitterInfo->rateArray_Prod_PUBCOMP = calloc(pJitterInfo->sampleCt, sizeof(int));
						else
							pJitterInfo->rateArray_Prod_PUBCOMP = realloc(pJitterInfo->rateArray_Prod_PUBCOMP, arraySize);

						if (pJitterInfo->rateArray_Prod_PUBCOMP == NULL) {
							snprintf(errString, MAX_ERROR_STRING,  "%sthe Producer PUBCOMP Jitter Array",
							                    (allocType == CREATION ? "" : "resizing " ));
							allocationFailed = 1;
						}
					} else {
						snprintf(errString, MAX_ERROR_STRING, "%sthe Producer PUBREC Jitter Array",
								            (allocType == CREATION ? "" : "resizing "));
						allocationFailed = 1;
					}
				} else {
					snprintf(errString, MAX_ERROR_STRING, "%sthe Producer PUBACK Jitter Array",
						                (allocType == CREATION ? "" : "resizing "));
					allocationFailed = 1;
				}
			} else {
				snprintf(errString, MAX_ERROR_STRING, "%sthe Producer PUBLISH Jitter Array",
						            (allocType == CREATION ? "" : "resizing "));
				allocationFailed = 1;
			}
		} /* if ((rc == 0) && ((mqttClientType & CONTAINS_PRODUCERS) > 0)) */

		if (allocationFailed)
			rc = provideAllocErrorMsg(errString, arraySize, __FILE__, __LINE__);
	}

	return rc;
} /* provisionJitterArrays */

/* *************************************************************************************
 * displayJitter
 *
 * Description:  Perofmr the displaying of the current Jitter information.
 *
 *   @param[in]  pJitterInfo         = Pointer to the JitterInfo structure.
 *   @param[in]  pArray              = References the Array for a particular type of data
 *                                     (i.e.  PUBLISH, PUBACK, PUBREC....etc).
 *   @param[in]  clntType            = The type of client (CONSUMER or PRODUCER)
 *   @param[in]  pType               = The PUBLISH type used to identify the Jitter array.
 * *************************************************************************************/
void displayJitter (mbJitterInfo_t *pJitterInfo, int *pArray, uint8_t clntType, uint8_t pType)
{
	int k;
	int currRate = 0;
	int minRate = 0;
	int maxRate = 0;
	int maxEntries;

	uint64_t aggrTotal;

	double aggrAverage = 0.0;
	double maxPercentDiff = 0.0;
	double minPercentDiff = 0.0;

	aggrTotal = 0;

	/* Determine the maximum number of entries in the array at this point. */
	if (pJitterInfo->wrapped == 1)
		maxEntries = pJitterInfo->sampleCt;
	else
		maxEntries = pJitterInfo->currIndex;

	/* Determine the current rate as well as the aggregate total for rates */
	for ( k = 0 ; k < maxEntries ; k++ ) {
		currRate = pArray[k];
		aggrTotal += currRate;
	}

	/* Set up the minRate and maxRate based on the client type. */
	switch (pType) {
		case PUBLISH:
			if (clntType == CONSUMER) {
				minRate = pJitterInfo->cons_PUBLISH_MinRate;
				maxRate = pJitterInfo->cons_PUBLISH_MaxRate;
			} else {
				minRate = pJitterInfo->prod_PUBLISH_MinRate;
				maxRate = pJitterInfo->prod_PUBLISH_MaxRate;
			}
			break;
		case PUBACK:
			if (clntType == CONSUMER) {
				minRate = pJitterInfo->cons_PUBACK_MinRate;
				maxRate = pJitterInfo->cons_PUBACK_MaxRate;
			} else {
				minRate = pJitterInfo->prod_PUBACK_MinRate;
				maxRate = pJitterInfo->prod_PUBACK_MaxRate;
			}
			break;
		case PUBREC:
			if (clntType == CONSUMER) {
				minRate = pJitterInfo->cons_PUBREC_MinRate;
				maxRate = pJitterInfo->cons_PUBREC_MaxRate;
			} else {
				minRate = pJitterInfo->prod_PUBREC_MinRate;
				maxRate = pJitterInfo->prod_PUBREC_MaxRate;
			}
			break;
		case PUBREL:
			if (clntType == CONSUMER) {
				minRate = pJitterInfo->cons_PUBREL_MinRate;
				maxRate = pJitterInfo->cons_PUBREL_MaxRate;
			} else {
				minRate = pJitterInfo->prod_PUBREL_MinRate;
				maxRate = pJitterInfo->prod_PUBREL_MaxRate;
			}
			break;
		case PUBCOMP:
			if (clntType == CONSUMER) {
				minRate = pJitterInfo->cons_PUBCOMP_MinRate;
				maxRate = pJitterInfo->cons_PUBCOMP_MaxRate;
			} else {
				minRate = pJitterInfo->prod_PUBCOMP_MinRate;
				maxRate = pJitterInfo->prod_PUBCOMP_MaxRate;
			}
			break;
	}

	if (minRate == 0x7FFFFFFF)
		minRate= 0;

	if (aggrTotal == 0) {
		aggrAverage = 0;
		maxPercentDiff = 0;
		minPercentDiff = 0;
	} else {
		aggrAverage = (int)(aggrTotal / maxEntries);
		minPercentDiff = (double)((fabs(aggrAverage - minRate)) / aggrAverage) * 100.0;
		maxPercentDiff = (double)((fabs(aggrAverage - maxRate)) / aggrAverage) * 100.0;
	}

	/* --------------------------------------------------------------------------------
	 * Using the state that was passed in determines what needs to be displayed.
	 * -------------------------------------------------------------------------------- */
	switch (pType) {
		case PUBLISH:
			fprintf(stdout, "\n%s Jitter Info:    Sample count =  %d/%d (act/max)\n\n" \
	                        "     %23s%14s%10s\n" \
    	                    "     %-10s%9d (%2d)%9d (%2d)%9d\n",
	                        (clntType == CONSUMER ? "Consumer" : "Producer"),
	                        maxEntries,
	                        pJitterInfo->sampleCt,
	                        "Max (%)", "Min (%)", "Avg",
    	                        (clntType == CONSUMER ? "RX PUB" : "TX PUB"),
                            maxRate,
                            (int)maxPercentDiff,
                            minRate,
                            (int)minPercentDiff,
                            (int)aggrAverage);
			break;
		case PUBACK:
		case PUBREC:
			fprintf(stdout, "     %-10s%9d (%2d)%9d (%2d)%9d\n",
                            (clntType == CONSUMER ? (pType == PUBACK ? "TX PUBACK" : "TX PUBREC") : (pType == PUBACK ? "RX PUBACK" : "RX PUBREC")),
                            maxRate,
                            (int)maxPercentDiff,
                            minRate,
                            (int)minPercentDiff,
                            (int)aggrAverage);
			break;
		case PUBREL:
			fprintf(stdout, "     %-10s%9d (%2d)%9d (%2d)%9d\n",
                            (clntType == CONSUMER ? "RX PUBREL" : "TX PUBREL"),
                            maxRate,
                            (int)maxPercentDiff,
                            minRate,
                            (int)minPercentDiff,
                            (int)aggrAverage);
			break;
		case PUBCOMP:
			fprintf(stdout, "     %-10s%9d (%2d)%9d (%2d)%9d\n",
                            (clntType == CONSUMER ? "RX PUBCOMP" : "TX PUBCOMP"),
                            maxRate,
                            (int)maxPercentDiff,
                            minRate,
                            (int)minPercentDiff,
                            (int)aggrAverage);
			break;
		default:
			break;
	} /* switch (pType) */
	fflush(stdout);

} /* displayJitter */

/* *************************************************************************************
 * showJitter
 *
 * Description:  Display the current Jitter information, by utilizing displayJitter().
 *               This interrogates the bitmask to see what data needs to be displayed.
 *
 *   @param[in]  mqttClientType      = Type of clients for all submitter threads:
 *                                        0x01 = CONSUMERS_ONLY
 *                                        0x02 = PRODUCERS_ONLY
 *                                        0x03 = MTHREAD_ONE_TYPE_PER_THREAD
 *                                        0x07 = CLIENTS_BOTH_PROD_CONS
 *   @param[in]  pJitterInfo         = Pointer to JitterInfo structure.
 * *************************************************************************************/
void showJitterInfo (int mqttClientType, mbJitterInfo_t *pJitterInfo)
{
	int i, k;
	int maskQoS;
	int * pArray = NULL;

	uint8_t clientType = 0;

	/* Go through the consumers 1st (k=0) and then the producers (k=1) */
	for ( i = 0 ; i < 2 ; i++ ) {
		if ((i == 0) && ((mqttClientType & CONTAINS_CONSUMERS) > 0)) {
			maskQoS = (int)pJitterInfo->rxQoSMask;
			clientType = CONSUMER;
		} else if ((i == 1) && ((mqttClientType & CONTAINS_PRODUCERS) > 0)) {
			maskQoS = (int)pJitterInfo->txQoSMask;
			clientType = PRODUCER;
		} else
			continue;

		/* ----------------------------------------------------------------------------
		 * Go thru each of the QoS' and determine which data needs to be displayed based
		 * on the QoS Mask stored in the jitter information structure.
		 * ----------------------------------------------------------------------------*/
		for ( k = 0 ; k <= MQTT_QOS_2 ; k++ ) {
			if (k == 0) {
				if (i == 0)
					pArray = pJitterInfo->rateArray_Cons_PUB;
				else
					pArray = pJitterInfo->rateArray_Prod_PUB;

				displayJitter(pJitterInfo, pArray, clientType, PUBLISH);
			} else if ((k == 1) && ((maskQoS & MQTT_QOS_1_BITMASK) > 0)) {
				if (i == 0)
					pArray = pJitterInfo->rateArray_Cons_PUBACK;
				else
					pArray = pJitterInfo->rateArray_Prod_PUBACK;

				displayJitter(pJitterInfo, pArray, clientType, PUBACK);
			} else if ((k == 2) && ((maskQoS & MQTT_QOS_2_BITMASK) > 0)) {
				if (i == 0) {
					pArray = pJitterInfo->rateArray_Cons_PUBREC;
					displayJitter(pJitterInfo, pArray, clientType, PUBREC);

					pArray = pJitterInfo->rateArray_Cons_PUBCOMP;
				} else {
					pArray = pJitterInfo->rateArray_Prod_PUBREC;
					displayJitter(pJitterInfo, pArray, clientType, PUBREC);

					pArray = pJitterInfo->rateArray_Prod_PUBCOMP;
				}

				displayJitter(pJitterInfo, pArray, clientType, PUBCOMP);
				break;
			}
		}
	}
} /* showJitterInfo */

/* *************************************************************************************
 * resetJitterInfo
 *
 * Description:  Reset the Data Arrays for the jitter captured for consumers and/or
 *               producers.
 *
 *   @param[in]  mqttClientType      = x
 *   @param[in]  pJitterInfo         = Pointer to JitterInfo structure.
 * *************************************************************************************/
void resetJitterInfo (int mqttClientType, mbJitterInfo_t *pJitterInfo)
{
	int  i, k;
	int  arraySize = sizeof(int) * pJitterInfo->sampleCt;
	int  maskQoS;

	int *pArray = NULL;

	/* Clean the consumers (k=0) and the producers (k=1) rate arrays */
	for ( i = 0 ; i < 2 ; i++ ) {
		if (i == 0) {
			pJitterInfo->cons_PUBLISH_MinRate = 0x7FFFFFFF;
			pJitterInfo->cons_PUBLISH_MaxRate = 0;
			pJitterInfo->cons_PUBACK_MinRate  = 0x7FFFFFFF;
			pJitterInfo->cons_PUBACK_MaxRate  = 0;
			pJitterInfo->cons_PUBREC_MinRate  = 0x7FFFFFFF;
			pJitterInfo->cons_PUBREC_MaxRate  = 0;
			pJitterInfo->cons_PUBREL_MinRate  = 0x7FFFFFFF;
			pJitterInfo->cons_PUBREL_MaxRate  = 0;
			pJitterInfo->cons_PUBCOMP_MinRate = 0x7FFFFFFF;
			pJitterInfo->cons_PUBCOMP_MaxRate = 0;
		} else {
			pJitterInfo->prod_PUBLISH_MinRate = 0x7FFFFFFF;
			pJitterInfo->prod_PUBLISH_MaxRate = 0;
			pJitterInfo->prod_PUBACK_MinRate  = 0x7FFFFFFF;
			pJitterInfo->prod_PUBACK_MaxRate  = 0;
			pJitterInfo->prod_PUBREC_MinRate  = 0x7FFFFFFF;
			pJitterInfo->prod_PUBREC_MaxRate  = 0;
			pJitterInfo->prod_PUBREL_MinRate  = 0x7FFFFFFF;
			pJitterInfo->prod_PUBREL_MaxRate  = 0;
			pJitterInfo->prod_PUBCOMP_MinRate = 0x7FFFFFFF;
			pJitterInfo->prod_PUBCOMP_MaxRate = 0;
		}

		if ((i == 0) && ((mqttClientType & CONTAINS_CONSUMERS) > 0))
			maskQoS = (int)pJitterInfo->rxQoSMask;
		else if ((i == 1) && ((mqttClientType & CONTAINS_PRODUCERS) > 0))
			maskQoS = (int)pJitterInfo->txQoSMask;
		else
			continue;

		/* Need to clean up all the arrays. */
		for ( k = 0 ; k <= MQTT_QOS_2 ; k++ ) {
			if (k == 0) {
				if (i == 0)
					pArray = pJitterInfo->rateArray_Cons_PUB;
				else
					pArray = pJitterInfo->rateArray_Prod_PUB;
			} else if ((k == 1) && ((maskQoS & MQTT_QOS_1_BITMASK) > 0)) {
				if (i == 0)
					pArray = pJitterInfo->rateArray_Cons_PUBACK;
				else
					pArray = pJitterInfo->rateArray_Prod_PUBACK;
			} else if ((k == 2) && ((maskQoS & MQTT_QOS_2_BITMASK) > 0)) {
				if (i == 0) {
					pArray = pJitterInfo->rateArray_Cons_PUBREC;
					memset(pArray, 0, arraySize);

					pArray = pJitterInfo->rateArray_Cons_PUBCOMP;
				} else {
					pArray = pJitterInfo->rateArray_Prod_PUBREC;
					memset(pArray, 0, arraySize);

					pArray = pJitterInfo->rateArray_Prod_PUBCOMP;
				}
			}

			memset(pArray, 0, arraySize);
		} /*  */
	} /* for ( i = 0 ; i < 2 ; i++ ) */

	pJitterInfo->wrapped = 0;
	pJitterInfo->currIndex = 0;
} /* resetJitterInfo */



/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 *
 *
 * ==================================================================================== */

/* *************************************************************************************
 * valStrIsNum
 *
 * Description:  Checks to see if the string passed is made up of all numbers (0-9).
 *
 *   @param[in]  testStr             = Character string to be tested to confirm each
 *                                     character is a number.
 *
 *   @return 0   = String is not a valid number.
 *           1   = String is a valid number.
 * *************************************************************************************/
uint8_t valStrIsNum (char *testStr)
{
	int rc = 0;
	int i;

	for ( i = 0 ; i < strlen(testStr) ; i++ ) {
		if (testStr[0] == '-')
			continue;
		if (isdigit(testStr[i]) == 0)
			return rc;
	}

	return 1;
} /* valStrIsNum */

/* *************************************************************************************
 * determineNumOfChars
 *
 * Description:  Determine how many bytes needed to convert the integer to a character
 *               string format.
 *
 *   @param[in]  parmNumber          = x
 *
 *   @return 0   =
 * *************************************************************************************/
int determineNumOfChars (int parmNumber)
{
	int numChars = 0;
	char strRep[16] = {0};

	/* --------------------------------------------------------------------------------
	 * Subtract 1 since the numbering starts at 0.  For example:  10 is 0 thru 9 which
	 * only requires 1 character.
	 * -------------------------------------------------------------------------------- */
	parmNumber--;

	snprintf(strRep, 16, "%d", parmNumber);  /* Convert integer to string */

	numChars = strlen(strRep); /* Determine number of characters. */

	if (numChars > 9) {
		MBTRACE(MBERROR, 1, "Unable to convert integer to string.\n");
		numChars = 0;
	}

    return numChars;
}

/* *************************************************************************************
 * determinePortRangeOverlap
 *
 * Description:  Determine if the SourcePortLo overlaps the current kernel port range.
 *
 *   @param[in]  srcPortLow          = The Source Port Low value.
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int determinePortRangeOverlap (int srcPortLow)
{
	int rc = 0;
	int krnlPortHigh = 1;

	char *savePtr;
	char *tmpPtr;
	char *delimits = " /\n/\t";
	char  line[KERNEL_LINE_SIZE];

	/* File Handle to the port range file. */
	FILE *fp = fopen(KRN_PORT_RANGE, "r");

	/* Process the cpuinfo file */
	if (fp)	{
		/* ----------------------------------------------------------------------------
 		* Iterate through the cpuinfo file line by line until reaching the end of the
		* file (i.e. NULL)
		* ----------------------------------------------------------------------------- */
		while ((fgets(line, KERNEL_LINE_SIZE, fp) != NULL)) {
			if (line[0] != '\0') {
				tmpPtr = strtok_r(line, delimits, &savePtr);
				if (tmpPtr) {
					tmpPtr = strtok_r(NULL, delimits, &savePtr);
					if (tmpPtr) {
						krnlPortHigh = atoi(tmpPtr);
					}
				}

				break;
			}
		}

		fclose(fp);

		if (krnlPortHigh > srcPortLow) {
			MBTRACE(MBWARN, 1, "An overlap with kernel local port range (%s) and env variable: SourcePortLo (%d) was detected, some source ports maybe be in use by other processes and will be skipped\n",
					KRN_PORT_RANGE, srcPortLow);
			rc = RC_PORTRANGE_OVERLAP;
		}
	} else {
		MBTRACE(MBWARN, 1, "Cannot open the %s file.\n", KRN_PORT_RANGE);
		fprintf(stdout, "(w) Cannot open the %s file.\n", KRN_PORT_RANGE);
		rc = RC_FILEIO_ERR;
	}

	fflush(stdout);

	return rc;
} /* determinePortRangeOverlap */



/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * File and Directory related routines
 *
 * ==================================================================================== */

/* *************************************************************************************
 * checkFileExists
 *
 * Description:  Check if the file specified exists
 *
 *   @param[in]  fileName            = Name of file to be checked if it exists.
 *
 *   @return 0   = File exists.
 *          -2   = File doesn't exist.
 * *************************************************************************************/
int checkFileExists (char *fileName)
{
	int rc;
	struct stat fileInfo;

	/* Use the stat command to determine if the file exists. */
    rc = stat(fileName, &fileInfo);
    if ((rc == 0) && (fileInfo.st_size == 0))
    	rc = RC_FILE_NOT_FOUND;

    return rc;

} /* checkFileExists */

/* *************************************************************************************
 * createFile
 *
 * Description:  Create a file to dump the client information to.
 *
 *   @param[in]  fpFile              = File Descriptor if successfully opened file.
 *   @param[in]  fName               = Name of the file to be opened.
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int createFile (FILE **fpFile, char *fName)
{
	int rc = 0;

	*fpFile = fopen(fName, "w");
	if (*fpFile == NULL) {
	    rc = RC_FILEIO_ERR;
	    MBTRACE(MBERROR, 1, "Unable to open the file (%s) for writing.  Check file permissions,\n" \
	    		 "   exiting (rc=%d)...\n",
				 fName,
				 rc);
	}

	return rc;
} /* createFile */

/* *************************************************************************************
 * removeFile
 *
 * Description:  Remove the file specified.
 *
 *   @param[in]  fileName            = Name of the file to be removed if it exists.
 *   @param[in]  fileType            = File type specified to be removed:
 *                                        REMOVE_EMPTY_FILE = 1
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
void removeFile (char *fileName, int fileType)
{
	int rc;
	struct stat fileInfo;

	FILE *fpFile;

	if (fileType == REMOVE_EMPTY_FILE) {
	    rc = stat(fileName, &fileInfo);
	    if ((rc == 0) && (fileInfo.st_size == 0))
			remove(fileName);
	} else {
		fpFile = fopen(fileName, "r");
		if (fpFile) {
			fclose(fpFile);
			remove(fileName);
		}
	}
} /* removeFile */

/* *************************************************************************************
 * renameFile
 *
 * Description:  Rename the file specified with a numeric suffix
 *
 *   @param[in]  origFname           = Name of file to be renamed.
 *   @param[in]  fpFile              = File Descriptor for the file specified.
 *   @param[in]  numSfx              = Suffix used when renaming file.
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int renameFile (char *origFName, FILE **fpFile, int numSfx)
{
	int rc = 0;
	int fnLen = 0;
	int tmpRC = 0;

	char *cPos = NULL;
	char  cSfx[8];
	char  tmpFN[MAX_CHAR_STRING_LEN];

	snprintf(cSfx, 8, "%d", numSfx);

	cPos = strstr(origFName, ".csv");
	fnLen = cPos - origFName;

	/* Generate the name for the rename file by using prefix filename and suffix number. */
	memcpy(tmpFN, origFName, fnLen);
	tmpFN[fnLen] = '_';
	tmpFN[fnLen+1] = 0;
	strcat(tmpFN, cSfx);
	strcat(tmpFN, ".csv");

	/* Close the current file. */
	tmpRC = fclose(*fpFile);
	*fpFile = NULL;
	if (tmpRC == 0) {
		tmpRC = rename(origFName, tmpFN);
		if (tmpRC == 0)
			*fpFile = fopen(origFName, "w");
		else {
			MBTRACE(MBERROR, 1, "Unable to rename %s\n", origFName);
			rc = tmpRC;
		}
	}

	return rc;
} /* renameFile */



/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * Topic related
 *
 * ==================================================================================== */


/* *************************************************************************************
 * getTopicVariableInfo
 *
 * Description:  If the Topic contains the COUNT variable then obtain the information
 *               on what range it is going to expand to.
 *
 *   @param[in]  topicName           = topic string containing the ${COUNT:x-y} variable
 *   @param[out] pTopicInfo          = the topic info structure to be populated by this function
 *
 *   @return 0   = Successful completion
 *         <>0   = An error/failure occurred
 * *************************************************************************************/
int getTopicVariableInfo (char *topicName, topicInfo_t *pTopicInfo)
{
	int rc = RC_BAD_CLIENTLIST;
	int len;
	int lenTopic = strlen(topicName);

	char *ip;
	char *startPos;
	char *endPos;
	char *ptr = NULL;
	char *varPosition;
	char  tmpVar[64];

	startPos = topicName;
	endPos = startPos + lenTopic;
	varPosition = strstr(topicName, "${COUNT:");

	/* Copy the characters prior to: ${COUNT} into the prefix variable. */
	assert(strlen(topicName) < MQTT_MAX_TOPIC_NAME);
	strncat(pTopicInfo->topicName_Pfx, topicName, (varPosition - startPos));

	ptr = varPosition + strlen("${COUNT:");

	for ( ip = ptr ; !isEOL(*ip) ; ip++ ) {
		if (*ip == '-') {
			len = ip-ptr;
			memcpy(tmpVar, ptr, len);
			tmpVar[len] = '\0';
			pTopicInfo->startNum = atoi(tmpVar);
			ptr = ++ip;
		} else if (*ip == '}') {
			len = ip-ptr;
			memcpy(tmpVar, ptr, len);
			tmpVar[len] = '\0';
			pTopicInfo->endNum = atoi(tmpVar);
			ip++;
			strncat(pTopicInfo->topicName_Sfx, ip, lenTopic - (endPos - ip));
			rc = 0;
			break;
		}
	}

	if (rc == RC_BAD_CLIENTLIST)
		MBTRACE(MBERROR, 1, "Format of the topic name with COUNT variable is incorrect: %s\n", topicName);

	return rc;
} /* getTopicVariableInfo */


/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * Basic functions
 *
 * ==================================================================================== */

/* *************************************************************************************
 * upper
 *
 * Description:  Uppercase the spring passed in it and return.
 *
 *   @param[in]  str                 = String to be converted to uppercase.
 *   @param[out] str                 = String converted to uppercase.
 * *************************************************************************************/
char * upper (char *str)
{
	char *ip;

	for ( ip = str ; !isEOL(*ip) ; ip++ )
		if ( *ip >= 'a' && *ip <= 'z' ) *ip = 'A' + (*ip-'a') ;

	return str ;
} /* upper */

/* *************************************************************************************
 * strip
 *
 * Description:  Strip the white space from the string passed in and return it.
 *
 *   @param[in]  str                 = String to have white space removed.
 *   @param[out] str                 = String returned without white space.
 * *************************************************************************************/
char * strip (char *str)
{
	int i, j, k;  /* i is first occurrence of non-space in the string */

	for ( i = 0 ; isspace(str[i]) ; i++ ) ;

	for ( j = 0, k = 0 ; !isEOL(str[i]) ; i++ ) {
		if ( j < i ) str[j] = str[i] ;
		if ( !isspace(str[j]) ) k = j+1 ;
		if ( k ) j++ ;
	}

	str[k] = '\0' ;

	return str ;
} /* strip */

/* *************************************************************************************
 * validateRate
 *
 * Description:  Checks to see if the string passed is made up of all numbers (0-9).
 *
 *   @param[in]  testStr             = String to be validated that contains only numbers.
 *
 *   @return 0   = false (String is NOT made up of all numbers)
 *           1   = true  (String is made up of all numbers)
 * *************************************************************************************/
int validateRate (char *testStr)
{
	int rc = 0;
	int i;

	for ( i = 0 ; i < strlen(testStr) ; i++ ) {
		if ((isdigit(testStr[i]) == 0) && (testStr[i] != '.'))
			return rc;
	}

	return 1;
} /* validateRate */



/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * Obtaining system related information (e.g. memory, cpu clock frequency)
 *
 * ==================================================================================== */

/* *************************************************************************************
 * read_cpu_proc_freq
 *
 * Description:  Read the /proc/cpuinfo to obtain the clock frequency.
 *               In Linux on __x86_64__ or __i386__ this is on the
 *               line: cpu MHz	    : xxxx.xxx
 *
 *   @return 0   = Successful completion
 *          66   = An error/failure occurred and exited the program.
 *         1e6   = CPU Frequency in MHz.
 * *************************************************************************************/
double read_cpu_proc_freq (void)
{
	char line[KERNEL_LINE_SIZE];

	/* File Handle to the cpuinfo file. */
	FILE *fp = fopen(PROC_CPUINFO, "r");

	/* Process the cpuinfo file */
	if (fp)	{
		/* ----------------------------------------------------------------------------
 		 * Iterate through the cpuinfo file line by line until reaching the end of the
		 * file (i.e. NULL)
		 * ---------------------------------------------------------------------------- */
		while ((fgets(line, KERNEL_LINE_SIZE, fp) != NULL)) {
			char *ip;

			do {
				if (line[0] == '#') break;  /* Skip line if it is a comment */

				line[KERNEL_LINE_SIZE-1] = 0 ;     /* Set the null terminator character
										     * to the end of the line */

				/* --------------------------------------------------------------------
				 * empty for loop body, move along the line until hitting an ':', newline
				 * or the end of the string
				 * -------------------------------------------------------------------- */
				for ( ip = line ; *ip != ':' && *ip != 0 && *ip != '\n' ; ip++ ) ;

				/* --------------------------------------------------------------------
			 	 * If the current pointer is not an ':' then the line is not a valid
				 * cpuinfo entry (i.e name:value).  Need to check if this is a blank
				 * line or not.  If not a blank line then provide a message indicating
				 * an invalid configuration line.
				 * -------------------------------------------------------------------- */
				if ( *ip != ':' ) break;

				ip[0] = 0;	/* ip will be pointing at ':', change this to 0*/
				ip++;		/* Now ip is pointing to the character just after the ':' */
				upper(strip(line)); /* Remove the spaces from left of the ':' sign
									 * and uppercase the property name */
				strip(ip);	/* Strip the spaces from right of the ':' sign */

				/* --------------------------------------------------------------------
 				 * At this point "line" is pointing to the property name and "ip" is
				 * pointing to the property value.
				 * -------------------------------------------------------------------- */
				if (strcmp(line, CPU_MHZ) == 0) {
					fclose(fp);
					return (atof(ip) * INV_MICROSECOND_EXP);
				}
			} while(0);
		}

		fclose(fp);

		/* If reached here then was not able to find the clock frequency.  Report it. */
		MBTRACE(MBERROR, 1, "Unable to find the clock frequency in the %s file.\n", PROC_CPUINFO);
	} else
		MBTRACE(MBERROR, 1, "Cannot open the %s file,  exiting (rc=%d)...\n\n",
				            PROC_CPUINFO,
							RC_ERR_OPEN_CPUINFO_FILE);

	exit(RC_ERR_OPEN_CPUINFO_FILE);
} /* read_cpu_proc_freq */

/* *************************************************************************************
 * getCPU
 *
 * Description:  Obtain the number of CPU Clock Ticks per second and set the User and
 *               System CPU Frequency.
 *
 *   @param[out] userCPUTime         = User CPU Frequency
 *   @param[out] systemCPUTime       = System CPU Frequency
 * *************************************************************************************/
void getCPU (double *userCPUTime, double *systemCPUTime)
{
	struct tms cpu;
	static int init = 1;
	double cpuFrequency = 0;

	if (init) {
		init = 0;
		cpuFrequency = (double)sysconf(_SC_CLK_TCK);  /* get the cpu clock ticks per second */
	}

	times(&cpu);
	*userCPUTime = (double)cpu.tms_utime / cpuFrequency;
	*systemCPUTime = (double)cpu.tms_stime / cpuFrequency;
} /* getCPU */

/*
 * Read the contents of a file into a buffer.
 *
 * @param[in]	path		= the path of the file to read
 * @param[out] 	buff		= the buffer to store the contents of the file in
 * @param[out] 	bytesRead	= the number of bytes read from the file
 *
 * @return 0 on successful completion, or a non-zero value
 */
int readFile(const char *path, char **buff, int *bytesRead){
	int rc = 0;
	FILE *file;
	char errorStr[1024];
	struct stat sb;

	/* check if file exists */
	if (stat(path,&sb) < 0){
	   	MBTRACE(MBERROR, 1, "File \"%s\" cannot be found (stat errno=%s), please specify a valid file path\n", path, strerror_r(errno,errorStr,1024));
	    return RC_FILEIO_ERR;
	}

	/* try opening the file for reading */
	file = fopen(path,"r");
	if(file == NULL) {
	  	MBTRACE(MBERROR, 1, "Unable to open file %s for reading, check permissions (errno=%s)\n", path, strerror_r(errno,errorStr,1024));
	   	return RC_FILEIO_ERR;
	}

	/* File path is valid, now lets try reading the file and attempt to parse it as JSON */
	// obtain file size:
	fseek(file, 0, SEEK_END);
	*bytesRead = ftell(file);
	rewind(file);

	// allocate buffer to hold file contents:
	*buff = (char*) calloc(*bytesRead, sizeof(char)); /*BEAM suppression: memory leak */
	if(*buff == NULL){
		MBTRACE(MBERROR, 1, "Unable to allocate memory buffer for file %s (errno=%s)\n", path, strerror_r(errno,errorStr,1024));
		fclose(file);
		return RC_MEMORY_ALLOC_ERR;
  	}

	// read file into buffer
	int result = fread(*buff, 1, *bytesRead, file);
	if(result != *bytesRead){
		MBTRACE(MBERROR, 1, "Unable to read file %s (errno=%s)\n", path, strerror_r(errno,errorStr,1024));
	    fclose(file);
	    free(*buff);   /*BEAM suppression: pointer to freed memory exposed*/
	    return RC_FILEIO_ERR;
	}

	// close file
	fclose(file);

	return rc;
}

/* *************************************************************************************
 * getFileContent
 *
 * Description:  Read a whole file into a buffer.  This is used when trying to obtain a
 *               specific setting (e.g. meminfo)
 *
 *   @param[in]  name                = Name of file to be copied to a buffer.
 *   @param[out] buf                 = Buffer used to contain the file specified.
 *   @param[in]  len                 = Number of bytes to copy into the buffer.
 *
 *   @return 1   = Successful completion.
 *           0   = An error/failure occurred.
 * *************************************************************************************/
int getFileContent (const char *name, char *buf, int len)
{
    int fd;
    int bRead;
    int totallen = 0;

    fd = open(name, O_RDONLY);
    if (fd < 0)
        return 0;
    for (;;) {
        bRead = read(fd, buf+totallen, len-1-totallen);
        if (bRead < 1) {
            if (errno == EINTR)
                continue;
            break;
        }

        totallen += bRead;
    }

    buf[totallen] = 0;
    close(fd);

    return totallen > 1 ? 1 : 0;
} /* getFileContent */

/* *************************************************************************************
 * getSystemMemory
 *
 * Description:  Obtain the amount of memory on the system by reading the /proc/meminfo.
 *
 *   @param[out] totalSysMemMB       = Size of requested system memory.
 *   @param[in]  memStat             = The type of memory stat found in /proc/meminfo.
 *                                     This is a string (e.g. "MemAvailable")
 *   @param[in]  rtnSize             = Return the memory in specified size.
 *
 *   @return 0   = Successful completion
 *          -1   = An error/failure occurred
 * *************************************************************************************/
int getSystemMemory (uint64_t *memStatsSize, char *memStat, long int sizeQualifier)
{
	int rc = 0;
	int bufLen = (128 * 1024);

	char *buf;
	char *pos;

	uint64_t sysMemKB = 0;

	buf = calloc(1, bufLen);
	if (buf) {
		if (getFileContent(SYSTEM_MEMORY, buf, bufLen)) {
			pos = strstr(buf, memStat);
			if (pos) {
				pos = strchr((pos+8), ':');
				if (pos) {
					sysMemKB = strtoull((pos+1), NULL ,10) * 1024;
					*memStatsSize = sysMemKB / sizeQualifier;
				}
			} else {
				/* Unable to determine the amount of memory so set rc = 90 (RC_INTERNAL_ERROR) */
				rc = RC_INTERNAL_ERROR;
			}
		}

		free(buf); /* Free memory used to hold file for reading memory. */
	} else
    	rc = provideAllocErrorMsg("determining the amount of memory", bufLen, __FILE__, __LINE__);

	return rc;
} /* getSystemMemory */

/* *************************************************************************************
 * Check various kernel parameters and provide warnings to the user of kernel parameters
 * that ought to be tuned.

 *   @param[in] path       = path of the kernel parameter
 *   @param[in] expected   = expected value
 *   @param[in] op		   = compare operation (eq, le, ge)
 *
 *   @return 0 = OK, anything else is an error
 *
 * *************************************************************************************/
int checkParam(const char *path, const char *expected, const char *op) {
	int rc = 0;
	char buf[KERNEL_LINE_SIZE];
	rc = getFileContent(path, (char *) &buf, KERNEL_LINE_SIZE);
	if(!rc) {
		MBTRACE(MBWARN, 5, "Unable to read kernel param %s\n", path);
		return RC_READ_KERN_ERR;
	}

	if(strcmp(op,"eq") == 0){
		return strcmp(buf, expected);
	}

	if(strcmp(op,"le") == 0){
		int exp = atoi(expected);
		int act = atoi(buf);
		return !(act <= exp);
	}

	if(strcmp(op,"ge") == 0) {
		int exp = atoi(expected);
		int act = atoi(buf);
		return !(act >= exp);
	}

	return rc;
}

#define PROC_TW_REUSE     "/proc/sys/net/ipv4/tcp_tw_reuse"
#define PROC_IP_PRT_RANGE "/proc/sys/net/ipv4/ip_local_port_range"
#define PROC_NETDEV_BKLG  "/proc/sys/net/core/netdev_max_backlog"

void checkSystemSettings(void) {
	int rc = 0;
	rc = checkParam(PROC_TW_REUSE,    "1",       "ge");  if(rc) {MBTRACE(MBWARN, 5, "Kernel parameter %s should be set to %s\n",   PROC_TW_REUSE,     "1");}
	rc = checkParam(PROC_NETDEV_BKLG, "1000000", "ge");  if(rc) {MBTRACE(MBWARN, 5, "Kernel parameter %s should be at least %s\n", PROC_NETDEV_BKLG,  "1000000");}
	if(g_MqttbenchInfo->mbSysEnvSet->useEphemeralPorts) {
		rc = checkParam(PROC_IP_PRT_RANGE, "5000 65000", "eq"); if(rc) {MBTRACE(MBWARN, 5, "Kernel parameter %s should be set to %s\n", PROC_IP_PRT_RANGE, "5000 65000");}
	}
}

/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * Clock, Timer, Time and Logging Related
 *
 * ==================================================================================== */

/* *************************************************************************************
 * createLogTimeStamp
 *
 * Description:  Create a timestamp for the log file.
 *               Format Types:
 *                  1 - "%FT%T"
 *                  2 - "%I:%M:%S %p"
 *                  3 - "%c"
 *
 *   @param[out] currTimeStamp       = String of current timestamp in the specified format.
 *   @param[out] lenTimeStamp        = Length of the current timestamp string.
 *   @param[in]  formatType          = The format of the timestamp to use.
 * *************************************************************************************/
void createLogTimeStamp (char currTimeStamp[], int lenTimeStamp, int formatType)
{
	struct timeb beginTime;
	time_t currTime;

	if (lenTimeStamp == MAX_TIME_STRING_LEN) {
		memset(currTimeStamp, 0, lenTimeStamp);

		/* Obtain the current logging time in format of:  HH:MM:SS */
		ftime(&beginTime);
		time(&currTime);

		switch (formatType) {
			case 1:
				strftime(currTimeStamp, MAX_TIME_STRING_LEN, "%FT%T", (struct tm *)localtime(&currTime));
				break;
			case 2:
				strftime(currTimeStamp, MAX_TIME_STRING_LEN, "%I:%M:%S %p", (struct tm *)localtime(&currTime));
				break;
			case 3:
				strftime(currTimeStamp, MAX_TIME_STRING_LEN, "%c", (struct tm *)localtime(&currTime));
				break;
			default:
				TRACE(1, "Invalid format type for the timestamp.\n");
				break;
		} /* switch (formatType) */
	}
} /* createLogTimeStamp */

/* *************************************************************************************
 * displayAndLogError
 *
 * Description:  Display the error to the screen and then log the error into the
 *               mqttbench.log
 *
 *   @param[in]  cErrorString        = The error message to display and log.
 * *************************************************************************************/
void displayAndLogError (char *cErrorString)
{
	MBTRACE(MBERROR, 1, "%s\n", cErrorString);
}

/* *************************************************************************************
 * getCurrTime
 *
 * Description:  Get the current time in seconds and return in double.
 *
 *   @param[in]  x                   = x
 * *************************************************************************************/
double getCurrTime (void)
{
	struct timeval tv;
	double currTime;  /* current time in seconds */

	/* Get time since 1970 in microseconds */
	gettimeofday(&tv,NULL);  /* NULL - no time zone specified */
	currTime = (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;  /* seconds + milliseconds */

	return currTime;
} /* getCurrTime */

/* *************************************************************************************
 * getTimer
 *
 * Description:  Update the milliseconds timer.  This function sets the total running time,
 *               the delta time, and the previous time (used to calculate the delta time)
 *               in milliseconds.
 *
 *   @param[in]  timer               = x
 * *************************************************************************************/
void getTimer (Timer *timer)
{
	double curTOD;   /* current time of day (TOD) in milliseconds */
	struct timeb tb; /* data structure required by the ftime API */
	ftime(&tb);      /* invoke the standard API for getting the date and time */
	curTOD = (double)tb.time + (double)tb.millitm * 1.0e-3; /* current time in seconds = seconds + milliseconds */
	timer->total = curTOD - timer->base; /* total time in milliseconds since the call to setTimer */
	timer->delta = curTOD - timer->last; /* time in milliseconds since the last call to getTimer */
	timer->last = curTOD;	/* set last time for subsequent call to getTimer */
} /* getTimer */

/* *************************************************************************************
 * setTimer
 *
 * Description:  Initialize the milliseconds timer.  This function sets the base time.
 *
 *   @param[in]  timer               = x
 * *************************************************************************************/
void setTimer (Timer *timer)
{
	getTimer(timer); /* Get the current time in milliseconds */

	/* --------------------------------------------------------------------------------
	* Set the initial base time to the last time which is also the current time since
	* setTimer is called only once per monitor thread
	* --------------------------------------------------------------------------------- */
	timer->base = timer->last;
} /* setTimer */

/* *************************************************************************************
 * initTime
 *
 * Description:  Initialize the base time in seconds - this function is only called once.
 * *************************************************************************************/
void initTime (void)
{
	struct timeval tv;
	double currTime = 0;       /* current time in seconds */
	double baseTOD = 0;     /* time in microseconds */

	/* If mbBaseTime is already set then exit the function */
	if (g_BaseTime == 0) {
		/* Get time since 1970 in microseconds */
		gettimeofday(&tv, NULL);  /* NULL - no time zone specified */
		currTime = (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;  /* seconds + microseconds */

		/* ----------------------------------------------------------------------------
		 * Loop until you get to the trailing edge of the previous microsecond, then set
		 * set the base time on the leading edge of the new microsecond.
		 * ---------------------------------------------------------------------------- */
		do {
			gettimeofday(&tv,NULL);
			baseTOD = (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;
		} while ( currTime == baseTOD );

		g_BaseTime = sysTime(); /* On the leading edge of the new microsecond,
		                         * the baseTime will be in nanoseconds using
		                         * the REALTIME clock */

		MBTRACE(MBINFO, 1, "Setting base time (seconds): %f, base time since calling initTime (seconds): %f\n",
			               baseTOD,
			               g_BaseTime);

		/* If using the Hi Resolution clock then need to get the Clock Speed */
		CLOCK_FREQ = read_cpu_proc_freq();
	}
} /* initTime */

/* *************************************************************************************
 * sysTime
 *
 * Description:  Retrieve the system time in seconds
 *
 *   @param[in]  x                   = x
 * *************************************************************************************/
double sysTime (void)
{
	static int init = 1;       /* One time init flag for the entire process */
	double currTime = 0;       /* Used to hold the current time in seconds (returned value) */

	static struct timespec t0; /* time 0 or base time in nanoseconds, set only the 1st time
	                            * sysTime is called */
	struct timespec ts; /* Set every time sysTime is called */

	/* This section of code should only be called once per process. */
	if (init) {
		init = 0; /* assure that this will not happen again for the life of the process */
		clock_gettime(clock_id, &t0);
		initTime();
	}

	/* Get the current system time using the realtime clock */
	clock_gettime(clock_id, &ts);

	/* --------------------------------------------------------------------------------
	* Subtract seconds of base time from current time to calibrate the current time to
	* the beginning of the process.
	* --------------------------------------------------------------------------------- */
	currTime = (double)(ts.tv_sec - t0.tv_sec) + ((double)ts.tv_nsec * 1.0e-9);

	return currTime;
} /* sysTime */



/* ====================================================================================
 * ====================================================================================
 * ====================================================================================
 *
 * Error Related
 *
 * ==================================================================================== */

/* *************************************************************************************
 * provideAllocErrorMsg
 *
 * Description:  Routine to provide an error message due to an memory allocation error.
 *
 *   @param[in]  errMsgType          = x
 *   @param[in]  elemSize            = x
 *   @param[in]  fName               = x
 *   @param[in]  lineNum             = x
 *
 *   @return 30  = Allocation error
 * *************************************************************************************/
int provideAllocErrorMsg (char *errMsg, int elemSize, char *fName, int lineNum)
{
	MBTRACE(MBERROR, 1, "Unable to allocate memory for %s (%d bytes) %s:%d.\n", errMsg, elemSize, fName, lineNum);

	/* --------------------------------------------------------------------------------
	 * Get the amount of free memory due to memory allocation error.  Since there was a
	 * failure in allocating memory use the system function to perform a free -m to
	 * help debug
	 * -------------------------------------------------------------------------------- */
	getAmountOfFreeMemory(LOG_DATA);

   	return RC_MEMORY_ALLOC_ERR;
} /* provideAllocErrorMsg */

/* *************************************************************************************
 * getAmountOfFreeMemory
 *
 * Description:  Determine the amount of free memory by calling getSystemMemory with the
 *               particular string "MemAvailable" and the return value to be in GB.
 *
 *   @param[in]  dataLocale          = Where to put the data results in terms of tracelog
 *                                     or to console.
 *
 *   @return 0   = Successful Completion.
 *         <>0   = An error/failure occurred.
 * *************************************************************************************/
void getAmountOfFreeMemory (int dataLocale)
{
	int rc = 0;
	uint64_t totalAvailMem = 0;

    /* --------------------------------------------------------------------------------
     * Call getSystemMemory for the amount of free memory.
     * -------------------------------------------------------------------------------- */
	rc = getSystemMemory(&totalAvailMem, MEM_AVAIL, MEM_IN_GB);
	if (rc == 0) {
		if (dataLocale == LOG_DATA) {
			MBTRACE(MBINFO, 1, "Available free memory: %ld GB.\n", (long int)totalAvailMem);
		} else if (dataLocale == DISPLAY_DATA) {
			fprintf(stdout, "Available free memory: %ld GB.\n", (long int)totalAvailMem);
			fflush(stdout);
		}
	} else
		MBTRACE(MBERROR, 1, "Failed to obtain amount of free memory (rc=%d).\n", rc);
} /* getAmountOfFreeMemory */

/* *************************************************************************************
 * setMBTraceLevel
 *
 * Description:  Set the trace level to the new value.
 *   @param[in]  newTraceLevel       = The requested level the user wants the
 *                                     TraceLevel set to.
 *   @return 0   = Successful Completion.
 *           1   = Invalid TraceLevel.
 ************************************************************************************/
int setMBTraceLevel (int newTraceLevel)
{
	/* Set the trace level to the requested value. */
	char clevel[2];

	if (newTraceLevel >= 0 && newTraceLevel <= 9) {
		clevel[0] = newTraceLevel + '0';
		clevel[1] = 0;

		ism_common_setTraceLevelX(ism_defaultTrace, clevel);
		g_MBTraceLevel = newTraceLevel;
		MBTRACE(MBINFO, 1, "TraceLevel successfully set to %d\n", newTraceLevel);
		return 0;
	}

	return 1;
} /* setMBTraceLevel */
