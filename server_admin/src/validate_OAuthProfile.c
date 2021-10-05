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
 * Item: OAuthProfile
 *
 * Description:
 * - OAuthProfile is a named object
 *
 * Schema:
 *{
 *    "OAuthProfile":{
 *        "ResourseURL":"string",
 *        "UserName":"string",
 *        "UserPassword":"string",
 *        "KeyFileName":"string",
 *        "AuthKey":"string",
 *        "UserInfoURL":"string",
 *        "UserInfoKey":"string",
 *        "GroupInfoKey":"string",
 *        "TokenSendMethod":"enum",
 *        "CheckServerCert":"enum",
 *        "GroupDelimiter":"string",
 *    }
 *}
 *
 * Where:
 *
 * ResourceURL      : URL to validate access token. Required.
 * UserName         : User name for Basic Authentication to the OAuth server
 * UserPassword     : User password for Basic Authentication to the OAuth server
 * KeyFileName      : OAuth server certificate file name.
 * AuthKey          : Token name property to be used in ResourceURL to validate
 *                    token. UTF8 string. No leading or trailing space. No control
 *                    characters or comma. Required.
 * UserInfoURL      : URL to retrieve the User Info.
 * UserInfoKey      : Username property in the response for user info request. UTF8
 *                    string. No leading or trailing space. No control characters or
 *                    comma.
 * GroupInfoKey     : Response for user info request can also include group information.
 *                    This configuration item specifies Group name property in the
 *                    response for user info request. UTF8 string. No leading or
 *                    trailing space. No control characters or comma.
 * TokenSendMethod  : Specify how to send user credentials to OAuth server - GET/POST and param/header
 * CheckServerCert  : To verify OAuth server certificate.
 * GroupDelimiter   : Group name property returned by user info request may include multiple
 *                    groups separated by a comma, space, hash etc. This configuration item
 *                    specifies character used as delimiter. No control characters.
 *
 * Component callback(s):
 * - Security?
 *
 */

XAPI int32_t ism_config_validate_OAuthProfile(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    json_type objType;
    int chkMode = 0;

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

    if ( json_typeof(mergedObj) == JSON_NULL ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "OAuthProfile");
        goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_object_size(mergedObj) == 0 ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "ResourceURL", "null");
        goto VALIDATION_END;
    }

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, props);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    /* Validate configuration items */
    char *certName = NULL;
    int   overwrite = 0;
    int   rURLDefined = 0;
    int   userNameDefined = 0;
    int   userPasswordDefined = 0;

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);
        objType = json_typeof(value);

        /* Overwrite and Verify are not in the schema */
        if (!strcmp(key, "Overwrite")) {
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

            /* special case for GroupDelimiter - allow space */
            if ( rc == ISMRC_BadPropertyValue && !strcmp(key, "GroupDelimiter")) {
                char *propValue = (char *)json_string_value(value);
                if ( *propValue == ' ') rc = ISMRC_OK;
            }
            if (rc != ISMRC_OK) goto VALIDATION_END;

            /* special case for UserName and UserPassword */
            if ( !strcmp(key, "UserName")) {
                char *propValue = (objType == JSON_STRING) ?(char *)json_string_value(value) : NULL;
                if ( propValue && *propValue != '\0') {
                    userNameDefined = 1;
                }
            }
            if ( !strcmp(key, "UserPassword")) {
                char *propValue = (objType == JSON_STRING) ?(char *)json_string_value(value) : NULL;
                if ( propValue && *propValue != '\0') {
                    userPasswordDefined = 1;
                }
            }
           if ( !strcmp(key, "KeyFileName") ) {
                char *propValue = (objType == JSON_STRING) ?(char *)json_string_value(value) : NULL;
                if ( propValue && *propValue != '\0') {
                    int clen = strlen(propValue) + 1;
                    certName = alloca(clen + 1 );
                    memset(certName, 0, clen+1);
                    memcpy(certName, propValue, clen);
                    certName[clen] = '\0';
                }
            } else if (!strcmp(key, "ResourceURL")) {
                rURLDefined = 1;
            } else if ( !strcmp(key, "GroupInfoKey") || !strcmp(key, "UserInfoKey")) {
                char *propValue = (char *)json_string_value(value);
                if ( propValue && *propValue != '\0') {
                    int len = strlen(propValue);
                    int count = ism_config_validate_UTF8(propValue, len, 1, 1);
                    if (count<1) {
                        ism_common_setError(ISMRC_UnicodeNotValid);
                        rc = ISMRC_UnicodeNotValid;
                        goto VALIDATION_END;
                    }

                    if (propValue[len-1] == ' ') {
                        rc = ISMRC_BadPropertyValue;
                        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propValue);
                        goto VALIDATION_END;
                    }
                }
            }
        }
        itemIter = json_object_iter_next(mergedObj, itemIter);
        continue;
    }

    /* check for required item ResourceURL */
    if ( rURLDefined == 0 ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "ResourceURL", "null");
        goto VALIDATION_END;
    }
    if ( (userNameDefined == 0 && userPasswordDefined == 1)) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "UserName", "null");
        goto VALIDATION_END;
    }

    /* Delete unwanted keys from mergedObj */
    /* Delete Overwrite entry */
    json_object_del(mergedObj, "Overwrite");

    /* Check if required items are specified
     * Also check if one of MinOneOptions has been specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, chkMode );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    if (certName && *certName) {
    	//Check if certificate/key have been used by other OAuthProfile
    	rc = ism_config_json_validateOAuthCertUnique(name, certName);
    }

    if (rc != ISMRC_OK) {
		goto VALIDATION_END;
	}

    //apply certificate if there is one
    if (certName && *certName) {
    	rc =  ism_config_restapi_applyOAuthCert(name, certName, overwrite);
    }
    if (rc) {
		TRACE(5, "%s: call msshell return error code: %d\n", __FUNCTION__, rc);
		goto VALIDATION_END;
	}

    /* Add missing default values */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);


VALIDATION_END:

	TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

