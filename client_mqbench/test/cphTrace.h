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

#ifndef _CHPTRACE

#define CPHTRACE_STARTINDENT 1

#include "cphArrayList.h"
#include "cphSpinLock.h"

/* These defines are used to enable or disable trace accordng to whether the preprocessor directive CPH_DOTRACE
   has been set */
#ifdef CPH_DOTRACE
#define CPHTRACEINI   cphTraceIni
#define CPHTRACEFREE  cphTraceFree
#define CPHTRACEOPENTRACEFILE cphTraceOpenTraceFile
#define CPHTRACEON    cphTraceOn
#define CPHTRACEENTRY cphTraceEntry
#define CPHTRACEEXIT  cphTraceExit
#define CPHTRACEMSG   cphTraceMsg
#define CPHTRACELINE  cphTraceLine
#define CPHTRACEWARNING cphTraceWarning
#else
#define CPHTRACEINI(a) (void)a
#define CPHTRACEFREE(a) (void)a
#define CPHTRACEOPENTRACEFILE(a) (void)a
#define CPHTRACEON(a) (void)a
#define CPHTRACEENTRY(a,b) (void)a;(void)b
#define CPHTRACEEXIT(a,b) (void)a;(void)b
#define CPHTRACEMSG(a,b,...) (void)a;(void)b
#define CPHTRACELINE(...) 
#define CPHTRACEWARNING(a) (void)a
#endif

/* The CPH_TRACETHREAD structure - we maintain a linked list of these, there is one CPH_TRACETHREAD slot for every thread registered
   to cph */
typedef struct _cphtracethread {
    int threadId;
	int indent;
} CPH_TRACETHREAD;

/* The CPH_TRACE structure - there is a single one of these for a cph process */
typedef struct _cphtrace {
    int traceOn;                 /* Flag used to stop and start tracing */
    char traceFileName[256];     /* The filename of the trace output file */
    FILE *tFp;                   /* File pointer for the trace file */
    CPH_ARRAYLIST *threadlist;   /* Pointer to the list of CPH_TRACETHREAD entries - there is one per cph thread */
    CPH_SPINLOCK *pTraceAddLock; /* Lock to be used when adding entries to the CPH_TRACETHREAD list */
} CPH_TRACE;

/* Function prototypes */
void cphTraceIni(CPH_TRACE** ppTrace);
int cphTraceWarning(int dummy);
int  cphTraceFree(CPH_TRACE** ppTrace);
int cphTraceOpenTraceFile(CPH_TRACE *pTrace);
int cphTraceOn(CPH_TRACE *pTrace);
int cphTraceOff(CPH_TRACE *pTrace);
int cphTraceIsOn(CPH_TRACE *pTrace);
void cphTraceEntry(CPH_TRACE *pTrace, char *aFunction);
void cphTraceExit(CPH_TRACE *pTrace, char *aFunction);
void cphTraceMsg(CPH_TRACE *pTrace, char *aFunction, char *aFmt, ...);
int cphTraceLine(CPH_TRACE *pTrace, CPH_TRACETHREAD *pTraceThread, char *aLine);
int cphTraceListThreads(CPH_TRACE *pTrace);
CPH_TRACETHREAD *cphTraceGetTraceThread(CPH_TRACE *pTrace);
CPH_TRACETHREAD *cphTraceLookupTraceThread(CPH_TRACE *pTrace, int threadId);

#define _CHPTRACE
#endif
