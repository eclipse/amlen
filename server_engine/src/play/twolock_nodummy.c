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
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>

#include "twolock_nodummy.h"

void init_list(list_t *list)
{
    list->head = NULL;
    list->tail = NULL;

    int os_rc = pthread_spin_init(&(list->getlock), PTHREAD_PROCESS_PRIVATE);
    assert(os_rc == 0);

    os_rc = pthread_spin_init(&(list->putlock), PTHREAD_PROCESS_PRIVATE);
    assert(os_rc == 0);
}

void addItem( list_t *list
            , listitem_t *item
            , uint64_t number)
{
    bool done_put = false;
    int os_rc;

    item->number = number;
    item->next = NULL;

    //dirty read tail
    if (list->tail != NULL) {
        os_rc = pthread_spin_lock(&(list->putlock));
        assert(os_rc == 0);

        //Now can validly read tail
        if (list->tail != NULL) {
            //We have putlock and a non-empty list... we can validly add an item
            item->prev = list->tail;
            list->tail->next = item;
            list->tail = item;
            done_put = true;
        }
        os_rc = pthread_spin_unlock(&(list->putlock));
        assert(os_rc == 0);
    }

    if (!done_put) {
        //Hmmm we may be adding to an empty list and thus may need to set head
        //We need the getlock and the putlock (in that order)
        os_rc = pthread_spin_lock(&(list->getlock));
        assert(os_rc == 0);
        os_rc = pthread_spin_lock(&(list->putlock));
        assert(os_rc == 0);

        if (list->tail != NULL) {
            //We have putlock and a non-empty list... we can validly add an item
            item->prev = list->tail;
            list->tail->next = item;

            assert(list->head != NULL);
        } else {
            item->prev = NULL;
            assert(list->head == NULL);
            list->head= item;
        }
        list->tail = item;

        os_rc = pthread_spin_unlock(&(list->putlock));
        assert(os_rc == 0);
        os_rc = pthread_spin_unlock(&(list->getlock));
        assert(os_rc == 0);
    }
}

void removeItem( list_t *list
               , listitem_t *item)
{
    int os_rc = pthread_spin_lock(&(list->getlock));
    assert(os_rc == 0);

    if (item->next == NULL) {
        //Hmmm appear to be getting last item...need both locks
        os_rc = pthread_spin_lock(&(list->putlock));
        assert(os_rc == 0);
        assert(list->tail !=  NULL);

        if (item->prev != NULL) {
            item->prev->next = item->next;
        } else {
            assert(list->head == item);
            list->head = item->next;
        }
        if (item->next != NULL) {
            item->next->prev = item->prev;
        } else {
            assert(list->tail == item);
            list->tail = item->prev;
        }
        item->prev = NULL;
        item->next = NULL;

        os_rc = pthread_spin_unlock(&(list->putlock));
        assert(os_rc == 0);
    } else {
        if (item->prev != NULL) {
            item->prev->next = item->next;
        } else {
            assert(list->head == item);
            list->head = item->next;
        }
        //We know next is non-null else we'd be in other leg of if
        item->next->prev = item->prev;

        item->prev = NULL;
        item->next = NULL;
    }

    os_rc = pthread_spin_unlock(&(list->getlock));
    assert(os_rc == 0);
}

void deinit_list(list_t *list)
{
    int os_rc = pthread_spin_destroy(&(list->getlock));
    assert(os_rc == 0);

    os_rc = pthread_spin_destroy(&(list->putlock));
    assert(os_rc == 0);
}
