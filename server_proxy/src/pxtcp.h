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

#ifndef __TCP_DEFINED
#define __TCP_DEFINED

#include <ismutil.h>
#include <pxtransport.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>

/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif


/*
 * Initialize the transport component
 * @return A return code, 0=good
 */
int ism_transport_initTCP();


/*
 * Start the transport
 * @return A return code, 0=good
 */
XAPI int ism_transport_startTCP(void);

/*
 * Start a TCP listener.
 * This is called whenever a listener has been modified to perform that change.
 * This includes starting and stopping the listener.
 */
XAPI int ism_transport_startTCPEndpoint(ism_endpoint_t * listener);

/*
 * Update a TCP listener without restarting.
 */
XAPI int ism_transport_updateTCPEndpoint(ism_endpoint_t * endpoint);

/*
 * Terminate the transport
 * @return A return code, 0=good
 */
XAPI int ism_transport_termTCP(void);

/**
 * Print transport statistics
 */
XAPI void ism_transport_printStatsTCP(void);

/*
 * Get the SSL object from the transport
 */
XAPI struct ssl_st * ism_transport_getSSL(ism_transport_t * transport);

/*
 * Get the unique connId from the transport
 */
XAPI uint64_t ism_transport_getConnId(ism_transport_t * transport);


/* Transport states */

#define ISM_TRANSPORT_STATE_RR              0x0001 /* READ_WANT_READ (not used)) */
#define ISM_TRANSPORT_STATE_RW              0x0002 /* READ_WANT_WRITE (used in TLS processing) */
#define ISM_TRANSPORT_STATE_WR              0x0004 /* WRITE_WANT_READ (used in TLS processing */
#define ISM_TRANSPORT_STATE_WW              0x0008 /* WRITE_WANT_WRITE (not used) */
#define ISM_TRANSPORT_CAN_WRITE             0x0010
#define ISM_TRANSPORT_CAN_READ              0x0020
#define ISM_TRANSPORT_HANDSHAKE_IN_PROCESS  0x0040
#define ISM_TRANSPORT_SHUTDOWN_IN_PROCESS   0x0080
#define ISM_TRANSPORT_CONNECTED             0x0100
#define ISM_TRANSPORT_DISCONNECTED          0x0200
#define ISM_TRANSPORT_ERROR                 0x0400
#define ISM_TRANSPORT_CONNECT_IN_PROCESS    0x0800
#define ISM_TRANSPORT_SHUTDOWN_FORCE        0x1000
#define ISM_TRANSPORT_DELAY_WAIT            0x2000


#define JM_Connect      0
#define JM_Disconnect   1
#define JM_Failed       2
#define JM_Active       3
#define JM_Info         4

int ism_proxy_jsonMessage(concat_alloc_t * buf, ism_transport_t * transport, int which, int rc, const char * reason);
int ism_proxy_jsonGWNotification(concat_alloc_t * buf, ism_transport_t * transport, const char * request,
        const char * topic, const char * typeID, const char * devID, int rc, const char * reason);

void ism_proxy_connectMsg(ism_transport_t * transport);
void ism_proxy_disconnectMsg(ism_transport_t * transport, int rc, const char * reason);
void ism_proxy_failedMsg(ism_transport_t * transport, int rc, const char * reason);
int  ism_proxy_meteringMsg(ism_transport_t * transport, char * timest);
int  ism_proxy_GWNotify(ism_transport_t * transport, const char * request, const char * topic,
        const char * typeID, const char * devID, int rc, const char * reason);

typedef struct px_tcp_stats_t {
    volatile uint64_t tcpC2PDataReceived;
    volatile uint64_t tcpC2PDataSent;
    volatile uint64_t tcpP2SDataReceived;
    volatile uint64_t tcpP2SDataSent;
    volatile uint64_t incomingConnectionsTotal;
    volatile uint64_t outgoingConnectionsTotal;
    volatile uint64_t incomingTLSv1_1;
    volatile uint64_t incomingTLSv1_2;
    volatile uint64_t incomingTLSv1_3;
    volatile int      incomingConnectionsCounter;
    volatile int      outgoingConnectionsCounter;
    volatile int      pendingOutgoingConnectionsCounter;
} px_tcp_stats_t;

XAPI int ism_proxy_getTCPStats(px_tcp_stats_t * stats);

typedef struct px_mux_stats_t {
    volatile uint64_t virtualConnectionsTotal;
    volatile uint64_t physicalConnectionsTotal;
} px_mux_stats_t;
XAPI int ism_proxy_getMuxStats(px_mux_stats_t * stats, int * pCount);

#ifdef __cplusplus
}
#endif
#endif
