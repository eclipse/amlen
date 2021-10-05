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

#ifndef  H_rmmWinPthreads_H
#define  H_rmmWinPthreads_H

#define rmm_thread_create     pthread_create
#define rmm_thread_detach     pthread_detach
#define rmm_thread_cancel     pthread_cancel
#define rmm_thread_join       pthread_join

#define THREAD_RETURN_TYPE    void *  
#ifndef PTHREAD_CANCELED
#define PTHREAD_CANCELED NULL
#endif
#define THREAD_RETURN_VALUE   PTHREAD_CANCELED

#define rmm_mutex_t           pthread_mutex_t      
#define rmm_mutex_init        pthread_mutex_init   
#define rmm_mutex_lock        pthread_mutex_lock   
#define rmm_mutex_unlock      pthread_mutex_unlock 
#define rmm_mutex_trylock     pthread_mutex_trylock
#define rmm_thread_exit()  pthread_exit(THREAD_RETURN_VALUE)


#endif




