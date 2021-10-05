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
#include <ismutil.h>
#include <pthread.h>
#include <assert.h>
#include <ismjson.h>

typedef struct bufferPoolNode bufferPoolNode;

struct bufferPoolNode
{
    ism_byteBufferPool pool;
    char name[64];
    bufferPoolNode * next;
};

typedef struct
{
   bufferPoolNode * head;
} bufferPoolHead ;

static int poolLockType = 1;
static bufferPoolHead bufferPoolList;

void ism_bufferPoolInit(void) {
    if(ism_common_getBooleanConfig("UseSpinLocks", 0))
        poolLockType = 1;
    else
        poolLockType = 0;
    bufferPoolList.head = NULL;
}

XAPI void ism_utils_addBufferPoolsDiagnostics(ism_json_t * jobj, const char * name) {

    ism_json_startObject(jobj, name);

    bufferPoolNode *node = bufferPoolList.head;
    while( node != NULL ) {
        ism_json_startObject(jobj, node->name);
        ism_json_putULongItem(jobj, "Free", node->pool->free);
        ism_json_putULongItem(jobj, "Allocated", node->pool->allocated);
        ism_json_endObject(jobj);
        node = node->next;
    }

    ism_json_endObject(jobj);

}

XAPI void ism_utils_traceBufferPoolsDiagnostics(int32_t traceLevel) {

    bufferPoolNode *node = bufferPoolList.head;
    while( node != NULL ) {
        TRACE(traceLevel,"Buffer Pool %s Free: %d Allocated: %d\n", node->name, node->pool->free, node->pool->allocated);
        node = node->next;
    }

}


/**
 * Allocate the Byte Buffer
 * Note: Pool will NULL.
 * @param bufSize size of the buffer.
 */
 ism_byteBuffer ism_allocateByteBuffer(int bufSize){
	ism_byteBuffer bb = ism_common_malloc(ISM_MEM_PROBE(ism_memory_bufferPools,1),sizeof(ism_byte_buffer_t)+bufSize);
    if (__builtin_expect((bb == NULL), 0)) {
        ism_common_shutdown(1);
        return NULL; /* Unreachable */
    }
	bb->buf = (char*)(bb+1);
	bb->getPtr = bb->putPtr = bb->buf;
	bb->pool = NULL;
	bb->allocated = bufSize;
	bb->used = 0;
	bb->next = NULL;
	return bb;
}

/**
 * Free ByteBuffer Object
 * @param bb byte buffer object
 */
void ism_freeByteBuffer(ism_byteBuffer bb){
	ism_common_free(ism_memory_bufferPools,bb);
}

/**
 * Lock the pool
 * Note: locking either Mutex or Spin lock which
 * depends on the pool lock type
 * @param pool the buffer pool
 */
static inline void poolLock(ism_byteBufferPool pool) {
    if(poolLockType)
        pthread_spin_lock(&pool->lock);
    else
        pthread_mutex_lock(&pool->mutex);
}

/**
 * UnLock the pool
 * Note: locking either Mutex or Spin lock which
 * depends on the pool lock type
 * @param pool the buffer pool
 */
static inline void poolUnlock(ism_byteBufferPool pool) {
    if(poolLockType)
        pthread_spin_unlock(&pool->lock);
    else
        pthread_mutex_unlock(&pool->mutex);
}

/**
 * Get the buffer pool info
 */
void ism_common_getBufferPoolInfo(ism_byteBufferPool pool, int *minSize, int *maxSize, int *allocated, int *free) {
	if (pool) {
	    poolLock(pool);
		if (minSize)
		    *minSize = pool->minPoolSize;
		if (maxSize)
		    *maxSize = pool->maxPoolSize;
		if (allocated)
		    *allocated = pool->allocated;
		if (free)
		    *free = pool->free;
		poolUnlock(pool);
	}
}

/**
 * Create the buffer pool
 * @param bufSize buffer block size
 * @param minPoolSize minimum pool size
 * @param maxPoolSize maximum pool size
 * @return the pool object
 */
ism_byteBufferPool ism_common_createBufferPool(int bufSize,int minPoolSize, int maxPoolSize,const char *name){
	ism_byteBufferPool pool = ism_common_calloc(ISM_MEM_PROBE(ism_memory_bufferPools,2),1,sizeof(ism_byteBufferPool_t));
	int i;
	pthread_spin_init(&pool->lock,0);
	pthread_mutex_init(&pool->mutex, NULL);
	pool->bufSize = bufSize;
	pool->minPoolSize = minPoolSize;
	pool->maxPoolSize = maxPoolSize;
	if (pool->maxPoolSize < pool->minPoolSize)
	    pool->maxPoolSize = pool->minPoolSize;
	for (i = 0; i < minPoolSize; i++) {
		ism_byteBuffer bb = ism_allocateByteBuffer(bufSize);
		bb->next = pool->head;
		bb->pool = pool;
		pool->head = bb;
	}
	pool->free = pool->allocated = minPoolSize;
	bufferPoolNode * node = ism_common_malloc(ISM_MEM_PROBE(ism_memory_bufferPools,2),sizeof(bufferPoolNode));
	node->next = NULL;
	ism_common_strlcpy(node->name, name, 64);
	node->pool = pool;
	if (bufferPoolList.head == NULL) {
	    bufferPoolList.head = node;
	} else {
	    bufferPoolNode * index = bufferPoolList.head;
	    while( index->next != NULL ) {
	        index = index->next;
	    }
	    index->next = node;
	}
	return pool;
}

/*
 * Free up the buffer pool
 */
void ism_common_destroyBufferPool(ism_byteBufferPool pool){
    poolLock(pool);
    /* Only free the pool if we are done using all of them.  We are shutting down anyway */
    if (pool->allocated == pool->free) {
        while (pool->head){
            ism_byteBuffer bb = pool->head;
            pool->head = bb->next;
            ism_freeByteBuffer(bb);
        }
    }
	poolUnlock(pool);
	pthread_spin_destroy(&pool->lock);
    pthread_mutex_destroy(&pool->mutex);

	ism_common_free(ism_memory_bufferPools,pool);
}

/*
 * Get a buffer object
 * Note: the returned buffer will mark as inuse
 * @param pool the buffer pool
 * @param force force to allocate the buffer from memory if the pool reached the max
 * @return the buffer object
 */
ism_byteBuffer ism_common_getBuffer(ism_byteBufferPool pool, int force){
	ism_byteBuffer bb = NULL;
	poolLock(pool);
	if (LIKELY(pool->free > 0)) {
		bb = pool->head;
		pool->head = pool->head->next;
		pool->free--;
		bb->next = NULL;
		bb->inuse = 1;
	    poolUnlock(pool);
		bb->getPtr = bb->putPtr = bb->buf;
		bb->used = 0;
		return bb;
	}
	if (LIKELY((pool->allocated < pool->maxPoolSize) || force)) {
		pool->allocated++;
        poolUnlock(pool);
		bb = ism_allocateByteBuffer(pool->bufSize);
		bb->pool = pool;
		bb->inuse = 1;
		return bb;
	}
    poolUnlock(pool);
	return NULL;
}

/*
 * Get the buffer list
 * @param pool the buffer pool
 * @param count the number of buffer objects that want to get
 * @param force force to allocate more buffer from memory if pool reached the max
 */
ism_byteBuffer ism_common_getBuffersList(ism_byteBufferPool pool, int count, int force){
    ism_byteBuffer result = NULL;
    int available = 0;
    poolLock(pool);
    while(pool->free){
        ism_byteBuffer bb = pool->head;
        pool->head = bb->next;
        bb->getPtr = bb->putPtr = bb->buf;
        bb->used = 0;
        bb->inuse = 1;
        bb->next = result;
        result = bb;
        pool->free--;
        count--;
        if (LIKELY(count == 0)) {
            poolUnlock(pool);
            return result;
        }
    }
    available = pool->maxPoolSize - pool->allocated;
    if ((available < count) && (force == 0)){
        count = (available > 0) ? available : 0;
    }
    pool->allocated += count;
    poolUnlock(pool);
    while(count > 0){
        ism_byteBuffer bb = ism_allocateByteBuffer(pool->bufSize);
        bb->pool = pool;
        bb->next = result;
        bb->inuse = 1;
        result = bb;
        count--;
    }
    return result;
}

/*
 * Return the buffer to the pool
 * Note: the buffer object will mark as not inuse
 * @param bb the buffer object
 * @param file the file name which invoke the function
 * @param where the line number where the function is invoked
 *
 */
void ism_common_returnBuffer(ism_byteBuffer bb, const char * file, int where){
	ism_byteBufferPool pool = bb->pool;
	if (pool != NULL) {
        poolLock(pool);
        if(bb->inuse==0){
        		poolUnlock(pool);
        		TRACE(5, "Invalid return of the buffer to the pool. The buffer is not in use. File=%s Line=%d\n", file?file:"", where);
        		return;
        }
		if (LIKELY(pool->free < pool->maxPoolSize)) {
			bb->inuse = 0;
			bb->next = pool->head;
			pool->head = bb;
			pool->free++;
	        poolUnlock(pool);
			return;
		}
		pool->allocated--;
        poolUnlock(pool);
	}
	ism_freeByteBuffer(bb);
}

/*
 * Return the buffer list to the pool
 */
void ism_common_returnBuffersList(ism_byteBuffer head, ism_byteBuffer tail, int count){
    if (count) {
        ism_byteBufferPool pool = head->pool;
        if (pool != NULL) {
            poolLock(pool);
            //Mark the inuse to 0 for the buffer in the list
            int icount= 0;
            ism_byteBuffer bb = head;
            while(icount < count && bb != NULL){
            		bb->inuse = 0;
            		bb = bb->next;
            		icount++;
            }
            if (pool->allocated <= pool->maxPoolSize) {
                tail->next = pool->head;
                pool->head = head;
                pool->free += count;
                poolUnlock(pool);
                return;
            }
            pool->allocated -= count;
            poolUnlock(pool);
        }
        while(head){
            ism_byteBuffer bb = head;
            head = bb->next;
            ism_freeByteBuffer(bb);
        }
    }
}
