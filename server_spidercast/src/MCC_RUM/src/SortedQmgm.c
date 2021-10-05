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


#ifndef  H_SortedQmgm_C
#define  H_SortedQmgm_C

#include "SortedQmgm.h"

#define calc_pos \
{\
  rPos = aPos - pbb->iBase ;\
  if ( rPos - pbb->iGet < 0 ) *iError = -1 ;\
  else if ( rPos - (pbb->iGet + pbb->iSize) >= 0 ) *iError = 1 ;\
  else {if ( rPos - pbb->iSize >= 0 ) rPos -= pbb->iSize ; *iError = 0;}\
}
 
/*******************************************************/
/*******************************************************/
SortedQRec * SQ_alloc(int size,int base)
{
  int iError[1] ; 
  SortedQRec *pbb ;

  if ( (pbb = (SortedQRec *)malloc(sizeof(SortedQRec))) != NULL )
  {
    if ( (pbb->Flags = (char *)malloc(size*sizeof(char))) != NULL )
    {
      if ( (pbb->Buffs = (void **)malloc(size*sizeof(void *))) != NULL )
      {
        memset(pbb->Flags,0,size*sizeof(char  )) ;
        memset(pbb->Buffs,0,size*sizeof(void *)) ;
        pbb->iSize = size ;
        pbb->iGet  = 0 ;
        pbb->iBase = base ;
        MUTEX_INIT(&pbb->mutex) ;
        SQ_set_base(pbb,0,base) ; 
        SQ_lock(pbb) ; 
        SQ_unlock(pbb) ; 
        SQ_AND_flag(pbb,0,base,iError,0) ; 
        SQ_OR_flag (pbb,0,base,iError,0) ; 
        SQ_get_nBuffs(pbb) ;
      }
      else
      {
        free(pbb->Flags) ;
        free(pbb) ;
        pbb = (SortedQRec *)NULL ;
      }
    }
    else
    {
      free(pbb) ;
      pbb = (SortedQRec *)NULL ;
    }
  }
  return pbb ;
}

/*******************************************************/
/*******************************************************/
void   SQ_set_base(SortedQRec *pbb,int iLock,int base)
{

  if ( iLock ) MUTEX_POLLLOCK(&pbb->mutex) ;
    pbb->iBase = base ;
  if ( iLock ) MUTEX_UNLOCK(&pbb->mutex) ;
  return ;

}

/*******************************************************/
/*******************************************************/
void SQ_free(SortedQRec *pbb, int free_buffs)
{
 void *buff ;

  if ( free_buffs )
  {
    while ( SQ_get_tailSN(pbb,0) < SQ_get_headSN(pbb,0) )
    {
      if ( (buff = SQ_inc_tail(pbb,0)) != NULL )
        free(buff) ;
    }
  }

  free(pbb->Flags) ;
  free(pbb->Buffs) ;
  MUTEX_DESTROY(&pbb->mutex) ;
  free(pbb) ;

}

/*******************************************************/
/*******************************************************/

static RMM_INLINE void * SQ_get_buff_0(SortedQRec *pbb,int aPos,int *iError)
{
 int rPos ;
 void *rc ;

  calc_pos ; 
  rc = *iError ? NULL : pbb->Buffs[rPos] ;
  return rc ;
}
/*------------------------------------*/
static RMM_INLINE void * SQ_get_buff_1(SortedQRec *pbb,int aPos,int *iError)
{
 int rPos ;
 void *rc ;

  MUTEX_POLLLOCK(&pbb->mutex) ;
  calc_pos ; 
  rc = *iError ? NULL : pbb->Buffs[rPos] ;
  MUTEX_UNLOCK(&pbb->mutex) ;
  return rc ;

}

/*******************************************************/
/*******************************************************/

static RMM_INLINE int SQ_put_buff_0(SortedQRec *pbb,int aPos,int *iError,void *buff)
{
 int rPos ;
 int rc ;

  calc_pos ; 
  if ( *iError )
    rc = 0 ; 
  else
  {
    if ( pbb->Buffs[rPos] == NULL )
    {
      pbb->Buffs[rPos] = buff ;
      rc = 1 ;
    }
    else
    {
      rc = -1 ;
      if ( buff == NULL )
        pbb->Buffs[rPos] = buff ;
    }
  }
  return rc ;

}
/*******************************************************/
static RMM_INLINE int SQ_put_buff_1(SortedQRec *pbb,int aPos,int *iError,void *buff)
{
 int rPos ;
 int rc ;

  MUTEX_POLLLOCK(&pbb->mutex) ;
  calc_pos ; 
  if ( *iError )
    rc = 0 ; 
  else
  {
    if ( pbb->Buffs[rPos] == NULL )
    {
      pbb->Buffs[rPos] = buff ;
      rc = 1 ;
    }
    else
    {
      rc = -1 ;
      if (!buff )
        pbb->Buffs[rPos] = buff ;
    }
  }
  MUTEX_UNLOCK(&pbb->mutex) ;
  return rc ;

}

/*******************************************************/
#define SQ_INC_TAIL \
{\
  rc = pbb->Buffs[pbb->iGet] ;\
  pbb->Buffs[pbb->iGet] = NULL ;\
  pbb->Flags[pbb->iGet] = 0    ;\
  if ( ++pbb->iGet >= pbb->iSize )\
  {\
    pbb->iGet  = 0 ;\
    pbb->iBase+= pbb->iSize ;\
  }\
}
/*******************************************************/

static RMM_INLINE void * SQ_inc_tail_0(SortedQRec *pbb)
{
 void *rc ;

  SQ_INC_TAIL
  return rc ;

}
/*******************************************************/
static RMM_INLINE void * SQ_inc_tail_1(SortedQRec *pbb)
{
 void *rc=NULL ;

  MUTEX_POLLLOCK(&pbb->mutex) ;
  SQ_INC_TAIL
  MUTEX_UNLOCK(&pbb->mutex) ;
  return rc ;

}

/*******************************************************/
/*******************************************************/

static RMM_INLINE void * SQ_get_tailPP_0(SortedQRec *pbb,uint32_t *sn)
{
 void *rc ;

  if ( (rc = pbb->Buffs[pbb->iGet]) != NULL )
  {
    SQ_INC_TAIL
  }
  else
    *sn = pbb->iBase + pbb->iGet ;
  return rc ;

}
/*******************************************************/
static RMM_INLINE void * SQ_get_tailPP_1(SortedQRec *pbb,uint32_t *sn)
{
 void *rc ;

  MUTEX_POLLLOCK(&pbb->mutex) ;
  if ( (rc = pbb->Buffs[pbb->iGet]) != NULL )
    SQ_inc_tail(pbb,0) ; 
  else
    *sn = pbb->iBase + pbb->iGet ;
  MUTEX_UNLOCK(&pbb->mutex) ;
  return rc ;

}

/*******************************************************/
/*******************************************************/

/*******************************************************/
/*******************************************************/
char   SQ_AND_flag(SortedQRec *pbb,int iLock,int aPos,int *iError,char flag)
{
 int rPos ;
 char rc ;

  if ( iLock ) MUTEX_POLLLOCK(&pbb->mutex) ;
  calc_pos ; 
  if ( *iError )
    rc = 0 ; 
  else
  {
    rc = pbb->Flags[rPos] ;
    pbb->Flags[rPos] &= flag ;
  }
  if ( iLock ) MUTEX_UNLOCK(&pbb->mutex) ;
  return rc ;

}

/*******************************************************/
/*******************************************************/
char   SQ_OR_flag(SortedQRec *pbb,int iLock,int aPos,int *iError,char flag)
{
 int rPos ;
 char rc ;

  if ( iLock ) MUTEX_POLLLOCK(&pbb->mutex) ;
  calc_pos ; 
  if ( *iError )
    rc = 0 ; 
  else
  {
    rc = pbb->Flags[rPos] ;
    if ( flag )
      pbb->Flags[rPos] |= flag ;
  }
  if ( iLock ) MUTEX_UNLOCK(&pbb->mutex) ;
  return rc ;

}

/*******************************************************/
/*******************************************************/
static RMM_INLINE int    SQ_get_tailSN_0(SortedQRec *pbb)
{
  return pbb->iBase + pbb->iGet ;
}
/*******************************************************/
static RMM_INLINE int    SQ_get_tailSN_1(SortedQRec *pbb)
{
 int rc ;

  MUTEX_POLLLOCK(&pbb->mutex) ;
  rc = pbb->iBase + pbb->iGet ;
  MUTEX_UNLOCK(&pbb->mutex) ;
  return rc ;
}

/*******************************************************/
/*******************************************************/
int    SQ_get_headSN(SortedQRec *pbb,int iLock)
{
 int rc ;

  if ( iLock ) MUTEX_POLLLOCK(&pbb->mutex) ;
  rc = pbb->iBase + pbb->iGet + pbb->iSize - 1 ;
  if ( iLock ) MUTEX_UNLOCK(&pbb->mutex) ;
  return rc ;

}

/*******************************************************/
/*******************************************************/
int    SQ_lock(SortedQRec *pbb)
{
  return MUTEX_LOCK(&pbb->mutex) ;
}

/*******************************************************/
/*******************************************************/
int    SQ_unlock(SortedQRec *pbb)
{
  return MUTEX_UNLOCK(&pbb->mutex) ;
}

/*******************************************************/
/*******************************************************/
int SQ_get_nBuffs(SortedQRec *pbb)
{
  int i ,n ;
  for ( n=0 , i=0 ; i<pbb->iSize ; i++ )
    if ( pbb->Buffs[i] )
      n++ ;
  return n ;
}

/*******************************************************/
/*******************************************************/
#endif
