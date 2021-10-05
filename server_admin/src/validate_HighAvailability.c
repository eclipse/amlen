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
#include "configInternal.h"
#include "validateInternal.h"


/*
 * Component: Store
 * Item: HighAvailability
 *
 * Validation rules:
 * - Fixed name object, created by default in disabled state
 * - Can have only one instance - use can not delete this object
 * - .....
 *
 * Schema:
 *
 *   {
 *       "HighAvailability": {
 *           "[haconfig]": {
 *               "EnableHA":"boolean",
 *               "StartupMode":"string",             (AutoDetect,StandAlone)
 *               "RemoteDiscoveryNIC":"string",      (IP address)
 *               "LocalReplicationNIC":"string",     (IP address)
 *               "LocalDiscoveryNIC":"string",       (IP address)
 *               "DiscoveryTimeout":"integer",       (10-2147483647 - default 600)
 *               "PreferredPrimary:"boolean",
 *               "HeartbeatTimeout":"integer",       (1-2147483647 - default 10)
 *               "Group":"string", *                 (max length: 128)
 *               "ExternalReplicationNIC":"",        (IP Address)
 *               "ExternalReplicationPort":0,        (1024-65535)
 *               "ReplicationPort":0,                (1024-65535)
 *               "RemoteDiscoveryPort":0             (1024-65535)
 *           }
 *       }
 *   }
 *
 * NOTE: "haconfig" is fixed.
 *
 */

#if 0
/* Check if HA is enabled */
static int ism_config_validate_isGroupUpdateAllowed(json_t *mergedObj, const char *group) {
    int isUpdateAllowed = 2; /* update config */

    /* check current server state - if it is in Maintenence mode - allow Group change */
    int state = ism_admin_get_server_state();
    if ( state == ISM_SERVER_MAINTENANCE ) return isUpdateAllowed;

    /* get current HA role */
    ismHA_View_t haView = {0};
    ism_ha_store_get_view(&haView);

    /* Compare current group with set group */
    const char *groupCFG = json_string_value(json_object_get(mergedObj, "Group"));
    json_t *haEnabled = json_object_get(mergedObj, "EnableHA");

    if ( groupCFG && strcmp(groupCFG, group)) {
        if ( haView.NewRole == ISM_HA_ROLE_PRIMARY && haView.SyncNodesCount == 2 ) {
            isUpdateAllowed = 1; /* update config and sync */
        } else {
            if ( json_typeof(haEnabled) == JSON_TRUE ) {
                if ( haView.NewRole == ISM_HA_ROLE_DISABLED ) {
                    isUpdateAllowed = 2; /* update config */
                } else {
                    isUpdateAllowed = 0; /* update not allowed */
                }
            }
        }
    }

    TRACE(7, "Check Group Update: setGroup:%s confGroup:%s role:%d syncnodes:%d allowed:%d\n",
                    group?group:"", groupCFG?groupCFG:"", haView.NewRole, haView.SyncNodesCount, isUpdateAllowed);
    return isUpdateAllowed;
}
#endif

XAPI int32_t ism_config_validate_HighAvailability(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int ignore_action, ism_prop_t *ignore_props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value = NULL;
    int objType = 0;

    TRACE(9, "Entry %s: currPostObj:%p, mergedObj:%p, item:%s, name:%s\n",
        __FUNCTION__, currPostObj?currPostObj:0, mergedObj?mergedObj:0, item?item:"null", name?name:"null");

    if ( !mergedObj || !name ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Validate name - name can only be haconfig */
    if ( name && strcmp(name, "haconfig") ) {
        rc = ISMRC_BadOptionValue;
        ism_common_setErrorData(rc, "%s%s%s", "HighAvailability", "Name", name);
        goto VALIDATION_END;
    }

    /* In current post, check AdminEndpoint delete is requested - it is not allowed */
    json_t *aepObj = json_object_get(currPostObj, "HighAvailability");
    if ( !aepObj ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }
    if ( aepObj && json_typeof(aepObj) == JSON_NULL ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "HighAvailability");
        goto VALIDATION_END;
    }

    /* check for JSON_OBJECT */
    objType = json_typeof(aepObj);
    if ( objType != JSON_OBJECT ) {
        rc = ISMRC_PropertiesNotValid;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }


    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    int rdndefined = 0;
    int ldndefined = 0;
    int lrndefined = 0;
    int nicsdefined = 0;
    char *groupStr = NULL;
    int   enableHA = 0;

    /* Validate for one instance only and optional object name (name should be AdminEndpoint) */
    json_object_foreach(mergedObj, key, value) {
        if (json_typeof(value) == JSON_OBJECT) {
            /* size of mergedObj can not be more than 1 */
            if ( json_object_size(mergedObj) > 1 ) {
                /* Only one instance of AdminEndpoint is allowed */
                rc = ISMRC_TooManyItems;
                ism_common_setErrorData(ISMRC_TooManyItems, "%s%d%d", "HighAvailability", 2, 1);
                goto VALIDATION_END;

            }
        }
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(mergedObj) == JSON_NULL || json_object_size(mergedObj) == 0 ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "HighAvailability");
        goto VALIDATION_END;
    }

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);
        objType = json_typeof(value);

        if ( objType == JSON_INTEGER && (!strcmp(key, "RemoteDiscoveryPort") || !strcmp(key, "ReplicationPort") || !strcmp(key, "ExternalReplicationPort"))) {
            int port = json_integer_value(value);
            if ( port == 0 || ( port >= 1024 && port <= 65535)) {
                rc = ISMRC_OK;
            } else {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%d", key, port);
                rc = ISMRC_BadPropertyValue;
                goto VALIDATION_END;
            }
        } else {
            rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
            if (rc != ISMRC_OK) {
            	TRACE(9, "ism_config_validateItemDataJson returned %d, key=%s\n", rc, key);
            	goto VALIDATION_END;
            }
        }

        if ( objType == JSON_STRING ) {
        	char *ipval = (char *)json_string_value(value);
            if ( !strcmp(key, "ExternalReplicationNIC")) {
            	/* Only allow a single IP address */
				if ( ipval && (strstr(ipval, ","))) {
					ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, ipval);
					rc = ISMRC_BadPropertyValue;
					goto VALIDATION_END;
				}
            } else if ( !strcmp(key, "RemoteDiscoveryNIC") ) {
            	/* Only allow a single IP address and no empty string */
				if ( ipval && (strstr(ipval, ","))) {
					ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, ipval);
					rc = ISMRC_BadPropertyValue;
					goto VALIDATION_END;
				}

				if ( ipval ) {
					if ( *ipval != '\0' ) {
						rdndefined = 2;
					    nicsdefined += 1;
					} else {
						rdndefined = 1;
					}
				}

            } else if ( !strcmp(key, "LocalReplicationNIC") ) {
            	/* Only allow a single IP address and no empty string */
				if ( ipval && (strstr(ipval, ","))) {
					ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, ipval);
					rc = ISMRC_BadPropertyValue;
					goto VALIDATION_END;
				}

				if ( ipval ) {
					if ( *ipval != '\0' ) {
					    lrndefined = 2;
					    nicsdefined += 1;
					} else {
						lrndefined = 1;
					}
				}

            } else if ( !strcmp(key, "LocalDiscoveryNIC") ) {
            	/* Only allow a single IP address and no empty string */
				if ( ipval && (ipval && (strstr(ipval, ",")))) {
					ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, ipval);
					rc = ISMRC_BadPropertyValue;
					goto VALIDATION_END;
				}

				if ( ipval ) {
					if ( *ipval != '\0' ) {
						ldndefined = 2;
					    nicsdefined += 1;
					} else {
						ldndefined = 1;
					}
				}

            } else if (!strcmp(key, "Group")) {
            	groupStr = (char *)json_string_value(value);
            }
        }

        if ( objType == JSON_TRUE && !strcmp(key, "EnableHA")) {
            enableHA = 1;
        }

        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Check if required items are specified */
    int chkMode = 0;
    rc = ism_config_validate_checkRequiredItemList(reqList, chkMode );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Group should be defined if HA is enabled */
    if ( enableHA == 1 ) {
        if ( groupStr == NULL || *groupStr == '\0' ) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Group", groupStr?groupStr:"NULL");
            rc = ISMRC_BadPropertyValue;
            goto VALIDATION_END;
        }
    }

    if ( enableHA == 1  && nicsdefined != 3 ) {
    	if ( rdndefined != 2 ) {
			rc = ISMRC_PropertyRequired;
			ism_common_setErrorData(rc, "%s%s", "RemoteDiscoveryNIC", rdndefined?"\"\"":"null");
			goto VALIDATION_END;
    	}

    	if ( lrndefined != 2 ) {
			rc = ISMRC_PropertyRequired;
			ism_common_setErrorData(rc, "%s%s", "LocalReplicationNIC", lrndefined?"\"\"":"null");
			goto VALIDATION_END;
    	}

    	if ( ldndefined != 2 ) {
			rc = ISMRC_PropertyRequired;
			ism_common_setErrorData(rc, "%s%s", "LocalDiscoveryNIC", ldndefined?"\"\"":"null");
			goto VALIDATION_END;
        }
    }

    /* set HA disable global flag */
    ism_config_json_disableHACheck(enableHA?0:1);

    /* Add missing default values or rest all items to default for HighAvailability */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}
