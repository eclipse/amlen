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

/* @file pxhttp.c
 */
#define TRACE_COMP Http
#include <pxtransport.h>
#include <protoex.h>
#define MQTT_CONST_ONLY
#include <pxmqtt.h>
#include <selector.h>
#include <byteswap.h>
#include <iotrest.h>
#include <auth.h>
#include <imacontent.h>
#define endian_int16(x)  bswap_16(x)
#define endian_int32(x)  bswap_32(x)
#define endian_int64(x)  bswap_64(x)

#define GETX_ACK     0x01
#define GETX_SUBID   0x02
#define GETX_TOPIC   0x04
#define GETX_TIMEOUT 0x08
#define GETX_SUBOPT  0x10
#define GETX_FILTER  0x20

/*
 * The MQTT protocol specific area of the transport object
 */
typedef struct ism_protobj_t {
    char               eyecatcher[8];
    ism_transport_t *  transport;
    ism_transport_t *  client_transport;
    int32_t            inprogress;        /* Count of actions in progress */
    uint32_t           savedSize;
    void *             savedData;
    ism_http_t *       http;
    iot_replyMessage_t replyMsg;
    const char *       topic;
    uint32_t           timeout;
    uint32_t           ackID;
    uint32_t           subID;
    uint32_t           subopt;
    const char *       filter;

    pthread_spinlock_t lock;
    pthread_spinlock_t sessionlock;
    uint8_t            active;
    volatile uint8_t   closed;
    volatile uint8_t   startState;        /* Start state 0=not yet, 1=in progress, 2=stolen, 3=done */
    uint8_t            connectPending;
    uint8_t            clientClosing;
    uint8_t            resv[3];
} ism_protobj_t;

typedef struct ism_protobj_t houtPobj_t;

int g_httpOutSessionExpire = 30*3600;
static int g_useMux     = 0;

const char * ism_mqtt_mqttCommand(int ixin);
void ism_hout_doneConnection(ism_transport_t * transport);
static int parsePublishX(ism_transport_t * transport, uint8_t * bp, int buflen);
int ism_transport_frameMqtt(ism_transport_t * transport, char * buffer, int pos, int avail, int * used);
int ism_transport_addMqttFrame(ism_transport_t * transport, char * buf, int len, int command);
int ism_transport_connectStream(ism_transport_t * transport, ism_transport_t * ctransport, ism_tenant_t * tenant);
ism_transport_t * ism_mux_createVirtualConnection(ism_server_t * server, int tid, int * pRC, char * errMsg);
static int sendGETX(ism_http_t * http, uint32_t subID, const char * topic, uint32_t timeout, int subopt, const char * filter);
int ism_hout_init(void);
int ism_http_splitPath(char * path, char * * parts, int partmax);

extern int ism_iotrest_getInprogress(ism_transport_t *transport);

extern px_http_stats_t httpStats;

/*
 * TCP connection complete
 * Send the CONNECT
 */
int ism_hout_connected(ism_transport_t * transport, int rc) {
    if (__sync_bool_compare_and_swap(&transport->pobj->connectPending, 1, 0)) {
        ism_transport_t * ctransport;
        TRACE(8, "Outgoing connection complete: connect=%d ip=%s port=%u rc=%d savedSize=%d inprogress=%d\n",
                transport->index, transport->server_addr, transport->serverport, rc, transport->pobj->savedSize,
                transport->pobj->inprogress);
        if (rc == 0) {
            houtPobj_t * pobj = transport->pobj;
            char * senddata = pobj->savedData;
            int    sendlen = pobj->savedSize;
            transport->state = ISM_TRANST_Open;
            if (sendlen) {
                transport->send(transport, senddata, sendlen, 0, SFLAG_HASFRAME);
                pobj->active = 1;
            }
            transport->ready = 5;   /* TCP and TLS connection, but no MQTT connection */
        }
        /* If close is in progress, proceed with it */
        ctransport = transport->pobj->client_transport;
        if (ctransport && ctransport->pobj) {
        	if (ism_iotrest_getInprogress(ctransport) <= 0) {
                TRACE(6, "Complete close: connect=%u client=%s client connect=%u inprogress=%d\n",
                		transport->index, transport->name, ctransport->index, ctransport->pobj->inprogress);
                ism_hout_doneConnection(transport);
                return 0;
            }
            if (rc) {
                ctransport->close(ctransport, rc, 0, "Could not connect to MessageSight server");
            }
        }
        return 0;
    }
    return 1;
}


/*
 * Receive a connection closing notification for the MQTT protocol.
 * We start the closing and it is completed in replyClosed().
 */
int ism_hout_closing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    houtPobj_t * pobj = (houtPobj_t *) transport->pobj;
    int32_t count;

    TRACE(8, "ism_hout_closing: connect=%u client=%s rc=%d clean=%d reason=%s\n",
            transport->index, transport->name, rc, clean, reason);

    if (pobj == NULL) /* Connection was broken during initConnection phase */
        return 0;

    /*
     * Set the indicator that close is in progress. If set failed,
     * then this has been done before and we don't need to proceed.
     */
    if (!__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
        return 0;
    }
    transport->close_rc = rc;
    if (rc && reason && *reason)
        transport->reason = ism_transport_putString(transport, reason);

    if (transport->pobj->active) {
        transport->pobj->active = 0;
        transport->pobj->replyMsg(transport->pobj->http, rc, reason, 1, 0, NULL, NULL, 0);
    }

    /*
     * Subtract the "in progress" indicator. If it becomes negative,
     * no actions are in progress, so it is safe to clean up protocol data
     * and close the connection. If it is non-negative, there are
     * actions in progress. The action that sets this value to 0
     * would re-invoke closing().
     */
    count = __sync_sub_and_fetch(&pobj->inprogress, 1);
    if (count >= 0) {
        TRACE(8, "ism_hout_closing postponed: connect=%u client=%s inprogress=%d\n",
                transport->index, transport->name, count+1);
        return 0;
    }

    /* Stop message delivery, destroy session and client state */
    ism_hout_doneConnection(transport);
    return 0;
}


/*
 * Close our connection, and close the client connection
 */
void ism_hout_doneConnection(ism_transport_t * transport) {
    houtPobj_t * pobj = (houtPobj_t *) transport->pobj;
    char xbuf[256];
    const char * reason;
    int  rc = transport->close_rc;
    /*
     * Set the indicator that close is complete. If set failed,
     * then this has been done before and we don't need to proceed.
     */
    if (!__sync_bool_compare_and_swap(&pobj->closed, 1, 2)) {
        return;
    }

    reason = transport->reason;
    if (reason == NULL) {
        reason = "The connection has completed normally";
    } else {
        ism_common_getErrorString(rc, xbuf, sizeof xbuf);
        reason = xbuf;
    }

    if (transport->pobj->client_transport) {
        if (ism_hout_connected(transport, rc)) {
        	if (!__sync_bool_compare_and_swap(&pobj->clientClosing, 1, 2)) {
        		transport->pobj->client_transport->close(transport->pobj->client_transport, rc, rc==0, reason);
        	}
        }
    }

    if (pobj->savedSize) {
        pobj->savedSize = 0;
        ism_common_free(ism_memory_proxy_http,pobj->savedData);
        pobj->savedData = NULL;
    }
    pthread_spin_destroy(&pobj->lock);

    /* Tell the transport we are done */
    transport->closed(transport);
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
 * Replace a string
 */
static void  replaceString(const char * * oldval, const char * val) {
    if (!my_strcmp(*oldval, val))
        return;
    if (*oldval) {
        char * freeval = (char *)*oldval;
        if (val && !strcmp(freeval, val))
            return;
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_utils,1000),val);
        else
            *oldval = NULL;
        ism_common_free(ism_memory_proxy_utils,freeval);
    } else {
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_utils,1000),val);
        else
            *oldval = NULL;
    }
}


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


/*
 * Send the ACLs to the server.
 * For now we just send it as a single packet.  We might later want to break this up.
 */
static int sendACLs(ism_transport_t * transport) {
    char xbuf [2048];
    ism_acl_t * acl;
    const char * aclKey;
    ism_transport_t * stransport = (ism_transport_t *)transport->hout;
    concat_alloc_t buf = {xbuf, sizeof xbuf, 19};

    if (!stransport)
        return 1;


    xbuf[16] = 0;
    xbuf[17] = 1;
    xbuf[18] = EXIV_EndExtension;
    aclKey = ism_proxy_getACLKey(transport);
    acl = ism_protocol_findACL(aclKey, 0, NULL);
    if (acl) {
        ism_protocol_getACL(&buf, acl);
        ism_protocol_unlockACL(acl);
        transport->has_acl = 1;
    } else {
        bputchar(&buf, '!');
        ism_json_putBytes(&buf, aclKey);
        transport->has_acl = 0;
    }
    stransport->send(stransport, buf.buf+16, buf.used-16, MT_SENDACL, SFLAG_FRAMESPACE);
    return 0;
}

/*
 * Receive MQTT binary for the HOUT connection.
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
int ism_hout_receive(ism_transport_t * transport, char * inbuf, int buflen, int kind) {
    uint8_t * bp = (uint8_t *) inbuf;
    int rc = ISMRC_OK;
    char * reason = NULL;
    char xbuf[4096];

    int  decrement = 1;   /* Decrement inprogress */
    concat_alloc_t buf = {xbuf, sizeof xbuf, 16};
    houtPobj_t * pobj = transport->pobj;
    uint8_t command = (uint8_t)((kind >> 4) & 15);

    /* If we are in the process of closing, return closed */
    if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
        __sync_fetch_and_sub(&pobj->inprogress, 1);
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }

    /* Do the receive trace */
    if (SHOULD_TRACE(9)) {
        char obuf[64];
        sprintf(obuf, "HOUT receive %02x %s connect=%u inprogress=%d", kind & 0xff, ism_mqtt_mqttCommand(command), transport->index, pobj->inprogress);
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
            if ((kind&0x0f) == 0x0f) {
                rc = parsePublishX(transport, bp, buflen);
            } else {
                rc = ISMRC_InvalidCommand;
                ism_common_setError(rc);
            }
            decrement = 1;
            break;

        /*
         * Connect ACK.  There are extra return codes in proxy protocol
         */
        case MT_CONNACK:
            if (buflen > 4) {
                rc = ism_common_getExtensionValue((const char *)bp+4, buflen-4, EXIV_ServerRC, rc);
                reason = (char *)ism_common_getExtensionString((const char *)bp+4, buflen-4, EXIV_ReasonString, NULL);
            }
            if (rc) {
                if (transport->pobj->active) {
                    transport->pobj->active = 0;
                    transport->pobj->replyMsg(transport->pobj->http, rc, reason, 1, 0, NULL, NULL, 0);
                }
                decrement = 1;
            } else {
                ism_transport_t * ctransport = transport->pobj->client_transport;
                if (!g_useMux) {
                    /* Remember whether this connection is from the primary or backup */
                    ism_server_setLastGoodAddress(transport->server, transport->connect_order);
                }
                if (ctransport && ctransport->has_acl) {
                	const char * aclKey = ism_proxy_getACLKey(ctransport);
                    TRACE(6, "Send ACL to the server: %s\n", aclKey);
                    ism_acl_t * acl = ism_protocol_findACL(aclKey, 0, NULL);
                    if (acl) {
                        TRACE(8, "Send ACL to server: connect=%u name=%s was=%p\n",
                                transport->index, aclKey, acl->object);
                        acl->object = ctransport;
                        ism_protocol_unlockACL(acl);
                    }
                    sendACLs(ctransport);
                }
                sendGETX(transport->pobj->http, transport->pobj->subID, transport->pobj->topic, transport->pobj->timeout,
                        transport->pobj->subopt, transport->pobj->filter);
                decrement = 1;
            }
            break;

        /*
         * Disconnect.
         * In MQTT this is only client to server, but we allow it server to proxy in the proxy
         * protocol to send an error code to the proxy.
         */
        case MT_DISCONNECT:
            rc = ISMRC_ClosedByServer;
            if (buflen > 3) {
                rc = ism_common_getExtensionValue((const char *)bp+2, buflen-2, EXIV_ServerRC, rc);
                reason = (char *)ism_common_getExtensionString((const char *)bp+2, buflen-2, EXIV_ReasonString, NULL);
            }
            TRACE(6, "Receive DISCONNECT from server: connect=%u name=%s rc=%u reason=%s active=%d\n",
                        transport->index, transport->name, rc, reason, transport->pobj->active);

            decrement = 1;
            transport->close(transport, rc, rc==0, reason);
            rc = 0;
            break;

        /*
         * All other packets are an error
         */
        default:
            rc = ISMRC_InvalidCommand; /* Invalid command */
            ism_common_setError(rc);
            decrement = 1;
            break;
        }
    }

    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);

    /*
     * If close is in progress, proceed with it.
     * For connect we wait to do this until the connection is established
     */
    if (LIKELY(decrement)) {
        int inprogress = __sync_sub_and_fetch(&pobj->inprogress, 1);
        TRACE(9, "HOUT end receive: command=%s connect=%u inprogress=%d\n",
                ism_mqtt_mqttCommand(command), transport->index, inprogress);
        if (inprogress < 0) {
            TRACE(8, "Continue close of connection: connect=%d name=%s\n", transport->index, transport->name);
            ism_hout_doneConnection(transport);
        }
    }
    if (rc) {
        if (!reason) {
            if (rc != ism_common_getLastError())
                ism_common_setError(rc);
            ism_common_formatLastError(xbuf, sizeof xbuf);
        }
        transport->close(transport, rc, 0, xbuf);
    }
    return rc;
}


/*
 * Parse a PUBLISHX with a response to the GETX
 */
static int parsePublishX(ism_transport_t * transport, uint8_t * bp, int buflen) {
    int kind;
    xUNUSED int qos;
    xUNUSED int isMsgID;
    int extlen;
    int subID = 0;
    char * ext;
    char * payload;
    int    payload_len;
    int rc = 0;
    const char * reason = NULL;
    const char * topic = NULL;
    int  reasonlen;
    int  topiclen=0;
    int  close = 0;

    TRACE(8, "Parse PUBLISHX: connect=%u name=%s buflen=%d\n",
        		transport->index, transport->name, buflen);

    if (buflen > 3) {
        kind = *bp++;
        buflen--;
        qos = (uint8_t)((kind >> 1) & 3);
        isMsgID = (uint8_t)((kind>>4) & 1);
        /* Process extension */
        if (kind&0x20) {
            extlen = BIGINT16(bp);
            bp += 2;
            buflen -= 2;
            ext = (char *)bp;
            if (extlen < buflen) {
                rc = ism_common_getExtensionValue(ext, extlen, EXIV_ServerRC, 0);
                reason = ism_common_getExtensionString(ext, extlen,  EXIV_ReasonString, &reasonlen);
                subID = ism_common_getExtensionValue(ext, extlen, EXIV_SubID, 0);
                bp += extlen;
                buflen -= extlen;
            } else {
                rc = ISMRC_BadClientData;
                ism_common_setError(rc);
            }
        }
    } else {
        rc = ISMRC_BadClientData;
        ism_common_setError(rc);
    }

    TRACE(8, "Parse PUBLISHX: connect=%u name=%s subID=%d reason=%s rc=%d buflen=%d\n",
    		transport->index, transport->name, subID, reason, rc, buflen);

    __sync_add_and_fetch(&httpStats.httpS2PMsgsReceived, 1);

    /* parse topic */
    if (buflen >= 2) {
        topiclen = BIGINT16(bp);
        bp += 2;
        buflen -= 2;
        if (buflen >= topiclen) {
            topic = (char *)bp;
            bp += topiclen;
            buflen -= topiclen;
        }
    }
    if (rc == 0 && !topic) {
        rc = ISMRC_BadClientData;
        ism_common_setError(rc);
    }
    if (rc) {
        payload = "";
        payload_len = 0;
        if (rc >= ISMRC_Error)
            close = 1;
    } else {
        payload = (char *)bp;
        payload_len = buflen;
        close = 0;
    }
    char * tcopy = alloca(topiclen+1);
    if (topiclen)
        memcpy(tcopy, topic, topiclen);
    tcopy[topiclen] = 0;
    transport->pobj->active = 0;
    transport->pobj->replyMsg(transport->pobj->http, rc, reason, close,
             subID, tcopy, payload, payload_len);
    return 0;
}


/*
 * Send the GETX to the server
 */
int sendGETX(ism_http_t * http, uint32_t subID, const char * topic, uint32_t timeout, int subopt, const char * filter) {
    char xbuf[4096];
    concat_alloc_t zbuf = {xbuf, sizeof xbuf, 16};
    concat_alloc_t * buf = &zbuf;
    ism_transport_t * transport = (ism_transport_t *)http->transport->hout;

    TRACE(8, "Send GETX: connect=%u name=%s subID=%d topic=%s\n", transport->index, transport->name, subID, topic);

    if (!transport)
        return ISMRC_Error;

    /* Compute the options value */
    int opt = 0;
    if (topic && * topic)
        opt |= GETX_TOPIC;
    if (subID)
        opt |= GETX_SUBID;
    if (timeout)
        opt |= GETX_TIMEOUT;
    if (subopt)
        opt |= GETX_SUBOPT;
    if (filter && *filter)
        opt |= GETX_FILTER;
    bputchar(buf, opt);

    /* Put out the fields */
    if (opt & GETX_SUBID) {              /* SubID */
        bputchar(buf, (char)subID>>8);
        bputchar(buf, (char)subID);
    }
    if (opt & GETX_TOPIC) {              /* Topic */
        int topiclen = strlen(topic);
        bputchar(buf, (char)topiclen>>8);
        bputchar(buf, (char)topiclen);
        ism_common_allocBufferCopyLen(buf, topic, topiclen);
    }
    if (opt & GETX_TIMEOUT) {            /* Timeout */
        uint32_t ival = endian_int32(timeout);
        ism_common_allocBufferCopyLen(buf, (char *)&ival, 4);
    }
    if (opt & GETX_SUBOPT) {             /* Subscription options */
        bputchar(buf, (char)subopt>>8);
        bputchar(buf, (char)subopt);
    }
    if (opt & GETX_FILTER) {             /* Subscription filter */
        int filterlen = strlen(filter);
        bputchar(buf, (char)filterlen>>8);
        bputchar(buf, (char)filterlen);
        ism_common_allocBufferCopyLen(buf, filter, filterlen);
    }

    /* Send the frame */
    transport->send(transport, buf->buf+16, buf->used-16, 0x5F, SFLAG_FRAMESPACE);
    return 0;
}

/*
 * Set the saved data after first adding the MQTT frame.
 * This code assumes the buffer has size for the frame before the data location.
 */
void setSavedData(ism_transport_t * transport, char * buf, int len, int kind) {
    int flen = ism_transport_addMqttFrame(transport, buf, len, kind);
    buf -= flen;
    len += flen;
    transport->pobj->savedData = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_http,1),len);
    memcpy(transport->pobj->savedData, buf, len);
    transport->pobj->savedSize = len;
}


/*
 * Get a message for HTTP outbound.
 *
 * This is called to get a message from the server.  This runs async and completes
 * with a callback.  The data object is the http object.
 *
 * @param http    The HTTP object
 * @param subID   The subscription ID (can be zero)
 * @param topic   The topic name
 * @param timeout The max time to get the message in milliseconds
 * @param subopt  The subscription options (QoS)
 * @param filter  The subscription filter
 * #param callback The reply message callback
 * @return  An ISMRC which indicates if the request is valid
 */
int iot_getMessage(ism_http_t * http, int subID, const char * topic, int timeout,
        int subopt, const char * filter, iot_replyMessage_t callback) {
    ism_transport_t * transport;
    ism_transport_t * stransport;
    if (!http || !http->transport || !topic || !callback) {
        ism_common_setError(ISMRC_Error);
        return ISMRC_Error;
    }
    transport = http->transport;
    stransport = (ism_transport_t *)transport->hout;

    /*
     * If no server transport, make one
     */
    if (stransport) {
        stransport->pobj->active = 1;
        stransport->pobj->replyMsg = callback;
        stransport->pobj->http = http;
        TRACE(8, "Send a GETX to an existing connection: connect=%u name=%s\n", transport->index, transport->name);
        sendGETX(http, subID, topic, timeout, subopt, filter);
    } else {
        char xbuf[2048];
        concat_alloc_t zbuf = {xbuf, sizeof xbuf, 16};
        concat_alloc_t * buf = &zbuf;
        houtPobj_t * pobj;
        char * errMsg;
        char * cid;
        int  rc;

        /*
         * Create outgoing connection
         */
        if (g_useMux) {
            errMsg = alloca(128);
            stransport = ism_mux_createVirtualConnection(transport->server, transport->tid, &rc, errMsg);
        } else {
            rc = ISMRC_AllocateError;
            errMsg = "Memory allocation error.";
            stransport = ism_transport_newOutgoing(transport->endpoint, 1);
        }
        if (!stransport) {
            if (__builtin_expect((__sync_sub_and_fetch(&transport->pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
                ism_hout_doneConnection(transport);
            } else {
                transport->close(transport, rc, 0, errMsg);
            }
            return -1;
        }
        transport->hout = stransport;

        /*
         * Fill in the new outgoing transport object
         */
        stransport->protocol = "hmqtt4";
        stransport->protocol_family = "mqtt";
        pobj = (houtPobj_t *) ism_transport_allocBytes(stransport, sizeof(houtPobj_t), 1);
        memset(pobj, 0, sizeof(houtPobj_t));
        stransport->pobj = pobj;
        stransport->receive = ism_hout_receive;
        stransport->frame = ism_transport_frameMqtt;
        stransport->addframe = ism_transport_addMqttFrame;
        stransport->actionname = ism_mqtt_mqttCommand;
        stransport->auth_permissions = 0xffffff;
        stransport->closing = ism_hout_closing;
        pthread_spin_init(&pobj->lock, 0);
        stransport->tid = transport->tid;
        stransport->connected = ism_hout_connected;
        stransport->pobj->client_transport = transport;
        stransport->pobj->transport = stransport;
        stransport->pobj->replyMsg = callback;
        stransport->tenant = transport->tenant;
        stransport->server = transport->server;
        stransport->client_class = transport->client_class=='g' ? 'H' : 'h';
        stransport->originated = 1;
        stransport->pobj->connectPending = 1;
        stransport->name = ism_transport_putString(stransport, transport->name);
        cid = (char *)stransport->name;
        *cid = stransport->client_class;
        stransport->clientID = stransport->name;
        TRACE(7, "Make outgoing connection. connect=%u clientID=%s org=%s server=%s client connect=%u\n",
                stransport->index, stransport->name, stransport->tenant->name, stransport->server->name,
				transport->index);
        stransport->ready = 3;
        transport->ready = 3;

        /* Make an outgoing connection and continue in the reply */
        ism_common_allocBufferCopyLen(buf, "\0\6MQTTpx\4", 9);
        bputchar(buf, 0);     /* basic flags */
        int  more_flags = XCFLAG_Domain | XCFLAG_ExtraFlags;
        bputchar(buf, (char)more_flags);   /* Additional flags */

        /* Keepalive */
        bputchar(buf, 0);
        bputchar(buf, 0);

        /* client ID */
        int clientIdLen = strlen(stransport->name);
        bputchar(buf, (char)clientIdLen>>8);
        bputchar(buf, (char)clientIdLen);
        ism_common_allocBufferCopy(buf, stransport->name);
        buf->used--;

        /* Send org name */
        bputchar(buf, (char)(transport->tenant->namelen>>8));
        bputchar(buf, (char)transport->tenant->namelen);
        ism_common_allocBufferCopyLen(buf, transport->tenant->name, transport->tenant->namelen);

        /* Add in the extension */
        if (more_flags & XCFLAG_ExtraFlags) {
            int extlen;
            int extloc = ism_protocol_reserveBuffer(buf, 2);
            /* Put out session expire */
            ism_common_putExtensionValue(buf, EXIV_ExpireTTL, g_httpOutSessionExpire);
            /* Check if we have ACLs */
            if (transport->client_class == 'H') {
            	if (!(transport->tenant->allow_anon==1 && transport->tenant->check_user)){
            		ism_common_putExtensionValue(buf, EXIV_ACLWait, 1);
            		transport->has_acl = 2;
            	}
            }
            ism_common_putExtensionValue(buf, EXIV_ExpectedMsgRate, EXPECTEDMSGRATE_LOW);
            extlen = buf->used-extloc-2;
            buf->buf[extloc] = (char)(extlen<<8);
            buf->buf[extloc+1] = (char)(extlen);
        }

        /* Save GETX values for when connect is done */
        setSavedData(stransport, buf->buf+16, buf->used-16, (MT_CONNECT<<4));
        stransport->pobj->http = http;
        stransport->pobj->subID = subID;
        stransport->pobj->timeout = timeout;
        stransport->pobj->subopt = subopt;
        replaceString(&stransport->pobj->filter, filter);
        replaceString(&stransport->pobj->topic, topic);

        if (g_useMux) {
            ism_hout_connected(stransport, 0);
        } else {
            rc = ism_transport_connect(stransport, transport, transport->tenant, transport->server->tlsCTX);
            return rc;
        }
    }
    return 0;
}

/*
 * Notify HTTP outbound when the client connection is closing
 *
 * This must be called when any connection which has issued a iot_getMessage is closing.
 * This will close the HTTP outbound connections to the server if one exists.
 *
 * @param transport  The client to proxy connection
 * @param rc         The return code of the close
 * @param reason     The reason string of the close
 */
void iot_doneConnection(ism_transport_t * transport, int rc, const char * reason) {
    ism_transport_t * stransport = (ism_transport_t *)transport->hout;
    if (stransport) {
    	houtPobj_t * pobj = (houtPobj_t *) stransport->pobj;
    	if (pobj) {
    		if (!__sync_bool_compare_and_swap(&pobj->clientClosing, 0, 1)) {
    			return;
    		}
    	}
        if (!reason)
            reason = "Client connection closed";
        TRACE(8, "iot_doneConnection: connect=%d rc=%d\n", transport->index, rc);
        stransport->close(stransport, rc, !rc, reason);
    }
}


/*
 * Test reply message
 */
void test_replyMessage(ism_http_t * http, int rc, const char * reason, int close,
             int subID, const char * topic, const char * payload, int len) {
    if (rc) {
        printf("replyMessage: rc=%d reason=%s close=%d\n", rc, reason, close);
    } else {
        if (!payload)
            len = 0;
        char * pbuf = alloca(len+1);
        if (len)
            memcpy(pbuf, payload, len);
        pbuf[len] = 0;
        printf("replyMessage: subID=%d topic=%s payload=%s\n", subID, topic, pbuf);
    }
}


/*
 * Hout initialization
 */
int ism_hout_init(void) {
    g_useMux = ism_common_getIntConfig("MqttUseMux", 1);
    TRACE(3, "HTTP Outbound use mux connection=%d\n", g_useMux);
    return 0;
}
