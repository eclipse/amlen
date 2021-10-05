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
#define OBJECT_TYPE "ClientSet"

#include <validateConfigData.h>
#include "validateInternal.h"
#include "engine.h"
#include "transport.h"
#include <pthread.h>
#include <ismregex.h>
#include "janssonConfig.h"

extern pthread_key_t adminlocalekey;

extern int ism_config_json_parseConfig(ism_json_parse_t * parseobj, char *component, char *item, ism_http_t *http );

typedef int (*getClientStateMonitor_f)(ismEngine_ClientStateMonitor_t ** ppMonitor,
        uint32_t *                        pResultCount,
        ismEngineMonitorType_t            type,
        uint32_t                          maxResults,
        ism_prop_t *                      pFilterProperties);
typedef int32_t (*destroyDisconnectedClientState_f)(
        const char *                   pClientId,
        void *                         pContext,
        size_t                         contextLength,
        ismEngine_CompletionCallback_t pCallbackFn);
typedef int32_t (*unsetRetainedMsgOnDest_f)(
        ismEngine_SessionHandle_t       hSession,
        ismDestinationType_t            destinationType,
        const char *                    pDestinationName,
        uint32_t                        options,
        uint64_t                        serverTime,
        ismEngine_TransactionHandle_t   hTran,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn);
typedef int32_t (*freeClientStateMonitor_f)(
        ismEngine_ClientStateMonitor_t * ppMonitor);
typedef int (*disableClientSet_f)(const char * regex_str, int rc);
typedef int (*enableClientSet_f)(const char * regex_str);
typedef int (*threadInit_f)(uint8_t isStoreCrit);
typedef int (*threadTerm_f)(uint8_t closeStoreStream);

static int32_t ism_config_updateTaskFile(const char * fileName);
static void deleteClientSetInList(ismAdmin_DeleteClientSetMonitor_t * cs);

extern const char *configDir;
extern pthread_mutex_t g_cfgfilelock;
const char *taskFile = "QueuedTask";

/*
 * Validate ClientSet
 */

XAPI int32_t ism_config_validate_ClientSet(ism_json_parse_t *json, char *component, char *item, int actionType, char *inpbuf, int inpbuflen, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    char *name = NULL;
    int chkMode = 0;
    ism_config_itemValidator_t * reqList = NULL;

    TRACE(9, "Entry %s: json: %p, component: %s, item: %s, action: %d, inpbuf: %s, inpbuflen: %d\n",
        __FUNCTION__, json?json:0, component?component:"null", item?item:"null", actionType, inpbuf?inpbuf:"null", inpbuflen);

    /* Sanity check */
    if ( strcmpi(component, "Server") || strcmpi(item, OBJECT_TYPE) ) {
        /* Using incorrect API to validate the object */
        ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", OBJECT_TYPE, component, item);
        rc = ISMRC_BadOptionValue;
        goto VALIDATION_END;
    }

    switch (actionType){
    case ISM_ADMIN_DELETE:
    case ISM_ADMIN_EXPORT:
    case ISM_ADMIN_IMPORT:
        break;
    default:
        // TODO New error for invalid action for this object type
        goto VALIDATION_END;
    }

    /* Get list of required items from schema */
    reqList = ism_config_validate_getRequiredItemList(ISM_CONFIG_SCHEMA, item, &rc);
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Variables set to validate special rules */
    int isGet = 0;

    /* Loop thru the configuration items in json object and validate the objects */
    int count = json->ent[0].count;
    int i = 0;

    /* General item validation against schema.
     * Any special cases
     */
    for (i = 0; i <= count; i++) {
        ism_json_entry_t * jent = json->ent + i;
        if ( !jent || jent->name == NULL )
          continue;

        /* get property name and value */
        char *propName = (char *)jent->name;
        char *propValue = (char *)jent->value;

        /* Invalid item is found in the json object */
        if ( !propName || *propName == '\0' ) {
            ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", OBJECT_TYPE, propName?propName:"NULL", propValue?propValue:"NULL");
            rc = ISMRC_BadOptionValue;
            goto VALIDATION_END;
        }

        if (!strcmp(propName, "Name")) propName = "ClientID";
        rc = ism_config_validate_checkItemDataType( reqList, propName, propValue, &name, &isGet, 0, props);
        if ( rc != ISMRC_OK ) {
            //already set ... ism_common_setError(rc);
            goto VALIDATION_END;
        }


    }

    /* Check if all required items are specified.
     * If item exist, then get the existing value of the configuration items,
     * merge the items with the required list and then check if any required
     * item is missing. For updates cases, it is not necessary to send the
     * complete list of items.
     */
    if (isGet && chkMode != 1) {
        chkMode = 1;
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, chkMode );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    ism_field_t field;
    switch (actionType) {
    case ISM_ADMIN_DELETE:

        // Don't allow File, Delete or Replace
        // Must have ClientID
        ism_common_getProperty(props, "ClientID", &field);
        if (VT_Null == field.type || VT_String != field.type || field.val.s[0] == 0) {
            rc = ISMRC_PropertyRequired;
            ism_common_setErrorData(rc, "%s%s", "ClientID", "null");
            goto VALIDATION_END;
        }
        rc = ism_common_getProperty(props, "File", &field);
        if (!rc) {
            ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", OBJECT_TYPE, "File", VT_Null == field.type ? "NULL" : field.val.s);
            rc = ISMRC_BadOptionValue;
        }
        rc = ism_common_getBooleanProperty(props, "Delete", -999);
        if (rc != -999) {
            ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", OBJECT_TYPE, "Delete", 0 == rc ? "false" : "true");
            rc = ISMRC_BadOptionValue;
            goto VALIDATION_END;
        }
        rc = ism_common_getBooleanProperty(props, "Replace", -999);
        if (rc != -999) {
            ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", OBJECT_TYPE, "Replace", 0 == rc ? "false" : "true");
            rc = ISMRC_BadOptionValue;
            goto VALIDATION_END;
        } else {
            rc = 0;
        }
        break;
    case ISM_ADMIN_EXPORT:
        // Don't allow Replace
        // Must have File, ClientID
        break;
    case ISM_ADMIN_IMPORT:
        // Don't allow ClientID, Retain or Delete
        // Must have File
        break;
    }



VALIDATION_END:

    ism_config_validate_freeRequiredItemList(reqList);
    if (name)
        ism_common_free(ism_memory_admin_misc,name);

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

static getClientStateMonitor_f             getClientStateMonitor = NULL;   ///> function pointers
static destroyDisconnectedClientState_f    destroyDisconnectedClientState = NULL;
static freeClientStateMonitor_f            freeClientStateMonitor = NULL;
static unsetRetainedMsgOnDest_f            unsetRetainedMsgOnDest = NULL;
static disableClientSet_f                  disableClientSet = NULL;
static enableClientSet_f                   enableClientSet = NULL;
static threadInit_f                        threadInit = NULL;
static threadTerm_f                        threadTerm = NULL;


//*********************************************************************
/// @brief  delete ClientSet status
///
/// Represents the monitoring information for a client-state.
//*********************************************************************
struct ismAdmin_DeleteClientSetMonitor_t
{
    pthread_mutex_t mutex;            ///< mutex lock
    pthread_cond_t  cond;             ///< pthread condition
    char*        clientID;            ///< regular expression for ClientIds
    char*        retain;              ///< regular expression for RETAINed messages topics
    uint32_t waitsec;                 ///< wait time in seconds
    uint32_t identifier;              ///< ID for this unit of work
    uint32_t resultCount;             ///< total clients found
    uint32_t completeCount;           ///< clients completed
    uint32_t asynchCount;             ///< number of clients completing asynchronously
    uint32_t errorCount;              ///< clients in error
    int32_t deleteRetainedRC;         ///< return code from deleting the RETAINed messages
    ismClientSetState_t status;       ///< current state of operation
    uint64_t doneTimestamp;
    ismAdmin_DeleteClientSetMonitor_t  *  prev;
    ismAdmin_DeleteClientSetMonitor_t  *  next;

};

void unsetRetainedCallback(int32_t retcode, void * handle, void * pContext) {
    ismAdmin_DeleteClientSetMonitor_t *work = (ismAdmin_DeleteClientSetMonitor_t *)pContext;
    pthread_mutex_lock(&work->mutex);
    work->status = ismCLIENTSET_DONE;
    // printf("Delete client set %s is set to done", work->clientID);
    TRACE(7, "Delete client set %s is set to done", work->clientID);
    work->doneTimestamp = ism_common_currentTimeNanos();
    work->deleteRetainedRC = retcode;
    pthread_cond_signal(&work->cond);
    pthread_mutex_unlock(&work->mutex);
}

/*
 * Free ismAdmin_DeleteClientSetMonitor_t object
 */
static void freeClientSet(ismAdmin_DeleteClientSetMonitor_t *cs) {
    if (!cs)
        return;

    if (cs->clientID)
        ism_common_free(ism_memory_admin_misc,cs->clientID);      ///< regular expression for ClientIds
    if (cs->retain)
        ism_common_free(ism_memory_admin_misc,cs->retain);        ///< regular expression for RETAINed messages topics
    cs->prev = NULL;
    cs->next = NULL;
    ism_common_free(ism_memory_admin_misc,cs);
}

/*
 * Deleting the clients is complete, need to maybe delete the RETAINed messages.
 * Enter holding the mutex, exit having released the mutex
 */
void ism_config_DeleteClientSetComplete(ismAdmin_DeleteClientSetMonitor_t *work) {
    if (work->retain) {
        TRACE(7, "Delete RETAINed messages on subcriptions matching: %s\n", work->retain);
        work->status = ismCLIENTSET_DELETINGMSGS;
        // printf("start retained cs=%s\n", work->clientID);
        pthread_mutex_unlock(&work->mutex);
        int rc = unsetRetainedMsgOnDest(
                NULL, ismDESTINATION_REGEX_TOPIC,
                work->retain,
                ismENGINE_UNSET_RETAINED_OPTION_NONE, // Should we PUBLISH for cluster?
                ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                NULL,
                work,
                sizeof(work),
                unsetRetainedCallback);
        if (rc != ISMRC_AsyncCompletion) {
            unsetRetainedCallback(rc, NULL, work);
        }
    } else {
        work->status = ismCLIENTSET_DONE;
        TRACE(7, "Set delete client set done: clientID=%s retain=%s", work->clientID, work->retain);
        pthread_cond_signal(&work->cond);
        pthread_mutex_unlock(&work->mutex);
    }
}

 void DeleteClientCallback(int32_t retcode, void * handle, void * pContext) {
    ismAdmin_DeleteClientSetMonitor_t *work = *(ismAdmin_DeleteClientSetMonitor_t **)pContext;
    // Grab lock on work object
    pthread_mutex_lock(&work->mutex);
    if (ISMRC_OK == retcode) {
        __sync_fetch_and_add(&work->completeCount, 1);
    } else {
        __sync_fetch_and_add(&work->errorCount, 1);
    }
    if (work->resultCount == work->completeCount + work->errorCount) {
        // Process the work complete.....
        work->status = ismCLIENTSET_CLIENTSCOMPLETE;
        ism_config_DeleteClientSetComplete(work);
    } else {
        pthread_mutex_unlock(&work->mutex);
    }
}

XAPI int32_t ism_config_DeleteClientSet(ismAdmin_DeleteClientSetMonitor_t* work)
{
    int rc;
    ism_prop_t *clientProps;
    ismEngine_ClientStateMonitor_t *pMonitor;
    uint32_t count;

    TRACE(6, "Beginning DeleteClientSet with ClientID=%s, Retain=%s\n", work->clientID, work->retain);

    rc = disableClientSet(work->clientID, ISMRC_EndpointDisabled);
    TRACE(7, "Clients disabled rc=%d\n", rc);
    if (ISMRC_OK != rc)
        goto DELETE_END;
    clientProps = ism_common_newProperties(16);
    ism_field_t filterField;
    filterField.type = VT_String;
    filterField.val.s = work->clientID;
    ism_common_setProperty(clientProps, ismENGINE_MONITOR_FILTER_REGEX_CLIENTID,
            &filterField);
    // By default, ism_engine_getClientStateMonitor will only return disconnected MQTT
    // or HTTP clients, if we want to include other protocols, we need to specify that.
    // "All" means everything except PROTOCOL_ID_SHARED.
    filterField.val.s = ismENGINE_MONITOR_FILTER_PROTOCOLID_ALL;
    ism_common_setProperty(clientProps, ismENGINE_MONITOR_FILTER_PROTOCOLID,
            &filterField);
    rc = getClientStateMonitor(&pMonitor, &count, ismENGINE_MONITOR_ALL_UNSORTED,
            0, clientProps);
    if (ISMRC_OK != rc) {
        TRACE(5, "getClientStateMonitor rc=%d\n", rc);
        goto DELETE_END;
    }
    TRACE(7, "getClientStateMonitor %s returned %d entries.\n", work->clientID, count);
    if (count) {
        int i;
        pthread_mutex_lock(&work->mutex);
        work->resultCount = count;
        work->completeCount = 0;
        work->errorCount = 0;
        work->status = ismCLIENTSET_HAVECLIENTLIST;
        pthread_mutex_unlock(&work->mutex);
        for (i=0; i<work->resultCount; i++) {
            /* Ask engine to destroy each client */
            if (pMonitor[i].fIsConnected) {
                // should not be connected
                // TODO what should we do here?
            }
            rc = destroyDisconnectedClientState(pMonitor[i].ClientId, &work, sizeof(work), DeleteClientCallback);
            if (ISMRC_AsyncCompletion == rc) {
                // Do asynch completion work
                TRACE(8, "Delete ClientID = %s completing asynchronously.\n", pMonitor[i].ClientId);
                work->asynchCount++;
            } else if (ISMRC_OK == rc ){
                TRACE(8, "Delete ClientID = %s completed.\n", pMonitor[i].ClientId);
                // Do synch completion work
                __sync_fetch_and_add(&work->completeCount, 1);
            } else {
                TRACE(8, "Delete ClientID = %s returned error %d.\n", pMonitor[i].ClientId, rc);
                __sync_fetch_and_add(&work->errorCount, 1);
            }
        }
    }

    // Check if we need to wait for asynch client deletes.....
    pthread_mutex_lock(&work->mutex);
    if (!count || (work->resultCount == work->completeCount + work->errorCount)) {
        // Work has completed.
        // Process the work complete.....
        work->status = ismCLIENTSET_CLIENTSCOMPLETE;
        ism_config_DeleteClientSetComplete(work);
    } else {
    }
    // Wait for full completion
    if (!(work->status == ismCLIENTSET_DONE)) {
        TRACE(7, "Waiting for ismCLIENTSET_DONE\n");
        for (;;) {
            pthread_cond_wait(&work->cond, &work->mutex);
            if (work->status == ismCLIENTSET_DONE)
                break;
        }
        pthread_mutex_unlock(&work->mutex);
    }
    ism_config_updateTaskFile(taskFile);
    // Delete this operation from pending queue (in config) and free the work block

DELETE_END:
    TRACE(7, "Re-enable clients matching %s\n", work->clientID);
    enableClientSet(work->clientID);
    if (pMonitor)
        freeClientStateMonitor(pMonitor);
    return 0;
}

typedef struct {
    uint64_t                                  list_count;        /**< Count of ClientSet list                       */
    ismAdmin_DeleteClientSetMonitor_t *       clientSet;         /**< A double link list holding the clientSet data */
    ismAdmin_DeleteClientSetMonitor_t *       tail;              /**< A point to the last node of ismAdmin_DeleteClientSetMonitor_t       */
    pthread_spinlock_t                        cslock;            /**< Lock over clientSet                           */
    int32_t                                   init;              /**< the list has been initialed flag              */
    ism_threadh_t                             handle;            /**< handle of the thread                          */
    ism_threadh_t                             handle2;           /**< Retry admin worker                            */
} ism_clientset_t ;

ism_clientset_t *requests = NULL;

static void * AdminWorker(void * parm, void * context, int value) {
    ism_clientset_t *queue = (ism_clientset_t *)parm;
    threadInit(0);        // Need to init engine on thread before calling engine

    TRACE(5, "Start AdminWorker thread for deleting ClientSet\n");
    while (1) {
        pthread_spin_lock(&(queue->cslock));
        ismAdmin_DeleteClientSetMonitor_t *current = queue->clientSet;
        while (current) {
            if (current->status == ismCLIENTSET_DONE &&
                current->doneTimestamp &&
                (current->doneTimestamp + (3600*1000000000L)) < ism_common_currentTimeNanos()) {
                ismAdmin_DeleteClientSetMonitor_t * next = current->next;
                current = next;
            } else {
                if (current->status == ismCLIENTSET_WAITING || current->status == ismCLIENTSET_RESTARTING) {
                    break;
                }
                current = current->next;
            }
        }

        if (current) {
            current->status = ismCLIENTSET_START;
            pthread_spin_unlock(&(queue->cslock));
            ism_config_DeleteClientSet(current);
        } else {
            queue->handle = 0;
            pthread_spin_unlock(&(queue->cslock));
            TRACE(5, "No more Delete ClientSet work, End AdminWorker thread.\n");
            threadTerm(1);
            return NULL;
        }
    }
    return NULL;
}

/*
 * Admin thread
 */
static void * AdminWorker2(void * parm, void * context, int value) {
    ism_clientset_t *queue = (ism_clientset_t *)parm;
    threadInit(0);        // Need to init engine on thread before calling engine

    TRACE(5, "Start AdminWorker2 thread for deleting ClientSet\n");
    while (1) {
        pthread_spin_lock(&(queue->cslock));
        ismAdmin_DeleteClientSetMonitor_t *current = queue->clientSet;
        while (current) {
           if (current->status == ismCLIENTSET_RESTARTING) {
               break;
           }
           current = current->next;
        }

        if (current) {
            current->status = ismCLIENTSET_START;
            pthread_spin_unlock(&(queue->cslock));
            ism_config_DeleteClientSet(current);
        } else {
            queue->handle2 = 0;
            pthread_spin_unlock(&(queue->cslock));
            TRACE(5, "No more Delete ClientSet work, End AdminWorker2 thread.\n");
            threadTerm(1);
            return NULL;
        }
    }
}

/*
 * One time initialization of external methods
 * @return A return code 0=good, 1=bad
 */
static int initClientSet(void) {
    static int inited = 0;
    static int isbad  = 1;
    if (!inited) {
        /* Get all the pointers we need: */
        if (!getClientStateMonitor)
            getClientStateMonitor = (getClientStateMonitor_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_getClientStateMonitor_fnptr", 0L);
        if (!destroyDisconnectedClientState)
            destroyDisconnectedClientState = (destroyDisconnectedClientState_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_destroyDisconnectedClientState_fnptr", 0L);
        if (!freeClientStateMonitor)
            freeClientStateMonitor = (freeClientStateMonitor_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_freeClientStateMonitor_fnptr", 0L);
        if (!unsetRetainedMsgOnDest)
            unsetRetainedMsgOnDest = (unsetRetainedMsgOnDest_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_unsetRetainedMsgOnDest_fnptr", 0L);
        if (!disableClientSet)
            disableClientSet = (disableClientSet_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_disableClientSet_fnptr", 0L);
        if (!enableClientSet)
            enableClientSet = (enableClientSet_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_enableClientSet_fnptr", 0L);
        if (!threadInit)
            threadInit = (threadInit_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_threadInit_fnptr", 0L);
        if (!threadTerm)
            threadTerm = (threadTerm_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_threadTerm_fnptr", 0L);

        if (getClientStateMonitor && disableClientSet && enableClientSet && destroyDisconnectedClientState &&
            freeClientStateMonitor && unsetRetainedMsgOnDest && threadInit && threadTerm) {
            TRACE(1, "Unable to initialize client set methods\n");
            isbad = 0;
        }
        inited = true;
    }
    return isbad;
}


/*
 * Handle the ClientSet commands "delete"
 *
 * @param  json                   The pased json for the command
 * @param  validateData           Boolean whether to validate the data
 * @param  inpbuf                 Original input buffer
 * @param  inpbuflen              and length
 * @param  props                  The parsed properties
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int32_t ism_config_ClientSet(ism_json_parse_t *json, int validateData, char *inpbuf, int inpbuflen,
        ism_prop_t *props, int actionType, char **retbuf)
{
    int32_t rc = ISMRC_OK;
    char *clientSet = NULL;
    char *retain    = NULL;

    switch (actionType) {
    case ISM_ADMIN_DELETE:
        clientSet = (char *)ism_json_getString(json, "ClientID");
        retain = (char *)ism_json_getString(json, "Retain");

        if (!clientSet) {
            TRACE(3, "%s: ClientSet is NULL\n", __FUNCTION__);
            rc = ISMRC_PropertiesNotValid;
            ism_common_setError(ISMRC_PropertiesNotValid);
            goto CONFIG_END;
        }

        if (initClientSet()) {
            TRACE(3, "Cannot obtain needed functions\n");
            rc = ISMRC_NotImplemented;
            ism_common_setError(rc);
            goto CONFIG_END;
        }

        //add the clientset into the list. The API will initial the request list if not exist
        rc = ism_config_addClientSetToList(clientSet, retain);
        if (rc != ISMRC_OK && rc != ISMRC_ExistingKey ) {
            TRACE(3, "%s: Failed to add the clientset: %s, retain: %s into the request list.\n", __FUNCTION__, clientSet, retain?retain:"null");
            goto CONFIG_END;
        }

        if (rc == ISMRC_ExistingKey) {
            char buffer[256];
            int msgIDnum;
            ismAdmin_DeleteClientSetMonitor_t *cs = NULL;
            ismClientSetState_t status = ism_config_getClientSetStatus(clientSet, retain, &cs, 1);
            switch (status) {
            case ismCLIENTSET_DONE:
                msgIDnum = 6197;
                break;
            case ismCLIENTSET_WAITING:
            case ismCLIENTSET_RESTARTING:
            case ismCLIENTSET_START:
                msgIDnum = 6195;
                break;
            default:
                msgIDnum = 6196;
                break;
            }
            char lbuf[200];
            char  msgID[12];
            ism_admin_getMsgId(msgIDnum, msgID);
            if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf,
                    sizeof(lbuf), NULL) != NULL) {
                if (6195 == msgIDnum) {
                    *retbuf = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),lbuf);
                } else {
                    char repl0[20];
                    char repl1[20];
                    char repl2[20];
                    const char *repl[3];
                    repl[0] = ism_common_itoa(cs->resultCount, repl0);
                    repl[1] = ism_common_itoa(cs->completeCount, repl1);
                    repl[2] = ism_common_itoa(cs->errorCount, repl2);
                    ism_common_formatMessage(buffer, sizeof(buffer), lbuf, repl, 3);
                    *retbuf = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),buffer);
                }
            } else {
                switch (msgIDnum) {
                case 6195:
                    strcpy(buffer, "The request is pending");
                    break;
                case 6196:
                    sprintf(buffer,
                        "The request is being processed. Clients found: %i, Clients deleted: %i, Deletion errors: %i",
                        cs->resultCount, cs->completeCount, cs->errorCount);
                    break;
                case 6197:
                    sprintf(buffer,
                        "The request is complete. Clients found: %i, Clients deleted: %i, Deletion errors: %i",
                        cs->resultCount, cs->completeCount, cs->errorCount);
                    break;
                }
                *retbuf = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),buffer);
            }
            TRACE(5, "%s: ClientSet is already in the request list\n", __FUNCTION__);
            rc = ISMRC_OK;
            freeClientSet(cs);
            goto CONFIG_END;
        } else {
            // printf("New: cs=%s ret=%s\n", clientSet, retain);
            rc = ism_config_updateTaskFile(taskFile);
        }

        pthread_spin_lock(&(requests->cslock));
        if (!requests->handle) {
            rc = ism_common_startThread(&(requests->handle), AdminWorker, requests, NULL, 0,
                                ISM_TUSAGE_LOW, 0, "AdminWorker", NULL);
        }
        pthread_spin_unlock(&(requests->cslock));

        break;
    default:
        ism_common_setError(ISMRC_NotImplemented);
        rc = ISMRC_NotImplemented;
    }

CONFIG_END:
    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 * Structure for async check of delete client set completion
 */
typedef struct async_check_t {
    ism_http_t * http;
    ism_rest_api_cb callback;
    char * clientset;
    char * retain;
    uint32_t count;
    uint32_t maxcount;
    char  more[4];
} async_check_t;

/*
 * In a timer, check if the client set is done.
 * If it is done or we have waited long enough then return
 */
static int checkClientSet(ism_timer_t timer, ism_time_t timestamp, void * callbackParam) {
    int rc = 0;
    async_check_t * async = callbackParam;
    ismAdmin_DeleteClientSetMonitor_t *cs = NULL;
    ismClientSetState_t status = ism_config_getClientSetStatus(async->clientset, async->retain, &cs, 1);

    async->count++;
    switch (status) {
    case ismCLIENTSET_DONE:
        rc = 6197;
        ism_common_setErrorData(rc, "%d%d%d", cs->resultCount, cs->completeCount, cs->errorCount);
        break;
    case ismCLIENTSET_WAITING:
    case ismCLIENTSET_RESTARTING:
    case ismCLIENTSET_START:
        if (async->count >= async->maxcount)
            return 1;   /* Try again later */
        rc = 6195;
        ism_common_setError(rc);
        break;
    case ismCLIENTSET_NOTFOUND:
        rc = 0;
        ism_common_setError(rc);
        break;
    default:
        if (async->count < async->maxcount)
            return 1;
        rc = 6196;
        ism_common_setErrorData(rc, "%d%d%d", cs->resultCount, cs->completeCount, cs->errorCount);
        break;
    }
    if (timer)
        ism_common_cancelTimer(timer);
    if (cs)
        freeClientSet(cs);
    ism_confg_rest_createErrMsg(async->http, rc, NULL, 0);
    if (rc == 6195 || rc == 6196)
        rc = ISMRC_AsyncCompletion;
    if (rc == 6197)
        rc = 0;
    async->callback(async->http, rc);
    ism_common_free(ism_memory_admin_misc,async);
    return 0;
}

/*
 * Handle the ClientSet "delete"
 *
 * @param  clientID               regex to match ClientID to delete
 * @param  retain                 regex to match topic string for retained messages to delete
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int32_t ism_config_admin_deleteClientSet(ism_http_t *http, ism_rest_api_cb callback, const char * clientID, const char * retain, uint32_t waitsec)
{
    int32_t rc = ISMRC_OK;
    int ha_role = 0;
    int retry = 0;
    const char * clientSet = clientID;
    ismAdmin_DeleteClientSetMonitor_t * cs = NULL;

    // printf("clientID=%s retain=%s waitsec=%u\n", clientID, retain, waitsec);
    if (!clientSet) {
        TRACE(3, "%s: ClientSet is NULL\n", __FUNCTION__);
        rc = ISMRC_PropertiesNotValid;
        ism_common_setError(ISMRC_PropertiesNotValid);
        goto CONFIG_END;
    }

    /* Validate the regular expressions */

    /* Check for a valid regular expression */
    ism_regex_t regex;

    if (ism_regex_compile(&regex, clientID)) {
        TRACE(3, "%s: Error compiling regular expression \"%s\"\n", __FUNCTION__, clientID);
        rc = ISMRC_RegularExpression;
        ism_common_setErrorData(rc, clientID);
        goto CONFIG_END;
    } else {
        ism_regex_free(regex);
    }

    if (retain) {
        if (ism_regex_compile(&regex, retain)) {
            TRACE(3, "%s: Error compiling regular expression \"%s\"\n", __FUNCTION__, retain);
            rc = ISMRC_RegularExpression;
            ism_common_setErrorData(rc, retain);
            goto CONFIG_END;
        } else {
            ism_regex_free(regex);
        }
    }

    /* Only allow ClientSet deletes on Primary HA Node i.e after engine/store have been initialized */
    if ((ism_admin_getHArole(NULL, &ha_role) != ISM_HA_ROLE_PRIMARY) &&  (ism_admin_isHAEnabled() == 1)) {
        rc = ISMRC_NotAllowedOnStandby;
        ism_common_setErrorData(rc, "%s%s", "DELETE", "ClientSet");
        goto CONFIG_END;
    }
    if (initClientSet()) {
        rc = ISMRC_NotImplemented;
        ism_common_setError(rc);
        goto CONFIG_END;
    }

    if (serverState != ISM_SERVER_RUNNING) {
        rc = ISMRC_ServerStateProduction;
        ism_common_setError(rc);
        goto CONFIG_END;
    }

    /* add the clientset into the list. The API will initial the request list if not exist */
    rc = ism_config_addClientSetToList(clientSet, retain);
    if (rc != ISMRC_OK && rc != ISMRC_ExistingKey ) {
        TRACE(3, "%s: Failed to add the clientset: %s, retain: %s into the request list.\n", __FUNCTION__, clientSet, retain?retain:"null");
        goto CONFIG_END;
    }

    if (rc == ISMRC_ExistingKey) {
        cs = NULL;
        ismClientSetState_t status = ism_config_getClientSetStatus(clientSet, retain, &cs, 0);
        // printf("Existing: cs=%s ret=%s state=%u\n", clientSet, retain, status);

        switch (status) {
        case ismCLIENTSET_DONE:
            rc = 6197;
            ism_common_setErrorData(rc, "%d%d%d", cs->resultCount, cs->completeCount, cs->errorCount);
            ism_config_updateClientSetStatus(cs->clientID, cs->retain, ismCLIENTSET_RESTARTING);
            retry = 1;
            break;
        case ismCLIENTSET_WAITING:
        case ismCLIENTSET_RESTARTING:
        case ismCLIENTSET_START:
            rc = 6195;
            ism_common_setError(rc);
            goto CONFIG_END;
        case ismCLIENTSET_NOTFOUND:   /* Should not happen, but there is a small window */
            rc = 0;
            ism_common_setError(rc);
            goto CONFIG_END;
        default:
            rc = 6196;
            ism_common_setErrorData(rc, "%d%d%d", cs->resultCount, cs->completeCount, cs->errorCount);
            goto CONFIG_END;
        }
        TRACE(5, "%s: ClientSet %s is already in the request list\n", clientSet, __FUNCTION__);
    }
    rc = ism_config_updateTaskFile(taskFile);

    pthread_spin_lock(&(requests->cslock));
    if (retry) {
        if (!requests->handle2) {
            rc = ism_common_startThread(&(requests->handle2), AdminWorker2, requests, NULL, 1,
                            ISM_TUSAGE_LOW, 0, "AdminWorker2", NULL);
        }
    } else {
        if (!requests->handle) {
            rc = ism_common_startThread(&(requests->handle), AdminWorker, requests, NULL, 0,
                            ISM_TUSAGE_LOW, 0, "AdminWorker", NULL);
        }
    }
    pthread_spin_unlock(&(requests->cslock));

    async_check_t * async = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,405),1, sizeof(async_check_t) + strlen(clientSet) + (retain ? strlen(retain) : 0));
    async->http = http;
    async->callback = callback;
    async->clientset = (char *)(&async->more);
    async->maxcount = (waitsec > 86400 ? 86400 : waitsec) * 20;    /* We check every 50 ms, max of 1 day */
    strcpy(async->clientset, clientSet);
    if (retain) {
        async->retain = async->clientset + strlen(clientSet) + 1;
        strcpy(async->retain, retain);
    }
    ism_common_setTimerRate(ISM_TIMER_LOW, checkClientSet, async, 50, 50, TS_MILLISECONDS);
    callback = NULL;

CONFIG_END:
    if (cs)
        freeClientSet(cs);
    if (callback) {
        ism_confg_rest_createErrMsg(http, rc, NULL, 0);
        if (rc == 6195 || rc == 6196)
            rc = ISMRC_AsyncCompletion;
        if (rc == 6197)
            rc = 0;
        callback(http, rc);
    }
    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 * initial clientset_t
 */
static int initClientSetList(void) {
    int rc = ISMRC_OK;


    requests = (ism_clientset_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,406),1, sizeof(ism_clientset_t));
    if (!requests) {
        TRACE(2, "ClientSet List cannot be initialed\n");
        ism_common_setError(ISMRC_AllocateError);
        return ISMRC_AllocateError;
    }

    requests->init       = 1;
    pthread_spin_init(&requests->cslock, 0);

    TRACE(9, "%s: request list has been initialed.\n", __FUNCTION__);
    return rc;
}

/*
 * construct a new ismAdmin_DeleteClientSetMonitor_t object
 */
static ismAdmin_DeleteClientSetMonitor_t * newClientSet(void) {
    ismAdmin_DeleteClientSetMonitor_t *cs = (ismAdmin_DeleteClientSetMonitor_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,407),sizeof(ismAdmin_DeleteClientSetMonitor_t));
    if (!cs) {
        TRACE(3, "%s: memory allocation failed\n", __FUNCTION__);
        return NULL;
    }

    cs->clientID         = NULL;               ///< regular expression for ClientIds
    cs->retain           = NULL;               ///< regular expression for RETAINed messages topics
    cs->waitsec          = 0;
    cs->identifier       = 0;                  ///< ID for this unit of work
    cs->resultCount      = 0;                  ///< total clients found
    cs->completeCount    = 0;                  ///< clients completed
    cs->asynchCount      = 0;                  ///< number of clients completing asynchronously
    cs->errorCount       = 0;                  ///< clients in error
    cs->deleteRetainedRC = 0;                  ///< return code from deleting the RETAINed messages
    cs->status           = ismCLIENTSET_WAITING; ///< current state of operation
    cs->doneTimestamp    = 0;
    cs->prev             = NULL;
    cs->next             = NULL;

    return cs;
}

/*
 * duplicate ismAdmin_DeleteClientSetMonitor_t object
 */
static ismAdmin_DeleteClientSetMonitor_t * dupClientSet(ismAdmin_DeleteClientSetMonitor_t *cs) {
    if (!cs)
        return NULL;
    ismAdmin_DeleteClientSetMonitor_t * new = newClientSet();
    if (cs->clientID) new->clientID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),cs->clientID);      ///< regular expression for ClientIds
    if (cs->retain)   new->retain   = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),cs->retain);        ///< regular expression for RETAINed messages topics
    new->identifier       = cs->identifier;                      ///< ID for this unit of work
    new->resultCount      = cs->resultCount;                     ///< total clients found
    new->completeCount    = cs->completeCount;                   ///< clients completed
    new->asynchCount      = cs->asynchCount;                     ///< number of clients completing asynchronously
    new->errorCount       = cs->errorCount;                      ///< clients in error
    new->deleteRetainedRC = cs->deleteRetainedRC;                ///< return code from deleting the RETAINed messages
    new->status           = cs->status;                          ///< current state of operation
    new->doneTimestamp    = cs->doneTimestamp;

    return new;
}
/*
 * Check if the clientset is already in the list
 */
static int checkClientSetDup(const char *clientID, const char *retain) {
    int rc = ISMRC_OK;
    ismAdmin_DeleteClientSetMonitor_t * cs = NULL;

    if (!clientID)
        goto CHECK_END;

    pthread_spin_lock(&requests->cslock);
    cs = requests->clientSet;
    while(cs) {
        if (cs->clientID && !strcmp(cs->clientID, clientID)) {
            if (cs->retain ) {
                if (retain && !strcmp(cs->retain, retain)) {
                    rc = ISMRC_ExistingKey;
                    break;
                }
            } else {
                if (!retain) {
                    rc = ISMRC_ExistingKey;
                    break;

                }
            }
        }
        cs = cs->next;
    }
    pthread_spin_unlock(&requests->cslock);

CHECK_END:
    if (rc ) {
        TRACE(9, "Found duplicated entry: clientSet: %s, retain: %s\n", clientID?clientID:"null", retain?retain:"null");
    }
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);
    return rc;

}

/*
 *
 * Delete a clientset from the deleting list. The called has to
 * lock the list before calling this API.
 *
 * @param  cs              The ismAdmin_DeleteClientSetMonitor_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
static void deleteClientSetInList(ismAdmin_DeleteClientSetMonitor_t * cs) {

    // check clientID is not null
    if (!cs ) {
        ism_common_setError(ISMRC_ArgNotValid);
        TRACE(3, "%s: ClientSet is null. No delete is needed\n", __FUNCTION__);
        return;
    }

    TRACE(9, "%s: delete ClientSet: %s, retain: %s\n", __FUNCTION__, cs->clientID, cs->retain?cs->retain:"null");
    if (cs->prev) {
        cs->prev->next = cs->next;
        if (cs->next == NULL)
            requests->tail = cs->prev;
    } else {
        if (cs->next) {
            requests->clientSet = cs->next;
            cs->next->prev = NULL;
        } else {
            requests->clientSet = NULL;
            requests->tail = NULL;
        }
    }

    requests->list_count--;
    freeClientSet(cs);

    return;

}

/*
 *
 * Add a clientset into the deleting list.
 *
 * @param  clientID         The regular expression of clientID
 * @param  retain           The regular expression of retained message. Can be NULL.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_addClientSetToList(const char *clientID, const char *retain) {
    int rc = ISMRC_OK;
    ismAdmin_DeleteClientSetMonitor_t * cs = NULL;

    // check clientID is not null
    if (!clientID ) {
        rc = ISMRC_ArgNotValid;
        ism_common_setError(rc);
        goto ADD_CLIENT_END;
    }

    // init clientSet if not initialed
    if ( requests == NULL) {
        rc = initClientSetList();
        if (rc)
            goto ADD_CLIENT_END;
    }

    if (requests->clientSet != NULL) {
        rc = checkClientSetDup(clientID, retain);
        if (rc) {
            TRACE(5, "Delete ClientSet: %s, retain: %s has status: %d\n", clientID, retain?retain:"null", rc);
            //TODO: How to report the status through REST API?
            goto ADD_CLIENT_END;
        }
    }

    // create a new ClientSet
    cs = newClientSet();
    if (!cs) {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto ADD_CLIENT_END;
    }

    pthread_mutex_init(&cs->mutex, 0);
    pthread_cond_init(&cs->cond, 0);
    cs->clientID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),clientID);

    if (retain)
        cs->retain = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),retain);

    pthread_spin_lock(&requests->cslock);
    if (requests->clientSet) {
        cs->prev = requests->tail;
        requests->tail->next = cs;
        requests->tail = cs;
    } else {
        requests->clientSet = cs;
        requests->tail = cs;
    }
    requests->list_count++;

    pthread_spin_unlock(&requests->cslock);
    TRACE(9, "%s: ClientSet: %s, retain: %s has been added into the task list\n", __FUNCTION__, clientID, retain?retain:"null");

ADD_CLIENT_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);
    return rc;

}

/*
 *
 * Delete a clientset from the deleting list.
 *
 * @param  clientID         The regular expression of clientID
 * @param  retain           The regular expression of retained message. Can be NULL.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_deleteClientSetFromList(const char *clientID, const char *retain) {
    int rc = ISMRC_NotFound;
    ismAdmin_DeleteClientSetMonitor_t * cs = NULL;

    // check clientID is not null
    if (!clientID ) {
        rc = ISMRC_ArgNotValid;
        ism_common_setError(rc);
        goto DELETE_CLIENT_END;
    }

    // init clientSet if not initialized
    if ( requests == NULL || requests->clientSet == NULL) {
        goto DELETE_CLIENT_END;
    }

    pthread_spin_lock(&requests->cslock);

    cs = requests->clientSet;
    while(cs) {
        if (cs->clientID && !strcmp(cs->clientID, clientID)) {
            if (cs->retain ) {
                if (retain && !strcmp(cs->retain, retain)) {
                    if (cs->prev) {
                        cs->prev->next = cs->next;
                        if (cs->next == NULL)
                            requests->tail = cs->prev;
                    } else {
                        if (cs->next) {
                            requests->clientSet = cs->next;
                            //cs->next->prev = NULL;
                        } else {
                            requests->clientSet = NULL;
                            requests->tail = NULL;
                        }
                    }

                    requests->list_count--;
                    rc = ISMRC_OK;
                    break;
                }
            } else {
                if (!retain) {
                    if (cs->prev) {
                        cs->prev->next = cs->next;
                        if (cs->next == NULL)
                            requests->tail = cs->prev;
                    } else {
                        if (cs->next) {
                            requests->clientSet = cs->next;
                            cs->next->prev = NULL;
                        } else {
                            requests->clientSet = NULL;
                            requests->tail = NULL;
                        }
                    }
                    requests->list_count--;
                    rc = ISMRC_OK;
                    break;

                }
            }
        }
        cs = cs->next;
    }
    pthread_spin_unlock(&requests->cslock);

    if (rc == ISMRC_OK && cs) {
        TRACE(5, "Delete ClientSet from the list: clientSet: %s, retain: %s\n", clientID, retain?retain:"null");
        cs->prev = NULL;
        cs->next = NULL;
        freeClientSet(cs);
    }

DELETE_CLIENT_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);
    return rc;

}

/*
 *
 * Update the clientset status in the deleting list.
 *
 * @param  clientID         The regular expression of clientID
 * @param  retain           The regular expression of retained message. Can be NULL.
 * @param  status           The enum value of the clientset status
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_updateClientSetStatus(const char *clientID, const char *retain, ismClientSetState_t status) {
    int rc = ISMRC_NotFound;
    ismAdmin_DeleteClientSetMonitor_t * cs = NULL;

    if (!clientID)
        goto UPDATE_END;

    cs = requests->clientSet;
    while(cs) {
        if (cs->clientID && !strcmp(cs->clientID, clientID)) {
            if (cs->retain ) {
                if (retain && !strcmp(cs->retain, retain)) {
                    cs->status = status;
                    rc = ISMRC_OK;
                    break;
                }
            } else {
                if (!retain) {
                    cs->status = status;
                    rc = ISMRC_OK;
                    break;

                }
            }
        }
        cs = cs->next;
    }
    pthread_spin_unlock(&requests->cslock);

UPDATE_END:
    if (!rc ) {
        TRACE(5, "Entry: clientSet: %s, retain: %s status has been updated to %d\n", clientID?clientID:"null", retain?retain:"null", status);
    }
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);
    return rc;

}

/*
 *
 * Get the clientset status from the deleting list.
 *
 * @param  clientID         The regular expression of clientID
 * @param  retain           The regular expression of retained message. Can be NULL.
 *
 * Returns enum value of ismClientSetState.
 * */
ismClientSetState_t ism_config_getClientSetStatus(const char *clientID, const char *retain, ismAdmin_DeleteClientSetMonitor_t ** clientSet, int del) {
    ismAdmin_DeleteClientSetMonitor_t * cs = NULL;
    ismClientSetState_t status = ismCLIENTSET_NOTFOUND;

    if (!clientID)
        goto UPDATE_END;

    pthread_spin_lock(&requests->cslock);
    cs = requests->clientSet;
    while(cs) {
        if (cs->clientID && !strcmp(cs->clientID, clientID)) {
            if (cs->retain ) {
                if (retain && !strcmp(cs->retain, retain)) {
                    status = cs->status;
                    *clientSet = dupClientSet(cs);
                    if (del && cs->status == ismCLIENTSET_DONE)
                        deleteClientSetInList(cs);
                    break;
                }
            } else {
                if (!retain) {
                    status = cs->status;
                    *clientSet = dupClientSet(cs);
                    if (del && cs->status == ismCLIENTSET_DONE)
                        deleteClientSetInList(cs);
                    break;

                }
            }
        }
        cs = cs->next;
    }
    pthread_spin_unlock(&requests->cslock);

UPDATE_END:
    TRACE(9, "Exit %s: status: %d\n", __FUNCTION__, status);
    return status;

}

void ism_config_cleanClientSetList(void) {
    ismAdmin_DeleteClientSetMonitor_t * cs = NULL;

    if (!requests)
        return;

    pthread_spin_lock(&requests->cslock);
    cs = requests->clientSet;
    while(cs) {
        requests->clientSet = cs->next;
        if (cs->clientID ) ism_common_free(ism_memory_admin_misc,cs->clientID);
        if (cs->retain )   ism_common_free(ism_memory_admin_misc,cs->retain);
        cs->identifier       = 0;
        cs->resultCount      = 0;                  ///< total clients found
        cs->completeCount    = 0;                  ///< clients completed
        cs->asynchCount      = 0;                  ///< number of clients completing asynchronously
        cs->errorCount       = 0;                  ///< clients in error
        cs->deleteRetainedRC = 0;                  ///< return code from deleting the RETAINed messages
        cs->status           = ismCLIENTSET_START; ///< current state of operation
        cs->prev             = NULL;
        cs->next             = NULL;

        cs = requests->clientSet;
    }

    requests->init = 0;
    requests->list_count = 0;

    pthread_spin_unlock(&requests->cslock);
    pthread_spin_destroy(&requests->cslock);

    return;

}


/*
 * This is a CUNIT test only function.
 * We don't want to expose ismAdmin_DeleteClientSetMonitor_t
 */
int ism_validate_ClientSetStatus(const char *clientID, const char *retain, ismClientSetState_t status) {
    ismAdmin_DeleteClientSetMonitor_t *cs = NULL;
    ismClientSetState_t rtn = ism_config_getClientSetStatus(clientID, retain, &cs, 1);
    if (status == rtn)
        return 0;
    else
        return 1;
}

void ism_config_startClientSetTask(void) {
    int rc = ISMRC_OK;
    char cfilepath[1024];

    if ( initClientSet()) {
        return;
    }
    if ( requests == NULL) {
        rc = initClientSetList();
        if (rc)
            return;
    }

    if (serverState != ISM_SERVER_RUNNING) {
        rc = ISMRC_ServerStateProduction;
        ism_common_setError(rc);
        return;
    }
    sprintf(cfilepath, "%s/%s", configDir, "QueuedTask");
    json_t *jsonFile = ism_config_json_fileToObject(cfilepath);
    if (jsonFile) {
        json_t *clientSets = json_object_get(jsonFile, "ClientSetDelete");
        if (json_is_array(clientSets) && json_array_size(clientSets)) {
            /* Need to add the requests loaded from the file into the requests list */
            size_t count = json_array_size(clientSets);
            pthread_spin_lock(&(requests->cslock));
            for (int i=0; i<count; i++) {
                json_t *clientSet = json_array_get(clientSets, i);
                if (json_is_object(clientSet)) {
                    json_t *element = json_object_get(clientSet, "ClientID");
                    char *clientID = NULL;
                    if (json_is_string(element)) clientID = (char *)json_string_value(element);
                    element = json_object_get(clientSet, "Retain");
                    char *retain = NULL;
                    if (json_is_string(element)) retain = (char *)json_string_value(element);
                    if (clientID || retain) {
                        // create a new ClientSet
                        ismAdmin_DeleteClientSetMonitor_t *cs = newClientSet();
                        if (!cs) {
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);
                            pthread_spin_unlock(&(requests->cslock));
                            return;
                        }

                        pthread_mutex_init(&cs->mutex, 0);
                        pthread_cond_init(&cs->cond, 0);
                        if (clientID)
                            cs->clientID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),clientID);

                        if (retain)
                            cs->retain = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),retain);

                        if (requests->clientSet) {
                            cs->prev = requests->tail;
                            requests->tail->next = cs;
                            requests->tail = cs;
                        } else {
                            requests->clientSet = cs;
                            requests->tail = cs;
                        }
                        requests->list_count++;

                        TRACE(9, "%s: ClientSet: %s, retain: %s has been added into the task list\n", __FUNCTION__, clientID, retain?retain:"null");
                    }
                }
            }
            if (!requests->handle) {
                ism_common_startThread(&(requests->handle), AdminWorker, requests, NULL, 0,
                                    ISM_TUSAGE_LOW, 0, "AdminWorker", NULL);
            }
            pthread_spin_unlock(&(requests->cslock));
            TRACE(5, "%s: delete ClientSet tasks thread has been started.\n", __FUNCTION__);
        }
    }
    return;
}

static int32_t ism_config_dumpTaskConfig(const char * filepath) {
    int32_t rc = ISMRC_OK;
    FILE *dest = NULL;
    ismAdmin_DeleteClientSetMonitor_t * cs = NULL;

    /* open file to dump queued task
     *
     * { "ClientSetDelete":
     *   [
     *    {"ClientID":"xyz","Retain":"ABC", "Status":""},
     *    {"ClientID":"llin","Status":""},
     *    {"ClientID":"Marc","Retain":"Marc's_Message", "Status":""}
     *   ]
     * }
     * */
    dest = fopen(filepath, "w");
    if( dest == NULL ) {
        TRACE(2,"Can not open destination file '%s'. rc=%d\n", filepath, errno);
        ism_common_setError(ISMRC_Error);
        return ISMRC_Error;
    }

    int anything = 0;
    pthread_spin_lock(&(requests->cslock));
    if (requests->clientSet) {

        cs = requests->clientSet;
        while(cs) {
            int added = 0;
            if (ismCLIENTSET_DONE != cs->status) {
                if (0 == anything) {
                    fprintf(dest, "{ \"ClientSetDelete\":\n");
                    fprintf(dest, "   [\n");
                    anything = 1;
                }
                added = 1;
                fprintf(dest, "      {\"ClientID\":\"%s\"", cs->clientID);
                if (cs->retain)
                    fprintf(dest, ",\"Retain\":\"%s\"}", cs->retain);
                else
                    fprintf(dest, "}");
            }
            cs = cs->next;
            if (added) {
                if (cs)
                    fprintf(dest, ",\n");
                else
                    fprintf(dest, "\n");
            }
        }

        if (anything) {
            fprintf(dest, "   ]\n");
            fprintf(dest, "}\n");
        }
    }
    pthread_spin_unlock(&(requests->cslock));

    fclose(dest);
    if (anything == 0) {
        remove(filepath);
        rc = 1;    /* File not created */
    }
    TRACE(5, "Write ClientSet task file %s: rc=%d\n", filepath, rc);
    return rc;
}

static int32_t ism_config_updateTaskFile(const char * fileName) {
    int32_t rc = ISMRC_OK;
    char bfilepath[1024];
    char cfilepath[1024];
    char ofilepath[1024];
    char tfilepath[1024];
    int  err;

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

    if ( access(ofilepath, F_OK) == 0) {
        TRACE(5, "Remove original ClientSet task list %s.\n", ofilepath);
        remove(ofilepath);
    }

    int hasTask = 0;
    pthread_spin_lock(&(requests->cslock));
    if (requests->clientSet != NULL) {
         hasTask = 1;
    }
    pthread_spin_unlock(&(requests->cslock));

    if (hasTask) {
        /* dump content of current configuration to a .temp file */
        if ( (rc = ism_config_dumpTaskConfig(tfilepath)) == ISMRC_OK ) {
            if ( access(cfilepath, F_OK) == 0 ) {
                /* rename current to .bak */
                if ( rename(cfilepath, bfilepath) == 0 ) {
                    if ( rename( tfilepath, cfilepath ) != 0 ) {
                        err = errno;
                        TRACE(2, "Could not rename temporary ClientSet task file to current. rc=%s (%d)\n", strerror(err), err);
                        rc = ISMRC_Error;
                    } else {
                        char * filebuf = NULL;
                        int    filelen;
                        err = ism_common_readFile(cfilepath, &filebuf, &filelen);
                        if (err || !filebuf) {
                            TRACE(2, "Unable to read ClientSet task file %s: rc=%d\n", cfilepath, err);
                        } else {
                            TRACE(5, "Updated ClientSet task file %s:\n%s", cfilepath, filebuf);
                        }
                    }
                } else {
                    err = errno;
                    TRACE(2, "Could not rename current ClientSet task file to a backup file. rc=%s (%d)\n", strerror(err), err);
                    rc = ISMRC_Error;
                }
            } else {
                if ( rename( tfilepath, cfilepath ) != 0 ) {
                    err = errno;
                    TRACE(2, "Could not rename temporary ClientSet task file to current. rc=%s (%d)\n", strerror(err), err);
                    rc = ISMRC_Error;
                } else {
                    char * filebuf = NULL;
                    int    filelen;
                    err = ism_common_readFile(cfilepath, &filebuf, &filelen);
                    if (err || !filebuf) {
                        TRACE(2, "Unable to read ClientSet task file %s: rc=%d\n", cfilepath, err);
                    } else {
                        TRACE(5, "Updated ClientSet task file %s:\n%s", cfilepath, filebuf);
                    }
                }
            }
        } else {
            hasTask = 0;
            rc = ISMRC_OK;
        }
    }
    if (!hasTask) {
        rc = unlink(cfilepath);
        if (rc) {
            TRACE(2, "Could not delete current ClientSet task file %s. rc=%s (%d)\n", cfilepath, strerror(errno), errno);
        } else {
            TRACE(5, "Delete ClientSet task file: %s\n", cfilepath);
        }
    }
    if (rc)
        ism_common_setError(rc);

    pthread_mutex_unlock(&g_cfgfilelock);
    return rc;
}
