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

#ifndef TWOLOCK_NODUMMY_H_
#define TWOLOCK_NODUMMY_H_

#include <stdint.h>
#include <pthread.h>

#include "engineCommon.h"

typedef struct tag_listitem_t {
    struct tag_listitem_t *prev;
    struct tag_listitem_t *next;
    uint64_t number;
} listitem_t;

typedef struct tag_list_t {
    pthread_spinlock_t getlock CACHELINE_ALIGNED;
    listitem_t *head;
    pthread_spinlock_t putlock CACHELINE_ALIGNED;
    listitem_t *tail;
} list_t;


void init_list(list_t *list);

void addItem( list_t *list
            , listitem_t *item
            , uint64_t number);

void removeItem( list_t *list
               , listitem_t *item);

void deinit_list(list_t *list);


#endif /* TWOLOCK_NODUMMY_H_ */
