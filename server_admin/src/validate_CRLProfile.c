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
#include <crlprofile.h>
#include "validateInternal.h"


extern int ism_config_setApplyCertErrorMsg(int rc, char *item);
extern char * ism_config_getStringObjectValue(char *object, char *instance, char *item, int getLock);

#define OVERWRITE_NOT_SET	4
#define SAME_NAME_OVERWRITE	99

/*
 * Component: Transport
 * Item: CRLProfile
 *
 * Validation rules:
 * - Named object
 * - User can create multiple objects
 *
 * Schema:
 *
 * {
 *     "CRLProfile": {
 *         "<CRLProfileName>": {
 *             "CRLSource": "string",
 *             "UpdateInterval": "number",
 *             "RevalidateConnection": "boolean",
 *         }
 *     }
 * }
 *
 *
 * Where:
 *
 * CRLProfileName 		: The name of this CRL profile.
 * CRLSource      		: The uploaded filename or url path that is used to obtain the crl.
 * UpdateInterval 		: The frequency (in minutes) at which to download new crl if url is specified.
 * RevalidateConnection : If set, close connection that use the old crl. (future implementation)
 *
 * Component callback(s):
 * - Transport
 *
 */

XAPI int32_t ism_config_validate_CRLProfile(json_t * currPostObj, json_t * validateObj, char * objName, char * instName, int action, ism_prop_t * props)
{
    int32_t rc = ISMRC_OK;
    int compType = -1;
    ism_config_itemValidator_t * reqList = NULL;

    const char * key;
    json_t * value;
    int objType;

    int overwrite = -1;

    char * crlSrc = NULL;
    char * crlFileName = NULL;
	char * oldCRLProfName = NULL;

    uint8_t crlSrcChanged = 0;
    uint8_t isSrcUrl = 0;
    uint8_t revalidate = 0;


    TRACE(9, "Entry %s: currPostObj:%p, validateObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, validateObj?validateObj:0, objName?objName:"null", instName?instName:"null", action);

    if ( !validateObj || !props ) {
    	TRACE(3, "%s: validation object: %p or IMA properties: %p is null.\n", __FUNCTION__, validateObj?validateObj:0, props?props:0);
    	rc = ISMRC_NullPointer;
    	ism_common_setError(rc);
    	goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(validateObj) == JSON_NULL ) {
    	rc = ISMRC_DeleteNotAllowed;
    	ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "CRLProfile");
    	goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, objName, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }
    //General name validation
    rc = ism_config_validateItemData( reqList, "Name", instName, action, props);
	if ( rc != ISMRC_OK ) {
		goto VALIDATION_END;
	}

    /* Validate configuration items */
    json_t * mergedObj = validateObj;

    /* Iterate thru the Composite Object */
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

        if (!strcmp(key, "Overwrite") ) {
            if ( objType != JSON_TRUE && objType != JSON_FALSE ) {
                /* Return error */
                rc = ISMRC_BadPropertyType;
                ism_common_setErrorData(rc, "%s%s%s%s", objName?objName:"null", instName?instName:"null", key, ism_config_json_typeString(objType));
                goto VALIDATION_END;
            } else if ( objType == JSON_TRUE )
                overwrite = 1;
            else
                overwrite = 0;
            goto NEXT_KEY;
        }

        rc = ism_config_validateItemDataJson( reqList, instName, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;

        /* check if string is a url or file */
        if (!strcmp(key, "CRLSource")) {
        	crlSrc = (char *) json_string_value(value);
        	if (crlSrc && (*crlSrc != '\0')) {
        	    /* assume its a legit file, script call downstream checks if it exists */
        		if (ism_config_validateDataType_URL((char *) key, crlSrc, "2048", "http://", objName) == 0)
        			isSrcUrl = 1;

       			oldCRLProfName = ism_config_getStringObjectValue(objName, instName, (char *) key, 0);
       			if (oldCRLProfName && (*oldCRLProfName != '\0')) {
       				if (strcmp(oldCRLProfName, crlSrc))
       					crlSrcChanged = 1;
        		} else if (action == 1)
        		    crlSrcChanged = 1;
        	} else {
        		rc = ISMRC_PropertyRequired;
        		ism_common_setErrorData(rc, "%s%s", "CRLSource", "null");
        		goto VALIDATION_END;
        	}
        } else if (!strcmp(key, "RevalidateConnection")) {
        	if ( objType != JSON_TRUE && objType != JSON_FALSE ) {
        	    if (objType != JSON_NULL) {
        	        /* Return error */
        	        rc = ISMRC_BadPropertyType;
        	        ism_common_setErrorData(rc, "%s%s%s%s", objName?objName:"null", instName?instName:"null", key, ism_config_json_typeString(objType));
        	        goto VALIDATION_END;
        	    }
        	}

        	if (objType == JSON_TRUE)
        	    revalidate = 1;
        }
NEXT_KEY:
        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Delete Overwrite from the configuration - no need to persist this item */
    if ( overwrite != -1 ) {
       	json_object_del(mergedObj, "Overwrite");
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0);
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* if url given, download via curl request to the /tmp/userfiles directory but only on creates and updates to the url path	*/
    if (isSrcUrl == 1 && ((action == 0) || (crlSrcChanged == 1 && overwrite == 1 ))) {
       	char * filePath = NULL;
       	int filePathLen;
       	/* assume the name of certificate is after the last slash */
       	crlFileName = strrchr(crlSrc,'/');
       	crlFileName++;
       	filePathLen = strlen(USERFILES_DIR) + strlen(crlFileName)+2;
       	filePath = alloca(filePathLen);
       	memset(filePath,0,filePathLen);
       	snprintf(filePath, filePathLen, "%s%s", USERFILES_DIR, crlFileName);
       	ism_config_sendCRLCurlRequest(crlSrc, filePath, &rc);
       	if (rc != ISMRC_OK) {
       		unlink(filePath);
       		goto VALIDATION_END;
       	}
    }

    /* this call forks a script which only validates certificates and moves them to the staging directory, only on adds and updates */
    if (action == 0) {
    	/* Case 1 This is a brand new crl profile */
    	rc = ism_admin_ApplyCertificate("REVOCATION", "staging", "false", instName, isSrcUrl?crlFileName:crlSrc, NULL);
    } else {
    	if (overwrite == 1) {
    	    /* Case 2: This is an update with overwrite set */
            rc = ism_admin_ApplyCertificate("REVOCATION", "staging", "true", instName, isSrcUrl?crlFileName:crlSrc, NULL);
        } else if (crlSrcChanged == 1){
    	    /* Source changed but no overwrite set, error out... */
    	    rc = OVERWRITE_NOT_SET;
        }
    }

    if ( rc == SAME_NAME_OVERWRITE ) rc = ISMRC_OK;

    if (rc != ISMRC_OK) {
    	TRACE(5, "%s: apply certificate returned error code: %d\n", __FUNCTION__, rc);
		int xrc = 0;
		xrc = ism_config_setApplyCertErrorMsg(rc, objName);
		rc = xrc;
		if ( rc == 6184 )
			ism_common_setErrorData(rc, "%s", isSrcUrl?crlFileName:crlSrc);
		goto VALIDATION_END;
    }

    // Add missing default values to mergedObj
    rc = ism_config_addMissingDefaults(objName, mergedObj, reqList);
    if (rc != ISMRC_OK) {
    	TRACE(3, "%s: failed to add default settings into the object: %s.\n", __FUNCTION__, objName?objName:"null");
    	goto VALIDATION_END;
    }

    /* For FIlE type updates only, we apply new crl file to relevant security profiles and call revalidate if needed */
    if ((isSrcUrl == 0) && (action == 1)) {
        rc = ism_config_updateCRLProfileForSecurity(instName, crlSrc);
        if ((rc == ISMRC_OK) && (revalidate == 1)) {
            rc = ism_admin_executeCRLRevalidateOptionAllEndpoints(instName);
        }
    }

VALIDATION_END:

    if ( oldCRLProfName ) ism_common_free(ism_memory_admin_misc,oldCRLProfName);

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}

