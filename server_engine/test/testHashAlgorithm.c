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

/*********************************************************************/
/*                                                                   */
/* Module Name: testHashAlgorithm.c                                  */
/*                                                                   */
/* Description: Test hash algorithms for store handles               */
/*                                                                   */
/*********************************************************************/
#include <sys/stat.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>

#include "engine.h"
#include "engineInternal.h"
#include "ha.h"
#include "admin.h"
#include "adminHA.h"
#include "test_utils_initterm.h"
#include "test_utils_ismlogs.h"
#include "test_utils_file.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"

void *libismprotocol = NULL;
void *libismmonitoring = NULL;

static uint64_t test_chainCounts0[] = { 389, 3079, 24593, 196613, 1572869,  6291469, 201326611, 0};
//static uint64_t test_chainCounts0[] = { 389, 3079, 24593, 196613, 1572869, 12582917,  50331653, 201326611, 0};
static uint64_t test_chainCounts1[] = { 256, 2048, 16384, 131072, 1048576,  8388608, 41331653, 201326611, 0};

typedef uint64_t(*testHash_Function_t)(uint64_t key,
                                       uint64_t chainCount);

uint64_t hashFunc_showKey(uint64_t key, uint64_t chainCount)
{
    printf("%016lx\n", key);
    return key%chainCount;
}

uint64_t hashFunc_simpleMod(uint64_t key, uint64_t chainCount)
{
    return key%chainCount;
}

uint64_t hashFunc_simpleMod2(uint64_t key, uint64_t chainCount)
{
    return (key<<1)%chainCount;
}

uint64_t hashFunc_Mix(uint64_t key, uint64_t numChains)
{
  key = (~key) + (key << 21);
  key = key ^ (key >> 24);
  key = (key + (key << 3)) + (key << 8);
  key = key ^ (key >> 14);
  key = (key + (key << 2)) + (key << 4);
  key = key ^ (key >> 28);
  key = key + (key << 31);
  return key % numChains;
}

uint64_t hashFunc_Mix2(uint64_t key, uint64_t numChains)
{
    key = (~key) + ((int64_t)key << 21);
    key = key ^ (key >> 24);
    key = (key + ((int64_t)key << 3)) + ((int64_t)key << 8);
    key = key ^ (key >> 14);
    key = (key + ((int64_t)key << 2)) + ((int64_t)key << 4);
    key = key ^ (key >> 28);
    key = key + ((int64_t)key << 31);
    return key % numChains;
}

uint64_t hashFunc_Flail(uint64_t key, uint64_t numChains)
{
    uint64_t calcVal = ((key & 0xffff000000000000)>>48) ^ ((key & 0x0000ffffffffffff)>>2);
    //printf("%016lx %016lx\n", key, calcVal);
    return calcVal % numChains;
}

uint64_t hashFunc_Flail2(uint64_t key, uint64_t numChains)
{
    uint64_t calcVal = (key ^ key <<16) & 0xffffffffff0000;
    //printf("%016lx %016lx\n", key, calcVal);
    return calcVal % numChains;
}

uint64_t hashFunc_Switch(uint64_t key, uint64_t numChains)
{
    return (key ^ (key<<16)) % numChains;
}

uint64_t hashFunc_FNV(uint64_t key, uint64_t numChains)
{
  uint64_t hash = 14695981039346656037ULL;
  uint8_t *pb = (uint8_t *)&key;

  for(int32_t i=0; i<sizeof(uint64_t); i++)
  {
      hash = hash ^ pb[i];
      hash = hash * 1099511628211ULL;
  }

  return hash % numChains;
}

//****************************************************************************
/// @brief  Timed functions...
//****************************************************************************
#define BUFFER_SIZE 1000000
#define NUM_CHAINLENGTHBOUNDARIES 14
void testHash(testHash_Function_t hashFunc,
              FILE *keyFile,
              int32_t startChain,
              uint64_t *test_chainCounts,
              bool bShowChains)
{
    uint64_t *buffer = malloc(sizeof(uint64_t) * BUFFER_SIZE);
    uint8_t *chainLengths = NULL;

    if (hashFunc == NULL || keyFile == NULL || buffer == NULL)
    {
        goto mod_exit;
    }

    // Try with different table sizes
    for(int32_t i=startChain; test_chainCounts[i] != 0; i++)
    {
        double totalTime = 0;
        uint64_t chainCount = test_chainCounts[i];

        chainLengths = realloc(chainLengths, sizeof(chainLengths[0])*chainCount);

        if (chainLengths == NULL)
        {
            printf("failed to allocate chainLengths array for %lu count\n", chainCount);
            goto mod_exit;
        }

        memset(chainLengths, 0, sizeof(chainLengths[0])*chainCount);

        printf("Testing with %lu chains\n", chainCount);

        fseek(keyFile, 0, SEEK_SET);

        size_t keysRead;
        uint64_t chainNum;
        while((keysRead = fread(buffer, sizeof(uint64_t), BUFFER_SIZE, keyFile)) != 0)
        {
            for(size_t keyNum=0; keyNum<keysRead; keyNum++)
            {
                double t = ism_common_readTSC();

                chainNum = hashFunc(buffer[keyNum], chainCount);

                totalTime += ism_common_readTSC()-t;

                chainLengths[chainNum]++;
            }
        }

        printf("totalTime = %.2f\n", totalTime);

        uint32_t chainLengthBoundaries[NUM_CHAINLENGTHBOUNDARIES]={1,2,3,4,5,6,7,8,9,10,20,30,50,1000};
        uint32_t chainSizeCounts[NUM_CHAINLENGTHBOUNDARIES+1]={0};

        for(chainNum = 0; chainNum < chainCount; chainNum++)
        {
            int32_t x=0;
            for(x=0; x< NUM_CHAINLENGTHBOUNDARIES; x++)
            {
                if(chainLengths[chainNum] < chainLengthBoundaries[x])
                {
                    chainSizeCounts[x]++;
                    break;
                }
            }
            if(x == NUM_CHAINLENGTHBOUNDARIES)
            {
                /* We weren't less than any boundary */
                chainSizeCounts[NUM_CHAINLENGTHBOUNDARIES]++;
            }
        }

        int32_t x=0;
        for(x=0; x< NUM_CHAINLENGTHBOUNDARIES; x++)
        {
            printf("Num chains of length < %u = %u\n", chainLengthBoundaries[x], chainSizeCounts[x]);
        }
        printf("Number of longer chains is %u\n", chainSizeCounts[NUM_CHAINLENGTHBOUNDARIES]);

        if (bShowChains)
        {
            for(chainNum = 0; chainNum < chainCount; chainNum++)
            {
                printf("%09lu:%d\n", chainNum, chainLengths[chainNum]);
            }
        }
    }

mod_exit:

    if (buffer != NULL) free(buffer);
    return;
}

//****************************************************************************
/// @brief Process Command line arguments
///
/// @param[in]     argc  Count of arguments
/// @param[in]     argv  Array of arguments
///
/// @return The index of the first non-option argument on success, -1 on error.
//****************************************************************************
int processArgs(int argc, char **argv,
                testHash_Function_t *pHashFunc,
                FILE **pKeyFile,
                int32_t *pStartChain,
                uint64_t **pChainCounts,
                bool *pShowChains)
{
    int usage = 0;
    char opt;
    int long_index;

    const char short_options[] = "h:k:s:t:c";

    struct option long_options[] = {
        { "hashFunc", 1, NULL, short_options[0] },
        { "keyFile", 1, NULL, short_options[2] },
        { "startChain", 1, NULL, short_options[4] },
        { "testCounts", 1, NULL, short_options[6] },
        { "showChains", 1, NULL, short_options[8] },
        { NULL, 1, NULL, 0 } };

    // Basic test 10,000 10k messages just put to a queue
    *pHashFunc = hashFunc_showKey;
    *pKeyFile = NULL;
    *pShowChains = false;
    *pChainCounts = test_chainCounts0;
    *pStartChain = 7;

    while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1)
    {
        // Now process options
        switch (opt)
        {
            case 'h':
                if (strcasecmp(optarg, "showKey") == 0) *pHashFunc = hashFunc_showKey;
                else if (strcasecmp(optarg, "simpleMod") == 0) *pHashFunc = hashFunc_simpleMod;
                else if (strcasecmp(optarg, "simpleMod2") == 0) *pHashFunc = hashFunc_simpleMod2;
                else if (strcasecmp(optarg, "mix") == 0) *pHashFunc = hashFunc_Mix;
                else if (strcasecmp(optarg, "mix2") == 0) *pHashFunc = hashFunc_Mix2;
                else if (strcasecmp(optarg, "switch") == 0) *pHashFunc = hashFunc_Switch;
                else if (strcasecmp(optarg, "flail") == 0) *pHashFunc = hashFunc_Flail;
                else if (strcasecmp(optarg, "flail2") == 0) *pHashFunc = hashFunc_Flail2;
                else if (strcasecmp(optarg, "fnv") == 0) *pHashFunc = hashFunc_FNV;
                else usage = 1;
                break;
            case 'k':
                *pKeyFile = fopen(optarg, "r");
                if (*pKeyFile == NULL) usage = 1;
                break;
            case 's':
                *pStartChain = atoi(optarg);
                break;
            case 't':
                switch(atoi(optarg))
                {
                    case 0:
                        *pChainCounts = test_chainCounts0;
                        break;
                    case 1:
                        *pChainCounts = test_chainCounts1;
                        break;
                    default:
                        usage = 1;
                        break;
                }
                break;
            case 'c':
                *pShowChains = true;
                break;
            case '?':
            default:
                usage = 1;
                break;
        }
    }

    if (usage)
    {
        fprintf(stderr, "Usage: %s -h hashFunc -k keyFile [-s startChain] [-c]\n", argv[0]);
        fprintf(stderr, "       -h - Specify the function to use when hashing each key\n");
        fprintf(stderr, "       -k - Specify the key file to use\n");
        fprintf(stderr, "       -s - Specify the chain count to start\n");
        fprintf(stderr, "       -c - Show chains at end of run\n");
    }

    return usage ? -1 : optind;
}

int main(int argc, char *argv[])
{
    int retval;

    testHash_Function_t hashFunc = NULL;
    FILE *keyFile = NULL;
    int32_t startChain = 0;
    uint64_t *chainCounts = test_chainCounts0;
    bool showChains = false;

    retval = processArgs(argc, argv,
                         &hashFunc,
                         &keyFile,
                         &startChain,
                         &chainCounts,
                         &showChains);

    if (retval == -1) goto mod_exit;

    if (hashFunc == NULL || keyFile == NULL)
    {
        retval = -2;
        goto mod_exit;
    }

    ism_common_initUtil();

    testHash(hashFunc, keyFile, startChain, chainCounts, showChains);

mod_exit:

    if (keyFile != NULL) fclose(keyFile);

    return retval;
}
