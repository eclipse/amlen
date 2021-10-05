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

//*********************************************************************
/// @file  mempool.c
/// @brief Allocate memory for multiple uses that will all be freed at
///        the same time
//*********************************************************************
#define TRACE_COMP Engine
#include <stdint.h>
#include <assert.h>
#include <pthread.h>

#include <engineInternal.h>
#include <memHandler.h>

#include <mempool.h>
#include <mempoolInternal.h>

static int32_t iemp_moveToNewPage( ieutThreadData_t *pThreadData
                                 , iempMemPoolOverallHeader_t *poolHdr
                                 , iempMemPoolPageHeader_t **pPage);

int32_t iemp_createMemPool( ieutThreadData_t *pThreadData
                          , iemem_memoryType memTypeAndProbe
                          , size_t reservedMem
                          , size_t initialPageSize
                          , size_t subsequentPageSize
                          , iempMemPoolHandle_t *pMemPoolHdl)
{
    int32_t rc = OK;

    //Check the first page is big enough to store the headers/reservedmem we store on it (it should be much
    //bigger, to have space for normal memory to be used

    //Round up the reservedMem so it ends on an 8byte boundary
    size_t roundedReservedMem;
    size_t sizeToEndOfReservedMem =  sizeof(iempMemPoolPageHeader_t)
                                   + sizeof(iempMemPoolOverallHeader_t)
                                   + reservedMem;
    if ((sizeToEndOfReservedMem & 0x07) != 0)
    {
        roundedReservedMem = reservedMem + (8 - (sizeToEndOfReservedMem & 0x07));
        assert(((  sizeof(iempMemPoolPageHeader_t)
                 + sizeof(iempMemPoolOverallHeader_t)
                 + roundedReservedMem) &0x7) == 0);
    }
    else
    {
        roundedReservedMem = reservedMem;
    }

    if( initialPageSize <=  (  sizeof(iempMemPoolPageHeader_t)
                             + sizeof(iempMemPoolOverallHeader_t)
                             + roundedReservedMem))
    {
        ieutTRACEL(pThreadData, initialPageSize, ENGINE_ERROR_TRACE, FUNCTION_IDENT "MemPool too small: %lu for %lu (rounded: %lu) reserved\n",
                                                     __func__, initialPageSize, reservedMem,  roundedReservedMem);
        rc = ISMRC_AllocateError;
        goto mod_exit;
    }

    assert(subsequentPageSize >  sizeof(iempMemPoolPageHeader_t));

    iempMemPoolPageHeader_t *firstPage = (iempMemPoolPageHeader_t *)iemem_malloc( pThreadData
                                                                                , memTypeAndProbe
                                                                                , initialPageSize);

    if (firstPage != NULL)
    {
        iempMemPoolOverallHeader_t *poolHdr = (iempMemPoolOverallHeader_t *)(firstPage + 1);

        ismEngine_SetStructId(firstPage->StrucId, iempSTRUCID);
        firstPage->nextPage      = NULL;
        firstPage->pageSize      = initialPageSize;
        firstPage->nextMemOffset =     sizeof(iempMemPoolPageHeader_t)
                                     + sizeof(iempMemPoolOverallHeader_t)
                                     + roundedReservedMem;

        poolHdr->lastPage             = (struct iempMemPoolPageHeader_t * volatile)firstPage;
        poolHdr->reservedMemRemaining = roundedReservedMem;
        poolHdr->reservedMemInitial   = roundedReservedMem;
        poolHdr->subsequentPageSize   = subsequentPageSize;
        poolHdr->memType              = IEMEM_GET_MEMORY_TYPE(memTypeAndProbe);

        rc = pthread_spin_init(&(poolHdr->listlock), PTHREAD_PROCESS_PRIVATE);

        if (rc != OK)
        {
            ieutTRACE_FFDC( ieutPROBE_001, true
                          , "failed init list lock.", rc
                          , "page", firstPage, initialPageSize
                          , NULL );
        }

        ieutTRACEL(pThreadData, firstPage, ENGINE_FNC_TRACE, FUNCTION_IDENT "Allocated: %lu (%lu reserved)->%p\n",
                                                             __func__, initialPageSize, roundedReservedMem,firstPage );

        //Let the caller know where the new pool is
        *pMemPoolHdl = firstPage;
    }
    else
    {
        ieutTRACEL(pThreadData, initialPageSize, ENGINE_ERROR_TRACE, FUNCTION_IDENT "Allocation failed: %lu for %lu reserved\n",
                                                             __func__, initialPageSize, roundedReservedMem );
        rc = ISMRC_AllocateError;
    }
mod_exit:
    return rc;
}

void iemp_destroyMemPool( ieutThreadData_t *pThreadData
                        , iempMemPoolHandle_t *pMemPoolHdl)
{
    if ((*pMemPoolHdl) != NULL)
    {
        ismEngine_CheckStructId((*pMemPoolHdl)->StrucId, iempSTRUCID, ieutPROBE_001);
        iempMemPoolPageHeader_t *page = *pMemPoolHdl;
        iempMemPoolOverallHeader_t *poolHdr = (iempMemPoolOverallHeader_t *)(*pMemPoolHdl + 1);
        iemem_memoryType memType = poolHdr->memType;

        pthread_spin_destroy(&(poolHdr->listlock));

        while( page != NULL)
        {
            iempMemPoolPageHeader_t *temp = page;

            page = (iempMemPoolPageHeader_t *)page->nextPage;

            iemem_freeStruct(pThreadData, memType, temp, temp->StrucId);
        }

        *pMemPoolHdl = NULL;
    }
}

int32_t iemp_useReservedMem( ieutThreadData_t *pThreadData
                           , iempMemPoolHandle_t memPoolHdl
                           , size_t *memAmount
                           , void **mem)
{
    ismEngine_CheckStructId(memPoolHdl->StrucId, iempSTRUCID, ieutPROBE_001);
    iempMemPoolOverallHeader_t *poolHdr = (iempMemPoolOverallHeader_t *)(memPoolHdl + 1);
    int32_t rc = OK;
    size_t roundedMemAmount;
    bool memReserved = false;
    void *grabbedMem = NULL;

    //Round mem chunk size to a multiple of 8
    if ((*memAmount & 0x07) != 0)
    {
        roundedMemAmount = RoundUp8(*memAmount);
        assert( (roundedMemAmount & 0x7) == 0);
    }
    else
    {
        roundedMemAmount = *memAmount;
    }

    do
    {
        size_t reservedRemaining = poolHdr->reservedMemRemaining;
        grabbedMem = NULL;

        if (roundedMemAmount <= poolHdr->reservedMemRemaining)
        {
            //Remove memory from the end of the reserved amount
            grabbedMem = ((void *)(poolHdr+1))+reservedRemaining - roundedMemAmount;

            memReserved = __sync_bool_compare_and_swap( &(poolHdr->reservedMemRemaining)
                                                      , reservedRemaining
                                                      , reservedRemaining - roundedMemAmount);
        }
        else
        {
            //Want more reserved memory than is reserved - that is VERY bad, we should
            //be reserving enough
            ieutTRACEL(pThreadData, roundedMemAmount, ENGINE_SEVERE_ERROR_TRACE, FUNCTION_IDENT "Want to reserve %lu only %lu reserved available\n",
                                                                         __func__, roundedMemAmount, reservedRemaining);
            rc = ISMRC_AllocateError;
            assert(0);
            break;
        }
    }
    while (!memReserved);

    if (memReserved)
    {
        ieutTRACEL(pThreadData, roundedMemAmount, ENGINE_FNC_TRACE, FUNCTION_IDENT "Used %lu of reservedMem\n",
                                                                     __func__, roundedMemAmount );
        *mem=grabbedMem;
        *memAmount=roundedMemAmount;
    }

    return rc;
}

void iemp_tryReleaseReservedMem( ieutThreadData_t *pThreadData
                               , iempMemPoolHandle_t memPoolHdl
                               , void *mem
                               , size_t memAmount )
{
    iempMemPoolOverallHeader_t *poolHdr = (iempMemPoolOverallHeader_t *)(memPoolHdl + 1);
    size_t reservedRemaining = poolHdr->reservedMemRemaining;
    bool memReleased;

    // If we're the last thing on the reserved memory we can try release it
    if (mem == (void *)(poolHdr+1)+reservedRemaining)
    {
        // try and give back the amount we've used
        memReleased = __sync_bool_compare_and_swap( &(poolHdr->reservedMemRemaining)
                                                  , reservedRemaining
                                                  , reservedRemaining + memAmount);
    }
    else
    {
        memReleased = false;
    }

    ieutTRACEL(pThreadData, memReleased, ENGINE_HIGH_TRACE, FUNCTION_IDENT "mem=%p memAmount=%lu released=%d\n",
               __func__, mem, memAmount, (int)memReleased);
}

int32_t iemp_allocate( ieutThreadData_t *pThreadData
                     , iempMemPoolHandle_t memPoolHdl
                     , size_t memAmount
                     , void **mem)
{
    ismEngine_CheckStructId(memPoolHdl->StrucId, iempSTRUCID, ieutPROBE_001);
    iempMemPoolOverallHeader_t *poolHdr = (iempMemPoolOverallHeader_t *)(memPoolHdl + 1);
    iempMemPoolPageHeader_t *page = (iempMemPoolPageHeader_t *)poolHdr->lastPage;

    bool memReserved = false;
    size_t currPageOffset = 0;
    int32_t rc = OK;

    do
    {
        //See if the memory we want will fit on the current page
        currPageOffset = page->nextMemOffset;
        size_t newPageOffset  = currPageOffset + memAmount;

        //Round mem chunk size to a multiple of 8
        if ((newPageOffset & 0x07) != 0)
        {
            newPageOffset += (8 - (newPageOffset & 0x07));
            assert( (newPageOffset & 0x7) == 0);
        }

        //Does it fit on the page?
        if (newPageOffset <= page->pageSize)
        {
            //Try and grab the memory we want
            memReserved = __sync_bool_compare_and_swap( &(page->nextMemOffset)
                                                      , currPageOffset
                                                      , newPageOffset);
        }
        else
        {
            rc = iemp_moveToNewPage( pThreadData
                                   , poolHdr
                                   , &page);
        }
    }
    while (rc == OK && !memReserved);

    if (memReserved)
    {
        *mem=((void *)page)+currPageOffset;

        ieutTRACEL(pThreadData, currPageOffset, ENGINE_FNC_TRACE, FUNCTION_IDENT "Assigned %lu bytes to %p\n",
                                                                     __func__, memAmount, *mem );
    }

    return rc;
}

void iemp_clearMemPool( ieutThreadData_t *pThreadData
                      , iempMemPoolHandle_t memPoolHdl
                      , bool keepReserved )
{
    ismEngine_CheckStructId(memPoolHdl->StrucId, iempSTRUCID, ieutPROBE_001);
    //free all the pages except the first one
    iempMemPoolPageHeader_t *page = (iempMemPoolPageHeader_t *)memPoolHdl->nextPage;
    iempMemPoolOverallHeader_t *poolHdr = (iempMemPoolOverallHeader_t *)(memPoolHdl + 1);

    while (page != NULL)
    {
        iempMemPoolPageHeader_t *temp = page;

        page = (iempMemPoolPageHeader_t *)page->nextPage;

        iemem_freeStruct(pThreadData, poolHdr->memType, temp, temp->StrucId);
    }

    poolHdr->lastPage = (struct iempMemPoolPageHeader_t * volatile)memPoolHdl; //Last page is the only page

    if (!keepReserved) poolHdr->reservedMemRemaining = poolHdr->reservedMemInitial;

    memPoolHdl->nextPage = NULL;
    memPoolHdl->nextMemOffset =  sizeof(iempMemPoolPageHeader_t)
                               + sizeof(iempMemPoolOverallHeader_t)
                               + poolHdr->reservedMemInitial;
}

static void iemp_listlock_lock(pthread_spinlock_t *lock)
{
    int osrc = pthread_spin_lock(lock);

    if (osrc != OK)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                      , "failed to take list lock.", osrc
                      , NULL );
    }
}

static void iemp_listlock_unlock(pthread_spinlock_t *lock)
{
    int osrc = pthread_spin_unlock(lock);

    if (osrc != OK)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                      , "failed to release list lock.", osrc
                      , NULL );
    }
}

static int32_t iemp_moveToNewPage( ieutThreadData_t *pThreadData
                                 , iempMemPoolOverallHeader_t *poolHdr
                                 , iempMemPoolPageHeader_t **pPage)
{
    bool movedPage = false;
    iempMemPoolPageHeader_t *page = *pPage;
    int32_t rc = OK;

    //Is there another page we can try?
    while (page->nextPage != NULL)
    {
        page = (iempMemPoolPageHeader_t *)(page->nextPage);
        movedPage = true;
    }

    if (!movedPage)
    {
        iemp_listlock_lock(&(poolHdr->listlock));

        //Did someone add another page since we checked, if so.... don't do it again
        while (page->nextPage != NULL)
        {
            page = (iempMemPoolPageHeader_t *)page->nextPage;
            movedPage = true;
        }

        if (!movedPage)
        {
            iempMemPoolPageHeader_t *newPage = iemem_malloc( pThreadData
                                                           , poolHdr->memType
                                                           , poolHdr->subsequentPageSize);

            if (newPage != NULL)
            {
                ismEngine_SetStructId(newPage->StrucId, iempSTRUCID);
                newPage->nextPage = NULL;
                newPage->pageSize = poolHdr->subsequentPageSize;
                newPage->nextMemOffset = sizeof(iempMemPoolPageHeader_t);


                page->nextPage = (struct iempMemPoolPageHeader_t * volatile)newPage;
                page = newPage;
                poolHdr->lastPage = (struct iempMemPoolPageHeader_t * volatile)newPage;
            }
            else
            {
                rc = ISMRC_AllocateError;
            }
        }

        iemp_listlock_unlock(&(poolHdr->listlock));
    }

    if (rc == OK)
    {
        ieutTRACEL(pThreadData, page, ENGINE_FNC_TRACE, FUNCTION_IDENT "Moved to page: %p\n",
                                                                               __func__, page);
        *pPage = page;
    }
    else
    {
        ieutTRACEL(pThreadData, page, ENGINE_WORRYING_TRACE, FUNCTION_IDENT "failed: %d\n",
                                                                               __func__, rc);
    }

    return rc;
}
