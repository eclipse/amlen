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
/// @file dumpFormatMain.c
/// @brief Main functions of the engine dump formatting code.
//****************************************************************************
#define TRACE_COMP Engine

#include <getopt.h>
#include <dlfcn.h>
#include <string.h>
#include <engine.h>
#include <dumpFormat.h>
#include <commonMemHandler.h>

//****************************************************************************
/// @brief Process Command line arguments
///
/// @param[in]     argc  Count of arguments
/// @param[in]     argv  Array of arguments
///
/// @return The index of the first non-option argument on success, -1 on error.
//****************************************************************************
int iefm_processArgs(int argc, char **argv,
                     void **libHandle,
                     bool *emitOffsets,
                     bool *emitTypes,
                     bool *emitRawData,
                     FILE **outputFilePointer,
                     char **outputPath,
                     bool *flatten,
                     bool *showProgress,
                     char **structureFilePath,
                     void **filterAddress)
{
    int usage = 0;
    char opt;
    int long_index;

    struct option long_options[] = {
        { "funcLib", 1, NULL, 'l' },
        { "emitOffsets", 0, NULL, 0},
        { "emitTypes", 0, NULL, 0},
        { "emitRawData", 0, NULL, 'R'},
        { "stdout", 0, NULL, 'O'},
        { "stderr", 0, NULL, 'E'},
        { "outputPath", 0, NULL, 'o'},
        { "flatten", 0, NULL, 'f'},
        { "quiet", 0, NULL, 'q' },
        { "structureFile", 1, NULL, 's'},
        { "fa", 1, NULL, 0},
        { NULL, 1, NULL, 0 } };

    *libHandle = NULL;
    *emitOffsets = false;
    *emitTypes = false;
    *emitRawData = false;
    *outputFilePointer = NULL;
    *outputPath = NULL;
    *flatten = false;
    *showProgress = true;
    *structureFilePath = NULL;
    *filterAddress = NULL;

    while ((opt = getopt_long(argc, argv, "l:ROEo:fqs:", long_options, &long_index)) != -1)
    {
        // Turn long_index into long_options value
        if (opt == 0)
        {
            switch(long_index)
            {
                case 1:
                    *emitOffsets = true;
                    break;
                case 2:
                    *emitTypes = true;
                    break;
                case 10:
                    *filterAddress = (void *)strtoul(optarg, NULL, 0);
                    break;
                default:
                    usage=1;
                    break;
            }
        }
        else
        {
            // Now process options
            switch (opt)
            {
                case 'l':
                    *libHandle = dlopen(optarg, RTLD_LAZY | RTLD_GLOBAL);

                    if (*libHandle == NULL)
                    {
                        fprintf(stderr, "ERROR: Could not load '%s', errno=%d\n", optarg, errno);
                        usage=1;
                    }
                    break;
                case 'R':
                    *emitRawData = true;
                    break;
                case 'O':
                    *outputFilePointer = stdout;
                    break;
                case 'E':
                    *outputFilePointer = stderr;
                    break;
                case 'o':
                    *outputPath = ism_common_strdup(ISM_MEM_PROBE(ism_memory_engine_misc,1000),optarg);
                    // TODO: Ensure output Path exists?
                    break;
                case 'f':
                    *flatten = true;
                    break;
                case 'q':
                    *showProgress = false;
                    break;
                case 's':
                    if (access(optarg, F_OK) == 0)
                        *structureFilePath = ism_common_strdup(ISM_MEM_PROBE(ism_memory_engine_misc,1000),optarg);
                    else
                    {
                        fprintf(stderr, "ERROR: Description file '%s' does not exist.\n", optarg);
                    }
                    break;
                case '?':
                default:
                    usage=1;
                    break;
            }
        }
    }

    // No progress indicator if writing to stdout or stderr
    if (*outputFilePointer != NULL) *showProgress = false;

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-l library] [-o path] [-s file] [-q] [-O] [-E] [-U] <files>\n", argv[0]);
        fprintf(stderr, "       -l - Specify a formatting library to load\n");
        fprintf(stderr, "       -o - Specify an output path\n");
        fprintf(stderr, "       -f - Flatten output (ignore group markers)\n");
        fprintf(stderr, "       -q - Quiet (no progress shown)\n");
        fprintf(stderr, "       -s - Specify an alternative structure description file\n");
        fprintf(stderr, "       -R - Output data as raw data blocks\n");
        fprintf(stderr, "       -O - Output to stdout\n");
        fprintf(stderr, "       -E - Output to stderr\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "      --%s - Emit offsets of structure members\n", long_options[1].name);
        fprintf(stderr, "      --%s - Emit types of structure members\n", long_options[2].name);
    }

    return usage ? -1 : optind;
}

//****************************************************************************
/// @brief Main function
///
/// @param[in]     argc  Count of arguments
/// @param[in]     argv  Array of arguments
///
/// @return 0 on success or an ISMRC_ error code.
//****************************************************************************
int32_t main(int argc, char *argv[])
{
    int32_t rc = OK;
    iefmHeader_t processedHeader = {0};

    int32_t fileIndex = iefm_processArgs(argc, argv,
                                         &processedHeader.libHandle,
                                         &processedHeader.emitOffsets,
                                         &processedHeader.emitTypes,
                                         &processedHeader.emitRawData,
                                         &processedHeader.outputFile,
                                         &processedHeader.outputPath,
                                         &processedHeader.flatten,
                                         &processedHeader.showProgress,
                                         &processedHeader.structureFilePath,
                                         &processedHeader.filterAddress);

    // Look for errors in our argument processing, or a missing file spec
    if (fileIndex == -1)
    {
        rc = ISMRC_InvalidParameter;
    }
    else if (fileIndex >= argc)
    {
        fprintf(stderr, "No files specified.\n");
        rc = ISMRC_InvalidParameter;
    }

    if (rc != OK) goto mod_exit;

    // Separate structure file - read it in.
    if (processedHeader.structureFilePath != NULL)
    {
        processedHeader.inputFilePath = processedHeader.structureFilePath;
        rc = iefm_readAndFormatFile(&processedHeader);
    }

    if (rc != OK) goto mod_exit;

    // Work out the maximum file path of the input files
    for(int32_t i=fileIndex; i<argc; i++)
    {
        size_t fileNameLen = strlen(basename(argv[i]));

        if (fileNameLen > processedHeader.maxInputFieldSize)
        {
            processedHeader.maxInputFieldSize = fileNameLen;
        }
    }

    // Process each of the files specified in turn
    while(rc == OK && fileIndex < argc)
    {
        iefmHeader_t dumpHeader = processedHeader;

        dumpHeader.inputFilePath = argv[fileIndex++];

        rc = iefm_readAndFormatFile(&dumpHeader);
    }

mod_exit:

    if (processedHeader.outputPath != NULL) ism_common_free(ism_memory_engine_misc,processedHeader.outputPath);
    if (processedHeader.libHandle != NULL) dlclose(processedHeader.libHandle);
    if (processedHeader.structureFilePath != NULL) ism_common_free(ism_memory_engine_misc,processedHeader.structureFilePath);

    return rc;
}
