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

#define TRACE_COMP Admin

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>


#include "adminInternal.h"
#include "configInternal.h"
#include "serverConfigSchema.c"
#include "monitoringSchema.c"


int storeNotAvailableError =ISMRC_OK;

extern int cleanStore;
extern int adminMode;
extern int adminModeRC;
extern int modeChanged;
extern int modeChangedValue;
extern int modeChangedPrev;
extern int modeFirst;
extern int adminInitError;
extern int adminInitWarn;
extern pthread_key_t adminlocalekey;
extern ism_json_parse_t * ConfigSchema;
extern ism_json_parse_t * MonitorSchema;
extern int isConfigSchemaLoad;
extern int isMonitorSchemaLoad;
extern int ismCUNITEnv;

/* Function pointers to overcome build order issues
 *
 * Required function pointers are set in main.c - as global config property and
 * get from global config property where required
 *
 * Pointer for Queue Manager Connection test function in mqcbridge process - this is set in main.c of server_mqcbridge project
 * and used in admin as follows:
 *     qmConnectionTest = (qmConnectionTest_f)(uintptr_t)ism_common_getLongConfig("_TEST_QueueManagerConnection_fnptr", 0L);
 *     if ( qmConnectionTest ) {
 *           rc = qmConnectionTest(pInputQMName, pConnectionName, pChannelName, pSSLCipherSpec);
 *     } else {
 *           rc = ISMRC_NotImplemented;
 *     }
 */
typedef int (*qmConnectionTest_f)(const char *pInputQMName, const char *pConnectionName, const char *pChannelName, const char *pSSLCipherSpec, const char *pUserName, const char *pUserPassword);
static qmConnectionTest_f qmConnectionTest = NULL;

typedef int (*closeConnection_f)(const char * clientID, const char * userID, const char * client_addr, const char * endpoint);
static closeConnection_f closeConnection = NULL;

/* Create JSON string with RC and error string */
int32_t ism_admin_setReturnCodeAndStringJSON(concat_alloc_t *output_buffer, int rc, const char * returnString) {

    char tmpbuf[1024];

    sprintf(tmpbuf, "{ \"RC\":\"%d\", \"ErrorString\":", rc);

    ism_common_allocBufferCopyLen(output_buffer, tmpbuf, strlen(tmpbuf));
    /*JSON encoding for the Error String.*/
    ism_json_putString(output_buffer, returnString);

    ism_common_allocBufferCopyLen(output_buffer, "}", 1);

    return ISMRC_OK;

}

int32_t ism_admin_maskJSONField(const char * line, char * fieldName, char * retValue)
{

        size_t n;
        if(line==NULL || retValue==NULL) return 1;

        char * ptr = strstr(line,fieldName);
        if(ptr == NULL){

            return 1;
        }
        ptr+=strlen(fieldName)+3;
        n = ptr-line;
        ptr = strchr(ptr,'\"');
        memcpy(retValue,line,n);
        sprintf(retValue+n,"******%s",ptr);

        return 0;
}


/* Maps server state to server state string */
char * ism_admin_getStateStr(int type) {
    char *retstr = "Unknown";

    switch (type) {
    case ISM_SERVER_INITIALIZING:
        retstr = "Initializing";
        break;

    case ISM_SERVER_RUNNING:
        retstr = "Running (production)";
        break;

    case ISM_SERVER_STOPPING:
        retstr = "Stopping";
        break;

    case ISM_SERVER_INITIALIZED:
        retstr = "Initialized";
        break;

    case ISM_TRANSPORT_STARTED:
        retstr = "TransportStarted";
        break;

    case ISM_PROTOCOL_STARTED:
        retstr = "ProtocolStarted";
        break;

    case ISM_STORE_STARTED:
        retstr = "StoreStarted";
        break;

    case ISM_ENGINE_STARTED:
        retstr = "EngineStarted";
        break;

    case ISM_MESSAGING_STARTED:
        retstr = "MessagingStarted";
        break;

    case ISM_SERVER_MAINTENANCE:
        retstr = "Running (maintenance)";
        break;

    case ISM_SERVER_STANDBY:
        retstr = "Standby";
        break;

    case ISM_SERVER_STARTING_STORE:
        retstr = "StoreStarting";
        break;

    case ISM_SERVER_STARTING_ENGINE:
        retstr = "EngineStarting";
        break;

    case ISM_SERVER_STARTING_TRANSPORT:
        retstr = "TransportStarting";
        break;

    default:
        retstr = "Unknown";
        break;
    }
    return retstr;
}


/* Return server status string */
char * ism_admin_getStatusStr(int type) {
    char *retstr = "Unknown";

    switch (type) {
    case ISM_SERVER_INITIALIZING:
    case ISM_SERVER_INITIALIZED:
    case ISM_TRANSPORT_STARTED:
    case ISM_PROTOCOL_STARTED:
        retstr = "Initializing";
        break;

    case ISM_SERVER_RUNNING:
    case ISM_MESSAGING_STARTED:
    case ISM_SERVER_MAINTENANCE:
    case ISM_SERVER_STANDBY:
        retstr = "Running";
        break;

    case ISM_SERVER_STOPPING:
        retstr = "Stopping";
        break;

    case ISM_SERVER_STARTING_STORE:
        retstr = "StoreStarting";
        break;

    case ISM_SERVER_STARTING_ENGINE:
    case ISM_ENGINE_STARTED:
    case ISM_STORE_STARTED:
        retstr = "EngineStarting";
        break;

    case ISM_SERVER_STARTING_TRANSPORT:
        retstr = "TransportStarting";
        break;

    default:
        retstr = "Unknown";
        break;
    }
    return retstr;
}


/* Maps action string to action type */
int32_t ism_admin_getActionType(const char *actionStr) {
    int32_t rc = ISM_ADMIN_LAST;
    if ( !actionStr )
        return ISM_ADMIN_LAST;
    if ( !strcasecmp(actionStr, "set"))
        rc = ISM_ADMIN_SET;
    else     if ( !strcasecmp(actionStr, "get"))
        rc = ISM_ADMIN_GET;
    else     if ( !strcasecmp(actionStr, "stop"))
        rc = ISM_ADMIN_STOP;
    else     if ( !strcasecmp(actionStr, "apply"))
        rc = ISM_ADMIN_APPLY;
    else     if ( !strcasecmp(actionStr, "import"))
        rc = ISM_ADMIN_IMPORT;
    else     if ( !strcasecmp(actionStr, "export"))
        rc = ISM_ADMIN_EXPORT;
    else     if ( !strcasecmp(actionStr, "trace"))
        rc = ISM_ADMIN_TRACE;
    else     if ( !strcasecmp(actionStr, "status"))
        rc = ISM_ADMIN_STATUS;
    else     if ( !strcasecmp(actionStr, "version"))
        rc = ISM_ADMIN_VERSION;
    else     if ( !strcasecmp(actionStr, "setmode"))
        rc = ISM_ADMIN_SETMODE;
    else     if ( !strcasecmp(actionStr, "harole"))
        rc = ISM_ADMIN_HAROLE;
    else     if ( !strcasecmp(actionStr, "certsync"))
        rc = ISM_ADMIN_CERTSYNC;
    else     if ( !strcasecmp(actionStr, "test"))
        rc = ISM_ADMIN_TEST;
    else     if ( !strcasecmp(actionStr, "purge"))
        rc = ISM_ADMIN_PURGE;
    else     if ( !strcasecmp(actionStr, "rollback"))
        rc = ISM_ADMIN_ROLLBACK;
    else     if ( !strcasecmp(actionStr, "commit"))
        rc = ISM_ADMIN_COMMIT;
    else     if ( !strcasecmp(actionStr, "forget"))
        rc = ISM_ADMIN_FORGET;
    else     if ( !strcasecmp(actionStr, "filesync"))
        rc = ISM_ADMIN_SYNCFILE;
    else     if ( !strcasecmp(actionStr, "close"))
        rc = ISM_ADMIN_CLOSE;
    else     if ( !strcasecmp(actionStr, "notify"))
            rc = ISM_ADMIN_NOTIFY;
    else     if ( !strcasecmp(actionStr, "msshell"))
        rc = ISM_ADMIN_RUNMSSHELLSCRIPT;
    else     if ( !strcasecmp(actionStr, "create"))
        rc = ISM_ADMIN_CREATE;
    else     if ( !strcasecmp(actionStr, "update"))
        rc = ISM_ADMIN_UPDATE;
    else     if ( !strcasecmp(actionStr, "delete"))
        rc = ISM_ADMIN_DELETE;
    else     if ( !strcasecmp(actionStr, "restart"))
        rc = ISM_ADMIN_RESTART;


    return rc;
}

/* Set run mode */
XAPI int32_t ism_admin_setAdminMode(int mode, int ErrorCode) {
    int rc = ISMRC_OK;
    char jsonSTR[1024];

    sprintf(jsonSTR, "{ \"AdminMode\":%d }", mode);
    rc = ism_config_set_fromJSONStr(jsonSTR, "AdminMode", 0);

    return rc;
}

/* return Message ID */
void ism_admin_getMsgId(ism_rc_t code, char * buffer) {
    sprintf(buffer, "CWLNA%04d", code%10000);
}

/* Timer task to STOP imaserver */
int32_t ism_admin_stopTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
	int rc = ISMRC_OK;
    TRACE(1, "Server is stopped by Admin action\n");

    if ( restartServer == 1 ) {
    	    int rc1 = 0;
        /* check if configuration reset is required */
        if ( resetConfig == 1 ) {
            TRACE(4, "Reset configuration to default and restart server\n");
            rc1 = system(IMA_SVR_INSTALL_PATH "/bin/resetServerConfig.sh 1 2 &");
        } else {
            TRACE(4, "Restart server.\n");
            rc1 = system(IMA_SVR_INSTALL_PATH "/bin/resetServerConfig.sh 0 2 &");
        }
        if (rc1 != 0) {
            TRACE(4, "Reset server config call returned error: rc=%d\n", rc1);
        }
        sleep(2);
    }

    ism_common_shutdown_int(__FILE__, __LINE__, 0);
    if(rc == 0)
    	ism_common_cancelTimer(key);
    return rc;
}


/* Set server runmode */
void ism_admin_change_mode(char *modeStr, concat_alloc_t *output_buffer) {
    char  lbuf[1024];
    const char * repl[3];

    int rc = ISMRC_OK;
    int mode = 0;
    int curmode = ism_admin_getmode();

    TRACE(7, "curmode=%d storeNotAvailableError=%d adminInitError=%d adminModeRC=%d\n",
            curmode, storeNotAvailableError, adminInitError, adminModeRC);
    if ( !modeStr || ( modeStr && *modeStr == '\0' )){
        char tmpbuf[1024];
        int xlen;
        char msgID[12];

        if ( curmode == 0 && (storeNotAvailableError != 0 || adminInitError != ISMRC_OK || adminModeRC != ISMRC_OK ) ) {
            ism_admin_getMsgId(6126, msgID);
            if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey),
                    lbuf, sizeof(lbuf), &xlen) != NULL) {
                repl[0] = (curmode > 0) ? "maintenance" : "production";
                ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
            } else {
                sprintf(tmpbuf, "" IMA_SVR_COMPONENT_NAME_FULL " is in \"maintenance\" mode however the configured mode is \"%s\"\n",
                        (curmode > 0) ? "maintenance" : "production");
            }
            ism_admin_setReturnCodeAndStringJSON(output_buffer, rc, tmpbuf);
            return;
        }

        if ( modeChanged ) {
            if ( curmode == 2 )  {
                rc = ISMRC_CleanStorePending;
            } else {
                rc = ISMRC_RunModeChangePending;
            }

            char buf[256];
            char *errstr = NULL;
            errstr = (char *)ism_common_getErrorStringByLocale(rc, ism_common_getRequestLocale(adminlocalekey), buf, 256);

            if ( errstr ){
                repl[0] = (modeChangedPrev > 0) ? "maintenance" : "production";
                if ( rc == ISMRC_CleanStorePending ) {
                    repl[1] = "clean_store";
                    repl[2] = "maintenance";
                    ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), errstr, repl, 3);
                } else {
                    repl[1] = (curmode > 0) ? "maintenance" : "production";
                     ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), errstr, repl, 2);
                }
                ism_admin_setReturnCodeAndStringJSON(output_buffer, rc, tmpbuf);
            } else {
                ism_admin_setReturnCodeAndStringJSON(output_buffer, rc, "Unknown");
            }
            return;
        }

        if (curmode == 2 ) {
            ism_admin_getMsgId(6123, msgID);
            if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey),
                    lbuf, sizeof(lbuf), &xlen) != NULL) {
                repl[0] = "maintenance";
                repl[1] = "clean_store";
                ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 2);
            } else {
                sprintf(tmpbuf, "The " IMA_SVR_COMPONENT_NAME_FULL " is in \"%s\" mode with \"%s\" with action pending.\n", "maintenance", "clean_store");
            }
        } else {
            ism_admin_getMsgId(6111, msgID);
            if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey),
                    lbuf, sizeof(lbuf), &xlen) != NULL) {
                repl[0] = (curmode > 0) ? "maintenance" : "production";
                ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
            } else {
                sprintf(tmpbuf, "The " IMA_SVR_COMPONENT_NAME_FULL " is in \"%s\" mode.\n", (curmode>0)?"maintenance":"production");
            }
        }

        ism_admin_setReturnCodeAndStringJSON(output_buffer, 0,(const char *) &tmpbuf);
        return;
    }

    mode = atoi(modeStr);
    if (mode == 0){
        TRACE(3, "Set Appliance runmode to \"production\".\n");
    }else{
        TRACE(3, "Set Appliance runmode to \"%s\".\n", (mode>1)?"clean_store pending":"maintenance");
    }

    /* modeChangedPrev =-1, modeChanged=0, mode=2, curmode=1 */
    /* curmode=2 storeNotAvailableError=0 adminInitError=0 adminModeRC=0 */

    if ( mode == 2 ) {

        if ( curmode == 0 && adminModeRC != 0 ) {
            rc = ISMRC_RunModeChangePending;
            TRACE(3, "Set Appliance runmode to \"clean_store\" failed. The current mode is \"%s\" with change flag %d. adminModeRC:%d\n",(curmode > 0) ? "maintenance" : "production", modeChanged, adminModeRC);
            char nmsgID[12];
            char nbuf[1024];
            int nlen;
            ism_admin_getMsgId(6135, nmsgID);
            if (ism_common_getMessageByLocale(nmsgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &nlen) != NULL) {
                repl[0] = 0;
                ism_common_formatMessage(nbuf, sizeof(nbuf), lbuf, repl, 1);
            } else {
                sprintf(nbuf,
                    "Cannot run clean_store.\nDue to an internal error, the server status is set to \"Maintenance\",\nhowever the configured mode is \"Production\".\nUse the \"imaserver status\" command for more information about the server status.\n");
            }
            ism_admin_setReturnCodeAndStringJSON(output_buffer, rc,(const char *) &nbuf);
            return;
        }
        if ( curmode == 0 && adminModeRC == 0 ) {
            rc = ISMRC_RunModeChangePending;
            TRACE(3, "Set Appliance runmode to \"clean_store\" failed. The current mode is \"%s\" with change flag %d. adminModeRC:%d\n",(curmode > 0) ? "maintenance" : "production", modeChanged, adminModeRC);
            char nmsgID[12];
            char nbuf[1024];
            int nlen;
            ism_admin_getMsgId(6122, nmsgID);
            if (ism_common_getMessageByLocale(nmsgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &nlen) != NULL) {
                repl[0] = 0;
                ism_common_formatMessage(nbuf, sizeof(nbuf), lbuf, repl, 1);
            } else {
                sprintf(nbuf,
                   "Cannot run clean_store.\nThe " IMA_SVR_COMPONENT_NAME_FULL " is not running in \"Maintenance\" mode.\nUse the \"imaserver runmode\" command to set run mode.\n");
            }
            ism_admin_setReturnCodeAndStringJSON(output_buffer, rc,(const char *) &nbuf);
            return;
        }
        if ( curmode != 2 && modeChanged ) {
            rc = ISMRC_RunModeChangePending;
            TRACE(3, "Set Appliance runmode to \"clean_store\" failed. The current mode is \"%s\" with change flag %d\n",(curmode > 0) ? "maintenance" : "production", modeChanged);
            char nmsgID[12];
            char nbuf[1024];
            int nlen;
            ism_admin_getMsgId(6134, nmsgID);
            if (ism_common_getMessageByLocale(nmsgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &nlen) != NULL) {
                repl[0] = 0;
                ism_common_formatMessage(nbuf, sizeof(nbuf), lbuf, repl, 1);
            } else {
                sprintf(nbuf,
                    "Cannot run clean_store.\nThe " IMA_SVR_COMPONENT_NAME_FULL " restart is required to change to \"Maintenance\" mode.\nUse the \"imaserver stop\" followed by \"imaserver start\" command to restart the server.\n");
            }
            ism_admin_setReturnCodeAndStringJSON(output_buffer, rc,(const char *) &nbuf);
            return;
        }
        //Set the clean to one so it can be processed
        __sync_val_compare_and_swap( &cleanStore, cleanStore, 1);
        TRACE(2, "The cleanStore variable is set to 1. The store will be cleaned on next stop.\n");
    }
    if ( mode == 3 ) {
        //Set the backup to disk
        __sync_val_compare_and_swap( &cleanStore, cleanStore, 2);
        TRACE(2, "The cleanStore variable is set to 2. The store will be backed up on next stop.\n");
    }

    if ( (mode != curmode ) && mode <= 1 ) {
        rc = ism_admin_setAdminMode(mode, 0);
    }
    if ( rc == ISMRC_OK ) {
        int xlen;
        char msgID[12];
        char tmpbuf[1024];

        /*Set mode is dirty or changed*/
        /* For defect 34787*/
        TRACE(3, "modeChangedPrev =%d, modeChanged=%d, mode=%d, curmode=%d.\n", modeChangedPrev, modeChanged, mode, curmode);
        switch (modeChangedPrev) {
        case -1:
        {
            //modeChangedPrev == -1 only after imaserver init.
            modeFirst = curmode;
            if ( mode != curmode) {
                modeChanged = 1;
                modeChangedPrev=curmode;
                modeChangedValue = mode;
                if ( mode == 2 ) {
                    ism_admin_getMsgId(6125, msgID);
                    if (ism_common_getMessageByLocale(msgID,ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                        repl[0] = "maintenance";
                        repl[1] = "clean_store";
                        repl[2] = "maintenance";
                        ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 3);
                    } else {
                        sprintf(tmpbuf,
                            "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode with \"%s\" action pending. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                            "maintenance", "clean_store", "maintenance");
                    }
                } else {
                    ism_admin_getMsgId(6124, msgID);
                    if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                           repl[0] = (curmode > 0) ? "maintenance" : "production";
                           repl[1] = (mode > 0 ) ? "maintenance" : "production";
                        ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 2);
                    } else {
                        sprintf(tmpbuf,
                            "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                            (curmode > 0) ? "maintenance" : "production", (mode > 0) ? "maintenance" : "production");
                    }
                }
                rc = 0;
            } else {
                TRACE(5, "mode \"%d\" matches the current mode.\n", mode);
                if ( mode == 2 ) {
                    ism_admin_getMsgId(6125, msgID);
                    if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                        repl[0] = "maintenance";
                        repl[1] = "clean_store";
                        repl[2] = "maintenance";
                        ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 3);
                    } else {
                        sprintf(tmpbuf,
                            "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode with \"%s\" action pending. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                            "maintenance", "clean_store", "maintenance");
                    }
                } else {
                    ism_admin_getMsgId(6124, msgID);
                    if (ism_common_getMessageByLocale(msgID,ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                           repl[0] = (mode > 0) ? "maintenance" : "production";
                           repl[1] = (mode > 0) ? "maintenance" : "production";
                        ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 2);
                    } else {
                        sprintf(tmpbuf,
                            "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                            (mode > 0) ? "maintenance" : "production", (mode > 0) ? "maintenance" : "production");
                    }
                }
                rc = 0;
            }
            break;
        }
        case 0:
        case 1:
        case 2:
        {
            if ( mode == curmode ) {
                if (modeChanged) {
                    if ( mode == 2 ) {
                        ism_admin_getMsgId(6125, msgID);
                        if (ism_common_getMessageByLocale(msgID,ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                            repl[0] = "maintenance";
                            repl[1] = "clean_store";
                            repl[2] = "maintenance";
                            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 3);
                        } else {
                            sprintf(tmpbuf,
                                "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode with \"%s\" action pending. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                                "maintenance", "clean_store", "maintenance");
                        }
                    } else {
                        ism_admin_getMsgId(6124, msgID);
                        if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                               repl[0] = (modeChangedPrev > 0) ? "maintenance" : "production";
                               repl[1] = (mode > 0) ? "maintenance" : "production";
                            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 2);
                        } else {
                            sprintf(tmpbuf,
                                "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                                (modeChangedPrev > 0) ? "maintenance" : "production", (mode > 0) ? "maintenance" : "production");
                        }
                    }
                    rc = 0;
                    break;

                } else {
                    if ( mode == 2 ) {
                        ism_admin_getMsgId(6125, msgID);
                        if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                            repl[0] = "maintenance";
                            repl[1] = "clean_store";
                            repl[2] = "maintenance";
                            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 3);
                        } else {
                            sprintf(tmpbuf,
                                "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode with \"%s\" action pending. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                                "maintenance", "clean_store", "maintenance");
                        }
                    } else {
                        ism_admin_getMsgId(6124, msgID);
                        if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                               repl[0] = (curmode > 0) ? "maintenance" : "production";
                               repl[1] = (mode > 0 ) ? "maintenance" : "production";
                            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 2);
                        } else {
                            sprintf(tmpbuf,
                                "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                                (curmode > 0) ? "maintenance" : "production", (mode > 0) ? "maintenance" : "production");
                        }
                    }
                    break;
                }
            } else {
                if (curmode == 2) {
                    //Set the clean to zero so it won't be processed
                    __sync_val_compare_and_swap( &cleanStore, cleanStore, 0);
                    TRACE(2, "The cleanStore variable is reset. The store won't be cleaned on the next start.\n");
                }
                /*
                 * If the latest mode is the same as the initial one, set everything back
                 */
                if ( mode == modeFirst ) {
                    modeChanged = 0;
                    modeChangedPrev = -1;
                    modeChangedValue = 0;
                    sprintf(tmpbuf, "The " IMA_SVR_COMPONENT_NAME_FULL " is in \"%s\" mode.\n", (mode>0)?"maintenance":"production");
                    rc = 0;
                    break;
                }
                if ( mode != modeChangedPrev) {
                    modeChanged = 1;
                    modeChangedPrev = curmode;
                    modeChangedValue = mode;
                    if ( mode == 2 ) {
                        ism_admin_getMsgId(6125, msgID);
                        if (ism_common_getMessageByLocale(msgID,ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                            repl[0] = "maintenance";
                            repl[1] = "clean_store";
                            repl[2] = "maintenance";
                            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 3);
                        } else {
                            sprintf(tmpbuf,
                                "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode with \"%s\" action pending.\nWhen it is restarted, it will be in \"%s\" mode.\n",
                                "maintenance", "clean_store", "maintenance");
                        }
                    } else {
                        ism_admin_getMsgId(6124, msgID);
                        if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                               repl[0] = (curmode > 0) ? "maintenance" : "production";
                               repl[1] = (mode > 0 ) ? "maintenance" : "production";
                            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 2);
                        } else {
                            sprintf(tmpbuf,
                                "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                                (curmode > 0) ? "maintenance" : "production", (mode > 0) ? "maintenance" : "production");
                        }
                    }
                    rc = 0;
                } else {
                    modeChangedPrev = curmode;
                    modeChangedValue = mode;
                    TRACE(5, "mode \"%d\" match the previous mode.\n", mode);
                    if (modeChanged) {
                        if ( mode == 2 ) {
                            ism_admin_getMsgId(6125, msgID);
                            if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                                repl[0] = "maintenance";
                                repl[1] = "clean_store";
                                repl[2] = "maintenance";
                                ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 3);
                            } else {
                                sprintf(tmpbuf,
                                    "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode with \"%s\" action pending.\nWhen it is restarted, it will be in \"%s\" mode.\n",
                                    "maintenance", "clean_store", "maintenance");
                            }
                        } else {
                            ism_admin_getMsgId(6124, msgID);
                            if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                                   repl[0] = (curmode > 0) ? "maintenance" : "production";
                                   repl[1] = (mode > 0 ) ? "maintenance" : "production";
                                ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 2);
                            } else {
                                sprintf(tmpbuf,
                                    "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                                    (curmode > 0) ? "maintenance" : "production", (mode > 0) ? "maintenance" : "production");
                            }
                        }
                    } else {
                        if ( mode == 2 ) {
                            ism_admin_getMsgId(6125, msgID);
                            if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                                repl[0] = "maintenance";
                                repl[1] = "clean_store";
                                repl[2] = "maintenance";
                                ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 3);
                            } else {
                                sprintf(tmpbuf,
                                    "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode with \"%s\" action pending.\nWhen it is restarted, it will be in \"%s\" mode.\n",
                                    "maintenance", "clean_store", "maintenance");
                            }
                        } else {
                            ism_admin_getMsgId(6124, msgID);
                            if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
                                   repl[0] = (curmode > 0) ? "maintenance" : "production";
                                   repl[1] = (mode > 0 ) ? "maintenance" : "production";
                                ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 2);
                            } else {
                                sprintf(tmpbuf,
                                    "The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode. \nWhen it is restarted, it will be in \"%s\" mode.\n",
                                    (curmode > 0) ? "maintenance" : "production", (mode > 0) ? "maintenance" : "production");
                            }
                        }
                    }
                    rc = 0;
                }
            }
            break;
        }
        default:
            TRACE(5, "previoud mode \"%d\" is not supported.\n", modeChangedPrev);
            rc = ISMRC_NotFound;
            sprintf(tmpbuf, "The " IMA_SVR_COMPONENT_NAME_FULL " \"runmode\" \"%d\" is not supported.\n", modeChangedPrev);
            break;
        }
        ism_admin_setReturnCodeAndStringJSON(output_buffer, rc,(const char *) &tmpbuf);

    } else {
        char buf[256];
        char *errstr = NULL;
        errstr = (char *)ism_common_getErrorStringByLocale(rc,  ism_common_getRequestLocale(adminlocalekey), buf, 256);
        if ( errstr ) {
            ism_admin_setReturnCodeAndStringJSON(output_buffer, rc,(const char *)errstr);
        }
        else
            ism_admin_setReturnCodeAndStringJSON(output_buffer, rc,"Unknown");
    }
}


/*
 * Create REST API return message based on return messageID in http outbuf.
 *
 * @param  http          ism_http_t object
 * @param  retcode       Message ID for the message.
 * @param  repl          replacement string array
 * @param  replSize      replacement string array size.
 *
 */
void ism_confg_rest_createReturnMsg(ism_http_t * http, int retcode, const char **repl, int replSize) {
    int xlen;
    char  msgID[12];
    char  rbuf[4096];
    char  lbuf[4096];
    char *locale = NULL;

    http->outbuf.used = 0;

    if (http->locale && *(http->locale) != '\0') {
        locale = http->locale;
    } else {
        locale = "en_US";
    }

    ism_admin_getMsgId(retcode, msgID);
    if ( ism_common_getMessageByLocale(msgID, locale, lbuf, sizeof(lbuf), &xlen) != NULL ) {
        ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, replSize);
    } else {
        rbuf[0]='\0';
    }
    ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
    ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
    ism_json_putBytes(&http->outbuf, "\",\"Code\":\"");
    ism_json_putEscapeBytes(&http->outbuf, msgID, (int) strlen(msgID));
    ism_json_putBytes(&http->outbuf, "\",\"Message\":\"");
    ism_json_putEscapeBytes(&http->outbuf, rbuf, (int) strlen(rbuf));
    ism_json_putBytes(&http->outbuf, "\" }\n");

    return;
}

/*******
 * Dummy functions for cunit tests
 */
static int checkConnection(ism_transport_t * transport, const char * clientID, const char * userID, const char * client_addr, const char * endpoint) {
    if (transport->name[0] == '_' && transport->name[1] == '_')
        return 0;
    if (clientID) {
        if (!ism_common_match(transport->name, clientID)) {
            return 0;
        }
    }
    if (userID) {
        if (transport->userid == NULL) {
            if (*userID)
                return 0;
        } else {
            if (!ism_common_match(transport->userid, userID))
                return 0;
        }
    }
    if (client_addr && transport->client_addr) {
        if (!ism_common_match(transport->client_addr, client_addr))
            return 0;
    }
    if (endpoint) {
        if (!ism_common_match(transport->endpoint_name, endpoint))
            return 0;
    }
    return 1;
}

static int ism_transport_closeConnection_dummy(const char * clientID, const char * userID, const char * client_addr, const char * endpoint) {
    int count = 0;
    ism_transport_t *transport = (ism_transport_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,522),1, sizeof(ism_transport_t));
    transport->name = "ClientA";
    transport->userid = "UserA";
    transport->client_addr = "9.3.179.167";

    if (checkConnection(transport, clientID, userID, client_addr, endpoint)) {
        printf("Force disconnect:client=%s client_addr=%s user=%s\n", transport->name, transport->client_addr, transport->userid);
        count++;
    } else {
        printf("Could not find a matching connection\n");
    }

    ism_common_free(ism_memory_admin_misc,(void *)transport);

    return count;
}

/**
 * End dummy functions for cunit tests
 *****************
 */



/* Admin function to invoke close connection API */
int ism_admin_closeConnection(ism_http_t *http) {
    int32_t rc = ISMRC_OK;
    int count;

    int noErrorTrace = 0;
    json_t *post = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);
    if ( !post || rc != ISMRC_OK) {
        return rc;
    }

    char *cID = NULL;
    char *cAddr = NULL;
    char *uID = NULL;
    int foundItem = 0;

    json_t *objval = NULL;
    const char *objkey = NULL;
    json_object_foreach(post, objkey, objval) {
        foundItem += 1;
        if ( json_typeof(objval) == JSON_OBJECT ) {
            rc = ISMRC_InvalidObjectConfig;
            ism_common_setErrorData(rc, "%s", "close/connection");
            break;
        }

        if ( !strcmp(objkey, "Version")) continue;

        if ( !strcmp(objkey, "ClientID")) {
            if ( json_typeof(objval) == JSON_STRING ) {
                char *tmpstr = (char *)json_string_value(objval);
                if ( tmpstr && *tmpstr != '\0' ) cID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);

            }
        } else if ( !strcmp(objkey, "ClientAddress")) {
            if ( json_typeof(objval) == JSON_STRING ) {
                char *tmpstr = (char *)json_string_value(objval);
                if ( tmpstr && *tmpstr != '\0' ) cAddr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);
            }
        } else if ( !strcmp(objkey, "UserID")) {
            if ( json_typeof(objval) == JSON_STRING ) {
                char *tmpstr = (char *)json_string_value(objval);
                if ( tmpstr ) uID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);
            }
        } else {
            rc = ISMRC_ArgNotValid;
            ism_common_setErrorData(rc, "%s", objkey?objkey:"");
            break;
        }
    }

    json_decref(post);

    if ( foundItem == 0 ) {
        rc = ISMRC_InvalidObjectConfig;
        ism_common_setErrorData(rc, "%s", "close/connection");
    }

    if ( rc != ISMRC_OK ) {
        if ( cID ) ism_common_free(ism_memory_admin_misc,cID);
        if ( cAddr ) ism_common_free(ism_memory_admin_misc, cAddr );
        if ( uID ) ism_common_free(ism_memory_admin_misc, uID );
        return rc;
    }

    if ( (!cID) && (!cAddr) && (!uID) ) {
        rc = 6204;
        ism_common_setError(rc);
        return rc;
    }

    TRACE(5, "Close connection: ClientID:%s ClientAddress:%s UserID:%s\n", cID?cID:"", cAddr?cAddr:"", uID?uID:"");

    const char *endpoint = NULL;

    if ( ismCUNITEnv == 0 ) {
        /* get ism_transport_closeConnection(0 function pointer set in main.c */
        closeConnection = (closeConnection_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_closeConnection_fnptr", 0L);
        if ( closeConnection ) {
            count = closeConnection(cID, uID, cAddr, endpoint);
        } else {
            rc = ISMRC_NotImplemented;
        }
    } else {
        count = ism_transport_closeConnection_dummy(cID, uID, cAddr, endpoint);
    }

    if ( rc == ISMRC_OK ) {
        const char * repl[1];
        int replSize = 0;
		if (count == 0) {
		    rc = 6136;
		    ism_common_setError(rc);
		} else {

            char buf[32];
            repl[0] = ism_common_itoa(count, buf);
            replSize = 1;
		    ism_confg_rest_createReturnMsg(http, 6137, repl, replSize );

		}
    } else {
        ism_common_setError(rc);
    }

    return rc;
}


int ism_admin_applyPSKFile(const char *PSKFile)
{
	int rc = ISMRC_OK;

	/*Check if file exists*/
	struct stat   buffer;
	int filestat =  stat (PSKFile, &buffer);

	if (filestat>=0)
	{
		rc = ism_ssl_applyPSKfile(PSKFile, 0);
		TRACE(2, "Apply PSK File Status: %d\n", rc);
	}else{
		TRACE(2,"PSK file does not exist. Skip applying PSK file.");
	}
	return rc;
}

/* No longer used - call path of http websocket code */
int ism_admin_processPSKNotification(ism_json_parse_t *json, concat_alloc_t *output_buffer)
{
	int rc=ISMRC_OK;

	const char *defaultStr = "";
	char *notificationType = (char *)ism_json_getString(json, "NotificationType");
	if(notificationType!=NULL && strcasecmp(notificationType,"apply")==0){
		rc = ism_admin_applyPSKFile(defaultStr);
	}

	char errstr [512];
	memset(&errstr,0, sizeof(errstr));
	ism_common_getErrorString(rc, errstr, sizeof(errstr));
	ism_admin_setReturnCodeAndStringJSON(output_buffer, rc, errstr);

	return rc;

}

/******************************************************************************
 *
 *    File command implementation
 *
 ******************************************************************************
 */

#define USER_MAX_FILE_PATH 1024

/* Delete a file from userfiles dir */
static void ism_admin_internal_FileDelete(char *fileName, concat_alloc_t *output_buffer)
{
    int rc = ISMRC_OK;
    char outbuf[2048];

    char filePath[USER_MAX_FILE_PATH];
    int fileNameLenLimit = USER_MAX_FILE_PATH - strlen(USERFILES_DIR) - 1;

    if ( !fileName || *fileName == '\0' || strlen(fileName) >= fileNameLenLimit )
    {
        TRACE(3, "Invalid file name or fileName (%s) length exceeded limit (%d)\n", fileName?fileName:"", fileNameLenLimit);
        rc = ISMRC_NullPointer;
        sprintf(outbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Error deleting %s, No such file or directory\" }", rc, fileName?fileName:"");
        goto FUNC_END;
    }

    if ( strstr(fileName, "..") )
    {
        TRACE(3, "Illegal file name %s\n", fileName?fileName:"");
        rc = ISMRC_BadPropertyValue;
        sprintf(outbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Illegal file name\" }", rc);
        goto FUNC_END;
    }

    snprintf(filePath, USER_MAX_FILE_PATH, "%s/%s", USERFILES_DIR, fileName);

    char linkPath[USER_MAX_FILE_PATH];

    ssize_t len = readlink(filePath, linkPath, sizeof(linkPath));
    if (len >= 0)
    {
       linkPath[len] = '\0';
       if (unlink(linkPath) < 0)
       {
           TRACE(3, "Could not delete file %s. errno: %d\n", linkPath, errno);
           rc = ISMRC_Error;
           sprintf(outbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Error deleting %s, No such file or directory\" }", rc, fileName?fileName:"");
           goto FUNC_END;
       }
    }

    if (unlink(filePath) < 0)
    {
        TRACE(3, "Could not delete file %s. errno: %d\n", filePath, errno);
        rc = ISMRC_Error;
        sprintf(outbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Error deleting %s, No such file or directory\" }", rc, fileName?fileName:"");
    }

FUNC_END:

    if ( rc == ISMRC_OK )
    {
        sprintf(outbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"\" }", rc);
    } else {
        /* reset rc to ISMRC_OK - no need to return error as output buffer is already updated with correct rc */
        rc = ISMRC_OK;
    }

    /* Update output_buffer */
    ism_common_allocBufferCopyLen(output_buffer, outbuf, strlen(outbuf));

    return;
}

/* List files in userfiles dir */
static void ism_admin_internal_FileList(concat_alloc_t *output_buffer)
{
    int rc = ISMRC_OK;
    char outbuf[2048];

    DIR *dp = opendir(USERFILES_DIR);
    if (dp == 0)
    {
        TRACE(3, "No such file or directory: %s\n", USERFILES_DIR);
        rc = ISMRC_NotFound;
        sprintf(outbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Unable to read file list, No such file or directory\" }", rc);
        goto FUNC_END;
    }

    struct dirent *ep;
    int found = 0;
    while ((ep = readdir(dp)) != NULL)
    {
        struct stat sb;
        char buff[USER_MAX_FILE_PATH];
        snprintf(buff, sizeof(buff), "%s/%s", USERFILES_DIR, ep->d_name);
        if (stat(buff, &sb) < 0 || !S_ISREG(sb.st_mode))
            continue;

        char datestr[32];
        strftime(datestr, 32, "%b %d, %Y %I:%M:%S %p", gmtime(&(sb.st_mtime)));

        char filedata[2048];
        snprintf( filedata, sizeof(filedata), "%s %ld bytes created %s\n", ep->d_name, sb.st_size, datestr);

        ism_common_allocBufferCopyLen(output_buffer, filedata, strlen(filedata));
        found = 1;
    }
    closedir(dp);
    if ( found == 0 ) {
        char buf[512];
        sprintf(buf, IMA_SVR_DATA_DIR "/userfiles directory is empty.\n");
        ism_common_allocBufferCopyLen(output_buffer, buf, strlen(buf));
    }

FUNC_END:

    return;
}

/* Invoke a script - used for command like file */
static int32_t ism_admin_msshellFileCommand(ism_json_parse_t *parseobj, concat_alloc_t *output_buffer)
{
    int rc = ISMRC_OK;

    char *stype = (char *)ism_json_getString(parseobj, "ScriptType");
    char *action = (char *)ism_json_getString(parseobj, "Command");
    char *arg1 = (char *)ism_json_getString(parseobj, "Arg1");
    char *arg2 = (char *)ism_json_getString(parseobj, "Arg2");
    char *arg3 = (char *)ism_json_getString(parseobj, "Password");


    TRACE(5, "Invoke MSSHELL script type %s to run %s command\n", stype, action?action:"");

    /* file list command process */
    if ( action && !strcmpi(action, "list")) {
        ism_admin_internal_FileList(output_buffer);
        return rc;
    }

    /* file delete command process */
    if ( action && !strcmpi(action, "delete")) {
        ism_admin_internal_FileDelete(arg1, output_buffer);
        return rc;
    }

    char scrname[1024];
    sprintf(scrname, IMA_SVR_INSTALL_PATH "/bin/msshell_%s.sh", stype);

    int pipefd[2];
    if ( pipe(pipefd) == -1 ) {
    	TRACE(2, "pipe returned error\n");
    	return 1;
    }

    pid_t pid = vfork();
    if (pid < 0) {
        TRACE(1, "Could not vfork process to run script\n");
        return 1;
    }
    if (pid == 0){
    	dup2(pipefd[1], STDOUT_FILENO);
    	close(pipefd[0]);
    	close(pipefd[1]);
        execl(scrname, stype, action, arg1?arg1:"", arg2?arg2:"", arg3?arg3:"", NULL);
        int urc = errno;
        TRACE(1, "Unable to run %s: errno=%d error=%s\n", stype?stype:"null", urc, strerror(urc));
        _exit(1);
    }

    close(pipefd[1]);
    char buffer[4096];
    int size = read(pipefd[0], buffer, sizeof(buffer));
    close(pipefd[0]);

    int st;
    waitpid(pid, &st, 0);
    rc = WIFEXITED(st)? WEXITSTATUS(st):1;

    if ( size == 0 && rc == ISMRC_OK ) {
        /* On success current CLI command on appliance returns empty string */
    	sprintf(buffer, "%s", "");
    }

    ism_admin_setReturnCodeAndStringJSON(output_buffer, rc, buffer);

    return rc;
}


static int32_t ism_admin_msshellApplyCommand(ism_json_parse_t *parseobj, concat_alloc_t *output_buffer) {
	int rc = ISMRC_OK;

    char *stype = (char *)ism_json_getString(parseobj, "ScriptType");
    char *action = (char *)ism_json_getString(parseobj, "Command");
    char *arg1 = (char *)ism_json_getString(parseobj, "Arg1");
    char *arg2 = (char *)ism_json_getString(parseobj, "Arg2");
    char *arg3 = (char *)ism_json_getString(parseobj, "Arg3");
    char *arg4 = (char *)ism_json_getString(parseobj, "Arg4");
    char *arg5 = (char *)ism_json_getString(parseobj, "Arg5");

    char scrname[1024];
    sprintf(scrname, IMA_SVR_INSTALL_PATH "/bin/msshell_%s.sh", stype);

    int pipefd[2];
    if ( pipe(pipefd) == -1 ) {
    	TRACE(2, "pipe returned error\n");
    	return 1;
    }

    pid_t pid = vfork();
    if (pid < 0) {
        TRACE(1, "Could not vfork process to run script\n");
        return 1;
    }
    if (pid == 0){
    	dup2(pipefd[1], STDOUT_FILENO);
    	close(pipefd[0]);
    	close(pipefd[1]);

    	if ( *action == 'S' ) {
    		/* args for Server cert - "apply", "Server", overwrite, name, keyname, keypwd, certpwd */
            execl(scrname, stype, stype, action, arg1, arg2, arg3, arg4, arg5, NULL);
    	} else if (*action == 'M') {
    		/* args for MQ cert - "apply", "MQ", overwrite, mqsslkey, mqstashpwd */
    		execl(scrname, stype, stype, action, arg1, arg2, arg3, NULL);
    	} else if (*action == 'T') {
    		/* args for Trusted cert - "apply", "TRUSTED", overwrite, secprofile, trustedcert */
    		execl(scrname, stype, stype, action, arg1, arg2, arg3, NULL);
    	} else {
    		/* args for LDAP cert - "apply", "LDAP", overwrite, ldapcert */
    		execl(scrname, stype, stype, action, arg1, arg2, NULL);
    	}

    	int urc = errno;
    	TRACE(1, "Unable to run %s: errno=%d error=%s\n", stype?stype:"null", urc, strerror(urc));
        _exit(1);
    }

    close(pipefd[1]);
    char buffer[4096];
    int size = read(pipefd[0], buffer, sizeof(buffer));
    close(pipefd[0]);

    int st;
    waitpid(pid, &st, 0);
    rc = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    if ( size == 0 && rc == ISMRC_OK ) {
    	sprintf(buffer, "The command completed successfully.");
    }

    ism_admin_setReturnCodeAndStringJSON(output_buffer, rc, buffer);

    return rc;
}

static int32_t ism_admin_msshellPlatformCommand(ism_json_parse_t *parseobj, concat_alloc_t *output_buffer) {
	int rc = ISMRC_OK;

    char *stype = (char *)ism_json_getString(parseobj, "ScriptType");
    char *action = (char *)ism_json_getString(parseobj, "Command");
    char *arg1 = (char *)ism_json_getString(parseobj, "Arg1");

    char scrname[1024];
    sprintf(scrname, IMA_SVR_INSTALL_PATH "/bin/msshell_%s.sh", stype);

    int pipefd[2];
    if ( pipe(pipefd) == -1 ) {
    	TRACE(2, "pipe returned error\n");
    	return 1;
    }

    pid_t pid = vfork();
    if (pid < 0) {
        TRACE(1, "Could not vfork process to run script\n");
        return 1;
    }
    if (pid == 0){
    	dup2(pipefd[1], STDOUT_FILENO);
    	close(pipefd[0]);
    	close(pipefd[1]);

    	if ( strcmpi(action, "must-gather") == 0 ) {
    		/* Run must-gather */
            execl(scrname, stype, stype, action, arg1, NULL);
        	int urc = errno;
        	TRACE(1, "Unable to run %s: errno=%d error=%s\n", stype?stype:"null", urc, strerror(urc));
    	}
        _exit(1);
    }

    close(pipefd[1]);
    char buffer[4096];
    int size = read(pipefd[0], buffer, sizeof(buffer));
    close(pipefd[0]);

    int st;
    waitpid(pid, &st, 0);
    rc = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    if ( size == 0 && rc == ISMRC_OK ) {
        /* On success current CLI command on appliance returns empty string */
    	sprintf(buffer, "%s", " \n");
    }

    ism_admin_setReturnCodeAndStringJSON(output_buffer, rc, buffer);

    return rc;
}


static int32_t ism_admin_msshellShowCommand(ism_json_parse_t *parseobj, concat_alloc_t *output_buffer) {
	int rc = ISMRC_OK;

    char *stype = (char *)ism_json_getString(parseobj, "ScriptType");
    char *action = (char *)ism_json_getString(parseobj, "Command");
    char *arg1 = (char *)ism_json_getString(parseobj, "Arg1");

    char scrname[1024];
    sprintf(scrname, IMA_SVR_INSTALL_PATH "/bin/msshell_%s.sh", stype);

    int pipefd[2];
    if ( pipe(pipefd) == -1 ) {
    	TRACE(2, "pipe returned error\n");
    	return 1;
    }

    pid_t pid = vfork();
    if (pid < 0) {
        TRACE(1, "Could not vfork process to run script\n");
        return 1;
    }
    if (pid == 0){
    	dup2(pipefd[1], STDOUT_FILENO);
    	close(pipefd[0]);
    	close(pipefd[1]);

    	if ( strcmpi(stype, "show") == 0 ) {
    		/* Run must-gather */
            execl(scrname, stype, stype, action, arg1, NULL);
           	int urc = errno;
            TRACE(1, "Unable to run %s: errno=%d error=%s\n", stype?stype:"null", urc, strerror(urc));
    	}
        _exit(1);
    }

    close(pipefd[1]);
    char buffer[4096];
    int size = read(pipefd[0], buffer, sizeof(buffer));
    close(pipefd[0]);

    int st;
    waitpid(pid, &st, 0);
    rc = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    if ( size == 0 && rc == ISMRC_OK ) {
    	sprintf(buffer, "The command completed successfully.");
    }

    ism_admin_setReturnCodeAndStringJSON(output_buffer, rc, buffer);

    return rc;
}

/* Invoke a script - used for command like file */
int32_t ism_admin_runMsshellScript(ism_json_parse_t *parseobj, concat_alloc_t *output_buffer)
{
    int rc = ISMRC_OK;

    char *stype = (char *)ism_json_getString(parseobj, "ScriptType");
    char *command = (char *)ism_json_getString(parseobj, "Command");

    if ( !stype || *stype == '\0' || !command || *command == '\0'  ) {
    	TRACE(2, "Invalid MSSHELL type (%s) or command (%s).\n", stype?stype:"", command?command:"");
    	return ISMRC_NullPointer;
    }

    if ( !strcmpi(stype, "file")) {
    	rc = ism_admin_msshellFileCommand(parseobj, output_buffer);
    } else if ( !strcmpi(stype, "apply")) {
    	rc = ism_admin_msshellApplyCommand(parseobj, output_buffer);
    } else if ( !strcmpi(stype, "platform")) {
    	rc = ism_admin_msshellPlatformCommand(parseobj, output_buffer);
    } else if ( !strcmpi(stype, "show")) {
    	rc = ism_admin_msshellShowCommand(parseobj, output_buffer);
    } else {
    	rc = ISMRC_NotImplemented;
    }

    return rc;
}

/* Get the callback list from schema for the specified object
 *
 * @param  type         The schema type
 * @param  name         The configuration item name
 * @param  callbacks    The pointer to return callback list
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * callbackList string has to be freed by caller
 *
 * */

int ism_config_getCallbacks(int type, char *itemName, char **callbacks) {
    int rc = ISMRC_OK;
    ism_json_parse_t * json = NULL;
    char *callbackList = NULL;

    TRACE(9, "Entry %s: type: %d, name: %s\n", __FUNCTION__, type, itemName?itemName:"null" );

    /* sanity check */
    if ( !itemName || *itemName == '\0' )
	{
		rc = ISMRC_NullPointer;
		ism_common_setError(ISMRC_NullPointer);
		goto GET_CALLBACKS_END;
	}

    /*
	 * ism_admin_setAdminMode will create a JSON action string with item "AdminMode"
	 * or "InternalErrorCode" which do not exist in the schema.
	 */
    if ( !strcmpi(itemName, "AdminMode") || !strcmpi(itemName, "InternalErrorCode") ) {
		*callbacks = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"Server");
		goto GET_CALLBACKS_END;
    }

    if (type != ISM_CONFIG_SCHEMA && type != ISM_MONITORING_SCHEMA) {
    	rc = ISMRC_BadPropertyType;
    	TRACE(3, "%s: The schema type: %d is invalid.\n", __FUNCTION__, type);
    	goto GET_CALLBACKS_END;
    }
	if (type == ISM_CONFIG_SCHEMA) {
		if ( isConfigSchemaLoad == 0) {
			ConfigSchema = ism_admin_getSchemaObject(type);
			isConfigSchemaLoad = 1;
	    }
		json = ConfigSchema;
	} else {
		if ( isMonitorSchemaLoad == 0) {
		    MonitorSchema = ism_admin_getSchemaObject(type);
		    isMonitorSchemaLoad = 1;
		}
		json = MonitorSchema;
	}

    /* Find position of the object in JSON object */
    int pos = ism_json_get(json, 0, itemName);
    if ( pos == -1 )
    {
        ism_common_setErrorData(ISMRC_BadPropertyName, "%s", itemName);
        rc = ISMRC_BadPropertyName;
        goto GET_CALLBACKS_END;
    }

    callbackList = ism_admin_getAttr( json, pos, "Callbacks" );
    if (!callbackList || *callbackList == '\0') {
    	rc = ISMRC_NotFound;
    	TRACE(3, "%s: The Callbacks for item: %s cannot be found.\n", __FUNCTION__, itemName);
    }else {
    	*callbacks = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),callbackList);
    }

GET_CALLBACKS_END:
    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

/* Get the component from schema for the specified object
 *
 * @param  type         The schema type
 * @param  name         The configuration item name
 * @param  component    The pointer to return component name
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * component string has to be freed by caller
 *
 * */

int ism_config_getComponent(int type, char *itemName, char **component, ism_objecttype_e *objtype) {
    int rc = ISMRC_OK;
    ism_json_parse_t * json = NULL;
    char *compName = NULL;
    char *objectType = NULL;

    TRACE(9, "Entry %s: type: %d, name: %s\n", __FUNCTION__, type, itemName?itemName:"null" );

    /* sanity check */
    if ( !itemName || *itemName == '\0' )
	{
		rc = ISMRC_NullPointer;
		// ism_common_setError(ISMRC_NullPointer);
		goto GET_COMPONENT_END;
	}

    /*
	 * ism_admin_setAdminMode will create a JSON action string with item "AdminMode"
	 * or "InternalErrorCode" which do not exist in the schema.
	 */
    if ( !strcmp(itemName, "AdminMode") || !strcmp(itemName, "InternalErrorCode")
    		|| !strcmp(itemName, "TraceBackup") || !strcmp(itemName, "TraceBackupCount")
    		|| !strcmp(itemName, "TraceBackupDestination")
    		|| !strcmp(itemName, "TraceModuleLocation")
    		|| !strcmp(itemName, "TraceModuleOptions")) {
		*component = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"Server");
		if (objtype) {
			*objtype = ISM_SINGLETON_OBJTYPE;
		}
		goto GET_COMPONENT_END;
    }

	// TrustedCertificate or ClientCertificate is not a configuration object in schema
	// but it requires POST/DELETE action to put certificates
	// into trustedstore or delete them. To Keep defined http payload
	// format, just fake its component and object type
    if ( !strcmp(itemName, "TrustedCertificate") || !strcmp(itemName, "ClientCertificate") ) {
		*component = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"Transport");
		if (objtype) {
		    *objtype = ISM_COMPOSITE_OBJTYPE;
		}
		goto GET_COMPONENT_END;
    }

    if (type != ISM_CONFIG_SCHEMA && type != ISM_MONITORING_SCHEMA) {
    	rc = ISMRC_BadPropertyType;
    	TRACE(3, "%s: The schema type: %d is invalid.\n", __FUNCTION__, type);
    	goto GET_COMPONENT_END;
    }
	if (type == ISM_CONFIG_SCHEMA) {
		if ( isConfigSchemaLoad == 0) {
			ConfigSchema = ism_admin_getSchemaObject(type);
			isConfigSchemaLoad = 1;
	    }
		json = ConfigSchema;
	} else {
		if ( isMonitorSchemaLoad == 0) {
		    MonitorSchema = ism_admin_getSchemaObject(type);
		    isMonitorSchemaLoad = 1;
		}
		json = MonitorSchema;
	}

    /* Find position of the object in JSON object */
	int pos = 0;
    pos = ism_json_get(json, 0, itemName);
    if ( pos == -1 )
    {
        ism_common_setErrorData(ISMRC_BadPropertyName, "%s", itemName);
        rc = ISMRC_BadPropertyName;
        goto GET_COMPONENT_END;
    }

    compName = ism_admin_getAttr( json, pos, "Component" );
    if (!compName || *compName == '\0') {
    	rc = ISMRC_NotFound;
    	TRACE(3, "%s: The component for item: %s cannot be found.\n", __FUNCTION__, itemName);
    }else {
    	*component = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),compName);
    }

    if (objtype) {
		objectType = ism_admin_getAttr( json, pos, "ObjectType" );
		if (!objectType || *objectType == '\0') {
			rc = ISMRC_NotFound;
			TRACE(3, "%s: The ObjectType for item: %s cannot be found.\n", __FUNCTION__, itemName);
		}else {
			if (!strcmpi(objectType, "Singleton"))
				*objtype = ISM_SINGLETON_OBJTYPE;
			else if (!strcmpi(objectType, "composite"))
				*objtype = ISM_COMPOSITE_OBJTYPE;
			else {
				*objtype = ISM_INVALID_OBJTYPE;
				TRACE(3, "%s: The ObjectType: %s for item: %s is not supported.\n", __FUNCTION__, objectType, itemName);
			}
		}
    }

GET_COMPONENT_END:
    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

char * ism_admin_getAttr(ism_json_parse_t *json, int pos, char *name) {
    int jPos = ism_json_get(json, pos, name);
    if ( jPos != -1 ) {
        ism_json_entry_t * tent = json->ent + jPos;
        return (char *)tent->value;
    }
    return NULL;
}

/* Returns monitoring or configuration schema JSON formated string */
XAPI char * ism_admin_getSchemaJSONString(int type) {
    int i, size;
    char *buf, *tmpbuf;

    if ( type == ISM_CONFIG_SCHEMA ) {
        size = sizeof(serverConfigSchema_json);
        buf = (char *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,527),1, (size + 1));
        tmpbuf = buf;

        for ( i = 0; i < size; i++) {
            char c = serverConfigSchema_json[i];
            *tmpbuf = c;
            tmpbuf++;
        }
    } else if ( type == ISM_MONITORING_SCHEMA ) {
        size = sizeof(monitoringSchema_json);
        buf = (char *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,528),1, (size + 1));
        tmpbuf = buf;

        for ( i = 0; i < size; i++) {
            char c = monitoringSchema_json[i];
            *tmpbuf = c;
            tmpbuf++;
        }
    } else {
        ism_common_setError(ISMRC_InvalidComponent);
        TRACE(2, "Unsupported schema type: %d\n", type);
        return NULL;
    }

    return buf;
}

/* Returns monitoring or configuration schema JSON object */
XAPI ism_json_parse_t * ism_admin_getSchemaObject(int type) {

    char *buf = ism_admin_getSchemaJSONString(type);

    if ( !buf ) return NULL;

    /* create config schema json object */
    ism_json_parse_t *parseobj = NULL;
    int buflen = 0;
    char svchar;

    parseobj = (ism_json_parse_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,529),1, sizeof(ism_json_parse_t));
    memset(parseobj, 0, sizeof(ism_json_parse_t));

    /* Parse policy buffer and create rule definition */
    buflen = strlen(buf);
    svchar = buf[buflen];
    ((char *)buf)[buflen] = 0;
    ((char *)buf)[buflen] = svchar;

    parseobj->src_len = buflen;
    parseobj->source = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,530),buflen+1);
    memcpy(parseobj->source, buf, buflen);
    parseobj->source[buflen] = 0;   /* null terminate for debug */
    ism_json_parse(parseobj);
    ism_common_free(ism_memory_admin_misc,buf);

    if (parseobj->rc) {

        ism_common_setError(ISMRC_NullPointer);
        if(parseobj->source)
            ism_common_free(ism_memory_admin_misc,parseobj->source);
        ism_common_free(ism_memory_admin_misc,parseobj);
        return NULL;
    }

    return parseobj;
}

int32_t ism_admin_ApplyCertificate(char *action, char *arg1, char *arg2, char *arg3, char *arg4, char *arg5) {
	int rc = ISMRC_OK;
    char scrname[1024];
    char *stype = "apply";
    sprintf(scrname, IMA_SVR_INSTALL_PATH "/bin/certApply.sh");

    pid_t pid = vfork();
    if (pid < 0) {
        TRACE(1, "Could not vfork process to run script\n");
        return 1;
    }
    if (pid == 0){
    	if ( *action == 'S' ) {
    		/* args for Server cert - "apply", "Server", overwrite, name, keyname, keypwd, certpwd */
    		TRACE(5, "%s:REST API apply: %s %s %s %s %s %s %s %s\n", __FUNCTION__, scrname, stype, action, arg1, arg2, arg3, arg4, arg5);
            execl(scrname, stype, stype, action, arg1, arg2, arg3, arg4, arg5, NULL);
    	} else if (*action == 'M') {
    		/* args for MQ cert - "apply", "MQ", overwrite, mqsslkey, mqstashpwd */
    		execl(scrname, stype, stype, action, arg1, arg2, arg3, NULL);
    	} else if (*action == 'T') {
    		/* args for Trusted cert - "apply", "TRUSTED", overwrite, secprofile, trustedcert */
    		execl(scrname, stype, stype, action, arg1, arg2, arg3, NULL);
    	} else if (*action == 'C') {
            /* args for Client cert - "apply", "CLIENT", overwrite, secprofile, certname */
            execl(scrname, stype, stype, action, arg1, arg2, arg3, NULL);
    	} else if (*action == 'L') {
    		/* args for LDAP cert - "apply", "LDAP", overwrite, ldapcert */
    		execl(scrname, stype, stype, action, arg1, arg2, NULL);
    	} else if (*action == 'R') {
    		/* args for CRL cert - "apply", "REVOCATION", overwrite, secprofile, crlprofname */
    		if (!strcmp(arg1, "apply"))
    			execl(scrname, stype, stype, action, arg2, arg3, arg4, arg5, NULL);
    		/* args for CRL cert - "staging", "REVOCATION", overwrite, crlprofname, crlcertname */
    		else
    			execl(scrname, arg1, arg1, action, arg2, arg3, arg4, NULL);
        }

    	int urc = errno;
    	TRACE(1, "Unable to run %s: errno=%d error=%s\n", stype?stype:"null", urc, strerror(urc));
        _exit(1);
    }

    int st;
    waitpid(pid, &st, 0);
    rc = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    return rc;
}

/*
 * setup cunit env
 */
XAPI void ism_admin_setCunitEnv(void) {
	ismCUNITEnv = 1;
}

/*
 * Get cunit env value
 */
XAPI int ism_admin_getCunitEnv(void) {
	return ismCUNITEnv;
}

/**
 * Create error response - for Angel
 */
int ism_admin_createErrorResponse(concat_alloc_t *output_buffer, int rc, const char * returnString) {

    char tmpbuf[1024];
    char msgID[12];
    ism_admin_getMsgId(rc, msgID);

    sprintf(tmpbuf, "{ \"Version\":\"v1\", \"Code\":\"%s\", \"Message\":", msgID);

    ism_common_allocBufferCopyLen(output_buffer, tmpbuf, strlen(tmpbuf));
    /*JSON encoding for the Error String.*/
    ism_json_putString(output_buffer, returnString);

    ism_common_allocBufferCopyLen(output_buffer, "}", 1);

    return ISMRC_OK;

}

/**
 * Test various objects
 */
XAPI int ism_admin_testObject(const char *objtype, json_t *objval, concat_alloc_t *output_buffer) {
    int rc = ISMRC_OK;

    if ( !objtype || !objval )
        return ISMRC_NullPointer;

    if ( !strcmp(objtype, "QueueManagerConnection")) {
    	const char *pInputQMName = NULL;
    	const char *pConnectionName = NULL;
    	const char *pChannelName = NULL;
    	const char *pSSLCipherSpec = NULL;
    	const char *puserName = NULL;
    	const char *puserPassword = NULL;

    	json_t *obj = json_object_get(objval, "QueueManagerName");
    	if ( obj && json_typeof(obj) == JSON_STRING ) pInputQMName = json_string_value(obj);

    	obj = json_object_get(objval, "ConnectionName");
    	if ( obj && json_typeof(obj) == JSON_STRING ) pConnectionName = json_string_value(obj);

    	obj = json_object_get(objval, "ChannelName");
    	if ( obj && json_typeof(obj) == JSON_STRING ) pChannelName = json_string_value(obj);

    	obj = json_object_get(objval, "SSLCipherSpec");
    	if ( obj && json_typeof(obj) == JSON_STRING ) pSSLCipherSpec = json_string_value(obj);

    	obj = json_object_get(objval, "ChannelUserName");
    	if ( obj && json_typeof(obj) == JSON_STRING ) puserName = json_string_value(obj);

    	obj = json_object_get(objval, "ChannelUserPassword");
    	if ( obj && json_typeof(obj) == JSON_STRING ) puserPassword = json_string_value(obj);

        /* for MQ connection test - get mqc_testQueueManagerConnection() function pointer, this is set in main.c of mqcbridge process */
        qmConnectionTest = (qmConnectionTest_f)(uintptr_t)ism_common_getLongConfig("_TEST_QueueManagerConnection_fnptr", 0L);
        if ( qmConnectionTest ) {
            rc = qmConnectionTest(pInputQMName, pConnectionName, pChannelName, pSSLCipherSpec, puserName, puserPassword);
        } else {
            rc = ISMRC_NotImplemented;
        }

#if 0
        /*Set the response*/
        if ( rc != ISMRC_OK ) {
            char buf[256];
            char *errstr = NULL;
            errstr = (char *)ism_common_getErrorStringByLocale(rc,  ism_common_getRequestLocale(adminlocalekey),  buf, 256);
            ism_admin_createErrorResponse(output_buffer, rc, errstr);
        } else {
            /*Get Test Connection success message and return.*/
            int xlen;
            char  rbuf[1024];
            char  lbuf[1024];
            const char * repl[5];

            char  msgID[12];
            ism_admin_getMsgId(6132, msgID);
            if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
                repl[0] = 0;
                 ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, 0);
            } else {
                sprintf(rbuf, "Test connection succeeded.");
            }

            ism_admin_createErrorResponse(output_buffer, rc, rbuf);
        }
#endif


    } else {
        rc = ISMRC_NotImplemented;
    }

    if ( rc != ISMRC_OK ) {
        ism_common_setError(rc);
    }

    return rc;
}
