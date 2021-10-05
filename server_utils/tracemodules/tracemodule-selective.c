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
//In order to compile this trace module: copy to server_utils/src then add to server_utils/Makefile:
//
//libtracemodule-selective-FILES = tracemodule-selective.c
//
//LIB-TARGETS += $(LIBDIR)/libtracemodule-selective$(SO)
//$(LIBDIR)/libtracemodule-selective$(SO): $(call objects, $(libtracemodule-selective-FILES))
//	echo label=$(BUILD_LABEL) id=$(BUILD_ID) version=$(ISM_VERSION) FIPS_BUILD=$(FIPS_BUILD) CC=$(CC) FIPSLD_CC=$(FIPSLD_CC)
//	$(call make-c-library)
//
//DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libtracemodule-selective$(SO)
//$(DEBUG_LIBDIR)/libtracemodule-selective$(SO): $(call debug-objects, $(libtracemodule-selective-FILES))
//	$(call debug-make-c-library)
//
//COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libtracemodule-selective$(SO)
//$(COVERAGE_LIBDIR)/libtracemodule-selective$(SO): $(call coverage-objects, $(libtracemodule-selective-FILES))
//	$(call coverage-make-c-library)
//
// To have more control (e.g. which libraries are linked):
//gcc -c -o tracemodule-selective.o tracemodule-selective.c -Wall -Wold-style-definition   -fPIC -rdynamic -D_GNU_SOURCE -std=gnu99  -O3 -g -DNDEBUG    -I$BROOT/server_ship/include -I. -DISMDATE=`date +%Y-%m-%d` -DISMTIME=`date +%H:%M` -DBUILD_LABEL= -DBUILD_ID= -DISM_VERSION=""
//gcc -pthread -Wl,--no-as-needed -Wl,--no-warn-search-mismatch  -o libtracemodule-selective.so -O3 -g -DNDEBUG  -Wall -Wold-style-definition  -shared -shared-libgcc  -Xlinker -Map -Xlinker libtracemodule-selective.so.map  -Wl,-soname,libtracemodule-selective.so -Wl,--enable-new-dtags tracemodule-selective.o  -lrt -lm  -ldl

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
    addLine,      // Add this line to the trace, but don't dump
    dumpTrace     // Add this line to the trace and dump
} traceAction_t;

//Path to write dumps to
#define DUMPPATH "/ima/logs/"

#define TRACEBUFFER_STRUCTID "SMTB" //Shared Mem Trace Buffer

typedef struct tag_traceBuffer_t {
	char structID[4];
    uint32_t lineLength;
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

// Function signatures that change the state of the trace module
typedef struct tag_funcSpec_t {
    int lineno;
    const char *source;
} funcSpec_t;

// Trace lines that activate tracing
const funcSpec_t activateFunc[]   = { {684,  "engine.c"},                 // >>> ism_engine_destroyClientState TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hClient %p, options %u)\n", __func__, hClient, options);
                                      {4143, "engine.c" },                // >>> ism_engine_destroyConsumer TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hConsumer %p)\n", __func__, hConsumer);
                                      {1796, "topicTreeSubscriptions.c"}, // >>> iett_releaseSubscription TRACE(ENGINE_FNC_TRACE, FUNCTION_ENTRY "subscription=%p, internalAttrs=0x%08x, fInline=%s\n", __func__, subscription, subscription->internalAttrs, fInline ? "true":"false");
                                      {0,    NULL} };                     // <SENTINEL>
// Trace lines that deactivate tracing
const funcSpec_t deactivateFunc[] = { {768,  "engine.c"},                 // <<< ism_engine_destroyClientState TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
                                      {4261, "engine.c"},                 // <<< ism_engine_destroyConsumer TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
                                      {1813, "topicTreeSubscriptions.c"}, // <<< iett_releaseSubscription TRACE(ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d (useCount=%u)\n", __func__, rc, oldCount-1);
                                      {0,    NULL} };                     // <SENTINEL>
// Individual lines of interest (perhaps that happen asynchronously)
const funcSpec_t individualFunc[] = { {4334, "engine.c"},                 // finishDestroyConsumer TRACE(ENGINE_HIGH_TRACE, "Finishing destroy for consumer %p (inline = %d)\n", pConsumer, (int)fInlineDestroy);
                                      {1889, "topicTreeSubscriptions.c"}, // TRACE(ENGINE_FNC_TRACE, FUNCTION_IDENT "consumer=%p, subscription=%p\n", __func__, consumer, subscription);
                                      {4454, "engine.c"},                 // TRACE(ENGINE_HIGH_TRACE, "Deallocating consumer %p from %s\n", pConsumer, __func__);
                                      {0,    NULL} };                     // <SENTINEL>
// A single function that is used to round up tracing
const funcSpec_t roundupFunc =        {493,  "engine.c" };                // <<< ism_engine_term TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

char *fixedFilename = "/ima/logs/selectiveTrace"; // NULL for separate file names by pid&tid&time

uint64_t numLines = 10000;
uint32_t lineLength = 300;

traceBuffer_t *allocateTraceBuffer(uint64_t numLines, uint32_t lineLength);
void traceDump(traceBuffer_t *buffer, char *filename);

int ism_initTraceModule(  ism_prop_t *props
		                , char *errorBuffer
		                , int errorBuffSize
		                , int *trclevel )
{
    int rc = pthread_spin_init(&allLock, PTHREAD_PROCESS_PRIVATE);

    // Force the highest trace level
    *trclevel = 9;

	return rc;
}

//We don't override the set error routines for the moment, we let their output come out in trace
//   We could cause a trace dump on particular errors!

//Optional, called whenever properties are updated...
void ism_insertCfgUpdate(ism_prop_t *props)
{
    // Do nothing
}

// Should we be increasing or decreasing the trace activation level?
traceAction_t getTraceAction(int lineno, const char *file)
{
    traceAction_t action = noTrace;

    // Activate Trace
    const funcSpec_t *checkFunc = activateFunc;

    while(checkFunc->source != NULL)
    {
        if (lineno == checkFunc->lineno && strstr(file, checkFunc->source))
        {
            traceActive++;

            if (traceActive == 1 && traceBuffer == NULL)
            {
                traceBuffer = allocateTraceBuffer(numLines, lineLength);
            }

            goto mod_exit;
        }

        checkFunc++;
    }

    // Deactivate Trace
    checkFunc = deactivateFunc;

    while(checkFunc->source != NULL)
    {
        if (lineno == checkFunc->lineno && strstr(file, checkFunc->source))
        {
            traceActive--;

            if (traceActive == 0)
            {
                action = dumpTrace;
            }

            goto mod_exit;
        }

        checkFunc++;
    }

    // Trace individual lines (e.g. async events)
    if (traceActive == 0)
    {
        checkFunc = individualFunc;

        while(checkFunc->source != NULL)
        {
            if (lineno == checkFunc->lineno && strstr(file, checkFunc->source))
            {
                if (traceBuffer == NULL) traceBuffer = allocateTraceBuffer(numLines, lineLength);
                if (traceBuffer != NULL) action = dumpTrace;
                goto mod_exit;
            }

            checkFunc++;
        }
    }

    // Round up all active trace information
    if (roundupFunc.source != NULL && lineno == roundupFunc.lineno && strstr(file, roundupFunc.source))
    {
        if (pthread_spin_lock(&allLock) == 0)
        {
            traceBuffer_t *thisBuffer = allBuffers;

            while(thisBuffer)
            {
                traceDump(thisBuffer, fixedFilename);
                thisBuffer = thisBuffer->next;
            }

            pthread_spin_unlock(&allLock);
        }
    }

mod_exit:

    if (traceActive != 0 && traceBuffer != NULL) action = addLine;

    return action;
}

XAPI void ism_insertTrace(int level, int opt, const char * file, int lineno, const char * fmt, ...)
{
    va_list  arglist;
    int cur;
    int inlen  = 0;
    traceAction_t action = getTraceAction(lineno, file);

    // Only interested if we're between our trace lines

    if (action == noTrace) return;

    cur = traceBuffer->nextTraceLine;

    if ((++(traceBuffer->nextTraceLine)) == traceBuffer->numLines)
    {
        cur = 0;
        traceDump(traceBuffer, fixedFilename);
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
    curpos += sprintf(curpos, "%s:%u", fp, lineno);
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

    if (action == dumpTrace) traceDump(traceBuffer, fixedFilename);
}

//prints label then 4 lots of (hexified) 40 chars per line
void ism_insertTraceData(const char * label, int option, const char * file,
                            int lineno, void * vbuf, int buflen, int maxlen)
{
    // Only interested if we're between our trace lines
    if (getTraceAction(lineno, file) == noTrace) return;

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
            sprintf(traceFilename, "%s/shmtracemodule.%d.%ld.%lu", DUMPPATH, (int)getpid(), syscall(SYS_gettid), time(NULL));
            filename = traceFilename;
        }
    }

    fp=fopen(filename, "a");

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
}

traceBuffer_t *allocateTraceBuffer(uint64_t numLines, uint32_t lineLength)
{
    traceBuffer_t *newBuffer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,2),1, sizeof(traceBuffer_t) + (numLines * lineLength));

    if (newBuffer != NULL && pthread_spin_lock(&allLock) == 0)
    {
        memcpy(newBuffer->structID, TRACEBUFFER_STRUCTID, 4);
        #ifdef ALLOW_TIMESTAMP
        newBuffer->trc_ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
        newBuffer->trc_ts_hour = ism_common_currentTimeNanos() / NANOS_IN_HOUR;
        #endif
        newBuffer->numLines = numLines;
        newBuffer->lineLength = lineLength;
        newBuffer->nextTraceLine = 0;
        newBuffer->next = allBuffers;
        allBuffers = newBuffer;

        pthread_spin_unlock(&allLock);
    }

    return newBuffer;
}
