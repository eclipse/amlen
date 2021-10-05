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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "cphUtil.h"
#include "cphTrace.h"

#ifndef _CPHLOG

/* Definitions of the various log levels which cph messages can be assigned to */
#define LOGNONE    0
#define LOGERROR   1
#define LOGWARNING 2
#define LOGINFO    3
#define LOGVERBOSE 4

/* The CPH_LOG control structure */
typedef struct s_cphLog {

    
    int verbosityToSTDOUT;    /* anything less than this goes to stdout */
    int verbosityToSTDERR;    /* anything less than this goes to stderr */
	int errorsReported;       /* Set to CPH_TRUE if any message is written to stderr */
    CPH_TRACE *pTrc;          /* We keep a pointer to the trace control block so the log facility can trace log lines */

} CPH_LOG;

/* Function prototypes */
int cphLogIni(CPH_LOG **ppLog, CPH_TRACE *pTrc);
void cphLogPrintLn(CPH_LOG *pLog, int logLevel, char *string);
void cphLogFree(CPH_LOG *pLog);
void setVerbosity(CPH_LOG *pLog, int i);
void setErrVerbosity(CPH_LOG *pLog, int i);

#define _CPHLOG
#endif
