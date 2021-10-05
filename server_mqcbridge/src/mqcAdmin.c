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

#define MQC_GLOBAL_INIT
#include <mqcInternal.h>

extern int ism_server_config(char * object, char * namex, ism_prop_t * props, ism_ConfigChangeType_t flag);

ism_prop_t * getConfigProps(int comptype);

/* Function prototypes */
static int mqc_freeQM(mqcQM_t * pQM);
static int mqc_freeRule(mqcRule_t * pRule, bool decrementCounter);
static int mqc_freeRuleQM(mqcRuleQM_t * pRuleQM);

static int mqc_loadQM(ism_prop_t * props, const char * propqual,
        const char * name, const char * oldname, mqcQM_t ** ppQM);
static int mqc_loadRule(ism_prop_t * props, const char * propqual,
        const char * name, const char * oldname, mqcRule_t ** ppRule);

static int mqc_updateQM(ism_prop_t * props, const char * propqual,
        mqcQM_t * pQM, const char * name, bool rename, bool force);
static int mqc_updateRule(ism_prop_t * props, const char * propqual,
        mqcRule_t * pRule, const char * name, bool rename);

static int mqc_deleteQM(ism_prop_t * props, const char * propqual,
        const char * name, bool force);
static int mqc_deleteRule(ism_prop_t * props, const char * propqual,
        const char * name);

static int mqc_setQMIndex(mqcQM_t * pQM, bool rename);
static int mqc_setRuleIndex(mqcRule_t * pRule, bool rename);

/* Functions */
int mqc_loadRecords(void) {
    int i, count, exitrc, rc = 0;
    ismc_session_t * pSess = NULL;
    ismc_manrec_list_t * pManRecList = NULL;
    mqcManagerRecord_t * pRecord;
    mqcQM_t * pQM;
    mqcRule_t * pRule;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    /* Create session */
    pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, 0, 0);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession pSess <%p>\n", pSess);

    if (pSess == NULL) {
        rc = mqc_ISMCError("ismc_createSession");
        goto MOD_EXIT;
    }

    rc = ismc_getManagerRecords(pSess, &pManRecList, &count);
    TRACE(MQC_ISMC_TRACE, "ismc_getManagerRecords rc(%d) count(%d)\n", rc,
            count);

    if (rc) {
        rc = mqc_ISMCError("ismc_getManagerRecords");
        goto MOD_EXIT;
    }

    /* Clear QueueManagerConnection indices (IMA reconnect) */
    pQM = mqcMQConnectivity.pQMList;

    while (pQM) {
        pQM->index = 0;
        memset(&pQM->indexRecord, 0, sizeof(struct ismc_manrec_t));

        /* Has the store been cold started ? */
        if (count == 0)
            pQM->flags |= QM_COLDSTART;
        pQM = pQM->pNext;
    }

    /* Clear DestinationMappingRule indices (IMA reconnect) */
    pRule = mqcMQConnectivity.pRuleList;

    while (pRule) {
        pRule->index = 0;
        memset(&pRule->indexRecord, 0, sizeof(struct ismc_manrec_t));
        pRule = pRule->pNext;
    }

    /* Clear LastIndex indices (IMA reconnect) */
    mqcMQConnectivity.lastQMIndex = 0;
    mqcMQConnectivity.lastQMIndexRecord.eyecatcher[0] = 0;
    mqcMQConnectivity.lastRuleIndex = 0;
    mqcMQConnectivity.lastRuleIndexRecord.eyecatcher[0] = 0;

    for (i = 0; i < count; i++) {
        pRecord = (mqcManagerRecord_t *) pManRecList[i].data;

        switch (pRecord->type) {
        case MRTYPE_OBJECTIDENTIFIER:
            memcpy(mqcMQConnectivity.objectIdentifier,
                    pRecord->object.objectIdentifier,
                    MQC_OBJECT_IDENTIFIER_LENGTH);
            mqcMQConnectivity.objectIdentifier[MQC_OBJECT_IDENTIFIER_LENGTH] =
                    0;
            TRACE(MQC_NORMAL_TRACE,
                    "Found objectIdentifier %s in server store\n",
                    mqcMQConnectivity.objectIdentifier);
            break;

        case MRTYPE_QMC:
            TRACE(MQC_NORMAL_TRACE, "QueueManagerConnection '%.*s' index(%d)\n",
                    pRecord->object.qmc.length, pRecord->object.qmc.name,
                    pRecord->object.qmc.index);

            pQM = mqcMQConnectivity.pQMList;

            while (pQM) {
                if ((pRecord->object.qmc.length == strlen(pQM->pName))
                        && (memcmp(pRecord->object.qmc.name, pQM->pName,
                                pRecord->object.qmc.length) == 0)) {
                    /* It is possible for the store to contain        */
                    /* multiple records for a single QM (see          */
                    /* mqc_setRuleIndex). Since index numbers always  */
                    /* increment, the correct one is the largest.     */
                    if (pQM->index < pRecord->object.qmc.index) {
                        /* Use the index from the store record since  */
                        /* it is larger (ie more recently written).   */
                        if (pQM->indexRecord.eyecatcher[0] != 0) {
                            /* Delete the store record associated     */
                            /* with the superseded index number.      */

                            rc = ismc_deleteManagerRecord(pSess,
                                    &pQM->indexRecord);
                            TRACE(MQC_ISMC_TRACE,
                                    "ismc_deleteManagerRecord rc(%d)\n", rc);

                            if (rc) {
                                rc = mqc_ISMCError("ismc_deleteManagerRecord");
                                goto MOD_EXIT;
                            }
                        }
                        pQM->index = pRecord->object.qmc.index;
                        memcpy(&pQM->indexRecord, pManRecList[i].handle,
                                sizeof(struct ismc_manrec_t));
                    }

                    break;
                }
                pQM = pQM->pNext;
            }

            if (pQM == NULL) {
                TRACE(MQC_NORMAL_TRACE,
                        "Deleting unknown QueueManagerConnection\n");

                rc = ismc_deleteManagerRecord(pSess, pManRecList[i].handle);
                TRACE(MQC_ISMC_TRACE, "ismc_deleteManagerRecord rc(%d)\n", rc);

                if (rc) {
                    rc = mqc_ISMCError("ismc_deleteManagerRecord");
                    goto MOD_EXIT;
                }
            }
            break;

        case MRTYPE_DMR:
            TRACE(MQC_NORMAL_TRACE, "DestinationMappingRule '%.*s' index(%d)\n",
                    pRecord->object.dmr.length, pRecord->object.dmr.name,
                    pRecord->object.dmr.index);

            pRule = mqcMQConnectivity.pRuleList;

            while (pRule) {
                if ((pRecord->object.dmr.length == strlen(pRule->pName))
                        && (memcmp(pRecord->object.dmr.name, pRule->pName,
                                pRecord->object.dmr.length) == 0)) {
                    /* It is possible for the store to contain        */
                    /* multiple records for a single rule (see        */
                    /* mqc_setRuleIndex). Since index numbers always  */
                    /* increment, the correct one is the largest.     */
                    if (pRule->index < pRecord->object.dmr.index) {
                        /* Use the index from the store record since  */
                        /* it is larger (ie more recently written).   */
                        if (pRule->indexRecord.eyecatcher[0] != 0) {
                            /* Delete the store record associated     */
                            /* with the superseded index number.      */

                            rc = ismc_deleteManagerRecord(pSess,
                                    &pRule->indexRecord);
                            TRACE(MQC_ISMC_TRACE,
                                    "ismc_deleteManagerRecord rc(%d)\n", rc);

                            if (rc) {
                                rc = mqc_ISMCError("ismc_deleteManagerRecord");
                                goto MOD_EXIT;
                            }
                        }
                        pRule->index = pRecord->object.dmr.index;
                        memcpy(&pRule->indexRecord, pManRecList[i].handle,
                                sizeof(struct ismc_manrec_t));
                    }

                    break;
                }
                pRule = pRule->pNext;
            }

            if (pRule == NULL) {
                TRACE(MQC_NORMAL_TRACE,
                        "Deleting unknown DestinationMappingRule\n");

                rc = ismc_deleteManagerRecord(pSess, pManRecList[i].handle);
                TRACE(MQC_ISMC_TRACE, "ismc_deleteManagerRecord rc(%d)\n", rc);

                if (rc) {
                    rc = mqc_ISMCError("ismc_deleteManagerRecord");
                    goto MOD_EXIT;
                }
            }
            break;

        case MRTYPE_QMC_LASTINDEX:
            TRACE(MQC_NORMAL_TRACE, "QueueManagerConnection LastIndex(%d)\n",
                    pRecord->object.lastindex);
            mqcMQConnectivity.lastQMIndex = pRecord->object.lastindex;
            memcpy(&mqcMQConnectivity.lastQMIndexRecord, pManRecList[i].handle,
                    sizeof(struct ismc_manrec_t));
            break;

        case MRTYPE_DMR_LASTINDEX:
            TRACE(MQC_NORMAL_TRACE, "DestinationMappingRule LastIndex(%d)\n",
                    pRecord->object.lastindex);
            mqcMQConnectivity.lastRuleIndex = pRecord->object.lastindex;
            memcpy(&mqcMQConnectivity.lastRuleIndexRecord,
                    pManRecList[i].handle, sizeof(struct ismc_manrec_t));
            break;

        default:
            TRACE(MQC_NORMAL_TRACE, "Deleting unknown record type(%d)\n",
                    pRecord->type);

            rc = ismc_deleteManagerRecord(pSess, pManRecList[i].handle);
            TRACE(MQC_ISMC_TRACE, "ismc_deleteManagerRecord rc(%d)\n", rc);

            if (rc) {
                rc = mqc_ISMCError("ismc_deleteManagerRecord");
                goto MOD_EXIT;
            }
            break;
        }
    }

    /* Set any unset QueueManagerConnection indices */
    pQM = mqcMQConnectivity.pQMList;

    while (pQM) {
        if (pQM->index == 0) {
            pQM->index = ++mqcMQConnectivity.lastQMIndex;
            rc = mqc_setQMIndex(pQM, false);
            if (rc)
                goto MOD_EXIT;
        }
        pQM = pQM->pNext;
    }

    /* Set any unset DestinationMappingRule indices */
    pRule = mqcMQConnectivity.pRuleList;

    while (pRule) {
        if (pRule->index == 0) {
            pRule->index = ++mqcMQConnectivity.lastRuleIndex;
            rc = mqc_setRuleIndex(pRule, false);
            if (rc)
                goto MOD_EXIT;
        }
        pRule = pRule->pNext;
    }

    MOD_EXIT:

    if (pManRecList) {
        ismc_freeManagerRecords(pManRecList);
        TRACE(MQC_ISMC_TRACE, "ismc_freeManagerRecords\n");
    }

    if (pSess) {
        exitrc = ismc_closeSession(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", exitrc);

        exitrc = ismc_free(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", exitrc);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_setQMIndex(mqcQM_t * pQM, bool rename) {
    int len, exitrc, rc = 0;
    ismc_session_t * pSess = NULL;
    mqcManagerRecord_t * pRecord = NULL;
    ismc_manrec_t pManRec;
    bool first = true;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "'%s' %d\n", __func__, pQM->pName,
            pQM->index);

    if (ism_common_threadSelf() != mqcMQConnectivity.reconnect_thread_handle) {
        while ((mqcMQConnectivity.flags & MQC_DISABLED)
                || (mqcMQConnectivity.flags & MQC_RECONNECTING)) {
            if (first) {
                mqcMQConnectivity.flags |= MQC_IMMEDIATE_RECONNECT;
                mqc_waitReconnectAttempt();
                first = false;
            } else {
                rc = RC_MQC_RECONNECT;
                goto MOD_EXIT;
            }
        }
    }

    /* Create session */
    pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, 0, 0);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession pSess <%p>\n", pSess);

    if (pSess == NULL) {
        rc = mqc_ISMCError("ismc_createSession");
        goto MOD_EXIT;
    }

    /* Create QueueManagerConnection record */
    len = offsetof(mqcManagerRecord_t, object)
            + offsetof(mqcQueueManagerConnection_t, name) + strlen(pQM->pName);

    pRecord = mqc_malloc(len);

    if (pRecord == NULL) {
        mqc_allocError("malloc", len, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }

    memset(pRecord, 0, len);
    pRecord->eyecatcher = MANAGER_RECORD_EYECATCHER;
    pRecord->version = MANAGER_RECORD_VERSION;
    pRecord->size = len;
    pRecord->type = MRTYPE_QMC;
    pRecord->object.qmc.index = pQM->index;
    pRecord->object.qmc.length = strlen(pQM->pName);
    memcpy(pRecord->object.qmc.name, pQM->pName, strlen(pQM->pName));

    pManRec = ismc_createManagerRecord(pSess, (const void *) pRecord,
            pRecord->size);
    TRACE(MQC_ISMC_TRACE, "ismc_createManagerRecord <%p>\n", pManRec);

    if (pManRec == NULL) {
        rc = mqc_ISMCError("ismc_createManagerRecord");
        goto MOD_EXIT;
    }

    memcpy(&pQM->indexRecord, pManRec, sizeof(struct ismc_manrec_t));
    ism_common_free(ism_memory_mqcbridge_misc,pManRec);
    pManRec = NULL;

    if (rename == false) {
        /* Remove previous QueueManagerConnection LastIndex record */
        if (mqcMQConnectivity.lastQMIndexRecord.eyecatcher[0]) {
            rc = ismc_deleteManagerRecord(pSess,
                    &mqcMQConnectivity.lastQMIndexRecord);
            TRACE(MQC_ISMC_TRACE, "ismc_deleteManagerRecord rc(%d)\n", rc);

            if (rc) {
                rc = mqc_ISMCError("ismc_deleteManagerRecord");
                goto MOD_EXIT;
            }
        }

        /* Create QueueManagerConnection LastIndex record */
        memset(pRecord, 0, len);
        pRecord->eyecatcher = MANAGER_RECORD_EYECATCHER;
        pRecord->version = MANAGER_RECORD_VERSION;
        pRecord->size = offsetof(mqcManagerRecord_t, object)
                + sizeof(pRecord->object.lastindex);
        pRecord->type = MRTYPE_QMC_LASTINDEX;
        pRecord->object.lastindex = pQM->index;

        pManRec = ismc_createManagerRecord(pSess, (const void *) pRecord,
                pRecord->size);
        TRACE(MQC_ISMC_TRACE, "ismc_createManagerRecord <%p>\n", pManRec);

        if (pManRec == NULL) {
            rc = mqc_ISMCError("ismc_createManagerRecord");
            goto MOD_EXIT;
        }

        memcpy(&mqcMQConnectivity.lastQMIndexRecord, pManRec,
                sizeof(struct ismc_manrec_t));
        ism_common_free(ism_memory_mqcbridge_misc,pManRec);
        pManRec = NULL;
    }

    MOD_EXIT:

    if (pSess) {
        exitrc = ismc_closeSession(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", exitrc);

        exitrc = ismc_free(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", exitrc);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_deleteQMIndex(mqcQM_t * pQM) {
    int exitrc, rc = 0;
    ismc_session_t * pSess = NULL;
    bool first = true;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    while ((mqcMQConnectivity.flags & MQC_DISABLED)
            || (mqcMQConnectivity.flags & MQC_RECONNECTING)) {
        if (first) {
            mqcMQConnectivity.flags |= MQC_IMMEDIATE_RECONNECT;
            mqc_waitReconnectAttempt();
            first = false;
        } else {
            rc = RC_MQC_RECONNECT;
            goto MOD_EXIT;
        }
    }

    /* Create session */
    pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, 0, 0);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession pSess <%p>\n", pSess);

    if (pSess == NULL) {
        rc = mqc_ISMCError("ismc_createSession");
        goto MOD_EXIT;
    }

    rc = ismc_deleteManagerRecord(pSess, &pQM->indexRecord);
    TRACE(MQC_ISMC_TRACE, "ismc_deleteManagerRecord rc(%d)\n", rc);

    if (rc) {
        rc = mqc_ISMCError("ismc_deleteManagerRecord");
        goto MOD_EXIT;
    }

    MOD_EXIT:

    if (pSess) {
        exitrc = ismc_closeSession(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", exitrc);

        exitrc = ismc_free(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", exitrc);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_setRuleIndex(mqcRule_t * pRule, bool rename) {
    int len, exitrc, rc = 0;
    ismc_session_t * pSess = NULL;
    mqcManagerRecord_t * pRecord = NULL;
    ismc_manrec_t pManRec;
    bool first = true;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "'%s' %d\n", __func__, pRule->pName,
            pRule->index);

    if (ism_common_threadSelf() != mqcMQConnectivity.reconnect_thread_handle) {
        while ((mqcMQConnectivity.flags & MQC_DISABLED)
                || (mqcMQConnectivity.flags & MQC_RECONNECTING)) {
            if (first) {
                mqcMQConnectivity.flags |= MQC_IMMEDIATE_RECONNECT;
                mqc_waitReconnectAttempt();
                first = false;
            } else {
                rc = RC_MQC_RECONNECT;
                goto MOD_EXIT;
            }
        }
    }

    /* Create session */
    pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, 0, 0);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession pSess <%p>\n", pSess);

    if (pSess == NULL) {
        rc = mqc_ISMCError("ismc_createSession");
        goto MOD_EXIT;
    }

    /* Create DestinationMappingRule record */
    len = offsetof(mqcManagerRecord_t, object)
            + offsetof(mqcDestinationMappingRule_t, name)
            + strlen(pRule->pName);

    pRecord = mqc_malloc(len);

    if (pRecord == NULL) {
        mqc_allocError("malloc", len, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }

    memset(pRecord, 0, len);
    pRecord->eyecatcher = MANAGER_RECORD_EYECATCHER;
    pRecord->version = MANAGER_RECORD_VERSION;
    pRecord->size = len;
    pRecord->type = MRTYPE_DMR;
    pRecord->object.dmr.index = pRule->index;
    pRecord->object.dmr.length = strlen(pRule->pName);
    memcpy(pRecord->object.dmr.name, pRule->pName, strlen(pRule->pName));

    pManRec = ismc_createManagerRecord(pSess, (const void *) pRecord,
            pRecord->size);
    TRACE(MQC_ISMC_TRACE, "ismc_createManagerRecord <%p>\n", pManRec);

    if (pManRec == NULL) {
        rc = mqc_ISMCError("ismc_createManagerRecord");
        goto MOD_EXIT;
    }

    memcpy(&pRule->indexRecord, pManRec, sizeof(struct ismc_manrec_t));
    ism_common_free(ism_memory_mqcbridge_misc,pManRec);
    pManRec = NULL;

    if (rename == false) {
        /* Remove previous QueueManagerConnection LastIndex record */
        if (mqcMQConnectivity.lastRuleIndexRecord.eyecatcher[0]) {
            rc = ismc_deleteManagerRecord(pSess,
                    &mqcMQConnectivity.lastRuleIndexRecord);
            TRACE(MQC_ISMC_TRACE, "ismc_deleteManagerRecord rc(%d)\n", rc);

            if (rc) {
                rc = mqc_ISMCError("ismc_deleteManagerRecord");
                goto MOD_EXIT;
            }
        }

        /* Create DestinationMappingRule LastIndex record */
        memset(pRecord, 0, len);
        pRecord->eyecatcher = MANAGER_RECORD_EYECATCHER;
        pRecord->version = MANAGER_RECORD_VERSION;
        pRecord->size = offsetof(mqcManagerRecord_t, object)
                + sizeof(pRecord->object.lastindex);
        pRecord->type = MRTYPE_DMR_LASTINDEX;
        pRecord->object.lastindex = pRule->index;

        /* There might already be a stored record for this mapping    */
        /* rule recording the earlier index number, in which case we  */
        /* ought to delete the obsolete one. However, if we do that   */
        /* we create a race condition if we crash between the create  */
        /* and delete, resulting in two records or none depending on  */
        /* the sequence of the create and delete. To prevent that, we */
        /* don't do the delete here and instead make the              */
        /* mqc_loadRecords function allow for multiple records for a  */
        /* single rule, discarding the ones with lower index numbers. */

        pManRec = ismc_createManagerRecord(pSess, (const void *) pRecord,
                pRecord->size);
        TRACE(MQC_ISMC_TRACE, "ismc_createManagerRecord <%p>\n", pManRec);

        if (pManRec == NULL) {
            rc = mqc_ISMCError("ismc_createManagerRecord");
            goto MOD_EXIT;
        }

        memcpy(&mqcMQConnectivity.lastRuleIndexRecord, pManRec,
                sizeof(struct ismc_manrec_t));
        ism_common_free(ism_memory_mqcbridge_misc,pManRec);
        pManRec = NULL;
    }

    MOD_EXIT:

    if (pSess) {
        exitrc = ismc_closeSession(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", exitrc);
        if (exitrc == 0) {
            exitrc = ismc_free(pSess);
            TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", exitrc);
        }
    }

    if (pRecord) {
        mqc_free(pRecord);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_deleteRuleIndex(mqcRule_t * pRule) {
    int exitrc, rc = 0;
    ismc_session_t * pSess = NULL;
    bool first = true;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    while ((mqcMQConnectivity.flags & MQC_DISABLED)
            || (mqcMQConnectivity.flags & MQC_RECONNECTING)) {
        if (first) {
            mqcMQConnectivity.flags |= MQC_IMMEDIATE_RECONNECT;
            mqc_waitReconnectAttempt();
            first = false;
        } else {
            rc = RC_MQC_RECONNECT;
            goto MOD_EXIT;
        }
    }

    /* Create session */
    pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, 0, 0);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession pSess <%p>\n", pSess);

    if (pSess == NULL) {
        rc = mqc_ISMCError("ismc_createSession");
        goto MOD_EXIT;
    }

    rc = ismc_deleteManagerRecord(pSess, &pRule->indexRecord);
    TRACE(MQC_ISMC_TRACE, "ismc_deleteManagerRecord rc(%d)\n", rc);

    if (rc) {
        rc = mqc_ISMCError("ismc_deleteManagerRecord");
        goto MOD_EXIT;
    }

    MOD_EXIT:

    if (pSess) {
        exitrc = ismc_closeSession(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", exitrc);

        exitrc = ismc_free(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", exitrc);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_TraceProps(ism_prop_t * props) {
    int i, rc = 0;
    const char * propname;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if ( props ) {
        for (i = 0; ism_common_getPropertyIndex(props, i, &propname) == 0; i++) {
            TRACE(MQC_NORMAL_TRACE, "'%s'\n", propname);
        }
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

/* This function is a wrapper for ism_server_config that enables us   */
/* to set MQ trace after the IMA trace level has been updated.        */
int mqc_serverCallback(char * object, char * name, ism_prop_t * props,
        ism_ConfigChangeType_t type) {
    int rc = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    rc = ism_server_config(object, name, props, type);

    mqc_setMQTrace(ism_common_getTraceLevel());

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_adminCallback(char * object, char * propqual, ism_prop_t * props,
        ism_ConfigChangeType_t type) {
    int len, rc = 0;
    int normalTraceLevel = MQC_NORMAL_TRACE;
    mqcQM_t * pQM;
    mqcRule_t * pRule;
    bool bQM, force = false, rename = false;
    const char * name = NULL, *oldname;
    char * ourpropname = NULL;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.adminCallbackMutex);

    if (mqcMQConnectivity.traceAdmin) {
        normalTraceLevel = ism_common_getTraceLevel();
        ism_common_setTraceLevel(mqcMQConnectivity.traceAdminLevel);
    }

    mqc_TraceProps(props);

    if ((strlen(object) == strlen("QueueManagerConnection"))
            && (memcmp(object, "QueueManagerConnection",
                    strlen("QueueManagerConnection")) == 0)) {
        bQM = true;

        /* Get the Force option and Name */
        len = strlen("QueueManagerConnection.Force.") + strlen(propqual) + 1;
        ourpropname = mqc_malloc(len);
        if (ourpropname == NULL) {
            mqc_allocError("malloc", len, errno);
            rc = RC_MQC_DISABLE;
            goto MOD_EXIT;
        }
        sprintf(ourpropname, "QueueManagerConnection.Force.%s", propqual);
        force = ism_common_getIntProperty(props, ourpropname, 0);
        sprintf(ourpropname, "QueueManagerConnection.Name.%s", propqual);
        name = ism_common_getStringProperty(props, ourpropname);
        mqc_free(ourpropname);
    } else if ((strlen(object) == strlen("DestinationMappingRule"))
            && (memcmp(object, "DestinationMappingRule",
                    strlen("DestinationMappingRule")) == 0)) {
        bQM = false;

        /* Get the Name */
        len = strlen("DestinationMappingRule.Name.") + strlen(propqual) + 1;
        ourpropname = mqc_malloc(len);
        if (ourpropname == NULL) {
            mqc_allocError("malloc", len, errno);
            rc = RC_MQC_DISABLE;
            goto MOD_EXIT;
        }
        sprintf(ourpropname, "DestinationMappingRule.Name.%s", propqual);
        name = ism_common_getStringProperty(props, ourpropname);
        mqc_free(ourpropname);

    } else if ((strlen(object) == strlen("MQConnectivityLog"))
            && (memcmp(object, "MQConnectivityLog", strlen("MQConnectivityLog"))
                    == 0)) {
        ism_server_config(object, propqual, props, type);
        goto MOD_EXIT;
    } else {
        TRACE(MQC_NORMAL_TRACE, "Unknown object '%s'\n", object);
        rc = ISMRC_UnexpectedConfig;
        goto MOD_EXIT;
    }

    // No name found
    if (name == NULL) {
        TRACE(MQC_NORMAL_TRACE,
                "No name supplied for object '%s' propqual '%s'\n", object,
                propqual);
        rc = RC_INVALID_CONFIG;
        goto MOD_EXIT;
    }

    switch (type) {
    case ISM_CONFIG_CHANGE_NAME:
        oldname = ism_common_getStringProperty(props, "OLD_Name");
        rename = true;

        if (oldname == NULL) {
            TRACE(MQC_NORMAL_TRACE, "No OLD_Name property\n");
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }

        if (bQM) {
            rc = mqc_loadQM(props, propqual, name, oldname, &pQM);
            if (rc)
                goto MOD_EXIT;

            rc = mqc_updateQM(props, propqual, pQM, oldname, rename, force);
            if (rc)
                goto MOD_EXIT;
        } else {
            rc = mqc_loadRule(props, propqual, name, oldname, &pRule);
            if (rc)
                goto MOD_EXIT;

            rc = mqc_updateRule(props, propqual, pRule, oldname, rename);
            if (rc)
                goto MOD_EXIT;
        }
        break;
    case ISM_CONFIG_CHANGE_PROPS:
        if (bQM) {
            rc = mqc_loadQM(props, propqual, name, NULL, &pQM);
            if (rc)
                goto MOD_EXIT;

            rc = mqc_updateQM(props, propqual, pQM, name, rename, force);
            if (rc)
                goto MOD_EXIT;
        } else {
            rc = mqc_loadRule(props, propqual, name, NULL, &pRule);
            if (rc)
                goto MOD_EXIT;

            rc = mqc_updateRule(props, propqual, pRule, name, rename);
            if (rc)
                goto MOD_EXIT;
        }
        break;
    case ISM_CONFIG_CHANGE_DELETE:
        if (bQM) {
            rc = mqc_deleteQM(props, propqual, name, force);
            if (rc)
                goto MOD_EXIT;
        } else {
            rc = mqc_deleteRule(props, propqual, name);
            if (rc)
                goto MOD_EXIT;
        }
        break;

    default:
        TRACE(MQC_NORMAL_TRACE, "Unknown Config Change Type '%d'\n", type);
        rc = RC_INVALID_CONFIG;
        goto MOD_EXIT;
        break;
    }

    MOD_EXIT:

    switch (rc) {
    case 0:
        break;
    case RC_INVALID_STATE_UPDATE:
        rc = ISMRC_MappingStateUpdate;
        break;
    case RC_NOT_DISABLED:
        if (bQM) {
            if (type == ISM_CONFIG_CHANGE_DELETE) {
                rc = ISMRC_QMDelete;
            } else {
                rc = ISMRC_QMUpdate;
            }
        } else {
            rc = ISMRC_MappingUpdate;
        }
        break;
    case RC_INVALID_TOPIC:
        rc = ISMRC_BadTopic;
        break;
    case RC_MQC_RECONNECT:
        rc = ISMRC_ISMNotAvailable;
        break;
    case RC_NOT_FOUND:
        if (type == ISM_CONFIG_CHANGE_DELETE) {
            rc = ISMRC_OK;
        } else {
            rc = ISMRC_NotFound;
        }
        break;
    case RC_TOO_MANY_RULES:
        rc = ISMRC_TooManyRules;
        break;
    case RC_UNRESOLVED:
        rc = ISMRC_Unresolved;
        break;
    default:
        rc = ISMRC_UnexpectedConfig;
        break;
    }

    if (mqcMQConnectivity.traceAdmin) {
        ism_common_setTraceLevel(normalTraceLevel);
    }

    pthread_mutex_unlock(&mqcMQConnectivity.adminCallbackMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_deleteQM(ism_prop_t * props, const char * propqual, const char * name,
        bool force) {
    int rc = 0;
    mqcQM_t * pThisQM, *pPrevQM;
    mqcRule_t * pRule;
    mqcRuleQM_t * pRuleQM;
    bool found = false;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.qmListMutex);
    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);

    /* Find this QM */
    pThisQM = mqcMQConnectivity.pQMList;
    pPrevQM = NULL;

    while (pThisQM) {
        if ((strlen(pThisQM->pName) == strlen(name))
                && (memcmp(pThisQM->pName, name, strlen(pThisQM->pName)) == 0)) {
            found = true;
            break;
        }
        pPrevQM = pThisQM;
        pThisQM = pThisQM->pNext;
    }

    if (found == false) {
        TRACE(MQC_NORMAL_TRACE, "QM '%s' unknown\n", name);
        rc = RC_NOT_FOUND;
        goto MOD_EXIT;
    }

    /* Do we have any rules using the QM ? */
    pRule = mqcMQConnectivity.pRuleList;

    while (pRule) {
        pthread_mutex_lock(&pRule->ruleQMListMutex);
        pRuleQM = pRule->pRuleQMList;

        while (pRuleQM) {
            if (pRuleQM->pQM == pThisQM) {
                TRACE(MQC_NORMAL_TRACE,
                        "QM in use with rule '%s' so delete not allowed\n",
                        pRule->pName);
                pthread_mutex_unlock(&pRule->ruleQMListMutex);
                rc = RC_NOT_DISABLED;
                goto MOD_EXIT;
            }

            pRuleQM = pRuleQM->pNext;
        }
        pthread_mutex_unlock(&pRule->ruleQMListMutex);

        pRule = pRule->pNext;
    }

    /* Do we have any IMA XIDs associated with this QM ? */
    rc = mqc_checkIMAXIDsForQM(pThisQM, force);
    if (rc)
        goto MOD_EXIT;

    /* Remove index */
    rc = mqc_deleteQMIndex(pThisQM);
    if (rc)
        goto MOD_EXIT;

    /* Remove QM from list */
    if (pPrevQM)
        pPrevQM->pNext = pThisQM->pNext;
    else
        mqcMQConnectivity.pQMList = pThisQM->pNext;

    mqc_freeQM(pThisQM);

    MOD_EXIT:
    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);
    pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_deleteRule(ism_prop_t * props, const char * propqual, const char * name) {
    int rc = 0;

    mqcRule_t * pThisRule, *pPrevRule;
    bool found = false;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);

    /* Find this rule */
    pThisRule = mqcMQConnectivity.pRuleList;
    pPrevRule = NULL;

    while (pThisRule) {
        if ((strlen(pThisRule->pName) == strlen(name))
                && (memcmp(pThisRule->pName, name, strlen(pThisRule->pName))
                        == 0)) {
            found = true;
            break;
        }

        pPrevRule = pThisRule;
        pThisRule = pThisRule->pNext;
    }

    if (found == false) {
        TRACE(MQC_NORMAL_TRACE, "Rule '%s' unknown\n", name);
        rc = RC_NOT_FOUND;
        goto MOD_EXIT;
    }

    if (pThisRule->Enabled) {
        TRACE(MQC_NORMAL_TRACE,
                "Rule '%s' not disabled so delete not allowed\n", name);
        rc = RC_NOT_DISABLED;
        goto MOD_EXIT;
    }

    /* Remove index */
    rc = mqc_deleteRuleIndex(pThisRule);
    if (rc)
        goto MOD_EXIT;

    /* Remove rule from list */
    if (pPrevRule)
        pPrevRule->pNext = pThisRule->pNext;
    else
        mqcMQConnectivity.pRuleList = pThisRule->pNext;

    mqc_freeRule(pThisRule, true);

    MOD_EXIT:
    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_updateQM(ism_prop_t * props, const char * propqual, mqcQM_t * pQM,
        const char * name, bool rename, bool force) {
    int rc = 0;
    mqcQM_t * pThisQM, *pPrevQM;
    mqcRule_t * pRule;
    mqcRuleQM_t * pRuleQM;
    bool found = false;
    int oldindex = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.qmListMutex);
    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);

    /* Are we modifying an existing QM ? */
    pThisQM = mqcMQConnectivity.pQMList;
    pPrevQM = NULL;

    while (pThisQM) {
        if ((strlen(pThisQM->pName) == strlen(name))
                && (memcmp(pThisQM->pName, name, strlen(pThisQM->pName)) == 0)) {
            TRACE(MQC_NORMAL_TRACE, "Modifying existing QM '%s'\n", name);
            found = true;
            break;
        }
        pPrevQM = pThisQM;
        pThisQM = pThisQM->pNext;
    }

    if (found) {
        /* Do we have any enabled rules using the existing QM ? */
        pRule = mqcMQConnectivity.pRuleList;

        while (pRule) {
            pthread_mutex_lock(&pRule->ruleQMListMutex);
            pRuleQM = pRule->pRuleQMList;

            while (pRuleQM) {
                if (pRuleQM->pQM == pThisQM) {
                    TRACE(MQC_NORMAL_TRACE, "QM in use with rule '%s'\n",
                            pRule->pName);

                    /* Is the rule disabled ? */
                    if (pRule->Enabled) {
                        TRACE(MQC_NORMAL_TRACE,
                                "Rule not disabled so update not allowed\n");
                        mqc_freeQM(pQM);
                        pthread_mutex_unlock(&pRule->ruleQMListMutex);
                        rc = RC_NOT_DISABLED;
                        goto MOD_EXIT;
                    } else {
                        TRACE(MQC_NORMAL_TRACE,
                                "Rule disabled so update allowed\n");
                    }
                }

                pRuleQM = pRuleQM->pNext;
            }
            pthread_mutex_unlock(&pRule->ruleQMListMutex);

            pRule = pRule->pNext;
        }

        if (rename) {
            oldindex = pThisQM->index;
        } else {
            /* Do we have any IMA XIDs associated with this QM ? */
            rc = mqc_checkIMAXIDsForQM(pThisQM, force);
            if (rc)
                goto MOD_EXIT;
        }
    }

    /* Set QM index */
    pQM->index = (rename) ? oldindex : ++mqcMQConnectivity.lastQMIndex;
    rc = mqc_setQMIndex(pQM, rename);
    if (rc)
        goto MOD_EXIT;

    if (found) {
        /* Update pRuleQM -> pQM */
        pRule = mqcMQConnectivity.pRuleList;

        while (pRule) {
            pthread_mutex_lock(&pRule->ruleQMListMutex);
            pRuleQM = pRule->pRuleQMList;

            while (pRuleQM) {
                if (pRuleQM->pQM == pThisQM) {
                    TRACE(MQC_NORMAL_TRACE, "Updating QM in rule '%s'\n",
                            pRule->pName);
                    pRuleQM->pQM = pQM;
                }

                pRuleQM = pRuleQM->pNext;
            }
            pthread_mutex_unlock(&pRule->ruleQMListMutex);

            pRule = pRule->pNext;
        }

        /* Remove QM from list */
        if (pPrevQM)
            pPrevQM->pNext = pThisQM->pNext;
        else
            mqcMQConnectivity.pQMList = pThisQM->pNext;

        mqc_freeQM(pThisQM);
    }

    /* Add QM to start of list */
    pQM->pNext = mqcMQConnectivity.pQMList;
    mqcMQConnectivity.pQMList = pQM;

    MOD_EXIT:
    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);
    pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_addRuleQMs(mqcRule_t * pRule) {
    int rc = 0;
    mqcQM_t * pQM;
    mqcRuleQM_t * pRuleQM;
    mqcRuleQM_t per_rule_qm_init = { RULEQM_INIT };
    bool knownqm;
    char * qmlist;
    char * pComma;
    char * pList;
    bool ruleQMListMutexLocked = false;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.qmListMutex);
    pthread_mutex_lock(&pRule->ruleQMListMutex);
    ruleQMListMutexLocked = true;

    qmlist = mqc_malloc(strlen(pRule->pQueueManagerConnection) + 1);
    if (qmlist == NULL) {
        mqc_allocError("malloc", strlen(pRule->pQueueManagerConnection) + 1,
                errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(qmlist, pRule->pQueueManagerConnection);
    pList = qmlist;

    do {
        pComma = strchr(pList, ',');

        if (pComma)
            *pComma = 0;

        pQM = mqcMQConnectivity.pQMList;

        knownqm = false;

        while (pQM) {
            if (strcmp(pList, pQM->pName) == 0) {
                knownqm = true;
                break;
            }
            pQM = pQM->pNext;
        }

        if (knownqm == false) {
            TRACE(MQC_NORMAL_TRACE, "QM '%s' unknown\n", pList);
            if (ruleQMListMutexLocked) {
                pthread_mutex_unlock(&pRule->ruleQMListMutex);
                ruleQMListMutexLocked = false;
            }
            mqc_freeRule(pRule, true);
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }

        pList += strlen(pList) + 1;

        /* Add RuleQM to start of RuleQMList */
        pRuleQM = mqc_malloc(sizeof(mqcRuleQM_t));
        if (pRuleQM == NULL) {
            mqc_allocError("malloc", sizeof(mqcRuleQM_t), errno);
            rc = RC_RULEQM_DISABLE;
            goto MOD_EXIT;
        }
        memcpy(pRuleQM, &per_rule_qm_init, sizeof(mqcRuleQM_t));
        if (pRule->Enabled)
            pRuleQM->flags |= RULEQM_STARTING;

        /* Allocate initial memory for properties */
        ism_common_allocAllocBuffer(&pRuleQM->buf, 1024, 0);
        pRuleQM->buf.used = 0;

        pRuleQM->pQM = pQM;
        pRuleQM->pRule = pRule;

        pRuleQM->pNext = pRule->pRuleQMList;
        pRule->pRuleQMList = pRuleQM;
        pRuleQM->index = pRule->ruleQMCount++;

    } while (pComma);

    MOD_EXIT: if (ruleQMListMutexLocked) {
        pthread_mutex_unlock(&pRule->ruleQMListMutex);
        ruleQMListMutexLocked = false;
    }

    pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);

    if (qmlist)
        mqc_free(qmlist);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_rulesMatch(mqcRule_t * pRuleA, mqcRule_t * pRuleB) {
    int rc = 1;
    int lengthQMCA = strlen(pRuleA->pQueueManagerConnection);
    int lengthQMCB = strlen(pRuleB->pQueueManagerConnection);
    int lengthSourceA = 0;
    int lengthSourceB = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if ((pRuleA->RuleType != pRuleB->RuleType) || (lengthQMCA != lengthQMCB)) {
        rc = 0;
        goto mod_exit;
    }

    if (memcmp(pRuleA->pQueueManagerConnection, pRuleB->pQueueManagerConnection,
            lengthQMCA) != 0) {
        rc = 0;
        goto mod_exit;
    }

    /* When comparing source strings, we must use the strings originally      */
    /* supplied by the admin component and not the version we may have        */
    /* modified to include a wildcard.                                        */

    if (pRuleA->sourceEndOffset != 0) {
        lengthSourceA = pRuleA->sourceEndOffset;
    } else {
        lengthSourceA = strlen(pRuleA->pSource);
    }

    if (pRuleB->sourceEndOffset != 0) {
        lengthSourceB = pRuleB->sourceEndOffset;
    } else {
        lengthSourceB = strlen(pRuleB->pSource);
    }

    if (lengthSourceA != lengthSourceB) {
        rc = 0;
        goto mod_exit;
    }

    if (memcmp(pRuleA->pSource, pRuleB->pSource, lengthSourceA) != 0) {
        rc = 0;
        goto mod_exit;
    }

    if (strlen(pRuleA->pDestination) != strlen(pRuleB->pDestination)) {
        rc = 0;
        goto mod_exit;
    }

    if (memcmp(pRuleA->pDestination, pRuleB->pDestination,
            strlen(pRuleA->pDestination)) != 0) {
        rc = 0;
        goto mod_exit;
    }

    mod_exit:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_updateRule(ism_prop_t * props, const char * propqual, mqcRule_t * pRule,
        const char * name, bool rename) {
    int rc = 0;
    mqcRule_t * pThisRule, *pPrevRule;
    bool found = false;
    int oldindex = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);

    /* Are we modifying an existing rule ? */
    pThisRule = mqcMQConnectivity.pRuleList;
    pPrevRule = NULL;

    while (pThisRule) {
        if ((strlen(pThisRule->pName) == strlen(name))
                && (memcmp(pThisRule->pName, name, strlen(pThisRule->pName))
                        == 0)) {
            TRACE(MQC_NORMAL_TRACE, "Modifying existing rule '%s'\n", name);
            found = true;
            break;
        }

        pPrevRule = pThisRule;
        pThisRule = pThisRule->pNext;
    }

    if (found) {
        if (pThisRule->Enabled) {
            /* The rule is currently enabled so the only things that  */
            /* can be changed are MaxMessages and RetainedMessages.   */
        	/* OR rule can be disabled                                */
            if (pThisRule->Enabled != pRule->Enabled) {
                TRACE(MQC_NORMAL_TRACE, "Enabled changed from '%d' to '%d'\n",
                        pThisRule->Enabled, pRule->Enabled);

                if (mqc_rulesMatch(pThisRule, pRule)) {
                    pThisRule->Enabled = pRule->Enabled;

                    if (pRule->Enabled) {
                        mqc_startRule(pThisRule);
                    } else {
                        mqc_termRule(pThisRule,
                        true); /* user requested */
                    }
                    mqc_freeRule(pRule, false);
                    goto MOD_EXIT;
                } else {
                    TRACE(MQC_NORMAL_TRACE,
                            "Enabled state update must not be made with other updates\n");
                    rc = RC_INVALID_STATE_UPDATE;
                    mqc_freeRule(pRule, false);
                    goto MOD_EXIT;
                }
            }

            if (pRule->flags & RULE_PROTECTED_UPDATE) {
                TRACE(MQC_NORMAL_TRACE,
                        "Rule not disabled so update not allowed\n");
                rc = RC_NOT_DISABLED;
                mqc_freeRule(pRule, false);
                goto MOD_EXIT;
            } else {
                TRACE(MQC_NORMAL_TRACE,
                        "Changing RetainedMessages from %d to %d\n",
                        pThisRule->RetainedMessages, pRule->RetainedMessages);
                pThisRule->RetainedMessages = pRule->RetainedMessages;

                if (pThisRule->MaxMessages != pRule->MaxMessages) {
                    TRACE(MQC_NORMAL_TRACE,
                            "Changing MaxMessages from %d to %d.\n",
                            pThisRule->MaxMessages, pRule->MaxMessages);
                    pThisRule->flags |= RULE_MAXMESSAGES_UPDATED;
                    pThisRule->MaxMessages = pRule->MaxMessages;
                    TRACE(MQC_ISMC_TRACE, "pThisRule -> flags = %d\n",
                            pThisRule->flags);
                }

                mqc_freeRule(pRule, false);
                goto MOD_EXIT;
            }
        }

        /* The rule is disabled, process all configuration items */
        if (rename)
            oldindex = pThisRule->index;

        /* Set rule index */
        pRule->index = (rename) ? oldindex : ++mqcMQConnectivity.lastRuleIndex;
        rc = mqc_setRuleIndex(pRule, rename);
        if (rc)
            goto MOD_EXIT;

        /* Remove rule from list */
        if (pPrevRule)
            pPrevRule->pNext = pThisRule->pNext;
        else
            mqcMQConnectivity.pRuleList = pThisRule->pNext;

        mqc_freeRule(pThisRule, false);

        /* Add our rule QueueManagerConnection(s) */
        rc = mqc_addRuleQMs(pRule);
        if (rc)
            goto MOD_EXIT;

    } else /* !found */
    {
        /* Add our rule QueueManagerConnection(s) */
        rc = mqc_addRuleQMs(pRule);
        if (rc)
            goto MOD_EXIT;

        /* Set rule index */
        pRule->index = (rename) ? oldindex : ++mqcMQConnectivity.lastRuleIndex;
        rc = mqc_setRuleIndex(pRule, rename);
        if (rc)
            goto MOD_EXIT;
    }

    /* Add rule to start of rule list */
    pRule->pNext = mqcMQConnectivity.pRuleList;
    mqcMQConnectivity.pRuleList = pRule;

    /* Start rule if enabled */
    if (pRule->Enabled) {
        mqc_startRule(pRule);
    }

    MOD_EXIT:
    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_loadIMA(mqcIMA_t * pIMA) {
    int rc = 0;
    char * pIMAProtocol;
    char * pIMAAddress;
    int IMAPort;
    char * pIMAClientID;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pIMAProtocol = (char *) ism_common_getStringConfig("IMAProtocol");
    if (pIMAProtocol == NULL) {
        pIMAProtocol = "tcp";
        TRACE(MQC_NORMAL_TRACE, "Assuming protocol of '%s'\n", pIMAProtocol);
    }

    pIMAClientID = (char *) ism_common_getStringConfig("IMAClientID");
    if (pIMAClientID == NULL) {
        pIMAClientID = "__MQConnectivity";
        TRACE(MQC_NORMAL_TRACE, "Assuming client Id of '%s'\n", pIMAClientID);
    }

    pIMAAddress = (char *) ism_common_getStringConfig("IMAAddress"); // ONLY NEEDED FOR mqcbridge.cfg support
    if (pIMAAddress == NULL) {
        pIMAAddress = (char *) ism_common_getStringConfig("Endpoint.Interface.!MQConnectivityEndpoint");
    }

    IMAPort = ism_common_getIntConfig("IMAPort", 0); // ONLY NEEDED for mqcbridge.cfg support
    if (IMAPort == 0) {
        IMAPort = ism_common_getIntConfig("Endpoint.Port.!MQConnectivityEndpoint", 0);
    }

    if ((pIMAAddress == NULL) || ((pIMAAddress[0] != '/') && (IMAPort == 0))) {
        TRACE(MQC_NORMAL_TRACE,
                "IMA requires !MQConnectivityEndpoint Interface & Port specifications\n");
        rc = RC_INVALID_CONFIG;
        goto MOD_EXIT;
    }

    pIMA->pIMAProtocol = mqc_malloc(strlen(pIMAProtocol) + 1);
    if (pIMA->pIMAProtocol == NULL) {
        mqc_allocError("malloc", strlen(pIMAProtocol) + 1, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(pIMA->pIMAProtocol, pIMAProtocol);

    pIMA->pIMAAddress = mqc_malloc(strlen(pIMAAddress) + 1);
    if (pIMA->pIMAAddress == NULL) {
        mqc_allocError("malloc", strlen(pIMAAddress) + 1, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(pIMA->pIMAAddress, pIMAAddress);

    pIMA->IMAPort = IMAPort;

    pIMA->pIMAClientID = mqc_malloc(strlen(pIMAClientID) + 1);
    if (pIMA->pIMAClientID == NULL) {
        mqc_allocError("malloc", strlen(pIMAClientID) + 1, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(pIMA->pIMAClientID, pIMAClientID);

    MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_loadTuning(void) {
    int rc = 0;
    int overrideValue = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    mqcMQConnectivity.batchSize = ism_common_getIntConfig("MQConnectivity.BatchSize",
                                                          MQC_BATCH_SIZE);
    TRACE(MQC_NORMAL_TRACE, "Batch size is %d (default = %d)\n",
          mqcMQConnectivity.batchSize, MQC_BATCH_SIZE);

    mqcMQConnectivity.ruleLimit = ism_common_getIntConfig("MQConnectivity.RuleLimit",
                                                          MQC_RULE_LIMIT);
    TRACE(MQC_NORMAL_TRACE, "Maximum number of rules is %d (default = %d)\n",
          mqcMQConnectivity.ruleLimit, MQC_RULE_LIMIT);

    mqcMQConnectivity.discardUndeliverable = ism_common_getIntConfig("MQConnectivity.DiscardUndeliverable",
                                                                     MQC_DISCARD_UNDELIVERABLE);
    TRACE(MQC_NORMAL_TRACE, "Discard undeliverable messages is %d (default = %d)\n",
          mqcMQConnectivity.discardUndeliverable, MQC_DISCARD_UNDELIVERABLE);

    mqcMQConnectivity.batchDataLimit = ism_common_getIntConfig("MQConnectivity.BatchDataLimit",
                                                               MQC_BATCH_DATA_LIMIT);
    TRACE(MQC_NORMAL_TRACE, "Batch data limit is %d (default = %d)\n",
          mqcMQConnectivity.batchDataLimit, MQC_BATCH_DATA_LIMIT);

    mqcMQConnectivity.longBatchTimeout = ism_common_getIntConfig("MQConnectivity.LongBatchTimeout",
                                                                 MQC_LONG_BATCH_TIMEOUT);
    TRACE(MQC_NORMAL_TRACE, "Long Batch Timeout is %d (default = %d)\n",
          (int) mqcMQConnectivity.longBatchTimeout, MQC_LONG_BATCH_TIMEOUT);

    mqcMQConnectivity.shortBatchTimeout = ism_common_getIntConfig("MQConnectivity.ShortBatchTimeout",
                                                                  MQC_SHORT_BATCH_TIMEOUT);
    TRACE(MQC_NORMAL_TRACE, "Short Batch Timeout is %d (default = %d)\n",
          (int) mqcMQConnectivity.shortBatchTimeout, MQC_SHORT_BATCH_TIMEOUT);

    mqcMQConnectivity.immediateRetryCount = ism_common_getIntConfig("MQConnectivity.ImmediateRetryCount",
                                                                    MQC_IMMEDIATE_RETRY_COUNT);
    TRACE(MQC_NORMAL_TRACE, "ImmediateRetryCount is %d (default = %d)\n",
          mqcMQConnectivity.immediateRetryCount, MQC_IMMEDIATE_RETRY_COUNT);

    mqcMQConnectivity.shortRetryInterval = ism_common_getIntConfig("MQConnectivity.ShortRetryInterval",
                                                                   MQC_SHORT_RETRY_INTERVAL);
    TRACE(MQC_NORMAL_TRACE, "ShortRetryInterval is %d (default = %d)\n",
          mqcMQConnectivity.shortRetryInterval, MQC_SHORT_RETRY_INTERVAL);

    mqcMQConnectivity.shortRetryCount = ism_common_getIntConfig("MQConnectivity.ShortRetryCount",
                                                                MQC_SHORT_RETRY_COUNT);
    TRACE(MQC_NORMAL_TRACE, "ShortRetryCount is %d (default = %d)\n",
          mqcMQConnectivity.shortRetryCount, MQC_SHORT_RETRY_COUNT);

    mqcMQConnectivity.longRetryInterval = ism_common_getIntConfig("MQConnectivity.LongRetryInterval",
                                                                  MQC_LONG_RETRY_INTERVAL);
    TRACE(MQC_NORMAL_TRACE, "LongRetryInterval is %d (default = %d)\n",
          mqcMQConnectivity.longRetryInterval, MQC_LONG_RETRY_INTERVAL);

    mqcMQConnectivity.longRetryCount = ism_common_getIntConfig("MQConnectivity.LongRetryCount",
                                                               MQC_LONG_RETRY_COUNT);
    TRACE(MQC_NORMAL_TRACE, "LongRetryCount from static configuration is %d (default = %d)\n",
          mqcMQConnectivity.longRetryCount, MQC_LONG_RETRY_COUNT);

    mqcMQConnectivity.defaultConnectionCount = ism_common_getIntConfig("MQConnectivity.DefaultConnectionCount",
                                                                       MQC_DEFAULT_CONNECTION_COUNT);
    TRACE(MQC_NORMAL_TRACE, "DefaultConnectionCount is %d (default = %d)\n",
          mqcMQConnectivity.defaultConnectionCount, MQC_DEFAULT_CONNECTION_COUNT);

    mqcMQConnectivity.MQTraceMaxFileSize = ism_common_getIntConfig("MQConnectivity.MQTraceMaxFileSize",
                                                                   MQC_MQ_TRACE_MAX_FILE_SIZE);
    TRACE(MQC_NORMAL_TRACE, "MQTraceMaxFileSize is %d (default = %d)\n",
          mqcMQConnectivity.MQTraceMaxFileSize, MQC_MQ_TRACE_MAX_FILE_SIZE);

    mqcMQConnectivity.MQTraceUserDataSize = ism_common_getIntConfig("MQConnectivity.MQTraceUserDataSize",
                                                                    MQC_MQ_TRACE_USER_DATA_SIZE);
    TRACE(MQC_NORMAL_TRACE, "MQTraceMaxFileSize is %d (default = %d)\n",
          mqcMQConnectivity.MQTraceUserDataSize, MQC_MQ_TRACE_USER_DATA_SIZE);

    mqcMQConnectivity.enableReadAhead = ism_common_getIntConfig("MQConnectivity.EnableReadAhead",
                                                                MQC_ENABLE_READ_AHEAD);
    TRACE(MQC_NORMAL_TRACE, "EnableReadAhead is %d (default = %d)\n",
          mqcMQConnectivity.enableReadAhead, MQC_ENABLE_READ_AHEAD);

    overrideValue = ism_common_getIntConfig("MQConnectivity.PurgeXIDS", MQC_PURGE_XIDS);
    if (overrideValue != 0) {
        mqcMQConnectivity.ima.flags |= IMA_PURGE_XIDS;
    }
    TRACE(MQC_NORMAL_TRACE, "PurgeXIDS is %d (default = %d)\n",
          overrideValue, MQC_PURGE_XIDS);

    // Whether NOLOCAL is overridden for all Consume Rules (MessageSight -> MQ)
    overrideValue = ism_common_getIntConfig("MQConnectivity.OverrideNoLocalAtMessageSight", MQC_NO_OVERRIDE);

    if (overrideValue != MQC_NO_OVERRIDE)
    {
        // Override to NOLOCAL *not set*
        if (overrideValue == MQC_OVERRIDE_NOLOCAL_FALSE)
        {
            mqcMQConnectivity.overrideNoLocalAtMessageSight = MQC_OVERRIDE_NOLOCAL_FALSE;
        }
        // Any positive value, override to NOLOCAL *set*
        else if (overrideValue > 0)
        {
            mqcMQConnectivity.overrideNoLocalAtMessageSight = MQC_OVERRIDE_NOLOCAL_TRUE;
        }

        TRACE(MQC_INTERESTING_TRACE, "OverrideNoLocalAtMessageSight is %d. Used %d (default = %d)\n",
              overrideValue, (int)mqcMQConnectivity.overrideNoLocalAtMessageSight, MQC_NO_OVERRIDE);
    }

    /* MOD_EXIT: */
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_loadQM(ism_prop_t * props, const char * propqual, const char * name,
        const char * oldname, mqcQM_t ** ppQM) {
    int len, rc = 0;
    char * ourpropname = NULL;
    const char * pDescription, *pQMName, *pConnectionName, *pChannelName,
            *pSSLCipherSpec;

    const char * pChannelUserName, * pChannelPassword;

    mqcQM_t * pQM, *pThisQM;
    mqcQM_t qm_init = { QM_INIT };

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.qmListMutex);

    /* Are we updating an existing QM ? */
    pThisQM = mqcMQConnectivity.pQMList;

    while (pThisQM) {
        if (oldname) {
            if ((strlen(pThisQM->pName) == strlen(oldname))
                    && (memcmp(pThisQM->pName, oldname, strlen(pThisQM->pName))
                            == 0)) {
                TRACE(MQC_NORMAL_TRACE, "Renaming existing QM '%s'\n", oldname);
                break;
            }
        } else {
            if ((strlen(pThisQM->pName) == strlen(name))
                    && (memcmp(pThisQM->pName, name, strlen(pThisQM->pName))
                            == 0)) {
                TRACE(MQC_NORMAL_TRACE, "Modifying existing QM '%s'\n", name);
                break;
            }
        }
        pThisQM = pThisQM->pNext;
    }

    /* ourpropname length is the longest of ...                       */
    /* QueueManagerConnection.Description.                            */
    /* QueueManagerConnection.QueueManagerName.                       */
    /* QueueManagerConnection.ConnectionName.                         */
    /* QueueManagerConnection.ChannelName.                            */
    len = strlen("QueueManagerConnection.QueueManagerName.") + strlen(propqual)
            + 48;
    ourpropname = mqc_malloc(len);
    if (ourpropname == NULL) {
        mqc_allocError("malloc", len, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }

    sprintf(ourpropname, "QueueManagerConnection.Description.%s", propqual);
    pDescription = ism_common_getStringProperty(props, ourpropname);
    if (pDescription == NULL)
        pDescription = "";

    sprintf(ourpropname, "QueueManagerConnection.QueueManagerName.%s",
            propqual);
    pQMName = ism_common_getStringProperty(props, ourpropname);
    if (pQMName == NULL) {
        if (pThisQM) {
            pQMName = pThisQM->pQMName;
        } else {
            TRACE(MQC_NORMAL_TRACE,
                    "QueueManagerConnection '%s' has no QueueManagerName\n",
                    name);
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }
    }

    sprintf(ourpropname, "QueueManagerConnection.ConnectionName.%s", propqual);
    pConnectionName = ism_common_getStringProperty(props, ourpropname);
    if (pConnectionName == NULL) {
        if (pThisQM) {
            pConnectionName = pThisQM->pConnectionName;
        } else {
            TRACE(MQC_NORMAL_TRACE,
                    "QueueManagerConnection '%s' has no ConnectionName\n",
                    name);
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }
    }

    sprintf(ourpropname, "QueueManagerConnection.ChannelName.%s", propqual);
    pChannelName = ism_common_getStringProperty(props, ourpropname);
    if (pChannelName == NULL) {
        if (pThisQM) {
            pChannelName = pThisQM->pChannelName;
        } else {
            TRACE(MQC_NORMAL_TRACE,
                    "QueueManagerConnection '%s' has no ChannelName\n", name);
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }
    }

    sprintf(ourpropname, "QueueManagerConnection.ChannelUserName.%s", propqual);
    pChannelUserName = ism_common_getStringProperty(props, ourpropname);
    if (pChannelUserName == NULL) {
        if (pThisQM) {
            pChannelUserName = pThisQM->pChannelUserName;
        }
    } else if (*pChannelUserName == '\0') {
        pChannelUserName = NULL;
    }

    sprintf(ourpropname, "QueueManagerConnection.ChannelUserPassword.%s", propqual);
    pChannelPassword = ism_common_getStringProperty(props, ourpropname);
    if (pChannelPassword == NULL) {
        if (pThisQM) {
            pChannelPassword = pThisQM->pChannelPassword;
        }
    } else if (*pChannelPassword == '\0') {
        pChannelPassword = NULL;
    }

    if ( pChannelUserName && !pChannelPassword ) {
    	/* Password can not be NULL or empty */
        TRACE(MQC_NORMAL_TRACE, "QueueManagerConnection '%s' has a ChannelUserName but no ChannelUserPassword\n", name);
        rc = RC_INVALID_CONFIG;
        goto MOD_EXIT;
    }

    /* SSLCipherSpec is optional */
    sprintf(ourpropname, "QueueManagerConnection.SSLCipherSpec.%s", propqual);
    pSSLCipherSpec = ism_common_getStringProperty(props, ourpropname);
    if (pSSLCipherSpec == NULL) {
        if (pThisQM) {
            pSSLCipherSpec = pThisQM->pSSLCipherSpec;
        }
    }

    pQM = mqc_malloc(sizeof(mqcQM_t));
    if (pQM == NULL) {
        mqc_allocError("malloc", sizeof(mqcQM_t), errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }
    memcpy(pQM, &qm_init, sizeof(mqcQM_t));
    pQM->BatchSize = mqcMQConnectivity.batchSize;

    pQM->pName = mqc_malloc(strlen(name) + 1);
    if (pQM->pName == NULL) {
        mqc_allocError("malloc", strlen(name) + 1, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(pQM->pName, name);

    pQM->pDescription = mqc_malloc(strlen(pDescription) + 1);
    if (pQM->pDescription == NULL) {
        mqc_allocError("malloc", strlen(pDescription) + 1, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(pQM->pDescription, pDescription);

    pQM->lengthOfQMName = strlen(pQMName);

    pQM->pQMName = mqc_malloc((pQM->lengthOfQMName) + 1);
    if (pQM->pQMName == NULL) {
        mqc_allocError("malloc", (pQM->lengthOfQMName) + 1, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(pQM->pQMName, pQMName);

    pQM->pConnectionName = mqc_malloc(strlen(pConnectionName) + 1);
    if (pQM->pConnectionName == NULL) {
        mqc_allocError("malloc", strlen(pConnectionName) + 1, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(pQM->pConnectionName, pConnectionName);

    pQM->pChannelName = mqc_malloc(strlen(pChannelName) + 1);
    if (pQM->pChannelName == NULL) {
        mqc_allocError("malloc", strlen(pChannelName) + 1, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(pQM->pChannelName, pChannelName);

    if (pChannelUserName) {
        pQM->pChannelUserName = mqc_malloc(strlen(pChannelUserName) + 1);
        if (pQM->pChannelUserName == NULL) {
            mqc_allocError("malloc", strlen(pChannelUserName) + 1, errno);
            rc = RC_MQC_DISABLE;
            goto MOD_EXIT;
        }
        strcpy(pQM->pChannelUserName, pChannelUserName);
    }

    if (pChannelPassword) {
        pQM->pChannelPassword = mqc_malloc(strlen(pChannelPassword) + 1);
        if (pQM->pChannelPassword == NULL) {
            mqc_allocError("malloc", strlen(pChannelPassword) + 1, errno);
            rc = RC_MQC_DISABLE;
            goto MOD_EXIT;
        }
        strcpy(pQM->pChannelPassword, pChannelPassword);
    }

    if (pSSLCipherSpec && strlen(pSSLCipherSpec)) {
        pQM->pSSLCipherSpec = mqc_malloc(strlen(pSSLCipherSpec) + 1);
        if (pQM->pSSLCipherSpec == NULL) {
            mqc_allocError("malloc", strlen(pSSLCipherSpec) + 1, errno);
            rc = RC_MQC_DISABLE;
            goto MOD_EXIT;
        }
        strcpy(pQM->pSSLCipherSpec, pSSLCipherSpec);
    }

    *ppQM = pQM;

    MOD_EXIT:
    pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);

    if (ourpropname)
        mqc_free(ourpropname);
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_loadQMs(mqcQM_t ** ppQMList) {
    int i, rc = 0;
    const char * propname, *propqual, *name;
    mqcQM_t * pQM;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.qmListMutex);

    for (i = 0;
            ism_common_getPropertyIndex(mqcMQConnectivity.props, i, &propname)
                    == 0; i++) {
        if ((strlen(propname) >= strlen("QueueManagerConnection.Name."))
                && (memcmp(propname, "QueueManagerConnection.Name.",
                        strlen("QueueManagerConnection.Name.")) == 0)) {
            propqual = propname + strlen("QueueManagerConnection.Name.");
            name = ism_common_getStringProperty(mqcMQConnectivity.props,
                    propname);

            rc = mqc_loadQM(mqcMQConnectivity.props, propqual, name,
            NULL, /* oldname */
            &pQM);
            if (rc == RC_INVALID_CONFIG) {
                TRACE(MQC_NORMAL_TRACE,
                        "QueueManagerConnection '%s' configuration ignored\n",
                        name);
                rc = 0;
                continue;
            }
            if (rc)
                goto MOD_EXIT;

            /* Add qm to start of qm list */
            pQM->pNext = *ppQMList;
            *ppQMList = pQM;
        }
    }

    MOD_EXIT:
    pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

/*
 * Check whether the specified topic string is valid for either publish or subscribe.
 *
 * @param[in]     topicString  Null-terminated topic string to check.
 * @param[in]     forPublish   Verifying for publish, if not, subscribe.
 *
 * @remark The string is considered valid for publish unless it contains a
 *         wildcard/multicard character, or has more than iettMAX_TOPIC_DEPTH (32)
 *         substrings.
 *
 *         The string is considered valid for subscribe unless it has more than
 *         iettMAX_TOPIC_DEPTH (32) substrings.
 *
 *         Note: This was originally copied from iett_validateTopicString inside
 *               the IMA engine - if the rules for IMA change, this will need to
 *               change too.
 *
 * @return true if the topic string is considered valid, otherwise false.
 */
bool mqc_validateIMATopicString(const char *topicString, const int forPublish,
        int32_t *pLevelCount) {
    bool valid = true;
    int32_t curSubstringCount = 0;
    const char *curPos = topicString;
    const char *prevPos = curPos;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    do {
        char curChar = *curPos;

        if (curChar == '/' || curChar == '\0') {
            if ((curPos == prevPos + 1) && forPublish) {
                if (*prevPos == '#' || *prevPos == '+') {
                    TRACE(9, "Topic for publish contains wildcards\n");
                    valid = false;
                    break;
                }
            }

            curSubstringCount++;

            if (curChar == '\0') {
                if (curSubstringCount > 32) /* This should be == iettMAX_TOPIC_DEPTH */
                {
                    TRACE(9, "Topic substring count (%d) exceeds max (%d)\n",
                            curSubstringCount, 32);
                    valid = false;
                }
                break;
            } else {
                prevPos = ++curPos;
            }
        } else {
            curPos++;
        }
    } while (1);

    if (pLevelCount != NULL) {
        *pLevelCount = curSubstringCount;
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "curSubstringCount = %d, valid = %d\n",
            __func__, curSubstringCount, valid);

    return valid;
}

int mqc_loadRule(ism_prop_t * props, const char * propqual, const char * name,
        const char * oldname, mqcRule_t ** ppRule) {
    int len, rc = 0;
    int qmConnectionLength = 0;
    char * ourpropname = NULL;
    char * pComma = NULL;
    const char * pFrom = NULL;
    char * pTarget = NULL;
    int i = 0;
    size_t fromLength = 0;
    int sourceLength = 0;
    const char * pDescription, *pQueueManagerConnection, *pDestination;
    char * pSource;
    const char * pSourceIn;
    const char * pRetainedMessages = NULL;
    int RuleType, Enabled, MaxMessages, SubLevel;
    mqcRetainedType_t RetainedMessages = mqcRetained_None;
    mqcRule_t * pRule = NULL;
    mqcRule_t rule_init = { RULE_INIT };
    bool valid = true, setSource = true, setDestination = true;
    bool decrementCounter = false;
    mqcRule_t * pThisRule;
    int32_t levelCount = 0;
    bool protectedUpdate = false;

    /* We create a new mqcRule_t structure to contain the values      */
    /* supplied by the admin component. If this is an update to an    */
    /* existing rule we use the existing mqcRule_t block to provide   */
    /* any property value that is not explicitly set by the admin     */
    /* component.                                                     */

    /* Most properties can only be changed when the rule is disabled, */
    /* however, two - RetainedMessages and MaxMessages - can be       */
    /* altered while the rule is enabled. Therefore while processing  */
    /* property changes we check whether any of them come from the    */
    /* group that requires the rule to be disabled (ie most of them)  */
    /* so that we can enforce that restriction later. Any time we     */
    /* find one of the restricted ones we set protectedUpdate to true */

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);

    /* Are we updating an existing rule ? */
    pThisRule = mqcMQConnectivity.pRuleList;

    while (pThisRule) {
        if (oldname) {
            if ((strlen(pThisRule->pName) == strlen(oldname))
                    && (memcmp(pThisRule->pName, oldname,
                            strlen(pThisRule->pName)) == 0)) {
                TRACE(MQC_NORMAL_TRACE, "Renaming existing rule '%s'\n",
                        oldname);
                break;
            }
        } else {
            if ((strlen(pThisRule->pName) == strlen(name))
                    && (memcmp(pThisRule->pName, name, strlen(pThisRule->pName))
                            == 0)) {
                TRACE(MQC_NORMAL_TRACE, "Modifying existing rule '%s'\n", name);
                break;
            }
        }

        pThisRule = pThisRule->pNext;
    }

    /* ourpropname length is the longest of ...                       */
    /* DestinationMappingRule.Description.                            */
    /* DestinationMappingRule.QueueManagerConnection.                 */
    /* DestinationMappingRule.RuleType.                               */
    /* DestinationMappingRule.Source.                                 */
    /* DestinationMappingRule.Destination.                            */
    /* DestinationMappingRule.Enabled.                                */
    /* DestinationMappingRule.MaxMessages.                            */
    /* DestinationMappingRule.RetainedMessages.                       */
    /* DestinationMappingRule.SubLevel.                               */
    len = strlen("DestinationMappingRule.QueueManagerConnection.")
            + strlen(propqual) + 1;
    ourpropname = mqc_malloc(len);
    if (ourpropname == NULL) {
        mqc_allocError("malloc", len, errno);
        rc = RC_MQC_DISABLE;
        goto MOD_EXIT;
    }

    sprintf(ourpropname, "DestinationMappingRule.Description.%s", propqual);
    pDescription = ism_common_getStringProperty(props, ourpropname);
    if (pDescription == NULL)
        pDescription = "";

    sprintf(ourpropname, "DestinationMappingRule.QueueManagerConnection.%s",
            propqual);
    pQueueManagerConnection = ism_common_getStringProperty(props, ourpropname);
    if (pQueueManagerConnection == NULL) {
        if (pThisRule) {
            pQueueManagerConnection = pThisRule->pQueueManagerConnection;
        } else {
            TRACE(MQC_NORMAL_TRACE,
                    "DestinationMappingRule '%s' has no QueueManagerConnection\n",
                    name);
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }
    } else {
        if (!pThisRule || strcmp(pThisRule->pQueueManagerConnection, pQueueManagerConnection) != 0) {
            protectedUpdate = true;
        }
    }

    sprintf(ourpropname, "DestinationMappingRule.RuleType.%s", propqual);
    RuleType = ism_common_getIntProperty(props, ourpropname, -1);
    if (RuleType == -1) {
        if (pThisRule) {
            RuleType = pThisRule->RuleType;
        } else {
            TRACE(MQC_NORMAL_TRACE,
                    "DestinationMappingRule '%s' has no RuleType\n", name);
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }
    } else {
        if (!pThisRule || pThisRule->RuleType != RuleType) {
            protectedUpdate = true;
        }
    }

    sprintf(ourpropname, "DestinationMappingRule.Source.%s", propqual);
    pSourceIn = ism_common_getStringProperty(props, ourpropname);
    if (pSourceIn == NULL) {
        /* The admin component did not provide a value for source string but  */
        /* if this is an update to an existing rule we can use the value in   */
        /* the existing rule block.                                           */

        if (pThisRule != NULL) {
            setSource = false;

            if (pThisRule->sourceEndOffset != 0) {
                sourceLength = pThisRule->sourceEndOffset;
            } else {
                sourceLength = strlen(pThisRule->pSource);
            }

            TRACE(MQC_NORMAL_TRACE, "pThisRule -> sourceEndOffset = %d\n",
                    pThisRule->sourceEndOffset);

            /* Allow two extra bytes in case we have to add a wildcard later  */
            TRACE(MQC_NORMAL_TRACE, "2139: sourceLength = %d\n", sourceLength);
            pSource = mqc_malloc(sourceLength + 2 + 1);
            if (pSource == NULL) {
                mqc_allocError("malloc", sourceLength + 2 + 1, errno);
                rc = RC_MQC_DISABLE;
                goto MOD_EXIT;
            }

            memset(pSource, 0, sourceLength + 2 + 1);
            strncpy(pSource, pThisRule->pSource, sourceLength);

            /* However, it is possible that the source had been modified to   */
            /* append a wildcard, and the updated version of the rule may no  */
            /* longer need that (eg if the rule type has changed) so we       */
            /* remove anything that we may have added previously and update   */
            /* the stored value of the source string length.                  */

            if (pThisRule->sourceEndOffset != 0) {
                pSource[pThisRule->sourceEndOffset] = 0;
            }
        } else {
            TRACE(MQC_NORMAL_TRACE,
                    "DestinationMappingRule '%s' has no Source\n", name);
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }
    } else {
        /* The admin component provided a source string so we make a local    */
        /* copy of it. Allow two extra bytes in case we have to add a         */
        /* wildcard later.                                                    */
        TRACE(MQC_NORMAL_TRACE, "pSourceIn = '%s'\n", pSourceIn);
        sourceLength = strlen(pSourceIn);
        TRACE(MQC_NORMAL_TRACE, "2176: sourceLength = %d\n", sourceLength);
        pSource = mqc_malloc(sourceLength + 2 + 1);
        if (pSource == NULL) {
            mqc_allocError("malloc", sourceLength + 2 + 1, errno);
            rc = RC_MQC_DISABLE;
            goto MOD_EXIT;
        }
        strcpy(pSource, pSourceIn);

        protectedUpdate = true;

        if (pThisRule != NULL) {
            int compareLength;
            if (pThisRule->sourceEndOffset != 0) {
                compareLength = pThisRule->sourceEndOffset;
            } else {
                compareLength = sourceLength;
            }
            if (strncmp(pThisRule->pSource, pSource, compareLength) == 0) {
                protectedUpdate = false;
            }
        }
    }

    sprintf(ourpropname, "DestinationMappingRule.Destination.%s", propqual);
    pDestination = ism_common_getStringProperty(props, ourpropname);
    if (pDestination == NULL) {
        if (pThisRule) {
            setDestination = false;
            pDestination = pThisRule->pDestination;
        } else {
            TRACE(MQC_NORMAL_TRACE,
                    "DestinationMappingRule '%s' has no Destination\n", name);
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }
    } else {
        if (!pThisRule || strcmp(pThisRule->pDestination, pDestination) != 0) {
            protectedUpdate = true;
        }
    }

    sprintf(ourpropname, "DestinationMappingRule.Enabled.%s", propqual);
    Enabled = ism_common_getIntProperty(props, ourpropname, -1);
    if (Enabled == -1) {
        if (pThisRule) {
            Enabled = pThisRule->Enabled;
        } else {
            TRACE(MQC_NORMAL_TRACE,
                    "DestinationMappingRule '%s' has no Enabled\n", name);
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }
    }

    sprintf(ourpropname, "DestinationMappingRule.MaxMessages.%s", propqual);
    MaxMessages = ism_common_getIntProperty(props, ourpropname, -1);
    if (MaxMessages == -1) {
        if (pThisRule) {
            MaxMessages = pThisRule->MaxMessages;
        } else {
            TRACE(MQC_NORMAL_TRACE,
                    "DestinationMappingRule '%s' has no MaxMessages\n", name);
            rc = RC_INVALID_CONFIG;
            goto MOD_EXIT;
        }
    }

    sprintf(ourpropname, "DestinationMappingRule.RetainedMessages.%s",
            propqual);
    pRetainedMessages = ism_common_getStringProperty(props, ourpropname);
    if (pRetainedMessages == NULL) {
        TRACE(MQC_NORMAL_TRACE,
                "DestinationMappingRule '%s' returned NULL for pRetainedMessages\n",
                name);
        if (pThisRule) {
            RetainedMessages = pThisRule->RetainedMessages;
        } else {
            /* This is not an error - we must be running post-upgrade */
            /* with a config file that never had the property, so     */
            /* NULL means "None".                                     */
            TRACE(MQC_NORMAL_TRACE,
                    "DestinationMappingRule '%s' using default of None for RetainedMessages property\n",
                    name);
        }
    } else {
        TRACE(MQC_NORMAL_TRACE,
                "DestinationMappingRule '%s' has RetainedMessages = %s\n", name,
                pRetainedMessages);
        if (!strcmp(pRetainedMessages, "All")) {
            RetainedMessages = mqcRetained_All;
        }
    }

    sprintf(ourpropname, "DestinationMappingRule.SubLevel.%s", propqual);
    SubLevel = ism_common_getIntProperty(props, ourpropname, -1);
    if (SubLevel == -1) {
        if (pThisRule) {
            SubLevel = pThisRule->SubLevel;
        } else {
            SubLevel = 1;
            TRACE(MQC_NORMAL_TRACE,
                    "DestinationMappingRule '%s' using default SubLevel %d.\n", name, SubLevel);
        }
    } else {
        if (!pThisRule || pThisRule->SubLevel != SubLevel) {
            protectedUpdate = true;
        }
    }

    pRule = mqc_malloc(sizeof(mqcRule_t));
    if (pRule == NULL) {
        mqc_allocError("malloc", sizeof(mqcRule_t), errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }

    memcpy(pRule, &rule_init, sizeof(mqcRule_t));

    pRule->RuleType = RuleType;

    if (protectedUpdate) {
        pRule->flags |= RULE_PROTECTED_UPDATE;
    }

    if (!pThisRule) {
        /* We are adding a new rule so increment the count. */
        if (mqcMQConnectivity.ruleCount == mqcMQConnectivity.ruleLimit) {
            TRACE(MQC_NORMAL_TRACE,
                    "Maximum number of rules (%d) already reached.\n",
                    mqcMQConnectivity.ruleLimit);
            rc = RC_TOO_MANY_RULES;
            goto MOD_EXIT;
        } else {
            mqcMQConnectivity.ruleCount++;
            pRule->flags |= RULE_COUNTED;
            TRACE(MQC_NORMAL_TRACE, "ruleCount incremented to %d.\n",
                    mqcMQConnectivity.ruleCount);

            /* If something goes wrong after this such that the rule  */
            /* is not ultimately defined then we must decrement the   */
            /* overall count of rules.                                */
            decrementCounter = true;
        }
    }

    /* Validate IMA topic */
    if (IMA_TOPIC_CONSUME_RULE(pRule) && setSource) {
        valid = mqc_validateIMATopicString(pSource,
        false, &levelCount); /* subscribe */
    } else if (IMA_TOPIC_PRODUCE_RULE(pRule) && setDestination) {
        valid = mqc_validateIMATopicString(pDestination,
        true, &levelCount); /* publish */
    }

    TRACE(MQC_NORMAL_TRACE, "levelCount = %d.\n", levelCount);

    if (valid == false) {
        rc = RC_INVALID_TOPIC;
        goto MOD_EXIT;
    }

    pRule->pName = mqc_malloc(strlen(name) + 1);
    if (pRule->pName == NULL) {
        mqc_allocError("malloc", strlen(name) + 1, errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(pRule->pName, name);

    pRule->pDescription = mqc_malloc(strlen(pDescription) + 1);
    if (pRule->pDescription == NULL) {
        mqc_allocError("malloc", strlen(pDescription) + 1, errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }
    strcpy(pRule->pDescription, pDescription);

    /* We allow the default number of connections per queue manager   */
    /* to be more than one by replicating the appearance of each QM   */
    /* in the supplied list as often as required by the default. For  */
    /* example, if the user supplies QM1,QM2 and the default number   */
    /* of connections is 3 then we generate QM1,QM1,QM1,QM2,QM2,QM2   */

    qmConnectionLength = strlen(pQueueManagerConnection) + 2;
    qmConnectionLength *= mqcMQConnectivity.defaultConnectionCount;
    pRule->pQueueManagerConnection = mqc_malloc(qmConnectionLength);
    if (pRule->pQueueManagerConnection == NULL) {
        mqc_allocError("malloc", strlen(pQueueManagerConnection) + 1, errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }

    pTarget = pRule->pQueueManagerConnection;
    pFrom = pQueueManagerConnection;

    do {
        pComma = strchr(pFrom, ','); /* pComma is NULL if no comma found. */

        if (pComma != NULL) {
            fromLength = ((ptrdiff_t)pComma)-((ptrdiff_t)pFrom);
        } else {
            fromLength = strlen(pFrom);
        }

        for (i = 0; i < mqcMQConnectivity.defaultConnectionCount; i++) {
            memcpy(pTarget, pFrom, fromLength);
            pTarget += fromLength;
            *pTarget = ',';
            pTarget++;
        }

        pFrom += fromLength + 1;

    } while (pComma != NULL);

    /* Remove the trailing comma */
    pTarget--;
    *pTarget = 0;

    TRACE(MQC_NORMAL_TRACE, "2423: pSource = %s.\n", pSource);

    pRule->pSource = pSource;

    if (WILD_CONSUME_RULE(pRule)) {
        TRACE(MQC_NORMAL_TRACE,
                "2429: Changing pRule -> sourceEndOffset from %d to %d.\n",
                pRule->sourceEndOffset, sourceLength);
        pRule->sourceEndOffset = sourceLength;

        if ((pRule->pSource)[sourceLength - 1] == '/') {
            strcat(pRule->pSource, "#");
        } else {
            strcat(pRule->pSource, "/#");
            levelCount++;
            if (levelCount > 32) /* This should be == iettMAX_TOPIC_DEPTH */
            {
                TRACE(MQC_NORMAL_TRACE,
                        "Appending wildcard to topic string exceeds max.\n");
                rc = RC_INVALID_TOPIC;
                goto MOD_EXIT;
            }
        }
    }

    /* ToDo */
    /* We must propagate the clean up of a re-used source string to the destination */
    /* string as well - meaning the same process of keeping a reference to where    */
    /* the null terminator used to be before we appended anything, then using that  */
    /* to clean up if the rule is updated. However, looking at the code below, we   */
    /* observe that the destination code does not use the technique to append       */
    /* either a /# or a # depending on whether the user supplied a trailing / so we */
    /* need to add that too.                                                        */

    if (setDestination && WILD_PRODUCE_RULE(pRule)) {
        pRule->pDestination = mqc_malloc(strlen(pDestination) + 2 + 1);
        if (pRule->pDestination == NULL) {
            mqc_allocError("malloc", strlen(pDestination) + 2 + 1, errno);
            rc = RC_MQC_DISABLE;
            goto MOD_EXIT;
        }
        sprintf(pRule->pDestination, "%s/#", pDestination);
    } else {
        pRule->pDestination = mqc_malloc(strlen(pDestination) + 1);
        if (pRule->pDestination == NULL) {
            mqc_allocError("malloc", strlen(pDestination) + 1, errno);
            rc = RC_MQC_DISABLE;
            goto MOD_EXIT;
        }
        strcpy(pRule->pDestination, pDestination);
    }

    pRule->Enabled = Enabled;
    pRule->MaxMessages = MaxMessages;
    pRule->RetainedMessages = RetainedMessages;
    pRule->SubLevel = SubLevel;

    // Allow CONSUME rules to override the use of NOLOCAL.
    if (IMA_CONSUME_RULE(pRule) && (mqcMQConnectivity.overrideNoLocalAtMessageSight != MQC_NO_OVERRIDE))
    {
        if (mqcMQConnectivity.overrideNoLocalAtMessageSight == MQC_OVERRIDE_NOLOCAL_FALSE)
        {
            pRule->flags &= ~RULE_USE_NOLOCAL;
        }
        else
        {
            pRule->flags |= RULE_USE_NOLOCAL;
        }
    }

    TRACE(MQC_NORMAL_TRACE, "pRule -> flags set to %X.\n", pRule->flags);

    *ppRule = pRule;

    MOD_EXIT:

    if ((rc != 0) && (pRule != NULL)) {
        mqc_freeRule(pRule, decrementCounter);
    }

    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

    if (ourpropname)
        mqc_free(ourpropname);
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_loadRules(mqcRule_t ** ppRuleList, mqcQM_t * pQMList) {
    int i, rc = 0;
    const char * propname, *propqual, *name;
    mqcRule_t * pRule;

    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    for (i = 0;
            ism_common_getPropertyIndex(mqcMQConnectivity.props, i, &propname)
                    == 0; i++) {
        if ((strlen(propname) >= strlen("DestinationMappingRule.Name."))
                && (memcmp(propname, "DestinationMappingRule.Name.",
                        strlen("DestinationMappingRule.Name.")) == 0)) {
            propqual = propname + strlen("DestinationMappingRule.Name.");
            name = ism_common_getStringProperty(mqcMQConnectivity.props,
                    propname);

            rc = mqc_loadRule(mqcMQConnectivity.props, propqual, name,
            NULL, /* oldname */
            &pRule);
            if ((rc == RC_INVALID_CONFIG) || (rc == RC_INVALID_TOPIC)) {
                TRACE(MQC_NORMAL_TRACE,
                        "DestinationMappingRule '%s' configuration ignored\n",
                        name);
                rc = 0;
                continue;
            }

            if (rc)
                goto MOD_EXIT;

            /* Add our rule QueueManagerConnection(s) */
            rc = mqc_addRuleQMs(pRule);

            if (rc == RC_INVALID_CONFIG) {
                TRACE(MQC_NORMAL_TRACE,
                        "DestinationMappingRule '%s' configuration ignored\n",
                        name);
                rc = 0;
                continue;
            }

            if (rc)
                break;

            /* Add rule to start of rule list */
            pRule->pNext = *ppRuleList;
            *ppRuleList = pRule;
        }
    }

    MOD_EXIT:
    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

void mqc_printIMA(void) {
    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    printf("IMA:\n");
    printf("  StrucId:       '%.4s'\n", mqcMQConnectivity.ima.StrucId);
    printf("  Protocol:      '%s'\n", mqcMQConnectivity.ima.pIMAProtocol);
    printf("  Address:       '%s'\n", mqcMQConnectivity.ima.pIMAAddress);
    printf("  Port:          '%d'\n", mqcMQConnectivity.ima.IMAPort);
    printf("  ClientID:      '%s'\n", mqcMQConnectivity.ima.pIMAClientID);
    printf("  LastQMIndex:   '%d'\n", mqcMQConnectivity.lastQMIndex);
    printf("  LastRuleIndex: '%d'\n", mqcMQConnectivity.lastRuleIndex);

    if (mqcMQConnectivity.flags & MQC_RECONNECT) {
        printf("  State:         'Reconnect'\n");
        printf("  Error:         %d '%s' '%s'\n",
                mqcMQConnectivity.ima.errorMsgId,
                mqcMQConnectivity.ima.errorInsert[0],
                mqcMQConnectivity.ima.errorInsert[1]);
    } else
        printf("  State:         'Enabled'\n");

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, 0);
}

void mqc_printQMs(void) {
    mqcQM_t * pQM;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.qmListMutex);

    pQM = mqcMQConnectivity.pQMList;

    while (pQM) {
        printf("QM:\n");
        printf("  StrucId:        '%.4s'\n", pQM->StrucId);
        printf("  Name:           '%s'\n", pQM->pName);
        printf("  Index:          '%d'\n", pQM->index);
        printf("  Description:    '%s'\n", pQM->pDescription);
        printf("  QMName:         '%s'\n", pQM->pQMName);
        printf("  ConnectionName: '%s'\n", pQM->pConnectionName);
        printf("  ChannelName:    '%s'\n", pQM->pChannelName);
//        printf("  ChannelUserName:    '%s'\n", pQM -> pChannelUserName);
//        printf("  ChannelPassword:    '%s'\n", pQM -> pChannelPassword);
        printf("  BatchSize:      '%d'\n", pQM->BatchSize);
        printf("  SSLCipherSpec:  '%s'\n", pQM->pSSLCipherSpec);

        pQM = pQM->pNext;
    }

    pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, 0);
}

void mqc_printRules(void) {
    mqcRule_t * pRule;
    mqcRuleQM_t * pRuleQM;
    char * pRuleType;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);

    pRule = mqcMQConnectivity.pRuleList;

    while (pRule) {
        switch (pRule->RuleType) {
        case mqcRULE_FROM_IMATOPIC_TO_MQQUEUE:
            pRuleType = "IMA Topic to MQ Queue";
            break;
        case mqcRULE_FROM_IMATOPIC_TO_MQTOPIC:
            pRuleType = "IMA Topic to MQ Topic";
            break;
        case mqcRULE_FROM_MQQUEUE_TO_IMATOPIC:
            pRuleType = "MQ Queue to IMA Topic";
            break;
        case mqcRULE_FROM_MQTOPIC_TO_IMATOPIC:
            pRuleType = "MQ Topic to IMA Topic";
            break;
        case mqcRULE_FROM_IMAWILDTOPIC_TO_MQQUEUE:
            pRuleType = "IMA Wildcard Topic to MQ Queue";
            break;
        case mqcRULE_FROM_IMAWILDTOPIC_TO_MQTOPIC:
            pRuleType = "IMA Wildcard Topic to MQ Topic";
            break;
        case mqcRULE_FROM_IMAWILDTOPIC_TO_MQWILDTOPIC:
            pRuleType = "IMA Wildcard Topic to MQ Wildcard Topic";
            break;
        case mqcRULE_FROM_MQWILDTOPIC_TO_IMATOPIC:
            pRuleType = "MQ Wildcard Topic to IMA Topic";
            break;
        case mqcRULE_FROM_MQWILDTOPIC_TO_IMAWILDTOPIC:
            pRuleType = "MQ Wildcard Topic to IMA Wildcard Topic";
            break;
        case mqcRULE_FROM_IMAQUEUE_TO_MQQUEUE:
            pRuleType = "IMA Queue to MQ Queue";
            break;
        case mqcRULE_FROM_IMAQUEUE_TO_MQTOPIC:
            pRuleType = "IMA Queue to MQ Topic";
            break;
        case mqcRULE_FROM_MQQUEUE_TO_IMAQUEUE:
            pRuleType = "MQ Queue to IMA Queue";
            break;
        case mqcRULE_FROM_MQTOPIC_TO_IMAQUEUE:
            pRuleType = "MQ Topic to IMA Queue";
            break;
        case mqcRULE_FROM_MQWILDTOPIC_TO_IMAQUEUE:
            pRuleType = "IMA Queue to MQ Topic";
            break;
        default:
            pRuleType = "Unknown";
            break;
        }

        printf("\n%s:\n", pRuleType);
        printf("  StrucId:           '%.4s'\n", pRule->StrucId);
        printf("  Name:              '%s'\n", pRule->pName);
        printf("  Index:             '%d'\n", pRule->index);
        printf("  Description:       '%s'\n", pRule->pDescription);
        printf("  QM:                '%s'\n", pRule->pQueueManagerConnection);
        printf("  Source:            '%s'\n", pRule->pSource);
        printf("  Destination:       '%s'\n", pRule->pDestination);
        printf("  Enabled:           '%d'\n", pRule->Enabled);
        printf("  MaxMessages:       '%d'\n", pRule->MaxMessages);
        printf("  RetainedMessages:  '%d'\n", pRule->RetainedMessages);
        printf("  SubLevel:          '%d'\n", pRule->SubLevel);

        if (pRule->flags & RULE_RECONNECT) {
            printf("  State:       'Reconnect'\n");
            printf("  Error:       %d '%s' '%s'\n", pRule->errorMsgId,
                    pRule->errorInsert[0], pRule->errorInsert[1]);
        } else if (pRule->flags & RULE_DISABLED) {
            printf("  State:       'Disabled'\n");
            printf("  Error:       %d '%s' '%s'\n", pRule->errorMsgId,
                    pRule->errorInsert[0], pRule->errorInsert[1]);
        } else
            printf("  State:       'Enabled'\n");

        /* Print out each queue manager ... */
        pthread_mutex_lock(&pRule->ruleQMListMutex);
        pRuleQM = pRule->pRuleQMList;

        while (pRuleQM) {
            printf("\n");
            printf("    StrucId: '%.4s'\n", pRuleQM->StrucId);
            printf("    Name:    '%s'\n", pRuleQM->pQM->pName);
            printf("    QMName:  '%s'\n", pRuleQM->pQM->pQMName);
            printf("    SyncQ:   '%s'\n", pRuleQM->syncQName);
            printf("    V.R.M.F: '%d.%d.%d.%d\n", pRuleQM->vrmf[0],
                    pRuleQM->vrmf[1], pRuleQM->vrmf[2], pRuleQM->vrmf[3]);
            printf("    Index:   '%d.%d'\n", pRule->index, pRuleQM->pQM->index);

            if (pRuleQM->flags & RULEQM_RECONNECT) {
                printf("    State:   'Reconnect'\n");
                printf("    Error:   %d '%s' '%s' '%s' '%s'\n",
                        pRuleQM->errorMsgId, pRuleQM->errorInsert[0],
                        pRuleQM->errorInsert[1], pRuleQM->errorInsert[2],
                        pRuleQM->errorInsert[3]);
            } else if (pRuleQM->flags & RULEQM_DISABLED) {
                printf("    State:   'Disabled'\n");
                printf("    Error:   %d '%s' '%s' '%s' '%s'\n",
                        pRuleQM->errorMsgId, pRuleQM->errorInsert[0],
                        pRuleQM->errorInsert[1], pRuleQM->errorInsert[2],
                        pRuleQM->errorInsert[3]);
            } else
                printf("    State:   'Enabled'\n");

            pRuleQM = pRuleQM->pNext;
        }
        pthread_mutex_unlock(&pRule->ruleQMListMutex);

        pRule = pRule->pNext;
    }

    printf("\nruleCount = %d\n", mqcMQConnectivity.ruleCount);

    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, 0);
}

int mqc_freeQM(mqcQM_t * pQM) {
    int rc = 0;
    int qmindex = pQM->index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d\n", __func__, qmindex);

    if (pQM->pName)
        mqc_free(pQM->pName);
    if (pQM->pDescription)
        mqc_free(pQM->pDescription);
    if (pQM->pQMName)
        mqc_free(pQM->pQMName);
    if (pQM->pConnectionName)
        mqc_free(pQM->pConnectionName);
    if (pQM->pChannelName)
        mqc_free(pQM->pChannelName);
    if (pQM->pSSLCipherSpec)
        mqc_free(pQM->pSSLCipherSpec);
    if (pQM->pChannelUserName)
        mqc_free(pQM->pChannelUserName);
    if (pQM->pChannelPassword)
        mqc_free(pQM->pChannelPassword);
    mqc_free(pQM);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d rc=%d\n", __func__, qmindex, rc);
    return rc;
}

int mqc_freeRule(mqcRule_t * pRule, bool decrementCounter) {
    int rc = 0;
    int ruleindex = pRule->index;
    mqcRuleQM_t * pRuleQM, *pNextRuleQM;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d\n", __func__, ruleindex);

    if (pRule->pName)
        mqc_free(pRule->pName);
    if (pRule->pDescription)
        mqc_free(pRule->pDescription);
    if (pRule->pQueueManagerConnection)
        mqc_free(pRule->pQueueManagerConnection);
    if (pRule->pSource)
        mqc_free(pRule->pSource);
    if (pRule->pDestination)
        mqc_free(pRule->pDestination);

    pRuleQM = pRule->pRuleQMList;

    while (pRuleQM) {
        pNextRuleQM = pRuleQM->pNext;

        mqc_freeRuleQM(pRuleQM);

        pRuleQM = pNextRuleQM;
    }

    if (decrementCounter) {
        mqcMQConnectivity.ruleCount--;
        TRACE(MQC_NORMAL_TRACE, "ruleCount decremented to %d.\n",
                mqcMQConnectivity.ruleCount);
    }

    mqc_free(pRule);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d rc=%d\n", __func__, ruleindex, rc);
    return rc;
}

int mqc_freeRuleQM(mqcRuleQM_t * pRuleQM) {
    int rc = 0;
    mqcRule_t * pRule = pRuleQM->pRule;
    mqcQM_t * pQM = pRuleQM->pQM;
    int ruleindex = pRule->index;
    int qmindex = pQM->index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex,
            qmindex);

    ism_common_freeAllocBuffer(&pRuleQM->buf);

    mqc_free(pRuleQM);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex,
            qmindex, rc);
    return rc;
}

int mqc_testQueueManagerConnection(const char *pInputQMName,
        const char *pConnectionName, const char *pChannelName,
        const char *pSSLCipherSpec, const char *pUserName, const char *pUserPassword) {
    int rc = 0;

    MQCNO cno = { MQCNO_DEFAULT };
    MQCD cd = { MQCD_CLIENT_CONN_DEFAULT };
    MQSCO sco = { MQSCO_DEFAULT };
    MQCSP csp = {MQCSP_DEFAULT};

    MQHCONN hConn = MQHC_UNUSABLE_HCONN;
    MQLONG CompCode;
    MQLONG Reason;
    PMQCHAR pQMName = (PMQCHAR) pInputQMName;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if ((pInputQMName == NULL) || (pConnectionName == NULL)
            || (pChannelName == NULL)) {
        rc = ISMRC_NullArgument;
        goto MOD_EXIT;
    }

    cno.Version = MQCNO_CURRENT_VERSION;
    cno.ClientConnPtr = &cd;

    cd.Version = MQCD_CURRENT_VERSION;
    cd.MaxMsgLength = 104857600;
    strncpy(cd.ChannelName, pChannelName, MQ_CHANNEL_NAME_LENGTH);
    strncpy(cd.ConnectionName, pConnectionName, MQ_CONN_NAME_LENGTH);

    if (pSSLCipherSpec != NULL) {
        strncpy(cd.SSLCipherSpec, pSSLCipherSpec,
        MQ_SSL_CIPHER_SPEC_LENGTH);

        sco.Version = MQSCO_CURRENT_VERSION;
        snprintf(sco.KeyRepository,
        MQ_SSL_KEY_REPOSITORY_LENGTH, "%s/mqconnectivity",
                ism_common_getStringConfig("MQCertificateDir"));
        cno.SSLConfigPtr = &sco;

        TRACE(MQC_MQAPI_TRACE, "SSLCIPH '%.*s' SSLKEYR '%.*s'\n",
        MQ_SSL_CIPHER_SPEC_LENGTH, cd.SSLCipherSpec,
        MQ_SSL_KEY_REPOSITORY_LENGTH, sco.KeyRepository);
    }

    if (pUserName)
    {
        //add channel user name and password to csp
        csp.AuthenticationType = MQCSP_AUTH_USER_ID_AND_PWD;
        csp.CSPUserIdPtr = (char *)pUserName;
        csp.CSPUserIdLength = strlen(pUserName);
        csp.CSPPasswordPtr = (char *)pUserPassword;
        csp.CSPPasswordLength = pUserPassword?strlen(pUserPassword):0;
        cno.SecurityParmsPtr = &csp;
    }

    MQCONNX(pQMName, &cno, &hConn, &CompCode, &Reason);

    TRACE(MQC_MQAPI_TRACE,
            "MQCONNX completed with CompCode = %d, Reason = %d\n", CompCode,
            Reason);

    /* MQCONNX can return a large number of reason codes, many of     */
    /* which are not actually possible in our case. The following     */
    /* switch statements provide a mapping from MQ to ISM return      */
    /* codes.                                                         */

    /* ISMRC_SSLError.                                                */
    /* There is a known short-coming in MQ that when a connection is  */
    /* rejected because of a problem with SSL, then you get no        */
    /* details about what the problem might be. So, we can report     */
    /* only that the connection attempt failed because of an issue    */
    /* with SSL.                                                      */

    /* ISMRC_ChannelError                                             */
    /* The SVRCONN channel (ie channel name) either doesn't exist or  */
    /* isn't available for some reason.                               */

    /* ISMRC_HostError                                                */
    /* The host machine is not responding. Therefore we don't know    */
    /* whether the queue manager exists or not. (This really means    */
    /* that we didn't get a response from the MQ listener.)           */

    /* ISMRC_QMError                                                  */
    /* The queue manager exists but is not responding. (Meaning that  */
    /* the MQ listener on the remote machine did respond but the      */
    /* queue manager didn't.                                          */

    /* ISMRC_CDError                                                  */
    /* The MQCD is in some way faulty. Given our use of it that means */
    /* either the channel name or connection name is faulty.          */

    /* ISMRC_QMNameError                                              */
    /* There is no queue manager of the given name on the host.       */

    /* ISMRC_NotAuthorized                                            */
    /* The operation is not authorized. Typically, the security setup */
    /* for the queue manager has not been implemented as described in */
    /* the documentation.                                             */

    switch (CompCode) {
    case MQCC_OK:
        rc = ISMRC_OK;
        break;

    case MQCC_WARNING:
        rc = ISMRC_UnableToConnectToQM;
        break;

    default:
    case MQCC_FAILED:
        switch (Reason) {
        case MQRC_CD_ERROR:
            rc = ISMRC_CDError;
            break;

        case MQRC_Q_MGR_NOT_AVAILABLE:
            rc = ISMRC_QMError;
            break;

        case MQRC_HOST_NOT_AVAILABLE:
            rc = ISMRC_HostError;
            break;

        case MQRC_SCO_ERROR:
        case MQRC_SSL_CONFIG_ERROR:
        case MQRC_KEY_REPOSITORY_ERROR:
        case MQRC_SSL_INITIALIZATION_ERROR:
            rc = ISMRC_SSLError;
            break;

        case MQRC_CHANNEL_NOT_AVAILABLE:
        case MQRC_CHANNEL_CONFIG_ERROR:
        case MQRC_UNKNOWN_CHANNEL_NAME:
            rc = ISMRC_ChannelError;
            break;

        case MQRC_Q_MGR_NAME_ERROR:
            rc = ISMRC_QMNameError;
            break;

        case MQRC_NOT_AUTHORIZED:
            rc = ISMRC_NotAuthorizedAtQM;
            break;

        default:
            rc = ISMRC_UnableToConnectToQM;
            break;
        }
        break;
    }

    MQDISC(&hConn, &CompCode, &Reason);

    TRACE(MQC_MQAPI_TRACE, "MQDISC completed with CompCode = %d, Reason = %d\n",
            CompCode, Reason);

    MOD_EXIT:

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;

}
