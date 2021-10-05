/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

/*********************************************************************/
/*                                                                   */
/* Module Name: adminHA.c                                            */
/*                                                                   */
/* Description: High Availability Admin functions                    */
/*                                                                   */
/*********************************************************************/

#define TRACE_COMP Admin

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ismutil.h>
#include <admin.h>
#include <config.h>
#include <adminHA.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>

extern uint32_t ism_config_processJSONString(int noSend, int msgLen, char * line, ism_prop_t *currList, int storeMQCData);
extern int32_t ism_admin_getServerProcType(void);
extern int ism_admin_applyPSKFile(void);
extern int ism_admin_init_stop(int mode, ism_http_t *http);
extern int ism_config_disableHA(void);
extern int ism_admin_isClusterConfigured(void);
extern int ism_admin_initRestart(int delay);
extern int ism_config_json_getRestartNeededFlag(void);
extern void ism_common_setServerUID(const char * value);
extern int ism_config_set_fromJSONStr(char *jsonStr, char *object, int validate);
extern char * ism_config_getServerName(void);
extern int ism_config_json_updateFile(int getLock);


#define TMP_CONFIGDIR "/ima/config"
#define TMP_PRIMCFG   "/ima/config/serverDynamic.primary"

/* Defines for HA sync message codes */
#define ISM_HA_MSG_CONFIG_SYNC    'S'
#define ISM_HA_MSG_CONFIG_MSG1    '{'
#define ISM_HA_MSG_CONFIG_MSG2    '['
#define ISM_HA_MSG_CONTINUE       'C'
#define ISM_HA_MSG_CERT_SYNC      'K'
#define ISM_HA_MSG_RESTART        'R'
#define ISM_HA_MSG_CLEANSTORE     'P'
#define ISM_HA_MSG_STOP           'E'
#define ISM_HA_MSG_SYNC_HAGROUP   'G'

static int32_t ism_admin_ha_syncFiles(void);
static int32_t ism_admin_ha_sendCertificates(void);
static int ism_admin_disableHACluster(void);

const char *haErrorReasonStr[] = { " ", "CONFIG_ERROR", "DISCOVERY_TIMEOUT", "SPLIT_BRAIN", "SYNC_ERROR", "UNRESOLVED_STORE", "RESYNC_ERROR",
                "GROUP_MISMATCH", "UNKNOWN", "SYSTEM_ERROR", "UNKNOWN" };

/* Process type */
static int proctype;

static int adminHADisable_forCleanStore = 0;

char *localServerName = NULL;
char *remoteServerName = NULL;
char *orgServerUID = NULL;

/* Struct to keep Admin HA data */
ismAdminHAInfo_t *adminHAInfo = NULL;
int haLocksInited = 0;
int adminHAExitFlagSet = 0;

/* Configuration admin message */
ismHA_AdminMessage_t *configAdminMsg;
#define MAX_RETURN_ADMINMSG_LENGTH 256

pthread_spinlock_t cfgLogLock;
static pthread_mutex_t fileSyncLock;
int fileSyncCount=0;

int standbyConfigSyncStatus = 0;
int standbySyncInProgress = 0;
int termStoreFlag = 0;

/* Defines HA admin action list */
typedef struct ismAdminActionList
{
    int       actionType;   /* Action type           */
    int       actionIntVal; /* Subtype or value      */
    char     *actionStrVal; /* Action Message        */
    int       actionMsgLen; /* Action message length */
} ismAdminActionList_t;

static ism_common_list *haActionList = NULL;  /* HA Admin action list */

#define MAX_CFGMSG_SIZE  102400

static int startStandbySync = 0;
static int syncState = 0;
static char * ldapCertDir = NULL;
static char * mqCertDir = NULL;
static char * keyCertDir = NULL;
static char * trustCertDir = NULL;
static int clientSock = -1;
int admin_haServerSock = -1;

XAPI void ism_admin_ha_disableHA_for_cleanStore(void) {
    adminHADisable_forCleanStore = 1;
}

/**
 * Returns HA return code string
 */
XAPI const char * ism_config_json_haRCString(int type) {
    if ( type < 0 || type > 10 ) type = 10;
    return haErrorReasonStr[type];
}

/* HA role to string */
char * ism_admin_get_harole_string(ismHA_Role_t role) {
    char *roleStr = "UNKNOWN";

    switch(role) {
    case ISM_HA_ROLE_UNSYNC:
        roleStr = "UNSYNC";
        break;

    case ISM_HA_ROLE_PRIMARY:
        roleStr = "PRIMARY";
        break;

    case ISM_HA_ROLE_STANDBY:
        roleStr = "STANDBY";
        break;

    case ISM_HA_ROLE_ERROR:
        roleStr = "UNSYNC_ERROR";
        break;

    case ISM_HA_ROLE_DISABLED:
        roleStr = "HADISABLED";
        break;

    default:
        break;

    }
    return roleStr;
}


/**
 * Get configuration data from Standby node
 * Caller will have to free result
 */
XAPI int ism_ha_admin_get_standby_serverName(char *srvName) {
    int rc = ISMRC_OK;
    char *result = NULL;

    /* check current HA role */
    pthread_spin_lock(&adminHAInfo->lock);
    int syncStandby = adminHAInfo->sSyncStart;
    pthread_spin_unlock(&adminHAInfo->lock);

    TRACE(5, "Send get ServerName message to Stanby. ServerName:%s syncStartFlag=%d\n", srvName?srvName:"", syncStandby);

    if ( syncStandby == 1 ) {
        /* send admin message */

        char updateStr[1024];
        sprintf(updateStr, "W=%s", srvName?srvName:"");
        int len = strlen(updateStr);
        char *tmpstr = alloca(len+1);

        if ( tmpstr ) {                /* BEAM suppression: constant condition */
            configAdminMsg->pData = tmpstr;
            memset(tmpstr, 0, len+1);
            memcpy(tmpstr, updateStr, len);
            configAdminMsg->DataLength = len+1;

            configAdminMsg->ResBufferLength = 256;
            configAdminMsg->pResBuffer = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,534),256);
            memset(configAdminMsg->pResBuffer, 0, 256);

            rc = ism_ha_store_send_admin_msg(configAdminMsg);
            if ( rc != ISMRC_OK ) {
                TRACE(1, "Failed to send get ServerName message to Standby node. RC=%d\n", rc);
            } else {
                TRACE(5, "Received ServerName from Standby: %s\n",
                    configAdminMsg->pResBuffer?configAdminMsg->pResBuffer:"");
                char *p = configAdminMsg->pResBuffer;
                len = 0;
                if (p) {
                	len = strlen(p);
                	char * restmp = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,535),configAdminMsg->ResLength+1);
                	if ( restmp != NULL ) {
                		memcpy(restmp, configAdminMsg->pResBuffer, configAdminMsg->ResLength);
                		restmp[configAdminMsg->ResLength]='\0';
                	}
                    result = restmp;
                }
            }

            ism_common_free(ism_memory_admin_misc,configAdminMsg->pResBuffer);

        } else {
            rc = ISMRC_AllocateError;
        }
    }

    if ( rc != ISMRC_OK )
        ism_common_setError(rc);

    if ( result ) {
    	if ( remoteServerName) ism_common_free(ism_memory_admin_misc,remoteServerName);
    	remoteServerName = result;
    }

    return rc;
}


/* Restart required processes on standby node after sync complete */
static int ism_admin_ha_restartProcess(void) {
    TRACE(1, "Restarting services\n");
    ism_common_sleep(50000);

    pid_t pid = vfork();
    if (pid < 0) {
        TRACE(1, "Could not vfork process to restart processes\n");
        return 1;
    }
    if (pid == 0){
        execl(IMA_SVR_INSTALL_PATH "/bin/restartStandbyServices.sh", "restartStandbyServices.sh",  NULL);
        int urc = errno;
        TRACE(1, "Unable to run restartStandbyServices.sh: errno=%d error=%s\n", urc, strerror(urc));
        _exit(1);
    }
    int st;
    waitpid(pid, &st, 0);
    int rc = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    ism_admin_applyPSKFile();

    return rc;
}

/* start a script on complete admin sync tasks */
static int ism_admin_ha_startScript(char *scriptPath, char *scriptName, char *argVal) {
    pid_t pid = vfork();
    if (pid < 0) {
        TRACE(1, "Could not vfork process to sync certificates.\n");
        return 1;
    }
    if (pid == 0){
        execl(scriptPath, scriptName, argVal, NULL);
        int urc = errno;
        TRACE(1, "Unable to run %s: errno=%d error=%s\n", scriptName?scriptName:"null", urc, strerror(urc));
        _exit(1);
    }
    int st;
    waitpid(pid, &st, 0);
    int rc = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    return rc;
}

XAPI int ism_ha_admin_isUpdateRequired(void) {
    int enabled = 0;

    if (ism_admin_getServerProcType() == ISM_PROTYPE_SERVER) {
        /* check current HA role */
        pthread_spin_lock(&adminHAInfo->lock);
        int syncStandby = adminHAInfo->sSyncStart;
        pthread_spin_unlock(&adminHAInfo->lock);

        if ( syncStandby == 1 )
            enabled = 1;
    }

    return enabled;
}

/* Process view changed action */
static void ism_admin_ha_process_view_change(int action) {
    /*
     * check action type and process it
     */
    switch (action) {
    case ISM_HA_ACTION_STANDBY_TO_PRIMARY:
    	TRACE(4, "AdminHA: Received HA view change notification: ISM_HA_ACTION_STANDBY_TO_PRIMARY\n");
        pthread_spin_lock(&adminHAInfo->lock);
        if ( termStoreFlag == 1 ) {
        	TRACE(4, "Terminate store flag is set. Ignore ISM_HA_ACTION_STANDBY_TO_PRIMARY.\n");
        	pthread_spin_unlock(&adminHAInfo->lock);
        	break;
        }
        adminHAInfo->sSyncStart = -1;
        pthread_spin_unlock(&adminHAInfo->lock);
        ism_admin_ha_restartProcess();
        ism_admin_send_continue();
        LOG(NOTICE, Admin, 6112, "%s", "High Availability mode change \"{0}\" is initiated.", "STANDBY_TO_PRIMARY");
        break;

    case ISM_HA_ACTION_TERMINATE_STORE:
        /* Terminate HA */
        TRACE(4, "AdminHA: Received HA view change notification: ISM_HA_ACTION_TERMINATE_STORE\n");
        pthread_spin_lock(&adminHAInfo->lock);
        termStoreFlag = 1;
        pthread_spin_unlock(&adminHAInfo->lock);

        /* check if restartNeeded is set - due to change in ClusterMembership enable/disable flag */
        if ( ism_config_json_getRestartNeededFlag() == 1 && ism_ha_admin_isUpdateRequired() == 0 ) {
            ism_admin_initRestart(5);
        } else {
            ism_admin_send_stop(ISM_ADMIN_STATE_TERMSTORE);
            LOG(NOTICE, Admin, 6112, "%s", "High Availability mode change \"{0}\" is initiated.", "TERMINATE_STORE");
        }

        pthread_spin_lock(&adminHAInfo->lock);
        adminHAInfo->sSyncStart = 0;
        pthread_spin_unlock(&adminHAInfo->lock);
        break;

    case ISM_HA_ACTION_STANDBY_SYNC_START:
        LOG(NOTICE, Admin, 6112, "%s", "High Availability mode change \"{0}\" is initiated.", "STANDBY_SYNC_START");
        pthread_spin_lock(&adminHAInfo->lock);
        adminHAInfo->sSyncStart = 1;
        pthread_spin_unlock(&adminHAInfo->lock);
        break;

    case ISM_HA_ACTION_STANDBY_SYNC_STOP:
        LOG(NOTICE, Admin, 6112, "%s", "High Availability mode change \"{0}\" is initiated.", "STANDBY_SYNC_STOP");
        pthread_spin_lock(&adminHAInfo->lock);
        adminHAInfo->sSyncStart = 0;
        pthread_spin_unlock(&adminHAInfo->lock);
        break;

    case ISM_HA_ACTION_TERMINATE_STANDBY:
        ism_admin_send_stop(ISM_ADMIN_STATE_TERMSTORE);
        LOG(NOTICE, Admin, 6112, "%s", "High Availability mode change \"{0}\" is initiated.", "TERMINATE_STANDBY");
        pthread_spin_lock(&adminHAInfo->lock);
        adminHAInfo->sSyncStart = 0;
        pthread_spin_unlock(&adminHAInfo->lock);
        break;

    case ISM_HA_ACTION_SPLITBRAIN_RESTART:
        ism_admin_init_stop(ISM_ADMIN_MODE_CLEANSTORE, NULL);
        LOG(NOTICE, Admin, 6112, "%s", "High Availability mode change \"{0}\" is initiated.", "SPLITBRAIN_RESTART");
        pthread_spin_lock(&adminHAInfo->lock);
        adminHAInfo->sSyncStart = 0;
        pthread_spin_unlock(&adminHAInfo->lock);
        break;

    default:
        break;
    }
}

static int ism_admin_ha_send_configData(const char *generatedGroup, int generatedGroupLen) {
    ismHA_AdminMessage_t adminMsg;
    int rc = ISMRC_OK;

    /* Persist generatedGroup if returned by store during state transfer */
    if ( generatedGroupLen && generatedGroup ) {
        ism_config_setHAGroupID(generatedGroup, generatedGroupLen, NULL);
    }

    /* Transfer Admin state to standby
     * - Send admin message with configuration changes to standby
     */

    /* dump config content to a temp file */
    TRACE(4, "Create primary server config file for transfer to standy\n");
    rc = ism_config_dumpJSONConfig(TMP_PRIMCFG);
    if ( rc != ISMRC_OK ) {
        TRACE(1, "Failed to create primary server config file. RC=%d\n", rc);
    } else {
        /* send file to standby */
        TRACE(4, "Send config file to standy\n");
        rc = ism_ha_store_transfer_file(TMP_CONFIGDIR, "serverDynamic.primary");
        if ( rc != ISMRC_OK ) {
            TRACE(1, "Failed to transfer config to Standby node. RC=%d\n", rc);
        } else {
            TRACE(4, "Config file is transferred to standy\n");
        }
    }

    /* transfer other files */
    rc = ism_admin_ha_syncFiles();

    if ( rc == ISMRC_OK ) {
        /* We have sent the config file from primary to standby
         * Check config state on standby
         */
        int startSync = 0;
        for (;;) {
            char *pData = (char *)alloca(8);
            if ( startSync == 0 ) {
                sprintf(pData, "Start\n");
                startSync = 1;
            } else {
                sprintf(pData, "Check\n");
            }
            pData[5] = 0;
            memset(&adminMsg, 0, sizeof(adminMsg));
            adminMsg.pData = pData;
            adminMsg.DataLength = 5;
            adminMsg.pResBuffer = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,538),256);
            memset(adminMsg.pResBuffer, 0, 256);
            adminMsg.ResBufferLength = 256;
            rc = ism_ha_store_send_admin_msg(&adminMsg);
            if ( rc != ISMRC_OK ) {
                TRACE(1, "Failed to send admin message to the standby node. RC=%d\n", rc);
                break;
            }
            /* check returned result */
            TRACE(6, "Config SYNC response from Standby: %s\n", adminMsg.pResBuffer);
            if (!strcmpi(adminMsg.pResBuffer, "SyncComplete"))
                break;

            if (!strcmpi(adminMsg.pResBuffer, "SyncError")) {
                TRACE(1, "Config SYNC failed on Standby\n");
                rc = ISMRC_AdmnHAStandbyError;
                break;
            }

            ism_common_sleep(5000000);
        }

        if ( adminMsg.pResBuffer )
            ism_common_free(ism_memory_admin_misc,adminMsg.pResBuffer);
    } else {
        ism_common_setError(rc);
    }

    return rc;
}

/* Process Transfer state action */
static int ism_admin_ha_process_transfer_state(const char *generatedGroup, int generatedGroupLen) {
    int rc = ISMRC_OK;

    /* send config data */
    rc = ism_admin_ha_send_configData(generatedGroup, generatedGroupLen);

    /* Inform store that admin state is transferred */
    rc = ism_ha_store_transfer_state_completed(rc);

    /* set standby sync flag */
    pthread_spin_lock(&adminHAInfo->lock);
    int sSyncStart = 1;
    if ( rc != ISMRC_OK ) {
        TRACE(1, "Failed to transfer admin state to the standby node. RC=%d\n", rc);
        sSyncStart = 0;
    }
    adminHAInfo->sSyncStart = sSyncStart;
    pthread_spin_unlock(&adminHAInfo->lock);

    /* get standby servername */
    ism_ha_admin_get_standby_serverName(localServerName);

    return rc;
}



/* Process admin message received by standby node */
static void ism_admin_ha_process_admin_message(char *msg, int msglen) {
    int rc = ISMRC_OK;

    if ( *msg == 'S' ) {
    	standbySyncInProgress = 1;
        standbyConfigSyncStatus = 0;

        TRACE(1, "Start SYNC on Standby node: Sync certificates.\n");
        /* Sync certs on standby node */
        /* Backup certificates */
        if (ism_admin_ha_startScript(IMA_SVR_INSTALL_PATH "/bin/syncCerts.sh", "syncCerts.sh", "standby") != 0) {
            TRACE(1, "Could not restore from certificate backup file\n");
            rc = ISMRC_AdmnHAStandbyError;
            ism_common_setError(rc);
            ism_admin_setLastRCAndError();
            standbyConfigSyncStatus = 1;
        }
        TRACE(1, "SYNC on Standby node: Sync configuration.\n");
        /* Process configuration changes */
        rc = ism_config_processJSONConfig(TMP_PRIMCFG);
        if ( rc != ISMRC_OK ) {
            TRACE(1, "Failed to sync standby node\n");
            rc = ISMRC_AdmnHAStandbyError;
            ism_common_setError(rc);
            ism_admin_setLastRCAndError();
            standbyConfigSyncStatus = 1;
            syncState = -1;
        } else {
            TRACE(1, "SYNC on Standby node: Server dynamic configuration SYNC is complete.\n");
            syncState = 1;

            /* Update config file */
            int getLock = 0;
            int rc1 = ism_config_json_updateFile(getLock);
            TRACE(3, "SYNC on Standby node: update config file. rc=%d\n", rc1);
        }
        standbySyncInProgress = 0;
    } else if ( *msg == '{' || *msg == '[' ) {
        int storeMQCData = 1;
        rc = ism_config_processJSONString(proctype, msglen, msg, NULL, storeMQCData);
        if ( rc != ISMRC_OK ) {
            ism_common_setError(rc);
        }
    } else if ( *msg == 'K' ) {
        TRACE(4, "Synchronize certificate on standby node: Start\n");
        if (ism_admin_ha_startScript(IMA_SVR_INSTALL_PATH "/bin/syncCerts.sh", "syncCerts.sh", "standby") != 0) {
            TRACE(1, "Synchronize certificate on standby node: Failed\n");
            rc = ISMRC_AdmnHAStandbyError;
            ism_common_setError(rc);
        } else {
            TRACE(4, "Synchronize certificate on standby node: End\n");
        }
    } else if ( *msg == 'X' ) {
        TRACE(4, "Restart standby node.\n");
        rc = ism_admin_init_stop(ISM_ADMIN_MODE_RESTART, NULL);
        if (rc != ISMRC_OK) {
            TRACE(1, "Restart standby node: Failed\n");
            ism_common_setError(rc);
        } else {
            TRACE(4, "Restart standby node is initiated.\n");
        }
    } else if ( *msg == 'Y' ) {
        TRACE(4, "CleanStore standby node.\n");
        rc = ism_admin_init_stop(ISM_ADMIN_MODE_CLEANSTORE, NULL);
        if (rc != ISMRC_OK) {
            TRACE(1, "CleanStore standby node: Failed\n");
            ism_common_setError(rc);
        } else {
            TRACE(4, "CleanStore on standby node is initiated.\n");
        }
    } else if ( *msg == 'H' ) {
        if (!strcmp(msg, "HADisabldeInCluster")) {
            TRACE(4, "Disable HA in Cluster environment.\n");
            rc = ism_admin_disableHACluster();
            if (rc != ISMRC_OK) {
                TRACE(1, "Disable HA in Cluster environment: Failed\n");
                ism_common_setError(rc);
            } else {
                TRACE(4, "Disable HA in Cluster environment is initiated.\n");
            }
        }
    }

    if ( msg ) {
        ism_common_free(ism_memory_admin_misc,msg);
    }
}

/* Admin HA thread */
static void * ism_admin_ha_thread(void * param, void * context, int value) {
    if ( adminHAInfo == NULL ) {
        TRACE(1, "HA Admin is not initialized yet.\n");
        return NULL;
    }

    for (;;) {
        pthread_mutex_lock(&adminHAInfo->threadMutex);
        pthread_cond_wait(&adminHAInfo->threadCond, &adminHAInfo->threadMutex);

        TRACE(9, "HA Admin Thread - received signal\n");

        if ( adminHADisable_forCleanStore == 1 ) {
            TRACE(1, "ism_admin_ha_thread: HA is disabled for clean store\n");
            pthread_mutex_unlock(&adminHAInfo->threadMutex);
            continue;
        }

        if ( adminHAExitFlagSet == 1 ) {
            adminHAExitFlagSet = 0;
            TRACE(1, "HA Admin is stopped.\n");
            /* Destroy HA action list */
            ism_common_list_destroy(haActionList);
            ism_common_free(ism_memory_admin_misc,haActionList);
            haActionList= NULL;
            pthread_mutex_unlock(&adminHAInfo->threadMutex);
            return NULL;
        }

        /* check if data exist in the list */
        ismAdminActionList_t *actionItem;
        while ( ism_common_list_remove_head(haActionList, (void *)&actionItem) == 0 ) {
            int action = actionItem->actionType;
            TRACE(1, "HA Admin Thread - process action: %d\n", action);

            /* Process action */
            switch(action) {
                case ISM_HA_ADMIN_NOTIFY_VIEW_CHANGE:
                {
                    int viewChange = actionItem->actionIntVal;
                    ism_admin_ha_process_view_change(viewChange);
                    break;
                }

                case ISM_HA_ADMIN_NOTIFY_TRANSFER_STATE:
                {
                    __sync_val_compare_and_swap( &haSyncInProgress, haSyncInProgress, 1);
                       TRACE(4, "Set haSyncInProgress: %d\n", haSyncInProgress);
                    if ( ism_admin_ha_process_transfer_state(actionItem->actionStrVal, actionItem->actionMsgLen) != ISMRC_OK ) {
                        __sync_val_compare_and_swap( &haSyncInProgress, haSyncInProgress, 0);
                    }
                    LOG(NOTICE, Admin, 6113, NULL, "High Availability state transfer is initiated on the Primary node.");
                    break;
                }

                case ISM_HA_ADMIN_NOTIFY_ADMIN_MESSAGE:
                {
                    char *msg = actionItem->actionStrVal;
                    int msglen = actionItem->actionMsgLen;
                    ism_admin_ha_process_admin_message(msg, msglen);
                    LOG(NOTICE, Admin, 6114, NULL, "High Availability node has received an Administrative message from the Primary node.");
                    break;
                }

                case ISM_HA_ADMIN_NOTIFY_RESET:
                default:
                {
                    TRACE(2, "HA notify is %d\n", action);
                    break;
                }
            }
        }

        TRACE(9, "HA Admin Thread - cycle complete\n");
        pthread_mutex_unlock(&adminHAInfo->threadMutex);
    }

    return NULL;
}


/* Free entry of admin action list */
void ism_admin_freeActionEntry(void *data) {
    ismAdminActionList_t *item = (ismAdminActionList_t *)data;
    if ( !item )
        return;

    if ( item->actionStrVal )
        ism_common_free(ism_memory_admin_misc,item->actionStrVal);

    ism_common_free(ism_memory_admin_misc,item);
    item = NULL;
    return;
}

/* Called by ism_admin_init() to initialize locks in HA environment */
XAPI void ism_adminHA_init_locks(void) {
    if ( haLocksInited == 0 ) {
        haLocksInited = 1;
        adminHAInfo = (ismAdminHAInfo_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,544),1, sizeof(ismAdminHAInfo_t));
        pthread_mutex_init(&adminHAInfo->threadMutex, 0);
        pthread_cond_init(&adminHAInfo->threadCond, 0);
        pthread_spin_init(&adminHAInfo->lock, 0);

        /* Process config record file */
        pthread_spin_init(&cfgLogLock, 0);
        pthread_mutex_init(&fileSyncLock, 0);
    }
    return;
}

/* Called by ism_admin_init() to initialize HA in IMA server */
XAPI int32_t ism_adminHA_init(void) {

    proctype = ism_admin_getServerProcType();

    if ( proctype == ISM_PROTYPE_TRACE || proctype == ISM_PROTYPE_SNMP ) {
        /* No need to initialize HA for traceprocessor or SNMP agent */
        return ISMRC_OK;
    }

    proctype = ism_admin_getServerProcType();
    ism_adminHA_init_locks();
    adminHAInfo->sSyncStart = -1;

    /* Initialize config admin message */
    configAdminMsg = (ismHA_AdminMessage_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,545),1, sizeof(ismHA_AdminMessage_t));

    /* Initialize list for config data from Primary */
    haActionList = (ism_common_list *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,546),1, sizeof(ism_common_list));
    ism_common_list_init(haActionList, 1, (void *)ism_admin_freeActionEntry);



    /* start admin HA thread in imaserver process */
    if ( proctype == ISM_PROTYPE_SERVER ) {
        ism_common_startThread( &adminHAInfo->threadId, ism_admin_ha_thread,
            NULL, NULL, 0, ISM_TUSAGE_NORMAL, 0, "AdminHA", "Admin HA thread");
    }

    /* certificate dirs */
    /* LDAP Certificate files */
    ldapCertDir = (char *)ism_common_getStringProperty(
    ism_common_getConfigProperties(), "LDAPCertificateDir");

    /* MQC Certificate files */
    mqCertDir = (char *)ism_common_getStringProperty(
    ism_common_getConfigProperties(), "MQCertificateDir");

    /* keystore Certificate files */
    keyCertDir = (char *)ism_common_getStringProperty(
    ism_common_getConfigProperties(), "KeyStore");

    /* truststore Certificate files */
    trustCertDir = (char *)ism_common_getStringProperty(
    ism_common_getConfigProperties(),"TrustedCertificateDir");

    if ( proctype == ISM_PROTYPE_SERVER ) {
        localServerName = ism_config_getServerName();
        TRACE(5, "Local ServerName: %s\n", localServerName?localServerName:"(null)");
    }

    return ISMRC_OK;
}

XAPI int ism_adminHA_term_thread(void) {
	/* engine cunit tests is not using admin init and term function correctly, so there is a possiblity that adminHAInfo is
	 * not initialized. Terminate HA thread only if adminHAInfo is initialized.
	 */
	if ( adminHAInfo ) {
		/* set AdminHA thread exit flag and send signal to terminate HA thread */
		adminHAExitFlagSet = 1;
        pthread_cond_signal(&adminHAInfo->threadCond);
	}

	return 0;
}

/* Called by ism_admin_term() to cleanup HA */
XAPI int32_t ism_adminHA_term(void) {
    if ( configAdminMsg ) {
        ism_common_free(ism_memory_admin_misc,configAdminMsg);
    }
    if ( clientSock != -1 ) {
        close(clientSock);
        clientSock = -1;
    }
    if ( admin_haServerSock != -1 ) {
        close(admin_haServerSock);
        admin_haServerSock = -1;
    }
    return ISMRC_OK;
}

/* Returns current role to CLI/UI */
XAPI ismHA_Role_t ism_admin_getHArole(concat_alloc_t *output_buffer, int *rc) {
    ismHA_Role_t role = ISM_HA_ROLE_PRIMARY;
    *rc = ISMRC_OK;

    /* invoke store ha API to get current mode */
    ismHA_View_t haView = {0};
    *rc = ism_ha_store_get_view(&haView);

    if ( *rc == ISMRC_OK ) {
        role = haView.NewRole;
        TRACE(7, "NewRole:%d OldRole:%d ActiveNodes:%d SyncNodes:%d ReasonCode:%d ReasonStr:%s LocalReplicationNIC:%s LocalDiscoveryNIC:%s RemoteDiscoveryNIC:%s\n",
            haView.NewRole, haView.OldRole, haView.ActiveNodesCount,
            haView.SyncNodesCount, haView.ReasonCode,
            haView.pReasonParam? haView.pReasonParam:"Unknown",
            haView.LocalReplicationNIC?haView.LocalReplicationNIC:"",
            haView.LocalDiscoveryNIC?haView.LocalDiscoveryNIC:"",
            haView.RemoteDiscoveryNIC?haView.RemoteDiscoveryNIC:"");
    } else {
        TRACE(2, "ism_ha_store_get_view() returned Error: RC=%d\n", *rc);
    }

    /* create return buffer */
    if ( output_buffer ) {
        char rbuf[2048];
        sprintf(rbuf,
                "{ \"NewRole\":\"%s\",\"OldRole\":\"%s\",\"ActiveNodes\":\"%d\",\"SyncNodes\":\"%d\",\"ReasonCode\":\"%d\",\"ReasonString\":\"%s\",\"LocalReplicationNIC\":\"%s\",\"LocalDiscoveryNIC\":\"%s\",\"RemoteDiscoveryNIC\":\"%s\",\"RemoteServerName\":\"%s\" }",
                ism_admin_get_harole_string(haView.NewRole), ism_admin_get_harole_string(haView.OldRole),
                haView.ActiveNodesCount, haView.SyncNodesCount, haView.ReasonCode,
                haView.pReasonParam? haView.pReasonParam:"",
                haView.LocalReplicationNIC?haView.LocalReplicationNIC:"",
                haView.LocalDiscoveryNIC?haView.LocalDiscoveryNIC:"",
                haView.RemoteDiscoveryNIC?haView.RemoteDiscoveryNIC:"",
                remoteServerName?remoteServerName:"");

        ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
    }

    return role;
}

#if 0
/* get file size */
static int ism_config_getFileSize(char *fname) {
    /* check the file size */
    FILE *file;
    size_t sz = 0;
    if (( file = fopen(fname, "r")) == NULL ) {
        return 0;
    }
    fseek(file, 0, SEEK_END);
    sz = ftell(file);
    fclose(file);
    TRACE(9, "File:%s  Size:%d\n", fname, (int)sz);
    return sz;
}
#endif


/* Synchronize files and certificates */
static int32_t ism_admin_ha_syncFiles(void) {
    int32_t rc = ISMRC_OK;

    /* sync certificates */
    /* Backup certificates */
    TRACE(1, "Certificate synchronization: START\n");
    if (ism_admin_ha_startScript(IMA_SVR_INSTALL_PATH "/bin/syncCerts.sh", "syncCerts.sh", "primary") != 0) {
        TRACE(1, "Could not create backup of certificate dirs\n");
        rc = ISMRC_AdmnHAPrimaryError;
    } else {
        /* transfer certificate tar file */
        rc = ism_ha_store_transfer_file(TMP_CONFIGDIR, "certdir.tar");
    }
    TRACE(1, "Certificate synchronization: END: rc=%d\n", rc);

#if 0
    if ( rc == ISMRC_OK ) {
        /* Transfer MQConneectivity */
        TRACE(1, "Certificate synchronization MQConnectivity: START\n");
        char *cfgdir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(), "ConfigDir");
        if (!cfgdir) {
            TRACE(2, "ConfigDir is not specified\n");
            rc = ISMRC_PropertyNotFound;
            ism_common_setError(rc);
            return rc;
        }

        /* check the file size */
        char fname[1024];
        sprintf(fname, "%s/mqcbridge_dynamic.cfg", cfgdir);
        if ( ism_config_getFileSize(fname) > 0 ) {
            rc = ism_ha_store_transfer_file(cfgdir, "mqcbridge_dynamic.cfg");
            if ( rc != ISMRC_OK ) {
                LOG(ERROR, Admin, 6116, "%s%d", "Failed to transfer \"{0}\" to Standby node: RC={1}.", fname, rc);
            } else {
                TRACE(4, "MQConnectivity config file is transferred to Standby node\n");
            }
        }
        TRACE(1, "Certificate synchronization MQConnectivity: END : rc=%d\n", rc);
    }
#endif

    if ( rc != ISMRC_OK ) {
        ism_common_setError(rc);
    }

    return rc;
}

XAPI int32_t ism_admin_ha_queueAdminAction(int actionType, int actionValue, ismHA_AdminMessage_t *adminMsg) {
    int32_t rc = ISMRC_OK;
    int addAction = 0;
    char *msg = NULL;
    int actionDone = 0;

    if ( adminHADisable_forCleanStore == 1 ) {
        TRACE(1, "AdminMsgQ: HA is disabled for clean store\n");
        rc = ISMRC_Error;
        return rc;
    }

    ismAdminActionList_t *actionItem = (ismAdminActionList_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,548),sizeof(ismAdminActionList_t));
    if ( !actionItem ) {
        TRACE(1, "AdminMsgQ: actionItem malloc failed\n");
        return ISMRC_AllocateError;
    }

    switch(actionType) {

    case ISM_HA_ADMIN_NOTIFY_ADMIN_MESSAGE:
    {
        if ( adminMsg && adminMsg->DataLength <=  0 ) {
            TRACE(1, "Received ZERO bytes Admin message from store\n");
            ism_common_free(ism_memory_admin_misc,actionItem);
            rc = ISMRC_NullPointer;
            return rc;
        }

        msg = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,550),adminMsg->DataLength + 1);                    /* BEAM suppression: operating on NULL */
        if ( msg == NULL ) {
               ism_common_free(ism_memory_admin_misc,actionItem);
               TRACE(1, "AdminMsgQ: malloc failed\n");
               return ISMRC_AllocateError;
        }
        memcpy(msg, adminMsg->pData, adminMsg->DataLength);
        msg[adminMsg->DataLength] = 0;
        actionItem->actionStrVal = msg;
        actionItem->actionMsgLen = adminMsg->DataLength;
        actionItem->actionType = ISM_HA_ADMIN_NOTIFY_ADMIN_MESSAGE;
        actionItem->actionIntVal = 0;

        /* add to action list */
        addAction = 1;

        /* set return message */
        if ( *msg == 'S' ) {    /* Start Sync */
            sprintf(adminMsg->pResBuffer, "SyncStarted");
            adminMsg->ResLength = 11;
            startStandbySync = 1;
        } else if ( *msg == '[' || *msg == '{' ) {   /* start config message queue */
            sprintf(adminMsg->pResBuffer, "UpdateQueued");
            adminMsg->ResLength = 13;
        } else if ( *msg == 'K' ) {   /* start cert sync */
            sprintf(adminMsg->pResBuffer, "Done");
            adminMsg->ResLength = 5;
        } else if ( *msg == 'G' ) {
        	/* set HA Group */
        	addAction = 0;
        	char *group = msg + 2;
        	if ( ism_config_setHAGroupID(NULL, 0, group) == ISMRC_OK ) {
        		sprintf(adminMsg->pResBuffer, "%s", "0");
        		adminMsg->ResLength = strlen(adminMsg->pResBuffer);
        	} else {
        		sprintf(adminMsg->pResBuffer, "%s", "1");
        		adminMsg->ResLength = strlen(adminMsg->pResBuffer);
        	}
        	actionDone = 1;
        } else if ( *msg == 'W' ) {   /* Send ServerName */
            addAction = 0;
            /* check if message contains remote ServerName */
            if ( adminMsg->DataLength > 2 ) {
            	if ( remoteServerName ) ism_common_free(ism_memory_admin_misc,remoteServerName);
            	char *remUI = msg+2;
            	remoteServerName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),remUI);
            	TRACE(5, "Standby: Received ServerName from Primary: %s\n", remoteServerName);
            }
            char *srvname = ism_config_getServerName();
            if ( srvname && *srvname != '\0') {
            	if ( localServerName ) ism_common_free(ism_memory_admin_misc,localServerName);
            	localServerName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),srvname);
            }
            sprintf(adminMsg->pResBuffer, "%s", srvname?srvname:"");
            adminMsg->ResLength = strlen(adminMsg->pResBuffer);
            actionDone = 1;
            TRACE(5, "Standby: Returned ServerName to Primary: %s\n", adminMsg->pResBuffer);
        } else {
            if ( syncState == 1 ) {
                sprintf(adminMsg->pResBuffer, "SyncComplete");
                adminMsg->ResLength = 12;
            } else if ( syncState == -1 || standbyConfigSyncStatus == 1 ) {
                sprintf(adminMsg->pResBuffer, "SyncError");
                adminMsg->ResLength = 9;
            } else {
                sprintf(adminMsg->pResBuffer, "SyncContinue");
                adminMsg->ResLength = 12;
            }
        }
        if ( addAction == 1 ) {
            TRACE(9, "Add AdminMsg to ActionQ: %s\n", msg);
        }
        break;
    }

    case ISM_HA_ADMIN_NOTIFY_VIEW_CHANGE:
    {
        actionItem->actionType = ISM_HA_ADMIN_NOTIFY_VIEW_CHANGE;
        actionItem->actionIntVal = actionValue;
        actionItem->actionStrVal = NULL;

        /* add to action list */
        addAction = 1;
        TRACE(1, "Add ViewChange to ActionQ\n");
        break;

    }

    case ISM_HA_ADMIN_NOTIFY_TRANSFER_STATE:
    {
        actionItem->actionStrVal = (char *)adminMsg->pData;
        if ( adminMsg->pData ) {
            actionItem->actionMsgLen = strlen(adminMsg->pData);
        } else {
            actionItem->actionMsgLen = 0;
        }
        actionItem->actionType = ISM_HA_ADMIN_NOTIFY_TRANSFER_STATE;
        actionItem->actionStrVal = NULL;
        actionItem->actionIntVal = 0;

        /* add to action list */
        addAction = 1;
        TRACE(1, "Add TransferState to ActionQ\n");
        break;
    }

    default:
    {
        TRACE(3, "HAAdminActionQ: Invalid action type: %d\n", actionType);
        break;
    }
    }

    if ( addAction == 1 ) {
        /* add action to list */
        rc = ism_common_list_insert_tail(haActionList, (const void *)actionItem);
        if ( rc != ISMRC_OK ) {
            if ( msg )
                ism_common_free(ism_memory_admin_misc,msg);
            ism_common_free(ism_memory_admin_misc,actionItem);
            TRACE(1, "AdminMsgQ: Insert list - malloc failed\n");
            ism_common_setError(rc);
            return ISMRC_AllocateError;
        }

        TRACE(4, "HA AdminAction: queued: type=%d val=%d msg=%s\n",
            actionType, actionValue, msg?msg:"");
        if (actionValue == 3) {
            TRACE(4, "ActionValue is 3. Unset haSyncInProgress.\n");
            __sync_val_compare_and_swap( &haSyncInProgress, haSyncInProgress, 0);
        }
    } else {
        if ( actionDone == 0 ) {
            TRACE(3, "HA AdminAction: Failed to queue: type=%d val=%d msg=%s rc=%d\n",
                actionType, actionValue, msg?msg:"", rc);
        }
        ism_common_free(ism_memory_admin_misc,actionItem);
    }

    return rc;				/* BEAM suppression: memory leak */
}

XAPI int32_t ism_ha_admin_update_standby(char *inpbuf, int inpbuflen, int updateCert) {
    int32_t rc = ISMRC_OK;

    if ( !inpbuf || inpbuflen == 0 ) {
        /* can happen if updates are required only on primary node */
        return rc;
    }

    /* cheeck current HA role */
    pthread_spin_lock(&adminHAInfo->lock);
    int syncStandby = adminHAInfo->sSyncStart;
    pthread_spin_unlock(&adminHAInfo->lock);

    if ( syncStandby == 1 ) {
        /* send input buffer as admin message */
        TRACE(9, "Send config data to stanby. len=%d\n", (int)strlen(inpbuf));

        char *tmpstr = alloca(inpbuflen);
        char resBuffer[MAX_RETURN_ADMINMSG_LENGTH];
        if ( tmpstr ) {                            /* BEAM suppression: constant condition */
            if ( updateCert == 1) {
                TRACE(5, "Update certificates \n");
                ism_admin_ha_sendCertificates();
                configAdminMsg->pData = tmpstr;
                memset(tmpstr, 0, inpbuflen);
                *tmpstr = 'K';
                configAdminMsg->DataLength = 2;
                configAdminMsg->pResBuffer = resBuffer;
                configAdminMsg->ResBufferLength = MAX_RETURN_ADMINMSG_LENGTH;
                rc = ism_ha_store_send_admin_msg(configAdminMsg);
                if ( rc != ISMRC_OK ) {
                    TRACE(1, "Failed to send apply certificate message to standby node. RC=%d\n", rc);
                } else {
                    TRACE(1, "Certificates received by standby\n");
                    ism_common_sleep(1000);
                }
            }
            if ( rc == ISMRC_OK ) {
                configAdminMsg->pData = tmpstr;
                memset(tmpstr, 0, inpbuflen);
                memcpy( tmpstr, inpbuf, inpbuflen);
                configAdminMsg->DataLength = inpbuflen;
                configAdminMsg->pResBuffer = resBuffer;
                configAdminMsg->ResBufferLength = MAX_RETURN_ADMINMSG_LENGTH;
                rc = ism_ha_store_send_admin_msg(configAdminMsg);
                if ( rc != ISMRC_OK ) {
                    TRACE(1, "Failed to send config data to standby node. RC=%d\n", rc);
                } else {
                    TRACE(5, "Config message sent to standby: %s\n",
                        configAdminMsg->pResBuffer?configAdminMsg->pResBuffer:"NULL");
                }
            }
        } else {
            rc = ISMRC_AllocateError;
        }
    }

    if ( rc != ISMRC_OK )
        ism_common_setError(rc);

    return rc;
}


/* Send message to standby via store */
XAPI int32_t ism_admin_ha_send_configMsg(char *configMsg, int msgLen) {
    int32_t rc = ISMRC_OK;
    ismHA_AdminMessage_t adminMsg;

    /* check current HA role */
    pthread_spin_lock(&adminHAInfo->lock);
    int syncStandby = adminHAInfo->sSyncStart;
    pthread_spin_unlock(&adminHAInfo->lock);

    if ( syncStandby == 1 ) {
        int resLen=2048;
        char resBuffer[resLen];
        char *pData = (char *)alloca(msgLen+1);
        memcpy(pData, configMsg, msgLen);
        pData[msgLen] = 0;
        memset(&adminMsg, 0, sizeof(adminMsg));
        adminMsg.pData = pData;
        adminMsg.DataLength = msgLen+1;
        adminMsg.pResBuffer = resBuffer;
        memset(resBuffer, 0, resLen);
        adminMsg.ResBufferLength = resLen;
        rc = ism_ha_store_send_admin_msg(&adminMsg);
        if ( rc != ISMRC_OK ) {
            TRACE(1, "Failed to send MQC config message to the standby node. RC=%d\n", rc);
            ism_common_setError(rc);
         }
    }

    return rc;
}


static int32_t ism_admin_ha_sendCertificates(void) {
    int32_t rc = ISMRC_OK;

    TRACE(5, "Transfer certificate files: START\n");
    /* backup cert dir */
    if (ism_admin_ha_startScript(IMA_SVR_INSTALL_PATH "/bin/syncCerts.sh", "syncCerts.sh", "primary") != 0) {
        TRACE(1, "Could not create backup of certificate dirs\n");
    } else {
        /* transfer certificate tar file */
        rc = ism_ha_store_transfer_file(TMP_CONFIGDIR, "certdir.tar");
    }
    TRACE(5, "Transfer certificate files: END: %d\n", rc);

    return rc;
}

XAPI int32_t ism_admin_ha_syncMQCertificates(void) {
    int32_t rc = ISMRC_OK;
    if ( mqCertDir ) {
        TRACE(5, "Transfer MQC certificates: START\n");
        rc = ism_ha_store_transfer_file(mqCertDir, "mqconnectivity.kdb");
        if ( rc == ISMRC_OK ) {
            rc = ism_ha_store_transfer_file(mqCertDir, "mqconnectivity.sth");
        }
        TRACE(5, "Transfer MQC certificates: END: %d\n", rc);
    }
    return rc;
}

XAPI int32_t ism_admin_ha_syncFile(const char *dirname, const char *filename) {
    int32_t rc = ISMRC_OK;

    if ( !dirname || (dirname && *dirname == '\0') || !filename || (filename && *filename == '\0')) {
        TRACE(3, "Invalid DIR (%s) and/or FILE (%s) for transfer\n", dirname?dirname:"NULL", filename?filename:"NULL");
        return ISMRC_NotFound;
    }

    /* check current HA role */
    ismHA_View_t haView = {0};
    ism_ha_store_get_view(&haView);
    if ( haView.NewRole == ISM_HA_ROLE_PRIMARY ) {
        rc = ism_ha_store_transfer_file((char *)dirname, (char *)filename);
        TRACE(5, "File sync is initiated: dirname=%s filename=%s\n", dirname, filename);
    } else {
        TRACE(4, "Not a Primary node. File Sync is ignored. dirname=%s filename=%s\n", dirname, filename);
    }

    return rc;
}

int ism_admin_getIfAddresses(char **ip, int *count, int includeIPv6) {
    struct ifaddrs *ifafirst;
    struct ifaddrs *ifap;
    char *addrStr = NULL;
    int num = 0;

    /* Check first interface */
    if ( getifaddrs(&ifafirst) != 0 )
        return 0;


    /* loop thru all interfaces */
    for (ifap = ifafirst; ifap; ifap = ifap->ifa_next)
    {
        /* discard loopback, unconfigured and down interfaces */
        if ( ifap->ifa_addr == NULL || (ifap->ifa_flags & IFF_UP) == 0 )
        {
            continue;
        }

        /* Check for IPv4 address */
        if ( ifap->ifa_addr->sa_family == AF_INET )
        {
            struct sockaddr_in *sa4;
            char buf[64];

            /* get IP address */
            sa4 = (struct sockaddr_in *)(ifap->ifa_addr);
            addrStr = (char *)inet_ntop( ifap->ifa_addr->sa_family,
                 &(sa4->sin_addr), buf, sizeof(buf));

            if ( addrStr )
            {
                ip[num] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),buf);
                num++;
            }
        }

        /* Check for IPv6 address */
        if ( ifap->ifa_addr->sa_family == AF_INET6 )
        {
            struct sockaddr_in6 *sa6;
            char buf[64];
            char format[68];

            /* get IP address */
            sa6 = (struct sockaddr_in6 *)(ifap->ifa_addr);
            addrStr = (char *)inet_ntop( ifap->ifa_addr->sa_family,
                 &(sa6->sin6_addr), buf, sizeof(buf));

        	if ( addrStr ) {
                if (IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr)) {
                    TRACE(7, "%s is link-local\n", buf);
                    if (!includeIPv6) continue;
                } else if (IN6_IS_ADDR_SITELOCAL(&sa6->sin6_addr)) {
                	TRACE(9, "%s is site-local\n", buf);
                } else if (IN6_IS_ADDR_V4MAPPED(&sa6->sin6_addr)) {
                	TRACE(9, "%s is v4mapped\n", buf);
                } else if (IN6_IS_ADDR_V4COMPAT(&sa6->sin6_addr)) {
                	TRACE(9, "%s is v4compat\n", buf);
                } else if (IN6_IS_ADDR_LOOPBACK(&sa6->sin6_addr)) {
                	TRACE(9, "%s is host", buf);
                } else if (IN6_IS_ADDR_UNSPECIFIED(&sa6->sin6_addr)) {
                	TRACE(9, "%s is unspecified\n", buf);
                } else {
                	TRACE(9, "%s is global\n", buf);
                }
        		memset(format, '\0', sizeof(format));
        		sprintf(format, "%s%s%s", "[", buf, "]");
                ip[num] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),format);
                num++;
            }
        }
    }
    *count = num;
    freeifaddrs(ifafirst);
    return 1;
}
#define MAX_SSHHOST 100


/**
 * Update Group on Standby node
 */
XAPI int32_t ism_ha_admin_set_standby_group(const char *group) {
    int32_t rc = ISMRC_OK;

    /* check current HA role */
    pthread_spin_lock(&adminHAInfo->lock);
    int syncStandby = adminHAInfo->sSyncStart;
    pthread_spin_unlock(&adminHAInfo->lock);

    if ( syncStandby == 1 ) {
        /* send admin message */
        TRACE(9, "Send update Group to Stanby.\n");
        char updateStr[1024];
        sprintf(updateStr, "G=%s", group);
        int len = strlen(updateStr);
        char *tmpstr = alloca(len+1);
        int resLen=2048;
        char resBuffer[resLen];
        if ( tmpstr ) {                /* BEAM suppression: constant condition */
            configAdminMsg->pData = tmpstr;
            memset(tmpstr, 0, len+1);
            memcpy(tmpstr, updateStr, len);
            configAdminMsg->DataLength = len+1;
            configAdminMsg->ResBufferLength = resLen;
            configAdminMsg->pResBuffer = resBuffer;
            rc = ism_ha_store_send_admin_msg(configAdminMsg);
            if ( rc != ISMRC_OK ) {
                TRACE(1, "Failed to send update group message to Standby node. RC=%d\n", rc);
            } else {
                TRACE(5, "Received configuration from Standby: %s\n",
                    configAdminMsg->pResBuffer?configAdminMsg->pResBuffer:"NULL");
                char *res = configAdminMsg->pResBuffer;
                if ( res && *res == '1')
                	rc = ISMRC_GroupUpdateFailed;
            }
        } else {
            rc = ISMRC_AllocateError;
        }
    }

    if ( rc != ISMRC_OK )
        ism_common_setError(rc);

    return rc;
}

/**
 * Process HA disable case when cluster membership is enabled
 *
 * Generate a new ServerUID on the standby and then terminate the standby server.
 * It is recommended to clean the standby's store in this case or at least advise
 * the user to do so. If the standby is not running when HA is disabled the user
 * should get a warning with an indication that they should clean the store and
 * disable cluster on the standby (when/if it is next started).
 *
 */
XAPI int ism_admin_haDisabledInCluster(int flag) {
    int rc = ISMRC_OK;
    int rc1 = ISMRC_OK;

    /* check node is in an HA pair and is a primary node */
    int isPrimary = ism_ha_admin_isUpdateRequired();
    int inCluster = ism_admin_isClusterConfigured();

    TRACE(9, "Check and send disableHA to standby: flag=%d isPrimary=%d inCluster=%d\n", flag, isPrimary, inCluster);

    if ( isPrimary == 1 && inCluster == 1 && flag == 1 ) {

    	TRACE(6, "HA is disabled on Primary when ClusterMembership is configured. Send HADisabledInCluster message to standby.\n");

        /* Send Message to Standby */
        char *cfgMsg = "HADisabldeInCluster";
        int msglen = strlen(cfgMsg);

        /* Send message to standby till deisgn is finalized */
        rc = ism_admin_ha_send_configMsg(cfgMsg, msglen);
    } else if ( ism_admin_getHArole(NULL, &rc1) == ISM_HA_ROLE_STANDBY && inCluster == 1 && flag == 1 ) {
     	TRACE(6, "HA is disabled on Standby when ClusterMembership is configured.\n");
     	rc = ism_admin_disableHACluster();
    } else if ( ism_admin_getHArole(NULL, &rc1) == ISM_HA_ROLE_STANDBY && inCluster == 0 && flag == 1 ) {
        /* Disable HA */
        rc = ism_config_disableHA();
        if ( rc == ISMRC_OK ) {
        	/* Check if we have org UID - if not cache it */
        	if ( !orgServerUID ) {
        		orgServerUID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ism_common_getServerUID());
        	}

            /* Generate a new ServerUID */
            ism_common_generateServerUID();
        }
    } else if ( ism_admin_getHArole(NULL, &rc1) == ISM_HA_ROLE_STANDBY && inCluster == 0 && flag == 0 ) {
    	/* HA is enabled again before server Standby node restart */
    	/* check if we have cached ServerUID - if so reset to the cached data */
    	if ( orgServerUID ) {
    	    int validate = 0;
    	    char command[1024];
    	    sprintf(command, "{ \"ServerUID\":\"%s\" }", orgServerUID);
    	    rc = ism_config_set_fromJSONStr(command, "ServerUID", validate);
    	    if (rc) {
    	        TRACE(2, "Unable to set dynamic config: item=ServerUID value=%s rc=%d\n", orgServerUID, rc);
    	    }
    		ism_common_setServerUID(orgServerUID);
    	}
    }

    return rc;
}


/* Disable HA on standby node in cluster environment */
int ism_admin_disableHACluster(void) {
    int rc = ISMRC_OK;

    /* Disable HA */
    rc = ism_config_disableHA();

    if ( rc == ISMRC_OK ) {

        /* Generate a new ServerUID */
        ism_common_generateServerUID();

        /* Set AdminMode to maintenance and restart server */
        int restartFlag = 1;
        int errorRC = ISMRC_HADisOnClusterPrim;
        rc = ism_admin_setMaintenanceMode(errorRC, restartFlag);
    }

    return rc;
}
