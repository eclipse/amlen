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

#include <ismc_p.h>

int ismc_connect(ismc_connection_t * conn) {
    if (!conn)
    	return ismc_setError(ISMRC_NullPointer, "The connection object is NULL");
	if (conn->h.id != OBJID_Connection)
    	return ismc_setError(ISMRC_ObjectNotValid, "The connection object is not valid");
	if (ismc_getStringProperty(conn, "ClientID") == NULL)
		return ismc_setError(ISMRC_ClientIDRequired, "The client ID for connection object is required");

	conn->isConnected = 1;
	return 0;
}

int ismc_sendAction(ismc_connection_t * conn, action_t * action) {
    actionhdr * header = &action->hdr;
    int messageType = header->itemtype;
    int respId = 0;
    ismc_message_t * message;
    action_t * act;

    if (action->hdr.action == Action_ack || action->hdr.action == Action_resumeSession) {
        return 0;
    }

    if (action->hdr.action == Action_message || action->hdr.action == Action_messageWait) {
        respId = 0;
        messageType = ITEMT_None;
    }

    if (messageType == ITEMT_None) {		// Message
        int consumerId = 2;
        ismc_session_t * session = NULL;
        ismc_consumer_t * consumer = NULL;

        /*
         * 1. Find consumer
         * 2. Add task to call the message listener associated with the consumer
         */
        consumer = ism_common_getHashMapElementLock(conn->consumers, &consumerId, sizeof(int32_t));
        if (consumer) {
            session = consumer->session;
        }

        if (!consumer) {
        	TRACE(5, "Received a message for unknown consumer with ID %d\n", consumerId);
        	return 0;
        }

        /*
         * If the consumer/message listener is not set, queue the message
         * for sync receive
         */
        if (consumer->onmessage == NULL) {
        	/* TODO
        	 * Scaffolding to add functionality missing in the engine -
        	 * sync message delivery through async receipt and delivery
        	 * upon ismc_request */
            act = ismc_newAction(conn, NULL, 0);
            memcpy(&act->hdr, header, sizeof(actionhdr));
//            act->hdr.response_id = respId;
            act->hdr.item = consumerId;
            act->hdr.msgid = endian_int64(consumer->lastDelivered + 1);
            act->buf.used = 0;
            act->buf.pos = 0;
            ism_common_allocBufferCopyLen(&act->buf, action->buf.buf, action->buf.used);

            ism_common_list_insert_tail(consumer->messages, act);
            act->rc = 0;

            if (action->hdr.action == Action_messageWait) {
            	action->doneWaiting = ISMC_MESSAGE_RECEIVED;
            }

            return 0;
        }

        act = ismc_newAction(conn, consumer->session, 0);
        memcpy(&act->hdr, header, sizeof(actionhdr));
//        act->hdr.response_id = respId;
        act->buf.used = 0;
        act->buf.pos = 0;
        ism_common_allocBufferCopyLen(&act->buf, action->buf.buf, action->buf.used);

        message = ismc_makeMessage(consumer, act);

        ismc_freeAction(action);

        if (!message) {
            return 0;
        }

        /* Apply message selector, if present, before delivering the message */
        if (!consumer->selectRule ||
                (ismc_filterMessage(message, consumer->selectRule) != SELECT_FALSE)) {
            /* Create new task for delivery thread */
            ismc_addTask(session->deliveryThreadId, consumer, message);
        }

    } else {
        respId = endian_int64(header->msgid);
        act = ismc_getAction(respId);

        /* Complete the action */
        pthread_mutex_lock(&act->waitLock);

        if (act->doneWaiting == ISMC_WAITING_MESSAGE) {
            act->doneWaiting = ISMC_MESSAGE_RECEIVED;
            ismc_setAction(respId, NULL);

//            memcpy(&act->hdr, header, sizeof(actionhdr));
            act->buf.used = 0;
            act->buf.pos = 0;
//            ism_common_allocBufferCopyLen(&act->buf, (char*)(header + 1), action->action_len - sizeof(actionhdr));

            if (act->hdr.action == Action_createConsumer ||
            		act->hdr.action == Action_createDurable ||
            		act->hdr.action == Action_createBrowser) {
                ismc_consumer_t * consumer = action->userdata;
                consumer->consumerid = 2;

                ismc_addConsumerToSession(consumer);
                ismc_addConsumerToConnection(consumer);
            } else {
                act->parseReply(act);
            }
        }
        act->rc = 0;
        pthread_cond_signal(&act->waitCond);
        pthread_mutex_unlock(&act->waitLock);
    }

    return 0;
}

/*
 * Disconnect from the server
 *
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_disconnect(ismc_connection_t * connect) {
    action_t * act = ismc_newAction(connect, NULL, Action_closeConnection);
    connect->isConnected = 0;
    ismc_request(act, 1);
    act->hdr.action = Action_termConnection;
    ismc_request(act, 1);

    ismc_freeAction(act);

    connect->isClosed = 1;

    return 0;
}
