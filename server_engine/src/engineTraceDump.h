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
/// @file  engineTraceDump.h
/// @brief Dump inmemory trace to a binary encrypted file
//****************************************************************************
#ifndef __ISM_ENGINE_ENGINETRACEDUMP_DEFINED
#define __ISM_ENGINE_ENGINETRACEDUMP_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engineCommon.h"

typedef struct tag_ietdTraceDumpFileHeader_t {
    char StrucId[4]; //TDFH
    uint32_t Version;
    char IMAVersion[40];
    char BuildLabel[40];
    char BuildTime[40];
} ietdTraceDumpFileHeader_t;

#define IETD_TRACEDUMP_FILEHEADER_STRUCID         "TDFH"
#define IETD_TRACEDUMP_FILEHEADER_CURRENT_VERSION 1

typedef struct tag_ietdTraceDumpThreadHeader_t {
    uint64_t pThreadData;    ///<Address in Memory of pThreadData
    uint64_t numTracePoints;
    uint64_t startBufferPos; ///<Buffer position at start of ThreadData Copy
    uint64_t endBufferPos;   ///<Buffer position at end of ThreadData Copy
} ietdTraceDumpThreadHeader_t;

typedef struct tag_ietdTraceDumpFileFooter_t {
    uint32_t threadsDumped;   ///< Num threads dumped
    uint32_t positionOfFirstThread; ///< We dump the thread that caused dump first. This is the number of threads
                                    ///  already written to this file when we would have dumped the threaddata written first
} ietdTraceDumpFileFooter_t;

//****************************************************************************
/// @brief  Create an in-memory trace dump file with the specified name
///
/// @param[in]     fileName       The file to be create (or null to generate one)
/// @param[in]     password       Password to encrypt file with (or null for "default")
/// @param[out]    RetFilePath    Path to file we wrote
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t ietd_dumpInMemoryTrace( ieutThreadData_t *pThreadData
                              , char *fileName
                              , char *password
                              , char **RetFilePath);
#ifdef __cplusplus
}
#endif

#endif
