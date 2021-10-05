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


#ifndef  H_BuffBoxMgm_H
#define  H_BuffBoxMgm_H


typedef struct 
{
   int     iSize , 
           iPut , 
           iGet , 
           iRoll,
           iBase;
   MUTEX_TYPE mutex  ;
   COND_TYPE  condE, condF ; 
   int        waitE, waitF ; 
   void **Buffs ;  
}  BuffBoxRec ; 

#define BB_get_buff(pbb,iLock) BB_get_buff_##iLock(pbb)
static RMM_INLINE void * BB_get_buff_0(BuffBoxRec *pbb) ; 
static RMM_INLINE void * BB_get_buff_1(BuffBoxRec *pbb) ; 

#define BB_put_buff(pbb,buff,iLock) BB_put_buff_##iLock(pbb,buff) 
static RMM_INLINE void * BB_put_buff_0(BuffBoxRec *pbb , void *buff) ; 
static RMM_INLINE void * BB_put_buff_1(BuffBoxRec *pbb , void *buff) ; 

#define BB_get_nBuffs(pbb,iLock) BB_get_nBuffs_##iLock(pbb) 
static RMM_INLINE int    BB_get_nBuffs_0(BuffBoxRec *pbb) ; 
static RMM_INLINE int    BB_get_nBuffs_1(BuffBoxRec *pbb) ; 

static void   BB_waitF(BuffBoxRec *pbb) ; 
static void * BB_see_buff_r(BuffBoxRec *pbb, int rPos, int iLock) ; 
static void * BB_see_buff_a(BuffBoxRec *pbb, int aPos, int iLock) ; 
static int    BB_lock(BuffBoxRec *pbb) ; 
static int    BB_unlock(BuffBoxRec *pbb) ; 
static void   BB_waitE(BuffBoxRec *pbb) ; 
static void   BB_signalE(BuffBoxRec *pbb) ; 
static void   BB_signalF(BuffBoxRec *pbb) ; 
static BuffBoxRec * BB_alloc(int size, int base) ; 
static void BB_free(BuffBoxRec *pbb, int free_buffs) ; 

/************  LinkedList implementation   ************/



typedef struct 
{
   int     iSize , 
           iOff , 
           iGet , 
           iBase;
   MUTEX_TYPE mutex  ;
   COND_TYPE  condE, condF ; 
   int        waitE, waitF ; 
   void   *head ;  
   void   *tail  ;  
}  LinkedListRec ; 

typedef void *(*LL_get_buff_t)(LinkedListRec *pbb) ;
#define LL_get_buff(pbb,iLock) LL_get_buff_##iLock(pbb)
static RMM_INLINE void * LL_get_buff_0(LinkedListRec *pbb) ; 
static RMM_INLINE void * LL_get_buff_1(LinkedListRec *pbb) ; 

typedef void *(*LL_put_buff_t)(LinkedListRec *pbb, void *buff) ;
#define LL_put_buff(pbb,buff,iLock) LL_put_buff_##iLock(pbb,buff)  
static RMM_INLINE void * LL_put_buff_0(LinkedListRec *pbb, void *buff) ; 
static RMM_INLINE void * LL_put_buff_1(LinkedListRec *pbb, void *buff) ; 

#define LL_get_nBuffs(pbb,iLock) LL_get_nBuffs_##iLock(pbb) 
static RMM_INLINE int    LL_get_nBuffs_0(LinkedListRec *pbb) ; 
static RMM_INLINE int    LL_get_nBuffs_1(LinkedListRec *pbb) ; 

static void * LL_see_buff_r(LinkedListRec *pbb, int rPos, int iLock) ; 
static int    LL_waitingF(LinkedListRec *pbb) ; 
static int    LL_trylock(LinkedListRec *pbb) ; 
static int    LL_lock(LinkedListRec *pbb) ; 
static int    LL_unlock(LinkedListRec *pbb) ; 
static void   LL_waitE(LinkedListRec *pbb) ; 
static void   LL_waitF(LinkedListRec *pbb) ; 
static void   LL_twaitF(LinkedListRec *pbb, int milli) ; 
static void   LL_signalE(LinkedListRec *pbb) ; 
static void   LL_signalF(LinkedListRec *pbb) ; 
static LinkedListRec * LL_alloc(int off, int base) ; 
static void LL_free(LinkedListRec *pbb, int free_buffs) ; 
#define LL_isEmpty(x) ((x)->iSize==0)

#endif
