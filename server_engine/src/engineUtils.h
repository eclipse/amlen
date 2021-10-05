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
/// @file  engineUtils.h
/// @brief Engine component utilities header file.
//****************************************************************************
#ifndef __ISM_ENGINEUTILS_DEFINED
#define __ISM_ENGINEUTILS_DEFINED

#include "memHandler.h"

// Acquire a reference to the specified thread data
void ieut_acquireThreadDataReference(ieutThreadData_t *pThreadData);

// Release a reference on the specified thread data
void ieut_releaseThreadDataReference(ieutThreadData_t *pThreadData);

// Create a basic thread data structure, which enables tracing
int32_t ieut_createBasicThreadData(void);

// Complete the creation of a thread data structure with full initialization
int32_t ieut_createFullThreadData(ieutThreadData_t *pThreadData);

// Complete the creation of all thread data structures that were created before
// we were able to create full thread data.
void ieut_createFullThreadDataForAllThreads(void);

// Enumerate the global list of thread data structures calling a callback function for each
void ieut_enumerateThreadData(ieutThreadData_EnumCallback_t *callback, void *context);

// Wait for up to x minutes for a uint32_t counter to get to 0
int32_t ieut_waitForRemainingActions(ieutThreadData_t *pThreadData,
                                     volatile uint32_t *remainingActions,
                                     const char *callingFunction,
                                     uint32_t waitForMinutes);

// Wait for a thread to end and initiate server shutdown if it does not end in the specified timeout
void ieut_waitForThread(ieutThreadData_t *pThreadData, ism_threadh_t thread, void ** retvalptr, uint32_t timeout);

// JSON manipulation structures and functions
typedef struct tag_ieutJSONBuffer_t
{
    bool newObject;
    concat_alloc_t buffer;
} ieutJSONBuffer_t;

// Start a new object with optional name: "name": {
void ieut_jsonStartObject(ieutJSONBuffer_t *outputJSON,
                          const char *name);

// End the current object: }
void ieut_jsonEndObject(ieutJSONBuffer_t *outputJSON);

// Start a new array with optional name: "name": [
void ieut_jsonStartArray(ieutJSONBuffer_t *outputJSON,
                         char *name);

// End the current array: ]
void ieut_jsonEndArray(ieutJSONBuffer_t *outputJSON);

// Add a named string to the current buffer: "name":"value"
void ieut_jsonAddString(ieutJSONBuffer_t *outputJSON,
                        char *name,
                        char *value);

// Add a string to the current buffer: "name"
void ieut_jsonAddSimpleString(ieutJSONBuffer_t *outputJSON,
                              char *value);

// Add a named pointer value to the current buffer: "name":"value"
void ieut_jsonAddPointer(ieutJSONBuffer_t *outputJSON,
                         char *name,
                         void *value);

// Add a named int32_t to the current buffer: "name":value
void ieut_jsonAddInt32(ieutJSONBuffer_t *outputJSON,
                       char *name,
                       int32_t value);

// Add a named uint32_t to the current buffer: "name":value
void ieut_jsonAddUInt32(ieutJSONBuffer_t *outputJSON,
                        char *name,
                        uint32_t value);

// Add a named uint32_t to the current buffer in hexadecimal: "name":"0xvalue"
void ieut_jsonAddHexUInt32(ieutJSONBuffer_t *outputJSON,
                           char *name,
                           uint32_t value);

// Add a named int64_t to the current buffer: "name":value
void ieut_jsonAddInt64(ieutJSONBuffer_t *outputJSON,
                       char *name,
                       int64_t value);

// Add a named uint64_t to the current buffer: "name":value
void ieut_jsonAddUInt64(ieutJSONBuffer_t *outputJSON,
                        char *name,
                        uint64_t value);

// Add a named double to the current buffer (2 decimal places): "name":value
void ieut_jsonAddDouble(ieutJSONBuffer_t *outputJSON,
                        char *name,
                        double value);

// Add a named boolean to the current buffer: "name":true|false
void ieut_jsonAddBool(ieutJSONBuffer_t *outputJSON,
                      char *name,
                      bool value);

// Add a named store handle to the current buffer: "name":true|false
void ieut_jsonAddStoreHandle(ieutJSONBuffer_t *outputJSON,
                             char *name,
                             ismStore_Handle_t value);

// Build a null terminated output string with the current buffer,
// this also resets the buffer.
char *ieut_jsonGenerateOutputBuffer(ieutThreadData_t *pThreadData,
                                    ieutJSONBuffer_t *outputJSON,
                                    iemem_memoryType memType);

// Append a null terminator at the end of the current JSON buffer
void ieut_jsonNullTerminateJSONBuffer(ieutJSONBuffer_t *outputJSON);

// Reset the JSON buffer, but don't release the memory
void ieut_jsonResetJSONBuffer(ieutJSONBuffer_t *outputJSON);

// Release the resources being held by the buffer (including memory)
void ieut_jsonReleaseJSONBuffer(ieutJSONBuffer_t *outputJSON);

#endif /* __ISM_ENGINEUTILS_DEFINED */

/*********************************************************************/
/* End of engineUtils.h                                              */
/*********************************************************************/
