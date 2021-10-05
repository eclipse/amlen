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
#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct tag_threaddata_t {
    listitem_t *itemArray;
    list_t   *list;
    uint32_t batchSize;
    uint32_t threadNum;
    uint32_t totalThreads;
    uint32_t numbatches;
} threaddata_t;

void *adderRemover(void *data) {
    threaddata_t *threaddata = (threaddata_t *)data;

    for (uint32_t batchNum=0; batchNum < threaddata->numbatches; batchNum++) {
        listitem_t *firstInBatch = &(threaddata->itemArray[
                                                (threaddata->batchSize)
                                               *(  (threaddata->totalThreads * batchNum)
                                                  +(threaddata->threadNum))]);
        //printf("For thread %u, batch %u index of first item is %u\n", threaddata->threadNum, batchNum, (threaddata->batchSize) *(  (threaddata->totalThreads * batchNum)+(threaddata->threadNum)));

        //Add the items to the list....
        listitem_t *currItem = firstInBatch;
        uint32_t itemNum = (threaddata->batchSize * batchNum);

        for (uint32_t i=0; i<threaddata->batchSize; i++) {
            //printf("For thread %u, batch %u number for item %u is %lu\n", threaddata->threadNum, batchNum, i, ((((uint64_t)threaddata->threadNum) << 32)|itemNum));
            addItem( threaddata->list
                   , currItem
                   , ((((uint64_t)threaddata->threadNum) << 32)|itemNum));
            currItem++;
            itemNum++;
        }

        //...and remove them again
        currItem = firstInBatch;
        itemNum = (threaddata->batchSize * batchNum);

        for (uint32_t i=0; i<threaddata->batchSize; i++) {
            //printf("For thread %u, batch %u removing item %u\n", threaddata->threadNum, batchNum, i);
            assert(currItem->number == ((((uint64_t)threaddata->threadNum) << 32)|itemNum));
            removeItem(threaddata->list, currItem);
            currItem++;
            itemNum++;
        }
    }

    return NULL;
}


int main(int argc, char **argv)
{
    uint32_t numThreads = 5;
    uint32_t batchesPerThread = 50;
    uint32_t batchSize = 2000;

    listitem_t *fakeQueueArray;
    list_t   acklist;
    threaddata_t threadData[numThreads];
    pthread_t threadId[numThreads];
    int os_rc;

    fakeQueueArray = malloc((numThreads * batchesPerThread * batchSize) * sizeof(listitem_t));
    //printf("Num items in array is %u\n", (numThreads * batchesPerThread * batchSize));
    assert(fakeQueueArray != NULL);

    init_list(&acklist);

    for (uint32_t i=0; i<numThreads; i++) {
        threadData[i].itemArray = fakeQueueArray;
        threadData[i].list = &acklist;
        threadData[i].batchSize = batchSize;
        threadData[i].threadNum = i;
        threadData[i].totalThreads = numThreads;
        threadData[i].numbatches =  batchesPerThread;

        os_rc = test_task_startThread(&threadId[i],adderRemover, &(threadData[i]),"adderRemover");
        assert(os_rc == 0);
    }

    for (uint32_t i=0; i<numThreads; i++) {
        os_rc = pthread_join(threadId[i], NULL);
        assert(os_rc == 0);
    }
    deinit_list(&acklist);

#ifndef NDEBUG
    //All items should have been added and removed and therefore have appropriate number
    uint32_t itemNum = 0;
    for (uint32_t i = 0; i < batchesPerThread; i++) {
        for (uint64_t j = 0; j < numThreads; j++) {
            for (uint32_t k = 0; k < batchSize; k++) {
                if(fakeQueueArray[itemNum].number != ((j << 32)|(i*batchSize + k)) ) {
                    printf("itemnum is %u, i = %u, j = %u, k = %u,  Expected %lu, Got %lu\n",
                            itemNum, i, j, k,
                            ((j << 32)|(i*batchSize + k)), fakeQueueArray[itemNum].number);
                }
                itemNum++;
            }
        }
    }
#endif

    free(fakeQueueArray);
}
