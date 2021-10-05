/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <alloca.h>
#include <errno.h>
#include <time.h>

#ifndef XAPI
#define XAPI
#endif

#include <ismrc.h>
#include "mccTrace.h"
#include "mccLUSinternal.h"
#include "mccLookupSet.h"
#include "RemoteServerInfo.h"
#include "mccBFSet.h"
#include "mccWildcardBFSet.h"

#define USE_LOCK 0

#if USE_LOCK
#define BFSet_rwlock_t           pthread_rwlock_t
#define BFSet_rwlock_init(x,y)   pthread_rwlock_init(x,y)
#define BFSet_rwlock_destroy(x)  pthread_rwlock_destroy(x)
#define BFSet_rwlock_wrlock(x)   pthread_rwlock_wrlock(x)
#define BFSet_rwlock_rdlock(x)   pthread_rwlock_rdlock(x)
#define BFSet_rwlock_unlock(x)   pthread_rwlock_unlock(x)
#else
#define BFSet_rwlock_t           int
#define BFSet_rwlock_init(x,y)   0
#define BFSet_rwlock_destroy(x)
#define BFSet_rwlock_wrlock(x)
#define BFSet_rwlock_rdlock(x)
#define BFSet_rwlock_unlock(x)
#endif

typedef struct mcc_ebfsLL
{
    struct mcc_ebfsLL     *next ;
    mcc_hash_t             hashParams[1] ;
    mcc_bfs_BFSetHandle_t  ebfs ;
} mcc_ebfsLL ; 

#define  NODE_STATE_ACTIVE    1
#define  NODE_STATE_HAS_EBF   2
#define  NODE_STATE_HAS_WBF   4
#define  NODE_STATE_HAS_RA    8

typedef struct
{
    mcc_ebfsLL                       *ebfsLL ;
    struct ismCluster_RemoteServer_t  node[1] ;
    int                               flags;
} mcc_node_t ; 


struct mcc_lus_LUSet_t
{
    mcc_ebfsLL                 *ebfs1st ;
    mcc_wcbfs_WCBFSetHandle_t   wbfs ;
    mcc_node_t                 *nodeMap ;
    BFSet_rwlock_t              lock[1];
    int                         mapSize ;
    int                         state ;
    int                         numRA;
    int                         id ; 
    size_t                      dbg_cnt[4];
};

static uint8_t   mask1[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80} ;
static int id; 

/*********************************************************************/

XAPI ism_rc_t mcc_lus_createLUSet(mcc_lus_LUSetHandle_t *phLUSetHandle)
{
    struct mcc_lus_LUSet_t *lus ;
    size_t size ;

    if (!phLUSetHandle )
    {
        TRACE(1,"Error: %s failed, NULL argument, rc=%d\n", __FUNCTION__, ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

    size = sizeof(struct mcc_lus_LUSet_t) ;
    if (!(lus = (mcc_lus_LUSetHandle_t)ism_common_malloc(ISM_MEM_PROBE(ism_memory_cluster_misc,19),size)) )
        return ISMRC_AllocateError ;
    memset(lus,0,size) ;
    lus->id = __sync_add_and_fetch(&id,1);

    if ( BFSet_rwlock_init(lus->lock,NULL) )
    {
        ism_common_free(ism_memory_cluster_misc,lus) ;
        return ISMRC_Error ;
    }
    lus->state = 1 ;
    *phLUSetHandle = lus ;
    return ISMRC_OK ;
}

/*********************************************************************/

XAPI ism_rc_t mcc_lus_deleteLUSet(mcc_lus_LUSetHandle_t *phLUSetHandle)
{
    struct mcc_lus_LUSet_t *lus ;
    mcc_ebfsLL *eLL ;
    int rc=ISMRC_OK ;

    if (!phLUSetHandle )
        return ISMRC_Error ;
    lus = *phLUSetHandle ;
    if (!lus )
        return ISMRC_Error ;

    *phLUSetHandle = NULL ;
    BFSet_rwlock_wrlock(lus->lock) ;
    do
    {
        lus->state = 0 ;
        if ( lus->nodeMap && lus->mapSize )
            ism_common_free(ism_memory_cluster_misc,lus->nodeMap) ;
        while ( lus->ebfs1st && rc==ISMRC_OK )
        {
            eLL = lus->ebfs1st ;
            lus->ebfs1st = eLL->next ; 
            rc = mcc_bfs_deleteBFSet(eLL->ebfs);
            ism_common_free(ism_memory_cluster_misc,eLL) ; 
        }  
        if ( rc == ISMRC_OK && lus->wbfs )
            rc = mcc_wcbfs_deleteWCBFSet(lus->wbfs) ;
    } while(0) ;
    BFSet_rwlock_unlock(lus->lock) ;
    BFSet_rwlock_destroy(lus->lock) ;
    ism_common_free(ism_memory_cluster_misc,lus) ;
    return rc ;
}

/*********************************************************************/

static mcc_node_t *getNode(mcc_lus_LUSetHandle_t hLUSetHandle,ismCluster_RemoteServerHandle_t hRemoteServer, int *rc)
{
    mcc_lus_LUSetHandle_t lus=hLUSetHandle ;
    int i;
    mcc_node_t *pNode ;

    i = hRemoteServer->index ;
    if ( i >= lus->mapSize )
    {
        void *ptr ;
        size_t s = (i + 64) & (~63) ;
        if (!(ptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_cluster_misc,24),lus->nodeMap,s*sizeof(mcc_node_t))) )
        {
            *rc = ISMRC_AllocateError ;
            return NULL ;
        }
        lus->nodeMap = ptr ;
        memset(lus->nodeMap+lus->mapSize,0,(s-lus->mapSize)*sizeof(mcc_node_t)) ;
        lus->mapSize = s ;
    }
    pNode = lus->nodeMap+i ;
    if ( pNode->flags& NODE_STATE_ACTIVE)
    {
        if ( pNode->node->engineHandle != hRemoteServer->engineHandle )
        {
            *rc = ISMRC_Error ;
            return NULL ;
        }
    }
    else
    {
        memset(pNode,0,sizeof(*pNode)) ;
        memcpy(pNode->node, hRemoteServer, sizeof(*hRemoteServer)) ;
        pNode->flags |= NODE_STATE_ACTIVE ;
    }
    return pNode;
}
/*********************************************************************/

XAPI ism_rc_t mcc_lus_addBF(mcc_lus_LUSetHandle_t hLUSetHandle,
        ismCluster_RemoteServerHandle_t hRemoteServer,
        const char *pBFBytes, size_t BFLen, mcc_hash_HashType_t hashType,
        int numHashValues, int fIsWildcard)
{
    mcc_lus_LUSetHandle_t lus=hLUSetHandle ;
    int i;
    int rc=ISMRC_OK ;

    if ( !(hLUSetHandle && hRemoteServer && pBFBytes && BFLen) )
        return ISMRC_Error ;

    BFSet_rwlock_wrlock(lus->lock) ;
    do
    {
        mcc_hash_t hashParams[1] ;
        mcc_node_t *pNode ;
        if (!(pNode = getNode(lus,hRemoteServer,&rc)))
            break;
        i = hRemoteServer->index ;
        memset(hashParams,0,sizeof(hashParams)) ;
        hashParams->hashType = hashType ;
        hashParams->numHashValues = numHashValues ;
        if ( fIsWildcard )
        {
            if (!lus->wbfs )
            {
                if ((rc = mcc_wcbfs_createWCBFSet(&lus->wbfs)) != ISMRC_OK )
                    break ;
            }
            rc = mcc_wcbfs_addBF(lus->wbfs, i, hashParams, pBFBytes, BFLen, hRemoteServer);
            if ( rc != ISMRC_OK )
                break ;
            pNode->flags |= NODE_STATE_HAS_WBF ;
        }
        else
        {
            mcc_ebfsLL *eLL ;
            if ( (eLL=pNode->ebfsLL) )
            {
                if ( memcmp(eLL->hashParams,hashParams,sizeof(mcc_hash_t)) )
                {
                    if ((rc = mcc_bfs_deleteBF(eLL->ebfs, i)) != ISMRC_OK )
                        break ;
                    pNode->ebfsLL = NULL ;
                }
                else
                {
                    rc = mcc_bfs_replaceBF(eLL->ebfs, i, pBFBytes, BFLen) ;
                    break ;
                }
            }
            for ( eLL=lus->ebfs1st ; eLL && memcmp(eLL->hashParams,hashParams,sizeof(mcc_hash_t)) ; eLL = eLL->next ) ; // empty body
            if (!eLL )
            {
                mcc_bfs_BFSetParameters_t params[1] ;
                if (!(eLL = ism_common_malloc(ISM_MEM_PROBE(ism_memory_cluster_misc,25),sizeof(mcc_ebfsLL))) )
                {
                    rc = ISMRC_AllocateError ;
                    break ;
                }
                memset(eLL,0,sizeof(mcc_ebfsLL)) ;
                memcpy(eLL->hashParams,hashParams,sizeof(mcc_hash_t)) ;
                memset(params,0,sizeof(params)) ;
                params->numBFs        = 64 ;
                params->maxBFLen      = BFLen ;
                params->numHashValues = numHashValues ;
                params->hashType      = hashType ;
                params->id            = lus->id ; 
                params->dbg_cnt       = lus->dbg_cnt ; 
                if ((rc = mcc_bfs_createBFSet(&eLL->ebfs, params)) != ISMRC_OK )
                {
                    ism_common_free(ism_memory_cluster_misc,eLL) ;
                    break ;
                }
                eLL->next = lus->ebfs1st ;
                lus->ebfs1st = eLL ;
            }
            pNode->ebfsLL = eLL ;
            pNode->flags |= NODE_STATE_HAS_EBF ;
            rc = mcc_bfs_addBF(eLL->ebfs, i, pBFBytes, BFLen, hRemoteServer);
        }
    } while(0);
    BFSet_rwlock_unlock(lus->lock) ;
    return rc ;
}

/*********************************************************************/

XAPI ism_rc_t mcc_lus_updateBF(mcc_lus_LUSetHandle_t hLUSetHandle,
        ismCluster_RemoteServerHandle_t hRemoteServer,
        int fIsWildcard, const int32_t *pUpdates, size_t updatesLen)
{
    mcc_lus_LUSetHandle_t lus=hLUSetHandle ;
    int i;
    int rc=ISMRC_OK ;

    if ( !(hLUSetHandle && hRemoteServer && pUpdates && updatesLen) )
        return ISMRC_Error ;

    BFSet_rwlock_wrlock(lus->lock) ;
    do
    {
        mcc_node_t *pNode ;
        i = hRemoteServer->index ;
        pNode = lus->nodeMap+i ;
        if ( i >= lus->mapSize || !(pNode->flags&NODE_STATE_ACTIVE) )
            rc = ISMRC_Error ;
        else
            if ( fIsWildcard )
            {
                if (!lus->wbfs )
                    rc = ISMRC_Error ;
                else
                    rc = mcc_wcbfs_updateBF(lus->wbfs, i, pUpdates, updatesLen);
            }
            else
            {
                mcc_ebfsLL *eLL ;
                if ( (eLL=pNode->ebfsLL) )
                    rc = mcc_bfs_updateBF(eLL->ebfs, i, pUpdates, updatesLen);
                else
                    rc = ISMRC_Error ;
            }
    } while(0);
    BFSet_rwlock_unlock(lus->lock) ;
    return rc ;
}

/*********************************************************************/

XAPI ism_rc_t mcc_lus_deleteBF(mcc_lus_LUSetHandle_t hLUSetHandle,
        ismCluster_RemoteServerHandle_t hRemoteServer,
        int fIsWildcard)
{
    mcc_lus_LUSetHandle_t lus=hLUSetHandle ;
    int i;
    int rc=ISMRC_OK ;

    if ( !(hLUSetHandle && hRemoteServer) )
        return ISMRC_Error ;

    BFSet_rwlock_wrlock(lus->lock) ;
    do
    {
        mcc_node_t *pNode ;
        i = hRemoteServer->index ;
        pNode = lus->nodeMap+i ;
        if ( i >= lus->mapSize || !(pNode->flags&NODE_STATE_ACTIVE) )
            rc = ISMRC_Error ;
        else
            if ( fIsWildcard )
            {
                if ( !(pNode->flags & NODE_STATE_HAS_WBF) )
                    rc = ISMRC_NotFound ;
                else
                    if (!lus->wbfs )
                        rc = ISMRC_Error ;
                    else
                        rc = mcc_wcbfs_deleteBF(lus->wbfs, i);
                pNode->flags &= ~NODE_STATE_HAS_WBF;
            }
            else
            {
                mcc_ebfsLL *eLL ;
                if ( !(pNode->flags & NODE_STATE_HAS_EBF) )
                    rc = ISMRC_NotFound ;
                else
                    if ( (eLL=pNode->ebfsLL) )
                    {
                        rc = mcc_bfs_deleteBF(eLL->ebfs, i);
                        pNode->ebfsLL = NULL ;
                    }
                    else
                        rc = ISMRC_NotFound ;
                pNode->flags &= ~NODE_STATE_HAS_EBF;
            }
    } while(0);
    BFSet_rwlock_unlock(lus->lock) ;
    return rc ;
}

/*********************************************************************/

XAPI ism_rc_t mcc_lus_addPattern(mcc_lus_LUSetHandle_t hLUSetHandle,
        ismCluster_RemoteServerHandle_t hRemoteServer,
        mcc_lus_Pattern_t *pPattern)
{
    mcc_lus_LUSetHandle_t lus=hLUSetHandle ;
    int rc=ISMRC_OK ;

    if ( !(hLUSetHandle && hRemoteServer && pPattern) )
        return ISMRC_Error ;

    BFSet_rwlock_wrlock(lus->lock) ;
    do
    {
        mcc_node_t *pNode ;
        if (!(pNode = getNode(lus,hRemoteServer,&rc)))
            break;
        if (!lus->wbfs )
        {
            if ((rc = mcc_wcbfs_createWCBFSet(&lus->wbfs)) != ISMRC_OK )
                break ;
        }
        rc = mcc_wcbfs_addPattern(lus->wbfs, pNode->node->index, pPattern);
    } while(0);
    BFSet_rwlock_unlock(lus->lock) ;
    return rc ;
}

/*********************************************************************/

XAPI ism_rc_t mcc_lus_deletePattern(mcc_lus_LUSetHandle_t hLUSetHandle,
        ismCluster_RemoteServerHandle_t hRemoteServer,
        uint64_t patternId)
{
    mcc_lus_LUSetHandle_t lus=hLUSetHandle ;
    int i;
    int rc=ISMRC_OK ;

    if ( !(hLUSetHandle && hRemoteServer ) )
        return ISMRC_Error ;

    BFSet_rwlock_wrlock(lus->lock) ;
    do
    {
        mcc_node_t *pNode ;
        i = hRemoteServer->index ;
        pNode = lus->nodeMap+i ;
        if ( i >= lus->mapSize || !(pNode->flags&NODE_STATE_ACTIVE) )
            rc = ISMRC_Error ;
        else
            if ( pNode->node->engineHandle != hRemoteServer->engineHandle )
                rc = ISMRC_Error ;
            else
                if (!lus->wbfs )
                    rc = ISMRC_Error ;
                else
                    rc = mcc_wcbfs_deletePattern(lus->wbfs, i, patternId);
    } while(0);
    BFSet_rwlock_unlock(lus->lock) ;
    return rc ;
}

/*********************************************************************/

XAPI ism_rc_t mcc_lus_deleteServer(mcc_lus_LUSetHandle_t hLUSetHandle,
        ismCluster_RemoteServerHandle_t hRemoteServer)
{
    mcc_lus_LUSetHandle_t lus=hLUSetHandle ;
    int i;
    int rc=ISMRC_OK ;

    if ( !(hLUSetHandle && hRemoteServer ) )
        return ISMRC_Error ;

    BFSet_rwlock_wrlock(lus->lock) ;
    do
    {
        mcc_node_t *pNode ;
        i = hRemoteServer->index ;
        pNode = lus->nodeMap+i ;
        if ( i >= lus->mapSize || !(pNode->flags&NODE_STATE_ACTIVE) )
            rc = ISMRC_NotFound ;
        else
            if ( pNode->node->engineHandle != hRemoteServer->engineHandle )
                rc = ISMRC_Error ;
            else
            {
                if ( lus->wbfs && (pNode->flags & NODE_STATE_HAS_WBF) )
                {
                    rc = mcc_wcbfs_deleteBF(lus->wbfs, i);
                }
                if ( pNode->ebfsLL && (pNode->flags & NODE_STATE_HAS_EBF) )
                {
                    int rc1 = mcc_bfs_deleteBF(pNode->ebfsLL->ebfs, i);
                    if ( rc1 != ISMRC_OK && (rc == ISMRC_OK || rc1 != ISMRC_NotFound) )
                        rc = rc1 ;
                }
                if ( (pNode->flags&NODE_STATE_HAS_RA) )
                    lus->numRA--;
                memset(pNode,0,sizeof(*pNode)) ;
            }
    } while(0);
    BFSet_rwlock_unlock(lus->lock) ;
    return rc ;
}
/*********************************************************************/


XAPI ism_rc_t mcc_lus_setRouteAll(mcc_lus_LUSetHandle_t hLUSetHandle,
        ismCluster_RemoteServerHandle_t hRemoteServer,
        int fRouteAll)
{
    mcc_lus_LUSetHandle_t lus=hLUSetHandle ;
    int rc=ISMRC_OK ;

    if ( !(hLUSetHandle && hRemoteServer) )
        return ISMRC_Error ;

    BFSet_rwlock_wrlock(lus->lock) ;
    do
    {
        mcc_node_t *pNode ;
        if (!(pNode = getNode(lus,hRemoteServer,&rc)))
            break;
        if ( fRouteAll )
        {
            if (!(pNode->flags&NODE_STATE_HAS_RA))
            {
                pNode->flags |= NODE_STATE_HAS_RA;
                lus->numRA++;
            }
        }
        else
        {
            if ( (pNode->flags&NODE_STATE_HAS_RA))
            {
                pNode->flags &= ~NODE_STATE_HAS_RA;
                lus->numRA--;
            }
        }
    } while(0);
    BFSet_rwlock_unlock(lus->lock) ;
    return rc ;

}
/*********************************************************************/

XAPI ism_rc_t mcc_lus_lookup(mcc_lus_LUSetHandle_t hLUSetHandle,
        ismCluster_LookupInfo_t *pLookupInfo)
{
    mcc_lus_LUSetHandle_t lus=hLUSetHandle ;
    int i,j,k,nr, rl ;
    uint8_t *skip ;
    ismCluster_RemoteServerHandle_t *r ;
    int rc=ISMRC_OK ;
    //  int line=0 ;

    if ( !(hLUSetHandle && pLookupInfo) )
    {
        //  line = __LINE__ ;
        rc = ISMRC_Error ;
        goto _ret ;
    }

    BFSet_rwlock_rdlock(lus->lock) ;
    do
    {
        mcc_ebfsLL *eLL ;
        size_t size = lus->mapSize>>3 ;
        skip = alloca(size) ;
        memset(skip,0,size) ;
        for ( i=0 ; i<pLookupInfo->numDests ; i++ )
        {
            ismCluster_RemoteServerHandle_t p = pLookupInfo->phMatchedServers[i] ;
            if ( p->deletedFlag )
                continue;
            j = p->index >> 3 ;
            k = p->index &  7 ;
            skip[j] |= mask1[k] ;
        }
        if ( lus->numRA )
        {
            int n;
            for ( i=0,n=0 ; n<lus->numRA ; i++ )
            {
                mcc_node_t *pNode = lus->nodeMap+i ;
                if ( pNode->flags&NODE_STATE_HAS_RA )
                {
                    n++ ;
                    ismCluster_RemoteServerHandle_t p = pNode->node ;
                    j = p->index >> 3 ;
                    k = p->index &  7 ;
                    if (!(skip[j]&mask1[k]) )
                    {
                        if ( pLookupInfo->numDests >= pLookupInfo->destsLen )
                        {
                            rc = ISMRC_ClusterArrayTooSmall ;
                            pLookupInfo->numDests = -1 ;
                            //              line = __LINE__ ;
                            break ;
                        }
                        pLookupInfo->phDests[pLookupInfo->numDests++] = p->engineHandle ;
                        skip[j] |= mask1[k] ;
                    }
                }
            }
            if ( rc != ISMRC_OK )
                break;
        }
        rl = pLookupInfo->destsLen - pLookupInfo->numDests ;
        size = rl * sizeof(ismCluster_RemoteServerHandle_t *) ;
        r = alloca(size) ;
        for ( eLL=lus->ebfs1st ; eLL && rc==ISMRC_OK ; eLL = eLL->next )
        {
            if ( (rc = mcc_bfs_lookup(eLL->ebfs, pLookupInfo->pTopic, pLookupInfo->topicLen, skip, r, rl, &nr)) != ISMRC_OK )
            {
                if ( rc == ISMRC_ClusterArrayTooSmall )
                    pLookupInfo->numDests = -1 ;
                //        line = __LINE__ ;
                break ;
            }
            rl -= nr ;
            for ( i=0 ; i<nr ; i++ )
            {
                ismCluster_RemoteServerHandle_t p = r[i] ;
                //        if ( p->deletedFlag )
                //          continue;
                if ( pLookupInfo->numDests >= pLookupInfo->destsLen )
                {
                    rc = ISMRC_ClusterArrayTooSmall ;
                    pLookupInfo->numDests = -1 ;
                    //          line = __LINE__ ;
                    break ;
                }
                pLookupInfo->phDests[pLookupInfo->numDests++] = p->engineHandle ;
                j = p->index >> 3 ;
                k = p->index &  7 ;
                skip[j] |= mask1[k] ;
            }
        }
        if ( rc != ISMRC_OK )
            break ;
        if (!lus->wbfs )
            break ;
        if ( (rc = mcc_wcbfs_lookup(lus->wbfs, pLookupInfo->pTopic, pLookupInfo->topicLen, skip, r, rl, &nr)) != ISMRC_OK )
        {
            if ( rc == ISMRC_ClusterArrayTooSmall )
                pLookupInfo->numDests = -1 ;
            //      line = __LINE__ ;
            break ;
        }
        for ( i=0 ; i<nr ; i++ )
        {
            ismCluster_RemoteServerHandle_t p = r[i] ;
            //      if ( p->deletedFlag )
            //        continue;
            if ( pLookupInfo->numDests >= pLookupInfo->destsLen )
            {
                rc = ISMRC_ClusterArrayTooSmall ;
                pLookupInfo->numDests = -1 ;
                //        line = __LINE__ ;
                break ;
            }
            pLookupInfo->phDests[pLookupInfo->numDests++] = p->engineHandle ;
        }
    } while(0);
    BFSet_rwlock_unlock(lus->lock) ;
    _ret:
    //  if ( rc != ISMRC_OK ) printf("!!! %s: failed at %d\n",__FUNCTION__,line);
    return rc ;
}
