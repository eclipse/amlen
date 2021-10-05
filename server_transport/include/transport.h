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

#ifndef __TRANSPORT_DEFINED
#define __TRANSPORT_DEFINED

#include <ismutil.h>
#include <netinet/tcp.h>

typedef concat_alloc_t ism_map_t;

/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flags used by send.  This can be ORed together
 */
#define SFLAG_ASYNC       0x0001    /* The send is asynchronous  */
#define SFLAG_SYNC        0x0002    /* The send is synchronous (the caller is probably waiting) */
#define SFLAG_FRAMESPACE  0x0004    /* The buffer contains space for the frame before the content */
#define SFLAG_HASFRAME    0x0008    /* Do not add a frame since it has one already */
#define SFLAG_OUTOFORDER  0x0010    /* Send bypassing "transport suspended" check  */

/*
 * Return values used by send.
 */
#define SRETURN_OK        0         /* Data was sent successfully                    */
#define SRETURN_BAD_STATE 1001      /* Transport is not in open state                */
#define SRETURN_CLOSED    1002      /* Connection closed                             */
#define SRETURN_SUSPEND   1003      /* Send buffer is full, suspend message delivery */
#define SRETURN_ALLOC     1004      /* Allocation error                              */


#define MAX_FIRST_MSG_LENGTH    0xFFFF

/**
 * @file transport.h
 *
 * The transport component of ISM defines the communication of actions and messages between on the
 * server side of client/server communications.  The primary object of the transport component
 * is the transport object.  This represents a logical connection between the client and server
 * (or between two servers).  There can be multiple transport implementations in the ISM server.
 * <p>
 * Each transport object defines the core methods of the transport implementation, and also defines a framer
 * and a protocol.  In some cases there is a single framer and a single protocol for a a transport
 * implementation.  In other cases the framer is defined by the first packet received on the connection.
 * <p>
 * In the case of the TCP transport, the transport object represents a TCP connection.  There are four
 * built-in framers: JMS, MQTT, plug-in, and WebSockets.  Each of these has a matching protocol.
 * An external framer can be registered for use by the plug-in processing.
 */
typedef struct ism_delivery_t  ism_delivery_t;

typedef struct ism_transport_t ism_transport_t;

/**
 * Send a buffer to the client.
 *
 * This callback is supplied by the transport.
 *
 * The buffer supplied is valid until the function returns.  Returning from this function does not
 * imply that the buffer has been sent to the client, only that it is scheduled.
 *
 * @param transport  The transport object
 * @param buf        The data to send to the client
 * @param len        The length of the data
 * @param frame      A frame specific value.  This is used when one of the protocol fields is in the frame.
 * @param flags      A set of send flags (see SFLAG_)
 * @return A return code: 0=good
 */
typedef int (* ism_transport_send_t)(ism_transport_t * transport, char * buf, int len, int frame, int flags);

/**
 * Close the connection.
 *
 * @param transport   The transport object
 * @param rc          A return code as the cause of the close
 * @param clean       Set nonzero if this is a clean close of the connection
 * @param reason      A string representing the reason for the close.
 * @return A return code.  The connection is closed regardless of the return code.
 */
typedef int (* ism_transport_close_t)(ism_transport_t * transport, int rc, int clean, const char * reason);


/**
 * Notify the transport that the protocol is done closing a connection.
 * Resources should not be freed until this call is made.
 * @param transport   The transport object
 * @return A return code.  The connection is closed regardless of the return code.
 */
typedef int (* ism_transport_closed_t)(ism_transport_t * transport);

/**
 * Call to handle a received transport frame.
 *
 * A transport does not need to implement framing, but if it does this provides a
 * common framing mechanism.
 * <p>
 * When a buffer is received from the network, the buffer is sent to the framer to
 * determine where the frame boundaries are.  When a complete frame is found, the
 * framer calls receive on the frame and returns 0.  If there is not enough data in the
 * buffer for a complete frame, then the framer returns the number of bytes it needs.
 * When the framer is done with bytes in the buffer, it updates the used location
 * to indicate that bytes before this point are no longer needed.  If the invoker gets
 * a positive return value, it should extend the buffer if required and receive more
 * data to the required size.
 * <p>
 * In most cases, the framer will allow the buffer to grow until all of the data for
 * one message is present.  It is possible for the framer to keep the data elsewhere
 * and free up the buffer at any point.
 * <p>
 * Only one framer call per transport object can be made at the same time.
 *
 * @param transport  The transport object
 * @param buf        The buffer containing received bytes
 * @param pos        The position of the data in the buffer
 * @param avail      The number of valid bytes in the buffer
 * @param used       Output - the position in the buffer which is no longer needed
 */
typedef int (*ism_transport_frame_t)(ism_transport_t * transport, char * buf, int pos, int avail, int * used);

/**
 * Call to add a frame before the current buffer position.
 *
 * This method assumes that the frame is a small number of bytes which can be added to the
 * buffer before the current message bytes.  This is normally some combination of length and
 * command.
 * <p>
 * If the framer must modify the data or put something at the end of the data,
 * it should replace the transport send function, and when done call that function with the
 * SFLAG_NOFRAME flag set.
 *
 * @param transport The transport object
 * @param buf       The buffer containing received bytes
 * @param len       The length of the message (not including the frame)
 * @param protval   The protocol specific value
 */
typedef int (*ism_transport_addframe_t)(ism_transport_t * transport, char * buf, int len, int protval);

/**
 * Call the protocol with a received frame.
 *
 * When using a framer, this call is made from the framer.  When not using a framer this is called
 * directly from the receive logic of the transport.
 *
 * The buffer supplied is valid until the function returns.
 *
 * @param transport  The transport object
 * @param buf        The data received from the client
 * @param len        The length of the data
 * @param protval    The protocol specific value
 * @return A return code.  If a non-zero return code is returned, the connection should be closed.
 */
typedef int (* ism_transport_receive_t)(ism_transport_t * transport, char * buf, int len, int protval);

/**
 * Dump the protocol data for this connection.
 *
 *
 * @param transport  The transport object
 * @param buf        The output buffer
 * @param len        The length of the output buffer
 * @return A return code.
 */
typedef int (* ism_dumpPobj_t)(ism_transport_t * transport, char * buf, int len);


/**
 * Notify the protocol that the transport is closing.
 *
 * @param transport   The transport object
 * @param rc          A return code to pass
 * @param clean       Set nonzero if this is a clean close of the connection
 * @param reason      A string representing the reason for the close.
 * @return A return code.  The connection is closed regardless of the return code.
 */
typedef int (* ism_transport_closing_t)(ism_transport_t * transport, int rc, int clean, const char * reason);


/**
 * Notify the protocol of a delivery failure
 * @param transport   The transport object
 * @param rc          The return code
 * @param context     The protocol consumer context
 */
typedef void (* ism_delivery_failure_t)(ism_transport_t * transport, int rc, void * context);

/**
 * Get the protocol specific action name
 * @param action     The action code
 * @return The name of the action
 */
typedef const char * (* ism_action_name_t)(int action);

/*
 * Notify on outgoing connection completion.
 *
 * @param transport  The transport object
 * @param rc         The TCP error code
 * @return A return code: 0=good
 */
typedef int  (* ism_transport_connected_t)(ism_transport_t * transport, int rc);

typedef int (* ism_transport_livenessCheck_t)(ism_transport_t * transport);

/**
 * State of the transport object
 */
typedef enum ism_transport_state_e {
    ISM_TRANST_Closed  = 0,       /* An unused transport object                */
    ISM_TRANST_Open    = 1,       /* Messages are processed only in this state */
    ISM_TRANST_Opening = 2,       /* In the process of starting up             */
    ISM_TRANST_Closing = 3        /* In the process of closing                 */
}ism_transport_state_e;

/**
 * Used to sub-allocate strings and structures within the transport object.
 */
struct suballoc_t {
    struct suballoc_t * next;
    uint32_t   size;
    uint32_t   pos;
};

/**
 * Callback for a delivery request.
 *
 * The delivery worker thread loops thru all available work and calls this callback associated with each
 * available work item.  When the work item completes, it sets the return code to remove this work item
 * (it is done) or to reschedule the work item (there is more work to do).
 *
 * @param transport  The transport object
 * @param userdata   The user object
 * @param flags      The option flags
 * @return A return code: 0=remove, 1=reschedule
 */
typedef int (* ism_transport_onDelivery_t)(ism_transport_t * transport, void * userdata, int flags);


/*
 * Add a work item to the delivery object associated with a transport.
 *
 * @param transport  The transport object
 * @param ondelivery The method to invoke for work
 * @param userdata   The data context for the work item
 * @return A return code: 0=good
 */
typedef int  (* ism_transport_addwork_t)(ism_transport_t * transport, ism_transport_onDelivery_t ondelivery, void * userdata);

/*
 * Resume message delivery for the transport.
 *
 * @param transport  The transport object
 * @param userdata   The data context for the work item
 * @return A return code: 0=good
 */
typedef int  (* ism_transport_resume_t)(ism_transport_t * transport, void * userdata);

/*
 * Expire the connection.
 *
 * Invoke the owner of the expire callback which is assumed to be security.
 * If there is no expire callback, close the connection.
 *
 * @param transport  The transport object
 * @return A return code: 0=good
 */
typedef int  (* ism_transport_expire_t)(ism_transport_t * transport);

/*
 * Validate the connection.
 *
 * Check if the connection is valid. By default, check if clientID is set.
 *
 * @param transport  The transport object
 * @return A return code: 0=good
 */
typedef int  (* ism_transport_validate_t)(ism_transport_t * transport);

/*
 * Bits for protocol mask
 */
#define PMASK_JMS          0x0000000000000001    /* Allow JMS */
#define PMASK_MQTT         0x0000000000000002    /* Allow MQTT */
#define PMASK_RMSG         0x0000000000000004    /* Allow RMSG */
#define PMASK_AnyProtocol  0x3fffffffffff00ff    /* Allow any external protocol */

#define PMASK_Admin        0x0000000000000100    /* Allow admin */
#define PMASK_Monitoring   0x0000000000000200    /* Allow monitoring */
#define PMASK_Echo         0x0000000000000400    /* Allow echo */
#define PMASK_MQConn       0x0000000000000800    /* Allow MQ connectivity */
#define PMASK_Cluster      0x0000000000001000    /* Allow Cluster data */
#define PMASK_Internal     0x000000000000ff00
#define PMASK_AnyInternal  0x3fffffffffffffff    /* Allow all internal */

/*
 * Bits for transport mask
 */
#define TMASK_TCP          0x0001    /* Allow TCP (or similar) transport */
#define TMASK_WebSockets   0x0002    /* Allow WebSockets transport */
#define TMASK_UDP          0x0004    /* Allow UDP (or similar) transport */
#define TMASK_AnyTrans     0x00ff    /* ALlow any transport */
#define TMASK_Secure       0x0100    /* Allow secure */
#define TMASK_NotSecure    0x0200    /* Allow non-secure */
#define TMASK_AnySecure    0x0300    /* Allow any security */

enum ism_SSL_Methods {
    SecMethod_SSLv3     =  1,
    SecMethod_TLSv1     =  2,
    SecMethod_TLSv1_1   =  3,
    SecMethod_TLSv1_2   =  4,
    SecMethod_TLSv1_3   =  5
};

enum ism_Cipher_Methods {
    CipherType_Best      = 1,      /* Best security */
    CipherType_Fast      = 2,      /* Optimize for speed with high security */
    CipherType_Medium    = 3,      /* Deprecated, now the same as Fast */
    CipherType_Custom    = 4
};


typedef struct msg_stat_t {
    uint64_t  read_msg;
    uint64_t  read_bytes;
    uint64_t  write_msg;
    uint64_t  write_bytes;
    uint64_t  lost_msg;
    uint64_t  warn_msg;
} msg_stat_t;

#define MAX_STAT_THREADS 128

#define ENDPOINT_NEED_SOCKET  1
#define ENDPOINT_NEED_SECURE  2
#define ENDPOINT_NEED_ALL     3

/*
 * Statistics for an endpoint.
 * This does not change when the endpoint is modified.
 */
typedef struct ism_endstat_t {
    uint64_t     connect_active;    /* Currently active connections           */
    uint64_t     connect_count;     /* Connection count since reset           */
    uint64_t     bad_connect_count; /* Count of connections which have failed to connect since reset */
    uint64_t     resv;
    msg_stat_t   count[MAX_STAT_THREADS]; /* Per io thread message statistics */
} ism_endstat_t;

/*
 * The security profile is a named object used to create the security context
 */
typedef struct ism_secprof_t {
    struct ism_secprof_t * next;    /* Chain together security profiles  (must be first)  */
    const char * name;              /* The name of this security profile     */
    uint8_t      method;            /* The openssl method                    */
    uint8_t      clientcipher;      /* Use the client cipher order           */
    uint8_t      clientcert;        /* Use client certificates               */
    uint8_t      ciphertype;        /* The type of cipher to use             */
    uint32_t     sslop;             /* The ssl options                       */
    const char * ciphers;           /* The actual cipher string              */
    const char * certprof;          /* The certificate file                  */
    const char * ltpaprof;          /* LTPA profile name                     */
    const char * oauthprof;         /* OAuth profile name                    */
    const char * crlprof;			/* CRL profile name						 */
    ismHashMap * allowedClientsMap;
    uint8_t      passwordauth;      /* Use password authentication           */
    uint8_t      tlsenabled;        /* Use TLS encryption                    */
    uint8_t      allownullpassword; /* Allow NULL password */
    uint8_t      resv[5];
} ism_secprof_t;

/*
 * Define the endpoint object
 */
typedef struct ism_endpoint_t {
    struct ism_endpoint_t * next;   /* Link to next endpoint (must be first)  */
    const char * name;              /* Listener name                          */
    const char * ipaddr;            /* IP address as a string                 */
    const char * secprof;           /* Security profile name                  */
    const char * msghub;            /* Message hub name                       */
    const char * conpolicies;       /* Connection policies                    */
    const char * topicpolicies;     /* Topic policies                         */
    const char * qpolicies;         /* Queue policies                         */
    const char * subpolicies;       /* Subscription policies                  */

    char         transport_type[8]; /* Transport type string                  */
    int          port;              /* Port number                            */
    int          rc;                /* Startup rc                             */
    uint8_t      enabled;
    volatile uint8_t      isStopped;
    uint8_t      secure;            /* Security type                          */
    uint8_t      useClientCert;     /* Use/check client certificates          */
    uint8_t      usePasswordAuth;   /* Use/check password authentication      */
    uint8_t      needed;            /* Action needed                          */
    uint8_t      oldendp;           /* There was an old endpoint              */
    uint8_t      doNotBatch;
    uint64_t     protomask;         /* Protocol mask                          */
    uint32_t     transmask;         /* Transport mask                         */
    uint32_t     maxSendSize;
    uint8_t      isAdmin;
    uint8_t      isInternal;
    uint8_t      enableAbout;
    uint8_t      resv[5];
    ism_secprof_t * secProfile;
    uint32_t     rssv2[4];
    uint32_t     sslctx_count;
    struct ssl_ctx_st *   sslCTX1;  /* one SSL context                        */
    struct ssl_ctx_st * * sslCTX;
    SOCKET       sock;              /* The accept socket                      */
    int          thread_count;      /* The number of threads for stats        */
    int          maxMsgSize;        /* Maximum message size                   */
    uint32_t     maxSendBufferSize;
    uint32_t     maxRecvBufferSize;

    /* External links */
    void *       transport_ext;
    void *       security_policy;

    /* Statistics */
    ism_time_t   config_time;       /* Time of configuration                  */
    ism_time_t   reset_time;        /* Time of statistics reset               */
    ism_endstat_t * stats;
} ism_endpoint_t;

typedef struct ism_endpoint_t ism_listener_t;

/*
 * The certificate profile is a named object
 */
typedef struct ism_certprof_t {
    struct ism_certprof_t * next;    /* Chain together certificate profiles (must be first) */
    const char *  name;              /* Name of this certificate profile                    */
    const char *  cert;              /* Certificate name (often a file name)                */
    const char *  key;               /* Key name (often a file name)                        */
} ism_certprof_t;

/*
 * We only keep the name of a message hub
 */
typedef struct ism_msghub_t{
    struct ism_msghub_t * next;
    const char * name;
    const char * descr;
} ism_msghub_t;

/**
 * Define a transport connection.
 *
 * This object is the communications mechanism between the transport and protocol components.
 * It allows any transport to work with any protocol without any compile time binding.
 *
 * Any character stings in this object are either constants or allocated using the
 * ism_transport_putString or ism_transport_allocBytes() methods.
 *
 * When a transport makes the transport object, it can vary the size of the final buffer.
 * This is used for the initial allocation for the tobj and allocated strings.  This should
 * be long enough so that allocating more space is rare, but not so large it wastes space
 * if there are a lot of connections.
 */
struct ism_transport_t {
    /* Information set by the transport layer */
    const char *      protocol;          /**< The name of the protocol                       */
    const char *      protocol_family;   /**< The family of the protocol as: mqtt, jms, admin, mq  */
    const char *      client_addr;       /**< Client IP address as a string                  */
    const char *      client_host;       /**< Client hostname  (commonly not set)            */
    const char *      server_addr;       /**< Server IP address as a string                  */
    uint16_t          clientport;        /**< Port on the client                             */
    uint16_t          serverport;        /**< Port of the server                             */
    uint8_t           crtChckStatus;     /**< Status of client certificate check             */
    uint8_t           adminCloseConn;    /**< Flag if connection is closed administratively  */
    uint8_t           usePSK;
    uint8_t           originated;
    ism_domain_t *    domain;            /**< Domain object                                  */
    ism_trclevel_t *  trclevel;          /**< Tracelevel object                              */
    const char *      endpoint_name;
    struct ssl_st *   ssl;               /**< openssl SSL object if secure                   */
    const char *      userid;            /**< Primary userid                                 */
    const char *      cert_name;         /**< Name from key/certificate                      */
    const char *      clientID;          /**< The client ID                                  */
    const char *      name;              /**< Connection name.  This is used for trace.  The clientID is commonly used    */
    uint32_t          index;             /**< The index of this transport entry              */
    int               monitor_id;        /**< The ID of this transport for monitoring        */
    uint32_t          tlsReadBytes;      /**< The size of the TLS headers read               */
    uint32_t          tlsWriteBytes;     /**< The size of the TLS headers written            */
    /* Up to this point all fields must be the same as in ima_transport_info_t               */
    /* State information */
    volatile ism_transport_state_e state;/**< The state of the connection                    */
    int               protocol_id;       /**< The protocol ID                                */
    uint8_t           tid;               /**< Thread index                                   */
    uint8_t           secure;            /**< Connection is secure                           */
    uint16_t          virtualSid;        /**< Virtual stream IDtype 0=for "real connection"  */
    char              closestate[4];     /**< States used for progress in closing            */
    void *            handleCheck;       /**< Used the guarantee the transport object is not reused */
    pthread_spinlock_t lock;
    volatile int      workCount;
    volatile uint64_t lastAccessTime;
    ism_time_t        expireTime;        /**< Expiration time                                */

    /* Information about the connection */
    const char *      channel;           /**< Name of the channel or http object             */
    ism_time_t        connect_time;      /**< Connection time                                */
    ism_endpoint_t *  listener;          /**< The endpoint object                            */
    struct ism_http_t * http;            /**< The http object                                */
    const char *      origin;            /**< The origin if any                              */

    /* Callbacks provided by the transport */
    ism_transport_send_t     send;       /**< Method to call to send a message               */
    ism_transport_frame_t    frame;      /**< Method to call to frame a message              */
    ism_transport_addframe_t addframe;   /**< Method to call to a a frame to a message       */
    ism_transport_close_t    close;      /**< Method to call to force a connection to close  */
    ism_transport_closed_t   closed;     /**< Method to call when all objects in a connection are closed */
    ism_transport_addwork_t  addwork;    /**< Method to call to add work to a delivery queue */
    ism_transport_resume_t   resume;     /**< Method to call to resume message delivery      */

    /* Subobjects */
    struct ism_transobj *    tobj;       /**< The transport local object                     */
    struct ism_frameobj *    fobj;       /**< The frame local object                         */
    uint8_t                  rcvState;
    uint8_t                  durable;    /**< The sesson is durable                          */
    uint8_t                  resvf[6];

    /* Authorization and authentication */
    uint8_t           enabled_checked;   /**< Checked clientId against allowed regex         */ 
    uint8_t           authenticated;     /**< Indication of what is authenticated            */
    uint8_t           ready;             /**< Indication that we have received first packet  */
    uint8_t           delay_close;       /**< Closing delay in 10ms                          */
    uint8_t           www_auth;          /**< Use www auth                                   */
    uint8_t           at_server;         /**< 0=client should send, 1=server should send     */
    uint8_t           nostats;           /**< Do not collect stats                           */
    uint8_t           nolog;             /**< Do not log the connection                      */
    uint8_t           resvaf;
    struct ismSecurity_t * security_context;   /*<< The security context                     */
    ism_transport_expire_t  expire;      /**> Method to call to expire the connection        */

    /* Statistics */
    int               sendQueueSize;
    int               suspended;
    ism_time_t        reset_time;        /**< The time of the last statistics reset          */
    uint64_t          read_bytes;        /**< The bytes read since reset                     */
    uint64_t          read_msg;          /**< The messages read since reset                  */
    uint64_t          write_bytes;       /**< The bytes written since reset                  */
    uint64_t          write_msg;         /**< The messages written since reset               */
    uint64_t          lost_msg;
    uint64_t          warn_msg;


    /* Information set by the protocol layer */
    ism_prop_t *      props;             /**< Connection properties.  Set by the protocol    */
    ism_transport_receive_t  receive;    /**< Method to call when a message is received      */
    ism_transport_closing_t  closing;    /**< Method to call when a connection is closing    */
    ism_transport_addframe_t addframep;  /**< Protocol added method to add a frame, this is moved to addframe after handshake */
    ism_delivery_failure_t   deliveryfailure;  /**< Protocol specific callback for delivery failure */
    ism_dumpPobj_t    dumpPobj;          /**< Method to dump protocol information            */
    ism_action_name_t actionname;        /**< Get the action name                            */
    ism_transport_connected_t  connected;/**< Acknowledge the connect                        */
    ism_transport_livenessCheck_t checkLiveness; /* Callback to check if client is alive */
    struct ism_protobj_t * pobj;         /**< The protocol local object */


    /**
     * Suballocation structure  This must be the last item in the structure .
     * When the transport object is allocated, the extra space after it is used to allocate
     * strings and objects which are freed when the transport object is freed.
     */
    struct suballoc_t suballoc;
};

/**
 * Create a endpoint object
 */
XAPI ism_endpoint_t * ism_transport_createEndpoint(const char * name, const char * msghub, const char * transport_type,
        const char * ipaddr, const char * secprofile, const char * conpolicy, const char * topicpolicy,
        const char * qpolicy, const char * subpolicy, int mkstats);
#define ism_transport_createListener ism_transport_createEndpoint
#define secpolicies conpolicies

/*
 * Notify transport that the connection is ready for processing.
 * @param transport  The transport object
 */
XAPI void ism_transport_connectionReady(ism_transport_t * transport);


/**
 * Make a transport object.
 *
 * Allocate a transport object and set it to OPENING state.
 * @param endpoint  The endpoint object
 * @param extrasize Initial space allocated for strings and objects
 * @param tobjsize  Space allocated for transport specific object
 * @return The new transport objects
 */
MALLOC XAPI ism_transport_t * ism_transport_newTransport(ism_endpoint_t * endpoint, int tobjSize, int fromPool);

/**
 * Make a transport object for an outgoing connection.
 * Allocate a transport object.
 * @param endpoint  The endpoint object.  If NULL a default outgoing endpoint will be used.
 * @param fromPool  Allocate from pool
 */
ism_transport_t * ism_transport_newOutgoing(ism_endpoint_t * endpoint, int fromPool);

/**
 * Free a transport object.
 *
 * Free all memory associated with the transport object.  The invoker must guarantee that there
 * are no references to the transport object before calling this function.
 * @param transport   The transport object
 */
XAPI void ism_transport_freeTransport(ism_transport_t * transport);



/**
 * Create a delivery object.
 *
 * This is created as a common service by transport.  A transport is not required to use this mechanism
 * and therefore it should never be directly called by the protocol.
 *
 * The delivery object represents a work queue which serially delivers work to a callback.  If there is
 * no work available it waits for work to be added.
 *
 * This is normally only called early in the start processing.  It should be considered a fatal error
 * if it fails.
 *
 * @param name  A name of 1 to 16 characters which is suitable for use as a thread name.
 * @return A delivery object or NULL to indicate an error.
 */
XAPI ism_delivery_t * ism_transport_createDelivery(const char * name);


/*
 * Add a work item to the delivery object.
 *
 * @param delivery   The delivery object
 * @param ondelivery The method to invoke for work
 * @param userdata   The data context for the work item
 * @return A return code: 0=good
 */
XAPI int  ism_transport_addDelivery(ism_delivery_t * delivery, ism_transport_t * transport,
        ism_transport_onDelivery_t ondelivery, void * userdata);


/**
 * Callback to check for a protocol.
 *
 * When the transport or framer needs to determine the protocol, it calls each registered
 * protocol until it finds one which accepts the transport object.
 *
 * @param transport  A transport object
 */
typedef int (* ism_transport_onConnection_t)(ism_transport_t * transport);

/**
 * Callback to start messaging a protocol.
 *
 * This callback is called once at start messaging. This can be used by protocols which use the engine
 * as the engine is known to be up at this point.
 *
 * @param transport  A transport object
 */
typedef int (* ism_transport_onStartMessaging_t)(void);

/**
 * Register a protocol handler.
 *
 * This adds the callback to the set of functions to call when When a new connection is established.
 * The transport calls all registered protocols until it finds one which accepts the connection.
 * @param callback  The method to invoke when a new connection is found
 * @return A return code, 0=good
 */
XAPI int ism_transport_registerProtocol(ism_transport_onStartMessaging_t onStart, ism_transport_onConnection_t onConnection);

/**
 * Call registered protocols until we get one which accepts the connection.
 * @param transport
 * @return A return code: 0=connection accepted, !0=continue searching
 */
XAPI int ism_transport_findProtocol(ism_transport_t * transport);


/**
 * Register a frame checker.
 * For external framer is designed to be use by the plug-in component.  This is called
 * to allow additional framers to be added externally.  These framers are called only after all
 * internal framers have bee tried.
 * <p>
 * The frame checker does not consume the packet.  If the frame checker cannot decide based on the
 * number of bytes it has received, it can ask for more by sending a positive return code for the
 * number of bytes it needs.  A negative value indicates that no framer is known, and a zero return
 * code indicates that the framer has accepted this connection.
 *
 * If a zero return code is returned, the frame checker must set the frame and addframe methods in the
 * transport object.
 * <p>
 * This is called with the transport->lock held.  This must be released if any long actions are
 * done in the frame checker.
 * <p>
 * When a message is received, the frame is determined using the frame() method in the transport object.
 * The frame can return a needed value until an entire frame is in the receive buffer.
 *
 * When a message is sent, a frame is added before the content.  If the framer must modify the data
 * or put something at the end of the data, it should replace the transport send function, and when done
 * call that function with the SFLAG_NOFRAME flag set.
 *
 * @param transport  A transport object
 * @param buffer     The buffer containing the handshake
 * @param buflen     The length of the buffer
 * @param used       The output value for the number of bytes used
 * @return 0=accepted, negative if the connection is not accepted, and positive if more bytes are needed.
 */
typedef int (* ism_transport_registerf_t)(ism_transport_t * transport, char * buffer, int buflen, int * used);


/**
 * Register a frame checker.
 * When the initial packet of a connection is received, this method will be called to determine
 * if this is the framer for this connection.
 *
 * @param callback  The method to invoke when a new connection is found
 * @return A return code, 0=good
 */
XAPI int ism_transport_registerFramer(ism_transport_registerf_t callback);


/**
 * Call registered framers until we get one which accepts the connection.
 *
 * @param transport  The transport object
 * @param buffer     The buffer containing the handshake
 * @param buflen     The length of the buffer
 * @param used       The return value for the number of bytes used
 * @return 0=accepted, negative if the connection is not accepted, and positive if more bytes are needed.
 */
XAPI int ism_transport_findFramer(ism_transport_t * transport, char * buffer, int buflen, int * used);

/**
 * Put a string into the transport object.
 *
 * This allocates space inside the transport object which is freed when the tranaport object
 * is freed.  The various strings in the transport object can either be allocated in this way,
 * or can be constants.
 *
 * @param transport  The transport object
 * @param str        The string to copy
 * @return The copy of the string within the transport object.
 */
XAPI const char * ism_transport_putString(ism_transport_t * transport, const char * str);

/**
 * Allocate bytes in the transport object.
 * @param transport  The transport object
 * @param len        The length to allocate
 * @param align      Eight byte align if non-zero
 * @return The address of the allocated memory
 */
XAPI char * ism_transport_allocBytes(ism_transport_t * transport, int len, int align);
/**
 * Create a security profile.
 *
 * This method does not link in the secuirty profile, it only creates the object.
 * Once created, it should not be modified.
 *
 * @param name     The name of the profile
 * @param methods  The methods string
 * @param cipthertype  The enumeration of cipher type
 * @param ciphers  The string cipher list
 * @param certfile The certificate file
 * @param ltpaprof The LTPA Profile name
 *
 */
XAPI ism_secprof_t * ism_transport_createSecProfile(const char * name, uint32_t methods,
        uint32_t ciphertype, const char * ciphers, const char * certprof, const char * ltpaprof, const char * oauthprof);

/**
 * Create a certificate profile.
 *
 * This method does not link in the secuirty profile, it only creates the object.
 * Once created, it should not be modified.
 *
 * @param name     The name of the profile
 * @param cert     The certificate file
 * @param key      The key file
 */
XAPI ism_certprof_t * ism_transport_createCertProfile(const char * name, const char * cert, const char * key);

/**
 * Set connection expiration.
 *
 * At some some after the expiration time, the expire() method on transport is called if it is non-null.
 * If it is null the connection is closed.
 * If the expiration time is zero, the connection does not expire.
 *
 * @param transport  The connection
 * @param expire     The expiration time
 * @return The previous expiration time
 */
XAPI ism_time_t ism_transport_setConnectionExpire(ism_transport_t * transport, ism_time_t expire);

/**
 * Add a transport to the list of monitored transport objects.
 * @param transport  A transport object
 * @return The transport id or -1 to indicate an error
 */
XAPI int ism_transport_addMonitor(ism_transport_t * transport);

/**
 * Add a transport to the list of monitored transport objects without delay.
 * @param transport  A transport object
 * @return The transport id or -1 to indicate an error
 */
XAPI int ism_transport_addMonitorNow(ism_transport_t * transport);

/**
 * Remove a transport from the list of monitored transport objects.
 * @param transport A transport object
 * @param freeit    If non-zero, mark the transport as ready to free.
 *                  This is used when the transport is being removed from monitoring
 *                  because it is being closed.
 */
XAPI int ism_transport_removeMonitor(ism_transport_t * transport, int freeit);

/*
 * Remove a transport from the list of monitored transport objects without delay
 *
 * The normal form of this method will schedule work if it cannot get the monitor lock.
 * This form is used when delaying the removal is not acceptable.
 *
 * @param transport   A pointer to a transport object to stop monitoring
 */
XAPI void ism_transport_removeMonitorNow(ism_transport_t * transport);

/**
 * Get a transport pointer from a monitor id.
 * @param id      A monitor ID
 * @return The transpot object or NULL to indicate that the ID is not valid.
 */
XAPI ism_transport_t * ism_transport_getTransport(int id);

/*
 * Do not log based on client address or proxy
 * @param transport A transport object
 * @return 0=log, 1=do not log
 */
XAPI int ism_transport_noLog(ism_transport_t * transport);


/**
 * The endpoint monitoring object
 */
typedef struct ism_endpoint_mon_t {
    const char * name;              /**< Endpoint name                */
    const char * ipaddr;            /**< IP address or null for any   */
    uint8_t      enabled;           /**< Is enabled                   */
    uint8_t      secure;
    uint16_t     port;              /**< Port                         */
    uint32_t     errcode;           /**< Last error code              */
    ism_time_t   config_time;       /**< Time of configuration        */
    ism_time_t   reset_time;        /**< Time of statistics reset     */
    uint64_t     connect_active;    /**< Currently active connections */
    uint64_t     connect_count;     /**< Connection count since reset */
    uint64_t     bad_connect_count; /**< Count of connections which have failed to connect since reset */
    uint64_t     lost_msg_count;    /**< Count of messages lost for size or queue depth */
    uint64_t     warn_msg_count;    /**< Count of messages lost for size or queue depth for some but not all destinations */
    uint64_t     read_msg_count;    /**< Message count since reset    */
    uint64_t     read_bytes_count;  /**< Bytes count since reset      */
    uint64_t     write_msg_count;   /**< Message count since reset    */
    uint64_t     write_bytes_count; /**< Bytes count since reset      */
} ism_endpoint_mon_t;

typedef struct ism_endpoint_mon_t ism_listener_mon_t;

/**
 * Get a list of endpoint monitoring objects.
 *
 * Return a monitoring object for each of the endpoints which match the selection.
 * If a pattern is specified, only those which match this name will be returned.
 * The pattern is a string with an asterisk to match zero or more characters.
 *
 * @param monlis   The output with a list of endpoint monitoring objects.  This must be freed using
 *                 ism_transport_freeEndpointMonitor.
 * @param names    A pattern to match against the endpoint names or NULL to match all.
 * @return         The count of endpoint monitoring objects returned
 */
XAPI int ism_transport_getEndpointMonitor(ism_endpoint_mon_t * * monlis, const char * names);
#define ism_transport_getListenerMonitor ism_transport_getEndpointMonitor


/**
 * Free a list of endpoint monitoring objects
 */
XAPI void ism_transport_freeEndpointMonitor(ism_endpoint_mon_t * monlis);
#define ism_transport_freeListenerMonitor ism_transport_freeEndpointMonitor

typedef struct ismSocketInfoTcp {
    struct tcp_info tcpInfo;
    uint32_t        siocinq;
    uint32_t        siocoutq;
    uint8_t         validInfo;
} ismSocketInfoTcp;

/**
 * The connection monitoring object
 */
typedef struct ism_connect_mon_t {
    const char *    name;              /**< Connection name.  This is used for trace.  The clientID is commonly used    */
    const char *    protocol;          /**< The name of the protocol                       */
    const char *    client_addr;       /**< Client IP address as a string                  */
    const char *    userid;            /**< Primary userid                                 */
    const char *    listener;          /**< The endpoint object                            */
    uint32_t        port;              /**< The server port                                */
    uint32_t        index;             /**< The index used in trace                        */
    void *          resv1;

    /* Statistics */
    ism_time_t      connect_time;      /**< Connection time                                */
    ism_time_t      reset_time;        /**< The time of the last statistics reset          */
    uint64_t        duration;          /**< The duration of the connection in nanoseconds  */
    uint64_t        read_bytes;        /**< The bytes read since reset                     */
    uint64_t        read_msg;          /**< The messages read since reset                  */
    uint64_t        write_bytes;       /**< The bytes written since reset                  */
    uint64_t        write_msg;         /**< The messages written since reset               */
    uint64_t        lost_msg;          /**< Lost message count since reset                 */
    uint64_t        warn_msg;          /**< Paritailly successful publish count since reset                 */
    int             sendQueueSize;     /**< The size of the send queue                     */
    int             isSuspended;       /**< Is this connection currently suspended         */
    /* Socket level info */
    ismSocketInfoTcp    socketInfoTcp;
} ism_connect_mon_t;

#define ISM_INCOMING_CONNECTION     0x1
#define ISM_OUTGOING_CONNECTION     0x2

/**
 * Get a list of connection monitoring objects.
 *
 * Return a monitoring object for each of the connections which match the selections.
 * If a pattern is specified, only those which match this name will be returned.
 * The pattern is a string with an asterisk to match zero or more characters.  If NULL
 * is specified for a selector, all connections match.  If multiple selector strings are
 * specified, the connection must match all of them.
 *
 * Since the number of connections can be very large, you can specify a maximum count
 * to return.  If the count of connections returned is equal to the maxcount, the function can
 * be reinvoked with the updated position to get additional monitoring objects.
 *
 * @param moncon   The output with a list of connection monitoring objects.  This must be freed using
 *                 ism_transport_freeConnectionMonitor.
 * @param maxcount The maximum number of connections to return.
 * @param position This is an input and output parameter which indicates where we are in the set of connection.
 *                 On first call this should be set to zero.  On subsequent calls the updated value should
 *                 be used.
 * @param options  Options.  Set to zero if no options are known.
 * @param names    A pattern to match against the connection names or NULL to match all.  The name of the
 *                 connection is normally the clientID.
 * @param procools A pattern to match against the protocol name, or NULL to match all.
 * @param endpoints A pattern to match against the endpoint name, or NULL to match all.
 * @param userids   A pattern to match against the username, or NULL to match all.
 * @param clientips A pattern to match against the client IP address.  If the client is connected using IPv6, this
 *                 address is surrounded by brackets.  Thus "[*" will match all IPv6 client addresses.
 * @param minPort  Minimal client port to be included
 * @param maxPort  Maximal client port to be included
 * @param connectionDirection Bitmap for requesting incoming amd/or outgoing connections
 * @return         The count of connection monitoring objects returned
 */
XAPI int ism_transport_getConnectionMonitor(ism_connect_mon_t * * moncon, int maxcount, int * posiiton, int options,
        const char * names, const char * protocols, const char * endpoints,
        const char * userids, const char * clientip, uint16_t minPort, uint16_t maxPort, int connectionDirection);


/**
 * Free a list of connection
 */
XAPI void ism_transport_freeConnectionMonitor(ism_connect_mon_t * moncon);


/**
 * Get number of active connections
 */
XAPI int ism_transport_getNumActiveConns(void);


/*
 * Get a list of transport objects.
 * * DEPRECATED *
 * @param translist  An array to return a list of transport object.
 * @param count      The size of the list to return in number of pointers.
 * @param start      Return list starting at.
 * @return The count of available transports, which can be more that the count specified.
 */
XAPI int ism_transport_getMonitor(ism_transport_t * * translist, int count, int start);

/**
 * * DEPRECATED *
 * Free a list of transport objects.
 * @param translist  The transport object list returned from ism_transport_getMonitor()
 * @param count      The count of items as a number of pointers.  This can be the
 *                   original count, or the found count if it is less than the
 *                   original count.
 */
XAPI void ism_transport_freeMonitor(ism_transport_t * * translist, int ocunt);

/*
 * Create an outgoing connection.
 * If there is a need for a callback when the connection is complete, the completed field
 * in the transport object should be filled in before this is called.
 * @param transport   The outgoing transport object
 * @param ctransport  The incoming transport object (can be NULL)
 * @param server      The IP address of the server
 * @param port        The port of the server
 * @param tlsCTX      The TLS context (or NULL for non-secure)
 * @return An ISMRC return code value.  A zero only indicates that connection attempt was started,
 *     not that it is good.
 */
XAPI int  ism_transport_connect(ism_transport_t * transport, ism_transport_t * ctransport,
        const char * server, int port, struct ssl_ctx_st * tlsCTX);

/**
 * Initialize the transport component
 * @return A return code, 0=good
 */
int ism_transport_init(void);


/**
 * Start the transport
 * @return A return code, 0=good
 */
XAPI int ism_transport_start(void);

/*
 * Start messaging.
 *
 * This is called after all recovery is done, and starts up the rest of the messaage
 * endpoints.
 */
int ism_transport_startMessaging(void);


/**
 * Terminate the transport
 * @return A return code, 0=good
 */
XAPI int ism_transport_term(void);

/**
 * Force the disconnect of connections for speciofied endpoints.
 *
 * @param rc        Specify the return code to use for the disconnect.  This is put into the trce.
 *                  If zero is specified, a default return code is used.
 * @param reason    A reason string which can be NULL.  This is used as the connection close reason.
 *                  If this is NULL, "Forced disconnect" is used.
 * @param endpoint  The endpoint name which can contain asterisks as wildcard characters.
 *                  Use the value "*" to disconnect all connections.
 * @return The count of connections closed
 */
XAPI int ism_transport_disconnectEndpoint(int rc, const char * reason, const char * endpoint);

/**
 * Force a disconnection.
 *
 * Any parameter which is null does not participate in the match.
 * <p>
 * Note that this method will return as soon as we have scheduled a close of the
 * connection, which is probably before it is done.
 * <p>
 * If the match needs to include a literal asterisk, the 0xff codepoint can be substituted.
 *
 * @param clientID  The clientID which can contain asterisks as wildcard characters.
 * @param userID    The userID which can contain asterisks as wildcard characters.
 * @param client_addr  The client address and an IP address which can contain asterisks.
 *                  If this is an IPv6 address it will be surrounded by brackets.
 * @param endpoint  The endpoint name which can contain asterisks as wildcard characters.
 * @return  The count disconnected
 *
 */
XAPI int ism_transport_closeConnection(const char * clientID, const char * userID, const char * client_addr, const char * endpoint);

/**
 * Print transport statistics
 */
XAPI void ism_transport_printStats(const char * args);

/*
 * Find the mask for a protocol, creating it if necessary
 */
uint64_t ism_transport_pluginMask(const char * protocol, int make);

/**
 * Check that the config object name is valid.
 *
 * The name must not contain control characters or the equal character.
 * It must begin with a character of at least 0x40, and cannot end with a space.
 * These limits are designed to resolve any parsing problems.
 * The length is not limited here, but the GUI imposes a length restriction.
 *
 * @param name  The name to check
 * @return 1=valid, 0=not valid
 */
XAPI int ism_transport_validName(const char * name);

/*
 * Return if we are in FIPS mode.
 * If FIPS mode is modified, the server must be restarted.
 * @return 1 if in FIPS mode, 0 if not in FIPS mode
 */
XAPI int ism_transport_getFIPSmode(void);

/*
 * Dump fields from a transport object
 * @param level     The trace level
 * @param transport The transport object to trace
 * @param where     A string identifying the trace
 * @param full      Dump all fields
 */
XAPI void ism_transport_dumpTransport(int level, ism_transport_t * transport, const char * where, int full);

/*
 * Dump fields from a endpoint object
 * @param level     The trace level
 * @param endpoint  The endpoint object to trace
 * @param where     A string identifying the trace
 * @param full      Dump all fields
 */
XAPI void ism_transport_dumpEndpoint(int level, ism_endpoint_t * endpoint, const char * where, int full);

/*
 * Dump fields from a cecurity profile object
 * @param level     The trace level
 * @param secprof   The security profile object to trace
 * @param where     A string identifying the trace
 * @param full      Dump all fields
 */
XAPI void ism_transport_dumpSecProfile(int level, ism_secprof_t * secprof, const char * where, int full);

/*
 * Dump fields from a certificate profile object
 * @param level     The trace level
 * @param certprof  The certificate profile object to trace
 * @param where     A string identifying the trace
 * @param full      Dump all fields
 */
XAPI void ism_transport_dumpCertProfile(int level, ism_certprof_t * certprof, const char * where, int full);

/*
 * Print the endpoints
 * @param pattern  A selection pattern with asterisk wildcard by endpointname.  A null requests all endpoints.
 */
XAPI void ism_transport_printEndpoints(const char * pattern);
/*
 * Trace a message hub
 */
XAPI void ism_transport_dumpMsgHub(int level, ism_msghub_t * msghub, const char * where, int full);
/*
 * setendpoint command
 */
XAPI int ism_transport_setEndpoint(char * args);
XAPI int ism_transport_setSecProf(char * args);
XAPI int ism_transport_setCertProf(char * args);
XAPI int ism_transport_setMsgHub(char * args);

/*
 * Print the security profiles
 */
XAPI void ism_transport_printSecProfile(const char * pattern);

/*
 * Print the certificate profiles
 */
XAPI void ism_transport_printCertProfile(const char * pattern);

/*
 * Print the message hub
 */
XAPI void ism_transport_printMsgHub(const char * pattern);

/*
 * Sends ping to the client
 */
XAPI void ism_transport_checkClientLiveness(const char * clientID, uint32_t excludeConnection);

/*
 * Dump the protocol object associated with this transport object.
 */
XAPI void ism_transport_dumpConnectionPObj(int conID);

/*
 * Get the number of IOP threads
 */
XAPI int ism_tcp_getIOPcount(void);

/*
 * Disable the ability to connect from a set of clientIDs.
 *
 * If there are currently any connections from a matching clientID, the connection is closed.
 * Connections from a metching clientID is not allowed until the same string is used for an
 * ism_transport_enableClientSet, or the server restarts.  If it is necessary to disable
 * connection at server start, this method must be called before messaging is started.
 *
 * If a client set is disabled multiple times, it must be enabled the same number of times.
 *
 * @param regex_str  A regular expression to match the clientIDs to be disabled
 * @param rc         The return code to set when a connection is attempted
 * @return A return code. 0=good
 */
XAPI int  ism_transport_disableClientSet(const char * regex_str, int rc);

/*
 * Enable the ability to connect from a set of clientIDs.
 *
 * If connection from a client set has been disabled, then enable it again.  The string used for
 * enable must exactly match the string used for disable.  If a client set is disabled multiple times,
 * it must be enabled the same number of times.  If the string is not found in the list of disabled
 * client states, no action is taken.
 *
 * @param regex_str  A regular expression to match the clietnIds to be enabled
 * @return A return code. 0=good
 */
XAPI int  ism_transport_enableClientSet(const char * regex_str);

/*
 * Check if this client is allowed by the disable rules.
 *
 * Check if a connection from a client has been disabled.  This support is commonly used for tp
 * temporarily stop connections from a set of clientIDs while some processing is being done.
 *
 * @param transport - the transport to check the name field of
 * @return A return code, 0=allowed, otherwise the return code specified when disabled.
 */
XAPI int  ism_transport_clientAllowed(ism_transport_t * transport);


/*
 * Get the SSL object associated with the transport object
 * @param transport  The transport object
 * @return THe SSL object or NULL if there is none
 */
XAPI struct ssl_st * ism_transport_getSSL(ism_transport_t * transport);


/*
 * An http object has 0 or more incoming content objects.
 * A GET or HEAD has zero.  Most PUT or POST actions have 1, but a
 * multipart content will have multiple, one for each part.
 */
typedef struct ism_http_content_t {
    char *       content;         /* Content, can be modifed during processing */
    uint32_t     content_len;   /* Length of the content */
    uint8_t      flags;           /* Allocation flags      */
    uint8_t      resv[3];
    const char * content_type;  /* Type of the content */
    const char * filename;
} ism_http_content_t;


/*
 * When an HTTP request is received, an http object is created and attached to
 * the transport object.  While this object exists the transport object is in
 * response mode and will not accept more data from the client.
 *
 * The path is parsed and the first level is used as an alias to determine the
 * protocol.  That protocol may further parse levels to determine the version
 * and sub-protocol.
 *
 * The path and query from the location are separated and un-escaped into UTF-8
 * strings.
 *
 * The Accept-Language header is converted to an ICU locale name
 *
 * The content buffer can be modified as required.
 *
 * An empty buffer object is created.  The pointer is into the heap so this
 * buffer can be used in any thread.  It cannot be used after the respond.
 *
 * The private fields are not directly usable except in implementing the access
 * methods.
 */
typedef struct ism_http_t {
    ism_transport_t * transport;
    uint8_t      http_op;         /* as HTTP_OP_* */
    uint8_t      state;           /* The state of the HTTP object */
    uint8_t      subprot;         /* Sub-protocol */
    uint8_t      will_close;      /* The connection will close after the respond */
    uint8_t      resv[4];
    int32_t      max_age;         /* Max_age for cache, -1 = no-cache */
    uint32_t     content_count;   /* Count of contents, GET and HEAD always have 0 */
    ism_http_content_t * content; /* Content structure */
    char *       path;            /* Path in UTF-8 */
    char *       query;           /* The parameter section of the location in UTF-8 */
    char *       locale;          /* The locale name in ICU conventions */
    concat_alloc_t  outbuf;       /* The output buffer */

    /* Additional fields set by protocol */
    const char * user_path;       /* Portion of the path not used for routing */
    char         version[16];     /* Version number of the API */
    uint32_t     val1;            /* Protocol specific value */
    uint32_t     val2;            /* Protocol specific value */

    /* Private fields */
    ism_map_t    headers;         /* Input headers and cookies */
    ism_map_t    out_headers;     /* Output headers */
    ism_prop_t * header_props;
    uint32_t     header_count;
    uint32_t     cookie_count;
    ism_http_content_t single_content;
    /* There will be additional private fields */
} ism_http_t;


enum ism_http_op_e {
    HTTP_OP_GET     = 'G',   /* Get an object */
    HTTP_OP_POST    = 'P',   /* Post changes to an object */
    HTTP_OP_PUT     = 'U',   /* Put an object */
    HTTP_OP_DELETE  = 'D',   /* Delete an object */
    HTTP_OP_HEAD    = 'H',   /* Head: same as GET except that content is not sent */
    HTTP_OP_OPTIONS = 'O',   /* Options: not sent to protocol */
    HTTP_OP_WS      = 'W'    /* WebSockets data */
};

/*
 * Create a new HTTP object.
 *
 * @param  http_op   The HTTP operation as HTTP_OP_*
 * @param  path      The path portion of the HTTP location
 * @param  query     The query portion of the HTTP location
 * @param  locale    The locale name in ICU notation or NULL to indicate that locale is unknown
 * @param  data      The content data
 * @param  datalen   The length of the data.
 * @param  datatype  The content type of the data
 * @param  hdrs      The header data as a concise map
 * @param  hdrlen    The length of the header map
 * @param  buflen    The length of the output buffer to originally create
 * @return  An HTTP object or NULL to indicate an error
 */
XAPI ism_http_t * ism_http_newHttp(int http_op, const char * path, const char * query, const char * locale,
        char * data, int datalen, const char * datatype, char * hdrs, int hdrlen, int buflen);

/*
 * Return the current time in HTTP format.
 *
 * @param buffer  The output buffer
 * @param buflen  The length of the output buffer.  This should be at least 32
 * @return The buffer supplied with the date filled in
 */
XAPI const char * ism_http_time(char * buffer, int buflen);

/*
 * Map the locale from the HTTP Accept-Language format to ICU format.
 *
 * @param  locale  The name of the Accept-Language locale string
 * @param  buffer  The output buffer
 * @param  buflen  The output buffer len.  This should be at least 6.
 * @return The ICU locale string in the supplied buffer.
 */
XAPI const char *  ism_http_mapLocale(const char * locale, char * buffer, int buflen);

/*
 * Map from a file name to a content type
 *
 * This mapping is done only based on the file extension.
 * @param name  The file name (only the extension is used)
 * @param max_age The max age in seconds for this type
 * @return  The file content
 */
XAPI const char * ism_http_getContentType(const char * name, int * max_age);

/*
 * Respond to an http request.
 *
 * Send the response on the connection.  After this call the http object no longer belongs
 * to the invoker and can be freed up at any time.
 *
 * The data is http->outbuf is used as the content data.
 * If ism_http_sendPart() has been called, the contents of http->outbuf are ignored.
 *
 * This can be called in another thread than the one which received the request.
 *
 * The actual sending of the response may be asynchronous and the http object exists
 * until the ressponse is complete, but since the invoker cannot know when this will happen
 * it must not use this object after this call is made.
 *
 * @param  http     The http object
 * @param  http_rc  The http status code
 * @param  content_type  The http content type.  If null the content type is determined by inspection.
 * @return An ISMRC return code
 */
XAPI int ism_http_respond(ism_http_t * http, int http_rc, const char * content_type);

/*
 * Respond to an http request with a file.
 *
 * Send the response on the connection using data from a file and free up this http object.
 * After this call the http object no longer belongs to the invoker and can be freed up
 * at any time.
 *
 * The http status field is always 200 if the file is found and 404 otherwise.
 *
 * This can be called in another thread than the one which received the request.
 * until the response is complete, but since the invoker cannot know when this will happen
 * it must not use this object after this call is made.
 *
 * @param  http     The http object
 * @param  filename The name of the local file
 * @return  An ISMRC return code
 */
XAPI int ism_http_respondFile(ism_http_t * http, const char * filename);

/*
 * Get the value of a header field.
 *
 * Those header fields processed by the http parser are not included in this list.
 *
 * @param http   An http object
 * @param name  The name of the header
 * @return The value of the header or NULL to indicate it does not exist
 */
XAPI const char * ism_http_getHeader(ism_http_t * http, const char * name);

/*
 * Set an output header field.
 *
 * @param http  An http object
 * @param name  The name of the header
 * @param value The value of the hdeaer
 */
XAPI void ism_http_setHeader(ism_http_t * http, const char * name, const char * value);

/*
 * Get the value of a cookie.
 *
 * @param http   An http object
 * @param name  The name of the cookie
 * @return The value of the cookie or NULL to indicate it does not exist
 */
XAPI const char * ism_http_getCookie(ism_http_t * http, const char * name);

/*
 * Get the name of a header field by index.
 *
 * This is used to iterate thru the list of headers.  You should start with 0
 * and end with the first NULL response.
 * @param http   An http object
 * @param index  Which header
 * @return The name of the header or NULL to indicate there are not that many
 */
XAPI const char * ism_http_getHeaderByIndex(ism_http_t * http, int index);


/*
 * Get the name of a cookie by index.
 *
 * This is used to iterate thru the list of cookies.  You should start with 0
 * and end with the first NULL response.
 * @param http   An http object
 * @param index  Which cookie
 * @return The name of the header or NULL to indicate there are not that many
 */
XAPI const char * ism_http_getCookieByIndex(ism_http_t * http, int index);

/*
 * Send one content in a multipart.  ism_http_respond() must be called when all
 * parts have been sent.  If this is called, the http status will be set to 200.
 *
 * The data is http->outbuf is used as the content data.  The http->outbuf is
 * then empied for use in the next object.
 *
 * @param  http     The http object
 * @param  content_type  The http content type
 * @return An ISMRC return code
 */
XAPI int  ism_http_sendPart(ism_http_t http, const char * content_type);

/*
 * Return for forwarder monitoring stats
 */
typedef struct fwd_monstat_t {
    ism_time_t  timestamp;           /* Monitoring time */
    uint32_t    channel_count;       /* Channels the forwarder knows about */
    uint32_t    recvrate;            /* Rolled up receive rate */
    uint32_t    sendrate0;           /* Rolled up unreliable send rate */
    uint32_t    sendrate1;           /* Rolled up reliable send rate */
} fwd_monstat_t;

/*
 * Allow out of order linkage
 */
typedef int (* fwd_stat_f)(concat_alloc_t * buf, int option);
typedef int (* fwd_monstat_f)(fwd_monstat_t * monstat, int option);
XAPI void ism_transport_registerFwdStat(fwd_stat_f fwdstat, fwd_monstat_f fwdmonstat);

/*
 * Get forwarder stats in JSON format.
 *
 * This is in transport since monitoring in before protocol in the build order.
 * @param buf     The output buffer
 * @param option  Option bits  0x01=include outermost JSON object.
 * @return The count of entries in the statistics object
 */
int  ism_fwd_getForwarderStats(concat_alloc_t * buf, int option);

/*
 * Get forwarder monitoring stats.
 *
 * The monitoring stats are a rolled up forwarder statistics
 * @param monstat The monitor statistics result
 * @param option  Option bits, use 0
 * @return A return code, 0=good
 */
int  ism_fwd_getMonitoringStats(struct fwd_monstat_t * monstat, int option);

/*
 * Set the header wanted list.
 *
 * The wanted list gives a list of headers and cookies which are used by a protocol.
 * On the first http header in a connection, all headers are given, so if a header or
 * cookie is only needed on the first header in a connection it does not need to be
 * declared.  A negative count means that all headers and cookies which are not handled
 * by the http parser are passed to the protocol.
 * <p>
 * Some common headers are put into canonical case before checking, but any custom
 * headers should be specified in the canonical case.
 *
 * @param transport  The connection object
 * @param count      The number of entries in the wanted_list (negative means send all headers)
 * @param wanted_list An array of wanted headers.
 *
 */
void ism_transport_setHeaderList(ism_transport_t * transport, int count, const char * * wanted_list);

/**
 * Print transport TCP statistics
 */
XAPI void ism_transport_printStatsTCP(char ** params, int paramsCount );

typedef int  (* ism_transport_AsyncJob_t)(ism_transport_t * transport, void * param1, uint64_t param2);
XAPI void ism_transport_submitAsyncJobRequest(ism_transport_t * transport, ism_transport_AsyncJob_t job, void * param1, uint64_t param2);

typedef union {
    uint32_t    iValue;
    struct {
        uint16_t    stream;
        uint8_t     cmd;
        uint8_t     rsrv;
    } hdr;
} ism_muxHdr_t;


/**
 * Revoke connections based on CRL revocation list for specified endpoint
 *
 * @param endpoint  The endpoint name
 *
 */
XAPI int ism_transport_revokeConnectionsEndpoint(const char * endpoint);


#ifdef __cplusplus
}
#endif
#endif
