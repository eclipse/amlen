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

//*********************************************************************
/// @file  engineDebug.h
/// @brief Internal debug functions for the engine
//*********************************************************************
#ifndef __ISM_ENGINE_DEBUGFUNCS_DEFINED
#define __ISM_ENGINE_DEBUGFUNCS_DEFINED

#ifdef __cplusplus
extern "C" {
#endif


#include <engineInternal.h>

uint32_t iedb_debugOpen( char *tag
                       , uint32_t Options
                       , ismEngine_DebugHdl_t *phDebug );

void iedb_debugClose( ismEngine_DebugHdl_t *phDebug );

void iedb_debugPrintf( ismEngine_DebugHdl_t hDebug
                    , char *format
                    , ... );

bool iedb_isHandleValid(ismEngine_DebugHdl_t hDebug);

uint32_t iedb_debugGetOptions(ismEngine_DebugHdl_t hDebug);

void iedb_debugIndent(ismEngine_DebugHdl_t hDebug );

void iedb_debugOutdent(ismEngine_DebugHdl_t hDebug );


#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_DEBUGFUNCS_DEFINED */

/*********************************************************************/
/* End of engineDebug.h                                              */
/*********************************************************************/
