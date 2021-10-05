/*
 * Copyright (c) 2019-2021 Contributors to the Eclipse Foundation
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

#include <commonMemHandler.h>
#include <stdint.h>

#include "ismutil.h"


typedef struct ism_tls_t {
    char        tname[16];           /* Thread name */
    ism_ts_t *  trc_ts;
    ism_time_t  tz_set_time;
    ism_thread_cleanup_func_t thread_cleanup; /* The function to be invoked at thread exit time */
    void *      cleanup_parm;        /* Parameter to be passed to thread cleanup function */
    int         errcode;             /* Last error */
    int         count;               /* The replacement data count */
    int         data_alloc;          /* Allocated length of the data at the end of this structure */
    int         data_len;            /* Data length */
    int         tname_len;           /* Thread name length */
    uint64_t    uuid_rand;           /* Random node per thread */
    int         uuid_seq;            /* Sequence with random seed */
#ifdef COMMON_MALLOC_WRAPPER
    ism_threadmemusage_t     *memUsage;      ///< Per-thread memory accounting
#endif
    int         resvi;
    int         lineno;              /* Line number where the error was reported */
    char        filename[32];        /* Filename where the error was reported */
    /* The data follows this */
} ism_tls_t;

#ifndef ISM_COMMON_THREADDATA_HOME
extern __thread ism_tls_t *ism_common_threaddata;
#endif
