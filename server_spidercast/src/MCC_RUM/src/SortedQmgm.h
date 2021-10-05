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

#ifndef  H_SortedQmgm_H
#define  H_SortedQmgm_H

typedef struct 
{
   int iSize , 
       iBase , 
       iGet ;
   MUTEX_TYPE mutex  ;
   char  *Flags ; 
   void **Buffs ;  
}  SortedQRec ; 

static SortedQRec * SQ_alloc(int size,int base) ; 
static void   SQ_free(SortedQRec *pbb, int free_buffs) ; 

#define SQ_get_buff(pbb,iLock,aPos,iError) SQ_get_buff_##iLock(pbb,aPos,iError) 
static RMM_INLINE void * SQ_get_buff_0(SortedQRec *pbb,int aPos,int *iError) ; 
static RMM_INLINE void * SQ_get_buff_1(SortedQRec *pbb,int aPos,int *iError) ; 

#define SQ_put_buff(pbb,iLock,aPos,iError,buff) SQ_put_buff_##iLock(pbb,aPos,iError,buff) 
static RMM_INLINE int    SQ_put_buff_0(SortedQRec *pbb,int aPos,int *iError,void *buff) ; 
static RMM_INLINE int    SQ_put_buff_1(SortedQRec *pbb,int aPos,int *iError,void *buff) ; 

#define SQ_inc_tail(pbb,iLock) SQ_inc_tail_##iLock(pbb) 
static RMM_INLINE void * SQ_inc_tail_0(SortedQRec *pbb) ; 
static RMM_INLINE void * SQ_inc_tail_1(SortedQRec *pbb) ; 

#define SQ_get_tailPP(pbb,sn,iLock) SQ_get_tailPP_##iLock(pbb,sn) 
static RMM_INLINE void * SQ_get_tailPP_0(SortedQRec *pbb,uint32_t *sn) ; 
static RMM_INLINE void * SQ_get_tailPP_1(SortedQRec *pbb,uint32_t *sn) ; 

static char   SQ_AND_flag(SortedQRec *pbb,int iLock,int aPos,int *iError,char flag) ; 
static char   SQ_OR_flag(SortedQRec *pbb,int iLock,int aPos,int *iError,char flag) ; 

#define SQ_get_tailSN(pbb,iLock) SQ_get_tailSN_##iLock(pbb) 
static RMM_INLINE int    SQ_get_tailSN_0(SortedQRec *pbb) ; 
static RMM_INLINE int    SQ_get_tailSN_1(SortedQRec *pbb) ; 

static int    SQ_get_headSN(SortedQRec *pbb,int iLock) ; 
static int    SQ_lock(SortedQRec *pbb) ; 
static int    SQ_unlock(SortedQRec *pbb) ; 
static int    SQ_get_nBuffs(SortedQRec *pbb) ; 
static void   SQ_set_base(SortedQRec *pbb,int iLock,int base) ; 

#endif
