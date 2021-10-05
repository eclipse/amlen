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
/// @file dumpFormatUtils.c
/// @brief Utility functions for the dump formatter
//****************************************************************************
#define TRACE_COMP Engine

#include <dumpFormat.h>       /* Dump formatting routines            */
#include <stdarg.h>
#include <math.h>

//****************************************************************************
/// @internal
///
/// @brief Extract a uint16_t from the specified buffer position
///
/// @param[in]     buffer      Buffer to extract the value from
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns uint16_t value
//****************************************************************************
uint32_t iefm_getUint16(char *buffer, iefmHeader_t *dumpHeader)
{
    uint16_t value;

    memcpy(&value, buffer, sizeof(uint16_t));

    return dumpHeader->byteSwap ? (value<<8)|(value>>8) : value;
}

//****************************************************************************
/// @internal
///
/// @brief Extract a uint32_t from the specified buffer position
///
/// @param[in]     buffer      Buffer to extract the value from
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns uint32_t value
//****************************************************************************
uint32_t iefm_getUint32(char *buffer, iefmHeader_t *dumpHeader)
{
    uint32_t value;

    memcpy(&value, buffer, sizeof(uint32_t));

    return dumpHeader->byteSwap ? __builtin_bswap32(value) : value;
}

//****************************************************************************
/// @internal
///
/// @brief Extract an int32_t from the specified buffer position
///
/// @param[in]     buffer      Buffer to extract the value from
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns int32_t value
//****************************************************************************
int32_t iefm_getInt32(char *buffer, iefmHeader_t *dumpHeader)
{
    int32_t value;

    memcpy(&value, buffer, sizeof(int32_t));

    return dumpHeader->byteSwap ? __builtin_bswap32(value) : value;
}

//****************************************************************************
/// @internal
///
/// @brief Extract a uint64_t from the specified buffer position
///
/// @param[in]     buffer      Buffer to extract the value from
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns uint64_t value
//****************************************************************************
uint64_t iefm_getUint64(char *buffer, iefmHeader_t *dumpHeader)
{
    uint64_t value;

    memcpy(&value, buffer, sizeof(uint64_t));

    return dumpHeader->byteSwap ? __builtin_bswap64(value) : value;
}

//****************************************************************************
/// @internal
///
/// @brief Extract a pointer from the specified buffer position
///
/// @param[in]     buffer      Buffer to extract the value from
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns void * value
//****************************************************************************
void *iefm_getPointer(char *buffer, iefmHeader_t *dumpHeader)
{
    void *value;

    memcpy(&value, buffer, sizeof(value));

    return dumpHeader->byteSwap ? (void *)__builtin_bswap64((uint64_t)value) : value;
}

//****************************************************************************
/// @internal
///
/// @brief  Get the loaded address of a particular member of a structure
///
/// @param[in]       dumpHeader  Dump formatting header
/// @param[in]       structure   Structure to use
/// @param[in]       name        Member name to look for
//  @param[out]      member      Pointer to the member description
///
/// @return Address of member on success, NULL on failure.
//****************************************************************************
void *iefm_getMemberAddress(iefmHeader_t *dumpHeader,
                            iefmStructureDescription_t *structure,
                            char *memberName,
                            iefmMemberDescription_t **member)
{
    void *memberAddress = NULL;
    iefmMemberDescription_t *localMember = NULL;

    for(int32_t i=0; i<structure->memberCount; i++)
    {
        if (strcmp(structure->member[i].name, memberName) == 0)
        {
            localMember = &structure->member[i];

            // Check that the requested value sits within the buffer
            if ((localMember->offset > structure->length) ||
                (localMember->offset + localMember->length > structure->length))
            {
                iefm_printLine(dumpHeader, "ERROR: Structure length (%lu) too small to contain member (off: %lu, len: %lu)",
                               structure->length, localMember->offset, localMember->length);
            }
            else
            {
                memberAddress = structure->buffer + localMember->offset;
                *member = localMember;
            }
            break;
        }
    }

    return memberAddress;
}

//****************************************************************************
/// @internal
///
/// @brief  Get the address in the structure buffer of the specified original
///         address.
///
/// @param[in]       dumpHeader    Dump formatting header
/// @param[in]       structure     Structure to use
/// @param[in]       startAddress  Original address to retrieve
///
/// @return Buffer address of the original date, NULL if not in buffer.
//****************************************************************************
void *iefm_getBufferAddress(iefmHeader_t *dumpHeader,
                            iefmStructureDescription_t *structure,
                            void *startAddress)
{
    void *bufferAddress = NULL;

    if (startAddress >= structure->startAddress)
    {
        size_t offset = (size_t)(startAddress-structure->startAddress);

        if (offset < structure->length)
        {
            bufferAddress = structure->buffer + offset;
        }
    }

    return bufferAddress;
}

//****************************************************************************
/// @internal
///
/// @brief  Convert a number to a human readable string
///
/// @param[in]      number       The number to convert
/// @param[in,out]  humanNumber  The buffer into which to write the output
//****************************************************************************
void iefm_convertToHumanNumber(int64_t number, char *humanNumber)
{
    char buffer[50];
    char *p = &buffer[sizeof(buffer)-1];
    int32_t i = 0;
    bool negative = false;

    if (number < 0)
    {
        negative = true;
        number *= -1;
    }

    *p = '\0';
    do
    {
        if(i%3 == 0 && i != 0)
            *--p = ',';
        *--p = '0' + number % 10;
        number /= 10;
        i++;
    } while(number != 0);

    if (negative)
        *--p = '-';

    strcpy(humanNumber, p);
}

//****************************************************************************
/// @internal
///
/// @brief  Convert a numeric byte size into a human readable string
///
/// @param[in]      bytes        The byte size to convert
/// @param[in,out]  humanFileSize The buffer into which to write the output
//****************************************************************************
void iefm_convertToHumanFileSize(size_t bytes, char *humanFileSize)
{
    if (bytes < 1024)
    {
        sprintf(humanFileSize, "%luB", bytes);
    }
    else
    {
        #define iefmHUMAN_READABLE_PREFIX "KMGTPE"
        int32_t exp = (int32_t)(log((double)bytes)/log(1024));
        sprintf(humanFileSize, "%.1f%c", bytes/pow(1024,exp), iefmHUMAN_READABLE_PREFIX[exp-1]);
    }
}

//****************************************************************************
/// @internal
///
/// @brief  Prints a horizontal separator line to a formatted trace
///
/// @param[in]       dumpHeader  Dump formatting header
//****************************************************************************
void iefm_printSeparator(iefmHeader_t *dumpHeader)
{
    fprintf(dumpHeader->outputFile, iefmDUMP_SEPARATOR_LINE "\n");
}

//****************************************************************************
/// @internal
///
/// @brief  Prints a line-feed to a formatted dump
///
/// @param[in]       dumpHeader  Dump formatting header
//****************************************************************************
void iefm_printLineFeed(iefmHeader_t *dumpHeader)
{
    fprintf(dumpHeader->outputFile, "\n");

    dumpHeader->inOutputLine = false;
}

//****************************************************************************
/// @internal
///
/// @brief  Prints a line to a formatted dump
///
/// @param[in]       dumpHeader  Dump formatting header
/// @param[in]       format      printf format string
/// @param[in]       ...         variable arguments
//****************************************************************************
void iefm_printLine(iefmHeader_t *dumpHeader, char *format, ...)
{
    va_list ap;

    va_start(ap, format);

    if (dumpHeader->inOutputLine == false)
    {
        fprintf(dumpHeader->outputFile, "%.*s", dumpHeader->outputIndent*4, iefmINDENT_BLANKS);
    }

    vfprintf(dumpHeader->outputFile, format, ap);

    va_end(ap);

    iefm_printLineFeed(dumpHeader);
}

//****************************************************************************
/// @internal
///
/// @brief  Prints a string to the output file, with no line-feed.
///
/// @param[in]       dumpHeader  Dump formatting header
/// @param[in]       format      printf format string
/// @param[in]       ...         variable arguments
//****************************************************************************
void iefm_print(iefmHeader_t *dumpHeader, char *format, ...)
{
    va_list ap;

    va_start(ap, format);

    if (dumpHeader->inOutputLine == false)
    {
        fprintf(dumpHeader->outputFile, "%.*s", dumpHeader->outputIndent*4, iefmINDENT_BLANKS);
    }

    vfprintf(dumpHeader->outputFile, format, ap);

    dumpHeader->inOutputLine = true;

    va_end(ap);

    return;
}

//****************************************************************************
/// @internal
///
/// @brief  Indent the output text
///
/// @param[in]       dumpHeader  Dump formatting header
//****************************************************************************
void iefm_indent(iefmHeader_t *dumpHeader)
{
    iefm_printLine(dumpHeader, "{");

    if (dumpHeader->outputIndent < iefmMAX_OUTPUT_INDENT)
    {
        dumpHeader->outputIndent++;
    }
}

//****************************************************************************
/// @internal
///
/// @brief  Outdent the output text
///
/// @param[in]       dumpHeader  Dump formatting header
//****************************************************************************
void iefm_outdent(iefmHeader_t *dumpHeader)
{
    if (dumpHeader->outputIndent > 0)
    {
        dumpHeader->outputIndent--;
    }

    iefm_printLine(dumpHeader, "}");
}

//****************************************************************************
/// @internal
///
/// @brief Check that the strucId matches the expected value writing an error
///        to the output file if not.
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     structure   Structure (including buffer, length & startAddress)
//****************************************************************************
void iefm_checkStrucId(iefmHeader_t *dumpHeader,
                       iefmStructureDescription_t *structure)
{
    void *memberAddress;
    iefmMemberDescription_t *member;
    size_t strucIdValueLen = strlen(structure->strucIdValue);

    if (strucIdValueLen > 0 &&
        (structure->strucIdMember != NULL && structure->strucIdMember[0] != '\0'))
    {
        memberAddress = iefm_getMemberAddress(dumpHeader, structure, structure->strucIdMember,
                                              &member);

        if (memberAddress == NULL)
        {
            iefm_printLine(dumpHeader, "ERROR: '%s.%s' not found in data.",
                           structure->name, structure->strucIdMember);
        }
        else if (strncmp(memberAddress, structure->strucIdValue, strucIdValueLen) != 0)
        {
            iefm_printLine(dumpHeader, "ERROR: '%s.%s' value '%.*s' does not match expected '%.*s'",
                           structure->name, structure->strucIdMember,
                           strucIdValueLen, memberAddress,
                           strucIdValueLen, structure->strucIdValue);
        }
    }
}

//****************************************************************************
/// @internal
///
/// @brief Find a structure in the existing dump header
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     type        The type of data we are looking for
///
/// @returns Pointer to a structure description, NULL if none is found
//****************************************************************************
iefmStructureDescription_t *iefm_findStructure(iefmHeader_t *dumpHeader, char *type)
{
    iefmStructureDescription_t *foundStructure = NULL;

    // Look for an existing structure description
    for(int32_t i=0; i<dumpHeader->structureCount; i++)
    {
        if (type[0] == dumpHeader->structure[i].name[0] &&
            strcmp(type, dumpHeader->structure[i].name) == 0)
        {
            foundStructure = &dumpHeader->structure[i];
            break;
        }
    }

    return foundStructure;
}

//****************************************************************************
/// @internal
///
/// @brief Generate a hash value for the specified key string.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @remark This is a version of the xor djb2 hashing algorithm
///
/// @return The hash value
//****************************************************************************
uint32_t iefm_generateHash(const char *key)
{
    uint32_t keyHash = 5381;
    char curChar;

    // TO GET STATIC HASH VALUES:    printf("key: '%s'", key);
    while((curChar = *key++))
    {
        keyHash = (keyHash * 33) ^ curChar;
    }
    // TO GET STATIC HASH VALUES:    printf(" 0x%08x\n", keyHash);


    return keyHash;
}
