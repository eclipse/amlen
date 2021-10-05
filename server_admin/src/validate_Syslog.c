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
 * Item: LogLocation
 * Component: Server
 *
 * Description:
 * 		Location of the log file
 *
 * Schema:
 * {
 * 		"Syslog": {
 *      	"Syslog": {
 *      	    "Enabled":boolean,
 *				"Host":"string",
 *			    "Port":integer,
 *			    "Protocol":"string"
 *			}
 *		}
 *	}
 *
 *
 * Type: Log destination type (syslog or file)
 * Destination: filename, if type is file; syslog facility if type is syslog
 *
 * Component callback(s):
 * - Server
 * Validation rules:
 *
 * User defined Configuration Item(s):
 * ----------------------------------
 *
 * N/A
 *
 * Internal Configuration Item(s):
 * ------------------------------
 *
 * Unused configuration Item(s):
 * ----------------------------
 *
 *
 * Dependent validation rules:
 * --------------------------
 * LogLocation can not be deleted
 *
 *
 */
XAPI int32_t ism_config_validate_Syslog(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    json_type objType;

    TRACE(9, "Entry %s: currPostObj:%p, validateObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, validateObj?validateObj:0, item?item:"null", name?name:"null", action);

    if (!name || strcmp(name, "Syslog")) {
    	rc = ISMRC_ArgNotValid;
		ism_common_setErrorData(rc, "%s", item);
		goto VALIDATION_END;
    }

    if ( !validateObj || !props ) {
    	TRACE(3, "%s: validation object: %p or IMA properties: %p is null.\n", __FUNCTION__, validateObj?validateObj:0, props?props:0);
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(validateObj) == JSON_NULL ) {
    	rc = ISMRC_DeleteNotAllowed;
		ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "Syslog");
		goto VALIDATION_END;
    }

    if (!currPostObj) {
    	TRACE(5, "Input configuration is empty, no validation is needed.\n");
    	goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Validate configuration items */
    json_t * mergedObj = validateObj;
    /* Validate for one instance only and optional object name (name should be Syslog) */
    json_object_foreach(mergedObj, key, value) {
        if (json_typeof(value) == JSON_OBJECT) {
            /* size of mergedObj can not be more than 1 */
            if ( json_object_size(mergedObj) > 1 ) {
                /* Only one instance of Syslog is allowed */
                rc = ISMRC_TooManyItems;
                ism_common_setErrorData(ISMRC_TooManyItems, "%s%d%d", item, 2, 1);
                goto VALIDATION_END;

            }
        }
    }

    int isEnabled = 0;
    int hasHost = 0;

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);
        objType = json_typeof(value);

        if (!key) {
        	itemIter = json_object_iter_next(mergedObj, itemIter);
        	continue;
        }

        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;

        /* Check if syslog or file */
        if (!strcmp(key, "Enabled")) {
        	if (objType == JSON_TRUE) {
        		isEnabled = 1;
        	}
        } else if (!strcmp(key, "Host")) {
        	const char * valStr = (char *)json_string_value(value);
            if ( valStr && *valStr == '\0') {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, valStr);
                goto VALIDATION_END;
            }
        }

        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Add missing default values or rest all items to default for HighAvailability */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);

    /* get Host */
    json_t *hObj = json_object_get(mergedObj, "Host");
    if ( hObj && json_typeof(hObj) == JSON_STRING ) {
    	hasHost = 1;
    }
    if (isEnabled && !hasHost) {
		ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", "Host", "null");
		rc = ISMRC_PropertyRequired;
    	goto VALIDATION_END;
    }

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

