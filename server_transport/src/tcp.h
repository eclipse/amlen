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
#include <transport.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

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
XAPI int ism_transport_startTCPEndpoint(ism_listener_t * listener);

/*
 * Update a TCP listener without restarting.
 */
XAPI int ism_transport_updateTCPEndpoint(ism_listener_t * endpoint);

/*
 * Terminate the transport
 * @return A return code, 0=good
 */
XAPI int ism_transport_termTCP(void);

XAPI int  ism_transport_getSocketInfo(ism_transport_t * transport, ismSocketInfoTcp *sockInfo);
/* Transport states */

#define ISM_TRANSPORT_STATE_RR              0x0001 /* READ_WANT_READ */
#define ISM_TRANSPORT_STATE_RW              0x0002 /* READ_WANT_WRITE */
#define ISM_TRANSPORT_STATE_WR              0x0004 /* WRITE_WANT_READ */
#define ISM_TRANSPORT_STATE_WW              0x0008 /* WRITE_WANT_WRITE */
#define ISM_TRANSPORT_CAN_WRITE             0x0010
#define ISM_TRANSPORT_CAN_READ              0x0020
#define ISM_TRANSPORT_HANDSHAKE_IN_PROCESS  0x0040
#define ISM_TRANSPORT_SHUTDOWN_IN_PROCESS   0x0080
#define ISM_TRANSPORT_CONNECTED             0x0100
#define ISM_TRANSPORT_DISCONNECTED          0x0200
#define ISM_TRANSPORT_ERROR                 0x0400
#define ISM_TRANSPORT_CONNECT_IN_PROCESS    0x0800
#define ISM_TRANSPORT_SHUTDOWN_FORCE        0x1000

typedef int (*comp_tls_init)(uint8_t isStoreCrit);

extern  comp_tls_init   engine_tls_init;


#ifdef __cplusplus
}
#endif
#endif
