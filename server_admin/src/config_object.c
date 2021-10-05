/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

/*
 * This file contains ISM server configuration APIs.
 * The APIs handle both static and dynamic configuration options.
 */

#define TRACE_COMP Admin

#include "configInternal.h"
#include "validateInternal.h"

extern const char *configDir;
extern pthread_mutex_t g_cfgfilelock;
extern pthread_mutex_t g_cfglock;
extern int disableSet;

extern int ism_config_purgeCompProp(char *compname, int force);

static int32_t ism_config_updateCfgFile(const char * fileName, int proctype) {
    int32_t rc = ISMRC_OK;
    char bfilepath[1024];
    char cfilepath[1024];
    char ofilepath[1024];
    char tfilepath[1024];

    if (!fileName)  {
        TRACE(2, "A NULL pointer is passed for the configuration file name.\n");
        ism_common_setError(ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }

    pthread_mutex_lock(&g_cfgfilelock);

    sprintf(cfilepath, "%s/%s", configDir, fileName);
    sprintf(ofilepath, "%s/%s.org", configDir, fileName);
    sprintf(bfilepath, "%s/%s.bak", configDir, fileName);
    sprintf(tfilepath, "%s/%s.tmp", configDir, fileName);

    if ( access(ofilepath, F_OK) == -1 ) {
        TRACE(5, "Make a copy of initial configuration file %s.\n", ofilepath);
        copyFile(cfilepath, ofilepath);
    }

    /* dump content of current configuration to a .temp file */
    if ( (rc = ism_config_dumpConfig(tfilepath, proctype)) == ISMRC_OK ) {
        /* rename current to .bak */
        if ( rename(cfilepath, bfilepath) == 0 ) {
            if ( rename( tfilepath, cfilepath ) != 0 ) {
                TRACE(2, "Could not rename temporary configuration to current. rc=%d\n", errno);
                rc = ISMRC_Error;
            }
        } else {
            TRACE(2, "Could not rename current configuration file to a backup file. rc=%d\n", errno);
            rc = ISMRC_Error;
        }
    }
    if (rc)
        ism_common_setError(rc);

    pthread_mutex_unlock(&g_cfgfilelock);
    return rc;
}


/*
 *
 * Set dynamic config from ism_prop_t object
 *
 * @param  json                   The JSON string for set configuration
 * @param  validateData           The flag indicates get handle action
 * @param  inpbuf                 The original input string
 * @param  inpbuflen              The input string length
 * @param  props                  The configuration properties will be created from input string
 * @param  storeOnStandby         The flag about store status
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int32_t ism_config_set_object(ism_json_parse_t *json, int validateData, char *inpbuf, int inpbuflen, ism_prop_t *props, int storeOnStandby) {
    int32_t rc = ISMRC_OK;
    /*action  0: create
     *        1: update
     *        2: delete
     */
    int action = 0;
    int setAdminMode = 0;
    int isHAConfig = 0;
    int frc = 0;
    int composite = 0;
    int purgeCompProp = 0;
    int mode = ISM_CONFIG_CHANGE_PROPS;

    /* create json string for endpoint object before action against it */
	char eptmpbuf[16384];
	concat_alloc_t epbuf = { eptmpbuf, sizeof(eptmpbuf), 0, 0 };


    const char *name = NULL;

    TRACE(7, "Entry %s: json: %p, validateData: %d, inpbuf: %s, inpbuflen: %d, props: %p, storeOnStandby: %d\n",
    	__FUNCTION__, json?json:0, validateData, inpbuf?inpbuf:"null", inpbuflen, props?props:0, storeOnStandby);

    if ( disableSet == 1 ) {
		TRACE(2, "Node is running in standby mode. Dynamic set is disabled\n");
		rc = ISMRC_ConfigNotAllowed;
		ism_common_setError(rc);
		goto CONFIG_END;
	}

    /* check for NULL pointer */
    if ( !json ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto CONFIG_END;
    }

    /* Check component and object type */
    //char *component = (char *)ism_json_getString(json, "Component");
    char *item = (char *)ism_json_getString(json, "Item");
    char *component = NULL;
    rc = ism_config_getComponent(ISM_CONFIG_SCHEMA, item, &component, NULL);
    if (rc != ISMRC_OK) {
        TRACE(3, "NULL component=%s or item=%s\n", component?component:"", item?item:"");
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto CONFIG_END;
    }

    /* set AdminMode flag */
    if (!strcmpi(item, "AdminMode")) setAdminMode = 1;

    /* set HAConfig flag */
    char *forceStr = (char *)ism_json_getString(json, "StandbyForce");
    if ( forceStr && *forceStr == 'T' ) frc = 1;
	if ( frc == 0 && !strcmpi(component, "HA") && !strcmpi(item, "HighAvailability") ) {
		isHAConfig = 1;
	}

	/* set composite flag */
	char *type = (char *)ism_json_getString(json, "Type");
    if ( type && strcasecmp(type,"composite") == 0) {
        composite = 1;
    }

    /* check if handle has been registered */
    int comptype = ism_config_getCompType(component);
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle && validateData == 0) {
        ism_config_register(comptype, NULL, NULL, &handle);
        if ( !handle ) {
            /* Invalid component - return error */
        	rc = ISMRC_InvalidComponent;
            ism_common_setError(ISMRC_InvalidComponent);
            TRACE(3, "%s: register component: %s failed.\n", __FUNCTION__, component);
            goto CONFIG_END;
        }
    } else if ( !handle )  {
        /* try for object specific handle */
        handle = ism_config_getHandle(comptype, item);
        if ( !handle ) {
			/* Invalid component - return error */
			rc = ISMRC_InvalidComponent;
			ism_common_setError(ISMRC_InvalidComponent);
			TRACE(3, "%s: Register Component: %s failed.\n", __FUNCTION__, component);
			goto CONFIG_END;
        }
    }

#if 0
    /* check for new name */
    const char *newname = ism_common_getStringProperty(props, "NewName");
    if ( newname && *newname != '\0' ) {
        mode = ISM_CONFIG_CHANGE_NAME;
        //name = newname;
    }

    /* ISM_CONFIG_CHANGE_NAME has not been implemented */
    if ( mode == ISM_CONFIG_CHANGE_NAME ) {
        ism_common_setError(ISMRC_NotImplemented);
        rc = ISMRC_NotImplemented;
        goto CONFIG_END;
    }
#endif


    name = ism_common_getStringProperty(props, "Name");

    /* check set action. 0 - create, 1 - update, 2 - delete */
    char *update = (char *)ism_json_getString(json, "Update");
	if ( update && strncasecmp(update, "true", 4) == 0 ) {
		action = 1;
	}

    char *delete = (char *)ism_json_getString(json, "Delete");
    if ( delete && strncasecmp(delete, "true", 4) == 0 ) {
    	mode = ISM_CONFIG_CHANGE_DELETE;
    	action = 2;
    	if (!strcmp(item, "ClusterMembership")) {
    		rc = ISMRC_DeleteNotAllowed;
    		ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", item);
    		goto CONFIG_END;
    	}
    }
    
    /* lock the config repository */
    pthread_mutex_lock(&g_cfglock);

	/* Get component property list */
	ism_prop_t *cprops = compProps[handle->comptype].props;

	rc = ism_config_updateConfigRepository(handle, item, (char *)name, mode, props, cprops, &purgeCompProp, action);
	if (rc != ISMRC_OK)  {
		goto CONFIG_END;
	}


	/*
	 * TODO:
	 * Endpoint configuration is a special case.
	 * For HA
	 * For deleting action, need to load Endpoint current configuration into epbuf first
	 * For updating/create, need to load configuration into epbuf after local operation
	 * The epbuf willb be used in HASyncUp API
	 *
	    int rcEPGetConfig = ISMRC_OK;
    	int getConfigLock = 0;
    	memset(eptmpbuf, '0', 16384);
    	if (action == 2 ) {
    		if ( isHAConfig == 0 && ism_ha_admin_isUpdateRequired() == 1 && !strcmpi(item, "Endpoint") ) {
    			rcEPGetConfig = ism_config_getObjectConfig("Transport", "Endpoint", (char *)name, NULL, 1, &epbuf, getConfigLock, 1);
    			if ( rcEPGetConfig != ISMRC_OK ) {
    				TRACE(3, "%s: Could not create config JSON string for Endpoint %s. rc=%d", __FUNCTION__, name, rcEPGetConfig);
    				goto CONFIG_END;
    			}
    		}
    	else {
            //do updateConfigRepository first.
    	}
     */

    /* HA sycn-up if needed */
    rc = ism_config_HASyncUp(item, action, composite, setAdminMode, isHAConfig, inpbuf, inpbuflen, epbuf);
    if (rc == ISMRC_OK) {
        /* update config file if HA update is OK */
        int32_t pType = ism_admin_getServerProcType();
        char *cFname = "mqcbridge_dynamic.cfg";
        char *dynFname = "server_dynamic.cfg";

        if ( storeOnStandby == 1 && comptype == ISM_CONFIG_COMP_MQCONNECTIVITY ) {
    		pType = ISM_PROTYPE_MQC;
    		ism_config_updateCfgFile(cFname, pType);
    	} else {
    		ism_config_updateCfgFile(dynFname, pType);
    	}

    	if ( purgeCompProp == 1 ) {
            int force = 0;
            if ( comptype == ISM_CONFIG_COMP_ENGINE ) {
                 force = 1;
            }
    		ism_config_purgeCompProp(component, force);
    	}
    }

CONFIG_END:
    pthread_mutex_unlock(&g_cfglock);
    if (component) ism_common_free(ism_memory_admin_misc,component);
    if (rc != ISMRC_OK) {
    	TRACE(3, "%s: set config action failed.\n", __FUNCTION__);
    }
    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);
    
    return rc;
}
