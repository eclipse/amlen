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

/*
 * File: testTimerCUnit.c
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <ismutil.h>
#include <log.h>
#include <testLoggingCUnit.h>
#include <ismrc.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

//#define LOGFILE  "/tmp/ismservertest.log"
#define LOGFILE  "stdout"

extern ism_enumList enum_auxlogger_settings [];
extern ism_logWriter_t * g_logwriter[];
extern int ism_log_startLogTableCleanUpTimerTask();

extern int g_verbose;


/*   (programming note 1)
 * (programming note 11)
 * Array of timer tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_logging[] =
  {
        { "Logging Init Test", CUnit_test_ISM_Logging_init },
        { "Logging GetLogLevel Test", CUnit_test_ISM_Logging_getLogLevel },
        { "Logging GetTraceLevel Test", CUnit_test_ISM_Logging_getTraceLevelforLog },
        { "Logging getErrorString Test", CUnit_test_ISM_Logging_getErrorString },
        { "Logging getErrorStringByLocale Test", CUnit_test_ISM_Logging_getErrorStringByLocale },
        { "Logging setgetError Test", CUnit_test_ISM_Logging_setgetError },
        { "Logging formatLastError Test", CUnit_test_ISM_Logging_formatLastError },
        { "Logging conditionallyLogged", CUnit_test_ISM_Logging_ism_log_conditionallyLogged },
        { "Logging getLoggedCount", CUnit_test_ISM_Logging_ism_log_getLoggedCount },


        CU_TEST_INFO_NULL
   };

int initLoggingTests(void) {
   	int rc = 0;
    if(getenv("QUALIFY_SHARED") == NULL) {
        const char * userName = getenv("USER");
        if(userName) {
            setenv("QUALIFY_SHARED", userName, 1);
        }
    }
    return rc;
}

/* The timer tests cleanup function.
 * Removes any files created and terminates the timer thread.
 * Returns zero on success, non-zero otherwise.
 */
int cleanLoggingTests(void) {
    int rc = 0;
    return rc;
}

void CUnit_test_ISM_Logging_getLogLevel(void)
{
	printf("\nCUnit_test_ISM_Logging_getLogLevel\n");

	ISM_LOGLEV level;
	char * logLevel = "INFO";
	int rc = 0;

	logLevel = "INFO";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_INFO);

	logLevel = "I";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_INFO);

	logLevel = "WARN";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_WARN);

	logLevel = "W";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_WARN);

	logLevel = "NOTICE";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_NOTICE);

	logLevel = "N";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_NOTICE);

	logLevel = "ERROR";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_ERROR);

	logLevel = "E";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_ERROR);

	logLevel = "CRIT";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_CRIT);

	logLevel = "C";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_CRIT);

	logLevel = "INVALID";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(rc==-1);
}

void CUnit_test_ISM_Logging_getTraceLevelforLog(void)
{
	printf("\nCUnit_test_ISM_Logging_getLogLevel\n");

	ISM_LOGLEV level;
	char * logLevel = "INFO";
	int rc = 0;

	logLevel = "INFO";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_INFO);
	CU_ASSERT(ism_log_getTraceLevelForLog(level)==6);

	logLevel = "I";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_INFO);

	logLevel = "WARN";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_WARN);
	CU_ASSERT(ism_log_getTraceLevelForLog(level)==4);

	logLevel = "W";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_WARN);

	logLevel = "NOTICE";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_NOTICE);
	CU_ASSERT(ism_log_getTraceLevelForLog(level)==5);

	logLevel = "N";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_NOTICE);

	logLevel = "ERROR";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_ERROR);
	CU_ASSERT(ism_log_getTraceLevelForLog(level)==1);

	logLevel = "E";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_ERROR);


	logLevel = "CRIT";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_CRIT);

	logLevel = "C";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(level==ISM_LOGLEV_CRIT);
	CU_ASSERT(ism_log_getTraceLevelForLog(level)==1);

	logLevel = "INVALID";
	rc = ism_log_getISMLogLevel(logLevel,&level);
	CU_ASSERT(rc==-1);
}

void CUnit_test_ISM_Logging_getErrorString(void) {
	char * str = NULL;
	char buffer[300];

	str = (char *)ism_common_getErrorString(ISMRC_Failure, buffer, 300);
	CU_ASSERT(!strcmp(str, "Failure"));

	str = (char *)ism_common_getErrorString(ISMRC_AllocateError, buffer, 300);
	CU_ASSERT(!strcmp(str, "Unable to allocate memory."));

}


void CUnit_test_ISM_Logging_setgetError(void) {
	const char * logLevel;
    const char * logFile;
    ism_field_t f;

    ism_common_initUtil();

    f.type = VT_String;
    f.val.s = LOGFILE;

    ism_common_setProperty( ism_common_getConfigProperties(), "LogLocation.Destination.DefaultLog", &f);

    f.type = VT_String;
    f.val.s = "file";

    ism_common_setProperty( ism_common_getConfigProperties(), "LogLocation.LocationType.DefaultLog", &f);

    f.type = VT_String;
   	f.val.s = "WARN";
    ism_common_setProperty( ism_common_getConfigProperties(), "LogLevel", &f);

    logFile = ism_common_getStringConfig("LogLocation.Destination.DefaultLog");

    CU_ASSERT(!strcmp(logFile, LOGFILE));

    logLevel = ism_common_getStringConfig("LogLevel");
	CU_ASSERT(!strcmp(logLevel, "WARN"));

    CU_ASSERT(ism_log_createLogger(0, ism_common_getConfigProperties())==0);

	ism_common_setError(ISMRC_Failure);
	CU_ASSERT(ism_common_getLastError() == ISMRC_Failure);
}


void CUnit_test_ISM_Logging_formatLastError(void) {
	char buffer[300];

    ism_common_initUtil();

	ism_log_init();

	ism_common_setError(ISMRC_Failure);
	ism_common_formatLastError((char *)&buffer, sizeof(buffer));
	// printf("Last Error Formatted: %s.\n", buffer);
	CU_ASSERT(!strcmp(buffer,"Failure"));

	ism_common_setErrorData(ISMRC_AllocateError, " ");
	ism_common_formatLastError((char *)&buffer, sizeof(buffer));
	// printf("Last Error Formatted: %s.\n", buffer);
	CU_ASSERT(!strcmp(buffer, "Unable to allocate memory."));
	ism_log_term();
}

void CUnit_test_ISM_Logging_init(void) {
    printf("\nCUnit_test_ISM_Logging_init\n");
    const char * logLevel;
    const char * logFile;
    ism_field_t f;
    char * nulval;

    ism_common_initUtil();

    f.type = VT_String;
    f.val.s = LOGFILE;

    ism_common_setProperty( ism_common_getConfigProperties(), "LogLocation.Destination.DefaultLog", &f);

    f.type = VT_String;
    f.val.s = "file";

    ism_common_setProperty( ism_common_getConfigProperties(), "LogLocation.LocationType.DefaultLog", &f);

    f.type = VT_String;
   	f.val.s = "WARN";
    ism_common_setProperty( ism_common_getConfigProperties(), "LogLevel", &f);

    logFile = ism_common_getStringProperty(
                                    ism_common_getConfigProperties(),
                                    "LogLocation.Destination.DefaultLog");

    CU_ASSERT(!strcmp(logFile, LOGFILE));

    logLevel = ism_common_getStringProperty(
                                    ism_common_getConfigProperties(),
                                    "LogLevel");
	CU_ASSERT(!strcmp(logLevel, "WARN"));

	ism_log_init();

    CU_ASSERT(ism_log_createLogger(0, ism_common_getConfigProperties())==0);

    LOG(CRIT, Transport, 2000, "%d%-s%s", "The log value is {0}. {1}. UTF8Str={2}", 200, " CRIT Logging", "\x61\x62\x63");
    LOG(ERROR, Transport, 2000, "%d%s", "The log value is {0}. {1}", 201, " ERROR Logging \x61\x62\x63");
    LOG(WARN, Transport, 2000, "%d%s", "The log value is {0}. {1}", 202, " WARN Logging \x61\x62\x63");
    LOG(NOTICE, Transport, 2000, "%d%-s", "The log value is {0}. {1}", 203, " NOTICE Logging \" \r \n \x61\x62\x63");
    LOG(INFO, Transport, 2000, "%d%s", "The log value is {0}. {1}", 204, " Info Logging \x61\x62\x63");
    nulval = (char *)NULL;
    LOG(INFO, Transport, 2200, "%d%s", "With NULL replacement Data. {0} {1}", 204, nulval);


    f.type = VT_String;
   	f.val.s = "CRIT";
    ism_common_setProperty( ism_common_getConfigProperties(), "LogLevel", &f);
    LOG(INFO, Transport, 2005, "%d%s", "Trace Message {0}. {1}", 204, " Info Logging");
    LOG(INFO, Transport, 2006, "%d%s", "Trace Message {0}. {1}", 204, " Info Logging");
    LOG(INFO, Transport, 2007, "%d%s", "Trace Message {0}. {1}", 204, " Info Logging");
    ism_log_term();

}
void CUnit_test_ISM_Logging_getErrorStringByLocale(void)
{
	char * str =NULL;
	char buffer[300];
	char *bufptr=(char *)&buffer;
	const char * defaultLocale = ism_common_getLocale();
    if (g_verbose)
	    printf("Locale is : %s\n", defaultLocale);

	str = (char *)ism_common_getErrorStringByLocale(ISMRC_Failure, NULL, (char *)bufptr, 300);
	CU_ASSERT(!strcmp(str,"Failure"));
}

int ism_log_setSummarizeLogsEnable(int enabled);

void CUnit_test_ISM_Logging_ism_log_conditionallyLogged(void)
{
	int rcount=10;
	int count=0;

	ism_common_initUtil();

	ism_log_init();

	int x=0;

	ism_log_setSummarizeLogsEnable(1);

	for(x=0; x< rcount; x++){
		count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 900, TRACE_DOMAIN,  NULL, NULL, NULL);
		CU_ASSERT(count==x);
	}

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN),  ISM_MSG_CAT(Connection), 922, TRACE_DOMAIN, "ClientID1", NULL, NULL);
			CU_ASSERT(count==x);
	}

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN),  ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN, "ClientID2", "IP1", NULL);
			CU_ASSERT(count==x);
	}

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection),932, TRACE_DOMAIN, "ClientID3", "IP1", NULL);
			CU_ASSERT(count==x);
	}
	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN, "ClientID4", "IP1", NULL);
			CU_ASSERT(count==x);
	}

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN, "ClientID4", "IP2", NULL);
			CU_ASSERT(count==x);
	}
	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN, "ClientID4", "IP3", NULL);
			CU_ASSERT(count==x);
	}
	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN) , ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN , "ClientID4", "IP4", NULL);
			CU_ASSERT(count==x);
	}

	/*Test with Reason*/
	/*char * reason="reason1";
	for(x=0; x< rcount; x++){
		count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN) , ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN , "ClientID4", "IP4", reason);
		printf("Count:%d. X:%d\n", count, x);
		CU_ASSERT(count==x);
	}

	reason="reason2";
	for(x=0; x< rcount; x++){
		count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN) , ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN , "ClientID4", "IP4", reason);
		printf("Count:%d. X:%d\n", count, x);
		CU_ASSERT(count==x);
	}

	char reason_buf[128];
	for(x=0; x< rcount; x++){
		sprintf(reason_buf, "reasonS%d", x);
		count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN) , ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN , "ClientID4", "IP4", reason_buf);
		printf("Count:%d. X:%d\n", count, x);
		CU_ASSERT(count==0);
	}*/

	ism_log_term();

}

int ism_common_getLoggedCount(int msgCode, const char * clientID, const char * sourceIP, const char * reason);


void CUnit_test_ISM_Logging_ism_log_getLoggedCount(void)
{
	int rcount=10;
	int count=0;

	ism_common_initUtil();

	ism_log_init();

	ism_common_initTimers();
	ism_log_startLogTableCleanUpTimerTask();

	ism_log_setSummarizeLogsEnable(1);

	int x=0;

	for(x=0; x< rcount; x++){
		count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 900,  TRACE_DOMAIN, NULL, NULL, NULL);
		CU_ASSERT(count==x);
	}
	count = ism_common_getLoggedCount(900, NULL, NULL, NULL);
	printf("LogCount=%d. Xvalue=%d\n", count, x);

	CU_ASSERT(count==x);

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 922, TRACE_DOMAIN,  "ClientID1", NULL, NULL);
			CU_ASSERT(count==x);
	}
	count = ism_common_getLoggedCount(922, "ClientID1", NULL, NULL);
	CU_ASSERT(count==x);

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 932,  TRACE_DOMAIN, "ClientID2", "IP1", NULL);
			CU_ASSERT(count==x);
	}
	count = ism_common_getLoggedCount(932, "ClientID2", "IP1", NULL);
	CU_ASSERT(count==x);

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN, "ClientID3", "IP1", NULL);
			CU_ASSERT(count==x);
	}
	count = ism_common_getLoggedCount(932,  "ClientID3", "IP1", NULL);
	CU_ASSERT(count==x);

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN, "ClientID4", "IP1", NULL);
			CU_ASSERT(count==x);
	}
	count = ism_common_getLoggedCount(932,  "ClientID4", "IP1", NULL);
	CU_ASSERT(count==x);

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 932,  TRACE_DOMAIN, "ClientID4", "IP2", NULL);
			CU_ASSERT(count==x);
	}
	count = ism_common_getLoggedCount(932,  "ClientID4", "IP2", NULL);
	CU_ASSERT(count==x);

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN, "ClientID4", "IP3", NULL);
			CU_ASSERT(count==x);
	}
	count = ism_common_getLoggedCount(932, "ClientID4", "IP3", NULL);
	CU_ASSERT(count==x);

	for(x=0; x< rcount; x++){
			count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 932,  TRACE_DOMAIN, "ClientID4", "IP4", NULL);
			CU_ASSERT(count==x);
	}
	count = ism_common_getLoggedCount(932,  "ClientID4", "IP4", NULL);
	CU_ASSERT(count==x);

	/*Test with Reason*/
	/*char * reason="reason1";
	for(x=0; x< rcount; x++){
		count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN) , ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN , "ClientID4", "IP4", reason);
		CU_ASSERT(count==x);
	}
	count = ism_common_getLoggedCount(932,  "ClientID4", "IP4", reason);
	CU_ASSERT(count==x);

	reason="reason2";
	for(x=0; x< rcount; x++){
		count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN) , ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN , "ClientID4", "IP4", reason);
		CU_ASSERT(count==x);
	}
	count = ism_common_getLoggedCount(932,  "ClientID4", "IP4", reason);
	CU_ASSERT(count==x);

	char reason_buf[128];
	for(x=0; x< rcount; x++){
		sprintf(reason_buf, "reasonS%d", x);
		count = ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN) , ISM_MSG_CAT(Connection), 932, TRACE_DOMAIN , "ClientID4", "IP4", reason_buf);
		CU_ASSERT(count==0);
		count = ism_common_getLoggedCount(932,  "ClientID4", "IP4", reason_buf);
		CU_ASSERT(count==1);

	}*/


	ism_log_term();

}
