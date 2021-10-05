/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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

#ifndef  H_rmmRutils_H
#define  H_rmmRutils_H

#include "llmLogUtil.h"
/**-------------------------------------**/

#ifndef RMM_L_E_MEMORY_ALLOC
#define RMM_L_E_MEMORY_ALLOC  RMM_L_E_MEMORY_ALLOCATION_ERROR
#endif

#ifndef RMM_L_E_INTERNAL_ERROR
#define RMM_L_E_INTERNAL_ERROR RMM_L_E_INTERNAL_SOFTWARE_ERROR
#endif
/**-------------------------------------**/
static void rmmRmutex_lock(void);
static void rmmRmutex_unlock(void);
static void stop_stream_service(rStreamInfoRec *pst);
static int raise_stream_event(rStreamInfoRec *pst, int type, void **params, int nparams);
static void init_why_bad_msgs(WhyBadRec *wb) ;
static void get_why_bad_msgs(WhyBadRec *wb,TBHandle tbh);
static int add_why_bad_msg(WhyBadRec *wb,int severity, const char *caller, const char *msg) ;
static void clr_why_bad(WhyBadRec *wb,int why_bad_ind) ;
static void set_why_bad(WhyBadRec *wb,int why_bad_ind) ;
static void sortsn(int n, unsigned int *vec) ;
static void sortuli(int n, rmm_uint64 *vec, void *v_ind) ;
static void hash_add(HashStreamInfo **ht, HashStreamInfo *pst) ;
static void hash_del(HashStreamInfo **ht, HashStreamInfo *pst) ;
static HashStreamInfo *hash_get(HashStreamInfo **ht, rmmStreamID_t sid) ;
static uint32_t hash_sid(rmmStreamID_t sid) ;

static rStreamInfoRec *find_stream(rmmReceiverRec *rInfo, rmmStreamID_t sid) ;
static void remove_stream(rStreamInfoRec *pst) ;
static void add_stream(rStreamInfoRec *pst) ;

static int  tp_lock(rmmReceiverRec *rInfo, rTopicInfoRec *ptp, int twait, int id);
static void tp_unlock(rmmReceiverRec *rInfo, rTopicInfoRec *ptp);
static void return_packet(rmmReceiverRec *rInfo, rmmPacket *rPack);
static void trim_packetQ(rmmReceiverRec *rInfo, rTopicInfoRec  *ptp, int lock);
static void workMA(rmmReceiverRec *rInfo, rStreamInfoRec *pst) ;
static void print_topic_params(const rumRxQueueParameters *params, TBHandle tbh);

static void wakePP(rmmReceiverRec *rInfo, rStreamInfoRec *pst) ;

#endif
