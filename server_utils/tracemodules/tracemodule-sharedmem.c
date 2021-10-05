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
#include <fcntl.h>           /* For O_* constants       */
#include <limits.h>          /* file length limits      */

//
//In order to compile this trace module: copy to server_utils/src then add to server_utils/Makefile:
//
//libtracemodule-sharedmem-FILES = tracemodule-sharedmem.c
//
//LIB-TARGETS += $(LIBDIR)/libtracemodule-sharedmem$(SO)
//$(LIBDIR)/libtracemodule-sharedmem$(SO): $(call objects, $(libtracemodule-sharedmem-FILES))
//	echo label=$(BUILD_LABEL) id=$(BUILD_ID) version=$(ISM_VERSION) FIPS_BUILD=$(FIPS_BUILD) CC=$(CC) FIPSLD_CC=$(FIPSLD_CC)
//	$(call make-c-library)
//
//DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libtracemodule-sharedmem$(SO)
//$(DEBUG_LIBDIR)/libtracemodule-sharedmem$(SO): $(call debug-objects, $(libtracemodule-sharedmem-FILES))
//	$(call debug-make-c-library)
//
//COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libtracemodule-sharedmem$(SO)
//$(COVERAGE_LIBDIR)/libtracemodule-sharedmem$(SO): $(call coverage-objects, $(libtracemodule-sharedmem-FILES))
//	$(call coverage-make-c-library)
//
// To have more control (e.g. which libraries are linked):
//gcc -c -o tracemodule-sharedmem.o tracemodule-sharedmem.c -Wall -Wold-style-definition   -fPIC -rdynamic -D_GNU_SOURCE -std=gnu99  -O3 -g -DNDEBUG    -I$BROOT/server_ship/include -I. -DISMDATE=`date +%Y-%m-%d` -DISMTIME=`date +%H:%M` -DBUILD_LABEL= -DBUILD_ID= -DISM_VERSION=""
//gcc -pthread -Wl,--no-as-needed -Wl,--no-warn-search-mismatch  -o libtracemodule-sharedmem.so -O3 -g -DNDEBUG  -Wall -Wold-style-definition  -shared -shared-libgcc  -Xlinker -Map -Xlinker libtracemodule-sharedmem.so.map  -Wl,-soname,libtracemodule-sharedmem.so -Wl,--enable-new-dtags tracemodule-sharedmem.o  -lrt -lm  -ldl



//Defines that control which bits of info this module traces out:
//#define ALLOW_TIMESTAMP     //Uncommenting this line adds a timestamp to each traceline
#define ALLOW_THREADNAMES     //Commenting this disables thread names
//#define ALLOW_SOURCELINES    //Uncommenting this line adds file+line to each traceline




//Add the uid on the name of the shared memory set
#define SHMOBJ_PATH "/ism-shmtracebuffer::%d"

//Path to write dumps to
#define DUMPPATH "/ima/logs/"


#define TRACEBUFFER_STRUCTID "SMTB" //Shared Mem Trace Buffer

typedef struct tag_traceBuffer_t {
	char structID[4];
	uint64_t numLines;
	uint32_t lineLength;
	uint64_t nextTraceLine;
	char traceBuffer[]; //actually numLines * lineLength
} traceBuffer_t ;

traceBuffer_t *traceBuffer;
uint64_t numLines = 90000;
uint32_t lineLength = 120;


traceBuffer_t *connectSharedMem(uint64_t numLines, uint32_t lineLength);
void traceDump(traceBuffer_t *buffer, char *filename);


int ism_initTraceModule(  ism_prop_t *props
		                , char *errorBuffer
		                , int errorBuffSize
		                , int *trclevel )
{
	int rc=0;

	//Initialise error string to empty
	if (errorBuffSize > 0) {
		errorBuffer[0] = '\0';
	}

	traceBuffer_t *newTraceBuffer = connectSharedMem(numLines, lineLength);

	if (newTraceBuffer != NULL) {
		traceBuffer = newTraceBuffer;
	} else{
		snprintf(errorBuffer, errorBuffSize,"Could not initialise trace buffer");
		rc = 10;
	}

	return rc;
}

//We don't override the set error routines for the moment, we let their output come out in trace
//   We could cause a trace dump on particular errors!

//Optional, called whenever properties are updated...
void ism_insertCfgUpdate(ism_prop_t *props)
{
	//Someone is updating config... take it as a signal to dump trace
	traceDump(traceBuffer, NULL);
}




XAPI void ism_insertTrace(int level, int opt, const char * file, int lineno, const char * fmt, ...)
{
    va_list  arglist;
    bool updated;
    uint64_t cur, new;
    int inlen  = 0;

    do
    {
        cur = traceBuffer->nextTraceLine;
        new = (cur == traceBuffer->numLines-1)?0:cur+1;

        updated = __sync_bool_compare_and_swap( &(traceBuffer->nextTraceLine), cur, new);
    } while (!updated);

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
    //if (opt & TOPT_TIME)
    {
        /*
         * Check for crossing hour or updated localtime.
         * It is possible if not very likely that the timezone will change
         * either at a DST boundary, or when the timezone is modified.  We
         * check every hour even though this is not very common.
         */
        ism_time_t now = ism_common_currentTimeNanos();
        int hour = (int)(now / NANOS_IN_HOUR);
        if (hour != trc_ts_hour)
        {
            ism_common_checkTimezone(trc_ts);
            trc_ts_hour = (int)(now / NANOS_IN_HOUR);
        }
        ism_common_setTimestamp(trc_ts, ism_common_currentTimeNanos());
        ism_common_formatTimestamp(trc_ts, curpos, 64, 7, ISM_TFF_SPACE|ISM_TFF_ISO8601);
        curpos += strlen(curpos);
    }
#endif

#ifdef ALLOW_THREADNAMES
    //if (opt & TOPT_THREAD)
    {
        if (curpos != startline)
            *curpos++ = ' ';
        curpos += ism_common_getThreadName(curpos, 16);
    }
#endif

#ifdef ALLOW_SOURCELINES
    //if (opt & TOPT_WHERE)
    {
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
    }
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
}

//prints label then 4 lots of (hexified) 40 chars per line
void ism_insertTraceData(const char * label, int option, const char * file,
                            int lineno, void * vbuf, int buflen, int maxlen)
{
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
    char localfilename[256];
    if (filename == NULL) {
    	sprintf(localfilename, "%s/shmtracemodule.%lu", DUMPPATH, time(NULL));
    	filename = localfilename;
    }

    fp=fopen(filename, "w");

    if (fp) {
        uint64_t startline = buffer->nextTraceLine;
        uint64_t traceline = startline;

        do {
            if (traceline == buffer->numLines) {
                traceline = 0;
            }
            fprintf(fp, "%.*s", buffer->lineLength, buffer->traceBuffer + (traceline * buffer->lineLength));

            traceline++;
        } while(traceline != startline);

        fclose(fp);
    }
}

//Used during setup, we can't trace as if our setup is failing trace is currently not working
void logdirect(char * format, ...)
{
  va_list args;
  FILE *fp;
  char filename[PATH_MAX+1];

  sprintf(filename, "%s/shmtracemodule-setup", DUMPPATH);

  fp=fopen(filename, "a");

  if (fp != NULL) {
	  fprintf(fp, "%lu ", time(NULL));

	  va_start( args, format );
	  vfprintf(fp, format, args);
	  va_end( args );

	  fclose(fp);
  }
}


traceBuffer_t *connectSharedMem(uint64_t numLines, uint32_t lineLength)
{
	//Try and connect to existing shared memory....
	char shmName[NAME_MAX + 1] = "";
	int shmfd;


	traceBuffer_t *ourTraceBuffer = NULL;

	/* reconnect to existing shared memory object    --  shm_open()  */
	sprintf(shmName, SHMOBJ_PATH, (int)getuid());
	shmfd = shm_open(shmName, O_RDWR, S_IRWXU | S_IRWXG);
	if (shmfd < 0)
	{
		logdirect("failed in shm_open() errno %d\n", errno);
	}
	else
	{
		logdirect("reconnected to shared memory object %s\n", shmName);

		//find out current size...
		struct stat statbuf = {0};
		if (fstat(shmfd, &statbuf) != 0) {
			logdirect("failed to fstat shm. errno: %d\n", errno);
			close(shmfd);
		} else if (statbuf.st_size < sizeof(traceBuffer_t)) {
			char buf[sizeof(traceBuffer_t)];

			read(shmfd, buf, statbuf.st_size);
			logdirect("shm had impossible size: %d. shm buf: %.*s\n", statbuf.st_size, sizeof(traceBuffer_t), buf);
			close(shmfd);
		} else {
			/* Size looks sensible   --  mmap() */
			ourTraceBuffer = (traceBuffer_t *)mmap(NULL,statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
			if (ourTraceBuffer == NULL)
			{
				logdirect("In mmap() existing, failed size: %d, errno: %d\n", statbuf.st_size, errno);
			}

			//Ok... we don't need the fd anymore
			close(shmfd);

			if( (ourTraceBuffer != NULL)
					&& (memcmp(ourTraceBuffer->structID, TRACEBUFFER_STRUCTID, 4) == 0)) {
				//Looks like a trace buffer
				traceDump(ourTraceBuffer, NULL);
			}

			if (     (ourTraceBuffer->numLines != numLines)
				  || (ourTraceBuffer->lineLength != lineLength)) {
				//We wanted a different size... unmap it as we'll resize it.

				if (munmap(ourTraceBuffer, statbuf.st_size) != 0) {
					logdirect("In mUNmap(), failed size: %d, errno: %d\n", statbuf.st_size, errno);
				}
				ourTraceBuffer = NULL;
			}
		}
	}

	if (ourTraceBuffer == NULL) {
		//We couldn't find any existing shared mem segment or we (and dumped it) but we disconnected as we wanted to change the size...
	    uint64_t shared_seg_size = sizeof(traceBuffer_t) + (numLines * lineLength);

	    /* creating the shared memory object    --  shm_open()  */
	    sprintf(shmName, SHMOBJ_PATH, (int)getuid());
	    shmfd = shm_open(shmName, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
	    if (shmfd < 0) {
	    	logdirect("Couldn't create shm with shm_open. errno = %d\n", errno);
	    	return NULL;
	    } else {
	    	logdirect("Created shared memory object %s\n", shmName);

	    	/* adjusting mapped file size (make room for the whole segment to map)      --  ftruncate() */
	    	if(ftruncate(shmfd, shared_seg_size) != 0) {
	    		logdirect("Couldn't ftruncate shared memory object %s to be %lu bytes\n", shmName, shared_seg_size);
	    		close(shmfd);
	    		return NULL;
	    	} else {
	    		/* requesting the shared segment    --  mmap() */
	    		ourTraceBuffer = (traceBuffer_t *)mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	    		if (ourTraceBuffer == NULL)
	    		{
	    			logdirect("In mmap() creating new, failed size: %d, errno: %d\n", shared_seg_size, errno);
	    		}
	    		else
	    		{
	    			//Successfully created and mapped new shared mem buffer... initialise it
	    			memcpy(ourTraceBuffer->structID, TRACEBUFFER_STRUCTID, 4);
	    			ourTraceBuffer->numLines = numLines;
	    			ourTraceBuffer->lineLength = lineLength;
	    			ourTraceBuffer->nextTraceLine = 0;
	    		}

	    		//Whether or not the mmap worked we don't need the fd anymore...
	    		close(shmfd);
	    	}
	    }
	}

	return ourTraceBuffer;
}
