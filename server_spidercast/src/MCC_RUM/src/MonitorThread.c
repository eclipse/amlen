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


#define MONITOR_INTERVAL_MILLI      250

/* constants for dynamic rate control */
#define DYNAMIC_RATE_UPDATE_MILLI   40
#define RATE_DECREASE_MARK          DYNAMIC_RATE_UPDATE_MILLI/10
#define DECREASE_PERCENT            90
#define RATE_INCREASE_MARK          DYNAMIC_RATE_UPDATE_MILLI/100
#define INCREASE_PERCENT            105

/* constants for dynamic batching time */
#define BATCH_TIME_UPDATE_SEC      2
#define BATCH_TIME_DECREASE_MARK   0
#define BATCH_TIME_INCREASE_MARK   MONITOR_INTERVAL_MILLI/10
#define BATCH_TIME_MAX             4

/* TaskTimer tasks (10 tasks defined, some run only in certain configurations)  */
static rmm_uint64 call_remove_closed_topics(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm);
static rmm_uint64 call_monitor_all_streams(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm);
static rmm_uint64 call_print_snapshot(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm);
static rmm_uint64 call_update_batch_time(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm);
static rmm_uint64 call_update_dynamic_rate(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm);
static rmm_uint64 update_token_bucket(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm);
static rmm_uint64 signal_fireout(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm);


/* internal functions */
static int remove_closed_topics(int inst);
static int monitor_all_streams(int inst, int *naks_last_round);
static int print_snapshot(int inst);
static int update_batch_time(int inst);
static int update_dynamic_rate(int inst, int *naks_last_round);

static int monitor_stream(StreamInfoRec_T *pSinf, int decrease_batch_time);
static int print_stream_data(StreamInfoRec_T *pSinf, TBHandle tbh);
static int print_transmitter_data(int inst, TBHandle tbh);

/* Task 0: set internal clock time ; Interval - 1 (or what the OS can provide) */

/* Task 1: remove_closed_topics ; Interval - MONITOR_INTERVAL_MILLI_LONG (250) */
rmm_uint64 call_remove_closed_topics(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm)
{
  int next_time;
  int inst = taskParm[0];

  remove_closed_topics(inst) ;
  next_time = MONITOR_INTERVAL_MILLI;

  return curTime + next_time ;
}

/* Task 2: monitor_all_streams ; Interval - MONITOR_INTERVAL_MILLI (250) */
rmm_uint64 call_monitor_all_streams(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm)
{
  int next_time;
  int inst = taskParm[0];

  /* taskParm[1] holds the value of last_nak_count */
  if ( monitor_all_streams(inst, &taskParm[1]) == RMM_SUCCESS )
    next_time = MONITOR_INTERVAL_MILLI;
  else
    next_time = MONITOR_INTERVAL_MILLI * 10;

  return curTime + next_time ;
}

/* Task 3: print_snapshot ; Interval - SnapshotCycleMilli_tx */
rmm_uint64 call_print_snapshot(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm)
{
  int next_time;
  int inst = taskParm[0];

  if ( rmmTRec[inst]->T_advance.SnapshotCycleMilli_tx <= 0 )
    return curTime + 10000 ;

  print_snapshot(inst);
  next_time = rmmTRec[inst]->T_advance.SnapshotCycleMilli_tx;

  return curTime + next_time ;
}

/* Task 4: update_batch_time ; Interval - BATCH_TIME_UPDATE_SEC*1000 (10000) */
rmm_uint64 call_update_batch_time(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm)
{
  int next_time;
  int inst = taskParm[0];

  if ( update_batch_time(inst) == RMM_SUCCESS )
    next_time = BATCH_TIME_UPDATE_SEC*1000;
  else
    next_time = 60000;

  return curTime + next_time ;
}

/* Task 5: update_dynamic_rate ; Interval - DYNAMIC_RATE_UPDATE_MILLI (40) */
rmm_uint64 call_update_dynamic_rate(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm)
{
  int next_time;
  int inst = taskParm[0];

  /* taskParm[1] holds the value of last_nak_count */
  if ( update_dynamic_rate(inst, &taskParm[1]) == RMM_SUCCESS )
    next_time = DYNAMIC_RATE_UPDATE_MILLI;
  else
    next_time = 1000;

  return curTime + next_time ;
}

/* Task 6: update_token_bucket ; Interval - bucket->sleep_time (9) */
rmm_uint64 update_token_bucket(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm)
{
  int inst = taskParm[0];
  int time_elapsed;
  token_bucket *bucket;

  if ( inst < 0 || inst >= MAX_Tx_INSTANCES || rmmTRec[inst] == NULL || rmmTrunning == 0 )
    return curTime + 100;

  bucket = rmmTRec[inst]->global_token_bucket;

  if ( bucket == NULL || bucket->status != THREAD_RUNNING )
    return curTime + 100;

  time_elapsed = (int)(curTime -  reqTime) + bucket->sleep_time;
  if ( time_elapsed <= 0 )
    return curTime + 1;
  /* update tokens */
  pthread_mutex_lock(&bucket->mutex);
  bucket->tokens += bucket->tokens_per_milli * time_elapsed;
  if (bucket->tokens > bucket->size)
    bucket->tokens = bucket->size ;
  pthread_mutex_unlock(&bucket->mutex);

  /* send signal for threads waiting for tokens */
  pthread_cond_broadcast(&bucket->waiting);

  return curTime + bucket->sleep_time;
}

/* Task 7: signal_fireout ; Interval 10 */
rmm_uint64 signal_fireout(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *taskParm)
{
  int inst = taskParm[0];
  rmm_signal_fireout(inst, 0, NULL);
  /* in RUM this task also counts Monitor loops since clock task doesn't run  */
  rmmTRec[inst]->MonitorStatus.loops++;
  return curTime + 10;

}

/***** Task 1 *****************************************************************/
int remove_closed_topics(int inst)
{
  StreamInfoRec_T *pSinf;
  int stream;
  rmm_uint64 current_time;
  char *packet;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;

  if ( rmmTmutex_trylock(0e0) != 0 ) return RMM_FAILURE;

  if ( inst < 0 || inst >= MAX_Tx_INSTANCES || rmmTRec[inst] == NULL || rmmTrunning == 0 ||
    rmmTRec[inst]->initialized == RMM_FALSE  )
  {
    rmmTmutex_unlock() ;
    return RMM_FAILURE;
  }

  current_time = rmmTRec[inst]->rumInfo->CurrentTime;

  /* check for closed streams */
  for (stream = 0; stream < rmmTRec[inst]->max_stream_index; stream++)
  {
    if ( (pSinf=rmmTRec[inst]->all_streams[stream]) == NULL ) continue;
    if ( pSinf->active == RMM_TRUE ) continue;

    /* clean closed streams */
    if ( pSinf->closed == RMM_TRUE )
    {
       
      /* if close is seen the first time just set remove time */
      if ( pSinf->remove_time == 0 )
      {
        pSinf->remove_time = current_time + 500;
        continue;
      }

      /* if remove_time passed, remove stream from all_streams array - resources are not freed yet */
      if ( current_time > pSinf->remove_time )
      {
        /* check that stream entry can be moved to closed_streams array */
        if ( rmmTRec[inst]->closed_streams[stream] != NULL )
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4330),"%d%d",
              "A non-empty entry ({0} {1}) was found in the closed streams array.",stream,inst);
          continue;
        }
        /* make sure there is no thread activity on the stream */
        if ( pSinf->Spm_on != RMM_FALSE  || pSinf->Repair_on != RMM_FALSE || pSinf->FireOut_on != RMM_FALSE )
          continue;

        if ( pSinf->FO_in_use == RMM_TRUE )
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4331),"%d%d",
              "A closed stream ({0} {1}) was found with fireout in use.",stream,inst);
          continue;
        }

        /* make sure application is no using zero delay mutex */
        if ( pthread_mutex_trylock(&pSinf->zero_delay_mutex) == 0 )
        {
          while ( (packet = LL_get_buff(pSinf->Odata_Q, 1)) != NULL )
            MM_put_buff(rmmTRec[inst]->DataBuffPool, packet);
          LL_lock(pSinf->Odata_Q) ;
          LL_signalF(pSinf->Odata_Q) ;
          LL_unlock(pSinf->Odata_Q) ;
          pthread_mutex_unlock(&pSinf->zero_delay_mutex);
        }
        else
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4332),"%d%d",
              "The zero delay mutex variable could not be locked while removing closed stream ({0} {1}).",stream,inst);
          LL_lock(pSinf->Odata_Q) ;
          LL_signalF(pSinf->Odata_Q) ;
          LL_unlock(pSinf->Odata_Q) ;
          continue;
        }

        /* everything is ready - remove stream */
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_TRACE,MSG_KEY(8230),"%s%s%d",
          "remove_closed_topics(): topic {0} stream {1} (instance {2}) successfully closed after heartbeat timeout.",
           pSinf->topic_name,pSinf->stream_id_str,inst);

        rmmTRec[inst]->closed_streams[stream] = pSinf;
        rmmTRec[inst]->all_streams[stream] = NULL;   /* stream can no longer be used by threads */

        pSinf->remove_time = current_time + 10000;

        if ( --rmmTRec[inst]->number_of_streams < 0 )
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3240),"%d%d",
              "The number of streams {0} is not valid for instance {1}.",
               rmmTRec[inst]->number_of_streams, inst);
          rmmTRec[inst]->number_of_streams = 0;
        }
        if ( rmmTRec[inst]->number_of_streams > 0 )
          rmmTRec[inst]->MaxPendingStreamPackets = rmmTRec[inst]->MaxPendingPackets / rmmTRec[inst]->number_of_streams;
        else
          rmmTRec[inst]->MaxPendingStreamPackets = rmmTRec[inst]->MaxPendingPackets;
      }
      continue ;
    } /* if pSinf->closed */

    /* check if close_time has passed and close stream */
    if ( current_time < pSinf->close_time )
      continue;
    else
    {
      int closed = pSinf->closed ;
      if ( pSinf->spm_pending == RMM_TRUE )
      {
        if ( pSinf->SCP_tries++ >= MAX_SCP_TRIES )
        {
          pSinf->closed = RMM_TRUE;
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4333),"%d%d",
              "The stream ({0} {1}) was closed with pending SCP.",stream,inst);
        }
      }
      else
        pSinf->closed = RMM_TRUE;
      LL_lock(pSinf->Odata_Q) ;
      LL_signalF(pSinf->Odata_Q) ;
      LL_unlock(pSinf->Odata_Q) ;
      if ( closed != RMM_TRUE ) /* Update instance Total stats with counters of closed stream */
      {
         rmmTRec[inst]->total_msgs_sent_closed      += pSinf->stats.messages_sent ;
         rmmTRec[inst]->total_bytes_sent_closed     += pSinf->stats.bytes_transmitted ;
         rmmTRec[inst]->total_packets_sent_closed   += pSinf->stats.packets_sent ;
         rmmTRec[inst]->total_repair_packets_closed += pSinf->stats.rdata_packets ;
      }
    }
  } /* stream loop */

  /* remove closed streams in closed_streams array */
  for (stream = 0; stream < rmmTRec[inst]->max_stream_index; stream++)
  {
    if ( (pSinf=rmmTRec[inst]->closed_streams[stream]) == NULL ) continue;

    /* free all stream resources and remove it */
    if ( current_time > pSinf->remove_time )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_TRACE,MSG_KEY(8231),"%s%s%d",
          "remove_closed_topics(): successfully cleaned resources for topic {0} stream {1} (instance {2}).",
           pSinf->topic_name,pSinf->stream_id_str,inst);

      free_stream_memory(pSinf);
      free(pSinf);
      rmmTRec[inst]->closed_streams[stream] = NULL;
    }
  }

  rmmTmutex_unlock() ;

  return RMM_SUCCESS;
}

/****** Task 2 ****************************************************************/
int monitor_all_streams(int inst, int *naks_last_round)
{
  StreamInfoRec_T *pSinf;
  int i;
  int naks_this_round=0, nak_count=0 ;
  unsigned int  packet_rate=0, rate_kbps=0, repair_packets=0, naks_received=0, acks_received=0 ;
  rmm_uint64  msgs_sent=0, bytes_sent=0, packets_sent=0, total_msgs_sent=0, total_bytes_sent=0, total_packets_sent=0, total_repair_packets=0 ;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;


  if ( inst < 0 || inst >= MAX_Tx_INSTANCES || rmmTRec[inst] == NULL || rmmTrunning == 0 ||
    rmmTRec[inst]->initialized == RMM_FALSE  )
    return RMM_FAILURE;

  for (i = 0; i < rmmTRec[inst]->max_stream_index; i++)  // NIR what do we want to do with stats keep in parent of child
  {
    if ( (pSinf = rmmTRec[inst]->all_streams[i]) == NULL)   continue;
    if ( pSinf->closed == RMM_TRUE ) continue;
    pSinf->Monitor_on = RMM_TRUE;
    monitor_stream(pSinf, RMM_FALSE);
    nak_count += pSinf->stats.naks_received;
    packet_rate +=  pSinf->stats.packets_per_sec ;
    rate_kbps +=  pSinf->stats.rate_kbps ;
    packets_sent += pSinf->stats.packets_sent - pSinf->stats.reset_packets_sent ;
    repair_packets += pSinf->stats.rdata_packets - pSinf->stats.reset_rdata_packets ;
    naks_received += pSinf->stats.naks_received  - pSinf->stats.reset_naks_received ;
    acks_received += pSinf->stats.acks_received ;
    msgs_sent += (pSinf->stats.messages_sent - pSinf->stats.reset_messages_sent) ;
    bytes_sent += pSinf->stats.bytes_transmitted - pSinf->stats.reset_bytes_transmitted ;
    total_msgs_sent      += pSinf->stats.messages_sent ;
    total_bytes_sent     += pSinf->stats.bytes_transmitted ;
    total_packets_sent   += pSinf->stats.packets_sent ;
    total_repair_packets += pSinf->stats.rdata_packets ;
    pSinf->Monitor_on = RMM_FALSE;
  }

  naks_this_round = nak_count - *naks_last_round;
  rmmTRec[inst]->packet_rate = packet_rate;
  if ( rmmTRec[inst]->maximal_packet_rate < packet_rate )
       rmmTRec[inst]->maximal_packet_rate = packet_rate ;
  rmmTRec[inst]->rate_kbps = rate_kbps;
  if ( rmmTRec[inst]->maximal_rate_kbps < rate_kbps )
       rmmTRec[inst]->maximal_rate_kbps = rate_kbps ;
  rmmTRec[inst]->packets_sent = packets_sent;
  rmmTRec[inst]->repair_packets = repair_packets;
  rmmTRec[inst]->naks_received = naks_received;
  rmmTRec[inst]->acks_received = acks_received;
  rmmTRec[inst]->msgs_sent = msgs_sent;
  rmmTRec[inst]->bytes_sent = bytes_sent;
  rmmTRec[inst]->total_msgs_sent      = rmmTRec[inst]->total_msgs_sent_closed + total_msgs_sent ;
  rmmTRec[inst]->total_bytes_sent     = rmmTRec[inst]->total_bytes_sent_closed + total_bytes_sent ;
  rmmTRec[inst]->total_packets_sent   = rmmTRec[inst]->total_packets_sent_closed + total_packets_sent;
  rmmTRec[inst]->total_repair_packets = rmmTRec[inst]->total_repair_packets_closed + total_repair_packets ;

  if ( naks_this_round > 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9230),"%d%d%d",
        "monitor_all_streams(): Received {0} NAKs during last {1} milliseconds;  total NAKs {2}.",
      naks_this_round, MONITOR_INTERVAL_MILLI, nak_count);
  }
  *naks_last_round = nak_count;

  return RMM_SUCCESS;
}

/****** Task 3 ****************************************************************/
int print_snapshot(int inst)
{
  StreamInfoRec_T *pSinf;
  int i;
  TCHandle tcHandle = NULL;
  TBHandle tbh = NULL;


  if ( inst < 0 || inst >= MAX_Tx_INSTANCES || rmmTRec[inst] == NULL || rmmTrunning == 0 ||
    rmmTRec[inst]->initialized == RMM_FALSE  )
    return RMM_FAILURE;
  tcHandle = rmmTRec[inst]->tcHandle;
  tbh = llmCreateTraceBuffer(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5010));
  if(tbh == NULL) return RMM_SUCCESS;
  print_transmitter_data(inst,tbh);

  /*llmAddTraceData(tbh,"",", Active streams: [");*/
  if ( rmmTRec[inst]->T_advance.PrintStreamInfo_tx )
  {
    for (i = 0; i < rmmTRec[inst]->max_stream_index; i++)
    {
      if ( (pSinf = rmmTRec[inst]->all_streams[i]) == NULL)   continue;
      if ( pSinf->closed == RMM_TRUE ) continue;
      pSinf->Monitor_on = RMM_TRUE;
      /*if(i == 0){
          llmAddTraceData(tbh,"","[");
      }else{
          llmAddTraceData(tbh,"",", [");
      }*/
      if ( !pSinf->disable_snapshot )
        print_stream_data(pSinf,tbh);
      /*llmAddTraceData(tbh,"","RMM Transmitter SnapShot Report End");*/

      pSinf->Monitor_on = RMM_FALSE;
    }
  }
  llmAddTraceData(tbh,"","\nRUM Transmitter SnapShot Report End\n");
  llmCompositeTraceInvoke(tbh);
  return RMM_SUCCESS;
}

/****** Task 4 ****************************************************************/
int update_batch_time(int inst)
{
  StreamInfoRec_T *pSinf;
  int i;

  if ( inst < 0 || inst >= MAX_Tx_INSTANCES || rmmTRec[inst] == NULL || rmmTrunning == 0 ||
    rmmTRec[inst]->initialized == RMM_FALSE  )
    return RMM_FAILURE;

  for (i = 0; i < rmmTRec[inst]->max_stream_index; i++)
  {
    if ( (pSinf = rmmTRec[inst]->all_streams[i]) == NULL)   continue;
    if ( pSinf->closed == RMM_TRUE || pSinf->lb_parent_info ) continue;
    if ( rmmTRec[inst]->T_advance.BatchingMode != 0 )
    {
      pSinf->Monitor_on = RMM_TRUE;
      monitor_stream(pSinf, RMM_TRUE);
      pSinf->Monitor_on = RMM_FALSE;
    }
  }

  return RMM_SUCCESS;
}

/******* Task 5 ***************************************************************/
int update_dynamic_rate(int inst, int *naks_last_round)
{
  StreamInfoRec_T *pSinf;
  int naks_this_round, nak_count=0;
  int i, rate, rate_changed=0;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;

  if ( inst < 0 || inst >= MAX_Tx_INSTANCES || rmmTRec[inst] == NULL || rmmTrunning == 0 ||
    rmmTRec[inst]->initialized == RMM_FALSE  )
    return RMM_FAILURE;

  if ( rmmTRec[inst]->T_config.LimitTransRate != RMM_DYNAMIC_RATE_LIMIT )
    return RMM_FAILURE;

  for (i = 0; i < rmmTRec[inst]->max_stream_index; i++)
  {
    if ( (pSinf = rmmTRec[inst]->all_streams[i]) == NULL)   continue;
    if ( pSinf->closed == RMM_TRUE || pSinf->lb_parent_info ) continue;
    pSinf->Monitor_on = RMM_TRUE;
    nak_count += pSinf->stats.naks_received;
    pSinf->Monitor_on = RMM_FALSE;
  }

  naks_this_round = nak_count - *naks_last_round;

  if ( naks_this_round > RATE_DECREASE_MARK )
  {
    if ( update_bucket_rate(rmmTRec[inst]->global_token_bucket, DECREASE_PERCENT) == RMM_SUCCESS )
      rate_changed = 1;
  }

  if ( naks_this_round <= RATE_INCREASE_MARK )
  {
    if ( update_bucket_rate(rmmTRec[inst]->global_token_bucket, INCREASE_PERCENT) == RMM_SUCCESS )
      rate_changed = 1;
  }
  if ( rate_changed )
  {
    rate = 8*rmmTRec[inst]->global_token_bucket->tokens_per_milli;
    rmmTRec[inst]->bucket_rate = rate;
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_TRACE,MSG_KEY(8232),"%d",
        "update_dynamic_rate(): Token Bucket rate updated to {0} kbps.",rate);
  }

  *naks_last_round = nak_count;

  return RMM_SUCCESS;
}




/**********************************************************************/
int monitor_stream(StreamInfoRec_T *pSinf, int decrease_batch_time)
{
  int inst = pSinf->inst_id;
  int naks_received;
  char *packet;

  if ( pSinf->monitor_needed.trail == 0 ) pSinf->monitor_needed.trail = 1;
  if ( pSinf->monitor_needed.lead == 0 ) pSinf->monitor_needed.lead = 1;

  if ( decrease_batch_time == RMM_FALSE )
  {
    pSinf->stats.packets_per_sec = 1000*(pSinf->txw_lead - pSinf->stats.last_txw_lead) / MONITOR_INTERVAL_MILLI;
    pSinf->stats.last_txw_lead = pSinf->txw_lead;
    pSinf->stats.rate_kbps = (int)(8*(pSinf->stats.bytes_transmitted - pSinf->stats.last_bytes_transmitted) / MONITOR_INTERVAL_MILLI);
    pSinf->stats.last_bytes_transmitted = pSinf->stats.bytes_transmitted;
  }

  /* dynamic batching time */
  if ( rmmTRec[inst]->T_advance.BatchingMode != 0 )
  {
    naks_received = pSinf->stats.naks_received - pSinf->stats.last_nak_count;

    if ( naks_received > BATCH_TIME_INCREASE_MARK )
    {
      pSinf->batching_time = rmmTRec[inst]->max_batching_time ;
      pSinf->stats.last_nak_count = pSinf->stats.naks_received;
    }

    if ( decrease_batch_time == RMM_TRUE )
    {
      if ( naks_received <= BATCH_TIME_DECREASE_MARK )
      {
        pSinf->batching_time = .9e0*pSinf->batching_time + .1e0*rmmTRec[inst]->min_batching_time ;
      }
      pSinf->stats.last_nak_count = pSinf->stats.naks_received;
    }

    if ( rmmTRec[inst]->T_advance.BatchingMode > 0 ) /* only for tx_feedbac/smooth */
    {
      if ( rmmTRec[inst]->packet_rate > rmmTRec[inst]->max_batching_packets &&
        pSinf->stats.packets_per_sec > rmmTRec[inst]->max_batching_packets/rmmTRec[inst]->number_of_streams )
      {
        pSinf->batching_time = .8e0*pSinf->batching_time + .2e0*rmmTRec[inst]->max_batching_time ;
      }
    }
  }

  /* clean pending queue if connection was closed */
  if ( pSinf->conn_invalid == RMM_TRUE )
  {
    while ( (packet = LL_get_buff(pSinf->Odata_Q, 1)) != NULL )
      MM_put_buff(rmmTRec[inst]->DataBuffPool, packet);
  }

  return RMM_SUCCESS;
}

/**********************************************************************/
int print_stream_data(StreamInfoRec_T *pSinf, TBHandle tbh)
{
  int nNcf, nMsgs, nRdata, nOdata, nHistory; //,n;
  uint64_t msgs, msgs_GM, bytes_GB, bytes, rt_bytes_GB, rt_bytes;
  char timestr[32] ;
  double mpp;

  get_time_string(timestr);

  nNcf        = LL_get_nBuffs(pSinf->Ncf_Q, 0);
  nMsgs       = pSinf->mtl_messages ;
  nRdata      = LL_get_nBuffs(pSinf->Rdata_Q, 0);
  nOdata      = LL_get_nBuffs(pSinf->Odata_Q, 0);
  nHistory    = BB_get_nBuffs(pSinf->History_Q, 0);
  msgs_GM     = (uint64_t)(pSinf->stats.messages_sent/0x40000000) ;
  msgs        = (uint64_t)(pSinf->stats.messages_sent%0x40000000) ;
  bytes_GB    = (uint64_t)(pSinf->stats.bytes_transmitted/0x40000000) ;
  bytes       = (uint64_t)(pSinf->stats.bytes_transmitted%0x40000000) ;
  rt_bytes_GB = (uint64_t)(pSinf->stats.bytes_retransmitted/0x40000000) ;
  rt_bytes    = (uint64_t)(pSinf->stats.bytes_retransmitted%0x40000000) ;
  mpp         = (double)pSinf->stats.packets_sent + nOdata ;

  if ( mpp )
    mpp = (double)(rmm_int64)pSinf->stats.messages_sent/mpp ;

  //n = 0;

  llmAddTraceData(tbh,"%s%s%s","\nStream information for stream {0}({1}) at time {2}: \n",
      pSinf->stream_id_str,pSinf->topic_name,timestr);

  llmAddTraceData(tbh,"%d%d","id: {0}, reliability: {1}",pSinf->rmmT.handle, pSinf->reliability);
  llmAddTraceData(tbh,"%s%d%d%d",", connectionId: {0}, Status: [{1} {2} {3}]\n",
      pSinf->conn_id_str, pSinf->active,pSinf->closed,pSinf->conn_invalid);
  llmAddTraceData(tbh,"%u%u%u","txw_trail: {0}, txw_lead: {1}, spm_sqn: {2} \n",
      pSinf->txw_trail, pSinf->txw_lead, pSinf->spm_seq_num);
  llmAddTraceData(tbh,"%u%u%u%llu%llu","Naks: [Received: {0} Filtered: {1}], Rdata Sent: ({2} {3}GB+{4}) \n",
        pSinf->stats.naks_received, pSinf->stats.naks_filtered, pSinf->stats.rdata_packets,
        UINT_64_TO_LLU(rt_bytes_GB), UINT_64_TO_LLU(rt_bytes));
  llmAddTraceData(tbh,"%llu%llu%llu%llu%e","Messages sent: ({0}GMsgs+{1} {2}GB+{3}), mpp: {4} \n",
        UINT_64_TO_LLU(msgs_GM), UINT_64_TO_LLU(msgs), UINT_64_TO_LLU(bytes_GB), UINT_64_TO_LLU(bytes), mpp);
  llmAddTraceData(tbh,"%d%e%u%u%u","Packet Rate: {0}   Adapt_rate: ({1} {2} {3} {4}) \n",
        pSinf->stats.packets_per_sec, pSinf->batching_time,pSinf->stats.partial_packets,
        pSinf->stats.partial_2fast,pSinf->stats.partial_trylock);

  llmAddTraceData(tbh,"%d%d%d%d%d","nNcf: {0}, nMsgs: {1}, nRdata: {2}, nOdata: {3}, nHistory: {4}",
        nNcf, nMsgs, nRdata, nOdata, nHistory);
  return RMM_SUCCESS;
}

/**********************************************************************/
int print_transmitter_data(int inst, TBHandle tbh)
{
  int nDatapool, nDatapool_use, nDatapool_max;
  int nCtrlpool, nCtrlpool_use, nCtrlpool_max;
  int l ;

  char timestr[32] ;

  get_time_string(timestr);

  nDatapool_max = MM_get_max_size(rmmTRec[inst]->DataBuffPool);
  nDatapool_use = MM_get_buffs_in_use(rmmTRec[inst]->DataBuffPool);
  nDatapool     = MM_get_nBuffs(rmmTRec[inst]->DataBuffPool);

  nCtrlpool_max = MM_get_max_size(rmmTRec[inst]->CtrlBuffPool);
  nCtrlpool_use = MM_get_buffs_in_use(rmmTRec[inst]->CtrlBuffPool);
  nCtrlpool     = MM_get_nBuffs(rmmTRec[inst]->CtrlBuffPool);

  llmAddTraceData(tbh,"%s","RUM Transmitter Snapshot Report ({0}): ",timestr);
  l = rmmTRec[inst]->FireOutStatus.loops ;
  llmAddTraceData(tbh,"%d%d%d%d%d%d","Instance: {0} \nThreads info: FireOut: {1}, Monitor: {2}, Repair: ({3} {4}), SPM: {5}",
      inst,l, rmmTRec[inst]->MonitorStatus.loops, rmmTRec[inst]->RepairStatus.loops+rmmTRec[inst]->TcpStatus.loops,
      rmmTRec[inst]->TcpStatus.tPos*8+rmmTRec[inst]->RepairStatus.tPos, rmmTRec[inst]->SpmStatus.loops);
  llmAddTraceData(tbh,""," \n") ; 
  llmAddTraceData(tbh,"%d%d%d%d","nStreams: {0}, max_stream_index: {1}, MemoryAlert: {2}, nConns: {3} \n",
      rmmTRec[inst]->number_of_streams, rmmTRec[inst]->max_stream_index, rmmTRec[inst]->MemoryAlert,rmmTRec[inst]->rumInfo->nConnections);
  llmAddTraceData(tbh,"%d%d%d","MaxPacketsAllowed: {0}, MinHistoryPackets: {1}, HistorySizePackets: {2} \n",
      rmmTRec[inst]->MaxPacketsAllowed, rmmTRec[inst]->MinHistoryPackets, rmmTRec[inst]->total_history_size_packets) ;
  llmAddTraceData(tbh,"%d%d","MaxPendingPackets: {0}, MaxPendingStreamPackets: {1} \n",
      rmmTRec[inst]->MaxPendingPackets, rmmTRec[inst]->MaxPendingStreamPackets);
  llmAddTraceData(tbh,"%d%d","Packet Rate: {0}pkt/sec, Bucket Rate: {1}kbps \n",
      rmmTRec[inst]->packet_rate, rmmTRec[inst]->bucket_rate);
  llmAddTraceData(tbh,"%d%d%d","Datapool: max size: {0}, used: {1}, idle: {2} \n",
      nDatapool_max, nDatapool_use, nDatapool);
  llmAddTraceData(tbh,"%d%d%d","Ctrlpool: max size: {0}, used: {1}, idle: {2} \n",
      nCtrlpool_max, nCtrlpool_use, nCtrlpool);
  llmAddTraceData(tbh,"%llu%llu%llu%llu%u","Messages sent: {0}, Bytes sent: {1}, Packets sent: {2}, Repair packets {3}, Acks {4} \n",
      rmmTRec[inst]->total_msgs_sent, rmmTRec[inst]->total_bytes_sent, rmmTRec[inst]->total_packets_sent, rmmTRec[inst]->total_repair_packets,rmmTRec[inst]->acks_received);

  return RMM_SUCCESS;
}

/**********************************************************************/

