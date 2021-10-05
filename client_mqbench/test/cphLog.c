const static char sccsid[] = "%Z% %W% %I% %E% %U%";
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

/* The functions in this module provide a logging mechanism for cph. This permits messages to 
** be written to stdout or stderr according to the specified verbosity levels and the level
** at which an individual message is written */

#include "cphLog.h"

/*
** Method: cphLogIni
**
** Create and initialise a CPH_LOG control block.
**
** Output Parameters: ppLog - double pointer in which the address for the newly created 
** control block is set 
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphLogIni(CPH_LOG **ppLog, CPH_TRACE *pTrc) {

	CPH_LOG *pLog = NULL;

	pLog = malloc(sizeof(CPH_LOG));
	if (pLog != NULL) {
	   pLog->verbosityToSTDOUT = LOGVERBOSE;
	   pLog->verbosityToSTDERR = LOGNONE;
	   pLog->errorsReported = CPHFALSE;
       pLog->pTrc = pTrc;
	}
	*ppLog = pLog;
	return CPHTRUE;
}

/*
** Method: cphLogFree
**
** This method frees the log control block. The pointer to the control 
** block is set to NULL on successful execution.
**
** Input/Output Parameters: ppLog - double pointer to the CPH_LOG control block
**
*/
void cphLogFree(CPH_LOG *pLog) {
	if (NULL != pLog) {
		free(pLog);
		pLog = NULL;
	}
}

/*
** Method: cphLogPrintLn
**
** Write a specified message to the logging mechanism according to the
** specified verbosity levels
**
** Input Parameters: pLog - a pointer to the CPH_LOG control structure
**                   lovLevel - the verbosity level pertaining to this message
**                   aString - character string containing the contents of the log message
**
*/
void cphLogPrintLn(CPH_LOG *pLog, int logLevel, char *string)
{
#define CPH_ROUTINE "cphLogPrintLn"
    CPHTRACEENTRY(pLog->pTrc, CPH_ROUTINE );

    if ( logLevel==LOGERROR ) {
        pLog->errorsReported = CPHTRUE;
    }
    if ( logLevel <= pLog->verbosityToSTDERR ) {
        fprintf(stderr, "%s\n", string);
        fflush(stderr);
    } else if (logLevel<=pLog->verbosityToSTDOUT ) {
        printf("%s\n", string);
        fflush(stdout);
    }

    /* Write the log message to the trace file if we are tracing */
    CPHTRACEMSG(pLog->pTrc, "%s", string);

    CPHTRACEEXIT(pLog->pTrc, CPH_ROUTINE );

}

/*
** Method: setVerbosity
**
** This function is called to set the stdout verbosity level. Log messages with a 
** verbosity level less than or equal to this (but greater than the stderr verbosity
** level), will be written to stdout.
**
** Input Parameters: pLog - pointer to the CPH_LOG control block
**                   i - the new level for the stdout verbosity
**
*/
void setVerbosity(CPH_LOG *pLog, int i) {
   pLog->verbosityToSTDOUT = i;
}

/*
** Method: setErrVerbosity
**
** This function is called to set the stderr verbosity level. Log messages with a 
** verbosity level less than or equal to this will be written to stderr and not to
** stdout.
**
** Input Parameters: pLog - pointer to the CPH_LOG control block
**                   i - the new level for the stderr verbosity
**
*/
void setErrVerbosity(CPH_LOG *pLog, int i) {
   pLog->verbosityToSTDERR = i;
} 
