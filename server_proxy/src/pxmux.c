/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

#define TRACE_COMP Mux

#include <pxtransport.h>
#include <pxtcp.h>
#include <tenant.h>
#include <ismutil.h>
#include <assert.h>
#include <errno.h>
enum muxcmd_e {
    MuxData              = 1,
    MuxCreateStream      = 2,
    MuxCloseStream       = 3,
    MuxCreatePhysical    = 'M',
    MuxCreatePhysicalAck = 4,
    MuxClosePhysical     = 5
};
#define MAX_FRAME_SIZE  32
static int numOfIOPs = 0;
typedef struct conReq_t {
    ism_server_t        *   server;
    int                     index;
} conReq_t;

typedef struct ism_protobj_t {
    ism_server_t        *   server;
    ismArray_t              vcArray;
    conReq_t            *   pReq;
    pthread_spinlock_t      lock;
    int                     index;
    uint8_t                 closed;
    int                     rc;
    const char *            reason;
} ism_protobj_t;

typedef ism_protobj_t mux_pobj_t;

#define    VC_OPEN                  0x1
#define    VC_CLOSED_BY_CLIENT      0x2
#define    VC_CLOSED_BY_SERVER      0x4
#define    PC_CLOSED                0x8

typedef struct vcInfo_t {
    ism_transport_t *   transport;
    uint8_t             state;
} vcInfo_t;

/*
 * Stats
 */
static px_mux_stats_t * muxStats=NULL;  //MUX Stats per IoP


static const char * instanceID = NULL;
static char      proxyInfo[1024];
static uint16_t  proxyInfoLength;
static int vcClosed(ism_transport_t * transport);
static int vcClose(ism_transport_t * transport, int rc, int clean, const char * reason);
static int muxSend(ism_transport_t * transport, char * buf, int len, int protval, int flags);
static void completePhysicalConnectionClose(ism_transport_t * transport);
static void handlePhysicalConnectionClose(conReq_t * pReq, ism_time_t delay) ;
extern ism_transport_t * ism_transport_getPhysicalTransport(ism_transport_t * transport);
int ism_transport_stopped(void);
/*
 * Return the name of a command given the value.
 * This is used for trace and debug
 */
static const char * muxCommand(int ix) {
    switch (ix) {
    case MuxData:           return "Data";
    case MuxCreateStream:   return "CreateStream";
    case MuxCloseStream:    return "CloseStream";
    case MuxCreatePhysical: return "CreatePhysical";
    case MuxCreatePhysicalAck: return "CreatePhysicalAck";
    case MuxClosePhysical:  return "ClosePhysical";
    }
    return "UNKNOWN";
}


extern  int ism_tcp_getIOPcount(void);
XAPI void ism_transport_muxInit(int usemux) {
    numOfIOPs = ism_tcp_getIOPcount();
    instanceID = getenv("INSTANCE_ID");
    if(instanceID == NULL) {
        instanceID = "msproxy";
    }
    sprintf(proxyInfo,"%s %s %s", ism_common_getVersion(), ism_common_getBuildLabel(), ism_common_getBuildTime());
    proxyInfoLength = strlen(proxyInfo);

    /*Initialize Stats object*/
    muxStats = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_mux_stats,1),numOfIOPs, sizeof(px_mux_stats_t));
}

extern int ism_transport_createMuxConnection(ism_transport_t * transport);
static int muxReceive(ism_transport_t * transport, char * data, int datalen, int kind);

static int closeRequestJob(ism_transport_t * transport, void * param1, uint64_t param2) {
    int reqCount = 0;
    TRACE(8, "closeRequestJob > : transport=%p\n", transport);
    transport->pobj->closed = 2;
    if(ism_common_getArrayNumElements(transport->pobj->vcArray)) {
        uint16_t i = 0;
        for (i = 0xffff; i > 0; i--) {
            vcInfo_t * vcInfo = ism_common_getArrayElement(transport->pobj->vcArray, i);
            if (vcInfo) {
                if (vcInfo->state & VC_CLOSED_BY_CLIENT) {
                    ism_common_removeArrayElement(transport->pobj->vcArray, i);
                    ism_common_free(ism_memory_proxy_mux_virtual_connection,vcInfo);
                } else {
                    int rc = transport->pobj->rc;
                    const char * reason = transport->pobj->reason;
                    if (rc == 0)
                        rc = ISMRC_ClosedByServer;
                    if (!reason)
                        reason = "Physical connection closed";
                    vcInfo->state |= PC_CLOSED;
                    vcInfo->transport->close(vcInfo->transport, rc, 0, reason );
                    reqCount++;
                }
            }
        }
    }
    if(!reqCount)
        completePhysicalConnectionClose(transport);
    TRACE(8, "closeRequestJob < : transport=%p reqCount=%d\n", transport, reqCount);
    return 0;
}

static int muxClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    ism_server_t * server = transport->server;
    mux_pobj_t * pobj = (mux_pobj_t*)transport->pobj;
    serverConnection_t * pSC = &server->mux[transport->pobj->index];
    TRACE(5, "muxClosing: transport=%p connect=%d rc=%d(%s) server=%p pSC=%p state=%d\n", transport, transport->index, rc, reason, server, pSC, pSC->state);
    pthread_spin_lock(&pSC->lock);
    if ((pSC->state == TCP_DISCONNECTED) ||
        (pSC->state == TCP_DISCONNECTING)) {
        pthread_spin_unlock(&pSC->lock);
        return 0;
    }
    if (pSC->state == TCP_CON_IN_PROCESS) {
        pSC->state = TCP_DISCONNECTED;
        pSC->transport = NULL;
        pSC->useCount = 0;
        pthread_spin_unlock(&pSC->lock);
        transport->closed(transport);
        //restart connection
        handlePhysicalConnectionClose(pobj->pReq, 10000000000);
        return 0;
    }
    pSC->useCount--;
    pSC->state = TCP_DISCONNECTING;
    if (pSC->useCount) {
        pthread_spin_unlock(&pSC->lock);
        return 0;
    }
    transport->pobj->rc = rc;
    transport->pobj->reason = ism_transport_putString(transport, reason);
    pthread_spin_unlock(&pSC->lock);
    ism_transport_submitAsyncJobRequest(transport, closeRequestJob, NULL, 0);
    return 0;
}



static void handlePhysicalConnectionClose(conReq_t * pReq, ism_time_t delay);

static void completePhysicalConnectionClose(ism_transport_t * transport) {
    ism_server_t * server = transport->server;
    mux_pobj_t * pobj = (mux_pobj_t*)transport->pobj;
    serverConnection_t * pSC = &server->mux[pobj->index];
    int count = 0;
    if (pobj->vcArray)
        count = ism_common_getArrayNumElements(pobj->vcArray);


    TRACE(5, "Complete close of MUX connection: connect=%u name=%s count=%u\n", transport->index, transport->name, count);
    pthread_spin_lock(&pSC->lock);
    if(transport == pSC->transport) {
        pSC->state = TCP_DISCONNECTED;
        pSC->transport = NULL;
    }
    pthread_spin_unlock(&pSC->lock);
    if(pobj->vcArray)
        ism_common_destroyArray(pobj->vcArray);
    pobj->vcArray = NULL;
    handlePhysicalConnectionClose(pobj->pReq, 10000000000);
    transport->closed(transport);
}


/*
 * Notification of the outgoing connection complete.
 * If there is a saved packet send it at this point.
 */
static int muxConnectionComplete(ism_transport_t * transport, int rc) {
    ism_server_t * server = transport->server;
    serverConnection_t * pSC = &server->mux[transport->pobj->index];
    TRACE(5, "Outgoing MUX connection complete: connect=%u name=%s ipaddr=%s port=%u rc=%d\n",
            transport->index, transport->name, transport->server_addr, transport->serverport, rc);
    if (rc == 0) {
        ism_muxHdr_t hdr = {0};
        char * buff;
        char * ptr;
        int totalLen, nameLen, len;
        char * name = alloca(strlen(instanceID)+16);
        sprintf(name,"%s.%d",instanceID, transport->tid);
        nameLen = strlen(name);
        totalLen = proxyInfoLength + nameLen + 5;
        buff = alloca(totalLen+16);
        ptr = buff+16;
        *ptr = 1; //version;
        ptr++;
        len = htons(nameLen);
        memcpy(ptr, &len, 2);
        ptr += 2;
        memcpy(ptr, name, nameLen);
        ptr += nameLen;
        len = htons(proxyInfoLength);
        memcpy(ptr, &len, 2);
        ptr += 2;
        memcpy(ptr, proxyInfo, proxyInfoLength);
        ptr += proxyInfoLength;
        hdr.hdr.cmd = MuxCreatePhysical;
        hdr.hdr.stream = 0x5558; //"XU"
        transport->ready = 5;
        transport->state = ISM_TRANST_Open;
#ifndef HAS_BRIDGE
        ism_server_setLastGoodAddress(transport->server, transport->connect_order);
#endif
        pthread_spin_lock(&pSC->lock);
        pSC->state = PROTOCOL_CON_IN_PROCESS;
        pthread_spin_unlock(&pSC->lock);
        /* Send the MuxCreatePhysical cmd */
        transport->send(transport, buff+16, totalLen, hdr.iValue, SFLAG_FRAMESPACE);
    } else {
        completePhysicalConnectionClose(transport);
    }
    return 0;
}

static inline ima_pxtransport_t * muxGetServerConnection(ism_server_t * server, int index) {
    ima_pxtransport_t * transport = NULL;
    pthread_spin_lock(&server->mux[index].lock);
    if(LIKELY(server->mux[index].transport && (server->mux[index].state == PROTOCOL_CONNECTED))) {
        transport = server->mux[index].transport;
        server->mux[index].useCount++;
        TRACE(8, "muxGetServerConnection: transport=%p useCount=%d\n", transport, server->mux[index].useCount);
    }
    pthread_spin_unlock(&server->mux[index].lock);
    return transport;
}

static inline void muxFreeServerConnection(ima_pxtransport_t * transport) {
    ism_server_t * server = transport->server;
    mux_pobj_t * pobj = transport->pobj;
    int shouldClose = 0;
    pthread_spin_lock(&server->mux[pobj->index].lock);
    server->mux[pobj->index].useCount--;
    if (server->mux[pobj->index].useCount == 0) {
        shouldClose = 1;
    }
    TRACE(8, "muxFreeServerConnection: transport=%p useCount=%d\n", transport, server->mux[pobj->index].useCount);
    pthread_spin_unlock(&server->mux[pobj->index].lock);
    if (shouldClose) {
        ism_transport_submitAsyncJobRequest(transport, closeRequestJob, NULL, 0);
    }
}

extern void ism_tcp_init_transport(ism_transport_t * transport);
/*
 * Construct the outgoing connection
 */
int ism_mux_serverConnect(conReq_t * pReq) {
    ism_server_t * server = pReq->server;
    int index = pReq->index;
    ism_transport_t * transport;
    mux_pobj_t * pobj;

    if (ism_transport_stopped()) {
        TRACE(5, "Cannot restart mux %s:%u because the pxory is shutting down\n", server->name, index);
        return ISMRC_ServerTerminating;
    }
    /*
     * Create outgoing connection
     */
    transport = ism_transport_newOutgoing(NULL, 1);
    ism_tcp_init_transport(transport);
    transport->originated = 2;
    transport->protocol = "mux";
    transport->protocol_family = "amux";
    pobj = (mux_pobj_t *) ism_transport_allocBytes(transport, sizeof(mux_pobj_t), 1);
    memset(pobj, 0, sizeof(mux_pobj_t));
    pthread_spin_init(&pobj->lock, 0);
    pobj->vcArray = ism_common_createArray(0xffff);
    if(pobj->vcArray == NULL) {
        handlePhysicalConnectionClose(pReq, 10000000000);
        ism_transport_freeTransport(transport);
        return ISMRC_OK;
    }
    pobj->server = server;
    pobj->index = index;
    pobj->pReq = pReq;
    transport->pobj = pobj;
    transport->receive = muxReceive;
    transport->actionname = muxCommand;
    transport->tid = index;
    transport->connected = muxConnectionComplete;
    transport->closing = muxClosing;
    pthread_spin_lock(&server->lock);
    transport->server = server;
    server->mux[index].transport = transport;
    server->mux[index].state = TCP_CON_IN_PROCESS;
    server->mux[index].useCount = 1;
//    pobj->activeRequestsCount = 0;
    server->mux[index].index = index;
    pthread_spin_unlock(&server->lock);

    char * temp = alloca(strlen(server->name) + 2412);
    sprintf(temp, "%s:%u", server->name, index);
    transport->name = ism_transport_putString(transport, temp);
    transport->clientID = transport->name;

    __sync_add_and_fetch(&muxStats[pReq->index].physicalConnectionsTotal, 1);
//    TRACE(7, "Make outgoing connection to server=%s transport=%p\n", server->name, transport);
    if(ism_transport_createMuxConnection(transport))
        completePhysicalConnectionClose(transport);

    return ISMRC_OK;
}

static int startMuxConnectionTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    conReq_t * pReq = ((conReq_t*) userdata);
    ism_server_t * server = pReq->server;
    if(!server->mux[pReq->index].disabled)
        ism_mux_serverConnect(pReq);
    ism_common_cancelTimer(timer);
    return 0;
}

static void handlePhysicalConnectionClose(conReq_t * pReq, ism_time_t delay) {
	__sync_sub_and_fetch(&muxStats[pReq->index].physicalConnectionsTotal, 1);
	ism_common_setTimerOnce(ISM_TIMER_HIGH, startMuxConnectionTimer, (void*) pReq, delay);
}


int ism_transport_startMuxConnections(ism_server_t * server) {
    int i;
    for(i = 0; i < numOfIOPs; i++) {
        conReq_t * pReq = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_mux_connection,1),sizeof(conReq_t));
        pReq->server = server;
        pReq->index = i;
        ism_common_setTimerOnce(ISM_TIMER_HIGH, startMuxConnectionTimer, (void*) pReq, 1000000000);
    }
    return 0;
}

static int muxReceive(ism_transport_t * transport, char * data, int datalen, int kind) {
    ism_muxHdr_t hdr = {0};
    int cmd;
    uint16_t stream;
    mux_pobj_t * pobj = transport->pobj;
    ism_server_t * server = transport->server;
    vcInfo_t * vcInfo = NULL;
    int rc = ISMRC_OK;
    hdr.iValue = kind;
    cmd = hdr.hdr.cmd;
    stream = hdr.hdr.stream;
    if (pobj->closed) {
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }
    /* Trace */
    if (SHOULD_TRACEC(9, Mux)) {
        char xbuf [128];
        int maxsize = ism_common_getTraceMsgData();
        sprintf(xbuf, "mux recv %d %s connect=%u stream=%u",
                cmd, transport->actionname(cmd), transport->index, stream);
        traceDataFunction(xbuf, 0, __FILE__, __LINE__, data, datalen, maxsize);
    }
    switch (cmd) {
    case MuxData:
        vcInfo = ism_common_getArrayElement(pobj->vcArray, stream);
        assert(vcInfo != NULL);
        if(vcInfo && (vcInfo->state == VC_OPEN)) {
            int used = 0;
            while (!rc && used < datalen) {
                rc = vcInfo->transport->frame(vcInfo->transport, data, used, datalen, &used);
            }
            vcInfo->transport->read_bytes += datalen;
            if (rc) {
                if(vcInfo->transport->state == ISM_TRANST_Open) {
                    /* Error */
                    TRACE(3, "Frame error: index=%u cmd=%u stream=%u rc=%d\n", vcInfo->transport->index, cmd, stream, rc);
                } else {
                    TRACE(7, "Virtual connection was closed from callback: index=%u cmd=%u stream=%u rc=%d\n", vcInfo->transport->index, cmd, stream, rc);
                }
                rc = ISMRC_OK; /* Do not close physical connection */
            }
        } else {
            /* Error */
            if(vcInfo) {
                TRACE(6, "Stream state is not valid: index=%u cmd=%u name=%s stream=%u state=0x%x\n", transport->index, cmd, transport->name, stream, vcInfo->state);
            } else {
                TRACE(3, "Stream not found: index=%u cmd=%u name=%s stream=%u\n", transport->index, cmd, transport->name, stream);
            }
        }
        break;
    case MuxCloseStream:
        vcInfo = ism_common_getArrayElement(pobj->vcArray, stream);
        assert(vcInfo != NULL);
        if(vcInfo) {
            if(vcInfo->state == VC_OPEN) {
                int xrc = ISMRC_ServerNotAvailable;
                if(datalen > 3) {
                    memcpy(&xrc, data, sizeof(xrc));
                    xrc = ntohl(xrc);
                }
                vcInfo->state |= VC_CLOSED_BY_SERVER;
                vcInfo->transport->close(vcInfo->transport, xrc, 0, "Connection closed by server");
            } else {
                if(vcInfo->state & VC_CLOSED_BY_CLIENT) {
                    ism_common_removeArrayElement(pobj->vcArray, stream);
                    __sync_sub_and_fetch(&muxStats[transport->tid].virtualConnectionsTotal, 1);
                    TRACE(8, "MuxCloseStream:  transport_index=%u transport_name=%s transport->tid=%d VirtualConnectionsTotal=%lu\n",
                                                transport->index, transport->name, transport->tid, muxStats[transport->tid].virtualConnectionsTotal);
                    ism_common_free(ism_memory_proxy_mux_virtual_connection,vcInfo);
                }
            }
        } else {
            TRACE(3, "Stream not found: index=%u cmd=%u name=%s stream=%u\n", transport->index, cmd, transport->name, stream);
        }
        break;
    case MuxCreatePhysicalAck:
        pthread_spin_lock(&server->mux[pobj->index].lock);
        server->mux[pobj->index].state = PROTOCOL_CONNECTED;
#ifndef HAS_BRIDGE
        ism_server_setLastGoodAddress(transport->server, transport->connect_order);
#endif
        pthread_spin_unlock(&server->mux[pobj->index].lock);
        break;
    case MuxCreatePhysical:
    case MuxCreateStream:
    default:
        TRACE(4,"Unexpected command for multiplex protocol: %d %s connect=%u\n",cmd, transport->actionname(cmd), transport->index);
        transport->close(transport, ISMRC_BadClientData, 0, "Unexpected command");
        rc = ISMRC_BadClientData;
        break;
    }
    return rc;
}


/*
 * Virtual stream closing
 */
static int vcClose(ism_transport_t * transport, int rc, int clean, const char * reason) {
    if (!transport)
        return 1;

    // printf("close %s state=%d family=%s\n", transport->name, transport->state, transport->protocol_family);
    if (!(__sync_bool_compare_and_swap(&transport->state, ISM_TRANST_Open, ISM_TRANST_Closing)
            || __sync_bool_compare_and_swap(&transport->state, ISM_TRANST_Opening, ISM_TRANST_Closing))) {
        TRACE(9, "The stream cannot close due to state: connect=%u name=%s state=%u\n",
                transport->index, transport->name, transport->state);
        return 1;
    }
    if (!reason)
        reason = "";

    TRACE(8, "vcClose: connect=%u tid=%d currentVCCount=%llu name=%s reason=%s\n", transport->index,transport->tid, (ULL)muxStats[transport->tid].virtualConnectionsTotal, transport->name, reason);
    uint32_t uptime = (uint32_t)(((ism_common_currentTimeNanos()-transport->connect_time)+500000000)/1000000000);  /* in seconds */
    TRACE(5, "Closing virtual connection (CWLNA1121): connect=%u name=%s protocol=%s endpoint=\"%s\""
            " From=%s:%u UserID=\"%s\" Uptime=%u RC=%d Reason\"%s\""
            " ReadBytes=%llu ReadMsg=%llu WriteBytes=%llu WriteMsg=%llu LostMsg=%llu\n",
        transport->index, transport->name, transport->protocol, transport->endpoint_name,
        transport->client_addr, transport->clientport, transport->userid ? transport->userid : "",
        uptime, rc, reason, (ULL)transport->read_bytes, (ULL)transport->read_msg,
        (ULL)transport->write_bytes, (ULL)transport->write_msg, (ULL)transport->lost_msg);

    /*
     * Call the protocol
     */
    if (transport->closing != NULL) {
        transport->closing(transport, rc, clean, reason);
    } else {
        vcClosed(transport);
    }
    return 0;
}

static int sendCloseStream(ism_transport_t * transport, uint16_t stream, int rc) {
    ism_muxHdr_t hdr = {0};
    char * buff = alloca(32);
    rc = htonl(rc);
    buff += 16;
    memcpy(buff, &rc, sizeof(rc));
    hdr.hdr.cmd = MuxCloseStream;
    hdr.hdr.stream = stream;
    transport->send(transport, buff, sizeof(rc), hdr.iValue, SFLAG_FRAMESPACE);
    return ISMRC_OK;
}

static int sendCreateStream(ism_transport_t * transport, uint16_t stream) {
    ism_muxHdr_t hdr = {0};
    char * buff = alloca(32);
    buff += 16;
    hdr.hdr.cmd = MuxCreateStream;
    hdr.hdr.stream = stream;
    transport->send(transport, buff, 0, hdr.iValue, SFLAG_FRAMESPACE);
    return ISMRC_OK;
}

static int vcCloseJob(ism_transport_t * transport, void * param1, uint64_t param2) {
    ism_transport_t * vcTran = (ism_transport_t *) param1;
    ism_transport_t * mxTran =  ism_transport_getPhysicalTransport(vcTran);

    vcInfo_t * vcInfo = ism_common_getArrayElement(mxTran->pobj->vcArray, vcTran->virtualSid);
    assert(vcInfo != NULL);
    assert(vcInfo->transport == vcTran);
    if(vcInfo) {
        TRACE(8, "vcCloseJob: vcIndex=%u vcName=%s sid=%u mxIndex=%u mxName=%s rc=%d\n",
                vcTran->index, vcTran->name, vcTran->virtualSid, mxTran->index,
                mxTran->name, vcTran->closestate[3]);
        if (vcInfo->state == VC_OPEN) {
        	sendCloseStream(mxTran, vcTran->virtualSid, vcTran->closestate[3]);
            vcInfo->state |= VC_CLOSED_BY_CLIENT;
            vcInfo->transport = NULL;
        } else {
        	ism_common_removeArrayElement(mxTran->pobj->vcArray, vcTran->virtualSid);
        	__sync_sub_and_fetch(&muxStats[transport->tid].virtualConnectionsTotal, 1);
            TRACE(8, "vcCloseJob: after removal: transport_index=%u transport_name=%s transport->tid=%d VirtualConnectionsTotal=%lu\n",
                        transport->index, transport->name, transport->tid, muxStats[transport->tid].virtualConnectionsTotal);
        	ism_common_free(ism_memory_proxy_mux_virtual_connection,vcInfo);

        }
        /* Free up the virtual transport object */
        ism_transport_freeTransport(vcTran);

        if (mxTran->pobj->closed == 2) {
            if(ism_common_getArrayNumElements(mxTran->pobj->vcArray) == 0) {
                completePhysicalConnectionClose(mxTran);
            }
        }
    }
    return 0;
}

/*
 * Virtual stream closed
 */
static int vcClosed(ism_transport_t * transport) {
    ism_transport_t * mxTran =  ism_transport_getPhysicalTransport(transport);
    assert(transport->virtualSid != 0);
    TRACE(8, "vcClosed: vcIndex=%u vcName=%s sid=%u mxIndex=%u mxName=%s\n",
            transport->index, transport->name, transport->virtualSid, mxTran->index, mxTran->name);
    ism_transport_submitAsyncJobRequest(transport, vcCloseJob, transport, 0);
    return 0;
}

XAPI int ism_transport_InitialHandshake(ism_transport_t * transport, char * buffer, int pos, int avail, int * used);
XAPI int ism_transport_nextConnectID(void);

/*
 * Create an incoming virtual connection object
 */
static  ism_transport_t * createVirtualConnection(ism_transport_t * transport, int * pRC, char * errMsg) {
    mux_pobj_t * pobj = transport->pobj;
    vcInfo_t   * vcInfo = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_mux_virtual_connection,2),sizeof(vcInfo_t));
    if(vcInfo) {
        uint16_t streamID = ism_common_addToArray(pobj->vcArray,vcInfo);

        if(streamID) {
            ism_transport_t * vtransport = ism_transport_newTransport(transport->endpoint, 0, 1);
            if(vtransport) {
                vcInfo->state = VC_OPEN;
                vcInfo->transport = vtransport;
                vtransport->tobj = (struct ism_transobj *) transport;
                vtransport->virtualSid = streamID;
                vtransport->close = vcClose;
                vtransport->closed = vcClosed;
                vtransport->send = muxSend;
                vtransport->index = ism_transport_nextConnectID();
                vtransport->frame = ism_transport_InitialHandshake;
                vtransport->client_addr = transport->client_addr;
                vtransport->server_addr = transport->server_addr;
                vtransport->clientport = transport->clientport;
                vtransport->serverport = transport->serverport;
                vtransport->userid = transport->userid;
                vtransport->tid = transport->tid;
                vtransport->state = ISM_TRANST_Open;
                vtransport->originated = 1;
                sendCreateStream(transport, streamID);
                __sync_add_and_fetch(&muxStats[transport->tid].virtualConnectionsTotal, 1);
                TRACE(8, "createVirtualConnection: transport_index=%u transport_name=%s transport->tid=%d virtualConnectionsTotal=%lu\n",
                                    transport->index, transport->name, transport->tid, muxStats[transport->tid].virtualConnectionsTotal);
                return vtransport;
            } else {
                *pRC = ISMRC_AllocateError;
                strcpy(errMsg, "Memory allocation error");
            }
        } else {
            *pRC = ISMRC_ServerCapacity;
            strcpy(errMsg, "Too many virtual connections");
            int array_elements_count = ism_common_getArrayNumElements(pobj->vcArray);
            TRACE(5, "Failed to create the virtual connection. Max Virtual Connections is reached. transport_index=%u transport_name=%s VC_Array_Element_Count=%d\n",
                               transport->index, transport->name, array_elements_count);
        }
        ism_common_free(ism_memory_proxy_mux_virtual_connection,vcInfo);
    } else {
        *pRC = ISMRC_AllocateError;
        strcpy(errMsg, "Memory allocation error");
    }
    return NULL;
}

XAPI  ism_transport_t * ism_mux_createVirtualConnection(ism_server_t * server, int tid, int * pRC, char * errMsg) {
    ism_transport_t * vcTran = NULL;
    ism_transport_t * mxTran = muxGetServerConnection(server, tid);
    if(mxTran) {
        vcTran = createVirtualConnection(mxTran, pRC, errMsg);
        muxFreeServerConnection(mxTran);
    } else {
        *pRC = ISMRC_ServerNotAvailable;
        strcpy(errMsg, "No connection to server");
    }
    return vcTran;
}


/*
 * Send bytes in a virtual
 * This is set on the virtual transport object
 */
static int muxSend(ism_transport_t * transport, char * buf, int len, int protval, int flags) {
    if(transport->virtualSid) {
        ism_transport_t * mxTran =  ism_transport_getPhysicalTransport(transport);
        char * freePtr = NULL;
        ism_muxHdr_t hdr = {0};
        hdr.hdr.cmd = MuxData;
        hdr.hdr.stream = transport->virtualSid;
        TRACE(9, "muxSend: transport=%p clientID=%s sid=%u len=%d protval=%d flags=%d\n",
                transport, transport->clientID, transport->virtualSid, len, protval, flags);
        /*
         * We need to have space in the buffer for the mux header, so copy buffer if needed
         */
        if (UNLIKELY((flags & SFLAG_HASFRAME) || !(flags & SFLAG_FRAMESPACE))) {
            char * tmpBuf;
            if((len + MAX_FRAME_SIZE) > 16384) {
                tmpBuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_utils,95),len+MAX_FRAME_SIZE);
                freePtr = tmpBuf;
            } else {
                tmpBuf = alloca(len+MAX_FRAME_SIZE);
            }
            if(!tmpBuf)
                return SRETURN_ALLOC;
            memcpy(tmpBuf+MAX_FRAME_SIZE, buf,len);
            buf = tmpBuf+MAX_FRAME_SIZE;
        }
        /*
         * Add the "real" protocol frame
         */
        if (LIKELY(!(flags & SFLAG_HASFRAME))) {
            int flen = transport->addframe(transport, buf, len, protval);
            buf -= flen;
            len += flen;
        }
        mxTran->send(mxTran, buf, len, hdr.iValue, SFLAG_FRAMESPACE);

        if (UNLIKELY(freePtr != NULL))
            ism_common_free(ism_memory_proxy_utils,freePtr);
        __sync_add_and_fetch(&transport->write_bytes, len);
        return SRETURN_OK;
    }
    return SRETURN_BAD_STATE;
}

/**
 *Get MUX stats
 */
int ism_proxy_getMuxStats(px_mux_stats_t * stats, int * pCount) {
    int count = 0;
    if(*pCount < numOfIOPs){
        *pCount = numOfIOPs;
        return 1;
    }
    for(count=0; count< numOfIOPs; count++){
    	memcpy(&stats[count], &muxStats[count], sizeof(px_mux_stats_t));
    }

    *pCount = count;
    return 0;
}


/**
 * Check if Server Connection is available
 * @param server
 * @param index
 * @return 1 for Server Connection is available. 0 is not
 */
int ism_mux_checkServerConnection(ism_server_t * server, int index) {
    int retValue=0;
    pthread_spin_lock(&server->mux[index].lock);
    if(LIKELY(server->mux[index].transport && (server->mux[index].state == PROTOCOL_CONNECTED))) {
        retValue=1;
    }
    pthread_spin_unlock(&server->mux[index].lock);
    TRACE(8, "ism_proxy_muxCheckServerConnection: index=%d available=%d\n", index, retValue);
    return retValue;
}
