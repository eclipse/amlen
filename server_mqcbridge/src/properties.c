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

/* Macros */
#define MQC_TRACE_MQSETMP(Type, ValueLength, pValue, CompCode, Reason)                 \
    switch (Type)                                                                      \
    {                                                                                  \
    case MQTYPE_STRING:                                                                \
        TRACE(MQC_MQAPI_TRACE,                                                         \
              "MQSETMP(%s) Type(MQTYPE_STRING) Value(%.*s) CompCode(%d) Reason(%d)\n", \
              name, ValueLength, (PMQCHAR)pValue,  CompCode, Reason);                  \
        break;                                                                         \
    case MQTYPE_BOOLEAN:                                                               \
        TRACE(MQC_MQAPI_TRACE,                                                         \
              "MQSETMP(%s) Type(MQTYPE_BOOLEAN) Value(%d) CompCode(%d) Reason(%d)\n",  \
              name, *(PMQBOOL)pValue, CompCode, Reason);                               \
        break;                                                                         \
    case MQTYPE_INT8:                                                                  \
        TRACE(MQC_MQAPI_TRACE,                                                         \
              "MQSETMP(%s) Type(MQTYPE_INT8) Value(%d) CompCode(%d) Reason(%d)\n",     \
              name, *(PMQINT8)pValue, CompCode, Reason);                               \
        break;                                                                         \
    case MQTYPE_INT16:                                                                 \
        TRACE(MQC_MQAPI_TRACE,                                                         \
              "MQSETMP(%s) Type(MQTYPE_INT16) Value(%d) CompCode(%d) Reason(%d)\n",    \
              name, *(PMQINT16)pValue, CompCode, Reason);                              \
        break;                                                                         \
    case MQTYPE_INT32:                                                                 \
        TRACE(MQC_MQAPI_TRACE,                                                         \
              "MQSETMP(%s) Type(MQTYPE_INT32) Value(%d) CompCode(%d) Reason(%d)\n",    \
              name, *(PMQINT32)pValue, CompCode, Reason);                              \
        break;                                                                         \
    case MQTYPE_INT64:                                                                 \
        TRACE(MQC_MQAPI_TRACE,                                                         \
              "MQSETMP(%s) Type(MQTYPE_INT64) Value(%ld) CompCode(%d) Reason(%d)\n",   \
              name, *(PMQINT64)pValue, CompCode, Reason);                              \
        break;                                                                         \
    case MQTYPE_FLOAT32:                                                               \
        TRACE(MQC_MQAPI_TRACE,                                                         \
              "MQSETMP(%s) Type(MQTYPE_FLOAT32) Value(%f) CompCode(%d) Reason(%d)\n",  \
              name, *(PMQFLOAT32)pValue, CompCode, Reason);                            \
        break;                                                                         \
    case MQTYPE_FLOAT64:                                                               \
        TRACE(MQC_MQAPI_TRACE,                                                         \
              "MQSETMP(%s) Type(MQTYPE_FLOAT64) Value(%f) CompCode(%d) Reason(%d)\n",  \
              name, *(PMQFLOAT64)pValue, CompCode, Reason);                            \
        break;                                                                         \
    default:                                                                           \
        TRACE(MQC_MQAPI_TRACE,                                                         \
              "MQSETMP(%s) Type(%d) CompCode(%d) Reason(%d)\n",                        \
              name, Type, CompCode, Reason);                                           \
        break;                                                                         \
    }

/* Function prototypes */
int mqc_IMAStreamToMQ(mqcRuleQM_t * pRuleQM,
                      ismc_message_t * pMessage,
                      PMQLONG pBufferLength, PPMQVOID ppBuffer);

int mqc_IMAMapToMQ(mqcRuleQM_t * pRuleQM,
                   ismc_message_t * pMessage,
                   PMQLONG pBufferLength, PPMQVOID ppBuffer);

int mqc_MQStreamToIMA(mqcRuleQM_t * pRuleQM,
                      ismc_message_t * pMessage,
                      MQLONG BufferLength, PMQVOID pBuffer);

int mqc_MQMapToIMA(mqcRuleQM_t * pRuleQM,
                   ismc_message_t * pMessage,
                   MQLONG BufferLength, PMQVOID pBuffer);

/* Functions */
int mqc_setMQProperties(mqcRuleQM_t * pRuleQM,
                        ismc_message_t * pMessage, PMQMD pMD,
                        PMQLONG pBufferLength, PPMQVOID ppBuffer)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG Type, ValueLength = 0, CompCode, Reason;
    MQSMPO smpo = {MQSMPO_DEFAULT};
    MQCHARV vs = {MQCHARV_DEFAULT};
    MQPD pd = {MQPD_DEFAULT};
    PMQVOID pValue = NULL;
    int mtype, i, domain;
    ism_field_t field;
    const char * name;
    ism_obj_hdr_t * obj = (ism_obj_hdr_t *)pMessage;

    MQBOOL Boolean;
    MQINT8 Int8;
    MQINT16 Int16;
    MQINT32 Int32;
    MQINT64 Int64;
    MQFLOAT32 Float32;
    MQFLOAT64 Float64;
    bool known;
    bool settype = true;

    concat_alloc_t dest;
    char xbuf[64];

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    memset(&dest, 0, sizeof(dest));
    dest.buf = xbuf;
    dest.len = sizeof(xbuf);

    /* Reset message handle */
    if (pRuleQM -> hMsg != MQHM_UNUSABLE_HMSG)
    {
        rc = mqc_deleteMessageHandleQM(pRuleQM, pRuleQM -> hPubConn, true);
        if (rc) goto MOD_EXIT;
    }

    *pBufferLength = ismc_getContentSize(pMessage);
    *ppBuffer = pMessage -> body.buf;

    mtype = ismc_getMessageType(pMessage);

    switch (mtype)
    {
    case MTYPE_BytesMessage:
        TRACE(MQC_NORMAL_TRACE, "Message Type (MTYPE_BytesMessage)\n");
        memcpy(pMD -> Format, MQFMT_NONE, MQ_FORMAT_LENGTH);

        /* The default JMS type for MQFMT_NONE is "jms_bytes" */
        settype = false;
        //pValue = "jms_bytes";
        //ValueLength = strlen("jms_bytes");
        break;
    case MTYPE_MapMessage:
        TRACE(MQC_NORMAL_TRACE, "Message Type (MTYPE_MapMessage)\n");
        memcpy(pMD -> Format, MQFMT_STRING, MQ_FORMAT_LENGTH);

        rc = mqc_IMAMapToMQ(pRuleQM, pMessage, pBufferLength, ppBuffer);
        if (rc) goto MOD_EXIT;

        pValue = "jms_map";
        ValueLength = strlen("jms_map");
        break;
    case MTYPE_ObjectMessage:
        TRACE(MQC_NORMAL_TRACE, "Message Type (MTYPE_ObjectMessage)\n");
        memcpy(pMD -> Format, MQFMT_NONE, MQ_FORMAT_LENGTH);

        pValue = "jms_object";
        ValueLength = strlen("jms_object");
        break;
    case MTYPE_StreamMessage:
        TRACE(MQC_NORMAL_TRACE, "Message Type (MTYPE_StreamMessage)\n");
        memcpy(pMD -> Format, MQFMT_STRING, MQ_FORMAT_LENGTH);

        rc = mqc_IMAStreamToMQ(pRuleQM, pMessage, pBufferLength, ppBuffer);
        if (rc) goto MOD_EXIT;

        pValue = "jms_stream";
        ValueLength = strlen("jms_stream");
        break;
    case MTYPE_TextMessage:
        TRACE(MQC_NORMAL_TRACE, "Message Type (MTYPE_TextMessage)\n");
        memcpy(pMD -> Format, MQFMT_STRING, MQ_FORMAT_LENGTH);
        pMD -> CodedCharSetId = 1208;

        /* The default JMS type for MQFMT_STRING is "jms_text" */
        settype = false;
        // pValue = "jms_text";
        // ValueLength = strlen("jms_text");
        break;
    case MTYPE_Message:
    default:
        if (mtype == MTYPE_Message)
        {
            TRACE(MQC_NORMAL_TRACE, "Message Type (MTYPE_Message)\n");
        }
        else
        {
            TRACE(MQC_NORMAL_TRACE, "Message Type (%d)\n", mtype);
        }
        memcpy(pMD -> Format, MQFMT_NONE, MQ_FORMAT_LENGTH);

        pValue = "jms_none";
        ValueLength = strlen("jms_none");
        break;
    }

    /* Set MQ JMS message type property "mcd.Msd" */
    if (settype)
    {
        rc = mqc_createMessageHandleQM(pRuleQM, pRuleQM -> hPubConn);
        if (rc) goto MOD_EXIT;

        name = "mcd.Msd";
        vs.VSPtr = (MQPTR) name;
        vs.VSLength = strlen(name);

        Type = MQTYPE_STRING;

        MQSETMP(pRuleQM -> hPubConn,
                pRuleQM -> hMsg,
                &smpo,
                &vs,
                &pd,
                Type,
                ValueLength,
                pValue,
                &CompCode,
                &Reason);
        MQC_TRACE_MQSETMP(Type, ValueLength, pValue, CompCode, Reason);

        if (CompCode == MQCC_FAILED)
        {
            mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQSETMP", (char *)name, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }
    }

    /* Set MQ JMS destination "jms.Dst" for queues.                   */
    /* (Topic destinations are set automatically).                    */
    /* Destination queues are of the form (no queue manager name):    */
    /* queue:///<queue name>                                          */
    if (MQ_QUEUE_PRODUCE_RULE(pRule))
    {
        if (pRuleQM -> hMsg == MQHM_UNUSABLE_HMSG)
        {
            rc = mqc_createMessageHandleQM(pRuleQM, pRuleQM -> hPubConn);
            if (rc) goto MOD_EXIT;
        }

        name = "jms.Dst";
        vs.VSPtr = (MQPTR) name;
        vs.VSLength = strlen(name);

        ism_common_allocBufferCopyLen(&dest, "queue:///", strlen("queue:///"));
        ism_common_allocBufferCopyLen(&dest, pRule -> pDestination, strlen(pRule -> pDestination));

        Type = MQTYPE_STRING;
        pValue = dest.buf;
        ValueLength = dest.used;

        MQSETMP(pRuleQM -> hPubConn,
                pRuleQM -> hMsg,
                &smpo,
                &vs,
                &pd,
                Type,
                ValueLength,
                pValue,
                &CompCode,
                &Reason);
        MQC_TRACE_MQSETMP(Type, ValueLength, pValue, CompCode, Reason);

        if (CompCode == MQCC_FAILED)
        {
            mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQSETMP", (char *)name, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }
    }

    /* Set MQTT QoS property */
    if (ismc_getQuality(pMessage) == 1)
    {
        if (pRuleQM -> hMsg == MQHM_UNUSABLE_HMSG)
        {
            rc = mqc_createMessageHandleQM(pRuleQM, pRuleQM -> hPubConn);
            if (rc) goto MOD_EXIT;
        }

        name = "mqtt.qos";
        vs.VSPtr = (MQPTR) name;
        vs.VSLength = strlen(name);

        Type = MQTYPE_INT32;
        Int32 = 1;
        pValue = &Int32;
        ValueLength = sizeof(Int32);

        MQSETMP(pRuleQM -> hPubConn,
                pRuleQM -> hMsg,
                &smpo,
                &vs,
                &pd,
                Type,
                ValueLength,
                pValue,
                &CompCode,
                &Reason);
        MQC_TRACE_MQSETMP(Type, ValueLength, pValue, CompCode, Reason);

        if (CompCode == MQCC_FAILED)
        {
            mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQSETMP", (char *)name, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }
    }

    pMD -> Priority = ismc_getPriority(pMessage);

    pMD -> Persistence = (ismc_getDeliveryMode(pMessage) == ISMC_PERSISTENT) ?
        MQPER_PERSISTENT : MQPER_NOT_PERSISTENT;

    for (i = 0; (ism_common_getPropertyIndex(obj -> props, i, &name) == 0); i++)
    {
        rc = ismc_getProperty(pMessage, name, &field);
        TRACE(MQC_NORMAL_TRACE, "name '%s' type(%d) len(%u)\n", name, field.type, field.len);

        known = true;

        switch(field.type)
        {
        case VT_Null:
            pValue = NULL;
            Type = MQTYPE_NULL;
            ValueLength = 0;
            break;
        case VT_String:
            Type = MQTYPE_STRING;
            pValue = field.val.s;
            ValueLength = strlen(field.val.s);
            break;
        case VT_ByteArray:
            Type = MQTYPE_BYTE_STRING;
            pValue = field.val.s;
            ValueLength = field.len;
            break;
        case VT_Boolean:
            Type = MQTYPE_BOOLEAN;
            Boolean = field.val.i;
            pValue = &Boolean;
            ValueLength = sizeof(Boolean);
            break;
        case VT_Byte:
        case VT_UByte:
            Type = MQTYPE_INT8;
            Int8 = field.val.i;
            pValue = &Int8;
            ValueLength = sizeof(Int8);
            break;
        case VT_Short:
        case VT_UShort:
            Type = MQTYPE_INT16;
            Int16 = field.val.i;
            pValue = &Int16;
            ValueLength = sizeof(Int16);
            break;
        case VT_Integer:
        case VT_UInt:
            Type = MQTYPE_INT32;
            Int32 = field.val.i;
            pValue = &Int32;
            ValueLength = sizeof(Int32);
            break;
        case VT_Long:
        case VT_ULong:
            Type = MQTYPE_INT64;
            Int64 = field.val.l;
            pValue = &Int64;
            ValueLength = sizeof(Int64);
            break;
        case VT_Float:
            Type = MQTYPE_FLOAT32;
            Float32 = field.val.f;
            pValue = &Float32;
            ValueLength = sizeof(Float32);
            break;
        case VT_Double:
            Type = MQTYPE_FLOAT64;
            Float64 = field.val.d;
            pValue = &Float64;
            ValueLength = sizeof(Float64);
            break;
        case VT_Name:
        case VT_NameIndex:
        case VT_Char:
        case VT_Map:
        case VT_Unset:
        default:
            known = false;
            break;
        }

        if (known)
        {
            if ((strcmp(name, "JMSDestination") == 0) ||
                (strcmp(name, "JMSMessageID") == 0))
            {
            }
            else
            {
                if (pRuleQM -> hMsg == MQHM_UNUSABLE_HMSG)
                {
                    rc = mqc_createMessageHandleQM(pRuleQM, pRuleQM -> hPubConn);
                    if (rc) goto MOD_EXIT;
                }

                /* MQ JMSReplyTo are of the form:                     */
                /* topic://<topic string>                             */
                /* queue://<queue manager name>/<queue name>          */
                if (strcmp(name, "JMSReplyTo") == 0)
                {
                    if (ismc_getReplyTo(pMessage, &domain))
                    {
                        dest.used = 0;

                        if (domain == ismc_Topic)
                        {
                            ism_common_allocBufferCopyLen(&dest, "topic://", strlen("topic://"));
                        }
                        else if (domain == ismc_Queue)
                        {
                            ism_common_allocBufferCopyLen(&dest, "queue://", strlen("queue://"));
                            ism_common_allocBufferCopyLen(&dest, pQM -> pQMName, strlen(pQM -> pQMName));
                            ism_common_allocBufferCopyLen(&dest, "/", strlen("/"));
                        }
                        ism_common_allocBufferCopyLen(&dest, pValue, ValueLength);
                        pValue = dest.buf;
                        ValueLength = dest.used;
                    }
                }

                vs.VSPtr = (MQPTR) name;
                vs.VSLength = strlen(name);

                MQSETMP(pRuleQM -> hPubConn,
                        pRuleQM -> hMsg,
                        &smpo,
                        &vs,
                        &pd,
                        Type,
                        ValueLength,
                        pValue,
                        &CompCode,
                        &Reason);
                MQC_TRACE_MQSETMP(Type, ValueLength, pValue, CompCode, Reason);

                if (CompCode == MQCC_FAILED)
                {
                    mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQSETMP", (char *)name, Reason);
                    rc = RC_RULEQM_RECONNECT;
                    goto MOD_EXIT;
                }
            }
        }
    }

    uint64_t expireMillis = (uint64_t)(ismc_getExpiration(pMessage));

    // Pass message expiry on to MQ
    if (expireMillis != 0)
    {
        pMD -> Expiry = 0;

        uint64_t nowMillis = ism_common_currentTimeNanos()/1000000UL;

        if (expireMillis > nowMillis)
        {
            // MessageDescriptor expiry is expressed as an interval in tenths of seconds
            // convert.
            pMD -> Expiry = (MQLONG)((expireMillis-nowMillis)/100UL);
        }

        // MQ will not accept an expiry of 0, so we translate this to 1 (tenth of a second)
        if (pMD -> Expiry == 0)
        {
            pMD -> Expiry = 1;
        }
        // MQ will accept a max expiry of 999999999, so we translate any bigger to 999999999
        else if (pMD -> Expiry > 999999999)
        {
            pMD -> Expiry = 999999999;
        }

    }

MOD_EXIT:
    ism_common_freeAllocBuffer(&dest);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_IMAStreamToMQ(mqcRuleQM_t * pRuleQM,
                      ismc_message_t * pMessage,
                      PMQLONG pBufferLength, PPMQVOID ppBuffer)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    concat_alloc_t in;
    ism_field_t field;
    char element[64];
    int i;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    pRuleQM -> buf.used = 0;

    memset(&in, 0, sizeof(in));
    in.buf = pMessage -> body.buf;
    in.used = pMessage -> body.len;
    in.pos = 1;

    ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<stream>", strlen("<stream>"));

    while (ism_protocol_getObjectValue(&in, &field) == 0)
    {
        switch (field.type)
        {
        case VT_Null:
            break;
        case VT_String:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt>", strlen("<elt>"));
            /* Does our string have '<' or '>' characters ? */
            if (strchr(field.val.s, '<') || strchr(field.val.s, '>'))
            {
                for (i = 0; i < strlen(field.val.s); i++)
                {
                    if (field.val.s[i] == '<')
                    {
                        ism_common_allocBufferCopyLen(&pRuleQM -> buf, "&lt;", strlen("&lt;"));
                    }
                    else if (field.val.s[i] == '>')
                    {
                        ism_common_allocBufferCopyLen(&pRuleQM -> buf, "&gt;", strlen("&gt;"));
                    }
                    else
                    {
                        ism_common_allocBufferCopyLen(&pRuleQM -> buf, &field.val.s[i], 1);
                    }
                }
            }
            else
            {
                ism_common_allocBufferCopyLen(&pRuleQM -> buf, field.val.s, strlen(field.val.s));
            }
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "</elt>", strlen("</elt>"));
            break;
        case VT_ByteArray:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt dt='bin.hex'>", strlen("<elt dt='bin.hex'>"));
            for (i = 0; i < field.len; i++)
            {
                sprintf(element, "%02X", field.val.s[i]);
                ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            }
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "</elt>", strlen("</elt>"));
            break;
        case VT_Boolean:
            sprintf(element, "<elt dt='boolean'>%d</elt>", field.val.i);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Byte:
        case VT_UByte:
            sprintf(element, "<elt dt='i1'>%d</elt>", field.val.i);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Short:
        case VT_UShort:
            sprintf(element, "<elt dt='i2'>%d</elt>", field.val.i);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Integer:
        case VT_UInt:
            sprintf(element, "<elt dt='i4'>%d</elt>", field.val.i);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Long:
        case VT_ULong:
            sprintf(element, "<elt dt='i8'>%ld</elt>", field.val.l);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Float:
            sprintf(element, "<elt dt='r4'>%f</elt>", field.val.f);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Double:
            sprintf(element, "<elt dt='r8'>%f</elt>", field.val.d);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Name:
        case VT_NameIndex:
            break;
        case VT_Char:
            sprintf(element, "<elt dt='char'>%c</elt>", field.val.i);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Map:
        case VT_Unset:
        default:
            break;
        }
    }

    ism_common_allocBufferCopyLen(&pRuleQM -> buf, "</stream>", strlen("</stream>"));

    *pBufferLength = pRuleQM -> buf.used;
    *ppBuffer = pRuleQM -> buf.buf;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_IMAMapToMQ(mqcRuleQM_t * pRuleQM,
                   ismc_message_t * pMessage,
                   PMQLONG pBufferLength, PPMQVOID ppBuffer)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    concat_alloc_t in, name;
    ism_field_t field;
    char element[64], xbuf[64];
    int i;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    pRuleQM -> buf.used = 0;

    memset(&in, 0, sizeof(in));
    in.buf = pMessage -> body.buf;
    in.used = pMessage -> body.len;
    in.pos = 1;

    memset(&name, 0, sizeof(name));
    name.buf = xbuf;
    name.len = sizeof(xbuf);

    ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<map>", strlen("<map>"));

    while (ism_protocol_getObjectValue(&in, &field) == 0)
    {
        switch (field.type)
        {
        case VT_Null:
            break;
        case VT_String:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt name=\"", strlen("<elt name=\""));
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, name.buf, name.used);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "\">", strlen("\">"));
            /* Does our string have '<' or '>' characters ? */
            if (strchr(field.val.s, '<') || strchr(field.val.s, '>'))
            {
                for (i = 0; i < strlen(field.val.s); i++)
                {
                    if (field.val.s[i] == '<')
                    {
                        ism_common_allocBufferCopyLen(&pRuleQM -> buf, "&lt;", strlen("&lt;"));
                    }
                    else if (field.val.s[i] == '>')
                    {
                        ism_common_allocBufferCopyLen(&pRuleQM -> buf, "&gt;", strlen("&gt;"));
                    }
                    else
                    {
                        ism_common_allocBufferCopyLen(&pRuleQM -> buf, &field.val.s[i], 1);
                    }
                }
            }
            else
            {
                ism_common_allocBufferCopyLen(&pRuleQM -> buf, field.val.s, strlen(field.val.s));
            }
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "</elt>", strlen("</elt>"));
            break;
        case VT_ByteArray:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt name=\"", strlen("<elt name=\""));
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, name.buf, name.used);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "\" dt='bin.hex'>", strlen("\" dt='bin.hex'>"));
            for (i = 0; i < field.len; i++)
            {
                sprintf(element, "%02X", field.val.s[i]);
                ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            }
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "</elt>", strlen("</elt>"));
            break;
        case VT_Boolean:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt name=\"", strlen("<elt name=\""));
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, name.buf, name.used);
            sprintf(element, "\" dt='boolean'>%d</elt>", field.val.i);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Byte:
        case VT_UByte:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt name=\"", strlen("<elt name=\""));
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, name.buf, name.used);
            sprintf(element, "\" dt='i1'>%d</elt>", field.val.i);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Short:
        case VT_UShort:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt name=\"", strlen("<elt name=\""));
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, name.buf, name.used);
            sprintf(element, "\" dt='i2'>%d</elt>", field.val.i);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Integer:
        case VT_UInt:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt name=\"", strlen("<elt name=\""));
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, name.buf, name.used);
            sprintf(element, "\" dt='i4'>%d</elt>", field.val.i);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Long:
        case VT_ULong:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt name=\"", strlen("<elt name=\""));
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, name.buf, name.used);
            sprintf(element, "\" dt='i8'>%ld</elt>", field.val.l);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Float:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt name=\"", strlen("<elt name=\""));
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, name.buf, name.used);
            sprintf(element, "\" dt='r4'>%f</elt>", field.val.f);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Double:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt name=\"", strlen("<elt name=\""));
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, name.buf, name.used);
            sprintf(element, "\" dt='r8'>%f</elt>", field.val.d);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Name:
            name.used = 0;
            ism_common_allocBufferCopyLen(&name, field.val.s, strlen(field.val.s));
            break;
        case VT_NameIndex:
            break;
        case VT_Char:
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, "<elt name=\"", strlen("<elt name=\""));
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, name.buf, name.used);
            sprintf(element, "\" dt='char'>%c</elt>", field.val.i);
            ism_common_allocBufferCopyLen(&pRuleQM -> buf, element, strlen(element));
            break;
        case VT_Map:
        case VT_Unset:
        default:
            break;
        }
    }

    ism_common_allocBufferCopyLen(&pRuleQM -> buf, "</map>", strlen("</map>"));

    *pBufferLength = pRuleQM -> buf.used;
    *ppBuffer = pRuleQM -> buf.buf;

    ism_common_freeAllocBuffer(&name);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_setIMAProperties(mqcRuleQM_t * pRuleQM,
                         ismc_message_t * pMessage, PMQMD pMD,
                         MQLONG BufferLength, PMQVOID pBuffer, char * topic)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG Type, Length, CompCode, Reason;
    MQIMPO impo = {MQIMPO_DEFAULT};
    MQCHARV inqvs = {MQPROP_INQUIRE_ALL};
    MQPD pd = {MQPD_DEFAULT};

    concat_alloc_t name;
    char xbuf[64];

    ism_field_t field;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    memset(&name, 0, sizeof(name));
    name.buf = xbuf;
    name.len = sizeof(xbuf);

    ismc_clearContent(pMessage);
    ismc_clearProperties(pMessage);

    /* Set initial message type */
    pMessage -> msgtype = (memcmp(pMD -> Format, MQFMT_STRING, MQ_FORMAT_LENGTH) == 0) ?
        MTYPE_TextMessage : MTYPE_BytesMessage;

    /* MQMD properties */

    int msgPersistence =
            ((pMD -> Persistence == MQPER_PERSISTENT) ? ISMC_PERSISTENT
                    : ISMC_NON_PERSISTENT);
    int msgQuality = ((pMD -> Persistence == MQPER_PERSISTENT) ? 2 : 0);
    int64_t ttlMillis = ((pMD->Expiry == MQEI_UNLIMITED) ? 0 : (int64_t)(pMD->Expiry) * 100L);

    TRACE(MQC_MQAPI_TRACE,
          "Priority(%d) DeliveryMode(%d) Quality(%d) TTL(%ld)\n",
          pMD -> Priority,
          msgPersistence,
          msgQuality,
          ttlMillis);

    ismc_setPriority(pMessage, pMD -> Priority);
    ismc_setDeliveryMode(pMessage, msgPersistence);
    ismc_setQuality(pMessage, msgQuality);
    ismc_setExpiration(pMessage, 0); // Make sure no explicit expiry set
    ismc_setTimeToLive(pMessage, ttlMillis);

    /* Properties */
    impo.Options |= MQIMPO_INQ_NEXT;

    while (1)
    {
        impo.ReturnedName.VSPtr = name.buf;
        /* Allow space for NULL terminator */
        impo.ReturnedName.VSBufSize = name.len - 1;

        Type = MQTYPE_AS_SET;

        MQINQMP(pRuleQM -> hSubConn,
                pRuleQM -> hMsg,
                &impo,
                &inqvs,
                &pd,
                &Type,
                pRuleQM -> buf.len - 1, /* Allow space for NULL terminator */
                pRuleQM -> buf.buf,
                &Length,
                &CompCode,
                &Reason);
        TRACE(MQC_MQAPI_TRACE,
              "MQINQMP CompCode(%d) Reason(%d)\n",
              CompCode, Reason);

        if (CompCode == MQCC_FAILED)
        {
            if (Reason == MQRC_PROPERTY_NOT_AVAILABLE) break;

            if (Reason == MQRC_PROPERTY_NAME_TOO_BIG)
            {
                /* Allow space for NULL terminator */
                ism_common_allocAllocBuffer(&name, impo.ReturnedName.VSLength + 1, 0);
                name.used = 0;
                continue;
            }

            if (Reason == MQRC_PROPERTY_VALUE_TOO_BIG)
            {
                /* Allow space for NULL terminator */
                ism_common_allocAllocBuffer(&pRuleQM -> buf, Length + 1, 0);
                pRuleQM -> buf.used = 0;
                continue;
            }

            mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQINQMP", "", Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }

        /* Null terminate name */
        ((char *)impo.ReturnedName.VSPtr)[impo.ReturnedName.VSLength] = 0;

        TRACE(MQC_NORMAL_TRACE, "Name '%s' Type(%d)\n",
              (char *) impo.ReturnedName.VSPtr, Type);

        /* JMS message type */
        if ((impo.ReturnedName.VSLength == strlen("mcd.Msd")) &&
            (memcmp(impo.ReturnedName.VSPtr, "mcd.Msd", strlen("mcd.Msd")) == 0))
        {
            if ((Length == strlen("jms_bytes")) &&
                (memcmp(pRuleQM -> buf.buf, "jms_bytes", strlen("jms_bytes")) == 0))
            {
                pMessage -> msgtype = MTYPE_BytesMessage;
            }
            else if ((Length == strlen("jms_map")) &&
                     (memcmp(pRuleQM -> buf.buf, "jms_map", strlen("jms_map")) == 0))
            {
                pMessage -> msgtype = MTYPE_MapMessage;

                rc = mqc_MQMapToIMA(pRuleQM, pMessage, BufferLength, pBuffer);
                if (rc) goto MOD_EXIT;
            }
            else if ((Length == strlen("jms_object")) &&
                     (memcmp(pRuleQM -> buf.buf, "jms_object", strlen("jms_object")) == 0))
            {
                pMessage -> msgtype = MTYPE_ObjectMessage;
            }
            else if ((Length == strlen("jms_stream")) &&
                     (memcmp(pRuleQM -> buf.buf, "jms_stream", strlen("jms_stream")) == 0))
            {
                pMessage -> msgtype = MTYPE_StreamMessage;

                rc = mqc_MQStreamToIMA(pRuleQM, pMessage, BufferLength, pBuffer);
                if (rc) goto MOD_EXIT;
            }
            else if ((Length == strlen("jms_text")) &&
                     (memcmp(pRuleQM -> buf.buf, "jms_text", strlen("jms_text")) == 0))
            {
                pMessage -> msgtype = MTYPE_TextMessage;
            }
            else if ((Length == strlen("jms_none")) &&
                     (memcmp(pRuleQM -> buf.buf, "jms_none", strlen("jms_none")) == 0))
            {
                pMessage -> msgtype = MTYPE_Message;
            }

            continue;
        }

        /* JMSType */
        if ((impo.ReturnedName.VSLength == strlen("mcd.Type")) &&
            (memcmp(impo.ReturnedName.VSPtr, "mcd.Type", strlen("mcd.Type")) == 0))
        {
            /* Null terminate value */
            pRuleQM -> buf.buf[Length] = 0;
            ismc_setTypeString(pMessage, pRuleQM -> buf.buf);

            continue;
        }

        /* JMS properties */
        if ((impo.ReturnedName.VSLength >= strlen("jms.")) &&
            (memcmp(impo.ReturnedName.VSPtr, "jms.", strlen("jms.")) == 0))
        {
            TRACE(MQC_MQAPI_TRACE,
                  "Processing JMS properties\n");

            if ((impo.ReturnedName.VSLength == strlen("jms.Rto")) &&
                (memcmp((char *)impo.ReturnedName.VSPtr + 4, "Rto", strlen("Rto")) == 0))
            {
                /* Null terminate value */
                pRuleQM -> buf.buf[Length] = 0;

                if ((Length >= strlen("topic://")) &&
                    (memcmp(pRuleQM -> buf.buf, "topic://", strlen("topic://")) == 0))
                {
                    ismc_setReplyTo(pMessage, pRuleQM -> buf.buf + strlen("topic://"), ismc_Topic);
                }
                else if ((Length >= strlen("queue://")) &&
                         (memcmp(pRuleQM -> buf.buf, "queue://", strlen("queue://")) == 0))
                {
                    ismc_setReplyTo(pMessage, pRuleQM -> buf.buf + strlen("queue://"), ismc_Queue);
                }
            }
            else if ((impo.ReturnedName.VSLength == strlen("jms.Tms")) &&
                     (memcmp((char *)impo.ReturnedName.VSPtr + 4, "Tms", strlen("Tms")) == 0))
            {
                char * endptr;
                /* Null terminate value */
                pRuleQM -> buf.buf[Length] = 0;
                ismc_setTimestamp(pMessage, strtol(pRuleQM -> buf.buf, &endptr, 10));
            }
            else if ((impo.ReturnedName.VSLength == strlen("jms.Cid")) &&
                     (memcmp((char *)impo.ReturnedName.VSPtr + 4, "Cid", strlen("Cid")) == 0))
            {
                /* Null terminate value */
                pRuleQM -> buf.buf[Length] = 0;
                ismc_setCorrelationID(pMessage, pRuleQM -> buf.buf);
            }
            else if ((impo.ReturnedName.VSLength == strlen("jms.Gid")) &&
                     (memcmp((char *)impo.ReturnedName.VSPtr + 4, "Gid", strlen("Gid")) == 0))
            {
                /* Null terminate value */
                pRuleQM -> buf.buf[Length] = 0;
                rc = ismc_setStringProperty(pMessage, "JMSXGroupID", pRuleQM -> buf.buf);

            }
            else if ((impo.ReturnedName.VSLength == strlen("jms.Seq")) &&
                     (memcmp((char *)impo.ReturnedName.VSPtr + 4, "Seq", strlen("Seq")) == 0))
            {
                /* Null terminate value */
                pRuleQM -> buf.buf[Length] = 0;
                ismc_setIntProperty(pMessage, "JMSXGroupSeq", atoi(pRuleQM -> buf.buf), VT_Integer);
            }
            /* Unused: jms.Dst, jms.Dlv, jms.Uci */

            continue;
        }

        /* MQTT properties */
        if ((impo.ReturnedName.VSLength >= strlen("mqtt.")) &&
            (memcmp(impo.ReturnedName.VSPtr, "mqtt.", strlen("mqtt.")) == 0))
        {
            TRACE(MQC_MQAPI_TRACE,
                  "Processing MQTT properties\n");

            if ((impo.ReturnedName.VSLength == strlen("mqtt.qos")) &&
                (memcmp((char *)impo.ReturnedName.VSPtr + 5, "qos", strlen("qos")) == 0))
            {
                if (*(PMQINT32)pRuleQM -> buf.buf == 1)
                {
                    TRACE(MQC_MQAPI_TRACE, "Quality(1)\n");
                    ismc_setQuality(pMessage, 1);
                }
            }
            /* Unused: mqtt.clientId, mqtt.msgid */

            continue;
        }

        /* User properties */
        switch (Type)
        {
        case MQTYPE_BOOLEAN:
            field.type = VT_Boolean;
            field.val.i = *(PMQBOOL)pRuleQM -> buf.buf;
            ismc_setProperty(pMessage, impo.ReturnedName.VSPtr, &field);
            break;
        case MQTYPE_BYTE_STRING:
            field.type = VT_ByteArray;
            field.len = Length;
            field.val.s = pRuleQM -> buf.buf;
            ismc_setProperty(pMessage, impo.ReturnedName.VSPtr, &field);
            break;
        case MQTYPE_INT8:
            ismc_setIntProperty(pMessage, impo.ReturnedName.VSPtr, *(PMQINT8)pRuleQM -> buf.buf, VT_Byte);
            break;
        case MQTYPE_INT16:
            ismc_setIntProperty(pMessage, impo.ReturnedName.VSPtr, *(PMQINT16)pRuleQM -> buf.buf, VT_Short);
            break;
        case MQTYPE_INT32:
            ismc_setIntProperty(pMessage, impo.ReturnedName.VSPtr, *(PMQINT32)pRuleQM -> buf.buf, VT_Integer);
            break;
        case MQTYPE_INT64:
            field.type = VT_Long;
            field.val.l = *(PMQINT64)pRuleQM -> buf.buf;
            ismc_setProperty(pMessage, impo.ReturnedName.VSPtr, &field);
            break;
        case MQTYPE_FLOAT32:
            field.type = VT_Float;
            field.val.f = *(PMQFLOAT32)pRuleQM -> buf.buf;
            ismc_setProperty(pMessage, impo.ReturnedName.VSPtr, &field);
            break;
        case MQTYPE_FLOAT64:
            field.type = VT_Double;
            field.val.d = *(PMQFLOAT64)pRuleQM -> buf.buf;
            ismc_setProperty(pMessage, impo.ReturnedName.VSPtr, &field);
            break;
        case MQTYPE_STRING:
            if (strcmp(impo.ReturnedName.VSPtr, "MQTopicString") == 0)
            {
                if (WILD_IMA_PRODUCE_RULE(pRule))
                {
                    /* Map wildcard topic name */
                    TRACE(MQC_NORMAL_TRACE, "From topic '%.*s'\n", Length, pRuleQM -> buf.buf);

                    if (Length > (strlen(pRule -> pSource) - 1))
                    {
                        memmove(topic + (strlen(pRule -> pDestination) - 1),
                                pRuleQM -> buf.buf + (strlen(pRule -> pSource) - 1),
                                Length - (strlen(pRule -> pSource) - 1));
                    }
                    memcpy(topic, pRule -> pDestination, strlen(pRule -> pDestination) - 1);
                    /* null terminate */
                    topic[Length - strlen(pRule -> pSource) + strlen(pRule -> pDestination)] = '\0';

                    TRACE(MQC_NORMAL_TRACE, "To topic '%s'\n", topic);
                }
            }
            else
            {
                /* Null terminate value */
                pRuleQM -> buf.buf[Length] = 0;
                rc = ismc_setStringProperty(pMessage, impo.ReturnedName.VSPtr, pRuleQM -> buf.buf);
            }
            break;
        case MQTYPE_NULL:
            break;
        default:
            break;
        }
    }

    /* Set message content */
    switch (pMessage -> msgtype)
    {
    /* Convert MQFMT_STRING messages that were not JMS map or stream messages */
    case MTYPE_TextMessage:
        while (1)
        {
            MQXCNVC(pRuleQM -> hSubConn,
                    MQDCC_NONE,
                    pMD -> CodedCharSetId,
                    BufferLength,
                    pBuffer,
                    1208,
                    pMessage -> body.len,
                    pMessage -> body.buf,
                    &pMessage -> body.used,
                    &CompCode,
                    &Reason);
            TRACE(MQC_MQAPI_TRACE,
                  "MQXCNVC CompCode(%d) Reason(%d)\n", CompCode, Reason);
        
            if (Reason == MQRC_CONVERTED_MSG_TOO_BIG)
            {
                ism_common_allocAllocBuffer(&pMessage -> body, pMessage -> body.len * 2, 0);
                pMessage -> body.used = 0;
                continue;
            }
        
            if (CompCode == MQCC_FAILED)
            {
               mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQXCNVC", "", Reason);
               rc = RC_RULEQM_RECONNECT;
               goto MOD_EXIT;
            }
        
            break;
        }
        break;
    case MTYPE_MapMessage:
    case MTYPE_StreamMessage:
        /* Already set message content */
        break;
    case MTYPE_BytesMessage:
    case MTYPE_ObjectMessage:
    default:
        /* No conversion */
        ismc_setContent(pMessage, pBuffer, BufferLength);
        break;
    }

MOD_EXIT:
    ism_common_freeAllocBuffer(&name);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_MQStreamToIMA(mqcRuleQM_t * pRuleQM,
                      ismc_message_t * pMessage,
                      MQLONG BufferLength, PMQVOID pBuffer)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    char * pChar, * pNextChar, * pSubChar;
    int byte, i;

    ism_field_t field;

    concat_alloc_t buf;
    char xbuf[64];

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    memset(&buf, 0, sizeof(buf));
    buf.buf = xbuf;
    buf.len = sizeof(xbuf);

    pMessage -> body.used = 1;
    pMessage -> body.buf[0] = 0x9E;

    /* <stream><elt dt='boolean'>1</elt></stream> */

    if (BufferLength < strlen("<stream></stream>"))
    {
        TRACE(MQC_NORMAL_TRACE, "Length(%d) < <stream></stream>\n", BufferLength);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }

    if (memcmp(pBuffer, "<stream></stream>", strlen("<stream></stream>")) == 0)
    {
        TRACE(MQC_NORMAL_TRACE, "Empty stream message\n");
        goto MOD_EXIT;
    }

    if (BufferLength < strlen("<stream><elt></elt></stream>"))
    {
        TRACE(MQC_NORMAL_TRACE, "Length(%d) < <stream><elt></elt></stream>\n", BufferLength);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }

    pChar = pBuffer + strlen("<stream><elt");

    while (1)
    {
        if (*pChar == '>')
        {
            pChar++;
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_String value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Does our string have '<' or '>' characters ? */
            if (strstr(pChar, "&lt;") || strstr(pChar, "&gt;"))
            {
                ism_common_allocBufferCopyLen(&buf, pChar, pNextChar - pChar + 1);

                pSubChar = strstr(buf.buf, "&lt;");
                while (pSubChar)
                {
                    *pSubChar = '<';
                    memcpy(pSubChar + 1, pSubChar + strlen("&lt;"),
                           buf.used - (pSubChar - buf.buf) - strlen("lt;"));
                    pSubChar = strstr(pSubChar, "&lt;");
                }

                pSubChar = strstr(buf.buf, "&gt;");
                while (pSubChar)
                {
                    *pSubChar = '>';
                    memcpy(pSubChar + 1, pSubChar + strlen("&gt;"),
                           buf.used - (pSubChar - buf.buf) - strlen("gt;"));
                    pSubChar = strstr(pSubChar, "&gt;");
                }

                field.val.s = buf.buf;
                buf.used = 0;
            }
            else
            {
                field.val.s = pChar;
            }
            /* Add VT_String to IMA message */
            field.type = VT_String;
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='boolean'>", strlen(" dt='boolean'>")) == 0)
        {
            pChar += strlen(" dt='boolean'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Boolean value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Boolean to IMA message */
            field.type = VT_Boolean;
            field.val.i = atoi(pChar);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='i1'>", strlen(" dt='i1'>")) == 0)
        {
            pChar += strlen(" dt='i1'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Byte value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Byte to IMA message */
            field.type = VT_Byte;
            field.val.i = atoi(pChar);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='bin.hex'>", strlen(" dt='bin.hex'>")) == 0)
        {
            pChar += strlen(" dt='bin.hex'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_ByteArray value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }

            /* Add VT_ByteArray to IMA message */
            field.type = VT_ByteArray;
            field.len = (pNextChar - pChar) / 2;

            ism_common_allocAllocBuffer(&buf, field.len, 0);
            buf.used = 0;

            for (i = 0; i < field.len; i++)
            {
                sscanf(pChar + 2 * i, "%02X", &byte);
                buf.buf[i] = byte;
            }

            field.val.s = buf.buf;
            ism_protocol_putObjectValue(&pMessage -> body, &field);
        }
        else if (memcmp(pChar, " dt='char'>", strlen(" dt='char'>")) == 0)
        {
            pChar += strlen(" dt='char'>");
            pNextChar = pChar + 1;
            /* Add VT_Char to IMA message */
            field.type = VT_Char;
            field.val.i = *pChar;
            ism_protocol_putObjectValue(&pMessage -> body, &field);
        }
        else if (memcmp(pChar, " dt='r8'>", strlen(" dt='r8'>")) == 0)
        {
            pChar += strlen(" dt='r8'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Double value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Double to IMA message */
            field.type = VT_Double;
            sscanf(pChar, "%lf", &field.val.d);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='r4'>", strlen(" dt='r4'>")) == 0)
        {
            pChar += strlen(" dt='r4'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Float value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Float to IMA message */
            field.type = VT_Float;
            sscanf(pChar, "%f", &field.val.f);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='i4'>", strlen(" dt='i4'>")) == 0)
        {
            pChar += strlen(" dt='i4'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Integer value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Integer to IMA message */
            field.type = VT_Integer;
            field.val.i = atoi(pChar);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='i8'>", strlen(" dt='i8'>")) == 0)
        {
            pChar += strlen(" dt='i8'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Long value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Long to IMA message */
            field.type = VT_Long;
            field.val.l = atoi(pChar);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='i2'>", strlen(" dt='i2'>")) == 0)
        {
            pChar += strlen(" dt='i2'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Short value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Short to IMA message */
            field.type = VT_Short;
            field.val.i = atoi(pChar);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else
        {
            TRACE(MQC_NORMAL_TRACE, "Unexpected type '%.10s'\n", pChar);
            rc = RC_RULEQM_DISABLE;
            goto MOD_EXIT;
        }

        /* Skip to next element */
        if (BufferLength - (pNextChar - (char *)pBuffer) < strlen("</elt></stream>"))
        {
            TRACE(MQC_NORMAL_TRACE, "Remaining Length(%ld) < </elt></stream>\n",
                  BufferLength - (pNextChar - (char *)pBuffer));
            rc = RC_RULEQM_DISABLE;
            goto MOD_EXIT;
        }
        if (memcmp(pNextChar, "</elt></stream>", strlen("</elt></stream>")) == 0)
        {
            break;
        }
        if (BufferLength - (pNextChar - (char *)pBuffer) < strlen("</elt><elt></elt></stream>"))
        {
            TRACE(MQC_NORMAL_TRACE, "Remaining Length(%ld) < </elt><elt></elt></stream>\n",
                  BufferLength - (pNextChar - (char *)pBuffer));
            rc = RC_RULEQM_DISABLE;
            goto MOD_EXIT;
        }
        pChar = pNextChar + strlen("</elt><elt");
    }

MOD_EXIT:
    ism_common_freeAllocBuffer(&buf);

    if (rc)
    {
        mqc_messageError(pRuleQM, "Stream", BufferLength, pBuffer);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_MQMapToIMA(mqcRuleQM_t * pRuleQM,
                   ismc_message_t * pMessage,
                   MQLONG BufferLength, PMQVOID pBuffer)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    char * pChar, * pNextChar, * pSubChar;
    int byte, i;

    ism_field_t field;

    concat_alloc_t buf;
    char xbuf[64];

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    memset(&buf, 0, sizeof(buf));
    buf.buf = xbuf;
    buf.len = sizeof(xbuf);

    pMessage -> body.used = 1;
    pMessage -> body.buf[0] = 0x9F;

    /* <map><elt name="MyBoolean" dt='boolean'>1</elt></map> */

    if (BufferLength < strlen("<map></map>"))
    {
        TRACE(MQC_NORMAL_TRACE, "Length(%d) < <map></map>\n", BufferLength);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }

    if (memcmp(pBuffer, "<map></map>", strlen("<map></map>")) == 0)
    {
        TRACE(MQC_NORMAL_TRACE, "Empty map message\n");
        goto MOD_EXIT;
    }

    if (BufferLength < strlen("<map><elt name=\"\"></elt></map>"))
    {
        TRACE(MQC_NORMAL_TRACE, "Length(%d) < <map><elt name=\"\"></elt></map>\n", BufferLength);
        rc = RC_RULEQM_DISABLE;
        goto MOD_EXIT;
    }

    pChar = pBuffer + strlen("<map><elt name=\"");

    while (1)
    {
        /* Extract name */
        pNextChar = memchr(pChar, '"', BufferLength - (pChar - (char *)pBuffer));
        if (pNextChar == NULL)
        {
            TRACE(MQC_NORMAL_TRACE, "Name has no \"\n");
            rc = RC_RULEQM_DISABLE;
            goto MOD_EXIT;
        }
        *pNextChar = 0;
        /* Add VT_Name to IMA message*/
        field.type = VT_Name;
        field.val.s = pChar;
        ism_protocol_putObjectValue(&pMessage -> body, &field);
        *pNextChar = '"';
        pChar = pNextChar + 1;

        if (*pChar == '>')
        {
            pChar++;
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_String value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Does our string have '<' or '>' characters ? */
            if (strstr(pChar, "&lt;") || strstr(pChar, "&gt;"))
            {
                ism_common_allocBufferCopyLen(&buf, pChar, pNextChar - pChar + 1);

                pSubChar = strstr(buf.buf, "&lt;");
                while (pSubChar)
                {
                    *pSubChar = '<';
                    memcpy(pSubChar + 1, pSubChar + strlen("&lt;"),
                           buf.used - (pSubChar - buf.buf) - strlen("lt;"));
                    pSubChar = strstr(pSubChar, "&lt;");
                }

                pSubChar = strstr(buf.buf, "&gt;");
                while (pSubChar)
                {
                    *pSubChar = '>';
                    memcpy(pSubChar + 1, pSubChar + strlen("&gt;"),
                           buf.used - (pSubChar - buf.buf) - strlen("gt;"));
                    pSubChar = strstr(pSubChar, "&gt;");
                }

                field.val.s = buf.buf;
                buf.used = 0;
            }
            else
            {
                field.val.s = pChar;
            }
            /* Add VT_String to IMA message */
            field.type = VT_String;
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='boolean'>", strlen(" dt='boolean'>")) == 0)
        {
            pChar += strlen(" dt='boolean'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Boolean value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Boolean to IMA message */
            field.type = VT_Boolean;
            field.val.i = atoi(pChar);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='i1'>", strlen(" dt='i1'>")) == 0)
        {
            pChar += strlen(" dt='i1'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Byte value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Byte to IMA message */
            field.type = VT_Byte;
            field.val.i = atoi(pChar);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='bin.hex'>", strlen(" dt='bin.hex'>")) == 0)
        {
            pChar += strlen(" dt='bin.hex'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_ByteArray value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }

            /* Add VT_ByteArray to IMA message */
            field.type = VT_ByteArray;
            field.len = (pNextChar - pChar) / 2;

            ism_common_allocAllocBuffer(&buf, field.len, 0);
            buf.used = 0;

            for (i = 0; i < field.len; i++)
            {
                sscanf(pChar + 2 * i, "%02X", &byte);
                buf.buf[i] = byte;
            }

            field.val.s = buf.buf;
            ism_protocol_putObjectValue(&pMessage -> body, &field);
        }
        else if (memcmp(pChar, " dt='char'>", strlen(" dt='char'>")) == 0)
        {
            pChar += strlen(" dt='char'>");
            pNextChar = pChar + 1;
            /* Add VT_Char to IMA message */
            field.type = VT_Char;
            field.val.i = *pChar;
            ism_protocol_putObjectValue(&pMessage -> body, &field);
        }
        else if (memcmp(pChar, " dt='r8'>", strlen(" dt='r8'>")) == 0)
        {
            pChar += strlen(" dt='r8'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Double value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Double to IMA message */
            field.type = VT_Double;
            sscanf(pChar, "%lf", &field.val.d);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='r4'>", strlen(" dt='r4'>")) == 0)
        {
            pChar += strlen(" dt='r4'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Float value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Float to IMA message */
            field.type = VT_Float;
            sscanf(pChar, "%f", &field.val.f);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='i4'>", strlen(" dt='i4'>")) == 0)
        {
            pChar += strlen(" dt='i4'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Integer value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Integer to IMA message */
            field.type = VT_Integer;
            field.val.i = atoi(pChar);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='i8'>", strlen(" dt='i8'>")) == 0)
        {
            pChar += strlen(" dt='i8'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Long value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Long to IMA message */
            field.type = VT_Long;
            field.val.l = atoi(pChar);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else if (memcmp(pChar, " dt='i2'>", strlen(" dt='i2'>")) == 0)
        {
            pChar += strlen(" dt='i2'>");
            pNextChar = memchr(pChar, '<', BufferLength - (pChar - (char *)pBuffer));
            if (pNextChar == NULL)
            {
                TRACE(MQC_NORMAL_TRACE, "VT_Short value has no <\n");
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
            *pNextChar = 0;
            /* Add VT_Short to IMA message */
            field.type = VT_Short;
            field.val.i = atoi(pChar);
            ism_protocol_putObjectValue(&pMessage -> body, &field);
            *pNextChar = '<';
        }
        else
        {
            TRACE(MQC_NORMAL_TRACE, "Unexpected type '%.10s'\n", pChar);
            rc = RC_RULEQM_DISABLE;
            goto MOD_EXIT;
        }

        /* Skip to next element */
        if (BufferLength - (pNextChar - (char *)pBuffer) < strlen("</elt></map>"))
        {
            TRACE(MQC_NORMAL_TRACE, "Remaining Length(%ld) < </elt></map>\n",
                  BufferLength - (pNextChar - (char *)pBuffer));
            rc = RC_RULEQM_DISABLE;
            goto MOD_EXIT;
        }
        if (memcmp(pNextChar, "</elt></map>", strlen("</elt></map>")) == 0)
        {
            break;
        }
        if (BufferLength - (pNextChar - (char *)pBuffer) < strlen("</elt><elt></elt></map>"))
        {
            TRACE(MQC_NORMAL_TRACE, "Remaining Length(%ld) < </elt><elt></elt></map>\n",
                  BufferLength - (pNextChar - (char *)pBuffer));
            rc = RC_RULEQM_DISABLE;
            goto MOD_EXIT;
        }
        pChar = pNextChar + strlen("</elt><elt name=\"");
    }

MOD_EXIT:
    ism_common_freeAllocBuffer(&buf);

    if (rc)
    {
        mqc_messageError(pRuleQM, "Map", BufferLength, pBuffer);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

