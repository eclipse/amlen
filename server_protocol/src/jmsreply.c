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

/* @file jmsreply.c
 * This file contains the various reply methods.  These methods are callbacks from the engine
 * which are sometimes called directly from the protocol.
 */

/*
 * Reply from a authorization request
 */
static int jmsReplyAuthTT(ism_timer_t timer, ism_time_t timestamp, void * callbackParam) {
    ism_protocol_action_t * action = (ism_protocol_action_t *)callbackParam;
    ism_transport_t * transport = action->transport;
    ismEngine_ClientStateHandle_t clienth;
    int rc = action->rc;

    if (timer)
        ism_common_cancelTimer(timer);

    if (rc != ISMRC_OK) {
        /*
         * Authentication failed
         */
        ism_common_setError(rc);
        /* Failed to authenticate. Reply the error code*/
        replyAction(rc, NULL, action);
        if (timer)
            ism_common_free(ism_memory_protocol_misc,action);
        return 0;
    } else {
        TRACEL(7, transport->trclevel, "User is authenticated and authorized: connect=%u client=%s user=%s\n",
            transport->index, transport->name, transport->userid);
    }

    /* Set max TCP buffer based on expected message rate */
    ism_protocol_setSocketBuffer(transport);

    /*
     * Create the engine client state
     */
    rc = ism_engine_createClientState(transport->clientID, PROTOCOL_ID_JMS,
            ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE, NULL, NULL,
            transport->security_context, &clienth,
            action, action->actionsize, replyAction);

    if (rc == ISMRC_ClientIDInUse) {
        replyAction(rc, NULL, action);
        if (timer)
            ism_common_free(ism_memory_protocol_misc,action);
        return 0;
    }

    if (rc != ISMRC_AsyncCompletion) {
        if (rc != ISMRC_ResumedClientState) {
            ism_common_setError(rc);
        }
        replyAction(rc, clienth, action);
    }
    if (timer)
        ism_common_free(ism_memory_protocol_misc,action);
    return 0;
}

static void jmsReplyAuth(int rc, void * callbackParam) {
    if (__builtin_expect(jms_unit_test,0)){
        /* CUNIT */
        ism_protocol_action_t * action = (ism_protocol_action_t *)callbackParam;
        action->rc = rc;
        jmsReplyAuthTT(NULL,0,action);
    } else {
        size_t size = ((ism_protocol_action_t *)callbackParam)->actionsize;
        ism_protocol_action_t * action = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,138),size);
        memcpy(action,callbackParam,size);
        action->rc = rc;
        ism_common_setTimerOnce(ISM_TIMER_HIGH, jmsReplyAuthTT, action,1);
    }
}

/*
 * Handles reply for Action_createConnection
 */
static inline void handleReplyCreateConnection(ism_protocol_action_t * action, void * handle) {
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    const char * cvttype;
    char versionstr[16];

    resetAction(action);
    pobj->handle = handle;
    pobj->sessions_alloc = 128;
    pobj->sessions = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,139),128, sizeof(ism_jms_session_t*));
    pobj->prodcons_alloc = 128;
    pobj->prodcons = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,140),128, sizeof(ism_jms_prodcons_t*));
    pobj->convertType = CT_Auto;
    cvttype = getproperty(action, "ConvertMessageType");
    if (cvttype) {
        int32_t ct = ism_common_enumValue(enum_cvttype, cvttype);
        if (ct != INVALID_ENUM)
            pobj->convertType = (uint8_t) ct;
    }

    TRACEL(7, transport->trclevel, "Create JMS connection: connect=%u client=%s client_version=%s client_build=%s\n", transport->index, transport->name,
            showVersion(pobj->client_version, versionstr), pobj->client_build_id);
    ism_transport_connectionReady(transport);

}

/*
 * Handles reply for Action_createSession
 */
static inline int handleReplyCreateSession(ism_protocol_action_t * action, void * handle, concat_alloc_t *buf) {
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    int id = 0;
    ism_jms_session_t * session = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,141),1,sizeof(ism_jms_session_t));
    resetAction(action);
    if (session) {
        int noack = getbooleanproperty(action, "DisableACK");

        session->handle = handle;
        pthread_spin_init(&(session->lock), 0);
        session->transacted = (action->valcount > 1) ? action->values[1].val.i : 0;
        if (!session->transacted && !noack) {
            session->ackmode = (action->valcount > 2) ? action->values[2].val.i : 3;
        } else {
            session->ackmode = 0;
        }
        session->domain = (action->valcount > 0) ? action->values[0].val.i : ismDESTINATION_QUEUE;
        session->suspended  = SUSPENDED_BY_PROTOCOL;
        session->seqnum = 1;
        id = setSession(transport, session);
    }
    if (!id) {
        int rc;
        action->rc = ISMRC_AllocateError;
        ism_common_setError(ISMRC_AllocateError);
        if (session) {
            pthread_spin_destroy(&session->lock);
            ism_common_free(ism_memory_protocol_misc,session);
        }
        rc = ism_engine_destroySession(handle, action, sizeof(ism_protocol_action_t), replyError);
        if (rc != ISMRC_AsyncCompletion) {
            if (rc) {
                ism_common_setError(rc);
            }
            replyError(rc, NULL, action);
        }
        return 1;
    }
    ism_protocol_putIntValue(buf, id);
    TRACEL(7, transport->trclevel, "Create JMS session: connect=%u client=%s session=%d\n",
            transport->index, transport->name, id);
    action->hdr.hdrcount++;
    if (pobj->started) {
        __sync_add_and_fetch(&pobj->inprogress, 1);
        if (transport->addwork) {
            TRACEL(7, transport->trclevel, "Submitting job request to resume session: connect=%u session=%d\n", transport->index, session->which);
            transport->addwork(transport, resumeSessionDelivery, (void *)(uintptr_t)session->which);
        } else {
            /* for CUNIT */
            resumeSessionDelivery(transport, (void *)(uintptr_t)session->which, 0);
        }
    }

    return 0;
}

static void replyCloseConsumerAction(int32_t rc, void * handle, void * vaction) {
	ism_protocol_action_t * action = (ism_protocol_action_t *) vaction;
    ism_transport_t * transport = action->transport;
    ism_jms_session_t * session = action->session;
    ism_jms_prodcons_t * consumer = action->prodcons;

    TRACEL(7, transport->trclevel, "handleReplyCloseConsumer: Close JMS consumer connect=%u client=%s id=%d name=%s domain=%s\n",
            transport->index, transport->name, consumer->which, consumer->name, domainName(consumer->domain));
    if (session->ackmode != 2) {
        if(clearConsumerUndeliveredMessage(session, consumer, 0, 1, action, replyCloseConsumerAction) == ISMRC_AsyncCompletion)
        	return;
    }
    if (consumer) {
        if (consumer->shared == SHARED_GlobalND) {
            if (consumer->subName) {
                checkSubscriptionConsumer(consumer->subName, transport_SharedND);
            }
        }
        if (consumer->shared == SHARED_NonDurable) {
            if (consumer->subName) {
                checkSubscriptionConsumer(consumer->subName, transport);
            }
        }
    }
}

/*
 * Handles reply for Action_closeConsumer
 */
static inline void handleReplyCloseConsumer(ism_protocol_action_t * action) {
    ism_transport_t * transport = action->transport;
    ism_jms_prodcons_t * consumer = action->prodcons;

    TRACEL(7, transport->trclevel, "handleReplyCloseConsumer: Close JMS consumer connect=%u client=%s id=%d name=%s domain=%s\n",
            transport->index, transport->name, consumer->which, consumer->name, domainName(consumer->domain));
    replyCloseConsumerAction(0,NULL,action);
}

/*
 * Handles reply for Action_createProducer
 */
static inline int handleReplyCreateProducer(ism_protocol_action_t * action, void * handle, concat_alloc_t  * buf) {
    ism_transport_t * transport = action->transport;
    int id = 0;
    ism_jms_prodcons_t * producer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,143),1,sizeof(ism_jms_prodcons_t));
    if (producer) {
        producer->handle = handle;
        producer->domain = action->values[0].val.i;
        producer->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),getproperty(action, "Name"));
        producer->hdr.itemtype = ITEMT_Producer;
        producer->hdr.item = endian_int32(action->hdr.item);
        pthread_spin_init(&producer->lock,0);
        id = setProdcons(transport, producer);
        producer->which = id;
    }
    if (id <= 0) {
        int rc;
        if (producer) {
            if (producer->name)
                ism_common_free(ism_memory_protocol_misc,producer->name);
            ism_common_free(ism_memory_protocol_misc,producer);
        }

        action->rc = ISMRC_AllocateError;
        rc = ism_engine_destroyProducer(handle, action, sizeof(ism_protocol_action_t), replyError);
        if (rc != ISMRC_AsyncCompletion) {
            if (rc) {
                ism_common_setError(rc);
            }
            replyError(rc, NULL, action);
        }
        return 1;
    }
    ism_protocol_putIntValue(buf, id);
    TRACEL(7, transport->trclevel, "Create JMS producer connect=%u client=%s id=%d name=%s domain=%s handle=%p\n",
            transport->index, transport->name, id, producer->name, domainName(producer->domain), producer->handle);
    action->hdr.hdrcount++;    /* The second header contains producer id */
    return 0;
}

/*
 * Handles reply for Action_unsubscribeDurable
 */
static inline void handleReplyUnsubscribeDurable(ism_protocol_action_t * action) {
    const char * subName = (action->valcount > 0) ? action->values[0].val.s : NULL;
    if (subName) {
        ism_transport_t * transport = action->transport;
        ism_jms_session_t * session = action->session;
        ism_undelivered_message_t * undelMsg;
        TRACEL(7, transport->trclevel, "Unsubscribe from durable JMS subscription: connect=%u client=%s "
                 "durable=%s handle=%p\n",
                 transport->index, transport->name, subName, action->session->handle);

        /* Free all delivery handles that belong to this subscription */
        pthread_spin_lock(&session->lock);
        undelMsg = session->incompMsgHead;
        while (undelMsg) {
            ism_undelivered_message_t * msg = undelMsg;
            undelMsg = msg->next;
            if (msg->consumer && msg->subName &&
                !strcmp(msg->subName, subName)) {
                freeUndeliveredMessage(session, msg);
            }
        }
        pthread_spin_unlock(&session->lock);
    }
}

/*
 * Handles reply for Action_recover
 */
static inline void handleReplyRecover(int32_t rc, void * handle, void * vaction) {
	ism_protocol_action_t * action = (ism_protocol_action_t *) vaction;
    ism_jms_session_t * session = action->session;
    if ((rc == ISMRC_OK) && (!(__sync_fetch_and_or(&session->suspended, SUSPENDED_BY_PROTOCOL)))) {
        rc = ism_engine_stopMessageDelivery(session->handle,
                action, sizeof(ism_protocol_action_t), handleReplyRecover);
    }
    if(rc == ISMRC_AsyncCompletion)
            return;
    action->recordCount = 0;
    if (rc == ISMRC_OK && session->ackmode) {
        ismEngine_DeliveryHandle_t array[1024];
        ismEngine_DeliveryHandle_t * msgs2free = array;
        int counter = 0;
        int size = 1024;
        ism_transport_t * transport = action->transport;
        ism_undelivered_message_t * undelMsg;
        TRACEL(7, transport->trclevel, "Recover JMS session: connect=%u session=%d client=%s firstID=%lu\n", transport->index, session->which, transport->name, action->recordCount);
        pthread_spin_lock(&session->lock);
        undelMsg = session->incompMsgHead;
        while (undelMsg) {
            ism_undelivered_message_t * msg = undelMsg;
            undelMsg = msg->next;
            /* Undelivered messages need to be NACKed */
            if (msg->deliveryHandle) {
                TRACEL(9, transport->trclevel, "Mark as undelivered: connect=%u session=%d seq=%lu\n", transport->index, session->which, msg->msgID);
            	if(UNLIKELY(counter == size)) {
            		size = (size << 1);
            		if( msgs2free == array) {
            			msgs2free = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,146),sizeof(ismEngine_DeliveryHandle_t)*size);
            			memcpy(msgs2free,array,sizeof(array));
            		} else {
            			msgs2free = ism_common_realloc(ISM_MEM_PROBE(ism_memory_protocol_misc,147),msgs2free,(sizeof(ismEngine_DeliveryHandle_t)*size));
            		}
            	}
                msgs2free[counter++] = msg->deliveryHandle;
            }
            action->recordCount = msg->msgID;
            freeUndeliveredMessage(session, msg);
        }
        pthread_spin_unlock(&session->lock);
        if (counter) {
            rc = ism_engine_confirmMessageDeliveryBatch(session->handle,NULL,counter,msgs2free,
            		ismENGINE_CONFIRM_OPTION_NOT_DELIVERED, action, action->actionsize, replyAction);
            if(rc == ISMRC_AsyncCompletion)
            	return;
        }
    }
    replyAction(rc, NULL, action);
}


static void cleanupJmsClientState(ism_transport_t * transport) {
	int i;
	jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    /*
     * If any of the prodcons objects are shared consumers, decrement the count of the
     */
    pthread_spin_lock(&(pobj->lock));
    for (i=0; i<pobj->prodcons_alloc; i++) {
        ism_jms_prodcons_t * consumer = pobj->prodcons[i];
        if (consumer) {
            if (consumer->shared == SHARED_GlobalND) {
                consumer->shared = 0;
                if (consumer->subName) {
                    checkSubscriptionConsumer(consumer->subName, transport_SharedND);
                }
            }
        }
    }

    /*
     * All sessions, producers, and consumers engine objects were deleted with the client state
     */

    pobj->prodcons_used = 0;
    pobj->sessions_used = 0;
    pobj->started       = 0;
    pobj->handle = NULL;

    /*
     * Return the auth token if it exists
     */
    if (transport->security_context) {
        ism_security_returnAuthHandle(transport->security_context);
        transport->security_context = NULL;
    }

    /*
     * Free up the sessions and prodcons
     */
    for (i = 0; i < pobj->sessions_alloc; i++) {
        if (pobj->sessions[i]) {
            freeUndeliveredMessages(pobj->sessions[i]);
            ism_common_free(ism_memory_protocol_misc,pobj->sessions[i]);
        }
    }
    ism_common_free(ism_memory_protocol_misc,pobj->sessions);
    pobj->sessions = NULL;
    pobj->sessions_alloc = 0;

    for (i = 0; i < pobj->prodcons_alloc; i++) {
        if (pobj->prodcons[i])
            freeConsumer(pobj->prodcons[i]);
    }
    ism_common_free(ism_memory_protocol_misc,pobj->prodcons);
    pobj->prodcons = NULL;
    pobj->prodcons_alloc = 0;

    /* Free error hashmap */
    if (pobj->errors) {
        ism_common_destroyHashMap(pobj->errors);
    }
    pobj->errors = NULL;
    pthread_spin_unlock(&(pobj->lock));

}


/*
 * Reply to an action
 */
static void replyStartConnection(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    if(__sync_sub_and_fetch(&action->recordCount,1) > 0)
    	return;
    if (rc) {
        replyAction(rc, NULL, action);
        return;
    }
    pthread_spin_lock(&(pobj->lock));
    pobj->started = 1;
    pthread_spin_unlock(&(pobj->lock));
    replyAction(rc, NULL, action);
    __sync_add_and_fetch(&pobj->inprogress, 1);
    if (transport->addwork) {
        TRACEL(8, transport->trclevel, "Submitting job request to start connection: connect=%u client=%s\n", transport->index, transport->name);
        ism_transport_submitAsyncJobRequest(transport, resumeConnectionDelivery, NULL,0);
    } else {
        /* for CUnit */
        resumeConnectionDelivery(transport, NULL,0);
    }

}
static void handleStartConnection(ism_protocol_action_t * action, concat_alloc_t * map) {
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    int rc = ISMRC_OK;
    /* Flow control - resume connection (== start, if here for the first time) */
    if (action->valcount) {
        /* This is a resume connection case. We should mark all messages that were dropped
         * during the stop as not delivered
         */
        int i;
        uint64_t sqns[256];
        uint32_t count = action->values[0].val.i;
        uint64_t * acksqn = getAckSqn(pobj, map,count,sqns,256,NULL,&rc);
        action->recordCount = 1;
        if(LIKELY(acksqn != NULL)) {
            for (i = 0; i < count; i++) {
                ism_jms_prodcons_t * consumer = getProdcons(transport,sqns[i++]);
                if (consumer) {
                    if(clearConsumerUndeliveredMessage(consumer->session, consumer,acksqn[i],0, action, replyStartConnection) == ISMRC_AsyncCompletion)
                    	__sync_add_and_fetch(&action->recordCount,1);
                }
            }
            if (__builtin_expect((acksqn != sqns),0)) {
                ism_common_free(ism_memory_protocol_misc,acksqn);
            }
        }
    }
    replyStartConnection(rc, NULL, action);
}

/*
 * Reply to an action
 */
static void replyAction(int32_t rc, void * handle, void * vaction) {
    int   id = 0;
    int   extralen = 0;
    int   i = 0;
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    ism_jms_session_t * session;
    ism_jms_prodcons_t * cons = NULL;
    ism_jms_prodcons_t * producer;
    ism_time_t now;
    char xbuf[1024];
    xUNUSED int zrc;

    concat_alloc_t buf = {action->buf, sizeof(action->buf)};
    ism_xid_t * xidArray;
    int origAction = action->hdr.action;
#ifdef PERF_FULL
    int actionType = action->hdr.action;
#endif

    TRACEL(8, transport->trclevel, "replyAction action=%s connect=%u rc=%d resp=%lu session=%u\n", getActionName(origAction),
            transport->index, rc, endian_int64(action->hdr.msgid), endian_int32(action->hdr.item));

    if (rc == ISMRC_ResumedClientState) {    /* Ignore resumed state return code */
        rc = 0;
    }

    action->hdr.hdrcount = 1;    /* First header contains the return code */
    /*
     * Set return code
     */
    if (origAction != Action_commitSession) {
        ism_protocol_putIntValue(&buf, rc);
    } else {
        if ((action->session) && (action->session->transactionError)) {
            ism_protocol_putIntValue(&buf, action->session->transactionError);
            action->session->transactionError = 0;
        } else {
            ism_protocol_putIntValue(&buf, rc);
        }
    }
    if (action->clientTrans) {
        action->clientTrans->pobj->subscribeLock = 0;
    }

    if (rc == 0) {
        switch (action->hdr.action) {
        case Action_ackSync:
        	break;
        case Action_commitSession:
            /* Session has been committed, need to create a new transaction handle. */
            action->session->transaction = handle;				/* BEAM suppression: operating on NULL */

            break;

        case Action_createConnection:
            handleReplyCreateConnection(action,handle);
            break;

        case Action_closeConnection:
            TRACEL(7, transport->trclevel, "Close JMS connection: connect=%u client=%s\n", transport->index, transport->name);
            for (i=0; i<pobj->prodcons_alloc; i++) {
                ism_jms_prodcons_t * consumer = pobj->prodcons[i];
                if (consumer) {
                    if (consumer->shared == SHARED_GlobalND) {
                        if (consumer->subName) {
                            checkSubscriptionConsumer(consumer->subName, transport_SharedND);
                        }
                    }
                }
            }

            cleanupJmsClientState(transport);
            transport->closestate[3] = 1;
            break;

        case Action_createSession:
            if (handleReplyCreateSession(action,handle,&buf))
                return;
            break;

        case Action_closeSession:
            pthread_spin_lock(&pobj->sessionlock);
            TRACEL(7, transport->trclevel, "Close JMS session: connect=%u client=%s session=%d\n",
                     transport->index, transport->name, action->session->which);
            session = removeSession(transport, action->session->which);

            if (session) {
                ism_undelivered_message_t * curr = session->freeMsgs;
                while (curr) {
                    ism_undelivered_message_t * next = curr->next;
                    ism_common_free(ism_memory_protocol_misc,curr);
                    curr = next;
                }
                pthread_spin_destroy(&(session->lock));
                ism_common_free(ism_memory_protocol_misc,session);
            }
            pthread_spin_unlock(&pobj->sessionlock);
            break;

        case Action_createTransaction:
            action->session->transaction = handle;
            action->session->transacted = JMS_LOCAL_TRANSACTION;   /* Local transaction only */

            break;

        case Action_createConsumer:
            if (action->noConsumer) {
                id = 0;
            } else {
                cons = action->prodcons;
                cons->handle = handle;
                id = action->prodcons->which;
                TRACEL(7, transport->trclevel, "Create JMS consumer: connect=%u client=%s consumer=%d name=%s domain=%s handle=%p\n",
                          transport->index, transport->name, id, cons->name, domainName(cons->domain), cons->handle);
            }
            ism_protocol_putIntValue(&buf, id);
            action->hdr.hdrcount++;    /* The second header contains consumer id  */
            break;

        case Action_checkBrowser:
            ism_protocol_putIntValue(&buf, (uint32_t)(uintptr_t)handle);
            action->hdr.hdrcount++;    /* The second header contains count  */
            break;

        case Action_createBrowser:
            cons = action->prodcons;
            cons->handle = handle;
            id = action->prodcons->which;
            ism_protocol_putIntValue(&buf, id);
            TRACEL(7, transport->trclevel, "Create JMS browser: connect=%u client=%s id=%d name=%s domain=%s handle=%p\n",
                    transport->index, transport->name, id, cons->name, domainName(cons->domain), cons->handle);
            action->hdr.hdrcount++;    /* The second header contains consumer id  */
            break;

        case Action_createDurable:
            cons = action->prodcons;
            cons->handle = handle;
            id = action->prodcons->which;
            ism_protocol_putIntValue(&buf, id);
            TRACEL(7, transport->trclevel, "Create JMS durable consumer: connect=%u client=%s consumer=%d durable=%s name=%s domain=%s handle=%p\n",
                    transport->index, transport->name, id, cons->subName, cons->name, domainName(cons->domain), cons->handle);
            action->hdr.hdrcount++;    /* The second header contains consumer id */
            break;

        case Action_updateSubscription:
        	break;

        case Action_closeConsumer:
            handleReplyCloseConsumer(action);
            break;

        case Action_createProducer:
            if (handleReplyCreateProducer(action, handle, &buf))
                return;
            break;

        case Action_closeProducer:
            producer = removeProdcons(transport, action->prodcons->which, 0);
            if (producer) {
                TRACEL(7, transport->trclevel, "Close JMS producer: connect=%d client=%s producer=%d name=%s domain=%s\n",
                        transport->index, transport->name, producer->which, producer->name, domainName(producer->domain));
                if (producer->name)
                    ism_common_free(ism_memory_protocol_misc,producer->name);
                ism_common_free(ism_memory_protocol_misc,producer);
            }
            break;

        case Action_unsubscribeDurable:
            handleReplyUnsubscribeDurable(action);
            break;

        case Action_rollbackSession:
            /* Session has been rolled back, need to create a new transaction handle. */
            action->session->transaction = handle;
            ism_protocol_putLongValue(&buf, action->recordCount);
            action->hdr.hdrcount++;    /* The first header contains last msg id to cleanup on client */
            break;

        case Action_recover:
            ism_protocol_putLongValue(&buf, action->recordCount);
            action->hdr.hdrcount++;    /* The second header contains the last used ID */
            break;

        case Action_initConnection:
        case Action_getTime:
            ism_protocol_putIntValue(&buf, ServerVersion);
            action->hdr.hdrcount++;    /* The second header contains client version  */
            now = ism_common_currentTimeNanos();

            if (!memcmp(pobj->instanceHash, "\0\0\0\0", 4)) {
                uint32_t * hash = (uint32_t *)pobj->instanceHash;
                uint64_t lval = endian_int64(now);
                uint32_t crcval = ism_common_crc(0, (char *)&lval, 8);
                if (crcval == 0)
                    crcval = 0xdeadbeef;
                *hash = endian_int32(crcval);
            }

            transport->ready = 1;       /* We no longer need the DoS timer  */

            /*
             * Start up the timer if specified and if the first time
             */
            if (pobj->keepaliveTimeout) {
                int  timeout = abs(pobj->keepaliveTimeout);
                if (pobj->keepAliveCheckInterval == 0) {
                    if (timeout < 60)
                        timeout = 60;
                    pobj->keepaliveTimeout = timeout;
                    pobj->keepAliveCheckInterval = timeout/5;
                    pobj->keepAliveTimer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) keepAliveTimer,
                        transport, pobj->keepAliveCheckInterval, pobj->keepAliveCheckInterval, TS_SECONDS);
                }
            }
            ism_protocol_putLongValue(&buf, now);
            action->hdr.hdrcount++;           /* The third header contains time   */

            ism_common_strlcpy(xbuf, XSTR(BUILD_LABEL), 24);
            ism_protocol_putStringValue(&buf, xbuf);
            action->hdr.hdrcount++;           /* The fourth header contains build ID   */
            break;

        case Action_createQMRecord:
            ism_protocol_putLongValue(&buf, (uint64_t)handle);
            action->hdr.hdrcount++;    /* The second header contains the queue manager record handle */

            break;

        case Action_destroyQMRecord:
            break;

        case Action_getQMRecords:
            action->recordCount = 0;
            action->xbuf.buf = xbuf;
            action->xbuf.len = sizeof(xbuf);
            zrc = ism_engine_listQManagerRecords(action->session->handle, action, replyGetRecords);
            ism_protocol_putIntValue(&buf, (int)action->recordCount);
            action->hdr.hdrcount++;    /* The second header contains record count  */
            ism_protocol_putByteArrayValue(&buf, action->xbuf.buf, action->xbuf.used);
            ism_common_freeAllocBuffer(&action->xbuf);
            action->xbuf.buf    = NULL;
            action->xbuf.len    = 0;
            action->xbuf.inheap = 0;
            if (buf.inheap) {
                /*
                 * Full response does not fit into action structure,
                 * so it is in the heap now.
                 *
                 * Allocate additional memory, if necessary and
                 * leave starting 4 bytes for JMS framing.
                 */
                if (buf.len - buf.used < (sizeof(action->hdr) + 4)) {
                    /* Add more space to buf to also fit the header */
                    ism_common_allocAllocBuffer(&buf, (sizeof(action->hdr) + 4), 0);
                }
                memmove(buf.buf + sizeof(action->hdr) + 4, buf.buf, buf.used);
                memcpy(buf.buf + 4, &action->hdr, sizeof(action->hdr));
                buf.used += sizeof(action->hdr);
            }

            break;

        case Action_createQMXidRecord:
            ism_protocol_putLongValue(&buf, (uint64_t)handle);
            action->hdr.hdrcount++;    /* The second header contains the XA record handle */

            break;

        case Action_destroyQMXidRecord:
            break;

        case Action_getQMXidRecords:
            action->recordCount = 0;
            action->xbuf.buf = xbuf;
            action->xbuf.len = sizeof(xbuf);
            zrc = ism_engine_listQMgrXidRecords(action->session->handle, handle, action, replyGetRecords);
            ism_protocol_putIntValue(&buf, (int)action->recordCount);
            action->hdr.hdrcount++;    /* The second header contains record count */
            ism_protocol_putByteArrayValue(&buf, action->xbuf.buf, action->xbuf.used);
            ism_common_freeAllocBuffer(&action->xbuf);
            action->xbuf.buf    = NULL;
            action->xbuf.len    = 0;
            action->xbuf.inheap = 0;
            if (buf.inheap) {
                /*
                 * Full response does not fit into action structure,
                 * so it is in the heap now.
                 *
                 * Allocate additional memory, if necessary and
                 * leave starting 4 bytes for JMS framing.
                 */
                if (buf.len - buf.used < (sizeof(action->hdr) + 4)) {
                    /* Add more space to buf to also fit the header */
                    ism_common_allocAllocBuffer(&buf, (sizeof(action->hdr) + 4), 0);
                }
                memmove(buf.buf + sizeof(action->hdr) + 4, buf.buf, buf.used);
                memcpy(buf.buf + 4, &action->hdr, sizeof(action->hdr));
                buf.used += sizeof(action->hdr);
            }
            break;

        case Action_listSubscriptions:
            action->recordCount = 0;
            action->xbuf.buf = xbuf;
            action->xbuf.len = sizeof(xbuf);
            zrc = ism_engine_listSubscriptions(pobj->handle, NULL, action, addSubscription);
            ism_protocol_putIntValue(&buf, (int)action->recordCount);
            action->hdr.hdrcount++;    /* The second header contains subscription count */
            ism_protocol_putByteArrayValue(&buf, action->xbuf.buf, action->xbuf.used);
            ism_common_freeAllocBuffer(&action->xbuf);
            action->xbuf.buf    = NULL;
            action->xbuf.len    = 0;
            action->xbuf.inheap = 0;
            if (buf.inheap) {
                /*
                 * Full response does not fit into action structure,
                 * so it is in the heap now.
                 *
                 * Allocate additional memory, if necessary and
                 * leave starting 4 bytes for JMS framing.
                 */
                if (buf.len - buf.used < (sizeof(action->hdr) + 4)) {
                    /* Add more space to buf to also fit the header */
                    ism_common_allocAllocBuffer(&buf, (sizeof(action->hdr) + 4), 0);
                }
                memmove(buf.buf + sizeof(action->hdr) + 4, buf.buf, buf.used);
                memcpy(buf.buf + 4, &action->hdr, sizeof(action->hdr));
                buf.used += sizeof(action->hdr);
            }
            break;

        case Action_startGlobalTransaction:
            /* Global transaction has been created. */
            action->session->transaction = handle;
            action->session->transacted = JMS_GLOBAL_TRANSACTION; /* Global transaction */
            break;

        case Action_endGlobalTransaction:
            /* Global transaction has been disassociated from this session. */
            action->session->transaction = NULL;
            break;

        case Action_prepareGlobalTransaction:
        case Action_commitGlobalTransaction:
        case Action_rollbackGlobalTransaction:
        case Action_forgetGlobalTransaction:
            break;

        case Action_recoverGlobalTransactions:
            xidArray = handle;
            ism_protocol_putIntValue(&buf, action->recordCount);
            action->hdr.hdrcount++;    /* The second header contains XID count */

            action->xbuf.buf = xbuf;
            action->xbuf.len = sizeof(xbuf);

            for (i = 0; i < action->recordCount; i++) {
                ism_protocol_putXidValue(&action->xbuf, xidArray + i);
            }

            ism_common_free(ism_memory_protocol_misc,xidArray);

            ism_protocol_putByteArrayValue(&buf, action->xbuf.buf, action->xbuf.used);
            ism_common_freeAllocBuffer(&action->xbuf);
            action->xbuf.buf = NULL;
            action->xbuf.len = 0;
            action->xbuf.inheap = 0;
            if (buf.inheap) {
                /*
                 * Full response does not fit into action structure,
                 * so it is in the heap now.
                 *
                 * Allocate additional memory, if necessary and
                 * leave starting 4 bytes for JMS framing.
                 */
                if (buf.len - buf.used < (sizeof(action->hdr) + 4)) {
                    /* Add more space to buf to also fit the header */
                    ism_common_allocAllocBuffer(&buf, (sizeof(action->hdr) + 4), 0);
                }
                memmove(buf.buf + sizeof(action->hdr) + 4, buf.buf, buf.used);
                memcpy(buf.buf + 4, &action->hdr, sizeof(action->hdr));
                buf.used += sizeof(action->hdr);
            }

            break;
        case Action_startConnection:
        {
            int counter = 0;
            uint64_t * sqns = (uint64_t*)alloca(pobj->sessions_alloc*sizeof(uint64_t)*2);
            for (i = 0; i < pobj->sessions_alloc; i++) {
                ism_jms_session_t * sess = pobj->sessions[i];
                if (sess && (sess->seqnum > 1)) {
                    sqns[counter++] = sess->which;
                    sqns[counter++] = sess->seqnum-1;
                }
            }
            if (counter) {
                ism_protocol_putIntValue(&buf,counter/2);
                for (i = 0; i < counter; i++) {
                    ism_protocol_putIntValue(&buf,(uint32_t)sqns[i++]);
                    ism_protocol_putLongValue(&buf,sqns[i]);
                }
                action->hdr.hdrcount++;
            }
        }
            break;
        default:
            break;
        }
    } else {
        if (rc == ISMRC_DestinationInUse) {
            ism_protocol_putIntValue(&buf, (int)(uintptr_t)handle);
            action->hdr.hdrcount++;           /* The second header contains count  */
        }
        if (action->hdr.action == Action_createProducer) {
            int msgcode;

            switch (rc) {
            case ISMRC_NotAuthorized:
                msgcode = 2302;

                if (!previouslyLogged(pobj, msgcode)) {
                    LOG(WARN, Connection, 2302, "%u%-s%-s%-s%-s",
                        "Unable to create a producer due to an authorization failure: "
                        "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3} UserID={4}.",
                        transport->index, transport->name, transport->protocol, transport->listener->name,
                        transport->userid ? transport->userid : "");
                }
                break;

            case ISMRC_DestNotValid:
                msgcode = 2303;
                if (!previouslyLogged(pobj, msgcode)) {
                    LOG(WARN, Connection, 2303, "%u%-s%-s%-s",
                "Unable to create a producer because the destination is not valid: "
                "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.",
                transport->index, transport->name, transport->protocol, transport->listener->name);
                }
                break;

            case ISMRC_BadSysTopic:
                if (action->hdr.action == Action_createDurable) {
                    msgcode = 2206;
                    if (!previouslyLogged(pobj, msgcode)) {
                        LOG(WARN, Connection, 2206, "%u%-s%-s%-s",
                            "A durable subscription is not allowed on a system topic: "
                            "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.",
                            transport->index, transport->name, transport->protocol, transport->listener->name);
                    }
                } else if (action->hdr.action == Action_createProducer) {
                    msgcode = 2304;
                    if (!previouslyLogged(pobj, msgcode)) {
                        LOG(WARN, Connection, 2304, "%u%-s%-s%-s",
                            "Unable to create a producer because the topic is a system topic: "
                            "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.",
                            transport->index, transport->name, transport->protocol, transport->listener->name);
                    }
                }
                break;

            default:
                msgcode = 2301000 + rc;

                if (!previouslyLogged(pobj, msgcode)) {
                    char xerrbuf[4096];

                    if (ism_common_getLastError() != rc)
                        ism_common_setError(rc);

                    ism_common_formatLastError(xerrbuf, 4096);

                    LOG(WARN, Connection, 2301, "%u%-s%-s%-s%-s%s%d",
                        "Unable to create a producer: "
                        "ConnectionID={0} ClientID={1} Endpoint={2} UserID={3} Protocol={4} Error={5} RC={6}.",
                        transport->index, transport->name, transport->listener->name,
                        transport->userid ? transport->userid : "",
                        transport->protocol, xerrbuf, rc);
                }
                break;
            }
        }
    }
    extralen = buf.used;
    action->hdr.action = Action_reply;

    TRACEL(8, transport->trclevel, "replyAction send reply: action=%s connect=%u client=%s rc=%d\n",
            getActionName(action->hdr.action), transport->index, transport->name, rc);
    if (!buf.inheap) {
        transport->send(transport, (char *) &action->hdr, sizeof(action->hdr) + extralen,
                0, SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
    } else {
        transport->send(transport, buf.buf+4, buf.used, 0, SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
        ism_common_freeAllocBuffer(&buf);
    }

    if ((rc == 0) && ((origAction == Action_createConsumer) || (origAction == Action_createDurable)) && (cons && cons->suspended)) {
        TRACEL(7, transport->trclevel, "Submitting job request to resume consumer: action=%s connect=%u consumer=%d(%p)\n",
            getActionName(origAction), transport->index, cons->which, cons->handle);
        ism_transport_submitAsyncJobRequest(transport, resumeConsumerDelivery, (void*)((uintptr_t)cons->which),0);
    } else {
        /* Action has been taken, check if close was requested */
        if (__sync_sub_and_fetch(&pobj->inprogress, 1) == -1) { /* BEAM suppression: constant condition */
            ism_protocol_action_t act = {0};
            act.transport = transport;
            act.hdr.action = Action_closeConnection;
            replyClosing(0, NULL, &act);
        }
    }


    /* We have a failure */
    if (rc) {
        /*
         * If we detected a bad condition, we first tried to set the reply but we then close
         * the connection as this is a serious condition and we do not want further damage.
         */
        if (rc == ISMRC_BadClientData) {
            TRACEL(6, transport->trclevel, "The JMS connection data is not valid: connect=%u\n", transport->index);
            transport->close(transport, ISMRC_BadClientData, 0, "The data received from the client is not valid");
        }

        /*
         * If the createConnection fails, close the connection
         */
        if (origAction == Action_createConnection) {
            if (rc == ISMRC_ClientIDInUse) {
                ism_transport_checkClientLiveness(transport->clientID, transport->index);
            } else {
                /* TODO: Consider adding the following two lines to
                 * prevent crashes for cases where setError() has
                 * not yet been called for some reason.
                 */
                /* if (!ism_common_getLastError())
                    ism_common_setError(rc); */
                ism_common_formatLastError(xbuf, sizeof xbuf);
                __sync_add_and_fetch(&transport->listener->stats->bad_connect_count, 1);
                TRACEL(9, transport->trclevel, "Failed in create connection: connect=%u\n", transport->index);
                transport->close(transport, rc, 0, xbuf);
            }
        }
    }


#ifdef PERF_FULL
       double endTime = ism_common_readTSC();
    TRACEL(5, transport->trclevel, "Reply=%s: time2complete=%g  \n", getActionName(actionType), (endTime-action->tsc) );
#endif
}


/*
 * Close the consumer and send the reply.
 */
static void replyCloseConsumer(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_jms_prodcons_t * prodcons = action->prodcons;

    if (prodcons) {
        /*
         * Set the indicator that close is in progress. If set failed,
         * then this has been done before and we don't need to proceed.
         */
        if (!__sync_bool_compare_and_swap(&prodcons->closing, 0, 1)) {
            jmsProtoObj_t * pobj = (jmsProtoObj_t*) transport->pobj;
            TRACEL(7, transport->trclevel, "JMS replyCloseConsumer is already closing: connect=%u client=%s consumer=%d name=%s domain=%s\n",
                  transport->index, transport->name, prodcons->which, prodcons->name, domainName(prodcons->domain));
            /* Action has been taken, check if close was requested */
            if (__sync_sub_and_fetch(&pobj->inprogress, 1) == -1) { /* BEAM suppression: constant condition */
                ism_protocol_action_t act = {0};
                act.transport = transport;
                act.hdr.action = Action_closeConnection;
                replyClosing(0, NULL, &act);
            }
            return;
        }

        TRACEL(8, transport->trclevel, "replyCloseConsumer: Close JMS consumer connect=%u client=%s id=%d name=%s domain=%s\n",
                transport->index, transport->name, prodcons->which, prodcons->name, domainName(prodcons->domain));
        pthread_spin_lock(&prodcons->lock);
        rc = ism_engine_destroyConsumer(prodcons->handle, action, sizeof(ism_protocol_action_t), replyAction);
        prodcons->handle = NULL;
        pthread_spin_unlock(&prodcons->lock);
        if (rc != ISMRC_AsyncCompletion)
            replyAction(rc, NULL, action);
    } else {
        ism_common_setError(ISMRC_Closed);
        replyAction(ISMRC_Closed, NULL, action);
    }
}


/*
 * Mark all not ACKed messages as "not delivered" and destroy the session.
 */
static void replyCloseSession(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = (ism_protocol_action_t *) vaction;
    ism_jms_session_t * session = action->session;
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t * pobj = (jmsProtoObj_t*)transport->pobj;

    /* If session is transacted - rollback */
    if (session->transacted && session->transaction) {
        void * txHandle = session->transaction;
        session->transaction = NULL;
        if (session->transacted == JMS_LOCAL_TRANSACTION) {
            rc = ism_engine_rollbackTransaction(session->handle,
                     txHandle, action, sizeof(ism_protocol_action_t), replyCloseSession);
            if (rc == ISMRC_AsyncCompletion)
                return;
        }
    }
    if (clearUndeliveredMessages(action, replyCloseSession) == ISMRC_AsyncCompletion)
        return;
    handle = session->handle;
    pthread_spin_lock(&pobj->sessionlock);
    session->handle = 0;
    pthread_spin_unlock(&pobj->sessionlock);
    rc = ism_engine_destroySession(handle, action, sizeof(ism_protocol_action_t), replyAction);
    if (rc != ISMRC_AsyncCompletion)
          replyAction(rc, NULL, action);
}
/*
 * Reply during closing.
 * This code is called when the connection is closed without going thru normal shutdown.
 */
static void replyClosing(int32_t rc, void * handle, void * vaction) {
    int  i;
    ism_protocol_action_t * action = (ism_protocol_action_t *)vaction;
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t * pobj = (jmsProtoObj_t*)transport->pobj;

    TRACEL(8, transport->trclevel, "replyClosing: rc=%d action=%s connect=%u client=%s\n", rc, getActionName(action->hdr.action),
            transport->index, transport->name);


    switch (action->hdr.action) {

    /* Close a consumer or producer.  If last one, close the sessions */
    case Action_closeConsumer:
        /*
         * First, stop message delivery (to avoid immediate redelivery)
         * and NACK all outstanding messages for all sessions.
         *
         * Any remaining messages in the incompmsgids list can be cleared,
         * as delivery handles will be cleaned up by destroyConsumer and
         * destroySession.
         */
        for (i = 0; i < pobj->sessions_alloc; i++) {
            ism_jms_session_t *  session = pobj->sessions[i];

            if (session != NULL && session->handle != NULL) {
                /* Do not proceed until the message delivery has been stopped completely. */
                if (!(__sync_fetch_and_or(&session->suspended, SUSPENDED_BY_PROTOCOL))) {
                    rc = ism_engine_stopMessageDelivery(session->handle, action, sizeof(ism_protocol_action_t), replyClosing);
                    if (rc == ISMRC_AsyncCompletion) {
                        return;
                    }
                }

                /* If session is transacted - rollback */
                if (session->transacted && session->transaction) {
                    handle = session->transaction;
                    session->transaction = NULL;
                    if (session->transacted == JMS_LOCAL_TRANSACTION) {
                        rc = ism_engine_rollbackTransaction(session->handle, handle, action,
                            sizeof(ism_protocol_action_t), replyClosing);

                        if (rc == ISMRC_AsyncCompletion)
                            return;
                    }
                }
                rc = clearUndeliveredMessages(action, replyClosing);
                if (rc == ISMRC_AsyncCompletion)
                    return;
            }
        }
        TRACEL(7, transport->trclevel, "Close JMS Consumer: connect=%u client=%s\n", transport->index, transport->name);
        for (i = action->valcount; i < pobj->prodcons_alloc; i++) {
            ism_jms_prodcons_t * pc = pobj->prodcons[i];
            if (pc == NULL)
                continue;
            action->prodcons = pc;
            action->valcount = i+1;
            if (pc->kind == KIND_CONSUMER || pc->kind == KIND_BROWSER) {
                action->transport = pc->transport;
                action->session = getSession(action->transport, pc->hdr.item);
                rc = jmsCloseConsumer(action);
                if (rc == ISMRC_AsyncCompletion)
                    return;
            } else {
            	handle = pc->handle;
                if (handle) {
                	pc->handle = NULL;
                    rc = ism_engine_destroyProducer((ismEngine_ProducerHandle_t)handle,
                          action, sizeof(ism_protocol_action_t), replyClosing);
                }
                if (rc == ISMRC_AsyncCompletion)
                    return;
            }
        }
        action->prodcons = NULL;
        action->session = NULL;
        action->valcount = 0;
        action->hdr.action = Action_closeSession;
        /* fall thru */;

    /* Session closed.  If last one, close the connection */
    case Action_closeSession:
        TRACEL(7, transport->trclevel, "Close JMS Session: connect=%u client=%s\n", transport->index, transport->name);
        while(pobj->sessions_used){
          if (action->session) {
              removeSession(transport,action->session->which);
              pthread_spin_destroy(&(action->session->lock));
              ism_common_free(ism_memory_protocol_misc,action->session);
              action->session = NULL;
          }
          for (i = action->valcount; i < pobj->sessions_alloc; i++) {
              ism_jms_session_t * session = pobj->sessions[i];
              if (session != NULL) {
                  action->session = session;
                  action->valcount = i+1;
                  if (session->handle != NULL) {
                      rc = ism_engine_destroySession(session->handle, action, sizeof(ism_protocol_action_t), replyClosing);
                      session->handle = NULL;
                      if (rc == ISMRC_AsyncCompletion)
                          return;
                  }
                  break;
              }
          }
        }
        action->valcount = 0;
        action->hdr.action = Action_closeConnection;
        /* Fall thru */

    /*
     * Close the connection
     */
    case Action_closeConnection:
        /*
         * Destroy the engine client state object
         */
        if (pobj->handle) {
            handle = pobj->handle;

            /* Delete the connection artifacts */
            TRACEL(7, transport->trclevel, "Close JMS Connection: connect=%u client=%s pobj=%p handle=%p keepAliveTimer=%p\n",
                     transport->index, transport->name, pobj, pobj->handle, pobj->keepAliveTimer);

            pobj->handle = NULL;

            rc = ism_engine_destroyClientState(handle, ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                    action, sizeof(ism_protocol_action_t), replyClosing);

            if (rc == ISMRC_AsyncCompletion)
                return;
        }

        cleanupJmsClientState(transport);

        /*
         * Stop the keepalive timer
         */
        if (pobj->keepAliveTimer) {
            ism_timer_t timer = pobj->keepAliveTimer;
            if(__sync_bool_compare_and_swap(&pobj->keepAliveTimer, timer, NULL)) {
                ism_common_cancelTimer(timer);
            }
        }
        pthread_spin_destroy(&pobj->lock);
        pthread_spin_destroy(&pobj->sessionlock);

        /* Tell the transport we are done */
        transport->closed(transport);
        break;
    }
}

/*
 * Commit session request processing. All unconfirmed messages are added to
 * the transaction and then commit is executed in the engine.
 * This function can be called directly or via a callback from
 * ism_engine_stopMessageDelivery.
 */
static void replyCommitSession(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_jms_session_t * session = action->session;
    if (session->transactionError == 0) {
        uint32_t flag = ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS;

        /* All messages are in transaction, commit and recreate the transaction handle */
        handle = session->transaction;
        session->transaction = NULL;
        rc = ism_engine_commitTransaction(session->handle,
                handle, flag, action, sizeof(ism_protocol_action_t), createTransaction);
        if (rc != ISMRC_AsyncCompletion)
            createTransaction(rc, NULL, action);
    } else {
        action->recordCount = -1;
        replyRollbackSession(rc, NULL, action);
    }
}


/*
 * Complete create browser after session is created
 */
static void replyCreateBrowser(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_jms_prodcons_t * consumer = action->prodcons;
    ism_prop_t * cprops = NULL;
    ismEngine_ConsumerHandle_t consumerh;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    uint32_t consumerOption = ismENGINE_CONSUMER_OPTION_BROWSE | ismENGINE_CONSUMER_OPTION_PAUSE;

    if (rc) {
        replyAction(rc, handle, vaction);
        return;
    }

    if ((action->hdr.action == Action_createSession) && (transport->pobj->browser_session_handle == NULL)) {
        transport->pobj->browser_session_handle = handle;
        xUNUSED int zrc = ism_engine_startMessageDelivery(handle, ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
        action->hdr.action = Action_createBrowser;
    }

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_NONE;

    /*
     * Construct a properties structure if required
     */
    if (consumer->rule) {
        ism_field_t field;
        cprops = ism_common_newProperties(20);
        field.type  = VT_ByteArray;
        field.val.s = (char *)consumer->rule;
        field.len   = consumer->rulelen;
        ism_common_setProperty(cprops, "Selector", &field);
        consumerOption |= ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION;
    }

    /*
     * Create the engine object
     */
    rc = ism_engine_createConsumer(transport->pobj->browser_session_handle, consumer->domain,
        consumer->name, &subAttrs, NULL, consumer, sizeof(*consumer), replyReceive,
        cprops, consumerOption, &consumerh,
        action, action->actionsize, replyCreateConsumer);

    if (cprops) {
        ism_common_freeProperties(cprops);
    }
    if (rc != ISMRC_AsyncCompletion) {
        replyCreateConsumer(rc, consumerh, action);
    }

}


/*
 * Check for errors when creating consumer.
 * If there is an error, remove the consumer object from
 * the transport and free it.
 */
static void replyCreateConsumer(int32_t rc, void * handle, void * vaction) {
    if (rc) {
        ism_protocol_action_t * action = vaction;
        ism_transport_t * transport = action->transport;
        ism_jms_prodcons_t * consumer = action->prodcons;
        jmsProtoObj_t * pobj = (jmsProtoObj_t*) transport->pobj;
        int msgcode;

        switch (rc) {
        case ISMRC_NotAuthorized:
            msgcode = 2202;
            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2202, "%u%-s%-s%-s%-s",
                    "A message consumer could not be created due to an authorization failure: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3} UserID={4}.",
                    transport->index, transport->name, transport->protocol, transport->listener->name,
                    transport->userid ? transport->userid : "");
            }
            break;

        case ISMRC_DestNotValid:
            msgcode = 2205;
            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2205, "%u%-s%-s%-s",
                    "Unable to create a consumer because the destination is not valid: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.",
                    transport->index, transport->name, transport->protocol, transport->listener->name);
            }
            break;

        case ISMRC_ServerCapacity:
        case ISMRC_AllocateError:
            transport->closestate[3] = 3;   /* Reason for close is server capacity */
            /* fall thru */

        default:
            msgcode = 2201000 + rc;
            if (!previouslyLogged(pobj, msgcode)) {
                char xerrbuf[4096];

                if (ism_common_getLastError() != rc)
                    ism_common_setError(rc);

                ism_common_formatLastError(xerrbuf, 4096);

                LOG(WARN, Connection, 2201, "%u%-s%-s%-s%-s%s%d",
                    "A message consumer could not be created: "
                    "ConnectionID={0} ClientID={1} Endpoint={2} UserID={3} Protocol={4} Error={5} RC={6}.",
                    transport->index, transport->name, transport->listener->name,
                    transport->userid ? transport->userid : "",
                    transport->protocol, xerrbuf, rc);
            }
            break;
        }

        /*
         * Destroy the subscription if we failed to create the consumer
         */
        if (consumer) {
            if ((action->subscriptionFound == SUB_NotFound || action->subscriptionFound == SUB_Resubscribe)&&
                    consumer->subName) {
                void * clientState = pobj->handle;
                switch (action->shared) {
                case SHARED_Global:       clientState = client_Shared;              break;
                case SHARED_GlobalND:     clientState = client_SharedND;            break;
                }
                TRACEL(8, transport->trclevel, "Destroy subscription when createConsumer failed: client=%s name=%s rc=%d\n",
                      transport->name, consumer->subName, rc);
                xUNUSED int zrc = ism_engine_destroySubscription(pobj->handle, consumer->subName, clientState, NULL, 0, NULL);
            }
            removeProdcons(transport, consumer->which, 1);
            freeConsumer(consumer);
        }
    }

    replyAction(rc, handle, vaction);
}


/*
 * Callback for saving each manager and XA records.
 * For each record, the handle and the data is stored in the buffer within the action.
 */
static void replyGetRecords(void * data, size_t dataLength, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;

    action->recordCount++;

    ism_protocol_putLongValue(&action->xbuf, (uint64_t)handle);
    ism_protocol_putByteArrayValue(&action->xbuf, data, dataLength);
}


/*
 * Reply with an error
 */
static void replyError(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    concat_alloc_t buf = {action->buf, sizeof(action->buf)};
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    char  xbuf[2048];
    buf.buf = &xbuf[4];
    buf.len = sizeof(xbuf)-4;
    buf.inheap = 0;
    memcpy(buf.buf, &action->hdr, sizeof(actionhdr));
    buf.used = sizeof(actionhdr);
    TRACEL(5, transport->trclevel, "JMS reply with error: rc=%d connect=%u client=%s\n", rc, transport->index, transport->name);

    /*
     * Set return code, if not set yet.
     */
    if (rc == 0) {
        rc = action->rc;
    }
    ism_protocol_putIntValue(&buf, rc);
    action->hdr.hdrcount = 1;
    transport->send(transport, buf.buf, buf.used,
                    0, SFLAG_SYNC|SFLAG_FRAMESPACE|SFLAG_OUTOFORDER);
    ism_common_freeAllocBuffer(&buf);

    if (__sync_sub_and_fetch(&pobj->inprogress, 1) == -1) { /* BEAM suppression: constant condition */
        ism_protocol_action_t act = {0};
        act.transport = transport;
        act.hdr.action = Action_closeConnection;
        replyClosing(0, NULL, &act);
    }
}


/*
 * Reply from a put message
 * We only send a reply for a message with wait.
 */
static void replyMessage(int32_t rc, void * handle, void * vaction) {
    struct ism_protocol_action_t * act = vaction;
    ism_transport_t * transport = act->transport;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    actionhdr * hdr = &(act->hdr);
    int extralen;
    concat_alloc_t buf = {act->buf, sizeof(act->buf)};
    int msgcode;

    TRACEL(8, transport->trclevel, "replyMessage act=%s rc=%d connect=%u client=%s session=%d\n", getActionName(act->hdr.action),
            rc, transport->index, transport->name, act->session->which);

    if (rc == ISMRC_SomeDestinationsFull) {
            __sync_add_and_fetch(&transport->listener->stats->count[transport->tid].warn_msg, 1);
            __sync_add_and_fetch(&transport->warn_msg, 1);
            /* ISMRC_SomeDestinationsFull provides a warning statistic for
             * the server.  From the client perspective, the request has
             * succeeded. So change the rc to 0 now that the stat has been
             * recorded.
             */
            rc = 0;
    } else if (rc == ISMRC_NoMatchingDestinations || rc == ISMRC_NoMatchingLocalDestinations) {
        /* From the client perspective, the request succeeded */
        rc = 0;
    }

    /*
     * Log first occurrence of various errors
     *
     * In some cases, we set the connection close reason so that if the client responds by closing the
     * connection we will show that as the connection close reason.  Note that we are not certain that
     * the reason the client closed the connection was the previous bad return code, but we know that that
     * problem did just occur.  On a good message we reset this close reason.
     */
    if (rc) {
        __sync_add_and_fetch(&transport->listener->stats->count[transport->tid].lost_msg, 1);
        __sync_add_and_fetch(&transport->lost_msg, 1);
        switch (rc) {
        case ISMRC_DestinationFull:
            msgcode = 2107;
            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2107, "%u%-s%-s%-s",
                    "The client request to publish a message to the server failed because the destination is full: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.",
                    transport->index, transport->name, transport->protocol, transport->listener->name);
            }
            break;

        case ISMRC_DestNotValid:
            msgcode = 2108;
            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2108, "%u%-s%-s%-s",
            "The client request to publish a message to the server failed because the destination is not valid: "
            "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.",
            transport->index, transport->name, transport->protocol, transport->listener->name);
            }
            break;

        case ISMRC_NotAuthorized:
            msgcode = 2106;
            transport->closestate[3] = 5;   /* Reason for close is not authorized */
            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2106, "%u%-s%-s%-s%-s",
                    "The client request to publish a message to the server failed due to an authorization failure: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3} UserID={4}.",
                    transport->index, transport->name, transport->protocol, transport->listener->name,
                    transport->userid ? transport->userid : "");
            }
            break;

        case ISMRC_MsgTooBig:
            msgcode = 2102;
            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2102, "%u%-s%-s%-s%d",
                "The client request to publish a message to the server failed because it is too large for the endpoint: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3} MsgSize={4}.",
                    transport->index, transport->name, transport->protocol, transport->listener->name,
                    (int)act->recordCount);
            }

            break;

        case ISMRC_ServerCapacity:
        	msgcode = 2109;
            transport->closestate[3] = 3;   /* Reason for close is server capacity */
            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2109, "%u%-s%-s%-s",
                    "The client request to publish a persistent message to the server because the server capacity has been reached: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.",
                    transport->index, transport->name, transport->protocol, transport->listener->name);
            }

            break;

        case ISMRC_AllocateError:
            transport->closestate[3] = 3;   /* Reason for close is server capacity */
            /* fall thru */

        default:
            msgcode = 2101000 + rc;
            if (!previouslyLogged(pobj, msgcode)) {
                char xerrbuf[4096];

                if (ism_common_getLastError() != rc)
                    ism_common_setError(rc);

                ism_common_formatLastError(xerrbuf, 4096);
                LOG(WARN, Connection, 2101, "%u%-s%-s%-s%-s%s%d",
                	"The client request to publish a message to the server failed: ConnectionID={0} ClientID={1} Endpoint={2} UserID={3} Protocol={4} Error={5} RC={6}.",
                    transport->index, transport->name, transport->listener->name,
                    transport->userid ? transport->userid : "",
                    transport->protocol, xerrbuf, rc);
            }
            break;
        }
    } else {
        transport->closestate[3] = 0;    /* Connection closed by client */
    }

    /*
     * For a synchronous message, send the reply
     */
    if (act->hdr.action == Action_messageWait || act->hdr.action == Action_msgNoProdWait) {
        ism_protocol_putIntValue(&buf, rc);
        hdr->action = Action_reply;
        hdr->hdrcount = 1;
        extralen = buf.used;
        transport->send(transport, (char *)hdr, sizeof(actionhdr) + extralen,
                              0, SFLAG_SYNC|SFLAG_FRAMESPACE|SFLAG_OUTOFORDER);

    }

    /*
     * For an asynchronous message, send a raiseException action only if there is a problem
     */
    else {
        if (rc) {
            ism_jms_session_t * session = act->session;
            if (session && session->transaction && (session->transactionError == 0)) {
                session->transactionError = rc;
            }
            /*
             *Do not send IMARC_DestinationFull error to the client if message is not persistent (defect 23667)
             */
            if ((rc != ISMRC_DestinationFull) && session){
                char xbuf[2048];
                hdr->hdrcount = 8;
                hdr->action = Action_raiseException;
                buf.buf = &xbuf[4];
                buf.len = sizeof(xbuf)-4;
                memcpy(buf.buf, hdr, sizeof(actionhdr));
                buf.used = sizeof(actionhdr);
                ism_protocol_putIntValue(&buf, rc);
                ism_protocol_putIntValue(&buf, session->which);
                if (act->prodcons) {
                    ism_protocol_putIntValue(&buf, act->prodcons->which);
                    ism_protocol_putByteValue(&buf, 0);
                    ism_protocol_putNullValue(&buf);
                } else {
                    /*
                     * If no producer was used to send the message, there are 2 headers
                     * containing destination name and domain.
                     */
                    const char * destinationName = NULL;
                    int domain = ismDESTINATION_QUEUE;
                    int i;
                    for (i = 0; i < act->valcount; i++) {
                        if (act->values[i].type == VT_String) {
                            destinationName = act->values[i].val.s;
                        } else if (act->values[i].type == VT_Byte) {
                            domain = act->values[i].val.i;
                        }
                    }
                    ism_protocol_putIntValue(&buf, 0);
                    if (act->valcount == 0) {
                        ism_protocol_putNullValue(&buf);
                        ism_protocol_putStringValue(&buf, "unknown");
                    } else {
                       ism_protocol_putByteValue(&buf, domain);
                       ism_protocol_putStringValue(&buf, destinationName);
                    }
                }
                ism_protocol_putStringValue(&buf, transport->server_addr);
                ism_protocol_putIntValue(&buf, transport->serverport);
                ism_protocol_putStringValue(&buf, transport->listener->name);

                transport->send(transport, buf.buf, buf.used, 0, SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
                ism_common_freeAllocBuffer(&buf);
            }
        }
    }

    if (buf.inheap) {
        ism_common_freeAllocBuffer(&buf);
    }

    /* Action has been taken, check if close was requested */
    if (__sync_sub_and_fetch(&pobj->inprogress, 1) == -1) { /* BEAM suppression: constant condition */
        ism_protocol_action_t action = {0};
        action.transport = transport;
        action.hdr.action = Action_closeConnection;
        replyClosing(0, NULL, &action);
    }

#ifdef PERF_FULL
    {
        double endTime = ism_common_readTSC();
        TRACEL(5, transport->trclevel, "ReplyMessage: time2complete=%g  \n",(endTime-->hdr.tsc) );
    }
#endif
}


/*
 * Reply with a received message.
 */
static bool replyReceive (
        ismEngine_ConsumerHandle_t consumerh,
        ismEngine_DeliveryHandle_t deliveryh,
        ismEngine_MessageHandle_t  msgh,
        uint32_t                   seqnum,
        ismMessageState_t          state,
        uint32_t                   options,
        ismMessageHeader_t *       hdr,
        uint8_t                    areas,
        ismMessageAreaType_t       areatype[areas],
        size_t                     areasize[areas],
        void *                     areaptr[areas],
        void *                     vaction,
        ismEngine_DelivererContext_t * _delivererContext ) {
    ism_jms_prodcons_t * cons = vaction;
    ism_transport_t * transport = cons->transport;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;

    ism_protocol_action_t * jmsg;
    uint32_t  proplen = 0;
    uint32_t  bodylen = 0;
    char *    propp = NULL;
    char *    bodyp = NULL;
    int    needconvert = 0;
    concat_alloc_t  buf = {0};
    char   xbuf[10000];
    int    i;
    int    rc;
    int    expired;
    bool   returncode = true;
    int    flags = SFLAG_ASYNC|SFLAG_FRAMESPACE;
    ism_jms_session_t * session;
    uint64_t msgid;
    actionhdr   * jhdr;
    static int hdrpos = OffsetOf(ism_protocol_action_t, hdr);

    ism_jms_prodcons_t * consumer = getProdcons(transport, cons->which);

    assert(consumer != NULL);
    session = getSession(transport, consumer->hdr.item);
    if (consumer->closing || (!pobj->started && consumer->kind != KIND_BROWSER)) {
        ism_engine_suspendMessageDelivery(consumerh, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
        if (deliveryh) {
            xUNUSED int zrc = ism_engine_confirmMessageDelivery(session->handle,
                    NULL, deliveryh, ismENGINE_CONFIRM_OPTION_NOT_DELIVERED,
                    NULL, 0, NULL);
        }
        return false;
    }

    /*
     * We have a message
     */
    assert(msgh);

    /*
     * Find the properties and body
     */
    for (i=0; i<areas; i++) {
        if (areatype[i] == ismMESSAGE_AREA_PROPERTIES) {
            proplen = (uint32_t)areasize[i];
            propp = (char *)areaptr[i];
        } else if (areatype[i] == ismMESSAGE_AREA_PAYLOAD) {
            bodylen = (uint32_t)areasize[i];
            bodyp = (char *)areaptr[i];
        }
    }

    /*
     * Do expiration - Only check messages being redelivered
     * (engine checks on first delivery but not for redelivers as that
     * isn't appropriate for all protocols so we do it here)
     */
    expired =    (hdr->Expiry != 0)
              && (hdr->RedeliveryCount > 0)
              && (hdr->Expiry < ism_common_nowExpire());

    if (expired && deliveryh) {
        /*
         * We only do expiry if we have a deliveryhandle (so server stats
         * are correct) but messages being redelivered will use have handles
         * aside from the unlikely edge case where it was delivered to an
         * acking client who then nacked it and subsequently it's redelivered
         * to an non-acking client. In that case client will receive the expired
         * message
         */
        xUNUSED int zrc = ism_engine_confirmMessageDelivery(session->handle,
                session->transaction, deliveryh, ismENGINE_CONFIRM_OPTION_EXPIRED,
                NULL, 0, NULL);
        returncode = true;
        goto cleanup;
    }

    /*
     * Initialize the action header
     */
    jmsg = (ism_protocol_action_t *)xbuf;
    jhdr = &jmsg->hdr;
    jhdr->hdrcount = 0;
    jhdr->itemtype = ITEMT_None;
    jhdr->item     = endian_int32(consumer->which);

    TRACEL(8, transport->trclevel, "replyReceive connect=%u client=%s consumer=%d msglen=%d proplen=%d dup=%d\n", transport->index,
            transport->name, consumer->which, bodylen, proplen, hdr->RedeliveryCount);

    /*
     * Fill in the transport header
     */
    jhdr->action = Action_message;
    jhdr->priority = hdr->Priority;
    jhdr->flags = 0;                    /* By default - non-persistent, qos=0, not retained */
    if (hdr->Flags & ismMESSAGE_FLAGS_RETAINED)
        jhdr->flags |= ACTFLAG_Retain;           /* Set the retain flag */
    if (hdr->Flags & ismMESSAGE_FLAGS_PROPAGATE_RETAINED)
        jhdr->flags |= ACTFLAG_RetainPublish;    /* Set the publish flag as the propagate flag */
    if (hdr->Persistence & ismMESSAGE_PERSISTENCE_PERSISTENT) {
        jhdr->flags |= ACTFLAG_Persistent;       /* Set the persistent flag */
        if (hdr->Reliability == ismMESSAGE_RELIABILITY_AT_LEAST_ONCE)
            jhdr->flags |= 0x01;          /* Set QoS=1 */
        else
            jhdr->flags |= 0x02;          /* Set QoS=2 */
    } else {
        /* Non-persistent, can be QoS 0 or 1 */
        if (hdr->Reliability == ismMESSAGE_RELIABILITY_AT_LEAST_ONCE) {
            jhdr->flags |= 0x01;        /* Set QoS=1 */
        }
    }
    jhdr->dup = hdr->RedeliveryCount;              /* Set the redelivery flag */

    /*
     * Map message type
     */
    switch (hdr->MessageType) {
    case MTYPE_BytesMessage:   jhdr->bodytype = MTYPE_BytesMessage;   break;
    case MTYPE_Message:
    case MTYPE_MQTT:           jhdr->bodytype = MTYPE_Message;        break;
    case MTYPE_MapMessage:     jhdr->bodytype = MTYPE_MapMessage;     break;
    case MTYPE_ObjectMessage:  jhdr->bodytype = MTYPE_ObjectMessage;  break;
    case MTYPE_StreamMessage:  jhdr->bodytype = MTYPE_StreamMessage;  break;
    case MTYPE_TextMessage:
    case MTYPE_TextMessageNull:
    case MTYPE_MQTT_Text:      jhdr->bodytype = MTYPE_TextMessage;    break;

    case MTYPE_MQTT_TextObject:
        jhdr->bodytype = MTYPE_MapMessage;
        needconvert = 1;
        break;

    case MTYPE_MQTT_TextArray:
        jhdr->bodytype = MTYPE_StreamMessage;
        needconvert = 1;
        break;

    case MTYPE_MQTT_Binary:
        switch (pobj->convertType) {
        /* Determine text or bytes message by whether the bytes are valid UTF-8 and have no prohibited control characters */
        case CT_Unknown:
        case CT_Auto:
            jhdr->bodytype = ism_common_validUTF8NoCC(bodyp, bodylen) >= 0 ?
                    MTYPE_TextMessage : MTYPE_BytesMessage;
            break;
        case CT_Text:
            /* Force the message to a TextMessage, even if the body is not valid UTF-8 */
            jhdr->bodytype = MTYPE_TextMessage;
            break;
        case CT_Bytes:
            jhdr->bodytype = MTYPE_BytesMessage;
            break;
        }
        break;

    default:
        if (pobj->convertType == CT_Text) {
            /* Force the message to a TextMessage, even if the body is not valid UTF-8 */
            jhdr->bodytype = MTYPE_TextMessage;
        } else {
            jhdr->bodytype = MTYPE_BytesMessage;
        }
        break;
    }


    /*
     * Fill in the JMS header
     */
    buf.buf = xbuf;
    buf.used = OffsetOf(ism_protocol_action_t, buf);
    buf.len  = sizeof(xbuf);
    buf.inheap = 0;

   /*
    * Add order ID header field, if requested
    */
   if (consumer->orderIdRequested) {
       jhdr->hdrcount++;
       ism_protocol_putLongValue(&buf, hdr->OrderId);
   }

    /*
     * Copy the properties
     */
    ism_protocol_putMapValue(&buf, propp, proplen);

    /*
     * Copy the body
     */
    if (!needconvert) {
        if (bodylen == 0 && hdr->MessageType == MTYPE_TextMessageNull) {
            ism_protocol_putNullValue(&buf);
        } else {
            ism_protocol_putByteArrayValue(&buf, bodyp, bodylen);
        }
    } else {
        switch (hdr->MessageType) {
        case MTYPE_MQTT_TextObject:
             convertJSONBody(&buf, bodyp, bodylen, 0);
             break;
        case MTYPE_MQTT_TextArray:
             convertJSONBody(&buf, bodyp, bodylen, 1);
             break;
        default:
             break;
        }
    }

    /*
     * If the cache count within the delivery thread is exceeded, request a resume
     */
    if (__sync_add_and_fetch(&consumer->inBatch, 1) > consumer->cacheSize) {
         ((ism_protocol_action_t *)buf.buf)->hdr.flags |= ACTFLAG_Suspended;
         /* Tell the engine to suspend message delivery */
         ism_engine_suspendMessageDelivery(consumerh, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
         __sync_fetch_and_or(&consumer->suspended, SUSPENDED_BY_PROTOCOL);
         returncode = false;
    }

    /*
     * Send the message.
     * In ACK or transaction mode, save the delivery handle
     */
    if (__builtin_expect((!consumer->noack || session->transaction), 1)) {
        pthread_spin_lock(&session->lock);
        if (__builtin_expect((deliveryh != 0), 1)) {
            ism_undelivered_message_t * msg = session->freeMsgs;
            if (msg) {
                session->freeMsgs = msg->next;
            } else {
                pthread_spin_unlock(&session->lock);
                msg = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,158),sizeof(ism_undelivered_message_t));
                pthread_spin_lock(&session->lock);
            }
            msg->deliveryHandle = deliveryh;
            msg->consumer = consumer;
            /*
             * Pointers have to be accessed from the copy,
             * since consumer may have been freed when subscribing with existing
             * subscription name.
             * msg->consumer is OK, since it is only used for NULL-check and for
             * comparison against existing consumer pointer.
             */
            msg->subName = consumer->subName;
            msg->next = NULL;
            msgid = session->seqnum++;
            msg->msgID = msgid;
            msg->prev = session->incompMsgTail;
            if (session->incompMsgTail) {
                session->incompMsgTail->next = msg;
            } else {
                session->incompMsgHead = msg;
            }
            session->incompMsgTail = msg;
            session->incompMsgCount++;
            consumer->incompMsgCount++;
        } else {
            msgid = session->seqnum++;
        }
        ((ism_protocol_action_t *)buf.buf)->hdr.msgid = endian_int64(msgid);
        rc = transport->send(transport, buf.buf + hdrpos, buf.used - hdrpos, 0, flags);
        pthread_spin_unlock(&session->lock);
    } else {
        /*
         * Tell the engine that the message has been consumed, if necessary
         */
        if (deliveryh) {
            xUNUSED int zrc = ism_engine_confirmMessageDelivery(session->handle, NULL,
                    deliveryh, ismENGINE_CONFIRM_OPTION_CONSUMED, NULL, 0, NULL);
        }

        /*
         * Client consumers depend on sequence number.
         */
        msgid = session->seqnum++;
        ((ism_protocol_action_t *)buf.buf)->hdr.msgid = endian_int64(msgid);

        rc = transport->send(transport, buf.buf + hdrpos, buf.used - hdrpos, 0, flags);
    }
    if (rc == SRETURN_SUSPEND) {
        __sync_fetch_and_or(&consumer->suspended, SUSPENDED_BY_TRANSPORT);
        if (returncode) {
            ism_engine_suspendMessageDelivery(consumerh, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
            returncode = false;
        }
    }

    __sync_add_and_fetch(&transport->write_msg, 1);
    __sync_add_and_fetch(&transport->listener->stats->count[transport->tid].write_msg, 1);

    /*
     * Cleanup
     */
cleanup:
    ism_engine_releaseMessage(msgh);

    /*
     * Cleanup
     */
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
    if (!returncode) {
        TRACEL(8, transport->trclevel, "replyReceive - suspend consumer: connect=%u consumer=%d(%p) suspended=%d\n",
              transport->index, consumer->which, consumer->handle, consumer->suspended);
    }
    return returncode;
}

/*
 * As each individual session is stopped, ack messages that were delivered
 * to the client application and mark the remaining messages as undelivered.
 */
static void replyStopSession(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;

    if (rc) {
        ism_common_setError(rc);
        replyAction(rc, NULL, action);
        return;
    }

    if (action->valcount) {
        /* The stop request included an array of consumers with last delivered
         * messages for each.  We need to check to see whether there
         * are pending acks required for these messages before we go
         * on to stop the connection and nack undelivered messages.
         */
        uint32_t count = action->flag;
        uint64_t * acksqn = (uint64_t *)action->xbuf.buf;
        action->valcount = 0;
        action->recordCount = 1;

        if (!(__builtin_expect((acksqn == NULL),0))) {
            for (int i = 0; i < count; i++) {
                ism_jms_prodcons_t * consumer = getProdcons(transport,acksqn[i++]);
                if (consumer) {
                    /* Only ack the last delivered message for each consumer
                     * if the ack mode is NOT client ack.
                     */
                    if (consumer->session->ackmode
                         && (consumer->session->ackmode != 2))  {
                        if(consumer->msglistener) {
                            rc = ackDeliveredMessages(transport, consumer->session, ismENGINE_CONFIRM_OPTION_CONSUMED, acksqn[i], acksqn[i], 0, NULL, action, replyStopSession);
                        } else {
                            rc = ackDeliveredMessages(transport, consumer->session, ismENGINE_CONFIRM_OPTION_CONSUMED, 0, acksqn[i], 2, &acksqn[i-1], action, replyStopSession);
                        }
                        if(rc == ISMRC_AsyncCompletion)
                            __sync_add_and_fetch(&action->recordCount,1);
                    }
                    if(clearConsumerUndeliveredMessage(consumer->session, consumer,acksqn[i],0, action, replyStopSession) == ISMRC_AsyncCompletion)
                    	__sync_add_and_fetch(&action->recordCount,1);
                }
            }
            if(action->xbuf.inheap)
                ism_common_freeAllocBuffer(&action->xbuf);
        }

    }
    if(__sync_sub_and_fetch(&action->recordCount,1) > 0)
    	return;
    replyAction(0, NULL, action);
    return;
}

/*
 * Rollback session request processing. All unconfirmed messages are added to
 * the transaction and then rollback is executed in the engine.
 * This function can be called directly or via a callback from
 * ism_engine_stopMessageDelivery.
 */
static void replyRollbackSession(int32_t rc, void * handle, void * vaction) {
    ismEngine_DeliveryHandle_t array[1024];
    ismEngine_DeliveryHandle_t * msgs2free = array;
    int counter = 0;
    int size = 1024;
    ism_protocol_action_t * action = vaction;
    ism_jms_session_t * session = action->session;
    ism_undelivered_message_t * undelMsg;
    uint64_t    lastID = 0;

    if (rc) {
        ism_common_setError(rc);
        replyAction(rc, NULL, action);
        return;
    }

    if (!(__sync_fetch_and_or(&session->suspended, SUSPENDED_BY_PROTOCOL))) {
        rc = ism_engine_stopMessageDelivery(session->handle,
                action, action->actionsize, replyRollbackSession);
    }

    if(rc == ISMRC_AsyncCompletion)
        return;

    if(action->recordCount == -1){
        /*
         * Mark all undelivered messasges as NOT_DELIVERED which will cause them to be
         * redelivered with the same sequence number.
         */
        pthread_spin_lock(&session->lock);
        undelMsg = session->incompMsgHead;
        while (undelMsg) {
            ism_undelivered_message_t * msg = undelMsg;
            undelMsg = msg->next;
            if (msg->deliveryHandle) {
            	if(UNLIKELY(counter == size)) {
            		size = (size << 1);
            		if( msgs2free == array) {
            			msgs2free = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,159),sizeof(ismEngine_DeliveryHandle_t)*size);
            			memcpy(msgs2free,array,sizeof(array));
            		} else {
            			msgs2free = ism_common_realloc(ISM_MEM_PROBE(ism_memory_protocol_misc,160),msgs2free,(sizeof(ismEngine_DeliveryHandle_t)*size));
            		}
            	}
                msgs2free[counter++] = msg->deliveryHandle;
            }
            lastID = msg->msgID;
            freeUndeliveredMessage(session, msg);
        }
        pthread_spin_unlock(&session->lock);
        action->recordCount = lastID;
        if (counter)
            rc = ism_engine_confirmMessageDeliveryBatch(session->handle,NULL,counter,msgs2free,
            		ismENGINE_CONFIRM_OPTION_NOT_DELIVERED, action, action->actionsize, replyRollbackSession);
        if(rc) {
            if (rc == ISMRC_AsyncCompletion)
            	return;
            ism_common_shutdown(1);
        }
    }
    /* All messages are in transaction, rollback */
    pthread_spin_lock(&session->lock);
    handle = session->transaction;
    session->transaction = NULL;
    pthread_spin_unlock(&session->lock);
    rc = ism_engine_rollbackTransaction(session->handle,
            handle, action, sizeof(ism_protocol_action_t), createTransaction);
    if (rc != ISMRC_AsyncCompletion)
        createTransaction(rc, NULL, action);
}

/*
 * Check counts for unsubscribe.
 */
static void checkUnsub(ismEngine_SubscriptionHandle_t subHandle, const char * pSubName, const char *pTopicString,
        void * properties, size_t propertiesLength, const ismEngine_SubscriptionAttributes_t *pSubAttributes, uint32_t consumerCount, void * vaction) {
    ism_protocol_action_t * action = vaction;
    if (pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE)
        action->recordCount = consumerCount;
}

/*
 * Unsubscribe durable subscription.
 */
static void replyUnsubscribeDurable(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t * pobj = (jmsProtoObj_t*)transport->pobj;
    char * subName;
    const char * clientName = transport->name;
    void * clientState;
    ism_transport_t * clientTrans = transport;
    int  sharedNamespace;

    if (rc) {
        ism_common_setError(rc);
        replyError(rc, NULL, action);
        return;
    }

    subName = (action->valcount > 0) ? action->values[0].val.s : NULL;   /* Subscription name */
    sharedNamespace = (action->valcount > 1) ? action->values[1].val.i : 0;  /* Use shared namespace */

    if (subName) {
        if (transport->pobj->isGenerated || sharedNamespace) {
            clientState = client_Shared;
            clientName = "__Shared";
            clientTrans = transport_Shared;
            action->shared = SHARED_Global;
        } else {
            clientState = pobj->handle;
            SHARED_SUB_NAME_DURABLE(subName);
            action->shared = SHARED_False;
        }
        action->recordCount = -1;
        xUNUSED int zrc = ism_engine_listSubscriptions(clientState, subName, action, checkUnsub);
        if (action->recordCount != 0) {
            TRACEL(4, transport->trclevel, "Unable to unsubscribe: client=%s name=%s count=%d\n", clientName, subName, (int32_t)action->recordCount);
            rc = ((int32_t)action->recordCount) < 0 ? ISMRC_NotFound : ISMRC_DestinationInUse;
            ism_common_setError(rc);
            replyAction(rc, (void *)(uintptr_t)action->recordCount, action);
        } else {
            action->clientTrans = clientTrans;
            if (!__sync_bool_compare_and_swap(&clientTrans->pobj->subscribeLock, 0, 1)) {
                ism_protocol_action_t * actcopy = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,161),action->actionsize);
                memcpy(actcopy, action, action->actionsize);
                clientTrans->addwork(clientTrans, doUnsubscribe, actcopy);
                return;
            }
            rc = ism_engine_destroySubscription(pobj->handle, subName, clientState, action, action->actionsize, replyAction);
            if (rc != ISMRC_AsyncCompletion) {
                if (rc) {
                    ism_common_setError(rc);
                }
                replyAction(rc, NULL, action);
            }
        }
    } else {
        ism_common_setError(ISMRC_NameNotValid);
        replyAction(ISMRC_NameNotValid, NULL, action);
    }
}

/*
 * Update durable subscription.
 */
static void replyUpdateDurable(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t * pobj = (jmsProtoObj_t*)transport->pobj;
    char * subName;
    char * shared;
    const char * clientName = transport->name;
    void * clientState = pobj->handle;
    ism_transport_t * clientTrans = transport;

    if (rc) {
        ism_common_setError(rc);
        replyError(rc, NULL, action);
        return;
    }

    subName = (action->valcount > 0) ? action->values[0].val.s : NULL;
    shared = (action->valcount > 1) ? action->values[1].val.s : NULL;        /* SubscriptionShared value */
    if (subName) {
    	int i, j, count, flag = 1;
    	ism_prop_t * props;

    	/* Handle shared subscriptions */
        action->shared = SHARED_False;
        if (shared && transport->pobj->client_version >= ImaVersion_1_1) {
            if (!strcmpi(shared, "False"))
                action->shared = SHARED_False;
            else if (!strcmpi(shared, "True"))
                action->shared = transport->pobj->isGenerated ? SHARED_Global : SHARED_True;
            else if (!strcmpi(shared, "NonDurable"))
                action->shared = transport->pobj->isGenerated ? SHARED_GlobalND : SHARED_NonDurable;
            else {
                ism_common_setError(ISMRC_BadClientData);
                replyAction(ISMRC_BadClientData, NULL, action);
                return;
            }
        }

    	if (transport->pobj->isGenerated) {
    		clientName = "__Shared";
    		clientTrans = transport_Shared;
    	}

		switch (action->shared) {
		case SHARED_False:
		case SHARED_True:         SHARED_SUB_NAME_DURABLE(subName);     break;
		case SHARED_NonDurable:   SHARED_SUB_NAME_NONDURABLE(subName);  break;
		case SHARED_Global:       clientState = client_Shared;          break;
		case SHARED_GlobalND:     clientState = client_SharedND;        break;
		}

    	/* Check the subscription */
        action->recordCount = -1;
        xUNUSED int zrc = ism_engine_listSubscriptions(clientState, subName, action, checkUnsub);
        if (action->recordCount < 0) {
            TRACEL(4, transport->trclevel, "Unable to update subscription: client=%s name=%s\n", clientName, subName);
            ism_common_setError(ISMRC_NotFound);
            replyAction(rc, NULL, action);

            return;
        }

        props = ism_common_newProperties(20);

    	/* Copy valid properties into props array and verify
    	 * that all properties in the list are valid (as defined in updateableProperties). */
    	count = sizeof(updateableProperties) / sizeof(char *);
    	for (i = 0; i < action->propcount; i++) {
    		flag = 0;
    		for (j = 0; j < count; j++) {              /* BEAM suppression: loop doesn't iterate */
    			if (!strcmp(action->props[i].name, updateableProperties[j])) {
                    ism_common_setProperty(props, action->props[i].name, &action->props[i].f);
    				flag = 1;
    				break;
    			}
    		}
    		if (!flag) {
    			break;
    		}
    	}
    	if (!flag) {
    		TRACEL(4, transport->trclevel, "Invalid property name=%s connect=%u client=%s name=%s\n",
    				action->props[i].name, transport->index, transport->name, subName);
    		ism_common_freeProperties(props);
    		ism_common_setError(ISMRC_BadClientData);
    		replyAction(ISMRC_BadClientData, NULL, action);
    		return;
    	}

		/*
		 * Only one subscribe at a time can be done on a connection
		 */
    	action->clientTrans = clientTrans;
    	if (!__sync_bool_compare_and_swap(&clientTrans->pobj->subscribeLock, 0, 1)) {
    		ism_protocol_action_t * actcopy = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,162),action->actionsize);
    		memcpy(actcopy, action, action->actionsize);

			ism_common_freeProperties(props);

    		clientTrans->addwork(clientTrans, doUpdate, actcopy);
    		return;
    	}
    	rc = ism_engine_updateSubscription(pobj->handle,
    			subName,
    			props,
    			clientState,
    			action,
    			action->actionsize,
    			replyAction);
    	if (rc != ISMRC_AsyncCompletion) {
    		if (rc) {
    			ism_common_setError(rc);
                TRACEL(4, transport->trclevel, "Unable to update: client=%s name=%s rc=%d\n", clientName, subName, rc);
    		}
    		replyAction(rc, NULL, action);
    	}

   		ism_common_freeProperties(props);
    } else {
        ism_common_setError(ISMRC_NameNotValid);
        replyAction(ISMRC_NameNotValid, NULL, action);
    }
}

static void replyEndGlobalTransaction(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_jms_session_t * session = action->session;
    ismEngine_TransactionHandle_t transaction = session->transaction;
    uint32_t flag = action->flag;
    TRACEL(7, transport->trclevel, "replyEndGlobalTransaction: connect=%u client=%s session=%d transaction=%p flag=%u rc=%d\n",
             transport->index, transport->name, session->which, transaction, flag, rc);
    session->transaction = NULL;
    if(rc == ISMRC_OK) {
        rc = ism_engine_endTransaction(session->handle,
                transaction, flag, action, sizeof(ism_protocol_action_t), replyAction);
    }
    if (rc != ISMRC_AsyncCompletion) {
        replyAction(rc, NULL, action);
    }
    return;
}
