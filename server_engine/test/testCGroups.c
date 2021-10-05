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
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/shm.h>
#include <sys/mman.h>
#include <limits.h>
#include <fcntl.h>
#include <ctype.h>

#include <ismutil.h>

ism_domain_t  ism_defaultDomain = {"DOM", 0, "Default" };
ism_trclevel_t * ism_defaultTrace = &ism_defaultDomain.trace;

#include <engineInternal.h>
#include <memHandler.h>

///Stores data about memory cgroup statistics
typedef struct tag_my_iemem_cgroupMemInfo_t {
    uint64_t limitInBytes;
    uint64_t usageInBytes;
    uint64_t accurateUsageInBytes;
    uint64_t freeBytes;
    uint64_t accurateFreeBytes;
    uint64_t cacheBytes;
    uint64_t rssBytes;
    uint64_t swapBytes;
    uint64_t mappedFileBytes;
    uint64_t activeFileBytes;
    uint64_t inactiveFileBytes;
    uint64_t tmpfsBytes; // cache - active_file - inactive_file
} iemem_my_cgroupMemInfo_t;

// To account for shared memory, we ought to include tmpfsBytes. So we have 2 versions of these
// macros, the default one which does make use of tmpfs bytes and another which does not.
#if 1
#define ACC_FREE_INC_BUFFERS(_CGMI) ((_CGMI)->accurateFreeBytes+(_CGMI)->cacheBytes-(_CGMI)->tmpfsBytes)
#define FREE_INC_BUFFERS(_CGMI) ((_CGMI)->freeBytes+(_CGMI)->cacheBytes-(_CGMI)->tmpfsBytes)
#else
#define ACC_FREE_INC_BUFFERS(_CGMI) ((_CGMI)->accurateFreeBytes+(_CGMI)->cacheBytes)
#define FREE_INC_BUFFERS(_CGMI) ((_CGMI)->freeBytes+(_CGMI)->cacheBytes)
#endif

#include "test_utils_assert.h"

bool humanShowDetail = false;

//****************************************************************************
/// @internal
///
/// @brief  Convert a number to a human readable string
///
/// @param[in]      number       The number to convert
/// @param[in,out]  humanNumber  The buffer into which to write the output
//****************************************************************************
const char qualifier[] = {'X', 'K', 'M', 'G', 'T'};
char *makeHuman(uint64_t number, char *humanNumber)
{
    char buffer[50];
    char *p;
    int32_t i;
    uint64_t baseNumber = number;

    for(int32_t n=0; n<(humanShowDetail?5:1); n++)
    {
        p = &buffer[sizeof(buffer)-1];
        i = 0;

        *p = '\0';

        if (n != 0)
        {
            *--p = 'B';
            *--p = qualifier[n];
        }
        else if (!humanShowDetail)
        {
            *--p = 'B';
        }

        do
        {
            if(i%3 == 0 && i != 0)
                *--p = ',';
            *--p = '0' + number % 10;
            number /= 10;
            i++;
        } while(number != 0);

        if (n == 0)
        {
            strcpy(humanNumber, p);
        }
        else
        {
            strcat(humanNumber, n == 1 ? " (" : " ");
            strcat(humanNumber, p);
        }

        if ((baseNumber = baseNumber / 1024) == 0)
        {
            if (n >= 1) strcat(humanNumber, ")");
            break;
        }

        number = baseNumber;
    }

    return humanNumber;
}

void test_cgroups_trace(int level, int opt, const char * file, int lineno, const char * fmt, ...)
{
    printf("%s:%d\n", file, lineno);
}

ism_common_TraceFunction_t traceFunction = test_cgroups_trace;

//****************************************************************************
/// @brief Read (the first part) of a file into a buffer for parsing
///
/// @param[out]    buffer[IEMEM_MEMINFO_BUFFERSIZE]  buffer to store the contents in
/// @param[out]    bytesRead                         How many bytes were read from the file
///
/// @remark Reading files from /proc should occur in a single read (as they update between reads)
///
/// @remark Only the first IEMEM_MEMINFO_BUFFERSIZE (at most) of the file are read
//*******************************************************************************
static inline int32_t iemem_readMemInfoFile(char *filename,
                                            char buffer[IEMEM_MEMINFO_BUFFERSIZE],
                                            int *bytesRead)
{
    int f = open(filename, O_RDONLY);
    int32_t rc = OK;

    if (f < 0)
    {
        TRACE(ENGINE_ERROR_TRACE, "Couldn't open %s, errno: %d\n", filename, errno);
        rc = ISMRC_Error;
    }
    else
    {
        int bytes_read = read(f, buffer, IEMEM_MEMINFO_BUFFERSIZE - 1);

        if (bytes_read < 1)
        {
            TRACE(ENGINE_ERROR_TRACE, "Couldn't read from %s, errno: %d\n", filename, errno);
            rc = ISMRC_Error;
        }
        else
        {
            buffer[bytes_read] = 0;
            *bytesRead = bytes_read;
        }
        close(f);
    }
    return rc;
}

//****************************************************************************
/// @brief Read a simple (single value) from a file
///
/// @param[in]     filename            The file name to use
/// @param[out]    pValue              Pointer to receive a uint64_t value
///
/// @return OK or an ISMRC_ error
//*******************************************************************************
static inline int32_t iemem_readSimpleValueFromFile(char *filename, uint64_t *pValue)
{
    int32_t rc = OK;
    char buffer[50];

    int f = open(filename, O_RDONLY);
    if (f < 0)
    {
        TRACE(ENGINE_ERROR_TRACE, "Couldn't open %s, errno: %d\n", filename, errno);
        rc = ISMRC_NotFound;
    }
    else
    {
        size_t bytes_read = read(f, buffer, sizeof(buffer)-1);

        if (bytes_read < 1)
        {
            TRACE(ENGINE_ERROR_TRACE, "Couldn't read from %s, errno: %d\n", filename, errno);
            rc = ISMRC_NotFound;
        }
        else
        {
            *pValue = strtoul(buffer, NULL, 10);
        }

        close(f);
    }

    return rc;
}

//****************************************************************************
/// @brief Read the memory cgroup information for the running process
///
/// @param[out]    buffer[IEMEM_MEMINFO_BUFFERSIZE]  buffer to use when reading files
/// @param[out]    cgroupMemInfo                     structure to fill in
///
/// @return OK if the cgroup could be determined, and the files read, else ISMRC_NotFound
//*******************************************************************************
static inline int32_t iemem_readCgroupMemInfo(char buffer[IEMEM_MEMINFO_BUFFERSIZE],
                                              iemem_my_cgroupMemInfo_t *cgroupMemInfo)
{
    int bytes_read = 0;
    int32_t rc = OK;

    static char *cgroupDirectory = NULL;
    static bool cgroupIsUnified = false; //In v1 of cgroups there were separate dirs for memory/cpu etc. In v2 they have been combined (and format changed)

    // Work out which cgroup we're a member of (if any)
    if (cgroupDirectory == NULL)
    {
        int isTopLevelGroup = 0;
        bool isUnified = false;
        char *pathptr = NULL;

        rc = ism_common_getCGroupPath(ISM_CGROUP_MEMORY,
                                      buffer,
                                      IEMEM_MEMINFO_BUFFERSIZE,
                                      &pathptr,
                                      &isTopLevelGroup);
        if (rc == ISMRC_OK)
        {
            isUnified = false; //found a memory specific cgroup
        }
        else
        {
            rc = ism_common_getCGroupPath(ISM_CGROUP_UNIFIED,
                                          buffer,
                                          IEMEM_MEMINFO_BUFFERSIZE,
                                          &pathptr,
                                          &isTopLevelGroup);
            if (rc == ISMRC_OK)
            {
                isUnified = true; //found a v2 unified cgroup
            }
        }

        if (rc == ISMRC_OK)
        {
            size_t pathLen = strlen(pathptr);
            char *myCgroupDirectory = malloc(pathLen+2);
            assert(myCgroupDirectory != NULL);

            // Copy the cgroup name into our buffer, and add a slash if required.
            memcpy(myCgroupDirectory, pathptr, pathLen);
            if (pathLen > 1) myCgroupDirectory[pathLen++] = '/';
            myCgroupDirectory[pathLen] = 0;

            // Update the name, if someone else beat us to it, check we agree and free our area
            if (__sync_bool_compare_and_swap(&cgroupDirectory,
                                             NULL,
                                             myCgroupDirectory) == true)
            {
                cgroupIsUnified = isUnified;
                TRACE(ENGINE_INTERESTING_TRACE, "%s CGroup directory string: '%s'\n",
                        (isUnified ? "Unified": "Memory"), myCgroupDirectory);

            }
            else
            {
                assert(strcmp(cgroupDirectory, myCgroupDirectory) == 0);
                free(myCgroupDirectory);
            }
        }
        else
        {
            TRACE(ENGINE_ERROR_TRACE, "Couldn't find cgroup %d\n",rc);
            rc = ISMRC_NotFound;
        }
    }

    // We know our cgroup directory and it is not blank, so now we can try to access
    // information from it.
    if (cgroupDirectory != NULL && cgroupDirectory[0] != 0)
    {
        assert(rc == OK);

        // Read limit
        char fname[100+strlen(cgroupDirectory)];
        if (cgroupIsUnified)
        {
            sprintf(fname, IEMEM_UNIFIED_CGROUP_LIMIT_IN_BYTES_FORMAT, cgroupDirectory);
        }
        else
        {
            sprintf(fname, IEMEM_MEMORY_CGROUP_LIMIT_IN_BYTES_FORMAT, cgroupDirectory);
        }
        rc = iemem_readSimpleValueFromFile(fname, &cgroupMemInfo->limitInBytes);

        // Read usage
        // From https://www.kernel.org/doc/Documentation/cgroups/memory.txt
        // 5.5 usage_in_bytes
        // For efficiency, as other kernel components, memory cgroup uses some optimization
        // to avoid unnecessary cacheline false sharing. usage_in_bytes is affected by the
        // method and doesn't show 'exact' value of memory (and swap) usage, it's a fuzz
        // value for efficient access. (Of course, when necessary, it's synchronized.)
        // If you want to know more exact memory usage, you should use RSS+CACHE(+SWAP)
        // value in memory.stat(see 5.2).
        //
        // In practice, the fuzz value seems to be accurate enough, and means we only
        // need to read the cache from memory.stat.
        if (rc == OK)
        {
            if (cgroupIsUnified)
            {
                sprintf(fname, IEMEM_UNIFIED_CGROUP_USAGE_IN_BYTES_FORMAT, cgroupDirectory);
            }
            else
            {
                sprintf(fname, IEMEM_MEMORY_CGROUP_USAGE_IN_BYTES_FORMAT, cgroupDirectory);
            }

            rc = iemem_readSimpleValueFromFile(fname, &cgroupMemInfo->usageInBytes);
        }

        // Read from stats
        if (rc == OK)
        {
            bytes_read = 0;

            sprintf(fname, IEMEM_CGROUP_MEMORY_STAT_FORMAT, cgroupDirectory);

            rc = iemem_readMemInfoFile(fname, buffer, &bytes_read);
        }
        // Pull values from the stat file
        if (rc == OK)
        {
            #define IEMEM_STAT_LINES 6
            char       *valueIdentifier[IEMEM_STAT_LINES] = {"cache ", "inactive_file ", "active_file ", "rss ", "swap ", "mapped_file ",};
            size_t   valueIdentifierLen[IEMEM_STAT_LINES] = {6,        14,               12,             4,      5,       12};
            bool   valueIdentifierFound[IEMEM_STAT_LINES] = {false,    false,            false,          false,  false,   false};
            uint64_t              value[IEMEM_STAT_LINES] = {0,        0,                0,              0,      0,       0};

            int32_t bufPos = 0;
            int32_t foundVals = 0;

            //While we haven't found all the values and we have more data left to parse...
            while ((foundVals < IEMEM_STAT_LINES) && (bufPos < bytes_read))
            {
                int32_t i;

                //for each string we're looking for...
                for (i=0; i < IEMEM_STAT_LINES; i++)
                {
                    if ( !(valueIdentifierFound[i]) )
                    {
                        if (strncmp(&buffer[bufPos], valueIdentifier[i], valueIdentifierLen[i]) == 0)
                        {
                            //We've matched one of the strings we're looking for
                            bufPos += valueIdentifierLen[i];

                            errno = 0;
                            value[i] = strtoul(&buffer[bufPos], NULL, 10);

                            if (errno == 0)
                            {
                                valueIdentifierFound[i] = true;
                                foundVals++;
                                break;
                            }
                            else
                            {
                                TRACE(ENGINE_ERROR_TRACE, "Failed to parse numeric value %d from memory.stat.", i);
                                rc = ISMRC_Error;
                            }
                        }
                    }
                }

                //Skip to the next line
                while((bufPos < bytes_read) && (buffer[bufPos] != '\n'))
                {
                    bufPos++;
                }

                while((bufPos < bytes_read) && isspace(buffer[bufPos]))
                {
                    bufPos++;
                }
            }

            assert(rc == OK);

            cgroupMemInfo->cacheBytes = value[0];
            cgroupMemInfo->activeFileBytes = value[1];
            cgroupMemInfo->inactiveFileBytes = value[2];
            cgroupMemInfo->rssBytes = value[3];
            cgroupMemInfo->swapBytes = value[4];
            cgroupMemInfo->mappedFileBytes = value[5];

            // Memory in tempfs file systems is included in cache, but will not be freed under
            // memory pressure, so we calculate it based on the formula:
            //
            //       tmpfs = cache - active_file - inactive_file
            //
            // This can then be used in later calculations.
            cgroupMemInfo->tmpfsBytes = cgroupMemInfo->cacheBytes - cgroupMemInfo->activeFileBytes - cgroupMemInfo->inactiveFileBytes;

            #if 0
            printf("cache: %lu\nrss: %lu\nmapped_file: %lu\nswap: %lu\ninactive_file: %lu\nactive_file:%lu\n",
                    cgroupMemInfo->cacheBytes,
                    cgroupMemInfo->rssBytes,
                    cgroupMemInfo->mappedFileBytes,
                    cgroupMemInfo->swapBytes,
                    cgroupMemInfo->inactiveFileBytes,
                    cgroupMemInfo->activeFileBytes);
            getchar();
            #endif
        }

        if (rc == OK)
        {
            cgroupMemInfo->freeBytes = cgroupMemInfo->limitInBytes - cgroupMemInfo->usageInBytes;
            cgroupMemInfo->accurateUsageInBytes = cgroupMemInfo->rssBytes + cgroupMemInfo->swapBytes + cgroupMemInfo->cacheBytes - cgroupMemInfo->tmpfsBytes;
            cgroupMemInfo->accurateFreeBytes = cgroupMemInfo->limitInBytes - cgroupMemInfo->accurateUsageInBytes;
        }
    }
    else
    {
        rc = ISMRC_NotFound;
    }

    return rc;
}

void TouchPages(void *ptr, size_t memorySize)
{
    if (ptr != NULL)
    {
        char *localPtr = (char *)ptr;

        *localPtr = '\0';

        size_t morePages = memorySize/4096;

        while(morePages > 0)
        {
            localPtr += 4096;
            *localPtr = '\0';
            morePages--;
        }
    }
}

void ShowUsage(void)
{
    char buffer[IEMEM_MEMINFO_BUFFERSIZE];
    iemem_my_cgroupMemInfo_t myCGroupMemInfo = {0};
    char uIB[50], auIB[50], fIB[50], afIB[50];

    DEBUG_ONLY int32_t rc = iemem_readCgroupMemInfo(buffer, &myCGroupMemInfo);

    assert(rc == OK);

    printf("----\nAccU: %s\nAFiB: %s\n",
           makeHuman(myCGroupMemInfo.accurateUsageInBytes, auIB),
           makeHuman(ACC_FREE_INC_BUFFERS(&myCGroupMemInfo), afIB));
    printf("Used: %s\nFriB: %s\n",
           makeHuman(myCGroupMemInfo.usageInBytes, uIB),
           makeHuman(FREE_INC_BUFFERS(&myCGroupMemInfo), fIB));
}

char *humanUsage(char *fmtBuffer)
{
    char buffer[IEMEM_MEMINFO_BUFFERSIZE];
    iemem_my_cgroupMemInfo_t myCGroupMemInfo = {0};

    DEBUG_ONLY int32_t rc = iemem_readCgroupMemInfo(buffer, &myCGroupMemInfo);

    assert(rc == OK);

    return makeHuman(myCGroupMemInfo.usageInBytes, fmtBuffer);
}

char *humanFreeIncBuff(char *fmtBuffer)
{
    char buffer[IEMEM_MEMINFO_BUFFERSIZE];
    iemem_my_cgroupMemInfo_t myCGroupMemInfo = {0};

    DEBUG_ONLY int32_t rc = iemem_readCgroupMemInfo(buffer, &myCGroupMemInfo);

    assert(rc == OK);

    return makeHuman(FREE_INC_BUFFERS(&myCGroupMemInfo), fmtBuffer);
}

void test_mem_largeAllocs(char **ptrs, bool noisy)
{
    char fmtBuffer[50];
    char fmtBuffer2[50];

    printf(">>>> %s Used: %s FiB: %s\n",
            __func__, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));

    for(int32_t loop=0; loop<10; loop++)
    {
        uint64_t amount=50*(loop+1)*IEMEM_MEBIBYTE;
        if (noisy) printf("++++ Allocating %s\n", makeHuman(amount, fmtBuffer));
        ptrs[0] = malloc(amount);
        assert(ptrs[0] != NULL);
        TouchPages(ptrs[0], malloc_usable_size(ptrs[0]));
        if (noisy) ShowUsage();
        if (noisy) printf("++++ Freeing %s\n", makeHuman(malloc_usable_size(ptrs[0]), fmtBuffer));
        free(ptrs[0]);
        ptrs[0]=NULL;
        if (noisy) ShowUsage();
    }

    printf("==== %s Used: %s FiB: %s (pre-trim)\n",
            __func__, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
    malloc_trim(0);
    printf("<<<< %s Used: %s FiB %s\n",
            __func__, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
}

void test_mem_growShrink(char **ptrs, uint64_t chunkSize, bool noisy)
{
    char fmtBuffer[50];
    char fmtBuffer2[50];

    printf(">>>> %s %lu byte Chunks Used: %s FiB: %s\n",
            __func__, chunkSize, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));

    int32_t chunkCount=(int32_t)(malloc_usable_size(ptrs)/sizeof(char *));

    for(int32_t loop=0; loop<10; loop++)
    {
        if (noisy)
        {
            printf("++++ Loop %d\n", loop+1);
            ShowUsage();
        }

        if (noisy) printf("++++ Allocating %d %lu byte chunks\n", chunkCount, chunkSize);
        for(int32_t chunk=0; chunk<chunkCount; chunk++)
        {
            ptrs[chunk] = malloc(chunkSize);
            TouchPages(ptrs[chunk], malloc_usable_size(ptrs[chunk]));
        }
        if (noisy) ShowUsage();

        if (noisy) printf("++++ Freeing in reverse order\n");
        for(int32_t chunk=chunkCount-1; chunk>=0; chunk--)
        {
            free(ptrs[chunk]);
            ptrs[chunk] = NULL;
        }
        if (noisy) ShowUsage();
    }

    printf("==== %s %lu byte Chunks Used: %s FiB: %s (pre-trim)\n",
            __func__, chunkSize, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
    malloc_trim(0);
    printf("<<<< %s %lu byte Chunks Used: %s FiB: %s\n",
            __func__, chunkSize, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
}

void test_mem_growUngrow(char **ptrs, uint64_t chunkSize, bool noisy)
{
    char fmtBuffer[50];
    char fmtBuffer2[50];

    printf(">>>> %s %lu byte Chunks Used: %s FiB: %s\n",
            __func__, chunkSize, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));

    int32_t chunkCount=(int32_t)(malloc_usable_size(ptrs)/sizeof(char *));

    for(int32_t loop=0; loop<1; loop++)
    {
        if (noisy)
        {
            printf("**** Loop %d\n", loop+1);
            ShowUsage();
        }

        if (noisy) printf("++++ Allocating %d %lu byte chunks\n", chunkCount, chunkSize);
        for(int32_t chunk=0; chunk<chunkCount; chunk++)
        {
            ptrs[chunk] = malloc(chunkSize);
            TouchPages(ptrs[chunk], malloc_usable_size(ptrs[chunk]));
        }
        if (noisy) ShowUsage();

        if (noisy) printf("++++ Freeing all but last\n");
        for(int32_t chunk=0; chunk<chunkCount-1; chunk++)
        {
            free(ptrs[chunk]);
            ptrs[chunk] = NULL;
        }
        if (noisy) ShowUsage();
    }

    printf("==== %s %lu byte Chunks Used: %s FiB: %s (pre-trim)\n",
            __func__, chunkSize, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
    malloc_trim(0);
    printf("==== %s %lu byte Chunks Used: %s FiB: %s (post-trim)\n",
            __func__, chunkSize, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
    if(noisy) printf("++++ Freeing last chunk\n");
    free(ptrs[chunkCount-1]);
    ptrs[chunkCount-1] = NULL;
    printf("<<<< %s %lu byte Chunks Used: %s FiB: %s\n",
            __func__, chunkSize, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
}

void test_file_write(uint64_t writeSize, const char *fileName, uint64_t totalFileSize, bool noisy)
{
    char fmtBuffer[50];
    char fmtBuffer2[50];

    printf(">>>> %s %lu byte buffers to %s Used: %s FiB: %s\n",
            __func__, writeSize, fileName, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));

    char *writeBuffer = malloc(writeSize);
    assert(writeBuffer != NULL);
    FILE *fp = fopen(fileName, "w");
    assert(fp != NULL);

    uint64_t writeCount = totalFileSize / writeSize;

    if (noisy) printf("++++ Writing %lu %lu byte buffers\n", writeCount, writeSize);
    for(uint64_t write=0; write<writeCount; write++)
    {
        DEBUG_ONLY size_t written = fwrite(writeBuffer, writeSize, 1, fp);
        assert(written == 1);
    }

    free(writeBuffer);
    printf("==== %s %lu byte buffers to %s Used: %s FiB: %s (pre-close)\n",
            __func__, writeSize, fileName, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
    fclose(fp);
    printf("==== %s %lu byte buffers to %s Used: %s FiB: %s (pre-trim)\n",
            __func__, writeSize, fileName, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
    malloc_trim(0);
    printf("==== %s %lu byte buffers to %s Used: %s FiB: %s\n",
            __func__, writeSize, fileName, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
}

void test_file_read(uint64_t readSize, const char *fileName, bool noisy)
{
    char fmtBuffer[50];
    char fmtBuffer2[50];

    printf(">>>> %s %lu byte buffers from %s Used: %s FiB: %s\n",
            __func__, readSize, fileName, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));

    char *readBuffer = malloc(readSize);
    uint64_t readCount = 0;
    assert(readBuffer != NULL);
    FILE *fp = fopen(fileName, "r");
    assert(fp != NULL);

    if (noisy) printf("++++ Reading %lu byte buffers\n", readSize);
    while(fread(readBuffer, readSize, 1, fp) == 1)
    {
        readCount += 1;
    }
    if (noisy) printf("++++ Read total of %lu buffers\n", readCount);
    free(readBuffer);
    printf("==== %s %lu byte buffers from %s Used: %s FiB: %s (pre-close)\n",
            __func__, readSize, fileName, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
    fclose(fp);
    printf("==== %s %lu byte buffers from %s Used: %s FiB: %s (pre-trim)\n",
            __func__, readSize, fileName, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
    malloc_trim(0);
    printf("==== %s %lu byte buffers from %s Used: %s FiB: %s\n",
            __func__, readSize, fileName, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
}

void test_shm_largeAllocs(char **ptrs, bool noisy)
{
    char fmtBuffer[50];
    char fmtBuffer2[50];
    char shmName[NAME_MAX + 1] = "";

    printf(">>>> %s Used: %s FiB: %s\n",
            __func__, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));

    sprintf(shmName, "/testCGroups_%s::%u", __func__, getuid());

    struct shminfo info = {0};
    DEBUG_ONLY int rc = shmctl(0, IPC_INFO, (struct shmid_ds *)&info);
    if (rc == -1)
    {
        printf("shmctl errno: %d\n", errno);
        exit(99);
    }

    size_t shmmax = (size_t)(info.shmmax);

    for(int32_t loop=0; loop<4; loop++)
    {
        uint64_t amount=10*(loop+1)*IEMEM_MEBIBYTE;

        int shmfd = shm_open(shmName, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
        assert(shmfd >= 0);
        if (ftruncate64(shmfd, amount) == -1)
        {
            printf("ftruncate errno: %d\n", errno);
            exit(99);
        }

        uint64_t segment;
        uint64_t segments = amount / shmmax;
        uint64_t remaining = amount % shmmax;

        if (noisy) printf("++++ Allocating %s in %lu segments\n", makeHuman(amount, fmtBuffer), segments+(remaining == 0 ? 0 : 1));
        for(segment=0; segment<segments; segment++)
        {
            ptrs[segment] = (char *)mmap(NULL, shmmax, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, segment*shmmax);
            if (ptrs[segment] == MAP_FAILED)
            {
                printf("mmap errno: %d\n", errno);
                exit(99);
            }
            TouchPages(ptrs[segment], shmmax);
        }
        if (remaining != 0)
        {
            ptrs[segment] = (char *)mmap(NULL, remaining, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, segment*shmmax);
            if (ptrs[segment] == MAP_FAILED)
            {
                printf("mmap errno: %d\n", errno);
                exit(99);
            }
            TouchPages(ptrs[segment], remaining);
            segment++;
        }
        if (noisy) ShowUsage();

        if (noisy) printf("++++ Freeing %s\n", makeHuman(amount, fmtBuffer));
        for(segment=0; segment<segments; segment++)
        {
            munmap(ptrs[segment], info.shmmax);
            ptrs[segment] = NULL;
        }
        if (remaining != 0)
        {
            munmap(ptrs[segment], remaining);
            ptrs[segment] = NULL;
        }
        if (noisy) ShowUsage();

        close(shmfd);
    }

    printf("==== %s Used: %s FiB: %s (pre-unlink)\n",
            __func__, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
    shm_unlink(shmName);
    printf("==== %s Used: %s FiB: %s (pre-trim)\n",
            __func__, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
    malloc_trim(0);
    printf("<<<< %s Used: %s FiB %s\n",
            __func__, humanUsage(fmtBuffer), humanFreeIncBuff(fmtBuffer2));
}

void CheckFreeIncBufferUnchanged(iemem_my_cgroupMemInfo_t *prevCGroupMemInfo, uint64_t allowDiff, bool noisy)
{
    char buffer[IEMEM_MEMINFO_BUFFERSIZE];

    iemem_my_cgroupMemInfo_t thisCGroupMemInfo = {0};

    DEBUG_ONLY int32_t rc = iemem_readCgroupMemInfo(buffer, &thisCGroupMemInfo);
    assert(rc == OK);

    uint64_t prevFiB = FREE_INC_BUFFERS(prevCGroupMemInfo);
    uint64_t thisFiB = FREE_INC_BUFFERS(&thisCGroupMemInfo);

    uint64_t diff;

    if (prevFiB > thisFiB)
    {
        diff = prevFiB-thisFiB;
    }
    else
    {
        diff = thisFiB-prevFiB;
    }

    char diffB[50];

    if (diff > allowDiff)
    {
        printf("     FreeIncBuffers changed by %s\n", makeHuman(diff, diffB));
        fflush(NULL);
        //exit(99);
    }
    else if (noisy)
    {
        printf("     FreeIncBuffers changed by %s\n", makeHuman(diff, diffB));
    }

    memcpy(prevCGroupMemInfo, &thisCGroupMemInfo, sizeof(*prevCGroupMemInfo));
}

int main(int argc, char **argv)
{
    int testStatus=0;
    char buffer[IEMEM_MEMINFO_BUFFERSIZE];

    iemem_my_cgroupMemInfo_t myCGroupMemInfo = {0};

    int32_t rc = iemem_readCgroupMemInfo(buffer, &myCGroupMemInfo);

    if (rc != OK)
    {
        printf("RC from iemem_readCGroupMemInfo was %d, skipping tests\n", rc);
    }
    // If the limit is less than 1 terabyte, we'll do more tests, otherwise assume no cgroup limit
    else if (myCGroupMemInfo.limitInBytes > 1000000000000)
    {
        printf("Limit in bytes > 1,000,000,000,000 assuming no cgroup, skipping tests\n");
    }
    else
    {
        char lIB[50];
        char **ptrs = NULL;

        printf("Limit: %s\n", makeHuman(myCGroupMemInfo.limitInBytes, lIB));

        ShowUsage();

        printf("====\nAllocating 10MB for pointer array\n");
        ptrs = malloc(10*IEMEM_MEBIBYTE);
        TouchPages(ptrs, malloc_usable_size(ptrs));
        ShowUsage();

        // Memory testing
        printf("----\n");

        iemem_my_cgroupMemInfo_t prevCGroupMemInfo;
        iemem_readCgroupMemInfo(buffer, &prevCGroupMemInfo);

        // Memory allocation
        test_mem_largeAllocs(ptrs, false);
        CheckFreeIncBufferUnchanged(&prevCGroupMemInfo, 10*IEMEM_MEBIBYTE, false);
        test_mem_growShrink(ptrs, 100, false);
        CheckFreeIncBufferUnchanged(&prevCGroupMemInfo, 10*IEMEM_MEBIBYTE, false);
        test_mem_growShrink(ptrs, 150, false);
        CheckFreeIncBufferUnchanged(&prevCGroupMemInfo, 10*IEMEM_MEBIBYTE, false);
        test_mem_growShrink(ptrs, 1024, false);
        CheckFreeIncBufferUnchanged(&prevCGroupMemInfo, 10*IEMEM_MEBIBYTE, false);
        test_mem_growUngrow(ptrs, 100, false);
        CheckFreeIncBufferUnchanged(&prevCGroupMemInfo, 10*IEMEM_MEBIBYTE, false);
        test_mem_growUngrow(ptrs, 512, false);
        CheckFreeIncBufferUnchanged(&prevCGroupMemInfo, 10*IEMEM_MEBIBYTE, false);

        // File reading / writing
        for(int32_t loop=0; loop<2; loop++)
        {
            test_file_write(10240, "/var/tmp/testCGroups.dat", 2048*IEMEM_MEBIBYTE, false);
            CheckFreeIncBufferUnchanged(&prevCGroupMemInfo, 10*IEMEM_MEBIBYTE, false);
            test_file_read(1024, "/var/tmp/testCGroups.dat", true);
            CheckFreeIncBufferUnchanged(&prevCGroupMemInfo, 10*IEMEM_MEBIBYTE, false);
        }
        unlink("/var/tmp/testCGroups.dat");

        // Shared memory
        test_shm_largeAllocs(ptrs, false);

        printf("====\nFreeing pointer array\n");
        free(ptrs);
        ShowUsage();
    }

    return testStatus;
}
