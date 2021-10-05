/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
#ifndef TRACE_COMP
#define TRACE_COMP Plugin
#endif
#include <ismutil.h>
#include <transport.h>
#define ACTION_NAMES
#define EXT_RC_STRINGS
#include "plugin.h"
#include <security.h>
#include <monitoring.h>
#include <admin.h>
#include "imacontent.h"
#include <assert.h>
#include <pthread.h>
#include <dirent.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>


static int registerWork(ism_plugin_job_t * job);
void ism_plugin_replyCloseClient(ism_transport_t * transport);
static int close_callback(ism_transport_t * transport, int rc, int clean, const char * reason);
static int closed_callback(ism_transport_t * transport);

static int iopCount;

/*
 * Return names for plugin actions
 */
const char * ism_plugin_getActionName(int action) {
    switch (action) {
    /* Server to plug-in - global  */
    case PluginAction_Initialize      :  return "Initialize";
    case PluginAction_StartMessaging  :  return "StartMessaging";
    case PluginAction_Terminate       :  return "Terminate";
    case PluginAction_Endpoint        :  return "Endpoint";
    case PluginAction_InitChannel     :  return "InitChannel";
    case PluginAction_DefinePlugin    :  return "DefinePlugin";
    case PluginAction_OnConnection    :  return "OnConnection";
    case PluginAction_ConfigChange    :  return "ConfigChange";
    case PluginAction_Authenticate    :  return "Authenticate";
    case PluginAction_GetStats        :  return "GetStats";
    case PluginAction_InitConnection  :  return "InitConnection";

    /* Server to Plug-in - connection */
    case PluginAction_OnClose         :  return "OnClose";
    case PluginAction_OnComplete      :  return "OnComplete";
    case PluginAction_OnData          :  return "OnData";
    case PluginAction_OnMessage       :  return "OnMessage";
    case PluginAction_OnConnected     :  return "OnConnected";
    case PluginAction_OnLivenessCheck :  return "OnLivenessCheck";
    case PluginAction_OnHttpData      :  return "OnHttpData";
    case PluginAction_SuspendDelivery :  return "SuspendDelivery";
    case PluginAction_OnGetMessage    :  return "OnGetMessage";

    /* Plug-in to server - global */
    case PluginAction_NewConnection   :  return "NewConnection";
    case PluginAction_Stats           :  return "Stats";
    case PluginAction_Reply           :  return "Reply";
    case PluginAction_Log             :  return "Log";

    /* Plug-in to server - connection */
    case PluginAction_SendData        :  return "SendData";
    case PluginAction_Accept          :  return "Accept";
    case PluginAction_Identify        :  return "Identify";
    case PluginAction_Subscribe       :  return "Subscribe";
    case PluginAction_CloseSub        :  return "CloseSub";
    case PluginAction_DestroySub      :  return "DestroySub";
    case PluginAction_Send            :  return "Send";
    case PluginAction_Close           :  return "Close";
    case PluginAction_AsyncReply      :  return "AsyncReply";
    case PluginAction_SetKeepalive    :  return "SetKeepalive";
    case PluginAction_Acknowledge     :  return "Acknowledge";
    case PluginAction_DeleteRetain    :  return "DeleteRetain";
    case PluginAction_SendHttp        :  return "SendHttp";
    case PluginAction_ResumeDelivery  :  return "ResumeDelivery";
    case PluginAction_GetMessage      :  return "GetMessage";
    case PluginAction_CreateTransaction: return "CreateTransaction";
    case PluginAction_CommitTransaction: return "CommitTransaction";
    case PluginAction_RollbackTransaction: return "RollbackTransaction";
    case PluginAction_UpdateProperties: return "UpdateProperties";

    default                           :
        TRACE(5, "Unknown plugin action: %d\n", action);
        return "Unknown";
    }
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
 * Action structure for plugin incoming action
 */
typedef struct ism_plugin_act_t {
    uint8_t    action;
    uint8_t    hdrcount;
    uint8_t    paction;
    uint8_t    options;
    int        rc;
    int        connect;
    concat_alloc_t buf;
    ism_transport_t * channel;
    ism_transport_t * transport;
    ism_field_t * hdrs;
    ism_field_t pfield;
    ism_field_t body;
    uint64_t    seqnum;
    ism_plugin_cons_t * consumer;
} ism_plugin_act_t;


/*
 * Virutal connection linkage used during close
 */
typedef struct ism_transobj {
    ism_transport_t * next;
    ism_transport_t * prev;
} tobj_virt_t;


/*
 * Forward references
 */

ism_transport_t * ism_plugin_getChannelTransport(int which);
XAPI void ism_plugin_freeChannelTransport(int which);
ism_plugin_t * ism_plugin_findByName(const char * name);
ism_plugin_t * ism_plugin_findByAlias(const char * protocol);
int ism_plugin_replyControl(ism_transport_t * transport, int rc);
ism_plugin_t * ism_plugin_findByWSProtocol(const char * protocol);
ism_plugin_t * ism_plugin_findByFirstByte(uint8_t b);
int ism_transport_addNoFrame(ism_transport_t * transport, char * buffer, int len, int kind);
int ism_transport_noFrame(ism_transport_t * transport, char * buffer, int pos, int avail, int * used);
int ism_transport_allowConnection(ism_transport_t * transport);
int ism_plugin_sendData(ism_plugin_act_t * action, int kind);
int ism_plugin_newconn(ism_plugin_act_t * action, const char * protocol, const char * endpoint);
int ism_plugin_sendHttp(ism_plugin_act_t * action, int rc, const char * content_type, ism_field_t * props, ism_field_t * body);
int ism_plugin_stats(ism_plugin_act_t * action, int stat_syte, uint64_t heap_size, uint64_t heap_used,
        uint32_t gc_rate, uint32_t cpu_percent);
int ism_plugin_log(ism_transport_t * transport, const char * msgid, int level, const char * category,
        const char * filen, int lineno, const char * msgformat, ism_field_t * replf);
static int pluginAcceptConnection(ism_plugin_act_t * action, const char * protocol, const char * protocol_family, const char * plugin_name);
int ism_plugin_identify(ism_plugin_act_t * action, int auth, int keepAlive, int32_t maxMsgInFlight, ism_field_t * props);
int ism_plugin_subscribe(ism_plugin_act_t * action, int flags, int share, int transacted, const char * dest, const char * subname, const char * selector);
static int ism_plugin_closeSub(ism_plugin_act_t * action, const char * subname, int share);
static int ism_plugin_destroySub(ism_plugin_act_t * action, const char * subname, int share);
static int ism_plugin_message(ism_plugin_act_t * action, int msgtype, int flags, const char * dest, int64_t handle, ism_field_t * props, ism_field_t * body);
static int ism_plugin_closeConnection(ism_transport_t * transport, int rc, const char * reason);
static int ism_plugin_getRetainedMessage(ism_plugin_act_t * action, int seqnum, const char * topic);
static int ism_plugin_deleteRetain(ism_plugin_act_t * action, const char * topic);
int ism_tcp_addWork(ism_transport_t * transport, ism_transport_onDelivery_t ondelivery, void * userdata);
static int cleanupTimer(ism_timer_t key, ism_time_t timestamp, void * userdata);

static ism_plugin_job_t * findWork(ism_transport_t * transport, uint32_t seqnum, int remove);
static void makeConnectMap(concat_alloc_t * map, ism_transport_t * transport);
int ism_plugin_closing(ism_transport_t * transport, int rc, int clean, const char * reason);
static void replyAction(int32_t rc, void * handle, void * vaction);
static void replyCreateTransaction(int32_t rc, void * handle, void * vaction);
static void replyTransactionAction(int32_t rc, void * handle, void * vaction);
static int replyAuthTT(ism_timer_t timer, ism_time_t timestamp, void * callbackParam);
static ism_plugin_cons_t * findConsumer(ism_transport_t * transport);
static void freeConsumer(ism_transport_t * transport, ism_plugin_cons_t * consumer);
static ism_plugin_cons_t * findConsumerByName(ism_transport_t * transport, const char * name);
static void initHTTPServerHeaderSetting(void);
static inline const char *getServerHTTPHeaderString(void);
ism_endpoint_t * ism_transport_findEndpoint(const char * name);
static ismPluginPobj_t * clientsListHead = NULL;
static ismPluginPobj_t * clientsListTail = NULL;
pthread_spinlock_t pluginClientsListLock;
extern int plugin_unit_test;
static ism_timer_t       cleanup_timer = NULL;
static ism_transport_t * closedConnections;
static pthread_mutex_t   virtLock;

static bool g_sendServerHTTPHeader = true;
/*
 * Initialize virtual connections
 */
void ism_plugin_virtInit(void) {
    initHTTPServerHeaderSetting();
    iopCount = ism_tcp_getIOPcount();
    pthread_mutex_init(&virtLock, 0);
    cleanup_timer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) cleanupTimer, NULL, 10, 3, TS_SECONDS);
}


/*
 * Add a connection to the list of clients
 */
static void pluginAddToClientsList(ismPluginPobj_t * pobj, int32_t keepAlive) {
    TRACE(7, "pluginAddToClientsList: pobj=%p keepAlive=%d(%d)\n", pobj, keepAlive, pobj->keepAlive);
    if (keepAlive < 1)
        keepAlive = 0;
    pthread_spin_lock(&pluginClientsListLock);
    if (pobj->keepAlive == -1) {
        pobj->next = NULL;
        pobj->prev = clientsListTail;
        if (clientsListTail) {
            clientsListTail->next = pobj;
        } else {
            clientsListHead = pobj;
        }
        clientsListTail = pobj;
    }
    if (pobj->keepAlive > -2)
        pobj->keepAlive = keepAlive;

    pthread_spin_unlock(&pluginClientsListLock);
}

/*
 * Remove a connection from the list of clients
 */
static void pluginRemoveFromClientsList(ismPluginPobj_t * pobj, int lock) {
    lock = lock && (!plugin_unit_test);
    TRACE(7, "pluginRemoveFromClientsList: pobj=%p lock=%d\n", pobj, lock);
    if (lock)
        pthread_spin_lock(&pluginClientsListLock);
    if (pobj->keepAlive > -1) {
        if (pobj->prev) {
            pobj->prev->next = pobj->next;
        } else {
            clientsListHead = pobj->next;
        }
        if (pobj->next) {
            pobj->next->prev = pobj->prev;
        } else {
            clientsListTail = pobj->prev;
        }
        pobj->keepAlive = -1;
        pobj->next = pobj->prev = NULL;
    }
    pobj->keepAlive = -2;
    if (lock)
        pthread_spin_unlock(&pluginClientsListLock);
}

typedef struct pluginClientCloseRequestParam_t {
    int rc;
    const char * msg;
} pluginClientCloseRequestParam_t;

pluginClientCloseRequestParam_t connectionBrokeRequest = {ISMRC_UnableToConnect,  "No connection to plugin process" };
pluginClientCloseRequestParam_t pluginTerminatedRequest = {ISMRC_ServerTerminating,  "Server is going to shutdown" };

static int pluginClientClose(ism_transport_t * transport, void * userdata, int flags) {
    pluginClientCloseRequestParam_t * param = (pluginClientCloseRequestParam_t*)userdata;
    ism_plugin_closeConnection(transport, param->rc, param->msg);
    return 0;
}

void ism_plugin_closeAllClients(int shutdown) {
    ismPluginPobj_t * pobj;
    pluginClientCloseRequestParam_t * param = (shutdown) ? &pluginTerminatedRequest : &connectionBrokeRequest;
    pthread_spin_lock(&pluginClientsListLock);
    for (pobj = clientsListHead; pobj != NULL; pobj = pobj->next) {
        if (pobj->transport && pobj->transport->addwork)
            pobj->transport->addwork(pobj->transport, pluginClientClose, param);
    }
    pthread_spin_unlock(&pluginClientsListLock);
}

/*
 * Time out a connection
 */
static int pluginCheckLiveness(ism_transport_t * transport) {
    ismPluginPobj_t * pobj = transport->pobj;
    if (pobj->keepAlive < 1) {
        char xbuf[128];
        concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
        ism_protocol_putIntValue(&buf, transport->monitor_id);
        ism_transport_t * channel = ism_plugin_getChannelTransport(transport->tid);
        if (channel) {
            channel->send(channel, buf.buf+6, buf.used-6, (PluginAction_OnLivenessCheck<<8)+1, SFLAG_FRAMESPACE);
            ism_plugin_freeChannelTransport(transport->tid);
        } else {
            ism_plugin_closeConnection(transport,ISMRC_UnableToConnect, "No connection to plugin process");
        }
    }
    return 0;
}

static void pluginSuspendDelivery(ism_plugin_act_t * action) {
    ism_transport_t * channel = action->channel;
    ism_transport_t * transport = action->transport;
    char xbuf[64];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    ism_protocol_putIntValue(&buf, transport->monitor_id);
    channel->send(channel, buf.buf+6, buf.used-6, (PluginAction_SuspendDelivery<<8)+1, SFLAG_FRAMESPACE);

}

static int pluginResumeDelivery(ism_transport_t * transport, void * userdata) {
    ism_transport_t * channel = ism_plugin_getChannelTransport(transport->tid);
    if (channel) {
        char xbuf[64];
        concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
        ism_protocol_putIntValue(&buf, transport->monitor_id);
        channel->send(channel, buf.buf+6, buf.used-6, (PluginAction_ResumeDelivery<<8)+1, SFLAG_FRAMESPACE);
        ism_plugin_freeChannelTransport(transport->tid);
        return 0;
    }
    ism_plugin_closeConnection(transport,ISMRC_UnableToConnect, "No connection to plugin process");
    return 0;
}
static int pluginCommitTransaction(ism_plugin_act_t * action, int64_t handle) {
    ism_transport_t * transport = action->transport;
    ismPluginPobj_t * pobj = transport->pobj;
    if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
      __sync_fetch_and_sub(&pobj->inprogress, 1);
      ism_common_setError(ISMRC_Closed);
      return ISMRC_Closed;
    }
    void * transactionHandle = (void*) ((uintptr_t)handle);
    int rc = ism_engine_commitTransaction(pobj->session_handle,	transactionHandle, 0,
    		action, sizeof(ism_plugin_act_t), replyTransactionAction);
    if (rc != ISMRC_AsyncCompletion)
    	replyTransactionAction(rc, NULL, action);
    return ISMRC_OK;
}

static int pluginRollbackTransaction(ism_plugin_act_t * action, int64_t handle) {
    ism_transport_t * transport = action->transport;
    ismPluginPobj_t * pobj = transport->pobj;
    if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
      __sync_fetch_and_sub(&pobj->inprogress, 1);
      ism_common_setError(ISMRC_Closed);
      return ISMRC_Closed;
    }
    void * transactionHandle = (void*) ((uintptr_t)handle);
    int rc = ism_engine_rollbackTransaction(pobj->session_handle, transactionHandle,
    		action, sizeof(ism_plugin_act_t), replyTransactionAction);
    if (rc != ISMRC_AsyncCompletion)
    	replyTransactionAction(rc, NULL, action);
    return ISMRC_OK;
}

static int pluginCreateTransaction(ism_plugin_act_t * action) {
    ism_transport_t * transport = action->transport;
    ismPluginPobj_t * pobj = transport->pobj;
    if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
      __sync_fetch_and_sub(&pobj->inprogress, 1);
      ism_common_setError(ISMRC_Closed);
      return ISMRC_Closed;
    }
  	replyTransactionAction(0, NULL, action);
    return ISMRC_OK;
}



static void pluginResumeConsumerDelivery(ism_plugin_act_t * action, int deliveredMsgCount) {
    ism_transport_t * transport = action->transport;
    ismPluginPobj_t * pobj = transport->pobj;
    pthread_spin_lock(&pobj->sessionlock);
    int oldCounter = pobj->msgInFlightCounter;
    pobj->msgInFlightCounter -= deliveredMsgCount;
    if ((oldCounter < pobj->maxMsgInFlight) ||                 /* No consumers should be suspended */
       (pobj->msgInFlightCounter >= pobj->maxMsgInFlight)) {   /* We should not resume yet */
        pthread_spin_unlock(&pobj->sessionlock);
        return;
    }
    /* Create a list of suspended consumers while we still own the lock */
    int count = 0;
    int i;
    void * * consumers = alloca(sizeof(void *)*pobj->consumer_alloc);
    for (i = 1; i < pobj->consumer_alloc; i++) {
        ism_plugin_cons_t * consumer = pobj->consumers[i];
        if ((!consumer) || (consumer->closed) || (!consumer->chandle) || (!consumer->suspended))
            continue;
        consumers[count++] = consumer->chandle;
        consumer->suspended = 0;
    }
    pthread_spin_unlock(&pobj->sessionlock);
    for (i = 0; i < count; i++) {
        xUNUSED int zrc = ism_engine_resumeMessageDelivery(consumers[i], 0, NULL, 0, NULL);
    }
}

/*
 * Check the last access for a connection
 */
static int checkLastAccessTime(ismPluginPobj_t * pobj, uint64_t currTime) {
    ism_transport_t * transport = pobj->transport;
    if ((pobj->keepAlive > 0) && !pobj->closed) {
        /* If last access + (1.5 * keep alive interval) < current time */
        if ((transport->lastAccessTime + pobj->keepAlive + (pobj->keepAlive >> 1)) < currTime) {
            TRACE(3, "Plug-in warning: KeepAlive timeout: connection=%u client=%s\n",
                    transport->index, transport->name);
            pluginCheckLiveness(transport);
        }
    }
    return 1;
}

/*
 * This function performs disconnect, if the client fails to communicate
 * within the Keep Alive timer schedule.
 */
int ism_plugin_TimerDisconnect(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ismPluginPobj_t * curr;
    ismPluginPobj_t * next;
    pthread_spin_lock(&pluginClientsListLock);
    uint64_t currTime = (uint64_t) ism_common_readTSC();
    curr = clientsListHead;
    while (curr) {
        next = curr->next;
        checkLastAccessTime(curr, currTime);
        curr = next;
    }
    pthread_spin_unlock(&pluginClientsListLock);
    return 1;
}

/*
 * Receive as a surrogate for the client
 */
int ism_plugin_receiveData(ism_transport_t * transport, char * databuf, int buflen, int command) {
    ism_transport_t * channel = ism_plugin_getChannelTransport(transport->tid);
    if (channel) {
        char xbuf[16*1024];
        int rc = 0;
        concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
        ism_protocol_putIntValue(&buf, transport->monitor_id);
        ism_protocol_putNullValue(&buf);
        ism_protocol_putByteArrayValue(&buf, databuf, buflen);
        /*
         * The plugin code to honor maxMessageSize is not yet implemented,
         * but check the 16MB limit to stop the channel from failing.
         */
        if (buf.used >= 16*1024*1024) {
            ism_plugin_closeConnection(transport,ISMRC_MsgTooBig, "The data packet is too large");
            rc = -1;
        } else {
            channel->send(channel, buf.buf+6, buf.used-6, (PluginAction_OnData<<8)+1, SFLAG_FRAMESPACE);
            ism_plugin_freeChannelTransport(transport->tid);
        }
        if(buf.inheap)
            ism_common_freeAllocBuffer(&buf);
        return rc;
    }
    ism_plugin_closeConnection(transport,ISMRC_UnableToConnect, "No connection to plugin process");
    return -1;
}


/*
 * Reply with a received message.  This is the engine callback from the message consumer.
 */
static bool replyMessage(ismEngine_ConsumerHandle_t consumerh,
        ismEngine_DeliveryHandle_t deliveryh, ismEngine_MessageHandle_t msgh,
        uint32_t seqnum, ismMessageState_t state, uint32_t options,
        ismMessageHeader_t * hdr, uint8_t areas,
        ismMessageAreaType_t areatype[areas], size_t areasize[areas],
        void * areaptr[areas], void * vaction) {

	char xbuf[12000];
    concat_alloc_t buf = { xbuf, sizeof xbuf, 6 };
    ism_field_t ftopic;
    concat_alloc_t pbuf;
    uint32_t proplen = 0;
    uint32_t bodylen = 0;
    char * propp = NULL;
    char * bodyp = NULL;
    int i;
    bool returncode = true;
    uint8_t flags = 0;
    uint8_t qos = (hdr->Reliability & 3);

    ism_plugin_cons_t * consumer=NULL;
	ism_transport_t * transport=NULL;
	ism_protobj_t * pobj = NULL;
	int isGetRetainedMessage =0;
	int actionType;
	ism_plugin_act_t * action=NULL;

	if (consumerh != NULL) {
		consumer = (ism_plugin_cons_t *) vaction;
		transport = consumer->transport;
		pobj = transport->pobj;
	} else {
		isGetRetainedMessage=1;
		action = (ism_plugin_act_t *) vaction;
		transport = action->transport;
	}
	pobj = transport->pobj;

    if (!isGetRetainedMessage) {
		if (qos > consumer->qos)
			qos = consumer->qos;
		consumer = pobj->consumers[consumer->which];
    }
    flags = qos;

    /* Find the props and body */
    for (i = 0; i < areas; i++) {
        if (areatype[i] == ismMESSAGE_AREA_PROPERTIES) {
            proplen = (uint32_t) areasize[i];
            propp = (char *) areaptr[i];
        } else if (areatype[i] == ismMESSAGE_AREA_PAYLOAD) {
            bodylen = (uint32_t) areasize[i];
            bodyp = (char *) areaptr[i];
        }
    }

    /*
     * Find the topic name in the properties
     */
    uint8_t desttype=ismDESTINATION_TOPIC;
    if (!isGetRetainedMessage) {
    	desttype = consumer->desttype;
    }

    if (proplen && desttype == ismDESTINATION_TOPIC) {
        memset(&pbuf, 0, sizeof(pbuf));
        pbuf.buf = propp;
        pbuf.len = proplen;
        pbuf.used = proplen;
        ism_findPropertyNameIndex(&pbuf, ID_Topic, &ftopic);
    } else {
        ftopic.type = VT_Null;
        ftopic.val.s = NULL;
    }

    if (!isGetRetainedMessage) {
		/*
		 * If the topic name is missing, use the name of the subscription.  This is a problem as
		 * it might contain wildcard characters which we will need to replace later.
		 */
		if (ftopic.type != VT_String || ftopic.val.s == NULL) {
			ftopic.val.s = (char *) consumer->dest;
			if (consumer->desttype == ismDESTINATION_TOPIC)
				TRACEL(8, transport->trclevel, "The destination name was not in the properties, using topic in consumer object: connect=%u client=%s topic=%s\n",
						transport->index, transport->name, ftopic.val.s);
		}
    }
    if (hdr->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN)
        flags |= 0x18;

    if (hdr->Persistence)
        flags |= 0x04;

    if (!isGetRetainedMessage) {

    	if (consumer->desttype == ismDESTINATION_QUEUE) {
    		flags |= 0x20;
    	}
		/*
		 *
		 * Send the onMessage action
		 */
		ism_protocol_putIntValue(&buf, transport->monitor_id);
		if (qos && deliveryh) {
			ism_plugin_job_t *job = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,38),1, sizeof(ism_plugin_job_t));
			job->deliveryh = deliveryh;
			job->transport = transport;
			registerWork(job);
			ism_protocol_putIntValue(&buf, job->seqnum);   /* seqnum */
		} else {
			ism_protocol_putIntValue(&buf, 0);   /* seqnum */
		}
		ism_protocol_putByteValue(&buf, hdr->MessageType);
		ism_protocol_putByteValue(&buf, flags);
		ism_protocol_putStringValue(&buf, consumer->name);
		ism_protocol_putStringValue(&buf, ftopic.val.s);
		if (proplen)
			ism_protocol_putMapValue(&buf, propp, proplen);
		else
			ism_protocol_putNullValue(&buf);
		ism_protocol_putByteArrayValue(&buf, bodyp, bodylen);
		actionType=PluginAction_OnMessage;
    } else {
    	/*
		 * Send the onGetMessage action
		 */
		ism_protocol_putIntValue(&buf, transport->monitor_id);
		ism_protocol_putIntValue(&buf, action->seqnum);
		/*set RC*/
		ism_protocol_putIntValue(&buf, ISMRC_OK);

		ism_protocol_putByteValue(&buf, hdr->MessageType);
		ism_protocol_putByteValue(&buf, flags);
		ism_protocol_putStringValue(&buf, ftopic.val.s);
		if (proplen)
			ism_protocol_putMapValue(&buf, propp, proplen);
		else
			ism_protocol_putNullValue(&buf);
		ism_protocol_putByteArrayValue(&buf, bodyp, bodylen);

		actionType=PluginAction_OnGetMessage;
    }
    ism_transport_t * channel = ism_plugin_getChannelTransport(transport->tid);
    if (channel) {
        channel->send(channel, buf.buf+6, buf.used-6, (actionType<<8)+6, SFLAG_FRAMESPACE);
        ism_plugin_freeChannelTransport(transport->tid);

        if (!isGetRetainedMessage) {
			pthread_spin_lock(&pobj->sessionlock);
			pobj->msgInFlightCounter++;
			if (pobj->msgInFlightCounter >= pobj->maxMsgInFlight) {
				consumer->suspended = 1;
				ism_engine_suspendMessageDelivery(consumerh, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
				returncode = false;
			}
			pthread_spin_unlock(&pobj->sessionlock);
        }
    }
    if (deliveryh) {
        if (channel == NULL) {
            xUNUSED int zrc = ism_engine_confirmMessageDelivery(pobj->session_handle, NULL, deliveryh,
                    ismENGINE_CONFIRM_OPTION_NOT_DELIVERED, NULL, 0, NULL);
        }
    }

    if (!isGetRetainedMessage) {
		register uint64_t * p1 = (channel) ? &transport->listener->stats->count[transport->tid].write_msg : &transport->listener->stats->count[transport->tid].lost_msg;
		register uint64_t * p2 = &transport->write_msg;

		__sync_add_and_fetch(p1, 1);
		__sync_add_and_fetch(p2, 1);
    }

    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
    ism_engine_releaseMessage(msgh);
    if (!channel)
        ism_plugin_closeConnection(transport,ISMRC_UnableToConnect, "No connection to plugin process");

    return returncode;
}

/*
 * Validate the header
 */
static void validate(ism_plugin_act_t * action, int hdrcnt, const char * types) {
    int  i;
    ism_field_t * hdr = action->hdrs;
    if (action->hdrcount < hdrcnt) {
        action->rc = ISMRC_BadClientData;
        ism_common_setError(action->rc);
    }
    for (i=0; i<hdrcnt; i++) {
    	char type = types[i];
        switch (type) {
        case 'I':
            if (hdr[i].type != VT_Integer && hdr[i].type != VT_Byte) {
                action->rc = ISMRC_BadClientData;
                ism_common_setError(action->rc);
            }
            break;
        case 'C':
            if (hdr[i].type != VT_Integer) {
                action->rc = ISMRC_BadClientData;
                ism_common_setError(action->rc);
            }
            action->connect = hdr[i].val.i;
            action->transport = ism_transport_getTransport(action->connect);
            if (!action->transport) {
                action->rc = ISMRC_BadClientData;
                ism_common_setError(action->rc);
            } else {
                if (action->channel->clientport < 2048 && (action->channel->clientport%iopCount) != action->transport->tid) {
                    TRACE(2, "Plugin action not on correct channel: monitor=%u channel=%u\n", action->connect, action->channel->clientport);
                    action->rc = ISMRC_BadClientData;
                }
            }
            break;
        case 'Q':
        case 'L':
            if (hdr[i].type != VT_Long && hdr[i].type != VT_Integer) {
                action->rc = ISMRC_BadClientData;
                ism_common_setError(action->rc);
            } else {
                if (hdr[i].type == VT_Integer) {
                    uint64_t lval = (uint64_t)(uint32_t)hdr[i].val.i;
                    hdr[i].val.l = (int64_t)lval;
                    hdr[i].type = VT_Long;
                }
                if (LIKELY(type == 'Q'))
                	action->seqnum = hdr[i].val.l;
            }
            break;
        case 'S':
            if (hdr[i].type != VT_String && hdr[i].type != VT_Null) {
                action->rc = ISMRC_BadClientData;
                ism_common_setError(action->rc);
            }
            if (hdr[i].type == VT_Null)
                hdr[i].val.s = NULL;
            break;
        }
    }
    if (types[hdrcnt]=='P' && action->pfield.type != VT_Map) {
        action->rc = ISMRC_BadClientData;
        ism_common_setError(action->rc);
    }
}


/*
 * Validate the fields in an action
 */
static void validateAction(ism_plugin_act_t * action) {
    /*
     * Validate the actions
     */
    if (action->rc == 0) {
        switch (action->action) {
        case PluginAction_SendData:        /* h0=connect h1=kind b=data */
            validate(action, 1, "CI");
            break;
        case PluginAction_Send:            /* h0=oonnect, h1=seqnum, h2=msgtype, h3=flags, h4=dest, h5=transaction, p=props, b=body  */
            validate(action, 6,"CQIISLP");
            break;
        case PluginAction_Acknowledge:     /* h0=connect, h1=seqnum, h2=rc h3=transaction*/
            validate(action, 4,"CQIL");
            break;
        case PluginAction_CommitTransaction: /* h0=connect, h1=seqnum, h2=txID */
            validate(action, 3,"CQL");
            break;
        case PluginAction_SendHttp:        /* h0=connect h1=rc h2=content_type, p=map, b-body */
            validate(action, 3, "CIS");
            break;
        case PluginAction_NewConnection:   /* h0=seqnum, h1=protocol */
            validate(action, 2, "QSS");
            break;
        case PluginAction_Stats:           /* h0=type, h1=heapsize, h2=headused, h3=gc_rate, h4=cpu */
            validate(action, 5, "ILLII");
            break;
        case PluginAction_Reply:           /* h0=rc */
            validate(action, 1, "I");
            break;
        case PluginAction_RollbackTransaction: /* h0=connect, h1=seqnum, h2=txID */
            validate(action, 3,"CQL");
            break;
        case PluginAction_Log:           /* h0=rc */
            validate(action, 6, "SISSIS");
            break;
        case PluginAction_Accept:          /* h0=connect, h1=protocol, h2=protocol_family, h3=plugin_name*/
            validate(action, 4, "CSSS");
            break;
        case PluginAction_Identify:  /* h0=connect, h1=seqnum, h2=auth, h3=keepalive, h4=maxMsgInFlight, p=connection */
            validate(action, 5, "CQIIIP");
            break;
        case PluginAction_Subscribe:       /* h0=connect, h1=seqnum, h2=flags, h3=share, h4=tansacted, h5=dest, h6=name, h7=selector */
            validate(action, 8,"CQIIISSS");
            break;
        case PluginAction_CreateTransaction: /* h0=connect, h1=seqnum */
            validate(action, 2,"CQ");
            break;
        case PluginAction_CloseSub:        /* h0=connect, h1=seqnum, h2=subname, h3=share */
            validate(action, 3,"CQSI");
            break;
        case PluginAction_DestroySub:      /* h0=connect, h1=seqnum, h2=subname, h3=share */
            validate(action, 4,"CQSI");
            break;
        case PluginAction_Close:           /* h0=connect, h1=seqnum, h2=rc, h3=reason */
            validate(action, 4,"CQIS");
            break;
        case PluginAction_SetKeepalive:           /* h0=connect, h1=timeout */
            validate(action, 2,"CI");
            break;
        case PluginAction_ResumeDelivery:     /* h0=connect, h1=msgCount */
            validate(action, 2,"CI");
            break;
        case PluginAction_GetMessage:     /* h0=connect, h1=seqnum, h2=topic */
        	validate(action, 3,"CQS");
            break;
        case PluginAction_DeleteRetain:     /* h0=connect, h1=topic */
			validate(action, 3,"CQS");
			break;
        default:
            action->rc = ISMRC_BadClientData;
        }
    }
}


/*
 * Set the keepalive after accept
 */
static int setKeepAlive(ism_plugin_act_t * action, int timeout) {
    ism_transport_t * transport = action->transport;
    ismPluginPobj_t * pobj = transport->pobj;
    pluginAddToClientsList(pobj, timeout);
    return 0;
}


/*
 * Do a message acknowledge
 */
static int doAcknowledge(ism_plugin_act_t * action, int rc, int64_t handle) {
    ism_transport_t * transport = action->transport;
    ismPluginPobj_t * pobj = transport->pobj;
    ism_plugin_job_t * job = findWork(transport, action->seqnum, 1);
    if (job) {
        if (pobj->session_handle) {
            //TODO: check rc and set delivery option accordingly
            xUNUSED int zrc = ism_engine_confirmMessageDelivery(pobj->session_handle, (void*)((uintptr_t)handle),
            		job->deliveryh, ismENGINE_CONFIRM_OPTION_CONSUMED, NULL, 0, NULL);
        }
        ism_common_free(ism_memory_protocol_misc,job);
    }

    return 0;
}


/*
 * Receive an action from the plug-in process
 */
int ism_plugin_receive(ism_transport_t * transport, char * buf, int buflen, int command) {
    ism_plugin_act_t action = {0};
    int  i;
    int  rc = 0;
    int  lrc;
    char trcbuf [256];
    char trcbuf2 [64];
    ism_field_t hdr[15];

    action.action = command >> 8;
    action.hdrcount = command&0x0f;
    memset(&action.buf, 0, sizeof(action.buf));
    action.buf.buf = buf;
    action.buf.used = buflen;
    action.channel = transport;
    action.seqnum = 0;
    action.hdrs = hdr;

    /*
     * Binary dump trace of the message except createConnection which can contain a password
     * For message actions we only trace message data if msgdata is set for that many bytes.
     */
    if (SHOULD_TRACE(9)) {
        int maxsize = ism_common_getTraceMsgData()+40;
        sprintf(trcbuf, "Plug-in receive %s %u channel=%u", ism_plugin_getActionName(action.action),
                action.hdrcount, transport->clientport);
        TRACEDATA(9, trcbuf, 0, buf, buflen, maxsize);
    }

    for (i=0; i<action.hdrcount; i++) {
        lrc = ism_protocol_getObjectValue(&action.buf, hdr+i);
        if (lrc) {
            TRACEL(2, transport->trclevel, "Bad plug-in header value: connect=%u which=%d rc=%d\n", transport->index, i, lrc);
            rc += lrc;
        }
    }

    /* Parse the properties */
    if (action.buf.pos < action.buf.used) {
        action.pfield.len = 0;
        lrc = ism_protocol_getObjectValue(&action.buf, &action.pfield);
        if (lrc) {
            TRACEL(2, transport->trclevel, "Bad plug-in properties: connect=%u rc=%d\n", transport->index, lrc);
            rc += lrc;
        }
        /* Treat empty properties as no properties */
        if (action.pfield.type != VT_Map) {
            action.pfield.type = VT_Null;
        } else if (action.pfield.len == 0) {
            action.pfield.type = VT_Null;
        }
    } else {
        action.pfield.len  = 0;
        action.pfield.type = VT_Null;
    }

    /* Parse the body */
    if (action.buf.pos < action.buf.used) {
        action.body.len = 0;
        lrc = ism_protocol_getObjectValue(&action.buf, &action.body);
        if (lrc || action.body.type == VT_Null) {
            TRACEL(2, transport->trclevel, "Bad plug-in body: connect=%u rc=%d\n", transport->index, lrc);
            rc += lrc;
        }
    } else {
        action.body.len = 0;
        action.body.type = VT_Null;
    }

    action.rc = rc;
    validateAction(&action);
    if (action.rc) {
        TRACEL(2, transport->trclevel, "Bad plug-in data: connect=%u rc=%d\n", transport->index, action.rc);
        if (rc) {
            sprintf(trcbuf2, "The plug-in action data is not valid: connect=%u", transport->index);
            TRACEDATA(2, trcbuf2, 0, buf, buflen, 2048);
            ism_common_setError(ISMRC_MessageNotValid);
            transport->close(transport, ISMRC_BadClientData, 0, "The data from the client is not valid");
            return ISMRC_MessageNotValid;
        }
        return action.rc;
    }

    /*
     * Process the actions
     */
    switch (action.action) {
    case PluginAction_SendData:        /* h0=connect, h1=kind */
        rc = ism_plugin_sendData(&action, hdr[1].val.i);
        break;
    case PluginAction_Send:            /* h2=msgtype, h3=flags, h4=dest, h5=transaction, p=props, b=body  */
        rc = ism_plugin_message(&action, hdr[2].val.i, hdr[3].val.i, hdr[4].val.s, hdr[5].val.l, &action.pfield, &action.body);
        break;
    case PluginAction_SendHttp:        /* h0=connect, h1=rc, h2=content_type, p=map, b=body */
        rc = ism_plugin_sendHttp(&action, hdr[1].val.i, hdr[2].val.s, &action.pfield, &action.body);
        break;
    case PluginAction_Acknowledge:     /* h0=connect,h1=seqnum, h2=rc, h3=transaction */
        rc = doAcknowledge(&action, hdr[2].val.i, hdr[3].val.l);
        break;
    case PluginAction_ResumeDelivery:     /* h0=connect,h1=msgCount */
        pluginResumeConsumerDelivery(&action, hdr[1].val.i);
        break;
    case PluginAction_CommitTransaction: /* h0=connect,h1=seqnum, h2=handle */
    	rc = pluginCommitTransaction(&action,hdr[2].val.l);
    	break;
    case PluginAction_RollbackTransaction: /* h0=connect,h1=seqnum, h2=handle */
    	rc = pluginRollbackTransaction(&action,hdr[2].val.l);
    	break;
    case PluginAction_CreateTransaction: /* h0=connect,h1=seqnum, h2=handle */
    	rc = pluginCreateTransaction(&action);
    	break;
    case PluginAction_NewConnection:   /* h0=seqnum, h1=protocol, h2=endpoint */
        rc = ism_plugin_newconn(&action, hdr[1].val.s, hdr[2].val.s);
        break;
    case PluginAction_Stats:           /* h0=type, h1=heapsize, h2=headused, h3=gc_rate, h4=cpu */
        rc = ism_plugin_stats(&action, hdr[0].val.i, hdr[1].val.l, hdr[2].val.l,hdr[3].val.i, hdr[4].val.i);
        break;
    case PluginAction_Reply:           /* h0=rc */
        rc = ism_plugin_replyControl(action.channel, hdr[0].val.i);
        break;
    case PluginAction_Log:           /* h0=rc */
        rc = ism_plugin_log(action.channel, hdr[0].val.s, hdr[1].val.i, hdr[2].val.s, hdr[3].val.s, hdr[4].val.i,
                hdr[5].val.s, &action.pfield);
        break;
    case PluginAction_Accept:          /* h0=connect,h1=protocol, h2=protocol_family, h3=plugin_name */
        rc = pluginAcceptConnection(&action, hdr[1].val.s, hdr[2].val.s, hdr[3].val.s);
        break;
    case PluginAction_Identify:        /* h0=connect, h1=seqnum, h2=auth, h3=keepalive, h4=maxMsgInFlight, p=connection */
        rc = ism_plugin_identify(&action, hdr[2].val.i, hdr[3].val.i, hdr[4].val.i, &action.pfield);
        break;
    case PluginAction_Subscribe:       /* h0==connect, h2=flags, h3=share, h4=tansacted, h5=dest, h6=name, h7=selector */
        rc = ism_plugin_subscribe(&action, hdr[2].val.i, hdr[3].val.i, hdr[4].val.i, hdr[5].val.s, hdr[6].val.s, hdr[7].val.s);
        break;
    case PluginAction_CloseSub:        /* h0=connect, h2=name, h3=shared */
        rc = ism_plugin_closeSub(&action, hdr[2].val.s, hdr[3].val.i);
        break;
    case PluginAction_DestroySub:       /* h0=connect, h2=name, h3=shared */
        rc = ism_plugin_destroySub(&action, hdr[2].val.s, hdr[3].val.i);
        break;
    case PluginAction_Close:           /* h0=connect h2=rc, h3=reason */
        rc = ism_plugin_closeConnection(action.transport, hdr[2].val.i, hdr[3].val.s);
        break;
    case PluginAction_SetKeepalive:           /* h0=connect h1=timeout */
        rc = setKeepAlive(&action, hdr[1].val.i);
        break;
    case PluginAction_GetMessage:           /* h0=connect h1=seqnum h2=topic*/
		rc = ism_plugin_getRetainedMessage(&action, hdr[1].val.i,hdr[2].val.s);
		break;
    case PluginAction_DeleteRetain:           /* h0=connect h1=seqnum h2=topic*/
		rc = ism_plugin_deleteRetain(&action, hdr[2].val.s);
		break;

    }
    return 0;
}


/*
 * Send data out bound
 */
int ism_plugin_sendData(ism_plugin_act_t * action, int kind) {
    ism_transport_t * transport = action->transport;
    if (action->body.type == VT_ByteArray) {
        int rc = transport->send(transport, action->body.val.s, action->body.len, kind==1 ? 1 : 2, 0);
        if (rc == SRETURN_SUSPEND)
            pluginSuspendDelivery(action);
        return 0;
    }
    return 1;
}

/*
 * Send an onConnected action
 */
static void pluginReplyConnect(ism_plugin_act_t * action) {
    char xbuf[512];
    char mbuf[512];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    ism_protocol_putIntValue(&buf, action->transport->monitor_id);
    ism_protocol_putIntValue(&buf, action->seqnum);
    ism_protocol_putIntValue(&buf, action->rc);
    if (action->rc) {
        /*
         * Handle the case where whoever set the bad return code did not call
         * ism_common_setError() and thus the last error is incorrect.
         */
        if (action->rc != ism_common_getLastError()) {
            ism_common_setError(action->rc);
        }
        ism_common_formatLastErrorByLocale(ism_common_getLocale(), mbuf, sizeof(mbuf));
        ism_protocol_putStringValue(&buf, mbuf);
    } else {
        ism_protocol_putNullValue(&buf);
    }
    action->channel->send(action->channel, buf.buf+6, buf.used-6, (PluginAction_OnConnected<<8)+4, SFLAG_FRAMESPACE);
}


/*
 * Send an onComplete action
 */
static inline void replyComplete(ism_plugin_act_t * action, int rc) {
    if (action->seqnum) {
        ism_transport_t * transport = action->transport;
        ism_transport_t * channel = action->channel;
        char xbuf[1024];
        concat_alloc_t buf = {xbuf, sizeof xbuf, 6};

        ism_protocol_putIntValue(&buf, transport->monitor_id);
        ism_protocol_putIntValue(&buf, (int)action->seqnum);
        ism_protocol_putIntValue(&buf, rc);   /* rc */

        /*
         * If whoever set the RC did not do a ism_common_setError(), force it now so that the last
         * error string is correct.
         */
        if (rc != ism_common_getLastError()) {
            ism_common_setError(rc);
        }
        if (UNLIKELY(rc != 0)) {
            char errbuf[1024];
            ism_common_formatLastErrorByLocale(ism_common_getLocale(), errbuf, sizeof(errbuf));
            ism_protocol_putStringValue(&buf, errbuf);
        } else {
            ism_protocol_putNullValue(&buf);
        }
        channel->send(channel, buf.buf+6, buf.used-6, (PluginAction_OnComplete<<8)+4, SFLAG_FRAMESPACE);
        if (buf.inheap)
            ism_common_freeAllocBuffer(&buf);
    }
}

/*
 * Close callback for virtual connections
 */
static int close_callback(ism_transport_t * transport, int rc, int clean, const char * reason) {
    /*
     * Make sure that the close callback is only processed once.  The first one wins.
     */
    if (!transport)
        return 1;
    if (!reason)
        reason = "";

    if (!(__sync_bool_compare_and_swap(&transport->state, ISM_TRANST_Open, ISM_TRANST_Closing)
            || __sync_bool_compare_and_swap(&transport->state, ISM_TRANST_Opening, ISM_TRANST_Closing))) {
        TRACE(6, "The connection cannot close due to state: index=%u name=%s state=%u\n",
                transport->index, transport->name, transport->state);
        return 1;
    }

    TRACE(7, "plugin close_callback: index=%u name=%s reason=%s\n", transport->index, transport->name, reason);

    /*
     * Call the protocol
     */
    if (transport->closing != NULL) {
        transport->closing(transport, rc, clean, reason);
    } else {
        closed_callback(transport);
    }

    return 0;
}

/*
 * Start the close of a connections
 */
static int closed_callback(ism_transport_t * transport) {
    TRACE(8, "plugin closed callback: connect=%u name=%s transport=%p\n", transport->index, transport->name, transport);
    /* Destroy the Security Context */
    if (transport->security_context) {
        ism_security_destroy_context(transport->security_context);
        transport->security_context = NULL;
    }

    if (transport->monitor_id) {
        ism_transport_removeMonitor(transport, 1);
    }

    pthread_mutex_lock(&virtLock);
    transport->tobj->next = closedConnections;
    if (closedConnections) {
        closedConnections->tobj->prev = transport;
    }
    transport->tobj->prev = NULL;
    closedConnections = transport;
    transport->state = ISM_TRANST_Closed;
    pthread_mutex_unlock(&virtLock);
    return 0;
}

/*
 * Timer function for cleanup
 */
static int cleanupTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_transport_t * transport;
    ism_transport_t * next;
    pthread_mutex_lock(&virtLock);
    transport = closedConnections;
    while (transport) {
        next = transport->tobj->next;
        if (transport->workCount) {
            transport = next;
            continue;
        }

        if (transport->monitor_id) {
            ism_transport_removeMonitor(transport, 1);
        }
        if (transport->closestate[0] > 1) {
            transport->closestate[1]++;
        }
        if (transport->closestate[1] > 1) {
            if (next) {
                next->tobj->prev = transport->tobj->prev;
            }
            if (transport->tobj->prev) {
                transport->tobj->prev->tobj->next = next;
            } else {
                closedConnections = next;
            }
            TRACE(8, "plugin cleanupTimer - going to free connection: connect=%u\n", transport->index);
            ism_transport_freeTransport(transport);
        }
        transport = next;
    }
    pthread_mutex_unlock(&virtLock);

    return 1;
}

/*
 * Process a new virtul connection
 */
int ism_plugin_newconn(ism_plugin_act_t * action, const char * protocol, const char * endpoint) {
    ism_transport_t * transport;
    ism_transport_t * channel;
    ism_endpoint_t * endp;

    endp = ism_transport_findEndpoint(endpoint);

    /* Create a new transport object */
    transport = ism_transport_newTransport(endp, sizeof(tobj_virt_t), 0);
    transport->addwork = ism_tcp_addWork;
    transport->protocol = ism_transport_putString(transport, protocol);
    if (action->hdrcount > 3 && action->hdrs[3].type == VT_String) {
        transport->protocol_family = ism_transport_putString(transport, action->hdrs[3].val.s);
    } else {
        transport->protocol_family = transport->protocol;
    }
    ism_protobj_t * pobj = (ism_protobj_t *) ism_transport_allocBytes(transport, sizeof(ism_protobj_t), 1);
    memset((char *) pobj, 0, sizeof(ism_protobj_t));
    transport->pobj = pobj;
    pobj->transport = transport;
    pthread_spin_init(&pobj->lock, 0);
    pthread_spin_init(&pobj->sessionlock, 0);
    pobj->keepAlive = -1;
    transport->resume = pluginResumeDelivery;
    transport->close = close_callback;
    transport->closing = ism_plugin_closing;
    transport->closed = closed_callback;
    pluginAddToClientsList(pobj, 0);

    ism_security_create_context(ismSEC_POLICY_CONNECTION, transport, &transport->security_context);

    /* Send back the onConnection */
    char xbuf[4098];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    if (!transport->monitor_id) {
        ism_transport_addMonitorNow(transport);
    }

    ism_protocol_putIntValue(&buf, (uint32_t)action->seqnum);
    ism_protocol_putIntValue(&buf, 1);    /* Connection related seqnum */
    ism_protocol_putByteValue(&buf, 3);   /* Connection type 3 (virtual) */
    ism_protocol_putIntValue(&buf, transport->monitor_id);   /* New monitor ID */
    makeConnectMap(&buf, transport);
    channel = ism_plugin_getChannelTransport(transport->tid);
    if (channel) {
        channel->send(channel, buf.buf+6, buf.used-6, (PluginAction_OnConnection<<8)+4, SFLAG_FRAMESPACE);
        ism_plugin_freeChannelTransport(transport->tid);
    } else {
        /* Use control channel */
        action->channel->send(action->channel, buf.buf+6, buf.used-6, (PluginAction_OnConnection<<8)+4, SFLAG_FRAMESPACE);
    }
    return 0;
}


/*
 * Get plug-in statistics.
 */
int ism_plugin_stats(ism_plugin_act_t * action, int stat_type, uint64_t heap_size, uint64_t heap_used,
        uint32_t gc_rate, uint32_t cpu_percent) {
    uint32_t  heap_mb      = (uint32_t)((heap_size+(512*1024))/(1024*1024));
    uint32_t  heap_percent = (int32_t)(((heap_used + (heap_size/200)) * 100) / heap_size);
    int       statsize;
    char xbuf [1024];
    char date [64];
    const char * server = ism_common_getServerName();

    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_common_formatTimestamp(ts, date, sizeof date, 7,  ISM_TFF_SPACE | ISM_TFF_ISO8601);
    ism_common_closeTimestamp(ts);
    sprintf(xbuf, "{ \"Version\":\"%s\", \"NodeName\":\"%s\", \"TimeStamp\":\"%s\", \"ObjectType\": \"Plugin\",\n"
            "\"HeapSizeMB\":%u, \"HeapUsedPercent\":\%u, \"GCRate\":%u, \"CPU\":%u }",
            ism_common_getVersion(), server, date, heap_mb, heap_percent, gc_rate, cpu_percent);
    statsize = strlen(xbuf);
    if (stat_type == 0) {
        ism_monitoring_submitMonitoringEvent(ismMonObjectType_Plugin, "Plugin", 6, xbuf, statsize, ismPublishType_SYNC);
    } else {
        printf("%s", xbuf);
    }
    return 0;
}



/*
 * Send a message to the server logs
 */
int ism_plugin_log(ism_transport_t * transport, const char * msgid, int level, const char * category,
        const char * filen, int lineno, const char * msgformat, ism_field_t * replf) {
    char * r[16];
    char   buf[400];
    char * bufp = buf;
    char   types [64];
    int    i;
    ism_field_t field = {0};
    int    cat = LOGCAT_Server;


    for (i=0; i<16; i++)
        r[i] = NULL;

    if (msgid == NULL || !*msgid)
        msgid = "*";
    if (level < ISM_LOGLEV_CRIT || level > ISM_LOGLEV_INFO)
        level = ISM_LOGLEV_WARN;
    if (category) {
        if (!strcmpi(category, "connection"))
            cat = LOGCAT_Connection;
        else if (!strcmpi(category, "admin"))
            cat = LOGCAT_Admin;
        else if (!strcmpi(category, "security"))
            cat = LOGCAT_Security;
    }
    if (filen == NULL)
        filen = "java";
    if (msgformat == NULL)
        return 0;

    /*
     * Convert the replacement data
     */
    types[0] = 0;
    if (replf->type == VT_Map) {
        concat_alloc_t map = {replf->val.s, replf->len, replf->len};
        for (i=0; i<16; i++) {
            if (ism_findPropertyNameIndex(&map, i, &field))
                break;
            strcat(types, field.type == VT_String ? "%-s" : "%s");
            r[i] = bufp;
            switch (field.type) {
            case VT_String:    r[i] = field.val.s;                    break;
            case VT_Boolean:   r[i] = field.val.i ? "true" : "false"; break;
            case VT_Byte:
            case VT_Short:
            case VT_Integer:   sprintf(bufp, "%d", field.val.i);      break;
            case VT_UByte:
            case VT_Char:
            case VT_UShort:
            case VT_UInt:      sprintf(bufp, "%u", field.val.i);      break;
            case VT_Long:      sprintf(bufp, "%ld", field.val.l);     break;
            case VT_ULong:     sprintf(bufp, "%lu", field.val.l);     break;
            case VT_Float:     sprintf(bufp, "%g", field.val.f);      break;
            case VT_Double:    sprintf(bufp, "%g", field.val.d);      break;
            default:           r[i] = NULL;                           break;
            }
            if (r[i] == bufp)
                bufp += strlen(bufp) + 1;
        }
    }
    /*
     * Do the log
     */
    ism_common_logInvoke(NULL, level, 0, msgid, cat, ism_defaultTrace, "log",
            filen, lineno, types, msgformat, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7],
            r[8], r[9], r[10], r[11], r[12], r[13], r[14], r[15]);
    return 0;
}



/*
 * Accept or reject an incoming physical connection.
 * If the protocol is NULL the connection is not accepted
 */
static int pluginAcceptConnection(ism_plugin_act_t * action, const char * protocol, const char * protocol_family, const char * plugin_name) {
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    transport->protocol = ism_transport_putString(transport, protocol);
    transport->protocol_family = ism_transport_putString(transport, protocol_family);
    pobj->plugin_name = ism_transport_putString(transport, protocol_family);
    /* Check if allowed */
    ism_transport_allowConnection(transport);
    return 0;
}

/*
 *
 */
static void pluginStealCallback(int32_t reason, ismEngine_ClientStateHandle_t hClient, uint32_t options, void *pContext) {
    /* TODO: If reason is not ISMRC_ClientIDReused, need to produce a different message */
    ism_transport_t * transport = (ism_transport_t *) pContext;
    transport->close(transport, ISMRC_ClientIDReused, 0, "The connection is closed because the ClientID is used in another connection");
}

/*
 * Reply from asynchronous authentication.
 * This is run as a timer job so that it runs in a thread with the store initialized.
 */
static int replyAuthTT(ism_timer_t timer, ism_time_t timestamp, void * callbackParam) {
    ism_plugin_act_t * action = (ism_plugin_act_t *) callbackParam;
    int authrc = action->rc;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    int rc = 0;
    ismEngine_ClientStateHandle_t client;

    if (timer)
        ism_common_cancelTimer(timer);
    if (authrc != ISMRC_OK) {
        /*Failed to authenticate. Set the error code*/
        if (authrc != ISMRC_Closed)
            action->rc = ISMRC_NotAuthorized;
        if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
            ism_plugin_replyCloseClient(transport);
        }
        /* Send a reply to the plug-in for unauthorized connection .*/
        pluginReplyConnect(action);
        return 0;
    } else {
        TRACEL(8, transport->trclevel, "User is authenticated and authorized: connect=%u user=%s\n", transport->index, transport->userid);
    }

    /* Set max TCP buffer based on expected message rate */
    ism_protocol_setSocketBuffer(transport);

    action->paction = Action_createConnection;
    ism_common_setError(0);
    rc = ism_engine_createClientState(transport->clientID, PROTOCOL_ID_PLUGIN, (uint32_t)action->options, transport,
            pluginStealCallback, transport->security_context, &client,
            action, sizeof(*action), replyAction);

    if (rc != ISMRC_AsyncCompletion) {
        replyAction(rc, client, action);
    }
    if (timer)
        ism_common_free(ism_memory_protocol_misc,action);
    return 0;
}

/*
 * Reply from asynchronous authorization request.
 * This is commonly called in one of the security worker threads, but move further
 * processing to a timer thread as the store is initialized there.
 */
static void replyAuth(int authrc, void * callbackParam) {
    ism_plugin_act_t * action = (ism_plugin_act_t *) callbackParam;
    if (__builtin_expect(plugin_unit_test, 0)) {
        ism_transport_t * transport = action->transport;
        ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
        /* -1 was already reached, so closing is in progress already */
        if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
            __sync_fetch_and_sub(&pobj->inprogress, 1);
            return;
        }
        action->rc = authrc;
        replyAuthTT(NULL, 0, action);
    } else {
        ism_transport_t * transport = action->transport;
        ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
        /* -1 was already reached, so closing is in progress already */
        if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
            __sync_fetch_and_sub(&pobj->inprogress, 1);
            return;
        }
        action = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,41),sizeof(*action));
        memcpy(action, callbackParam, sizeof(*action));
        action->rc = authrc;
        ism_common_setTimerOnce(ISM_TIMER_HIGH, replyAuthTT, action, 1);
    }
}

/*
 * Base62 is used for the random part of a generated clientID.
 */
static char base62 [62] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
    'W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l',
    'm','n','o','p','q','r','s','t','u','v','w','x','y','z',
};

/*
 * Generate a client ID.
 */
static int generate_cid(ism_transport_t * transport, char * buf) {
    uint64_t rval;
    uint8_t * randbuf = (uint8_t *)&rval;
    char * bp = buf + 1;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    RAND_pseudo_bytes(randbuf, 8);
#else
    RAND_bytes(randbuf, 8);
#endif
    buf[0] = '_';
    if (transport->client_addr && *transport->client_addr)
        strcpy(bp, transport->client_addr);
    else
        strcpy(bp, transport->protocol);
    bp += strlen(bp);
    *bp++ = '_';
    for (int i=0; i<8; i++) {
        *bp++ = base62[(int)(rval%62)];
        rval /= 62;
    }
    *bp++ = 0;
    return bp-buf;
}

/*
 * Authenticate a connection.
 * This is called when we have a clientID and user.
 */
int ism_plugin_identify(ism_plugin_act_t * action, int auth, int32_t keepAlive, int32_t maxMsgInFlight, ism_field_t * props) {
    int  rc;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    action->paction = Action_createConnection;
    const char * password = NULL;
    ism_field_t field;
    transport->ready = 1;       /* We no longer need the DoS timer  */
    int sameuid = 0;
    action->options = ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE;
    if (props->type == VT_Map) {
        concat_alloc_t map = {props->val.s, props->len, props->len};
#ifndef DEBUG
        if(SHOULD_TRACE(6)) {
            ism_prop_t * dbgProps = ism_common_newProperties(128);
            if(ism_protocol_deserializeProperties(&map, dbgProps)) {
                TRACEL(6, transport->trclevel, "Authentication: failed to deserialize connection properties: connection=%u\n", transport->index);
            } else {
                int i;
                char buf[8192];
                for(i = 0; i < ism_common_getPropertyCount(dbgProps); i++) {
                    const char * propName = NULL;
                    ism_field_t  fld = {0};
                    ism_common_getPropertyIndex(dbgProps, i, &propName);
                    ism_common_getProperty(dbgProps, propName, &fld);
                    switch(fld.type) {
                    case VT_String:
                    case VT_Name:
                        sprintf(buf," %s=%s", propName, ((fld.val.s)? fld.val.s : "null"));
                        break;
                    case VT_Boolean:
                        sprintf(buf," %s=%s", propName, ((fld.val.i)? "true" : "false"));
                        break;
                    case VT_Byte:
                    case VT_Char:
                    case VT_Short:
                    case VT_Integer:
                    case VT_NameIndex:
                        sprintf(buf," %s=%d", propName, (int32_t) fld.val.i);
                        break;
                    case VT_Long:
                        sprintf(buf," %s=%lld", propName, (long long int) fld.val.l);
                        break;
                    case VT_UByte:
                    case VT_UShort:
                    case VT_UInt:
                        sprintf(buf," %s=%u", propName, (uint32_t) fld.val.i);
                        break;
                    case VT_ULong:
                        sprintf(buf," %s=%llu", propName, (long long unsigned int)fld.val.l);
                        break;
                    case VT_Null:
                        sprintf(buf," %s=NULL", propName);
                        break;
                    case VT_Float:
                        sprintf(buf," %s=%f", propName, fld.val.f);
                        break;
                    case VT_Double:
                        sprintf(buf," %s=%g", propName, fld.val.d);
                        break;
                    case VT_ByteArray:
                        sprintf(buf," %s=VT_ByteArray", propName);
                        break;
                    case VT_Map:
                        sprintf(buf," %s=VT_Map", propName);
                        break;
                    case VT_Unset:
                        sprintf(buf," %s=VT_Unset", propName);
                        break;
                    case VT_Xid:
                        sprintf(buf," %s=VT_Xid", propName);
                        break;
                    default:
                        break;
                    }
                }
                TRACEL(6, transport->trclevel, "Authentication: Connection properties for connection=%u are: %s\n", transport->index, buf);
            }
            ism_common_freeProperties(dbgProps);
        }
#endif
        ism_findPropertyName(&map, "ClientID", &field);
        if (field.type != VT_String) {
            field.val.s = "";
        }
        const char * clientid = field.val.s;
        /*
         * Reserve all clientIDs starting with a double underscore for internal users.
         */
        if (clientid[0]=='_') {
            if (clientid[1]=='_' && transport->receive) {
                ism_common_setError(ISMRC_ClientIDInUse);
                transport->close(transport, ISMRC_ClientIDInUse, 0, "The client ID is in use");
            } else if (strchr(clientid+2, '_')) {
                transport->pobj->isGenerated = 1;
            }
        }
        /* Generate a clientID */
        if (clientid[0] == 0) {
            char cidbuf[32];
            generate_cid(transport, cidbuf);
            field.val.s = cidbuf;
            transport->pobj->isGenerated = 0;
        }
        transport->clientID = ism_transport_putString(transport, field.val.s);
        transport->name = transport->clientID;

        ism_findPropertyName(&map, "User", &field);
        if (field.type == VT_String) {
            if (transport->userid && field.val.s && !strcmp(transport->userid, field.val.s)) {
                sameuid = 1;
            } else {
                transport->userid = ism_transport_putString(transport, field.val.s);
            }
        }
        ism_findPropertyName(&map, "Password", &field);
        if (field.type == VT_String) {
            password = ism_transport_putString(transport, field.val.s);
        } else {
            if (sameuid && transport->protocol && *transport->protocol=='/') {
                /* Un-obfuscate the password */
                char * pw;
                password = (char *)transport->userid + strlen(transport->userid) + 1;
                pw = (char *)password;
                while (*pw) {
                    *pw++ ^= 0xfd;
                }
                sameuid = 2;
            }
        }
        ism_findPropertyName(&map, "CommonName", &field);
        if (field.type == VT_String) {
            transport->cert_name = ism_transport_putString(transport, field.val.s);
        }
        ism_findPropertyName(&map, "AllowSteal", &field);
        if ((field.type == VT_Boolean) && field.val.i) {
            action->options = ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL;
        }
        ism_findPropertyName(&map, "Durable", &field);
        if ((field.type == VT_Boolean) && field.val.i) {
            action->options |= ismENGINE_CREATE_CLIENT_OPTION_DURABLE;
        }
        ism_findPropertyName(&map, "TransactionCapable", &field);
        if ((field.type == VT_Boolean) && field.val.i) {
            action->hdrcount = 1;
        } else {
            action->hdrcount = 0;
        }
    }
    pobj->maxMsgInFlight = maxMsgInFlight;

    /*
     * Check if connection is allowed at this time
     */
    ism_common_setError(0);
    rc = ism_transport_clientAllowed(transport);
    if (rc) {
        ism_common_setError(rc);
        action->rc = rc;
        if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
            ism_plugin_replyCloseClient(transport);
        }
        /* Send a reply to the plug-in for unauthorized connection. */
        pluginReplyConnect(action);
    }

    /*
     * Call asynchronous authentication if the usePasswordAuth is true or the username is specified
     * for non-TLSL connections, but not if no user check is requested.
     */
    int authnRequired = transport->listener->usePasswordAuth != 0;
    if (!transport->secure && transport->userid && auth != 1) {
        authnRequired = 1;
    }

    pluginAddToClientsList(pobj,keepAlive);

    action->rc = 0;
    TRACEL(7, transport->trclevel, "Authentication: submit plugin async authentication: connect=%u client=%s user=%s required=%u\n" ,
        transport->index, transport->name, transport->userid, authnRequired);
    ism_security_authenticate_user_async(transport->security_context,
        transport->userid, transport->userid ? strlen(transport->userid) : 0,
        password, password ? strlen(password) : 0,
        action, sizeof(*action), replyAuth, authnRequired, 0);
    /* Re-obfuscate the password */
    if (sameuid == 2) {
        char * pw;
        pw = (char *)password;
        while (*pw) {
            *pw++ ^= 0xfd;
        }
    }

    /* Create client state */
    return 0;
}

/*
 * create
 */
static void pluginCreateDurableConsumer(int32_t rc, void * handle, void * vaction) {
    ism_plugin_act_t * action = (ism_plugin_act_t *) vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    ism_plugin_cons_t * consumer = action->consumer;
    int consumerOpt = ismENGINE_CONSUMER_OPTION_PAUSE;
    ismEngine_ConsumerHandle_t consumerh = NULL;
    action->rc = rc;
    if (rc) {
        replyAction(rc, NULL, action);
        return;
    }
    if (consumer->qos)
        consumerOpt |= ismENGINE_CONSUMER_OPTION_ACK;
    rc = ism_engine_createConsumer(pobj->session_handle, ismDESTINATION_SUBSCRIPTION,
            consumer->name, NULL, pobj->client_handle, consumer, sizeof(ism_plugin_cons_t), replyMessage,
            NULL, consumerOpt, &consumerh, action, sizeof(ism_plugin_act_t), replyAction);
    if (rc == ISMRC_AsyncCompletion) {
        return;
    }
    replyAction(rc, consumerh, action);
}


/*
 *
 */
static void recreateSubscription(int32_t rc, void * handle, void * vaction) {
    ism_plugin_act_t * action = (ism_plugin_act_t *) vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    ism_plugin_cons_t * consumer = action->consumer;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
    action->rc = rc;
    if (rc) {
        replyAction(rc, NULL, action);
        return;
    }
    subAttrs.subOptions += consumer->qos;
    subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
    if (action->hdrcount)
    	subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    rc = ism_engine_createSubscription(pobj->client_handle, consumer->name, NULL, ismDESTINATION_TOPIC,
            consumer->dest, &subAttrs, NULL, action, sizeof(ism_plugin_act_t), pluginCreateDurableConsumer);
    if (rc != ISMRC_AsyncCompletion) {
        pluginCreateDurableConsumer(rc,NULL,action);
    }
}


/*
 *
 */
static void pluginReSubscribe(ismEngine_SubscriptionHandle_t subHandle,
        const char * oldSubName, const char * oldTopicName,
        void * xproperties, size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes, uint32_t consumerCount, void * vaction) {
    ism_plugin_act_t * action = (ism_plugin_act_t *) vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    ism_plugin_cons_t * consumer = action->consumer;
    const char * subName = consumer->name;
    const char * topicName = consumer->dest;
    int rc = 0;
    if (strcmp(oldSubName, subName))
        return;
    if (strcmp(oldTopicName, topicName)) {
        if (consumerCount > 0) {
            action->options = SUB_Error;
            action->rc = ISMRC_DestinationInUse;
            ism_common_setError(action->rc);
            return;
        }
        action->options = SUB_Resubscribe;
        rc = ism_engine_destroySubscription(pobj->client_handle, subName, pobj->client_handle, action,
                        sizeof(ism_plugin_act_t), recreateSubscription);
        if (rc != ISMRC_AsyncCompletion)
            recreateSubscription(rc,NULL, action);
        return;
    }
    action->options = SUB_Found;
    pluginCreateDurableConsumer(0,NULL,action);
}

/*
 * Subscribe
 * flags: bit0=queue, bit1,2=qos
 */
int ism_plugin_subscribe(ism_plugin_act_t * action, int flags, int share, int transacted, const char * dest, const char * subname, const char * selector) {
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    ismEngine_ConsumerHandle_t consumerh;
    int desttype = (flags&0x20) ? ismDESTINATION_QUEUE : ismDESTINATION_TOPIC;
    ism_plugin_cons_t * consumer;
    int  rc;

    ism_common_setError(0);
    if (!dest || strlen(dest) == 0) {
        ism_common_setError(ISMRC_NoDestination);
        replyComplete(action, ISMRC_NoDestination);
        return 1;
    }

    /* Durable subscription to system topics are not allowed */
    if (share && dest[0]=='$') {
        ism_common_setError(ISMRC_BadSysTopic);
        replyComplete(action, ISMRC_BadSysTopic);
        return 1;
    }

    /* Check the subscription name */
    if (!subname) {
        ism_common_setError(ISMRC_NameNotValid);
        replyComplete(action, ISMRC_NameNotValid);
        return 1;
    }

    if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
      __sync_fetch_and_sub(&pobj->inprogress, 1);
      ism_common_setError(ISMRC_Closed);
      return ISMRC_Closed;
    }

    action->paction = Action_createConsumer;

    if ((pobj->client_handle == NULL) || (pobj->session_handle == NULL)) {
        replyAction(ISMRC_Closed, NULL, action);
        return ISMRC_Closed;
    }
    if (share > 1) {
        replyAction(ISMRC_BadClientData, NULL, action);
        return ISMRC_BadClientData;
    }

    consumer = findConsumer(transport);
    if (!consumer) {
        ism_common_setError(ISMRC_AllocateError);
        replyAction(ISMRC_AllocateError, NULL, action);
        return ISMRC_AllocateError;
    }

    consumer->dest  = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),dest);
    consumer->name  = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),subname);
    consumer->desttype = (uint8_t)desttype;
    consumer->qos      = (flags>>2)&3;
    consumer->chandle = NULL;
    action->consumer = consumer;

    if ((share == 0) || (desttype == ismDESTINATION_QUEUE)) {
        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + consumer->qos };
        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
        int consumerOpt = ismENGINE_CONSUMER_OPTION_PAUSE;
        if (consumer->qos)
            consumerOpt |= ismENGINE_CONSUMER_OPTION_ACK;
        if (transacted)
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
        rc = ism_engine_createConsumer(pobj->session_handle,
                desttype, dest, &subAttrs, NULL,
                consumer, sizeof(ism_plugin_cons_t), replyMessage, NULL,
                consumerOpt, &consumerh, action, sizeof(ism_plugin_act_t), replyAction);

        if (rc == ISMRC_AsyncCompletion) {
            return 0;
        }
        replyAction(rc, consumerh, action);
        return 0;
    }
    if (share == 1) { /* Durable not shared */
        action->options = SUB_NotFound;
        if (transacted)
        	action->hdrcount = 1;
        else
        	action->hdrcount = 0;
        rc = ism_engine_listSubscriptions(pobj->client_handle, consumer->name, action, pluginReSubscribe);
        if (rc) {
            replyAction(rc, NULL, action);
            return 0;
        }
        if (action->options == SUB_Error) {
            replyAction(action->rc, NULL, action);
            return 0;
        }
        if (action->options == SUB_NotFound) {
            recreateSubscription(0, NULL, action);
            return 0;
        }
    }

#if 0
    /* Compile selector if requested */
    if (selector) {
        rulelen = 0;
        rc = ism_common_compileSelectRule(&action->rule, &rulelen, action->values[1].val.s);
        action->rulelen = rulelen;
        if (rc) {
            replyAction(rc, NULL, action);
            break;
        }
    }

    void * clientState = pobj->handle;
    switch (shared) {
    case 4:  clientState = client_Shared;    break;
    case 6:  clientState = client_SharedND;  break;
    }
    if (action->shared&6) {
        // action->nolocal = 0;
    }


    rc = ism_engine_listSubscriptions(clientState, (char *)subName, action, forSubscription);
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
#endif
    return 0;
}

/*
 * Close a subscription
 */
int ism_plugin_closeSub(ism_plugin_act_t * action, const char * subname, int share) {
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;
    if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
      __sync_fetch_and_sub(&pobj->inprogress, 1);
      ism_common_setError(ISMRC_Closed);
      return ISMRC_Closed;
    }
    ism_plugin_cons_t   * consumer = findConsumerByName(transport, subname);
    ism_common_setError(0);
    int rc = ISMRC_Closed;
    action->paction = Action_closeConsumer;
    action->consumer = consumer;
    if (consumer) {
        void * handle = __sync_val_compare_and_swap(&consumer->chandle, consumer->chandle, NULL);
        if (handle) {
            rc = ism_engine_destroyConsumer(handle, action, sizeof(ism_plugin_act_t), replyAction);
            if (rc == ISMRC_AsyncCompletion)
                return 0;
        }
    }
    replyAction(rc, NULL, action);
    return 0;
}

/*
 * Check if a subscription is in use
 */
static void pluginCheckUnsub(ismEngine_SubscriptionHandle_t subHandle, const char * pSubName, const char *pTopicString,
        void * properties, size_t propertiesLength, const ismEngine_SubscriptionAttributes_t *pSubAttributes, uint32_t consumerCount, void * vaction) {
    ism_plugin_act_t * action = vaction;
    if (pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE)
        action->options = (consumerCount) ? 1 : 0;
}


/*
 * Destroy a subscription
 */
int ism_plugin_destroySub(ism_plugin_act_t * action, const char * subname, int share) {
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;
    if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
      __sync_fetch_and_sub(&pobj->inprogress, 1);
      ism_common_setError(ISMRC_Closed);
      return ISMRC_Closed;
    }
    action->options = 0xff;
    action->paction = Action_unsubscribeDurable;
    xUNUSED int zrc = ism_engine_listSubscriptions(pobj->client_handle, subname, action, pluginCheckUnsub);
    if (action->options != 0) {
        TRACEL(4, transport->trclevel, "Unable to destroy durable subscription: connection=%u client=%s name=%s count=%d\n", transport->index,
                transport->name, subname, (int32_t)action->options);
        int rc = (action->options == 0xff) ? ISMRC_NotFound : ISMRC_DestinationInUse;
        ism_common_setError(rc);
        replyAction(rc, NULL, action);
        return rc;
    }
    ism_common_setError(0);
    int rc = ism_engine_destroySubscription(pobj->client_handle, subname, pobj->client_handle, action, sizeof(ism_plugin_act_t), replyAction);
    if (rc == ISMRC_AsyncCompletion) {
        return 0;
    }
    replyAction(rc, NULL, action);
    return rc;
}

/*
 * We always use two message areas, one for properties and the other for payload
 */
static ismMessageAreaType_t MsgAreas[2] = {
    ismMESSAGE_AREA_PROPERTIES,
    ismMESSAGE_AREA_PAYLOAD
};

/*
 * Reply to a publish
 */
static void replyPublish(int32_t rc, void * handle, void * vaction) {
    ism_plugin_act_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;
    if (rc == ISMRC_SomeDestinationsFull) {
        transport->listener->stats->count[transport->tid].read_msg++;
        transport->listener->stats->count[transport->tid].warn_msg++;
        /* ISMRC_SomeDestinationsFull provides a warning statistic for
         * the server.  From the client perspective, the request has
         * succeeded. So change the rc to 0 now that the stat has been
         * recorded.
         */
        rc = 0;
    } else if (rc == ISMRC_NoMatchingDestinations || rc == ISMRC_NoMatchingLocalDestinations) {
        transport->listener->stats->count[transport->tid].read_msg++;
        /* Informational return codes */
        rc = 0;
    }
    replyComplete(action, rc);
    /* If close is in progress, proceed with it */
    if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
        ism_plugin_replyCloseClient(transport);
    }
}

/*
 * Get a HTTP valid date
 */
static void http_time(char * date) {
    struct tm tm;
    time_t timex;
    static char * days [] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static char * months [] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    time(&timex);
    gmtime_r(&timex, &tm);
    sprintf(date, "%s, %02u %s %04u %02u:%02u:%02u GMT",
            days[tm.tm_wday], tm.tm_mday, months[tm.tm_mon],
            (tm.tm_year+1900), tm.tm_hour, tm.tm_min, tm.tm_sec);
}

#define HTTP_RESPONSE_ERROR "\
HTTP/1.1 %d %s\r\n\
%s\
Date: %s\r\n\
Access-Control-Allow-Origin: %s\r\n\
Access-Control-Allow-Credentials: true\r\n\
Connection: %s\r\n\
Cache-Control: %s\r\n\
Content-Type: %s\r\n\
Content-Length: %d\r\n\r\n"

#define HTTP_SEND_DATA "\
HTTP/1.1 200 OK\r\n\
%s\
Date: %s\r\n\
Access-Control-Allow-Origin: %s\r\n\
Access-Control-Allow-Credentials: true\r\n\
Connection: %s\r\n\
Cache-Control: %s\r\n\
Content-Type: %s\r\n\
Content-Length: %d\r\n"


#define HTTP_NOT_AVAIL "\
HTTP/1.1 503 Service Unavailable\r\n\
%s\
Date: %s\r\n\
Connection: close\r\n\
Content-Type: text/plain;charset=utf-8\r\n\
Content-Length: %d\r\n\r\n%s\r\n"

#define HTTP_Opt_Close    1
#define HTTP_Opt_ReadMsg  2
#define HTTP_Opt_WriteMsg 4
/*
 * Send a message
 */
int ism_plugin_sendHttp(ism_plugin_act_t * action, int rc, const char * content_type, ism_field_t * props, ism_field_t * body) {
    ism_transport_t * transport = action->transport;
    const char * error_str;
    char date [32];
    char xbuf [2048];
    const char * expose_hdr[32];
    const char * origin;
    int    expose_cnt = 0;
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    concat_alloc_t map = {0};
    int bodylen = 0;
    ism_field_t field;
    int   i;
    int   options = 0;
    int sendRC = 0;
    const char * disp;

    /* Process options */
    if (action->hdrcount > 3 && action->hdrs[3].type == VT_Integer) {
        options = action->hdrs[3].val.i;
        if (options & HTTP_Opt_Close) {
            transport->at_server = 2;    /* Force close */
        }
        if (options & HTTP_Opt_ReadMsg) {
            transport->listener->stats->count[transport->tid].read_msg++;
            transport->read_msg++;
        }
        if (options & HTTP_Opt_WriteMsg) {
            transport->listener->stats->count[transport->tid].write_msg++;
            transport->write_msg++;
        }
    }

    /*
     * Get properties if any
     */
    if (props) {
        map.buf = props->val.s;
        map.len = props->len;
        map.used = props->len;
    }

    /*
     * We only allow some HTTP errors, get the known text for them
     */
    switch (rc) {
    case 200:  error_str = "OK";                       break;
    case 201:  error_str = "Created";                  break;
    case 202:  error_str = "Accepted";                 break;
    case 203:  error_str = "Non-authoritative";        break;
    case 204:  error_str = "No content";               break;
    case 205:  error_str = "Content reset";            break;
    case 400:  error_str = "Bad request";              break;
    case 403:  error_str = "Forbidden";                break;
    case 404:  error_str = "Not found";                break;
    case 406:  error_str = "Not acceptable";           break;
    case 413:  error_str = "Request entity too large"; break;
    case 415:  error_str = "Unsupported media type";   break;
    case 501:  error_str = "Not implemented";          break;
    case 503:  error_str = "Service unavailable";      break;
    default:
        rc = 404;
        error_str = "Not found";
    }
    if (rc >= 300)
        transport->at_server = 2;

    http_time(date);

    if (body && body->type == VT_ByteArray && rc != 204)
        bodylen = body->len;

    if (content_type==NULL || !*content_type || strlen(content_type)>1024) {
        content_type = "text/plain;charset=utf-8";
        if (bodylen > 0 && (body->val.s[0]=='{' || body->val.s[0]=='['))
            content_type = "application/json";
    }

    origin = transport->origin;
    if (!origin)
        origin = "*";
    disp = transport->at_server==2 ? "close" : "keep-alive\r\nKeep-Alive: timeout=60";

    /*
     * Process error
     */
    if (rc != 200) {
        sprintf(xbuf, HTTP_RESPONSE_ERROR, rc, error_str, getServerHTTPHeaderString(),
                date, origin, disp, "no-cache", content_type, bodylen);
        buf.used = strlen(xbuf);
        if (bodylen)
            ism_common_allocBufferCopyLen(&buf, body->val.s, bodylen);
        if (SHOULD_TRACEC(9, Http)) {
            ism_common_allocBufferCopyLen(&buf, "", 1);
            buf.used--;
            TRACEX(9, Http, 0, "httpout connect=%u: [\n%s]\n", transport->index, buf.buf);
        }
        transport->send(transport, buf.buf, buf.used, 0, 0);
    }

    /* Send data */
    else {
        sprintf(xbuf, HTTP_SEND_DATA, getServerHTTPHeaderString(), date, origin,
                disp, "no-cache", content_type, bodylen);
        buf.used = strlen(xbuf);
        map.pos = 0;
        while (map.pos < map.used) {
            rc = ism_protocol_getObjectValue(&map, &field);
            if (!rc && field.type == VT_Name) {
                const char * name = field.val.s;
                ism_protocol_getObjectValue(&map, &field);
                if (field.type == VT_String) {
                    if (*name == ']') {
                        ism_common_allocBufferCopyLen(&buf, &name[1], strlen(name) - 1);
                        ism_common_allocBufferCopyLen(&buf, ": ", 2);
                        ism_common_allocBufferCopyLen(&buf, field.val.s, strlen(field.val.s));
                        ism_common_allocBufferCopyLen(&buf, "\r\n", 2);
                        if (expose_cnt < 32)
                            expose_hdr[expose_cnt++] = &name[1];
                    }
                }
            }
        }

        /* If there are custom headers, fill in the CORS header   */
        if (expose_cnt > 0) {
            ism_common_allocBufferCopy(&buf, "Access-Control-Expose-Headers: ");
            buf.used--;
            for (i = 0; i < expose_cnt; i++) {
                if (i != 0)
                    ism_common_allocBufferCopyLen(&buf, ", ", 2);
                ism_common_allocBufferCopyLen(&buf, expose_hdr[i], strlen(expose_hdr[i]));
            }
            ism_common_allocBufferCopyLen(&buf, "\r\n", 2);
        }

        /* Copy in the content.  Use a single send for small content */
        ism_common_allocBufferCopyLen(&buf, "\r\n", 2);    /* End of header */
        if (bodylen < 1024) {
            if (bodylen)
                ism_common_allocBufferCopyLen(&buf, body->val.s, body->len);
            sendRC = transport->send(transport, buf.buf, buf.used, 0, 0);
        } else {
            transport->send(transport, buf.buf, buf.used, 0, 0);
            sendRC = transport->send(transport, body->val.s, body->len, 0, 0);
        }
        if (SHOULD_TRACEC(9, Http)) {
            ism_common_allocBufferCopyLen(&buf, "", 1);
            buf.used--;
            TRACEX(9, Http, 0, "httpout connect=%u: [\n%s]\n", transport->index, buf.buf);
        }
    }
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
    /*
     * If either the client or the plug-in asked to close the connection, do so now
     */
    if (transport->at_server == 2) {
        transport->close(transport, 0, 1, "HTTP connection close");
    } else {
        transport->at_server = 0;
        if (sendRC == SRETURN_SUSPEND) {
            pluginSuspendDelivery(action);
        }
    }
    return 0;
}


/*
 * Send a not available response
 */
static void not_available(ism_transport_t * transport) {
    char date [32];
    char xbuf[500];
    const char * str = "Unable to connect to plug-in server";
    http_time(date);
    sprintf(xbuf, HTTP_NOT_AVAIL, getServerHTTPHeaderString(),
            date, (int)strlen(str), str);
    transport->send(transport, xbuf, (int)strlen(xbuf), 0, 0);
}

/*
 * Handle HTTP data
 * In the case of HTTP data the frame has set up the data for us as needed to
 * send on to the plug-in server.
 */
int ism_plugin_receiveHttpData(ism_transport_t * transport, char * databuf, int buflen, int command) {
    /* TODO: handle incomplete data */
    ism_transport_t * channel = ism_plugin_getChannelTransport(transport->tid);
    if (channel) {
        channel->send(channel, databuf, buflen, (PluginAction_OnHttpData<<8)+4, 0);
        ism_plugin_freeChannelTransport(transport->tid);
        return 0;
    }
    not_available(transport);
    ism_plugin_closeConnection(transport, ISMRC_UnableToConnect, "Unable to connect to plug-in server");
    return -1;
}

/*
 * Send a message
 */
int ism_plugin_message(ism_plugin_act_t * action, int msgtype, int flags, const char * dest, int64_t handle, ism_field_t * props, ism_field_t * body) {
    int  rc = 0;
    ismEngine_MessageHandle_t msgh;
    ismMessageHeader_t hdr;
    size_t areasize[2];
    void * areaptr[2];
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;

    /* If we are in the process of closing, return closed */
    if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
      __sync_fetch_and_sub(&pobj->inprogress, 1);
      ism_common_setError(ISMRC_Closed);
      return ISMRC_Closed;
    }

    /*
     * Set up the header
     */
    memset(&hdr, 0, sizeof hdr);
    hdr.Persistence = (flags & 0x04) ? ismMESSAGE_PERSISTENCE_PERSISTENT : ismMESSAGE_PERSISTENCE_NONPERSISTENT;
    hdr.Reliability = flags&3;
    hdr.Priority = 4;
    hdr.RedeliveryCount = 0;
    hdr.Expiry = 0;
    if (flags & 0x18) {
        hdr.Flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;
        hdr.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    } else {
        hdr.Flags = 0;
    }
    hdr.MessageType = msgtype;

    /* Set the properties and body area */
    areasize[0] = props->type == VT_Map ? props->len : 0;
    areaptr[0] = props->type == VT_Map ? props->val.s : NULL;
    areasize[1] = body->type == VT_ByteArray ? body->len : 0;
    areaptr[1] = body->type == VT_ByteArray ? body->val.s : NULL;

    /*
     * Create the message
     */
    ism_common_setError(0);
    if (transport->pobj->session_handle) {
        rc = ism_engine_createMessage(&hdr, 2, MsgAreas, areasize, areaptr, &msgh);
    } else {
        rc = ISMRC_ObjectNotValid;
        ism_common_setError(rc);
    }

    if (rc) {
        /* Engine reported error when creating message, set an error and ACK */
        replyComplete(action, rc);
        /* If close is in progress, proceed with it */
        if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
           ism_plugin_replyCloseClient(transport);
        }
        return 1;
    }

    if (!dest && props->type == VT_Map) {
        ism_field_t f;
        concat_alloc_t pbuf = {props->val.s, props->len, props->len};
        ism_findPropertyNameIndex(&pbuf, ID_Topic, &f);
        if (f.type == VT_String)
            dest = f.val.s;
    }

    transport->read_msg++;

    if (transport->pobj->session_handle && dest) {     /* BEAM suppression: constant condition */
        int destType = (flags & 0x20) ? ismDESTINATION_QUEUE : ismDESTINATION_TOPIC;
        if (action->seqnum) {
            action->paction = Action_message;
            rc = ism_engine_putMessageOnDestination(transport->pobj->session_handle,
                destType , dest, (void*)((uintptr_t)handle), msgh,
                action, sizeof(ism_plugin_act_t), replyPublish);

            if (rc != ISMRC_AsyncCompletion) {
                replyPublish(rc, NULL, action);
            }
        } else {
            rc = ism_engine_putMessageOnDestination(transport->pobj->session_handle,
                    destType, dest, (void*)((uintptr_t)handle), msgh, NULL, 0, NULL);
            //TODO: Handle error (make sure to ignore informational return codes)
            /* If close is in progress, proceed with it */
            if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
                ism_plugin_replyCloseClient(transport);
            }
        }
        if (!rc) {
            transport->listener->stats->count[transport->tid].read_msg++;
        } else {
            if (rc == ISMRC_SomeDestinationsFull) {
                transport->listener->stats->count[transport->tid].read_msg++;
                transport->listener->stats->count[transport->tid].warn_msg++;
                /* ISMRC_SomeDestinationsFull provides a warning statistic for
                 * the server.  From the client perspective, the request has
                 * succeeded. So change the rc to 0 now that the stat has been
                 * recorded.
                 */
                rc = 0;
            } else if (rc == ISMRC_NoMatchingDestinations || rc == ISMRC_NoMatchingLocalDestinations) {
                transport->listener->stats->count[transport->tid].read_msg++;
                /* Informational return codes */
                rc = 0;
            } else if (rc != ISMRC_AsyncCompletion) {
                transport->listener->stats->count[transport->tid].lost_msg++;
            }
        }

    } else {
        /* TODO */
        printf("no dest\n");
        transport->listener->stats->count[transport->tid].lost_msg++;
        /* If close is in progress, proceed with it */
        if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
           ism_plugin_replyCloseClient(transport);
        }
        return 1;
    }

    return 0;
}

/*
 * The engine connection is closed, close our connection, and tell the transport it can close its connection
 */
void ism_plugin_replyDoneConnection(int32_t rc, void * handle, void * vaction) {
    ism_plugin_act_t * act = (ism_plugin_act_t *) vaction;
    ism_transport_t * transport = act->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    ism_plugin_cons_t * consumer;

    int i;

    TRACEL(7, transport->trclevel, "close %s connection: connect=%u\n", transport->protocol, transport->index);

    /* Delete the connection artifacts */
    for (i = 0; i < pobj->consumer_alloc; i++) {
       consumer = pobj->consumers[i];
       if (consumer!=NULL) {
           if (consumer->dest!=NULL) {
               ism_common_free(ism_memory_protocol_misc,(void *) consumer->dest);
               consumer->dest = NULL;
           }
           if (consumer->name!=NULL) {
               ism_common_free(ism_memory_protocol_misc,(void *) consumer->name);
               consumer->name = NULL;
           }
           ism_common_free(ism_memory_protocol_misc,consumer);
       }
    }
    if (pobj->consumers) {
        ism_common_free(ism_memory_protocol_misc,pobj->consumers);
        pobj->consumers = NULL;
        pobj->consumer_alloc = 0;
    }

    if (pobj->errors) {
        ism_common_destroyHashMap(pobj->errors);
        pobj->errors = NULL;
    }

    pluginRemoveFromClientsList(pobj,1);
    pthread_spin_destroy(&pobj->lock);
    pthread_spin_destroy(&pobj->sessionlock);

    /* Tell the transport we are done */
    transport->closed(transport);
}


/*
 * Session closed.  If last one, close the connection
 */
void ism_plugin_replyCloseClient(ism_transport_t * transport) {
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    void * handle;
    ism_plugin_act_t act ={0};

    act.transport = transport;
    act.action = Action_closeConnection;

    ism_security_returnAuthHandle(transport->security_context);

    pthread_spin_lock(&pobj->sessionlock);
    /* Clean up pending jobs */
    while(pobj->jobsHead) {
        ism_plugin_job_t * job = pobj->jobsHead;
        pobj->jobsHead = job->next;
        ism_common_free(ism_memory_protocol_misc,job);
    }
    pobj->jobsTail = NULL;

    pobj->session_handle = 0;

    /* Ensure that we don't call ism_engine_destroyClientState twice. */
    handle = pobj->client_handle;
    pobj->client_handle = 0;

    pthread_spin_unlock(&pobj->sessionlock);

    if (handle) {
        int rc = ism_engine_destroyClientState(handle,
                ismENGINE_DESTROY_CLIENT_OPTION_NONE, &act, sizeof(act),
                ism_plugin_replyDoneConnection);
        if (rc == ISMRC_AsyncCompletion)
            return;
    }
    ism_plugin_replyDoneConnection(0, NULL, &act);
}

/*
 * Complete the closing of a connection
 */
static void completeConnectionClosing(ism_transport_t * transport) {
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    int count = __sync_sub_and_fetch(&pobj->inprogress, 1);
    if (count >= 0) {
        TRACEL(8, transport->trclevel, "ism_plugin_replyCloseClient postponed as there are %d actions/messages in progress: connect=%u client=%s",
                count+1, transport->index, transport->name);
        return;
    }
    ism_plugin_replyCloseClient(transport);
    return;
}

/*
 * Close a virtual or physical connection
 */
int ism_plugin_closeConnection(ism_transport_t * transport, int rc, const char * reason) {
    ism_protobj_t * pobj = transport->pobj;
    int closed = __sync_fetch_and_or(&pobj->closed, 4);
    if (!closed) {
        transport->at_server = 0;
        transport->close(transport, rc, (rc==0), reason);
        return 0;
    }
    if (closed == 3) {
        completeConnectionClosing(transport);
    }
    return 0;
}

/*
 * Structure for the close action
 */
typedef struct pi_close_action_t {
    ism_transport_t * transport;
    int  len;
    int  resv;
} pi_close_action_t;


/*
 * When the engine stopMessageDelivery returns, send the OnClose to the plugin
 */
static void replyStopped(int32_t rc, void * handle, void * vaction) {
    pi_close_action_t * action = (pi_close_action_t *)vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    int closed;
    if (action->len) {
        /* Send onClose action to the plugin */
        ism_transport_t * channel = ism_plugin_getChannelTransport(transport->tid);
        if (channel) {
            char * buf = (char *)(action+1);
            rc = channel->send(channel, buf+6, action->len-6, (PluginAction_OnClose<<8)+4, SFLAG_FRAMESPACE);
            ism_plugin_freeChannelTransport(transport->tid);
            if ((rc == 0) || (rc != SRETURN_SUSPEND)) {
                ism_common_free(ism_memory_protocol_misc,action);
                closed = __sync_fetch_and_or(&pobj->closed, 2);
                if (closed == 5)
                    completeConnectionClosing(transport);
                return;
            }

        }
    }
    ism_common_free(ism_memory_protocol_misc,action);
    /* Close was received from the plugin or send action failed */
    closed = __sync_or_and_fetch(&pobj->closed, 2);
    if (closed == 0x7) {
        completeConnectionClosing(transport);
    }
    return;
}

/*
 * The connection is closing.
 * Stop message delivery and then send the OnClose
 */
int ism_plugin_closing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    TRACEL(8, transport->trclevel, "ism_plugin_closing: connect=%u client=%s rc=%d clean=%d reason=%s\n",
            transport->index, transport->name, rc, clean, reason);

    /*
     * Set the indicator that close is in progress. If set failed,
     * then this has been done before and we don't need to proceed.
     */
    int closed = __sync_fetch_and_or(&pobj->closed, 1); /* Transport closed */
    if (closed & 1)
        return 0;
    pi_close_action_t * action = NULL;
    if (!(closed & 4)) {
        /*
         * Construct the OnClose action
         * Headers (4):  h0=connect h1=seqnum h2=rc, h3=reason
         */
        char xbuf[2048];
        concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
        ism_protocol_putIntValue(&buf, transport->monitor_id);
        ism_protocol_putIntValue(&buf, 1);   /* Connection related */
        ism_protocol_putIntValue(&buf, rc);
        ism_protocol_putStringValue(&buf, reason);

        /* Save the OnClose action to allow us to go async in the engine */
        action = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,49),sizeof(pi_close_action_t) + buf.used);
        action->len = buf.used;
        memcpy((char *)(action+1), buf.buf, buf.used);
    } else {
        action = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,50),1,sizeof(pi_close_action_t));
    }
    action->transport = transport;

    if (transport->pobj->session_handle) {
        /*
         * Stop message delivery
         */
        rc = ism_engine_stopMessageDelivery(transport->pobj->session_handle, action,
                sizeof(pi_close_action_t*), replyStopped);
        if (rc != ISMRC_AsyncCompletion)
            replyStopped(0, NULL, action);
    } else {
        replyStopped(0, NULL, action);
    }
    return 0;
}

/*
 * Process a new plugin connection
 *
 * plugin is accepted with multiple encodings and framing.
 */
int ism_plugin_connection(ism_transport_t * transport) {
    int rc = 1;
    char contype;
    ism_transport_t *channel = ism_plugin_getChannelTransport(transport->tid);
    if (channel) {
        ism_plugin_t * plugin = ism_plugin_findByWSProtocol(transport->protocol);
        if (plugin) {
            ism_protobj_t * pobj = (ism_protobj_t *) ism_transport_allocBytes(transport, sizeof(ism_protobj_t), 1);
            memset((char *) pobj, 0, sizeof(ism_protobj_t));
            transport->pobj = pobj;
            transport->protocol = ism_transport_putString(transport, transport->protocol);
            pobj->transport = transport;
            pobj->keepAlive = -1;
            TRACE(7, "ism_plugin_connection: connection=%u pobj=%p \n", transport->index, pobj);
            pthread_spin_init(&pobj->lock, 0);
            pthread_spin_init(&pobj->sessionlock, 0);
            if (*transport->protocol == '/') {
                transport->receive = ism_plugin_receiveHttpData;
                contype = 2;   /* HTTP */
                transport->www_auth = plugin->www_auth;   /* Only for http */
                ism_transport_setHeaderList(transport, plugin->httpheader_count, plugin->httpheader);
            } else {
                transport->receive = ism_plugin_receiveData;
                contype = 1;   /* WebSockets */
            }
            transport->resume = pluginResumeDelivery;
            transport->checkLiveness = pluginCheckLiveness;
            transport->closing = ism_plugin_closing;
            transport->delay_close = 20;         /* Delay processing of connection close from TCP */
            pluginAddToClientsList(pobj, 0);
            char xbuf[4098];
            concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
            if (!transport->monitor_id) {
                ism_transport_addMonitorNow(transport);
            }
            ism_protocol_putIntValue(&buf, transport->monitor_id);
            ism_protocol_putIntValue(&buf, 1);    /* Connection related seqnum */
            ism_protocol_putByteValue(&buf, contype);   /* Connection type 1 */
            makeConnectMap(&buf, transport);
            channel->send(channel, buf.buf+6, buf.used-6, (PluginAction_OnConnection<<8)+3, SFLAG_FRAMESPACE);
            if (buf.inheap)
                ism_common_freeAllocBuffer(&buf);
            rc = 0;
        }
        ism_plugin_freeChannelTransport(transport->tid);
    }

    return rc;
}


/*
 * Register work.
 * This is called with the transport lock held.
 *
 * This will assign a sequence number and place this job onto the work list for the specified connection.
 * The connection is normally the tid associated with the transport object.
 * The work count of the transport is incremented.
 */
xUNUSED static int registerWork(ism_plugin_job_t * job) {
    ism_transport_t * transport = job->transport;
    ismPluginPobj_t * pobj = transport->pobj;
    pthread_spin_lock(&pobj->sessionlock);
    job->prev = pobj->jobsTail;
    job->next = NULL;
    if (pobj->jobsTail) {
        pobj->jobsTail->next = job;
        pobj->seqnum++;
    } else {
        pobj->jobsHead = job;
        pobj->seqnum = 2;
    }
    pobj->jobsTail = job;
    job->seqnum = pobj->seqnum;
    pthread_spin_unlock(&pobj->sessionlock);
    return job->seqnum;
}

/*
 * Find a work item for this connection
 */
static ism_plugin_job_t * findWork(ism_transport_t * transport, uint32_t seqnum, int remove) {
    ism_plugin_job_t * job = NULL;
    if (LIKELY(transport != NULL)) {
        ismPluginPobj_t * pobj = transport->pobj;
        pthread_spin_lock(&pobj->sessionlock);
        for (job = pobj->jobsHead; job != NULL; job = job->next) {
            if (job->seqnum != seqnum)
                continue;
            if (remove) {
                if (job->prev)
                    job->prev->next = job->next;
                else
                    pobj->jobsHead = job->next;
                if (job->next)
                    job->next->prev = job->prev;
                else
                    pobj->jobsTail = job->prev;
            }
            break;
        }
        pthread_spin_unlock(&pobj->sessionlock);

    }
    return job;
}

/*
 * Allocate a consumer ID
 */
static ism_plugin_cons_t * findConsumer(ism_transport_t * transport) {
    int i;
    int newSize;
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    ism_plugin_cons_t * * newArray;
    ism_plugin_cons_t   * consumer = NULL;
    pthread_spin_lock(&(pobj->lock));

    for (i = 1; i < pobj->consumer_alloc; i++) {
        consumer = (ism_plugin_cons_t *) pobj->consumers[i];
        if (consumer == NULL) {
            consumer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,51),1, sizeof(ism_plugin_cons_t));
            if (consumer == NULL) {
                pthread_spin_unlock(&(pobj->lock));
                return NULL;
            }
            pobj->consumers[i] = consumer;
            consumer->which = i;
            consumer->transport = transport;
            break;
        } else if (consumer->closed) {
            consumer->closed = 0;
            break;
        }
    }

    /* If empty entry found, return. */
    if (i < pobj->consumer_alloc) {
        pthread_spin_unlock(&(pobj->lock));
        return consumer;
    }

    /*
     * Extend array of consumers, if existing array is insufficient.
     */
    newSize = (pobj->consumer_alloc) ? pobj->consumer_alloc * 2 : 64;
    newArray = ism_common_realloc(ISM_MEM_PROBE(ism_memory_protocol_misc,52),pobj->consumers,newSize*sizeof(ism_plugin_cons_t*));
    if (newArray == NULL) {
        pthread_spin_unlock(&(pobj->lock));
        return NULL;
    }
    pobj->consumers = newArray;
    pobj->consumer_alloc = newSize;

    consumer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,53),1, sizeof(ism_plugin_cons_t));
    if (consumer == NULL) {
        pthread_spin_unlock(&(transport->pobj->lock));
        return NULL;
    }
    transport->pobj->consumers[i] = consumer;
    consumer->closed = 0;
    consumer->which = i;
    consumer->transport = transport;
    pthread_spin_unlock(&(transport->pobj->lock));
    return consumer;
}

/*
 * Find a consumer by name
 */
static ism_plugin_cons_t * findConsumerByName(ism_transport_t * transport, const char * name) {
    ism_plugin_cons_t   * consumer = NULL;
    int i;
    ism_protobj_t * pobj = transport->pobj;
    pthread_spin_lock(&(pobj->lock));
    for (i = 1; i < pobj->consumer_alloc; i++) {
        consumer = (ism_plugin_cons_t *) pobj->consumers[i];
        if (consumer && !consumer->closed && consumer->name && !strcmp(name, consumer->name))
            break;
    }
    pthread_spin_unlock(&(transport->pobj->lock));
    return consumer;
}

/*
 * Free a consumer
 */
static void freeConsumer(ism_transport_t * transport, ism_plugin_cons_t * consumer) {
    if (consumer == NULL)
        return;
    pthread_spin_lock(&(transport->pobj->lock));
    if (consumer->name)
        ism_common_free(ism_memory_protocol_misc,consumer->name);
    if (consumer->dest)
        ism_common_free(ism_memory_protocol_misc,consumer->dest);
    consumer->name = NULL;
    consumer->dest = NULL;
    consumer->chandle = NULL;
    consumer->qos = 0;
    consumer->share = 0;
    consumer->closed = 1;
    consumer->suspended = 0;
    pthread_spin_unlock(&(transport->pobj->lock));
}

/*
 * Convert endpoint object to a Map object
 */
static void makeConnectMap(concat_alloc_t * map, ism_transport_t * transport) {
    int sizepos = map->used;
    int len;

    /* Ensure we have some buffer */
    ism_protocol_ensureBuffer(map, 16);

    /*
     * Reserve space for data size
     */
    map->buf[map->used] = (char)(S_Map+3);
    map->used += 4;
    if (transport->protocol && *transport->protocol && strcmp(transport->protocol, "unknown")) {
        ism_protocol_putNameValue(map, "Protocol");
        ism_protocol_putStringValue(map, transport->protocol);
    }
    if (transport->protocol_family && *transport->protocol_family) {
        ism_protocol_putNameValue(map, "ProtocolFamily");
        ism_protocol_putStringValue(map, transport->protocol);
    }
    if (transport->clientID && *transport->clientID) {
        ism_protocol_putNameValue(map, "ClientID");
        ism_protocol_putStringValue(map, transport->clientID);
    }
    if (transport->client_addr && *transport->client_addr) {
        ism_protocol_putNameValue(map, "ClientAddr");
        ism_protocol_putStringValue(map, transport->client_addr);
    }
    if (transport->endpoint_name && *transport->endpoint_name) {
        ism_protocol_putNameValue(map, "Endpoint");
        ism_protocol_putStringValue(map, transport->endpoint_name);
    }
    if (transport->userid && *transport->userid) {
        ism_protocol_putNameValue(map, "User");
        ism_protocol_putStringValue(map, transport->userid);
    }
    if (transport->cert_name && *transport->cert_name) {
        ism_protocol_putNameValue(map, "CommonName");
        ism_protocol_putStringValue(map, transport->cert_name);
    }

    if (transport->serverport) {
        ism_protocol_putNameValue(map, "Port");
        ism_protocol_putIntValue(map, transport->serverport);
    }

    if (transport->secure) {
        ism_protocol_putNameValue(map, "Secure");
        ism_protocol_putByteValue(map, transport->secure);
    }

    if (transport->domain && transport->domain->id && transport->domain->name) {
        ism_protocol_putNameValue(map, "Domain");
        ism_protocol_putStringValue(map, transport->domain->name);
    }

    len = map->used - sizepos - 4;
    map->buf[sizepos+1] = (char)(len >> 16);
    map->buf[sizepos+2] = (char)((len >> 8) & 0xff);
    map->buf[sizepos+3] = (char)(len & 0xff);
}

static void replyCreateTransaction(int32_t rc, void * handle, void * vaction) {
    ism_plugin_act_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;
    if (LIKELY(action->action != PluginAction_CreateTransaction)) {
    	rc = action->rc;
    }
    if (UNLIKELY(rc != 0)) {
    	replyComplete(action, rc);
    } else {
        ism_transport_t * channel = action->channel;
        char xbuf[128];
        concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
        uint64_t value = (uint64_t)((uintptr_t)handle);
        ism_protocol_putIntValue(&buf, transport->monitor_id);
        ism_protocol_putIntValue(&buf, (int)action->seqnum);
        ism_protocol_putIntValue(&buf, 0);   /* rc */
        ism_protocol_putLongValue(&buf,value);
        channel->send(channel, buf.buf+6, buf.used-6, (PluginAction_OnComplete<<8)+4, SFLAG_FRAMESPACE);
    }

    /* If close is in progress, proceed with it */
    if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
        ism_plugin_replyCloseClient(transport);
    }

}

static void replyTransactionAction(int32_t rc, void * handle, void * vaction) {
    ismEngine_TransactionHandle_t transHandle = NULL;
    ism_plugin_act_t * action = vaction;
	if (LIKELY(rc == 0)) {
	    ism_transport_t * transport = action->transport;
	    ism_protobj_t * pobj = transport->pobj;
	    action->rc = 0;
	    rc = ism_engine_createLocalTransaction(pobj->session_handle, &transHandle, action,
        		sizeof(ism_plugin_act_t), replyCreateTransaction);
        if (rc == ISMRC_AsyncCompletion)
        	return;
	} else {
		action->rc = rc;
	}
	replyCreateTransaction(rc, transHandle, vaction);
}


/*
 * Reply to a simple action from the engine
 */
static void replyAction(int32_t rc, void * handle, void * vaction) {
    ism_plugin_act_t * action = vaction;
    ism_transport_t * transport = action->transport;
    ism_protobj_t * pobj = transport->pobj;
    ism_plugin_cons_t * consumer;
    ismEngine_SessionHandle_t sessionh;

    if (LIKELY(rc == 0)) {
        int options = ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND;
        switch (action->paction) {
        case Action_createConnection:
            pobj->client_handle = handle;
            pobj->consumer_alloc = 128;
            pobj->consumers = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,56),128, sizeof(ism_plugin_cons_t *));
            ism_transport_connectionReady(transport);
            if (action->hdrcount)
            	options |= ismENGINE_CREATE_SESSION_TRANSACTIONAL;
            action->paction = Action_createSession;
            rc = ism_engine_createSession(pobj->client_handle, options, &sessionh, action, sizeof(ism_plugin_act_t), replyAction);
            if (rc == ISMRC_AsyncCompletion)
                break;
            action->rc = rc;
            handle = sessionh;
            /* fall thru */

        case Action_createSession:
            pobj->session_handle = handle;
            if (!rc) {
                rc = ism_engine_startMessageDelivery(pobj->session_handle,
                    ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
            }

            /* Send a reply to the plug-in for unauthorized connection. */
            // printf("send onConnected after start message delivery\n");
            pluginReplyConnect(action);
            break;

        case Action_createConsumer:
            consumer = action->consumer;
            consumer->chandle = handle;
            TRACEL(7, transport->trclevel, "Create plug-in consumer: connect=%u client=%s consumer=%d name=%s dest=%s seqnum=%u\n",
                   transport->index, transport->name, consumer->which, consumer->name, consumer->dest, (uint32_t)action->seqnum);
            replyComplete(action,0);
            xUNUSED int zrc = ism_engine_resumeMessageDelivery(handle, 0, NULL, 0, NULL);
            break;
        case Action_closeConsumer:
            consumer = action->consumer;
            TRACEL(7, transport->trclevel, "Close plug-in consumer: connect=%u client=%s consumer=%d name=%s dest=%s seqnum=%u\n",
                   transport->index, transport->name, consumer->which, consumer->name, consumer->dest, (uint32_t)action->seqnum);
            freeConsumer(transport, consumer);
            replyComplete(action,0);
            break;
        case Action_unsubscribeDurable:
            replyComplete(action,0);
            break;
        }
    } else {
        action->rc = rc;
        switch (action->paction) {
        case Action_createConnection:
            if (rc == ISMRC_ClientIDInUse)
                ism_transport_checkClientLiveness(transport->clientID, transport->index);
            /* fall thru */
        case Action_createSession:			/* BEAM suppression: fall through */
            pluginReplyConnect(action);
            break;
        case Action_createConsumer:
            consumer = action->consumer;
            if (consumer) {
                TRACEL(7, transport->trclevel, "Failed to create plug-in consumer: connect=%u client=%s name=%s dest=%s seqnum=%u rc=%d\n",
                       transport->index, transport->name, consumer->name, consumer->dest, (uint32_t)action->seqnum, rc);
            } else {
                TRACEL(7, transport->trclevel, "Failed to create plug-in consumer: connect=%u client=%s seqnum=%u rc=%d\n",
                       transport->index, transport->name, (uint32_t)action->seqnum,rc);
            }
            freeConsumer(transport, consumer);
            replyComplete(action,rc);
            break;
        case Action_closeConsumer:
            consumer = action->consumer;
            if (consumer) {
                TRACEL(7, transport->trclevel, "Failed to close plug-in consumer: connect=%u client=%s name=%s dest=%s seqnum=%u rc=%d\n",
                       transport->index, transport->name, consumer->name, consumer->dest, (uint32_t)action->seqnum, rc);
            } else {
                TRACEL(7, transport->trclevel, "Failed to close plug-in consumer: connect=%u client=%s seqnum=%u rc=%d\n",
                       transport->index, transport->name, (uint32_t)action->seqnum,rc);
            }
            freeConsumer(transport, consumer);
            replyComplete(action,rc);
            break;
        case Action_unsubscribeDurable:
            replyComplete(action,rc);
            break;
        }
    }
    /* If close is in progress, proceed with it */
    if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
        ism_plugin_replyCloseClient(transport);
    }
}


/*
 * Check if this is a frame we support
 */
int ism_plugin_framechecker(ism_transport_t * transport, char * buffer, int buflen, int * used) {
    int  rc = -1;
    ism_plugin_t * plugin;

    /* TODO: this is broken if multiple protocols share a first byte */
    ism_transport_t * channel = ism_plugin_getChannelTransport(transport->tid);
    if (channel) {
        plugin = ism_plugin_findByFirstByte((uint8_t)buffer[0]);
        if (plugin) {
            char xbuf[4098];
            concat_alloc_t buf = {xbuf, sizeof xbuf, 6};

            ism_protobj_t * pobj = (ism_protobj_t *) ism_transport_allocBytes(transport, sizeof(ism_protobj_t), 1);
            memset((char *) pobj, 0, sizeof(ism_protobj_t));
            transport->pobj = pobj;
            pobj->transport = transport;
            pthread_spin_init(&pobj->lock, 0);
            pthread_spin_init(&pobj->sessionlock, 0);
            pobj->keepAlive = -1;
            TRACE(7, "ism_plugin_framechecker: connection=%u pobj=%p \n", transport->index, pobj);
            transport->receive = ism_plugin_receiveData;
            transport->resume = pluginResumeDelivery;
            transport->closing = ism_plugin_closing;
            transport->checkLiveness = pluginCheckLiveness;
            transport->frame = ism_transport_noFrame;
            transport->addframe = ism_transport_addNoFrame;
            transport->delay_close = 20;
            pluginAddToClientsList(pobj, 0);

            /*
             * Send the OnConnection action
             */
            if (!transport->monitor_id) {
                ism_transport_addMonitorNow(transport);
            }
            ism_protocol_putIntValue(&buf, transport->monitor_id);
            ism_protocol_putIntValue(&buf, 1);
            ism_protocol_putByteValue(&buf, 0);
            makeConnectMap(&buf, transport);
            ism_protocol_putByteArrayValue(&buf, buffer, buflen);
            *used = buflen;    /* Set the buffer as used */
            channel->send(channel, buf.buf+6, buf.used-6, (PluginAction_OnConnection<<8)+3, SFLAG_FRAMESPACE);
            if (buf.inheap)
                ism_common_freeAllocBuffer(&buf);
            rc = 0;
        }
        ism_plugin_freeChannelTransport(transport->tid);
    }

    return rc;
}

/**
 * Callback when ger retained message is completed in Engine
 */
static void replyGetRetainedMessageAction(int32_t rc, void * handle, void * vaction) {
	if (rc != ISMRC_OK) {
		ism_plugin_act_t * action = vaction;
		ism_transport_t * transport = action->transport;
		char xbuf[12000];
		concat_alloc_t buf = { xbuf, sizeof xbuf, 6 };
		ism_protocol_putIntValue(&buf, transport->monitor_id);
		ism_protocol_putIntValue(&buf, action->seqnum);
		ism_protocol_putIntValue(&buf, rc);

		int actionType=PluginAction_OnGetMessage;
		ism_transport_t * channel = ism_plugin_getChannelTransport(transport->tid);
		if (channel) {
			channel->send(channel, buf.buf+6, buf.used-6, (actionType<<8)+3, SFLAG_FRAMESPACE);
			ism_plugin_freeChannelTransport(transport->tid);
		}
	}
}

/**
 * Get retained message from Engine for a topic.
 * @param action action object
 * @param seqnum the seq number from the plugin
 * @param topic  the topic name
 */
static int  ism_plugin_getRetainedMessage(ism_plugin_act_t * action, int seqnum, const char * topic)
{
	 ism_transport_t * transport = action->transport;
	 ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
	 int rc=0;

	 rc = ism_engine_getRetainedMessage(pobj->session_handle,
	                                topic,
	                                action,
	                                sizeof(ism_plugin_act_t),
	                                replyMessage,
	                                action,
	                                sizeof(ism_plugin_act_t),
	                                replyGetRetainedMessageAction);


	if (rc != ISMRC_AsyncCompletion) {
		replyGetRetainedMessageAction(rc, NULL, action);
	}

	return rc;
}


/*
 * Reply from completion of unsetRetained
 */
static void replyDeleteRetain(int32_t rc, void * handle, void * vaction) {
    ism_plugin_act_t * action = vaction;
    ism_transport_t * transport = action->transport;
    if (rc == ISMRC_SomeDestinationsFull) {
        transport->listener->stats->count[transport->tid].read_msg++;
        transport->listener->stats->count[transport->tid].warn_msg++;
        /* ISMRC_SomeDestinationsFull provides a warning statistic for
         * the server.  From the client perspective, the request has
         * succeeded. So change the rc to 0 now that the stat has been
         * recorded.
         */
        rc = 0;
    } else if (rc == ISMRC_NoMatchingDestinations || rc == ISMRC_NoMatchingLocalDestinations) {
        transport->listener->stats->count[transport->tid].read_msg++;
        /* Informational return codes */
        rc = 0;
    }
    replyComplete(action, rc);
}

/**
 * delete retained message from Engine for a topic.
 * @param action action object
 * @param topic  the topic name
 */
static int  ism_plugin_deleteRetain(ism_plugin_act_t * action, const char * topic) {
	ism_transport_t * transport = action->transport;
	ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
	int rc=0;

	if (action->seqnum) {
	    rc = ism_engine_unsetRetainedMessageOnDestination(pobj->session_handle,
	                     ismDESTINATION_TOPIC, topic, ismENGINE_UNSET_RETAINED_OPTION_NONE,
	                     ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME, NULL,
	                     action, sizeof *action, replyDeleteRetain);
	    if (rc != ISMRC_AsyncCompletion) {
	        replyDeleteRetain(rc, NULL, action);
	    }
	} else {
	    rc = ism_engine_unsetRetainedMessageOnDestination(pobj->session_handle,
	                     ismDESTINATION_TOPIC, topic, ismENGINE_UNSET_RETAINED_OPTION_NONE,
	                     ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME, NULL,
	                     NULL, 0, NULL);
	}
    return 0;
}
/**
 * Decide whether HTTP Headers should include the "Server:" header
 */
static void initHTTPServerHeaderSetting(void) {
    /*
     * Do we tell clients what the server software is using HTTP Headers?
     * Useful during debug but in production, can leak info used to choose exploits to
     * attack us.
     */
    int includeServerHTTPHeader = ism_common_getIntConfig("IncludeServerHTTPHeader", 1);

    if (includeServerHTTPHeader) {
        g_sendServerHTTPHeader = true;
    } else {
        TRACE(5, "Disabling Server HTTP Header (IncludeServerHTTPHeader = 0)\n");
        g_sendServerHTTPHeader = false;
    }
}
/**
 * Get any text we should include in HTTP Headers to identify the server
 */
static inline const char *getServerHTTPHeaderString(void) {
    if (g_sendServerHTTPHeader) {
        return "Server: " IMA_PRODUCTNAME_FULL "\r\n";
    } else {
        return "";
    }
}

