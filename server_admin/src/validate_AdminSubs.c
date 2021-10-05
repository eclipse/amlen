/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
#include "validateInternal.h"

/*
 * Component: Engine
 * Item: AdminSubscription
 */
XAPI int32_t ism_config_validate_AdminSubscription(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;

    TRACE(9, "Entry %s: currPostObj:%p, validateObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, validateObj?validateObj:0, item?item:"null", name?name:"null", action);

    if ( !validateObj || !props || !name ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Name MUST be in the form /SHARENAME/TOPICSTRING where SHARENAME must be non-empty */
    if (name[0] != '/' || strlen(name) < 3 || name[1] == '/' || strchr(&name[1], '/') == NULL) {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Name", name);
        goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, NULL);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    /* Validate configuration items */

    /* Iterate thru the node */
    void *itemIter = json_object_iter(validateObj);
    while(itemIter)
    {
        const char *key = json_object_iter_key(itemIter);
        json_t *value = json_object_iter_value(itemIter);
        int objType = json_typeof(value);
        char *propString = (objType == JSON_STRING) ? (char *)json_string_value(value) : NULL;

        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;

        if (!strcmp(key, "SubscriptionPolicy"))
        {
            if (propString == NULL || *propString == '\0')
            {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propString);
                goto VALIDATION_END;
            }
        } else if (!strcmp(key, "QualityOfServiceFilter")) {
            if (propString == NULL) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propString);
                goto VALIDATION_END;
            }
        }

        /* validate special cases related to this object here*/
        itemIter = json_object_iter_next(validateObj, itemIter);
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    // TODO: More validation

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

/*
 * Component: Engine
 * Item: DurableNamespaceAdminSub
 */
XAPI int32_t ism_config_validate_DurableNamespaceAdminSub(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;

    TRACE(9, "Entry %s: currPostObj:%p, validateObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, validateObj?validateObj:0, item?item:"null", name?name:"null", action);

    if ( !validateObj || !props || !name ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Name MUST NOT contain a slash */
    if (strchr(name, '/') != NULL) {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Name", name);
        goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, NULL);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    /* Validate configuration items */

    /* Iterate thru the node */
    void *itemIter = json_object_iter(validateObj);
    while(itemIter)
    {
        const char *key = json_object_iter_key(itemIter);
        json_t *value = json_object_iter_value(itemIter);
        int objType = json_typeof(value);
        char *propString = (objType == JSON_STRING) ? (char *)json_string_value(value) : NULL;

        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;

        if (!strcmp(key, "SubscriptionPolicy")) {
            if (propString == NULL || *propString == '\0') {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propString);
                goto VALIDATION_END;
            }
        } else if (!strcmp(key, "TopicFilter") ||
                   !strcmp(key, "QualityOfServiceFilter")) {
            if (propString == NULL) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propString);
                goto VALIDATION_END;
            }
        }

        /* validate special cases related to this object here*/
        itemIter = json_object_iter_next(validateObj, itemIter);
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    // TODO: More validation

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

/*
 * Component: Engine
 * Item: NonpersistentAdminSub
 */
XAPI int32_t ism_config_validate_NonpersistentAdminSub(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;

    TRACE(9, "Entry %s: currPostObj:%p, validateObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, validateObj?validateObj:0, item?item:"null", name?name:"null", action);

    if ( !validateObj || !props || !name ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Name MUST NOT contain a slash */
    if (strchr(name, '/') != NULL) {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Name", name);
        goto VALIDATION_END;
    }

    /* only characters less than 0x41 that are allowed as first character are numbers */
    if (name[0] < 'A') {
    	if ( name[0] >= '0' && name[0] <= '9') {
    		// Numbers are allowed
    	} else {
    		rc = ISMRC_BadPropertyValue;
			ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Name", name);
			goto VALIDATION_END;
    	}
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, NULL);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    /* Validate configuration items */

    /* Iterate thru the node */
    void *itemIter = json_object_iter(validateObj);
    while(itemIter)
    {
        const char *key = json_object_iter_key(itemIter);
        json_t *value = json_object_iter_value(itemIter);
        int objType = json_typeof(value);
        char *propString = (objType == JSON_STRING) ? (char *)json_string_value(value) : NULL;

        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;

        if (!strcmp(key, "TopicPolicy")) {
            if (propString == NULL || *propString == '\0') {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propString);
                goto VALIDATION_END;
            }
        } else if (!strcmp(key, "TopicFilter") ||
                   !strcmp(key, "QualityOfServiceFilter")) {
            if (propString == NULL) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propString);
                goto VALIDATION_END;
            }
        }


        /* validate special cases related to this object here*/
        itemIter = json_object_iter_next(validateObj, itemIter);
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    // TODO: More validation

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}
