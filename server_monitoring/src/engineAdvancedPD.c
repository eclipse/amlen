/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

#define TRACE_COMP Monitoring

#include "engineAdvancedPD.h"

extern pthread_key_t monitoring_localekey;

/**
 * Returns action type from action string
 */
static int ismmon_getActionType(char *action) {
    if ( !action || (action && *action == '\0'))
        return ISMMON_ENGINE_ADVPD_ACTION_NONE;

    if ( !strcasecmp(action, "dumptopic")) {
        return ISMMON_ENGINE_ADVPD_ACTION_DUMPTOPIC;
    } else if ( !strcasecmp(action, "dumptopictree")) {
        return ISMMON_ENGINE_ADVPD_ACTION_DUMPTOPICTREE;
    } else if ( !strcasecmp(action, "dumpqueue")) {
        return ISMMON_ENGINE_ADVPD_ACTION_DUMPQUEUE;
    } else if ( !strcasecmp(action, "dumpclient")) {
        return ISMMON_ENGINE_ADVPD_ACTION_DUMPCLIENT;
    } else if ( !strcasecmp(action, "dumplocks")) {
        return ISMMON_ENGINE_ADVPD_ACTION_DUMPLOCKS;
    }

    return ISMMON_ENGINE_ADVPD_ACTION_NONE;
}

XAPI int32_t ism_monitoring_getAdvancedEnginePDData(
    char               * action,
    ism_json_parse_t   * inputJSONObj,
    concat_alloc_t     * output_buffer )
{
    int32_t rc = ISMRC_OK;
    char rbuf[5000];

    int actionType = ismmon_getActionType(action);

    if ( actionType == ISMMON_ENGINE_ADVPD_ACTION_NONE ) {
        sprintf(rbuf, "Invalid or NULL Action Type received: %s\n", action? action:"");
        ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
        return ISMRC_Error;
    }

    char *objectName = (char *)ism_json_getString(inputJSONObj, "ObjectName");
    char *filePath = (char *)ism_json_getString(inputJSONObj, "FilePath");
    /* All require a filePath, and DumpTopic requires an objectName */
    if(filePath && (actionType != ISMMON_ENGINE_ADVPD_ACTION_DUMPTOPIC || objectName)) {
        TRACE(7,"ism_monitoring_getAdvancedEnginePDData: action=%s  objectName=%s filePath=%s\n",
              action, objectName ? objectName : "", filePath);

        char resultPath[strlen(filePath)+strlen(".partial")+1]; // Allow for possibly asynchronous filename

        strcpy(resultPath, filePath);

        int32_t detailLevel = ism_json_getInt(inputJSONObj, "Level", 5);
        int64_t userDataBytes = ism_json_getInt(inputJSONObj, "UserData", 0); // Default to NO user data

        /*
         * Call the appropriate engine routine based on the action type
         */
        if (actionType == ISMMON_ENGINE_ADVPD_ACTION_DUMPCLIENT) {
            rc = ism_engine_dumpClientState(objectName, detailLevel, userDataBytes, resultPath);
        } else if (actionType == ISMMON_ENGINE_ADVPD_ACTION_DUMPTOPIC) {
            rc = ism_engine_dumpTopic(objectName, detailLevel, userDataBytes, resultPath);
        } else if (actionType == ISMMON_ENGINE_ADVPD_ACTION_DUMPTOPICTREE) {
            rc = ism_engine_dumpTopicTree(objectName, detailLevel, userDataBytes, resultPath);
        } else if (actionType == ISMMON_ENGINE_ADVPD_ACTION_DUMPQUEUE) {
            rc = ism_engine_dumpQueue(objectName, detailLevel, userDataBytes, resultPath);
        } else if (actionType == ISMMON_ENGINE_ADVPD_ACTION_DUMPLOCKS) {
            rc = ism_engine_dumpLocks(objectName, detailLevel, userDataBytes, resultPath);
        }

        /*
         * For a good return code, return the result as a JSON string, ResultPath will be FilePath.partial
         * if the dump file is being produced asynchronously, and FilePath.dat if synchronous.
         */
        if (rc == ISMRC_OK) {
            sprintf(rbuf, "{ \"Action\":\"%s\", \"ObjectName\":\"%s\", \"FilePath\":\"%s\", \"Level\":\"%d\", \"ResultPath\":\"%s\" }",
                    action, objectName ? objectName : "", filePath, detailLevel, resultPath);
        } else {
            char buf[256];
            char *errstr = NULL;
            errstr = (char *)ism_common_getErrorStringByLocale(rc, ism_common_getRequestLocale(monitoring_localekey), buf, 256);
            sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"%s\" }", rc, errstr ? errstr : "Unknown");
        }

        ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
    } else {
    	if (!objectName && !filePath)
            sprintf(rbuf, "NULL objectName and NULL filePath are received\n");
    	else if (!objectName)
    		sprintf(rbuf, "NULL objectName is received\n");
    	else
    		sprintf(rbuf, "NULL filePath is received\n");
        ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
        rc = ISMRC_Error;
    }

    return rc;
}
