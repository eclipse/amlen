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
/// @file dumpFormat.c
/// @brief Dump formatting routines.
//****************************************************************************
#define TRACE_COMP Engine

#include <getopt.h>
#include <dlfcn.h>
#include <assert.h>
#include <sys/ioctl.h>

#include <engine.h>           /* Engine external header file         */
#include <engineCommon.h>     /* Engine common internal header file  */
#include <engineDump.h>       /* Engine dump constants and functions */
#include <dumpFormat.h>       /* Dump formatting routines            */

//****************************************************************************
/// @brief Show a progress line
///
/// @param[in]     dumpHeader   Dump formatting header
//****************************************************************************
#define iefmPROGRESS_BAR            "===================="
#define iefmPROGRESS_SHOW_IFS_WIDTH 17
#define iefmPROGRESS_SHOW_PB_WIDTH  iefmPROGRESS_SHOW_IFS_WIDTH + strlen(iefmPROGRESS_BAR) + 4
#define iefmPROGRESS_SHOW_OPS_WIDTH iefmPROGRESS_SHOW_PB_WIDTH + 8
#define iefmPROGRESS_SHOW_BLK_WIDTH iefmPROGRESS_SHOW_OPS_WIDTH + 13

static void iefm_showProgress(iefmHeader_t *dumpHeader)
{
    // Calculate the ratio of complete-to-incomplete.
    size_t currentInputPos = ftell(dumpHeader->inputFile);
    double ratio = currentInputPos/(double)(dumpHeader->inputFileSize);
    int count = ratio * strlen(iefmPROGRESS_BAR);

    char humanInputSize[50];
    iefm_convertToHumanFileSize(dumpHeader->inputFileSize, humanInputSize);
    size_t currentOutputPos = ftell(dumpHeader->outputFile);
    char humanOutputSize[50];
    iefm_convertToHumanFileSize(currentOutputPos, humanOutputSize);
    char humanFormattedCount[50];
    iefm_convertToHumanNumber((int64_t)dumpHeader->formattedCount, humanFormattedCount);
    char humanBlockCount[50];
    iefm_convertToHumanNumber((int64_t)dumpHeader->blockCount, humanBlockCount);

    // Build a string of progress information
    int32_t inputFieldSize = dumpHeader->maxInputFieldSize;
    int32_t outputFieldSize = inputFieldSize + (int32_t)strlen(".fmt");

    char progressBuffer[inputFieldSize + outputFieldSize + iefmPROGRESS_SHOW_BLK_WIDTH + 1];

    sprintf(progressBuffer, "%-*.*s %6.2f%% %s %.*s [%.*s%c%.*s] %7s %-*.*s (%s)",
            inputFieldSize, inputFieldSize, dumpHeader->inputFileName,
            ratio*100,
            humanInputSize,
            (int)(6-strlen(humanInputSize)), iefmINDENT_BLANKS,
            count, iefmPROGRESS_BAR,
            ratio == 1 ? iefmPROGRESS_BAR[0] : '>',
            (int)(strlen(iefmPROGRESS_BAR)-count), iefmINDENT_BLANKS,
            humanOutputSize,
            outputFieldSize, outputFieldSize, dumpHeader->outputFileName,
            humanFormattedCount);

    // Try and avoid showing partial progress
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    if (w.ws_col < (inputFieldSize + iefmPROGRESS_SHOW_IFS_WIDTH))
        progressBuffer[w.ws_col] = '\0';
    else if (w.ws_col < (inputFieldSize + iefmPROGRESS_SHOW_PB_WIDTH))
        progressBuffer[inputFieldSize + iefmPROGRESS_SHOW_IFS_WIDTH] = '\0';
    else if (w.ws_col < (inputFieldSize + outputFieldSize + iefmPROGRESS_SHOW_OPS_WIDTH))
        progressBuffer[inputFieldSize + iefmPROGRESS_SHOW_PB_WIDTH] = '\0';
    else if (w.ws_col < (inputFieldSize + outputFieldSize + iefmPROGRESS_SHOW_BLK_WIDTH))
        progressBuffer[inputFieldSize + outputFieldSize + iefmPROGRESS_SHOW_OPS_WIDTH] = '\0';

    fprintf(stdout, "%s\r", progressBuffer);
    fflush(stdout);
}

//****************************************************************************
/// @brief Read the end of the dump header and optionally write the formatted
///        header
///
/// @param[in]     dumpHeader   Dump formatting header (partially complete)
/// @param[in[     writeOutput  Whether to write the formatted header or not
///
/// @remark The header has been read up-to and including the version number
///
/// @returns OK or an ISMRC_ error code
//****************************************************************************
static int32_t iefm_readAndFormatHeader(iefmHeader_t *dumpHeader, bool writeOutput)
{
    int32_t rc = OK;

    char *bufPos = dumpHeader->buffer;
    size_t requestLen;
    size_t readLen;

    if (dumpHeader->inputVersion == iedmDUMP_DESCRIPTION_VERSION_1)
    {
        requestLen = sizeof(uint32_t)*3; // POINTER_SIZE & SIZE_T_SIZE & DETAIL_LEVEL

        readLen = fread(bufPos, 1, requestLen, dumpHeader->inputFile);

        if (readLen != requestLen)
        {
            rc = ISMRC_Error;
            goto mod_exit;
        }

        dumpHeader->inputPointerSize = iefm_getUint32(bufPos, dumpHeader); bufPos += sizeof(uint32_t);
        dumpHeader->inputSizeTSize = iefm_getUint32(bufPos, dumpHeader); bufPos += sizeof(uint32_t);
        dumpHeader->inputDetailLevel = iefm_getUint32(bufPos, dumpHeader); bufPos += sizeof(uint32_t);

        if (dumpHeader->inputPointerSize != 8 || dumpHeader->inputSizeTSize != 8)
        {
            fprintf(stderr, "ERROR: Only expecting 64-bit data.\n");
            rc = ISMRC_Error;
            goto mod_exit;
        }

        // Read in dump time
        requestLen = 500;

        readLen = fread(dumpHeader->buffer, 1, requestLen, dumpHeader->inputFile);
        char *nullPos = memchr(dumpHeader->buffer, '\0', readLen);
        if (nullPos == NULL)
        {
            fprintf(stderr, "ERROR: Could not identify dump time in %lu bytes.\n", readLen);
            rc = ISMRC_Error;
            goto mod_exit;
        }
        else
        {
            fseek(dumpHeader->inputFile, -(readLen-((nullPos+1)-dumpHeader->buffer)), SEEK_CUR);
        }

        // Remove leading spaces
        bufPos = dumpHeader->buffer;
        while(*bufPos == ' ') bufPos++;

        if ((dumpHeader->inputDumpTime = ism_common_strdup(ISM_MEM_PROBE(ism_memory_engine_misc,1000),bufPos)) == NULL)
        {
            fprintf(stderr, "ERROR: Cannot allocate storage for dump time\n");
            rc = ISMRC_AllocateError;
            goto mod_exit;
        }

        // Read in build version information
        requestLen = 500;

        readLen = fread(dumpHeader->buffer, 1, requestLen, dumpHeader->inputFile);
        nullPos = memchr(dumpHeader->buffer, '\0', readLen);
        if (nullPos == NULL)
        {
            fprintf(stderr, "ERROR: Could not identify version information in %lu bytes of file\n",
                    readLen);
            rc = ISMRC_Error;
            goto mod_exit;
        }
        else
        {
            fseek(dumpHeader->inputFile, -(readLen-((nullPos+1)-dumpHeader->buffer)), SEEK_CUR);
        }

        // Remove leading spaces
        bufPos = dumpHeader->buffer;
        while(*bufPos == ' ') bufPos++;

        if ((dumpHeader->inputBuildInfo = ism_common_strdup(ISM_MEM_PROBE(ism_memory_engine_misc,1000),bufPos)) == NULL)
        {
            fprintf(stderr, "ERROR: Could not allocate storage for build info.\n");
            rc = ISMRC_Error;
            goto mod_exit;
        }
    }
    else
    {
        fprintf(stderr, "Unexpected version (%u) specified.\n", dumpHeader->inputVersion);
        rc = ISMRC_Error;
        goto mod_exit;
    }

    if (writeOutput)
    {
        iefm_printSeparator(dumpHeader);
        iefm_printLine(dumpHeader, "Dump file:      %s", dumpHeader->inputFileName);
        iefm_printLine(dumpHeader, "Dump Time:      %s", dumpHeader->inputDumpTime);
        iefm_printLine(dumpHeader, "Build Info:     %s", dumpHeader->inputBuildInfo);
        iefm_printLine(dumpHeader, "Detail Level:   %u", dumpHeader->inputDetailLevel);
        iefm_printLine(dumpHeader, "Version:        %u", dumpHeader->inputVersion);
        if (dumpHeader->filterAddress != NULL)
        {
            iefm_printLine(dumpHeader, "Filter Address: %p", dumpHeader->filterAddress);
        }
        iefm_printSeparator(dumpHeader);
    }

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Write the formatted footer
///
/// @param[in]     dumpHeader   Dump formatting header (partially complete)
/// @param[in[     writeOutput  Whether to write the formatted footer or not
//****************************************************************************
static void iefm_writeDumpFooter(iefmHeader_t *dumpHeader, bool writeOutput)
{
    if (writeOutput)
    {
        iefm_printSeparator(dumpHeader);
        iefm_printLine(dumpHeader, "Blocks Read:      %lu\nBlocks Formatted: %lu",
                       dumpHeader->blockCount, dumpHeader->formattedCount);
        iefm_printSeparator(dumpHeader);
    }
}

//****************************************************************************
/// @brief Get type information for the member specified
///
/// @param[in]     dumpHeader   Dump formatting header
/// @param[in]     iefmMemberDescription_t member
//****************************************************************************
static void iefm_getTypeInfo(iefmHeader_t *dumpHeader, iefmMemberDescription_t *member)
{
    assert(strlen(member->type) < sizeof(member->mappedType));

    strcpy(member->mappedType, member->type);

    // Is this an array?
    char *ptr = strchr(member->mappedType, '[');

    if (ptr != NULL)
    {
        char *end;

        member->arraySize = strtod(ptr+1, &end);

        while(--ptr > member->mappedType)
        {
            if (*ptr != ' ')
            {
                *(ptr+1) = '\0';
                break;
            }
        }
    }
    else
    {
        member->arraySize = 0;
    }

    // Find the data type, note we iterate over mapping types
    do
    {
        assert(strchr(member->mappedType, '[') == NULL);

        // Is this an in-line value or a pointer?
        ptr = strchr(member->mappedType, '*');

        if (ptr != NULL)
        {
            member->isPointer = true;

            while(--ptr > member->mappedType)
            {
                if (*ptr != ' ')
                {
                    *(ptr+1) = '\0';
                    break;
                }
            }
        }
        else
        {
            member->isPointer = false;
        }

        member->hash = iefm_generateHash(member->mappedType);
    }
    while(iefm_mapTypes(dumpHeader, member->mappedType, member->hash) == true);

    if ((member->hash == iefmHASH_char && strcmp(member->mappedType, iefmKEY_char) == 0) ||
        (member->hash == iefmHASH_void && strcmp(member->mappedType, iefmKEY_void) == 0) ||
        (member->hash == iefmHASH_bool && strcmp(member->mappedType, iefmKEY_bool) == 0) ||
        (member->hash == iefmHASH_uint8_t && strcmp(member->mappedType, iefmKEY_uint8_t) == 0))
    {
        member->increment = 1;
    }
    else if ((member->hash == iefmHASH_uint16_t && strcmp(member->mappedType, iefmKEY_uint16_t) == 0))
    {
        member->increment = 2;
    }
    else if ((member->hash == iefmHASH_uint32_t && strcmp(member->mappedType, iefmKEY_uint32_t) == 0) ||
             (member->hash == iefmHASH_int32_t && strcmp(member->mappedType, iefmKEY_int32_t) == 0) ||
             (member->hash == iefmHASH_enum && strcmp(member->mappedType, iefmKEY_enum) == 0))
    {
        member->increment = 4;
    }
    else if (member->hash == iefmHASH_uint64_t && strcmp(member->mappedType, iefmKEY_uint64_t) == 0)
    {
        member->increment = 8;
    }
    else
    {
        member->increment = 0;
    }

    return;
}
//****************************************************************************
/// @brief Read a structure description from the dump
///
/// @param[in]     dumpHeader       Dump formatting header
///
/// @returns OK or an ISMRC_ error code
//****************************************************************************
#define iefmREAD_STRUCTURE_BUFFER_SIZE 1000
static int32_t iefm_readStructureDescription(iefmHeader_t *dumpHeader)
{
    int32_t rc = OK;

    char *bufPos;
    size_t requestLen = iefmREAD_STRUCTURE_BUFFER_SIZE;
    size_t readLen = 0;
    int32_t fieldNo = 0;

    iefmMemberDescription_t *newMember = NULL;
    iefmStructureDescription_t *newStructure = ism_common_realloc(ISM_MEM_PROBE(ism_memory_engine_misc,55),dumpHeader->structure,
                                                       sizeof(iefmStructureDescription_t)*(dumpHeader->structureCount+1));

    if (newStructure == NULL)
    {
        rc = ISMRC_AllocateError;
        goto mod_exit;
    }

    dumpHeader->structure = newStructure;
    newStructure = &dumpHeader->structure[dumpHeader->structureCount];
    memset(newStructure, 0, sizeof(iefmStructureDescription_t));
    newStructure->formatter = iefm_dataFormatter;
    newStructure->treatAsStruct = true;
    dumpHeader->structureCount += 1;

    do
    {
        // Read in up to requestLen bytes
        readLen = fread(dumpHeader->buffer, 1, requestLen, dumpHeader->inputFile);

        bufPos = dumpHeader->buffer;
        bufPos[readLen] = '\0';

        do
        {
            if (dumpHeader->inputVersion == iedmDUMP_DESCRIPTION_VERSION_1)
            {
                // String fields
                if (fieldNo < 5)
                {
                    // End of structure found
                    if (*bufPos == (char)iedmDUMP_BLOCK_TYPE_DESCRIPTION)
                    {
                        readLen -= 1;
                        goto mod_exit;
                    }

                    size_t stringLen = strlen(bufPos);

                    if (stringLen >= readLen) break;

                    readLen -= stringLen+1;

                    char *newString = ism_common_strdup(ISM_MEM_PROBE(ism_memory_engine_misc,1000),bufPos);

                    if (newString == NULL)
                    {
                        rc = ISMRC_AllocateError;
                        goto mod_exit;
                    }

                    bufPos += stringLen+1;

                    switch(fieldNo)
                    {
                        case 0:
                            newStructure->name = newString;
                            break;
                        case 1:
                            newStructure->strucIdMember = newString;
                            break;
                        case 2:
                            newStructure->strucIdValue = newString;
                            break;
                        case 3:
                            newMember = ism_common_realloc(ISM_MEM_PROBE(ism_memory_engine_misc,56),newStructure->member,
                                                sizeof(iefmMemberDescription_t)*(newStructure->memberCount+1));

                            if (newMember == NULL)
                            {
                                rc = ISMRC_AllocateError;
                                goto mod_exit;
                            }

                            newStructure->member = newMember;
                            newMember = &newStructure->member[newStructure->memberCount];
                            memset(newMember, 0, sizeof(iefmMemberDescription_t));
                            newStructure->memberCount += 1;

                            newMember->name = newString;
                            break;
                        case 4:
                            newMember->type = newString;
                            iefm_getTypeInfo(dumpHeader, newMember);
                            break;
                    }
                }
                // uint32_t fields
                else
                {
                    if (readLen < sizeof(uint32_t)) break;

                    readLen -= sizeof(uint32_t);

                    switch(fieldNo)
                    {
                        case 5:
                            newMember->offset = iefm_getUint32(bufPos, dumpHeader);
                            break;
                        case 6:
                            newMember->length = iefm_getUint32(bufPos, dumpHeader);
                            fieldNo = 2; // Set up for next member
                            break;
                    }

                    bufPos += sizeof(uint32_t);
                }

                fieldNo += 1;
            }
        }
        while(readLen > 0);

        if (readLen > 0) fseek(dumpHeader->inputFile, -readLen, SEEK_CUR);
    }
    while(1);

mod_exit:

    // Everything was OK, we can use either a custom formatter, or the generic one
    if (rc == OK)
    {
        newStructure->formatter = iefm_findCustomFormatter(dumpHeader,
                                                           newStructure->name,
                                                           iefm_structureFormatter);

        // Find out the length of the names in this structure (for formatting purposes)
        newStructure->maxMemberNameLen = 0;

        for(int32_t i=0; i<newStructure->memberCount; i++)
        {
            size_t thisMemberNameLen = strlen(newStructure->member[i].name);

            if (thisMemberNameLen > newStructure->maxMemberNameLen)
            {
                newStructure->maxMemberNameLen = thisMemberNameLen;
            }
        }

        // Find out the length of the types in this structure (for formatting purposes)
        newStructure->maxMemberTypeLen = 0;

        for(int32_t i=0; i<newStructure->memberCount; i++)
        {
            size_t thisMemberTypeLen = strlen(newStructure->member[i].type);

            if (thisMemberTypeLen > newStructure->maxMemberTypeLen)
            {
                newStructure->maxMemberTypeLen = thisMemberTypeLen;
            }

        }
    }

    // Back-track over unused characters
    if (readLen != 0) fseek(dumpHeader->inputFile, -readLen, SEEK_CUR);

    return rc;
}

//****************************************************************************
/// @brief Read a start group block from the dump input file and
///        format to the output file
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     showGroups  Whether to display the group or move over it
///
/// @returns OK or an ISMRC_ error code
//****************************************************************************
static int32_t iefm_readAndFormatStartGroup(iefmHeader_t *dumpHeader, bool showGroups)
{
    int32_t rc = OK;

    size_t requestLen = 30; // Assume small group type names
    size_t readLen = 0;

    // This code currently assumes it is reading from an IMADUMP file, if we have
    // more file types supported, they will need a different way to read in a block
    assert(dumpHeader->inputFileType == iefmFILE_TYPE_IMADUMP);

    do
    {
        assert(requestLen <= iefmDUMPHEADER_BUFFER_LENGTH);

        // Read in up to requestLen bytes
        readLen = fread(dumpHeader->buffer, 1, requestLen, dumpHeader->inputFile);

        dumpHeader->buffer[readLen] = '\0';

        if (dumpHeader->inputVersion == iedmDUMP_DESCRIPTION_VERSION_1)
        {
            // String field
            size_t stringLen = strlen(dumpHeader->buffer);

            if (stringLen >= readLen)
            {
                requestLen += 30;
            }
            else
            {
                readLen -= stringLen+1;

                if (showGroups == true)
                {
                    // TODO: Consider invoking a custom formatter for start group
                    if (stringLen != 0) iefm_printLine(dumpHeader, "%s", dumpHeader->buffer);
                    iefm_indent(dumpHeader);
                }
                break;
            }
        }

        if (readLen > 0) fseek(dumpHeader->inputFile, -readLen, SEEK_CUR);
    }
    while(1);

    if (readLen > 0) fseek(dumpHeader->inputFile, -readLen, SEEK_CUR);

    return rc;
}

//****************************************************************************
/// @brief Read an end group block from the dump input file and
///        format to the output file
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     showGroups  Whether to display the group or move over it
///
/// @returns OK or an ISMRC_ error code
//****************************************************************************
static int32_t iefm_readAndFormatEndGroup(iefmHeader_t *dumpHeader, bool showGroups)
{
    int32_t rc = OK;

    // This code currently assumes it is reading from an IMADUMP file, if we have
    // more file types supported, they will need a different way to read in a block
    assert(dumpHeader->inputFileType == iefmFILE_TYPE_IMADUMP);

    // Nothing further to read
    if (showGroups == true)
    {
        // TODO: Consider invoking a custom formatter for end group
        iefm_outdent(dumpHeader);
    }

    return rc;
}

//****************************************************************************
/// @brief Read a data block from the dump input file and format to the output file
///
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns OK or an ISMRC_ error code
//****************************************************************************
static int32_t iefm_readAndFormatDataBlock(iefmHeader_t *dumpHeader)
{
    int32_t rc = OK;

    char *bufPos;
    size_t requestLen = 100;
    size_t readLen = 0;
    int32_t fieldNo = 0;

    char *newString = NULL;
    iefmStructureDescription_t localStructure = {0};

    void *readBuffer = NULL;
    char *startAddress = NULL;
    char *endAddress = NULL;
    size_t length = 0;

    // This code currently assumes it is reading from an IMADUMP file, if we have
    // more file types supported, they will need a different way to read in a block
    assert(dumpHeader->inputFileType == iefmFILE_TYPE_IMADUMP);

    do
    {
        assert(requestLen <= iefmDUMPHEADER_BUFFER_LENGTH);

        // Read in up to requestLen bytes
        readLen = fread(dumpHeader->buffer, 1, requestLen, dumpHeader->inputFile);

        bufPos = dumpHeader->buffer;
        bufPos[readLen] = '\0';

        do
        {
            if (dumpHeader->inputVersion == iedmDUMP_DESCRIPTION_VERSION_1)
            {
                // String fields - structure
                if (fieldNo == 0)
                {
                    size_t stringLen = strlen(bufPos);

                    if (stringLen >= readLen)
                    {
                        requestLen += 100;
                        break;
                    }

                    readLen -= stringLen+1;

                    iefmStructureDescription_t *foundStructure = NULL;

                    // Not asking for unformatted data, so look for this structure definition
                    if (dumpHeader->emitRawData == false)
                    {
                        foundStructure = iefm_findStructure(dumpHeader, bufPos);
                    }

                    // If we found one, use it as the basis for calling the formatter
                    if (foundStructure != NULL)
                    {
                        memcpy(&localStructure, foundStructure, sizeof(iefmStructureDescription_t));
                    }
                    // None found, build a temporary structure description
                    else
                    {
                        newString = ism_common_strdup(ISM_MEM_PROBE(ism_memory_engine_misc,1000),bufPos);

                        if (newString == NULL)
                        {
                            rc = ISMRC_AllocateError;
                            goto mod_exit;
                        }

                        localStructure.name = newString;
                        localStructure.treatAsStruct = true;

                        // Explicitly requesting unformatted data
                        if (dumpHeader->emitRawData == true)
                        {
                            localStructure.formatter = iefm_dataFormatter;
                        }
                        else
                        {
                            localStructure.formatter = iefm_findCustomFormatter(dumpHeader,
                                                                                localStructure.name,
                                                                                iefm_dataFormatter);
                        }
                    }

                    bufPos += stringLen+1;
                }
                // uint32_t fields
                else if (fieldNo == 1)
                {
                    if (readLen < dumpHeader->inputPointerSize) break;

                    readLen -= dumpHeader->inputPointerSize;

                    startAddress = iefm_getPointer(bufPos, dumpHeader);

                    bufPos += dumpHeader->inputPointerSize;
                }
                else if (fieldNo == 2)
                {
                    if (readLen < dumpHeader->inputPointerSize) break;

                    readLen -= dumpHeader->inputPointerSize;

                    endAddress = iefm_getPointer(bufPos, dumpHeader);

                    bufPos += dumpHeader->inputPointerSize;
                }
                else if (fieldNo == 3)
                {
                    if (readLen < dumpHeader->inputSizeTSize) break;

                    readLen -= dumpHeader->inputSizeTSize;

                    length = (size_t)iefm_getUint64(bufPos, dumpHeader);

                    bufPos += dumpHeader->inputSizeTSize;
                }

                fieldNo += 1;
            }
        }
        while(readLen > 0 && fieldNo < 4);

        if (readLen > 0) fseek(dumpHeader->inputFile, -readLen, SEEK_CUR);
    }
    while(fieldNo < 3);

    localStructure.startAddress = startAddress;
    localStructure.endAddress = endAddress;

    // If we are interested in this address, format the output
    if (dumpHeader->filterAddress == NULL ||
        ((localStructure.startAddress <= dumpHeader->filterAddress) &&
         ((dumpHeader->filterAddress <= localStructure.endAddress))))
    {
        readBuffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,57),length+1);

        if (readBuffer == NULL)
        {
            rc = ISMRC_AllocateError;
            goto mod_exit;
        }

        readLen = fread(readBuffer, 1, length, dumpHeader->inputFile);

        if (readLen != length)
        {
            rc = ISMRC_Error;
            goto mod_exit;
        }

        ((char *)readBuffer)[readLen] = '\0';

        iefmStructureFormatter_t formatter = (iefmStructureFormatter_t)localStructure.formatter;

        localStructure.buffer = readBuffer;
        localStructure.length = length;

        rc = formatter(dumpHeader, &localStructure);

        dumpHeader->formattedCount++;
    }
    // Not interested in this address range, move on.
    else
    {
        fseek(dumpHeader->inputFile, length, SEEK_CUR);
    }

    readLen = 0; // Don't want to back-track

mod_exit:

    // Free the buffer used to read in the structure content
    if (readBuffer != NULL) ism_common_free(ism_memory_engine_misc,readBuffer);

    // Back-track over unused characters
    if (readLen != 0) fseek(dumpHeader->inputFile, -readLen, SEEK_CUR);

    // Free up the temporary string if we used it.
    if (newString != NULL) ism_common_free(ism_memory_engine_misc,newString);

    return rc;
}

//****************************************************************************
/// @brief Read and Format the specified file
///
/// @param[in]     dumpHeader     The dump Header to use
//****************************************************************************
XAPI int32_t iefm_readAndFormatFile(iefmHeader_t *dumpHeader)
{
    int32_t rc = OK;
    bool readingStructureFile = (dumpHeader->structureFilePath != NULL &&
                                 dumpHeader->inputFilePath == dumpHeader->structureFilePath);
    bool showProgress = dumpHeader->showProgress && !readingStructureFile;
    bool showHeaders = dumpHeader->filterAddress == NULL && !readingStructureFile;
    bool showGroups = !(dumpHeader->flatten) && showHeaders;

    dumpHeader->inputFileType = iefmFILE_TYPE_NONE;

    dumpHeader->buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,60),iefmDUMPHEADER_BUFFER_LENGTH);

    if (dumpHeader->buffer == NULL)
    {
        fprintf(stderr, "Failed to allocate buffer for header.\n");
        rc = ISMRC_AllocateError;
        goto mod_exit;
    }

    if (dumpHeader->inputFilePath != NULL)
    {
        if ((dumpHeader->inputFileName = strrchr(dumpHeader->inputFilePath, '/')) == NULL)
        {
            dumpHeader->inputFileName = dumpHeader->inputFilePath;
        }
        else
        {
            dumpHeader->inputFileName++;
        }

        dumpHeader->inputFile = fopen(dumpHeader->inputFilePath, "r");

        if (dumpHeader->inputFile == NULL)
        {
            fprintf(stderr, "Unable to open input file '%s'\n", dumpHeader->inputFileName);
            goto mod_exit;
        }

        // Calculate input file size
        fseek(dumpHeader->inputFile, 0, SEEK_END);
        dumpHeader->inputFileSize = ftell(dumpHeader->inputFile);
    }
    else
    {
        assert(dumpHeader->inputFile != NULL);
        assert(dumpHeader->outputFile != NULL);
    }

    // Move to the start of the input file
    fseek(dumpHeader->inputFile, 0, SEEK_SET);

    char *bufPos = dumpHeader->buffer;
    size_t requestLen = strlen(iedmDUMP_HEADER_EYECATCHER)+(sizeof(uint32_t)*2);
    size_t readLen = fread(bufPos, 1, requestLen, dumpHeader->inputFile);

    // Didn't read in enough to identify the file, skip it
    if (readLen != requestLen) goto mod_exit;

    // This is an IMADUMP file, read the header
    if (memcmp(bufPos, iedmDUMP_HEADER_EYECATCHER, strlen(iedmDUMP_HEADER_EYECATCHER)) == 0)
    {
        uint32_t tmpVal;

        bufPos += strlen(iedmDUMP_HEADER_EYECATCHER);

        // Check whether byte swapping is going to be needed
        memcpy(&tmpVal, bufPos, sizeof(tmpVal));
        bufPos += sizeof(tmpVal);

        if (tmpVal != iedmDUMP_HEADER_BYTE_ORDER_MARK) dumpHeader->byteSwap = true;

        // Work out what version of structure definitions we're using
        dumpHeader->inputVersion = iefm_getUint32(bufPos, dumpHeader);

        // We are unable to format a dump with a higher description version that we know about
        if (dumpHeader->inputVersion > iedmDUMP_DESCRIPTON_VERSION)
        {
            fprintf(stderr, "Unable to format version %u file '%s' with this version %u formatter.\n",
                    dumpHeader->inputVersion, dumpHeader->inputFileName, (unsigned int)iedmDUMP_DESCRIPTON_VERSION);
            goto mod_exit;
        }

        dumpHeader->inputFileType = iefmFILE_TYPE_IMADUMP;
    }
    // For now, we can only handle IMADUMP files as input
    else
    {
        goto mod_exit;
    }

    // Open the output file if one was not provided
    if (dumpHeader->outputFile == NULL && !readingStructureFile)
    {
        size_t allocLen = strlen(dumpHeader->inputFilePath) + strlen(".fmt") + 1;

        if (dumpHeader->outputPath != NULL) allocLen += strlen(dumpHeader->outputPath);

        dumpHeader->outputFilePath = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,61),allocLen);

        if (dumpHeader->outputFilePath == NULL)
        {
            fprintf(stderr, "Failed to allocate storage for output file path name.");
            rc = ISMRC_AllocateError;
            goto mod_exit;
        }

        if (dumpHeader->outputPath != NULL)
        {
            strcpy(dumpHeader->outputFilePath, dumpHeader->outputPath);

            if (dumpHeader->outputFilePath[strlen(dumpHeader->outputFilePath)] != '/')
            {
                strcat(dumpHeader->outputFilePath, "/");
            }

            strcat(dumpHeader->outputFilePath, dumpHeader->inputFileName);
        }
        else
        {
            strcpy(dumpHeader->outputFilePath, dumpHeader->inputFilePath);
        }

        strcat(dumpHeader->outputFilePath, ".fmt");

        if ((dumpHeader->outputFileName = strrchr(dumpHeader->outputFilePath, '/')) == NULL)
        {
            dumpHeader->outputFileName = dumpHeader->outputFilePath;
        }
        else
        {
            dumpHeader->outputFileName++;
        }

        dumpHeader->outputFile = fopen(dumpHeader->outputFilePath, "w");

        if (dumpHeader->outputFile == NULL)
        {
            fprintf(stderr, "Unable to open output file '%s'\n", dumpHeader->outputFileName);
            rc = ISMRC_Error;
            goto mod_exit;
        }
    }

    // Initial checks are over - read the data
    if (dumpHeader->inputFileType == iefmFILE_TYPE_IMADUMP)
    {
        uint8_t blockType = 0;

        if (iefm_readAndFormatHeader(dumpHeader, showHeaders) != OK)  goto mod_exit;

        while(fread(&blockType, sizeof(blockType), 1, dumpHeader->inputFile) == 1)
        {
            if (blockType == iedmDUMP_BLOCK_TYPE_DESCRIPTION)
            {
                if (readingStructureFile || dumpHeader->structureFilePath == NULL)
                {
                    rc = iefm_readStructureDescription(dumpHeader);
                }
                else
                {
                    // Read through to the end of the description
                    while((fread(&blockType, sizeof(blockType), 1, dumpHeader->inputFile) == 1) &&
                          (blockType != iedmDUMP_BLOCK_TYPE_DESCRIPTION));
                }
            }
            else
            {
                // If reading descriptions, stop when we get to the end
                if (readingStructureFile) break;

                dumpHeader->blockCount++;

                if (blockType < iedmDUMP_BLOCK_TYPE_DESCRIPTION)
                {
                    rc = iefm_readAndFormatDataBlock(dumpHeader);
                }
                else if (blockType == iedmDUMP_BLOCK_TYPE_START_GROUP)
                {
                    rc = iefm_readAndFormatStartGroup(dumpHeader, showGroups);
                }
                else if (blockType == iedmDUMP_BLOCK_TYPE_END_GROUP)
                {
                    rc = iefm_readAndFormatEndGroup(dumpHeader, showGroups);
                }
                // Unknown block type...
                else
                {
                    fprintf(stderr, "Unknown block type 0x%02x.\n", blockType);
                    rc = ISMRC_Error;
                }
            }

            if (rc != OK)
            {
                fprintf(stderr, "Failing with rc %d at offset %lu in input file '%s'.\n",
                        rc, (unsigned long)ftell(dumpHeader->inputFile), dumpHeader->inputFileName);
                break;
            }

            if (showProgress && (dumpHeader->formattedCount % 2000 == 0))
            {
                iefm_showProgress(dumpHeader);
            }
        }

        iefm_writeDumpFooter(dumpHeader, showHeaders);

        if (showProgress)
        {
            iefm_showProgress(dumpHeader);
            fprintf(stdout, "\n");
        }
    }

mod_exit:

    // Free up all of the structures & members we read in unless they were from
    // a structure file, in which case we keep them (and ultimately leak them).
    if (dumpHeader->structure != NULL && dumpHeader->structureFilePath == NULL)
    {
        for(int32_t s=0; s<dumpHeader->structureCount; s++)
        {
            iefmStructureDescription_t *structure = &dumpHeader->structure[s];

            if (structure->member != NULL)
            {
                for(int32_t m=0; m<structure->memberCount; m++)
                {
                    iefmMemberDescription_t *member = &structure->member[m];

                    if (member->name != NULL) ism_common_free(ism_memory_engine_misc,member->name);
                    if (member->type != NULL) ism_common_free(ism_memory_engine_misc,member->type);
                }

                ism_common_free(ism_memory_engine_misc,structure->member);
            }

            if (structure->name != NULL) ism_common_free(ism_memory_engine_misc,structure->name);
            if (structure->strucIdMember != NULL) ism_common_free(ism_memory_engine_misc,structure->strucIdMember);
            if (structure->strucIdValue != NULL) ism_common_free(ism_memory_engine_misc,structure->strucIdValue);
        }

        ism_common_free(ism_memory_engine_misc,dumpHeader->structure);
        dumpHeader->structure = NULL;
    }

    if (dumpHeader->inputBuildInfo != NULL)
    {
        ism_common_free(ism_memory_engine_misc,dumpHeader->inputBuildInfo);
        dumpHeader->inputBuildInfo = NULL;
    }
    if (dumpHeader->inputDumpTime != NULL)
    {
        ism_common_free(ism_memory_engine_misc,dumpHeader->inputDumpTime);
        dumpHeader->inputDumpTime = NULL;
    }
    if (dumpHeader->buffer != NULL)
    {
        ism_common_free(ism_memory_engine_misc,dumpHeader->buffer);
        dumpHeader->buffer = NULL;
    }
    if (dumpHeader->outputFilePath != NULL)
    {
        if (dumpHeader->outputFile != NULL)
        {
            fflush(dumpHeader->outputFile);
            fclose(dumpHeader->outputFile);
            dumpHeader->outputFile = NULL;
        }

        ism_common_free(ism_memory_engine_misc,dumpHeader->outputFilePath);
        dumpHeader->outputFilePath = NULL;
    }
    if (dumpHeader->inputFile != NULL)
    {
        fclose(dumpHeader->inputFile);
        dumpHeader->inputFile = NULL;
    }

    return rc;
}
