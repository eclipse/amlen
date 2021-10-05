/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
/// @file  engineDiag.h
/// @brief Defines structures and functions used for engine advanced diagnostics
//****************************************************************************
#ifndef __ISM_ENGINE_DIAG_DEFINED
#define __ISM_ENGINE_DIAG_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "engineInternal.h"
#include "memHandler.h"
#include "exportCrypto.h"

typedef struct tag_ediaEchoAsyncData_t
{
    char StrucId[4];     ///< Eyecatcher "EDEC"
    char *OutBuf;
    void *pContext;
    size_t contextLength;
    ismEngine_CompletionCallback_t pCallbackFn;
} ediaEchoAsyncData_t;

#define ediaECHOASYNCDIAGNOSTICS_STRUCID "EDEC"

#define ediaVALUE_MODE_ECHO                  "Echo"
#define ediaVALUE_MODE_MEMORYDETAILS         "MemoryDetails"
#define ediaVALUE_MODE_DUMPCLIENTSTATES      "DumpClientStates"
#define ediaVALUE_MODE_OWNERCOUNTS           "OwnerCounts"
#define ediaVALUE_MODE_DUMPTRACEHISTORY      "DumpTraceHistory"
#define ediaVALUE_MODE_SUBDETAILS            "SubDetails"
#define ediaVALUE_MODE_DUMPRESOURCESETS      "DumpResourceSets"
#define ediaVALUE_MODE_RESOURCESETREPORT     "ResourceSetReport"
#define ediaVALUE_MODE_MEMORYTRIM            "MemoryTrim"
#define ediaVALUE_MODE_ASYNCCBSTATS          "AsyncCBStats"

#define ediaVALUE_FILTER_CLIENTID            "ClientId"
#define ediaVALUE_FILTER_SUBNAME             "SubName"

#define ediaVALUE_PARM_FILENAME              "Filename"
#define ediaVALUE_PARM_PASSWORD              "Password"

#define ediaVALUE_OUTPUT_CLIENTOWNERCOUNT    "ClientOwnerCount"
#define ediaVALUE_OUTPUT_SUBOWNERCOUNT       "SubOwnerCount"

#define ediaVALUE_OUTPUT_UNTRACEDARGS        "<UNTRACED>"

#define ediaCOMPONENTNAME_ENGINE             "engine"

int32_t edia_createEncryptedDiagnosticFile(ieutThreadData_t *pThreadData,
                                           char **filePath,
                                           char *componentName,
                                           char *fileName,
                                           char *password,
                                           ieieEncryptedFileHandle_t *diagFile);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_DIAG_DEFINED */
