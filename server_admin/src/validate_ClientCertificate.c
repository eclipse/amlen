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

extern int ism_config_setApplyCertErrorMsg(int rc, char *item);
extern void ism_config_deleteTmpCertFile(char **filelist, int filecount);
extern int ism_config_isFileExist( char * filepath);
extern int ism_config_checkTrustedCertExist(int type, char *secProfile, char *trustCert, int *isNewFile, int *ncerts);

/*
 * Component: Transport
 * Item: ClientCertificate
 *
 * Validation rules:
 * - Named object
 * - User can associated multiple TrustedCertificates with a SecurityProfile
 * - .....
 *
 * Schema:
 *
 * {
 *     "ClientCertificate": [
 *         {
 *             "CertificateName": "string",
 *             "SecurityProfileName": "string"
 *         },
 *         ...
 *     ]
 * }
 *
 *
 * Where:
 *
 * CertificateName         : Client Certificate file name.
 * SecurityProfileName     : Name of the security profile.
 *
 *
 */

XAPI int32_t ism_config_validate_ClientCertificate(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    int objType;
    char **filelist = NULL;
    int filecount = 0;
    int i = 0;


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
		ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "ClientCertificate");
		goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* No Name for the array*/

    /* Validate configuration items */
    json_t * mergedObj = validateObj;
    filelist = alloca(json_array_size(mergedObj));

    for (i=0; i<json_array_size(mergedObj); i++) {
		json_t *instEntry = json_array_get(mergedObj, i);

		if (!instEntry) {
			TRACE(3, "InstanceEntry %d is empty\n", i);
			continue;
		}

		int overwrite = 0;
		char *clientCert = NULL;
		char *spName = NULL;
		int isNewFile = 0;

		void *itemIter = json_object_iter(instEntry);
		while(itemIter)
		{
			key = json_object_iter_key(itemIter);
			value = json_object_iter_value(itemIter);
			objType = json_typeof(value);

	        if (!key) {
	        	itemIter = json_object_iter_next(mergedObj, itemIter);
	        	continue;
	        }

	        if ( !strcmp(key, "CertificateName")) {
	        	if (objType != JSON_STRING) {
	        		ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item?item:"null", "null", key, ism_config_json_typeString(objType));
					rc = ISMRC_BadPropertyType;
					goto VALIDATION_END;
	        	}
				clientCert = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),(char *)json_string_value(value));
				if ( clientCert && *clientCert != '\0' ) {
					rc = ism_config_validateItemData( reqList, (char *)key, (char *)clientCert, action, props);
					if ( rc != ISMRC_OK ) {
						goto VALIDATION_END;
					}
				}else {
					ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", item?item:"null", key, "null");
					rc = ISMRC_BadOptionValue;
					goto VALIDATION_END;
				}
	        } else if ( !strcmp(key, "SecurityProfileName")) {
	        	if (objType != JSON_STRING) {
	        		ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item?item:"null", "null", key, ism_config_json_typeString(objType));
					rc = ISMRC_BadPropertyType;
					goto VALIDATION_END;
	        	}
	        	spName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),(char *)json_string_value(value));
        		if ( spName && *spName != '\0' ) {
					rc = ism_config_validateItemData( reqList, (char *)key, (char *)spName, action, props);
					if ( rc != ISMRC_OK ) {
						goto VALIDATION_END;
					}
				}else {
					ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", item?item:"null", key, "null");
					rc = ISMRC_BadOptionValue;
					goto VALIDATION_END;
				}

        		//check if the SecurityProfile exists
        		if (ism_config_objectExist("SecurityProfile", (char *)spName, NULL) == 0)  {
                    rc = ISMRC_ObjectNotFound;
                    ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", key, spName);
                    goto VALIDATION_END;
        		}
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

	        itemIter = json_object_iter_next(instEntry, itemIter);
		} //End of while loop

		if (!clientCert) {
			rc = ISMRC_PropertyRequired;
			ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", "CertificateName", "null");
			goto VALIDATION_END;
		}

		if (!spName) {
			rc = ISMRC_PropertyRequired;
			ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", "SecurityProfile", "null");
			goto VALIDATION_END;
		}

		//check if Trusted Certificate exists. If it is in /tmp/userfile, isNewFile will be assigned.
		int ncerts = 0;
		rc = ism_config_checkTrustedCertExist(1, spName, clientCert, &isNewFile, &ncerts);
		if (rc) {
			goto VALIDATION_END;
		}

		if ( isNewFile == 1 && ncerts >= 100 ) {
			/* limit exceeded - return error */
			rc = ISMRC_TooManyItems;
			ism_common_setErrorData(rc, "%s%d%d", "ClientCertificate", ncerts, 100);
			goto VALIDATION_END;
		}

		//copy trusted Certificate to trustedstore
		if (isNewFile) {
			if (overwrite)
				/* args for Trusted cert - "apply", "TRUSTED", overwrite, secprofile, trustedcert */
				rc = ism_admin_ApplyCertificate("CLIENT", "true", (char *)spName, (char *)clientCert, NULL, NULL);
			else
				rc = ism_admin_ApplyCertificate("CLIENT", "false", (char *)spName, (char *)clientCert, NULL, NULL);

			if (rc) {
				TRACE(5, "%s: call certApply return error code: %d\n", __FUNCTION__, rc);
				int xrc = 0;
				xrc = ism_config_setApplyCertErrorMsg(rc, item);
				rc = xrc;
				if ( rc == 6184 ) {
					ism_common_setErrorData(rc, "%s", clientCert?clientCert:"");
				}
				goto VALIDATION_END;
			}

			//add the new file into filelist for later deleting
            int flen = strlen(clientCert);
			char *newfile = alloca(flen + 1);
			memset(newfile, 0, flen+1);
			memcpy(newfile, clientCert, flen);
			newfile[flen] = '\0';
			filelist[filecount] = newfile;
			filecount++;
		} else {
            if ( overwrite == 0 ) {
                rc = ism_config_setApplyCertErrorMsg(4, item);
            }
        }

		if (overwrite)
		    json_object_del(instEntry, "Overwrite");
		if (clientCert)  ism_common_free(ism_memory_admin_misc,clientCert);
		if (spName)     ism_common_free(ism_memory_admin_misc,spName);
    }  // end of the for loop

VALIDATION_END:
    // Delete uploaded trusted certificates
    if (rc == ISMRC_OK) {
    	ism_config_deleteTmpCertFile(filelist, filecount);
    }

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}
