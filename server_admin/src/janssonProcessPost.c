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
#include <sys/utsname.h>


#define JSON_CONFIG_FNAME "server_dynamic.json"

extern int ism_config_json_replaceArrayConfig(const char *objName, json_t *newObj);
extern int ism_admin_nodeUpdateAllowed(int *rc, ism_ConfigComponentType_t compType, char *object);
extern void ism_common_setServerUID(const char * value);
extern int ism_admin_haDisabledInCluster(int flag);
extern char * ism_security_createAdminUserPasswordHash(const char * password);
extern void ism_config_setMQConnectivityEnabledFlag(int flag, int wait);
extern int ism_config_updateEndpointInterfaceName(json_t *mobj, char *name);
extern int ism_admin_initRestart(int delay);
extern ismHA_Role_t ism_admin_getHArole(concat_alloc_t *output_buffer, int *rc);
extern char *ism_config_getClusterStatusStr(void);
extern void ism_config_setServerName(int getLock, int setDefault);
extern int ism_config_updateStandbyNode(json_t *objval, int comptype, char *item, char *name, int getConfigLock, int deleteFlag);
extern int ism_admin_isHAEnabled(void);
extern void ism_common_setServerName(const char * value);
extern char * ism_config_getServerName(void);
extern int ism_ha_admin_get_standby_serverName(char *srvName);
extern int ism_config_json_getAdminPort(int getLock);
extern int ism_config_json_processLicensePayload(json_t *post, int getLock);
extern int ism_config_json_sendMQConnectvityFlagValue(void);
extern int ism_admin_executeCRLRevalidateOptionForSecurityProfile(const char * secProfileName);
extern int ism_admin_executeCRLRevalidateOptionOneEndpoint(const char * epname);
extern void ism_config_updateResourceSetDescriptor(json_t *mergedObj, int haSync);
extern int ism_admin_dumpStatus(void);

extern const char * configDir;
extern json_t *srvConfigRoot;                 /* Server configuration JSON object  */
extern pthread_rwlock_t    srvConfiglock;     /* Lock for servConfigRoot           */
extern pthread_spinlock_t  configSpinLock;

extern char *adminUser;
extern char *adminUserPassword;
extern int adminMode;
extern int adminModeRC;
extern int restartNeeded;
extern int lastAminPort;
extern int backupApplied;
extern int adminInitError;

int revalidateEndpointForCRL; // protected by srvConfigLock;

typedef int32_t  (*getStatistics_f)(const ismCluster_Statistics_t *pStatistics);
typedef int32_t  (*removeLocalServer_f)();
typedef int32_t (*ism_snmpConfig_f) (int oldValue);
static ism_snmpConfig_f snmpConfig_f = NULL;

int initTermStoreHA = 0;



/*
 * NOTE ------------------
 * If schema is updated to add a new configuration item, make sure to add the configuration item name
 * in this list. Configuration item order is important to handle import or apply a big blob of data.
 *
 * Important: QueueManagerConnection and DestinationMapping rule should always be the last entries in
 * the list.
 */
struct {
    int    defined;
    char * objectName;
    int    mqcObject;
} configOrderList[] = {
    { 0, "Version", 0 },
    { 0, "LicensedUsage", 0 },
    { 0, "Accept", 0 },
    { 0, "AdminMode", 0 },
    { 0, "ServerUID", 0 },
    { 0, "ServerName", 0 },
    { 0, "FIPS", 0 },
    { 0, "AdminUserID", 0 },
    { 0, "AdminUserPassword", 0 },
    { 0, "EnableDiskPersistence", 0 },
    { 0, "LogLevel", 0 },
    { 0, "ConnectionLog", 0 },
    { 0, "SecurityLog", 0 },
    { 0, "AdminLog", 0 },
    { 0, "MQConnectivityLog", 1 },
    { 0, "TraceLevel", 1 },
    { 0, "TraceSelected", 0 },
    { 0, "TraceMax", 1 },
    { 0, "TraceConnection", 0 },
    { 0, "TraceOptions", 0 },
    { 0, "TraceMessageData", 1 },
    { 0, "MQConnectivityEnabled", 0 },
    { 0, "SNMPEnabled", 0 },
	{ 0, "PreSharedKey", 0 },
    { 0, "CertificateProfile", 0 },
    { 0, "LTPAProfile", 0 },
    { 0, "OAuthProfile", 0 },
    { 0, "CRLProfile", 0 },
    { 0, "SecurityProfile", 0 },
    { 0, "TrustedCertificate", 0 },
    { 0, "ClientCertificate", 0 },
    { 0, "ConnectionPolicy", 0 },
    { 0, "ConfigurationPolicy", 0 },
    { 0, "AdminEndpoint", 0 },
    { 0, "TopicPolicy", 0 },
    { 0, "QueuePolicy", 0 },
    { 0, "SubscriptionPolicy", 0 },
    { 0, "MessageHub", 0 },
    { 0, "Endpoint", 0 },
    { 0, "LDAP", 0 },
    { 0, "HighAvailability", 0 },
    { 0, "ClusterMembership", 0 },
    { 0, "Queue", 0 },
    { 0, "AdminSubscription", 0 },
    { 0, "DurableNamespaceAdminSub", 0 },
    { 0, "NonpersistentAdminSub", 0 },
    { 0, "MQTTClient", 0 },
    { 0, "TopicMonitor", 0 },
    { 0, "Subscription", 0 },
    { 0, "ClusterRequestedTopics", 0 },
    { 0, "Protocol", 0 },
    { 0, "Plugin", 0 },
    { 0, "PluginServer", 0 },
    { 0, "PluginPort", 0 },
    { 0, "PluginDebugServer", 0 },
    { 0, "PluginDebugPort", 0 },
    { 0, "PluginMaxHeapSize", 0 },
    { 0, "PluginVMArgs", 0 },
    { 0, "ClientSet", 0 },
    { 0, "ResourceSetDescriptor", 0 },
    { 0, "TraceBackup", 1 },
    { 0, "TraceBackupDestination", 1 },
    { 0, "TraceBackupCount", 1 },
    { 0, "TraceModuleLocation", 1 },
    { 0, "TraceModuleOptions", 1 },
    { 0, "LogLocation", 1 },
    { 0, "Syslog", 1 },
    { 0, "MQCertificate", 0 },
    { 0, "TolerateRecoveryInconsistencies", 0 },
    { 0, "QueueManagerConnection", 1 },
    { 0, "DestinationMappingRule", 1 },
};

#define NOCFGITEMS (sizeof(configOrderList)/sizeof(configOrderList[0]))

int orgCreated = 0;
int disableHA = 0;
int setAdminModeByHA = 0;
int regenUID = 0;

/* This function expects that the caller has config lock */
XAPI void ism_config_json_disableHACheck(int flag) {
    pthread_spin_lock(&configSpinLock);
    TRACE(9, "Set disableHA flag to %d\n", flag);
    disableHA = flag;

    int rc1 = 0;

    if ( ism_admin_getHArole(NULL, &rc1) == ISM_HA_ROLE_STANDBY ) {
    	if ( disableHA == 1 && adminMode == 0 ) {
            /* Set AdminMode to maintenance */
    		json_object_set(srvConfigRoot, "AdminMode", json_integer(1));
    		setAdminModeByHA = 1;
    		if ( haRestartNeeded == 0 ) haRestartNeeded = ISMRC_RestartNeeded;
    	} else if ( disableHA == 0 && setAdminModeByHA == 1 ) {
            /* ReSet AdminMode */
    		json_object_set(srvConfigRoot, "AdminMode", json_integer(0));
    		setAdminModeByHA = 0;
    		if ( haRestartNeeded == ISMRC_RestartNeeded ) haRestartNeeded = 0;
    	}
    }
    pthread_spin_unlock(&configSpinLock);
}

XAPI void ism_config_json_setRestartNeededFlag(void) {
    pthread_spin_lock(&configSpinLock);
    restartNeeded = 1;
    pthread_spin_unlock(&configSpinLock);
}

XAPI int ism_config_json_getRestartNeededFlag(void) {
    int retval = 0;
    pthread_spin_lock(&configSpinLock);
    retval = restartNeeded;
    pthread_spin_unlock(&configSpinLock);
    return retval;
}


XAPI void ism_config_json_setUIDRegenFlag(int flag) {
    pthread_spin_lock(&configSpinLock);
    regenUID = flag;
    pthread_spin_unlock(&configSpinLock);
}

XAPI int ism_config_json_getUIDRegenFlag(void) {
    int retval = 0;
    pthread_spin_lock(&configSpinLock);
    retval = regenUID;
    pthread_spin_unlock(&configSpinLock);
    return retval;
}

/* Enable/Disable ClusterMembership
 * This function sets required flags and configuration when Cluster is diabled on
 * servers configured with or without HA
 */
static int ism_config_json_enableDisableClusterMembership(int oldValue, int newValue) {
    int rc = ISMRC_OK;
    int rc1 = ISMRC_OK;

    /* If switching from enabled to disabled */
    if (oldValue == 1 && newValue == 0) {

        /* If switching from enabled to disabled, then:
         * - call ism_cluster_removeLocalServer - this function regenerates UID
         * - On a StandAlone server - auto restart server
         * - On a Primary and Standby nodes - auto restart
         * - Can not disable ClusterMembership on Standby node
         */

        /* On Standby node Cluster can not be disabled
         * This check is made in ism_config_json_processObject()
         */

        getStatistics_f getStats = (getStatistics_f)(uintptr_t)ism_common_getLongConfig("_ism_cluster_getStatistics_fnptr", 0L);
        removeLocalServer_f rLS = (removeLocalServer_f)(uintptr_t)ism_common_getLongConfig("_ism_cluster_removeLocalServer_fnptr", 0L);
        if (getStats && rLS) {
            ismCluster_Statistics_t stats;
            int statsRC = getStats(&stats);
            if (ISMRC_OK == statsRC) {
                if (ISM_CLUSTER_LS_STATE_ACTIVE == stats.state) {
                    rc = rLS();
                    if ( rc == ISMRC_ClusterNotAvailable || rc == ISMRC_ClusterDisabled ) {
                    	/* If server UID Regen flag is set - reset it back */
                    	if ( ism_config_json_getUIDRegenFlag() == 1 ) {
                    		ism_config_json_setUIDRegenFlag(0);
                    	}
                        /* no neeed to restart server - just return success */
                        rc = ISMRC_OK;
                        return rc;
                    }

                    /* call returned OK or other errors - ServerUID is already regenerated by the cluster component.
                     * Need to restart server. If in HA environment, also need to terminate store.
                     */
                    if (ISMRC_OK != rc) {
                        if (rc == ISMRC_ClusterRemoveLocalServerNoAck) {
                            char buffer[1024];
                            TRACE(7, "%s: %s", __FUNCTION__, ism_common_getErrorString(ISMRC_ClusterRemoveLocalServerNoAck, buffer, 1024));
                            /* Set rc back to ISMRC_OK so we still update the configuration file */
                            rc = ISMRC_OK;
                            ism_common_setError(0);
                        } else {
                            ism_common_setError(rc);
                        }
                    }

                    /* set restart flag */
                    ism_config_json_setRestartNeededFlag();

                    /* terminate store - coordinated store shutdown if disabled on Primary */
                    if ( ism_admin_getHArole(NULL, &rc1) == ISM_HA_ROLE_PRIMARY ) {
                        TRACE(1, "Disable ClusterMembership in HighAvailability: terminate store\n");
                        initTermStoreHA = 1; /* ism_store_termHA() will get invoked in main.c */
                    }
                }
            }
        }

        /* On Standby set restart flag */
        if ( ism_admin_getHArole(NULL, &rc1) == ISM_HA_ROLE_STANDBY ) {
        	ism_config_json_setRestartNeededFlag();
        }

    } else if (oldValue == 0 && newValue == 1) {              /* Switching from disabled to enabled */

        /* If switching from disabled to enabled, then:
         * - Regenerate UID on StandAlone node
         * - In HA enviornment, regenerate UID only on Primary - UID will get replicated
         */

    	if ( ism_admin_getHArole(NULL, &rc1) == ISM_HA_ROLE_PRIMARY || ism_admin_isHAEnabled() == 0 ) {
    		ism_common_generateServerUID();
    	}

    }

    return rc;
}

/* Enable/Disable HA
 * This function sets required flags and configuration when HA is diabled on
 * servers configured with or without ClusterMembership
 */
#if 0
static int ism_config_json_enableDisableHA(int oldValue, int newValue) {
	int rc = ISMRC_OK;

	/* Regenerate UID when HA is disabled on a Standby node */

	return rc;
}
#endif

/**
 * Updates configuration file in JSON format
 */
XAPI int ism_config_json_updateFile(int getLock) {
    int rc = ISMRC_OK;
    char bfilepath[1024];
    char cfilepath[1024];
    char ofilepath[1024];
    char tfilepath[1024];

	if ( getenv("CUNIT") != NULL ) {
		return rc;
	}

    /* get config lock */
    if ( getLock == 1 )
        pthread_rwlock_wrlock(&srvConfiglock);

    sprintf(cfilepath, "%s/%s",     configDir, JSON_CONFIG_FNAME);
    sprintf(ofilepath, "%s/%s.org", configDir, JSON_CONFIG_FNAME);
    sprintf(bfilepath, "%s/%s.bak", configDir, JSON_CONFIG_FNAME);
    sprintf(tfilepath, "%s/%s.tmp", configDir, JSON_CONFIG_FNAME);

    if ( orgCreated == 0 ) {
        if ( access(ofilepath, F_OK) == -1 ) {
            TRACE(5, "Make a copy of initial configuration file %s.\n", ofilepath);
            copyFile(cfilepath, ofilepath);
        }
        orgCreated = 1;
    }

    /* dump content of current configuration to a .temp file */
    int loop = 0;
    for ( loop=0; loop<5; loop++ ) {
        if ( srvConfigRoot ) {
            rc = ISMRC_OK;
            errno = 0;
            char *dumpStr = json_dumps(srvConfigRoot, JSON_INDENT(2)|JSON_PRESERVE_ORDER|JSON_ENCODE_ANY);
            if ( dumpStr ) {
                FILE *dest = NULL;
                dest = fopen(tfilepath, "w");
                if ( dest ) {
                    fprintf(dest, "%s", dumpStr);
                    fclose(dest);
                    // dumpstr comes from the jason libraries so doesn't include an eyecatcher
                    ism_common_free_raw(ism_memory_admin_misc,dumpStr);

                    /* rename current to .bak */
                    rename(cfilepath, bfilepath);
                    if ( rename( tfilepath, cfilepath ) != 0 ) {
                        TRACE(2, "Could not rename temporary configuration to current. rc=%d\n", errno);
                        rc = ISMRC_Error;
                    }
                    break;
                } else {
                    TRACE(2, "Failed to open config file: errno:%d\n", errno);
                    rc = ISMRC_Error;
                    // dumpstr comes from the jason libraries so doesn't include an eyecatcher
                    ism_common_free_raw(ism_memory_admin_misc,dumpStr);
                    ism_common_setError(rc);
                    break;
                }
            } else {
                if ( errno == EAGAIN && loop < 4 ) {
                    TRACE(9, "Update configuration file: try count=%d\n", loop);
                    ism_common_sleep(100000);
                    continue;
                } else {
                    json_t *tmpSrvConfigPtr = json_deep_copy(srvConfigRoot);
                    int retval = json_dump_file( tmpSrvConfigPtr, tfilepath, JSON_INDENT(2)|JSON_PRESERVE_ORDER|JSON_ENCODE_ANY );
                    if ( retval == 0 ) {
                        if ( rename( tfilepath, cfilepath ) != 0 ) {
                            TRACE(2, "Could not rename temporary configuration to current. rc=%d\n", errno);
                            rc = ISMRC_Error;
                            ism_common_setError(rc);
                            break;
                        }
                        json_t *tmpPtr = srvConfigRoot;
                        srvConfigRoot = tmpSrvConfigPtr;
                        json_decref(tmpPtr);
                    } else {
                        TRACE(2, "Failed to update configuration file: errno:%d\n", errno);
                        rc = ISMRC_Error;
                        ism_common_setError(rc);
                        break;
                    }

                    continue;
                }
            }
        } else {
            TRACE(2, "Configuration root node is NULL.\n");
            rc = ISMRC_NullPointer;
            ism_common_setError(rc);
            break;
        }
    }

    /* Unlock config lock */
    if ( getLock == 1 )
        pthread_rwlock_unlock(&srvConfiglock);

    return rc;
}

/* Process Singleton objects */
static int ism_config_json_processSingletonObject(ism_config_t *handle, const char *object, json_t *objval, json_t *post, int haSync, int validate, json_t *jdefval, int restCall) {
    int rc = ISMRC_OK;

    /* Check if settable */
    int itype = 0;
    int otype = 0;
    int comp  = 0;
    int isAdminUserID = 0;
    int isAdminUserPassword = 0;
    json_t *schemaObj = ism_config_findSchemaObject(object, NULL, &comp, &otype, &itype);

    if ( validate == 1 ) {
        json_t *settableObj = json_object_get(schemaObj, "Settable");
        if ( settableObj && json_typeof(settableObj) == JSON_STRING ) {
            char *settable = (char *)json_string_value(settableObj);
            if (settable && (*settable=='n' || *settable=='N')) {
            	rc = 6208;
                ism_common_setErrorData(rc, "%s", object);
                return rc;
            }
        }
    }

    if ( validate == 1 ) {
        rc = ism_config_validate_Singleton(post, objval, (char *)object);
    }

    if ( rc == ISMRC_OK ) {

        if ( !strcmp(object, "MQConnectivityEnabled")) {
            int flag = 0;
            if ( objval && json_typeof(objval) == JSON_TRUE ) flag = 1;
            ism_config_setMQConnectivityEnabledFlag(flag, 0);
            if ( flag == 0 ) ism_config_json_sendMQConnectvityFlagValue();
        }

        /* Create props list for callback */
        ism_prop_t *callBackProps = ism_common_newProperties(1);
        int objType = json_typeof(objval);

        if (objType == JSON_NULL) {

        	/* PreShared Key file name is always the same */
        	if (!strcmp(object, "PreSharedKey")) {

				json_string_set(objval,"");
				const char *PSKdir = ism_common_getStringConfig("PreSharedKeyDir");
				const char *PSKName = "psk.csv";
				char DestPSKFile[strlen(PSKdir)+strlen(PSKName)+1];
				sprintf(DestPSKFile,"%s/%s", PSKdir, PSKName);
				unlink(DestPSKFile);

				objType = JSON_STRING;
				objval = json_string("");

        	} else {
				json_t *jtype = json_object_get(schemaObj, "Type");
				if (NULL == jdefval || JSON_NULL == json_typeof(jdefval)) {
					/* No default, should not get here!!!! */
					/* TODO: What to do here????? */
				} else {
					const char *defval = json_string_value(jdefval);
					const char * type = json_string_value(jtype);
					if (!strcmpi(type, "Boolean")) {
						if (!strcmpi(defval, "False")) {
							objType = JSON_FALSE;
							objval = json_false();
						} else {
							objType = JSON_TRUE;
							objval = json_true();
						}
					} else if (!strcmpi(type, "Number")) {
						int intval = atoi(defval);
						objType = JSON_INTEGER;
						objval = json_integer(intval);
					} else {
						objType = JSON_STRING;
						objval = jdefval;
					}
				}
        	}
        }

        if (!strcmp(object, "ServerName")) {
            if ( json_typeof(objval) == JSON_NULL ) {
               	/* Set ServerName to default */
               	ism_config_setServerName(0, 1);
                objval = json_object_get(srvConfigRoot, "ServerName");
        		objType = JSON_STRING;
            } else if ( json_typeof(objval) == JSON_STRING ) {
              	const char *sname = json_string_value(objval);
               	if ( !sname || *sname == '\0') {
                   	/* Set ServerName to default */
                   	ism_config_setServerName(0, 1);
                    objval = json_object_get(srvConfigRoot, "ServerName");
            		objType = JSON_STRING;
               	}
            }
        }

        rc = ism_config_json_addItemPropToList(objType, object, objval, callBackProps);
        if ( rc != ISMRC_OK ) {
            ism_common_freeProperties(callBackProps);
            return rc;
        }

        if ( handle && handle->callback ) {
            if ( !strcmp(object, "ServerUID") && restCall == 0  ) {
                const char *uid = json_string_value(objval);
               ism_common_setServerUID(uid);
            } else if (!strcmp(object, "AdminUserID")) {
            	const char *value = json_string_value(objval);
            	if (!value || *value == 0) {
                    TRACE(2, "NULL value is specified for AdminUserID\n");
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", object, "NULL");
                    ism_common_freeProperties(callBackProps);
                    return ISMRC_BadPropertyValue;
            	}
            	isAdminUserID = 1;
            } else if (!strcmp(object, "AdminUserPassword")) {
            	const char *value = json_string_value(objval);
            	if (!value || *value == 0) {
                    TRACE(2, "NULL value is specified for AdminUserPassword\n");
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", object, "NULL");
                    ism_common_freeProperties(callBackProps);
                    return ISMRC_BadPropertyValue;
            	}

            	/* Encrypt Password */
                char *encPasswd = ism_security_createAdminUserPasswordHash((char *)value);
                if (encPasswd == NULL) {
                	TRACE(2, "Failed to encrypt AdminUserPassword\n");
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", object, value);
                    ism_common_freeProperties(callBackProps);
                    return ISMRC_BadPropertyValue;
                }

                objval = json_string(encPasswd);
                isAdminUserPassword = 1;
            } else {
                rc = handle->callback((char *)object, NULL, callBackProps, ISM_CONFIG_CHANGE_PROPS);

                /* if callback returns OK, update standby node */
                if ( haSync == 0 ) {
                    int getConfigLock = 0; /* lock is already taken */
                    int deleteFlag = 0;
                    rc = ism_config_updateStandbyNode(objval, handle->comptype, (char *)object, NULL, getConfigLock, deleteFlag);
                    if (rc != ISMRC_OK)
                        TRACE(3, "%s: ism_config_updateStandbyNode updating object: %s return errorcode: %d\n", __FUNCTION__, object?object:"null", rc);
                }
            }
        }
        ism_common_freeProperties(callBackProps);
    }

    /* Update config */
    if ( rc == ISMRC_OK  || rc == ISMRC_NotImplemented ) {
        /* TODO: remove when validation of all objects are implemented */
        if ( rc == ISMRC_NotImplemented ) {
            rc = ISMRC_OK;
            ism_common_setError(0);
        }
        /* Update config */

    		if ( json_object_get(srvConfigRoot, object)) {
                json_object_set(srvConfigRoot, object, objval);
            } else {
                json_object_set_new(srvConfigRoot, object, objval);
            }

        if (isAdminUserID) {
        	if (adminUser) ism_common_free(ism_memory_admin_misc,adminUser);
        	adminUser = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),json_string_value(objval));
        } else if (isAdminUserPassword) {
        	if (adminUserPassword) ism_common_free(ism_memory_admin_misc,adminUserPassword);
        	adminUserPassword = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),json_string_value(objval));
        }

        /* Call global flag setting functions */
        if ( !strcmp(object, "MQConnectivityEnabled")) {
            int flag = 0;
            if ( objval && json_typeof(objval) == JSON_TRUE ) flag = 1;
            ism_config_setMQConnectivityEnabledFlag(flag, 1);
        }
    }

    return rc;
}

/* Process composite objects */
static int ism_config_json_processCompositeObject(ism_config_t *handle, const char *object, const char *name, json_t *objval, json_t *post, int haSync, int validate, concat_alloc_t *mqcBuffer, int count) {
    int rc = ISMRC_OK;
    int dataType;
    const char *itemkey = NULL;
    void *itemiter = NULL;
    ism_prop_t *props = NULL;
    int action = 0;
    json_t *mergedObj = NULL;
    int mqcObj = 0;
    int isTMorCRT = 0;
    int isTC = 0;

    /* if this function is invoked on a stanby node:
     * - do not check for settable items
     * - no need to validate - already validated on primary
     * - invoke callback and update config
     */

    if ( haSync == 0 ) {
        /* check for non-settable items */
        itemiter = json_object_iter(objval);
        while (itemiter) {
            itemkey = json_object_iter_key(itemiter);

            /* find object in schema */
            int itype = 0;
            int otype = 0;
            int comp  = 0;
            json_t *schemaObj = ism_config_findSchemaObject(object, itemkey, &comp, &otype, &itype);

            if ( object && !strcmp(object, "Endpoint") && !strcmp(itemkey, "InterfaceName")) {
                /* Ignore Endpoint configuration item InterfaceName - though it is not
                 * user settable, we need to allow Endpoint configuration to handle the case
                 * of export/import
                 */
                TRACE(9, "Endpoint configuration item InterfaceName is ignored.\n");
            } else {

                json_t *settableObj = json_object_get(schemaObj, "Settable");
                if ( settableObj && json_typeof(settableObj) == JSON_STRING ) {
                    char *settable = (char *)json_string_value(settableObj);
                    if (settable && (*settable=='n' || *settable=='N')) {
                    	rc = 6208;
                        ism_common_setErrorData(rc, "%s", itemkey);
                        goto PROCESSCOMPOSITE_END;
                    }
                }
            }

            itemiter = json_object_iter_next(objval, itemiter);
        }

        /* Get merged value */
        TRACE(8, "Get merged data of object:%s  Instance:%s\n", object?object:"NULL", name?name:"NULL");
        json_t *arrayObjects = NULL;
        int isArray = 0;
        if ( !strcmp(object, "TrustedCertificate") ||  !strcmp(object, "TopicMonitor") || !strcmp(object, "ClientCertificate") ||
             !strcmp(object, "ClusterRequestedTopics")) {
            arrayObjects = json_object_get(post, object);
            if (json_is_array(arrayObjects)) {
            	isArray = 1;
            } else {
            	rc = ISMRC_InvalidCfgObject;
                ism_common_setErrorData(rc, "%s", object);
                goto PROCESSCOMPOSITE_END;
            }
        } else if ( !strcmp(object, "Plugin")) {

        	    json_t *curInst = ism_config_json_getComposite(object, name, 0);
        	    if ( curInst ) {
        	        mergedObj = json_deep_copy(curInst);

        	        json_t *pobjPassed = json_object_get(post, object);
        	        if ( pobjPassed ) {
        	            json_t *pinstPassed = json_object_get(pobjPassed, name);
        	            if ( pinstPassed ) {
        	                json_t *fObj = json_object_get(pinstPassed, "File");
        	                json_t *pObj = json_object_get(pinstPassed, "PropertiesFile");
        	                const char *tmpstr = NULL;
        	                if ( fObj ) {
        	                    tmpstr = json_string_value(fObj);
        	                    if ( tmpstr && *tmpstr != '\0') {
        	                        json_object_set(mergedObj, "File", fObj);
        	                    }
        	                }
        	                if ( pObj ) {
        	                    tmpstr = json_string_value(pObj);
        	                    if ( tmpstr && *tmpstr != '\0') {
        	                        json_object_set(mergedObj, "PropertiesFile", pObj);
        	                    }
        	                }
        	            }
        	        }
        	    } else {
        	        mergedObj = json_deep_copy(objval);
        	    }
        } else {

            mergedObj = ism_config_json_getMergedObject(object, name, NULL, post, &dataType);
        }

        /* Validate data */
        props = ism_common_newProperties(32);

        /* check for existing object - if exist - it is an update case */
        action = ism_config_objectExist((char *)object, (char *)name, NULL);

        /* Validate object */
        if ( validate == 1 ) {
            if ( isArray == 1 ) {
                rc = ism_config_jsonValidateObject(post, arrayObjects, (char *)object, (char *)name, action, props);
            } else {
                rc = ism_config_jsonValidateObject(post, mergedObj, (char *)object, (char *)name, action, props);
            }
        }
    } else {
        // The "ResourceSetDescriptor" coming from the primary looks how we want it to
        // look in server_dynamic.json, but that isn't how the code expects to deal with
        // it, it has an extra layer of 'ResourceSetDescriptor' object around it.
        //
        // We remove the outer 'ResourceSetDescriptor' layer.
        if (!strcmp(object, "ResourceSetDescriptor")) {
            json_t *copyval = json_object_get(objval, "ResourceSetDescriptor");
            if (!copyval || !json_is_object(copyval)) {
                copyval = objval;
            }

            mergedObj = json_deep_copy(copyval);

            ism_config_updateResourceSetDescriptor(mergedObj, haSync);
        } else {
            /* Invoked on HA standby node - configuration data is already merged and sent by primary node */
            mergedObj = json_deep_copy(objval);

            /* For endpoint object - set Interface based on InterfaceName */
            if ( !strcmp(object, "Endpoint")) {
                TRACE(6, "Standby node: update Endpoint Interface based on InterfaceName: Endpoint=%s\n", name);
                rc = ism_config_updateEndpointInterfaceName(mergedObj, (char *)name);
            }
        }
    }

    /* process callbacks */
    if ( rc == ISMRC_OK ) {
        if (!strcmp(object, "TopicMonitor") || !strcmp(object, "ClusterRequestedTopics")) {
            //TopicMonitor / ClusterRequestedTopics callback doesn't handle existing topicstring update. will return error if already exist.
            //using post for this reason
            int i = 0;
            json_t * tm_array = json_object_get(post, object);
            for (i=0; i<json_array_size(tm_array); i++) {
                json_t *instObj = json_array_get(tm_array, i);
                rc = ism_config_json_callCallback( handle, object, NULL, instObj, haSync, ISM_CONFIG_CHANGE_PROPS, 0);
                if ( (rc == ISMRC_ExistingTopicMonitor || rc == ISMRC_ExistingClusterRequestedTopic) && validate == 0 ) rc = ISMRC_OK;
                if (rc != ISMRC_OK) {
                    // Roll back the previously successful additions.
                    for(i--; i>=0; i--) {
                        instObj = json_array_get(tm_array, i);
                        (void)ism_config_json_callCallback( handle, object, NULL, instObj, haSync, ISM_CONFIG_CHANGE_DELETE, 0);
                    }
                    goto PROCESSCOMPOSITE_END;
                }
            }
            isTMorCRT = 1;
        } else if (!strcmp(object, "TrustedCertificate") || !strcmp(object, "ClientCertificate")) {
            int i = 0;
            json_t *tc_array = json_object_get(post, object);
            for (i=0; i<json_array_size(tc_array); i++) {
                json_t *instObj = json_array_get(tc_array, i);
                rc = ism_config_json_callCallback( handle, object, name, instObj, haSync, ISM_CONFIG_CHANGE_PROPS, 0);
                if (rc != ISMRC_OK)
                    goto PROCESSCOMPOSITE_END;
            }
            isTC = 1;
        } else {
            rc = ism_config_json_callCallback( handle, object, name, mergedObj, haSync, ISM_CONFIG_CHANGE_PROPS, 0);
            if (rc != ISMRC_OK) {
                goto PROCESSCOMPOSITE_END;
            }

            if (revalidateEndpointForCRL == 1) {
                if (!strcmp(object, "SecurityProfile")) {
                    ism_admin_executeCRLRevalidateOptionForSecurityProfile(name);
                } else if (!strcmp(object, "Endpoint") || !strcmp(object, "AdminEndpoint")) {
                    ism_admin_executeCRLRevalidateOptionOneEndpoint(name);
                }
            }

            /* for mqc objects, create json string and add to a buffer */
            if ( mqcBuffer ) {
                mqcObj = 1;
                /* dump object in buffer and return */
                if (count == 0 ) {
                    ism_common_allocBufferCopyLen(mqcBuffer, "{\"", 2);
                } else {
                	ism_common_allocBufferCopyLen(mqcBuffer, "\"", 1);
                }
                ism_common_allocBufferCopyLen(mqcBuffer, name, strlen(name));
                ism_common_allocBufferCopyLen(mqcBuffer, "\":", 2);
                char *buf = json_dumps(mergedObj, JSON_ENCODE_ANY);
                if ( buf ) {
                    ism_common_allocBufferCopyLen(mqcBuffer, buf, strlen(buf));
                    // The buffer is created by the json function so free the raw memory
                    ism_common_free_raw(ism_memory_admin_misc,buf);
                } else {
                    ism_common_allocBufferCopyLen(mqcBuffer, "{}", 2);
                }
            }
        }
    }

    /* Update config */
    if ( rc == ISMRC_OK  ) {
        if ( isTMorCRT == 1 ) {
            json_t *updatedObj = ism_config_json_getMergedArray(object, post, &rc);
            ism_config_json_replaceArrayConfig(object, updatedObj);
        } else if ( isTC == 0 ) {
            if (mqcObj == 0 || !strcmp(object, "Syslog") || !strcmp(object, "LogLocation")) {
                /* get current config and replace with mergedObj */
                json_t *currConfig = ism_config_json_getComposite(object, name, 0);
                if ( currConfig) {
                    /* update with merged value */
                    if ( json_object_update(currConfig, mergedObj) < 0 ) {
                        rc = ISMRC_Error;
                        TRACE(4, "Failed to update configuration of Object:%s Name:%s\n", object?object:"NULL", name?name:"NULL");
                        json_decref(mergedObj);
                        goto PROCESSCOMPOSITE_END;
                    }
                } else {
                    /* Add new object */
                    rc = ism_config_json_setComposite(object, name, mergedObj);
                }
            }
        }
    }

    // If this was a successful verify, reset to ISMRC_OK so we can continue to process other items.
    if (ISMRC_VerifyTestOK == rc) rc = ISMRC_OK;

PROCESSCOMPOSITE_END:
    if ( props )
       ism_common_freeProperties(props);

    revalidateEndpointForCRL = 0;

    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}

/**
 * Process one JSON configuration object
 */
static int ism_config_process_oneObject(json_t *post, const char *objkey, json_t *objval, int haSync, int validate, int restCall, concat_alloc_t *mqcBuffer) {
    int rc = ISMRC_OK;
    int isComposite = 0;
    json_t *schemaObj = NULL;
    json_t *jdefval = NULL;
    int count = 0;
    
    ism_ConfigComponentType_t compType = ism_config_findSchemaGetComponentType((char *)objkey, &isComposite, &schemaObj);
    if ( compType == ISM_CONFIG_COMP_LAST ) {
        rc = ISMRC_InvalidCfgObject;
        ism_common_setErrorData(rc, "%s", objkey?objkey:"NULL");
        return rc;
    }

    /* In HA - check if updates are allowed */
    if ( haSync == 0 && ism_admin_nodeUpdateAllowed(&rc, compType, (char *)objkey) == 0 ) {
        return rc;
    }

    ism_config_t *handle = ism_config_getHandle(compType, NULL);

    /* disable post delete all for now */
	if (!objval || json_typeof(objval) == JSON_NULL) {
    	if (!isComposite && NULL != schemaObj) {
    		jdefval = json_object_get(schemaObj, "Default");
    	}
    	if ( strcmp(objkey, "ServerName") && (isComposite || !jdefval)) {
    		rc = ISMRC_BadRESTfulRequest;
    		int erlen = strlen(objkey) + 9;
    		char *erstr = alloca(erlen + 1);
    		sprintf(erstr, "\"%s\":null", objkey);
    		ism_common_setErrorData(rc, "%s", erstr);
    		return rc;
    	}
	}

	/*
	 * If the object is supposed to be composite, but we have a non-object value instead,
	 * report an error
	 */
	if (isComposite && !json_is_object(objval) && !json_is_array(objval)) {
    	rc = ISMRC_BadRESTfulRequest;
    	char * erval = json_dumps(objval, JSON_ENCODE_ANY);
		int erlen = strlen(objkey) + strlen(erval) + 4;
		char *erstr = alloca(erlen + 1);
		sprintf(erstr, "\"%s\":%s", objkey, erval);
		ism_common_setErrorData(rc, "%s", erstr);
		ism_common_free_raw(ism_memory_admin_misc,erval);
        return rc;
    }

    if ( isComposite == 1 ) {
        /* Special processing for AdminEndpoint, LDAP and HightAvailability
         * Though these objects are named object, but has predefined names and can have only one instance of
         * these objects configured.
         */


        if ( !strcmp(objkey, "AdminEndpoint") ) {

            rc = ism_config_json_processCompositeObject(handle, "AdminEndpoint", "AdminEndpoint", objval, post, haSync, validate, mqcBuffer, 0);

        } else if ( !strcmp(objkey, "LDAP") ) {

            if ( haSync == 1 ) {
                /* data received by standby node include "ldapconfig" as name in the JSON object */
                json_t *tmpObjval = json_object_get(objval, "ldapconfig");
                rc = ism_config_json_processCompositeObject(handle, "LDAP", "ldapconfig", tmpObjval, post, haSync, validate, mqcBuffer, 0);
            } else {
                rc = ism_config_json_processCompositeObject(handle, "LDAP", "ldapconfig", objval, post, haSync, validate, mqcBuffer, 0);
            }

        } else if ( !strcmp(objkey, "MQCertificate") ) {

            rc = ism_config_json_processCompositeObject(handle, "MQCertificate", "MQCert", objval, post, haSync, validate, mqcBuffer, 0);

        } else if ( !strcmp(objkey, "HighAvailability") ) {

            rc = ism_config_json_processCompositeObject(handle, "HighAvailability", "haconfig", objval, post, haSync, validate, mqcBuffer, 0);

        } else if ( !strcmp(objkey, "ClusterMembership") ) {

            if ( haSync == 1 ) {
                /* data received by standby node include "cluster" as name in the JSON object */
                json_t *tmpObjval = json_object_get(objval, "cluster");
                rc = ism_config_json_processCompositeObject(handle, "ClusterMembership", "cluster", tmpObjval, post, haSync, validate, mqcBuffer, 0);
            } else {
                rc = ism_config_json_processCompositeObject(handle, "ClusterMembership", "cluster", objval, post, haSync, validate, mqcBuffer, 0);
            }

        } else if ( !strcmp(objkey, "TrustedCertificate") || !strcmp(objkey, "TopicMonitor") || !strcmp(objkey, "ClientCertificate") ||
                    !strcmp(objkey, "ClusterRequestedTopics")) {

            rc = ism_config_json_processCompositeObject(handle, objkey, NULL, objval, post, haSync, validate, mqcBuffer, 0);

        } else if ( !strcmp(objkey, "ResourceSetDescriptor")) {

            rc = ism_config_json_processCompositeObject(handle, "ResourceSetDescriptor", "ResourceSetDescriptor", objval, post, haSync, validate, mqcBuffer, count);

        } else if ( !strcmp(objkey, "Subscription") || !strcmp(objkey, "MQTTClient") || !(strcmp(objkey, "ResourceSet"))) {

            rc = ISMRC_InvalidCfgObject;
    		ism_common_setErrorData(rc, "%s", objkey);

        } else if (!strcmp(objkey, "Syslog")) {
            rc = ism_config_json_processCompositeObject(handle, "Syslog", "Syslog", objval, post, haSync, validate, mqcBuffer, 0);
        } else {

            /* get instance of composite object and validate the object */
            json_t *instval = NULL;
            const char *instkey = NULL;
            int insttyp;
            void *institer = json_object_iter(objval);
            while (institer) {
                instkey = json_object_iter_key(institer);
                instval = json_object_iter_value(institer);
                insttyp = json_typeof(instval);

                if ( insttyp == JSON_OBJECT ) {

                    /* Special case Endpoint->InterfaceName */
                    if ( !strcmp(objkey, "Endpoint")) {
                        /* If rest call is trying to set InterfaceName, ignore the value */
                        if ( restCall == 1 ) {
                            json_t * intf = json_object_get(instval, "InterfaceName");
                            if ( intf ) {
                                json_object_del(instval, "InterfaceName");
                            }
                        }
                    }

                    if (count > 0 && mqcBuffer) {
                    	ism_common_allocBufferCopyLen(mqcBuffer, ",", 1);
                    }

                    rc = ism_config_json_processCompositeObject(handle, objkey, instkey, instval, post, haSync, validate, mqcBuffer, count);
                    if ( rc != ISMRC_OK ) break;

                	count += 1;

                } else if ( insttyp == JSON_NULL ) {
            		rc = ISMRC_BadRESTfulRequest;
            		int erlen = strlen(instkey) + 9;
            		char *erstr = alloca(erlen + 1);
            		sprintf(erstr, "\"%s\":null", instkey);
            		ism_common_setErrorData(rc, "%s", erstr);
            		return rc;
                } else {
                	rc = ISMRC_PropertiesNotValid;
                	ism_common_setError(rc);
                	return rc;
                }

                institer = json_object_iter_next(objval, institer);
            }

        }

        if (count > 0 && mqcBuffer) {
        	ism_common_allocBufferCopyLen(mqcBuffer, "}", 1);
        }

    } else {

        /* process singleton object - skip Version */
		if ( strcmp(objkey, "Version")) {
            if ( restCall == 1 && !strcmp(objkey, "ServerUID")) {
                   rc = ISMRC_BadPropertyName;
                   ism_common_setErrorData(ISMRC_BadPropertyName, "%s", "ServerUID");
            } else if (!strcmp(objkey, "SNMPEnabled")) {
                	int oldSnmpEnabledValue;
                	oldSnmpEnabledValue = ism_admin_isSNMPConfigured(1);
                	rc = ism_config_json_processSingletonObject(handle, objkey, objval, post, haSync, validate, jdefval, restCall);
                	if (!rc)
                	{
                		if (!snmpConfig_f)
                			snmpConfig_f = (ism_snmpConfig_f)(uintptr_t)ism_common_getLongConfig("_ism_snmp_config_fnptr", 0L);
                		snmpConfig_f(oldSnmpEnabledValue);
                	}
            } else {
                rc = ism_config_json_processSingletonObject(handle, objkey, objval, post, haSync, validate, jdefval, restCall);

                if ( rc == ISMRC_OK && mqcBuffer ) {
                    char *buf = json_dumps(objval, JSON_ENCODE_ANY);
                    if ( buf ) {
                        ism_common_allocBufferCopyLen(mqcBuffer, buf, strlen(buf));
                        // buffer comes from jansson
                        ism_common_free_raw(ism_memory_admin_misc,buf);
                    }
                }
            }
        }
    }

    return rc;
}

/**
 * Process JSON configuration object
 */
XAPI int ism_config_json_processObject(json_t *post, char *object, char *httpPath, int haSync, int validate, int restCall, concat_alloc_t *mqcBuffer, int *onlyMqcItems) {
    int rc = ISMRC_OK;
    int updateConfig = 0;
    json_t *objval = NULL;
    const char *objkey = NULL;
    int contLoop = 0;
    int invokeDisableCMFunc = 0;
    int newEnableCM = 0;
    int oldEnableCM = 0;
    int invokeDisableHAFunc = 0;
    int nitems = 0;
    int nprocessed = 0;
    int hasCMObj = 0;
    int hasMQCObject = 0;
    int hasHAObj = 0;
    int i = 0;
    int hasSyslog = 0;
    int hasServerName = 0;

    if ( !post ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto PROCESSPOST_END;
    }

    /* Get read lock */
    pthread_rwlock_wrlock(&srvConfiglock);

    /* check no of configuration items in the post */
    nitems = json_object_size(post);

    TRACE(9, "Process POST Payload. Number of items: %d\n", nitems);

    /* No need to process if POST payload has no item */
    if ( nitems == 0 ) {
    	rc = ISMRC_BadRESTfulRequest;
    	ism_common_setErrorData(rc, "%s", "payload");
        goto PROCESSPOST_END;
    }

    /* Check and process one item */
    if ( nitems == 1 ) {
        json_object_foreach(post, objkey, objval) {

            nprocessed += 1;

            /* By pass version */
            if ( !strcmp(objkey, "Version") ) {
                continue;
            } else if (!strcmp(objkey, "Delete") && restCall == 1 ) {
                rc = ISMRC_BadRESTfulRequest;
                ism_common_setErrorData(rc, "%s", httpPath?httpPath:"NULL");
                goto PROCESSPOST_END;
            } else {
                /* If object is specified in the path, this should match with objkey */
                if ( object && strcmp(object, objkey)) {
                    rc = ISMRC_BadRESTfulRequest;
                    ism_common_setErrorData(rc, "%s", httpPath?httpPath:"NULL");
                    goto PROCESSPOST_END;
                }

                /* Make POST call fail for all items except LicensedUsage and Accept, if license is not accepted */
                if ( restCall == 1 && ism_admin_checkLicenseIsAccepted() == 0 && strcmp(objkey, "LicensedUsage") && strcmp(objkey, "Accept")) {
                	rc = ISMRC_LicenseError;
                	ism_common_setError(rc);
                	goto PROCESSPOST_END;
                }

                /* DO not allow config if server is stopping */
                int32_t srvState = ism_admin_get_server_state();
                if ( srvState == ISM_SERVER_STOPPING ) {
                    rc = 6209;
                    ism_common_setErrorData(6209, "%s%s", "Stopping", objkey);
                    goto PROCESSPOST_END;
                } else if ( srvState != ISM_SERVER_RUNNING ) {
                    /* Do not allow config of some objects if server is not running */
                    if ( haSync == 0 && (!strcmp(objkey, "AdminSubscription") ||
                                         !strcmp(objkey, "DurableNamespaceAdminSub") ||
                                         !strcmp(objkey, "NonpersistentAdminSub"))) {
                        rc = ISMRC_RequireRunningServer;
                        ism_common_setErrorData(rc, "%s", objkey);
                        goto PROCESSPOST_END;
                    }
                }

                /* Process license acceptance related config items */
                if ( !strcmp(objkey, "LicensedUsage") || !strcmp(objkey, "Accept")) {
                	if ( restCall == 1 ) {
                	    rc = ism_config_json_processLicensePayload(post, 0);
                	    goto PROCESSPOST_END;
                	} else {
                		continue;
                	}
                }

                /* Set/check variables for some of objects that has special processing mechanism */
                if ( !strcmp(objkey, "HighAvailability")) {
                    hasHAObj = 1;
                }
                if ( !strcmp(objkey, "Syslog")) {
                	hasSyslog = 1;
                }
                if ( !strcmp(objkey, "ClusterMembership")) {
                    hasCMObj = 1;

                    int rc1 = 0;
                    int stdbyREST = 0;
                    if ( ism_admin_getHArole(NULL, &rc1) == ISM_HA_ROLE_STANDBY && restCall == 1 ) stdbyREST = 1;

                    /* get old and new values of EnableClusterMembership */
                    int type = 0;
                    json_t *enableCM = ism_config_json_getObject("ClusterMembership", "cluster", "EnableClusterMembership", 0, &type);
                    if ( enableCM && json_typeof(enableCM) == JSON_TRUE ) oldEnableCM = 1;
                    json_t *newEnableCMObj = json_object_get(objval, "EnableClusterMembership");
                    if ( !newEnableCMObj ) {
                        newEnableCM = oldEnableCM;
                    } else {
                        if ( json_typeof(newEnableCMObj) == JSON_TRUE ) {
                        	newEnableCM = 1;
                        }
                    }

                    char *oldDSList = NULL, *newDSList = NULL;
                    int dsListUpdated = 0;

                    json_t *disListObj = ism_config_json_getObject("ClusterMembership", "cluster", "DiscoveryServerList", 0, &type);
                    if ( disListObj && json_typeof(disListObj) == JSON_STRING ) oldDSList = (char *)json_string_value(disListObj);

                    json_t *newDSListObj = json_object_get(objval, "DiscoveryServerList");
                    if ( newDSListObj && json_typeof(newDSListObj) == JSON_STRING ) newDSList = (char *)json_string_value(newDSListObj);

                    if ( (!oldDSList && newDSList) || (oldDSList && !newDSList && !stdbyREST) ) {
                    	dsListUpdated = 1;
                    } else if ( oldDSList && newDSList && strcmp(oldDSList, newDSList) ) {
                    	dsListUpdated = 1;
                    }

                    char *oldCLName = NULL, *newCLName = NULL;
                    int clNameUpdated = 0;

                	json_t *clNameObj = ism_config_json_getObject("ClusterMembership", "cluster", "ClusterName", 0, &type);
                	if ( clNameObj && json_typeof(clNameObj) == JSON_STRING ) oldCLName = (char *)json_string_value(clNameObj);

                    json_t *newCLNameObj = json_object_get(objval, "ClusterName");
                	if ( newCLNameObj && json_typeof(newCLNameObj) == JSON_STRING ) newCLName = (char *)json_string_value(newCLNameObj);

                	if ( (!oldCLName && newCLName) || (oldCLName && !newCLName && !stdbyREST) ) {
                		clNameUpdated = 1;
                	} else if ( newCLName && newCLName && strcmp(oldCLName, newCLName)) {
                		clNameUpdated = 1;
                	}

                    /* On standby node, do not allow users to modify EnableClusterMembership */
                    if ( stdbyREST == 1 ) {
                    	if ( oldEnableCM != newEnableCM ) {
                            rc = ISMRC_PropUpdateNotAllowed;
                            ism_common_setErrorData(ISMRC_PropUpdateNotAllowed, "%s%s%s", "ClusterMembership", "cluster", "EnableClusterMembership");
                            goto PROCESSPOST_END;
                    	}
                    	if ( dsListUpdated == 1 || clNameUpdated == 1 ) {
                            rc = ISMRC_PropUpdateNotAllowed;
                            ism_common_setErrorData(ISMRC_PropUpdateNotAllowed, "%s%s%s", "ClusterMembership", "cluster",
                            		dsListUpdated?"DiscoveryServerList":"ClusterName");
                            goto PROCESSPOST_END;
                    	}
                    }
                }

                for (i=0; i<NOCFGITEMS; i++) {
                	if ( !strcmp(objkey, configOrderList[i].objectName)) {
                	    break;
                	}
                }

                if ( !strcmp(objkey, "DestinationMappingRule") || !strcmp(objkey, "QueueManagerConnection")) {
                	*onlyMqcItems = 1;
                }

                /* for mqc objects, create json string and add to a buffer */
                if ( mqcBuffer && configOrderList[i].mqcObject == 1 ) {
                	if ( hasMQCObject == 0 ) {
                		ism_common_allocBufferCopyLen(mqcBuffer, "{\"Configuration\":{", 18);
                		hasMQCObject = 1;
                	} else {
                		ism_common_allocBufferCopyLen(mqcBuffer, ",", 1);
                	}

               	    int buflen = strlen(objkey) + 3;
               	    char *buffer = (char *)alloca(buflen + 1);
               	    sprintf(buffer, "\"%s\":", objkey);
                    ism_common_allocBufferCopyLen(mqcBuffer, buffer, buflen);
                }

                if ( hasMQCObject == 1 ) {
                    rc = ism_config_process_oneObject(post, objkey, objval, haSync, validate, restCall, mqcBuffer);
                } else {
                	rc = ism_config_process_oneObject(post, objkey, objval, haSync, validate, restCall, NULL);
                }

                if ( rc == ISMRC_OK ) {

                    if ( strcmp(objkey, "TrustedCertificate") && strcmp(objkey, "ClientCertificate")) {
                        updateConfig = 1;
                    }

                    if ( hasMQCObject == 1 ) {
                    	if ( hasSyslog == 1 ) {
                    		ism_common_allocBufferCopyLen(mqcBuffer, "}}}", 3);
                    	} else {
                            ism_common_allocBufferCopyLen(mqcBuffer, "}}", 2);
                    	}

                        /* MQC object configuration update is processed by async callback handling function */
                        updateConfig = 0;
                    }

                    if ( hasCMObj == 1 ) {
                        invokeDisableCMFunc = 1;
                    }

                    if ( hasHAObj == 1 ) {
                        invokeDisableHAFunc = 1;
                    }

                    if ( !strcmp(objkey, "ServerName")) {
                        hasServerName = 1;
                    }

                }
            }
        }
        goto PROCESSPOST_END;
    }

    /* POST payload includes multiple items
     * - loop thru post json objects to check for invalid items
     */
    for (i=0; i<NOCFGITEMS; i++) {
        configOrderList[i].defined = 0;
    }

    *onlyMqcItems = 1;

    json_object_foreach(post, objkey, objval) {
        contLoop = 0;
        /* If object is specified in the path, this should match with objkey */
        if ( object && strcmp(object, objkey)) {
            rc = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(rc, "%s", httpPath?httpPath:"NULL");
            goto PROCESSPOST_END;
        }

        /* Check if Delete is set for this payload - Delete could be set by Primary node in HA Pair,
         * for REST API call, setting Delete should result in an error response
         */
        if ( !strcmp(objkey, "Delete")) {
            if ( restCall == 1 ) {
                rc = ISMRC_BadRESTfulRequest;
                ism_common_setErrorData(rc, "%s", httpPath?httpPath:"NULL");
                goto PROCESSPOST_END;
            }
            continue;
        }

        /* Validate item from ordered list */
        for (i=0; i<NOCFGITEMS; i++) {
            char *oname = configOrderList[i].objectName;
            if ( !strcmp(objkey, oname)) {
                contLoop = 1;
                configOrderList[i].defined = 1;
                break;
            }
        }
        if ( contLoop == 0 ) {
            /* Invalid object is specified - return error */
            rc = ISMRC_BadPropertyName;
            ism_common_setErrorData(rc, "%s", objkey?objkey:"NULL");
            goto PROCESSPOST_END;
        }
    }

    /* loop thru the ordered list */
    for ( i=0; i<NOCFGITEMS; i++ ) {
        if ( configOrderList[i].defined == 0 )
            continue;

        objkey = configOrderList[i].objectName;

        TRACE(9, "Process configuration object: %s\n", objkey);

        /* find item in POST payload and process each item */
        objval = json_object_get(post, objkey);

        nprocessed += 1;

        if ( !strcmp(objkey, "Version")) {
            continue;
        }

        /* Make POST call fail for all items except LicensedUsage and Accept, if license is not accepted */
        if ( restCall == 1 && ism_admin_checkLicenseIsAccepted() == 0 && strcmp(objkey, "LicensedUsage") && strcmp(objkey, "Accept")) {
        	rc = ISMRC_LicenseError;
        	ism_common_setError(rc);
        	goto PROCESSPOST_END;
        }

        /* DO not allow config if server is stopping */
        if ( ism_admin_get_server_state() == ISM_SERVER_STOPPING ) {
        	rc = 6209;
        	ism_common_setErrorData(6209, "%s%s", "Stopping", objkey);
        	goto PROCESSPOST_END;
        }

        /* Process license acceptance related config items */
        if ( !strcmp(objkey, "LicensedUsage") || !strcmp(objkey, "Accept")) {
        	if ( restCall == 1 ) {
        	    rc = ism_config_json_processLicensePayload(post, 0);
        	    goto PROCESSPOST_END;
        	} else {
        		continue;
        	}
        }

        /* Set/check variables for some of objects that has special processing mechanism */
        if ( *onlyMqcItems == 1 && strcmp(objkey, "DestinationMappingRule") && strcmp(objkey, "QueueManagerConnection")) {
        	*onlyMqcItems = 0;
        }
        if ( !strcmp(objkey, "Syslog")) {
        	hasSyslog = 1;
        }
        if ( !strcmp(objkey, "ClusterMembership")) {
            hasCMObj = 1;
            /* get old and new values of EnableClusterMembership */
            int type = 0;
            json_t *enableCM = ism_config_json_getObject("ClusterMembership", "cluster", "EnableClusterMembership", 0, &type);
            if ( enableCM && json_typeof(enableCM) == JSON_TRUE ) oldEnableCM = 1;

            json_t *clusterObj = json_object_get(objval, "cluster");
            if ( clusterObj ) {
                json_t *newEnableCMObj = json_object_get(clusterObj, "EnableClusterMembership");
                if ( newEnableCMObj && json_typeof(newEnableCMObj) == JSON_TRUE ) newEnableCM = 1;
            }

            /* On standby node, do not allow users to modify EnableClusterMembership */
            int rc2= 0;
            if ( ism_admin_getHArole(NULL, &rc2) == ISM_HA_ROLE_STANDBY && restCall == 1 && oldEnableCM != newEnableCM ) {
                rc = ISMRC_PropUpdateNotAllowed;
                ism_common_setErrorData(ISMRC_PropUpdateNotAllowed, "%s%s%s", "ClusterMembership", "cluster", "EnableClusterMembership");
                goto PROCESSPOST_END;
            }
        }

        if ( !strcmp(objkey, "HighAvailability")) {
            hasHAObj = 1;
        }

        /* for mqc objects, create json string and add to a buffer */
        int sendMQCBuff = 0;
        if ( mqcBuffer && configOrderList[i].mqcObject == 1 ) {
        	if ( hasMQCObject == 0 ) {
        		ism_common_allocBufferCopyLen(mqcBuffer, "{\"Configuration\":{", 18);
        		hasMQCObject = 1;
        	} else {
        		ism_common_allocBufferCopyLen(mqcBuffer, ",", 1);
        	}

        	int buflen = strlen(objkey) + 3;
        	char *buffer = (char *)alloca(buflen + 1);
        	sprintf(buffer, "\"%s\":", objkey);
            ism_common_allocBufferCopyLen(mqcBuffer, buffer, buflen);
            sendMQCBuff = 1;
        }

        if ( sendMQCBuff == 1 ) {
            rc = ism_config_process_oneObject(post, objkey, objval, haSync, validate, restCall, mqcBuffer);
        } else {
        	rc = ism_config_process_oneObject(post, objkey, objval, haSync, validate, restCall, NULL);
        }

        if ( rc != ISMRC_OK )
           goto PROCESSPOST_END;

        updateConfig = 1;

        if ( hasCMObj == 1 ) {
            invokeDisableCMFunc = 1;
        }

        if ( hasHAObj == 1 ) {
            invokeDisableHAFunc = 1;
        }

        if ( !strcmp(objkey, "ServerName")) {
            hasServerName = 1;
        }
    }

    /* If MQC objects are defined in POST Payload, add closing brackets in mqcBuffer */
    if ( hasMQCObject == 1 ) {
    	if ( hasSyslog == 1 ) {
    		ism_common_allocBufferCopyLen(mqcBuffer, "}}", 3);
    	} else {
            ism_common_allocBufferCopyLen(mqcBuffer, "}}", 2);
    	}
        updateConfig = 0;
    }

PROCESSPOST_END:

    /* Update configuration file */
    TRACE(6, "ism_config_json_processObject: nitems=%d nprocessed=%d rc=%d updateConfigFlag=%d\n", nitems, nprocessed, rc, updateConfig);

    if ( updateConfig == 1 ) {
        int getLock = 0;
        int rc1 = ISMRC_OK;
        rc1 = ism_config_json_updateFile(getLock);

        if ( rc == ISMRC_OK && rc1 != ISMRC_OK ) {
            rc = rc1;
        }
    }

    /* Unlock configuration lock, so that HA, cluster, SNMP admin calls can get read lock if required */
    pthread_rwlock_unlock(&srvConfiglock);

    /* Check and invoke ClusterMembership disable function */
    if ( rc != ISMRC_OK )
        return rc;

    /* Check and invoke ClusterMembership disable function */
    if ( invokeDisableCMFunc == 1 ) {
        ism_config_json_enableDisableClusterMembership(oldEnableCM, newEnableCM);
    }

    /* Check and invoke HA disable function in cluster environment on Primary node */
    if ( invokeDisableHAFunc == 1 ) {
        ism_admin_haDisabledInCluster(disableHA);
    }

    if ( hasServerName == 1 ) {
        /* on primary node - send message to update ServerName on standby node */
        int rc1 = ISMRC_OK;
        if ( ism_admin_getHArole(NULL, &rc1) == ISM_HA_ROLE_PRIMARY ) {
        	char *srvName = (char *)ism_config_getServerName();
        	ism_ha_admin_get_standby_serverName(srvName);
        }
    }

    return rc;
}

/**
 * Process configuration JSON fragment sent in Payload of HTTP Post
 */
XAPI int ism_config_json_processPost(ism_http_t *http, ism_rest_api_cb httpCallback) {
    int rc = ISMRC_OK;
    json_t *post = NULL;

    if ( !http ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto PROCESSPOST_END;
    } else {
        TRACE(9, "Entry %s: http: %p\n", __FUNCTION__, http);
    }

    /* check for restart case - set by import of backup file */
    if ( adminInitError == ISMRC_RestartNeeded && backupApplied == 1 ) {
    	rc = ISMRC_RestartNeeded;
    	ism_common_setError(rc);
    	goto PROCESSPOST_END;
    }

    /* Create JSON object from HTTP POST Payload */
    int noErrorTrace = 0;
    post = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);
    if ( !post || rc != ISMRC_OK) {
        goto PROCESSPOST_END;
    }

    /* Get version info */
    json_t *verObj = json_object_get(post, "Version");
    if ( verObj ) {
        /* check version with current schema version
         * This check is not required in v2.0 release. The check will be required if we update
         * configuration schema in a future release - basically to support multiple versions of
         * REST APIs. */
        char *ver = http->version;
        if ( ver && *ver != '\0' && strcmp(http->version, SERVER_SCHEMA_VERSION)) {
            TRACE(7, "Version is not specified in URI. Use current configuration schema version: %s\n", SERVER_SCHEMA_VERSION);

            /* match with URL version info */
            if ( json_typeof(verObj) != JSON_STRING ) {
               ism_common_setErrorData(6001, "%s%d", "invalid Version type", json_typeof(verObj));
                rc = ISMRC_ArgNotValid;
                goto PROCESSPOST_END;
            }

            char *version = (char *)json_string_value(verObj);
            if ( http->version && version && strcmp(version, http->version)) {
                rc = ISMRC_ArgNotValid;
                ism_common_setErrorData(6001, "%s%d", "Version mismatch", rc);
                goto PROCESSPOST_END;
            }
        }
    }

    /* Check if object info is in path */
    char *object = NULL, *more = NULL;
    if (http->user_path && strlen(http->user_path) > 1) {
        char *tmpstr = (char *)http->user_path;
        object = ism_common_getToken(tmpstr, "/", "/", &more );
    }

    /* If object is specified in the path, not allowed */
    if ( object ) {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(rc, "%s", http->path?http->path:"NULL");
        goto PROCESSPOST_END;
    }

    /* Process POST */
    int haSync = 0;
    int validate = 1;
    int restCall = 1;
    char tbuf[2048];
    concat_alloc_t mqcBuffer = { tbuf, sizeof(tbuf), 0, 0 };
    int onlyMqcItems = 0;
    memset(tbuf, 0, 2048);
    rc = ism_config_json_processObject(post, object, http->path, haSync, validate, restCall, &mqcBuffer, &onlyMqcItems);

    if ( rc == ISMRC_OK && mqcBuffer.used > 0 ) {
        /* buffer contains MQCbridge configuration
         * - Send data to mqc brdige process and do not invoke http callback
         */
        TRACE(7, "Send configuration message to MQC process: len=%d msg=%s\n", mqcBuffer.used, mqcBuffer.buf );
        int persistData = 1;
        rc = ism_admin_mqc_send(mqcBuffer.buf, mqcBuffer.used, http, httpCallback, persistData, ISM_MQC_LAST, NULL);

        if ( rc == ISMRC_MQCProcessError && onlyMqcItems == 0 ) {
        	rc = ISMRC_OK;
            int getLock = 1;
            ism_config_json_updateFile(getLock);
        } else if ( rc == ISMRC_OK || rc == ISMRC_AsyncCompletion) {
            httpCallback = NULL; //Callback will be invoked asynchronously
            rc = ISMRC_OK;
        }
    }

PROCESSPOST_END:

    if ( post )
        json_decref(post);

    /* Update return message */
    const char * repl[1];
    int replSize = 0;
    if (rc != ISMRC_OK ) {
        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
    } else {
        /* check if restart is scheduled */
        if ( ism_config_json_getRestartNeededFlag() == 1 ) {
            ism_admin_initRestart(10);
            ism_confg_rest_createErrMsg(http, 6168, repl, replSize);
        } else {
            ism_confg_rest_createErrMsg(http, 6011, repl, replSize);
        }
    }

    if ( httpCallback ) {
        httpCallback(http, rc);
    }

    return ISMRC_OK;
}

/**
 * Configure an object from JSON STRING
 *
 * Only use this function during server initialization step, cunit tests and
 * for items that doesn't require HA sync.
 */
XAPI int ism_config_set_fromJSONStr(char *jsonStr, char *object, int validate) {
    int rc = ISMRC_OK;

    if ( !jsonStr || *jsonStr == '\0' || !object || *object == '\0' ) {
        TRACE(1, "NULL or Empty string received to configure an object.\n");
        return ISMRC_NullPointer;
    }

    /* Create post from string */
    json_error_t error = {0};
    json_t *post = json_loads(jsonStr, 0, &error);
    if ( !post ) {
        TRACE(1, "Could not convert JSON string into an JSON object: error_text:%s\n", error.text);
        return ISMRC_BadRESTfulRequest;
    }

    int restCall = -1;
    int onlyMqcItems = 0;
    rc = ism_config_json_processObject(post, object, NULL, 0, validate, restCall, NULL, &onlyMqcItems);

    if ( post )
        json_decref(post);

    return rc;
}

/**
 * Set HADisabled to false
 * - Gets invoked on Standby node
 */
XAPI int ism_config_disableHA(void) {
    int rc = ISMRC_OK;
    int disableCM = 0;
    int rc1;

    /* if this is a standby node and Cluster is enabled and active, disable cluster also */
    if ( ism_admin_getHArole(NULL, &rc1) == ISM_HA_ROLE_STANDBY ) {
    	if ( ism_admin_isClusterConfigured() == 1 ) {
    		char *clStatus = ism_config_getClusterStatusStr();
    		if ( !strcmp(clStatus, "Standby")) {
    			disableCM = 1;
    		}
    	}
    }


    /* lock config lock */
    pthread_rwlock_wrlock(&srvConfiglock);

    json_t *obj = json_object_get(srvConfigRoot, "HighAvailability");
    json_t *inst = json_object_get(obj, "haconfig");

    json_object_set(inst, "EnableHA", json_false());
    // json_object_set(inst, "Enabled", json_false());

    if ( disableCM == 1 ) {
        obj = json_object_get(srvConfigRoot, "ClusterMembership");
        inst = json_object_get(obj, "cluster");

        json_object_set(inst, "EnableClusterMembership", json_false());
        // json_object_set(inst, "Enabled", json_false());
    }

    int getLock = 0;
    rc = ism_config_json_updateFile(getLock);

    /* Unlock config lock */
    pthread_rwlock_unlock(&srvConfiglock);

    return rc;
}

/**
 * Sets ServerName
 */
XAPI void ism_config_setServerName(int getLock, int setDefault) {
    struct utsname buf;

    /* get current admin port */
	int port = ism_config_json_getAdminPort(getLock);

    /* Get current ServerName */
    if ( getLock == 1 ) pthread_rwlock_wrlock(&srvConfiglock);

	if ( setDefault == 1 ) {

	    if ( uname(&buf) == 0 ) {
	    	char *hname = (char *)alloca(strlen(buf.nodename) + 8);
	    	if ( port > 0 ) {
	    	    sprintf(hname, "%s:%d", buf.nodename, port);
	    	} else {
	    		sprintf(hname, "%s", buf.nodename);
	    	}
	    	json_object_set(srvConfigRoot, "ServerName", json_string(hname));
	    	TRACE(3, "Set ServerName to %s\n", hname);
	    } else {
	    	TRACE(3, "Could not set ServerName. uname call returned error: %d\n", errno);
	    }

	} else {

		json_t *obj = json_object_get(srvConfigRoot, "ServerName");
		if ( obj && json_typeof(obj) == JSON_STRING ) {
			const char *sName = json_string_value(obj);
			if ( !sName || *sName == '\0' ) {
				lastAminPort = port;
				if ( uname(&buf) == 0 ) {
					char *hname = (char *)alloca(strlen(buf.nodename) + 8);
					if ( port > 0 ) {
						sprintf(hname, "%s:%d", buf.nodename, port);
					} else {
						sprintf(hname, "%s", buf.nodename);
					}
					json_object_set(srvConfigRoot, "ServerName", json_string(hname));
					TRACE(3, "Set ServerName to %s\n", hname);
					ism_common_setServerName(hname);
				} else {
					TRACE(3, "Could not set ServerName. uname call returned error: %d\n", errno);
				}
			}
		}

    }

    if ( getLock == 1 ) pthread_rwlock_unlock(&srvConfiglock);

    return;
}

/* Set admin mode to maintenance */
XAPI int ism_admin_setMaintenanceMode(int errorRC, int restartFlag) {
	int rc = ISMRC_OK;

	/* Set AdminMode and AdminModeRC */
    pthread_spin_lock(&configSpinLock);
    TRACE(4, "Set Server mode to Maintenance\n");
	json_object_set(srvConfigRoot, "AdminMode", json_integer(1));
	if ( errorRC != ISMRC_OK ) {
	    json_object_set_new(srvConfigRoot, "AdminModeRC", json_integer(errorRC));
	}
    int getLock = 0;
    ism_config_json_updateFile(getLock);
    pthread_spin_unlock(&configSpinLock);
    
    /* Update status file */
    ism_admin_dumpStatus();

    /* Restart server */
    if ( restartFlag == 1) {
    	rc = ism_admin_initRestart(10);
    }

    return rc;
}

/* Get AdminModeRC */
XAPI int ism_config_json_getAdminModeRC(void) {
	int rc = ISMRC_OK;
    pthread_spin_lock(&configSpinLock);
    json_t *adminModeRCObj = json_object_get(srvConfigRoot, "AdminModeRC");
    if ( adminModeRCObj && json_typeof(adminModeRCObj) == JSON_INTEGER ) {
    	rc = json_integer_value(adminModeRCObj);
    }
    pthread_spin_unlock(&configSpinLock);
    return rc;
}

/* Reset AdminModeRC */
XAPI void ism_config_json_resetAdminModeRC(void) {
    pthread_spin_lock(&configSpinLock);
    json_t *adminModeRCObj = json_object_get(srvConfigRoot, "AdminModeRC");
    if ( adminModeRCObj ) {
    	json_object_del(srvConfigRoot, "AdminModeRC");
        int getLock = 0;
        ism_config_json_updateFile(getLock);
    }
    pthread_spin_unlock(&configSpinLock);
}

/* Set AdminMode - this function is always called with config lock */
XAPI int ism_config_json_setAdminMode(int flag) {
	int rc = ISMRC_OK;

	if ( flag != 0 && flag != 1 )
		return rc;

    TRACE(4, "Set Server Maintenance mode to %s\n", flag?"Start":"Stop");

	json_object_set(srvConfigRoot, "AdminMode", json_integer(flag));
    int getLock = 0;
    ism_config_json_updateFile(getLock);

    return rc;
}


/*
 * Get list of properties of all named objects belog to TRANSPORT, ENGINE and SECUIRY components
 * This list is used in HA pair.
 *   - The Standby node creates this list before applying changes sent by Primary, e.g.
 *     Endpoint.TestEP = 1
 *   - During config data processing sent by Primary node to Standby node, if the object sent
 *     by Primary is compared with the list created. If object exist in Standby, the value
 *     in the property list is set to 0, e.g. if TestEP is in Primary list then Endpoint.TestEP
 *     is set to 0
 *   - After processing all data sent by the Primary node, the objects are deleted from Standby node
 *     whose value in the list is still 1 *
 */
XAPI ism_prop_t * ism_config_json_getObjectNames(void) {
    int found = 0;
    int i;
    ism_prop_t *currItems = ism_common_newProperties(512);

    pthread_rwlock_rdlock(&srvConfiglock);

    TRACE(5, "Standby: create list of existing named objects\n");

    for (i=0; i<NOCFGITEMS; i++) {
    	char *objName = configOrderList[i].objectName;

    	/* get component and object type */
        int itype = 0;
        int otype = 0;
        int comp  = 0;
        json_t *schemaObj = ism_config_findSchemaObject(objName, NULL, &comp, &otype, &itype);

        TRACE(9, "Standby: Check object=%s otype=%d comp=%d\n", objName, otype, comp);

        if ( schemaObj && otype == 1 && ( comp == ISM_CONFIG_COMP_TRANSPORT || comp == ISM_CONFIG_COMP_ENGINE || comp == ISM_CONFIG_COMP_SECURITY || comp == ISM_CONFIG_COMP_MQCONNECTIVITY )) {
        	/* Get object, loop thru the list and add item name to ism_prop_t list */
        	json_t *obj = json_object_get(srvConfigRoot, objName);

        	if ( !strcmp(objName, "TopicMonitor") || !strcmp(objName, "ClusterRequestedTopics")) {

        		int j = 0;
                for (j=0; j<json_array_size(obj); j++) {
                    json_t *instObj = json_array_get(obj, j);
                    if (instObj) {
                        const char *val = json_string_value(instObj);
						if ( val && *val != '\0') {
							ism_field_t f;
							char propName[4096];
							sprintf(propName, "%s.%s", objName, val);
							f.type = VT_Integer;
							f.val.i = 1;
							ism_common_setProperty(currItems, propName, &f);
							found += 1;
						}
                    }
                }

        	} else {

				json_t *objval = NULL;
				const char *objkey = NULL;
				if ( obj) {
					json_object_foreach(obj, objkey, objval) {
						if ( objkey && *objkey != '\0') {
							ism_field_t f;
							char propName[2048];
							sprintf(propName, "%s.%s", objName, objkey);
							f.type = VT_Integer;
							f.val.i = 1;
							ism_common_setProperty(currItems, propName, &f);
							found += 1;
						}
					}
				}

        	}
        }
    }

    pthread_rwlock_unlock(&srvConfiglock);

    TRACE(5, "Standby: number of named objects to be verified after configuration sync: %d\n", found);

    if ( found == 0 ) {
        ism_common_freeProperties(currItems);
        currItems = NULL;
    }

    return currItems;
}
