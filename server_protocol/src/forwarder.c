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

/*
 * The forwarder implements both sides of the forwarder protocol.  Each server has
 * a cluster messaging endpoint.  Each other member of the cluster will open a
 * outgoing forwarder channel to this endpoint.  Each channnel is uni-directional
 * and only replies such as acknowledge is sent back over the channel.
 *
 * When a message is routed by cluster and/or engine to another server, it is put
 * on the remote server (forwarding) queue associated with that server.  There are
 * separate forwarding queues for QoS=0 and QoS>0.  Thus for each channel we
 * create two consumers.
 *
 * While there is an open outgoing channel, messages are received from the engine and sent
 * across the outgoing forwarding channel.  When a message is received on the incoming
 * channel it is processed just like the original publish of the message and sent to the engine.
 *
 * Messages at QoS>0 are send using reliable data transfer to ensure message reliability.
 */
#define TRACE_COMP Forwarder
#include "forwarder.h"
#include <cluster.h>


#define MkVersion(v,r,m,f) (v)*1000000+(r)*10000+(m)*100+(f)
/*
 * Server version:
 * 2.0.0.0 Is the original version
 */
int fwd_Version2_0       = MkVersion(2,0,0,0);
int fwd_Version_Current  = MkVersion(2,0,0,0);

/*
 * Dummpy endpoint so we do not segfault
 */
static ism_endpoint_t outEndpoint = {
    .name        = "!Forwarder",
    .ipaddr      = "",
    .secprof     = "",           /* Security profile name                  */
    .msghub      = "!Forwarder", /* Message hub name                       */
    .conpolicies = "",           /* Connection policies                    */
    .topicpolicies = "",         /* Messaging policies                     */
    .qpolicies   = "",
    .subpolicies = "",
    .transport_type = "none",
    .protomask   = PMASK_Internal,
    .transmask   = TMASK_AnyTrans,
    .thread_count = 1,
    .maxSendBufferSize = 8*1024*1024,
    .maxRecvBufferSize = 8*1024*1024,
};


int fwd_unit_test = 0;
static ismFwdPobj_t * clientListHead = NULL;
static ismFwdPobj_t * clientListTail = NULL;
       ism_fwd_channel_t * fwd_channelList = NULL;
static pthread_spinlock_t clientListLock;
       pthread_mutex_t fwd_configLock;

uint64_t fwd_flowCount = 100;
uint64_t fwd_flowSize = 1048576;
uint32_t fwd_maxXA = 100;
uint32_t fwd_minXA = 2;

/*
 * Global variables
 */
ism_transport_t * fwd_transport = NULL;
ismEngine_ClientStateHandle_t fwd_client = NULL;
ismEngine_SessionHandle_t fwd_sessionh = NULL;
volatile int fwd_startMessaging = 0;
struct ssl_ctx_st * fwd_tlsCTX = NULL;     /* TLS context  */
volatile int  fwd_stopping = 0;
int fwd_commit_time = 100;
int fwd_commit_count = 1000;
ism_timer_t fwd_commit_timer = NULL;
int fwd_enabled = 1;

/*
 * Forward references
 */
ism_fwd_channel_t * ism_fwd_findChannel(const char * uid);
static void completeConnectionClosing(ism_transport_t * transport);
void ism_fwd_replyCloseClient(ism_transport_t * transport);
int ism_fwd_startDelivery(ism_transport_t * transport, void * userdata, uint64_t flags);
int ism_fwd_resume(ism_transport_t * transport, void * userdata);
static int closeChannel(ism_timer_t key, ism_time_t timestamp, void * userdata);
int ism_protocol_termForwarder(void);
int ism_fwd_sendRecover(ism_transport_t * transport);
int  ism_fwd_addDeliveryHandle(ism_fwd_channel_t * channel, uint64_t seqn, ismEngine_DeliveryHandle_t deliveryh);
int ism_fwd_timedCommit(ism_timer_t key, ism_time_t timestamp, void * userdata);
int ism_fwd_commitTimeCheck(ism_timer_t key, ism_time_t timestamp, void * userdata);
static int my_strcmp(const char * str1, const char * str2);
int ism_fwd_recoverTransactions(void);

/*
 * Return the forwarder endpoint
 */
ism_endpoint_t * ism_fwd_getOutEndpoint(void) {
    return &outEndpoint;
}

/*
 * Create a channel
 */
ism_fwd_channel_t * ism_fwd_newChannel(const char * serverUID, const char * serverName) {
    ism_fwd_channel_t * channel = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,249),1, sizeof(ism_fwd_channel_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&channel->lock, &attr);
    ism_fwd_replaceString(&channel->uid, serverUID);
    ism_fwd_replaceString(&channel->name, serverName);
    channel->status_time = ism_common_currentTimeNanos();

    /*
     * Link it in.  If possible keep in server name order
     */
    if (!serverName || !fwd_channelList || !fwd_channelList->name ||
            strcmp(serverName, fwd_channelList->name) <= 0) {
        channel->next = fwd_channelList;
        fwd_channelList = channel;
    } else {
        ism_fwd_channel_t * chan = fwd_channelList;
        while (chan) {
            if (chan->name && (!chan->next || strcmp(serverName, chan->name) >= 0)) {
                channel->next = chan->next;
                chan->next = channel;
                break;
            }
            chan = chan->next;
        }
    }
    return channel;
}

/*
 * Cluster event notification.
 *
 * This method is called by the cluster component when there is a change to the remote server
 * definition.  Update the channel as required.
 */
int32_t ism_fwd_cluster_notification(
        PROTOCOL_RS_EVENT_TYPE_t           eventType,
        ismProtocol_RemoteServerHandle_t   remoteServer,
        const char *                       serverName,
        const char *                       serverUID,
        const char *                       serverAddr,
        int                                serverPort,
        uint8_t                            useTLS,
        ismCluster_RemoteServerHandle_t    clusterHandle,
        ismEngine_RemoteServerHandle_t     engineHandle,
        void *                             context,
        ismProtocol_RemoteServerHandle_t * outRemoteServer) {
    const char * ipaddr;
    int          port;
    uint8_t      secure;


    ism_fwd_channel_t * channel = remoteServer;

    if (!fwd_enabled || fwd_stopping)
        return 0;

    switch (eventType) {
    /*
     * Create a new channel.
     */
    case PROTOCOL_RS_CREATE:
        pthread_mutex_lock(&fwd_configLock);
        channel = ism_fwd_findChannel(serverUID);
        if (!channel) {
            TRACE(5, "ism_fwd_cluster_notification(PROTOCOL_RS_CREATE): New channel: ServerName=%s ServerUID=%s addr=%s port=%d secure=%d\n",
                    serverName, serverUID, serverAddr, serverPort, useTLS);
            channel = ism_fwd_newChannel(serverUID, serverName);
        } else {
            TRACE(5, "ism_fwd_cluster_notification(PROTOCOL_RS_CREATE): Existing channel: ServerName=%s ServerUID=%s addr=%s port=%d secure=%d\n",
                    serverName, serverUID, serverAddr, serverPort, useTLS);
            ism_fwd_replaceString(&channel->name, serverName);
        }

        channel->clusterHandle = clusterHandle;
        channel->engineHandle  = engineHandle;
         ism_fwd_replaceString(&channel->ipaddr, serverAddr);
        channel->port   = serverPort;
        channel->secure = useTLS;
        channel->status_time = ism_common_currentTimeNanos();
        channel->cc_state = CHST_Open;
        if (context && !strcmp(context, "CUNIT"))
            channel->unit_test = 1;

        /* If after start messaging, start the channel */
        if (fwd_startMessaging) {
            channel->retry = 0;
            ism_fwd_startChannel(channel);
        }
        pthread_mutex_unlock(&fwd_configLock);

        /* Return the channel to cluster component */
        *outRemoteServer = channel;
        break;

    /*
     * Disconnect this channel.
     * Stop message processing for a channel, but leave the channel object available
     */
    case PROTOCOL_RS_DISCONNECT:
        pthread_mutex_lock(&channel->lock);
        TRACE(5, "ism_fwd_cluster_notification(PROTOCOL_RS_DISCONNECT): name=%s uid=%s\n", channel->name, channel->uid);
        channel->cc_state = CHST_Closed;
        channel->status_time = ism_common_currentTimeNanos();
        ism_fwd_disconnectChannel(channel);
        pthread_mutex_unlock(&channel->lock);
        break;

    /*
     * Close this channel.
     * After making this call, the invoker will never use the channel object again.
     */
    case PROTOCOL_RS_REMOVE:
        pthread_mutex_lock(&fwd_configLock);
        pthread_mutex_lock(&channel->lock);
        TRACE(5, "ism_fwd_cluster_notification(PROTOCOL_RS_REMOVE): name=%s uid=%s\n", channel->name, channel->uid);
        channel->cc_state = CHST_Closed;

        /* Unlink the channel */
        if (fwd_channelList) {
            if (fwd_channelList == channel) {
                fwd_channelList = channel->next;
            } else {
                ism_fwd_channel_t * ch = fwd_channelList;
                while (ch->next) {
                    if (ch->next == channel) {
                        ch->next = channel->next;
                        channel->next = NULL;
                        break;
                    }
                    ch = ch->next;
                }
            }
        }
        ism_fwd_disconnectChannel(channel);
        ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t) closeChannel, channel, 100000000);
        pthread_mutex_unlock(&channel->lock);
        pthread_mutex_unlock(&fwd_configLock);
        break;

    /*
     * Update client info for this channel.
     * Change the channel definition, and restart the channel if required.
     */
    case PROTOCOL_RS_CONNECT:
        pthread_mutex_lock(&fwd_configLock);
        pthread_mutex_lock(&channel->lock);
        TRACE(5, "ism_fwd_cluster_notification(PROTOCOL_RS_CONNECT): name=%s uid=%s addr=%s port=%d secure=%d\n", channel->name, channel->uid,
                serverAddr, serverPort, useTLS);
        ipaddr = channel->ipaddr;
        secure =  channel->secure;
        port   =  channel->port;
         ism_fwd_replaceString(&channel->ipaddr, serverAddr);
         ism_fwd_replaceString(&channel->name, serverName);
        channel->port   = serverPort;
        channel->secure = secure;
        channel->cc_state = CHST_Open;
        if(fwd_startMessaging) {
            int changed = channel->port != port || channel->secure != secure || my_strcmp(channel->ipaddr, ipaddr);
            if(channel->out_channel) {
                if(changed) {
                    channel->retry = 0;
                    ism_fwd_disconnectChannel(channel);
                } else {
                    ism_cluster_remoteServerConnected(channel->clusterHandle);
                }
            } else {
                channel->retry = 0;
                ism_fwd_startChannel(channel);
            }
        }
        pthread_mutex_unlock(&channel->lock);
        pthread_mutex_unlock(&fwd_configLock);
        break;
    case  PROTOCOL_RS_TERM:
        ism_protocol_termForwarder();
        break;
    }
    return 0;
}


/*
 * Calculate a message rate
 */
int calcRate(ism_fwd_channel_t * channel, uint64_t now_count, uint64_t old_count) {
    int  rate;
    double now = ism_common_readTSC();
    if (now <= channel->old_send_time)
        return 0;
    rate = (int)((now_count-old_count) / (now-channel->old_send_time));
    if (rate < 0)
        rate = 0;
    return rate;
}

/*
 * Calculate rate as float value
 */
double calcRateD(ism_fwd_channel_t * channel, double now, uint64_t now_count, uint64_t old_count) {
    double  rate;
    if (now <= channel->old_send_time)
        return 0.0;
    rate = (now_count-old_count) / (now-channel->old_send_time);
    if (rate < 0.0)
        rate = 0.0;
    return rate;
}


/*
 * Put out stats for a queue
 */
static void putQueueStats(concat_alloc_t * buf, char * xbuf, const char * name, int which,
        ism_fwd_channel_t * channel, ismEngine_QueueStatistics_t * qstat, uint64_t suspend) {
    int sendrate = calcRate(channel, qstat->ConsumedMsgs, which ? channel->old_send1 : channel->old_send0);
    sprintf(xbuf, ",\n"
        "    \"%s\": { \"BufferedMsgs\":%lu, \"BufferedMsgsHWM\":%lu, \"BufferedBytes\":%lu, \"MaxBytes\":%lu,\n"
        "        \"SentMsgs\":%lu, \"MsgSendRate\":%u, \"DiscardedMsgs\":%lu, \"ExpiredMsgs\":%lu, \"Suspend\":%lu }",
        name, qstat->BufferedMsgs, qstat->BufferedMsgsHWM, qstat->BufferedMsgBytes, qstat->MaxMessageBytes,
        qstat->ConsumedMsgs, sendrate, qstat->DiscardedMsgs, qstat->ExpiredMsgs, suspend);
    ism_json_putBytes(buf, xbuf);
}


/*
 * Output on forwarder channel stat
 */
static void channelStat(concat_alloc_t * buf, ism_fwd_channel_t * channel, ism_ts_t * ts) {
    char xbuf [1024];
    char date [64];
    const char * health;
    const char * hastate;
    const char * status;
    int  reconnect;
    ismEngine_RemoteServerStatistics_t rs_stat = {{0}};

    ism_json_putBytes(buf, "  { \"ServerName\":");
    ism_json_putString(buf, channel->name ? channel->name : "");
    ism_json_putBytes(buf, ", \"ServerUID\":");
    ism_json_putString(buf, channel->uid);
    ism_common_setTimestamp(ts, channel->status_time);

    status = "Inactive";
    if (channel->cluster_state == ISM_CLUSTER_RS_STATE_ACTIVE)
        status = "Active";
    else if (channel->cluster_state == ISM_CLUSTER_RS_STATE_CONNECTING)
        status = "Connecting";

    switch (channel->cluster_health) {
    case ISM_CLUSTER_HEALTH_GREEN :   health = "Green";   break;
    case ISM_CLUSTER_HEALTH_YELLOW :  health = "Yellow";  break;
    case ISM_CLUSTER_HEALTH_RED :     health = "Red";     break;
    default:                          health = "Unknown"; break;
    }

    switch (channel->cluster_ha) {
    case ISM_CLUSTER_HA_DISABLED :        hastate = "None";    break;
    case ISM_CLUSTER_HA_PRIMARY_SINGLE :  hastate = "Single";  break;
    case ISM_CLUSTER_HA_PRIMARY_PAIR :    hastate = "Pair";    break;
    case ISM_CLUSTER_HA_STANDBY :         hastate = "Standby"; break;
    case ISM_CLUSTER_HA_ERROR :           hastate = "Error";   break;
    default:                              hastate = "Unknown"; break;
    }
    reconnect = channel->connections-2;
    if (reconnect<0)
        reconnect = 0;

    /* Get the engine stats */
    if (channel->engineHandle) {
        xUNUSED int zrc = ism_engine_getRemoteServerStatistics(channel->engineHandle, &rs_stat);
    }

    ism_common_formatTimestamp(ts, date, sizeof date, 6,  ISM_TFF_ISO8601);

    /* Format the header */
    int recvrate = calcRate(channel, channel->read_msg, channel->old_recv);
    sprintf(xbuf, ", \"Status\":\"%s\",\n"
            "    \"StatusTime\":\"%s\", \"Health\":\"%s\", \"Memory\":%u,\n"
            "    \"HAStatus\":\"%s\", \"RetainedSync\":%s, \"Reconnect\":%u,\n"
            "    \"ReadMsg\":%lu, \"ReadBytes\":%lu, \"WriteMsg\":%lu, \"WriteBytes\":%lu, \"ReadMsgRate\":%u",
            status, date, health, channel->cluster_memory, hastate, rs_stat.retainedSync ? "true" : "false",
                    reconnect, channel->read_msg, channel->read_bytes, channel->write_msg, channel->write_bytes,
                    recvrate);
    ism_json_putBytes(buf, xbuf);

    /* Put out the queue stats */
    putQueueStats(buf, xbuf, "Reliable",   1, channel, &rs_stat.q1, channel->suspend1);
    putQueueStats(buf, xbuf, "Unreliable", 0, channel, &rs_stat.q0, channel->suspend0);
    ism_json_putBytes(buf, "\n  }");
}


/*
 * Periodic update of rate info
 */
static void updateChannelRates(ism_fwd_channel_t * channel) {
    if (channel->engineHandle) {
        ismEngine_RemoteServerStatistics_t rs_stat = {{0}};
        xUNUSED int zrc = ism_engine_getRemoteServerStatistics(channel->engineHandle, &rs_stat);
        channel->old_recv = channel->read_msg;
        channel->old_send0 = rs_stat.q0.ConsumedMsgs;
        channel->old_send1 = rs_stat.q1.ConsumedMsgs;
    }
}

/*
 * Get the forwarder monitoring stats
 */
static int getForwarderMonitorStats(fwd_monstat_t * monstat, int option) {
    ism_fwd_channel_t * channel;
    int count = 1;
    double recvrate = 0.0;
    double sendrate0 = 0.0;
    double sendrate1 = 0.0;
    double now;
    ismEngine_RemoteServerStatistics_t rs_stat = {{0}};

    if (!fwd_startMessaging || fwd_stopping)
        return ISMRC_ClusterNotAvailable;

    monstat->timestamp = ism_common_currentTimeNanos();

    pthread_mutex_lock(&fwd_configLock);
    now = ism_common_readTSC();

    /*
     * For all channels add up the statistics
     */
    channel = fwd_channelList;
    while (channel) {
        recvrate  += calcRateD(channel, now, channel->read_msg, channel->old_recv);
        if (channel->engineHandle) {
            if (!ism_engine_getRemoteServerStatistics(channel->engineHandle, &rs_stat)) {
                sendrate0 += calcRateD(channel, now, rs_stat.q0.ConsumedMsgs, channel->old_send0);
                sendrate1 += calcRateD(channel, now, rs_stat.q1.ConsumedMsgs, channel->old_send1);
            }
            count++;
        }
        channel = channel->next;
    }
    pthread_mutex_unlock(&fwd_configLock);

    /*
     * Store the results
     */
    monstat->channel_count = count;
    monstat->recvrate = (int)recvrate;
    monstat->sendrate0 = (int)sendrate0;
    monstat->sendrate1 = (int)sendrate1;
    return 0;
}

/*
 * Output the forwarder stats as a JSON string
 */
static int getForwarderStats(concat_alloc_t * buf, int option) {
    int count = 0;
    int complete = option&3;
    ism_fwd_channel_t * channel;
    ismCluster_ViewInfo_t * info;
    int i;

    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);

    /*
     * Check for disconnected servers
     */
    pthread_mutex_lock(&fwd_configLock);
    if (fwd_startMessaging && !fwd_stopping) {
        if (!ism_cluster_getView(&info)) {
            ismCluster_RSViewInfo_t * server;
            for (i=0; i<info->numRemoteServers; i++) {
                server = info->pRemoteServers+i;
                channel = ism_fwd_findChannel(server->pServerUID);
                if (!channel) {
                    channel = ism_fwd_newChannel(server->pServerUID, server->pServerName);
                    channel->clusterHandle = server->phServerHandle;
                }
                channel->cluster_state  = (uint8_t)server->state;
                channel->cluster_health = (uint8_t)server->healthStatus;
                channel->cluster_ha     = (uint8_t)server->haStatus;
                //if (server->stateChangeTime)
                //    channel->status_time = server->stateChangeTime;
            }
            ism_cluster_freeView(info);
        }
    }

    if (complete) {
        if (complete == 2) {
            ism_json_putBytes(buf, "{ \"Version\":\"v1\", \"Cluster\": ");
        }
        ism_json_putBytes(buf, "[\n");
    }

    if (fwd_startMessaging && !fwd_stopping) {
        channel = fwd_channelList;
        while (channel) {
            channelStat(buf, channel, ts);
            count++;
            channel = channel->next;
            ism_json_putBytes(buf, channel ? ",\n" : "\n");
        }
    }
    pthread_mutex_unlock(&fwd_configLock);
    ism_common_closeTimestamp(ts);
    if (complete) {
        ism_json_putBytes(buf, "] " );
        if (complete == 2) {
            ism_json_putBytes(buf, "}");
        }
    }
    return count;
}


/*
 * Initialize the forwarder protocol
 */
int ism_protocol_initForwarder(void) {
    int  rc;
    const char * envvar = getenv("CUNIT");
    fwd_unit_test = envvar ? atoi(envvar) : 0;
    TRACE(4, "==== Init forwarder test=%d\n", fwd_unit_test);
    ism_transport_registerProtocol(ism_fwd_startMessaging, ism_fwd_connection);
    pthread_spin_init(&clientListLock, 0);
    pthread_mutex_init(&fwd_configLock, 0);

    /*
     * Temporary when using a single outgoing endpoint
     */
    int iops = ism_tcp_getIOPcount() + 1;    /* Extra for admin */
    int statlen = sizeof(ism_endstat_t);
    if (iops > MAX_STAT_THREADS)
        statlen += (iops-MAX_STAT_THREADS) * sizeof(msg_stat_t);
    outEndpoint.stats = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,250),1, statlen);
    outEndpoint.thread_count = iops;

    fwd_flowCount = ism_common_getIntConfig("ForwarderFlowCount", fwd_flowCount);
    fwd_flowSize  = ism_common_getIntConfig("ForwarderFlowSize",  fwd_flowSize);
    fwd_maxXA     = ism_common_getIntConfig("ForwarderMaxXA",     fwd_maxXA);
    if (fwd_maxXA<2)
        fwd_maxXA = 2;
    fwd_minXA     = ism_common_getIntConfig("ForwarderMinXA",     fwd_minXA);
    if (fwd_minXA >= fwd_maxXA)
        fwd_minXA = fwd_maxXA-1;
    if (fwd_maxXA < 1)
        fwd_maxXA = 1;

    rc = ism_cluster_registerProtocolEventCallback(ism_fwd_cluster_notification, NULL);
    if (rc) {
        TRACE(2, "ism_cluster_registerProtocolEventCallback rc=%d\n", rc);
        if (fwd_unit_test != 42)
            fwd_enabled = 0;
    }

    /* Register for out of order entry point */
    ism_transport_registerFwdStat(getForwarderStats, getForwarderMonitorStats);

    /*
     * Set global commit properties
     */
    fwd_commit_time  = ism_common_getIntConfig("ForwarderCommitTime",  fwd_commit_time);
    fwd_commit_count = ism_common_getIntConfig("ForwarderCommitCount", fwd_commit_count);
    if (fwd_commit_time < 20)
        fwd_commit_time = 20;
    if (fwd_commit_count < 1)
        fwd_commit_count = 1;

    return 0;
}

/*
 * For unit test, call directly to an internal engine callback
 */
int32_t iers_clusterEventCallback(ENGINE_RS_EVENT_TYPE_t eventType,
                                  ismEngine_RemoteServerHandle_t hRemoteServer,
                                  ismCluster_RemoteServerHandle_t hClusterHandle,
                                  const char *pServerName,
                                  const char *pServerUID,
                                  void *pRemoteServerData,
                                  size_t remoteServerDataLength,
                                  char **pSubscriptionTopics,
                                  size_t subscriptionsTopicCount,
                                  uint8_t fIsRouteAll,
                                  uint8_t fCommitUpdate,
                                  ismEngine_RemoteServer_PendingUpdateHandle_t  *phPendingUpdateHandle,
                                  ismCluster_EngineStatistics_t *pEngineStatistics,
                                  void *pContext,
                                  ismEngine_RemoteServerHandle_t *phRemoteServerHandle);



XAPI struct ssl_ctx_st * ism_transport_clientTlsContext(const char * name, const char * tlsversion, const char * cipher);

/*
 * Start the forwarder protocol
 */
int ism_protocol_startForwarder(void) {
    fwd_tlsCTX = ism_transport_clientTlsContext("forwarder", "TLSv1.2", "ECDHE-ECDSA-AES128-GCM-SHA256");
    TRACE(6, "Create forwarder TLS context: %p\n", fwd_tlsCTX);
    ism_engine_registerRetainedForwardingCallback(ism_fwd_requestRetain);
    return 0;
}

/*
 * Start the forwarder endpoint engine objects
 */
int ism_protocol_startFwdEndpoint(void) {
    int options;
    int rc;

    /*
     * Make clients and sessions for incoming endpoint
     */
    fwd_transport = ism_transport_newTransport(&outEndpoint, 0, 0);
    fwd_transport->protocol_family = "fwd";
    fwd_transport->protocol = "fwd";
    rc = ism_security_create_context(ismSEC_POLICY_CONNECTION, fwd_transport, &fwd_transport->security_context);
    if (rc) {
        TRACE(3, "Failure creating forwarder security context: rc=%d\n", rc);
        return rc;
    }
    rc = ism_engine_createClientState("__Forwarder", PROTOCOL_ID_FWD, 0,
            NULL, NULL, fwd_transport->security_context, &fwd_client,
            NULL, 0, NULL);
    if (rc) {
        TRACE(3, "Failure creating forwarder client state: rc=%d\n", rc);
        return rc;
    }
    options = ismENGINE_CREATE_SESSION_TRANSACTIONAL | ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND;
    TRACE(7, "create incoming session\n");
    rc = ism_engine_createSession(fwd_client, options, &fwd_sessionh, NULL, 0, NULL);
    if (rc) {
        TRACE(3, "Failure creating forwarder session: rc=%d\n", rc);
        return rc;
    }
    TRACE(7, "The forwarder engine objects are created successfully\n");
    return 0;
}



/*
 * Start up for forwarding CUnit
 */
int ism_protocol_startCUnit(void) {
    char key[32];
    char val[512];
    char * value;
    char * name;
    char * uid;
    char * addr = NULL;
    int   port = 0;
    int   secure = 0;
    int   i;
    int   rc;
    ismProtocol_RemoteServerHandle_t outHandle;
    ismEngine_RemoteServerHandle_t engineHandle;

    /*
     * Unit test support.  This code is not used in the product.
     */
    for (i=0;;i++) {
        sprintf(key, "RemoteServer.%d", i);
        value = (char *)ism_common_getStringConfig(key);
        if (!value || !*value)
            break;
        ism_common_strlcpy(val, value, sizeof val);
        name = val;
        uid = strchr(name, ',');
        if (uid) {
            *uid++ = 0;
            value = strchr(uid, ',');
            if (value) {
                *value++ = 0;
                secure = *value == '1';
                addr = strchr(value, ',');
                if (addr) {
                    *addr++ = 0;
                    value = strchr(addr, ',');
                    if (value) {
                        *value++ = 0;
                        port = atoi(value);
                    }
                }
            }
        }
        if (port > 0 && port < 65536) {
            TRACE(3, "Test remote server %d: name=%s uid=%s secure=%d addr=%s port=%d\n", i, name, uid, secure, addr, port);
            rc = iers_clusterEventCallback(ENGINE_RS_CREATE, NULL, NULL, name, uid, NULL, 0, NULL, 0,
                    false, false, NULL, NULL, NULL, &engineHandle);
            TRACE(5, "Create remote engine handle.  rc=%d\n", rc);
            ism_fwd_cluster_notification(PROTOCOL_RS_CREATE, NULL, name, uid, addr, port,
                    secure&1, NULL, engineHandle, (secure&2)?"CUNIT":NULL, &outHandle);
            char *testTopics[] = {"#"};
            rc = iers_clusterEventCallback(ENGINE_RS_ADD_SUB, engineHandle, NULL, NULL, NULL, NULL, 0,
                    testTopics, 1,
                    false, false, NULL, NULL, NULL, NULL);
            TRACE(5, "Add subscription.  rc=%d\n", rc);
        } else {
            TRACE(3, "The remote server test setting %d is not correct.\n", i);
        }
    }
    return 0;
}



/*
 * Terminate the forwarder protocol
 */
int ism_protocol_termForwarder(void) {
    ism_fwd_channel_t * channel;
    pthread_mutex_lock(&fwd_configLock);
    fwd_stopping = 1;

    if (fwd_commit_timer) {
        ism_common_cancelTimer(fwd_commit_timer);
        fwd_commit_timer = NULL;
    }


    channel = fwd_channelList;
    while (channel) {
        pthread_mutex_lock(&channel->lock);
        channel->cc_state = CHST_Closed;
        channel->engineHandle = NULL;
        ism_fwd_disconnectChannel(channel);
        pthread_mutex_unlock(&channel->lock);
        channel = channel->next;
    }
    pthread_mutex_unlock(&fwd_configLock);
    return 0;
}

int ism_fwd_commit_outstanding = 0;

/*
 * Start messaging callback
 */
int ism_fwd_startMessaging(void) {
    ism_fwd_channel_t * channel;
    int count;
    int oldcount;
    int rep;
    int  rc;

    if (!fwd_enabled)
        return 0;

    /* For unit testing only */
    if (fwd_unit_test == 42)
        ism_protocol_startCUnit();
    rc = ism_protocol_startFwdEndpoint();
    if (rc) {
        return rc;
    }

    /*
     * Recover any existing transactions
     */
    ism_fwd_recoverTransactions();

    /*
     * Wait for all commits to complete
     */
    oldcount = 0;
    while ((count = __sync_fetch_and_add(&ism_fwd_commit_outstanding, 0)) > 0) {
        if (count != oldcount)
            rep = 0;
        if (rep > 500) {
            TRACE(1, "Unable to commit all transactions.");
            break;
        }
        rep++;
        ism_common_sleep(10000);
    }

    pthread_mutex_lock(&fwd_configLock);
    /*
     * Start the commit timer
     */
    fwd_commit_timer = ism_common_setTimerRate(ISM_TIMER_LOW, ism_fwd_commitTimeCheck, NULL,
            fwd_commit_time*5, fwd_commit_time, TS_MILLISECONDS);

    /*
     * Start channels
     */
    fwd_startMessaging = 1;
    channel = fwd_channelList;
    while (channel) {
        channel->retry = 0;
        ism_fwd_startChannel(channel);
        channel = channel->next;
    }
    pthread_mutex_unlock(&fwd_configLock);

    return 1;
}

/*
 * Start messaging callback.
 * This must be called with the configlock held
 */
ism_fwd_channel_t * ism_fwd_findChannel(const char * uid) {
    ism_fwd_channel_t * channel;
    channel = fwd_channelList;
    while (channel) {
        if (!strcmp(channel->uid, uid)) {
            return channel;
        }
        channel = channel->next;
    }
    return NULL;
}

/*
 * Close the channel on the timer thread.
 * The channel is unlinked before this is called.
 */
xUNUSED static int closeChannel(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_fwd_channel_t * channel = (ism_fwd_channel_t *)userdata;
	ism_common_cancelTimer(key);
    pthread_mutex_lock(&channel->lock);
    if(channel->out_channel || channel->in_channel) {
        ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t) closeChannel, channel, 100000000);
        pthread_mutex_unlock(&channel->lock);
        return 0;
    }
    pthread_mutex_unlock(&channel->lock);
    pthread_mutex_destroy(&channel->lock);
    ism_common_free(ism_memory_protocol_misc,channel);
    return 0;
}

/*
 * Process a new connection.
 */
int ism_fwd_connection(ism_transport_t * transport) {
    int rc = 1;
    ismFwdPobj_t * pobj = NULL;

    /*
     * Forwarder connection
     */
    if (!strcmpi(transport->protocol, "fwd")) {
        pobj = (ismFwdPobj_t *) ism_transport_allocBytes(transport, sizeof(ismFwdPobj_t), 1);
        memset(pobj, 0, sizeof(ismFwdPobj_t));
        // transport->dumpPobj = fwdDumpPobj;
        transport->pobj = pobj;
        transport->receive = ism_fwd_receive;
        transport->resume = ism_fwd_resume;
        transport->actionname = ism_fwd_getActionName;
        transport->protocol_family = "fwd"; /* Constant string */
        transport->closing = ism_fwd_closing;
        pthread_spin_init(&pobj->lock, 0);
        pthread_spin_init(&pobj->sessionlock, 0);
        ism_fwd_addToClientList(pobj);
        rc = 0;
    }
    return rc;
}

/*
 * Validate the gtrid portion of a XID
 * This only needs to be good enough to stop other methods from failing
 */
static int validXID(const char * gtrid) {
    const char * pos = strchr(gtrid, '_');
    if (pos) {
        pos = strchr(pos+1, '_');
        if (pos) {
            pos++;
            if (*pos >= '1' && *pos <= '9') {
                pos++;
                while (*pos >= '0' && *pos <= '9')
                    pos++;
                if (*pos == 0 && (int)(pos-gtrid) < 64)
                    return 1;
            }
        }
    }
    return 0;
}

/*
 * Validate the header
 */
static void validate(ism_fwd_act_t * action, int hdrcnt, int outgoing, const char * types) {
    int  i;
    ism_field_t * hdr = action->hdrs;

    /*
     * Check we have the correct number of parameters
     */
    if (action->hdrcount < hdrcnt) {
        action->rc = ISMRC_BadClientData;
        ism_common_setError(action->rc);
    }

    /*
     * Check for valid incomming and outgoing
     */
    if (action->transport->originated) {
        if (outgoing == 0) {
            action->rc = ISMRC_BadClientData;
            ism_common_setError(action->rc);
        }
    } else {
        if (outgoing == 1) {
            action->rc = ISMRC_BadClientData;
            ism_common_setError(action->rc);
        }
    }

    /*
     * Check for correct data types
     */
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

        case 'X':
            if (hdr[i].type != VT_String || !validXID(hdr[i].val.s)) {
                action->rc = ISMRC_BadClientData;
                ism_common_setError(action->rc);
            }
            break;
        }
    }

    /*
     * Check for properties
     */
    if (types[hdrcnt]=='P' && action->pfield.type != VT_Map && action->pfield.type != VT_Null) {
        action->rc = ISMRC_BadClientData;
        ism_common_setError(action->rc);
    }
}

/*
 * Validate the fields in an action
 */
static void validateAction(ism_fwd_act_t * action) {
    /*
     * Validate the actions
     */
    if (action->rc == 0) {
        switch (action->action) {
        case FwdAction_Message:        /* h0=seqnum, h1=msgtype, h2=flags, h3=dest, h4=expiry, p=props, b=body */
        case FwdAction_RMessage:       /* h0=seqnum, h1=msgtype, h2=flags, h3=dest, h4=expiry, p=props, b=body */
            validate(action, 5, 0, "QIISIP");
            break;

        case FwdAction_Processed:      /* h0=seqnum */
            validate(action, 1, 1, "L");
            break;

        case FwdAction_Prepare:        /* h0=xid, h1=count, b=ids */
            validate(action, 3, 1, "XIQ");
            break;

        case FwdAction_Commit:         /* h0=xid */
        case FwdAction_CommitRecover:
        case FwdAction_RollRecover:
            validate(action, 1, 1, "X");
            break;

        case FwdAction_Connect:        /* h0=ver, h1=time, h2=srvname, h3=srvuid */
            validate(action, 4, 0, "ILSSI");
            break;

        case FwdAction_ConnectReply:   /* h0=version, h1=time, h2=rc  */
            validate(action, 4, 1, "ILII");
            break;

        case FwdAction_Recover:         /* h0=xid */
            validate(action, 1, 0, "S");     /* can be empty so cannot be validated as a xid */
            break;

        case FwdAction_RollbackReply:   /* h0=xid */
            validate(action, 1, 0, "X");
            break;

        case FwdAction_PrepareReply:    /* h0=xid h1=rc */
        case FwdAction_CommitReply:     /* h0=xid h1=rc */
            validate(action, 2, 0, "XI");
            break;

        case FwdAction_Start:
            validate(action, 0, 1, "");
            break;

        case FwdAction_Rollback:       /* h0=xid, h1=count, b=ids */
            validate(action, 2, 1, "XI");
            break;

        case FwdAction_RequestRetain:   /* h0=source h2=options h3=timestamp h4=corrid */
            validate(action, 4, 1, "SILL");
            break;

        case FwdAction_Ping:
        case FwdAction_Pong:
            /* No validtion */
            break;

        case FwdAction_Disconnect:     /* h0=rc, h1=reason */
            validate(action, 2, 2, "IS");
            break;

        default:
            action->rc = ISMRC_BadClientData;
            ism_common_setError(action->rc);
        }
    }
}


/*
 * Receive data.
 *
 * This is an instance of ism_transport_receive_t invoked using transport->receive().
 *
 */
int ism_fwd_receive(ism_transport_t * transport, char * buf, int buflen, int command) {
    ism_fwd_act_t action = {0};
    int  i;
    int  rc = 0;
    int  lrc;
    int  sync = 0;
    char trcbuf [256];
    char trcbuf2 [256];
    ism_field_t hdr[15];
    int32_t ipcount;
    action.action = command >> 8;
    action.hdrcount = command&0x0f;
    memset(&action.buf, 0, sizeof(action.buf));
    action.buf.buf = buf;
    action.buf.used = buflen;
    action.transport = transport;
    action.seqnum = 0;
    action.hdrs = hdr;

    ipcount = __sync_fetch_and_add(&transport->pobj->inprogress, 1);
    /*
     * Binary dump trace of the message except createConnection which can contain a password
     * For message actions we only trace message data if msgdata is set for that many bytes.
     */
    if (SHOULD_TRACE(9)) {
        int maxsize = ism_common_getTraceMsgData()+40;
        sprintf(trcbuf, "Forwarder receive %s %u connect=%u inprogress=%d", ism_fwd_getActionName(action.action),
                action.hdrcount, transport->index, ipcount);
        TRACEDATA(9, trcbuf, 0, buf, buflen, maxsize);
    }

    /* If we are in the process of closing, return closed */
    if (UNLIKELY(ipcount < 0)) { /* BEAM suppression: constant condition */
        __sync_fetch_and_sub(&transport->pobj->inprogress, 1);
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }

    for (i=0; i<action.hdrcount; i++) {
        lrc = ism_protocol_getObjectValue(&action.buf, hdr+i);
        if (lrc) {
            TRACEL(2, transport->trclevel, "Bad forwarder header value: connect=%u which=%d rc=%d\n", transport->index, i, lrc);
            rc += lrc;
        }
    }

    /* Parse the properties */
    if (action.buf.pos < action.buf.used) {
        action.pfield.len = 0;
        lrc = ism_protocol_getObjectValue(&action.buf, &action.pfield);
        if (lrc) {
            TRACEL(2, transport->trclevel, "Bad forwarder properties: connect=%u rc=%d\n", transport->index, lrc);
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
            TRACEL(2, transport->trclevel, "Bad forwarder body: connect=%u rc=%d\n", transport->index, lrc);
            rc += lrc;
        }
    } else {
        action.body.len = 0;
        action.body.type = VT_Null;
    }

    action.rc = rc;
    validateAction(&action);
    if (action.rc == 0) {
        /*
         * Process the actions
         */
        switch (action.action) {
        case FwdAction_Message:
        case FwdAction_RMessage:
            rc = ism_fwd_doMessage(&action, hdr[0].val.l, hdr[1].val.i, hdr[2].val.i, hdr[3].val.s,
                                   (uint32_t)(hdr[4].val.i), &action.pfield, &action.body);
            break;

        case FwdAction_Processed:      /* h0=seqnum */
            rc = ism_fwd_doProcessed(&action, hdr[0].val.l);
            sync = 1;
            break;

        case FwdAction_Prepare:
            rc = ism_fwd_doPrepare(&action, hdr[0].val.s, hdr[1].val.i);
            break;

        case FwdAction_Commit:
            rc = ism_fwd_doCommit(&action, hdr[0].val.s, 0);
            break;

        case FwdAction_PrepareReply:
            rc = ism_fwd_doPrepareReply(&action, hdr[0].val.s, hdr[1].val.i);
            break;

        case FwdAction_CommitReply:
            rc = ism_fwd_doCommitReply(&action, hdr[0].val.s, hdr[1].val.i);
            sync = 1;
            break;

        case FwdAction_Connect:        /* h0=version, h1=timestamp, h2=name, h3=uid  */
            rc = ism_fwd_doConnect(&action, hdr[0].val.i, hdr[1].val.l, hdr[2].val.s, hdr[3].val.s);
            break;

        case FwdAction_ConnectReply:     /* h0=version, h1=timestamp, h2=rc, h3=flags */
            rc = ism_fwd_doConnectReply(&action, hdr[2].val.i, hdr[0].val.i, hdr[1].val.l, hdr[3].val.i);
            break;

        case FwdAction_Recover:
            rc = ism_fwd_doRecover(&action, hdr[0].val.s);
            break;

        case FwdAction_Start:
            rc = ism_fwd_doStart(&action);
            break;

        case FwdAction_CommitRecover:
            rc = ism_fwd_doCommit(&action, hdr[0].val.s, 1);
            break;

        case FwdAction_Rollback:
            rc = ism_fwd_doRollback(&action, hdr[0].val.s, hdr[1].val.i);
            break;

        case FwdAction_RollRecover:
            rc = ism_fwd_doRollbackPrepared(&action, hdr[0].val.s, 1);
            break;

        case FwdAction_RequestRetain:   /* h0=source h1=options h2=timestamp h3=corrid */
            rc = ism_fwd_doRequestRetain(&action, hdr[0].val.s, hdr[1].val.i, hdr[2].val.l, hdr[3].val.l);
            sync = 1;
            break;

        case FwdAction_Ping:
            sync = 1;
            break;

        case FwdAction_Pong:
            /* TODO */
            sync = 1;
            break;

        case FwdAction_Disconnect:     /* h0=rc, h1=reason */
            /* TODO */
        	sync = 1;
            break;
        case FwdAction_RollbackReply:
            /* TODO ???*/
        	sync = 1;
            break;
        default:
        	TRACE(1, "fwd_receive unknown action %d, index=%u \n", action.action, transport->index);
        	action.rc = ISMRC_NotImplemented;
        	break;
        }

    }

    if (action.rc) {
        ipcount = __sync_sub_and_fetch(&transport->pobj->inprogress, 1);
        sprintf(trcbuf2, "The forwarder action data is not valid: connect=%u action=%s inprogress=%d cmd=%04x",
                transport->index, ism_fwd_getActionName(action.action), ipcount, command);
        TRACEDATA(2, trcbuf2, 0, buf, buflen, 2048);
        ism_common_setError(ISMRC_MessageNotValid);
        if (UNLIKELY(ipcount < 0)) { /* BEAM suppression: constant condition */
            ism_fwd_replyCloseClient(transport);
        } else {
            transport->close(transport, ISMRC_BadClientData, 0, "The data from the client is not valid");
        }
        return ISMRC_MessageNotValid;
    } else {
        if (sync) {
            if (UNLIKELY(__sync_sub_and_fetch(&transport->pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
                ism_fwd_replyCloseClient(transport);
            }
        }
    }
    TRACE(9, "Leave receive, index=%u inprogress=%d\n", transport->index, transport->pobj->inprogress);
    return rc;
}


#if 0
/*
 * Callback when the clientID is reused.
 * For the forwarder this should not commonly happen.
 */
static void fwdStealCallback(ismEngine_ClientStateHandle_t hClient, void *pContext) {
    ism_transport_t * transport = (ism_transport_t *) pContext;
    transport->close(transport, ISMRC_ClientIDReused, 0, "The channel is closed because another connection for the same channel is started");
}
#endif


/*
 * Send a PONG when we receive a PING.
 * The PING is allowed to contain any headers, and we put the first one if it exists into the PONG.
 */
int  ism_fwd_doPing(ism_fwd_act_t * action) {
    ism_transport_t * transport = action->transport;
    char xbuf[2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 6};
    if (action->hdrs[0].type != VT_Null) {
        ism_protocol_putObjectValue(&buf, &action->hdrs[0]);
        transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_Pong<<8)+1, SFLAG_FRAMESPACE);
        ism_common_freeAllocBuffer(&buf);
    } else {
        transport->send(transport, buf.buf+6, buf.used-6, (FwdAction_Pong<<8), SFLAG_FRAMESPACE);
    }
    return 0;
}

/*
 * Structure for the close action
 */
typedef struct fwd_close_action_t {
    ism_transport_t * transport;
    int  len;
    int  resv;
} fwd_close_action_t;

/*
 * Complete the closing of a connection
 */
static void completeConnectionClosing(ism_transport_t * transport) {
    ism_protobj_t * pobj = (ism_protobj_t *) transport->pobj;
    int count = __sync_sub_and_fetch(&pobj->inprogress, 1);
    if (count >= 0) {
        TRACEL(6, transport->trclevel, "completeConnectionClosing: closing postponed as there are %d actions/messages in progress: connect=%u client=%s",
                count+1, transport->index, transport->name);
        return;
    } else {
        TRACEL(9, transport->trclevel, "completeConnectionClosing: inprogress=%d connect=%u client=%s", count, transport->index, transport->name);
    }
    ism_fwd_replyCloseClient(transport);
    return;
}

#if 0
/*
 * When the engine stopMessageDelivery returns, send the disconnect action
 */
static void replyStopped(int32_t rc, void * handle, void * vaction) {
    fwd_close_action_t * action = (fwd_close_action_t *)vaction;
    ism_transport_t * transport = action->transport;

    if (action->len) {
        /* Send disconnect action */
        char * buf = (char *)(action+1);
        int rc = transport->send(transport, buf+6, action->len-6, (FwdAction_Disconnect<<8)+4, SFLAG_FRAMESPACE);
        if ((rc == 0) || (rc != SRETURN_SUSPEND)) {
            ism_common_free(ism_memory_protocol_misc,action);
            completeConnectionClosing(transport);
            return;
        }
    }

    ism_common_free(ism_memory_protocol_misc,action);

    completeConnectionClosing(transport);
    return;
}
#endif

/*
 * Close a connection
 */
int ism_fwd_closeConnection(ism_transport_t * transport, int rc, const char * reason) {
    ism_protobj_t * pobj = transport->pobj;
    int closed = __sync_fetch_and_or(&pobj->closed, 4);
    if (!closed) {
        transport->close(transport,rc, (rc==0), reason);
        return 0;
    }
    if (closed == 3) {
        completeConnectionClosing(transport);
    }
    return 0;
}

/*
 * The connection is closing.
 */
int ism_fwd_closing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    ism_protobj_t * pobj = transport->pobj;

    TRACEL(5, transport->trclevel, "Forwarder closing: connect=%u client=%s rc=%d clean=%d reason=%s\n",
            transport->index, transport->name, rc, clean, reason);

    /*
     * Set the indicator that close is in progress. If set failed,
     * then this has been done before and we don't need to proceed.
     */
    if (!__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
        return 0;
    }

    pthread_spin_lock(&pobj->sessionlock);
    pobj->closing = 1;
    pthread_spin_unlock(&pobj->sessionlock);

    /* Subtract the "in progress" indicator. If it becomes negative,
     * no actions are in progress, so it is safe to clean up protocol data
     * and close the connection. If it is non-negative, there are
     * actions in progress. The action that sets this value to 0
     * would re-invoke closing().
     */
    int count = __sync_sub_and_fetch(&pobj->inprogress, 1);
    if (count >= 0) {
        TRACEL(6, transport->trclevel, "ism_fwd_closing postponed as there are %d actions/messages in progress: connect=%u client=%s",
                count+1, transport->index, transport->name);
        return 0;
    } else {
        TRACEL(6, transport->trclevel, "ism_fwd_closing: inprogress=%d connect=%u client=%s",
                count, transport->index, transport->name);
    }

    /* Stop message delivery, destroy session and client state */
    ism_fwd_replyCloseClient(transport);

    return 0;
}


extern void ism_fwd_cleanPendingXAs(ism_protobj_t * pobj);

/*
 * The engine connection is closed, close our connection, and tell the transport it can close its connection
 */
void ism_fwd_replyDoneConnection(int32_t rc, void * handle, void * vaction) {
    ism_fwd_act_t * act = (ism_fwd_act_t *) vaction;
    ism_transport_t * transport = act->transport;
    ismFwdPobj_t * pobj = (ismFwdPobj_t *) transport->pobj;
    ism_fwd_channel_t * channel = pobj->channel;
    TRACEL(5, transport->trclevel, "close fwd connection(%s): index=%u name=%s\n",
            ((transport->originated) ? "outgoing" : "incoming"), transport->index, transport->name);
    ism_fwd_cleanPendingXAs(pobj);
    if(channel) {
        pthread_mutex_lock(&channel->lock);
        if (((channel->in_state == CHST_Open) && (channel->in_channel == transport)) ||
        		((channel->out_state == CHST_Open) && (channel->out_channel == transport)))
            ism_cluster_remoteServerDisconnected(channel->clusterHandle);
        if (transport->originated) {
            if (channel->out_channel == transport) {
                channel->out_state = CHST_Closed;
                channel->status_time = ism_common_currentTimeNanos();
                channel->out_channel = NULL;
                /* Set reconnect timer */
                if (!fwd_stopping && (channel->retry_timer == NULL) && (channel->cc_state == CHST_Open)) {
                    int retry = channel->retry;
                    uint64_t delay_ns;
                    if (retry < 500)
                        channel->retry = retry = 500;
                    else if (retry < 50000)
                        channel->retry *= 2;
                    delay_ns = (uint64_t)retry * 1000000;   /* milliseconds to nanoseconds */
                    channel->retry_timer = ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t) ism_fwd_retryOutgoing, channel, delay_ns);
                    // printf("set timer on disconnect: channel=%s ready=%u timer=%p\n", channel->name, transport->ready, channel->retry_timer);
                }
            }
        } else {
            if (channel->in_channel == transport) {
               channel->in_state = CHST_Closed;
               channel->status_time = ism_common_currentTimeNanos();
               channel->in_channel = NULL;
            }
        }
        pthread_mutex_unlock(&channel->lock);
    }
    pthread_spin_destroy(&pobj->lock);
    pthread_spin_destroy(&pobj->sessionlock);

    /* Tell the transport we are done */
    transport->closed(transport);
}

/*
 * Session closed.
 */
void ism_fwd_replyCloseClient(ism_transport_t * transport) {
    ismFwdPobj_t * pobj = (ismFwdPobj_t *) transport->pobj;
    ism_fwd_act_t act = { 0 };
    act.transport = transport;
    void * handle;

    if (!__sync_bool_compare_and_swap(&transport->pobj->closed, 1, 2)) {
        TRACEL(4, transport->trclevel, "ism_fwd_replyCloseClient called more than once for: index=%u name=%s\n",
                transport->index, transport->name);
        return;
    } else {
        TRACEL(6, transport->trclevel, "ism_fwd_replyCloseClient: index=%u name=%s transport=%p\n",
                transport->index, transport->name, transport);
    }

    ism_fwd_removeFromClientList(pobj, 1);

    ism_security_returnAuthHandle(transport->security_context);
    pthread_spin_lock(&pobj->sessionlock);
    pobj->session_handle = NULL;

    /* Ensure that we don't call ism_engine_destroyClientState twice. */
    handle = pobj->client_handle;
    pobj->client_handle = NULL;

    pthread_spin_unlock(&pobj->sessionlock);

    if (handle) {
        int rc = ism_engine_destroyClientState(handle,
                ismENGINE_DESTROY_CLIENT_OPTION_NONE, &act, sizeof(act),
                ism_fwd_replyDoneConnection);
        if (rc == ISMRC_AsyncCompletion)
            return;
    }
    ism_fwd_replyDoneConnection(0, NULL, &act);
}

/*
 * Add a connection to the list of clients
 */
void ism_fwd_addToClientList(ismFwdPobj_t * pobj) {
    TRACE(7, "ism_fwd_addToClientList: pobj=%p\n", pobj);
    pthread_spin_lock(&clientListLock);
    if (pobj->keepAlive == -1) {
        pobj->next = NULL;
        pobj->prev = clientListTail;
        if (clientListTail) {
            clientListTail->next = pobj;
        } else {
            clientListHead = pobj;
        }
        clientListTail = pobj;
    }
    if (pobj->keepAlive > -2)
        pobj->keepAlive = 0;

    pthread_spin_unlock(&clientListLock);
}

/*
 * Remove a connection from the list of clients
 */
void ism_fwd_removeFromClientList(ismFwdPobj_t * pobj, int lock) {
    TRACE(7, "ism_f=fwd_removeFromClientList: pobj=%p lock=%d\n", pobj, lock);
    if (lock)
        pthread_spin_lock(&clientListLock);
    if (pobj->keepAlive > -1) {
        if (pobj->prev) {
            pobj->prev->next = pobj->next;
        } else {
            clientListHead = pobj->next;
        }
        if (pobj->next) {
            pobj->next->prev = pobj->prev;
        } else {
            clientListTail = pobj->prev;
        }
        pobj->keepAlive = -1;
        pobj->next = pobj->prev = NULL;
    }
    pobj->keepAlive = -2;
    if (lock)
        pthread_spin_unlock(&clientListLock);
}


/*
 * This is called with the config lock held
 */
int ism_fwd_disconnectChannel(ism_fwd_channel_t * channel) {
    if (channel->retry_timer) {
        ism_common_cancelTimer(channel->retry_timer);
        channel->retry_timer = NULL;
    }
    if (channel->out_channel) {
        channel->out_channel->close(channel->out_channel, ISMRC_ClosedByServer, 0, "Remote server disconnecting");
    }
    if (channel->in_channel) {
        channel->in_channel->close(channel->out_channel, ISMRC_ClosedByServer, 0, "Remote server disconnecting");
    }
    return 0;
}


/*
 * Check if we should commit transaction based on time
 */
int ism_fwd_commitTimeCheck(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_fwd_channel_t * channel;
    double now = ism_common_readTSC();
    double check_time = now - 0.05;   /* If the transaction is more than 50 milliseconds old */
    pthread_mutex_lock(&fwd_configLock);
    if (fwd_startMessaging && !fwd_stopping) {
        channel = fwd_channelList;
        while (channel) {
            // TRACE(9, "check commit: channel=%s now=%g  check=%g start_xa=%g\n", channel->name, now, check_time, channel->start_xa);
            if (channel->start_xa && channel->start_xa < check_time) {
//                channel->start_xa = 0;
                ism_common_setTimerOnce(ISM_TIMER_HIGH, (ism_attime_t) ism_fwd_timedCommit, channel, 1);
            }
            if ((now - channel->old_send_time) > 10.0) {
                updateChannelRates(channel);
                channel->old_send_time = now;
                // printf("set old send time: %u  recv=%lu send0=%lu send1=%lu\n", (uint32_t)now,
                //        channel->old_recv, channel->old_send0, channel->old_send1);
            }
            channel = channel->next;
        }
    }
    pthread_mutex_unlock(&fwd_configLock);
    return 1;
}

/*
 * Replace a string
 */
void  ism_fwd_replaceString(const char * * oldval, const char * val) {
    if (!my_strcmp(*oldval, val))
        return;
    if (*oldval) {
        char * freeval = (char *)*oldval;
        if (val && !strcmp(freeval, val))
            return;
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),val);
        else
            *oldval = NULL;
        ism_common_free(ism_memory_protocol_misc,freeval);
    } else {
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),val);
        else
            *oldval = NULL;
    }
}

/*
 * Do a strcmp allowing for NULLs
 */
static int my_strcmp(const char * str1, const char * str2) {
    if (str1 == NULL || str2 == NULL) {
        if (str1 == NULL && str2 == NULL)
            return 0;
        return str1 ? 1 : -1;
    }
    return strcmp(str1, str2);
}

/*
 * Return names for fowarder actions
 */
const char * ism_fwd_getActionName(int action) {
    switch (action) {
    case FwdAction_Message         :  return "Message";
    case FwdAction_RMessage        :  return "RMessage";
    case FwdAction_Connect         :  return "Connect";
    case FwdAction_ConnectReply    :  return "ConnectReply";
    case FwdAction_Recover         :  return "Recover";
    case FwdAction_PrepareReply    :  return "PrepareReply";
    case FwdAction_CommitReply     :  return "CommitReply";
    case FwdAction_RollbackReply   :  return "RollbackReply";
    case FwdAction_Start           :  return "Start";
    case FwdAction_Prepare         :  return "Prepare";
    case FwdAction_Commit          :  return "Commit";
    case FwdAction_Rollback        :  return "Rollback";
    case FwdAction_Processed       :  return "Processed";
    case FwdAction_RequestRetain   :  return "RequestRetain";
    case FwdAction_CommitRecover   :  return "CommitReecover";
    case FwdAction_RollRecover     :  return "RollRecover";
    case FwdAction_Ping            :  return "Ping";
    case FwdAction_Pong            :  return "Pong";
    case FwdAction_Disconnect      :  return "Disconnect";
    }
    return "Unknown";
}

