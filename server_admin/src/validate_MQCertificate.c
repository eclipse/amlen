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

extern int ism_config_restapi_applyMQCert(char *keyfile, char *stashfile, int overwrite);
extern void ism_config_deleteTmpCertFile(char **filelist, int filecount);

/*
 * Component: MQConnectivity
 * Item: MQCertificate
 *
 * Schema:
 *
 * {
 *     "MQCertificate": [
 *         {
 *             "MQSSLKey": "string",
 *             "MQStashPassword": "string"
 *         },
 *         ...
 *     ]
 * }
 */

XAPI int32_t ism_config_validate_MQCertificate(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    int objType;
    char **filelist = NULL;
    int filecount = 0;
	int overwrite = 0;
	char *certFile = NULL;
	char *passFile = NULL;
    int flen = 0;
	char *newfile = NULL;


    TRACE(9, "Entry %s: currPostObj:%p, mergedObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, mergedObj?mergedObj:0, item?item:"null", name?name:"null", action);

    if ( !mergedObj || !props ) {
    	TRACE(3, "%s: validation object: %p or IMA properties: %p is null.\n", __FUNCTION__, mergedObj?mergedObj:0, props?props:0);
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(mergedObj) == JSON_NULL ) {
    	rc = ISMRC_DeleteNotAllowed;
		ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "MQCertificate");
		goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    filelist = alloca(2);

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

		if ( !strcmp(key, "MQSSLKey")) {

			if (objType != JSON_STRING) {
				ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item?item:"null", "null", key, ism_config_json_typeString(objType));
				rc = ISMRC_BadPropertyType;
				goto VALIDATION_END;
			}
			certFile = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),(char *)json_string_value(value));
			if ( certFile && *certFile != '\0' ) {
				rc = ism_config_validateItemData( reqList, (char *)key, (char *)certFile, action, props);
				if ( rc != ISMRC_OK ) {
					goto VALIDATION_END;
				}
			} else {
				ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", item?item:"null", key, "null");
				rc = ISMRC_BadOptionValue;
				goto VALIDATION_END;
			}

			//add the new file into filelist for later deleting
            flen = strlen(certFile);
			newfile = alloca(flen + 1);
			memset(newfile, 0, flen+1);
			memcpy(newfile, certFile, flen);
			newfile[flen] = '\0';
			filelist[filecount] = newfile;
			filecount++;

		} else if ( !strcmp(key, "MQStashPassword")) {

			if (objType != JSON_STRING) {
				ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item?item:"null", "null", key, ism_config_json_typeString(objType));
				rc = ISMRC_BadPropertyType;
				goto VALIDATION_END;
			}
			passFile = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),(char *)json_string_value(value));
			if ( passFile && *passFile != '\0' ) {
				rc = ism_config_validateItemData( reqList, (char *)key, (char *)passFile, action, props);
				if ( rc != ISMRC_OK ) {
					goto VALIDATION_END;
				}
			} else {
				ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", item?item:"null", key, "null");
				rc = ISMRC_BadOptionValue;
				goto VALIDATION_END;
			}

            flen = strlen(passFile);
			newfile = alloca(flen + 1);
			memset(newfile, 0, flen+1);
			memcpy(newfile, passFile, flen);
			newfile[flen] = '\0';
			filelist[filecount] = newfile;
			filecount++;

		} else if ( !strcmp(key, "Overwrite")) {

			if ( objType != JSON_TRUE && objType != JSON_FALSE ) {
				/* Return error */
				ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item?item:"null", "null", key, ism_config_json_typeString(objType));
				rc = ISMRC_BadPropertyType;
				goto VALIDATION_END;
			}
			if ( objType == JSON_TRUE )
				overwrite = 1;

		} else {

			TRACE(3, "%s: The configuration item: %s is not supported.\n", __FUNCTION__, key);
			ism_common_setErrorData(ISMRC_ArgNotValid, "%s", key);
			rc = ISMRC_ArgNotValid;
			goto VALIDATION_END;

		}

		itemIter = json_object_iter_next(mergedObj, itemIter);
	}

	/* Apply MQCert files */
	rc = ism_config_restapi_applyMQCert(certFile, passFile, overwrite);

VALIDATION_END:

	if (overwrite)
		json_object_del(mergedObj, "Overwrite");

	if (certFile)  ism_common_free(ism_memory_admin_misc,certFile);
	if (passFile)  ism_common_free(ism_memory_admin_misc,passFile);

    if (rc == ISMRC_OK) {
    	ism_config_deleteTmpCertFile(filelist, filecount);
    }

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}
