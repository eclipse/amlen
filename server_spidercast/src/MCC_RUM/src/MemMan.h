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

#ifndef  H_MemMan_H
#define  H_MemMan_H

/* Memory alert */
#define  MEMORY_ALERT_OFF            0
#define  MEMORY_ALERT_LOW            1
#define  MEMORY_ALERT_MID            2
#define  MEMORY_ALERT_HIGH           3
#ifndef BASE_WAIT_NANO
#define BASE_WAIT_NANO        16000000
#endif
#ifndef INT_MAX
#define INT_MAX 0x7fffffff
#endif

#include "BuffBoxMgm.h"

typedef struct
{
   int32_t iBufSize ,
           iBoxSize ,
           iCurIdle ,
           iMaxSize ,
           iCurSize ,
           iWaiting ,
           iLowMark ,
           iHiwMark ,
           iOffZero ,
           iLenZero ,
           iLastErr ;
   MUTEX_TYPE      mutex  ;
   COND_TYPE       cond ; 
   void          **pbb ; 
}  MemManRec ;

static RMM_INLINE void *      MM_get_buff(MemManRec *pmm, int MaxTry, int *iError) ;
static RMM_INLINE void *      MM_put_buff(MemManRec *pmm, void *buff) ;
static int32_t     MM_get_buffs_in_use(MemManRec *pmm) ;
static int32_t     MM_get_nBuffs(MemManRec *pmm) ;
static int32_t     MM_get_cur_size(MemManRec *pmm) ;
static int32_t     MM_get_max_size(MemManRec *pmm) ;
static int32_t     MM_get_buf_size(MemManRec *pmm) ;
static void        MM_dec_size(MemManRec *pmm,int new_size) ;
static void        MM_put_buffs(MemManRec *pmm, int nbuffs, void **buffs) ;
static MemManRec * MM_alloc(int32_t buf_size, int32_t box_size, int32_t max_size, int32_t hi_mark, int32_t low_mark) ;
static MemManRec * MM_alloc2(int32_t buf_size, int32_t box_size, int32_t max_size, int32_t hi_mark, int32_t low_mark, int32_t off0, int32_t len0) ;
static void        MM_free(MemManRec *pmm, int free_buffs) ;

#endif
