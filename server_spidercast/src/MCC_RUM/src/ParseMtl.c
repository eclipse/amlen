/*
 * Copyright (c) 2008-2021 Contributors to the Eclipse Foundation
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

 #define LAST_ARG ,rumPacket *rPack

static int parseMsgs_om_ntf_nmp(rTopicInfoRec *ptp, rStreamInfoRec *pst, int nMsgs, char *bptr, char *eop,rmmMessageID_t msg_cur, char *packet  LAST_ARG);

static int parseMTL_nfo_bt_om(rStreamInfoRec *pst, char *packet);


static int process_frag_msg(rTopicInfoRec *ptp, rStreamInfoRec *pst, char *msg, int msgLen, int msgID,int msgSN, int totLen,int offset, rmmHeader *phrm) ;


static void setParseMTL(rTopicInfoRec *ptp)
{
  ptp->parseMTL = parseMTL_nfo_bt_om ; 
}



/*
 * Set the message parsing alorithm for a stream.
 */
static void setParseMsgsStream(rTopicInfoRec *ptp, rStreamInfoRec *pst)
{
  pst->parseMsgs = parseMsgs_om_ntf_nmp ;
}


/*
 * Set the parse message algorithm for all streams in a topic
 */

 #define SET_RESERVED(x) {(x).rum_length = RUMCAPI_VERSION; (x).rum_reserved = RUM_INIT_SIG;}


static int put_buff_in_readyQ(rmmReceiverRec *rInfo, rTopicInfoRec *ptp, rmmPacket *rPack, rmmHeader *phrm, void *om_user, int is_mslj)
{
  int od ; 

  LL_lock(ptp->packQ) ;

  if ( ptp->need_trim )
  {
    if ( ptp->need_trim > 100 )
    {
      ptp->need_trim = 1 ;
      trim_packetQ(rInfo,ptp,0) ;
    }
    else
      ptp->need_trim++ ;
    rPack->rmm_expiration = rInfo->CurrentTime + ptp->maxTimeInQ ;
  }
  od = (LL_get_nBuffs(ptp->packQ,0) == 0) ;
  while ( LL_put_buff(ptp->packQ,rPack,0) == NULL )
  {
    rInfo->FullPackQ++ ;
    if ( rInfo->GlobalSync.goDN || ptp->closed )
    {
      LL_unlock(ptp->packQ) ;
      rPack->rmm_flags_reserved |= 2 ; 
      return_packet(rInfo, rPack) ;
      return -1 ; 
    }
    if ( rInfo->CurrentTime > rInfo->NextSlowAlert )
    {
      llmSimpleTraceInvoke(rInfo->tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4600),"%s",
          "The application could be too slow in consuming messages for topic {0}.",
          ptp->topicName);
      rInfo->NextSlowAlert = rInfo->CurrentTime + 1000 ;
    }
    LL_waitF(ptp->packQ) ;
  }
  LL_signalE(ptp->packQ) ;
  LL_unlock(ptp->packQ) ;

  if ( od || ptp->odEach )
    ptp->on_data(om_user) ;
  rInfo->EmptyPackQ += od ;

  return 0 ; 
}

/*
 * parse for the message translation layer.
 */

/*------------------------------------------*/

static int parseMTL_nfo_bt_om(rStreamInfoRec *pst, char *packet)
{
  char *eop ;
  char * bptr , frag;
  int nmi ;
  uint32_t frag_msgSN , frag_msglen , frag_msgoff ;
  uint32_t nMsgs , msgLen ;
  rmm_uint64 prevtotmsgs=0 ;
  rmmHeader *phrm ;
  rmmMessageID_t msn_cur=0 ;
  rumPacket *rPack=NULL;
  rmmReceiverRec *rInfo ;
  rTopicInfoRec *ptp ;
  rInfo = rInstances[pst->instance_id] ;
  ptp   = rInfo->rTopics[pst->topic_id] ;

  phrm = (rmmHeader *)packet ;
  bptr = packet + phrm->PackOff ;
  eop  = bptr + phrm->PackLen ; /* EndOfPacket !! */

  NgetShort(bptr,nMsgs) ;
  NgetChar(bptr,frag) ;
  if (!frag )
  {
    pst->TotMsgsOut  += nMsgs ;
    ptp->TotMsgsOut  += nMsgs ;
    pst->TotBytesOut += (eop-bptr)-nMsgs*2 ;

    msn_cur = ptp->msn_next ;
    ptp->msn_next += nMsgs ;

    nmi = pst->parseMsgs(ptp,pst,nMsgs,bptr,eop,msn_cur,packet,rPack) ;
    pst->TotMsgsDel  += nmi ;
    ptp->TotMsgsDel  += nmi ;

  }
  else
  {
    if ( nMsgs != 1 )
    {
        llmSimpleTraceInvoke(rInfo->tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3600),"%d",
            "The number of messages ({0}) in the received packet is not valid.", nMsgs);
      return 1;
    }

    prevtotmsgs = pst->TotMsgsOut ;

    NgetInt(bptr,frag_msgSN) ;
    NgetInt(bptr,frag_msglen) ;
    NgetInt(bptr,frag_msgoff) ;
    NgetShort(bptr,msgLen) ;
    process_frag_msg(ptp, pst, bptr, msgLen, frag_msgSN,frag_msgSN,frag_msglen, frag_msgoff,phrm) ;

    if ( prevtotmsgs != pst->TotMsgsOut )
    {
      ptp->msn_next++ ;
    }
  }
  return 1;
}
/*------------------------------------------*/

static int rmm_filter_msg(rTopicInfoRec *ptp, rmmRxMessage *msi, int ezPass, int isTf, int isPr)
{
  int nmi = 1 ; 
  int msgLen = msi->msg_len ;
  char *bptr = msi->msg_buf;
  char *hdrPtr = msi->msg_buf;

  if ( nmi )
  {
    msi->msg_buf   = bptr ;
    msi->msg_len   = msgLen ;
    msi->hdr_len   = (uint32_t)(bptr-hdrPtr);
  }

  return nmi ; 
}
/*------------------------------------------*/

/*
 * Process a fragmented mesage.
 * A fragmented message is one which is too large to fit into a single packet.
 */
int process_frag_msg(rTopicInfoRec *ptp, rStreamInfoRec *pst, char *msg, int msgLen, int msgID,
                     int msgSN, int totLen,int offset, rmmHeader *phrm0)
{
  int n , nmi=0;
  FragMsgInfoRec *frg  ;
  int iError;
  rmmPacket *rPack=NULL ;
  rmmHeader *phrm ;
  rmmRxMessage *msi;
  rmmReceiverRec *rInfo=NULL ;

  rInfo = rInstances[pst->instance_id] ;
  msi = NULL ;
  frg = NULL ;
  n = LL_get_nBuffs(pst->fragQ,0) ;
  while ( n-- )
  {
    if ( (frg = LL_get_buff(pst->fragQ,0)) == NULL )
    {
        llmSimpleTraceInvoke(rInfo->tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3601),"%s",
            "The buffer could not be obtained from the queue {0}.", "fragQ");
      continue ;
    }
    if ( frg->msgID == msgID )
      break ;
    else
    {
       /* throw too old fragments */
      if ( ptp->ordered || XgtY(pst->rxw_trail,frg->msgSN+frg->totLen/(frg->curLen/frg->curNum)+10) )
      {
        llmSimpleTraceInvoke(rInfo->tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4090),"%s%d%d%d%d%d",
          "Throwing incomplete fragmented message for stream {0}. Additional information: {1} {2} {3} {4} {5}",
          pst->sid_str,frg->msgSN,frg->msgID,frg->totLen,frg->curLen,frg->curNum);
        free(frg->buffer) ;
        free(frg) ;
      }
      else
        LL_put_buff(pst->fragQ,frg,0) ;
    }
  }
  if ( n < 0 )
  {
    if ( (frg = (FragMsgInfoRec *)malloc(sizeof(FragMsgInfoRec))) == NULL )
    {
      LOG_MEM_ERR(rInfo->tcHandle,"process_frag_msg",(sizeof(FragMsgInfoRec)));
      return RMM_FAILURE ;
    }
    else
    {
      memset(frg,0,sizeof(FragMsgInfoRec)) ; 
      frg->msgSN  = msgSN ;
      frg->msgID  = msgID ;
      frg->totLen = totLen ;
      if ( (frg->buffer = malloc(sizeof(rmmHeader)+totLen)) == NULL )
      {
          LOG_MEM_ERR(rInfo->tcHandle,"process_frag_msg",sizeof(rmmHeader)+totLen);
        free(frg) ;
        frg = NULL ;
        return RMM_FAILURE ;
      }
      frg->packet = frg->buffer + sizeof(rmmHeader) ; 
    }
    memset(frg->buffer,0,sizeof(rmmHeader)) ; 
  }
  if ( frg )
  {
    frg->eTime = rInfo->CurrentTime + 60000 ; 
    if ( frg->packet )
    {
      memcpy(frg->packet+offset,msg,msgLen) ;
    }
    frg->curLen += msgLen ;
    frg->curNum++ ;
    if ( frg->curLen == frg->totLen )
    {
      if ( frg->packet )
      {
        pst->TotMsgsOut  += 1  ;
        ptp->TotMsgsOut  += 1  ;
        pst->TotBytesOut += frg->totLen ;

        phrm = (rmmHeader *)frg->buffer ; 
        if ( ptp->on_data )
        {
          while ( (rPack = MM_get_buff(rInfo->packStrucQ,-1,&iError)) == NULL &&
                  (rPack = MM_get_buff(rInfo->packStrucQ, 8,&iError)) == NULL )
          {
            if ( rInfo->GlobalSync.goDN )
            {
              free(frg->buffer) ;
              free(frg) ;
              return RMM_FAILURE ;
            }
          }
          rPack->rmm_pst_reserved = pst ;
          rPack->rmm_buff_reserved = frg->buffer ;
          rPack->rmm_flags_reserved = 0 ;
          rPack->stream_info = &pst->si ;
          rPack->rmm_instance = rInfo->instance ;
          rPack->first_packet_sqn = frg->msgSN ;
          rPack->last_packet_sqn = frg->msgSN + frg->curNum - 1 ;

          msi =&rPack->msgs_info[0] ;
        }
        else
        {
          if ( ptp->on_message )
          {
            msi = &pst->msg ;
          }
          else
          {
            rPack = pst->pck ;
            rPack->rmm_buff_reserved = frg->buffer ;
            rPack->rmm_flags_reserved = 0 ;
            rPack->rmm_instance = rInfo->instance ;
            rPack->first_packet_sqn = frg->msgSN ;
            rPack->last_packet_sqn = frg->msgSN + frg->curNum - 1 ;
  
            msi =&rPack->msgs_info[0] ;
          }
        }
        nmi = 1 ;
        msi->stream_info = &pst->si ;

        msi->msg_buf = frg->packet ;
        msi->msg_len = frg->totLen ;

        nmi = rmm_filter_msg(ptp, msi, 0          , pst->isTf, pst->isPr) ; 

        if ( nmi )
        {
          msi->msg_sqn   = ptp->msn_next ;
          pst->TotMsgsDel += nmi  ;
          ptp->TotMsgsDel += nmi  ;
        }
        if ( ptp->on_data )
        {
          rPack->num_msgs = nmi ; /* BEAM suppression: operating on NULL */
      
          if ( nmi )
          {
            msi->stream_info = &pst->si ;

            if ( put_buff_in_readyQ(rInfo, ptp, rPack, phrm, pst->om_user,0) < 0 )
              free(frg) ;
          }
          else
          {
            return_packet(rInfo, rPack) ;
          }
        }
        else
        {
          if ( ptp->on_message )
          {
            if ( nmi )
            {
              ptp->on_message(msi,pst->om_user) ;
            }
          }
          else
          {
            if ( nmi )
            {
              rPack->num_msgs  = nmi ;
              ptp->on_packet(rPack,pst->om_user) ;
            }
          }
          free(frg->buffer) ;
        }
      }
      free(frg) ;
    }
    else
    if ( LL_put_buff(pst->fragQ,frg,0) == NULL )
    {
        llmSimpleTraceInvoke(rInfo->tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3602),"%s",
            "The buffer could not be put into the queue {0}.", "fragQ");
      free(frg->buffer) ;
      free(frg) ;
    }
  }
  else
    return RMM_FAILURE ;
  return RMM_SUCCESS ; /* BEAM suppression: memory leak */
}


/*++++++++++++++++++++++++++++*/
/*++++++++++++++++++++++++++++*/



/*
 * Parse for on message with no turboflow and no properties
 */
int parseMsgs_om_ntf_nmp(rTopicInfoRec *ptp, rStreamInfoRec *pst, int nMsgs, char *bptr, char *eop,rmmMessageID_t msn_cur, char *packet  LAST_ARG)
{
  int nmi ;
  uint32_t msgLen ;
  rmmRxMessage *msi;

  msi = &pst->msg ; 
  nmi = nMsgs ;
  msn_cur--;
  while ( nMsgs-- )
  {
    msn_cur++ ;
    NgetShort(bptr,msgLen) ;
    msi->msg_buf = bptr ;
    msi->msg_len = msgLen ;
    msi->hdr_len = 0;
    bptr += msgLen ;
    msi->msg_sqn = msn_cur ;
    ptp->on_message(msi,pst->om_user) ;
  }
  return nmi ;
}




