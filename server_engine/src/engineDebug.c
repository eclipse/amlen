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
/// @file  engineDebug.c
/// @brief Code for engine debug routines
//****************************************************************************
#define TRACE_COMP Engine

#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stdarg.h>

#include "intermediateQ.h"
#include "intermediateQInternal.h"
#include "queueCommon.h"
#include "messageDelivery.h"
#include "engineDebug.h"

typedef struct tag_iedbDebugHandle_t
{
    char StrucId[4];                ///< Eyecatcher "EDBG"
    uint32_t Options;
    uint32_t Indent;
    FILE *fp;
} iedbDebugHandle_t;
#define iedbDEBUG_HANDLE_STRUCID "EDBG"

#define iedbDebugMaxIndent      10
static char iedbDebugBlanks[] = "                                        ";

//****************************************************************************
/// @brief  Open the debug output file
///
/// Opens the debug output file for writing
///
/// @param[in]     tag              Descriptive name
/// @param[out]    phDebug          File handle
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
uint32_t iedb_debugOpen( char *tag
                       , ismEngine_DebugOptions_t Options
                       , ismEngine_DebugHdl_t *phDebug )
{
    uint32_t rc = OK;
    time_t curTime;
    struct tm tm, *ptm;
    char debugFilename[PATH_MAX+1];
    iedbDebugHandle_t *pDebug;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(tag != NULL);

    pDebug=(iedbDebugHandle_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,49),sizeof(iedbDebugHandle_t));
    if (pDebug == NULL)
    {
        goto mod_exit;
    }
    pDebug->Options = Options;
    pDebug->Indent = 0;
    pDebug->fp = NULL;

    if (Options & ismDEBUGOPT_TRACE)
    {
      // Writing to trace
    }
    else if (Options & ismDEBUGOPT_STDOUT)
    {
        pDebug->fp = stdout;
    }
    else
    {
        // Writing to a file, whose name was specified fully
        if (Options & ismDEBUGOPT_NAMEDFILE)
        {
            strcpy(debugFilename, tag);
        }
        else
        {
            curTime = time(NULL);
            ptm = localtime_r(&curTime, &tm);
            if (ptm == NULL)
            {
                rc = ISMRC_Error;
                ism_common_free(ism_memory_engine_misc,pDebug);
                goto mod_exit;
            }

            sprintf(debugFilename, "ismDebug.%d-%d-%d.%02d-%02d-%02d.%s.isd",
                    ptm->tm_year,
                    ptm->tm_mon+1,
                    ptm->tm_mday,
                    ptm->tm_hour,
                    ptm->tm_min,
                    ptm->tm_sec,
                    tag);
        }

        pDebug->fp = fopen(debugFilename, "a");
        if (pDebug->fp == NULL)
        {
            TRACE(ENGINE_ERROR_TRACE, "%s: Failed to open debug file(%s) errno(%d)\n",
                  __func__, debugFilename, errno);
            rc = ISMRC_Error;
            ism_common_free(ism_memory_engine_misc,pDebug);
            goto mod_exit;
        }
    }

    *phDebug = (ismEngine_DebugHdl_t *)pDebug;

mod_exit:
    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief  Close the debug output file
///
/// Unlocks and optionally closes the debug output file for writing
///
/// @param[in,out]   phDebug          Debug handle
//****************************************************************************
void iedb_debugClose( ismEngine_DebugHdl_t *phDebug )
{
    DEBUG_ONLY int osrc;
    iedbDebugHandle_t *pDebug = (iedbDebugHandle_t *)*phDebug;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(pDebug != NULL);

    if ((pDebug->fp != NULL) &&
         (!(pDebug->Options & ismDEBUGOPT_STDOUT)))
    {
        osrc = fclose(pDebug->fp);
        assert(osrc == 0);
        pDebug->fp = NULL;
    }

    ism_common_free(ism_memory_engine_misc,pDebug);

    *phDebug = NULL;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}

//****************************************************************************
/// @brief  Get the debug options from a specified debug handle
///
/// Return the value of the Options field of the specified handle
///
/// @param[in,out]   phDebug          Debug handle
///
/// @returns The value of Options from the handle
//****************************************************************************
uint32_t iedb_debugGetOptions(ismEngine_DebugHdl_t hDebug)
{
    return ((iedbDebugHandle_t *)hDebug)->Options;
}

//****************************************************************************
/// @brief  Prints a line to the debug file
///
/// @param[in]       phDebug          Debug handle
/// @param[in]       format           print format srting
/// @param[in]       ...              variable arguments
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void iedb_debugPrintf( ismEngine_DebugHdl_t hDebug
                     , char *format
                     , ... )
{
    iedbDebugHandle_t *pDebug = (iedbDebugHandle_t *)hDebug;
    uint32_t indent=(pDebug->Indent > iedbDebugMaxIndent)?iedbDebugMaxIndent:pDebug->Indent;
    va_list ap;

    va_start(ap, format);

    if (pDebug->Options & ismDEBUGOPT_TRACE)
    {
        if (ENGINE_FFDC <= TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)])
        {
            char  buf[512];
            size_t len;
            len = snprintf(buf, 510, "%.*s", indent*4, iedbDebugBlanks);

            len += vsnprintf(buf+len, 510-len, format, ap);
            if (len <= 510)
            {
              buf[len]='\n';
              buf[len+1]='\0';
            }
            else
            {
              buf[510]='\n';
              buf[511]='\0';
            }

            TRACE(ENGINE_FFDC, buf);
        }
    }
    else
    {
        assert(pDebug->fp != NULL);

        fprintf(pDebug->fp, "%.*s", indent*4, iedbDebugBlanks);
        vfprintf(pDebug->fp, format, ap);
        fprintf(pDebug->fp, "\n");
    }

    va_end(ap);

    return;
}

//****************************************************************************
/// @brief  Indents printing to a debug handle
///
/// @param[in]       phDebug          Debug handle
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void iedb_debugIndent( ismEngine_DebugHdl_t hDebug )
{
    iedbDebugHandle_t *pDebug = (iedbDebugHandle_t *)hDebug;

    pDebug->Indent++;

    return;
}

//****************************************************************************
/// @brief  Outdents printing to a debug handle
///
/// @param[in]       phDebug          Debug handle
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void iedb_debugOutdent( ismEngine_DebugHdl_t hDebug )
{
    iedbDebugHandle_t *pDebug = (iedbDebugHandle_t *)hDebug;

    assert(pDebug->Indent > 0);

    if (pDebug->Indent > 0)
      pDebug->Indent--;

    return;
}

//****************************************************************************
/// @brief  Outdents printing to a debug handle
///
/// @param[in]       phDebug          Debug handle
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
bool iedb_isHandleValid( ismEngine_DebugHdl_t hDebug )
{
    iedbDebugHandle_t *pDebug = (iedbDebugHandle_t *)hDebug;

   return ((pDebug != NULL) && !memcmp(pDebug->StrucId, iedbDEBUG_HANDLE_STRUCID, 4))?true:false;
}

