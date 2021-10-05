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

/* *** NOTE:  This file is included from jms.c.  It is not separately compiled *** */

/* @file jmsrecieve.c
 * This file contains the jmsReceive method which is used to interpret the wire format
 * from the client.  In many cases significant function is done in the reply function
 * rather than directly here.
 */

/*
 * Check if the endpoint for this connection is an internal endpoint.
 * Any endpoint with any of the internal flags set is considered to be internal
 */
#define isInternal() (transport->listener->protomask & PMASK_Internal)

extern int ism_protocol_auth(char * username, int isoauth, int isltpa);

static int jmsPingClient(ism_transport_t * transport, int ping) {
    char msg[8] = {0};
    msg[4] = ping;
    int rc = transport->send(transport, &(msg[4]), 1, 0, SFLAG_FRAMESPACE);
    if (rc && (rc != SRETURN_SUSPEND)) {
        TRACEL(7, transport->trclevel, "Failed to ping client: connect=%u\n", transport->index);
        transport->close(transport, ISMRC_PingFailed, 0, "Failed to ping client.");
        return 0;
    }
    TRACEL(8, transport->trclevel, "Ping was sent to client: connect=%u client=%s ip=%s port=%u rc=%d\n", transport->index, transport->name,
            transport->client_addr, transport->clientport,rc);
    return 1;
}

static int keepAliveTimer(ism_timer_t key, ism_time_t timestamp, void * userdata){
    ism_transport_t * transport = (ism_transport_t *)userdata;
    jmsProtoObj_t * pobj = (jmsProtoObj_t *) transport->pobj;
    uint64_t laTime = transport->lastAccessTime;
    uint64_t currTime = (uint64_t) ism_common_readTSC();
    uint64_t diffTime = currTime - laTime;
    if (pobj->closed) {
    	ism_common_cancelTimer(key);
    	return 0;
    }
    if (diffTime > pobj->keepaliveTimeout){
        TRACEL(7, transport->trclevel, "The connection has timed out: connect=%u timeout=%llu(%u)\n", transport->index,
                (ULL)diffTime, pobj->keepaliveTimeout);
        transport->close(transport, ISMRC_ConnectTimedOut, 0, "The connection has timed out");
        ism_common_cancelTimer(key);
        return 0;
    }
    if (diffTime > 4*pobj->keepAliveCheckInterval) {
    	int rc = jmsPingClient(transport, 0);
    	if(!rc)
    		ism_common_cancelTimer(key);
    }
    return 1;
}

static int keepAliveTimerOnce(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_transport_t * transport = (ism_transport_t *)userdata;
    jmsProtoObj_t * pobj = (jmsProtoObj_t *) transport->pobj;
    if (!pobj->closed && pobj->keepAliveTimer) {
        ism_timer_t timer = pobj->keepAliveTimer;
        if(__sync_bool_compare_and_swap(&pobj->keepAliveTimer, timer, NULL)) {
            ism_common_cancelTimer(key);
            TRACEL(7, transport->trclevel, "The connection has timed out (no ping reply): connect=%u\n", transport->index);
            transport->close(transport, ISMRC_ConnectTimedOut, 0, "The connection has timed out");
        }
    }
    return 0;
}

/*
 * Check that the connection is alive
 */
static int jmsCheckLiveness(ism_transport_t * transport) {
    jmsProtoObj_t * pobj = (jmsProtoObj_t *) transport->pobj;
    /* If keepAliveTimer != NULL we are checking the liveness already */
    if (!pobj->closed && (pobj->keepAliveTimer == NULL)) {
        if (jmsPingClient(transport, 0)) {
            ism_timer_t timer = ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t) keepAliveTimerOnce,
                    transport, 60000000000);
            if(!__sync_bool_compare_and_swap(&pobj->keepAliveTimer, NULL, timer)) {
                ism_common_cancelTimer(timer);
            }
        }
    }
    return 0;
}

/*
 * Receive an action on for the JMS protocol.
 */
#ifdef PERF
static int jmsReceive_int(ism_transport_t * transport, const char * buf, int buflen, int kind);
static int jmsReceive(ism_transport_t * transport, const char * buf, int buflen, int kind) {
    int rc;
    double t1;
    int doStat = 0;
    const actionhdr *hdr = (const actionhdr *)buf;
    ism_latencystat_t * stat = NULL;
    if ((hdr->action == Action_message) ||(hdr->action == Action_messageWait)) {
        stat = msgProcStat;
    } else {
        if (hdr->action == Action_commitSession) {
            stat = commitStat;
        }
    }
    if (stat) {
        __sync_fetch_and_add(&stat->count, 1);
        if (__sync_add_and_fetch(&stat->takesample, 1) == histSampleRate) {
            t1 = ism_common_readTSC();
            doStat = 1;
        }
    }
    rc  = jmsReceive_int(transport,buf,buflen,kind);
    if (doStat) {
        double t2 = ism_common_readTSC();
        int latency = (int) ((t2 - t1) * invHistUnits + 0.5);
        __sync_fetch_and_add(&stat->takesample, -histSampleRate);
        if (latency < stat->histSize) {
            __sync_fetch_and_add(&(stat->histogram[latency]), 1);
        } else {
            __sync_fetch_and_add(&stat->big, 1);
        }
        if (latency > stat->max)
            stat->max = latency;
    }
    return rc;
}
static int jmsReceive_int(ism_transport_t * transport, const char * buf, int buflen, int kind) {
#else
static int jmsReceive(ism_transport_t * transport, char * buf, int buflen, int kind) {
#endif
    ism_jms_session_t  * session = NULL;
    ism_jms_prodcons_t * prodcons = NULL;
    uint32_t         sessionid = 0;
    uint32_t         prodconsid = 0;
    int              i;
    int              rc = 0;
    int              lrc;
    int              id = 0;
    int              count;
    int              propcount = 0;
    int              extra = 0;
    int              chklen;
    ism_protocol_action_t * action;

    concat_alloc_t   map;
    ism_field_t      fname;
    ism_field_t      f[16];           /* Max 15 fields */
    ism_field_t      pfield;
    ism_field_t      body;
    const char *     name;
    const char *     subName;
    const char *     clientid;
    const char *     username;
    const char *     password;
    ismEngine_SessionHandle_t sessionh;
    ismEngine_ConsumerHandle_t consumerh;
    ismEngine_MessageHandle_t msgh;
    int              noack;
    int              actionsize;
    ism_propent_t    prp[100];        /* Max 100 properties for non-message */
    char             trcbuf[2048];
    char             trcbuf2[256];
    char             actionbuf[4096];
    int              domain;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int              rulelen;
    jmsProtoObj_t  * pobj = NULL;
    ism_transport_t * clientTrans;
    ism_jms_prodcons_t * consumer;

    if (!(serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED)) {
        ism_common_setError(ISMRC_Closed);
        transport->close(transport, ISMRC_MessagingNotAvailable, 0, "Messaging is not available");
        return ISMRC_Closed;
    }
    pobj = (jmsProtoObj_t*)transport->pobj;

    /* If protocol is closing the connection, reject the message */
    if (pobj->closed) {
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }

    if (pobj->keepAliveTimer && !pobj->keepaliveTimeout) {
        /* This is a ping timer */
        ism_timer_t timer = pobj->keepAliveTimer;
        if(__sync_bool_compare_and_swap(&pobj->keepAliveTimer, timer, NULL)) {
            ism_common_cancelTimer(timer);
        }
    }

    if (buflen == 1) {
        if (buf[0] == 0) /* Ping request */
            jmsPingClient(transport,1);
        return ISMRC_OK;
    }

    /*
     * There is a 20 byte fixed header on all actions.
     */
    if (buflen < sizeof(actionhdr)) {
        TRACEL(2, transport->trclevel, "An action which is too short was received: len=%d connect=%u client=%s\n",
                buflen, transport->index, transport->name);
        sprintf(trcbuf2, "Received JMS data index=%u", transport->index);
        TRACEDATA(2, trcbuf2, 0, buf, buflen, buflen);
        ism_common_setError(ISMRC_MessageNotValid);
        transport->close(transport, ISMRC_BadClientData, 0, "The data received from the client is not valid");
        return ISMRC_MessageNotValid;
    }
    action = (ism_protocol_action_t *)actionbuf;
    memset(action, 0, sizeof(ism_protocol_action_t));
    memcpy(&action->hdr, buf, sizeof(actionhdr));

    action->transport = transport;

    /*
     * Binary dump trace of the message except createConnection which can contain a password
     * For message actions we only trace message data if msgdata is set for that many bytes.
     */
    if (SHOULD_TRACE(9)) {
        int maxsize = ism_common_getTraceMsgData()+sizeof(actionhdr);
        if (action->hdr.action > Action_msgNoProdWait || action->hdr.action == Action_ack) {
            if (maxsize < buflen)
                maxsize = 1000;
        }
        sprintf(trcbuf, "JMS receive %s connect=%u", getActionName(action->hdr.action), transport->index);
        TRACEDATA(9, trcbuf, 0, buf, buflen, maxsize);
    }

    /*
     * Set session ID, producer/consumer ID and corresponding pointers,
     * when available based on the action type.
     */
    switch (action->hdr.itemtype) {
    case ITEMT_None:
        /* This is an async ack, carries session id and the message sequence number */
        sessionid = endian_int32(action->hdr.item);
        if (sessionid > 0) {
            session = getSession(transport, sessionid);
        }
        if (!session || !session->handle) {
            TRACEL(2, transport->trclevel, "Unknown session in action header: itemtype=None session=%d connect=%u\n", sessionid, transport->index);
            rc = 1;
            break;
        }
        action->session = session;
        break;

    case ITEMT_Thread:
        /* Connection-related, carries response id */
        break;

    case ITEMT_Session:
        /* Session-related (create producer/consumer), carries
         * session id and response id */
        sessionid = endian_int32(action->hdr.item);
        if (sessionid > 0) {
            session = getSession(transport, sessionid);
        }
        if (!session || !session->handle) {
            TRACEL(2, transport->trclevel, "Unknown session in action header: itemtype=Session session=%d connect=%u\n", sessionid, transport->index);
            rc = 1;
            break;
        }
        action->session = session;
        break;

    case ITEMT_Consumer:
        /* Close consumer/set listener/receive, carries consumer id and response id */
        prodconsid = endian_int32(action->hdr.item);
        if (prodconsid > 0) {
            prodcons = getProdcons(transport, prodconsid);
        }
        if (!prodcons || !prodcons->handle) {
            TRACEL(2, transport->trclevel, "Unknown consumer in action header: itemtype=Consumer consumer=%d connect=%u\n", prodconsid, transport->index);
            rc = 1;
            break;
        }
        action->prodcons = prodcons;

        sessionid = prodcons->hdr.item;
        if (sessionid > 0) {
            session = getSession(transport, sessionid);
        }
        if (!session) {
            if ((action->hdr.action == Action_startConsumer) ||
               (action->hdr.action == Action_suspendConsumer)) {
                TRACEL(6, transport->trclevel, "Action (%s) requested for consumer on closed session: connect=%u consumer=%d session=%d\n",
                        getActionName(action->hdr.action), transport->index, prodcons->which, sessionid);
                return 0;
            }
            TRACEL(2, transport->trclevel, "Unknown session in action header: itemtype=Consumer sessionid=%d connect=%u action=%s\n", sessionid, transport->index,getActionName(action->hdr.action));
            rc = 1;
            break;
        }
        action->session = session;
        break;

    case ITEMT_Producer:
        /* Close producer, carries producer id and response id */
        prodconsid = endian_int32(action->hdr.item);
        if (prodconsid > 0) {
            prodcons = getProdcons(transport, prodconsid);
        }
        if (!prodcons || !prodcons->handle) {
            TRACEL(2, transport->trclevel, "Unknown producer in action header: itemtype=Producer producer=%d connect=%u action=%s\n", prodconsid, transport->index, getActionName(action->hdr.action));
            rc = 1;
            break;
        }
        action->prodcons = prodcons;

        sessionid = prodcons->hdr.item;
        if (sessionid > 0) {
            session = getSession(transport, sessionid);
        }
        if (!session) {
            TRACEL(2, transport->trclevel, "Unknown session in action header: itemtype=Producer session=%d connect=%u action=%s\n", sessionid, transport->index, getActionName(action->hdr.action));
            rc = 1;
            break;
        }
        action->session = session;
        break;

    default:
        TRACEL(2, transport->trclevel, "Unknown itemtype in action header: itemtype=0x%02x connect=%u action=%s\n", (uint8_t)action->hdr.itemtype, transport->index,getActionName(action->hdr.action));
        rc = 1;
        break;
    }

    if (rc) {
        sprintf(trcbuf2, "The JMS action header is not valid: connect=%u action=%s", transport->index,getActionName(action->hdr.action));
        TRACEDATA(2, trcbuf2, 0, &action->hdr, sizeof(action->hdr), sizeof(action->hdr));
        ism_common_setError(ISMRC_MessageNotValid);
        transport->close(transport, ISMRC_BadClientData, 0, "The data received from the client is not valid");
        return ISMRC_MessageNotValid;
    }

    memset(&map, 0, sizeof(map));
    map.buf    = (char *)buf;
    map.used   = buflen;
    map.pos    = sizeof(actionhdr);

#ifdef PERF_FULL
    action->tsc = ism_common_readTSC();
#endif

    /* Parse the header or values */
    count = action->hdr.hdrcount&0xf;
    for (i=0; i<count; i++) {
        lrc = ism_protocol_getObjectValue(&map, f+i);
        if (lrc) {
            TRACEL(2, transport->trclevel, "Bad action header variable: connect=%u which=%d rc=%d\n", transport->index, i, lrc);
            rc += lrc;
        }
    }

    /* Parse the properties */
    if (map.pos < map.used) {
        pfield.len = 0;
        lrc = ism_protocol_getObjectValue(&map, &pfield);
        if (lrc) {
            TRACEL(2, transport->trclevel, "Bad action header properties: connect=%u rc=%d\n", transport->index, lrc);
            rc += lrc;
        }
        /* Treat empty properties as no properties */
        if (pfield.type != VT_Map) {
            pfield.type = VT_Null;
        } else if (pfield.len == 0) {
            pfield.type = VT_Null;
        }
    } else {
        pfield.len  = 0;
        pfield.type = VT_Null;
    }

    /* Parse the body */
    if (map.pos < map.used) {
        body.len = 0;
        lrc = ism_protocol_getObjectValue(&map, &body);
        if (lrc) {
            TRACEL(2, transport->trclevel, "Bad action header body: connect=%u rc=%d\n", transport->index, lrc);
            rc += lrc;
        }
    } else {
        body.len = 0;
        body.type = VT_Null;
    }

    if (rc) {
        sprintf(trcbuf2, "The JMS action data is not valid: connect=%u", transport->index);
        TRACEDATA(2, trcbuf2, 0, buf, buflen, 2048);
        ism_common_setError(ISMRC_MessageNotValid);
        transport->close(transport, ISMRC_BadClientData, 0, "The data received from the client is not valid");
        return ISMRC_MessageNotValid;
    }

    /* -1 was already reached, so closing is in progress already */
    if (__sync_add_and_fetch(&pobj->inprogress, 1) < 1) {
    	__sync_sub_and_fetch(&pobj->inprogress, 1);
    	ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }

#ifdef DEBUG
    int ptrc = pthread_spin_trylock(&pobj->lock);
    if (ptrc == 0)
        pthread_spin_unlock(&pobj->lock);
    TRACEL(8, transport->trclevel, "---------jmsReceive action=%s connect=%u session=%d inprogress=%d lock=%d\n",
        getActionName(action->hdr.action), transport->index, sessionid, pobj->inprogress, ptrc);
#endif
    /*
     * Handle actions other than a message
     */
    if (action->hdr.action >= Action_createConnection) {
        TRACEL(8, transport->trclevel, "jmsReceive action=%s connect=%u client=%s session=%d hdr=%d type=%d item=%d id=%lu\n",
                 getActionName(action->hdr.action), transport->index, transport->name,
                 sessionid, action->hdr.hdrcount,
                 action->hdr.itemtype, endian_int32(action->hdr.item),
                 endian_int64(action->hdr.msgid));

        /* Expand the properties for everything except messages */
        if (pfield.type != VT_Null ) {
            concat_alloc_t abuf = {pfield.val.s, pfield.len, pfield.len, 0};
            while (abuf.pos < abuf.used) {
                rc += ism_protocol_getObjectValue(&abuf, &fname);
                rc += ism_protocol_getObjectValue(&abuf, &prp[propcount].f);

                TRACEL(8, transport->trclevel, "prop %s %s\n", ism_protocol_dumpField(&fname, trcbuf, sizeof(trcbuf)),
                    ism_protocol_dumpField(&prp[propcount].f, trcbuf2, sizeof(trcbuf2)));

                if (rc==0 && fname.type == VT_Name) {
                    prp[propcount].name = fname.val.s;
                    extra += (int)strlen(fname.val.s) + 1;
                    if (prp[propcount].f.type == VT_String) {
                        extra += (int)strlen(prp[propcount].f.val.s) + 1;
                    } else if (prp[propcount].f.type == VT_ByteArray) {
                        extra += prp[propcount].f.len;
                    }
                    propcount++;
                } else {
                    int inprogress = __sync_sub_and_fetch(&pobj->inprogress, 1);
                    if (inprogress < 0) { /* BEAM suppression: constant condition */
                        if(inprogress == -1) {
                            ism_protocol_action_t act = {0};
                            act.transport = transport;
                            act.hdr.action = Action_closeConnection;
                            replyClosing(0, NULL, &act);
                        }
                        return ISMRC_Closed;
                    } else {
                        ism_common_setError(ISMRC_PropertiesNotValid);
                        transport->close(transport, ISMRC_BadClientData, 0, "The data received from the client is not valid");
                        return ISMRC_PropertiesNotValid;
                    }
                }
            }
        }

        for (i=0; i<count; i++) {
            if (f[i].type == VT_String) {
                extra += (int)strlen(f[i].val.s)+1;
            } else if (f[i].type == VT_ByteArray) {
                extra += f[i].len;
            }
        }

        /*
         * Make a copy in allocated memory of the action structure
         * This can be passed to the engine.
         */
        actionsize = sizeof(ism_protocol_action_t) + count*sizeof(ism_field_t) + propcount*sizeof(ism_propent_t) + extra;
        if (actionsize > sizeof actionbuf) {
            action = alloca(actionsize);
            memcpy(action, actionbuf, sizeof(ism_protocol_action_t));
        }
        action->actionsize = actionsize;
        action->transport = transport;
        action->valcount = count;
        action->propcount = propcount;
        action->values = (ism_field_t *)(action+1);
        action->props  = (ism_propent_t *)(action->values + count);
        action->old_action = NULL;
        if (count)
            memcpy(action->values, f, count*sizeof(ism_field_t));
        if (propcount)
            memcpy(action->props, prp, propcount*sizeof(ism_propent_t));

        char * pw = NULL;
        /*
         * Perform the action
         */
        switch (action->hdr.action) {
        case Action_ack:        /* Asynchronous ACK */
            if (session && session->ackmode) {
                uint64_t seqnum = action->values[1].val.l;
                uint64_t maxsqn = 0;
                uint64_t sqns[256];
                uint32_t acksqn_count = action->values[0].val.i;
                uint64_t * acksqn = getAckSqn(pobj,&map,acksqn_count,sqns,256,&maxsqn,&rc);

                if (__builtin_expect((acksqn == NULL), 0)) {
                    replyAction(rc, NULL, action);
                    break;
                }
                ackDeliveredMessages(transport, session, ismENGINE_CONFIRM_OPTION_CONSUMED, seqnum,
                        maxsqn, acksqn_count, acksqn, NULL, NULL);
                if (__builtin_expect((acksqn != sqns),0)) {
                    ism_common_free(ism_memory_protocol_misc,acksqn);
                }
            }
            /* Action has been taken, check if close was requested */
            if (__sync_sub_and_fetch(&pobj->inprogress, 1) == -1) { /* BEAM suppression: constant condition */
                ism_protocol_action_t act = { 0 };
                act.transport = transport;
                act.hdr.action = Action_closeConnection;
                replyClosing(0, NULL, &act);
            }
            break;

        case Action_ackSync:
            if (session && session->ackmode) {
                consumer = NULL;
                uint64_t seqnum = action->values[1].val.l;
                uint64_t maxsqn = 0;
                uint64_t sqns[256];
                uint32_t acksqn_count = action->values[0].val.i;
                uint64_t * acksqn = getAckSqn(pobj,&map,acksqn_count,sqns,256,&maxsqn,&rc);

                if (__builtin_expect((acksqn == NULL),0)) {
                    replyAction(rc, NULL, action);
                    break;
                }
                if (action->valcount >= 3) {
                  if (action->values[2].type != VT_Null) {
                      /* AUTO_ACK with message listener and zero cache size */
                      int consumerID = action->values[2].val.i;
                      consumer = getProdcons(transport,consumerID);
                  }
                }
                if (consumer && (consumer->cacheSize == 0) && (transport->addwork)){
                	if (__sync_sub_and_fetch(&consumer->inBatch,1) < 1){
                    	__sync_add_and_fetch(&pobj->inprogress, 1);
                        TRACEL(8, transport->trclevel, "Submitting job request to resume consumer: connect=%u consumer=%d inBatch=%d\n", transport->index, consumer->which, consumer->inBatch);
                       ism_transport_submitAsyncJobRequest(transport, resumeConsumerDelivery, (void*)((uintptr_t)consumer->which),0);
                	}
                }
                rc = ackDeliveredMessages(transport, session, ismENGINE_CONFIRM_OPTION_CONSUMED, seqnum,
                        maxsqn, acksqn_count, acksqn, action, replyAction);
                if (__builtin_expect((acksqn != sqns),0)) {
                    ism_common_free(ism_memory_protocol_misc,acksqn);
                }
            }
            if(rc != ISMRC_AsyncCompletion)
                replyAction(rc, NULL, action);
            break;

        case Action_commitSession:
            if (session && session->handle) {
                /*
                 * The first header field is the last delivered sequence number.
                 */
                if ((!session->transacted) || (session->transaction == NULL)) {
                    ism_common_setError(ISMRC_BadClientData);
                    replyAction(ISMRC_BadClientData, NULL, action);
                    break;
                }

                action->session = session;
                if (action->valcount == 2) {
                    /*
                     * ACK all delivered messages
                     */
                    uint64_t seqnum = action->values[1].val.l;
                    uint64_t maxsqn = 0;
                    uint64_t sqns[256];
                    uint32_t acksqn_count = action->values[0].val.i;
                    uint64_t * acksqn = getAckSqn(pobj,&map,acksqn_count,sqns,256,&maxsqn,&rc);
                    if (__builtin_expect((acksqn == NULL),0)) {
                        replyAction(rc, NULL, action);
                        break;
                    }
                    rc = ackDeliveredMessages(transport, session, ismENGINE_CONFIRM_OPTION_CONSUMED,
                            seqnum, maxsqn, acksqn_count, acksqn, action, replyCommitSession);
                    if (__builtin_expect((acksqn != sqns),0)) {
                        ism_common_free(ism_memory_protocol_misc,acksqn);
                    }
                }
                if (rc != ISMRC_AsyncCompletion)
                    replyCommitSession(rc, NULL, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }

            break;

        case Action_createConnection:
            clientid = getproperty(action, "ClientID");
            username = getproperty(action, "userid");
            password = getproperty(action, "password");
            if (!clientid || !*clientid) {
                TRACEL(2, transport->trclevel, "The client ID is required and is not specified: connect=%u", transport->index);
                ism_common_setError(ISMRC_ClientIDRequired);
                replyAction(ISMRC_ClientIDRequired, NULL, action); /* Client ID is required */
                break;
            }

            /*
             * Reserve all clientIDs starting with a double underscore for internal users.
             */
            if (clientid[0]=='_') {
                if (!(transport->listener->protomask&PMASK_Internal) && clientid[1]=='_') {
                    ism_common_setError(ISMRC_ClientIDInUse);
                    replyAction(ISMRC_ClientIDInUse, NULL, action);   /* Client ID is in use (actually reserved) */
                    break;
                }
                if (strchr(clientid+2, '_')) {
                    transport->pobj->isGenerated = 1;
                }
            }

            transport->clientID = ism_transport_putString(transport, clientid);
            if (username) {
                transport->userid = ism_transport_putString(transport, username);
            }
            transport->name = transport->clientID;

            int expireX = getintproperty(action, "ExpireConnection", 0);
            if (expireX) {
                TRACEL(7, transport->trclevel, "Set JMS expire time: index=%u client=%s expire=%d\n",
                        transport->index, transport->name, expireX);
                ism_transport_setConnectionExpire(transport, ism_common_currentTimeNanos()+expireX*1000000000L);
            }

            /* Check if the connection is temporarily not allowed */
            rc = ism_transport_clientAllowed(transport);
            if (rc) {
                ism_common_setError(rc);
                replyAction(rc, NULL, action);
            }

            /*
             * User authentication
             */
            int authnRequired = 0;

            if (username) {
                int pw_len = 0;
                int user_len = (int) strlen(username);
                if (password) {
                    pw = alloca(strlen(password));
                    instanceUnHash(transport, password, pw);
                    pw_len = strlen(pw);
                }

                action2actionBuff(action);

                /* Call Asynch Auth if the usePasswordAuth is true or the user ID is specified for non-SSL connection */
                if (transport->listener->usePasswordAuth ||
                    (!transport->listener->secure && transport->userid)) {
                    int isltpa = ism_security_context_isLTPA(transport->security_context);
                    int isoauth = ism_security_context_isOAuth(transport->security_context);
                    TRACEL(8, transport->trclevel, "Authentication: submit async authentication and authorization: connect=%u client=%s user=%s\n",
                            transport->index, transport->name, transport->userid);
                    authnRequired = 1;
                    if(isoauth || isltpa) {
                    	if (ism_protocol_auth((char *)username, isoauth, isltpa)) {
                    		rc = ISMRC_NotAuthorized;
                    		ism_common_setError(rc);
                    		replyAction(rc, NULL, action);
                    	}
                    }

                    ism_security_authenticate_user_async(transport->security_context, transport->userid, user_len, pw, pw_len,
                           (void *) action, actionsize, jmsReplyAuth, authnRequired, 0);
                } else {
                    action->rc = 0;
                    // jmsReplyAuthTT(NULL, 0, action);
                    TRACEL(8, transport->trclevel, "Authorization: submit async authorization: connect=%u client=%s user=%s\n",
                            transport->index, transport->name, transport->userid?transport->userid:"");
                    ism_security_authenticate_user_async(transport->security_context, transport->userid, user_len, pw, pw_len,
                           (void *) action, actionsize, jmsReplyAuth, authnRequired, 0);
                }
            } else {
              /* If username is not specified, fail authentication if UserAuth enforcement is true */
                if (transport->listener->usePasswordAuth) {
                    action->rc = ISMRC_NotAuthenticated;
                    jmsReplyAuthTT(NULL,0, action);
                } else {
                    int pw_len = 0;
                    int user_len = 0;
                    action->rc = ISMRC_OK;
                    // jmsReplyAuthTT(NULL,0, action);
                    action2actionBuff(action);
                    TRACEL(8, transport->trclevel, "Authorization: submit async authorization: connect=%u client=%s user=%s\n",
                            transport->index, transport->name, transport->userid?transport->userid:"");
                    ism_security_authenticate_user_async(transport->security_context, transport->userid, user_len, pw, pw_len,
                           (void *) action, actionsize, jmsReplyAuth, authnRequired, 0);
                }
            }
            break;

        case Action_stopSession:
            if (session && session->handle) {
                int32_t stop_rc = 0;
                if (action->xbuf.inheap)
                    ism_common_freeAllocBuffer(&action->xbuf);
                memset(&action->xbuf, 0, sizeof(concat_alloc_t));
                if (action->valcount) {
                    int ackseq_rc;
                    uint64_t maxsqn = 0;
                    uint64_t sqns[256];
                    uint32_t acksqn_count = action->values[0].val.i;
                    uint64_t * acksqn = getAckSqn(pobj,&map,acksqn_count,sqns,256,&maxsqn,&ackseq_rc);
                    action->flag = acksqn_count;
                    ism_common_allocBufferCopyLen(&action->xbuf, (char *)acksqn, acksqn_count * sizeof(uint64_t));
                }
                action->session = session;
                if (!(__sync_fetch_and_or(&session->suspended, SUSPENDED_BY_PROTOCOL))) {
                    stop_rc = ism_engine_stopMessageDelivery(session->handle, action, actionsize, replyStopSession);
                }
                if (stop_rc != ISMRC_AsyncCompletion)
                    replyStopSession(stop_rc, NULL, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_stopConnection:
            for (i = 0; i < pobj->sessions_alloc; i++) {
                session = getSession(transport, i);
                if (session && session->handle) {
                    if (!(__sync_fetch_and_or(&session->suspended, SUSPENDED_BY_PROTOCOL))) {
                        rc = ism_engine_stopMessageDelivery(session->handle, NULL, 0, NULL);
                    }
                }
                action->session = session;
            }
            pthread_spin_lock(&(pobj->lock));
            pobj->started = 0;
            pthread_spin_unlock(&(pobj->lock));

            replyAction(0, NULL, action);
            break;

        case Action_startConnection:
        	handleStartConnection(action, &map);
            break;

        case Action_closeConnection:
            if (pobj->handle) {
                pthread_spin_lock(&(pobj->lock));
					for (i = 0; i < pobj->sessions_alloc; i++) {
						if (pobj->sessions[i])
							pobj->sessions[i]->handle = NULL;
					}
				    for (i = 0; i < pobj->prodcons_alloc; i++) {
				        if (pobj->prodcons[i])
				            pobj->prodcons[i]->handle = NULL;
				    }
                pthread_spin_unlock(&(pobj->lock));
                rc = ism_engine_destroyClientState(pobj->handle,
                        ismENGINE_DESTROY_CLIENT_OPTION_NONE, action,
                        sizeof(ism_protocol_action_t), replyAction);
                if (rc != ISMRC_AsyncCompletion)
                    replyAction(rc, NULL, action);
            } else {
//                ism_common_setError(ISMRC_BadClientData);
//                replyAction(ISMRC_BadClientData, NULL, action);     /* Will cause the connection to close */
                replyAction(0, NULL, action);
            }
            break;

        case Action_createSession:
            if (pobj->handle) {
                int options = ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND;
                action2actionBuff(action);
                if (action->values[1].val.i)
                    options |= ismENGINE_CREATE_SESSION_TRANSACTIONAL;
                rc = ism_engine_createSession(pobj->handle, options,
                        &sessionh, action, actionsize, replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                    replyAction(rc, sessionh, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);     /* Will cause the connection to close */
            }
            break;

        case Action_closeSession:
            if (session && session->handle) {
                action->session = session;
                /* Stop message delivery (to avoid immediate redelivery)
                 * and mark all not ACKed messages as "not delivered" */
                if (!(__sync_fetch_and_or(&session->suspended, SUSPENDED_BY_PROTOCOL))) {
                    rc = ism_engine_stopMessageDelivery(session->handle,
                            action, sizeof(ism_protocol_action_t), replyCloseSession);
                    TRACEL(8, transport->trclevel, "Action_closeSession: Stop message delivery requested. connect=%u rc=%d session=%d\n",
                            transport->index, rc, session->which);
                }
                if (rc != ISMRC_AsyncCompletion) {
                    replyCloseSession(rc, NULL, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);       /* Will cause the connection to close */
            }
            break;

        case Action_createTransaction:
            if (pobj->handle) {
                ismEngine_TransactionHandle_t transHandle;

                TRACEL(7, transport->trclevel, "Create transaction connect=%u client=%s session=%d\n",
                        transport->index, transport->name, sessionid);
                if (session && session->handle && session->transacted) {
                    action->session = session;
                    action2actionBuff(action);
                    rc = ism_engine_createLocalTransaction(session->handle,
                            &transHandle, action, actionsize, replyAction);
                    if (rc != ISMRC_AsyncCompletion) {
                      replyAction(rc, transHandle, action);
                    }
                } else {
                    ism_common_setError(ISMRC_BadClientData);
                    replyAction(ISMRC_BadClientData, NULL, action);     /* Will cause the connection to close */
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);     /* Will cause the connection to close */
            }

            break;

        case Action_createDestination:
            /* val0=domain, val1=dest, val3= */
            if (action->valcount > 0 && action->values[0].type == VT_Byte) {
                domain = action->values[0].val.i;
            } else {
                domain = ismDESTINATION_QUEUE;
            }
            /* The destination name is required */
            if (action->valcount > 1 && action->values[1].type == VT_String)
                name = action->values[1].val.s;
            else
                name = NULL;
            if (!name || strlen(name) == 0) {
                ism_common_setError(ISMRC_NoDestination);
                replyAction(ISMRC_NoDestination, NULL, action);
                break;
            }
            rc = ism_engine_createTemporaryDestination(pobj->handle, domain, name,
                    NULL, action, sizeof(ism_protocol_action_t), replyAction);
            if (rc != ISMRC_AsyncCompletion) {
                replyAction(rc, NULL, action);
            }
            break;

        case Action_createConsumer:
            /* val0=domain, val1=nolocal, val2=selector, props=dest */
            if (action->valcount > 0 && action->values[0].type == VT_Byte) {
                domain = action->values[0].val.i;
            } else {
                domain = ismDESTINATION_QUEUE;
            }

            /* Check if noLocal is requested */
            if (action->valcount > 1 && action->values[1].type == VT_Boolean) {
                action->nolocal = action->values[1].val.i;
            }

            /* Compile selector if requested */
            if (action->valcount>2 && action->values[2].type == VT_String) {
                rulelen = 0;
                rc = ism_common_compileSelectRule(&action->rule, &rulelen, action->values[2].val.s);
                if (rc) {
                    replyAction(rc, NULL, action);
                    break;
                }
                TRACEL(7, transport->trclevel, "Select = %s\n", action->values[2].val.s);  /* TODO */
            }


            /* The destination name is required */
            name = getproperty(action, "Name");
            if (!name || *name == 0) {
                ism_common_setError(ISMRC_NoDestination);
                replyAction(ISMRC_NoDestination, NULL, action);
                break;
            }
            /* Any system topic must start with $ */
            if (name[0]=='$') {
                if (name[1]!='S' || name[2]!= 'Y' || name[3]!='S' || name[4]!='/' ) {
                	ism_common_setError(ISMRC_BadSysTopic);
	                replyAction(ISMRC_BadSysTopic, NULL, action);
    	            break;
    	        }
            }

            noack = getbooleanproperty(action, "DisableACK");
            if (session && session->handle) {
                ism_prop_t * cprops = NULL;

                int orderIdRequested = getbooleanproperty(action, "RequestOrderID");
#if 0
                if (!(__sync_fetch_and_or(&session->suspended,SUSPENDED_BY_PROTOCOL))) {
                    ism_engine_stopMessageDelivery(session->handle, NULL, 0, NULL);
                }
#endif
                action->session = session;

                consumer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,166),1,sizeof(ism_jms_prodcons_t));
                if (consumer) {
                    consumer->domain = domain;
                    consumer->name   = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),name);
                    consumer->kind = KIND_CONSUMER;
                    id = setProdcons(transport,consumer);
                    consumer->which = id;
                    consumer->noack = noack;
                    consumer->transport = transport;
                    consumer->session = session;
                    consumer->handle = NULL;
                    consumer->rule = action->rule;
                    consumer->cacheSize = getintproperty(action, "ClientMessageCache", -1);
                    if (consumer->cacheSize > 0) {
                        consumer->cacheSize = 1 + consumer->cacheSize/3;
                    } else {
                        if (consumer->cacheSize == -1)          /* For ismc */
                            consumer->cacheSize = INT_MAX;
                    }

                    memcpy(&consumer->hdr, &action->hdr, sizeof(struct actionhdr));
                    consumer->hdr.item = sessionid;
                    pthread_spin_init(&consumer->lock,0);
                    consumer->hdr.itemtype = ITEMT_Consumer;
                    consumer->orderIdRequested = orderIdRequested;
                    action->prodcons = consumer;
                    /* Engine restriction - messages are delivered asynchronously only */
                }

                /*
                 * If we cannot set the consumer, report an error and free up memory
                 */
                if (id <= 0) {
                    if (id == 0) {
                        ism_common_setError(ISMRC_AllocateError);
                        replyCreateConsumer(ISMRC_AllocateError, NULL, action);
                    } else {
                        ism_common_setError(ISMRC_TooManyProdCons);
                        replyCreateConsumer(ISMRC_TooManyProdCons, NULL, action);
                    }
                    return true;
                }

                if (!consumer->noack && session->ackmode) {
                    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
                } else {
                    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_NONE;
                }

                if (action->nolocal) {
                    subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL;
                }

                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;

                /*
                 * Construct a properties struture if required
                 */
                uint32_t consumerOption = ismENGINE_CONSUMER_OPTION_NONE;
                if (consumer->rule) {
                    ism_field_t field;
                    cprops = ism_common_newProperties(20);
                    field.type  = VT_ByteArray;
                    field.val.s = (char *)consumer->rule;
                    field.len   = rulelen;
                    ism_common_setProperty(cprops, "Selector", &field);
                    if (domain == ismDESTINATION_TOPIC) {
                        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION;
                    } else {
                        consumerOption = ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION;
                    }
                }
                if ((pobj->started) && transport->addwork) {
                    consumerOption |= ismENGINE_CONSUMER_OPTION_PAUSE;
                    consumer->suspended = SUSPENDED_BY_PROTOCOL;
                }
                if ((!consumer->noack && session->ackmode) || session->transacted) {
                    consumerOption |= ismENGINE_CONSUMER_OPTION_ACK;
                }

                /*
                 * Create the engine object
                 */
                rc = ism_engine_createConsumer(session->handle, consumer->domain,
                    name, &subAttrs, NULL, consumer, sizeof(*consumer), replyReceive,
                    cprops, consumerOption, &consumerh,
                    action, sizeof(ism_protocol_action_t), replyCreateConsumer);

                if (cprops) {
                    ism_common_freeProperties(cprops);
                }

                if (rc != ISMRC_AsyncCompletion) {
                  replyCreateConsumer(rc, consumerh, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_checkBrowser:
            rc = ism_engine_checkAvailableMessages(action->prodcons->handle);
            /* rc == 0 means we have messages available */
            replyAction(0, (void *)(uintptr_t)(rc == ISMRC_OK), action);
            break;

        case Action_createBrowser:
            /* val0=queue name, va1=selector */
            action->session = session;

            /* Check the name */
            if (action->valcount>0 && action->values[0].type == VT_String) {
                name = action->values[0].val.s;
            } else {
                name = NULL;
            }
            if (!name || !*name) {
                ism_common_setError(ISMRC_NoDestination);
                replyAction(ISMRC_NoDestination, NULL, action);
                break;
            }

            /* Compile selector if requested */
            if (action->valcount>1 && action->values[1].type == VT_String) {
                rulelen = 0;
                rc = ism_common_compileSelectRule(&action->rule, &rulelen, action->values[1].val.s);
                if (rc) {
                    replyAction(rc, NULL, action);
                    break;
                }
                TRACEL(7, transport->trclevel, "Select = %s\n", action->values[1].val.s);
            }
            if (session && session->handle) {
                action->session = session;
                consumer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,167),1,sizeof(ism_jms_prodcons_t));
                if (consumer) {
                    consumer->domain = ismDESTINATION_QUEUE;
                    consumer->name   = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),name);
                    consumer->kind = KIND_BROWSER;
                    id = setProdcons(transport, consumer);
                    consumer->which = id;
                    consumer->transport = transport;
                    consumer->handle = NULL;
                    consumer->rule = action->rule;
                    consumer->rulelen = rulelen;
                    memcpy(&consumer->hdr, &action->hdr, sizeof(struct actionhdr));
                    consumer->hdr.item = sessionid;
                    consumer->hdr.itemtype = ITEMT_Consumer;
                    consumer->cacheSize = 250;  /* Set block size to 1/4 client cache size of 1000 */
                    pthread_spin_init(&consumer->lock,0);
                    action->prodcons = consumer;
                    /* Engine restriction - messages are delivered asynchronously only */
                }
                if (id <= 0) {
                    if (id == 0) {
                        ism_common_setError(ISMRC_AllocateError);
                        replyCreateConsumer(ISMRC_AllocateError, NULL, action);
                    } else {
                        ism_common_setError(ISMRC_TooManyProdCons);
                        replyCreateConsumer(ISMRC_TooManyProdCons, NULL, action);
                    }
                    return true;
                }

                if (!pobj->browser_session_handle) {
                    pthread_mutex_lock(&jmslock);
                    if (!pobj->browser_session_handle) {
                        /* Call holding jmslock to prevent two calls at same time */
                        action->hdr.action = Action_createSession;
                        rc = ism_engine_createSession(pobj->handle,
                                ismENGINE_CREATE_SESSION_BROWSE_ONLY | ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                               &sessionh, action, sizeof(ism_protocol_action_t), replyCreateBrowser);
                        if (rc != ISMRC_AsyncCompletion)
                            replyCreateBrowser(rc, sessionh, action);
                    }
                    pthread_mutex_unlock(&jmslock);
                }
                else
                    replyCreateBrowser(0, NULL, action);

            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_createDurable:
            /* val0=durable val1=selector val2=maxmessages props=dest */

            /* Check the destination name */
            name = getproperty(action, "Name");
            if (!name || strlen(name) == 0) {
                ism_common_setError(ISMRC_NoDestination);
                replyAction(ISMRC_NoDestination, NULL, action);
                break;
            }

            /* Durable subscription to system topics are not allowed */
            if (name[0]=='$') {
                ism_common_setError(ISMRC_BadSysTopic);
                replyAction(ISMRC_BadSysTopic, NULL, action);
                break;
            }


            /* Check the durable name */
            subName = (action->valcount > 0) ? action->values[0].val.s : NULL;   /* Check type==string */
            if (!subName) {
                ism_common_setError(ISMRC_NameNotValid);
                replyAction(ISMRC_NameNotValid, NULL, action);
                break;
            }

            /* Check if noLocal is requested */
            action->nolocal = getbooleanproperty(action, "noLocal");

            /*
             * Check shared.
             * Shared subscription only works if the client version is at least 1.1
             */
            action->shared = 0;
            action->noConsumer = 0;
            const char * sprop = getproperty(action, "SubscriptionShared");
            if (sprop && transport->pobj->client_version >= ImaVersion_1_1) {
                if (!strcmpi(sprop, "False")) {
                    action->shared = SHARED_False;
                } else if (!strcmpi(sprop, "True")) {
                    action->shared = transport->pobj->isGenerated ? SHARED_Global : SHARED_True;
                } else if (!strcmpi(sprop, "NonDurable")) {
                    action->shared = transport->pobj->isGenerated ? SHARED_GlobalND : SHARED_NonDurable;
                } else if (!strcmpi(sprop, "GlobalShared")) {
                    action->shared = SHARED_Global;
                } else if (!strcmpi(sprop, "GlobalNoConsumer")) {
                    action->shared = SHARED_Global;
                    action->noConsumer = 1;
                } else {
                    ism_common_setError(ISMRC_BadClientData);
                    replyAction(ISMRC_BadClientData, NULL, action);
                }
            }

            /* Compile selector if requested */
            if (action->valcount>1 && action->values[1].type == VT_String) {
                rulelen = 0;
                rc = ism_common_compileSelectRule(&action->rule, &rulelen, action->values[1].val.s);
                action->rulelen = rulelen;
                if (rc) {
                    replyAction(rc, NULL, action);
                    break;
                }
            }

            if (session && session->handle) {
                void * clientState = pobj->handle;

                action->session = session;
                clientTrans = transport;
                switch (action->shared) {
                case SHARED_False:
                case SHARED_True:         SHARED_SUB_NAME_DURABLE(subName);                              break;
                case SHARED_NonDurable:   SHARED_SUB_NAME_NONDURABLE(subName);                           break;
                case SHARED_Global:       clientState = client_Shared;    clientTrans = transport_Shared;    break;
                case SHARED_GlobalND:     clientState = client_SharedND;  clientTrans = transport_SharedND;  break;
                }
                /* Ignore nolocal for shared subscriptions */
                if (action->shared) {
                    action->nolocal = 0;
                }

                /*
                 * We need to make sure that only a single subscription in in progress at a time for
                 * a connection (or pseudo connection).  If there is a subscribe in progress, reschedule
                 * onto the delivery thread.
                 */
                if (!__sync_bool_compare_and_swap(&clientTrans->pobj->subscribeLock, 0, 1)) {
                    action->clientTrans = clientTrans;
                    ism_protocol_action_t * actcopy = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,168),action->actionsize);
                    memcpy(actcopy, action, action->actionsize);
                    actcopy->values = (ism_field_t *)(actcopy+1);
                    actcopy->props  = (ism_propent_t *)(actcopy->values + actcopy->valcount);
                    action2actionBuff(actcopy);
                    clientTrans->addwork(clientTrans, doSubscribe, actcopy);
                    break;
                }
                action->clientTrans = clientTrans;
                /* Only a single subscription with this name can be found */
                action->subscriptionFound = SUB_NotFound;
                action2actionBuff(action);
                rc = ism_engine_listSubscriptions(clientState, (char *)subName, action, jmsReSubscribe);
                if (rc) {
                    replyAction(rc, NULL, action);
                } else {
                    if (action->subscriptionFound == SUB_NotFound) {
                        /* Create subscription and consumer */
                        action->recordCount = RESUB_CreateSubscription;
                        recreateConsumerAndSubscription(0, NULL, action);
                    } else if (action->subscriptionFound == SUB_Error) {
                        replyAction(action->rc, NULL, action);
                    }
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_updateSubscription:
            /* val0=name val1=shared */

            /* Check the durable name */
            subName = (action->valcount > 0) ? action->values[0].val.s : NULL; /* Check type==string */
            if (!subName) {
                ism_common_setError(ISMRC_NameNotValid);
                replyAction(ISMRC_NameNotValid, NULL, action);
                break;
            }

            if (isInternal() && session && session->handle) {
            	action->session = session;
            	replyUpdateDurable(rc, NULL, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_unsubscribeDurable:
            if (session && session->handle) {
                action->session = session;
                replyUnsubscribeDurable(rc, NULL, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_closeConsumer:
            action->session = session;

            if (session && session->handle) {
                /*
                 * ACK all delivered messages (if ack mode is not CLIENT)
                 */
                if (action->valcount) {
                    uint64_t maxsqn = action->values[0].val.l;
                    if (action->prodcons->msglistener){
                        rc = ackDeliveredMessages(transport, session, ismENGINE_CONFIRM_OPTION_CONSUMED,
                                maxsqn, maxsqn, 0, NULL, action, replyCloseConsumer);
                    } else {
                        uint64_t acksqn[2];
                        acksqn[0] = action->prodcons->which;
                        acksqn[1] = maxsqn;
                        rc = ackDeliveredMessages(transport, session, ismENGINE_CONFIRM_OPTION_CONSUMED,
                                0, maxsqn, 2, acksqn, action, replyCloseConsumer);
                    }
                }
                if(rc != ISMRC_AsyncCompletion)
                    replyCloseConsumer(rc, NULL, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_createProducer:
            domain = getDomain(action);
            name = getproperty(action, "Name");
            if (!name || strlen(name) == 0) {
                ism_common_setError(ISMRC_NoDestination);
                replyAction(ISMRC_NoDestination, NULL, action);
                break;
            }
            if (domain == ismDESTINATION_TOPIC &&
                name[0]=='$') {
                replyAction(ISMRC_BadSysTopic, NULL, action);
                break;
            }
            if (session) {
                ismEngine_ProducerHandle_t producerh;
                action2actionBuff(action);
                rc = ism_engine_createProducer(session->handle, domain, (char *)name,
                        &producerh, action, actionsize, replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                  replyAction(rc, producerh, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_closeProducer:
            if (session && session->handle) {
                prodcons = action->prodcons;
                if (prodcons) {
                    rc = ism_engine_destroyProducer(prodcons->handle,
                               action, sizeof(ism_protocol_action_t), replyAction);

                    if (rc != ISMRC_AsyncCompletion)
                        replyAction(rc, NULL, action);
                } else {
                    ism_common_setError(ISMRC_Closed);
                    replyAction(ISMRC_Closed, NULL, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_startConsumer:
            consumer = getProdcons(transport, action->prodcons->which);
            if (consumer &&
                ((consumer->cacheSize != 0)
                 || (consumer->session->ackmode == 2)
                 || (consumer->session->transacted)
                 || consumer->noack)) {

                /*
                 * We know the consumer calls resume when it reaches
                 * half of its capacity.  The server's cacheSize is set
                 * to (1/3 + 1) of the consumer cache size.
                 * Reset the server's inBatch count to 1 when the client
                 * resumes flow to allow up to (server) cacheSized more messages to
                 * be pushed to the client cache.
                 */
                int old_inbatch = __atomic_exchange_n(&consumer->inBatch, 1, __ATOMIC_SEQ_CST);

                TRACEL(7, transport->trclevel, "Submitting job request to resume consumer: " ""
                        "connect=%u client=%s consumer=%d(%p) cacheSize=%d inBatch=%d(was %d)\n",
                    transport->index, transport->name, action->prodcons->which, action->prodcons->handle,
                    consumer->cacheSize, consumer->inBatch, old_inbatch);
                ism_transport_submitAsyncJobRequest(transport, resumeConsumerDelivery,
                        (void*)((uintptr_t)action->prodcons->which),0);
            }
            break;

        case Action_suspendConsumer:
            TRACEL(7, transport->trclevel, "Suspend consumer request: connect=%u client=%s consumer=%d(%p)\n",
              transport->index, transport->name, action->prodcons->which, action->prodcons->handle);
            __sync_fetch_and_or(&action->prodcons->suspended, SUSPENDED_BY_PROTOCOL);
            if (__sync_sub_and_fetch(&pobj->inprogress, 1) == -1) { /* BEAM suppression: constant condition */
                ism_protocol_action_t act = { 0 };
                act.transport = transport;
                act.hdr.action = Action_closeConsumer;
                replyClosing(0, NULL, &act);
            }
            break;
        case Action_resumeSession:
            if (transport->addwork && pobj->started) {
                TRACEL(7, transport->trclevel, "Submitting job request to resume session: connect=%u client=%s session=%d(%p)\n",
                  transport->index, transport->name, action->session->which,action->session->handle);
                transport->addwork(transport, resumeSessionDelivery, (void *)(uintptr_t)action->session->which);
            } else {
              if (__sync_sub_and_fetch(&pobj->inprogress, 1) == -1) { /* BEAM suppression: constant condition */
                  ism_protocol_action_t act = { 0 };
                  act.transport = transport;
                  act.hdr.action = Action_closeConnection;
                  replyClosing(0, NULL, &act);
              }
            }
            break;
        case Action_setMsgListener:
            TRACEL(7, transport->trclevel, "Set Message listener: connect=%u client=%s consumer=%d\n",
                    transport->index, transport->name, action->prodcons->which);
            action->prodcons->msglistener = 1;
            replyAction(0, NULL, action);
            break;

        case Action_removeMsgListener:
            TRACEL(7, transport->trclevel, "Reset Message listener: connect=%u client=%s consumer=%d\n",
                    transport->index, transport->name, action->prodcons->which);
            action->prodcons->msglistener = 0;
            replyAction(0, NULL, action);
            break;

        case Action_rollbackSession:
            TRACEL(7, transport->trclevel, "Rollback JMS session: connect=%u client=%s session=%d\n", transport->index, transport->name, sessionid);
            if (session && session->handle) {
                /*
                 * The first header field is the last delivered sequence number.
                 */
                if ((!session->transacted) || (session->transaction == NULL)) {
                    ism_common_setError(ISMRC_BadClientData);
                    replyAction(ISMRC_BadClientData, NULL, action);
                    break;
                }
                action->session = session;
                if (action->valcount == 2) {
                   /*
                    * NACK all delivered messages (will cause dup count to increment)
                    */
                    uint64_t seqnum = action->values[1].val.l;
                    uint64_t maxsqn = 0;
                    uint64_t sqns[256];
                    uint32_t acksqn_count = action->values[0].val.i;
                    uint64_t * acksqn = getAckSqn(pobj,&map,acksqn_count,sqns,256,&maxsqn,&rc);

                    if (__builtin_expect((acksqn == NULL),0)) {
                        replyAction(rc, NULL, action);
                        break;
                    }

                    /* Even though we mark them consumed, they will be redelivered on rollback */
                    rc = ackDeliveredMessages(transport,session, ismENGINE_CONFIRM_OPTION_CONSUMED,
                            seqnum, maxsqn, acksqn_count, acksqn, action, replyRollbackSession);
                    if (__builtin_expect((acksqn != sqns),0)) {
                        ism_common_free(ism_memory_protocol_misc,acksqn);
                    }
                }
                if (rc != ISMRC_AsyncCompletion)
                    replyRollbackSession(rc, NULL, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_recover:
            TRACEL(7, transport->trclevel, "Recover JMS session: connect=%u client=%s session=%d lastAck=%lu\n",
                    transport->index, transport->name, sessionid, endian_int64(action->hdr.msgid));

            if (session && session->handle && !session->transacted) {
                /*
                 * Mark as NOT_RECEIVED all delivered messages
                 */
                action->session = session;
                if (action->valcount == 2) {
                    uint64_t seqnum = action->values[1].val.l;
                    uint64_t maxsqn = 0;
                    uint64_t sqns[256];
                    uint32_t acksqn_count = action->values[0].val.i;
                    uint64_t * acksqn = getAckSqn(pobj,&map,acksqn_count,sqns,256,&maxsqn,&rc);

                    if (__builtin_expect((acksqn == NULL),0)) {
                        replyAction(rc, NULL, action);
                        break;
                    }
                    rc = ackDeliveredMessages(transport, session, ismENGINE_CONFIRM_OPTION_NOT_RECEIVED,
                            seqnum, maxsqn, acksqn_count, acksqn, action, handleReplyRecover);
                    if (__builtin_expect((acksqn != sqns),0)) {
                        ism_common_free(ism_memory_protocol_misc,acksqn);
                    }
                }
                if (rc != ISMRC_AsyncCompletion)
                	handleReplyRecover(rc, NULL, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_initConnection:           /* First communications in session */
            /* Client type */
            if (action->hdr.hdrcount > 0 && action->values[0].type == VT_String) {
                if (strstr(action->values[0].val.s, "MQ")) {
                    transport->protocol_family = "MQ";
                    transport->protocol = "tcpmq";
                }
            }
            if (action->hdr.hdrcount > 1 && action->values[1].type == VT_String) {
                ism_common_strlcpy(pobj->client_build_id, action->values[1].val.s, 24);
            }
            if (action->hdr.hdrcount > 2 && action->values[2].type == VT_Integer) {
                pobj->keepaliveTimeout = action->values[2].val.i;
            } else {
                pobj->keepaliveTimeout = 0;
            }
            pobj->client_version = endian_int32(action->hdr.item);

            TRACEL(8, transport->trclevel, "JMS initConnection: client_library=%s client_build=%s client_version=%s keeepalive=%d\n",
                    action->values[0].type == VT_String ? action->values[0].val.s : "", pobj->client_build_id,
                    showVersion(pobj->client_version, trcbuf2), pobj->keepaliveTimeout);

            replyAction(0, NULL, action);
            break;

        case Action_getTime:                  /* Time */
            replyAction(0, NULL, action);
            break;

        case Action_termConnection:
            transport->closestate[3] = 1;
            replyAction(0, NULL, action);
            break;

        case Action_createQMRecord:
            if (isInternal() && session && session->handle && body.len) {
                ismEngine_QManagerRecordHandle_t qmHandle;

                rc = ism_engine_createQManagerRecord(session->handle, body.val.s, body.len,
                    &qmHandle, action, sizeof(ism_protocol_action_t), replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                    replyAction(rc, qmHandle, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }

            break;

        case Action_destroyQMRecord:
            if (isInternal() && session && session->handle &&
                    count > 0 && f[0].type  == VT_Long) {
                ismEngine_QManagerRecordHandle_t qmHandle =
                        (ismEngine_QManagerRecordHandle_t)f[0].val.l;

                rc = ism_engine_destroyQManagerRecord(session->handle, qmHandle,
                        action, sizeof(ism_protocol_action_t), replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                    replyAction(rc, NULL, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }

            break;

        case Action_getQMRecords:
            if (isInternal() && session && session->handle) {
                replyAction(0, NULL, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_createQMXidRecord:
            if (isInternal() && session && session->handle && body.len && count > 0 && f[0].type == VT_Long) {
                ismEngine_QManagerRecordHandle_t qmHandle =
                        (ismEngine_QManagerRecordHandle_t)f[0].val.l;
                ismEngine_QMgrXidRecordHandle_t xaHandle;

                rc = ism_engine_createQMgrXidRecord( session->handle, qmHandle,
                        session->transaction, body.val.s, body.len,
                        &xaHandle, action, sizeof(ism_protocol_action_t), replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                    replyAction(rc, xaHandle, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }

            break;

        case Action_destroyQMXidRecord:
            if (isInternal() && session && session->handle && count > 0 && f[0].type == VT_Long) {
                ismEngine_QMgrXidRecordHandle_t xaHandle =
                        (ismEngine_QMgrXidRecordHandle_t)f[0].val.l;

                rc = ism_engine_destroyQMgrXidRecord(session->handle, xaHandle,
                        session->transaction, action, sizeof(ism_protocol_action_t), replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                    replyAction(rc, NULL, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }

            break;

        case Action_getQMXidRecords:
            if (isInternal() && session && session->handle && count > 0 && f[0].type == VT_Long) {
                replyAction(0, (void *)f[0].val.l, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }

            break;

        case Action_listSubscriptions:
            if (session && session->handle) {
                replyAction(0, NULL, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_startGlobalTransaction:
            if (session && session->handle) {
                ismEngine_TransactionHandle_t transHandle;
                ism_xid_t xid;
                uint32_t flag = action->values[1].val.i;
                ism_common_toXid(&action->values[0], &xid);
                TRACEL(7, transport->trclevel, "jmsReceive: startGlobalTransaction connect=%u client=%s session=%d xid=%s flag=%u\n",
                         transport->index, transport->name, sessionid,
                         ism_common_xidToString(&xid,trcbuf,sizeof(trcbuf)), flag);

                rc = ism_engine_createGlobalTransaction(session->handle, &xid,
                       flag, &transHandle, action, sizeof(ism_protocol_action_t), replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                    replyAction(rc, transHandle, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_endGlobalTransaction:
            /* When disassociating a transaction from session,
             * make sure all messages received by this session are
             * added to the global transaction. */
            if (session && session->handle) {
                if (session->transaction && session->transacted == JMS_GLOBAL_TRANSACTION) {
                    action->flag = action->values[0].val.i;
                    action->session = session;
                    if (action->valcount > 1) {
                        uint64_t seqnum = action->values[action->valcount-1].val.l;
                        uint64_t maxsqn = 0;
                        uint64_t sqns[256];
                        uint32_t acksqn_count = action->values[action->valcount-2].val.i;
                        uint64_t * acksqn = getAckSqn(pobj,&map,acksqn_count,sqns,256,&maxsqn,&rc);

                        if (__builtin_expect((acksqn == NULL),0)) {
                            replyAction(rc, NULL, action);
                            break;
                        }
                        rc = ackDeliveredMessages(transport, session, ismENGINE_CONFIRM_OPTION_CONSUMED,
                                seqnum, maxsqn, acksqn_count, acksqn, action, replyEndGlobalTransaction);
                        if (__builtin_expect((acksqn != sqns),0)) {
                            ism_common_free(ism_memory_protocol_misc,acksqn);
                        }
                    }
                    TRACEL(7, transport->trclevel, "jmsReceive: endGlobalTransaction connect=%u client=%s session=%d transaction=%p flag=%u rc=%d\n",
                             transport->index, transport->name, sessionid,session->transaction, action->flag, rc);
                    if (rc != ISMRC_AsyncCompletion) {
                        replyEndGlobalTransaction(rc, NULL, action);
                    }
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_commitGlobalTransaction:
            if (session && session->handle) {
                ism_xid_t xid;
                uint32_t flag = (action->values[1].val.i) ? ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMONEPHASE : ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS;
                ism_common_toXid(&action->values[0], &xid);
                TRACEL(7, transport->trclevel, "jmsReceive: commitGlobalTransaction connect=%u client=%s session=%d xid=%s flag=%d\n",
                         transport->index, transport->name, sessionid,
                         ism_common_xidToString(&xid,trcbuf,sizeof(trcbuf)),flag);
                rc = ism_engine_commitGlobalTransaction(session->handle,
                        &xid, flag, action, sizeof(ism_protocol_action_t), replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                    replyAction(rc, NULL, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;


        case Action_prepareGlobalTransaction:
            if (session && session->handle) {
                ism_xid_t xid;
                ism_common_toXid(&action->values[0], &xid);
                TRACEL(7, transport->trclevel, "jmsReceive: prepareGlobalTransaction connect=%u client=%s session=%d xid=%s\n",
                         transport->index, transport->name, sessionid,
                         ism_common_xidToString(&xid,trcbuf,sizeof(trcbuf)));
                rc = ism_engine_prepareGlobalTransaction(session->handle,
                        &xid, action, sizeof(ism_protocol_action_t), replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                    replyAction(rc, NULL, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_rollbackGlobalTransaction:
            if (session && session->handle) {
                ism_xid_t xid;
                ism_common_toXid(&action->values[0], &xid);
                TRACEL(7, transport->trclevel, "jmsReceive: rollbackGlobalTransaction connect=%u client=%s session=%d xid=%s\n",
                         transport->index, transport->name, sessionid,
                         ism_common_xidToString(&xid,trcbuf,sizeof(trcbuf)));
                rc = ism_engine_rollbackGlobalTransaction(session->handle,
                        &xid, action, sizeof(ism_protocol_action_t), replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                    replyAction(rc, NULL, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        case Action_recoverGlobalTransactions:
            if (session && session->handle) {
                int32_t flags = (action->valcount > 0 && action->values[0].type == VT_Integer)
                        ?action->values[0].val.i
                        :0;
                count = (action->valcount > 1 && action->values[1].type == VT_Integer)
                        ?action->values[1].val.i
                        :0;
                ism_xid_t * xidArray = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,172),count * sizeof(ism_xid_t));

                if (count >= 0) {
                    action->recordCount = ism_engine_XARecover(session->handle, xidArray, count, 0, flags);
                } else {
                    rc = ISMRC_BadClientData;
                }
                if (rc) {
                    ism_common_free(ism_memory_protocol_misc,xidArray);
                    xidArray = NULL;
                }
                replyAction(rc, xidArray, action);
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;
        case Action_forgetGlobalTransaction:
            if (session && session->handle) {
                ism_xid_t xid;
                ism_common_toXid(&action->values[0], &xid);
                TRACEL(7, transport->trclevel, "jmsReceive: forgetGlobalTransaction connect=%u client=%s session=%d xid=%s\n",
                         transport->index, transport->name, sessionid,
                         ism_common_xidToString(&xid,trcbuf,sizeof(trcbuf)));
                rc = ism_engine_forgetGlobalTransaction(&xid, action, sizeof(ism_protocol_action_t), replyAction);
                if (rc != ISMRC_AsyncCompletion) {
                    replyAction(rc, NULL, action);
                }
            } else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
            }
            break;

        default:
            /* Unknown action, force disconnect */
            ism_common_setError(ISMRC_BadClientData);
            replyAction(ISMRC_BadClientData, NULL, action);
            break;
        }
    }


    /*
     * Process a message
     */
    else {
        size_t        areasize[2];
        void *        areaptr[2];
        ismMessageHeader_t mhdr;
        action->actionsize = sizeof(ism_protocol_action_t);

        TRACEL(9, transport->trclevel, "Action=%s connect=%u session=%d consumer=%d seqnum=%lu buflen=%u\n", getActionName(action->hdr.action),
                transport->index, sessionid, endian_int32(action->hdr.item), endian_int64(action->hdr.msgid), buflen);
        switch (action->hdr.action) {
        case Action_message:
        case Action_messageWait:
        case Action_messageNoProd:
        case Action_msgNoProdWait:
            transport->read_msg++;
            transport->listener->stats->count[transport->tid].read_msg++;
            /*
             * Set up the header
             */
            memset(&mhdr, 0, sizeof mhdr);
            mhdr.Priority = action->hdr.priority;

            /*
             * Set up persistence and reliability defaults
             */
            if (action->hdr.flags&ACTFLAG_Persistent) {    /* Persistent -> QoS = 1 or 2 */
                mhdr.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
                if (action->hdr.flags&0x01) {
                    mhdr.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;   /* QoS = 1 */
                } else {
                    mhdr.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;    /* QoS = 2 */
                }
            } else {                        /* Non-persistent -> QoS = 0 or 1 */
                mhdr.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;

                /* If ACKs are not disabled and QoS is > 0, use QoS 1 */
                if (session->transacted || (action->hdr.flags&0x07)) {
                    mhdr.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;    /* QoS = 1 */
                } else {
                    mhdr.Reliability = ismMESSAGE_RELIABILITY_AT_MOST_ONCE;     /* QoS = 0 */
                }
            }

            /*
             * For retained messages, add ID_ServerTime and ID_OriginServer to the
             * set of properties supplied by the client.
             */
            if (action->hdr.flags & ACTFLAG_RetainPublish) {
                const char * serverUID = ism_common_getServerUID();
                if (serverUID) {
                    ism_actionbuf_t xprops = {0};
                    xprops.len = pfield.len + strlen(serverUID) + 30;
                    xprops.buf = alloca(xprops.len);
                    if (pfield.len)
                        memcpy(xprops.buf, pfield.val.s, pfield.len);
                    xprops.used = pfield.len;
                    ism_protocol_putNameIndex(&xprops, ID_ServerTime);
                    ism_protocol_putLongValue(&xprops, ism_common_currentTimeNanos());
                    ism_protocol_putNameIndex(&xprops, ID_OriginServer);
                    ism_protocol_putStringValue(&xprops, serverUID);
                    pfield.len = xprops.used;
                    pfield.val.s = xprops.buf;
                }
                mhdr.Flags |= ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;
                mhdr.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
            }

            /* Set expiration into header */
            if ((action->hdr.flags&ACTFLAG_Expires) && pfield.len) {
                ism_actionbuf_t xprops = {0};
                xprops.buf = pfield.val.s;
                xprops.len = pfield.len;
                xprops.used = pfield.len;
                ism_field_t fld;
                ism_findPropertyNameIndex(&xprops, ID_Expire, &fld);
                if (fld.type == VT_Long && fld.val.l > 0) {
                    int64_t client_expiry = fld.val.l;
                    int64_t client_ttl;
                    /* 1. Find ID_Timestamp                         */
                    /* ID_Expire is never sent without ID_Timestamp */
                    ism_findPropertyNameIndex(&xprops, ID_Timestamp, &fld);
                    /* 2. Compute ttl with timestamp - expire */
                    client_ttl = client_expiry - fld.val.l;
                    /* 3. If ttl < 1000, set it to 1000 */
                    if (client_ttl < 1000)
                        client_ttl = 1000;
                    /* 4. Get server timestamp                    */
                    /* 5. Compute mhdr.Expiry as server ts + ttl  */
                    /* NOTE: This may change if engine is updated */
                    /*  to take ttl directly.                     */
                    mhdr.Expiry = ism_common_nowExpire() + (uint32_t)(client_ttl/1000);
                }

                /* JMS 2.0 delivery time */
                ism_findPropertyNameIndex(&xprops, ID_DeliveryTime, &fld);
                if (fld.type == VT_Long && fld.val.l > 0) {
                    action->deliveryTime = fld.val.l;
                }
            }
            mhdr.MessageType = action->hdr.bodytype;
            if (mhdr.MessageType == MTYPE_TextMessage && body.type != VT_ByteArray) {
                mhdr.MessageType = MTYPE_TextMessageNull;
            }

            /* Set the properties area */
            if (pfield.len) {
                areasize[0] = pfield.len;
                areaptr[0] = pfield.val.s;
            } else {
                areasize[0] = 0;
                areaptr[0] = NULL;
            }

            /* Set the body area */
            if (body.len) {
                areasize[1] = body.len;
                areaptr[1] = body.val.s;
            } else {
                areasize[1] = 0;
                areaptr[1] = NULL;
            }

            /*
             * Check max message length for this endpoint
             */
            if (transport->listener->maxMsgSize) {
                chklen = (int)areasize[1];
                if (areasize[0] > 200)
                    chklen += areasize[0]-200;
                if (chklen > transport->listener->maxMsgSize) {
                    ism_common_setError(ISMRC_MsgTooBig);
                    action->valcount = count;
                    action->values = &f[0];
                    action->recordCount = chklen;
                    replyMessage(ISMRC_MsgTooBig, NULL, action);
                    break;
                }
            }

            /*
             * If session is not found or session is marked as transacted, but no transaction
             * handle was created, report an error
             */
            if (!session || (session->transacted && session->transaction == NULL)) {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
                break;
            }

            /*
             * Create and send the message
             */
            if ((action->hdr.action == Action_messageNoProd) ||
                (action->hdr.action == Action_msgNoProdWait)) {
                /*
                 * If no producer was used to send the message, there are 2 headers
                 * containing destination name and domain
                 */
                const char * destinationName = NULL;
                domain = ismDESTINATION_QUEUE;
                for (i=0; i<count; i++) {
                    if (f[i].type == VT_String) {
                        destinationName = f[i].val.s;
                    } else if (f[i].type == VT_Byte) {
                        domain = f[i].val.i;
                    }
                }

                if (destinationName == NULL) {
                    TRACEL(5, transport->trclevel, "Mandatory destination name was not specified for send without producer: connect=%u client=%s session=%d\n",
                            transport->index, transport->name, session->which);
                    ism_common_setError(ISMRC_NoDestination);
                    replyAction(ISMRC_NoDestination, NULL, action);
                    break;
                }
                if (domain==ismDESTINATION_TOPIC && destinationName[0]=='$') {
                    rc = ISMRC_BadSysTopic;
                } else {
                    if (mhdr.Flags&ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN && domain==ismDESTINATION_QUEUE)
                        mhdr.Flags &= ~ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;
                    if (action->deliveryTime > 0) {
                        rc = delayDelivery(session, action->deliveryTime, action, &mhdr, areasize, areaptr);
                        if (rc != ISMRC_NotDelivered)
                            break;
                    }
                    /*
                     * Create the message
                     */
                    rc = ism_engine_createMessage(&mhdr, 2, MsgAreas, areasize, areaptr, &msgh);
                    if (rc) {
                        replyMessage(rc, NULL, action);
                        break;
                    }
                    /*
                     * Put the message to the engine
                     */
                    rc = ism_engine_putMessageOnDestination(session->handle, domain, destinationName,
                             session->transaction, msgh, action, sizeof(ism_protocol_action_t), replyMessage);
                }
                if (rc != ISMRC_AsyncCompletion) {
                    action->valcount = count;
                    action->values = &f[0];
                    replyMessage(rc, NULL, action);
                }
            } else {
                /* Do not allow the engine call if the action to close this producer is already in flight */
                if (!prodcons) {
                    ism_common_setError(ISMRC_BadClientData);
                    replyAction(ISMRC_BadClientData, NULL, action);
                    break;
                }
                if (mhdr.Flags&ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN && prodcons->domain==ismDESTINATION_QUEUE) {
                    mhdr.Flags &= ~ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;

                }
                if (action->deliveryTime > 0) {
                    rc = delayDelivery(session, action->deliveryTime, action,&mhdr, areasize, areaptr);
                    if (rc != ISMRC_NotDelivered)
                        break;
                }
                /*
                 * Create the message
                 */
                rc = ism_engine_createMessage(&mhdr, 2, MsgAreas, areasize, areaptr, &msgh);
                if (rc) {
                    replyMessage(rc, NULL, action);
                    break;
                }
                /*
                 * Put the message to the engine
                 */
                rc = ism_engine_putMessage(session->handle, prodcons->handle, session->transaction,
                          msgh, action, sizeof(ism_protocol_action_t), replyMessage);
                if (rc != ISMRC_AsyncCompletion) {
                    replyMessage(rc, NULL, action);
                }
            }
            break;

        case Action_receiveMsg:
            /* For now we implement receive in the client using a message listener */
            ism_common_setError(ISMRC_NotImplemented);
            replyAction(ISMRC_NotImplemented, NULL, action);
            break;

        /* Receive message no wait.  This is not implemented */
        case Action_receiveMsgNoWait:
            /* For now we implement receive in the client using a message listener */
            ism_common_setError(ISMRC_NotImplemented);
            replyAction(ISMRC_NotImplemented, NULL, action);     /* TODO: Not implemented */
            break;

        default:
            break;
        }
    }
    return 0;
}


