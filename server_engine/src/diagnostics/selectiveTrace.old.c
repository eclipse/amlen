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
#include <stdbool.h>
#include "ismutil.h"

#include <sys/mman.h>        /* shm_* stuff, and mmap() */
#include <sys/stat.h>        /* For mode constants      */
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>           /* For O_* constants       */
#include <limits.h>          /* file length limits      */

//
//In order to compile this trace module outside of the build system:
//
//gcc -c -o selectiveTrace.o selectiveTrace.c -Wall -Wold-style-definition -fPIC -rdynamic -D_GNU_SOURCE -std=gnu99  -O3 -g -DNDEBUG -I$BROOT/server_ship/include -I. -DISMDATE=`date +%Y-%m-%d` -DISMTIME=`date +%H:%M` -DBUILD_LABEL= -DBUILD_ID= -DISM_VERSION=""
//gcc -pthread -Wl,--no-as-needed -Wl,--no-warn-search-mismatch  -o libseltrace.so -O3 -g -DNDEBUG  -Wall -Wold-style-definition  -shared -shared-libgcc  -Xlinker -Map -Xlinker libseltrace.so.map  -Wl,-soname,libseltrace.so -Wl,--enable-new-dtags selectiveTrace.o -lrt -lm  -ldl

//Defines that control which bits of info this module traces out:
#define ALLOW_TIMESTAMP     // Uncommenting this line adds a timestamp to each traceline
#define ALLOW_THREADNAMES   // Commenting this disables thread names
#define ALLOW_SOURCELINES   // Uncommenting this line adds file+line to each traceline

#ifdef ALLOW_TIMESTAMP
void ism_common_checkTimezone(ism_ts_t * ts);
#define NANOS_IN_HOUR 3600000000000L
#endif

typedef enum tag_traceAction_t {
    noTrace = 0,  // Don't trace
    activate,     // Activate tracing
    deactivate,   // Deactivate tracing
    single,       // Single trace line
    roundup,
    addLine,      // Add this line to the trace, but don't dump
    dumpTrace     // Add this line to the trace and dump
} traceAction_t;

//Path to write dumps to
#define DUMPPATH "/ima/logs/"

typedef struct tag_traceBuffer_t {
    uint32_t lineLength;
    uint32_t traceId;
    uint64_t numLines;
    uint64_t nextTraceLine;
    struct tag_traceBuffer_t *next;
    ism_ts_t *trc_ts;
    int trc_ts_hour;
    char traceBuffer[]; //actually numLines * lineLength
} traceBuffer_t ;

// Per-thread variables controlling whether this thread is tracing or not
__thread uint32_t traceActive = 0;
__thread traceBuffer_t *traceBuffer = NULL;
__thread char traceFilename[256] = "";

traceBuffer_t *allBuffers = NULL;
pthread_spinlock_t allLock = {0};
uint32_t allId = 1;

// Function signatures that change the state of the trace module
typedef struct tag_funcSpec_t {
    int lineno;
    const char *source;
} funcSpec_t;

// Trace lines that activate tracing
const funcSpec_t activateFunc[]   = { {0,    NULL} };                     // <SENTINEL>
// Trace lines that deactivate tracing
const funcSpec_t deactivateFunc[] = { {0,    NULL} };                     // <SENTINEL>
// Individual lines of interest (perhaps that happen asynchronously)
const funcSpec_t singleFunc[]     = { {0,    NULL} };                     // <SENTINEL>
// A single function that is used to round up tracing
const funcSpec_t roundupFunc      = {0, NULL};                            // <SENTINEL>

char *fixedFilename = DUMPPATH "selectiveTrace"; // NULL for separate file names by pid&tid

uint64_t g_numLines = 10000; //10000;
uint32_t g_lineLength = 300;

traceBuffer_t *allocateTraceBuffer(uint64_t numLines, uint32_t lineLength);
void traceDump(traceBuffer_t *buffer, char *filename);

int ism_initTraceModule(  ism_prop_t *props
                        , char *errorBuffer
                        , int errorBuffSize
                        , int *trclevel )
{
    int rc = pthread_spin_init(&allLock, PTHREAD_PROCESS_PRIVATE);

    // Force the highest trace level
    // *trclevel = 9;

    return rc;
}

//Optional, called whenever properties are updated...
void ism_insertCfgUpdate(ism_prop_t *props)
{
    // Do nothing
}

// Should we be increasing or decreasing the trace activation level?
traceAction_t getTraceAction(int lineno, const char *file, const char **fmt)
{
    traceAction_t action = noTrace;

    if (fmt != NULL && *fmt != NULL && (*fmt)[0] == '~')
    {
        // Activate
        if ((*fmt)[1] == 'A')
        {
            action = activate;
            *fmt += 2;
        }
        // Deactivate
        else if ((*fmt)[1] == 'D')
        {
            action = deactivate;
            *fmt += 2;
        }
        // Single line
        else if ((*fmt)[1] == 'S')
        {
            if (traceActive == 0) action = single;
            *fmt += 2;
        }
        // Round-up all active trace information
        else if ((*fmt)[1] == 'R')
        {
            action = roundup;
            *fmt += 2;
        }
    }
    else
    {
        // Activate
        const funcSpec_t *checkFunc = activateFunc;

        while(checkFunc->source != NULL)
        {
            if (lineno == checkFunc->lineno && strstr(file, checkFunc->source))
            {
                action = activate;
                goto mod_exit;
            }

            checkFunc++;
        }

        // Deactivate
        checkFunc = deactivateFunc;

        while(checkFunc->source != NULL)
        {
            if (lineno == checkFunc->lineno && strstr(file, checkFunc->source))
            {
                action = deactivate;
                goto mod_exit;
            }

            checkFunc++;
        }

        // Trace individual lines (e.g. async events)
        if (traceActive == 0)
        {
            checkFunc = singleFunc;

            while(checkFunc->source != NULL)
            {
                if (lineno == checkFunc->lineno && strstr(file, checkFunc->source))
                {
                    action = single;
                    goto mod_exit;
                }

                checkFunc++;
            }
        }

        // Round up all active trace information
        if (roundupFunc.source != NULL && lineno == roundupFunc.lineno && strstr(file, roundupFunc.source))
        {
            action = roundup;
        }
    }

mod_exit:

    if (action == activate)
    {
        traceActive++;

        if (traceActive == 1 && traceBuffer == NULL)
        {
            traceBuffer = allocateTraceBuffer(g_numLines, g_lineLength);
        }
    }
    else if (action == deactivate)
    {
        traceActive--;

        if (traceActive == 0)
        {
            action = dumpTrace;
        }
    }
    else if (action == single)
    {
        if (traceActive == 0)
        {
            if (traceBuffer == NULL) traceBuffer = allocateTraceBuffer(g_numLines, g_lineLength);
            if (traceBuffer != NULL) action = addLine; //dumpTrace;
        }
    }
    else if (action == roundup)
    {
        if (pthread_spin_lock(&allLock) == 0)
        {
            traceBuffer_t *thisBuffer = allBuffers;

            while(thisBuffer)
            {
                traceDump(thisBuffer, NULL);
                thisBuffer = thisBuffer->next;
            }

            pthread_spin_unlock(&allLock);
        }

        action = noTrace;
    }

    if (traceActive != 0 && traceBuffer != NULL) action = addLine;

    return action;
}

XAPI void ism_insertTrace(int level, int opt, const char * file, int lineno, const char * fmt, ...)
{
    va_list  arglist;
    int cur;
    int inlen  = 0;
    traceAction_t action = getTraceAction(lineno, file, &fmt);

    // Only interested if we're between our trace lines

    if (action == noTrace) return;

    cur = traceBuffer->nextTraceLine;

    if ((++(traceBuffer->nextTraceLine)) == traceBuffer->numLines)
    {
        cur = 0;
        traceDump(traceBuffer, NULL);
    }

    char *startline = &(traceBuffer->traceBuffer[cur * traceBuffer->lineLength]);
    char *curpos = startline;

#if 0
    if (fmt && !(opt&TOPT_NOGLOBAL)) {
        int outlen = (int)strlen(fmt);
        if (outlen > 1 && fmt[outlen-1] == '\n')
            opt |= trcOpt;
    } else {
        opt &= ~TOPT_NOGLOBAL;
    }
#endif


#ifdef ALLOW_TIMESTAMP
    /*
     * Check for crossing hour or updated localtime.
     * It is possible if not very likely that the timezone will change
     * either at a DST boundary, or when the timezone is modified.  We
     * check every hour even though this is not very common.
     */
    ism_time_t now = ism_common_currentTimeNanos();
    int hour = (int)(now / NANOS_IN_HOUR);
    if (hour != traceBuffer->trc_ts_hour)
    {
        ism_common_checkTimezone(traceBuffer->trc_ts);
        traceBuffer->trc_ts_hour = (int)(now / NANOS_IN_HOUR);
    }
    ism_common_setTimestamp(traceBuffer->trc_ts, ism_common_currentTimeNanos());
    ism_common_formatTimestamp(traceBuffer->trc_ts, curpos, 64, 7, ISM_TFF_SPACE|ISM_TFF_ISO8601);
    curpos += strlen(curpos);
#endif

#ifdef ALLOW_THREADNAMES
    if (curpos != startline)
        *curpos++ = ' ';
    curpos += ism_common_getThreadName(curpos, 16);
#endif

#ifdef ALLOW_SOURCELINES
    const char * fp;
    if (curpos != startline)
        *curpos++ = ' ';
    if (file) {
        fp = (char *)file + strlen(file);
        while (fp > file && fp[-1] != '/' && fp[-1] != '\\')
            fp--;
    } else {
        fp = "";
    }
    curpos += sprintf(curpos, "%s:%u", fp, (unsigned int)lineno);
#endif

    if (curpos != startline) {
        *curpos++ = ':';
        *curpos++ = ' ';
        *curpos = 0;
        inlen = curpos - startline;
    }
    if (fmt)
    {
        va_start(arglist, fmt);
        vsnprintf(curpos, traceBuffer->lineLength-inlen, fmt, arglist);
        startline[traceBuffer->lineLength-2]='\n';
        startline[traceBuffer->lineLength-1]='\0';
        va_end(arglist);
    }

    if (action == dumpTrace) traceDump(traceBuffer, NULL);
}

//prints label then 4 lots of (hexified) 40 chars per line
void ism_insertTraceData(const char * label, int option, const char * file,
                            int lineno, void * vbuf, int buflen, int maxlen)
{
    // Only interested if we're between our trace lines
    if (getTraceAction(lineno, file, NULL) == noTrace) return;

    ism_insertTrace(2, option, file, lineno, "%s", label);

    char *sourcebufpos = (char *)vbuf;
    static const char * hexdigit = "0123456789abcdef";

    uint32_t i;
    int len = (buflen < maxlen) ? buflen : maxlen;

    for (i = 0; i < 4; i++) {
        char buf[82];
        uint32_t j;
        char *curtargetpos = buf;

        for (j = 0; j < 40; j++) {
            if ((sourcebufpos - (char *)vbuf) > len)break;

            *curtargetpos++ = hexdigit[((*sourcebufpos)>>4)&0x0f];
            *curtargetpos++ = hexdigit[(*sourcebufpos) & 0x0f];
            sourcebufpos++;
        }
        *curtargetpos++ = '\n';
        *curtargetpos++ ='\0';
        ism_insertTrace(2, option, file, lineno, "%s",buf);
    }
}


/*
 * Dump a trace buffer to the specified file... if filename is NULL we invent one.
 */
void traceDump(traceBuffer_t *buffer, char *filename)
{
    FILE *fp = NULL;
    if (filename == NULL) {
        if (traceFilename[0] == '\0') {
            sprintf(traceFilename, "%s/seltrace.%d.%u", DUMPPATH, (int)getpid(), buffer->traceId);
        }
        filename = traceFilename;
    }

    fp=fopen(filename, "a");

    // Couldn't open the required file, try with a fixed default file
    if (fp == NULL)
    {
        int origErrno = errno;

        fp = fopen(DUMPPATH "seltrace.default", "a");

        if (fp)
        {
            fprintf(fp, "*** FAILED TO OPEN '%s' errno=%d\n", filename, origErrno);
        }
        else
        {
            printf("*** FAILED TO OPEN '%s' errno=%d\n", filename, origErrno);
        }
    }

    if (fp) {
        int endline = buffer->nextTraceLine;
        char *bufferPos = buffer->traceBuffer;
        int traceline = 0;
        int bytes = 0;

        while(traceline != endline) {
            if (*bufferPos)
            {
                bytes += fprintf(fp, "%.*s", buffer->lineLength, bufferPos);
                *bufferPos = '\0';
            }
            bufferPos += buffer->lineLength;
            traceline++;
        };

        if (bytes != 0) {
            fprintf(fp, "====================================== %lu\n", time(NULL));
        }

        fclose(fp);

        buffer->nextTraceLine = 0;
    }
    else {
        buffer->nextTraceLine = 0;
    }
}

traceBuffer_t *allocateTraceBuffer(uint64_t numLines, uint32_t lineLength)
{
    traceBuffer_t *newBuffer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_engine_misc,76),1, sizeof(traceBuffer_t) + (numLines * lineLength));

    if (newBuffer != NULL && pthread_spin_lock(&allLock) == 0)
    {
        #ifdef ALLOW_TIMESTAMP
        newBuffer->trc_ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
        newBuffer->trc_ts_hour = ism_common_currentTimeNanos() / NANOS_IN_HOUR;
        #endif
        newBuffer->numLines = numLines;
        newBuffer->lineLength = lineLength;
        newBuffer->nextTraceLine = 0;
        newBuffer->traceId = allId++;
        newBuffer->next = allBuffers;
        allBuffers = newBuffer;

        pthread_spin_unlock(&allLock);
    }

    return newBuffer;
}
