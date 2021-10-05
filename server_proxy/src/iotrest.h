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

#ifndef IOTREST_H
#define IOTREST_H

#include <ismutil.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct px_http_stats_t {
    volatile int        httpPendingAuthenticationRequests;
    volatile int        httpPendingAuthorizationRequests;
    volatile int        httpConnections;
    volatile int        httpConnectedDev;
    volatile int        httpConnectedApp;
    volatile int        httpConnectedGWs;
    volatile uint64_t   httpConnectionsTotal;
    volatile uint64_t   httpC2PMsgsReceived;
    volatile uint64_t   httpP2SMsgsSent;
    volatile uint64_t   httpS2PMsgsReceived;
    volatile uint64_t   httpP2CMsgsSent;
    volatile int   		httpPendingGetCommandRequests;
} px_http_stats_t;

XAPI int ism_proxy_getHTTPStats(px_http_stats_t * stats);


#ifdef __cplusplus
}
#endif

#endif /* IOTREST_H */
