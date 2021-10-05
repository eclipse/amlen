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

/* TODO: ismc does not support password obfuscation */

/*
 * @file ismc.c
 * Implement the ISM C client
 */
#include <ismc_p.h>
#include <ismc.h>

/*
 * Forward references
 */

static int checkConnected(ismc_connection_t * session);
static int checkDestination(ismc_destination_t * dest);
static int ackTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata);
static void ismc_removeSessionFromConnection(ismc_session_t * session);
static void ismc_addProducerToSession(ismc_producer_t * prod);
static void ismc_addSessionToConnection(ismc_session_t * session);
static void ismc_removeProducerFromSession(ismc_producer_t * prod);
static void ismc_removeConsumerFromSession(ismc_consumer_t * cons);
static int ismc_closeConsumerInternal(ismc_consumer_t * consumer);
static int ismc_closeProducerInternal(ismc_producer_t * producer);
static int parseReplyGeneric(action_t * action);
static int parseReplyConsumer(action_t * action);
static void ismc_resetThread(void * cleanup_parm);

static pthread_mutex_t * corrObjLock = NULL;
static pthread_mutex_t * actThLock = NULL;
static pthread_mutex_t * msgIdLock = NULL;
static action_t * * corrObj = NULL;
static int * activeThreads = NULL;
static int MAX_ACTION_ID = 0;
static __thread int threadId = -1;

static pthread_once_t ismcInitialized = PTHREAD_ONCE_INIT;

static void ismc_init(void) {
	char * maxImsThreads;

    msgIdLock = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,14),sizeof(pthread_mutex_t));
    pthread_mutex_init(msgIdLock, NULL);

    corrObjLock = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,15),sizeof(pthread_mutex_t));
    pthread_mutex_init(corrObjLock, NULL);

    actThLock = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,16),sizeof(pthread_mutex_t));
    pthread_mutex_init(actThLock, NULL);

    maxImsThreads = getenv("ISMMaxJMSThreads");
    if (maxImsThreads != NULL) {
        MAX_ACTION_ID = atoi(maxImsThreads);
    }
    if (MAX_ACTION_ID <= 0) {
        MAX_ACTION_ID = 4096;
    }
    corrObj = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,17),sizeof(action_t *), (MAX_ACTION_ID + 1));
    activeThreads = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,18),sizeof(int), (MAX_ACTION_ID + 1));

    ismc_allocateDeliveryThreads();

    ism_common_initTimers();
}

/*
 * Create a connection.
 *
 * This creates a connection object but does not establish the connection.
 * After making this call, it is normally necessary to set the connection properties
 * before making the connection.
 *
 * @return A connection object
 */
ismc_connection_t * ismc_createConnection(void) {
    ismc_connection_t * connect;

    (void) pthread_once(&ismcInitialized, ismc_init);

    /*
     * First check if unique thread ID is available
     */
    if (ismc_getThreadId() < 0) {
    	/* Report an error */
        ismc_setError(ISMRC_Error,
                "Failed to create session: "
                "the number of JMS client threads within a single process cannot exceed %d\n", MAX_ACTION_ID);

    	return NULL;
    }

    connect = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,19),1, sizeof(ismc_connection_t));
    connect->h.id = OBJID_Connection;
    pthread_spin_init(&connect->h.lock, 0);
    ismc_setStringProperty(connect, "ObjectType", "common");

    pthread_mutex_init(&connect->lock, NULL);
    pthread_mutex_init(&connect->senderMutex, NULL);

    connect->rcvActions = ism_common_createHashMap(20,HASH_INT32);
    connect->consumers = ism_common_createHashMap(10,HASH_INT32);

    return connect;
}

/*
 * Create the connection with options.
 *
 * This is a convenience method which takes a set of common connection properties and sets them.
 * Any of these values can be NULL and the associated property is not set.
 * @param clientid  The client identity (ClientID).  Defaults to the hostname.
 * @param protocol  The communications protocol (Protocol).  Defaults to "tcp".
 * @param server    The messaging server (Server).  Defaults to "127.0.0.1".
 * @param port      The port of the messaging server (Port).  Defaults to 16102.
 * @returns The connection object
 */
ismc_connection_t * ismc_createConnectionX(const char * clientid, const char * protocol, const char * server, int port) {
    ismc_connection_t * connect = ismc_createConnection();
    if (clientid)
        ismc_setStringProperty(connect, "ClientID", clientid);
    if (protocol)
        ismc_setStringProperty(connect, "Protocol", protocol);
    if (server)
        ismc_setStringProperty(connect, "Server", server);
    if (port)
        ismc_setIntProperty(connect, "Port", port, VT_Integer);

    return connect;
}


/*
 * Close the connection.
 *
 * After the close, the conection cannot be used for communications.
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_closeConnection(ismc_connection_t * connect) {
    int rc = 0;

    pthread_mutex_lock(&connect->lock);

    if (connect->h.state != OBJSTATE_Closed) {
        if (connect->sessions.array != NULL) {
            int i;
            for (i = 0; i < connect->sessions.numElements; i++) {
                if (((ismc_session_t * *)connect->sessions.array)[i] != NULL) {
                    ismc_closeSession(((ismc_session_t * *)connect->sessions.array)[i]);
                    ((ismc_session_t * *)connect->sessions.array)[i] = NULL;
                }
            }
            connect->sessions.numElements = 0;
            ism_common_free(ism_memory_ismc_misc,connect->sessions.array);
            connect->sessions.array = NULL;
        }

        ism_common_destroyHashMap(connect->consumers);
        connect->consumers = NULL;

        pthread_spin_lock(&connect->h.lock);
        connect->h.state = OBJSTATE_Closed;
        pthread_spin_unlock(&connect->h.lock);
        ismc_disconnect(connect);

        ism_common_destroyHashMap(connect->rcvActions);
        connect->rcvActions = NULL;

        pthread_mutex_destroy(&connect->senderMutex);
    }

    pthread_mutex_unlock(&connect->lock);

    return rc;
}


/*
 * Start the connection.

 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_startConnection(ismc_connection_t * connect) {
    action_t * act;
    int rc;

    rc = checkConnected(connect);
    if (rc == ISMRC_NotConnected || rc == ISMRC_Closed) {
        rc = ismc_connect(connect);
        if (rc) {
            return rc;
        }
    } else if (rc) {
        /* Connection object is not valid */
        return rc;
    }

    pthread_mutex_lock(&connect->lock);

    act = ismc_newAction(connect, NULL, Action_startConnection);
    ism_protocol_putMapProperties(&act->buf, connect->h.props);
    rc = ismc_request(act, 1);

    ismc_freeAction(act);

    pthread_spin_lock(&connect->h.lock);
    connect->isStarted = 1;
    pthread_spin_unlock(&connect->h.lock);

    pthread_mutex_unlock(&connect->lock);

    return rc;
}

/*
 * Stop the connection.
 *
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_stopConnection(ismc_connection_t * connect) {
    action_t * act;
    int rc;

    if (connect == NULL) {
        return ismc_setError(ISMRC_NullPointer, "The connection is NULL");
    }

    if (connect->h.id != OBJID_Connection) {
        return ismc_setError(ISMRC_ObjectNotValid, "Input is not a valid connection");
    }

    pthread_mutex_lock(&connect->lock);

    if (!connect->isStarted) {
    	pthread_mutex_unlock(&connect->lock);
        return 0;
    }

	pthread_spin_lock(&connect->h.lock);
    connect->isStarted = 0;
    pthread_spin_unlock(&connect->h.lock);

    rc = checkConnected(connect);
    if (rc) {
    	pthread_mutex_unlock(&connect->lock);
        return ismc_setError(rc, "Not connected");
    }

    act = ismc_newAction(connect, NULL, Action_stopConnection);
    rc = ismc_request(act, 1);
    ismc_freeAction(act);

    pthread_mutex_unlock(&connect->lock);

    return rc;
}

/*
 * Free up all resources associated with the ISM messaging object.
 *
 * @param connect  An ISM messaging object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_free(void * object) {
    if (object) {
        if ((((ism_obj_hdr_t *)object)->id & 0x7fffff00) == OBJID_ISM) {
            /* Consumer can be freed only if in-use count is not positive */
            if (((ism_obj_hdr_t *)object)->id == OBJID_Connection) {
                ismc_closeConnection((ismc_connection_t *)object);    /* Close the connection if we try to free it */
            }
            if (((ism_obj_hdr_t *)object)->id != OBJID_Consumer) {
                ismc_freeObject(object);
            } else {
                ismc_consumer_t * consumer = object;
                /*
                 * Don't free if ismc_free was already called for this
                 * consumer. Otherwise free if the message count is 0.
                 */
                pthread_spin_lock(&consumer->h.lock);
                if (consumer->h.state != OBJSTATE_Freed) {
                    consumer->h.state = OBJSTATE_Freed;
                    pthread_spin_unlock(&consumer->h.lock);

                    pthread_mutex_lock(&consumer->lock);
                    if (consumer->msgCount == 0) {
                    	pthread_mutex_unlock(&consumer->lock);
                        ismc_freeObject(object);
                    } else {
                    	pthread_mutex_unlock(&consumer->lock);
                    }
                } else {
                	pthread_spin_unlock(&consumer->h.lock);
                }
            }
        }
    }
    return 0;
}


/*
 * Create a session.
 *
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
ismc_session_t * ismc_createSession(ismc_connection_t * connect, int transacted, int ackmode) {
    action_t * act;
    ism_field_t field;
    int rc = checkConnected(connect);
    if (rc) {
        return NULL;
    } else {
        int i;
        const char * name;
        ismc_session_t * session;

        /*
         * First check if unique thread ID is available
         */
        if (ismc_getThreadId() < 0) {
        	/* Report an error */
            ismc_setError(ISMRC_Error,
                    "Failed to create session: "
                    "the number of JMS client threads within a single process cannot exceed %d\n", MAX_ACTION_ID);

        	return NULL;
        }

        session = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,21),1, sizeof(ismc_session_t));

        session->h.id = OBJID_Session;
        session->h.state = OBJSTATE_Open;
        pthread_spin_init(&session->h.lock, 0);
        pthread_mutex_init(&session->lock, NULL);
        pthread_mutex_init(&session->deliverLock, NULL);
        session->transacted = transacted?ISMC_LOCAL_TRANSACTION:0;
        if (!transacted) {
            session->ackmode = ackmode;
        }
        if (transacted && ackmode == SESSION_TRANSACTED_GLOBAL) {
        	session->transacted = ISMC_GLOBAL_TRANSACTION;
        }
        session->connect = connect;
        session->deliveryThreadId = -1;

        /* Copy properties from connection */
        for (i = 0; ism_common_getPropertyIndex(connect->h.props, i, &name) == 0; i++) {
            ism_common_getProperty(connect->h.props, name, &field);
            ismc_setProperty(session, name, &field);
        }

        session->fullSize = ismc_getIntProperty(session->h.props,
        		"ClientMessageCache",
        		ISMC_DEFAULT_MESSAGE_CACHE_FULL_SIZE);
        if (session->fullSize < 0)
        	session->fullSize = 1;
        session->emptySize = session->fullSize / 4;

        rc = ism_common_getProperty(session->h.props, "DisableACK", &field);
        if (rc) {
            session->disableACK = 0;
        } else {
            session->disableACK = field.val.i;
        }


        /*
         * Send the session create to the server
         */
        act = ismc_newAction(connect, session, Action_createSession);

        ism_protocol_putIntValue(&act->buf, 0);                          /* val0 = domain     */
        ism_protocol_putBooleanValue(&act->buf, session->transacted);    /* val1 = transacted */
        ism_protocol_putIntValue(&act->buf, ackmode);                    /* val2 = ackmode    */
        act->hdr.hdrcount = 3;
        ism_protocol_putMapProperties(&act->buf, session->h.props);
        rc = ismc_request(act, 1);
        if (act->rc == 0) {
            ism_protocol_getObjectValue(&act->buf, &field);
            session->sessionid = field.val.i;
        } else {
            ismc_setError(act->rc,
                    "Failed to create session (rc=%d). "
                    "This can happen if there are too many sessions in a single connection.",
                    act->rc);

            ismc_freeAction(act);
            ismc_free(session);
            return NULL;
        }

        /*
         * Create the local transaction if this is transacted and
         * the session is not marked for global transactions.
         */
        if (session->transacted == ISMC_LOCAL_TRANSACTION) {
            act->hdr.action = Action_createTransaction;
            act->hdr.hdrcount = 0;
            act->buf.used = 0;
            act->buf.inheap = 0;
            act->hdr.itemtype = ITEMT_Session;
            act->hdr.item = endian_int32(session->sessionid);
            rc = ismc_request(act, 1);
        }

        if (!rc) {
            ismc_addSessionToConnection(session);
        }

        ismc_freeAction(act);

        if (ackmode == SESSION_DUPS_OK_ACKNOWLEDGE) {
            int ackInterval = ism_common_getIntProperty(session->h.props, "AckInterval", 100);
            session->ackTimer =
                    ism_common_setTimerRate(ISM_TIMER_HIGH, ackTimerTask, session,
                            0, ackInterval, TS_MILLISECONDS);
        }

        return session;
    }
}


/*
 * Close a session.
 *
 * After the close, the connection cannot be used for communications.
 * @param session  The session object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_closeSession(ismc_session_t * session) {
    action_t  * action = NULL;
    int            rc = 0;
    int            i;
    ism_field_t    field;

    rc = checkAndLockSession(session);
    if (rc) {
    	unlockSession(session);
        return rc;
    }

    session->isClosed = 1;

    ismc_removeSessionFromConnection(session);

    if (session->ackTimer) {
        ism_common_cancelTimer(session->ackTimer);
    }

    /*
     * If we could have unacked messages, ack them now
     */
    if (session->ackmode == SESSION_DUPS_OK_ACKNOWLEDGE) {
        ismc_acknowledgeFinal(session);
    }

    if (!rc && session->h.state != OBJSTATE_Closed) {
        /* Clean up producers and consumers */
        if (session->producers.array != NULL) {
            for (i = 0; i < session->producers.numElements; i++) {
                if (((ismc_producer_t * *)session->producers.array)[i] != NULL) {
                    ismc_closeProducerInternal(((ismc_producer_t * *)session->producers.array)[i]);
                    ((ismc_producer_t * *)session->producers.array)[i] = NULL;
                }
            }
            session->producers.numElements = 0;
            ism_common_free(ism_memory_ismc_misc,session->producers.array);
            session->producers.array = NULL;
        }
        if (session->consumers.array != NULL) {
            for (i = 0; i < session->consumers.numElements; i++) {
                if (((ismc_consumer_t * *)session->consumers.array)[i] != NULL) {
                    ismc_closeConsumerInternal(((ismc_consumer_t * *)session->consumers.array)[i]);
                    ((ismc_consumer_t * *)session->consumers.array)[i] = NULL;
                }
            }
            session->consumers.numElements = 0;
            ism_common_free(ism_memory_ismc_misc,session->consumers.array);
            session->consumers.array = NULL;
        }

        session->h.state = OBJSTATE_Closed;

        ism_common_free(ism_memory_ismc_misc,session->acksqn);
        session->acksqn = NULL;

        field.type = VT_Boolean;
        field.val.i = 1;
        ismc_setProperty(session, "isClosed", &field);

        /*
         * Close the transaction. Roll back local transactions.
         */
         if (session->transacted == ISMC_LOCAL_TRANSACTION) {
             action = ismc_newAction(session->connect, session, Action_rollbackSession);
             ismc_writeAckSqns(action, session, NULL);

             rc = ismc_request(action, 1);
             ismc_freeAction(action);
         }

         /*
          * Close session
          */
         if (!rc) {
             action = ismc_newAction(session->connect, session, Action_closeSession);
             rc = ismc_request(action, 1);
             ismc_freeAction(action);
         }

         if (session->ackAction) {
             ismc_freeAction(session->ackAction);
             session->ackAction = NULL;
         }
    }

    unlockSession(session);
    pthread_mutex_destroy(&session->lock);
    pthread_mutex_destroy(&session->deliverLock);

    return rc;
}


/*
 * Commit a session.
 *
 * After the close, the connection cannot be used for communications.
 * @param session  The session object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_commitSession(ismc_session_t * session) {
    action_t * act;
    int rc = checkAndLockSession(session);

    if (!rc && !session->transacted) {
        rc = ismc_setError(ISMRC_ObjectNotValid, "The session must be transacted");
    }
    if (!rc) {
        act = ismc_newAction(session->connect, session, Action_commitSession);
        ismc_writeAckSqns(act, session, NULL);

        rc = ismc_request(act, 1);

        ismc_freeAction(act);

        if (!rc && session->transacted == ISMC_GLOBAL_TRANSACTION) {
            session->globalTransaction = 0;
        }
    }

    unlockSession(session);

    return rc;
}


/*
 * Recover a session.
 *
 * After the close, the connection cannot be used for communications.
 * @param session  The session object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_recoverSession(ismc_session_t * session) {
    int rc = checkAndLockSession(session);
    if (!rc) {
        action_t * act = NULL;

        if (session->ackmode == SESSION_AUTO_ACKNOWLEDGE) {
        	/* Check if there is an unacknowledged message from the last receive call */
        	pthread_mutex_lock(&session->deliverLock);
        	if (session->acksqn_count == 2) { /* There should not be more than 1 unacknowledged message */
        		act = ismc_newAction(session->connect, session, Action_ack);

        		ism_protocol_putIntValue(&act->buf, session->acksqn_count);
        		ism_protocol_putLongValue(&act->buf, 0);
        		act->hdr.hdrcount = 2;

        		ism_protocol_putNullValue(&act->buf);   /* Properties */
        		ism_protocol_putNullValue(&act->buf);   /* Body */
        		ism_protocol_putIntValue(&act->buf, (int)session->acksqn[0]);
        		ism_protocol_putLongValue(&act->buf, session->acksqn[1]);
        		session->acksqn_count = 0;

        		TRACE(7, "Recover session: session=%d sendAck=(%d, %ld)\n",
        				session->sessionid, (int)session->acksqn[0], session->acksqn[1]);
        	}
        	pthread_mutex_unlock(&session->deliverLock);
        	if (act != NULL) {
        		ismc_request(act, 0);
        		ismc_freeAction(act);
        	}
        }

    	act = ismc_newAction(session->connect, session, Action_recover);
        ismc_writeAckSqns(act, session, NULL);
        rc = ismc_request(act, 1);
        if (rc == 0) {
        	ism_field_t field;
        	int i;
        	uint64_t sqn;

            ism_protocol_getObjectValue(&act->buf, &field);
            sqn = field.val.l;
            session->lastDelivered = sqn;
            session->lastAcked = sqn;

            for (i = 0; i < session->consumers.numElements; i++) {
            	((ismc_consumer_t **)session->consumers.array)[i]->lastDelivered = sqn;
            }
        }

        /* Restart the session */
        act->hdr.action = Action_resumeSession;
        act->parseReply = parseReplyGeneric;
        ismc_request(act, 0);

        ismc_freeAction(act);
    }

    unlockSession(session);

    return rc;
}


/*
 * Rollback a session.
 *
 * After the close, the connection cannot be used for communications.
 * @param session  The session object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_rollbackSession(ismc_session_t * session) {
    int i;
    int rc = checkAndLockSession(session);

    if (!rc && !session->transacted) {
        rc = ismc_setError(ISMRC_ObjectNotValid, "The session must be transacted");
    }

    TRACE(7, ">>> rollback: session_id=%d before request: "
    		 "lastAckedMessage=%lu, lastDeliveredMessage=%lu.\n",
		     session->sessionid, session->lastAcked, session->lastDelivered);

    if (!rc) {
    	uint64_t sqn;
        action_t * action = ismc_newAction(session->connect, session, Action_rollbackSession);
        ismc_writeAckSqns(action, session, NULL);

        rc = ismc_request(action, 1);
        if (rc == 0) {
        	ism_field_t field;

            ism_protocol_getObjectValue(&action->buf, &field);
            sqn = field.val.l;
            session->lastDelivered = sqn;
            session->lastAcked = sqn;

            for (i = 0; i < session->consumers.numElements; i++) {
            	((ismc_consumer_t **)session->consumers.array)[i]->lastDelivered = sqn;
            }

            TRACE(7, "<<< rollback: session_id=%d after request: "
            		 "lastAckedMessage=%lu, lastDeliveredMessage=%lu.\n",
        		     session->sessionid, sqn, sqn);

            if (session->transacted == ISMC_GLOBAL_TRANSACTION) {
                session->globalTransaction = 0;
            }
        } else {
            TRACE(7, "<<< rollback: session_id=%d rc=%d.\n", session->sessionid, rc);
        }

        /* Restart the session */
        action->hdr.action = Action_resumeSession;
        action->parseReply = parseReplyGeneric;
        ismc_request(action, 0);

        ismc_freeAction(action);
    }

    unlockSession(session);

    return rc;
}


/**
 * Make a durable subscription.
 *
 * @param session  The session object.
 * @param topic    The topic name
 * @param subname  The subscription name
 * @param selector The selector object
 * @param nolocal  The nolocal flag
 * @return A return code: a pointer to the message consumer.
 */
ismc_consumer_t * ismc_subscribe(ismc_session_t * session, const char * topic, const char * subname,
        const char * selector, int nolocal) {
    size_t sellen = 0;
    char buf[64];
    ismc_consumer_t * consumer = NULL;
    ismRule_t * selectRule = NULL;
    unsigned int rc;

    rc = checkAndLockSession(session);
    if (!rc) {
        if (!topic || !subname) {
            ismc_setError(ISMRC_NoDestination, "Both topic name and subscription name must be specified");
            rc = ISMRC_NoDestination;
        }
    }
    if (!rc) {
        if (selector) {
            /* Compile selector and store it */
            sellen = strlen(selector);
            rc = ism_common_compileSelectRule(&selectRule, NULL, selector);
            if (rc != 0) {
                sellen = 0;
                selectRule = NULL;
            }
        }
    }
    if (!rc) {
        action_t * act;
        int i;
        const char * name;
        ism_field_t field;

        /* Create composite properties from session and destination properties */
        consumer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,25),1, sizeof(ismc_consumer_t)+sellen+1);
        consumer->h.id = OBJID_Consumer;
        consumer->h.state = OBJSTATE_Open;
        pthread_spin_init(&consumer->h.lock, 0);
        consumer->h.props     = ism_common_newProperties(20);
        consumer->h.propcount = 0;
        consumer->session  = session;
        consumer->action   = NULL;
        consumer->domain   = ismc_Topic;
        consumer->nolocal  = nolocal;
        consumer->selector = (const char *)(consumer+1);
        if (!selectRule) {
            consumer->selector = NULL;
        } else {
            strcpy((char *)consumer->selector, selector);
            consumer->selectRule = selectRule;
        }

        /*
         * Engine doesn't support sync ismc_receive yet, so
         * allocate the structure for holding
         * asynchronously received messages that could be requested
         * using ismc_receive.
         */
        consumer->messages = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,26),1, sizeof(ism_common_list));
        ism_common_list_init(consumer->messages, 1, NULL);

        /* Copy properties from session */
        for (i = 0; ism_common_getPropertyIndex(session->h.props, i, &name) == 0; i++) {
            ism_common_getProperty(session->h.props, name, &field);
            ismc_setProperty(consumer, name, &field);
        }

        consumer->disableACK = session->disableACK;

        rc = ism_common_getProperty(consumer->h.props, "RequestOrderID", &field);
        if (rc) {
            consumer->requestOrderID = 0;
        } else {
            consumer->requestOrderID = field.val.i;
        }

        /* Create and send the action */
        act = ismc_newAction(session->connect, session, Action_createDurable);
        act->parseReply = parseReplyConsumer;
        act->userdata = consumer;

        field.type = VT_Boolean;
        field.val.i = 1;
        ismc_setProperty(consumer, "isDurable", &field);

        field.val.i = nolocal;
        ismc_setProperty(consumer, "noLocal", &field);

        ismc_setStringProperty(consumer, "subscriptionName", subname);
        ismc_setStringProperty(consumer, "Name", topic);

        sprintf(buf, "ISMSession@%p", session);
        ismc_setStringProperty(consumer, "Session", buf);

        ism_protocol_putStringValue(&act->buf, subname);            /* val0 = subscription name   */
        ism_protocol_putStringValue(&act->buf, consumer->selector); /* val1 = selector            */
        act->hdr.hdrcount = 2;

        ism_protocol_putMapProperties(&act->buf, consumer->h.props);
        rc = ismc_request(act, 1);
        if (act->rc != 0) {
            ism_common_setError(rc);
            ism_common_free(ism_memory_ismc_misc,consumer->messages);
            ism_common_free(ism_memory_ismc_misc,consumer);
            consumer = NULL;
        }

        /* Restart the session */
        act->hdr.action = Action_resumeSession;
        act->parseReply = parseReplyGeneric;
        ismc_request(act, 0);

        ismc_freeAction(act);
    }

    unlockSession(session);

    return consumer;
}


/**
 * Remove a durable subscription.
 *
 * @param session  The session object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_unsubscribe(ismc_session_t * session, const char * name) {
    int rc;

    if (!name) {
        return ismc_setError(ISMRC_NoDestination, "Subscription name must be specified");
    }

    rc = checkAndLockSession(session);
    if (!rc) {
        action_t * act = ismc_newAction(session->connect, session, Action_unsubscribeDurable);

        ism_protocol_putStringValue(&act->buf, name);
        act->hdr.hdrcount = 1;
        rc = ismc_request(act, 1);

        ismc_freeAction(act);
    }

    if (rc) {
        ism_common_setError(rc);
    }

    unlockSession(session);

    return rc;
}



/**
 * Create a queue.
 *
 * @param name   The name of the queue
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
ismc_destination_t * ismc_createQueue(const char * name) {
    ismc_destination_t * dest = NULL;
    int  len = 0;

    if (name) {
        len = (int)strlen(name);
        /* check name */
    }
    if (len > 0) {
        dest = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,29),1, sizeof(ismc_destination_t)+len+1);
        dest->h.id = OBJID_Destination;
        dest->h.state = OBJSTATE_Open;
        pthread_spin_init(&dest->h.lock, 0);
        dest->h.props     = NULL;
        dest->h.propcount = 0;

        dest->name = (const char *)(dest+1);
        strcpy((char *)dest->name, name);
        dest->namelen = len;
        dest->domain = ismc_Queue;
        ismc_setStringProperty(dest, "ObjectType", "queue");
    }
    return dest;
}

/**
 * Create a topic.
 *
 * @param name   The name of the topic
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
ismc_destination_t * ismc_createTopic(const char * name) {
    ismc_destination_t * dest = NULL;
    int  len = 0;

    if (name) {
        len = (int)strlen(name);
        /* TODO check name */
    }
    if (len > 0) {
        dest = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,30),1, sizeof(ismc_destination_t)+len+1);
        dest->h.id = OBJID_Destination;
        dest->h.state = OBJSTATE_Open;
        pthread_spin_init(&dest->h.lock, 0);
        dest->name = (const char *)(dest+1);
        strcpy((char *)dest->name, name);

        dest->namelen = len;
        dest->domain = ismc_Topic;
        ismc_setStringProperty(dest, "ObjectType", "topic");
    }
    return dest;
}

/**
 * Create a message consumer.
 *
 * @param session  The session object.
 * @param dest     The topic or queue object
 * @param selector The selector object
 * @param nolocal  The nolocal flag
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
ismc_consumer_t * ismc_createConsumer(ismc_session_t * session, ismc_destination_t * dest, const char * selector, int nolocal) {
    size_t sellen = 0;
    char buf[64];
    ismc_consumer_t * consumer = NULL;
    ismRule_t * selectRule = NULL;
    unsigned int rc;
    int defaultCacheSize;

    rc = checkAndLockSession(session);
    if (!rc) {
        rc = checkDestination(dest);
    }
    if (!rc) {
        if (selector) {
            /* Compile selector and store it */
            sellen = strlen(selector);
            rc = ism_common_compileSelectRule(&selectRule, NULL, selector);
            if (rc != 0) {
                sellen = 0;
                selectRule = NULL;
            }
        }
    }
    if (!rc) {
        action_t * act;
        int i;
        const char * name;
        ism_field_t field;

        /* Create composite properties from session and destination properties */
        consumer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,31),1, sizeof(ismc_consumer_t)+sellen+1);
        consumer->h.id = OBJID_Consumer;
        consumer->h.state = OBJSTATE_Open;
        pthread_spin_init(&consumer->h.lock, 0);
        pthread_mutex_init(&consumer->lock, NULL);
        consumer->h.props     = ism_common_newProperties(20);
        consumer->h.propcount = 0;
        consumer->session  = session;
        consumer->dest     = dest;
        consumer->action   = NULL;
        consumer->selector = (const char *)(consumer+1);
        if (!selectRule) {
            consumer->selector = NULL;
        } else {
            strcpy((char *)consumer->selector, selector);
            consumer->selectRule = selectRule;
        }
        consumer->domain   = dest->domain;
        consumer->nolocal  = nolocal;

        /*
         * Engine doesn't support sync ismc_receive yet, so
         * allocate the structure for holding
         * asynchronously received messages that could be requested
         * using ismc_receive.
         */
        consumer->messages = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,32),1, sizeof(ism_common_list));
        ism_common_list_init(consumer->messages, 1, NULL);

        /* Copy properties from destination */
        for (i = 0; ism_common_getPropertyIndex(session->h.props, i, &name) == 0; i++) {
            ism_common_getProperty(session->h.props, name, &field);
            ismc_setProperty(consumer, name, &field);
        }
        for (i = 0; ism_common_getPropertyIndex(dest->h.props, i, &name) == 0; i++) {
            ism_common_getProperty(dest->h.props, name, &field);
            ismc_setProperty(consumer, name, &field);
        }

        defaultCacheSize = session->fullSize;
        if ((dest->domain == ismc_Queue) && (session->ackmode == SESSION_AUTO_ACKNOWLEDGE)) {
            defaultCacheSize = 0;
        }

        consumer->fullSize = ismc_getIntProperty(consumer->h.props, "ClientMessageCache", defaultCacheSize);
        if (consumer->fullSize < 0)
        	consumer->fullSize = 0;
        consumer->emptySize = consumer->fullSize / 4;

        rc = ism_common_getProperty(dest->h.props, "DisableACK", &field);
        if (rc) {
            consumer->disableACK = session->disableACK;
        } else {
            consumer->disableACK = field.val.i;
        }

        rc = ism_common_getProperty(consumer->h.props, "RequestOrderID", &field);
        if (rc) {
            consumer->requestOrderID = 0;
        } else {
            consumer->requestOrderID = field.val.i;
        }

        /* Create and send the action */
        act = ismc_newAction(session->connect, session, Action_createConsumer);
        act->parseReply = parseReplyConsumer;
        act->userdata = consumer;

        ismc_setStringProperty(consumer, "Name", dest->name);
        sprintf(buf, "ISMSession@%p", session);
        ismc_setStringProperty(consumer, "Session", buf);

        ismc_setIntProperty(consumer, "ClientMessageCache", consumer->fullSize, VT_Integer);
        consumer->suspendFlags &= ~ISMC_CONSUMER_SUSPEND_0;
        consumer->suspendFlags &= ~ISMC_CONSUMER_SUSPEND_1;

        field.type = VT_Boolean;
        field.val.i = consumer->nolocal;
        ismc_setProperty(consumer, "noLocal", &field);

        ism_protocol_putByteValue(&act->buf, consumer->domain);                 /* val0 = domain   */
        ism_protocol_putBooleanValue(&act->buf, consumer->nolocal);             /* val1 = nolocal  */
        ism_protocol_putStringValue(&act->buf, consumer->selector);             /* val2 = selector */
        act->hdr.hdrcount = 3;

        ism_protocol_putMapProperties(&act->buf, consumer->h.props);
        rc = ismc_request(act, 1);
        if (act->rc != 0) {
            ism_common_setError(rc);
            ism_common_free(ism_memory_ismc_misc,consumer->messages);
            ism_common_free(ism_memory_ismc_misc,consumer);
            consumer = NULL;
        }

        /* Restart the session */
        act->hdr.action = Action_resumeSession;
        act->parseReply = parseReplyGeneric;
        ismc_request(act, 0);

        ismc_freeAction(act);
    }

    unlockSession(session);

    return consumer;
}

/**
 * Close a message consumer - internal, locks and checks are done.
 *
 * @param consumer  The consumer object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_closeConsumerInternal(ismc_consumer_t * consumer) {
    ismc_session_t        * session;
    ism_common_list       * list;
    ism_common_listIterator iter;
    int rc = 0;

    session = consumer->session;

    TRACE(7, ">>> %s: consumerid=%d session_id=%d: closed=%d, messages=%d, lastDelivered=%lu.\n",
    		 __func__, consumer->consumerid, session->sessionid, consumer->isClosed,
    		 consumer->messages?consumer->messages->size:-1, consumer->lastDelivered);

    ismc_removeConsumerFromSession(consumer);

    /* Close the consumer */
    if (!consumer->isClosed) {
        ism_field_t field;
        action_t * act;

        if (session->ackmode == SESSION_DUPS_OK_ACKNOWLEDGE && !session->isClosed) {
            if ((session->lastDelivered - session->lastAcked) > 0 )
                ismc_acknowledgeInternal(consumer->session);
        }

        field.type = VT_Boolean;
        field.val.i = 1;

        consumer->isClosed = 1;
        /*
         * If the object is in the process of being freed, stop.
         * Otherwise mark is closed.
         */
        pthread_spin_lock(&consumer->h.lock);
        if (consumer->h.state == OBJSTATE_Freed) {
            pthread_spin_unlock(&consumer->h.lock);
            return ISMRC_Destroyed;
        }
        consumer->h.state = OBJSTATE_Closed;
        pthread_spin_unlock(&consumer->h.lock);

        ismc_setProperty(consumer, "isClosed", &field);

        act = ismc_newAction(consumer->session->connect, consumer->session, Action_closeConsumer);
        act->hdr.item = endian_int32(consumer->consumerid);

        ism_protocol_putLongValue(&act->buf, consumer->lastDelivered);
        act->hdr.hdrcount = 1;

        rc = ismc_request(act, 1);

        ismc_freeAction(act);
    }

    /* Free up received messages that were not processed */
    list = consumer->messages;
    ism_common_list_iter_init(&iter, list);
    while (ism_common_list_iter_hasNext(&iter)) {
        ism_common_list_node * node = ism_common_list_iter_next(&iter);
        action_t * act = node->data;
        if (act) {
            ismc_freeAction(act);
        }
    }
    ism_common_list_iter_destroy(&iter);
    ism_common_list_destroy(list);
    ism_common_free(ism_memory_ismc_misc,consumer->messages);
    consumer->messages = NULL;

    consumer->msgCount = 0;

    ismc_freeAction(consumer->action);
    consumer->action = NULL;
    pthread_mutex_destroy(&consumer->lock);

    TRACE(7, "<<< %s: consumerid=%d, session_id=%d.\n", __func__, consumer->consumerid, session->sessionid);

    return rc;
}

/**
 * Close a message consumer.
 *
 * @param consumer  The consumer object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_closeConsumer(ismc_consumer_t * consumer) {
    int rc = 0;

    if (consumer == NULL) {
        return ismc_setError(ISMRC_NullPointer, "Cannot close NULL consumer");
    }

    /* Remove from the session */
    if (consumer->session == NULL) {
        return ismc_setError(ISMRC_ObjectNotValid, "A consumer does not have associated session");
    }

    if (consumer->h.id != OBJID_Consumer) {
        return ismc_setError(ISMRC_ObjectNotValid, "Input to close is not a valid consumer");
    }

    rc = checkAndLockSession(consumer->session);
    if (rc) {
    	unlockSession(consumer->session);
    	return rc;
    }

    rc = ismc_closeConsumerInternal(consumer);

    unlockSession(consumer->session);

    return rc;
}

/**
 * Close a message producer - internal, locks and checks are done.
 *
 * @param producer  The producer object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_closeProducerInternal(ismc_producer_t * producer) {
    int rc = 0;
    ismc_session_t * session = producer->session;

    /* Remove from the session */
    ismc_removeProducerFromSession(producer);

    /* Close the producer */
    if (!producer->isClosed) {
        ism_field_t field;
        action_t * act;

        field.type = VT_Boolean;
        field.val.i = 1;

        producer->isClosed = 1;
        producer->h.state = OBJSTATE_Closed;

        ismc_setProperty(producer, "isClosed", &field);

        act = ismc_newAction(session->connect, session, Action_closeProducer);
        act->hdr.item = endian_int32(producer->producerid);

        rc = ismc_request(act, 1);
        ismc_freeAction(act);
    }

    return rc;
}


/**
 * Close a message producer.
 *
 * @param producer  The producer object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_closeProducer(ismc_producer_t * producer) {
    int rc = 0;
    ismc_session_t * session;

    if (producer == NULL) {
        return ismc_setError(ISMRC_NullPointer, "Cannot close NULL producer");
    }

    if (producer->session == NULL) {
        return ismc_setError(ISMRC_ObjectNotValid, "A producer does not have associated session");
    }

    if (producer->h.id != OBJID_Producer) {
        return ismc_setError(ISMRC_ObjectNotValid, "Input to close is not a valid producer");
    }

    session = producer->session;

    rc = checkAndLockSession(session);
    if (rc) {
    	unlockSession(session);
    	return rc;
    }

    rc = ismc_closeProducerInternal(producer);

    unlockSession(session);

    return rc;
}

/*
 * Update the ACK position
 */
void updateDelivered(ismc_session_t * session, uint64_t sqn, ismc_consumer_t * consumer) {
    int  done = 0;
    int  i;

    pthread_mutex_lock(&session->deliverLock);

    if (session->acksqn == NULL) {
        session->acksqn_len = 50;
        session->acksqn = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,36),session->acksqn_len, sizeof(uint64_t));
    }
    for (i=0; i<session->acksqn_count; i+=2) {
        if ((int)session->acksqn[i] == consumer->consumerid) {
            done = 1;
            session->acksqn[i+1] = sqn;
            break;
        }
    }
    if (!done) {
        if (session->acksqn_count == session->acksqn_len) {
            session->acksqn = ism_common_realloc(ISM_MEM_PROBE(ism_memory_ismc_misc,37),session->acksqn, session->acksqn_len*4*sizeof(uint64_t));
            session->acksqn_len = session->acksqn_len * 4;
        }
        session->acksqn[session->acksqn_count++] = consumer->consumerid;
        session->acksqn[session->acksqn_count++] = sqn;
    }

    pthread_mutex_unlock(&session->deliverLock);
}


int sendAutoAck(ismc_consumer_t * consumer) {
	if (!consumer->disableACK && (consumer->session->ackmode == SESSION_AUTO_ACKNOWLEDGE)) {
		ismc_acknowledgeInternalSync(consumer);
		return (consumer->fullSize != 0);
	}
	return 1;
}

/**
 * Receive a message.
 *
 * @param consumer  The consumer object.
 * @param timeout   The timeout in milliseconds.  If this is 0, there is no timeout. If this is negative,
 *                  return immediately if a message is not available.
 * @param pmessage  Pointer to receive the message if return code is good
 *
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_receive(ismc_consumer_t * consumer, int64_t timeout, ismc_message_t **pmessage) {
    ismc_message_t *  message;
    ismc_session_t *  session;
    int               rc;
    ism_time_t        endTime = 0;
    ism_common_list * list;
    action_t *        act;
    ism_time_t        expireTime;
    int               sendResume;

    *pmessage = NULL;

    if (!consumer) {
        rc = ISMRC_NullPointer;
        ismc_setError(rc, "Cannot receive - the message consumer is NULL");
        return rc;
    }

    if (consumer->h.id != OBJID_Consumer) {
        rc = ISMRC_ObjectNotValid;
        ismc_setError(rc, "Cannot receive - the message consumer is not valid");
        return rc;
    }

    if (consumer->isClosed) {
        rc = ISMRC_Closed;
        ismc_setError(rc, "Cannot receive - the message consumer is closed");
        return rc;
    }

    session = consumer->session;
    rc = ismc_checkSession(session);
    if (rc) {
        return rc;
    }

    rc = checkConnected(consumer->session->connect);
    if (rc) {
        ismc_setError(rc, "Connection is closed or not valid");
        return rc;
    }

    sendResume = (consumer->lastDelivered != 0) ? sendAutoAck(consumer) : 1;

    /*
     * Traverse the message list and retrieve the first match.
     */
    list = consumer->messages;

    do {
        if (endTime == 0)
            endTime = ism_common_currentTimeNanos() + timeout * 1000 * 1000;

        if (ism_common_list_remove_head(list, (void **)&act) == 0) {
            message = ismc_makeMessage(consumer, act);
            ismc_freeAction(act);

			if (message) {
				if (message->ack_sqn <= consumer->lastDelivered){
                    ismc_consumerCachedMessageRemoved(consumer, __func__, message, sendResume);
                    sendResume = 1;

					ismc_freeMessage(message);
					continue;
				}

                expireTime = message->expire;
                if (expireTime != 0 && (expireTime < ism_common_currentTimeNanos())) {
                    sendResume = sendAutoAck(consumer);
                    ismc_consumerCachedMessageRemoved(consumer, __func__, message, sendResume);
                    sendResume = 1;
                    ismc_freeMessage(message);
                    continue;
                } else {
                    updateDelivered(session, message->ack_sqn, consumer);
                    ismc_consumerCachedMessageRemoved(consumer, __func__, message, sendResume);
                    *pmessage = message;
                    return ISMRC_OK;
                }

			    ismc_freeMessage(message);
			}
            break;
        }
        ism_common_delay(2);
    } while (!consumer->session->isClosed && (timeout == 0 || endTime > ism_common_currentTimeNanos()));

    if (timeout > 0) {
        rc = ISMRC_TimeOut;
    } else {
        rc = ISMRC_NoMsgAvail;
    }

    return rc;
}


/**
 * Set a keepalive value for this connection.
 * @param conn      The connection object.
 * @param keepalive The keep alive interval (in seconds).
 */
void ismc_setKeepAlive(ismc_connection_t * conn, int keepalive) {
    // TODO Port implementation from Java JMS client
}


/*
 * Set a message listener.
 *
 * @param consumer  The consumer object.
 * @param onmessage The listener to call when a message arrives.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_setlistener(ismc_consumer_t * consumer, ismc_onmessage_t onmessage, void * userdata) {
    int rc = ISMRC_OK;
    ismc_session_t * session;

    if (!consumer) {
        return ismc_setError(ISMRC_NullPointer, "Consumer is not specified");
    }

    if (consumer->h.id != OBJID_Consumer) {
        return ismc_setError(ISMRC_ObjectNotValid, "Input to setlistener is not a valid consumer");
    }

    session = consumer->session;

    if (ismc_checkSession(consumer->session) != 0) {
        return ismc_setError(ISMRC_NotConnected, "Session is not valid");
    }

    if (checkConnected(consumer->session->connect) != 0) {
        return ismc_setError(ISMRC_NotConnected, "Not connected");
    }

    if (consumer->onmessage == NULL) {
        action_t * act;
        if (onmessage == NULL) {
            return ismc_setError(ISMRC_NullPointer, "Message listener is required, but not specified");
        }

        consumer->onmessage = onmessage;
        consumer->userdata = userdata;

        act = ismc_newAction(consumer->session->connect, consumer->session, Action_setMsgListener);
        act->hdr.item = endian_int32(consumer->consumerid);

        // Issue "setListener" request
        rc = ismc_request(act, 1);
        ismc_freeAction(act);

        session->deliveryThreadId = ismc_getDeliveryThreadId();
    } else {
        if (onmessage == NULL) {
            action_t * act = ismc_newAction(consumer->session->connect, consumer->session, Action_removeMsgListener);
            act->hdr.item = endian_int32(consumer->consumerid);

            // Issue "removeListener" request
            rc = ismc_request(act, 1);
            ismc_freeAction(act);
        }

        consumer->onmessage = onmessage;
        consumer->userdata = userdata;

    }

    return rc;
}


/**
 * Create a message producer.
 *
 * Unlike in JMS, you must provide a destination for a producer.
 * @param session  The session object.
 * @param dest     The topic or queue object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
ismc_producer_t * ismc_createProducer(ismc_session_t * session, ismc_destination_t * dest) {
    char buf[64];
    ismc_producer_t * producer = NULL;
    int i;
    const char * name;
    ism_field_t field;
    uint64_t uniqueId;
    action_t * act;
    int rc = ismc_checkSession(session);
    if (!rc) {
        rc = checkDestination(dest);
    }
    if (!rc) {
        producer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,38),1, sizeof(ismc_producer_t));
        producer->h.id = OBJID_Producer;
        producer->h.state = OBJSTATE_Open;
        pthread_spin_init(&producer->h.lock, 0);
        producer->session     = session;
        producer->dest        = dest;
        producer->domain      = dest->domain;
        producer->msgCount    = 0;
        producer->msgIdTime   = 0;
        producer->h.props     = ism_common_newProperties(20);
        producer->h.propcount = 0;

        /* Copy properties from session and destination */
        for (i = 0; ism_common_getPropertyIndex(session->h.props, i, &name) == 0; i++) {
            ism_common_getProperty(session->h.props, name, &field);
            ismc_setProperty(producer, name, &field);
        }
        for (i = 0; ism_common_getPropertyIndex(dest->h.props, i, &name) == 0; i++) {
            ism_common_getProperty(dest->h.props, name, &field);
            ismc_setProperty(producer, name, &field);
        }

        /* Some additional properties */
        ismc_setStringProperty(&producer->h, "Name", dest->name);
        sprintf(buf, "ISMSession@%p", session);
        ismc_setStringProperty(&producer->h, "Session", buf);

        /* Initialize message ID buffer */
        producer->msgIdBuffer[0] = 'I';
        producer->msgIdBuffer[1] = 'D';
        producer->msgIdBuffer[2] = ':';

        uniqueId = (uint64_t)producer;
        for (i = 11; i < 15; i++) {
            int j = (int)(uniqueId&0x1F);
            char ch = (j < 10)?('0' + j):('A' + j);

            producer->msgIdBuffer[i] = ch;
            uniqueId >>= 5;
        }
        ismc_makeMsgID(producer);

        /* Create and send action */
        act = ismc_newAction(session->connect, session, Action_createProducer);

        ism_protocol_putByteValue(&act->buf, producer->domain);                 /* val0 = domain */
        act->hdr.hdrcount = 1;

        ism_protocol_putMapProperties(&act->buf, producer->h.props);
        rc = ismc_request(act, 1);
        if (act->rc == 0) {
            ism_field_t field2;
            ism_protocol_getObjectValue(&act->buf, &field2);
            producer->producerid = field2.val.i;

            ismc_addProducerToSession(producer);
        } else {
            ism_common_setError(rc);
            ism_common_free(ism_memory_ismc_misc,producer);
            producer = NULL;
        }

        ismc_freeAction(act);
    }
    return producer;
}

/*
 * Create a message ID.
 * The message ID consists of the required characters "ID:" followed by 40 bit
 * timestamp in base 32, a four digit producer unique string, and a four base32 counter.
 * @param producer A message producer
 * @return A pointer to a message ID (non-malloc)
 */
const char * ismc_makeMsgID(ismc_producer_t * producer) {
    uint64_t count;
    int i;

    pthread_spin_lock(&producer->h.lock);

    count = producer->msgCount++;
    if ((count&0x3FF) != ((count-1)&0x3FF)) {
        ism_time_t now = ism_common_currentTimeNanos() / 1000000;
        if (now != producer->msgIdTime) {
            producer->msgIdTime = now;
            now >>= 2;
            for (i = 10; i > 2; i--) {
                int j = (int)(now&0x1f);
                char ch = (j < 10)?('0' + j):('A' + j);
                producer->msgIdBuffer[i] = ch;
                now >>= 5;
            }
        }
    }
    for (i = 18; i > 14; i--) {
        int j = (int)(count&0x1f);
        char ch = (j < 10)?('0' + j):('A' + j);
        producer->msgIdBuffer[i] = ch;
        count >>= 5;
    }

    pthread_spin_unlock(&producer->h.lock);

    return producer->msgIdBuffer;
}

/*
 * Create a message ID.
 * The message ID consists of the required characters "ID:" followed by 40 bit
 * timestamp in base 32, a four digit session unique string, and a four base32 counter.
 * @return A pointer to a message ID (non-malloc)
 */
const char * ismc_makeMsgIDX(ismc_session_t * session) {
    static char msgIdBuffer[19] = {'I', 'D', ':', '\0'};
    static int messageCount = 0;
    int i;
    uint64_t uniqueId;
    int count;

    pthread_mutex_lock(msgIdLock);

    uniqueId = (uint64_t)session;
    for (i = 11; i < 15; i++) {
        int j = (int)(uniqueId&0x1F);
        char ch = (j < 10)?('0' + j):('A' + j);

        msgIdBuffer[i] = ch;
        uniqueId >>= 5;
    }

    count = messageCount++;
    if ((count&0x3FF) != ((count-1)&0x3FF)) {
        ism_time_t now = ism_common_currentTimeNanos() / 1000000;
        now >>= 2;
        for (i = 10; i > 2; i--) {
            int j = (int)(now&0x1f);
            char ch = (j < 10)?('0' + j):('A' + j);
            msgIdBuffer[i] = ch;
            now >>= 5;
        }
    }
    for (i = 18; i > 14; i--) {
        int j = (int)(count&0x1f);
        char ch = (j < 10)?('0' + j):('A' + j);
        msgIdBuffer[i] = ch;
        count >>= 5;
    }

    pthread_mutex_unlock(msgIdLock);

    return msgIdBuffer;
}

/*
 * Send a message.
 *
 * @param producer  The producer object.
 * @param message   The producer to send.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_send(ismc_producer_t * producer, ismc_message_t * message) {
    int rc;
    ism_field_t field;
    int producerId;
    int actionType;
    int wait;
    int qos;

    action_t * sendMsgAction;

    if (!producer) {
        return ismc_setError(ISMRC_NullPointer, "Cannot send - the message producer is NULL");
    }

    if (producer->h.id != OBJID_Producer) {
        return ismc_setError(ISMRC_ObjectNotValid, "Cannot send - the message producer is not valid");
    }

    if (producer->isClosed) {
        return ismc_setError(ISMRC_Closed, "Cannot send - the message producer is closed");
    }

    rc = ismc_checkSession(producer->session);
    if (rc) {
        return rc;
    }

    rc = checkConnected(producer->session->connect);
    if (rc) {
        return rc;
    }

    rc = checkDestination(producer->dest);
    if (rc) {
        return rc;
    }

    if (message == NULL) {
        return ismc_setError(ISMRC_NullPointer, "Cannot send - the message is NULL");
    }

    /*
     * As we modify the input message, we must make sure that we lock the message from when
     * the header fields are modified, until we build the JMS header.  Of course if the
     * application is sending the message from multiple threads, it cannot look at the
     * results of the send.
     */
    pthread_spin_lock(&message->h.lock);

    /*
     * Complete the message with message properties
     */
    ism_common_getProperty(producer->h.props, "DisableMessageTimestamp", &field);
    if (!(field.type == VT_Boolean && field.val.i == 1)) {
        ism_time_t currentTime = ism_common_currentTimeNanos();
        int64_t tmillis = currentTime / 1000000;
        ismc_setTimestamp(message, tmillis);
        if (message->expire == 0) {
            ismc_setExpiration(message, (message->ttl == 0) ? 0 : (tmillis + message->ttl));
        }
    } else {
        ismc_setTimestamp(message, 0);
        ismc_setExpiration(message, 0);
    }

    ism_common_getProperty(producer->h.props, "DisableMessageID", &field);
    if (!(field.type == VT_Boolean && field.val.i == 1)) {
        ismc_setMessageID(message, ismc_makeMsgID(producer));
    } else {
        ismc_setMessageID(message, NULL);
    }

    /*
     * The JMS specification requires that this be called only in a single thread,
     * and therefore we should not need to lock, and indeed we have not locked the
     * previous work here.  However, since this code should be synchronized, if
     * should not cost us much to do so here, and it will prevent almost all
     * consequences of violating the JMS session threading model.
     */
    pthread_spin_lock(&producer->h.lock);

    producerId = producer->producerid;

    /*
     * If the message is non-persistent, or in a transaction, then send without wait
     */
    actionType = Action_messageWait;

    wait = 1;

    /* Use async mode only for non-persistent messages and not in transaction */
    if (ismc_getDeliveryMode(message) != ISMC_PERSISTENT && !producer->session->transacted) {
        actionType = Action_message;
        wait = 0;
    }

    sendMsgAction = ismc_newAction(producer->session->connect, producer->session, actionType);

    sendMsgAction->hdr.item = endian_int32(producerId);
    if (!wait) {
        sendMsgAction->hdr.msgid = 0;
    }

    qos = ismc_getQuality(message);

    sendMsgAction->hdr.hdrcount = 0;

    sendMsgAction->hdr.bodytype = ismc_getMessageType(message);
    sendMsgAction->hdr.priority = (ismc_getPriority(message) & 0x0F);
    if (ismc_getDeliveryMode(message) == ISMC_PERSISTENT) {
        sendMsgAction->hdr.flags |= 0x80;
    } else {
        if (!producer->session->disableACK && (qos < 0)) {
            /* If ACKs are not disabled and QoS is not set, default to QoS 1 */
            sendMsgAction->hdr.flags |= 0x01;
        }
    }

    if (qos > 0) {
        sendMsgAction->hdr.flags |= (qos & 0x07);
    }

    if (message->retain & 0x01) {
        sendMsgAction->hdr.flags |= (ACTFLAG_Retain|ACTFLAG_RetainPublish);
    }

    if (message->expire != 0) {
        sendMsgAction->hdr.flags |= ACTFLAG_Expires;
    }

    /*
     * Set message header fields, properties and message body into action.
     */
    ismc_putJMSValues(&sendMsgAction->buf, message->h.props, message,
            producer->dest->domain == ismc_Topic ? producer->dest->name : NULL);
    ism_protocol_putByteArrayValue(&sendMsgAction->buf, message->body.buf, message->body.used);

    pthread_spin_unlock(&message->h.lock);

    rc = ismc_request(sendMsgAction, wait);

    pthread_spin_unlock(&producer->h.lock);

    ismc_freeAction(sendMsgAction);

    return rc;
}

/*
 * Send a message without a producer
 * @param session  The session object
 * @param domain   The messaging domain
 * @param tqname   The name of the topic or queue
 * @param message  The message to send.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_sendX(ismc_session_t * session, int domain, const char * tqname, ismc_message_t * message) {
    ism_field_t field;
    int wait;
    action_t * sendMsgAction;
    int rc = ismc_checkSession(session);
    if (rc) {
        return rc;
    }

    if (tqname == NULL) {
        return ismc_setError(ISMRC_DestNotValid, "Cannot send - the destination must have a name");
    }

    if (domain != ismc_Queue && domain != ismc_Topic) {
        return ismc_setError(ISMRC_DestNotValid, "Cannot send - the destination domain must be queue or topic, but %d found", domain);
    }

    if (message == NULL) {
        return ismc_setError(ISMRC_NullPointer, "Cannot send - the message is NULL");
    }

    /*
     * As we modify the input message, we must make sure that we lock the message from when
     * the header fields are modified, until we build the JMS header.  Of course if the
     * application is sending the message from multiple threads, it cannot look at the
     * results of the send.
     */
    pthread_spin_lock(&message->h.lock);

    /*
     * Complete the message with message properties
     */
    ism_common_getProperty(session->connect->h.props, "DisableMessageTimestamp", &field);
    if (!(field.type == VT_Boolean && field.val.i == 1)) {
        ism_time_t currentTime = ism_common_currentTimeNanos();
        int64_t tmillis = currentTime / 1000000;
        ismc_setTimestamp(message, tmillis);
        if (message->expire == 0) {
            ismc_setExpiration(message, (message->ttl == 0) ? 0 : (tmillis + message->ttl));
        }
    } else {
        ismc_setTimestamp(message, 0);
        ismc_setExpiration(message, 0);
    }

    ism_common_getProperty(session->connect->h.props, "DisableMessageID", &field);
    if (!(field.type == VT_Boolean && field.val.i == 1)) {
        ismc_setMessageID(message, ismc_makeMsgIDX(session));
    } else {
        ismc_setMessageID(message, NULL);
    }

    /*
     * If the message is non-persistent, or in a transaction, then send without wait
     */
    wait = 1;

    /* Use async mode only for non-persistent messages and not in transaction */
    if (ismc_getDeliveryMode(message) != ISMC_PERSISTENT && !session->transacted) {
        wait = 0;
    }

    sendMsgAction = ismc_newAction(session->connect, session, wait ? Action_msgNoProdWait : Action_messageNoProd);

    if (!wait) {
        sendMsgAction->hdr.msgid = 0;
    }

    /* Some additional properties */
    ism_protocol_putByteValue(&sendMsgAction->buf, domain);               /* val0 = domain     */
    ism_protocol_putStringValue(&sendMsgAction->buf, tqname);             /* val1 = dest name  */
    sendMsgAction->hdr.hdrcount = 2;

    sendMsgAction->hdr.bodytype = ismc_getMessageType(message);
    sendMsgAction->hdr.priority = (ismc_getPriority(message) & 0x0F);
    if (ismc_getDeliveryMode(message) == ISMC_PERSISTENT) {
        sendMsgAction->hdr.flags |= 0x80;
    }
    sendMsgAction->hdr.flags |= (ismc_getQuality(message) & 0x07);

    if (message->retain & 0x01) {
        sendMsgAction->hdr.flags |= (ACTFLAG_Retain|ACTFLAG_RetainPublish);
    }

    if (message->expire != 0) {
        sendMsgAction->hdr.flags |= ACTFLAG_Expires;
    }

    /*
     * Set message header fields, properties and message body into action.
     */
    ismc_putJMSValues(&sendMsgAction->buf, message->h.props, message, domain==ismc_Topic ? tqname : NULL);
    ism_protocol_putByteArrayValue(&sendMsgAction->buf, message->body.buf, message->body.used);

    pthread_spin_unlock(&message->h.lock);

    rc = ismc_request(sendMsgAction, wait);

    ismc_freeAction(sendMsgAction);

    return rc;
}


/*
 * Create a new action.
 * @param  connect A connection.
 * @param  session A session.
 * @param  action  An action type.
 * @return A new action (malloc).
 */
action_t * ismc_newAction(ismc_connection_t * connect, ismc_session_t * session, int action) {
    action_t * act = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,40),1, sizeof(action_t));

    ismc_resetAction(act, connect, session, action);

    act->action_len = sizeof(act->hdr);
    act->buf.buf = act->xbuf;
    act->buf.len = sizeof(act->xbuf);
    act->buf.inheap = 0;

    pthread_mutex_init(&act->waitLock, 0);
    pthread_cond_init(&act->waitCond, 0);

    return act;
}

/*
 * Set new fields in existing action (reusing allocated action).
 * @param  act     A pointer to existing action
 * @param  connect A connection.
 * @param  session A session.
 * @param  action  An action type.
 */
void ismc_resetAction(action_t * act, ismc_connection_t * connect, ismc_session_t * session, int action) {
    act->connect = connect;
    act->session = session;
    act->hdr.action = action;

    act->parseReply = parseReplyGeneric;
    act->userdata = NULL;

    switch (action) {
    case Action_ack:
        act->hdr.itemtype = ITEMT_None;
        act->hdr.item = endian_int32(session->sessionid);
        break;

    case Action_startConsumer:
    case Action_closeConsumer:
    case Action_setMsgListener:
    case Action_removeMsgListener:
    case Action_receiveMsg:
        act->hdr.itemtype = ITEMT_Consumer;
        act->hdr.msgid = endian_int64(ismc_getThreadId());
        break;

    case Action_message:
    case Action_messageWait:
    case Action_closeProducer:
        act->hdr.itemtype = ITEMT_Producer;
        act->hdr.msgid = endian_int64(ismc_getThreadId());
        break;

    case Action_createConnection:
    case Action_startConnection:
    case Action_stopConnection:
    case Action_closeConnection:
    case Action_createSession:
    case Action_initConnection:
    case Action_termConnection:
    case Action_getTime:
        act->hdr.itemtype = ITEMT_Thread;
        act->hdr.item = 0;
        act->hdr.msgid = endian_int64(ismc_getThreadId());
        break;

    default:
        if (session) {
            act->hdr.itemtype = ITEMT_Session;
            act->hdr.item = endian_int32(session->sessionid);
            act->hdr.msgid = endian_int64(ismc_getThreadId());
        }
        break;
    }

    act->action_len = sizeof(act->hdr);
    act->hdr.hdrcount = 0;
    act->buf.pos = 0;
    act->buf.used = 0;
}

/*
 * Free an action and release underlying resources.
 * @param  act   An action to free
 */
void ismc_freeAction(action_t * act) {
    if (!act) {
        return;
    }

    if (act->hdr.itemtype != ITEMT_None && act->hdr.msgid > 0) {
        ismc_setAction(endian_int64(act->hdr.msgid), NULL);
    }

    pthread_mutex_destroy(&act->waitLock);
    pthread_cond_destroy(&act->waitCond);
    ism_common_freeAllocBuffer(&act->buf);
    ism_common_free(ism_memory_ismc_misc,act);
}

/*
 * This cleanup function is called when threads created
 * by ism_common_startThread function and used in ISMC are terminated.
 * It clears the "active thread" indicator in the table of thread IDs
 * (aka response IDs) used to correlate requests sent to and responses
 * received from the server.
 */
static void ismc_resetThread(void * cleanup_parm) {
	int id = (int64_t)cleanup_parm;
	if (id >= 0) {
		pthread_mutex_lock(actThLock);
		activeThreads[id] = 0;
		pthread_mutex_unlock(actThLock);
	}
}

/**
 * Obtain a unique thread ID (0-based).
 * @return  A thread id.
 */
int ismc_getThreadId(void) {
    /* Try to obtain the thread id from TLS or from freed up IDs, if available */
	int tid = threadId;

    if (tid < 0) {
    	/*
    	 * Check free IDs
    	 */
    	int i;

        pthread_mutex_lock(actThLock);
    	for (i = 1; i <= MAX_ACTION_ID; i++) {
    		if (activeThreads[i] == 0) {
    			tid = i;
    			activeThreads[i] = i;
    			break;
    		}
    	}
        pthread_mutex_unlock(actThLock);

        ism_common_setThreadCleanup(ismc_resetThread, (void *)(int64_t)tid);

        threadId = tid;
    }

    return tid;
}

/*
 * Get the action we were waiting for.
 * @param  respId  ID of the response.
 * @return An action corresponding to this response.
 */
action_t * ismc_getAction(uint64_t respId) {
    action_t * act;
    pthread_mutex_lock(corrObjLock);
    act = corrObj[(int)respId];
    pthread_mutex_unlock(corrObjLock);

    return act;
}

/**
 * Wake up all threads waiting for server response in ismc_request.
 * This function is used when an error is encountered in communications
 * layer.
 * @param rc  An error code.
 */
void ismc_wakeWaiters(ismc_connection_t * conn, int rc) {
    action_t * act;
    int i;

    for (i = 0; i <= MAX_ACTION_ID; i++) {
        act = corrObj[i];
        if (act && (act->connect == conn)) {
            pthread_mutex_lock(corrObjLock);
            corrObj[i] = NULL;
            pthread_mutex_unlock(corrObjLock);

            pthread_mutex_lock(&act->waitLock);
            act->doneWaiting = ISMC_RECEIVE_FAILED;
            act->rc = rc;
            pthread_cond_signal(&act->waitCond);
            pthread_mutex_unlock(&act->waitLock);
        }
    }
}

/**
 * Set action that we need to wait for (sync request).
 * @param  respId  A response ID.
 * @param  action  An action.
 */
void ismc_setAction(uint64_t respId, action_t * action) {
    pthread_mutex_lock(corrObjLock);
    corrObj[respId] = action;
    pthread_mutex_unlock(corrObjLock);
}

/*
 * Request an action.
 * @param  act  An action to request from the server
 * @param  wait 1, if synchronous request, 0 - asynchronous.
 * @return 0, if success, != otherwise
 */
int ismc_request(action_t * act, int wait) {
    /* Send action and wait */
    int rc;

    if (act->connect == NULL) {
        return -1;
    }

    if (wait) {
        ismc_setAction(endian_int64(act->hdr.msgid), act);

        pthread_mutex_lock(&act->waitLock);
        act->doneWaiting = ISMC_WAITING_MESSAGE;
        pthread_mutex_unlock(&act->waitLock);
    }

    act->action_len = sizeof(act->hdr) + act->buf.used;
    rc = ismc_sendAction(act->connect, act);
    if (rc != 0) {
        return rc;
    }

    if (wait) {
        pthread_mutex_lock(&act->waitLock);
        while (act->doneWaiting == ISMC_WAITING_MESSAGE) {      /* BEAM suppression: infinite loop */
            pthread_cond_wait(&act->waitCond, &act->waitLock);
        }

        if (act->doneWaiting == ISMC_TIMED_OUT) {
            rc = ISMRC_TimeOut;
        } else {
            rc = act->rc;
            if (rc == 1) {
                rc = 0;
            }
            if (rc) {
                ism_common_setError(rc);
            }
        }

        pthread_mutex_unlock(&act->waitLock);
    }

    return rc;
}

/*
 * Check that the connection is started and not closed
 */
static int checkConnected(ismc_connection_t * connect) {
    if (!connect)
        return ismc_setError(ISMRC_NullPointer, "The connection object is NULL");
    if (connect->h.id != OBJID_Connection)
        return ismc_setError(ISMRC_ObjectNotValid, "The connection object is not valid");
    if (!connect->isConnected)
        return ISMRC_NotConnected;
    if (connect->isClosed)
        return ISMRC_Closed;
    return 0;
}

/*
 * Check that the session is not closed.
 * @param session A session to check.
 * @return ISMRC_OK, if valid; non-zero reason code otherwise
 */
int ismc_checkSession(ismc_session_t * session) {
    if (session == NULL) {
        return ismc_setError(ISMRC_NullPointer, "The session object is NULL");
    }

    if (session->h.id != OBJID_Session) {
        return ismc_setError(ISMRC_ObjectNotValid, "The session object is not valid");
    }

    if (session->isClosed) {
        return ISMRC_Closed;
    }

    return ISMRC_OK;
}

/*
 * Check that the destination is valid
 * @param dest A destination to check.
 * @return ISMRC_OK, if valid; non-zero reason code otherwise
 */
static int checkDestination(ismc_destination_t * dest) {
    if (dest == NULL) {
        return ismc_setError(ISMRC_NoDestination, "The destination must be specified");
    } else if (dest->h.id != OBJID_Destination) {
        return ismc_setError(ISMRC_ObjectNotValid, "The destination must be an ISM destination");
    } else if (dest->name == NULL) {
        return ismc_setError(ISMRC_DestNotValid, "The destination must have a name");
    }

    return ISMRC_OK;
}

/**
 * Acknowledge message receipt.
 * @param session A session
 * @param sqn     Message sequence number.
 */
void ismc_acknowledge(ismc_session_t * session, uint64_t sqn) {
    int rc = checkAndLockSession(session);
    if (!rc) {
        if (session->ackmode != SESSION_CLIENT_ACKNOWLEDGE) {
        	unlockSession(session);
            return;
        }
        if ((sqn - session->lastDelivered) > 0)
            session->lastDelivered = sqn;

        if (session->lastAcked == session->lastDelivered) {
        	unlockSession(session);
            return;
        }
        ismc_acknowledgeInternal(session);

        unlockSession(session);
    }

    return;
}

/**
 * Acknowledge message receipt for the session.
 * @param session A session
 */
void ismc_acknowledgeInternal(ismc_session_t * session) {
    if (session->ackAction == NULL) {
        session->ackAction = ismc_newAction(session->connect, session, Action_ack);
    } else {
        session->ackAction->buf.used = 0;
        session->ackAction->hdr.hdrcount = 0;
    }

    if (ismc_writeAckSqns(session->ackAction, session, NULL)) {
        ismc_request(session->ackAction, 0);
    }
}

/**
 * Acknowledge the receipt of the final message for the session.
 * Called from ismc_closeSession under session lock after checking that
 * the session is valid.
 * @param session A session
 */
void ismc_acknowledgeFinal(ismc_session_t * session) {
	action_t * action = ismc_newAction(session->connect, session, Action_ackSync);

	if (ismc_writeAckSqns(action, session, NULL)) {
		ismc_request(action, 1);
	}

	ismc_freeAction(action);
}

/**
 * Adds the session to the connection.
 * @param session  A session
 */
static void ismc_addSessionToConnection(ismc_session_t * session) {
    int i;
    int found = 0;
    ismc_connection_t * conn = session->connect;
    pthread_spin_lock(&conn->h.lock);
    if (conn->sessions.array == NULL) {
        conn->sessions.totalSize = 10;
        conn->sessions.numElements = 0;
        conn->sessions.array = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,42),conn->sessions.totalSize * sizeof(ismc_session_t *));
    }

    if (conn->sessions.numElements == conn->sessions.totalSize) {
        conn->sessions.totalSize *= 2;
        conn->sessions.array = ism_common_realloc(ISM_MEM_PROBE(ism_memory_ismc_misc,43),conn->sessions.array,
                conn->sessions.totalSize * sizeof(ismc_session_t *));
    }

    for (i = 0; i < conn->sessions.numElements; i++) {
        if (((ismc_session_t * *)conn->sessions.array)[i] == NULL) {
            ((ismc_session_t * *)conn->sessions.array)[i] = session;
            found = 1;
            break;
        }
    }

    if (!found) {
        ((ismc_session_t * *)conn->sessions.array)[conn->sessions.numElements++] = session;
    }

    pthread_spin_unlock(&conn->h.lock);
}

/**
 * Adds the consumer to the connection.
 * @param consumer A consumer
 */
void ismc_addConsumerToConnection(ismc_consumer_t * consumer) {
    ismc_connection_t * conn = consumer->session->connect;
    ism_common_putHashMapElementLock(conn->consumers,
    		&consumer->consumerid, sizeof(consumer->consumerid),
    		consumer, NULL);
}

/**
 * Removes the session from the connection.
 * @param session  A session
 */
static void ismc_removeSessionFromConnection(ismc_session_t * session) {
    int i;
    ismc_connection_t * conn;

    if (!session) {
    	return;
    }

    conn = session->connect;
    if (!conn) {
    	return;
    }

    pthread_spin_lock(&conn->h.lock);

    do {
        if (conn->sessions.array == NULL) {
            break;
        }

        for (i = 0; i < conn->sessions.numElements; i++) {
            if (((ismc_session_t * *)conn->sessions.array)[i] == session) {
                ((ismc_session_t * *)conn->sessions.array)[i] = NULL;
                break;
            }
        }
    } while (0);

    pthread_spin_unlock(&conn->h.lock);
}

/**
 * Adds a consumer to the session.
 * @param cons  A consumer
 */
void ismc_addConsumerToSession(ismc_consumer_t * cons) {
    int i;
    int found = 0;
    ismc_session_t * session = cons->session;
    if (session->consumers.array == NULL) {
        session->consumers.totalSize = 10;
        session->consumers.numElements = 0;
        session->consumers.array = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,44),session->consumers.totalSize * sizeof(ismc_consumer_t *));
    }

    if (session->consumers.numElements == session->consumers.totalSize) {
        session->consumers.totalSize *= 2;
        session->consumers.array = ism_common_realloc(ISM_MEM_PROBE(ism_memory_ismc_misc,45),session->consumers.array,
                session->consumers.totalSize * sizeof(ismc_consumer_t *));
    }

    for (i = 0; i < session->consumers.numElements; i++) {
        if (((ismc_consumer_t * *)session->consumers.array)[i] == NULL) {
            ((ismc_consumer_t * *)session->consumers.array)[i] = cons;
            found = 1;
            break;
        }
    }

    if (!found) {
        ((ismc_consumer_t * *)session->consumers.array)[session->consumers.numElements++] = cons;
    }
}

/**
 * Removes the consumer from the session.
 * @param cons  A consumer
 */
static void ismc_removeConsumerFromSession(ismc_consumer_t * cons) {
    ismc_session_t * session;
    int i;

    if (!cons) {
    	return;
    }

    session = cons->session;
    if (!session) {
    	return;
    }

    do {
        if (session == NULL || session->consumers.array == NULL) {
            break;
        }

        for (i = 0; i < session->consumers.numElements; i++) {
            if (((ismc_consumer_t * *)session->consumers.array)[i] == cons) {
                ((ismc_consumer_t * *)session->consumers.array)[i] = NULL;
                break;
            }
        }
    } while (0);
}

/**
 * Adds the producer to the session.
 * @param prod  A producer
 */
static void ismc_addProducerToSession(ismc_producer_t * prod) {
    int i;
    int found = 0;
    ismc_session_t * session;

    if (!prod) {
    	return;
    }

    session = prod->session;
    if (!session) {
    	return;
    }

    if (session->producers.array == NULL) {
        session->producers.totalSize = 10;
        session->producers.numElements = 0;
        session->producers.array = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,46),session->producers.totalSize * sizeof(ismc_producer_t *));
    }

    if (session->producers.numElements == session->producers.totalSize) {
        session->producers.totalSize *= 2;
        session->producers.array = ism_common_realloc(ISM_MEM_PROBE(ism_memory_ismc_misc,47),session->producers.array,
        session->producers.totalSize * sizeof(ismc_producer_t *));
    }

    for (i = 0; i < session->producers.numElements; i++) {
        if (((ismc_producer_t * *)session->producers.array)[i] == NULL) {
            ((ismc_producer_t * *)session->producers.array)[i] = prod;
            found = 1;
            break;
        }
    }

    if (!found) {
        ((ismc_producer_t * *)session->producers.array)[session->producers.numElements++] = prod;
    }

}

/**
 * Removes the producer from the session.
 * @param prod  A producer
 */
static void ismc_removeProducerFromSession(ismc_producer_t * prod) {
    int i;
    ismc_session_t * session;

    if (!prod) {
    	return;
    }

    session = prod->session;
    if (!session) {
    	return;
    }

    do {
        if (session == NULL || session->producers.array == NULL) {
            break;
        }

        for (i = 0; i < session->producers.numElements; i++) {
            if (((ismc_producer_t * *)session->producers.array)[i] == prod) {
                ((ismc_producer_t * *)session->producers.array)[i] = NULL;
                break;
            }
        }
    } while (0);
}

/**
 * Periodically send acknowledgment for the session.
 * @param key       Timer key (not used)
 * @param timestamp Timestamp when the even has been invoked (not used)
 * @param userdata  A pointer to the session object
 */
static int ackTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ismc_session_t * session = userdata;

    if (session->isClosed) {
        ism_common_cancelTimer(key);
        return 0;
    }
    if (session->lastAcked == session->lastDelivered) {
        ism_common_cancelTimer(key);
        return 0;
    }
    ismc_acknowledgeInternal(session);
    return 1;
}

/**
 * Set the error listener for this connection.
 * @param connect  The connection object
 * @param onerror  The callback to call on an error
 * @param userdata A pointer to return to the callback
 * @return  The previous error listener
 */
ismc_onerror_t ismc_setErrorListener(ismc_connection_t * connect, ismc_onerror_t onerror, void * userdata) {
    ismc_onerror_t oldListener;
    int rc = checkConnected(connect);

    if (rc) {
        ismc_setError(rc, "Not connected");
        return NULL;
    }

    oldListener = connect->errorListener;
    connect->errorListener = onerror;
    connect->userdata = userdata;

    return oldListener;
}

/**
 * Create a manager record.
 * @param session   The session object
 * @param data        The data for the record.
 * @param len        The length of the data.
 * @return A queue manager record handle
 */
ismc_manrec_t ismc_createManagerRecord(ismc_session_t * session, const void * data, int len) {
    action_t * act;
    ism_field_t field;
    ismc_manrec_t manrec = NULL;
    int rc;

    rc = checkAndLockSession(session);
    if (rc) {
    	unlockSession(session);
        return NULL;
    }

    /*
     * Send the manager record create to the server
     */
    act = ismc_newAction(session->connect, session, Action_createQMRecord);
    act->hdr.hdrcount = 0;
    /* No properties */
    act->buf.buf[act->buf.used] = (char)S_Map;
    act->buf.used++;
    /* Data is in the body */
    ism_protocol_putByteArrayValue(&act->buf, data, len);
    rc = ismc_request(act, 1);
    unlockSession(session);

    if (act->rc == 0) {
        ism_protocol_getObjectValue(&act->buf, &field);
        manrec = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,48),1, sizeof(struct ismc_manrec_t));
        memcpy(manrec->eyecatcher, QM_RECORD_EYECATCHER, sizeof(manrec->eyecatcher));
        manrec->managed_record_id = (uint64_t)field.val.l;
    } else {
        ismc_setError(act->rc, "Failed to create manager record (rc=%d).", act->rc);
    }

    ismc_freeAction(act);

    return manrec;
}

/**
 * Delete a manager record.
 * @param session   The session object
 * @param manrec    The record handle.
 * @return A return code: 0=good.
 */
int ismc_deleteManagerRecord(ismc_session_t * session, ismc_manrec_t manrec) {
    action_t * act;
    int rc;

    if (manrec == NULL) {
        return ismc_setError(ISMRC_NullPointer, "The queue manager record is NULL");
    }

    if (memcmp(manrec->eyecatcher, QM_RECORD_EYECATCHER, sizeof(manrec->eyecatcher)) != 0) {
        return ismc_setError(ISMRC_ObjectNotValid, "The queue manager record is not valid");
    }

    rc = checkAndLockSession(session);
    if (rc) {
    	unlockSession(session);
        return rc;
    }

    /*
     * Send the manager record create to the server
     */
    act = ismc_newAction(session->connect, session, Action_destroyQMRecord);
    /* val0 = manager record id */
    ism_protocol_putLongValue(&act->buf, (int64_t)manrec->managed_record_id);
    act->hdr.hdrcount = 1;
    rc = ismc_request(act, 1);
    unlockSession(session);

    if (!rc) {
        memset(manrec->eyecatcher, 0x00, sizeof(manrec->eyecatcher));
    }

    ismc_freeAction(act);

    return rc;
}

/**
 * Get a set of manager records. If successful,
 * you must free the list when done.
 *
 * @param session   The session object.
 * @param manrecs    A pointer to the pointer to the list of record handles to be populated.
 * @param count     A pointer to the count of records in the list to be set.
 * @return A return code: 0=good.
 */
int ismc_getManagerRecords(ismc_session_t * session, ismc_manrec_list_t * * manrecs, int * count) {
    action_t * act;
    ism_field_t field;
    int rc;

    if (manrecs == NULL) {
        return ismc_setError(ISMRC_NullPointer, "The input record list is NULL");
    }

    if (count == NULL) {
        return ismc_setError(ISMRC_NullPointer, "The count pointer is NULL");
    }

    rc = checkAndLockSession(session);
    if (rc) {
    	unlockSession(session);
        return rc;
    }

    /*
     * Send the get queue manager records request to the server
     */
    act = ismc_newAction(session->connect, session, Action_getQMRecords);
    rc = ismc_request(act, 1);

    unlockSession(session);

    if (rc == 0) {
        /* The header field contains the count of records */
        ism_protocol_getObjectValue(&act->buf, &field);
        if (field.type == VT_Integer) {
            *count = field.val.i;

            /* The content of the message is a byte array that contains the list of
             * handles and associated data. */
            ism_protocol_getObjectValue(&act->buf, &field);
            if (field.type != VT_Null) {
                concat_alloc_t map = { 0 };
                ism_field_t recfield;
                int i;

                /* Include NULL-terminator */
                *manrecs = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,49),(*count + 1) * sizeof(struct ismc_manrec_list_t));

                map.len = field.len;
                map.buf = field.val.s;
                map.pos = 0;
                map.used = field.len;

                for (i = 0; i < *count; i++) {
                    /* Handle - 8-byte long */
                    ism_protocol_getObjectValue(&map, &recfield);
                    (*manrecs)[i].handle = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,50),sizeof(struct ismc_manrec_t));

                    (*manrecs)[i].handle->managed_record_id = recfield.val.l;
                    memcpy((*manrecs)[i].handle->eyecatcher, QM_RECORD_EYECATCHER, sizeof((*manrecs)[i].handle->eyecatcher));
                    /* Data - byte-array */
                    ism_protocol_getObjectValue(&map, &recfield);
                    (*manrecs)[i].len = recfield.len;
                    (*manrecs)[i].data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,51),recfield.len);
                    memcpy((*manrecs)[i].data, recfield.val.s, recfield.len);
                }
                (*manrecs)[*count].handle = 0;
            } else if (*count > 0) {
                rc = ISMRC_BadClientData;
                ismc_setError(rc, "Queue manager records are incorrect (rc=%d).", rc);
            }
        } else {
            rc = ISMRC_BadClientData;
            ismc_setError(rc, "Queue manager record count is missing (rc=%d).", rc);
        }
    } else {
        ismc_setError(act->rc, "Failed to get queue manager records (rc=%d).", act->rc);
    }

    ismc_freeAction(act);

    return rc;
}

/**
 * Free a list of manager records.
 * @param manrecs    A pointer to the list of record handles to be freed.
 */
void ismc_freeManagerRecords(ismc_manrec_list_t * manrecs) {
    ismc_manrec_list_t * record = manrecs;
    while (record && record->handle) {
        ism_common_free(ism_memory_ismc_misc,record->handle);
        ism_common_free(ism_memory_ismc_misc,record->data);
        record++;
    }
    ism_common_free(ism_memory_ismc_misc,manrecs);
}

/**
 * Create an XA record.
 * @param session   The session object
 * @param manager   The handle of the queue manager record to which
 *                  this XA record will belong.
 * @param data        The data for the record.
 * @param len        The length of the data.
 * @return A queue manager record handle
 */
ismc_xarec_t ismc_createXARecord(ismc_session_t * session, ismc_manrec_t manager, const void * data, int len) {
    action_t * act;
    ism_field_t field;
    ismc_xarec_t xarec = NULL;
    int rc;

    rc = checkAndLockSession(session);
    if (!rc) {
        if (manager == NULL) {
            rc = ismc_setError(ISMRC_NullPointer, "The queue manager record is NULL");
        } else if (memcmp(manager->eyecatcher, QM_RECORD_EYECATCHER, sizeof(manager->eyecatcher)) != 0) {
        	rc = ismc_setError(ISMRC_ObjectNotValid, "The queue manager record is not valid");
        }

    }
    if (rc) {
    	unlockSession(session);
        return NULL;
    }

    /*
     * Send the manager record create to the server
     */
    act = ismc_newAction(session->connect, session, Action_createQMXidRecord);
    /* val0 = manager record id */
    ism_protocol_putLongValue(&act->buf, (uint64_t)manager->managed_record_id);		/* BEAM suppression: operating on NULL */
    act->hdr.hdrcount = 1;
    /* No properties */
    act->buf.buf[act->buf.used] = (char)S_Map;
    act->buf.used++;
    /* Data is in the body */
    ism_protocol_putByteArrayValue(&act->buf, data, len);
    rc = ismc_request(act, 1);

    unlockSession(session);

    if (act->rc == 0) {
        ism_protocol_getObjectValue(&act->buf, &field);
        xarec = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,55),1, sizeof(struct ismc_xarec_t));
        memcpy(xarec->eyecatcher, XA_RECORD_EYECATCHER, sizeof(xarec->eyecatcher));
        xarec->xa_record_id = (uint64_t)field.val.l;
    } else {
        ismc_setError(act->rc, "Failed to create XA record (rc=%d).", act->rc);
    }

    ismc_freeAction(act);

    return xarec;
}

/**
 * Delete an XA record.
 * @param session   The session object
 * @param xarec        The record handle.
 * @return A return code: 0=good.
 */
int ismc_deleteXARecord(ismc_session_t * session, ismc_xarec_t xarec) {
    action_t * act;
    int rc;

    rc = checkAndLockSession(session);
    if (!rc) {
        if (xarec == NULL) {
            rc = ismc_setError(ISMRC_NullPointer, "The XA record is NULL");
        } else if (memcmp(xarec->eyecatcher, XA_RECORD_EYECATCHER, sizeof(xarec->eyecatcher)) != 0) {
            rc = ismc_setError(ISMRC_ObjectNotValid, "The XA manager record is not valid");
        }
    }

    if (rc) {
    	unlockSession(session);
        return rc;
    }

    /*
     * Send the manager record create to the server
     */
    act = ismc_newAction(session->connect, session, Action_destroyQMXidRecord);
    /* val0 = xa record id      */
    ism_protocol_putLongValue(&act->buf, (uint64_t)xarec->xa_record_id);		/* BEAM suppression: operating on NULL */
    act->hdr.hdrcount = 1;
    rc = ismc_request(act, 1);

    unlockSession(session);

    if (!rc) {
        memset(xarec->eyecatcher, 0x00, sizeof(xarec->eyecatcher));
    }

    ismc_freeAction(act);

    return rc;
}

/**
 * Get a set of XA records. If successful,
 * you must free the list when done.
 *
 * @param session   The session object.
 * @param manager   The handle of the queue manager record to which XA records belong.
 * @param xarecs    A pointer to the pointer to the list of record handles to be populated.
 * @param count     A pointer to the count of records in the list to be set.
 * @return A return code: 0=good.
 */
int ismc_getXARecords(ismc_session_t * session, ismc_manrec_t manager, ismc_xarec_list_t * * xarecs, int * count) {
    action_t * act;
    ism_field_t field;
    int rc;

    rc = checkAndLockSession(session);
    if (!rc) {
    	if (manager == NULL) {
    		rc = ismc_setError(ISMRC_NullPointer, "The queue manager record is NULL");
    	} else if (memcmp(manager->eyecatcher, QM_RECORD_EYECATCHER, sizeof(manager->eyecatcher)) != 0) {
    		rc = ismc_setError(ISMRC_ObjectNotValid, "The queue manager record is not valid");
    	} else if (xarecs == NULL) {
    		rc = ismc_setError(ISMRC_NullPointer, "The input record list is NULL");
    	} else if (count == NULL) {
    		rc = ismc_setError(ISMRC_NullPointer, "The count pointer is NULL");
    	}
    }

    if (rc) {
        unlockSession(session);
    	return rc;
    }

    /*
     * Send the manager record create to the server
     */
    act = ismc_newAction(session->connect, session, Action_getQMXidRecords);
    /* val0 = manager record id */
    ism_protocol_putLongValue(&act->buf, (uint64_t)manager->managed_record_id);		/* BEAM suppression: operating on NULL */
    act->hdr.hdrcount = 1;
    rc = ismc_request(act, 1);
    unlockSession(session);
    if (rc == 0) {
        /* The header field contains the count of records */
        ism_protocol_getObjectValue(&act->buf, &field);
        if (field.type == VT_Integer) {
            *count = field.val.i;					/* BEAM suppression: dereferencing NULL */

            /* The content of the message is a byte array that contains the list of
             * handles and associated data. */
            ism_protocol_getObjectValue(&act->buf, &field);
            if (field.type != VT_Null) {
                concat_alloc_t map = { 0 };
                ism_field_t recfield;
                int i;

                /* Include NULL-terminator */
                *xarecs = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,56),(*count + 1) * sizeof(struct ismc_xarec_list_t));		/* BEAM suppression: dereferencing NULL */

                map.len = field.len;
                map.buf = field.val.s;
                map.pos = 0;
                map.used = field.len;

                for (i = 0; i < *count; i++) {										/* BEAM suppression: dereferencing NULL */
                    /* Handle - 8-byte long */
                    ism_protocol_getObjectValue(&map, &recfield);
                    (*xarecs)[i].handle = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,57),sizeof(struct ismc_xarec_t));		/* BEAM suppression: dereferencing NULL */
                    memcpy((*xarecs)[i].handle->eyecatcher, XA_RECORD_EYECATCHER, sizeof((*xarecs)[i].handle->eyecatcher));
                    (*xarecs)[i].handle->xa_record_id = recfield.val.l;
                    /* Data - byte-array */
                    ism_protocol_getObjectValue(&map, &recfield);
                    (*xarecs)[i].len = recfield.len;
                    (*xarecs)[i].data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,58),recfield.len);
                    memcpy((*xarecs)[i].data, recfield.val.s, recfield.len);
                }
                (*xarecs)[*count].handle = 0;								/* BEAM suppression: dereferencing NULL */
            } else if (*count > 0) {
                rc = ISMRC_BadClientData;
                ismc_setError(rc, "Queue manager records are incorrect (rc=%d).", rc);
            }
        } else {
            rc = ISMRC_BadClientData;
            ismc_setError(rc, "Queue manager record count is missing (rc=%d).", rc);
        }
    } else {
        ismc_setError(act->rc, "Failed to get queue manager records (rc=%d).", act->rc);
    }

    ismc_freeAction(act);

    return rc;
}

/**
 * Free a list of XA records.
 * @param xarecs    A pointer to the list of record handles to be freed.
 */
void ismc_freeXARecords(ismc_xarec_list_t * xarecs) {
    ismc_xarec_list_t * record = xarecs;
    while (record && record->handle) {
        ism_common_free(ism_memory_ismc_misc,record->handle);
        ism_common_free(ism_memory_ismc_misc,record->data);
        record++;
    }
    ism_common_free(ism_memory_ismc_misc,xarecs);
}

void ismc_freeObject(void * object) {
    if (((ism_obj_hdr_t *)object)->props)
        ism_common_freeProperties(((ism_obj_hdr_t *)object)->props);
    ism_common_free(ism_memory_ismc_misc,object);
}


/**
 * Create a durable subscription and atomically set a message listener for it.
 *
 * @param session  The session object.
 * @param topic    The topic name
 * @param subname  The subscription name
 * @param selector The selector object
 * @param nolocal  The nolocal flag
 * @param onmessage The listener to call when a message arrives
 * @param userdata  The pointer to the user data to be passed to the listener
 * @return A return code: a pointer to the message consumer.
 */
ismc_consumer_t * ismc_subscribe_and_listen(ismc_session_t * session, const char * topic, const char * subname,
        const char * selector, int nolocal, ismc_onmessage_t onmessage, void * userdata) {
    size_t sellen = 0;
    char buf[64];
    ismc_consumer_t * consumer = NULL;
    ismRule_t * selectRule = NULL;
    int rc;

    rc = checkAndLockSession(session);
    if (!rc) {
        if (!topic || !subname) {
            ismc_setError(ISMRC_NoDestination, "Both topic name and subscription name must be specified");
            rc = ISMRC_NoDestination;
        }
    }
    if (!rc) {
        if (selector) {
            /* Compile selector and store it */
            sellen = strlen(selector);
            rc = ism_common_compileSelectRule(&selectRule, NULL, selector);
            if (rc != 0) {
                sellen = 0;
                selectRule = NULL;
            }
        }
    }
    if (!rc) {
        action_t * act;
        int i;
        const char * name;
        ism_field_t field;

        /* Create composite properties from session and destination properties */
        consumer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,63),1, sizeof(ismc_consumer_t)+sellen+1);
        consumer->h.id = OBJID_Consumer;
        consumer->h.state = OBJSTATE_Open;
        pthread_spin_init(&consumer->h.lock, 0);
        consumer->h.props     = ism_common_newProperties(20);
        consumer->h.propcount = 0;
        consumer->session  = session;
        consumer->action   = NULL;
        consumer->domain   = ismc_Topic;
        consumer->nolocal  = nolocal;
        consumer->selector = (const char *)(consumer+1);
        if (!selectRule) {
            consumer->selector = NULL;
        } else {
            strcpy((char *)consumer->selector, selector);
            consumer->selectRule = selectRule;
        }

        /*
         * Engine doesn't support sync ismc_receive yet, so
         * allocate the structure for holding
         * asynchronously received messages that could be requested
         * using ismc_receive.
         */
        consumer->messages = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,64),1, sizeof(ism_common_list));
        ism_common_list_init(consumer->messages, 1, NULL);

        /* Copy properties from session */
        for (i = 0; ism_common_getPropertyIndex(session->h.props, i, &name) == 0; i++) {
            ism_common_getProperty(session->h.props, name, &field);
            ismc_setProperty(consumer, name, &field);
        }

        consumer->disableACK = session->disableACK;

        rc = ism_common_getProperty(consumer->h.props, "RequestOrderID", &field);
        if (rc) {
            consumer->requestOrderID = 0;
        } else {
            consumer->requestOrderID = field.val.i;
        }

        if (onmessage == NULL) {
        	unlockSession(session);
            ismc_setError(ISMRC_NullPointer, "Message listener is required, but not specified");
            return NULL;
        }

        consumer->onmessage = onmessage;
        consumer->userdata = userdata;

        /* Create and send the action */
        act = ismc_newAction(session->connect, session, Action_createDurable);
        act->parseReply = parseReplyConsumer;
        act->userdata = consumer;

        field.type = VT_Boolean;
        field.val.i = 1;
        ismc_setProperty(consumer, "isDurable", &field);

        field.val.i = nolocal;
        ismc_setProperty(consumer, "noLocal", &field);

        ismc_setStringProperty(consumer, "subscriptionName", subname);
        ismc_setStringProperty(consumer, "Name", topic);

        sprintf(buf, "ISMSession@%p", session);
        ismc_setStringProperty(consumer, "Session", buf);

        ism_protocol_putStringValue(&act->buf, subname);            /* val0 = subscription name   */
        ism_protocol_putStringValue(&act->buf, consumer->selector); /* val1 = selector            */
        act->hdr.hdrcount = 2;

        ism_protocol_putMapProperties(&act->buf, consumer->h.props);

        rc = ismc_request(act, 1);
        if (act->rc != 0) {
            unlockSession(session);
            ism_common_setError(rc);
            ism_common_free(ism_memory_ismc_misc,consumer->messages);
            ism_common_free(ism_memory_ismc_misc,consumer);
            ismc_freeAction(act);
            return NULL;
        }

        ismc_freeAction(act);

        // Also send the setMsgListener action
        act = ismc_newAction(consumer->session->connect, consumer->session, Action_setMsgListener);
        act->hdr.item = endian_int32(consumer->consumerid);

        // Issue "setListener" request
        rc = ismc_request(act, 1);

        /* Restart the session */
        act->hdr.action = Action_resumeSession;
        act->parseReply = parseReplyGeneric;
        ismc_request(act, 0);

        ismc_freeAction(act);
        if (rc) {
            ism_common_setError(rc);
            ism_common_free(ism_memory_ismc_misc,consumer);
            consumer = NULL;
        } else {
        	session->deliveryThreadId = ismc_getDeliveryThreadId();
        }
    }

    unlockSession(session);

    return consumer;
}

/**
 * List existing durable subscriptions.
 * If successful, the list must be freed using ismc_freeDurableSubscriptionList.
 *
 * @param session       The session object.
 * @param subscriptions A pointer to the pointer to the list of subscriptions to be populated.
 * @param count         A pointer to the count of subscriptions in the list to be set.
 * @return A return code: 0=good.
 */
int ismc_listDurableSubscriptions(ismc_session_t * session, ismc_durablesub_t * * subscriptions, int * count) {
    action_t * act;
    ism_field_t field;
    int rc;

    if (subscriptions == NULL) {
        return ismc_setError(ISMRC_NullPointer, "The input subscription list is NULL");
    }

    if (count == NULL) {
        return ismc_setError(ISMRC_NullPointer, "The count pointer is NULL");
    }

    rc = checkAndLockSession(session);
    if (rc) {
        unlockSession(session);
        return rc;
    }

    /*
     * Send the list subscriptions request to the server
     */
    act = ismc_newAction(session->connect, session, Action_listSubscriptions);
    rc = ismc_request(act, 1);
    unlockSession(session);

    if (rc == 0) {
        /* The header field contains the count of records */
        ism_protocol_getObjectValue(&act->buf, &field);
        if (field.type == VT_Integer) {
            *count = field.val.i;

            /* The content of the message is a byte array that contains the list of
             * handles and associated data. */
            ism_protocol_getObjectValue(&act->buf, &field);
            if (field.type != VT_Null) {
                concat_alloc_t map = { 0 };
                ism_field_t recfield;
                int i;

                /* Include NULL-terminator */
                *subscriptions = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,68),(*count + 1) * sizeof(struct ismc_durablesub_t));

                map.len = field.len;
                map.buf = field.val.s;
                map.pos = 0;
                map.used = field.len;

                for (i = 0; i < *count; i++) {
                    /* nolocal indicator - boolean */
                    ism_protocol_getObjectValue(&map, &recfield);
                    (*subscriptions)[i].noLocal = recfield.val.i;

                    /* selector - string */
                    if (ism_protocol_getObjectValue(&map, &recfield) == 0 && recfield.type == VT_String) {
                        (*subscriptions)[i].selector = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,69),strlen(recfield.val.s) + 1);
                        strcpy((*subscriptions)[i].selector, recfield.val.s);
                    } else {
                        (*subscriptions)[i].selector = NULL;
                    }

                    /* subscription name - string */
                    ism_protocol_getObjectValue(&map, &recfield);
                    (*subscriptions)[i].subscriptionName = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,70),strlen(recfield.val.s) + 1);
                    strcpy((*subscriptions)[i].subscriptionName, recfield.val.s);

                    /* topic name - string */
                    ism_protocol_getObjectValue(&map, &recfield);
                    (*subscriptions)[i].topicName = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,71),strlen(recfield.val.s) + 1);
                    strcpy((*subscriptions)[i].topicName, recfield.val.s);
                }
                (*subscriptions)[*count].subscriptionName = 0;
            } else if (*count > 0) {
                rc = ISMRC_BadClientData;
                ismc_setError(rc, "List of subscriptions is incorrect (rc=%d).", rc);
            }
        } else {
            rc = ISMRC_BadClientData;
            ismc_setError(rc, "Subscription count is missing (rc=%d).", rc);
        }
    } else {
        ismc_setError(act->rc, "Failed to list subscriptions (rc=%d).", act->rc);
    }

    ismc_freeAction(act);

    return rc;
}

/**
 * Free a list of subscriptions.
 * @param subscriptions  A pointer to the list of subscriptions to be freed.
 */
void ismc_freeDurableSubscriptionList(ismc_durablesub_t * subscriptions) {
    ismc_durablesub_t * sub = subscriptions;
    while (sub && sub->subscriptionName) {
        ism_common_free(ism_memory_ismc_misc,sub->subscriptionName);
        ism_common_free(ism_memory_ismc_misc,sub->selector);
        ism_common_free(ism_memory_ismc_misc,sub->topicName);
        sub++;
    }
    ism_common_free(ism_memory_ismc_misc,subscriptions);
}

int parseReplyGeneric(action_t * action) {
    if (action->hdr.action != Action_reply) {
        action->hdr.action = Action_message;
    }

    /* Locate the return code - it is in the 1st header */
    action->rc = ISMRC_BadClientData;
    if (action->hdr.hdrcount > 0) {
        ism_field_t field;

        ism_protocol_getObjectValue(&action->buf, &field);
        if (field.type == VT_Integer) {
            action->rc = field.val.i;
        }
    }

    return action->rc;
}

int parseReplyConsumer(action_t * action) {
	int rc = parseReplyGeneric(action);

	if (!rc) {
        ism_field_t field;
        ismc_consumer_t * consumer = action->userdata;

        ism_protocol_getObjectValue(&action->buf, &field);
        consumer->consumerid = field.val.i;

        ismc_addConsumerToSession(consumer);
        ismc_addConsumerToConnection(consumer);
	}

	return rc;
}

/**
 * Writes ACK sequence number for all consumers in this session into the action buffer.
 * @param act      A pointer to the action
 * @param session  A pointer to the session
 * @return 0, if no new messages arrived, 1 otherwise
 */
int ismc_writeAckSqns(struct action_t * act, ismc_session_t * session, ismc_consumer_t * consumer) {
	int i;
	int result = 0;

	pthread_mutex_lock(&session->deliverLock);

    if ((session->lastAcked != session->lastDelivered) || (session->acksqn_count > 0)) {
        session->lastAcked = session->lastDelivered;

    	ism_protocol_putIntValue(&act->buf, session->acksqn_count);
    	ism_protocol_putLongValue(&act->buf, session->lastAcked);

    	if (consumer) {
    		ism_protocol_putIntValue(&act->buf, consumer->consumerid);
    		act->hdr.hdrcount = 3;
    	} else {
    		act->hdr.hdrcount = 2;
    	}

    	ism_protocol_putNullValue(&act->buf);   /* Properties */
        ism_protocol_putNullValue(&act->buf);   /* Body */
    	for (i=0; i < session->acksqn_count; i+=2) {
    		ism_protocol_putIntValue(&act->buf, (int)session->acksqn[i]);
    		ism_protocol_putLongValue(&act->buf, session->acksqn[i + 1]);
    	}
        session->acksqn_count = 0;

        result = 1;
    } else {
    	ism_protocol_putIntValue(&act->buf, 0);
    	ism_protocol_putLongValue(&act->buf, 0L);
    }

    pthread_mutex_unlock(&session->deliverLock);

    return result;
}

/**
 * Make a durable subscription with a specified maximum number of messages.
 *
 * @param session     The session object.
 * @param topic       The topic name
 * @param subname     The subscription name  (NULL for MQTT)
 * @param selector    The selector object
 * @param nolocal     The nolocal flag
 * @param maxMessages The maximum number of messages
 * @return A return code: a pointer to the message consumer. If NULL
 *         see ismc_getLastError for more error details.
 */
XAPI ismc_consumer_t * ismc_subscribe_with_options(ismc_session_t * session, const char * topic, const char * subname,
        const char * selector, int nolocal, int maxMessages) {
    size_t sellen = 0;
    char buf[64];
    ismc_consumer_t * consumer = NULL;
    ismRule_t * selectRule = NULL;
    unsigned int rc;

    rc = checkAndLockSession(session);
    if (!rc) {
        if (!topic || !subname) {
            ismc_setError(ISMRC_NoDestination, "Both topic name and subscription name must be specified");
            rc = ISMRC_NoDestination;
        }
    }
    if (!rc) {
        if (selector) {
            /* Compile selector and store it */
            sellen = strlen(selector);
            rc = ism_common_compileSelectRule(&selectRule, NULL, selector);
            if (rc != 0) {
                sellen = 0;
                selectRule = NULL;
            }
        }
    }
    if (!rc && maxMessages <= 0) {
        ismc_setError(ISMRC_ArgNotValid, "Maximum number of messages has to be positive");
        rc = ISMRC_ArgNotValid;
    }
    if (!rc) {
        action_t * act;
        int i;
        const char * name;
        ism_field_t field;

        /* Create composite properties from session and destination properties */
        consumer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,76),1, sizeof(ismc_consumer_t)+sellen+1);
        consumer->h.id = OBJID_Consumer;
        consumer->h.state = OBJSTATE_Open;
        pthread_spin_init(&consumer->h.lock, 0);
        consumer->h.props     = ism_common_newProperties(20);
        consumer->h.propcount = 0;
        consumer->session  = session;
        consumer->action   = NULL;
        consumer->domain   = ismc_Topic;
        consumer->nolocal  = nolocal;
        consumer->selector = (const char *)(consumer+1);
        if (!selectRule) {
            consumer->selector = NULL;
        } else {
            strcpy((char *)consumer->selector, selector);
            consumer->selectRule = selectRule;
        }

        /*
         * Engine doesn't support sync ismc_receive yet, so
         * allocate the structure for holding
         * asynchronously received messages that could be requested
         * using ismc_receive.
         */
        consumer->messages = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,77),1, sizeof(ism_common_list));
        ism_common_list_init(consumer->messages, 1, NULL);

        /* Copy properties from session */
        for (i = 0; ism_common_getPropertyIndex(session->h.props, i, &name) == 0; i++) {
            ism_common_getProperty(session->h.props, name, &field);
            ismc_setProperty(consumer, name, &field);
        }

        consumer->disableACK = session->disableACK;

        rc = ism_common_getProperty(consumer->h.props, "RequestOrderID", &field);
        if (rc) {
            consumer->requestOrderID = 0;
        } else {
            consumer->requestOrderID = field.val.i;
        }

        /* Create and send the action */
        act = ismc_newAction(session->connect, session, Action_createDurable);
        act->parseReply = parseReplyConsumer;
        act->userdata = consumer;

        field.type = VT_Boolean;
        field.val.i = 1;
        ismc_setProperty(consumer, "isDurable", &field);

        field.val.i = nolocal;
        ismc_setProperty(consumer, "noLocal", &field);

        ismc_setStringProperty(consumer, "subscriptionName", subname);
        ismc_setStringProperty(consumer, "Name", topic);

        sprintf(buf, "ISMSession@%p", session);
        ismc_setStringProperty(consumer, "Session", buf);

        ism_protocol_putStringValue(&act->buf, subname);            /* val0 = subscription name   */
        ism_protocol_putStringValue(&act->buf, consumer->selector); /* val1 = selector            */
        ism_protocol_putIntValue(&act->buf, maxMessages);        /* val2 = max messages        */
        act->hdr.hdrcount = 3;

        ism_protocol_putMapProperties(&act->buf, consumer->h.props);
        rc = ismc_request(act, 1);
        if (act->rc != 0) {
            ism_common_setError(rc);
            ism_common_free(ism_memory_ismc_misc,consumer->messages);
            ism_common_free(ism_memory_ismc_misc,consumer);
            consumer = NULL;
        }

        /* Restart the session */
        act->hdr.action = Action_resumeSession;
        act->parseReply = parseReplyGeneric;
        ismc_request(act, 0);

        ismc_freeAction(act);
    }

    unlockSession(session);

    return consumer;
}

XAPI int checkAndLockSession(ismc_session_t * session) {
    if (session == NULL) {
        return ismc_setError(ISMRC_NullPointer, "The session object is NULL");
    }

    if (session->h.id != OBJID_Session) {
        return ismc_setError(ISMRC_ObjectNotValid, "The session object is not valid");
    }

    pthread_mutex_lock(&session->lock);

    if (session->isClosed) {
        return ISMRC_Closed;
    }

    return ISMRC_OK;
}

XAPI int unlockSession(ismc_session_t * session) {
    if (session == NULL) {
        return ismc_setError(ISMRC_NullPointer, "The session object is NULL");
    }

    if (session->h.id != OBJID_Session) {
        return ismc_setError(ISMRC_ObjectNotValid, "The session object is not valid");
    }

	pthread_mutex_unlock(&session->lock);

	return ISMRC_OK;
}

/**
 * Update durable subscription.
 * Currently only max messages can be changed.
 *
 * @param properties  New properties for the subscription
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_updateSubscription(ismc_consumer_t * consumer, ism_prop_t * properties) {
    unsigned int rc;
    ismc_session_t * session;

    if (consumer == NULL) {
        return ismc_setError(ISMRC_NullPointer, "The consumer object is NULL");
    }

    session = consumer->session;

    rc = checkAndLockSession(session);
    if (!rc) {
        const char * subname;
        const char * shared;
        action_t * act;

        /* Create and send the action */
        act = ismc_newAction(session->connect, session, Action_updateSubscription);
        act->parseReply = parseReplyGeneric;

        subname = ismc_getStringProperty(consumer, "subscriptionName");
        shared = ismc_getStringProperty(consumer, "SubscriptionShared");

        ism_protocol_putStringValue(&act->buf, subname);         /* val0 = subscription name */
        act->hdr.hdrcount = 1;

        if (shared) {
        	ism_protocol_putStringValue(&act->buf, shared);          /* val1 = shared */
        	act->hdr.hdrcount++;
        }

        ism_protocol_putMapProperties(&act->buf, properties);

        rc = ismc_request(act, 1);
        if (act->rc != 0) {
            ism_common_setError(rc);
        }

        /* Restart the session */
        act->hdr.action = Action_resumeSession;
        act->parseReply = parseReplyGeneric;
        ismc_request(act, 0);

        ismc_freeAction(act);
    }

    unlockSession(session);

    return rc;

}

void ismc_consumerCachedMessageRemoved(ismc_consumer_t * consumer, const char * methodName, ismc_message_t * msg, int sendResume) {
	int rc;

	pthread_mutex_lock(&consumer->lock);
	consumer->msgCount--;

	// TODO Print the message itself
	TRACE(9, "Remove message from consumer client cache: method=%s consumer=%d sendResume=%d msgCount=%d\n",
			methodName, consumer->consumerid, sendResume, consumer->msgCount);

	if ((consumer->suspendFlags & ISMC_CONSUMER_SUSPEND_0) == 0) {
		pthread_mutex_unlock(&consumer->lock);
		return;
	}
	if (consumer->msgCount == 0) {
		consumer->suspendFlags |= ~ISMC_CONSUMER_SUSPEND_0;
		consumer->suspendFlags |= ~ISMC_CONSUMER_SUSPEND_1;

	    /*
	     * If this is the last pending message and free was called for
	     * this consumer, free it.
	     */
		pthread_spin_lock(&consumer->h.lock);
        if (consumer->h.state == OBJSTATE_Freed) {
        	pthread_spin_unlock(&consumer->h.lock);
			pthread_mutex_unlock(&consumer->lock);
            ismc_freeObject(consumer);
			return;
        }
        pthread_spin_unlock(&consumer->h.lock);
	} else {
		if (((consumer->suspendFlags & ISMC_CONSUMER_SUSPEND_1) == 0) ||
			((consumer->msgCount > consumer->emptySize) == 0)) {
			pthread_mutex_unlock(&consumer->lock);
			return;
		}
		consumer->suspendFlags |= ~ISMC_CONSUMER_SUSPEND_1;
	}
	pthread_mutex_unlock(&consumer->lock);

	if (sendResume) {
		if (consumer->action == NULL) {
			consumer->action = ismc_newAction(consumer->session->connect, consumer->session, Action_startConsumer);
		} else {
			ismc_resetAction(consumer->action, consumer->session->connect, consumer->session, Action_startConsumer);
		}
		consumer->action->hdr.item = endian_int32(consumer->consumerid);
		rc = ismc_request(consumer->action, false);
		if (rc) {
			TRACE(2, "Unable to send resume request to server: consumer=%d ack_sqn=%ld\n", consumer->consumerid, consumer->lastDelivered);
			return;
		}

		TRACE(8, "Start consumer=%d\n", consumer->consumerid);
	}
}

void ismc_consumerCachedMessageAdded(ismc_consumer_t * consumer, action_t * msg) {
	pthread_mutex_lock(&consumer->lock);

	consumer->msgCount++;

	if (msg->hdr.flags & ACTFLAG_Suspended) {
		consumer->suspendFlags |= ISMC_CONSUMER_SUSPEND_0;
		consumer->suspendFlags |= ISMC_CONSUMER_SUSPEND_1;
	}

	// TODO Print the message itself
	TRACE(9, "Add message to consumer client cache: consumer=%d msgCount=%d ack_sqn=%ld\n",
			consumer->consumerid, consumer->msgCount, endian_int64(msg->hdr.msgid));

	pthread_mutex_unlock(&consumer->lock);
}

int ismc_acknowledgeInternalSync(ismc_consumer_t * consumer) {
	int rc = 0;
	ismc_session_t * session = consumer->session;

	if (session->disableACK || session->isClosed)
        return rc;

    if (consumer->action == NULL) {
        consumer->action = ismc_newAction(session->connect, session, Action_ackSync);
    } else {
        ismc_resetAction(consumer->action, session->connect, session, Action_ackSync);
    }

    if (ismc_writeAckSqns(consumer->action, session, consumer)) {
    	ismc_request(consumer->action, 1);
    }

	return rc;
}
