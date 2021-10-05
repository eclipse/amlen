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

#ifndef SINGLELOCK_NODUMMY_H_
#define SINGLELOCK_NODUMMY_H_

#include <stdint.h>
#include <pthread.h>

#include "engineCommon.h"

typedef struct tag_listitem_t {
    struct tag_listitem_t *prev;
    struct tag_listitem_t *next;
    uint64_t number;
} listitem_t;

typedef struct tag_list_t {
    pthread_spinlock_t lock;
    listitem_t *head;
    listitem_t *tail;
} list_t;


void init_list(list_t *list);

void addItem( list_t *list
            , listitem_t *item
            , uint64_t number);

void removeItem( list_t *list
               , listitem_t *item);

void deinit_list(list_t *list);


#endif /* SINGLELOCK_NODUMMY_H_ */
