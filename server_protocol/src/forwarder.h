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
#ifndef FWD_H_DEFINED
#define FWD_H_DEFINED

#include <cluster.h>
#include <transport.h>
#include <protocol.h>
#include <engine.h>
#include <assert.h>
#include <imacontent.h>

/*
 * A forwarding connection is created from the sender to the receiver.  Messages only flow
 * in the sender to receiver direction, but other information comes back on the back channel.
 *
 * The first action sent on a new connection is a Connect which sends a version, timestamp,
 * server name, and server UID.
 *
 * The receiver responds with a ConnectReply which includes a return code, version, and timestamp.
 *
 * At this point the sender creates all of its engine object and checks if there are any prepared
 * transactions for this receiver.  In any case the sender returns a Recover action with a list of
 * XIDs which is empty in the case there are none.
 *
 * The receiver then processes the prepared transactions at both ends, and sends the appropriate
 * action.  When all transactions are cleaned up, the receiver sends a Start
 * action at which point the sender can begin sending messages.
 *
 * A message sent on the reliable channel has a sequence number.  The sender keeps a sequence number
 * to delivery handle hash.  The receiver creates a new global transaction as required and puts
 * messages within that transaction.  It stores the sequence number with the sender branch of its
 * transaction.
 *
 * When the receiver decides that the transaction is "full" it prepares the receive branch of the
 * transaction and sends a Prepare action which includes the set of sequence numbers to ACK.
 * The tow prepares can be done at the same time.  The sender prepares the send branch of the
 * transaction and then sends a PrepareReply action.
 *
 * When both sides of a transaction are prepared. the receiver acting as the tranasction manager
 * sends a Commit action to the sender to commit its branch, and commits the receive branch.  The
 * sender when done sends a CommitReply action and forgets the transaction.  When the receiver
 * branch commit is complete, and it receives the CommitReply action it can forget the transaction.
 *
 * The receiver can at any point before the prepare send a Rollback action.  The sender should NAK
 * all specified sequence numbers.  Sending a Rollback action should suspend message deliver and
 * the receiver must send a Start action after it has completed a set of rollbacks.
 *
 * If the sender terminates before a transaction is prepared, all un-ACKed messages should be marked
 * for redelivery.
 */

/*
 * Forwarder actions
 */
enum FwdAction_e {
    /* Sender to Receiver  */
    /* Send an unreliable message (QoS=0) */
    FwdAction_Message      = 0x03,   /* h0=seqnum, h1=msgtype, h2=flags, h3=dest, h4=expiry, p=props, b=body */
    /* Send a reliable message */
    FwdAction_RMessage     = 0x04,   /* h0=seqnum, h1=msgtype, h2=flags, h3=dest, h4=expiry, p=props, b=body */
    FwdAction_Recover      = 0x06,   /* h0=xid */
    FwdAction_PrepareReply = 0x07,   /* h0=xid h1=rc */
    FwdAction_CommitReply  = 0x08,   /* h0=xid h1=rc */
    FwdAction_RollbackReply= 0x09,   /* h0=xid */
    /* Sent as first message on the forwarding channel, expects ConnectReply */
    FwdAction_Connect      = 0x0E,   /* h0=version, h1=time, h2=name, h3=uid h4=xids p=config */

    /* Receiver to Sender */
    FwdAction_ConnectReply = 0x11,   /* h0=version, h2=time, h3=rc, h4=xids */
    /* Sent after all transaction processing */
    FwdAction_Start        = 0x12,   /* */
    FwdAction_Prepare      = 0x13,   /* h0=xid, h1=count, b=ids */
    FwdAction_Commit       = 0x14,   /* h0=xid */
    FwdAction_Rollback     = 0x15,   /* h0=xid, h1=count, b=ids */
    FwdAction_Processed    = 0x16,   /* h0=seqnum */
    FwdAction_RequestRetain= 0x17,   /* h0=source h1=options h2=timestamp h3=corrid */
    FwdAction_CommitRecover= 0x18,   /* h0=xid */
    FwdAction_RollRecover  = 0x19,   /* h0=xid */

    /* Both directions */
    FwdAction_Ping         = 0x21,   /* h0=data */
    FwdAction_Pong         = 0x22,   /* h0=data */
    FwdAction_Disconnect   = 0x23,   /* h0=rc, h1=reason */
};

/*
 * Define channel object
 */
struct ismProtocol_RemoteServer_t {
    struct ismProtocol_RemoteServer_t * next;
    const char * name;                /* External server name */
    const char * uid;                 /* Internal unique server name */
    const char * ipaddr;              /* Client IP address of the remote server */
    int          port;                /* Client port of the remote server */
    uint8_t      secure;              /* Use TLS for outgoing connections */
    uint8_t      in_state;            /* Current incoming channel state */
    uint8_t      out_state;           /* Current outgoing channel state */
    uint8_t      cc_state;
    uint8_t      unit_test;
    uint8_t      resv8[3];
    uint32_t     retry;               /* Time in milliseconds to wait for connection retry */
    uint32_t     resvi;
    pthread_mutex_t lock;
    ism_transport_t * in_channel;
    ism_transport_t * out_channel;
    struct fwd_xa_t * sender_xa;      /* Transactions for which this server is the sender */
    struct fwd_xa_t * receiver_xa;    /* Tranasctions for which this server is the receiver */
    uint32_t     version;             /* Version of other side */
    uint64_t     write_msg;
    uint64_t     write_bytes;
    uint64_t     read_msg;
    uint64_t     read_bytes;
    ism_time_t   status_time;         /* Status change time from cluster component */
    uint8_t      cluster_state;       /* State from cluster component */
    uint8_t      cluster_health;      /* Server health from cluster component */
    uint8_t      cluster_memory;      /* Memory percent from cluster component */
    uint8_t      cluster_ha;          /* Cluster HA state */
    uint32_t     connections;         /* Count of connections */
    uint64_t     suspend0;            /* Count of suspends for unreliable */
    uint64_t     suspend1;            /* Count of suspends for reliable */
    uint64_t     old_recv;            /* Previous recv count   */
    uint64_t     old_send0;           /* Previous send count for unreliable */
    uint64_t     old_send1;           /* Previous send count for reliable */
    double       old_send_time;       /* Time of previous send counts */
    pthread_mutex_t dhlock;
    struct fwd_dhmap_t * dhmap;       /* The delivery handle map */
    uint32_t     dhcount;             /* The count of entries in the map */
    uint32_t     dhmore;              /* The point at which we expand the map */
    uint32_t     dhalloc;             /* The allocated slots in the table */
    uint32_t     dhdiv;
    uint32_t     dhextra;
    uint32_t     resvi2;
    double       start_xa;
    struct ismCluster_RemoteServer_t * clusterHandle;
    struct ismEngine_RemoteServer_t *  engineHandle;
    struct fmd_xa_t *  xa;
    ism_timer_t  retry_timer;         /* The timer object for the retry timer */
};
typedef struct ismProtocol_RemoteServer_t ism_fwd_channel_t;

/*
 * Channel connection status
 */
enum ismChannelState_e {
    CHST_Closed        = 0,   /* Originally closed or after error */
    CHST_Open          = 1   /* A connection is open */
};

/*
 * Forwarder consumer
 */
typedef struct ism_fwd_cons_t {
    void *         handle;
    ism_transport_t * transport;
    uint8_t        which;
    uint8_t        suspended;
    uint8_t        resrv[6];
} ism_fwd_cons_t ;

/*
 * Global transaction object
 */
typedef struct fwd_xa_t {
    struct fwd_xa_t * next;
    char  gtrid[64];
    uint64_t sequence;
    ism_xid_t   xid;
    uint8_t prepared;
    uint8_t commit;
    uint8_t resv[2];
} fwd_xa_t;

typedef struct fwd_xa_info_t {
    struct fwd_xa_info_t * next;
    struct fwd_xa_info_t * prev;
    void *                 handle;
    uint64_t               xaSequence;
    char                   gtrid[64];
    uint64_t *             seqnum;            /* Table of sequence numbers in current transaction */
    int                    seqcount;          /* Number of messages in current transaction */
    int                    seqmax;            /* Allocated size of sequence number table */
    int                    readyMsgCounter;
} fwd_xa_info_t;



/*
 * Delivery handle map
 */
typedef struct fwd_dhmap_t {
    uint64_t                   seqn;
    ismEngine_DeliveryHandle_t deliveryh;
} fwd_dhmap_t;

/*
 * The forwarder protocol specific area of the transport object
 */
typedef struct ism_protobj_t {
    struct ism_protobj_t *  next;
    struct ism_protobj_t *  prev;
    void *                  client_handle;
    void *                  session_handle;
    void *                  transaction;
    ism_transport_t *       transport;
    ism_fwd_channel_t *     channel;
    ism_fwd_cons_t          consumer[2];
    pthread_spinlock_t      lock;
    pthread_spinlock_t      sessionlock;
    int32_t                 inprogress;             /* Count of actions in progress */
    int32_t                 keepAlive;
    volatile int            closed;                 /* Connection is not is use */
    uint32_t                sqnum;
    int32_t                 state;
    int32_t                 suspended;
    uint64_t                flowControlCount;
    uint64_t                flowControlSize;
    uint64_t                flowWriteMsg;
    uint64_t                flowWriteBytes;
    uint64_t                flowControlAcks;
    fwd_dhmap_t *           dhmap;
    uint32_t                dhcount;
    uint32_t                dhmore;
    uint32_t                dhalloc;
    uint32_t                dhdiv;
    fwd_xa_info_t *         currentXA;
    fwd_xa_info_t *         xaListHead;
    fwd_xa_info_t *         xaListTail;
    fwd_xa_t *              xaRecoveryList;
    ismHashMap *            errors;            /* Errors that were reported already */
    uint32_t                preparedXA;        /* Count of prepared transactions */
    uint16_t                xaInfoListSize;
    uint8_t                 closing;
    uint8_t                 resv;
} ism_protobj_t;

typedef struct ism_protobj_t   ismFwdPobj_t;

/*
 * Action structure for forwarder incoming action
 */
typedef struct ism_fwd_act_t {
    uint8_t    action;
    uint8_t    hdrcount;
    uint8_t    paction;
    uint8_t    options;
    int        rc;
    int        connect;
    concat_alloc_t buf;
    ism_fwd_channel_t * channel;
    ism_transport_t * transport;
    ism_field_t * hdrs;
    ism_field_t pfield;
    ism_field_t body;
    uint64_t    seqnum;
} ism_fwd_act_t;

/*
 * Structure for creating engine transaction
 */
typedef struct fwd_xatr_t {
    ism_transport_t *   transport;
    uint64_t            sequence;
    char                gtrid[64];
} fwd_xatr_t;

/*
 * Structure for connection action
 */
typedef struct fwd_conact_t {
    uint8_t    action;
    uint8_t    resv[3];
    int        version;
    int        rc;
    int        resvi;
    uint64_t   timestamp;
    ism_transport_t * transport;
} fwd_conact_t;

typedef struct fwd_msgact_t {
    uint8_t    action;
    uint8_t    resv[3];
    int        rc;
    uint64_t   seqnum;
    ism_transport_t * transport;
    void *     xaHandle;
} fwd_msgact_t;

typedef struct fwd_xa_action_t {
    int       count;
    int       which;
    uint8_t   op;
    uint8_t   action;
    uint8_t   inheap;
    uint8_t   resv[5];
    char gtrid[64];
    uint64_t  sequence;
    fwd_xa_t *        xa;
    ismEngine_TransactionHandle_t transh;
    ism_transport_t * transport;
    ismEngine_DeliveryHandle_t * deliveryh;
} fwd_xa_action_t;


/*
 * Structure for creating transactions during recovery
 */
typedef struct fwd_recover_action_t {
    uint8_t           action;
    uint8_t           resv[7];
    ism_transport_t * transport;
    fwd_xa_t *        xa;
    char              gtrid [48];
} fwd_recover_action_t;


/*
 * Global variables - defined in forwarder.c
 */
extern ism_transport_t * fwd_transport;
extern ismEngine_ClientStateHandle_t fwd_client;
extern ismEngine_SessionHandle_t fwd_sessionh;
extern volatile int      fwd_startMessaging ;
extern struct ssl_ctx_st * fwd_tlsCTX;     /* TLS context  */
extern volatile int  fwd_stopping;
extern uint64_t fwd_flowCount;
extern uint64_t fwd_flowSize;
extern int      fwd_unit_test;
extern int      fwd_Version2_0;
extern int      fwd_Version_Current;
extern pthread_mutex_t fwd_configLock;
extern int fwd_commit_count;
extern int fwd_commit_time;

extern uint32_t fwd_maxXA;
extern uint32_t fwd_minXA;


/*
 * Outgoing connection connected callback
 */
int  ism_fwd_connected(ism_transport_t * transport, int rc);

/*
 * Start messaging callback
 */
int ism_fwd_startMessaging(void);

/*
 * Outgoing connection complete
 */
int  ism_fwd_connection(ism_transport_t * transport);

/*
 * Receive from the forwarder channel
 */
int ism_fwd_receive(ism_transport_t * transport, char * buf, int buflen, int kind);

/*
 * Add a connection to the list of clients
 */
void ism_fwd_addToClientList(ismFwdPobj_t * pobj);

/*
 * Remove a connection from the list of clients
 */
void ism_fwd_removeFromClientList(ismFwdPobj_t * pobj, int lock);

/*
 * The connection is closing.
 */
int ism_fwd_closing(ism_transport_t * transport, int rc, int clean, const char * reason);

/*
 * Return names for fowarder actions
 */
const char * ism_fwd_getActionName(int action);

/*
 * Disconnect a channel
 */
int ism_fwd_disconnectChannel(ism_fwd_channel_t * channel);

/*
 * Start a channel
 */
int ism_fwd_startChannel(ism_fwd_channel_t * channel);

/*
 * Handle a Connection action
 */
int ism_fwd_doConnect(ism_fwd_act_t * action, uint32_t version, uint64_t timest,
        const char * name, const char * uid);

/*
 * Handle a Message action
 */
int ism_fwd_doMessage(ism_fwd_act_t * action, uint64_t seqnum, int msgtype,
        int flags, const char * dest, uint32_t expiry, ism_field_t * props, ism_field_t * body);

/*
 * Handle a ConnectReply action
 */
int ism_fwd_doConnectReply(ism_fwd_act_t * action, int rc, int version, uint64_t tstamp, int flags);

/*
 * Handle a Recover action
 */
int ism_fwd_doRecover(ism_fwd_act_t * transport, const char * xids);

/*
 * Handle a Prepare action
 */
int ism_fwd_doPrepare(ism_fwd_act_t * action, const char * xid, int count);

/*
 * Handle a Commit action
 */
int ism_fwd_doCommit(ism_fwd_act_t * action, const char * xid, int recover);

/*
 * Handle a Rollback action
 */
int ism_fwd_doRollback(ism_fwd_act_t * action, const char * xid, int count);

/*
 * Handle a Rollback recover action
 */
int ism_fwd_doRollbackPrepared(ism_fwd_act_t * action, const char * xid, int recover);

/*
 * Handle the Processed action
 */
int ism_fwd_doProcessed(ism_fwd_act_t * action, uint64_t seqnum);

/*
 * Handle a Forget action
 */
int ism_fwd_doForget(ism_fwd_act_t * action, const char * xid);

/*
 * Handle a PrepareReply action
 */
int ism_fwd_doPrepareReply(ism_fwd_act_t * action, const char * xid, int rc);

/*
 * Handle a CommitReply action
 */
int ism_fwd_doCommitReply(ism_fwd_act_t * action, const char * xid, int rc);

/*
 * Handle a Start action
 */
int ism_fwd_doStart(ism_fwd_act_t * action);

/*
 * Make a new XID.
 *
 * This can only be done on the coordinator side which is the receive side
 * in the forwarder.
 * @param xid     The allocated xid object.  Any content will be replaced
 * @param sender  The sender UID
 * @return  A xa which is filled in with the gtrid or NULL to indicate an error
 */
int ism_fwd_xa_init(fwd_xa_t * xa, const char * uid);

/*
 * Make an xa object from a gtrid
 */
fwd_xa_t * ism_fwd_makeXA(const char * gtrid, char branch, uint64_t sequence);

/*
 * Construct a new global transaction ID
 */
uint64_t ism_fwd_newGtrid(char * gtrid, const char * sender);

/*
 * Make a XID.
 * This does not checking as it is internal.
 */
void ism_fwd_makeXid(ism_xid_t * xid, char branch, const char * gtrid);

/*
 * Link in a transaction.
 * The transactions are linked in order from smallest to largest sequence.
 */
void ism_fwd_linkXA(ism_fwd_channel_t * channel, fwd_xa_t * xa, int sender, int lock);

/*
 * Unlink a transaction
 */
void ism_fwd_unlinkXA(ism_fwd_channel_t * channel, fwd_xa_t * xa, int sender, int lock);

/*
 * Find a transaction
 */
fwd_xa_t * ism_fwd_findXA(ism_fwd_channel_t * channel, const char * xid, int sender, int lock);

/*
 * Message engine callback
 */
bool ism_fwd_replyMessage(ismEngine_ConsumerHandle_t consumerh,
        ismEngine_DeliveryHandle_t deliveryh, ismEngine_MessageHandle_t msgh,
        uint32_t seqnum, ismMessageState_t state, uint32_t options,
        ismMessageHeader_t * hdr, uint8_t areas,
        ismMessageAreaType_t areatype[areas], size_t areasize[areas],
        void * areaptr[areas], void * vaction, ismEngine_DelivererContext_t * _delivererContext);
/*
 * Return the forwarder endpoint
 */
ism_endpoint_t * ism_fwd_getOutEndpoint(void);

/*
 * Close the client as a reply
 */
void ism_fwd_replyCloseClient(ism_transport_t * transport);

/*
 * Replace a string in a channel object
 */
void ism_fwd_replaceString(const char * * oldval, const char * val);

/*
 * Retry an outgoing connection
 */
int ism_fwd_retryOutgoing(ism_timer_t key, ism_time_t timestamp, void * userdata);

/*
 * Request retained messages
 */
int ism_fwd_doRequestRetain(ism_fwd_act_t * action, const char * originUID, uint32_t options, ism_time_t tstamp, uint64_t corrid);

/*
 * Engine callback to request retained messages
 */
int32_t ism_fwd_requestRetain(const char * requestUID, const char * originUID, uint32_t optioss, uint64_t tstamp, uint64_t corrid);

#endif
