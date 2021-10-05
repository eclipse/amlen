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
#define TRACE_COMP Jms
#define ACTION_NAMES
#include <ismmessage.h>
#include <transport.h>
#include <engine.h>
#include <protocol.h>
#include <admin.h>
#include <security.h>
#include "fendian.h"
#include "imacontent.h"
#include <ismjson.h>
#include <assert.h>
#include <selector.h>
#include <stddef.h>
#include <stdbool.h>
#include <alloca.h>
#include <limits.h>

#ifndef XSTR
#define XSTR(s) STR(s)
#define STR(s) #s
#endif

#ifdef PERF
#include <perfstat.h>
static int        histSize = 10000;     /* can contain up to 10 millisecond latencies */
static double     histUnits = 1e-6;     /* units are in microseconds */
static double     invHistUnits;         /* inverse of histogram units */
static int        histSampleRate = 0;   /* 0 means do not take measurements */
ism_latencystat_t *commitStat;
ism_latencystat_t *msgProcStat;
#endif

#define OffsetOf(s,m) offsetof(s,m)

#define MkVersion(v,r,m,f) (v)*1000000+(r)*10000+(m)*100+(f)

/*
 * Server version:
 * 1.1.0.0 Add shared subscriptions, add XA transactions
 */
xUNUSED
static int ImaVersion_1     = MkVersion(1,0,0,0);
static int ImaVersion_1_1   = MkVersion(1,1,0,0);
xUNUSED
static int ImaVersion_2_0   = MkVersion(2,0,0,0);
static int ServerVersion    = MkVersion(2,0,0,0);

static int jms_unit_test = 0;
static int g_disable_auth = 0;
static int jmsMaxConsumers = 1000;

static pthread_mutex_t     jmslock;

/*
 * This structure is used as the callback structure for receive
 */
typedef struct ism_rcv_t {
    ism_transport_t *  transport;
    double             tsc;
    struct actionhdr   hdr;
} ism_rcv_t;

/*
 * Enumeration for conversion type
 */
enum convert_type_e {
    CT_Unknown  = 0,
    CT_Auto     = 1,
    CT_Bytes    = 2,
    CT_Text     = 3
};

ism_enumList enum_cvttype [] = {
    { "ConvertType",  4,       },
    { "auto",         CT_Auto  },
    { "automatic",    CT_Auto  },
    { "bytes",        CT_Bytes },
    { "text",         CT_Text  },
};

/*
 * Possible values for action->subscriptionFound
 */
enum sub_found_type_e {
    SUB_NotFound    = 0,
    SUB_Found       = 1,
    SUB_Error       = 2,
    SUB_Resubscribe = 3
};

/*
 * Restart position for recreateConsumerAndSubscription
 */
enum re_subscribe_e {
    RESUB_DestroySubscription = 3,
    RESUB_CreateSubscription  = 4,
    RESUB_CreateConsumer      = 5
};

/*
 * Display name of domain for trace
 */
static const char * domainName(int domain) {
    switch (domain) {
    case ismDESTINATION_QUEUE:   return "queue";
    case ismDESTINATION_TOPIC:   return "topic";
    default:                     return "unknown";
    }
}

/*
 * List of updateable properties of durable subscriptions
 */
static const char * const updateableProperties[] = { "MaxMessages" };

/*
 * Producer / consumer tables
 */
typedef struct ism_jms_prodcons_t {
    ism_transport_t * transport;
    void *    handle;
    struct ism_jms_session_t * session;
    char *    name;
    char *    subName;
    char *    selector;
    struct actionhdr   hdr;
    uint32_t  which;
    uint8_t   domain;
    uint8_t   closing;
    uint8_t   kind;
    uint8_t   noack;
    uint8_t   orderIdRequested;  /* the order ID has been requested */
    uint8_t   msglistener;
    uint8_t   suspended;
    uint8_t   shared;      /* SHARED_* */
    int       rulelen;
    int       incompMsgCount;
    int       cacheSize;
    int       inBatch;
    ismRule_t * rule;
    pthread_spinlock_t lock;
} ism_jms_prodcons_t;

#define  KIND_PRODUCER 0
#define  KIND_CONSUMER 1
#define  KIND_BROWSER  2

#define SHARED_False       0
#define SHARED_True        1
#define SHARED_NonDurable  2
#define SHARED_Global      3
#define SHARED_GlobalND    4

/*
 * Entry in the list of undelivered messages
 */
typedef struct ism_undelivered_message_t {
    uint64_t                   msgID;
    ismEngine_DeliveryHandle_t deliveryHandle;        /* Delivery handle           */
    ism_jms_prodcons_t *       consumer;              /* A pointer to the consumer */
    const char * subName;                             /* Durable subscription name */
    struct ism_undelivered_message_t * prev;
    struct ism_undelivered_message_t * next;
} ism_undelivered_message_t;

/*
 * Session tables
 */
typedef struct ism_jms_session_t {
    void *    handle;
    ismEngine_TransactionHandle_t transaction;
    uint64_t  seqnum;           /* Next message id (seqnum) to be assigned */
    uint64_t  lastAckSeqNum;    /* Last acknowledged message id */
    uint32_t  which;
    int       suspended;
    /* Pointers to the list of seqnum->undelivered message handle and consumer (to check if durable) */
    ism_undelivered_message_t * incompMsgHead;
    ism_undelivered_message_t * incompMsgTail;
    ism_undelivered_message_t * freeMsgs;
    int       incompMsgCount;
    pthread_spinlock_t lock;
//    ism_xid_t xid;
    int       transactionError;
    uint8_t   domain;
    uint8_t   transacted;   /* 0 - not transacted, 1 - local transaction, 2 - global transaction */
#define JMS_LOCAL_TRANSACTION    1
#define JMS_GLOBAL_TRANSACTION   2
    uint8_t   ackmode;
    uint8_t   rsrv;
} ism_jms_session_t;


/*
 * JMS protocol specific
 */
typedef struct ism_protobj_t {
    void *    handle;
    void *    browser_session_handle;
    pthread_spinlock_t lock;
    pthread_spinlock_t sessionlock;
    ism_jms_session_t * * sessions;
    int       sessions_used;
    int       sessions_alloc;
    ism_jms_prodcons_t * * prodcons;
    int       prodcons_used;
    int       prodcons_alloc;
    ismHashMap * errors;              /* Errors that were reported already */
    int32_t   inprogress;             /* Count of actions in progress */
    char      instanceHash [4];
    uint8_t   closed;
    uint8_t   started;
    uint8_t   convertType;
    uint8_t   isGenerated;
    int       subscribeLock;          /* Only allow one subscribe at a time */
    int       consumer_count;
    int       keepAliveCheckInterval;
    volatile ism_timer_t keepAliveTimer;
    int       keepaliveTimeout;       /* Timeout for connection, 0=none         */
    int       client_version;		  /* Client version sent in initConnection   */
    char      client_build_id[24];    /* Client build id sent in initConnection */
} ism_protobj_t;

typedef ism_protobj_t jmsProtoObj_t;

/*
 * Action structure.  This is the parsed action.
 */
typedef struct ism_protocol_action_t {
    ism_transport_t *    transport;              /* Must be first item in consumer context */
    double               tsc;
    actionhdr            hdr;                   /* hdr and buf MUST not be separated by any other */
    char                 buf[48];               /* other field as they serve as a send buffer */
    int                  rc;
    uint8_t              subscriptionFound;
    uint8_t              nolocal;
    uint8_t              shared;                /* SHARED_* */
    uint8_t              noConsumer;
    int                  rulelen;
    int                  valcount;
    int                  propcount;
    uint32_t             flag;
    struct ism_protocol_action_t * old_action;  /* Needs reset if the action is moved */
    ism_field_t *        values;
    ism_propent_t *      props;
    ism_jms_prodcons_t * prodcons;
    ism_jms_session_t *  session;
    ismRule_t *          rule;
    ism_transport_t *    clientTrans;          /* Set only if we are in a subscription */
    concat_alloc_t       xbuf;
    int64_t              recordCount;
    int64_t              deliveryTime;
    int                  actionsize;
} ism_protocol_action_t;

static ismEngine_ClientStateHandle_t client_Shared = NULL;
static ismEngine_ClientStateHandle_t client_SharedND = NULL;
static ism_transport_t *             transport_Shared = NULL;
static ism_transport_t *             transport_SharedND = NULL;

#if 0
static void mypthread_spin_lock(pthread_spinlock_t * lock, const char * file, int line) {
    int opt = TOPT_TIME | TOPT_THREAD | TOPT_WHERE;
    traceFunction(5, opt, file, line, " >>> mypthread_spin_lock: lock=%p value=%d\n", lock, ((int)(*lock)));
    pthread_spin_lock(lock);
    traceFunction(5, opt, file, line, " <<< mypthread_spin_lock: lock=%p value=%d\n", lock, ((int)(*lock)));
}

static void mypthread_spin_unlock(pthread_spinlock_t * lock, const char * file, int line) {
    int opt = TOPT_TIME | TOPT_THREAD | TOPT_WHERE;
    traceFunction(5, opt, file, line, " >>> mypthread_spin_unlock: lock=%p value=%d\n", lock, ((int)(*lock)));
    pthread_spin_unlock(lock);
    traceFunction(5, opt, file, line, " <<< mypthread_spin_unlock: lock=%p value=%d\n", lock, ((int)(*lock)));
}
#endif

/*
 * Forward references
 */
static ism_jms_session_t * getSession(ism_transport_t * transport, int session_id);
static ism_jms_prodcons_t * getProdcons(ism_transport_t * transport, int pc_id);
static void convertJSONBody(concat_alloc_t * buf, char * body, uint32_t bodylen, int inarray);
static void createDurableConsumer(int32_t rc, void * handle, void * vaction);
static int jmsCloseConsumer(ism_protocol_action_t * action);
static int jmsConnectionResume(ism_transport_t * transport, void * userdata);
static void jmsReSubscribe(ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName, const char *pTopicString,
        void * properties, size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes, uint32_t consumerCount, void * vaction);

static void freeConsumer(ism_jms_prodcons_t * consumer);
static int getbooleanproperty(ism_protocol_action_t * action, const char * name);
static int getintproperty(ism_protocol_action_t * action, const char * name, int deflt);
static const char * getproperty(ism_protocol_action_t * action, const char * name);
static int clearUndeliveredMessages(ism_protocol_action_t * action, ismEngine_CompletionCallback_t  pCallbackFn);

static void cleanupConsumer(int32_t rc, void * handle, void * vaction);
static void recreateConsumerAndSubscription(int32_t rc, void * handle, void * vaction);
static void createTransaction(int32_t rc, void * handle, void * vaction);
static void addSubscription(ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName, const char * pTopicString, void * properties, size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes,  uint32_t consumerCount, void * vaction);
static uint64_t* getAckSqn(jmsProtoObj_t * pobj, concat_alloc_t * map, uint32_t acksqn_count,
                           uint64_t * acksqn, uint32_t acksqn_alloc,uint64_t * pMaxSqn, int * rc);
static int clearConsumerUndeliveredMessage(ism_jms_session_t * session, ism_jms_prodcons_t * consumer,
		uint64_t minSQN, int onClose, ism_protocol_action_t * action, ismEngine_CompletionCallback_t pCallbackFn);
static ismMessageAreaType_t MsgAreas [2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
xUNUSED static ismMessageAreaType_t MsgAreasNoProps [1] = {ismMESSAGE_AREA_PAYLOAD};
static int previouslyLogged(jmsProtoObj_t * pobj, int msgcode);
static int resumeConnectionDelivery(ism_transport_t * transport, void * userdata, uint64_t flags);
static int resumeSessionDelivery(ism_transport_t * transport, void * userdata, int flags);
static int resumeConsumerDelivery(ism_transport_t * transport, void * userdata, uint64_t flags);
static int jmsDumpPobj(ism_transport_t * transport, char * buf, int len);
static char * showVersion(int version, char * vstring);
static int doSubscribe(ism_transport_t * transport, void * vaction, int flags);
static int doUnsubscribe(ism_transport_t * transport, void * vaction, int flags);
static int doUpdate(ism_transport_t * transport, void * vaction, int flags);
static int jmsConnection(ism_transport_t * transport);
static void freeUndeliveredMessages(ism_jms_session_t * session);
static int keepAliveTimer(ism_timer_t key, ism_time_t timestamp, void * userdata);
/*
 * Methods in jmsreply.c
 */
static void jmsReplyAuth(int rc, void * callbackParam);
static int jmsReplyAuthTT(ism_timer_t timer, ism_time_t timestamp, void * callbackParam);
static void replyAction(int32_t rc, void * handle, void * vaction);
static void replyCommitSession(int32_t rc, void * handle, void * vaction);
static void replyCreateBrowser(int32_t rc, void * handle, void * vaction);
static void replyCreateConsumer(int32_t rc, void * handle, void * vaction);
static void replyCloseConsumer(int32_t rc, void * handle, void * vaction);
static void replyCloseSession(int32_t rc, void * handle, void * vaction);
static void replyStopSession(int32_t rc, void * handle, void * vaction);
static void replyClosing(int32_t rc, void * handle, void * vaction);
static void replyError(int32_t rc, void * handle, void * vaction);
static void replyGetRecords(void * data, size_t dataLength, void * handle, void * vaction);
static void replyMessage(int32_t rc, void * handle, void * vaction);
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
        void *                     vaction);
static void replyRollbackSession(int32_t rc, void * handle, void * vaction);
static void replyUnsubscribeDurable(int32_t rc, void * handle, void * vaction);
static void replyUpdateDurable(int32_t rc, void * handle, void * vaction);
static inline void handleReplyRecover(int32_t rc, void * handle, void * vaction);

/*
 * Modify the shared subscription name as the durable and non-durable shared subscription names
 * are not allowed to conflict with each other.
 *
 * Add an underscore before the name for non-durable, but to do this we must escape an underscore
 * in the first character of the subscription name.
 *
 * This is only done for shared subscriptions
 */
#define SHARED_SUB_NAME_DURABLE(dname) \
    if (((dname)[0]=='_' || (dname)[0]=='\\') && dname[1]!='_') { \
        char * fullSubName = alloca(2+strlen((dname))); \
        fullSubName[0] = '\\'; \
        strcpy(fullSubName+1, (dname)); \
        (dname) = fullSubName; \
    }

#define SHARED_SUB_NAME_NONDURABLE(dname) { \
    char * fullSubName = alloca(3+strlen((dname))); \
    char * dp = fullSubName; \
    *dp++ = '_'; \
    if ((dname)[0]=='_' || (dname)[0]=='\\') \
        *dp++ = '\\'; \
    strcpy(dp, (dname)); \
    (dname) = fullSubName; \
}

#if 0
static void dumpAction(ism_protocol_action_t *action, const char * fn, const char * file, int line) {
    int opt = TOPT_TIME | TOPT_THREAD | TOPT_WHERE;
    char xbuf[8192];
    char * buf = xbuf;
    uint64_t * ptr = (uint64_t*) action;
    void * data = action->props + action->propcount;
    //int dataSize = action->actionsize - (data - action);
    //int size = (dataSize > 800) ? 100 : (dataSize / 8);
    uint64_t * endPtr = (uint64_t*)(((char*)action) + action->actionsize);
    if((data > ((void*)action)) && (data < ((void*)endPtr))) {
      int n = 0;
      do {
          buf += n;
          n = sprintf(buf, " %p", ((void*)(*ptr)));
          ptr++;
      } while ( ((void*)ptr) < ((void*)data));
      strcat(buf, " |");
      n += 2;
      while(ptr < endPtr) {
          buf += n;
          n = sprintf(buf, " %p", ((void*)(*ptr)));
          ptr++;
      }
      strcat(buf,"}");
    } else {
	xbuf[0] = '\0';
    }
    traceFunction(5, opt, file, line, "%s: action=%p oldAction=%p props=%p values=%p data=%p { %s} \n",
            fn, action, action->old_action, action->props, action->values, data, xbuf);
}
#endif

#define resetAction(x) resetActionInt((x), __FILE__, __LINE__)
#define action2actionBuff(x) action2actionBuffInt((x), __FILE__, __LINE__)

/*
 * Recalculate all relative fields in action if it was passed asynchronously
 */
static void resetActionInt(ism_protocol_action_t *action, const char * file, int line) {
//    dumpAction(action, " >> resetAction", file, line);
    if (action->old_action && (action != action->old_action)) {
        int i;
        char * newData;
        char * oldData = (char *)(action->props + action->propcount);
        uintptr_t diff;
        action->values = (ism_field_t *)(action+1);
        action->props  = (ism_propent_t *)(action->values + action->valcount);
        newData = (char *)(action->props + action->propcount);
        diff = newData - oldData;
        for (i = 0; i < action->valcount; i++) {
            ism_field_t * field = action->values+i;
            if ((field->type == VT_String) || (field->type == VT_ByteArray)) {
                field->val.s += diff;
            }
        }
        for (i = 0; i < action->propcount; i++) {
            ism_propent_t * prop = action->props+i;
            prop->name += diff;
            if ((prop->f.type == VT_String) || (prop->f.type == VT_ByteArray)) {
                prop->f.val.s += diff;
            }
        }
        action->old_action = action;
    }
//    dumpAction(action, " << resetAction", file, line);
}

/*
 * Allocate a session id
 */
static int  setSession(ism_transport_t * transport, ism_jms_session_t * session) {
    int  i;
    int newSize;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    ism_jms_session_t * * newArray;
    pthread_spin_lock(&(pobj->lock));
    for (i=1; i<pobj->sessions_alloc; i++) {
         ism_jms_session_t * curr = pobj->sessions[i];
        if (curr == NULL) {
            session->which = i;
            pobj->sessions[i] = session;
            pobj->sessions_used++;
            pthread_spin_unlock(&(pobj->lock));
            return i;
        }
    }
       newSize = pobj->sessions_alloc*2;
       newArray = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,177),newSize,(sizeof(ism_jms_session_t *)));
    if (newArray == NULL) {
        pthread_spin_unlock(&(pobj->lock));
        return 0;
    }
    memcpy(newArray,pobj->sessions,pobj->sessions_alloc*sizeof(ism_jms_session_t *));
    ism_common_free(ism_memory_protocol_misc,pobj->sessions);
    pobj->sessions = newArray;
    pobj->sessions_alloc = newSize;
    session->which = i;
    pobj->sessions[i] = session;
    pobj->sessions_used++;
    pthread_spin_unlock(&(pobj->lock));
    return i;
}


/*
 * Allocate a producer or consumer ID
 */
static int  setProdcons(ism_transport_t * transport, ism_jms_prodcons_t * pc) {
    int  i;
    int newSize;
    ism_jms_prodcons_t * * newArray;
    ism_jms_prodcons_t * curr = NULL;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;

    pthread_spin_lock(&(pobj->lock));

    /* Check for too many consumers while under the lock */
    if ((pc->kind == KIND_CONSUMER || pc->kind == KIND_BROWSER)) {
        if (pobj->consumer_count >= jmsMaxConsumers) {
            pthread_spin_unlock(&(pobj->lock));
            return -1;
        }
        pobj->consumer_count++;
    }

    /* Find first empty slot */
    for (i = 1; i < pobj->prodcons_alloc; i++) {
        curr = pobj->prodcons[i];
        if (curr == NULL) {
            pobj->prodcons[i] = pc;
            pobj->prodcons_used++;
            pthread_spin_unlock(&(pobj->lock));
            return i;
        }
    }

    newSize = pobj->prodcons_alloc * 2;
    newArray = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,179),newSize, sizeof(ism_jms_prodcons_t *));
    if (newArray == NULL) {
        if ((pc->kind == KIND_CONSUMER || pc->kind == KIND_BROWSER))
            pobj->consumer_count--;
        pthread_spin_unlock(&(pobj->lock));
        ism_common_setError(ISMRC_AllocateError);
        return 0;
    }
    memcpy(newArray,pobj->prodcons,
    		pobj->prodcons_alloc * sizeof(ism_jms_prodcons_t *));
    ism_common_free(ism_memory_protocol_misc,pobj->prodcons);
    pobj->prodcons = newArray;
    pobj->prodcons_alloc = newSize;
    pobj->prodcons[i] = pc;
    pobj->prodcons_used++;
    pthread_spin_unlock(&(pobj->lock));
    return i;
}


/*
 * Get a session handle from a session
 */
static inline ism_jms_session_t * getSession(ism_transport_t * transport, int session_id) {
    ism_jms_session_t *result = NULL;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;

    pthread_spin_lock(&(pobj->lock));
    if ((session_id > 0) && (session_id < pobj->sessions_alloc) && pobj->sessions_used)
        result = pobj->sessions[session_id];
    pthread_spin_unlock(&(pobj->lock));
    return result;
}


/*
 * Get a session handle from a session
 */
static ism_jms_session_t * removeSession(ism_transport_t * transport, int session_id) {
    ism_jms_session_t *result = NULL;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;

    pthread_spin_lock(&(pobj->lock));
    if ((session_id > 0) && (session_id < pobj->sessions_alloc)) {
        result = pobj->sessions[session_id];
        pobj->sessions[session_id] = NULL;
        pobj->sessions_used--;
        result->handle = 0;        /* Ensure that if the session is accessed under lock, handle is not set */
    }
    pthread_spin_unlock(&(pobj->lock));
    return result;
}


/*
 * Get a producer or consumer handle from a session
 */
static ism_jms_prodcons_t * getProdcons(ism_transport_t * transport, int pc_id) {
    ism_jms_prodcons_t *result = NULL;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;

    pthread_spin_lock(&(pobj->lock));
    if ((pc_id > 0) && (pc_id < pobj->prodcons_alloc))
        result = pobj->prodcons[pc_id];
    pthread_spin_unlock(&(pobj->lock));
    return result;
}


/*
 * Remove the producer and consumer table
 */
static ism_jms_prodcons_t * removeProdcons(ism_transport_t * transport, int pc_id, int isConsumer) {
    ism_jms_prodcons_t *result = NULL;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    pthread_spin_lock(&(pobj->lock));
    if ((pc_id > 0) && (pc_id < pobj->prodcons_alloc)) {
        result = pobj->prodcons[pc_id];
        pobj->prodcons[pc_id] = NULL;
        if (result) {
        pobj->prodcons_used--;
            if (isConsumer)
                pobj->consumer_count--;
        }
    }
    pthread_spin_unlock(&(pobj->lock));
    return result;
}

struct sub_name {
    ism_transport_t * transport;
    char              subName[1];
};

/*
 * Check if there are no consumers.
 */
static void checkUnsubNonDurable(ismEngine_SubscriptionHandle_t subHandle, const char * pSubName, const char *pTopicString,
        void * properties, size_t propertiesLength, const ismEngine_SubscriptionAttributes_t *pSubAttributes, uint32_t consumerCount, void * subName) {
    xUNUSED int rc;
    struct sub_name * sub_name = (struct sub_name *)subName;
    TRACEL(8, sub_name->transport->trclevel, "checkUnsubNonDurable name=%s clientid=%s count=%u\n", pSubName, sub_name->transport->name, consumerCount);
    rc = ism_engine_destroySubscription(sub_name->transport->pobj->handle,
             pSubName, sub_name->transport->pobj->handle, NULL, 0, NULL);
}

/*
 *
 */
static int unsubNondurable(ism_transport_t * transport, void * subName, int flags) {
    xUNUSED int rc;
    struct sub_name * sub_name = (struct sub_name *)subName;
    if (__sync_bool_compare_and_swap(&sub_name->transport->pobj->subscribeLock, 0, 1)) {
        /* TODO: handle async return */
        rc = ism_engine_listSubscriptions(sub_name->transport->pobj->handle, sub_name->subName, subName, checkUnsubNonDurable);
        sub_name->transport->pobj->subscribeLock = 0;
        ism_common_free(ism_memory_protocol_misc,subName);
        return 0;
    } else {
        return 1;
    }
}

/*
 * Check if there are no consumers.
 */
static void checkNoConsumers(ismEngine_SubscriptionHandle_t subHandle, const char * pSubName, const char *pTopicString,
        void * properties, size_t propertiesLength, const ismEngine_SubscriptionAttributes_t *pSubAttributes, uint32_t consumerCount, void * xport) {
    ism_transport_t * transport = xport;
    xUNUSED int rc;
    TRACEL(8, transport->trclevel, "checkNoConsumers name=%s clientid=%s count=%u\n", pSubName, transport->name, consumerCount);
    if (__sync_bool_compare_and_swap(&transport->pobj->subscribeLock, 0, 1)) {
        /* TODO: handle async return */
        rc = ism_engine_destroySubscription(transport->pobj->handle, pSubName, transport->pobj->handle, NULL, 0, NULL);
        transport->pobj->subscribeLock = 0;
    } else {
        struct sub_name * sub_name = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,182),sizeof(struct sub_name) + strlen(pSubName));
        sub_name->transport = transport;
        strcpy(sub_name->subName, pSubName);
        transport->addwork(transport, unsubNondurable, (void *)sub_name);
    }
}


/*
 * Check the subscription consumer count.
 */
static void checkSubscriptionConsumer(const char * subName, ism_transport_t * transport) {
    TRACEL(8, transport->trclevel,"checkSubscriptionConsumer name=%s clientid=%s\n", subName, transport->name);
    xUNUSED int rc = ism_engine_listSubscriptions(transport->pobj->handle, subName, transport, checkNoConsumers);
}


/*
 * Show the name of an action
 */
static const char * getActionName(int action) {
    if (action<0 || action>MAX_ACTION_VALUE) {
        return "Unknown";
    }
    return ism_action_names[action];
}



/*
 * Free an undelivered message for the list
 */
static inline void freeUndeliveredMessage(ism_jms_session_t * session, ism_undelivered_message_t *msg) {
    if (msg->prev) {
        msg->prev->next = msg->next;
    } else {
        session->incompMsgHead = msg->next;
    }
    if (msg->next) {
        msg->next->prev = msg->prev;
    } else {
        session->incompMsgTail = msg->prev;
    }
    session->incompMsgCount--;
    msg->consumer->incompMsgCount--;
    msg->prev = NULL;
    msg->next = session->freeMsgs;
    session->freeMsgs = msg;
}



/*
 * Unhash a string
 */
static char * instanceUnHash(ism_transport_t * transport, const char * in, char * out) {
    static char a16[] = "rtoaphcleduminsk";
    char * cp = out;
    int    pos = 0;
    int    digit;
    char * dloc;
    char   ch;
    int    where = 0;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;

    while (*in) {
        ch = *in++;
        dloc = strchr(a16, ch);
        if (dloc) {
            if (pos) {
                *cp++ = ((digit<<4) + (dloc-a16)) ^ pobj->instanceHash[where++ % 4];
                pos = 0;
            } else {
                digit = dloc-a16;
                pos = 1;
            }
        }
    }
    *cp = 0;
    return out;
}

/*
 * Move selectMessage to utils but leave this one until all links are gone
 */
int ism_protocol_selectMessage(
        ismMessageHeader_t *       hdr,
        uint8_t                    areas,
        ismMessageAreaType_t       areatype[areas],
        size_t                     areasize[areas],
        void *                     areaptr[areas],
        const char *               topic,
        void *                     rule,
        size_t                     rulelen) {
    return ism_common_selectMessage(hdr, areas, areatype, areasize, areaptr, topic, rule, rulelen);
}


/*
 * Convert the JSON body to a JMS type
 */
static void convertJSONBody(concat_alloc_t * buf, char * body, uint32_t bodylen, int inarray) {
    int rc;
    ism_json_parse_t   parseobj;
    ism_json_entry_t * ent;

    /*
     * Parse the JSON
     */
    memset(&parseobj, 0, sizeof(parseobj));
    parseobj.src_len = bodylen;
    parseobj.source = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,183),bodylen+1);
    memcpy(parseobj.source, body, bodylen);
    parseobj.source[bodylen] = 0;   /* null terminate for debug */
    rc = ism_json_parse(&parseobj);

    ent = parseobj.ent;
    if (rc == 0 && ((inarray && ent->objtype==JSON_Array) || (!inarray && ent->objtype==JSON_Object))) {
        ism_protocol_putJSONMapValue(buf, ++ent, parseobj.ent_count, inarray);
    }
}




/*
 * Find the property by name
 */
static const char * getproperty(ism_protocol_action_t * action, const char * name) {
    int   i;
    ism_propent_t * ent = action->props;
    for (i=0; i<action->propcount; i++) {
        if (!strcmp(ent->name, name)) {
             if (ent->f.type == VT_String) {
                 return ent->f.val.s;
             }
        }
        ent++;
    }
    return NULL;
}


/*
 * Modified version of strtol which handles hex and decimal, but not octal
 */
static int str2l(const char * str, char * * endp) {
    const char * cp = str;
    int  ret;
    while (*cp==' ' || *cp=='\t') cp++;
    if (*cp == '0' && cp[1]=='x')
        ret = strtol(cp+2, endp, 16);
    else
        ret = strtol(cp, endp, 10);
    if (endp && *endp > str && **endp) {
        cp = *endp;
        while (*cp==' ' || *cp=='\t') cp++;
        *endp = (char *)cp;
    }
    return ret;
}

/*
 * Get an integer property from the action
 */
xUNUSED static int getintproperty(ism_protocol_action_t * action, const char * name, int deflt) {
    int   i;
    int   val;
    char * eos;
    ism_propent_t * ent = action->props;
    for (i=0; i<action->propcount; i++) {
        if (!strcmp(ent->name, name)) {
            switch (ent->f.type) {
            case VT_String:
                val = str2l(ent->f.val.s, &eos);
                if (*eos)
                    return deflt;
                return val;
            case VT_Integer:
            case VT_Byte:
            case VT_UByte:
            case VT_Short:
            case VT_UShort:
            case VT_UInt:
                return ent->f.val.i;
            case VT_Long:
            case VT_ULong:
                return (int)ent->f.val.l;
            default:
                return deflt;
            }
        }
        ent++;
    }
    return deflt;
}


/*
 * Find a boolean property
 */
static int getbooleanproperty(ism_protocol_action_t * action, const char * name) {
    int   i;
    ism_propent_t * ent = action->props;
    for (i=0; i<action->propcount; i++) {
        if (!strcmp(ent->name, name)) {
             switch (ent->f.type) {
             case VT_Boolean:
                 return ent->f.val.i;
             case VT_Integer:
             case VT_Byte:
             case VT_UByte:
             case VT_Short:
             case VT_UShort:
             case VT_UInt:
                 return !!ent->f.val.i;
             case VT_String:
                 return !strcmpi(ent->f.val.s, "true") || !strcmpi(ent->f.val.s, "on") ||
                        !strcmp(ent->f.val.s, "1") || !strcmpi(ent->f.val.s, "enabled");
             default:
                 break;
             }
        }
        ent++;
    }
    return 0;
}

/*
 * Get the messaging domain from the properties
 */
static int getDomain(ism_protocol_action_t * action) {
    int   i;
    ism_propent_t * ent = action->props;
    for (i=0; i<action->propcount; i++) {
        if (!strcmp(ent->name, "ObjectType")) {
             if (ent->f.type == VT_String) {
                 return strcmpi(ent->f.val.s, "topic") ? ismDESTINATION_QUEUE : ismDESTINATION_TOPIC;
             } else if (ent->f.type == VT_Integer) {
                 return (ent->f.val.i == ismDESTINATION_TOPIC) ? ismDESTINATION_TOPIC : ismDESTINATION_QUEUE;
             } else {
                 return ismDESTINATION_QUEUE;
             }
        }
        ent++;
    }
    return ismDESTINATION_QUEUE;
}

/*
 * Clear the undelivered messages in a consumer.
 */
static int clearConsumerUndeliveredMessage(ism_jms_session_t * session, ism_jms_prodcons_t * consumer,
		uint64_t minSqn, int onClose, ism_protocol_action_t * action, ismEngine_CompletionCallback_t pCallbackFn) {
    ism_undelivered_message_t * undelMsg;
    ismEngine_DeliveryHandle_t   array[1024];
    ismEngine_DeliveryHandle_t * msgs2free = array;
    int size = 1024;
    int counter = 0;
    int rc = ISMRC_OK;
    pthread_spin_lock(&session->lock);
    undelMsg = session->incompMsgHead;
    while (undelMsg) {
        ism_undelivered_message_t * msg = undelMsg;
        undelMsg = msg->next;
        if (msg->consumer != consumer)
            continue;
        if (msg->msgID <= minSqn)
            continue;
        /* If queue or durable */
        if ((onClose == 0) || ((consumer->domain == 1) ||(consumer->subName))) {
        	if(UNLIKELY(counter == size)) {
        		size = (size << 1);
        		if(array == msgs2free) {
        			msgs2free = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,184),sizeof(ismEngine_DeliveryHandle_t)*size);
        			memcpy(msgs2free,array,sizeof(array));
        		} else {
        			msgs2free = ism_common_realloc(ISM_MEM_PROBE(ism_memory_protocol_misc,185),msgs2free,(sizeof(ismEngine_DeliveryHandle_t)*size));
        		}
        	}
            msgs2free[counter++] = msg->deliveryHandle;
        }
        freeUndeliveredMessage(session,msg);
    }
    pthread_spin_unlock(&session->lock);
    if(counter) {
    	rc = ism_engine_confirmMessageDeliveryBatch(session->handle,NULL,counter,msgs2free,
    			ismENGINE_CONFIRM_OPTION_NOT_DELIVERED, action, action->actionsize, pCallbackFn);
    }
	if(array != msgs2free) {
		ism_common_free(ism_memory_protocol_misc,msgs2free);
	}
	return rc;
}

/*
 * Acknowledge delivered messages for a session.
 * This logic is also used in rollback and recover to mark any delivered messages to set the dup count.
 * @param session - pointer to a session
 * @param seqnum - sqn of the last message to acknowledge
 * */
static int ackDeliveredMessages(ism_transport_t * transport, ism_jms_session_t * session,
        uint32_t kind, uint64_t seqnum, uint64_t maxsqn, int acksqn_count, uint64_t * acksqn,
        ism_protocol_action_t * action, ismEngine_CompletionCallback_t pCallbackFn) {
    ism_undelivered_message_t * undelMsg;
    ism_undelivered_message_t * freeListHead = NULL;
    ism_undelivered_message_t * freeListTail = NULL;
    int freeListCount = 0;
    int rc = ISMRC_OK;
    int      i;

    if (maxsqn < seqnum)
        maxsqn = seqnum;

    /*
     * Prepare list of the message to acknowledge from the last  acknowledged one to the seqnum.
     */
    pthread_spin_lock(&session->lock);

    undelMsg = session->incompMsgHead;
#if DEBUG
    TRACEL(8, transport->trclevel, "ACK connect=%u kind=%d undelcount=%d seqnum=%ld maxsqn=%ld\n", transport->index,
            kind, session->incompMsgCount, seqnum, maxsqn);
    while (undelMsg) {
        ism_undelivered_message_t * msg = undelMsg;
        if (msg->msgID > maxsqn)
            break;
        TRACEL(8, transport->trclevel, "ACK check connect=%u undel=%lx seq=%ld consumer=%d\n", transport->index,
                (uint64_t)(uintptr_t)msg, msg->msgID, msg->consumer->which);
        undelMsg = msg->next;
    }
#endif

    undelMsg = session->incompMsgHead;
    while (undelMsg) {
        uint64_t chksqn = 0;
        ism_undelivered_message_t * msg = undelMsg;
        if (msg->msgID > maxsqn)
            break;

        undelMsg = msg->next;
        if (msg->consumer->msglistener) {
            chksqn = seqnum;
        } else {
            for (i = 0; i < acksqn_count; i+=2) {
                if (msg->consumer->which == ((int)acksqn[i])) {
                    chksqn = acksqn[i+1];
                    break;
                }
            }
        }
        if (msg->msgID <= chksqn) {
            freeListCount++;

            /* Move message from undel list to free list */
            if (!freeListHead)
                freeListHead = msg;
            if (freeListTail)
                freeListTail->next = msg;
            if (msg->prev == NULL)
                session->incompMsgHead = msg->next;
            else
                msg->prev->next = msg->next;
            if (msg->next == NULL)
                session->incompMsgTail = msg->prev;
            else
                msg->next->prev = msg->prev;
            freeListTail = msg;
            msg->next = NULL;
            msg->prev = NULL;    /* prev pointer not kept in free list */
        }
    }

    TRACEL(8,transport->trclevel, "ACK connect=%u kind=%d undelcount=%d freecount=%d seqnum=%ld maxsqn=%ld\n", transport->index,
            kind, session->incompMsgCount, freeListCount, seqnum, maxsqn);

    /*
     * If we have any to free do it unlocked
     */
    if (freeListCount) {
    	ismEngine_DeliveryHandle_t array[1024];
        ismEngine_DeliveryHandle_t * msgs2free;
        int counter = 0;
        ismEngine_TransactionHandle_t transaction = session->transaction;
        if(UNLIKELY(freeListCount > 1024)) {
        	msgs2free = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,187),sizeof(ismEngine_DeliveryHandle_t)*freeListCount);
        } else {
        	msgs2free = array;
        }
        if (kind != ismENGINE_CONFIRM_OPTION_CONSUMED)
            transaction = NULL;
        if (seqnum)
            session->lastAckSeqNum = seqnum;
        session->incompMsgCount -= freeListCount;
        pthread_spin_unlock(&session->lock);
        undelMsg = freeListHead;
        while (undelMsg) {
            /* Use delivery handle to confirm message delivery.*/
            if (undelMsg->deliveryHandle) {
                TRACEL(8, transport->trclevel, "ACK connect=%u msg=%lu consumer=%d handle=%lx\n",
                    transport->index, undelMsg->msgID, undelMsg->consumer->which,
                    (uint64_t)undelMsg->deliveryHandle);
                msgs2free[counter++] = undelMsg->deliveryHandle;
            }
            undelMsg->consumer->incompMsgCount--;
            undelMsg = undelMsg->next;
        }
        if (counter)
            rc = ism_engine_confirmMessageDeliveryBatch(session->handle,transaction,counter,
                    msgs2free, kind, action, ((action) ? action->actionsize : 0), pCallbackFn);
        if(msgs2free !=array)
        	ism_common_free(ism_memory_protocol_misc,msgs2free);
        pthread_spin_lock(&session->lock);
        freeListTail->next = session->freeMsgs;
        session->freeMsgs = freeListHead;
    }
    pthread_spin_unlock(&session->lock);
    return rc;
}

/*
 * Copy the referenced data inline into the action
 */
static void action2actionBuffInt(ism_protocol_action_t * action, const char * file, int line) {
    int i,len;
    char * data = (char *)(action->props + action->propcount);
//    dumpAction(action, " >> action2actionBuff", file, line);

    action->old_action = action;
    /* Copy value strings to the action structure */
    for (i=0; i<action->valcount; i++) {
        if (action->values[i].type == VT_String && action->values[i].val.s) {
            len = (int)strlen(action->values[i].val.s) + 1;
            memcpy(data, action->values[i].val.s, len);
            action->values[i].val.s = data;
            data += len;
            continue;
        }
        if (action->values[i].type == VT_ByteArray) {
            memcpy(data, action->values[i].val.s, action->values[i].len);
            action->values[i].val.s = data;
            data += action->values[i].len;
       }
    }

    /*
     * Copy property strings to the action structure
     */
    for (i=0; i<action->propcount; i++) {
        len = (int)strlen(action->props[i].name)+1;
        memcpy(data, action->props[i].name, len);
        action->props[i].name = data;
        data += len;
        if (action->props[i].f.type == VT_String && action->props[i].f.val.s) {
            len = (int)strlen(action->props[i].f.val.s) + 1;
            memcpy(data, action->props[i].f.val.s, len);
            action->props[i].f.val.s = data;
            data += len;
            continue;
        }
        if (action->props[i].f.type == VT_ByteArray) {
            memcpy(data, action->props[i].f.val.s, action->props[i].f.len);
            action->props[i].f.val.s = data;
            data += action->props[i].f.len;
        }
    }
//    dumpAction(action, " << action2actionBuff", file, line);
}


/*
 * Get the ack sqn array from the body
 */
static uint64_t* getAckSqn(jmsProtoObj_t * pobj, concat_alloc_t * map, uint32_t acksqn_count,
                           uint64_t * acksqn, uint32_t acksqn_alloc,uint64_t * pMaxSqn, int * rc) {
    int  i;
    ism_field_t f1,f2;
    uint64_t  maxSqn = 0;
    uint64_t * result = acksqn;

    if (__builtin_expect((acksqn_count == 0),0)){
        return result;
    }

    if (__builtin_expect((acksqn_count > (pobj->prodcons_alloc<<1)),0)){
        *rc = ISMRC_BadClientData;
        return NULL;
    }
    if (__builtin_expect((acksqn_count > acksqn_alloc),0)){
        result = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,189),sizeof(uint64_t)*acksqn_count);
        if (__builtin_expect((result == NULL),0)) {
            *rc = ISMRC_AllocateError;
            return NULL;
        }
    }
    for (i=0; i<acksqn_count; i++) {
        if (__builtin_expect((ism_protocol_getObjectValue(map, &f1) || ism_protocol_getObjectValue(map, &f2)), 0)) {
            *rc = ISMRC_BadClientData;
            if (result != acksqn) {
            	ism_common_free(ism_memory_protocol_misc,result);
            }
            return NULL;
        }
        acksqn[i++] = f1.val.i;
        acksqn[i] = f2.val.l;
        if (acksqn[i] > maxSqn)
            maxSqn = acksqn[i];
    }
    if (pMaxSqn)
        *pMaxSqn = maxSqn;
    return result;
}

#include "jmsdelay.c"

/*
 * Put reply functions in a separate source file
 */
#include "jmsreply.c"

/*
 * Put receive function in a separate source file
 */
#include "jmsreceive.c"



/*
 * Receive a connection closing notification for the JMS protocol.
 * We start the closing and it is completed in replyClosed().
 */
static int jmsClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    jmsProtoObj_t * pobj = (jmsProtoObj_t*)transport->pobj;
    ism_protocol_action_t act = {0};
    int32_t count;

    TRACEL(8, transport->trclevel, "jmsClosing connect=%u client=%s rc=%d clean=%d reason=%s\n",
            transport->index, transport->name, rc, clean, reason);
    if (pobj == NULL)/* Connection was broken during initConnection phase */
        return 0;

    /* Set the indicator that close is in progress. If set failed,
     * then this has been done before and we don't need to proceed. */
    if (!__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
        return 0;
    }

       /* Subtract the "in progress" indicator. If it becomes negative,
        * no actions are in progress, so it is safe to clean up protocol data
        * and close the connection. If it is non-negative, there are
        * actions in progress. The action that sets this value to 0
        * would re-invoke closing(). */
    count = __sync_sub_and_fetch(&pobj->inprogress, 1);
    if (count >= 0) {
        TRACEL(8, transport->trclevel, "jmsClosing postponed as there are %d actions/messages in progress: connect=%u client=%s\n",
                 count+1, transport->index, transport->name);
        return 0;
    }

    act.transport = transport;
    act.hdr.action = Action_closeConnection;   /* TODO: Consumer */
    act.valcount = 0;
    replyClosing(0, NULL, &act);
    return 0;
}



/*
 * Start message delivery for all subscribers  registered on connection.
 * @param transport  The transport object
 * @param userdata   The user object
 * @param flags      The option flags
 * @return A return code 0=good.
 */
static int resumeConnectionDelivery(ism_transport_t * transport, void * userdata, uint64_t flags) {
    int i;
    int mask = (userdata) ? (~SUSPENDED_BY_TRANSPORT) : (~SUSPENDED_BY_PROTOCOL);
    int rc = 0;
    int browsersOnly = 0;
    ism_jms_session_t * session;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    pthread_spin_lock(&(pobj->lock));
    if (!pobj->started) {
        if(!pobj->browser_session_handle) {
            pthread_spin_unlock(&(pobj->lock));
            if (__sync_sub_and_fetch(&pobj->inprogress, 1) == -1) { /* BEAM suppression: constant condition */
                ism_protocol_action_t act = { 0 };
                act.transport = transport;
                act.hdr.action = Action_closeConnection;
                replyClosing(0, NULL, &act);
            }
            return 0;
        }
        browsersOnly = 1;
    }
    TRACEL(7, transport->trclevel,"resumeConnectionDelivery. connect=%u userdata=%p inprogress=%d \n", transport->index, userdata, pobj->inprogress);

    pthread_spin_unlock(&(pobj->lock));
    if (userdata) {
        //Connection resumed by transport
        for (i = 0; i < pobj->prodcons_alloc; i++){
            ism_jms_prodcons_t * pc = pobj->prodcons[i];
            if (pc) {
                if(browsersOnly && (pc->kind != KIND_BROWSER))
                    continue;
                /* Extract current setting for SUSPENDED and turn off
                 * SUSPENDED_BY_TRANSPORT if it was set.
                 */
                int suspended = __sync_fetch_and_and(&pc->suspended, ~SUSPENDED_BY_TRANSPORT);
                if ((suspended & SUSPENDED_BY_TRANSPORT)
                    && !(suspended & SUSPENDED_BY_PROTOCOL))  {
                    if (pc->inBatch <= pc->cacheSize) {
                        /* If currently only SUSPENDED_BY_TRANSPORT, resume message
                         * delivery only if cache is not full.
                         */
                        resumeConsumerDelivery(transport,pc,1);
                    } else {
                        /* If currently only SUSPENDED_BY_TRANSPORT but cache is
                         * full, then set SUSPENDED_BY_PROTOCOL and do not attempt
                         * to resume.
                         */
                        TRACEL(8, transport->trclevel,"resumeConnectionDelivery setting SUSPENDED_BY_PROTOCOL. connect=%u consumer=%d inBatch=%d cacheSize=%d \n", transport->index, pc->which, pc->inBatch, pc->cacheSize);
                        __sync_or_and_fetch(&pc->suspended, SUSPENDED_BY_PROTOCOL);
                    }
                } else if (suspended & SUSPENDED_BY_PROTOCOL) {
                    TRACEL(4, transport->trclevel,"resumeConnectionDelivery UNEXPECTED - SUSPENDED_BY_TRANSPORT not set. connect=%u consumer=%d inBatch=%d \n", transport->index, pc->which, pc->inBatch);
                    /* If currently SUSPENDED_BY_PROTOCOL, attempt to resume. */
                    resumeConsumerDelivery(transport,pc,1);
                }
            }
        }
    } else {
        for (i = 0; i < pobj->sessions_alloc; i++) {
            pthread_spin_lock(&pobj->sessionlock);
            session = getSession(transport, i);
            if (session && session->handle) {
                if ((__sync_and_and_fetch(&session->suspended, mask) == 0) && session->handle) {
                    rc |= ism_engine_startMessageDelivery(session->handle,
                            ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
                }
            }
            pthread_spin_unlock(&pobj->sessionlock);
        }
    }
    if (__sync_sub_and_fetch(&pobj->inprogress, 1) == -1) { /* BEAM suppression: constant condition */
        ism_protocol_action_t act = { 0 };
        act.transport = transport;
        act.hdr.action = Action_closeConsumer;
        replyClosing(0, NULL, &act);
    }

    return 0;
}

/*
 * Start message delivery for all subscribers  registered on session.
 * @param transport  The transport object
 * @param userdata   The user object
 * @param flags      The option flags
 * @return A return code 0=good.
 */
static int resumeSessionDelivery(ism_transport_t * transport, void * userdata, int flags) {
    int rc = 0;
    ism_jms_session_t * session;
    uint64_t sessID = (uintptr_t) userdata;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    pthread_spin_lock(&pobj->sessionlock);
    session = getSession(transport, sessID);
    if (session && session->handle) {
        TRACEL(8, transport->trclevel,"Test resume session: connect=%u session=%d suspended=%d\n", transport->index, session->which, session->suspended);
        if ((__sync_and_and_fetch(&session->suspended, (~SUSPENDED_BY_PROTOCOL)) == 0) && session->handle) {

            rc = ism_engine_startMessageDelivery(session->handle,
                ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
            TRACEL(8, transport->trclevel, "Start message delivery for a session. connect=%u session=%d(%p) rc=%d \n",
                transport->index, session->which, session->handle, rc);
         }
    }
#ifdef DEBUG
    else {
        TRACEL(8, transport->trclevel, "resumeSessionDelivery - session is closed: connect=%u session=%d(%p) inprogress=%d\n",
                transport->index, (int)sessID, session, pobj->inprogress);
    }
#endif
    pthread_spin_unlock(&pobj->sessionlock);

    if (__sync_sub_and_fetch(&pobj->inprogress, 1) == -1) { /* BEAM suppression: constant condition */
        ism_protocol_action_t act = { 0 };
        act.transport = transport;
        act.hdr.action = Action_closeConnection;
        replyClosing(0, NULL, &act);
    }
    return 0;
}

/*
 * Start message delivery for consumer.
 * @param transport  The transport object
 * @param userdata   The user object
 * @param flags      The option flags
 * @return A return code 0=good.
 */
static int resumeConsumerDelivery(ism_transport_t * transport, void * userdata, uint64_t flags) {
    int rc = 0;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    ism_jms_prodcons_t * consumer = (flags) ? (ism_jms_prodcons_t *) userdata : getProdcons(transport, ((int)((uintptr_t)userdata)));
#ifdef DEBUG
    if(consumer) {
        TRACEL(8, transport->trclevel, "---------resumeConsumerDelivery connect=%u flags=%d userdata=%p consumer=%d(%p %p) inprogress=%d\n",
            transport->index, (int)flags, userdata, consumer->which, consumer, consumer->handle, pobj->inprogress);
    } else {
        TRACEL(8, transport->trclevel, "---------resumeConsumerDelivery connect=%u consumer=NULL userdata=%d inprogress=%d\n",
            transport->index, ((int)((uintptr_t)userdata)), pobj->inprogress);
    }
#endif
    if (consumer) {
        pthread_spin_lock(&consumer->lock);
        if (consumer->handle) {
            if (__sync_and_and_fetch(&consumer->suspended, ~SUSPENDED_BY_PROTOCOL) == 0) {
                rc = ism_engine_resumeMessageDelivery(consumer->handle, 0, NULL, 0, NULL);
                TRACEL(9, transport->trclevel, "Start message delivery for consumer. connect=%u consumer=%d(%p) inBatch=%d rc=%d\n",
                    transport->index, consumer->which, consumer->handle, consumer->inBatch, rc);

                /* Always return 0.  We do not want automatic retries of this action even
                 * if the rc indicates a failure.
                 */
                rc = 0;
            }
        } else {
            TRACEL(6, transport->trclevel,
                    "Could not start message delivery for consumer (consumer handle is null). connect=%u consumer=%d(%p)\n",
                     transport->index, consumer->which, consumer);
        }
        pthread_spin_unlock(&consumer->lock);
    } else {
        if(!flags) {
            TRACEL(6, transport->trclevel,
                    "Could not start message delivery for consumer (consumer not found). connect=%u consumer=%d\n",
                     transport->index, consumer->which);
        }
    }

    if (flags == 0) {
        int inprogress = __sync_sub_and_fetch(&pobj->inprogress, 1);
        if (inprogress < 0) { /* BEAM suppression: constant condition */
            if(inprogress == -1) {
                ism_protocol_action_t act = { 0 };
                act.transport = transport;
                act.hdr.action = Action_closeConnection;
                replyClosing(0, NULL, &act);
            }
            return 0;
        }

        /* TODO: Consider removing this unless we decide not to force rc=0. */
        if (rc) {
      	    __sync_add_and_fetch(&pobj->inprogress, 1);
        }
    }
    return rc;
}

/*
 * Resume message delivery for the JMS session/sessions for the transport.
 *
 * @param transport  The transport object
 * @param userdata   The data context for the work item
 * @return A return code: 0=good
 */
static int jmsConnectionResume(ism_transport_t * transport, void * userdata) {
    if (transport->addwork) {
        jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
        if (__sync_add_and_fetch(&pobj->inprogress, 1) < 1) {
        	__sync_sub_and_fetch(&pobj->inprogress, 1);
            return ISMRC_Closed;
        }
        return resumeConnectionDelivery(transport,userdata,0);
//        return transport->addwork(transport, resumeConnectionDelivery, userdata);
    } else {
        return -1;
    }
}


/*
 * Create a consumer for a durable subscription.
 * This function is invoked either asynchronously by the engine or synchronously by jmsReceive
 * once the durable subscription is created.
 */
static void createDurableConsumer(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    ism_jms_session_t * session = action->session;
    const char * subName;
    ism_jms_prodcons_t * consumer;
    ismEngine_ConsumerHandle_t consumerh;
    int id = 0;
    int mode;
    int noack;
    int consOpt = ismENGINE_CONSUMER_OPTION_NONE;
    void * clientState = pobj->handle;

    resetAction(action);
    subName = (action->valcount > 0) ? action->values[0].val.s : NULL;

    if (!rc && !subName) {
        rc = ISMRC_NameNotValid;
    }

    if (rc) {
        ism_common_setError(rc);
        replyCreateConsumer(rc, NULL, action);
        return;
    }
    if (action->noConsumer) {
        replyAction(0, NULL, action);
        return;
    }
    switch (action->shared) {
    case SHARED_False:
    case SHARED_True:         SHARED_SUB_NAME_DURABLE(subName);      break;
    case SHARED_NonDurable:   SHARED_SUB_NAME_NONDURABLE(subName);   break;
    case SHARED_Global:       clientState = client_Shared;           break;
    case SHARED_GlobalND:     clientState = client_SharedND;         break;
    }

    /* Check if noack is requested */
    noack = getbooleanproperty(action, "DisableACK");

    consumer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,191),1, sizeof(ism_jms_prodcons_t));
    if (consumer) {
        int orderIdRequested = getbooleanproperty(action, "RequestOrderID");
        consumer->domain = action->values[0].val.i;
        consumer->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000), getproperty(action, "Name"));
        consumer->kind = KIND_CONSUMER;
        consumer->subName = (char *)ism_transport_putString(transport, subName); /* Keep for life of transport */
        TRACEL(8, transport->trclevel, "createDurableConsumer connect=%u client=%s rc=%d topic=%s durable=%s\n",
                  transport->index, transport->name, rc, consumer->name, subName);
        id = setProdcons(transport, consumer);
        consumer->which = id;
        consumer->noack = noack;
        consumer->rule = action->rule;
        consumer->transport = transport;
        consumer->session = session;
        consumer->handle = NULL;
        consumer->shared = action->shared;
        consumer->orderIdRequested = orderIdRequested;
        consumer->cacheSize = getintproperty(action, "ClientMessageCache", -1);
        if (consumer->cacheSize > 0) {
            consumer->cacheSize = 1 + consumer->cacheSize/3;
        } else {
            if (consumer->cacheSize == -1)          /* For ismc */
                consumer->cacheSize = INT_MAX;
        }
        pthread_spin_init(&consumer->lock,0);
        memcpy(&consumer->hdr, &action->hdr, sizeof(struct actionhdr));
        consumer->hdr.item = session->which;
        consumer->hdr.itemtype = ITEMT_Consumer;
        action->prodcons = consumer;
    }

    /*
     * If we cannot set the consumer, report an error and free up memory
     */
    if (id <= 0) {
        if (id == 0) {
            ism_common_setError(ISMRC_AllocateError);
            replyCreateConsumer(ISMRC_AllocateError, NULL, action);
        } else {
            ism_common_setError(ISMRC_TooManyProdCons);
            replyCreateConsumer(ISMRC_TooManyProdCons, NULL, action);
        }
        return;
    }

    /*

     */
    if (!consumer->noack && session->ackmode) {
        mode = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    } else {
        mode = ismENGINE_SUBSCRIPTION_OPTION_NONE;
    }
    if (action->shared != SHARED_NonDurable && action->shared != SHARED_GlobalND) {
        mode |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
    }
    if ((!consumer->noack && session->ackmode) || session->transacted) {
        consOpt |= ismENGINE_CONSUMER_OPTION_ACK;
    }

    if (action->nolocal) {
        mode |= ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL;
    }

    mode |= ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;

    if ((pobj->started) && transport->addwork) {
        consOpt |= ismENGINE_CONSUMER_OPTION_PAUSE;
        consumer->suspended = SUSPENDED_BY_PROTOCOL;
    }

    rc = ism_engine_createConsumer(session->handle, ismDESTINATION_SUBSCRIPTION,
            subName, 0, clientState,
            consumer, sizeof(*consumer), replyReceive,
            NULL, consOpt, &consumerh,
            action, sizeof(*action), replyCreateConsumer);

    if (rc != ISMRC_AsyncCompletion) {
        replyCreateConsumer(rc, consumerh, action);
    }
}

/*
 * Create a consumer for a durable subscription - called
 * asynchronously by ism_engine_listDurableSubscriptions
 */
static void jmsReSubscribe(ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName, const char * oldTopicName,
        void * xproperties, size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes, uint32_t consumerCount, void * vaction) {

    ism_prop_t * props = (ism_prop_t *)xproperties;
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    const char * subName;
    const char * newTopicName;
    xUNUSED ism_jms_prodcons_t * consumer;
    ism_field_t newSelector;
    ism_field_t oldSelector;
    int   newNoLocal = action->nolocal;
    int   oldNoLocal = false;
    int   oldShared  = 0;
    int   selectorMismatch;

    resetAction(action);
    subName = (action->valcount > 0) ? action->values[0].val.s : NULL;
    newTopicName = getproperty(action, "Name");

    action->recordCount = 2;

    newSelector.type = VT_Null;
    newSelector.val.s = NULL;
    newSelector.len = 0;
    if (action->rule) {
        newSelector.type = VT_ByteArray;
        newSelector.len = action->rulelen;
        newSelector.val.s = (char *)action->rule;
    }
    if (newSelector.type != VT_ByteArray || newSelector.len == 0)
        newSelector.type = VT_Null;

    /*
     * Extract subscription properties:
     * NULL-terminated selector    (NULL, if absent)
     * 1-byte nolocal indicator
     */
    if (props) {
        oldNoLocal = ism_common_getBooleanProperty(props, "NoLocal", false);
        ism_common_getProperty(props, "Selector", &oldSelector);
    } else {
        oldSelector.type = VT_Null;
    }

    if (subName) {
    	switch (action->shared) {
    	case SHARED_False:
    	case SHARED_True:         SHARED_SUB_NAME_DURABLE(subName);      break;
    	case SHARED_NonDurable:   SHARED_SUB_NAME_NONDURABLE(subName);   break;
    	default:                                                         break;
    	}
    }

    TRACEL(8, transport->trclevel, "jmsReSubscribe connect=%u client=%s subscription=%s durable=%s shared=%d old_topic=%s new_topic=%s\n",
                transport->index, transport->name, pSubName, subName, action->shared, oldTopicName, newTopicName);

    if (subName && !strcmp(subName, pSubName)) {
        /*
         * Subscription name matches. If consumer topic and/or selector are different,
         * destroy the consumer and subscription, then create subscription and consumer from scratch.
         * Otherwise, create a new consumer for existing subscription.
         */
        action->subscriptionFound = SUB_Found;
        oldShared = !!(pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED);

        selectorMismatch = (oldSelector.type != newSelector.type) ||
                           (oldSelector.type == VT_ByteArray &&
                           (oldSelector.len != newSelector.len || memcmp(oldSelector.val.s, newSelector.val.s, oldSelector.len)));
        if (strcmp(oldTopicName, newTopicName) || newNoLocal != oldNoLocal || selectorMismatch) {
            /*
             * If there are active consumers, it is not valid to modify the subscriptions
             */
            if (consumerCount > 0) {
                action->rc = oldShared ? ISMRC_ExistingSubscription : ISMRC_DestinationInUse;
                ism_common_setError(action->rc);
                action->subscriptionFound = SUB_Error;
                return;
            }

            /*
             * If the share setting does not match between the new and existing subscription, we will
             * throw an exception at the client.
             */
            if (action->shared == SHARED_NonDurable || action->shared == SHARED_GlobalND) {
                action->rc = ISMRC_ShareMismatch;
                ism_common_setError(action->rc);
                action->subscriptionFound = SUB_Error;
                return;
            }
            if (oldShared != !!action->shared) {
                action->rc = ISMRC_ShareMismatch;
                ism_common_setError(action->rc);
                action->subscriptionFound = SUB_Error;
                return;
            }

            /*
             * Destroy subscription, then create a new one and a durable consumer
             * */
            TRACEL(6, transport->trclevel, "jmsReSubscribe with different parameters: connect=%u client=%s subscription=%s"
                " oldTopic=%s newTopic=%s oldNoLocal=%d newNoLocal=%d selectorMismatch=%d\n",
                transport->index, transport->name, subName,
                oldTopicName, newTopicName, oldNoLocal, newNoLocal, selectorMismatch);
            action->recordCount = 3;
            recreateConsumerAndSubscription(0, NULL, action);
        } else {
            if (oldShared == 0 && consumerCount > 0) {
                action->rc = ISMRC_DestinationInUse;
                ism_common_setError(action->rc);
                action->subscriptionFound = SUB_Error;
                return;
            }
            if (oldShared != !!action->shared) {
                action->rc = ISMRC_ShareMismatch;
                ism_common_setError(action->rc);
                action->subscriptionFound = SUB_Error;
                return;
            }

            if (action->shared) {
                ismEngine_ClientStateHandle_t clientState;
                switch (action->shared) {
                case SHARED_Global:       clientState = client_Shared;          break;
                case SHARED_GlobalND:     clientState = client_SharedND;        break;
                default:                  clientState = transport->pobj->handle;   break;
                }
                action->rc = ism_engine_reuseSubscription(transport->pobj->handle, subName, pSubAttributes, clientState);
                if(action->rc != ISMRC_OK) {
                    ism_common_setError(action->rc);
                    action->subscriptionFound = SUB_Error;
                    return;
                }
            }

            /* Re-create the durable consumer */
            action->recordCount = 5;
            recreateConsumerAndSubscription(0, NULL, action);
        }
    }
}

/*
 * Free consumer object. Can be called asynchronously from
 * engine callback.
 */
static void freeConsumer(ism_jms_prodcons_t * consumer) {
    if (consumer->name)
        ism_common_free(ism_memory_protocol_misc,consumer->name);
    if (consumer->rule)
    	//The rule is found via ism_common_compileSelectRule so is created by utils
        ism_common_free(ism_memory_utils_misc,consumer->rule);
    pthread_spin_destroy(&consumer->lock);
    ism_common_free(ism_memory_protocol_misc,consumer);
}


/*
 * The callback for closing the consumer when no reply is necessary
 * (connection is being closed).
 */
static int jmsCloseConsumer(ism_protocol_action_t * action) {
    ism_transport_t * transport = action->transport;
    ism_jms_prodcons_t * consumer = action->prodcons;
    int32_t rc= ISMRC_OK;

    if (consumer) {
        /* Set the indicator that close is in progress. If set failed,
         * then this has been done before and we don't need to proceed. */
        if (!__sync_bool_compare_and_swap(&consumer->closing, 0, 1)) {
            return ISMRC_OK;
        }

        TRACEL(7, transport->trclevel, "Close JMS consumer connect=%u client=%s consumer=%d(%p) name=%s domain=%s\n",
                transport->index, transport->name, consumer->which, consumer->handle, consumer->name, domainName(consumer->domain));

        pthread_spin_lock(&consumer->lock);
        if (consumer->handle) {
        	void * handle = consumer->handle;
			consumer->handle = NULL;
			rc = ism_engine_destroyConsumer(handle, action, sizeof(ism_protocol_action_t), cleanupConsumer);
        }
        pthread_spin_unlock(&consumer->lock);
        if (rc != ISMRC_AsyncCompletion) {
            cleanupConsumer(rc, (void*)-1, action);
        }
    }
    return rc;
}

/*
 * Free the consumer object at close time.
 */
static void cleanupConsumer(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_jms_session_t * session = action->session;
    ism_jms_prodcons_t * consumer = action->prodcons;
    ism_transport_t * transport = action->transport;
    removeProdcons(transport, consumer->which, 1);
    if (rc == ISMRC_OK) {
        if(clearConsumerUndeliveredMessage(session, consumer, 0, 1, action, cleanupConsumer) == ISMRC_AsyncCompletion)
        	return;
        if (consumer->shared == SHARED_GlobalND) {
            consumer->shared = 0;
            if (consumer->subName) {
                checkSubscriptionConsumer(consumer->subName, transport_SharedND);
            }
        }
    } else {
        ism_common_setError(rc);
    }
    freeConsumer(consumer);
    if (!handle) {
    	replyClosing(rc, NULL, vaction);
    }
}

/*
 * Remove all pending unACKed messages. This method should be called
 * only when closing the session.
 *
 * @param map  A pointer to the hashmap containing structures that
 * describe unacked messages.
 */
static int clearUndeliveredMessages(ism_protocol_action_t * action, ismEngine_CompletionCallback_t  pCallbackFn) {
	ism_jms_session_t * session = action->session;
    ismEngine_DeliveryHandle_t array[1024];
    ismEngine_DeliveryHandle_t * msgs2free = array;
    int counter = 0;
    int size = 1024;
    int rc = ISMRC_OK;
    ism_undelivered_message_t * undelMsg;
    if(session) {
        pthread_spin_lock(&session->lock);
        undelMsg = session->incompMsgHead;
        while (undelMsg) {
            ism_undelivered_message_t * msg = undelMsg;
            msg->consumer->incompMsgCount--;
            undelMsg = msg->next;
            if (session->handle && msg->deliveryHandle &&
                (msg->subName || (msg->consumer->domain == 1))) {
            	if(UNLIKELY(counter == size)) {
            		size = (size << 1);
            		if( msgs2free == array) {
            			msgs2free = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,195),sizeof(ismEngine_DeliveryHandle_t)*size);
            			memcpy(msgs2free,array,sizeof(array));
            		} else {
            			msgs2free = ism_common_realloc(ISM_MEM_PROBE(ism_memory_protocol_misc,196),msgs2free,(sizeof(ismEngine_DeliveryHandle_t)*size));
            		}
            	}
            	msgs2free[counter++] = msg->deliveryHandle;
            }
            ism_common_free(ism_memory_protocol_misc,msg);
        }
        session->incompMsgHead = NULL;
        session->incompMsgTail = NULL;
        session->incompMsgCount = 0;
        pthread_spin_unlock(&session->lock);
        if (counter)
            rc = ism_engine_confirmMessageDeliveryBatch(session->handle, NULL, counter, msgs2free,
            		ismENGINE_CONFIRM_OPTION_NOT_DELIVERED, action, action->actionsize, pCallbackFn);
        if(msgs2free != array)
        	ism_common_free(ism_memory_protocol_misc,msgs2free);
    }
    return rc;
}

/*
 * Free up the list of all pending unACKed messages. This method should be called
 * only after closing the connection.
 */
static void freeUndeliveredMessages(ism_jms_session_t * session) {
    ism_undelivered_message_t * undelMsg = session->incompMsgHead;
    while (undelMsg) {
        ism_undelivered_message_t * msg = undelMsg;
        undelMsg = msg->next;
        ism_common_free(ism_memory_protocol_misc,msg);
    }
    session->incompMsgHead = NULL;
    session->incompMsgTail = NULL;
    session->incompMsgCount = 0;
}

/*
 * Do a delayed subscribe
 */
static int doSubscribe(ism_transport_t * clientTrans, void * vaction, int flags) {
    ism_protocol_action_t * action = vaction;
    int  rc;

    /*
     * Only one subscribe at a time can be done on a connection
     */
    if (__sync_bool_compare_and_swap(&action->clientTrans->pobj->subscribeLock, 0, 1)) {
        ism_transport_t * transport = action->transport;
        void * clientState = transport->pobj->handle;

        const char * subName;
        resetAction(action);
        subName = action->values[0].val.s;
        switch (action->shared) {
        case SHARED_False:
        case SHARED_True:         SHARED_SUB_NAME_DURABLE(subName);     break;
        case SHARED_NonDurable:   SHARED_SUB_NAME_NONDURABLE(subName);  break;
        case SHARED_Global:       clientState = client_Shared;          break;
        case SHARED_GlobalND:     clientState = client_SharedND;        break;
        }

        /* Only a single subscription with this name can be found */
        action->subscriptionFound = SUB_NotFound;
        rc = ism_engine_listSubscriptions(clientState, (char *)subName, action, jmsReSubscribe);
        if (rc) {
            replyAction(rc, NULL, action);
        } else {
            if (action->subscriptionFound == SUB_NotFound) {
                /* Create subscription and consumer */
                action->recordCount = RESUB_CreateSubscription;
                recreateConsumerAndSubscription(0, NULL, action);
            } else if (action->subscriptionFound == SUB_Error) {
                replyAction(action->rc, NULL, action);
            }
        }
        ism_common_free(ism_memory_protocol_misc,action);
        return 0;
    } else {
        return 1;
    }
}

/*
 * Do a delayed subscribe
 */
static int doUnsubscribe(ism_transport_t * clientTrans, void * vaction, int flags) {
    ism_protocol_action_t * action = vaction;
    int   rc;

    /*
     * Only one subscribe at a time can be done on a connection
     */
    if (__sync_bool_compare_and_swap(&action->clientTrans->pobj->subscribeLock, 0, 1)) {
        ism_transport_t * transport = action->transport;
        void * clientState = transport->pobj->handle;
        const char * subName;

        resetAction(action);
        subName = action->values[0].val.s;
        switch (action->shared) {
        case SHARED_False:
        case SHARED_True:         SHARED_SUB_NAME_DURABLE(subName);     break;
        case SHARED_NonDurable:   SHARED_SUB_NAME_NONDURABLE(subName);  break;
        case SHARED_Global:       clientState = client_Shared;          break;
        case SHARED_GlobalND:     clientState = client_SharedND;        break;
        }
        rc = ism_engine_destroySubscription(transport->pobj->handle, subName, clientState, action, action->actionsize, replyAction);
        if (rc != ISMRC_AsyncCompletion) {
            if (rc) {
                ism_common_setError(rc);
            }
            replyAction(rc, NULL, action);
        }
        ism_common_free(ism_memory_protocol_misc,action);
        return 0;
    } else {
        return 1;
    }
}

/*
 * Do a delayed update
 */
static int doUpdate(ism_transport_t * clientTrans, void * vaction, int flags) {
	ism_protocol_action_t * action = vaction;
	int   rc;

	/*
	 * Only one subscribe at a time can be done on a connection
	 */
	if (__sync_bool_compare_and_swap(&action->clientTrans->pobj->subscribeLock, 0, 1)) {
		ism_transport_t * transport = action->transport;
		void * clientState = transport->pobj->handle;
		const char * clientName = transport->name;
		ism_prop_t * props = ism_common_newProperties(20);
		int i;
		const char * subName;

		resetAction(action);
		subName = action->values[0].val.s;

		/* Handle shared subscriptions */
    	if (transport->pobj->isGenerated) {
    		clientName = "__Shared";
    	}

		switch (action->shared) {
		case SHARED_False:
		case SHARED_True:         SHARED_SUB_NAME_DURABLE(subName);     break;
		case SHARED_NonDurable:   SHARED_SUB_NAME_NONDURABLE(subName);  break;
		case SHARED_Global:       clientState = client_Shared;          break;
		case SHARED_GlobalND:     clientState = client_SharedND;        break;
		}

    	/* Copy valid properties into props array (they are already verified). */
    	for (i = 0; i < action->propcount; i++) {
    		ism_common_setProperty(props, action->props[i].name, &action->props[i].f);
    	}

    	/* Finally, update the subscription */
		rc = ism_engine_updateSubscription(transport->pobj->handle,
				subName,
				props,
				clientState,
				action,
				action->actionsize,
				replyAction);
		ism_common_freeProperties(props);
		if (rc != ISMRC_AsyncCompletion) {
			if (rc) {
				TRACEL(4, transport->trclevel, "Unable to update: client=%s name=%s rc=%d\n", clientName, subName, rc);
				ism_common_setError(rc);
			}
			replyAction(rc, NULL, action);
		}
		ism_common_free(ism_memory_protocol_misc,action);
		return 0;
	} else {
		return 1;
	}
}


/*
 * Destroy and re-create durable consumer and subscription.
 * Called in stages, but starting stage can be selected (action->recordCount).
 *
 * 1. Destroy consumer (stop if consumer is NULL, closed or close in progress).
 * 2. Free consumer object (stop here).
 * 3. Destroy subscription.
 * 4. Create subscription.
 * 5. Create consumer.
 */
static void recreateConsumerAndSubscription(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ism_transport_t * transport = action->transport;
    xUNUSED ism_jms_prodcons_t * consumer = action->prodcons;
    ism_jms_session_t * session = action->session;
    jmsProtoObj_t  * pobj = (jmsProtoObj_t*)transport->pobj;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    const char * name;
    const char * subName;
    int maxMessages = 0;
    ism_prop_t * cprops = NULL;
    void * clientState = pobj->handle;

    if (rc) {
        ism_common_setError(rc);
        replyAction(rc, NULL, action);
        return;
    }

    TRACEL(9, transport->trclevel, "recreateConsumerAndSubscription connect=%u client=%s stage=%lu\n",
                    transport->index, transport->name, action->recordCount);

    resetAction(action);
    switch (action->recordCount) {
    case RESUB_DestroySubscription:
        /*
         * Destroy subscription.
         */
        subName = action->values[0].val.s;
        switch (action->shared) {
        case SHARED_False:
        case SHARED_True:         SHARED_SUB_NAME_DURABLE(subName);      break;
        case SHARED_NonDurable:   SHARED_SUB_NAME_NONDURABLE(subName);   break;
        case SHARED_Global:       clientState = client_Shared;           break;
        case SHARED_GlobalND:     clientState = client_SharedND;         break;
        }

        TRACEL(7, transport->trclevel, "Destroy JMS durable subscription: connect=%u client=%s durable=%s\n",
                transport->index, transport->name, subName);

        action->recordCount = 4;
        rc = ism_engine_destroySubscription(
                pobj->handle, subName, clientState, action,
                action->actionsize, recreateConsumerAndSubscription);
        if (rc != ISMRC_AsyncCompletion) {
            recreateConsumerAndSubscription(rc, NULL, action);
        }

        break;

    case RESUB_CreateSubscription:
        /*
         * Create subscription
         */
        subName = action->values[0].val.s;
        switch (action->shared) {
        case SHARED_False:
        case SHARED_True:         SHARED_SUB_NAME_DURABLE(subName);      break;
        case SHARED_NonDurable:   SHARED_SUB_NAME_NONDURABLE(subName);   break;
        case SHARED_Global:       clientState = client_Shared;           break;
        case SHARED_GlobalND:     clientState = client_SharedND;         break;
        }
        name = getproperty(action, "Name");

        if (session->ackmode) {
            subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
        } else {
            subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_NONE;
        }
        if (action->shared != SHARED_NonDurable && action->shared != SHARED_GlobalND)
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;

        /*
         * Construct a properties object if required
         */
        if (action->rule || action->nolocal) {
            ism_field_t field;
            cprops = ism_common_newProperties(20);
            if (action->rule) {
                field.type  = VT_ByteArray;
                field.val.s = (char *)action->rule;
                field.len   = action->rulelen;
                ism_common_setProperty(cprops, "Selector", &field);
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION;
            }
            if (action->nolocal) {
                field.type = VT_Boolean;
                field.val.i = 1;
                ism_common_setProperty(cprops, "NoLocal", &field);
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL;
            }
        }

        /* Set the shared and durable option bits */
        switch (action->shared) {
        case SHARED_False:        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;                                          break;
        case SHARED_True:
        case SHARED_Global:       subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_SHARED | ismENGINE_SUBSCRIPTION_OPTION_DURABLE;   break;
        case SHARED_NonDurable:
        case SHARED_GlobalND:     subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_SHARED;                                           break;
        }


        /*
         * If new value for MaxMessages (maximum number of messages for subscription)
         * is specified, set it in ism_engine_createSubscription call.
         */
        if (action->valcount > 2 && isInternal()) {
        	maxMessages = action->values[2].val.i;
            if (maxMessages > 0) {
                ism_field_t field;
                if (!cprops) {
                    cprops = ism_common_newProperties(1);
                }
                field.type  = VT_Integer;
                field.val.i = maxMessages;
                ism_common_setProperty(cprops, "MaxMessages", &field);

                TRACEL(7, transport->trclevel, "New max messages=%d connect=%u client=%s rc=%d topic=%s durable=%s\n",
                          maxMessages, transport->index, transport->name, rc, name, subName);
            }
        }

        /*
         * Create the engine object
         */
        action->recordCount = RESUB_CreateConsumer;
        action->subscriptionFound = SUB_Resubscribe;
        rc = ism_engine_createSubscription(pobj->handle, subName, cprops, ismDESTINATION_TOPIC,
                name, &subAttrs, clientState, action, action->actionsize, recreateConsumerAndSubscription);

        if (cprops) {
            ism_common_freeProperties(cprops);
        }
        if (rc != ISMRC_AsyncCompletion) {
            recreateConsumerAndSubscription(rc, NULL, action);
        }

        break;

    case RESUB_CreateConsumer:
        /* Create durable consumer */
        createDurableConsumer(rc, NULL, action);
        break;
    }
}

/*
 * Re-create local transaction handle and pass the action through to replyAction.
 * This function is called from replyCommitSession and replyRollbackSession.
 */
static void createTransaction(int32_t rc, void * handle, void * vaction) {
    ism_protocol_action_t * action = vaction;
    ismEngine_TransactionHandle_t transHandle = NULL;

    if (action->session->transacted == JMS_LOCAL_TRANSACTION) {
        rc = ism_engine_createLocalTransaction(action->session->handle,
                &transHandle, action, sizeof(ism_protocol_action_t), replyAction);
    }

    if (rc != ISMRC_AsyncCompletion)
        replyAction(rc, transHandle, action);
}

/*
 * Add subscription information to the buffer:
 * Nolocal indicator (boolean)
 * Selector (string, can be NULL)
 * Subscription name (string)
 * Topic name (string)
 */
static void addSubscription(ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName, const char * pTopicString,
        void * properties, size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes, uint32_t consumerCount, void * vaction) {
    ism_protocol_action_t * action = vaction;
    const char * selector = NULL;
    bool noLocal = false;

    /*
     * Extract subscription properties:
     */
    if (properties) {
        noLocal = ism_common_getBooleanProperty((ism_prop_t *)properties, ismENGINE_PROPERTY_NOLOCAL, false);
        selector = ism_common_getStringProperty((ism_prop_t *)properties, ismENGINE_PROPERTY_SELECTOR);
    }

    action->recordCount++;

    ism_protocol_putBooleanValue(&action->xbuf, noLocal);
    ism_protocol_putStringValue(&action->xbuf, selector);
    ism_protocol_putStringValue(&action->xbuf, pSubName);
    ism_protocol_putStringValue(&action->xbuf, pTopicString);
}

/*
 * Check if this message code was previously logged.
 * This is used to support only logging a condition once within a connection.
 */
static int previouslyLogged(jmsProtoObj_t * pobj, int msgcode) {
    void * alreadylogged = NULL;
    pthread_spin_lock(&(pobj->lock));
    if (!pobj->errors) {
        pobj->errors = ism_common_createHashMap(5,HASH_INT32);
    }
    ism_common_putHashMapElement(pobj->errors, &msgcode, sizeof(msgcode), (void *)1, &alreadylogged);
    pthread_spin_unlock(&(pobj->lock));

    if (alreadylogged)
        return 1;
    return 0;
}


extern int ism_tcp_addWork(ism_transport_t * transport, ism_transport_onDelivery_t ondelivery, void * userdata);
extern int ism_protocol_JmsDelayInit(void);
/*
 * Start messaging
 */
static int jmsStartMessaging(void) {
    jmsProtoObj_t * pobj;

    TRACE(6, "JMS start messaging\n");

    client_Shared = ism_protocol_getSharedClient(1);
    client_SharedND = ism_protocol_getSharedClient(0);

    /*
     * Create transport objects to allow us to run on the delivery thread.
     * Use the tcp addwork method with the default endpoint
     */
    transport_Shared   = ism_transport_newTransport(NULL, 256, 0);
    transport_Shared->name = "__Shared";
    transport_Shared->addwork = ism_tcp_addWork;
    transport_Shared->pobj = pobj = (jmsProtoObj_t *)ism_transport_allocBytes(transport_Shared, sizeof(jmsProtoObj_t), 1);
    memset((char *)pobj, 0, sizeof(jmsProtoObj_t));
    transport_Shared->pobj->handle = client_Shared;
    pthread_spin_init(&pobj->lock, 0);
    pthread_spin_init(&pobj->sessionlock, 0);

    transport_SharedND = ism_transport_newTransport(NULL, 256, 0);
    transport_SharedND->name = "__SharedND";
    transport_SharedND->addwork = ism_tcp_addWork;
    transport_SharedND->pobj = pobj = (jmsProtoObj_t *)ism_transport_allocBytes(transport_SharedND, sizeof(jmsProtoObj_t), 1);
    memset((char *)pobj, 0, sizeof(jmsProtoObj_t));
    transport_SharedND->pobj->handle = client_SharedND;
    pthread_spin_init(&pobj->lock, 0);
    pthread_spin_init(&pobj->sessionlock, 0);

    ism_protocol_JmsDelayInit();
    return 0;
}


/*
 * Process a new JMS connection.
 *
 * In the JMS connection, it is the responsibility of the client to initiate the handshake.
 * Therefore after accepting a connection, we just wait for the first action to begin.
 *
 */
static int jmsConnection(ism_transport_t * transport) {
    if (!strcmpi(transport->protocol, "jms") || !strcmpi(transport->protocol, "tcpjms")) {
        jmsProtoObj_t * pobj = (jmsProtoObj_t *)ism_transport_allocBytes(transport,
                sizeof(jmsProtoObj_t), 1);
        transport->protocol = "jms";
        transport->protocol_family = "jms";
        memset((char *)pobj, 0, sizeof(jmsProtoObj_t));
        transport->pobj = pobj;
        transport->dumpPobj = jmsDumpPobj;
        pthread_spin_init(&pobj->lock, 0);
        pthread_spin_init(&pobj->sessionlock, 0);
        transport->receive = jmsReceive;
        transport->closing = jmsClosing;
        transport->resume  = jmsConnectionResume;
        transport->actionname = getActionName;
        transport->checkLiveness = jmsCheckLiveness;
        return 0;
    }
    return 1;
}

/*
 * Initialize the JMS protocol
 */
int ism_protocol_initJMS(void) {
    jms_unit_test = (getenv("CUNIT") != NULL);
    ism_transport_registerProtocol(jmsStartMessaging, jmsConnection);
    TRACE(7, "Initialize JMS\n");
    g_disable_auth = ism_common_getBooleanConfig("DisableAuthentication", 0);
    jmsMaxConsumers = ism_common_getIntConfig("JmsMaxConsumers", 1000);
    pthread_mutex_init(&jmslock, NULL);

    /* Set Capabilities in Admin
     * Capabilities: UseTopic, UseShared, UseQueue, UseBrowse
     */
    int capability =  0x01 | 0x02 | 0x04 | 0x08;
    ism_admin_updateProtocolCapabilities("jms", capability);


#ifdef PERF
    {
        const char *histUnitsCfg = ism_common_getStringConfig("ProtocolHistUnits");
        if (histUnitsCfg) {
            histUnits = atof(histUnitsCfg);
            if (histUnits < 1e-9)
                histUnits = 1e-9;
        }
        invHistUnits = 1 / histUnits;
        histSize = ism_common_getIntConfig("ProtocolHistSize", 10000);
        histSampleRate = ism_common_getIntConfig("ProtocolHistSampleRate", 0);
        if (histSampleRate) {
            /* Allocate stats objects */
            commitStat = (ism_latencystat_t*)ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,203),1 ,sizeof(ism_latencystat_t));
            commitStat->histogram = (uint32_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,204),
                        histSize, sizeof(uint32_t));
            commitStat->histSize = histSize;
            commitStat->histUnits = histUnits;
            msgProcStat = (ism_latencystat_t*)ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,205),1 ,sizeof(ism_latencystat_t));
            msgProcStat->histogram = (uint32_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,206),
                        histSize, sizeof(uint32_t));
            msgProcStat->histSize = histSize;
            msgProcStat->histUnits = histUnits;
        }
    }
#endif
    return 0;
}

/*
 * Print JMS statistics.
 * This is used only as part of the perf support
 */
int ism_protocol_printJMSStats(void) {
#ifdef PERF
    char buffer[4096];
    if (histSampleRate) {
        TRACE(5,"JMS Protocol layer stats: \n");
    }
    if (commitStat) {
        buffer[0] = 0;
        /* Report how long it takes to execute jobs */
        ism_common_printHistogramStats(commitStat, buffer,
                sizeof(buffer));
        TRACE(5,"\tcommit latency: %s\n",buffer);
    }
    if (msgProcStat) {
        buffer[0] = 0;
        /* Report how long it takes to execute jobs */
        ism_common_printHistogramStats(msgProcStat, buffer,
                sizeof(buffer));
        TRACE(5,"\tmessageProc latency: %s\n",buffer);
    }
#endif
    return 0;
}

/*
 * Terminate JMS processing
 */
int ism_protocol_termJMS(void) {
    ism_protocol_printJMSStats(); /* BEAM suppression: no effect */
#ifdef PERF
    if (commitStat) {
        if (commitStat->histogram)
            ism_common_free(ism_memory_protocol_misc,commitStat->histogram);
        ism_common_free(ism_memory_protocol_misc,commitStat);
    }
    if (msgProcStat) {
        if (msgProcStat->histogram)
            ism_common_free(ism_memory_protocol_misc,msgProcStat->histogram);
        ism_common_free(ism_memory_protocol_misc,msgProcStat);
    }
#endif
    return 0;
}


/*
 * Dump a JMS protocol specific objects
 */
static int jmsDumpPobj(ism_transport_t * transport, char * buf, int len) {
    jmsProtoObj_t * pobj = (jmsProtoObj_t*) transport->pobj;
    int n, i;
    if (!buf || len < 8)
        return pobj ? pobj->inprogress : 0;
    if (!pobj) {
        sprintf(buf, "(null)");
        return 0;
    }
    n = snprintf(buf, len - 1, "JMS pobj: started=%d closed=%d sessionlock=%d sessions_used=%d prodcons_used=%d consumers=%d inprogress=%d\n",
            (int) pobj->started, (int) pobj->closed, pobj->sessionlock, pobj->sessions_used, pobj->prodcons_used, pobj->consumer_count, pobj->inprogress);
    if (n >= len) {
        buf[len - 1] = '\0';
        return 0;
    }
    for (i = 0; i < pobj->sessions_alloc; i++) {
        ism_jms_session_t * session = pobj->sessions[i];
        if (session == NULL)
            continue;
        buf += n;
        len -= n;
        n = snprintf(buf, len, "Session %d: suspended=%d handle=%p ackmode=%d incompMsgCount=%d seqnum=%lu\n",
                session->which, session->suspended, session->handle, session->ackmode, session->incompMsgCount,
                session->seqnum);
        if (n >= len) {
            buf[len - 1] = 0;
            return 0;
        }
    }
    for (i = 0; i < pobj->prodcons_alloc; i++) {
        ism_jms_prodcons_t * pc = pobj->prodcons[i];
        if (pc == NULL)
            continue;
        buf += n;
        len -= n;
        n = snprintf(buf, len, "prodcons %d: name=%s, suspended=%d closing=%d handle=%p domain=%d incompMsgCount=%d noack=%d kind=%d durable=%s\n",
              pc->which, pc->name, pc->suspended, (int) pc->closing, pc->handle, (int) pc->domain,
              pc->incompMsgCount, (int) pc->noack, (int) pc->kind, ((pc->subName) ? pc->subName: "(null)"));
        if (n >= len) {
            buf[len - 1] = 0;
            return 0;
        }
    }
    return 0;
}

static char * showVersion(int versionx, char * vstring) {
    uint32_t version = versionx&0x1fffffff;
    sprintf(vstring,"%u.%u.%u.%u", (int)((version/1000000)%100), (int)((version/10000)%100), (int)((version/100)%100), (int)(version%100));
    return vstring;
}
