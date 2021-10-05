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

#if defined(WIN32)
   #include <windows.h>
#elif defined(AMQ_UNIX)
   #include <time.h>
   #include <sys/time.h>

   #include <sys/types.h>
   #include <unistd.h>
   /*_syscall0(pid_t, gettid)*/   
   #include <pthread.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmqc.h>

#include "cphUtil.h"

/* Static variable that the controlC handlers sets to tell the rest of the program to close down */
extern int cphControlCInvoked;

/*
** Method: cphUtilSleep
**
** sleep for a specified number of milliseconds
**
** Input Parameters: msecs - the number of milliseconds to sleep
**
**
*/
void cphUtilSleep( int mSecs ) {
#if defined(WIN32)
   Sleep( mSecs );
#elif defined(AMQ_UNIX)
	struct timeval tv;
	tv.tv_sec = (int)(mSecs / 1000);
	tv.tv_usec = (mSecs % 1000) * 1000;
	select( 0, NULL, NULL, NULL, &tv );
#endif
}

/*
** Method: cphUtilTimeIni
**
** Initialise a CPH_TIME structure to zero. This structure is implemented differently on Linux and
** Windows. On Windows it is a ULARGE_INTEGER and on Linux a timeval structure.
**
** Input Parameters: pTime - pointer to the CPH_TIME value to be initialised
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphUtilTimeIni(CPH_TIME *pTime) {
#if defined(WIN32)
    pTime->LowPart = 0;
    pTime->HighPart = 0;
#elif defined(AMQ_UNIX)
    pTime->tv_sec = 0;
    pTime->tv_usec = 0;
#endif
    return(CPHTRUE);
}

/*
** Method: cphUtilGetCurrentTimeMillis
**
** Get the current time in milliseconds as a CPH_TIME value. CPH_TIME hides the machine dependencies between
** Windows and Linux.
**
** Returns: The CPH_TIME value corresponding to the current time
**
*/
CPH_TIME cphUtilGetCurrentTimeMillis(void) {
   #if defined(WIN32)
   FILETIME ft;
   ULARGE_INTEGER uLarge;

   /* This gets the time as a FILETIME, a 64-bit value representing the number of 100-nanosecond intervals since 
      January 1, 1601 (UTC). */
   GetSystemTimeAsFileTime(&ft);
   uLarge.LowPart = ft.dwLowDateTime;
   uLarge.HighPart = ft.dwHighDateTime;
   
   /* Convert to milliseconds */
   uLarge.QuadPart /= 10000;

   return uLarge;

   #elif defined(AMQ_UNIX)
   /* On Linux we get the number of seconds and micro seconds since the epoch */
   CPH_TIME atimeval;

   atimeval.tv_sec = 0;
   atimeval.tv_usec = 0;
   if (0 == gettimeofday(&atimeval, NULL)) {
       /* Convert the microseconds struct member to milliseconds */
       atimeval.tv_usec /= 1000;
   }
   return atimeval;
   #endif
}

/*
** Method: cphUtilGetTimeDifference
**
** Calculate the difference between two CPH_TIME values
**
** Input Parameters: time1 - the first CPH_TIME to be compared
**                   time2 - the second CPH_TIME to be compared
**
** Returns: a long value representing the first time less the second time
**
*/
long cphUtilGetTimeDifference(CPH_TIME time1, CPH_TIME time2) {
   CPH_TIME uDif;

#if defined(WIN32)
   uDif.QuadPart = time1.QuadPart - time2.QuadPart;
   return (long) uDif.QuadPart;
#elif defined(AMQ_UNIX)
   uDif.tv_sec = time1.tv_sec - time2.tv_sec;
   uDif.tv_usec = time1.tv_usec - time2.tv_usec;
   if (uDif.tv_sec > 0 && uDif.tv_usec < 0) {
       uDif.tv_sec--;
       uDif.tv_usec += 1000;

   }
   if (uDif.tv_sec < 0 && uDif.tv_usec > 0) {
       uDif.tv_sec++;
       uDif.tv_usec -= 1000;
   }
   return (long) ((uDif.tv_sec * 1000) + uDif.tv_usec);
#endif

}

/*
** Method: cphUtilTimeCompare
**
** Compare two CPH_TIMES and return an integer value set to -1, 0, or +1 depending on
** whether the first time is less than, equal two, or greater than the second time.
**
** Input Parameters: time1 - the first CPH_TIME to be compared
**                   time2 - the second CPH_TIME to be compared
**
** Returns:
**      -1 if pTime1 < pTime2
**       0  if pTime1 = pTime2
**       1  if pTime1 > pTime2
*/
int cphUtilTimeCompare(CPH_TIME time1, CPH_TIME time2) {
   int result=0;

#if defined(WIN32)
   FILETIME ft1, ft2;

   ft1.dwHighDateTime = time1.HighPart;
   ft1.dwLowDateTime = time1.LowPart;

   ft2.dwHighDateTime = time2.HighPart;
   ft2.dwLowDateTime = time2.LowPart;

   result = CompareFileTime(&ft1, &ft2);
#elif defined(AMQ_UNIX)
   long diffTime;

   diffTime = cphUtilGetTimeDifference(time1, time2);
   if (diffTime < 0) result = -1;
   if (diffTime > 0) result = 1;
#endif

   return result;
}

/*
** Method: cphUtilTimeIsZero
**
** Determine whether a CPH_TIME is at its zero initial state or not
**
** Input Parameters: pTime - the CPH_TIME to be checked
**
** Returns: CPHTRUE if the CPH_TIME is zero and CPHFALSE otherwise
**
*/
int cphUtilTimeIsZero(CPH_TIME pTime) {
#if defined(WIN32)
    if (pTime.HighPart == 0 && pTime.LowPart == 0) return(CPHTRUE);
#elif defined(AMQ_UNIX)
    if (pTime.tv_sec == 0 && pTime.tv_usec == 0) return(CPHTRUE);
#endif
    return(CPHFALSE);
}

/*
** Method: cphUtilGetThreadId
**
** Get the thread id of the calling thread
**
** Returns: the thread number of the calling thread (int)
**
*/
int cphUtilGetThreadId(void) {
    int threadId;
#if defined(WIN32)
    threadId = GetCurrentThreadId();
#elif defined(AMQ_UNIX)
    threadId = (int) pthread_self();
#else
#error "Don't know how to find threadId on this platform"
#endif
    return(threadId);
}

/*
** Method: cphUtilGetProcessId
**
** Get the process id of the calling process
**
** Returns: the process id of the calling process (int)
**
*/
int cphUtilGetProcessId(void) {
    int processId;
#if defined(WIN32)
    processId = GetCurrentProcessId();
#elif defined(AMQ_UNIX)
    processId = (int) getpid();
#endif
    return(processId);
}

/*
** Method: cphUtilSigInt
**
** We register this method to be called (via a call to signal) in the event Control-C is pressed.
** The method sets the static variable cphControlCInvoked to tell the rest of cph to close down
** and re-registers the method to be called again in the event of further control-C requests.
**
** Input Parameters: dummysignum - dummy argument, required
**
*/
/* NB: This method gets invoked by Windows on a different thread */
void cphUtilSigInt(int dummysignum) {
    cphControlCInvoked = CPHTRUE;
    signal(SIGINT, cphUtilSigInt);
}

/*
** Method: cphUtilGetTraceTime
**
** Return a character string representation of the time in the following format: 
   dd_mm_yyyy hh:hh:ss:mmm
**
** This method is used by the trace mechanism.
**
** Output Parameters: chTime - the character string in which the time is to be written
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphUtilGetTraceTime(char *chTime) {
#if defined(WIN32)
   SYSTEMTIME stime;

   GetSystemTime (&stime);

   sprintf(chTime, "%02d_%02d_%04d %02d:%02d:%02d.%03d",  
       stime.wDay, 
       stime.wMonth, 
       stime.wYear, 
       stime.wHour, 
       stime.wMinute, 
       stime.wSecond,
       stime.wMilliseconds);

   return(CPHTRUE);
#elif defined(AMQ_UNIX)
   /* This variable will get the number of seconds and micro seconds since the epoch */
   struct timeval atimeval;

   /* This is used to store the "broken down" time */
   struct tm  brokenDownTime;

   unsigned int milliSecs;

   if (0 == gettimeofday(&atimeval, NULL)) {

      localtime_r(&(atimeval.tv_sec), &brokenDownTime);

      milliSecs = atimeval.tv_usec / 1000;
 
      sprintf(chTime, "%02d_%02d_%04d %02d:%02d:%02d.%03d",  
       brokenDownTime.tm_mday, 
       1 + brokenDownTime.tm_mon, 
       1900 + brokenDownTime.tm_year, 
       brokenDownTime.tm_hour, 
       brokenDownTime.tm_min, 
       brokenDownTime.tm_sec,
       milliSecs);

      return(CPHTRUE);
   }
return(CPHFALSE);
#endif
}

/*
** Method: cphUtilRTrim
**
** Trim a string from the right
**
** Input Parameters: aLine - the character string to be trimmed
**
** Returns: a pointer to the character string to enable trim calls to be nested
**
*/
char *cphUtilRTrim(char *aLine) {
    if (NULL == aLine) return(NULL);
    if (0 == strlen(aLine)) return(aLine);

    while (aLine[strlen(aLine) - 1] == ' ') {
       aLine[strlen(aLine) - 1] = '\0';
    }

    return(aLine);
}

/*
** Method: cphUtilLTrim
**
** Trim a string from the left
**
** Input Parameters: aLine - the character string to be trimmed
**
** Returns: a pointer to the character string to enable trim calls to be nested
**
*/
char *cphUtilLTrim(char *aLine) {
    size_t i=0;
    size_t len=0;

    if (NULL != aLine) {
       if (0 == (len = strlen(aLine))) return(aLine);

       /* find the first non white space character */
       while ( (aLine[i] == ' ') && (i < len) ) i++;

       /* If the whole line was blanks return a NULL string */
       if (i == len) 
           *aLine = '\0';
       /* Otherwise move the non black section of the line up to the beginning and terminate with a null */
       else if (0 < i) {
          memmove(aLine, &aLine[i], len -i);
          aLine[len -i] = '\0';
       }
    }
    return(aLine);
}

/*
** Method: cphUtilTrim
**
** Trim a character string from the right and left by calling cphUtilRTrim and cphUtilLTrim
**
** Input Parameters: aLine - the character string to be trimmed
**
** Returns: a pointer to the character string to enable trim calls to be nested
**
*/
/* Trim a string from the right and the left */
char *cphUtilTrim(char *aLine) {
    return(cphUtilLTrim(cphUtilRTrim(aLine)));
}

/*
** Method: cphUtilStringEndsWith
**
** Scan a given string to see if it ends with another given string
**
** Input Parameters: aString - the string to be checked
**                   ending - the string we are checking for at the end of aString
**
** Returns: CPHTRUE if the first string ends with the second string, CPHFALSE otherwise
**
*/
int cphUtilStringEndsWith(char *aString, char *ending) {
    int result = CPHFALSE;
    char *ptr;

    /* Search for the sub-string */
    ptr = strstr(aString, ending);

    /* If strstr returned aString then the seach string was empty
       If strstr returned NULL then the seach string was not found 
    */
    if ( (aString != ptr) && (NULL != ptr) ) {
        /* Was the seach string at the end of the string ? */
        if (strlen(aString) == (ptr - aString) + strlen(ending))
            result = CPHTRUE;

    }
    return(result);
}

/*
** Method: cphUtilstrcrlf
**
** Replace all literal "\n" characters in a string with a binary \n
**
** Input Parameters: aString - a pointer to the string to be edited
**
** Returns: the pointer to the character string for calling convenience
**
*/
char *cphUtilstrcrlf(char *aString) {
    char *res;

    res = strstr(aString, "\\n");

    while ( (NULL != res) && (res != aString) ) {
       *res = ' ';
       *(res+1) = '\n';
       res = strstr(aString, "\\n");
    }
    return(aString);

}

/*
** Method: cphUtilstrcrlfTotabcrlf
**
** Replace all literal "\n" characters in a string with a binary CRLF and tab
**
** Input Parameters: aString - a pointer to the string to be edited
**
** Returns: the pointer to the character string for calling convenience
**
*/
char *cphUtilstrcrlfTotabcrlf(char *aString) {
    char *res;

    res = strstr(aString, "\\n");

    while ( (NULL != res) && (res != aString) ) {
       *res = '\n';
       *(res+1) = '\t';
       res = strstr(aString, "\\n");
    }
    return(aString);

}

/*
** Method: cphUtilMakeBigString
**
** Function to build a string of a given size. This is used to build the message contents to be published. The
** string is allocated with malloc and needs to be disposed of by the caller.
**
** Input Parameters: size - the length of the string to build 
**
** Returns: a pointer to the built character string
**
*/
char *cphUtilMakeBigString(int size) {

        int i=0;
        char *str = NULL;
        char c = 65;

        /* Actually grab one byte more than requested so we can terminate the string */
        /* NB: This string is deallocated by cphPublisherFree */
        if (NULL != (str = malloc(size + 1))) {
           for (i=0; i < size; i++) {
               str[i] = c++;
               if (c > 122) c = 65;
           }
        }

        /* Null terminate the string for trace etc */
        str[size] = '\0';

        return(str);
    }

/*
** Method: cphUtilMakeBigStringWithRFH2
**
** Function to build a string of a given size, plus an additional RFH2.
** This is used to build the message contents to be published. The
** string is allocated with malloc and needs to be disposed of by the caller.
**
** Input Parameters: size - the length of the string to build 
**
** Returns: a pointer to the built character string
**
*/
char *cphUtilMakeBigStringWithRFH2(int size, size_t *pRfh2Len) {

    int i=0;
    char *str = NULL;
    char c = 65;
    char *ptr = NULL;
    MQRFH2 rfh2 = {MQRFH2_DEFAULT};
    MQLONG rfh2Len;

#define OLD_RFH2_NAME_VALUE_DATA_LENGTH 76
#define RFH2_NAME_VALUE_DATA_LENGTH_1 32
#define RFH2_NAME_VALUE_DATA_LENGTH_2 144

    rfh2Len = sizeof(MQRFH2) + sizeof(MQLONG) + RFH2_NAME_VALUE_DATA_LENGTH_1 + sizeof(MQLONG)+ RFH2_NAME_VALUE_DATA_LENGTH_2;
    rfh2.StrucLength = rfh2Len;
    rfh2.CodedCharSetId = 1208;
    memcpy(rfh2.Format, "MQSTR    ", MQ_FORMAT_LENGTH);
    rfh2.NameValueCCSID = 1208;

    /* Actually grab one byte more than requested so we can terminate the string */
    /* NB: This string is deallocated by cphPublisherFree */
    if (NULL != (ptr = malloc(size + 1 + rfh2Len))) {
       *pRfh2Len = rfh2Len;
       str = ptr;
       memcpy(str, &rfh2, sizeof(MQRFH2));
       str += sizeof(MQRFH2);
       *((MQLONG *)str) = RFH2_NAME_VALUE_DATA_LENGTH_1;
       str += sizeof(MQLONG);
/*           memcpy(str, "<psc><Command>Publish</Command><Topic>MQJMS/PSIVT/Information</Topic></psc> ", RFH2_NAME_VALUE_DATA_LENGTH);*/
       memcpy(str, "<mcd><Msd>jms_text</Msd></mcd>  ", RFH2_NAME_VALUE_DATA_LENGTH_1);
       str += RFH2_NAME_VALUE_DATA_LENGTH_1;
       *((MQLONG *)str) = RFH2_NAME_VALUE_DATA_LENGTH_2;
       str += sizeof(MQLONG);
       memcpy(str, "<jms><Dst>topic://TOPIC1</Dst><Tms>1207047258454</Tms><Dlv>1</Dlv><Uci dt='bin.hex'>414D51435A4D53504552463420202020CDF7F147011B0020</Uci></jms>", RFH2_NAME_VALUE_DATA_LENGTH_2);
       str += RFH2_NAME_VALUE_DATA_LENGTH_2;
       for (i=0; i < size; i++) {
           str[i] = c++;
           if (c > 122) c = 65;
       }
    }

    /* Null terminate the string for trace etc */
    str[size] = '\0';

    return(ptr);
}

/*
** Method: cphUtilMakeBigStringWithRFH2
**
** Function to build a string of a given size, plus an additional RFH2.
** This is used to build the message contents to be published. The
** string is allocated with malloc and needs to be disposed of by the caller.
**
** Input Parameters: size - the length of the string to build 
**
** Returns: a pointer to the built character string
**
*/
char *cphBuildRFH2(MQLONG *pSize) {

    char *str = NULL;
    char *ptr = NULL;
    MQRFH2 rfh2 = {MQRFH2_DEFAULT};
    MQLONG rfh2Len;

#define RFH2_NAME_VALUE_DATA_LENGTH_1 32
#define RFH2_NAME_VALUE_DATA_LENGTH_2 144

    rfh2Len = sizeof(MQRFH2) + sizeof(MQLONG) + RFH2_NAME_VALUE_DATA_LENGTH_1 + sizeof(MQLONG)+ RFH2_NAME_VALUE_DATA_LENGTH_2;
    rfh2.StrucLength = rfh2Len;
    rfh2.CodedCharSetId = 1208;
    memcpy(rfh2.Format, "MQSTR    ", MQ_FORMAT_LENGTH);
    rfh2.NameValueCCSID = 1208;

    if (NULL != (ptr = malloc(rfh2Len))) {
       str = ptr;
       memcpy(str, &rfh2, sizeof(MQRFH2));
       str += sizeof(MQRFH2);
       *((MQLONG *)str) = RFH2_NAME_VALUE_DATA_LENGTH_1;
       str += sizeof(MQLONG);
       memcpy(str, "<mcd><Msd>jms_text</Msd></mcd>  ", RFH2_NAME_VALUE_DATA_LENGTH_1);
       str += RFH2_NAME_VALUE_DATA_LENGTH_1;
       *((MQLONG *)str) = RFH2_NAME_VALUE_DATA_LENGTH_2;
       str += sizeof(MQLONG);
       memcpy(str, "<jms><Dst>topic://TOPIC1</Dst><Tms>1207047258454</Tms><Dlv>1</Dlv><Uci dt='bin.hex'>414D51435A4D53504552463420202020CDF7F147011B0020</Uci></jms>", RFH2_NAME_VALUE_DATA_LENGTH_2);
    }

    *pSize = rfh2Len;
    return(ptr);
}

/*
** Method: cphGetEnv
**
** Returns the value of a specified environment variable
**
** Input Parameters: varName - the name of the environment variable it is required to retrieve
**
** Returns: a pointer to the value of the environment variable or NULL if it is not defined
**
*/
int cphGetEnv(char *varName, char *varValue, size_t buffSize)
{
    int status = CPHTRUE;
#if defined(WIN32)
    size_t requiredSize;

    /* Get the size of the array we need to retrieve the value */
    if (0 != getenv_s( &requiredSize, NULL, 0, varName))
        return(CPHFALSE);

    /* If this size is less than the memory we have, return an error */
    if (buffSize < requiredSize) return(CPHFALSE);

    /* Now get the value of the environment variable into the supplied buffer */
    if (0 != getenv_s(&requiredSize, varValue, buffSize, varName))
        status = CPHFALSE;
#elif defined(AMQ_UNIX)
    char *buff;
    if (NULL != (buff = getenv(varName))) {
        if (buffSize > strlen(buff))
           strcpy(varValue, buff);
        else
           status = CPHFALSE;
    } else {
        status = CPHFALSE;
    }
#endif
    return(status);
}
