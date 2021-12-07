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
#include <protoex.h>

#include "mqttclient.h"
#include "tcpclient.h"
#include "mbconfig.h"

/* External Variables */
extern mqtt_prop_ctx_t      *g_mqttCtx5;
extern mqtt_prop_field_t     g_mqttPropFields[];
extern ioProcThread_t      **ioProcThreads;        /* ioProcessor threads */
extern ioListenerThread_t  **ioListenerThreads;    /* ioListener threads */

/* ******************************** GLOBAL VARIABLES ********************************** */
uint8_t g_ClkSrc             = 0;        /* Clock source to be used: 0=TSC, 1=GTOD */
uint8_t g_DoDisconnect       = 0;        /* mqttbench will disconnect the transport flag. Used with -efd and noreconn */
uint8_t g_PrintUsage		 = 1;        /* Flag to check whether or not to print usage help text */
uint8_t g_RequestedQuit      = 0;
uint8_t g_StopIOPProcessing  = 0;        /* Specifically notify the IOP Threads to stop processing. */
uint8_t g_StopProcessing     = 0;        /* Stop processing messages and/or jobs */
uint8_t g_TCPConnLatOnly     = 0;

int16_t g_SubmitterExit      = 0;        /* Each submitter will increment as completed. */
int16_t g_WaitReady	         = 1;        /* Delay publishing messages on submitter threads to allow all clients to complete MQTT connections/subscriptions first */

int g_AppSimLoop             = 0;        /* A busy loop used to simulate an application processing a message */
int g_ChkMsgMask             = 0;        /* Mask for checking info (i.e. length, seq #) of a message. */
int g_DoCommandsExit         = 0;        /* Each consumer will increment as completed from DoCommands. */
int g_MBErrorRC              = 0;        /* Global return code used to provide more info for a fatal failure. */
int g_MostSubsPerClient    	 = 0;        /* The most subscriptions/client configured in the client list */
int g_TotalNumClients        = 0;        /* Total number of clients. */

int g_Equiv1Sec_Conn         = 0;        /* Conversion of g_UnitsConn to 1 sec. */
int g_Equiv1Sec_RTT          = 0;        /* Conversion of g_UnitsRTT to 1 sec. */
int g_Equiv1Sec_Reconn_RTT   = 0;        /* Conversion of g_UnitsReconnRTT to 1 sec. */
int g_LatencyMask            = 0;
int g_MaxRetryAttempts       = 0;        /* Max number of retry attempts before disconnecting */
int g_MinMsgRcvd             = 1000000;
int g_MaxMsgRcvd             = 0;
int g_TxBfrSize              = 0;		 /* the largest PUB or SUB packet that any client will send (also the size of the buffers in the TX buffer pool) */
char *g_predefinedMsgDir	 = NULL;     /* -M command line param, the default message directory containing JSON message files */

int *g_srcPorts              = NULL;     /* Source port counters for clients spread across multiple */
char **g_EnvSIPs             = NULL;     /* List of Source IP Addresses specified with the SIPList environment variable. */

char  *csvLogFile            = NULL;     /* Filename of the csv log file. */
char  *csvLatencyFile        = NULL;     /* Filename of the csv latency log file. */

double g_UnitsConn           = 1e3;      /* Units for Connection latency statistics */
double g_UnitsRTT            = 1e6;      /* Units for RTT Message latency statistics */
double g_UnitsReconnRTT      = 1e3;      /* Units for Reconnect RTT Message latency statistics. */
double g_MsgRate             = 1000;     /* Global message rate (msgs/sec) for the process (i.e. sum of all producers) */
double g_StartTimeConnTest   = 0.0;
double g_EndTimeConnTest     = 0.0;
double g_ConnRetryFactor     = 0.0;

unsigned char *g_alpnList    = NULL;     /* ALPN protocol list - https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_alpn_protos.html */
unsigned int g_alpnListLen   = 0;

mqttbench_t *pMQTTBench      = NULL;

pskArray_t *pIDSharedKeyArray = NULL;

mqttbenchInfo_t   *g_MqttbenchInfo = NULL;
mbMsgConfigInfo_t *g_MsgCfgInfo = NULL;
environmentSet_t  *pSysEnvSet = NULL;    /* Environment variables used to control the mqttbench test */
mbBurstInfo_t     *pBurstInfo = NULL;
mbCmdLineArgs_t   *pCmdLineArgs = NULL;  /* Structure holding command line arguments. */
mbJitterInfo_t    *pJitterInfo = NULL;
mbThreadInfo_t    *pThreadInfo = NULL;
mbTimerInfo_t     *pTimerInfo = NULL;    /* Structure that holds all the timer keys */
latencyUnits_t    *pLatencyUnits = NULL;

/* External Spinlocks */
pthread_spinlock_t mqttlatency_lock;
pthread_spinlock_t submitter_lock;
pthread_spinlock_t ready_lock;
pthread_spinlock_t connTime_lock;
pthread_spinlock_t mbinst_lock;

struct drand48_data randBuffer;

FILE *g_fpCSVLogFile      = NULL;
FILE *g_fpCSVLatencyFile  = NULL;
FILE *g_fpDebugFile       = NULL;
FILE *g_fpHgramFiles[MAX_NUM_HISTOGRAMS];

/* *************************************************************************************
 * setMqttbenchLogFile
 *
 * Description:  Sets the name and the path of the mqttbench trace log file.  Checks to
 *               see if the Environment Variable:  MqttbenchLogPath is set.   If this is
 *               set then the mqttbench_trace.log file will be the value set in this
 *               variable.  Otherwise it will be set to "mqttbench_trace.log" in the
 *               current directory.
 *
 *   @return 0   = Successful completion
 * *************************************************************************************/
int setMqttbenchLogFile (char *cTraceLogFile)
{
	int rc = RC_SUCCESSFUL;
	int logLen = 0;

	char *cPath = NULL;

	/* Get the value of the Env Variable:  MqttbenchLogPath */
	cPath = getenv("MqttbenchLogPath");

	/* See if the ENV Variable MqttbenchLogPath is set. */
	if (cPath) {
		logLen = (int)strlen(cPath);  /* Get current length of the path. */

		/* ----------------------------------------------------------------------------
		 * Check to see if the length of path + filename + slash (if needed) + 1 blank
		 * space is less than the maximum directory name.  If so, then copy the env
		 * variable, slash (if needed) and filename.
		 * ---------------------------------------------------------------------------- */
		if ((logLen + 1) < MAX_DIRECTORY_NAME)
			strcpy(cTraceLogFile, cPath);
		else
			rc = RC_STRING_TOO_LONG;
	} else
		strcpy(cTraceLogFile, MQTTBENCH_TRACE_FNAME);

	return rc;
} /* setMqttbenchLogFile */

/* *************************************************************************************
 * initMQTTBench
 *
 * Description:  Initialize the messageConfigInfo (g_MsgCfgInfo) with the min and max
 *               message sizes.
 *
 *   @return 0   = Successful completion
 * *************************************************************************************/
int initMQTTBench (void)
{
	int rc = 0xDEAD;

    MBTRACE(MBINFO, 1, "Initializing mqttbench data structures.\n");

	/* Initialize MQTT context for processing MQTT properties */
	g_mqttCtx5 = ism_common_makeMqttPropCtx(g_mqttPropFields, 5);

    /* Initialize some values to defaults. */
	g_MqttbenchInfo = (mqttbenchInfo_t *) calloc(1, sizeof(mqttbenchInfo_t));
	if (g_MqttbenchInfo) {
		/* Allocate memory for the message information structure. */
		g_MsgCfgInfo = calloc(1, sizeof(mbMsgConfigInfo_t));
		if (g_MsgCfgInfo) {
			g_MsgCfgInfo->minMsg = MIN_MSG_SIZE;  /* min range of message size (bytes) -s <min>-<max> */
			g_MsgCfgInfo->maxMsg = MIN_MSG_SIZE;  /* max range of message size (bytes) -s <min>-<max> */

			/* Allocate memory for the thread information structure */
			pThreadInfo = (mbThreadInfo_t *) calloc(1, sizeof(mbThreadInfo_t));
			if (pThreadInfo) {
				/* Allocate memory for the command line arguments. */
				pCmdLineArgs = (mbCmdLineArgs_t *)calloc(1, sizeof(mbCmdLineArgs_t));
				if (pCmdLineArgs) {
					/* Allocate memory for the timer information. */
					pTimerInfo = (mbTimerInfo_t *)calloc(1, sizeof(mbTimerInfo_t));
					if (pTimerInfo) {
					    /* Allocate memory for the system environment information. */
						pSysEnvSet = (environmentSet_t *)calloc(1, sizeof(environmentSet_t));
					   	if (pSysEnvSet)
					   		rc = 0;
					   	else
					    	provideAllocErrorMsg("environment structure", (int)sizeof(environmentSet_t), __FILE__, __LINE__);
					} else
				    	provideAllocErrorMsg("timer info structure", (int)sizeof(mbTimerInfo_t), __FILE__, __LINE__);
				} else
			    	provideAllocErrorMsg("command line structure", (int)sizeof(mbCmdLineArgs_t), __FILE__, __LINE__);
			} else
		    	provideAllocErrorMsg("thread info structure", (int)sizeof(mbThreadInfo_t), __FILE__, __LINE__);
		}
	}

	if (rc == 0) {
	   	/* Put all the pointers into the master mqttbench information structure. */
		g_MqttbenchInfo->mbMsgCfgInfo = g_MsgCfgInfo;
		g_MqttbenchInfo->mbThreadInfo = pThreadInfo;
		g_MqttbenchInfo->mbCmdLineArgs = pCmdLineArgs;
		g_MqttbenchInfo->mbTimerInfo = pTimerInfo;
		g_MqttbenchInfo->mbSysEnvSet = pSysEnvSet;
	}

	return rc;
} /* initMQTTBench */

/* *************************************************************************************
 * initMqttbenchLocks
 *
 * Description:  Initialize all the locks for mqttbench.
 * *************************************************************************************/
void initMqttbenchLocks (int latMask)
{
    /* Initialize the waitReady spinlock */
	pthread_spin_init(&ready_lock, 0);

    /* Initialize the submitter spinlock */
   	pthread_spin_init(&submitter_lock,0);

   	/* Initialize the mbinst spinlock */
	pthread_spin_init(&mbinst_lock, 0);

    /* If performing latency initialize the spinlock */
	if (latMask) {
		pthread_spin_init(&connTime_lock, 0);
		pthread_spin_init(&mqttlatency_lock, 0);
	}

} /* initMqttbenchLocks */

/* *************************************************************************************
 * checkParms
 *
 * Description:  This checks the particular parameter in regards to whether it was
 *               already passed, if an additional parameter is needed and are there any
 *               additional arguments available.
 *
 *   @param[in]  parmSet             = Flag indicating whether parameter already specified.
 *                                     0 = 1st time thru, 1 = Set previously
 *   @param[in]  parmString          = Parameter string to be tested.
 *   @param[in]  additionalArg       = Parameter has additional entries expected.
 *   @param[in]  argc                = Total # of arguments specified on command line.
 *   @param[in]  argIndex            = Index of the current parameter.
 *
 *   @return 0   = Successful Completion
 *           1   = An error and syntaxhelp was invoked.
 *          >1   = An error/failure occurred.
 * *************************************************************************************/
int checkParms (uint8_t parmSet, char *parmString, int additionalArg, int argc, int argIndex)
{
	int rc = 1;
    char errMsg[MIN_ERROR_STRING] = {0};

    if (parmSet == 0) {
    	if (additionalArg == 1) {
    		if ((++argIndex) < argc)
    			rc = 0;
    		else {
    			snprintf(errMsg, MIN_ERROR_STRING, "No argument specified with %s", parmString);
    			syntaxhelp(HELP_ALL, errMsg);
    			rc = RC_NO_ARGUMENT;
    		}
    	} else
    		rc = 0;
    } else {
    	MBTRACE(MBERROR, 1, "%s option already specified.\n", parmString);
    	rc = RC_PARAMETER_ALREADY_SPECIFIED;
    }

	return rc;
} /* checkParms */

/*
 * Allocate a message directory object, populate it with generated msgs, and insert it into the msgDirMap
 *
 * @param[in/out]	mqttbenchInfo   	=	mqttbench user input object (command line params, env vars, etc.)
 * @param[in]		mbMsgConfigInfo		= 	the global message config info object
 *
 * @return 0 = OK, anything else is an error
 */
static int generateMessageDir(mqttbenchInfo_t *mqttbenchInfo, mbMsgConfigInfo_t *mbMsgConfigInfo) {
	int rc = 0, numMsgSizes = 0;
	uint8_t fill[8] = { 0xD, 0xE, 0xA, 0xD, 0xB, 0xE, 0xE, 0xF };

	messagedir_object_t *msgDirObj = calloc(1, sizeof(messagedir_object_t));
	if(msgDirObj == NULL){
		return provideAllocErrorMsg("message directory object", sizeof(messagedir_object_t), __FILE__, __LINE__);
	}

	msgDirObj->generated = 1; // messages in this directory are binary generated messages

	/* Determine number of generated messages to create. Message sizes are power of 2 */
	int size = mbMsgConfigInfo->minMsg;
	do {
		numMsgSizes++;
		size <<= 1; /* multiply by 2 */
	} while ((size <= mbMsgConfigInfo->maxMsg) && (mbMsgConfigInfo->maxMsg > 0));

	/* Allocate array of message objects in the message directory object */
	msgDirObj->numMsgObj = numMsgSizes;
	msgDirObj->msgObjArray = (message_object_t *) calloc(numMsgSizes, sizeof(message_object_t));
	if(msgDirObj->msgObjArray == NULL){
		rc = provideAllocErrorMsg("array of message objects", numMsgSizes * sizeof(message_object_t), __FILE__, __LINE__);
		return rc;
	}

	/* Loop through the message sizes and allocate message objects */
	size = mbMsgConfigInfo->minMsg;
	for(int i=0; i < numMsgSizes; i++) {
		message_object_t *msgObj = &msgDirObj->msgObjArray[i];
		msgObj->msgDirObj = msgDirObj;

		msgObj->payloadBuf = calloc(1, sizeof(concat_alloc_t));
		if (msgObj->payloadBuf == NULL) {
			return provideAllocErrorMsg("a payload buffer (concat_alloc_t)", sizeof(concat_alloc_t), __FILE__, __LINE__);
		}

		msgObj->payloadBuf->len = msgObj->payloadBuf->used = size;
		msgObj->payloadBuf->buf = calloc(msgObj->payloadBuf->len, sizeof(char));
		msgObj->payloadBuf->inheap = 1;
		if (msgObj->payloadBuf->buf == NULL && msgObj->payloadBuf->len != 0) {
			return provideAllocErrorMsg("a payload buffer char array", msgObj->payloadBuf->len, __FILE__, __LINE__);
		}

		for (int j=0; j < msgObj->payloadBuf->len; j++){
			msgObj->payloadBuf->buf[j] = fill[j % 8]; // fill message with 0xDEADBEEF
		}

		size <<= 1 ; /* multiply by 2 */
	}

	/* Insert message directory object for generated messages into the message directory hashmap */
	ism_common_putHashMapElement(mbMsgConfigInfo->msgDirMap, PARM_MSG_SIZES, 0, msgDirObj, NULL);

	return rc;
}

/* *************************************************************************************
 * parseArgs
 *
 * Description:  Parse arguments from the command line
 *
 *   @param[in]  argc                = Total # of command line parameters.
 *   @param[in]  argv                = Array of command line parameters.
 *
 *   @return 0   = Successful Completion
 *          >0   = An error and syntaxhelp is invoked.
 *          -1   = A general error that needs to be bubbled up.
 * *************************************************************************************/
int parseArgs (int argc, char **argv)
{
    int rc = 0;
    int i;
    int argIndex; /* used to walk the array of command line arguments */
    int cmdlineLen = 0;
    int minMsg = g_MsgCfgInfo->minMsg;
    int maxMsg = g_MsgCfgInfo->maxMsg;
    int tidx = 0;

    int appSimLoop = 0;
    int beginStatsSecs = 0;
    int chkMsgMask = 0;                  /* Mask for checking info (i.e. length, seq #) of a message. */
    int latencyMask = 0;                 /* Mask of latency measurements (see CHK* constants in the constants section above) */
    int maxInflightMsgs = 0;             /* Maximum number of inflight message IDs (default = 65535 == (64K - 1)). */
    int numSubmitThrds = 1;              /* Number of submitter threads to be used for this instance (default = 1) */
    int totalMsgPubCount = 0;            /* Total number of messages to be published and then stop publishing.  */
    int resetLatStatsSecs = 0;           /* Number of seconds to wait before clearing latency stats. */
    int resetTStampXMsgs = 0;            /* Number of messages to receive before starting statistics on receiver (this is for thruput) */
    int pingTimeoutSecs = MQTT_PING_DEFAULT_TIMEOUT;  /* The maximum number of seconds to wait for a PINGRESP from the server. */
    int pingIntervalSecs = 0;            /* For each client, the number of seconds between send of the PINGREQ message to the server.  By default clients do not send the PINGREQ message. */
	int snapInterval = 0;
    int testDuration = 0;                /* Duration in seconds of the test */
    int tmpVar;

    uint64_t lingerTime_ns= 0;           /* Time (Secs) converted to nano seconds to linger around before disconnecting. */

    int8_t  rateControl = 0;             /* determines what form of rate control to use by producer threads in mqttbench
                                          *      rateControl == -1          ; no rate control
                                          *      rateControl == 0 (default) ; individual producer threads control their own rate */

    uint8_t determineJitter = 0;         /* Flag to indicate jitter is enabled (1) or not (0) */

    uint8_t reconnectEnabled = 1;
	uint8_t disconnectType = DISCONNECT_WITHOUT_UNSUBSCRIBE;    /* default disconnect behavior is NOT to unsubscribe */
	uint8_t roundRobinSendSub = 1;       /* Flag to indicate that subscription should be done in round robin fashion. */
    uint8_t snapStatTypes = 0;
    uint8_t traceLevel = CHAR2NUM(DEFAULT_TRACELEVEL);
    uint8_t validArg = 0;

    /* --------------------------------------------------------------------------------
     * The following set of uint8_t's are used to ensure that the user didn't specify
     * the same argument more than once.   Also, BEAM Report was flagging this.
     * -------------------------------------------------------------------------------- */
	uint8_t parm_beginStatsSecs = 0;
	uint8_t parm_BurstMode = 0;
	uint8_t parm_ClientList = 0;
	uint8_t parm_ClientListRange = 0;
	uint8_t parm_ClientTrace = 0;
	uint8_t parm_CSVFile = 0;
	uint8_t parm_DumpClientInfo = 0;
	uint8_t parm_Duration = 0;
	uint8_t parm_ExitOnFirstDisconnect = 0;
	uint8_t parm_HistogramUnits_Conn = 0;
	uint8_t parm_HistogramUnits_Msg = 0;
	uint8_t parm_HistogramUnits_Reconnect = 0;
	uint8_t parm_instanceID = 0;
	uint8_t parm_JitterCheck = 0;
	uint8_t parm_Latency = 0;
	uint8_t parm_LatencyCSVFile = 0;
	uint8_t parm_Linger = 0;
	uint8_t parm_MaxInflightMsgs = 0;
	uint8_t parm_MsgSize = 0;
	uint8_t parm_NoDNSCache = 0;
	uint8_t parm_NoReconnect = 0;
	uint8_t parm_NoWait = 0;
	uint8_t parm_NumSubmitThrds = 0;
	uint8_t parm_PingInterval = 0;
	uint8_t parm_PingTimeout = 0;
	uint8_t parm_PredefinedMsg = 0;
	uint8_t parm_PreSharedKey = 0;
	uint8_t parm_Quit = 0;
	uint8_t parm_Rate = 0;
	uint8_t parm_RateControl = 0;
	uint8_t parm_Reset = 0;
	uint8_t parm_ResetLatStats = 0;
	uint8_t parm_SendCount = 0;
	uint8_t parm_SendSequential = 0;
	uint8_t parm_SimulateApp = 0;
	uint8_t parm_Snap = 0;
	uint8_t parm_TraceLevel = 0;
	uint8_t parm_Verify = 0;

    char *eos;
    char *tok;
    char *cMin;
    char *cMax;
    char *ip;
    char  errMsg[MIN_ERROR_STRING] = {0};
    char  pskInfoFN[MAX_INFO_FILENAME];

    mqttbench_t **mbInstArray = NULL;
    DIR *pDir = NULL;

    /* Flags */
    uint8_t rateSet = 0;

  	MBTRACE(MBINFO, 1, "Parsing command line parameters\n");

    /* If no parameters were passed in then provide syntax help and exit. */
    if (argc <= 1) {
    	MBTRACE(MBERROR, 1, "The -cl <client list file> command line parameter is required, but was not specified\n");
    	syntaxhelp(PARM_CLIENTLIST, NULL);
    	return 1;
    }

    /* Write the command line input to the log file and store for future reference. */
    pCmdLineArgs->stringCmdLineArgs = calloc(MAX_CMDLINE_LEN, sizeof(char));
    if (pCmdLineArgs->stringCmdLineArgs) {
    	for ( argIndex = 1 ; argIndex < argc ; argIndex++ ) {
    		cmdlineLen += strlen(argv[argIndex] + 1);
    		if (cmdlineLen < MAX_CMDLINE_LEN) {
    			strcat(pCmdLineArgs->stringCmdLineArgs, argv[argIndex]);
    			strcat(pCmdLineArgs->stringCmdLineArgs, " ");
    		} else
    			break;
    	}
    } else {
    	rc = provideAllocErrorMsg("storing information", MAX_CMDLINE_LEN, __FILE__, __LINE__);
    	return rc;
    }

    /* --------------------------------------------------------------------------------
     * Walk the array of command line arguments skipping past argv[0] (i.e. the
     * program name).
     * -------------------------------------------------------------------------------- */
    for ( argIndex = 1 ; argIndex < argc ; argIndex++ )
    {
        if (rc)
        	break;

        tmpVar = 0;    /* Reset tmpVar */

        /* ----------------------------------------------------------------------------
         * option:  -help
         *
         * Provide the functions that are available for the doCommand screen.
         * ---------------------------------------------------------------------------- */
        if ((strcmp(argv[argIndex], "--help") == 0 ) ||
            (strcmp(argv[argIndex], "-h") == 0) ||
            (strcmp(argv[argIndex], "-?") == 0) ||
            (strcmp(argv[argIndex], "?") == 0))
        {
            syntaxhelp(HELP_ALL, "\n");
            rc = 1;
            break;
        }

        /* ----------------------------------------------------------------------------
         * option:  -as
         *
         * Simulate Application message processing of a message.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-as") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified and
        	 * if an additional argument is needed.  Increment the argument index to
        	 * get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_SimulateApp++, "-as", 1, argc, argIndex++);
        	if (rc == 0) {
        		/* Check if the next argument is a valid number. */
        		validArg = valStrIsNum(argv[argIndex]);
               	if (validArg == 1) {
                    /* Set global variable - Simulator loop count. */
               		tmpVar = atoi(argv[argIndex]);
                   	/* Check that appSimLoop is greater than 0.*/
                   	if (tmpVar < 1)
                   		validArg = 0;
               	}

               	/* Check that the argument was valid. If not, provide error message. */
               	if (validArg == 0) {
               		syntaxhelp(PARM_APPSIM, "A valid value for the appSimLoop parameter (-as) was not specified. Must be greater than 0.");
                   	rc = RC_INVALID_VALUE;
               		break;
               	} else
                   	appSimLoop = tmpVar;
            } else {
            	break;
            }

        	continue;
        } /* -as option */

        /* ----------------------------------------------------------------------------
         * option:  -b
         *
         * Begin statistics after x secs.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-b") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified and
        	 * if an additional argument is needed. Increment the argument index to
        	 * get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_beginStatsSecs++, "-b", 1, argc, argIndex++);
        	if (rc == 0) {
        		/* Check if the next argument is a valid number. */
        		validArg = valStrIsNum(argv[argIndex]);
        		if (validArg == 1) {
                    /* Set local variable - Seconds to wait before collecting statistics. */
                    tmpVar = atoi(argv[argIndex]);
                    /* Check that beginStatsSecs is greater than 0. */
                    if (tmpVar < 1)
                    	validArg = 0;
        		}

               	/* Check that the argument was valid. If not, provide error message. */
                if (validArg == 0) {
                    syntaxhelp(PARM_BSTATS, "Begin statistics is not valid (units in seconds).");
                    rc = RC_INVALID_VALUE;
                    break;
                } else
                	beginStatsSecs = tmpVar;
        	} else
        		break;

        	continue;
        } /* -b option */

        /* ----------------------------------------------------------------------------
         * option:  --burst
         *
         * Burst mode which has the interval of when to burst, how long to burst and
         * the burst rate.   The specified rate can be either faster or slower then
         * the current message rate.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "--burst") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified and
        	 * if an additional argument is needed. Increment the argument index to
        	 * get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_BurstMode++, "--burst", 1, argc, argIndex++);
        	if (rc == 0) {
            	/* --------------------------------------------------------------------
            	 * Check to see if the user specified the 3 additional arguments with
            	 * the --burst keyword.  They must be comma separated.
            	 *
            	 *     [interval,duration,rate]
            	 *
            	 *     where:
            	 *            interval  - How often in seconds to cause a burst.
            	 *            duration  - How long in seconds the burst should last.
            	 *            rate      - Rate at which to send during a burst.
            	 * -------------------------------------------------------------------- */
        		int64_t burstInterval = 0;
        		int64_t burstDuration = 0;
        		double  burstMsgRate = 0;
        		int     currIdx = 0;

               	if ((argIndex) < argc) {
               		if (memcmp(argv[(argIndex)], "-", 1) != 0) {
           				ip = argv[argIndex];

           				char *savePtrComma = NULL;  /* used by strtok_r to save next token */
           				char *fullString, *token;

           				fullString = strdup(ip);
           				token = strtok_r(fullString, ",", &savePtrComma);

           				while (token) {
           					switch (currIdx) {
           						case 0:
           							if (valStrIsNum(token) == 1) {
           								burstInterval = (strtoul(token, &eos, 0)) * NANO_PER_SECOND;
           								if (burstInterval < 0) {
           									MBTRACE(MBERROR, 1, "Burst interval is < 0.\n");
           									rc = RC_INVALID_VALUE;
           									break;
           								}
           							} else {
               							syntaxhelp(PARM_BURST, "Burst interval is not valid (units in seconds).");
               							rc = RC_INVALID_VALUE;
           							}
           							break;
           						case 1:
           							if (valStrIsNum(token) == 1) {
           								burstDuration = (strtoul(token, &eos, 0)) * NANO_PER_SECOND;
           								if (burstDuration < 0) {
           									MBTRACE(MBERROR, 1, "Burst duration is < 0.\n");
           									rc = RC_INVALID_VALUE;
           									break;
           								}
           							} else {
               							syntaxhelp(PARM_BURST, "Burst duration is not valid (units in seconds).");
               							rc = RC_INVALID_VALUE;
           							}
           							break;
           						case 2:
           							if (valStrIsNum(token) == 1) {
               							burstMsgRate = strtod(token, &eos);
               							if (burstMsgRate < 0) {
           									MBTRACE(MBERROR, 1, "Burst message rate is < 0.\n");
           									rc = RC_INVALID_VALUE;
           									break;
               							}
           							} else {
               							syntaxhelp(PARM_BURST, "Burst rate is not valid.");
               							rc = RC_INVALID_VALUE;
           							}
           							break;
           						default:
           							syntaxhelp(PARM_BURST, "Invalid option with '--burst'");
									rc = RC_INVALID_OPTION;
           							break;
           					}

           					currIdx++;

           					token = strtok_r(NULL, ",", &savePtrComma);
           				} /* while (token) */

           				free(fullString);

                		if (rc == 0) {
                			pBurstInfo = (mbBurstInfo_t *)calloc(1, (int)sizeof(mbBurstInfo_t));
                			if (pBurstInfo) {
                				pBurstInfo->burstMsgRate = burstMsgRate;
                				pBurstInfo->burstDuration = burstDuration;
                				pBurstInfo->currMode = BASE_MODE;
                				pBurstInfo->burstInterval = burstInterval;
                				g_MqttbenchInfo->burstModeEnabled = 1;
                				g_MqttbenchInfo->mbBurstInfo = pBurstInfo;
                			} else {
                				rc = provideAllocErrorMsg("the burst structure", sizeof(mbBurstInfo_t), __FILE__, __LINE__);
                				break;
                			}
                		}
           			}
               	}
        	} else
        		break;

        	continue;
        } /* --burst option */

        /* ----------------------------------------------------------------------------
         * option:  -c
         *
         * Total number of messages to send across all producers, which includes
         * every destination, and stop.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-c") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_SendCount++, "-c", 1, argc, argIndex++);
        	if (rc == 0) {
        		/* Check if the next argument is a valid number. */
        		validArg = valStrIsNum(argv[argIndex]);
        		if (validArg == 1) {
                    /* Set local variable - Destination Message Count. */
                    tmpVar = atoi(argv[argIndex]);
                    /* Check that perDestMsgCount is not negative. */
                    if (tmpVar < 0)
                    	validArg = 0;
        		}

               	/* Check that the argument was valid. If not, provide error message. */
                if (validArg == 0) {
                    syntaxhelp(PARM_COUNT, "Message count is not valid (units in messages).");
                    rc = RC_INVALID_VALUE;
                    break;
                } else
                	totalMsgPubCount = tmpVar;
        	} else
        		break;

            continue;
        } /* -c option */

        /* ----------------------------------------------------------------------------
         * option:  -cl  (client list file format is JSON)
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-cl") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_ClientList++, "-cl", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - Client List File Name. */
            	pCmdLineArgs->clientListPath = calloc(strlen(argv[argIndex]) + 1, sizeof(char));
                strcpy(pCmdLineArgs->clientListPath, argv[argIndex]);

                /* Check to see if the file really exists. */
                rc = checkFileExists(pCmdLineArgs->clientListPath);
                if (rc) {
                  	MBTRACE(MBERROR, 1, "File specified with -cl doesn't exist (%s).\n", pCmdLineArgs->clientListPath);
                   	break;
                }
        	} else
        		break;

            continue;
        } /* -cl option */

        /* ----------------------------------------------------------------------------
         * option:  -crange  or  --crange
         *
         * When using client list this will specify the range of clientIDs to use in
         * the client list.
         * ---------------------------------------------------------------------------- */
        if ((strcmp(argv[argIndex], "-crange") == 0) || (strcmp(argv[argIndex], "--crange") == 0)) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_ClientListRange++, "--crange", 1, argc, argIndex++);
        	if (rc == 0) {
        		pCmdLineArgs->crange = strdup(argv[argIndex]);
        		if (pCmdLineArgs->crange == NULL) {
        			provideAllocErrorMsg("-crange command line parameter", strlen(argv[argIndex]), __FILE__, __LINE__);
        			break;
        		}
        	} else
        		break;

        	continue;
        } /* -crange option */

        /* ----------------------------------------------------------------------------
         * option:  -csv
         *
         * CSV Logfile to capture the data in for throughput ....etc.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-csv") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_CSVFile++, "-csv", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - CSV Log Filename. */
           		csvLogFile = realloc(csvLogFile, (strlen(argv[argIndex]) + 1));
           		if (csvLogFile != NULL) {
           			strcpy(csvLogFile, argv[argIndex]);

                   	/* Remove the previous file if it exists. */
                   	removeFile(csvLogFile, REMOVE_FILE_EXISTS);

           			/* Attempt to open the data file for writing */
           			g_fpCSVLogFile = fopen(csvLogFile,"w");
           			if (g_fpCSVLogFile == NULL) {
           				MBTRACE(MBERROR, 1, "Unable to open the file (%s) for writing.  Check file permissions,  exiting...\n",
			                                csvLogFile);
           				rc = RC_FILEIO_ERR;
           				g_MBErrorRC = RC_FILEIO_ERR;
           				break;
           			}
           		} else {
           			errMsg[0] = '\0';
           			snprintf(errMsg, MIN_ERROR_STRING, "the CSV Log Filename: %s", argv[argIndex]);
           			rc = provideAllocErrorMsg(errMsg, (strlen(argv[argIndex]) + 1), __FILE__, __LINE__);
           			g_MBErrorRC = rc;
           			break;
           		}
        	} else
        		break;

        	continue;
        } /* -csv option */

        /* ----------------------------------------------------------------------------
         * option:  -cu
         *
         * Histogram Units for array elements when performing connection latency.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-cu") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_HistogramUnits_Conn++, "-cu", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - Latency Units. */
               	g_UnitsConn = 1.0 / strtod(argv[argIndex], &eos);
               	if ((g_UnitsConn == 0) && (*eos)) {
               		syntaxhelp(PARM_CUNITS, "Units for connection latency measurements (-cu) is not a valid floating point number.");
                   	rc = RC_INVALID_VALUE;
               		break;
               	}
           	} else
        		break;

        	continue;
        } /* -cu option */

        /* ----------------------------------------------------------------------------
         * option:  -d
         *
         * Test duration.   Valid values:
         *
         *  0 = Run forever
         *  N = Terminate after running for N seconds.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-d") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_Duration++, "-d", 1, argc, argIndex++);
        	if (rc == 0) {
        		/* Check if the next argument is a valid number. */
        		validArg = valStrIsNum(argv[argIndex]);
        		if (validArg == 1) {
                    /* Set local variable - Duration Amount*/
                    tmpVar = atoi(argv[argIndex]);
                    /* Check that testDuration is not negative. */
                    if (tmpVar < 0)
                    	validArg = 0;
        		}

               	/* Check that the argument was valid. If not, provide error message. */
                if (validArg == 0) {
               		syntaxhelp(PARM_DURATION, "Test duration (-d command line parameter) must be a positive integer (units are seconds).");
               		rc = RC_INVALID_VALUE;
               		break;
               	} else
               		testDuration = tmpVar;
            } else {
            	break;
            }

    		continue;
        } /* -d option */

        /* ----------------------------------------------------------------------------
         * option:  -dci
         *
         * Dump client information
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-dci") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_DumpClientInfo++, "-dci", 0, argc, argIndex);
        	if (rc == 0)
            	pCmdLineArgs->dumpClientInfo = 1;
        	else
            	break;

            continue;
        } /* -dci option */
        /* ----------------------------------------------------------------------------
         * option:  -efd
         *
         * Exit on 1st disconnect
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-efd") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_ExitOnFirstDisconnect++, "-efd", 0, argc, argIndex);
            if (rc == 0)
            	pCmdLineArgs->exitOnFirstDisconnect = 1;
            else
            	break;

            continue;
        } /* -efd option */

        /* ----------------------------------------------------------------------------
         * option:  -env
         *
         * Display the environment variable values to stdout.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-env") == 0) {
        	pCmdLineArgs->displayEnvInfo = 1;
        	continue;
        } /* -env option */

        /* ----------------------------------------------------------------------------
		 * option:  -i
		 *
		 * Set the instance ID for this instance of mqttbench (default is 0)
		 * ---------------------------------------------------------------------------- */
		if (strcmp(argv[argIndex], "-i") == 0) {
			/* ------------------------------------------------------------------------
			 * Perform a check on the parameter whether it was already specified
			 * and if an additional argument is needed. Increment the argument
			 * index to get the next arg value.
			 * ------------------------------------------------------------------------ */
			rc = checkParms(parm_instanceID++, "-i", 1, argc, argIndex++);
			if (rc == 0) {
				/* Get next argument - instance ID */
				g_MqttbenchInfo->instanceID = strtoul(argv[argIndex], &eos, 0);
				if (argv[argIndex] == eos || *eos != '\0') {
					MBTRACE(MBERROR, 1, "Invalid ID value specified for -i command line parameter: %s\n", argv[argIndex]);
					rc = RC_INVALID_VALUE;
					break;
				}
			} else
				break;

			continue;
		} /* -i option */

        /* ----------------------------------------------------------------------------
         * option:  -jitter  or  --jitter
         *
         * Check for jitter on the thruput rates.
         * ---------------------------------------------------------------------------- */
        if ((strcmp(argv[argIndex], "-jitter") == 0) || (strcmp(argv[argIndex], "--jitter") == 0)) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
           	rc = checkParms(parm_JitterCheck++, "--jitter", 1, argc, argIndex++);

        	if (rc == 0) {
            	int jitterInterval = MB_JITTER_INTERVAL;
            	int jitterSampleCt = MB_JITTER_SAMPLECT;

                if ((argIndex + 1) < (argc)) {
                    /* Increment the argument index to get the arg value. */
                	jitterInterval = atoi(argv[argIndex++]);
                	jitterSampleCt = atoi(argv[argIndex]);

                	if (jitterInterval <= 0) {
                        MBTRACE(MBERROR, 1, "Jitter interval must be greater than 0 (%d).\n",
                        		            jitterInterval);
                        rc = RC_INVALID_VALUE;
                        break;
                	}

                	if (jitterSampleCt <= 0) {
                		MBTRACE(MBERROR, 1, "Jitter sample count must be greater than 0 (%d).\n",
                				            jitterSampleCt);
                        rc = RC_INVALID_VALUE;
                        break;
                	}

                	pJitterInfo = calloc(1, sizeof(mbJitterInfo_t));
                	if (pJitterInfo) {
               			pJitterInfo->interval = jitterInterval;
               			pJitterInfo->sampleCt = jitterSampleCt;
               			pJitterInfo->rxQoSMask = -1;
               			pJitterInfo->txQoSMask = -1;
                       	determineJitter = 1;
                       	g_MqttbenchInfo->mbJitterInfo = pJitterInfo;
                       	rc = 0;
                	} else {
                    	rc = provideAllocErrorMsg("the structure to measure jitter", (int)sizeof(mbJitterInfo_t),
												  __FILE__, __LINE__);
                		break;
                	}
                }
        	} else
        		break;

            continue;
        } /* -jitter option */

        /* ----------------------------------------------------------------------------
         * option:  -l
         *
         * Number of seconds to stay connected in order to provide more graceful
         * disconnection rather than all connections dropping at once.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-l") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_Linger++, "-l", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - Linger Seconds. */
              	lingerTime_ns = strtoull(argv[argIndex], &eos, 0);
               	if (lingerTime_ns > 0)
               		lingerTime_ns *= NANO_PER_SECOND;
               	else {
               		MBTRACE(MBERROR, 1, "Invalid value specified for -l\n");
               		rc = RC_INVALID_VALUE;
               		break;
                }
            } else
            	break;

            continue;
        } /* -l option */

        /* ----------------------------------------------------------------------------
         * option:  -lcsv
         *
         * Latency CSV Filename
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-lcsv") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_LatencyCSVFile++, "-lcsv", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - Latency csv Filename. */
                csvLatencyFile = realloc(csvLatencyFile, (strlen(argv[argIndex]) + 1));
                if (csvLatencyFile != NULL) {
                    strcpy(csvLatencyFile, argv[argIndex]);

                   	/* Remove the previous file if it exists. */
                   	removeFile(csvLatencyFile, REMOVE_FILE_EXISTS);

                    /* Attempt to open the data file for writing */
                    g_fpCSVLatencyFile = fopen(csvLatencyFile,"w");
                    if (g_fpCSVLatencyFile == NULL) {
                        MBTRACE(MBERROR, 1, "Unable to open the file (%s) for writing.  " \
                        		            "Check file permissions, exiting...\n",
                                            csvLatencyFile);
                        rc = RC_FILEIO_ERR;
                        g_MBErrorRC = RC_FILEIO_ERR;
                        break;
                    }
                } else {
                	errMsg[0] = '\0';
                	snprintf(errMsg, MIN_ERROR_STRING, "the CSV Latency File: %s\n", argv[argIndex]);
                	rc = provideAllocErrorMsg(errMsg, (strlen(argv[argIndex]) + 1), __FILE__, __LINE__);
                    g_MBErrorRC = rc;
                    break;
                }
        	} else
        		break;

        	continue;
        } /* -lcsv option */

        /* ----------------------------------------------------------------------------
         * option:  -M
         *
         * Using predefined messages in the directory specified.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-M") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_PredefinedMsg++, "-M", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - Directory for message files. */
				g_predefinedMsgDir = strdup(argv[argIndex]);
				if (g_predefinedMsgDir) {
                        /* Check to see if the directory exists and if not provide error message. */
					if ((pDir = opendir(g_predefinedMsgDir)) == NULL) {
						MBTRACE(MBERROR, 1,	"Directory specified doesn't exist:  %s\n",	g_predefinedMsgDir);
                        rc = RC_DIRECTORY_NOT_FOUND;
                        break;
                    } else {
                        rc = 0;
                        closedir(pDir);
                    }
				} else {
					errMsg[0] = '\0';
					snprintf(errMsg, MIN_ERROR_STRING, "the predefined message directory variable: %s\n", argv[argIndex]);
					rc = provideAllocErrorMsg(errMsg, strlen(argv[argIndex]), __FILE__, __LINE__);
					break;
				}
        	} else {
        		break;
        	}

            continue;
        } /* -M option */

        /* ----------------------------------------------------------------------------
         * option:  -mim  or  --mim
         *
         * Maximum InFlight Messages
         * ---------------------------------------------------------------------------- */
        if ((strcmp(argv[argIndex], "-mim") == 0) || (strcmp(argv[argIndex], "--mim") == 0)) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_MaxInflightMsgs++, "--mim", 1, argc, argIndex++);
        	if (rc == 0) {
          		/* Check if the next argument is a valid number. */
           		validArg = valStrIsNum(argv[argIndex]);
           		if (validArg == 1) {
           			/* Set local variable - Maximum Message ID for QoS 1 and 2. */
           			tmpVar = atoi(argv[argIndex]);
                    /* Check that maxInflightMsgs is not less than 1 and greater 64K */
                    if ((tmpVar < 1) || (tmpVar > MQTT_MAX_MSGID))
                       	validArg = 0;
           		}

                /* Check that the argument was valid. If not, provide error message. */
                if (validArg == 0) {
                  	MBTRACE(MBERROR, 1, "-mim %d cannot exceed the maximum number of messages in flight allowed by the MQTT spec %d\n", tmpVar, MQTT_MAX_MSGID);
                   	rc = RC_INVALID_VALUE;
                   	break;
                } else {
                	maxInflightMsgs = tmpVar;
                }
        	} else
        		break;

        	continue;
        } /* -mim option */

        /* ----------------------------------------------------------------------------
         * option:  -nodnscache
         *
         * Resolve the DNS for every reconnect.
         * ---------------------------------------------------------------------------- */
        if ((strcmp(argv[argIndex], "-nodnscache") == 0) || (strcmp(argv[argIndex], "--nodnscache") == 0)) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_NoDNSCache++, "-nodnscache", 0, argc, argIndex++);
        	if (rc == 0) {
				MBTRACE(MBINFO, 1, "-nodnscache specified on the command line. Client will re-resolve the dns during a reconnect.\n");
				pCmdLineArgs->reresolveDNS = 1;
        	} else
        		break;

        	continue;
        } /* -nodnscache option */

        /* ----------------------------------------------------------------------------
         * option:  -noreconn  or  --noreconn
         *
         * Work Item # 93910
         *
         * Disable reconnect.
         * ---------------------------------------------------------------------------- */
        if ((strcmp(argv[argIndex], "-noreconn") == 0) || (strcmp(argv[argIndex], "--noreconn") == 0)) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_NoReconnect++, "--noreconn", 0, argc, argIndex);
        	if (rc == 0) {
				MBTRACE(MBINFO, 1, "-noreconn specified on the command line. Client will not reconnect when a connection is closed.\n");
        		reconnectEnabled = 0;
        	} else
        		break;

            continue;
        } /* -noreconn option */

        /* ----------------------------------------------------------------------------
         * option:  -nw
         *
         * Don't delay publishing for client to connect/subscribe
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-nw") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_NoWait++, "-nw", 0, argc, argIndex);
        	if (rc == 0) {
            	pCmdLineArgs->noWaitFlag = 1;
            	g_WaitReady = 0; // user indicated waiting for clients to complete connections/subscriptions should be disabled
        	} else
                break;

            continue;
        } /* -nw option */

        /* ----------------------------------------------------------------------------
         * option:  -pi 
         *
         * for each client this is the interval in seconds between PINGREQ messages sent to the server 
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-pi") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_PingInterval++, "-pi", 1, argc, argIndex++);
        	if (rc == 0) {
        		/* Check if the next argument is a valid number. */
        		validArg = valStrIsNum(argv[argIndex]);
        		if (validArg == 1) {
                    /* Set local variable - Number of seconds between PINGREQ messages. */
        			tmpVar = atoi(argv[argIndex]);
                    /* Verify value is greater than 0. */
                    if (tmpVar < 1)
                    	validArg = 0;
        		}

               	/* Check that the argument was valid. If not, provide error message. */
                if (validArg == 0) {
               		syntaxhelp(HELP_ALL, "ping interval is not valid (units in seconds).");
               		rc = RC_INVALID_VALUE;
               		break;
               	} else
               		pingIntervalSecs = tmpVar;
        	} else
        		break;

    		continue;
        } /* -pi option */

		/* ----------------------------------------------------------------------------
         * option:  -pt
         *
         * maximum number of seconds to wait for PINGRESP from server after sending PINGREQ
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-pt") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_PingTimeout++, "-pt", 1, argc, argIndex++);
        	if (rc == 0) {
        		/* Check if the next argument is a valid number. */
        		validArg = valStrIsNum(argv[argIndex]);
        		if (validArg == 1) {
                    /* Set local variable - Number of seconds to wait for PINGRESP messages. */
        			tmpVar = atoi(argv[argIndex]);
                    /* Verify value is greater than 0. */
                    if (tmpVar < 1)
                    	validArg = 0;
        		}

               	/* Check that the argument was valid. If not, provide error message. */
                if (validArg == 0) {
               		syntaxhelp(HELP_ALL, "ping timeout is not valid (units in seconds).");
               		rc = RC_INVALID_VALUE;
               		break;
               	} else
               		pingTimeoutSecs = tmpVar;
        	} else
        		break;

    		continue;
        } /* -pi option */

        /* ----------------------------------------------------------------------------
         * option:  -psk
         *
         * Work Item # 74125
         *
         * Provide an ability to pass in a list of IDs and PreShared keys which was
         * needed for customer POC.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-psk") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_PreSharedKey++, "-psk", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - PSK Filename. */
                strcpy(pskInfoFN, argv[argIndex]);

                /* Check to see if the file really exists. */
                rc = checkFileExists(pskInfoFN);
                if (rc) {
                  	MBTRACE(MBERROR, 1, "File specified with -psk doesn't exist (%s).\n", pskInfoFN);
                   	break;
                }

                /* Allocate the structure that contains the PreShared Key info. */
                pIDSharedKeyArray = calloc(1, sizeof(pskArray_t));
                if (pIDSharedKeyArray) {
                   	/* Determine the longest entry if negative return code then file doesn't exist. */
                   	rc = read_psk_lists_from_file(FIND_LONGEST_ENTRY, pskInfoFN, pIDSharedKeyArray);
                   	if (rc == 0) {
                   		/* Malloc the arrays that have entries. */
                   		if (pIDSharedKeyArray->id_Count > 0) {
                   			if (pIDSharedKeyArray->id_Count == pIDSharedKeyArray->key_Count) {
                   				/* Check to make sure the key is not larger than the bit array. */
                   				if ((pIDSharedKeyArray->key_Longest * 2) > MAX_BIT_ARRAY) {
               						MBTRACE(MBERROR, 1, "The largest PreShared Key is too large (MAX: %d)\n", MAX_BIT_ARRAY);
               						rc = RC_PSK_TOO_LARGE;
               						break;
                   				}

                   				pIDSharedKeyArray->pskIDArray = (char**)calloc(pIDSharedKeyArray->id_Count, sizeof(char *));
                   				if (pIDSharedKeyArray->pskIDArray == NULL) {
                   			    	rc = provideAllocErrorMsg("PreShared Key ID Array",
                   			    			                  (int)(sizeof(char *) * pIDSharedKeyArray->id_Count),
															  __FILE__, __LINE__);
                   					break;
                   				}

                   				if (pIDSharedKeyArray->key_Count > 0) {
                   					pIDSharedKeyArray->pskKeyArray = (char**)calloc(pIDSharedKeyArray->key_Count, sizeof(char *));
                   					if (pIDSharedKeyArray->pskKeyArray == NULL) {
                   				    	rc = provideAllocErrorMsg("PreShared Key Array",
                   				    			                 (int)(sizeof(char *) * pIDSharedKeyArray->key_Count),
																 __FILE__, __LINE__);
                   						break;
                   					}
                   				}
                   			} else {
                   				MBTRACE(MBERROR, 1, "The number of ID (%d) and PreShared Keys (%d) are different.\n",
                   					                pIDSharedKeyArray->id_Count,
                   					                pIDSharedKeyArray->key_Count);
                   				rc = RC_PARAMETER_MISMATCH;
                   				break;
                   			}
                   		}

               			rc = read_psk_lists_from_file(LOAD_ARRAY, pskInfoFN, pIDSharedKeyArray);
               			if (rc)
               				break;
                   	} else
                   		break;
                } else {
                   	rc = provideAllocErrorMsg("PreShared Key Array", (int)sizeof(pskArray_t), __FILE__, __LINE__);
                   	break;
                }
        	} else
        		break;

            continue;
        } /* -psk option */

        /* ----------------------------------------------------------------------------
         * option:  -quit  or  --quit
         *
         * Specifies the type of termination to perform when the timer expires.  The
         * default is 0 which performs in order: MQTT UNSUBSCRIBE, MQTT DISCONNECT and
         * closes the TCP Connection.
         *
         * Value     Description
         * =====     ========================================================
         *  0        Perform MQTT UNSUBSCRIBE, MQTT DISCONNECT and close TCP
         *           Connection.
         *  1        Perform MQTT DISCONNECT and close TCP Connection
         * ---------------------------------------------------------------------------- */
        if ((strcmp(argv[argIndex], "-quit") == 0) || (strcmp(argv[argIndex], "--quit") == 0)) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_Quit++, "--quit", 1, argc, argIndex++);
        	if (rc == 0) {
            	int closeType;

                /* Get next argument - Type of quit. */
                closeType = atoi(argv[argIndex]);

                if (closeType == 1)
                    disconnectType = 1;
                else if ((closeType > 1) || (closeType < 0)) {
                    syntaxhelp(PARM_QUIT, "-quit only supports arguments 0 (default) or 1.");
                    rc = RC_INVALID_VALUE;
                    break;
                }
        	} else
        		break;

            continue;
        } /* -quit option */

        /* ----------------------------------------------------------------------------
         * option:  -r
         *
         * Rate at which to send messages.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-r") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_Rate++, "-r", 1, argc, argIndex++);
        	if (rc == 0) {
               	rateSet = 1;
				g_MsgRate = atof(argv[argIndex]);

				/* Check that globalMsgRate is a number */
				if ((validateRate(argv[argIndex]) == 0) || (g_MsgRate <= 0)) {
					syntaxhelp(PARM_MSGRATE, "Message rate is not valid (units in messages/second).");
					rc = RC_INVALID_VALUE;
					break;
				}
        	} else
        		break;

            continue;
        } /* -r option */

        /* ----------------------------------------------------------------------------
         * option:  -rl
         *
         * Work Item # 151043
         *
         * Perform a reset on the latency stats based on the amount of seconds specified.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-rl") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_ResetLatStats++, "-rl", 1, argc, argIndex++);
        	if (rc == 0) {
        		/* Check if the next argument is a valid number. */
        		validArg = valStrIsNum(argv[argIndex]);
        		if (validArg == 1) {
                    /* Set local variable - Amount of time between resetting latency stats. */
        			tmpVar = atoi(argv[argIndex]);
                    /* Check that resetLatStatsSecs is greater than 60 and greater than 0. */
        			if (tmpVar < 60) {
        				if (tmpVar > 1) {
        					MBTRACE(MBWARN, 1, "Reset in-memory Latency minimum value is 60 seconds,  disabling...\n");
        					fprintf(stdout, "(w) Reset in-memory Latency minimum value is 60 seconds,  disabling...\n");
        					fflush(stdout);
        					tmpVar = 0;
        				} else
        					validArg = 0;
        			}
        		}

               	/* Check that the argument was valid. If not, provide error message. */
                if (validArg == 0) {
                    syntaxhelp(PARM_RESETLAT, "Reset in-memory Latency is not valid (units in seconds).");
                    rc = RC_INVALID_VALUE;
                    break;
                } else
    				resetLatStatsSecs = tmpVar;
        	} else
            	break;

            continue;
        } /* -rl option */

        /* ----------------------------------------------------------------------------
         * option:  -rr
         *
         * Reset the timestamp after x number of messages on the receive side,
         * which is needed for fan-in situation.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-rr") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_Reset++, "-rr", 1, argc, argIndex++);
        	if (rc == 0) {
        		/* Check if the next argument is a valid number. */
        		validArg = valStrIsNum(argv[argIndex]);
        		if (validArg == 1) {
                    /* Set local variable - Number of Received msgs before reset. */
                  	tmpVar = atoi(argv[argIndex]);
                    /* Check that resetTStampXMsgs is greater than 0. */
                    if (tmpVar < 1)
                    	validArg = 0;
        		}

               	/* Check that the argument was valid. If not, provide error message. */
                if (validArg == 0) {
                    syntaxhelp(PARM_RSTATS, "Reset timestamp after x messages is not valid (units in messages).");
                    rc = RC_INVALID_VALUE;
                    break;
                } else
                	resetTStampXMsgs = tmpVar;
        	} else
        		break;

        	continue;
        } /* -rr option */

        /* ----------------------------------------------------------------------------
         * option:  -ru
         *
         * Histogram Units for array elements when performing message latency and
         * reconnects are occurring.  This was specifically added for deploys.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-ru") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_HistogramUnits_Reconnect++, "-ru", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - Latency Units. */
               	g_UnitsReconnRTT = 1.0 / strtod(argv[argIndex], &eos);
               	if ((g_UnitsReconnRTT == 0) && (*eos)) {
               		syntaxhelp(PARM_RUNITS, "Units for reconnection latency measurements (-ru) is not a valid floating point number.");
                   	rc = RC_INVALID_VALUE;
               		break;
               	}
           	} else
        		break;

        	continue;
        } /* -ru option */

        /* ----------------------------------------------------------------------------
         * option:  -s
         *
         * Size of message to send.  There is a range:  <min>-<max>
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-s") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_MsgSize++, "-s", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - Message Sizes. */
                tok = argv[argIndex];
                cMin = strtok(tok, "-");
                cMax = strtok(NULL, " ");

                /* Check that minMsg and maxMsg are numbers */
     			if ((cMin == NULL) || (cMax == NULL) ||
       				(valStrIsNum(cMin) == 0) || (valStrIsNum(cMax) == 0))
                {
					syntaxhelp(PARM_MSGSIZE, "Message length is not valid.\n\n" \
							                 "Format of -s parameter is: <min>-<max>\n\n" \
	    					                 "\twhere: <min> and <max> are positive integer values (units in bytes).");

                    rc = RC_INVALID_VALUE;
                    break;
                } else {
                    minMsg = atoi(cMin);
                    maxMsg = atoi(cMax);
                }
     			/* --------------------------------------------------------------------
				 * -s 0-0 is a special case in which zero length messages will be
				 * generated. One use case for zero length messages is to clear retained
				 * messages in the message broker.
				 * -------------------------------------------------------------------- */
     			if ((minMsg == 0) && (maxMsg == 0))
     				g_MsgCfgInfo->specSendMsgLenZero = 1;

     			/* --------------------------------------------------------------------
     			 * If not the -s 0-0 case, then perform some additional validation
     			 * on minMsg and maxMsg
     			 * -------------------------------------------------------------------- */
     			if (g_MsgCfgInfo->specSendMsgLenZero == 0) {
     				/* Need to check that the minMsg is at least equal to the length of the header. */
     				if (minMsg < MIN_MSG_SIZE) {
     					MBTRACE(MBWARN, 1, "The <min> message length must be at least %d bytes in length. \n" \
     									   "Modifying <min> to that length.\n",
     									   MIN_MSG_SIZE);
     					minMsg = MIN_MSG_SIZE;
     				}

     				/* ----------------------------------------------------------------
     				 * Check to see that minMsg and maxMsg are greater than equal to
     				 * the minimum messages size.   If not then provide message that
     				 * the size is being increased to the minimum supported message.
     				 * ---------------------------------------------------------------- */
         			if (maxMsg < MIN_MSG_SIZE) {
     					MBTRACE(MBWARN, 1, "The <max> message length must be at least %d bytes in length. \n" \
     									   "Modifying <max> to that length.\n",
     									   MIN_MSG_SIZE);
     					maxMsg = MIN_MSG_SIZE;
         			}

     				/* ----------------------------------------------------------------
     				 * Check to see that maxMsg isn't smaller than minMsg. If so
     				 * then reset the maxMsg to the minMsg size.
     				 * ---------------------------------------------------------------- */
     				if (minMsg > maxMsg) {
     					MBTRACE(MBERROR, 1, "The <min> message length must be smaller than the <max> message length.  "
     										"Message Sizes (min/max): %d / %d\n",
     									    minMsg,
     									    maxMsg);
     					rc = RC_INVALID_VALUE;
     					break;
     				}
     			} /* if (specSendMsgLenZero == 0) */

     			g_MsgCfgInfo->minMsg = minMsg;
     			g_MsgCfgInfo->maxMsg = maxMsg;
        	} else
        		break;

            continue;
        } /* -s option */

        /* ----------------------------------------------------------------------------
         * option:  -snap or --snap
         *
         * Snapshot interval for taking latency information
         * ---------------------------------------------------------------------------- */
        if ((strcmp(argv[argIndex], "-snap") == 0) || (strcmp(argv[argIndex], "--snap") == 0)) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
           	rc = checkParms(parm_Snap++, "--snap", 1, argc, argIndex++);

        	if (rc == 0) {
            	/* --------------------------------------------------------------------
            	 * Check to see if the user specified the stat types to collect,
            	 * which is a comma separated list, and/or the interval.  The
            	 * stat types can be mixed together with the valid types are:
            	 *   - connects
            	 *   - connlatency
            	 *   - msgrates
            	 *   - msgcounts
            	 *   - msglatency
            	 * -------------------------------------------------------------------- */
        		for ( i = 0 ; i < 2 ; i++ ) {
                	if ((argIndex) < argc) {
                		if (memcmp(argv[argIndex], "-", 1) != 0) {
            				ip = argv[argIndex];

                			if (i == 0) {
                				char *savePtrComma = NULL;  /* used by strtok_r to save next token */
                				char *fullString, *token;

                				fullString = strdup(ip);
                				token = strtok_r(fullString, ",", &savePtrComma);

                				while (token) {
                					if (strcmp(token, "connects") == 0)
                						snapStatTypes |= SNAP_CONNECT;
                					else if (strcmp(token, "connlatency") == 0)
                						snapStatTypes |= SNAP_CONNECT_LATENCY;
                					else if (strcmp(token, "msgrates") == 0)
                						snapStatTypes |= SNAP_RATE;
                					else if (strcmp(token, "msgcounts") == 0)
                						snapStatTypes |= SNAP_COUNT;
                					else if (strcmp(token, "msglatency") == 0)
                						snapStatTypes |= SNAP_LATENCY;
                					else {
                		                errMsg[0] = '\0';
                		                snprintf(errMsg, MIN_ERROR_STRING, "Invalid snap option type: %s", token);
                		                syntaxhelp(PARM_SNAP, errMsg);
                						rc = RC_INVALID_OPTION;
                						i = 2;
                						break;
                					}

                					token = strtok_r(NULL, ",", &savePtrComma);
                				} /* while (token) */

                				free(fullString);
                				argIndex++;
                			}

                			if (((i == 1) || (snapStatTypes == 0)) && (rc == 0)) {
           						/* Check that the snap interval is a number */
           						if (valStrIsNum(ip) == 1) {
           							/* Snap interval in seconds. */
           							snapInterval = atoi(ip);
           						} else {
           							syntaxhelp(PARM_SNAP, "Snapshot interval is not valid (units in seconds).");
           							rc = RC_INVALID_VALUE;
                				}

           						if (snapStatTypes == 0)
           							i++;
                			}
                		} else
                			argIndex--;
                	}
        		} /* for ( i = 0 ; i < 2 ; i++ ) */

        		if (snapInterval == 0)
        			snapInterval = DEFAULT_SNAP_INTERVAL;
        	} else
        		break;

            continue;
        } /* -snap option */

        /* ----------------------------------------------------------------------------
         * option:  -sseq  or  --seq
         *
         * Indicates whether to send a message on every topic of a particular client
         * before sending on a subsequent client (i.e. depth first).  Default behavior
         * is to process a job for all clients before coming processing the next job
         * for a given client (i.e. breadth first).
         * ---------------------------------------------------------------------------- */
        if ((strcmp(argv[argIndex], "-sseq") == 0) || (strcmp(argv[argIndex], "--sseq") == 0)) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_SendSequential++, "--sseq", 0, argc, argIndex);
        	if (rc == 0)
            	roundRobinSendSub = 0;
        	else
        		break;

        	continue;
        } /* -sseq option */

        /* ----------------------------------------------------------------------------
         f* option:  -st
         *
         * Number of submitter threads to be used.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-st") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------- */
        	rc = checkParms(parm_NumSubmitThrds++, "-st", 1, argc, argIndex++);
        	if (rc == 0) {
        		/* Check if the next argument is a valid number. */
        		validArg = valStrIsNum(argv[argIndex]);
        		if (validArg == 1) {
                    /* Set local variable - Number of submit threads. */
                    tmpVar = atoi(argv[argIndex]);
                    /* Check that numSubmitThrds is greater than 0. */
                    if (tmpVar < 1)
                    	validArg = 0;
        		}

               	/* Check that the argument was valid. If not, provide error message. */
                if (validArg == 0) {
                    syntaxhelp(PARM_NUMSUBTHRDS, "Number of Submitter threads is not valid, must be a positive whole number).");
                    rc = RC_INVALID_VALUE;
                    break;
                } else
                	numSubmitThrds = tmpVar;

            	/* Test to ensure max number of submitter threads is not exceeded */
            	if(numSubmitThrds > MAX_SUBMITTER_THREADS) {
            		MBTRACE(MBERROR, 1, "Number of Submitter threads specified (%d) greater than max supported (%d).\n",
            				numSubmitThrds, MAX_SUBMITTER_THREADS);
            		syntaxhelp(PARM_NUMSUBTHRDS, "Number of Submitter threads is not valid, is greater than the max supported.\n");
            		rc = RC_INVALID_VALUE;
            		break;
            	}

            } else
                break;

            continue;
        } /* -st option */

        /* ----------------------------------------------------------------------------
         * option:  -t
         *
         * RateController flag which has the following values and definitions:
         *
         *  -2 = No Rate Control; Run as fast as possible with random distribution of
         *       clients selected for send.
         *  -1 = No Rate Control; Run as fast as possible.
         *   0 = Individual thread controls own rate (default).
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-t") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_RateControl++, "-t", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - RateController Type. */
        		tmpVar = atoi(argv[argIndex]);

                /* Check that the argument is a supported number (-2, -1, or 0) */
                if ((tmpVar > -3) && (tmpVar < 1))
                	rateControl = (int8_t)tmpVar;
                else {
                    syntaxhelp(PARM_RATECONTROL, "A valid value for the rateControl parameter (-t) was not specified.");
                    rc = RC_INVALID_VALUE;
                    break;
                }
            } else
                break;

            continue;
        } /* -t option */

        /* ----------------------------------------------------------------------------
         * option:  -tl
         *
         * Trace Level setting  (default:  5)
         *
         *   0 = No Tracing
         *   1 = Trace events  which normally occur only once.   Events which happen
         *       at init, start and stop.  This level can also be used when reporting
         *       errors.
         *   5 = Frequent actions which do not commonly occur at a per-message rate.
         *   7 = Trace events which commonly occur on a per-message basis and call/return
         *       from external functions which are not called on a per-messages basis.
         *   8 = Call/return from internal functions.
         *   9 = Detailed trace used to diagnose a problem.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-tl") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_TraceLevel++, "-tl", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - RateController Type. */
        		tmpVar = atoi(argv[argIndex]);

        		rc = setMBTraceLevel(tmpVar);
				if (rc) {
					syntaxhelp(PARM_TRACELEVEL, "A valid value for the tracelevel parameter (-tl) was not specified.");
					rc = RC_INVALID_VALUE;
					break;
				} else
					traceLevel = (uint8_t)tmpVar;
            } else
                break;

            continue;
        } /* -tl option */

		/* ----------------------------------------------------------------------------
		 * option:  -clientTrace <clientID regex>
		 *
		 * Match all clients with the provided regular expression and enable the flag
		 * traceEnabled inside the mqttclient_t structure.  This flag is checked at various
		 * points in the client lifecycle and will result in additional trace to assist
		 * in debugging problems.
		 * ---------------------------------------------------------------------------- */
		if (strcmp(argv[argIndex], "--clientTrace") == 0) {
			rc = checkParms(parm_ClientTrace++, "--clientTrace", 1, argc, argIndex++);
			if (rc == 0) {
				pCmdLineArgs->clientTraceRegexStr = strdup(argv[argIndex]);
				rc = ism_regex_compile(&(pCmdLineArgs->clientTraceRegex), pCmdLineArgs->clientTraceRegexStr);
				if(rc) {
					MBTRACE(MBERROR, 1, "An invalid regular expression string (%s) was provided with the --clientTrace command line parameter (regex error code: %d).\n", argv[argIndex], rc);
					syntaxhelp(PARM_CLIENTTRACE, "An invalid regular expression string was provided with the --clientTrace command line parameter");
					break;
				}
			} else
				break;

			continue;
        }
        /* ----------------------------------------------------------------------------
         * option:  -T
         *
         * LatencyMask which will identify the types of latency to measure.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-T") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_Latency++, "-T", 1, argc, argIndex++);
        	if (rc == 0) {
        		int tmpMask;
                /* Get next argument - Latency Type. */
        		tmpMask = strtoull(argv[argIndex], &eos, 0);
        		if ((tmpMask == 0) && (*eos)) {
        			syntaxhelp(PARM_LATENCY, "A valid value for the latencyMask parameter (-T) was not specified.\n\n" \
        					               "The value must be a hexadecimal number.");
        			rc = RC_INVALID_VALUE;
        			break;
        		} else {
        			latencyMask = tmpMask;
        		}
        	} else
        		break;

        	continue;
        } /* -T option */

        /* ----------------------------------------------------------------------------
         * option:  -u
         *
         * Histogram Units for array elements.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-u") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_HistogramUnits_Msg++, "-u", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - Latency Units. */
               	g_UnitsRTT = 1.0 / strtod(argv[argIndex], &eos);
               	if ((g_UnitsRTT == 0) && (*eos)) {
               		syntaxhelp(PARM_UNITS, "Units for latency measurements (-u) is not a valid floating point number.");
                   	rc = RC_INVALID_VALUE;
               		break;
               	}
           	} else
        		break;

        	continue;
        } /* -u option */

        /* ----------------------------------------------------------------------------
         * option:  -V
         *
         * chkMessageMask which will identify the types of checking to utilize.
         * ---------------------------------------------------------------------------- */
        if (strcmp(argv[argIndex], "-V") == 0) {
        	/* ------------------------------------------------------------------------
        	 * Perform a check on the parameter whether it was already specified
        	 * and if an additional argument is needed. Increment the argument
        	 * index to get the next arg value.
        	 * ------------------------------------------------------------------------ */
        	rc = checkParms(parm_Verify++, "-V", 1, argc, argIndex++);
        	if (rc == 0) {
                /* Get next argument - Validation Type. */
               	tmpVar = strtoull(argv[argIndex], &eos, 0);
                if ((tmpVar == 0) && (*eos)) {
                	syntaxhelp(PARM_VALIDATION, "A valid value for the checkMessageMask parameter (-V) was not specified.\n\n" \
                			   "The value must be a hexadecimal number.");
                	rc = RC_INVALID_VALUE;
                	break;
                } else
                	chkMsgMask = tmpVar;

                /* Provide info if there is any message checking being performed. */
                if (chkMsgMask) {
                    if((chkMsgMask & CHKMSGLENGTH) > 0) {
                    	MBTRACE(MBINFO, 1, "Checking message length on MQTT v5 (or later) consumers.\n");
                    }

                	if((chkMsgMask & CHKMSGSEQNUM) > 0) {
                        MBTRACE(MBINFO, 1, "Checking per topic message sequence order on MQTT v5 (or later) consumers.\n");
                    }
                }
            } else {
                break;
            }

        	continue;
        } /* -V option */

        else {  /* Unknown command */
        	/* ------------------------------------------------------------------------
             * Check to see if there are any more arguments and if so then this is an
             * this is an unknown command.
             * ------------------------------------------------------------------------ */
            if (strlen(argv[argIndex]) > 0) {
                errMsg[0] = '\0';
                snprintf(errMsg, MIN_ERROR_STRING, "Unknown command: %s", argv[argIndex]);
                syntaxhelp(PARM_HELP_END, errMsg);
                rc = RC_INVALID_PARAMETER;
                break;
            }

            continue;
        }
    } /* for ( argIndex = 1 ; argIndex < argc ; argIndex++ ) */

    /* --------------------------------------------------------------------------------
     * At this point all the parameters have been checked but now it is time to check
     * various settings and combinations of them.
     * -------------------------------------------------------------------------------- */
    do {
    	/* If there was a failure in checking the parameters then break out of do loop. */
    	if (rc)
    		break;

    	/* Test to see if there is a client list.  If not then return with an error. */
    	if (pCmdLineArgs->clientListPath == NULL) {
    		MBTRACE(MBERROR, 1, "No client list was specified.\n");
    		rc = RC_NO_CLIENTLIST_SPECIFIED;
    		break;
    	}

    	/* Test to see if requesting an output file when testDuration = 0 (doCommands) */
    	if ((testDuration == 0) && (csvLogFile) && (snapInterval == 0)) {
    		MBTRACE(MBINFO, 1, "All stats going to console, since the -snap parameter is not specified on the command line.\n");
    	}

       	/* ----------------------------------------------------------------------------
		 * If not testing for message sizes then need to set the min and max Message
		 * Size received here for latency data. This is just for display purposes
		 * ---------------------------------------------------------------------------- */
		if ((chkMsgMask & CHKMSGLENGTH) == 0) {
			g_MinMsgRcvd = g_MsgCfgInfo->minMsg;
			g_MaxMsgRcvd = g_MsgCfgInfo->maxMsg;
		}

       	/* ----------------------------------------------------------------------------
		 * Check if user specified quit and duration of 0.  If so, then ignore but
		 * provide warning in log file.  Quit option is only valid when duration > 0.
		 * ---------------------------------------------------------------------------- */
		if ((parm_Quit) && (testDuration == 0)) {
			MBTRACE(MBWARN, 1, "Ignoring -quit parameter, since it is only valid for a duration > 0.\n");
		}

    	/* ---------------------------------------------------------------------------------------------------------
    	 * Create the Message Directory Hash Map to be used for storing PUBLISH messages from the following sources:
    	 *
    	 *   - Generated messages based on size range, -s command line option
    	 *   - Global message directory, -M command line option
    	 *   - Client list file with at least one client specifying a message directory as the source of messages
    	 * -------------------------------------------------------------------------------------------------------- */
    	g_MsgCfgInfo->msgDirMap = ism_common_createHashMap((32*1024), HASH_STRING);
		if (g_MsgCfgInfo->msgDirMap == NULL) {
			rc = provideAllocErrorMsg("the Message Directory hash map", sizeof(g_MsgCfgInfo->msgDirMap), __FILE__, __LINE__);
			break;
		}

   		/* ----------------------------------------------------------------------------
   		 * Determine if the user requested Exit On First Disconnect and if so then
   		 * ensure reconnect is disabled.
   		 * ---------------------------------------------------------------------------- */
   		if (pCmdLineArgs->exitOnFirstDisconnect) {
   			if (reconnectEnabled) {
   				MBTRACE(MBWARN, 1, "Disabling reconnect due to -efd option.\n");
   				reconnectEnabled = 0;
   			}

   			g_DoDisconnect = 1;
   		} else if (!reconnectEnabled)
   			g_DoDisconnect = 1;

    	/* --------------------------------------------------------------------
         * Allocate the memory needed for mbInstArray, which is based on the
         * number of submitter threads.
         * -------------------------------------------------------------------- */
   		mbInstArray = (mqttbench_t **)calloc(numSubmitThrds, sizeof(mqttbench_t));
   		if (mbInstArray) {
   			g_MqttbenchInfo->mbInstArray = mbInstArray;

       		/* ------------------------------------------------------------------------
         	 * If there are multiple threads requested then need to create additional
         	 * mqttbench_t structures and update.
         	 * ------------------------------------------------------------------------ */
        	for ( i = 0 ; i < numSubmitThrds ; i++, tidx++ ) {
				/* Create a new mqttbench_t */
				pMQTTBench = createMQTTBench();
				if (pMQTTBench) {
					pMQTTBench->id = (int16_t)i;

					/* Add the current mqttbench_t to the mbInstArray. */
					mbInstArray[i] = pMQTTBench;
   				} else {
   					rc = RC_FAILED_TO_CREATE_MBINST;
  					break;
  				}
        	} /* for ( i = 0 ; i < numSubmitterThreads ; i++ ) */
		} else {
	    	rc = provideAllocErrorMsg("the Submitter thread array", (int)(numSubmitThrds * sizeof(mqttbench_t)),
									  __FILE__, __LINE__);
	    	break;
		}

        /* ------------------------------------------------------------------------
		 * Perform the following tasks, to process the -s or -M parameters
         *
         * 1. -s and -M option can NOT be specified together.  Provide error message
         *    and exit.
		 * 2. If -M option is provided process the message directory and store the
		 *    messagedir_object_t in the message directory map
		 * 3. If -s option is provided process create the artificial message directory
		 *    object with directory name "-S_PARM_SPECIFIED" and store it in the
		 *    message directory map
         * ------------------------------------------------------------------------- */

		/* Step 1 */
		if ((g_predefinedMsgDir) && (parm_MsgSize)) {
			MBTRACE(MBERROR, 1, "Can't specify both predefined messages (-M) and -s <min>-<max>, these parameters are mutually exclusive\n");
				rc = RC_PARAMETER_CONFLICT;
				break;
   			}

		/* Step 2 */
   		if (g_predefinedMsgDir) {
			/* ----------------------------------------------------------------------------------
			 * Warn users about using message files and measuring RTT latency on v3.1.1 consumers
			 * ---------------------------------------------------------------------------------- */
			if ((latencyMask & CHKTIMERTT) > 0) {
				MBTRACE(MBWARN, 1, "The -M command line parameter was specified in addition to the -T parameter with the RTT latency bit set in the mask. " \
								   "MQTT v3.1.1 (and earlier clients) do not support using message files with Round Trip Latency testing. " \
								   "Latency measurements will not be recorded for MQTT v3.1.1 (and earlier client). For MQTT v5 client (or later) " \
								   "an MQTT property will be used to pass the send timestamp and this property is reserved for mqttbench use ONLY.\n");
			}

			/* ------------------------------------------------------------------------------
			 * Warn users about using message files and msg length check on v3.1.1 consumers
			 * ------------------------------------------------------------------------------ */
			if ((chkMsgMask & CHKMSGLENGTH) > 0) {
				MBTRACE(MBWARN, 1, "The -M command line parameter was specified in addition to the -V parameter with the check message length bit set in the mask. " \
						           "MQTT v3.1.1 (and earlier clients) do not support using message files and checking message length. " \
								   "Message length will not be check for MQTT v3.1.1 (and earlier client). For MQTT v5 client (or later) " \
								   "an MQTT property will be used to pass the send message length and this property is reserved for mqttbench use ONLY.\n");
			}

			/* ----------------------------------------------------------------------------------------
			 * Warn users about using message files and msg sequence order checking on v3.1.1 consumers
			 * ---------------------------------------------------------------------------------------- */
			if ((chkMsgMask & CHKMSGSEQNUM) > 0) {
				MBTRACE(MBWARN, 1, "The -M command line parameter was specified in addition to the -V parameter with the check message sequence order bit set in the mask. " \
								   "MQTT v3.1.1 (and earlier clients) do not support using message files and checking message sequence order checking. " \
								   "Message sequence order checking will not be performed for MQTT v3.1.1 (and earlier client). For MQTT v5 client (or later) " \
								   "an MQTT property will be used to pass the send message sequence number and stream id and these properties are reserved for mqttbench use ONLY.\n");
			}

			rc = processMessageDir(g_predefinedMsgDir, g_MqttbenchInfo); // process the -M g_predefinedMsgDir and put it in the message directory map
			if (rc) {
				MBTRACE(MBERROR, 1, "Failed to process the message directory %s provided with the -M command line parameter\n", g_predefinedMsgDir);
				return rc;
			}
		}

   		/* Step 3 */
   		if(parm_MsgSize) {
   			rc = generateMessageDir(g_MqttbenchInfo, g_MsgCfgInfo); // allocate message dir object, populate with generated msgs, and insert into msgDirMap
   			if(rc) {
   				MBTRACE(MBERROR, 1, "Failed to generate the message directory from the -s command line parameter\n");
   				return rc;
   			}
   		}

		/* Check some combinations of beginStatsSecs. */
   		if (beginStatsSecs) {
   			/* Check the -rr option.   They are mutually exclusive. */
   			if (resetTStampXMsgs) {
   				syntaxhelp(PARM_BSTATS, "-b and -rr are mutually exclusive.");
   				rc = RC_PARAMETER_CONFLICT;
   				break;
   			}

        	/* ------------------------------------------------------------------------
   			 * Check to see if requesting begin statistics (-b option) and test
   			 * duration = 0.  This is not a valid combination.
   			 * ------------------------------------------------------------------------ */
   			if (testDuration == 0) {
   				MBTRACE(MBWARN, 1, "Conflicting command line parameter combination: -d 0 and -b %d. Overriding -b setting to 0.\n", beginStatsSecs);
   				beginStatsSecs = 0;
   			}
   		}

		/* ----------------------------------------------------------------------------
		 * Rate Control verifications
		 *
		 * -  RTT latency measurements are based on a sampling rate and message rate.
		 *    When using the value of -1 for rateControl (-t) no message rate is
		 *    specified.  Therefore RTT measurements cannot be taken.
		 *  - Rate control value = -1 and rate option (-r) are a conflict.
		 *  - Rate control value = -1 and burst option (--burst) are a conflict.
		 * ---------------------------------------------------------------------------- */
       	if (rateControl == -1) {
       		if ((latencyMask & CHKTIMERTT) > 0) {
       			MBTRACE(MBWARN, 1, "When using -t -1 and -T 0x1, the expected number of latency samples will " \
                                   "not be achieved. Should consider running with -t 1 and -r <rate>.\n");
       			fprintf(stdout, "(w) When using -t -1 and -T 0x1, the expected number of latency samples will " \
                                "not be achieved. Should consider running with -t 1 and -r <rate>.\n");
       			fflush(stdout);
       		}

       		/* -t -1 and -r <rate> are a conflict */
       		if (rateSet) {
       			syntaxhelp(PARM_LATENCY, "The -t -1 and -r <rate> options are mutually exclusive, pass only one of the options.");
       			rc = RC_PARAMETER_CONFLICT;
       			break;
       		}

       		/* -t -1 and --burst option are a conflict */
       		if (pBurstInfo) {
       			syntaxhelp(PARM_LATENCY, "The -t -1 and --burst options are mutually exclusive, pass only one of the options.");
       			rc = RC_PARAMETER_CONFLICT;
       			break;
       		}
       	}

    	/* ----------------------------------------------------------------------------
		 * If rate control was specified (-t -2) then check to see there is no conflict
		 * with rate set.  If there is no conflict then seed the random generator.
		 * ---------------------------------------------------------------------------- */
		if (rateControl == -2) {
			if (rateSet) {
    			syntaxhelp(PARM_RATECONTROL, "The -t -2 and -r <rate> are mutually exclusive, pass one or the other, but not both");
    			rc = RC_PARAMETER_CONFLICT;
    			break;
			} else
				srand48_r(time(NULL), &randBuffer);  /* initialization passing a time based seed */
		}

		/* File verifications and setting of default files. */
		/* ---------------------------------------------------------------------------- */
   		/* If the csv log file wasn't specified then set it to the default: mqttbench.csv */
       	if (csvLogFile == NULL) {
           	csvLogFile = (char *)calloc(1, (strlen(CSV_STATS_FNAME) + 1));
           	memcpy(csvLogFile, CSV_STATS_FNAME, strlen(CSV_STATS_FNAME));

           	/* Remove the previous file if it exists. */
           	removeFile(csvLogFile, REMOVE_FILE_EXISTS);

           	/* Attempt to open the data file for writing */
           	g_fpCSVLogFile = fopen(csvLogFile,"w");
           	if (g_fpCSVLogFile == NULL) {
           		MBTRACE(MBERROR, 1, "Unable to open the file (%s) for writing.  Check file permissions,  exiting...\n",
           				            csvLogFile);
               	rc = RC_FILEIO_ERR;
               	break;
           	}
       	}

   		/* If the csv latency file wasn't specified then set it to the default: mqttbench_latstats.csv */
       	if ((csvLatencyFile == NULL) && (latencyMask)) {
           	csvLatencyFile = (char *)calloc(1, (strlen(CSV_LATENCY_FNAME) + 1));
           	if (csvLatencyFile) {
           		memcpy(csvLatencyFile, CSV_LATENCY_FNAME, strlen(CSV_LATENCY_FNAME));

           		/* Remove the previous file if it exists. */
           		removeFile(csvLatencyFile, REMOVE_FILE_EXISTS);

           		/* Attempt to open the data file for writing */
           		g_fpCSVLatencyFile = fopen(csvLatencyFile,"w");
           		if (g_fpCSVLatencyFile == NULL) {
           			MBTRACE(MBERROR, 1, "Unable to open the file (%s) for writing.  Check file permissions,  exiting...",
           					            csvLatencyFile);
           			rc = RC_FILEIO_ERR;
           			break;
           		}
           	} else {
           		errMsg[0] ='\0';
           		snprintf(errMsg, MIN_ERROR_STRING, "the CSV Latency File (%s)", CSV_LATENCY_FNAME);
           		provideAllocErrorMsg(errMsg, (strlen(CSV_LATENCY_FNAME) + 1), __FILE__, __LINE__);
           		break;
           	}
       	}

   		/* ----------------------------------------------------------------------------
   		 * Perform checks for the situation where latency has been requested.
   		 *
   		 *   - Several combinations with other features are not supported.
   		 *   - Several latency combinations are not supported.
   		 * ---------------------------------------------------------------------------- */
   		if (latencyMask) {
   	       	/* Check to see if jitter enabled and if so exit. */
   	       	if (determineJitter == 1) {
   	       		MBTRACE(MBERROR, 1, "Latency testing is not supported with jitter.\n");
   	       		rc = RC_PARAMETER_CONFLICT;
   	       		break;
   	       	}

   	       	/* Check the latencyMask for valid combinations. */
   	       	if ((latencyMask & ~(CHKTIMERTT | CHKTIMESEND| CHKTIMETCPCONN | CHKTIMEMQTTCONN | CHKTIMESUBSCRIBE |
   	       		CHKTIMETCP2MQTTCONN | CHKTIMETCP2SUBSCRIBE | PRINTHISTOGRAM)) > 0) {
   	       		MBTRACE(MBWARN, 1, "Requested an undefined latency (0x%x).\n",
   	       			               (latencyMask & ~(CHKTIMERTT | CHKTIMESEND | CHKTIMETCPCONN | CHKTIMEMQTTCONN |
   	       			               CHKTIMESUBSCRIBE | CHKTIMETCP2MQTTCONN | CHKTIMETCP2SUBSCRIBE | PRINTHISTOGRAM)));

   	       		/* Turn off any unsupported bits in the latencyMask. */
   	       		latencyMask &= (CHKTIMERTT | CHKTIMESEND | CHKTIMETCPCONN | CHKTIMEMQTTCONN | CHKTIMESUBSCRIBE |
   	       			            CHKTIMETCP2MQTTCONN | CHKTIMETCP2SUBSCRIBE | PRINTHISTOGRAM);
   	       	}

   	       	/* ------------------------------------------------------------------------
   	       	 * Check to see if the following is being requested:
   	       	 *   - TCP - MQTT Connection
   	       	 *      --OR--
   	       	 *   - TCP - Subscription.
   	       	 *
   	       	 * If so, then make sure not trying to request TCP, MQTT or Subscription
   	       	 * separately.  If any of these conditions is true then turn off the
   	       	 * separate requests.
  	       	 * ------------------------------------------------------------------------ */
   	       	/* 1st Test for TCP - MQTT Connection vs TCP - Subscribe */
   	       	if (((latencyMask & CHKTIMETCP2MQTTCONN) > 0) &&
   	       		((latencyMask & CHKTIMETCP2SUBSCRIBE) > 0)) {
   	       		MBTRACE(MBWARN, 1, "Can not perform TCP/MQTT Connection and TCP/Subscribe latency simultaneously.\n" \
           		                   "Turning TCP/MQTT Connection latency off.\n");
   	       		latencyMask &= ~(CHKTIMETCP2MQTTCONN);
   	       	}

   	       	/* 2nd Test for TCP - MQTT Connection and single combinations. */
   	       	if (((latencyMask & CHKTIMETCP2MQTTCONN) > 0) &&
   	       		((latencyMask & (CHKTIMETCPCONN | CHKTIMEMQTTCONN)) > 0)) {
   	       		MBTRACE(MBWARN, 1, "Can not perform TCP/MQTT Connection simultaneously with either TCP connection\n"
           				           "or MQTT Connection latency.  Turning TCP and/or MQTT Connection latency off.\n");
   	       		latencyMask &= ~(CHKTIMETCPCONN | CHKTIMEMQTTCONN);
   	       	}

   	       	/* 3rd Test for TCP - MQTT Conenction and single combinations. */
   	       	if (((latencyMask & CHKTIMETCP2SUBSCRIBE) > 0) &&
   	       		((latencyMask & (CHKTIMETCPCONN | CHKTIMEMQTTCONN | CHKTIMESUBSCRIBE)) > 0)) {
   	       		MBTRACE(MBWARN, 1, "Can not perform TCP/Subscribe simultaneously with either TCP connection, MQTT Connection\n"
           				           "or Subscribe latency separately.  Turning off TCP, MQTT and/or Subscribe latency off.\n");
   	       		latencyMask &= ~(CHKTIMETCPCONN | CHKTIMEMQTTCONN | CHKTIMESUBSCRIBE);
   	       	}

   	       	/* Check to see if other latency requested with TCP, MQTT or Subscribe Connection. */
   	       	if (((latencyMask & (CHKTIMETCPCONN | CHKTIMEMQTTCONN | CHKTIMESUBSCRIBE)) > 0) &&
   	       		((latencyMask & CHKTIMESEND) > 0)) {
   	       		MBTRACE(MBWARN, 1, "Can not perform Send latency with TCP, MQTT or Subscribe latency.  Turning Send latency off.\n");
   	       		latencyMask &= ~(CHKTIMESEND);
   	       	}

   	       	/* Need to perform some tests to ensure snapshots and latency are compatible. */
   	        if ((snapInterval > 0) && (latencyMask > 0)) {
   	        	/* Make sure that snapshot requests match the latency mask combinations. */
   	        	if ((((latencyMask & CHKTIMERTT) == 0) && ((snapStatTypes & SNAP_LATENCY) > 0)) ||
					(((latencyMask & LAT_CONN_AND_SUBSCRIBE) == 0) && ((snapStatTypes & SNAP_CONNECT_LATENCY) > 0))) {
   	        		MBTRACE(MBERROR, 1, "the options from the -snap <opt> command line parameter are not consistent with the requested latency measurements (-T <bitmask>). \n");
   	        		rc = RC_INVALID_OPTION;
   	        		break;
				}
   	        }

   	        /* ------------------------------------------------------------------------
   	         * Check to see if just doing TCP Connection Latency.   If so then set the
   	         * global flag tcpConnLatency = 1.   Work item # 192995
   	         * ------------------------------------------------------------------------ */
   	        if ((latencyMask == CHKTIMETCPCONN) || (latencyMask == LAT_TCP_ONLY))
   	        	g_TCPConnLatOnly = 1;

   	        /* ------------------------------------------------------------------------
   	         * Create the Latency Units structure which will be used when setting up
   	         * the histograms.  This will be freed when startSubmitterThreads completes.
   	         * ------------------------------------------------------------------------ */
   	        pLatencyUnits = (latencyUnits_t *)calloc(1, sizeof(latencyUnits_t));
   	        if (pLatencyUnits) {
   	        	pLatencyUnits->ConnUnits = g_UnitsConn;
   	        	pLatencyUnits->RTTUnits = g_UnitsRTT;
   	        	pLatencyUnits->ReconnRTTUnits = g_UnitsReconnRTT;
   	        } else {
		    	provideAllocErrorMsg("Latency Units structure", (int)sizeof(mbCmdLineArgs_t), __FILE__, __LINE__);
           		break;
   	        }
   		} else {
   			if (resetLatStatsSecs > 0) {
   				MBTRACE(MBINFO, 1, "Reset latency (-rl) option specified but did not specify any latency.\n");
   				resetLatStatsSecs = 0;
   			}
   		} /* if (localLatencyMask) */

   		/* Provide information about Linger if it was specified. */
   		if (lingerTime_ns > 0) {
   			MBTRACE(MBINFO, 1, "Global connection linger time is set to %d seconds.\n", (int)(lingerTime_ns / NANO_PER_SECOND));
   		}

        /* Check the chkMsgMask for valid combinations. */
        if ((chkMsgMask & ~(CHKMSGLENGTH | CHKMSGSEQNUM)) > 0) {
        	MBTRACE(MBWARN, 1, "Requested an undefined message checking (0x%x).\n",
        			           (chkMsgMask & ~(CHKMSGLENGTH | CHKMSGSEQNUM)));

        	/* Turn off any unsupported bits in the chkMsgMask. */
        	chkMsgMask &= (CHKMSGLENGTH | CHKMSGSEQNUM);
        }

       	/* Set up the global variables for the 1sec and 5sec latency value based on UNITS. */
       	if (g_UnitsRTT < 0)
       		g_Equiv1Sec_RTT = (int)(1.0/g_UnitsRTT);
       	else
       		g_Equiv1Sec_RTT = (int)g_UnitsRTT;

       	if (g_UnitsConn < 0)
       		g_Equiv1Sec_Conn = (int)(1.0/g_UnitsConn);
       	else
       		g_Equiv1Sec_Conn = (int)g_UnitsConn;

       	if (g_UnitsReconnRTT < 0)
       		g_Equiv1Sec_Reconn_RTT = (int)(1.0/g_UnitsReconnRTT);
       	else
       		g_Equiv1Sec_Reconn_RTT = (int)g_UnitsReconnRTT;
       	/* ----------------------------------------------------------------------------
       	 * If waitReady option (-wr) was specified then need to set the value of
       	 * waitReady to the number of producer and consumer threads.
       	 * --------------------------------------------------------------------------- */
       	if (g_WaitReady)
			g_WaitReady = (int16_t) numSubmitThrds;

       	/* Set the global max message in flight based on default (64K) or -mim command line parameter */
       	if (maxInflightMsgs == 0 || maxInflightMsgs > MQTT_MAX_MSGID)
       		maxInflightMsgs = MQTT_MAX_MSGID;

    	pCmdLineArgs->maxInflightMsgs = maxInflightMsgs;

    	if (maxInflightMsgs > MAX_MSGID_SEARCH) {
			pCmdLineArgs->maxMsgIDSearch = MAX_MSGID_SEARCH;
			pCmdLineArgs->maxPubRecSearch = MAX_MSGID_SEARCH;
			//pCmdLineArgs->maxPubRecSearch = (MAX_MSGID_SEARCH >> 1);  // this could lead to a bad acker situation, need to test again
    	} else {
			pCmdLineArgs->maxMsgIDSearch = maxInflightMsgs;
			pCmdLineArgs->maxPubRecSearch = maxInflightMsgs;
			//pCmdLineArgs->maxPubRecSearch = (maxInflightMsgs >> 1); // this could lead to a bad acker situation, need to test again
    	}

       	/* ----------------------------------------------------------------------------
       	 * If the burst mode option (--burst) is specified then need to:
       	 *
       	 *   1. Update the base message rate in the pBurstInfo structure.
       	 *   2. If the interval is longer the duration of the test provide a warning.
       	 * ---------------------------------------------------------------------------- */
      	if (pBurstInfo) {
      		double testVal = pBurstInfo->burstDuration / NANO_PER_SECOND;

      		pBurstInfo->baseMsgRate = g_MsgRate;

      		if ((testDuration > 0) && (testDuration <= pBurstInfo->burstInterval)) {
      			MBTRACE(MBWARN, 1, "The burst mode interval is longer than the duration of the test.\n");
      		}

      		if ((testVal < 5) && (pBurstInfo->burstMsgRate > 40000)) {
      			MBTRACE(MBWARN, 1, "The burst duration is short (%llu) for the burst message rate (%f).\n",
      				               (ULL)(pBurstInfo->burstDuration / NANO_PER_SECOND),
						           pBurstInfo->burstMsgRate);
      		}
      	}
   	} while (0);

    if(rc) {
    	return rc;
    }

	/* --------------------------------------------------------------------------------
	 * Set the global variables based on the local value
	 * -------------------------------------------------------------------------------- */
	g_AppSimLoop = appSimLoop;
	g_ChkMsgMask = chkMsgMask;
	g_LatencyMask = latencyMask;

	/* --------------------------------------------------------------------------------
	 * Update the pCmdLineArgs with all the local command line variables so that
	 * the command line structure will be used throughout the rest of the application.
	 * -------------------------------------------------------------------------------- */
	pCmdLineArgs->appSimLoop = appSimLoop;
	pCmdLineArgs->beginStatsSecs = beginStatsSecs;
	pCmdLineArgs->chkMsgMask = chkMsgMask;
	pCmdLineArgs->determineJitter = determineJitter;
	pCmdLineArgs->latencyMask = latencyMask;
	pCmdLineArgs->lingerTime_ns = lingerTime_ns;
	pCmdLineArgs->numSubmitterThreads = numSubmitThrds;
	pCmdLineArgs->globalMsgPubCount = totalMsgPubCount;
	pCmdLineArgs->rateControl = rateControl;
	pCmdLineArgs->reconnectEnabled = reconnectEnabled;
	pCmdLineArgs->disconnectType = disconnectType;
	pCmdLineArgs->resetLatStatsSecs = resetLatStatsSecs;
	pCmdLineArgs->resetTStampXMsgs = resetTStampXMsgs;
	pCmdLineArgs->roundRobinSendSub = roundRobinSendSub;
	pCmdLineArgs->pingTimeoutSecs = pingTimeoutSecs;
	pCmdLineArgs->pingIntervalSecs = pingIntervalSecs;
	pCmdLineArgs->snapInterval = snapInterval;
	pCmdLineArgs->snapStatTypes = snapStatTypes;
	pCmdLineArgs->testDuration = testDuration;
	pCmdLineArgs->traceLevel = traceLevel;
	pCmdLineArgs->msgSizeRangeParmFlag = parm_MsgSize;
	pCmdLineArgs->msgDirParmFlag = parm_PredefinedMsg;

	/* --------------------------------------------------------------------------------
	 * Initialize the pthread spinlocks since the latency mask is known now.
	 * -------------------------------------------------------------------------------- */
	initMqttbenchLocks(latencyMask);

    return rc;
} /* parseArgs */

/* *************************************************************************************
 * readEnv
 *
 * Description:  Read the environment variables.
 *
 *   @return 0   = Successful completion
 * *************************************************************************************/
int readEnv (void)
{
    int rc = 0;
    int i;
	int delaySendCount = 0;              /* Number of Sends (TCP Conn, MQTT Conn, and/or Subscribe between delay */
	int delaySendTimeUsecs = 0;          /* Number of MicroSeconds to sleep for delay on sends. */
	int maxRetryAttempts = 0;            /* Max number of retry attempts before disconnecting */
	int memTrimSecs = 300;				 /* Enable malloc_trim() on a 5 min interval, by default */

	/* Get the # of CPU that mqttbench is assigned to */
	FILE *fp1 = popen("/usr/bin/nproc", "r");
	if(fp1){
		char nprocOutStr[64] = {0};
		char *out = fgets(nprocOutStr, sizeof(nprocOutStr) - 1, fp1);
		if(out) {
			pSysEnvSet->assignedCPU = atoi(nprocOutStr);
		}
		pclose(fp1);
	}

	/* Check if hyperthreading is enabled */
	FILE *fp2 = popen("cat /sys/devices/system/cpu/cpu0/topology/thread_siblings_list", "r");
	if(fp2) {
		char siblingStr[64] = {0};
		char *out = fgets(siblingStr, sizeof(siblingStr) - 1, fp2);
		if(out) {
			int count = 1;
			while(*out) if (*out++ == ',') ++count;
			pSysEnvSet->numSMT = count;
		}
		pclose(fp2);
	}

	int iopCount = pSysEnvSet->assignedCPU / pSysEnvSet->numSMT; /* no more than 1 IOP per core, not CPU */
	/* Default value for number of I/O Processor Threads */
	int numIOProcThrds = MAX(2, iopCount - (g_MqttbenchInfo->mbCmdLineArgs->numSubmitterThreads + 1));  /* + 1 to account for busy timer thread */
	int numIOListenerThrds = 1;             /* Default value for number of I/O Listener Threads */
	int ipIndex = 0;
	int numBfrsPerRequest = 0;
	int numKeepAlive = 0;
	int numInterfaceIPs = 0;
    int numEnvSourceIPs = 0;            /* Number of Source IP addresses set in the SIPList Env Variable. */
	int sampleRate = 1000;               /* Default Value for sample rate when performing latency testing. */
	int srcPortNumLow = 0;
	int tmpVal;

    uint8_t  clkSrc = 0;                 /* Clock source to be used: 0=TSC, 1=GTOD */

    uint64_t totalSystemMemoryMB = 0;

    double  connRetryFactor = 0.0;       /* The connection retry multiplier to provide a delay between retries. */

	char  *cAddRetry;
	char  *cAutoLog;
	char  *cBatchingDelay;
	char  *cBfrsPerRequest;
	char  *cClkSrc;
	char  *cConnHistSize;
	char  *cDelayCount;
	char  *cDelayTime;
	char  *cDisableLatencyWarn;
	char  *cGraphiteIP;
	char  *cGraphitePort;
	char  *cGraphiteMetricRoot;
	char  *cIOListenerThreads;
	char  *cIOProcThreads;
    char  *cKeepAlive;
    char  *cLatencySampleRate;
    char  *cMaxNumTXBfrs;
	char  *cMemTrimInterval;
	char  *cMqttbenchTLSBfrPool;
	char  *cMsgHistSize;
    char  *cPipeCommands;
    char  *cRecvBfrSize;
    char  *cRecvSockBfr;
    char  *cRetryBackOffFactor;
    char  *cRetryDelay;
    char  *cRetryMaxAttempts;
    char  *cSendBfrSize;
    char  *cSendSockBfr;
	char  *cSIPList;
    char  *cSrcPortLow;
	char  *cSSLCipher;
	char  *cSSLClientMethod;
    char  *cStreamIDMapSize;
	char  *cTLSBufferPoolSize;
	char  *cUseEphemeralPorts;
    char  *cUseNagle;
	char  *cALPNList;

	char  *savePtr1 = NULL;
	char  *tok = NULL;
	char  *tokComma = NULL;
	char **interfaceIPs = NULL;         /* List of Interface Addresses, which will validate the Source IP Addresses. */

	MBTRACE(MBINFO, 1, "Processing environment variables\n");

    /* Get the system environment variables needed. */
    cAddRetry				= getenv("AddRetry");           /* INTERNAL USE ONLY - Way to modify termination more gracefully */
    cBatchingDelay 			= getenv("BatchingDelay");      /* > 0 = add a delay in IOProc threads to allow batching ; if <= 0 perform 3 sched_yields() */
    cBfrsPerRequest         = getenv("BuffersPerReq");      /* Number of buffers to get with each request when the IOP creates the TX Bfr Pool. */
    cClkSrc                 = getenv("ClockSrc");           /* Clock source to use. 0=TSC, 1=TOD */
    cConnHistSize			= getenv("ConnHistSize");       /* Set the size of the connection histogram per IOP */
    cDelayCount             = getenv("DelayCount");			/* used for rate control of TCP connect, MQTT connect, MQTT subscribe ; number of delays */
    cDelayTime              = getenv("DelayTime");			/* used for rate control of TCP connect, MQTT connect, MQTT subscribe ; duration of delays */
    cDisableLatencyWarn     = getenv("DisableLatencyWarn"); /* Disable the timer which determines if latency entries larger than histogram. */
    cGraphiteIP				= getenv("GraphiteIP"); 		/* IPv4 address of the Graphite server to send metrics to */
    cGraphitePort			= getenv("GraphitePort"); 		/* Port number that the Graphite server is listening on */
    cGraphiteMetricRoot		= getenv("GraphiteMetricRoot");	/* root path on the Graphite server to write mqttbench metrics to */
    cIOListenerThreads 		= getenv("IOListenerThreads");  /* Number of I/O Listener threads */
    cIOProcThreads 			= getenv("IOProcThreads");  	/* Number of I/O Processor threads */
	cAutoLog                = getenv("MBAutoLog");          /* Indication to rename log files every 24 hrs. */
    cMaxNumTXBfrs           = getenv("MaxIOPTXBuffers");    /* Maximum number of TX Buffers to allocate */
	cMemTrimInterval        = getenv("MemTrimInterval");    /* # of seconds between performing malloc_trim */
    cKeepAlive              = getenv("MQTTKeepAlive");      /* MQTT Connection Keep Alive Timer value (1 - 64K) */
    cMsgHistSize			= getenv("MsgHistSize");        /* Set the size of the message (RTT) histogram per IOP */
    cPipeCommands           = getenv("PipeCommands");       /* 0 = read commands from STDIN, 1 = read commands from a named pipe /tmp/mqttbench-np . (default = 0)*/
    cRecvBfrSize 			= getenv("RecvBufferSize");    	/* Size of buffer used to read data from network (pool). */
    cRecvSockBfr 			= getenv("RecvSockBuffer");		/* receive socket buffer */
    cRetryBackOffFactor     = getenv("RetryBackoff");       /* The multiplier factor to use against the Retry Delay when there is a failure. */
    cRetryDelay             = getenv("RetryDelay");         /* Initial retry delay if needed during TCP Connections */
    cRetryMaxAttempts       = getenv("RetryMaxAttempts");   /* Maximum # of retries to attempt before disconnecting. */
    cLatencySampleRate 		= getenv("SampleRate");  		/* Sample Rate for latency. */
    cSendBfrSize			= getenv("SendBufferSize");    	/* Size of buffer used to send data to network (single buffer per client transport). */
    cSendSockBfr 			= getenv("SendSockBuffer");     /* send socket buffer */
	cSIPList                = getenv("SIPList");            /* Provides a list of Destination IPs - IoTF reqmt */
    cSrcPortLow             = getenv("SourcePortLo");       /* The starting port for the source ports. Default = 5000 */
    cSSLCipher              = getenv("SSLCipher");			/* Possible values: RSA (default) */
    cSSLClientMethod        = getenv("SSLClientMeth");      /* Possible values: SSLv23, SSLv3, TLSv1, TLSv11, TLSv12 (default) */
    cStreamIDMapSize		= getenv("StreamIDMapSize");    /* Size of the per IOP thread hash map used for tracking out of order message delivery per stream */
    cTLSBufferPoolSize      = getenv("TLSBufferPoolSize");  /* # of buffers per pool size for SSL Buffers. */
	cUseEphemeralPorts      = getenv("UseEphemeralPorts");  /* Indication to let OS select the source port using the ephemeral source port selection algorithm (do NOT bind source port) */
    cUseNagle               = getenv("UseNagle");           /* Enable/Disable Nagle's Algorithm. Default = 0 (setSockOpt w/TCP_NODELAY*/
	cMqttbenchTLSBfrPool    = getenv("UseTLSBfrPool");      /* Manage the TLS Buffer Pool internally */
	cALPNList               = getenv("ALPNList");           /* space separated list of application level protocols (i.e. ALPN protos) sent by the client during the TLS handshake 
                                                               e.g. export ALPNList="mqtt/3.1.1 mqtt/5.0"
															   https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_alpn_protos.html */

	/* --------------------------------------------------------------------------------
     * Check to see if there were any source IPs (SIPList) specified and parse the
     * string and put in a global array.
     * -------------------------------------------------------------------------------- */
    if (cALPNList && (strcmp(cALPNList, "") > 0)) {
		pSysEnvSet->ALPNList = strdup(cALPNList); /* Copy the current cALPNList into the System Environment Structure. */
		unsigned char alpn[4096] = {0};
		unsigned char *start, *end;
		start = end = alpn;
		char *token;
		char *rest = cALPNList;
		
		while ((token = strtok_r(rest, " ", &rest)) != NULL) {
			unsigned char len = strlen(token);
			*end = len;
			end++;
			memcpy(end, token, len);
			end+=len;
		}
		g_alpnListLen = end - start;
		g_alpnList = malloc(g_alpnListLen);
		memcpy(g_alpnList, alpn, g_alpnListLen);
	}

    /* --------------------------------------------------------------------------------
     * Check to see if there were any source IPs (SIPList) specified and parse the
     * string and put in a global array.
     * -------------------------------------------------------------------------------- */
    if (cSIPList && (strcmp(cSIPList, "") > 0)) {
    	/* Copy the current SIPList into the System Environment Structure. */
    	pSysEnvSet->strSIPList = strdup(cSIPList);

   		/* Test to see if the user specified a comma in the SIPList */
   		tokComma = memchr(cSIPList, ',', strlen(cSIPList));
   		if (tokComma == NULL) {
   			/* Allocate the g_EnvSIPs and check that it is valid */
   			g_EnvSIPs = calloc(MAX_LOCAL_IP_ADDRESSES, sizeof(char*));
   			if (g_EnvSIPs) {
   				savePtr1 = NULL;
   				tok = strtok_r(cSIPList, " ", &savePtr1);

   				while (tok) {
   					struct in_addr addr;
   					if (ipIndex >= MAX_LOCAL_IP_ADDRESSES) {
   						MBTRACE(MBWARN, 1, "Too many IPv4 addresses are specified (max=%d) in the SIPList env variable. Using only the first %d IPv4 addresses.\n",
           			                        MAX_LOCAL_IP_ADDRESSES, MAX_LOCAL_IP_ADDRESSES);
   						break;
   					}

   					rc = inet_aton(tok, &addr);
					/* Ensure the current IP Addr is shorter than the max supported version. */
   					if (strlen(tok) < INET_ADDRSTRLEN || rc == 0) {
   						g_EnvSIPs[ipIndex] = calloc(1, INET_ADDRSTRLEN);
   						if (g_EnvSIPs[ipIndex]) {
   							strcpy(g_EnvSIPs[ipIndex++], tok);   /*BEAM suppression: accessing beyond memory */
   							tok = strtok_r(NULL, " ", &savePtr1);
   						} else {
   							rc = provideAllocErrorMsg("src IP entry", INET_ADDRSTRLEN, __FILE__, __LINE__);
   							return rc;
   						}
   					} else {
   						MBTRACE(MBERROR, 1, "Entry %s, in the SIPList env variable is not a valid IPv4 address\n", tok);
   						return RC_INVALID_VALUE;
   					}
   				}

   				if(ipIndex == 0) {
   					MBTRACE(MBERROR, 1, "No valid IPv4 addresses were specified in the SIPList env variable.  SIPList is a space separated list of IPv4 addresses.\n");
   					return RC_INVALID_VALUE;
   				}

   				numEnvSourceIPs = ipIndex;

   			} else {
   				rc = provideAllocErrorMsg("src IP array", (MAX_LOCAL_IP_ADDRESSES * sizeof(char*)), __FILE__, __LINE__);
   				return rc;
   			} /* if (g_EnvSIPs) */
   		} else {
   			MBTRACE(MBERROR, 1, "SIPList format is incorrect.  IPv4 addresses must be separated by spaces.\n");
			return RC_INVALID_VALUE;
   		} /* if (tokComma == NULL) */
	}

    ipIndex = 0;

	/* --------------------------------------------------------------------------------
	 * Get the current set of interfaces associated with this machine.
	 * -------------------------------------------------------------------------------- */
    interfaceIPs = getListOfInterfaceIPs(&numInterfaceIPs);
    if ((interfaceIPs == NULL) || (numInterfaceIPs == 0)) {
    	if (numInterfaceIPs == 0)
    		MBTRACE(MBERROR, 1, "There are no IPv4 interfaces on this machine.\n");

    	return RC_NO_IP_ADDRESSES;
    }

    /* If no source IPs were provided in the SIPList env variable then use the list of source IP addresses
     * configured on the local interfaces */
    if(numEnvSourceIPs == 0){
    	numEnvSourceIPs = numInterfaceIPs;
    	g_EnvSIPs = interfaceIPs;
    }

    /* --------------------------------------------------------------------------------
     * Check to see if the g_srcPorts was allocated.  If so then populate with the
     * srcPortNumLow value.  The value is either the Environment variable SourcePortLo
     * or default value of 5000.
     * -------------------------------------------------------------------------------- */
	if (numEnvSourceIPs > 0) {
		g_srcPorts = calloc(numEnvSourceIPs, sizeof(int));
		if (g_srcPorts) {
			/* ------------------------------------------------------------------------
			 * Check to see if the environment variable was set for the starting source
			 * port number.
			 * ------------------------------------------------------------------------ */
			if (cSrcPortLow)
				srcPortNumLow = atoi(cSrcPortLow);
			else
				srcPortNumLow = LOCAL_PORT_RANGE_LO;

			pSysEnvSet->sourcePortLo = srcPortNumLow;

			for ( i = 0 ; i < numEnvSourceIPs ; i++ ) {
				g_srcPorts[i] = srcPortNumLow;
			}
		} else {
			rc = provideAllocErrorMsg("the Server Destinations", (int)(numEnvSourceIPs * sizeof(int)), __FILE__, __LINE__);
			return rc;
		}
	}

	/* Check to see if the user requested to use the OS ephemeral port selection algorithm for source port selection */
	if ((cUseEphemeralPorts) && (strcmp(cUseEphemeralPorts, "") > 0)) {
		tmpVal = atoi(cUseEphemeralPorts);
		if ((tmpVal < 0) || (tmpVal > 1)) {
			MBTRACE(MBERROR, 1, "Invalid value for the flag to indicate usage of the OS ephemeral port selection algorithm, "\
								"valid values are 0 or 1 (val=%d).\n", tmpVal);
			return RC_INVALID_ENV_SETTING;
		} else
			pSysEnvSet->useEphemeralPorts = (uint8_t) tmpVal;
	}

    /* Set the flag that indicates whether there is a port range overlap
     * If useEpheremalPorts is set then we don't care about source port low or port range overlap */
	if(!pSysEnvSet->useEphemeralPorts)
		pSysEnvSet->portRangeOverlap = (uint8_t) determinePortRangeOverlap(pSysEnvSet->sourcePortLo);

    /* Check that the Source IPs provided do exist on the machine or not.  */
   	rc = validateSrcIPs(interfaceIPs, numInterfaceIPs, g_EnvSIPs, numEnvSourceIPs);

    /* Set the sample rate for latency. */
    if ((cLatencySampleRate) && (strcmp(cLatencySampleRate, "") > 0))
		sampleRate = atoi(cLatencySampleRate);

    /* Check to see if the # of I/O processor threads was set or not. */
    if ((cIOProcThreads) && (strcmp(cIOProcThreads, "") > 0)) {
		numIOProcThrds = atoi(cIOProcThreads);

		if (numIOProcThrds > MAX_IO_PROC_THREADS) {
			MBTRACE(MBWARN, 1, "# of IOP Threads exceeds max (%d).  Setting to max.\n", MAX_IO_PROC_THREADS);
			fprintf(stdout, "(w) # of IOP Threads exceeds max (%d).  Setting to max.\n", MAX_IO_PROC_THREADS);
			fflush(stdout);
			numIOProcThrds = MAX_IO_PROC_THREADS;
		} else if (numIOProcThrds < 1) {
        	MBTRACE(MBERROR, 1, "Must have at least 1 I/O Processor Thread.\n\n)");
        	return RC_INVALID_VALUE;
		}
    }

    /* Check to see if the # of I/O listener threads is set or not. */
    if ((cIOListenerThreads) && (strcmp(cIOListenerThreads, "") > 0)) {
    	numIOListenerThrds = atoi(cIOListenerThreads);
    	if (numIOListenerThrds > MAX_IO_LISTENER_THREADS) {
			MBTRACE(MBWARN, 1, "# of IO Listener Threads exceeds max (%d).  Setting to max.\n", MAX_IO_LISTENER_THREADS);
			fprintf(stdout, "(w) # of IO Listener Threads exceeds max (%d).  Setting to max.\n", MAX_IO_LISTENER_THREADS);
			fflush(stdout);
			numIOListenerThrds = MAX_IO_LISTENER_THREADS;
    	} else if (numIOListenerThrds < 1) {
        	MBTRACE(MBERROR, 1, "Must have at least 1 I/O Listener Thread.\n\n)");
        	return RC_INVALID_VALUE;
    	}
    }

    /* Allocate array of IOP and IOL objects */
	ioProcThreads     = calloc((int) numIOProcThrds,     sizeof(ioProcThread_t*));
	ioListenerThreads = calloc((int) numIOListenerThrds, sizeof(ioListenerThread_t*));
	if (ioProcThreads == NULL || ioListenerThreads == NULL) {
		rc = provideAllocErrorMsg("IOP and IOL object arrays", (int)sizeof(mbThreadInfo_t), __FILE__, __LINE__);
	}

    /* --------------------------------------------------------------------------------
     * Check to see if the # of Buffers Per Request from IOP TX Buffer Pool is set or
     * not.  If not then use the default defined in mbconstants.h
     * -------------------------------------------------------------------------------- */
    if ((cBfrsPerRequest) && (strcmp(cBfrsPerRequest, "") > 0)) {
    	numBfrsPerRequest = atoi(cBfrsPerRequest);
    } else
    	numBfrsPerRequest = DEF_NUM_TX_BFRS;

    /* Check to see if the MQTTKeepAlive Timer value was specified. */
    if ((cKeepAlive) && (strcmp(cKeepAlive, "") > 0)) {
    	numKeepAlive = atoi(cKeepAlive);

    	if (numKeepAlive > 0xFFFF) {
      		MBTRACE(MBINFO, 1, "Setting MQTT Connection Keep Alive Timer: 0xFFFF\n");
        	numKeepAlive = 0xFFFF;
    	} else {
    		MBTRACE(MBINFO, 1, "Setting MQTT Connection Keep Alive Timer: 0x%x\n", numKeepAlive);
    	}
    } else
    	numKeepAlive = MQTT_KEEPALIVE_TIMER;

    /* Check to see if the maximum # of TX buffers was set. */
    if ((cMaxNumTXBfrs) && (strcmp(cMaxNumTXBfrs, "") > 0)) {
    	tmpVal = atoi(cMaxNumTXBfrs);
   		pSysEnvSet->maxNumTXBfrs = (int32_t)tmpVal;

    	if (pSysEnvSet->maxNumTXBfrs < numBfrsPerRequest) {
    		MBTRACE(MBINFO, 1, "Set number of buffers per requests to: %d\n", pSysEnvSet->maxNumTXBfrs);
    		numBfrsPerRequest = pSysEnvSet->maxNumTXBfrs;
    	}
    }

    /* Check to see if Auto Renaming files for every 24 hrs is set. */
    if ((cAutoLog) && (strcmp(cAutoLog, "") > 0)) {
    	tmpVal = atoi(cAutoLog);
    	if ((tmpVal == 0) || (tmpVal == 1)) {
    		pSysEnvSet->autoRenameFiles = (uint8_t)tmpVal;
    	} else {
			MBTRACE(MBERROR, 1, "Invalid setting for MBAutoLog: %d\n", pSysEnvSet->autoRenameFiles);
			return RC_INVALID_ENV_SETTING;
		}
    }

    /* Check to see if the Memory Trim Interval is set. */
    if ((cMemTrimInterval) && (strcmp(cMemTrimInterval, "") > 0)) {
    	memTrimSecs = atoi(cMemTrimInterval);
    	if (memTrimSecs > 0) {
		 	pSysEnvSet->memTrimSecs = memTrimSecs;
    	} else {
    		MBTRACE(MBERROR, 1, "Invalid setting for MemTrimInterval: %s\n", cMemTrimInterval);
    		return RC_INVALID_ENV_SETTING;
    	}
    } else {
    	pSysEnvSet->memTrimSecs = memTrimSecs;
    }

    /* Check to see if the Latency Warning timer is disabled or not. */
    if ((cDisableLatencyWarn) && (strcmp(cDisableLatencyWarn, "") > 0)) {
    	tmpVal = atoi(cDisableLatencyWarn);
    	if ((tmpVal == 0 || tmpVal == 1))
    		pSysEnvSet->disableLatencyWarn = (uint8_t)tmpVal;
    	else {
    		MBTRACE(MBERROR, 1, "Invalid setting for whether to Disable Latency Warning: %s\n", cDisableLatencyWarn);
    		return RC_INVALID_ENV_SETTING;
    	}
    }

    if ((cAddRetry) && (strcmp(cAddRetry,"") > 0))
    	pSysEnvSet->addRetry = atoi(cAddRetry);

    if ((cRetryMaxAttempts) && (strcmp(cRetryMaxAttempts, "") > 0))
    	maxRetryAttempts = atoi(cRetryMaxAttempts);
    else
    	maxRetryAttempts = MAX_CONN_RETRIES;

    pSysEnvSet->buffersPerReq = numBfrsPerRequest;
    pSysEnvSet->mqttKeepAlive = numKeepAlive;
    pSysEnvSet->numIOProcThreads = (uint8_t)numIOProcThrds;
    pSysEnvSet->numIOListenerThreads = (uint8_t)numIOListenerThrds;

    /* Set the environment settings structure */
    pSysEnvSet->batchingDelay = ((cBatchingDelay == NULL || (strcmp(cBatchingDelay, "") == 0)) ? 1: atoi(cBatchingDelay));
    pSysEnvSet->pipeCommands = ((cPipeCommands == NULL || (strcmp(cPipeCommands, "") == 0)) ? 0: atoi(cPipeCommands));

    /* The values of the environment variables used to configure buffer sizes can include
     * 'K' or 'M' to denote KB or MB the sendBufferSize must be larger than maxMsg */
    pSysEnvSet->recvBufferSize = getBufferSize(cRecvBfrSize,"16K");  /* 16KB is a good size buffer for secure connections */
    pSysEnvSet->sendBufferSize = getBufferSize(cSendBfrSize,"16K");  /* 16KB is a good size buffer for secure connections */
    pSysEnvSet->recvSockBuffer = getBufferSize(cRecvSockBfr,"32K");  /* receive socket buffer size */
    pSysEnvSet->sendSockBuffer = getBufferSize(cSendSockBfr,"16K");  /* send socket buffer size */
    pSysEnvSet->disableMessageTimeStamp = 1;
    pSysEnvSet->disableMessageID = 1;

	/* --------------------------------------------------------------------------------
	 * See if there are settings for SSLCipher and SSLClientMethod.  If NOT then set
	 * to the default values:
	 *
	 *    sslCipher       = RSA
	 *    sslClientMethod = TLSv12
	 * -------------------------------------------------------------------------------- */
	if ((cSSLClientMethod) && (strcmp(cSSLClientMethod, "") > 0))
		pSysEnvSet->sslClientMethod = strdup(cSSLClientMethod);
	else
		pSysEnvSet->sslClientMethod = "TLSv12";

	if ((cSSLCipher) && strcmp(cSSLCipher, "") > 0) {
		pSysEnvSet->sslCipher = strdup(cSSLCipher);
	} else {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		pSysEnvSet->sslCipher = "RSA"; // default Cipher list for TLS 1.2 or less
#else
		pSysEnvSet->sslCipher = "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256"; //default cipher suites for TLS 1.3 or later
#endif
	}
	pSysEnvSet->negotiated_sslCipher = strdup("");

    /* --------------------------------------------------------------------------------
     * Set the System Environment flag for managing the TLS buffer pool internally.
     * If set = 1 then mqttbench will manage the TLS Buffer Pool
     * If set = 0 then Server Utils will manage the TLS Buffer Pool with OpenSSL.
     * -------------------------------------------------------------------------------- */
    if ((cMqttbenchTLSBfrPool) && (strcmp(cMqttbenchTLSBfrPool, "") > 0)) {
    	tmpVal = atoi(cMqttbenchTLSBfrPool);
    	if ((tmpVal == 0) || (tmpVal == 1))
    		pSysEnvSet->mqttbenchTLSBfrPool = (uint8_t)tmpVal;
    	else {
    		MBTRACE(MBERROR, 1, "Unsupported value for TLS Buffer Pool (%d).\n", tmpVal);
    		return RC_INVALID_ENV_SETTING;
    	}
    } else
    	pSysEnvSet->mqttbenchTLSBfrPool = INIT_SSL_BUFFERPOOL;

    /* --------------------------------------------------------------------------------
     * Round up the txBfrSize to the next 128 byte boundary.  128 is equivalent to
     * shifting 7 to the right ( >>7).
     * -------------------------------------------------------------------------------- */
    if ((g_TxBfrSize % 128) > 0)
    	g_TxBfrSize = ((g_TxBfrSize >> 7) + 1) * 128;

    /* Check to see what clock is to be used. */
    if ((cClkSrc) && (strcmp(cClkSrc, "") > 0)) {
    	tmpVal = atoi(cClkSrc);
    	if (tmpVal > GTOD) {
    		MBTRACE(MBERROR, 1, "Requested invalid clock source (%d)\n.", tmpVal);
    		return RC_INVALID_ENV_SETTING;
    	} else {
    		clkSrc = (uint8_t)tmpVal;
    		g_ClkSrc = clkSrc;
    	}
    }

    /* Check to see if a Graphite server IP was provided */
	if ((cGraphiteIP) && (strcmp(cGraphiteIP, "") > 0)) {
		pSysEnvSet->GraphiteIP = strdup(cGraphiteIP);
	}
    /* Check to see if a Graphite server port was provided */
	if ((cGraphitePort) && (strcmp(cGraphitePort, "") > 0)) {
		tmpVal = atoi(cGraphitePort);
		if(tmpVal < 1 || tmpVal > 65535){
			MBTRACE(MBERROR, 1, "An invalid port number (%d) for GraphitePort environment variable was provided\n", tmpVal);
			return RC_INVALID_ENV_SETTING;
		} else {
			pSysEnvSet->GraphitePort = tmpVal;
		}
	} else {
		pSysEnvSet->GraphitePort = DEFAULT_GRAPHITE_PORT;
	}
	/* Check to see if a Graphite metric path root was provided */
	if ((cGraphiteMetricRoot) && (strcmp(cGraphiteMetricRoot, "") > 0)) {
		pSysEnvSet->GraphiteMetricRoot = strdup(cGraphiteMetricRoot);
	} else {
		pSysEnvSet->GraphiteMetricRoot = DEFAULT_GRAPHITE_METRIC_ROOT;
	}

    /* --------------------------------------------------------------------------------
     * If DelayCount and/or DelayTime are set then update the Global Variable settings
     * accordingly so that the hot loop can utilize them.
     * -------------------------------------------------------------------------------- */
    if (cDelayTime) {
    	delaySendTimeUsecs = atoi(cDelayTime);
    	if (delaySendTimeUsecs < 0) {
    		MBTRACE(MBERROR, 1, "Delay Send time is less than 0 (%d).\n", delaySendTimeUsecs);
    		return RC_INVALID_ENV_SETTING;
    	}
    } else
    	delaySendTimeUsecs = 1;

    if ((delaySendTimeUsecs) && (cDelayCount)) {
    	delaySendCount = atoi(cDelayCount);
		MBTRACE(MBINFO, 1, "Using Delay Send count: %d  time: %d usecs.\n", delaySendCount, delaySendTimeUsecs);
    }

    /* Check to see if the size of the Latency RTT Histogram is set or not. */
    if ((cMsgHistSize) && (strcmp(cMsgHistSize, "") > 0)) {
    	pSysEnvSet->maxMsgHistogramSize = atoi(cMsgHistSize);
    	if (pSysEnvSet->maxMsgHistogramSize < 0) {
    		MBTRACE(MBERROR, 1, "Size of Message Latency Histogram is less than 0 (%d).\n", pSysEnvSet->maxMsgHistogramSize);
    		return RC_INVALID_ENV_SETTING;
    	} else
    		pSysEnvSet->maxReconnHistogramSize = pSysEnvSet->maxMsgHistogramSize;
    } else {
      	pSysEnvSet->maxMsgHistogramSize = MAX_MSG_HISTOGRAM_SIZE;
      	pSysEnvSet->maxReconnHistogramSize = MAX_MSG_HISTOGRAM_SIZE;
    }

    /* Check to see if the size of the Latency RTT Histogram is set or not. */
    if ((cConnHistSize) && (strcmp(cConnHistSize, "") > 0)) {
    	pSysEnvSet->maxConnHistogramSize = atoi(cConnHistSize);
    	if (pSysEnvSet->maxConnHistogramSize < 0) {
    		MBTRACE(MBERROR, 1, "Size of Connection Latency Histogram is less than 0 (%d).\n", pSysEnvSet->maxConnHistogramSize);
    		return RC_INVALID_ENV_SETTING;
    	}
    } else
      	pSysEnvSet->maxConnHistogramSize = MAX_CONN_HISTOGRAM_SIZE;

    if (pSysEnvSet->maxMsgHistogramSize >= pSysEnvSet->maxConnHistogramSize)
    	pSysEnvSet->maxHgramSize = pSysEnvSet->maxMsgHistogramSize;
    else
    	pSysEnvSet->maxHgramSize = pSysEnvSet->maxConnHistogramSize;

    /* Check to see if the retry delay and factor have been set... */
    if (((cRetryDelay) && (strcmp(cRetryDelay, "") > 0)) &&
    	((cRetryBackOffFactor) && (strcmp(cRetryBackOffFactor, "") > 0))) {
       	/* The timer uses nanoseconds and since the RetryDelay is in microseconds,
       	 * then need to multiply by NANO_PER_MICRO to have it in nanoseconds.  */
       	pSysEnvSet->initConnRetryDelayTime_ns = atoi(cRetryDelay) * NANO_PER_MICRO;
    	connRetryFactor = atof(cRetryBackOffFactor);

    	if (connRetryFactor == 0)
    		MBTRACE(MBINFO, 1, "Env Variable RetryBackoff is set to 0.\n");
    	if (pSysEnvSet->initConnRetryDelayTime_ns == 0)
    		MBTRACE(MBINFO, 1, "Env Variable RetryDelay is set to 0.  Setting to default: %llu\n",
    			               (ULL)(DEFAULT_RETRY_DELAY_USECS * NANO_PER_MICRO));
    } else {
    	/* If the value of the retry delay is not set then going to set a default. */
    	pSysEnvSet->initConnRetryDelayTime_ns = (DEFAULT_RETRY_DELAY_USECS * NANO_PER_MICRO);
    }

    fflush(stdout);

    /* Check to see if going to enable or disable Nagle's Algorithm. */
    if ((cUseNagle) && (strcmp(cUseNagle, "") > 0)) {
    	tmpVal = atoi(cUseNagle);
    	if ((tmpVal < 0) || (tmpVal > 1)) {
    		MBTRACE(MBERROR, 1, "Unable to enable/disable Nagle's Algorithm (%d).\n", tmpVal);
    		return RC_INVALID_ENV_SETTING;
    	} else
    		pSysEnvSet->useNagle = (uint8_t)tmpVal;
    }

    /* See if TLS Buffer Pool Memory is specified. */

	/* Allocate the environment SSL Buffer structure */
	g_MqttbenchInfo->mbSSLBfrEnv = (mbSSLBufferInfo_t *)calloc(1, sizeof(mbSSLBufferInfo_t));
	if (g_MqttbenchInfo->mbSSLBfrEnv == NULL) {
		rc = provideAllocErrorMsg("the SSL Buffer Pool settings", (int)sizeof(mbSSLBufferInfo_t), __FILE__, __LINE__);
		return rc;
	}
	if ((cTLSBufferPoolSize) && (strcmp(cTLSBufferPoolSize, "") > 0))
		g_MqttbenchInfo->mbSSLBfrEnv->sslBufferPoolMemory = strtoul(cTLSBufferPoolSize, NULL, 10);

    /* --------------------------------------------------------------------------------
     * Obtain the amount of system memory in MB.  If using SSL then either the user
     * must set the environment variable:  TLSBufferPoolSize or use the system amount
     * to setup the TLS Buffer Pool.   If the user didn't set the Env Var and the
     * return code from the getSystemMemory() is -1 then exit.
   	 * -------------------------------------------------------------------------------- */
   	rc = getSystemMemory(&totalSystemMemoryMB, MEM_TOTAL, MEM_IN_MB);
   	if (rc == 0)
   		pSysEnvSet->totalSystemMemoryMB = totalSystemMemoryMB;
   	else {
		MBTRACE(MBWARN, 1, "Unable to determine the amount of memory on the system.\n");
		fprintf(stdout, "(w) Unable to determine the amount of memory on the system.\n");
		fflush(stdout);
   	}

   	/* Check to see if the StreamIDMapSize environment variable has been set */
   	if ((cStreamIDMapSize) && (strcmp(cStreamIDMapSize, "") > 0)) {
   		int size = atoi(cStreamIDMapSize);
		if (size <= 0) {
			MBTRACE(MBERROR, 1, "The environment variable StreamIDMapSize was set, but is not a non-negative integer (value=%s). Should " \
					            "be set to the desired size of the per IOP thread stream ID hash map.\n",
								cStreamIDMapSize);
			return RC_INVALID_ENV_SETTING;
		} else
			pSysEnvSet->streamIDMapSize = size;
   	} else {
   		pSysEnvSet->streamIDMapSize = 1024 * 1024; // set a default size for the stream ID map
   	}

    /* If the lingerSecs was passed in the command line, then set the value in the SysEnvSet. */
    if (pCmdLineArgs->lingerTime_ns > 0)
    	pSysEnvSet->lingerTime_ns = pCmdLineArgs->lingerTime_ns;

	/* --------------------------------------------------------------------------------
	 * Update global variables with settings from the System Environment Variable
	 * -------------------------------------------------------------------------------- */
    g_MaxRetryAttempts = maxRetryAttempts;
    g_ConnRetryFactor = connRetryFactor;

	/* --------------------------------------------------------------------------------
	 * Update the System Environment Variable structure (pSysEnvSet) with all the local
	 * environment variables captured here.
	 * -------------------------------------------------------------------------------- */
    pSysEnvSet->clkSrc = clkSrc;
	pSysEnvSet->connRetryFactor = connRetryFactor;
	pSysEnvSet->delaySendCount = delaySendCount;
	pSysEnvSet->delaySendTimeUsecs = delaySendTimeUsecs;
	pSysEnvSet->maxRetryAttempts = maxRetryAttempts;
	pSysEnvSet->numEnvSourceIPs = numEnvSourceIPs;
	pSysEnvSet->sampleRate = sampleRate;

	return rc;
} /* readEnv */

/* *************************************************************************************
 * checkForEnvConflicts
 *
 * Description: Start the timer threads and check to see if there is a requested CPU
 * affinity for each timer (Affinity_timer0 and Affinity_timer1) then set it.
 * *************************************************************************************/
int checkForEnvConflicts (mqttbenchInfo_t *g_mbInfo )
{
	int rc = 0;

	MBTRACE(MBINFO, 1, "Checking for conflicts between mqttbench input parameters.\n");

	do {
		/* See if the -rr option was used on a producer which is not applicable. */
		if (g_mbInfo->instanceClientType == PRODUCERS_ONLY) {
			if (pCmdLineArgs->resetTStampXMsgs) {
				MBTRACE(MBWARN, 1, "The -rr option is not applicable to producers.\n");
				fprintf(stdout, "(w) The -rr option is not applicable to producers.\n");
				fflush(stdout);

				pCmdLineArgs->resetTStampXMsgs = 0;
			}
		}

		if (pCmdLineArgs->latencyMask) {
			/* Check if requesting Send Latency, which means there needs to be producers. */
			if ((pCmdLineArgs->latencyMask & CHKTIMESEND) > 0) {
				if (g_mbInfo->instanceClientType == CONSUMERS_ONLY) {
					MBTRACE(MBWARN, 1, "Requesting Send latency with no producers.\n");
					fprintf(stdout, "(w) Requesting Send latency with no producers.\n");
					fflush(stdout);

					pCmdLineArgs->latencyMask &= ~CHKTIMESEND;
				}
			}

			/* ------------------------------------------------------------------------
			 * The only latency testing that can be performed on a producer is:
			 * TCP-IP, MQTT Connection and TCP-IP to MQTT Connection.  Subscription
      	 	 * testing is not supported on a producer.
      	 	 * ------------------------------------------------------------------------ */
			if (((pCmdLineArgs->latencyMask & (CHKTIMESUBSCRIBE | CHKTIMETCP2SUBSCRIBE)) > 0) &&
				(g_mbInfo->instanceClientType == PRODUCERS_ONLY)) {
				MBTRACE(MBERROR, 1, "Subscription Latency testing is not supported on producers,  exiting...\n");
				rc = RC_INVALID_REQUEST;
				break;
			}
		} /* if (pCmdLineArgs->latencyMask) */

		/* Create the arrays needed if determining jitter. */
		if (g_MqttbenchInfo->mbJitterInfo) {
			rc = provisionJitterArrays(CREATION,
					                   (int)g_mbInfo->instanceClientType,
									   pJitterInfo,
									   g_mbInfo->mbInstArray);
			if (rc) {
				MBTRACE(MBERROR, 1, "Unable to create Jitter arrays.\n\n");
				break;
			}
		}

		if (g_mbInfo->mbBurstInfo) {
			if (g_mbInfo->instanceClientType == CONSUMERS_ONLY) {
				MBTRACE(MBWARN, 1, "Burst mode is not supported on consumers.\n");
				fprintf(stdout, "(w) Burst mode is not supported on consumers.\n");
				fflush(stdout);
				free(g_mbInfo->mbBurstInfo);
				g_mbInfo->mbBurstInfo = NULL;
			}
		}
	} while(0);

	return rc;
} /* checkForEnvConflicts */

/* *************************************************************************************
 * initTimerThreads
 *
 * Description: Start the timer threads and check to see if there is a requested CPU
 * affinity for each timer (Affinity_timer0 and Affinity_timer1) then set it.
 * *************************************************************************************/
void initTimerThreads (void)
{
	int rc = 0;
	int i = 0;
	int affMapLen = 0;

	char   affMap[MAX_AFFINITY_MAP];
	char   envAffName[MAX_AFFINITY_NAME];
	char   envPropName[MAX_AFFINITY_NAME];
	char   threadName[MAX_THREAD_NAME];
	char * envAffStr;

	ism_field_t field;

	/* Set the Affinity for thread for the timer0 and 1. */
	for ( i = 0 ; i < MAX_TIMER_THREADS ; i++ ) {
		/* Set the following fields to zero. */
		memset(threadName, 0, MAX_THREAD_NAME);
		memset(affMap, 0, MAX_AFFINITY_MAP);
		memset(envPropName, 0, MAX_AFFINITY_NAME);

		snprintf(threadName, MAX_THREAD_NAME, "timer%d", i);
		snprintf(envAffName, MAX_AFFINITY_NAME, "Affinity_%s", threadName);
		snprintf(envPropName, MAX_AFFINITY_NAME, "Affinity.%s", threadName);

		envAffStr = getenv(envAffName);
		if (envAffStr) {
			affMapLen = ism_common_parseThreadAffinity(envAffStr, affMap);
			if (affMapLen) {
				field.type = VT_String;
				field.val.s = envAffStr;

				rc = ism_common_setProperty(ism_common_getConfigProperties(), envPropName, &field);
				if (rc) {
					MBTRACE(MBWARN, 1, "Unable to set the config property (CPU Affinity) for %s.\n", threadName);
					fprintf(stdout,"(w) Unable to set the config property (CPU Affinity) for %s.\n", threadName);
					fflush(stdout);
				}
			}
		}
	}

	/* Create the timer threads */
	ism_common_initTimers();

	/* Set the flag to indicate the timer threads have been created. */
	g_MqttbenchInfo->timerThreadsInit = 1;
} /* initTimerThreads( ) */
