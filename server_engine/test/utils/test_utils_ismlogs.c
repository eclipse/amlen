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
/// @file  test_utils_ismlogs.c
/// @brief Utilities to examine ism log files
//****************************************************************************
#include <sys/stat.h>
#include <stdlib.h>
#include <assert.h>

#include "engine.h"
#include "ismutil.h"
#define DEFINE_LOGMESSAGE_COUNTERS
#include "test_utils_ismlogs.h"

char globalLogDir[4096] = "";

//****************************************************************************
/// @brief  Set the directory to which log files should be written.
///
/// @param[in]     logDir       The directory to which log files are being
///                             written.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_setLogDir(char *logDir)
{
    strcpy(globalLogDir, logDir);

    return OK;
}

//****************************************************************************
/// @brief  Override ism_log_createLogger so we can grab the log files.
//****************************************************************************
XAPI int ism_log_createLogger(int which, ism_prop_t * props)
{
    static int (*real_ism_log_createLogger)(int, ism_prop_t *) = NULL;

    if (real_ism_log_createLogger == NULL)
    {
        real_ism_log_createLogger = dlsym(RTLD_NEXT, __func__);
        if (real_ism_log_createLogger == NULL) abort();
    }

    const char *checkPropertyName;

    switch(which)
    {
        case 0:
            checkPropertyName = "LogLocation.Destination.DefaultLog";
            break;
        case 1:
            checkPropertyName = "LogLocation.Destination.ConnectionLog";
            break;
        case 2:
            checkPropertyName = "LogLocation.Destination.SecurityLog";
            break;
        case 3:
            checkPropertyName = "LogLocation.Destination.AdminLog";
            break;
        case 4:
            checkPropertyName = "LogLocation.Destination.MQConnectivityLog";
            break;
        default:
            abort();
            break;
    }


    ism_prop_t *globalProps = ism_common_getConfigProperties();
    ism_field_t f;

    // If we relocated the log file - pass in the globalProps (with the
    // relocation in).
    if (ism_common_getProperty(globalProps, checkPropertyName, &f) == 0)
    {
        return(real_ism_log_createLogger(which, globalProps));
    }
    // Otherwise, call it as requested
    else
    {
        return(real_ism_log_createLogger(which, props));
    }
}

//****************************************************************************
/// @brief  Tell the server that the log file specified should be relocated
///         to the globally overridden config directory /logs.
///
/// @param[in]     logfileType  The type of log file being relocated
/// @param[in]     logLevel     The requested logging level.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_relocateLogFile(char *logfileType, char *logLevel)
{
    int32_t rc;
    char tempFilename[sizeof(globalLogDir)+strlen(logfileType)+6];

    if (globalLogDir[0] == '\0')
    {
        printf("ERROR: Global log directory not set\n");
        rc = ISMRC_InvalidValue;
        goto mod_exit;
    }

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();
    ism_field_t f;

    char propertyName[100];

    sprintf(tempFilename, "%s/%s.log", globalLogDir, logfileType);
    bool defaultLog = (bool)(strcmp(logfileType, "System") == 0);

    sprintf(propertyName, "LogLocation.Destination.%sLog", defaultLog ? "Default" : logfileType);
    f.type = VT_String;
    f.val.s = tempFilename;
    rc = ism_common_setProperty(staticConfigProps, propertyName, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s returned %d\n", propertyName, rc);
        goto mod_exit;
    }

    if (defaultLog)
    {
        sprintf(propertyName, "LogLevel");
    }
    else
    {
        sprintf(propertyName, "%sLog", logfileType);
    }
    f.type = VT_String;
    f.val.s = logLevel;
    rc = ism_common_setProperty(staticConfigProps, propertyName, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s returned %d\n", propertyName, rc);
        goto mod_exit;
    }

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Find the expected number of instances of text in the specified file
///
/// @param[in]     msgId        The message to find
/// @param[out]    foundCount   The count of instances found
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_findLogMessages(int msgId, int32_t *foundCount)
{
    int32_t rc = OK;
    int32_t found = 0;

    for(int32_t loop=0; loop<ismLogMsgsSeenCount; loop++)
    {
        if (ismLogMsgsSeen[loop] == msgId) found++;
    }

    (*foundCount) = found;

    return rc;
}
