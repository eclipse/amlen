/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
* Change Log:
* -------- ----------- ------------- ------------------------------------
* Reason:  Date:       Originator:   Comments:
* -------- ----------- ------------- ------------------------------------
*          10-04-07    Jerry Stevens File Creation
*/

#ifndef _CPHUTIL

/* CPHTRUE and CPHFALSE are used everywhere as an explicit TRUE/FALSE check */
#define CPHTRUE  1
#define CPHFALSE 0

/* This definition is used by the subscriber to indicate no message was received by cphSubsriberOneIteration in 
   the wait interval */
#define CPH_RXTIMEDOUT -1

/* Define the CPH_TIME structure - a machine independent wrapper for machine dependent time values */
#if defined(WIN32)
#include <Windows.h>
#define CPH_TIME ULARGE_INTEGER
#elif defined(AMQ_UNIX)
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#define CPH_TIME struct timeval
#endif

#include <signal.h>
#include <cmqc.h>

/* Function prototypes */
void cphUtilSleep( int mSecs );
CPH_TIME cphUtilGetCurrentTimeMillis(void);
long cphUtilGetTimeDifference(CPH_TIME time1, CPH_TIME time2);
int cphUtilTimeCompare(CPH_TIME time1, CPH_TIME time2);
int cphUtilTimeIni(CPH_TIME *pTime);
int cphUtilTimeIsZero(CPH_TIME pTime);
int cphUtilGetThreadId(void);
int cphUtilGetProcessId(void);
void cphUtilSigInt(int dummysignum);
int cphUtilGetTraceTime(char *chTime);
char *cphUtilRTrim(char *aLine);
char *cphUtilLTrim(char *aLine);
char *cphUtilTrim(char *aLine);
int cphUtilStringEndsWith(char *aString, char *ending);
char *cphUtilstrcrlf(char *aString);
char *cphUtilstrcrlfTotabcrlf(char *aString);
char *cphUtilMakeBigString(int size);
char *cphUtilMakeBigStringWithRFH2(int size, size_t *pRfh2Len);
char *cphBuildRFH2(MQLONG *pSize);
int cphGetEnv(char *varName, char *varValue, size_t buffSize);

#define _CPHUTIL
#endif
