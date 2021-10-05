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
/// @file  test_utils_config.c
/// @brief Utility functions for testing config
//****************************************************************************
#include <string.h>

#include "ismutil.h"
#include "ismjson.h"
#include "engine.h"
#include "engineInternal.h"
#include "store.h"
#include "security.h"
#include "transport.h"
#include "admin.h"

#include "test_utils_security.h"

// Allow addition and removal of properties from getPropertiesDynamic call
pthread_mutex_t propsLock = PTHREAD_MUTEX_INITIALIZER;
ism_prop_t *addDynamicProps = NULL;
ism_prop_t *removeDynamicProps = NULL;

//****************************************************************************
/// @brief  Override ism_config_getPropertiesDynamic function.
//****************************************************************************
XAPI ism_prop_t * ism_config_getPropertiesDynamic( ism_config_t * handle,
                                                   const char * object,
                                                   const char * name )
{
    static ism_prop_t * (*real_ism_config_getPropertiesDynamic)(ism_config_t *, const char *, const char *) = NULL;

    if (real_ism_config_getPropertiesDynamic == NULL)
    {
        real_ism_config_getPropertiesDynamic = dlsym(RTLD_NEXT, "ism_config_getPropertiesDynamic");
        assert(real_ism_config_getPropertiesDynamic != NULL);
    }

    ism_prop_t *retprops = real_ism_config_getPropertiesDynamic(handle, object, name);

    DEBUG_ONLY int32_t rc;
    const char *propertyName = NULL;

    pthread_mutex_lock(&propsLock);

    // Add any requested dynamic properties
    if (addDynamicProps != NULL)
    {
        // Make sure we have some properties to work with
        if (retprops == NULL) retprops = ism_common_newProperties(10);

        // Loop through the policies in the configuration ensuring we have an up-to-date
        // view of it.
        for (int32_t i = 0; ism_common_getPropertyIndex(addDynamicProps, i, &propertyName) == 0; i++)
        {
            ism_field_t f = {0};

            rc = ism_common_getProperty(addDynamicProps, propertyName, &f);
            assert(rc == OK);

            rc = ism_common_setProperty(retprops, propertyName, &f);
            assert(rc == OK);
        }
    }

    if (retprops != NULL && removeDynamicProps != NULL)
    {
        // Loop through the policies in the configuration ensuring we have an up-to-date
        // view of it.
        for (int32_t i = 0; ism_common_getPropertyIndex(removeDynamicProps, i, &propertyName) == 0; i++)
        {
            rc = ism_common_setProperty(retprops, propertyName, NULL);
            assert(rc == OK);
        }
    }

    pthread_mutex_unlock(&propsLock);

    return retprops;
}

//****************************************************************************
/// @brief  Provide properties to add to and remove from the dynamic properties
///         returned by ism_config_getPropertiesDynamic.
///
/// @param[in]     additions  ism_prop_t containing additional properties
/// @param[in]     removals   ism_prop_t whose named properties will be removed
//****************************************************************************
void test_setConfigDynamicPropsChanges(ism_prop_t *additons, ism_prop_t *removals)
{
    pthread_mutex_lock(&propsLock);
    addDynamicProps = additons;
    removeDynamicProps = removals;
    pthread_mutex_unlock(&propsLock);
    return;
}

// Get the result of the RESTAPI call into our http structure
void test_config_get_restapi_result(ism_http_t *http, int retcode)
{
    if (http->outbuf.used != 0 && getenv("TEST_IMA_SHOW_REST_RESPONSES") != NULL)
    {
        printf("REST call returned: '%.*s'\n", http->outbuf.used, http->outbuf.buf);
    }

    http->val1 = (uint32_t)retcode;
}

//****************************************************************************
/// @internal
///
/// @brief  Make a call to ism_config_json_processPost with an input JSON string
///
/// @param[in]     inputJSON    JSON string to be parsed and passed to config
///
/// @remark If the JSON string need not include the 'Action' or 'User' fields,
///         the Action is assumed as 'set' and user is only used by the admin
///         component that wrappers this function.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_configProcessPost(const char *inputJSON)
{
    int32_t rc = ISMRC_Error;

    ism_http_t http = {0};
    ism_http_content_t http_content = {0};
    char http_outbuf[1024];
    char *contentBuffer = strdup(inputJSON);

    http_content.content = contentBuffer;
    http_content.content_len = strlen(contentBuffer);

    http.content_count = 1;
    http.content = &http_content;
    http.outbuf.buf = http_outbuf;
    http.outbuf.len = sizeof(http_outbuf);

    /* Accept license before invoking API to process REST call */
    ism_admin_cunit_acceptLicense();
    (void)ism_config_json_processPost(&http, test_config_get_restapi_result);
    rc = (int32_t)(http.val1);

    free(contentBuffer);

    if (http.outbuf.inheap) ism_common_freeAllocBuffer(&http.outbuf);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Make a call to ism_config_json_processDelete for an object
///
/// @param[in]     objType      Type of object being referred to
/// @param[in]     objName      Name of object being referred to
/// @param[in]     queryParams  Parameters in key=value format
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_configProcessDelete(const char *objType, const char *objName, const char *queryParams)
{
    int32_t rc = ISMRC_Error;

    ism_http_t http = {0};
    char http_outbuf[1024];
    char http_user_path[strlen(objType)+strlen(objName)+2];
    char http_query[(queryParams ? strlen(queryParams) : 0)+1];

    http.outbuf.buf = http_outbuf;
    http.outbuf.len = sizeof(http_outbuf);
    strcpy(http.version, SERVER_SCHEMA_VERSION);

    sprintf(http_user_path, "%s/%s", objType, objName);
    http.user_path = http_user_path;

    if (queryParams != NULL)
    {
        strcpy(http_query, queryParams);
        http.query = http_query;
    }

    int serviceCall;

    // If the ObjType is one that should come in over a service call, we
    // need to tell the deleteAction function (via the parameter it calls
    // pflag)... The code that enforces this is ism_config_restapi_deleteAction
    // in config_restapi.c
    if (strcmp(objType, "ClientSet") == 0 ||
        strcmp(objType, "import") == 0 ||
        strcmp(objType, "export") == 0 ||
        strcmp(objType, "MQTTClient") == 0 ||
        strcmp(objType, "Subscription") == 0)
    {
        serviceCall = 1;
    }
    else
    {
        serviceCall = 0;
    }
    ism_admin_cunit_acceptLicense();
    (void)ism_config_restapi_deleteAction(&http, serviceCall, test_config_get_restapi_result);
    rc = (int32_t)(http.val1);

    if (http.outbuf.inheap) ism_common_freeAllocBuffer(&http.outbuf);

    return rc;
}
