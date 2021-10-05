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

#define TRACE_COMP Admin

#include <janssonConfig.h>
#include "configInternal.h"
#include "validateInternal.h"

#include <validateConfigData.h>

/*
 * Item: ConnectionPolicy
 *
 * Description:
 * - ConnectionPolicy is a named object
 * - ConnectionPolicy can not be deleted if no Endpoint use it.
 *
 * Schema:
 *{
 *    "ConnectionPolicy":{
 *        "<ConnectionPolicyName>": {
 *            "Description":"string",
 *            "ClientID":"String",
 *            "ClientAddress":"String",
 *            "UserID":"String",
 *            "GroupID":"String",
 *            "CommonNames":"String",
 *            "Protocol":"String",
 *            "AllowDurable":"Boolean",
 *            "AllowPersistentMessages":"Boolean",
 *            "ExpectedMessageRate":"enum string",
 *            "MaxSessionExpiryInterval": "Number"
 *        }
 *    }
 *}
 *
 * Where:
 *
 * ConnectionPolicyName          : The name of the connection policy
 * Description   : Description of the connection policy. Optional.
 * ClientID      : The client ID that is allowed to connect to IBM MessageSight. Optional.
 * ClientAddress : The client IP addresses that are allowed to connect to IBM MessageSight. Optional.
 * UserID        : The messaging user ID that is allowed to connect to IBM MessageSight. Optional.
 * GroupID       : The messaging group that is allowed to connect to IBM MessageSight. Optional.
 * CommonNames   : The client certificate common name that must be used to connect to IBM MessageSight. Optional.
 * Protocol      : The protocols are allowed to connect to IBM MessageSight.
 * AllowDurable  : Whether MQTT clients can connect using a setting of cleanSession=0 (True) or not (False). The default is False.
 * AllowPersistentMessages : Whether MQTT clients can publish messages with a QoS of 1 or 2 (True) or not (False). The default is True.
 * ExpectedMessageRate  : Number of unacked inflight messages that can be sent to client. The default is 128.
 * MaxSessionExpiryInterval : Number of seconds to retain client state, default is 0 i.e. never expires
 *
 * Component callback(s):
 * - Security
 *
 */

XAPI int32_t ism_config_validate_ConnectionPolicy(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
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
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "AdminEndpoint");
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

    /* check items that are not settable */

    /* Validate configuration items */
    json_t * mergedObj = validateObj;

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

        /* Validate Description */
        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;

		if (objType != JSON_NULL) {
	    	char *propValue = NULL;
	    	int gotValue = 0;
	    	if (objType == JSON_STRING) {
	    		propValue = (char *)json_string_value(value);
	    		gotValue = 1;
		    } else if (objType == JSON_FALSE || objType == JSON_TRUE || objType == JSON_INTEGER) {
	    		gotValue = 1;
	    	}
	    	if (gotValue) {
                if ( !strcmp(key, "ClientAddress")) {
                    if ( *propValue != '\0' && *propValue != '*' ) {
                        rc = ism_config_validateDataType_IPAddresses("ClientAddress", propValue, 0);
                        if ( rc != ISMRC_OK  ) {
                            goto VALIDATION_END;
                        }
                    }
                } else if (!strcmpi(key, "Protocol")) {
	    	    	rc = ism_config_validate_CheckProtocol(propValue, 0, 0 );
	    	    	if (rc != ISMRC_OK) {
	    				rc = ISMRC_BadPropertyValue;
	    				ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propValue);
	    				goto VALIDATION_END;
	    	    	}
	    	    } else if ( !strcmp(key, "UserID") || !strcmp(key, "GroupID")
	    	    		|| !strcmp(key, "ClientID") || !strcmp(key, "CommonName")) {
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
	    	    } else if ( strcmp(key, "Name") && strcmp(key, "Description") &&
	    	    		strcmp(key, "AllowDurable")  && strcmp(key, "AllowPersistentMessages")) {
	    	    	/* Only allow one * tailing other valid characters */
	    	    	rc = ism_config_validate_Asterisk((char *)key, propValue);
	    	    	if (rc != ISMRC_OK) {
	    	    		goto VALIDATION_END;
	    	    	}
	    		}
	    	}
		}

	    /* validate special cases related to this object here*/
        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Check if required items are specified */
    int chkMode = 0;
    if (action == 2) chkMode = 1;
    rc = ism_config_validate_checkRequiredItemList(reqList, chkMode );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Add missing default values */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

