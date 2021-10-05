/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
/// @file  engineTraceDump.c
/// @brief Dump inmemory trace to a diagnostic file
//****************************************************************************

#define TRACE_COMP Engine

#include <ctype.h>
#include <sys/stat.h>

#include "engineDiag.h"
#include "engineUtils.h"
#include "engineTraceDump.h"

typedef struct tag_ietdTraceDumpContext_t {
    ieutThreadData_t *pCurrentThread_ThreadData; ///< For the thread running code, not the data we're enumerating
    ieieEncryptedFileHandle_t diagFile;          ///< File we are dumping to
    uint32_t threadsDumped;                      ///< Number of threads we successfully dumped
    uint32_t threadNumSkipped;                   ///< Position in the list of threaddatas of "this" thread (first thread in dump)
    int32_t rc;                                  ///< current error, if any
    bool allowDumpThisThread;                    ///< If the threaddata passed to this function is the same as in this context, do we dump?
                                                 ///                   (If we don't threadNumSkipped is set to the current val of threadsDumped)
} ietdTraceDumpContext_t;


#if (ieutTRACEHISTORY_BUFFERSIZE > 0)
void ietd_dumpTraceHistoryBuf(ieutThreadData_t *pThreadData,
                              void *context)
{
    ietdTraceDumpContext_t *pDumpContext = (ietdTraceDumpContext_t *)context;

    if (pDumpContext->rc != ISMRC_OK)
    {
        //earlier call to this function failed... do nothing.
        goto mod_exit;
    }

    //If we have been passed the current thread, should we dump it
    if (   (pDumpContext->allowDumpThisThread)
        || (pThreadData != pDumpContext->pCurrentThread_ThreadData))
    {
        //Get current buffer position, copy arrays, Get updated position

        uint64_t    copyTraceHistoryIdent[ieutTRACEHISTORY_BUFFERSIZE];
        uint64_t    copyTraceHistoryValue[ieutTRACEHISTORY_BUFFERSIZE];
        uint64_t    startTraceHistoryBufPos;
        uint64_t    endTraceHistoryBufPos;

        startTraceHistoryBufPos = (volatile uint32_t)pThreadData->traceHistoryBufPos;
        memcpy(copyTraceHistoryIdent,pThreadData->traceHistoryIdent, ieutTRACEHISTORY_BUFFERSIZE*sizeof(uint64_t));
        memcpy(copyTraceHistoryValue,pThreadData->traceHistoryValue, ieutTRACEHISTORY_BUFFERSIZE*sizeof(uint64_t));
        endTraceHistoryBufPos   = (volatile uint32_t)pThreadData->traceHistoryBufPos;

        uint64_t lastTracePoint = ieutTRACEHISTORY_BUFFERSIZE;
        while( lastTracePoint-- > 0 )
        {
            if(copyTraceHistoryIdent[lastTracePoint] != 0)
            {
                //We've found a  valid trace point, dump upto and including this tracepoint
                break;
            }
        }

        ietdTraceDumpThreadHeader_t threadHdr = { (uint64_t)pThreadData
                                                , lastTracePoint+1
                                                , startTraceHistoryBufPos
                                                , endTraceHistoryBufPos};

        // Write this header to the output file
        int32_t rc = ieie_exportData(pDumpContext->pCurrentThread_ThreadData,
                                     pDumpContext->diagFile,
                                     ieieDATATYPE_TRACEDUMPTHREADHEADER,
                                     pDumpContext->threadsDumped,
                                     sizeof(threadHdr),
                                     &threadHdr);

        if (rc != OK)
        {
            pDumpContext->rc = rc;
            ism_common_setError(rc);
            goto mod_exit;
        }

        if (threadHdr.numTracePoints > 0)
        {
            // Write trace point locations to the file
            rc = ieie_exportData(pDumpContext->pCurrentThread_ThreadData,
                                 pDumpContext->diagFile,
                                 ieieDATATYPE_TRACEDUMPTHREADIDENTS,
                                 pDumpContext->threadsDumped,
                                 threadHdr.numTracePoints*sizeof(copyTraceHistoryIdent[0]),
                                 copyTraceHistoryIdent);

            if (rc != OK)
            {
                pDumpContext->rc = rc;
                ism_common_setError(rc);
                goto mod_exit;
            }

            // Write trace point locations to the file
            rc = ieie_exportData(pDumpContext->pCurrentThread_ThreadData,
                                 pDumpContext->diagFile,
                                 ieieDATATYPE_TRACEDUMPTHREADVALUES,
                                 pDumpContext->threadsDumped,
                                 threadHdr.numTracePoints*sizeof(copyTraceHistoryValue[0]),
                                 copyTraceHistoryValue);

            if (rc != OK)
            {
                pDumpContext->rc = rc;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }

        pDumpContext->threadsDumped++;
    }
    else
    {
        pDumpContext->threadNumSkipped = pDumpContext->threadsDumped;
    }
mod_exit:
    return;
}

static int32_t ietd_dumpHistories( ieutThreadData_t *pThreadData
                                 , ieieEncryptedFileHandle_t diagFile
                                 , uint32_t *pThreadsDumped
                                 , uint32_t *pPositionOfFirstThread)
{
    ietdTraceDumpContext_t context = {pThreadData, diagFile, 0, 0, ISMRC_OK, true};

    //Dump thread calling dump first...
    ietd_dumpTraceHistoryBuf(pThreadData, &context);

    // Enumerate all other threads (avoiding ours)
    if (context.rc == ISMRC_OK)
    {
        context.allowDumpThisThread = false;
        ieut_enumerateThreadData(ietd_dumpTraceHistoryBuf, &context);
    }

    *pThreadsDumped = context.threadsDumped;
    *pPositionOfFirstThread = context.threadNumSkipped;

    return context.rc;
}
#else
static int32_t ietd_dumpHistories( ieutThreadData_t *pThreadData
                                 , ieieEncryptedFileHandle_t diagFile
                                 , uint32_t *pThreadsDumped
                                 , uint32_t *pPositionOfFirstThread)
{
    TRACE(ENGINE_FFDC, "Trace histories not enabled\n");
    return ISMRC_NotImplemented;
}
#endif
//****************************************************************************
/// @brief  Create an in-memory trace dump file with the specified name
///
/// @param[in]     fileName       The file to be create (or null to generate one)s
/// @param[in]     password       Password to encrypt file with (or null for "default")
/// @param[out]    RetFilePath    Path to file we wrote if non-null on entry, CALLER MUST FREE
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t ietd_dumpInMemoryTrace( ieutThreadData_t *pThreadData
                              , char *fileName
                              , char *password
                              , char **RetFilePath)
{
    int32_t rc = OK;
    char *filePath = NULL;
    ieieEncryptedFileHandle_t diagFile = NULL;
    char generatedFileName[256];

    ieutTRACEL(pThreadData, fileName, ENGINE_FNC_TRACE, FUNCTION_ENTRY "fileName=%s\n",
        __func__,fileName);

#if (ieutTRACEHISTORY_BUFFERSIZE == 0)
    rc = ISMRC_NotImplemented;
    ism_common_setError(rc);
    goto mod_exit;
#endif

    if (fileName == NULL)
    {
        sprintf( generatedFileName
               ,"inmemtracedump.%u.%lu"
               , ism_common_nowExpire()
               , ism_engine_fastTimeUInt64());

        fileName = generatedFileName;
    }


    if (password == NULL)
    {
        password = "default";
    }

    rc =  edia_createEncryptedDiagnosticFile(pThreadData,
                                             &filePath,
                                             ediaCOMPONENTNAME_ENGINE,
                                             fileName,
                                             password,
                                             &diagFile);

    if (rc != OK)
    {
        diagFile = NULL;
        ism_common_setError(rc);
        goto mod_exit;
    }

    assert(filePath != NULL);
    assert(diagFile != NULL);

    if (RetFilePath != NULL)
    {
        *RetFilePath = filePath;
    }

    ietdTraceDumpFileHeader_t dumpHeader = { IETD_TRACEDUMP_FILEHEADER_STRUCID
                                           , IETD_TRACEDUMP_FILEHEADER_CURRENT_VERSION}; //Other fields, set to 0s

    strncpy(dumpHeader.IMAVersion, ism_common_getVersion(), sizeof(dumpHeader.IMAVersion)-1);
    strncpy(dumpHeader.BuildLabel, ism_common_getBuildLabel(), sizeof(dumpHeader.BuildLabel)-1);
    strncpy(dumpHeader.BuildTime, ism_common_getBuildTime(), sizeof(dumpHeader.BuildTime)-1);

    rc = ieie_exportData(pThreadData,
                         diagFile,
                         ieieDATATYPE_TRACEDUMPHEADER,
                         0,
                         sizeof(dumpHeader),
                         &dumpHeader);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }
    uint32_t threadsDumped = 0;
    uint32_t positionOfFirstThread = 0;

    rc = ietd_dumpHistories( pThreadData
                           , diagFile
                           , &threadsDumped
                           , &positionOfFirstThread);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    ietdTraceDumpFileFooter_t footer = {threadsDumped, positionOfFirstThread};

    rc = ieie_exportData(pThreadData,
                         diagFile,
                         ieieDATATYPE_TRACEDUMPFOOTER,
                         0,
                         sizeof(footer),
                         &footer);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

mod_exit:
    if (filePath != NULL && RetFilePath == NULL) iemem_free(pThreadData, iemem_diagnostics, filePath);
    if (diagFile != NULL) (void)ieie_finishWritingEncryptedFile(pThreadData, diagFile);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n",
                       __func__, rc);

    return rc;
}
