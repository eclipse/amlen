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

/*
 * The forwarder acts as both end of the reliable channel.  Although it uses XA style
 * transactions some of the function is not needed because it is known to be at both ends.
 *
 * The XIDs used within the forwarder are assumed to always be of the forwarder formatID.
 * The XID is normally passed across the forwarder interface as a string containing only the
 * global transaction ID.  For this to work, the character set must be strictly limited to
 * alphanumerics and a few special characters.  The engine interfaces take the XID in binary
 * format which includes the formatID and branch qualifier.
 *
 * The global transaction ID is of the form: SendServerUID_RecvServerUID_sequence.  The
 * sequence number is unique by RecvServerUID.  If at recovery any global transactions are
 * known on the receiver, the sequence number is set to higher than the largest known
 * sequence.  Otherwise the sequence number is set to 1.
 *
 * The space is not allowed within a forwarder XID, and so a list of XID strings is given
 * as a space separated list.
 *
 * The engine allows multiple global transactions to be associated with a session.  The
 * forwarder uses this so that when a transaction is full it can immediately switch to
 * a new transaction while preparing the old one.  A transaction can become full based on
 * message count, size in bytes, or elapsed time.  Limiting the number of un-commited
 * transactions forms the flow control on the reliable channel.
 */


/*
 * Forward references
 */
ism_fwd_channel_t * ism_fwd_findChannel(const char * uid);
ism_fwd_channel_t * ism_fwd_newChannel(const char * serverUID, const char * serverName);
int  ism_fwd_listDeliveryHandle(ism_fwd_channel_t * channel, uint64_t * seqn, ismEngine_DeliveryHandle_t * deliveryh, int count);
void ism_fwd_replyCloseClient(ism_transport_t * transport);
static void replyEngineCommit(int32_t rc, void * handle, void * vaction);
static void fwdCreateXA(ism_transport_t * transport);
static void fwdReliableACK(fwd_msgact_t * action);
/*
 * Sequence for the global transactions
 */
static uint64_t fwd_xid_seqn = 0;

static fwd_xa_info_t * createXAInfo(const char * gtrid, void * handle, uint64_t sequence) {
    fwd_xa_info_t * xaInfo = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,229),sizeof(fwd_xa_info_t)+(2*fwd_commit_count*sizeof(uint64_t)));
    xaInfo->seqmax = 2*fwd_commit_count;
    xaInfo->seqnum = (uint64_t*)(xaInfo+1);
    xaInfo->seqcount = 0;
    strcpy(xaInfo->gtrid, gtrid);
    xaInfo->xaSequence = sequence;
    xaInfo->handle = handle;
    xaInfo->next = NULL;
    xaInfo->prev = NULL;
    xaInfo->readyMsgCounter = 0;
    return xaInfo;
}

static void destroyXAInfo(fwd_xa_info_t * xaInfo) {
    if(xaInfo){
        if(xaInfo->seqnum != ((uint64_t*)(xaInfo+1)))
            ism_common_free(ism_memory_protocol_misc,xaInfo->seqnum);
        ism_common_free(ism_memory_protocol_misc,xaInfo);
    }
}

static inline int addMessageToXA(ism_transport_t * transport, fwd_xa_info_t * xaInfo, uint64_t sqn) {
    TRACE(9, "addMessageToXA clientID=%s index=%u seqnum=%lu seqcount=%u seqmax=%u trans=%p\n",
            transport->clientID, transport->index, sqn, xaInfo->seqcount, xaInfo->seqmax, xaInfo->handle);
    if(xaInfo->seqcount == xaInfo->seqmax) {
        int newMax = 2*xaInfo->seqmax;
        uint64_t * seqnum = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,232),newMax*sizeof(uint64_t));
        TRACE(5, "addMessageToXA - realloc:  clientID=%s index=%u xid=%s seqcount=%u seqmax=%u newmax=%u\n",
                transport->clientID, transport->index, xaInfo->gtrid, xaInfo->seqcount, xaInfo->seqmax, newMax);
        memcpy(seqnum, xaInfo->seqnum, xaInfo->seqcount*sizeof(uint64_t));
        if(xaInfo->seqnum != ((uint64_t*)(xaInfo+1)))
            ism_common_free(ism_memory_protocol_misc,xaInfo->seqnum);
        xaInfo->seqnum = seqnum;
        xaInfo->seqmax = newMax;
    }
    xaInfo->seqnum[xaInfo->seqcount++] = sqn;
    return xaInfo->seqcount;
}

void ism_fwd_cleanPendingXAs(ism_protobj_t * pobj) {
    fwd_xa_t * xa = NULL;
    pthread_spin_lock(&pobj->sessionlock);
    if(pobj->channel) {
        pthread_mutex_lock(&pobj->channel->lock);
        if(pobj->currentXA) {
            xa = ism_fwd_findXA(pobj->channel, pobj->currentXA->gtrid, 0, 0);
            ism_fwd_unlinkXA(pobj->channel, xa, 0, 0);
            destroyXAInfo(pobj->currentXA);
            pobj->currentXA = NULL;
            if(xa)
                ism_common_free(ism_memory_protocol_misc,xa);
        }
        while(pobj->xaListHead) {
            fwd_xa_info_t * xaInfo = pobj->xaListHead;
            pobj->xaListHead = pobj->xaListHead->next;
            xa = ism_fwd_findXA(pobj->channel, xaInfo->gtrid, 0, 0);
            ism_fwd_unlinkXA(pobj->channel, xa, 0, 0);
            destroyXAInfo(xaInfo);
            if(xa)
                ism_common_free(ism_memory_protocol_misc,xa);
        }
        pobj->xaListTail = NULL;
        pobj->xaInfoListSize = 0;
        pthread_mutex_unlock(&pobj->channel->lock);
    }
    while(pobj->xaRecoveryList) {
        xa = pobj->xaRecoveryList;
        pobj->xaRecoveryList = xa->next;
        ism_common_free(ism_memory_protocol_misc,xa);
    }
    pthread_spin_unlock(&pobj->sessionlock);
}

/*
 * Find a transaction
 */
fwd_xa_t * ism_fwd_findXA(ism_fwd_channel_t * channel, const char * xid, int sender, int lock) {
    fwd_xa_t * xa;

    if (lock)
        pthread_mutex_lock(&channel->lock);
    xa = sender ? channel->sender_xa : channel->receiver_xa;
    while (xa) {
        if (!strcmp(xid, xa->gtrid))
            break;
        xa = xa->next;
    }
    if (lock)
        pthread_mutex_unlock(&channel->lock);
    return  xa;
}


/*
 * Link in a transaction.
 * The transactions are linked in order from smallest to largest sequence.
 */
void ism_fwd_linkXA(ism_fwd_channel_t * channel, fwd_xa_t * xa, int sender, int lock) {
    fwd_xa_t * * head = sender ? &channel->sender_xa : &channel->receiver_xa;

    if (lock)
        pthread_mutex_lock(&channel->lock);
    if (*head == NULL || (*head)->sequence > xa->sequence) {
        xa->next = *head;
        *head = xa;
    } else {
        fwd_xa_t * prevxa = *head;
        fwd_xa_t * thisxa = prevxa->next;
        while (thisxa && thisxa->sequence < xa->sequence) {
            prevxa = thisxa;
            thisxa = thisxa->next;
        }
        xa->next = prevxa->next;
        prevxa->next = xa;
    }
    if (lock)
        pthread_mutex_unlock(&channel->lock);
}


/*
 * Unlink a transaction
 */
void ism_fwd_unlinkXA(ism_fwd_channel_t * channel, fwd_xa_t * xa, int sender, int lock) {
    if(xa) {
        fwd_xa_t * * head = sender ? &channel->sender_xa : &channel->receiver_xa;
        if (lock)
            pthread_mutex_lock(&channel->lock);
        if (*head == NULL)
            return;
        if (*head == xa) {
            *head = xa->next;
        } else {
            fwd_xa_t * prevxa = *head;
            while (prevxa->next && prevxa->next != xa) {
                prevxa = prevxa->next;
            }
            if (prevxa->next)
                prevxa->next = xa->next;
        }
        if (lock)
            pthread_mutex_unlock(&channel->lock);
    }
}


/*
 * We always use two message areas, one for properties and the other for payload
 */
static ismMessageAreaType_t MsgAreas[2] = {
    ismMESSAGE_AREA_PROPERTIES,
    ismMESSAGE_AREA_PAYLOAD
};

/*
 * Continue creating engine objects for an incoming forwarding channel
 */
static void replyConnect(int32_t rc, void * handle, void * vaction) {
    fwd_conact_t * action = (fwd_conact_t *)vaction;
    ism_transport_t * transport = action->transport;
    ismFwdPobj_t * pobj = transport->pobj;
    int   options = 0;
    ismEngine_ClientStateHandle_t client = NULL;
    ismEngine_SessionHandle_t sessionh = NULL;
    ismEngine_TransactionHandle_t transh = NULL;
    char xbuf[2048];

    action->rc = rc;

    switch (action->action) {
    /*
     * Create the client state
     */
    case Action_createConnection:
        action->action = Action_createSession;

        rc = ism_engine_createClientState(transport->clientID, PROTOCOL_ID_FWD, ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                transport, NULL, transport->security_context, &client,
                action, sizeof(fwd_conact_t), replyConnect);
        if (rc == ISMRC_AsyncCompletion)
            return;
        action->rc = rc;
        handle = client;
        /* fall thru */

    /*
     * Create the session
     */
    case Action_createSession:
        TRACE(7, "Forwarder create receiver ClientState name=%s index=%u rc=%d\n",
                transport->clientID, transport->index, rc);
        if (action->rc == 0) {
            pobj->client_handle = handle;
            action->action = Action_createTransaction;
            options = ismENGINE_CREATE_SESSION_TRANSACTIONAL | ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND;
            rc = ism_engine_createSession(pobj->client_handle, options, &sessionh,
                    action, sizeof(fwd_conact_t), replyConnect);
            TRACE(7, "Forwarder create incoming session name=%s rc=%d\n", transport->clientID, rc);
            if (rc == ISMRC_AsyncCompletion)
                return;
            action->rc = rc;
        }
        handle = sessionh;
        /* fall thru */

    /*
     * Create the global transaction
     */
    case Action_createTransaction:
        TRACE(7, "Forwarder create receiver session name=%s index=%u rc=%d\n",
                transport->clientID, transport->index, rc);
        if (action->rc == 0) {
            pobj->session_handle = handle;
            if (pobj->channel->receiver_xa == NULL && pobj->channel->sender_xa == NULL) {
                char gtrid[64];
                uint64_t sequence = ism_fwd_newGtrid(gtrid, pobj->channel->uid);
                fwd_xa_t * xa = ism_fwd_makeXA(gtrid, 'R', sequence);
                ism_fwd_linkXA(pobj->channel, xa, 0, 1);
                fwd_xa_info_t * xaInfo = createXAInfo(gtrid, NULL, sequence);
                pthread_spin_lock(&pobj->sessionlock);
                pobj->currentXA = xaInfo;
                pthread_spin_unlock(&pobj->sessionlock);
                action->action = Action_reply;
                pobj->channel->start_xa = ism_common_readTSC();
                rc = ism_engine_createGlobalTransaction(transport->pobj->session_handle, &xa->xid,
                        ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT,
                        &transh, action, sizeof(fwd_conact_t), replyConnect);
                if (rc == ISMRC_AsyncCompletion)
                    return;
                action->rc = rc;
            }
        }
        handle = transh;
        /* fall thru */

    /*
     * Send the reply
     */
    case Action_reply:
        if (action->action == Action_reply) {
            TRACE(7, "Forwarder create receiver transaction name=%s index=%urc=%d\n",
                    transport->clientID, transport->index, rc);
        }
        if (action->rc == 0) {
            concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
            if(handle) {
                pobj->currentXA->handle = handle;
                pobj->transaction = handle;
            }

            if (action->version > fwd_Version_Current)
                action->version = fwd_Version_Current;

            ism_protocol_putIntValue(&buf, action->version);
            ism_protocol_putLongValue(&buf, ism_common_currentTimeNanos());
            ism_protocol_putIntValue(&buf, rc);
            ism_protocol_putIntValue(&buf, (handle != NULL));  /* Auto start */
            transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_ConnectReply<<8)+4, SFLAG_FRAMESPACE);

        } else {
            pthread_mutex_lock(&pobj->channel->lock);
            fwd_xa_t * xa = ism_fwd_findXA(pobj->channel, pobj->currentXA->gtrid, 0, 0);
            ism_fwd_unlinkXA(pobj->channel, xa, 0, 0);
            destroyXAInfo(pobj->currentXA);
            pobj->currentXA = NULL;
            pthread_mutex_unlock(&pobj->channel->lock);
            ism_common_free(ism_memory_protocol_misc,xa);
        }
    }

    /* Decrement inprogress.  If close is in progress, proceed with it */
    int32_t ipcount = __sync_sub_and_fetch(&pobj->inprogress, 1);
    TRACE(6, "Leave reply connect, index=%u inprogress=%d\n", transport->index, ipcount);
    if (UNLIKELY(ipcount < 0)) {
        ism_fwd_replyCloseClient(transport);
    } else {
        if (action->rc) {
            transport->close(transport, action->rc, 0, "Unable to create forwarding channel");
        }
    }
}

static int fwdHandleClientIdReuse(ism_transport_t * transport, void * param1, uint64_t param2) {
    const char * msg = (const char *)param1;
    int rc = (int) (uintptr_t)param2;
    transport->close(transport, rc, 0, msg);
    return 0;
}
static int fwdCloseConnectionTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_transport_t * transport = (ism_transport_t *) userdata;
    ismFwdPobj_t * pobj = transport->pobj;
    int32_t ipcount = __sync_sub_and_fetch(&pobj->inprogress, 1);
    if (__builtin_expect((ipcount < 0), 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "Leave fwdCloseConnectionTimer, index=%u inprogress=%d\n", transport->index, ipcount);
        ism_fwd_replyCloseClient(transport);
    } else {
        transport->close(transport, ISMRC_ClientIDReused, 0, "Previous incoming channel is still open");
    }
    ism_common_cancelTimer(key);
    return 0;
}

/*
 * Process a connect action.
 * Look up the channel and if found attach this connection as the incomming connection.
 */
int ism_fwd_doConnect(ism_fwd_act_t * action, uint32_t version, uint64_t timest,
        const char * name, const char * uid) {
    ism_fwd_channel_t * channel;
    ism_transport_t * transport = action->transport;
    fwd_conact_t act = {0};
    char clientid[32];


    if (!name)
        name = "";
    transport->name = ism_transport_putString(transport, name);
    strcpy(clientid, "__Receiver_");
    ism_common_strlcat(clientid, uid, sizeof(clientid));
    transport->clientID = ism_transport_putString(transport, clientid);
    transport->ready = 2;

    pthread_mutex_lock(&fwd_configLock);
    channel = ism_fwd_findChannel(uid);

    /*
     * If the channel does not exist yet, create it.
     * The notifies on the two systems are in a race condition, and either could come before the other.
     */
    if (!channel) {
        TRACE(5, "Forwarder create channel on connect: ServerName=%s ServerUID=%s\n", name, uid);
        channel = ism_fwd_newChannel(uid, name);
    }
    pthread_mutex_lock(&channel->lock);

    if (channel->in_channel && channel->in_state == CHST_Open) {
        TRACE(3, "Submit job to close the previous incoming channel: index=%u ServerName=%s ServerUID=%s\n",
                channel->in_channel->index, channel->name, channel->uid);
        ism_transport_submitAsyncJobRequest(channel->in_channel, fwdHandleClientIdReuse, "Close previous incoming channel", ISMRC_ClientIDReused);
        ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t) fwdCloseConnectionTimer, transport, 1000000000);
        pthread_mutex_unlock(&channel->lock);
        pthread_mutex_unlock(&fwd_configLock);
        return 0;
    }

    channel->in_channel = transport;
    channel->in_state = CHST_Open;
    if (channel->out_state == CHST_Open) {
        channel->status_time = ism_common_currentTimeNanos();
        ism_cluster_remoteServerConnected(channel->clusterHandle);
    }
    transport->pobj->channel = channel;
    act.action = Action_createConnection;
    act.transport = transport;
    act.timestamp = timest;
    act.version = version;
    channel->connections++;

    pthread_mutex_unlock(&channel->lock);
    pthread_mutex_unlock(&fwd_configLock);

    replyConnect(0, NULL, &act);
    return 0;
}


/*
 * Reply to an incoming message
 */
static void fwdReplyPublish(int32_t rc, void * handle, void * vaction) {
    fwd_msgact_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;

    if (action->seqnum) {
        /* Unreliable */
        if (action->action == FwdAction_Message) {
            char xbuf[64];
            concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
            ism_protocol_putLongValue(&buf, action->seqnum);
            TRACEL(9, transport->trclevel, "Forwarder sending %s action for seqnum=%llu\n", ism_fwd_getActionName(FwdAction_Processed), (ULL)action->seqnum);
            transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_Processed<<8)+1, SFLAG_FRAMESPACE);
        }

        /* Reliable */
        else {
            action->rc = rc;
            if (rc) {
                /* Ignore informational return codes */
                if (rc == ISMRC_SomeDestinationsFull ||
                    rc == ISMRC_NoMatchingDestinations ||
                    rc == ISMRC_NoMatchingLocalDestinations) {
                      action->rc = ISMRC_OK;
                } else {
                    char xbuf[1024];
                    int ipcount = __sync_sub_and_fetch(&pobj->inprogress, 1);
                    ism_common_formatLastError(xbuf, sizeof xbuf);
                    TRACE(2, "Publish of forwarded message failed: index=%u name=%s inprogress=%d rc=%u, error=%s",
                            transport->index, transport->name, ipcount, rc, xbuf);
                    if(ipcount < 0) {
                        ism_fwd_replyCloseClient(transport);
                    } else {
                       transport->close(transport, rc, 0, xbuf);
                    }
                    return;
                }
            }
            fwdReliableACK(action);
        }
    }
    /* If close is in progress, proceed with it */
    int32_t ipcount = __sync_sub_and_fetch(&pobj->inprogress, 1);
    TRACE(9, "Leave reply publish, index=%u inprogress=%d\n", transport->index, ipcount);
    if (__builtin_expect((ipcount < 0), 0)) { /* BEAM suppression: constant condition */
        ism_fwd_replyCloseClient(transport);
    }
}
/*
 * Process an incoming message action
 */
int ism_fwd_doMessage(ism_fwd_act_t * action, uint64_t seqnum, int msgtype,
        int flags, const char * dest, uint32_t expiry, ism_field_t * props, ism_field_t * body) {
    int  rc = 0, ipcount;
    ismEngine_MessageHandle_t msgh;
    ismMessageHeader_t hdr;
    size_t areasize[2];
    void * areaptr[2];
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    uint8_t headerFlags = (flags & 0x18) ? ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN : 0;

    if (props->type == VT_Map) {
        ism_field_t f;
        concat_alloc_t pbuf = {props->val.s, props->len, props->len};
        /*
         * If dest is not specified, look it up from the topic in the properties
         */
        if (!dest) {
            ism_findPropertyNameIndex(&pbuf, ID_Topic, &f);
            if (f.type == VT_String)
                dest = f.val.s;
        }
    }

    /*
     * Set up the header
     */
    memset(&hdr, 0, sizeof hdr);
    hdr.Persistence = (flags & 0x04) ? ismMESSAGE_PERSISTENCE_PERSISTENT : ismMESSAGE_PERSISTENCE_NONPERSISTENT;
    hdr.Reliability = flags&3;
    hdr.Priority = 4;
    hdr.RedeliveryCount = 0;
    hdr.Expiry = expiry;
    hdr.Flags = headerFlags;
    hdr.MessageType = msgtype;

    /* Set the properties and body area */
    areasize[0] = props->type == VT_Map ? props->len : 0;
    areaptr[0] = props->type == VT_Map ? props->val.s : NULL;
    areasize[1] = body->type == VT_ByteArray ? body->len : 0;
    areaptr[1] = body->type == VT_ByteArray ? body->val.s : NULL;

    /* Update read stats.  We do not need a lock as all reads are in the transport IOP thread */
    transport->pobj->channel->read_msg++;
    transport->pobj->channel->read_bytes += (body->len + props->len);

    /*
     * Create the message
     */
    rc = ism_engine_createMessage(&hdr, 2, MsgAreas, areasize, areaptr, &msgh);
    if (rc) {
        /* Engine reported error when creating message, set an error and ACK */
        fwd_msgact_t act = {0};
        act.action = action->action;
        act.seqnum = seqnum;
        act.transport = transport;
        fwdReplyPublish(rc, NULL, &act);
        return 1;
    }

    /*
     * If everything is good, send the message to the engine to process
     */
    if (fwd_sessionh && dest) {     /* BEAM suppression: constant condition */
        int destType = ismDESTINATION_TOPIC;

        /* If either reliable or we are asking for a processed response, set an engine callback */
        if (seqnum) {
            ismEngine_TransactionHandle_t htran = NULL;
            int createXA = 0;
            fwd_msgact_t act = {0};
            act.action = action->action;
            act.seqnum = seqnum;
            act.transport = transport;
            if(action->action == FwdAction_RMessage) {
                pthread_spin_lock(&pobj->sessionlock);
                if(addMessageToXA(transport, pobj->currentXA, seqnum) == fwd_commit_count)
                    createXA = 1;
                htran = pobj->currentXA->handle;
                act.xaHandle = htran;
                pthread_spin_unlock(&pobj->sessionlock);
            }
            rc = ism_engine_putMessageOnDestination(fwd_sessionh, destType , dest, htran,
                    msgh, &act, sizeof(act), fwdReplyPublish);

            if (rc != ISMRC_AsyncCompletion) {
                if (rc == ISMRC_SomeDestinationsFull ||
                    rc == ISMRC_NoMatchingDestinations ||
                    rc == ISMRC_NoMatchingLocalDestinations) {
                      rc = ISMRC_OK;
                };
                if((rc == ISMRC_OK) && createXA) {
                    fwdCreateXA(transport);
                }
                fwdReplyPublish(rc, NULL, &act);
            } else {
                if(createXA)
                    fwdCreateXA(transport);
            }
        }
        /* For an unreliable message without a processed response, just send it to engine */
        else {
            rc = ism_engine_putMessageOnDestination(fwd_sessionh,
                destType, dest, NULL, msgh, NULL, 0, NULL);
            /* As we do not ask for a response, we do not check the RC here */
            if(rc && (rc != ISMRC_AsyncCompletion)) {
                if (rc != ISMRC_SomeDestinationsFull &&
                    rc != ISMRC_NoMatchingDestinations &&
                    rc != ISMRC_NoMatchingLocalDestinations) {
                    TRACE(6, "ism_fwd_doMessage: ignore error %d returned by engine index=%u\n", rc, transport->index);
                }
            }
            rc = 0;
            /* Decrement inprogress.  If close is in progress, proceed with it */
            ipcount = __sync_sub_and_fetch(&pobj->inprogress, 1);
            TRACE(9, "Leave ism_fwd_doMessage, index=%u inprogress=%d\n", transport->index, ipcount);
            if (UNLIKELY(ipcount < 0)) { /* BEAM suppression: constant condition */
                ism_fwd_replyCloseClient(transport);
            }
        }

    } else {
        /* If close is in progress, proceed with it */
        ipcount = __sync_sub_and_fetch(&pobj->inprogress, 1);
        TRACE(9, "Leave ism_fwd_doMessage, index=%u inprogress=%d\n", transport->index, ipcount);
        if (UNLIKELY(ipcount < 0)) { /* BEAM suppression: constant condition */
           ism_fwd_replyCloseClient(transport);
        }
        return 1;
    }

    return 0;
}


/*
 * Reply to the receiver prepare actions
 */
static void replyDoPrepare(int rc, void * handle, void * vaction) {
    fwd_xa_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_fwd_channel_t * channel = transport->pobj->channel;
    fwd_xa_t * xa;
    char xbuf[512];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};

    if (rc) {
        /* Prepare failed */
        TRACE(1, "Forwarder prepare failed: xid=fmd:R:%s index=%u name=%s rc=%d\n",
                action->gtrid, transport->index, transport->name, rc);
        pthread_mutex_lock(&channel->lock);
        xa = ism_fwd_findXA(channel, action->gtrid, 0, 0);
        if (xa) {
            ism_fwd_unlinkXA(channel, xa, 0, 0);
            ism_common_free(ism_memory_protocol_misc,xa);
        }
        pthread_mutex_unlock(&channel->lock);

        int32_t ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
        TRACE(6, "Leave prepare failed, index=%d inprogress=%d\n", transport->index, ipcount);
        if (ipcount < 0) {
            ism_fwd_replyCloseClient(transport);
        } else {
            transport->close(transport, rc, 0, "Close due to prepare failure");
        }
        return;
    } else {
        /* TODO: handle out of order prepare reply */
        pthread_mutex_lock(&channel->lock);
        xa = ism_fwd_findXA(channel, action->gtrid, 0, 0);
        if (xa) {
            xa->prepared = 1;
        } else {
            TRACE(2, "Unable to find global transaction after prepare: xa=%s index=%u name=%s\n",
                    action->gtrid, transport->index, transport->name);
            pthread_mutex_unlock(&channel->lock);
            return;
        }

        pthread_mutex_unlock(&channel->lock);
        /*
         * Send a commit to the sender
         */
        ism_protocol_putStringValue(&buf, action->gtrid);
        transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_Commit<<8)+1, SFLAG_FRAMESPACE);

        /*
         * Commit our branch
         */
        TRACE(7, "Forwarder receive commit XA: name=%s index=%u gtrid=%s\n",
                 transport->clientID, transport->index, action->gtrid);
        rc = ism_engine_completeGlobalTransaction(&xa->xid, ismTRANSACTION_COMPLETION_TYPE_COMMIT,
                 action, sizeof *action, replyEngineCommit);
        if (rc != ISMRC_AsyncCompletion) {
            replyEngineCommit(rc, action, action);
        } else {
            TRACE(7, "Async engine commit: name=%s index=%u gtrid=%s\n",
                    transport->clientID, transport->index, action->gtrid);
        }
    }
}


/*
 * send prepare XA request to the sender side.
 */
static void sendPrepareXA(ism_transport_t * transport, fwd_xa_info_t * xaInfo) {
    char xbuf[10240];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    /* Send prepare to sender */
    ism_protocol_putStringValue(&buf, xaInfo->gtrid);
    ism_protocol_putIntValue(&buf, xaInfo->seqcount);
    ism_protocol_putIntValue(&buf, xaInfo->xaSequence);
    ism_protocol_putNullValue(&buf);  /* map */
    ism_protocol_putByteArrayValue(&buf, (const char*)xaInfo->seqnum, xaInfo->seqcount*sizeof(uint64_t));
    TRACEL(7, transport->trclevel, "sendPrepareXA: xid=fwd:S:%s index=%u name=%s\n",
            xaInfo->gtrid, transport->index, transport->name);
    transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_Prepare<<8)+3, SFLAG_FRAMESPACE);
    if(buf.inheap)
        ism_common_freeAllocBuffer(&buf);
    destroyXAInfo(xaInfo);
}

/*
 * Reply from create global transaction on the sender side.
 */
static void replyCreateXA(int rc, void * handle, void * vaction) {
    fwd_xatr_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;
    fwd_xa_t * xa;
    fwd_xa_info_t * xaInfo;
    fwd_xa_info_t * xaInfo2prepare;
    if (rc) {
        char xbuf[1024];
        ism_common_formatLastError(xbuf, sizeof xbuf);
        pthread_mutex_lock(&pobj->channel->lock);
        xa = ism_fwd_findXA(pobj->channel, action->gtrid, 0, 0);
        ism_fwd_unlinkXA(pobj->channel, xa, 0, 0);
        pthread_mutex_unlock(&pobj->channel->lock);
        ism_common_free(ism_memory_protocol_misc,xa);
        int ipcount = __sync_sub_and_fetch(&pobj->inprogress, 1);
        TRACE(2, "Create global transaction failed: index=%u name=%s inprogress=%d gtrid=%s rc=%u, error=%s",
                transport->index, transport->name, ipcount, action->gtrid, rc, xbuf);
        if (ipcount < 0) {
            ism_fwd_replyCloseClient(transport);
        } else {
            transport->close(transport, rc, 0, xbuf);
        }
        return;
    }
    pobj->transaction = handle;
    xaInfo = createXAInfo(action->gtrid, handle, action->sequence);
    pthread_spin_lock(&pobj->sessionlock);
    xaInfo2prepare = pobj->currentXA;
    pobj->currentXA = xaInfo;
    if(xaInfo2prepare) {
        if(xaInfo2prepare->readyMsgCounter < xaInfo2prepare->seqcount) {
            xaInfo2prepare->prev = pobj->xaListTail;
            if(pobj->xaListHead){
                pobj->xaListTail->next = xaInfo2prepare;
            } else {
                pobj->xaListHead = xaInfo2prepare;
            }
            pobj->xaListTail = xaInfo2prepare;
            pobj->xaInfoListSize++;
            xaInfo2prepare = NULL;
        }
    }
    pthread_spin_unlock(&pobj->sessionlock);

    if(xaInfo2prepare)
        sendPrepareXA(transport, xaInfo2prepare);
    int32_t ipcount = __sync_sub_and_fetch(&pobj->inprogress, 1);
    TRACE(9, "Leave reply create XA, index=%u inprogress=%d\n", transport->index, ipcount);
    if (UNLIKELY(ipcount < 0)) {
        ism_fwd_replyCloseClient(transport);
    }
}


static void fwdCreateXA(ism_transport_t * transport) {
    ism_protobj_t * pobj = transport->pobj;
    ismEngine_TransactionHandle_t transh;
    fwd_xatr_t act = {0};
    fwd_xa_t * xa;
    int ipcount = __sync_fetch_and_add(&transport->pobj->inprogress, 1);
    act.sequence = ism_fwd_newGtrid(act.gtrid, transport->pobj->channel->uid);
    act.transport = transport;
    xa = ism_fwd_makeXA(act.gtrid, 'R', act.sequence);
    ism_fwd_linkXA(pobj->channel, xa, 0, 1);
    TRACE(9, "fwdCreateXA: name=%s index=%u inprogress=%d gtrid=%s\n", transport->clientID, transport->index, ipcount, act.gtrid);
    pobj->channel->start_xa = ism_common_readTSC();
    int rc = ism_engine_createGlobalTransaction(transport->pobj->session_handle, &xa->xid,
            ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT, &transh, &act, sizeof(act), replyCreateXA);
    if (rc != ISMRC_AsyncCompletion)
        replyCreateXA(rc, transh, &act);
}

/*
 * ACK a message on the reliable sender
 */
void fwdReliableACK(fwd_msgact_t * action) {
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;
    fwd_xa_info_t * xaInfo = NULL;
    int ready = 0;
    pthread_spin_lock(&pobj->sessionlock);
    if(pobj->closing){
        pthread_spin_unlock(&pobj->sessionlock);
        return;
    }
    if(pobj->currentXA->handle == action->xaHandle) {
        xaInfo = pobj->currentXA;
        xaInfo->readyMsgCounter++;
    } else {
        int i = 0;
        for(xaInfo = pobj->xaListHead; xaInfo != NULL; xaInfo = xaInfo->next) {
            if(xaInfo->handle == action->xaHandle) {
                xaInfo->readyMsgCounter++;
                if(xaInfo->readyMsgCounter == xaInfo->seqcount) {
                    ready = 1;
                    if(xaInfo->prev)
                        xaInfo->prev->next = xaInfo->next;
                    else
                        pobj->xaListHead = xaInfo->next;
                    if(xaInfo->next)
                        xaInfo->next->prev = xaInfo->prev;
                    else
                        pobj->xaListTail = xaInfo->prev;
                    pobj->xaInfoListSize--;
                }
                break;
            }
            i++;
        }
        if(i > 5)
            TRACE(5, "!!! fwdReliableACK: name=%s index=%u xaInfo index is %d\n", transport->clientID, transport->index, i);
    }
    pthread_spin_unlock(&pobj->sessionlock);

    if(ready) {
        sendPrepareXA(transport, xaInfo);
    }

    return;
}

/*
 * Reply from a commit at recover.
 * We did a commit, but the response could have been that it was already heuristically committed
 * and in that case just forget it.  In any case remove the xa object.
 */
static void replyCommitRecover(int32_t rc, void * handle, void * vaction) {
    fwd_recover_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;
    ism_fwd_channel_t * channel = pobj->channel;
    fwd_xa_t * xa = action->xa;

    if (rc ==ISMRC_HeuristicallyCommitted  || rc == ISMRC_HeuristicallyRolledBack) {
        if(xa) {
            xUNUSED int zrc = ism_engine_forgetGlobalTransaction(&xa->xid, NULL, 0, NULL);
        } else {
            ism_xid_t xid;
            ism_fwd_makeXid(&xid, 'R', action->gtrid);
            xUNUSED int zrc = ism_engine_forgetGlobalTransaction(&xid, NULL, 0, NULL);
        }
        rc = 0;
    }
    if (rc) {
        TRACE(2, "Forwarder recovery transaction commit failure: xid=fwd:R:%s index=%u name=%s\n",
                action->gtrid, transport->index, transport->name);
        ism_common_setError(rc);
    } else {
        TRACE(8, "Forwarder complete receive side transaction: xid=fwd:R:%s index=%u name=%s\n",
                action->gtrid, transport->index, transport->name);
    }

    /*
     * Unlink the XA object
     */
    if(xa) {
        ism_fwd_unlinkXA(channel, xa, 0, 0);
        TRACE(6, "Forwarder complete transaction: xid=fwd:R:%s index=%u name=%s\n",
                        action->gtrid, transport->index, transport->name);
        ism_common_free(ism_memory_protocol_misc,xa);
    } else {
        pthread_mutex_lock(&channel->lock);
        xa = ism_fwd_findXA(channel, action->gtrid, 0, 0);
        if (xa) {
            ism_fwd_unlinkXA(channel, xa, 0, 0);
            TRACE(6, "Forwarder complete transaction: xid=fwd:R:%s index=%u name=%s\n",
                            action->gtrid, transport->index, transport->name);
            ism_common_free(ism_memory_protocol_misc,xa);
        } else {
            TRACE(4, "Forwarder recover commit reply transaction not found: xid=fwd:R:%s index=%u name=%s\n",
                   action->gtrid, transport->index, transport->name);
        }
        pthread_mutex_unlock(&channel->lock);
    }
}

/*
 * Reply to the recover.
 * Tell the sender to commit all prepared transactions, and commit any of our own
 * prepared transactions, then send a Start to the sender.
 */
static void replyRecover(int32_t rc, void * handle, void * vaction) {
    fwd_recover_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;
    ism_fwd_channel_t * channel = pobj->channel;
    fwd_xa_t * xa;
    fwd_recover_action_t comact = {0};
    ismEngine_TransactionHandle_t transh = NULL;
    char xbuf [256];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    int cmd;

    switch (action->action) {
    case Action_prepareGlobalTransaction:
        if (action->xa) {
            cmd = FwdAction_CommitRecover;
            TRACEL(7, transport->trclevel, "Forwarder commit sender branch: xid=fwd:S:%s index=%u name=%s\n",
                    action->gtrid, transport->index, transport->name);
        } else {
            cmd = FwdAction_RollRecover;
            TRACEL(7, transport->trclevel, "Forwarder rollback sender branch: xid=fwd:S:%s index=%u name=%s\n",
                    action->gtrid, transport->index, transport->name);
        }

        /*
         * Send a commit for all sender prepared transactions
         */
        ism_protocol_putStringValue(&buf, action->gtrid);

        transport->send(transport, buf.buf+6, buf.used-6, (cmd<<8)+1, SFLAG_FRAMESPACE);
        if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
            ism_fwd_replyCloseClient(transport);
        }
        return;

    case Action_commitGlobalTransaction:
        comact.transport = transport;
        pthread_mutex_lock(&channel->lock);
        xa = channel->receiver_xa;
        while (xa) {
            if(xa->prepared) {
                TRACE(5, "Forwarder receive recover commit XA: name=%s index=%u gtrid=%s\n",
                        transport->clientID, transport->index, xa->gtrid);
                strcpy(comact.gtrid, xa->gtrid);
                comact.xa = NULL;
                rc = ism_engine_commitGlobalTransaction(transport->pobj->session_handle, &xa->xid, 0,
                        &comact, sizeof comact, replyCommitRecover);
                comact.xa = xa;
                xa = xa->next;
                if(rc != ISMRC_AsyncCompletion)
                    replyCommitRecover(rc, NULL, &comact);
            } else {
                fwd_xa_t * nextXA = xa->next;
                TRACE(5, "replyRecover - ignore unprepared XA: name=%s index=%u gtrid=%s\n",
                        transport->clientID, transport->index, xa->gtrid);
                ism_fwd_unlinkXA(channel, xa, 0, 0);
                ism_common_free(ism_memory_protocol_misc,xa);
                xa = nextXA;
            }
        }
        pthread_mutex_unlock(&channel->lock);
        char gtrid[64];
        uint64_t sequence = ism_fwd_newGtrid(gtrid, pobj->channel->uid);
        xa = ism_fwd_makeXA(gtrid, 'R', sequence);
        ism_fwd_linkXA(pobj->channel, xa, 0, 1);
        fwd_xa_info_t * xaInfo = createXAInfo(gtrid, NULL, sequence);
        pthread_spin_lock(&pobj->sessionlock);
        if(pobj->currentXA)
            destroyXAInfo(pobj->currentXA);
        pobj->currentXA = xaInfo;
        pthread_spin_unlock(&pobj->sessionlock);
        action->action = Action_reply;
        pobj->channel->start_xa = ism_common_readTSC();
        rc = ism_engine_createGlobalTransaction(transport->pobj->session_handle, &xa->xid,
                ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT,
                &transh, action, sizeof *action, replyRecover);
        if (rc == ISMRC_AsyncCompletion)
            break;
        handle = transh;
        /* fall thru */

    case Action_reply:
        /* Send start to sender */
        if (rc) {
            pthread_mutex_lock(&pobj->channel->lock);
            xa = ism_fwd_findXA(pobj->channel, pobj->currentXA->gtrid, 0, 0);
            ism_fwd_unlinkXA(pobj->channel, xa, 0, 0);
            destroyXAInfo(pobj->currentXA);
            pobj->currentXA = NULL;
            pthread_mutex_unlock(&pobj->channel->lock);
            ism_common_free(ism_memory_protocol_misc,xa);
            ism_common_setError(rc);
            if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
                ism_fwd_replyCloseClient(transport);
            } else {
                transport->close(transport, rc, 0, "Unable to create global transaction");
            }
            return;
        } else {
            if(handle) {
                pobj->currentXA->handle = handle;
                pobj->transaction = handle;
            }
            TRACEL(7, transport->trclevel, "Forwarder start: name=%s index=%u\n",
                    transport->name, transport->index);
            transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_Start<<8)+0, SFLAG_FRAMESPACE);

            int32_t ipcount = __sync_sub_and_fetch(&pobj->inprogress, 1);
            TRACE(9, "Leave reply recover, index=%u inprogress=%d\n", transport->index, ipcount);
            if (UNLIKELY(ipcount < 0)) {
                ism_fwd_replyCloseClient(transport);
            }
        }
    }
}


/*
 * Handle a recover action.
 * Recover is sent from sender to receiver with a list of prepared transactions.
 */
int ism_fwd_doRecover(ism_fwd_act_t * xaction, const char * gtrid) {
    fwd_recover_action_t action = {0};
    ism_transport_t * transport = xaction->transport;

    if(gtrid && gtrid[0]) {
        TRACE(5, "ism_fwd_doRecover: name=%s index=%u xid=fwd:S:%s\n",
                transport->clientID, transport->index, gtrid);
    } else {
        TRACE(5, "ism_fwd_doRecover - no more sender transactions: name=%s index=%u\n",
                transport->clientID, transport->index);
    }


    /*
     * Act on the next transaction to recover
     */
    action.transport = transport;
    if (!gtrid || !*gtrid) {
        /*
         * All transactions prepared on the the sender are done, process
         * any known only on the receiver side.
         */
        action.action = Action_commitGlobalTransaction;
        replyRecover(0, NULL, &action);
    } else {
        ism_protobj_t * pobj = transport->pobj;
        fwd_xa_t * xa = ism_fwd_findXA(pobj->channel, gtrid, 0, 1);
        /* Keep the highest transaction ID so we do not reuse it */
        const char * pos = strchr(gtrid, '_');
        if (pos) {
            pos = strchr(pos+1, '_');
            if (pos) {
                int seq = strtoul(pos+1, NULL, 10);
                if (seq > fwd_xid_seqn)
                    fwd_xid_seqn = seq+1;
            }
        }

        action.action = Action_prepareGlobalTransaction;
        if(xa && xa->prepared)
            action.xa = xa;
        strcpy(action.gtrid, gtrid);
        replyRecover(0, NULL, &action);
    }
    return 0;
}


/*
 * Reply from the commit of the global transaction at the receiver end.
 */
static void replyEngineCommit(int32_t rc, void * handle, void * vaction) {
    fwd_xa_action_t * action = (fwd_xa_action_t *)vaction;
    ism_transport_t * transport = action->transport;
    ism_fwd_channel_t * channel = transport->pobj->channel;
    fwd_xa_t * xa;
    int forget = 0;

    if (rc == ISMRC_HeuristicallyCommitted)
        rc = 0;
    if (rc) {
        /* TODO: log a commit failure */
        TRACE(2, "Forwarder transaction commit failure: xid=fwd:R:%s index=%u name=%s\n",
                action->gtrid, transport->index, transport->name);
        /* TODO: what else here */
    } else {
        TRACE(8, "Forwarder complete receive side transaction: xid=fwd:R:%s index=%u name=%s\n",
                action->gtrid, transport->index, transport->name);
    }

    pthread_mutex_lock(&channel->lock);
    xa = ism_fwd_findXA(channel, action->gtrid, 0, 0);
    if (xa) {
        if (++xa->commit > 1) {
            ism_fwd_unlinkXA(channel, xa, 0, 0);
            forget = 1;
        }
    } else {
        TRACE(4, "Forwarder commit reply transaction not found: xid=fwd:R:%s index=%u name=%s\n",
            action->gtrid, transport->index, transport->name);
    }
    pthread_mutex_unlock(&channel->lock);
    if (forget) {
        xUNUSED int zrc = ism_engine_forgetGlobalTransaction(&xa->xid, NULL, 0, NULL);
        TRACE(6, "Forwarder complete transaction: xid=fwd:R:%s index=%u name=%s\n",
                action->gtrid, transport->index, transport->name);
        ism_common_free(ism_memory_protocol_misc,xa);
    }
    int32_t ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
    TRACE(9, "Leave reply engine commit, index=%u inprogress=%d\n", transport->index, ipcount);
    if (UNLIKELY(ipcount < 0)) {
        ism_fwd_replyCloseClient(transport);
    }
}


/*
 * Handle the reply to a prepare (receiver).
 * Mark this transaction prepared, and commit any leading prepared.
 */
int ism_fwd_doPrepareReply(ism_fwd_act_t * action, const char * gtrid, int rc) {
    ism_transport_t * transport = action->transport;
    ism_fwd_channel_t * channel = transport->pobj->channel;
    ism_xid_t xid;
    fwd_xa_action_t act = {0};

    if (rc) {
        TRACE(1, "Forwarder prepare failed on sender side: xid=fmd:S:%s index=%u name=%s rc=%d\n",
                gtrid, transport->index, transport->name, rc);
        pthread_mutex_lock(&channel->lock);
        fwd_xa_t * xa = ism_fwd_findXA(channel, gtrid, 0, 0);
        if (xa) {
            ism_fwd_unlinkXA(channel, xa, 0, 0);
            ism_common_free(ism_memory_protocol_misc,xa);
        }
        pthread_mutex_unlock(&channel->lock);
        int32_t ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
        TRACE(6, "Leave prepare failed, index=%d inprogress=%d\n", transport->index, ipcount);
        if (ipcount < 0) {
            ism_fwd_replyCloseClient(transport);
        } else {
            transport->close(transport, rc, 0, "Closed due to a sender prepare failure");
        }
        return 1;
    }
    act.transport = transport;
    strcpy(act.gtrid, gtrid);
    ism_fwd_makeXid(&xid, 'R', gtrid);
    TRACE(8, "Prepare XA: name=%s index=%u gtrid=%s\n",
            transport->clientID, transport->index, gtrid);
    rc = ism_engine_prepareGlobalTransaction(transport->pobj->session_handle, &xid,
        &act, sizeof act, replyDoPrepare);
    if (rc != ISMRC_AsyncCompletion)
        replyDoPrepare(rc, NULL, &act);
    return 0;
}


/*
 * Handle the reply to a commit (receiver).
 * This is the commit reply from the server side.
 */
int ism_fwd_doCommitReply(ism_fwd_act_t * action, const char * gtrid, int rc) {
    ism_transport_t * transport = action->transport;
    ism_fwd_channel_t * channel = transport->pobj->channel;
    fwd_xa_t * xa;
    int forget = 0;

    if (rc ==ISMRC_HeuristicallyCommitted)
        rc = 0;
    if (rc) {
        /* TODO: log a commit failure */
        TRACE(2, "Forwarder sender transaction commit failure: xid=fwd:S:%s index=%u name=%s\n",
                gtrid, transport->index, transport->name);
    } else {
        TRACE(8, "Forwarder complete sender side transaction: xid=fwd:S:%s index=%u name=%s\n",
                gtrid, transport->index, transport->name);

    }
    pthread_mutex_lock(&channel->lock);
    xa = ism_fwd_findXA(channel, gtrid, 0, 0);
    if (xa) {
        if (++xa->commit > 1) {
            ism_fwd_unlinkXA(channel, xa, 0, 0);
            forget = 1;
        }
    } else {
        TRACE(4, "Forwarder commit reply transaction not found: xid=fwd:R:%s index=%u name=%s\n",
            gtrid, transport->index, transport->name);
    }
    pthread_mutex_unlock(&channel->lock);
    if (forget) {
        xUNUSED int zrc = ism_engine_forgetGlobalTransaction(&xa->xid, NULL, 0, NULL);
        ism_common_free(ism_memory_protocol_misc,xa);
        TRACE(6, "Forwarder complete transaction: xid=fwd:R:%s index=%u name=%s\n",
                gtrid, transport->index, transport->name);
    }
    return 0;
}


/*
 * Link a transaction to the correct channel.
 * This is used during recovery.
 * We are called with the three parts of the
 */
void linkTransaction(const char * sender, const char * receiver, int sequence) {
    ism_fwd_channel_t * channel;
    fwd_xa_t * xa = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,247),1, sizeof(fwd_xa_t));
    xa->sequence = sequence;
    xa->prepared = 1;         /* All recovered transactions are prepared */
    sprintf(xa->gtrid, "%s_%s_%u", sender, receiver, sequence);
    if (strcmp(sender, ism_common_getServerUID())) {
        channel = ism_fwd_findChannel(sender);
        if (!channel) {
            channel = ism_fwd_newChannel(sender, NULL);
        }
        ism_fwd_makeXid(&xa->xid, 'R', xa->gtrid);
        ism_fwd_linkXA(channel, xa, 0, 1);
        // printf("channel=%s receiver xa=%s\n", sender, xa->gtrid);
    } else {
        channel = ism_fwd_findChannel(receiver);
        if (!channel) {
            channel = ism_fwd_newChannel(receiver, NULL);
        }
        ism_fwd_makeXid(&xa->xid, 'S', xa->gtrid);
        ism_fwd_linkXA(channel, xa, 1, 1);
        // printf("channel=%s sender xa=%s\n", sender, xa->gtrid);
    }
}

extern int ism_fwd_commit_outstanding;

/*
 * Return from a local commit
 */
static void replyRecoverCommit(int rc, void * handle, void * vxa) {
    fwd_xa_t * xa = (fwd_xa_t *)vxa;
    int  count;

    count = __sync_sub_and_fetch(&ism_fwd_commit_outstanding, 1);
    xa->commit = 1;
    TRACE(4, "Completed commit at system recover: XID=fwd:R:%s count=%u\n", xa->gtrid, count);
}

extern ism_fwd_channel_t * fwd_channelList;

/*
 * Called at start messaging to recover global transactions.
 *
 * This time we heuristically commit any receiver transactions.
 */
int ism_fwd_recoverTransactions(void){
    /* Get a list of all XIDs from the engine */
    ism_xid_t xidlist[20];
    char gtrid[64];
    char xbuf[256];
    const char * serverUID = ism_common_getServerUID();
    char * receiver;
    int i;
    int maxinst = -1;
    int sequence;
    int count = 20;

    TRACE(8, "RecoverTransactions\n");
    pthread_mutex_lock(&fwd_configLock);
    while (count == 20) {
        count = ism_engine_XARecover(fwd_sessionh, xidlist, 20, 0, 0);
        TRACE(8, "RecoverTransaction: count=%u\n", count);
        for (i=0; i<count; i++) {
            if (xidlist[i].formatID == ISM_FWD_XID) {
                sequence = 0;
                memcpy(gtrid, xidlist[i].data+xidlist[i].bqual_length, xidlist[i].gtrid_length);
                gtrid[xidlist[i].gtrid_length] = 0;
                TRACE(8, "Recover transaction: %s\n", gtrid);
                char * pos = strchr(gtrid, '_');
                if (pos) {
                    *pos++ = 0;
                    receiver = pos;
                    pos = strchr(receiver, '_');
                    if (pos) {
                        *pos++ = 0;
                        sequence = atoi(pos);
                        if (!strcmp(receiver, serverUID)) {
                            if (sequence > maxinst)
                                maxinst = sequence;
                        } else if (strcmp(gtrid, serverUID)) {
                            sequence = 0;
                        }
                    }

                }
                if (sequence > 0) {
                    TRACE(4, "Recover forwarder transaction: %s\n", ism_common_xidToString(xidlist+i, xbuf, sizeof xbuf));
                    linkTransaction(gtrid, receiver, sequence);
                } else {
                    TRACE(1, "A XID was found with a forwarder formatID but with an incorrect gtrid: %s\n",
                            ism_common_xidToString(xidlist+i, xbuf, sizeof xbuf));
                }

            }
        }
    }
    if (maxinst >= 0)
        fwd_xid_seqn = maxinst+200-maxinst%100;

    ism_fwd_channel_t * channel;
    struct fwd_xa_t * xa;

    channel = fwd_channelList;
    while (channel) {
        xa = channel->receiver_xa;
        while (xa) {
            int  rc;

            /* Heuristic commit */
            TRACE(4, "Commit receiver transaction at system recover: XID=fwd:R:%s count=%u\n", xa->gtrid, count);
            rc = ism_engine_completeGlobalTransaction(&xa->xid, ismTRANSACTION_COMPLETION_TYPE_COMMIT,
                xa, sizeof *xa, replyRecoverCommit);
            if (rc != ISMRC_AsyncCompletion) {
                replyRecoverCommit(rc, NULL, xa);
            } else {
                TRACE(8, "Async engine heuristic commit at recover: gtrid=%s\n", xa->gtrid);
            }
            xa = xa->next;
        }
        channel = channel->next;
    }
    pthread_mutex_unlock(&fwd_configLock);
    return 0;
}


/*
 * Make a XID.
 * This does not checking as it is internal.
 */
void ism_fwd_makeXid(ism_xid_t * xid, char branch, const char * gtrid) {
    xid->formatID = ISM_FWD_XID;
    xid->bqual_length = 1;
    xid->gtrid_length = strlen(gtrid);
    xid->data[0] = branch;
    memcpy(&xid->data[1], gtrid, xid->gtrid_length);
    xid->data[xid->gtrid_length + 1] = 0;
}


/*
 * Make a new xa object.
 *
 * This can only be done on the coordinator side which is the receive side
 * in the forwarder.
 * @param sender  The sender UID
 * @return  A xa which is filled in with the gtrid or NULL to indicate an error
 */
int ism_fwd_xa_init(fwd_xa_t * xa, const char * uid) {
    if (!ism_common_validServerUID(uid)) {
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "ism_fwd_newXid");
        return ISMRC_ArgNotValid;
    }
    memset(xa, 0, sizeof(fwd_xa_t));
    xa->sequence = __sync_add_and_fetch(&fwd_xid_seqn, 1);
    sprintf(xa->gtrid, "%s_%s_%lu", uid, ism_common_getServerUID(), xa->sequence);
    ism_fwd_makeXid(&xa->xid, 'R', xa->gtrid);
    return ISMRC_OK;
}


/*
 * Make an xa object from a gtrid
 */
fwd_xa_t * ism_fwd_makeXA(const char * gtrid, char branch, uint64_t sequence) {
    fwd_xa_t * xa = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,248),1, sizeof(fwd_xa_t));
    xa->sequence = sequence;
    strcpy(xa->gtrid, gtrid);
    ism_fwd_makeXid(&xa->xid, branch, gtrid);
    return xa;
}


/*
 * Construct a new global transaction ID
 */
uint64_t ism_fwd_newGtrid(char * gtrid, const char * sender) {
    uint64_t  ret;
    if (!ism_common_validServerUID(sender)) {
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "ism_fwd_newGtrid");
        return 0;
    }
    ret = __sync_add_and_fetch(&fwd_xid_seqn, 1);
    if(snprintf(gtrid, 64, "%s_%s_%lu", sender, ism_common_getServerUID(), ret) >= 64) {
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "ism_fwd_newGtrid");
        return 0;
    }
    return ret;
}

/*
 * Commit a transaction based on time
 */
int ism_fwd_timedCommit(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_fwd_channel_t * channel = (ism_fwd_channel_t *) userdata;

	ism_common_cancelTimer(key);
    pthread_mutex_lock(&channel->lock);
    if (channel->in_state == CHST_Open) {
        ism_transport_t * transport = channel->in_channel;
        ism_protobj_t * pobj = transport->pobj;
        int ipcount = __sync_fetch_and_add(&transport->pobj->inprogress, 1);
        pthread_mutex_unlock(&channel->lock);
        if(ipcount < 0) {
            __sync_fetch_and_sub(&transport->pobj->inprogress, 1);
            return 0;
        }
        pthread_spin_lock(&pobj->sessionlock);
        if (pobj->currentXA && pobj->currentXA->seqcount) {
            pthread_spin_unlock(&pobj->sessionlock);
            fwdCreateXA(transport);
        } else {
            pthread_spin_unlock(&pobj->sessionlock);
        }
        ipcount = __sync_sub_and_fetch(&pobj->inprogress, 1);
        TRACE(9, "Leaveism_fwd_timedCommit, index=%u inprogress=%d\n", transport->index, ipcount);
        if (UNLIKELY(ipcount < 0)) {
            ism_fwd_replyCloseClient(transport);
        }
    }
    pthread_mutex_unlock(&channel->lock);
    return 0;
}

