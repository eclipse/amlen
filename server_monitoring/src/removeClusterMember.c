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


#include <config.h>
#include <janssonConfig.h>
#include <validateConfigData.h>
#include <cluster.h>

/**
 * Removes an inactive cluster member
 */
int ism_monitoring_removeInactiveClusterMember(ism_http_t *http) {
    int rc = ISMRC_OK;
    int noErrorTrace = 0;
    json_t *post = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);
    if ( !post || rc != ISMRC_OK) {
        return rc;
    }

    char *sName = NULL;
    char *sUID = NULL;
    int foundItem = 0;
    int nameSpecified = 0;
    int uidSpecified = 0;

    json_t *objval = NULL;
    const char *objkey = NULL;
    json_object_foreach(post, objkey, objval) {
        foundItem += 1;
        if ( json_typeof(objval) == JSON_OBJECT ) {
            rc = ISMRC_InvalidObjectConfig;
            ism_common_setErrorData(rc, "%s", "inactiveClusterMember");
            break;
        }

        if ( !strcmp(objkey, "Version")) continue;

        if ( !strcmp(objkey, "ServerName")) {
            nameSpecified = 1;
            if ( json_typeof(objval) == JSON_STRING ) {
                char *tmpstr = (char *)json_string_value(objval);
                if ( tmpstr ) sName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_monitoring_misc,1000),tmpstr);

            }
        } else if ( !strcmp(objkey, "ServerUID")) {
            uidSpecified = 1;
            if ( json_typeof(objval) == JSON_STRING ) {
                char *tmpstr = (char *)json_string_value(objval);
                if( tmpstr ) sUID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_monitoring_misc,1000),tmpstr);
            }
        } else {
            rc = ISMRC_ArgNotValid;
            ism_common_setErrorData(rc, "%s", "inactiveClusterMember");
            break;
        }
    }

    json_decref(post);

    if ( foundItem == 0 ) {
        rc = ISMRC_InvalidObjectConfig;
        ism_common_setErrorData(rc, "%s", "inactiveClusterMember");
    }

    if ( rc != ISMRC_OK ) {
        if ( sName ) ism_common_free(ism_memory_monitoring_misc,sName);
        if ( sUID ) ism_common_free(ism_memory_monitoring_misc, sUID );
        return rc;
    }

    if ( nameSpecified == 0 || uidSpecified == 0 ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "ServerName and ServerUID", "");
        return rc;
    }

    if (!sName || *sName == '\0' || !sUID || *sUID == '\0') {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "ServerName and ServerUID", "");
        return rc;
    }

    TRACE(5, "Remove Inactive ClusterMember: ServerName:%s ServerUID:%s\n", sName, sUID);

    ismCluster_ViewInfo_t * info;
    int deleted = 0;

    if ( !ism_cluster_getView(&info)) {

        ismCluster_RSViewInfo_t * server = NULL;
        int i = 0;

        for (i=0; i<info->numRemoteServers; i++) {

            server = info->pRemoteServers+i;

            char *svrName = (char *)server->pServerName;
            char *srvUID = (char *)server->pServerUID;

            TRACE(9, "Check remote ClusterMember: ServerName:%s ServerUID:%s\n", svrName?svrName:"", srvUID?srvUID:"");

            int found = 0;
            if ( !strcmp(svrName, sName) && !strcmp(srvUID, sUID) )
                found = 1;

            if ( found == 1 ) {
                /* check status */
                if ( server->state == ISM_CLUSTER_RS_STATE_INACTIVE ) {
                    TRACE(9, "Found inactive ClusterMember: ServerName:%s ServerUID:%s\n", sName?sName:"", sUID?sUID:"");
                    const ismCluster_RemoteServerHandle_t phServerHandle = server->phServerHandle;
                    rc = ism_cluster_removeRemoteServer(phServerHandle);
                    if ( rc != ISMRC_OK ) {
                        ism_common_setError(rc);
                        break;
                    }
                    deleted += 1;
                    break;
                }
            }
        }
        ism_cluster_freeView(info);
    }

    if ( deleted > 0 ) {
        TRACE(5, "Removed inactive ClusterMember: ServerName:%s ServerUID:%s\n", sName, sUID);
        rc = ISMRC_OK;
    } else {
        TRACE(5, "Could not find a matching inactive cluster member: ServerName:%s ServerUID:%s\n", sName, sUID);
        rc = ISMRC_ClusterMemberNotFound;
        ism_common_setErrorData(rc, "%s%s", sName?sName:"", sUID?sUID:"");
    }

    if ( sName ) ism_common_free(ism_memory_monitoring_misc,sName);
    if ( sUID ) ism_common_free(ism_memory_monitoring_misc, sUID );

    return rc;
}


