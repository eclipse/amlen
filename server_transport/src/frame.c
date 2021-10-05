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
static int allowMQTTpxProtocol = 0;
static int frameMux(ism_transport_t * transport, char * buffer, int pos, int avail, int * used);
int ism_transport_allowConnection(ism_transport_t * transport);
int ism_transport_httpframer(ism_transport_t * transport, char * buf, int pos, int avail, int * used);
ism_transport_t * ism_transport_findStream(ism_transport_t * transport, int stream);

/*
 * Do not add a frame
 */
int ism_transport_addNoFrame(ism_transport_t * transport, char * buffer, int len, int kind) {
    return 0;
}

/*
 * Simple direct frame
 */
int ism_transport_noFrame(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    int  buflen = avail-pos;
    transport->receive(transport, buffer+pos, buflen, 0);
    *used += buflen;
    return 0;
}

/*
 * Add the frame to before the current buffer position.
 * The JMS frame is a 4 byte big endian length.
 */
HOT static int addJmsFrame(ism_transport_t * transport, char * buffer, int len, int kind) {
    int biglen = ntohl(len);
    memcpy(buffer-4, &biglen, 4);

    if (SHOULD_TRACEC(9, Jms)) {
        char trcbuf[128];
        int maxsize;
        if (transport->actionname) {
            const char * actionname = transport->actionname(buffer[0]);
            if (strstr(actionname, "message"))
                maxsize = ism_common_getTraceMsgData()+20;   /* Limit the trace of messages */
            else if (!strcmp(actionname, "reply"))
                maxsize = 26;                                /* Limit dump of reply to init */
            else
                maxsize = len;                               /* Trace all of other actions */
            snprintf(trcbuf, sizeof trcbuf, "JMS send %s connect=%u", actionname, transport->index);
        } else {
            sprintf(trcbuf, "JMS send connect=%u", transport->index);
            maxsize = ism_common_getTraceMsgData()+20;       /* Limit the trace of messages */
        }
        traceDataFunction(trcbuf, 0, __FILE__, __LINE__, buffer, len, maxsize);
    }
    return 4;
}


/*
 * Implement the JMS framing.
 * If there is a whole message in the buffer, call the protocol receive.
 * The JMS frame is always a 4 byte big endian length.
 */
HOT static int frameJms(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    int  biglen;
    int  buflen = avail-pos;
    int  mlen;

    if (buflen < 4)
        return 4;
    memcpy(&biglen, buffer+pos, 4);
    mlen = ntohl(biglen);

    /* If not enough space, return the space needed */
    if (mlen+4 > buflen) {
        if (transport->rcvState || ((mlen+4) < MAX_FIRST_MSG_LENGTH))
            return mlen+4;
        transport->close(transport, 165, 0, "The initial packet is too large");
        return -1;
    }

    /* Call the protocol receive handler */
    if (__builtin_expect((mlen > 0), 1)) {
        transport->rcvState = 1;
        if (transport->receive(transport, buffer+pos+4, mlen, 0))
            return -1;                /* Connection closed */
    }
    /* Mark this message as used */
    *used += (mlen+4);
    return 0;
}


/*
 * Add the frame to before the current buffer position.
 * The plugin frame is a command, hdrcount, and a 4 byte big endian length.
 */
HOT static int addPluginFrame(ism_transport_t * transport, char * buffer, int len, int kind) {
    int biglen = ntohl(len);
    memcpy(buffer-4, &biglen, 4);
    buffer[-5] = (char)kind;
    buffer[-6] = (char)(kind>>8);

    if (SHOULD_TRACEC(9, Plugin)) {
        char trcbuf[128];
        int maxsize = ism_common_getTraceMsgData()+8;
        const char * actionname = transport->actionname(kind>>8);
        snprintf(trcbuf, sizeof trcbuf, "Plug-in send %s %u channel=%u",
                actionname, kind&0xff, transport->clientport);
        traceDataFunction(trcbuf, 0, __FILE__, __LINE__, buffer, len, maxsize);
    }
    return 6;
}

/*
 * Add the frame to before the current buffer position.
 * The plugin frame is a command, hdrcount, and a 4 byte big endian length.
 */
static int addAdminClientFrame(ism_transport_t * transport, char * buffer, int len, int kind) {
    int value = ntohl(len);
    memcpy(buffer-8, &value, 4);
    value = ntohl(kind);
    memcpy(buffer-4, &value, 4);
    if (SHOULD_TRACEC(9, Admin)) {
        char trcbuf[128];
        int maxsize = ism_common_getTraceMsgData()+8;
        snprintf(trcbuf, sizeof trcbuf, "MQCAdmin send: msg=%s\n", buffer);
        traceDataFunction(trcbuf, 0, __FILE__, __LINE__, buffer, len, maxsize);
    }
    return 8;
}


/*
 * Add the frame to before the current buffer position.
 * The plugin frame is a command, hdrcount, and a 4 byte big endian length.
 */
HOT static int addFwdFrame(ism_transport_t * transport, char * buffer, int len, int kind) {
    int biglen = ntohl(len);
    memcpy(buffer-4, &biglen, 4);
    buffer[-5] = (char)kind;
    buffer[-6] = (char)(kind>>8);

    if (SHOULD_TRACEC(9, Forwarder)) {
        char trcbuf[128];
        int maxsize = ism_common_getTraceMsgData()+8;
        const char * actionname = transport->actionname(kind>>8);
        snprintf(trcbuf, sizeof trcbuf, "Forwarder send %s %u connect=%u",
                actionname, kind&0xff, transport->index);
        traceDataFunction(trcbuf, 0, __FILE__, __LINE__, buffer, len, maxsize);
    }
    return 6;
}


/*
 * Implement the plug-in framing.
 * If there is a whole message in the buffer, call the protocol receive.
 * The JMS frame is always a 4 byte big endian length.
 */
static int framePlugin(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    int  biglen;
    int  buflen = avail-pos;
    int  mlen;
    uint8_t * bp = (uint8_t *)(buffer+pos);

    if (buflen < 6)
        return 6;
    memcpy(&biglen, bp+2, 4);
    mlen = ntohl(biglen);
    /* If not enough space, return the space needed */
    if (mlen+6 > buflen)
        return mlen+6;

    if (transport->receive(transport, (char *)bp+6, mlen, (((int)bp[0])<<8) + bp[1]))
        return -1;                /* Connection closed */
    *used += (mlen+6);
    return 0;
}

static int frameAdminClient(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    int  mlen = 0;
    int  buflen = avail-pos;
    int corID = 0;
    char * buf = buffer + pos;

    if (buflen < 12)
        return 12;
    mlen = ntohl(*((int*)buf));
    /* If not enough space, return the space needed */
    if (mlen+12 > buflen)
        return mlen+12;
    buf += sizeof(mlen);
    corID = ntohl(*((int*)buf));
    buf += sizeof(corID);

    if (transport->receive(transport, buf, mlen, corID))
        return -1;                /* Connection closed */
    *used += (mlen+12);
    return 0;
}


/*
 * Implement the MQTT framing.
 * The MQTT frame consists of a one byte command, followed by a 1 to 5 byte length where the lower
 * seven bits of each byte contain the value, and the high order bit says to continue to the next byte.
 */
HOT static int frameMqtt(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    char * bp = buffer += pos;
    int  kind;
    int  len;
    int  count = 2;
    int  buflen = avail-pos;
    int  multshift = 7;

    if (buflen < 2)
        return 2;
    kind = bp[0];
    len =  (uint8_t)bp[1];
    buflen -= 2;
    bp += 2;
    /* Handle a multi-byte length */
    if (len & 0x80) {
        len &= 0x7f;
        do {
            count++;
            if (count > 5) {
                TRACE(5, "frameMqtt: The MQTT length is too long: connect=%u From=%s:%u\n",
                        transport->index, transport->client_addr,transport->clientport);
                transport->close(transport, ISMRC_BadLength, 0, "The MQTT packet length is not valid");
                return -1;
            }
            if (buflen <= 0)
                return len + count;
            len += (*bp++ & 0x7f) << multshift;
            multshift += 7;
            buflen--;
        } while ((bp[-1]&0x80));
    }

    /*
     * If we have a whole message in the buffer
     */
    if (len <= buflen) {
        transport->rcvState = 1;
        /* Call the protocol receive handler */
        if (transport->receive(transport, bp, len, kind))
            return -1;                /* Connection closed */
        /* Show the message is consumed */
        *used += (len+count);
        return 0;
    } else {
        if (transport->rcvState || ((len+count) < (5*MAX_FIRST_MSG_LENGTH)))
            return len+count;
        transport->close(transport, 165, 0, "The initial packet is too large");;
        return -1;
    }
}


/*
 * Add the frame to before the current buffer position
 */
XAPI HOT int ism_transport_addMqttFrame(ism_transport_t * transport, char * buf, int len, int command) {
    int  lenlen = -1;

    if (__builtin_expect((len > 268435455 || len < 0), 0)) {
        return lenlen;
    }

    if (SHOULD_TRACEC(9, Mqtt)) {
        char xbuf [128];
        int maxsize = ism_common_getTraceMsgData();

        if ((command>>4) != 3 && maxsize < 1000)
            maxsize = 1000;

        if (transport->actionname) {
            sprintf(xbuf, "MQTT send %02x %s connect=%u", command&0xff, transport->actionname(command), transport->index);
        } else {
            sprintf(xbuf, "MQTT send %02x connect=%u", command&0xff, transport->index);
        }
        traceDataFunction(xbuf, 0, __FILE__, __LINE__, buf, len, maxsize);
    }

    if (len<=127) {
        lenlen = 2;
        buf[-2] = (char)command;
        buf[-1] = (char)(len&0x7f);
    } else if (len <= 16383) {
        lenlen = 3;
        buf[-3] = (char)command;
        buf[-2] = (char)((len&0x7f)|0x80);
        buf[-1] = (char)(len>>7);
    } else if (len <= 2097151) {
        lenlen = 4;
        buf[-4] = (char)command;
        buf[-3] = (char)((len&0x7f)|0x80);
        buf[-2] = (char)((len>>7)|0x80);
        buf[-1] = (char)(len>>14);
    } else {
        lenlen = 5;
        buf[-5] = (char)command;
        buf[-4] = (char)((len&0x7f)|0x80);
        buf[-3] = (char)((len>>7)|0x80);
        buf[-2] = (char)((len>>14)|0x80);
        buf[-1] = (char)(len>>21);
    }
    return lenlen;
}

/*
 * Implement the Multiplex framing.
 * Parse a multiplex frame
 */
static int frameMux(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    int buflen = avail-pos;
    uint32_t biglen;
    uint16_t stream;
    int      mlen;

    if (buflen < 7)
        return 7;
    buffer += pos;
    memcpy(&biglen, buffer, 4);
    buffer += 4;
    mlen = ntohl(biglen);

    /* Call the protocol receive handler */
    if (__builtin_expect((mlen > 0), 1)) {
        ism_muxHdr_t hdr = {0};
        /* If not enough space, return the space needed */
        if (mlen+4 > buflen) {
            if (transport->rcvState || ((mlen+4) < 2048))
                return mlen+4;
            transport->close(transport, ISMRC_FirstPacketTooBig, 0, "The initial packet is too large");
            return -1;
        }
        hdr.hdr.cmd = buffer[0];
        buffer++;
        memcpy(&stream, buffer, 2);
        buffer += 2;
        hdr.hdr.stream = ntohs(stream);
        transport->rcvState = 1;
        mlen -= 3;
        if (transport->receive(transport, buffer, mlen, hdr.iValue))
            return -1;                /* Connection closed */
    }
    /* Mark this message as used */
    *used += (mlen+7);
    return 0;
}


/*
 * Add the frame to before the current buffer position
 */
static int addMuxFrame(ism_transport_t * transport, char * buf, int len, int command) {
    uint16_t stream;
    int biglen = ntohl(len+3);
    ism_muxHdr_t hdr;
    hdr.iValue = command;
    buf -= 7;
    memcpy(buf, &biglen, 4);
    buf += 4;
    buf[0] = hdr.hdr.cmd;
    buf++;
    stream = htons(hdr.hdr.stream);
    memcpy(buf, &stream, 2);

    if (SHOULD_TRACEC(9, Protocol)) {
        char xbuf [128];
        int maxsize = ism_common_getTraceMsgData();
        int cmd = hdr.hdr.cmd;
        stream = hdr.hdr.stream;
        if (transport->actionname) {
            sprintf(xbuf, "mux send %02x %s stream=%u connect=%u",
                    cmd, transport->actionname(cmd), stream, transport->index);
        } else {
            sprintf(xbuf, "mux send %02x stream=%u connect=%u",
                    cmd, stream, transport->index);
        }
        traceDataFunction(xbuf, 0, __FILE__, __LINE__, buf, len, maxsize);
    }
    return 7;
}


/*
 * Framing control for handshake.
 * When we get the first buffer on a connection, determine the protocol to use.
 */
static int handshake(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    uint8_t ch;
    uint8_t ch2;
    int     buflen = avail-pos;
    int  rc = -1;
    int  istls = 0;
    int  assume_http = 0;

    /*
     * Require at least two characters for handshake.  This handles a case in HTTP where only
     * a single byte is sent to overcome the BEAST exploit.
     */
    if (buflen < 2)
        return 2;

    ch  = (uint8_t)buffer[pos];
    ch2 = (uint8_t)buffer[pos+1];

    TRACE(8, "Handshake connect=%d, data=%02x%02x\n", transport->index, ch, ch2);

    /*
     * JMS starts with a four byte length field of the initConnection message
     */
    do {

        /*
         * MQTT binary starts with a CONNECT command
         */
        if (ch == 0x10) {
            int  i;
            if (buflen < 12) {
                return 12;
            }
            for (i=2; i<4; i++) {                /* Skip over length field */
                if (buffer[pos+i] == 0) {
                    if (buffer[pos+i+1]==0)         /* Final length byte might be zero */
                        i++;
                    break;
                }
            }
            i += pos+1;                          /* Skip over the zero */
            if ((buffer[i]==0x04 && !memcmp(buffer+i+1, "MQTT", 4)) ||
                (buffer[i]==0x06 && !memcmp(buffer+i+1, "MQIsdp", 6)) ||
                (buffer[i]==0x06 && !memcmp(buffer+i+1, "MQTTpx", 6))) {
                transport->frame = frameMqtt;
                transport->addframe = ism_transport_addMqttFrame;
                transport->protocol = "mqtt-tcp";
                rc = ism_transport_findProtocol(transport);
                if (!rc)
                    TRACE(8, "MQTT connection connect=%d port=%d\n", transport->index, transport->serverport);
                break;
            }
        }

        /*
         * JMS connection starts with an init connection
         */
        if (ch == 0 && ch2 == 0) {               /* This can only be the init connection which is small */
            if (buflen < 8) {
                return 8;
            }
            if (buffer[pos+4] == 40) {   /* InitConnection */
                transport->frame =  frameJms;
                transport->addframe = addJmsFrame;
                transport->protocol = "tcpjms";
                rc = ism_transport_findProtocol(transport);
                if (!rc)
                    TRACE(8, "JMS connection connect=%d port=%d\n", transport->index, transport->serverport);
                break;
            }
            /*
             * Check for mux protocol.  This uses the same flag as the MQTT proxy protocol
             */
            if (allowMQTTpxProtocol) {
                if (buffer[pos+4] == 'M' && buffer[pos+5] == 'U' && buffer[pos+6] == 'X') {
                    transport->addframe = addMuxFrame;
                    transport->frame = frameMux;
                    transport->protocol = "mux";
                    rc = ism_transport_findProtocol(transport);
                    if (!rc)
                        TRACE(5, "Multiplex connection connect=%d port=%d\n", transport->index, transport->serverport);
                    break;
                }
            }
        }

        /*
         * Detect TLS
         * client hello is 22 (0x16) and the next byte is the major byte of the version:
         * 0x02 = SSLv2 0x03 = SSLv3 (3.0), TLSv1.0 (3.1), TLSv1.1, (3.2) or TLSv1.2 (3.3) or TLSv1.3 (3.4)
         */
        if (ch == 0x16) {
            if (ch2 == 2 || ch2 == 3) {
                if (transport->listener->secure == 2) {
                    rc = createTlsObjects(transport, (const char *)(buffer+pos), buflen);
                    *used = buflen;   /* Mark the buffer as used */
                    if (!rc)
                        TRACE(8, "TLS connection connect=%d port=%d\n", transport->index, transport->serverport);
                    pthread_spin_unlock(&transport->lock);
                    return rc;
                }
                istls = 1;
            }
        }


        /*
         * Plugin and forwarder internal communications
         */
        if (transport->listener->protomask & PMASK_Internal) {
            if (ch == 0x0f && ch2 == 0) {
                transport->addframe = addPluginFrame;
                transport->frame = framePlugin;
                transport->protocol = "plugin";
                rc = ism_transport_findProtocol(transport);
                break;
            }
            if (ch == 0x0e && (ch2 > 2 && ch2 < 9)) {
                transport->addframe = addFwdFrame;
                transport->frame = framePlugin;         /* Same as plugin */
                transport->protocol = "fwd";
                rc = ism_transport_findProtocol(transport);
                break;
            }
        }

        /*
         * If this is any valid http command, reserve it for use by WebSockets even
         * though WebSockets only uses the GET with update.
         */
        if (transport->listener->transmask & TMASK_WebSockets) {
            if (ch == 'G' || ch == 'H' || ch == 'P' || ch == 'O' || ch =='D' || ch == 'T') {
                if (buflen < 8) {
                    return 8;
                }

                if (!memcmp(buffer+pos, "GET ", 4) ||
                    !memcmp(buffer+pos, "HEAD ", 5) ||
                    !memcmp(buffer+pos, "PUT ", 4) ||
                    !memcmp(buffer+pos, "POST ", 5) ||
                    !memcmp(buffer+pos, "PATCH ", 6) ||
                    !memcmp(buffer+pos, "OPTIONS ", 8) ||
                    !memcmp(buffer+pos, "DELETE ", 7) ||
                    !memcmp(buffer+pos, "TRACE ", 6)) {
                    transport->frame = ism_transport_httpframer;          /* The connection is known to be http */
                    transport->addframe = ism_transport_addNoFrame;
                    transport->protocol = "http";
                    rc = 0;
                    break;
                } else {
                    assume_http = 1;
                }
            }
        }

        /*
         * Call any registered frame checkers.
         * This is done after all internal checkers have been called,
         */
        rc = ism_transport_findFramer(transport, buffer, buflen, used);
        if (rc == 0)
            break;
        if (rc > 0) {
            return rc;
        }

        /*
         * Legacy MQTT
         * This will just produce a different error as any valid MQTT is parsed above.
         */
        if (!istls && ch >= 0x10 && ch <= 0x1F) {
            transport->frame = frameMqtt;
            transport->addframe = ism_transport_addMqttFrame;
            transport->protocol = "mqtt-tcp";
            rc = ism_transport_findProtocol(transport);
            break;
        }

        /*
         * If it looks like HTTP and there is no other protocol, go thru HTTP to get the errors
         */
        if (assume_http) {
            transport->frame = ism_transport_httpframer;          /* The connection is known to be http */
            transport->addframe = ism_transport_addNoFrame;
            transport->protocol = "http";
            rc = 0;
            break;
        }
    } while(0);


    ism_transport_dumpTransport(7, transport, "handshake", 0);

    /*
     * Verify that we have a protocol
     */
    if (rc) {
        if (rc < -99) {
            /* Logged in wstcp.c */
        } else if (transport->protocol && *transport->protocol) {
            LOG(WARN, Connection, 1109, "%s%d%s%d%s", "No protocol handler found on connection: Protocol={4} From={0}:{1} "
                    "Server={2}:{3}.",
                    transport->client_addr, transport->clientport,
                    transport->server_addr, transport->serverport, transport->protocol);
            if (SHOULD_TRACEC(4, Tcp)) {
                char xbuf [128];
                sprintf(xbuf, "Handshake failed (No protocol handler found for %s): connect=%u", transport->protocol, transport->index);
                traceDataFunction(xbuf, 0, __FILE__, __LINE__, (buffer+pos), buflen, 16);
            }
        } else {
            LOG(WARN, Connection, 1110, "%s%d%s%d", "Unknown connection handshake on connection: From={0}:{1} "
                    "Server={2}:{3}.",
                    transport->client_addr, transport->clientport, transport->server_addr, transport->serverport);
            if (SHOULD_TRACEC(4, Tcp)) {
                char xbuf [128];
                sprintf(xbuf, "Unknown connection handshake: connect=%u", transport->index);
                traceDataFunction(xbuf, 0, __FILE__, __LINE__, (buffer+pos), buflen, 16);
            }
        }
        __sync_add_and_fetch(&transport->listener->stats->bad_connect_count, 1);
        if (transport->closed)
            transport->closed(transport);
        return -1;
    }
    if (__sync_bool_compare_and_swap(&transport->state, ISM_TRANST_Opening, ISM_TRANST_Open))
        return ism_transport_allowConnection(transport);
    return -1;
}

/*
 * Return the handshake framer
 */
ism_transport_frame_t ism_transport_getHandshake(void) {
    return handshake;
}

/*
 * Initialize framer
 */
void ism_transport_frame_init(void) {
    allowMQTTpxProtocol = ism_common_getBooleanConfig("Protocol.AllowMqttProxyProtocol", 0);
}

/*
 * Check for allow connection and log
 */
int ism_transport_allowConnection(ism_transport_t * transport) {
    /*
     * Check if the protocol is allowed
     */
    int allowed = 1;
    uint64_t active;
    uint64_t count;

    // printf("allowConnect name=%s family=%s\n", transport->name, transport->protocol_family);
    if (transport->protocol_family) {
        if (!*transport->protocol_family)
            return 0;
        uint64_t mask;
        if (!strcmp(transport->protocol_family, "mqtt"))
            mask = PMASK_MQTT;
        else if (!strcmp(transport->protocol_family, "admin"))
            mask = PMASK_Admin;
        else if (!strcmp(transport->protocol_family, "jms"))
            mask = PMASK_JMS;
        else if (!strcmp(transport->protocol_family, "MQ"))
            mask = PMASK_MQConn;
        else if (!strcmp(transport->protocol_family, "rmsg"))
            mask = PMASK_RMSG;
        else if (!strcmp(transport->protocol_family, "fwd"))
            mask = PMASK_Cluster;
        else
            mask = ism_transport_pluginMask(transport->protocol_family, 0);
        allowed = transport->listener->protomask & mask;
    }
    if (!allowed) {
        ism_common_setError(ISMRC_ConnectNotAuthorized);
        LOG(WARN, Connection, 1108, "%s%-s%s%d", "The protocol is not allowed on this endpoint: Protocol={0} Endpoint={1} From={2}:{3}.",
                transport->protocol_family, transport->listener->name, transport->client_addr, transport->clientport);
        transport->closed(transport);
        __sync_add_and_fetch(&transport->listener->stats->bad_connect_count, 1);
        return -1;
    }

    /*
     * There is a small window where a connection comes in on a disabled endpoint.
     * Close the connection if this is the case.  If the endpoint goes reenabled then
     * it is OK since this connection would then be validated by the new endpoint.
     */
    if (transport->listener->enabled == 0) {
        TRACE(5, "A connection is closed because the endpoint is not enabled: %s\n", transport->listener->name);
        if (transport->closed)    /* CUnit does not have full transport object */
            transport->closed(transport);
    }

    /*
     * Log the connection start
     */
    if (!ism_transport_noLog(transport)) {
        LOG(INFO, Connection, 1101, "%u%s%-s%s%d", "New TCP connection: ConnectionID={0} Protocol={1} Endpoint={2} From={3}:{4}.",
                transport->index, transport->protocol, transport->listener->name, transport->client_addr, transport->clientport);
    } else {
        TRACE(6, "New tcp connection: ConnectionID=%u Protocol=%s Endpoint=%s From=%s:%u\n",
                transport->index, transport->protocol, transport->endpoint_name, transport->client_addr, transport->clientport);
    }
    transport->write_bytes += transport->tlsWriteBytes;
    transport->read_bytes += transport->tlsReadBytes;
    count = __sync_add_and_fetch(&transport->listener->stats->connect_count, 1);
    active = __sync_add_and_fetch(&transport->listener->stats->connect_active, 1);
    TRACE(9, "Increment count for connections: connect=%u name=%s count=%lu active=%lu\n",
            transport->index, transport->name, count, active);
    return 0;
}
