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

#ifndef  H_rmmRutils_C
#define  H_rmmRutils_C

#include "rmmRutils.h"


/******************************************************************/
void rmmRmutex_lock(void)
{
  rmm_mutex_lock(&rmmRmutex) ;
}
void rmmRmutex_unlock(void)
{
  rmm_mutex_unlock(&rmmRmutex) ;
}
/******************************************************************/
void stop_stream_service(rStreamInfoRec *pst)
{
  rmmReceiverRec *rInfo ;

  rInfo = rInstances[pst->instance_id] ;
  LL_lock(rInfo->mastQ) ;
  if ( pst->inMaQ )
  {
    int n = LL_get_nBuffs(rInfo->mastQ,0) ;
    while ( n-- )
    {
      rStreamInfoRec *qst = (rStreamInfoRec *)LL_get_buff(rInfo->mastQ,0) ;
      if ( pst == qst )
        break ;
      LL_put_buff(rInfo->mastQ,qst,0) ;
    }
  }
  pst->inMaQ = 1 ;
  while ( pst->maIn ) /* BEAM suppression: infinite loop */
  {
    LL_unlock(rInfo->mastQ) ;
    tsleep(0,rInfo->aConfig.BaseWaitNano) ;
    LL_lock(rInfo->mastQ) ;
  }
  LL_unlock(rInfo->mastQ) ;
}
/******************************************************************/
/******************************************************************/
int raise_stream_event(rStreamInfoRec *pst, int type, void **params, int nparams)
{
  int rc=0 ; 
  rmmEvent ev ; 
  rmmReceiverRec *rInfo ;
  rTopicInfoRec *ptp ;

  if ( !pst )
    return 0 ; 
  rInfo = rInstances[pst->instance_id] ;
  ptp   = rInfo->rTopics[pst->topic_id] ;
  if ( !ptp || !ptp->on_event )
    return 0 ; 
  memcpy(&ev, &pst->ev, sizeof(rmmEvent)) ; 
  ev.type = type ; 
  ev.event_params = params ; 
  ev.nparams = nparams ; 
 #if USE_EA
  rc = PutRumEvent(rInfo->gInfo, &ev, ptp->on_event, ptp->ov_user) ;
 #else
  ptp->on_event(&ev,ptp->ov_user) ;
 #endif
  return rc ; 
}
/******************************************************************/

void sortuli(int n, rmm_uint64 *vec, void *v_ind)
{
  int minsize=12, sp, it , *ind=v_ind;
  rmm_uint64 *v=vec , *m , *ls[32] , *rs[32] , t ;
  rmm_uint64 *l , *r , c ;

  sp = 0 ; ls[sp] = v ; rs[sp] = v + (n-1) ;

  while( sp >= 0 )
  {
    while ( rs[sp] - ls[sp] > minsize )
    {
      l = ls[sp] ;
      r = rs[sp] ;

      for ( m=l ; m<r && (*m<=*(m+1)) ; m++ ) ; /* empty body */
      if ( m>=r ) break ;

      m = l + (r-l)/2 ;

      if ( (*l>*m) )
      {
        t = *l ; *l = *m ; *m = t ;
        it = ind[l-v] ; ind[l-v] = ind[m-v] ; ind[m-v] = it ;
      }

      if ( (*m>*r) )
      {
        t = *m ; *m = *r ; *r = t ;
        it = ind[m-v] ; ind[m-v] = ind[r-v] ; ind[r-v] = it ;
        if ( (*l>*m) )
        {
          t = *l ; *l = *m ; *m = t ;
          it = ind[l-v] ; ind[l-v] = ind[m-v] ; ind[m-v] = it ;
        }
      }

      c = *m ;

      while ( r - l > 1 )
      {
        while ( (*(++l)<c) ) ;
        while ( (*(--r)>c) ) ;

        t = *l ; *l = *r ; *r = t ;
        it = ind[l-v] ; ind[l-v] = ind[r-v] ; ind[r-v] = it ;
      }

      l = r - 1 ;

      if ( l < m )
      {
        ls[sp+1] = ls[sp] ;
        rs[sp+1] = l      ;
        ls[sp  ] = r      ;
      }
      else
      {
        ls[sp+1] = r      ;
        rs[sp+1] = rs[sp] ;
        rs[sp  ] = l      ;
      }
      sp++ ;
    }
    sp-- ;
  }

  for ( l = v , m = v + (n-1) ; l < m ; l++ )
  {
    if ( (*l>*(l+1)) )
    {
      c = *(l+1) ;
      it = ind[(l-v)+1] ;
      for ( r = l ; r >= v && (*r>c) ; r-- )
      {
        *(r+1) = *r ;
        ind[(r-v)+1] = ind[(r-v)] ;
      }
      *(r+1) = c ;
      ind[(r-v)+1] = it ;
    }
  }
}

/***********************************************************/

void sortsn(int n, unsigned int *vec)
{
  int minsize=12 , sp ;
  unsigned int *v=vec , *m , *ls[32] , *rs[32] , t ;
  unsigned int *l , *r , c ;

  sp = 0 ; ls[sp] = v ; rs[sp] = v + (n-1) ;

  while( sp >= 0 )
  {
    while ( rs[sp] - ls[sp] > minsize )
    {
      l = ls[sp] ;
      r = rs[sp] ;

      for ( m=l ; m<r && XleY(*m,*(m+1)) ; m++ ) ; /* empty body */
      if ( m>=r ) break ;

      m = l + (r-l)/2 ;

      if ( XgtY(*l,*m) )
      {
        t = *l ; *l = *m ; *m = t ;
      }

      if ( XgtY(*m,*r) )
      {
        t = *m ; *m = *r ; *r = t ;
        if ( XgtY(*l,*m) )
        {
          t = *l ; *l = *m ; *m = t ;
        }
      }

      c = *m ;

      while ( r - l > 1 )
      {
        while ( XltY(*(++l),c) ) ;
        while ( XgtY(*(--r),c) ) ;

        t = *l ; *l = *r ; *r = t ;
      }

      l = r - 1 ;

      if ( l < m )
      {
        ls[sp+1] = ls[sp] ;
        rs[sp+1] = l      ;
        ls[sp  ] = r      ;
      }
      else
      {
        ls[sp+1] = r      ;
        rs[sp+1] = rs[sp] ;
        rs[sp  ] = l      ;
      }
      sp++ ;
    }
    sp-- ;
  }

  for ( l = v , m = v + (n-1) ; l < m ; l++ )
  {
    if ( XgtY(*l,*(l+1)) )
    {
      c = *(l+1) ;
      for ( r = l ; r >= v && XgtY(*r,c) ; r-- )
      {
        *(r+1) = *r ;
      }
      *(r+1) = c ;
    }
  }
}

/** !!!!!!!!!!!!!!!!!!!!!! */


/******************************************************************/

void init_why_bad_msgs(WhyBadRec *wb)
{
  wb->num_why_bad_msgs = 0 ;
  wb->last_why_bad     = 0 ;
}

/******************************************************************/

int add_why_bad_msg(WhyBadRec *wb,int severity, const char *caller, const char *msg)
{
  int rc=0, n ;

  if ( !wb->num_why_bad_msgs )
  {
    clr_why_bad(wb,wb->num_why_bad_msgs) ;
    wb->how_bad_isit[wb->num_why_bad_msgs] = 1 ;
    strlcpy(wb->why_bad_msgs[wb->num_why_bad_msgs++],
      "No more space for diagnostics messages",LEN_WHY_BAD) ;
  }

  if ( wb->num_why_bad_msgs < MAX_WHY_BAD )
  {
    clr_why_bad(wb,wb->num_why_bad_msgs) ;
    n=snprintf(wb->why_bad_msgs[wb->num_why_bad_msgs],LEN_WHY_BAD,"%s: %s",caller,msg) ;
    if ( n > 0 && n < LEN_WHY_BAD )
    {
      wb->how_bad_isit[wb->num_why_bad_msgs] = severity ;
      rc = wb->num_why_bad_msgs++ ;
    }
  }

  return rc ;
}

/******************************************************************/

void get_why_bad_msgs(WhyBadRec *wb,TBHandle tbh){
 /* int first = 1; */
  int i;
  for ( i=0 ; i<wb->num_why_bad_msgs ; i++ )
    if ( wb->why_bad[i] )
    {
      /*if(first){*/
            llmAddTraceData(tbh, "%d%s", "[{0} : {1}]\n",
          wb->why_bad[i],wb->why_bad_msgs[i]);
      /*  first = 0; */
      /*}else{
        llmAddTraceData(tbh, "%d%s", ", [%d : %s]",
          wb->why_bad[i],wb->why_bad_msgs[i]);
      }*/
    }
}
/******************************************************************/

void clr_why_bad(WhyBadRec *wb,int why_bad_ind)
{
  int i ;
  if ( why_bad_ind < 0 )
    for ( i=0 ; i<wb->num_why_bad_msgs ; i++ )
      wb->why_bad[i] = 0 ;
    else
      wb->why_bad[why_bad_ind] = 0 ;
}

/******************************************************************/
void set_why_bad(WhyBadRec *wb,int why_bad_ind)
{
  wb->last_why_bad = why_bad_ind ;
  wb->why_bad[why_bad_ind]++ ;
}


/******************************************************************/
/** !!!!!!!!!!!!!!!!!!!!!! */
uint32_t hash_sid(rmmStreamID_t sid)
{
  sid = (~sid) + (sid << 18);
  sid = sid ^ (sid >> 31);
  sid = sid * 21;
  sid = sid ^ (sid >> 11);
  sid = sid + (sid << 6);
  sid = sid ^ (sid >> 22);
  return (uint32_t)((HASH_MASK&sid)) ;
}
/** !!!!!!!!!!!!!!!!!!!!!! */

void hash_add(HashStreamInfo **ht, HashStreamInfo *pst)
{
  unsigned int i ;

  i = hash_sid(pst->sid) ;
  pst->hsi_next = ht[i] ;
  ht[i] = pst ; 
}

void hash_del(HashStreamInfo **ht, HashStreamInfo *pst)
{
  unsigned int i ;
  HashStreamInfo *p, *q ; 

  i = hash_sid(pst->sid) ;
  for (p=NULL, q=ht[i] ; q ; p=q , q=q->hsi_next)
  {
    if ( q == pst )
    {
      if ( p )
        p->hsi_next = q->hsi_next ; 
      else
        ht[i] = q->hsi_next ; 
      break ; 
    }
  }
}

/** !!!!!!!!!!!!!!!!!!!!!! */

HashStreamInfo *hash_get(HashStreamInfo **ht, rmmStreamID_t sid)
{
  unsigned int i ;
  HashStreamInfo *q ; 

  i = hash_sid(sid) ;
  for (q=ht[i] ; q && q->sid!=sid ; q=q->hsi_next) ; /* empty body */
  return q ; 
}
/** !!!!!!!!!!!!!!!!!!!!!! */

rStreamInfoRec *find_stream(rmmReceiverRec *rInfo, rmmStreamID_t sid)
{
  rStreamInfoRec *pst ;

  pst = rInfo->last_pst ;
  if ( pst && pst->sid == sid )
    return pst ;

  if ( rInfo->rNumStreams < 4 )
    for ( pst=rInfo->firstStream ; pst ; pst=pst->next )
      if ( pst->sid == sid )
      {
        rInfo->last_pst = pst ;
        return pst ;
      }
  pst = (rStreamInfoRec *)hash_get(rInfo->hash_table,sid) ;
  if ( pst && pst->sid == sid )
    rInfo->last_pst = pst ;
  return pst ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

void add_stream(rStreamInfoRec *pst)
{
  rmmReceiverRec *rInfo=NULL ;

  rInfo = rInstances[pst->instance_id] ;
  if ( rInfo->lastStream )
  {
    pst->prev = rInfo->lastStream ;
    rInfo->lastStream->next = pst ;
    rInfo->lastStream = pst ;
  }
  else
    rInfo->firstStream = rInfo->lastStream = pst ;
  rInfo->rNumStreams++ ;
  hash_add(rInfo->hash_table,(HashStreamInfo *)pst) ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

void remove_stream(rStreamInfoRec *pst)
{
  rmmReceiverRec *rInfo ;
 /*  char myName[] = {"remove_stream"} ; */

  rInfo = rInstances[pst->instance_id] ;
  if ( rInfo->last_pst == pst )
    rInfo->last_pst = NULL ;

  if ( pst->prev )
  {
    if ( pst->next )
    {
      pst->prev->next = pst->next ;
      pst->next->prev = pst->prev ;
    }
    else
    {
      pst->prev->next = NULL ;
      rInfo->lastStream = pst->prev ;
    }
  }
  else
  {
    if ( pst->next )
    {
      rInfo->firstStream = pst->next ;
      pst->next->prev = NULL ;
    }
    else
    {
      rInfo->firstStream = NULL ;
      rInfo->lastStream  = NULL ;
    }
  }
  rInfo->TotMsgsOut += pst->TotMsgsOut;

  rInfo->rNumStreams-- ;
  hash_del(rInfo->hash_table,(HashStreamInfo *)pst) ;

  return ;

}


/** !!!!!!!!!!!!!!!!!!!!!! */
int tp_lock(rmmReceiverRec *rInfo, rTopicInfoRec *ptp, int twait,int id)
{
  int ok=0 , i0=0;
  rmm_uint64 to=0 ; 

  if ( ptp->maIn && !twait )
    return 0 ; 

  LL_lock(rInfo->mastQ) ;
  ptp->maOut++ ;
  do
  {
    if ( ptp->maIn )
    {
      if ( !twait )
        break ;
      if ( !to )
        to = rmmTime(NULL,NULL) + twait ;
      i0 = ptp->maIn ;
      rInfo->tp_wma++ ;
      LL_twaitF(rInfo->mastQ,10) ;
      rInfo->tp_wma-- ;
    }
    else
    {
      ptp->maIn = id ;
      ok = 1 ;
    }
  } while ( !ok && rmmTime(NULL,NULL) < to ) ;
  ptp->maOut-- ;
  LL_unlock(rInfo->mastQ) ;
  if ( !ok && twait )
  {
    llmSimpleTraceInvoke(rInfo->tcHandle,LLM_LOGLEV_TRACE,MSG_KEY(8131),"%s%d%d",
      "tp_lock(): topic {0} is busy by id {1} , req_id {2}",ptp->topicName,i0,id);
  }
  return ok ;
}
void tp_unlock(rmmReceiverRec *rInfo, rTopicInfoRec *ptp)
{
  LL_lock(rInfo->mastQ) ;
    ptp->maIn = 0 ;
    LL_signalE(rInfo->mastQ) ;
    if ( rInfo->tp_wma )
      LL_signalF(rInfo->mastQ) ;
  LL_unlock(rInfo->mastQ) ;
  if ( rInfo->UseNoMA )
  wakePP(rInfo, NULL) ;
}

/** ++++++++++++++++++++++ */

/** !!!!!!!!!!!!!!!!!!!!!! */
void return_packet(rmmReceiverRec *rInfo, rmmPacket *rPack)
{
  if ( rInfo )
  {
    rStreamInfoRec *pst = (rStreamInfoRec *)rPack->rmm_pst_reserved ;
    if ( (rPack->rmm_flags_reserved&1) )
    {
      MM_put_buff(rInfo->recvBuffsQ,rPack->rmm_buff_reserved) ;
      if ( rInfo->aConfig.UsePerConnInQ )
        rumR_PerConnInQdn(rInfo, pst) ; 
    }
    else
      free(rPack->rmm_buff_reserved) ;
    MM_put_buff(rInfo->packStrucQ,rPack) ;
  }
  else
  {
    free(rPack->rmm_buff_reserved);
    free(rPack) ;
  }

}
/** !!!!!!!!!!!!!!!!!!!!!! */

void trim_packetQ(rmmReceiverRec *rInfo, rTopicInfoRec  *ptp, int lock)
{
  int n ;
  rmmPacket *tPack ;

  if ( lock )
    LL_lock(ptp->packQ) ;

  if ( ptp->maxPackInQ > 0 )
  {
    n = LL_get_nBuffs(ptp->packQ,0) - ptp->maxPackInQ ;
    while ( n-- > 0 )
    {
      return_packet(rInfo, LL_get_buff(ptp->packQ,0)) ;
      ptp->TotPacksDrop++ ;
    }
  }
  if ( ptp->maxTimeInQ > 0 )
  {
    for (;;)
    {
      if ( (tPack = LL_see_buff_r(ptp->packQ,0,0)) == NULL )
        break ;
      if ( tPack->rmm_expiration > rInfo->CurrentTime )
        break ;
      return_packet(rInfo, LL_get_buff(ptp->packQ,0)) ;
      ptp->TotPacksDrop++ ;
    }
  }

  if ( lock )
    LL_unlock(ptp->packQ) ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

void wakePP(rmmReceiverRec *rInfo, rStreamInfoRec *pst)
{
  LL_lock(rInfo->mastQ) ;
  if ( pst && !pst->inMaQ )
  {
    pst->inMaQ = 1 ;
    LL_put_buff(rInfo->mastQ,pst,0) ;
  }
  LL_signalE(rInfo->mastQ) ;
  LL_unlock(rInfo->mastQ) ;

  LL_lock(rInfo->sockQ) ;
  rInfo->pp_signaled = 1 ;
  LL_signalE(rInfo->sockQ) ;
  LL_unlock(rInfo->sockQ) ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

/** !!!!!!!!!!!!!!!!!!!!!! */


void print_topic_params(const rumRxQueueParameters *params, TBHandle tbh)
{
  if(tbh == NULL) return;
  llmAddTraceData(tbh,"%s","RxQueueParams: name={0}, ",params->queue_name);
  llmAddTraceData(tbh,"%d","reliability={0}, ",
    params->reliability);
  llmAddTraceData(tbh,"%llx%llx%llx%llx",
    "accept_stream={0}, accept_user={1}, on_event={2}, event_user={3}, ",
    LLU_VALUE(params->accept_stream),LLU_VALUE(params->accept_user),
    LLU_VALUE(params->on_event),LLU_VALUE(params->event_user));
  llmAddTraceData(tbh,"%llx%llx%llx%llx",
    "on_message={0}, on_packet={1}, on_data={2}, user={3}, ",
    LLU_VALUE(params->on_message),LLU_VALUE(params->on_packet),
    LLU_VALUE(params->on_data),LLU_VALUE(params->user));

  llmAddTraceData(tbh,"%d%d%d%d",
    "enable_msg_properties={0}, notify_per_packet={1}, goback_time_milli={2}, max_pending_packets={3}, ",
    params->enable_msg_properties, params->notify_per_packet,params->goback_time_milli, params->max_pending_packets);
  llmAddTraceData(tbh,"%d%d",
    "max_queueing_time_milli={0}, stream_join_backtrack={1}, ",
    params->max_queueing_time_milli, params->stream_join_backtrack);
  llmAddTraceData(tbh,"%llx",
    "msg_selector={0}",LLU_VALUE(params->msg_selector));

  if(params->msg_selector != NULL){
    llmAddTraceData(tbh,"%s%llx%llx%llx%d",
      " (msg_selector.selector={0}, msg_selector.app_filters={1},"
      "msg_selector.all_props_filter={2}, msg_selector.all_props_user={3},"
      "msg_selector.app_filters_length={4})",
      params->msg_selector->selector,
      LLU_VALUE(params->msg_selector->app_filters),
      LLU_VALUE(params->msg_selector->all_props_filter),
      LLU_VALUE(params->msg_selector->all_props_user),
      params->msg_selector->app_filters_length);
  }
}

/** !!!!!!!!!!!!!!!!!!!!!! */

void workMA(rmmReceiverRec *rInfo, rStreamInfoRec *pst)
{
  int m, n, l, npk=0 , nps , first=1, wns ;
  pgm_seq sn=0 ;
  char * packet=NULL ;
  rTopicInfoRec  *ptp=NULL ;
 /*  char myName[] = {"workMA"} ; MAKE_LOG_MSG;  */

  do
  {
    LL_lock(rInfo->mastQ) ;
      if ( first )
      {
        first = 0 ;
        if ( pst  )
        {
          ptp = rInfo->rTopics[pst->topic_id] ;
          rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ;
          if ( !ptp->maIn && !ptp->maOut && pst->ns_event )
            goto ok ;
          if ( !pst->inMaQ )
          {
            pst->inMaQ = 1 ;
            LL_put_buff(rInfo->mastQ,pst,0) ;
          }
        }
      }
      else
      {
        ptp->maIn = 0 ;
        pst->maIn = 0 ;
        rInfo->ppIn-- ;
        if ( npk && !pst->inMaQ )
        {
          pst->inMaQ = 1 ;
          LL_put_buff(rInfo->mastQ,pst,0) ;
        }
        if ( rInfo->tp_wma )
          LL_signalF(rInfo->mastQ) ;
      }
      if ( rInfo->ppIn >= rInfo->rNumTopics )
      {
        LL_unlock(rInfo->mastQ) ;
        return ;
      }
      for ( ; ; )
      {
        wns = 0 ;
        n = LL_get_nBuffs(rInfo->mastQ,0) ;
        while ( n-- )
        {
          pst = (rStreamInfoRec *)LL_get_buff(rInfo->mastQ,0) ;
          ptp = rInfo->rTopics[pst->topic_id] ;
          if ( ptp->closed || (pst->killed&1) )
            continue ;
          if ( !pst->ns_event )
            wns = 1 ;
          else
          if ( !ptp->maIn && !ptp->maOut )
            goto ok ;
          LL_put_buff(rInfo->mastQ,pst,0) ;
        }
        if ( wns && !rInfo->pp_wns )
        {
          rInfo->pp_wns++ ;
          LL_waitE(rInfo->mastQ) ;
          rInfo->pp_wns-- ;
        }
        else
        {
          LL_unlock(rInfo->mastQ) ;
          return ;
        }
      }
     ok:
      pst->inMaQ = 0 ;
      pst->maIn = 1 ;
      ptp->maIn = 1 ;
      rInfo->ppIn++ ;
    LL_unlock(rInfo->mastQ) ;

    pst->ma_last_time = rInfo->CurrentTime ;
    npk = 0 ;
    m = 0 ;
    n = 0 ;
    l = 0 ;
    nps = 64 ; //pst->ChunkSize ;

    {
      while ( m<nps && !ptp->closed && !ptp->maOut )
      {
        if ( (packet = SQ_get_tailPP(pst->dataQ,&sn,1)) != NULL )
        {
          pst->TotPacksOut++ ;
          if ( ptp->parseMTL(pst,packet) )
          {
            MM_put_buff(rInfo->recvBuffsQ,packet) ;
            if ( rInfo->aConfig.UsePerConnInQ )
              rumR_PerConnInQdn(rInfo, pst) ; 
          }
          m++ ;
        }
        else
        {
          if ( XgtY(sn,pst->rxw_lead) )
            break ;
          if ( XltY(sn,pst->rxw_trail) || (pst->nakSQ && XltY(sn,SQ_get_tailSN(pst->nakSQ,1))) )
          {
            if ( (packet = SQ_inc_tail(pst->dataQ,1)) == NULL )
            {
              if ( XltY(sn,pst->rxw_trail) )
                n++ ;
              else
                l++ ;
            }
            else
            {
              pst->TotPacksOut++ ;
              if ( ptp->parseMTL(pst,packet) )
              {
                MM_put_buff(rInfo->recvBuffsQ,packet) ;
                if ( rInfo->aConfig.UsePerConnInQ )
                  rumR_PerConnInQdn(rInfo, pst) ; 
              }
              m++ ;
            }
          }
          else
            break ;
        }
      }
      pst->rxw_tail += (m+n+l) ;
    }
    npk = ( m>=nps || ptp->maOut ) ;
    ptp->TotPacksIn  += m ;
    pst->TotVisits++ ;
    if ( (n+l) && pst->mtl_offset )
    {
        TCHandle tcHandle = rInfo->tcHandle;
        char*   eventDescription;
      pst->TotPacksLost += n+l ;
      eventDescription = "Unrecoverable packet loss";
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4099),"%d%s%u%u%u",
          "An unrecoverable packet loss of {0} packets for stream {1} was detected. Additional information: sn={2}, rxw_trail={3}, rxw_lead={4}.",
          n,pst->sid_str,sn,pst->rxw_trail,pst->rxw_lead);
      if ( ptp->on_event )
      {
        void *EvPrms[2] ; EvPrms[0]=eventDescription; EvPrms[1]=&n;
        raise_stream_event(pst, RMM_PACKET_LOSS, EvPrms, 2) ;
      }
    }
  } while ( !rInfo->GlobalSync.goDN );

  return ;
}

#endif
