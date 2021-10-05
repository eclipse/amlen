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
 * Simple tcp server
 * This server it a very simple implementation of the ISM server interfaces but is not designed for high
 * speed and only handles websockets.
 *
 * - Only IPv4 is supported
 * - Only one endpoint is supported
 * - The connection count is assumed to be small, so blocking IO is used (one thread per connection)
 * - Sends block the sender until sent
 */
//#include <ismc_p.h>
//#include <admin.h>
#ifndef TRACE_COMP
#define TRACE_COMP Util
#endif

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#ifndef _WIN32
#include <sys/socket.h>
#endif
#include <ismutil.h>
#define closesocket(s) close(s)


typedef struct _simpleServer_t {
    char *                      address;
    pthread_cond_t              cond;
    pthread_mutex_t             lock;
    ism_simpleServer_request_cb requestCB;
    ism_simpleServer_connect_cb connectCB;
    ism_simpleServer_disconnect_cb disconnectCB;
    SOCKET                      serverSocket;
    SOCKET                      clientSocket;
    int                         waitCounter;
    volatile int                isRunning;
} simpleServer_t;

static int createAdminEndpoint(simpleServer_t * server) {
    /* Unix domain socket */
    xUNUSED struct stat sbuf;
    struct sockaddr_un sockAddr;
    int rc = 0;
    SOCKET sock;
    size_t sockAddrLen;
#if 0
    int xrc = stat(server->address, &sbuf);
    if (xrc == 0 && (sbuf.st_mode & S_IFMT) == S_IFSOCK) {
        TRACE(4, "Socket file %s already exists\n", server->address);
        xrc = unlink(server->address);
        if (xrc) {
            TRACE(3, "Unable to delete socket file: %s\n", server->address);
            return ISMRC_PortInUse;
        }
    }
#endif
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sun_family = AF_UNIX;
    ism_common_strlcpy(sockAddr.sun_path, server->address, sizeof(sockAddr.sun_path));
    sockAddrLen = SUN_LEN(&sockAddr);
    sock = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    /*
     * Bind the socket
     */
    rc = bind(sock, (struct sockaddr *) &sockAddr, sockAddrLen);
    if (rc == -1) {
        int err = errno;
        char * errstr = strerror(err);
        TRACE(3, "Unable to bind admin socket: addr=%s rc=%d error=%s\n", server->address, err, errstr);
        close(sock);
        return ISMRC_EndpointSocket;
    }

    rc = listen(sock, 256);
    if (rc == -1) {
        int err = errno;
        char * errstr = strerror(err);
        TRACE(3, "Unable to listen to admin socket: addr=%s rc=%d error=%s\n", server->address, err, errstr);
        close(sock);
        return ISMRC_EndpointSocket;
    }
    server->serverSocket = sock;
    return 0;
}

/*
 * Send data to the socket.
 * We assume that any errors will be detected on the read
 */
static int sendAdminReply(int clientSocket, concat_alloc_t * buff, int corID, int rc) {
    int * pInt = (int*) (buff->buf);
    *pInt = htonl(buff->used - 12);
    pInt++;
    *pInt = htonl(corID);
    pInt++;
    *pInt = htonl(rc);
    if (send(clientSocket, buff->buf, buff->used, 0) == -1) {
        return ISMRC_NetworkError;
    }
    return ISMRC_OK;
}

/*
 * Implement the MQC Admin framing.
 * The MQC Admin frame is always a 4 byte big endian length.
 */
static int handleAdminRequest(simpleServer_t * simpleServer, char * buffer, int pos, int avail, int * used) {
    char tbuf[8192];
    concat_alloc_t output_buffer = { tbuf, sizeof(tbuf), 12, 0 };
    int buflen = avail - pos;
    int mlen = 0;
    char locale[256];
    uint8_t len;
    int corID;
    int rc;
    char * buf = buffer + pos;
    if (buflen < 8)
        return 8;
    memcpy(&mlen, buf, 4);
    mlen = ntohl(mlen);

    /* If not enough data, return needed data length */
    if (mlen + 8 > buflen) {
        return mlen + 8;
    }
    buf += 4;

    memcpy(&corID, buf, 4);
    corID = ntohl(corID);
    buf += 4;
    len = (uint8_t)buf[0];
    buf++;
    memcpy(locale, buf, len);
    locale[len] = '\0';
    buf += len;
    simpleServer->requestCB(buf, (mlen-len-1), locale, &output_buffer, &rc);
    sendAdminReply(simpleServer->clientSocket, &output_buffer, corID, rc);
    if (output_buffer.inheap)
        ism_common_freeAllocBuffer(&output_buffer);

    /* Mark used data */
    *used += (mlen + 8);
    return 0;
}

static int processAdminRequests(simpleServer_t * simpleServer) {
    int bytes;
    int buflen = 128 * 1024;
    char * recvbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,108),buflen);
    int getpos = 0;
    int putpos = 0;
    int need = 0;
    int err = 0;
    do {
        int toRead = buflen - putpos;
        if (toRead > 0) {
            bytes = recv(simpleServer->clientSocket, recvbuf + putpos, toRead, 0);
            if (bytes <= 0) {
                err = errno;
                if (err) {
                    TRACE(3, "Admin connection was closed. Error: %d(%s)\n", err, strerror(err));
                } else {
                    TRACE(3, "Admin connection was closed by imaserver\n");
                }
                break;
            }
            putpos += bytes;
        }

        /*
         * Receive any messages which are in the buffer
         */
        while (getpos < putpos) {
            int32_t used = 0;
            need = handleAdminRequest(simpleServer, recvbuf, getpos, putpos, &used);
            if (need < 0)
                break;
            if (need > 0)
                break;
            getpos += used;
        }

        if (putpos > getpos) {
            int mlen = putpos - getpos;
            memmove(recvbuf, recvbuf + getpos, mlen);
            if (need > buflen) {
                int newlen = buflen + 1024;
                while (newlen < need) {
                    newlen += 10234;
                }
                recvbuf = ism_common_realloc(ISM_MEM_PROBE(ism_memory_utils_misc,109),recvbuf, newlen);
                buflen = newlen;
            }
            getpos = 0;
            putpos = mlen;
        } else {
            getpos = 0;
            putpos = 0;
        }
    } while (1);

    ism_common_free(ism_memory_utils_misc,recvbuf);
    return err;
}

static void * adminThreadProc(void * parm, void * context, int value) {
    simpleServer_t * simpleServer = (simpleServer_t*) parm;
    while (simpleServer->isRunning) {
        struct sockaddr_un in_addr;
        int rc = 0;
        socklen_t in_len = sizeof(in_addr);
        int clientSocket = accept(simpleServer->serverSocket, (struct sockaddr *) &in_addr, &in_len);
        if (clientSocket <= 0) {
            int err = errno;
            TRACE(3, "Invalid socket returned from accept. Error: %d(%s) \n", err, strerror(err));
            continue;
        }
        pthread_mutex_lock(&simpleServer->lock);
        simpleServer->clientSocket = clientSocket;
        pthread_cond_broadcast(&simpleServer->cond);
        pthread_mutex_unlock(&simpleServer->lock);
        if(simpleServer->connectCB)
            simpleServer->connectCB();
        rc = processAdminRequests(simpleServer);
        close(clientSocket);
        pthread_mutex_lock(&simpleServer->lock);
        simpleServer->clientSocket = 0;
        pthread_mutex_unlock(&simpleServer->lock);
        if(simpleServer->disconnectCB)
            simpleServer->disconnectCB(rc);
    }
    pthread_mutex_lock(&simpleServer->lock);
    while (simpleServer->waitCounter) {
        pthread_mutex_unlock(&simpleServer->lock);
        ism_common_sleep(1000);
        pthread_mutex_lock(&simpleServer->lock);
    }
    pthread_mutex_unlock(&simpleServer->lock);
    pthread_cond_destroy(&simpleServer->cond);
    pthread_mutex_consistent(&simpleServer->lock);
    ism_common_free(ism_memory_utils_misc,simpleServer->address);
    ism_common_free(ism_memory_utils_misc,simpleServer);
    return NULL;
}

XAPI ism_simpleServer_t ism_common_simpleServer_start(const char * serverAddress, ism_simpleServer_request_cb requestCB,
        ism_simpleServer_connect_cb connectCB, ism_simpleServer_disconnect_cb disconnectCB) {
    simpleServer_t * simpleServer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,113),1, sizeof(simpleServer_t));
    if (simpleServer) {
        simpleServer->address = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),serverAddress);
        if (simpleServer->address) {
            int rc = createAdminEndpoint(simpleServer);
            if (!rc) {
                ism_threadh_t adminThread;
                simpleServer->requestCB = requestCB;
                simpleServer->connectCB = connectCB;
                simpleServer->disconnectCB = disconnectCB;
                pthread_cond_init(&simpleServer->cond, NULL);
                pthread_mutex_init(&simpleServer->lock, NULL);
                simpleServer->isRunning = 1;
                ism_common_startThread(&adminThread, adminThreadProc, simpleServer, NULL, 0, ISM_TUSAGE_NORMAL, 0,
                        "simpleServer", "Simple Server connection endpoint");
                ism_common_detachThread(adminThread);
                return simpleServer;
            }
            ism_common_free(ism_memory_utils_misc,simpleServer->address);
        }
        ism_common_free(ism_memory_utils_misc,simpleServer);
    }
    return NULL;
}

XAPI void ism_common_simpleServer_stop(ism_simpleServer_t simpleServer) {
    pthread_mutex_lock(&simpleServer->lock);
    simpleServer->isRunning = 0;
    if (simpleServer->serverSocket) {
        close(simpleServer->serverSocket);
        unlink(simpleServer->address);
    }
    simpleServer->serverSocket = 0;
    pthread_cond_broadcast(&simpleServer->cond);
    pthread_mutex_unlock(&simpleServer->lock);
}

XAPI int ism_common_simpleServer_waitForConnection(ism_simpleServer_t simpleServer) {
    int rc = ISMRC_OK;
    pthread_mutex_lock(&simpleServer->lock);
    simpleServer->waitCounter++;
    while ((simpleServer->serverSocket == 0) && (simpleServer->serverSocket))
        pthread_cond_wait(&simpleServer->cond, &simpleServer->lock);
    if (simpleServer->serverSocket == 0)
        rc = ISMRC_Closed;
    simpleServer->waitCounter--;
    pthread_mutex_unlock(&simpleServer->lock);
    return rc;
}
