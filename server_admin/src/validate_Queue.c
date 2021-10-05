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
 * Item: Queue
 * Component: Engine
 *
 * Description:
 *
 * Schema:
 * {
 * 		"Queue": {
 *      	"Name":"string",
 *			"Description":"string",
 *			"AllowSend":"boolean",
 *			"ConcurrentConsumers":"boolean",
 *			"MaxMessages":"number"
 *		}
 *	}
 *
 *
 * Description: Description of the queue object. Optional Item.
 * AllowSend: Specify if the queue allows new messages to be sent.
 * ConcurrentConsumers: Specify if the queue allows multiple concurrent consumers.
 * MaxMessages: Maximum message count that can be buffered on the queue.
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
 * QueuePolicy can not be deleted if it is used by an Endpoint object.
 *
 *
 */
XAPI int32_t ism_config_validate_Queue(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;

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

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, props);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    /* name can not have + and # */
    if ( strstr(name, "+") || strstr(name, "#")) {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Name", name);
        goto VALIDATION_END;
    }

    /* Validate configuration items */

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);

		rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
		if (rc != ISMRC_OK) goto VALIDATION_END;

	    /* validate special cases related to this object here*/
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
