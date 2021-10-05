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
 * Item: SubscriptionPolicy
 * Component: Security
 *
 * Description:
 * 		Policy to validate subscription messaging activity
 * 		SubscriptionPolicy cannot be deleted if it is in use by an Endpoint object
 *
 * Schema:
 * {
 * 		"SubscriptionPolicy": {
 *      	"Name":"string",
 *			"Description":"string",
 *			"ClientID":"string",
 *			"ClientAddress":"string",
 *			"UserID":"string",
 *			"GroupID":"string",
 *			"CommonNames":"string",
 *			"Protocol":"string",
 *			"Subscription":"string",
 *			"ActionList":"string", 			(LIST: Receive,Control)
 *			"MaxMessages":"number",
 *			"MaxMessagesBehavior":"string",	(ENUM: RejectNewMessages,DiscardOldMessages)
 *			"DefaultSelectionRule":"string" (Selector)
 *		}
 *	}
 *
 *
 * Description: Description of the messaging policy object. Optional Item.
 * ClientID: match for Client IDs allowed to use this policy
 * ClientAddress: match for client IP addresses allowed to use this policy
 * UserID: match for User IDs allowed to use this policy
 * GroupID: match for Group IDs allowed to use this policy
 * CommonName: match for Certificate common names allowed to use this policy
 * Protocol: list of protocols allowed (may contain MQTT, JMS and any installed plug-in protocols)
 * Subscription: match for destination topics to use this policy
 * ActionList: Enum - Publish,Subscribe,Send,Receive,Browse,Control
 * MaxMessages: number
 * MaxMessagesBehavior: Enum - RejectNewMessages,DiscardOldMessage
 *
 * Component callback(s):
 * - Security
 * Validation rules:
 *
 * User defined Configuration Item(s):
 * ----------------------------------
 *
 * Update: Data type is boolean. Options: True, False. Default: False
 *       If set to True, object should be in the configuration file.
 *       If set to False, object should not be in the configuration file.
 *
 * Delete:  Data type is boolean. Options: True, False. Default: False
 *       If set to True, object should be in the configuration file.
 *       If set to False, no action is taken.
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
 * SubscriptionPolicy can not be deleted if it is used by an Endpoint object.
 *
 *
 */
XAPI int32_t ism_config_validate_SubscriptionPolicy(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    json_type objType;

    TRACE(9, "Entry %s: currPostObj:%p, mergedObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, mergedObj?mergedObj:0, item?item:"null", name?name:"null", action);

    if ( !mergedObj || !props || !name ) {
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }


    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(mergedObj) == JSON_NULL || json_object_size(mergedObj) == 0 ) {
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
	    // TODO: Validate DiscoveryServerList
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


                /* Policy Substitution checking*/
            	if (propValue) {
            		rc = ism_config_validate_PolicySubstitution(item, (char *)key, propValue);
            		if (rc != ISMRC_OK) {
            			goto VALIDATION_END;
            		}
            	}

                if ( !strcmp(key, "ClientAddress")) {
                    if ( *propValue != '\0' && *propValue != '*' ) {
                        rc = ism_config_validateDataType_IPAddresses("ClientAddress", propValue, 0);
                        if ( rc != ISMRC_OK  ) {
                            goto VALIDATION_END;
                        }
                    }
                } else if (!strcmp(key, "Protocol")) {
                    rc = ism_config_validate_CheckProtocol(propValue, 0, 0 );
                    if (rc != ISMRC_OK) {
                    	rc = ISMRC_BadPropertyValue;
                        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propValue);
                        goto VALIDATION_END;
                    }
                } else if ( !strcmp(key, "ClientID") || !strcmp(key, "UserID") ||
                		!strcmp(key, "GroupID") || !strcmp(key, "CommonNames") ) {
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
                } else if ( !strcmp(key, "Subscription")) {
                	if (!*propValue ) {
                		rc = ISMRC_BadPropertyValue;
                		ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propValue);
            			goto VALIDATION_END;
                	}
                }
            }
		}
        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

 	/* SubscriptionPolicy can not be deleted if used by an endpoint */
	if (action == 2) {
	    rc = ism_config_valDeleteEndpointObject("SubscriptionPolicies", name);
	    if (rc == ISMRC_PolicyInUse ) {
		    TRACE(3, "%s: The configuration object: %s, name: %s is still in use.\n", __FUNCTION__, item, name);
	    }
	    ism_common_setError(rc);
        goto VALIDATION_END;
	}

    /* Check if required items are specified
     * Also check if one of MinOneOptions has been specified */
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


