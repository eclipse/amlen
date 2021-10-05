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
 * 		"LogLocation": {
 *      	"<Log type>": {
 *				"LocationType":"string",
 *			    "Destination":"string"
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

const char * LOG_LOCATION_DESTINATION	= "Destination";
const char * LOG_LOCATION_TYPE 			= "LocationType";
const char * LOG_LOCATION_SYSLOG_TYPE	= "syslog";

XAPI int32_t ism_config_validate_LogLocation(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;

    TRACE(9, "Entry %s: currPostObj:%p, validateObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, validateObj?validateObj:0, item?item:"null", name?name:"null", action);

    if ( !validateObj || !props ) {
    	TRACE(3, "%s: validation object: %p or IMA properties: %p is null.\n", __FUNCTION__, validateObj?validateObj:0, props?props:0);
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(validateObj) == JSON_NULL ) {
    	rc = ISMRC_DeleteNotAllowed;
		ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "LogLocation");
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

    /* Validate Name */
	rc = ism_config_validateItemData( reqList, "Name", name, action, props);
	if ( rc != ISMRC_OK ) {
		goto VALIDATION_END;
	}

    /* Validate configuration items */
    json_t * mergedObj = validateObj;

    int isSyslog = 0;
    char * destination = NULL;

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);

        if (!key) {
        	itemIter = json_object_iter_next(mergedObj, itemIter);
        	continue;
        }

        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;

        /* Check if syslog or file */
        const char * valStr = (char *)json_string_value(value);
        if (!strcmp(key, LOG_LOCATION_TYPE)) {
        	if (!strcmp(valStr, LOG_LOCATION_SYSLOG_TYPE)) {
        		isSyslog = 1;
        	}
        } else if (!strcmp(key, LOG_LOCATION_DESTINATION)) {
        	if (valStr) {
        		destination = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),valStr);
        	}
        } else {
        	rc = ISMRC_BadPropertyName;
        	ism_common_setErrorData(rc, "%s", key);
        	goto VALIDATION_END;
        }

        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    // For syslog, ensure Destination is a valid syslog facility (0-23)
    if (isSyslog) {
    	char *endPtr = NULL;
    	long facility = strtol(destination, &endPtr, 10);
    	if ((endPtr && *endPtr != 0) || (facility < 0 || facility > 23)) {
    		rc = ISMRC_BadPropertyValue;
    		ism_common_setErrorData(rc, "%s%s", LOG_LOCATION_DESTINATION, destination);
    		goto VALIDATION_END;
    	}
    }

    if (destination) {
    	ism_common_free(ism_memory_admin_misc,destination);
    }

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

