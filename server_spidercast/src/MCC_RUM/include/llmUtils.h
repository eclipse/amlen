/*
 * Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
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
#ifndef LLM_UTILS_H
#define LLM_UTILS_H

#include <stdio.h>
#include <inttypes.h>

#ifndef LLM_API
    #ifdef _WIN32
        #define LLM_API extern __declspec(dllexport)
    #else
        #define LLM_API extern
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open a file with a utf-8 encoded file name.
 * This is used to open a file which has been specified in an XML document.
 * This function converts the file name as necessary and then opens it.
 * @param fname The file name encoded in utf-8
 * @param mode  The file open mode
 * @param err   The error code
 * @return An open FILE structure or NULL to indicate an error
 */

LLM_API FILE* llm_fopen(const char * fname, const char * mode, int *err);

/**
 * Converts stream id to string
 * @param sid The stream id
 * @param buff  output buffer (will be allocated if NULL)
 * @return An output buffer
 */
LLM_API char* llm_sid2str(uint64_t sid, char * buff);

/**
 * Converts string to stream id
 * @param sid The stream id
 * @return stream id
 */
LLM_API uint64_t llm_str2sid(const char *buff);

/**
 * Retrieves the local machines temp directory
 * @param dest buffer to hold temp directory path
 * @param len size of input buffer, returns number of characters
 *        in temp directory path
 * @return 0 for SUCCESS
 */
LLM_API int llm_tempdir(char *dest, size_t *len);

LLM_API char* llm_trim(const char *src);

#ifdef __cplusplus
}
#endif

#endif
