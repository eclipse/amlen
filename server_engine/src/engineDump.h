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
/// @file  engineDump.h
/// @brief Data types for Engine dumping routines
//****************************************************************************
#ifndef __ISM_ENGINEDUMP_DEFINED
#define __ISM_ENGINEDUMP_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include <engine.h>           /* Engine external header file         */
#include <engineCommon.h>     /* Engine common internal header file  */
#include <engineInternal.h>   /* Engine internal header file         */
#include <stdint.h>           /* Standard integer defns header file  */

/*********************************************************************/
/*                                                                   */
/* DATA TYPES                                                        */
/*                                                                   */
/*********************************************************************/
#define iedmUSE_OPEN
#if defined(iedmUSE_OPEN)
#include <fcntl.h>
typedef int iedmFile_t;
#define iedmINVALID_FILE -1
#define iedm_write(file_, addr_, len_)    \
   {                                      \
   int ignored __attribute__((unused));   \
   ignored = write(file_, addr_, len_);   \
   }
#define iedm_open(name_) creat(name_, 0644)
#define iedm_close(file_) close(file_)
#else
typedef FILE * iedmFile_t;
#define iedmINVALID_FILE NULL
#define iedm_write(file_, addr_, len_) fwrite(addr_, 1, len_, file_)
#define iedm_open(name_) fopen(name_, "w")
#define iedm_close(file_) fclose(file_)
#endif

// Maximum buffer size to use when writing to a file
#define iedmDUMP_DATA_BUFFER_SIZE_MAX 100*1024
// Minimum buffer size to use - if we cannot allocate this much, just
// use unbuffered writes to the file.
#define iedmDUMP_DATA_BUFFER_SIZE_MIN 1024
// Deepest stack size we support for loop checking
#define iedmDUMP_MAX_STACK_DEPTH 100

typedef struct tag_iedmDump_t
{
    iedmFile_t  fp;
    int32_t     detailLevel;
    int64_t     userDataBytes;
    char       *buffer;
    size_t      bufferPos;
    size_t      bufferSize;
    uint8_t     depth;
    void       *stack[iedmDUMP_MAX_STACK_DEPTH];
    bool        temporaryFile;
} iedmDump_t;

// Character identifying the different block types written to a dump
#define iedmDUMP_BLOCK_TYPE_DESCRIPTION 0xF0
#define iedmDUMP_BLOCK_TYPE_START_GROUP 0xF1
#define iedmDUMP_BLOCK_TYPE_END_GROUP   0xF2

//****************************************************************************
/// @brief Start an object, checking it has not already been dumped in this
///        stack
///
/// @param[in]     dump    Pointer to a dump structure
/// @param[in]     object  The object being started
///
/// @returns true of the object should be dumped, otherwise false.
//****************************************************************************
static inline bool iedm_dumpStartObject(iedmDump_t *dump, void *object)
{
    if (object != NULL && dump->depth < iedmDUMP_MAX_STACK_DEPTH)
    {
        int32_t localDepth = (int32_t)(dump->depth)-1;

        while(localDepth >= 0)
        {
            if (dump->stack[localDepth] == object) break;

            localDepth--;
        }

        // Didn't find this object in the current stack, dump it.
        if (localDepth == -1)
        {
            dump->stack[dump->depth++] = object;
            return true;
        }
    }

    return false;
}

//****************************************************************************
/// @brief End an object
///
/// @param[in]     dump    Pointer to a dump structure
/// @param[in]     object  The object being ended
//****************************************************************************
static inline void iedm_dumpEndObject(iedmDump_t *dump, void *object)
{
    assert(dump->depth > 0);

    dump->depth--;

    assert(dump->stack[dump->depth] == object); // Check allignment
}

//****************************************************************************
/// @brief Start a group of related data blocks
///
/// @param[in]     dump       Pointer to a dump structure
/// @param[in]     groupType  Type of group
//****************************************************************************
static inline void iedm_dumpStartGroup(iedmDump_t *dump, char *groupType)
{
    const size_t groupNameLen = strlen(groupType)+1;
    const size_t bufferSize = dump->bufferSize;
    const size_t entrySize = groupNameLen + 1;
    size_t bufferPos = dump->bufferPos;

    if (bufferPos + entrySize > bufferSize)
    {
        iedm_write(dump->fp, dump->buffer, bufferPos);
        bufferPos = 0;
    }

    if (entrySize > bufferSize)
    {
        const uint8_t blockType = iedmDUMP_BLOCK_TYPE_START_GROUP;
        iedm_write(dump->fp, &blockType, sizeof(blockType));
        iedm_write(dump->fp, groupType, groupNameLen);
    }
    else
    {
        char *bufferPointer = &dump->buffer[bufferPos];

        *(bufferPointer++) = iedmDUMP_BLOCK_TYPE_START_GROUP;
        memcpy(bufferPointer, groupType, groupNameLen);

        bufferPos += entrySize;
    }

    dump->bufferPos = bufferPos;
}

//****************************************************************************
/// @brief End a group of related data blocks
///
/// @param[in]     dump       Pointer to a dump structure
//****************************************************************************
static inline void iedm_dumpEndGroup(iedmDump_t *dump)
{
    const size_t bufferSize = dump->bufferSize;
    size_t bufferPos = dump->bufferPos;

    if (bufferPos == bufferSize && bufferSize > 0)
    {
        iedm_write(dump->fp, dump->buffer, bufferPos);
        dump->bufferPos = 0;
    }

    if (bufferSize == 0)
    {
        const uint8_t blockType = iedmDUMP_BLOCK_TYPE_END_GROUP;
        iedm_write(dump->fp, &blockType, sizeof(blockType));
    }
    else
    {
        dump->buffer[(dump->bufferPos)++] = iedmDUMP_BLOCK_TYPE_END_GROUP;
    }
}

//****************************************************************************
/// @brief Write the specified data to the specified dump file as a block
///
/// @param[in]     dump       Pointer to a dump structure
/// @param[in]     dataType   String identifying the content of the data
/// @param[in]     address    Address of the data to dump
/// @param[in]     length     Length of the data to dump
//****************************************************************************
static inline void iedm_dumpData(iedmDump_t *dump,
                                 char *dataType,
                                 void *address,
                                 size_t length)
{
    const size_t dataTypeLen = strlen(dataType)+1;
    const size_t totalLen = 1 + length + dataTypeLen + sizeof(void *) + sizeof(void *) + sizeof(size_t);
    const size_t bufferSize = dump->bufferSize;
    size_t bufferPos = dump->bufferPos;
    const void *endAddress = ((char *)address) + length - 1;

    // New data would overflow the buffer, write the buffer out.
    if (bufferPos + totalLen > bufferSize)
    {
        iedm_write(dump->fp, dump->buffer, bufferPos);
        bufferPos = 0;
    }

    // New data is bigger than buffer - write the new data straight out.
    if (totalLen >= bufferSize)
    {
        iedm_write(dump->fp, &dump->depth, sizeof(dump->depth));
        iedm_write(dump->fp, dataType, dataTypeLen);
        iedm_write(dump->fp, &address, sizeof(void *));
        iedm_write(dump->fp, &endAddress, sizeof(void *));
        iedm_write(dump->fp, &length, sizeof(size_t));
        iedm_write(dump->fp, address, length);
    }
    // New data fits in the buffer, write it to the buffer
    else
    {
        char *bufferPointer = &dump->buffer[bufferPos];

        *(bufferPointer++) = dump->depth;
        memcpy(bufferPointer, dataType, dataTypeLen);
        bufferPointer += dataTypeLen;
        memcpy(bufferPointer, &address, sizeof(address));
        bufferPointer += sizeof(void *);
        memcpy(bufferPointer, &endAddress, sizeof(endAddress));
        bufferPointer += sizeof(void *);
        memcpy(bufferPointer, &length, sizeof(size_t));
        bufferPointer += sizeof(size_t);
        memcpy(bufferPointer, address, length);
        bufferPointer += length;

        bufferPos = (size_t)(bufferPointer-dump->buffer);
    }

    dump->bufferPos = bufferPos;
}

//****************************************************************************
/// @brief Write the specified data to the specified dump file as a block
///
/// @param[in]     dump       Pointer to a dump structure
/// @param[in]     dataType   String identifying the content of the data
/// @param[in]       ...        Repeated pairs of parameters
/// @param[in]       address    Address of the data to dump
/// @param[in]       length     Length of the data to dump
//****************************************************************************
void iedm_dumpDataV(iedmDump_t *dump,
                    char *dataType,
                    int numBlocks,
                    ...);

/*********************************************************************/
/*                                                                   */
/* MACROS                                                            */
/*                                                                   */
/*********************************************************************/
#define iedmDUMP_DETAIL_LEVEL_1  1
#define iedmDUMP_DETAIL_LEVEL_2  2
#define iedmDUMP_DETAIL_LEVEL_3  3
#define iedmDUMP_DETAIL_LEVEL_4  4
#define iedmDUMP_DETAIL_LEVEL_5  5
#define iedmDUMP_DETAIL_LEVEL_6  6
#define iedmDUMP_DETAIL_LEVEL_7  7
#define iedmDUMP_DETAIL_LEVEL_8  8
#define iedmDUMP_DETAIL_LEVEL_9  9

#define iedmDUMP_DESCRIPTION_VERSION_1 1

// If you update the way in which the dump file is written (i.e. the
// following macros) this version must be updated
#define iedmDUMP_DESCRIPTON_VERSION iedmDUMP_DESCRIPTION_VERSION_1

#define iedmDUMP_HEADER_EYECATCHER      "IMADUMP"
#define iedmDUMP_HEADER_BYTE_ORDER_MARK 0x0000FEFF

// Write Dump header to a file
// IMADUMP<UTF-32_BYTE_ORDER_MARK><DESCRIPTION_VERSION><POINTER_SIZE><SIZE_T_SIZE><DETAIL_LEVEL>
// <DUMP_TIME>\0<VERSION_INFO>\0
#define iedm_writeDumpHeader(_file, _detail, _dumpTime, _ver)\
{   uint32_t _32_vals[5];\
    _32_vals[0]=iedmDUMP_HEADER_BYTE_ORDER_MARK;\
    _32_vals[1]=iedmDUMP_DESCRIPTON_VERSION;\
    _32_vals[2]=sizeof(void *);\
    _32_vals[3]=sizeof(size_t);\
    _32_vals[4]=_detail;\
    iedm_write(_file, iedmDUMP_HEADER_EYECATCHER, strlen(iedmDUMP_HEADER_EYECATCHER));\
    iedm_write(_file, &_32_vals, sizeof(_32_vals));\
    iedm_write(_file, _dumpTime, strlen(_dumpTime)+1);\
    iedm_write(_file, _ver, strlen(_ver)+1);\
}

// Begin writing the description of a structure to a file
// {<NAME>\0<STRUCID_MEMBER>\0<STRUCID_VALUE>\0
#define iedm_descriptionStart(_file, _name, _strucIdMember, _strucIdValue)\
{   const uint8_t blockType = iedmDUMP_BLOCK_TYPE_DESCRIPTION;\
    const iedmFile_t _fp = _file;\
    _name *_this = NULL;\
    uint32_t _32_vals[2];\
    iedm_write(_fp, &blockType, sizeof(blockType));\
    iedm_write(_fp, #_name, strlen(#_name)+1);\
    iedm_write(_fp, #_strucIdMember, strlen(#_strucIdMember)+1);\
    iedm_write(_fp, _strucIdValue, strlen(_strucIdValue)+1);

// Write the description of a structure member to a file
// <NAME>\0<TYPE>\0<OFFSET><LENGTH>
// Note <NAME> must not start with a '}'.
#define iedm_describeMember(_type, _member)\
    _32_vals[0] = (uint32_t)offsetof(typeof(*_this),_member);\
    _32_vals[1] = (uint32_t)sizeof(_this->_member);\
    iedm_write(_fp, #_member, strlen(#_member)+1);\
    iedm_write(_fp, #_type, strlen(#_type)+1);\
    iedm_write(_fp, _32_vals, sizeof(_32_vals));

// Stop writing the description of a structure to a file
// }\0
#define iedm_descriptionEnd\
    iedm_write(_fp, &blockType, sizeof(blockType));\
}

int32_t iedm_dumpQueueByHandle(ismQHandle_t queueHandle,
                               int32_t detailLevel,
                               int64_t userDataBytes,
                               char *resultPath);

#endif /* __ISM_ENGINEDUMP_DEFINED */

/*********************************************************************/
/* End of engineDump.h                                               */
/*********************************************************************/
