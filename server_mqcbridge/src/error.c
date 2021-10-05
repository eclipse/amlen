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
#include <log.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

int mqc_mapISMCRc(int ismcrc, bool bRuleQM, bool * pbImmediateReconnect)
{
    int rc = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    switch (ismcrc)
    {
    case ISMRC_DestinationFull:
    case ISMRC_DestNotValid:
        rc = (bRuleQM) ? RC_RULEQM_RECONNECT : RC_RULE_RECONNECT;
        break;
    default:
        /* Assume other errors mean we have lost connection to IMA */
        rc = RC_MQC_RECONNECT;
        break;
    }

    /* Don't attempt an immediate reconnect for return codes that     */
    /* require user intervention                                      */
    switch (ismcrc)
    {
    case ISMRC_DestNotValid:
    case ISMRC_NotConnected:
        *pbImmediateReconnect = false;
        break;
    default:
        *pbImmediateReconnect = true;
        break;
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_MQAPIError(mqcRuleQM_t * pRuleQM,
                   int rc, char * api, char * obj, MQLONG Reason, ...)
{
    int lrc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    mqcPRET pret;
    va_list args;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    for (pret = (mqcPRET)&mqcRetList[0]; pret -> ret != -1; pret++)
    {
        if (pret -> ret == Reason)  break;
    }

    /* Initialize our inserts */
    ism_common_strlcpy(pRuleQM -> errorInsert[0], obj, sizeof(pRuleQM -> errorInsert[0]));
    if (pret -> ret == -1)
    {
    	/* This is an unknown reason code. Just report the number. */
    	sprintf(pRuleQM -> errorInsert[1], "%d", Reason);
    }
    else
    {
    	/* Report the symbol name and number. */
    	sprintf(pRuleQM -> errorInsert[1], "%s (%d)", pret -> desc, Reason);
    }
    pRuleQM -> errorInsert[2][0] = '\0';
    pRuleQM -> errorInsert[3][0] = '\0';

    pRuleQM -> MQConnectivityRC = rc;
    pRuleQM -> OtherRC = Reason;
    pRuleQM -> pOtherRCSymbol = pRuleQM -> errorInsert[1];

    switch (rc)
    {
    case RC_MQCBError:
        pRuleQM -> errorMsgId = RC_MQCBError;
        /* LOG() needs the literal value for RC_MQCBError = 7029 */
        LOG(ERROR, MQConnectivity, 7029,
            "%-s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. Unable to define callback function for WebSphere MQ queue manager {2}. MQCB Reason {3}.",
            pRule -> pName,  pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);
        break;
    case RC_MQCONNXError:
        pRuleQM -> errorMsgId = RC_MQCONNXError;
        /* LOG() needs the literal value for RC_MQCONNXError = 7011 */
        LOG(ERROR, MQConnectivity, 7011,
            "%-s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. Unable to connect to WebSphere MQ queue manager {2}. MQCONNX Reason {3}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);
        break;
    case RC_MQOPENQueueError:
        pRuleQM -> errorMsgId = RC_MQOPENQueueError;
        /* LOG() needs the literal value for RC_MQOPENQueueError = 7012 */
        LOG(ERROR, MQConnectivity, 7012,
            "%-s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. Unable to open WebSphere MQ queue {2}. MQOPEN Reason {3}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);
        break;
    case RC_MQOPENTopicError:
        pRuleQM -> errorMsgId = RC_MQOPENTopicError;
        /* LOG() needs the literal value for RC_MQOPENTopicError = 7013 */
        LOG(ERROR, MQConnectivity, 7013,
            "%-s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. Unable to open WebSphere MQ topic {2}. MQOPEN Reason {3}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);
        break;
    case RC_MQSUBError:
        strcpy(pRuleQM -> errorInsert[2], pRuleQM -> errorInsert[1]); // Shift down
        va_start(args, Reason);
        ism_common_strlcpy(pRuleQM -> errorInsert[1],
                           va_arg(args, char *),
                           sizeof(pRuleQM -> errorInsert[1])); // subname
        pRuleQM -> errorMsgId = RC_MQSUBError;
        /* LOG() needs the literal value for RC_MQSUBError = 7014 */
        LOG(ERROR, MQConnectivity, 7014,
            "%-s%s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. Unable to subscribe to WebSphere MQ topic {2}. Subscription name {3}. MQSUB Reason {4}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1], pRuleQM -> errorInsert[2]);
        va_end(args);
        break;
    case RC_MQSUBTopicError:
        va_start(args, Reason);
        ism_common_strlcpy(pRuleQM -> errorInsert[1],
                           va_arg(args, char *),
                           sizeof(pRuleQM -> errorInsert[1])); // subname
        ism_common_strlcpy(pRuleQM -> errorInsert[2],
                           va_arg(args, char *),
                           sizeof(pRuleQM -> errorInsert[2])); // topic
        pRuleQM -> errorMsgId = RC_MQSUBTopicError;
        /* LOG() needs the literal value for RC_MQSUBTopicError = 7015 */
        LOG(ERROR, MQConnectivity, 7015,
            "%-s%s%-s%s%-s",
            "Destination mapping rule {0}. Queue manager connection {1}. Unable to subscribe to WebSphere MQ topic {2}. Subscription name {3}. Subscription exists with topic string ending {4}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1], pRuleQM -> errorInsert[2]);
        va_end(args);
        break;
    case RC_MQPUTQueueError:
        pRuleQM -> errorMsgId = RC_MQPUTQueueError;
        /* LOG() needs the literal value for RC_MQPUTQueueError = 7016 */
        LOG(ERROR, MQConnectivity, 7016,
            "%-s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. Unable to put to WebSphere MQ queue {2}. MQPUT Reason {3}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);
        break;
    case RC_PCFMQGETError:
        pRuleQM -> errorMsgId = RC_PCFMQGETError;
        /* LOG() needs the literal value for RC_PCFMQGETError = 7017 */
        LOG(ERROR, MQConnectivity, 7017,
            "%-s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. Unable to get PCF response from WebSphere MQ temporary dynamic reply queue {2}. MQGET Reason {3}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);
        break;
    case RC_MQCreateSyncQError:
        pRuleQM -> errorMsgId = RC_MQCreateSyncQError;
        /* LOG() needs the literal value for RC_MQCreateSyncQError = 7018 */
        LOG(ERROR, MQConnectivity, 7018,
            "%-s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. Unable to create " IMA_PRODUCTNAME_SHORT " WebSphere MQ synchronization queue {2}. Reason {3}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);
        break;
    case RC_MQDeleteSyncQError:
        pRuleQM -> errorMsgId = RC_MQDeleteSyncQError;
        /* LOG() needs the literal value for RC_MQDeleteSyncQError = 7019 */
        LOG(ERROR, MQConnectivity, 7019,
            "%-s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. Unable to delete " IMA_PRODUCTNAME_SHORT " WebSphere MQ synchronization queue {2}. Reason {3}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);
        break;
    case RC_MQAPILengthError:
        ism_common_strlcpy(pRuleQM -> errorInsert[0], api, sizeof(pRuleQM -> errorInsert[0]));
        va_start(args, Reason);
        sprintf(pRuleQM -> errorInsert[2], "%d", va_arg(args, int)); // length
        pRuleQM -> errorMsgId = RC_MQAPILengthError;
        /* LOG() needs the literal value for RC_MQAPILengthError = 7028 */
        LOG(ERROR, MQConnectivity, 7028,
            "%-s%s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. WebSphere MQ API failed. API {2}. Reason {3}. Length {4}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1], pRuleQM -> errorInsert[2]);
        va_end(args);
        break;
    case RC_MQAPIError:
    default:
        ism_common_strlcpy(pRuleQM -> errorInsert[0], api, sizeof(pRuleQM -> errorInsert[0]));
        pRuleQM -> errorMsgId = RC_MQAPIError;
        /* LOG() needs the literal value for RC_MQAPIError = 7010 */
        LOG(ERROR, MQConnectivity, 7010,
            "%-s%s%s%s",
            "Destination mapping rule {0}. Queue manager connection {1}. WebSphere MQ API failed. API {2}. Reason {3}.",
            pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);
        break;
    }

    /* Don't attempt an immediate reconnect for return codes that     */
    /* require user intervention                                      */
    switch (Reason)
    {
    case MQRC_UNKNOWN_OBJECT_NAME:
        pRuleQM -> immediateRetries = mqcMQConnectivity.immediateRetryCount;
        break;
    default:
        break;
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, lrc);
    return lrc;
}

int mqc_XAError(mqcRuleQM_t * pRuleQM,
                char * api, int xarc)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    int i = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    /* Attempt to reconnect if we have a broken connection or we      */
    /* were rolled back.                                              */
    /* XAEnd returns XA_RBROLLBACK when an async MQPUT has a failure. */
    /* XAPrepare returns XA_RBTIMEOUT (implicit XAEnd support).       */
    /* Any other error is unexpected so disable the rule.             */
    if ((xarc == XAER_RMERR) ||
        (xarc == XAER_RMFAIL) ||
        (xarc == XA_RBROLLBACK) ||
        (xarc == XA_RBTIMEOUT))
    {
        rc = RC_RULEQM_RECONNECT;

        /* After a commit failure we resend messages individually     */
        /* until we've successfully sent the number of messages that  */
        /* were in the failed batch or we discover an MQPUT error.    */
        if ((xarc == XA_RBROLLBACK) || (xarc == XA_RBTIMEOUT))
        {
            pRuleQM -> batchMsgsToResend = pRuleQM -> batchMsgsSent;
        }
    }
    else
    {
        rc = RC_RULEQM_DISABLE;
    }

    for (i = 0; XASymbols[i].xarc != XA_OK; i++)
    {
        if (XASymbols[i].xarc == xarc)
        {
            break;
        }
    }

    /* Initialize our inserts */
    ism_common_strlcpy(pRuleQM -> errorInsert[0], api, sizeof(pRuleQM -> errorInsert[0]));
    if (XASymbols[i].xarc != XA_OK)
    {
        ism_common_strlcpy(pRuleQM->errorInsert[1], XASymbols[i].pSymbol, sizeof(pRuleQM -> errorInsert[1]));
    }
    else
    {
        /* Report the symbol name and number. */
        sprintf(pRuleQM -> errorInsert[1], "%d", xarc);
    }
    pRuleQM -> errorInsert[2][0] = '\0';
    pRuleQM -> errorInsert[3][0] = '\0';

    pRuleQM -> MQConnectivityRC = RC_MQAPIError;
    pRuleQM -> OtherRC = xarc;
    pRuleQM -> pOtherRCSymbol = pRuleQM -> errorInsert[1];

    pRuleQM -> errorMsgId = RC_MQAPIError;

    /* LOG() needs the literal value for RC_MQAPIError = 7010 */
    LOG(ERROR, MQConnectivity, 7010,
        "%-s%s%s%s",
        "Destination mapping rule {0}. Queue manager connection {1}. WebSphere MQ API failed. API {2}. Reason {3}.",
        pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_ISMCError(char * api)
{
    int ismcrc, rc = 0;
    bool bImmediateReconnect;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    /* Retrieve the most recent ISM C error code - meaning, the one   */
    /* that prompted the call of this function.                       */
    ismcrc = ism_common_getLastError();

    if (ismcrc)
    {
        ism_common_strlcpy(mqcMQConnectivity.ima.errorInsert[0], api, sizeof(mqcMQConnectivity.ima.errorInsert[0]));

        ism_common_formatLastError(mqcMQConnectivity.ima.errorInsert[1], sizeof(mqcMQConnectivity.ima.errorInsert[1]));

        if ((ismcrc == ISMRC_UnableToConnect) ||
            (ismcrc == ISMRC_NetworkError))
        {
            mqcMQConnectivity.ima.errorMsgId = RC_ISMCNetworkError;
            /* LOG() needs the literal value for RC_ISMCNetworkError = 7008 */
            LOG(ERROR, MQConnectivity, 7008,
                "%s%s",
                "ISMC network error. API {0}. Reason {1}. This may be because the server is not running.",
                 mqcMQConnectivity.ima.errorInsert[0], mqcMQConnectivity.ima.errorInsert[1]);
        }
        else
        {
            mqcMQConnectivity.ima.errorMsgId = RC_ISMCError;
            /* LOG() needs the literal value for RC_ISMCError = 7005 */
            LOG(ERROR, MQConnectivity, 7005,
                "%s%s",
                "ISMC API failed. API {0}. Reason {1}.",
                mqcMQConnectivity.ima.errorInsert[0], mqcMQConnectivity.ima.errorInsert[1]);
        }
    }

    rc = mqc_mapISMCRc(ismcrc, false, &bImmediateReconnect);

    if (bImmediateReconnect == false)
        mqcMQConnectivity.immediateRetries = mqcMQConnectivity.immediateRetryCount;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_ISMCRuleError(mqcRule_t * pRule, char * api, char * obj)
{
    int ismcrc, rc = 0;
    int ruleindex = pRule -> index;
    bool bImmediateReconnect;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d\n", __func__, ruleindex);

    ismcrc = ism_common_getLastError();

    if (ismcrc)
    {
        /* ISMC is given an ISMRC on error.
           We need to add inserts to the associated message ourselves. */
        switch (ismcrc)
        {
        case ISMRC_DestNotValid:
            if (strlen(obj) > 128)
            {
                char error[256];
                sprintf(error, "...%.128s", obj + strlen(obj) - 128);
                ism_common_setErrorData(ismcrc, "%s", error);
            }
            else
            {
                ism_common_setErrorData(ismcrc, "%s", obj);
            }
            break;
        default:
            break;
        }

        pRule -> errorMsgId = RC_ISMCRuleError;
        ism_common_strlcpy(pRule -> errorInsert[0], api, sizeof(pRule -> errorInsert[0]));
        ism_common_formatLastError(pRule -> errorInsert[1], sizeof(pRule -> errorInsert[1]));
        /* LOG() needs the literal value for RC_ISMCRuleError = 7006 */
        LOG(ERROR, MQConnectivity, 7006,
            "%-s%s%s",
            "Destination Mapping {0}. ISMC API failed. API {1}. Reason {2}.",
            pRule -> pName, pRule -> errorInsert[0], pRule -> errorInsert[1]);
    }

    rc = mqc_mapISMCRc(ismcrc, false, &bImmediateReconnect);

    if (bImmediateReconnect == false)
        pRule -> immediateRetries = mqcMQConnectivity.immediateRetryCount;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d rc=%d\n", __func__, ruleindex, rc);
    return rc;
}

int mqc_ISMCRuleQMError(mqcRuleQM_t * pRuleQM, char * api, char * obj)
{
    int ismcrc, rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    bool bImmediateReconnect;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    ismcrc = ism_common_getLastError();

    if (ismcrc)
    {
        rc = RC_ISMCRuleQMError;

        /* ISMC is given an ISMRC on error.
           We need to add inserts to the associated message ourselves. */
        switch (ismcrc)
        {
        case ISMRC_DestNotValid:
            if (strlen(obj) > 128)
            {
                char error[256];
                sprintf(error, "...%.128s", obj + strlen(obj) - 128);
                ism_common_setErrorData(ismcrc, "%s", error);
            }
            else
            {
                ism_common_setErrorData(ismcrc, "%s", obj);
            }

            if ((strlen(api) == strlen("ismc_sendX")) &&
                (strcmp(api, "ismc_sendX") == 0))
            {
                rc = RC_ISMCDestNotValidError;
            }
            break;
        default:
            break;
        }

        ism_common_strlcpy(pRuleQM -> errorInsert[0], api, sizeof(pRuleQM -> errorInsert[0]));
        ism_common_formatLastError(pRuleQM -> errorInsert[1], sizeof(pRuleQM -> errorInsert[1]));
        pRuleQM -> errorInsert[2][0] = '\0';
        pRuleQM -> errorInsert[3][0] = '\0';

        switch (rc)
        {
        case RC_ISMCDestNotValidError:
            pRuleQM -> errorMsgId = RC_ISMCDestNotValidError;
            ism_common_strlcpy(pRuleQM -> errorInsert[2], pRuleQM -> subName, sizeof(pRuleQM -> errorInsert[2]));
            ism_common_strlcpy(pRuleQM -> errorInsert[3], pRuleQM -> managedQName, sizeof(pRuleQM -> errorInsert[3]));
            /* LOG() needs the literal value for RC_ISMCDestNotValidError = 7009 */
            LOG(ERROR, MQConnectivity, 7009,
                "%-s%s%s%s%s%s",
                "Destination mapping rule {0}. Queue manager connection {1}. ISMC API failed. API {2}. Reason {3}. Subscription name {4}. Managed queue name {5}.",
                pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1], pRuleQM -> errorInsert[2], pRuleQM -> errorInsert[3]);
            break;
        default:
            pRuleQM -> errorMsgId = RC_ISMCRuleQMError;
            /* LOG() needs the literal value for RC_ISMCRuleQMError = 7007 */
            LOG(ERROR, MQConnectivity, 7007,
                "%-s%s%s%s",
                "Destination mapping rule {0}. Queue manager connection {1}. ISMC API failed. API {2}. Reason {3}.",
                pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1]);
            break;
        }
    }

    rc = mqc_mapISMCRc(ismcrc, true, &bImmediateReconnect);

    if (bImmediateReconnect == false)
        pRuleQM -> immediateRetries = mqcMQConnectivity.immediateRetryCount;

    /* ISMC puts are synchronous. After a failure in a batch we       */
    /* resend messages individually until we've successfully sent the */
    /* number of messages that were in the failed batch or we         */
    /* discover an ismc_send(X) error.                                */
    if (((strlen(api) == strlen("ismc_send")) &&
         (strcmp(api, "ismc_send") == 0)) ||
        ((strlen(api) == strlen("ismc_sendX")) &&
         (strcmp(api, "ismc_sendX") == 0)))
    {
        if (pRuleQM -> batchMsgsToResend)
        {
            pRuleQM -> batchMsgsToResend = 0;
        }
        else
        {
            pRuleQM -> batchMsgsToResend = pRuleQM -> batchMsgsSent;
        }
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_allocError(char * api, int len, int syserrno)
{
    int rc = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ism_common_strlcpy(mqcMQConnectivity.ima.errorInsert[0], api, sizeof(mqcMQConnectivity.ima.errorInsert[0]));
    sprintf(mqcMQConnectivity.ima.errorInsert[1], "%d", len);
    sprintf(mqcMQConnectivity.ima.errorInsert[2], "%d", syserrno);
    mqcMQConnectivity.ima.errorMsgId = RC_AllocateError;
    /* LOG() needs the literal value for RC_AllocateError = 7021 */
    LOG(ERROR, MQConnectivity, 7021,
        "%s%s%s",
        "Unable to allocate memory. {0} {1} byte(s) errno {2}.",
        mqcMQConnectivity.ima.errorInsert[0],
        mqcMQConnectivity.ima.errorInsert[1],
        mqcMQConnectivity.ima.errorInsert[2]);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_ruleQMAllocError(mqcRuleQM_t * pRuleQM,
                         char * api, int len, int syserrno)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    ism_common_strlcpy(pRuleQM -> errorInsert[0], api, sizeof(pRuleQM -> errorInsert[0]));
    sprintf(pRuleQM -> errorInsert[1], "%d", len);
    sprintf(pRuleQM -> errorInsert[2], "%d", syserrno);
    pRuleQM -> errorInsert[3][0] = '\0';
    pRuleQM -> errorMsgId = RC_AllocateRuleQMError;
    /* LOG() needs the literal value for RC_AllocateRuleQMError = 7022 */
    LOG(ERROR, MQConnectivity, 7022,
        "%-s%s%s%s%s",
        "Destination mapping rule {0}. Queue manager connection {1}. Unable to allocate memory. {2} {3} byte(s) errno {4}.",
        pRule -> pName, pQM -> pName, pRuleQM -> errorInsert[0], pRuleQM -> errorInsert[1], pRuleQM -> errorInsert[2]);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_systemAPIError(char * api, int sysrc, int syserrno)
{
    int rc = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    mqcMQConnectivity.ima.errorMsgId = RC_SystemAPIError;
    ism_common_strlcpy(mqcMQConnectivity.ima.errorInsert[0], api, sizeof(mqcMQConnectivity.ima.errorInsert[0]));
    sprintf(mqcMQConnectivity.ima.errorInsert[1], "%d", sysrc);
    sprintf(mqcMQConnectivity.ima.errorInsert[2], "%d", syserrno);
    /* LOG() needs the literal value for RC_SystemAPIError = 7023 */
    LOG(ERROR, MQConnectivity, 7023,
        "%s%s%s",
        "System API failed. API {0}. rc {1} errno {2}",
        mqcMQConnectivity.ima.errorInsert[0],
        mqcMQConnectivity.ima.errorInsert[1],
        mqcMQConnectivity.ima.errorInsert[2]);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_messageError(mqcRuleQM_t * pRuleQM, char * type, int len, char * buffer)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    TRACEDATA(MQC_MQAPI_TRACE, "Invalid message", 0, buffer, len, len);

    ism_common_strlcpy(pRuleQM -> errorInsert[0], type, sizeof(pRuleQM -> errorInsert[0]));
    pRuleQM -> errorInsert[1][0] = '\0';
    pRuleQM -> errorInsert[2][0] = '\0';
    pRuleQM -> errorInsert[3][0] = '\0';
    pRuleQM -> errorMsgId = RC_InvalidMessage;
    /* LOG() needs the literal value for RC_InvalidMessage = 7020 */
    LOG(ERROR, MQConnectivity, 7020,
        "%s",
        "Invalid WebSphere MQ message. Message type {0}.",
        pRuleQM -> errorInsert[0]);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

void mqc_errorListener(int rc, const char * error,
                       ismc_connection_t * connect, ismc_session_t * session, void * userdata)
{
    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    TRACE(MQC_NORMAL_TRACE, "rc(%d) '%s'\n", rc, error);

    if (rc == ISMRC_DestNotValid)
    {
        /* For non persistent messages our ismc_send(X) call is       */
        /* asynchronous. The actual call can fail if our wildcard     */
        /* destination topic is invalid. We are informed of this      */
        /* failure in this callback. We ignore this error.            */
    }
    else
    {
        strcpy(mqcMQConnectivity.ima.errorInsert[0], "ErrorListener");
        ism_common_strlcpy(mqcMQConnectivity.ima.errorInsert[1], error, sizeof(mqcMQConnectivity.ima.errorInsert[1]));
        mqcMQConnectivity.ima.errorInsert[2][0] = '\0';
        mqcMQConnectivity.ima.errorMsgId = RC_ISMCError;
        /* LOG() needs the literal value for RC_ISMCError = 7005 */
        LOG(ERROR, MQConnectivity, 7005,
            "%s%s",
            "ISMC API failed. API {0}. Reason {1}.",
            mqcMQConnectivity.ima.errorInsert[0], mqcMQConnectivity.ima.errorInsert[1]);

        mqcMQConnectivity.flags |= MQC_RECONNECT;
        mqc_wakeReconnectThread();
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return;
}

int mqc_setMQTrace(int level)
{
    int fd = -1, rc = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d\n", __func__, level);

    rc = fork();

    if (rc < 0)
    {
        mqc_systemAPIError("fork", rc, errno);
        goto MOD_EXIT;
    }

    if (rc == 0)
    {
        /* Redirect stdout and stderr */
        fd = open("/dev/null", O_RDWR);

        if (fd != -1)
        {
            dup2(fd, fileno(stdout));
            dup2(fd, fileno(stderr));
            close (fd);
        }

        char Command[strlen(mqcMQConnectivity.MQInstallationPath) + 14];

        if (level == 9)
        {
            char TraceMaxFileSize[20];
            char TraceUserDataSize[20];

            sprintf(Command, "%s/bin/strmqtrc", mqcMQConnectivity.MQInstallationPath);
            sprintf(TraceMaxFileSize, "%d", mqcMQConnectivity.MQTraceMaxFileSize);
            sprintf(TraceUserDataSize, "%d", mqcMQConnectivity.MQTraceUserDataSize);

            rc = execl(Command, "strmqtrc", "-t", "detail", "-t", "all", "-l", TraceMaxFileSize, "-d", TraceUserDataSize, NULL);
        }
        else
        {
            sprintf(Command, "%s/bin/endmqtrc", mqcMQConnectivity.MQInstallationPath);

            rc = execl(Command, "endmqtrc", "-a", NULL);
        }

        /* We should never get here */
        mqc_systemAPIError("execl", rc, errno);
        exit(rc);
    }

	waitpid(rc, NULL, 0);

    rc = 0;

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

