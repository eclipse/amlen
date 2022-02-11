/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

#ifndef TRACE_COMP
#define TRACE_COMP Protocol
#endif
#include <iotmonitor.h>
#include <transport.h>
#include <mqtt.h>
#include <assert.h>
#include <ctype.h>

extern ismEngine_SessionHandle_t g_monitor_session;

/*
 * We always use two message areas, one for properties and the other for payload
 */
static ismMessageAreaType_t MsgAreas[2] = {
    ismMESSAGE_AREA_PROPERTIES,
    ismMESSAGE_AREA_PAYLOAD
};

#define DAYS_TO_NANOS(_days)  ((24*60*60*1000000000UL)*(_days))
#define NANOS_TO_DAYS(_nanos) ((_nanos)/(24*60*60*1000000000UL))

#define RECONCILE_RULE_EXPECTED_ELEMENTS 3

typedef struct tag_iot_reconcileRule_t
{
    char *clientIdSpec;     /* ELEMENT 1 */
    int retainType;         /* ELEMENT 2 */
    uint64_t expiry;        /* ELEMENT 3 */
    uint64_t expiryAgoTime; /* Nano time representing expiry days earlier than now */
    int matches;            /* How many retained messages matched this rule */
    int connectMatches;     /* How many 'Connect' retained messages matched this rule */
    int disconnectMatches;  /* How many 'Disconnect' retained messages matched this rule */
    int unknownMatches;     /* How many retained messages of an unknown type matched this rule */
    int cleared;            /* How many matching messages were cleared (JM_Clear sent) */
    int expired;            /* How many matching messages were expired (actually with JM_Clear sent) */
    int disconnected;       /* How many matching messages were disconnected (JM_Disconnect sent) */
    int untouched;          /* How many matching messages were left untouched */
} iot_reconcileRule_t;

typedef struct tag_iot_reconcileCallbackContext_t {
    int entryNo;
    char *topicFilter;
    iot_reconcileRule_t *rules;
    int ruleCount;
    int ruleMax;
    int seenMessages;            /* How many retained messages were seen */
    int unmatchedMessages;       /* How many retained messages matched no rules */
    int otherServerMessages;     /* How many retained messages were seen from other servers */
    char *firstOtherServer;      /* First 'other server' we saw while processing */
    ism_ts_t *ts;
    const char *serverUID;
    uint64_t nowNanos;
    uint64_t shutdownTime;
    int notifyRC;
    const char *notifyReason;
    ism_json_parse_t parseObj;
    ism_json_entry_t ents[10];
} iot_reconcileCallbackContext_t;

/*
 * Return a string given an action enumeration
 */
const char * actionName(int action) {
    /* Get the name of the action */
    switch (action) {
    case JM_Connect:     return MONITOR_MSG_ACTION_CONNECT;
    case JM_Disconnect:  return MONITOR_MSG_ACTION_DISCONNECT;
    case JM_Failed:      return MONITOR_MSG_ACTION_FAILEDCONNECT;
    case JM_Active:      return MONITOR_MSG_ACTION_ACTIVE;
    case JM_Info:        return MONITOR_MSG_ACTION_INFO;
    default:             return MONITOR_MSG_ACTION_UNKNOWN;
    }
}

#define PROT_MQTT_BIN    1  /* Binary with native framing         */
#define PROT_MQTT_WSBIN  2  /* Binary with WebSockets framing     */
/*
 * Get the protocol in external format
 */
const char * ism_mqtt_externalProtocol(ism_transport_t * transport, char * buf) {
    *buf = 0;
    if (transport->protocol_family && !strcmp(transport->protocol_family, "mqtt")) {
        sprintf(buf, "mqtt%u", transport->pobj->mqtt_version);
        if (transport->pobj->prot == PROT_MQTT_WSBIN)
            strcat(buf, "-ws");
        return buf;
    }
    return transport->protocol_family;
}


/*
 * Create a JSON message for connect/disconnect notification
 */
int ism_iot_jsonMessage(concat_alloc_t * buf, int which,
                        ism_transport_t * transport, ism_json_parse_t *parseObj, ism_ts_t *ts, uint64_t msgTime,
                        int rc, const char * reason) {
    char tbuf[64];
    char tbuf2[64];
    char proto[16];
    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, buf, 0, JSON_OUT_COMPACT);

    /* Format current and connect times */
    assert(ts != NULL);
    ism_common_setTimestamp(ts, msgTime);
    ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 7, ISM_TFF_ISO8601);

    ism_json_startObject(jobj, NULL);
    ism_json_putStringItem(jobj, "Action", actionName(which));
    ism_json_putStringItem(jobj, "Time", tbuf);
    if (parseObj) {
        /*
         * Recreate from an existing parse object
         */
        const char *objString;
        int objInt;

        if ((objString = ism_json_getString(parseObj, "ClientAddr")))
            ism_json_putStringItem(jobj, "ClientAddr", objString);
        if ((objString = ism_json_getString(parseObj, "ClientID")))
            ism_json_putStringItem(jobj, "ClientID", objString);
        if ((objInt = ism_json_getInt(parseObj, "Port", -1)) != -1)
            ism_json_putIntegerItem(jobj, "Port", objInt);
        if ((objInt = ism_json_getInt(parseObj, "Secure", -1)) >= 0)
            ism_json_putBooleanItem(jobj, "Secure", objInt);
        if ((objString = ism_json_getString(parseObj, "Protocol")))
            ism_json_putStringItem(jobj, "Protocol", objString);
        if ((objString = ism_json_getString(parseObj, "User")))
            ism_json_putStringItem(jobj, "User", objString);
        if ((objString = ism_json_getString(parseObj, "CertName")))
            ism_json_putStringItem(jobj, "CertName", objString);

        if ((objString = ism_json_getString(parseObj, "ConnectTime"))) {
            ism_json_putStringItem(jobj, "ConnectTime", objString);
        } else if ((objString = ism_json_getString(parseObj, "Time"))) {
            ism_json_putStringItem(jobj, "ConnectTime", objString);
        }
        /* Extra fields for connect */
        if (which == JM_Connect) {
            if ((objInt = ism_json_getInt(parseObj, "Durable", -1)) >= 0)
                ism_json_putBooleanItem(jobj, "Durable", objInt);
            if ((objInt = ism_json_getInt(parseObj, "NewSession", -1)) >= 0)
                ism_json_putBooleanItem(jobj, "NewSession", objInt);
        }
        /* Put out closing code */
        if ((objInt = ism_json_getInt(parseObj, "CloseCode", -1)) >= 0)
            ism_json_putIntegerItem(jobj, "CloseCode", objInt);
        if ((objString = ism_json_getString(parseObj, "Reason")))
            ism_json_putStringItem(jobj, "Reason", objString);
    } else {
        /*
         * Create from a current transport object
         */
        assert(transport != NULL);
        ism_json_putStringItem(jobj, "ClientAddr", transport->client_addr);
        if (transport->clientID && *transport->clientID)
            ism_json_putStringItem(jobj, "ClientID", transport->clientID);
        ism_json_putIntegerItem(jobj, "Port", transport->serverport);
        ism_json_putBooleanItem(jobj, "Secure", transport->pobj->monitor_opt>>12);
        if (*transport->protocol_family)
            ism_json_putStringItem(jobj, "Protocol", ism_mqtt_externalProtocol(transport, proto));
        if (transport->userid && *transport->userid && strcmp(transport->userid, "use-token-auth"))
            ism_json_putStringItem(jobj, "User", transport->userid);
        if (transport->cert_name && *transport->cert_name)
            ism_json_putStringItem(jobj, "CertName", transport->cert_name);
        ism_common_setTimestamp(ts, transport->connect_time);
        ism_common_formatTimestamp(ts, tbuf2, sizeof tbuf, 7, ISM_TFF_ISO8601);
        ism_json_putStringItem(jobj, "ConnectTime", tbuf2);

        /* Extra fields for connect */
        if (which == JM_Connect && transport->protocol_family && !strcmp(transport->protocol_family, "mqtt")) {
            ism_json_putBooleanItem(jobj, "Durable", transport->pobj->cleansession);
            if (!transport->pobj->cleansession && !transport->pobj->session_existed) {
                ism_json_putBooleanItem(jobj, "NewSession", !transport->pobj->session_existed);
            }
        }
        /* Put out closing code */
        ism_json_putIntegerItem(jobj, "CloseCode", rc);
        if (reason)
            ism_json_putStringItem(jobj, "Reason", reason);
        else
            ism_json_putStringItem(jobj, "Reason", "");

        /* Put out stats on a disconnect */
        if (which == JM_Disconnect) {
            ism_json_putLongItem(jobj, "ReadBytes",transport->read_bytes);
            ism_json_putLongItem(jobj, "ReadMsg", transport->read_msg);
            ism_json_putLongItem(jobj, "WriteBytes", transport->write_bytes);
            ism_json_putLongItem(jobj, "WriteMsg", transport->write_msg);
        }
    }
    ism_json_endObject(jobj);
    return 0;
}


/*
 * Send a monitoring message, for now always at QoS=0
 */
void sendNotification(int which, ism_transport_t * transport, ism_json_parse_t *parseObj, ism_ts_t *ts, uint64_t msgTime,
                      char *monitor_topic, int retain, uint64_t expiryDays, int rc, const char * reason)
{
    if (monitor_topic && g_monitor_session) {

        char xbuf [4096];
        concat_alloc_t buf = {xbuf, sizeof xbuf};

        ismMessageHeader_t hdr = {0};
        ismEngine_MessageHandle_t msgh;
        size_t areasize[2];
        void * areaptr[2];
        if (retain) {
            switch (which) {
            case JM_Connect:
                break;
            default:
                retain = 0;
                break;
            case JM_Clear:
                assert(retain==1 && rc != ISMRC_ClientIDReused);
                /* drop through */
            case JM_Disconnect:
                if (retain==1 && rc != ISMRC_ClientIDReused) {
                    TRACE(8, "Unset retained on: %s\n", monitor_topic);
                    rc = ism_engine_unsetRetainedMessageOnDestination(g_monitor_session,
                            ismDESTINATION_TOPIC, monitor_topic, ismENGINE_UNSET_RETAINED_OPTION_NONE,
                            msgTime, NULL, NULL, 0, NULL);
                    if (rc >= ISMRC_Error) {
                        TRACE(6, "Unable to unset monitoring message: rc=%d msg=%s\n", rc, buf.buf);
                    }
                    retain = 0;
                }
                if (rc == ISMRC_ClientIDReused) {
                    retain = 0;
                }
                break;
            }
        }

        /* JM_Clear doesn't publish another message */
        if (which != JM_Clear) {
            /*
             * Make the content
             */
            buf.compact = 3;

            ism_iot_jsonMessage(&buf, which, transport, parseObj, ts, msgTime, rc, reason);

            areasize[1] = buf.used++;

            /* Put the topic into the properties */
            ism_protocol_putNameIndex(&buf, ID_Topic);
            ism_protocol_putStringValue(&buf, monitor_topic);
            if (retain) {
                ism_protocol_putNameIndex(&buf, ID_ServerTime);
                ism_protocol_putLongValue(&buf, msgTime);
                ism_protocol_putNameIndex(&buf, ID_OriginServer);
                ism_protocol_putStringValue(&buf, ism_common_getServerUID());
            }
            areaptr[1] = buf.buf;
            areaptr[0] = buf.buf+areasize[1]+1;
            areasize[0] = buf.used-(areasize[1]+1);

            /*
             * Set up the header
             */
            hdr.Persistence = (retain) ? ismMESSAGE_PERSISTENCE_PERSISTENT : ismMESSAGE_PERSISTENCE_NONPERSISTENT;
            hdr.Reliability = ismMESSAGE_RELIABILITY_AT_MOST_ONCE;
            hdr.Priority = 4;
            hdr.Flags = retain ? ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN : 0;
            hdr.MessageType = MTYPE_MQTT_Text;
            /* Set expiry to the requested number of days */
            hdr.Expiry = ism_common_getExpire(DAYS_TO_NANOS(expiryDays) + msgTime);

            /*
             * Create the message
             */
            rc = ism_engine_createMessage(&hdr, 2, MsgAreas, areasize, areaptr, &msgh);
            if (rc) {
                TRACE(6, "Unable to create monitoring message: rc=%d msg=%s\n", rc, buf.buf);
            } else {
                TRACE(8, "Send monitor message: topic=%s retain=%u body=%s\n", monitor_topic,
                      !!(hdr.Flags&ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN), buf.buf);
                rc = ism_engine_putMessageOnDestination(g_monitor_session,
                         ismDESTINATION_TOPIC, monitor_topic, NULL, msgh, NULL, 0, NULL);
                if (rc >= ISMRC_Error) {
                    TRACE(6, "Unable to send monitoring message: rc=%d msg=%s\n", rc, buf.buf);
                }
            }
            if (buf.inheap)
                ism_common_freeAllocBuffer(&buf);
        }
    }
}


/*
 * Create a JSON connect message
 */
void ism_iot_connectMsg(ism_transport_t * transport) {
    if (transport->pobj->monitor_opt&1) {
        ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
        sendNotification(JM_Connect, transport, NULL, ts, ism_common_currentTimeNanos(),
                         transport->pobj->monitor_topic, (transport->pobj->monitor_opt>>8) & 0xF,
                         MONITOR_MSG_DEFAULT_EXPIRATION_DAYS, 0, NULL);
        ism_common_closeTimestamp(ts);
    }
}

/*
 * Create a notification message for a failed connection.
 * This is not retained.
 */
void ism_iot_failedMsg(ism_transport_t * transport, int rc, const char * reason) {
    if (transport->pobj->monitor_opt&2) {
        ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
        sendNotification(JM_Failed, transport, NULL, ts, ism_common_currentTimeNanos(),
                         transport->pobj->monitor_topic, (transport->pobj->monitor_opt>>8) & 0xF,
                         MONITOR_MSG_DEFAULT_EXPIRATION_DAYS, rc, reason);
        ism_common_closeTimestamp(ts);
    }
}


/*
 * Create a JSON disconnect message
 */
void ism_iot_disconnectMsg(ism_transport_t * transport, int rc, const char * reason) {
    if (transport->pobj->monitor_opt&2) {
        ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
        sendNotification(JM_Disconnect, transport, NULL, ts, ism_common_currentTimeNanos(),
                         transport->pobj->monitor_topic, (transport->pobj->monitor_opt>>8) & 0xF,
                         MONITOR_MSG_DEFAULT_EXPIRATION_DAYS, rc, reason);
        ism_common_closeTimestamp(ts);
    }
}

bool reconcileCallback(ismEngine_ConsumerHandle_t hConsumer,
                      ismEngine_DeliveryHandle_t hDelivery,
                      ismEngine_MessageHandle_t hMessage,
                      uint32_t deliveryId,
                      ismMessageState_t state,
                      uint32_t destinationOptions,
                      ismMessageHeader_t *pMsgDetails,
                      uint8_t areaCount,
                      ismMessageAreaType_t areaTypes[areaCount],
                      size_t areaLengths[areaCount],
                      void * pAreaData[areaCount],
                      void * pContext,
                      ismEngine_DelivererContext_t * _delivererContext)
{
    iot_reconcileCallbackContext_t *context = *(iot_reconcileCallbackContext_t **)pContext;

    int rc = ISMRC_InvalidPayload;
    bool matchedRule = false;
    char *originServer = NULL;
    char *msgTopic = NULL; /* If this is NULL, we'll go through the motions but change nothing */
    uint64_t msgTime = 0;

    for(int propertyArea = 0; propertyArea < areaCount; propertyArea++) {
        if (areaTypes[propertyArea] == ismMESSAGE_AREA_PROPERTIES) {
            concat_alloc_t  props = {pAreaData[propertyArea], areaLengths[propertyArea], areaLengths[propertyArea]};
            ism_field_t field;

            field.val.s = NULL;
            ism_common_findPropertyID(&props, ID_OriginServer, &field);
            originServer = field.val.s;

            field.val.s = NULL;
            ism_common_findPropertyID(&props, ID_Topic, &field);
            msgTopic = field.val.s;

            field.val.l = 0;
            ism_common_findPropertyID(&props, ID_ServerTime, &field);
            msgTime = (uint64_t)field.val.l;
            break;
        }
    }

    assert(originServer != NULL);
    assert(msgTime != 0);

    #if 0 /* TESTING_ONLY */
    originServer = context->serverUID;
    #endif

    /* Only process messages from this server */
    if (originServer != NULL && strcmp(originServer, context->serverUID) == 0) {
        /* Find and parse the ismMESSAGE_AREA_PAYLOAD area handed to us */
        for(int payloadArea = 0; payloadArea < areaCount; payloadArea++) {
            if (areaTypes[payloadArea] == ismMESSAGE_AREA_PAYLOAD) {
                char *source = context->parseObj.source;
                size_t src_len = (size_t)(context->parseObj.src_len);

                /* Parse the incoming payload as JSON */
                if (areaLengths[payloadArea] > src_len) {
                    src_len = areaLengths[payloadArea];
                    source = ism_common_realloc(ISM_MEM_PROBE(ism_memory_protocol_misc,211),source, src_len);
                    assert(source != NULL);
                }

                context->parseObj.rc = 0;
                context->parseObj.ent_count = 0;
                context->parseObj.source = source;
                context->parseObj.src_len = (int)(areaLengths[payloadArea]);
                memcpy(context->parseObj.source,
                       pAreaData[payloadArea],
                       areaLengths[payloadArea]);

                rc = ism_json_parse(&context->parseObj);

                if (rc) {
                    TRACE(3, "Failed to parse monitoring message '%.*s'\n",
                          (int)areaLengths[payloadArea], (char *)pAreaData[payloadArea]);
                }

                /* remember how much buffer we have for next time */  
                context->parseObj.src_len = src_len;
            }
        }

        const char *clientId;
        const char *action;

        if (rc == OK && (clientId = ism_json_getString(&context->parseObj, MONITOR_MSG_VALUE_CLIENTID)) != NULL) {
            for(int ruleNo=0; !matchedRule && ruleNo < context->ruleCount; ruleNo++) {
                iot_reconcileRule_t *rule = &context->rules[ruleNo];

                if (rule->clientIdSpec == NULL || ism_common_match(clientId, rule->clientIdSpec)) {
                    uint64_t newMsgTime;
                    /* Make sure we will replace the current retained message */
                    if (context->shutdownTime > msgTime) {
                        newMsgTime = context->shutdownTime;
                    } else {
                        newMsgTime = msgTime + 1; /* Add on 1 nanosecond */
                    }
                    if ((action = ism_json_getString(&context->parseObj, MONITOR_MSG_VALUE_ACTION)) != NULL) {
                        if (strcmp(action, MONITOR_MSG_ACTION_CONNECT) == 0) {
                            rule->connectMatches++;
                            switch(rule->retainType) {
                                case 0:
                                    sendNotification(JM_Clear,
                                                     NULL, &context->parseObj,
                                                     context->ts, newMsgTime,
                                                     msgTopic, 1,
                                                     rule->expiry, context->notifyRC, context->notifyReason);
                                    rule->cleared++;
                                    break;
                                default:
                                    sendNotification(JM_Disconnect,
                                                     NULL, &context->parseObj,
                                                     context->ts, newMsgTime,
                                                     msgTopic, rule->retainType,
                                                     rule->expiry, context->notifyRC, context->notifyReason);
                                    rule->disconnected++;
                                    break;
                            }
                        } else if (strcmp(action, MONITOR_MSG_ACTION_DISCONNECT) == 0) {
                            rule->disconnectMatches++;
                            int clearRetained;
                            if (rule->retainType == 2) {
                                // We want to clear this message if it was published earlier than
                                // the expiry days to be imposed by this rule ago.
                                clearRetained = (msgTime < rule->expiryAgoTime) ? 2 : 0;

                                if (clearRetained) {
                                    uint64_t msgAgeDays = NANOS_TO_DAYS(context->nowNanos-msgTime);
                                    TRACE(5, "Clearing %lu day old %s message for client %s (rule->expiry %lu days).\n",
                                          msgAgeDays, action, clientId, rule->expiry);
                                }
                            } else {
                                clearRetained = 1;
                            }

                            if (clearRetained != 0) {
                                sendNotification(JM_Clear,
                                                 NULL, &context->parseObj,
                                                 context->ts, newMsgTime,
                                                 msgTopic, 1, rule->expiry,
                                                 context->notifyRC, context->notifyReason);

                                if (clearRetained == 1)
                                    rule->cleared++;
                                else
                                    rule->expired++;
                            } else {
                                rule->untouched++;
                            }
                        } else {
                            rule->unknownMatches++;
                            rule->untouched++;
                        }
                    }

                    matchedRule = true;
                    rule->matches++;
                }
            }
        }
    } else {
        context->otherServerMessages++;

        /* Remember the first 'other server' we see during reconciliation */
        if (context->firstOtherServer == NULL && originServer != NULL) {
            context->firstOtherServer = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),originServer);
        }
    }

    context->seenMessages++;
    if (!matchedRule) context->unmatchedMessages++;

    ism_engine_releaseMessage(hMessage);

    return true;
}

/*
 * Reconcile the notification messages retained during previous invocations.
 */
int ism_iot_reconcile(void) {
    int rc = OK;
    ism_prop_t *configProps = ism_common_getConfigProperties();
    iot_reconcileCallbackContext_t context = {0};
    ismEngine_MessagingStatistics_t engineStats;
    char *entry = NULL;

    assert(configProps != NULL);

    /* We get messaging statistics to give us the last server shutdown time */
    ism_engine_getMessagingStatistics(&engineStats);

    context.nowNanos = ism_common_currentTimeNanos();
    context.shutdownTime = (uint64_t)(engineStats.ServerShutdownTime);
    assert(context.shutdownTime != 0);
    context.serverUID = ism_common_getServerUID();
    context.notifyRC = ISMRC_ServerTerminating;
    context.notifyReason = "The connection was closed because the server was shutdown.";
    context.ts = ism_common_openTimestamp(ISM_TZF_LOCAL);

    context.parseObj.ent = context.ents;
    context.parseObj.ent_alloc = (int)(sizeof(context.ents)/sizeof(context.ents[0]));
    context.firstOtherServer = NULL;

    double startTime = ism_common_readTSC();

    do {
        char entryName[strlen(RECONCILE_CONFIG_NAME)+10];

        sprintf(entryName, "%s.%d", RECONCILE_CONFIG_NAME, context.entryNo++);

        const char *configEntry = ism_common_getStringProperty(configProps, entryName);

        if (configEntry != NULL) {
            entry = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),configEntry);
            char *ptr = entry;
            while(isspace(*ptr)) ptr++;
            char *end = strchr(ptr, ',');
            char *value = end ? end+1 : end;
            while(end > ptr && isspace(*end)) end--;
            if (end <= ptr) {
                value = NULL;
            } else {
                *end = '\0';
                context.topicFilter = ptr;
            }

            int valCount = 0;
            while(value) {
                ptr = value;
                while(isspace(*ptr)) ptr++;
                end = strchr(ptr, ',');
                value = end ? end+1 : end;
                while(end > ptr && isspace(*end)) end--;
                if (end <= ptr && end != NULL) {
                    value = NULL;
                } else {
                    if (end) {
                        *end = '\0';
                    } else {
                        /* At the end, but this is not a final element */
                        if ((valCount % RECONCILE_RULE_EXPECTED_ELEMENTS) != (RECONCILE_RULE_EXPECTED_ELEMENTS-1)) {
                            context.ruleCount = 0;
                            value = NULL;
                        }
                    }

                    /* Don't expect to see any empty strings */
                    if (*ptr == '\0') {
                        context.ruleCount = 0;
                        value = NULL;
                    } else {
                        char *endptr = NULL;
                        switch (valCount % RECONCILE_RULE_EXPECTED_ELEMENTS) {
                            /* ClientId Specification */
                            case 0:
                                /* We have filled up all the rule slots available, so extend */
                                if (context.ruleCount == context.ruleMax) {
                                    context.ruleMax += 10;
                                    context.rules = ism_common_realloc(ISM_MEM_PROBE(ism_memory_protocol_misc,212),context.rules, sizeof(*context.rules)*context.ruleMax);
                                    assert(context.rules != NULL);
                                }
                                /* Don't add a clientIdSpec if it's just "*" */
                                if (strcmp(ptr,"*") != 0) {
                                    context.rules[context.ruleCount].clientIdSpec = ptr;
                                } else {
                                    context.rules[context.ruleCount].clientIdSpec = NULL;
                                }

                                context.rules[context.ruleCount].matches = 0;
                                context.rules[context.ruleCount].connectMatches = 0;
                                context.rules[context.ruleCount].disconnectMatches = 0;
                                context.rules[context.ruleCount].unknownMatches = 0;
                                context.rules[context.ruleCount].cleared = 0;
                                context.rules[context.ruleCount].expired = 0;
                                context.rules[context.ruleCount].disconnected = 0;
                                context.rules[context.ruleCount].untouched = 0;
                                break;
                            /* retainType */
                            case 1:
                                context.rules[context.ruleCount].retainType = strtod(ptr, &endptr);
                                /* Need a valid retainType */
                                if (endptr == ptr ||
                                    context.rules[context.ruleCount].retainType < 0 ||
                                    context.rules[context.ruleCount].retainType > 2) {

                                    context.ruleCount = 0;
                                    value = NULL;
                                }
                                break;
                            /* expiry */
                            case (RECONCILE_RULE_EXPECTED_ELEMENTS-1):
                                context.rules[context.ruleCount].expiry = strtoul(ptr, &endptr, 0);
                                /* Need a valid expiry */
                                if (endptr == ptr) {
                                    context.ruleCount = 0;
                                    value = NULL;
                                } else {
                                    uint64_t expiryNanos = DAYS_TO_NANOS(context.rules[context.ruleCount].expiry);
                                    context.rules[context.ruleCount].expiryAgoTime = (context.nowNanos-expiryNanos);
                                    context.ruleCount++;
                                }
                                break;
                        }

                        valCount++;
                    }
                }
            }

            if (context.ruleCount == 0) {
                TRACE(3, "Malformed %s entry (%s)\n", entryName, configEntry);
            } else {
                TRACE(5, "Looking for topicFilter '%s' applying %d rules\n", context.topicFilter, context.ruleCount);

                iot_reconcileCallbackContext_t *pContext = &context;

                rc = ism_engine_getRetainedMessage(g_monitor_session,
                                                   context.topicFilter,
                                                   &pContext,
                                                   sizeof(pContext),
                                                   reconcileCallback,
                                                   NULL, 0, NULL);

                if (rc != OK && rc != ISMRC_NotFound) {
                    assert(rc != ISMRC_AsyncCompletion);
                    TRACE(3, "Unable to reconcile messages on topicFilter '%s'. rc=%d\n", context.topicFilter, rc);
                    /* Ignore the failure and move on to the next */
                    rc = OK;
                } else {
                    TRACE(5, "Found %d messages on topicFilter '%s'\n", context.seenMessages, context.topicFilter);

                    if (rc != ISMRC_NotFound) {
                        for(int ruleNo=0; ruleNo<context.ruleCount; ruleNo++) {
                            TRACE(5, "Rule #%d: Matched %d (Connect:%d, Disconnect: %d, Unknown:%d) messages with ClientId '%s' (Cleared:%d, Expired:%d, Disconnected:%d, Untouched:%d)\n",
                                  ruleNo+1,
                                  context.rules[ruleNo].matches,
                                  context.rules[ruleNo].connectMatches,
                                  context.rules[ruleNo].disconnectMatches,
                                  context.rules[ruleNo].unknownMatches,
                                  context.rules[ruleNo].clientIdSpec ? context.rules[ruleNo].clientIdSpec : "*",
                                          context.rules[ruleNo].cleared, context.rules[ruleNo].expired, context.rules[ruleNo].disconnected, context.rules[ruleNo].untouched);
                        }

                        if (context.unmatchedMessages != 0) {
                            TRACE(5, "Failed to match %d messages with any rules (%d from other servers, firstOtherServer='%s')\n",
                                  context.unmatchedMessages, context.otherServerMessages,
                                  context.firstOtherServer ? context.firstOtherServer : "(null)");
                        }
                    } else {
                        /* Want to move on to the next */
                        rc = OK;
                    }
                }
            }
        } else {
            assert(rc == OK);
            break;
        }

        ism_common_free(ism_memory_protocol_misc,context.firstOtherServer);

        context.topicFilter = NULL;
        context.ruleCount = 0;
        context.seenMessages = 0;
        context.unmatchedMessages = 0;
        context.otherServerMessages = 0;
        context.firstOtherServer = NULL;

        ism_common_free(ism_memory_protocol_misc,entry);
        entry = NULL;
    }
    while(rc == OK);

    ism_common_closeTimestamp(context.ts);

    if (context.parseObj.free_ent) ism_common_free(ism_memory_utils_parser,context.parseObj.ent);
    ism_common_free(ism_memory_protocol_misc,context.parseObj.source);

    ism_common_free(ism_memory_protocol_misc,context.rules);

    double elapsedTime = ism_common_readTSC()-startTime;

    TRACE(5, "%s finished in %.2f seconds (rc=%d).\n", __func__, elapsedTime, rc);

    // If we return anything other than 0 we're going to stop startup do we want to do that?
    return rc;
}
