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

/*
 * Note: pxframe.c is included in pxtcp.c and therefore does not have its own header includes
 */

/*
 * Forward reference
 */
int ism_transport_allowConnection(ism_transport_t * transport);
ism_transport_t * ism_transport_findStream(ism_transport_t * transport, int stream);
extern int ism_transport_noLog(const char * client);

/*
 * Do not add a frame
 */
int ism_transport_addNoFrame(ism_transport_t * transport, char * buffer, int len, int kind) {
    return 0;
}

/*
 * Just send in whatever arrives and mark it as received
 */
int ism_transport_frameNone(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    int buflen = avail-pos;
    if (transport->receive(transport, buffer+pos, buflen, 0))
        return -1;                /* Connection closed */
    *used += buflen;
    return 0;
}


/*
 * Implement the MQTT framing.
 *
 * The MQTT frame consists of a one byte command, followed by a 1 to 4 byte length where the lower
 * seven bits of each byte contain the value, and the high order bit says to continue to the next byte.
 * The value is encoded in little endian.
 */
HOT int ism_transport_frameMqtt(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
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
                /* The MQTT variable length integer is at most 4 bytes in length */
                TRACE(5, "frameMqtt: The MQTT length is too long: connect=%u from=%s:%u\n",
                        transport->index, transport->client_addr, transport->clientport);
                transport->close(transport, ISMRC_BadLength, 0, "The MQTT packet length is not valid");
                return -1;
            }
            if (buflen <= 0)
                return count;
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
        int rrc = transport->receive(transport, bp, len, kind);
        if (rrc) {
            if (rrc < 0) {     /* Throttle */
                *used += (len+count);  /* The message is consumed */
                return -9;
            }
            return -1;                /* Connection closed */
        }
        /* Show the message is consumed */
        *used += (len+count);
        return 0;
    } else {
        /*
         * If we need more data, check if the requested data is too large and fail at this point.
         * Otherwise return requesting more data
         */
        if (LIKELY(transport->rcvState)) {
            /* On other than first packet, check against the endpoint maxMsgSize+64K */
            if ((transport->maxMsgSize == 0) ||
               ((len+count) < (transport->maxMsgSize+0x10000)))
                return len+count;
            TRACE(5, "frameMqtt: The control packet is too large: connect=%u from=%s:%u size=%u\n", transport->index,
                    transport->client_addr, transport->clientport, len+count);
            transport->close(transport, ISMRC_MsgTooBig , 0, "The MQTT packet is too large");
            return -1;
        } else {
            /* On the first packet, check against the fixed max size */
            if ((len+count) < (MAX_FIRST_MSG_LENGTH))
                return len+count;
            TRACE(5, "frameMqtt: The initial packet is too large: connect=%u from=%s:%u size=%u\n", transport->index,
                    transport->client_addr, transport->clientport, len+count);
            transport->close(transport, ISMRC_FirstPacketTooBig, 0, "The initial packet is too large");
            return -1;
        }
    }
}


/*
 * Add the frame to before the current buffer position.
 *
 * The trace data function only works correctly when SFLAG_FRAMESPACE was specified
 * on the send.
 */
XAPI HOT int ism_transport_addMqttFrame(ism_transport_t * transport, char * buf, int len, int command) {
    int  lenlen = -1;
    int  notrace = command & 0x100;
    command &= 0xff;

    if (__builtin_expect((len > 268435455 || len < 0), 0)) {
        return lenlen;
    }

    if (SHOULD_TRACEC(9, Mqtt) && !notrace) {
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

int ism_transport_httpframer(ism_transport_t * transport, char * buf, int pos, int avail, int * used);

/*
 * Add the frame to before the current buffer position.
 * The Kafka frame is a 4 byte big endian length.
 */
static int addKafkaFrame(ism_transport_t * transport, char * buffer, int len, int kind) {
    int biglen = ntohl(len);
    memcpy(buffer-4, &biglen, 4);

    if (SHOULD_TRACEC(8, Kafka)) {
        char trcbuf[128];
        int maxsize = maxsize = ism_common_getTraceMsgData()+64;
        sprintf(trcbuf, "Kafka send connect=%u", transport->index);
        traceDataFunction(trcbuf, 0, __FILE__, __LINE__, buffer, len, maxsize);
    }
    return 4;
}


/*
 * Implement the Kafka framing.
 * If there is a whole message in the buffer, call the protocol receive.
 * The Kafka frame is always a 4 byte big endian length.
 */
int ism_transport_frameKafka(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    int  biglen;
    int  buflen = avail-pos;
    int  mlen;

    if (buflen < 4)
        return 4;
    memcpy(&biglen, buffer+pos, 4);
    mlen = ntohl(biglen);

    /* If not enough space, return the space needed */
    if (mlen+4 > buflen) {
        if (transport->rcvState || ((mlen+4) < 16*MAX_FIRST_MSG_LENGTH))
            return mlen+4;
        transport->close(transport, 165, 0, "The initial packet is too large");
        return -1;
    }

    /* Call the protocol receive handler */
    if (__builtin_expect((mlen > 0), 1)) {
        transport->rcvState = 1;
        if (SHOULD_TRACEC(8, Kafka)) {
            char trcbuf[64];
            int maxsize = maxsize = ism_common_getTraceMsgData()+64;
            sprintf(trcbuf, "Kafka receive connect=%u", transport->index);
            traceDataFunction(trcbuf, 0, __FILE__, __LINE__, buffer+4, mlen, maxsize);
        }
        if (transport->receive(transport, buffer+pos+4, mlen, 0))
            return -1;                /* Connection closed */
    }
    /* Mark this message as used */
    *used += (mlen+4);
    return 0;
}

/*
 * Implement the Multiplex framing.
 * Parse a multiplex frame
 */
XAPI int ism_transport_frameMux(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
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
int ism_transport_addMuxFrame(ism_transport_t * transport, char * buf, int len, int command) {
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
XAPI int ism_transport_InitialHandshake(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    uint8_t ch;
    uint8_t ch2;
    int     buflen = avail-pos;
    int  rc = -1;

    /*
     * Require at least two characters for handshake.  This handles a case in HTTP where only
     * a single byte is sent to overcome the BEAST exploit.
     */
    if (buflen < 2)
        return 2;

    ch  = (uint8_t)buffer[pos];
    ch2 = (uint8_t)buffer[pos+1];

    TRACE(8, "Handshake id=%d, data=%02x%02x\n", transport->index, ch, ch2);
    pthread_spin_lock(&transport->lock);

    do {

        /*
         * MQTT binary starts with a CONNECT command
         */
        if (ch == 0x10) {
            int  i;
            if (buflen < 12) {
                pthread_spin_unlock(&transport->lock);
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
                (buffer[i]==0x06 && !memcmp(buffer+i+1, "MQIsdp", 6))) {
                transport->frame = ism_transport_frameMqtt;
                transport->addframe = ism_transport_addMqttFrame;
                transport->protocol = "mqtt-tcp";
                rc = ism_transport_findProtocol(transport);
                if (!rc)
                    TRACEX(7, Mqtt, 0, "MQTT connection id=%d from=%s:%u endpoint=%s port=%d\n", transport->index,
                            transport->client_addr, transport->clientport, transport->endpoint_name, transport->serverport);
                break;
            }
        }



        /*
         * Detect SSL/TLS
         * client hello is 22 (0x16) and the next byte is the major byte of the version:
         * 0x02 = SSLv2 0x03 = SSLv3 (3.0), TLSv1.0 (3.1), TLSv1.1, (3.2) or TLSv1.2 (3.3) TLSv1.3 (3.4)
         */
        if (ch == 0x16) {
            if (ch2 == 2 || ch2 == 3) {
#ifdef TLS_Shared
                rc = createTlsObjects(transport, (const char *)(buffer+pos), buflen);
                *used = buflen;   /* Mark the buffer as used */
                if (!rc)
                    TRACE(8, "TLS connection id=%d port=%d\n", transport->index, transport->serverport);
                pthread_spin_unlock(&transport->lock);
                return rc;
#endif
            }
        }



        /*
         * If this is any valid http command, reserve it for use by WebSockets even
         * though WebSockets only uses the GET with update.
         */
        if (transport->endpoint->transmask & TMASK_WebSockets) {
            if (ch == 'G' || ch == 'H' || ch == 'P' || ch == 'O' || ch =='D' || ch == 'T') {
                if (buflen < 8) {
                    pthread_spin_unlock(&transport->lock);
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
                }
            }
        }

        /* TODO: add extension tests */

        /*
         * Legacy MQTT
         * This will just produce a different error as any valid MQTT is parsed above.
         */
        if (ch >= 0x10 && ch <= 0x1F) {
            transport->frame = ism_transport_frameMqtt;
            transport->addframe = ism_transport_addMqttFrame;
            transport->protocol = "mqtt-tcp";
            rc = ism_transport_findProtocol(transport);
        }

    } while(0);

    pthread_spin_unlock(&transport->lock);

    ism_transport_dumpTransport(7, transport, "handshake", 0);

    /*
     * Verify that we have a protocol
     */
    if (rc) {
        if (rc < -99) {
            /* Logged in wstcp.c */
        } else if (transport->protocol && *transport->protocol) {
        	if(ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 1109, TRACE_DOMAIN , transport->name, transport->client_addr, NULL) == 0){
				LOG(WARN, Connection, 1109, "%s%d%s%d%s", "No protocol handler found on connection: Protocol={4} From={0}:{1} "
						"Server={2}:{3}.",
						transport->client_addr, transport->clientport,
						transport->server_addr, transport->serverport, transport->protocol);
        	}
        } else {
        	if(ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 1110, TRACE_DOMAIN, transport->name, transport->client_addr, NULL) ==0){
        		LOG(WARN, Connection, 1110, "%s%d%s%d", "Unknown connection handshake on connection: From={0}:{1} "
						"Server={2}:{3}.",
						transport->client_addr, transport->clientport, transport->server_addr, transport->serverport);
        	}
        }
        __sync_add_and_fetch(&transport->endpoint->stats->bad_connect_count, 1);
        if (transport->closed)
            transport->closed(transport);
        return -1;
    }
    return ism_transport_allowConnection(transport);
}


/*
 * Check for allow connection and log
 */
int ism_transport_allowConnection(ism_transport_t * transport) {
    /*
     * Check if the protocol is allowed
     */
    int allowed = 1;

    if (transport->protocol_family) {
        switch(*transport->protocol_family) {
        case 'a':  allowed = transport->endpoint->protomask & PMASK_Admin;       break;
        case 'j':  allowed = transport->endpoint->protomask & PMASK_JMS;         break;
        case 'm':  allowed = transport->endpoint->protomask & PMASK_MQTT;        break;
        case 'M':  allowed = transport->endpoint->protomask & PMASK_MQConn;      break;
        case '\0': return 0;    /* WebSockets, we do not know yet */
        default:   break;
        }
    }
    if (!allowed) {
    	if(ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 1108, TRACE_DOMAIN, transport->name, transport->client_addr, NULL) == 0){
			LOG(WARN, Connection, 1108, "%s%-s%s%d", "The protocol is not allowed on this endpoint: Protocol={0} Endpoint={1} From={2}:{3}.",
					transport->protocol_family, transport->endpoint->name, transport->client_addr, transport->clientport);
    	}
        transport->closed(transport);
        __sync_add_and_fetch(&transport->endpoint->stats->bad_connect_count, 1);
        return -1;
    }

    /*
     * There is a small window where a connection comes in on a disabled endpoint.
     * Close the connection if this is the case.  If the endpoint goes reenabled then
     * it is OK since this connection would then be validated by the new endpoint.
     */
    if (transport->endpoint->enabled == 0) {
        TRACE(5, "A connection is closed because the endpoint is not enabled: %s\n", transport->endpoint->name);
        if (transport->closed)    /* CUnit does not have full transport object */
            transport->closed(transport);
    }

    /*
     * Log the connection start
     */
    if (*transport->endpoint->name != '!') {
        if (!ism_transport_noLog(transport->client_addr)) {
             LOG(INFO, Connection, 1101, "%u%s%-s%s%d%s", "New tcp connection: ConnectionID={0} SNI={5} Protocol={1} Endpoint={2} From={3}:{4}",
                transport->index, transport->protocol, transport->endpoint->name, transport->client_addr, transport->clientport,
                transport->sniName ? transport->sniName : "");
        }
    }
    transport->counted = 1;
    transport->write_bytes += transport->tlsWriteBytes;
    transport->read_bytes += transport->tlsReadBytes;
    __sync_add_and_fetch(&transport->endpoint->stats->connect_count, 1);
    __sync_add_and_fetch(&transport->endpoint->stats->connect_active, 1);
    return 0;
}
