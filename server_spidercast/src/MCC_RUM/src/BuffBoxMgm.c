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

#ifndef  H_BuffBoxMgm_C
#define  H_BuffBoxMgm_C

#include "BuffBoxMgm.h"

/*******************************************************/
/*******************************************************/
static RMM_INLINE void * BB_get_buff_0(BuffBoxRec *pbb)
{
 void *rc ; 

  if ( pbb->iPut + pbb->iRoll - pbb->iGet > 0 )
  {
    rc = pbb->Buffs[pbb->iGet] ;
    if ( ++pbb->iGet >= pbb->iSize )
    {
      pbb->iGet = 0 ; 
      pbb->iRoll= 0 ; 
      pbb->iBase+= pbb->iSize ; 
    }
  }
  else
    rc = NULL ; 
  return rc ; 

}
/*******************************************************/
static RMM_INLINE void * BB_get_buff_1(BuffBoxRec *pbb)
{
 void *rc ; 

  MUTEX_POLLLOCK(&pbb->mutex) ; 
  if ( pbb->iPut + pbb->iRoll - pbb->iGet > 0 )
  {
    rc = pbb->Buffs[pbb->iGet] ;
    if ( ++pbb->iGet >= pbb->iSize )
    {
      pbb->iGet = 0 ; 
      pbb->iRoll= 0 ; 
      pbb->iBase+= pbb->iSize ; 
    }
  }
  else
    rc = NULL ; 
  MUTEX_UNLOCK(&pbb->mutex) ; 
  return rc ; 

}

/*******************************************************/
/*******************************************************/
/*******************************************************/
/*******************************************************/
static RMM_INLINE void * BB_put_buff_0(BuffBoxRec *pbb , void *buff)
{
 void *rc ; 

  if ( pbb->iPut + pbb->iRoll - pbb->iGet < pbb->iSize )
  {
    rc = buff ; 
    pbb->Buffs[pbb->iPut] = buff ;
    if ( ++pbb->iPut >= pbb->iSize )
    {
      pbb->iPut = 0 ; 
      pbb->iRoll= pbb->iSize ; 
    }
  }
  else
    rc = NULL ; 

  return rc ; 
}
/*******************************************************/
static RMM_INLINE void * BB_put_buff_1(BuffBoxRec *pbb , void *buff)
{
 void *rc ; 

  MUTEX_POLLLOCK(&pbb->mutex) ; 
  if ( pbb->iPut + pbb->iRoll - pbb->iGet < pbb->iSize )
  {
    rc = buff ; 
    pbb->Buffs[pbb->iPut] = buff ;
    if ( ++pbb->iPut >= pbb->iSize )
    {
      pbb->iPut = 0 ; 
      pbb->iRoll= pbb->iSize ; 
    }
  }
  else
    rc = NULL ; 

  MUTEX_UNLOCK(&pbb->mutex) ; 
  return rc ; 
}

/*******************************************************/
/*******************************************************/

/*******************************************************/
/*******************************************************/
static RMM_INLINE int BB_get_nBuffs_0(BuffBoxRec *pbb)
{
  return (pbb->iPut + pbb->iRoll - pbb->iGet) ;
}
static RMM_INLINE int BB_get_nBuffs_1(BuffBoxRec *pbb)
{
 int nBuffs ; 
  MUTEX_POLLLOCK(&pbb->mutex) ; 
  nBuffs = pbb->iPut + pbb->iRoll - pbb->iGet ;
  MUTEX_UNLOCK(&pbb->mutex) ; 
  return nBuffs ; 
}

/*******************************************************/
/*******************************************************/
void * BB_see_buff_r(BuffBoxRec *pbb, int rPos, int iLock)
{
 void *rc=NULL ; 

  if ( iLock ) MUTEX_POLLLOCK(&pbb->mutex) ; 
  if ( rPos >= 0 && rPos < pbb->iPut + pbb->iRoll - pbb->iGet )
  {
    rc = pbb->Buffs[(pbb->iGet+rPos)%pbb->iSize] ;
  }
  if ( iLock ) MUTEX_UNLOCK(&pbb->mutex) ; 
  return rc ; 

}

/*******************************************************/
/*******************************************************/
void * BB_see_buff_a(BuffBoxRec *pbb, int aPos, int iLock)
{
 int rPos ; 
 void *rc=NULL ; 

  if ( iLock ) MUTEX_POLLLOCK(&pbb->mutex) ; 
  rPos = aPos - pbb->iBase ; 
  if ( rPos >= pbb->iGet && rPos < pbb->iPut + pbb->iRoll )
  {
    rc = pbb->Buffs[rPos%pbb->iSize] ;
  }
  if ( iLock ) MUTEX_UNLOCK(&pbb->mutex) ; 
  return rc ; 

}

/*******************************************************/
/*******************************************************/
/*******************************************************/
/*******************************************************/
BuffBoxRec * BB_alloc(int size, int base)
{
  BuffBoxRec *pbb ; 

  if ( size < 0 )
    return NULL ; 

  if ( (pbb = (BuffBoxRec *)malloc(sizeof(BuffBoxRec))) != NULL )
  {
    memset(pbb,0,sizeof(BuffBoxRec)) ; 
    if ( size > 0 )
    {
      if ( (pbb->Buffs = (void **)malloc(size*sizeof(void *))) == NULL )
      {
        free(pbb) ; 
        return NULL ; 
      }
    }
    pbb->iSize = size ; 
    pbb->iBase = base ; 
    MUTEX_INIT(&pbb->mutex) ; 
    COND_INIT( &pbb->condE) ; 
    COND_INIT( &pbb->condF) ; 
  }
  return pbb ; 

}

/*******************************************************/
/*******************************************************/
int    BB_lock(BuffBoxRec *pbb)
{
  MUTEX_POLLLOCK(&pbb->mutex) ; 
  return 0;
}

/*******************************************************/
/*******************************************************/
int    BB_unlock(BuffBoxRec *pbb)
{
  return MUTEX_UNLOCK(&pbb->mutex) ; 
}

/*******************************************************/
/*******************************************************/
void   BB_waitE(BuffBoxRec *pbb)
{
  pbb->waitE++ ; 
  COND_WAIT(&pbb->condE,&pbb->mutex) ; 
  pbb->waitE-- ; 
}
void   BB_waitF(BuffBoxRec *pbb)
{
  pbb->waitF++ ; 
  COND_WAIT(&pbb->condF,&pbb->mutex) ; 
  pbb->waitF-- ; 
}

/*******************************************************/
/*******************************************************/
void   BB_signalE(BuffBoxRec *pbb)
{
  if ( pbb->waitE ) COND_SIGNAL(&pbb->condE) ; 
}
void   BB_signalF(BuffBoxRec *pbb)
{
  if ( pbb->waitF ) COND_SIGNAL(&pbb->condF) ; 
}

/*******************************************************/
/*******************************************************/
void BB_free(BuffBoxRec *pbb, int free_buffs)
{
  void * buff ; 

  if ( pbb->iSize > 0 )
  {
    if ( free_buffs )
    {
      while ( (buff=BB_get_buff(pbb,1)) != NULL )
        free(buff) ; 
    }
    free(pbb->Buffs) ; 
  }
  MUTEX_DESTROY(&pbb->mutex) ; 
  COND_DESTROY( &pbb->condE) ; 
  COND_DESTROY( &pbb->condF) ; 
  free(pbb) ; 

}

/**** LinkedList implementation   **********/


typedef struct
{
  void *next ; 
} NextRec ; 

#define LL_NEXT(x) ((NextRec *)((uintptr_t)(x) + pll->iOff))->next
#define LL_LAST    ((void *)1)

/*******************************************************/
/*******************************************************/
static RMM_INLINE void * LL_get_buff_0(LinkedListRec *pll)
{
 void *rc ; 

  if ( (rc = pll->head) )
  {
    if ( pll->iSize == 1 )
      pll->head = pll->tail = NULL ; 
    else
      pll->head = LL_NEXT(rc) ; 
    LL_NEXT(rc) = NULL ; 
    pll->iSize-- ; 
    pll->iBase++ ; 
  }
  return rc ; 
}
/*******************************************************/
static RMM_INLINE void * LL_get_buff_1(LinkedListRec *pll)
{
 void *rc ; 

  MUTEX_POLLLOCK(&pll->mutex) ; 
  if ( (rc = pll->head) )
  {
    if ( pll->iSize == 1 )
      pll->head = pll->tail = NULL ; 
    else
      pll->head = LL_NEXT(rc) ; 
    LL_NEXT(rc) = NULL ; 
    pll->iSize-- ; 
    pll->iBase++ ; 
  }
  MUTEX_UNLOCK(&pll->mutex) ; 
  return rc ; 
}

/*******************************************************/
/*******************************************************/
static RMM_INLINE void * LL_put_buff_0(LinkedListRec *pll , void *buff)
{
  void *rc ; 

  if ( !LL_NEXT(buff) )
  {
    rc = buff ; 
    if ( pll->tail )
      LL_NEXT(pll->tail) = buff ; 
    else
      pll->head = buff ; 
    pll->tail = buff ; 
    LL_NEXT(buff) = LL_LAST ; 
    pll->iSize++ ;
  }
  else
    rc = NULL ; 
  return rc ; 
}
/*******************************************************/
static RMM_INLINE void * LL_put_buff_1(LinkedListRec *pll , void *buff)
{
  void *rc ; 

  MUTEX_POLLLOCK(&pll->mutex) ; 
  if ( !LL_NEXT(buff) )
  {
    rc = buff ; 
    if ( pll->tail )
      LL_NEXT(pll->tail) = buff ; 
    else
      pll->head = buff ; 
    pll->tail = buff ; 
    LL_NEXT(buff) = LL_LAST ; 
    pll->iSize++ ;
  }
  else
    rc = NULL ; 
  MUTEX_UNLOCK(&pll->mutex) ; 
  return rc ; 
}

/*******************************************************/
/*******************************************************/

/*******************************************************/
/*******************************************************/

/*******************************************************/
/*******************************************************/
static RMM_INLINE int LL_get_nBuffs_0(LinkedListRec *pll)
{
  return pll->iSize ;
}
static RMM_INLINE int LL_get_nBuffs_1(LinkedListRec *pll)
{
 int nBuffs ; 
  MUTEX_POLLLOCK(&pll->mutex) ; 
  nBuffs = pll->iSize ;
  MUTEX_UNLOCK(&pll->mutex) ; 
  return nBuffs ; 
}

/*******************************************************/
/*******************************************************/
void * LL_see_buff_r(LinkedListRec *pll, int rPos, int iLock)
{
 void *p ; 

  if ( iLock ) MUTEX_POLLLOCK(&pll->mutex) ; 
  if ( rPos >= 0 && rPos < pll->iSize )
  {
    int n;
    for ( n=0, p=pll->head ; n<rPos && p ; n++, p=LL_NEXT(p) ) ; 
  }
  else
    p = NULL ; 
  if ( iLock ) MUTEX_UNLOCK(&pll->mutex) ; 
  return p ; 
}

/*******************************************************/
/*******************************************************/
LinkedListRec * LL_alloc(int off, int base)
{
  LinkedListRec *pll ; 

  if ( off < 0 )
    return NULL ; 

  if ( (pll = (LinkedListRec *)malloc(sizeof(LinkedListRec))) != NULL )
  {
    memset(pll,0,sizeof(LinkedListRec)) ; 
    pll->iOff  = off ; 
    pll->iBase = base ; 
    MUTEX_INIT(&pll->mutex) ; 
    COND_INIT( &pll->condE) ; 
    COND_INIT( &pll->condF) ; 
  }
  return pll ; 

}

/*******************************************************/
/*******************************************************/
int    LL_lock(LinkedListRec *pll)
{
  MUTEX_POLLLOCK(&pll->mutex) ; 
  return 0;
}

/*******************************************************/
/*******************************************************/
int    LL_unlock(LinkedListRec *pll)
{
  return MUTEX_UNLOCK(&pll->mutex) ; 
}

/*******************************************************/
/*******************************************************/
int    LL_trylock(LinkedListRec *pll)
{
  return MUTEX_TRYLOCK(&pll->mutex) ; 
}
/*******************************************************/
/*******************************************************/
void   LL_waitE(LinkedListRec *pll)
{
  pll->waitE++ ; 
  COND_WAIT(&pll->condE,&pll->mutex) ; 
  pll->waitE-- ; 
}
void   LL_waitF(LinkedListRec *pll)
{
  pll->waitF++ ; 
  COND_WAIT(&pll->condF,&pll->mutex) ; 
  pll->waitF-- ; 
}
int    LL_waitingF(LinkedListRec *pll)
{
  return pll->waitF ; 
}
void   LL_twaitF(LinkedListRec *pll, int milli)
{
  pll->waitF++ ; 
  COND_TWAIT(&pll->condF,&pll->mutex,(milli/1000),1000000*(milli%1000)) ; 
  pll->waitF-- ; 
}

/*******************************************************/
/*******************************************************/
void   LL_signalE(LinkedListRec *pll)
{
  if ( pll->waitE ) COND_SIGNAL(&pll->condE) ; 
}
void   LL_signalF(LinkedListRec *pll)
{
  if ( pll->waitF ) COND_SIGNAL(&pll->condF) ; 
}

/*******************************************************/
/*******************************************************/
void LL_free(LinkedListRec *pll, int free_buffs)
{
  void * buff ; 

  if ( pll->iSize > 0 )
  {
    if ( free_buffs )
    {
      while ( (buff=LL_get_buff(pll,1)) != NULL )
        free(buff) ; 
    }
  }
  MUTEX_DESTROY(&pll->mutex) ; 
  COND_DESTROY( &pll->condE) ; 
  COND_DESTROY( &pll->condF) ; 
  free(pll) ; 

}
#endif
