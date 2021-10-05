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


static void rmmTmutex_lock(void);
static void rmmTmutex_unlock(void);
static int rmmTmutex_trylock(double wait_time);

/*------------ Log and Debug functions ---------------------------*/
static void rmm_fprintf(FILE *fp, char *line);
/* Event Announcer */

/*------------ Utility functions ---------------------------*/
static int sqn_is_valid(uint32_t sqn, uint32_t trail, uint32_t lead);
static void rmm_signal_fireout(int inst, int iLock, StreamInfoRec_T *pSinf);
/*------------ Socket functions ---------------------------*/
static void set_gsi_high(uint32_t *target, void *source, int src_len)  ;
static int set_source_nla(int inst) ;
static void get_time_string(char *timestr);

static int  mtl_packet_send(StreamInfoRec_T *pSinf, int caller, char *new_buff) ;
static int  rmmT_submit_message(StreamInfoRec_T *pSinf, const char *msg, int msg_length, int rcms_len, char *rcms_buf, const rmmTxMessage *msg_params, int *ec);
static int  mtl_large_msg_send(StreamInfoRec_T *pSinf, const char *msg, int msg_len, int rcms_len, char *rcms_buf, const rmmTxMessage *msg_params, int props_len, int tf_len, int *ec) ;
static int  large_props_send(StreamInfoRec_T *pSinf, const char *msg, int msg_length, int rcms_len, char *rcms_buf, const rmmTxMessage *msg_params, int props_len, int tf_len, int *ec) ;
static char *get_mtl_buff(StreamInfoRec_T *pSinf, int wt);
static int waitOnPendingQ(StreamInfoRec_T *pSinf, int max_buffs);
static int chk_closed_release(StreamInfoRec_T *pSinf);

#define stream_history_cleaning_needed(pSinf) (pSinf->mem_alert > FO_STR_MEM_ALERT_OFF || ((pSinf->history_time_milli > 0 || pSinf->history_size_packets > 0) && rmmTRec[inst]->CurrentTime > pSinf->next_trim_time))

/***********************************************************/
void rmmTmutex_lock(void)
{
  rmm_mutex_lock(&rmmTmutex) ;
}

void rmmTmutex_unlock(void)
{
  rmm_mutex_unlock(&rmmTmutex) ;
}

int rmmTmutex_trylock(double wait_time)
{
  if ( rmm_mutex_trylock(&rmmTmutex) )
  {
    double etime = sysTime() + wait_time ;
    int i;
    for (i=0;;i++)
    {
      if ( !(i&255) && sysTime() > etime )
        return -1 ;
      if (!rmm_mutex_trylock(&rmmTmutex) )
        return 0 ;
      sched_yield() ;
    }
  }
  return 0 ;
}

/***********************************************************/
/***********************************************************/

/*------------Logging and debug functions-------------------------*/
/***********************************************************/


/***********************************************************/
/* default log and debug functions                         */


/***********************************************************/
void rmm_fprintf(FILE *fp, char *line)
{
#if STD_PRINTS
  if ( fp != NULL )
  {
    fprintf(fp, "%s",line);
    fflush(fp) ;
  }
#endif
}


/*----------  Utility functions   -------------------------*/
/***********************************************************/

/***********************************************************************/
int sqn_is_valid(uint32_t sqn, uint32_t trail, uint32_t lead)
{
  return (XgeY(sqn,trail) && XleY(sqn,lead)) ? RMM_TRUE : RMM_FALSE;
}

/*------------  Socket Utils  -----------------------------------------*/

/***********************************************************************/


/*--------  time Utils  -----------------------------------------------*/

/***********************************************************************/
void get_time_string(char *timestr)
{
  uint16_t milli ;
  char tmptime[32];

  milli  = (uint16_t)(rmmTime(NULL,tmptime)%1000) ;
  snprintf(timestr,32, "%.19s.%3.3hu", tmptime,milli);
}

/***********************************************************************/
void set_gsi_high(uint32_t *target, void *source, int src_len)
{
  unsigned char *src = source ;

  if ( src_len == 4 )
  {
    memcpy(target,src,4) ;
  }
  else
  {
    hash_it(target, src, src_len) ; 
  }
}

/******************************************************************/
int set_source_nla(int inst)
{
  int i,idx ; 
  int nla_index ;
  int scope=0 , s ; 

  for (i=0; i < rmmTRec[inst]->num_interfaces; i++)
  {
    idx = i ; 
    nla_index = 2*idx ;
    if ( idx == 0 ) /* primary Interface */
      rmmTRec[inst]->gsi_high = 0;
    else
      return RMM_FAILURE ;
  
    rmmTRec[inst]->source_nla[nla_index].length = 0;
    rmmTRec[inst]->source_nla[nla_index+1].length = 0;
  
    if ( IF_HAVE_IPV4(&rmmTRec[inst]->localIf[idx]) )
    {
      rmmTRec[inst]->source_nla_afi[nla_index]    = NLA_AFI ;
      rmmTRec[inst]->source_nla[nla_index].length = sizeof(IA4) ;
      memcpy(rmmTRec[inst]->source_nla[nla_index].bytes, &rmmTRec[inst]->localIf[idx].ipv4_addr, rmmTRec[inst]->source_nla[nla_index].length) ;
  
      /* set transmitter gsi_high using Primary Interface */
      if ( scope < (s=llm_addr_scope(rmmTRec[inst]->source_nla[nla_index].bytes, rmmTRec[inst]->source_nla[nla_index].length)) )
      {
        memcpy(&rmmTRec[inst]->gsi_high, rmmTRec[inst]->source_nla[nla_index].bytes, rmmTRec[inst]->source_nla[nla_index].length) ; 
        scope = s ; 
      }
    }
  
    rmmTRec[inst]->source_nla_is_global[idx] = 0 ;
    if ( IF_HAVE_IPV6(&rmmTRec[inst]->localIf[idx]) )
    {
      if ( IF_HAVE_GLOB(&rmmTRec[inst]->localIf[idx]) )
      {
        rmmTRec[inst]->source_nla_afi[nla_index+1]    = NLA_AFI6 ;
        rmmTRec[inst]->source_nla[nla_index+1].length = sizeof(IA6) ;
        memcpy(rmmTRec[inst]->source_nla[nla_index+1].bytes, &rmmTRec[inst]->localIf[idx].glob_addr, rmmTRec[inst]->source_nla[nla_index+1].length) ;
        rmmTRec[inst]->source_nla_is_global[idx] = 1 ;
      }
      else if ( IF_HAVE_SITE(&rmmTRec[inst]->localIf[idx]) )
      {
        rmmTRec[inst]->source_nla_afi[nla_index+1]    = NLA_AFI6 ;
        rmmTRec[inst]->source_nla[nla_index+1].length = sizeof(IA6) ;
        memcpy(rmmTRec[inst]->source_nla[nla_index+1].bytes, &rmmTRec[inst]->localIf[idx].site_addr, rmmTRec[inst]->source_nla[nla_index+1].length) ;
      }
      else if ( IF_HAVE_LINK(&rmmTRec[inst]->localIf[idx]) )
      {
        rmmTRec[inst]->source_nla_afi[nla_index+1]    = NLA_AFI6 ;
        rmmTRec[inst]->source_nla[nla_index+1].length = sizeof(IA6) ;
        memcpy(rmmTRec[inst]->source_nla[nla_index+1].bytes, &rmmTRec[inst]->localIf[idx].link_addr, rmmTRec[inst]->source_nla[nla_index+1].length) ;
      }

      /* set transmitter gsi_high if it was not set to the IPv4 address */
      if ( scope < (s=llm_addr_scope(rmmTRec[inst]->source_nla[nla_index+1].bytes, rmmTRec[inst]->source_nla[nla_index+1].length)) )
      {
        set_gsi_high(&rmmTRec[inst]->gsi_high, rmmTRec[inst]->source_nla[nla_index+1].bytes, rmmTRec[inst]->source_nla[nla_index+1].length) ; 
        scope = s ; 
      }
    }
  
  }

  return RMM_SUCCESS ;
}

/********************************************************************************/
void rmm_signal_fireout(int inst, int iLock, StreamInfoRec_T *pSinf)
{
  rmm_uint64 ct=0;
  int rc=0 ;
  if ( pSinf )
  {
    rc=1;
    pSinf->FO_attn++ ;
  }
  else
  {
    int stream;
    for ( stream=0; stream<rmmTRec[inst]->max_stream_index; stream++)
    {
      if ( (pSinf = rmmTRec[inst]->all_streams[stream]) == NULL ) continue;
      if ( pSinf->closed == RMM_TRUE || pSinf->lb_parent_info ) continue;
      if ( (pSinf->mtl_messages > 0 && !pSinf->no_partial) || LL_get_nBuffs(pSinf->Odata_Q, 0) > 0 ||
            pSinf->spm_pending || pSinf->spm_fo_generate )
      {
        rc=1;
        pSinf->FO_attn++ ;
      }
    }
  }

  if ( rc ||
       (ct=rmmTRec[inst]->rumInfo->CurrentTime)
       > rmmTRec[inst]->last_fo_time + 100 )
  {
    rmm_cond_signal(&rmmTRec[inst]->FO_CondSleep,iLock);
    rmm_cond_signal(&rmmTRec[inst]->FO_CondWait,iLock);
    if ( !rc )
      rmmTRec[inst]->last_fo_time = ct ;
  }
  return ;
}

/***********************   5 5 5 5   *****************************************************/

static int chk_closed_release(StreamInfoRec_T *pSinf)
{
  if ( pSinf->closed == RMM_TRUE || pSinf->release_thread )
  {
    int inst = pSinf->inst_id;
    TCHandle tcHandle = rmmTRec[inst]->tcHandle;
    if ( pSinf->release_thread )
    {
      pSinf->release_thread = 0 ;
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4202),"%s%d%d",
        "The thread executing rmmTxSubmitMsg was signaled while waiting on the congested stream {0}. Additional information: packets in the pending queue {1}, maximum packets allowed in the pending queue {2}.",
        pSinf->stream_id_str, LL_get_nBuffs(pSinf->Odata_Q, 0), rmmTRec[inst]->MaxPendingStreamPackets);
      return -1 ;
    }
    else
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4203),"%s%s",
        "The topic {0} was closed while the thread executing rmmTxSubmitMsg was waiting on the congested stream {1}.",
        pSinf->topic_name,pSinf->stream_id_str);
      return -2 ;
    }
  }
  return 0 ;
}

/***********************   5 5 5 5   *****************************************************/

static int waitOnPendingQ(StreamInfoRec_T *pSinf, int max_buffs)
{
  int rc ;
  while ( LL_get_nBuffs(pSinf->Odata_Q, 0) > max_buffs )
  {
    if ( (rc = chk_closed_release(pSinf)) < 0 )
      return rc ;
    LL_waitF(pSinf->Odata_Q);
  }
  return 0 ;
}

/****************************************************************************/

static char *get_mtl_buff(StreamInfoRec_T *pSinf, int wt)
{
  int inst = pSinf->inst_id;
  int rt ;
  int *MMerror = &rmmTRec[inst]->MemoryAlert ;
  char *new_mtl_buff ;

  if ( !(new_mtl_buff = (char *)MM_get_buff(rmmTRec[inst]->DataBuffPool, 1, MMerror)) )
  {
    while ( wt ) /* BEAM suppression: infinite loop */
    {
      int mt = 1 ;
      LL_lock(pSinf->Odata_Q);
      rt = waitOnPendingQ(pSinf, (int)rmmTRec[inst]->MaxPendingStreamPackets) ;
      LL_unlock(pSinf->Odata_Q);
      if ( rt < -1 )
        break ;
      if ( !(new_mtl_buff = (char *)MM_get_buff(rmmTRec[inst]->DataBuffPool,mt, MMerror)) )
      {
        if ( rt < 0 || chk_closed_release(pSinf) < 0 )
          break ;
      }
      else
        break ;
    }
  }
  return new_mtl_buff ;
}

/****************************************************************************/

static void rmmT_init_mtl_buff(StreamInfoRec_T *pSinf)
{
  char *bptr ; 
  memcpy(pSinf->mtl_buff,&pSinf->Odata_options_size,SHORT_SIZE) ;
  bptr = pSinf->mtl_buff + INT_SIZE ;
  memcpy(bptr, pSinf->pgm_header, PGM_HEADER_LENGTH) ;
  bptr += ODATA_HEADER_LENGTH ;
  /* update MSG_INFO option if needed */
  if ( pSinf->send_msn )
  {
    uli hostlong ;
    char *tptr ;
    tptr = pSinf->Odata_options + pSinf->msg_odata_optins_offset + 4 ; /* offset of msg option trail in odata options */
    hostlong.l = pSinf->msg_packet_trail ;
    NputLong(tptr, hostlong);        /* msg trail                            */
    hostlong.l = pSinf->msg_packet_lead ;
    NputLong(tptr, hostlong);        /* msg lead                             */
    NputChar(tptr, pSinf->reliability);
  }
  if ( pSinf->Odata_options != NULL && pSinf->Odata_options_size > 0 )
    memcpy(bptr, pSinf->Odata_options, pSinf->Odata_options_size);
  pSinf->mtl_buff_init = 1 ; 
}
/****************************************************************************/

       int  rmmT_submit_message(StreamInfoRec_T *pSinf, const char *msg, int msg_length, int rcms_len, char *rcms_buf, const rmmTxMessage *msg_params, int *ec)
{
  int currsize_limit, rc, props_len=0, tf_len=0 ;
  int inst = pSinf->inst_id;
  char *bptr, dont_batch=0 , one_seg=1 ;
  int msg_len = msg_length+rcms_len;

  /*      if ( rmmTRec[inst]->T_config.LogLevel >= RMM_LOGLEV_TRACE )
  max_debug(inst,D_METHOD_ENTRY,0,NULL,"rmmTSubmitMessage (API)"); */
  if ( msg_params )
  {
    if ( msg_params->num_frags > 0 )
      one_seg = 0 ;
    if ( msg_params->msg_sqn )
      *msg_params->msg_sqn = pSinf->msg_sqn;
    dont_batch = msg_params->dont_batch ;

    /* verify correct use of Turbo Flow */
  } /* if ( msg_params ) */

  /* handle msg properties */

  pthread_mutex_lock(&pSinf->zero_delay_mutex) ;

  if ( pSinf->Odata_options_changed )
  {
    if ( mtl_packet_send(pSinf, CALLER_SUB_MSG, NULL) == RMM_FAILURE )
    {
      pthread_mutex_unlock(&pSinf->zero_delay_mutex) ;
      if ( ec ) *ec = RMM_L_E_INTERNAL_SOFTWARE_ERROR ;
      return RMM_FAILURE ;
    }
    pSinf->Odata_options_changed = 0 ;
    construct_odata_options(pSinf      ) ; /* if fails leave options unchanged */
    pSinf->mtl_offset = INT_SIZE + ODATA_HEADER_LENGTH + pSinf->Odata_options_size;
    pSinf->mtl_currsize = pSinf->mtl_offset + FIXED_MTL_HEADER_SIZE;
    pSinf->free_bytes_in_packet = pSinf->mtl_buffsize - (pSinf->mtl_offset + FIXED_MTL_HEADER_SIZE + MTL_FRAG_FIELDS_LENGTH + SHORT_SIZE) ;
    pSinf->mtl_buff_init = 0 ; 
  }

  currsize_limit = pSinf->mtl_buffsize - msg_len ;
  if ( pSinf->mtl_currsize > currsize_limit )
  {
    if ( mtl_packet_send(pSinf, CALLER_SUB_MSG, NULL) == RMM_FAILURE )
    {
      pthread_mutex_unlock(&pSinf->zero_delay_mutex) ;
      if ( ec ) *ec = RMM_L_E_SEND_FAILURE ;
      return RMM_FAILURE ;
    }
  }
  else
  if ( pSinf->mtl_messages >= RMM_MAX_MSGS_IN_PACKET )
  {
    if ( mtl_packet_send(pSinf, CALLER_SUB_MSG, NULL) == RMM_FAILURE )
    {
      pthread_mutex_unlock(&pSinf->zero_delay_mutex) ;
      if ( ec ) *ec = RMM_L_E_SEND_FAILURE ;
      return RMM_FAILURE ;
    }
  }

  if ( pSinf->mtl_currsize <= currsize_limit )  /* message without fragmentation */
  {
    if ( !pSinf->mtl_buff_init )
    {
      if (!pSinf->mtl_buff && !(pSinf->mtl_buff = get_mtl_buff(pSinf, 1)) )
      {
        pthread_mutex_unlock(&pSinf->zero_delay_mutex) ;
        if ( ec ) *ec = RMM_L_E_MEMORY_ALLOCATION_ERROR ;
        return RMM_FAILURE ;
      }
      rmmT_init_mtl_buff(pSinf) ; 
    }
    bptr = pSinf->mtl_buff + pSinf->mtl_currsize ;
    NputShort(bptr, (uint16_t)msg_len) ;

    if ( rcms_len )
    {
      memcpy(bptr, rcms_buf,rcms_len) ;
      bptr += rcms_len ; 
    }

    if ( one_seg )
      memcpy(bptr, msg, msg_length) ;
    else
    {
      int i;
      for ( i=0 ; i<msg_params->num_frags ; i++ )
      {
        memcpy(bptr, msg_params->msg_frags[i],msg_params->frags_lens[i]) ;
        bptr += msg_params->frags_lens[i] ;
      }
    }
    pSinf->mtl_currsize += (msg_len + SHORT_SIZE);
    pSinf->mtl_messages ++ ;
    pSinf->msg_sqn++ ;

    if ( (pSinf->dont_batch || dont_batch) && !pSinf->must_batch )
    {
      if ( pSinf->fast_ack_notification )
        pSinf->mtl_buff[pSinf->fast_ack_offset] = 1 ;
      if ( (rc=mtl_packet_send(pSinf, CALLER_SUB_MSG, NULL)) == RMM_FAILURE )
      {
        pSinf->mtl_currsize -= (msg_len + SHORT_SIZE);
        pSinf->mtl_messages -- ;
        pSinf->msg_sqn-- ;
        if ( ec ) *ec = RMM_L_E_SEND_FAILURE ;
      }
      pthread_mutex_unlock(&pSinf->zero_delay_mutex) ;
      return rc ;
    }

    pthread_mutex_unlock(&pSinf->zero_delay_mutex) ;

    if ( !pSinf->no_partial )
    {
      pSinf->FO_attn++ ;
      if ( rmmTRec[inst]->FO_CondSleep.isOn || rmmTRec[inst]->FO_CondSleep.polling )
      {
        if ( pSinf->batching_time && rmmTRec[inst]->FO_wakeupTime <= sysTime() )
          rmm_cond_signal(&rmmTRec[inst]->FO_CondSleep, 0) ;
      }
      else
        rmm_cond_signal(&rmmTRec[inst]->FO_CondWait,0) ;
    }

    rc = RMM_SUCCESS ;
  }
  else    /* msg must be fragmented */
  {
    rc = mtl_large_msg_send(pSinf, msg, msg_length, rcms_len, rcms_buf, msg_params, props_len, tf_len, ec);
    if ( rc == RMM_SUCCESS )
    {
      pSinf->msg_sqn++;
      pSinf->msg_packet_lead = pSinf->msg_sqn;
      pSinf->mtl_buff_init = 0 ; 
    }

    pthread_mutex_unlock(&pSinf->zero_delay_mutex) ;
  }

  return rc;
}


/***********************   5 5 5 5   *****************************************************/
       int  mtl_packet_send(StreamInfoRec_T *pSinf, int caller, char *new_buff)
{
  char *bptr , *packet=NULL ;
  uint16_t tsdu ;
  int inst = pSinf->inst_id;
  int packet_size;

  uint16_t Odata_options_size ;
  int mtl_messages ;
  int mtl_currsize ;
  pgm_seq mtl_lead_sqn ;
  char *mtl_buff ;
  if ( pSinf->mtl_messages == 0 ) /* no messages to send        */
  {
    if ( caller == CALLER_FO )
    {
      MM_put_buff(rmmTRec[inst]->DataBuffPool, new_buff) ;
      pthread_mutex_unlock(&pSinf->zero_delay_mutex) ;
    }
    return RMM_SUCCESS;
  }

  mtl_buff = pSinf->mtl_buff ;
  pSinf->mtl_buff = new_buff ; /* we must make sure it is NULL when caller is NOT FireOut */
  pSinf->mtl_buff_init = 0 ;

  /* update StreamInfoRec */
  mtl_messages = pSinf->mtl_messages       ; pSinf->mtl_messages = 0;
  mtl_currsize = pSinf->mtl_currsize       ; pSinf->mtl_currsize = pSinf->mtl_offset + FIXED_MTL_HEADER_SIZE;
  mtl_lead_sqn = pSinf->mtl_lead_sqn       ; pSinf->mtl_lead_sqn++;
  pSinf->msg_packet_lead = pSinf->msg_sqn;

  pSinf->stats.messages_sent += mtl_messages;

  if ( pSinf->batching_time )
  {
    double stamp = sysTime() ;
    rmmTRec[inst]->batching_stamp = stamp + rmmTRec[inst]->min_batching_time ;
    pSinf->batching_stamp = stamp + pSinf->batching_time ;
  }

  if ( caller == CALLER_FO )
  {
    if ( LL_put_buff(pSinf->Odata_Q, mtl_buff, 1) == NULL )
    {
      rmmTxPacketHeader *pheader;
      TCHandle  tcHandle = rmmTRec[inst]->tcHandle;
     
      pheader = (rmmTxPacketHeader *)(mtl_buff+rmmTRec[inst]->packet_header_offset) ;
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1467),"%s%llx%llx",    
        "The {0} thread failed to place a packet in the pending queue. buff {1}, ll_next {2).","Fireout",LLU_VALUE(mtl_buff),LLU_VALUE(pheader->ll_next));
        pthread_mutex_unlock(&pSinf->zero_delay_mutex) ;
      Sleep(BASE_SLEEP) ;
        return RMM_FAILURE ;
      }
    pthread_mutex_unlock(&pSinf->zero_delay_mutex) ;
  }

  packet_size = mtl_currsize - INT_SIZE;
  memcpy(&Odata_options_size,mtl_buff,SHORT_SIZE) ;
  packet = mtl_buff;
  NputInt(packet, packet_size); /* packet length prefix in nbo */

  /* construct PGM ODATA Header */
  bptr = packet+PGM_HEADER_LENGTH-2;                 /* bptr points to TSDU Length   */
  tsdu = packet_size - ODATA_HEADER_LENGTH - Odata_options_size ;
  NputShort(bptr,tsdu );                   /* PGM headers not in TSDU      */
  NputInt(bptr, (mtl_lead_sqn+1));    /* lead sqn                     */
  if ( pSinf->reliability != RMM_UNRELIABLE )
  {
    NputInt(bptr, pSinf->txw_trail);         /* trail sqn                    */
  }
  else
  {
    NputInt(bptr, (mtl_lead_sqn+2));  /* trail=lead+1 for unreliable  */
  }

  bptr += Odata_options_size;

  /* construct MTL Header  */
  NputChar(bptr, pSinf->mtl_version) ;
  NputShort(bptr, (short)mtl_messages);
  NputChar(bptr, 0);                                 /* packet not fragmented       */
  /* memset(bptr, 0, MTL_FRAG_FIELDS_LENGTH); */     /* no frag. fields             */

  /* packet completed, if needed, claculate checksum */

  if ( caller == CALLER_SUB_MSG )
  {
    int nb, rc ;
    {
      LL_lock(pSinf->Odata_Q);
      LL_put_buff(pSinf->Odata_Q, mtl_buff, 0) ;
      nb = LL_get_nBuffs(pSinf->Odata_Q, 0) ;
      waitOnPendingQ(pSinf, (int)rmmTRec[inst]->MaxPendingStreamPackets) ;
      LL_unlock(pSinf->Odata_Q);
      if ( nb < 4 )
        rmm_signal_fireout(inst, 1, pSinf);
      rc = RMM_SUCCESS ;
    }
    if ( (pSinf->mtl_buff = get_mtl_buff(pSinf, 0)) )
    {
      rmmT_init_mtl_buff(pSinf) ; 
    }
    return rc ;
  }
  else
    return RMM_SUCCESS ;
}

/***********************   5 5 5 5   *****************************************************/
int  mtl_large_msg_send(StreamInfoRec_T *pSinf, const char *msg, int msg_length, int rcms_len, char *rcms_buf, const rmmTxMessage *msg_params, int props_len, int tf_len, int *ec)
{
  char *bptr ,*tptr , *new_mtl_buff , fast_ack=0;
  const char *mptr ;
  int hlen; //, tf_mode;
  uint16_t tsdu ;
  int nb;
  int msg_offset_in_packet, msg_bytes_in_packet, free_bytes_in_packet, frag_offset = 0;
  uint32_t frag_sqn;
  char frag_pos = FRAGMENT_FIRST;
  int packet_size, msg_len , num_frags=0 , buf_ind=0 ;
  int inst = pSinf->inst_id;
  uli hostlong ;

  if ( msg_params )
  {
    if ( msg_params->num_frags > 0 )
    {
      msg = msg_params->msg_frags[0] ;
      num_frags = msg_params->num_frags ;
    }
    if ( msg_params->dont_batch && pSinf->fast_ack_notification )
      fast_ack = 1 ;
  }
  else
  {
    //tf_mode = RMM_TF_MODE_DISABLED ; /* for static analysis that can't infer this */
    props_len = tf_len = 0 ;        /* for static analysis that can't infer this */
  }

  msg_offset_in_packet = pSinf->mtl_offset + FIXED_MTL_HEADER_SIZE + MTL_FRAG_FIELDS_LENGTH + SHORT_SIZE;  /* the SHORT_SIZE is for the message length */
  free_bytes_in_packet = pSinf->mtl_buffsize - msg_offset_in_packet ;
  frag_sqn = pSinf->mtl_lead_sqn + 1;

  hlen = tf_len + props_len + rcms_len ; 
  if ( hlen > free_bytes_in_packet )
    {
      /* I'll be back... */
    return large_props_send(pSinf, msg, msg_length, rcms_len, rcms_buf, msg_params, props_len, tf_len, ec) ;
  }

  msg_len = msg_length + hlen ;
  if ( num_frags > 0 )
    mptr = msg ;
  else
    mptr = msg - hlen ;

  while ( frag_offset < msg_len )
  {
    if ( !(new_mtl_buff = get_mtl_buff(pSinf, 1)) )
    {
      if (ec) *ec = RMM_L_E_QUEUE_ERROR ;
      return RMM_FAILURE ;
    }

    if ( frag_offset + free_bytes_in_packet < msg_len )
    {
      msg_bytes_in_packet = free_bytes_in_packet;
      frag_pos = (frag_offset == 0) ? FRAGMENT_FIRST : FRAGMENT_MID;
    }
    else
    {
      msg_bytes_in_packet = msg_len - frag_offset;
      frag_pos = FRAGMENT_LAST;
    }

    packet_size =  msg_offset_in_packet + msg_bytes_in_packet - INT_SIZE;
    bptr = new_mtl_buff ;
    NputInt(bptr,packet_size);

    /* construct PGM Header */
    memcpy(bptr, pSinf->pgm_header, PGM_HEADER_LENGTH) ;
    bptr += PGM_HEADER_LENGTH-2;              /* bptr points to TSDU Length  */
    tsdu = packet_size - ODATA_HEADER_LENGTH - pSinf->Odata_options_size;
    NputShort(bptr,tsdu );                 /* PGM headers not in TSDU     */
    NputInt(bptr, (pSinf->mtl_lead_sqn+1));  /* lead sqn                    */
    if ( pSinf->reliability != RMM_UNRELIABLE )
    {
      NputInt(bptr, pSinf->txw_trail);         /* trail sqn                    */
    }
    else
    {
      NputInt(bptr, (pSinf->mtl_lead_sqn+2));  /* trail=lead+1 for unreliable  */
    }

    /* update MSG_INFO option if needed */
    if ( pSinf->send_msn )
    {
      tptr = pSinf->Odata_options + 8 ; /* assuming it's the 1st option!!! */
      hostlong.l = pSinf->msg_packet_trail ;
      NputLong(tptr, hostlong);        /* msg trail                            */
      hostlong.l = pSinf->msg_packet_lead ;
      NputLong(tptr, hostlong);        /* msg lead                             */
      NputChar(tptr, pSinf->reliability);
    }

    /* copy ODATA options */
    memcpy(bptr, pSinf->Odata_options, pSinf->Odata_options_size);
    bptr += pSinf->Odata_options_size;

    /* construct MTL Header  */
    NputChar(bptr, pSinf->mtl_version) ;
    NputShort(bptr, 1);             /* only one message in packet         */
    NputChar(bptr, frag_pos);                 /* fragment position                  */
    NputInt(bptr, frag_sqn);          /* fragmentation fields: sqn          */
    NputInt(bptr, msg_len);           /*             total_message length   */
    NputInt(bptr, frag_offset);       /*             offset                 */
    NputShort(bptr, (short)msg_bytes_in_packet);  /* fragment length in packet   */

    /* copy Turbo Flow label into first fragment if needed */
    if ( frag_pos == FRAGMENT_FIRST && hlen > 0 )
    {
      if ( rcms_len > 0 )
      {
        memcpy(bptr, rcms_buf, rcms_len) ; 
        bptr += rcms_len ; 
      }
      frag_offset += hlen;
      msg_bytes_in_packet -= hlen;
    }
    else
    if ( frag_pos == FRAGMENT_LAST && fast_ack )
      new_mtl_buff[pSinf->fast_ack_offset] = 1 ;

    if ( num_frags > 0 )
    {
      int cur_len ;
      frag_offset += msg_bytes_in_packet;
      for (;;)
      {
        cur_len = msg_params->frags_lens[buf_ind] - (int)(mptr-msg_params->msg_frags[buf_ind]) ;
        if ( cur_len < msg_bytes_in_packet )
        {
          memcpy(bptr, mptr, cur_len); /* copy msg fragment to packet */
          bptr += cur_len ;
          msg_bytes_in_packet -= cur_len ;
          mptr = msg_params->msg_frags[++buf_ind] ;
        }
        else
        {
          memcpy(bptr, mptr, msg_bytes_in_packet); /* copy msg fragment to packet */
          mptr += msg_bytes_in_packet ;
          break ;
        }
      }
    }
    else
    {
      memcpy(bptr, mptr+frag_offset, msg_bytes_in_packet); /* BEAM suppression: accessing beyond memory */
      frag_offset += msg_bytes_in_packet;
    }

    /* packet completed, if needed, claculate checksum */


    {
      LL_lock(pSinf->Odata_Q);
      if ( waitOnPendingQ(pSinf, (int)rmmTRec[inst]->MaxPendingStreamPackets) < 0 )
      {
        LL_unlock(pSinf->Odata_Q);
        MM_put_buff(rmmTRec[inst]->DataBuffPool, new_mtl_buff) ;
        if (ec) *ec = RMM_L_E_INTERRUPT ;
        return RMM_FAILURE ;
      }
      LL_put_buff(pSinf->Odata_Q, new_mtl_buff, 0) ;
      nb = LL_get_nBuffs(pSinf->Odata_Q, 0) ;
      LL_unlock(pSinf->Odata_Q);
      if ( nb < 4 )
        rmm_signal_fireout(inst, 1, pSinf);
    }
    pSinf->mtl_lead_sqn++;
  }
  /* entire message sent to queue */
  pSinf->stats.messages_sent++;

  if ( ec) *ec = 0;
  return RMM_SUCCESS ;

}

/*****************************************************************************************/
int large_props_send(StreamInfoRec_T *pSinf, const char *msg, int msg_length, int rcms_len, char *rcms_buf, const rmmTxMessage *msg_params, int props_len, int tf_len, int *ec)
{
  int hlen;
  rmmTxMessage tx_msg;
  int i, rc, num_frags=0, inst = pSinf->inst_id;
  char *props_buf, *bptr;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;

  hlen = props_len+tf_len+rcms_len ;
  /* allocate buffer for props */
  props_buf = (char *)malloc(hlen) ;
  if ( props_buf == NULL )
  {
    if ( ec ) *ec = RMM_L_E_MEMORY_ALLOCATION_ERROR ;
    LOG_MEM_ERR(tcHandle,"large_props_send",props_len);
    return RMM_FAILURE ;
  }

  if ( msg_params != NULL)
    tx_msg = *msg_params ;
  else
  {
    {
      if ( ec != NULL ) *ec = RMM_L_E_BAD_PARAMETER ;
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(5295),"",
        "The msg_params pointer is NULL while sending a message with properties.");
      free(props_buf);
      return RMM_FAILURE ;
    }
  }
  if ( tx_msg.num_frags > 0 )
    num_frags = tx_msg.num_frags++ ;
  else
    tx_msg.num_frags = 2 ;


  tx_msg.msg_frags = (char **)malloc((tx_msg.num_frags) * sizeof(char *) ) ;
  tx_msg.frags_lens  = (int *)malloc((tx_msg.num_frags) * sizeof(int) ) ;
  if ( tx_msg.msg_frags == NULL || tx_msg.frags_lens == NULL )
  {
    int length = (tx_msg.msg_frags == NULL) ? ((tx_msg.num_frags) * sizeof(char *)) : ((tx_msg.num_frags) * sizeof(int));
    if ( props_buf ) free(props_buf) ;
    if ( tx_msg.msg_frags ) free(tx_msg.msg_frags);
    if ( tx_msg.frags_lens )  free(tx_msg.frags_lens);
    if ( ec ) *ec = RMM_L_E_MEMORY_ALLOCATION_ERROR ;
    LOG_MEM_ERR(tcHandle,"large_props_send",length);
    return RMM_FAILURE ;
  }

 if ( msg_params && num_frags > 0 )
 {
   for ( i=0 ; i<num_frags ; i++ )
   {
     tx_msg.msg_frags[i+1]  = msg_params->msg_frags[i] ;
     tx_msg.frags_lens[i+1] = msg_params->frags_lens[i] ;
   }
 }
 else
 {
   tx_msg.msg_frags[1]  = tx_msg.msg_buf ;
   tx_msg.frags_lens[1] = tx_msg.msg_len ;
 }
 tx_msg.msg_frags[0]  = props_buf ;
 tx_msg.frags_lens[0] = hlen ;

 bptr = props_buf ; 

 if ( rcms_len > 0 )
 {
   memcpy(bptr, rcms_buf, rcms_len) ; 
   bptr += rcms_len ; 
 }

 rc = mtl_large_msg_send(pSinf, msg, (msg_length+hlen), 0, NULL, &tx_msg, 0, 0, ec) ;
 /* free memory */
  {
    if ( props_buf ) free(props_buf) ;
    if ( tx_msg.msg_frags ) free(tx_msg.msg_frags);
    if ( tx_msg.frags_lens )  free(tx_msg.frags_lens);
  }

  return rc;
}


