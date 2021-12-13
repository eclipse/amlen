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
#define TRACE_COMP Forwarder
#include <forwarder.h>
#include <assert.h>
/*
 * This file contains the sender side methods of the forwarder.
 *
 * When the forwarder is notified of a new remote server, it starts up a outgoing message
 * channel.  If it fails to connect, or the connection breaks it attempts to recreate
 * this channel.  When the channel is established, it send a Connect action.
 *
 * The receiver sends a ConnectReply and indicates if it knows of any global transactions
 * for this sender.  As a response, the sender sends a list of prepared transactions associated
 * with the receiver.  If there are none, and the sender has none then message processing can
 * start right away, otherwise message processing waits until the sender receives a Start action.
 *
 * This code handles the actions of Prepare, Rollback, and Commit and sends the associated
 * reply actions.
 *
 * When a message is received, it is send as a Message action (for QoS=0) or an RMessage action
 * (for QoS>0).  When a reliable message is sent, the engine delivery handle is remembered in
 * the delivery handle map.  This map is indexed by the sequence number which is incremented for
 * each message sent.  When an ACK (prepare) or NAK (rollback) is sent, these delivery handles
 * handles are looked up in the map.
 */

/*
 * Forward references
 */
int ism_fwd_sendRecover(ism_transport_t * transport);
int  ism_fwd_listDeliveryHandle(ism_fwd_channel_t * channel, uint64_t * seqn, ismEngine_DeliveryHandle_t * deliveryh, int incount);
int  ism_fwd_addDeliveryHandle(ism_fwd_channel_t * channel, uint64_t seqn, ismEngine_DeliveryHandle_t deliveryh);
ism_fwd_channel_t * ism_fwd_findChannel(const char * uid);

/*
 * Outgoing connection connected callback.
 * This is called when the outgoing TCP channel is connected, and it send the Connect action
 * to start the forwarder handshake.
 */
int  ism_fwd_connected(ism_transport_t * transport, int rc) {
    char xbuf[2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    ismFwdPobj_t * pobj = transport->pobj;
    ism_fwd_channel_t * channel = pobj->channel;

    TRACE(5, "Outgoing forwarder connected: connect=%d name=%s rc=%d\n", transport->index, transport->name, rc);

    /*
     * There is a TCP connection for the channel
     */
    if (rc == 0) {
        /* Set the state */
        pthread_mutex_lock(&fwd_configLock);
        pthread_mutex_lock(&channel->lock);
        if((channel->out_channel != transport) || (channel->cc_state != CHST_Open)) {
            char err[256];
            sprintf(err, "Outgoing channel %p(%p) is in invalid state: %d", transport, channel->out_channel, channel->out_state);
            pthread_mutex_unlock(&channel->lock);
            pthread_mutex_unlock(&fwd_configLock);
            transport->close(transport, ISMRC_Error, 1, err);
            return 0;
        }
        channel->out_state = CHST_Open;
        if (channel->in_state == CHST_Open)
            ism_cluster_remoteServerConnected(channel->clusterHandle);
        channel->connections++;
        pthread_mutex_unlock(&channel->lock);
        pthread_mutex_unlock(&fwd_configLock);

        /* Reset the delivery handle map */
        pthread_mutex_lock(&channel->dhlock);
        if (channel->dhmap)
            ism_common_free(ism_memory_protocol_misc,channel->dhmap);
        channel->dhmap    = NULL;
        channel->dhcount  = 0;             /* The count of entries in the map */
        channel->dhmore   = 0;             /* The point at which we expand the map */
        channel->dhalloc  = 0;             /* The allocated slots in the table */
        channel->dhextra  = 0;
        pthread_mutex_unlock(&channel->dhlock);

        pthread_spin_lock(&transport->lock);
        transport->ready = 1;
        channel->retry = 1;
        transport->state = ISM_TRANST_Open;
        pthread_spin_unlock(&transport->lock);

        ism_protocol_putIntValue(&buf, fwd_Version_Current);
        ism_protocol_putLongValue(&buf, ism_common_currentTimeNanos());
        if (fwd_unit_test) {
            ism_protocol_putStringValue(&buf, pobj->channel->name);
            ism_protocol_putStringValue(&buf, pobj->channel->uid);
        } else {
            ism_protocol_putStringValue(&buf, ism_common_getServerName());
            ism_protocol_putStringValue(&buf, ism_common_getServerUID());
        }
        transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_Connect<<8)+4, SFLAG_FRAMESPACE);
        ism_common_freeAllocBuffer(&buf);
    }

    /*
     * The TCP connection failed
     */
    else {
        transport->close(transport, rc, 1, "Failed to create fwd outgoing connection.");
    }
    return 0;
}


/*
 * On ConnectReply, create engine objects for a sender forwarding channel.
 * This is restartable as an engine reply. If the engine functions return synchronously,
 * the methods fall thru the cases of the switch statement.
 *
 * The outgoing channel consists of a client state, a session, a consumer for QoS=0,
 * and a consumer for QoS>0.
 */
static void fwdConnectReplyAction(int32_t rc, void * handle, void * vaction) {
    ism_fwd_act_t * action = (ism_fwd_act_t *)vaction;
    ism_transport_t * transport = action->transport;
    ismFwdPobj_t * pobj = transport->pobj;
    int   options;
    ismEngine_ClientStateHandle_t client;
    ismEngine_SessionHandle_t sessionh;
    ismEngine_ConsumerHandle_t consumer;
    int done = 0;

    action->rc = rc;

    switch (action->paction) {

    /*
     * Create the client state
     */
    case Action_createConnection:
        action->paction = Action_createSession;

        rc = ism_engine_createClientState(transport->clientID, PROTOCOL_ID_FWD, ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                transport, NULL, transport->security_context, &client,
                action, sizeof(*action), fwdConnectReplyAction);
        TRACE(7, "createClientState name=%s rc=%d\n", transport->clientID, rc);
        if (rc == ISMRC_AsyncCompletion)
            break;
        action->rc = rc;
        handle = client;
        /* fall thru */

    /*
     * Create the session
     */
    case Action_createSession:
        if (action->rc == 0) {
            pobj->client_handle = handle;
            action->paction = Action_createConsumer;
            options = ismENGINE_CREATE_SESSION_TRANSACTIONAL | ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND;
            rc = ism_engine_createSession(pobj->client_handle, options, &sessionh, action, sizeof(ism_fwd_act_t), fwdConnectReplyAction);
            TRACE(7, "createSession name=%s rc=%d\n", transport->clientID, rc);
            if (rc == ISMRC_AsyncCompletion)
                break;
            action->rc = rc;
            handle = sessionh;
        }
        /* fall thru */

    /*
     * Create the QoS=0 consumer
     */
    case Action_createConsumer:
        if (action->rc == 0) {
            uint32_t consumerOptions =
                    ismENGINE_CONSUMER_OPTION_LOW_QOS | ismENGINE_CONSUMER_OPTION_NONPERSISTENT_DELIVERYCOUNTING;
            pobj->session_handle = handle;
            action->paction = Action_createConsumer+1;
            rc = ism_engine_createRemoteServerConsumer(pobj->session_handle,
                pobj->channel->engineHandle, pobj->consumer, sizeof(ism_fwd_cons_t),
                ism_fwd_replyMessage, consumerOptions,
                &consumer, action, sizeof action, fwdConnectReplyAction);
            TRACE(7, "createConsumerLow name=%s rc=%d\n", transport->clientID, rc);
            if (rc == ISMRC_AsyncCompletion)
                break;
            action->rc = rc;
            handle = consumer;
        }
        /* fall thru */

    /*
     * Create the QoS=1 consumer
     */
    case Action_createConsumer+1:
        if (action->rc == 0) {
            uint32_t consumerOptions =
                    ismENGINE_CONSUMER_OPTION_HIGH_QOS | ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_NONPERSISTENT_DELIVERYCOUNTING;
            pobj->consumer[0].handle = handle;
            action->paction = Action_startConsumer;

            rc = ism_engine_createRemoteServerConsumer(pobj->session_handle,
                pobj->channel->engineHandle, pobj->consumer+1, sizeof(ism_fwd_cons_t),
                ism_fwd_replyMessage, consumerOptions,
                &consumer, action, sizeof action, fwdConnectReplyAction);
            TRACE(7, "createConsumerHigh name=%s rc=%d\n", transport->clientID, rc);
            if (rc == ISMRC_AsyncCompletion)
                break;
            action->rc = rc;
            handle = consumer;
        }
        /* fall thru */

    /*
     * Start message delivery
     */
    case Action_startConsumer:
        if (action->rc == 0) {
            ism_fwd_channel_t * channel = transport->pobj->channel;
            pobj->consumer[1].handle = handle;
            if (channel->receiver_xa || channel->sender_xa || !action->options) {
                fwd_xa_t * xaListHead = NULL;
                fwd_xa_t * xaListTail = NULL;
                fwd_xa_t * xa;
                pthread_mutex_lock(&channel->lock);
                for( xa = channel->sender_xa; xa != NULL; xa = xa->next) {
                    fwd_xa_t * xaCopy = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,219),sizeof(fwd_xa_t));
                    memcpy(xaCopy, xa, sizeof(fwd_xa_t));
                    xaCopy->next = NULL;
                    if(xaListTail) {
                        xaListTail->next = xaCopy;
                    } else {
                        xaListHead = xaCopy;
                    }
                    xaListTail = xaCopy;
                }
                pthread_mutex_unlock(&channel->lock);
                if(xaListHead) {
                    pthread_spin_lock(&pobj->sessionlock);
                    pobj->xaRecoveryList = xaListHead;
                    pthread_spin_unlock(&pobj->sessionlock);
                }
                ism_fwd_sendRecover(transport);
            } else {
                rc = ism_engine_startMessageDelivery(pobj->session_handle,
                    ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
                if (rc == ISMRC_AsyncCompletion)
                    rc = 0;
                action->rc = rc;
            }
            done = 1;
        }
    }

    if (action->rc) {
        transport->close(transport, action->rc, 0, "Unable to create forwarding channel");
        done = 1;
    }

    if (done) {
        int32_t ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
        TRACE(8, "Leave replyAction, index=%u inprogress=%d\n", transport->index, ipcount);
        if (UNLIKELY(ipcount < 0)) {
            ism_fwd_replyCloseClient(transport);
        }
    }
}


/*
 * Do the connect reply action.
 * All of the work is handled in replyAction
 */
int ism_fwd_doConnectReply(ism_fwd_act_t * action, int rc, int version, uint64_t tstamp, int flags) {
    action->options = flags;
    action->paction = Action_createConnection;
    fwdConnectReplyAction(rc, NULL, action);
    return 0;
}


/*
 * Handle the Processed action
 */
int ism_fwd_doProcessed(ism_fwd_act_t * action, uint64_t seqnum) {
    ism_transport_t * transport = action->transport;
    ismFwdPobj_t * pobj = (ismFwdPobj_t *) transport->pobj;

    /*
     * Resume forwarder if only 1 ack in progress
     */
    pobj = (ismFwdPobj_t *) transport->pobj;
    pthread_spin_lock(&pobj->sessionlock);
    if (--pobj->flowControlAcks == 1) {
        transport->resume(transport, NULL);
    }
    pthread_spin_unlock(&pobj->sessionlock);
    return 0;
}


/*
 * Start or restart an outgoing forwarder channel
 */
int ism_fwd_startChannel(ism_fwd_channel_t * channel) {
    ismFwdPobj_t * pobj;
    struct ssl_ctx_st * tlsCTX = NULL;
    char clientid[32];
    int  rc;
    ism_transport_t * transport = ism_transport_newOutgoing(ism_fwd_getOutEndpoint(), 1);
    if (!transport) {
        return -1;
    }

    pthread_mutex_lock(&channel->lock);
    if (channel->out_channel || (channel->port == 0) || (channel->cc_state != CHST_Open)) {
        ism_transport_freeTransport(transport);
        pthread_mutex_unlock(&channel->lock);
        return 0;
    }
    if (channel->retry == 0) {
        TRACE(4, "Start forwarding channel: name=%s uid=%s\n", channel->name, channel->uid);
    } else {
        TRACE(6, "Retry forwarding channel connect: name=%s\n", channel->name);
    }
    channel->out_channel = transport;
    pthread_mutex_unlock(&channel->lock);
    /*
     * Create outgoing connection
     */
    ism_security_create_context(ismSEC_POLICY_CONNECTION, transport, &transport->security_context);
    transport->protocol = "fwd";
    ism_fwd_connection(transport);
    pobj = transport->pobj;
    transport->connected = ism_fwd_connected;   /* Callback when connected */
    transport->closing = ism_fwd_closing;
    pobj->channel = channel;
    pthread_spin_init(&pobj->lock, 0);
    pthread_spin_init(&pobj->sessionlock, 0);
    pobj->consumer[0].transport = transport;
    pobj->consumer[1].transport = transport;
    pobj->consumer[1].which = 1;
    pobj->sqnum = 1;
    pobj->flowControlCount = fwd_flowCount;
    pobj->flowControlSize = fwd_flowSize;
    if (channel->secure) {
        tlsCTX = fwd_tlsCTX;
    }
    transport->ready = 0;

    /*
     * Copy clientid into outgoing transport object
     */
    transport->name = ism_transport_putString(transport, channel->name);
    strcpy(clientid, "__Sender_");
    ism_common_strlcat(clientid, channel->uid, sizeof(clientid));
    transport->clientID = ism_transport_putString(transport, clientid);
    transport->userid = "";

    /*
     * Do the connect, this completes in ism_fwd_connected()
     */
    transport->ready = 3;
    rc = ism_transport_connect(transport, NULL, channel->ipaddr, channel->port, tlsCTX);
    if (rc) {
        char xbuf[256];
        ism_common_formatLastError(xbuf, sizeof xbuf);
        transport->close(transport, rc, 0, xbuf);
        channel->retry = 0;
        channel->status_time = ism_common_currentTimeNanos();
    }
    return rc;
}


/*
 * Handle a start action
 */
int ism_fwd_doStart(ism_fwd_act_t * action) {
    ism_transport_t * transport = action->transport;
    ismFwdPobj_t * pobj = (ismFwdPobj_t *) transport->pobj;

    xUNUSED int zrc = ism_engine_startMessageDelivery(pobj->session_handle,
        ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
    int32_t ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
    TRACE(8, "Leave ism_fwd_doStart, index=%u inprogress=%d\n", transport->index, ipcount);
    if (ipcount < 0) { /* BEAM suppression: constant condition */
        ism_fwd_replyCloseClient(transport);
    }
    return 0;
}


/*
 * Reply with a received message.  This is the engine callback from the message consumer.
 */
bool ism_fwd_replyMessage(ismEngine_ConsumerHandle_t consumerh,
        ismEngine_DeliveryHandle_t deliveryh, ismEngine_MessageHandle_t msgh,
        uint32_t seqnum, ismMessageState_t state, uint32_t options,
        ismMessageHeader_t * hdr, uint8_t areas,
        ismMessageAreaType_t areatype[areas], size_t areasize[areas],
        void * areaptr[areas], void * vaction, ismEngine_DelivererContext_t * _delivererContext) {
    char xbuf[12000];
    concat_alloc_t buf = { xbuf, sizeof xbuf, 6 };
    uint32_t proplen = 0;
    uint32_t bodylen = 0;
    char * propp = NULL;
    char * bodyp = NULL;
    int i;
    bool returncode = true;
    uint8_t flags = 0;
    uint8_t qos = (hdr->Reliability & 3);
    ism_fwd_cons_t * consumer;
    uint64_t sqnum = 0;

    ism_transport_t * transport = NULL;
    ism_protobj_t * pobj = NULL;
    int actionType;

    consumer = (ism_fwd_cons_t *) vaction;
    transport = consumer->transport;
    pobj = transport->pobj;

    flags = qos;

    /* Find the props and body */
    for (i = 0; i < areas; i++) {
        if (areatype[i] == ismMESSAGE_AREA_PROPERTIES) {
            proplen = (uint32_t) areasize[i];
            propp = (char *) areaptr[i];
        } else if (areatype[i] == ismMESSAGE_AREA_PAYLOAD) {
            bodylen = (uint32_t) areasize[i];
            bodyp = (char *) areaptr[i];
        }
    }


    if (hdr->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN)
        flags |= 0x18;

    if (hdr->Persistence)
        flags |= 0x04;

    /*
     * QoS>0
     */
    if (qos) {
        assert(deliveryh);
        actionType = (FwdAction_RMessage<<8)+5;
        sqnum = pobj->sqnum++;
        ism_fwd_addDeliveryHandle(pobj->channel, sqnum, deliveryh);
        pthread_spin_lock(&pobj->sessionlock);
        if (pobj->preparedXA > fwd_maxXA) {
            if (!pobj->suspended) {
                consumer->suspended = 1;
                pobj->channel->suspend1++;
                TRACE(7, "Suspend fwd qos0 name=%s count=%lu\n", transport->name, pobj->flowControlAcks);
                ism_engine_suspendMessageDelivery(consumerh, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
                returncode = false;
            }
            pobj->suspended |= SUSPENDED_BY_PROTOCOL;
        }
        pthread_spin_unlock(&pobj->sessionlock);
    }

    /*
     * QoS=0
     */
    else {
        actionType = (FwdAction_Message<<8)+5;
        if (transport->write_bytes > pobj->flowControlSize || transport->write_msg > pobj->flowControlCount) {
            /* Increment flowControlAcks counter */
            if (++(pobj->flowControlAcks) > 1) {   /* ACK counter */
                pthread_spin_lock(&pobj->sessionlock);
                if (!pobj->suspended) {
                    consumer->suspended = 1;
                    pobj->channel->suspend0++;
                    TRACE(7, "Suspend fwd qos0 name=%s count=%lu\n", transport->name, pobj->flowControlAcks);
                    ism_engine_suspendMessageDelivery(consumerh, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
                    returncode = false;
                }
                pobj->suspended |= SUSPENDED_BY_PROTOCOL;
                pthread_spin_unlock(&pobj->sessionlock);
            }
            sqnum = pobj->sqnum++;     /* Sequence number */
            pobj->flowControlCount = transport->write_msg + fwd_flowCount;
            pobj->flowControlSize = transport->write_bytes + fwd_flowSize;
        }
        /* QoS=0 only statistics */
        pobj->flowWriteMsg++;
        pobj->flowWriteBytes += (proplen+bodylen);
    }

    ism_protocol_putLongValue(&buf, sqnum);   /* seqnum QoS 0 not full */

    /*
     * Set channel statistics.
     * We keep stats in the channel but not in the transport or endpoint objects.
     */
    pobj->channel->write_msg++;
    pobj->channel->write_bytes += (proplen+bodylen);

    /*
     * Write the message action
     */
    ism_protocol_putByteValue(&buf, hdr->MessageType);
    ism_protocol_putByteValue(&buf, flags);
    ism_protocol_putNullValue(&buf);
    ism_protocol_putIntValue(&buf, hdr->Expiry);
    if (proplen)
        ism_protocol_putMapValue(&buf, propp, proplen);
    else
        ism_protocol_putNullValue(&buf);
    ism_protocol_putByteArrayValue(&buf, bodyp, bodylen);

    pthread_spin_lock(&pobj->sessionlock);

    /*
     * Send the message
     */
    int rc = transport->send(transport, buf.buf+6, buf.used-6, actionType, SFLAG_FRAMESPACE);
    if (UNLIKELY(rc == SRETURN_SUSPEND)) {
        TRACE(7, "Suspend fwd transport: %s\n", transport->name);

        pobj->suspended |= SUSPENDED_BY_TRANSPORT;
        ism_engine_suspendMessageDelivery(consumerh, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
        if (!pobj->suspended) {
            if (consumer->which)
                pobj->channel->suspend1++;
            else
                pobj->channel->suspend0++;
        }
        returncode = false;
    }
    pthread_spin_unlock(&pobj->sessionlock);


    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
    ism_engine_releaseMessage(msgh);

    return returncode;
}


/*
 * Start message delivery for all registered subscribers.
 * @param transport  The transport object
 * @param userdata   The user object
 * @param flags      The option flags
 * @return A return code 0=good.
 */
int ism_fwd_startDelivery(ism_transport_t * transport, void * userdata, uint64_t flags) {
    int rc = 0;
    ismFwdPobj_t * pobj = (ismFwdPobj_t *) transport->pobj;

    /* If the connection or session has been closed, do not start message delivery. */
    if (!pobj) {
        return 0;
    }

    pthread_spin_lock(&pobj->sessionlock);
    if (pobj->session_handle && !pobj->closed) {
        int wassuspended = pobj->suspended;
        int mask = (userdata) ? (~SUSPENDED_BY_TRANSPORT) : (~SUSPENDED_BY_PROTOCOL);
        pobj->suspended &= mask;
        if (pobj->suspended == 0) {
            pthread_spin_unlock(&pobj->sessionlock);
            TRACE(7, "Resume fwd %s was=%d\n", transport->name, wassuspended);
            rc = ism_engine_startMessageDelivery(pobj->session_handle,
                    ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
        } else {
            pthread_spin_unlock(&pobj->sessionlock);
        }
    } else {
        pthread_spin_unlock(&pobj->sessionlock);
    }

    int32_t ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
    TRACE(9, "Leave start delivery, index=%u inprogress=%d\n", transport->index, ipcount);
    if (UNLIKELY(ipcount < 0)) {
        ism_fwd_replyCloseClient(transport);
    }
    return rc;
}


/*
 * Resume message delivery for the forwarder session for the transport.
 *
 * @param transport  The transport object
 * @param userdata   The data context for the work item
 * @return A return code: 0=good
 */
int ism_fwd_resume(ism_transport_t * transport, void * userdata) {
    ismFwdPobj_t * pobj = (ismFwdPobj_t *) transport->pobj;
    int32_t ipcount = __sync_fetch_and_add(&transport->pobj->inprogress, 1);
    TRACE(9, "Leave ism_fwd_resume, index=%u inprogress=%d\n", transport->index, ipcount);
    if (UNLIKELY(ipcount < 0)) { /* BEAM suppression: constant condition */
        __sync_fetch_and_sub(&pobj->inprogress, 1);
        return 0;
    }
    ism_transport_submitAsyncJobRequest(transport, ism_fwd_startDelivery, userdata, 0);
    return 0;
}


/*
 * Reply to a prepare.
 * This does the actual work of the prepare which is to create a global transaction,
 * ACK all messages in that transaction, and call the engine to prepare the transaction.
 *
 * Each of these engine methods can return async, in which case this method is redriven.
 * If the engine returns synchronously, this method continues by falling thru to the next
 * case.
 */
void replyPrepare(int rc, void * handle, void * vaction) {
    fwd_xa_action_t * action = (fwd_xa_action_t *)vaction;
    ism_transport_t * transport = action->transport;
    ismFwdPobj_t * pobj = (ismFwdPobj_t *) transport->pobj;
    ism_fwd_channel_t * channel = pobj->channel;
    char xbuf [512];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    fwd_xa_t * xa;
    if (rc)
        action->action = Action_reply;

    switch (action->action) {

    /*
     * Create the global transaction
     */
    case Action_createTransaction:
        action->action = Action_ack;
        TRACE(7, "replyPrepare: createGlobalTransaction: name=%s index=%u gtrid=%s\n",
                 transport->clientID, transport->index, action->gtrid);
        xa = ism_fwd_makeXA(action->gtrid,'S', action->sequence);
        ism_fwd_linkXA(channel, xa, 1, 1);
        rc = ism_engine_createGlobalTransaction(pobj->session_handle, &xa->xid,
                ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT,
                &action->transh, action, sizeof(fwd_xa_action_t), replyPrepare);
        if (rc == ISMRC_AsyncCompletion)
            break;
        handle = action->transh;
        action->xa = xa;
        /* Fall thru */

    /*
     * Confirm message delivery of all messages in the transaction
     */
    case Action_ack:
        if(!rc) {
            xa = action->xa;
            action->xa = NULL;
            action->transh = handle;
            action->action = Action_prepareGlobalTransaction;
            TRACE(7, "replyPrepare: confirmMessageDeliveryBatch: name=%s index=%u gtrid=%s\n",
                     transport->clientID, transport->index, action->gtrid);
            rc = ism_engine_confirmMessageDeliveryBatch(transport->pobj->session_handle, action->transh,
                     action->count, action->deliveryh, ismENGINE_CONFIRM_OPTION_CONSUMED,
                     action, sizeof *action, replyPrepare);
            if (rc == ISMRC_AsyncCompletion)
                break;
            action->xa = xa;
        }
        /* Fall thru */

    /*
     * Prepare the global transaction
     */
    case Action_prepareGlobalTransaction:
        if(!rc) {
            xa = action->xa;
            action->xa = NULL;
            if(!xa) {
                xa = ism_fwd_findXA(channel, action->gtrid, 1,1);
            }
            if(xa) {
                action->action = Action_reply;
                xa->prepared = 1;
                TRACE(7, "replyPrepare: prepareGlobalTransaction: name=%s index=%u gtrid=%s\n",
                         transport->clientID, transport->index, action->gtrid);
                rc = ism_engine_prepareGlobalTransaction(transport->pobj->session_handle, &xa->xid,
                    action, sizeof *action, replyPrepare);
                if (rc == ISMRC_AsyncCompletion)
                    break;
            } else {
                rc = ISMRC_NotFound;
            }
        } else {
            pthread_mutex_lock(&channel->lock);
            xa = ism_fwd_findXA(channel, action->gtrid, 1,0);
            ism_fwd_unlinkXA(pobj->channel, xa, 1, 0);
            pthread_mutex_unlock(&channel->lock);
        }
        /* Fall thru */

    /*
     * Send the PrepareReply action back to the receive side
     */
    case Action_reply:
        if (action->inheap) {
            action->inheap = 0;
            ism_common_free(ism_memory_protocol_misc,action->deliveryh);
            action->deliveryh = NULL;
        }
        if(!rc) {
            pthread_spin_lock(&transport->pobj->sessionlock);
            transport->pobj->preparedXA++;
            pthread_spin_unlock(&transport->pobj->sessionlock);
        }

        ism_protocol_putStringValue(&buf, action->gtrid);
        ism_protocol_putIntValue(&buf, rc);
        transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_PrepareReply<<8)+2, SFLAG_FRAMESPACE);
        int32_t ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
        TRACE(9, "replyPrepare: sendReply inprogress=%d name=%s index=%u inprogress=%d gtrid=%s\n", ipcount,
                 transport->clientID, transport->index, ipcount, action->gtrid);
        if (UNLIKELY(ipcount < 0)) {
            ism_fwd_replyCloseClient(transport);
        }
    }

}


/*
 * Handle a prepare.
 * Most of the work is done in replyPrepare.
 */
int ism_fwd_doPrepare(ism_fwd_act_t * action, const char * gtrid, int count) {
    ism_transport_t * transport = action->transport;
    ism_fwd_channel_t * channel = transport->pobj->channel;
    fwd_xa_action_t act = {0};
    int rc = ISMRC_OK;

    if (count != action->body.len/8) {
        action->rc = ISMRC_BadClientData;
        rc = ISMRC_BadClientData;
        ism_common_setError(action->rc);
    }
    act.which = 0;
    act.op    = 'P';
    act.action = Action_createTransaction;
    act.deliveryh = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,221),count * sizeof(ismEngine_DeliveryHandle_t));
    act.inheap = 1;
    act.transport = transport;
    strcpy(act.gtrid, gtrid);
    act.sequence = action->seqnum;
    act.count = ism_fwd_listDeliveryHandle(channel, (uint64_t *)action->body.val.s, act.deliveryh, count);
    replyPrepare(rc, NULL, &act);
    return 0;
}


/*
 * Reply to a commit.
 */
void replyCommit(int rc, void * handle, void * vaction) {
    fwd_xa_action_t * action = (fwd_xa_action_t *)vaction;
    ism_transport_t * transport = action->transport;
    ism_fwd_channel_t * channel = transport->pobj->channel;
    char xbuf[1024];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    fwd_xa_t * xa;

    pthread_mutex_lock(&channel->lock);
    xa = ism_fwd_findXA(channel, action->gtrid, 1, 0);
    if (xa) {
        ism_fwd_unlinkXA(channel, xa, 1, 0);
    }
    pthread_mutex_unlock(&channel->lock);
    if(xa)
        ism_common_free(ism_memory_protocol_misc,xa);

    pthread_spin_lock(&transport->pobj->sessionlock);
    if(transport->pobj->preparedXA) {
        transport->pobj->preparedXA--;
        if (transport->pobj->preparedXA <= fwd_minXA) {
            if (transport->pobj->suspended)
                transport->resume(transport, NULL);
        }
    }
    pthread_spin_unlock(&transport->pobj->sessionlock);

    /* Reply to the commit */
    ism_protocol_putStringValue(&buf, action->gtrid);
    ism_protocol_putIntValue(&buf, rc);
    transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_CommitReply<<8)+2, SFLAG_FRAMESPACE);

    if (action->op == 'B') {
        ism_fwd_sendRecover(transport);
    }

    int32_t ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
    TRACE(9, "Leave commit, index=%d inprogress=%d\n", transport->index, ipcount);
    if (UNLIKELY(ipcount < 0)) {
        ism_fwd_replyCloseClient(transport);
    }
}


/*
 * Handle a commit.
 * The transaction must already be prepared, but this is controlled by the transaction
 * manager which is the receive side.
 */
int ism_fwd_doCommit(ism_fwd_act_t * action, const char * gtrid, int recover) {
    ism_transport_t * transport = action->transport;
    fwd_xa_action_t act = {0};
    ism_xid_t xid;
    int   rc;

    ism_fwd_makeXid(&xid, 'S', gtrid);
    act.op = recover ? 'B' : 'C';
    strcpy(act.gtrid, gtrid);
    act.transport = transport;

    rc = ism_engine_commitGlobalTransaction(transport->pobj->session_handle, &xid,
            ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT,
            &act, sizeof act, replyCommit);

    if (rc != ISMRC_AsyncCompletion) {
        replyCommit(rc, NULL, &act);
    }
    return 0;
}

/*
 * Reply to a rollback prepared
 */
static void replyRollbackPrepared(int32_t rc, void * handle, void * vaction) {
    fwd_xa_action_t * action = (fwd_xa_action_t *)vaction;
    ism_transport_t * transport = action->transport;
    ism_fwd_channel_t * channel = transport->pobj->channel;
    char xbuf [512];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    fwd_xa_t * xa;

    pthread_mutex_lock(&channel->lock);
    xa = ism_fwd_findXA(channel, action->gtrid, 1, 0);
    if (xa) {
        ism_fwd_unlinkXA(channel, xa, 1, 0);
        ism_common_free(ism_memory_protocol_misc,xa);
    }
    pthread_mutex_unlock(&channel->lock);
    pthread_spin_lock(&transport->pobj->sessionlock);
    if(transport->pobj->preparedXA) {
        transport->pobj->preparedXA--;
        if (transport->pobj->preparedXA <= fwd_minXA) {
            if (transport->pobj->suspended)
                transport->resume(transport, NULL);
        }
    }
    pthread_spin_unlock(&transport->pobj->sessionlock);
    if (action->op == 'R') {
        ism_fwd_sendRecover(transport);
    }

    /* Reply to the rollback */
    ism_protocol_putStringValue(&buf, action->gtrid);
    ism_protocol_putIntValue(&buf, rc);
    transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_RollbackReply<<8)+2, SFLAG_FRAMESPACE);

    int32_t ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
    TRACE(9, "Leave rollback prepared, index=%d inprogress=%d\n", transport->index, ipcount);
    if (UNLIKELY(ipcount < 0)) {
        ism_fwd_replyCloseClient(transport);
    }
}

/*
 * Rollback a prepared transaction
 */
int ism_fwd_doRollbackPrepared(ism_fwd_act_t * action, const char * gtrid, int recover) {
    ism_transport_t * transport = action->transport;
    fwd_xa_action_t act = {0};
    ism_xid_t xid;
    int   rc;

    ism_fwd_makeXid(&xid, 'S', gtrid);
    act.op = recover ? 'R' : 'D';
    strcpy(act.gtrid, gtrid);
    act.transport = transport;
    rc = ism_engine_rollbackGlobalTransaction(transport->pobj->session_handle, &xid,
            &act, sizeof act, replyRollbackPrepared);

    if (rc != ISMRC_AsyncCompletion) {
        replyRollbackPrepared(rc, NULL, &act);
    }
    return 0;
}

/*
 * Reply to a rollback
 */
static void replyRollback(int32_t rc, void * handle, void * vaction) {
    fwd_xa_action_t * action = *(fwd_xa_action_t * *)vaction;
    ism_transport_t * transport = action->transport;
    char xbuf [512];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};

    if (action->inheap) {
        action->inheap = 0;
        ism_common_free(ism_memory_protocol_misc,action->deliveryh);
    }
    ism_protocol_putStringValue(&buf, action->gtrid);
    ism_protocol_putIntValue(&buf, rc);
    transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_RollbackReply<<8)+2, SFLAG_FRAMESPACE);

    int32_t ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
    TRACE(9, "Leave reply rollback, index=%d inprogress=%d\n", transport->index, ipcount);
    if (UNLIKELY(ipcount < 0)) {
        ism_fwd_replyCloseClient(transport);
    }
}


/*
 * Handle a rollback action
 */
int ism_fwd_doRollback(ism_fwd_act_t * action, const char * xid, int count) {
    ism_transport_t * transport = action->transport;
    ism_fwd_channel_t * channel = transport->pobj->channel;
    fwd_xa_action_t act;
    int  rc;
    ismEngine_DeliveryHandle_t xdeliveryh [256];

    if (count != action->body.len/8) {
        action->rc = ISMRC_BadClientData;
        ism_common_setError(action->rc);
    }

    act.which = 0;
    act.op    = 'R';
    strcpy(act.gtrid, xid);
    if (count <= 256) {
        act.deliveryh = xdeliveryh;
    } else {
        act.deliveryh = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,225),count * sizeof(ismEngine_DeliveryHandle_t));
        act.inheap = 1;
    }
    act.count = ism_fwd_listDeliveryHandle(channel, (uint64_t *)action->body.val.s, act.deliveryh, count);
    rc = ism_engine_confirmMessageDeliveryBatch(transport->pobj->session_handle, NULL,
            act.count, act.deliveryh, ismENGINE_CONFIRM_OPTION_NOT_DELIVERED,
            &act, sizeof act, replyRollback);
    if (rc != ISMRC_AsyncCompletion) {
        replyRollback(0, NULL, &act);
    }
    return 0;
}


/*
 * Request retained messages
 */
int ism_fwd_doRequestRetain(ism_fwd_act_t * action, const char * originUID, uint32_t options, ism_time_t tstamp, uint64_t corrid) {
    int  rc = 0;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;
    ism_fwd_channel_t * channel = pobj->channel;
    /*
     * Call the engine
     */
    rc = ism_engine_forwardRetainedMessages(originUID, options, tstamp, corrid, channel->uid, NULL, 0, NULL);
    action->rc = rc;
    return rc;
}


/*
 * Send a recover action.
 *
 * This send back the next prepared global transactions
 * at the send side for the receiver of this channel.
 * When there are no more, send a zero length gtrid.
 */
int ism_fwd_sendRecover(ism_transport_t * transport) {
    ismFwdPobj_t * pobj = transport->pobj;
    fwd_xa_t * xa;
    char xbuf [256];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};

    pthread_spin_lock(&pobj->sessionlock);
    xa = pobj->xaRecoveryList;
    if (xa) {
        ism_protocol_putStringValue(&buf, xa->gtrid);
        pobj->xaRecoveryList = xa->next;
        pobj->preparedXA++;
    } else {
        ism_protocol_putStringValue(&buf, "");
    }
    pthread_spin_unlock(&pobj->sessionlock);
    if(xa)
        ism_common_free(ism_memory_protocol_misc,xa);

    transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_Recover<<8)+1, SFLAG_FRAMESPACE);
    return 0;
}

/*
 * Rotate left 55 bits, should be optimized by compiler
 */
static inline uint64_t fuzz(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed669ccd;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53;
    x ^= x >> 33;
    return x;
}

/*
 * Add a delivery handle to the hash, expanding the hash table if required.
 */
int  ism_fwd_addDeliveryHandle(ism_fwd_channel_t * channel, uint64_t seqn, ismEngine_DeliveryHandle_t deliveryh) {
    int   newalloc;
    uint32_t slot;
    uint32_t slot1;
    int   newdiv;
    int   i;
    uint32_t   extra;

    pthread_mutex_lock(&channel->dhlock);
    if (++channel->dhcount > channel->dhmore) {
        fwd_dhmap_t * newmap;
        uint32_t newextra = 0;
        if (channel->dhalloc < 2048)
            newalloc = 4096;
        else
            newalloc = channel->dhalloc * 4;
        newmap = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,227),newalloc, sizeof(fwd_dhmap_t));
        if (!newmap) {
            TRACE(1, "Unable to allocate delivery handle mapping table");
            return ISMRC_AllocateError;
        }
        TRACE(3, "Expand delivery handle mapping table: name=%s old=%u new=%u\n",
                channel->name, channel->dhalloc, newalloc);

        /*
         * Increase the size of the hash map and rehash all entries
         */
        for (i=0; i<channel->dhalloc; i++) {
            uint64_t val = channel->dhmap[i].seqn;
            if (val) {
                extra = 0;
                newdiv = newalloc-1;
                slot1 = slot = (uint32_t)(fuzz(val)%newdiv);
                while (newmap[slot].seqn != 0) {
                    slot++;
                    extra++;
                    if (slot == newalloc)
                        slot = 0;
                }
                if (extra > newextra)
                    newextra = extra;
                newmap[slot].seqn = val;
                newmap[slot].deliveryh = channel->dhmap[i].deliveryh;
            }
        }
        if (channel->dhmap) {
            ism_common_free(ism_memory_protocol_misc,channel->dhmap);
        }
        channel->dhmap = newmap;
        channel->dhalloc = newalloc;
        channel->dhmore = newalloc*6/10;
        channel->dhdiv = newalloc-1;
        channel->dhextra = newextra;
    }
    slot1 = slot = (uint32_t)(fuzz(seqn)%channel->dhdiv);
    extra = 0;
    while (channel->dhmap[slot].seqn != 0) {
        slot++;
        extra++;
        if (slot == channel->dhalloc)
            slot = 0;
    }
    if (extra > channel->dhextra)
        channel->dhextra = extra;
    TRACE(9, "Add delivery handle: slot1=%u slot=%u dhextra=%u extra=%u seq=%lx to %lx:%lx\n", slot1, slot, channel->dhextra, extra, seqn,
            (uint64_t)(deliveryh>>64), (uint64_t)deliveryh);
    channel->dhmap[slot].seqn = seqn;
    channel->dhmap[slot].deliveryh = deliveryh;

    pthread_mutex_unlock(&channel->dhlock);
    return 0;
}


/*
 * Find a delivery handle from a sequnece number and optional remove it.
 */
ismEngine_DeliveryHandle_t ism_fwd_findDeliveryHandle(ism_fwd_channel_t * channel, uint64_t seqn, int remove) {
    ismEngine_DeliveryHandle_t ret = (ismEngine_DeliveryHandle_t)0;
    int  slot;
    int  count = channel->dhextra;

    pthread_mutex_lock(&channel->dhlock);
    slot = (uint32_t)(fuzz(seqn)%channel->dhdiv);
    while (count-- >= 0) {
        if (channel->dhmap[slot].seqn == seqn)
            break;
        slot++;
        if (slot == channel->dhalloc)
            slot = 0;
    }
    if (channel->dhmap[slot].seqn == seqn) {
        if (remove) {
            channel->dhmap[slot].seqn = 0;
            channel->dhcount--;
        }
        ret = channel->dhmap[slot].deliveryh;
    } else {
        // TRACE(1, "Delivery handle not found: slot=%u seqnum=%lx\n",  slot, seqn);
    }
    pthread_mutex_unlock(&channel->dhlock);
    return ret;
}


/*
 * Convert a list of sequence numbers to a list of delivery handles.
 * The delivery handle is removed from the has by this call.
 *
 * This is used to return a list of sequence numbers for Prepare or Rollback.
 */
int  ism_fwd_listDeliveryHandle(ism_fwd_channel_t * channel, uint64_t * seqn, ismEngine_DeliveryHandle_t * deliveryh, int incount) {
    int  i;
    uint32_t slot;
    uint32_t slot1;
    uint64_t seqnum;
    fwd_dhmap_t * dhmap;
    int outcount = 0;

    pthread_mutex_lock(&channel->dhlock);
    dhmap = channel->dhmap;
    for (i=0; i<incount; i++) {
        uint32_t  count = channel->dhextra;
        memcpy(&seqnum, seqn+i, 8);   /* Force alignment */
        slot = (uint32_t)(fuzz(seqnum)%channel->dhdiv);
        slot1 = slot;
        while (count-- >= 0) {
            if (dhmap[slot].seqn == seqnum)
                break;
            slot++;
            if (slot == channel->dhalloc)
                slot = 0;
        }
        if (dhmap[slot].seqn == seqnum) {
            //TRACE(9, "List delivery handle: i=%d slot1=%u slot=%u %lx to %lx:%lx\n", i, slot1, slot, seqn[i],
            //        (uint64_t)(dhmap[slot].deliveryh>>64), (uint64_t)dhmap[slot].deliveryh);
            dhmap[slot].seqn = 0;
            channel->dhcount--;
            deliveryh[outcount++] = dhmap[slot].deliveryh;
            dhmap[slot].deliveryh = 0;
        } else {
            TRACE(1, "Delivery handle not found: i=%d slot1=%u slot=%u dhextra=%u seqnum=%lx, count=%d\n", i, slot1, slot, channel->dhextra, seqnum, count);
        }
    }
    pthread_mutex_unlock(&channel->dhlock);
    return outcount;
}


/*
 * Request retained messages from another server - this goes down the back-channel to the other server
 * This is a callback from the engine.
 */
int32_t ism_fwd_requestRetain(const char * respondUID, const char * originUID, uint32_t options, uint64_t tstamp, uint64_t corrid) {
    ism_fwd_channel_t * channel;
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    int  rc = 0;

    pthread_mutex_lock(&fwd_configLock);
    channel = ism_fwd_findChannel(respondUID);
    pthread_mutex_unlock(&fwd_configLock);
    if (channel) {
        if (channel->in_state == CHST_Open) {
            ism_protocol_putStringValue(&buf, originUID);
            ism_protocol_putIntValue(&buf, options);
            ism_protocol_putLongValue(&buf, tstamp);
            ism_protocol_putLongValue(&buf, corrid);
            channel->in_channel->send(channel->in_channel, buf.buf+6, buf.used-6, (FwdAction_RequestRetain<<8)+4, SFLAG_FRAMESPACE);
            ism_common_freeAllocBuffer(&buf);
        } else {
            rc = ISMRC_Closed;
        }
    } else {
        rc = ISMRC_NotFound;
    }
    return rc;
}


/*
 * Retry the outgoing connection
 */
int ism_fwd_retryOutgoing(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_fwd_channel_t * channel = (ism_fwd_channel_t *)userdata;
    int callStart = 0;
    pthread_mutex_lock(&channel->lock);
    // printf("retryOutgoing delay=%ums name=%s channel=%p timer=%p,%p\n", channel->retry,
    //        channel->name, channel, key, channel->retry_timer);
    if(channel->retry_timer) {
        ism_common_cancelTimer(channel->retry_timer);
        channel->retry_timer = NULL;
        if (channel->out_channel == NULL)
            callStart = 1;
    }
    pthread_mutex_unlock(&channel->lock);
    if (!fwd_stopping && callStart)
        ism_fwd_startChannel(channel);
    return 0;
}
