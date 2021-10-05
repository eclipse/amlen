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
/// @file  test_utils_file.h
/// @brief Common file / directory manipulation routines.
//****************************************************************************
#ifndef __ISM_TEST_UTILS_FILE_H_DEFINED
#define __ISM_TEST_UTILS_FILE_H_DEFINED

#include "ismutil.h"

typedef struct tag_testUtilsReplace_t
{
    const char *find;
    const char *replace;
} testUtilsReplace_t;

//****************************************************************************
/// @brief Remove the directory, including subdirectories & files.
///
/// @param[in]     directory  The directory to remove
///
/// @remark This routine will guard against being called with a blank string,
///         and against being called with a single '/' - but it should be used
///         with caution since it has the potential to destroy files on the
///         target machine.
///
/// @return OK on successful completion or non-zero on failure
//****************************************************************************
int32_t test_removeDirectory(char *directory);

//****************************************************************************
/// @brief Redirect stdout the specified file
///
/// @param[in]     fileName  The file name to which to redirect stdout
///
/// @return file descriptor of original stdout, or -1 on failure.
//****************************************************************************
int test_redirectStdout(char *fileName);

//****************************************************************************
/// @brief Reinstate the file descriptor as stdout
///
/// @param[in]     oldStdout  The stdout fd returned by test_redirectStdout
///
/// @return -1 on failure.
///
/// @see test_redirectStdout
//****************************************************************************
int test_reinstateStdout(int oldStdout);

//****************************************************************************
/// @brief Copy a source file to a target directory/file
///
/// @param[in]     sourceFile  The source file
/// @param[in]     targetDir   The target directory or file
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_copyFile(const char *sourceFile, const char *targetDir);

//****************************************************************************
/// @brief Change the mode of a file
///
/// @param[in]     targetFile   The target file
/// @param[in]     mode         file mode to set
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_chmod(const char *targetFile, const char *mode);

//****************************************************************************
/// @brief Copy a source file to a target directory
///
/// @param[in]     sourceFile      The source file
/// @param[in]     targetDir       The target directory
/// @param[in]     targetFilename  The target file name
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_copyFileRename(const char *sourceFile,
                        const char *targetDir,
                        const char *targetFilename);

//****************************************************************************
/// @brief Copy a source file to a target directory stripping out lines with
///        a particular string contained within them.
///
/// @param[in]     sourceFile  The source file
/// @param[in]     targetDir   The target directory
/// @param[in]     replaceInfo Array of replacements
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_copyFileWithReplace(const char *sourceFile,
                             const char *targetDir,
                             testUtilsReplace_t *replaceInfo);

//****************************************************************************
/// @brief Copy a source file to a target directory stripping out lines
///        containing a specified string
///
/// @param[in]     sourceFile  The source file
/// @param[in]     targetDir   The target directory
/// @param[in]     stripText   Text to identify lines to strip
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_copyFileWithStrip(const char *sourceFile,
                           const char *targetDir,
                           const char *stripText);

//****************************************************************************
/// @brief Append a source file to a target file
///
/// @param[in]     sourceFile  The source file
/// @param[in]     targetFile  The target file
///
/// @remark If sourceFile is NULL, just create targetFile
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_copyFileAppend(const char *sourceFile,
                        const char *targetFile);

//****************************************************************************
/// @brief Append a string to a target file
///
/// @param[in]     text        The text to append
/// @param[in]     targetFile  The target file
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_appendTextToFile(char *text,
                          const char *targetFile);

//****************************************************************************
/// @brief Read a config file in and put all the values found into a
///        properties structure
///
/// @param[in]     configFile  The config file to read
/// @param[in]     properties  The properties structure to fill in
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_readConfigFile(const char *configFile,
                        ism_prop_t *properties);

//****************************************************************************
/// @brief Get the fully qualified path to the specified resource file from
/// the server_engine test/resources subdirectory.
///
/// @param[in]     fileName    The resource file name to locate
///
/// @remark If there are no environment variables available to determine the
/// path of the source tree, the code will fail and return NULL.
///
/// The simplest variable to set is SROOT which should be the fully qualified
/// path to the directory above the server_engine source directory.
///
/// @return malloc'd path (caller must free) or NULL if no env set
//****************************************************************************
char *test_getTestResourceFilePath(const char *fileName);

#endif //end ifndef __ISM_TEST_UTILS_FILE_H_DEFINED
