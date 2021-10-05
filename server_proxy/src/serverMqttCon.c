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

#define TRACE_COMP Http

#include <ismutil.h>
#include <pxtransport.h>
#define MQTT_CONST_ONLY
#include <pxmqtt.h>
#include <tenant.h>
#include <protoex.h>
#define PostEvent          1
#define PostCommand        2

typedef struct mqttPublishRequest {
    struct mqttPublishRequest * next;
    ism_http_t *                httpReq;
    uint16_t                    msgID;
    uint16_t                    rsrv[3];
} mqttPublishRequest;

typedef struct ism_protobj_t {
    ism_server_t        *   server;
    ismHashMap          *   activeRequests;
    mqttPublishRequest  *   freeRequestsHead;
    pthread_spinlock_t      lock;
    uint16_t                nextMsgId;
    uint16_t                activeRequestsCount;
    int8_t                  index;
} ism_protobj_t;

static void handleServerClose(ism_server_t* server, ism_time_t delay) ;
extern int g_msgRoutingDefaultSendMQTT;
extern int ism_route_routeMessage(ism_transport_t * transport, const char * buf, int buf_len, int kind, int *sendMQTT);

typedef struct ism_protobj_t mqttCon_pobj_t;
extern void ism_rest_handlePubAck(ism_http_t * httpReq, int rc, const char * reason);
static uint16_t addActiveRequest(mqttCon_pobj_t * pobj, ism_http_t * httpReq) {
    pthread_spin_lock(&pobj->lock);
    if(pobj->freeRequestsHead) {
        uint32_t id;
        mqttPublishRequest * request = pobj->freeRequestsHead;
        pobj->freeRequestsHead = request->next;
        request->next = NULL;
        request->httpReq = httpReq;
        id = request->msgID;
        ism_common_putHashMapElement(pobj->activeRequests, &id, sizeof(id), request, NULL);
        pobj->activeRequestsCount++;
        pthread_spin_unlock(&pobj->lock);
        return request->msgID;
    }
    if(pobj->nextMsgId < 0xffff) {
        uint32_t id = pobj->nextMsgId++;
        mqttPublishRequest * request = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_mqtt_request,20),sizeof(mqttPublishRequest));
        request->httpReq = httpReq;
        request->msgID = (uint16_t)id;
        request->next = NULL;
        ism_common_putHashMapElement(pobj->activeRequests, &id, sizeof(id), request, NULL);
        pobj->activeRequestsCount++;
        pthread_spin_unlock(&pobj->lock);
        return request->msgID;
    }
    pthread_spin_unlock(&pobj->lock);
    return 0;
}

static ism_http_t * removeActiveRequest(mqttCon_pobj_t * pobj, uint16_t msgId) {
    uint32_t id = msgId;
    ism_http_t * httpReq = NULL;
    pthread_spin_lock(&pobj->lock);
    mqttPublishRequest * request = ism_common_removeHashMapElement(pobj->activeRequests, &id, sizeof(id));
    if(request) {
        httpReq = request->httpReq;
        request->next = pobj->freeRequestsHead;
        pobj->freeRequestsHead = request;
        request->httpReq = NULL;
        pobj->activeRequestsCount--;
    }
    pthread_spin_unlock(&pobj->lock);
    return httpReq;
}

static void freeActiveRequest(void *ptr) {
    mqttPublishRequest * request = (mqttPublishRequest *) ptr;
    ism_rest_handlePubAck(request->httpReq, ISMRC_NotConnected, "Connection to server closed");
    ism_common_free(ism_memory_proxy_mqtt_request,request);
}

extern void ism_tcp_init_transport(ism_transport_t * transport);
/*
 * Put one character to a concat buf
 */
static void bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}

typedef mqttCon_pobj_t ism_protobj_t;
static int startServerConnectionTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata);
static void handleServerClose(ism_server_t* server, ism_time_t delay);

static void completeConnectionClose(ism_transport_t * transport) {
    ism_server_t * server = transport->server;
    mqttCon_pobj_t * pobj = (mqttCon_pobj_t*)transport->pobj;
    serverConnection_t * pSC = &server->mqttCon;
    TRACE(4, "completeConnectionClose: complete connection closing\n");
    pthread_spin_lock(&pSC->lock);
    pSC->state = TCP_DISCONNECTED;
    pSC->transport = NULL;
    pobj->activeRequestsCount = 0;
    pthread_spin_unlock(&pSC->lock);
    pthread_spin_lock(&pobj->lock);
    if(pobj->activeRequests) {
        ism_common_destroyHashMapAndFreeValues(pobj->activeRequests, freeActiveRequest);
        pobj->activeRequests = NULL;
    }
    while(pobj->freeRequestsHead) {
        mqttPublishRequest * request = pobj->freeRequestsHead;
        pobj->freeRequestsHead = request->next;
        ism_common_free(ism_memory_proxy_mqtt_request,request);
    }
    pthread_spin_unlock(&pobj->lock);
    handleServerClose(transport->server, 10000000000);
    transport->closed(transport);
}

static int mqttConnectionClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    ism_server_t * server = transport->server;
    serverConnection_t * pSC = &server->mqttCon;
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
        handleServerClose(transport->server, 10000000000);
        return 0;
    }
    pSC->useCount--;
    pSC->state = TCP_DISCONNECTING;
    if (pSC->useCount) {
        pthread_spin_unlock(&pSC->lock);
        return 0;
    }
    pthread_spin_unlock(&pSC->lock);
    completeConnectionClose(transport);
    return 0;
}


static inline ima_pxtransport_t * getServerConnection(ism_server_t * server, int index) {
    ima_pxtransport_t * transport = NULL;
    if (server) {
        pthread_spin_lock(&server->mqttCon.lock);
        if(LIKELY(server->mqttCon.transport && (server->mqttCon.state == PROTOCOL_CONNECTED))) {
            transport = server->mqttCon.transport;
            server->mqttCon.useCount++;
        }
        pthread_spin_unlock(&server->mqttCon.lock);
    }
    return transport;
}

static inline void freeServerConnection(ima_pxtransport_t * transport) {
    ism_server_t * server = transport->server;
    int shouldClose = 0;
    pthread_spin_lock(&server->mqttCon.lock);
    server->mqttCon.useCount--;
    if(server->mqttCon.useCount == 0)
        shouldClose = 1;
    pthread_spin_unlock(&server->mqttCon.lock);
    if(shouldClose)
        completeConnectionClose(transport);
}

/*
 * Notification of the outgoing connection complete.
 * If there is a saved packet send it at this point.
 */
int ism_server_connectionComplete(ism_transport_t * transport, int rc) {
    ism_server_t * server = transport->server;
    serverConnection_t * pSC = &server->mqttCon;
    char xbuf[1024];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    TRACE(6, "Outgoing srv connection complete: connect=%d ip=%s port=%u rc=%d\n",
            transport->index, transport->server_addr, transport->serverport, rc);
    if (rc == 0) {
        int  extloc;
        transport->ready = 5;
        transport->state = ISM_TRANST_Open;
        memcpy(xbuf+16, "\0\6MQTTpx\4\0\0\0\0", 13);  /* MQTTpx v3.1.1 keepalive=0 */
        xbuf[16+9] = CFLAG_Clean;
        xbuf[16+10] = XCFLAG_NoUserCheck | XCFLAG_ExtraFlags | XCFLAG_NoLog;
        buf.used = 29;

        /* Put out the client ID */
        bputchar(&buf, 0);
        bputchar(&buf, 0);

        /* Put out the extension */
        extloc = buf.used;
        buf.used += 2;
        ism_common_putExtensionValue(&buf, EXIV_SendNAK, 1);
        ism_common_putExtensionValue(&buf, EXIV_SendQoS0NAK, 1);
        ism_common_putExtensionValue(&buf, EXIV_SubscribeNAK, 1);
        ism_common_putExtensionValue(&buf, EXIV_ServerInfo, 1);
        xbuf[extloc] = 0;
        xbuf[extloc+1] = 4;
        pthread_spin_lock(&pSC->lock);
        pSC->state = PROTOCOL_CON_IN_PROCESS;
        pthread_spin_unlock(&pSC->lock);
        /* Send the connect */
        transport->send(transport, buf.buf+16, buf.used-16, 0x10, SFLAG_FRAMESPACE);
    } else {
        completeConnectionClose(transport);
    }
    return 0;
}
extern int  ism_proxy_createMQTTConnection(ism_transport_t * transport, const char * servername);
extern const char * ism_mqtt_mqttCommand(int ix);

/*
 * Receive MQTT binary.
 *
 * This is an instance of ism_transport_receive_t invoked using transport->receive().
 *
 * This method is used when doing native binary MQTT.  The MQTT command byte is sent as
 * the kind parameter.
 *
 * In any case where we return a non-zero return code, the connection should have been
 * disconnected.
 *
 */
int ism_server_mqttRecv(ism_transport_t * transport, char * inbuf, int buflen, int kind) {
    int rc = ISMRC_OK;

    uint8_t * bp = (uint8_t *) inbuf;
    ism_server_t * server = transport->server;
    serverConnection_t * pSC = &server->mqttCon;

    uint8_t command = (uint8_t)((kind >> 4) & 15);

    /* Do the receive trace */
    if (SHOULD_TRACE(9)) {
        char obuf[64];
        sprintf(obuf, "MQTT receive %02x %s connect=%u", kind & 0xff, ism_mqtt_mqttCommand(command), transport->index);
        traceDataFunction(obuf, 0, __FILE__, __LINE__, inbuf, buflen, ism_common_getTraceMsgData());
    }

    if (!rc) {
        /*
         * Process the commands
         */
        switch (command) {
        /*
         * Publish
         */
        case MT_PUBLISH:
            /* TODO: add GET support */
            break;
        /*
         * Publish acknowledge for PUBLISHX
         */
        case MT_PUBACK: /* ACK publish */
            {
                const char * errMsg = NULL;
                uint16_t msgid = (uint16_t)BIGINT16(bp);
                ism_http_t * httpReq = removeActiveRequest(transport->pobj, msgid);
                if(httpReq) {
                    bp += 2;
                    buflen -= 2;
                    if(buflen > 2) {
                        uint16_t extLen =  (uint16_t)BIGINT16(bp);
                        bp += 2;
                        buflen -= 2;
                        if (extLen == buflen) {
                            errMsg = ism_common_getExtensionString((char*)bp, extLen, EXIV_ReasonString, NULL);
                            rc = ism_common_getExtensionValue((char*)bp, extLen, EXIV_ServerRC, 0);
                            bp += extLen;
                            buflen -= extLen;
                        } else {
                            //TODO: Bug in server
                        }
                    }
                    ism_rest_handlePubAck(httpReq, rc, errMsg);
                } else {
                    //TODO: This should never happen
                }
            }
            break;

        /*
         * Subscribe ACK
         */
        case MT_SUBACK:
            /* TODO: Add GET support */
            break;
        /*
         * Ping response.  We should not get this as a normal server
         */
        case MT_PINGRESP:
            break;
        /*
         * Connect ACK.  There are extra return codes in proxy protocol
         */
        case MT_CONNACK:
            {
                int mqttpx_version = 0;
                if (buflen > 2) {
                    char * ext = (char *)bp+2;
                    int    extlen = buflen-2;
                    mqttpx_version = ism_common_getExtensionValue(ext, extlen, EXIV_ProxyProtLevel, 2);
                    TRACE(5, "Set MQTTpx version: connect=%u name=%s version=%u\n",
                            transport->index, transport->name, mqttpx_version);
                    rc = ism_common_getExtensionValue(ext, extlen, EXIV_ServerRC, rc);
                    if(rc) {
                        const char * reason = ism_common_getExtensionString(ext, extlen, EXIV_ReasonString, NULL);
                        if (reason) {
                            TRACE(5, "Unable to connect to server: connect=%u client=%s, rc=%u reason=%s\n",
                                    transport->index, transport->name, rc, reason);
                        }
                    }
                }
                int xrc = bp[1];
                if (xrc > 6) {
                    if (rc == 0)
                        rc = ISMRC_ClosedByServer;    /* TODO: Get actual RC from ext */
                    ism_common_setError(rc);
                } else {
                    if (xrc) {
                        rc = xrc;
                        ism_common_setError(rc);      /* TODO: fix when we remove rc mapping */
                    }
                }
                if (mqttpx_version < 3) {
                    rc = ISMRC_ProtocolVersion;
                    ism_common_setError(rc);
                    pthread_spin_lock(&pSC->lock);
                    pSC->disabled = 1;
                    pthread_spin_unlock(&pSC->lock);
                }
                if (rc) {
                    transport->close(transport, rc, 0, "MQTT Connect failed");
                } else {
                    pthread_spin_lock(&pSC->lock);
                    if((pSC->transport == transport) && (pSC->state == PROTOCOL_CON_IN_PROCESS))
                        pSC->state = PROTOCOL_CONNECTED;
                    pthread_spin_unlock(&pSC->lock);

                }
            }
            break;

        /*
         * Disconnect.
         * In MQTT this is only client to server, but we allow it server to proxy in the proxy
         * protocol to send an error code to the proxy.
         */
        case MT_DISCONNECT:
            break;

        /*
         * All other packets are an error
         */
        default:
            rc = ISMRC_InvalidCommand; /* Invalid command */
            ism_common_setError(rc);
            break;
        }
    }

    return rc;
}

/*
 * Construct the outgoing connection
 */
int ism_iotrest_serverConnect(ism_server_t* server) {
    ism_transport_t * transport;
    mqttCon_pobj_t * pobj;
    /*
     * Create outgoing connection
     */
    transport = ism_transport_newOutgoing(NULL, 1);
    ism_tcp_init_transport(transport);
    transport->originated = 2;
    transport->protocol = "mqtt4-iotrest";  //TODO: Do we need to modify this to support MQTTv5
    transport->protocol_family = "admin";
    pobj = (mqttCon_pobj_t *) ism_transport_allocBytes(transport, sizeof(mqttCon_pobj_t), 1);
    memset(pobj, 0, sizeof(mqttCon_pobj_t));
    pthread_spin_init(&pobj->lock, 0);
    pobj->nextMsgId = 1;
    pobj->activeRequests = ism_common_createHashMap(64*1024, HASH_INT32);
    pobj->server = server;
    transport->pobj = pobj;
    transport->receive = ism_server_mqttRecv;
    transport->actionname = ism_mqtt_mqttCommand;
    transport->tid = 0;
    transport->connected = ism_server_connectionComplete;
    transport->closing = mqttConnectionClosing;
    pthread_spin_lock(&server->lock);
    transport->server = server;
    server->mqttCon.transport = transport;
    server->mqttCon.state = TCP_CON_IN_PROCESS;
    server->mqttCon.useCount = 1;
    pobj->activeRequestsCount = 0;
    server->mqttCon.index = 0;//TODO: Update if/when there will be more connections per server
    pthread_spin_unlock(&server->lock);

    transport->name = ism_transport_putString(transport, server->name);
    transport->clientID = transport->name;

    TRACE(7, "Make outgoing srv connection to server=%s transport=%p\n", server->name, transport);
    if (ism_proxy_createMQTTConnection(transport, NULL))
        completeConnectionClose(transport);
    return ISMRC_OK;
}

static int startServerConnectionTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    ism_server_t* server = ((ism_server_t*) userdata);
    if(!server->mqttCon.disabled)
        ism_iotrest_serverConnect(server);
    ism_common_cancelTimer(timer);
    return 0;
}

static void handleServerClose(ism_server_t* server, ism_time_t delay) {
    ism_common_setTimerOnce(ISM_TIMER_HIGH, startServerConnectionTimer, (void*) server, delay);
}

int ism_server_startServerConnection(ism_server_t * server) {
    ism_common_setTimerOnce(ISM_TIMER_HIGH, startServerConnectionTimer, (void*) server, 1000000000);
    return 0;
}

uint16_t ism_server_pendingHTTPRequestCount(ism_server_t * server) {
    uint16_t count = 0;
    ism_transport_t * mqttTransport = getServerConnection(server,0);
    if(mqttTransport) {
        count = mqttTransport->pobj->activeRequestsCount;
        freeServerConnection(mqttTransport);
    }
    return count;
}

#define HAS_EXTENSION 0x20
#define HAS_MSG_ID    0x10
#define IS_DUP        0x08
#define QOS_0         0
#define QOS_1         0x02
#define QOS_2         0x04
#define RETAINED      0x01

extern int ism_http_splitPath(char * path, char * * parts, int partmax);
extern int ism_http_createTopic(concat_alloc_t *buf, const char *org, const char *type, const char *id, uint8_t action, const char *actionName, const char *fmt);

int ism_server_mqttPublish(ism_http_t * http) {
    ism_transport_t * clientTransport = http->transport;
    ism_server_t * server = clientTransport->server;
    ism_transport_t * mqttTransport = getServerConnection(server,0);
    if(mqttTransport) {
        uint16_t msgID = addActiveRequest(mqttTransport->pobj, http);
        if(msgID) {
            char * parts[16];
            char * path = alloca(strlen(http->path)+1);
            strcpy(path, http->path);
            char xbuf[8192];
            int extPos;
            uint8_t kind = HAS_MSG_ID | QOS_0;
            int hasExtension = ((http->locale) && (http->locale[0])) ;
            concat_alloc_t buf = {xbuf, sizeof xbuf};
            ism_http_splitPath(path, parts, 16);
            buf.used = 16;
            if(hasExtension) {
                uint16_t extLen;
                kind |= HAS_EXTENSION;
                bputchar(&buf,kind);
                extPos = buf.used;
                buf.used += 2;
                ism_common_putExtensionString(&buf, EXIV_Locale, http->locale);
                extLen = buf.used - extPos - 2;
                buf.buf[extPos] = (char) (extLen >> 8);
                buf.buf[extPos + 1] = (char) (extLen);
            } else {
                bputchar(&buf,kind);
            }
            ism_http_createTopic(&buf,clientTransport->org, parts[4],parts[6], http->subprot, parts[8], http->content->format);
            bputchar(&buf,((char) (msgID >> 8)));
            bputchar(&buf,((char) msgID));
            ism_common_allocBufferCopyLen(&buf, http->content->content, http->content->content_len);
            int sendMQTT=g_msgRoutingDefaultSendMQTT;
			ism_route_routeMessage(clientTransport, buf.buf+16, buf.used-16, MT_PUBLISHX,  &sendMQTT);
			if(sendMQTT){
				mqttTransport->send(mqttTransport, buf.buf+16, buf.used-16, MT_PUBLISHX, SFLAG_FRAMESPACE);
				TRACE(9, "ism_server_mqttPublish: connect=%u org=%s path=%s server=%s\n", clientTransport->index, clientTransport->org, http->path, server->name);
			}
            freeServerConnection(mqttTransport);
            ism_common_freeAllocBuffer(&buf);
            return ISMRC_OK;
        }
        freeServerConnection(mqttTransport);
        TRACE(6, "ism_server_mqttPublish: failed to send message for org=%s path=%s to server=%s. No message id available\n", clientTransport->org, http->path, server->name);
        return ISMRC_ServerCapacity;
    }
    TRACE(6, "ism_server_mqttPublish: failed to send message for org=%s path=%s to server=%s. Server not connected\n", clientTransport->org, http->path, server->name);
    return ISMRC_NotConnected;
}

/**
 * Send ACL to server
 */
int ism_server_sendACL(ism_http_t * http, char *bufACL, int len) {
    ism_transport_t * clientTransport = http->transport;
    ism_server_t * server = clientTransport->server;
    ism_transport_t * mqttTransport = getServerConnection(server,0);
    if (mqttTransport) {
    	mqttTransport->send(mqttTransport, bufACL, len, MT_SENDACL, SFLAG_FRAMESPACE);
    	freeServerConnection(mqttTransport);
		return ISMRC_OK;
    } else {
        TRACE(6, "ism_server_sendACL: failed to send ACL for org=%s path=%s to server=%s. Server not connected\n", clientTransport->org, http->path, server->name);
        return ISMRC_NotConnected;
    }
}
