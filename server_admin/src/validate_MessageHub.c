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


/*
 * Item: MessageHub
 *
 * Description:
 * - Message hub
 * - MessageHub can not be deleted if it is used by an Endpoint object.
 *
 * Schema:
 *
 *   {
 *       "MessageHub":
 *           "<MessageHubName>": {
 *               "Description":"string"
 *           }
 *       }
 *   }
 *
 * Where:
 *
 * <MessageHubName>: MessageHub name
 *                   ism_config_validateDataType_name()
 * Description: Description of configuration item (optional item)
 *              ism_config_validationDataType_string()
 *
 * Component callback(s):
 * - Transport
 *
 */

XAPI int32_t ism_config_validate_MessageHub(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    int objType = 0;

    TRACE(9, "Entry %s: currPostObj:%p, mergedObj:%p, item:%s, action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, mergedObj?mergedObj:0, item?item:"null", action);

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

    /* In current post, check AdminEndpoint delete is requested - it is not allowed */
    json_t *mhObj = json_object_get(currPostObj, "MessageHub");
    json_t *objval = NULL;

    void *objiter = json_object_iter(mhObj);
    while (objiter) {
        objval = json_object_iter_value(objiter);
        objType = json_typeof(objval);
        break;
    }
    if ( objType == JSON_NULL ) {
        /* Delete instance */
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "AdminEndpoint");
        goto VALIDATION_END;
    }

    /* Validate configuration items */
    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_get(mergedObj, key);
        objType = json_typeof(value);

        /* Validate Description */
        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;
        itemIter = json_object_iter_next(mergedObj, itemIter);

    }

    /* Check if required items are specified */
    int chkMode = 0;
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

