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

#ifndef TRACE_COMP
#define TRACE_COMP Mux
#endif
#include <pthread.h>
#include <ismutil.h>
#include <transport.h>
#include <security.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

enum muxcmd_e {
    MuxData              = 1,
    MuxCreateStream      = 2,
    MuxCloseStream       = 3,
    MuxCreatePhysical    = 'M',
    MuxCreatePhysicalAck = 4,
    MuxClosePhysical     = 5
};

/*
 * Protocol specific object
 */
typedef struct ism_protobj_t {
    ism_transport_t *       transport;
    ism_transport_t * *     streams;
    pthread_spinlock_t      lock;
    uint16_t                streamCount;
    volatile uint8_t        closed;            /* Connection is not is use */
    uint8_t                 clientVersion;
    int                     rc;
    const char *            reason;
} mux_pobj_t;

#define MUX_SERVER_VERSION      1
#define MAX_VC_ID      0xffff
#define MAX_FRAME_SIZE  32

static char      serverInfo[1024];
static uint16_t  serverInfoLength;
extern ism_transport_t * ism_transport_getPhysicalTransport(ism_transport_t * transport);
extern int ism_transport_nextConnectID(void);
extern ism_transport_frame_t ism_transport_getHandshake(void);
static int vcClosed(ism_transport_t * transport);
/*
 * Send bytes in a virtual
 * This is set on the virtual transport object
 */
static int muxSend(ism_transport_t * transport, char * buf, int len, int protval, int flags) {
    ism_transport_t * mxTran = NULL;
    ism_transport_t * chkTran = NULL;
    if(transport->virtualSid) {
        mxTran =  ism_transport_getPhysicalTransport(transport);
        chkTran = mxTran->pobj->streams[transport->virtualSid];
        if(chkTran == transport) {
            char * freePtr = NULL;
            ism_muxHdr_t hdr = {0};
            hdr.hdr.cmd = MuxData;
            hdr.hdr.stream = transport->virtualSid;
            TRACE(9, "muxSend: transport=%p sid=%u len=%d protval=%d flags=%d\n",
                    transport, transport->virtualSid, len, protval, flags);
           /*
             * We need to have space in the buffer for the mux header, so copy buffer if needed
             */
            if (UNLIKELY((flags & SFLAG_HASFRAME) || !(flags & SFLAG_FRAMESPACE))) {
                char * tmpBuf;
                if((len + MAX_FRAME_SIZE) > 16384) {
                    tmpBuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,57),len+MAX_FRAME_SIZE);
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
                ism_common_free(ism_memory_protocol_misc,freePtr);
            __sync_add_and_fetch(&transport->write_bytes, len);
            return SRETURN_OK;
        }
    }
    TRACE(6, "muxSend - return BAD_STATE: transport=%p mxTran=%p chkTran=%p sid=%u len=%d protval=%d flags=%d\n",
            transport, mxTran, chkTran, transport->virtualSid, len, protval, flags);
    return SRETURN_BAD_STATE;

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
        TRACE(6, "The stream cannot close due to state: connect=%u name=%s state=%u\n",
                transport->index, transport->name, transport->state);
        return 1;
    }
    if (!reason)
        reason = "";

    TRACE(8, "vcClose: vsTran=%p mxTran=%p stream=%u connect=%u clientID=%s reason=%s\n", transport, transport->tobj, transport->virtualSid,
            transport->index, transport->clientID, reason);
    uint32_t uptime = (uint32_t)(((ism_common_currentTimeNanos()-transport->connect_time)+500000000)/1000000000);  /* in seconds */
    TRACE(5, "Closing virtual connection (CWLNA1111): connect=%u name=%s protocol=%s endpoint=\"%s\""
            " From=%s:%u UserID=\"%s\" Uptime=%u RC=%d Reason\"%s\""
            " ReadBytes=%llu ReadMsg=%llu WriteBytes=%llu WriteMsg=%llu LostMsg=%llu WarnMsg=%llu\n",
        transport->index, transport->name, transport->protocol, transport->endpoint_name,
        transport->client_addr, transport->clientport, transport->userid ? transport->userid : "",
        uptime, rc, reason, (ULL)transport->read_bytes, (ULL)transport->read_msg,
        (ULL)transport->write_bytes, (ULL)transport->write_msg, (ULL)transport->lost_msg, (ULL)transport->warn_msg);

    __sync_add_and_fetch(&transport->listener->stats->connect_active, -1);
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

static int sendAck(ism_transport_t * transport) {
    ism_muxHdr_t hdr = {0};
    char * info = alloca(1024);
    uint16_t length;
    info += 16;
    info[0] = MUX_SERVER_VERSION;
    length = htons(serverInfoLength);
    memcpy(info+1, &length, 2);
    memcpy(info+3, serverInfo, serverInfoLength);
    hdr.hdr.cmd = MuxCreatePhysicalAck;
    hdr.hdr.stream = 0;
    transport->send(transport, info, serverInfoLength+3, hdr.iValue, SFLAG_FRAMESPACE);
    return ISMRC_OK;
}

/*
 * Job scheduled on closed() call
 */
static int vcCloseJob(ism_transport_t * transport, void * param1, uint64_t param2) {
    ism_transport_t * vcTran = (ism_transport_t *) param1;

    if(vcTran == NULL || vcTran->virtualSid == 0){
        //This Virtual Connection already closed.
        TRACE(5, "vcCloseJob: The Virtual Connection is already closed. vcIndex=%u\n", vcTran->index);
        return 0;
    }
    ism_transport_t * mxTran =  ism_transport_getPhysicalTransport(vcTran);
    int rc = ISMRC_ClosedByServer;
    switch (vcTran->closestate[3]) {
        case 1: rc = 0;                      break;
        case 2: rc = ISMRC_BadClientID;      break;
        case 3: rc = ISMRC_ServerCapacity;   break;
        case 5: rc = ISMRC_NotAuthorized;    break;
        case 9: rc = ISMRC_ClosedOnSend;     break;
    }

    TRACE(6, "vcCloseJob: vcIndex=%u vcName=%s sid=%u mxIndex=%u mxName=%s rc=%d\n",
            vcTran->index, vcTran->name, vcTran->virtualSid, mxTran->index,
            mxTran->name, rc);
    sendCloseStream(mxTran, vcTran->virtualSid, rc);
    transport->pobj->streams[vcTran->virtualSid] = NULL;
    //Set the virtualSID to zero for this transport.
    vcTran->virtualSid=0;
    /* Free up the transport object */
    if(vcTran->security_context) {
        ism_security_destroy_context(vcTran->security_context);
        vcTran->security_context = NULL;
    }
    ism_transport_removeMonitorNow(vcTran);
    ism_transport_freeTransport(vcTran);
    transport->pobj->streamCount--;

    if(transport->pobj->closed == 2) {
        if(transport->pobj->streamCount == 0) {
            if(transport->pobj->streams) {
                ism_common_free(ism_memory_protocol_misc,transport->pobj->streams);
                transport->pobj->streams = NULL;
                transport->closed(transport);
            }
         }
    }
    return 0;
}

static int vcCloseWork(ism_transport_t * transport, void * param, int flags) {
    ism_transport_t * vcTran = (ism_transport_t *) param;
    TRACE(6, "vcCloseWork: vcIndex=%u vcName=%s sid=%u workCount=%d mxIndex=%u mxName=%s\n",
            vcTran->index, vcTran->name, vcTran->virtualSid, vcTran->workCount, transport->index, transport->name);
    if(vcTran->workCount > 0)
        return 1;
    ism_transport_submitAsyncJobRequest(transport, vcCloseJob, vcTran, 0);
    return 0;
}

/*
 * Virtual stream closed
 * TODO: real implementation
 */
static int vcClosed(ism_transport_t * transport) {
    ism_transport_t * mxTran =  ism_transport_getPhysicalTransport(transport);
    assert(transport->virtualSid != 0);
    TRACE(6, "vcClosed: vcIndex=%u vcName=%s sid=%u mxIndex=%u mxName=%s\n",
            transport->index, transport->name, transport->virtualSid, mxTran->index, mxTran->name);
    mxTran->addwork(mxTran, vcCloseWork, (void *)transport);
    return 0;
}



/*
 * Create an incoming virtual connection object
 */
static ism_transport_t * createVirtualConnection(ism_transport_t * transport, uint16_t stream) {
    mux_pobj_t * pobj = transport->pobj;
    ism_transport_t * vtransport;
    int  rc;

    vtransport = ism_transport_newTransport(transport->listener, 0, 1);
    if(vtransport) {
        vtransport->tobj = (struct ism_transobj *) transport;
        vtransport->virtualSid = stream;
        vtransport->close = vcClose;
        vtransport->closed = vcClosed;
        vtransport->send = muxSend;
        vtransport->index = ism_transport_nextConnectID();
        vtransport->frame = ism_transport_getHandshake();
        vtransport->client_addr = transport->client_addr;
        vtransport->server_addr = transport->server_addr;
        vtransport->clientport = transport->clientport;
        vtransport->serverport = transport->serverport;
        vtransport->userid = transport->userid;
        vtransport->addwork = transport->addwork;
        vtransport->tid = transport->tid;
        vtransport->state = ISM_TRANST_Opening;
        rc = ism_security_create_context(ismSEC_POLICY_CONNECTION, vtransport, &vtransport->security_context);
        if (rc) {
            TRACE(2, "Could not set security context for stream: index=%u rc=%d\n", vtransport->index, rc);
            /* Fail the creation of the transport if creation of sec context failed. */
            ism_transport_freeTransport(vtransport);
            return NULL;
        }
        pobj->streams[stream] = vtransport;
        pobj->streamCount++;
        ism_transport_addMonitorNow(vtransport);
    }
    return vtransport;
}

static int closeRequestJob(ism_transport_t * transport, void * param1, uint64_t param2) {
    int i = 0;
    transport->pobj->closed = 2;
    if(transport->pobj->streamCount == 0) {
        if(transport->pobj->streams) {
            ism_common_free(ism_memory_protocol_misc,transport->pobj->streams);
            transport->pobj->streams = NULL;
            transport->closed(transport);
        }
    } else {
        for(i = 0; i <= MAX_VC_ID; i++) {
            ism_transport_t * vcTran = transport->pobj->streams[i];
            if (vcTran) {
                vcTran->close(vcTran, transport->pobj->rc ? transport->pobj->rc : ISMRC_ClosedByServer, 0,
                    transport->pobj->reason ? transport->pobj->reason : "Physical connection closed");
            }
        }
    }
    return 0;
}



/*
 * Receive normal packet
 */
static int muxReceive(ism_transport_t * transport, char * data, int datalen, int kind) {
    ism_muxHdr_t hdr = {0};
    int cmd;
    uint16_t stream;
    mux_pobj_t * pobj = transport->pobj;
    ism_transport_t * vcTran = NULL;
    int rc = ISMRC_OK;
    hdr.iValue = kind;
    cmd = hdr.hdr.cmd;
    stream = hdr.hdr.stream;
    if (pobj->closed) {
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }
    /* Trace */
    if (SHOULD_TRACEC(9, Protocol)) {
        char xbuf [128];
        int maxsize = ism_common_getTraceMsgData();
        sprintf(xbuf, "mux recv %d %s connect=%u stream=%u",
                cmd, transport->actionname(cmd), transport->index, stream);
        traceDataFunction(xbuf, 0, __FILE__, __LINE__, data, datalen, maxsize);
    }

    switch (cmd) {
    case MuxData:
        vcTran = transport->pobj->streams[stream];
        if(vcTran) {
            int used = 0;
            while (!rc && used < datalen) {
                rc = vcTran->frame(vcTran, data, used, datalen, &used);
            }
            vcTran->read_bytes += datalen;
            if (rc) {
                if(vcTran->state != ISM_TRANST_Closing) {
                    /* Error */
                    TRACE(3, "Frame error: index=%u cmd=%s stream=%u rc=%d\n", vcTran->index, "MuxData", stream, rc);
                } else {
                    TRACE(7, "Virtual connection closed from callback: index=%u cmd=%s stream=%u rc=%d\n", vcTran->index, "MuxData", stream, rc);
                }
                rc = ISMRC_OK; /* Do not close physical connection */
            }
        } else {
            /* Error */
            TRACE(6, "Stream not found: index=%u cmd=%s name=%s stream=%u\n", transport->index, "MuxData", transport->name, stream);
        }
        break;
    case MuxCreateStream:
        assert(transport->pobj->streams[stream] == NULL);
        if(transport->pobj->streams[stream] == NULL) {
            vcTran = createVirtualConnection(transport, stream);
            if(vcTran == NULL) {
                sendCloseStream(transport, stream, ISMRC_AllocateError);
            } else {
                TRACE(5, "Virtual connection created: mxTran=%p mxIndex=%u vcTran=%p vcIndex=%u stream=%u\n", transport, transport->index, vcTran, vcTran->index, stream);
            }
        } else {
            TRACE(3, "Stream already exists: index=%u cmd=%s name=%s stream=%u\n", transport->index, "MuxCreateStream", transport->name, stream);
        }
        break;
    case MuxCloseStream:
        vcTran = transport->pobj->streams[stream];
        if(vcTran) {
            transport->pobj->streams[stream] = NULL;
            vcTran->closestate[3] = 1;
            vcTran->close(vcTran, ISMRC_ClosedByClient, 1, "Connection closed by client");
        } else {
            /* This may happened - if stream was closed on server side already */
            TRACE(6, "Stream not found: index=%u cmd=%s name=%s stream=%u\n", transport->index, "MuxCloseStream", transport->name, stream);
        }
        break;
    case MuxCreatePhysical:
        {
            uint16_t length;
            char * info;
            transport->pobj->clientVersion = *data;
            data++;
            memcpy(&length, data, sizeof(length));
            data += sizeof(length);
            length = ntohs(length);
            info = ism_transport_allocBytes(transport, length + 1, 0);
            transport->name = info;
            transport->clientID = info;
            memcpy(info, data, length);
            data += length;
            info[length] = '\0';
            memcpy(&length, data, sizeof(length));
            data += sizeof(length);
            length = ntohs(length);
            info = alloca(length+1);
            memcpy(info, data, length);
            info[length] = '\0';
            TRACE(5, "New multiplex connection created: transport=%p index=%u name=%s info=%s\n", transport, transport->index, transport->name, info);
            transport->ready = 1;
            sendAck(transport);
        }
        break;
    case MuxClosePhysical:
        if (__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
            closeRequestJob(transport, NULL, 0);
        }
        break;
    case MuxCreatePhysicalAck:
    default:
        TRACE(3,"Unexpected command for multiplex protocol: %d %s connect=%u\n",cmd, transport->actionname(cmd), transport->index);
        transport->close(transport, ISMRC_BadClientData, 0, "Unexpected command");
        rc = ISMRC_BadClientData;
        break;
    }
    return rc;
}

/*
 * Multiplex closing
 *
 * When we close the physical connection, we need to close all virtual connections
 */
static int muxClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    mux_pobj_t * pobj = transport->pobj;
    TRACE(5, "muxClosing: transport=%p index=%u name=%s streamCount=%u closed=%d\n", transport, transport->index,
            transport->name, pobj->streamCount, pobj->closed);
    if (!__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
        return 0;
    }
    pobj->rc = rc;
    if (reason)
        pobj->reason = ism_transport_putString(transport, reason);
    ism_transport_submitAsyncJobRequest(transport, closeRequestJob, NULL, 0);
    return 0;
}

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

XAPI int ism_transport_setMaxSocketBufSize(ism_transport_t * transport, int maxSendSize, int maxRecvSize);
/*
 * Initialize mux connection
 */
static int muxConnection(ism_transport_t * transport) {
    // printf("protocol=%s\n", transport->protocol);
    if (!strcmp(transport->protocol, "mux")) {
        mux_pobj_t * pobj = (mux_pobj_t *) ism_transport_allocBytes(transport, sizeof(mux_pobj_t), 1);
        memset((char *) pobj, 0, sizeof(mux_pobj_t));
        pthread_spin_init(&pobj->lock, 0);
        transport->pobj = pobj;
        transport->receive = muxReceive;
        transport->closing = muxClosing;
        transport->actionname = muxCommand;
        transport->protocol = "mux";
        transport->protocol_family = "mqtt";    /* Authorized with mqtt */
        pobj->streams = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,61),MAX_VC_ID+1, sizeof(ism_transport_t *));
        ism_transport_setMaxSocketBufSize(transport, 0, 0); /* Set socket buffer size to be unlimited */
        return 0;
    }
    return 1;
}

/*
 * Initialize mux protocol
 */
int ism_protocol_initMux(void) {
    ism_transport_registerProtocol(NULL, muxConnection);
    sprintf(serverInfo,"%s %s %s", ism_common_getVersion(), ism_common_getBuildLabel(), ism_common_getBuildTime());
    serverInfoLength = strlen(serverInfo);

    /* For now just use mqtt capabilities */
    // int capability =  ISM_PROTO_CAPABILITY_USETOPIC;
    // ism_admin_updateProtocolCapabilities("mux", capability);
    return 0;
}
