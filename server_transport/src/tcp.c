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
#ifndef TRACE_COMP
#define TRACE_COMP Tcp
#endif

#include <assert.h>
#include <security.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "tcp.h"
#include <dlfcn.h>
#include <log.h>
#include <sys/resource.h>
#define STR(s) XSTR(s)
#define XSTR(s) #s
#define htonll(x) __builtin_bswap64(x)
#define ntohll(x) __builtin_bswap64(x)
#define BILLION        1000000000L



extern int ism_config_startEndpointCRLTimer(const char * epname);
/*
 * Implement the tcp transport including both secure and non-secure tcp sockets.
 *
 * The transport component provides a framework in which to implement a transport.
 * The tcp transport implements the tcp specific parts of this.  tcp is a streaming
 * transport, so the tcp transport includes methods for breaking the stream into
 * packets of information.
 *
 * The tcp transport also includes the WebSockets handler and this is part of the
 * handshake and framing logic.
 *
 */


typedef struct ioProcessorThread_t * ioProcessorThread;
typedef struct ioListenerThread_t * ioListenerThread;
typedef struct ioConnectionThread_t * ioConnectionThread;
typedef struct asyncJobRequest_t {
    ism_transport_AsyncJob_t    func;
    ism_transport_t *           transport;
    void *                      param1;
    uint64_t                    param2;
    struct asyncJobRequest_t * next;
} asyncJobRequest_t;


/*
 * Define the tcp specific extension to the transport object
 */
typedef struct ism_transobj {
    ism_listener_t *      listener;            /* Endpoint object                            */
    ism_transport_t *     transport;           /* Pointer back to parent transport object    */
    ism_byteBuffer        sendBuffer;
    ism_byteBuffer        sndQueueHead;
    ism_byteBuffer        sndQueueTail;
    ism_byteBuffer        rcvBuffer;
    pthread_spinlock_t    slock;               /* Spin lock for other fields                  */
    int                   stopped;
    int                   socket;              /* Socket file descriptor                      */
    volatile uint16_t     state;
    uint8_t               outgoing;
    uint8_t               doNotBatch;
    uint8_t               sledgecount;
    ioProcessorThread     iopth;
    ioListenerThread      iolth;
    uint64_t              id;                  /* Non-wrapping ID                             */
    int                   needBytes;
    int                   secured;
    int                   isProcessing;
    int                   maxSendSize;
    struct ism_transobj * conListNext;
    struct ism_transobj * conListPrev;
    struct ism_transobj * iopNext;
    struct ism_transobj * iolNext;
    ism_timer_t           sledgetimer;
    SSL *                 ssl;                 /* SSL/TLS openSSL context                     */
    struct ssl_ctx_st *   tlsCTX;
    BIO *                 bio;                 /* openSSL basic IO context                    */
    BIO *                 bio1;
    char *                bio1DataPtr;
    asyncJobRequest_t   * asyncJobRequestsHead;
    asyncJobRequest_t   * asyncJobRequestsTail;
} ism_transobj;

typedef ism_transobj ism_connection_t;


/*
 * Define a job for an IO processor thread
 */
typedef struct ioProcJob {
    ism_connection_t *   con;
    uint64_t             events;
} ioProcJob;

/*
 * Header for job list for an IO processor thread
 */
typedef struct iopJobsList {
    ioProcJob *          jobs;
    int                  allocated;
    int                  used;
} iopJobsList ;

/*
 * Define information for an IO processor thread
 */
typedef struct ioProcessorThread_t {
    pthread_spinlock_t   lock;
    pthread_mutex_t      mutex;
    pthread_cond_t       cond;
    ism_byteBufferPool   bufferPool;
    ism_byteBuffer       recvBuffer;
    iopJobsList          jobsList[2];
    iopJobsList        * currentJobsList;
    iopJobsList        * nextJobsList;
    ism_threadh_t        thread;
    ioListenerThread     iolth;
    volatile int         isStopped;
    volatile int         initTLS;
    volatile int         connectionCounter;
} ioProcessorThread_t;

/*
 * Define a connection job for the IO processor thread
 */
typedef struct ioConnectionJob {
    struct ioConnectionJob * next;
    ism_listener_t *     listener;
    int                  socket;
    socklen_t            in_len;
    struct sockaddr_un   in_addr;
} ioConnectionJob;

/*
 * Define information for the IO listener thread
 */
typedef struct ioListenerThread_t {
    ism_threadh_t        thread;
    pthread_spinlock_t   lock;
    int                  efd;              /* The epoll file descriptor    */
    int                  pipe_wfd;         /* Pipe for sending requests and shutdown */
    ism_connection_t *   pendingRequests;
    ioConnectionJob *    connectionJobs;
} ioListenerThread_t;

/*
 * Define information for the connection thread
 */
typedef struct ioConnectionThread_t {
    ism_threadh_t  thread;
    int            efd;
    int            pipe_wfd;
} ioConnectionThread_t;

typedef struct epoll_event epoll_event;

/*
 * Structure for each possible connection indexed by socket number
 */
typedef struct socketInfo_t {
    uint32_t           maxSendSize;
    uint32_t           maxRecvSize;
    pthread_spinlock_t lock;
    volatile uint8_t   inUse;
    volatile uint8_t   sndBufAtMax;
    volatile uint8_t   rcvBufAtMax;
    uint8_t            resv;
} socketInfo_t;

static int inet_convert(const char * src, uint32_t * dst);

/*
 * Global variables
 */
static int sendSize;
static int recvSize;
static int iopDelay;
static int tobjFromPool;
static int disableMonitoring;
static socketInfo_t   * socketsInfo = NULL;
static int              maxSocketId = 0;
static int              allocSocketId = 0;
#define ACTIVE_CONNECTION_MAX_DEFAULT 2000000
static pthread_mutex_t conMutex = PTHREAD_MUTEX_INITIALIZER;
static ism_connection_t * activeConnections = NULL;
static ism_connection_t * closedConnections = NULL;
static uint64_t conCounter = 0;
static int      activeConnectionsMax;
static int      activeConnectionsCounter = 0;
static int      useSpinLocks = 0;

static int      tcpMaxCon;
static ioConnectionThread  conListener = NULL;
static ioListenerThread    ioListener = NULL;
static ioProcessorThread * ioProcessors = NULL;
static ioProcessorThread   adminIoProcessor = NULL;
static int                 numOfIOProcs = 0;
static ism_delivery_t *    delivery;              /* The delivery object                  */
static volatile int        g_stopped = 1;
static uint64_t            maxPoolSizeBytes;
static ism_timer_t         cleanup_timer = NULL;
static ism_timer_t         ddos_timer = NULL;
static ism_timer_t         chkRcvBuffTimer = NULL;
static int                 g_conciseLog = 0;
static int                 g_ctxPerThread = 0;
static uint32_t            g_nolog_list [64];
static uint32_t            g_nolog_count = 0;

/*
 * Forward declarations
 */
HOT static int sendBytes(ism_transport_t * transp, char * buf, int len, int protval, int flags);
static int createTlsObjects(ism_transport_t * transport, const char * data, int datalen);
static int addConnectionJob(ioListenerThread_t * iolth, ioConnectionJob * conJob);
int ism_transport_setNoLog(const char * nolog);
int ism_transport_getPSKCount(void);
void ism_common_traceSSLError(const char * errMsg, const char * file, int line);
void ism_common_logSSLError(const char * errmsg, const char * file, int line);

/*
 * Return the number of IOPs
 */
int ism_tcp_getIOPcount(void) {
    return numOfIOProcs;
}

/*
 * Reset the socket info
 */
static void resetSocketInfo(int id, int inUse, uint32_t maxSendSize, uint32_t maxRecvSize){
    pthread_spin_lock(&socketsInfo[id].lock);
    socketsInfo[id].inUse = inUse;
    socketsInfo[id].rcvBufAtMax = 0;
    socketsInfo[id].sndBufAtMax = 0;
    socketsInfo[id].maxRecvSize = maxRecvSize;
    socketsInfo[id].maxSendSize = maxSendSize;
    pthread_spin_unlock(&socketsInfo[id].lock);
}

XAPI int ism_transport_setMaxSocketBufSize(ism_transport_t * transport, int maxSendSize, int maxRecvSize) {
    /* If this is a TCP transport object */
    if (transport->virtualSid == 0 && transport->tobj && transport->tobj->transport == transport) {
        int sock = transport->tobj->socket;
        int wasset = 0;
        if (sock != 0) {
            pthread_spin_lock(&socketsInfo[sock].lock);
            if(socketsInfo[sock].inUse) {
                socketsInfo[sock].rcvBufAtMax = 0;
                socketsInfo[sock].sndBufAtMax = 0;
                socketsInfo[sock].maxRecvSize = maxRecvSize;
                socketsInfo[sock].maxSendSize = maxSendSize;
                wasset = 1;
            }
            pthread_spin_unlock(&socketsInfo[sock].lock);
            if (wasset)
                TRACE(8, "Set TCP buffer max size: send=%dK recv=%dK\n", maxSendSize/1024, maxRecvSize/1024);
        }
    }
    return 0;
}

/*
 * Create and set the server socket
 */
static int createSocket(const char * ipAddr, int port, const char * endpoint) {
    struct addrinfo hints = {0};
    struct addrinfo *result, *rp;
    struct addrinfo unixinfo = {0};
    int rc;
    int fd = -1;
    int32_t flags;
    char portstr[10];
    char ipstr[INET6_ADDRSTRLEN];



    if (ipAddr && (strcmpi(ipAddr, "All") == 0)) {
        ipAddr = NULL;
    }

    /*
     * Unix domain socket
     */
    if (ipAddr && *ipAddr == '/') {
        struct sockaddr_un sockunix;
        struct stat sbuf;

        int xrc = stat(ipAddr, &sbuf);
        if (xrc == 0 && (sbuf.st_mode & S_IFMT) == S_IFSOCK) {
            xrc = unlink(ipAddr);
            if (xrc) {
                TRACE(3, "Unable to delete socket file: %s\n", ipAddr);
            }
        }
        unixinfo = hints;
        unixinfo.ai_family = AF_UNIX;
        unixinfo.ai_socktype = SOCK_STREAM;
        unixinfo.ai_protocol = 0;
        sockunix.sun_family = AF_UNIX;
        ism_common_strlcpy(sockunix.sun_path, ipAddr, sizeof (sockunix.sun_path));
        unixinfo.ai_addrlen = SUN_LEN(&sockunix);
        unixinfo.ai_addr = (struct sockaddr *)&sockunix;
        result = &unixinfo;
    }

    /*
     * Normal TCP socket
     */
    else {
        hints.ai_family = AF_INET6;                 /* Return IPv6 choices */
        hints.ai_socktype = SOCK_STREAM;            /* We want a TCP socket */
        hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;  /* All interfaces, with IPv4 mapped to IPv6 */

        sprintf(portstr, "%d", port);

        /*
         * Allow the IP address to have a surrounding brackets.
         * This is normally used for IPv6 but we actually allow it for anything.
         */
        if (ipAddr && *ipAddr == '[') {
            int  iplen = strlen(ipAddr);
            if (iplen > 1) {
                char * newip = alloca(iplen);
                strcpy(newip, ipAddr+1);
                if (newip[iplen-2] == ']')
                    newip[iplen-2] = 0;
                ipAddr = newip;
            }
        }

        /*
         * Check for a valid IP address
         */
        rc = getaddrinfo(ipAddr, portstr, &hints, &result);
        if (rc != 0) {
            ism_common_setError(ISMRC_IPNotValid);
            return -1;
        }
    }


    /*
     * Find one which we can open a socket on.
     */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC | SOCK_NONBLOCK, rp->ai_protocol);
        if (fd == -1) {
            ism_common_setError(ISMRC_EndpointSocket);
            continue;
        }

        /*
         * Set the socket reusable
         */
        flags = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &flags, sizeof flags);

        /*
         * Bind the socket
         */
        rc = bind(fd, rp->ai_addr, rp->ai_addrlen);
        if (rc) {
            rc = errno;
            if (rc == EINVAL || rc == EADDRINUSE)
                ism_common_setError(ISMRC_PortInUse);
            else
                ism_common_setError(ISMRC_EndpointSocket);

            TRACE(3, "Unable to bind socket: endpoint=%s port=%d rc=%d error=%s\n", endpoint, port, rc, strerror(rc));
            close(fd);
            fd = -1;
            continue;
        }

        if (rp->ai_family == AF_UNIX) {
            TRACE(6, "TCP socket created for endpoint %s, bound to %s\n", endpoint, ipAddr);
        } else {
            inet_ntop(rp->ai_family, (void*) &(((struct sockaddr_in *) rp->ai_addr)->sin_addr), ipstr, sizeof ipstr);
            TRACE(6, "TCP socket created for endpoint %s, bound to [%s]:%d\n", endpoint, ipstr, port);
        }

        /*
         * Listen on the port
         */
        rc = listen(fd, tcpMaxCon);
        if (rc == -1) {
            int save_errno = errno;
            ism_common_setError(ISMRC_EndpointSocket);
            TRACE(5, "Failure in socket listen: endpoint=%s port=%d error=%s errno=%d\n",
                endpoint, port, strerror(save_errno), save_errno);
            close(fd);
            fd = -1;
            errno = save_errno;
            continue;
        }
        break;
    }

    if (result != &unixinfo)
        freeaddrinfo(result);

    return fd;
}

/*
 * Accept a new connection.
 * This is called within the connection thread to request the connection be done
 * in an IO processing thread.
 */
static int acceptNewConnection(ism_listener_t * endpoint){
    struct sockaddr_un in_addr;
    socklen_t in_len;
    int n1;
    int ifd;
    ioConnectionJob * conJob = NULL;

    if (!endpoint->enabled)
        return 0;
    TRACE(9, "Accept new TCP connection: endpoint=%s addr=%p\n", endpoint->name, endpoint);
    /*
     * Accept a new incoming connection
     */
    in_len = sizeof(in_addr);
    ifd = accept4(endpoint->sock, (struct sockaddr *) &in_addr, &in_len, SOCK_CLOEXEC | SOCK_NONBLOCK);
    if (ifd <= 0) {
        int err = errno;
        /*
         * If there error is anything other than try again, it is a serious error and we log it
         */
        if (err != EAGAIN) {
            LOG(ERROR, Transport, 1120, "%-s%-s%u", "Closing TCP connection due to accept failure. Endpoint={0} Error={1} RC={2}.",
                 endpoint->name, strerror(err), err);
            __sync_add_and_fetch(&endpoint->stats->bad_connect_count, 1);
        }
        return 0;
    }

    /*
     * If there are too many active connections and this is not an internal connection, reject it.
     */
    n1 = __sync_add_and_fetch(&activeConnectionsCounter, 1);
    if (n1 > activeConnectionsMax) {
        /* Allow admin connections anyway */
        if (!(endpoint->protomask & PMASK_Admin)) {
            char * ipbuf = alloca(256);
            *ipbuf = 0;
            getnameinfo((struct sockaddr *) &in_addr, sizeof(in_addr), ipbuf, 256, NULL, 0, NI_NUMERICHOST);
            if (strlen(ipbuf) >=  7 && !memcmp(ipbuf, "::ffff:", 7))        /* BEAM suppression: uninitialized */
                ipbuf += 7;
            LOG(ERROR, Transport, 1119, "%-s%s", "Closing TCP connection because there are too many active connections. Endpoint={0} From={1}.",
                   endpoint->name, ipbuf);
            __sync_sub_and_fetch(&activeConnectionsCounter, 1);        /* Not an active connection */
            __sync_add_and_fetch(&endpoint->stats->bad_connect_count, 1);     /* Increment endpoint bad connection count */
            close(ifd);
            return 0;
        }
    }

    /*
     * Schedule a job to complete the connection
     */
    conJob = ism_common_malloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 8),sizeof(ioConnectionJob));
    memcpy(&conJob->in_addr, &in_addr, in_len);
    conJob->in_len = in_len;
    conJob->listener = endpoint;
    conJob->socket = ifd;
    addConnectionJob(ioListener, conJob);
    return ifd;
}


/*
 * Increases socket buffer size
 * @param sock - socket
 * @param bufType - buffer type to increase (i.e. SO_SNDBUF or SO_RCVBUF)
 */
static int increaseSockBufSize(int sock, int bufType) {
    int err;
    socketInfo_t *pSI = &socketsInfo[sock];
    pthread_spin_lock(&pSI->lock);
    if (UNLIKELY(pSI->inUse)) {
        int currSize = 0;
        int newSize = 0;
        socklen_t optlen = sizeof(currSize);
        volatile uint8_t * pMax;
        if (bufType == SO_SNDBUF) {
            if(pSI->sndBufAtMax) {
                pthread_spin_unlock(&pSI->lock);
                return 0;
            }
            if (getsockopt(sock,SOL_SOCKET,SO_SNDBUF, &currSize,&optlen)) {
                    err = errno;
                    pSI->sndBufAtMax = 1;
                    pthread_spin_unlock(&pSI->lock);
                    TRACE(6, "increaseSockBufSize(%d, %s): getsockopt failed with error %d(%s)\n", sock,"SO_SNDBUF", err, strerror(err));
                    return 0;
            }
            if((pSI->maxSendSize != 0) && (currSize >= pSI->maxSendSize)) {
                pSI->sndBufAtMax = 1;
                pthread_spin_unlock(&pSI->lock);
                TRACE(6, "increaseSockBufSize(%d, %s): buffer size is already at maximum (%u)\n", sock,"SO_SNDBUF", pSI->maxSendSize);
                return 0;
            }
            pMax = &pSI->sndBufAtMax;
        } else {
            int inUse = 0;
            if (pSI->rcvBufAtMax) {
                pthread_spin_unlock(&pSI->lock);
                return 0;
            }
            if((pSI->maxRecvSize != 0) && (currSize >= pSI->maxRecvSize)) {
                pSI->rcvBufAtMax = 1;
                pthread_spin_unlock(&pSI->lock);
                TRACE(6, "increaseSockBufSize(%d, %s): buffer size is already at maximum (%u)\n", sock,"SO_RCVBUF", pSI->maxRecvSize);
                return 0;
            }
            if(getsockopt(sock,SOL_SOCKET,SO_RCVBUF, &currSize,&optlen) || ioctl(sock, SIOCINQ, &inUse)) {
                 err = errno;
                 pSI->rcvBufAtMax = 1;
                 pthread_spin_unlock(&pSI->lock);
                 TRACE(6, "increaseSockBufSize(%d, %s): get socket info failed with error %d(%s)\n", sock,"SO_RCVBUF", err, strerror(err));
                 return 0;
            }
            if (inUse < (currSize*0.9)) {
                pthread_spin_unlock(&pSI->lock);
                return 1;
            }
            pMax = &pSI->rcvBufAtMax;
        }
        newSize = currSize*2;
        /* Set to currSize since linux doubles the value on get */
        if (setsockopt(sock,SOL_SOCKET,bufType, &currSize,optlen)) {
            err = errno;
            *pMax = 1;
            pthread_spin_unlock(&pSI->lock);
            TRACE(6, "increaseSockBufSize(%d, %s): setsockopt failed with error %d(%s)\n", sock,((bufType == SO_SNDBUF) ? "SO_SNDBUF" : "SO_RCVBUF"), err, strerror(err));
            return 0;
        }
        if (getsockopt(sock,SOL_SOCKET,bufType, &currSize,&optlen)) {
            err = errno;
            *pMax = 1;
            pthread_spin_unlock(&pSI->lock);
            TRACE(6, "increaseSockBufSize(%d, %s): getsockopt failed with error %d(%s)\n", sock,((bufType == SO_SNDBUF) ? "SO_SNDBUF" : "SO_RCVBUF"), err, strerror(err));
            return 0;
        }
        if (currSize < newSize) {
            *pMax = 1;
            pthread_spin_unlock(&pSI->lock);
            TRACE(6, "increaseSockBufSize(%d, %s): buffer size value is less than requested %d < %d\n", sock,((bufType == SO_SNDBUF) ? "SO_SNDBUF" : "SO_RCVBUF"), currSize, newSize);
            return 0;
        }
    }
    pthread_spin_unlock(&pSI->lock);
    return 0;
}


#undef TRACE_DOMAIN
#define TRACE_DOMAIN transport->trclevel

/*
 * Include the core framing support including the JMS and MQTT framing support
 */
#include "frame.c"

/*
 * Special jobs for cleanup and shutdown
 */
#define SHUTDOWN_REQUEST 0x0000000100000000
#define CLEANUP_REQUEST  0x0000000200000000
#define SHUTDOWN_FORCE   0x0000000400000000


/*
 * Timer task to periodically check if receive buffer is full and increase it if possible
 */
static int conRcvBufCheckTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    int sock;
    int maxsocket;

    if (socketsInfo && !g_stopped){
        pthread_mutex_lock(&conMutex);
        maxsocket = maxSocketId;
        pthread_mutex_unlock(&conMutex);
        for (sock = 0; sock < maxsocket; sock++){
            if ( socketsInfo[sock].inUse && (socketsInfo[sock].rcvBufAtMax==0)) {
                increaseSockBufSize(sock,SO_RCVBUF);
            }
        }
    }
    return 1;
}

/*
 * Enqueue an IO job
 */
HOT static void addJob4Processing(ism_connection_t * con, uint64_t events) {
    ioProcessorThread_t* ioth = con->iopth;
    if (useSpinLocks)
        pthread_spin_lock(&ioth->lock);
    else
        pthread_mutex_lock(&ioth->mutex);
    iopJobsList * jobsList = ioth->currentJobsList;
    if (UNLIKELY(jobsList->used == jobsList->allocated)) {
        jobsList->allocated *= 2;
        jobsList->jobs = ism_common_realloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 28), jobsList->jobs, jobsList->allocated * sizeof(ioProcJob));
        if (jobsList->jobs == NULL) {
            ism_common_shutdown(1);
            return; /* Unreachable */
        }
    }
    ioProcJob * job = jobsList->jobs + jobsList->used;
    job->con = con;
    job->events = events;
    jobsList->used++;
    if (useSpinLocks)
        pthread_spin_unlock(&ioth->lock);
    else {
        int sendSignal = ((jobsList->used == 1) ? 1 : 0);
        pthread_mutex_unlock(&ioth->mutex);
        if(sendSignal)
            pthread_cond_signal(&ioth->cond);
    }
}

/*
 * Add a connection to the IO processing thread
 */
static int addConnectionToIOThread(ism_connection_t * con) {
    epoll_event event = { 0 };
    ioListenerThread_t * iolth = con->iopth->iolth;
    con->iolth = iolth;
    event.data.ptr = con;
    event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    if (!con->outgoing) {
        con->state = (con->secured ? ISM_TRANSPORT_HANDSHAKE_IN_PROCESS : (ISM_TRANSPORT_CONNECTED | ISM_TRANSPORT_CAN_READ
                | ISM_TRANSPORT_CAN_WRITE));
    }
    con->isProcessing = 0;
    if (epoll_ctl(ioListener->efd, EPOLL_CTL_ADD, con->socket, &event) == -1) {
        ism_transport_t * transport = con->transport;
        TRACE(3, "Unable to add socket to epoll: errno=%d connect=%u endpoint=%s", errno, transport->index, transport->endpoint_name);
        ism_common_setError(ISMRC_EndpointSocket);
        return -1;
    }
    __sync_add_and_fetch(&con->iopth->connectionCounter, 1);
    return 0;
}


/*
 * Remove a transport from an IO precessing thread.
 * This is done by submitting a job to the IO processing thread.
 */
static int removeTransportFromIOThread(ism_connection_t * con) {
    addJob4Processing(con, SHUTDOWN_REQUEST);
    __sync_sub_and_fetch(&con->iopth->connectionCounter, 1);
    return 0;
}

/*
 * Add a work item to a delivery thread.
 *
 * @param transport  The transport object
 * @param ondelivery The method to invoke for work
 * @param userdata   The data context for the work item
 * @return A return code: 0=good
 */
int ism_tcp_addWork(ism_transport_t * transport, ism_transport_onDelivery_t ondelivery, void * userdata) {
    return ism_transport_addDelivery(delivery, transport, ondelivery, userdata);
}


/*
 * Add a connection to a list of connections
 */
static void addConnectionToList(ism_connection_t * con) {
    if (!con->transport->addwork)
        con->transport->addwork = ism_tcp_addWork;
    pthread_mutex_lock(&conMutex);
    con->conListNext = activeConnections;
    con->conListPrev = NULL;
    if (activeConnections)
        activeConnections->conListPrev = con;
    activeConnections = con;

    /*
     * Commit 1024 entries at a time
     */
    if (con->socket+1024 > maxSocketId) {
        int  i;
        int newmax = (con->socket+2047) & ~0x3ff;
        if (newmax > allocSocketId)
            newmax = allocSocketId;
        for (i = maxSocketId; i < newmax; i++) {
            pthread_spin_init(&socketsInfo[i].lock, 0);
        }
        maxSocketId = newmax;
    }
    resetSocketInfo(con->socket,1,con->transport->listener->maxSendBufferSize,con->transport->listener->maxRecvBufferSize);
    pthread_mutex_unlock(&conMutex);
}

/*
 * Remove a connection from a list of connections
 */
static void removeConnectionFromList(ism_connection_t * con) {
    pthread_mutex_lock(&conMutex);
    if (con->transport->state != ISM_TRANST_Closed) {
        if (con->conListPrev) {
            con->conListPrev->conListNext = con->conListNext;
        } else {
            activeConnections = con->conListNext;
        }
        if (con->conListNext) {
            con->conListNext->conListPrev = con->conListPrev;
        }
        con->conListNext = closedConnections;
        if (closedConnections) {
            closedConnections->conListPrev = con;
        }
        con->conListPrev = NULL;
        closedConnections = con;
        con->state = ISM_TRANSPORT_DISCONNECTED;
        con->transport->state = ISM_TRANST_Closed;
    }
    pthread_mutex_unlock(&conMutex);
}

/*
 * Start the close of a connections
 */
static int closed_callback(ism_transport_t * transport) {
    TRACE(8, "TCP closed callback: connect=%u name=%s transport=%p\n", transport->index, transport->name, transport);
    if (transport->tobj->transport == NULL) {
        TRACE(8, "close connection which is only partially created: connect=%u\n", transport->index);
        ism_transport_freeTransport(transport);
        return 1;
    }
    return removeTransportFromIOThread(transport->tobj);
}

extern ism_logWriter_t * g_logwriter[LOGGER_Max+1];

/*
 * Check if we should log.
 * This is only implemented for IPv4 addresses
 */
static int noLog(const char * client) {
    uint32_t ipaddr;
    int      i;

    /* Only allow if client specified and IPv4 */
    if (!client || !*client || *client=='[' || g_nolog_count == 0)
        return 0;
    if (inet_convert(client, &ipaddr))
        return 0;
    for (i=0; i<g_nolog_count; i++) {
        if (ipaddr >= g_nolog_list[i*2] &&
            ipaddr <= g_nolog_list[i*2+1]) {
            return 1;
        }
    }
    return 0;
}


/*
 * Do not log based on client address or proxy
 * @param transport A transport object
 * @return 0=log, 1=do not log
 */
XAPI int ism_transport_noLog(ism_transport_t * transport) {
    return transport->nolog || transport->listener->isAdmin || noLog(transport->client_addr);
}


/*
 * Function to notify the protocol that a connection is closing
 */
static int close_callback(ism_transport_t * transport, int rc, int clean, const char * reason) {
    uint64_t active;
    /*
     * Make sure that the close callback is only processed once.  The first one wins.
     */
    if (!transport)
        return 1;

    // printf("close %s state=%d family=%s\n", transport->name, transport->state, transport->protocol_family);
    if (!(__sync_bool_compare_and_swap(&transport->state, ISM_TRANST_Open, ISM_TRANST_Closing)
            || __sync_bool_compare_and_swap(&transport->state, ISM_TRANST_Opening, ISM_TRANST_Closing))) {
        TRACE(6, "The connection cannot close due to state: connect=%u name=%s state=%u\n",
                transport->index, transport->name, transport->state);
        return 1;
    }

    if (!reason)
        reason = "";

    TRACE(7, "TCP close_callback: connect=%u name=%s reason=%s\n", transport->index, transport->name, reason);

    /*
     * Put out a trace that we are closing
     */
    uint32_t uptime = (uint32_t)(((ism_common_currentTimeNanos()-transport->connect_time)+500000000)/1000000000);  /* in seconds */
    if (!transport->protocol_family || !*transport->protocol_family) {
        if (!noLog(transport->client_addr)) {
            char from[64];
            sprintf(from,"[%s]:%u",transport->client_addr, transport->clientport);
            /* We have not completed the open */
            LOG(WARN, Connection, 1116, "%u%-s%d%d%-s%u%llu%llu%s",
                    "Closing TCP connection during handshake: ConnectionID={0} From={8} Endpoint={1} RC={2} Reason={4} "
                    "Uptime={5} ReadBytes={6} WriteBytes={7}.",
                    transport->index, transport->listener->name, rc, clean, reason, uptime,
                    (ULL)transport->read_bytes, (ULL)transport->write_bytes, from);
        } else {
            TRACE(6, "Close TCP connection during handshake (CWLNAS1116) ConnectionID=%u From=%s:%u Endpoint=%s RC=%u Reason=%s "
                    "Uptime=%u ReadBytes=%llu WriteBytes=%llu\n",
                    transport->index, transport->client_addr, transport->clientport, transport->endpoint_name,
                    rc, reason, uptime, (ULL)transport->read_bytes, (ULL)transport->write_bytes);
        }
        __sync_add_and_fetch(&transport->listener->stats->bad_connect_count, 1);
    } else {
        if (transport->listener) {
            int waslogged = 0;
            if (!ism_transport_noLog(transport)) {
                if (!clean || (g_logwriter[LOGGER_Connection] && TRACE_DOMAIN->logLevel[LOGGER_Connection] != AuxLogSetting_Min)) {
                    if (transport->originated) {
                        LOG(NOTICE, Connection, 1121, "%u%-s%s%-s%-s%u%d%d%-s%llu%llu%llu%llu%llu%llu",
                            "Closing TCP outgoing connection: ConnectionID={0} ClientID={1} Protocol={2} "
                            "Endpoint={3} UserID={4} Uptime={5} RC={6} Clean={7} Reason={8} "
                            "ReadBytes={9} ReadMsg={10} WriteBytes={11} WriteMsg={12} LostMsg={13} WarnMsg={14}.",
                            transport->index, transport->name, transport->protocol, transport->endpoint_name,
                            transport->userid ? transport->userid : "", uptime,
                            rc, clean, reason,
                            (ULL)transport->read_bytes, (ULL)transport->read_msg,
                            (ULL)transport->write_bytes,(ULL)transport->write_msg,
                            (ULL)transport->lost_msg,(ULL)transport->warn_msg);
                        waslogged = 1;
                    } else {
                        char from[64];
                        sprintf(from,"[%s]:%u",transport->client_addr, transport->clientport);
                        if (g_conciseLog) {
                            char xbuf[64];
                            reason = ism_common_getErrorName(rc, xbuf, sizeof xbuf);
                            if (reason == NULL)
                                reason = "OK";
                            else if (*reason != 'I')
                                reason = "Unknown";
                            else
                                reason += 6;

                            LOG(NOTICE, Connection, 1111, "%u%-s%s%-s%-s%u%d%d%s%llu%llu%llu%llu%llu%llu%llu%s",
                                "Closing: I={0} C={1} P={2} E={3} F={16} U={4} T={5} R={6}:{8} S={9},{10},{11},{12},{13}",
                                transport->index, transport->name, transport->protocol, transport->endpoint_name,
                                transport->userid ? transport->userid : "", uptime,
                                rc, clean, reason,
                                (ULL)transport->read_bytes, (ULL)transport->read_msg,
                                (ULL)transport->write_bytes,(ULL)transport->write_msg,
                                (ULL)transport->lost_msg, (ULL)transport->monitor_id,
                                (ULL)transport->warn_msg, from);
                            waslogged = 1;
                        } else {
                            LOG(NOTICE, Connection, 1111, "%u%-s%s%-s%-s%u%d%d%-s%llu%llu%llu%llu%llu%u%llu%s",
                                "Closing TCP connection: ConnectionID={0} MonitorID={14} ClientID={1} Protocol={2} Endpoint={3} From={16} UserID={4} Uptime={5} RC={6} Reason={8} ReadBytes={9} ReadMsg={10} WriteBytes={11} WriteMsg={12} LostMsg={13} WarnMsg={15}.",
                                transport->index, transport->name, transport->protocol, transport->endpoint_name,
                                transport->userid ? transport->userid : "", uptime,
                                rc, clean, reason,
                                (ULL)transport->read_bytes, (ULL)transport->read_msg,
                                (ULL)transport->write_bytes,(ULL)transport->write_msg,
                                (ULL)transport->lost_msg, transport->monitor_id,
                                (ULL)transport->warn_msg, from);
                            waslogged = 1;
                        }
                    }
                }
            }
            if (!waslogged) {
                TRACE(6, "Closing TCP connection %s (CWLNA1111): ConnectionID=%u MonitorID=%u ClientID=\"%s\" Protocol=%s Endpoint=\"%s\""
                        " From=%s:%u UserID=\"%s\" Uptime=%u RC=%d Reason\"%s\""
                        " ReadBytes=%llu ReadMsg=%llu WriteBytes=%llu WriteMsg=%llu LostMsg=%llu WarnMsg=%llu\n",
                    transport->originated ? "out" : "",
                    transport->index, transport->monitor_id, transport->name, transport->protocol, transport->endpoint_name,
                    transport->client_addr, transport->clientport, transport->userid ? transport->userid : "",
                    uptime, rc, reason,
                    (ULL)transport->read_bytes, (ULL)transport->read_msg,
                    (ULL)transport->write_bytes, (ULL)transport->write_msg,
                    (ULL)transport->lost_msg, (ULL)transport->warn_msg);
            }
            active = __sync_add_and_fetch(&transport->listener->stats->connect_active, -1);
            TRACE(9, "Decrement count for connections: connect=%u name=%s count=%lu active=%lu\n",
                    transport->index, transport->name, transport->listener->stats->connect_count, active);
        }
    }

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
 * Enqueue the connection request for an IO processing thread.
 * We use the pipe to indicate that a connection request is available
 */
static int addConnectionJob(ioListenerThread_t * iolth, ioConnectionJob * conJob) {
    char c = 'C';
    int needWrite;
    if (iolth == NULL)
        return 0;
    pthread_spin_lock(&iolth->lock);
    needWrite = (iolth->connectionJobs == NULL);
    conJob->next = iolth->connectionJobs;
    iolth->connectionJobs = conJob;
    pthread_spin_unlock(&iolth->lock);
    if (needWrite)
        return write(iolth->pipe_wfd, &c, 1);
    else
        return 1;
}


/*
 * Put an IP string in the transport object.
 * Convert this to either an IPv4 string, or an IPv6 string in brackets.
 */
static const char * putIPString(ism_transport_t * transport, const char * ip) {
    const char * pos;
    char *       ret;
    if(ip[0] == '/'){
        ret = (char *)ism_transport_putString(transport, ip);
    } else {
        int len = strlen(ip);
        if (len > 7 && !memcmp(ip, "::ffff:", 7) && !strchr(ip+7, ':')) {
            ret = (char *)ism_transport_putString(transport, ip+7);
        } else {
            pos = strchr(ip, ':');
            if (pos) {
                len = strcspn(ip, "%");
                ret = ism_transport_allocBytes(transport, len+3, 0);
                ret[0] = '[';
                memcpy(ret+1, ip, len);
                ret[len+1] = ']';
                ret[len+2] = 0;
            } else {
                ret = (char *)ism_transport_putString(transport, ip+7);
            }
        }
    }
    return (const char *)ret;
}

/*
 * Names for TLS errors
 */
static const char * SSL_ERRORS[9] = {
    "SSL_ERROR_NONE",
    "SSL_ERROR_SSL",
    "SSL_ERROR_WANT_READ",
    "SSL_ERROR_WANT_WRITE",
    "SSL_ERROR_WANT_X509_LOOKUP",
    "SSL_ERROR_SYSCALL",
    "SSL_ERROR_ZERO_RETURN",
    "SSL_ERROR_WANT_CONNECT",
    "SSL_ERROR_WANT_ACCEPT"
};


/*
 * Trace any errors from openssl
 */
static void sslTraceErr(ism_transport_t * transport, uint32_t  rc, const char * file, int line) {
    int          flags;
    const char * data;
    char         mbuf[1024];
    char *       pos;
    int          err = errno;

    if (rc) {
        const char * errStr = (rc < 9) ? SSL_ERRORS[rc] : "SSL_UNKNOWN_ERROR";
        if (err) {
            data = strerror_r(err, mbuf, 1024);
            traceFunction(3, TOPT_WHERE, file, line, "openssl connect=%u error(%d): %s : errno is \"%s\"(%d)\n",
                    transport->index, rc, errStr, data, err);
        } else {
            traceFunction(3, TOPT_WHERE, file, line, "openssl connect=%u error(%d): %s\n",
                    transport->index, rc, errStr);
        }
    }
    for (;;) {
        rc = (uint32_t)ERR_get_error_line_data(&file, &line, &data, &flags);
        if (rc == 0)
            break;
        ERR_error_string_n(rc, mbuf, sizeof mbuf);
        pos = strchr(mbuf, ':');
        if (!pos)
            pos = mbuf;
        else
            pos++;
        if (flags&ERR_TXT_STRING) {
            traceFunction(3, TOPT_WHERE, file, line, "openssl connect=%u error(%d): %s: data=\"%s\"\n",
                    transport->index, rc, pos, data);
        } else {
            traceFunction(3, TOPT_WHERE, file, line, "openssl connect=%u error(%d): %s\n",
                    transport->index, rc, pos);
        }
    }
}

/*
 * Create the client TLS objects
 */
int makeTlsClientObjects(ism_transport_t * transport) {
    SSL * ssl = NULL;
    BIO * bio = NULL;
    ism_connection_t * con = transport->tobj;

    ssl = SSL_new(con->tlsCTX);
    if (!ssl) {
        sslTraceErr(transport, 0, __FILE__, __LINE__);
    } else {
        SSL_set_connect_state(ssl);
        /*
         * Create the BIO objects
         */
        bio = BIO_new_socket(con->socket, BIO_NOCLOSE);
        if (!bio) {
            sslTraceErr(transport, 0, __FILE__, __LINE__);
        }
        if (!ssl || !bio) {
            if (ssl)
                SSL_free(ssl);
            ism_common_setError(ISMRC_NetworkError);
            transport->close(transport, ISMRC_NetworkError, 0, "Unable to create TLS client objects");
            return -1;
        }
        SSL_set_bio(ssl, bio, bio);
        con->bio = bio;
        con->secured = 1;
        transport->secure = 1;
        SSL_set_app_data(ssl, (char *)transport);
        con->ssl = ssl;
        transport->ssl = ssl;
        SSL_set_info_callback(ssl, ism_common_sslInfoCallback);
        SSL_set_msg_callback(ssl, ism_common_sslProtocolDebugCallback);
        SSL_set_msg_callback_arg(ssl, transport);
    }
    return 0;
}

/*
 * Create the TLS objects.
 *
 * As we already have one buffer from the client, we create a memory BIO to send this
 * to openSSL.  Otherwise we can just use the socket BIO.  Once we run out of the bytes
 * in this buffer, we revert to using the4 socket BIO.
 */
static int createTlsObjects(ism_transport_t * transport, const char * data, int datalen) {
    SSL * ssl = NULL;
    BIO * bio = NULL;
    BIO * bio1 = NULL;
    char * bio1DataPtr = NULL;
    ism_connection_t * con = transport->tobj;
    xUNUSED int rc;

    if (transport->listener->sslCTX == NULL)
        return -1;
    /*
     * Create the SSL object
     */
    ssl = SSL_new(transport->tid >= transport->listener->sslctx_count ? transport->listener->sslCTX[0] :
                  transport->listener->sslCTX[transport->tid]);
    if (!ssl) {
        sslTraceErr(transport, 0, __FILE__, __LINE__);
    } else {
        SSL_set_accept_state(ssl);
        /*
         * Create the BIO objects
         */
        bio = BIO_new_socket(con->socket, BIO_NOCLOSE);
        if (!bio) {
            sslTraceErr(transport, 0, __FILE__, __LINE__);
        } else {
            if (datalen) {
                bio1DataPtr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_tls, 1),datalen);
                if (bio1DataPtr) {
                    memcpy(bio1DataPtr,data,datalen);
                    bio1 = BIO_new_mem_buf((void *)bio1DataPtr, datalen);
                    if(bio1) {
                        BIO_set_mem_eof_return(bio1, -1);
                        rc = BIO_set_close(bio1, BIO_CLOSE);
                    }
                }
                if (!bio1) {
                    sslTraceErr(transport, 0, __FILE__, __LINE__);
                    BIO_free(bio);
                    if(bio1DataPtr) {
                        ism_common_free(ism_memory_tls,bio1DataPtr);
                        bio1DataPtr = NULL;
                    }
                    bio = NULL;
                }
            }
        }
    }

    if (!ssl || !bio) {
        if (ssl)
            SSL_free(ssl);
        close(con->socket);
        __sync_sub_and_fetch(&activeConnectionsCounter, 1);        /* Not an active connection */
        __sync_add_and_fetch(&transport->listener->stats->bad_connect_count, 1);     /* Increment endpoint bad connection count */
        return -1;
    }

    /*
     * Connect the memory BIO to the SSL object
     */
    if (datalen)
        SSL_set_bio(ssl, bio1, bio);       /* Set to allow us to inspect the indirect IO for now */
    else
        SSL_set_bio(ssl, bio, bio);
    con->bio = bio;
    con->bio1 = bio1;
    con->bio1DataPtr = bio1DataPtr;
    con->secured = 1;
    transport->secure = 1;
    SSL_set_app_data(ssl, (char *)transport);
    con->ssl = ssl;
    transport->ssl = ssl;
    SSL_set_info_callback(ssl, ism_common_sslInfoCallback);
    SSL_set_msg_callback(ssl, ism_common_sslProtocolDebugCallback);
    SSL_set_msg_callback_arg(ssl, transport);
    __sync_fetch_and_or(&con->state, ISM_TRANSPORT_HANDSHAKE_IN_PROCESS);
    return 0;
}


/*
 * Process a work item to do a connection
 */
static int processConnectionRequest(ioConnectionJob * conJob) {
    struct sockaddr_un in_addr;
    socklen_t in_len;
    int sock = conJob->socket;
    ism_transport_t * transport;
    ism_connection_t * con;
    char ipbuf[64];
    int flags, rc;

#undef TRACE_DOMAIN
#define TRACE_DOMAIN ism_defaultTrace

    if (conJob->listener->enabled == 0)
        return 0;

    if (conJob->in_addr.sun_family != AF_UNIX) {
        /*
         * Set the TCP nodelay on
         */
        flags = 1;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *) &flags, sizeof(flags))) {
            LOG(WARN, Transport, 1106, "%s%s%d", "Unable to set {0} socket option: Error={1} RC={2}.", "TCP_NODELAY", strerror(errno), errno);
            close(sock);
            __sync_sub_and_fetch(&activeConnectionsCounter, 1);        /* Not an active connection */
            __sync_add_and_fetch(&conJob->listener->stats->bad_connect_count, 1);     /* Increment endpoint bad connection count */
            return -1;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *) &flags, sizeof(flags))) {
            LOG(WARN, Transport, 1106, "%s%s%d", "Unable to set {0} socket option: Error={1} RC={2}.", "SO_KEEPALIVE", strerror(errno), errno);
            close(sock);
            __sync_sub_and_fetch(&activeConnectionsCounter, 1);        /* Not an active connection */
            __sync_add_and_fetch(&conJob->listener->stats->bad_connect_count, 1);     /* Increment endpoint bad connection count */
            return -1;
        }
    }

    /*
     * Set up the transport object
     */
    int fromPool = (tobjFromPool && (!conJob->listener->isAdmin));
    transport = ism_transport_newTransport(conJob->listener, sizeof(ism_connection_t), fromPool);
    transport->frame = handshake;
    if (conJob->in_addr.sun_family == AF_UNIX) {
        transport->clientport = 0;
        transport->client_addr="127.0.0.1";
    } else {
        if (((struct sockaddr *) &conJob->in_addr)->sa_family == AF_INET6) {
            transport->clientport = htons(((struct sockaddr_in6 *)&conJob->in_addr)->sin6_port);
        } else {
            transport->clientport = htons(((struct sockaddr_in *) &conJob->in_addr)->sin_port);
        }
        getnameinfo((struct sockaddr *) &conJob->in_addr, conJob->in_len, ipbuf, sizeof(ipbuf), NULL, 0, NI_NUMERICHOST);
        transport->client_addr = putIPString(transport, ipbuf);
    }


    /* Select for trace */
    if (ism_common_traceSelectEndpoint(transport->listener->name) ||
        ism_common_traceSelectClientAddr(transport->client_addr))
        transport->trclevel = &ism_defaultDomain.selected;

    in_len = sizeof(in_addr);
    getsockname(sock, (struct sockaddr *) &in_addr, &in_len);
    getnameinfo((struct sockaddr *) &in_addr, in_len, ipbuf, sizeof(ipbuf), NULL, 0, NI_NUMERICHOST);
    transport->serverport = conJob->listener->port;
    transport->server_addr = putIPString(transport, ipbuf);


    transport->protocol = "unknown";                       /* The protocol is not yet known */
    transport->protocol_family = "";
    transport->send = sendBytes;
    transport->close = close_callback;
    transport->closed = closed_callback;
    transport->addwork = ism_tcp_addWork;

    con = transport->tobj;
    con->id = __sync_add_and_fetch(&conCounter, 1);
    transport->index = con->id;
    transport->tid = con->id % numOfIOProcs;

#undef TRACE_DOMAIN
#define TRACE_DOMAIN transport->trclevel
    /*
     * Create the security context for the transport
     * */
    rc = ism_security_create_context(ismSEC_POLICY_CONNECTION, transport, &transport->security_context);
    if (rc != ISMRC_OK) {
        TRACE(2, "Could not set security context for transport: connect=%u rc=%d\n", transport->index, rc);
        /* Fail the creation of the transport if creation of sec context failed. */
        close(sock);
        ism_transport_freeTransport(transport);
        __sync_sub_and_fetch(&activeConnectionsCounter, 1);        /* Not an active connection */
        __sync_add_and_fetch(&conJob->listener->stats->bad_connect_count, 1);     /* Increment endpoint bad connection count */
        return -1;
    }

    con->listener = conJob->listener;
    con->transport = transport;
    con->socket = sock;
    con->rcvBuffer = NULL;
    // con->rcvBuffer = ism_allocateByteBuffer(recvSize);
    con->sendBuffer = NULL;
    con->iopNext = NULL;
    con->iolNext = NULL;
    con->maxSendSize = (con->listener->maxSendSize) ? con->listener->maxSendSize : sendSize;
    con->doNotBatch = con->listener->doNotBatch;
    pthread_spin_init(&con->slock, 0);

    if (con->listener->secure == 1) {
        createTlsObjects(transport, NULL, 0);
    } else {
        con->secured = 0;
        transport->secure = 0;
    }


    /* Check and set admin IOP for admin endpoint */
    if ( conJob->listener->isAdmin ) {
        TRACE(8, "New admin connection from %s:%d was accepted on endpoint %s: connect=%u tobj=%p ssl=%p security_context=%p\n", transport->client_addr,
                transport->clientport, con->listener->name, transport->index, transport->tobj, con->ssl, transport->security_context);
        con->iopth = adminIoProcessor;
    } else {
        con->iopth = ioProcessors[transport->tid];
        TRACE(7, "New connection from %s:%d was accepted on endpoint %s: connect=%u tobj=%p ssl=%p security_context=%p\n", transport->client_addr,
                transport->clientport, con->listener->name, transport->index, transport->tobj, con->ssl, transport->security_context);
    }

    addConnectionToList(con);
    if (!disableMonitoring)
        ism_transport_addMonitor(transport);
    addConnectionToIOThread(con);
    return 0;
}

/*
 * Get the next connection counter
 */
int ism_transport_nextConnectID(void) {
    return __sync_add_and_fetch(&conCounter, 1);
}

/*
 * Stop a listener
 */
static int stopPortListening(ism_listener_t * listener) {
    if (conListener){
        epoll_ctl(conListener->efd, EPOLL_CTL_DEL, listener->sock, NULL);
    }
    if (listener->sock >= 0)
        close(listener->sock);
    return 0;
}

/*
 * Connection listener thread
 */
static void * conListenerProc(void * parm, void * context, int value) {
    ioConnectionThread thData = (ioConnectionThread) parm;
    epoll_event events[1024];
    ism_listener_t * current[1024] = { 0 };
    uint32_t nextSize = 0;
    int pipefd[2];
    int rc;
    int run = 1;
    int efd = thData->efd;

    rc = pipe2(pipefd, O_NONBLOCK | O_CLOEXEC);
    assert(rc != -1);

    memset(&events[0], 0, sizeof(epoll_event));
    events[0].data.ptr = NULL;
    events[0].data.fd = pipefd[0];
    events[0].events = EPOLLIN | EPOLLRDHUP;
    rc = epoll_ctl(efd, EPOLL_CTL_ADD, pipefd[0], &events[0]);
    assert (rc != -1);
    thData->pipe_wfd = pipefd[1];

    /*
     * Loop for connections
     */
    while (run) {
        int i;
        int count;

        for (i = 0; i < nextSize; i++) {
            int j;
            ism_listener_t * port = current[i];
            current[i] = NULL;
            if ((port == NULL) || (port->enabled == 0))
                continue;
            for (j = 0; j < 1024; j++) {
                if (acceptNewConnection(port) > 0)
                    continue;
                break;
            }
        }
        nextSize = 0;
        ism_common_backHome();
        count = epoll_wait(efd, events, 1024, -1);
        ism_common_going2work();
        if (count > 0) {
            for (i = 0; i < count; i++) {
                struct epoll_event * event = &events[i];
                if (event->data.fd != pipefd[0]) {
                    ism_listener_t * listener = event->data.ptr;
                    if (listener && (listener->isStopped == 0)) {
                        current[nextSize++] = listener;
                    }
                    continue;
                } else {
                    char c;
                    rc = read(pipefd[0], &c, 1);
                    if ((rc > 0) && (c == 'S')) {
                        run = 0;
                        break;
                    }
                    continue;
                }
            }
            continue;
        }
        if ((count == 0) || (errno == EINTR))
            continue;
        ism_common_backHome();
//        ism_common_endThread(NULL);
        return NULL;
    }
    ism_common_backHome();
    close(thData->efd);
    close(pipefd[0]);
    close(pipefd[1]);

//    ism_common_endThread(NULL);
    return NULL;
}

#define CAN_READ(STATE)         ((((STATE) & ISM_TRANSPORT_STATE_RW) && ((STATE) & ISM_TRANSPORT_CAN_WRITE)) || (((STATE) & ISM_TRANSPORT_CAN_READ) && !((STATE) & ISM_TRANSPORT_STATE_WR)))
#define CAN_WRITE(STATE)        ((((STATE) & ISM_TRANSPORT_STATE_WR) && ((STATE) & ISM_TRANSPORT_CAN_READ)) || ((STATE) & ISM_TRANSPORT_CAN_WRITE))
#define SEND_BUFFER_SIZE        128*1024

/*
 * Write data without security
 */
HOT static int writeDataTCP(ism_connection_t * con) {
    ism_byteBuffer sendBuff = con->sendBuffer;
//    assert(sendBuff->putPtr == (sendBuff->buf + sendBuff->used));
    con->state &= ~(ISM_TRANSPORT_STATE_WW);
    if (sendBuff) {
        int toWrite = sendBuff->used - (sendBuff->getPtr - sendBuff->buf);
        if (UNLIKELY(toWrite > con->maxSendSize))
            toWrite = con->maxSendSize;
        int rc = write(con->socket, sendBuff->getPtr, toWrite);
        assert(toWrite > 0);
        if (rc > 0) {
            sendBuff->getPtr += rc;
            if (LIKELY((sendBuff->getPtr - sendBuff->buf) == sendBuff->used)) {
                sendBuff->putPtr = sendBuff->buf;
                sendBuff->getPtr = sendBuff->buf;
                sendBuff->used = 0;
                ism_common_returnBuffer(sendBuff, __FILE__, __LINE__);
                con->sendBuffer = NULL;
            }
            if (!con->transport->nostats) {
                con->transport->write_bytes += rc;
                con->transport->listener->stats->count[con->transport->tid].write_bytes += rc;
            }
            return 0;
        }
        if (rc <= 0 && errno == EAGAIN) {
#if EPOLL_DEBUG
            ism_transport_t * transport = con->transport;
#endif
            if (socketsInfo[con->socket].sndBufAtMax == 0) {
                if (increaseSockBufSize(con->socket, SO_SNDBUF)){
                    return 0;
                }
            }
            con->state |= ISM_TRANSPORT_STATE_WW;
            con->state &= ~ISM_TRANSPORT_CAN_WRITE;

#if EPOLL_DEBUG
            if((transport->name[0]) && (transport->protocol_family[0] == 'm')){
                TRACE(5, "writeDataTCP: EAGAIN for: con=%p connect=%u name=%s sock=%d from %s:%u\n", con,
                        transport->index, transport->name, con->socket, transport->client_addr, transport->clientport );
            }
#endif
//            con->transport->isSuspended = 1;
            return 1;
        } else {
            /* Error should be discovered by next read */
            con->transport->closestate[3] = 9;
            con->state &= ~ISM_TRANSPORT_CAN_WRITE;
            con->state |= ISM_TRANSPORT_CAN_READ;
            return 0;
        }
    }
    return 1;
}

/*
 * Write data with security.
 */
HOT static int writeDataSSL(ism_connection_t *con) {
    ism_byteBuffer sendBuff = con->sendBuffer;
//    assert(sendBuff->putPtr == (sendBuff->buf + sendBuff->used));
    con->state &= ~(ISM_TRANSPORT_STATE_WW | ISM_TRANSPORT_STATE_WR);
    if (sendBuff) {
        int toWrite = sendBuff->used - (sendBuff->getPtr - sendBuff->buf);
        if (UNLIKELY(toWrite > con->maxSendSize))
            toWrite = con->maxSendSize;
        errno = 0;
        int rc = SSL_write(con->ssl, sendBuff->getPtr, toWrite);
        int ec = (rc > 0) ? SSL_ERROR_NONE : SSL_get_error(con->ssl, rc);
//        assert(toWrite > 0);
        switch (ec) {
        case SSL_ERROR_NONE:
            if (rc > 0) {
                sendBuff->getPtr += rc;
                if (LIKELY((sendBuff->getPtr - sendBuff->buf) == sendBuff->used)) {
                    sendBuff->putPtr = sendBuff->buf;
                    sendBuff->getPtr = sendBuff->buf;
                    sendBuff->used = 0;
                    ism_common_returnBuffer(sendBuff, __FILE__, __LINE__);
                    con->sendBuffer = NULL;
                }
                con->transport->write_bytes += rc;
                con->transport->listener->stats->count[con->transport->tid].write_bytes += rc;
            }
            return 0;
        case SSL_ERROR_WANT_READ:
            con->state |= ISM_TRANSPORT_STATE_WR;
            con->state &= ~ISM_TRANSPORT_CAN_READ;
            //            con->transport->isSuspended = 1;
            return 1;
        case SSL_ERROR_WANT_WRITE:
            if (socketsInfo[con->socket].sndBufAtMax == 0) {
                if (increaseSockBufSize(con->socket, SO_SNDBUF)){
                    return 0;
                }
            }
            con->state |= ISM_TRANSPORT_STATE_WW;
            con->state &= ~ISM_TRANSPORT_CAN_WRITE;
            //            con->transport->isSuspended = 1;
            return 1;
        case SSL_ERROR_SSL:
            ism_common_logSSLError("TLS write error", __FILE__, __LINE__);
            /* fall thru */
        default:
            //Error should be discovered by next read
            con->transport->closestate[3] = 9;
            con->state |= ISM_TRANSPORT_CAN_READ;
            return 0;
        }
    }
    return 1;
}


/*
 * Write data to a connection
 */
HOT static int writeData(ism_connection_t *con) {
    int rc = 0;
    /*
     * For ssl send buffer should remain the same if WOULD_BLOCK
     * so we don't update buffer that was not fully sent last time
     */
    if (LIKELY(con->sendBuffer == NULL)) {
        pthread_spin_lock(&con->slock);
        if (LIKELY(con->sndQueueHead != NULL)) {
            con->sendBuffer = con->sndQueueHead;
            con->sndQueueHead = con->sendBuffer->next;
            con->sendBuffer->next = NULL;
            if (con->sndQueueHead == NULL) {
                con->sndQueueTail = NULL;
            }
            con->transport->sendQueueSize--;
        }
        pthread_spin_unlock(&con->slock);
    }
    if (con->secured) {
        rc = writeDataSSL(con);
    } else {
        rc = writeDataTCP(con);
    }
    if (UNLIKELY(con->transport->suspended)) {
        if ((con->sendBuffer == NULL) && (con->sndQueueHead == NULL) && (con->transport->resume)) {
            if (__sync_bool_compare_and_swap(&con->transport->suspended, 1, 0)) {
                con->transport->resume(con->transport, (void*)-1);
#ifdef DEBUG
                TRACEL(8, con->transport->trclevel, "Connection resumed: connect=%u\n", con->transport->index );
#endif
            }
        }
    }
    return rc;
}

/*
 * Process data from the client.
 * This is called from either the TCP or SSL reads.
 */
static int processData(ism_connection_t * con, ism_byteBuffer rcvBuffer) {
    ism_transport_t * transport = con->transport;
    int dataLen = rcvBuffer->used;
    int offset = 0;

    if (dataLen <= 0)
        return 0;

    if (!transport->nostats) {
        transport->read_bytes += dataLen;
        transport->listener->stats->count[con->transport->tid].read_bytes += dataLen;
        transport->lastAccessTime = (uint64_t)ism_common_readTSC();
    }

    /*
     * We have remaining data from previous call
     */
    if (con->rcvBuffer && con->rcvBuffer->used) {
        int needlen = con->rcvBuffer->used + dataLen;
        if (con->needBytes > needlen)
            needlen = con->needBytes;        /* Make the buffer big enough to handle the needed bytes */
        if (needlen > con->rcvBuffer->allocated) {
            ism_byteBuffer tmpBuf = ism_allocateByteBuffer(needlen + 1024);
            if (UNLIKELY(tmpBuf == NULL)) {
                /* Failed to allocate buffer - close connection */
                ism_common_setError(ISMRC_AllocateError);
                transport->close(transport, ISMRC_AllocateError, 0, "Not enough memory to read a message");
                return -1;
            }
            memcpy(tmpBuf->buf, con->rcvBuffer->buf, con->rcvBuffer->used);
            tmpBuf->used = con->rcvBuffer->used;
            tmpBuf->putPtr = tmpBuf->buf + tmpBuf->used;
            ism_common_returnBuffer(con->rcvBuffer, __FILE__, __LINE__);
            con->rcvBuffer = tmpBuf;
        }
        memcpy(con->rcvBuffer->putPtr, rcvBuffer->getPtr, dataLen);
        con->rcvBuffer->putPtr += dataLen;
        con->rcvBuffer->used += dataLen;
        rcvBuffer = con->rcvBuffer;
        dataLen = con->rcvBuffer->used;      /* Length has changed */
        /* If more is needed, return */
        if (con->rcvBuffer->used < con->needBytes) {
            return 0;
        }

        /* Connection error and we should have started connection close */
        if (con->needBytes < 0) {
            con->needBytes = 0;
            return -1;
        }
    }

    /*
     * Process a buffer which can contain multiple records
     */
    while (dataLen > 0) {
        int used = 0;
        con->needBytes = transport->frame(transport, rcvBuffer->buf, offset, rcvBuffer->used, &used);
        // TRACE(9, "frame: connect=%u need=%d offset=%d datalen=%d used=%d\n", transport->index, con->needBytes, offset, dataLen, used);
        offset += used;
        dataLen -= used;
        if (con->needBytes)
            break;
    }

    /*
     * If have complete
     */
    if (con->needBytes == 0 && dataLen == 0) {
        rcvBuffer->getPtr = rcvBuffer->putPtr = rcvBuffer->buf;
        rcvBuffer->used = 0;
        if (con->rcvBuffer) {
            ism_common_returnBuffer(con->rcvBuffer, __FILE__, __LINE__);
            con->rcvBuffer = NULL;
        }
        return 0;
    }
    /* Connection error */
    if (con->needBytes < 0) {
        con->needBytes = 0;
        return -1;
    }

    /*
     * If we have remaining bytes, record them in a buffer
     */
    if (dataLen > 0) {
        /* Allocate a new buffer */
        if (!con->rcvBuffer || dataLen < con->rcvBuffer->allocated) {
            ism_byteBuffer tmpBuf = ism_allocateByteBuffer(con->needBytes + 1024);
            if (tmpBuf == NULL) {
                /* Failed to allocate buffer - close connection */
                ism_common_setError(ISMRC_AllocateError);
                transport->close(transport, ISMRC_AllocateError, 0, "Not enough memory to read a message");
                return -1;
            }
            memcpy(tmpBuf->buf, rcvBuffer->buf+offset, dataLen);
            tmpBuf->used = dataLen;
            tmpBuf->putPtr += dataLen;
            if (con->rcvBuffer)
                ism_common_returnBuffer(con->rcvBuffer, __FILE__, __LINE__);
            con->rcvBuffer = tmpBuf;
        } else {
            /* Just use the buffer we have */
            if (offset)
                memmove(con->rcvBuffer, rcvBuffer->buf+offset, dataLen);
            con->rcvBuffer->used = dataLen;
            con->rcvBuffer->getPtr = con->rcvBuffer->buf;
            con->rcvBuffer->putPtr = con->rcvBuffer->buf + dataLen;
        }
        TRACE(9, "More data needed: connect=%u name=%s datalen=%d offset=%d\n", transport->index, transport->name, dataLen, offset);
        return 0;
    }
    return 0;
}

/*
 * Delayed close.
 *
 * This is done for plug-in transports to allow the final packet from the client to be
 * sent to the plug-in and processed before we handle the close.  This allows the plug-in
 * to handle this as a normal close rather than as a closed by client.
 */
static int closedByClient(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_transport_t * transport = userdata;
    ism_common_cancelTimer(key);
    __sync_sub_and_fetch(&transport->workCount, 1);
    transport->close(transport, ISMRC_ClosedByClient, 0, "The connection was closed by the client.");
    return 0;
}

/*
 * Read data from a non-secure connection
 */
static int readDataTCP(ism_connection_t * con, ism_byteBuffer rcvBuffer) {
    ism_transport_t * transport = con->transport;
    con->state &= ~(ISM_TRANSPORT_STATE_RR);
    int rc = read(con->socket, rcvBuffer->buf, rcvBuffer->allocated);
    if (LIKELY(rc > 0)) {
        rcvBuffer->used = rc;
        rcvBuffer->putPtr = rcvBuffer->buf + rc;
        rcvBuffer->getPtr = rcvBuffer->buf;
        processData(con, rcvBuffer);
        return 0;
    }
    if (rc < 0 && errno == EAGAIN) {
        con->state |= ISM_TRANSPORT_STATE_RR;
        con->state &= ~ISM_TRANSPORT_CAN_READ;
        return 1;
    }
    if (rc < 0 && errno == EINTR)
        return 0;
    con->state |= ISM_TRANSPORT_ERROR;

    switch (transport->closestate[3]) {
    case 1:
        transport->close(transport, 0, 1, "The connection has completed normally.");
        break;
    case 2:
        ism_common_setError(ISMRC_BadClientID);
        transport->close(transport, ISMRC_BadClientID, 0, "The ClientID is not valid");
        break;
    case 3:
        ism_common_setError(ISMRC_ServerCapacity);
        transport->close(transport, ISMRC_ServerCapacity, 0, "The server capacity is reached");
        break;
    case 5:
        ism_common_setError(ISMRC_ConnectNotAuthorized);
        transport->close(transport, ISMRC_ConnectNotAuthorized, 0, "Connection not authorized");
        break;
    case 9:
        ism_common_setError(ISMRC_ClosedOnSend);
        transport->close(transport, ISMRC_ClosedOnSend, 0, "Connection closed during a send operation.");
        break;
    default:
        if (transport->originated) {
            ism_common_setError(ISMRC_ClosedByServer);
            transport->close(transport, ISMRC_ClosedByServer, 0, "The connection was closed by the server.");
        } else {
            if (transport->delay_close) {
                __sync_add_and_fetch(&transport->workCount, 1);
                ism_common_setTimerOnce(ISM_TIMER_HIGH, closedByClient, transport,
                        ((uint64_t)transport->delay_close) * 10000000);   /* 10 ms */
            } else {
                char errMsg[512];
                int err = errno;
                if (rc == 0) {
                    strcpy(errMsg, "The connection was closed by the client.");
                } else {
                    snprintf(errMsg, sizeof errMsg, "The connection was closed by the client: rc=%d, error=%s(%d)", rc, strerror(err),err);
                }
                ism_common_setError(ISMRC_ClosedByClient);
                transport->close(transport, ISMRC_ClosedByClient, 0, errMsg);
            }
        }
    }
    return -1;
}


/*
 * Read data from a SSL/TLS secure connection
 */
static int readDataSSL(ism_connection_t * con, ism_byteBuffer rcvBuffer) {
    int   clean;
    const char * reason;
    ism_transport_t * transport = con->transport;

    con->state &= ~(ISM_TRANSPORT_STATE_RW | ISM_TRANSPORT_STATE_RR);
    errno = 0;
    int rc = SSL_read(con->ssl, rcvBuffer->buf, rcvBuffer->allocated);
    int ec = (rc > 0) ? SSL_ERROR_NONE : SSL_get_error(con->ssl, rc);

    switch (ec) {
    case SSL_ERROR_NONE:
        if (LIKELY(rc > 0)) {
            rcvBuffer->used = rc;
            rcvBuffer->putPtr = rcvBuffer->buf + rc;
            rcvBuffer->getPtr = rcvBuffer->buf;
            processData(con, rcvBuffer);
        }
        return 0;
    case SSL_ERROR_WANT_READ:
        con->state |= ISM_TRANSPORT_STATE_RR;
        con->state &= ~ISM_TRANSPORT_CAN_READ;
        return 1;
    case SSL_ERROR_WANT_WRITE:
        con->state |= ISM_TRANSPORT_STATE_RW;
        con->state &= ~ISM_TRANSPORT_CAN_WRITE;
        return 1;
    case SSL_ERROR_SSL:
        ism_common_logSSLError("TLS read error", __FILE__, __LINE__);
            /* fall thru */
    case SSL_ERROR_ZERO_RETURN:    /* EOF */
    default:                       /* Error */
        if ((ec == SSL_ERROR_ZERO_RETURN) && (SSL_get_shutdown(con->ssl))) {
            clean = 1;
            reason = "The connection has completed normally.";
            ec = 0;
        } else {
            con->state |= ISM_TRANSPORT_ERROR;
            sslTraceErr(transport, ec, __FILE__, __LINE__);
            clean = 0;
            reason = "The connection was closed by the client.";
            ec = ISMRC_ClosedByClient;
        }
        switch (transport->closestate[3]) {
        case 1:
            reason = "The connection has completed normally.";
            ec = 0;
            clean = 1;
            break;
        case 2:
            reason = "The ClientID is not valid";
            ism_common_setError(ISMRC_BadClientID);
            ec = ISMRC_BadClientID;
            break;
        case 3:
            reason = "The server capacity is reached";
            ism_common_setError(ISMRC_ServerCapacity);
            ec = ISMRC_ServerCapacity;
            break;
        case 5:
            reason = "Connection not authorized";
            ism_common_setError(ISMRC_ConnectNotAuthorized);
            ec = ISMRC_ConnectNotAuthorized;
            break;
        case 9:
            reason = "Connection closed during a send operation";
            ism_common_setError(ISMRC_ClosedOnSend);
            ec = ISMRC_ClosedOnSend;
            break;
        default:
            break;
        }
        transport->close(transport, ec, clean, reason);
        return -1;
    }
    return 1;
}


/*
 * Stop the IO listening thread
 */
static int stopIOListening(ism_connection_t * con) {
    ioListenerThread_t * iolth = con->iolth;
    char c = 'C';
    if (iolth == NULL)
        return 0;
    con->iolth = NULL;
    pthread_spin_lock(&iolth->lock);
    con->iolNext = iolth->pendingRequests;
    iolth->pendingRequests = con;
    pthread_spin_unlock(&iolth->lock);
    return write(iolth->pipe_wfd, &c, 1);
}


/*
 * Shutdown the connection
 */
static int connectionShutdown(ism_connection_t * con) {
    int rc;

    if (!con->secured || !con->ssl) {
        stopIOListening(con);
        return 1;
    }

    if (!SSL_in_init(con->ssl)) {
        rc = SSL_shutdown(con->ssl);
        if (rc == 0)                     /* If half shutdown, complete */
            rc = SSL_shutdown(con->ssl);
        if (rc < 0) {                    /* Could be error or EAGAIN */
            int ec = SSL_get_error(con->ssl, rc);
            if (ec == SSL_ERROR_WANT_READ || ec == SSL_ERROR_WANT_WRITE)
                return 1;                /* Wait for more data */
            sslTraceErr(con->transport, ec,__FILE__,__LINE__);
        }
    }
    stopIOListening(con);
    return 1;
}


/*
 * Function to do final close of a connection
 */
static int connectionCloseComplete(ism_connection_t * con) {
    ism_transport_t * transport = con->transport;
    if (transport->state == ISM_TRANST_Closed)
        return 0;
    TRACE(8, "connectionCloseComplete: connect=%u name=%s\n", transport->index, transport->name);
    if (con->socket > 0) {
        resetSocketInfo(con->socket,0,0,0);
        close(con->socket);
        con->socket = 0;
    }
    if (con->secured) {
        if (con->ssl) {
            SSL_free(con->ssl);
            con->ssl = NULL;
            transport->ssl = NULL;
            con->bio = NULL;
        }
    }
    if (con->rcvBuffer) {
        ism_common_returnBuffer(con->rcvBuffer, __FILE__, __LINE__);
        con->rcvBuffer = NULL;
    }
    if (con->sendBuffer){
        ism_common_returnBuffer(con->sendBuffer, __FILE__, __LINE__);
        con->sendBuffer = NULL;
    }
    while (con->sndQueueHead) {
        ism_byteBuffer bb = con->sndQueueHead;
        con->sndQueueHead = bb->next;
        ism_common_returnBuffer(bb, __FILE__, __LINE__);
    }

    /* Destroy the Security Context */
    if (con->transport->security_context) {
        ism_security_destroy_context(con->transport->security_context);
        transport->security_context = NULL;
    }

    while (con->asyncJobRequestsHead) {
        asyncJobRequest_t * next = con->asyncJobRequestsHead->next;
        ism_common_free(ism_memory_transportBuffers,con->asyncJobRequestsHead);
        con->asyncJobRequestsHead = next;
    }
    con->asyncJobRequestsTail = NULL;

    pthread_spin_destroy(&con->slock);
    if (!disableMonitoring)
        ism_transport_removeMonitor(con->transport, 1);
    removeConnectionFromList(con);
    return 0;
}


/*
 * Close the connection during an SSL/TLS handshake
 */
static int sslHandshake(ism_connection_t * con) {
    int   rc;
    char err[2048];
    ism_transport_t * transport = con->transport;

    if (con->state & ISM_TRANSPORT_ERROR){
        ism_common_setError(ISMRC_ClosedTLSHandshake);
        transport->close(transport, ISMRC_ClosedTLSHandshake, 0, "Connection closed during TLS handshake");
        return -1;
    }
    if (transport->originated) {
        rc = SSL_connect(con->ssl);
    } else {
        rc = SSL_accept(con->ssl);
    }
    if (rc == 0) {
        ism_common_setError(ISMRC_ClosedTLSHandshake);
        transport->close(transport, ISMRC_ClosedTLSHandshake, 0, "Connection closed during TLS handshake");
        sslTraceErr(transport, 0, __FILE__, __LINE__);
        return -1;
    }
    if (rc < 0) {
        int ec = SSL_get_error(con->ssl, rc);
        switch (ec) {
        case SSL_ERROR_WANT_WRITE:
            return 0;
        case SSL_ERROR_WANT_READ:
            if (con->bio1) {
                TRACEX(8, TLS, 0, "Switch BIO:  connect=%u\n", transport->index);
                SSL_set_bio(con->ssl, con->bio, con->bio);    /* Will also free bio1*/
                if(con->bio1DataPtr) {
                    ism_common_free(ism_memory_tls,con->bio1DataPtr);
                    con->bio1DataPtr = NULL;
                }
                con->bio1 = NULL;
            }
            return 0;
        default:
            sslTraceErr(transport, ec, __FILE__, __LINE__);
            con->state |= ISM_TRANSPORT_ERROR;
            if (transport->originated) {
                con->transport->write_bytes += con->transport->tlsWriteBytes;
                con->transport->read_bytes += con->transport->tlsReadBytes;
                if (con->transport->connected) {
                    con->transport->connected(con->transport, ISMRC_ServerNotAvailable);
                }
                ism_common_setError(ISMRC_ServerNotAvailable);
                transport->close(transport, ISMRC_ServerNotAvailable, 0, "Server not available");
            } else {
                ism_common_setError(ISMRC_ClosedTLSHandshake);
                transport->close(transport, ISMRC_ClosedTLSHandshake, 0, "Connection closed during TLS handshake");
            }
            return -1;
        }
    } else {
        /*
         * On a completed TLS outgoing connection
         */
        if (transport->originated) {
            uint64_t active = __sync_add_and_fetch(&transport->listener->stats->connect_active, 1);
            uint64_t count = __sync_add_and_fetch(&transport->listener->stats->connect_count, 1);
            TRACE(9, "Increment count for outgoing secure connections: connect=%u name=%s count=%lu active=%lu\n",
                    transport->index, transport->name, count, active);
            transport->write_bytes += transport->tlsWriteBytes;
            transport->read_bytes += transport->tlsReadBytes;
            if (transport->connected) {
                transport->connected(transport, 0);
            }
            /* TODO: for self-signed, verify that this is ours */
            X509 * cert = SSL_get_peer_certificate(con->ssl);

            rc = SSL_get_verify_result(con->ssl);
            if ((rc != X509_V_OK) && (rc != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)) {
                const char * sslErr = X509_verify_cert_error_string(rc);
                sprintf(err,"Server certificate verification failed: %s", sslErr);
                X509_free(cert);
                if (SHOULD_TRACE(4)) {
                    BUF_MEM* certInfo = ism_ssl_getPeerCertInfo(con->ssl,
                            (TRACE_DOMAIN->trcComponentLevels[TRACECOMP_Transport] > 7), 1);
                    TRACE(4, "Certificate verification failed: From=%s:%d connect=%u error=%s\n%s\n",
                             transport->client_addr, transport->clientport, transport->index, sslErr,
                             certInfo->data);
                    BUF_MEM_free(certInfo);
                }
                ism_common_setError(ISMRC_CertificateNotValid);
                transport->close(transport, ISMRC_CertificateNotValid, 0, "Certificate not valid");
                return -1;
            }
            X509_NAME * name = X509_get_subject_name(cert);
            char commonName [64];
            if (X509_NAME_get_text_by_NID(name, NID_commonName, commonName, sizeof(commonName)) != -1) {
                transport->cert_name = ism_transport_putString(transport, commonName);
            }
            X509_free(cert);
        }

        /*
         * On a completed TLS incoming connection
         */
        else {
            if (con->listener->useClientCert && !transport->usePSK){
                X509 * cert = SSL_get_peer_certificate(con->ssl);
                if (cert) {
                    X509_NAME * name;
                    rc = SSL_get_verify_result(con->ssl);
                    if (rc != X509_V_OK) {
                        const char *sslErr = X509_verify_cert_error_string(rc);
                        sprintf(err,"Client certificate verification failed: %s", sslErr);
                        X509_free(cert);
                        if (SHOULD_TRACE(4)) {
                            BUF_MEM* certInfo = ism_ssl_getPeerCertInfo(con->ssl,
                                    (TRACE_DOMAIN->trcComponentLevels[TRACECOMP_Transport] > 7), 1);
                            TRACE(4, "Certificate verification failed: From=%s:%d connect=%u error=%s\n%s\n",
                                    transport->client_addr, transport->clientport, transport->index, sslErr,
                                    certInfo->data);
                            BUF_MEM_free(certInfo);
                        }
                        ism_common_setError(ISMRC_CertificateNotValid);
                        transport->close(transport, ISMRC_CertificateNotValid, 0, "Certificate not valid");
                        return -1;
                    }
                    name = X509_get_subject_name(cert);
                    if (name) {
                        char commonName [1024];
                        if (X509_NAME_get_text_by_NID(name, NID_commonName, commonName, sizeof(commonName)) != -1) {
                            transport->cert_name = ism_transport_putString(transport, commonName);
                        } else {
                            transport->cert_name = "";
                        }
                    }
                    X509_free(cert);
                } else {
                    if (con->listener->useClientCert != 2) {
                        ism_common_setError(ISMRC_NoCertificate);
                        transport->close(transport, ISMRC_NoCertificate, 0, "Certificate missing");
                        return -1;
                    }
                }
            }
            if (transport->usePSK) {
                const char* identity = SSL_get_psk_identity(con->ssl);
                int len = strlen(identity) + 1;
                transport->cert_name = (const char *)ism_transport_allocBytes(transport, len, 0);
                memcpy((char *)transport->cert_name,identity,len);
            }
        }
        con->state = (ISM_TRANSPORT_CONNECTED | ISM_TRANSPORT_CAN_READ | ISM_TRANSPORT_CAN_WRITE);
        return 0;
    }
    return 0;
}


/*
 * Return the SSL object from a transport object
 */
struct ssl_st * ism_transport_getSSL(ism_transport_t * transport) {
    return transport->tobj->ssl;
}

/*
 *
 */
static void addAsyncRequest(ism_connection_t * con, asyncJobRequest_t *pReq) {
    int addJob=0;
    pReq->next = NULL;
    pthread_spin_lock(&con->slock);
    if (con->asyncJobRequestsTail) {
        con->asyncJobRequestsTail->next = pReq;
    } else {
        con->asyncJobRequestsHead =pReq;
        addJob = 1;
    }
    con->asyncJobRequestsTail = pReq;
    pthread_spin_unlock(&con->slock);
    if (addJob)
        addJob4Processing(con, 0);
}

static void processAsyncRequests(ism_connection_t * con) {
    asyncJobRequest_t * curr;
    pthread_spin_lock(&con->slock);
    curr = con->asyncJobRequestsHead;
    con->asyncJobRequestsHead = con->asyncJobRequestsTail = NULL;
    pthread_spin_unlock(&con->slock);
    while (curr) {
        asyncJobRequest_t * next = curr->next;
        if (curr->func(curr->transport,curr->param1, curr->param2)) {
            addAsyncRequest(con,curr);
        } else {
            ism_common_free(ism_memory_transportBuffers,curr);
        }
        curr = next;
    }
}


/*
 * Sledgehammer timer for connection shutdown
 */
static int sledgeTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_connection_t * con = (ism_connection_t *) userdata;
    pthread_spin_lock(&con->slock);
    if (con->sledgecount > 0) {
        con->sledgecount--;
        if (con->sledgecount == 0)
            addJob4Processing(con, SHUTDOWN_FORCE);
    }
    pthread_spin_unlock(&con->slock);
    return 1;
}

/*
 * Process an IO request
 */
HOT static int processIORequest(ism_connection_t * con, ioProcessorThread iopth) {
    int rc1 = 1;
    int rc2 = 1;
    int state = con->state;
    ism_transport_t * transport = con->transport;

    if (UNLIKELY(state & ISM_TRANSPORT_DISCONNECTED)) {
        return 1;
    }

    /* Clear any openSSL errors which might pertain to other connections */
    if (con->secured) {
        while (ERR_get_error());
    }

    /*
     * If we are in shutdown, do not read any more data, but try to drain the send buffers.
     * Close the connection anyway if we do not get any progress in draining the buffer.
     */
    if (UNLIKELY(state & ISM_TRANSPORT_SHUTDOWN_IN_PROCESS)) {
        rc1 = writeData(con);
        if (state & ISM_TRANSPORT_STATE_WW && !(state & ISM_TRANSPORT_SHUTDOWN_FORCE)) {
            if (rc1 == 0)
                return 0;
            if (rc1 == 1) {
                if (!con->sledgetimer) {
                    pthread_spin_lock(&con->slock);
                    con->sledgecount = 8;
                    pthread_spin_unlock(&con->slock);
                    con->sledgetimer = ism_common_setTimerRate(ISM_TIMER_HIGH, (ism_attime_t) sledgeTimer, con, 200, 200, TS_MILLISECONDS);
                } else {
                    pthread_spin_lock(&con->slock);
                    if (con->sledgecount-- == 0)
                        rc1 = 2;
                    pthread_spin_unlock(&con->slock);
                }
                if (rc1 == 1)
                    return 1;
            }
        }
        if (con->sledgetimer) {
            ism_common_cancelTimer(con->sledgetimer);
            con->sledgetimer = NULL;
        }
        return connectionShutdown(con);
    }

    if (UNLIKELY(state & ISM_TRANSPORT_CONNECT_IN_PROCESS)) {
        int err = 0;
        uint64_t active;
        uint64_t count;
        socklen_t len = sizeof(err);
        getsockopt(con->socket, SOL_SOCKET, SO_ERROR, &err, &len);
        if (!err) {
            struct sockaddr_in6 sa;
            socklen_t sal = sizeof(sa);
            if (getpeername(con->socket, (struct sockaddr*)&sa, &sal)) {
                if (errno == ENOTCONN)
                    return 1;
            }
            if (con->tlsCTX) {
                makeTlsClientObjects(transport);
                state = con->state = ISM_TRANSPORT_HANDSHAKE_IN_PROCESS |
                        ISM_TRANSPORT_CONNECTED | ISM_TRANSPORT_CAN_READ | ISM_TRANSPORT_CAN_WRITE;
            } else {
                state = con->state = ISM_TRANSPORT_CONNECTED | ISM_TRANSPORT_CAN_READ | ISM_TRANSPORT_CAN_WRITE;
                active = __sync_add_and_fetch(&transport->listener->stats->connect_active, 1);
                count = __sync_add_and_fetch(&transport->listener->stats->connect_count, 1);
                TRACE(9, "Increment count for outgoing connections: connect=%u name=%s count=%lu active=%lu\n",
                        transport->index, transport->name, count, active);
                if (transport->connected) {
                    transport->connected(transport, 0);
                }
            }
        } else {
            con->state |= ISM_TRANSPORT_ERROR;
            con->transport->write_bytes += con->transport->tlsWriteBytes;
            con->transport->read_bytes += con->transport->tlsReadBytes;
            if (con->transport->connected) {
                con->transport->connected(con->transport, ISMRC_ServerNotAvailable);
            }
            TRACE(4, "Connection error: connect=%u name=%s rc=%d error=%s\n", con->transport->index,
                    con->transport->name, err, strerror(err));
            ism_common_setError(ISMRC_ServerNotAvailable);
            transport->close(transport, ISMRC_ServerNotAvailable, 0, "Server not available");
            return 1;
        }
    }

    if (UNLIKELY(state & ISM_TRANSPORT_HANDSHAKE_IN_PROCESS)) {
        /* SSL handshake is required */
        rc1 = sslHandshake(con);
        if (rc1 == 0)
            return 0;
        rc1 = writeData(con);
        if (rc1 < 0)
            return rc1;
    }

    /* Check if we can read */
    if (LIKELY(transport->state != ISM_TRANST_Closing)) {
        if (CAN_READ(state)) {
            if (!con->secured) {
                rc1 = readDataTCP(con, iopth->recvBuffer);
            } else {
                rc1 = readDataSSL(con, iopth->recvBuffer);
            }
            if (UNLIKELY(rc1 < 0)) {
                processAsyncRequests(con);
                return rc1;
            }
        }
    }

    processAsyncRequests(con);

    if (CAN_WRITE(state)) {
        rc2 = writeData(con);
        if (UNLIKELY(rc2 < 0))
            return rc2;
    }
    return (rc1 && rc2);
}

/*
 * The ioListener thread
 */
static void * ioListenerProc(void * parm, void * context, int value) {
    ioListenerThread_t * thData = (ioListenerThread_t *) parm;
    int eventSize = 64*1024;
    epoll_event * events = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 9),eventSize, sizeof(epoll_event));
    int pipefd[2];
    int run = 1;
    int efd = thData->efd;
    xUNUSED int rc = pipe2(pipefd, O_NONBLOCK | O_CLOEXEC);
    assert(rc != -1);

    memset(&events[0], 0, sizeof(epoll_event));
    events[0].data.fd = pipefd[0];
    events[0].events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    rc = epoll_ctl(efd, EPOLL_CTL_ADD, pipefd[0], &events[0]);
    assert (rc != -1);

    thData->pipe_wfd = pipefd[1];
    while (run) {
        int i;
        int count;
        ism_connection_t * pendingRequests;
        ioConnectionJob * conJobs;
        pthread_spin_lock(&thData->lock);
        pendingRequests = thData->pendingRequests;
        conJobs = thData->connectionJobs;
        thData->pendingRequests = NULL;
        thData->connectionJobs = NULL;
        pthread_spin_unlock(&thData->lock);
        while (pendingRequests) {
            ism_connection_t * con = pendingRequests;
            pendingRequests = pendingRequests->iolNext;
            epoll_ctl(efd, EPOLL_CTL_DEL, con->socket, NULL);
            addJob4Processing(con, CLEANUP_REQUEST);
        }
        while (conJobs) {
            ioConnectionJob * job = conJobs;
            conJobs = job->next;
            processConnectionRequest(job);
            ism_common_free(ism_memory_transportBuffers,job);
        }
        ism_common_backHome();
        count = epoll_wait(efd, events, eventSize, -1);
        ism_common_going2work();
        if (LIKELY(count > 0)) {
            for (i = 0; i < count; i++) {
                struct epoll_event * event = &events[i];
                if (LIKELY(event->data.fd != pipefd[0])) {
                    ism_connection_t * con = event->data.ptr;
                    if (LIKELY(con != NULL)) {
                        uint64_t epollEvents = event->events;
#if EPOLL_DEBUG
                        ism_transport_t * transport = con->transport;
#endif
#if 0
                        if (epollEvents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                            __sync_fetch_and_or(&con->state, ISM_TRANSPORT_ERROR);
                        }
#endif
#if EPOLL_DEBUG
                        if((epollEvents & EPOLLOUT) && (transport->name[0])) {
                            TRACE(5, "ioListenerProc() : EPOLLOUT for: con=%p connect=%u name=%s sock=%d from %s:%u\n", con,
                                   transport->index, transport->name, con->socket, transport->client_addr, transport->clientport );
                        }
#endif
                        addJob4Processing(con, epollEvents);
                        continue;
                    }
                } else {
                    char c;
                    while (read(pipefd[0], &c, 1) > 0) {
                        if (c == 'S') {
                            run = 0;
                            break;
                        }
                    }
                    if (run)
                        continue;
                    break;
                }
            }
            if (UNLIKELY(count == eventSize)) {
                eventSize *= 2;
                events = ism_common_realloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 29),events, eventSize * sizeof(epoll_event));
            }
            continue;
        }
        if ((count == 0) || (errno == EINTR))
            continue;
        ism_common_free(ism_memory_transportBuffers,events);
        ism_common_backHome();
//        ism_common_endThread(NULL);
        return NULL;
    }
    ism_common_backHome();
    ism_common_free(ism_memory_transportBuffers,events);
    close(thData->efd);
    close(pipefd[0]);
    close(pipefd[1]);
//    ism_common_endThread(NULL);
    return NULL;
}

/*
 * Send bytes on output
 */
HOT static int sendBytes(ism_transport_t * transport, char * buf, int len, int protval, int flags) {
    char fbuf[32];
    int flen = 0;
    int buflen;
    ism_byteBufferPool pool;
    ism_byteBuffer sndBuffer = NULL;
    ism_byteBuffer sndBufferHead = NULL;
    ism_byteBuffer sndBufferTail = NULL;
    int force = 0;
    int addJob = 0;
    ism_connection_t * con = transport->tobj;
    int counter = 0;
    int rc = SRETURN_OK;
    int state = con->state & (ISM_TRANSPORT_ERROR | ISM_TRANSPORT_DISCONNECTED | ISM_TRANSPORT_SHUTDOWN_IN_PROCESS);
    if (UNLIKELY(state))
        return SRETURN_BAD_STATE;
    /* Handle frames */
    if (UNLIKELY(flags & SFLAG_HASFRAME)) {
        flen = 0;
    } else {
        if (LIKELY(flags & SFLAG_FRAMESPACE)) {
            /* If there is space for the frame before the buffer, put it there. */
            flen = transport->addframe(transport, buf, len, protval);
            buf -= flen;
            len += flen;
            flen = 0;
        } else {
            flen = transport->addframe(transport, fbuf + sizeof(fbuf), len, protval);
        }
    }
    buflen = len + flen;
    if (LIKELY(con->doNotBatch == 0)) {
        pthread_spin_lock(&con->slock);
        sndBuffer = con->sndQueueTail;
        if (sndBuffer && ((sndBuffer->used + buflen) < sndBuffer->allocated)) {
            //        assert(sndBuffer->putPtr == (sndBuffer->buf + sndBuffer->used));
            /* There is enough space in last buffer in the send queue */
            if (UNLIKELY(flen)) {
                memcpy(sndBuffer->putPtr, fbuf + sizeof(fbuf) - flen, flen);
                sndBuffer->putPtr += flen;
                sndBuffer->used += flen;
            }
            memcpy(sndBuffer->putPtr, buf, len);
            sndBuffer->putPtr += len;
            sndBuffer->used += len;
            if (UNLIKELY(con->transport->suspended))
                rc = SRETURN_SUSPEND;
            pthread_spin_unlock(&con->slock);
            return rc;
        }
        pthread_spin_unlock(&con->slock);
    }
    pool = con->iopth->bufferPool;
    do {
        sndBuffer = ism_common_getBuffer(pool, force);
        if (LIKELY(sndBuffer != NULL)) {
            int toCopy;
            if (UNLIKELY(flen)) {
                memcpy(sndBuffer->putPtr, fbuf + sizeof(fbuf) - flen, flen);
                sndBuffer->putPtr += flen;
                sndBuffer->used += flen;
                buflen -= flen;
                flen = 0;
            }
            toCopy = ((sndBuffer->used + buflen) < sndBuffer->allocated) ? buflen : (sndBuffer->allocated
                    - sndBuffer->used);
            memcpy(sndBuffer->putPtr, buf, toCopy);
            sndBuffer->putPtr += toCopy;
            sndBuffer->used += toCopy;
            buf += toCopy;
            buflen -= toCopy;
            if (sndBufferTail) {
                sndBufferTail->next = sndBuffer;
                sndBufferTail = sndBuffer;
            } else {
                sndBufferHead = sndBufferTail = sndBuffer;
            }
//            assert(sndBuffer->putPtr == (sndBuffer->buf + sndBuffer->used));
//            assert(sndBuffer->used <= sndBuffer->allocated);
            counter++;
            if (UNLIKELY(buflen))
                continue;
            break;
        }
        force = 1;
        if (buflen)
            continue;
        break;
    } while (1);
    pthread_spin_lock(&con->slock);
    if (UNLIKELY(force))
        __sync_bool_compare_and_swap(&transport->suspended,0,1);
    if (con->sndQueueTail) {
        con->sndQueueTail->next = sndBufferHead;
        con->sndQueueTail = sndBufferTail;
    } else {
        con->sndQueueHead = sndBufferHead;
        con->sndQueueTail = sndBufferTail;
        addJob = 1;
    }
    transport->sendQueueSize += counter;
    if (transport->sendQueueSize > 128)
        __sync_bool_compare_and_swap(&transport->suspended,0,1);
    if (UNLIKELY(transport->suspended)) {
        rc = SRETURN_SUSPEND;
#ifdef DEBUG
        TRACEL(8, transport->trclevel, "Connection suspended: connect=%u sendQueueSize=%d\n", transport->index, transport->sendQueueSize);
#endif
    }
    pthread_spin_unlock(&con->slock);
    if (addJob)
        addJob4Processing(con, 0);
    return rc;
}


#if 0
static comp_tls_init   engine_tls_init = NULL;
static void get_tls_init_functions(void) {
    void * lib = dlopen(NULL,RTLD_LAZY);
    if (lib) {
        engine_tls_init = dlsym(lib,"ism_engine_threadInit");
        dlclose(lib);
        if (engine_tls_init == NULL) {
            char  * errStr = dlerror();
            if (errStr)
                fprintf(stderr,"Unable to get symbol %s : %s","ism_engine_threadInit", errStr);
        }
    } else {
        char  * errStr = dlerror();
        if (errStr)
            fprintf(stderr,"dlopen failed: %s",errStr);
    }
}
#else
extern void ism_engine_threadInit(uint8_t isStoreCrit);
#endif

#undef TRACE_DOMAIN
#define TRACE_DOMAIN ism_defaultTrace

/*
 * IO Processor thread
 */
HOT static void * ism_tcp_ioProcessorThreadProc(void * parm, void * context, int value) {
    int i;
    ioProcessorThread_t * thData = (ioProcessorThread_t*) parm;
    uint32_t currentAllocated = 64 * 1024;
    ism_connection_t** current = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 10),currentAllocated, sizeof(ism_connection_t*));
    uint32_t currentSize = 0;
    uint32_t nextSize = 0;

    /* Initialize the engine TLS for the thread */
    ism_engine_threadInit(value);

    while (!thData->isStopped) {
        int rc;
        iopJobsList * currentJobsList;
        ism_common_backHome();
        if (useSpinLocks) {
            if (iopDelay && (nextSize == 0) && (thData->currentJobsList->used == 0)) {
                if (value) {
                    if (iopDelay > 0) {
                        for (i = 0; i < iopDelay; i++) {
                            pthread_yield();
                        }
                    } else {
                        ism_common_sleep(-iopDelay);
                    }
                } else {
                    /* This is an admin IOP thread */
                    ism_common_sleep(1000);
                }
            }
            pthread_spin_lock(&thData->lock);
        } else {
            pthread_mutex_lock(&thData->mutex);
            if (iopDelay && (nextSize == 0) && (thData->currentJobsList->used == 0)) {
                pthread_cond_wait(&thData->cond, &thData->mutex);
                if(thData->isStopped){
                    pthread_mutex_unlock(&thData->mutex);
                    continue;
                }
            }
        }
        currentJobsList = thData->currentJobsList;
        thData->currentJobsList = thData->nextJobsList;
        thData->nextJobsList = currentJobsList;
        if (useSpinLocks)
            pthread_spin_unlock(&thData->lock);
        else
            pthread_mutex_unlock(&thData->mutex);
        ism_common_going2work();
        for (i = 0; i < currentJobsList->used; i++) {
            ioProcJob * job = currentJobsList->jobs + i;
            ism_connection_t * con = job->con;
            uint64_t events = job->events;
            if (events) {
                if (events & 0xFFFFFFFF) {
#if EPOLL_DEBUG
                    ism_transport_t * transport = con->transport;
#endif
//                    int state = con->state;
                    if (events & EPOLLIN) {
                        con->state |= ISM_TRANSPORT_CAN_READ;
                    }
                    if (events & EPOLLOUT)   {
                        con->state |= ISM_TRANSPORT_CAN_WRITE;
#if EPOLL_DEBUG
                        if(transport->name[0]) {
	                        TRACE(5, "ioProc() : EPOLLOUT for: con=%p connect=%u name=%s sock=%d from %s:%u\n", con,
                                transport->index, transport->name, con->socket, transport->client_addr, transport->clientport );
                        }
#endif
                    }
                    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                        ism_transport_t * transport = con->transport;
                        if(transport->originated && (con->state & ISM_TRANSPORT_CONNECT_IN_PROCESS)) {
                            TRACE(4, "ioProc() : Set error state for connection in process: con=%p transport=%p connect=%u name=%s sock=%d events=0x%llx\n",
                                    con, transport, transport->index, transport->name, con->socket, (ULL)events);
                        }
                        __sync_fetch_and_or(&con->state, (ISM_TRANSPORT_ERROR | ISM_TRANSPORT_CAN_READ) );
                    }
//                    fprintf(stderr,"ism_tcp_ioProcessorThreadProc: tobj=%p oldState=0x%x state=0x%x\n", con, state, con->state);
                } else {
                    if (events == SHUTDOWN_REQUEST)
                        con->state |= ISM_TRANSPORT_SHUTDOWN_IN_PROCESS;
                    else if (events == SHUTDOWN_FORCE)
                        con->state |= (ISM_TRANSPORT_SHUTDOWN_IN_PROCESS | ISM_TRANSPORT_SHUTDOWN_FORCE);
                    else
                        con->state |= ISM_TRANSPORT_DISCONNECTED;
                }
            }
            if (!con->isProcessing) {
                if (nextSize == currentAllocated) {
                    currentAllocated *= 2;
                    current = ism_common_realloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 30),current, currentAllocated * sizeof(ism_connection_t*));
                }
                current[nextSize++] = con;
                con->isProcessing = 1;
            }
        }
        currentJobsList->used = 0;
        currentSize = nextSize;
        nextSize = 0;
        for (i = 0; i < currentSize; i++) {
            ism_connection_t * con = current[i];
            current[i] = NULL;
            if (con->transport->state != ISM_TRANST_Closed) {
                ism_common_going2work();
                rc = processIORequest(con, thData);
                if (!rc) {
                    current[nextSize++] = con;
                } else {
                    con->isProcessing = 0;
                    if (con->state & ISM_TRANSPORT_DISCONNECTED) {
                        connectionCloseComplete(con);
                    }
                }
                ism_common_backHome();
            }
        }
    }
    ism_common_backHome();
    ism_common_free(ism_memory_transportBuffers,current);
    ism_common_destroyBufferPool(thData->bufferPool);
    ism_common_returnBuffer(thData->recvBuffer, __FILE__, __LINE__);
//    ism_common_endThread(NULL);
    return NULL;
}

/*
 * Create the connection thread
 */
static ioConnectionThread createIOCThread(const char * threadname) {
    ioConnectionThread iocth = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 11),1, sizeof(ioConnectionThread_t));
    iocth->efd = epoll_create1(EPOLL_CLOEXEC);
    ism_common_startThread(&iocth->thread, conListenerProc, iocth, NULL, 0, ISM_TUSAGE_NORMAL, 0, threadname,
            "TCP connection listener");
    return iocth;

}

/*
 * Stop the connection thread
 */
static void stopIOCThread(ioConnectionThread iocth) {
    if (iocth) {
        void * result = NULL;
        char c = 'S';
        if (write(iocth->pipe_wfd, &c, 1) > 0) {
            pthread_join(iocth->thread, &result);
        }
    }
    return;
}

/*
 * Start the listener thread
 */
static ioListenerThread createIOLThread(const char * threadname) {
    ioListener = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 12),1, sizeof(ioListenerThread_t));
    ioListener->efd = epoll_create(65536);
    pthread_spin_init(&ioListener->lock, 0);
    ism_common_startThread(&ioListener->thread, ioListenerProc, ioListener, NULL, 0, ISM_TUSAGE_NORMAL, 0, threadname,
            "TCP IO listener");
    return ioListener;
}

/*
 * Stop the listener thread
 */
static void stopIOLThread(ioListenerThread iolth) {
    if (iolth) {
        void * result = NULL;
        char c = 'S';
        if (write(iolth->pipe_wfd, &c, 1) > 0) {
            pthread_join(iolth->thread, &result);
        }
    }
    return;
}

/*
 * Create the IO processor thread
 */
static ioProcessorThread createIOPThread(const char * threadname, ioListenerThread iolth) {
    ioProcessorThread_t * iopth = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 13),1, sizeof(ioProcessorThread_t));
    int maxPoolSize = (int)(maxPoolSizeBytes/sendSize);
    iopth->iolth = iolth;
    pthread_spin_init(&iopth->lock, 0);
    pthread_mutex_init(&iopth->mutex, NULL);
    pthread_cond_init(&iopth->cond, NULL);
    iopth->bufferPool = ism_common_createBufferPool(sendSize, 1024, maxPoolSize,threadname);
    iopth->recvBuffer = ism_allocateByteBuffer(recvSize);
    iopth->jobsList[0].allocated = 64*1024;
    iopth->jobsList[0].used = 0;
    iopth->jobsList[0].jobs = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 14),iopth->jobsList[0].allocated, sizeof(ioProcJob));
    iopth->jobsList[1].allocated = 64*1024;
    iopth->jobsList[1].used = 0;
    iopth->jobsList[1].jobs = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 15),iopth->jobsList[1].allocated, sizeof(ioProcJob));
    iopth->currentJobsList = &(iopth->jobsList[0]);
    iopth->nextJobsList = &(iopth->jobsList[1]);
    int value = 1;
    if ( *threadname == 'A' )
        value = 0;
    ism_common_startThread(&iopth->thread, ism_tcp_ioProcessorThreadProc, iopth, NULL, value, ISM_TUSAGE_NORMAL, 0, threadname,
            "TCP IO Processor");
    return iopth;
}


/*
 * Stop the IO processor thread
 */
static void stopIOPThread(ioProcessorThread iopth) {
    void * result = NULL;
    if (iopth) {
        if(useSpinLocks) {
            iopth->isStopped = 1;
        }else {
            pthread_mutex_lock(&iopth->mutex);
            iopth->isStopped = 1;
            pthread_mutex_unlock(&iopth->mutex);
            pthread_cond_signal(&iopth->cond);
        }
        pthread_join(iopth->thread, &result);
    }
    return;
}


/*
 * Timer function for cleanup
 */
static int cleanupTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_connection_t * curr;
    ism_connection_t * next;
    int counter = 0;
    pthread_mutex_lock(&conMutex);
    curr = closedConnections;
    while (curr) {
        ism_transport_t * transport = curr->transport;
        next = curr->conListNext;
        if (transport->workCount) {
            curr = next;
            continue;
        }
        if (disableMonitoring) {
            if (next) {
                next->conListPrev = curr->conListPrev;
            }
            if (curr->conListPrev) {
                curr->conListPrev->conListNext = next;
            } else {
                closedConnections = next;
            }
            ism_transport_freeTransport(transport);
            counter++;
        } else {
            if (transport->monitor_id) {
                ism_transport_removeMonitorNow(transport);
            }
            if (transport->closestate[0] > 1) {
                transport->closestate[1]++;
            }
            if (transport->closestate[1] > 1) {
                if (next) {
                    next->conListPrev = curr->conListPrev;
                }
                if (curr->conListPrev) {
                    curr->conListPrev->conListNext = next;
                } else {
                    closedConnections = next;
                }
                TRACE(8, "cleanupTimer - going to free connection: connect=%u name=%s\n", transport->index, transport->name);
                ism_transport_freeTransport(transport);
                counter++;
            }
        }
        curr = next;
    }
    pthread_mutex_unlock(&conMutex);
    if (counter)
        __sync_sub_and_fetch(&activeConnectionsCounter, counter);

    return 1;
}

/*
 * Timer function for stale connections lookup
 */
static int ddosTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_connection_t * currCon;
    ism_time_t  currTime = ism_common_currentTimeNanos();

    pthread_mutex_lock(&conMutex);
    for (currCon = activeConnections; currCon != NULL ; currCon = currCon->conListNext) {
        ism_transport_t * transport = currCon->transport;

        if (!transport->ready) {
            if ((currTime - transport->connect_time) > 60*BILLION) {
                TRACE(6, "Close a connection because the initial packet has not been received: connect=%u\n", transport->index);
                ism_common_setError(ISMRC_NoFirstPacket);
                transport->close(transport, ISMRC_NoFirstPacket, 0, "No data was received on the connection");
            }
        }
    }
    pthread_mutex_unlock(&conMutex);
    return 1;
}


/*
 * Initialize the TCP transport
 */
int ism_transport_initTCP(void) {
    struct rlimit rlim;
    int    maxcount,i;
    uint64_t  maxPoolSizeMB;

    /* Set the receive size */
    recvSize = ism_common_getBuffSize("TcpRecvSize", ism_common_getStringConfig("TcpRecvSize"), "16384");
    if (recvSize < 512)
        recvSize = 512;
    if (recvSize > (1024 * 1024))
        recvSize = 1024 * 1024;

    /* Set the send size */
    sendSize = ism_common_getBuffSize("TcpSendSize", ism_common_getStringConfig("TcpSendSize"), "16384");
    if (sendSize < 512)
        sendSize = 512;
    if (sendSize > (1024 * 1024))
        sendSize = 1024 * 1024;
    tcpMaxCon = ism_common_getIntConfig("TcpMaxCon", 65535);

    numOfIOProcs = ism_common_getIntConfig("TcpThreads", 1);
    ioProcessors = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 16),numOfIOProcs, sizeof(ioProcessorThread));
    maxPoolSizeMB = ism_common_getIntConfig("TcpMaxTransportPoolSizeMB", 500);
    if (maxPoolSizeMB < 32)
        maxPoolSizeMB = 32;

    maxPoolSizeBytes =  ((maxPoolSizeMB*1024*1024) / (numOfIOProcs + 1));

    iopDelay = ism_common_getIntConfig("TcpIOPThreadDelayMicro", -1);
    tobjFromPool = ism_common_getBooleanConfig("TcpGetTobjFromPool", 1);
    disableMonitoring = ism_common_getIntConfig("TcpDisableMonitoring", 0);
    TRACE(4, "Initialize the TCP transport: threads=%d poolsize=%uMB\n", numOfIOProcs + 1, (uint32_t)maxPoolSizeMB);

    /*
     * Start a timer for cleanup
     */
    TRACE(8, "set tcp cleanup: cleanup_timer=%llu\n", (ULL)cleanup_timer);
    if (!cleanup_timer) {
        cleanup_timer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) cleanupTimer, NULL, 10, 3, TS_SECONDS);
    }
    if (!ddos_timer) {
        ddos_timer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) ddosTimer, NULL, 60, 60, TS_SECONDS);
    }

    activeConnectionsMax = ism_common_getIntConfig("TcpMaxConnections", ACTIVE_CONNECTION_MAX_DEFAULT);
    getrlimit(RLIMIT_NOFILE, &rlim);
    maxcount = (int)rlim.rlim_cur - 512;
    maxcount = maxcount/100 * 50;
    if (activeConnectionsMax > maxcount)
        activeConnectionsMax = maxcount;
    TRACE(5, "Set maximum TCP connections: %d\n", activeConnectionsMax);
    maxSocketId = 4096;
    allocSocketId = rlim.rlim_cur;
    if (allocSocketId < 4096)
        allocSocketId = 4096;
    socketsInfo = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 17),allocSocketId, sizeof(socketInfo_t));
    for (i = 0; i < maxSocketId; i++) {
        pthread_spin_init(&socketsInfo[i].lock,0);
    }
    g_stopped = 1;
    chkRcvBuffTimer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) conRcvBufCheckTimer, NULL, 30, 30, TS_SECONDS);

    g_conciseLog = ism_common_getIntConfig("ConnectionLogConcise", 0);
    useSpinLocks = ism_common_getBooleanConfig("UseSpinLocks", 0);
    g_ctxPerThread = ism_common_getBooleanConfig("TlsContextPerThread", 0);

    /*
     * NoLogAddres is an IPv4 address and scope such as "127.0.0.0/24".
     * The scope can be between 8 and 30.  If no score is given, a single
     * IP address is blocked from logging.
     */
    const char * nolog = ism_common_getStringConfig("ConnectionLogIgnore");
    if (nolog) {
        ism_transport_setNoLog(nolog);
    }
    return 0;
}

/*
 * Return the max tcp connections
 */
int ism_transport_getTcpMax(void) {
    return allocSocketId;
}


/* These methods can only be used during the start TCP listener callback */
ism_secprof_t * ism_transport_getSecProfile(const char * name);
ism_certprof_t * ism_transport_getCertProfile(const char * name);
ism_msghub_t * ism_transport_getMsgHub(const char * name);


/*
 * SSL methods enumeration
 */
ism_enumList enum_methods2 [] = {
    { "Method",    4,                 },
    { "SSLv3",     SecMethod_SSLv3,   },
    { "TLSv1",     SecMethod_TLSv1,   },
    { "TLSv1.1",   SecMethod_TLSv1_1, },
    { "TLSv1.2",   SecMethod_TLSv1_2, },
    { "TLSv1.3",   SecMethod_TLSv1_3, },
};


/*
 * Update the TCP endpoint without restarting
 */
int ism_transport_updateTCPEndpoint(ism_listener_t * endpoint) {
    epoll_event event = {0};
    event.data.ptr = endpoint;
    event.events = EPOLLIN | EPOLLRDHUP;
    return epoll_ctl(conListener->efd, EPOLL_CTL_MOD, endpoint->sock, &event) == -1 ? 1 : 0;
}

static ismHashMap * getAllowedClientsMap(ima_transport_info_t * tInfo) {
    ism_transport_t * transport = ( ism_transport_t *) tInfo;
    if(transport->listener && transport->listener->secProfile)
        return transport->listener->secProfile->allowedClientsMap;
    return NULL;
}

/*
 * An openssl verify_cb callback
 */
static  int checkPrecertifiedClient(int preverify_ok, X509_STORE_CTX *storeCTX) {
    return ism_ssl_checkPreverifiedClient(preverify_ok, storeCTX, getAllowedClientsMap);
}

/*
 * Start the transport.
 * This is called multiple times if there are multiple tcp ports.
 * This is called with the listenerlock held
 */
int ism_transport_startTCPEndpoint(ism_endpoint_t * endpoint) {
    SOCKET  portSocket;
    epoll_event event;
    int  i;
    int  reOpen = 0;
    ism_certprof_t * certprof = NULL;

    if (endpoint->needed == 0)
        return 0;                 /* Nothing to do */

    /*
     * If a socket and/or security context exists, delete them
     */
    if (endpoint->sock) {
        stopPortListening(endpoint);
        endpoint->sock = 0;
        reOpen = 1;
    }

    /*
     * Start timer for CRL updates
     * - Timer is always running provided there were no errors inside the CRL timer task
     */
    ism_config_startEndpointCRLTimer(endpoint->name);

    /*
     * Delete any existing contexts.  As openssl keeps a reference count, the actual
     * context will only be deleted when the reference count goes to zero.
     */
    if (endpoint->sslCTX && endpoint->isInternal != 2) {
        for (i=0; i < endpoint->sslctx_count; i++) {
            ism_common_destroy_ssl_ctx(endpoint->sslCTX[i]);
        }
        if (endpoint->sslCTX != &endpoint->sslCTX1)
            ism_common_free(ism_memory_transportBuffers,endpoint->sslCTX);
        endpoint->sslctx_count = 1;
        endpoint->sslCTX = NULL;
    }

    /*
     * If we are enabled, start processing
     */
    if (endpoint->enabled) {
        endpoint->thread_count = numOfIOProcs + 1;
        if (endpoint->msghub) {
            ism_msghub_t * msghub = ism_transport_getMsgHub(endpoint->msghub);
            if (!msghub) {
                endpoint->enabled = 0;
                endpoint->rc = ISMRC_MessageHub;
                ism_common_setError(endpoint->rc);
                return endpoint->rc;
            }
        }
        ism_secprof_t * secprofile = NULL;
        if (endpoint->isInternal != 2) {
            if (endpoint->secure != 2)
                endpoint->secure = 0;
            if (endpoint->secprof) {
                secprofile = ism_transport_getSecProfile(endpoint->secprof);
                if (!secprofile) {
                    endpoint->enabled = 0;
                    endpoint->rc = ISMRC_NoSecProfile;
                    ism_common_setError(endpoint->rc);
                    return endpoint->rc;
                }
                endpoint->secProfile = secprofile;

                certprof = ism_transport_getCertProfile(secprofile->certprof);
                if (secprofile->tlsenabled && !certprof) {
                    endpoint->enabled = 0;
                    endpoint->rc = ISMRC_NoCertProfile;
                    ism_common_setError(endpoint->rc);
                    return endpoint->rc;
                }

                if (certprof && (secprofile->tlsenabled || endpoint->isAdmin))  {
                    /* Only set SSL-related values if TLSEnabled is False */
                    endpoint->secure = 1;
                    endpoint->useClientCert = secprofile->clientcert;
                    if (endpoint->isAdmin && !secprofile->tlsenabled) {
                        endpoint->secure = 2;
                        if (endpoint->useClientCert)
                            endpoint->useClientCert = 2;
                    }

                    endpoint->sslCTX1 = ism_common_create_ssl_ctx(endpoint->name, ism_common_enumName(enum_methods2, secprofile->method),
                        secprofile->ciphers, certprof->cert, certprof->key, 1, secprofile->sslop, endpoint->useClientCert, secprofile->name,
                        ((secprofile->allowedClientsMap) ? checkPrecertifiedClient : NULL), NULL);
                    if (endpoint->sslCTX1 == NULL) {
                        endpoint->enabled = 0;
                        endpoint->rc = ISMRC_CreateSSLContext;
                        ism_common_setError(endpoint->rc);
                        TRACE(2, "The SSL context could not be created for endpoint %s\n", endpoint->name);
                        /* This is logged in ism_common_create_ssl_ctl() */
                        return endpoint->rc;
                    }
                    endpoint->sslCTX = &endpoint->sslCTX1;
                    endpoint->sslctx_count = 1;
                }
                /* Set value for client authentication. */
                endpoint->usePasswordAuth = secprofile->passwordauth;
            } else {
                endpoint->secure = 0;
            }
        }


        /* Set up socket and listen for new connections */
        portSocket = createSocket(endpoint->ipaddr, endpoint->port, endpoint->name);
        if (portSocket == -1) {
            /*
             * If we just closed the listen socket, wait a bit and then retry once.
             * Even with aocket reuse specified, sometimes the listen socket is in a state
             * where it will not close.  If this single retry does not work, the user
             * must do the retry.
             */
            if (reOpen) {
                ism_common_sleep(10000);
                portSocket = createSocket(endpoint->ipaddr, endpoint->port, endpoint->name);
            }
            if (portSocket == -1) {
                char * xbuf = alloca(4096);
                endpoint->rc = ism_common_getLastError();
                ism_common_formatLastError(xbuf, 4096);
                LOG(ERROR, Transport, 1102, "%-s%d%-s%d%-s%d", "Unable to start TCP endpoint: Endpoint={0} Port={1} Error={2} RC={3} TcpError={4} Errno={5}.",
                        endpoint->name, endpoint->port, xbuf, endpoint->rc, strerror(errno), errno);
                if (endpoint->sslCTX) {
                    ism_common_destroy_ssl_ctx(endpoint->sslCTX1);
                    endpoint->sslCTX = NULL;
                }
                endpoint->enabled = 0;
                return endpoint->rc;
            }
        }
        endpoint->sock = portSocket;
        TRACE(5, "Start TCP endpoint: name=%s transport=%s port=%u secure=%u secprof=%s\n", endpoint->name,
                endpoint->transport_type, endpoint->port, endpoint->secure,
                endpoint->secprof ? endpoint->secprof : "");

        memset(&event, 0, sizeof(epoll_event));
        event.data.ptr = endpoint;
        event.events = EPOLLIN | EPOLLRDHUP;
        if (epoll_ctl(conListener->efd, EPOLL_CTL_ADD, endpoint->sock, &event) == -1) {
            ism_common_setError(ISMRC_EndpointSocket);
            endpoint->rc = ISMRC_EndpointSocket;
            if (endpoint->sslCTX) {
                ism_common_destroy_ssl_ctx(endpoint->sslCTX1);
                endpoint->sslCTX = NULL;
            }
            endpoint->enabled = 0;
            return endpoint->rc;
        }
        endpoint->needed = 0;

        /*
         * If we want a TLS context per thread, make them now.
         * If that fails, use the ones we did get
         */
        if (g_ctxPerThread && certprof && endpoint->isInternal != 2 && ism_transport_getPSKCount() == 0) {
            int ctxcount = numOfIOProcs;
            SSL_CTX * * ctxlist = ism_common_calloc(ISM_MEM_PROBE(ism_memory_tls, 2),ctxcount, sizeof (SSL_CTX *));
            ctxlist[0] = endpoint->sslCTX1;
            for (i=1; i<ctxcount; i++) {
                ctxlist[i] = ism_common_create_ssl_ctx(endpoint->name, ism_common_enumName(enum_methods2, secprofile->method),
                    secprofile->ciphers, certprof->cert, certprof->key, 1, secprofile->sslop, endpoint->useClientCert, secprofile->name,
                    ((secprofile->allowedClientsMap) ? checkPrecertifiedClient : NULL), NULL);
                if (ctxlist[1] == NULL)
                    break;
                printf("Make TLS context: endpoint=%s thread=%d\n", endpoint->name, i);
            }
            if (i < ctxcount)
                ctxcount = i;
            endpoint->sslCTX = ctxlist;
            endpoint->sslctx_count = ctxcount;
        }
    }
    return 0;
}


/*
 * Start the transport.
 * This is called once with a list of indexes for configuration entries.
 */
int ism_transport_startTCP(void) {
    xUNUSED int    rc;
    int    i;
    char   threadname[16];

    /*
     * Start the connection thread
     */
    conListener = createIOCThread("tcpconnect");

    /*
     * Start the IO thread
     */
    ioListener = createIOLThread("tcplisten");

    /*
     * Start the delivery thread
     */
    delivery = ism_transport_createDelivery("tcpdelivery");

    /*
     * Start the IO processor threads
     */
    for (i = 0; i < numOfIOProcs; i++) {
        sprintf(threadname, "tcpiop.%u", (uint16_t)i);
        ioProcessors[i] = createIOPThread(threadname, ioListener);
    }

    g_stopped = 0;

    return 0;
}

/*
 * Start IoP thread for Admin
 */
int ism_transport_startAdminIOP(void) {
    /* Start IO process thread for admin */
    char   threadname[16];
    sprintf(threadname, "AdminIOP");
    adminIoProcessor = createIOPThread(threadname, ioListener);
    return 0;
}

/*
 * Close all connections.
 * Go thru the whole list of connections and do a close waiting until
 * there are no more in the list or we reach the maximum retries.
 */
static void closeAllConnections(int notAdmin) {
    ism_connection_t * con;
    int i;
    int activeCons = 0;
    char xbuf[8192];
    static int hasWaited = 0;
    int lastwait = 0;
    int nextlastwait = 0;
    int waittime = hasWaited ? 1 : 60;

    pthread_mutex_lock(&conMutex);
    if (!activeConnections) {
        pthread_mutex_unlock(&conMutex);
        return;
    }
    con = activeConnections;
    while (con) {
        ism_common_setErrorData(ISMRC_ServerTerminating, "%d%s", con->transport->index, con->transport->name);
        if (!notAdmin || (con->transport->listener && !con->transport->listener->isAdmin))
            con->transport->close(con->transport, ISMRC_ServerTerminating, 1, "The connection was closed because the server was shutdown.");
        con = con->conListNext;
        activeCons++;
    }

    TRACE(3, "Close all connection process is initiated for %d connections\n", activeCons);

    for (i = 0; i < waittime; i++) {    /* wait time in seconds */
        uint32_t nonAdmin = 0;
        uint32_t thisProgress = 0;
        pthread_mutex_unlock(&conMutex);
        ism_common_sleep(1000000);    /* 1 second */
        pthread_mutex_lock(&conMutex);
        if (activeConnections == NULL) {
            break;
        }

        con = activeConnections;
        while (con) {
            if (!notAdmin || (con->transport->listener && !con->transport->listener->isAdmin)) {
                nonAdmin++;
                if (SHOULD_TRACE(9)) {
                    xbuf[0] = 0;
                    if (con->transport->dumpPobj)
                        con->transport->dumpPobj(con->transport, xbuf, sizeof xbuf);
                    TRACE(1, "Connection still open at %d seconds: name=%s connect=%u family=%s %s\n",
                        i, con->transport->name, con->transport->index, con->transport->protocol_family, xbuf);
                }
                if (con->transport->dumpPobj)
                    thisProgress += con->transport->dumpPobj(con->transport, NULL, 0);
            }
            con = con->conListNext;

        }
        /*
         * End the loop if we have not closed any more in the last two attempts
         */
        if (nonAdmin == 0 || (nonAdmin == lastwait && nextlastwait == lastwait )) {
            TRACE(1, "nonAdmin=%d lastwait=%d nextlastwait=%d waittime=%d\n", (int)nonAdmin, lastwait, nextlastwait, waittime);
            break;
        }
        nextlastwait = lastwait;
        lastwait = nonAdmin;

        TRACE(1, "Connections still open after %d seconds: count=%u inprocess=%u\n",
                i+1, nonAdmin, thisProgress);
    }

    /*
     * Trace any outstanding connections
     */
    con = activeConnections;
    while (con) {
        if (con->transport->adminCloseConn != 2) {
            xbuf[0] = 0;
            if (con->transport->dumpPobj)
                con->transport->dumpPobj(con->transport, xbuf, sizeof xbuf);
            TRACE(5, "Connection was not closed during TCP transport termination: transport=%p tobj=%p pobj=%p connect=%u protocol=%s name=%s : %s\n",
                    con->transport, con, con->transport->pobj, con->transport->index, con->transport->protocol, con->transport->name, xbuf);
            con->transport->adminCloseConn = 2;
            hasWaited = 1;
        }
        con = con->conListNext;
    }
    pthread_mutex_unlock(&conMutex);

    /*
     * If it took too long to terminate, just send a SIGTERM
     */
    if (activeConnections != NULL && !notAdmin && cleanStore == 0 ) {
        TRACE(5, "Not all connections were closed during TCP transport termination.\n");
        ism_common_shutdown(0);
    }
}

/*
 * Closes all connections.
 */
void ism_transport_closeAllConnections(int bypassAdminEPCheck) {
    /* Close all existing connections */
    TRACE(1, "Close all TCP connections at server termination\n");  /* level 1 to force sync */
    closeAllConnections(bypassAdminEPCheck);
}


/*
 * Terminate the transport
 */
int ism_transport_termTCP(void) {
    int i;

    g_stopped = 1;
    if (chkRcvBuffTimer) {
        ism_common_cancelTimer(chkRcvBuffTimer);
        chkRcvBuffTimer = NULL;
    }

    /* Stop receiving new connection */
    stopIOCThread(conListener);

    /* Close all existing connections */
    TRACE(1, "Close all TCP connections at server termination\n");  /* level 1 to force sync */
    closeAllConnections(0);
    ism_common_sleep(200000);
    if (cleanup_timer) {
        ism_common_cancelTimer(cleanup_timer);
        cleanup_timer = NULL;
    }

    cleanupTimer(0,0,NULL);
    ism_common_sleep(200000);
    cleanupTimer(0,0,NULL);
    stopIOLThread(ioListener);

    /* Stop threads */
    for (i = 0; i < numOfIOProcs; i++) {
        stopIOPThread(ioProcessors[i]);
    }
    for (i = 0; i < numOfIOProcs; i++) {
        ioProcessorThread iopth = ioProcessors[i];
        if (iopth) {
            if (iopth->jobsList[0].jobs)
                ism_common_free(ism_memory_transportBuffers,iopth->jobsList[0].jobs);
            if (iopth->jobsList[1].jobs)
                ism_common_free(ism_memory_transportBuffers,iopth->jobsList[1].jobs);
            ism_common_free(ism_memory_transportBuffers,iopth);
            ioProcessors[i] = NULL;
        }
    }
    /* cleanup for admin IO Processor thread */
    if ( adminIoProcessor ) {
        stopIOPThread(adminIoProcessor);
        ioProcessorThread iopth = adminIoProcessor;
        if (iopth->jobsList[0].jobs)
            ism_common_free(ism_memory_transportBuffers,iopth->jobsList[0].jobs);
        if (iopth->jobsList[1].jobs)
            ism_common_free(ism_memory_transportBuffers,iopth->jobsList[1].jobs);
        ism_common_free(ism_memory_transportBuffers,iopth);
        adminIoProcessor = NULL;
    }
    if (ioListener) {
        ism_common_free(ism_memory_transportBuffers,ioListener);
        ioListener = NULL;
    }
    if (conListener) {
        ism_common_free(ism_memory_transportBuffers,conListener);
        conListener = NULL;
    }
    if (socketsInfo) {
        ism_common_free(ism_memory_transportBuffers,socketsInfo);
        socketsInfo = NULL;
    }
    return 0;
}

/*
 * Send a ping to the client
 */
XAPI void ism_transport_checkClientLiveness(const char* clientID, uint32_t excludeConnection) {
    ism_connection_t * con;
    if (clientID) {
        pthread_mutex_lock(&conMutex);
        for (con = activeConnections; con != NULL; con = con->conListNext) {
            ism_transport_t * transport = con->transport;
            if ((transport->index != excludeConnection) &&
                transport->pobj && transport->checkLiveness &&
                transport->clientID && (strcmp(clientID,transport->clientID) == 0)) {
                xUNUSED int rc = transport->checkLiveness(transport);
            }
        }
        pthread_mutex_unlock(&conMutex);
    }
}

/*
 * Dump the protocol object associated with this transport object.
 * TODO: should this go to a string so we can put it in the trace?
 */
void ism_transport_dumpConnectionPObj(int conID) {
    ism_transport_t * transport = NULL;
    ism_connection_t * con;
    pthread_mutex_lock(&conMutex);
    for (con = activeConnections; con != NULL; con = con->conListNext) {
         if (con->transport->index == conID) {
             transport = con->transport;
             break;
         }
    }
    pthread_mutex_unlock(&conMutex);
    if (transport) {
        if (transport->dumpPobj) {
            char buff[8096];
            transport->dumpPobj(transport,buff,sizeof(buff));
            printf("%s", buff);
        } else {
            printf("Connection %d has pobj of unknown type\n", conID);
        }
    } else {
        printf("Connection %d not found\n", conID);
    }
}

extern ism_transport_t * ism_transport_getPhysicalTransport(ism_transport_t * transport);

void ism_transport_submitAsyncJobRequest(ism_transport_t * transport, ism_transport_AsyncJob_t job, void * param1, uint64_t param2) {
    ism_connection_t * con = ism_transport_getPhysicalTransport(transport)->tobj;
    asyncJobRequest_t * pReq = ism_common_malloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 7),sizeof(asyncJobRequest_t));
    pReq->func = job;
    pReq->transport = transport;
    pReq->param1 = param1;
    pReq->param2 = param2;
    addAsyncRequest(con,pReq);
}



/* *************************************************************************************
 * createOutgoingConnection
 * *************************************************************************************/
static int createOutgoingConnection(ism_transport_t * transport, const struct addrinfo * addr) {
    ism_connection_t * connection = transport->tobj;
    char ipbuf[128];

    pthread_spin_init(&connection->slock, 0);
    connection->outgoing = 1;
    connection->iopth = ioProcessors[transport->tid];
    connection->transport = transport;
    connection->listener = transport->listener;
    connection->maxSendSize = sendSize;

    /* Create the socket */
    int sock = socket(addr->ai_family, addr->ai_socktype | SOCK_CLOEXEC | SOCK_NONBLOCK, addr->ai_protocol);
    int err = errno;
    if (sock < 0) {
        addConnectionToList(connection);
        ism_common_setError(ISMRC_EndpointSocket);
        TRACE(5, "Unable to create socket: connect=%u error=%s (%d)", transport->index, strerror(err), err);
        return ISMRC_EndpointSocket;
    }

    /* Set the socket reusable */
    int flags = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags)) < 0) {
        err = errno;
        addConnectionToList(connection);
        ism_common_setError(ISMRC_EndpointSocket);
        TRACE(5, "Unable to set socket reusable: connect=%u error=%s (%d)", transport->index, strerror(err), err);
        return ISMRC_EndpointSocket;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *) &flags, sizeof(flags))) {
        err = errno;
        ism_common_setError(ISMRC_EndpointSocket);
        TRACE(5, "Unable to set %s socket option: Error=%s RC=%d.", "SO_KEEPALIVE", strerror(err), err);
        addConnectionToList(connection);
        return ISMRC_EndpointSocket;
    }

    connection->socket = sock;
    addConnectionToList(connection);
    __sync_fetch_and_or(&connection->state, ISM_TRANSPORT_CONNECT_IN_PROCESS);
    if(addr->ai_family != AF_UNIX) {
        flags = 1;
        /* Set name in numeric form into the transport object */
        getnameinfo((struct sockaddr *) addr->ai_addr, addr->ai_addrlen, ipbuf, sizeof(ipbuf), NULL, 0, NI_NUMERICHOST);
        transport->client_addr = putIPString(transport, ipbuf);
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *) &flags, sizeof(flags))) {
            LOG(WARN, Transport, 1106, "%s%s%d", "Unable to set {0} socket option: Error={1} RC={2}.", "TCP_NODELAY", strerror(errno), errno);
            return ISMRC_EndpointSocket;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *) &flags, sizeof(flags))) {
            LOG(WARN, Transport, 1106, "%s%s%d", "Unable to set {0} socket option: Error={1} RC={2}.", "SO_KEEPALIVE", strerror(errno), errno);
            return ISMRC_EndpointSocket;
        }
        addConnectionToIOThread(connection);
    } else {
        transport->client_addr = putIPString(transport, ((struct sockaddr_un*)(addr->ai_addr))->sun_path);
    }
    transport->server_addr = transport->client_addr;

    /*
     * Do the connection.
     * As this is non-blocking, it will almost always return EINPROGRESS and continue
     * when the connection is writeable.
     */
    if (connect(sock, addr->ai_addr, addr->ai_addrlen) < 0) {   /* establish a TCP connection */
        err = errno;
        if (err != EINPROGRESS) {
            connection->state = ISM_TRANSPORT_ERROR;
            ism_common_setError(ISMRC_UnableToConnect);
            TRACE(6, "Unable to make outgoing connection: connect=%u server=%s port=%u error=%s (%d)\n",
                    transport->index, transport->client_addr, transport->clientport, strerror(err), err);
            return ISMRC_UnableToConnect;
        }
        if(addr->ai_family == AF_UNIX) {
            addConnectionToIOThread(connection);
        }
//      fprintf(stderr,"createOutgoingConnection: connect=%u tobj=%p server=%s port=%u error=%s (%d)\n",
//                    transport->index, transport->tobj, transport->client_addr, transport->clientport, strerror(err), err);
    } else {
        TRACE(5, "Outgoing connection created successfully: connect=%u transport=%p\n", transport->index, transport);
        if(addr->ai_family == AF_UNIX) {
            addConnectionToIOThread(connection);
        }
        if (__sync_bool_compare_and_swap(&connection->state,ISM_TRANSPORT_CONNECT_IN_PROCESS,
                (ISM_TRANSPORT_CONNECTED | ISM_TRANSPORT_CAN_READ | ISM_TRANSPORT_CAN_WRITE))) {
            transport->write_bytes += transport->tlsWriteBytes;
            transport->read_bytes += transport->tlsReadBytes;
            if (transport->connected) {
                transport->connected(transport, 0);
            }
        } else {
            ism_common_setError(ISMRC_UnableToConnect);
            return ISMRC_UnableToConnect;
        }
    }
    return 0;
}


/*
 * Make an outgoing connection
 */
int ism_transport_connect(ism_transport_t * transport, ism_transport_t * ctransport,
        const char * server, int port, struct ssl_ctx_st * tlsCTX) {
    struct addrinfo hints = {0};
    struct addrinfo * result, * rp;
    struct addrinfo unixinfo = {0};
    struct sockaddr_un sockunix;
    int  rc;
    char portstr[10];
    transport->tobj = (struct ism_transobj *)ism_transport_allocBytes(transport, sizeof(ism_connection_t), 1);
    memset(transport->tobj, 0, sizeof(ism_connection_t));
    if (transport->originated == 0) {
        transport->originated = 1;
        transport->clientport = port;
        transport->serverport = port;
    }
    transport->send = sendBytes;
    transport->close = close_callback;
    transport->closed = closed_callback;
    transport->tobj->tlsCTX = tlsCTX;

    /*
     * The frame is added as part of transport.
     */
    if (!strcmp(transport->protocol_family, "mqtt")) {
        transport->frame = frameMqtt;
        transport->addframe = ism_transport_addMqttFrame;
    } else if (!strcmp(transport->protocol_family, "plugin")) {
        transport->frame = framePlugin;
        transport->addframe = addPluginFrame;
    } else if (!strcmp(transport->protocol_family, "fwd")) {
        transport->frame = framePlugin;
        transport->addframe = addFwdFrame;
    } else if (!strcmp(transport->protocol, "adminClient")) {
        transport->frame = frameAdminClient;
        transport->addframe = addAdminClientFrame;
    } else {
        ism_common_setError(ISMRC_NotImplemented);
        return -1;
    }

    /*
     * An outgoing connection is run in the same IOP thread as the incoming connection
     * but we use a separate index so we can tell them apart in the trace.  However if there
     * are an even number of IOP threads we double increment the counter for outgoing
     * connection so that the connections are evenly distributed among the IOP threads.
     */
    if (numOfIOProcs & 1)
        transport->tobj->id = __sync_add_and_fetch(&conCounter, 1);
    else
        transport->tobj->id = __sync_add_and_fetch(&conCounter, 2);
    transport->index = transport->tobj->id;
    /* If this comes from a client transport, use the same thread */
    if (ctransport) {
        transport->tid = ctransport->tid;
    } else {
        if (transport->originated != 2)
            transport->tid = transport->index % numOfIOProcs;
    }

    /*
     * Unix domain socket
     */
    if (server && *server == '/') {
        unixinfo.ai_family = AF_UNIX;
        unixinfo.ai_socktype = SOCK_STREAM;
        unixinfo.ai_protocol = 0;
        sockunix.sun_family = AF_UNIX;
        ism_common_strlcpy(sockunix.sun_path, server, sizeof (sockunix.sun_path));
        unixinfo.ai_addrlen = SUN_LEN(&sockunix);
        unixinfo.ai_addr = (struct sockaddr *)&sockunix;
        result = &unixinfo;
    } else {
        hints.ai_family = AF_INET6;                 /* Return IPv6 choices */
        hints.ai_socktype = SOCK_STREAM;            /* We want a TCP socket */
        hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;  /* All interfaces, with IPv4 mapped to IPv6 */
        sprintf(portstr, "%d", port);

        /*
         * Allow the IP address to have a surrounding brackets.
         * This is normally used for IPv6 but we actually allow it for anything.
         */
        if (server && *server == '[') {
            int  iplen = strlen(server);
            if (iplen > 1) {
                char * newip = alloca(iplen);
                strcpy(newip, server+1);
                if (newip[iplen-2] == ']')
                    newip[iplen-2] = 0;
                server = newip;
            }
        }

        /*
         * Check for a valid IP address
         */
        rc = getaddrinfo(server, portstr, &hints, &result);
        if (rc != 0) {
            ism_common_setError(ISMRC_IPNotValid);
            return -1;
        }
    }

    /*
     * Find one which we can open a socket on.
     */
    for (rp = result; rp != NULL; rp = rp->ai_next) {       /* BEAM suppression: loop doesn't iterate */
        rc = createOutgoingConnection(transport, rp);
        /* TODO: error handling */
        break;
    }
    if (rc && transport->connected) {
        transport->write_bytes += transport->tlsWriteBytes;
        transport->read_bytes += transport->tlsReadBytes;
        transport->connected(transport, rc);
    }

    if (result != &unixinfo)
        freeaddrinfo(result);

    return rc;
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


/*
 * Set the nolog config
 */
int ism_transport_setNoLog(const char * nolog) {
    char * temp;
    uint32_t nolog_list[64] = {0};
    uint32_t ipaddr;
    uint32_t ipaddr2;
    struct in_addr inaddr = {0};
    struct in_addr inaddr2 = {0};
    int      count = 0;
    int scope = 32;
    char * token;
    char * more;
    char * eos;
    uint32_t mask;
    struct addrinfo hints = {0};
    struct addrinfo * result;
    struct addrinfo * rp;

    if (nolog == NULL) {
        g_nolog_count = 0;
        return 0;
    }

    temp = alloca(strlen(nolog)+1);
    strcpy(temp, nolog);
    more = temp;
    while ((token = ism_common_getToken(more, " \t,", " \t,", &more))) {
        char * pos = strchr(token, '-');
        if (pos) {
            *pos = 0;
            if (inet_convert(token, &ipaddr)) {
                *pos = '-';   /* hyphen is allowed in an IP address */
                pos = NULL;
            } else {
                if (inet_convert(pos+1, &ipaddr2))
                    scope = -1;
                ipaddr = ntohl(ipaddr);
                ipaddr2 = ntohl(ipaddr2);
                if (ipaddr > ipaddr2)
                    scope = -1;
                inaddr.s_addr = ntohl(ipaddr);
                inaddr2.s_addr = ntohl(ipaddr2);
            }
        }
        if (!pos) {
            pos = strchr(token, '/');
            if (pos) {
                 scope = strtoul(pos+1, &eos, 10);
                 if (*eos)
                     scope = -1;   /* error */
                 *pos = 0;
            } else {
                scope = 32;
            }

            hints.ai_family = AF_INET;                  /* Return IPv6 choices */
            hints.ai_socktype = SOCK_STREAM;            /* We want a TCP socket */
            if (getaddrinfo(token, NULL, &hints, &result)) {
                TRACE(3, "An ipaddr is not valid for nolog: {%s}\n", token);
                scope = -1;
            }
            ipaddr = 0;
            for (rp = result; rp; rp = rp->ai_next) {
                if (rp->ai_family == AF_INET) {
                    memcpy(&ipaddr, &(((struct sockaddr_in *)rp->ai_addr)->sin_addr), 4);
                    break;
                }
            }
            freeaddrinfo(result);
            if (scope >=8 && scope <= 32) {
                mask = 0xffffffff << (32-scope);
                ipaddr = ntohl(ipaddr) & mask;
                ipaddr2 = ipaddr + ~mask;
                inaddr.s_addr = ntohl(ipaddr);
                inaddr2.s_addr = ntohl(ipaddr2);
            }
        }
        if (scope < 8 || scope > 32 || count >= 32) {
            char xbuf[1024];
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ConnectionLogIgnore", nolog);
            ism_common_formatLastError(xbuf, sizeof xbuf);
            LOG(WARN, Server, 930, "%-s%u", "ConectionLogIgnore is not set because the value is not valid: Error={0} Code={1}",
                             xbuf, ism_common_getLastError());
            return 1;
        } else {
            char zbuf [64];
            nolog_list[count*2] = ipaddr;
            nolog_list[count*2+1] = ipaddr2;
            count++;
            ism_common_strlcpy(zbuf, inet_ntoa(inaddr2), sizeof zbuf);
            TRACE(4, "Do not log connections from: %s-%s\n", inet_ntoa(inaddr), zbuf);
        }
    }
    g_nolog_count = count;
    if (count) {
        memcpy(g_nolog_list, nolog_list, count*2*sizeof(uint32_t));
    }
    return 0;
}


XAPI int  ism_transport_getSocketInfo(ism_transport_t * transport, ismSocketInfoTcp *sockInfo) {
    socklen_t length = sizeof(struct tcp_info);
    ism_connection_t * con = transport->tobj;

    if ( getsockopt( con->socket, SOL_TCP, TCP_INFO, (void *)&sockInfo->tcpInfo, &length) == 0 ) {
        sockInfo->validInfo = 1;
    }
    if(ioctl(con->socket, SIOCINQ, &sockInfo->siocinq) != -1) {
        sockInfo->validInfo |= 2;
    }
    if(ioctl(con->socket, SIOCOUTQ, &sockInfo->siocoutq) != -1) {
        sockInfo->validInfo |= 4;
    }
    return 0;
}


/**
 * Print transport statistics
 */
XAPI void ism_transport_printStatsTCP(char ** params, int paramsCount ) {
    ism_connect_mon_t * moncon = NULL;
    ism_connect_mon_t * transport;
    int  position = 0;
    int  verbose = 0;
    int  count;
    int  i;
    const char * clientID = NULL;
    const char * clientIP = NULL;
    uint16_t     minPort = 0;
    uint16_t     maxPort = 0xffff;

    for(i = 0; i < paramsCount; i++) {
        char * param = params[i];
        if (param && strlen(param)>9 && !memcmp(param, "clientID=", 9)) {
            clientID = param+9;
            continue;
        }
        if (param && strlen(param)>9 && !memcmp(param, "clientIP=", 9)) {
            clientIP = param+9;
            continue;
        }
        if (param && strlen(param)>9 && !memcmp(param, "minPort=", 8)) {
            minPort = atoi(param+8);
            continue;
        }
        if (param && strlen(param)>9 && !memcmp(param, "maxPort=", 8)) {
            maxPort = atoi(param+8);
            continue;
        }
        if (param && *param=='v') {
            verbose = 1;
        }
    }
    count = ism_transport_getConnectionMonitor(&moncon, 100, &position, 0, clientID, NULL, NULL,
            NULL, clientIP, minPort, maxPort, ISM_INCOMING_CONNECTION | ISM_OUTGOING_CONNECTION);
    transport = moncon;
    for (i=0; i<count; i++) {
        if (verbose) {
        printf("Connection id=%d name=%s from=%s readbytes=%llu readmsg=%llu writebytes=%llu writemsg=%llu sendQueueSize=%d isSuspended=%d"
               "socketInfo: tcpi_unacked=%u tcpi_lost=%u tcpi_last_data_sent=%u tcpi_last_ack_sent=%u tcpi_last_data_recv=%u "
               "tcpi_last_ack_recv=%u tcpi_rtt=%u tcpi_rttvar=%u tcpi_snd_cwnd=%u tcpi_reordering=%u tcpi_total_retrans=%u siocinq=%u siocoutq=%u\n",
                transport->index, transport->name, transport->client_addr, (ULL) transport->read_bytes,
                (ULL) transport->read_msg, (ULL) transport->write_bytes, (ULL) transport->write_msg,
                transport->sendQueueSize, transport->isSuspended, transport->socketInfoTcp.tcpInfo.tcpi_unacked,
                transport->socketInfoTcp.tcpInfo.tcpi_lost, transport->socketInfoTcp.tcpInfo.tcpi_last_data_sent,
                transport->socketInfoTcp.tcpInfo.tcpi_last_ack_sent, transport->socketInfoTcp.tcpInfo.tcpi_last_data_recv,
                transport->socketInfoTcp.tcpInfo.tcpi_last_ack_recv, transport->socketInfoTcp.tcpInfo.tcpi_rtt,
                transport->socketInfoTcp.tcpInfo.tcpi_rttvar, transport->socketInfoTcp.tcpInfo.tcpi_snd_cwnd,
                transport->socketInfoTcp.tcpInfo.tcpi_reordering, transport->socketInfoTcp.tcpInfo.tcpi_total_retrans,
                transport->socketInfoTcp.siocinq, transport->socketInfoTcp.siocoutq);
        } else {
            printf("Connection id=%d name=%s from=%s readbytes=%llu readmsg=%llu writebytes=%llu writemsg=%llu\n",
                transport->index, transport->name, transport->client_addr, (ULL) transport->read_bytes,
                (ULL) transport->read_msg, (ULL) transport->write_bytes, (ULL) transport->write_msg);
        }
        transport++;
    }
    if (count == 0) {
        printf("There are no connections that match: clientID=%s clientIP=%s portRange=[%u %u]\n",
                ((clientID)? clientID : ""),((clientIP)? clientIP : ""),minPort, maxPort);
    } else {
        ism_transport_freeConnectionMonitor(moncon);
    }

}
