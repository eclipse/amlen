/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

/*********************************************************************/
/*                                                                   */
/* Module Name: hashFunction.c                                       */
/*                                                                   */
/* Description: Hash functions and related utilities                 */
/*                                                                   */
/*********************************************************************/
#ifndef MCP_CLUSTER_HASH_API_DEFINED
#define MCP_CLUSTER_HASH_API_DEFINED

#include "city_c.h"
#include "MurmurHash3.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MCC_MAX_HASH_VALUES   26
#define MCC_INITIAL_HASH_SEED 17
#define MCC_MIN_INPUT_ARRAY_LEN 32



typedef enum
{
   ISM_HASH_TYPE_NONE               = 0, /* An empty Bloom filter                   */
   ISM_HASH_TYPE_CITY64_LC          = 1, /* City 64 bit, with linear combinations   */
   ISM_HASH_TYPE_CITY64_CH         	= 2, /* City 64 bit, seed chaining              */
   ISM_HASH_TYPE_MURMUR_x64_128_LC 	= 3, /* Murmur 64 bit, with linear combinations */
   ISM_HASH_TYPE_MURMUR_x64_128_CH 	= 4  /* Murmur 64 bit, seed chaining            */
} mcc_hash_HashType_t;

/*******************************************************************************/
/* Function template to obtain multiple hash values for a given key            */
/*                                                                             */
/* @param pKey          The key (string) on which to apply the hash            */
/* @param keyLen        Length of the key string                               */
/* @param numValues     Number of values to obtain                             */
/* @param maxValue      Defines the range of the results (0 to maxValue-1)     */
/* @param pResults      An array to hold the results. The array length must be */
/*                      at least numValues                                     */
/*                                                                             */
/*******************************************************************************/
typedef void (*mcc_hash_getAllValues_t)(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);

/*******************************************************************************/
/* Function template to obtain a single hash value for a given key             */
/*                                                                             */
/* @param pKey          The key (string) on which to apply the hash            */
/* @param keyLen        Length of the key string                               */
/* @param maxValue      Defines the range of the results (0 to maxVakue-1)     */
/* @param pInput        An array that holds input parameters for the function. */
/*                      On output the array contains the parameters that       */
/*                      should be used in order to ontain the next value.      */
/*                      On the first round the first 4 bytes should be zero.   */
/*                      The array should be at least MCC_MIN_INPUT_ARRAY_LEN   */
/*                      bytes. Input/output parameter                          */
/* @param pResult       Pointer to where the result should be written          */
/*                                                                             */
/*******************************************************************************/
typedef void (*mcc_hash_getSingleValue_t)(char *pKey, size_t keyLen, uint32_t maxValue, char *pInput, uint32_t *pResult);

void mcc_hash_getAllValues_city64_LC(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);
void mcc_hash_getAllValues_city64_LC_raw(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);
void mcc_hash_getAllValues_city64_simple(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);

void mcc_hash_getAllValues_murmur3_x86_32(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);
void mcc_hash_getAllValues_murmur3_x64_128(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);
void mcc_hash_getAllValues_murmur3_x64_128_raw(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);
void mcc_hash_getAllValues_murmur3_x64_128_LC(const char *pKey, size_t keyLen, int numValues, uint32_t maxValue, uint32_t *pResults);

void mcc_hash_getSingleValue_city64_simple(const char *pKey, size_t keyLen, uint32_t maxValue, char *pInput, uint32_t *pResult);
void mcc_hash_getSingleValue_murmur3_x86_32(const char *pKey, size_t keyLen, uint32_t maxValue, char *pInput, uint32_t *pResult);

#ifdef __cplusplus
}
#endif

#endif /* __CLUSTER_HASH_API_DEFINED */
