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
#include <validateConfigData.h>

#include "validateInternal.h"


/*
 * Top level function to validate any set action request sent to admin port.
 *
 * This function checks the type of the object and invokes validation function
 * of the object.
 *
 * NOTES for developers:
 *
 * 1. Identify the object name, and add a hook in ism_config_validate_object() API
 *    to invoke validation function of the object.
 * 2. For composite object validation, create a file validate_<objectName>.c and add
 *    validation function in the file. Use the following naming convention
 *    for function prototype:
 *
 *    int32_t ism_config_validateDataType_<name>(ism_json_parse_t *json, char *inpbuf, int inpbuflen)
 *
 *    In typical signature, arguments will same as received by  ism_config_validate_object().
 *
 *    Function should returns ISMRC_OK on success, ISMRC_* on error
 *
 *    Use validate_genericData.c file as a template.
 *
 * 2. Add function prototype in include/validateConfigData.h file.
 *    Add function validation rules in this file.
 *
 * 3. If there are any internal data, functions etc, define these in validate_<objectName>.c file
 *    unless the function is of generic nature and can be reused for validation of other objects.
 *    If any function can be reused, add these functions in validate_internal.c.
 *
 * 4. Generic data validations are in validate_genericData.c file. Object validation should use these
 *    validation function.
 *
 * 5. Create CUNIT test in validate_<objectName>_test.c file in test directory
 *
 * 6. Update CUNIT header file admin_test.h to include generic data cunit tests
 *
 */
XAPI int32_t ism_config_validate_object(int actionType, ism_json_parse_t *json, char *inpbuf, int inpbuflen, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    //action 0 -- create, 1 -- update, 2 -- delete
    int action = 0;
    char *component = NULL;

    TRACE(9, "Entry %s: json: %p, inpbuf: %s, inpbuflen: %d\n",
    	__FUNCTION__, json?json:0, inpbuf?inpbuf:"null", inpbuflen);
    		

    /* check for NULL pointer */
    if ( !json || !inpbuf || *inpbuf == '\0' || inpbuflen == 0 ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    /* Check component and object type */
    //char *component = (char *)ism_json_getString(json, "Component");
    char *item = (char *)ism_json_getString(json, "Item");
    rc = ism_config_getComponent(ISM_CONFIG_SCHEMA, item, &component, NULL);
    //if ( !component || *component == '\0' || !item || *item == '\0' ) {
    if (rc != ISMRC_OK) {
        TRACE(3, "NULL component=%s or item=%s\n", component?component:"", item?item:"");
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    /* have to do this for reason in defect 86363*/
    char *delete = (char *)ism_json_getString(json, "Delete");
    if (delete && !strcmpi(delete, "True") ) {
    	action = 2;
    }

    char *update = (char *)ism_json_getString(json, "Update");
	if (update && !strcmpi(update, "True") ) {
		if (action == 2) {
	    	TRACE(3, "%s: Both Delete and Update flags have been set.\n", __FUNCTION__);
	    	rc = ISMRC_PropertiesNotValid;
	    	ism_common_setError(rc);
	        goto VALIDATION_END;
		} else {
	    	action = 1;
		}
	}
    
	ism_json_parse_t *schemajson = ism_config_getSchema(ISM_CONFIG_SCHEMA);
	int pos = ism_json_get(schemajson, 0, item);
    /* Add hook to call object validation function */
    if ( pos != -1 ) {
        char *objectType = ism_config_validate_getAttr("ObjectType", schemajson, pos);
    	if (objectType && !strcmp("Singleton", objectType)) {
     		char *value = (char *)ism_json_getString(json, "Value");
     		if ( !strcmp(item, "TraceLevel") || !strcmp(item, "TraceSelected")
            		|| !strcmp(item, "TraceConnection") || !strcmp(item, "TraceMax")
            		|| !strcmp(item, "TraceOptions")
            		|| !strcmp(item, "TraceBackupDestination")
            		|| !strcmp(item, "TraceModuleLocation")
            		|| !strcmp(item, "TraceModuleOptions")) {
                /* validate trace related configuration items */
                rc = ismcli_validateTraceObject(0, item, value);
                if ( rc != ISMRC_OK ) {
                	/* Only returns ISMRC_BadPropertyValue or ISMRC_BadPropertyName */
                	if (rc == ISMRC_BadPropertyValue)
                		ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", item, value==NULL ? "NULL" : value);
                	else
                		ism_common_setErrorData(ISMRC_BadPropertyName, "%s", item);
                }
            } else if (!strcmp(item, "ServerUID")) {
            	TRACE(2, "Unable to set ServerUID.\n");
            	rc = ISMRC_BadPropertyName;
            	ism_common_setErrorData(ISMRC_BadPropertyName, "%s", "ServerUID");
            }
            goto VALIDATION_END;
    	}
    }
    
    /* Validate composite object MessageHub */

    /* Validate composite object CRLProfile */
    /*if ( strcmp(item, "CRLProfile") == 0 ) {
        rc = ism_config_validate_CRLProfile(json, component, item, action, inpbuf, inpbuflen, props);
        goto VALIDATION_END;
    }*/

    /* Validate composite object ClientSet */
    if ( strcmp(item, "ClientSet") == 0 ) {
        rc = ism_config_validate_ClientSet(json, component, item, actionType, inpbuf, inpbuflen, props);
        goto VALIDATION_END;
    }
    

VALIDATION_END:
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);  
    if (component) ism_common_free(ism_memory_admin_misc,component);
    
    return rc;
}

