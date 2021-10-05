/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

#define TRACE_COMP Admin


#include <janssonConfig.h>
#include "configInternal.h"
#include "validateInternal.h"

extern int revalidateEndpointForCRL;
/*
 * Item: AdminEndpoint
 *
 * Description:
 * - AdminEndpoint is pre-configured named object
 * - Name of the AdminEndpoint is AdminEndpoint
 * - User can only update pre-configured AdminEndpoint
 * - AdminEndpoint can not be deleted
 *
 * Schema:
 *
 *   {
 *       "AdminEndpoint": {
 *           "Description":"string",
 *           "Interface":"string",
 *           "Port":"number",
 *           "SecurityProfile":"string",
 *           "ConfigurationPolicies":"string"
 *       }
 *   }
 *
 * Where:
 *
 * Description: Description of configuration item - user can not set Description
 * Interface: IP address, All and * allowed.
 * Port: Integer, range 1-65535
 * SecurityProfile: String - Security Profile is optional.
 *                           If LTPA and OAuth is set in the SecurityProfile, it will be ignored
 *                           for authenticating a user using LTPA or OAuth.
 * ConfigurationPolicies: Comma-separated list of ConfigurationPolicy. This item is optional.
 *
 * If SecurityProfile is set, then ConfigurationPolicies is a required item.
 * If ConfigurationPolicies is set, then SecurityProfile is a required item.
 *
 *
 * Component callback(s):
 * - Transport
 *
 */

XAPI int32_t ism_config_validate_AdminEndpoint(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value = NULL;
    int objType = 0;

    TRACE(9, "Entry %s: currPostObj:%p, mergedObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, mergedObj?mergedObj:0, item?item:"null", name?name:"null", action);

    if ( !mergedObj || !props || !name ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Validate name - name can only be AdminEndpoint */
    if ( name && strcmp(name, "AdminEndpoint") ) {
        rc = ISMRC_BadOptionValue;
        ism_common_setErrorData(rc, "%s%s%s", "AdminEndpoint", "Name", name);
        goto VALIDATION_END;
    }

    /* In current post, check AdminEndpoint delete is requested - it is not allowed */
    json_t *aepObj = json_object_get(currPostObj, "AdminEndpoint");
    if ( !aepObj ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }
    if ( aepObj && json_typeof(aepObj) == JSON_NULL ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "AdminEndpoint");
        goto VALIDATION_END;
    }

    /* check for JSON_OBJECT */
    objType = json_typeof(aepObj);
    if ( objType != JSON_OBJECT ) {
        rc = ISMRC_PropertiesNotValid;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }


    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Validate for one instance only and optional object name (name should be AdminEndpoint) */
    json_object_foreach(mergedObj, key, value) {
        if (json_typeof(value) == JSON_OBJECT) {
            /* size of mergedObj can not be more than 1 */
            if ( json_object_size(mergedObj) > 1 ) {
                /* Only one instance of AdminEndpoint is allowed */
                rc = ISMRC_TooManyItems;
                ism_common_setErrorData(ISMRC_TooManyItems, "%s%d%d", "AdminEndpoint", 2, 1);
                goto VALIDATION_END;

            }
        }
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(mergedObj) == JSON_NULL || json_object_size(mergedObj) == 0 ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "AdminEndpoint");
        goto VALIDATION_END;
    }

    /* Validate configuration items */
    int  dataType;
    xUNUSED int  secProfEmpty = -1; /* 0-null, 1-empty, 2-not-empty */
    xUNUSED int  conPolEmpty = -1;  /* 0-null, 1-empty, 2-not-empty */

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);
        objType = json_typeof(value);

        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;

        if ( !strcmp(key, "Port")) {
            itemIter = json_object_iter_next(mergedObj, itemIter);
            continue;
        }

        if ( !strcmp(key, "Interface")) {
            itemIter = json_object_iter_next(mergedObj, itemIter);
            continue;
        }

        /* Validate ConfigurationPolicies */
        if ( !strcmp(key, "ConfigurationPolicies")) {
            if ( json_typeof(value) == JSON_NULL ) {
                conPolEmpty = 0;
            } else {
                char *propValue = (char *)json_string_value(value);

                if (propValue) {
                    if ( *propValue == '\0' ) {
                        json_object_set(mergedObj, key, json_null());
                        conPolEmpty = 1;
                    } else {
                        conPolEmpty = 2;
                        /* Could be a comma-separated list */
                        char * token;
                        char * nexttoken = NULL;
                        int len = strlen(propValue);
                        char *option = alloca(len + 1);
                        memcpy(option, propValue, len);
                        option[len] = '\0';
                        int count = 0;

                        /* Remove leading and trailing spaces around comma */
                        ism_config_fixCommaList(mergedObj, key, option);

                        for (token = strtok_r(option, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken))
                        {
                            /* check if policy exist */
                            int found = ism_config_objectExist("ConfigurationPolicy", token, currPostObj);
                            if ( found == 0 ) {
                                rc = ISMRC_ObjectNotFound;
                                ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "ConfigurationPolicy", token);
                                goto VALIDATION_END;
                            }
                            count += 1;
                        }

                        if ( count > MAX_POLICIES_PER_SECURITY_CONTEXT ) {
                            rc = ISMRC_TooManyItems;
                            ism_common_setErrorData(ISMRC_TooManyItems, "%s%d%d", "ConfigurationPolicies", count, MAX_POLICIES_PER_SECURITY_CONTEXT);
                            goto VALIDATION_END;
                        }
                        if ( count == 0 ) conPolEmpty = 1;
                    }
                }
            }

            itemIter = json_object_iter_next(mergedObj, itemIter);
            continue;
        }

        /* Validate SecurityProfile */
        if ( !strcmp(key, "SecurityProfile")) {
            /* Get value of SecurityProfile */
            if ( json_typeof(value) == JSON_NULL ) {
                secProfEmpty = 0;
            } else {

                char *propValue = (char *)json_string_value(value);

                if ( propValue ) {
                    if ( *propValue == '\0' ) {
                        secProfEmpty = 1;
                    } else {
                        secProfEmpty = 2;
                        /* Check if security profile exist and get usePasswordAuthentication item configured in the SecurityProfile */
                        value = NULL;

                        int found = ism_config_objectExist("SecurityProfile", propValue, currPostObj);
                        if ( found == 0 ) {
                            rc = ISMRC_ObjectNotFound;
                            ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "SecurityProfile", propValue);
                            goto VALIDATION_END;
                        }

                        /*
                         * If usePasswordAuthentication is enbled in the used security profile,
                         * then it is required that either AdminUserID and AdminUserPassword is set, or
                         * an external LDAP is configured.
                         */
                        value = ism_config_json_getMergedObject("SecurityProfile", propValue, "UsePasswordAuthentication", currPostObj, &dataType);
                        if ( value ) {
                            char *usePWAuth = NULL;
                            if ( dataType == JSON_STRING ) {
                                usePWAuth = (char *)json_string_value(value);
                            }

                            if ( usePWAuth && !strcmpi(usePWAuth, "true")) {
                                /* check if AdminUserID and AdminUserPassword is set or external LDAP is configured */
                                json_t *adminUserObj = ism_config_json_getMergedObject("AdminUserID", NULL, NULL, currPostObj, &dataType);
                                json_t *adminPassObj = ism_config_json_getMergedObject("AdminUserPassword", NULL, NULL, currPostObj, &dataType);

                                if (!adminUserObj || !adminPassObj ) {
                                    /* check if external LDAP is enabled */
                                    value = ism_config_json_getMergedObject("LDAP", "ldapconfig", "Enabled", currPostObj, &dataType);
                                    if ( value == NULL ) {
                                        rc = ISMRC_AdminUserRequired;
                                        ism_common_setError(ISMRC_AdminUserRequired);
                                        goto VALIDATION_END;
                                    }
                                    int ldapEnabled = json_is_true(value);
                                    if ( !ldapEnabled ) {
                                        rc = ISMRC_AdminUserRequired;
                                        ism_common_setError(ISMRC_AdminUserRequired);
                                        goto VALIDATION_END;
                                    }
                                }
                            }
                        }

                        /* Has security profile changed ? We also need to know if we need to revalidate endpoint using this security profile
                         * because of the CRL change. The revalidation is called after the new security context has been updated in the callback to
                         * transport component. We do not check if the source of the CRL is a url or filename here. */

                        char * crlProfStr = NULL;
                        json_t * oldSecProfObj = NULL;
                        char * oldSecProfStr = NULL;

                        /* Get the current security profile of object - can't use mergedObj */
                        oldSecProfObj = ism_config_json_getObject("AdminEndpoint", name, "SecurityProfile", 0, &dataType);
                        if (oldSecProfObj) {
                            oldSecProfStr = (char *) json_string_value(oldSecProfObj);
                        }

                        if (oldSecProfStr && (*oldSecProfStr != '\0')) {
                            /* same security profile don't bother */
                            if (!strcmp(oldSecProfStr, propValue)) {
                                goto NEXT_KEY;
                            }
                        }
                        /* Switching security profiles or endpoint being set with a new one */
                        value = NULL;
                        value = ism_config_json_getObject("SecurityProfile", propValue, "CRLProfile", 0, &dataType);
                        if (value) {
                            crlProfStr = (char *) json_string_value(value);
                            if (crlProfStr && (*crlProfStr != '\0')) {
                                (void) ism_config_json_getObject("CRLProfile", crlProfStr, "RevalidateConnection", 0, &dataType);
                                if (dataType == JSON_TRUE)
                                    revalidateEndpointForCRL = 1;
                            }
                        }
                    }
                }
            }
NEXT_KEY:
            itemIter = json_object_iter_next(mergedObj, itemIter);
            continue;
        }

        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* more special validation logical tied with this object related to ConfigurationPolicies and SecurityProfile */
    /*if ( (conPolEmpty == 0 && secProfEmpty == 0 ) || ( conPolEmpty == 1 && secProfEmpty == 1 ) || ( conPolEmpty == 2 && secProfEmpty == 2 )) {
        rc = ISMRC_OK;
    } else if ( ( conPolEmpty == 0 && secProfEmpty == 1 ) || ( conPolEmpty == 1 && secProfEmpty == 0 ) ) {
        rc = ISMRC_NeedMoreConfigItems;
        if ( secProfEmpty == 1 ) {
            ism_common_setErrorData(rc, "%s%s", "ConfigurationPolicies", "SecurityProfile");
        } else {
            ism_common_setErrorData(rc, "%s%s", "SecurityProfile", "ConfigurationPolicies");
        }
        goto VALIDATION_END;
    } else if ( ( conPolEmpty == 1 && secProfEmpty == 2 ) || ( conPolEmpty == 2 && secProfEmpty == 1 ) ) {
        rc = ISMRC_NeedMoreConfigItems;
        if ( secProfEmpty == 2 ) {
            ism_common_setErrorData(rc, "%s%s", "ConfigurationPolicies", "SecurityProfile");
        } else {
            ism_common_setErrorData(rc, "%s%s", "SecurityProfile", "ConfigurationPolicies");
        }
        goto VALIDATION_END;
    }*/



    /* Check if required items are specified */
    int chkMode = 0;
    rc = ism_config_validate_checkRequiredItemList(reqList, chkMode );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }


    /* Add missing default values or rest all items to default for AdminEndpoint */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

