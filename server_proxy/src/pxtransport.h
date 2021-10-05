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

#ifndef __PXTRANSPORT_DEFINED
#define __PXTRANSPORT_DEFINED

#include <ismutil.h>
#include <tenant.h>
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


#define MAX_FIRST_MSG_LENGTH    0x20000 /* 128K */


typedef concat_alloc_t ism_map_t;

/**
 * @file pxtransport.h
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
 * In the case of the TCP transport, the transport object represents a TCP connection.  There are two
 * built-in framers: JMS and MQTT.  Each of these has a matching protocol.  Other framers can be
 * registered, and we do register the WebSockets framer which allows any protocol specified in the
 * WebSockets handshake.
 */
//typedef struct ism_transport_t ism_transport_t;
typedef struct ism_delivery_t  ism_delivery_t;

typedef struct ima_pxtransport_t ism_transport_t;

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
 * If the framer wants to throttle reads (or more likely the receiver asked to throttle
 * reads), the framer returns -9.  In the case of errors it returns any other negative
 * return code (normally -1).
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
 * @return A return code 0=good, >0=more data needed, -9=throttle, -1=error
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
 * If a return code > 0 is returned, the connection must have been closed.
 *
 * @param transport  The transport object
 * @param buf        The data received from the client
 * @param len        The length of the data
 * @param protval    The protocol specific value
 * @return A return code.  0=good, >0=error, <0=throttle reads
 */
typedef int (* ism_transport_receive_t)(ism_transport_t * transport, char * buf, int len, int protval);



struct ism_server_t;



/**
 * Dump the protocol data for this connection.
 *
 * @param transport  The transport object
 * @param buf        The output buffer
 * @param len        The length of the output buffer
 * @return A return code.
 */
typedef int (* ism_dumpPobj_t)(ism_transport_t * transport, char * buf, int len);

/**
 * Send a fair use request.
 *
 * The fair use provider is assumed to be the transport, and is controlled by the protocol.
 * @param transport  The transport object
 * @parma request    Which fair use request
 * @param val1       The first value which depends on the request
 * @param val2       The second value which depends on the request
 * @param A ISMRC return code.
 */
typedef int (* ism_fairuse_t)(ism_transport_t * transport, int request, int val1, int val2);
#define FUR_SetMaxMUPS   1   /* val1=mups, val2=multiplier */
#define FUR_SetUnitSize  2   /* val1=unit size */
#define FUR_Message      3   /* val1=count  val2=size */

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

/*
 * Notify on outgoing connection completion.
 *
 * @param transport  The transport object
 * @param vaitval    The wait value (commonly the message ID)
 * @param rc         The TCP error code
 * @param reason     The reason string
 * @return A return code: 0=good
 */
typedef int  (* ism_transport_ack_t)(ism_transport_t * transport, int waitval, int rc, const char * reason);

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
 * Resume message delivery for the transport.
 *
 * @param transport  The transport object
 * @param userdata   The data context for the work item
 * @return A return code: 0=good
 */
typedef int  (* ism_transport_resume_t)(ism_transport_t * transport, void * userdata);

/*
 * Bits for protocol mask
 */
#define PMASK_JMS          0x0001    /* Allow JMS */
#define PMASK_MQTT         0x0002    /* Allow MQTT */
#define PMASK_Plugin       0x0004    /* Allow plug-in protocol */
#define PMASK_AnyProtocol  0x00ff    /* Allow any external protocol */

#define PMASK_Admin        0x0100    /* Allow admin */
#define PMASK_Monitoring   0x0200    /* Allow monitoring */
#define PMASK_Echo         0x0400    /* Allow echo */
#define PMASK_MQConn       0x0800    /* Allow MQ connectivity */
#define PMASK_Internal     0x0f00
#define PMASK_AnyInternal  0x0fff    /* Allow all internal */

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
    CipherType_Medium    = 3,      /* Optimize for speed and allow medium security */
    CipherType_Custom    = 4
};

enum ism_Authorization_Methods {
    AuthType_None        = 0,
    AuthType_Username    = 1,
    AuthType_Basic       = 2,
    AuthType_Cert        = 3
};


typedef struct msg_stat_t {
    uint64_t  read_msg;
    uint64_t  qos1_read_msg;
    uint64_t  qos2_read_msg;
    uint64_t  read_bytes;
    uint64_t  write_msg;
    uint64_t  write_bytes;
    uint64_t  lost_msg;
} msg_stat_t;

typedef struct px_msgsize_stats_t {
	//Read Msg Size Stat
	uint64_t   C2P512BMsgReceived;
    uint64_t   C2P1KBMsgReceived;
    uint64_t   C2P4KBMsgReceived;
    uint64_t   C2P16KBMsgReceived;
    uint64_t   C2P64KBMsgReceived;
    uint64_t   C2PLargeMsgReceived;
    //Write Msg Size Stat
    uint64_t   P2S512BMsgReceived;
	uint64_t   P2S1KBMsgReceived;
	uint64_t   P2S4KBMsgReceived;
	uint64_t   P2S16KBMsgReceived;
	uint64_t   P2S64KBMsgReceived;
	uint64_t   P2SLargeMsgReceived;
} px_msgsize_stats_t;

#define MAX_STAT_THREADS 32

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
 * Define the endpoint object
 */
typedef struct ism_endpoint_t {
    struct ism_endpoint_t * next;   /* Link to next endpoint (must be first)  */
    const char * name;              /* Listener name                          */
    const char * ipaddr;            /* IP address as a string                 */
    const char * secprof;           /* Security profile name                  */
    const char * clientclass;       /* The client class name                  */
    const char * resvc[2];
    char         transport_type[8]; /* Transport type string                  */
    int          port;              /* Port number                            */
    int          rc;                /* Startup rc                             */
    uint8_t      enabled;
    volatile uint8_t      isStopped;
    uint8_t      secure;            /* Security type 0=non3, 1=required, 2=optional */
    uint8_t      needed;            /* Action needed                          */
    uint8_t      oldendp;           /* There was an old endpoint              */
    uint8_t      separator;         /* Domain separator                       */
    uint8_t      need_user;         /* Need a userid                          */
    uint8_t      enableAbout;
    uint32_t     protomask;         /* Protocol mask                          */
    uint32_t     transmask;         /* Transport mask                         */
    struct ssl_ctx_st * sslCTX;     /* SSL context                            */
    ismHashMap * sslCTXMap;
    SOCKET       sock;              /* The accept socket                      */
    int          thread_count;      /* The number of threads for stats        */
    int          maxMsgSize;        /* Maximum message size                   */
    uint8_t      authorder [8];     /* Authentication order                   */
    uint8_t      tls_method;        /* The openssl method                     */
    uint8_t      clientcipher;      /* Use the client cipher order            */
    uint8_t      clientcert;        /* Use client certificates                */
    uint8_t      ciphertype;        /* The type of cipher to use              */
    uint32_t     sslop;             /* The ssl options                        */
    const char * ciphers;           /* The actual cipher string               */
    const char * cert;              /* The certificate file                   */
    const char * key;               /* The key file                           */
    const char * keypw;             /* The key password (can be obfuscated)   */
    uint32_t     resvi;
    uint32_t     lb_count;          /* locked by authstat_lock                */
    /* Statistics */
    ism_time_t   config_time;       /* Time of configuration                  */
    ism_time_t   reset_time;        /* Time of statistics reset               */
    ism_endstat_t * stats;
} ism_endpoint_t;

#define CRL_STATUS_NONE             0
#define CRL_STATUS_OK               1
#define CRL_STATUS_EXPIRED          2
#define CRL_STATUS_FUTURE           3
#define CRL_STATUS_INVALID          4
#define CRL_STATUS_UNAVAILABLE      5
#define CRL_STATUS_NOTCRL           9  /* This is not a CRL verify problem */

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
typedef struct ima_pxtransport_t {
    const char *      protocol;          /**< The name of the protocol                       */
    const char *      protocol_family;   /**< The family of the protocol as: mqtt, jms, admin, mq  */
    const char *      client_addr;       /**< Client IP address as a string                  */
    const char *      client_host;       /**< Client hostname  (commonly not set)            */
    const char *      server_addr;       /**< Server IP address as a string                  */
    uint16_t          clientport;        /**< Port on the client                             */
    uint16_t          serverport;        /**< Port of the server                             */
    uint8_t           crtChckStatus;     /**< Status of client certificate check             */
    uint8_t           adminCloseConn;    /**< Flag if connection is closed administratively  */
    uint8_t			  usePSK;
    uint8_t           originated;
    ism_domain_t *    domain;            /**< Domain object                                  */
    ism_trclevel_t *  trclevel;          /**< Tracelevel object                              */
    const char * 	  endpoint_name;
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
    /* Information set by the transport layer */
    /* State information */
    volatile ism_transport_state_e state;/**< The state of the connection                    */
    uint32_t          maxMsgSize;
    char              closestate[4];     /**< States used for progress in closing            */
    uint16_t          virtualSid;        /**< Virtual stream IDtype 0=for "real connection"  */
    uint8_t           tid;               /**< Thread index                                   */
    uint8_t           secure;            /**< Connection is secure                           */
    uint8_t           requireClientCert;
    uint8_t           crlStatus;
    uint8_t           usedSNI;
    uint8_t           has_acl;
    pthread_spinlock_t lock;
    volatile double   lastAccessTime;    /**< Last time connection was active from readTSC() */

    /* Information about the connection */
    const char *      genClientID;       /**< The client ID                                  */
    const char *      channel;           /**< Name of the channel or http object             */
    ism_time_t        connect_time;      /**< Connection time                                */
    ism_endpoint_t *  endpoint;          /**< The endpoint object                            */
    struct ism_tenant_t * tenant;
    struct ism_http_t * http;            /**< The http object                                */
    const char *      origin;            /**< The origin if any                              */
    const char *      reason;            /**< Closing reason                                 */

    /* Callbacks provided by the transport */
    ism_transport_send_t     send;       /**< Method to call to send a message               */
    ism_transport_frame_t    frame;      /**< Method to call to frame a message              */
    ism_transport_addframe_t addframe;   /**< Method to call to a a frame to a message       */
    ism_transport_close_t    close;      /**< Method to call to force a connection to close  */
    ism_transport_closed_t   closed;     /**< Method to call when all objects in a connection are closed */
    ism_fairuse_t            fairuse;    /**< Method to call to update fair use              */

    /* Subobjects */
    struct ism_transobj *    tobj;       /**< The transport local object                     */
    struct ism_frameobj *    fobj;       /**< The frame local object                         */

    /* Authorization and authentication */
    struct ismSecurity_t * security_context;   /*<< The security context                   */
    /* userid is below to force cacheline */
    uint8_t           authenticated;     /**< Indication of what is authenticated            */
    uint8_t           ready;             /**< Indication that we have received first packet */
    uint8_t           at_server;
    uint8_t           www_auth;
    uint32_t          auth_permissions;  /**< Authorization permission bits                  */
    uint32_t          auth_cb_called;   /**< Authentication callback had been called         */

    /* Statistics */
    uint32_t          close_rc;          /**< Closing return code                            */
    int32_t           sendQueueSize;
    ism_time_t        metering_time;
    uint64_t          read_bytes;        /**< The bytes read since reset                     */
    uint64_t          read_msg;          /**< The messages read since reset                  */
    uint64_t          write_bytes;       /**< The bytes written since reset                  */
    uint64_t          write_msg;         /**< The messages written since reset               */
    uint64_t          lost_msg;
    uint64_t          read_bytes_prev;   /**< Read bytes already metered                     */
    uint64_t          write_bytes_prev;  /**< Write bytes already metered                    */

    /* Information set by the protocol layer */
    ism_prop_t *      props;             /**< Connection properties.  Set by the protocol    */
    ism_transport_receive_t  receive;    /**< Method to call when a message is received      */
    ism_transport_closing_t  closing;    /**< Method to call when a connection is closing    */
    ism_gotAddress_f         gotAddress; /**< Method to call when address resolution completes */
    void *                   getAddrCB;  /**< The getaddrinfo control block                  */
    ism_transport_addframe_t addframep;  /**< Protocol added method to add a frame, this is moved to addframe after handshake */
    ism_dumpPobj_t    dumpPobj;          /**< Method to dump protocol information            */
    ism_action_name_t actionname;        /**< Get the action name                            */
    ism_transport_connected_t  connected;/**< Acknowledge the connect                        */
    struct ism_protobj_t * pobj;         /**< The protocol local object */
    struct ismPXACT_Client_t * aobj;     /**< The pxActivity client object */
    struct ism_server_t  * server;
    const char *       deviceID;         /**< The device or app ID */
    const char *       typeID;           /**< The type ID */
    const char *       org;              /**< The org name */
    const char *       sniName;          /**< The SNI name */
    const char *       certIssuerName;
    uint8_t            client_class;     /* The client class such as 'd', 'g', 'a', or 'A' */
    uint8_t            alt_monitor;      /* This client class uses alt monitoring (mostly apps) */
    uint8_t            use_userid;
    uint8_t            connect_order;
    uint8_t            connect_tried;
    uint8_t            counted;          /* stats have been incremented on connect, so decrement them on close */
    uint8_t            rcvState;
    uint8_t            no_monitor;       /* Do not monitor */
    uint8_t            useMups;          /* Use message rate throttling */
    uint8_t            pwIsRequired;     /* requireUser actually means require password */
    uint8_t            durable;
    uint8_t            slotused;         /* The slot which was selected (kept until gotConnection() */
    uint8_t            resvi[2];
    uint16_t           keepalive;        /**< Keepalive for serverless */
    uint64_t           waitID;
    ism_transport_ack_t ack;             /**< The ack callback */
    void *             hout;             /**< HTTP outbound sub-object */
    ism_time_t         lastDiscardMsgsLogTime;   /**< Last DiscardMsgs Log Time */

    /**
     * Suballocation structure  This must be the last item in the structure .
     * When the transport object is allocated, the extra space after it is used to allocate
     * strings and objects which are freed when the transport object is freed.
     */
    struct suballoc_t suballoc;
} ima_pxtransport_t;


/**
 * Create a endpoint object
 */
XAPI ism_endpoint_t * ism_transport_createEndpoint(const char * name, int mkstats);

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
 * This callback is called once at start messaging with a "*start*" protocol.  No protocol
 * should accept this transport object.  This can be used by protocols which use the engine
 * as the engine is known to be up at this point.
 *
 * @param transport  A transport object
 */
typedef int (* ism_transport_register_t)(ism_transport_t * transport);

/**
 * Register a protocol handler.
 *
 * This adds the callback to the set of functions to call when When a new connection is established.
 * The transport calls all registered protocols until it finds one which accepts the connection.
 * @param callback  The method to invoke when a new connection is found
 * @return A return code, 0=good
 */
XAPI int ism_transport_registerProtocol(ism_transport_register_t callback);

/**
 * Call registered protocols until we get one which accepts the connection.
 * @param transport
 * @return A return code: 0=connection accepted, !0=continue searching
 */
XAPI int ism_transport_findProtocol(ism_transport_t * transport);


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
 */

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
 * Remove a transport from the list of monitored transport objects.
 * @param transport A transport object
 * @param freeit    If non-zero, mark the transport as ready to free.
 *                  This is used when the transport is being removed from monitoring
 *                  because it is being closed.
 */
XAPI int ism_transport_removeMonitor(ism_transport_t * transport, int freeit);


/*
 * Create an outgoing connection.
 * If there is a need for a callback when the connection is complete, the completed field
 * in the transport object should be filled in before this is called.
 * @param transport   The outgoing transport object
 * @param ctransport  The incoming transport object (can be NULL)
 * @param tenant      The tenant for the outgoing connection
 * @param tlsCTX      The TLS context (or NULL to not use TLS)
 * @return An ISMRC return code value.  A zero only indicates that connection attempt was started,
 *     not that it is good.
 */
XAPI int  ism_transport_connect(ism_transport_t * transport, ism_transport_t * ctransport,
        ism_tenant_t * tenant, struct ssl_ctx_st * tlsCTX);

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
 * Force a disconnection.
 *
 * Any parameter which is null does not participate in the match.
 * Note that this method will return as soon as we have scheduled a close of the
 * connection, which is probably before it is done.
 *
 * @param clientID  The clientID which can contain asterisks as wildcard characters.
 * @param userID    The userID which can contain asterisks as wildcard characters.
 * @param client_addr  The client address and an IP address which can contain asterisks.
 *                  If this is an IPv6 address it will be surrounded by brackets.
 * @param endpoint  The endpoint name which can contain asterisks as wildcard characters.
 * @param tenant    The tenant name which can contain asterisks as wildcard characters.
 * @param server    The sever name which can contain asterisks as wildcard characters.
 * @param permissions A bitmask of operation bits.  A zero means unconditional disconnect.
 * @return  The count disconnected
 *
 */
int ism_transport_closeConnection(const char * clientID, const char * userID, const char * client_addr,
        const char * endpoint, const char * tenant, const char * server, uint32_t permissions);

/*
 * Force a disconnection to server
 *
 * Thiw will include MUX, Monitor, and HTTP connections.
 *
 * Any parameter which is null does not participate in the match.
 * Note that this method will return as soon as we have scheduled a close of the
 * connection, which is probably before it is done.
 *
 * @param server    The sever name which can contain asterisks as wildcard characters.
 * @return  The count disconnected
 *
 */
int ism_transport_closeServerConnection(const char * server);

/**
 * Print transport statistics
 */
XAPI void ism_transport_printStats(const char * args);

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

XAPI int ism_transport_getEndpointList(const char * match, ism_json_t * jobj, int json, const char * name);

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
 * Print the endpoints
 * @param pattern  A selection pattern with asterisk wildcard by endpointname.  A null requests all endpoints.
 */
XAPI void ism_transport_printEndpoints(const char * pattern);

/*
 * setendpoint command
 */
XAPI int ism_transport_setEndpoint(char * args);

ism_endpoint_t * ism_transport_getEndpoint(const char * name);
int ism_tenant_getEndpointJson(ism_endpoint_t * endpoint, ism_json_t * jobj, const char * name);

/*
 * Dump connection protocol object.
 */
XAPI void ism_transport_dumpConnectionPObj(int conID);

typedef int  (* ism_transport_AsyncJob_t)(ism_transport_t * transport, void * param1, uint64_t param2);
XAPI void ism_transport_submitAsyncJobRequest(ism_transport_t * transport, ism_transport_AsyncJob_t job, void * param1, uint64_t param2);

/*
 * Map an openssl verify return code to a CRL status
 *
 * @param verifyrc  The return code from the openssl verify_cert()
 * @return A CRL status, or CRL_STATUS_NOTCRL if the verify return code is not CRL related
 */
int ism_proxy_mapCrlReturnCode(int verifyrc);


typedef struct ism_http_content_t {
    char *       content;         /* Content, can be modifed during processing */
    uint32_t     content_len;   /* Length of the content */
    uint8_t      flags;           /* Allocation flags      */
    uint8_t      resv[3];
    const char * content_type;  /* Type of the content */
    const char * format;
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
    uint8_t      will_close;      /* The connection will be cloased after the respond */
    uint8_t      norespond;       /* Do not respond (used for testing) */
    uint8_t      resv[3];
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

typedef union {
    uint32_t    iValue;
    struct {
        uint16_t    stream;
        uint8_t     cmd;
        uint8_t     rsrv;
    } hdr;
} ism_muxHdr_t;


/**
 * Reply with a message for HTTP outbound
 *
 * This will either give back a message, or a return code specifying why no message is returned.
 * If the server detects an error it will say to close the client connection.  The connection also
 * needs to be closed if the client requested it be closed.
 *
 * @param http    The HTTP object
 * @param rc      The return code
 * @param reason  The reason string
 * @param close   The server requires the connection to be closed
 * @param subID   The subscription ID
 * @param topic   The topic name (NULL if the RC is non-zero)
 * @param payload The message content
 * @param len     The message length
 */
typedef void (* iot_replyMessage_t)(ism_http_t * http, int rc, const char * reason, int close,
             int subID, const char * topic, const char * payload, int len);

/**
 * Get a message for HTTP outbound.
 *
 * This is called to get a message from the server.  If there is a problem with the request
 * It gives a bad return code as a result.  Otherwise it will send ism_iot_replyMessage() with
 * the result of the get.
 *
 * @param http    The HTTP object.  The transport object must be set
 * @param subID   The subscription ID (can be zero)
 * @param topic   The topic name
 * @param timeout The max time to get the message in milliseconds
 * @param subopt  The subscription options (QoS)
 * @param filter  The subscription filter
 * #param callback The reply message callback
 * @return  An ISMRC which indicates if the request is valid.  If zero, the callback will be
 * invoked later to complete the get.
 */
int iot_getMessage(ism_http_t * http, int subID, const char * topic, int timeout,
        int subopt, const char * filter, iot_replyMessage_t callback);

/**
 * Notify HTTP outbound when the client connection is closing
 *
 * This must be called when any connection which has issued a iot_getMessage is closing.
 * This will close the HTTP outbound connections to the server if one exists.
 *
 * @param transport  The client to proxy connection
 * @param rc         The return code of the close
 * @param reason     The reason string of the close
 */
void iot_doneConnection(ism_transport_t * transport, int rc, const char * reason);

/*
 * Get the TLS method from the transport object.
 * @param transport The transport object
 * @return The security of the transport
 */
XAPI enum ism_SSL_Methods ism_transport_getTLSMethod(ism_transport_t * transport);

/*
 * The the TLS method as a string from the transport object
 * @param transport The transport object
 * @return The string form of the TLS method
 */
const char * ism_transport_getTLSMethodName(ism_transport_t * transport);

/*
 * Init the ackwait methods
 */
extern void ism_transport_ackwaitInit(void);

/*
 * Get the WaitID for a transport object.
 * If the transport object already has a WaitID then return it, otheriwse assign one.
 * @param  The transport object
 * @return the waitID object
 */
extern uint64_t ism_transport_getWaitID(ism_transport_t * transport);

/*
 * Free the waitID associated with a transport object
 * @param The transport object
 */
extern void ism_transport_freeWaitID(ism_transport_t * transport);


/*
 * Acknowledge something being waited on
 * @param waitid  The wait ID assigned to the transport object
 * @param waitval The wait value
 * @param rc      The reason code for the failure (or 0 for good)
 * @param reason  A reason string or NULL to indicate no reason
 */
extern void ism_transport_ack(uint64_t waitid, int waitval, int rc, const char * reason);

#ifdef __cplusplus
}
#endif
#endif
