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
/// @file  test_utils_file.c
/// @brief Common file / directory manipulation routines.
//****************************************************************************
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ftw.h>

#include "test_utils_file.h"

int test_removeDirectoryCallback(const char *fpath,
                                 const struct stat *sb,
                                 int typeflag,
                                 struct FTW *ftwbuf)
{
    return remove(fpath);
}

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
int test_removeDirectory(char *directory)
{
    int rc = 0;
    size_t directoryLength = strlen(directory);

    // Try and guard against being called to delete the entire filesystem
    if (directoryLength == 0 ||
        (directoryLength == 1 && directory[0] == '/'))
    {
        rc = 1;
        goto mod_exit;
    }

    rc = nftw(directory,
              test_removeDirectoryCallback,
              64,
              FTW_DEPTH | FTW_PHYS);

    if (rc != 0)
    {
        printf("ERROR: Unable to remove directory rc=%d\n", rc);
        goto mod_exit;
    }

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Redirect stdout the specified file
///
/// @param[in]     fileName  The file name to which to redirect stdout
///
/// @return file descriptor of original stdout, or -1 on failure.
//****************************************************************************
int test_redirectStdout(char *fileName)
{
    fflush(stdout);

    int origStdout = dup(1);

    if (origStdout == -1)
    {
        fprintf(stderr, "ERROR: Could not duplicate stdout errno=%d\n", errno);
        goto mod_exit;
    }

    int newStdout;

    if (strcmp(fileName, "/dev/null") == 0)
    {
        newStdout = open(fileName, O_WRONLY);
    }
    else
    {
        newStdout = open(fileName, O_CREAT,S_IRWXU);
    }

    if (newStdout == -1)
    {
        int myErrno = errno;
        test_reinstateStdout(origStdout);
        fprintf(stderr, "ERROR: Could not open '%s' errno=%d\n", fileName, myErrno);
        origStdout = -1;
        goto mod_exit;
    }
    else
    {
        int newFd = dup2(newStdout, 1);
        close(newStdout);

        if (newFd != 1)
        {
            fprintf(stderr, "ERROR: New stdout did not become fd 1, it became fd %d\n", newFd);
            close(newFd);
            dup2(origStdout, 1);
            origStdout = -1;
            goto mod_exit;
        }
    }

mod_exit:

    if (origStdout == -1) fflush(stderr);

    return origStdout;
} /* BEAM suppression: file not closed */

//****************************************************************************
/// @brief Reinstate the file descriptor as stdout
///
/// @param[in]     oldStdout  The stdout fd returned by test_redirectStdout
///
/// @return -1 on failure.
///
/// @see test_redirectStdout
//****************************************************************************
int test_reinstateStdout(int oldStdout)
{
    int rc;

    fflush(stdout);

    rc = dup2(oldStdout, 1);
    if (rc == -1)
    {
        fprintf(stderr, "ERROR: Could not dup2 %d to 1, errno=%d\n", oldStdout, errno);
        goto mod_exit;
    }
    else
    {
        close(oldStdout);
    }

mod_exit:

    if (rc == -1) fflush(stderr);

    return rc;
}

//****************************************************************************
/// @brief Copy a source file to a target directory/file
///
/// @param[in]     sourceFile  The source file
/// @param[in]     targetDir   The target directory or file
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_copyFile(const char *sourceFile, const char *targetDir)
{
    int rc = 0;

    if (strcmp(sourceFile, targetDir) != 0)
    {
        pid_t pid = fork();

        if (pid)
        {
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status))
            {
                rc = WEXITSTATUS(status);
            }
        }
        else
        {
            execlp("cp", "cp", sourceFile, targetDir, (char *)0);
        }
    }

    return rc;
}

//****************************************************************************
/// @brief Change the mode of a file
///
/// @param[in]     targetFile   The target file
/// @param[in]     mode         file mode to set
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_chmod(const char *targetFile, const char *mode)
{
    int rc = 0;

    pid_t pid = fork();

    if (pid)
    {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            rc = WEXITSTATUS(status);
        }
    }
    else
    {
        execlp("chmod", "chmod", mode, targetFile, (char *)0);
    }

    return rc;
}

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
                        const char *targetFilename)
{
    int rc = 0;

    pid_t pid = fork();

    if (pid)
    {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            rc = WEXITSTATUS(status);
        }
    }
    else
    {
        char targetPath[strlen(targetDir)+strlen(targetFilename)+2];

        sprintf(targetPath, "%s/%s", targetDir, targetFilename);

        execlp("cp", "cp", sourceFile, targetPath, (char *)0);
    }

    return rc;
}

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
                             testUtilsReplace_t *replaceInfo)
{
    int rc = 0;
    char targetFile[strlen(targetDir)+strlen(sourceFile)+1];
    char *sourceFileName = strrchr(sourceFile, '/');

    if (sourceFileName != NULL) sourceFileName += 1;
    else sourceFileName = (char *)sourceFile;

    strcpy(targetFile, targetDir);
    if (targetFile[strlen(targetFile)-1] != '/') strcat(targetFile, "/");
    strcat(targetFile, sourceFileName);

    testUtilsReplace_t *curReplaceInfo = replaceInfo;
    while((curReplaceInfo != NULL) && (curReplaceInfo->find != NULL))
    {
        if (strlen(curReplaceInfo->replace) > strlen(curReplaceInfo->find))
        {
            printf("ERROR: Replacement \"%s\" string longer than find string \"%s\"\n",
                   curReplaceInfo->replace, curReplaceInfo->find);
            rc = 1;
            goto mod_exit;
        }

        curReplaceInfo += 1;
    }

    FILE *source = fopen(sourceFile, "r");
    if (source == NULL)
    {
        printf("ERROR: Failed to open source '%s'\n", sourceFile);
        rc = 1;
        goto mod_exit;
    }

    FILE *target = fopen(targetFile, "w");
    if (target == NULL)
    {
        fclose(source);
        printf("ERROR: Failed to open target '%s'\n", targetFile);
        rc = 1;
        goto mod_exit;
    }

    char *line = NULL;
    size_t line_len;

    while(getline(&line, &line_len, source) != -1)
    {
        curReplaceInfo = replaceInfo;

        while((curReplaceInfo != NULL) && (curReplaceInfo->find != NULL))
        {
            char *foundPos;
            char *searchPos = line;

            while((foundPos = strstr(searchPos, curReplaceInfo->find)) != NULL)
            {
                char *endFoundPos = foundPos + strlen(curReplaceInfo->find);
                memcpy(foundPos, curReplaceInfo->replace, strlen(curReplaceInfo->replace));
                memmove(foundPos+strlen(curReplaceInfo->replace), endFoundPos, strlen(endFoundPos)+1);
                searchPos = foundPos+strlen(curReplaceInfo->replace);
            }

            curReplaceInfo += 1;
        }

        fprintf(target, "%s", line);
    }

    if (line != NULL) free(line);

    fclose(source);
    fclose(target);

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Copy a source file to a target directory stripping out lines with
///        a particular string contained within them.
///
/// @param[in]     sourceFile  The source file
/// @param[in]     targetDir   The target directory
/// @param[in]     stripText   Text to identify lines to strip
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_copyFileWithStrip(const char *sourceFile,
                           const char *targetDir,
                           const char *stripText)
{
    int rc = 0;
    char targetFile[strlen(targetDir)+strlen(sourceFile)+1];
    char *sourceFileName = strrchr(sourceFile, '/');

    if (sourceFileName != NULL) sourceFileName += 1;
    else sourceFileName = (char *)sourceFile;

    strcpy(targetFile, targetDir);
    if (targetFile[strlen(targetFile)-1] != '/') strcat(targetFile, "/");
    strcat(targetFile, sourceFileName);

    FILE *source = fopen(sourceFile, "r");
    if (source == NULL)
    {
        printf("ERROR: Failed to open source '%s'\n", sourceFile);
        rc = 1;
        goto mod_exit;
    }

    FILE *target = fopen(targetFile, "w");
    if (target == NULL)
    {
        fclose(source);
        printf("ERROR: Failed to open target '%s'\n", targetFile);
        rc = 1;
        goto mod_exit;
    }

    char *line = NULL;
    size_t line_len;

    while(getline(&line, &line_len, source) != -1)
    {
        if ((stripText == NULL) || (strstr(line,stripText) == NULL))
        {
            fprintf(target, "%s", line);
        }
    }

    if (line != NULL) free(line);

    fclose(source);
    fclose(target);

mod_exit:

    return rc;
}

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
                        const char *targetFile)
{
    int rc = 0;

    FILE *target = fopen(targetFile, "a");

    FILE *source = NULL;

    if (sourceFile != NULL) source = fopen(sourceFile, "r");

    if (target == NULL)
    {
        printf("ERROR: Failed to open target file '%s'\n", targetFile);
        rc = 1;
        goto mod_exit;
    }
    else if (sourceFile != NULL && source == NULL)
    {
        printf("ERROR: Failed to open source file '%s'\n", sourceFile);
        rc = 2;
        goto mod_exit;
    }

    if (source != NULL)
    {
        char *line = NULL;
        size_t line_len;

        while(getline(&line, &line_len, source) != -1)
        {
            fprintf(target, "%s", line);
        }

        if (line != NULL) free(line);
    }

mod_exit:

    if (target != NULL) fclose(target);
    if (source != NULL) fclose(source);

    return rc;
}

//****************************************************************************
/// @brief Append a string to a target file
///
/// @param[in]     text        The text to append
/// @param[in]     targetFile  The target file
///
/// @return 0 on success or non-zero on failure
//****************************************************************************
int test_appendTextToFile(char *text,
                          const char *targetFile)
{
    int rc = 0;

    FILE *target = fopen(targetFile, "a");

    if (target == NULL)
    {
        printf("ERROR: Failed to open target file '%s'\n", targetFile);
        rc = 1;
        goto mod_exit;
    }

    fprintf(target, "%s", text);

    fclose(target);

mod_exit:

    return rc;
}

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
                        ism_prop_t *properties)
{
    FILE * f;
    char * keyword;
    char * value;
    char * more;
    ism_field_t var = {0};

    f = fopen(configFile, "r");
    if (!f)
    {
        printf("ERROR: Config file '%s' not found\n", configFile);
        return -1;
    }

    char *line = NULL;
    size_t line_len;

    while(getline(&line, &line_len, f) != -1)
    {
        keyword = ism_common_getToken(line, " \t\r\n", " =\t\r\n", &more);
        if (keyword && keyword[0]!='*' && keyword[0]!='#') {
            value   = ism_common_getToken(more, " =\t\r\n", "\r\n", &more);
            if (!value)
                value = "";
            var.type = VT_String;
            var.val.s = value;
            ism_common_canonicalName(keyword);
            ism_common_setProperty(properties, keyword, &var);
        }
    }

    if (line != NULL) free(line);
    fclose(f);

    return 0;
}

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
char *test_getTestResourceFilePath(const char *fileName)
{
    char *testResourceFilePath = NULL;

    // Work out where to copy config files from
    char *sroot = getenv("SROOT");
    char *prjDir = getenv("PRJDIR");
    char *rootRel = getenv("ROOTREL");

    const char *testResourceDirectory = "server_engine/test/resources";

    if (prjDir && rootRel)
    {
        testResourceFilePath = malloc(strlen(prjDir)+1+
                                      strlen(rootRel)+1+
                                      strlen(testResourceDirectory)+1+
                                      strlen(fileName)+1);

        sprintf(testResourceFilePath, "%s/%s/%s/%s", prjDir, rootRel, testResourceDirectory, fileName);
    }
    else if (sroot)
    {
        testResourceFilePath = malloc(strlen(sroot)+1+
                                      strlen(testResourceDirectory)+1+
                                      strlen(fileName)+1);

        sprintf(testResourceFilePath, "%s/%s/%s", sroot, testResourceDirectory, fileName);
    }

    return testResourceFilePath;
}
