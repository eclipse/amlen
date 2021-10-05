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

//****************************************************************************
/// @file  test_utils_ismlogs.h
/// @brief Common initialization and termination routines for use by tests
//****************************************************************************
#ifndef __ISM_TEST_UTILS_ISMLOGS_H_DEFINED
#define __ISM_TEST_UTILS_ISMLOGS_H_DEFINED

#include <stdbool.h>
#include <stdint.h>

#if defined(DEFINE_LOGMESSAGE_COUNTERS)
int *ismLogMsgsSeen = NULL;
int  ismLogMsgsSeenCount = 0;
int  ismLogMsgsSeenTotal = 0;
#else
extern int *ismLogMsgsSeen;
extern int  ismLogMsgsSeenCount;
extern int  ismLogMsgsSeenTotal;
#endif

#if defined(COUNT_LOGMESSAGES)
XAPI void ism_common_logInvoke(ism_common_log_context *context, const ISM_LOGLEV level, int msgnum, const char * msgID, int32_t category, ism_trclevel_t * trclvl,
                               const char * func, const char * file, int line,
                               const char * types, const char * fmts, ...)
{
    int32_t oldValue = __sync_fetch_and_add(&ismLogMsgsSeenCount, 1);

    if (oldValue == ismLogMsgsSeenTotal)
    {
        ismLogMsgsSeenTotal += 100;
        // tempIsmLogMsgsSeen is used for storing the temporary return value of realloc, which can be NULL
        int * tempIsmLogMsgsSeen = realloc(ismLogMsgsSeen, ismLogMsgsSeenTotal*sizeof(int));
        if(tempIsmLogMsgsSeen == NULL){
            free(ismLogMsgsSeen);
        }
        ismLogMsgsSeen = tempIsmLogMsgsSeen;
        assert(ismLogMsgsSeen != NULL);
    }

    ismLogMsgsSeen[oldValue] = msgnum;

    return;
}
#endif

//****************************************************************************
/// @brief  Set the directory to which log files should be written.
///
/// @param[in]     logDir       The directory to which log files are being
///                             written.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_setLogDir(char *logDir);

//****************************************************************************
/// @brief  Tell the server that the log file specified should be relocated
///         to the globally overridden config directory /logs.
///
/// @param[in]     logfileType  The type of log file being relocated
/// @param[in]     logLevel     The requested logging level.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_relocateLogFile(char *logfileType, char *logLevel);

//****************************************************************************
/// @brief Find the expected number of instances of text in the specified file
///
/// @param[in]     msgId        The message to find
/// @param[out]    foundCount   The count of instances found
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_findLogMessages(int msgId, int32_t *foundCount);

#endif //end ifndef __ISM_TEST_UTILS_ISMLOGS_H_DEFINED
