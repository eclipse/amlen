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
/// @file  dumpFormat.h
/// @brief Data types for Dump formatting tool
//****************************************************************************
#ifndef __ISM_IMADUMPFORMAT_DEFINED
#define __ISM_IMADUMPFORMAT_DEFINED

#include <stdint.h>            /* Standard integer defns header file  */
#include <stdio.h>             /* Standard I/O function definitions   */
#include <stdbool.h>           /* Boolean definitions                 */
#include <memory.h>            /* Memory function definitions         */
#include <dumpFormatHashes.h>

/*********************************************************************/
/*                                                                   */
/* DATA TYPES                                                        */
/*                                                                   */
/*********************************************************************/
#define iefmDUMPHEADER_BUFFER_LENGTH        10240

typedef enum tag_iefmFILE_TYPE
{
    iefmFILE_TYPE_NONE = 0,
    iefmFILE_TYPE_IMADUMP = 1
} iefmFILE_TYPE;

///> Description of a structure member
typedef struct tag_iefmMemberDescription_t
{
  char     *name;            ///> Name of the structure member
  uint32_t  offset;          ///> Offset within the structure of this member
  uint32_t  length;          ///> Length of the member
  char     *type;            ///> Defined type of the member
  char      mappedType[50];  ///> Mapped type of the member
  uint32_t  hash;            ///> Hash value for the mapped type
  bool      isPointer;       ///> If the type is a pointer
  int32_t   arraySize;       ///> The number of array elements in the type
  int32_t   increment;       ///> How much to increment the buffer pointer for an element
} iefmMemberDescription_t;

///> Description of a structure
typedef struct tag_iefmStructureDescription_t
{
    char                     *name;               ///> Name of the structure
    char                     *strucIdMember;      ///> Name of the structure member containing strucId
    char                     *strucIdValue;       ///> Expected value of the strucId member
    void                     *formatter;          ///> Formatting function to use for this structure
    iefmMemberDescription_t  *member;             ///> Array of members read from description
    uint32_t                  memberCount;        ///> Count of structure members read from description
    void                     *buffer;             ///> Buffer containing this structure
    size_t                    length;             ///> Length of the buffer
    void                     *startAddress;       ///> Start address of this structure in original dump
    void                     *endAddress;         ///> End address of this structure in original dump
    size_t                    maxMemberNameLen;   ///> Longest name of a member (for formatting)
    size_t                    maxMemberTypeLen;   ///> Longest name of a type (for formatting)
    bool                      treatAsStruct;      ///> Whether to treat this as a structure (indent/outdent around)
} iefmStructureDescription_t;

///> Description of the dump being formatted
typedef struct tag_iefmHeader_t
{
    // Input File
    int32_t                     maxInputFieldSize;   ///> Field size required to display the longest file name
    char                       *inputFilePath;       ///> Path of the input file being formatted
    char                       *inputFileName;       ///> Name of the input file (path stripped)
    FILE                       *inputFile;           ///> File pointer of the opened input file
    size_t                      inputFileSize;       ///> Size of the input file
    iefmFILE_TYPE               inputFileType;       ///> What type of input file we are reading from
    uint32_t                    inputVersion;        ///> The iedmDUMP_DESCRIPTON_VERSION pulled from the input
    uint32_t                    inputPointerSize;    ///> Pointer size pulled from the input (must be 8)
    uint32_t                    inputSizeTSize;      ///> size_t size pulled from the input (must be 8)
    uint32_t                    inputDetailLevel;    ///> Detail level specified
    char                       *inputDumpTime;       ///> Time of dump pulled from the input
    char                       *inputBuildInfo;      ///> Build information read from the file
    bool                        byteSwap;            ///> Whether the data needs to be byte swapped (NO!)
    // Output File
    char                       *outputPath;          ///> Directory specifying where output files should go
    char                       *outputFilePath;      ///> Path of the output file being written
    char                       *outputFileName;      ///> Name of the output file (path stripped)
    FILE                       *outputFile;          ///> File pointer of the opened output file
    // Requested options
    char                       *structureFilePath;   ///> Path to a separate file from which to read structure descriptions
    bool                        emitOffsets;         ///> Whether to include member offsets
    bool                        emitTypes;           ///> Whether to include member types
    bool                        emitRawData;         ///> Whether to display raw data blocks or formatted data
    bool                        flatten;             ///> Whether to flatten the output or show groups
    bool                        showProgress;        ///> Whether to show progress bars
    void                       *filterAddress;       ///> Request the formatting of a specific block
    void                       *libHandle;           ///> dlopen handle of the library containing user formatters
    // Runtime state variables
    char                       *buffer;              ///> Temporary buffer
    bool                        inOutputLine;        ///> Whether we are mid-way through a print line
    uint32_t                    outputIndent;        ///> How much we are currently indenting the output by
    uint64_t                    blockCount;          ///> Running total of data blocks read
    uint64_t                    formattedCount;      ///> Running total of data blocks formatted
    iefmStructureDescription_t *structure;           ///> Array of structures read from the dump file
    uint32_t                    structureCount;      ///> Count of structures read from the dump file
} iefmHeader_t;

#define iefmMAX_OUTPUT_INDENT 20
#define iefmINDENT_BLANKS "                                                                                "
#define iefmDUMP_SEPARATOR_LINE "-------------------------------------------------------------------------------"
#define iefmHEX_BYTES_PER_LINE 16

#define iefmDUMP_FORMAT_LIB  "libismdumpfmt.so"
#define iefmDUMP_FORMAT_FUNC "iefm_readAndFormatFile"

typedef int32_t (*iefmStructureFormatter_t)(iefmHeader_t *dumpHeader,
                                            iefmStructureDescription_t *structure);
typedef int32_t (*iefmReadAndFormatFile_t)(iefmHeader_t *dumpHeader);

//****************************************************************************
/// @brief Read and Format the specified file
///
/// @param[in]     dumpHeader     The dump Header to use
//****************************************************************************
int32_t iefm_readAndFormatFile(iefmHeader_t *dumpHeader);

//****************************************************************************
/// @brief Extract a uint16_t from the specified buffer position
///
/// @param[in]     buffer      Buffer to extract the value from
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns uint16_t value
//****************************************************************************
uint32_t iefm_getUint16(char *buffer, iefmHeader_t *dumpHeader);

//****************************************************************************
/// @brief Extract a uint32_t from the specified buffer position
///
/// @param[in]     buffer      Buffer to extract the value from
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns uint32_t value
//****************************************************************************
uint32_t iefm_getUint32(char *buffer, iefmHeader_t *dumpHeader);

//****************************************************************************
/// @brief Extract an int32_t from the specified buffer position
///
/// @param[in]     buffer      Buffer to extract the value from
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns int32_t value
//****************************************************************************
int32_t iefm_getInt32(char *buffer, iefmHeader_t *dumpHeader);

//****************************************************************************
/// @brief Extract a uint64_t from the specified buffer position
///
/// @param[in]     buffer      Buffer to extract the value from
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns uint64_t value
//****************************************************************************
uint64_t iefm_getUint64(char *buffer, iefmHeader_t *dumpHeader);

//****************************************************************************
/// @brief Extract a pointer from the specified buffer position
///
/// @param[in]     buffer      Buffer to extract the value from
/// @param[in]     dumpHeader  Dump formatting header
///
/// @returns void * value
//****************************************************************************
void *iefm_getPointer(char *buffer, iefmHeader_t *dumpHeader);

//****************************************************************************
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
                            iefmMemberDescription_t **member);

//****************************************************************************
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
                            void *startAddress);

//****************************************************************************
/// @brief  Convert a number to a human readable string
///
/// @param[in]      number       The number to convert
/// @param[in,out]  humanNumber  The buffer into which to write the output
//****************************************************************************
void iefm_convertToHumanNumber(int64_t number, char *humanNumber);

//****************************************************************************
/// @brief  Convert a numeric byte size into a human readable string
///
/// @param[in]      bytes        The byte size to convert
/// @param[in,out]  humanFileSize The buffer into which to write the output
//****************************************************************************
void iefm_convertToHumanFileSize(size_t bytes, char *humanFileSize);

//****************************************************************************
/// @brief  Prints a horizontal separator line to a formatted trace
///
/// @param[in]       dumpHeader  Dump formatting header
//****************************************************************************
void iefm_printSeparator(iefmHeader_t *dumpHeader);

//****************************************************************************
/// @brief  Prints a line-feed to a formatted dump
///
/// @param[in]       dumpHeader  Dump formatting header
//****************************************************************************
void iefm_printLineFeed(iefmHeader_t *dumpHeader);

//****************************************************************************
/// @brief  Prints a line to a formatted dump
///
/// @param[in]       dumpHeader  Dump formatting header
/// @param[in]       format      printf format string
/// @param[in]       ...         variable arguments
//****************************************************************************
void iefm_printLine(iefmHeader_t *dumpHeader, char *format, ...);

//****************************************************************************
/// @brief  Prints a string to the output file, with no line-feed.
///
/// @param[in]       dumpHeader  Dump formatting header
/// @param[in]       format      printf format string
/// @param[in]       ...         variable arguments
//****************************************************************************
void iefm_print(iefmHeader_t *dumpHeader, char *format, ...);

//****************************************************************************
/// @brief  Indent the output text
///
/// @param[in]       dumpHeader  Dump formatting header
//****************************************************************************
void iefm_indent(iefmHeader_t *dumpHeader);

//****************************************************************************
/// @brief  Outdent the output text
///
/// @param[in]       dumpHeader  Dump formatting header
//****************************************************************************
void iefm_outdent(iefmHeader_t *dumpHeader);

//****************************************************************************
/// @brief Check that the strucId matches the expected value writing an error
///        to the output file if not.
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     structure   Structure (including buffer, length & startAddress)
//****************************************************************************
void iefm_checkStrucId(iefmHeader_t *dumpHeader,
                       iefmStructureDescription_t *structure);

//****************************************************************************
/// @brief Find a structure in the existing dump header
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     type        The type of data we are looking for
///
/// @returns Pointer to a structure description, NULL if none is found
//****************************************************************************
iefmStructureDescription_t *iefm_findStructure(iefmHeader_t *dumpHeader, char *type);

//****************************************************************************
/// @brief Find a custom formatter for the specified structure
///
/// @param[in]     dumpHeader        The dump formatting header
/// @param[in]     structureName     The name of the structure
/// @param[in]     defaultFormatter  The default formatting function to use
///                                  if no other is found.
///
/// @remark This will find a function named iefm_<STRUCTURE>_Formatter in a
///         loaded library of formatting routines, failing that, it tries to
///         load one from the program, if all else fails, the default specified
///         is used.
///
/// @returns Pointer to either the special formatter, or a generic one.
//****************************************************************************
iefmStructureFormatter_t iefm_findCustomFormatter(iefmHeader_t *dumpHeader,
                                                  char *structureName,
                                                  iefmStructureFormatter_t defaultFormatter);

//****************************************************************************
/// @brief Basic formatter for any data block
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     structure   Structure (including buffer, length & startAddress)
///
/// @returns OK or an ISMRC_ error code
//****************************************************************************
int32_t iefm_dataFormatter(iefmHeader_t *dumpHeader,
                           iefmStructureDescription_t *structure);

//****************************************************************************
/// @brief Basic formatter for a structure
///
/// @param[in]     dumpHeader  Dump formatting header
/// @param[in]     structure   Structure (including buffer, length & startAddress)
///
/// @returns OK or an ISMRC_ error code
//****************************************************************************
int32_t iefm_structureFormatter(iefmHeader_t *dumpHeader,
                                iefmStructureDescription_t *structure);

//****************************************************************************
/// @brief Map some types to other types
///
/// @param[in]     dumpHeader  Header information from the dump
/// @param[in,out] type        Type string to check, updated if mapped
/// @param[in]     hash        Type hash
///
/// @returns true if a mapping was made, or false
//****************************************************************************
bool iefm_mapTypes(iefmHeader_t *dumpHeader, char *type, uint32_t hash);

//****************************************************************************
/// @brief Generate a hash value for the specified key string.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @remark This is a version of the xor djb2 hashing algorithm
///
/// @return The hash value
//****************************************************************************
uint32_t iefm_generateHash(const char *key);

#endif /* __ISM_IMADUMPFORMAT_DEFINED */

/*********************************************************************/
/* End of dumpFormat.h                                               */
/*********************************************************************/
