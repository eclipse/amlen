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
#include "validateInternal.h"

/* Validator for TopicMonitor */
/* To support publishing of monitoring data on $SYS topic tree,
 * restrict $SYS from TopicMonitor
 */
static int validateTopicStringValue(const char *value) {
    int i = 0;

    if ( !value ) {
        return 1;
    }

    int len = strlen(value);
    /* Restrict $SYS */
    if ( len >= 4 && strncmp(value, "$SYS", 4) == 0 )
        return 1;

    for (i=0; i<len-1; i++) {
        if ( value[i] == '+' || value[i] == '#') {
            return 1;
        }

    }
    /*The last character must be #*/
    if(value[len-1]!='#'){
        return 1;
    }
    return 0;
}

/*
 * Component: Transport
 * Item: SecurityProfile
 *
 * Validation rules:
 * - Named object
 * - User can create multiple objects
 * - .....
 *
 * Schema:
 *
 * {
 *     "TopicMonitor": [
 *         "TopicString":"string",
 *         ....
 *     ]
 * }
 *
 *
 * Where:
 *
 * TopicString          : Topic string to be monitored.
 *
 * Component callback(s):
 * - Engine
 *
 */

XAPI int32_t ism_config_validate_TopicMonitor(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int i = 0;
    int compType = -1;
    int objType;

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
		ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "TopicMonitor");
		goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* No Name for TopicString array*/

    /* check items that are not settable */

    /* Validate configuration items */
    json_t * mergedObj = validateObj;

    /* Iterate thru the node */
    for (i=0; i<json_array_size(mergedObj); i++) {
    	json_t *instEntry = json_array_get(mergedObj, i);

        objType = json_typeof(instEntry);
        if (objType != JSON_STRING ) {
			/* Return error */
        	ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item?item:"null", "null", "TopicString", ism_config_json_typeString(objType));
			rc = ISMRC_BadPropertyType;
			goto VALIDATION_END;

		}


        char *propValue = (char *)json_string_value(instEntry);
        if ( propValue && *propValue != '\0' ) {
			rc = ism_config_validateItemData( reqList, "TopicString", (char *)propValue, action, props);
			if ( rc != ISMRC_OK ) {
				goto VALIDATION_END;
			}

			rc = validateTopicStringValue(propValue);
			if (rc != ISMRC_OK) {
				rc = ISMRC_BadPropertyValue;
				ism_common_setErrorData(rc, "%s%s", "TopicMonitor", propValue);
			}
        }else {
        	ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s%s", item?item:"null", "null", "TopicString", "null");
			rc = ISMRC_BadOptionValue;
			goto VALIDATION_END;
        }
    }

	//rc = ism_config_validate_checkRequiredItemList(reqList, 0 );


VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}
