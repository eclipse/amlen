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
 * Component: Security
 * Item: LTPAProfile
 *
 * Validation rules:
 * - Named object
 * - User can create multiple objects
 * - .....
 *
 * Schema:
 *
 * {
 *     "LTPAProfile": {
 *         "<LTPAProfileName>": {
 *             "KeyFileName": "string",
 *             "Password": "string",
 *             "Overwrite": "Boolean"
 *         }
 *     }
 * }
 *
 *
 * Where:
 *
 * LTPAProfileName    : The name of this LTPA profile.
 * KeyFileName        : The name of the ltpa certificate that is used by this LTPAProfile
 * Password           : The password for the ltpa certificate.
 * Overwrite          : If set to true, overwrite the existing ltpa certificate.
 *
 * Component callback(s):
 * - Security
 *
 */

XAPI int32_t ism_config_validate_LTPAProfile(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    int objType;
    int overwrite = 0;
    char *certName = NULL;
    char *certPwd = NULL;
    int hasNewPwd = 0;
    char *password = NULL;

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
		ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "LTPAProfile");
		goto VALIDATION_END;
    }

    if (!currPostObj) {
    	TRACE(5, "Input configuration is empty, no validation is needed.\n");
    	goto VALIDATION_END;
    } else {
    	//check if new payload has Password specified.
    	json_t *inst = json_object_get(currPostObj, "LTPAProfile");
    	if (inst && json_typeof(inst) == JSON_OBJECT) {
    		json_t *ninst = json_object_get(inst, name);
    		if (ninst && json_typeof(ninst) == JSON_OBJECT) {
				json_t *bindpwd = json_object_get(ninst, "Password");
				if (bindpwd) {
					TRACE(9, "%s: The new configuration has Password specified.\n", __FUNCTION__);
					hasNewPwd = 1;
				}
    		}
    	}
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

        if (!strcmp(key, "Overwrite") ) {
            if ( objType != JSON_TRUE && objType != JSON_FALSE ) {
                /* Return error */
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item, name, key, ism_config_json_typeString(objType));
                rc = ISMRC_BadPropertyType;
                goto VALIDATION_END;
            } else {
                if ( objType == JSON_TRUE ) {
                    overwrite = 1;
                }
            }
        } else {
            rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
    		if (rc != ISMRC_OK) goto VALIDATION_END;
            char *propValue = (char *)json_string_value(value);
            if ( !strcmp(key, "KeyFileName") ) {
                if ( propValue && *propValue != '\0' ) {
                    int clen = strlen(propValue) + 1;
    				certName = alloca(clen + 1 );
    				memset(certName, 0, clen+1);
    				memcpy(certName, propValue, clen);
    				certName[clen] = '\0';
                }
            } else if ( !strcmp(key, "Password") && propValue && *propValue != '\0' ) {
				if (!strcmp(propValue, "XXXXXX")) {
					rc = ISMRC_BadPropertyValue;
					ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, "XXXXXX");
					goto VALIDATION_END;
				}

				int clen = strlen(propValue) + 1;
				certPwd = alloca(clen + 1 );
				memset(certPwd, 0, clen+1);
				memcpy(certPwd, propValue, clen);
				certPwd[clen] = '\0';
            }
        }

        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Delete unwanted keys from mergedObj */
    /* Delete Overwrite entry */
    json_object_del(mergedObj, "Overwrite");

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    //Check if certificate/key have been used by other CertificateProfile
    rc = ism_config_json_validateLTPACertUnique(name, certName);
    if (rc != ISMRC_OK) {
		goto VALIDATION_END;
	}

    /* Check if LTPA key can be decrypted using provided password */
    if ( certName ) {
        char *pwd = NULL;

        /* if certPwd is not specified in this call, get the current value from configuration */
        if ( !certPwd ) {
            json_t *passwd = json_object_get(mergedObj, "Password");
            if ( !passwd || json_typeof(passwd) != JSON_STRING ) {
                int otype = passwd?json_typeof(passwd): JSON_BOOLEAN_TYPE + 1;
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item, name, "Password", ism_config_json_typeString(otype));
                rc = ISMRC_BadPropertyType;
                goto VALIDATION_END;
            }
            char *passwdStr = (char *)json_string_value(passwd);
            pwd = ism_security_decryptAdminUserPasswd(passwdStr);
        }

        if ( !certPwd && !pwd ) {
            /* Missing password */
            rc = ISMRC_PropertyRequired;
            ism_common_setErrorData(rc, "%s%s", "Password", "null");
            goto VALIDATION_END;
        }

        /* validate key and password combination */
        char *keyfname = NULL;
        ismLTPA_t *secretKey=NULL;
        int keypathlen = 0;
        const char *ltpaKeyStore = ism_common_getStringProperty(ism_common_getConfigProperties(), "LTPAKeyStore");

        if (getenv("CUNIT") == NULL) {
            ltpaKeyStore = IMA_SVR_DATA_PATH "/userfiles";
        }

        if ( ltpaKeyStore ) {
            keypathlen = strlen(certName) + strlen(ltpaKeyStore) + 32;
        } else {
            keypathlen = strlen(certName) + 1024;
        }

        keyfname = (char *)alloca(keypathlen);
        sprintf(keyfname, "%s/%s", ltpaKeyStore?ltpaKeyStore:"", certName);

        if ( certPwd ) {
            rc = ism_security_ltpaReadKeyfile(keyfname, certPwd, &secretKey);
        } else {
            rc = ism_security_ltpaReadKeyfile(keyfname, pwd, &secretKey);
        }

        if (secretKey == NULL || rc != ISMRC_OK ) {
            rc = ISMRC_LTPAInvalidKeyFile;
            ism_common_setError(rc);
            if ( pwd ) ism_common_free(ism_memory_admin_misc,pwd);
            goto VALIDATION_END;
        } else {
            ism_security_ltpaDeleteKey(secretKey);
        }
        if ( pwd ) ism_common_free(ism_memory_admin_misc,pwd);
    }


    //apply certificate if there is one
    if (getenv("CUNIT") == NULL){
        rc =  ism_config_restapi_applyLTPACert(name, certName, certPwd, overwrite);
        if (rc) {
		    TRACE(5, "%s: call msshell return error code: %d\n", __FUNCTION__, rc);
	    }
    }

    // update mergedObj with encrypted password
	if (certPwd && hasNewPwd) {
		password = ism_security_encryptAdminUserPasswd(certPwd);
		if ( password == NULL || *password == '\0') {
			rc = ISMRC_BadPropertyValue;
			ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Password", "null");
			goto VALIDATION_END;
		}
		TRACE(5, "%s: Password has been encrypted.\n", __FUNCTION__);

		json_object_set(mergedObj, "Password", json_string((const char *)password));
		if (password) ism_common_free(ism_memory_admin_misc,password);
	}

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

