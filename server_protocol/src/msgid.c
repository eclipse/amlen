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
#include <assert.h>
#include "mqtt.h"
#ifdef TRACE_COMP
#undef TRACE_COMP
#endif
#define TRACE_COMP Mqtt
static int g_numMsgIdMaps;
static int g_msgIdMapCapacity;
static ismHashMap **g_msgIdsMapTx;
static ismHashMap **g_msgIdsMapRx;
static struct drand48_data randBuffer;

void ism_msgid_init(int32_t numOfHashMaps, int32_t hashMapInitialCapacity) {
    int i;
    g_numMsgIdMaps = numOfHashMaps;
    g_msgIdMapCapacity = hashMapInitialCapacity;

    g_msgIdsMapTx = malloc(sizeof(ismHashMap *) * numOfHashMaps);
    g_msgIdsMapRx = malloc(sizeof(ismHashMap *) * numOfHashMaps);

    for (i = 0; i < numOfHashMaps; i++) {
        g_msgIdsMapTx[i] = ism_common_createHashMap((uint32_t)hashMapInitialCapacity, HASH_INT64);
        g_msgIdsMapRx[i] = ism_common_createHashMap((uint32_t)hashMapInitialCapacity, HASH_INT64);
    }
    srand48_r(time(NULL), &randBuffer);

    TRACE(5, "Initializing %d global Rx and Tx msgid hashmaps of size %d\n", g_numMsgIdMaps, g_msgIdMapCapacity);
}

/*
 * Message ID list.
 * There is one list for each session which uses assigned message ID.
 * There are a set of slots and each one is the head of a chain.
 * Since we use the next available msgid, it should be common to
 * only use the slots as long as there are a small number of
 * outstanding messages.
 */
struct ism_msgid_list_t {
    ism_msgid_info_t * freeList;
    ismHashMap * idsMap;
    ism_transport_t * transport;
    uint64_t client_uid;
    uint16_t inUseCount;
    uint16_t freeCount;
    uint16_t range;
    uint16_t rsrv;
};

#define COMPUTE_KEY(k,m,c)  \
  k = (c);                  \
  k <<=16;                  \
  k = k | (m);

ism_msgid_list_t * ism_create_msgid_list(ism_transport_t *transport, int isRX, uint16_t range) {
    ism_msgid_list_t * mlist;
    mqttProtoObj_t * pobj = (mqttProtoObj_t*) transport->pobj;
    int mapIdx;
    mlist = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,64),1, sizeof(*mlist));
    mlist->transport = transport;
    mlist->client_uid = pobj->client_uid;
    mapIdx = mlist->client_uid % g_numMsgIdMaps;

    if (isRX){
        mlist->idsMap = g_msgIdsMapRx[mapIdx];
    } else {
        mlist->idsMap = g_msgIdsMapTx[mapIdx];
    }
    mlist->range = range;
    return mlist;
}

/*
 * Free up the message IDs list
 */
void ism_msgid_freelist(struct ism_msgid_list_t * mlist) {
    if (mlist) {
        ism_msgid_info_t * entry;
        while (mlist->freeList) {
            entry = mlist->freeList;
            mlist->freeList = entry->next;
            ism_common_free(ism_memory_protocol_misc,entry);
        }
        if (mlist->inUseCount) {
            uint16_t msgid;
            ism_common_HashMapLock(mlist->idsMap);
            for (msgid = 1; (mlist->inUseCount && (msgid <= 0xFFFF)); msgid++) {
                uint64_t key;
                COMPUTE_KEY(key,msgid,mlist->client_uid);
                entry = ism_common_removeHashMapElement(mlist->idsMap, &key, sizeof(key));
                if (entry) {
                    ism_common_free(ism_memory_protocol_misc,entry);
                    mlist->inUseCount--;
                }
            }
            ism_common_HashMapUnlock(mlist->idsMap);
        }
        ism_common_free(ism_memory_protocol_misc,mlist);
    }
}

/*
 * Free a message ID
 */
static void freeMsgInfo(ism_msgid_list_t * mlist, ism_msgid_info_t * entry) {
    if (entry) {
        mlist->inUseCount--;
        if (mlist->freeCount < mlist->range){
            mlist->freeCount++;
            entry->handle = 0;
            entry->next = mlist->freeList;
            mlist->freeList = entry;
        } else {
            ism_common_free(ism_memory_protocol_misc,entry);
        }
    }
}

int  ism_msgid_addMsgIdInfo(ism_msgid_list_t * mlist, __uint128_t handle, uint16_t msgid, uint16_t state) {
    ism_msgid_info_t * entry;
    uint64_t key;
    COMPUTE_KEY(key,msgid,mlist->client_uid);
    if (mlist->freeList){
        entry = mlist->freeList;
        mlist->freeList = entry->next;
        entry->next = NULL;
        mlist->freeCount--;
    } else {
        entry = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,69),1, sizeof(ism_msgid_info_t));
    }
    entry->msgid = msgid;
    entry->handle = handle;
    entry->state = state;
    entry->pending = 1;
    ism_common_putHashMapElementLock(mlist->idsMap, &key, sizeof(key), entry, NULL);
    mlist->inUseCount++;

    return 0;
}

/*
 * Check if the message id is in the list.
 */
__uint128_t ism_msgid_delMsgIdInfo(ism_msgid_list_t * mlist, uint16_t msgid, int *pPending) {
    ism_msgid_info_t * entry;
    uint64_t key;
    __uint128_t result = 0;
    COMPUTE_KEY(key,msgid,mlist->client_uid);
    entry = ism_common_removeHashMapElementLock(mlist->idsMap, &key, sizeof(key));
    if(entry) {
        result = entry->handle;
        if(pPending)
            *pPending = entry->pending;
        freeMsgInfo(mlist, entry);
    }
    return result;
}

/*
 * Check if the message id is in the list.
 */
ism_msgid_info_t * ism_msgid_getMsgIdInfo(ism_msgid_list_t * mlist, uint16_t msgid) {
    ism_msgid_info_t * entry;
    uint64_t key;
    COMPUTE_KEY(key,msgid,mlist->client_uid);
    entry = ism_common_getHashMapElementLock(mlist->idsMap, &key, sizeof(key));
    return entry;
}
