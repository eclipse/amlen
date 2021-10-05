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
#include <signal.h>
#include <termios.h>

#include "mqttbench.h"
#include "tcpclient.h"
#include "mbconfig.h"

extern void ism_common_initTrace(void);
extern void ism_bufferPoolInit(void);

/***************************************** Global variables ***********************************/
                 /* Minimum message length = 21 bytes
                  *
                  * Includes 1 byte (flag), 8 bytes (timestamp), 8 bytes (seq num) and
                  * 4 bytes (payload length)
                  *
                  *
                  * mqttbench application framing
                  *
                  *  0                   1                   2                   3
                  *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
                  * +---------------+-----------------------------------------------+
                  * |     TSF (1)   |                 Timestamp (8)                 :
                  * +---------------+                                               :
                  * :                                                               :
                  * :               +-----------------------------------------------+
                  * :               |             App msg sqn number (8)            :
                  * +---------------+                                               :
                  * :                                                               :
                  * :               +-----------------------------------------------:
                  * :               |             Payload length (4)                :
                  * +---------------------------------------------------------------+
                  * :               |             Payload ('0xFF'...)               :
                  * +---------------------------------------------------------------+
                  *
                  * Header             - TSF + Timestamp + AppMsgSqn + Payload length
                  * TSF                - (1 byte) Flag to indicate that a timestamp is present
                  *                      in the message.  0 = no timestamp; 1 = timestamp
                  * Timestamp          - (8 byte) timestamp using TSC clock of producer thread
                  * App msg sqn number - (8 byte) message sequence number
                  * Payload length     - (4 byte) payload length
                  *
                  *-------------------------------------------------------------------
                  * Payload            - (-s <size> - Header) bytes of char 'F'
                  */

/* ******************************** GLOBAL VARIABLES ********************************** */
/* Externs */
extern uint8_t g_PrintUsage;
extern uint8_t g_RequestedQuit;
extern int16_t g_SubmitterExit;
extern int g_MBErrorRC;
extern int g_TxBfrSize;
extern char *csvLatencyFile;
extern double g_StartTimeConnTest;
extern double g_EndTimeConnTest;
extern mqttbenchInfo_t *g_MqttbenchInfo;
extern ioProcThread_t **ioProcThreads;
extern ioListenerThread_t **ioListenerThreads;
extern latencyUnits_t *pLatencyUnits;
extern FILE *g_fpCSVLogFile;
extern FILE *g_fpCSVLatencyFile;
extern FILE *g_fpHgramFiles[];

ismHashMap *qtMap = NULL;
int g_MBTraceLevel;
int g_MoreDetail = 1;
ism_threadh_t g_thrdHandleDoCommands = 0;

/* *************************************************************************************
 * syntaxhelp
 *
 * Description:  Syntax help on how to invoke mqttbench.
 *
 *   @param[in]  msg                 = x
 * *************************************************************************************/
void syntaxhelp (int helpType, char *msg)
{
	int i;
	int headerEnd = 7;

	const char *g_HelpHdrStr[] = {
		[0] = "mqttbench -cl <clientlist> [-crange <x1-x2,y1-y2>] [-d <seconds> | -c <numMsgs>] [-r <msgs/sec>]\n",
		[1] = "          [-s <min>-<max> | -M <directory>] [--mim <numMsgsInflight>] [-st <numThreads>] [-i <ID>]\n",
		[2] = "          [-tl <tracelevel>] [--clientTrace <regex>] [--nodnscache] [--noreconn] [-t <mode>]\n",
		[3] = "          [-T <mask>] [-cu <units>] [-ru <units>] [-u <units>] [-lcsv <filename>] [-rl <seconds>]\n",
		[4] = "          [--snap <options> <seconds>] [-csv <filename>] [--burst <int,dur,rate>] [-rr <numMsgs>]\n",
		[5] = "          [-b <seconds>] [-l <seconds>] [-as <loops>] [--sseq] [-nw] [--quit <type>] [-V <mask>]\n",
		[6] = "          [-dci] [-env] [-psk <filename>] [-jitter <int> <ct>] [-v] [--help]\n"
	};

	const char *g_HelpParmStr[] = {
		[PARM_CLIENTLIST]    = "   -cl <clientlist>         ; clientlist is the JSON file containing the configured list of clients to start.\n"
					           "                            ; See the mqttbench documentation and Python helper scripts for examples and how\n"
						       "                            ; to build an mqttbench client list files.\n",
		[PARM_CRANGE]        = "   -crange <x1-x2,y1-y2>    ; Range(s) of clients, as they appear in the JSON client list file, to be included in the test.\n"
		                       "                            ; multiple ranges are separated by a comma.\n",
		[PARM_DURATION]      = "   -d <seconds>             ; Test duration. A value 0 means run forever (default).\n",
		[PARM_COUNT]         = "   -c <numMsgs>             ; Publish a global message count (numMsgs), and then stop publishing.  Default is no limit.\n",
		[PARM_MSGRATE]       = "   -r <msg/sec>             ; Global message rate (msgs/sec) across all publishers.  For example, if the test is\n"
						       "                            ; configured with 3 publishers and -r 3000 is provided, then each publisher will\n"
						       "                            ; send 1000 msgs/sec.\n",
		[PARM_MSGSIZE]       = "   -s <min>-<max>           ; The range of messages sizes to be used in this test (units for min and max are bytes)\n"
						       "                            ; When <min>=<max> there is only 1 message size created. The default <min>=<max>=32 bytes.\n"
						       "                            ; 32 bytes is also the smallest supported binary generated message size.\n"
						       "                            ; These messages will be sent in round robin order per destination.\n",
		[PARM_PREDEFINED]    = "   -M <directory>           ; directory containing JSON messages files, each describing a message to be published by\n"
						       "                            ; publishers. -s and -M are mutually exclusive. See mqttbench documentation for the format\n"
						       "                            ; of the JSON message files\n",
		[PARM_MIM]           = "   --mim <numMsgsInflight>  ; Maximum Inflight Messages to allocate per client (QoS 1 and 2 only).\n",
		[PARM_NUMSUBTHRDS]   = "   -st <numThreads>         ; The number of submitter threads used to publish messages (default = 1). 1 thread is\n",
		[PARM_MBID]          = "   -i <ID>                  ; The ID of this instance of mqttbench (default = 0).  Used for message sequence checking\n",
		[PARM_TRACELEVEL]    = "   -tl <tracelevel>         ; set the trace level for mqttbench (valid values 1-9, 9 being very detailed trace).\n"
		 			           "                            ; Default is 5, only use >5 for debugging, not for benchmarking.\n",
		[PARM_CLIENTTRACE]   = "   --clientTrace \"<regex>\"  ; Match all client IDs with the provided regular expression and enable low level trace on\n"
						       "                            ; these clients to assist in debugging. Client trace can be disabled on the interactive command line.\n"
						       "                            ; Do not enable client trace while benchmarking. Put quotes around the regex to prevent evaluation by\n"
						       "                            ; the bash interpreter.\n",
		[PARM_NODNSCACHE]    = "   --nodnscache             ; If specified, then will re-resolve the DNS on every reconnect (default = 0).\n",
		[PARM_NORECONN]      = "   --noreconn               ; If specified, then will not attempt to reconnect on an error (default: reconnect).\n",
		[PARM_RATECONTROL]   = "   -t <mode>                ; Rate control modes:\n"
						       "                            ;   -2 = no rate control, and randomly select client and topic to publish next\n"
						       "                            ;   -1 = no rate control\n"
						       "                            ;    0 = individual submitter threads control rate (default).\n"
						       "                            ; sufficient for most tests.\n",
		[PARM_LATENCY]       = "   -T <mask>                ; Enable latency measurements:\n"
				               "                            ;    0x1:  enable RTT latency measurements.\n"
				               "                            ;    0x2:  enable latency measurement of the PUBACK time, i.e. PUB - PUBACK delta time.\n"
				               "                            ;    0x4:  enable latency measurement of the tcp/ssl connection.\n"
				               "                            ;    0x8:  enable latency measurement of the mqtt connection.\n"
				               "                            ;    0x10: enable latency measurement of the subscription.\n"
				               "                            ;    0x20: enable latency measurement of the tcp - mqtt connection.\n"
				               "                            ;    0x40: enable latency measurement of the tcp - subscription.\n"
				               "                            ;    0x80: enable the printing of the histogram for the latency specified.\n",
		[PARM_CUNITS]        = "   -cu <units>              ; The units to measure connection latency (i.e. units for microseconds is: 1e-6\n",
		[PARM_RUNITS]        = "   -ru <units>              ; The units to measure message latency during reconnects (i.e. units for \n"
				               "                            ; milliseconds is: 1e-3\n",
		[PARM_UNITS]         = "   -u <units>               ; The units to measure message latency (i.e. units for microseconds is: 1e-6\n",
		[PARM_LCSV]          = "   -lcsv <filename>         ; The file name of the csv file to write latency data to.\n",
		[PARM_RESETLAT]      = "   -rl <seconds>            ; Reset in-memory Latency Interval.  Time interval (seconds) when latency\n"
				               "                            ; statistics will be reset.  The minimum value is 10 seconds.\n",
		[PARM_SNAP]          = "   --snap <opts> <secs>     ; Print statistical snapshots at <secs> interval.  Options supported:\n"
				               "                            ;    connects    = Connection information\n"
				               "                            ;    msgrates    = Message Rates\n"
				               "                            ;    msgcounts   = Message Counts\n"
				               "                            ;    connlatency = Connection Latency, required if -T 0x4, 0x8, 0x10, 0x20, or 0x40\n"
				               "                            ;    msglatency  = Message Round Trip Latency, required if -T 0x1 \n",
		[PARM_CSV]           = "   -csv <filename>          ; The file name of the csv file to write throughput data to.\n",
		[PARM_BURST]         = "   --burst <int,dur,rate>   ; Periodically updates the message rate.  Parameters include:\n"
				               "                            ;    int  = Interval (in secs) between rate changes.\n"
				               "                            ;    dur  = Duration (in secs) of new message rate.\n"
				               "                            ;    rate = Message Rate (in msgs/sec) for the duration.\n",
		[PARM_RSTATS]        = "   -rr <numMsgs>            ; Reset statistics after numMsgs messages received by consumers.\n",
		[PARM_BSTATS]        = "   -b <seconds>             ; Begin collecting statistics after x seconds.\n",
		[PARM_LINGER]        = "   -l <seconds>             ; Specifies how long clients should stay connected, before closing their connections\n",
		[PARM_APPSIM]        = "   -as <# loops>            ; The number of loops in message processing to simulate application processing\n"
				               "                            ; of a message.\n",
		[PARM_SSEQ]          = "   --sseq                   ; Send/Subscribe to messages/topics sequentially (depth based).  By default sends\n"
				               "                            ; and subscribes are processed across clients breadth first (i.e. client1->topic1,\n"
				               "                            ; client2->topic1, client3->topic1....etc). Setting this option indicates that\n"
				               "                            ; sends and subscribes are processed across clients depth first (i.e. client1->topic1,\n"
				               "                            ; client1->topic2, client1->topic3, client2->topic1....etc).\n",
		[PARM_NOWAIT]        = "   -nw                      ; By default mqttbench waits for all client to connect and all subscriptions to complete\n"
				               "                            ; before publishing starts. Enabling this \"nowait\" flag tells mqttbench to start publishing after\n"
				               "                            ; the first publisher connects. This could result in uneven distribution of publish counts\n",
		[PARM_QUIT]          = "   --quit <type>            ; Type of termination:\n"
				               "                            ;    0 = UnSubscribe, Disconnect and close TCP connection (default).\n"
				               "                            ;    1 = Disconnect and close TCP connection.\n",
		[PARM_VALIDATION]    = "   -V <mask>                ; Enable message checking:\n"
				               "                            ;    0x1: enable message length check.\n"
				               "                            ;    0x2: enable message sequence number checking.\n",
		[PARM_DCI]           = "   -dci                     ; Log all the publish and subscribe topics per client to the file mb_ClientInfo.txt\n",
		[PARM_ENV]           = "   -env                     ; Display the current environment variables values and command line parameters.\n",
		[PARM_PSK]           = "   -psk <filename>          ; The filename that contains a list of PreShared Keys with IDs. When setting this parameter\n"
				               "                            ; the use must set the SSLCipher env variable to a PSK cipher, for example PSK-AES256-CBC-SHA.\n",
		[PARM_JITTER]        = "   -jitter <int> <ct>       ; Store the jitter on throughput specifying interval (<int> in milliseconds) along\n"
				               "                            ; with number of samples <ct>. Defaults: int=5000 ct=10000\n",
		[PARM_VERSION]       = "   -v(ersion)               ; mqttbench version information\n",
		[PARM_HELP]          = "   --help                   ; print this help.\n"
	};

	const char *g_HelpEnvStr[] = {
		[ENV_SIPLIST]        = "     - SIPList              ; The list of source IPv4 addresses to be used by clients. The MQTT server should be reachable\n"
                               "                            ; from these addresses.  By default mqttbench will use the first IPv4 address it can connect from.\n",
		[ENV_IOPROCTHRDS]    = "     - IOProcThreads        ; Number of I/O processor threads for network reads and writes. Default is 3. As a general\n"
                               "                            ; rule do not specify more than (<# of CPU> - 3)\n",
		[ENV_DELAYTIME]      = "     - DelayTime            ; Delay in microseconds between client connections. Used in conjunction with DelayCount.\n"
                               "                            ; the default is 0 delay, which is often too fast, typically 100-500 us is a reasonable value\n",
		[ENV_DELAYCOUNT]     = "     - DelayCount           ; Number of clients to connect between delays, specified by DelayTime. Default is 1.\n",
		[ENV_KEEPALIVE]      = "     - MQTTKeepAlive        ; Global MQTT Connection Keep Alive timer value for all client.  Default is 65535 seconds.\n",
		[ENV_SSLCIPHER]      = "     - SSLCipher            ; The TLS cipher suggested by the client during the TLS handshake. Default is RSA\n",
		[ENV_SSLMETHOD]      = "     - SSLClientMeth        ; The TLS protocol version used by the client during the TLS handshake. Default is TLS 1.2\n",
		[ENV_SOURCEPORTLO]   = "     - SourcePortLo         ; Specify the starting source port number used by clients when creating connections to a\n"
                               "                            ; server.  This env variable is ignored when UseEphemeralPorts env variable is enabled\n",
		[ENV_EPHEMERALPORTS] = "     - UseEphemeralPorts    ; Indicate that clients should not explicitly bind to a source port, and allow the\n"
                               "                            ; OS ephemeral port selection algorithm choose the source port. Default is 0 (Off).\n",
		[ENV_MAXIOPTXBUFS]   = "     - MaxIOPTXBuffers      ; Maximum number of buffers for each I/O processor thread TX buffer pool. Default is usually 32K.\n",
		[ENV_LOGPATH]        = "     - MqttbenchLogPath     ; Fully qualified path and name for the mqttbench trace log file. Default is the current directory\n"
                               "                            ; with the filename:  mqttbench_trace.log\n",
		[ENV_PIPECMDS]       = "     - PipeCommands         ; Allows user to send commands to the command thread from a named pipe, /tmp/mqttbench-np rather\n"
                               "                            ; than from stdin\n",
		[ENV_CONNHISTSIZE]   = "     - ConnHistSize         ; The size of the Connection latency histogram held in memory for -cu time units (default 100K).\n",
		[ENV_MSGHISTSIZE]    = "     - MsgHistSize          ; The size of the Message latency histogram held in memory for -u time units (default 100K).\n",
		[ENV_RETRYDELAY]     = "     - RetryDelay           ; Delay in microseconds before reconnecting client after a connection is broken.\n",
		[ENV_RETRYBACKOFF]   = "     - RetryBackoff         ; The Back-Off factor used to increase the delay time between each TCP reconnect.\n"
	};

	fprintf(stdout,"mqttbench (v %s)\n\n", MB_VERSION);

	if ((helpType == HELP_ALL) && (g_PrintUsage)) {
		for ( i = 0 ; i < headerEnd ; i++ ) {
			fprintf(stdout, "%s", g_HelpHdrStr[i]);
		}

		for ( i = (PARM_HELP_START + 1) ; i < PARM_HELP_END ; i++ ) {
			fprintf(stdout, "%s", g_HelpParmStr[i]);
		}

		fprintf(stdout,"\n\n");
		fprintf(stdout,	"   Environment variables:\n");

		for ( i = (ENV_HELP_START + 1) ; i < ENV_HELP_END ; i++ ) {
			fprintf(stdout, "%s", g_HelpEnvStr[i]);
		}

		g_PrintUsage = 0;
		g_MoreDetail = 0;
	}

	if ((helpType != HELP_ALL) && (helpType > PARM_HELP_START) && (helpType < PARM_HELP_END)) {
		fprintf(stdout, "%s\n", g_HelpParmStr[helpType]);

		if ((helpType == PARM_VERSION) || (helpType == PARM_HELP))
			g_MoreDetail = 0;
	}

	if ((msg) && (strcmp(msg, "\n") != 0) && (strlen(msg) > 0)) {
		fprintf(stdout,"%s\n",msg);
		MBTRACE(MBERROR, 1, "%s\n", msg);
	}

	fprintf(stdout, "\n");
	fflush(stdout);

} /* syntaxhelp */

/*
 * Set the signal handler for the process
 *
 * @param[in]   signal		= the signal sent to this process
 *
 */
void sighandler(int signal){
	MBTRACE(MBINFO, 1, "Received signal %s, mqttbench exiting with rc=%d...\n", strsignal(signal), g_MBErrorRC);
	fprintf(stdout,"mqttbench exiting, check the mqttbench trace file (%s) for execution details.\n", MQTTBENCH_TRACE_FNAME); fflush(stdout);

	int numIOP = g_MqttbenchInfo->mbSysEnvSet->numIOProcThreads;

	/* Tell I/O processors to stop processing */
	for (int i=0; i < numIOP; i++) {
		ioProcThread_t *iop = ioProcThreads[i];
		iop->isStopped = 1;
	}

	sleep(2); // give threads time to stop processing before exiting
	exit(g_MBErrorRC);
}

/* *************************************************************************************
 * main
 *
 * Description:  Start up mqttbench
 *
 *   @param[in]  argc                = number of command line arguments
 *   @param[in]  argv                = array of command line arguments
 *
 *   @return 0   = Successful completion
 * *************************************************************************************/
int main (int argc, char **argv) {
    int rc = 0;
    int i;
    int latencyMask = 0;
    int testDuration = 0;
    int numSubmitThrds = 0;
    int startupType = LOGONLY;

    char threadName[MAX_THREAD_NAME] = {0};
    char cTraceLogFile[MAX_DIRECTORY_NAME] = MQTTBENCH_TRACE_FNAME;

	double testStartTime;

    uint8_t connTimeCompleted = 0;

    /* Store TTY settings, in case they are lost during command thread cancellation */
    struct termios tty_stdin_restore;
    struct termios tty_stdout_restore;
    tcgetattr(STDIN_FILENO, &tty_stdin_restore);
    tcgetattr(STDOUT_FILENO, &tty_stdout_restore);

    /**********************************************************************************
     * Determine if there were only 2 arguments.   If so then see if requesting the
     * version number.   If so then provide version # and exit.
     *********************************************************************************/
    if ((argc == 2) &&
    	((strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "-version") == 0) || (strcmp(argv[1], "-ver") == 0))) {
    	SHOWVERSION
		g_MoreDetail = 0;
    	rc=0;
    	goto END;
    }

    /* Clean up pre-existing stats and log files */
    resetLogFiles();

    /* Initialize the ISM common utilities (create thread local storage for main thread, get system information, etc.) */
    ism_common_initUtil();

    /* Establish the location of the mqttbench_trace.log file. */
    rc = setMqttbenchLogFile(cTraceLogFile);
    if (rc) {
    	exit(rc);
    }

	/* Set the config entries for the log file.*/
	setMBConfig("TraceFile", cTraceLogFile);
	setMBConfig("TraceLevel", DEFAULT_TRACELEVEL);
	setMBConfig("TraceSelected", "5"); g_MBTraceLevel = 5;
	setMBConfig("TraceOptions", "where,thread,append,time");
	setMBConfig("TraceMessageData", "1024");
	setMBConfig("TraceMax", "300MB");
	setMBConfig("TraceFlush", "2000");

	/* Initialize the ISM Common Trace */
	ism_common_initTrace();

	/* Initialize the buffer pool */
	ism_bufferPoolInit();

	/* Install signal handler for the process, must be set after TRACE is configured */
	signal(SIGALRM, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGKILL, sighandler);
	signal(SIGINT,  sighandler);

	sigignore(SIGPIPE); /* in case other side closes the connection before connection bring-up has completed */

    /**********************************************************************************
     * Allocate memory for mqttbenchInfo_t structure (holds user input, e.g. command
     * line args, env variables, etc.)
     *********************************************************************************/
    rc = initMQTTBench();
    if (rc) {
		MBTRACE(MBERROR, 1, "Failed to initialize mqttbench data structures\n");
    	goto END;
    }

	environmentSet_t *pSysEnvSet = g_MqttbenchInfo->mbSysEnvSet;
	mbCmdLineArgs_t *pCmdLineArgs = g_MqttbenchInfo->mbCmdLineArgs;
	mbThreadInfo_t *pThreadInfo = g_MqttbenchInfo->mbThreadInfo;
	mbTimerInfo_t *pTimerInfo = g_MqttbenchInfo->mbTimerInfo;

   	/* Parse command line arguments */
 	rc = parseArgs(argc, argv);
	if (rc) {
        MBTRACE(MBERROR, 1, "Failed to parse command line arguments passed to mqttbench\n");
		goto END;
	}

	numSubmitThrds = pCmdLineArgs->numSubmitterThreads;
	testDuration = pCmdLineArgs->testDuration;
	latencyMask = pCmdLineArgs->latencyMask;

	/* Prepare the latency histogram files */
	if ((latencyMask & PRINTHISTOGRAM) > 0) {
		/* Turn off the bit in the mask and open the file to print the histogram to. */
		latencyMask &= ~PRINTHISTOGRAM;
		pCmdLineArgs->latencyMask = latencyMask;

		/* Open CSV files for latency statistics */
		MBTRACE(MBINFO, 2, "Preparing for latency testing by creating necessary histogram file(s).\n");
		rc = prep2PrintHistogram(g_fpHgramFiles, csvLatencyFile, latencyMask);
		if(rc){
			MBTRACE(MBERROR, 1, "Failed to create latency statistics files\n");
			goto END;
		}
	}

   	/* Process environment variables */
	rc = readEnv();
	if (rc) {
		MBTRACE(MBERROR, 1, "Failed to process environment variables used by mqttbench\n");
		goto END;
	}

	/* Process the mqttbench configuration file (aka client list file) */
	MBTRACE(MBINFO, 1, "Processing the client list configuration file\n");
	fprintf(stdout, "Processing the client list configuration file\n"); fflush(stdout);
	rc = processConfig(g_MqttbenchInfo);
	if(rc){
		MBTRACE(MBERROR, 1, "Failed to process the mqttbench configuration\n");
		goto END;
	} else {
		MBTRACE(MBINFO, 2, "Processed the client list configuration file and allocated/initialized the MQTT clients.\n");
	}

	/* Determine the submitter thread client composition (producers, consumers, or both) */
	rc = doDetermineNumClientTypeThreads(ACTUAL_THREADTYPE, numSubmitThrds,	g_MqttbenchInfo->mbInstArray, pThreadInfo);
	if (rc) {
		MBTRACE(MBERROR, 5, "Unable to determine submitter thread client composition\n");
		goto END;
	}

	/******************************************************************
	 * Check system settings and warn user if tuning is required
	 ******************************************************************/
	checkSystemSettings();

    /* --------------------------------------------------------------------------------
     * Check for inconsistencies with the environment which pertain to Producers and/or
     * Consumers.
     * -------------------------------------------------------------------------------- */
	rc = checkForEnvConflicts(g_MqttbenchInfo);
	if (rc) {
		MBTRACE(MBERROR, 1, "Failed with environment conflicts.\n");
		goto END;
	} else {
		MBTRACE(MBINFO, 2, "Completed checking environment conflicts.\n");

		/* Update local variables which are tested for conflicts and updated. */
		latencyMask = pCmdLineArgs->latencyMask;
	}

	/*******************************************************************
	 * Initialize tcpclient (also initializes OpenSSL for secure
	 * connection testing)
	 *******************************************************************/
	rc = tcpInit(g_MqttbenchInfo);
	if (rc) {
		MBTRACE(MBERROR, 1, "Failed to initialize the tcpclient module\n");
		goto END;
	}

    /* Create the timer threads and set the CPU Affinity */
    initTimerThreads();

    /* Write information about execution environment for support and troubleshooting */
    if (pCmdLineArgs->displayEnvInfo) {
    	startupType = LOGANDDISPLAY;
    }

	provideEnvInfo (GENERALENV, startupType, g_MqttbenchInfo);

    /* Create and start the I/O processor and I/O listener threads */
    rc = startIOThreads(g_MqttbenchInfo, g_TxBfrSize);
    if (rc) {
    	MBTRACE(MBERROR, 1, "Failed to start I/O processor and listener threads\n");
    	goto END;
    }

	/* --------------------------------------------------------------------------------
	 * If testDuration = 0 (i.e. run forever), then create interactive command line thread
	 * -------------------------------------------------------------------------------- */
	if (testDuration == 0) {
		int affMapLen;

		char  affMap[MAX_AFFINITY_MAP] = {0};
		char  envAffName[MAX_AFFINITY_NAME] = {0};
		char *envAffStr;

		memcpy(threadName, "commands", strlen("commands"));
		snprintf(envAffName, MAX_AFFINITY_NAME, "Affinity_%s", threadName);
		envAffStr = getenv(envAffName);
		affMapLen = ism_common_parseThreadAffinity(envAffStr, affMap);

		/* Start the consumer thread(s). */
		rc = ism_common_startThread(&g_thrdHandleDoCommands,
		                            doCommands,
		                            (void *)g_MqttbenchInfo,
		                            NULL,     /* context */
		                            0,        /* value   */
		                            ISM_TUSAGE_NORMAL,
		                            0,
		                            threadName,
		                            NULL);
		if (rc) {
			MBTRACE(MBERROR, 1, "Failed to start the commands thread (rc: %d).\n", rc);
			goto END;
		}

		if (affMapLen)
			ism_common_setAffinity(g_thrdHandleDoCommands, affMap, affMapLen);

		ism_common_sleep(SLEEP_1_SEC); /* give command thread a chance to start before starting submitter threads */
	}

    /* --------------------------------------------------------------------------------
     * Allocate, initialize and start submitter client threads. The RC will contain the
     * number of topics created. If < 1 then exit since there is nothing to do.
     * -------------------------------------------------------------------------------- */
   	rc = startSubmitterThreads(g_MqttbenchInfo, ioProcThreads, pLatencyUnits);
	if (rc) {
		MBTRACE(MBERROR, 1, "Failed to start the submitter threads (rc: %d).\n", rc);
		goto END;
	}
    /* --------------------------------------------------------------------------------
     * Check to see if there are any problems yet by looking at the following:
     *
     * g_MBErrorRC     = Mqttbench Error Return Code.
     * g_RequestedQuit = User requested quit or problem and threads exited and set it.
     * g_SubmitterExit = Counter which indicates how many threads exited.
     *
     * If so then need to cleanup and exit since there were issues.
     * --------------------------------------------------------------------------------- */
    if ((g_MBErrorRC) || (g_RequestedQuit) || (g_SubmitterExit)) {
        if (g_MBErrorRC > 0)
    		rc = g_MBErrorRC;

    	/* ----------------------------------------------------------------------------
    	 * If the global Submitter Exit Flag is not equal to the number of submitter
    	 * threads then try 3 more times to allow proper thread cleanup.  If the flag
    	 * doesnt equal the number of threads after 3 times then set the count to be
    	 * equal to the number of threads.
    	 * ---------------------------------------------------------------------------- */
    	if (g_SubmitterExit < numSubmitThrds) {
    		int exitCount;

    		for ( exitCount = 0 ; exitCount < 3 ; exitCount++ ) {
    			if (g_SubmitterExit < numSubmitThrds)
    				ism_common_sleep(SLEEP_1_SEC);
    			else
    				break;
    		}
   			g_SubmitterExit = numSubmitThrds;
    	}

    	goto END;
    }

	/* Capture the start time. */
	testStartTime = getCurrTime();

	/* --------------------------------------------------------------------------------
	 * Create Timer Tasks
	 *
	 * Type Supported:
	 *   - Snapshots
	 *   - Reset Latency
	 *   - Memory Trim
	 * -------------------------------------------------------------------------------- */
	/* Create a timer task if performing snapshots (-snap parameter). */
	rc = addMqttbenchTimerTasks(g_MqttbenchInfo);
	if (rc) {
		MBTRACE(MBERROR, 1, "There were errors while setting up the mqttbench timer tasks.\n");
		goto END;
	}

	if (pTimerInfo->snapTimerKey)
		connTimeCompleted = 1;  /* Set flag = 1 in order to be able to exit. */

	/* --------------------------------------------------------------------------------
	 * All the connections have been started and have started publishing messages.
	 * Check to see if the burst mode structure is NOT NULL.   If not NULL then
	 * schedule the initial burst mode timer.
	 * -------------------------------------------------------------------------------- */
	if (g_MqttbenchInfo->burstModeEnabled) {
		rc = addBurstTimerTask(pTimerInfo, g_MqttbenchInfo);
		if (rc) {
			MBTRACE(MBERROR, 1,	"Failed to create the bursty message rate timer task.\n");
			goto END;
		}
	}

	/* --------------------------------------------------------------------------------
	 * If performing latency testing and the time duration is either 0 or greater than
	 * 30 mins and the Env Variable DisableLatencyWarn is set to 0 or not set at all
	 * then set the timer up for providing warning message if there are latency values
	 * greater than the histogram can support.
	 * -------------------------------------------------------------------------------- */
	if (((testDuration == 0) || (testDuration > SECONDS_IN_30MINS))	&& (pSysEnvSet->disableLatencyWarn == 0)) {
		rc = addWarningTimerTask(g_MqttbenchInfo);
		if (rc) {
			MBTRACE(MBERROR, 1,	"Failed to create the big latency warning timer task.\n");
			goto END;
		}
	}

	/* Sleep for 1 second to allow the timer tasks to start up */
	ism_common_sleep(SLEEP_1_SEC);

	/* Run forever, waiting for user to quit from the interactive command line */
	if (testDuration == 0) {
		/* If AutoRenameFiles is set then add the AutoRenameLogFileTimer. */
		if (pSysEnvSet->autoRenameFiles == 1)
			addAutoRenameTimerTask(g_MqttbenchInfo);

		if (((g_MqttbenchInfo->instanceClientType & CONTAINS_PRODUCERS) > 0) && (pCmdLineArgs->globalMsgPubCount > 0)) {
			while (g_SubmitterExit < numSubmitThrds) { /*BEAM suppression: infinite loop*/
				ism_common_sleep(SLEEP_1_SEC);

				/* Check to see if there was an error or the user type quit in
				 * doCommands.  Need to break out of the while loop and exit. */
				if ((g_MBErrorRC) || (g_RequestedQuit))
					break;
			} /* while loop */

			if ((g_MBErrorRC == 0) && (g_RequestedQuit == 0) && (rc == 0)) {
				if (pCmdLineArgs->snapInterval == 0) {
					rc = getStats(g_MqttbenchInfo, OUTPUT_CONSOLE, 0, NULL);
					if (rc == 0) {
						if ((latencyMask & LAT_CONN_AND_SUBSCRIBE) > 0) {
							rc = provideConnStats(g_MqttbenchInfo,
									              g_MqttbenchInfo->instanceClientType,
								                  g_EndTimeConnTest,
								                  g_StartTimeConnTest,
								                  g_fpCSVLatencyFile,
								                  g_fpHgramFiles);

							connTimeCompleted = 1;  /* Set flag = 1 in order to be able to exit. */
						}
					}
				}

				if (rc == 0) {
					ism_time_t timer = NANO_PER_SECOND * 2;  /* wait at most 2 seconds for the command thread to terminate */
					ism_common_joinThreadTimed(g_thrdHandleDoCommands, NULL, timer);
				}
			} else {
				if (g_MBErrorRC) {
		   			ism_common_cancelThread(g_thrdHandleDoCommands);
					doStartTermination(g_MBErrorRC);
					ism_common_sleep(10 * SLEEP_1_SEC);
					rc = g_MBErrorRC;
					goto END;
				}
			}
		} else {
			ism_common_joinThread(g_thrdHandleDoCommands, NULL);
			if (pCmdLineArgs->snapInterval == 0 && !g_MBErrorRC) {
				rc = getStats(g_MqttbenchInfo, OUTPUT_CONSOLE, 0, NULL);
				if (rc == 0) {
					if ((latencyMask & LAT_CONN_AND_SUBSCRIBE) > 0) {
						rc = provideConnStats(g_MqttbenchInfo,
								              g_MqttbenchInfo->instanceClientType,
								              g_EndTimeConnTest,
											  g_StartTimeConnTest,
								              g_fpCSVLatencyFile,
											  g_fpHgramFiles);

						connTimeCompleted = 1; /* Set flag = 1 in order to be able to exit. */
					}
				}
			}

			if (g_MBErrorRC) {
				rc = g_MBErrorRC;
				goto END;
			}
		}
	} else if (testDuration > 0) { /* Duration is > 0, time boxed test */
		int numMsgs = 0;
		int sleepCt;

		ioProcThread_t *iop;

		/* ----------------------------------------------------------------------------
		 * Test to see if the user requested that the statistics and/or the timestamps
		 * need to be reset which will happen after the 1st message is received on this
		 * thread which must contain consumers.
		 *
		 * The 2 options that would cause this to occur are:
		 *   1) -b (Begin Stats) option
		 *   2) -rr (Reset after x number of messages) option.
		 * ---------------------------------------------------------------------------- */
		if (((g_MqttbenchInfo->instanceClientType & CONTAINS_CONSUMERS) > 0) &&
			(pCmdLineArgs->beginStatsSecs || pCmdLineArgs->resetTStampXMsgs)) {
			while (numMsgs == 0) {
				for ( i = 0, numMsgs = 0 ; i < pSysEnvSet->numIOProcThreads ; i++ ) {
					iop = ioProcThreads[i];
					numMsgs += iop->currRxMsgCounts[PUBLISH];
				}
			}

			MBTRACE(MBINFO, 1, "Received 1st message.\n");
		}

		MBTRACE(MBINFO, 1, "mqttbench will terminate in %d seconds (test duration)...\n", testDuration);
		fprintf(stdout, "\nmqttbench will terminate in %d seconds (test duration)...\n", testDuration);
		fflush(stdout);

		if (pCmdLineArgs->beginStatsSecs || pCmdLineArgs->resetTStampXMsgs) {
			if (pCmdLineArgs->beginStatsSecs) {
				ism_common_sleep(pCmdLineArgs->beginStatsSecs * SLEEP_1_SEC);
				MBTRACE(MBINFO, 1, "Resetting start time for collecting statistics.\n");
				testDuration -= pCmdLineArgs->beginStatsSecs;
			} else {
				while (numMsgs < pCmdLineArgs->resetTStampXMsgs) {
					for ( i = 0, numMsgs = 0 ; i < pSysEnvSet->numIOProcThreads ; i++ ) {
						iop = ioProcThreads[i];
						numMsgs += iop->currRxMsgCounts[PUBLISH];
						ism_common_sleep(SLEEP_1_USEC);
					}
				}

				MBTRACE(MBINFO, 1, "Received %d messages...resetting start time for collecting statistics.\n", pCmdLineArgs->resetTStampXMsgs);
			}

			submitResetStatsJob();
		}

		testStartTime = getCurrTime();

		/* ----------------------------------------------------------------------------
		 * Instead of sleeping the entire testDuration now sleep 1 sec and check to
		 * see if the mbErrorRC was set.  If so break.
		 * ---------------------------------------------------------------------------- */
		for ( sleepCt = 0 ; sleepCt < testDuration ; sleepCt++ ) {
			sleep(1);

			if (g_MBErrorRC)
				break;
		}

		/* If not performing snapshots then place the statistics in the csv file. */
		if (pCmdLineArgs->snapInterval == 0) {
			rc = getStats(g_MqttbenchInfo, OUTPUT_CSVFILE, testStartTime, g_fpCSVLogFile);
			if (rc == 0) {
				if ((latencyMask & LAT_CONN_AND_SUBSCRIBE) > 0) {
					rc = provideConnStats(g_MqttbenchInfo,
										  g_MqttbenchInfo->instanceClientType,
							              g_EndTimeConnTest,
							              g_StartTimeConnTest,
							              g_fpCSVLatencyFile,
							              g_fpHgramFiles);

			   		connTimeCompleted = 1;  /* Set flag = 1 in order to be able to exit. */
				}

				MBTRACE(MBINFO, 1, "Completed writing the stats to csv file.\n");
				fprintf(stdout, "Completed writing the stats to csv file.\n");
				fflush(stdout);
			}
		}
	} /* Test Duration is < 0) */

	if ((latencyMask & (CHKTIMETCPCONN | CHKTIMEMQTTCONN | CHKTIMESUBSCRIBE)) > 0) {
		while (!connTimeCompleted) /*BEAM suppression: infinite loop*/
			ism_common_sleep(SLEEP_1_SEC);
	}

	/* Check to see if there was an error (e.g. A closed connection due to a dropped message). */
	if (g_MBErrorRC)
		rc = g_MBErrorRC;

	/* --------------------------------------------------------------------------------
	 * Start termination.  This performs various things:  cleaning up memory, printing
	 * the statistics to the stats file and the latency to the latency file (-lcsv).
	 * -------------------------------------------------------------------------------- */
	doStartTermination(rc);

END:
	if (rc) {
		MBTRACE(MBERROR, 1, "mqttbench exiting with rc=%d...\n", rc);
	} else {
		MBTRACE(MBINFO, 1, "mqttbench exiting...\n");
	}

	/* Restore TTY settings in case settings were lost during command thread cancellation */
    tcsetattr(STDIN_FILENO, TCSANOW, &tty_stdin_restore);
    tcsetattr(STDOUT_FILENO, TCSANOW, &tty_stdout_restore);

    if (g_MoreDetail) {
    	fprintf(stdout,"\nmqttbench exiting, check the mqttbench trace file (%s) for execution details.\n\n", cTraceLogFile);
    	fflush(stdout);
    }

    return rc;
} /* main */
