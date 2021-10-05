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

#ifndef  H_MemMan_C
#define  H_MemMan_C

#include "MemMan.h"

/*******************************************************/
/*******************************************************/
static RMM_INLINE void *MM_get_buff(MemManRec *pmm, int MaxTry, int *iError)
{
  void *buff=NULL ;

  MUTEX_POLLLOCK(&pmm->mutex) ;
  for(;;)
  {
    if ( pmm->iCurIdle > 0 )
    {
      buff = pmm->pbb[--pmm->iCurIdle] ;
      *iError = MEMORY_ALERT_OFF ;
      break ;
    }
    if ( pmm->iCurSize < pmm->iMaxSize || MaxTry == -1 )
    {
      pmm->iCurSize++ ;
      *iError = (pmm->iCurSize > pmm->iMaxSize) ? MEMORY_ALERT_HIGH : MEMORY_ALERT_OFF ;
      MUTEX_UNLOCK(&pmm->mutex) ;
      if ( (buff=malloc(pmm->iBufSize)) != NULL )
      {
        if ( pmm->iLenZero )
          memset((char *)buff+pmm->iOffZero,0,pmm->iLenZero) ; 
        MUTEX_POLLLOCK(&pmm->mutex) ;
        break ;
      }
      MUTEX_POLLLOCK(&pmm->mutex) ;
      pmm->iCurSize-- ;
      *iError = MEMORY_ALERT_HIGH ;
    }
    else
      *iError = MEMORY_ALERT_MID ;
    if ( --MaxTry <= 0)
      break ;
    pmm->iWaiting++ ; 
    COND_TWAIT(&pmm->cond,&pmm->mutex,0,BASE_WAIT_NANO) ; 
    pmm->iWaiting-- ; 
  }
  if ( *iError == MEMORY_ALERT_OFF )
  {
    if ( pmm->iCurSize - pmm->iCurIdle > pmm->iHiwMark ||
        (pmm->iLastErr != MEMORY_ALERT_OFF &&
         pmm->iCurSize - pmm->iCurIdle > pmm->iLowMark) )
      *iError = MEMORY_ALERT_LOW ;
  }
  pmm->iLastErr = *iError ; 
  MUTEX_UNLOCK(&pmm->mutex) ;

  return buff ;

}

/*******************************************************/
/*******************************************************/
/*******************************************************/
/*******************************************************/
static RMM_INLINE void *MM_put_buff(MemManRec *pmm, void *buff)
{
  MUTEX_POLLLOCK(&pmm->mutex) ;
  if ( pmm->iCurIdle >= pmm->iBoxSize ||
       pmm->iCurSize >  pmm->iMaxSize  )
  {
    pmm->iCurSize-- ;
    if ( pmm->iWaiting )
      COND_SIGNAL(&pmm->cond) ; 
  
    MUTEX_UNLOCK(&pmm->mutex) ;
    free(buff) ;
    return NULL ; 
  }
  else
  {
    pmm->pbb[pmm->iCurIdle++] = buff ; 
    if ( pmm->iWaiting )
      COND_SIGNAL(&pmm->cond) ; 
  
    MUTEX_UNLOCK(&pmm->mutex) ;
    return buff ; 
  }
}

/*******************************************************/
/*******************************************************/
void MM_put_buffs(MemManRec *pmm, int nbuffs, void **buffs)
{
  int n ; 

  MUTEX_POLLLOCK(&pmm->mutex) ;
  n = nbuffs ; 
  if ( (pmm->iCurSize - pmm->iMaxSize) > 0 )
  {
    n -= (pmm->iCurSize - pmm->iMaxSize) ; 
    if ( n < 0 )
      n = 0 ; 
  }
  if ( n > (pmm->iBoxSize - pmm->iCurIdle) ) 
    n = (pmm->iBoxSize - pmm->iCurIdle) ; 
  if ( n > 0 )
  {
    memcpy(&pmm->pbb[pmm->iCurIdle], buffs, n*sizeof(void *)) ; 
    pmm->iCurIdle += n ; 
  }
  for ( ; n<nbuffs ; n++ )
  {
    pmm->iCurSize-- ;
    free(buffs[n]) ;
  }
  if ( pmm->iWaiting )
    COND_SIGNAL(&pmm->cond) ; 

  MUTEX_UNLOCK(&pmm->mutex) ;
  return ;

}
/*******************************************************/
/*******************************************************/
void  MM_dec_size(MemManRec *pmm, int new_size)
{
  MUTEX_POLLLOCK(&pmm->mutex) ;
  if ( new_size > 0 )
    pmm->iMaxSize = new_size ;
  else
    pmm->iMaxSize = (int)((double)pmm->iCurSize*.95e0) ;
  MUTEX_UNLOCK(&pmm->mutex) ;
  return ;
}
/*******************************************************/
/*******************************************************/
int32_t MM_get_buffs_in_use(MemManRec *pmm)
{
  return pmm->iCurSize - pmm->iCurIdle ;
}

/*******************************************************/
/*******************************************************/
int32_t MM_get_cur_size(MemManRec *pmm)
{
  return pmm->iCurSize  ;
}

/*******************************************************/
/*******************************************************/
int32_t MM_get_max_size(MemManRec *pmm)
{
  return ((pmm->iMaxSize==INT_MAX) ? 0 : pmm->iMaxSize) ; 
}

/*******************************************************/
/*******************************************************/
int32_t MM_get_buf_size(MemManRec *pmm)
{
  return pmm->iBufSize ;
}

/*******************************************************/
/*******************************************************/
int32_t MM_get_nBuffs(MemManRec *pmm)
{
  return pmm->iCurIdle ; 
}

/*******************************************************/
/*******************************************************/
MemManRec * MM_alloc(int32_t buf_size, int32_t box_size, int32_t max_size, int32_t hi_mark, int32_t low_mark)
{
  return MM_alloc2(buf_size, box_size, max_size, hi_mark, low_mark, 0, 0) ; 
}
MemManRec * MM_alloc2(int32_t buf_size, int32_t box_size, int32_t max_size, int32_t hi_mark, int32_t low_mark, int32_t off0, int32_t len0)
{
  MemManRec *pmm ;

  if ( buf_size < 0 || box_size < 0 || off0 < 0 || len0 < 0 || off0+len0 > buf_size )
    return NULL ; 

  if ( (pmm = (MemManRec *)malloc(sizeof(MemManRec))) == NULL )
    return pmm ;
  memset(pmm,0,sizeof(MemManRec)) ; 
  if ( box_size > 0 )
  {
    if ( (pmm->pbb = (void **)malloc(sizeof(void *)*box_size)) == NULL )
    {
      free(pmm) ;
      pmm = (MemManRec *)NULL ;
      return pmm ;
    }
  }

  if ( buf_size < 1024 )
    pmm->iBufSize = (buf_size+63)&(~63) ; 
  else
    pmm->iBufSize = (buf_size+255)&(~255) ; 
  pmm->iBoxSize = box_size ;
  pmm->iMaxSize = (max_size<=0) ? INT_MAX : max_size ; 
  if ( hi_mark <= 0 )
    pmm->iHiwMark = pmm->iLowMark = pmm->iMaxSize ; 
  else
  {
    pmm->iHiwMark = hi_mark ; 
    if ( low_mark <= 0 || low_mark >= hi_mark )
      pmm->iLowMark = pmm->iHiwMark/2 ; 
    else
      pmm->iLowMark = low_mark ; 
  }

  pmm->iOffZero = off0 ; 
  pmm->iLenZero = len0 ; 

  MUTEX_INIT(&pmm->mutex) ;
  COND_INIT (&pmm->cond) ;
  /* the calls below are only needed to avoid compiler warnings */
  MM_get_buffs_in_use(pmm) ; 
  MM_get_cur_size(pmm) ; 
  MM_get_buf_size(pmm) ; 
  MM_dec_size(pmm,pmm->iMaxSize) ; 

  return pmm ;

}

/*******************************************************/
/*******************************************************/
void    MM_free(MemManRec *pmm, int free_buffs)
{
  void *buff ; 

  MUTEX_POLLLOCK(&pmm->mutex) ;
  if ( pmm->iBoxSize > 0 )
  {
    if ( free_buffs )
      while ( pmm->iCurIdle > 0 )
        if ( (buff=pmm->pbb[--pmm->iCurIdle]) != NULL ) 
          free(buff) ; 
    free(pmm->pbb) ;
  }
  MUTEX_UNLOCK(&pmm->mutex) ;
  MUTEX_DESTROY(&pmm->mutex) ;
  COND_DESTROY(&pmm->cond) ;
  free(pmm) ;

}
#endif
