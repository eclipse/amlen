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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>

#include <hashFunction.h>

uint32_t MCC_PRIMES[MCC_MAX_HASH_VALUES] =
{ 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613,
		393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843, 50331653,
		100663319, 201326611, 402653189, 805306457, 1610612741 };

/* City64 with linear combination providing multiple values */
void mcc_hash_getAllValues_city64_LC(const char *pKey, size_t keyLen, int numValues,
		uint32_t maxValue, uint32_t *pResults)
{
	int i;
	uint64_t h01;

	h01 = CityHash64(pKey, keyLen);

	pResults[0] = (uint32_t) h01;
	pResults[1] = (uint32_t) (h01 >> 32);

	for (i = 2; i < numValues; i++)
	{
		pResults[i] = (pResults[0] + MCC_PRIMES[i] * pResults[1]) % maxValue;
	}

	pResults[0] = pResults[0] % maxValue;
	pResults[1] = pResults[1] % maxValue;
}

/* City double hash providing multiple values */
void mcc_hash_getAllValues_city64_LC_raw(const char *pKey, size_t keyLen,
		int numValues, uint32_t maxValue, uint32_t *pResults)
{
	int i;
	uint64_t h01;

	h01 = CityHash64(pKey, keyLen);

	pResults[0] = (uint32_t) h01;
	pResults[1] = (uint32_t) (h01 >> 32);

	for (i = 2; i < numValues; i++)
	{
		pResults[i] = (pResults[0] + MCC_PRIMES[i] * pResults[1]);
	}
}

/* City hash get multiple values  */
/* The pResults array must be able to include up to 2 additional values */
void mcc_hash_getAllValues_city64_simple(const char *pKey, size_t keyLen,
		int numValues, uint32_t maxValue, uint32_t *pResults)
{
	int i;
	uint64_t hv, phv = MCC_INITIAL_HASH_SEED;

	for (i = 0; i < numValues; i += 2)
	{
		hv = CityHash64WithSeed(pKey, keyLen, phv);
		pResults[i] = ((uint32_t) hv) % maxValue;
		if (i + 1 < numValues) // can save this if we know the array is going to be large enough
			pResults[i + 1] = ((uint32_t) (hv >> 32)) % maxValue;
		phv = hv;
	}
}

/* MurmurHash3 get multiple values  */
void mcc_hash_getAllValues_murmur3_x86_32(const char *pKey, size_t keyLen,
		int numValues, uint32_t maxValue, uint32_t *pResults)
{
	int i;
	uint32_t hv = MCC_INITIAL_HASH_SEED;

	for (i = 0; i < numValues; i++)
	{
		MurmurHash3_x86_32(pKey, keyLen, hv, &pResults[i]);
		hv = pResults[i];
		pResults[i] = pResults[i] % maxValue;
	}
}

/* MurmurHash3 get multiple values  */
/* The pResults array must be able to include up to 4 additional values */
void mcc_hash_getAllValues_murmur3_x64_128(const char *pKey, size_t keyLen,
		int numValues, uint32_t maxValue, uint32_t *pResults)
{
	int i, n;
	uint32_t hv = MCC_INITIAL_HASH_SEED, out[4];

	for (i = 0; i < numValues; i += 4)
	{
		MurmurHash3_x64_128(pKey, keyLen, hv, out);
		hv = out[0];
		n = i;
		pResults[n++] = out[0] % maxValue;
		pResults[n++] = out[1] % maxValue;
		if (n >= numValues)
			break;
		pResults[n++] = out[2] % maxValue;
		pResults[n] = out[3] % maxValue;
	}
}

/* MurmurHash3 get multiple values  */
/* The pResults array must be able to include up to 4 additional values */
void mcc_hash_getAllValues_murmur3_x64_128_raw(const char *pKey, size_t keyLen,
		int numValues, uint32_t maxValue, uint32_t *pResults)
{
	int i;
	uint32_t hv = MCC_INITIAL_HASH_SEED, out[4];

	for (i = 0; i < numValues; i += 4)
	{
		MurmurHash3_x64_128(pKey, keyLen, hv, out);
		pResults[i] = hv = out[0];
		pResults[i + 1] = out[1];
		pResults[i + 2] = out[2];
		pResults[i + 3] = out[3];
	}
}

void mcc_hash_getAllValues_murmur3_x64_128_LC(const char *pKey, size_t keyLen,
		int numValues, uint32_t maxValue, uint32_t *pResults)
{
	int i;
	uint32_t hv = MCC_INITIAL_HASH_SEED, out[4];

	MurmurHash3_x64_128(pKey, keyLen, hv, out);
	pResults[0] = out[0] % maxValue;
	pResults[1] = out[1] % maxValue;
	pResults[2] = out[2] % maxValue;
	pResults[3] = out[3] % maxValue;

	for (i = 4; i < numValues; i++)
	{
		pResults[i] = (out[0] + MCC_PRIMES[i] * out[1]) % maxValue;
	}
}

/*******************************************************************************/
void mcc_hash_getSingleValue_city64_simple(const char *pKey, size_t keyLen,
		uint32_t maxValue, char *pInput, uint32_t *pResult)
{
	uint32_t round;
	uint64_t hv, phv = MCC_INITIAL_HASH_SEED;

	memcpy(&round, pInput, sizeof(uint32_t));
	if (!(round & 0x1))  // round is even
	{
		if (round > 0)
			memcpy(&phv, pInput + sizeof(uint32_t), sizeof(uint32_t));

		hv = CityHash64WithSeed(pKey, keyLen, phv);
		pResult[0] = (uint32_t) hv % maxValue;
		memcpy(pInput + sizeof(uint32_t), &hv, sizeof(uint64_t));
	}
	else
	{
		memcpy(&hv, pInput + sizeof(uint32_t), sizeof(uint64_t));
		pResult[0] = ((uint32_t) (hv >> 32)) % maxValue;
	}
	round++;
	memcpy(pInput, &round, sizeof(uint32_t));
}

void mcc_hash_getSingleValue_murmur3_x86_32(const char *pKey, size_t keyLen,
		uint32_t maxValue, char *pInput, uint32_t *pResult)
{
	uint32_t round, hv = MCC_INITIAL_HASH_SEED;

	memcpy(&round, pInput, sizeof(uint32_t));
	if (round > 0)
	{
		memcpy(&hv, pInput + sizeof(uint32_t), sizeof(uint32_t));
		round++;
	}

	MurmurHash3_x86_32(pKey, keyLen, hv, pResult);
	memcpy(pInput, &round, sizeof(uint32_t));
	memcpy(pInput + sizeof(uint32_t), pResult, sizeof(uint32_t));
	pResult[0] = pResult[0] % maxValue;
}

