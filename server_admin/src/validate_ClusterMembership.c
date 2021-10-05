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
#define OBJECT_TYPE "ClusterMembership"

#include <validateConfigData.h>
#include "validateInternal.h"
#include "cluster.h"
#include <arpa/inet.h>


extern json_t * ism_config_json_getCompositeItem(const char *object, const char *name, char *item, int getLock);

/*
 * Component: Cluster
 * Item: ClusterMembership
 *
 * Validation rules:
 *
 * User defined Configuration Item(s):
 * ----------------------------------
 * Name: "clusterMembershipConfig", Hard-coded name. User can not set the name.
 *
 * Update: Data type is boolean. Options: True, False. Default: False
 *       If set to True, object should be in the configuration file.
 *       If set to False, object should not be in the configuration file.
 *
 * Internal Configuration Item(s):
 * ------------------------------
 *
 * Unused configuration Item(s):
 * ----------------------------
 *
 *
 * Dependent validation rules:
 * --------------------------
 *
 *
 */


static char *cm_cluster_invalid="\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1e\x1f\x7f,\"\\/=";

/*
 * Item: ClusterMembership
 *
 * Description:
 * - ClusterMembership is a named object
 *
 * Schema:
 *{
 *    "ClusterMembership":{
 *        "EnableClusterMembership":"boolean",
 *        "ClusterName":"string",
 *        "UseMulticastDiscovery":"boolean",
 *        "MulticastDiscoveryTTL":"number",
 *        "DiscoveryServerList":"string",
 *        "ControlAddress":"string",
 *        "ControlPort":"number",
 *        "ControlExternalAddress":"string",
 *        "ControlExternalPort":"number",
 *        "MessagingAddress":"string",
 *        "MessagingPort":"number",
 *        "MessagingExternalAddress":"string",
 *        "MessagingExternalPort":"number",
 *        "MessagingUseTLS":"boolean",
 *        "DiscoveryPort":"number",
 *        "DiscoveryTime":"number",
 *        "RequireCertificates":boolean"
 *    }
 *}
 *
 * Where:
 *
 * EnableClusterMembership : Identifies whether clustering is enabled.
 * ClusterName            : The name of the cluster. Required.
 * UseMulticastDiscovery  : Identifies whether cluster members are in a list or
 *                          discovered by multicast. Optional.
 * MulticastDiscoveryTTL  : When UseMulticastDiscovery is true, the number of
 *                          routers (hops) that multicast traffic is allowed to
 *                          pass through. Optional.
 * DiscoveryServerList    : Comma-delimited list of members belonging to the
 *                          cluster this server wants to join. Required if
 *                          UseMulticastDiscovery is False.
 * ControlAddress         : The IP Address of the control interface. Required if
 *                          EnableClusterMembership is True.
 * ControlPort            : The port number to use for the ControlInterface.
 *                          Though required, does not need to be specified
 *                          because there is a default.
 * ControlExternalAddress : The external IP Address or Hostname of the control interface
 *                          if different from ControlAddress.
 * ControlExternalPort    : The external port number to use for the ControlExternalAddress.
 * MessagingAddress       : The IP Address of the messaging channel, if different from
 *                          the ControlAddress.
 * MessagingPort          : The port number to use for the MessagingInterface, must be
 *                          different from the ControlPort.
 * MessagingExternalAddress : The external IP Address or Hostname of the messaging channel
 *                          if different from MessagingAddress.
 * MessagingExternalPort  : The external port number to use for the MessagingExternalAddress.
 * MessagingUseTLS        : Specify if the messaging channel should use TLS.
 * DiscoveryPort          : Port number used for multicast discovery. This port must
 *                          be identical for all members of the cluster.
 * DiscoveryTime          : The time, in seconds, that Cluster spends during server
 *                          start up to discover other servers in the cluster and
 *                          get updated information from them."
 * RequireCertificates    : If using TLS are certificates required
 *
 * Component callback(s):
 * - Cluster, Transport
 *
 */

XAPI int32_t ism_config_validate_ClusterMembership(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    json_type objType;

    TRACE(9, "Entry %s: currPostObj:%p, mergedObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, mergedObj?mergedObj:0, item?item:"null", name?name:"null", action);

    if ( !mergedObj ) {
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }


    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Validate for one instance only and optional object name (name should be AdminEndpoint) */
    json_object_foreach(mergedObj, key, value) {
        if (json_typeof(value) == JSON_OBJECT) {
            /* size of mergedObj can not be more than 1 */
            if ( json_object_size(mergedObj) > 1 ) {
                /* Only one instance of ClusterMembership is allowed */
                rc = ISMRC_TooManyItems;
                ism_common_setErrorData(ISMRC_TooManyItems, "%s%d%d", item, 2, 1);
                goto VALIDATION_END;

            }
        }
    }

    /* Check if request has any data - delete is not allowed */
    if ( action == 2 || json_typeof(mergedObj) == JSON_NULL || json_object_size(mergedObj) == 0 ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", item);
        goto VALIDATION_END;
    }

    /* Validate Name - though ClusterMembership has fixed name, still validate so that reqList is updated correctly */
    rc = ism_config_validateItemData( reqList, "Name", name, action, NULL);
    if ( rc != ISMRC_OK ) {
        ism_common_setError(rc);
        goto VALIDATION_END;
    }
    if (strcmp(name, "cluster")) {
    	rc = ISMRC_BadPropertyValue;
    	ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", item, name);
    	goto VALIDATION_END;
    }

    /* Validate configuration items */
    int enabled = 0;
    char *clusterName = NULL;
    int hasControlAddress = 0;
    char *discoveryServerList = NULL;
    int umd = 1;

    /* Get old value of Enabled flag */
    int oldEnabled = 0;
    json_t * oldEnableObj = ism_config_json_getCompositeItem("ClusterMembership", "cluster", "EnableClusterMembership", 0);
    if ( oldEnableObj && json_typeof(oldEnableObj) == JSON_TRUE ) oldEnabled = 1;

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);
        objType = json_typeof(value);

        if ( objType == JSON_NULL &&
              ( !strcmp(key, "DiscoveryServerList") ||
                !strcmp(key, "ControlExternalAddress") ||
                !strcmp(key, "MessagingAddress") ||
                !strcmp(key, "MessagingExternalAddress") ))
        {
            /* Allow null */
            rc = ISMRC_OK;
        } else {
		    rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        }
		if (rc != ISMRC_OK) goto VALIDATION_END;

	    /* validate special cases related to this object here*/

		if (objType != JSON_NULL) {
	    	char *propValue = NULL;
	    	if (objType == JSON_STRING) propValue = (char *)json_string_value(value);
		    if (!strcmp(key, "DiscoveryServerList")) {
		    	discoveryServerList = propValue;
	            int len = NULL == propValue ? 0 : strlen(propValue);

		    	char *tmpstr = (char *)alloca(len+1);
		        memcpy(tmpstr, propValue, len);
		        tmpstr[len] = 0;

		        char *token, *nexttoken = NULL;
		        for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL;
		                token = strtok_r(NULL, ",", &nexttoken))
		        {
		        	// format is IP:PORT
                    char *rest = token;
		        	// Check for IPv6
		        	if ('[' == *rest) {
		        		// Find end
		        		char *end = strstr(rest, "]");
		        		// Next character must be ":" to separate port
		        		if (!end || ':' != *(end+1)) {
			    			ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", key, propValue);
			        		rc = ISMRC_PropertyRequired;
				        	goto VALIDATION_END;
		        		}
		        		*end = 0;
		    			struct in6_addr low6;
						int rc2 = inet_pton(AF_INET6, rest+1, &low6);
						if ( rc2 != 1 ) {
			    			ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", key, propValue);
			        		rc = ISMRC_PropertyRequired;
				        	goto VALIDATION_END;
						}
		        		// Validate port
		        		rc = ism_config_validateDataType_number((char *)key, end+2, "0", "65535", NULL);
		        		if (rc) {
			    			ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", key, propValue);
			        		rc = ISMRC_PropertyRequired;
				        	goto VALIDATION_END;
		        		}
		        	} else {
		        		// IPv4
		        		char *end = strstr(rest, ":");
		        		if (!end) {
			    			ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", key, propValue);
			        		rc = ISMRC_PropertyRequired;
				        	goto VALIDATION_END;
		        		}
		        		*end = 0;
		    			struct sockaddr_in ipaddr;
		    			int rc4 = 0;
		    			rc4 = inet_pton(AF_INET, rest, &ipaddr.sin_addr);
		    			if (rc4 != 1 )  {
			    			ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", key, propValue);
			        		rc = ISMRC_PropertyRequired;
				        	goto VALIDATION_END;
		    			}
		        		// Validate port
		        		rc = ism_config_validateDataType_number((char *)key, end+1, "0", "65535", NULL);
		        		if (rc) {
			    			ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", key, propValue);
			        		rc = ISMRC_PropertyRequired;
				        	goto VALIDATION_END;
		        		}
		        	}
		        }
		    } else if (!strcmp(key, "ControlAddress") || !strcmp(key, "MessagingAddress")) {
		    	if (propValue && strlen(propValue)>0) { // Empty is fine, for now
		    		rc = ism_config_validate_IPAddress(propValue, 1);
		    		if (ISMRC_OK != rc) {
		    			TRACE(2, "The IP address for %s is not valid: %s\n", key, propValue);
		    			rc = ISMRC_BadPropertyValue;
		    			ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propValue);
		    			goto VALIDATION_END;
		    		}

		    		if (!strcmp(key, "ControlAddress")) hasControlAddress = 1;
		    	}
		    } else if (!strcmp(key, "EnableClusterMembership") || !strcmp(key, "Enabled")) {
		    	if (JSON_TRUE == objType) {
		    		enabled = 1;
		    	}
		    } else if (!strcmp(key, "ClusterName")) {
		    	clusterName = propValue;
		    } else if (!strcmp(key, "UseMulticastDiscovery")) {
		    	if (JSON_FALSE == objType) {
		    		umd = 0;
		    	}
		    } else if (!strcmp(key, "ControlExternalAddress") || !strcmp(key, "MessagingExternalAddress")) {
		    	if (propValue && *propValue) {
		    		if (strchr(propValue, '[')) {
		    			rc = ISMRC_BadPropertyValue;
		    			ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propValue);
		    			goto VALIDATION_END;
		    		}
		    	}
		    }
	    }
        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Check if required items are specified */
    int chkMode = 1;
    rc = ism_config_validate_checkRequiredItemList(reqList, chkMode );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Validate if other required items are specified, if ClusterMemebership is enabled */
    if (1 == enabled) {
   		if ( NULL == clusterName || *clusterName == '\0' ) {
	        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ClusterName", "NULL");
	        rc = ISMRC_BadPropertyValue;
	        goto VALIDATION_END;
    	}

   		/* Validate ClusterName
    	 * The ClusterName must not have leading or trailing spaces and cannot contain control
    	 * characters, commas, double quotation marks, backslashes, slashes, or equal signs.
    	 */
   		if (strcspn(clusterName, cm_cluster_invalid) != strlen(clusterName)
   				|| ' ' == clusterName[0] || ' ' == clusterName[strlen(clusterName)-1]) {
	        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ClusterName", clusterName);
	        rc = ISMRC_BadPropertyValue;
	        goto VALIDATION_END;
    	}

   		/* Check for ControlAddress */
        if ( hasControlAddress == 0 ) {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ControlAddress", "NULL");
            goto VALIDATION_END;
        }

	    /* if UseMulticastDiscovery is false, then there should be a valid DiscoveryServerList */
	    if ( umd == 0 ) {
    	    if ( !discoveryServerList || *discoveryServerList == '\0' ) {
    	        rc = ISMRC_BadPropertyValue;
    		    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "DiscoveryServerList", "NULL");
    		    goto VALIDATION_END;
    	    }
	    }
	}


    /* Add missing default values */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);

    if (rc) goto VALIDATION_END;

    /* prevent ClusterName change if ClusterMembership is enabled */
    /* get old name */
    json_t * oldName = ism_config_json_getCompositeItem("ClusterMembership", "cluster", "ClusterName", 0);
    if ( oldName ) {
    	char *oldNameStr = (char *)json_string_value(oldName);
    	if ( oldNameStr && *oldNameStr != '\0' && clusterName ) {
    		if (oldEnabled == 1 && enabled == 1 && strcmp(clusterName, oldNameStr)) {
    			rc = ISMRC_UpdateNotAllowed;
    			ism_common_setErrorData(rc, "%s%s", "ClusterMembership", "ClusterName");
    		}
    	}
    }

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

