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

extern int ism_config_isFileExist( char * filepath);
extern int ismCUNITEnv;

static int checkPluginFileExist(char *pname, char *zipFile, char *propFile, int overwrite) {
	int rc = ISMRC_OK;
	char *pluginDir = IMA_SVR_DATA_PATH "/data/config/plugin/plugins";
	char *pluginStagingDir = IMA_SVR_DATA_PATH "/data/config/plugin/staging/install";

	int isNewFile = 0;

	if (!pname || (!zipFile && !propFile)) {
		rc = ISMRC_PropertyRequired;
		ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", "File, PropertiesFile", "null");
		goto CHECK_CERTEXIST_END;
	}

	/* If zipFile is specified in REST call - file should exist in userfiles dir */
	if ( zipFile && *zipFile != '\0' ) {
        int clen = strlen(USERFILES_DIR) + strlen(zipFile) + 1;
        char *cpath = alloca(clen);

        /* check userfiles dir */
        snprintf(cpath, clen, "%s%s", USERFILES_DIR, zipFile);
        if ( ism_config_isFileExist(cpath) ) {
            isNewFile = 1;
        } else {
            rc = ISMRC_ObjectNotFound;
            ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "File", zipFile);
            return rc;
    	}

    	/* check plugin dir */
        int tlen = strlen(pluginDir) + strlen(pname) + 14;
        char *tpath = alloca(tlen);
        snprintf(tpath, tlen, "%s/%s/plugin.json", pluginDir, pname);
        if ( !ism_config_isFileExist(tpath) ) {

        	/* check plugin staging dir */
            tlen = strlen(pluginStagingDir) + strlen(pname) + 14;
            tpath = alloca(tlen);
            snprintf(tpath, tlen, "%s/%s/plugin.json", pluginStagingDir, pname);
            if ( ism_config_isFileExist(tpath)) {
            	isNewFile += 1;
            }
        } else {
        	isNewFile += 1;
        }

		if ( isNewFile > 1 && overwrite <= 0 ) {
			TRACE(7, "Plugin ZIP file %s exists in userfiles dir. User Overwrite option.\n", zipFile);
			rc = 6171;
			ism_common_setErrorData(rc, "%s%s", "File", "PropertiesFile");
			return rc;
		}
	}

	isNewFile = 0;

    if ( propFile && *propFile != '\0' ) {
        int clen = strlen(USERFILES_DIR) + strlen(propFile) + 1;
        char *cpath = alloca(clen);

        /* check userfiles dir */
        snprintf(cpath, clen, "%s%s", USERFILES_DIR, propFile);
        if ( ism_config_isFileExist(cpath) ) {
            isNewFile = 1;
        } else {
            rc = ISMRC_ObjectNotFound;
            ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "PropertiesFile", propFile);
            return rc;
    	}

    	/* check plugin dir */
        int tlen = strlen(pluginDir) + strlen(pname) + 24;
        char *tpath = alloca(tlen);
        snprintf(tpath, tlen, "%s/%s/pluginproperties.json", pluginDir, pname);
        if ( !ism_config_isFileExist(tpath) ) {

        	/* check plugin staging dir */
            tlen = strlen(pluginStagingDir) + strlen(pname) + 24;
            tpath = alloca(tlen);
            snprintf(tpath, tlen, "%s/%s/pluginproperties.json", pluginStagingDir, pname);
            if ( ism_config_isFileExist(tpath)) {
            	isNewFile += 1;
            }
        } else {
        	isNewFile += 1;
        }

		if ( isNewFile > 1 && overwrite <= 0 ) {
			TRACE(7, "Plugin PropertiesFile %s exists in userfiles dir. User Overwrite option.\n", propFile);
			rc = 6171;
			ism_common_setErrorData(rc, "%s%s", "File", "PropertiesFile");
			return rc;
		}
    }

CHECK_CERTEXIST_END:
    return rc;
}


/*
 * Item: plugin
 * Component: Protocol
 *
 * Description:
 *
 * Schema:
 * {
 * 		"Plugin": {
 *      	"Name":"string",
 *			"File":"string",
 *			"PropertiesFile":"String"
 *		}
 *	}
 *
 *
 * Component callback(s):
 * - Protocol
 * Validation rules:
 *
 *
 */
XAPI int32_t ism_config_validate_Plugin(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
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

    /* Validate configuration items */
    int overwrite = -1;
    char *zipFile = NULL;
    char *propFile = NULL;

	/* Plugin items should be validated using current post */
	json_t *validateObj = NULL;
	json_t *postObj = json_object_get(currPostObj, item);
	if ( postObj ) {
	    validateObj = json_object_get(postObj, name);
	}

    if ( ismCUNITEnv != 0 ) {
    	validateObj = mergedObj;
    }

	if ( !validateObj ) {
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    /* Iterate thru the node */
    void *itemIter = json_object_iter(validateObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);
        int objType = json_typeof(value);

		if ( !strcmp(key, "Overwrite")) {
			if ( objType != JSON_TRUE && objType != JSON_FALSE ) {
				/* Return error */
			    ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item?item:"null", "null", key, ism_config_json_typeString(objType));
			    rc = ISMRC_BadPropertyType;
			    goto VALIDATION_END;
			}
		   	if ( objType == JSON_TRUE )
		   		overwrite = 1;
		   	else
		   		overwrite = 0;

		   	itemIter = json_object_iter_next(validateObj, itemIter);
		   	continue;
		}

		rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
		if (rc != ISMRC_OK) goto VALIDATION_END;

		if ( !strcmp(key, "File")) {
			if ( objType != JSON_STRING ) {
				/* Return error */
			    ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item?item:"null", "null", key, ism_config_json_typeString(objType));
			    rc = ISMRC_BadPropertyType;
			    goto VALIDATION_END;
			}
			zipFile = (char *)json_string_value(value);

		} else if ( !strcmp(key, "PropertiesFile")) {
			if ( objType != JSON_STRING ) {
				/* Return error */
			    ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", item?item:"null", "null", key, ism_config_json_typeString(objType));
			    rc = ISMRC_BadPropertyType;
			    goto VALIDATION_END;
			}
			propFile = (char *)json_string_value(value);
		}

	    /* validate special cases related to this object here*/
        itemIter = json_object_iter_next(validateObj, itemIter);
    }

	if (overwrite != -1) {
		if ( json_object_get(mergedObj, "Overwrite")) {
	        json_object_del(mergedObj, "Overwrite");
		}
	}


    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Add missing default values */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    if ( ismCUNITEnv == 0 ) {
		/* check if files exists. If the file is in /tmp/userfile, isNewFile will be set */
		rc = checkPluginFileExist(name, zipFile, propFile, overwrite);
    }


VALIDATION_END:

	TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;

}
