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


static THREAD_RETURN_TYPE RumRepairThread(void *param) ;
static THREAD_RETURN_TYPE RepairThread_rum(void *param) ;
static int existsStreamId_rum(int inst, char *nakheader, int *counter) ;

static void buildSingleRdata(StreamInfoRec_T *pSinf, uint32_t packet_sqn, double cur_time);
static void buildListRdata(StreamInfoRec_T *pSinf, uint32_t packet_sqn, char *nak_list, char p_type) ;
static int extractOptions(StreamInfoRec_T *pSinf, char *nak_head, char **opts);
static char *processOptions(StreamInfoRec_T *pSinf, char *nak_head, SAS *from_addr, int *block_naks, int is_ib, char **opts);

/**********************************************************************/
/**********************************************************************/

/**********************************************************************/
int extractOptions(StreamInfoRec_T *pSinf, char *nak_head, char **opts)
{
  uint8_t option_size;
  uint8_t opt_type;
  uint8_t opt_end;
  char *optList;
  int inst = pSinf->inst_id ;
  int i, offset, opt_count = 0;
  short nla_afi;
  TCHandle  tcHandle = rmmTRec[inst]->tcHandle;

  memset(opts,0,3*sizeof(char *)) ; 

  if ( !(nak_head[5] & rmmTRec[inst]->T_advance.opt_present) )
    return RMM_SUCCESS ; 

  offset = 20;
  i = 2;
  while ( i-- > 0 )
  {
    memcpy(&nla_afi, nak_head+offset, 2);
    nla_afi = ntohs(nla_afi);
    if ( nla_afi == NLA_AFI )
      offset += 8;
    else if ( nla_afi == NLA_AFI6 )
      offset += 20;
    else
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4204),"%d%s",
            "The NLA_AFI value {0} found in the NAK packet for stream {1} was not valid.",
            nla_afi,pSinf->stream_id_str);
      return RMM_FAILURE ; 
    }
  }

  optList = nak_head + offset;
  do
  {
    opt_type = (optList[0]&(char)~PGM_OPT_END) ;
    opt_end  = (optList[0]&(char) PGM_OPT_END) ;
    if ( (option_size = optList[1]) < 4 )       /* the size of this option   */
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4205),"%d%d%s",
            "The option (type={0} length={1}) found in the NAK packet for stream {2} was not valid.",
            opt_type, option_size,pSinf->stream_id_str);
      return RMM_FAILURE ; 
    }
    if ( opt_type == PGM_OPT_NAK_LIST )
    {
      opts[0] = optList;            /* there is at most one nak list per packet */
    }

    optList += option_size;      /* next option                */
    opt_count++;
  }
  while ( opt_end == 0 && opt_count <= 16 );

  return RMM_SUCCESS ; 

}
/**********************************************************************/
char *processOptions(StreamInfoRec_T *pSinf, char *nak_head, SAS *from_addr, int *block_naks, int is_ib, char **opts)
{
  char *nakList = NULL;

  *block_naks = 0 ;

  nakList = opts[0] ; 

  return nakList;

}

/**********************************************************************/

/**********************************************************************/
void buildSingleRdata(StreamInfoRec_T *pSinf, uint32_t packet_sqn, double cur_time)
{
  char *packet;
  int inst = pSinf->inst_id, mbu=0 ; 
  TCHandle  tcHandle = rmmTRec[inst]->tcHandle;
  pSinf->stats.naks_received++;

  if ( sqn_is_valid(packet_sqn, pSinf->txw_trail, pSinf->txw_lead) == RMM_FALSE )   /* if the sequence is out of transmittion window  */
  {
    if ( (pSinf->stats.naks_filtered % 50000) == 0 ){
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6202),"%u%u%u",
            "buildSingleRdata(): Invalid NAK received sqn {0},  trail ={1}  lead = {2}.",
            packet_sqn, pSinf->txw_trail, pSinf->txw_lead);
    }  else {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9202),"%u%u%u",
            "buildSingleRdata(): Invalid NAK received sqn {0},  trail ={1}  lead = {2}.",
            packet_sqn, pSinf->txw_trail, pSinf->txw_lead);
    }
    pSinf->stats.naks_filtered++;
    return;
  }
  pthread_mutex_lock(&pSinf->rdata_mutex);
  if ( (packet = BB_see_buff_a(pSinf->History_Q, packet_sqn, 1)) == NULL )
  {
    if ( sqn_is_valid(packet_sqn, pSinf->txw_trail, pSinf->txw_lead) == RMM_TRUE  )
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3210),"%u%u%u%s",
            "The RDATA packet {0} could not be obtained from the history queue, transmit window trail: {1} transmit window lead: {2}, for stream {3}.",
            packet_sqn, pSinf->txw_trail, pSinf->txw_lead,pSinf->stream_id_str);
    }
    pthread_mutex_unlock(&pSinf->rdata_mutex);
    pSinf->stats.naks_filtered++;
    return;
  }

  if ( pSinf->direct_send )
  {
    pgm_seq sqn ; 
    memcpy(&sqn, (packet+pSinf->sqn_offset),4);
    sqn = ntohl(sqn) ; 
    if ( sqn != packet_sqn )
    {
      pgm_seq try_sqn=0;
      int i,j,m,n;
      n = BB_get_nBuffs(pSinf->History_Q, 1) ; 
      for ( i=1 ; i<n ; i++ )
      {
        for ( j=-1, m=0 ; j<2 ; j += 2 )
        {
          try_sqn = packet_sqn+(j*i) ; 
          if ( (packet = BB_see_buff_a(pSinf->History_Q, try_sqn, 1)) == NULL )
          {
            m++ ; 
            continue ; 
          }
          memcpy(&sqn, (packet+pSinf->sqn_offset),4);
          sqn = ntohl(sqn) ; 
          if ( sqn == packet_sqn ) break ; 
        }
        if ( sqn == packet_sqn ) break ; 
        if ( m > 1 ) break ; 
      }
      if ( sqn == packet_sqn ) 
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(1451),"%u%u%s",
            "The RDATA packet {0} was found in position {1} of the history queue for direct send stream {2}.",
            packet_sqn, try_sqn, pSinf->stream_id_str);
      }
      else
      {
        if ( sqn_is_valid(packet_sqn, pSinf->txw_trail, pSinf->txw_lead) == RMM_TRUE  )
        {
            llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3210),"%u%u%u%s",
                "The RDATA packet {0} could not be obtained from the history queue, transmit window trail: {1} transmit window lead: {2}, for stream {3}.",
                packet_sqn, pSinf->txw_trail, pSinf->txw_lead,pSinf->stream_id_str);
        }
        pthread_mutex_unlock(&pSinf->rdata_mutex);
        pSinf->stats.naks_filtered++;
        return;
      }
    }
  }


    packet[INT_SIZE+4]= (char)PGM_TYPE_RDATA;  /* type = RDATA  */

  if ( LL_put_buff(pSinf->Rdata_Q, packet, 1) == NULL )
  {
    pthread_mutex_unlock(&pSinf->rdata_mutex);
    rmm_signal_fireout(inst, 1, pSinf);
    pSinf->stats.naks_filtered++;
    if ( !mbu )
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9203),"%u",
          "buildSingleRdata(): (repair_is_pending) found matching sqn {0} in RDATA queue.",
          packet_sqn);
    return ;
  }

  pSinf->stats.rdata_packets++;
  pthread_mutex_unlock(&pSinf->rdata_mutex);
  rmm_signal_fireout(inst, 1, pSinf);
}

/**********************************************************************/
void buildListRdata(StreamInfoRec_T *pSinf, uint32_t packet_sqn, char *nak_list, char p_type)
{
  int i;
  uint8_t opt_len ;
  uint32_t sqn;
  double cur_time=0e0 ;


  if ( p_type == PGM_TYPE_NAK )
  {
    cur_time = rmmTRec[pSinf->inst_id]->rdata_supp_sec>0 ? sysTime() : 0e0 ; 
    buildSingleRdata(pSinf, packet_sqn, cur_time); /* send the first sqn */
  }

  if (!nak_list ) return ; 
  opt_len = nak_list[1];
  if ( p_type != PGM_TYPE_NAK )
    cur_time = rmmTRec[pSinf->inst_id]->rdata_supp_sec>0 ? sysTime() : 0e0 ; 
  for (i = 4; i < opt_len; i += 4)
  {
    memcpy(&sqn, (nak_list+i),4);
    buildSingleRdata(pSinf, ntohl(sqn), cur_time);
  }
}

/**********************************************************************/

int existsStreamId_rum(int inst, char *nakheader, int *counter)
{
  int i;
  uint32_t gsi_high;
  uint16_t gsi_low, idx;
  StreamInfoRec_T *pSinf;

  memcpy(&gsi_low, (nakheader+12), 2);     /* rmmTRec port is in nbo */
  if (gsi_low != rmmTRec[inst]->port) return RMM_FALSE;

  memcpy(&gsi_high, (nakheader+8), 4);
  if (gsi_high != rmmTRec[inst]->gsi_high) return RMM_FALSE;

  /* counter is not used for src_port! must go over all streams */
  memcpy(&idx, (nakheader+2), 2);

  for (i=0; i<rmmTRec[inst]->max_stream_index; i++)
  {
    if ( (pSinf=rmmTRec[inst]->all_streams[i]) == NULL ) continue;
    if ( pSinf->src_port == idx )
    {
      *counter = pSinf->rmmT.handle;
      return RMM_TRUE;
    }
  }

  return RMM_FALSE;
}

/**********************************************************************/

THREAD_RETURN_TYPE RepairThread_rum(void *param)
{
  int inst, block_naks ;
  pthread_mutex_t visiMutex;
  SAS from_addr;
  int rc;

  rumInstanceRec *rumInfo ;
  StreamInfoRec_T *pSinf;

  int bread, counter;
  uint8_t nak_list_len;
  uint32_t nak_sqn;

  int inak_list[MAX_NAK_LIST_SIZE/4 +1];
  char *nakhead, p_type;
  char *bptr, *nak_list;
  char *recvbuf;
  const char *myName = "RepairThread" ; 
  TCHandle tcHandle = NULL;
  nak_list = (char *)inak_list;

  inst = *(int *)param ;
  rumInfo = rmmTRec[inst]->rumInfo;
  tcHandle = rumInfo->tcHandles[2];
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5069),"%s%llu%d",
    "The {0} thread is running. Thread id: {1} ({2}).",
        myName,LLU_VALUE(my_thread_id()),
#ifdef OS_Linuxx
        (int)gettid());
#else
        (int)my_thread_id());
#endif


  if ( (rc=pthread_mutex_init(&visiMutex, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3269),"%s%d",
        "The RUM transmitter failed to initialize the {0} pthread mutex variable. The error code is {1}.","visiMutex",rc);

    rmmTRec[inst]->RepairStatus.status = THREAD_EXIT;
    rmm_thread_exit();
  }

  pthread_mutex_lock(&rmmTRec[inst]->Gprps_mutex);
  rmmTRec[inst]->RepairStatus.status = THREAD_RUNNING; /* report that the repair thread has been created */
  pthread_mutex_unlock(&rmmTRec[inst]->Gprps_mutex);

  /* MAIN thread LOOP */
  while( 1 )
  {
    /* check for stop condition */
    pthread_mutex_lock(&visiMutex);
    if ( rmmTRec[inst]->RepairStatus.status == THREAD_KILL )
    {
      pthread_mutex_unlock(&visiMutex);
      break;
    }
    pthread_mutex_unlock(&visiMutex);

    rmmTRec[inst]->RepairStatus.loops++;

    /*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

    BB_lock(rumInfo->recvNacksQ) ;
    while ( (recvbuf=BB_get_buff(rumInfo->recvNacksQ,0)) == NULL )
    {
      BB_waitE(rumInfo->recvNacksQ) ;
      if ( rmmTRec[inst]->RepairStatus.status == THREAD_KILL )
        break ;
    }
    BB_signalF(rumInfo->recvNacksQ) ;
    BB_unlock(rumInfo->recvNacksQ) ;

    if ( recvbuf == NULL )
      continue ;

    do
    {
      bptr = recvbuf ;
      NgetInt(bptr,bread) ;

      nakhead = bptr ;

      p_type = nakhead[4] & 0x0f ;
      if ( p_type != PGM_TYPE_NAK )
      {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6225),"%d%d",
          "RepairThread received packet with bad packet type (not NAK packet). type received {0}, bytes read {1}.",nakhead[4],bread);
        continue; /* packet is not a NAK */
      }

      if ( existsStreamId_rum(inst, nakhead, &counter) != RMM_TRUE ) /* wrong stream id */
      {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6204),"",
              "RepairThread received NAK packet for non-existing stream.");

        continue; /* discard packet */
      }

      if ( (pSinf = rmmTRec[inst]->all_streams[counter]) == NULL ) continue;
      if ( pSinf->closed == RMM_TRUE || !pSinf->reliable ) continue;
      if ( pSinf->conn_invalid == RMM_TRUE || pSinf->keepHistory == RMM_FALSE ) continue;
      pSinf->Repair_on = RMM_TRUE;
      /* check for NAK list and build NCF packet */
      nak_list = NULL ;
      if ( nakhead[5] & rmmTRec[inst]->T_advance.opt_present )  /* are there options present? */
      {
        char *opts[3] ; 
        if ( extractOptions(pSinf, (char *)nakhead, opts) == RMM_SUCCESS )
          nak_list = processOptions(pSinf, (char *)nakhead, &from_addr, &block_naks,0, opts);
      }

      memcpy(&nak_sqn, (nakhead+16), INT_SIZE);
      nak_sqn = ntohl(nak_sqn);
      buildListRdata(pSinf, nak_sqn, nak_list, p_type);

      rmm_signal_fireout(inst, 1, pSinf) ; 

      /* report NAK */
      if ( rmmTRec[inst]->T_config.LogLevel >= RUM_LOGLEV_XTRACE )
      {
        if ( nak_list )
        {
          nak_list_len = (uint8_t)nak_list[1];
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9204),"%s%d%d",
              "RepairThread(): received NAK List for stream {0} for {1} sequence numbers: first sqn is {2}.",
              pSinf->stream_id_str,nak_list_len/4, nak_sqn);

        }
        else{
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9205),"%s%d",
              "RepairThread(): received single NAK for stream {0} for sequence number {1}.",
              pSinf->stream_id_str,nak_sqn);

        }

      }

      pSinf->Repair_on = RMM_FALSE;
    } while (0);

    MM_put_buff(rumInfo->nackBuffsQ,recvbuf);

    /*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

  }/* while (1) */
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5067),"%s%llu","The {0}({1}) thread was stopped.",myName,
      LLU_VALUE(my_thread_id()));

  pthread_mutex_destroy(&visiMutex);
  pthread_mutex_lock(&rmmTRec[inst]->Gprps_mutex);
  rmmTRec[inst]->RepairStatus.status = THREAD_EXIT;
  pthread_mutex_unlock(&rmmTRec[inst]->Gprps_mutex);
  rmm_thread_exit();

}


/**********************************************************************/
/**********************************************************************/

THREAD_RETURN_TYPE RumRepairThread(void *param)
{

  /* note that the thread exits, with rmm_thread_exit(), from the functions it call  */
 #if USE_PRCTL
  {
    char tn[16] ; 
    rmm_strlcpy(tn,__FUNCTION__,16);
    prctl( PR_SET_NAME,(uintptr_t)tn);
  }
 #endif
  return RepairThread_rum(param) ;

}



/**********************************************************************/
/**********************************************************************/
