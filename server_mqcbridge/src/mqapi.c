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

#include <mqcInternal.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

#define MQC_FIRST 0x00000001
#define MQC_NEXT  0x00000002
#define MQC_LAST  0x00000004

#define MODEL_SYNC_Q_NAME "SYSTEM.IMA.MODEL.SYNC"

/* Function prototypes */
int mqc_PCF(mqcRuleQM_t * pRuleQM, MQHCONN hConn,
            PMQLONG pAdminMsgLen, PPMQBYTE ppAdminMsg, int flags);

int mqc_deleteSyncQueue(mqcRuleQM_t * pRuleQM, MQHCONN hConn, char * name);

int mqc_inquireTopicStatus(mqcRuleQM_t * pRuleQM, MQHCONN hConn);
int mqc_inquireTopicUseDLQ(mqcRuleQM_t * pRuleQM, MQHCONN hConn);

int mqc_inquireDLQ(mqcRuleQM_t * pRuleQM, MQHCONN hConn);

/* Functions */
int mqc_connectedQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    /* RC_MQConnected = 7024 */
    LOG(INFO, MQConnectivity, 7024,
        "%-s%s%s",
        "Destination mapping rule {0}. Queue manager connection {1}. Connected to WebSphere MQ queue manager {2}.",
        pRule -> pName, pQM -> pName, pQM -> pQMName);

    pRuleQM -> flags |= RULEQM_CONNECTED;
    pRuleQM -> flags &= ~RULEQM_STARTING;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_deleteSubscription(mqcRuleQM_t * pRuleQM, MQHCONN hConn, char * subname)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    MQLONG adminMsgLen = 0;
    MQBYTE *pAdminMsg = NULL;

    MQCFH *pPCFHeader = NULL;
    MQCFH *pPCFReply = NULL;
    MQCFST *pPCFSubscriptionName = NULL;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d '%s'\n", __func__, ruleindex, qmindex, subname);

    adminMsgLen = MQCFH_STRUC_LENGTH +
                  MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(subname));

    pAdminMsg = mqc_malloc(adminMsgLen);
    if (pAdminMsg == NULL)
    {
        mqc_ruleQMAllocError(pRuleQM, "malloc", adminMsgLen, errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }
    memset(pAdminMsg, 0, adminMsgLen);

    pPCFHeader = (MQCFH *) pAdminMsg;
    pPCFSubscriptionName = (MQCFST *) (pAdminMsg + MQCFH_STRUC_LENGTH);

    if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
    {
        pPCFHeader->Type = MQCFT_COMMAND_XR;
    }
    else
    {
        pPCFHeader->Type = MQCFT_COMMAND;
    }

    pPCFHeader->StrucLength = MQCFH_STRUC_LENGTH;
    pPCFHeader->Version = MQCFH_VERSION_3;
    pPCFHeader->Command = MQCMD_DELETE_SUBSCRIPTION;
    pPCFHeader->MsgSeqNumber = 1;
    pPCFHeader->Control = MQCFC_LAST;
    pPCFHeader->ParameterCount = 0;

    /* Setup parameter block: Subscription name */
    pPCFSubscriptionName->Type = MQCFT_STRING;
    pPCFSubscriptionName->StrucLength = MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(subname));
    pPCFSubscriptionName->Parameter = MQCACF_SUB_NAME;
    pPCFSubscriptionName->CodedCharSetId = MQCCSI_DEFAULT;
    pPCFSubscriptionName->StringLength = strlen(subname);
    memcpy(pPCFSubscriptionName->String, subname, strlen(subname));
    pPCFHeader->ParameterCount++;

    rc = mqc_PCF(pRuleQM, hConn, &adminMsgLen, &pAdminMsg, MQC_FIRST | MQC_LAST);
    if (rc) goto MOD_EXIT;

    pPCFReply = (MQCFH *) pAdminMsg;

    TRACE(MQC_MQAPI_TRACE,
          "Delete Subscription '%s' '%s' CompCode(%d) Reason(%d)\n",
          pQM -> pQMName, subname, pPCFReply->CompCode, pPCFReply->Reason);

    if (pPCFReply->CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQCMD_DELETE_SUBSCRIPTION",
                       pRule -> pDestination, pPCFReply->Reason);

        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

MOD_EXIT:

    if (pAdminMsg) mqc_free(pAdminMsg);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_inquireSubscriptions(mqcRuleQM_t * pRuleQM, MQHCONN hConn)
{
    int i, rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    char subname[MQ_SUB_NAME_LENGTH + 1];

    MQLONG adminMsgLen = 0;
    MQBYTE *pAdminMsg = NULL;

    MQCFH *pPCFHeader = NULL;
    MQCFH *pPCFReply = NULL;
    MQCFST *pPCFSubscriptionName = NULL;
    PMQLONG pType, pStrucLength, pParameter;
    char * pChar;
    bool closePCF = false;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    /* MQ z/OS defect 27137.                                              */
    /* We really want to use a subname "SYSTEM.IMA.<objectIdentifier>.*". */
    /* However we are limited to 20 characters (including wildcard).      */
    sprintf(subname, "SYSTEM.IMA.%.8s*", mqcMQConnectivity.objectIdentifier);

    adminMsgLen = MQCFH_STRUC_LENGTH +
                  MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(subname));

    pAdminMsg = mqc_malloc(adminMsgLen);
    if (pAdminMsg == NULL)
    {
        mqc_ruleQMAllocError(pRuleQM, "malloc", adminMsgLen, errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }
    memset(pAdminMsg, 0, adminMsgLen);

    pPCFHeader = (MQCFH *) pAdminMsg;
    pPCFSubscriptionName = (MQCFST *) (pAdminMsg + MQCFH_STRUC_LENGTH);

    if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
    {
        pPCFHeader->Type = MQCFT_COMMAND_XR;
    }
    else
    {
        pPCFHeader->Type = MQCFT_COMMAND;
    }

    pPCFHeader->StrucLength = MQCFH_STRUC_LENGTH;
    pPCFHeader->Version = MQCFH_VERSION_3;
    pPCFHeader->Command = MQCMD_INQUIRE_SUBSCRIPTION;
    pPCFHeader->MsgSeqNumber = 1;
    pPCFHeader->Control = MQCFC_LAST;
    pPCFHeader->ParameterCount = 0;

    /* Setup parameter block: Subscription name */
    pPCFSubscriptionName->Type = MQCFT_STRING;
    pPCFSubscriptionName->StrucLength = MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(subname));
    pPCFSubscriptionName->Parameter = MQCACF_SUB_NAME;
    pPCFSubscriptionName->CodedCharSetId = MQCCSI_DEFAULT;
    pPCFSubscriptionName->StringLength = strlen(subname);
    memcpy(pPCFSubscriptionName->String, subname, strlen(subname));
    pPCFHeader->ParameterCount++;

    rc = mqc_PCF(pRuleQM, hConn, &adminMsgLen, &pAdminMsg, MQC_FIRST);
    if (rc) goto MOD_EXIT;

    closePCF = true;

    pPCFReply = (MQCFH *) pAdminMsg;

    TRACE(MQC_MQAPI_TRACE,
          "Inquire Subscription '%s' '%s' CompCode(%d) Reason(%d)\n",
          pQM -> pQMName, subname, pPCFReply->CompCode, pPCFReply->Reason);

    if ((pPCFReply->Reason == MQRC_NO_SUBSCRIPTION) ||
        (pPCFReply->Reason == MQRCCF_NONE_FOUND)) goto MOD_EXIT;

    if (pPCFReply->CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQCMD_INQUIRE_SUBSCRIPTION",
                       pRule -> pDestination, pPCFReply->Reason);

        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    while (1)
    {
        pChar = (char *)(pPCFReply + 1);

        for (i = 0; i < pPCFReply->ParameterCount; i++)
        {
            pType = (PMQLONG) pChar;
            pStrucLength = (pType + 1);
            pParameter = (pStrucLength + 1);

            if (*pParameter == MQCACF_SUB_NAME)
            {
                pPCFSubscriptionName = (PMQCFST) pChar;

                /* MQ z/OS defect 27137.                              */
                /* Check the returned subscription name matches.      */
                sprintf(subname, "SYSTEM.IMA.%.12s.", mqcMQConnectivity.objectIdentifier);

                if ((pPCFSubscriptionName->StringLength >= strlen(subname)) &&
                    (memcmp(pPCFSubscriptionName->String, subname, strlen(subname)) == 0))
                {
                    sprintf(subname, "%.*s",
                            pPCFSubscriptionName->StringLength,
                            pPCFSubscriptionName->String);

                    rc = mqc_deleteSubscription(pRuleQM, hConn, subname);
                    if (rc) goto MOD_EXIT;
                }
            }

            pChar += *pStrucLength;
        }

        if (pPCFReply->Control == MQCFC_NOT_LAST)
        {
            rc = mqc_PCF(pRuleQM, hConn, &adminMsgLen, &pAdminMsg, MQC_NEXT);
            if (rc) goto MOD_EXIT;

            pPCFReply = (MQCFH *) pAdminMsg;

            continue;
        }

        break;
    }

MOD_EXIT:

    if (closePCF)
    {
        mqc_PCF(pRuleQM, hConn, 0, NULL, MQC_LAST);
    }

    if (pAdminMsg) mqc_free(pAdminMsg);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_inquireSyncQueues(mqcRuleQM_t * pRuleQM, MQHCONN hConn)
{
    int i, j, rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    char name[MQ_Q_NAME_LENGTH + 1];

    MQLONG adminMsgLen = 0;
    MQBYTE *pAdminMsg = NULL;

    MQCFH *pPCFHeader = NULL;
    MQCFH *pPCFReply = NULL;
    MQCFST *pPCFQueueName = NULL;
    MQCFSL *pPCFQueueNames = NULL;
    PMQLONG pType, pStrucLength, pParameter;
    char * pChar;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    sprintf(name, "SYSTEM.IMA.%.12s.*", mqcMQConnectivity.objectIdentifier);

    adminMsgLen = MQCFH_STRUC_LENGTH +
                  MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(name));

    pAdminMsg = mqc_malloc(adminMsgLen);
    if (pAdminMsg == NULL)
    {
        mqc_ruleQMAllocError(pRuleQM, "malloc", adminMsgLen, errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }
    memset(pAdminMsg, 0, adminMsgLen);

    pPCFHeader = (MQCFH *) pAdminMsg;
    pPCFQueueName = (MQCFST *) (pAdminMsg + MQCFH_STRUC_LENGTH);

    if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
    {
        pPCFHeader->Type = MQCFT_COMMAND_XR;
    }
    else
    {
        pPCFHeader->Type = MQCFT_COMMAND;
    }

    pPCFHeader->StrucLength = MQCFH_STRUC_LENGTH;
    pPCFHeader->Version = MQCFH_VERSION_3;
    pPCFHeader->Command = MQCMD_INQUIRE_Q_NAMES;
    pPCFHeader->MsgSeqNumber = 1;
    pPCFHeader->Control = MQCFC_LAST;
    pPCFHeader->ParameterCount = 0;

    /* Setup parameter block: Queue name */
    pPCFQueueName->Type = MQCFT_STRING;
    pPCFQueueName->StrucLength = MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(name));
    pPCFQueueName->Parameter = MQCA_Q_NAME;
    pPCFQueueName->CodedCharSetId = MQCCSI_DEFAULT;
    pPCFQueueName->StringLength = strlen(name);
    memcpy(pPCFQueueName->String, name, strlen(name));
    pPCFHeader->ParameterCount++;

    rc = mqc_PCF(pRuleQM, hConn, &adminMsgLen, &pAdminMsg, MQC_FIRST | MQC_LAST);
    if (rc) goto MOD_EXIT;

    pPCFReply = (MQCFH *) pAdminMsg;

    TRACE(MQC_MQAPI_TRACE,
          "Inquire Queue '%s' '%s' CompCode(%d) Reason(%d)\n",
          pQM -> pQMName, name, pPCFReply->CompCode, pPCFReply->Reason);

    if (pPCFReply->CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQCMD_INQUIRE_Q_NAMES",
                       pRule -> pDestination, pPCFReply->Reason);

        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    pChar = (char *)(pPCFReply + 1);

    for (i = 0; i < pPCFReply->ParameterCount; i++)
    {
        pType = (PMQLONG) pChar;
        pStrucLength = (pType + 1);
        pParameter = (pStrucLength + 1);

        if (*pParameter == MQCACF_Q_NAMES)
        {
            pPCFQueueNames = (PMQCFSL) pChar;

            for (j = 0; j < pPCFQueueNames->Count; j++)
            {
                sprintf(name, "%.*s", pPCFQueueNames->StringLength,
                        &pPCFQueueNames->Strings[j * pPCFQueueNames->StringLength]);

                rc = mqc_deleteSyncQueue(pRuleQM, hConn, name);
                if (rc) goto MOD_EXIT;
            }
        }

        pChar += *pStrucLength;
    }

MOD_EXIT:

    if (pAdminMsg) mqc_free(pAdminMsg);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_coldStart(mqcRuleQM_t * pRuleQM, MQHCONN hConn)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    mqcQM_t * pThisQM;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    pthread_mutex_lock(&mqcMQConnectivity.coldStartMutex);

    if (pQM -> flags & QM_COLDSTART)
    {
        /* Multiple QueueManagerConnection objects can point to the   */
        /* same queue manager. We need to ensure we haven't already   */
        /* done the cold start processing on this queue manager.      */
        pthread_mutex_lock(&mqcMQConnectivity.qmListMutex);

        pThisQM = mqcMQConnectivity.pQMList;

        while (pThisQM)
        {
            if (!(pThisQM -> flags & QM_COLDSTART) &&
                (memcmp(pQM -> QMgrIdentifier, pThisQM -> QMgrIdentifier,
                        sizeof(pQM -> QMgrIdentifier)) == 0))
            {
                TRACE(MQC_NORMAL_TRACE,
                      "Already cold started '%.*s'\n",
                      MQ_Q_MGR_IDENTIFIER_LENGTH, pQM -> QMgrIdentifier);

                pQM -> flags &= ~QM_COLDSTART;

                pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);
                goto MOD_EXIT;
            }

            pThisQM = pThisQM -> pNext;
        }

        pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);

        rc = mqc_inquireSubscriptions(pRuleQM, hConn);
        if (rc) goto MOD_EXIT;

        rc = mqc_inquireSyncQueues(pRuleQM, hConn);
        if (rc) goto MOD_EXIT;

        pQM -> flags &= ~QM_COLDSTART;
    }

MOD_EXIT:
    pthread_mutex_unlock(&mqcMQConnectivity.coldStartMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_inquireVersionQM(mqcRuleQM_t * pRuleQM,
                         MQHCONN hConn)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    MQLONG CompCode, Reason;
    MQOD od = { MQOD_DEFAULT };
    MQLONG selectors[2] = { MQIA_PLATFORM, MQCA_Q_MGR_IDENTIFIER };
    MQLONG version_selectors[1] = { MQCA_VERSION };
    MQHOBJ hObj = MQHO_UNUSABLE_HOBJ;
    MQCHAR version[MQ_VERSION_LENGTH];
    char num[3];
    MQLONG platformType = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    od.ObjectType = MQOT_Q_MGR;

    MQOPEN(hConn,
           &od,
           MQOO_INQUIRE,
           &hObj,
           &CompCode,
           &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQOPEN CompCode(%d) Reason(%d)\n", CompCode, Reason);

    if (CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQOPEN", pQM -> pQMName, Reason);
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    MQINQ(hConn,
          hObj,
          2,                    /* SelectorCount */
          selectors,
          1,                    /* IntAttrCount */
          &platformType,        /* IntAttrs */
          MQ_Q_MGR_IDENTIFIER_LENGTH,
                                /* CharAttrLength */
          pQM -> QMgrIdentifier,
                                /* CharAttrs */
          &CompCode,
          &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQINQ CompCode(%d) Reason(%d)\n", CompCode, Reason);

    TRACE(MQC_MQAPI_TRACE,
          "MQINQ platformType = %d QMgrIdentifier = '%.*s'\n",
          platformType, MQ_Q_MGR_IDENTIFIER_LENGTH, pQM -> QMgrIdentifier);

    pQM -> platformType = platformType;

    // Look for performance enhancements available on distributed platform
    if (platformType != MQPL_ZOS)
    {
        MQINQ(hConn,
                hObj,
                1,
                version_selectors,
                0,
                NULL,
                MQ_VERSION_LENGTH,
                version,
                &CompCode,
                &Reason);
        TRACE(MQC_MQAPI_TRACE,
                "MQINQ CompCode(%d) Reason(%d)\n", CompCode, Reason);

        /* MQCA_VERSION supported at 7.1 and above */
        if (Reason == MQRC_NONE)
        {

            num[2] = 0;
            strncpy(num, &version[0], 2);
            pRuleQM -> vrmf[0] = atoi(num);
            strncpy(num, &version[2], 2);
            pRuleQM -> vrmf[1] = atoi(num);
            strncpy(num, &version[4], 2);
            pRuleQM -> vrmf[2] = atoi(num);
            strncpy(num, &version[6], 2);
            pRuleQM -> vrmf[3] = atoi(num);

            TRACE(MQC_MQAPI_TRACE,
                    "Version '%.*s' %d.%d.%d.%d\n", MQ_VERSION_LENGTH, version,
                    pRuleQM -> vrmf[0], pRuleQM -> vrmf[1], pRuleQM -> vrmf[2], pRuleQM -> vrmf[3]);

            /**********************************************************/
            /* Implicit XAEnd support:                                */
            /* =======================                                */
            /* RTC MQ tasks 20604 / 20730. CMVC feature 158931.       */
            /*                                                        */
            /* Sharing conversations:                                 */
            /* ======================                                 */
            /* RTC MQ defects 12935 / 19068. CMVC defect 158893.      */
            /*                                                        */
            /* Read ahead:                                            */
            /* ===========                                            */
            /* RTC MQ defect 30129.                                   */
            /*                                                        */
            /* MCAST topic status:                                    */
            /* ===================                                    */
            /* RTC MQ feature 1222.                                   */
            /**********************************************************/
            if ((pRuleQM -> vrmf[0] > 7) ||
                ((pRuleQM -> vrmf[0] == 7) &&
                 (pRuleQM -> vrmf[1] == 1) &&
                 (pRuleQM -> vrmf[2] == 0) &&
                 (pRuleQM -> vrmf[3] >= 3)) ||
                ((pRuleQM -> vrmf[0] == 7) &&
                 (pRuleQM -> vrmf[1] == 5) &&
                 (pRuleQM -> vrmf[2] == 0) &&
                 (pRuleQM -> vrmf[3] >= 1)))
            {
                pRuleQM -> flags |= RULEQM_IMPLICIT_XA_END |
                                    RULEQM_SHARING_CONVERSATIONS;
            }

            if ((pRuleQM -> vrmf[0] > 7) ||
                (mqcMQConnectivity.enableReadAhead))
            {
                pRuleQM -> flags |= RULEQM_READ_AHEAD;
            }

            if (pRuleQM -> vrmf[0] > 7)
            {
                pRuleQM -> flags |= RULEQM_MCAST_TOPIC_STATUS;
            }
        }
    }

MOD_EXIT:

    if (hObj != MQHO_UNUSABLE_HOBJ)
    {
        MQCLOSE(hConn,
                &hObj,
                MQCO_NONE,
                &CompCode,
                &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQCLOSE CompCode(%d) Reason(%d)\n", CompCode, Reason);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_connectPublishQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;
    MQCD cd = {MQCD_CLIENT_CONN_DEFAULT};
    MQCNO cno = {MQCNO_DEFAULT};
    MQSCO sco = {MQSCO_DEFAULT};
    MQCSP csp = {MQCSP_DEFAULT};

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    cno.Version = MQCNO_CURRENT_VERSION;
    cno.ClientConnPtr = &cd;

    cd.Version = MQCD_CURRENT_VERSION;
    cd.MaxMsgLength = 104857600;
    strncpy(cd.ChannelName, pQM -> pChannelName, MQ_CHANNEL_NAME_LENGTH);
    strncpy(cd.ConnectionName, pQM -> pConnectionName, MQ_CONN_NAME_LENGTH);

    if (pQM->pChannelUserName)
    {
        csp.AuthenticationType = MQCSP_AUTH_USER_ID_AND_PWD;
        //add channel user name and password to csp
        csp.CSPUserIdPtr = pQM->pChannelUserName;
        csp.CSPUserIdLength = strlen(pQM->pChannelUserName);
        //do something to decrypt the password.
        csp.CSPPasswordPtr = pQM->pChannelPassword;
        csp.CSPPasswordLength = strlen(pQM->pChannelPassword);
        cno.SecurityParmsPtr = &csp;
    }

    if (pQM -> pSSLCipherSpec)
    {
        strncpy(cd.SSLCipherSpec, pQM -> pSSLCipherSpec, MQ_SSL_CIPHER_SPEC_LENGTH);

        sco.Version = MQSCO_CURRENT_VERSION;
        snprintf(sco.KeyRepository, MQ_SSL_KEY_REPOSITORY_LENGTH,
                 "%s/mqconnectivity", ism_common_getStringConfig("MQCertificateDir"));
        cno.SSLConfigPtr = &sco;

        TRACE(MQC_MQAPI_TRACE,
              "SSLCIPH '%.*s' SSLKEYR '%.*s'\n",
              MQ_SSL_CIPHER_SPEC_LENGTH, cd.SSLCipherSpec,
              MQ_SSL_KEY_REPOSITORY_LENGTH, sco.KeyRepository);
    }

    /* Set the Product ID prior to the MQCONN so that the queue manager knows */
    /* who we are.                                                            */
    mqc_SPISetProdId(pRuleQM,
                     &CompCode,
                     &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "mqc_SPISetProdId CompCode(%d) Reason(%d)\n",
          CompCode, Reason);

    MQCONNX(pQM -> pQMName,
            &cno,
            &pRuleQM -> hPubConn,
            &CompCode,
            &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQCONNX(hPubConn) '%s' CompCode(%d) Reason(%d)\n",
          pQM -> pQMName, CompCode, Reason);

    if (CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQCONNXError, "MQCONNX", pQM -> pQMName, Reason);
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    /* Inquire the queue manager version "VVRRMMFF" */
    rc = mqc_inquireVersionQM(pRuleQM, pRuleQM -> hPubConn);
    if (rc) goto MOD_EXIT;

    rc = mqc_inquireDLQ(pRuleQM, pRuleQM -> hPubConn);
    if (rc) goto MOD_EXIT;

    /* We are now connected  */
    mqc_connectedQM(pRuleQM);

    /* Set unusable message handle (in case of rule reconnect) */
    pRuleQM -> hMsg = MQHM_UNUSABLE_HMSG;

    /* Has the store been cold started ? */
    rc = mqc_coldStart(pRuleQM, pRuleQM -> hPubConn);
    if (rc) goto MOD_EXIT;

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_connectSubscribeQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;
    MQCD cd = {MQCD_CLIENT_CONN_DEFAULT};
    MQCNO cno = {MQCNO_DEFAULT};
    MQSCO sco = {MQSCO_DEFAULT};
    MQCSP csp = {MQCSP_DEFAULT};

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    cno.Version = MQCNO_CURRENT_VERSION;
    cno.ClientConnPtr = &cd;

    cd.Version = MQCD_CURRENT_VERSION;
    cd.MaxMsgLength = 104857600;
    strncpy(cd.ChannelName, pQM -> pChannelName, MQ_CHANNEL_NAME_LENGTH);
    strncpy(cd.ConnectionName, pQM -> pConnectionName, MQ_CONN_NAME_LENGTH);

    if (pQM->pChannelUserName)
    {
        csp.AuthenticationType = MQCSP_AUTH_USER_ID_AND_PWD;
        //add channel user name and password to csp
        csp.CSPUserIdPtr = pQM->pChannelUserName;
        csp.CSPUserIdLength = strlen(pQM->pChannelUserName);
        //do something to decrypt the password.
        csp.CSPPasswordPtr = pQM->pChannelPassword;
        csp.CSPPasswordLength = strlen(pQM->pChannelPassword);
        cno.SecurityParmsPtr = &csp;
    }

    if (pQM -> pSSLCipherSpec)
    {
        strncpy(cd.SSLCipherSpec, pQM -> pSSLCipherSpec, MQ_SSL_CIPHER_SPEC_LENGTH);

        sco.Version = MQSCO_CURRENT_VERSION;
        snprintf(sco.KeyRepository, MQ_SSL_KEY_REPOSITORY_LENGTH,
                 "%s/mqconnectivity", ism_common_getStringConfig("MQCertificateDir"));
        cno.SSLConfigPtr = &sco;

        TRACE(MQC_MQAPI_TRACE,
              "SSLCIPH '%.*s' SSLKEYR '%.*s'\n",
              MQ_SSL_CIPHER_SPEC_LENGTH, cd.SSLCipherSpec,
              MQ_SSL_KEY_REPOSITORY_LENGTH, sco.KeyRepository);
    }

    /* Set the Product ID prior to the MQCONN so that the queue manager knows */
    /* who we are.                                                            */
    mqc_SPISetProdId(pRuleQM,
                     &CompCode,
                     &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "mqc_SPISetProdId CompCode(%d) Reason(%d)\n",
          CompCode, Reason);

    while (1)
    {
        MQCONNX(pQM -> pQMName,
                &cno,
                &pRuleQM -> hSubConn,
                &CompCode,
                &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQCONNX(hSubConn) '%s' CompCode(%d) Reason(%d)\n",
              pQM -> pQMName, CompCode, Reason);

        if (CompCode == MQCC_FAILED)
        {
            mqc_MQAPIError(pRuleQM, RC_MQCONNXError, "MQCONNX", pQM -> pQMName, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }

        memcpy(pRuleQM -> subConnectionId, cno.ConnectionId, MQ_CONNECTION_ID_LENGTH);

        /* Inquire the queue manager version "VVRRMMFF" */
        rc = mqc_inquireVersionQM(pRuleQM, pRuleQM -> hSubConn);
        if (rc) goto MOD_EXIT;

        rc = mqc_inquireDLQ(pRuleQM, pRuleQM -> hSubConn);
        if (rc) goto MOD_EXIT;

        /* If we don't support sharing conversations we should        */
        /* reconnect with SharingConversations = 0                    */
        if ((cd.SharingConversations) &&
           !(pRuleQM -> flags & RULEQM_SHARING_CONVERSATIONS))
        {
            MQDISC(&pRuleQM -> hSubConn,
                   &CompCode,
                   &Reason);
            TRACE(MQC_MQAPI_TRACE,
                  "MQDISC(hSubConn) '%s' CompCode(%d) Reason(%d)\n",
                  pQM -> pQMName, CompCode, Reason);

            cd.SharingConversations = 0;
            continue;
        }

        break;
    }

    /* We are now connected  */
    mqc_connectedQM(pRuleQM);

    /* Create message handle */
    rc = mqc_createMessageHandleQM(pRuleQM, pRuleQM -> hSubConn);
    if (rc) goto MOD_EXIT;

    /* Has the store been cold started ? */
    rc = mqc_coldStart(pRuleQM, pRuleQM -> hSubConn);
    if (rc) goto MOD_EXIT;

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_disconnectPublishQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if (pRuleQM -> hPubConn != MQHC_UNUSABLE_HCONN)
    {
        if (pRuleQM -> hMsg != MQHM_UNUSABLE_HMSG)
        {
            mqc_deleteMessageHandleQM(pRuleQM, pRuleQM -> hPubConn, false);
        }

        MQDISC(&pRuleQM -> hPubConn,
               &CompCode,
               &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQDISC(hPutConn) '%s' CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName, CompCode, Reason);
    }

    pRuleQM -> flags &= ~RULEQM_CONNECTED;
    pRuleQM -> flags &= ~RULEQM_STARTING;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_disconnectSubscribeQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if (pRuleQM -> hSubConn != MQHC_UNUSABLE_HCONN)
    {
        if (pRuleQM -> hMsg != MQHM_UNUSABLE_HMSG)
        {
            mqc_deleteMessageHandleQM(pRuleQM, pRuleQM -> hSubConn, false);
        }

        MQDISC(&pRuleQM -> hSubConn,
               &CompCode,
               &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQDISC(hSubConn) '%s' CompCode(%d) Reason(%d)\n",
              pQM -> pQMName, CompCode, Reason);
    }

    pRuleQM -> flags &= ~RULEQM_CONNECTED;
    pRuleQM -> flags &= ~RULEQM_STARTING;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_inquireManagedQueueName(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;
    MQLONG selectors[1] = { MQCA_Q_NAME };
    MQCHAR48 name;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    MQINQ(pRuleQM -> hSubConn,
          pRuleQM -> hObj,
          1,
          selectors,
          0,
          NULL,
          MQ_Q_NAME_LENGTH,
          name,
          &CompCode,
          &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQINQ CompCode(%d) Reason(%d)\n", CompCode, Reason);

    /* Is this a multicast subscription ? */
    if ((Reason == MQRC_SELECTOR_ERROR) &&
        (pRuleQM -> flags & RULEQM_NON_DURABLE))
    {
        goto MOD_EXIT;
    }

    if (CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQOPEN", pQM -> pQMName, Reason);
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    mqcCopyBlankPadToNullPad(pRuleQM -> managedQName, name, MQ_Q_NAME_LENGTH);

    TRACE(MQC_MQAPI_TRACE, "Managed Queue Name '%s'\n", pRuleQM -> managedQName);

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_subscribeQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG Options, CompCode, Reason;
    MQOD od = {MQOD_DEFAULT};
    MQSD sd = {MQSD_DEFAULT};
    concat_alloc_t buf;
    char xbuf[64];

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    memset(&buf, 0, sizeof(buf));
    buf.buf = xbuf;
    buf.len = sizeof(xbuf);

    if (MQ_QUEUE_CONSUME_RULE(pRule))
    {
        strncpy(od.ObjectName, pRule -> pSource, MQ_Q_NAME_LENGTH);

        Options = MQOO_INPUT_AS_Q_DEF;

        Options |= (pRuleQM -> flags & RULEQM_READ_AHEAD) ?
            MQOO_READ_AHEAD : MQOO_NO_READ_AHEAD;

        MQOPEN(pRuleQM -> hSubConn,
               &od,
               Options,
               &pRuleQM -> hObj,
               &CompCode,
               &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQOPEN '%s' '%s' Options(0x%08x) CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName, pRule -> pSource, Options, CompCode, Reason);

        if (CompCode == MQCC_FAILED)
        {
            mqc_MQAPIError(pRuleQM, RC_MQOPENQueueError, "MQOPEN", pRule -> pSource, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }
    }
    else
    {
        sd.Options = MQSO_CREATE  |
                     MQSO_RESUME  |
                     MQSO_MANAGED |
                     MQSO_DURABLE;

        sd.Options |= (pRuleQM -> flags & RULEQM_READ_AHEAD) ?
            MQSO_READ_AHEAD : MQSO_NO_READ_AHEAD;

        sd.ObjectString.VSPtr = pRule -> pSource;
        sd.ObjectString.VSLength = strlen(pRule -> pSource);

        /* Pass in a ResObjectString so we can check that if we have   */
        /* resumed a subscription the topic string is as expected. We  */
        /* allow an additional 100 bytes for reporting in case the     */
        /* subscription name exists with a different topic string.     */
        MQLONG bufSize = strlen(pRule -> pSource) + 100;

        ism_common_allocAllocBuffer(&buf, bufSize + 1, 0);

        sd.ResObjectString.VSPtr     = buf.buf;
        sd.ResObjectString.VSBufSize = bufSize;

        /* SubName */
        /* Error reporting to the UI relies on the subscription name  */
        /* generated here being at most 48 bytes including the null   */
        /* terminator. See mqcStatistics_t in mqcbridgeExternal.h     */
        sprintf(pRuleQM -> subName, "SYSTEM.IMA.%.12s.%.8d.%.8d.SUB",
                mqcMQConnectivity.objectIdentifier, ruleindex, pRuleQM -> index);
        sd.SubName.VSPtr = pRuleQM -> subName;
        sd.SubName.VSLength = strlen(pRuleQM -> subName);

        TRACE(MQC_NORMAL_TRACE, "pRuleQM -> subName '%s'\n", pRuleQM -> subName);

        sd.SubLevel = (MQLONG)pRule->SubLevel;

        while (1)
        {
            MQSUB(pRuleQM -> hSubConn,
                  &sd,
                  &pRuleQM -> hObj,
                  &pRuleQM -> hSub,
                  &CompCode,
                  &Reason);
            TRACE(MQC_MQAPI_TRACE,
                  "MQSUB '%s' '%s' Options(0x%08x) SubLevel(%d) CompCode(%d) Reason(%d)\n",
                  pRuleQM -> pQM -> pQMName, pRule -> pSource, sd.Options, sd.SubLevel, CompCode, Reason);

            /* Is this a multicast or DURSUB(NO) subscription ? */
            if ((Reason == MQRC_DURABILITY_NOT_ALLOWED) &&
               !(pRuleQM -> flags & RULEQM_NON_DURABLE))
            {
                sd.Options &= ~MQSO_DURABLE;
                pRuleQM -> flags |= RULEQM_NON_DURABLE;
                continue;
            }

            break;
        }

        if (CompCode == MQCC_FAILED)
        {
            mqc_MQAPIError(pRuleQM, RC_MQSUBError, "MQSUB", pRule -> pSource, Reason, pRuleQM -> subName);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }

        /* Null terminate ResObjectString */
        ((char *)sd.ResObjectString.VSPtr)[MIN(sd.ResObjectString.VSLength,
                                               sd.ResObjectString.VSBufSize)] = 0;

        TRACE(MQC_MQAPI_TRACE,
              "MQSUB ResObjectString VSLength(%d) VSBufSize(%d) '%s'\n",
              sd.ResObjectString.VSLength, sd.ResObjectString.VSBufSize,
              (char *) sd.ResObjectString.VSPtr);

        /* Have we resumed a subscription with a different topic string ? */
        if ((sd.ResObjectString.VSLength != strlen(pRule -> pSource)) ||
            (memcmp(pRule -> pSource, sd.ResObjectString.VSPtr, strlen(pRule -> pSource))))
        {
            mqc_MQAPIError(pRuleQM, RC_MQSUBTopicError, "MQSUB", pRule -> pSource, Reason,
                           pRuleQM -> subName, sd.ResObjectString.VSPtr);
            rc = RC_RULEQM_DISABLE;
            goto MOD_EXIT;
        }

        /* Inquire managed queue name */
        rc = mqc_inquireManagedQueueName(pRuleQM);
        if (rc) goto MOD_EXIT;

        rc = mqc_inquireTopicUseDLQ(pRuleQM, pRuleQM -> hSubConn);
        if (rc) goto MOD_EXIT;

    }

MOD_EXIT:
    ism_common_freeAllocBuffer(&buf);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_quiesceConsumeQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if ((pRuleQM -> hSubConn != MQHC_UNUSABLE_HCONN) &&
        (pRuleQM -> hObj != MQHO_UNUSABLE_HOBJ))
    {
        MQCLOSE(pRuleQM -> hSubConn,
                &pRuleQM -> hObj,
                MQCO_QUIESCE,
                &CompCode,
                &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQCLOSE(hObj) MQCO_QUIESCE '%s' '%s' CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName, pRule -> pSource, CompCode, Reason);

        if (Reason == MQRC_NONE)
            pRuleQM -> flags |= RULEQM_QUIESCED;
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_unsubscribeQM(mqcRuleQM_t * pRuleQM, bool requested)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG Options, CompCode, Reason;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d %d\n", __func__, ruleindex, qmindex, requested);

    /* Close subscription */
    if ((pRuleQM -> hSubConn != MQHC_UNUSABLE_HCONN) &&
        (pRuleQM -> hSub != MQHO_UNUSABLE_HOBJ))
    {
        Options = (requested) ? MQCO_REMOVE_SUB : MQCO_NONE;

        MQCLOSE(pRuleQM -> hSubConn,
                &pRuleQM -> hSub,
                Options,
                &CompCode,
                &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQCLOSE(hSub) '%s' '%s' Options(%X) CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName,
              pRule -> pSource,
              Options,
              CompCode,
              Reason);

        pRuleQM -> hSub = MQHO_UNUSABLE_HOBJ;
    }
    else
    {
    	if (pRuleQM -> hSubConn != MQHC_UNUSABLE_HCONN)
    	{
            TRACE(MQC_MQAPI_TRACE,
                  "pRuleQM -> hSubConn == MQHC_UNUSABLE_HCONN\n");
    	}
    	if (pRuleQM -> hSub != MQHO_UNUSABLE_HOBJ)
    	{
    		TRACE(MQC_MQAPI_TRACE,
    				"pRuleQM -> hSub == MQHC_UNUSABLE_HOBJ\n");
    	}
    }
    /* Close subscription queue */
    if ((pRuleQM -> hSubConn != MQHC_UNUSABLE_HCONN) &&
        (pRuleQM -> hObj != MQHO_UNUSABLE_HOBJ))
    {
        MQCLOSE(pRuleQM -> hSubConn,
                &pRuleQM -> hObj,
                MQCO_NONE,
                &CompCode,
                &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQCLOSE(hObj) '%s' '%s' CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName, pRule -> pSource, CompCode, Reason);

        pRuleQM -> hObj = MQHO_UNUSABLE_HOBJ;
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_processMQMessage(mqcRuleQM_t * pRuleQM,
                         PMQMD pMD,
                         MQLONG messlen,
                         PMQVOID buffer)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    ismc_message_t * pMessage;
    ismc_session_t * pSess;
    ismc_producer_t * pProd;
    char topic[MQ_TOPIC_STR_LENGTH + 1] = "";
    bool bTransactional;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    /* Is this a transactional message ? */
    bTransactional = ((pMD -> Persistence == MQPER_PERSISTENT) &&
                     !(pRuleQM -> flags & RULEQM_NO_SYNCPOINT)) ? true : false;

    /* Use transacted session for transactional messages */
    pMessage = (bTransactional) ?
        pRuleQM -> pMessage : pRuleQM -> pNonTranMessage;

    rc = mqc_setIMAProperties(pRuleQM, pMessage, pMD, messlen, buffer, topic);
    if (rc) goto MOD_EXIT;

    /* If this rule is publishing to an IMA topic, then check whether the */
    /* rule requires the retain bit to be set.                            */

    if (IMA_TOPIC_PRODUCE_RULE(pRule))
    {
    	if (pRule -> RetainedMessages == mqcRetained_All)
    	{
    		ismc_setRetain(pMessage, 1);
    	}
    	else
    	{
    	    ismc_setRetain(pMessage, 0);
    	}
    }

    if (WILD_IMA_PRODUCE_RULE(pRule))
    {
        /* Check we had an MQTopicString property */
        if (topic[0] == 0)
        {
            mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQINQMP",
                           "MQTopicString", MQRC_PROPERTY_NOT_AVAILABLE);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }

        /* Use transacted session for transactional messages */
        pSess = (bTransactional) ?
            pRuleQM -> pSess : pRuleQM -> pNonTranSess;

        rc = ismc_sendX(pSess, 2, topic, pMessage);
        TRACE(MQC_ISMC_TRACE, "ismc_sendX '%s' rc(%d)\n", topic, rc);

        if (rc)
        {
            rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_sendX", topic);
            goto MOD_EXIT;
        }
    }
    else
    {
        /* Use transacted session for transactional messages */
        pProd = (bTransactional) ?
            pRuleQM -> pProd : pRuleQM -> pNonTranProd;

        rc = ismc_send(pProd, pMessage);
        TRACE(MQC_ISMC_TRACE, "ismc_send '%s' rc(%d)\n", pRule -> pDestination, rc);

        if (rc)
        {
            rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_send", pRule -> pDestination);
            goto MOD_EXIT;
        }
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_openProducerQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;
    MQOD od = {MQOD_DEFAULT};

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if (NONWILD_MQ_PRODUCE_RULE(pRule))
    {
        if (MQ_QUEUE_PRODUCE_RULE(pRule))
        {
            strncpy(od.ObjectName, pRule -> pDestination, MQ_Q_NAME_LENGTH);
        }
        else
        {
            od.ObjectType = MQOT_TOPIC;
            od.Version = MQOD_VERSION_4;

            od.ObjectString.VSPtr = pRule -> pDestination;
            od.ObjectString.VSLength = strlen(pRule -> pDestination);
        }

        MQOPEN(pRuleQM -> hPubConn,
               &od,
               MQOO_OUTPUT,
               &pRuleQM -> hPutObj,
               &CompCode,
               &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQOPEN '%s' '%s' Type(%d) CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName, pRule -> pDestination, od.ObjectType, CompCode, Reason);

        if (CompCode == MQCC_FAILED)
        {
            if (MQ_QUEUE_PRODUCE_RULE(pRule))
            {
                mqc_MQAPIError(pRuleQM, RC_MQOPENQueueError, "MQOPEN", pRule -> pDestination, Reason);
            }
            else
            {
                mqc_MQAPIError(pRuleQM, RC_MQOPENTopicError, "MQOPEN", pRule -> pDestination, Reason);
            }
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }

        /* Is this a multicast topic ? */
        if (MQ_TOPIC_PRODUCE_RULE(pRule))
        {
            rc = mqc_inquireTopicStatus(pRuleQM, pRuleQM -> hPubConn);
            if (rc) goto MOD_EXIT;
        }
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_closeProducerQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if (NONWILD_MQ_PRODUCE_RULE(pRule))
    {
        if ((pRuleQM -> hPubConn != MQHC_UNUSABLE_HCONN) &&
            (pRuleQM -> hPutObj != MQHO_UNUSABLE_HOBJ))
        {
            MQCLOSE(pRuleQM -> hPubConn,
                    &pRuleQM -> hPutObj,
                    MQCO_NONE,
                    &CompCode,
                    &Reason);
            TRACE(MQC_MQAPI_TRACE,
                  "MQCLOSE '%s' '%s' CompCode(%d) Reason(%d)\n",
                  pRuleQM -> pQM -> pQMName, pRule -> pDestination, CompCode, Reason);
        }
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

/* Send PCF command and receive response messages */
int mqc_PCF(mqcRuleQM_t * pRuleQM, MQHCONN hConn,
           PMQLONG pAdminMsgLen, PPMQBYTE ppAdminMsg, int flags)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    MQPMO pmo = {MQPMO_DEFAULT};
    MQGMO gmo = {MQGMO_DEFAULT};
    MQMD md = {MQMD_DEFAULT};
    MQOD replyQueueOD = {MQOD_DEFAULT};
    MQOD cmdQueueOD = {MQOD_DEFAULT};
    MQLONG CompCode, Reason;
    MQLONG adminMsgLen = 0;
    PMQBYTE pAdminMsg = NULL;
    MQHOBJ hObjCmdQ = MQHO_UNUSABLE_HOBJ;
    MQHOBJ hObjReplyQ = MQHO_UNUSABLE_HOBJ;

    char replyQueue[MQ_Q_NAME_LENGTH + 1];

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if (flags & MQC_NEXT)
    {
        hObjCmdQ = pRuleQM -> hObjCmdQ;
        hObjReplyQ = pRuleQM -> hObjReplyQ;
    }

    if (flags & (MQC_FIRST | MQC_NEXT))
    {
        adminMsgLen = *pAdminMsgLen;
        pAdminMsg = *ppAdminMsg;
    }

    if (flags & MQC_FIRST)
    {
        /* Create a tempdyn queue that we */
        /* can use to receive responses from admin commands.         */
        strncpy(replyQueueOD.ObjectName,
                MODEL_REPLY_QUEUE,
                MQ_Q_NAME_LENGTH);
        strncpy(replyQueueOD.DynamicQName,
                "SYSTEM.IMA.PCFQ.*",
                MQ_Q_NAME_LENGTH);

        MQOPEN(hConn,
               &replyQueueOD,
               MQOO_INPUT_SHARED,
               &hObjReplyQ,
               &CompCode,
               &Reason);

        mqcCopyBlankPadToNullPad(replyQueue, replyQueueOD.ObjectName, MQ_Q_NAME_LENGTH);

        TRACE(MQC_MQAPI_TRACE,
              "MQOPEN '%s' '%s' CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName,
              replyQueue,
              CompCode,
              Reason);

        if (CompCode == MQCC_FAILED)
        {
            mqc_MQAPIError(pRuleQM, RC_MQOPENQueueError, "MQOPEN", MODEL_REPLY_QUEUE, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }

        strncpy(cmdQueueOD.ObjectName, COMMAND_QUEUE, MQ_Q_NAME_LENGTH);

        /* Open the queue manager's command queue */
        MQOPEN(hConn,
               &cmdQueueOD,
               MQOO_OUTPUT,
               &hObjCmdQ,
               &CompCode,
               &Reason);

        TRACE(MQC_MQAPI_TRACE,
              "MQOPEN '%s' '%s' CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName, cmdQueueOD.ObjectName, CompCode, Reason);

        if (CompCode == MQCC_FAILED)
        {
            mqc_MQAPIError(pRuleQM, RC_MQOPENQueueError, "MQOPEN", COMMAND_QUEUE, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }

        pmo.Options = MQPMO_NEW_MSG_ID | MQPMO_NEW_CORREL_ID | MQPMO_NO_SYNCPOINT;
        memcpy(md.Format, MQFMT_ADMIN, MQ_FORMAT_LENGTH);
        md.MsgType = MQMT_REQUEST;
        memcpy(md.ReplyToQ, replyQueueOD.ObjectName, MQ_Q_NAME_LENGTH);

        MQPUT(hConn,
              hObjCmdQ,
              &md,
              &pmo,
              adminMsgLen,
              pAdminMsg, 
              &CompCode, 
              &Reason);

        TRACE(MQC_MQAPI_TRACE,
              "MQPUT '%s' '%s' CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName, cmdQueueOD.ObjectName, CompCode, Reason);

        if (CompCode == MQCC_FAILED)
        {
            mqc_MQAPIError(pRuleQM, RC_MQPUTQueueError, "MQPUT", COMMAND_QUEUE, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }
    }

    if (flags & (MQC_FIRST | MQC_NEXT))
    {
        /* Get the response message re-using the original admin msg buffer */
        /* this may mean we can get the response without retrying the get  */

        gmo.Version = MQGMO_VERSION_2;
        gmo.MatchOptions = MQMO_NONE;

        gmo.Options = MQGMO_WAIT | MQGMO_NO_SYNCPOINT | MQGMO_CONVERT;
        gmo.WaitInterval = 20000; /* 20 second limit for waiting     */

        /* If the command server isn't running, we will get a no message     */
        /* available response from this get.                                 */

        MQGET(hConn,
              hObjReplyQ,
              &md,
              &gmo,
              adminMsgLen,
              pAdminMsg,
              &adminMsgLen,
              &CompCode,
              &Reason);

        TRACE(MQC_MQAPI_TRACE,
              "MQGET '%s' '%s' CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName,
              replyQueue,
              CompCode,
              Reason);

        if (Reason == MQRC_TRUNCATED_MSG_FAILED)
        {
            /* Reallocate the buffer and try the get again */

            pAdminMsg = ism_common_realloc(ISM_MEM_PROBE(ism_memory_mqcbridge_misc,7),pAdminMsg, adminMsgLen);
            if (pAdminMsg == NULL)
            {
                mqc_ruleQMAllocError(pRuleQM, "realloc", adminMsgLen, errno);
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }

            md.Encoding = MQENC_NATIVE;
            md.CodedCharSetId = MQCCSI_Q_MGR;

            MQGET(hConn,
                  hObjReplyQ,
                  &md,
                  &gmo,
                  adminMsgLen,
                  pAdminMsg,
                  &adminMsgLen,
                  &CompCode,
                  &Reason);

            TRACE(MQC_MQAPI_TRACE,
                  "MQGET '%s' '%s' CompCode(%d) Reason(%d)\n",
                  pRuleQM -> pQM -> pQMName,
                  replyQueue,
                  CompCode,
                  Reason);
        }

        if (CompCode != MQCC_OK)
        {
            mqc_MQAPIError(pRuleQM, RC_PCFMQGETError, "MQGET", replyQueue, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }
    }

MOD_EXIT:

    if (rc || (flags & MQC_LAST))
    {
        /* Close the queue manager's command queue */
        if ((hConn != MQHC_UNUSABLE_HCONN) &&
            (hObjCmdQ != MQHO_UNUSABLE_HOBJ))
        {
            MQCLOSE(hConn,
                    &hObjCmdQ,
                    MQCO_NONE,
                    &CompCode,
                    &Reason);
    
            TRACE(MQC_MQAPI_TRACE,
                  "MQCLOSE '%s' '%s' CompCode(%d) Reason(%d)\n",
                  pRuleQM -> pQM -> pQMName,
                  COMMAND_QUEUE,
                  CompCode,
                  Reason);
        }
    
        /* Close the temp dyn response queue */
        if ((hConn != MQHC_UNUSABLE_HCONN) &&
            (hObjReplyQ != MQHO_UNUSABLE_HOBJ))
        {
            MQCLOSE(hConn,
                    &hObjReplyQ,
                    MQCO_NONE,
                    &CompCode,
                    &Reason);
    
            TRACE(MQC_MQAPI_TRACE,
                  "MQCLOSE '%s' '%s' CompCode(%d) Reason(%d)\n",
                  pRuleQM -> pQM -> pQMName, replyQueue, CompCode, Reason);
        }
    }

    /* Might we be called again with MQC_NEXT ? */
    if ((rc == 0) && !(flags & (MQC_LAST | MQC_NEXT)))
    {
        pRuleQM -> hObjCmdQ = hObjCmdQ;
        pRuleQM -> hObjReplyQ = hObjReplyQ;
    }

    if (flags & (MQC_FIRST | MQC_NEXT))
    {
        *pAdminMsgLen = adminMsgLen;
        *ppAdminMsg = pAdminMsg;
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

//Version of mqc_createSyncQueue() that relies on a model queue existing (that wasn't originally doc'd) but that
//avoids create privileges in MQ (which are qmgr-wide)
void mqc_createSyncQueueViaModel(mqcRuleQM_t * pRuleQM, MQHCONN hConn, bool *pQueueFound)
{
    MQOD     od = {MQOD_DEFAULT};    /* Object Descriptor             */
    MQHOBJ   hSyncObj;               /* object handle                 */
    MQLONG   openCode;               /* MQOPEN completion code        */
    MQLONG   closeCode;              /* MQCLOSE completion code       */
    MQLONG   openReason;             /* MQOpen reason code            */
    MQLONG   closeReason;            /* MQClose reason code           */
    MQLONG   openOptions;            /* MQOPEN options                */
    MQLONG   closeOptions;           /* MQCLOSE options               */

    openOptions = MQOO_INPUT_SHARED       /* open queue for input          */
                | MQOO_FAIL_IF_QUIESCING; /* but not if MQM stopping       */

    strcpy(od.ObjectName, MODEL_SYNC_Q_NAME);
    strncpy(od.DynamicQName, pRuleQM -> syncQName, MQ_Q_NAME_LENGTH); /* dynamic queue name            */

    MQOPEN(hConn,                   /* connection handle              */
           &od,                     /* object descriptor for queue    */
           openOptions,               /* open options                   */
           &hSyncObj,               /* object handle                  */
           &openCode,               /* completion code                */
           &openReason);                /* reason code                    */

    //Trace expected return codes first:
    if (  openReason == MQRC_NONE
       || openReason == MQRC_UNKNOWN_OBJECT_NAME   //Model doesn't exist - common and expected
       )
    {
        TRACE(MQC_MQAPI_TRACE,
              "Create SyncQ (from Model) As Expected: '%s' '%s' CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName,
              pRuleQM -> syncQName,
              openCode,
              openReason);
    }
    else
    {
        TRACE(MQC_INTERESTING_TRACE,
          "Create SyncQ (from Model) Unexpected Error: '%s' '%s' CompCode(%d) Reason(%d)\n",
          pRuleQM -> pQM -> pQMName,
          pRuleQM -> syncQName,
          openCode,
          openReason);
    }

    if (openCode != MQCC_FAILED)
    {
        *pQueueFound = true;

        //We have the queue open but we don't expect to at this code (as the code expects a PCF create, so close it)
        closeOptions = 0;                   /* no close options              */

        MQCLOSE(hConn,                    /* connection handle             */
                &hSyncObj,                /* object handle                 */
                closeOptions,
                &closeCode,               /* completion code               */
                &closeReason);            /* reason code                   */

        TRACE(MQC_MQAPI_TRACE,
                        "Close SyncQ (from Model) '%s' '%s' CompCode(%d) Reason(%d)\n",
                        pRuleQM -> pQM -> pQMName,
                        pRuleQM -> syncQName,
                        closeCode,
                        closeReason);
    }
}
/* Create a synchronization queue. */
/* Local queue. Persistent messages by default. */
/* Strategy is try opening based on a model queue first (to avoid needing create privileges in MQ (which are qmgr-wide))
 * Assuming we can't open (e.g. because the model queue doesn't exist which is likely as model doc was (is?) not doc'd - fall back
 * to PCF queue
 */
int mqc_createSyncQueue(mqcRuleQM_t * pRuleQM, MQHCONN hConn, bool *pQueueFound)
{
    int rc = 0;
    MQLONG adminMsgLen = 0;
    MQBYTE *pAdminMsg = NULL;
    MQCFH *pPCFHeader = NULL;
    MQCFH *pPCFReply = NULL;
    MQCFST *pPCFQueueNameStr = NULL;
    MQCFIN *pPCFQTypeInt = NULL;
    MQCFIN *pPCFDefPsistInt = NULL;
    MQCFST *pPCFQueueDescriptor = NULL;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%s %s\n", __func__,
            pRuleQM -> pQM -> pQMName,
            pRuleQM -> syncQName);

    *pQueueFound = false;

    mqc_createSyncQueueViaModel(pRuleQM, hConn, pQueueFound);

    if (*pQueueFound == true) {
        //We successfully created the queue based on the model queue, we are done
        goto MOD_EXIT;
    }

    adminMsgLen = MQCFH_STRUC_LENGTH
        + MQCFST_STRUC_LENGTH_FIXED + MQ_Q_NAME_LENGTH /* Queue name */
        + MQCFIN_STRUC_LENGTH +    /* Queue type */
        + MQCFIN_STRUC_LENGTH +     /* Default persistence */
        + MQCFST_STRUC_LENGTH_FIXED + MQ_Q_DESC_LENGTH; /* Queue descriptor */

    pAdminMsg = mqc_malloc(adminMsgLen);
    if (pAdminMsg == NULL)
    {
        mqc_ruleQMAllocError(pRuleQM, "malloc", adminMsgLen, errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }
    memset(pAdminMsg, 0, adminMsgLen);

    pPCFHeader = (MQCFH *) pAdminMsg;
    pPCFQueueNameStr = (MQCFST *) (pAdminMsg + MQCFH_STRUC_LENGTH);
    pPCFQTypeInt =
        (MQCFIN *) ((MQBYTE*) pPCFQueueNameStr + MQCFST_STRUC_LENGTH_FIXED + MQ_Q_NAME_LENGTH);
    pPCFDefPsistInt =
        (MQCFIN *) ((MQBYTE*) pPCFQTypeInt + MQCFIN_STRUC_LENGTH);
    pPCFQueueDescriptor = (MQCFST *) ((MQBYTE*) pPCFDefPsistInt + MQCFIN_STRUC_LENGTH);

    if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
    {
        pPCFHeader->Type = MQCFT_COMMAND_XR;
    }
    else
    {
        pPCFHeader->Type = MQCFT_COMMAND;
    }

    pPCFHeader->StrucLength = MQCFH_STRUC_LENGTH;
    pPCFHeader->Version = MQCFH_VERSION_3;
    pPCFHeader->Command = MQCMD_CREATE_Q;
    pPCFHeader->MsgSeqNumber = 1;
    pPCFHeader->Control = MQCFC_LAST;
    pPCFHeader->ParameterCount = 0;

    /* Setup parameter block: Queue Name */
    pPCFQueueNameStr->Type = MQCFT_STRING;
    pPCFQueueNameStr->StrucLength = MQCFST_STRUC_LENGTH_FIXED + MQ_Q_NAME_LENGTH;
    pPCFQueueNameStr->Parameter = MQCA_Q_NAME;
    pPCFQueueNameStr->CodedCharSetId = MQCCSI_DEFAULT;
    pPCFQueueNameStr->StringLength   = MQ_Q_NAME_LENGTH;
    memset(pPCFQueueNameStr->String, ' ', MQ_Q_NAME_LENGTH);
    strncpy(pPCFQueueNameStr->String, pRuleQM -> syncQName, MQ_Q_NAME_LENGTH);
    pPCFHeader->ParameterCount++;

    /* Setup parameter block: Queue type - local */
    pPCFQTypeInt->Type = MQCFT_INTEGER;
    pPCFQTypeInt->StrucLength = MQCFIN_STRUC_LENGTH;
    pPCFQTypeInt->Parameter = MQIA_Q_TYPE;
    pPCFQTypeInt->Value = MQQT_LOCAL;
    pPCFHeader->ParameterCount++;

    /* Setup parameter block: Default persistence - persistent */
    pPCFDefPsistInt->Type = MQCFT_INTEGER;
    pPCFDefPsistInt->StrucLength = MQCFIN_STRUC_LENGTH;
    pPCFDefPsistInt->Parameter = MQIA_DEF_PERSISTENCE;
    pPCFDefPsistInt->Value = MQPER_PERSISTENT;
    pPCFHeader->ParameterCount++;

    /* Setup parameter block: Queue Descriptor */
    pPCFQueueDescriptor->Type = MQCFT_STRING;
    pPCFQueueDescriptor->StrucLength = MQCFST_STRUC_LENGTH_FIXED + MQ_Q_DESC_LENGTH;
    pPCFQueueDescriptor->Parameter = MQCA_Q_DESC;
    pPCFQueueDescriptor->CodedCharSetId = MQCCSI_DEFAULT;
    pPCFQueueDescriptor->StringLength   = MQ_Q_DESC_LENGTH;
    memset(pPCFQueueDescriptor->String, ' ', MQ_Q_DESC_LENGTH);
    strncpy(pPCFQueueDescriptor->String, "IMA Synchronisation Queue", MQ_Q_DESC_LENGTH);
    pPCFHeader->ParameterCount++;

    rc = mqc_PCF(pRuleQM, hConn, &adminMsgLen, &pAdminMsg, MQC_FIRST | MQC_LAST);
    if (rc) goto MOD_EXIT;

    pPCFReply = (MQCFH *) pAdminMsg;

    TRACE(MQC_MQAPI_TRACE,
          "Create SyncQ '%s' '%s' CompCode(%d) Reason(%d)\n",
          pRuleQM -> pQM -> pQMName,
          pRuleQM -> syncQName,
          pPCFReply->CompCode,
          pPCFReply->Reason);

    if ((pPCFReply->Reason == MQRCCF_OBJECT_ALREADY_EXISTS) ||
        (pPCFReply->CompCode == MQCC_OK))
    {
        if (pPCFReply->Reason == MQRCCF_OBJECT_ALREADY_EXISTS)
        {
            *pQueueFound = true;
        }
        rc = 0;
    }
    else
    {
        mqc_MQAPIError(pRuleQM, RC_MQCreateSyncQError,
                       "MQCMD_CREATE_Q", pRuleQM -> syncQName, pPCFReply->Reason);
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

MOD_EXIT:

    if (pAdminMsg) mqc_free(pAdminMsg);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_deleteDynamicSyncQueue(mqcRuleQM_t * pRuleQM, MQHCONN hConn, char * name)
{
    MQOD     od = {MQOD_DEFAULT};    /* Object Descriptor             */
    MQHOBJ   hSyncObj;               /* object handle                 */
    MQLONG   openCode;               /* MQOPEN completion code        */
    MQLONG   closeCode;              /* MQCLOSE completion code       */
    MQLONG   openReason;             /* MQOpen reason code            */
    MQLONG   closeReason;            /* MQClose reason code           */
    MQLONG   openOptions;            /* MQOPEN options                */
    MQLONG   closeOptions;           /* MQCLOSE options               */
    int rc;

    openOptions = MQOO_INPUT_SHARED       /* open queue for input          */
              | MQOO_FAIL_IF_QUIESCING; /* but not if MQM stopping       */

    strcpy(od.ObjectName, name);

    MQOPEN(hConn,                   /* connection handle              */
           &od,                     /* object descriptor for queue    */
           openOptions,               /* open options                   */
           &hSyncObj,               /* object handle                  */
           &openCode,               /* completion code                */
           &openReason);                /* reason code                    */

    TRACE(MQC_MQAPI_TRACE,
          "Open SyncQ (from Model) '%s' '%s' CompCode(%d) Reason(%d)\n",
          pRuleQM -> pQM -> pQMName,
          name,
          openCode,
          openReason);

    if (openCode != MQCC_FAILED)
    {
        //We have the queue open - now Purge it - won't work if not dynamic
        closeOptions = MQCO_DELETE_PURGE; /* Delete incl. any messages     */

        MQCLOSE(hConn,                    /* connection handle             */
                &hSyncObj,                /* object handle                 */
                closeOptions,
                &closeCode,               /* completion code               */
                &closeReason);            /* reason code                   */

        if (   closeReason == MQRC_NONE
            || closeReason == MQRC_OPTION_NOT_VALID_FOR_TYPE //Not a model-based dynamic queue
           )
        {
            TRACE(MQC_MQAPI_TRACE,
                        "ClosePurge SyncQ (from Model) as Expected '%s' '%s' CompCode(%d) Reason(%d)\n",
                        pRuleQM -> pQM -> pQMName,
                        name,
                        closeCode,
                        closeReason);
        }
        else
        {
            TRACE(MQC_INTERESTING_TRACE,
                        "ClosePurge SyncQ (from Model) Unexpected Error '%s' '%s' CompCode(%d) Reason(%d)\n",
                        pRuleQM -> pQM -> pQMName,
                        name,
                        closeCode,
                        closeReason);
        }

        rc = closeCode;

        if (closeCode != MQCC_OK)
        {
            //Just close without purge so we don't leave a dangling handle
            closeOptions = 0; /* No Close options  */

            MQCLOSE(hConn,                    /* connection handle             */
                    &hSyncObj,                /* object handle                 */
                    closeOptions,
                    &closeCode,               /* completion code               */
                    &closeReason);            /* reason code                   */

            TRACE(MQC_MQAPI_TRACE,
                        "Close (Plain) on SyncQ (from Model) '%s' '%s' CompCode(%d) Reason(%d)\n",
                        pRuleQM -> pQM -> pQMName,
                        name,
                        closeCode,
                        closeReason);
        }
    }
    else
    {
        rc = openCode;
    }

    return rc;
}

/* Delete the sync queue associated with the queue manager and rule   */
/* identified by the parameter.                                       */

int mqc_deleteSyncQueue(mqcRuleQM_t * pRuleQM, MQHCONN hConn, char * name)
{
   int rc = 0;
   MQLONG adminMsgLen = 0;
   MQBYTE *pAdminMsg = NULL;
   MQCFH *pPCFHeader = NULL;
   MQCFST *pPCFQueueNameStr = NULL;
   MQCFIN *pPCFPurgeInt = NULL;
   MQCFH *pPCFReply = NULL;

   TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%s %s\n", __func__,
           pRuleQM -> pQM -> pQMName,
           name);

   //Assume it was created as a PermDyn queue from the model and see if we can
   //purge it that way...
   rc = mqc_deleteDynamicSyncQueue(pRuleQM, hConn, name);

   if (rc == ISMRC_OK)
   {
       //Already managed it...we're done
       goto MOD_EXIT;
   }
   else
   {
       //Not a problem - we'll fail back to our traditional (PCF) removal method, ignore the error
       rc = ISMRC_OK;
   }

   adminMsgLen = MQCFH_STRUC_LENGTH
       + MQCFST_STRUC_LENGTH_FIXED + MQ_Q_NAME_LENGTH /* Queue name */
       + MQCFIN_STRUC_LENGTH;    /* Purge = yes */

   pAdminMsg = mqc_malloc(adminMsgLen);
   if (pAdminMsg == NULL)
   {
       mqc_ruleQMAllocError(pRuleQM, "malloc", adminMsgLen, errno);
       rc = RC_RULEQM_DISABLE;
       goto MOD_EXIT;
   }
   memset(pAdminMsg, 0, adminMsgLen);

   pPCFHeader = (MQCFH *) pAdminMsg;
   pPCFQueueNameStr = (MQCFST *) (pAdminMsg + MQCFH_STRUC_LENGTH);
   pPCFPurgeInt = (MQCFIN *) ((MQBYTE*) pPCFQueueNameStr + MQCFST_STRUC_LENGTH_FIXED + MQ_Q_NAME_LENGTH);

   if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
   {
       pPCFHeader->Type = MQCFT_COMMAND_XR;
   }
   else
   {
       pPCFHeader->Type = MQCFT_COMMAND;
   }

   pPCFHeader->StrucLength = MQCFH_STRUC_LENGTH;
   pPCFHeader->Version = MQCFH_VERSION_3;
   pPCFHeader->Command = MQCMD_DELETE_Q;
   pPCFHeader->MsgSeqNumber = 1;
   pPCFHeader->Control = MQCFC_LAST;
   pPCFHeader->ParameterCount = 2;

   /* Setup parameter block: Queue Name */
   pPCFQueueNameStr->Type = MQCFT_STRING;
   pPCFQueueNameStr->StrucLength = MQCFST_STRUC_LENGTH_FIXED + MQ_Q_NAME_LENGTH;
   pPCFQueueNameStr->Parameter = MQCA_Q_NAME;
   pPCFQueueNameStr->CodedCharSetId = MQCCSI_DEFAULT;
   pPCFQueueNameStr->StringLength = MQ_Q_NAME_LENGTH;
   memset(pPCFQueueNameStr->String, ' ', MQ_Q_NAME_LENGTH);
   strncpy(pPCFQueueNameStr->String, name, MQ_Q_NAME_LENGTH);

   /* Setup parameter block: Purge - yes */
   pPCFPurgeInt->Type = MQCFT_INTEGER;
   pPCFPurgeInt->StrucLength = MQCFIN_STRUC_LENGTH;
   pPCFPurgeInt->Parameter = MQIACF_PURGE;
   pPCFPurgeInt->Value = MQPO_YES;

   rc = mqc_PCF(pRuleQM, hConn, &adminMsgLen, &pAdminMsg, MQC_FIRST | MQC_LAST);
   if (rc) goto MOD_EXIT;

   pPCFReply = (MQCFH *) pAdminMsg;

   TRACE(MQC_MQAPI_TRACE,
         "Delete SyncQ '%s' '%s' CompCode(%d) Reason(%d)\n",
         pRuleQM -> pQM -> pQMName,
         name,
         pPCFReply->CompCode,
         pPCFReply->Reason);

   if (pPCFReply->CompCode == MQCC_OK)
   {
       rc = 0;
   }
   else
   {
       mqc_MQAPIError(pRuleQM, RC_MQDeleteSyncQError,
                      "MQCMD_DELETE_Q", name, pPCFReply->Reason);

       rc = RC_RULEQM_RECONNECT;
       goto MOD_EXIT;
   }

MOD_EXIT:

   if (pAdminMsg) mqc_free(pAdminMsg);

   TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
   return rc;
}

/* Open the synchronisation queue for exclusive access to ensure that */
/* any connection from a previous instance of MQ Connectivity has     */
/* been removed.                                                      */
int mqc_openSyncQueueExclusive(mqcRuleQM_t * pRuleQM, MQHCONN hConn)
{
    int rc = 0;
    int retryCount = 10;
    int delayTime = 1000 * 1000; /* 1.0s in microseconds */

    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    MQLONG CompCode = MQCC_OK;
    MQLONG Reason = MQRC_NONE;
    MQOD od = { MQOD_DEFAULT };
    bool queueFound = false;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    /* Initialise the name of the sync q for this rule/QM pair */
    memset(pRuleQM -> syncQName, 0, MQ_Q_NAME_LENGTH);
    sprintf(pRuleQM -> syncQName,
            "SYSTEM.IMA.%.12s.%.8d.%.8d.%.6d",
            mqcMQConnectivity.objectIdentifier,
            pRule -> index,
            pQM -> index,
            pRuleQM -> index);

    TRACE(MQC_MQAPI_TRACE, "Sync Queue Name = <%s>\n", pRuleQM -> syncQName);

    /* Attempt to create the required syncq. If this fails because    */
    /* the queue already exists, that's fine.                         */
    rc = mqc_createSyncQueue(pRuleQM, hConn, &queueFound);
    if (rc)
        goto MOD_EXIT;

    strncpy(od.ObjectName, pRuleQM -> syncQName, MQ_Q_NAME_LENGTH);

    while (true)
    {
        MQOPEN(hConn,
               &od,
               MQOO_INPUT_EXCLUSIVE | MQOO_OUTPUT | MQOO_BROWSE,
               &pRuleQM -> hSyncObj,
               &CompCode,
               &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQOPEN '%s' '%s' for browse. CompCode(%d) Reason(%d)\n",
              pRuleQM -> pQM -> pQMName,
              pRuleQM -> syncQName,
              CompCode,
              Reason);

        if (Reason == MQRC_OBJECT_IN_USE)
        {
            if (retryCount-- <= 0)
            {
                TRACE(MQC_MQAPI_TRACE,
                      "Retry limit exceeded while opening SyncQ: %s.\n",
                      pQM -> pQMName);
                mqc_MQAPIError(pRuleQM,
                               RC_MQAPIError,
                               "MQOPEN exclusive for SyncQ failed",
                               pQM -> pQMName,
                               Reason);
                rc = RC_RULEQM_RECONNECT;
                goto MOD_EXIT;
            }
            else
            {
                TRACE(MQC_MQAPI_TRACE,
                      "Waiting for SyncQ [%d]: %s.\n",
                      retryCount,
                      pQM -> pQMName);
                ism_common_sleep(delayTime);
                continue;
            }
        }
        if (Reason == MQRC_NONE)
        {
            break;
        }
        else
        {
            TRACE(MQC_MQAPI_TRACE,
                  "MQOPEN for SyncQ, %s, failed. Reason: %X\n",
                  pQM -> pQMName,
                  Reason);
            mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQOPEN", pQM -> pQMName, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;            
        }
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE,
          FUNCTION_EXIT "%d.%d rc=%d\n",
          __func__,
          ruleindex,
          qmindex,
          rc);
    return rc;
}

int mqc_closeAndDeleteSyncQueue(mqcRuleQM_t * pRuleQM, MQHCONN hConn)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if ((hConn != MQHC_UNUSABLE_HCONN) &&
        (pRuleQM -> hSyncObj != MQHO_UNUSABLE_HOBJ))
    {
        MQCLOSE(hConn,
                &pRuleQM -> hSyncObj,
                MQCO_NONE,
                &CompCode,
                &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQCLOSE '%s' '%s' CompCode(%d) Reason(%d)\n",
              pQM -> pQMName, pRuleQM -> syncQName, CompCode, Reason);

        mqc_deleteSyncQueue(pRuleQM, hConn, pRuleQM -> syncQName);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_createMessageHandleQM(mqcRuleQM_t * pRuleQM, MQHCONN hConn)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;
    MQCMHO cmho = {MQCMHO_DEFAULT};

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    MQCRTMH(hConn,
            &cmho,
            &pRuleQM -> hMsg,
            &CompCode,
            &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQCRTMH '%s' CompCode(%d) Reason(%d)\n",
          pRuleQM -> pQM -> pQMName, CompCode, Reason);

    if (CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQCRTMH", "", Reason);
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_deleteMessageHandleQM(mqcRuleQM_t * pRuleQM, MQHCONN hConn, bool report)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;
    MQDMHO dmho = {MQDMHO_DEFAULT};

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    MQDLTMH(hConn,
            &pRuleQM -> hMsg,
            &dmho,
            &CompCode,
            &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQDLTMH '%s' CompCode(%d) Reason(%d)\n",
          pRuleQM -> pQM -> pQMName, CompCode, Reason);

    if ((CompCode == MQCC_FAILED) && report)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQDLTMH", "", Reason);
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_cancelWaiterQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG CompCode, Reason;
    MQHCONN hNotifyConn;
    MQCD cd = {MQCD_CLIENT_CONN_DEFAULT};
    MQCNO cno = {MQCNO_DEFAULT};
    MQSCO sco = {MQSCO_DEFAULT};
    MQCSP csp = {MQCSP_DEFAULT};

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    cno.Version = MQCNO_CURRENT_VERSION;
    cno.ClientConnPtr = &cd;

    cd.Version = MQCD_CURRENT_VERSION;
    cd.MaxMsgLength = 104857600;
    strncpy(cd.ChannelName, pQM -> pChannelName, MQ_CHANNEL_NAME_LENGTH);
    strncpy(cd.ConnectionName, pQM -> pConnectionName, MQ_CONN_NAME_LENGTH);

    if (pQM -> pSSLCipherSpec)
    {
        strncpy(cd.SSLCipherSpec, pQM -> pSSLCipherSpec, MQ_SSL_CIPHER_SPEC_LENGTH);

        sco.Version = MQSCO_CURRENT_VERSION;
        snprintf(sco.KeyRepository, MQ_SSL_KEY_REPOSITORY_LENGTH,
                 "%s/mqconnectivity", ism_common_getStringConfig("MQCertificateDir"));
        cno.SSLConfigPtr = &sco;

        TRACE(MQC_MQAPI_TRACE,
              "SSLCIPH '%.*s' SSLKEYR '%.*s'\n",
              MQ_SSL_CIPHER_SPEC_LENGTH, cd.SSLCipherSpec,
              MQ_SSL_KEY_REPOSITORY_LENGTH, sco.KeyRepository);
    }

    if (pQM->pChannelUserName)
    {
        csp.AuthenticationType = MQCSP_AUTH_USER_ID_AND_PWD;
        //add channel user name and password to csp
        csp.CSPUserIdPtr = pQM->pChannelUserName;
        csp.CSPUserIdLength = strlen(pQM->pChannelUserName);
        //do something to decrypt the password.
        csp.CSPPasswordPtr = pQM->pChannelPassword;
        csp.CSPPasswordLength = strlen(pQM->pChannelPassword);
        cno.SecurityParmsPtr = &csp;
    }

    /* Set the Product ID prior to the MQCONN so that the queue manager knows */
    /* who we are.                                                            */
    mqc_SPISetProdId(pRuleQM,
                     &CompCode,
                     &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "mqc_SPISetProdId CompCode(%d) Reason(%d)\n",
          CompCode, Reason);

    MQCONNX(pQM -> pQMName,
            &cno,
            &hNotifyConn,
            &CompCode,
            &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQCONNX(hNotifyConn) '%s' CompCode(%d) Reason(%d)\n",
          pQM -> pQMName, CompCode, Reason);

    if (CompCode == MQCC_FAILED)
    {
        goto MOD_EXIT;
    }

    rc = mqc_SPINotify(pRuleQM,
                       hNotifyConn,
                       pRuleQM -> hSub,
                       pRuleQM -> subConnectionId,
                       &CompCode,
                       &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "mqc_SPINotify rc(%d) CompCode(%d) Reason(%d)\n",
          rc, CompCode, Reason);
    if (rc) goto MOD_EXIT;

    MQDISC(&hNotifyConn,
           &CompCode,
           &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQDISC(hNotifyConn) '%s' CompCode(%d) Reason(%d)\n",
          pQM -> pQMName, CompCode, Reason);

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_MQSTAT(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQSTS sts = {MQSTS_DEFAULT};
    MQLONG CompCode, Reason;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    MQSTAT(pRuleQM -> hPubConn,
           MQSTAT_TYPE_ASYNC_ERROR,
           &sts,
           &CompCode,
           &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQSTAT '%s' CompCode(%d) Reason(%d)\n",
          pRuleQM -> pQM -> pQMName, CompCode, Reason);

    if (CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQSTAT", "", Reason);
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    TRACE(MQC_MQAPI_TRACE,
          "sts.CompCode(%d) sts.Reason(%d)\n", sts.CompCode, sts.Reason);

    if (sts.CompCode == MQCC_FAILED)
    {
        TRACE(MQC_MQAPI_TRACE,
              "PutSuccessCount(%d) PutWarningCount(%d) PutFailureCount(%d)\n",
              sts.PutSuccessCount, sts.PutWarningCount, sts.PutFailureCount);

        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQSTAT", "", sts.Reason);
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_putMessageQM(mqcRuleQM_t * pRuleQM,
                     ismc_message_t * pMessage)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQOD od = {MQOD_DEFAULT};
    MQMD md = {MQMD_DEFAULT};
    MQPMO pmo = {MQPMO_DEFAULT};
    MQLONG BufferLength, CompCode, Reason;
    PMQVOID pBuffer;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    /* Set MQ message properties */
    rc = mqc_setMQProperties(pRuleQM, pMessage,
                             &md, &BufferLength, &pBuffer);
    if (rc) goto MOD_EXIT;

    pmo.Version = MQPMO_CURRENT_VERSION;
    pmo.NewMsgHandle = (pRuleQM -> hMsg == MQHM_UNUSABLE_HMSG) ?
        MQHM_NONE : pRuleQM -> hMsg;
    pmo.Options |= MQPMO_NEW_MSG_ID;

    /* In IMA JMS NP messages have QoS of 1.                          */
    /* This means they are included in the IMA transaction and should */
    /* also be included in the MQ transaction.                        */
    if (pRuleQM -> flags & RULEQM_MULTICAST)
    {
        pmo.Options |= MQPMO_NO_SYNCPOINT;
    }
    else
    {
        if (ismc_getQuality(pMessage))
        {
            pmo.Options |= MQPMO_SYNCPOINT;

            /* Send transactional messages synchronously until we     */
            /* know we could issue a commit (check                    */
            /* MQRC_NO_SUBS_MATCHED).                                 */
            /* Or if we had an error committing the last batch as     */
            /* this will enable us to discover what the error was.    */
            if (!(pRuleQM -> flags & RULEQM_MQ_XA_COMMIT))
            {
                if (MQ_TOPIC_PRODUCE_RULE(pRule))
                {
                    pmo.Options |= MQPMO_WARN_IF_NO_SUBS_MATCHED;
                }
            }
            else if (pRuleQM -> batchMsgsToResend == 0)
            {
                pmo.Options |= MQPMO_ASYNC_RESPONSE;
            }
        }
        else
        {
            pmo.Options |= MQPMO_NO_SYNCPOINT |
                           MQPMO_ASYNC_RESPONSE;
        }
    }

    /* If the destination is an MQ topic then we must check whether */
    /* the rule requires the publish to be retained.                */
    if (MQ_TOPIC_PRODUCE_RULE(pRule))
    {
    	if (pRule -> RetainedMessages == mqcRetained_All)
    	{
    		pmo.Options |= MQPMO_RETAIN;
    	}
    }

    if (ismc_getDeliveryMode(pMessage) == ISMC_PERSISTENT)
    {
        pRuleQM -> persistentCount++;
    }
    else
    {
        pRuleQM -> nonpersistentCount++;
    }

    if (pRule -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQWILDTOPIC)
    {
        char topic[MQ_TOPIC_STR_LENGTH];
        int Length, domain;
        char * pDestination;

        pDestination = (char *)ismc_getDestination(pMessage, &domain);
        Length = strlen(pDestination);

        memcpy(topic, pDestination, Length);

        /* Map wildcard topic name */
        TRACE(MQC_NORMAL_TRACE, "From topic '%.*s'\n", Length, topic);

        if (Length > (strlen(pRule -> pSource) - 1))
        {
            memmove(topic + (strlen(pRule -> pDestination) - 1),
                    topic + (strlen(pRule -> pSource) - 1),
                    Length - (strlen(pRule -> pSource) - 1));
        }
        memcpy(topic, pRule -> pDestination, strlen(pRule -> pDestination) - 1);

        Length = Length - strlen(pRule -> pSource) + strlen(pRule -> pDestination);

        TRACE(MQC_NORMAL_TRACE, "To topic '%.*s'\n", Length, topic);

        od.ObjectType = MQOT_TOPIC;
        od.Version = MQOD_VERSION_4;

        od.ObjectString.VSPtr = topic;
        od.ObjectString.VSLength = Length;

        MQPUT1(pRuleQM -> hPubConn,
               &od,
               &md,
               &pmo,
               BufferLength,
               pBuffer,
               &CompCode,
               &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQPUT1 '%s' '%.*s' Options(0x%8.8X) CompCode(%d) Reason(%d)\n",
              pQM -> pQMName, Length, topic, pmo.Options, CompCode, Reason);
    }
    else
    {
        MQPUT(pRuleQM -> hPubConn,
              pRuleQM -> hPutObj,
              &md,
              &pmo,
              BufferLength,
              pBuffer,
              &CompCode,
              &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQPUT '%s' '%s' Options(0x%8.8X) CompCode(%d) Reason(%d)\n",
              pQM -> pQMName, pRule -> pDestination, pmo.Options, CompCode, Reason);
    }

    if (CompCode == MQCC_FAILED)
    {
        /* We've now discovered the MQPUT error */
        pRuleQM -> batchMsgsToResend = 0;

        switch (Reason)
        {
        case MQRC_DATA_LENGTH_ERROR:
        case MQRC_MSG_TOO_BIG_FOR_Q:
        case MQRC_MSG_TOO_BIG_FOR_Q_MGR:
            mqc_MQAPIError(pRuleQM, RC_MQAPILengthError, "MQPUT", "", Reason, BufferLength);
            break;
        default:
            mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQPUT", "", Reason);
            break;
        }

        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    pRuleQM -> batchMsgsSent += 1;
    pRuleQM -> batchDataSent += BufferLength;

    if (pRuleQM -> flags & RULEQM_MULTICAST)
    {
        if (ismc_getQuality(pMessage))
        {
            /* We have an IMA transactional message in our batch      */
            pRule -> flags |= RULE_IMA_XA_COMMIT;
        }
    }
    else
    {
        if (ismc_getQuality(pMessage))
        {
            /* We have an IMA transactional message in our batch      */
            pRule -> flags |= RULE_IMA_XA_COMMIT;

            /* In general, if the reason code is MQRC_NO_SUBS_MATCHED */
            /* then no message was actually sent, so there is no      */
            /* reason to schedule a commit. However, if the MQPUT was */
            /* the publication of a retained message then a message   */
            /* was sent (even if there are no current subscribers) so */
            /* we must commit that.                                   */

            if ((!(Reason == MQRC_NO_SUBS_MATCHED)) ||
                (pmo.Options & MQPMO_RETAIN))
            {
            	/* We have an MQ transactional message in our batch   */
            	pRuleQM -> flags |= RULEQM_MQ_XA_COMMIT;
            }
        }

        /* We need to issue an MQSTAT if the batch contains           */
        /* nonpersistent messages - an asynchronous MQPUT failure of  */
        /* a nonpersistent message in a transaction does not result   */
        /* in a commit failure.                                       */
        if (ismc_getDeliveryMode(pMessage) == ISMC_NON_PERSISTENT)
        {
            pRuleQM -> flags |= RULEQM_MQSTAT_REQUIRED;
        }
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

/* Inquire topic status to determine MCAST value.                     */
/* Note, until V8, MCAST is not supported on topic status.            */
/* Instead we query the associated administrative topic.              */
int mqc_inquireTopic(mqcRuleQM_t * pRuleQM, MQHCONN hConn, char * name)
{
    int i, rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    MQLONG adminMsgLen = 0;
    MQBYTE *pAdminMsg = NULL;

    MQCFH *pPCFHeader = NULL;
    MQCFH *pPCFReply = NULL;
    MQCFST *pPCFTopicName = NULL;
    MQCFIL *pPCFTopicAttrs = NULL;
    MQCFIN *pPCFMulticastInt = NULL;
    PMQLONG pType, pStrucLength, pParameter;
    char * pChar;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    // Some platforms don't support multicast, so avoid making the call altogether
    if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
    {
        TRACE(MQC_NORMAL_TRACE, "Platform %d does not support multicast topics\n", pRuleQM -> pQM -> platformType);
        assert((pRuleQM -> flags & RULEQM_MULTICAST) == 0);
        goto MOD_EXIT;
    }

    adminMsgLen = MQCFH_STRUC_LENGTH +
                  MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(name)) +
                  MQCFIL_STRUC_LENGTH_FIXED + sizeof(MQLONG);

    pAdminMsg = mqc_malloc(adminMsgLen);
    if (pAdminMsg == NULL)
    {
        mqc_ruleQMAllocError(pRuleQM, "malloc", adminMsgLen, errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }
    memset(pAdminMsg, 0, adminMsgLen);

    pPCFHeader = (MQCFH *) pAdminMsg;
    pPCFTopicName = (MQCFST *) (pAdminMsg + MQCFH_STRUC_LENGTH);
    pPCFTopicAttrs =
        (MQCFIL *) ((MQBYTE*) pPCFTopicName + MQCFST_STRUC_LENGTH_FIXED +
                    mqcRoundUp4(strlen(name)));

    if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
    {
        pPCFHeader->Type = MQCFT_COMMAND_XR;
    }
    else
    {
        pPCFHeader->Type = MQCFT_COMMAND;
    }

    pPCFHeader->StrucLength = MQCFH_STRUC_LENGTH;
    pPCFHeader->Version = MQCFH_VERSION_3;
    pPCFHeader->Command = MQCMD_INQUIRE_TOPIC;
    pPCFHeader->MsgSeqNumber = 1;
    pPCFHeader->Control = MQCFC_LAST;
    pPCFHeader->ParameterCount = 0;

    /* Setup parameter block: Topic name */
    pPCFTopicName->Type = MQCFT_STRING;
    pPCFTopicName->StrucLength = MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(name));
    pPCFTopicName->Parameter = MQCA_TOPIC_NAME;
    pPCFTopicName->CodedCharSetId = MQCCSI_DEFAULT;
    pPCFTopicName->StringLength = strlen(name);
    memcpy(pPCFTopicName->String, name, strlen(name));
    pPCFHeader->ParameterCount++;

    /* Setup parameter block: Attributes */
    pPCFTopicAttrs->Type = MQCFT_INTEGER_LIST;
    pPCFTopicAttrs->Count = 1;
    pPCFTopicAttrs->StrucLength = MQCFIL_STRUC_LENGTH_FIXED + sizeof(MQLONG);
    pPCFTopicAttrs->Parameter = MQIACF_TOPIC_ATTRS;
    pPCFTopicAttrs->Values[0] = MQIA_MULTICAST;
    pPCFHeader->ParameterCount++;

    rc = mqc_PCF(pRuleQM, hConn, &adminMsgLen, &pAdminMsg, MQC_FIRST | MQC_LAST);
    if (rc) goto MOD_EXIT;

    pPCFReply = (MQCFH *) pAdminMsg;

    TRACE(MQC_MQAPI_TRACE,
          "Inquire Topic '%s' '%s' CompCode(%d) Reason(%d)\n",
          pQM -> pQMName, name, pPCFReply->CompCode, pPCFReply->Reason);

    if (pPCFReply->CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQCMD_INQUIRE_TOPIC",
                       pRule -> pDestination, pPCFReply->Reason);

        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    pChar = (char *)(pPCFReply + 1);

    for (i = 0; i < pPCFReply->ParameterCount; i++)
    {
        pType = (PMQLONG) pChar;
        pStrucLength = (pType + 1);
        pParameter = (pStrucLength + 1);

        if (*pParameter == MQIA_MULTICAST)
        {
            pPCFMulticastInt = (PMQCFIN) pChar;

            if ((pPCFMulticastInt->Value == MQMC_ENABLED) ||
                (pPCFMulticastInt->Value == MQMC_ONLY))
            {
                TRACE(MQC_NORMAL_TRACE, "Found multicast topic '%s'\n", name);
                pRuleQM -> flags |= RULEQM_MULTICAST;
            }
        }

        pChar += *pStrucLength;
    }

MOD_EXIT:

    if (pAdminMsg) mqc_free(pAdminMsg);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_inquireTopicStatus(mqcRuleQM_t * pRuleQM, MQHCONN hConn)
{
    int i, rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    MQLONG adminMsgLen = 0;
    MQBYTE *pAdminMsg = NULL;

    MQCFH *pPCFHeader = NULL;
    MQCFH *pPCFReply = NULL;
    MQCFST *pPCFTopicString = NULL;
    MQCFIL *pPCFTopicStatusAttrs = NULL;
    MQCFST *pPCFTopicName = NULL;
    MQCFIN *pPCFMulticastInt = NULL;
    PMQLONG pType, pStrucLength, pParameter;
    char * pChar;
    char name[MQ_TOPIC_NAME_LENGTH + 1];

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    adminMsgLen = MQCFH_STRUC_LENGTH +
                  MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(pRule -> pDestination)) +
                  MQCFIL_STRUC_LENGTH_FIXED + sizeof(MQLONG);

    pAdminMsg = mqc_malloc(adminMsgLen);
    if (pAdminMsg == NULL)
    {
        mqc_ruleQMAllocError(pRuleQM, "malloc", adminMsgLen, errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }
    memset(pAdminMsg, 0, adminMsgLen);

    pPCFHeader = (MQCFH *) pAdminMsg;
    pPCFTopicString = (MQCFST *) (pAdminMsg + MQCFH_STRUC_LENGTH);
    pPCFTopicStatusAttrs =
        (MQCFIL *) ((MQBYTE*) pPCFTopicString + MQCFST_STRUC_LENGTH_FIXED +
                    mqcRoundUp4(strlen(pRule -> pDestination)));

    if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
    {
        pPCFHeader->Type = MQCFT_COMMAND_XR;
    }
    else
    {
        pPCFHeader->Type = MQCFT_COMMAND;
    }

    pPCFHeader->StrucLength = MQCFH_STRUC_LENGTH;
    pPCFHeader->Version = MQCFH_VERSION_3;
    pPCFHeader->Command = MQCMD_INQUIRE_TOPIC_STATUS;
    pPCFHeader->MsgSeqNumber = 1;
    pPCFHeader->Control = MQCFC_LAST;
    pPCFHeader->ParameterCount = 0;

    /* Setup parameter block: Topic string */
    pPCFTopicString->Type = MQCFT_STRING;
    pPCFTopicString->StrucLength = MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(pRule -> pDestination));
    pPCFTopicString->Parameter = MQCA_TOPIC_STRING;
    pPCFTopicString->CodedCharSetId = MQCCSI_DEFAULT;
    pPCFTopicString->StringLength = strlen(pRule -> pDestination);
    memcpy(pPCFTopicString->String, pRule -> pDestination, strlen(pRule -> pDestination));
    pPCFHeader->ParameterCount++;

    /* Setup parameter block: Status Attributes */
    pPCFTopicStatusAttrs->Type = MQCFT_INTEGER_LIST;
    pPCFTopicStatusAttrs->Count = 1;
    pPCFTopicStatusAttrs->StrucLength = MQCFIL_STRUC_LENGTH_FIXED + sizeof(MQLONG);
    pPCFTopicStatusAttrs->Parameter = MQIACF_TOPIC_STATUS_ATTRS;
    if (pRuleQM -> flags & RULEQM_MCAST_TOPIC_STATUS)
    {
        pPCFTopicStatusAttrs->Values[0] = MQIA_MULTICAST;
    }
    else
    {
        pPCFTopicStatusAttrs->Values[0] = MQCA_ADMIN_TOPIC_NAME;
    }
    pPCFHeader->ParameterCount++;

    rc = mqc_PCF(pRuleQM, hConn, &adminMsgLen, &pAdminMsg, MQC_FIRST | MQC_LAST);
    if (rc) goto MOD_EXIT;

    pPCFReply = (MQCFH *) pAdminMsg;

    TRACE(MQC_MQAPI_TRACE,
          "Inquire Topic Status '%s' '%s' CompCode(%d) Reason(%d)\n",
          pQM -> pQMName, pRule -> pDestination, pPCFReply->CompCode, pPCFReply->Reason);

    if (pPCFReply->CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQCMD_INQUIRE_TOPIC_STATUS",
                       pRule -> pDestination, pPCFReply->Reason);

        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    pChar = (char *)(pPCFReply + 1);

    for (i = 0; i < pPCFReply->ParameterCount; i++)
    {
        pType = (PMQLONG) pChar;
        pStrucLength = (pType + 1);
        pParameter = (pStrucLength + 1);

        if (pRuleQM -> flags & RULEQM_MCAST_TOPIC_STATUS)
        {
            if (*pParameter == MQIA_MULTICAST)
            {
                pPCFMulticastInt = (PMQCFIN) pChar;

                if ((pPCFMulticastInt->Value == MQMC_ENABLED) ||
                    (pPCFMulticastInt->Value == MQMC_ONLY))
                {
                    TRACE(MQC_NORMAL_TRACE, "Found multicast topic '%s'\n", pRule -> pDestination);
                    pRuleQM -> flags |= RULEQM_MULTICAST;
                }
            }
        }
        else
        {
            if (*pParameter == MQCA_ADMIN_TOPIC_NAME)
            {
                pPCFTopicName = (PMQCFST) pChar;

                sprintf(name, "%.*s", pPCFTopicName->StringLength, pPCFTopicName->String);

                if (name[0] == ' ')
                {
                    TRACE(MQC_NORMAL_TRACE, "Not an administrative topic '%s'\n", name);
                }
                else
                {
                    rc = mqc_inquireTopic(pRuleQM, hConn, name);
                    if (rc) goto MOD_EXIT;
                }
            }
        }

        pChar += *pStrucLength;
    }

MOD_EXIT:

    if (pAdminMsg) mqc_free(pAdminMsg);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_inquireTopicUseDLQ(mqcRuleQM_t * pRuleQM, MQHCONN hConn)
{
    int i, rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    MQLONG adminMsgLen = 0;
    MQBYTE *pAdminMsg = NULL;

    MQCFH *pPCFHeader = NULL;
    MQCFH *pPCFReply = NULL;
    MQCFST *pPCFTopicString = NULL;
    MQCFIL *pPCFTopicStatusAttrs = NULL;
    MQCFIN *pPCFUseDLQInt = NULL;
    PMQLONG pType, pStrucLength, pParameter;
    char * pChar;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    adminMsgLen = MQCFH_STRUC_LENGTH +
                  MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(pRule -> pSource)) +
                  MQCFIL_STRUC_LENGTH_FIXED + sizeof(MQLONG);

    pAdminMsg = mqc_malloc(adminMsgLen);
    if (pAdminMsg == NULL)
    {
        mqc_ruleQMAllocError(pRuleQM, "malloc", adminMsgLen, errno);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }
    memset(pAdminMsg, 0, adminMsgLen);

    pPCFHeader = (MQCFH *) pAdminMsg;
    pPCFTopicString = (MQCFST *) (pAdminMsg + MQCFH_STRUC_LENGTH);
    pPCFTopicStatusAttrs =
        (MQCFIL *) ((MQBYTE*) pPCFTopicString + MQCFST_STRUC_LENGTH_FIXED +
                    mqcRoundUp4(strlen(pRule -> pSource)));

    if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
    {
        pPCFHeader->Type = MQCFT_COMMAND_XR;
    }
    else
    {
        pPCFHeader->Type = MQCFT_COMMAND;
    }

    pPCFHeader->StrucLength = MQCFH_STRUC_LENGTH;
    pPCFHeader->Version = MQCFH_VERSION_3;
    pPCFHeader->Command = MQCMD_INQUIRE_TOPIC_STATUS;
    pPCFHeader->MsgSeqNumber = 1;
    pPCFHeader->Control = MQCFC_LAST;
    pPCFHeader->ParameterCount = 0;

    /* Setup parameter block: Topic string */
    pPCFTopicString->Type = MQCFT_STRING;
    pPCFTopicString->StrucLength = MQCFST_STRUC_LENGTH_FIXED + mqcRoundUp4(strlen(pRule -> pSource));
    pPCFTopicString->Parameter = MQCA_TOPIC_STRING;
    pPCFTopicString->CodedCharSetId = MQCCSI_DEFAULT;
    pPCFTopicString->StringLength = strlen(pRule -> pSource);
    memcpy(pPCFTopicString->String, pRule -> pSource, strlen(pRule -> pSource));
    pPCFHeader->ParameterCount++;

    /* Setup parameter block: Status Attributes */
    pPCFTopicStatusAttrs->Type = MQCFT_INTEGER_LIST;
    pPCFTopicStatusAttrs->Count = 1;
    pPCFTopicStatusAttrs->StrucLength = MQCFIL_STRUC_LENGTH_FIXED + sizeof(MQLONG);
    pPCFTopicStatusAttrs->Parameter = MQIACF_TOPIC_STATUS_ATTRS;
    pPCFTopicStatusAttrs->Values[0] = MQIA_USE_DEAD_LETTER_Q;
    pPCFHeader->ParameterCount++;

    rc = mqc_PCF(pRuleQM, hConn, &adminMsgLen, &pAdminMsg, MQC_FIRST | MQC_LAST);
    if (rc) goto MOD_EXIT;

    pPCFReply = (MQCFH *) pAdminMsg;

    TRACE(MQC_MQAPI_TRACE,
          "Inquire Topic Status '%s' '%s' CompCode(%d) Reason(%d)\n",
          pQM -> pQMName, pRule -> pSource, pPCFReply->CompCode, pPCFReply->Reason);

    if (pPCFReply->CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQCMD_INQUIRE_TOPIC_STATUS",
                       pRule -> pSource, pPCFReply->Reason);

        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    pChar = (char *)(pPCFReply + 1);

    for (i = 0; i < pPCFReply->ParameterCount; i++)
    {
        pType = (PMQLONG) pChar;
        pStrucLength = (pType + 1);
        pParameter = (pStrucLength + 1);

        if (*pParameter == MQIA_USE_DEAD_LETTER_Q)
        {
            pPCFUseDLQInt = (PMQCFIN) pChar;

            if (pPCFUseDLQInt->Value == MQUSEDLQ_YES)
            {
                TRACE(MQC_NORMAL_TRACE,
                      "Enabling DLQ for '%s'\n", pRule -> pSource);
                pRuleQM -> flags |= RULEQM_USEDLQ;
            }
        }

        pChar += *pStrucLength;
    }

MOD_EXIT:

    if (pAdminMsg) mqc_free(pAdminMsg);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_inquireDLQ(mqcRuleQM_t * pRuleQM,
                   MQHCONN hConn)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    MQLONG CompCode, Reason;
    MQOD od = { MQOD_DEFAULT };
    MQLONG selectors[1] = { MQCA_DEAD_LETTER_Q_NAME };
    MQHOBJ hObj = MQHO_UNUSABLE_HOBJ;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    od.ObjectType = MQOT_Q_MGR;

    MQOPEN(hConn,
           &od,
           MQOO_INQUIRE,
           &hObj,
           &CompCode,
           &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQOPEN CompCode(%d) Reason(%d)\n", CompCode, Reason);

    if (CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQOPEN", pQM -> pQMName, Reason);
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    MQINQ(hConn,
          hObj,
          1,                    /* SelectorCount */
          selectors,
          0,                    /* IntAttrCount */
          NULL,                 /* IntAttrs */
          MQ_Q_NAME_LENGTH + 1, /* CharAttrLength */
          pQM -> dlqName,       /* CharAttrs */
          &CompCode,
          &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQINQ CompCode(%d) Reason(%d)\n", CompCode, Reason);

    TRACE(MQC_MQAPI_TRACE,
          "MQINQ  dlqName = '%.*s'\n",
          MQ_Q_NAME_LENGTH,
          pQM -> dlqName);

MOD_EXIT:

    if (hObj != MQHO_UNUSABLE_HOBJ)
    {
        MQCLOSE(hConn,
                &hObj,
                MQCO_NONE,
                &CompCode,
                &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQCLOSE CompCode(%d) Reason(%d)\n", CompCode, Reason);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}
