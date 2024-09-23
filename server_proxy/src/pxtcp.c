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
#include <pxtransport.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "pxtcp.h"
#include <openssl/opensslv.h>
#include <openssl/safestack.h>
#include <openssl/tls1.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <dlfcn.h>
#include <log.h>
#include <sys/resource.h>
#include <tenant.h>
#include <alloca.h>
#include <throttle.h>
#include <malloc.h>
#include <sched.h>
#define STR(s) XSTR(s)
#define XSTR(s) #s
#define htonll(x) __builtin_bswap64(x)
#define ntohll(x) __builtin_bswap64(x)
#define BILLION        1000000000L
#define MAX_IOP_NUM    128
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
 * TLS methods enumeration
 */
static ism_enumList enum_methods [] = {
#if OPENSSL_VERSION_NUMBER < 0x10101000L
    { "Method",    4,                 },
#else
    { "Method",    5,                 },
#endif
    { "TLSv1",     SecMethod_TLSv1,   },
    { "TLSv1.1",   SecMethod_TLSv1_1, },
    { "TLSv1.2",   SecMethod_TLSv1_2, },
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
    { "TLSv1.3",   SecMethod_TLSv1_3, },
#endif
    { "None",      0,                 },
};

/*
 * Define the tcp specific extension to the transport object
 */
typedef struct ism_transobj {
    ism_endpoint_t *      endpoint;            /* Endpoint object                            */
    ism_transport_t *     transport;           /* Pointer back to parent transport object    */
    ism_byteBuffer        sendBuffer;
    ism_byteBuffer        sndQueueHead;
    ism_byteBuffer        sndQueueTail;
    ism_byteBuffer        rcvBuffer;
    pthread_spinlock_t    slock;               /* Spin lock for other fields                  */
    int                   stopped;
    int                   socket;              /* Socket file descriptor                      */
    ioProcessorThread     iopth;
    ioListenerThread      iolth;
    uint64_t              id;                  /* Non-wrapping ID                             */
    volatile uint16_t     state;
    uint8_t				  outgoing;
    uint8_t				  mu_log;              /* Log setting for fair use policy */
    int                   needBytes;
    uint8_t               secured;
    uint8_t               isProcessing;
    uint8_t               sledgecount;
    uint8_t               resvi[5];
    struct ism_transobj * conListNext;
    struct ism_transobj * conListPrev;
    struct ism_transobj * iopNext;
    struct ism_transobj * iolNext;
    const char *          servername;          /* Set SNI */
    ism_timer_t           sledgetimer;
    SSL *                 ssl;                 /* SSL/TLS openSSL context                     */
    struct ssl_ctx_st *   tlsCTX;
    BIO *                 bio;                 /* openSSL basic IO context                    */
    BIO *                 bio1;
    char *                bio1DataPtr;
    ism_server_t *		  server;
    asyncJobRequest_t   * asyncJobRequestsHead;
    asyncJobRequest_t   * asyncJobRequestsTail;

    /* Throttle values - This is all handled in the IOP thread so no lock or atomics are required */
    double             restart_time;       /* The time to resume reading from the connection */
    double             reset_time;         /* Start of current 1 second window (zero is not yet set) */
    double             pending_time;       /* Pending count update if message comes before this time */
    double             con_mups;           /* message units per second */
    double             con_mups_max;       /* max message units per second */
    double             mu_total;           /* The total message units for this connection */
    uint32_t           mu_size;            /* The message unit size */
    uint32_t           mu_count;           /* Statistic for number of seconds we exceeded the mups */
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
    char                 eyecatcher [4];
    int                  which;
    pthread_spinlock_t   lock;
    pthread_mutex_t      mutex;
    pthread_cond_t       cond;
    volatile int         isStopped;
    volatile uint32_t    connectionCount;
    ism_byteBufferPool   bufferPool;
    ism_byteBuffer       recvBuffer;
    iopJobsList          jobsList[2];
    iopJobsList        * currentJobsList;
    iopJobsList        * nextJobsList;
    ism_threadh_t        thread;
    ioListenerThread     iolth;
} ioProcessorThread_t;

/*
 * Define a connection job for the IO processor thread
 */
typedef struct ioConnectionJob {
    struct ioConnectionJob * next;
    ism_endpoint_t *     endpoint;
    int                  socket;
    socklen_t            in_len;
    struct sockaddr_in6  in_addr;
} ioConnectionJob;

/*
 * Define information for the IO endpoint thread
 */
typedef struct ioListenerThread_t {
    char                 eyecatcher [4];
    int                  which;
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
    char           eyecatcher [4];
    int            which;
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

int ism_transport_noLog(const char * client);
int ism_ssl_needCRL(ima_transport_info_t * transport, const char * org, X509 * cert);
extern int ism_ssl_SNI_init(void);
extern void ism_proxy_updateAuth(ism_time_t timestamp);
extern void ism_pxact_disconnect(ism_transport_t * transport, int rc);
void ism_transport_closeAllConnections(int notAdmin, int notKafka);
extern int ism_ssl_stopCrlWait(ism_transport_t * transport, const char * org);
void ism_common_traceSSLError(const char * errMsg, const char * file, int line);
const char * ism_transport_makepw(const char * data, char * buf, int len, int dir);
static void saveAsIPString(const char * ipIn, char * ipOut);

/*
 * Global variables
 */
int sendSize;
static int recvSize;
static int iopDelay;
static socketInfo_t   * socketsInfo = NULL;
static int              maxSocketId = 0;
static int              allocSocketId = 0;
#define ACTIVE_CONNECTION_MAX_DEFAULT 2000000
static pthread_mutex_t conMutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t g_nolog_list [64];
static uint32_t g_nolog_count = 0;
static ism_connection_t * activeConnections = NULL;
static ism_connection_t * closedConnections = NULL;
static uint64_t   conCounter = 0;
static int      incomingConnectionsMax;
static int      useSpinLocks = 0;
ismHashMap *               g_org2sslCTXMap = NULL;
static int                 tcpMaxCon;
static ioConnectionThread  conListener = NULL;
static ioListenerThread    ioListener = NULL;
static ioProcessorThread * ioProcessors = NULL;
static ioProcessorThread   monitorIoProcessor = NULL;
static int                 numOfIOProcs = 0;
static volatile int        g_stopped = 1;
static uint64_t            maxPoolSizeBytes;
static ism_timer_t         cleanup_timer = NULL;
static ism_timer_t         ddos_timer = NULL;
static ism_timer_t         nullmsg_timer = NULL;
static ism_timer_t         chkRcvBuffTimer = NULL;
       int                 g_bigLog = 0;
extern uint64_t            g_metering_delta;
static int                 checkServerCert = 0;
static const char* 		   allowExpiredCertOrg;
static int                 useLCPolicy = 0;
       int                 g_isBridge = 0;
       int				   g_tlsseclevel=-1;

/*
 * Stats
 */
static px_tcp_stats_t tcpStats = {0};

/*
 * Forward declarations
 */
HOT static int sendBytes(ism_transport_t * transp, char * buf, int len, int protval, int flags);
static int createTlsObjects(ism_transport_t * transport, const char * data, int datalen);
static int addConnectionJob(ioListenerThread_t * iolth, ioConnectionJob * conJob);
static int inet_convert(const char * src, uint32_t * dst);
static int createOutgoingConnection(ism_transport_t * transport, ism_server_t * server, int port, ioProcessorThread iopth);
static int connectionCloseComplete(ism_connection_t * con, int reuse);
static int ism_tcp_fairuse(ism_transport_t * transport, int request, int val1, int val2);
int ism_kafka_term(void);
extern void ism_tenant_term(void);

/*
 * Thunk to get g_stopped
 */
int ism_transport_stopped(void) {
    return g_stopped;
}

/*
 * Get the next connection counter
 */
int ism_transport_nextConnectID(void) {
    return __sync_add_and_fetch(&conCounter, 1);
}

uint64_t ism_transport_getConnId(ism_transport_t * transport) {
	return transport ? (transport->tobj ? transport->tobj->id : 0) : 0;
}	

/*
 * Return the SSL object from a transport object
 */
struct ssl_st * ism_transport_getSSL(ism_transport_t * transport) {
    return transport->tobj->ssl;
}

extern int ism_ssl_setSniCtx(SSL *ssl, const char * sniName, int * requireClientCert, int * updated);

static int getSNIname(SSL *ssl, char * sniName, int maxLen) {
    const char * servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    if (servername && (servername[0] != '\0') && (servername[0] != '.')) {
        const char * ptr = strchr(servername, '.');
        if (ptr) {
            size_t len = ptr - servername;
            if (len > (maxLen-1))
                len = (maxLen-1);
            memcpy(sniName, servername, len);
            sniName[len] = '\0';
        } else {
            strncpy(sniName, servername, maxLen);
        }
        return 1;
    }
    return 0;
}

/*
 * TLS SNI extension server name callback.
 */
static int ssl_servername_cb(SSL *ssl, int *ad, void *arg) {
	if (ssl){
	    char sniName[64];
	    int rc = SSL_TLSEXT_ERR_OK;
	    if (getSNIname(ssl, sniName, sizeof(sniName))) {
	        int requireClientCert = 0;
	        int updated = 0;
	        ism_transport_t * transport = (ism_transport_t *)SSL_get_app_data(ssl);
	        rc = ism_ssl_setSniCtx(ssl, sniName, &requireClientCert, &updated);
            if (transport) {
                transport->sniName =  ism_transport_putString(transport, sniName);
                transport->requireClientCert = requireClientCert;
                transport->crlStatus = CRL_STATUS_OK;
                transport->usedSNI = updated;
            }
	    }
        return rc;
	}
    return SSL_TLSEXT_ERR_NOACK;
}

#if 0
xUNUSED static int verify_ssl_cert_cb(int preverify_ok, X509_STORE_CTX *x509_ctx) {
	SSL * ssl = X509_STORE_CTX_get_ex_data(x509_ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
	ism_transport_t * transport = (ism_transport_t *)SSL_get_app_data(ssl);
	if (!preverify_ok) {
		int err = X509_STORE_CTX_get_error(x509_ctx);
		if (err != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) {
            if (SHOULD_TRACE(5)) {
    			char buf[512];
    			int depth = X509_STORE_CTX_get_error_depth(x509_ctx);
    			X509   *cert = X509_STORE_CTX_get_current_cert(x509_ctx);
    			X509_NAME_oneline(X509_get_subject_name(cert), buf, sizeof(buf));
                TRACE(6, "Certificate verification failed: From=%s:%u connect=%u depth=%d error=%s\n%s\n",
                        transport->client_addr, transport->clientport, transport->index, depth,
                        X509_verify_cert_error_string(err), buf);
            }
			transport->invalidSslCert = 1;
		}
	}
	return 1;
}
#endif

/*
 * Return the number of IOPs
 */
int ism_tcp_getIOPcount(void) {
    return numOfIOProcs;
}

/*
 * Reset the socket info
 */
static void resetSocketInfo(int id, int inUse, uint32_t maxSendSize, uint32_t maxRecvSize) {
    pthread_spin_lock(&socketsInfo[id].lock);
    socketsInfo[id].inUse = inUse;
    socketsInfo[id].rcvBufAtMax = 0;
    socketsInfo[id].sndBufAtMax = 0;
    socketsInfo[id].maxRecvSize = maxRecvSize;
    socketsInfo[id].maxSendSize = maxSendSize;
    pthread_spin_unlock(&socketsInfo[id].lock);
}

XAPI int ism_transport_setMaxSocketBufSize(ism_transport_t * transport, int maxSendSize, int maxRecvSize) {
    if (transport->virtualSid == 0 && transport->tobj) {
        int sock = transport->tobj->socket;
        if (sock != 0) {
            pthread_spin_lock(&socketsInfo[sock].lock);
            if (socketsInfo[sock].inUse) {
                socketsInfo[sock].rcvBufAtMax = 0;
                socketsInfo[sock].sndBufAtMax = 0;
                socketsInfo[sock].maxRecvSize = maxRecvSize;
                socketsInfo[sock].maxSendSize = maxSendSize;
            }
            pthread_spin_unlock(&socketsInfo[sock].lock);
        }
    }
    return 0;
}


/*
 * Create and set the server socket
 */
static int createSocket(const char * ipAddr, int port, const char * name) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int rc;
    int fd = -1;
    int32_t flags;
    char portstr[10];
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;                 /* Return IPv6 choices */
    hints.ai_socktype = SOCK_STREAM;            /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;  /* All interfaces, with IPv4 mapped to IPv6 */


    if (ipAddr && (strcmpi(ipAddr, "All") == 0)) {
        ipAddr = NULL;
    }
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

            TRACE(3, "Unable to bind socket: port=%d rc=%d error=%s\n", port, rc, strerror(rc));
            close(fd);
            fd = -1;
            continue;
        }
        inet_ntop(rp->ai_family, (void*) &(((struct sockaddr_in *) rp->ai_addr)->sin_addr), ipstr, sizeof ipstr);
        TRACE(5, "TCP socket created for endpoint %s, bound to [%s]:%d\n", name, ipstr, port);

        /*
         * Listen on the port
         */
        rc = listen(fd, tcpMaxCon);
        if (rc == -1) {
            int save_errno = errno;
            ism_common_setError(ISMRC_EndpointSocket);
            /* TODO: log */
            TRACE(3, "Failure in socket listen: endpoint=%s port=%d error=%s errno=%d\n",
                name, port, strerror(save_errno), save_errno);
            close(fd);
            fd = -1;
            errno = save_errno;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    return fd;
}

/*
 * Accept a new connection.
 * This is called within the connection thread to request the connection be done
 * in an IO processing thread.
 */
static int acceptNewConnection(ism_endpoint_t * endpoint) {
    struct sockaddr_in6 in_addr;
    socklen_t in_len;
    int n1;
    int ifd;
    ioConnectionJob * conJob = NULL;

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
    n1 = __sync_add_and_fetch(&tcpStats.incomingConnectionsCounter, 1);
    __sync_add_and_fetch(&tcpStats.incomingConnectionsTotal, 1);
    if (n1 > incomingConnectionsMax) {
        /* Allow admin connections anyway */
        if (endpoint->protomask != PMASK_AnyInternal) {
            char * ipbuf = alloca(256);
            *ipbuf = 0;
            getnameinfo((struct sockaddr *) &in_addr, sizeof(in_addr), ipbuf, 256, NULL, 0, NI_NUMERICHOST);
            if (strlen(ipbuf) >=  7 && !memcmp(ipbuf, "::ffff:", 7))        /* BEAM suppression: uninitialized */
                ipbuf += 7;
            uint16_t clientPort;
            if (((struct sockaddr *) &in_addr)->sa_family == AF_INET6) {
                clientPort = htons(in_addr.sin6_port);
            } else {
                clientPort = htons(((struct sockaddr_in * ) &in_addr)->sin_port);
            }
            LOG(ERROR, Transport, 1119, "%-s%s%u", "Closing TCP connection because there are too many active connections. Endpoint={0} From={1}:{2}.",
                   endpoint->name, ipbuf, clientPort);
            __sync_sub_and_fetch(&tcpStats.incomingConnectionsCounter, 1);        /* Not an active connection */
            __sync_add_and_fetch(&endpoint->stats->bad_connect_count, 1);     /* Increment endpoint bad connection count */
            close(ifd);
            return 0;
        }
    }

    /*
     * Schedule a job to complete the connection
     */
    conJob = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,1),sizeof(ioConnectionJob));
    memcpy(&conJob->in_addr,&in_addr,in_len);
    conJob->in_len = in_len;
    conJob->endpoint = endpoint;
    conJob->socket = ifd;
    addConnectionJob(ioListener, conJob);
    return ifd;
}

#undef TRACE_DOMAIN
#define TRACE_DOMAIN transport->trclevel

/*
 * Include the core framing support including the JMS and MQTT framing support
 */
#include "pxframe.c"

/*
 * Special jobs for cleanup and shutdown
 */
#define SHUTDOWN_REQUEST 0x0000000100000000
#define CLEANUP_REQUEST  0x0000000200000000
#define SHUTDOWN_FORCE   0x0000000400000000
#define DELAY_DONE       0x0000000500000000

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
            if (pSI->sndBufAtMax) {
                pthread_spin_unlock(&pSI->lock);
                return 0;
            }
            if (getsockopt(sock,SOL_SOCKET,SO_SNDBUF, &currSize,&optlen)) {
                    err = errno;
                    pSI->sndBufAtMax = 1;
                    pthread_spin_unlock(&pSI->lock);
                    TRACEL(6, ism_defaultTrace, "increaseSockBufSize(%d, %s): getsockopt failed with error %d(%s)\n", sock,"SO_SNDBUF", err, strerror(err));
                    return 0;
            }
            if ((pSI->maxSendSize != 0) && (currSize >= pSI->maxSendSize)) {
                pSI->sndBufAtMax = 1;
                pthread_spin_unlock(&pSI->lock);
                TRACEL(6, ism_defaultTrace, "increaseSockBufSize(%d, %s): buffer size is already at maximum (%u)\n", sock,"SO_SNDBUF", pSI->maxSendSize);
                return 0;
            }
            pMax = &pSI->sndBufAtMax;
        } else {
            int inUse = 0;
            if (pSI->rcvBufAtMax) {
                pthread_spin_unlock(&pSI->lock);
                return 0;
            }
            if ((pSI->maxRecvSize != 0) && (currSize >= pSI->maxRecvSize)) {
                pSI->rcvBufAtMax = 1;
                pthread_spin_unlock(&pSI->lock);
                TRACEL(6, ism_defaultTrace, "increaseSockBufSize(%d, %s): buffer size is already at maximum (%u)\n", sock,"SO_RCVBUF", pSI->maxRecvSize);
                return 0;
            }
            if (getsockopt(sock,SOL_SOCKET,SO_RCVBUF, &currSize,&optlen) || ioctl(sock, SIOCINQ, &inUse)) {
                 err = errno;
                 pSI->rcvBufAtMax = 1;
                 pthread_spin_unlock(&pSI->lock);
                 TRACEL(6, ism_defaultTrace, "increaseSockBufSize(%d, %s): get socket info failed with error %d(%s)\n", sock,"SO_RCVBUF", err, strerror(err));
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
            TRACEL(6, ism_defaultTrace, "increaseSockBufSize(%d, %s): setsockopt failed with error %d(%s)\n", sock,((bufType == SO_SNDBUF) ? "SO_SNDBUF" : "SO_RCVBUF"), err, strerror(err));
            return 0;
        }
        if (getsockopt(sock,SOL_SOCKET,bufType, &currSize,&optlen)) {
            err = errno;
            *pMax = 1;
            pthread_spin_unlock(&pSI->lock);
            TRACEL(6, ism_defaultTrace, "increaseSockBufSize(%d, %s): getsockopt failed with error %d(%s)\n", sock,((bufType == SO_SNDBUF) ? "SO_SNDBUF" : "SO_RCVBUF"), err, strerror(err));
            return 0;
        }
        if (currSize < newSize) {
            *pMax = 1;
            pthread_spin_unlock(&pSI->lock);
            TRACEL(6, ism_defaultTrace, "increaseSockBufSize(%d, %s): buffer size value is less than requested %d < %d\n", sock,((bufType == SO_SNDBUF) ? "SO_SNDBUF" : "SO_RCVBUF"), currSize, newSize);
            return 0;
        }
    }
    pthread_spin_unlock(&pSI->lock);
    return 0;
}

/*
 * Timer task to periodically check if receive buffer is full and increase it if possible
 */
static int conRcvBufCheckTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    int sock;
    int maxsocket;

    if (socketsInfo && !g_stopped) {
        pthread_mutex_lock(&conMutex);
        maxsocket = maxSocketId;
        pthread_mutex_unlock(&conMutex);
        for (sock = 0; sock < maxsocket; sock++) {
            if (socketsInfo[sock].inUse && (socketsInfo[sock].rcvBufAtMax==0)) {
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
    if (ioth) {
        if (useSpinLocks)
            pthread_spin_lock(&ioth->lock);
        else
            pthread_mutex_lock(&ioth->mutex);
        iopJobsList * jobsList = ioth->currentJobsList;
        if (UNLIKELY(jobsList->used == jobsList->allocated)) {
            jobsList->allocated *= 2;
            jobsList->jobs = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,2),jobsList->jobs, jobsList->allocated * sizeof(ioProcJob));
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
            if (sendSignal) {
                pthread_cond_signal(&ioth->cond);
            }
        }
    }
}
/*
 * Enqueue a null job which only has the affect of waking up the IOP.
 * This guarantees that the IOP loop is run even if no new jobs come in.
 *
 * This is not needed if we are using spinlocks, and does nothing if there
 * are already jobs in the job list.
 */
static void addNullJob4Processing(ioProcessorThread_t * ioth) {
    if (!useSpinLocks) {
        pthread_mutex_lock(&ioth->mutex);
        iopJobsList * jobsList = ioth->currentJobsList;
        if (!jobsList->used) {
            ioProcJob * job = jobsList->jobs;
            job->con = NULL;
            job->events = 0;
            jobsList->used++;
            pthread_mutex_unlock(&ioth->mutex);
            pthread_cond_signal(&ioth->cond);
        } else {
            pthread_mutex_unlock(&ioth->mutex);
        }
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
        TRACE(3, "Unable to add socket to epoll: errno=%d transport=%u", errno, transport->index);
        ism_common_setError(ISMRC_EndpointSocket);
        return -1;
    }
    return 0;
}


/*
 * Remove a transport from an IO precessing thread.
 * This is done by submitting a job to the IO processing thread.
 */
static int removeTransportFromIOThread(ism_connection_t * con) {
    addJob4Processing(con, SHUTDOWN_REQUEST);
    return 0;
}

/*
 * Add a connection to a list of connections
 */
static void addConnectionToList(ism_connection_t * con) {
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
    if (con->socket) {
        uint32_t maxSendSize = (con->outgoing) ? 0 : (16*1024);
        uint32_t maxRecvSize = (con->outgoing) ? 0 : (32*1024);
    	resetSocketInfo(con->socket, 1, maxSendSize, maxRecvSize);
    }
    pthread_mutex_unlock(&conMutex);
}

/*
 * Remove a connection from a list of connections
 */
static void removeConnectionFromList(ism_connection_t * con, int reuse) {
    if (!con->transport->originated && con->iopth) {
        __sync_sub_and_fetch(&con->iopth->connectionCount,1);
    }
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
        if (!reuse) {
            con->conListNext = closedConnections;
            if (closedConnections) {
                closedConnections->conListPrev = con;
            }
            con->conListPrev = NULL;
            closedConnections = con;
            con->state = ISM_TRANSPORT_DISCONNECTED;
            con->transport->state = ISM_TRANST_Closed;
        }
    }
    pthread_mutex_unlock(&conMutex);
}

/*
 * Implements transport->closed for tcp
 */
static int connectionCloseInit(ism_transport_t * transport) {
    TRACE(8, "connectionCloseInit: connect=%u name=%s transport=%p\n", transport->index, transport->name, transport);
    if (transport->tobj->iopth) {
        TRACE(9, "connectionCloseInit: connect=%u iopth=%u\n", transport->index, transport->tobj->iopth->which);
    	return removeTransportFromIOThread(transport->tobj);
    }

    /* This should not ever happen */
    TRACE(1, "Free transport init: %p\n", transport);
    ism_transport_freeTransport(transport);
    return 0;
}

extern ism_logWriter_t * g_logwriter[LOGGER_Max+1];

/*
 * Check if we should log.
 * This is only implemented for IPv4 addresses
 */
int ism_transport_noLog(const char * client) {
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
 * verify callback to implement CRL
 * This is only used when client certs are used
 */
int ism_transport_crlVerify(int good, X509StoreCtx * ctx) {
    int err;
    int rc;
    int ret = good;
    SSL * ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    ism_transport_t * transport = SSL_get_app_data(ssl);

    int depth = X509_STORE_CTX_get_error_depth(ctx);
    err = X509_STORE_CTX_get_error(ctx);
    // printf("verify depth=%d good=%d err=%d\n", depth, good, err);
    if (depth == 0) {      /* We only look at CRL problems in lowest level */
        if (err == X509_V_ERR_UNABLE_TO_GET_CRL) {
            X509 * cert = X509_STORE_CTX_get_current_cert(ctx);
            STACK_OF(DIST_POINT) * cdp = X509_get_ext_d2i(cert, NID_crl_distribution_points, NULL, NULL);
            if (!cdp) {
                X509_STORE_CTX_set_error(ctx, 0);
                transport->crlStatus = CRL_STATUS_NONE;
                ret = 1;    /* Ignore if no CRL available */
            } else {
                sk_DIST_POINT_pop_free(cdp, DIST_POINT_free);
                /* check download and create a waiter */
                rc = ism_ssl_needCRL((ima_transport_info_t *)transport, transport->sniName, cert);
                if (rc == 0) {
                    X509_STORE_CTX_set_error(ctx, 0);
                    transport->crlStatus = CRL_STATUS_NONE;
                    ret = 1;
                } else {
                    ret = 0;
                }
            }
        }else{
        	TRACE(9, "CRL TLS Error Check: depth=%d good=%d err=%d errStr=%s\n", depth, good, err, X509_verify_cert_error_string(err));
        }
    } else {
        if (err) {
            TRACE(9, "verify err depth=%d good=%d err=%d errStr=%s\n", depth, good, err, X509_verify_cert_error_string(err));
        }
    }
    if (!ret) {
    	if(err== X509_V_ERR_CERT_HAS_EXPIRED && allowExpiredCertOrg && transport->sniName && !strcmp(transport->sniName, allowExpiredCertOrg)){
    		TRACE(5, "Reset ret value for Expired Certificate. verify err depth=%d good=%d err=%d errStr=%s\n", depth, good, err, X509_verify_cert_error_string(err));
    		X509_STORE_CTX_set_error(ctx, 0);
			transport->crlStatus = CRL_STATUS_NONE;
			ret = 1;
    	}else{
			ism_common_setErrorData(ISMRC_CertificateNotValid, "%s", X509_verify_cert_error_string(err));
			TRACE(5, "Cert verify failure: connect=%d From=%s:%u error=%s (%d) transport->org=%s\n", transport->index,
					transport->client_addr, transport->clientport, X509_verify_cert_error_string(err), err, transport->sniName);
    	}
    }
    return ret;
}

/*
 * Function to notify the protocol that a connection is closing
 */
static int closeConnectionNotify(ism_transport_t * transport, int rc, int clean, const char * reason) {
    char xbuf [64];

    /*
     * Make sure that closeConnection is only called by one thread.  First one wins.
     */
    if (!transport)
        return 1;
    if (!(__sync_bool_compare_and_swap(&transport->state, ISM_TRANST_Open, ISM_TRANST_Closing)
            || __sync_bool_compare_and_swap(&transport->state, ISM_TRANST_Opening, ISM_TRANST_Closing))) {
        return 1;
    }

    if (!reason)
        reason = "";

    TRACE(8, "closeConnectionNotify: connect=%u(%p) reason=%s\n", transport->index, transport, reason);

    /*
     * Put out a trace that we are closing
     */
    if (!transport->protocol_family || !*transport->protocol_family) {
        if (!ism_transport_noLog(transport->client_addr)) {
            uint32_t uptime = (uint32_t)(((ism_common_currentTimeNanos()-transport->connect_time)+500000000)/1000000000);  /* in seconds */
            /* We have not completed the open */
            if ((!transport->name || !*transport->name) && transport->sniName)
                transport->name = transport->sniName;
            if (ism_common_conditionallyLogged(NULL, ISM_LOGLEV(INFO), ISM_MSG_CAT(Connection), 1116, TRACE_DOMAIN, transport->name, transport->client_addr, reason) ==0){
            	LOG(WARN, Connection, 1116, "%u%-s%d%d%-s%u%llu%llu%s%u",
					"Closing TCP connection during handshake: ConnectionID={0} From={8}:{9} Endpoint={1} RC={2} Clean={3} Reason={4} Uptime={5} ReadBytes={6} WriteBytes={7}.",
					transport->index, transport->endpoint_name, rc, clean, reason, uptime, (ULL)transport->read_bytes, (ULL)transport->write_bytes,
					transport->client_addr, transport->clientport);
            }
        }
        __sync_add_and_fetch(&transport->endpoint->stats->bad_connect_count, 1);
    } else {
        if (transport->endpoint) {
            const char * fmt;
            const char * kinds;
            if (!transport->originated) {
#ifndef NO_PROXY
                char timest [32];   /* Only get the timestamp once */
                timest[0] = 0;
#ifdef PX_CLIENTACTIVITY
                ism_pxact_disconnect(transport, rc);
#endif
                /* If we are monitoring, disconnectMsg will set the timestamp but at millisecond resolution */
                if (transport->ready > 2) {

                    if (!transport->no_monitor) {
                    	ism_proxy_disconnectMsg(transport, rc, reason);
                    }
                } else {
                    if (rc)
                        ism_proxy_failedMsg(transport, rc, reason);
                }
                /* Send a metering message if we have authenticated the connection */
                if (transport->ready > 1) {
                    ism_proxy_meteringMsg(transport, timest);
                }
#endif
            }
            if (*transport->endpoint->name != '!' && !ism_transport_noLog(transport->client_addr)) {
                uint32_t uptime = (uint32_t)(((ism_common_currentTimeNanos()-transport->connect_time)+500000000)/1000000000);  /* in seconds */
                if (!clean || (g_logwriter[LOGGER_Connection] && TRACE_DOMAIN->logLevel[LOGGER_Connection] != AuxLogSetting_Min)) {
                    if (transport->originated) {
                        if (transport->ready) {
                            if (g_bigLog) {
                                fmt = "Closing TCP outgoing connection: ConnectionID={0} ClientID={1} Protocol={2} "
                                      "Endpoint={3} UserID={4} Uptime={5} RC={6} Reason={8} "
                                      "ReadBytes={9} ReadMsg={10} WriteBytes={11} WriteMsg={12} LostMsg={13}.";
                                kinds = "%u%-s%s%-s%-s%u%d%d%-s%llu%llu%llu%llu%llu%llu";
                            } else {
                                fmt = "Closing out: I={0} C={1} T={5} R={6}:{8} S={9},{10},{11},{12},{13}";
                                kinds = "%u%-s%s%-s%-s%u%d%d%s%llu%llu%llu%llu%llu%llu";
                                reason = ism_common_getErrorName(rc, xbuf, sizeof xbuf);
                                if (reason == NULL)
                                    reason = "OK";
                                else if (*reason != 'I')
                                    reason = "Unknown";
                                else
                                    reason += 6;
                            }
                            if (ism_common_conditionallyLogged(NULL, ISM_LOGLEV(INFO), ISM_MSG_CAT(Connection), 1121, TRACE_DOMAIN, transport->name, transport->client_addr, reason) == 0){
                                LOG(INFO, Connection, 1121, kinds, fmt,
                                    transport->index, transport->name, transport->protocol, transport->endpoint->name,
                                    transport->userid ? transport->userid : "", uptime,
                                    rc, clean, reason,
                                    (ULL)transport->read_bytes, (ULL)transport->read_msg,
                                    (ULL)transport->write_bytes,(ULL)transport->write_msg,
                                    (ULL)transport->lost_msg, (ULL)0);
                            }
                        }
                    } else {
                        if (transport->tobj->mu_count && transport->tobj->mu_log) {
                            LOG(WARN, Connection, 1129, "%-s%s%u%u%e%u%s%u%e",
                                    "Fair use policy message rate exceeded: ClientID={0} From={1}:{2} Unit={3} Rate={4} Count={5} Uptime={7} TotalMU={8}{6}",
                                    transport->name, transport->client_addr, transport->clientport,
									transport->tobj->mu_size, transport->tobj->con_mups_max,
									transport->tobj->mu_count, transport->tobj->mu_log==2 ? " Log=only" : "", uptime, transport->tobj->mu_total);
                        }
                        if (g_bigLog) {
                            fmt =  "Closing TCP connection: ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3} "
                                   "From={16}:{17} Secure={18} UserID={4} Uptime={5} RC={6} Reason={8} "
                                   "ReadBytes={9} ReadMsg={10} WriteBytes={11} WriteMsg={12} LostMsg={13}.";
                            kinds = "%u%-s%s%-s%-s%u%d%d%-s%llu%llu%llu%llu%llu%llu%u%s%u%s";
                        } else {
                            fmt = "Closing: I={0} C={1} P={2} E={3} F={16}:{17} U={4} T={5} R={6}:{8} S={9},{10},{11},{12},{13}";
                            kinds = "%u%-s%s%-s%-s%u%d%d%s%llu%llu%llu%llu%llu%llu%u%s%u%s";
                            reason = ism_common_getErrorName(rc, xbuf, sizeof xbuf);
                            if (reason == NULL)
                                reason = "OK";
                            else if (*reason != 'I')
                                reason = "Unknown";
                            else
                                reason += 6;
                        }
                        if (ism_common_conditionallyLogged(NULL, ISM_LOGLEV(NOTICE), ISM_MSG_CAT(Connection), 1111, TRACE_DOMAIN, transport->name, transport->client_addr, reason) == 0){
							LOG(NOTICE, Connection, 1111, kinds, fmt,
								transport->index, transport->name, transport->protocol, transport->endpoint->name,
								transport->userid ? transport->userid : "", uptime,
								rc, clean, reason,
								(ULL)transport->read_bytes, (ULL)transport->read_msg,
								(ULL)transport->write_bytes,(ULL)transport->write_msg,
								(ULL)transport->lost_msg, (ULL)0, 0, transport->client_addr, transport->clientport,
								ism_transport_getTLSMethodName(transport));
                        }
                    }
                } else {
                    TRACE(6, "Closing TCP connection %s (CWLNA1111): ConnectionID=%u ClientID=\"%s\" Protocol=%s Endpoint=\"%s\""
                            " From=%s:%u Secure=%s UserID=\"%s\" Uptime=%u RC=%d Clean=%d Reason\"%s\""
                            " ReadBytes=%llu ReadMsg=%llu WriteBytes=%llu WriteMsg=%llu LostMsg=%llu",
                        transport->originated ? "out" : "in",
                        transport->index, transport->name, transport->protocol, transport->endpoint->name,
                        transport->client_addr, transport->clientport,
                        ism_transport_getTLSMethodName(transport),
                        transport->userid ? transport->userid : "", uptime,
                        rc, clean, reason,
                        (ULL)transport->read_bytes, (ULL)transport->read_msg,
                        (ULL)transport->write_bytes,(ULL)transport->write_msg,
                        (ULL)transport->lost_msg);
                }
            }
            if (!transport->originated && transport->counted)
                __sync_add_and_fetch(&transport->endpoint->stats->connect_active, -1);
        }
    }

    /*
     * Call the protocol
     */
    if (transport->closing != NULL) {
        transport->closing(transport, rc, clean, reason);
    } else {
        connectionCloseInit(transport);
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
        if (err && err != EAGAIN) {
            data = strerror_r(err, mbuf, 1024);
            traceFunction(5, TOPT_WHERE, file, line, "TLS error connect=%u client=%s err=%s errno=%s(%d)\n",
                    transport->index, transport->name, errStr, data, err);
        } else {
            traceFunction(5, TOPT_WHERE, file, line, "TLS error connect=%u client=%s err=%s\n",
                    transport->index, transport->name, errStr);
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
            traceFunction(5, TOPT_WHERE, file, line, "TLS trace connect=%u client=%s err=%s: data=\"%s\"\n",
                    transport->index, transport->name, pos, data);
        } else {
            traceFunction(5, TOPT_WHERE, file, line, "TLS trace connect=%u client=%s err=%s\n",
                    transport->index, transport->name, pos);
        }
    }
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

    /*
     * Create the SSL object
     */
    ssl = SSL_new(transport->endpoint->sslCTX);
    if (!ssl) {
        sslTraceErr(transport, 0, __FILE__, __LINE__);
    } else {
        /*
         * Create the BIO objects
         */
        bio = BIO_new_socket(con->socket, BIO_NOCLOSE);
        if (!bio) {
            sslTraceErr(transport, 0, __FILE__, __LINE__);
        } else {
            if (datalen) {
                bio1DataPtr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,3),datalen);
                if (bio1DataPtr) {
                    memcpy(bio1DataPtr,data,datalen);
                    bio1 = BIO_new_mem_buf((void *)bio1DataPtr, datalen);
                    if (bio1) {
                        BIO_set_mem_eof_return(bio1, -1);
                        rc = BIO_set_close(bio1, BIO_CLOSE);
                    }
                }
                if (!bio1) {
                    sslTraceErr(transport, 0, __FILE__, __LINE__);
                    BIO_free(bio);
                    bio = NULL;
                    if (bio1DataPtr) {
                        ism_common_free(ism_memory_proxy_tcp,bio1DataPtr);
                        bio1DataPtr = NULL;
                    }
                }
            }
        }
    }

    if (!ssl || !bio) {
        if (ssl)
            SSL_free(ssl);
        close(con->socket);
        __sync_sub_and_fetch(&tcpStats.incomingConnectionsCounter, 1);        /* Not an active connection */
        __sync_add_and_fetch(&transport->endpoint->stats->bad_connect_count, 1);     /* Increment endpoint bad connection count */
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
    con->state |= ISM_TRANSPORT_HANDSHAKE_IN_PROCESS;
    return 0;
}


static inline int assignIOP(ism_transport_t * transport) {
    ism_connection_t * con = transport->tobj;

    /* Check and set admin IOP for admin endpoint */
    if (transport->endpoint->port == 9089) {
        transport->tid = numOfIOProcs;
        TRACE(9, "New admin connection From %s:%u was accepted on endpoint %s: connect=%u tobj=%p ssl=%p security_context=%p\n", transport->client_addr,
                transport->clientport, con->endpoint->name, transport->index, transport->tobj, con->ssl, transport->security_context);
        con->iopth = monitorIoProcessor;
        __sync_add_and_fetch(&monitorIoProcessor->connectionCount,1);
        return 0;
    }

    /* Assign to the proc with the least connections */
    if (useLCPolicy) {
        int i;
        uint16_t    maxCount = 0xffff;
        transport->tid = MAX_IOP_NUM;
        for(i = 0; i < numOfIOProcs; i++) {
            if (ioProcessors[i]->connectionCount < maxCount) {
                transport->tid = i;
                maxCount = ioProcessors[i]->connectionCount;
            }
        }
    } else {
        /* Use round robin */
        transport->tid = con->id % numOfIOProcs;
    }
    con->iopth = ioProcessors[transport->tid];
    __sync_add_and_fetch(&con->iopth->connectionCount,1);
    TRACE(8, "New connection From %s:%u was accepted. connect=%u endponit=%s tobj=%p ssl=%p security_context=%p\n", transport->client_addr,
            transport->clientport, transport->index, con->endpoint->name, transport->tobj, con->ssl, transport->security_context);
    return 0;
}

/*
 * Process a work item to do a connection
 */
static int processConnectionRequest(ioConnectionJob * conJob) {
    struct sockaddr_in6 in_addr;
    socklen_t in_len;
    int sock = conJob->socket;
    ism_transport_t * transport;
    ism_connection_t * con;
    char ipbuf[64];
    int flags;

#undef TRACE_DOMAIN
#define TRACE_DOMAIN ism_defaultTrace

    /*
     * Set the TCP nodelay on
     */
    flags = 1;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *) &flags, sizeof(flags))) {
        LOG(WARN, Transport, 1106, "%s%s%d", "Unable to set {0} socket option: Error={1} RC={2}.", "TCP_NODELAY", strerror(errno), errno);
        close(sock);
        __sync_sub_and_fetch(&tcpStats.incomingConnectionsCounter, 1);        /* Not an active connection */
        __sync_add_and_fetch(&conJob->endpoint->stats->bad_connect_count, 1);     /* Increment endpoint bad connection count */
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *) &flags, sizeof(flags))) {
        LOG(WARN, Transport, 1106, "%s%s%d", "Unable to set {0} socket option: Error={1} RC={2}.", "SO_KEEPALIVE", strerror(errno), errno);
        close(sock);
        __sync_sub_and_fetch(&tcpStats.incomingConnectionsCounter, 1);        /* Not an active connection */
        __sync_add_and_fetch(&conJob->endpoint->stats->bad_connect_count, 1);     /* Increment endpoint bad connection count */
        return -1;
    }

    /*
     * Set up the transport object
     */
    transport = ism_transport_newTransport(conJob->endpoint, sizeof(ism_connection_t), (conJob->endpoint->port != 9089));
    transport->frame = ism_transport_InitialHandshake;
    if (((struct sockaddr *) &conJob->in_addr)->sa_family == AF_INET6) {
        transport->clientport = htons(conJob->in_addr.sin6_port);
    } else {
        transport->clientport = htons(((struct sockaddr_in *) &conJob->in_addr)->sin_port);
    }
    getnameinfo((struct sockaddr *) &conJob->in_addr, conJob->in_len, ipbuf, sizeof(ipbuf), NULL, 0, NI_NUMERICHOST);
    transport->client_addr = putIPString(transport, ipbuf);

    /* Select for trace */
    if (ism_common_traceSelectEndpoint(transport->endpoint->name) ||
        ism_common_traceSelectClientAddr(transport->client_addr))
        transport->trclevel = &ism_defaultDomain.selected;

    in_len = sizeof(in_addr);
    getsockname(sock, (struct sockaddr *) &in_addr, &in_len);
    getnameinfo((struct sockaddr *) &in_addr, in_len, ipbuf, sizeof(ipbuf), NULL, 0, NI_NUMERICHOST);
    transport->serverport = conJob->endpoint->port;
    transport->server_addr = putIPString(transport, ipbuf);

    transport->protocol = "unknown";                       /* The protocol is not yet known */
    transport->protocol_family = "";
    transport->send = sendBytes;
    transport->close = closeConnectionNotify;
    transport->closed = connectionCloseInit;
    transport->fairuse = ism_tcp_fairuse;

    con = transport->tobj;
    con->id = __sync_add_and_fetch(&conCounter, 1);
    transport->index = con->id;
    transport->maxMsgSize = conJob->endpoint->maxMsgSize;
#undef TRACE_DOMAIN
#define TRACE_DOMAIN transport->trclevel


    con->endpoint = conJob->endpoint;
    con->transport = transport;
    con->socket = sock;
    con->rcvBuffer = NULL;
    con->sendBuffer = NULL;
    con->iopNext = NULL;
    con->iolNext = NULL;
    pthread_spin_init(&con->slock, 0);
#ifndef TLS_Shared
    if (con->endpoint->secure == 1) {
        createTlsObjects(transport, NULL, 0);
    }
#else
    con->secured = 0;
    transport->secure = con->secured;
#endif


    assignIOP(transport);

    addConnectionToList(con);
    addConnectionToIOThread(con);
    return 0;
}


/*
 * Stop a endpoint
 */
static int stopPortListening(ism_endpoint_t * endpoint) {
    if (conListener) {
        epoll_ctl(conListener->efd, EPOLL_CTL_DEL, endpoint->sock, NULL);
    }
    if (endpoint->sock >= 0)
        close(endpoint->sock);
    return 0;
}

/*
 * Connection endpoint thread
 */
static void * conListenerProc(void * parm, void * context, int value) {
    ioConnectionThread thData = (ioConnectionThread) parm;
    epoll_event events[1024];
    ism_endpoint_t * current[1024] = { 0 };
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
            ism_endpoint_t * port = current[i];
            current[i] = NULL;
            if ((port == NULL) || (port->isStopped))
                continue;
            for (j = 0; j < 1024; j++) {
                if (acceptNewConnection(port) > 0)
                    continue;
                break;
            }
        }
        nextSize = 0;
        count = epoll_wait(efd, events, 1024, -1);
        if (count > 0) {
            for (i = 0; i < count; i++) {
                struct epoll_event * event = &events[i];
                if (event->data.fd != pipefd[0]) {
                    ism_endpoint_t * endpoint = event->data.ptr;
                    if (endpoint && (endpoint->isStopped == 0)) {
                        current[nextSize++] = endpoint;
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
        return NULL;
    }
    close(thData->efd);
    close(pipefd[0]);
    close(pipefd[1]);

    return NULL;
}

/*
 * Normally CAN_READ is controlled by the CAN_READ bit in the state, but when using TLS we can also
 * require a write before we read more.
 */
#define CAN_READ(STATE)         ((((STATE) & ISM_TRANSPORT_STATE_RW) && ((STATE) & ISM_TRANSPORT_CAN_WRITE)) || (((STATE) & ISM_TRANSPORT_CAN_READ) && !((STATE) & ISM_TRANSPORT_STATE_WR)))

/*
 * Normally CAN_WRITE is controlled by the CAN_WRITE bit in the state, but wehn using TLS we can also
 * require a read before we write more.
 */
#define CAN_WRITE(STATE)        ((((STATE) & ISM_TRANSPORT_STATE_WR) && ((STATE) & ISM_TRANSPORT_CAN_READ)) || ((STATE) & ISM_TRANSPORT_CAN_WRITE))

/*
 * Write data without security
 */
HOT static int writeDataTCP(ism_connection_t * con) {
    ism_byteBuffer sendBuff = con->sendBuffer;
    con->state &= ~(ISM_TRANSPORT_STATE_WW);
    if (sendBuff) {
        int toWrite = sendBuff->used - (sendBuff->getPtr - sendBuff->buf);
        int rc = write(con->socket, sendBuff->getPtr, toWrite);
        if (con->state & ISM_TRANSPORT_SHUTDOWN_IN_PROCESS) {
            ism_transport_t * transport = con->transport;
            TRACE(9, "writeDataTCP in connection flush: connect=%u client=%s size=%d rc=%d\n",
                    transport->index, transport->name, toWrite, rc);
        }
        assert(toWrite > 0);
        if (rc > 0) {
            if (toWrite == rc) {
                sendBuff->putPtr = sendBuff->buf;
                sendBuff->getPtr = sendBuff->buf;
                sendBuff->used = 0;
                ism_common_returnBuffer(sendBuff, __FILE__, __LINE__);
                con->sendBuffer = NULL;
            } else {
                sendBuff->getPtr += rc;
            }
            con->transport->write_bytes += rc;
            if (!con->transport->originated) {
                con->transport->endpoint->stats->count[con->transport->tid].write_bytes += rc;
                __sync_add_and_fetch(&tcpStats.tcpC2PDataSent, rc);
            } else {
                __sync_add_and_fetch(&tcpStats.tcpP2SDataSent, rc);
            }
            return 0;
        }
        if (rc <= 0 && errno == EAGAIN) {
            if (socketsInfo[con->socket].sndBufAtMax == 0) {
                if (increaseSockBufSize(con->socket, SO_SNDBUF)) {
                    return 0;
                }
            }
            con->state |= ISM_TRANSPORT_STATE_WW;
            con->state &= ~ISM_TRANSPORT_CAN_WRITE;
            return 1;
        } else {
            /* Error should be discovered by next read */
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
    con->state &= ~(ISM_TRANSPORT_STATE_WW | ISM_TRANSPORT_STATE_WR);
    if (sendBuff && con->ssl && SSL_get_shutdown(con->ssl)==0) {
        int toWrite = sendBuff->used - (sendBuff->getPtr - sendBuff->buf);
        errno = 0;
        int rc = SSL_write(con->ssl, sendBuff->getPtr, toWrite);
        int ec = (rc > 0) ? SSL_ERROR_NONE : SSL_get_error(con->ssl, rc);
        if (con->state & ISM_TRANSPORT_SHUTDOWN_IN_PROCESS) {
            ism_transport_t * transport = con->transport;
            TRACE(9, "writeDataSSL in connection flush: connect=%u client=%s size=%d rc=%d ec=%d\n",
                   transport->index, transport->name, toWrite, rc, ec);
        }
        switch (ec) {
        case SSL_ERROR_NONE:
            if (rc > 0) {
                if (toWrite == rc) {
                    sendBuff->putPtr = sendBuff->buf;
                    sendBuff->getPtr = sendBuff->buf;
                    sendBuff->used = 0;
                    ism_common_returnBuffer(sendBuff, __FILE__, __LINE__);
                    con->sendBuffer = NULL;
                } else {
                    sendBuff->getPtr += rc;
                }
                con->transport->write_bytes += rc;
                if (!con->transport->originated) {
                    con->transport->endpoint->stats->count[con->transport->tid].write_bytes += rc;
                    __sync_add_and_fetch(&tcpStats.tcpC2PDataSent, rc);
                } else {
                    __sync_add_and_fetch(&tcpStats.tcpP2SDataSent, rc);
                }
            }
            return 0;
        case SSL_ERROR_WANT_READ:
            con->state |= ISM_TRANSPORT_STATE_WR;
            con->state &= ~ISM_TRANSPORT_CAN_READ;
            return 1;
        case SSL_ERROR_WANT_WRITE:
            if (socketsInfo[con->socket].sndBufAtMax == 0) {
                if (increaseSockBufSize(con->socket, SO_SNDBUF)) {
                    return 0;
                }
            }
            con->state |= ISM_TRANSPORT_STATE_WW;
            con->state &= ~ISM_TRANSPORT_CAN_WRITE;
            return 1;
        case SSL_ERROR_SSL:
            ism_common_traceSSLError("TLS write error", __FILE__, __LINE__);
            /* fall thru */
        default:
            /* Error should be discovered by next read */
            con->state &= ~ISM_TRANSPORT_CAN_WRITE;
            con->state |= ISM_TRANSPORT_CAN_READ;
            return 0;
        }
    }
    return 1;
}


/*
 * Write data to a connection
 */
HOT static int writeData(ism_connection_t * con) {
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
    return rc;
}

/*
 * Process data from the client.
 * This is called from either the TCP or SSL reads.
 */
static int processData(ism_connection_t * con, ism_byteBuffer rcvBuffer) {
    ism_transport_t * transport = con->transport;
    int dataLen;
    int offset = 0;

    if (rcvBuffer) {
        dataLen = rcvBuffer->used;
        if (dataLen <= 0)
            return 0;

        transport->read_bytes += dataLen;

        /* Update stats */
        if (!transport->originated) {
            transport->endpoint->stats->count[con->transport->tid].read_bytes += dataLen;
            __sync_add_and_fetch(&tcpStats.tcpC2PDataReceived, dataLen);
            transport->lastAccessTime = ism_common_readTSC();
        } else {
            __sync_add_and_fetch(&tcpStats.tcpP2SDataReceived, dataLen);
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
    } else {
        if (!con->rcvBuffer)
            return 0;
        dataLen = con->rcvBuffer->used;
        rcvBuffer = con->rcvBuffer;
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
    if (con->needBytes == 0) {
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
        if (con->needBytes == -9) {
            con->needBytes = 0;
            if (dataLen > 0) {               /* If throttling and we have bytes still available */
                con->pending_time = 0.0;
                con->mu_count++;             /* Increment the count */
            }
            TRACE(9, "needbytes datalen=%d\n", dataLen);
        } else {
            con->needBytes = 0;
            return -1;
        }
    }

    /*
     * If we have remaining bytes, record them in a buffer
     */
    if (dataLen > 0) {
        /* Allocate a new buffer */
        if (!con->rcvBuffer || dataLen > con->rcvBuffer->allocated) {
            ism_byteBuffer tmpBuf = ism_allocateByteBuffer(con->needBytes + dataLen + 1024);
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
                memmove(con->rcvBuffer->buf, rcvBuffer->buf+offset, dataLen);
            con->rcvBuffer->used = dataLen;
            con->rcvBuffer->getPtr = con->rcvBuffer->buf;
            con->rcvBuffer->putPtr = con->rcvBuffer->buf + dataLen;
        }
        TRACE(9, "More data needed: connect=%u datalen=%d offset=%d\n", transport->index, dataLen, offset);
    } else {
        if (con->rcvBuffer) {
            ism_common_returnBuffer(con->rcvBuffer, __FILE__, __LINE__);
            con->rcvBuffer = NULL;
        }
    }
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
        transport->close(transport, ISMRC_BadClientID, 0, "The ClientID is not valid");
        break;
    case 3:
        transport->close(transport, ISMRC_ServerCapacity, 0, "The server capacity is reached");
        break;
    case 5:
        closeConnectionNotify(transport, ISMRC_ConnectNotAuthorized, 0, "Connection not authorized");
        break;
    default: {
            int err = errno;
            char errStr[256];

            if (!g_stopped && transport->originated && transport->server) {
                ism_server_setLastBadAddress(transport->server, transport->connect_order);
            }

            if (err && rc && err != ECONNRESET && err != EAGAIN) {
                sprintf(errStr,"The connection was closed by the %s or network. Error=%s (%d)",
				    ((transport->originated) ? "server" : "client"), strerror(err), err);
            } else {
				sprintf(errStr,"The connection was closed by the %s or network (%d).",
				    ((transport->originated) ? "server" : "client"), rc);
				err = 0;
            }
            rc = (transport->originated) ? ISMRC_ClosedByServer : ISMRC_ClosedByClient;
            transport->close(transport, rc, 0, errStr);
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

    if (!con->ssl)
        return 1;

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
        ism_common_traceSSLError("TLS read error", __FILE__, __LINE__);
        /* fall thru */
    case SSL_ERROR_ZERO_RETURN:
    default:

        if ((ec == SSL_ERROR_ZERO_RETURN) && (SSL_get_shutdown(con->ssl))) {
            clean = 1;
            reason = "The connection has completed normally.";
            ec = 0;
        } else {
            int err = errno;
            const char * errStr = (ec < 9) ? SSL_ERRORS[ec] : "SSL_UNKNOWN_ERROR";
            char * mbuf;

            sslTraceErr(transport, ec, __FILE__, __LINE__);

            if (!g_stopped && transport->originated && transport->server) {
                ism_server_setLastBadAddress(transport->server, transport->connect_order);
            }
            mbuf = alloca(1024);
            if (err && err != ECONNRESET && err != EAGAIN) {
                snprintf(mbuf, 1024, "The connection was closed by the %s or network. Error=%s(%d) SSLError=%s",
                        ((transport->originated) ? "server" : "client"), strerror(err), err, errStr);
            } else {
                snprintf(mbuf, 1024, "The connection was closed by the %s or network (%d)",
                        ((transport->originated) ? "server" : "client"), ec);
            }
            con->state |= ISM_TRANSPORT_ERROR;
            clean = 0;
            reason = mbuf;
            ec = transport->originated ? ISMRC_ClosedByServer : ISMRC_ClosedByClient;
        }
        switch (transport->closestate[3]) {
        case 1:
            reason = "The connection has completed normally.";
            ec = 0;
            clean = 1;
            break;
        case 2:
            reason = "The ClientID is not valid";
            ec = ISMRC_BadClientID;
            break;
        case 5:
            reason = "Connection not authorized";
            ec = ISMRC_ConnectNotAuthorized;
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
    int sslShutdownMode = 0;
    if (!con->secured || !con->ssl) {
        stopIOListening(con);
        return 1;
    }
    //Get Shutdown Mode
    sslShutdownMode = SSL_get_shutdown(con->ssl);

    if (sslShutdownMode == 0 && !SSL_in_init(con->ssl)) {
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
static int connectionCloseComplete(ism_connection_t * con, int reuse) {
    ism_transport_t * transport = con->transport;
    if (transport->state == ISM_TRANST_Closed)
        return 0;
    TRACE(7, "connectionCloseComplete: connect=%u client=%s\n", transport->index, transport->name);
    if (reuse) {
        epoll_ctl(conListener->efd, EPOLL_CTL_DEL, con->socket, NULL);
    }
    if (con->socket > 0) {
        resetSocketInfo(con->socket, 0, 0, 0);
        close(con->socket);
        con->socket = 0;
    }

    /* Remove from CRLWait */
    if (transport->crtChckStatus == 9) {
        ism_ssl_stopCrlWait(transport, transport->org);
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
    if (con->sendBuffer) {
        ism_common_returnBuffer(con->sendBuffer,  __FILE__, __LINE__);
        con->sendBuffer = NULL;
    }
    while (con->sndQueueHead) {
        ism_byteBuffer bb = con->sndQueueHead;
        con->sndQueueHead = bb->next;
        ism_common_returnBuffer(bb, __FILE__, __LINE__);
    }

    while(con->asyncJobRequestsHead) {
        asyncJobRequest_t * next = con->asyncJobRequestsHead->next;
        ism_common_free(ism_memory_proxy_tcp,con->asyncJobRequestsHead);
        con->asyncJobRequestsHead = next;
    }
    con->asyncJobRequestsTail = NULL;

    pthread_spin_destroy(&con->slock);
    removeConnectionFromList(con, reuse);
    return 0;
}


/*
 * Delay Connection Close Complete
 */
static int delayConnectionCloseComplete(ism_timer_t key, ism_time_t timestamp, void * userdata) {
	ism_connection_t * con = (ism_connection_t *) userdata;
	ism_common_cancelTimer(key);
	ism_throttle_setConnectReqInQ(con->transport->clientID, 0);
	/* Run connectionCloseComplete in IOP thread */
	addJob4Processing(con, DELAY_DONE);
	return 0;
}


/*
 * Map an openssl verify return code to a CRL status
 * @param verifyrc  The return code from the openssl verify_cert()
 * @return A CRL status, or CRL_STATUS_NOTCRL if the verify return code is not CRL related
 */
int ism_proxy_mapCrlReturnCode(int verifyrc) {
    switch (verifyrc) {
    case X509_V_OK:
        return CRL_STATUS_OK;
    case X509_V_ERR_UNABLE_TO_GET_CRL:
        return CRL_STATUS_UNAVAILABLE;
    case X509_V_ERR_CRL_HAS_EXPIRED:
        return CRL_STATUS_EXPIRED;
    case X509_V_ERR_CRL_NOT_YET_VALID:
        return CRL_STATUS_FUTURE;
    case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
    case X509_V_ERR_CRL_SIGNATURE_FAILURE:
    case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
    case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
    case X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER:
    case X509_V_ERR_KEYUSAGE_NO_CRL_SIGN:
        return CRL_STATUS_INVALID;
    default:
        return CRL_STATUS_NOTCRL;
    }
}


/*
 * Close the connection during an SSL/TLS handshake
 */
static int sslHandshake(ism_connection_t * con) {
    ism_transport_t * transport = con->transport;
    int  rc;

    if (con->state & ISM_TRANSPORT_ERROR){
        ism_common_setErrorData(ISMRC_ClosedTLSHandshake, "%u%s", transport->index, transport->sniName ? transport->sniName : "");
        transport->close(transport, ISMRC_ClosedTLSHandshake, 0, "Connection closed during TLS handshake");
        return -1;
    }
    if (transport->originated) {
        rc = SSL_connect(con->ssl);
    } else {
        rc = SSL_accept(con->ssl);
    }
    if (rc == 0) {
        ism_common_setErrorData(ISMRC_ClosedTLSHandshake, "%u%s", transport->index, transport->sniName ? transport->sniName : "");
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
                TRACEX(8, TLS, 0, "Switch BIO: connect=%u\n", transport->index);
                SSL_set_bio(con->ssl, con->bio, con->bio);    /* Will also free bio1*/
                if (con->bio1DataPtr) {
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
                ism_common_setErrorData(ISMRC_ServerNotAvailable, "%s", transport->server ? transport->server->name : "");
                if (!g_stopped && transport->server) {
                    ism_server_setLastBadAddress(transport->server, transport->connect_order);
                }

                con->transport->write_bytes += con->transport->tlsWriteBytes;
                con->transport->read_bytes += con->transport->tlsReadBytes;
                if (con->transport->connected) {
                    con->transport->connected(con->transport, ISMRC_ServerNotAvailable);
                }
                transport->close(transport, ISMRC_ServerNotAvailable, 0, "Server not available");
                __sync_sub_and_fetch(&tcpStats.pendingOutgoingConnectionsCounter, 1);
            } else {
                if (ism_common_getLastError() == ISMRC_CertificateNotValid) {
                    char * lasterr = alloca(512);
                    ism_common_formatLastError(lasterr, 512);
                    transport->close(transport, ISMRC_CertificateNotValid, 0, lasterr);
                } else {
                    ism_common_setErrorData(ISMRC_ClosedTLSHandshake, "%u%s", transport->index, transport->sniName ? transport->sniName : "");
                    transport->close(transport, ISMRC_ClosedTLSHandshake, 0, "Connection closed during TLS handshake");
                }
            }
            return -1;
        }
    } else {
        /*
         * On a completed TLS outgoing connection
         */
        transport->write_bytes += transport->tlsWriteBytes;
        transport->read_bytes += transport->tlsReadBytes;
        if (transport->originated) {
            if (transport->connected) {
                transport->connected(con->transport, 0);
            }
            /* TODO: for self-signed, verify that this is ours */
            X509 * cert = SSL_get_peer_certificate(con->ssl);

            rc = SSL_get_verify_result(con->ssl);
            if (rc == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT && !checkServerCert)
                rc = X509_V_OK;
            if (rc != X509_V_OK) {
                const char * sslErr = X509_verify_cert_error_string(rc);
                X509_free(cert);
                if (SHOULD_TRACE(4)) {
                    BUF_MEM* certInfo = ism_ssl_getPeerCertInfo(con->ssl,
                            (TRACE_DOMAIN->trcComponentLevels[TRACECOMP_Transport] > 7), 1);
                    TRACE(4, "Certificate verification failed: From=%s:%u connect=%u error=%s (%d)\n%s\n",
                             transport->client_addr, transport->clientport, transport->index, sslErr, rc,
                             certInfo->data);
                    BUF_MEM_free(certInfo);
                }
                ism_common_setErrorData(ISMRC_CertificateNotValid, "%s", sslErr);
                char * lastError = alloca(512);
                ism_common_formatLastError(lastError, 512);
                transport->close(transport, ISMRC_CertificateNotValid, 0, lastError);
                return -1;
            }
            X509_free(cert);
        } else {
            /* Implement stats for incoming connections by TLS version */
            int method = ism_transport_getTLSMethod(transport);
            switch (method) {
            case SecMethod_TLSv1_1:
                __sync_add_and_fetch(&tcpStats.incomingTLSv1_1, 1);
                break;
            case SecMethod_TLSv1_2:
                __sync_add_and_fetch(&tcpStats.incomingTLSv1_2, 1);
                break;
            case SecMethod_TLSv1_3:
                __sync_add_and_fetch(&tcpStats.incomingTLSv1_3, 1);
                break;
            }
            /* For client connection verify certificate if required for org */
            if (transport->requireClientCert) {
                X509 * cert = SSL_get_peer_certificate(con->ssl);
                if (cert) {
                    X509_NAME * name;
                    rc = SSL_get_verify_result(con->ssl);
                    if (rc != X509_V_OK) {
                        if (transport->crlStatus) {
                            int crls = ism_proxy_mapCrlReturnCode(rc);
                            if (crls != CRL_STATUS_NOTCRL) {
                                transport->crlStatus = crls;
                            } else {
                                const char * sslErr = X509_verify_cert_error_string(rc);
                                X509_free(cert);
                                if (SHOULD_TRACE(4)) {
                                    BUF_MEM* certInfo = ism_ssl_getPeerCertInfo(con->ssl,
                                            (TRACE_DOMAIN->trcComponentLevels[TRACECOMP_Transport] > 7), 1);
                                    TRACE(4, "Certificate verification failed: connect=%d From=%s:%u error=%s\n%s\n",
                                            transport->index, transport->client_addr, transport->clientport, sslErr,
                                            certInfo->data);
                                    BUF_MEM_free(certInfo);
                                }
                                ism_common_setErrorData(ISMRC_CertificateNotValid, "%s", sslErr);
                                transport->close(transport, ISMRC_CertificateNotValid, 0, "Certificate not valid");
                                return -1;
                            }
                        }
                    }
                    name = X509_get_subject_name(cert);
                    if (name) {
                        char commonName[1024];
                        if (X509_NAME_get_text_by_NID(name, NID_commonName, commonName, sizeof(commonName)) != -1) {
                            transport->cert_name = ism_transport_putString(transport, commonName);
                        }
                    }
                    name = X509_get_issuer_name(cert);
                    if (name) {
                        char commonName[1024];
                        if (X509_NAME_get_text_by_NID(name, NID_commonName, commonName, sizeof(commonName)) != -1) {
                            transport->certIssuerName = ism_transport_putString(transport, commonName);
                        }
                    }
                    X509_free(cert);
                }
            }
        }
        con->state = (ISM_TRANSPORT_CONNECTED | ISM_TRANSPORT_CAN_READ | ISM_TRANSPORT_CAN_WRITE);
        return 0;
    }
    return 0;
}

/*
 * Add an async request to be processed in the IOP thread
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

/*
 * Process async work
 */
static void processAsyncRequests(ism_connection_t * con) {
    asyncJobRequest_t * curr;
    pthread_spin_lock(&con->slock);
    curr = con->asyncJobRequestsHead;
    con->asyncJobRequestsHead = con->asyncJobRequestsTail = NULL;
    pthread_spin_unlock(&con->slock);
    while(curr) {
        asyncJobRequest_t * next = curr->next;
        if (curr->func(curr->transport,curr->param1, curr->param2)) {
            addAsyncRequest(con,curr);
        } else {
            ism_common_free(ism_memory_proxy_tcp,curr);
        }
        curr = next;
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
        if (con->servername) {
            if (SSL_set_tlsext_host_name(ssl, con->servername) != 1) {
                TRACE(5, "Unable to set servername: connect=%u name=%s servername=%s\n",
                        transport->index, transport->name, con->servername);
            }
        }
        SSL_set_info_callback(ssl, ism_common_sslInfoCallback);
        SSL_set_msg_callback(ssl, ism_common_sslProtocolDebugCallback);
        SSL_set_msg_callback_arg(ssl, transport);
    }
    return 0;
}


/*
 * Sledgehammer timer for connection shutdown
 */
static int sledgeTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_connection_t * con = (ism_connection_t *) userdata;
    ism_transport_t * transport = con->transport;
    pthread_spin_lock(&con->slock);
    if (con->sledgecount > 0) {
        con->sledgecount--;
        TRACE(9, "Flush countdown: connect=%u name=%s count=%d\n", transport->index, transport->name, con->sledgecount);
        if (con->sledgecount == 0)
            addJob4Processing(con, SHUTDOWN_FORCE);
    }
    pthread_spin_unlock(&con->slock);
    return 1;
}

/*
 * Process a single IO request
 *
 * Return 0 if there is more data to process
 * Return 1 if there is no more data to process
 * Return -9 if we are throttled
 * Return -1 if there is an error
 */
HOT static int processIORequest(ism_connection_t * con, ioProcessorThread iopth, double currTime) {
    int rc1 = 1, rc2 = 1;
    int state = con->state;
    ism_transport_t * transport = con->transport;
#ifdef DEBUGX
    TRACE(9, "processIORequest: connect=%u client=%s state=%04x\n", transport->index,
    		transport->name, state);
#endif
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
        state = con->state;
        TRACE(9, "tcp flush connection writedata: connect=%u client=%s rc=%u state=%04x\n",
                con->transport->index, con->transport->name, rc1, state);
        if (rc1 == 0 && (state & ISM_TRANSPORT_CAN_WRITE))
            return 0;
        if (rc1 == 1 && (state & ISM_TRANSPORT_STATE_WW) && !(state & ISM_TRANSPORT_SHUTDOWN_FORCE)) {
            if (!con->sledgetimer) {
                pthread_spin_lock(&con->slock);
                con->sledgecount = 8;
                pthread_spin_unlock(&con->slock);
                TRACE(9, "Flush countdown start timer: connect=%u name=%s\n", con->transport->index, con->transport->name);
                con->sledgetimer = ism_common_setTimerRate(ISM_TIMER_HIGH, (ism_attime_t) sledgeTimer, con, 200, 200, TS_MILLISECONDS);
            } else {
                pthread_spin_lock(&con->slock);
                if (con->sledgecount-- == 0) {
                    TRACE(9, "Flush countdown end: connect=%u name=%s count=%d\n", con->transport->index, con->transport->name, con->sledgecount);
                    rc1 = 2;
                }
                pthread_spin_unlock(&con->slock);
            }
            if (rc1 == 1)
                return 1;
        }
        if (con->sledgetimer) {
            ism_common_cancelTimer(con->sledgetimer);
            con->sledgetimer = NULL;
        }
        return connectionShutdown(con);
    }

    /*
     * Handle an outgoing connection in progress
     */
    if (UNLIKELY(state & ISM_TRANSPORT_CONNECT_IN_PROCESS)) {
        int err = 0;
        socklen_t len = sizeof(err);

        getsockopt(con->socket, SOL_SOCKET, SO_ERROR, &err, &len);

        if (!err) {
            struct sockaddr_in6 sa;
            socklen_t sal = sizeof(sa);
            if (getpeername(con->socket, (struct sockaddr*)&sa, &sal)) {
                if (errno == ENOTCONN) {
                    con->state = ISM_TRANSPORT_CONNECT_IN_PROCESS;
                    TRACE(8, "processIORequest - Peer not yet connected: connect=%u server=%s port=%u state=%04x\n",
                            transport->index, transport->server_addr, transport->serverport, con->state);
                    return 1;
                }
            }
            TRACE(7, "processIORequest - Connection request complete: connect=%u server=%s port=%u state=0x%x protocol=%s family=%s\n",
                    transport->index, transport->server_addr, transport->serverport, state, transport->protocol, transport->protocol_family);
            if (con->tlsCTX) {
                makeTlsClientObjects(transport);
                state = con->state = ISM_TRANSPORT_HANDSHAKE_IN_PROCESS |
                        ISM_TRANSPORT_CONNECTED | ISM_TRANSPORT_CAN_READ | ISM_TRANSPORT_CAN_WRITE;
            } else {
                con->state = (ISM_TRANSPORT_CONNECTED | ISM_TRANSPORT_CAN_READ | ISM_TRANSPORT_CAN_WRITE);
                if (transport->connected) {
                    transport->connected(con->transport, 0);
                }
            }
            __sync_sub_and_fetch(&tcpStats.pendingOutgoingConnectionsCounter, 1);
        } else {
            TRACE(7, "processIORequest - Error during connection request: connect=%u server=%s port=%u state=%0x ready=%u tried=%u result=%s (%d)\n",
                    transport->index, transport->server_addr, transport->serverport, state,
                    transport->ready, transport->connect_tried, strerror(err), err);
            if (!g_stopped && transport->server && transport->originated) {
                ism_server_setLastBadAddress(transport->server, transport->connect_order);
            }

            con->state |= ISM_TRANSPORT_ERROR;
            con->transport->write_bytes += con->transport->tlsWriteBytes;
            con->transport->read_bytes += con->transport->tlsReadBytes;
            if (con->transport->connected) {
                con->transport->connected(con->transport, ISMRC_ServerNotAvailable);
            }
            transport->close(transport, ISMRC_ServerNotAvailable, 0, "Server not available");
            __sync_sub_and_fetch(&tcpStats.pendingOutgoingConnectionsCounter, 1);

            return 1;
        }
    }

    /*
     * Process a incoming handshake
     */
    if (UNLIKELY(state & ISM_TRANSPORT_HANDSHAKE_IN_PROCESS)) {
        /* SSL handshake is required */
        rc1 = sslHandshake(con);
        if (rc1 == 0)
            return 0;
        rc1 = writeData(con);
        if (rc1 < 0)
            return rc1;
    }

    /*
     * If closing
     */
    if (UNLIKELY(transport->state == ISM_TRANST_Closing)) {
        /* Transport is closing */
    	processAsyncRequests(con);
    	return -1;
    }

    /*
     * Normal read processing
     */
    if (CAN_READ(state)) {
        if (con->restart_time && currTime < con->restart_time) {
            rc1 = -9;   /* throttle */
        } else {
            if (con->rcvBuffer && con->needBytes == 0) {
                rc1 = processData(con, NULL);
                if (rc1 < 0 && rc1 != -9) {
                    processAsyncRequests(con);
                    return rc1;
                }
            } else {
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
    }
    processAsyncRequests(con);

    /*
     * Normal write processing
     */
    if (CAN_WRITE(state)) {
        rc2 = writeData(con);
        if (UNLIKELY(rc2 < 0))
            return rc2;
    }

    /* End for throttling */
    if (rc1 == -9)
        return -9;
    return (rc1 && rc2);
}

/*
 * The ioListener thread
 */
static void * ioListenerProc(void * parm, void * context, int value) {
    ioListenerThread_t * thData = (ioListenerThread_t *) parm;
    int eventSize = 64*1024;
    epoll_event * events = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,4),eventSize, sizeof(epoll_event));
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
            ism_common_free(ism_memory_proxy_tcp,job);
        }

        count = epoll_wait(efd, events, eventSize, -1);
        if (LIKELY(count > 0)) {
            for (i = 0; i < count; i++) {
                struct epoll_event * event = &events[i];
                if (LIKELY(event->data.fd != pipefd[0])) {
                    ism_connection_t * con = event->data.ptr;
                    if (LIKELY(con != NULL)) {
                        uint64_t xevents = event->events;
                        addJob4Processing(con, xevents);
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
                events = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,5),events, eventSize * sizeof(epoll_event));
            }
            continue;
        }
        if ((count == 0) || (errno == EINTR))
            continue;
        ism_common_free(ism_memory_proxy_tcp,events);
        return NULL;
    }
    ism_common_free(ism_memory_proxy_tcp,events);
    close(thData->efd);
    close(pipefd[0]);
    close(pipefd[1]);
    return NULL;
}

/*
 * External interface to sendBytes
 */
int ism_proxy_sendBytes(ism_transport_t * transport, char * buf, int len, int protval, int flags) {
    return sendBytes(transport, buf, len, protval, flags);
}

/*
 * Send bytes on output
 */
HOT static int sendBytes(ism_transport_t * transport, char * buf, int len, int protval, int flags) {
    char fbuf[16];
    int flen = 0;
    int buflen;
    ism_byteBufferPool pool;
    ism_byteBuffer sndBuffer = NULL;
    ism_byteBuffer sndBufferHead = NULL;
    ism_byteBuffer sndBufferTail = NULL;
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
        pthread_spin_unlock(&con->slock);
        return rc;
    }
    pthread_spin_unlock(&con->slock);
    pool = con->iopth->bufferPool;
    do {
        sndBuffer = ism_common_getBuffer(pool, 1);
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
        } else {
        	//TODO: Handle out of memory condition
        }
    } while (buflen > 0);
    pthread_spin_lock(&con->slock);
    if (con->sndQueueTail) {
        con->sndQueueTail->next = sndBufferHead;
        con->sndQueueTail = sndBufferTail;
    } else {
        con->sndQueueHead = sndBufferHead;
        con->sndQueueTail = sndBufferTail;
        addJob = 1;
    }
    con->transport->sendQueueSize += counter;
    pthread_spin_unlock(&con->slock);
    if (addJob)
        addJob4Processing(con, 0);
    return rc;
}

/*
 * Process a disconnect with possible throttle
 */
void processDisconnect(ism_transport_t * transport) {
    ism_connection_t * con = transport->tobj;
    if (ism_throttle_isEnabled() && !(con->state & ISM_TRANSPORT_SHUTDOWN_FORCE)) {
        int limit = ism_throttle_getThrottleLimit(con->transport->clientID, THROTTLET_CONNCLOSEERR);
        ism_time_t delay_nanos= 0;
        if (limit > 0) {
            delay_nanos = ism_throttle_getDelayTimeInNanosByType(limit, THROTTLET_CONNCLOSEERR);
        }
        if (delay_nanos > 0) {
            ism_throttle_setConnectReqInQ(con->transport->clientID, 1);
            con->state |= ISM_TRANSPORT_DELAY_WAIT;
            TRACE(8, "Delay Closing Connection: ClientID=%s connect=%d limit=%d delay=%ldd\n", con->transport->clientID, con->transport->index, limit, delay_nanos);
            ism_common_setTimerOnce(ISM_TIMER_HIGH, (ism_attime_t)delayConnectionCloseComplete, con, delay_nanos);
        } else {
            connectionCloseComplete(con, 0);
        }
    } else {
        connectionCloseComplete(con, 0);
    }
}

#undef TRACE_DOMAIN
#define TRACE_DOMAIN ism_defaultTrace


/*
 * IO Processor thread
 */
HOT static void * ism_tcp_ioProcessorThreadProc(void * parm, void * context, int value) {
    int i;
    ioProcessorThread_t * thData = (ioProcessorThread_t*) parm;
    uint32_t currentAllocated = 64 * 1024;
    ism_connection_t** current = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,6),currentAllocated, sizeof(ism_connection_t*));
    uint32_t currentSize;
    uint32_t nextSize = 0;
    uint32_t nextAvail = 0;

    while (!thData->isStopped) {
        int rc;
        iopJobsList * currentJobsList;
        double currTime;

        /*
         * Wait until there is work
         */
        if (useSpinLocks) {
            /* This was the old hardware spinlock support, but does not work well on shared or virtual systems */
            if (iopDelay && (nextAvail == 0) && (thData->currentJobsList->used == 0)) {
                if (value) {
                    if (iopDelay > 0) {
                        for (i = 0; i < iopDelay; i++) {
                            sched_yield();
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
            /* Use mutex and condition wait when there is no work to do  */
            pthread_mutex_lock(&thData->mutex);
            if (iopDelay && (nextAvail == 0) && (thData->currentJobsList->used == 0)) {
                pthread_cond_wait(&thData->cond, &thData->mutex);
                if (thData->isStopped){
                    pthread_mutex_unlock(&thData->mutex);
                    continue;
                }
            }
        }

        /* Switch job lists so we can add work while we process this one */
        currentJobsList = thData->currentJobsList;
        thData->currentJobsList = thData->nextJobsList;
        thData->nextJobsList = currentJobsList;

        /* Unlock the thread so others can add work */
        if (useSpinLocks)
            pthread_spin_unlock(&thData->lock);
        else
            pthread_mutex_unlock(&thData->mutex);

        currTime = ism_common_readTSC();
        /*
         * Move all connections from the job list to the internal job list
         */
        for (i = 0; i < currentJobsList->used; i++) {
            ioProcJob * job = currentJobsList->jobs + i;
            ism_connection_t * con = job->con;
            if (con) {
                uint64_t events = job->events;
                if (events) {
                    if (events & 0xFFFFFFFF) {
                        if (events & EPOLLIN) {
                            con->state |= ISM_TRANSPORT_CAN_READ;
                            if (con->pending_time) {
                                if (con->pending_time > currTime) {    /* If past pending time */
                                    con->mu_count++;                   /* Increment the count of exceeded mups */
                                }
                                con->pending_time = 0.0;               /* Reset pending */
                            }
                        }
                        if (events & EPOLLOUT) {
                            con->state |= ISM_TRANSPORT_CAN_WRITE;
                            if (con->sledgecount) {
                                pthread_spin_lock(&con->slock);
                                if (con->sledgetimer && con->sledgecount)
                                    con->sledgecount = 8;
                                pthread_spin_unlock(&con->slock);
                            }
                        }
                        if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                            __sync_fetch_and_or(&con->state,  (ISM_TRANSPORT_ERROR | ISM_TRANSPORT_CAN_READ));
                        }
                    } else {
                        if (events == SHUTDOWN_REQUEST) {
                            con->state |= ISM_TRANSPORT_SHUTDOWN_IN_PROCESS;
                            ism_transport_t * transport = con->transport;
                            TRACE(9, "Connection shutdown request: connect=%u name=%s state=%04x processing=%d\n",
                                    transport->index, transport->name, con->state, con->isProcessing);
                        } else if (events == SHUTDOWN_FORCE) {
                            con->state |= (ISM_TRANSPORT_SHUTDOWN_IN_PROCESS | ISM_TRANSPORT_SHUTDOWN_FORCE);
                            ism_transport_t * transport = con->transport;
                            TRACE(9, "Connection shutdown force: connect=%u name=%s state=%04x processing=%d\n",
                                transport->index, transport->name, con->state, con->isProcessing);
                        } else if (events == DELAY_DONE) {
                            con->state &= ~ISM_TRANSPORT_DELAY_WAIT;
                            con->state |= ISM_TRANSPORT_SHUTDOWN_FORCE;
                        } else {
                            con->state |= ISM_TRANSPORT_DISCONNECTED;
                            ism_transport_t * transport = con->transport;
                            TRACE(9, "Connection disconnect request: connect=%u name=%s state=%04x processing=%d\n",
                                transport->index, transport->name, con->state, con->isProcessing);
                        }
                    }
                }
                /* If not already in the list, add it */
                if (!con->isProcessing) {
                    if (nextSize == currentAllocated) {
                        currentAllocated *= 2;
                        current = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,7),current, currentAllocated * sizeof(ism_connection_t*));
                    }
                    current[nextSize++] = con;
                    con->isProcessing = 1;
                }
            }
        }

        /*
         * Process all of the current jobs
         */
        currentJobsList->used = 0;
        currentSize = nextSize;
        nextSize = 0;
        nextAvail = 0;
        // TRACE(9, "Process IO: time=%0.3f nextsize=%d\n", currTime, nextSize);

        for (i = 0; i < currentSize; i++) {
            ism_connection_t * con = current[i];
            ism_transport_t * transport = con->transport;
            current[i] = NULL;
            if (transport->state != ISM_TRANST_Closed) {
                /* Throttle message rate */
                if (con->restart_time) {
                    TRACE(9, "restart wait: time=%0.3f restart=%0.3f\n", currTime, con->restart_time);
                    if (con->restart_time <= currTime) {
                        con->reset_time = con->restart_time;
                        /* If log only or past the restart time */
                        if (con->mu_log==2 || con->con_mups <= con->con_mups_max) {
                            con->con_mups = 0;
                            con->restart_time = 0.0;
                        } else {
                            /* Decrement mups after one second */
                            con->con_mups -= con->con_mups_max;
                            /* If there is still more, increment time */
                            if (con->con_mups >= con->con_mups_max) {
                                con->restart_time += 1.0;
                                if (con->pending_time)
                                    con->pending_time += 1.0;
                            } else {
                                con->restart_time = 0.0;
                            }
                        }
                        //  printf("update restart_time:  mups=%f max=%f reset=%f restart=%f pending=%f\n",
                        //        con->con_mups, con->con_mups_max, con->reset_time, con->restart_time, con->pending_time);
                    }
                }

                /* Do the IO processing */
                rc = processIORequest(con, thData, currTime);
                if (!rc) {
                    /* More data available so put the connection back on the current job list */
                    current[nextSize++] = con;
                    nextAvail++;
                } else {
                    if (rc == -9) {
                        current[nextSize++] = con;
                    } else {
                        /* No more data is available so remove from the current job list */
                        con->isProcessing = 0;
                        if (con->state & ISM_TRANSPORT_DISCONNECTED) {
                            if (!(con->state & ISM_TRANSPORT_DELAY_WAIT))
                                processDisconnect(con->transport);
                        }
                    }
                }
            }
        }
    }
    ism_common_free(ism_memory_proxy_tcp,current);
    ism_common_destroyBufferPool(thData->bufferPool);
    ism_common_returnBuffer(thData->recvBuffer, __FILE__, __LINE__);
    return NULL;
}


/*
 * Create the connection thread
 */
static ioConnectionThread createIOCThread(const char * threadname) {
    ioConnectionThread iocth = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,8),1, sizeof(ioConnectionThread_t));
    strcpy(iocth->eyecatcher, "IOC");
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
 * Start the endpoint thread
 */
static ioListenerThread createIOLThread(const char * threadname) {
    ioListener = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,9),1, sizeof(ioListenerThread_t));
    strcpy(ioListener->eyecatcher, "IOL");
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
 * Create an IOP thread
 */
static ioProcessorThread createIOPThread(const char * threadname, ioListenerThread iolth) {
    ioProcessorThread_t * iopth = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,10),1, sizeof(ioProcessorThread_t));
    int maxPoolSize;
    int value;

    /*
     * Set up the IOP thread structure
     */
    iopth->iolth = iolth;
    strcpy(iopth->eyecatcher, "IOP");
    pthread_spin_init(&iopth->lock, 0);
    pthread_mutex_init(&iopth->mutex, NULL);
    pthread_cond_init(&iopth->cond, NULL);
    iopth->recvBuffer = ism_allocateByteBuffer(recvSize);
    iopth->jobsList[0].allocated = 16*1024;
    iopth->jobsList[0].used = 0;
    iopth->jobsList[0].jobs = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,11),iopth->jobsList[0].allocated, sizeof(ioProcJob));
    iopth->jobsList[1].allocated = 16*1024;
    iopth->jobsList[1].used = 0;
    iopth->jobsList[1].jobs = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,12),iopth->jobsList[1].allocated, sizeof(ioProcJob));
    iopth->currentJobsList = &(iopth->jobsList[0]);
    iopth->nextJobsList = &(iopth->jobsList[1]);
    if ( *threadname == 'M' ) {
    	maxPoolSize = 1024;
        value = 0;
    } else {
        maxPoolSize = (int)(maxPoolSizeBytes/sendSize);
        value = 1;
    }
    iopth->bufferPool = ism_common_createBufferPool(sendSize, 64, maxPoolSize, threadname);

    /*
     * Start the IOP thread
     */
    ism_common_startThread(&iopth->thread, ism_tcp_ioProcessorThreadProc, iopth, NULL, value, ISM_TUSAGE_NORMAL, 0,
            threadname, "TCP IO Processor");
    return iopth;
}


/*
 * Stop the IO processor thread
 */
static void stopIOPThread(ioProcessorThread iopth) {
    void * result = NULL;
    if (iopth) {
        if (useSpinLocks) {
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
    int inCounter = 0;
    int outCounter = 0;
    pthread_mutex_lock(&conMutex);
    curr = closedConnections;
    while (curr) {
        ism_transport_t * transport = curr->transport;
        next = curr->conListNext;

        if (next) {
            next->conListPrev = curr->conListPrev;
        }
        if (curr->conListPrev) {
            curr->conListPrev->conListNext = next;
        } else {
            closedConnections = next;
        }
        if (curr->outgoing)
            outCounter++;
        else
            inCounter++;
        ism_transport_freeTransport(transport);
        curr = next;
    }
    pthread_mutex_unlock(&conMutex);
    if (inCounter)
        __sync_sub_and_fetch(&tcpStats.incomingConnectionsCounter, inCounter);
    if (outCounter)
        __sync_sub_and_fetch(&tcpStats.outgoingConnectionsCounter, outCounter);

    ism_proxy_updateAuth(timestamp);
    return 1;
}


/*
 * Select connection
 */
inline static int selectConnection(ism_transport_t * transport, const char * clientID, const char * userID,
        const char * client_addr, const char * endpoint, const char * tenant, const char * server) {
    if (transport->name[0] == '_' && transport->name[1] == '_')
        return 0;
    if (clientID) {
        if (!ism_common_match(transport->name, clientID)) {
            return 0;
        }
    }
    if (userID) {
        if (transport->userid == NULL) {
            if (*userID)
                return 0;
        } else {
            if (!ism_common_match(transport->userid, userID))
                return 0;
        }
    }
    if (client_addr && transport->client_addr) {
        if (!ism_common_match(transport->client_addr, client_addr))
            return 0;
    }
    if (endpoint) {
        if (!ism_common_match(transport->endpoint_name, endpoint))
            return 0;
    }
    if (tenant && transport->tenant) {
        if (!ism_common_match(transport->tenant->name, tenant))
            return 0;
    }
    if (server && transport->tenant) {
        if (!ism_common_match(transport->tenant->serverstr, server))
            return 0;
    }
    return 1;
}

/*
 * Force a disconnection.
 *
 * Any parameter which is null does not participate in the match.
 * Note that this method will return as soon as we have scheduled a close of the
 * connection, which is probably before it is done.
 *
 * @param clientID  The clientID which can contain asterisks as wildcard characters.
 * @param userID    The userID which can contain asterisks as wildcard characters.
 * @param client_addr  The client address and an IP address which can contain asterisks.
 *                  If this is an IPv6 address it will be surrounded by brackets.
 * @param endpoint  The endpoint name which can contain asterisks as wildcard characters.
 * @param tenant    The tenant name which can contain asterisks as wildcard characters.
 * @param server    The sever name which can contain asterisks as wildcard characters.
 * @param permissions A bitmask of operation bits.  A zero means unconditional disconnect.
 * @return  The count disconnected
 *
 */
int ism_transport_closeConnection(const char * clientID, const char * userID, const char * client_addr,
        const char * endpoint, const char * tenant, const char * server, uint32_t permissions) {
    ism_transport_t * transport;
    int  count = 0;
    char xbuf[256];
    ism_connection_t * curr;
    ism_connection_t * next;


    if (!clientID && !userID && !client_addr && !endpoint && ! tenant && !server)
        return 0;
    ism_common_getErrorString(ISMRC_EndpointDisabled, xbuf, sizeof xbuf);

    pthread_mutex_lock(&conMutex);
    curr = activeConnections;
    while (curr) {
        transport = curr->transport;
        next = curr->conListNext;
        transport = curr->transport;
        /* Ignore handshake, internal, and admin connections   */
        if (transport->adminCloseConn == 0 && transport->name && *transport->name &&
                transport->endpoint_name && *transport->endpoint_name != '!') {
            if (selectConnection(transport, clientID, userID, client_addr, endpoint, tenant, server)) {
                /* Check for unconditional disconnect or lesser permissions */
                if (!permissions || (transport->auth_permissions & ~permissions)) {
                    TRACEL(5, transport->trclevel, "Force disconnect: client=%s From=%s:%u user=%s endpoint=%s\n",
                            transport->name, transport->client_addr, transport->clientport, transport->userid, transport->endpoint_name);
                    transport->adminCloseConn = 1;
                    transport->close(transport, ISMRC_EndpointDisabled, 0, xbuf);
                    count++;
                } else {
                    /* If we do not disconnect, record the new permissions */
                	transport->auth_permissions = permissions;
                }
            }
        }
        curr = next;
    }
    pthread_mutex_unlock(&conMutex);
    return count;
}

/*
 * Force a disconnection to server
 *
 * Thiw will include MUX, Monitor, and HTTP connections.
 *
 * Any parameter which is null does not participate in the match.
 * Note that this method will return as soon as we have scheduled a close of the
 * connection, which is probably before it is done.
 *
 * @param server    The sever name which can contain asterisks as wildcard characters.
 * @return  The count disconnected
 *
 */
int ism_transport_closeServerConnection(const char * server) {
    ism_transport_t * transport;
    int  count = 0;
    char xbuf[256];
    ism_connection_t * curr;
    ism_connection_t * next;


    if (!server)
        return 0;
    ism_common_getErrorString(ISMRC_EndpointDisabled, xbuf, sizeof xbuf);

    pthread_mutex_lock(&conMutex);
    curr = activeConnections;
    while (curr) {
        transport = curr->transport;
        next = curr->conListNext;
        transport = curr->transport;
        /* Ignore handshake, internal, and admin connections   */
        if (transport->adminCloseConn == 0 && transport->protocol && *transport->protocol && (transport->originated==1 || transport->originated==2)
        		&& transport->server && !strcmp(transport->server->name, server)) {
            if (  !strcmp(transport->protocol, "mux") || !strcmp(transport->protocol, "mqtt4-mon") || !strcmp(transport->protocol, "mqtt4-iotrest")) {
                 TRACEL(6, transport->trclevel, "Force disconnect the server connection: client=%s From=%s:%u user=%s endpoint=%s\n",
                        transport->name, transport->client_addr, transport->clientport, transport->userid, transport->endpoint_name);
                transport->adminCloseConn = 1;
                transport->close(transport, ISMRC_EndpointDisabled, 0, xbuf);
                count++;
            }
        }
        curr = next;
    }
    pthread_mutex_unlock(&conMutex);
    return count;
}


/*
 * Scan thru all connections to see if a metering message is requed.
 */
int ism_transport_checkMeteringTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    ism_connection_t * currCon;
    ism_time_t  currTime = timestamp;
    char tbuf [32];

    tbuf[0] = 0;
    pthread_mutex_lock(&conMutex);
    for (currCon = activeConnections; currCon != NULL ; currCon = currCon->conListNext) {
        ism_transport_t * transport = currCon->transport;
        if (transport->metering_time && currTime >= transport->metering_time) {
            if (!tbuf[0]) {
                /* Only make the timestamp once */
                ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_UTC);
                ism_common_setTimestamp(ts, ism_common_currentTimeNanos());
                ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 6, ISM_TFF_ISO8601);
                ism_common_closeTimestamp(ts);
            }
#ifndef NO_PROXY
            if (transport->ready > 1) {
			    if (ism_proxy_meteringMsg(transport, tbuf) == 0) {
			        transport->metering_time += g_metering_delta;
            	} else {
            	    /* If we fail creating a metering message, do not try to send any more */
            	    break;
            	}
            }
#endif
        }
    }
    pthread_mutex_unlock(&conMutex);
    return 1;
}

/*
 * Timer function for stale connections lookup
 */
static int ddosTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_connection_t * currCon;
    double now;

    pthread_mutex_lock(&conMutex);
    now = ism_common_readTSC();
    for (currCon = activeConnections; currCon != NULL ; currCon = currCon->conListNext) {
        ism_transport_t * transport = currCon->transport;
        if (transport && !transport->originated) {
            double doTimeout = 0;
            switch (transport->ready) {
            case 0:  /* No initial packet - 60 sec timeout */
                if ((now - transport->lastAccessTime) > 60.0) {    /* TODO: configurable */
                    TRACE(6, "Close a connection because the initial packet has not been received: connect=%u From=%s:%u port=%u\n",
                        transport->index, transport->client_addr, transport->clientport, transport->serverport);
                    transport->close(transport, ISMRC_NoFirstPacket, 0, "No data was received on the connection");
                }
                break;
            case 1: /* Handshake: 300 sec timeout */
                doTimeout = 300.0;
                break;
            case 4: /* MQTT serverless keepalive timeout */
                if (transport->keepalive) {
                    doTimeout = 3600.0;    /* Default to one hour */
                } else {
                    doTimeout = transport->keepalive * 1.5;
                }
                break;
            case 6: /* HTTP serverless 300 sec timeout */
                doTimeout = 300;
                break;
            case 7: /* Kafka/Mhub 60 sec Connect Timeout */
			   doTimeout = 60;
			   break;
            }
            if (doTimeout) {
                if ((now - transport->lastAccessTime) > doTimeout) {
                    TRACE(6, "Close a connection because no data was received on it: connect=%u client=%s From=%s:%u port=%u\n",
                        transport->index, transport->name, transport->client_addr, transport->clientport, transport->serverport);
                    transport->close(transport, ISMRC_TimeOut, 0, "The receive timed out");
                }
            }
        }
    }
    pthread_mutex_unlock(&conMutex);
    return 1;
}

/*
 * Timer function to ensure the IOP loop is run occasionally even if there are no new events.
 * This is required in low rate test environment, and mostly does nothing in a high volue
 * production environment.
 */
static int nullmsgTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    int  i;
    for (i=0; i<numOfIOProcs; i++) {
        addNullJob4Processing(ioProcessors[i]);
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


/*
 * Set the nolog config
 */
int ism_proxy_setNoLog(const char * nolog) {
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
                TRACE(3, "nolog ipaddr cannot be resolved: '%s'\n", token);
                LOG(WARN, Server, 926, "%-s", "ConnectionLogIgnore cannot resolve address: {0}", token);
                scope = -2;
                result = NULL;
            }
            ipaddr = 0;
            for (rp = result; rp; rp = rp->ai_next) {
                if (rp->ai_family == AF_INET) {
                    memcpy(&ipaddr, &(((struct sockaddr_in *)rp->ai_addr)->sin_addr), 4);
                    break;
                }
            }
            if (result)
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
            if (scope != -2) {
                char xbuf[1024];
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ConnectionLogIgnore", nolog);
                ism_common_formatLastError(xbuf, sizeof xbuf);
                LOG(WARN, Server, 930, "%-s%u", "ConectionLogIgnore is not set because the value is not valid: Error={0} Code={1}",
                                 xbuf, ism_common_getLastError());
                return 1;
            }
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

/*
 * Implement fair use for TCP
 */
static int ism_tcp_fairuse(ism_transport_t * transport, int request, int val1, int val2) {
    ism_connection_t * con = transport->tobj;
    int rc = 0;
    switch (request) {

    /*
     * Set the message unit size.  val1=musize (1 to 256MB). val2=log (0=false, 1=true, 2=only)
     * Setting a smaller unit size makes the size of the message more important.
     */
    case FUR_SetUnitSize:
        if (val1 < 1 || val1 > 0x1000000) {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%d", "MUPSUnit", val1);
            return rc;
        }
        con->mu_size  = val1;
        con->mu_log   = (uint8_t)val2;
        con->mu_count = 0;
        break;

    /*
     * SetMaxMUPS:  val1=mups (0=disable) val2=multiplier
     * A fractional message rate can be set using the multiplier.
     * For instance, to allow 2.3 message units per second you can
     * set mups=23 and multplier=10
     */
    case FUR_SetMaxMUPS:
    	/*
    	 * Check multiplier.  The mups value is already checked in
    	 * ism_proxy_makeFairUse() in tenant.c
    	 */
    	if (val2 < 1 || val2 > 10000) {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%d", "MUPSMul", val2);
            return rc;
        }
        con->con_mups_max = ((double)val1)/val2;
        transport->useMups = val1 != 0;
        break;

    /*
     * Increment the message units.
     */
    case FUR_Message:
        if (con->con_mups_max && con->mu_size) {
            float mups = (val2 / con->mu_size) + val1;
            con->con_mups += mups;
            con->mu_total += mups;
            if (con->con_mups >= con->con_mups_max) {
                double currTime = ism_common_readTSC();
                /* We do not count a mups exceeded until we get a message */
                if (con->pending_time) {
                    con->mu_count++;
                    con->pending_time = 0.0;
                }
                /* If past our reset time, decrement the mups by mups max */
                if (con->reset_time) {
                    if (currTime-con->reset_time > 1.0) {
                        if (con->con_mups <= con->con_mups_max) {
                            con->con_mups = 0;
                        } else {
                            con->con_mups -= con->con_mups_max;
                        }
                        /* Zero out mups as we are below the rate */
                        if (con->con_mups <= con->con_mups_max) {
                            con->reset_time = currTime;
                            con->restart_time = 0.0;
                            return 0;
                        }
                    }
                    /* Determine the time at which we will count a message as exceeding the rate */
                    con->pending_time = con->reset_time + (int)(currTime - con->reset_time) + 1.0;
                    if (con->mu_log == 2) {
                        con->restart_time = currTime;
                    } else {
                        con->restart_time = con->pending_time;
                        con->pending_time += 0.05;
                    }
                } else {
                    con->restart_time = currTime;
                    con->pending_time = 0.0;
                }
                TRACE(9, "throttle connect=%d curr=%0.3f reset=%0.3f pending=%0.3f\n", transport->index, currTime, con->restart_time, con->pending_time);
                return -9;
            }
        }
        break;
    }
    return rc;
}


/*
 * Initialize the TCP transport
 */
int ism_transport_initTCP(void) {
    struct rlimit rlim;
    int    maxcount,i;
    uint64_t  maxPoolSizeMB;
    g_org2sslCTXMap = ism_common_createHashMap(64*1024, HASH_STRING);
    /* Set the receive size */
    recvSize = ism_common_getBuffSize("TcpRecvSize", ism_common_getStringConfig("TcpRecvSize"), "8192");
    if (recvSize < 512)
        recvSize = 512;
    if (recvSize > (1024 * 1024))
        recvSize = 1024 * 1024;

    /* Set the send size */
    sendSize = ism_common_getBuffSize("TcpSendSize", ism_common_getStringConfig("TcpSendSize"), "8192");
    if (sendSize < 512)
        sendSize = 512;
    if (sendSize > (1024 * 1024))
        sendSize = 1024 * 1024;
    tcpMaxCon = ism_common_getIntConfig("TcpMaxCon", 4096);
    useSpinLocks = ism_common_getBooleanConfig("UseSpinLocks", 0);

    numOfIOProcs = ism_common_getIntConfig("IOThreads", 1);
    if (numOfIOProcs > MAX_IOP_NUM)
        numOfIOProcs = MAX_IOP_NUM;
    useLCPolicy = ism_common_getIntConfig("useLCPolicy", 0);
    ioProcessors = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,13),numOfIOProcs+1, sizeof(ioProcessorThread));
    maxPoolSizeMB = ism_common_getIntConfig("TcpMaxTransportPoolSizeMB", g_isBridge ? 100 : 500);
    if (maxPoolSizeMB < 32)
        maxPoolSizeMB = 32;

    maxPoolSizeBytes =  ((maxPoolSizeMB*1024*1024) / (numOfIOProcs + 1));

    iopDelay = ism_common_getIntConfig("IODelay", -1);

    g_bigLog = !ism_common_getIntConfig("ConnectionLogConcise", 0);

    /*
     * NoLogAddres is an IPv4 address and scope such as "127.0.0.0/24".
     * The scope can be between 8 and 30.  If no score is given, a single
     * IP address is blocked from logging.
     */
    const char * nolog = ism_common_getStringConfig("ConnectionLogIgnore");
    if (nolog) {
        ism_proxy_setNoLog(nolog);
    }

    TRACE(6, "Initialize the TCP transport: threads=%d poolsize=%uMB\n", numOfIOProcs, (uint32_t)maxPoolSizeMB);


    checkServerCert = ism_common_getBooleanConfig("CheckServerCertificate", 0);

    /*Allow The organization to use the Expired TLS Certificate*/
    allowExpiredCertOrg = ism_common_getStringConfig("AllowExpiredCertOrg");
    if(allowExpiredCertOrg!=NULL)
    	TRACE(5, "Allow Expired Certificate for organization: %s\n", allowExpiredCertOrg);

    /**
     * Get TLS Security Level Configuration (if any)
     * Support the setting of level 0 to 5.
     * If the configuration is invalid, will set it to unset (-1)
     **/
    g_tlsseclevel = ism_common_getIntConfig("TlsSecurityLevel", -1);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    if (g_tlsseclevel != -1) {
        TRACE(5, "TLS Security Level (%d) ignored. Security level not supported with OpenSSL < 1.1\n", g_tlsseclevel);
        g_tlsseclevel = -1;
    }
#else
    if(g_tlsseclevel<0 || g_tlsseclevel > 5){
        if(g_tlsseclevel != -1) {
    	    TRACE(5, "TLS Security Level (%d) is invalid. Default TLS Security Level will be used.\n", g_tlsseclevel);
        }
        g_tlsseclevel = -1;
    }
#endif

    /*
     * Start a timer for cleanup
     */
    TRACE(8, "set tcp cleanup: cleanup_timer=%llu\n", (ULL)cleanup_timer);
    if (!cleanup_timer) {
        cleanup_timer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) cleanupTimer, NULL, 2, 3, TS_SECONDS);
    }

    /*
     * Start a denial of service timer
     */
    if (!ddos_timer) {
        ddos_timer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) ddosTimer, NULL, 60, 33, TS_SECONDS);
    }

    incomingConnectionsMax = ism_common_getIntConfig("TcpMaxConnections", ACTIVE_CONNECTION_MAX_DEFAULT);
    getrlimit(RLIMIT_NOFILE, &rlim);
    maxcount = (int)rlim.rlim_cur - 512;
    maxcount = maxcount/100 * 50;
    if (incomingConnectionsMax > maxcount)
    	incomingConnectionsMax = maxcount;
    TRACE(6, "Set maximum TCP connections: %d\n", incomingConnectionsMax);
    maxSocketId = 4096;
    allocSocketId = rlim.rlim_cur;
    if (allocSocketId < 4096)
        allocSocketId = 4096;
    socketsInfo = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,14),allocSocketId, sizeof(socketInfo_t));
    for (i = 0; i < maxSocketId; i++) {
        pthread_spin_init(&socketsInfo[i].lock, 0);
    }
    g_stopped = 1;
    chkRcvBuffTimer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) conRcvBufCheckTimer, NULL, 30, 30, TS_SECONDS);

    ism_ssl_SNI_init();
    return 0;
}

/*
 * Return the max tcp connections
 */
int ism_transport_getTcpMax(void) {
    return allocSocketId;
}



/*
 * SSL methods enumeration
 */
ism_enumList enum_methods2 [] = {
    { "Method",    5,                 },
    { "SSLv3",     SecMethod_TLSv1,   },
    { "TLSv1",     SecMethod_TLSv1,   },
    { "TLSv1.1",   SecMethod_TLSv1_1, },
    { "TLSv1.2",   SecMethod_TLSv1_2, },
    { "TLSv1.3",   SecMethod_TLSv1_3, },
};


/*
 * Update the TCP endpoint without restarting
 */
int ism_transport_updateTCPEndpoint(ism_endpoint_t * endpoint) {
    epoll_event event = {0};
    event.data.ptr = endpoint;
    event.events = EPOLLIN | EPOLLRDHUP;
    return epoll_ctl(conListener->efd, EPOLL_CTL_MOD, endpoint->sock, &event) == -1 ? 1 : 0;
}

static void destroyEndpointSSLCtx(ism_endpoint_t * endpoint) {
    if (endpoint->sslCTX) {
        ism_common_destroy_ssl_ctx(endpoint->sslCTX);
        endpoint->sslCTX = NULL;
    }
}

/*
 * Key password callback
 */
static int getkeypw(char * buf, int size, int rwflag, void * userdata) {
    const char * pwin = (char *)userdata;
    if (!userdata) {
        if (size > 0)
            *buf =  0;
        return 1;
    }
    if (*pwin == '!') {
        ism_transport_makepw(pwin, buf, size, 1);
    } else {
        ism_common_strlcpy(buf, pwin, size);
    }
    return strlen(buf);
}

/*
 * Start the transport.
 * This is called multiple times if there are multiple tcp ports.
 * This is called with the endpointlock held
 */
int ism_transport_startTCPEndpoint(ism_endpoint_t * endpoint) {
    SOCKET  portSocket;
    epoll_event event;
    int  reOpen = 0;

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
    destroyEndpointSSLCtx(endpoint);

    /*
     * If we are enabled, start processing
     */
    if (endpoint->enabled) {
        endpoint->thread_count = numOfIOProcs + 1;

        if (endpoint->secure == 1) {
            endpoint->sslCTX = ism_common_create_ssl_ctxpw(endpoint->name, ism_common_enumName(enum_methods2, endpoint->tls_method),
                endpoint->ciphers, endpoint->cert, endpoint->key, 1, endpoint->sslop, endpoint->clientcert==1,
                endpoint->name, NULL, NULL, endpoint->keypw ? getkeypw : NULL, (void *)endpoint->keypw);
            if (endpoint->sslCTX == NULL) {
                endpoint->enabled = 0;
                endpoint->rc = ISMRC_CreateSSLContext;
                ism_common_setError(endpoint->rc);
                TRACE(1, "The TLS context could not be created for endpoint %s\n", endpoint->name);
                /* This is logged in ism_common_create_ssl_ctl() */
                return endpoint->rc;
            }
            /* Set callback to handle SNI extension */
            SSL_CTX_set_tlsext_servername_callback(endpoint->sslCTX, ssl_servername_cb);
            SSL_CTX_set_tlsext_servername_arg(endpoint->sslCTX, NULL);

            /*Set the TLS Security Level if the configuration TlsSecurityLevel is set*/
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
            int defSecLevel =  SSL_CTX_get_security_level(endpoint->sslCTX);
            if(g_tlsseclevel != -1 && g_tlsseclevel != defSecLevel){
                SSL_CTX_set_security_level(endpoint->sslCTX, g_tlsseclevel);
            }
            int currSecLevel = SSL_CTX_get_security_level(endpoint->sslCTX);
            TRACE(5, "Transport TLS Security Level: default=%d current=%d\n", defSecLevel, currSecLevel);
#endif
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
                destroyEndpointSSLCtx(endpoint);
                endpoint->enabled = 0;
                return endpoint->rc;
            }
        }
        endpoint->sock = portSocket;
        TRACE(5, "Start TCP endpoint: name=%s transport=%s port=%u tls==%s\n", endpoint->name,
                endpoint->transport_type, endpoint->port, ism_common_enumName(enum_methods, endpoint->tls_method));

        memset(&event, 0, sizeof(epoll_event));
        event.data.ptr = endpoint;
        event.events = EPOLLIN | EPOLLRDHUP;
        if (epoll_ctl(conListener->efd, EPOLL_CTL_ADD, endpoint->sock, &event) == -1) {
            ism_common_setError(ISMRC_EndpointSocket);
            endpoint->rc = ISMRC_EndpointSocket;
            destroyEndpointSSLCtx(endpoint);
            endpoint->enabled = 0;
            return endpoint->rc;
        }
        endpoint->needed = 0;
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
     * Start the IO processor threads
     */
    for (i = 0; i < numOfIOProcs; i++) {
        snprintf(threadname,sizeof(threadname), "tcpiop.%u", i);
        ioProcessors[i] = createIOPThread(threadname, ioListener);
        ioProcessors[i]->which = i;
    }

    /*
     * Start a null message timer which is used ot make sure the IOP runs occasionally even if no events are received.
     * This is necessary for testing message rate throttling but is generally not required in production envivonments.
     */
    if (!nullmsg_timer && !useSpinLocks) {
        nullmsg_timer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) nullmsgTimer, NULL, 3777, 200, TS_MILLISECONDS);
    }

    g_stopped = 0;

    return 0;
}

/*
 * Start IoP thread for notifications
 */
int ism_transport_startMonitorIOP(void) {
    /* Start IO process thread for notfications */
    monitorIoProcessor = createIOPThread("notifier", ioListener);
    ioProcessors[numOfIOProcs] = monitorIoProcessor;
    ioProcessors[numOfIOProcs]->which = numOfIOProcs;
    return 0;
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
    if (nullmsg_timer) {
        ism_common_cancelTimer(nullmsg_timer);
        nullmsg_timer = NULL;
    }
    if (ddos_timer) {
    	ism_common_cancelTimer(ddos_timer);
    	ddos_timer = NULL;
    }

    /* Stop receiving new connection */
    stopIOCThread(conListener);

    /*Terminate all connections to Servers*/
    TRACE(6, "Close all Server connections\n");
    ism_tenant_term();

    /* Close all existing connections */
    TRACE(6, "Close all connections\n");
    ism_transport_closeAllConnections(0, 0);
    TRACE(6, "After close all connections\n");
    ism_common_sleep(500);
    if (cleanup_timer) {
        ism_common_cancelTimer(cleanup_timer);
        cleanup_timer = NULL;
    }

#ifndef NO_PROXY
    ism_kafka_term();
    cleanupTimer(0, 0, NULL);
    ism_common_sleep(1000);
#endif
    cleanupTimer(0, 0, NULL);
    stopIOLThread(ioListener);

    /* Stop threads */
    TRACE(5, "Stop IOP threads\n");
    for (i = 0; i < numOfIOProcs; i++) {
        stopIOPThread(ioProcessors[i]);
    }
    for (i = 0; i < numOfIOProcs; i++) {
        ioProcessorThread iopth = ioProcessors[i];
        if (iopth) {
            if (iopth->jobsList[0].jobs)
                ism_common_free(ism_memory_proxy_tcp,iopth->jobsList[0].jobs);
            if (iopth->jobsList[1].jobs)
                ism_common_free(ism_memory_proxy_tcp,iopth->jobsList[1].jobs);
            ism_common_free(ism_memory_proxy_tcp,iopth);
            ioProcessors[i] = NULL;
        }
    }
    ioProcessors[i] = NULL;
    /* cleanup for admin IO Processor thread */
    if ( monitorIoProcessor ) {
        stopIOPThread(monitorIoProcessor);
        ioProcessorThread iopth = monitorIoProcessor;
        if (iopth->jobsList[0].jobs)
            ism_common_free(ism_memory_proxy_tcp,iopth->jobsList[0].jobs);
        if (iopth->jobsList[1].jobs)
            ism_common_free(ism_memory_proxy_tcp,iopth->jobsList[1].jobs);
        ism_common_free(ism_memory_proxy_tcp,iopth);
        monitorIoProcessor = NULL;
    }
    if (ioListener) {
        ism_common_free(ism_memory_proxy_tcp,ioListener);
        ioListener = NULL;
    }
    if (conListener) {
        ism_common_free(ism_memory_proxy_tcp,conListener);
        conListener = NULL;
    }
    if (socketsInfo) {
        ism_common_free(ism_memory_proxy_tcp,socketsInfo);
        socketsInfo = NULL;
    }
    if (SHOULD_TRACE(8)) {
        TRACE(1, "Complete TCP termination\n");
    }
    return 0;
}

/*
 * Dump the protocol object associated with this transport object.
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
    asyncJobRequest_t * pReq = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_tcp,15),sizeof(asyncJobRequest_t));
    pReq->func = job;
    pReq->transport = transport;
    pReq->param1 = param1;
    pReq->param2 = param2;
    addAsyncRequest(con,pReq);
}

/*
 * Initialize a transport object
 */
void ism_tcp_init_transport(ism_transport_t * transport) {
	transport->tobj = (struct ism_transobj *)ism_transport_allocBytes(transport, sizeof(ism_connection_t), 1);
    transport->send = sendBytes;
    transport->close = closeConnectionNotify;
    transport->closed = connectionCloseInit;
}

/*
 * Close all connections.
 * Go thru the whole list of connections and do a close waiting until
 * there are no more in the list or we reach the maximum retries.
 */
void ism_transport_closeAllConnections(int notAdmin, int notKafka) {
    ism_connection_t * con;
    int i;
    int activeCons = 0;
    char xbuf[8192];
    int lastwait = 0;
    int nextlastwait = 0;
    int waittime = 60;

    pthread_mutex_lock(&conMutex);
    if (!activeConnections) {
        pthread_mutex_unlock(&conMutex);
        return;
    }
    con = activeConnections;
    while (con) {
        if (!(con->state & ISM_TRANSPORT_CONNECTED) ||
            ((!notAdmin || *con->transport->protocol_family != 'a') &&
             (!notKafka || *con->transport->protocol_family != 'k'))) {
            ism_common_setErrorData(ISMRC_ServerTerminating, "%d%s", con->transport->index, con->transport->name);
            con->transport->close(con->transport, ISMRC_ServerTerminating, 0, "The connection was closed due to a shutdown.");
        }
        con = con->conListNext;
        activeCons++;
    }

    TRACE(3, "Close all connection process is initiated for %d connections\n", activeCons);
    uint32_t nonCloser = 0;
    for (i = 0; i < waittime; i++) {    /* wait time in seconds */
        nonCloser = 0;
        uint32_t thisProgress = 0;
        pthread_mutex_unlock(&conMutex);
        ism_common_sleep(1000000);    /* 1 second */
        pthread_mutex_lock(&conMutex);
        if (activeConnections == NULL) {
            break;
        }

        con = activeConnections;
        while (con) {
            if ((!notAdmin || *con->transport->protocol_family != 'a') &&
                (!notKafka || *con->transport->protocol_family != 'k')) {
                nonCloser++;
                /* If a kafka connection do not stop the wait early */
                if (*con->transport->protocol_family == 'k')
                    lastwait = 0;
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
        if (nonCloser == 0 || (nonCloser == lastwait && nextlastwait == lastwait )) {
            TRACE(1, "nonCloser=%d lastwait=%d nextlastwait=%d waittime=%d\n", (int)nonCloser, lastwait, nextlastwait, waittime);
            break;
        }
        nextlastwait = lastwait;
        lastwait = nonCloser;

        TRACE(1, "Connections still open after %d seconds: count=%u inprocess=%u\n",
                i+1, nonCloser, thisProgress);
    }

    /*
     * Trace any outstanding connections
     */
    nonCloser=0;
    con = activeConnections;
    while (con) {
        xbuf[0] = 0;
        if (con->transport->dumpPobj)
            con->transport->dumpPobj(con->transport, xbuf, sizeof xbuf);
        if ((!notAdmin || *con->transport->protocol_family != 'a') &&
            (!notKafka || *con->transport->protocol_family != 'k')) {
        	 	 nonCloser++;
             TRACE(5, "Connection was not closed during TCP transport termination: transport=%p connect=%u protocol=%s name=%s %c %s\n",
               con->transport, con->transport->index, con->transport->protocol, con->transport->name, (*xbuf ? ':' : ' '), xbuf);
        		//LOG(WARN, Connection, 1122, "%p%u%s%s%c%s", "Connection was not closed during TCP transport termination: transport={0} connect={1} protocol={2} name={3} {4} {5}",
        	    //        con->transport, con->transport->index, con->transport->protocol, con->transport->name, (*xbuf ? ':' : ' '), xbuf);

        } else {
            TRACE(5, "Connection still open after initial connection close: connect=%u protocol=%s name=%s\n",
                con->transport->index, con->transport->protocol, con->transport->name);
        }
        con = con->conListNext;
    }
    pthread_mutex_unlock(&conMutex);

    LOG(NOTICE, Connection, 1122, "%d%d", "Closed active connections during TCP transport termination. TotalConnections={0} notClose={1}",
    			activeCons, nonCloser);

    /*
     * If it took too long to terminate, just send a SIGKILL
     */
    if (activeConnections != NULL && !notAdmin) {
        TRACE(5, "Not all connections were closed during TCP transport termination.\n");
        ism_common_shutdown(0);
    }
}

/*
 * Put an IP string in the transport object.
 * Convert this to either an IPv4 string, or an IPv6 string in brackets.
 */
static void saveAsIPString(const char * ipIn, char * ipOut) {
    const char * pos;
    int len = strlen(ipIn);
    if (len > 7 && !memcmp(ipIn, "::ffff:", 7) && !strchr(ipIn+7, ':')) {
        strcpy(ipOut,ipIn+7);
    } else {
        pos = strchr(ipIn, ':');
        if (pos) {
            len = strcspn(ipIn, "%");
            ipOut[0] = '[';
            memcpy(ipOut+1, ipIn, len);
            ipOut[len+1] = ']';
            ipOut[len+2] = 0;
        } else {
            strcpy(ipOut,ipIn);
        }
    }
}

static void moreOutgoing(ism_transport_t * transport, int rc, struct addrinfo * addrinfo);

/*
 * Create an outgoing connection
 */
static int createOutgoingConnection(ism_transport_t * transport, ism_server_t * server, int port, ioProcessorThread iopth) {
	ism_connection_t * connection = transport->tobj;

	if (!transport->index) {
        __sync_add_and_fetch(&tcpStats.outgoingConnectionsCounter, 1);
        connection->id = __sync_add_and_fetch(&conCounter, 1);
        transport->index = connection->id;
	}
    transport->tobj->server = server;
    connection->iopth = iopth;
    connection->transport = transport;
    connection->endpoint = transport->endpoint;
    connection->outgoing = 1;
    pthread_spin_init(&connection->slock, 0);
	connection->state = 0;
	__sync_add_and_fetch(&tcpStats.pendingOutgoingConnectionsCounter, 1);
	TRACE(9, "createOutgoingConnection: connect=%u name=%s server=%s thread=%u port=%u\n",
	        transport->index, transport->name, server->name, iopth->which, transport->serverport);
	return server->getAddress(server, transport, moreOutgoing);
}

/*
 * Continue making an outgoing connection after name resolution
 *
 * At the time of this callback, the transport object has the following fields set:
 * server_addr:  The original unresolved address
 * serverport:   The port to use
 *
 */
static void moreOutgoing(ism_transport_t * transport, int rc, struct addrinfo * addrinfo) {
    int sock = -1;
    struct addrinfo * rp;
    int    err = 0;
    ism_connection_t * connection = transport->tobj;
    char ipinfo[64];
    char ipinfo2[64];
    char xbuf [1024];

    if (rc < 0 || rc >= ISMRC_Error) {
        if (rc < 0) {
            ism_common_setErrorData(ISMRC_UnknownHost, "%s%s", transport->server_addr, gai_strerror(rc));
            rc = ISMRC_UnknownHost;
        }
        ism_server_setLastBadAddress(transport->server, transport->slotused);
        if (transport->connected) {
            transport->connected(transport, rc);
        }
        ism_common_formatLastError(xbuf, sizeof xbuf);
        transport->close(transport, ISMRC_UnknownHost, 0, xbuf);
        return;
    } else {
        __sync_add_and_fetch(&tcpStats.outgoingConnectionsTotal, 1);
    }
    transport->connect_order = rc;
    rc = 0;
    transport->clientport = 0;

    /* Set server name */
    if (!transport->tobj->servername) {
        /* If we have an ipString and it does not appear to be numeric IP.   */
        if (*transport->server_addr >= 'A' && *transport->server_addr != '[')
            transport->tobj->servername = transport->server_addr;
    }
    transport->client_addr = transport->server_addr;

    for (rp = addrinfo; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP);
        if (sock < 0) {
            err = errno;
            ism_common_setError(ISMRC_EndpointSocket);
            continue;
        }

        /* Set the socket reusable */
        int flags = 1;
        if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags)) < 0) ||
             setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *) &flags, sizeof(flags)) ||
             setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *) &flags, sizeof(flags))) {
            err = errno;
            ism_common_setErrorData(ISMRC_EndpointSocket, "%s%s", transport->server_addr, strerror(err));
            addConnectionToList(connection);
            transport->connected(transport, ISMRC_EndpointSocket);
            transport->close(transport, ISMRC_EndpointSocket, 0, "Unable to create the socket");
            return;
        }
        break;
    }
    if (sock < 0) {
        ism_common_setError(ISMRC_EndpointSocket);
        TRACE(5, "Unable to create socket: connect=%u error=%s (%d)\n", transport->index, strerror(err), err);
        addConnectionToList(connection);
        transport->connected(transport, ISMRC_EndpointSocket);
        transport->close(transport, ISMRC_EndpointSocket, 0, "Unable to create the socket");
        return;
    }

    getnameinfo((struct sockaddr *) rp->ai_addr, rp->ai_addrlen, ipinfo, sizeof(ipinfo), NULL, 0, NI_NUMERICHOST);
    saveAsIPString(ipinfo, ipinfo2);
    transport->server_addr = ism_transport_putString(transport, ipinfo2);

    connection->socket = sock;
    addConnectionToList(connection);
    __sync_fetch_and_or(&connection->state, ISM_TRANSPORT_CONNECT_IN_PROCESS);
    transport->connect_tried++;

    /*
     * Do the connection.
     * As this is non-blocking, it will almost always return EINPROGRESS and continue
     * when the connection is writable.  In any case we move the processing into
     * the IoP.
     */
    struct sockaddr_in6 * addr6 = (struct sockaddr_in6 *)rp->ai_addr;
    addr6->sin6_port = htons(transport->serverport);
    int connrc = connect(sock, addr6, rp->ai_addrlen);
    err = errno;
    TRACE(9, "Add connection to thread: connect=%u sock=%u state=%04x\n", transport->index, connection->socket, connection->state);
    addConnectionToIOThread(connection);
    if (connrc && err != EINPROGRESS) {
        addJob4Processing(connection, EPOLLOUT);
    } else {
        /* Get client address and port */
        uint8_t address[32];
        char    tmpStr[INET6_ADDRSTRLEN];
        int addrSize = sizeof(address);
        getsockname(sock, (struct sockaddr*) address, (socklen_t *)&addrSize);
        struct sockaddr * clientAddr = (struct sockaddr*) address;
        if (clientAddr->sa_family == AF_INET) {
            struct sockaddr_in * clientAddrIn = (struct sockaddr_in*) address;
            transport->clientport = ntohs(clientAddrIn->sin_port);
            inet_ntop(AF_INET,&(clientAddrIn->sin_addr),tmpStr, sizeof(tmpStr));
        } else {
            struct sockaddr_in6 * clientAddrIn6 = (struct sockaddr_in6*) address;
            transport->clientport = ntohs(clientAddrIn6->sin6_port);
            inet_ntop(AF_INET,&(clientAddrIn6->sin6_addr),tmpStr, sizeof(tmpStr));
        }
        transport->client_addr = ism_transport_putString(transport, tmpStr);
    }

    TRACE(7, "moreOutgoing: connect=%u server=[%s]:%u server_name=%s client=[%s]:%u state=0x%x sock=%d result=%s (%d)\n", transport->index,
    		transport->server_addr, transport->serverport, transport->tobj->servername, transport->client_addr, transport->clientport,
    		connection->state, sock, strerror(err), err);
}


/*
 * Make an outgoing connection
 */
int ism_transport_connect(ism_transport_t * transport, ism_transport_t * ctransport,
        ism_tenant_t * tenant, struct ssl_ctx_st * tlsCTX) {
    int  rc;
    ism_tcp_init_transport(transport);
    transport->originated = 1;

    /*
     * The frame is added as part of transport.  For now we only support MQTT but this will need to be
     * generalized when we want to support additional outgoing protocols.
     */
    if (!strcmp(transport->protocol_family, "mqtt")) {
        transport->frame = ism_transport_frameMqtt;
        transport->addframe = ism_transport_addMqttFrame;
    } else {
        ism_common_setError(ISMRC_NotImplemented);
        return ISMRC_NotImplemented;
    }
    if (tlsCTX) {
        transport->tobj->tlsCTX = tlsCTX;
        transport->tobj->secured = 1;
    }


    /*
     * An outgoing connection is run in the same IOP thread as the incoming connection
     * but we use a separate index so we can tell them apart in the trace.  However if there
     * are an even number of IOP threads we doubima_pxtransport_tle increment the counter for outgoing
     * connection so that the connections are evenly distributed among the IOP threads.
     */
    /* If this comes from a client transport, use the same thread */
    if (ctransport) {
        transport->tid = ctransport->tid;
    } else {
        transport->tid = transport->index % numOfIOProcs;
    }

    rc = createOutgoingConnection(transport, tenant->server, tenant->server->port, ioProcessors[transport->tid]);
    transport->write_bytes += transport->tlsWriteBytes;
    transport->read_bytes += transport->tlsReadBytes;
    if (rc) {
        char * xbuf = alloca(2048);
        if (transport->connected) {
    	    transport->connected(transport, rc);
        }
        ism_common_formatLastError(xbuf, 2048);
        transport->close(transport, ISMRC_UnknownHost, 0, xbuf);
    }
    return rc;
}

/*
 * Create an outgoing multiplex connection
 */
XAPI int ism_transport_createMuxConnection(ism_transport_t * transport) {
    transport->frame = ism_transport_frameMux;
    transport->addframe = ism_transport_addMuxFrame;   /* Will be changed later */
    transport->tobj->tlsCTX = transport->server->tlsCTX;
    return createOutgoingConnection(transport, transport->server, 0, ioProcessors[transport->tid]);
}


/*
 * Create an outgoing kafka broker connection
 */
XAPI int  ism_kafka_createConnection(ism_transport_t * transport, ism_server_t * server) {
    transport->frame = ism_transport_frameKafka;
    transport->addframe = addKafkaFrame;
    transport->send = sendBytes;
    transport->tobj->tlsCTX = server->tlsCTX;
    __sync_add_and_fetch(&tcpStats.outgoingConnectionsCounter, 1);
    transport->tobj->id = __sync_add_and_fetch(&conCounter, 1);
    transport->index = transport->tobj->id;
    transport->tid = transport->tobj->id % numOfIOProcs;
    return createOutgoingConnection(transport, server, 0, ioProcessors[transport->tid]);
}

/*
 * Create an outgoing monitor connection (MQTT)
 */
XAPI int  ima_monitor_createConnection(ism_transport_t * transport, ism_server_t *server) {
    transport->frame = ism_transport_frameMqtt;
    transport->addframe = ism_transport_addMqttFrame;
    transport->send = sendBytes;
    transport->tobj->tlsCTX = server->tlsCTX;
	return createOutgoingConnection(transport, server, 0, monitorIoProcessor);
}

/*
 * Create an outgoing MQTT connection
 */
XAPI int  ism_proxy_createMQTTConnection(ism_transport_t * transport, const char * servername) {
    transport->frame = ism_transport_frameMqtt;
    transport->addframe = ism_transport_addMqttFrame;
    transport->send = sendBytes;
    transport->tobj->tlsCTX = transport->server->tlsCTX;
    __sync_add_and_fetch(&tcpStats.outgoingConnectionsCounter, 1);
    transport->tobj->id = __sync_add_and_fetch(&conCounter, 1);
    transport->index = transport->tobj->id;
    transport->tid = transport->tobj->id % numOfIOProcs;
    if (servername)
        transport->tobj->servername = ism_transport_putString(transport, servername);
    return createOutgoingConnection(transport, transport->server, 0, ioProcessors[transport->tid]);
}


int ism_proxy_getTCPStats(px_tcp_stats_t * stats) {
    memcpy(stats, &tcpStats, sizeof(px_tcp_stats_t));
    return 0;
}

/**
 * Dump Client IDs
 */
int ism_proxy_getAllActiveClientIDsList(const char * match, ism_json_t * jobj, int json, const char * name) {
   ism_connection_t * currCon;
   int count=0;
   char     datetime[100];
   FILE *fptr;
   time_t rawtime;
   time ( &rawtime );
   struct tm * cTime = localtime(&rawtime);
   snprintf(datetime, sizeof(datetime),
		 "_%04d%02d%02d_%02d%02d%02d",
		 cTime->tm_year + 1900, cTime->tm_mon + 1, cTime->tm_mday,
		 cTime->tm_hour, cTime->tm_min, cTime->tm_sec);

   char * output_path = alloca(2048) ;
   sprintf(output_path, "%s%s.txt","/var/dump/activeClients",datetime);

   if (json)
	   ism_json_startObject(jobj, name);
    else
	   ism_json_startArray(jobj, name);

   fptr = fopen(output_path,"w");

   if(fptr == NULL)
	{
	  return 1 ;
	}

   ism_json_putStringItem(jobj, "Output Path",output_path);
   pthread_mutex_lock(&conMutex);
   for (currCon = activeConnections; currCon != NULL ; currCon = currCon->conListNext) {
       ism_transport_t * transport = currCon->transport;
       if (transport && !transport->originated) {
    	   	   if (ism_common_match(transport->org, match)) {
			   const char * clientId = transport->clientID;
			   if(clientId!=NULL && strlen(clientId) > 8){
				   fprintf(fptr,"%s\n",clientId);
				   count++;
			   }
    	   	   }
       }
   }
   pthread_mutex_unlock(&conMutex);
   fclose(fptr);

   ism_json_putIntegerItem(jobj, "Total Active Connections",count);
   ism_json_endObject(jobj);
   return 0;
}
