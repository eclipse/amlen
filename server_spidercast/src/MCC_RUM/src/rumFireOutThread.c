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

#define HB_DEBUG 0  /* print log message per HB sent */

/* constants indicating result of sending bytes on a socket */
#define RUM_FO_SEND_ERROR        -10
#define RUM_FO_PARTIAL_SEND      -11
#define RUM_FO_CONNECTION_FAILED -12

/* constants indicating result of get_partial_packet. must maintain this order! */
#define FO_PARTIAL_SENT      0
#define FO_PARTIAL_POSTPONE -1
#define FO_PARTIAL_MUTEX    -2

/* constants indicating buffer destination after complete write */
#define BUFF_DEST_NONE   0
#define BUFF_DEST_DATA   1
#define BUFF_DEST_NACK   2
#define BUFF_DEST_FREE   3


/* internal functions */
static int send_partial_packet(ConnInfoRec *cInfo, int inst) ; 
static int get_partial_packet(StreamInfoRec_T *pSinf, char **new_buff);
static int send_stream_packets(ConnInfoRec *cInfo, StreamInfoRec_T *pSinf, int packets_to_send, int new_history_size, int rate_limit_on, char **new_buff);
static int send_single_packet( ConnInfoRec *cInfo, StreamInfoRec_T *pSinf, int rate_limit_on, int isOdata, char *packet, int *bytes_sent);
static int trim_stream_history(StreamInfoRec_T *pSinf, int new_history_size);
static int send_unreliable_stream_packets(ConnInfoRec *cInfo, StreamInfoRec_T *pSinf, int packets_to_send, int new_history_size, int rate_limit_on, char **new_buff);
static int repair_history_queue(StreamInfoRec_T *pSinf);
static int is_connT_valid(ConnInfoRec *cInfo);
static int set_connT_invalid(ConnInfoRec *cInfo, int inst, int line);
static int update_stream_array(ConnInfoRec *cInfo, StreamInfoRec_T *pSinf, int inst, int empty_index, int rmv, int lock);
static int send_connection_control_packet(ConnInfoRec *cInfo, rumInstanceRec *rumInfo);
static int send_connection_tx_stream_report_packet(ConnInfoRec *cInfo, rumInstanceRec *rumInfo);
static int send_connection_rx_stream_report_packet(ConnInfoRec *cInfo, rumInstanceRec *rumInfo);
static int send_connection_rx_nack_packet(ConnInfoRec *cInfo, rumInstanceRec *rumInfo);
static void process_rx_stream_report_packet(ConnInfoRec *cInfo, rumInstanceRec *rumInfo) ; 
static THREAD_RETURN_TYPE FireOutThread(void *param);

static int _FO_errno ; 


/********************************************************************************/
int get_partial_packet(StreamInfoRec_T *pSinf, char **new_buff)
{
  int MMerror ; 
  int inst = pSinf->inst_id;
  
  if ( pSinf->batching_time )  /* use system clock for high resolution */
  {
    if ( rmmTRec[inst]->T_advance.BatchGlobal )
    {
      double current_time=sysTime();
      double stamp = rmmMax(pSinf->batching_stamp, rmmTRec[inst]->batching_stamp) ;
      if ( current_time < stamp )
      {
        double fairness_factor = rmmTRec[inst]->number_of_streams * rmmTRec[inst]->min_batching_time ; /* if stream was not serviced for a long time send anyway */
        if ( !((pSinf->batching_stamp + fairness_factor) < current_time) )
        {
          pSinf->stats.partial_2fast++ ; 
          return FO_PARTIAL_POSTPONE;
        }
      }
    }
    else
    {
      if ( sysTime() < pSinf->batching_stamp )
      {
        pSinf->stats.partial_2fast++ ; 
        return FO_PARTIAL_POSTPONE;
      }
    }
  }
  
  /* get the partial packet */
  if ( !(*new_buff) && !(*new_buff = (char *)MM_get_buff(rmmTRec[inst]->DataBuffPool, 0, &MMerror)) )
    return FO_PARTIAL_MUTEX;
  if ( pthread_mutex_trylock(&pSinf->zero_delay_mutex) == 0 )
  {
    mtl_packet_send(pSinf, CALLER_FO,*new_buff);
    *new_buff = NULL ; 
    pSinf->stats.partial_packets++ ; 
    return FO_PARTIAL_SENT;
  }
  else
  {
    pSinf->stats.partial_trylock++ ; 
    return FO_PARTIAL_MUTEX;
  }
}

/********************************************************************************/
int send_stream_packets(ConnInfoRec *cInfo, StreamInfoRec_T *pSinf, int packets_to_send, int new_history_size, int rate_limit_on , char **new_buff)
{
  char *packet ;
  int nbuffs;
  int i, pts ;
  int transmitted, sent, rc=0;
  int bytes_sent =0;
  uint32_t packet_lead;
  int inst = pSinf->inst_id;
  TCHandle tcHandle = NULL;
  /* send SPM if pending ; make sure no other packet is sent before the pending SPM */
  if ( pSinf->spm_pending == RMM_TRUE || pSinf->spm_fo_generate == RMM_TRUE )
  {
    if ( cInfo->wrInfo.bptr != NULL )
      return 0;

    pthread_mutex_lock(&pSinf->spm_mutex) ;
    if ( pSinf->spm_pending == RMM_TRUE || pSinf->spm_fo_generate == RMM_TRUE )
    {
      if ( pSinf->reliability != pSinf->new_reliability )
      {
        pSinf->reliability = pSinf->new_reliability ;
        pSinf->reliable    = (pSinf->reliability != RMM_UNRELIABLE) ;
        pSinf->is_failover = 0 ; 
        pSinf->is_backup   = 0 ; 
        pSinf->no_partial  = (pSinf->app_controlled_batch || pSinf->is_backup)  ; 
      }
  
      if ( pSinf->spm_fo_generate == RMM_TRUE )
        BuildSpmPacket(pSinf, 0);
      /* generate_SPM(pSinf); */
      rc = send_single_packet(cInfo, pSinf, 0, 0, pSinf->Spm_P, &bytes_sent);
      pSinf->spm_pending = RMM_FALSE;
      pSinf->spm_fo_generate = RMM_FALSE;
      pSinf->stats.last_txw_lead-- ;              /* to count SPMs in packet rate */
    }
    pthread_mutex_unlock(&pSinf->spm_mutex) ;
  }
  if (rc < 0 ) 
    return rc;
  
  /* service RDATA Queue */
  if ( new_history_size > 0 )
  {
    pthread_mutex_lock(&pSinf->rdata_mutex) ;
  }
  if ( (nbuffs = LL_get_nBuffs(pSinf->Rdata_Q, 0)) > 0 )
  {
    transmitted = 0;
    pts = ( new_history_size>0 ? nbuffs : rmmMin(nbuffs, (packets_to_send * rmmTRec[inst]->T_advance.RdataSendPercent/100) ) );
    LL_lock(pSinf->Rdata_Q);
    for (i=0; i < pts; i++)
    {
      if ( cInfo->wrInfo.bptr == NULL )
      {
        if ( (packet=LL_get_buff(pSinf->Rdata_Q, 0)) != NULL )
        {
          if ( (rc = send_single_packet(cInfo, pSinf, rate_limit_on, 0, packet, &sent)) > 0 )
          {
            transmitted += sent;
            packets_to_send-- ;
          }
          else
          {
            break;
          }
        }
      }
      else
        rc = send_partial_packet(cInfo,inst);
    }
    LL_unlock(pSinf->Rdata_Q);
    bytes_sent += transmitted;
    pSinf->stats.bytes_retransmitted += transmitted;
    
    if ( rc < 0 )
    {
      if ( new_history_size > 0 )
        pthread_mutex_unlock(&pSinf->rdata_mutex) ;
      return rc;
    }
  }
  /* NIR - TODO make sure RdataQ is empty before trimming history */
  if ( new_history_size > 0 )
  {
    if ( trim_stream_history(pSinf, new_history_size) > 0 )
    {
      if ( rmmTRec[inst]->MemoryAlert > 0 )
      {
        pthread_mutex_lock(&rmmTRec[inst]->MemoryAlert_mutex);
        rmmTRec[inst]->MemoryAlert = 0;
        pthread_mutex_unlock(&rmmTRec[inst]->MemoryAlert_mutex);
      }
    }
    if ( pSinf->monitor_needed.rdata_sqn == 1 )
    {
      repair_history_queue(pSinf);
      pSinf->monitor_needed.rdata_sqn = 0;
    }
    
    pthread_mutex_unlock(&pSinf->rdata_mutex) ;
  }
  
  /* service ODATA Queue */
  if ( packets_to_send <= 0 ) return bytes_sent;
  
  if ( (nbuffs = LL_get_nBuffs(pSinf->Odata_Q, 0)) == 0 )
  {
    if ( pSinf->mtl_messages > 0 && !pSinf->no_partial ) /* zero/config delay batching */
    {
      if ( (rc=get_partial_packet(pSinf,new_buff)) == FO_PARTIAL_SENT )
        nbuffs = LL_get_nBuffs(pSinf->Odata_Q, 0);
      else
        if ( rmmTRec[inst]->partial_pending > rc )
             rmmTRec[inst]->partial_pending = rc ;
    }
  }
  tcHandle = rmmTRec[inst]->tcHandle;
  transmitted = 0;
  pts = rmmMin(nbuffs, packets_to_send );
  for (i=0; i < pts; i++)
  {
    if ( cInfo->wrInfo.bptr == NULL )
    {
      LL_lock(pSinf->Odata_Q) ; 
      packet=LL_get_buff(pSinf->Odata_Q, 0) ; 
      if ( LL_waitingF(pSinf->Odata_Q) && 2*LL_get_nBuffs(pSinf->Odata_Q, 0) <= (int)rmmTRec[inst]->MaxPendingStreamPackets )
        LL_signalF(pSinf->Odata_Q) ; 
      LL_unlock(pSinf->Odata_Q) ; 
      if ( packet != NULL )
      {
        /* increase lead even if only partial packet is sent */
        pSinf->txw_lead++;
        if ( BB_put_buff(pSinf->History_Q, packet, 1) == NULL )
        {
          /* insert to history buffer failed probably because BufferBox is full so clean history */
          trim_stream_history(pSinf, new_history_size);
          if ( BB_put_buff(pSinf->History_Q, packet, 1) == NULL ) /* second fail. give up */
          {
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3270),"",
                  "The RUM Fireout thread failed to insert the packet into the history queue.");
            MM_put_buff(rmmTRec[inst]->DataBuffPool, packet) ;
            return RMM_FAILURE;
          }
        }


        if ( (rc = send_single_packet(cInfo, pSinf,rate_limit_on, 1, packet, &sent)) > 0 )
        {
          transmitted += sent;
          pSinf->stats.packets_sent++;
          if ( pSinf->monitor_needed.lead == 1 )
          {
            memcpy(&packet_lead, (packet+pSinf->sqn_offset), 4); /* data sqn in packet */
            packet_lead = ntohl(packet_lead);
            if ( pSinf->txw_lead != packet_lead )
            {
              pSinf->txw_lead = packet_lead;
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4271),"",
                  "The txw_lead was updated from the packet.");

            }
            pSinf->monitor_needed.lead = 0;
          }
        }
        else
        {
          break;
        }
      }
    }
    else  /* bptr != NULL */
      rc = send_partial_packet(cInfo,inst);
  }
  bytes_sent += transmitted;
  pSinf->stats.bytes_transmitted += transmitted;
  
  if ( rc > 0 )
    return  bytes_sent ;
  else
    return rc;
}
  
  
/********************************************************************************/
int send_single_packet(ConnInfoRec *cInfo, StreamInfoRec_T *pSinf, int rate_limit_on, int isOdata, char *packet, int *bytes_sent)
{
    TCHandle tcHandle = NULL;

  int psize_nbo, rc ; 
  int inst = pSinf->inst_id;
  
  *bytes_sent = 0 ;
  tcHandle = rmmTRec[inst]->tcHandle;

  if ( cInfo->wrInfo.bptr == NULL && cInfo->wrInfo.buffer == NULL)
  {
    memcpy(&psize_nbo, packet, INT_SIZE) ;
    cInfo->wrInfo.reqlen = ntohl(psize_nbo) + INT_SIZE;
    if ( cInfo->wrInfo.reqlen <= 0 || cInfo->wrInfo.reqlen > (int)rmmTRec[inst]->DataPacketSize )
    {
      if ( isOdata && !pSinf->keepHistory)  
        MM_put_buff(rmmTRec[inst]->DataBuffPool, packet) ;
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3271),"%d%d%llx",
          "The packet is not valid. Additional information: packetSize={0}, maxSize={1}, wrInfo.buffer={2}",
          cInfo->wrInfo.reqlen, rmmTRec[inst]->DataPacketSize, LLU_VALUE(cInfo->wrInfo.buffer));
      return RUM_FO_SEND_ERROR;
    }
    cInfo->wrInfo.offset = 0;
    cInfo->wrInfo.bptr   = packet;
    /* if history not maintained make sure the buffer is returned to the pool after write is complete */
    if ( isOdata && !pSinf->keepHistory)  
    {
      cInfo->wrInfo.buffer = packet;
      cInfo->wrInfo.need_free = BUFF_DEST_DATA;
    }
    else
    {
      cInfo->wrInfo.buffer = NULL;
      cInfo->wrInfo.need_free = BUFF_DEST_NONE;
    }
  }
  else
  {
    if ( isOdata && !pSinf->keepHistory)
      MM_put_buff(rmmTRec[inst]->DataBuffPool, packet) ;
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3272),"%llx%llx",
          "The packet is not valid. Additional information: wrInfo.bptr={0}, wrInfo.buffer={1}",
          LLU_VALUE(cInfo->wrInfo.bptr), LLU_VALUE(cInfo->wrInfo.buffer));
    return RUM_FO_SEND_ERROR;
  }
  
  if ( rate_limit_on )
  {
    if ( rmmTRec[inst]->T_config.LimitTransRate == RUM_STATIC_RATE_LIMIT || isOdata )
      get_tokens(rmmTRec[inst]->global_token_bucket, cInfo->wrInfo.reqlen);
    else
      credit_get_tokens(rmmTRec[inst]->global_token_bucket, cInfo->wrInfo.reqlen);
  }
  
  rc = rmm_write(cInfo) ; 
  
  if ( rc >= 0 )
  {
    cInfo->wrInfo.offset += rc ;
    *bytes_sent = cInfo->wrInfo.reqlen; 
    if ( cInfo->wrInfo.offset == cInfo->wrInfo.reqlen )
    {
      if ( cInfo->wrInfo.buffer != NULL && cInfo->wrInfo.need_free == BUFF_DEST_DATA )
      {
        TempPool *TP = cInfo->tempPool ; 
        if ( TP->n >= TP->size )
        {
          MM_put_buffs(rmmTRec[inst]->DataBuffPool, TP->n, TP->buffs) ; 
          TP->n = 0 ; 
        }
        TP->buffs[TP->n++] = cInfo->wrInfo.buffer ;
        cInfo->wrInfo.buffer = NULL;
      }
      cInfo->wrInfo.bptr   = NULL;
      cInfo->wrInfo.offset = 0;
    }
    else
    {
      cInfo->wrInfo.bptr += rc;
      return RUM_FO_PARTIAL_SEND;
    }
  }
  else /* rc == 0 doesn't indicate a failure */
  {
    rc = rmmErrno ; 
    if ( !eWB(rc) )
    {
      _FO_errno = rc ; 
      return RUM_FO_CONNECTION_FAILED;
    }
    else
      return RUM_FO_PARTIAL_SEND;
  }
  
  return rc;
}

/********************************************************************************/
int send_partial_packet(ConnInfoRec *cInfo, int inst)
{
  int  rc ; 

#if HB_DEBUG
  char info[250];
  char * infolist[2];
  int info_len = sizeof(info);


  snprintf(info,info_len, "send_partial_packet: on connection to %s, (remaining %d)",cInfo->req_addr, (cInfo->wrInfo.reqlen-cInfo->wrInfo.offset)) ; 
  infolist[0] = info;
  base_log(inst, RMM_L_I_GENERAL_INFO, RMM_LOGLEV_INFO, 1,infolist,"FireoutThread");
  /* base_log(inst, RMM_L_I_XTRACE_INFO, RMM_LOGLEV_XTRACE, 1,errorlist,"FireoutThread"); */
#endif

  
  if ( cInfo->wrInfo.bptr == NULL || (cInfo->wrInfo.reqlen - cInfo->wrInfo.offset) <= 0 )
    return RUM_FO_SEND_ERROR;
  
  rc = rmm_write(cInfo) ; 
  if ( rc >= 0 )
  {
    cInfo->wrInfo.offset += rc ; 
    if ( cInfo->wrInfo.offset == cInfo->wrInfo.reqlen )
    {
      if ( cInfo->wrInfo.buffer != NULL )
      {
        if ( cInfo->wrInfo.need_free == BUFF_DEST_DATA )
        {
          TempPool *TP = cInfo->tempPool ; 
          if ( TP->n >= TP->size )
          {
            MM_put_buffs(rmmTRec[inst]->DataBuffPool, TP->n, TP->buffs) ; 
            TP->n = 0 ; 
          }
          TP->buffs[TP->n++] = cInfo->wrInfo.buffer ;
        }
        else
        if ( cInfo->wrInfo.need_free == BUFF_DEST_NACK )
          MM_put_buff(rmmTRec[inst]->rumInfo->nackBuffsQ, cInfo->wrInfo.buffer) ; 

        cInfo->wrInfo.buffer = NULL ;
      }
      cInfo->wrInfo.bptr = NULL;
      cInfo->wrInfo.offset = 0;
    }
    else
    {
      cInfo->wrInfo.bptr += rc;
      return RUM_FO_PARTIAL_SEND;
    }
  }
  else /* rc == 0 doesn't indicate a failure */
  {
    rc = rmmErrno ; 
    if ( !eWB(rc) )
    {
      _FO_errno = rc ; 
      return RUM_FO_CONNECTION_FAILED;
    }
    else
      return RUM_FO_PARTIAL_SEND;
  }
  
  return rc;
}


/********************************************************************************/
int trim_stream_history(StreamInfoRec_T *pSinf, int new_history_size)
{
  int  trim_size, trimed;
  void *packet ;
  char *history_first ; 
  int  find_trail = 0;
  uint32_t new_trail;
  int inst = pSinf->inst_id;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;
  
  if ( (trim_size = BB_get_nBuffs(pSinf->History_Q, 0) - new_history_size) > rmmTRec[inst]->T_advance.MinTrimSize )
  {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9272),"%s%d",
          "trim_stream_history(): trimming history for Topic {0}; number of packets removed = {1}.",
          pSinf->topic_name, trim_size);
    pSinf->txw_trail += trim_size ;
    BB_lock(pSinf->History_Q);
    trimed = trim_size;
    while ( trim_size > 0 )
    {
      if ( (packet = BB_get_buff(pSinf->History_Q, 0)) == NULL )
      {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4272),"%s",
          "The RUM Fireout thread was unable to find the trail sequence number after trimming the history for topic {0}.",
          pSinf->topic_name);
       
        find_trail = 1;       /* error! we must find the trail sqn in packet when finish */
        break;
      }
      else MM_put_buff(rmmTRec[inst]->DataBuffPool, packet) ;
      trim_size-- ;
    }
    if ( find_trail || pSinf->monitor_needed.trail == 1 || pSinf->is_failover )
    {
      if ( (history_first = (char *)BB_see_buff_r(pSinf->History_Q, 0, 0)) != NULL )
      {
        memcpy(&new_trail, (history_first+pSinf->sqn_offset), 4); /* data sqn in packet */
        new_trail = ntohl(new_trail);
        if ( pSinf->txw_trail != new_trail )
        {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4273),"%d%s%d",
          "The trail sequence number ({0}) is not valid for stream {1} and will be updated to {2} from the history packet.",
          pSinf->txw_trail, pSinf->stream_id_str,new_trail);
          
          pSinf->txw_trail = new_trail;
        }
        if ( pSinf->monitor_needed.trail == 1 ) pSinf->monitor_needed.trail = 0;
        
        /* update msg_packet_trail from packet */
      }
    }
    
    BB_unlock(pSinf->History_Q);
    return trimed;
  }
  return 0;
  
}

/********************************************************************************/
int send_unreliable_stream_packets(ConnInfoRec *cInfo, StreamInfoRec_T *pSinf, int packets_to_send, int new_history_size, int rate_limit_on , char **new_buff)
{
  char *packet ;
  int nbuffs;
  int i, pts ;
  int transmitted, sent, rc=0;
  int bytes_sent =0;
  int inst = pSinf->inst_id;
  
  /* send SPM if pending ; make sure no other packet is sent before the pending SPM */
  if ( pSinf->spm_pending == RMM_TRUE || pSinf->spm_fo_generate == RMM_TRUE )
  {
    if ( cInfo->wrInfo.bptr != NULL )
      return 0;

    pthread_mutex_lock(&pSinf->spm_mutex) ;
    if ( pSinf->spm_pending == RMM_TRUE || pSinf->spm_fo_generate == RMM_TRUE )
    {
      if ( pSinf->reliability != pSinf->new_reliability )
      {
        pSinf->reliability = pSinf->new_reliability ;
        pSinf->reliable    = (pSinf->reliability != RMM_UNRELIABLE) ;
      }
  
      if ( pSinf->spm_fo_generate == RMM_TRUE )
        BuildSpmPacket(pSinf, 0);
      /* generate_SPM(pSinf); */
      rc = send_single_packet(cInfo, pSinf, 0, 0, pSinf->Spm_P, &bytes_sent);
      pSinf->spm_pending = RMM_FALSE;
      pSinf->spm_fo_generate = RMM_FALSE;
      pSinf->stats.last_txw_lead-- ;              /* to count SPMs in packet rate */
    }
    pthread_mutex_unlock(&pSinf->spm_mutex) ;
  }
  if (rc < 0 ) 
    return rc;
  
  /* service ODATA Queue */
  if ( (nbuffs = LL_get_nBuffs(pSinf->Odata_Q, 0)) == 0 )
  {
    if ( pSinf->mtl_messages > 0 && !pSinf->no_partial ) /* zero/config delay batching */
    {
      if ( !*new_buff && cInfo->tempPool->n )
        *new_buff = cInfo->tempPool->buffs[--cInfo->tempPool->n] ; 
      if ( (rc=get_partial_packet(pSinf,new_buff)) == FO_PARTIAL_SENT )
        nbuffs = LL_get_nBuffs(pSinf->Odata_Q, 0);
      else
        if ( rmmTRec[inst]->partial_pending > rc )
             rmmTRec[inst]->partial_pending = rc ;
    }
  }
  
  transmitted = 0;
  pts = rmmMin(nbuffs, packets_to_send );
  for (i=0; i < pts; i++)
  {
    if ( cInfo->wrInfo.bptr == NULL )
    {
      LL_lock(pSinf->Odata_Q) ; 
      packet=LL_get_buff(pSinf->Odata_Q, 0) ; 
      if (  LL_waitingF(pSinf->Odata_Q) && 2*LL_get_nBuffs(pSinf->Odata_Q, 0) <= (int)rmmTRec[inst]->MaxPendingStreamPackets )
        LL_signalF(pSinf->Odata_Q) ; 
      LL_unlock(pSinf->Odata_Q) ; 
      if ( packet != NULL )
      {

        /* increase lead even if only partial packet is sent */
        pSinf->txw_lead++;
        /* NIR - TODO decide how to update the lead */
        pSinf->txw_trail = pSinf->txw_lead + 1;
        if ( (rc = send_single_packet(cInfo, pSinf,rate_limit_on, 1, packet, &sent)) > 0 )
        {
          pSinf->stats.packets_sent++;
          transmitted += sent;
        }
        else
        {
          break;
        }
        /* packet is returned to pool when write is completed */
        /* MM_put_buff(rmmTRec[inst]->DataBuffPool, packet) ;  */
      }
    }
    else  /* bptr != NULL */
    {
      rc = send_partial_packet(cInfo,inst);
      break;
    }
  }
  bytes_sent += transmitted;
  pSinf->stats.bytes_transmitted += transmitted;
  
  if ( rc > 0 )
    return  bytes_sent ;
  else
    return rc;
}

/********************************************************************************/
int repair_history_queue(StreamInfoRec_T *pSinf)
{
  char *packet;
  uint32_t packet_sqn;
  int inst = pSinf->inst_id;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3273),"",
      "The history queue is not valid and will be initialized.");
  if ( LL_get_nBuffs(pSinf->Rdata_Q, 1) > 0 )  /* make sure Rdata_Q is empty */
  {
    llmSimpleTraceInvoke(rmmTRec[inst]->tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4276),"%s%s",
          "The RDATA queue is not empty while the history queue for stream {0} ({1}) is being repaired.",
          pSinf->stream_id_str, pSinf->topic_name);
    
    while ( LL_get_buff(pSinf->Rdata_Q, 1) != NULL );
  }
  trim_stream_history(pSinf, 1);
  if ( (packet = BB_get_buff(pSinf->History_Q, 0)) != NULL )
  {
    memcpy(&packet_sqn, packet+pSinf->sqn_offset, 4);   /* get sqn from packet    */
    MM_put_buff(rmmTRec[inst]->DataBuffPool, packet);
    if ( ntohl(packet_sqn) == pSinf->txw_lead )
    {
      pSinf->txw_trail = pSinf->txw_lead + 1;         
      return RMM_SUCCESS;
    }
  }
  /* recreate History queue */
  while ( (packet = BB_get_buff(pSinf->History_Q, 0)) != NULL )
    MM_put_buff(rmmTRec[inst]->DataBuffPool, packet);
  
  pSinf->txw_trail = pSinf->txw_lead + 1;
  BB_free(pSinf->History_Q, 1);
  if ( (pSinf->History_Q = BB_alloc(2*rmmTRec[inst]->MaxPendingPackets, pSinf->txw_trail)) == NULL)
  {
      llmSimpleTraceInvoke(rmmTRec[inst]->tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3277),"%s%s",
          "The RUM Fireout thread failed to recreate the RDATA queue for stream {0} ({1}). The stream reliability mode will be changed to unreliable.",
          pSinf->stream_id_str, pSinf->topic_name);
    
    pSinf->reliability = RMM_UNRELIABLE;
    pSinf->reliable    = (pSinf->reliability != RMM_UNRELIABLE) ;
    return RMM_FAILURE;
  }
  return RMM_SUCCESS;
}

/********************************************************************************/
int is_connT_valid(ConnInfoRec *cInfo)
{
  if ( cInfo->is_invalid )
    return 0 ;
  else
    return 1 ; 
}

/********************************************************************************/
int set_connT_invalid(ConnInfoRec *cInfo, int inst, int line)
{
  int stream , n;
  StreamInfoRec_T *pSinf ;
  char *packet;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;
  if ( (cInfo->is_invalid&T_INVALID) )
    return 1;
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4278),"%llu%s%d%d%d",
      "The RUM Fireout thread is going to invalidate connection {0}. Additional information: req_addr={1}, rc={2}, is_inv={3}, line={4}.",
      UINT_64_TO_LLU(cInfo->connection_id),cInfo->req_addr,_FO_errno,cInfo->is_invalid,line);

  pthread_mutex_lock(&rmmTRec[inst]->Fireout_mutex);
  cInfo->is_invalid |= T_INVALID;
  n = cInfo->n_tx_streams ; 
  cInfo->n_tx_streams = 0 ; 
  pthread_mutex_unlock(&rmmTRec[inst]->Fireout_mutex);
  
  if ( cInfo->wrInfo.buffer != NULL )
  {
    if ( cInfo->wrInfo.need_free == BUFF_DEST_DATA )
      MM_put_buff(rmmTRec[inst]->DataBuffPool, cInfo->wrInfo.buffer) ; 
    else
    if ( cInfo->wrInfo.need_free == BUFF_DEST_NACK )
      MM_put_buff(rmmTRec[inst]->rumInfo->nackBuffsQ, cInfo->wrInfo.buffer) ; 
    cInfo->wrInfo.buffer = NULL ;
  }
  
  for ( stream=0; stream<n; stream++)
  {
    if ( (pSinf = (StreamInfoRec_T *)cInfo->tx_streams[stream]) == NULL ) continue;

    if ( pSinf->closed == RMM_TRUE )
    {
      pSinf->FO_in_use = RMM_FALSE;
      pSinf->conn_invalid_time = rmmTRec[inst]->rumInfo->CurrentTime;
      llmSimpleTraceInvoke(rmmTRec[inst]->tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4280),"%s%llu",
          "A closed stream ({0}) was found while invalidating connection {1}.",
          pSinf->stream_id_str, UINT_64_TO_LLU(cInfo->connection_id));
      continue ; 
    }

    /* clean pending queue */
    LL_lock(pSinf->Odata_Q) ; 
    while ( (packet = LL_get_buff(pSinf->Odata_Q, 0)) != NULL )
      MM_put_buff(rmmTRec[inst]->DataBuffPool, packet);
    
    /* mark stream as invalid connection */ 
    pSinf->conn_invalid = RMM_TRUE;
    pSinf->FO_in_use = RMM_FALSE;
    pSinf->conn_invalid_time = rmmTRec[inst]->rumInfo->CurrentTime;
    LL_signalF(pSinf->Odata_Q) ; 
    LL_unlock(pSinf->Odata_Q) ; 

    /* clean history queue */
    if ( pSinf->keepHistory )
      while ( (packet = BB_get_buff(pSinf->History_Q, 0)) != NULL )
        MM_put_buff(rmmTRec[inst]->DataBuffPool, packet) ;
  }
  
  return 1;
}

/********************************************************************************/
int update_stream_array(ConnInfoRec *cInfo, StreamInfoRec_T *pSinf, int inst, int empty_index, int rmv, int lock)
{
  int rc=RMM_SUCCESS;
  
  if ( lock )
    pthread_mutex_lock(&rmmTRec[inst]->Fireout_mutex);

  if ( cInfo->is_invalid&T_INVALID )
    rc = RMM_FAILURE;
  else
  if ( rmv )
  {
    if ( empty_index >= 0 && empty_index < cInfo->n_tx_streams && cInfo->tx_streams[empty_index] == NULL )
      cInfo->tx_streams[empty_index] = cInfo->tx_streams[--cInfo->n_tx_streams];
    else
      rc = RMM_FAILURE;
  }
  else
  {
    if ( cInfo->n_tx_streams < CONNCTION_STREAMS_ARRAY_SIZE )
      cInfo->tx_streams[cInfo->n_tx_streams++] = pSinf ; 
    else
      rc = RMM_FAILURE;
  }
  
  if ( lock )
    pthread_mutex_unlock(&rmmTRec[inst]->Fireout_mutex);
  
  return rc ; 
}

/********************************************************************************/
int send_connection_control_packet(ConnInfoRec *cInfo, rumInstanceRec *rumInfo)
{
  int  rc=0 ;
  uint32_t ccp_sqn;

  int inst = rumInfo->instance_t;
  int diff, diff2 ;
  TCHandle tcHandle = rumInfo->tcHandles[2];
 /*
  if ( !cInfo->init_here )
    return 1 ; 
 */
  diff = (int)(rumInfo->CurrentTime- cInfo->last_t_time);

  if ( diff < cInfo->hb_interval )
  {
    pthread_mutex_lock(&rumInfo->ClockMutex) ; 
    diff2 = (int)(rumInfo->CurrentTime- cInfo->last_t_time);
    pthread_mutex_unlock(&rumInfo->ClockMutex) ; 
    if ( diff2 < cInfo->hb_interval )
    {
      cInfo->hb_skip++ ; 
      return rc;
    }
    else
    {
      if ( diff2 >= 2*cInfo->hb_interval )
      {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4281),"%d%d",
          "The RUM Fireout thread has a visibility problem. Additional information: diff={0}, diff2={1}.",
          diff,diff2);
      }
    }
  }       
  
  if ( cInfo->wrInfo.bptr != NULL )
    return RUM_FO_SEND_ERROR;
  
  /* set CCP as the next packet to write */
  cInfo->wrInfo.bptr   = (char *)cInfo->ccp_header;
  cInfo->wrInfo.buffer = NULL ; 
  cInfo->wrInfo.need_free = BUFF_DEST_NONE;
  cInfo->wrInfo.reqlen = cInfo->ccp_header_len;
  cInfo->wrInfo.offset = 0;
  
  /* update CCP sequence number */
  ccp_sqn = htonl(cInfo->ccp_sqn++);
  memcpy(cInfo->wrInfo.bptr + 20, &ccp_sqn, 4);
  
  rc = rmm_write(cInfo) ; 
  if ( rc >= 0 )
  {
    cInfo->last_t_time = rumInfo->CurrentTime;
    cInfo->wrInfo.offset += rc ; 
    if ( cInfo->wrInfo.offset == cInfo->wrInfo.reqlen )
    {
      cInfo->wrInfo.bptr = NULL;
      cInfo->wrInfo.offset = 0;
    }
    else
    {
      cInfo->wrInfo.bptr += rc;
      return RUM_FO_PARTIAL_SEND;
    }
  }
  else /* rc == 0 doesn't indicate a failure */
  {
    rc = rmmErrno ; 
    if ( !eWB(rc) )
    {
      _FO_errno = rc ; 
      return RUM_FO_CONNECTION_FAILED;
    }
    else
      return RUM_FO_PARTIAL_SEND;
  }

  if ( diff > 3*cInfo->hb_interval )
  {
      llmSimpleTraceInvoke(rmmTRec[inst]->tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4282),"%llu%s%d%d%d%d",
          "No heartbeats were sent on connection {0} to {1} during the last {2} milliseconds. Additional information: hb_interval={3}, hb_timeout={4}, FO_loops={5}.",
          UINT_64_TO_LLU(cInfo->connection_id),cInfo->req_addr, diff, cInfo->hb_interval, cInfo->hb_to,rmmTRec[inst]->FireOutStatus.loops);
  }

#if HB_DEBUG
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6290),"%s%d%d%d%d",
      "send_connection_control_packet: on connection to {0}, (time from last send {1}, hb_interval {2}, hb_skip {3}, FO_loops {4})",
        cInfo->req_addr, diff, cInfo->hb_interval,cInfo->hb_skip,rmmTRec[inst]->FireOutStatus.loops) ; 
#endif

  return rc;
}

/********************************************************************************/
int send_connection_tx_stream_report_packet(ConnInfoRec *cInfo, rumInstanceRec *rumInfo)
{
  int stream, rc=0, num_streams=0;
  char *ptr;
  uint32_t len;
  StreamInfoRec_T *pSinf ;
  
  if ( rumInfo->CurrentTime < cInfo->next_tx_str_rep_time )
    return rc;

  if ( cInfo->wrInfo.bptr != NULL )
    return RUM_FO_SEND_ERROR;
  
  cInfo->next_tx_str_rep_time = rumInfo->CurrentTime + rumInfo->advanceConfig.StreamReportIntervalMilli ;

  ptr = cInfo->tx_str_rep_packet + 24;
  /* praper Stream Report packet */
  for ( stream=0; stream<cInfo->n_tx_streams; stream++)
  {
    if ( (pSinf = (StreamInfoRec_T *)cInfo->tx_streams[stream]) == NULL ) continue;
    num_streams++;
    memcpy(ptr,&pSinf->stream_id,8);
    ptr += 8;
  }
  len = 20 + 8*num_streams;
  ptr = cInfo->tx_str_rep_packet;
  NputInt(ptr, len); 
  ptr = cInfo->tx_str_rep_packet+20;
  NputInt(ptr, num_streams); 

  /* set Stream Report packet as the next packet to write */
  cInfo->wrInfo.bptr   = (char *)cInfo->tx_str_rep_packet ; 
  cInfo->wrInfo.buffer = NULL ; 
  cInfo->wrInfo.need_free = BUFF_DEST_NONE;
  cInfo->wrInfo.reqlen = len+4 ; 
  cInfo->wrInfo.offset = 0;
  
  rc = rmm_write(cInfo) ; 
  if ( rc >= 0 )
  {
    cInfo->last_t_time = rumInfo->CurrentTime;
    cInfo->wrInfo.offset += rc ; 
    if ( cInfo->wrInfo.offset == cInfo->wrInfo.reqlen )
    {
      cInfo->wrInfo.bptr = NULL;
      cInfo->wrInfo.offset = 0;
    }
    else
    {
      cInfo->wrInfo.bptr += rc;
      return RUM_FO_PARTIAL_SEND;
    }
  }
  else /* rc == 0 doesn't indicate a failure */
  {
    rc = rmmErrno ; 
    if ( !eWB(rc) )
    {
      _FO_errno = rc ; 
      return RUM_FO_CONNECTION_FAILED;
    }
    else
      return RUM_FO_PARTIAL_SEND;
  }
  
  return rc;
}

/********************************************************************************/
int send_connection_rx_stream_report_packet(ConnInfoRec *cInfo, rumInstanceRec *rumInfo)
{
  int  rc=0, netint;
  uint32_t len ; 

  if ( !cInfo->str_rep_tx_pending )
    return rc ; 

  if ( cInfo->wrInfo.bptr != NULL )
    return RUM_FO_SEND_ERROR;

  if ( pthread_mutex_trylock(&cInfo->mutex) != 0 )
    return rc ; 

  if ( !cInfo->str_rep_tx_pending )
  {
    pthread_mutex_unlock(&cInfo->mutex) ;
    return rc ; 
  }
  cInfo->str_rep_tx_pending = 0 ; 

  memcpy(&netint,cInfo->rx_str_rep_tx_packet,4) ; 
  len = ntohl(netint) + 4 ; 
  memcpy(cInfo->rx_str_rep_packet,cInfo->rx_str_rep_tx_packet,len) ; 
  pthread_mutex_unlock(&cInfo->mutex) ;
  
  /* set CCP as the next packet to write */
  cInfo->wrInfo.bptr   = cInfo->rx_str_rep_packet ; 
  cInfo->wrInfo.buffer = NULL ; 
  cInfo->wrInfo.need_free = BUFF_DEST_NONE;
  cInfo->wrInfo.reqlen = len;
  cInfo->wrInfo.offset = 0;
  
  rc = rmm_write(cInfo) ; 
  if ( rc >= 0 )
  {
    cInfo->last_t_time = rumInfo->CurrentTime;
    cInfo->wrInfo.offset += rc ; 
    if ( cInfo->wrInfo.offset == cInfo->wrInfo.reqlen )
    {
      cInfo->wrInfo.bptr = NULL;
      cInfo->wrInfo.offset = 0;
    }
    else
    {
      cInfo->wrInfo.bptr += rc;
      return RUM_FO_PARTIAL_SEND;
    }
  }
  else /* rc == 0 doesn't indicate a failure */
  {
    rc = rmmErrno ; 
    if ( !eWB(rc) )
    {
      _FO_errno = rc ; 
      return RUM_FO_CONNECTION_FAILED;
    }
    else
      return RUM_FO_PARTIAL_SEND;
  }
  
  return rc;
}

/********************************************************************************/
int send_connection_rx_nack_packet(ConnInfoRec *cInfo, rumInstanceRec *rumInfo)
{
  int  rc=0 , n=64 ; 
  uint32_t len, netint ; 
  char *packet ; 

  if ( cInfo->wrInfo.bptr != NULL )
    return RUM_FO_SEND_ERROR;

  while ( n-- )
  {
    if ( (packet = BB_get_buff(cInfo->sendNacksQ,1)) == NULL )
      return rc ; 
    BB_signalF(cInfo->sendNacksQ);
  
    memcpy(&netint,packet,4) ; 
    len = ntohl(netint) + 4 ; 
    
    /* set Nack as the next packet to write */
    cInfo->wrInfo.bptr   = packet ; 
    cInfo->wrInfo.buffer = packet ; 
    cInfo->wrInfo.need_free = BUFF_DEST_NACK;
    cInfo->wrInfo.reqlen = len;
    cInfo->wrInfo.offset = 0;
    
    rc = rmm_write(cInfo) ; 
    if ( rc >= 0 )
    {
      cInfo->last_t_time = rumInfo->CurrentTime;
      cInfo->wrInfo.offset += rc ; 
      if ( cInfo->wrInfo.offset == cInfo->wrInfo.reqlen )
      {
        MM_put_buff(rumInfo->nackBuffsQ, cInfo->wrInfo.buffer) ; 
        cInfo->wrInfo.buffer = NULL ; 
        cInfo->wrInfo.bptr = NULL;
        cInfo->wrInfo.offset = 0;
      }
      else
      {
        cInfo->wrInfo.bptr += rc;
        return RUM_FO_PARTIAL_SEND;
      }
    }
    else /* rc == 0 doesn't indicate a failure */
    {
      rc = rmmErrno ; 
      if ( !eWB(rc) )
      {
        _FO_errno = rc ; 
        return RUM_FO_CONNECTION_FAILED;
      }
      else
        return RUM_FO_PARTIAL_SEND;
    }
  }
  
  return rc;
}

/********************************************************************************/
void process_rx_stream_report_packet(ConnInfoRec *cInfo, rumInstanceRec *rumInfo)
{
  int i, stream, num_streams;
  char *bptr;
  uint32_t len;
  StreamInfoRec_T *pSinf ;
  rumEvent ev ; 
  TCHandle tcHandle = rumInfo->tcHandles[2];
  char* eventDescription;
  void *evprms[1];

  if ( !cInfo->str_rep_rx_pending )
    return ; 

  if ( pthread_mutex_trylock(&cInfo->mutex) != 0 )
    return ; 

  if ( !cInfo->str_rep_rx_pending )
  {
    pthread_mutex_unlock(&cInfo->mutex) ;
    return ; 
  }
  cInfo->str_rep_rx_pending = 0 ; 

  bptr = cInfo->rx_str_rep_rx_packet ; 
  NgetInt(bptr,len) ; 

  if ( len < 20 )
  {
    pthread_mutex_unlock(&cInfo->mutex) ;
    return ; 
  }

  bptr += 16 ; 
  NgetInt(bptr,num_streams) ; 

  for ( stream=0; stream<cInfo->n_tx_streams; stream++)
  {
    if ( (pSinf = (StreamInfoRec_T *)cInfo->tx_streams[stream]) == NULL ) continue;
    if ( pSinf->on_event == NULL || pSinf->active == RUM_FALSE )
      continue ; 
    if ( pSinf->create_time + 30000 > rumInfo->CurrentTime )
      continue ; 

    bptr = cInfo->rx_str_rep_rx_packet + 24 ; 
    for( i=0 ; i<num_streams ; i++ )
    {
      if ( memcmp(bptr,&pSinf->stream_id,sizeof(rmmStreamID_t)) == 0 )
        break ; 
      bptr += sizeof(rmmStreamID_t) ; 
    }
    if ( i<num_streams )
      continue ; 
    eventDescription = "Tx stream does not present on rx side.";
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4283),"%s",
      "The transmitter stream {0} is unknown on receiver side.",pSinf->stream_id_str);
    evprms[0] = eventDescription;

    memset(&ev,0,sizeof(rumEvent)) ; 
    ev.stream_id = pSinf->stream_id ; 
    ev.event_params = evprms ; 
    ev.nparams = 1 ; 
    ev.type = RUM_STREAM_NOT_PRESENT ;
    ev.port = cInfo->apiInfo.remote_server_port ; 
    strlcpy(ev.source_addr,cInfo->apiInfo.remote_addr,RUM_ADDR_STR_LEN) ; 
    strlcpy(ev.queue_name,pSinf->topic_name,RUM_MAX_QUEUE_NAME) ; 
   #if USE_EA
    PutRumEvent(rumInfo, &ev, pSinf->on_event, pSinf->event_user) ; 
   #else
    pSinf->on_event(&ev,pSinf->event_user) ;
   #endif
  }

  pthread_mutex_unlock(&cInfo->mutex) ;

}

/********************************************************************************/
THREAD_RETURN_TYPE FireOutThread(void *param)
{
  int inst, this_fo_id , nr ; 
  int stream, i, done, streams_to_serve, nloops = 0 ;
  int bytes_sent, packets_to_send, rc , cntl_bytes , data_bytes ; 
  int rate_limit_on = 0;
  int do_select=0 , go_retry , only_data=0 ; 
  int new_history_size = 0;
  rumInstanceRec *rumInfo ;
  StreamInfoRec_T *pSinf ;
  uint32_t Original_MinHistoryPackets=0 ; 
  pthread_mutex_t visiMutex;
  int *iparam = param;
  struct sigaction sigact ; 
  char *new_buff=NULL ; 
  void *old_buffs[256] ; 
  TempPool TP[1] ; 
 #if USE_POLL
  int polled;
 #else
  SOCKET np1 ; 
  fd_set wfds;
  struct timeval tv ; 
 #endif
  ConnInfoRec *cInfo ; 
  TCHandle tcHandle = NULL;
  char errMsg[LOG_MSG_SIZE];
  inst = iparam[0];
  this_fo_id = iparam[1];
  
  memset(&sigact,0,sizeof(struct sigaction)) ; 
  sigact.sa_handler = SIG_IGN ; 
  sigaction(SIGPIPE,&sigact,NULL) ; 

 #if USE_PRCTL
  {
    char tn[16] ; 
    rmm_strlcpy(tn,__FUNCTION__,16);
    prctl( PR_SET_NAME,(uintptr_t)tn);
  }
 #endif
  
  rumInfo = rmmTRec[inst]->rumInfo; 
    tcHandle = rumInfo->tcHandles[2];
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5069),"%s%llu%d",
    "The {0} thread is running. Thread id: {1} ({2}).",
        "FireOut",LLU_VALUE(my_thread_id()),
#ifdef OS_Linuxx
        (int)gettid());
#else
        (int)my_thread_id());
#endif

  if ( rmm_get_thread_priority(pthread_self(),errMsg,LOG_MSG_SIZE) == RMM_SUCCESS )
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6226),"%s",
    "The RUM receiver got the thread priority successfully. Return Status: {0}", errMsg);


 #ifdef RMM_DBG_PACKET_DROP
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4227),"",
      "WARNING - This RMM library has been compiled with RMM_DBG_PACKET_DROP, it should be used for testing ONLY");
  }
 #endif

  if ( (rc=pthread_mutex_init(&visiMutex, NULL)) != 0 )  
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3269),"%s%d",
        "The RUM transmitter failed to initialize the {0} pthread mutex variable. The error code is {1}.","visiMutex",rc);

    rmmTRec[inst]->FO_Thrds[this_fo_id] = THREAD_EXIT;
    rmm_thread_exit();
  }
  
  llmFD_ZERO(&wfds);
  
  rate_limit_on = (rmmTRec[inst]->T_config.LimitTransRate != RUM_DISABLED_RATE_LIMIT);
  
  if (this_fo_id == 0 )
  {
    for (i=0; i < FIREOUT_ARRAY_SIZE ; i++)
    {
      rmmTRec[inst]->FO_Thrds[i] = THREAD_INIT ;  /* make sure in case FO is restarted */
    }
    Original_MinHistoryPackets = rmmTRec[inst]->MinHistoryPackets;
    if ( rmmTRec[inst]->T_advance.FireoutPollMicro > 0)
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5270),"%d%d%d",
          "The FireOut thread {0} was initialized using the following configuration parameters : FireoutPollMicro={1}, npoll={2}.", 
          this_fo_id, rmmTRec[inst]->T_advance.FireoutPollMicro, rmmTRec[inst]->FO_CondWait.pi->npoll);
    }
    rmmTRec[inst]->FireOutStatus.status = THREAD_RUNNING;
  }
  rmmTRec[inst]->FO_Thrds[this_fo_id] = THREAD_RUNNING;

  TP->buffs = old_buffs ; 
  TP->size = rmmTRec[inst]->MaxPacketsAllowed - rmmTRec[inst]->MinHistoryPackets - rmmTRec[inst]->MaxPendingPackets ; 
  if ( TP->size > 256 )
       TP->size = 256 ;
  else
  if ( TP->size < 16 )
       TP->size = 16 ;
  TP->n = 0 ; 

  rmm_rwlock_rdlock(&rumInfo->ConnSyncRW) ; 
  
  while ( 1 )
  {
    rmmTRec[inst]->FireOutStatus.loops++ ; 
    rmmTRec[inst]->last_fo_time = rmmTRec[inst]->rumInfo->CurrentTime;

    /* check for stop condition */
    pthread_mutex_lock(&visiMutex); 
    if ( rmmTRec[inst]->FireOutStatus.status == THREAD_KILL )
    {
      pthread_mutex_unlock(&visiMutex);
      break;
    }
    pthread_mutex_unlock(&visiMutex);
    
    /* main FO thread (id = 0) creates and destroys additional FO threads */
    if (this_fo_id == 0 && nloops >= rmmTRec[inst]->T_advance.MaintenanceLoop)
    {
   /* rmmTRec[inst]->FireOutStatus.loops += nloops; */
      nloops = 0;
      if ( rmmTRec[inst]->MinHistoryPackets < Original_MinHistoryPackets &&
        (uint32_t)(MM_get_nBuffs(rmmTRec[inst]->DataBuffPool)) > HISTORY_INCREASE_PERC*rmmTRec[inst]->MinHistoryPackets/100 )
      {
        rmmTRec[inst]->MinHistoryPackets = (100+HISTORY_INCREASE_PERC)*rmmTRec[inst]->MinHistoryPackets/100;
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5271),"%d",
          "The FireOut thread has increased history to {0} packets.", 
          rmmTRec[inst]->MinHistoryPackets);
      }
    }
    
    bytes_sent = 0 ;
    packets_to_send = rmmTRec[inst]->T_advance.PacketsPerRound;
    
    /* check if history cleaning is needed */
    new_history_size = 0;
    if ( (rmmTRec[inst]->MinHistoryPackets > 0) && (MM_get_buffs_in_use(rmmTRec[inst]->DataBuffPool) > rmmTRec[inst]->T_advance.CleaningMarkPercent*((int)rmmTRec[inst]->MaxPacketsAllowed/100) || rmmTRec[inst]->MemoryAlert > 0) )
    {
      if ( rmmTRec[inst]->MemoryAlert == MEMORY_ALERT_HIGH && this_fo_id == 0 )
      {
        rmmTRec[inst]->MinHistoryPackets = (100-HISTORY_INCREASE_PERC)*rmmTRec[inst]->MinHistoryPackets/100;
      llmSimpleTraceInvoke(rmmTRec[inst]->tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3278),"%d",
          "The RUM Fireout thread has reduced the minimal history size to {0} because a MEMORY_ALERT_HIGH event was raised.",
          (rmmTRec[inst]->MinHistoryPackets*rmmTRec[inst]->DataPacketSize/1024));
      }
      if ( rmmTRec[inst]->number_of_streams > 0 )
        new_history_size = rmmTRec[inst]->MinHistoryPackets / rmmTRec[inst]->number_of_streams;
      else new_history_size = rmmTRec[inst]->MinHistoryPackets;
      packets_to_send = rmmTRec[inst]->T_advance.PacketsPerRoundWhenCleaning;
    }
    
    rmm_rwlock_rdrelock(&rumInfo->ConnSyncRW) ; 
   #if USE_POLL
    polled = 0 ; 
   #else
    np1 = 0 ; 
   #endif
    do_select=0 ; 
    for ( cInfo=rumInfo->firstConnection ; cInfo ; cInfo=cInfo->next )
    {
      if ( cInfo->state != RUM_CONNECTION_STATE_ESTABLISHED ) continue;
      if ( !is_connT_valid(cInfo) )
      {
        llmFD_CLR(cInfo->sfd, &wfds) ; 
        set_connT_invalid(cInfo, inst,__LINE__) ;
        continue ; 
      }
     #if !USE_POLL
      llmFD_SET(cInfo->sfd, &wfds) ; 
      if ( np1 < cInfo->sfd )
        np1 = cInfo->sfd ;
     #endif
      if ( cInfo->wrInfo.bptr != NULL )
        do_select++ ; 
    }
    if ( do_select
        #if !USE_POLL
         && np1 
        #endif
       )
    {
     #if USE_POLL
      polled = 1 ; 
      nr = poll(rumInfo->wfds, rumInfo->nwfds, 10) ; 
      if ( nr < 0 ); // we do nothing here
     #else
      np1++ ; 
    
      /* call select to check ready connections */
      tv.tv_sec  = 0 ; 
      tv.tv_usec = 10000 ; 
      nr = rmm_select((int)np1, NULL, &wfds, NULL, &tv) ;
      if ( nr < 0 )
      {
        if ( (rc = rmmErrno ) != EINTR )
        {
          /*set_why_bad(wb,iBad_select) ;*/
        }
        llmFD_ZERO(&wfds) ;
      }
     #endif
    }
    
    if ( rmmTRec[inst]->FireOutStatus.status == THREAD_KILL )
      break;
    
    do_select = go_retry = 0 ; 
    /* go over all connections and send packets */
    for ( cInfo=rumInfo->firstConnection ; cInfo ; cInfo=cInfo->next )
    {
      if ( !is_connT_valid(cInfo) )
      {
        llmFD_CLR(cInfo->sfd, &wfds) ; 
        set_connT_invalid(cInfo, inst,__LINE__) ;
        continue ; 
      }
      if ( cInfo->state != RUM_CONNECTION_STATE_ESTABLISHED ) continue;
     #if USE_POLL
      if ( polled && !(rumInfo->wfds[cInfo->ind].revents&POLLOUT) )
     #else
      if ( !llmFD_ISSET(cInfo->sfd,&wfds) )
     #endif
      {
        /* NIR - TODO check how long the connection is not writeable and print warning */
        do_select++ ; 
        continue;
      }
      
      /* rest of the code is for  llmFD_ISSET(cInfo->sfd,&wfds)  */
      
      cInfo->tempPool = TP ; 
      /* if partial packet waiting, send it */
      if ( cInfo->wrInfo.bptr != NULL )
      {
        rc = send_partial_packet(cInfo,inst);
        if ( rc < 0 )
        {
          if ( rc == RUM_FO_CONNECTION_FAILED )
          {
            llmFD_CLR(cInfo->sfd, &wfds) ; 
            set_connT_invalid(cInfo, inst,__LINE__) ;
          }
          go_retry |= (rc==RUM_FO_PARTIAL_SEND) ; /* BEAM suppression: not boolean */
          continue ; 
        }
      }
      
      if ( cInfo->wrInfo.bptr != NULL )
      {
        do_select++ ; 
        continue;
      }

      cntl_bytes = data_bytes = 0 ; 
      streams_to_serve = cInfo->n_tx_streams ; 
       /* if ( (streams_to_serve = cInfo->n_tx_streams) == 0 ) */
       /*   continue;                                          */
      stream = cInfo->last_stream_index;
      while ( streams_to_serve-- > 0)
      {
        stream-- ; 
        if ( stream < 0 || stream >= cInfo->n_tx_streams )
          stream = cInfo->n_tx_streams-1 ; 

        if ( (pSinf = ((StreamInfoRec_T *)cInfo->tx_streams[stream])) == NULL )
        {
          /* shouldn't happen, array should be compact */
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4284),"%llu%s%d%d",
              "The stream information found for connection {0} ({1} {2} {3}) was NULL.",
              UINT_64_TO_LLU(cInfo->connection_id),cInfo->req_addr,cInfo->n_tx_streams,stream);
          update_stream_array(cInfo, NULL, inst, -1, 1, 1);
          continue;
        }
        
        /* check is stream was not closed; if closed update stream array  */
        if ( pSinf->closed == RMM_TRUE )
        {
          cInfo->tx_streams[stream] = NULL;
          update_stream_array(cInfo, NULL, inst, stream, 1, 1);
          pSinf->FO_in_use = RMM_FALSE;
          continue;
        }
        
        /* send stream packets */
        pSinf->FireOut_on = RMM_TRUE;
        if ( pSinf->keepHistory )
          rc = send_stream_packets(cInfo, pSinf, packets_to_send, new_history_size, rate_limit_on, &new_buff);
        else
          rc = send_unreliable_stream_packets(cInfo, pSinf, packets_to_send, 0, rate_limit_on, &new_buff);
        
        pSinf->FireOut_on = RMM_FALSE;
        
        if ( rc == RUM_FO_CONNECTION_FAILED )
        {
          llmFD_CLR(cInfo->sfd, &wfds) ; 
          set_connT_invalid(cInfo, inst,__LINE__) ;
          break;
        }
         /* connection congested - move to next connection */
        if ( rc == RUM_FO_PARTIAL_SEND )
        {
          go_retry |= 1 ; 
          break;
        }
        /* move to next stream on current connection */
        if ( rc > 0 )
        {
          bytes_sent += rc;
          data_bytes += rc;
          if (!pSinf->is_backup )
            cInfo->last_t_time = rumInfo->CurrentTime;
        }
      } /* while */
      cInfo->last_stream_index = stream ;
      
      if ( bytes_sent && only_data < 1024 )
        only_data++ ; 
      else
      {
        only_data = 0 ; 
        /* send connection rx nack packet if needed */
        rc = send_connection_rx_nack_packet(cInfo, rumInfo); 
        if ( rc < 0 )
        {
          if ( rc == RUM_FO_CONNECTION_FAILED )
          {
            llmFD_CLR(cInfo->sfd, &wfds) ; 
            set_connT_invalid(cInfo, inst,__LINE__) ;
          }
          go_retry |= (rc==RUM_FO_PARTIAL_SEND) ; /* BEAM suppression: not boolean */
          continue ; 
        }
        cntl_bytes += rc ;
  
        if ( !(cInfo->hb_to <= 0 || (cInfo->one_way_hb && !cInfo->init_here)) )
        {
          /* send connection control packet if needed */
          rc = send_connection_control_packet(cInfo, rumInfo); 
          if ( rc < 0 )
          {
            if ( rc == RUM_FO_CONNECTION_FAILED )
            {
              llmFD_CLR(cInfo->sfd, &wfds) ; 
              set_connT_invalid(cInfo, inst,__LINE__) ;
            }
            go_retry |= (rc==RUM_FO_PARTIAL_SEND) ; /* BEAM suppression: not boolean */
            continue ; 
          }
          cntl_bytes += rc ;
     
          /* send connection tx stream report packet if needed */
          rc = send_connection_tx_stream_report_packet(cInfo, rumInfo); 
          if ( rc < 0 )
          {
            if ( rc == RUM_FO_CONNECTION_FAILED )
            {
              llmFD_CLR(cInfo->sfd, &wfds) ; 
              set_connT_invalid(cInfo, inst,__LINE__) ;
            }
            go_retry |= (rc==RUM_FO_PARTIAL_SEND) ; /* BEAM suppression: not boolean */
            continue ; 
          }
          cntl_bytes += rc ;
          
          /* send connection rx stream report packet if needed */
          rc = send_connection_rx_stream_report_packet(cInfo, rumInfo); 
          if ( rc < 0 )
          {
            if ( rc == RUM_FO_CONNECTION_FAILED )
            {
              llmFD_CLR(cInfo->sfd, &wfds) ; 
              set_connT_invalid(cInfo, inst,__LINE__) ;
            }
            go_retry |= (rc==RUM_FO_PARTIAL_SEND) ; /* BEAM suppression: not boolean */
            continue ; 
          }
          cntl_bytes += rc ;
        }
      }
      
      /* check connection rx stream report packet if arrived */
      process_rx_stream_report_packet(cInfo, rumInfo); 
     #ifdef OS_Linuxx
      if ( cntl_bytes > 10 && data_bytes == 0 )
      {
        int ival=1;
        if (setsockopt(cInfo->sfd,IPPROTO_TCP,TCP_QUICKACK,(rmm_poptval)&ival, sizeof(ival)) < 0 )
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4285),"%d",
              "The RUM Fireout thread failed to set the TCP_QUICKACK option. The error code is {0}.",rmmErrno);
        }
        else
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5272),"",
              "The FireOut thread has set the TCP_QUICKACK option.");
        }
      }
     #endif
    } /* for loop on connections */ 
    
    if ( go_retry )
    {
      if ( bytes_sent == 0 )
      sched_yield() ; 
    }
    else
    if ( this_fo_id == 0 )
    {
      if ( bytes_sent == 0 && !do_select )
      {
        if ( rmmTRec[inst]->partial_pending != FO_PARTIAL_POSTPONE )
        {  
          rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ; 
          rmm_cond_wait(&rmmTRec[inst]->FO_CondWait, 1) ;   /* Fireout is periodically signaled by Monitor thread ; */
          rmm_rwlock_rdlock(&rumInfo->ConnSyncRW) ; 
        }
        else
        {
          rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ; 
          if ( rmmTRec[inst]->T_advance.BatchYield )
            sched_yield() ; 
          else
          {
            rmmTRec[inst]->FO_wakeupTime = sysTime() + rmmTRec[inst]->min_batching_time ;
            rmm_cond_wait(&rmmTRec[inst]->FO_CondSleep, 1) ;
          }
          rmm_rwlock_rdlock(&rumInfo->ConnSyncRW) ; 
        }
      }
      
      rmmTRec[inst]->partial_pending = RMM_FALSE;
    }
    else
    {
      if ( bytes_sent < 100*rmmTRec[inst]->T_advance.MaxPendingQueueKBytes )
      {
        rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ; 
        Sleep(BASE_SLEEP);
        rmm_rwlock_rdlock(&rumInfo->ConnSyncRW) ; 
      }
    }
    
    nloops++;
      
  } /* while loop */
  
  rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ; 
  
  if ( new_buff )
    MM_put_buff(rmmTRec[inst]->DataBuffPool, new_buff) ;

  if ( TP->n > 0 )
  {
    MM_put_buffs(rmmTRec[inst]->DataBuffPool, TP->n, TP->buffs) ; 
    TP->n = 0 ; 
  }
  
  if (this_fo_id == 0 )   /* main FireOut waits until all other FO thread exit */
  {
    done = 0; nloops = 10;
    while ( !done && nloops-- > 0 )
    {
      done = 1;
      for (i=1; i < FIREOUT_ARRAY_SIZE ; i++)
        if ( rmmTRec[inst]->FO_Thrds[i] == THREAD_RUNNING ||
          rmmTRec[inst]->FO_Thrds[i] == THREAD_KILL )
          done = 0;
        Sleep(SHORT_SLEEP);
    }
  }
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5067),"%s%llu",
    "The {0}({1}) thread was stopped.","Fireout",
      LLU_VALUE(my_thread_id()));
  
  pthread_mutex_destroy(&visiMutex);
  rmmTRec[inst]->FO_Thrds[this_fo_id] = THREAD_EXIT;
  
  if (this_fo_id == 0 )
    rmmTRec[inst]->FireOutStatus.status = THREAD_EXIT;
  
  rmm_thread_exit();
  
}
