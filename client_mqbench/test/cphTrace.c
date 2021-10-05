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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>

#include "cphTrace.h"
#include "cphUtil.h"
//#include "cphConfig.h"
#include "cphListIterator.h"

/*
** Method: cphTraceIni
**
** This function creates and initialises the cph trace control block. This is one such trace control block in a running
** cph process. The control block contains a list of CPH_TRACETHREAD * pointers which are used to maintain the thread
** specific trace data. The trace control block contains a CPH_SPINLOCK * pointer which is used when adding new threads
** to the list of thread specific data.
**
** Input Parameters: ppTrace - double pointer to the newly created control block
**
*/
void cphTraceIni(CPH_TRACE** ppTrace) {
    CPH_TRACE *pTrace = NULL;
    CPH_SPINLOCK *pTraceAddLock;
    int status = CPHTRUE;

    /* Give a warning that we are running in trace mode */
    CPHTRACEWARNING(CPHTRUE);


    if (CPHFALSE == cphSpinLock_Init( &(pTraceAddLock))) {
        //strcpy(errorString, "Cannot create trace add lock");
        //cphConfigInvalidParameter(pConfig, "Cannot create trace add lock");
        printf("ERROR - cannot create trace add lock");
        status = CPHFALSE;
    }

    if ((CPHTRUE == status) && (NULL != (pTrace = malloc(sizeof(CPH_TRACE))))) {
        pTrace->traceOn = CPHFALSE;
        cphArrayListIni(&(pTrace->threadlist));

        pTrace->pTraceAddLock = pTraceAddLock;

        pTrace->traceFileName[0] = '\0';
        pTrace->tFp = NULL;
    }
    *ppTrace = pTrace;
}

/*
** Method: cphTraceOpenTraceFile
**
** This function opens the trace file - we do not do this at the time of initialising the trace
** structure as at that time we have not read the configuration parameters to know whether 
** trace has been requested.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphTraceOpenTraceFile(CPH_TRACE *pTrace) {
    int status = CPHTRUE;

    sprintf(pTrace->traceFileName, "cph_%d.txt", cphUtilGetProcessId());
    if (NULL == (pTrace->tFp = fopen(pTrace->traceFileName, "wt"))) {
        printf("ERROR - cannot open trace file.\n");
        status = CPHFALSE;
    }

    return(status);
}

/*
** Method: cphTraceWarning
**
** This function displays a warning that trace has been compiled into cph and there may 
** therefore be a performance penalty
**
** Returns: CPHTRUE
**
*/
int cphTraceWarning(int dummy) {
   printf("\n%s%s%s%s%s",
          "***************************************************************************\n",
          "* WARNING - this is a trace build of cph. There is a performance penalty  *\n",
          "*           running with built in trace. Rebuild cph without the          *\n",
          "*           CPH_DOTRACE preprocessor definition to remove built in trace. *\n",
          "***************************************************************************\n\n");
return(CPHTRUE);
}

/*
** Method: cphTraceFree
**
** This method frees up the trace control block
**
** Input Parameters: ppTrace - double pointer to the trace control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int  cphTraceFree(CPH_TRACE** ppTrace) {
    if (NULL != *ppTrace) {
        if (NULL != (*ppTrace)->tFp) {
            fclose((*ppTrace)->tFp);
        }
        cphSpinLock_Destroy( &((*ppTrace)->pTraceAddLock));
        cphArrayListFree(&(*ppTrace)->threadlist);
        free(*ppTrace);
        *ppTrace = NULL;

        return(CPHTRUE);
    }

    return(CPHFALSE);
}

/*
** Method: cphTraceOn
**
** This function activates tracing by setting the traceOn member of the trace control block.
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphTraceOn(CPH_TRACE *pTrace) {
    if (NULL != pTrace) {
        pTrace->traceOn = CPHTRUE;
        return(CPHTRUE);
    }
    return (CPHFALSE);
}

/*
** Method: cphTraceOff
**
** This method deactivates tracing by unsetting the traceOn member of the trace control block.
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphTraceOff(CPH_TRACE *pTrace) {
    if (NULL != pTrace) {
        pTrace->traceOn = CPHTRUE;
        return(CPHTRUE);
    }
    return(CPHFALSE);
}

/*
** Method: cphTraceIsOn
**
** Function to inquire whether tracing has been activated or not.
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Returns: CPHTRUE if trace is activated and CPHFALSE otherwise
**
*/
int cphTraceIsOn(CPH_TRACE *pTrace) {
    int res = CPHFALSE;

    if (NULL != pTrace) {
        res = pTrace->traceOn;
    }
    return(res);
}

/*
** Method: cphTraceLine
**
** This function writes a line of data to the trace file. The line is preprended with correct date and time to
** milli-second granularitythe and the correct indentation level for this thread.
**
** Input Parameters: pTrace - pointer to the trace control block
**                   pTraceThread - pointer to the CPH_TRACETHREAD structure for the thread under 
**                                  which this activity is to be traced
**                   aLine - character string containing the line to be sent to the trace.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphTraceLine(CPH_TRACE *pTrace, CPH_TRACETHREAD *pTraceThread, char *aLine) {
    char Line[512] = { '\0' };
    char chTime[80];

    cphUtilGetTraceTime(chTime);
    if (pTraceThread->indent > 0) memset(Line, ' ', pTraceThread->indent);
    Line[pTraceThread->indent] = '\0';
    strcat(Line, aLine);
    //printf("%s\t%d\t%s\n", chTime, pTraceThread->threadId, Line);

    fprintf(pTrace->tFp, "%s\t%d\t%s\n", chTime, pTraceThread->threadId, Line);
    fflush(pTrace->tFp);
    return(CPHTRUE);
}

/*
** Method: cphTraceEntry
**
** This is the function that gets called by a function that wants to trace its entry.
** The function prepends a "{ " prefix to the function name before calling cphTraceLine
** which adds the time, thread number and correct indentation.
**
** Input Parameters: pTrace - pointer to the trace control block
**                   aFunction - character string which names the function
**
**
*/
void cphTraceEntry(CPH_TRACE *pTrace, char *aFunction) {
    char entryLine[80];
    CPH_TRACETHREAD *pTraceThread;

    if (CPHTRUE == pTrace->traceOn) {
        /* Find or allocate the thread slot for this thread */
        pTraceThread = cphTraceGetTraceThread(pTrace);

        pTraceThread->indent++;
        sprintf(entryLine, "{ %s.", aFunction);

        cphTraceLine(pTrace, pTraceThread, entryLine);
    }
}

/*
** Method: cphTraceExit
**
** This is the function that gets called by a function that wants to trace its exit.
** The function prepends a "} " prefix to the function name before calling cphTraceLine
** which adds the time, thread number and correct indentation.
**
** Input Parameters: pTrace - pointer to the trace control block
**                   aFunction - character string which names the function
**
**
*/
void cphTraceExit(CPH_TRACE *pTrace, char *aFunction) {
    char exitLine[80];
    CPH_TRACETHREAD *pTraceThread;

    if (CPHTRUE == pTrace->traceOn) {
        /* Find or allocate the thread slot for this thread */
        pTraceThread = cphTraceGetTraceThread(pTrace);
        
        sprintf(exitLine, "} %s.", aFunction);

        cphTraceLine(pTrace, pTraceThread, exitLine);
        pTraceThread->indent--;

    }
}

/*
** Method: cphTraceMsg
**
** This function allows a function to send an informative comment to the trace
** file. It supports a variable number of arguments with the syntax of a printf
** format string. Like cphTraceEntry and cphTraceExit, the function calls
** cphTraceLine to finish formatting the trace data with time, thread and indentation
** and write it to the trace file.
**
** Input Parameters: pTrace - pointer to the trace control block
**                   aFunction - character string naming the function being traced
**                   aFmt - printf style format string
**                   ...  - variable number of arguments according to the format string
**
**
*/
void cphTraceMsg(CPH_TRACE *pTrace, char *aFunction, char *aFmt, ...) {
    char Line[1024] = { '\0' };
    char *s;
    char c;
    char tmpbuff[80];
    int x;
    size_t len;
    va_list marker;
    CPH_TRACETHREAD *pTraceThread;

    if (CPHTRUE == pTrace->traceOn) {
        /* Find or allocate the thread slot for this thread */
        pTraceThread = cphTraceGetTraceThread(pTrace);

        va_start( marker, aFmt );     /* Initialize variable arguments. */
        while (*aFmt) {
            len=strlen(Line);

            if (*aFmt != '%')
                Line[len] = *aFmt;
            else {

                c = *++aFmt;
                /* %s - string */
                if (c == 's') {
                    s = va_arg(marker, char *);

                    /* Truncate any string at 80 bytes for the trace display */
                    strncat(Line, s, 256);
                    if (strlen(s) > 256) strcat(Line, "... ");
                } else {
                    /* Treat anything else as %d */
                    x = va_arg(marker, int);
                    sprintf(tmpbuff, "%d", x);
                    strcat(Line, tmpbuff);
                }
            }
            aFmt++;
        }
        va_end( marker );              /* Reset variable arguments.      */

        cphTraceLine(pTrace, pTraceThread, Line);
    }
}

/*
** Method: cphTraceGetTraceThread
**
** This function determines whether this thread is known to the exiting list of thread specific 
** data in the trace control block. If this thread is not known to the list, a new entry is added.
** If the thread is known of already, then the pointer to the relevant CPH_TRACETHREAD variable is
** determined. The address of the new or existing CPH_TRACETHREAD data is then returned to the 
** caller.
**
** Input Parameters: pTrace - pointer to the trace control block
** 
** Returns: a CPH_TRACETHREAD pointer to the new or existing CPH_TRACETHREAD data
**
*/
CPH_TRACETHREAD *cphTraceGetTraceThread(CPH_TRACE *pTrace) {
    int thisThreadId;
    CPH_TRACETHREAD *pTraceThread;
    int found = CPHFALSE;

    /* Get the thread id of this process */
    thisThreadId = cphUtilGetThreadId();

    /* If the list is empty then just set found to false so we add it to the list */
    if (CPHTRUE == cphArrayListIsEmpty(pTrace->threadlist))
        found = CPHFALSE;
    else {
        if (NULL != (pTraceThread = cphTraceLookupTraceThread(pTrace, thisThreadId))) {
            found = CPHTRUE;
        }
    }

    /* We don't know about this thread so it is a new thread we need to add to the list */
    if (CPHFALSE == found) {
        if (NULL != (pTraceThread = malloc(sizeof(CPH_TRACETHREAD)))) {
            pTraceThread->threadId = thisThreadId;
            pTraceThread->indent = CPHTRACE_STARTINDENT;
            cphSpinLock_Lock(pTrace->pTraceAddLock); 
            cphArrayListAdd(pTrace->threadlist, pTraceThread);
            cphSpinLock_Unlock(pTrace->pTraceAddLock); 
        }
    }

    if (NULL == pTraceThread) {
        printf("ERROR - internal trace ERROR!!\n");
        exit(1);
    }
    return(pTraceThread);
}

/*
** Method: cphTraceListThreads
**
** This function displays a summary of the threads that have been active in the program. This is called
** at the end of cph as a summary to help identify the various thread numbers.
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphTraceListThreads(CPH_TRACE *pTrace) {

    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM * pItem;

    CPH_TRACETHREAD *pTraceThread;

    /* If this thread is the same one as the last time we used trace then we still have the offset into the list */
 
        pIterator = cphArrayListListIterator( pTrace->threadlist);

        /* If the list is empty then just set found to false so we add it to the list */
        if (CPHTRUE == cphListIteratorHasNext(pIterator)) {
 
            /* Look up this thread id in our list */
            do {
                pItem = cphListIteratorNext(pIterator);
                pTraceThread = pItem->item;

                printf("   pItem: %p, pTraceThread: %p, pTraceThread->threadId: %d.\n", pItem, pTraceThread, pTraceThread->threadId);

            } while (cphListIteratorHasNext(pIterator));
            cphListIteratorFree(&pIterator);
        }

   return(CPHTRUE);
}

/*
** Method: cphTraceLookupTraceThread
**
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Output parameters: Lookup a thread id in the trace's list of registered trace numbers
**
** Returns: a CPH_TRACETHREAD pointer to the existing CPH_TRACETHREAD data
**
*/
CPH_TRACETHREAD *cphTraceLookupTraceThread(CPH_TRACE *pTrace, int threadId) {
    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM * pItem;
    CPH_TRACETHREAD *pTraceThread = NULL;
    int n=0;
    int found = CPHFALSE;

    pIterator = cphArrayListListIterator( pTrace->threadlist);


    /* Look up this thread id in our list */
    do {
        pItem = cphListIteratorNext(pIterator);
        pTraceThread = pItem->item;

        if (threadId == pTraceThread->threadId) {
            found = CPHTRUE;
            break;
        }
        n++;
    } while (cphListIteratorHasNext(pIterator));
    cphListIteratorFree(&pIterator);

    if (CPHFALSE == found) return(NULL);
    return(pTraceThread);
}
