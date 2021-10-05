/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

/*
 * The implementation of delay fully supports the JMS deliveryTime specification including support
 * for persistence and transactions.  This is done by putting persistent and transacted messages
 * onto a topic ($SYS_Delay) and immediately receive the message, but do not confirm it.
 *
 * At the completion of the delay the message is NAKed which will cause it to be redelivered and
 * at that point we sent it to the original destination.  This mechanism means we have only one
 * copy of the message which is kept by the engine and will be recovered after a server restart.
 *
 * For non-persistent, non-transacted messages we put the message into our delay structure and send
 * it when the delay is complete.
 *
 * TODO: For now this code is disabled and we just send the message immediately.  When the code is
 * enabled there are some error handling items which should be implemented.
 */

/*
 * Stats
 */
int64_t g_delayCount = 0;          /* Count of delay entries */
int64_t g_delayMemory = 0;         /* Memory used for delay entries */
int     g_delayDisabled = 0;
static pthread_spinlock_t delaylock;


// #define ALLOW_DELAY
#ifndef ALLOW_DELAY
/*
 * Implementation when delay is disabled.
 */
static int  delayDelivery(ism_jms_session_t * session, int64_t deliveryTime,
        ism_protocol_action_t * action, ismMessageHeader_t * mhdr, size_t * areaSize, void * * areaData) {
    return ISMRC_NotDelivered;
}
#else

/*
 * An entry in the delay queue
 */
struct delayEntry {
    struct delayEntry * next;
    int64_t  deliveryTime;
    struct   delayMessage * dmsg;
    size_t   len;
    ismEngine_DeliveryHandle_t deliveryh;
};

/*
 * A copied message on the delay queue
 */
struct delayMessage {
    struct delayMessage * next;
    ismMessageHeader_t mhdr;
    size_t    areaSize[2];
    char *    areaData[2];
};

static ism_timer_t        delayTimer;
static int64_t            starttime;
static int delayCheck(ism_timer_t key, ism_time_t timestamp, void * userdata);
static void addToDelayList(struct delayEntry * dent);
static struct delayEntry * removeFromDelayList(int64_t time);
static int getDestination(int proplen, const void * propptr, const char * * xdestname);
static void replyPutDelay(int32_t rc, void * handle, void * vaction);

static int64_t first_time;
static int64_t last_time;

static struct delayEntry * first_entry;
static struct delayEntry * last_entry;
static ismEngine_ClientStateHandle_t client_Delay = NULL;
static ismEngine_SessionHandle_t session_Delay = NULL;
static ism_transport_t * transport_Delay = NULL;
/*
 * Delay the message delivery
 */
static int  delayDelivery(ism_jms_session_t * session, int64_t deliveryTime,
    ism_protocol_action_t * action, ismMessageHeader_t * mhdr,
    size_t * areaSize, void * * areaData) {
    size_t len;
    int    domain;
    const char * destname;
    int    rc;
    ismEngine_MessageHandle_t   msgh;

    /* Make sure that topic/queue name is set */
    domain = getDestination((int)areaSize[0], (void *)areaData[0], &destname);
    if (domain == 0) {
        return ISMRC_NotDelivered;
    }

    /*
     * Check if we are authorized to publish or send.
     * We check now since we do not check when we eventually do the send.
     */
    rc = ism_security_validate_policy(transport_Delay->security_context,
           domain == ismDESTINATION_TOPIC ? ismSEC_AUTH_TOPIC : ismSEC_AUTH_QUEUE,
           destname,
           domain == ismDESTINATION_TOPIC ? ismSEC_AUTH_ACTION_PUBLISH : ismSEC_AUTH_ACTION_SEND,
           ISM_CONFIG_COMP_PROTOCOL,
           NULL);
    if (rc) {
        return ISMRC_NotDelivered;
    }

    /*
     * If non-persistent, copy the message directly to the delay list
     */
    if (mhdr->Persistence == ismMESSAGE_PERSISTENCE_NONPERSISTENT && !session->transacted) {
        struct delayEntry * dent;
        struct delayMessage * dmsg;

        len = sizeof(struct delayEntry) + sizeof(struct delayMessage) + areaSize[0] + areaSize[1];
        dent = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,174),1, len);

        dmsg = (struct delayMessage *)(dent+1);
        dent->dmsg = dmsg;
        dent->deliveryTime = deliveryTime;
        dent->len = len;

        dmsg->areaSize[0] = areaSize[0];
        dmsg->areaSize[1] = areaSize[1];
        dmsg->areaData[0] = (char *)(dmsg+1);
        dmsg->areaData[1] = dmsg->areaData[0] + areaSize[0];
        memcpy(dmsg->areaData[0], areaData[0], areaSize[0]);
        memcpy(dmsg->areaData[1], areaData[1], areaSize[1]);
        dmsg->mhdr = *mhdr;

        /* Put message on delay list */
        addToDelayList(dent);
        replyMessage(0, NULL, action);
    } else {
        if (g_delayDisabled)
            return ISMRC_NotDelivered;
        /*
         * Process a persistent message by putting it on the delay queue
         */
        rc = ism_engine_createMessage(mhdr, 2, MsgAreas, areaSize, areaData, &msgh);
        if (rc) {
            replyMessage(rc, NULL, action);
            return rc;
        }
        rc = ism_engine_putMessageOnDestination(session_Delay,
                ismDESTINATION_TOPIC, "$SYS_Delay", session->transaction,
                msgh, action, sizeof(ism_protocol_action_t), replyMessage);
        if (rc != ISMRC_AsyncCompletion) {
            replyMessage(rc, NULL, action);
        }
    }
    return 0;
}


/*
 * Put a message on the delay queue.
 */
static void addToDelayList(struct delayEntry * dent) {
    static int delayRunning = 0;
    pthread_spin_lock(&delaylock);
    if (!delayRunning) {
        delayRunning = 1;
        /* start up the delay timer */
        delayTimer = ism_common_setTimerRate(ISM_TIMER_LOW, delayCheck, NULL, 82, 640, TS_MILLISECONDS);
    }
    /* add the message in first to deliver order */
    if (dent->deliveryTime >= last_time) {
        if (!first_entry) {
            first_entry = dent;
            last_entry = dent;
            first_time = dent->deliveryTime;
            last_time = dent->deliveryTime;
        } else {
            last_entry->next = dent;
            last_time = dent->deliveryTime;
        }
    } else {
        struct delayEntry * insert = NULL;
        struct delayEntry * entp = first_entry;
        while (entp->deliveryTime <= dent->deliveryTime) {
            insert = entp;
            entp = entp->next;
        }
        if (insert == NULL) {
            dent->next = first_entry;
            first_entry = dent;
            first_time = dent->deliveryTime;
        } else {
            dent->next = entp;;
            insert->next = dent;
        }
    }

    g_delayCount++;
    TRACE(8, "Add message from delay queue. count=%d now=%.3f time=%.3f entry=%p\n", (int)g_delayCount,
            (double)((ism_common_currentTimeNanos() / 1000000)-starttime)/1000.0,
            (dent->deliveryTime-starttime)/1000.0, dent);
    g_delayMemory += dent->len;
    pthread_spin_unlock(&delaylock);
}

/*
 * Remove an entry from the list if it is past its delivery time.
 */
static struct delayEntry * removeFromDelayList(int64_t time) {
    struct delayEntry * ret = NULL;
    pthread_spin_lock(&delaylock);
    if (first_entry && time >= first_time) {
        ret = first_entry;
        first_entry = ret->next;
        if (first_entry == NULL) {
            last_entry = NULL;
            first_time = 0;
            last_time = 0;
        } else {
            first_time = first_entry->deliveryTime;
        }

        g_delayMemory -= ret->len;
        g_delayCount--;
        TRACE(8, "Remove message from delay queue. count=%d entry=%p\n", (int)g_delayCount, ret);
    }

    pthread_spin_unlock(&delaylock);
    return ret;
}


/*
 * Reply with a received message.
 */
static bool delayReceive (
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
        void *                     vaction) {
    int64_t now = ism_common_currentTimeNanos() / 1000000;
    ism_field_t  fld;
    int          rc;
    struct delayEntry dent = {0};
    int          domain;
    const char * destname;

    assert(msgh);

    ism_actionbuf_t xprops = {0};
    xprops.buf = (char *)areaptr[0];
    xprops.len = areasize[0];
    xprops.used = xprops.len;

    ism_findPropertyNameIndex(&xprops, ID_DeliveryTime, &fld);
    dent.deliveryh = deliveryh;
    if (fld.type == VT_Long)
        dent.deliveryTime = fld.val.l;
    dent.len = sizeof(struct delayEntry);

    /* If past the delivery time, send the message */
    if (dent.deliveryTime <= now || !deliveryh) {
        /*
         * Create the message
         */
        rc = ism_engine_createMessage(hdr, 2, MsgAreas, areasize, areaptr, &msgh);
        if (rc) {
            replyPutDelay(rc, NULL, &dent);
        } else {
            domain = getDestination((int)areasize[0], areaptr[0], &destname);
            /*Put the message to the engine */
            if (domain && session_Delay) {
                rc = ism_engine_putMessageOnDestination(session_Delay, domain, destname,
                         NULL, msgh, &dent, dent.len, replyPutDelay);
                if (rc != ISMRC_AsyncCompletion) {
                    replyPutDelay(rc, NULL, &dent);
                }
            } else {
                /* Should not ever happen */
                TRACE(1, "Unable to put delayed message: domain=%d\n", domain);
            }
        }
    } else {
        /* Put the message on the delay queue */
        struct delayEntry * denta = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,175),sizeof(struct delayEntry));
        *denta = dent;
        addToDelayList(denta);
    }
    return true;
}


/*
 * Reply on completion of put of delay message
 */
static void replyPutDelay(int32_t rc, void * handle, void * vaction) {
    struct delayEntry * dent = (struct delayEntry *)vaction;
    if (rc) {
        /* Ignore informational return codes */
        if (rc != ISMRC_SomeDestinationsFull &&
            rc != ISMRC_NoMatchingDestinations &&
            rc != ISMRC_NoMatchingLocalDestinations) {
            TRACE(5, "Unable to send delayed message: rc=%d\n", rc);
            /* TODO:  fix this */
        }
    }
    if (dent->deliveryh) {
        ism_engine_confirmMessageDelivery(session_Delay, NULL, dent->deliveryh,
                ismENGINE_CONFIRM_OPTION_CONSUMED, NULL, 0, NULL);
    }
}


/*
 * Get domain and destination
 */
static int getDestination(int proplen, const void * propptr, const char * * xdestname) {
    int domain = 0;
    const char * destname = NULL;
    ism_actionbuf_t xprops = {0};
    ism_field_t fld;

    xprops.buf = (char *)propptr;
    xprops.len = proplen;
    xprops.used = proplen;
    ism_findPropertyNameIndex(&xprops, ID_Topic, &fld);
    if (fld.type == VT_String) {
        domain = ismDESTINATION_TOPIC;
        destname = fld.val.s;
    } else {
        ism_findPropertyNameIndex(&xprops, ID_Queue, &fld);
        if (fld.type == VT_String) {
            domain = ismDESTINATION_QUEUE;
            destname = fld.val.s;
        }
    }
    if (xdestname)
        *xdestname = destname;
    return domain;
}

/*
 * Delay timer tick
 */
static int delayCheck(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ismEngine_MessageHandle_t msgh;
    int   domain = 0;
    const char * destname;
    int   rc;

    int64_t now = timestamp/1000000;

    struct delayEntry * dent = removeFromDelayList(now);
    struct delayMessage * dmsg;

    while (dent) {
        dmsg = dent->dmsg;
        if (dmsg) {
            /* Create the message  */
            rc = ism_engine_createMessage(&dmsg->mhdr, 2, MsgAreas, dmsg->areaSize, (void * *)dmsg->areaData, &msgh);
            if (rc) {
                replyPutDelay(rc, NULL, dent);
            } else {
                domain = getDestination((int)dmsg->areaSize[0], dmsg->areaData[0], &destname);
                /* Put the message to the engine  */
                if (domain && session_Delay) {
                    rc = ism_engine_putMessageOnDestination(session_Delay, domain, destname,
                             NULL, msgh, dent, dent->len, replyPutDelay);
                    if (rc != ISMRC_AsyncCompletion) {
                        replyPutDelay(rc, NULL, dent);
                    }
                } else {
                    /* Should not ever happen */
                    TRACE(1, "Unable to put delayed message: domain=%d\n", domain);
                }
            }
        } else {
            /* NACK the delivery handle, causing it to be redelivered */
            ism_engine_confirmMessageDelivery(session_Delay, NULL, dent->deliveryh,
                    ismENGINE_CONFIRM_OPTION_NOT_DELIVERED, NULL, 0, NULL);
        }
        ism_common_free(ism_memory_protocol_misc,dent);
        dent = removeFromDelayList(now);
    }
    return 1;
}

int ism_tcp_addWork(ism_transport_t * transport, ism_transport_onDelivery_t ondelivery, void * userdata);

static int subFound = 0;

/*
 * Check if the subscription exists
 */
static void delaySubEntry(ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName, const char * oldTopicName,
        void * xproperties, size_t propertiesLength,
        uint32_t subOptions, uint32_t consumerCount, void * vaction) {
    subFound++;
}

/*
 * The creation of the consumer is complete.
 * If this fails, disable delay for persistent and transactions.
 */
static void delayCompleteCreate(int32_t rc, void * handle, void * vaction) {
    if (rc) {
        /* TODO: log */
        TRACE(1, "Delay is disabled because the consumer could not be created.\n");
        g_delayDisabled = 1;
    }
    ism_engine_startMessageDelivery(session_Delay, 0, NULL, 0, NULL);
}

/*
 * Create the consumer for the delay topic
 */
static void delayCreateConsumer(int32_t rc, void * handle, void * vaction) {
    ismEngine_ConsumerHandle_t consumerh;
    if (rc) {
        /* TODO log */
        TRACE(1, "Delay is disabled because the subscription could not be created.\n");
        g_delayDisabled = 1;
    } else {
        int  mode = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
        int consOpt = ismENGINE_CONSUMER_OPTION_NONE;
        rc = ism_engine_createConsumer(session_Delay, ismDESTINATION_SUBSCRIPTION,
                "__Delay", mode, client_Delay,
                NULL, 0, delayReceive,
                NULL, consOpt, &consumerh,
                NULL, 0, delayCompleteCreate);

        if (rc != ISMRC_AsyncCompletion) {
            delayCompleteCreate(rc, consumerh, NULL);
        }
    }
}
#endif

int ism_tcp_addWork(ism_transport_t * transport, ism_transport_onDelivery_t ondelivery, void * userdata);

/*
 * Initialize the delay system.
 *
 * Create a delay queue.  What we do with this queue is to immediately receive each
 * message and keep them with an undelivered handle.  When we want to delivery the message
 * we NAK the message which will re-send the message.  When a message is received past its
 * delivery time, we then publish it to the ultimate destination.
 *
 * We write the the ultimate destination without authority checks because we checked at the
 * time of the publish.  The theory is that
 */
extern int ism_protocol_JmsDelayInit(void) {
    int  rc = 0;
    /* Init delay lock */
    pthread_spin_init(&delaylock, 0);
#ifdef ALLOW_DELAY
    ism_endpoint_t * endpoint;
    ism_transport_t * transport;
    ism_field_t  field;

    /* Create a fake endpoint.  This endpoint is not linked in so will not show up anywhere */
    endpoint = ism_transport_createEndpoint("__Delay", "__Delay", "tcp", "*", NULL, NULL, NULL, 1);
    endpoint->enabled = 1;
    endpoint->protomask = PMASK_AnyInternal;
    endpoint->transmask = TMASK_AnyTrans;
    endpoint->config_time = ism_common_currentTimeNanos();
    endpoint->rc = 0;

    /* Create a fake JMS connection */
    transport = ism_transport_newTransport(endpoint, 256, 0);
    transport->addwork = ism_tcp_addWork;
    transport->protocol = "jms";
    jmsConnection(transport);
    transport_Delay = transport;

    rc = ism_security_create_context(ismSEC_POLICY_CONNECTION, transport, &transport->security_context);
    if (rc) {
        TRACE(1, "Unable to create session: name=__Delay rc=%d\n", rc);
    } else {
        /*
         * Create some fake engine objects
         */
        rc = ism_engine_createClientState("__Delay", PROTOCOL_ID_JMS, ismENGINE_CREATE_CLIENT_OPTION_NONE, NULL, NULL,
                transport->security_context, &client_Delay, NULL, 0, NULL);
        transport->pobj->handle = client_Delay;
        if (rc == ISMRC_ResumedClientState) {    /* Ignore resumed statte return code */
            rc = 0;
        }
        if (rc) {
            TRACE(1, "Unable to create client state: name=__Delay rc=%d", rc);
        } else {
            int options = ismENGINE_CREATE_SESSION_OPTION_NONE;
            rc = ism_engine_createSession(client_Delay, options, &session_Delay, NULL, 0, NULL);
            if (rc) {
                TRACE(1, "Unable to create session: name=__Delay rc=%d", rc);
            }
        }
    }

    /*
     * Create the subscription and consumer for the delay topic
     */
    subFound = 0;
    rc = ism_engine_listSubscriptions(client_Delay, "__Delay", NULL, delaySubEntry);
    if (subFound == 0) {
        int  mode = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;

        ism_prop_t * cprops = ism_common_newProperties(1);
        field.type  = VT_Integer;
        field.val.i = 4000000;   /* 4 million */
        ism_common_setProperty(cprops, "MaxMessages", &field);

        rc = ism_engine_createSubscription(client_Delay, "__Delay", cprops, ismDESTINATION_TOPIC,
                "$SYS_Delay", mode, client_Delay, NULL, 0, delayCreateConsumer);
        ism_common_freeProperties(cprops);

        if (rc != ISMRC_AsyncCompletion) {
            delayCreateConsumer(rc, NULL, NULL);
        }
    } else {
        delayCreateConsumer(0, NULL, NULL);
    }

    starttime = ism_common_currentTimeNanos() / 1000000;

#endif
    return rc;
}
