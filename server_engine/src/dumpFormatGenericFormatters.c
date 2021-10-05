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
/// @file dumpFormatGenericFormatters.c
/// @brief In-built generic formatting routines for engine dumps.
//****************************************************************************
#define TRACE_COMP Engine

#include <dumpFormat.h>
#include <engine.h>
#include <ctype.h>
#include <assert.h>

//****************************************************************************
/// @internal
///
/// @brief Basic formatter for any data block
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     structure   Structure (including buffer, length & startAddress)
///
/// @returns OK or an ISMRC_ error code
//****************************************************************************
int32_t iefm_dataFormatter(iefmHeader_t *dumpHeader,
                           iefmStructureDescription_t *structure)
{
    int32_t rc = OK;

    if (structure->treatAsStruct == true)
    {
        iefm_printLine(dumpHeader, "%p - %p %s",
                       structure->startAddress, structure->endAddress,
                       structure->name);
        iefm_indent(dumpHeader);
    }

    char line[iefmHEX_BYTES_PER_LINE * 6]; // Not precise, just big enough.
    int32_t hexPos = 12;
    int32_t asciiPos = 0;

    char *bufPos = (char *)structure->buffer;

    size_t i;

    // Start long data dumps on a new line
    if (structure->length > iefmHEX_BYTES_PER_LINE && dumpHeader->inOutputLine)
    {
        iefm_printLineFeed(dumpHeader);
    }

    for(i=0; i<structure->length;)
    {
        if (i%iefmHEX_BYTES_PER_LINE == 0)
        {
            if (hexPos != 12)
            {
                iefm_printLine(dumpHeader, "%s", line);
            }

            sprintf(line, "0x%08x: ", (uint32_t)i);
            hexPos = 12;
            memset(&line[hexPos], ' ', sizeof(line)-hexPos);

            asciiPos = hexPos + (iefmHEX_BYTES_PER_LINE*2)+(iefmHEX_BYTES_PER_LINE/4)+2;
            line[asciiPos-2] = '|';
            strcpy(&line[asciiPos + (iefmHEX_BYTES_PER_LINE)+(iefmHEX_BYTES_PER_LINE/4)], "|");
        }

        static char *iefmHEX_CHARS = "0123456789ABCDEF";

        line[hexPos++] = iefmHEX_CHARS[((*bufPos)>>4)&0x0F];
        line[hexPos++] = iefmHEX_CHARS[(*bufPos)&0x0F];
        line[asciiPos++] = isprint((int)*bufPos) ? *bufPos : '.';

        i++;
        bufPos++;

        if (i%4 == 0)
        {
            line[hexPos++] = ' ';
            line[asciiPos++] = ' ';
        }
    }

    // Write out the stragglers
    if (hexPos != 12)
    {
        iefm_printLine(dumpHeader, "%s", line);
    }

    if (structure->treatAsStruct) iefm_outdent(dumpHeader);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief Basic formatter for a structure
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     structure   Structure (including buffer, length & startAddress)
/// @param[in]     member      Member being formatted
/// @param[in]     address     Loaded (buffer) address of the member
///
/// @returns OK or an ISMRC_ error code
//****************************************************************************
void iefm_memberFormatter(iefmHeader_t *dumpHeader,
                          iefmStructureDescription_t *structure,
                          iefmMemberDescription_t *member,
                          void *address)
{
    // Print the name and type, nicely padded
    size_t padding;

    if (dumpHeader->emitTypes == true)
    {
        padding = structure->maxMemberTypeLen - strlen(member->type)+1;
        iefm_print(dumpHeader, "%s", member->type);
        iefm_print(dumpHeader, "%.*s",
                   (padding <= 4*iefmMAX_OUTPUT_INDENT) ? padding : iefmMAX_OUTPUT_INDENT*4, iefmINDENT_BLANKS);
        // iefm_print(dumpHeader, "#%s[%d]#", member->mappedType, member->arraySize);
    }

    padding = structure->maxMemberNameLen - strlen(member->name)+1;
    iefm_print(dumpHeader, "%s", member->name);
    iefm_print(dumpHeader, ":%.*s",
               (padding <= 4*iefmMAX_OUTPUT_INDENT) ? padding : iefmMAX_OUTPUT_INDENT*4, iefmINDENT_BLANKS);

    iefmStructureDescription_t localStructure;
    iefmStructureFormatter_t localFormatter = NULL;

    uint32_t localTypeHash = member->hash;
    bool localIsPointer = member->isPointer;
    int32_t localArraySize = member->arraySize;
    int32_t localIncrement = member->increment;
    uint32_t localBufferMax = 0;

    if (localIncrement == 0)
    {
        iefmStructureDescription_t *foundStructure = iefm_findStructure(dumpHeader, member->mappedType);

        if (foundStructure != NULL)
        {
            memcpy(&localStructure, foundStructure, sizeof(localStructure));

            iefmMemberDescription_t *lastMember = &foundStructure->member[foundStructure->memberCount-1];
            localStructure.length = lastMember->offset + lastMember->length;
        }
        else
        {
            memset(&localStructure, 0, sizeof(localStructure));

            localStructure.name = member->type;
            localStructure.formatter = iefm_findCustomFormatter(dumpHeader,
                                                                localStructure.name,
                                                                iefm_dataFormatter);

            if (localArraySize > 0) localStructure.length = member->length / localArraySize;
            else localStructure.length = member->length;
        }

        localIncrement = localIsPointer ? dumpHeader->inputPointerSize : localStructure.length;
        localFormatter = (iefmStructureFormatter_t)localStructure.formatter;
    }

    if (localIsPointer) localIncrement = dumpHeader->inputPointerSize;
    if (localArraySize > 0 && localTypeHash != iefmHASH_char) iefm_printLineFeed(dumpHeader);

    int32_t localArrayElement = 0;

    do
    {
        char *bufferPointer;

        if (localArraySize > 0 && localTypeHash != iefmHASH_char)
        {
            iefm_print(dumpHeader, "[%d] (%d/%d) ", localArrayElement, localArrayElement+1, localArraySize);
        }

        if (localIsPointer)
        {
            // address : The address of this member (or member array entry) in our loaded buffer.
            // originalPointer : The (pointer) value of this member from the original dump
            // bufferPointer : The equivalent location of originalPointer in our buffer
            void *originalPointer = iefm_getPointer(address, dumpHeader);
            bufferPointer = iefm_getBufferAddress(dumpHeader, structure, originalPointer);

            iefm_print(dumpHeader, "%p ", originalPointer);

            // If this is a pointer to ourselves, note it but don't follow the infinite
            // loop that it leads to!
            if (originalPointer == structure->startAddress)
            {
                iefm_print(dumpHeader, "*self*");
                bufferPointer = NULL;
            }
            else if (bufferPointer != NULL && member->hash == iefmHASH_void)
            {
                localBufferMax = (uint32_t)((char *)structure->endAddress-(char *)originalPointer)+1;
            }
        }
        else
        {
            bufferPointer = address;
        }

        if (bufferPointer == NULL)
        {
            iefm_printLineFeed(dumpHeader);
        }
        else if (localFormatter == NULL)
        {
            switch(localTypeHash)
            {
                case iefmHASH_char:
                    if (localIsPointer) iefm_printLine(dumpHeader, "\"%s\"", bufferPointer);
                    else
                    {
                        char c = *bufferPointer;

                        if (c != '\0') iefm_print(dumpHeader, "%c", c);
                    }
                break;
                case iefmHASH_void:
                    if (localIsPointer)
                    {
                        bool addElipsis = false;

                        if (localBufferMax > 8)
                        {
                            addElipsis = true;
                            localBufferMax = 8;
                        }

                        for(uint32_t i=0; i<localBufferMax; i++)
                        {
                            iefm_print(dumpHeader, "%02x", (uint8_t)bufferPointer[i]);
                        }

                        if (addElipsis) iefm_printLine(dumpHeader, "...");
                        else iefm_printLineFeed(dumpHeader);
                    }
                    else
                    {
                        iefm_print(dumpHeader, "%02x", *(uint8_t *)bufferPointer);
                    }
                    break;
                case iefmHASH_uint16_t:
                    {
                        uint16_t tmpVal = iefm_getUint16(bufferPointer, dumpHeader);

                        iefm_printLine(dumpHeader, "%hu (0x%04x)", tmpVal, tmpVal);
                    }
                    break;
                case iefmHASH_uint32_t:
                    {
                        uint32_t tmpVal = iefm_getUint32(bufferPointer, dumpHeader);

                        iefm_printLine(dumpHeader, "%u (0x%08x)", tmpVal, tmpVal);
                    }
                    break;
                case iefmHASH_int32_t:
                case iefmHASH_enum:
                    iefm_printLine(dumpHeader, "%d", iefm_getInt32(bufferPointer, dumpHeader));
                    break;
                case iefmHASH_uint64_t:
                    {
                        uint64_t tmpVal = iefm_getUint64(bufferPointer, dumpHeader);
                        iefm_printLine(dumpHeader, "%lu (0x%016lx)", tmpVal, tmpVal);
                    }
                    break;
                case iefmHASH_bool:
                    iefm_printLine(dumpHeader, "%s", *((char *)bufferPointer) == '\0' ? "false" : "true");
                    break;
                case iefmHASH_uint8_t:
                    iefm_printLine(dumpHeader, "%d", (int32_t)(*bufferPointer));
                    break;
                default:
                    //iefm_printLine(dumpHeader, "UNKNOWN HASH TYPE %08x", localTypeHash);
                    break;
            }
        }
        else
        {
            size_t bufferOffset = bufferPointer - (char *)(structure->buffer);

            localStructure.buffer = bufferPointer;
            localStructure.startAddress = structure->startAddress + bufferOffset;
            localStructure.endAddress = localStructure.startAddress + localStructure.length-1;

            (void)localFormatter(dumpHeader, &localStructure);
        }

        address += localIncrement;
    }
    while(++localArrayElement < localArraySize);


    if (localTypeHash == iefmHASH_char && !localIsPointer) iefm_printLineFeed(dumpHeader);

    return;
}

//****************************************************************************
/// @internal
///
/// @brief Basic formatter for a structure
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     structure   Structure (including buffer, length & startAddress)
///
/// @returns OK or an ISMRC_ error code
//****************************************************************************
int32_t iefm_structureFormatter(iefmHeader_t *dumpHeader,
                                iefmStructureDescription_t *structure)
{
    int32_t rc = OK;

    iefm_printLine(dumpHeader, "%p - %p %s", structure->startAddress, structure->endAddress, structure->name);
    iefm_indent(dumpHeader);

    iefm_checkStrucId(dumpHeader, structure);

    for(int32_t i=0; i<structure->memberCount; i++)
    {
        iefmMemberDescription_t *member = &structure->member[i];
        void *address = structure->buffer + structure->member[i].offset;

        if (dumpHeader->emitOffsets == true)
        {
            iefm_print(dumpHeader, "(%04x-%04x) ", (uint32_t)member->offset,
                       (uint32_t)(member->offset+member->length)-1);
        }

        iefm_memberFormatter(dumpHeader, structure, member, address);
    }

    iefm_outdent(dumpHeader);

    return rc;
}
