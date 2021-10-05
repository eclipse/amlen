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

int mqc_connectIMA(void)
{
    int rc;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    mqcMQConnectivity.ima.pConn = ismc_createConnection();
    TRACE(MQC_ISMC_TRACE, "ismc_createConnection <%p>\n", mqcMQConnectivity.ima.pConn);

    if (!mqcMQConnectivity.ima.pConn)
    {
        rc = mqc_ISMCError("ismc_createConnection");
        goto MOD_EXIT;
    }

    ismc_setStringProperty(mqcMQConnectivity.ima.pConn, "ClientID",
                           mqcMQConnectivity.ima.pIMAClientID);
    ismc_setStringProperty(mqcMQConnectivity.ima.pConn, "Protocol",
                           mqcMQConnectivity.ima.pIMAProtocol);
    ismc_setStringProperty(mqcMQConnectivity.ima.pConn, "Server",
                           mqcMQConnectivity.ima.pIMAAddress);
    ismc_setIntProperty(mqcMQConnectivity.ima.pConn, "Port",
                        mqcMQConnectivity.ima.IMAPort, VT_Integer);
    ismc_setIntProperty(mqcMQConnectivity.ima.pConn, "KeepAlive", 0, VT_Integer);

    rc = ismc_connect(mqcMQConnectivity.ima.pConn);
    TRACE(MQC_ISMC_TRACE, "ismc_connect rc(%d)\n", rc);

    if (rc)
    {
        rc = mqc_ISMCError("ismc_connect");
        goto MOD_EXIT;
    }

    mqcMQConnectivity.ima.flags |= IMA_CONNECTED;

    ismc_setErrorListener(mqcMQConnectivity.ima.pConn, mqc_errorListener, NULL);
    TRACE(MQC_ISMC_TRACE, "ismc_setErrorListener\n");

    rc = ismc_startConnection(mqcMQConnectivity.ima.pConn);
    TRACE(MQC_ISMC_TRACE, "ismc_startConnection rc(%d)\n", rc);

    if (rc)
    {
        rc = mqc_ISMCError("ismc_startConnection");
        goto MOD_EXIT;
    }

MOD_EXIT:

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_disconnectIMA(void)
{
    int rc = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (mqcMQConnectivity.ima.pConn)
    {
        if (mqcMQConnectivity.ima.flags & IMA_CONNECTED)
        {
            rc = ismc_closeConnection(mqcMQConnectivity.ima.pConn);
            TRACE(MQC_ISMC_TRACE, "ismc_closeConnection rc(%d)\n", rc);
            if (rc) mqc_ISMCError("ismc_closeConnection");

            mqcMQConnectivity.ima.flags &= ~IMA_CONNECTED;
        }

        rc = ismc_free(mqcMQConnectivity.ima.pConn);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
        mqcMQConnectivity.ima.pConn = NULL;
        if (rc) mqc_ISMCError("ismc_free");
    }

    rc = 0;
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_createProducerIMA(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    int transacted, ackmode;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    /* We create a non-transacted and a transacted session */
    /* This allows us to put non transactional MQ messages outside of an IMA transaction */

    /* Non-transacted session */
    /* Defect 29600 - set an ackmode so NP QoS 1 messages are not downgraded to QoS 0 */
    transacted = 0;
    ackmode    = SESSION_AUTO_ACKNOWLEDGE;

    pRuleQM -> pNonTranSess = ismc_createSession(mqcMQConnectivity.ima.pConn, transacted, ackmode);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession pNonTranSess <%p>\n", pRuleQM -> pNonTranSess);

    if (!pRuleQM -> pNonTranSess)
    {
        rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_createSession", "");
        goto MOD_EXIT;
    }

    pRuleQM -> pNonTranMessage = ismc_createMessage(pRuleQM -> pNonTranSess, MTYPE_BytesMessage);
    TRACE(MQC_ISMC_TRACE, "ismc_createMessage pNonTranMessage <%p>\n", pRuleQM -> pNonTranMessage); 

    if (!pRuleQM -> pNonTranMessage)
    {
        rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_createMessage", "");
        goto MOD_EXIT;
    }

    ism_common_allocAllocBuffer(&pRuleQM -> pNonTranMessage -> body, 64, 0);

    /* Transacted session */
    transacted = 1;
    ackmode    = SESSION_TRANSACTED_GLOBAL;

    pRuleQM -> pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, transacted, ackmode);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession pSess <%p>\n", pRuleQM -> pSess);

    if (!pRuleQM -> pSess)
    {
        rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_createSession", "");
        goto MOD_EXIT;
    }

    pRuleQM -> pMessage = ismc_createMessage(pRuleQM -> pSess, MTYPE_BytesMessage);
    TRACE(MQC_ISMC_TRACE, "ismc_createMessage pMessage <%p>\n", pRuleQM -> pMessage); 

    if (!pRuleQM -> pMessage)
    {
        rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_createMessage", "");
        goto MOD_EXIT;
    }

    ism_common_allocAllocBuffer(&pRuleQM -> pMessage -> body, 64, 0);

    /* Non wildcard rules use ismc_send() which require a producer */
    if (NONWILD_IMA_PRODUCE_RULE(pRule))
    {
        if (IMA_TOPIC_PRODUCE_RULE(pRule))
        {
            pRuleQM -> pDest = ismc_createTopic(pRule -> pDestination);
            TRACE(MQC_ISMC_TRACE, "ismc_createTopic topic '%s' <%p>\n", pRule -> pDestination, pRuleQM -> pDest); 

            if (!pRuleQM -> pDest)
            {
                rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_createTopic", pRule -> pDestination);
                goto MOD_EXIT;
            }
        }
        else
        {
            pRuleQM -> pDest = ismc_createQueue(pRule -> pDestination);
            TRACE(MQC_ISMC_TRACE, "ismc_createQueue queue '%s' <%p>\n", pRule -> pDestination, pRuleQM -> pDest); 

            if (!pRuleQM -> pDest)
            {
                rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_createQueue", pRule -> pDestination);
                goto MOD_EXIT;
            }
        }

        /* Non-transacted session */
        pRuleQM -> pNonTranProd = ismc_createProducer(pRuleQM -> pNonTranSess, pRuleQM -> pDest);
        TRACE(MQC_ISMC_TRACE, "ismc_createProducer pNonTranProd <%p>\n", pRuleQM -> pNonTranProd);

        if (!pRuleQM -> pNonTranProd)
        {
            rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_createProducer", pRule -> pDestination);
            goto MOD_EXIT;
        }

        /* Transacted session */
        pRuleQM -> pProd = ismc_createProducer(pRuleQM -> pSess, pRuleQM -> pDest);
        TRACE(MQC_ISMC_TRACE, "ismc_createProducer pProd <%p>\n", pRuleQM -> pProd);

        if (!pRuleQM -> pProd)
        {
            rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_createProducer", pRule -> pDestination);
            goto MOD_EXIT;
        }
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_deleteProducerIMA(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if (NONWILD_IMA_PRODUCE_RULE(pRule))
    {
        if (pRuleQM -> pNonTranProd)
        {
            rc = ismc_closeProducer(pRuleQM -> pNonTranProd);
            TRACE(MQC_ISMC_TRACE, "ismc_closeProducer rc(%d)\n", rc);
            if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_closeProducer", "");

            rc = ismc_free(pRuleQM -> pNonTranProd);
            TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
            pRuleQM -> pNonTranProd = NULL;
            if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_free", "");
        }
        if (pRuleQM -> pProd)
        {
            rc = ismc_closeProducer(pRuleQM -> pProd);
            TRACE(MQC_ISMC_TRACE, "ismc_closeProducer rc(%d)\n", rc);
            if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_closeProducer", "");

            rc = ismc_free(pRuleQM -> pProd);
            TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
            pRuleQM -> pProd = NULL;
            if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_free", "");
        }
        if (pRuleQM -> pDest)
        {
            rc = ismc_free(pRuleQM -> pDest);
            TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
            pRuleQM -> pDest = NULL;
            if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_free", "");
        }
    }

    if (pRuleQM -> pNonTranMessage)
    {
        ismc_freeMessage(pRuleQM -> pNonTranMessage);
        TRACE(MQC_ISMC_TRACE, "ismc_freeMessage\n");
        pRuleQM -> pNonTranMessage = NULL;
        if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_freeMessage", "");
    }
    if (pRuleQM -> pMessage)
    {
        ismc_freeMessage(pRuleQM -> pMessage);
        TRACE(MQC_ISMC_TRACE, "ismc_freeMessage\n");
        pRuleQM -> pMessage = NULL;
        if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_freeMessage", "");
    }
    if (pRuleQM -> pNonTranSess)
    {
        rc = ismc_closeSession(pRuleQM -> pNonTranSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", rc);
        if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_closeSession", "");

        rc = ismc_free(pRuleQM -> pNonTranSess);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
        pRuleQM -> pNonTranSess = NULL;
        if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_free", "");
    }
    if (pRuleQM -> pSess)
    {
        rc = ismc_closeSession(pRuleQM -> pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", rc);
        if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_closeSession", "");

        rc = ismc_free(pRuleQM -> pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
        pRuleQM -> pSess = NULL;
        if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_free", "");
    }

    rc = 0;
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_createRuleConsumerIMA(mqcRule_t * pRule)
{
    int rc = 0;
    int ruleindex = pRule -> index;
    int transacted, ackmode;
    char subname[40];

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d\n", __func__, ruleindex);

    transacted = 1;
    ackmode    = SESSION_TRANSACTED_GLOBAL;

    pRule -> pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, transacted, ackmode);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession <%p>\n", pRule -> pSess);

    if (!pRule -> pSess)
    {
        rc = mqc_ISMCRuleError(pRule, "ismc_createSession", "");
        goto MOD_EXIT;
    }

    ismc_setIntProperty(pRule -> pSess, "RequestOrderID", true, VT_Byte);

    if (IMA_TOPIC_CONSUME_RULE(pRule))
    {
        sprintf(subname, "__MQConnectivity.%.8d.SUB", pRule -> index);
        pRule -> pConsumer = ismc_subscribe_with_options(pRule -> pSess,
                                                         pRule -> pSource,
                                                         subname,
                                                         NULL,
                                                         pRule -> flags & RULE_USE_NOLOCAL ? 1 : 0,
                                                         pRule -> MaxMessages);
        TRACE(MQC_ISMC_TRACE, "ismc_subscribe_with_options '%s' '%s' (%d) <%p>\n",
              pRule -> pSource, subname, pRule -> MaxMessages, pRule -> pConsumer);

        if (!pRule -> pConsumer)
        {
            rc = mqc_ISMCRuleError(pRule, "ismc_subscribe_with_options", subname);
            goto MOD_EXIT;
        }
    }
    else
    {
        pRule -> pDest = ismc_createQueue(pRule -> pSource);
        TRACE(MQC_ISMC_TRACE, "ismc_createQueue queue '%s' <%p>\n", pRule -> pSource, pRule -> pDest);

        if (!pRule -> pDest)
        {
            rc = mqc_ISMCRuleError(pRule, "ismc_createQueue", pRule -> pSource);
            goto MOD_EXIT;
        }

        pRule -> pConsumer = ismc_createConsumer(pRule -> pSess,
                                                 pRule -> pDest,
                                                 NULL,
                                                 pRule -> flags & RULE_USE_NOLOCAL ? 1 : 0);
        TRACE(MQC_ISMC_TRACE, "ismc_createConsumer '%s' <%p>\n",
              pRule -> pSource, pRule -> pConsumer);

        if (!pRule -> pConsumer)
        {
            rc = mqc_ISMCRuleError(pRule, "ismc_createConsumer", pRule -> pSource);
            goto MOD_EXIT;
        }
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d rc=%d\n", __func__, ruleindex, rc);
    return rc;
}

int mqc_deleteRuleConsumerIMA(mqcRule_t * pRule, bool requested)
{
    int rc = 0;
    int ruleindex = pRule -> index;
    char subname[40];

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d\n", __func__, ruleindex);

    if (pRule -> pDest)
    {
        rc = ismc_free(pRule -> pDest);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
        pRule -> pDest = NULL;
        if (rc) mqc_ISMCRuleError(pRule, "ismc_free", "");
    }

    if (pRule -> pConsumer)
    {
        rc = ismc_closeConsumer(pRule -> pConsumer);
        TRACE(MQC_ISMC_TRACE, "ismc_closeConsumer rc(%d)\n", rc);
        if (rc) mqc_ISMCRuleError(pRule, "ismc_closeConsumer", "");

        rc = ismc_free(pRule -> pConsumer);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
        pRule -> pConsumer = NULL;
        if (rc) mqc_ISMCRuleError(pRule, "ismc_free", "");

        if (requested && IMA_TOPIC_CONSUME_RULE(pRule))
        {
            sprintf(subname, "__MQConnectivity.%.8d.SUB", pRule -> index);
            rc = ismc_unsubscribe(pRule -> pSess, subname);
            TRACE(MQC_ISMC_TRACE, "ismc_unsubscribe '%s' rc(%d)\n", subname, rc);
            if (rc) mqc_ISMCRuleError(pRule, "ismc_unsubscribe", "");
        }
    }

    if (pRule -> pSess)
    {
        rc = ismc_closeSession(pRule -> pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", rc);
        if (rc) mqc_ISMCRuleError(pRule, "ismc_closeSession", "");

        rc = ismc_free(pRule -> pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
        pRule -> pSess = NULL;
        if (rc) mqc_ISMCRuleError(pRule, "ismc_free", "");
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d rc=%d\n", __func__, ruleindex, rc);
    return rc;
}

int mqc_createRuleQMSessionIMA(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    int transacted, ackmode;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    transacted = 1;
    ackmode    = SESSION_TRANSACTED_GLOBAL;

    pRuleQM -> pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, transacted, ackmode);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession <%p>\n", pRuleQM -> pSess);

    if (!pRuleQM -> pSess)
    {
        rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_createSession", "");
        goto MOD_EXIT;
    }

    ismc_setIntProperty(pRuleQM -> pSess, "RequestOrderID", true, VT_Byte);

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_deleteRuleQMSessionIMA(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if (pRuleQM -> pSess)
    {
        rc = ismc_closeSession(pRuleQM -> pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", rc);
        if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_closeSession", "");

        rc = ismc_free(pRuleQM -> pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
        pRuleQM -> pSess = NULL;
        if (rc) mqc_ISMCRuleQMError(pRuleQM, "ismc_free", "");
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

