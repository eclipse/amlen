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
/// @file  test_utils_monitoring.c
/// @brief Utility functions for testing monitoring
//****************************************************************************
#include "ismutil.h"
#include "ismjson.h"
#include "engine.h"
#include "engineInternal.h"
#include "test_utils_monitoring.h"

void *libismmonitoring = NULL;

//****************************************************************************
/// @brief  Make a call to ism_monitoring_getEngineStats with an input JSON string
///
/// @param[in]     action       Which engine stat action is required,
///                             "Subscription", "Topic", "Queue" or "MQTTClient"
/// @param[in]     inputJSON    JSON string to be parsed and passed to
///                             ism_monitoring_getEngineStats
/// @param[out]    outputBuffer output buffer for the results of the request
///
/// @remark If the JSON string need not include the 'Action' or 'User' fields,
///         the Action is assumed as 'set' and user is only used by the admin
///         component that wrappers this function.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_getEngineStats(char *action,
                            const char *inputJSON,
                            concat_alloc_t *outputBuffer)
{
    int32_t rc = OK;

    ism_json_parse_t parseObj = { 0 };
    ism_json_entry_t ents[20];

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseObj.source = (char *)strdup(inputJSON);
    parseObj.src_len = strlen(parseObj.source);

    rc = ism_json_parse(&parseObj);

    if (rc != OK)
    {
        printf("ERROR: ism_json_parse() for inputJSON returned %d\n", rc);
        goto mod_exit;
    }

    if (libismmonitoring == NULL)
    {
        libismmonitoring = dlopen("libismmonitoring.so", RTLD_LAZY | RTLD_GLOBAL);
    }

    if (libismmonitoring != NULL)
    {
        int (*ism_monitoring_getEngineStats)(char *, ism_json_parse_t *, concat_alloc_t *) = dlsym(libismmonitoring, "ism_monitoring_getEngineStats");

        if (ism_monitoring_getEngineStats != NULL)
        {
            rc = ism_monitoring_getEngineStats(action, &parseObj, outputBuffer);
        }
        else
        {
            rc = ISMRC_NotFound;
        }
    }
    else
    {
        rc = ISMRC_NotFound;
    }

mod_exit:

    if (parseObj.source != NULL)
    {
        free(parseObj.source);
    }

    return rc;
}
