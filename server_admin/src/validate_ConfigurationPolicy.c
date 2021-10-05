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

#include <validateConfigData.h>
#include "validateInternal.h"


/*
 *
 * Item: ConfigurationPolicy
 *
 * Description:
 * - Policy to validate administrative and monitoring actions
 * - ConfigurationPolicy can not be deleted if it is used by AdminEndpoint object.

 *
 * Schema:
 *
 *   {
 *       "ConfigurationPolicy": {
 *           "<ConfigurationPolicyName>": {
 *               "Description":"string",
 *               "UserID":"string",
 *               "GroupID":"string",
 *               "CommonName":"string",
 *               "ClientAddress":"string",
 *               "ActionList":"string"
 *           }
 *       }
 *   }
 *
 * Where:
 *
 * Description: Description of the configuration object. Optional Item.
 * UserID: User ID
 * GroupID: Group ID
 * CommonName: Certificate common name
 * ClientAddress: Data type IPAddress
 * ActionList: Enum - Configure,View,Monitor,Manage
 *
 * Component callback(s):
 * - Security
 *
 */

XAPI int32_t ism_config_validate_ConfigurationPolicy(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    int objType;

    TRACE(9, "Entry %s: currPostObj:%p, mergedObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, mergedObj?mergedObj:0, item?item:"null", name?name:"null", action);

    if ( !mergedObj || !props || !name ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(mergedObj) == JSON_NULL ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", item?item:"null");
        goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Check for delete case */
    if ( json_typeof(mergedObj) == JSON_NULL ) {
        rc = ISMRC_NotImplemented;
        ism_common_setErrorData(rc, "%s", "ConfiguationPolicy");
        goto VALIDATION_END;
    }

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, props);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    /* Validate configuration items */
    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);
        objType = json_typeof(value);

        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;

        /* validate special cases related to this object here*/
        if (objType != JSON_NULL) {
            char *propValue = NULL;
            int gotValue = 0;
            if (objType == JSON_STRING) {
                propValue = (char *)json_string_value(value);
                gotValue = 1;
            } else if (objType == JSON_FALSE || objType == JSON_TRUE || objType == JSON_INTEGER) {
                gotValue = 1;
            }

            if ( gotValue ) {
                /* validate special cases related to this object here*/
                if ( !strcmp(key, "ClientAddress")) {
                    if ( *propValue != '\0' && *propValue != '*' ) {
                        rc = ism_config_validateDataType_IPAddresses("ClientAddress", propValue, 0);
                        if ( rc != ISMRC_OK  ) {
                            goto VALIDATION_END;
                        }
                    }
                } else if ( !strcmp(key, "UserID") || !strcmp(key, "GroupID") || !strcmp(key, "CommonName")) {
                    /* Do not allow ** */
                    if ( propValue && strstr(propValue, "**")) {
                        rc = ISMRC_BadPropertyValue;
                        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propValue);
                        goto VALIDATION_END;
                    }
                    /* Only allow one * tailing other valid characters */
                    rc = ism_config_validate_Asterisk((char *)key, propValue);
                    if (rc != ISMRC_OK) {
                        goto VALIDATION_END;
                    }
                    /* Policy Substitution checking*/
                    rc = ism_config_validate_PolicySubstitution(item, (char *)key, propValue);
                    if (rc != ISMRC_OK) {
                        goto VALIDATION_END;
                    }
                }

                /* Only allow one * tailing other valid characters */
                if ( strcmp(key, "Description") && strcmp(key, "ActionList")) {
                    rc = ism_config_validate_Asterisk((char *)key, propValue);
                    if (rc != ISMRC_OK) {
                        goto VALIDATION_END;
                    }
                }
            }
        }

        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Add missing default values */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

