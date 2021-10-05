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
/// @file  test_utils_security.c
/// @brief Utility functions for testing security
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

ism_prop_t *policyProps[50] = {0};
void *policyContext[50] = {NULL};
int32_t policyCount = 0;

const char *includeProperties[] = {"ClientID",
                                   "UserID",
                                   "DestinationType",
                                   "Destination",
                                   "Topic",
                                   "Queue",
                                   "SubscriptionName",
                                   "ActionList",
                                   "MaxMessages",
                                   "MaxMessagesBehavior",
                                   "MaxMessageTimeToLive",
                                   "DisconnectedClientNotification",
                                   "UID",
                                   "DefaultSelectionRule"};

//****************************************************************************
/// @brief  Internal function to add a policy
//****************************************************************************
int test_fake_internal_addPolicy(char *newPolicyName,
                                 const char *policyType,
                                 bool update,
                                 ism_prop_t *configCallbackProps)
{
    int32_t rc = OK;
    int32_t i;
    const char *configPropertyName;
    ism_prop_t *newPolicyProps = ism_common_newProperties(ism_common_getPropertyCount(configCallbackProps)+1);

    // Remember what type of policy this one is
    ism_field_t f;
    f.type = VT_String;
    f.val.s = (char *)policyType;
    ism_common_setProperty(newPolicyProps, "TESTPolicyType", &f);

    for (i = 0; ism_common_getPropertyIndex(configCallbackProps, i, &configPropertyName) == 0; i++)
    {
        const char *stringValue = ism_common_getStringProperty(configCallbackProps, configPropertyName);
        char *propertyName = strchr(configPropertyName, '.');

        if (propertyName != NULL)
        {
            propertyName += 1;
            char *lastDot = strchr(propertyName, '.');

            if (lastDot != NULL)
            {
                f.val.s = (char *)stringValue;
                *lastDot = '\0';
                ism_common_setProperty(newPolicyProps, propertyName, &f);
                *lastDot = '.';
            }
        }

    }

    for(i=0; i<policyCount; i++)
    {
        const char *policyName = ism_common_getStringProperty(policyProps[i], "Name");

        if (newPolicyName != NULL && policyName != NULL &&
            strcmp(newPolicyName, policyName) == 0)
        {
            if (update == false && policyContext[i] != NULL)
            {
                ism_common_freeProperties(newPolicyProps);
                rc = ISMRC_Error;
            }
            else
            {
                ism_common_freeProperties(policyProps[i]);
                policyProps[i] = newPolicyProps;
            }
            break;
        }
    }

    if (rc == OK)
    {
        if (i == policyCount) policyProps[policyCount++] = newPolicyProps;

        char configCallbackPropName[128];

        // Make sure and pass the context back to the config callback
        f.type = VT_Long;
        f.val.l = (int64_t)policyContext[i];
        sprintf(configCallbackPropName, "%s.Context.%s", policyType, newPolicyName);
        ism_common_setProperty(configCallbackProps, configCallbackPropName, &f);

        // Force a callback of the engine...
        rc = ism_engine_configCallback((char *)policyType,
                                       newPolicyName,
                                       configCallbackProps,
                                       ISM_CONFIG_CHANGE_PROPS);
    }

    return rc;
}

//****************************************************************************
/// @brief  Fake Security callback for use by tests that interact with config
//****************************************************************************
int test_fake_security_configCallback(char * object,
                                      char * name,
                                      ism_prop_t * props,
                                      ism_ConfigChangeType_t flag)
{
    test_fake_internal_addPolicy(name, object, true, props);

    return ISMRC_OK;
}

//****************************************************************************
/// @brief  Fake ism_config_valDeleteEndpointObject function.
//****************************************************************************
int32_t ism_config_valDeleteEndpointObject(char *object, char *name)
{
    return ISMRC_OK;
}

//****************************************************************************
/// @brief  Fake ism_security_validate_policy function.
//****************************************************************************
int32_t ism_security_validate_policy(ismSecurity_t *secContext,
                                     ismSecurityAuthObjectType_t objectType,
                                     const char * objectName,
                                     ismSecurityAuthActionType_t actionType,
                                     ism_ConfigComponentType_t compType,
                                     void ** context)
{
    int32_t rc = ISMRC_NotAuthorized;
    int32_t i;

    for(i=0; i<policyCount; i++)
    {
        const char *checkValue;

        // Should only consider appropriate messaging policies
        if ((checkValue = ism_common_getStringProperty(policyProps[i], "TESTPolicyType")) == NULL) continue;

        const char *DestinationProp;

        switch(objectType)
        {
            case ismSEC_AUTH_TOPIC:
                if (strcasecmp(checkValue, ismENGINE_ADMIN_VALUE_TOPICPOLICY) != 0) continue;
                DestinationProp = "Topic";
                break;
            case ismSEC_AUTH_QUEUE:
                if (strcasecmp(checkValue, ismENGINE_ADMIN_VALUE_QUEUEPOLICY) != 0) continue;
                DestinationProp = "Queue";
                break;
            case ismSEC_AUTH_SUBSCRIPTION:
                if (strcasecmp(checkValue, ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY) != 0) continue;
                DestinationProp = "SubscriptionName";
                break;
            default:
                printf("Unhandled object type %d\n", objectType);
                rc = ISMRC_Error;
                break;
        }

        if (rc != ISMRC_NotAuthorized) break;

        // Now need to consider the requested action
        if ((checkValue = ism_common_getStringProperty(policyProps[i], "ActionList")) == NULL) continue;

        switch(actionType)
        {
            case ismSEC_AUTH_ACTION_SEND:
                if (strcasestr(checkValue, "send") == NULL) continue;
                break;
            case ismSEC_AUTH_ACTION_RECEIVE:
                if (strcasestr(checkValue, "receive") == NULL) continue;
                break;
            case ismSEC_AUTH_ACTION_BROWSE:
                if (strcasestr(checkValue, "browse") == NULL) continue;
                break;
            case ismSEC_AUTH_ACTION_PUBLISH:
                if (strcasestr(checkValue, "publish") == NULL) continue;
                break;
            case ismSEC_AUTH_ACTION_SUBSCRIBE:
                if (strcasestr(checkValue, "subscribe") == NULL) continue;
                break;
            case ismSEC_AUTH_ACTION_CONTROL:
                if (strcasestr(checkValue, "control") == NULL) continue;
                break;
            default:
                printf("Unhandled action type %d\n", actionType);
                rc = ISMRC_Error;
                break;
        }

        if (rc != ISMRC_NotAuthorized) break;

        // Check that the requested destination matches
        checkValue = ism_common_getStringProperty(policyProps[i], DestinationProp);

        if (checkValue != NULL &&
            ism_common_match(objectName, checkValue) == 0) continue;

        // Check that the clientID matches
        checkValue = ism_common_getStringProperty(policyProps[i], "ClientID");

        if (checkValue != NULL &&
            ism_common_match(((mock_ismSecurity_t *)secContext)->transport->clientID, checkValue) == 0) continue;

        // Check that the userId matches
        checkValue = ism_common_getStringProperty(policyProps[i], "UserID");

        if (checkValue != NULL &&
            ism_common_match(((mock_ismSecurity_t *)secContext)->transport->userid, checkValue) == 0) continue;

        // All matched... we are authorized
        rc = OK;

        if (rc != ISMRC_NotAuthorized) break;
    }

    // Give the engine context for the policy back to the caller
    if (rc == OK && context != NULL)
    {
        if (compType != ISM_CONFIG_COMP_ENGINE)
        {
            printf("Unexpected component type %d specified with non-null context pointer\n", compType);
            assert(false);
        }

        *context = policyContext[i];
    }

    return rc;
}

//****************************************************************************
/// @brief  Fake ism_security_context_getExpectedMsgRate function.
//****************************************************************************
XAPI ExpectedMsgRate_t ism_security_context_getExpectedMsgRate(ismSecurity_t *sContext) {
    if (sContext) {
        //It's actually a fake context;
        mock_ismSecurity_t *mockContext = (mock_ismSecurity_t *)sContext;
        return mockContext->ExpectedMsgRate;
    } else
        return EXPECTEDMSGRATE_UNDEFINED;
}

//****************************************************************************
/// @brief  Fake ism_security_context_getInflightMsgLimit function.
//****************************************************************************
/* 0 here is not indicative of an error */
XAPI uint32_t ism_security_context_getInflightMsgLimit(ismSecurity_t *sContext) {
    if (sContext) {
        //It's actually a fake context;
        mock_ismSecurity_t *mockContext = (mock_ismSecurity_t *)sContext;

        return mockContext->InFlightMsgLimit;
    } else
        return 0;
}

//****************************************************************************
/// @brief  Fake ism_security_context_getClientStateExpiry function.
//****************************************************************************
XAPI uint32_t ism_security_context_getClientStateExpiry(ismSecurity_t *sContext) {
    if (sContext) {
        //It's actually a fake context;
        mock_ismSecurity_t *mockContext = (mock_ismSecurity_t *)sContext;

        return mockContext->ClientStateExpiry;
    } else
        return iecsEXPIRY_INTERVAL_INFINITE;
}

//****************************************************************************
/// @brief  Fake ism_security_context_setClientStateExpiry function.
//****************************************************************************
XAPI void ism_security_context_setClientStateExpiry(ismSecurity_t *sContext, uint32_t newExpiry) {
    if (sContext) {
        //It's actually a fake context;
        mock_ismSecurity_t *mockContext = (mock_ismSecurity_t *)sContext;
        mockContext->ClientStateExpiry = newExpiry;
    }
}

//****************************************************************************

//****************************************************************************
/// @brief  Fake ism_security_set_policyContext function.
//****************************************************************************
XAPI int32_t ism_security_set_policyContext(const char *name,
                                            ismSecurityPolicyType_t policyType,
                                            ism_ConfigComponentType_t compType,
                                            void *newContext)
{
    int32_t rc = ISMRC_NotFound;

    for(int32_t i=0; i<policyCount; i++)
    {
        const char *checkValue = ism_common_getStringProperty(policyProps[i], ismENGINE_ADMIN_PROPERTY_NAME);

        if (checkValue && !strcmp(name, checkValue))
        {
            policyContext[i] = newContext;
            rc = ISMRC_OK;
            break;
        }
    }

    if (rc == ISMRC_NotFound)
    {
        // printf("IGNORING UNFOUND POLICY name '%s'\n", name);
        rc = OK;
    }
    return rc;
}

//****************************************************************************
/// @brief  Fake ism_security_addPolicy function.
//****************************************************************************
int32_t ism_security_addPolicy(const char *policyType, char *polstr)
{
    int32_t rc = OK;

    ism_json_parse_t parseObj = { 0 };
    ism_json_entry_t ents[20];

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseObj.source = (char *)strdup(polstr);
    parseObj.src_len = strlen(parseObj.source);

    rc = ism_json_parse(&parseObj);

    char *policyName;

    if (rc == OK)
    {
        policyName = (char *)ism_json_getString(&parseObj, "Name");
        if (policyName == NULL)
        {
            rc = ISMRC_InvalidParameter;
        }
    }

    if (rc == OK)
    {
        ism_field_t f;
        ism_prop_t *configCallbackProps = ism_common_newProperties(parseObj.ent_count);
        char configCallbackPropName[128];

        f.type = VT_String;

        f.val.s = policyName;
        if (f.val.s != NULL)
        {
            sprintf(configCallbackPropName, "%s.Name.%s", policyType, policyName);
            ism_common_setProperty(configCallbackProps, configCallbackPropName, &f);
        }

        for(int32_t i=0; i<sizeof(includeProperties)/sizeof(includeProperties[0]); i++)
        {
            f.val.s = (char *)ism_json_getString(&parseObj, includeProperties[i]);

            if (f.val.s != NULL)
            {
                sprintf(configCallbackPropName, "%s.%s.%s", policyType, includeProperties[i], policyName);
                ism_common_setProperty(configCallbackProps, configCallbackPropName, &f);
            }
        }

        bool update=false;

        const char *checkValue;

        if ((checkValue = ism_json_getString(&parseObj, "Update")) != NULL &&
            strcmp(checkValue, "true") == 0)
        {
            update = true;
        }

        test_fake_internal_addPolicy(policyName, policyType, update, configCallbackProps);

        ism_common_freeProperties(configCallbackProps);
    }

    if (parseObj.source != NULL) free(parseObj.source);

    return rc;
}

//****************************************************************************
/// @brief  Release memory used by fake security routines
///
/// @param[in]     enabled  Whether fake validation routines should be enabled
//****************************************************************************
void test_freeFakeSecurityData(void)
{
    for(int32_t i=0; i<policyCount; i++)
    {
        ism_common_freeProperties(policyProps[i]);
        policyProps[i] = NULL;
        policyContext[i] = NULL;
    }

    policyCount = 0;
}

//****************************************************************************
/// @brief  Save the fake security configuration to a props array
///
/// @param[in,out] props  Array of 50 properties to store the result
//****************************************************************************
void test_saveFakeSecurityData(ism_prop_t **props)
{
    int32_t i;

    for(i=0; i<policyCount; i++)
    {
        props[i] = policyProps[i];
        policyProps[i] = NULL;
    }

    for(; i<50; i++)
    {
        props[i] = NULL;
    }
}

//****************************************************************************
/// @brief  Load the fake security configuration from a saved props array
///
/// @param[in]     props  Array of 50 properties to load
//****************************************************************************
void test_loadFakeSecurityData(ism_prop_t **props)
{
    for(int32_t i=0; i<50; i++)
    {
        policyProps[i] = props[i];

        if (props[i] != NULL)
        {
            policyCount = i+1;

            const char *name = ism_common_getStringProperty(props[i], "Name");

            if (name)
            {
                ism_field_t f;
                ism_prop_t *configCallbackProps = ism_common_newProperties(20);
                char configCallbackPropName[128];
                const char *policyType = ism_common_getStringProperty(props[i], "TESTPolicyType");

                if (policyType == NULL) abort();

                // Set context to NULL
                f.type = VT_Long;
                f.val.l = 0;
                sprintf(configCallbackPropName, "%s.Context.%s", policyType, name);
                ism_common_setProperty(configCallbackProps, configCallbackPropName, &f);

                const char *propertyName;
                for (int32_t j = 0; ism_common_getPropertyIndex(props[i], j, &propertyName) == 0; j++)
                {
                    f.type = VT_String;
                    f.val.s = (char *)ism_common_getStringProperty(props[i], propertyName);

                    sprintf(configCallbackPropName, "%s.%s.%s", policyType, propertyName, name);
                    ism_common_setProperty(configCallbackProps, configCallbackPropName, &f);
                }

                ism_engine_configCallback((char *)policyType,
                                          (char *)name,
                                          configCallbackProps,
                                          ISM_CONFIG_CHANGE_PROPS);

                ism_common_freeProperties(configCallbackProps);
            }
        }
    }
}

//****************************************************************************
/// @internal
///
/// @brief  Create a mock-up of a connection security context for the engine
///
/// @param[in,out] ppListener    Address of pointer to listener to use / create
/// @param[in,out] ppTransport   Address of pointer to transport to use / create
/// @param[out]    ppSecContext  Address of pointer to receive context
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_createSecurityContext(ism_listener_t **ppListener,
                                   ism_transport_t **ppTransport,
                                   ismSecurity_t **ppSecContext)
{
    int32_t rc = OK;

    if ((*ppListener = calloc(1, sizeof(ism_listener_t))) == NULL)
    {
        rc = ISMRC_AllocateError;
    }
    else if ((*ppTransport = calloc(1, sizeof(ism_transport_t))) == NULL)
    {
        rc = ISMRC_AllocateError;
    }
    else
    {
        (*ppTransport)->listener =  *ppListener;

        if ((*ppSecContext = calloc(1, sizeof(mock_ismSecurity_t))) == NULL)
        {
            rc = ISMRC_AllocateError;
        }
        else
        {
            ((mock_ismSecurity_t *)(*ppSecContext))->transport = *ppTransport;
            ((mock_ismSecurity_t *)(*ppSecContext))->ClientStateExpiry = iecsEXPIRY_INTERVAL_INFINITE;
        }
    }

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Add a security policy to the security component
///
/// @param[in]     policyType    One of
///                                 TopicPolicy
///                                 QueuePolicy
///                                 SubscriptionPolicy
/// @param[in]     inputBuffer   JSON formatted character string representing policy
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_addSecurityPolicy(char *policyType, char *inputBuffer)
{
    int32_t rc = OK;

    char *dupBuffer = strdup(inputBuffer); // security modifies data, so can't be const

    if (dupBuffer == NULL)
    {
        printf("ERROR: Unable to duplicate inputBuffer\n");
        rc = ISMRC_AllocateError;
        goto mod_exit;
    }

    // NOTE: OVERLOADING FILE PARAMETER with policyType
    rc = ism_security_addPolicy(policyType, dupBuffer);

    if (rc != OK)
    {
        printf("POLICY '%s'\n", dupBuffer);
        printf("ERROR: ism_security_addPolicy() returned %d\n", rc);
        goto mod_exit;
    }

mod_exit:
    if (dupBuffer != NULL) free(dupBuffer);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Fake up admin deletion of policy
///
/// @param[in]     inputBuffer   JSON formatted character string representing policy
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_deleteSecurityPolicy(char *UID, char *Name)
{
    int32_t rc;

    // Find the context for this policy, set it in the properties and then forget it
    ism_field_t f;
    ism_prop_t *configCallbackProps = ism_common_newProperties(3);
    char configCallbackPropName[128];
    void **deletedContext = NULL;
    const char *policyType = NULL;

    for(int32_t i=0; i<policyCount; i++)
    {
        const char *policyUID = ism_common_getStringProperty(policyProps[i], "UID");

        if (policyUID != NULL && strcmp(UID, policyUID) == 0)
        {
            policyType = ism_common_getStringProperty(policyProps[i], "TESTPolicyType");
            if (policyType == NULL) abort();

            f.type = VT_Long;
            f.val.l = (int64_t)policyContext[i];
            sprintf(configCallbackPropName, "%s.Context.%s", policyType, Name);
            ism_common_setProperty(configCallbackProps, configCallbackPropName, &f);
            deletedContext = &policyContext[i];
            break;
        }
    }

    if (policyType == NULL) abort();

    f.type = VT_String;

    f.val.s = Name;
    sprintf(configCallbackPropName, "%s.Name.%s", policyType, Name);
    ism_common_setProperty(configCallbackProps, configCallbackPropName, &f);

    f.val.s = UID;
    sprintf(configCallbackPropName, "%s.UID.%s", policyType, Name);
    ism_common_setProperty(configCallbackProps, configCallbackPropName, &f);

    // Force a callback of the engine...
    rc = ism_engine_configCallback((char *)policyType,
                                   Name,
                                   configCallbackProps,
                                   ISM_CONFIG_CHANGE_DELETE);

    ism_common_freeProperties(configCallbackProps);

    // Not in-use, engine deleted the policy
    if (rc == OK)
    {
        if (deletedContext != NULL) *deletedContext = NULL;
    }

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Add a named messaging policy to the endpoint associated with this
///         security context.
///
/// @param[in,out] pSecContext  The security context to update
/// @param[in]     policy       The name of the policy to add
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_addPolicyToSecContext(ismSecurity_t *pSecContext, char *policy)
{
    return OK;
}

//****************************************************************************
/// @internal
///
/// @brief  Destroy the mock-up of a security context, and any other storage
//          used for it.
///
/// @param[in]     pListener    Pointer to previously created listener
/// @param[in]     pTransport   Pointer to previously created transport
/// @param[in]     pSecContext  Pointer to previously created context
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_destroySecurityContext(ism_listener_t *pListener,
                                    ism_transport_t *pTransport,
                                    ismSecurity_t *pSecContext)
{
    int32_t rc = OK;

    if (pListener != NULL) free(pListener);
    if (pTransport != NULL) free(pTransport);
    if (pSecContext != NULL) free(pSecContext);

    return rc;
}
