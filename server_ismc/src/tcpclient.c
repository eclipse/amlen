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
 * tcpclient.c
 */

#include <ismc_p.h>
#ifdef _WIN32
#include <ws2tcpip.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#define closesocket(s) close(s)
#endif

#ifndef XSTR
#define XSTR(s) STR(s)
#define STR(s) #s
#endif

typedef struct receiver_parm {
    ismc_connection_t * conn;
    int recvBufSize;
} receiver_parm_t;

static int allnumeric(const char * cp);
static int inet_convert(const char * src, uint32_t * dst);
static void * receiver(void * arg, void * context, int value);
static int processData(ismc_connection_t * conn, char * data, int len);
static int sendN(SOCKET sock, char * buffer, int len);
static void raiseException(ismc_connection_t * conn, char * data, int len);
static char * ism_common_allocBufferCopyLenSmall(concat_alloc_t * buf, const char * newbuf, int len);

static ismc_producer_t * findProducerInConnection(ismc_connection_t * conn, int producerId);
static ismc_session_t * findSessionInConnection(ismc_connection_t * conn, int sessionId);

#ifdef _WIN32
static volatile long wsa_inited = 0;
#endif

#define ClientVersion 1000000
/*
 * Make the connection.
 *
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_connect(ismc_connection_t * conn) {
    int   rc = 0;
    const char * server;
    const char * port;
    const char * protocol;
    int recvBuffer;
    int sendBuffer;
    int recvBufferSize;
    struct addrinfo hints = {0};
    struct addrinfo unixinfo;
    struct addrinfo * result = NULL;
    struct addrinfo * rp;
    struct sockaddr_in sa = {0};
    const char * errstr;
    xUNUSED ism_time_t   server_time = 0L;
    xUNUSED int          server_version = 0;
    ism_field_t  field;
    action_t *   action;
    char         versionbuf[24];

    if (!conn)
        return ismc_setError(ISMRC_NullPointer, "The connection object is NULL");
    if (conn->h.id != OBJID_Connection)
        return ismc_setError(ISMRC_ObjectNotValid, "The connection object is not valid");
    if (ismc_getStringProperty(conn, "ClientID") == NULL)
        return ismc_setError(ISMRC_ClientIDRequired, "The client ID for connection object is required");

#ifdef _WIN32
    if (_InterlockedCompareExchange(&wsa_inited, 1, 0) == 0) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
#endif

    pthread_mutex_lock(&conn->lock);

    server     = ismc_getStringProperty(conn, "Server");
    if (!server)
        server = "127.0.0.1";
    port       = ismc_getStringProperty(conn, "Port");
    if (!port)
        port = "16102";
    protocol   = ismc_getStringProperty(conn, "Protocol");
    if (!protocol)
        protocol = "tcp";
    recvBuffer = ismc_getIntProperty(conn, "RecvSockBuffer", -1);
    sendBuffer = ismc_getIntProperty(conn, "SendSockBuffer", -1);
    recvBufferSize = ismc_getIntProperty(conn, "RecvBufferSize", 8192);

    /*
     * Clear buffer-related properties from connection, so that
     * they won't appear in other messaging objects.
     */
    ism_common_setProperty(conn->h.props, "RecvSockBuffer", NULL);
    ism_common_setProperty(conn->h.props, "SendSockBuffer", NULL);
    ism_common_setProperty(conn->h.props, "RecvBufferSize", NULL);

#ifndef _WIN32
    if (*server == '/') {
        struct sockaddr_un sockunix;
        unixinfo = hints;
        unixinfo.ai_family = AF_UNIX;
        unixinfo.ai_socktype = SOCK_STREAM;
        unixinfo.ai_protocol = 0;
        sockunix.sun_family = AF_UNIX;
        ism_common_strlcpy(sockunix.sun_path, server, sizeof (sockunix.sun_path));
        unixinfo.ai_addrlen = SUN_LEN(&sockunix);
        unixinfo.ai_addr = (struct sockaddr *)&sockunix;
        result = &unixinfo;
    } else
#endif
    {
        memset(&hints, 0, sizeof hints);
        hints.ai_family= AF_UNSPEC;           /* Set to AF_INET for IPv4 only */

        /*
         * If the address is numeric, force no DNS
         */
        if (strchr(server, ':')) {
            hints.ai_family = AF_INET6;
            hints.ai_flags = AI_NUMERICHOST;
        } else if (allnumeric(server)) {
            hints.ai_family = AF_INET;
            hints.ai_flags = AI_NUMERICHOST;
        }

        /*
         * Resolve the address
         */
        rc = getaddrinfo(server, port, &hints, &result);
        if (rc) {
            uint32_t addr;
            if (!inet_convert(server, &addr)) {
                result = &hints;
                hints.ai_family = AF_INET;
                hints.ai_addrlen = 4;
                result->ai_addr = (struct sockaddr *) &sa;
                rc = 0;
            } else {
#ifdef _WIN32
                errstr = gai_strerrorA(rc);
#else
                errstr = gai_strerror(rc);
#endif
                rc = ISMRC_UnknownHost;
                ismc_setError(rc, errstr);
            }
        }
    }

    /*
     * Try the results until we get one which works, while ignoring
     * non-TCP sockets.
     */
    if (!rc) {
        for (rp = result; rp != NULL; rp = rp->ai_next) {
        	if (rp->ai_socktype != SOCK_STREAM)
        	    continue;

            conn->sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (conn->sock == (SOCKET)-1) {
                continue;
            }
            if (connect(conn->sock, rp->ai_addr, (int)rp->ai_addrlen) != (SOCKET)-1) {
                break;
            }
            closesocket(conn->sock);
            conn->sock = (SOCKET)-1;
        }
        if (!rp) {
            rc = ISMRC_UnableToConnect;
            ismc_setError(rc, "Cannot find the server to connect to");
        } else {
            receiver_parm_t recvParms;

            /* Switch to non-blocking mode */
            int x = 1;
            ioctl(conn->sock, FIONBIO, &x);

            if (rp->ai_family != AF_UNIX) {
                setsockopt(conn->sock, IPPROTO_TCP, TCP_NODELAY, (const char *) &x, sizeof(x));

                /* Set the receive buffer */
                if (recvBuffer > 0) {
                    setsockopt(conn->sock, SOL_SOCKET, SO_RCVBUF, (const char *)&recvBuffer, sizeof(recvBuffer));
                }

                /* Set the send buffer */
                if (sendBuffer > 0) {
                    setsockopt(conn->sock, SOL_SOCKET, SO_SNDBUF, (const char *)&sendBuffer, sizeof(sendBuffer));
                }
            }

            /* Start the receive thread and wait until it is up and running */
            recvParms.conn = conn;
            recvParms.recvBufSize = recvBufferSize;
            pthread_barrier_init(&recvParms.conn->barrier, NULL, 2);

            ism_common_startThread(&conn->recvThread, receiver, (void *)&recvParms, NULL, 0,
                    ISM_TUSAGE_NORMAL, 0, "ismcr", "Receiver thread for JMS ISMC client");

            pthread_barrier_wait(&recvParms.conn->barrier);

            /*
             * Do the JMS handshake
             */
            action = ismc_newAction(conn, NULL, Action_initConnection);
            ism_protocol_putStringValue(&action->buf, "ismc_MQ");   /* Fixed string for now */
            ism_common_strlcpy(versionbuf, XSTR(BUILD_LABEL), 24);
            ism_protocol_putStringValue(&action->buf, versionbuf);  /* Build ID             */
            ism_protocol_putIntValue(&action->buf, 0);              /* KeepaliveTimeout     */
            action->hdr.hdrcount = 3;
            action->hdr.item = endian_int32(ClientVersion);
            rc = ismc_request(action, 1);
            if (rc == 0) {
                /* Check hdrcount */
                ism_protocol_getObjectValue(&action->buf, &field);
                if (field.type == VT_Integer)
                    server_version = field.val.i;
                ism_protocol_getObjectValue(&action->buf, &field);
                if (field.type == VT_Long)
                    server_time    = field.val.l;
                /* TODO: Server BUILD_ID */
                /* TODO: Trace connection parms */
                ismc_freeAction(action);

                /* Send the createConnection action.  That should be in ismc.c */
                action = ismc_newAction(conn, NULL, Action_createConnection);
                action->hdr.item = endian_int32(ClientVersion);
                ism_protocol_putMapProperties(&action->buf, conn->h.props);
                rc = ismc_request(action, 1);
                ismc_freeAction(action);
                if (rc == 0) {
                    conn->isConnected = 1;
                } else {
                    ismc_setError(ISMRC_NotConnected, "Create connection request was rejected by the server");
                }
            } else {
                ismc_freeAction(action);
                ismc_setError(ISMRC_NotConnected, "Create connection request was rejected by the server");
            }
        }

        /*
         * Free up the results if they were allocated
         */
        if (result && result != &hints && result != &unixinfo) {
            freeaddrinfo(result);
        }
    }

//    if (rc) {
//        char buf[512];
//        rc = ismc_getLastError(buf, sizeof(buf));
//        printf("Error occurred in ismc_connect (%d): %s\n", rc, buf);
//    }

    pthread_mutex_unlock(&conn->lock);

    return rc;
}

/*
 * Disconnect from the server
 *
 * @param connect  The connection object
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
int ismc_disconnect(ismc_connection_t * connect) {
    action_t * act;

    if (!connect)
        return ismc_setError(ISMRC_NullPointer, "The connection object is NULL");
    if (connect->h.id != OBJID_Connection)
        return ismc_setError(ISMRC_ObjectNotValid, "The connection object is not valid");
    if (!connect->isConnected || connect->isClosed)
        return 0;

    /* At this point, connection is valid, proceed with close and disconnect */
    act = ismc_newAction(connect, NULL, Action_closeConnection);
    connect->isConnected = 0;
    ismc_request(act, 1);

    ismc_freeAction(act);

    /*
     * Set up barrier to ensure the receiver thread is done
     * before returning
     */
    connect->isClosed = 1;
    closesocket(connect->sock);
    ism_common_joinThread(connect->recvThread, NULL);

    return 0;
}


/*
 * Send an action to the server
 * @param connect A connection
 * @param action  An action to perform
 * @return 0=good
 */
int ismc_sendAction(ismc_connection_t * connect, action_t * action) {
    fd_set writeFdSet;
    SOCKET sock = connect->sock;
    int maxFd = (int)sock + 1;
    int rc;
    struct timeval timeout;
    char buf[1024];
    int errorCode = 0;

    TRACE(8, "ismc_sendAction len=%u action=%u hdrcount=%u bodytype=%u priority=%u\n",
          action->action_len, action->hdr.action, action->hdr.hdrcount, action->hdr.bodytype, action->hdr.priority);

    /* Initialize set of socket descriptors (of 1) for waiting for write */
    FD_ZERO(&writeFdSet);
    FD_SET(sock, &writeFdSet);

    /* Set select timeout to 1 second */
    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;

    if (connect->isClosed) {
        return ISMRC_Closed;
    }

    pthread_mutex_lock(&connect->senderMutex);

    do {
        /* Do select for write */
        rc = select(maxFd, NULL, &writeFdSet, NULL, &timeout);
        if (connect->isClosed) {
            break;
        }

        if (rc < 0) {
            /* Select failed */
            errorCode = ismc_setError(ISMRC_NetworkError, "Socket connection select for write error: %s",
                    strerror_r(errno, buf, sizeof(buf)));
            break;
        } else if (rc == 0) {
            /* Timeout, stop */
            errorCode = ismc_setError(ISMRC_NetworkError, "Timed out while waiting on select for write");
            break;
        } else {
            /*
             * The socket is ready for writing.
             * First, write the request length.
             * Then, if complete action fits into the action structure buffer,
             * send it in one call.
             * Otherwise, send the action header and the action body separately.
             */

            uint32_t alen = htonl(action->action_len);

            /* Send action length */
            if (sendN(sock, (char*)&alen, sizeof(uint32_t)) < 0) {
                if (connect->isClosed) {
                    break;
                }

                /* Error sending action */
                errorCode = ismc_setError(ISMRC_NetworkError, "Error writing data: %s",
                        strerror_r(errno, buf, sizeof(buf)));
                break;
            }

            /* Check the action request size */
            if (!action->buf.inheap) {
                /* Fits into the buffer, send it in one operation */
                if (sendN(sock, (char*)&(action->hdr), action->action_len) < 0) {
                    if (connect->isClosed) {
                        break;
                    }

                    /* Error sending action */
                    errorCode = ismc_setError(ISMRC_NetworkError, "Error writing data: %s",
                            strerror_r(errno, buf, sizeof(buf)));
                    break;
                }
            } else {
                /* Doesn't fit into the buffer, send the header and body separately */

                /* Send the header */
                if (sendN(sock, (char*)&(action->hdr), sizeof(action->hdr)) < 0) {
                    if (connect->isClosed) {
                        break;
                    }

                    /* Error sending action */
                    errorCode = ismc_setError(ISMRC_NetworkError, "Error writing data: %s",
                            strerror_r(errno, buf, sizeof(buf)));
                    break;
                }

                /* Send the rest of the data */
                if (sendN(sock, action->buf.buf, action->buf.used) < 0) {
                    if (connect->isClosed) {
                        break;
                    }

                    /* Error sending action */
                    errorCode = ismc_setError(ISMRC_NetworkError, "Error writing data: %s",
                            strerror_r(errno, buf, sizeof(buf)));
                    break;
                }
            }
        }
    } while (0);

    pthread_mutex_unlock(&connect->senderMutex);

    if (!connect->isClosed && errorCode) {
        if (connect->errorListener) {
            rc = ismc_getLastError(buf, sizeof(buf));
            connect->errorListener(rc, buf, connect, NULL, connect->userdata);
            TRACE(5, "Error occurred during send: %s\n", buf);
        } else {
            TRACE(5, "Error occurred during send: %d\n", errorCode);
        }
    }

    return errorCode;
}

/*
 * Check for a numeric IPv4 address
 */
static int allnumeric(const char * cp) {
    while (*cp) {
        if ((*cp < '0' || *cp >'9') && *cp != '.')
            return 0;
        cp++;
    }
    return 1;
}


/*
 * Do a simple AF_INET conversion since inet_addr does not differentiate between
 * the value 255.255.255.255 and a bad address.
 */
static int inet_convert(const char * src, uint32_t * dst) {
    uint32_t s1, s2, s3, s4;
    char *   eos;

    do {
        s1 = strtoul(src, &eos, 0);
        if (*eos != '.') {
            if (!*eos) {
                s4 = s1&0xff;
                s3 = (s1>>8)&0xff;
                s2 = (s1>>16)&0xff;
                s1 = (s1>>24)&0xff;
                break;
            }
            return 1;
        }
        s2 = strtoul(eos+1, &eos, 0);
        if (*eos != '.') {
            if (!*eos) {
                s4 = s2&0xff;
                s3 = (s2>>8)&0xff;
                s2 = (s2>>16)&0xff;
                break;
            }
            return 1;
        }
        s3 = strtoul(eos+1, &eos, 0);
        if (*eos != '.') {
            if (!*eos) {
                s4 = s3&0xff;
                s3 = (s3>>8)&0xff;
                break;
            }
            return 1;
        }
        s4 = strtoul(eos+1, &eos, 0);
        if (*eos)
            return 1;
    } while (0);
    if (s1>255 || s2>255 || s3>255 || s4>255)
        return 1;

    s1 = (s1<<24) + (s2<<16) + (s3<<8) + s4;
    *dst = s1;
    return 0;
}

/**
 * Receiver thread.
 */
static void * receiver(void * arg, void * context, int value) {
    receiver_parm_t * recvParms = (receiver_parm_t *)arg;
    int recvBufferSize = recvParms->recvBufSize;
    ismc_connection_t * conn = recvParms->conn;
    SOCKET sock = conn->sock;
    fd_set readFdSet;
    int maxFd = (int)sock + 1;
    struct timeval timeout;
    int nRead = 0;
    int bytesLeft;
    char * buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,1),recvBufferSize);
    char * position = buffer;
    char buf[1024];
    int rc = 0;
    const int LENGTH_FIELD_SIZE = 4;
    int errorCode = 0;

    /* Notify main thread that the receive thread is ready to process responses */
    pthread_barrier_wait(&conn->barrier);

    while (!conn->isClosed) {
        int32_t dataLen = 0;
        int smallMessage = 0; // For ping support

        /* Set select timeout to 1 second */
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        /* Initialize set of socket descriptors (of 1) for waiting on read */
        FD_ZERO(&readFdSet);
        FD_SET(sock, &readFdSet);

        /* Do select on read */
        rc = select(maxFd, &readFdSet, NULL, NULL, &timeout);
        if (conn->isClosed) {
            break;
        }

        if (rc < 0) {
            /* Select failed, stop the thread */
            errorCode = ismc_setError(ISMRC_NetworkError, "Socket connection select for read error: %s",
                    strerror_r(errno, buf, sizeof(buf)));
            break;
        } else if (rc == 0) {
            /* Timeout, keep waiting */
            continue;
        }

        /* Try reading from the socket */
        rc = recv(sock, buffer + nRead, recvBufferSize - nRead, 0);
        if (rc < 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
#endif
                /* Interrupted, keep reading */
                continue;
            } else {
                /* Read failed */
                errorCode = ismc_setError(ISMRC_NetworkError, "Socket connection read error: %s",
                              strerror_r(errno, buf, sizeof(buf)));
                break;
            }
        } else if (rc == 0) {
            /* Connection closed by server */
            errorCode = ismc_setError(ISMRC_NetworkError, "Socket connection closed by server");
            break;
        }

        if (SHOULD_TRACE(9)) {
            char trcbuf[64];
            sprintf(trcbuf, "receiver recv connect=%u", sock);
            TRACEDATA(9, trcbuf, 0, buffer + nRead, rc,
                  ism_common_getTraceMsgData()+sizeof(actionhdr)+6);
        }

        nRead += rc;

        if (nRead < LENGTH_FIELD_SIZE) {
            continue;
        }

        bytesLeft = nRead;
        while (bytesLeft > LENGTH_FIELD_SIZE) {
            uint32_t slen;
            memcpy(&slen, position, LENGTH_FIELD_SIZE);
            dataLen = ntohl(slen);
            if (dataLen < sizeof(actionhdr)) {
//                errorCode = ismc_setError(ISMRC_MessageNotValid, "Received message with invalid length %d", dataLen);
            	TRACE(7, "Received message with invalid length %d - ignore\n", dataLen);
            	smallMessage = 1;
                break;
            }

            /* If the message is not complete, keep reading */
            if (dataLen > (bytesLeft - LENGTH_FIELD_SIZE)) {
                break;
            }

            /* Have full message, process it */
            errorCode = processData(conn, position + LENGTH_FIELD_SIZE, dataLen);
            if (errorCode) {
            	break;
            }

            position  += dataLen + LENGTH_FIELD_SIZE;
            bytesLeft -= (dataLen + LENGTH_FIELD_SIZE);
        }

        if (errorCode) {
            break;
        }
        if (smallMessage) {
        	continue;
        }

        if (bytesLeft <= 0) {
            /* No more data in the buffer, wait for more */
            position = buffer;
            nRead = 0;
            continue;
        }

        if ((dataLen + LENGTH_FIELD_SIZE) > recvBufferSize) {
            /* Buffer is not large enough, resize */
            int64_t offset = position - buffer;
            recvBufferSize = dataLen + 1024;
            buffer = ism_common_realloc(ISM_MEM_PROBE(ism_memory_ismc_misc,2),buffer, recvBufferSize);
            memmove(buffer, buffer + offset, bytesLeft);
            position = buffer;
            nRead = bytesLeft;
        } else {
            /* There is a partial message in the buffer */
            if ((position - buffer) > 0) {
                /* In the middle of the buffer, move to the beginning */
                memmove(buffer, position, bytesLeft);
                position = buffer;
                nRead = bytesLeft;
            }
        }
    }

    closesocket(sock);

    /*
     * Report the error which terminated us
     */
    if (!conn->isClosed && errorCode) {
        if (conn->errorListener) {
            rc = ismc_getLastError(buf, sizeof(buf));
            conn->errorListener(rc, buf, conn, NULL, conn->userdata);
        }
        conn->isConnected = 0;
        conn->isClosed = 1;
    } else {
        /* Normal close - notify main thread that receiver is done */
        errorCode = 0;
    }
    ismc_wakeWaiters(conn, errorCode);

    ism_common_free(ism_memory_ismc_misc,buffer);

    return NULL;
}

static int processData(ismc_connection_t * conn, char * data, int len) {
    actionhdr * header = (actionhdr *)(data);
    int messageType = header->itemtype;
    uint64_t respId = 0;
    ismc_message_t * message;
    action_t * action;

    if (header->action == Action_raiseException) {
        raiseException(conn, data, len);
        return 0;
    }


    if (messageType == ITEMT_None) {        // Message
        int consumerId = endian_int32(header->item);
        ismc_session_t * session = NULL;
        ismc_consumer_t * consumer = NULL;

        /*
         * 1. Find consumer by id
         * 2. Add task to call the message listener associated with the consumer
         */
        consumer = ism_common_getHashMapElementLock(conn->consumers, &consumerId, sizeof(int32_t));
        if (consumer) {
            session = consumer->session;
        }

        if (!consumer) {
        	TRACE(5, "Received a message for unknown consumer with ID %d\n", consumerId);
        	return 0;
        }

        /*
         * If the consumer/message listener is not set, queue the message
         * for sync receive
         */
        if (consumer->onmessage == NULL) {
            /* Sync message delivery through async receipt and delivery upon ismc_request */
            action = ismc_newAction(conn, NULL, 0);
            memcpy(&action->hdr, header, sizeof(actionhdr));
            action->hdr.item = consumerId;
            action->buf.used = 0;
            action->buf.pos = 0;

            ism_common_allocBufferCopyLenSmall(&action->buf, (char*)(header + 1), len - sizeof(actionhdr));

            ismc_consumerCachedMessageAdded(consumer, action);

            ism_common_list_insert_tail(consumer->messages, action);
            return 0;
        }

        action = ismc_newAction(conn, consumer->session, 0);
        memcpy(&action->hdr, header, sizeof(actionhdr));
        action->buf.used = 0;
        action->buf.pos = 0;
        ism_common_allocBufferCopyLenSmall(&action->buf, (char*)(header + 1), len - sizeof(actionhdr));

        message = ismc_makeMessage(consumer, action);

        ismc_freeAction(action);

        if (!message) {
            return 0;
        }

        ismc_consumerCachedMessageAdded(consumer, action);

        /* Apply message selector, if present, before delivering the message */
        if (!consumer->selectRule ||
                (ismc_filterMessage(message, consumer->selectRule) != SELECT_FALSE)) {
            /* Create new task for delivery thread */
            ismc_addTask(session->deliveryThreadId, consumer, message);
        }

    } else {
        action_t * act;
        respId = endian_int64(header->msgid);
        act = ismc_getAction(respId);

        if (act) {
        	/* Complete the action */
        	pthread_mutex_lock(&act->waitLock);

        	if (act->doneWaiting == ISMC_WAITING_MESSAGE) {
        		act->doneWaiting = ISMC_MESSAGE_RECEIVED;
        		ismc_setAction(respId, NULL);

        		memcpy(&act->hdr, header, sizeof(actionhdr));
        		act->buf.used = 0;
        		act->buf.pos = 0;
        		ism_common_allocBufferCopyLenSmall(&act->buf, (char*)(header + 1), len - sizeof(actionhdr));

        		act->parseReply(act);
        	}

        	pthread_cond_signal(&act->waitCond);
        	pthread_mutex_unlock(&act->waitLock);
        } else {
        	TRACE(5, "Act is NULL, respId=%ld\n", respId);
        	return ISMRC_ObjectNotValid;
        }
    }

    return 0;
}

/**
 * Send data to a non-blocking socket.
 * @param sock   A socket file descriptor
 * @param buffer A pointer to a buffer with data
 * @param len    Number of bytes to send
 * @return The number of bytes written or -1 if failure occurred
 */
static int sendN(SOCKET sock, char * buffer, int len) {
    int nWritten = 0;

    if (len <= 0) {
        return 0;
    }

    while (nWritten < len) {
        int n = send(sock, buffer + nWritten, len - nWritten, 0);

        if (n < 0) {
        	char buf[512];
        	const char * errStr;
#ifdef _WIN32
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
            	strerror_s(buf, sizeof(buf), WSAGetLastError());
            	errStr = buf;
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
            	errStr = strerror_r(errno, buf, sizeof(buf));
#endif
            	ismc_setError(ISMRC_NetworkError, "Send failed: %s", errStr);
                return -1;
            } else {
                continue;
            }
        }

        if (n > 0 && SHOULD_TRACE(9)) {
            char trcbuf[64];
            sprintf(trcbuf, "sendN send connect=%u", sock);
            TRACEDATA(9, trcbuf, 0, buffer + nWritten, n,
                      ism_common_getTraceMsgData()+sizeof(actionhdr)+6);
        }

        nWritten += n;
    }

    return nWritten;
}

/**
 * Passes the asynchronously received error to the error listener, if present.
 *
 * Currently only "Message too big" is reported.
 * 1st header - return code
 * If 2 headers, 2nd header is the producer ID
 * If 3 headers, 2nd header is domain type,
 *               3rd header is destination name
 *
 * @param conn   The connection object.
 * @param data   A pointer to the data received.
 * @param len    The length of the data.
 */
void raiseException(ismc_connection_t * conn, char * data, int len) {
    char buf[512];
    actionhdr * header = (actionhdr *)(data);
    ism_field_t field;
    int rc = 0;
    concat_alloc_t cbuf = { 0 };
    int domain = 0;
    const char * domainString;
    const char * destinationName;
    ismc_session_t * sess = NULL;
    ismc_producer_t * prod = NULL;
    int producerId = 0;
    char pIdstr[256];

    /* If error listener is not specified, do not do anything */
    if (!conn->errorListener) {
        TRACE(8, "Error was reported asynchronously, but error listener was not specified\n");
        return;
    }

    if (header->hdrcount < 2) {
        ism_common_setError(ISMRC_BadClientData);
        return;
    }

    /* Set up the buffer for reading data */
    ism_common_allocAllocBuffer(&cbuf, len, 0);
    cbuf.used = 0;
    cbuf.pos = 0;
    ism_common_allocBufferCopyLen(&cbuf, (char*)(header + 1), len - sizeof(actionhdr));

    /* Get the return code */
    ism_protocol_getObjectValue(&cbuf, &field);
    if (field.type == VT_Integer) {
        rc = field.val.i;
    }

    /* Get the session id */
    ism_protocol_getObjectValue(&cbuf, &field);
    if (field.type == VT_Integer) {
        int sessionId = field.val.i;
        sess = findSessionInConnection(conn, sessionId);
    }

    /* Producer ID is available */
    ism_protocol_getObjectValue(&cbuf, &field);
    if (field.type == VT_Integer) {
        producerId = field.val.i;
        sprintf(pIdstr, "%d",producerId);
    } else {
        sprintf(pIdstr, "\"null\"");
    }

    if (producerId != 0) {
        prod = findProducerInConnection(conn, producerId);
        domain = prod->domain;
        domainString = (domain == ismc_Topic)?"topic":"queue";
        destinationName = prod->dest->name;
    } else {
        /* Domain and destination name are available */
        ism_protocol_getObjectValue(&cbuf, &field);
        if (field.type == VT_Byte) {
            domain = field.val.i;
            domainString = (domain == ismc_Topic)?"topic":"queue";
        } else {
            domainString = "destination";
        }
        ism_protocol_getObjectValue(&cbuf, &field);
        if (field.type == VT_String) {
            destinationName = field.val.s;
        } else {
            destinationName = "unknown";
        }
    }
    /* Error message substitutions are different for each return code */
    switch (rc) {
    case ISMRC_MsgTooBig:
    {
        const char * server = NULL;
        const char * portstr;
        if (producerId != 0) {
            /* Get previously unread headers */
            ism_protocol_getObjectValue(&cbuf, &field);
            ism_protocol_getObjectValue(&cbuf, &field);
        }
        server = ismc_getStringProperty(conn, "Server");
        if (!server)
            server = "unknown_server";
        portstr = ismc_getStringProperty(conn, "Port");
        if (!portstr)
            portstr = "unknown_port";
        sprintf(buf,"Message for %s %s from producer %s is too big for %s:%s",domainString, destinationName, pIdstr, server, portstr);
        break;
    }
    case ISMRC_DestinationFull:
        sprintf(buf,"Failed to send message for %s %s from producer %s. Destination is full.",domainString, destinationName, pIdstr);
        break;
    case ISMRC_DestNotValid:
        sprintf(buf,"Failed to send message for %s %s from producer %s. Destination is not valid.",domainString, destinationName, pIdstr);
        break;
    case ISMRC_SendNotAllowed:
        sprintf(buf,"Failed to send message for %s %s from producer %s. Sending of messages to destination is not allowed.",domainString, destinationName, pIdstr);
        break;
    }

    conn->errorListener(rc, buf, conn, sess, conn->userdata);
}

/**
 * Find a producer from the connection and producer ID.
 * @param conn  A connection
 * @param producerId  A producer ID value
 */
ismc_producer_t * findProducerInConnection(ismc_connection_t * conn, int producerId) {
    int i, j;

    if (!conn) {
    	return NULL;
    }

    pthread_spin_lock(&conn->h.lock);

    do {
        if (conn->sessions.array == NULL) {
            pthread_spin_unlock(&conn->h.lock);
            return NULL;
        }

        for (i = 0; i < conn->sessions.numElements; i++) {
            ismc_session_t * session = ((ismc_session_t * *)conn->sessions.array)[i];
                for (j = 0; j < session->producers.numElements; j++) {
                    ismc_producer_t * prod = ((ismc_producer_t * *)session->producers.array)[j];
                    if (prod->producerid == producerId) {
                        pthread_spin_unlock(&conn->h.lock);
                        return prod;
                    }
                }
        }
    } while (0);

    pthread_spin_unlock(&conn->h.lock);
    return NULL;
}

/**
 * Find a session from the connection and session ID.
 * @param conn       A connection
 * @param sessionId  A session ID value
 * @return A pointer to the session
 */
ismc_session_t * findSessionInConnection(ismc_connection_t * conn, int sessionId) {
    int i;

    if (!conn) {
    	return NULL;
    }

    pthread_spin_lock(&conn->h.lock);

    do {
        if (conn->sessions.array == NULL) {
            pthread_spin_unlock(&conn->h.lock);
            return NULL;
        }

        for (i = 0; i < conn->sessions.numElements; i++) {
            ismc_session_t * session = ((ismc_session_t * *)conn->sessions.array)[i];
            if (session && session->sessionid == sessionId) {
                pthread_spin_unlock(&conn->h.lock);
                return session;
            }
        }
    } while (0);

    pthread_spin_unlock(&conn->h.lock);
    return NULL;
}

/*
 * Put out within a constrained buffer.
 * Allocates the precise amount of memory requested and thus might be less
 * suited than ism_common_allocBufferCopyLen if re-allocations are needed.
 *
 * @param buf      The constrained buffer
 * @param newbuf   A pointer to the source data
 * @param len      The size of the data to copy
 *
 * @return A pointer to the beginning of the copied data in the constrained buffer
 *         or NULL if memory allocation failed.
 */
char * ism_common_allocBufferCopyLenSmall(concat_alloc_t * buf, const char * newbuf, int len) {
    char * ret;
    if (buf->used + len  > buf->len) {
        int newsize = buf->used + len;
        if (buf->inheap) {
            char * tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_alloc_buffer,4),buf->buf, newsize);
            if (tmp)
                buf->buf = tmp;
            else
                return 0;
        } else {
            char * tmpbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_alloc_buffer,5),newsize);
            if (tmpbuf && buf->used)
                memcpy(tmpbuf, buf->buf, buf->used > buf->len ? buf->len : buf->used);
            buf->buf = tmpbuf;
        }
        if (!buf->buf)
            return 0;
        buf->inheap = 1;
        buf->len = newsize;
    }
    ret = buf->buf+buf->used;
    memcpy(ret, newbuf, len);
    buf->used += len;
    return ret;
}
