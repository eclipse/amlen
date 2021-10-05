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

#ifndef __ISMC_P_DEFINED
#define __ISMC_P_DEFINED

/** @file ismc_p.h
 * Define private objects for the ISM client implementation
 */
#include <ismc.h>
#include <ismutil.h>
#include <protocol.h>
#include <fendian.h>
#include <selector.h>
#include <imacontent.h>
#include <pthread.h>
#ifndef _WIN32
#include <sys/socket.h>
#endif

#define OBJID_ISM         0x4d534900
#define OBJID_Connection  0x4d534901
#define OBJID_Session     0x4d534902
#define OBJID_Destination 0x4d534903
#define OBJID_Consumer    0x4d534904
#define OBJID_Producer    0x4d534905
#define OBJID_Message     0x4d534906

#define OBJSTATE_Init     0
#define OBJSTATE_Open     1
#define OBJSTATE_Closing  2
#define OBJSTATE_Closed   3
#define OBJSTATE_Freed    4

/*
 * Message priority
 */
#define ismMESSAGE_PRIORITY_MINIMUM          0
#define ismMESSAGE_PRIORITY_DEFAULT          4
#define ismMESSAGE_PRIORITY_MAXIMUM          9

/* Helpful priority constant corresponding to the JMS default priority */
#define ismMESSAGE_PRIORITY_JMS_DEFAULT      4

/*
 * Client message cache.
 */
#define ISMC_DEFAULT_MESSAGE_CACHE_FULL_SIZE		1000
#define ISMC_DEFAULT_MESSAGE_CACHE_EMPTY_SIZE		20

/*
 * Common ISM object header.
 * This is the superclass for all messaging objects
 */
typedef struct ismobj_hdr_t {
    int    id;                   /* Object identifier                               */
    int    state;                /* Current state                                   */
    int    propcount;            /* Property count                                  */
    ism_prop_t * props;          /* Object properties                               */
    pthread_spinlock_t  lock;    /* Synchronize access to properties                */
} ism_obj_hdr_t;

/**
 * Generic expandable list of objects
 */
typedef struct ismc_obj_array_t {
    void ** array;                   /**< Pointer to the list of objects */
    int numElements;                 /**< Number of elements in the list */
    int totalSize;                   /**< Maximum capacity of the list   */
} ismc_obj_array_t;

/*
 * Connection object
 */
struct ismc_connection_t {
    ism_obj_hdr_t     h;               /* Common header                                    */
    SOCKET            sock;            /* Socket                                           */
    struct sockaddr   sockaddr;        /* Resolved socket address                          */
    char              pad[8];          /* sockaddr_in is not actually long enough for IPv6 */
    uint8_t           isConnected;     /* The connection has been connected                */
    uint8_t           isStarted;       /* The connection is started                        */
    volatile uint8_t  isClosed;        /* The connection is closed                         */
    pthread_mutex_t   senderMutex;     /* Lock for sends that use this connection          */
    ismHashMap      * rcvActions;      /* Asynchronous receive actions                     */
    ismc_obj_array_t  sessions;        /* Sessions                                         */
    ismHashMap      * consumers;       /* Consumers                                        */
    ismc_onerror_t    errorListener;   /* The error listener                               */
    void            * userdata;        /* The user data for the error listener             */
    ism_threadh_t     recvThread;      /* Receiver thread                                  */
    pthread_barrier_t barrier;         /* Barrier for starting/stopping receiver thread    */
    pthread_mutex_t   lock;            /* Connection synchronization                       */
    uint8_t           resv[5];
};

struct action_t;

/*
 * Session object
 */
struct ismc_session_t {
    ism_obj_hdr_t  h;            /* Common header                                    */
    ismc_connection_t * connect; /* The connection                                   */
    int      sessionid;          /* The session ID                                   */
    struct action_t * ackAction; /* Action for ACKs                                  */
    uint64_t lastDelivered;      /* Last delivered message                           */
    uint64_t lastAcked;          /* Last acknowledged message                        */
    uint64_t * acksqn;           /* ACK sequence numbers by consumer                 */
    int        acksqn_count;     /* Count of used acksqn entries                     */
    int        acksqn_len;       /* Length of acksqn array                           */
    int      deliveryThreadId;   /* Delivery thread ID, initialized to -1            */
    uint8_t  isClosed;           /* The connection is closed                         */
    uint8_t  transacted;
#define ISMC_LOCAL_TRANSACTION   1
#define ISMC_GLOBAL_TRANSACTION  2
    uint8_t  ackmode;
    uint8_t  disableACK;         /* An indicator of whether ACKs are disabled        */
    ismc_obj_array_t producers;  /* Producers                                        */
    ismc_obj_array_t consumers;  /* Consumers                                        */
    ism_timer_t ackTimer;        /* ACK timer (for DUPS_OK)                          */
    uint8_t  globalTransaction;  /* Session can be used for global transactions      */
    pthread_mutex_t lock;        /* Session synchronization                          */
    pthread_mutex_t deliverLock; /* Delivery lock                                    */
    int32_t fullSize;            /* Client message cache full threshold              */
    int32_t emptySize;           /* Client message cache empty threshold             */
    uint8_t  resv[3];
};

typedef enum ismc_sync_wait_e {
    ISMC_WAITING_MESSAGE  = 0,
    ISMC_MESSAGE_RECEIVED = 1,
    ISMC_TIMED_OUT        = 2,
    ISMC_RECEIVE_FAILED   = 3
} ismc_sync_wait_e;

/**
 * This callback is supplied by the creator of the action
 *
 * @param action
 * @return A return code: 0=good
 */
typedef int (* ism_action_parse_t)(struct action_t * action);

/*
 * Action definition
 */
typedef struct action_t {
    ismc_connection_t * connect;     /* The associated connection                      */
    ismc_session_t    * session;     /* The associated session                         */
    pthread_mutex_t     waitLock;    /* Lock for waiting on sync events                */
    pthread_cond_t      waitCond;    /* Condition for waiting on sync events           */
    ismc_sync_wait_e    doneWaiting; /* 0 - waiting, 1 - have message, 2 - timeout     */
    uint32_t            action_len;  /* Space for a length field                       */
    int32_t             rc;          /* Return code from actions                       */
    ism_action_parse_t  parseReply;
    void              * userdata;
    actionhdr           hdr;
    char                xbuf[64];    /* Space for content fields                       */
    concat_alloc_t      buf;
} action_t;


/*
 * Destination object
 */
struct ismc_destination_t {
    ism_obj_hdr_t  h;            /* Common header                                    */
    const char * name;           /* The topic or queue name                          */
    int   domain;                /* Messaging domain, queue or topic                 */
    int   namelen;               /* The length of the name                           */
};

/*
 * Consumer object
 */
struct ismc_consumer_t {
    ism_obj_hdr_t  h;            /* Common header                                    */
    ismc_session_t * session;    /* The associated session                           */
    ismc_destination_t * dest;   /* The associated destination                       */
    ismc_onmessage_t onmessage;  /* Callback when a message is received              */
    void       * userdata;       /* Userdata for the onmessage callback              */
    const char * selector;       /* The selector string  (malloc)                    */
    ismRule_t  * rule;           /* The selector rule    (malloc)                    */
    action_t   * action;         /* Existing action (ismc_freeAction to free)        */
    ismRule_t  * selectRule;     /* Message selector (ismc_freeSelectRule to free)   */
    ism_common_list * messages;  /* Asynchronously received messages for sync        */
    int          consumerid;     /* The consumer ID                                  */
    uint8_t      domain;         /* Messaging domain, queue or topic                 */
    uint8_t      nolocal;
    uint8_t      isClosed;       /* The connection is closed                         */
    uint8_t      disableACK;     /* An indicator of whether ACKs are disabled        */
    uint8_t      requestOrderID; /* An indicator of whether order ID is requested    */
    uint8_t      suspendFlags;   /* Suspend indicators                               */
#define ISMC_CONSUMER_SUSPEND_0		0x01
#define ISMC_CONSUMER_SUSPEND_1		0x02
    char         resv[2];        /* Reserved                                         */
    int32_t      msgCount;       /* The number of messages scheduled to be delivered */
    int32_t      fullSize;       /* Client message cache full threshold              */
    int32_t      emptySize;      /* Client message cache empty threshold             */
    pthread_mutex_t  lock;       /* Consumer synchronization                         */
    volatile uint64_t lastDelivered;
};

/*
 * Producer object
 */
struct ismc_producer_t {
    ism_obj_hdr_t  h;            /* Common header                                    */
    ismc_session_t * session;    /* The associated session                           */
    ismc_destination_t * dest;   /* The associated destination                       */
    uint64_t     msgCount;       /* The message count associated with with producer  */
    int          producerid;     /* The producer ID                                  */
    uint8_t      domain;         /* Messaging domain, queue or topic                 */
    uint8_t      isClosed;       /* The connection is closed                         */
    char         msgIdBuffer[19];/* The message ID buffer                            */
    uint64_t     msgIdTime;      /* Time when the message ID was created             */
    uint8_t      resv[7];
};


#define HAS_MSGID   0x01
#define HAS_CORRID  0x02
#define HAS_TYPE    0x04
#define HAS_REPLY   0x08
#define HAS_DEST    0x10
#define IS_PERSIST  0x20

/*
 * Message object
 */
struct ismc_message_t {
    ism_obj_hdr_t  h;            /* Common header                                    */
    ismc_session_t * session;    /* The associated session                           */
    uint8_t        msgtype;      /* The type of message content                      */
    uint8_t        priority;     /* The message priority 0-9                         */
    uint8_t        qos;          /* MQTT QoS                                         */
    uint8_t        retain;       /* Retain flag 1=publish 3=deliver                  */
    uint8_t        redelivered;  /* Redelivered count >0 = redelivered               */
    uint8_t        replyDomain;  /* Replyto domain                                   */
    uint8_t        destDomain;   /* Destination domain                               */
    uint8_t        has;
    uint8_t        suspend;      /* Consumer suspended                               */
    char res[7];
    int64_t        timestamp;    /* The message timestamp                            */
    int64_t        expire;       /* The expiration time                              */
    uint64_t       ttl;          /* Time-to-live                                     */
    uint64_t       ack_sqn;      /* ACK sequence number                              */
    uint64_t       order_id;     /* Engine's order ID for this message               */
    concat_alloc_t body;         /* The body data                                    */
};

#define QM_RECORD_EYECATCHER    "REQM"
#define XA_RECORD_EYECATCHER    "REXA"

struct ismc_manrec_t {
    char eyecatcher[4];
    uint64_t managed_record_id;
};

struct ismc_xarec_t {
    char eyecatcher[4];
    uint64_t xa_record_id;
};

/**
 * Allocate structures containing information about/for delivery threads.
 */
void ismc_allocateDeliveryThreads(void);

/**
 * Get the ID of the next available delivery thread.
 * Start it, if not started yet.
 *
 * @return An ID of the available delivery thread.
 */
int ismc_getDeliveryThreadId(void);

/**
 * Add a task for a delivery thread to pass a message to a message listener
 * associated with given consumer.
 *
 * @param  threadId  An ID of the delivery thread to use.
 * @param  consumer  A message consumer to consume the message (via associated listener)
 * @param  message   A received message to consume.
 */
void ismc_addTask(int threadId, ismc_consumer_t * consumer, ismc_message_t * message);

/**
 * Stop all delivery threads and release associated resources.
 */
void stopThreads(void);

/**
 * Obtain a unique thread ID (0-based).
 * @return  A thread id.
 */
int ismc_getThreadId(void);

/*
 * Create a new action.
 * @param  connect A connection.
 * @param  session A session.
 * @param  action  An action type.
 * @return A new action (malloc).
 */
XAPI action_t * ismc_newAction(ismc_connection_t * connect, ismc_session_t * session, int action);

/*
 * Set new fields in existing action (reusing allocated action).
 * @param  act     A pointer to existing action
 * @param  connect A connection.
 * @param  session A session.
 * @param  action  An action type.
 */
XAPI void ismc_resetAction(action_t * act, ismc_connection_t * connect, ismc_session_t * session, int action);

/*
 * Free an action and release underlying resources.
 * @param  act   An action to free
 */
void ismc_freeAction(action_t * act);

/*
 * Get the action we were waiting for.
 * @param  respId  ID of the response.
 * @return An action corresponding to this response.
 */
action_t * ismc_getAction(uint64_t respid);

/**
 * Set action that we need to wait for (sync request).
 * @param  respId  A response ID.
 * @param  action  An action.
 */
void ismc_setAction(uint64_t respid, action_t * action);

/*
 * Send an action to the server
 * @param connect A connection
 * @param action  An action to perform
 * @return 0=good
 */
int ismc_sendAction(ismc_connection_t * connect, action_t * action);

/*
 * Request an action.
 * @param  act  An action to request from the server
 * @param  wait 1, if synchronous request, 0 - asynchronous.
 * @return 0, if success, != otherwise
 */
XAPI int ismc_request(action_t * act, int wait);

/*
 * Create a message ID.
 * The message ID consists of the required characters "ID:" followed by 40 bit
 * timestamp in base 32, a four digit producer unique string, and a four base32 counter.
 * @param producer A message producer
 * @return A pointer to a message ID (non-malloc)
 */
const char * ismc_makeMsgID(ismc_producer_t * producer);

/**
 * Put a map value from properties.
 *
 * A map is represented as a byte array a bytes and a length.  The bytes contain
 * alternating names and values.
 *
 * @param map      The map/message/action to put to
 * @param props    The properties object
 * @param message  The message
 */
XAPI void ismc_putJMSValues(ism_actionbuf_t * map, ism_prop_t * props, ismc_message_t * message, const char * topic);

/**
 * Create a message from the response received from the server.
 * @param session A session.
 * @param action  An action containing server response.
 * @return A new message (malloc)
 */
XAPI ismc_message_t * ismc_makeMessage(ismc_consumer_t * consumer, action_t * action);

/*
 * Do message selection on a message
 */
XAPI int ismc_filterMessage(ismc_message_t * message, ismRule_t * rule);

/**
 * Acknowledge message receipt.
 * @param session A session
 * @param sqn     Message sequence number.
 */
void ismc_acknowledge(ismc_session_t * session, uint64_t sqn);

/**
 * Acknowledge message receipt for the session.
 * @param session A session
 */
void ismc_acknowledgeInternal(ismc_session_t * session);

/**
 * Acknowledge the receipt of the final message for the session.
 * @param session A session
 */
void ismc_acknowledgeFinal(ismc_session_t * session);

int ismc_acknowledgeInternalSync(ismc_consumer_t * consumer);

/**
 * Wake up all threads waiting for server response in ismc_request.
 * This function is used when an error is encountered in communications
 * layer or when the client disconnects.
 * @param conn A connection object that initiated connection.
 * @param rc   An error code.
 */
void ismc_wakeWaiters(ismc_connection_t * conn, int rc);

/**
 * Free ISMC object. Does not do any checks, so
 * validity needs to be ensured prior to calling this function.
 * @param object  A pointer to ISMC object.
 */
void ismc_freeObject(void * object);

int ismc_checkSession(ismc_session_t * session);

/*
 * Start the server
 */
XAPI int ism_transport_startSimpleServer(const char * ipaddr, uint32_t port);

/*
 * Create durable subscription and atomically set the message listener for it.
 * @param session  The session object.
 * @param topic    The topic name
 * @param subname  The subscription name  (NULL for MQTT)
 * @param selector The selector object
 * @param nolocal  The nolocal flag
 * @param onmessage The listener to call when a message arrives
 * @param userdata  The pointer to the user data to be passed to the listener
 * @return A return code: a pointer to the message consumer. If NULL
 *         see ismc_getLastError for more error details.
 */
XAPI ismc_consumer_t * ismc_subscribe_and_listen(ismc_session_t * session, const char * topic, const char * subname,
        const char * selector, int nolocal, ismc_onmessage_t onmessage, void * userdata);

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
        const char * selector, int nolocal, int maxMessages);

/**
 * List existing durable subscriptions.
 * If successful, the list must be freed using ismc_freeDurableSubscriptionList.
 *
 * @param session       The session object.
 * @param subscriptions A pointer to the pointer to the list of subscriptions to be populated.
 * @param count         A pointer to the count of subscriptions in the list to be set.
 * @return A return code: 0=good.
 */
XAPI int ismc_listDurableSubscriptions(ismc_session_t * session, ismc_durablesub_t * * subscriptions, int * count);

/**
 * Free a list of subscriptions.
 * @param subscriptions  A pointer to the list of subscriptions to be freed.
 */
XAPI void ismc_freeDurableSubscriptionList(ismc_durablesub_t * subscriptions);

XAPI void ismc_addConsumerToSession(ismc_consumer_t * consumer);
XAPI void ismc_addConsumerToConnection(ismc_consumer_t * consumer);

XAPI int ismc_writeAckSqns(struct action_t * act, ismc_session_t * session, ismc_consumer_t * consumer);

XAPI int checkAndLockSession(ismc_session_t * session);
XAPI int unlockSession(ismc_session_t * session);

XAPI void ismc_consumerCachedMessageAdded(ismc_consumer_t * consumer, action_t * msg);
XAPI void ismc_consumerCachedMessageRemoved(ismc_consumer_t * consumer, const char * methodName, ismc_message_t * msg, int sendResume);

#endif // __ISMC_P_DEFINED
