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

/**
 * @file ismc.h
 * Define the C client library for IBM MessageSight.
 * This client supports all JMS functionality in a messages, but does not support
 * all of the access methods of JMS, nor impose all of the JMS restrictions.
 * There are no separate methods for the queue and topic domains.
 *
 * There are 6 messaging objects defined:
 * <br> connection
 * <br> session
 * <br> destination
 * <br> consumer
 * <br> producer
 * <br> message
 *
 * All messaging objects have properties which consist of a name,type,value
 * triplet.  Many options are set using properties.
 *
 * All of the objects other than messages are thread-safe and you can them in any thread.
 * This is unlike JMS where all session based objects can be used only in the thread which
 * created the session.  The message object is not thread-safe.  If a message object can be
 * used in more than one thread at a time, the invoker must provide the synchronization.
 *
 * Error conditions are returned as return codes.  A method is provided to get the last
 * bad return code for this thread, and to retrieve a reason string associated with this
 * error.
 */
#ifndef __ISMC_DEFINED
#define __ISMC_DEFINED

#include <ismutil.h>
#include <ismmessage.h>
#include <stdarg.h>
/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * The message objects are encapsulated structures.
 */
typedef struct ismc_connection_t  ismc_connection_t;   /**< The connection object    */
typedef struct ismc_session_t     ismc_session_t;      /**< The session object       */
typedef struct ismc_destination_t ismc_destination_t;  /**< The destination object   */
typedef struct ismc_consumer_t    ismc_consumer_t;     /**< The consumer object      */
typedef struct ismc_producer_t    ismc_producer_t;     /**< The producer object      */
typedef struct ismc_message_t     ismc_message_t;      /**< The message object       */

typedef struct ismc_manrec_t * ismc_manrec_t;
typedef struct ismc_manrec_list_t {
    ismc_manrec_t   handle;
    void *          data;
    int             len;
} ismc_manrec_list_t;

typedef struct ismc_xarec_t * ismc_xarec_t;
typedef struct ismc_xarec_list_t {
    ismc_xarec_t    handle;
    void *          data;
    int             len;
} ismc_xarec_list_t;

typedef struct ismc_durablesub_t {
    char * subscriptionName;
    char * topicName;
    char * selector;
    int    noLocal;
} ismc_durablesub_t;

#define ismc_Queue 1  /**< Messaging domain point to point (queue) */
#define ismc_Topic 2  /**< Messaging domain publication / subscribe (topic) */

#define ismc_QOS0  0x00   /*<< MQTT QoS=0 No quality guarantee */
#define ismc_QOS1  0x01   /*<< MQTT QoS=1 At least once        */
#define ismc_QOS2  0x02   /*<< MQTT QoS=2 Once and only once   */

#define ismc_MqttQualityToQos(x)    (((x) / 2) - 1)
#define ismc_JmsQualityToQos(x)     (((x) / 2) - 1)
#define ismc_MqttQosToQuality(x)    (((x) + 1) * 2)
#define ismc_JmsQosToQuality(x)     (((x) + 1) * 2)

/**
 * JMS session acknowledgment modes.
 * @see javax.jms.Session
 */
enum sessionmode_t {
    SESSION_NO_ACK              = 0,    /**< No ACK (not a JMS option)                */
    SESSION_AUTO_ACKNOWLEDGE    = 1,    /**< Automatic receipt acknowledgment         */
    SESSION_CLIENT_ACKNOWLEDGE  = 2,    /**< Client acknowledgment                    */
    SESSION_DUPS_OK_ACKNOWLEDGE = 3,    /**< Lazy acknowledgment                      */
    SESSION_TRANSACTED          = 0,    /**< Session is transacted, acknowledgment
                                             mode is ignored                          */
    SESSION_TRANSACTED_GLOBAL   = 4     /**< Session is used for global transactions,
                                             acknowledgment mode is ignored           */
};

/**
 * JMS message delivery modes.
 * @see javax.jms.DeliveryMode
 */
enum msgdeliverymode_t {
    ISMC_NON_PERSISTENT         = 1,    /**< Non-persistent message delivery       */
    ISMC_PERSISTENT             = 2     /**< Persistent message delivery           */
};

#ifndef ISMRC_Good
#define ISMRC_Good 0
#endif

/**
 * Recognized flag values in ismc_recoverGlobalTransactions call
 */
#define TMSTARTRSCAN    0x01000000
#define TMENDRSCAN      0x00800000

/**
 * Callback to receive message.
 * This callback is used as a message listener to receive message aysnchronously.
 * @param message   The message
 * @param consumer  The message consumer
 * @param userdata  The userdata associated with the message listener
 */
typedef void (* ismc_onmessage_t )(ismc_message_t * message, ismc_consumer_t * consumer, void * userdata);


/**
 * Callback for errors.
 * This callback is used to an error handler to receive a notification of asynchronous errors.
 * @param  rc       The return code
 * @param  reason   The reason string
 * @param  connect  The connection object
 * @param  session  The session object or NULL if not applicable
 * @param  userdata The user data from the error listener
 */
typedef void (* ismc_onerror_t)(int rc, const char * reason,
		ismc_connection_t * connect, ismc_session_t * session, void * userdata);


/**
 * Create a connection.
 *
 * This creates a connection object but does not establish the connection.
 * After making this call, it is normally necessary to set the connection properties
 * before making the connection using the ismc_connect() method.
 * A connection must be established before a session object is created.
 * @return A connection object
 */
XAPI ismc_connection_t * ismc_createConnection(void);


/**
 * Create the connection with options.
 *
 * This is a convenience method which takes a set of common connection properties and sets them.
 * Any of these values can be NULL and the associated property is not set.
 * <p>
 * This gives the set of properties which are normally required to be set to form a connection.
 * @param clientid  The client identity (ClientID).  Defaults to the hostname.
 * @param protocol  The communications protocol (Protocol).  Defaults to "tcp".
 * @param server    The messaging server (Server).  Defaults to "127.0.0.1".
 * @param port      The port of the messaging server (Port).  Defaults to 16102.  Zero indicates to not set the port.
 * @returns The connection object
 */
XAPI ismc_connection_t * ismc_createConnectionX(const char * clientid, const char * protocol, const char * server, int port);


/**
 * Set the error listener for this connection.
 * @param connect  The connection object
 * @param onerror  The callback to call on an error
 * @param userdata A pointer to return to the callback
 * @return  The previous error listener
 */
XAPI ismc_onerror_t ismc_setErrorListener(ismc_connection_t * connect, ismc_onerror_t onerror, void * userdata);

/**
 * Make the connection.
 *
 * Before calling connect, it is normally necessary to set the clientID, protocol, server, and port for the
 * connection, but protocol and port can often be defaulted.
 *
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_connect(ismc_connection_t * connect);

/**
 * Break the connection.
 *
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_disconnect(ismc_connection_t * connect);

/**
 * Close the connection.
 *
 * After the close, the connection cannot be used for communications.
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_closeConnection(ismc_connection_t * connect);


/**
 * Start the connection.
 *
 * When a connection is created, it is initially in the stopped state and must be started before
 * any messages can be received.  Messages can be sent while the connection is in the stopped
 * state.
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_startConnection(ismc_connection_t * connect);


/**
 * Stop the connection.
 *
 *
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_stopConnection(ismc_connection_t * connect);


/**
 * Free up all resources associated with the MessageSight messaging object.
 *
 * @param connect  An MessageSight messaging object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_free(void * object);


/**
 * Create a session.
 *
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI ismc_session_t * ismc_createSession(ismc_connection_t * connect, int transacted, int ackmode);


/**
 * Close a session.
 *
 * After the close, the connection cannot be used for communications.
 * @param session  The session object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_closeSession(ismc_session_t * session);


/**
 * Commit a session.
 *
 * After the close, the connection cannot be used for communications.
 * @param session  The session object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_commitSession(ismc_session_t * session);


/**
 * Recover a session.
 *
 * After the close, the connection cannot be used for communications.
 * @param session  The session object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_recoverSession(ismc_session_t * session);


/**
 * Rollback a session.
 *
 * After the close, the conection cannot be used for communications.
 * @param session  The session object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_rollbackSession(ismc_session_t * session);


/**
 * Make a durable subscription.
 *
 * @param session  The session object.
 * @param topic    The topic name
 * @param subname  The subscription name  (NULL for MQTT)
 * @param selector The selector object
 * @param nolocal  The nolocal flag
 * @return A return code: a pointer to the message consumer. If NULL
 *         see ismc_getLastError for more error details.
 */
XAPI ismc_consumer_t * ismc_subscribe(ismc_session_t * session, const char * topic, const char * subname,
        const char * selector, int nolocal);

/**
 * Update durable subscription.
 * Currently only max messages can be changed.
 *
 * @param consumer    The consumer object.
 * @param properties  New properties for the subscription
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_updateSubscription(ismc_consumer_t * consumer, ism_prop_t * properties);

/**
 * Remove a durable subscription.
 *
 * @param session  The session object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_unsubscribe(ismc_session_t * session, const char * name);


/**
 * Create a queue.
 *
 * @param name   The name of the queue
 * @return A return code: A pointer to the object.  If NULL see ismc_getLastError for more error details.
 */
XAPI ismc_destination_t * ismc_createQueue(const char * name);


/**
 * Create a topic.
 *
 * @param name   The name of the topic
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI ismc_destination_t * ismc_createTopic(const char * name);


/**
 * Create a message consumer.
 *
 * @param session  The session object.
 * @param dest     The topic or queue object
 * @param selector The selector object
 * @param nolocal  The nolocal flag
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI ismc_consumer_t * ismc_createConsumer(ismc_session_t * session, ismc_destination_t * dest, const char * selector, int nolocal);


/**
 * Close a message consumer.
 *
 * @param consumer  The consumer object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_closeConsumer(ismc_consumer_t * consumer);


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
XAPI int ismc_receive(ismc_consumer_t * consumer, int64_t timeout, ismc_message_t **pmessage);


/**
 * Set a message listener.
 *
 * @param consumer  The consumer object.
 * @param onmessage The listener to call when a message arrives.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_setlistener(ismc_consumer_t * consumer, ismc_onmessage_t onmessage, void * userdata);


/**
 * Create a message producer.
 *
 * Unlike in JMS, you must provide a destination for a producer.
 * @param session  The session object.
 * @param dest     The topic or queue object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI ismc_producer_t * ismc_createProducer(ismc_session_t * session, ismc_destination_t * dest);


/**
 * Close a message producer.
 *
 * @param consumer  The  producer object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_closeProducer(ismc_producer_t * producer);


/**
 * Send a message.
 *
 * @param producer  The producer object.
 * @param message   The message to send.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI int ismc_send(ismc_producer_t * producer, ismc_message_t * message);


/*
 * Send a message without a producer
 * @param session  The session object
 * @param domain   The messaging domain
 * @param tqname   The name of the topic or queue
 * @param message  The message to send.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_sendX(ismc_session_t * session, int domain, const char * tqname, ismc_message_t * message);


/**
 * Create a message.
 *
 * @param consumer  The consumer object.
 * @param msgtype   The message type.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
XAPI ismc_message_t * ismc_createMessage(ismc_session_t * session, enum msgtype_e msgtype);


/**
 * Create a message with extended properties.
 *
 * @param  session  The session the message is in
 * @param  type     The message type
 * @param  priority The message priority
 * @param  qos      The quality of service
 * @param  retain   The message retain flag
 * @param  ttl      The message time to live
 */
XAPI ismc_message_t * ismc_createMessageX(ismc_session_t * session, enum msgtype_e msgtype,
                                          int priority, int qos, int retain, uint64_t ttl);

/**
 * Free the message and release resources associated with it
 * @param  message  The message
 */
void ismc_freeMessage(ismc_message_t * message);

/**
 * Get the type of the message
 * @param  The message
 * @return THe type of the message (MTYPE_)
 */
XAPI int ismc_getMessageType(ismc_message_t * message);

/**
 * Set the correlation ID of the message.
 * Only up to 24 bytes of this NULL-terminated string will be used.
 *
 * The correlation ID is defined by the application.
 * @param message  A message
 * @param corrid   A NULL-terminated string representing correlation
 *                 identifier. Must not start with "ID:".
 * @return A return code: 0=good
 */
XAPI int ismc_setCorrelationID(ismc_message_t * message, const char * corrid);

/**
 * Get the correction ID of the message.
 *
 * @param message  A message
 */
XAPI const char * ismc_getCorrelationID(ismc_message_t * message);


/**
 * Set the delivery mode of the message as an integer value.
 * @param message  A message
 * @param delmode the delivery mode 0=non-persistent, 1=persistent
 * @return A return code: 0=good
 *
 */
XAPI int ismc_setDeliveryMode(ismc_message_t * message, int delmode);

/**
 * Get the delivery mode of the message as an integer value.
 * @param message  A message
 * @return the delivery mode 0=non-persistent, 1=persistent
 */
XAPI int ismc_getDeliveryMode(ismc_message_t * message);

/**
 * Set the message ID of the message.
 * This messageID is replaced by the system generated messageID when a message is sent.
 * @param message  A message
 * @param msgid    A message identifier represented by a NULL-terminated string.
 *                 Only up to 24 characters are used.
 * @return A return code: 0=good
 */
XAPI int ismc_setMessageID(ismc_message_t * message, const char * msgid);

/**
 * Get the message ID of the message.
 * The message ID is automatically created when a message is sent unless the
 * DisableMessageID property is set on the message producer in which case the
 * message ID of the message is set to null.
 * @param message  A message
 * @return The message ID or NULL to indicate there is no message ID
 */
XAPI const char * ismc_getMessageID(ismc_message_t * message);

/**
 * Set the priority of the message.
 * @param message  A message
 * @param priority The message priority as a value 0 to 9.  The value 4 is a normal message.
 * @return A return code: 0=good
 */
XAPI int ismc_setPriority(ismc_message_t * message, int priority);

/**
 * Get the message priority.
 * @param message  A message
 * @return The message priority as a value 0 to 9.  The value 4 is a normal priority.
 */
XAPI int ismc_getPriority(ismc_message_t * message);

/**
 * Set the redelivered flag.
 * This flag is automatically set when a message is received.
 * @param message  A message
 * @param redelivered The redelivered flag, 0=not redelivered
 * @return A return code: 0=good
 */
XAPI int ismc_setRedelivered(ismc_message_t * message, int redelivered);

/**
 * Return the redelivered flag
 * @param message  A message
 * @return The redelivered flag, 0=not redelivered
 */
XAPI int ismc_getRedelivered(ismc_message_t * message);

/**
 * Set the reply to destination
 *
 * @param message  A message
 * @param The destination name
 * @param The destination domain (1=queue, 2=topic)
 * @return A return code: 0=good
 */
XAPI int ismc_setReplyTo(ismc_message_t * message, const char * dest, int domain);

/**
 * Return the reply to destination
 *
 * @param message  A message
 * @param domain   The return location for the domain.  If non-null return the domain in this variable.
 *                 This can have the values: 0=unknown, 1=queue, 2=topic.
 * @return The name of the destination or NULL to indicate the destination name is unknown.
 */
XAPI const char * ismc_getReplyTo(ismc_message_t * message, int * domain);

/**
 * Set the original destination.
 *
 * The original destination is reset when a message is sent.
 * @param message  A message
 * @param The destination name
 * @param The destination domain (1=queue, 2=topic)
 * @return A return code: 0=good
 */
XAPI int ismc_setDestination(ismc_message_t * message, const char * dest, int domain);

/**
 * Return the original destination.
 *
 * If the message contains its original destination, return it.  This can be used to find
 * the original topic on which the message was published when using wild card subscriptions.
 *
 * @param message  A message
 * @param domain   The return location for the domain.  If non-null return the domain in this variable.
 *                 This can have the values: 0=unknown, 1=queue, 2=topic.
 * @return The name of the sent to destination or NULL to indicate the destination name is unknown.
 */
XAPI const char * ismc_getDestination(ismc_message_t * message, int * domain);

/**
 * Set the quality of service of a message
 *
 * The quality of service is not used by JMS, but can be set and returned for messages
 * interchanged with other messaging protocols.  The quality of service has a value of 0-15.
 *
 * @param message  A message
 * @param quality  The quality of service (0-15)
 * @return A return code: 0=good
 */
XAPI int ismc_setQuality(ismc_message_t * message, int qos);

/**
 * Return quality of service of a message
 *
 * The quality of service is not used by JMS, but can be set and returned for messages
 * interchanged with other messaging protocols.  The quality of service has a value of 0-7.
d
 *
 * @param message  A message
 * @return The quality of service.
 */
XAPI int ismc_getQuality(ismc_message_t * message);

/**
 * Set the retrain flag of a message
 *
 * Retain is not used by JMS, but can be set and returned for messages
 * interchanged with other messaging protocols.  The retain flag is boolean.
 *
 * @param message  A message
 * @param retain  The quality of service (0-7)
 * @return A return code: 0=good
 */
XAPI int ismc_setRetain(ismc_message_t * message, int retain);

/**
 * Return the retain flag of a message
 *
 * Retain is not defined by JMS, but can be set and returned for messages
 * interchanged with other messaging protocols.  The retain flag is boolean.
 *
 * @param message  A message
 * @return The retain flag.
 */
XAPI int ismc_getRetain(ismc_message_t * message);

/**
 * Return the propagate retain flag of a message
 *
 * Retain is defined by JMS, but can be set and returned for messages
 * interchanged with other messaging protocols.  This flag indicates whether
 * the retain property should be propagated if this client is acting as a
 * forwarder of messages.
 *
 * @param message  A message
 * @return The propagate retain flag.
 */
XAPI int ismc_getProgagateRetain(ismc_message_t * message);

/**
 * Set the timestamp of the message as a millisecond timestamp.
 * The message timestamp is automatically set when a message is sent.
 * @param message  A message
 * @param tsmillis The timestamp
 * @return A return code: 0=good
 */
XAPI int ismc_setTimestamp(ismc_message_t * message, int64_t tsmillis);

/**
 * Get the timestamp of the message as a millisecond timestamp.
 * The timestamp uses the Java and Unix epoch starting on 1970-01-01:T00UTC and
 * ignoring leap seconds.
 * @param message  A message
 */
XAPI int64_t ismc_getTimestamp(ismc_message_t * message);

/**
 * Set the expiration time of a message.
 * The expiration time is automatically set when a message is sent.
 * @param message  A message
 * @param tsmilli  The expiration time
 * @return A return code: 0=good
 */
XAPI int ismc_setExpiration(ismc_message_t * message, int64_t tsmillis);

/**
 * Get the timestamp of the message as a millisecond timestamp.
 * The timestamp uses the Java and Unix epoch starting on 1970-01-01:T00UTC and
 * ignoring leap seconds.
 * @param message  A message
 */
XAPI int64_t ismc_getTimestamp(ismc_message_t * message);

/**
 * Set the expiration time of a message.
 * The expiration time is automatically set when a message is sent.
 * The expiration time uses the Java and Unix epoch starting on 1970-01-01:T00UTC and
 * ignoring leap seconds.
 * @param message  A message
 * @param tsmilli  The expiration time
 * @return A return code: 0=good
 */
XAPI int ismc_setExpiration(ismc_message_t * message, int64_t tsmillis);

/**
 * Get the expiration time of a message.
 * The expiration time uses the Java and Unix epoch starting on 1970-01-01:T00UTC and
 * ignoring leap seconds.
 * @param message  A message
 * @return The expiration time, or zero to indicate the message does not expire.
 */
XAPI int64_t ismc_getExpiration(ismc_message_t * message);

/**
 * Set the time to live of a message.
 * The time to live is in milliseconds.
 * @param message  A message
 * @param millis  The time to live or zero to indicate that the message does not expire
 * @return A return code: 0=good
 */
XAPI int ismc_setTimeToLive(ismc_message_t * message, int64_t millis);

/**
 * Get the time to live of a message.
 * The time to live is in milliseconds.
 * @param message  A message
 * @return The time to live, or zero to indicate the message does not expire.
 */
XAPI int64_t ismc_getTimeToLive(ismc_message_t * message);

/**
 * Set a message type string.
 * This is a user defined string.
 * @param message  A message
 * @param jmstype  The type string.
 * @return A return code: 0=good
 */
XAPI int ismc_setTypeString(ismc_message_t * message, const char * jmstype);

/**
 * Get the message type string.
 * This is a user defined string.
 * @param message  A message
 * @return A type string or NULL to indicate there is no type string
 */
XAPI const char * ismc_getTypeString(ismc_message_t * message);

/**
 * Set a property on an MessageSight object.
 *
 * All messaging objects have properties.  For objects other than messages, the
 * properties are inherited from the object used to create the object.
 * Properties can be changed at any time, but some only have an effect when
 * a sub-object is created from this object.
 * <p>
 * For the message object, there are two kinds of properties: the system
 * properties and user defined properties.  Any property starting with the
 * characters JMS (except those starting with JMSX) is a system property.
 * Such properties are not allowed as user defined properties.
 * <p>
 * Setting properties is thread-safe for all objects except messages.  When
 * working on message properties the application must provide its own
 * sychronization.
 * <p>
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The MessageSight C client does not allow you to set
 *                 system properties (starting with JMS).
 * @param  field   The field containing the value.  Most field types are allowed,
 *                 but JMS folds many of these together (or instance the unsigned
 *                 scalar fields).  Name and NameIndex field types cannot be used.
 * @return  A return code: 0=good
 */
XAPI int ismc_setProperty(void * object, const char * name, ism_field_t * field);


/**
 * Set a string property on an MessageSight object.
 *
 * Set the property for an object with the a type of null terminated string.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The MessageSight C client does not check for valid
 *                 JMS names.
 * @param  value   The value as a null terminated string encoded in UTF-8.
 * @return A return code: 0=good
 */
XAPI int ismc_setStringProperty(void * object, const char * name, const char * value);


/**
 * Set an integer property on an MessageSight object.
 *
 * This can be used to set any of the data types represented as integer including:
 * Null, Boolean, Byte, Integer, Short, UByte, UInt, and UShort.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The MessageSight C client does not check for valid
 *                 JMS names.
 * @param  datatype The datatype of the integer field
 * @param  value   The integer value.
 * @return A return code: 0=good
 *
 */
XAPI int ismc_setIntProperty(void * object, const char * name, int value, int datatype);


/**
 * Get a property from an MessageSight object.
 *
 * Most of the defined field types are possible, but JMS only produces a subset
 * of them.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The MessageSight C client allows you to read system properties
 *                 (those starting with JMS) using this interface.
 * @param  field   A field into which to return the property.
 * @return A return code: 0=good
 */
XAPI int ismc_getProperty(void * object, const char * name, ism_field_t * field);


/**
 * Get a string property from an MessageSight object.
 *
 * Type conversion is not done with the restricted type conversion of JMS.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The MessageSight C client does not check for valid
 *                 JMS names.
 * @return A return code: 0=good
 */
XAPI const char * ismc_getStringProperty(void * object, const char * name);


/**
 * Get an integer property from an MessageSight object
 *
 * Type conversion is not done with the restricted type conversion of JMS.
 * This method can also be used to get a boolean property.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The MessageSight C client does not check for valid
 *                 JMS names.
 * @param  default_val  The default value to return when the property is missing
 *                 or cannot be converted to an integer.
 * @return A return code: 0=good
 */
XAPI int ismc_getIntProperty(void * object, const char * name, int default_val);


/**
 * Get the connection associated with an object.
 *
 * Return the Connection property associated with the object as a connection object.
 * @param An IMS messaging object
 * @return A connection object or NULL to indicate there is no associated connection.
 */
XAPI ismc_connection_t * ismc_getConnection(void * object);


/**
 * Get the session associated with an object.
 *
 * Return the Session property associated with the object as a session object.
 * @param An IMS messaging object
 * @return A connection object or NULL to indicate there is no associated session.
 */
XAPI ismc_session_t * ismc_getSession(void * object);


/**
 * Clear the properties of an object.
 * This can clear the properties of any MessageSight client object.
 * @return A return code: 0=good
 */
XAPI int ismc_clearProperties(void * object);


/**
 * Set the bytes contents of a message.
 * Replace the bytes content of the message.
 * @return A return code: 0=good
 */
XAPI int ismc_setContent(ismc_message_t * message, const char * bytes, int len);


/**
 * Get the size of the content of a message in bytes.
 * The content of any message type has a message length in bytes.
 * @param message The message
 * @return A return code: 0=good
 */
XAPI int ismc_getContentSize(ismc_message_t * message);


/**
 * Get the count of content items in the message.
 * For text and bytes messages this is either 0 or 1.
 * For map and stream messages it gives the count of items.
 * @param message The message
 * @return A count of items in the message
 */
XAPI int ismc_getContentCount(ismc_message_t * message);


/**
 * Get the content of a message as bytes.
 * The content of any message type can be returned as bytes.
 * @param message  The message
 * @param buffer   The address to put the data
 * @param start    The starting offset within the message data
 * @param len      The length of the buffer to return the data
 * @return A return code: 0=good
 */
XAPI int ismc_getContent(ismc_message_t * message, char * buffer, int start, int len);


/**
 * Set the contents of a message as a string.
 * Replace the current contents of the message with the specified string.
 * @param message  The message
 * @param string   The string as UTF-8 to use as the text of the message
 * @return A return code: 0=good
 */
XAPI int ismc_setContentString(ismc_message_t * message, const char * string);


/**
 * Set the contents of a message as a name,type,value field.
 *
 * @param message  The message
 * @param name     The name of the field.  This must be a valid name.
 * @param field The return location of the data.
 * @return A return code: 0=good
 */
XAPI int ismc_addMapContent(ismc_message_t * message, const char * name, ism_field_t * field);


/**
 * Set the contents of a message as a type,value field.
 * Fields are put in the message in the order they are added.
 * @param message  The message
 * @param name     The name of the field
 * @param field The return location of the data.
 * @return A return code: 0=good
 */
XAPI int ismc_addStreamContent(ismc_message_t * message, ism_field_t * field);


/**
 * Get the contents of a message as a name,type,value field
 * This implements the MapMessage pattern.
 * When the field contains a pointer it is into the message data and is only valid as long
 * as the message is valid.
 * @param message  The message
 * @param field The return location of the data.
 * @return A return code: 0=good
 */
XAPI int ismc_getContentField(ismc_message_t * messaage, const char * name, ism_field_t * field);


/**
 * Get the contents of a message as a type,value field by index.
 * This implements the StreamMessage pattern.
 * When the field contains a pointer it is into the message data and is only valid as long
 * as the message is valid.
 * @param message  The message
 * @param index    The index of the field to return
 * @param field The return location of the data.
 * @return A return code: 0=good
 */
XAPI int ismc_getContentFieldIx(ismc_message_t * message, int index, ism_field_t * field);

/**
 * Clear the content of a message
 * @param message  The message to modify.
 * @return A return code: 0=good
 */
XAPI int ismc_clearContent(ismc_message_t * message);

/**
 * Set the last error for this thread.
 * @param rc      The error code.
 * @param reason  The reason string.  This is not required for one of the
 * @param ...     Replacement strings
 * @return The return code
 */
XAPI int ismc_setError(int rc, const char * reason, ...);


/**
 * Get the last error in this thread.
 * The reason string of the last error is filled in the supplied buffer.
 * @param errbuf  The buffer to return the error reason string
 * @param len     The length of the error buffer.
 * @return The last return code
 */
XAPI int ismc_getLastError(char * errbuf, int len);

/**
 * Get error string.
 * The reason string of the last error is filled in the supplied buffer.
 * Since no replacement data is available, any message with replacement data is
 * returned as a Java message format string.
 * @param rc      The return code to show the reason for
 * @param errbuf  The buffer to return the error reason string
 * @param len     The length of the error buffer.
 */
XAPI const char * ismc_getErrorString(int rc, char * errbuf, int len);

/**
 * Set a keepalive value for this connection.
 * @param conn      The connection object.
 * @param keepalive The keep alive interval (in seconds).
 */
XAPI void ismc_setKeepAlive(ismc_connection_t * conn, int keepalive);

/**
 * Create a manager record.
 * @param session   The session object
 * @param data        The data for the record.
 * @param len        The length of the data.
 * @return A queue manager record handle
 */
XAPI ismc_manrec_t ismc_createManagerRecord(ismc_session_t * session, const void * data, int len);

/**
 * Delete a manager record.
 * @param session   The session object
 * @param manrec    The record handle.
 * @return A return code: 0=good.
 */
XAPI int ismc_deleteManagerRecord(ismc_session_t * session, ismc_manrec_t manrec);

/**
 * Get a set of manager records. If successful,
 * you must free the list when done.
 *
 * @param session   The session object.
 * @param manrecs    A pointer to the pointer to the list of record handles to be populated.
 * @param count     A pointer to the count of records in the list to be set.
 * @return A return code: 0=good.
 */
XAPI int ismc_getManagerRecords(ismc_session_t * session, ismc_manrec_list_t * * manrecs, int * count);

/**
 * Free a list of manager records.
 * @param manrecs    A pointer to the list of record handles to be freed.
 */
XAPI void ismc_freeManagerRecords(ismc_manrec_list_t * manrecs);

/**
 * Create an XA record.
 * @param session   The session object
 * @param manager   The handle of the queue manager record to which
 *                  this XA record will belong.
 * @param data        The data for the record.
 * @param len        The length of the data.
 * @return A queue manager record handle
 */
XAPI ismc_xarec_t ismc_createXARecord(ismc_session_t * session, ismc_manrec_t manager, const void * data, int len);

/**
 * Delete an XA record.
 * @param session   The session object
 * @param xarec        The record handle.
 * @return A return code: 0=good.
 */
XAPI int ismc_deleteXARecord(ismc_session_t * session, ismc_xarec_t xarec);

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
XAPI int ismc_getXARecords(ismc_session_t * session, ismc_manrec_t manager, ismc_xarec_list_t * * xarecs, int * count);

/**
 * Free a list of XA records.
 * @param xarecs    A pointer to the list of record handles to be freed.
 */
XAPI void ismc_freeXARecords(ismc_xarec_list_t * manrecs);

/**
 * Get the order ID for this message.
 * @param message   The message object.
 * @return An order ID, if available; otherwise 0.
 */
XAPI uint64_t ismc_getOrderID(ismc_message_t * message);

XAPI int ismc_startGlobalTransaction(ismc_session_t * session, ism_xid_t * xid);

XAPI int ismc_endGlobalTransaction(ismc_session_t * session);

XAPI int ismc_prepareGlobalTransaction(ismc_session_t * session, ism_xid_t * xid);

XAPI int ismc_commitGlobalTransaction(ismc_session_t * session, ism_xid_t * xid, int onePhase);

XAPI int ismc_rollbackGlobalTransaction(ismc_session_t * session, ism_xid_t * xid);

/**
 * Request an array of the XIDs known to the MessageSight Server to be returned
 * in the buffer provided. Count defines the number of XID items that
 * would fit in the buffer. This function call establishes a cursor into
 * the XID table so repeated calls may be required in order that all of
 * the known XIDs are returned. A flags value of TMSTARTRSCAN will cause
 * the cursor to be reset to the start of the list before the list of XIDs
 * is returned. A flags value of TMENDRSCAN will cause the cursor to be reset
 * after the list of XIDs is returned. Both flags values may be specified
 * concurrently on a single invocation.
 *
 * @param session
 * @param xidBuffer  An array of XID items
 * @param count      The number of entries that fit in xidBuffer
 * @param flags
 * @return Number of entries placed in the xidBuffer
 */
XAPI int ismc_recoverGlobalTransactions(ismc_session_t * session, ism_xid_t * xidBuffer, int count, int flags);

#ifdef __cplusplus
}
#endif
#endif
