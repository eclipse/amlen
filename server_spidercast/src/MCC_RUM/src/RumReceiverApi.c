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

#define RUM_R
#define MAKE_HIST 0


#include "rmmSystem.h"
#include "rumCapi.h"
#include "rumPrivate.h"
#include "rmmConstants.h"
#include "rmmWinPthreads.h"
#include "rmmCutils.h"
#include "SockMgm.h"
#include "BuffBoxMgm.h"
#include "SortedQmgm.h"
#include "MemMan.h"
#include "rumRprivate.h"
#include "rmmPacket.h"


/*  Internal Data  */

static rmm_mutex_t  rmmRmutex = PTHREAD_MUTEX_INITIALIZER ;

static int rNumInstances=0 ;
static rmmReceiverRec *rInstances[MAX_Rx_INSTANCES] ;
static int *nla_afi_len=NULL , *nla_afi_af , *sa_ip_len , *sa_ip_off ; 


/*  Internal Routines  */

/*------------------------------------------*/
static int  rumR_Init(rumInstanceRec *gInfo, int *error_code) ; 
static int  rumR_ChangeLogLevel(rumInstanceRec *gInfo, int new_log_level, int *error_code) ; 

static int  rumR_CreateQueue(rumInstanceRec *gInfo, const rumRxQueueParameters *queue_params, rumQueueR *queue_r, int *error_code) ; 

static int rumR_CloseQueue(rumInstanceRec *gInfo, int q_handle, int *error_code) ; 
static int rumR_ClearRejectedStreams(rumInstanceRec *gInfo, int *error_code);
static int rumR_RemoveStream(rumInstanceRec *gInfo, rumStreamID_t stream_id, int *error_code);
static int rumR_Stop(rumInstanceRec *gInfo, int timeout_milli, int *error_code) ; 
/*------------------------------------------*/
static int _rumR_Init(int instance, rumInstanceRec *gInfo, TCHandle tcHandle, int *error_code) ;
static int _rumR_CloseTopic(rmmReceiverRec *rInfo, int q_handle, int *error_code) ; 
/*------------------------------------------*/
static int readPacket(rmmReceiverRec *rInfo, char *buff, uint32_t bufflen, int myid);
static int doSelect(rmmReceiverRec *rInfo);
static int extractPacket(rmmReceiverRec *rInfo, ConnInfoRec *cInfo, char *buff, uint32_t bufflen);
static THREAD_RETURN_TYPE PacketProcessor_tcp(void *arg) ;
static THREAD_RETURN_TYPE NAKGenerator(void *arg) ;
static THREAD_RETURN_TYPE MessageAnnouncer(void *arg) ;
static THREAD_RETURN_TYPE EventAnnouncer(void *arg) ; 
static THREAD_RETURN_TYPE ConnectionHandler(void *arg) ; 
static int _MessageAnnouncer(rmmReceiverRec *rInfo) ;
/*------------------------------------------*/
static int is_conn_valid(rumInstanceRec *gInfo,rmmReceiverRec *rInfo,ConnInfoRec *cInfo,int invalid) ; 
static void delete_conn_streams(rumInstanceRec *gInfo,rmmReceiverRec *rInfo,ConnInfoRec *cInfo) ; 
static void delete_closed_connection(rumInstanceRec *gInfo,ConnInfoRec *cInfo) ; 
static void add_ready_connection(rumInstanceRec *gInfo,ConnInfoRec *cInfo) ; 
static void send_conn_close_event(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static void delete_all_connections(rumInstanceRec *gInfo) ; 
/*------------------------------------------*/
static int  add_scp(rmmReceiverRec *rInfo, scpInfoRec *sInfo) ; 
static int  del_scp(rmmReceiverRec *rInfo, rumConnectionID_t cid, rmmStreamID_t sid) ; 
static int find_scp(rmmReceiverRec *rInfo, rmmStreamID_t sid, scpInfoRec *sInfo);
static void process_tx_stream_report(rmmReceiverRec *rInfo, rumConnectionID_t cid, uint8_t *packet) ; 
/*------------------------------------------*/
static int  check_glb(rStreamInfoRec *pst, rTopicInfoRec  *ptp, pgm_seq psn_glb, rmmMessageID_t msn_glb) ;
static void process_glbs(rStreamInfoRec *pst, rTopicInfoRec  *ptp) ;
static void process_glb(rStreamInfoRec *pst, rTopicInfoRec  *ptp, pgm_seq psn_glb, rmmMessageID_t msn_glb) ;
/*------------------------------------------*/
static rStreamInfoRec *check_new_stream(rmmReceiverRec *rInfo, rmmStreamID_t sid, uint8_t *buff, uint8_t *packet, rumHeader *phrm, pgmHeaderCommon *phc, 
                          uint8_t pacType, uint8_t *pacTypes, WhyBadRec *wb, int iBad_pactype, int iBad_nlaafi, int iBad_pgmopt);
static int check_first_packet(rmmReceiverRec *rInfo, rStreamInfoRec *pst, rTopicInfoRec  *ptp, uint8_t *packet,uint8_t pacType);
static rStreamInfoRec *create_stream(rmmStreamID_t sid, ipFlat *sip, rTopicInfoRec *ptp, int topicLen, char *topicName, uint16_t src_port,
                          uint16_t dst_port, uint32_t gsi_high, uint16_t gsi_low,pgm_seq rxw_trail,pgm_seq rxw_lead) ;
static void delete_stream(rStreamInfoRec *pst) ;
static void kill_stream(rStreamInfoRec *pst, char* reason) ;
static void destroy_stream(rStreamInfoRec *pst) ; 
static void reject_stream(rmmReceiverRec *rInfo, rmmStreamID_t sid, rumConnectionID_t cid, int type) ;
static void clear_rejected_streams(rmmReceiverRec *rInfo, rumConnectionID_t cid, int type) ;
static int             find_rejected_stream(rmmReceiverRec *rInfo, rmmStreamID_t sid) ;
static rTopicInfoRec  *find_topic(rmmReceiverRec *rInfo, char *topicName, int topicLen) ;
static rTopicInfoRec  *find_streamset(rmmReceiverRec *rInfo, rumStreamParameters *psp) ;
static void *pp_get_buffer(rmmReceiverRec *rInfo,WhyBadRec *wb, int iBad_recvQ) ;
static int (*buildNsendNAK)(rStreamInfoRec *pst, int n, pgm_seq *sqn_list, int sorted) ;
static int buildNsendNAK_pgm(rStreamInfoRec *pst, int n, pgm_seq *sqn_list, int sorted) ;
static void SnapShot(rmmReceiverRec *rInfo) ;
/* static void MemoryAlertHandler(int alert_level) ; */
static int check_nak_element(NackInfoRec *pnk, rStreamInfoRec *pst) ;
static void process_data_packet(rmmReceiverRec *rInfo, rStreamInfoRec *pst, const char *myName,
               uint32_t rxw_trail, uint32_t rxw_lead,uint32_t PackLen,rmmMessageID_t msg_num, uint8_t *packet,
               WhyBadRec *wb, int iBad_lout, int iBad_rout,int iBad_mem0, int iBad_mem1, int iBad_mem2, int iBad_mem3, int iBad_mem4) ;
static rmm_uint64 setNTC(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm) ; 
static rmm_uint64 MemMaint(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm) ; 
static rmm_uint64 callCHBTO(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm) ; 
static rmm_uint64 calcTP(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm) ; 
static rmm_uint64 chkSLB(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm) ;
static rmm_uint64 calcBW(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm) ; 
static rmm_uint64 callSS(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm) ; 
static rmm_uint64 ConnStreamReport(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm) ; 
static rmm_uint64 chkHBTO(rmmReceiverRec *rInfo) ; 
static int cancel_recv_thread(rmmReceiverRec *rInfo, const char *name, pthread_t id) ; 
static int detach_recv_thread(rmmReceiverRec *rInfo, const char *name, pthread_t id) ; 
static int stop_recv_threads(rmmReceiverRec *rInfo) ; 
static int start_recv_threads(rmmReceiverRec *rInfo) ; 
static int free_recv_data(rmmReceiverRec *rInfo) ; 
static int init_recv_data(rmmReceiverRec *rInfo) ; 
static void event_delivered(rmmReceiverRec *rInfo, EventInfo *evi, int rc, int eaCall) ; 
static int ea_free_ptr(void *ptr) ; 
static void wakeMA(rmmReceiverRec *rInfo, rStreamInfoRec *pst) ;
static void put_pst_inQ(rmmReceiverRec *rInfo,rStreamInfoRec *pst) ;
static int  rmmR_chk_instance(int instance, TCHandle *tcHandle, int *ec) ;
static int  rmmR_chk_topic(rmmReceiverRec *rInfo, int topic, const char *caller, int *ec) ;
/*---------------*/
static void setParseMTL(rTopicInfoRec *ptp);
static void setParseMsgsStream(rTopicInfoRec *ptp, rStreamInfoRec *pst);
static int update_conn_list(ConnInfoRec **seed, ConnInfoRec *cInfo, int *nConns, int add);
static int update_conn_info(rumInstanceRec *gInfo, ConnInfoRec *cInfo, int add);
static void rumR_PerConnInQup(rmmReceiverRec *rInfo, rStreamInfoRec *pst);
static void rumR_PerConnInQdn(rmmReceiverRec *rInfo, rStreamInfoRec *pst);
/**-------------------------------------**/

/*---------------*/

/**  Included 'c' files  **/

#include "rmmCutils.c"
#include "SockMgm.c"
#include "rmmRutils.c"
#include "MemMan.c"
#include "BuffBoxMgm.c"
#include "SortedQmgm.c"
#include "ConnectionHandler.c"
#include "ParseMtl.c"

/**-------------------------------------**/

int update_conn_list(ConnInfoRec **seed, ConnInfoRec *cInfo, int *nConns, int add)
{
  if ( add )
  {
    cInfo->next = *seed ; 
    *seed = cInfo ; 
    cInfo->ind = *nConns ; 
    *nConns = *nConns + 1 ; 
    return 0 ; 
  }
  else
  {
    ConnInfoRec *pc = *seed;
    if ( pc == cInfo )
    {
      *seed = cInfo->next ; 
      *nConns = *nConns - 1 ; 
      return 0 ; 
    }
    for ( ; pc ; pc = pc->next )
    {
      pc->ind-- ; 
      if ( pc->next == cInfo )
      {
        pc->next = cInfo->next ; 
        *nConns = *nConns - 1 ; 
        return 0 ; 
      }
    }
    for ( pc = *seed ; pc ; pc = pc->next )
      pc->ind++ ; 
    return -1 ; 
  }
}

int update_conn_info(rumInstanceRec *gInfo, ConnInfoRec *cInfo, int add)
{
  int rc;
 #if USE_POLL
  struct pollfd *new ;
  int use_ib = (gInfo->basicConfig.Protocol==RUM_IB) ; 
 #endif

  if ( add )
  {
    rc = update_conn_list(&gInfo->firstConnection, cInfo, &gInfo->nConnections, 1) ;  
   #if USE_POLL
    if ( gInfo->lrfds < (1+use_ib)*gInfo->nConnections )
    {
      int l = gInfo->lrfds + (1+use_ib)*256 ; 
      if ( l < (1+use_ib)*1024 ) l = (1+use_ib)*1024 ; 
      if ( !(new = realloc(gInfo->rfds, l*sizeof(struct pollfd))) )
      {
        char *methodName = "update_conn_info";
        TCHandle tcHandle = NULL;
        rmmReceiverRec *rInfo ; 
        rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
        tcHandle = rInfo->tcHandle ; 
        LOG_MEM_ERR(tcHandle,methodName,(l*sizeof(struct pollfd)));
        return -1 ; 
      }
      gInfo->rfds = new ; 
      memset(gInfo->rfds+gInfo->lrfds, 0, (l-gInfo->lrfds)*sizeof(struct pollfd)) ; 
      gInfo->lrfds = l ; 
    }
    new = gInfo->rfds + (1+use_ib)*cInfo->ind ; 
    new->fd = cInfo->sfd ; 
    new->events = POLLIN ; 
    if ( !use_ib )
    {
      if ( gInfo->lwfds < gInfo->nConnections )
      {
        int l = gInfo->lwfds + 256 ; 
        if ( l < 1024 ) l = 1024 ; 
        if ( !(new = realloc(gInfo->wfds, l*sizeof(struct pollfd))) )
        {
          char *methodName = "update_conn_info";
          TCHandle tcHandle = NULL;
          rmmReceiverRec *rInfo ; 
          rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
          tcHandle = rInfo->tcHandle ; 
          LOG_MEM_ERR(tcHandle,methodName,(l*sizeof(struct pollfd)));
          return -1 ; 
        }
        gInfo->wfds = new ; 
        memset(gInfo->wfds+gInfo->lwfds, 0, (l-gInfo->lwfds)*sizeof(struct pollfd)) ; 
        gInfo->lwfds = l ; 
      }
      new = gInfo->wfds + cInfo->ind ; 
      new->fd = cInfo->sfd ; 
      new->events = POLLOUT; 
    }
   #endif
  }
  else
  {
    rc = update_conn_list(&gInfo->firstConnection, cInfo, &gInfo->nConnections, 0) ;  
   #if USE_POLL
    if ( !rc )
    {
      int i;
      int ind = cInfo->ind ; 
      for ( i=(1+use_ib)*ind ; i<(1+use_ib)*gInfo->nConnections ; i++ )
        gInfo->rfds[i] = gInfo->rfds[i+(1+use_ib)] ; 
      if ( !use_ib )
      {
        for ( i=ind ; i<gInfo->nConnections ; i++ )
          gInfo->wfds[i] = gInfo->wfds[i+1] ; 
      }
    }
   #endif
  }
 #if USE_POLL
  gInfo->nrfds = (1+use_ib)*gInfo->nConnections ;
  if ( !use_ib )
    gInfo->nwfds = gInfo->nConnections ;
 #endif
  if ( gInfo->rInfo )
  {
    rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
    int nc = gInfo->nConnections ; 
    if ( nc < 1 ) nc = 1 ; 
    nc += 4 ; 
    rInfo->PerConnInQwm[0] = 4*rInfo->MaxPacketsAllowed/5/nc ; 
    rInfo->PerConnInQwm[1] = 2*rInfo->MaxPacketsAllowed/5/nc ; 
  }
  return rc ; 
}

/**-------------------------------------**/
/**-------------------------------------**/
int  rumR_Init(rumInstanceRec *gInfo, int *error_code)
{
  int i, rc , instance ;              
  TCHandle tcHandle = gInfo->tcHandles[1];
  const char* methodName = "rumR_Init"; 
  LOG_METHOD_ENTRY();

  rmmRmutex_lock() ;

  if ( rNumInstances == 0 )
  {
    rmmInitTime() ; /* init BaseTimeSecs under mutex */

  /* check_time_funcs() ; */

    memset(rInstances,0,sizeof(rmmReceiverRec *)*MAX_Rx_INSTANCES) ;
    rNumInstances++ ; /* just to make sure the above is done only once  */
  }

  if ( rNumInstances < MAX_Rx_INSTANCES )
    i = rNumInstances ;
  else
  {
    for ( i=0 ; i<MAX_Rx_INSTANCES && rInstances[i] != NULL ; i++ ) ; /* empty body */
    if ( i >= MAX_Rx_INSTANCES )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3100),"%d",
        "The maximum number of receiver instances has been reached. The maximum number is {0}.",MAX_Rx_INSTANCES );
      rmmRmutex_unlock() ;
      *error_code = RUM_L_E_TOO_MANY_INSTANCES ; 
      return RMM_FAILURE ;
    }
  }
  i = gInfo->instance ; 
  if ( i >= MAX_Rx_INSTANCES ||  rInstances[i] != NULL )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3101),"%d",
      "The maximum number of receiver instances has been reached. The maximum number is {0}.",MAX_Rx_INSTANCES );
    rmmRmutex_unlock() ;
    *error_code = RUM_L_E_TOO_MANY_INSTANCES ; 
    return RMM_FAILURE ;
  }
  instance = i ;
  rc = _rumR_Init(instance, gInfo, tcHandle,error_code) ; 

  if ( rc == RMM_SUCCESS )
  {
    gInfo->instance_r = instance ;
    gInfo->rxRunning = 1 ;
    if ( instance >= rNumInstances )
      rNumInstances++ ;
  }
  LOG_METHOD_EXIT();

  rmmRmutex_unlock() ;

  return rc ;
}


/**-------------------------------------**/
/**-------------------------------------**/
int _rumR_Init(int instance, rumInstanceRec *gInfo, TCHandle tcHandle, int *error_code) 
{
  int rc ;
  SAS sas ;
  SA4 *sa4 ; 
  SA6 *sa6 ; 
  int ival ; 
  rmmReceiverRec *rInfo=NULL ;
  const char* methodName = "_rumR_Init";
  char errMsg[ERR_MSG_SIZE];
  TBHandle tbHandle;
  LOG_METHOD_ENTRY();


  if ( !nla_afi_len )
  {
    ival = ( NLA_AFI<NLA_AFI6 ? NLA_AFI6 : NLA_AFI ) + 1 ; 
    if ( (nla_afi_len=(int *)malloc(sizeof(int)*ival)) == NULL ||
         (nla_afi_af =(int *)malloc(sizeof(int)*ival)) == NULL )
    {
      LOG_MEM_ERR(tcHandle,methodName,(sizeof(int)*ival));
      *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ; 
      return RMM_FAILURE ;
    }
    memset(nla_afi_len,0,sizeof(int)*ival) ; 
    memset(nla_afi_af ,0,sizeof(int)*ival) ; 
    nla_afi_len[NLA_AFI] = sizeof(IA4) ; 
    nla_afi_len[NLA_AFI6]= sizeof(IA6) ; 
    nla_afi_af [NLA_AFI] = AF_INET ; 
    nla_afi_af [NLA_AFI6]= AF_INET6 ; 

    ival = ( AF_INET<AF_INET6 ? AF_INET6 : AF_INET ) + 1 ; 
    if ( (sa_ip_len=(int *)malloc(sizeof(int)*ival)) == NULL ||
         (sa_ip_off=(int *)malloc(sizeof(int)*ival)) == NULL )
    {
      LOG_MEM_ERR(tcHandle,methodName,(sizeof(int)*ival));
      *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ; 
      return RMM_FAILURE ;
    }
    memset(sa_ip_len,0,sizeof(int)*ival) ; 
    memset(sa_ip_off,0,sizeof(int)*ival) ; 
    sa_ip_len[AF_INET ] = sizeof(IA4) ; 
    sa_ip_len[AF_INET6] = sizeof(IA6) ; 
    sa4 = (SA4 *)&sas ; 
    sa6 = (SA6 *)&sas ; 
    sa_ip_off[AF_INET ] = (int)((char *)&sa4->sin_addr  - (char *)sa4) ; 
    sa_ip_off[AF_INET6] = (int)((char *)&sa6->sin6_addr - (char *)sa6) ; 
  }

  if ( (rInfo = (rmmReceiverRec *)malloc(sizeof(rmmReceiverRec))) == NULL )
  {
    LOG_MEM_ERR(tcHandle,methodName,(sizeof(rmmReceiverRec)));
    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ; 
    return RMM_FAILURE ;
  }

  memset(rInfo,0,sizeof(rmmReceiverRec)) ; /* so that only non-zeros need to be initialized! */
  rInfo->instance   = instance ;
  rInfo->gInfo = gInfo ; 
  rInfo->tcHandle = tcHandle;
  rInfo->rConfig = gInfo->basicConfig ;
  rInfo->aConfig = gInfo->advanceConfig ;

  rInfo->extTime = sysTOD ; 

  rInfo->fp_log = gInfo->fp_log ; 
  pthread_mutex_init(&rInfo->wb_mutex,NULL) ;
  init_why_bad_msgs(&rInfo->why_bad) ;

  if ( gInfo->on_log_event )
    rInfo->on_log_event    = gInfo->on_log_event ;
  else
    rInfo->on_log_event    = myLogger ;
  rInfo->on_alert_event   = gInfo->on_alert_event ;
  rInfo->log_user         = gInfo->log_user ;
  rInfo->alert_user       = gInfo->alert_user ;

  rInfo->PGM_OPT_PRESENT = rInfo->aConfig.opt_present ; 
  rInfo->PGM_OPT_NET_SIG = rInfo->aConfig.opt_net_sig ; 

  rInfo->nbad        = 0 ;
  rInfo->rNumTopics  = 0 ;
  rInfo->rNumStreams = 0 ;
  rInfo->TotPacksIn  = 0 ;
  rInfo->TotPacksOut = 0 ;

  rInfo->CurrentTime = rmmTime(NULL,NULL) ; 
  rInfo->NakAlertCount = 0 ;

  rInfo->NumPRthreads = 1 ;
  rInfo->UseNoMA = rInfo->aConfig.UseNoMA ;
  if ( rInfo->UseNoMA )
  {
    rInfo->NumPPthreads = rInfo->aConfig.NumMAthreads ; 
    rInfo->NumMAthreads = 0 ;
  }
  else
  {
    rInfo->NumPPthreads = 1 ; 
    rInfo->NumMAthreads = rInfo->aConfig.NumMAthreads ;
  }
  rInfo->NumPPthreads += rInfo->NumPRthreads ; 
  rInfo->NumPPthreads += rInfo->aConfig.NumExthreads ; 
  if ( rInfo->NumPPthreads < 1 )
       rInfo->NumPPthreads = 1 ;
  rInfo->NumPRthreads = 0 ;
  rInfo->DataPacketSize    = rInfo->aConfig.DataPacketSize+sizeof(rumHeader) ; 

  rInfo->MaxSqnPerNack = rInfo->aConfig.MaxSqnPerNack ;

  rInfo->MaxMemoryAllowed  = (uintptr_t)rInfo->rConfig.RxMaxMemoryAllowedMBytes*1024*1024 ;
  rInfo->MaxPacketsAllowed = (unsigned int)(rInfo->MaxMemoryAllowed / rInfo->DataPacketSize + 10) ;

  rInfo->dataBuffsQsize    = rInfo->MaxPacketsAllowed/4 ;
  rInfo->recvBuffsQsize    = rInfo->dataBuffsQsize ;
  rInfo->nackStrucQsize    = rInfo->aConfig.nackBuffsQsize ; 
  rInfo->evntStrucQsize    = rInfo->aConfig.evntQsize ; 
  rInfo->packStrucQsize    = rInfo->dataBuffsQsize ;
  rInfo->dataQsize         = rInfo->MaxPacketsAllowed ;
  rInfo->nackQsize         = rInfo->aConfig.nackQsize ;
  rInfo->recvQsize         = rInfo->aConfig.recvQsize ;
  rInfo->rsrvQsize         = rInfo->aConfig.rsrvQsize ;
  rInfo->evntQsize         = rInfo->aConfig.evntQsize ;
  rInfo->fragQsize         = rInfo->aConfig.fragQsize ;
  if ( rInfo->aConfig.packQsize > 0 )
    rInfo->packQsize         = rInfo->aConfig.packQsize ;
  else
    rInfo->packQsize         = rInfo->MaxPacketsAllowed ;

  rInfo->PerConnInQwm[0] = 4*rInfo->MaxPacketsAllowed/5 ; 
  rInfo->PerConnInQwm[1] = 2*rInfo->MaxPacketsAllowed/5 ; 

  gethostname(rInfo->HostName,256) ; 
  tbHandle = llmCreateTraceBuffer(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(1385));
  if ( rmmGetLocalIf(rInfo->rConfig.RxNetworkInterface,(rInfo->rConfig.Protocol==RUM_IB),0,0,rInfo->rConfig.IPVersion,&rInfo->HostIF,&rc,errMsg,tbHandle) != RMM_SUCCESS )
  {
    llmCompositeTraceInvoke(tbHandle);
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3105),"%s",
      "The RUM receiver failed to obtain the host address. The error is: {0}", errMsg);
    *error_code = RUM_L_E_LOCAL_INTERFACE ; 
    free(rInfo) ;
    return RMM_FAILURE ;
  }
  else
    if(tbHandle != NULL) llmCompositeTraceInvoke(tbHandle);

  if ( IF_HAVE_IPV4(&rInfo->HostIF) )
    rmm_ntop(AF_INET ,&rInfo->HostIF.ipv4_addr,rInfo->HostAddress,64) ;
  else
  if ( IF_HAVE_GLOB(&rInfo->HostIF) )
    rmm_ntop(AF_INET6,&rInfo->HostIF.glob_addr,rInfo->HostAddress,64) ;
  else
  if ( IF_HAVE_SITE(&rInfo->HostIF) )
    rmm_ntop(AF_INET6,&rInfo->HostIF.site_addr,rInfo->HostAddress,64) ;
  else
  if ( IF_HAVE_LINK(&rInfo->HostIF) )
    rmm_ntop(AF_INET6,&rInfo->HostIF.link_addr,rInfo->HostAddress,64) ;
  else
    strlcpy(rInfo->HostAddress,"Unknown!",64) ; 

  gInfo->RxlocalIf = rInfo->HostIF ; 

  /*  Allocate memory and init mutexes */

  if ( init_recv_data(rInfo) == RMM_FAILURE )
  {
    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ; 
    free_recv_data(rInfo) ; 
    return RMM_FAILURE ;
  }

  /*  Start Threads */

  gInfo->rInfo = rInfo ; 

  if ( start_recv_threads(rInfo) == RMM_FAILURE )
  {
    pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
    if ( rInfo->GlobalSync.chEC )
      *error_code = rInfo->GlobalSync.chEC ; 
    else
      *error_code = RUM_L_E_THREAD_ERROR ; 
    pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
    stop_recv_threads(rInfo) ;
    free_recv_data(rInfo) ; 
    gInfo->rInfo = NULL ; 
    return RMM_FAILURE ;
  }
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5106),"",
    "All threads for the RUM receiver are running.");

  rInfo->cr_string = llm_static_copyright_string ;
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5107),"%s%s%s%s",
    "The RUM receiver (Version {0}|{1}|) is initialized on {2} {3}.", RMM_API_VERSION,version_string,rInfo->HostName,rInfo->HostAddress) ;



  tbHandle = llmCreateTraceBuffer(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5108));
  if(tbHandle != NULL){
    llmAddTraceData(tbHandle,"","RUM receiver configurations:  ");
    llmAddTraceData(tbHandle,"%d","Basic: [Protocol={0}, ", rInfo->rConfig.Protocol);
    llmAddTraceData(tbHandle,"%d","LogLevel={0}], ", rInfo->rConfig.LogLevel);
    llmAddTraceData(tbHandle,"%d","SocketBufferSize={0}, ", rInfo->rConfig.SocketReceiveBufferSizeKBytes);
    llmAddTraceData(tbHandle,"%d","MaxMemoryAllowed={0}, ", rInfo->rConfig.RxMaxMemoryAllowedMBytes);
    llmAddTraceData(tbHandle,"%s","NetworkInterface={0}, ", rInfo->rConfig.RxNetworkInterface);
    llmAddTraceData(tbHandle,"%d","IPVersion={0}, ", rInfo->rConfig.IPVersion);
    llmAddTraceData(tbHandle,"%s","AdvanceConfigFile={0}]", rInfo->rConfig.AdvanceConfigFile);
    
    /*Advanced Configuration*/
    llmAddTraceData(tbHandle,"%d"," , Advanced: [UseNoMA={0}, ", rInfo->aConfig.UseNoMA);
    llmAddTraceData(tbHandle,"%d","NumMAthreads={0}, ", rInfo->aConfig.NumMAthreads);
    llmAddTraceData(tbHandle,"%d","NumExthreads={0}, ", rInfo->aConfig.NumExthreads);
    llmAddTraceData(tbHandle,"%d","ThreadPerTopic={0}, ", rInfo->aConfig.ThreadPerTopic);
    llmAddTraceData(tbHandle,"%d","DataPacketSize={0}, ", rInfo->aConfig.DataPacketSize);
    llmAddTraceData(tbHandle,"%d","BaseWaitMili={0}, ", rInfo->aConfig.BaseWaitMili);
    llmAddTraceData(tbHandle,"%d","LongWaitMili={0}, ", rInfo->aConfig.LongWaitMili);
    llmAddTraceData(tbHandle,"%d","BaseWaitNano={0}, ", rInfo->aConfig.BaseWaitNano);
    llmAddTraceData(tbHandle,"%d","LongWaitNano={0}, ", rInfo->aConfig.LongWaitNano);
    llmAddTraceData(tbHandle,"%d","SnapShotCycle={0}, ", rInfo->aConfig.SnapshotCycleMilli_rx);
    llmAddTraceData(tbHandle,"%d","PrintStreamInfo_rx={0}, ", rInfo->aConfig.PrintStreamInfo_rx);
    llmAddTraceData(tbHandle,"%d","MemoryCrisisCycle={0}, ", rInfo->aConfig.MemoryCrisisCycle);
    llmAddTraceData(tbHandle,"%d","NackGenerCycle={0}, ", rInfo->aConfig.NackGenerCycle);
    llmAddTraceData(tbHandle,"%d","TaskTimerCycle={0}, ", rInfo->aConfig.TaskTimerCycle);
    llmAddTraceData(tbHandle,"%d","NackTimeoutBOF={0}, ", rInfo->aConfig.NackTimeoutBOF);
    llmAddTraceData(tbHandle,"%d","NackTimeoutNCF={0}, ",rInfo->aConfig.NackTimeoutNCF);
    llmAddTraceData(tbHandle,"%d","NackTimeoutData={0}, ",rInfo->aConfig.NackTimeoutData);
    llmAddTraceData(tbHandle,"%d","NackRetriesNCF={0}, ",rInfo->aConfig.NackRetriesNCF);  
    llmAddTraceData(tbHandle,"%d","NackRetriesData={0}, ",rInfo->aConfig.NackRetriesData);  
    llmAddTraceData(tbHandle,"%d","MaxNacksPerCycle={0}, ",rInfo->aConfig.MaxNacksPerCycle); 
    llmAddTraceData(tbHandle,"%d","MaxSqnPerNack={0}, ",rInfo->aConfig.MaxSqnPerNack); 
    llmAddTraceData(tbHandle,"%d","LogLevel={0}, ",rInfo->aConfig.LogLevel); 
    llmAddTraceData(tbHandle,"%s","LogFile={0}] ",rInfo->aConfig.LogFile); 

    /*Internal Configs*/
    llmAddTraceData(tbHandle,"","Internal: "); 
    llmAddTraceData(tbHandle,"%d","[DataPacketSize={0}, ",rInfo->DataPacketSize); 
    llmAddTraceData(tbHandle,"%llu","MaxMemoryAllowed={0}, ",(rmm_uint64)rInfo->MaxMemoryAllowed); 
    llmAddTraceData(tbHandle,"%u","MaxPacketsAllowed={0}, ",rInfo->MaxPacketsAllowed); 
    llmAddTraceData(tbHandle,"%d","MaxSqnPerNack={0}, ",rInfo->MaxSqnPerNack);
    llmAddTraceData(tbHandle,"%d","recvBuffsQsize={0}, ",rInfo->recvBuffsQsize);
    llmAddTraceData(tbHandle,"%d","dataBuffsQsize={0}, ",rInfo->dataBuffsQsize);
    llmAddTraceData(tbHandle,"%d","nackStrucQsize={0}, ",rInfo->nackStrucQsize);
    llmAddTraceData(tbHandle,"%d","evntStrucQsize={0}, ",rInfo->evntStrucQsize);
    llmAddTraceData(tbHandle,"%d","dataQsize={0}, ",rInfo->evntStrucQsize);
    llmAddTraceData(tbHandle,"%d","nackQsize={0}, ",rInfo->nackQsize);
    llmAddTraceData(tbHandle,"%d","recvQsize={0}, ",rInfo->recvQsize);
    llmAddTraceData(tbHandle,"%d","rsrvQsize={0}, ",rInfo->rsrvQsize);
    llmAddTraceData(tbHandle,"%d","fragQsize={0}, ",rInfo->fragQsize);
    llmAddTraceData(tbHandle,"%d","NumPPthreads={0}, ",rInfo->NumPRthreads);
    llmAddTraceData(tbHandle,"%d","NumMAthreads={0}]",rInfo->NumMAthreads);
    /*Write to the log*/
    llmCompositeTraceInvoke(tbHandle);
  }



  rInstances[instance] = rInfo ;
  gInfo->instance_r = instance ;
  gInfo->rInfo = rInfo ; 

  LOG_METHOD_EXIT();


  return RMM_SUCCESS ;
}

int  rumR_ChangeLogLevel(rumInstanceRec *gInfo, int log_level, int *error_code)
{
  int instance ; 
  rmmReceiverRec *rInfo=NULL ;
  const char* methodName="rumR_ChangeLogLevel";
  TCHandle tcHandle = NULL;
  if ( (instance=rmmR_chk_instance(gInfo->instance, &tcHandle,error_code)) < 0 )
    return RMM_FAILURE ;
  LOG_METHOD_ENTRY();

  rInfo = rInstances[instance] ;


  if ( log_level < RMM_LOGLEV_NONE     ||
       log_level > RMM_LOGLEV_XTRACE )
  {
    *error_code = RUM_L_E_BAD_PARAMETER ; 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3109),"%d",
      "The specified log level ({0}) is not valid. ", log_level);
    rmmRmutex_unlock() ;
    return RMM_FAILURE ;
  }
  rInfo->rConfig.LogLevel = log_level ; 

  rmmRmutex_unlock() ;
  LOG_METHOD_EXIT();
  return RMM_SUCCESS ; 

}

/**-------------------------------------**/
/**-------------------------------------**/

int  rumR_CreateQueue(rumInstanceRec *gInfo, const rumRxQueueParameters *queue_params, rumQueueR *queue_r, int *ec)
{
 int rc , i , instance , topicLen , handle ; 
 rTopicInfoRec *ptp ;
  rmmReceiverRec *rInfo=NULL ;

  const char* methodName="rumR_CreateQueue";
  TCHandle tcHandle = NULL;
  TBHandle    tbh = NULL;
  if ( (instance=rmmR_chk_instance(gInfo->instance, &tcHandle,ec)) < 0 )
    return RMM_FAILURE ;
  LOG_METHOD_ENTRY();

  rInfo = rInstances[instance] ;
  tbh = llmCreateTraceBuffer(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5001));
  if(tbh != NULL){
    llmAddTraceData(tbh,"","The RUM receiver queue is being created using the following configuration:");
    print_topic_params(queue_params,tbh);
    llmCompositeTraceInvoke(tbh);
    tbh = NULL;
  }


  if ( queue_params->accept_stream == NULL )
  {
    if ( queue_params->queue_name == NULL )
      topicLen = 0 ;
    else
    {
      topicLen = (int)strllen(queue_params->queue_name,RUM_MAX_QUEUE_NAME) ;
      if ( topicLen+1 > RUM_MAX_QUEUE_NAME )
      {
        if ( ec ) *ec = RUM_L_E_BAD_PARAMETER ;
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3110),"%d%d",
          "The queue name length of {0} is more than the maximum length of {1}.", topicLen, RUM_MAX_QUEUE_NAME);
        rmmRmutex_unlock() ;
        return RMM_FAILURE ;
      }
    }
  
    for ( i=0 ; i<rInfo->rNumTopics ; i++ )
    {
      if ( (ptp = rInfo->rTopics[i]) == NULL )
        continue ;
      if ( ptp->accept_stream )
        continue ; 
      if ( topicLen == ptp->topicLen && (topicLen == 0 || memcmp(ptp->topicName,queue_params->queue_name,topicLen)) == 0 )
      {
        if ( ec ) *ec = RUM_L_E_BAD_PARAMETER ;
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3111),"%s",
          "The specified queue name {0} already exists.", ptp->topicName);
        rmmRmutex_unlock() ;
        return RMM_FAILURE ;
      }
    }
  }
  else
    topicLen = 0 ; 

  if ( queue_params->reliability != RUM_UNRELIABLE_RCV    &&
       queue_params->reliability != RUM_RELIABLE_RCV        )
  {
    if ( queue_params->reliability == RUM_UNRELIABLE             ||      /* BEAM suppression: incompatible types */
         queue_params->reliability == RUM_RELIABLE               ||      /* BEAM suppression: incompatible types */
         queue_params->reliability == RUM_RELIABLE_NO_HISTORY      )     /* BEAM suppression: incompatible types */
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3113),"%d",
        "The specified reliability configuration parameter {0} should be used for a transmitter queue only. Specify a valid reliablity configuration parameter for the RUM receiver.", queue_params->reliability);
    }
    else
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3112),"%d",
        "The specified reliability configuration {0} is not valid. ", queue_params->reliability);
    }
    if ( ec ) *ec = RUM_L_E_BAD_PARAMETER ;
    rmmRmutex_unlock() ;
    return RMM_FAILURE ;
  }

  if ( (queue_params->on_data!=NULL) + (queue_params->on_message!=NULL) + (queue_params->on_packet!=NULL) != 1 )
  {
    if ( ec ) *ec = RMM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3114),"",
      "The RUM receiver queue requires only one of the following callbacks: on_data, on_packet, or on_message. Multiple callbacks were specified.");
    return RMM_FAILURE ;
  }


  /**+++++++++++++++++++++++++++++++++++++++**/

  if ( rInfo->rNumTopics < MAX_TOPICS )
    handle = rInfo->rNumTopics ;
  else
  {
    for ( handle=0 ; handle < rInfo->rNumTopics && rInfo->rTopics[handle] != NULL ; handle++ ) ; /* empty body */
    if ( handle >= MAX_TOPICS )
    {
      if ( ec ) *ec = RUM_L_E_TOO_MANY_STREAMS ; 
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3117),"%d%d",
        "The number of queues ({0}) is more than the maximum number of queues ({1}) per instance.",handle,MAX_TOPICS );
      rmmRmutex_unlock() ;
      return RMM_FAILURE ;
    }
  }

  if ( (ptp = (rTopicInfoRec *)malloc(sizeof(rTopicInfoRec))) == NULL )
  {
    if ( ec ) *ec = RUM_L_E_MEMORY_ALLOCATION_ERROR ; 
    LOG_MEM_ERR(tcHandle,methodName,(sizeof(rTopicInfoRec)));
    rmmRmutex_unlock() ;
    return RMM_FAILURE ;
  }
  memset(ptp,0,sizeof(rTopicInfoRec)) ;  /*  need to init only non-zeros */

  if ( queue_params->accept_stream == NULL )
  {
    if ( queue_params->queue_name != NULL )
    {
      ptp->topicLen = topicLen ;
      if ( ptp->topicLen > 0 )
      {
        memcpy(ptp->topicName,queue_params->queue_name,topicLen) ;
      }
      ptp->topicName[ptp->topicLen] = 0 ;
    }
    else
      ptp->topicLen = 0 ;
  }

  strlcpy(ptp->ev.queue_name,ptp->topicName,RUM_MAX_QUEUE_NAME) ;

  ptp->instance_id   = instance ;
  ptp->topic_id      = handle ;
  ptp->reliability   = queue_params->reliability ;
  ptp->reliable      = (ptp->reliability != RMM_UNRELIABLE ) ; 
  ptp->ordered       = 1 ; 
  ptp->failover      = 0 ; 
  ptp->msn_next      = 1 ; 

  ptp->accept_stream = queue_params->accept_stream ; 
  ptp->ss_user       = queue_params->accept_user   ; 
  ptp->on_message = queue_params->on_message ;
  ptp->on_packet  = queue_params->on_packet  ;
  ptp->on_data    = queue_params->on_data    ;
  ptp->om_user    = queue_params->user ;
  ptp->on_event = queue_params->on_event ;
  ptp->ov_user  = queue_params->event_user ;

  ptp->byStream = 0 ; 

  setParseMTL(ptp) ;


  if ( ptp->on_data )
  {
    int off = offsetof(rmmPacket,rmm_ptr_reserved) ;
    if ( (ptp->packQ = LL_alloc(off,0) ) == NULL )
    {
      if ( ec ) *ec = RMM_L_E_MEMORY_ALLOC ;
      LOG_MEM_ERR(tcHandle,methodName,(sizeof(rInfo->packQsize)));
      free(ptp) ;
      return RMM_FAILURE ;
    }

    if ( rInfo->packStrucQ == NULL &&
        (rInfo->packStrucQ = MM_alloc2(sizeof(rmmPacket),rInfo->packStrucQsize,rInfo->MaxPacketsAllowed,0,0, off, (int)sizeof(void *))) == NULL )
    {
      if ( ec ) *ec = RMM_L_E_MEMORY_ALLOC ;
      LOG_MEM_ERR(tcHandle,methodName,(sizeof(rmmPacket)*rInfo->MaxPacketsAllowed));
      LL_free(ptp->packQ,0) ;
      free(ptp) ;
      return RMM_FAILURE ;
    }

    if ( queue_params->max_pending_packets > 0 )
      ptp->maxPackInQ = queue_params->max_pending_packets ;
    if ( queue_params->max_queueing_time_milli > 0 )
      ptp->maxTimeInQ = queue_params->max_queueing_time_milli ;
    ptp->need_trim = (ptp->maxPackInQ>0 || ptp->maxTimeInQ>0) ;
    rInfo->nNeedTrim += ptp->need_trim ;

    ptp->odEach = queue_params->notify_per_packet ;
  }

  if ( queue_params->goback_time_milli > 0 )
  {
    ptp->goback.lag_time = queue_params->goback_time_milli ; 
    rInfo->haveGoback = 1 ; 
  }

  if ( queue_params->stream_join_backtrack > 0 )
    ptp->stream_join_backtrack = queue_params->stream_join_backtrack ; 

  pthread_mutex_init(&ptp->maMutex,NULL) ;

  rmm_rwlock_wrlock(&rInfo->GlobalSync.rw) ;

  queue_r->instance = gInfo->instance ;
  queue_r->handle = handle ;
  rInfo->rTopics[handle] = ptp ;

  if ( handle >= rInfo->rNumTopics )
    rInfo->rNumTopics = handle+1 ;

  clear_rejected_streams(rInfo,0,2) ;

  rmm_rwlock_wrunlock(&rInfo->GlobalSync.rw) ;

  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(3124),"",
    "The RUM receiver flushed the rejected streams list of type 2." );


  rc = RUM_SUCCESS ; 


  {
    tbh = llmCreateTraceBuffer(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5126));
    if(tbh != NULL){
      llmAddTraceData(tbh,"%d%s","The RUM receiver queue ({0} | {1}) was created using the following configuration parameters: ",
        ptp->topic_id, ptp->topicName);
      llmAddTraceData(tbh,"%d%d%d%d%d%d%d","topic length={0},  reliability={1}, isTurboFlow={2}, isPr={3}, goback_time_milli={4}, npp={5}, byStream={6}, ",
        ptp->topicLen,ptp->reliability,ptp->isTf,ptp->isPr,queue_params->goback_time_milli,ptp->odEach, ptp->byStream);
      llmAddTraceData(tbh,"%llx%llx%llx%llx%llx%llx%llx%llx",\
        "accept_stream={0}, ss_user={1}, on_event={2}, ov_user={3}, on_message={4}, on_packet={5}, on_data={6}, om_user={7}, ",
        LLU_VALUE(ptp->accept_stream),LLU_VALUE(ptp->ss_user),LLU_VALUE(ptp->on_event),
        LLU_VALUE(ptp->ov_user),LLU_VALUE(ptp->on_message),LLU_VALUE(ptp->on_packet),
        LLU_VALUE(ptp->on_data),LLU_VALUE(ptp->om_user));
      llmCompositeTraceInvoke(tbh);
    }
  }

  /**+++++++++++++++++++++++++++++++++++++++**/


  LOG_METHOD_EXIT();
  rmmRmutex_unlock() ;
  return rc ;

}

/**-------------------------------------**/
/**-------------------------------------**/

int rumR_CloseQueue(rumInstanceRec *gInfo, int q_handle, int *error_code)
{
  int rc, instance,topic ;
  rTopicInfoRec *ptp ;
  rmmReceiverRec *rInfo=NULL ;
  const char* methodName="rumR_CloseQueue";
  TCHandle tcHandle = NULL;
  if ( (instance=rmmR_chk_instance(gInfo->instance, &tcHandle,error_code)) < 0 )
    return RMM_FAILURE ;
  LOG_METHOD_ENTRY();
  rInfo = rInstances[instance] ;
  if ( (topic=rmmR_chk_topic(rInfo, q_handle, methodName,error_code)) < 0 )
    return RMM_FAILURE ;
  ptp = rInfo->rTopics[topic] ;

  LL_lock(rInfo->mastQ) ;
  ptp->closed = 1 ;
  LL_unlock(rInfo->mastQ) ;

  rc = (tp_lock(rInfo,ptp,500,3)) ? RMM_SUCCESS : RMM_BUSY ; /* !! no need to tp_unlock here */

  if (ptp->packQ)
  {
    LL_lock(ptp->packQ) ;
    LL_signalE(ptp->packQ) ;
    LL_signalF(ptp->packQ) ;
    LL_unlock(ptp->packQ) ;
    if ( rc == RMM_SUCCESS && ptp->on_data )
      ptp->on_data(ptp->om_user) ;
  }

  if ( rc == RMM_SUCCESS )
  {
    rmm_rwlock_wrlock(&rInfo->GlobalSync.rw) ;
    rc = _rumR_CloseTopic(rInfo,topic,error_code) ;
    rmm_rwlock_wrunlock(&rInfo->GlobalSync.rw) ;
  }
  else
  {
  LOG_BUSY_TOPIC(ptp->topicName);
  }
  LOG_METHOD_EXIT();
  rmmRmutex_unlock() ;
  return rc ;
}

/**-------------------------------------**/

int _rumR_CloseTopic(rmmReceiverRec *rInfo, int topic, int *error_code)
{
  rTopicInfoRec *ptp ;
  rStreamInfoRec *pst, *nst ; 
  rumInstanceRec *gInfo ; 
  const char* methodName="_rumR_CloseTopic";
  TCHandle tcHandle = rInfo->tcHandle;
  LOG_METHOD_ENTRY();

  ptp = rInfo->rTopics[topic] ;

  for ( pst=rInfo->firstStream ; pst ; pst=nst )
  {
    nst = pst->next ;
    if ( ptp == rInfo->rTopics[pst->topic_id] )
    {
      scpInfoRec sInfo[1] ; 
      memset(sInfo,0,sizeof(scpInfoRec)) ; 
      sInfo->sid         = pst->sid ; 
      sInfo->cid         = pst->cInfo->connection_id ; 
      sInfo->reliability = pst->reliability ; 
      sInfo->keepHistory = pst->keepHistory ; 
      sInfo->isTf        = pst->isTf        ; 
      sInfo->isPr        = pst->isPr        ; 
      sInfo->src_nla     = pst->src_nla     ; 
      sInfo->src_port    = ntohs(pst->src_port) ; 
      sInfo->topicLen    = pst->topicLen    ; 
      memcpy(sInfo->topicName,pst->topicName,pst->topicLen+1) ; 
      add_scp(rInfo,sInfo) ; 
      remove_stream(pst) ;
      pst->maIn = 0 ; /* if this is 1 it means a thread was cancelled without resetting it */
      delete_stream(pst) ;
    }
  }

  rInfo->rTopics[topic] = NULL ;

  gInfo = rInfo->gInfo ; 
  PutFcbEvent(gInfo, gInfo->free_callback, ptp->om_user) ; 
  PutFcbEvent(gInfo, gInfo->free_callback, ptp->ov_user) ; 
  PutFcbEvent(gInfo, gInfo->free_callback, ptp->ss_user) ; 

  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5149),"%s",
    "The queue {0} is closed.", ptp->topicName);


  pthread_mutex_destroy(&ptp->maMutex) ;

  if (ptp->packQ)
  {
    rumPacket *rPack;
    LL_lock(ptp->packQ) ;
    while ( (rPack=LL_get_buff(ptp->packQ,0)) != NULL )
    {
      return_packet(rInfo, rPack);
    }
    LL_signalE(ptp->packQ) ;
    LL_unlock(ptp->packQ) ;
    tsleep(0,rInfo->aConfig.BaseWaitNano) ;
    LL_free(ptp->packQ,0) ;
    if ( ptp->need_trim )
      rInfo->nNeedTrim-- ;
  }

  if ( ptp->BitMapPos )
    free(ptp->BitMapPos) ; 

  free(ptp) ;

  LOG_METHOD_EXIT();
  return RMM_SUCCESS ;
}

/**-------------------------------------**/
/**-------------------------------------**/
void rumR_PerConnInQup(rmmReceiverRec *rInfo, rStreamInfoRec *pst)
{
  if ( rInfo && pst && pst->cInfo && (++pst->cInfo->inQ[0]) - pst->cInfo->inQ[1] > rInfo->PerConnInQwm[0] && !pst->cInfo->hold_it )
  {
    rmm_rwlock_rdlock(&rInfo->gInfo->ConnSyncRW) ; 
    pst->cInfo->hold_it = 1 ; 
   #if USE_POLL
    rInfo->gInfo->rfds[(1+pst->cInfo->use_ib)*pst->cInfo->ind].events &= (~POLLIN) ; 
   #endif
    rmm_rwlock_rdunlock(&rInfo->gInfo->ConnSyncRW) ; 
  }
}
void rumR_PerConnInQdn(rmmReceiverRec *rInfo, rStreamInfoRec *pst)
{
  if ( rInfo && pst && pst->cInfo && pst->cInfo->inQ[0] - (++pst->cInfo->inQ[1]) <= rInfo->PerConnInQwm[1] && pst->cInfo->hold_it )
  {
    rmm_rwlock_rdlock(&rInfo->gInfo->ConnSyncRW) ; 
    pst->cInfo->hold_it = 0 ; 
   #if USE_POLL
    rInfo->gInfo->rfds[(1+pst->cInfo->use_ib)*pst->cInfo->ind].events |= POLLIN ; 
   #endif
    pst->cInfo->last_r_time = rInfo->CurrentTime ; 
    rmm_rwlock_rdunlock(&rInfo->gInfo->ConnSyncRW) ; 
  }
}

/**-------------------------------------**/
/**-------------------------------------**/

int rumR_ClearRejectedStreams(rumInstanceRec *gInfo, int *error_code)
{
  int instance ; 
  rmmReceiverRec *rInfo=NULL ;
  const char* methodName="rumR_ClearRejectedStreams";
  TCHandle tcHandle = NULL;
  if ( (instance=rmmR_chk_instance(gInfo->instance, &tcHandle,error_code)) < 0 )
    return RMM_FAILURE ;
  LOG_METHOD_ENTRY();
  rInfo = rInstances[instance] ;

  clear_rejected_streams(rInfo,0,(1|2)) ;
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5154),"",
    "The RUM receiver flushed the rejected streams list of types 1 and 2.");
  LOG_METHOD_EXIT();

  rmmRmutex_unlock() ;
  return RMM_SUCCESS ;
}

/**-------------------------------------**/
/**-------------------------------------**/

int rumR_RemoveStream(rumInstanceRec *gInfo, rumStreamID_t stream_id, int *error_code)
{
  int i , instance , rc ; 
  rmmReceiverRec *rInfo=NULL ;
  //rTopicInfoRec *ptp ;
  rStreamInfoRec *pst ;
  char sid_str[20] ; 
  const char* methodName="rumR_RemoveStream";
  TCHandle tcHandle = NULL;
  if ( (instance=rmmR_chk_instance(gInfo->instance, &tcHandle,error_code)) < 0 )
    return RMM_FAILURE ;
  LOG_METHOD_ENTRY();
  rInfo = rInstances[instance] ;

  rmm_rwlock_wrlock(&rInfo->GlobalSync.rw) ;
  if ( (pst = find_stream(rInfo,stream_id)) == NULL )
  {
    b2h(sid_str,(uint8_t *)&stream_id,sizeof(rumStreamID_t)) ; 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(3155),"%s",
      "The stream {0} is not in the active list.", sid_str);
    if ( (i=find_rejected_stream(rInfo,stream_id)) )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4156),"%s%d",
        "The stream {0} is already in the type {1} rejected list.", sid_str, i );
      rc = RMM_SUCCESS ; 
    }
    else
    {
      *error_code = RUM_L_E_BAD_PARAMETER ; 
      reject_stream(rInfo,stream_id,0,1) ; 
      rc = RMM_FAILURE ; 
    }
  }
  else
  {
    scpInfoRec sInfo[1] ; 
    //ptp = rInfo->rTopics[pst->topic_id] ;
    memset(sInfo,0,sizeof(scpInfoRec)) ; 
    sInfo->sid         = pst->sid ; 
    sInfo->cid         = pst->cInfo->connection_id ; 
    sInfo->reliability = pst->reliability ; 
    sInfo->keepHistory = pst->keepHistory ; 
    sInfo->isTf        = pst->isTf        ; 
    sInfo->isPr        = pst->isPr        ; 
    sInfo->src_nla     = pst->src_nla     ; 
    sInfo->src_port    = ntohs(pst->src_port) ; 
    sInfo->topicLen    = pst->topicLen    ; 
    memcpy(sInfo->topicName,pst->topicName,pst->topicLen+1) ; 
    add_scp(rInfo,sInfo) ; 
    reject_stream(rInfo,pst->sid,pst->cInfo->connection_id,1) ; 
    remove_stream(pst) ;
    delete_stream(pst) ;
    rc = RMM_SUCCESS ; 
  }
  rmm_rwlock_wrunlock(&rInfo->GlobalSync.rw) ;
  LOG_METHOD_EXIT();
  rmmRmutex_unlock() ;
  return rc ;
}

/**-------------------------------------**/
/**-------------------------------------**/

int rumR_Stop(rumInstanceRec *gInfo, int timeout_milli, int *error_code)
{
 int i,  instance , rc  ;
 rmmReceiverRec *rInfo=NULL ;
  rTopicInfoRec *ptp ;
  rStreamInfoRec *pst ; 
  const char* methodName="rumR_Stop";
  TCHandle tcHandle = NULL;
  if ( (instance=rmmR_chk_instance(gInfo->instance, &tcHandle,error_code)) < 0 )
    return RMM_FAILURE ;
  LOG_METHOD_ENTRY();
  rInfo = rInstances[instance] ;

  tsleep(0,milli2nano(timeout_milli)) ; 

  rc = RMM_SUCCESS ; 
  if ( stop_recv_threads(rInfo) != RMM_SUCCESS )
  {
    *error_code = RUM_L_E_THREAD_ERROR ; 
    rc = RMM_FAILURE ; 
  }

  for ( pst=rInfo->firstStream ; pst ; pst=pst->next )
    pst->maIn = 0 ; 

  for ( i=0 ; i<rInfo->rNumTopics ; i++ )
  {
    if ( (ptp=rInfo->rTopics[i]) != NULL )
      ptp->maIn = 0 ;
  }

  for ( i=0 ; i<rInfo->rNumTopics ; i++ )
  {
    if ( (ptp=rInfo->rTopics[i]) == NULL )
      continue ;
    if ( (rc=_rumR_CloseTopic(rInfo,i,error_code)) != RMM_SUCCESS )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3156),"%s%d",
        "The RUM receiver failed to close queue {0}. The error code is {1}.", ptp->topicName,*error_code );
      rc = RMM_FAILURE ; 
    }
  }

  delete_all_connections(gInfo) ; 

  gInfo->rInfo = NULL ; 
  rInstances[instance] = NULL ;
  if ( free_recv_data(rInfo) != RMM_SUCCESS )
  {
    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ; 
    rc = RMM_FAILURE ; 
  }
  rInfo = NULL ;
  gInfo->instance_r = -1;
  gInfo->rxRunning = 0 ;

  LOG_METHOD_EXIT();
  rmmRmutex_unlock() ;
  return rc ;
}

int init_recv_data(rmmReceiverRec *rInfo)
{
  int i,rc,iError ; 
  void *buff , *buffs[1024] ;
  const char* methodName="init_recv_data";
  TCHandle tcHandle = rInfo->tcHandle;
  LOG_METHOD_ENTRY();

  if ( (rInfo->rsrvQ = BB_alloc(rInfo->rsrvQsize,0) ) == NULL )
  {
    LOG_MEM_ERR(tcHandle,methodName,(rInfo->rsrvQsize*sizeof(void*)));
    return RMM_FAILURE ;
  }

  if ( (rInfo->rmevQ = BB_alloc(rInfo->evntQsize,0) ) == NULL )
  {
    LOG_MEM_ERR(tcHandle,methodName,(rInfo->evntQsize*sizeof(void*)));
    return RMM_FAILURE ;
  }

  if ( (rInfo->lgevQ = BB_alloc(rInfo->evntQsize,0) ) == NULL )
  {
    LOG_MEM_ERR(tcHandle,methodName,(rInfo->evntQsize*sizeof(void*)));
    return RMM_FAILURE ;
  }

  i = offsetof(ConnInfoRec,ll_next) ;
  if ( (rInfo->sockQ = LL_alloc(i,0) ) == NULL )
  {
    LOG_MEM_ERR(tcHandle,methodName,(MAX_READY_CONNS*sizeof(void*)));
    return RMM_FAILURE ;
  }

  i = offsetof(rStreamInfoRec,ma_next) ; 
  if ( (rInfo->mastQ = LL_alloc(i,0) ) == NULL )
  {
    LOG_MEM_ERR(tcHandle,methodName,(MAX_STREAMS*sizeof(void*)));
    return RMM_FAILURE ;
  }

  i = rInfo->aConfig.MemoryAlertPctHi*rInfo->MaxPacketsAllowed/100 ;
  rc= rInfo->aConfig.MemoryAlertPctLo*rInfo->MaxPacketsAllowed/100 ;
  if ( (rInfo->dataBuffsQ = MM_alloc(rInfo->DataPacketSize,rInfo->dataBuffsQsize,rInfo->MaxPacketsAllowed,i,rc)) == NULL )
  {
    LOG_MEM_ERR(tcHandle,methodName,(rInfo->dataBuffsQsize*sizeof(void*)));
    return RMM_FAILURE ;
  }

  rInfo->recvBuffsQ = rInfo->dataBuffsQ ; 

  if ( (rInfo->nackStrucQ = MM_alloc(sizeof(NackInfoRec),rInfo->nackStrucQsize,0,0,0)) == NULL )
  {
    LOG_MEM_ERR(tcHandle,methodName,(rInfo->nackStrucQsize*sizeof(void*)));
    return RMM_FAILURE ;
  }

  if ( (rInfo->evntStrucQ = MM_alloc(sizeof(EventInfo),rInfo->evntStrucQsize,0,0,0)) == NULL )
  {
    LOG_MEM_ERR(tcHandle,methodName,(rInfo->evntStrucQsize*sizeof(void*)));
    return RMM_FAILURE ;
  }

  for ( i=0 ; i<1024 ; i++ )
  {
    if ( (buff = MM_get_buff(rInfo->dataBuffsQ,0,&iError)) == NULL )
      break ;
    buffs[i] = buff ;
  }

  while ( i-- )
  {
    MM_put_buff(rInfo->dataBuffsQ,buffs[i]) ;
  }

  if ( rmm_cond_init(&rInfo->nakTcw,0) != RMM_SUCCESS )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3165),"%s",
      "The RUM receiver failed to initialize the {0} CondWait variable.","nakTcw");
    return RMM_FAILURE ;
  }

  if ( rmm_cond_init(&rInfo->eaWcw,0) != RMM_SUCCESS )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3165),"%s",
      "The RUM receiver failed to initialize the {0} CondWait variable.","eaWcw");
    return RMM_FAILURE ;
  }

  if ( rmm_cond_init(&rInfo->prWcw,0) != RMM_SUCCESS )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3165),"%s",
      "The RUM receiver failed to initialize the {0} CondWait variable.","prWcw");
    return RMM_FAILURE ;
  }

  if ( rmm_rwlock_init(&rInfo->GlobalSync.rw) != RMM_SUCCESS )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3168),"",
       "The RUM receiver failed to initialize the GlobalSync.rw rwlock variable.");
    return RMM_FAILURE ;
  }

  if ( (rc=pthread_mutex_init(&rInfo->GlobalSync.mutex, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3169),"%s%d",
      "The RUM receiver failed to initialize the {0} pthread mutex variable. The error code is {1}.","GlobalSync.mutex",rc);
    return RMM_FAILURE ;
  }

  if ( (rc=pthread_mutex_init(&rInfo->ppMutex, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3169),"%s%d",
      "The RUM receiver failed to initialize the {0} pthread mutex variable. The error code is {1}.","ppMutex",rc);
    return RMM_FAILURE ;
  }

  if ( (rc=pthread_mutex_init(&rInfo->prMutex, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3169),"%s%d",
      "The RUM receiver failed to initialize the {0} pthread mutex variable. The error code is {1}.","prMutex",rc);
    return RMM_FAILURE ;
  }

  if ( (rc=pthread_mutex_init(&rInfo->maMutex, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3169),"%s%d",
      "The RUM receiver failed to initialize the {0} pthread mutex variable. The error code is {1}.","maMutex",rc);
    return RMM_FAILURE ;
  }

  if ( (rc=pthread_mutex_init(&rInfo->nbMutex, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3169),"%s%d",
      "The RUM receiver failed to initialize the {0} pthread mutex variable. The error code is {1}.","nbMutex",rc);
    return RMM_FAILURE ;
  }

  if ( (rc=pthread_mutex_init(&rInfo->rTasks.visiMutex, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3169),"%s%d",
      "The RUM receiver failed to initialize the {0} pthread mutex variable. The error code is {1}.","visiMutex",rc);
    return RMM_FAILURE ;
  }

  if ( (rc=pthread_mutex_init(&rInfo->deadQlock, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3169),"%s%d",
      "The RUM receiver failed to initialize the {0} pthread mutex variable. The error code is {1}.","deadQlock",rc);
    return RMM_FAILURE ;
  }

  if ( (rc=pthread_mutex_init(&rInfo->pstsQlock, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3169),"%s%d",
      "The RUM receiver failed to initialize the {0} pthread mutex variable. The error code is {1}.","pstsQlock",rc);
    return RMM_FAILURE ;
  }
  LOG_METHOD_EXIT();

  return RMM_SUCCESS ; 
}

int free_recv_data(rmmReceiverRec *rInfo)
{
  int i ; 
  rStreamInfoRec *pst, *qst ; 

  for ( pst=rInfo->deadQfirst ; pst ; pst=qst )
  {
    qst = pst->next ; 
    free(pst) ; 
  }

  for ( pst=rInfo->pstsQfirst ; pst ; pst=qst )
  {
    qst = pst->next ; 
    free(pst) ; 
  }

  if (rInfo->evntStrucQ) MM_free(rInfo->evntStrucQ,1) ;
  if (rInfo->nackStrucQ) MM_free(rInfo->nackStrucQ,1) ;
  if (rInfo->dataBuffsQ) MM_free(rInfo->dataBuffsQ,1) ;
   
  if (rInfo->rmevQ) BB_free(rInfo->rmevQ,1) ;
  if (rInfo->lgevQ) BB_free(rInfo->lgevQ,1) ;
  if (rInfo->sockQ) LL_free(rInfo->sockQ,0) ;
  if (rInfo->rsrvQ) BB_free(rInfo->rsrvQ,1) ;
  if (rInfo->mastQ) LL_free(rInfo->mastQ,0) ;

  for ( i=rInfo->nSCPs-1 ; i>=0 ; i-- )
  {
    if ( rInfo->SCPs[i] )
      free(rInfo->SCPs[i]) ; 
  }

          
  pthread_mutex_destroy(&rInfo->wb_mutex) ;
  rmm_cond_destroy(&rInfo->nakTcw) ;
  rmm_cond_destroy(&rInfo->prWcw) ;
  rmm_cond_destroy(&rInfo->eaWcw) ;
  rmm_rwlock_destroy(&rInfo->GlobalSync.rw) ;
  pthread_mutex_destroy(&rInfo->GlobalSync.mutex) ;
  pthread_mutex_destroy(&rInfo->ppMutex) ;
  pthread_mutex_destroy(&rInfo->prMutex) ;
  pthread_mutex_destroy(&rInfo->maMutex) ;
  pthread_mutex_destroy(&rInfo->nbMutex) ;
  pthread_mutex_destroy(&rInfo->rTasks.visiMutex) ;
  pthread_mutex_destroy(&rInfo->pstsQlock) ;
  pthread_mutex_destroy(&rInfo->deadQlock) ;

  free(rInfo) ;

  return RMM_SUCCESS ; 
}

/*------------------------------------------*/

int start_recv_threads(rmmReceiverRec *rInfo)
{
  int i,rc ; 
  rum_uint64 etime ; 
  TaskInfo task ; 

   int scope ;
  pthread_attr_t attr_proc ;
  pthread_attr_t attr_sys , *pattr_sys=NULL ;
  char errMsg[ERR_MSG_SIZE];
  TCHandle tcHandle = rInfo->tcHandle;

/*@@@@@@@@@@@@@@@@@@@@@*/
   pthread_attr_init(&attr_sys ) ;
   pthread_attr_getscope(&attr_sys,&scope) ;
   if ( scope == PTHREAD_SCOPE_PROCESS )
   {
     if ( pthread_attr_setscope(&attr_sys,PTHREAD_SCOPE_SYSTEM) != 0 )
     {
       pthread_attr_destroy(&attr_sys ) ;
       pthread_attr_init(&attr_proc) ;
       pattr_sys = &attr_proc ; 
       llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5177),"%p",
         "The default thread scope is PTHREAD_SCOPE_PROCESS ({0}). The thread model is M:1.",pattr_sys);
     }
     else
     {
       pattr_sys = &attr_sys ;
       llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5178),"%p",
         "The default thread scope is PTHREAD_SCOPE_PROCESS ({0}). The thread model is M:N.",pattr_sys);
     }
   }
   else
   {
     pattr_sys = &attr_sys ;
     pthread_attr_init(&attr_proc) ;
     if ( pthread_attr_setscope(&attr_proc,PTHREAD_SCOPE_PROCESS) != 0 )
     {
       llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5179),"",
         "The default thread scope is PTHREAD_SCOPE_SYSTEM. The thread model is 1:1.");
     }
     else
     {
       llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5180),"",
         "The default thread scope is PTHREAD_SCOPE_SYSTEM. The thread model is M:N.");
     }
     pthread_attr_destroy(&attr_proc) ;
   }

   if ( pattr_sys && rInfo->aConfig.ThreadPriority_rx )
   {
     if ( rmm_set_thread_priority(pattr_sys, SCHED_RR,rInfo->aConfig.ThreadPriority_rx, errMsg, ERR_MSG_SIZE) == RMM_FAILURE )
       llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3181),"%d%s",
         "RUM failed to set the thread priority to {0}. The error is: {1}", rInfo->aConfig.ThreadPriority_rx,errMsg);
   }
   if ( pattr_sys && rInfo->aConfig.ThreadAffinity_rx > 0 )
   {
    #if USE_AFF
     if ( rmm_set_thread_affinity(pattr_sys, 0, rInfo->aConfig.ThreadAffinity_rx, errMsg, ERR_MSG_SIZE) == RMM_FAILURE )
     {
       pattr_sys = NULL ; 
       llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(5182),"%llx%s",
         "RUM failed to set the thread affinity to {0}. The error is: {1}", rInfo->aConfig.ThreadAffinity_rx,errMsg);
     }
    #else
     llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4183),"",
       "The ThreadAffinity configuration parameter is not supported.");
    #endif
   }

   if ( pattr_sys && rInfo->aConfig.ThreadStackSize > 0 )
   {
     if ( rmm_set_thread_stacksize(pattr_sys, rInfo->aConfig.ThreadStackSize, errMsg, ERR_MSG_SIZE) == RMM_FAILURE )
     {
       llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_WARN, MSG_KEY(4098), "%d%s%s",
        "LLM failed to set the thread stack size to {0} for thread {1}. The error is {2}.",
         rInfo->aConfig.ThreadStackSize, "RUM Rx", errMsg);
     }
   }
/*@@@@@@@@@@@@@@@@@@@@@*/

  i=0 ; 
  memset(&task,0,sizeof(TaskInfo)) ; 

  task.taskData = (void *)rInfo ; 
  task.taskCode = setNTC ; 
  i |= TT_add_task(&rInfo->rTasks,&task) ; 
 /*
  task.taskData = (void *)rInfo ; 
  task.taskCode = signalNG ; 
  i |= TT_add_task(&rInfo->rTasks,&task) ; 
 */
  task.taskData = (void *)rInfo ; 
  task.taskCode = MemMaint ; 
  i |= TT_add_task(&rInfo->rTasks,&task) ; 
  task.taskData = (void *)rInfo ; 
  task.taskCode = calcTP ; 
  i |= TT_add_task(&rInfo->rTasks,&task) ; 
  task.taskData = (void *)rInfo ;
  task.taskCode = chkSLB ;
  i |= TT_add_task(&rInfo->rTasks,&task) ;
  task.taskData = (void *)rInfo ; 
  task.taskCode = calcBW ; 
  i |= TT_add_task(&rInfo->rTasks,&task) ; 
  if ( rInfo->aConfig.SnapshotCycleMilli_rx > 0 )
  {
    task.nextTime = rmmTime(NULL,NULL) + 1000 ;
    task.taskData = (void *)rInfo ; 
    task.taskCode = callSS ; 
    i |= TT_add_task(&rInfo->rTasks,&task) ; 
  }
 /**/
  task.taskData = (void *)rInfo ; 
  task.taskCode = callCHBTO ; 
  i |= TT_add_task(&rInfo->rTasks,&task) ; 
 /**/
  task.taskData = (void *)rInfo ; 
  task.taskCode = ConnStreamReport ; 
  i |= TT_add_task(&rInfo->rTasks,&task) ; 
  if ( i )
  {
    TT_del_all_tasks(&rInfo->rTasks) ; 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3184),"",
   "The RUM receiver failed to create tasks for the TaskTimer thread.");
    return RMM_FAILURE ;
  }
  rInfo->rTasks.reqSleepTime = rInfo->aConfig.TaskTimerCycle ;

  if ( (rc=pthread_create(&rInfo->ntThreadID, NULL, TaskTimer, (void *)&rInfo->rTasks)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3185),"%d",
      "The RUM receiver failed to create the TaskTimer thread. The error code is {0}.", rc);
    return RMM_FAILURE ;
  }
  for ( i=0 ; i<10 && rInfo->rTasks.threadId==0 ; i++ )
    tsleep(0,rInfo->aConfig.BaseWaitNano) ;
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5186),"%s%llu","The {0} thread was created. Thread id: {1}",
    "TaskTimer",LLU_VALUE(rInfo->rTasks.threadId));


  if ( (rc=pthread_create(&rInfo->eaThreadID, NULL, EventAnnouncer, (void *)rInfo)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3187),"%s%d",
      "The RUM receiver failed to create the {0} thread. The error code is {1}.", "EventAnnouncer",rc);
    return RMM_FAILURE ;
  }

  rInfo->GlobalSync.chUP = -2 ; 
  if ( (rc=pthread_create(&rInfo->chThreadID, NULL, ConnectionHandler, (void *)rInfo->gInfo)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3187),"%s%d",
      "The RUM receiver failed to create the {0} thread. The error code is {1}.", "ConnectionHandler",rc);
    return RMM_FAILURE ;
  }

  for ( i=0 ; i<rInfo->NumPPthreads && i<MAX_THREADS ; i++ )
  {
    if ( (rc=pthread_create(&rInfo->ppThreadID[i], pattr_sys, PacketProcessor_tcp, (void *)rInfo)) != 0 && pattr_sys )
          rc=pthread_create(&rInfo->ppThreadID[i], NULL     , PacketProcessor_tcp, (void *)rInfo) ; 
    if ( rc )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3188),"%s%d",
         "The RUM receiver failed to create the {0} threads. The error code is {1}.", "PacketProcessor",rc);
      return RMM_FAILURE ;
    }
  }

  if ( (rc=pthread_create(&rInfo->ngThreadID, NULL, NAKGenerator, (void *)rInfo)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3189),"%s%d",
       "The RUM receiver failed to create the {0} thread. The error code is {1}.", "NAKGenerator",rc);
    return RMM_FAILURE ;
  }

  for ( i=0 ; i<rInfo->NumMAthreads && i<MAX_THREADS ; i++ )
  {
    if ( (rc=pthread_create(&rInfo->maThreadID[i], pattr_sys, MessageAnnouncer, (void *)rInfo)) != 0 && pattr_sys )
          rc=pthread_create(&rInfo->maThreadID[i], NULL     , MessageAnnouncer, (void *)rInfo) ; 
    if ( rc )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3190),"%s%d",
         "The RUM receiver failed to create the {0} threads. The error code is {1}.", "MessageAnnouncer",rc); 
      return RMM_FAILURE ;
    }
  }
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(3191),"","Start waiting for threads to start up.");
  etime = rmmTime(NULL,NULL) + rInfo->aConfig.BindRetryTime + 2000 ; 
  do
  {
    pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
    i =   (rInfo->GlobalSync.maUP == rInfo->NumMAthreads &&
           rInfo->GlobalSync.prUP == rInfo->NumPRthreads &&
           rInfo->GlobalSync.ppUP == rInfo->NumPPthreads &&
           rInfo->GlobalSync.eaUP == 1 &&
           rInfo->GlobalSync.chUP == 1 &&
           rInfo->rTasks.ThreadUP == 1 &&
           rInfo->GlobalSync.ngUP == 1 ) ;
    pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
    if ( i )
      return RMM_SUCCESS ; 
    tsleep(0,rInfo->aConfig.BaseWaitNano) ;
  } while ( rmmTime(NULL,NULL) < etime && rInfo->GlobalSync.chUP != 0 ) ; 
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(3192),"","Stop waiting for threads to start up.");

  return RMM_FAILURE ;
}

int stop_recv_threads(rmmReceiverRec *rInfo)
{
  int i,n ; 
  TCHandle tcHandle = rInfo->tcHandle;
  rInfo->GlobalSync.goDN = 1 ;
  pthread_mutex_lock(&rInfo->rTasks.visiMutex) ; 
  rInfo->rTasks.goDown = 1 ; 
  pthread_mutex_unlock(&rInfo->rTasks.visiMutex) ; 
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(3193),"","Start waiting for threads to stop.");
  tsleep(0,rInfo->aConfig.BaseWaitNano) ;

  n = 1000 ; 
  while ( n-- )
  {
    pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
    i =   (rInfo->GlobalSync.maUP == 0 &&
           rInfo->GlobalSync.prUP == 0 &&
           rInfo->GlobalSync.ppUP == 0 &&
           rInfo->GlobalSync.eaUP == 0 &&
           rInfo->GlobalSync.chUP == 0 &&
           rInfo->rTasks.ThreadUP == 0 &&
           rInfo->GlobalSync.ngUP == 0 ) ;
    pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
    if ( i )
      break ; 

    LL_signalE(rInfo->sockQ) ; 
    LL_signalF(rInfo->sockQ) ; 
    rmm_cond_signal(&rInfo->eaWcw,1) ;
    rmm_cond_signal(&rInfo->prWcw,1) ;
    wakeMA(rInfo,NULL) ; 
    rmm_cond_signal(&rInfo->nakTcw,1) ;
    tsleep(0,rInfo->aConfig.BaseWaitNano) ;
  }

  if ( n <= 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(3194),"","Start canceling threads.");
  }

  
  if ( rInfo->GlobalSync.ppUP )
  {
    for ( i=0 ; i<rInfo->NumPPthreads  ; i++ )
    {
      cancel_recv_thread(rInfo,"PacketProcessor",rInfo->ppThreadID[i]) ; 
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5067),"%s%llu",
         "The {0}({1}) thread was stopped.",
         "PacketProcessor", THREAD_ID(rInfo->ppThreadID[i]));
    }
  }
  else
    for ( i=0 ; i<rInfo->NumPPthreads  ; i++ )
      detach_recv_thread(rInfo,"PacketProcessor",rInfo->ppThreadID[i]) ; 

  
  if ( rInfo->GlobalSync.ngUP )
  {
    cancel_recv_thread(rInfo,"NAKGenerator",rInfo->ngThreadID) ; 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5067),"%s%llu",
      "The {0}({1}) thread was stopped.","NAKGenerator", THREAD_ID(rInfo->ngThreadID));
  }
  else
    detach_recv_thread(rInfo,"NAKGenerator",rInfo->ngThreadID) ; 

  if ( rInfo->rTasks.ThreadUP )
  {
    cancel_recv_thread(rInfo,"TaskTimer",rInfo->ntThreadID) ; 
  }
  else
    detach_recv_thread(rInfo,"TaskTimer",rInfo->ntThreadID) ; 

  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5197),"%s%llu",
     "The {0} thread stopped. Thread id: {1}","TaskTimer", THREAD_ID(rInfo->ntThreadID));
  
  if ( rInfo->GlobalSync.maUP )
  {
    for ( i=0 ; i<rInfo->NumMAthreads  ; i++ )
    {
      cancel_recv_thread(rInfo,"MessageAnnouncer",rInfo->maThreadID[i]) ; 
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5067),"%s%llu",
         "The {0}({1}) thread was stopped.","MessageAnnouncer", THREAD_ID(rInfo->maThreadID[i]));
    }
  }
  else
    for ( i=0 ; i<rInfo->NumMAthreads  ; i++ )
    {
      detach_recv_thread(rInfo,"MessageAnnouncer",rInfo->maThreadID[i]) ; 
    }

  
  if ( rInfo->GlobalSync.eaUP )
  {
    cancel_recv_thread(rInfo,"EventAnnouncer",rInfo->eaThreadID) ; 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5199),"%s%llu",
         "The {0} thread stopped. Thread id: {1}","EventAnnouncer", THREAD_ID(rInfo->ngThreadID));
  }
  else
    detach_recv_thread(rInfo,"EventAnnouncer",rInfo->eaThreadID) ; 

  if ( rInfo->GlobalSync.chUP )
  {
    cancel_recv_thread(rInfo,"ConnectionHandler",rInfo->chThreadID) ; 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5200),"%s%llu",
        "The {0} thread stopped. Thread id: {1}","ConnectionHandler", THREAD_ID(rInfo->chThreadID));
    cip_remove_all_conns(rInfo->gInfo) ; 
  }
  else
    detach_recv_thread(rInfo,"ConnectionHandler",rInfo->chThreadID) ; 
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5087),"",
      "All RUM receiver threads have stopped.");


  return RMM_SUCCESS ;
}

int detach_recv_thread(rmmReceiverRec *rInfo, const char *name, pthread_t id)
{
  int rc = 0 ;
  TCHandle tcHandle = rInfo->tcHandle;
  if (id){
   rc=pthread_detach(id);
  }
  if(rc){
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3080),"%s%d",
      "RUM failed to detach thread {0}. The error code is {1}.",
    name,rc);
    return RMM_FAILURE;
  }else{
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6106),"%s%llu",
      "Thread {0} with thread id {1} was detached.", name,THREAD_ID(id));
    return RMM_SUCCESS;
  }
}

int cancel_recv_thread(rmmReceiverRec *rInfo, const char *name, pthread_t id)
{
  int rc ;
  void *result ;
  TCHandle tcHandle = rInfo->tcHandle;

  result = NULL ;

  if ( (rc=pthread_cancel(id)) != 0 )
  {
    if ( rc == ESRCH )
    {
      detach_recv_thread(rInfo,name,id) ;
      return RMM_SUCCESS ;
    }
    else
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3081),"%s%llu%d",
        "The RUM receiver failed to cancel the {0} thread with thread id {1}. The error code is {2}.",
      name,THREAD_ID(id),rc);
      return RMM_FAILURE ;
    }
  }
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6107),"%s%llu",
    "Thread {0} with thread id {1} was canceled.", name,THREAD_ID(id));

  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6108),"%s%llu",
    "Before join to thread {0} ({1}).", name,THREAD_ID(id));
  if ( (rc=pthread_join(id,&result)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3082),"%s%llu%d","The RUM receiver failed to join the {0} thread with thread id {1}. The error code is {2}.",
        name,THREAD_ID(id),rc);
    return RMM_FAILURE ;
  }
  if ( result != PTHREAD_CANCELED )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3083),"%s%llu%llx","The RUM receiver failed to cancel the {0} thread with thread id {1}. The error code is {2}.",
        name,THREAD_ID(id),LLU_VALUE(result));
    return RMM_FAILURE ;
  }
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6109),"%s%llu",
    "After join to thread {0} ({1}).", name,THREAD_ID(id));

  return RMM_SUCCESS ;
}

static int  rmmR_chk_instance(int instance, TCHandle *tcHandle, int *ec) 
{
  rmmRmutex_lock() ;

  if ( instance < 0  ||
       instance >= rNumInstances ||
       rInstances[instance] == NULL )
  {
    if ( ec ) *ec = RUM_L_E_INSTANCE_INVALID ; 
    rmmRmutex_unlock() ;
    return -1 ;
  }
  *tcHandle = rInstances[instance]->tcHandle;
  return instance ;

}
int rmmR_chk_topic(rmmReceiverRec *rInfo, int topic, const char *methodName, int *ec)
{
  if ( topic < 0 ||
       topic >= rInfo->rNumTopics ||
       rInfo->rTopics[topic] == NULL ||
       rInfo->rTopics[topic]->topic_id != topic )
  {
    if ( ec ) *ec = RUM_L_E_QUEUE_INVALID ; 
    llmSimpleTraceInvoke(rInfo->tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3209),"%d%s",
      "The queue {0} is not valid when calling the {1} method.",topic,methodName);
    rmmRmutex_unlock() ;
    return -1 ;
  }
  return topic ;

}

int is_conn_valid(rumInstanceRec *gInfo,rmmReceiverRec *rInfo,ConnInfoRec *cInfo,int invalid)
{
  TCHandle tcHandle = rInfo->tcHandle;
  if ( invalid || cInfo->is_invalid )
  {
    if ( !(cInfo->is_invalid&R_INVALID) )
    {
      cInfo->is_invalid |= R_INVALID ; 
      {
       #if USE_SSL
        if ( gInfo->use_ssl && cInfo->sslInfo->ssl )
        {
          SSL_shutdown(cInfo->sslInfo->ssl) ; 
        }
       #endif
        shutdown(cInfo->sfd,SHUT_RD) ; 
       /* defer to delete, since it might be reallocated!
        if ( cInfo->rdInfo.buffer )
        {
          free(cInfo->rdInfo.buffer) ; 
          cInfo->rdInfo.buffer = NULL ; 
        }
       */
      }
      rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ;
      if ( !(cInfo->is_invalid&A_INVALID) && !cInfo->ev_sent )    
        send_conn_close_event(gInfo,cInfo) ; 
      rmm_rwlock_wrlock(&rInfo->GlobalSync.rw) ;
      delete_conn_streams(gInfo,rInfo,cInfo) ; 
      rmm_rwlock_wrunlock(&rInfo->GlobalSync.rw) ;
      rmm_rwlock_rdlock(&gInfo->ConnSyncRW) ;
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(5210),"%s%d%x",
        "The RUM receiver connection {0} is no longer valid on the receiver side (c_inv={1} ,is_inv={2}).",cInfo->req_addr,invalid,cInfo->is_invalid);
      if ( !(cInfo->is_invalid&P_INVALID) )
      {
        LL_lock(rInfo->sockQ) ; 
        if ( !cInfo->ppIn )
          cInfo->is_invalid |= P_INVALID ;
        LL_unlock(rInfo->sockQ) ; 
      }
    }
    if ( (cInfo->is_invalid&T_INVALID) && (cInfo->is_invalid&P_INVALID) )
    {                          
      shutdown(cInfo->sfd,SHUT_WR) ; 
      delete_closed_connection(gInfo,cInfo) ; 
    }
    else
      send_FO_signal(gInfo,1) ; 
    return 0 ; 
  }

  return ( cInfo->state == RUM_CONNECTION_STATE_ESTABLISHED ) ;
}

void delete_conn_streams(rumInstanceRec *gInfo,rmmReceiverRec *rInfo,ConnInfoRec *cInfo)
{
  rTopicInfoRec  *ptp=NULL ;
  rStreamInfoRec *pst, *nst ; 
  TCHandle tcHandle = rInfo->tcHandle;

  for ( pst=rInfo->firstStream ; pst ; pst=nst )
  {
    nst = pst->next ;
    if ( pst->cInfo->connection_id == cInfo->connection_id )
    {
      char* eventDescription = "Connection closed for the stream.";
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5211),"%s%s%llu",
        "The connection for the stream {0} was closed. (cid={1}) (nmsg={2}). ", pst->sid_str,cInfo->req_addr,UINT_64_TO_LLU(pst->TotMsgsOut));
      ptp   = rInfo->rTopics[pst->topic_id] ;
      stop_stream_service(pst);
      if ( ptp->on_event )
      {
        void *EvPrms[2] ; EvPrms[0]=eventDescription;
        raise_stream_event(pst, RUM_STREAM_BROKE, EvPrms, 1) ; 
      }
      remove_stream(pst) ;
      delete_stream(pst) ;
    }
  }
  del_scp(rInfo,cInfo->connection_id,0);
  clear_rejected_streams(rInfo,cInfo->connection_id,(1|2|4)) ; 
}

void delete_closed_connection(rumInstanceRec *gInfo,ConnInfoRec *cInfo)
{
  int i ; 

  rmm_rwlock_r2wlock(&gInfo->ConnSyncRW) ; 
  i = update_conn_info(gInfo, cInfo, 0);
  gInfo->connUpd = 1 ; 
  rmm_rwlock_w2rlock(&gInfo->ConnSyncRW) ; 
  if ( i == 0 )
  {
    {
     #if USE_SSL
      if ( gInfo->use_ssl && cInfo->sslInfo->ssl )
      {
        SSL_free    (cInfo->sslInfo->ssl) ; 
        pthread_mutex_destroy(cInfo->sslInfo->lock) ;
      }
     #endif
      shutdown(cInfo->sfd,SHUT_RDWR) ;
      closesocket(cInfo->sfd) ; 
      if ( cInfo->rdInfo.buffer )
        free(cInfo->rdInfo.buffer) ; 
      if ( cInfo->wrInfo.buffer )
        free(cInfo->wrInfo.buffer) ; 
    }
    pthread_mutex_destroy(&cInfo->mutex) ;

    if ( cInfo->init_here )
    {
      if ( gInfo->free_callback )
        PutFcbEvent(gInfo, gInfo->free_callback, cInfo->conn_listener->user) ;
      PutFcbEvent(gInfo, ea_free_ptr, cInfo->conn_listener) ;
    }
    else
    if ( cInfo->conn_listener )
    {
      pthread_mutex_lock(&gInfo->ConnectionListenersMutex) ; 
      cInfo->conn_listener->n_active-- ; 
      if ( !cInfo->conn_listener->is_valid &&
            cInfo->conn_listener->n_cip <= 0 &&
            cInfo->conn_listener->n_active <= 0 )
      {
        if ( gInfo->free_callback )
          PutFcbEvent(gInfo, gInfo->free_callback, cInfo->conn_listener->user) ;
        PutFcbEvent(gInfo, ea_free_ptr, cInfo->conn_listener) ;
      }
      pthread_mutex_unlock(&gInfo->ConnectionListenersMutex) ; 
    }
    if ( cInfo->sendNacksQ )
    {
      char *buff ; 
      while ( (buff=BB_get_buff(cInfo->sendNacksQ,0)) != NULL )
        MM_put_buff(gInfo->nackBuffsQ,buff);
      BB_free(cInfo->sendNacksQ,1) ; 
    }
    free(cInfo) ; 
  }
}

void add_ready_connection(rumInstanceRec *gInfo,ConnInfoRec *cInfo)
{
  rmm_rwlock_wrlock(&gInfo->ConnSyncRW) ; 
  update_conn_info(gInfo, cInfo, 1);
  gInfo->connUpd = 1 ; 
  rmm_rwlock_wrunlock(&gInfo->ConnSyncRW) ; 
}

void send_conn_close_event(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  rumConnectionEvent ev ; 

  if ( cInfo->ev_sent )
    return ; 
  cInfo->ev_sent = 1 ; 

  if ( !cInfo->conn_listener->is_valid )
    return ; 

  memset(&ev,0,sizeof(ev)) ; 
  cInfo->apiInfo.connection_state  = RUM_CONNECTION_STATE_CLOSED ; 
  memcpy(&ev.connection_info,&cInfo->apiInfo,sizeof(rumConnection)) ;
  if ( cInfo->to_detected )
    ev.type = RUM_CONNECTION_HEARTBEAT_TIMEOUT ; 
  else
    ev.type = RUM_CONNECTION_BROKE ; 
 #if USE_EA
  PutConEvent(gInfo, &ev, cInfo->conn_listener->on_event, cInfo->conn_listener->user) ; 
 #else
  cInfo->conn_listener->on_event(&ev,cInfo->conn_listener->user) ; 
 #endif
}

void delete_all_connections(rumInstanceRec *gInfo)
{
  ConnInfoRec *cInfo ; 

  while ( gInfo->firstConnection )
  {
    cInfo = gInfo->firstConnection ; 
    update_conn_info(gInfo, cInfo, 0);
    {
      shutdown(cInfo->sfd,SHUT_RDWR) ;
      closesocket(cInfo->sfd) ; 
      if ( cInfo->rdInfo.buffer )
        free(cInfo->rdInfo.buffer) ; 
      if ( cInfo->wrInfo.buffer )
        free(cInfo->wrInfo.buffer) ; 
    }
    pthread_mutex_destroy(&cInfo->mutex) ;
    if ( cInfo->sendNacksQ )
    {
      char *buff ; 
      while ( (buff=BB_get_buff(cInfo->sendNacksQ,0)) != NULL )
        MM_put_buff(gInfo->nackBuffsQ,buff);
      BB_free(cInfo->sendNacksQ,1) ; 
    }
    free(cInfo) ; 
  }
}

void process_tx_stream_report(rmmReceiverRec *rInfo, rumConnectionID_t cid, uint8_t *packet)
{
  int i,j,n, nsid ; 
  uint8_t *bptr ; 
  //rumInstanceRec *gInfo ; 
  rTopicInfoRec  *ptp=NULL ;
  rStreamInfoRec *pst, *nst ; 
  rmmStreamID_t sids[MAX_STREAMS] ; 
  TCHandle tcHandle = rInfo->tcHandle;
  char* eventDescription;
  //gInfo = rInfo->gInfo ; 

  bptr = packet + sizeof(pgmHeaderCommon) ;
  NgetInt(bptr,n) ; 

  nsid = 0 ; 
  for ( pst=rInfo->firstStream ; pst ; pst=nst )
  {
    nst = pst->next ;
    if ( pst->cInfo->connection_id == cid )
    {
      bptr = packet + (sizeof(pgmHeaderCommon) + INT_SIZE) ; 
      for ( j=0 ; j<n ; j++ )
      {
        if ( memcmp(bptr,&pst->sid,sizeof(rmmStreamID_t)) == 0 )
          break ; 
        bptr += sizeof(rmmStreamID_t) ; 
      }
      if ( j<n )
        continue ; 

      if ( !XltY(pst->rxw_lead,SQ_get_tailSN(pst->dataQ,1)) )
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5212),"%s%s",
          "The stream {0} is not defined in the Transmitter Stream Report (TX_STR_REP) for connection {1}.",pst->sid_str,pst->cInfo->req_addr );
        continue ; 
      }
      sids[nsid++] = pst->sid ; 
    }
  }
  if ( nsid )
  {
    rmm_rwlock_r2wlock(&rInfo->GlobalSync.rw) ;
    for ( i=0 ; i<nsid ; i++ )
    {
      if ( (pst = find_stream(rInfo,sids[i])) == NULL )
        continue ; 
      eventDescription = "Stream is not defined on the transmitter."; 
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5213),"%s%s",
        "The stream {0} is not defined in the Transmitter Stream Report (TX_STR_REP) for connection {1}.",pst->sid_str,pst->cInfo->req_addr );
      stop_stream_service(pst) ; 
      ptp   = rInfo->rTopics[pst->topic_id] ;
      if ( ptp->on_event )
      {
        void *EvPrms[2] ; EvPrms[0]=eventDescription;
        raise_stream_event(pst, RUM_STREAM_NOT_PRESENT, EvPrms, 1) ; 
      }
      remove_stream(pst) ;
      delete_stream(pst) ;
    }
    rmm_rwlock_w2rlock(&rInfo->GlobalSync.rw) ;
  }

  pthread_mutex_lock(&rInfo->nbMutex) ;
  for ( i=rInfo->nbad-1 ; i>=0 ; i-- )
  {
    if ( rInfo->RejectedStreams[i].cid == cid )
    {
      bptr = packet + (sizeof(pgmHeaderCommon) + INT_SIZE) ; 
      for ( j=0 ; j<n ; j++ )
      {
        if ( memcmp(bptr,&rInfo->RejectedStreams[i].sid,sizeof(rmmStreamID_t)) == 0 )
          break ; 
        bptr += sizeof(rmmStreamID_t) ; 
      }
      if ( j>=n )
        rInfo->RejectedStreams[i] = rInfo->RejectedStreams[--rInfo->nbad] ; 
    }
  }
  pthread_mutex_unlock(&rInfo->nbMutex) ;

  for ( i=rInfo->nSCPs-1 ; i>=0 ; i-- )
  {
    if ( rInfo->SCPs[i]->cid == cid )
    {
      bptr = packet + (sizeof(pgmHeaderCommon) + INT_SIZE) ; 
      for ( j=0 ; j<n ; j++ )
      {
        if ( memcmp(bptr,&rInfo->SCPs[i]->sid,sizeof(rmmStreamID_t)) == 0 )
          break ; 
        bptr += sizeof(rmmStreamID_t) ; 
      }
      if ( j>=n )
      {
        free(rInfo->SCPs[i]) ; 
        rInfo->SCPs[i] = rInfo->SCPs[--rInfo->nSCPs] ; 
      }
    }
  }

}

int extractPacket(rmmReceiverRec *rInfo, ConnInfoRec *cInfo, char *buff, uint32_t bufflen)
{
  int nask , nget , PrefixSize , iError ; 
  uint32_t packlen , n ; 
  char *packet ; 
  rumHeader *phrm ;
  pgmHeaderCommon *phc ; 
  rumInstanceRec *gInfo ; 
  char errMsg[ERR_MSG_SIZE];
  const char* methodName = "extractPacket";
  TCHandle tcHandle = rInfo->tcHandle;

  gInfo = rInfo->gInfo ; 
  PrefixSize = sizeof(rumHeader) ;
  bufflen -= PrefixSize ; 

  for ( ; ; )
  {
    for ( ; ; )
    {
      if ( cInfo->rdInfo.offset >= cInfo->rdInfo.reqlen )
      {
        cInfo->last_r_time = rInfo->CurrentTime ; 
        NgetInt(cInfo->rdInfo.bptr,packlen) ; 

        cInfo->rdInfo.reqlen += packlen ; 
        if ( cInfo->rdInfo.offset >= cInfo->rdInfo.reqlen )
        {
          cInfo->rdInfo.reqlen += INT_SIZE ; 
          cInfo->nPacks++ ;

          phc = (pgmHeaderCommon *)cInfo->rdInfo.bptr ; 
          switch( phc->type )
          {
            case PGM_TYPE_ODATA :
            default :
              if ( packlen > bufflen )
              {
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4214),"%s%llu%d%d",
                  "The size of the packet ({3} bytes) received on connection {1} from {0} is larger than the configured packet size ({2} bytes).  The connection {1} will be closed.",cInfo->req_addr,cInfo->connection_id,bufflen,packlen );
                if(llmIsTraceAllowed(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7214)))
                {
                  char dump[512];
                  dumpBuff(methodName,dump,256,cInfo->rdInfo.buffer,64);
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7214),"%s%d%d%s",
                    "The RUM receiver invalidates the connection {0} (bufflen: {1} < {2}). dumpBuff={3}",
                    cInfo->req_addr,bufflen,packlen,dump);
                }
                return -1 ; 
              }
              packet = buff ; 
              phrm = (rumHeader *)packet ; 
              phrm->PackOff = PrefixSize ; 
              phrm->PackLen = packlen ; 
              phrm->ConnInd = 0 ; 
              phrm->ConnId  = cInfo->connection_id ; 
              memcpy(packet+PrefixSize,cInfo->rdInfo.bptr,packlen) ; 
              cInfo->rdInfo.bptr += packlen ; 
              return 1 ; 
            case RUM_CON_HB :
              cInfo->rdInfo.bptr   += packlen ; 
              break ; 
            case RUM_RX_STR_REP :
              if ( packlen+INT_SIZE > sizeof(cInfo->rx_str_rep_rx_packet) )
              {
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4215),"%s%d%d",
                "The RUM connection {0} is no longer valid on the receiver side (rx_str_rep_rx_packet: {1} < {2}).",cInfo->req_addr,(int)sizeof(cInfo->rx_str_rep_rx_packet),(int)(packlen+INT_SIZE) );
                if(llmIsTraceAllowed(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7215)))
                {
                  char dump[512];
                  dumpBuff(methodName,dump,256,cInfo->rdInfo.buffer,64);
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7215),"%s%d%d%s",
                    "The RUM receiver invalidates the connection {0} (rx_str_rep_rx_packet: {1} < {2}). dumpBuff={3}",
                    cInfo->req_addr,(int)sizeof(cInfo->rx_str_rep_rx_packet),(int)(packlen+INT_SIZE),dump);
                }
                return -1 ; 
              }
              if ( pthread_mutex_trylock(&cInfo->mutex) == 0 )
              {
                memcpy(cInfo->rx_str_rep_rx_packet,cInfo->rdInfo.bptr-INT_SIZE,packlen+INT_SIZE) ; 
                cInfo->str_rep_rx_pending = 1 ; 
                pthread_mutex_unlock(&cInfo->mutex) ;
              }
              cInfo->rdInfo.bptr   += packlen ; 
              break ; 
            case PGM_TYPE_NAK :
              if ( packlen+INT_SIZE > NACK_BUFF_SIZE )
              {
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4237),"%s%d%d",
                "The RUM connection is no longer valid {0} on the receiver side (nack_packet: {1} < {2}).",cInfo->req_addr,NACK_BUFF_SIZE,(int)(packlen+INT_SIZE));
                if(llmIsTraceAllowed(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7237)))
                {
                  char dump[512];
                  dumpBuff(methodName,dump,256,cInfo->rdInfo.buffer,128);
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7237),"%s%d%d%s",
                    "The RUM receiver invalidates the connection {0} (nack_packet: {1} < {2}). dumpBuff={3}",
                    cInfo->req_addr,NACK_BUFF_SIZE,(int)(packlen+INT_SIZE),dump);
                }
                return -1 ; 
              }
              while ( (packet = (char *)MM_get_buff(gInfo->nackBuffsQ,4,&iError)) == NULL )
              {
                if ( rInfo->GlobalSync.goDN )
                  return -1 ; 
              }
              memcpy(packet,cInfo->rdInfo.bptr-INT_SIZE,packlen+INT_SIZE) ; 
              cInfo->rdInfo.bptr   += packlen ; 
        
              BB_lock(gInfo->recvNacksQ) ; 
              while ( BB_put_buff(gInfo->recvNacksQ,packet,0) == NULL )
              {
                if ( rInfo->GlobalSync.goDN )
                {
                  BB_unlock(gInfo->recvNacksQ) ; 
                  MM_put_buff(gInfo->nackBuffsQ,packet) ;
                  return -1 ; 
                }
                BB_waitF(gInfo->recvNacksQ) ; 
              }
              BB_signalE(gInfo->recvNacksQ) ; 
              BB_unlock(gInfo->recvNacksQ) ; 
              break ; 
          }
        }
        else
        {
          cInfo->rdInfo.bptr -= INT_SIZE ; 
          if ( cInfo->rdInfo.buflen < cInfo->rdInfo.reqlen )
          {
            if ( packlen > cInfo->rdInfo.buflen )
            {
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4216),"%s%d%d",
                  "The RUM connection {0} is no longer valid on the receiver side (cInfo->rdInfo.bufflen: {1} < {2}).",cInfo->req_addr,cInfo->rdInfo.buflen,packlen);
              if(llmIsTraceAllowed(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7216)))
              {
                char dump[512];
                dumpBuff(methodName,dump,256,cInfo->rdInfo.buffer,128);
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7216),"%s%d%d%s",
                  "The RUM receiver invalidates the connection {0} (cInfo->rdInfo.bufflen: {1} < {2}). dumpBuff={3}",
                  cInfo->req_addr,cInfo->rdInfo.buflen,packlen,dump);
              }
              return -1 ; 
            }
          }
          else
          {
            cInfo->rdInfo.reqlen -= packlen ; 
          }
          break ; 
        }
      }
      else
      {
        break ; 
      }
    }
    if ( !cInfo->use_ib && !cInfo->use_shm )
    {
      nask = cInfo->rdInfo.buflen-cInfo->rdInfo.offset ; 
      if ( nask < rInfo->DataPacketSize )
      {
        n = (int)(cInfo->rdInfo.bptr - cInfo->rdInfo.buffer) ; 
        if ( n >= cInfo->rdInfo.offset-n )
        {
          memcpy(cInfo->rdInfo.buffer,cInfo->rdInfo.bptr,cInfo->rdInfo.offset-n) ; 
          cInfo->rdInfo.bptr = cInfo->rdInfo.buffer ; 
          cInfo->rdInfo.offset -= n ; 
          cInfo->rdInfo.reqlen = INT_SIZE ; 
        }
        else
        if ( nask <= 0 && n > 0 )
        {
          memmove(cInfo->rdInfo.buffer,cInfo->rdInfo.bptr,cInfo->rdInfo.offset-n) ; 
          cInfo->rdInfo.bptr = cInfo->rdInfo.buffer ; 
          cInfo->rdInfo.offset -= n ; 
          cInfo->rdInfo.reqlen = INT_SIZE ; 
        }
      }
    }

    cInfo->nCalls++ ; 
    nask = cInfo->rdInfo.buflen-cInfo->rdInfo.offset ; 
    nget = rmm_read(cInfo, cInfo->rdInfo.buffer+cInfo->rdInfo.offset, nask, 0 , &iError, errMsg) ; 
    if ( nget > 0 )
    {
     #if MAKE_HIST
      rInfo->hist[nget]++;
     #endif
      cInfo->nReads++ ; 
      cInfo->nBytes += nget ; 
      cInfo->rdInfo.offset += nget ; 
      continue ; 
    }
    if ( nget < 0 )
    {
      int rc = rmmErrno ; 
      if ( nget == -1 )
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4218),"%s%d%s",
          "The RUM connection {0} is no longer valid on the receiver side (nget<0: rc={1}, errMsg: {2}).",cInfo->req_addr,rc,errMsg);
      }
      else
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4219),"%s%d%d",
          "The RUM connection {0} is no longer valid on the receiver side (nget=0 => EOF detected => other side closed connection) {1} {2}).",cInfo->req_addr,cInfo->rdInfo.buflen,cInfo->rdInfo.offset);
      }
      return -1 ; 
    }
    return 0 ; 
  }
}

int doSelect(rmmReceiverRec *rInfo)
{
  SOCKET sfd , np1=0 ; 
  int nr , nOk=0 ; 
  int iError , max_read_size;
 #if !USE_POLL
  fd_set rfds ; 
  struct timeval tv ; 
 #endif
  const char* methodName = "doSelect";

  rumInstanceRec *gInfo ; 
  ConnInfoRec    *cInfo , *nextC;
  TCHandle tcHandle = rInfo->tcHandle;

  gInfo = rInfo->gInfo ; 

  rmm_rwlock_rdlock(&gInfo->ConnSyncRW) ; 
  while ( !nOk && !rInfo->GlobalSync.goDN )
  {
    while ( !gInfo->nConnections )
    {
      rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ; 
      rmm_cond_twait(&rInfo->prWcw,0,rInfo->aConfig.BaseWaitNano,1) ;
      rmm_rwlock_rdlock(&gInfo->ConnSyncRW) ; 
      if ( rInfo->GlobalSync.goDN )
        goto exitThread ; 
    }
 // if ( gInfo->connUpd || doSelect ) 
    {
      llmFD_ZERO(&rfds) ;
      gInfo->connUpd = 0 ; 
      np1 = 0 ; 
      for ( cInfo=gInfo->firstConnection ; cInfo ; cInfo=nextC )
      {
        nextC = cInfo->next ; 
        if ( cInfo->rdInfo.buffer == NULL && !cInfo->use_shm && !cInfo->use_ib )
        {
          max_read_size = 16*rInfo->DataPacketSize ; 
          if ( (cInfo->rdInfo.buffer = malloc(max_read_size)) == NULL )
          {
            LOG_MEM_ERR(tcHandle,methodName,(max_read_size));
            max_read_size = rInfo->DataPacketSize ; 
            if ( (cInfo->rdInfo.buffer = MM_get_buff(rInfo->recvBuffsQ,8,&iError)) == NULL )
            {
              llmFD_CLR(cInfo->sfd,&rfds) ; 
              continue ; 
            }
          }
          cInfo->rdInfo.buflen = max_read_size ; 
          cInfo->rdInfo.bptr   = cInfo->rdInfo.buffer ; 
          cInfo->rdInfo.reqlen = INT_SIZE ; 
          cInfo->rdInfo.offset = 0 ; 
          cInfo->last_r_time = rInfo->CurrentTime ; 
        }
        sfd = cInfo->sfd ; 
        if ( !is_conn_valid(gInfo,rInfo,cInfo,0) )
        {
          llmFD_CLR(sfd,&rfds) ; 
          continue ; 
        }
        if ( cInfo->hold_it ) continue ; 

        sfd = cInfo->sfd ;
        llmFD_SET(sfd,&rfds) ; 
        if ( np1 < sfd )
             np1 = sfd ;
      }
      if ( np1 )
        np1++ ; 
      else
      {
        rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ; 
        rmm_cond_twait(&rInfo->prWcw,0,rInfo->aConfig.BaseWaitNano,1) ;
        rmm_rwlock_rdlock(&gInfo->ConnSyncRW) ; 
        continue ; 
      }
    }
   #if USE_POLL
    nr = poll(gInfo->rfds, gInfo->nrfds, 10) ; 
   #else
    {
      tv.tv_sec  = 0 ; 
      tv.tv_usec = nOk ? 0 : 10000 ; 
      nr = rmm_select((int)np1,&rfds,NULL,NULL,&tv) ;
    }
   #endif
    if ( nr >= 0 ) rInfo->TotSelects++;

    for ( cInfo=gInfo->firstConnection ; cInfo ; cInfo=nextC )
    {
      nextC = cInfo->next ; 
      sfd = cInfo->sfd ; 
      if ( !is_conn_valid(gInfo,rInfo,cInfo,0) )
      {
        llmFD_CLR(sfd,&rfds) ; 
        continue ; 
      }
      if ( cInfo->hold_it ) continue ; 
     #if USE_POLL
      if ( (gInfo->rfds[(1+cInfo->use_ib)*cInfo->ind].revents&POLLIN) )
     #else
      if ( llmFD_ISSET(sfd,&rfds) )
     #endif
      {
        if ( nOk < MAX_READY_CONNS )
          rInfo->ConnOk[nOk++] = cInfo ; 
      }
      else
      {
        if ( !(cInfo->hb_to <= 0 || (cInfo->one_way_hb && cInfo->init_here)) &&
             cInfo->last_r_time + cInfo->hb_to < rInfo->CurrentTime )
        {
          cInfo->to_detected = 1 ; 
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4224),"%s%d%d%llu%llu",
              "The RUM connection {0} is no longer valid on the receiver side (HBTO: {1} {2} {3} {4}).",  
            cInfo->req_addr,cInfo->hb_to,(int)(rInfo->CurrentTime-cInfo->last_r_time),
            LLU_VALUE(cInfo->last_r_time), LLU_VALUE(rInfo->CurrentTime));
          is_conn_valid(gInfo,rInfo,cInfo,1) ; 
          llmFD_CLR(sfd,&rfds) ; 
          continue ; 
        }
        else
        {
          llmFD_SET(sfd,&rfds) ; 
        }
      }
    }
    rmm_rwlock_rdrelock(&gInfo->ConnSyncRW) ; 
  }
 exitThread:
  rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ; 
  return nOk ; 
}

int readPacket(rmmReceiverRec *rInfo, char *buff, uint32_t bufflen, int myid)
{
  int rc=0 , nr=0 ; 
  ConnInfoRec *cInfo=NULL ; 

  for ( ; ; )
  {
    if ( LL_trylock(rInfo->sockQ) )
    {
      LL_lock(rInfo->sockQ) ; 
      rInfo->tryLock++ ; 
    } 
    if ( cInfo )
    {
      if ( rc > 0 )
      {
        if ( !cInfo->iPoll )
          rInfo->prOut++ ; 
        if ( !cInfo->is_invalid && !cInfo->hold_it && LL_put_buff(rInfo->sockQ,cInfo,0) )
        {
          if ( rInfo->prOut > 4096 )
            LL_signalE(rInfo->sockQ) ; 
          cInfo->iPoll = 0 ; 
        }
        else
          cInfo->ppIn = 0 ; 
        pthread_mutex_lock(&rInfo->ppMutex) ; 
        LL_unlock(rInfo->sockQ) ; 
        return 1 ; 
      }
      else
      if ( rc < 0 )
      {
        cInfo->ppIn = 0 ; 
        cInfo->is_invalid |= P_INVALID ; 
      }
      else
      if ( !cInfo->is_invalid && cInfo->iPoll < rInfo->aConfig.RecvPacketNpoll )
      {
        cInfo->iPoll++ ; 
        if ( cInfo->hold_it || !LL_put_buff(rInfo->sockQ,cInfo,0) )
          cInfo->ppIn = 0 ; 
        nr++ ; 
      }
      else
        cInfo->ppIn = 0 ; 
    }
    while ( ((nr + rInfo->prOut > 4096) && !rInfo->prIn) || (cInfo=LL_get_buff(rInfo->sockQ,0)) == NULL )
    {
      if ( !rInfo->prIn && rInfo->tryLock < 256 )
      {
        nr = 0 ; 
        rInfo->prOut = 0 ; 
        rInfo->prIn++ ; 
        LL_unlock(rInfo->sockQ) ; 
        rc = doSelect(rInfo) ; 
        LL_lock(rInfo->sockQ) ; 
        rInfo->prIn-- ; 
        if ( rc <= 0 )
        {
          LL_unlock(rInfo->sockQ) ; 
          return rc ; 
        }
        do
        {
          cInfo = rInfo->ConnOk[--rc] ; 
          if ( !(cInfo->ppIn || cInfo->is_invalid) )
          {
            if ( LL_put_buff(rInfo->sockQ,cInfo,0) )
            {
              cInfo->ppIn = 1 ; 
              cInfo->iPoll = 0 ; 
            }
          }
        } while ( rc ) ; 
        if ( LL_get_nBuffs(rInfo->sockQ,0) > 1 )
          LL_signalE(rInfo->sockQ) ; 
      }
      else
      {
        if ( rInfo->aConfig.UsePerConnInQ )
        {
          rInfo->tryLock = 0 ; 
          LL_unlock(rInfo->sockQ) ; 
          tsleep(0,rInfo->aConfig.BaseWaitNano) ;
          LL_lock(rInfo->sockQ) ; 
        }
        else
        {
          rInfo->tryLock = 0 ; 
          LL_waitE(rInfo->sockQ) ; 
          nr = 0 ; 
        }
        if ( rInfo->GlobalSync.goDN )
        {
          LL_unlock(rInfo->sockQ) ; 
          return 0 ; 
        }
        if ( rInfo->pp_signaled )
        {
          rInfo->pp_signaled = 0 ; 
          LL_unlock(rInfo->sockQ) ; 
          workMA(rInfo,NULL) ;
          LL_lock(rInfo->sockQ) ; 
        }
      }
    }
//  if ( rInfo->prOut > 3072 && !rInfo->prIn && !LL_get_nBuffs(rInfo->sockQ,0) )
//    LL_signalE(rInfo->sockQ) ; 
    LL_unlock(rInfo->sockQ) ; 

    rc = extractPacket(rInfo, cInfo, buff, bufflen) ; 
    if ( !rc && nr > LL_get_nBuffs(rInfo->sockQ,0) )
      sched_yield() ; 
  }
}


/** !!!!!!!!!!!!!!!!!!!!!! */
int  check_glb(rStreamInfoRec *pst, rTopicInfoRec  *ptp, pgm_seq psn_glb, rmmMessageID_t msn_glb)
{
  rmmMessageID_t msn_next ; 

  if ( ptp->goback.lag_time )
    msn_next = ptp->goback.msn_next[0] + (ptp->goback.msn_next[2]-ptp->goback.msn_next[1]) ; 
  else
    msn_next = ptp->msn_next ; 
  if ( msn_glb > msn_next )
    return 1 ;
  if ( msn_glb > pst->msn_glb ||
      (msn_glb == pst->msn_glb && XltY(psn_glb,pst->psn_glb)) )
  {
    pst->psn_glb = psn_glb ;
    pst->msn_glb = msn_glb ;
  }
  return 0 ;
}
void process_glbs(rStreamInfoRec *pst, rTopicInfoRec  *ptp)
{
  int i,j ;

  for ( i=0 , j=0 ; i<pst->num_glbs ; i++ )
  {
    if ( check_glb(pst,ptp,pst->psn_glbs[i],pst->msn_glbs[i]) )
    {
      if ( j < i )
      {
        pst->psn_glbs[j] = pst->psn_glbs[i] ;
        pst->msn_glbs[j] = pst->msn_glbs[i] ;
      }
      j++ ;
    }
  }
  pst->num_glbs = j ;
}
void process_glb(rStreamInfoRec *pst, rTopicInfoRec  *ptp, pgm_seq psn_glb, rmmMessageID_t msn_glb)
{
  int i,j ;

  if ( check_glb(pst,ptp,psn_glb,msn_glb) )
  {
    if ( pst->num_glbs >= MAX_GLB_2KEEP )
      process_glbs(pst,ptp) ;
    if ( pst->num_glbs <  MAX_GLB_2KEEP )
    {
      pst->psn_glbs[pst->num_glbs] = psn_glb ;
      pst->msn_glbs[pst->num_glbs] = msn_glb ;
      pst->num_glbs++ ;
    }
    else
    {
      pst->psn_glbs[MAX_GLB_2KEEP] = psn_glb ;
      pst->msn_glbs[MAX_GLB_2KEEP] = msn_glb ;
      sortuli(MAX_GLB_2KEEP+1,pst->msn_glbs,pst->psn_glbs);
      for ( i=1 , j=0 ; i<=MAX_GLB_2KEEP ; i++ )
      {
        if ( pst->psn_glbs[j] != pst->psn_glbs[i] )
        {
          if ( ++j < i )
          {
            pst->psn_glbs[j] = pst->psn_glbs[i] ;
            pst->msn_glbs[j] = pst->msn_glbs[i] ;
          }
        }
      }
      pst->num_glbs = j ;
    }
  }
}

/** !!!!!!!!!!!!!!!!!!!!!! */
/** !!!!!!!!!!!!!!!!!!!!!! */

THREAD_RETURN_TYPE PacketProcessor_tcp(void *arg)
{
  uint16_t nla_afi , option_len ; 
  uint32_t spm_id, rxw_trail=0, rxw_lead=0 ; 
  pgm_seq psn_glb  ; 
  uli msn_trail={0}, msn_lead={0} , msn_glb ;
  int iBad_pactype, iBad_nlaafi, iBad_pgmopt, //iBad_recvQ, 
      iBad_lout, iBad_rout, iBad_mem0, iBad_mem1, iBad_mem2, iBad_mem3, iBad_mem4 ;
  int reliability=0, n , wma , myid ; 
  rmmStreamID_t sid ;
  rumHeader *phrm ;
  pgmHeaderCommon *phc ;
  pgmHeaderSPM    *phs ;
  pgmHeaderData   *phd ;
  pgmOptHeader    *php , opt ; 
  rTopicInfoRec  *ptp=NULL ;
  rStreamInfoRec *pst=NULL ;
  uint8_t *buff, *packet , *bptr , *tptr , *opt_end, opt_type , isActive ;
  uint8_t pacTypes[16] , pacType ;
  WhyBadRec *wb ;
  //rumInstanceRec *gInfo ;
  rmmReceiverRec *rInfo = (rmmReceiverRec *)arg;
  const char* myName = "PacketProcessor";
  char* eventDescription;
  TCHandle tcHandle = rInfo->tcHandle;
  char errMsg[ERR_MSG_SIZE];

 #if USE_PRCTL
  {
    char tn[16] ; 
    rmm_strlcpy(tn,__FUNCTION__,16);
    prctl( PR_SET_NAME,(uintptr_t)tn);
  }
 #endif

  //gInfo = rInfo->gInfo ; 
  pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
  myid = rInfo->GlobalSync.ppUP++ ;
  pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
  wb = &rInfo->why_bad ;
  php = &opt ; 

  memset(pacTypes,0,16) ;
  pacTypes[PGM_TYPE_SPM  ] = 0 ;
  pacTypes[PGM_TYPE_ODATA] = 1 ;
  pacTypes[PGM_TYPE_RDATA] = 1 ;
  pacTypes[PGM_TYPE_NAK  ] = 0 ;
  pacTypes[PGM_TYPE_NNAK ] = 0 ;
  pacTypes[PGM_TYPE_NCF  ] = 0 ;
  pacTypes[RUM_TYPE_SCP  ] = 1 ;

  pthread_mutex_lock(&rInfo->wb_mutex) ;
  //iBad_recvQ   = add_why_bad_msg(wb,0,myName,"RecvQ empty") ;
  iBad_pactype = add_why_bad_msg(wb,0,myName,"Bad Packet type") ;
  iBad_nlaafi  = add_why_bad_msg(wb,0,myName,"Bad NLA_AFI") ;
  iBad_pgmopt  = add_why_bad_msg(wb,0,myName,"Bad PGM Option") ;
  iBad_lout    = add_why_bad_msg(wb,0,myName,"Left  of rWindow range") ;
  iBad_rout    = add_why_bad_msg(wb,0,myName,"Right of rWindow range") ;
  iBad_mem0    = add_why_bad_msg(wb,0,myName,"Failed to allocate buffer from dataQ") ;
  iBad_mem1    = add_why_bad_msg(wb,0,myName,"Failed to allocate buffer from rsrvQ") ;
  iBad_mem2    = add_why_bad_msg(wb,0,myName,"Odata beyond rsrvQ size") ;
  iBad_mem3    = add_why_bad_msg(wb,0,myName,"Failed to allocate BIG buffer") ;
  iBad_mem4    = add_why_bad_msg(wb,0,myName,"Failed to put packet in dataQ") ;
  pthread_mutex_unlock(&rInfo->wb_mutex) ;
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5069),"%s%llu%d",
    "The {0} thread is running. Thread id: {1} ({2}).",
    "PacketProcessor",LLU_VALUE(my_thread_id()),
#ifdef OS_Linuxx
    (int)gettid());
#else
    (int)my_thread_id());
#endif

  if ( rmm_get_thread_priority(pthread_self(),errMsg,ERR_MSG_SIZE) == RMM_SUCCESS )
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6226),"%s",
    "The RUM receiver got the thread priority successfully. Return Status: {0}", errMsg);


  while ( !rInfo->GlobalSync.goDN )
  {
    if (!(buff=pp_get_buffer(rInfo,wb,myid)) )
      break ;
  

    rmm_rwlock_rdlock(&rInfo->GlobalSync.rw) ; 

    phrm = (rumHeader *)buff ;
    packet = buff + phrm->PackOff ;

    phc = (pgmHeaderCommon *)packet ;

    if ( phc->type == RUM_TX_STR_REP )
    {
      process_tx_stream_report(rInfo,phrm->ConnId,packet) ; 
      rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ; 
      pthread_mutex_unlock(&rInfo->ppMutex) ; 
      MM_put_buff(rInfo->recvBuffsQ,buff) ;
      continue ;
    }

    pacType = phc->type&0x0f ;
    rInfo->TotPacks[pacType]++ ;

    bptr = (void *)&sid ;
    memcpy(bptr,&phc->gsid_high,6) ;
    bptr += 6 ;
    tptr = (pacType == PGM_TYPE_NAK) ? (uint8_t *)&phc->dest_port : (uint8_t *)&phc->src_port ; 
    memcpy(bptr,tptr, 2) ;

    if ( (pst = find_stream(rInfo,sid)) == NULL )
    {
      if ( (pst = check_new_stream(rInfo,sid,buff,packet,phrm,phc,pacType,pacTypes,wb,iBad_pactype,iBad_nlaafi,iBad_pgmopt)) == NULL )
      {
        MM_put_buff(rInfo->recvBuffsQ,buff) ;
        rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ; 
        pthread_mutex_unlock(&rInfo->ppMutex) ; 
        continue ;
      }
    }
    pthread_mutex_lock(&pst->ppMutex) ; 
    pthread_mutex_unlock(&rInfo->ppMutex) ; 
    if ( pst->killed )
    {
      pthread_mutex_unlock(&pst->ppMutex) ; 
      rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ;
      MM_put_buff(rInfo->recvBuffsQ,buff) ;
      continue ;
    }
    pst->pp_last_time = rInfo->CurrentTime ; 
    ptp = rInfo->rTopics[pst->topic_id] ;

    if ( !pst->mtl_offset )
    {
      if ( !check_first_packet(rInfo, pst, ptp, packet,pacType) )
      {
        if ( pst->killed )
        {
          pthread_mutex_unlock(&pst->ppMutex) ; 
          destroy_stream(pst) ; 
          rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ; 
          MM_put_buff(rInfo->recvBuffsQ,buff) ;
          continue ;
        }
        if ( !(ptp->failover && pacType == RUM_TYPE_SCP) )
        {
          pthread_mutex_unlock(&pst->ppMutex) ; 
          rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ; 
          MM_put_buff(rInfo->recvBuffsQ,buff) ;
          continue ;
        }
      }
    }

    wma = 0 ; 
    switch (pacType)
    {
      case PGM_TYPE_RDATA :
        pst->RdataPacks++ ;
        /*fall through*/
      case PGM_TYPE_ODATA :
        phd = (pgmHeaderData *)packet ;
        bptr = packet + sizeof(pgmHeaderData) ;
        if ( phd->options&rInfo->PGM_OPT_PRESENT )
        {
          php = (pgmOptHeader *)bptr ;
          opt_type = (php->option_type&~PGM_OPT_END) ;
          if ( opt_type != PGM_OPT_LENGTH )
          {
            llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3227),"%d%d%s",
              "No PGM_OPT_LENGTH option was found for data packet ({0} {1}) on stream {2}.", opt_type,PGM_OPT_LENGTH,pst->sid_str);
            kill_stream(pst,"No PGM_OPT_LENGTH option found for data packet.") ;
            wma = 0 ; 
            break ;
          }
          bptr += ntohs(php->option_other) ;
        }

        bptr++ ;
        if ( pst->mtl_offset != (bptr - packet) )
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4228),"%d%d%s",
            "The RUM receiver detects a mtl_offset change ({0} {1}) on stream {2}.",pst->mtl_offset,(int)(bptr - packet) , pst->sid_str);
          pst->mtl_offset = (int)(bptr - packet) ;
        }

        rxw_lead  = ntohl(phd->data_sqn) ;
        rxw_trail = ntohl(phd->trailing_sqn) ;
        if ( pst->msn_offset )
        {
          tptr = packet + pst->msn_offset + INT_SIZE + LONG_SIZE ; 
          NgetLong(tptr,msn_lead ) ; 
        }
        else
          msn_lead.l = 0 ; 
        process_data_packet(rInfo,pst,myName,rxw_trail,rxw_lead,phrm->PackLen,msn_lead.l,buff  ,
                            wb,iBad_lout,iBad_rout,iBad_mem0,iBad_mem1,iBad_mem2,iBad_mem3,iBad_mem4) ;
        buff = NULL ; 
        wma++ ; 
        break ;
      case RUM_TYPE_SCP :
      case PGM_TYPE_SPM :
        phs = (pgmHeaderSPM *)packet ;
        spm_id    = ntohl(phs->spm_sqn) ;
        rxw_trail = ntohl(phs->trailing_sqn) ;
        rxw_lead  = ntohl(phs->leading_sqn) ;

        if ( XltY(pst->rxw_trail,rxw_trail) )
        {
          pst->rxw_trail = rxw_trail ;
          if ( XgtY(rxw_trail,SQ_get_tailSN(pst->dataQ,1)) )
          {
            if ( pst->mtl_offset )
              wma++ ; 
           /*
            snprintf(LogMsg[0],LOG_MSG_SIZE," Stream %s: Trail advanced beyond Tail SP %u %u ",pst->sid_str,SQ_get_tailSN(pst->dataQ,1),rxw_trail) ;
            rmmLogger(0,rInfo,myName,LogMsg,RMM_L_W_GENERAL_WARNING) ;
           */
          }
        }
        if ( XltY(pst->rxw_lead,rxw_lead)  )
          pst->rxw_lead  = rxw_lead  ;

        nla_afi = ntohs(phs->nla_afi) ;
        if ( XltY(pst->last_spm,spm_id) )
        {
          pst->path_nla.length = nla_afi_len[nla_afi] ; 
          memcpy(pst->path_nla.bytes,&phs->path_nla,pst->path_nla.length) ;  
        }
        pst->last_spm  = spm_id ;

        reliability = -1 ; 
        bptr = packet + sizeof(pgmHeaderSPM) + nla_afi_len[nla_afi] - 4 ; 
        memcpy(php,bptr,sizeof(pgmOptHeader)) ;
        if ( php->option_type != PGM_OPT_LENGTH )
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3229),"%d%d",
            "The RUM receiver detects a bad SPM packet ({0} {1}).",php->option_type,PGM_OPT_LENGTH);
          kill_stream(pst,"The RUM receiver detects bad SPM packet") ;
          set_why_bad(wb,iBad_pgmopt) ;
          wma = 0 ; 
          break ;
        }
        opt_end = bptr + ntohs(php->option_other) ;
        option_len = php->option_len ; 
        while ( option_len > 0 && (bptr += option_len) < opt_end && !(php->option_type&PGM_OPT_END) )
        {
          memcpy(php,bptr,sizeof(pgmOptHeader)) ;
          opt_type = (php->option_type&~PGM_OPT_END) ; 
          option_len = ntohs(php->option_other) ; 
          if ( opt_type == RMM_OPT_MSG_INFO )
          {
            tptr = bptr + 4 ;
            NgetLong(tptr,msn_trail) ; 
            NgetLong(tptr,msn_lead ) ; 
            NgetChar(tptr,reliability) ; 
            if ( pst->msn_trail.l < msn_trail.l )
                 pst->msn_trail.l = msn_trail.l ;
            if ( pst->msn_lead.l  < msn_lead.l )
                 pst->msn_lead.l  = msn_lead.l ;
            continue ; 
          }
          if ( opt_type == RMM_OPT_MSG_TO_PAC )
          {
            tptr = bptr + 4 ;
            NgetChar(tptr,n) ;
            tptr += 3 ;
            process_glbs(pst,ptp) ;
            while ( n-- )
            {
              NgetInt (tptr,psn_glb) ;
              NgetLong(tptr,msn_glb) ;
              process_glb(pst,ptp,psn_glb,msn_glb.l) ;
            }
            continue ;
          }
          if ( opt_type == RUM_OPT_CONTROL )
          {
            tptr = bptr + 4 ;
            NgetChar (tptr,reliability) ;
            reliability &= ~0xc0 ; 
            NgetChar (tptr,isActive) ;
            if ( !isActive )
            {
              tptr++ ; /* NgetChar (tptr,keepHistory) ; */
              tptr++ ; /* NgetChar (tptr,isPr) ; */
              NgetInt(tptr,n) ;
              pst->active = isActive ; 
              pst->close_time = rInfo->CurrentTime + n ; 
              eventDescription = "Stream is going to shutdown.";
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5230),"%s%d%llu",
                "The stream {0} will shutdown in {1} milliseconds. The total number of messages sent on this stream is {2}.",pst->sid_str,n,UINT_64_TO_LLU(pst->TotMsgsOut));
              if ( ptp->on_event )
              {
                void *EvPrms[2] ; EvPrms[0]=eventDescription; 
                raise_stream_event(pst, RMM_CLOSED_TRANS, EvPrms, 1) ; 
              }
            }
            continue ;
          }
        }

        n = 0 ;


        if ( reliability != -1 && reliability != pst->reliability )
        {
          pst->reliability = ptp->reliable ? reliability :  RMM_UNRELIABLE ;
          pst->reliable = (ptp->reliable && pst->reliability != RMM_UNRELIABLE) ;
          if ( ptp->reliable && !pst->reliable )
          {
            eventDescription = "Stream is not reliable while topic require reliability.";
            void *EvPrms[2] ; EvPrms[0]=eventDescription; 
            llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3234),"%s",
              "The stream {0} reliability parameter does not meet the queue reliability requirements and the stream will be rejected.",pst->sid_str);
            if ( ptp->on_event )
            {
              raise_stream_event(pst, RMM_RELIABILITY, EvPrms, 1) ; 
            }
            kill_stream(pst,eventDescription) ;
            wma = 0 ; 
            break ; 
          }
        }
        break ;
      default:
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3237),"%d",
          "The received packet type {0} is not valid.",((int)pacType));
        if(llmIsTraceAllowed(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7217)))
        {
          char dump[512];
          dumpBuff(myName,dump,256,buff,64);
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7217),"%d%s",
            "The packet type {0} is not valid. dumpBuff={1}",
            ((int)pacType),dump);
        }
        break ;
    }
    if ( buff )
      MM_put_buff(rInfo->recvBuffsQ,buff) ;
    pthread_mutex_unlock(&pst->ppMutex) ; 
    if ( wma )  /* wma is forced to 0 when pst->killed */
    {
      if ( rInfo->UseNoMA )
      {
        workMA(rInfo,pst) ;
        continue ; /*  rmm_rwlock_rdunlock() already done within workMA! */
      }
      else
        wakeMA(rInfo,pst) ;
    }
    else
    if ( pst->killed )
      destroy_stream(pst) ; 
    rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ;
  }
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5067),"%s%llu",
     "The {0}({1}) thread was stopped.","PacketProcessor", THREAD_ID(pthread_self()));
  pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
  rInfo->GlobalSync.ppUP-- ;
  pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
  rmm_thread_exit() ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

rStreamInfoRec *check_new_stream(rmmReceiverRec *rInfo, rmmStreamID_t sid, uint8_t *buff, uint8_t *packet, rumHeader *phrm, pgmHeaderCommon *phc,
                                uint8_t pacType, uint8_t *pacTypes, WhyBadRec *wb, int iBad_pactype, int iBad_nlaafi, int iBad_pgmopt)
{
  uint16_t src_port=0, nla_afi , option_len ; 
  uint32_t rxw_trail=0, rxw_lead=0 , new_trail , late_join=0;
  uli msn_trail={0}, msn_lead={0} ; 
  ConnInfoRec *cInfo ; 
  pgmHeaderSPM    *phs ;
  pgmOptHeader    *php , opt ; 
  pgmHeaderData   *phd ;
  rTopicInfoRec  *ptp=NULL ;
  rStreamInfoRec *pst=NULL ;
  rumStreamParameters sp , *psp ; 
  ipFlat src_nla ;  
  uint8_t *bptr , *tptr , opt_type , *opt_end, isActive , keepHistory=0 ; 
  int topicLen, n, reliability=0, isTf=0 , isPr=0, have_lj=0, iss ; 
  char *topicName , *src_addr , sid_str[20] ;
  rumEvent ev ; 
  rumInstanceRec *gInfo ;
  scpInfoRec *sInfo, sInfo_ ; 
  TCHandle tcHandle = rInfo->tcHandle;
  char* eventDescription=NULL;
    gInfo = rInfo->gInfo ; 
    psp = &sp ; 
    php = &opt ; 

    do
    {
      if ( find_rejected_stream(rInfo,sid) )
        continue ;

      if ( pacTypes[pacType] == 0 )
      {
        if(llmIsTraceAllowed(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7238)))
        {
          char dump[512];
          dumpBuff("check_new_stream",dump,256,buff,64);
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7238),"%d%s",
            "check_new_stream: Bad Packet type ; {0}. dumpBuff={1}",((int)pacType),dump);
        }
        set_why_bad(wb,iBad_pactype) ;
        reject_stream(rInfo,sid,phrm->ConnId,4) ;
        continue ;
      }

      rmm_rwlock_rdlock(&gInfo->ConnSyncRW) ; 
      if ( (cInfo=rum_find_connection(gInfo,phrm->ConnId,phrm->ConnInd,0)) == NULL ) 
      {
        rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ; 
        b2h(sid_str,(uint8_t *)&phrm->ConnId,sizeof(phrm->ConnId)) ;
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7239),"%s",
          "The connection {0} is not recognized",sid_str);
        continue ;
      }
      if ( cInfo->is_invalid ) 
      {
        rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ; 
        b2h(sid_str,(uint8_t *)&phrm->ConnId,sizeof(phrm->ConnId)) ;
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7240),"%s",
            "The connection {0} is not valid",sid_str);
        continue ;
      }
      rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ; 

      memset(&ev,0,sizeof(rumEvent)) ; 
      ev.stream_id = sid ; 
      topicName = ev.queue_name ; 

      have_lj = 0 ; 

      if ( pacType == RUM_TYPE_SCP || 
           pacType == PGM_TYPE_SPM )
      {
        phs = (pgmHeaderSPM *)packet ;
        rxw_trail = ntohl(phs->trailing_sqn) ;
        rxw_lead  = ntohl(phs->leading_sqn) ;
        nla_afi   = ntohs(phs->nla_afi) ;
        if ( nla_afi != NLA_AFI &&
             nla_afi != NLA_AFI6)
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7241),"%d%d%d",
            "Bad SCP Packet ; nla_afi={0} {1} {2}",nla_afi,NLA_AFI,NLA_AFI6);
          set_why_bad(wb,iBad_nlaafi) ;
          reject_stream(rInfo,sid,phrm->ConnId,4) ;
          continue ; 
        }
        bptr = packet + sizeof(pgmHeaderSPM) + nla_afi_len[nla_afi] - 4 ; 
        src_nla.length = nla_afi_len[nla_afi] ; 
        memcpy(src_nla.bytes,&phs->path_nla,src_nla.length) ; 
        rmm_ntop(nla_afi_af[nla_afi],src_nla.bytes,ev.source_addr,RUM_ADDR_STR_LEN) ;
        src_addr = ev.source_addr ; 
        src_port = ntohs(phs->src_port) ; 
        ev.port = src_port ; 
        topicLen = RUM_MAX_QUEUE_NAME+1 ; 
  
        memcpy(php,bptr,sizeof(pgmOptHeader)) ;
        if ( php->option_type != PGM_OPT_LENGTH )
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7242),"%d%d",
            "Bad Packet ; 1st opt not PGM_OPT_LENGTH ; {0} {1}",php->option_type,PGM_OPT_LENGTH);
          set_why_bad(wb,iBad_pgmopt) ;
          reject_stream(rInfo,sid,phrm->ConnId,4) ;
          continue ;
        }
        n = 0 ;
        reliability = -1 ; 
        isActive = 2 ; 
        keepHistory = 0 ; 
        opt_end = bptr + ntohs(php->option_other) ;
        option_len = php->option_len ; 
        while ( option_len > 0 && (bptr += option_len) < opt_end && !(php->option_type&PGM_OPT_END) )
        {
          n++ ; 
          memcpy(php,bptr,sizeof(pgmOptHeader)) ;
          opt_type = (php->option_type&~PGM_OPT_END) ; 
          option_len = ntohs(php->option_other) ; 
          if ( opt_type == RUM_OPT_QUEUE_NAME )
          {
            topicLen = option_len-4 ;
            if ( topicLen > RUM_MAX_QUEUE_NAME )
            {
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7243),"%d%d",
                "QueueName is too long {0} {1}",topicLen,RUM_MAX_QUEUE_NAME);
              break ;
            }
            tptr = bptr + 4 ;
            memcpy(topicName,tptr,topicLen) ;
            topicName[topicLen] = 0 ;
            continue ; 
          }
          if ( opt_type == RUM_OPT_CONTROL )
          {
            tptr = bptr + 4 ;
            NgetChar (tptr,reliability) ;
            reliability &= ~0xc0 ; 
            isTf = 0 ; 
            NgetChar (tptr,isActive) ;
            NgetChar (tptr,keepHistory) ;
            if ( !isActive )
            {
              b2h(sid_str,(uint8_t *)&sid,sizeof(sid)) ;
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4244),"%s",
                "The RUM receiver is attempting to accept a non-active stream {0}.",sid_str);
              break ;
            }
            NgetChar (tptr,isPr) ;
            continue ;
          }
          if ( opt_type == PGM_OPT_JOIN )
          {
            tptr = bptr + 4 ;
            NgetInt(tptr,late_join) ;
            have_lj = 1 ; 
            continue ;
          }
          if ( opt_type == RMM_OPT_MSG_INFO )
          {
            tptr = bptr + 4 ;
            NgetLong(tptr,msn_trail) ; 
            NgetLong(tptr,msn_lead ) ; 
            NgetChar(tptr,reliability) ; 
            continue ; 
          }
        }
  
        if ( !isActive )
        {
          reject_stream(rInfo,sid,phrm->ConnId,4) ;
          continue ;
        }
        if ( n >= 16 || topicLen > RUM_MAX_QUEUE_NAME || isActive == 2 )
        {
          b2h(sid_str,(uint8_t *)&sid,sizeof(sid)) ;
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3245),"%s%d%d%d%d",
            "The RUM receiver found one or more of the following problems with stream{0}: [n({1})>=16 , topicLen({2})>RUM_MAX_QUEUE_NAME({3}) , isActive({4})==2]",sid_str, n,topicLen,RUM_MAX_QUEUE_NAME,isActive);
          reject_stream(rInfo,sid,phrm->ConnId,4) ;
          continue ;
        }

      }
      else
      if ( pacType == PGM_TYPE_ODATA ||
           pacType == PGM_TYPE_RDATA )
      {
        b2h(sid_str,(uint8_t *)&sid,sizeof(sid)) ;
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4246),"%s%x%x",
          "The first packet of stream {0} is not RUM_TYPE_SCP type ({2}). The packet type is {1}.", sid_str, pacType,RUM_TYPE_SCP);
        sInfo = &sInfo_ ; 
        if ( !find_scp(rInfo,sid,sInfo) )
        {
          b2h(sid_str,(uint8_t *)&sid,sizeof(sid)) ;
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3248),"%s%llu",
            "The first packet of stream {0} (cid={1}) does not contain SCP.", sid_str,UINT_64_TO_LLU(cInfo->connection_id));
          reject_stream(rInfo,sid,phrm->ConnId,4) ;
          continue ;
        }
        phd = (pgmHeaderData *)packet ;
        rxw_lead  = ntohl(phd->data_sqn) ;
        rxw_trail = ntohl(phd->trailing_sqn) ;

        reliability = sInfo->reliability ; 
        keepHistory = sInfo->keepHistory ; 
        isTf        = sInfo->isTf        ; 
        isPr        = sInfo->isPr        ; 
        src_nla     = sInfo->src_nla     ; 
        src_port    = sInfo->src_port    ; 
        topicLen    = sInfo->topicLen    ; 
        memcpy(topicName,sInfo->topicName,topicLen+1) ; 

        nla_afi   = (src_nla.length==nla_afi_len[NLA_AFI]) ? NLA_AFI : NLA_AFI6 ; 
        src_addr = ev.source_addr ; 
        rmm_ntop(nla_afi_af[nla_afi],src_nla.bytes,ev.source_addr,RUM_ADDR_STR_LEN) ;
        ev.port = src_port ; 

        bptr = packet + sizeof(pgmHeaderData) ;
        if ( phd->options&rInfo->PGM_OPT_PRESENT )
        {
          phd++ ;
          memcpy(php,phd,sizeof(pgmOptHeader)) ;
          if ( php->option_type != PGM_OPT_LENGTH )
          {
            b2h(sid_str,(uint8_t *)&sid,sizeof(sid)) ;
            llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3249),"%s%d%d",
              "The RUM receiver detected a bad packet on stream {0}. The PGM_OPT_LENGTH option (Option type={1}, PGM_OPT_LENTH={2}) is missing from this data packet.", sid_str, php->option_type,PGM_OPT_LENGTH);
          }
          else
          {
            opt_end = bptr + ntohs(php->option_other) ;
            option_len = php->option_len ; 
            while ( option_len > 0 && (bptr += option_len) < opt_end && !(php->option_type&PGM_OPT_END) )
            {
              memcpy(php,bptr,sizeof(pgmOptHeader)) ;
              opt_type = (php->option_type&~PGM_OPT_END) ; 
              option_len = ntohs(php->option_other) ; 
              if ( opt_type == PGM_OPT_JOIN )
              {
                tptr = bptr + 4 ;
                NgetInt(tptr,late_join) ;
                have_lj = 1 ; 
                continue ;
              }
              if ( opt_type == RMM_OPT_MSG_INFO )
              {
                tptr = bptr + 4 ;
                NgetLong(tptr,msn_trail) ; 
                NgetLong(tptr,msn_lead ) ; 
                continue ; 
              }
            }
          }
        }
      }
      else
      {
        if(llmIsTraceAllowed(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7250)))
        {
          char dump[512];
          dumpBuff("check_new_stream",dump,256,buff,64);
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7250),"%d%s",
            "check_new_stream: Bad Packet type ; {0} (place 1). dumpBuff={1}",((int)pacType),dump);
        }
        set_why_bad(wb,iBad_pactype) ;
        reject_stream(rInfo,sid,phrm->ConnId,4) ;
        continue ;
      }

      iss = 0 ; 
      if ( (ptp = find_topic(rInfo,topicName,topicLen)) == NULL )
      {
        iss = 1 ; 
        ev.type = RUM_NEW_SOURCE ; 

        memset(psp,0,sizeof(rumStreamParameters)) ; 
        psp->stream_id = sid ; 
        psp->rum_connection = &cInfo->apiInfo ; 
        psp->event = &ev ; 
        psp->reliability = reliability ; 
        psp->is_late_join = have_lj ; 

        ptp = find_streamset(rInfo,psp) ;
      }

      b2h(sid_str,(uint8_t *)&sid,sizeof(sid)) ;
      if ( ptp == NULL )
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5251),"%s%s%d",
          "The stream {0} is not requested. Queue name={1}, queue name length={2}", sid_str,topicName,topicLen);   
        sInfo = &sInfo_ ; 
        memset(sInfo,0,sizeof(scpInfoRec)) ; 
        sInfo->sid         = sid ; 
        sInfo->cid         = cInfo->connection_id ; 
        sInfo->reliability = reliability ; 
        sInfo->keepHistory = keepHistory ; 
        sInfo->isTf        = isTf        ; 
        sInfo->isPr        = isPr        ; 
        sInfo->src_nla     = src_nla     ; 
        sInfo->src_port    = src_port    ; 
        sInfo->topicLen    = topicLen    ; 
        memcpy(sInfo->topicName,topicName,topicLen+1) ; 
        add_scp(rInfo,sInfo) ; 
        reject_stream(rInfo,sid,phrm->ConnId,2) ;
        continue ;
      }

      n = 0 ; 
      if ((ptp->reliable && reliability == RMM_UNRELIABLE) )
      {
        eventDescription = "The Stream reliability is below the topic required reliability.";
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3257),"%d%d%s",
          "The reliability {0} of stream {2} is below the reliability {1} required by the queue.", reliability,ptp->reliability,sid_str);
        n = 1 ; 
      }
      if ( n )
      {
        if ( ptp->on_event )
        {
          void *EvPrms[2] ; EvPrms[0]=eventDescription; 
          ev.type = RUM_NEW_SOURCE_FAILED ; 
          ev.nparams = 1 ;
          ev.event_params = EvPrms ;
         #if USE_EA
          PutRumEvent(gInfo, &ev, ptp->on_event, ptp->ov_user) ; 
         #else
          ptp->on_event(&ev,ptp->ov_user) ;
         #endif
          ev.event_params = NULL ; 
        }
        reject_stream(rInfo,sid,phrm->ConnId,4) ;
        continue ;
      }

      if ((!ptp->reliable && reliability != RMM_UNRELIABLE) )
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4258),"%d%d%s",
          "The reliability {0} of stream {2} is above the reliability {1} required by the queue.", reliability,ptp->reliability,sid_str);
      }

      n = 0 ; 
      if ( have_lj )
      {
        n = 4 ; 
        new_trail = late_join ; 
      }
      else
      if ( ptp->stream_join_backtrack )
      {
        n = 5 ;
        new_trail = rxw_lead - ptp->stream_join_backtrack ; 
        if ( XltY(new_trail,rxw_trail) )
          new_trail = rxw_trail ;
      }
      else
      {
        n = 6 ; 
        new_trail = XgtY(rxw_trail,rxw_lead) ? rxw_trail : rxw_lead ;
      }
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5258),"%s%d%d%d%d%llu%llu%llu",
        "The new stream ({0}) was detected by the RUM receiver. Additional information: n={1}, rxw_trail={2}, new_trail={3}, rxw_lead={4}, msn_trail.l={5}, ptp->msn_next={6}, msn_lead.l={7}", 
        sid_str, n, rxw_trail,new_trail,rxw_lead,UINT_64_TO_LLU(msn_trail.l),
        UINT_64_TO_LLU(ptp->msn_next), UINT_64_TO_LLU(msn_lead.l));

      if ( XgtY(rxw_trail,new_trail) )
      {
        eventDescription = "The RUM receiver failed to start the late join session.";
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3259),"%u%u",
          "The RUM receiver failed to start the late join session. Trail Packet={0}, late join mark={1}", rxw_trail,late_join);
  
        if ( ptp->on_event )
        {
          void *EvPrms[2] ; EvPrms[0]=eventDescription; 
          ev.type = RMM_LATE_JOIN_FAILURE ; 
          ev.nparams = 1 ;
          ev.event_params = EvPrms ;
         #if USE_EA
          PutRumEvent(gInfo, &ev, ptp->on_event, ptp->ov_user) ; 
         #else
          ptp->on_event(&ev,ptp->ov_user) ;
         #endif
          ev.event_params = NULL ; 
        }
        new_trail = rxw_trail ; 
      }
      eventDescription = "New Stream recognized."; 
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5260),"%s%s%d%u%u",
        "The RUM receiver recognized the new stream.  Stream Id={0}, src={1}|{2}, trail={3}, lead={4}. ", 
        sid_str, src_addr,src_port,rxw_trail,rxw_lead);  
      pst = NULL ;
      pst = create_stream(sid,&src_nla,ptp,topicLen,topicName,phc->src_port,phc->dest_port,phc->gsid_high,phc->gsid_low,new_trail,rxw_lead) ;
      if ( pst )
      {
        pst->pp_last_time = rInfo->CurrentTime ; 
        pst->reliability = reliability ; 
        pst->keepHistory = keepHistory ; 
        pst->reliable = (reliability != RMM_UNRELIABLE) ; 
        pst->msn_trail = msn_trail ; 
        pst->msn_lead  = msn_lead  ; 
        pst->cInfo = cInfo ; 
        if ( iss && psp->new_stream_user )
          pst->om_user = psp->new_stream_user ; 
        pst->have_lj   = (have_lj || ptp->stream_join_backtrack) ; 
        pst->isTf = isTf ; 
        pst->isPr = isPr ; 
        pst->si.use_msg_properties = isPr ; 
        rmm_rwlock_r2wlock(&rInfo->GlobalSync.rw) ;
        add_stream(pst) ; 
        setParseMsgsStream(ptp,pst) ; 
        rmm_rwlock_w2rlock(&rInfo->GlobalSync.rw) ;
      }
      else
      {
        eventDescription = "The RUM receiver failed to create the stream";
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3261),"",
          "The RUM receiver failed to create the stream.");
  
        if ( ptp->on_event )
        {
          void *EvPrms[2] ; EvPrms[0]=eventDescription; 
          ev.type = RUM_NEW_SOURCE_FAILED ; 
          ev.nparams = 1 ;
          ev.event_params = EvPrms ;
         #if USE_EA
          PutRumEvent(gInfo, &ev, ptp->on_event, ptp->ov_user) ; 
         #else
          ptp->on_event(&ev,ptp->ov_user) ;
         #endif
          ev.event_params = NULL ; 
        }
        continue ;
      }
      if ( ptp->on_event )
      {
        void *EvPrms[2] ; EvPrms[0]=eventDescription; EvPrms[1]=&(pst->si) ;
        if ( !raise_stream_event(pst, RMM_NEW_SOURCE, EvPrms, 2) )
          pst->ns_event = 1 ; 
      }
      else
        pst->ns_event = 1 ; 
    } while ( 0 ) ;

    return pst ; 
}

/** ++++++++++++++++++++++ */
int check_first_packet(rmmReceiverRec *rInfo, rStreamInfoRec *pst, rTopicInfoRec  *ptp, uint8_t *packet,uint8_t pacType)
{
  pgm_seq rxw_lead , rxw_trail ; 
  uli msn_lead={0} ; 
  pgmOptHeader   *php , opt ; 
  pgmHeaderData  *phd ; 
  uint8_t *bptr , *tptr , version, expver ; 
  int iok  ; 
  char sid_str[20]; 
  TCHandle tcHandle = rInfo->tcHandle;
  char* eventDescription;
  php = &opt ; 
  iok = 0 ; 
  do
  {
    if ( pacType == PGM_TYPE_ODATA ||
        (pacType == PGM_TYPE_RDATA && pst->have_lj ) )
    {
      phd = (pgmHeaderData *)packet ;
      rxw_lead  = ntohl(phd->data_sqn) ;
      rxw_trail = ntohl(phd->trailing_sqn) ;

      bptr = packet + sizeof(pgmHeaderData) ;
      tptr = packet ;
      if ( phd->options&rInfo->PGM_OPT_PRESENT )
      {
        phd++ ;
        memcpy(php,phd ,sizeof(pgmOptHeader)) ;
        if ( php->option_type != PGM_OPT_LENGTH )
        {
          eventDescription = "The data packet contains no PGM_OPT_LENGTH option.";
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3262),"%d%d%s",
            "The data packet on stream {2} does not contain a PGM_OPT_LENGTH option ({0} {1}).", php->option_type,PGM_OPT_LENGTH,pst->sid_str);
          kill_stream(pst,eventDescription) ;
          continue ;
        }
        bptr += ntohs(php->option_other) ;
      }

      NgetChar(bptr,version) ;
      expver = PGM_VERSION ; 
      if ( expver != version )
      {
        void *EvPrms[3] ;
        eventDescription = "RUM version conflict.";
        EvPrms[0]=eventDescription;
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3264),"%d%d%s",
          "The RUM receiver found a RUM version conflict (exp={0}, rec={1}) on stream{2}.", expver,version,pst->sid_str);
        if ( ptp->on_event )
        {
          int v1=expver , v2=version ; 
          EvPrms[1] = &v1;
          EvPrms[2] = &v2;
          raise_stream_event(pst, RMM_VERSION_CONFLICT, EvPrms, 3) ;
        }
        if(llmIsTraceAllowed(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9260)))
        {
          char dump[512];
          dumpBuff("check_first_packet",dump,256,packet,128);
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(9260),"%s",
            "check_first_packet:kill_stream. dumpBuff={0}",dump);
        }
        kill_stream(pst,eventDescription) ;
        continue ;
      }
      SQ_set_base(pst->dataQ,1,pst->rxw_trail) ;
      pst->mtl_offset = (int)(bptr - packet) ;
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5265),"%s%u%u%u%u%d%d",
        "The RUM receiver found the first data packet on stream {0}. (trail={1} {2}, lead={3} {4}, mtl_offset={5}, msn_offset={6})", pst->sid_str,pst->rxw_trail,rxw_trail,pst->rxw_lead,rxw_lead,pst->mtl_offset,pst->msn_offset);

      if ( pst->msn_offset )
      {
        tptr += (INT_SIZE + LONG_SIZE) ; 
        NgetLong(tptr,msn_lead ) ; 
        b2h(sid_str,(uint8_t *)&msn_lead,sizeof(msn_lead)) ;
        eventDescription = "The RUM receiver found the first message on stream.";
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5266),"%s%s%llu",
          "The RUM receiver found the first message on stream {0}. Message Id: {1} (={2}).", 
          pst->sid_str,sid_str,UINT_64_TO_LLU(msn_lead.l));
        if ( ptp->on_event )
        {
          void *EvPrms[2] ; EvPrms[0]=eventDescription; EvPrms[1]=&msn_lead; 
          raise_stream_event(pst, RMM_FIRST_MESSAGE, EvPrms, 2) ;
        }
      }
      if ( !rInfo->UseNoMA )
        wakeMA(rInfo,pst) ; 
      iok = 1 ; 
    }
  } while(0) ; 

  return iok ; 
}
/** ++++++++++++++++++++++ */

void *pp_get_buffer(rmmReceiverRec *rInfo,WhyBadRec *wb, int iBad_recvQ)
{
  int iError ; 
  void *buff=NULL ;

  while ( (buff = (char *)MM_get_buff(rInfo->recvBuffsQ, 8,&iError)) == NULL && (!rInfo->aConfig.UsePerConnInQ ||
          (buff = (char *)MM_get_buff(rInfo->recvBuffsQ,-1,&iError)) == NULL) )
  {
    if ( rInfo->GlobalSync.goDN )
      return NULL ; 
  }
  if ( readPacket(rInfo,buff,(uint32_t)rInfo->DataPacketSize,iBad_recvQ) <= 0 )
  {
    MM_put_buff(rInfo->recvBuffsQ,buff) ;
    return NULL ;
  }

  return buff ;
}

/** ++++++++++++++++++++++ */
void process_data_packet(rmmReceiverRec *rInfo, rStreamInfoRec *pst, const char *myName, 
               uint32_t rxw_trail, uint32_t rxw_lead,uint32_t PackLen,rmmMessageID_t msg_num, unsigned char *packet,
               WhyBadRec *wb, int iBad_lout, int iBad_rout,int iBad_mem0, int iBad_mem1, int iBad_mem2, int iBad_mem3, int iBad_mem4)
{
  int iError , n , sigMa=0 , ibuff ; 
  unsigned char *bptr , *buff ; 
  rumHeader *phrm ;
  TCHandle tcHandle = rInfo->tcHandle;
  buff = packet ; 

  if ( buff != NULL )
  {
    phrm = (rumHeader *)buff ;
    phrm->PackOff += pst->mtl_offset ; 
    phrm->PackLen -= pst->mtl_offset ; 
    phrm->PackSqn  = rxw_lead ;
    if ( pst->msn_offset )
    {
      phrm->PackOff -= LONG_SIZE ;
      bptr = buff + phrm->PackOff ;
      memcpy(bptr,&msg_num,LONG_SIZE) ;
    }

    if ( (ibuff=SQ_put_buff(pst->dataQ,1,rxw_lead,&iError,buff)) > 0 )
    {
      sigMa = 1 ; 

      /* SQ_OR_flag(pst->nakSQ,1,rxw_lead,&iError,SQ_FLAG_PP_DATA) ; */

      rInfo->TotPacksIn++ ;
      pst->TotPacksIn++ ;
      if ( rInfo->aConfig.UsePerConnInQ )
        rumR_PerConnInQup(rInfo, pst) ; 
    }
    else
    if ( ibuff == 0 )
    {
      pgm_seq tail , head ; 
      tail = SQ_get_tailSN(pst->dataQ,1) ; 
      head = SQ_get_headSN(pst->dataQ,1) ; 
      if ( iError == 0 )
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4267),"%d%d%d%d%s",
          "The RUM receiver failed to put the packet in the data queue for stream {4}. (tail={0}, lead={1}, head={2}). Error: {3}", tail,rxw_lead,head,iError,pst->sid_str);
        set_why_bad(wb,iBad_mem4) ;
      }
      else if ( iError == -1 )
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5268),"%d%d%d",
          "The packet is left of the receive window range.(tail={0}, lead={1}, head={2}).", tail,rxw_lead,head);
        set_why_bad(wb,iBad_lout) ;
      }
      else
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5269),"%d%d%d",
          "The packet is right of the receive window range.(tail={0}, lead={1}, head={2}).", tail,rxw_lead,head);
        set_why_bad(wb,iBad_rout) ;
      }

      MM_put_buff(rInfo->dataBuffsQ,buff) ;
      buff = NULL ;
    }
    else   /* There was a packet there already !!! */
    {
      MM_put_buff(rInfo->dataBuffsQ,buff) ;
      pst->DupPacks++ ;
    }
  }

 /* only for oData
  if ( rxw_lead != (pst->rxw_lead+1) )
  {
    snprintf(LogMsg[0],LOG_MSG_SIZE," Out of order packet! stream: %s exp: %u recv: %u",
            pst->sid_str,(pst->rxw_lead+1),rxw_lead) ;
    rmmLogger(0,rInfo,myName,LogMsg,RMM_L_W_GENERAL_WARNING) ;
  }
 */
  if ( XgtY(rxw_lead,pst->rxw_lead)  )
  {
    n = XgtY(rxw_lead,pst->rxw_lead+1) ;
    pst->rxw_lead = rxw_lead  ;
    if ( !buff || n )
    {
      rInfo->NakAlertCount++ ;
   /*
      rmm_cond_signal(&rInfo->nakTcw,1) ;
   */
    }
  }
  if ( XltY(pst->rxw_trail,rxw_trail) )
    pst->rxw_trail = rxw_trail ;
  if ( sigMa && !rInfo->UseNoMA )
  {
    wakeMA(rInfo,pst) ; 
  }
}

/** ++++++++++++++++++++++ */

rmm_uint64 setNTC(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm)
{
  rumInstanceRec *gInfo ;
  rmmReceiverRec *rInfo ;

  rInfo = (rmmReceiverRec *)arg ;
  gInfo = rInfo->gInfo ; 

  rInfo->CurrentTime = curTime ; 
  pthread_mutex_lock(&gInfo->ClockMutex) ; 
  gInfo->CurrentTime = curTime ; 
  pthread_mutex_unlock(&gInfo->ClockMutex) ; 
  return curTime+1 ; 
}

/** ++++++++++++++++++++++ */
rmm_uint64 MemMaint(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm)
{
  int iError ; 
  void *buff ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)arg;
  TCHandle tcHandle = rInfo->tcHandle;
  char* eventDescription;

  if ( (buff = MM_get_buff(rInfo->dataBuffsQ,0,&iError)) != NULL )
    MM_put_buff(rInfo->dataBuffsQ,buff) ;

  if ( rInfo->aConfig.MemoryAlertPctHi > 0 )
  {
    if ( rInfo->MemoryAlertLevel == MEMORY_ALERT_OFF &&
         iError                  != MEMORY_ALERT_OFF )
    {
      eventDescription = "The memory alert is on.";
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4181),"%d%d",
        "The memory alert is on. {0} out of {1} buffers are in use.", MM_get_buffs_in_use(rInfo->recvBuffsQ),MM_get_max_size(rInfo->recvBuffsQ));
      if ( rInfo->on_alert_event )
      {
        int n ;
        rmmEvent ev;
        void *EvPrms[2] ; EvPrms[0]=eventDescription; EvPrms[1]=&n;
        memset(&ev,0,sizeof(rmmEvent)) ;
        ev.type = RMM_MEMORY_ALERT_ON ;
        n = 100 * MM_get_buffs_in_use(rInfo->recvBuffsQ) / MM_get_max_size(rInfo->recvBuffsQ) ;
        ev.nparams = 2 ;
        ev.event_params = EvPrms ;
       #if USE_EA
        PutRumEvent(rInfo->gInfo, &ev, rInfo->on_alert_event, rInfo->alert_user) ;
       #else
        rInfo->on_alert_event(&ev,rInfo->alert_user) ;
       #endif
      }
    }
    else
    if ( rInfo->MemoryAlertLevel != MEMORY_ALERT_OFF &&
         iError                  == MEMORY_ALERT_OFF )
    {
      eventDescription = "The memory alert is off.";
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4182),"%d%d",
          "The memory alert is off. {0} out of {1} buffers are in use.", MM_get_buffs_in_use(rInfo->recvBuffsQ),MM_get_max_size(rInfo->recvBuffsQ));
      if ( rInfo->on_alert_event )
      {
        int n ;
        rmmEvent ev;
        void *EvPrms[2] ; EvPrms[0]=eventDescription; EvPrms[1]=&n;
        memset(&ev,0,sizeof(rmmEvent)) ;
        ev.type = RMM_MEMORY_ALERT_OFF;
        n = 100 * MM_get_buffs_in_use(rInfo->recvBuffsQ) / MM_get_max_size(rInfo->recvBuffsQ) ;
        ev.nparams = 2 ;
        ev.event_params = EvPrms ;
       #if USE_EA
        PutRumEvent(rInfo->gInfo, &ev, rInfo->on_alert_event, rInfo->alert_user) ;
       #else
        rInfo->on_alert_event(&ev,rInfo->alert_user) ;
       #endif
      }
    }
  }
  rInfo->MemoryAlertLevel = iError ;

  return curTime+16*rInfo->aConfig.TaskTimerCycle ;
}
/** ++++++++++++++++++++++ */

rmm_uint64 calcTP(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm)
{
  rmm_uint64    totpacks ; 
  rStreamInfoRec *pst ;
  rmmReceiverRec *rInfo ;

  rInfo = (rmmReceiverRec *)arg ;

  if ( rmm_rwlock_tryrdlock(&rInfo->GlobalSync.rw) == RMM_FAILURE )
    return curTime+1 ; 

  totpacks = 0 ;
  for ( pst=rInfo->firstStream ; pst ; pst=pst->next )
    totpacks += (pst->TotPacksIn-pst->TotPacksOut) ;

  if ( totpacks > 0 )
  {
    for ( pst=rInfo->firstStream ; pst ; pst=pst->next )
      pst->ChunkSize = (int)(64+256*rInfo->rNumStreams*(pst->TotPacksIn-pst->TotPacksOut)/totpacks) ;
  }

  rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ; 

  return curTime+8*rInfo->aConfig.TaskTimerCycle ;
}

/** ++++++++++++++++++++++ */

rmm_uint64 chkSLB(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm)
{
  int i ;
  rmm_uint64 next_time ;
  rTopicInfoRec *ptp ;
  rmmReceiverRec *rInfo ;

  rInfo = (rmmReceiverRec *)arg ;

  next_time = curTime+250 ;

  if ( rmm_rwlock_tryrdlock(&rInfo->GlobalSync.rw) == RMM_FAILURE )
    return curTime+1 ;

  for ( i=0 ; i<rInfo->rNumTopics ; i++ )
  {
    if ( (ptp = rInfo->rTopics[i]) == NULL )
      continue ;

    if ( tp_lock(rInfo,ptp,0,2) )
    {
    //sort_labels(ptp) ; 
      tp_unlock(rInfo,ptp) ; 
    }
  }

  rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ;

  return next_time ;
}
/** ++++++++++++++++++++++ */

rmm_uint64 calcBW(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm)
{
  rmm_uint64    et,n1,n2,n3;
  rmmReceiverRec *rInfo = (rmmReceiverRec *)arg;
  rStreamInfoRec *pst ; 
  TCHandle tcHandle = rInfo->tcHandle;
  char* eventDescription;

  et = (rmm_uint64   )(1000+(curTime-reqTime)) ;
  if ( et > 0 )
  {
    rum_uint64 tb ; 
    rumInstanceRec *gInfo ; 
    ConnInfoRec *cInfo ; 

    n1 = n2 = n3 = 0 ; 
    tb = 0 ; 
    gInfo = rInfo->gInfo ; 
    rmm_rwlock_rdlock(&gInfo->ConnSyncRW) ; 
    for ( cInfo=gInfo->firstConnection ; cInfo ; cInfo=cInfo->next )
    {
      n1 += cInfo->nReads ; 
      n2 += cInfo->nPacks ; 
      n3 += cInfo->nCalls ; 
      tb += cInfo->nBytes ; 
    }
    rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ; 
    rInfo->TotReads = n1 ; 
    rInfo->TotCalls = n3 ; 
    rInfo->TotPacksRecv[0] = n2 ; 
    rInfo->TotBytesRecv[0] = tb ; 

    n1 = 1000*(rInfo->TotPacksRecv[0]-rInfo->TotPacksRecv[1]) / et ;
    n2 = (rmm_uint64   )(1000*(rInfo->TotBytesRecv[0]-rInfo->TotBytesRecv[1]) /(et*128)) ;
    rInfo->PacksRateIn = n1 ;
    rInfo->BytesRateIn = n2 ;
    rInfo->TotPacksRecv[1] = rInfo->TotPacksRecv[0] ;
    rInfo->TotBytesRecv[1] = rInfo->TotBytesRecv[0] ;
  }

  if ( rmm_rwlock_tryrdlock(&rInfo->GlobalSync.rw) == RMM_FAILURE )
    return curTime+1 ;

  if ( rInfo->nNeedTrim )
  {
    int i,m,n;
    rTopicInfoRec *ptp ;
    for ( i=0 ; i<rInfo->rNumTopics ; i++ )
    {
      if ( (ptp=rInfo->rTopics[i]) == NULL )
        continue ;
      if ( ptp->need_trim )
      {
        trim_packetQ(rInfo,ptp,1) ;
        if ( ptp->TotPacksDrop != ptp->OldPacksDrop )
        {
          LL_lock(ptp->packQ) ;
          m = LL_get_nBuffs(ptp->packQ,0) ; 
          n = (int)(ptp->TotPacksDrop - ptp->OldPacksDrop) ;
          ptp->OldPacksDrop = ptp->TotPacksDrop ;
          LL_unlock(ptp->packQ) ;
          if ( n > 0 )
          {
            eventDescription = "The packets were trimmed from receiver queue.";
            llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4184),"%d%s%d%d%d",
              "There are {0} packets that were trimmed from receiver queue {1}. (Current qSizq={2}, trim_params={3} {4}).",  
              n,ptp->topicName,m,ptp->maxPackInQ,ptp->maxTimeInQ);
            if ( ptp->on_event )
            {
              rmmEvent ev;
              void *EvPrms[2] ; EvPrms[0]=eventDescription; EvPrms[1]=&m;
              memset(&ev,0,sizeof(rmmEvent)) ;
              ev.type = RMM_RECEIVE_QUEUE_TRIMMED ;
              m = (int)n ; 
              ev.nparams = 2 ;
              ev.event_params = EvPrms ;
              strlcpy(ev.queue_name,ptp->topicName,RMM_TOPIC_NAME_LEN);
             #if USE_EA
              PutRumEvent(rInfo->gInfo, &ev, ptp->on_event, ptp->ov_user) ; 
             #else
              ptp->on_event(&ev,ptp->ov_user) ;
             #endif
            }
          }
        }
      }
    }
  }

  pthread_mutex_lock(&rInfo->deadQlock) ;
  for (;;)
  {
    pst = rInfo->deadQfirst ; 
    if ( !pst || pst->pp_last_time > rInfo->CurrentTime )
      break ;
    rInfo->deadQfirst = pst->next ; 
    pthread_mutex_lock(&rInfo->pstsQlock) ;
    pst->next = rInfo->pstsQfirst ; 
    rInfo->pstsQfirst = pst ; 
    pthread_mutex_unlock(&rInfo->pstsQlock) ;
  }
  pthread_mutex_unlock(&rInfo->deadQlock) ;

  rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ;

  return curTime+1000 ;
}

/** ++++++++++++++++++++++ */

rmm_uint64 callSS(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm)
{
  rmmReceiverRec *rInfo ;

  rInfo = (rmmReceiverRec *)arg ;


  if ( rmm_rwlock_tryrdlock(&rInfo->GlobalSync.rw) == RMM_FAILURE )
    return curTime+1 ; 

  SnapShot(rInfo) ; 

  rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ; 

  return curTime + rInfo->aConfig.SnapshotCycleMilli_rx ;
}

/** ++++++++++++++++++++++ */
rmm_uint64 callCHBTO(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm)
{
  rmm_uint64 nexTime ; 
  rmmReceiverRec *rInfo ;

  rInfo = (rmmReceiverRec *)arg ;

  if ( rmm_rwlock_tryrdlock(&rInfo->GlobalSync.rw) == RMM_FAILURE )
    return curTime+1 ; 

  nexTime = chkHBTO(rInfo) ; 

  rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ; 

  return nexTime ; 
}
/** ++++++++++++++++++++++ */

rmm_uint64 ConnStreamReport(rmm_uint64 reqTime, rmm_uint64 curTime, void *arg, int *prm)
{
  int n ; 
  rmm_uint64 nexTime ; 
  uint32_t k ; 
  char *bptr ; 
  rmmReceiverRec *rInfo ;
  rumInstanceRec *gInfo ;
  ConnInfoRec    *cInfo ; 
  rStreamInfoRec *pst ;

  rInfo = (rmmReceiverRec *)arg ;
  gInfo = rInfo->gInfo ; 

  if ( rmm_rwlock_tryrdlock(&rInfo->GlobalSync.rw) == RMM_FAILURE )
    return curTime+50; 

  if ( rmm_rwlock_tryrdlock(&gInfo->ConnSyncRW) == RMM_FAILURE )
  {
    rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ; 
    return curTime+50; 
  }

  nexTime = curTime + rInfo->aConfig.StreamReportIntervalMilli ; 

  for ( cInfo=gInfo->firstConnection ; cInfo ; cInfo=cInfo->next )
  {
    if ( (cInfo->hb_to <= 0 || (cInfo->one_way_hb && !cInfo->init_here)) )
      continue ; 

    if ( curTime < cInfo->next_rx_str_rep_time )
    {
      if ( nexTime > cInfo->next_rx_str_rep_time )
           nexTime = cInfo->next_rx_str_rep_time ;
      continue ; 
    }

    if ( pthread_mutex_trylock(&cInfo->mutex) != 0 )
    {
      nexTime = curTime+20 ; 
      continue ; 
    }

    if ( !cInfo->str_rep_tx_pending )
    {
      bptr = cInfo->rx_str_rep_tx_packet + (INT_SIZE+sizeof(pgmHeaderCommon)+INT_SIZE) ;
      n = 0 ; 
      for ( pst=rInfo->firstStream ; pst ; pst=pst->next )
      {
        if ( pst->cInfo == cInfo )
        {
          memcpy(bptr,&pst->sid,sizeof(rumStreamID_t)) ; 
          bptr += sizeof(rumStreamID_t) ; 
          n++ ; 
        }
      }
      k = (int)(bptr - cInfo->rx_str_rep_tx_packet) - INT_SIZE ; 
      bptr = cInfo->rx_str_rep_tx_packet ;
      NputInt(bptr,k) ; 
      bptr += sizeof(pgmHeaderCommon) ; 
      NputInt(bptr,n) ; 
      cInfo->str_rep_tx_pending = 1 ; 
      cInfo->next_rx_str_rep_time = curTime + rInfo->aConfig.StreamReportIntervalMilli ;
    }
    else
      nexTime = curTime+20 ; 
    pthread_mutex_unlock(&cInfo->mutex) ;
  }

  rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ; 
  rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ; 

  return nexTime ; 
}

/** ++++++++++++++++++++++ */

rmm_uint64 chkHBTO(rmmReceiverRec *rInfo)
{
  int i, nsid ; 
  rmm_uint64 nexTime ; 
  rTopicInfoRec *ptp ;
  rStreamInfoRec *pst ;
  //rumInstanceRec *gInfo ;
  rmmStreamID_t sids[MAX_STREAMS] ; 
  TCHandle tcHandle = rInfo->tcHandle;
  char* eventDescription;

  //gInfo = rInfo->gInfo ; 
  nexTime = rInfo->CurrentTime + 250 ; 
  nsid = 0 ; 
  for ( pst=rInfo->firstStream ; pst ; pst=pst->next )
  {
    if ( !pst->active )
    {
      if ( rInfo->CurrentTime > pst->close_time )
        sids[nsid++] = pst->sid ; 
      else
      if ( nexTime > pst->close_time )
           nexTime = pst->close_time ;
    }
  }

  if ( nsid )
  {
    for ( i=0 ; i<nsid ; i++ )
    {
      if ( (pst = find_stream(rInfo,sids[i])) == NULL )
        continue ; 
      eventDescription = "The RUM receiver deletes the stream because a close SCP was received.";
      ptp = rInfo->rTopics[pst->topic_id] ;
      if ( ptp->on_event )
      {
        void *EvPrms[2] ; EvPrms[0]=eventDescription; 
        raise_stream_event(pst, RMM_HEARTBEAT_TIMEOUT, EvPrms, 1) ;
      }
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5273),"%s%llu",
        "The RUM receiver deletes the stream {0} because a close SCP was received. (nmsg={1}).",pst->sid_str,UINT_64_TO_LLU(pst->TotMsgsOut));
    }
    rmm_rwlock_r2wlock(&rInfo->GlobalSync.rw) ;
    for ( i=0 ; i<nsid ; i++ )
    {
      if ( (pst = find_stream(rInfo,sids[i])) == NULL )
        continue ; 
      remove_stream(pst) ;
      delete_stream(pst) ;
    }
    rmm_rwlock_w2rlock(&rInfo->GlobalSync.rw) ;
  }

  return nexTime ; 
}


int PutRumEvent(rumInstanceRec *gInfo, rumEvent *ev, rum_on_event_t on_event, void *user)
{
  int i,l,n,iError , iok ; 
  char *ip, *iq ; 
  RumEventInfo *evi ; 
  EventInfo *buf=NULL ; 
  rmmReceiverRec *rInfo=NULL ; 
  iok = 0 ; 
  
  if(on_event==NULL) return 0;

  do
  {
    if ( !gInfo )
      break ; 
    rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
  
    if ( !rInfo )
      break ; 
  
    if ( !rInfo->GlobalSync.eaUP )
      break ; 
    
    if ( (buf=(EventInfo *)MM_get_buff(rInfo->evntStrucQ,1,&iError)) == NULL )
      break ; 
    iok++ ; 
    
    buf->evType = RUM_EVENT_ID ; 
    evi = &buf->u.rumEv ; 
  
    memcpy(&evi->ev,ev,sizeof(rmmEvent)) ; 
    evi->on_event = on_event ; 
    evi->user = user ; 
    if ( ev->nparams > MAX_NUM_PRMS )
      evi->ev.nparams = MAX_NUM_PRMS ; 
    evi->ev.event_params = (void **)evi->prm ; 
    n = sizeof(evi->prms) ;
    for ( i=0 , ip=(char *)evi->prms ; n>0 && i<evi->ev.nparams ; i++ )
    {
      evi->prm[i] = ip ; 
      iq = ev->event_params[i] ; 
      if ( i==0 )
      {
        l = (int)strlcpy(ip,iq,n) ; 
        l = (l+8)&~7 ; 
      }
      else
      {
        l = get_param_size(ev->type,i) ; 
        if ( l > n ) l = n ; 
        memcpy(ip,iq,l) ; 
        l = (l+7)&~7 ; 
      }
      ip += l ; 
      n  -= l ; 
    }
  
    BB_lock(rInfo->rmevQ) ; 
    while ( BB_put_buff(rInfo->rmevQ,buf,0) == NULL )
    {
      if ( rInfo->GlobalSync.goDN )
      {
        iok-- ; 
        break ; 
      }
      BB_waitF(rInfo->rmevQ) ; 
    }
    BB_unlock(rInfo->rmevQ) ; 
    rmm_cond_signal(&rInfo->eaWcw,1) ;
    iok++ ; 
  } while ( 0 ) ; 
  if ( iok < 2 )
  {
    if ( iok > 0 )
      MM_put_buff(rInfo->evntStrucQ,buf) ;
    on_event(ev,user) ; 
  }
  return ( iok==2 ) ; 
}

void PutConEvent(rumInstanceRec *gInfo, rumConnectionEvent *ev, rum_on_connection_event_t on_event, void *user)
{
  int i,n , iError ; 
  char *ip, *iq ; 
  ConEventInfo *evi ; 
  EventInfo *buf ; 
  char *msg=NULL ; 
  rmmReceiverRec *rInfo ; 

  if ( !gInfo )
  {
    on_event(ev,user) ; 
    return ;
  }
  rInfo = (rmmReceiverRec *)gInfo->rInfo ; 

  if ( !rInfo )
  {
    on_event(ev,user) ; 
    return ;
  }

  if ( !rInfo->GlobalSync.eaUP )
  {
    on_event(ev,user) ; 
    return ;
  }
  
  if ( (buf=(EventInfo *)MM_get_buff(rInfo->evntStrucQ,1,&iError)) == NULL )
  {
    on_event(ev,user) ; 
    return ;
  }

  if ( ev->connect_msg != NULL && ev->msg_len > 0 )
  {
    if ( (msg = malloc(ev->msg_len)) == NULL )
    {
      on_event(ev,user) ; 
      MM_put_buff(rInfo->evntStrucQ,buf) ;
      return ;
    }
  }

  buf->evType = CON_EVENT_ID ; 
  evi = &buf->u.conEv ; 

  memcpy(&evi->ev,ev,sizeof(rumConnectionEvent)) ; 
  evi->on_event = on_event ; 
  evi->user = user ; 
  if ( ev->nparams > MAX_NUM_PRMS )
    evi->ev.nparams = MAX_NUM_PRMS ; 
  evi->ev.event_params = (void **)evi->prm ; 
  n = sizeof(evi->prms) ;
  for ( i=0 , ip=(char *)evi->prms ; n>0 && i<evi->ev.nparams ; i++ )
  {
    evi->prm[i] = ip ; 
    iq = ev->event_params[i] ; 
    if ( i==0 )
    {
      while ( n-- && *iq )
        *ip++ = *iq++ ; 
      if ( !n )
        ip-- ; 
      *ip++ = 0 ; 
    }
    else
    {
      memcpy(ip,iq,(n<ONE_PRM_SIZE)?n:ONE_PRM_SIZE) ; 
      ip += ONE_PRM_SIZE ; 
      n  -= ONE_PRM_SIZE ; 
    }
  }

  if ( ev->connect_msg != NULL && ev->msg_len > 0 )
  {
    evi->ev.connect_msg = msg ; 
    memcpy(evi->ev.connect_msg,ev->connect_msg,ev->msg_len) ; 
  }
  else
    evi->ev.connect_msg = NULL ; 

  BB_lock(rInfo->rmevQ) ; 
  while ( BB_put_buff(rInfo->rmevQ,buf,0) == NULL )
  {
    if ( rInfo->GlobalSync.goDN )
    {
      BB_unlock(rInfo->rmevQ) ; 
      if ( evi->ev.connect_msg != NULL )
        free(evi->ev.connect_msg) ; 
      MM_put_buff(rInfo->evntStrucQ,buf) ;
      on_event(ev,user) ; 
      return ; 
    }
    BB_waitF(rInfo->rmevQ) ; 
  }
  BB_unlock(rInfo->rmevQ) ; 
  rmm_cond_signal(&rInfo->eaWcw,1) ;

}

void PutFcbEvent(rumInstanceRec *gInfo, rum_free_callback_t free_callback, void *user)
{
  int iError ; 
  FcbEventInfo *evi ; 
  EventInfo *buf ; 
  rmmReceiverRec *rInfo ; 

  if ( free_callback==NULL || user==NULL )
    return ;

  if ( !gInfo )
  {
    free_callback(user) ; 
    return ;
  }
  rInfo = (rmmReceiverRec *)gInfo->rInfo ; 

  if ( !rInfo )
  {
    free_callback(user) ; 
    return ;
  }

  if ( !rInfo->GlobalSync.eaUP )
  {
    free_callback(user) ; 
    return ;
  }

  if ( (buf=(EventInfo *)MM_get_buff(rInfo->evntStrucQ,16,&iError)) == NULL )
  {
    /* free_callback(user) ;  */ /* better memory leak than sigmentation fault! */
    return ;
  }

  buf->evType = FCB_EVENT_ID ; 
  evi = &buf->u.fcbEv ; 

  evi->free_callback = free_callback ; 
  evi->user = user ; 

  BB_lock(rInfo->rmevQ) ; 
  while ( BB_put_buff(rInfo->rmevQ,buf,0) == NULL )
  {
    if ( rInfo->GlobalSync.goDN )
    {
      BB_unlock(rInfo->rmevQ) ; 
      MM_put_buff(rInfo->evntStrucQ,buf) ;
      free_callback(user) ; 
      return; 
    }
    BB_waitF(rInfo->rmevQ) ; 
  }
  BB_unlock(rInfo->rmevQ) ; 
  rmm_cond_signal(&rInfo->eaWcw,1) ;

}

/** ++++++++++++++++++++++ */

int ea_free_ptr(void *ptr)
{
  free(ptr) ; 
  return 0 ; 
}

/** ++++++++++++++++++++++ */

void event_delivered(rmmReceiverRec *rInfo, EventInfo *evi, int rc, int eaCall)
{
  RumEventInfo *rev ; 
  rStreamInfoRec *pst ; 

  if ( evi->evType == RUM_EVENT_ID )
  {
    rev = &evi->u.rumEv ; 
    if ( rev->ev.type == RUM_NEW_SOURCE )
    {
      rmm_rwlock_rdlock(&rInfo->GlobalSync.rw) ; 
      if ( (pst = find_stream(rInfo,rev->ev.stream_id)) != NULL )
      {
        pst->ns_event = 1 ; 
        if ( pst->mtl_offset )
          wakeMA(rInfo,pst) ; 
      }
      rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ; 
    }
  }
}


/** ++++++++++++++++++++++ */

THREAD_RETURN_TYPE EventAnnouncer(void *arg)
{
  int n ; 
  EventInfo *evi ; 
  RumEventInfo *rev ; 
  LogEventInfo *lev ; 
  ConEventInfo *cev ; 
  FcbEventInfo *fcb ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)arg;
  TCHandle tcHandle = rInfo->tcHandle;

 #if USE_PRCTL
  {
    char tn[16] ; 
    rmm_strlcpy(tn,__FUNCTION__,16);
    prctl( PR_SET_NAME,(uintptr_t)tn);
  }
 #endif

  pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
  rInfo->GlobalSync.eaUP = 1 ;
  pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5069),"%s%llu%d",
    "The {0} thread is running. Thread id: {1} ({2}).",
    "EventAnnouncer",LLU_VALUE(my_thread_id()),
#ifdef OS_Linuxx
    (int)gettid());
#else
    (int)my_thread_id());
#endif

  while ( !rInfo->GlobalSync.goDN )
  {
    do
    {
      n = 0 ; 
      while ( (evi=(EventInfo *)BB_get_buff(rInfo->rmevQ,1)) != NULL )
      {
        BB_signalF(rInfo->rmevQ) ; 
        if ( evi->evType == CON_EVENT_ID )
        {
          cev = &evi->u.conEv ; 
          if ( cev && cev->on_event )
          {
            cev->on_event(&cev->ev,cev->user) ; 
            if ( cev->ev.connect_msg != NULL )
              free(cev->ev.connect_msg) ; 
          }
        }
        else
        if ( evi->evType == RUM_EVENT_ID )
        {
          rev = &evi->u.rumEv ; 
          if ( rev && rev->on_event )
          {
            rev->on_event(&rev->ev,rev->user) ; 
            event_delivered(rInfo, evi, 0, 1) ; 
          }
        }
        else
        if ( evi->evType == FCB_EVENT_ID )
        {
          fcb = &evi->u.fcbEv ; 
          if ( fcb && fcb->free_callback && fcb->user )
            fcb->free_callback(fcb->user) ; 
        }
        MM_put_buff(rInfo->evntStrucQ,evi) ;
        n++ ; 
      }
    
      if    ( (evi=(EventInfo *)BB_get_buff(rInfo->lgevQ,1)) != NULL )
      {
        BB_signalF(rInfo->lgevQ) ; 
        if ( evi->evType == LOG_EVENT_ID )
        {
          lev = &evi->u.logEv ; 
          lev->on_event(&lev->ev,lev->user) ; 
        }
        MM_put_buff(rInfo->evntStrucQ,evi) ;
        n++ ; 
      }
    } while ( n ) ; 

    rmm_cond_wait(&rInfo->eaWcw,1) ;
  }

  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5067),"%s%llu",
    "The {0}({1}) thread was stopped.",
    "EventAnnouncer", LLU_VALUE(my_thread_id()));

  pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
  rInfo->GlobalSync.eaUP = 0 ;
  pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
  rmm_thread_exit() ;
}

/** ++++++++++++++++++++++ */

/** ++++++++++++++++++++++ */

THREAD_RETURN_TYPE NAKGenerator(void *arg)
{
 int iError ,n , m , rc  , nakQi ; 
 pgm_seq tail, head , sn , *nak_sqn_list=NULL ;
 char sq_flag , nakORncf ;
 int sleepTime ; 
 rStreamInfoRec *pst ;
 NackInfoRec *pnk ;
 rmmReceiverRec *rInfo = (rmmReceiverRec *)arg;
 const char* methodName = "NAKGenerator";
 TCHandle tcHandle = rInfo->tcHandle;

 #if USE_PRCTL
  {
    char tn[16] ; 
    rmm_strlcpy(tn,__FUNCTION__,16);
    prctl( PR_SET_NAME,(uintptr_t)tn);
  }
 #endif

  pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
  rInfo->GlobalSync.ngUP = 1 ;
  pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5069),"%s%llu%d",
    "The {0} thread is running. Thread id: {1} ({2}).",
    "NAKGenerator",LLU_VALUE(my_thread_id()),
#ifdef OS_Linuxx
    (int)gettid());
#else
    (int)my_thread_id());
#endif

  buildNsendNAK = buildNsendNAK_pgm ;

  while ( rInfo->MaxSqnPerNack > 0 &&
          (nak_sqn_list = malloc((rInfo->MaxSqnPerNack+4)*sizeof(pgm_seq))) == NULL )
    rInfo->MaxSqnPerNack-- ;
  if ( rInfo->MaxSqnPerNack <= 0 )
  {
    LOG_MEM_ERR(tcHandle,methodName,((rInfo->MaxSqnPerNack+4)*sizeof(pgm_seq)));

    pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
    rInfo->GlobalSync.ngUP = 0 ;
    pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
    rmm_thread_exit() ;
  }

  nakORncf = (SQ_FLAG_PP_NAK|SQ_FLAG_PP_NCF) ;

  sleepTime = 1+milli2nano(rInfo->aConfig.NackGenerCycle)/2 ; 

  while ( 1 )
  {
    /* rmm_cond_wait(&rInfo->nakTcw,1) ; */
    tsleep(0,sleepTime) ;

    if ( rInfo->GlobalSync.goDN )
      break ;

    if ( rInfo->NextNakTime >= rInfo->CurrentTime &&
         rInfo->LastNakAlertCount>= rInfo->NakAlertCount )
      continue ; 

    rInfo->LastNakAlertCount = rInfo->NakAlertCount ;
    rInfo->NextNakTime = rInfo->CurrentTime + rInfo->aConfig.NackGenerCycle ;

    if ( rmm_rwlock_rdlock(&rInfo->GlobalSync.rw) == RMM_FAILURE )
      continue ; 

    for ( pst=rInfo->firstStream ; pst ; pst=pst->next )
    {
      if ( pst->killed )
        continue ; 
      pst->ng_last_time = rInfo->CurrentTime ;
      /* the rest is only done for "NAKable" streams */
      if (!(pst->mtl_offset || pst->have_lj) ||
         !(pst->keepHistory && pst->reliable && pst->path_nla.length ) )  /* NAKs are not allowed otherwise */
      {
        continue ;
      }

     /*  Advance the nackSQ to the current trail, throwing unneeded nack elements */

      tail = SQ_get_tailSN(pst->dataQ,1) ;
      if ( XltY(tail,pst->rxw_trail) )
           tail = pst->rxw_trail ;
      for ( sn=SQ_get_tailSN(pst->nakSQ,0) , n=0 ; XltY(sn,tail) ; sn++ )
      {
        if ( (pnk = SQ_inc_tail(pst->nakSQ,1)) != NULL )
        {
          MM_put_buff(rInfo->nackStrucQ,pnk) ;
          n++ ;
        }
      }
      pst->nakQn -= n ;
      pst->nak_stat[2] += n ;
      for ( sn=SQ_get_tailSN(pst->nakSQ,0) ; XltY(sn,pst->ng_last) ; sn++ )
      {
        if ( (pnk=SQ_get_buff(pst->nakSQ,0,sn,&iError)) != NULL )
          break ;
        if ( iError != 0 )
          break ;
        SQ_inc_tail(pst->nakSQ,1) ;
      }

     /*  Scan the dataSQ for missing packets, creating new nack elements */

      tail = SQ_get_tailSN(pst->nakSQ, 0) ;
      if ( XltY(tail,pst->ng_last+1) )
        tail = pst->ng_last+1 ;
      head = SQ_get_headSN(pst->nakSQ, 0) ;
      if ( XgtY(head,pst->rxw_lead) )
        head = pst->rxw_lead ;

      for ( sn=tail ; !XgtY(sn,head) && pst->nakQn<rInfo->nackQsize ; sn++ )
      {
        if ( !SQ_get_buff(pst->dataQ,1,sn,&iError) )
        {
          if ( iError == 0 )
          {
            if ( (pnk = MM_get_buff(rInfo->nackStrucQ,5,&iError)) == NULL )
            {
             /*
              snprintf(LogMsg[0],LOG_MSG_SIZE," Failed to allocate buffer from nackStrucQ");
              rmmLogger(0,rInfo,myName,LogMsg,RMM_D_GENERAL) ;
             */
              break ;
            }
            memset(pnk,0,sizeof(NackInfoRec)) ;
            pnk->pSN   = sn ;
            pnk->state = NAK_BACK_OFF_STATE ;
            pnk->timer = rInfo->CurrentTime ;
            pnk->rtt_time = rInfo->CurrentTime ;

            sq_flag = SQ_OR_flag(pst->nakSQ,0,pnk->pSN,&iError,0) ;
            if ( sq_flag&nakORncf )
            {
              pnk->NewNakNcfTime = rInfo->CurrentTime ;
              pnk->flag          = sq_flag ;
            }

            if ( SQ_put_buff(pst->nakSQ,0,sn,&iError,pnk) <= 0 )
            {
              MM_put_buff(rInfo->nackStrucQ,pnk) ;
              break ;
            }
            pst->nakQn++ ;
            pst->nak_stat[0]++ ;
          }
          else
            break ;
        }
      }
      pst->ng_last = sn-1 ;

     /* Update retransmision timeout intervals  */

      if ( pst->ng_num_toN > 500 )
      {
        n = 6*(pst->ng_sum_toN / pst->ng_num_toN)/2  ;
        n = (15*n + 85*pst->NackTimeoutNCF)/100 ;
        if ( n > 4*rInfo->aConfig.NackTimeoutNCF )
             n = 4*rInfo->aConfig.NackTimeoutNCF ;
        else
        if ( n <   rInfo->aConfig.NackTimeoutNCF/4 )
             n =   rInfo->aConfig.NackTimeoutNCF/4 ;
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5277),"%d%d",
          "The NackTimeOutNCF configuration parameter is changed from {0} to {1}.",  pst->NackTimeoutNCF,n);
        pst->NackTimeoutNCF = n ;
        pst->ng_num_toN = 0 ;
        pst->ng_sum_toN = 0 ;
      }
      if ( pst->ng_num_toD > 500 )
      {
        n = 6*(pst->ng_sum_toD / pst->ng_num_toD)/2  ;
        n = (15*n + 85*pst->NackTimeoutData) / 100 ;
        if ( n > 4*rInfo->aConfig.NackTimeoutData )
             n = 4*rInfo->aConfig.NackTimeoutData ;
        else
        if ( n <   rInfo->aConfig.NackTimeoutData/4 )
             n =   rInfo->aConfig.NackTimeoutData/4 ;
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5278),"%d%d",
          "The NackTimeOutData configuration parameter is changed from {0} to {1}.",  pst->NackTimeoutData,n);

        pst->NackTimeoutData = n ;
        pst->ng_num_toD = 0 ;
        pst->ng_sum_toD = 0 ;
      }

     /* Go over nak elements, check if data arrived, update timer, send NAKs */

      m = 0 ;
      if ( (n=pst->nakQn) > rInfo->aConfig.MaxNacksPerCycle )
        n = rInfo->aConfig.MaxNacksPerCycle ;
      tail = SQ_get_tailSN(pst->nakSQ, 0) ;
      head = SQ_get_headSN(pst->nakSQ, 0) ;

      for ( sn= tail , nakQi=0 ; XltY(sn,head) && nakQi<n ; sn++ )
      {
        if ( (pnk=SQ_get_buff(pst->nakSQ,0,sn,&iError)) != NULL )
        {
          nakQi++ ;
          rc = check_nak_element(pnk, pst) ;
          switch ( rc )
          {
            case 1 :
              if ( pnk->NcfRetryCount+pnk->DataRetryCount )
                pst->nak_stat[6]++ ;
              nak_sqn_list[m++] = pnk->pSN ;
              if ( m >= rInfo->MaxSqnPerNack )
              {
                if ( buildNsendNAK(pst,m,nak_sqn_list,1) == RMM_FAILURE )
                {
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3279),"",
                      "The RUM receiver failed to send a NAK.");
                }
                m = 0 ;
              }
             /*  falling through... */
            case 0 :
              break ;
            case -1:
              SQ_put_buff(pst->nakSQ,1,pnk->pSN,&iError,NULL) ;
              MM_put_buff(rInfo->nackStrucQ,pnk) ;
              pst->nakQn-- ;
              break ;
            default:
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5280),"%d",
                "The check_nak_element method of the RUM receiver did not return a valid return code. Return code: {0}.",rc);
              break ;
          }
        }
      }
      if ( m > 0 )
      {
        if ( buildNsendNAK(pst,m,nak_sqn_list,1) == RMM_FAILURE )
        {
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3281),"",
            "The RUM receiver failed to send a NAK.");
        }
        m = 0 ;
      }
    }
    rmm_rwlock_rdunlock(&rInfo->GlobalSync.rw) ;
  }

  free( nak_sqn_list ) ; 

  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5067),"%s%llu",
    "The {0}({1}) thread was stopped.",
    "NAKGenerator", THREAD_ID(pthread_self()));

  pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
  rInfo->GlobalSync.ngUP = 0 ;
  pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
  rmm_thread_exit() ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

int check_nak_element(NackInfoRec *pnk, rStreamInfoRec *pst)
{
 int iError ;
 char nakORncf = (SQ_FLAG_PP_NAK|SQ_FLAG_PP_NCF) ;
  rmmReceiverRec *rInfo = rInstances[pst->instance_id];
  TCHandle tcHandle = rInfo->tcHandle;

  if ( SQ_get_buff(pst->dataQ,1,pnk->pSN,&iError)  != NULL )
  {
   /*
    snprintf(LogMsg[0],LOG_MSG_SIZE," dataOK for sqn  %d ; ",pnk->pSN) ;
    rmmLogger(0,rInfo,myName,LogMsg,RMM_D_GENERAL) ;
   */
    if ( !(pnk->NcfRetryCount + pnk->DataRetryCount) )
    {
      pst->ng_sum_toD += (int)(rInfo->CurrentTime - pnk->rtt_time) ;
      pst->ng_num_toD++ ;
    }
    pst->nak_stat[1]++ ;
    return -1 ;
  }
  if ( iError != 0 )
  {
   /*
    snprintf(LogMsg[0],LOG_MSG_SIZE," trail advanced: %d ; ",pnk->pSN) ;
    rmmLogger(0,rInfo,myName,LogMsg,RMM_D_GENERAL) ;
   */
    pst->nak_stat[2]++ ;
    return -1 ;
  }
  if ( XltY(pnk->pSN,pst->rxw_trail) )
  {
   /**/
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(7283),"%u",
      "Trail advanced={0}.", pnk->pSN);
   /**/
    pst->nak_stat[2]++ ;
    return -1 ;
  }

  if ( pnk->OldNakNcfTime < pnk->NewNakNcfTime )
  {
    pnk->OldNakNcfTime = pnk->NewNakNcfTime ;
    if ( pnk->flag&nakORncf )
    {
      pnk->timer = pnk->NewNakNcfTime ;
      if ( pnk->flag&SQ_FLAG_PP_NCF )
      {
        pnk->state =  NAK_WAIT_DATA_STATE ;
        if ( !(pnk->NcfRetryCount + pnk->DataRetryCount) )
        {
          pst->ng_sum_toN += (int)(rInfo->CurrentTime - pnk->rtt_time) ;
          pst->ng_num_toN++ ;
        }
      }
      else
      if ( pnk->state == NAK_BACK_OFF_STATE )
        pnk->state = NAK_WAIT_NCF_STATE ;
    }
  }

  if ( rInfo->MemoryCrisisMode && (int)(pnk->pSN-SQ_get_tailSN(pst->dataQ,1)) > BB_get_nBuffs(rInfo->rsrvQ,0) )
    return 0 ;

  if ( pnk->state == NAK_BACK_OFF_STATE )
  {
    if ( (rInfo->CurrentTime - pnk->timer) >= rInfo->aConfig.NackTimeoutBOF )
    {
      pnk->state = NAK_WAIT_NCF_STATE ;
      pnk->timer = rInfo->CurrentTime ;
      pnk->rtt_time = rInfo->CurrentTime ;
      return 1 ;
    }
    return 0 ;
  }

  if ( pnk->state == NAK_WAIT_NCF_STATE )
  {
    if ( (rInfo->CurrentTime - pnk->timer) >= pst->NackTimeoutNCF )
    {
      if ( pnk->NcfRetryCount++ > rInfo->aConfig.NackRetriesNCF )
      {
       /*
        snprintf(LogMsg[0],LOG_MSG_SIZE," NAK_NCF_RETRIES reached!! ") ;
        rmmLogger(0,rInfo,myName,LogMsg,RMM_D_GENERAL) ;
       */
        pst->nak_stat[3]++ ;
        return -1 ;
      }
      pnk->state = NAK_BACK_OFF_STATE ;
    }
    return 0 ;
  }

  if ( pnk->state == NAK_WAIT_DATA_STATE )
  {
    if ( (rInfo->CurrentTime - pnk->timer) >= pst->NackTimeoutData )
    {
      if ( pnk->DataRetryCount++ > rInfo->aConfig.NackRetriesData )
      {
       /*
        snprintf(LogMsg[0],LOG_MSG_SIZE," NAK_DATA_RETRIES reached!! ") ;
        rmmLogger(0,rInfo,myName,LogMsg,RMM_D_GENERAL) ;
       */
        pst->nak_stat[4]++ ;
        return -1 ;
      }
      pnk->state = NAK_BACK_OFF_STATE ;
    }
    return 0 ;
  }
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3284),"%d",
    "The NAK state {0} is not valid", pnk->state);
  return -1 ;

}

/** !!!!!!!!!!!!!!!!!!!!!! */

THREAD_RETURN_TYPE MessageAnnouncer(void *arg)
{
  int rc ;
  rmmReceiverRec *rInfo = (rmmReceiverRec *)arg;
  TCHandle tcHandle = rInfo->tcHandle;
  char errMsg[ERR_MSG_SIZE];

 #if USE_PRCTL
  {
    char tn[16] ; 
    rmm_strlcpy(tn,__FUNCTION__,16);
    prctl( PR_SET_NAME,(uintptr_t)tn);
  }
 #endif

  pthread_mutex_lock(&rInfo->maMutex) ;
  rInfo->GlobalSync.maUP++ ;
  pthread_mutex_unlock(&rInfo->maMutex) ;

  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5069),"%s%llu%d",
    "The {0} thread is running. Thread id: {1} ({2}).",
    "MessageAnnouncer",LLU_VALUE(my_thread_id()),
#ifdef OS_Linuxx
    (int)gettid());
#else
    (int)my_thread_id());
#endif

  if ( rmm_get_thread_priority(pthread_self(),errMsg,ERR_MSG_SIZE) == RMM_SUCCESS )
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6285),"%s",
      "Get thread priority status: {0}.", errMsg);


  rc=_MessageAnnouncer(rInfo) ;
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5286),"%s",
    "The {0} service is stopped. ","MessageAnnouncer" );
  if ( !rc )
    pthread_mutex_lock(&rInfo->maMutex) ;
  rInfo->GlobalSync.maUP-- ;
  pthread_mutex_unlock(&rInfo->maMutex) ;
  rmm_thread_exit() ;
}

int _MessageAnnouncer(rmmReceiverRec *rInfo)
{
  int m, n, npk=0 , nps ;
  pgm_seq sn=0 , rxw_trail;
  char * packet=NULL ;
  rTopicInfoRec  *ptp=NULL ;
  rStreamInfoRec *pst=NULL ;
  //rumInstanceRec *gInfo ;
  TCHandle tcHandle = rInfo->tcHandle;
  char* eventDescription;

  //gInfo = rInfo->gInfo ; 

  while ( !rInfo->GlobalSync.goDN )  /* BEAM suppression: infinite loop */
  {
    LL_lock(rInfo->mastQ) ;
      if ( pst && ptp )
      {
        ptp->maIn = 0 ;
        pst->maIn = 0 ;
        if ( npk && !pst->inMaQ )
        {
          pst->inMaQ = 1 ;
          LL_put_buff(rInfo->mastQ,pst,0) ;
        }
      }
  
      while ( 1 )
      {
        n = LL_get_nBuffs(rInfo->mastQ,0) ;
        while ( n-- )
        {
          pst = (rStreamInfoRec *)LL_get_buff(rInfo->mastQ,0) ;
          ptp = rInfo->rTopics[pst->topic_id] ;
          if ( ptp->closed || pst->killed )
            continue ; 
          if ( !ptp->maIn && pst->ns_event )
            break ;
          LL_put_buff(rInfo->mastQ,pst,0) ;
        }
        if ( n >= 0 )
          break ;
        rInfo->ma_wfb++ ; 
        LL_waitE(rInfo->mastQ) ;
        rInfo->ma_wfb-- ; 
        if ( rInfo->GlobalSync.goDN )
        {
          LL_unlock(rInfo->mastQ) ;
          return 0 ;
        }
      }
      LL_signalF(rInfo->mastQ) ;
      pst->inMaQ = 0 ;
      pst->maIn = 1 ;
      ptp->maIn = 1 ;
    LL_unlock(rInfo->mastQ) ;


    pst->ma_last_time = rInfo->CurrentTime ;
    npk = 0 ;
    m = 0 ;
    n = 0 ;
    nps = 16 ; /* pst->ChunkSize ; */

    {
      while ( m<nps && !ptp->closed )
      {
        rxw_trail = pst->rxw_trail ;
        if ( (packet = SQ_get_tailPP(pst->dataQ,&sn,1)) != NULL )
        {
          if ( ptp->parseMTL(pst,packet) )
          {
            MM_put_buff(rInfo->dataBuffsQ,packet) ;
            if ( rInfo->aConfig.UsePerConnInQ )
              rumR_PerConnInQdn(rInfo, pst) ; 
          }
          m++ ;
        }
        else
        {
          if ( XltY(sn,rxw_trail) )
             /*
              (XltY(sn,pst->ng_last) && SQ_get_buff(pst->nakSQ,1,sn,&iError) == NULL) )
             */
          {
            SQ_inc_tail(pst->dataQ,1) ;
            n++ ;
          }
          else
            break ;
        }
      }
      pst->rxw_tail += (m+n) ;
      npk = ( m>=nps ) ;
      rInfo->TotPacksOut += m ; 
    }
    ptp->TotPacksIn  += m ;
    pst->TotPacksOut += m ;
    pst->TotVisits++ ;
    if ( n ) /* && !ptp->failover ) */
    {
      pst->TotPacksLost += n ;
      eventDescription = "Unrecoverable packet loss.";
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4287),"%d%s%u%u%u",
        "The packet loss of {0} packets on stream {1} is not recoverable. (sn={2}, trail={2}, lead={3}) ",n,pst->sid_str,sn,pst->rxw_trail,pst->rxw_lead );
      if ( ptp->on_event )
      {
        void *EvPrms[2] ; EvPrms[0]=eventDescription; EvPrms[1]=&n; 
        raise_stream_event(pst, RMM_PACKET_LOSS, EvPrms, 2) ;
      }
    }
  }

  return 0 ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

rStreamInfoRec *create_stream(rmmStreamID_t sid, ipFlat *sip, rTopicInfoRec *ptp, int topicLen, char *topicName,
             uint16_t src_port, uint16_t dst_port, uint32_t gsi_high,
             uint16_t gsi_low, pgm_seq rxw_trail, pgm_seq rxw_lead)
{
  int ok;
 rStreamInfoRec *pst ;
  rmmReceiverRec *rInfo=rInstances[ptp->instance_id] ;
  const char* methodName = "create_stream";
  TCHandle tcHandle = rInfo->tcHandle;


  if ( rInfo->rNumStreams >= MAX_STREAMS )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3288),"%d",
      "The RUM receiver cannot accept more streams. The maximum number of streams is {0}.", MAX_STREAMS );
    pst = (rStreamInfoRec *)NULL ;
    return pst ;
  }

  pst = NULL ; 
  pthread_mutex_lock(&rInfo->pstsQlock) ;
  if ( rInfo->pstsQfirst )
  {
    pst = rInfo->pstsQfirst ; 
    rInfo->pstsQfirst = pst->next ; 
  }
  pthread_mutex_unlock(&rInfo->pstsQlock) ;
  if ( !pst ) 
    pst = (rStreamInfoRec *)malloc(sizeof(rStreamInfoRec)) ; 
  if ( !pst ) 
  {
    LOG_MEM_ERR(tcHandle,methodName,(sizeof(rStreamInfoRec)));
    return pst ;
  }
  memset(pst,0,sizeof(rStreamInfoRec)) ;
  /* Note: only non-zeros should be initialized below */

  pst->sid        = sid ;
  memcpy(pst->sid_chr,&pst->sid,sizeof(pst->sid)) ;
  b2h(pst->sid_str,(uint8_t *)&pst->sid,sizeof(pst->sid)) ;
  pst->instance_id= ptp->instance_id ;
  pst->topic_id   = ptp->topic_id ;
  pst->active     = 1 ;
  pst->reliable   = ptp->reliability ;
  pst->TotPacksIn = 0 ;
  pst->TotPacksOut= 0 ;
  pst->ChunkSize  = 64 ;

  pst->NackTimeoutNCF = rInfo->aConfig.NackTimeoutNCF ;
  pst->NackTimeoutData= rInfo->aConfig.NackTimeoutData;

  pst->pp_last_time = rInfo->CurrentTime ; 

  pst->rxw_trail  = rxw_trail ; /*** TODO late_join trail   ***/
  pst->rxw_lead   = rxw_lead ;

  pst->src_port   = src_port ;
  pst->dest_port  = dst_port ;
  pst->gsi_high   = gsi_high ;
  pst->gsi_low    = gsi_low  ;

  pst->src_nla.length = sip->length ; 
  memcpy(pst->src_nla.bytes,sip->bytes,pst->src_nla.length) ; 

  if ( pst->src_nla.length == 4 )
    rmm_ntop(AF_INET ,pst->src_nla.bytes,pst->sIP_str,RUM_ADDR_STR_LEN) ; 
  else
    rmm_ntop(AF_INET6,pst->src_nla.bytes,pst->sIP_str,RUM_ADDR_STR_LEN) ; 

  pst->topicLen   = topicLen ;
  memcpy(pst->topicName,topicName,topicLen) ;
  pst->topicName[topicLen] = 0 ;

  pst->path_nla = pst->src_nla ; 

  ok = 0 ; 
  do
  {
    int i ; 
    if ( pthread_mutex_init(&pst->ppMutex, NULL) != 0 )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3289),"",
      "The RUM receiver failed to initialize the pthread mutex variable." );
      break ;
    }
    if ( (pst->dataQ = SQ_alloc(rInfo->dataQsize,pst->rxw_trail)) == NULL )
    {
      LOG_MEM_ERR(tcHandle,methodName,(rInfo->dataQsize));
      break ;
    }
  
    if ( (pst->nakSQ = SQ_alloc(rInfo->dataQsize,pst->rxw_trail)) == NULL )
    {
      LOG_MEM_ERR(tcHandle,methodName,(rInfo->dataQsize));
      break ;
    }
  
    i = offsetof(FragMsgInfoRec,ll_next) ;
    if ( (pst->fragQ = LL_alloc(i,0) ) == NULL )
    {
      LOG_MEM_ERR(tcHandle,methodName,(rInfo->fragQsize));
      break ;
    }
  
    if ( ptp->on_packet )
    {
  
      if ( (pst->pck = malloc(sizeof(rumPacket))) == NULL )
      {
        LOG_MEM_ERR(tcHandle,methodName,(sizeof(rumPacket)));

        break ;
      }
      pst->pck->rmm_pst_reserved = pst ; 
      pst->pck->stream_info = &pst->si ;
      for ( i=0 ; i<RUM_MAX_MSGS_IN_PACKET ; i++ )
        pst->pck->msgs_info[i].stream_info = &pst->si ;
    }
  
    pst->si.stream_id = pst->sid ; 
    pst->si.port = ntohs(pst->gsi_low) ;
    pst->si.source_addr = pst->sIP_str ;
    pst->si.queue_name = pst->topicName ;
  
    pst->msg.stream_info = &pst->si ;
    pst->om_user = ptp->om_user ; 
  
    pst->ev.stream_id = pst->sid ; 
    pst->ev.port = ntohs(pst->gsi_low) ;
    strlcpy(pst->ev.source_addr,pst->sIP_str,64) ;
    memcpy(pst->ev.queue_name,pst->topicName,pst->topicLen) ;
    pst->ev.queue_name[pst->topicLen] = 0 ;
    ok = 1 ; 
  } while(0) ; 
  if ( !ok )
  {
    if ( pst->dataQ ) SQ_free(pst->dataQ,0) ;
    if ( pst->nakSQ ) SQ_free(pst->nakSQ,0) ;
    if ( pst->fragQ ) LL_free(pst->fragQ,0) ;
    if ( pst->pck ) free(pst->pck) ;
    pthread_mutex_lock(&rInfo->pstsQlock) ;
    pst->next = rInfo->pstsQfirst ; 
    rInfo->pstsQfirst = pst ; 
    pthread_mutex_unlock(&rInfo->pstsQlock) ;
    pst = (rStreamInfoRec *)NULL ;
  }

  return pst ;

}


/** !!!!!!!!!!!!!!!!!!!!!! */

void delete_stream(rStreamInfoRec *pst)
{
 int n ;
 rTopicInfoRec *ptp ;
 char *packet ;
 FragMsgInfoRec *frg ;
 rmmReceiverRec *rInfo ;
 rumInstanceRec *gInfo ; 
 /*  char myName[] = {"delete_stream"} ; */

  rInfo = rInstances[pst->instance_id] ;
  ptp = rInfo->rTopics[pst->topic_id] ;

  stop_stream_service(pst) ; 

  rInfo->TotPacksIn -= pst->TotPacksIn ;
  rInfo->TotPacksOut-= pst->TotPacksOut;

  n = (int)(pst->rxw_lead + 1 - SQ_get_tailSN(pst->dataQ, 0)) ; 
  while ( n-- > 0 )
    if ( (packet=SQ_inc_tail(pst->dataQ,0)) != NULL )
      MM_put_buff(rInfo->dataBuffsQ,packet) ;
  SQ_free(pst->dataQ,0) ;

  n = (int)(pst->ng_last + 1 - SQ_get_tailSN(pst->nakSQ,0)) ; 
  while ( n-- > 0 )
    if ( (packet=SQ_inc_tail(pst->nakSQ,0)) != NULL )
      MM_put_buff(rInfo->nackStrucQ,packet) ;
  SQ_free(pst->nakSQ,0) ;

  while ( (frg = LL_get_buff(pst->fragQ,0)) != NULL )
  {
    free(frg->buffer) ;
    free(frg) ;
  }
  LL_free(pst->fragQ,0) ;

  gInfo = rInfo->gInfo ; 
  if ( pst->om_user != ptp->om_user ) 
    PutFcbEvent(gInfo, gInfo->free_callback, pst->om_user) ; 
  pthread_mutex_destroy(&pst->ppMutex) ;

  if ( pst->pck ) free(pst->pck) ;

  if ( ptp->on_data )
  {
    rumPacket *rPack ; 
    if (ptp->packQ)
    {
      LL_lock(ptp->packQ) ;
      n = LL_get_nBuffs(ptp->packQ,0) ;
      while ( n-- > 0 )
      {
        if ( (rPack=LL_get_buff(ptp->packQ,0)) != NULL )
        {
          if ( rPack->stream_info->stream_id == pst->sid )
            return_packet(rInfo, rPack);
          else
            LL_put_buff(ptp->packQ,rPack,0) ; 
        }
      }
      LL_signalF(ptp->packQ) ;
      LL_signalE(ptp->packQ) ;
      LL_unlock(ptp->packQ) ;
    }

    pst->pp_last_time = rInfo->CurrentTime + 60000 ; 
    pst->next = NULL ; 
    pthread_mutex_lock(&rInfo->deadQlock) ;
    if ( rInfo->deadQfirst )
      rInfo->deadQlast->next = pst ; 
    else
      rInfo->deadQfirst = pst ; 
    rInfo->deadQlast = pst ; 
    pthread_mutex_unlock(&rInfo->deadQlock) ;
  }
  else
  {
    pthread_mutex_lock(&rInfo->pstsQlock) ;
    pst->next = rInfo->pstsQfirst ; 
    rInfo->pstsQfirst = pst ; 
    pthread_mutex_unlock(&rInfo->pstsQlock) ;
  }

  return ;

}

/** !!!!!!!!!!!!!!!!!!!!!! */

void kill_stream(rStreamInfoRec *pst, char *reason)
{
 rmmReceiverRec *rInfo ; 
 rTopicInfoRec *ptp ;
 /*  char myName[] = {"kill_stream"} ; */


  rInfo = rInstances[pst->instance_id] ;
  ptp   = rInfo->rTopics[pst->topic_id] ;
  if ( reason && ptp->on_event )
  {
    void *EvPrms[1]; EvPrms[0] = reason;
    raise_stream_event(pst, RUM_STREAM_ERROR, EvPrms, 1) ;
  }
  pst->killed = 1 ;

  return ;
}

void destroy_stream(rStreamInfoRec *pst)
{
  rmmReceiverRec *rInfo = rInstances[pst->instance_id] ;
  rmmStreamID_t sid = pst->sid ; 
  rmm_rwlock_r2wlock(&rInfo->GlobalSync.rw) ;
  if ( (pst = find_stream(rInfo,sid)) != NULL )
  {
    reject_stream(rInfo,pst->sid,pst->cInfo->connection_id,4) ; 
    stop_stream_service(pst) ; 
    remove_stream(pst) ;
    delete_stream(pst) ;
  }
  rmm_rwlock_w2rlock(&rInfo->GlobalSync.rw) ;
}


/** !!!!!!!!!!!!!!!!!!!!!! */

void reject_stream(rmmReceiverRec *rInfo, rmmStreamID_t sid, rumConnectionID_t cid, int type)
{
  int i ;
  char sid_str[20] ; 
  TCHandle tcHandle = rInfo->tcHandle;

  pthread_mutex_lock(&rInfo->nbMutex) ;
  for ( i=0 ; i<rInfo->nbad ; i++ )
  {
    if ( rInfo->RejectedStreams[i].sid == sid )
    {
      rInfo->RejectedStreams[i].cid   = cid  ; 
      rInfo->RejectedStreams[i].type |= type ; 
      pthread_mutex_unlock(&rInfo->nbMutex) ;
      return ; 
    }
  }
      
  if ( rInfo->nbad >= MAX_STREAMS )
  {
    for ( i=1 ; i<MAX_STREAMS ; i++ )
      rInfo->RejectedStreams[i-1] = rInfo->RejectedStreams[i] ;  
    i = rInfo->nbad = MAX_STREAMS-1 ; 
  }

  rInfo->RejectedStreams[i].sid = sid ;
  rInfo->RejectedStreams[i].cid = cid  ; 
  rInfo->RejectedStreams[i].type= type ; 
  rInfo->nbad++ ; 
  pthread_mutex_unlock(&rInfo->nbMutex) ;

  b2h(sid_str,(uint8_t *)&sid,sizeof(sid)) ;
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5294),"%s",
    "The RUM receiver added the stream {0} to the rejected stream list.", sid_str );

}

/** !!!!!!!!!!!!!!!!!!!!!! */

int find_rejected_stream(rmmReceiverRec *rInfo, rmmStreamID_t sid)
{
  int i, rc ; 
 /*  char myName[] = {"find_rejected_stream"} ; */

  pthread_mutex_lock(&rInfo->nbMutex) ;
  for ( i=rInfo->nbad-1 ; i>=0 ; i-- )
  {
    if ( rInfo->RejectedStreams[i].sid == sid )
    {
      rc = rInfo->RejectedStreams[i].type ; 
      pthread_mutex_unlock(&rInfo->nbMutex) ;
      return rc ; 
    }
  }
  pthread_mutex_unlock(&rInfo->nbMutex) ;
  return 0 ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

void clear_rejected_streams(rmmReceiverRec *rInfo, rumConnectionID_t cid, int type)
{
  int i ; 
 /*  char myName[] = {"find_rejected_stream"} ; */

  pthread_mutex_lock(&rInfo->nbMutex) ;
  for ( i=rInfo->nbad-1 ; i>=0 ; i-- )
  {
    if ( (cid == 0 || rInfo->RejectedStreams[i].cid == cid) &&
         (rInfo->RejectedStreams[i].type&type) )
    {
      rInfo->RejectedStreams[i] = rInfo->RejectedStreams[--rInfo->nbad] ; 
    }
  }
  pthread_mutex_unlock(&rInfo->nbMutex) ;
  return ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

rTopicInfoRec *find_topic(rmmReceiverRec *rInfo, char *topicName, int topicLen)
{
 int i ;
 rTopicInfoRec *ptp ;
 /*  char myName[] = {"find_topic"} ; */

  for ( i=rInfo->rNumTopics-1 ; i>=0 ; i-- )
  {
    ptp = rInfo->rTopics[i] ;
    if ( ptp                != NULL &&
         ptp->accept_stream == NULL &&
         ptp->topicName     != NULL &&
         ptp->topicLen      == topicLen &&
         memcmp(ptp->topicName,topicName,topicLen) == 0 )
    {
      return ptp ;
    }
  }
  return (rTopicInfoRec *)NULL ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

rTopicInfoRec *find_streamset(rmmReceiverRec *rInfo, rumStreamParameters *psp) 
{
 int i ;
 rTopicInfoRec *ptp ;
 /*  char myName[] = {"find_streamset"} ; */

  for ( i=rInfo->rNumTopics-1 ; i>=0 ; i-- )
  {
    ptp = rInfo->rTopics[i] ;
    if ( ptp                != NULL &&
         ptp->accept_stream != NULL &&
         ptp->accept_stream(psp,ptp->ss_user) == RUM_STREAM_ACCEPT )
    {
      return ptp ;
    }
  }
  return (rTopicInfoRec *)NULL ;
}

/** !!!!!!!!!!!!!!!!!!!!!! */

int  add_scp(rmmReceiverRec *rInfo, scpInfoRec *scp)
{
  scpInfoRec *sInfo ; 

  if ( find_scp(rInfo,scp->sid,NULL) )
    return RMM_SUCCESS ; 

  if ( rInfo->nSCPs >= MAX_STREAMS )
  {
    return RMM_FAILURE ; 
  }

  if ( (sInfo=(scpInfoRec *)malloc(sizeof(scpInfoRec))) == NULL )
  {
    return RMM_FAILURE ; 
  }

  memcpy(sInfo,scp,sizeof(scpInfoRec)) ; 
  rInfo->SCPs[rInfo->nSCPs++] = sInfo ; 

  return RMM_SUCCESS ; 
}

int  del_scp(rmmReceiverRec *rInfo, rumConnectionID_t cid, rmmStreamID_t sid)
{
  int i , rc=RMM_FAILURE ;
  scpInfoRec *sInfo ; 

  for ( i=rInfo->nSCPs-1 ; i>=0 ; i-- )
  {
    if ( (sInfo=rInfo->SCPs[i]) == NULL )
    {
      rInfo->SCPs[i] = rInfo->SCPs[--rInfo->nSCPs] ; 
      continue ; 
    }
    if ( sInfo->cid == cid || sInfo->sid == sid )
    {
      free(rInfo->SCPs[i]) ; 
      rInfo->SCPs[i] = rInfo->SCPs[--rInfo->nSCPs] ; 
      rc = RMM_SUCCESS ; 
    }
  }
  return rc ; 

}
      
int find_scp(rmmReceiverRec *rInfo,  rmmStreamID_t sid, scpInfoRec *res)
{
  int i ; 
  scpInfoRec *sInfo ; 

  for ( i=rInfo->nSCPs-1 ; i>=0 ; i-- )
  {
    if ( (sInfo=rInfo->SCPs[i]) == NULL )
    {
      rInfo->SCPs[i] = rInfo->SCPs[--rInfo->nSCPs] ; 
      continue ; 
    }
    if ( sInfo->sid == sid )
    {
      if ( res )
      {
        memcpy(res,sInfo,sizeof(scpInfoRec)) ; 
      }
      return 1 ; 
    }
  }
  return 0 ; 

}

/** !!!!!!!!!!!!!!!!!!!!!! */


int buildNsendNAK_pgm(rStreamInfoRec *pst, int n, pgm_seq *sqn_list, int sorted)
{
  int i, *pi , psize, rc , iError;
  uint16_t i16 ; 
  char *buff, *packet , *bptr ; 
  char daddr[RUM_ADDR_STR_LEN] , dport[16] ; 
  pgmHeaderNAK *phn ;
  pgmOptHeader *pop ;
  rmmReceiverRec *rInfo = rInstances[pst->instance_id];
  rumInstanceRec *gInfo ; 
  TCHandle tcHandle = rInfo->tcHandle;
  char errMsg[ERR_MSG_SIZE];

  gInfo = rInfo->gInfo ; 

  if ( pst->path_nla.length == 0 )
    return RMM_FAILURE ;

  if ( !sorted )
    sortsn(n,sqn_list) ;

  if ( (buff = (char *)MM_get_buff(gInfo->nackBuffsQ,16,&iError)) == NULL )
    return RMM_FAILURE ;
  memset(buff,0,MAX_NAK_LENGTH) ;
  packet = buff + INT_SIZE ; 
  phn    = (pgmHeaderNAK *)packet ;

 /* printf("_dbg: %d %4x %8x \n",pst->src_port,pst->gsi_low,pst->gsi_high); */
  phn->src_port     = pst->dest_port ;
  phn->dest_port    = pst->src_port ;
  phn->type         = PGM_TYPE_NAK  ;
  phn->gsid_high    = pst->gsi_high ;
  phn->gsid_low     = pst->gsi_low  ;
  phn->tsdu_length  = 0 ;
  if ( pst->src_nla.length == nla_afi_len[NLA_AFI] )
    phn->src_nla_afi  = htons(NLA_AFI) ;
  else
    phn->src_nla_afi  = htons(NLA_AFI6) ;
  phn->reserved_1   = 0 ;
  memcpy(&phn->src_nla,pst->src_nla.bytes,pst->src_nla.length) ;
  bptr = (char *)&phn->src_nla + pst->src_nla.length ;
  if ( pst->cInfo->lcl_addr.length == nla_afi_len[NLA_AFI] )
    i16 = NLA_AFI ; 
  else
    i16 = NLA_AFI6; 
  NputShort(bptr,i16) ; 
  NputShort(bptr,0) ; 
  memcpy(bptr,pst->cInfo->lcl_addr.bytes,pst->cInfo->lcl_addr.length) ; 
  pst->nak_pac_opt_off = (int)(bptr-packet)+pst->cInfo->lcl_addr.length ; 

  phn->options      = 0  ;
  phn->checksum     = 0 ;
  phn->tsdu_length  = 0 ;
  phn->nak_sqn      = htonl(sqn_list[0]) ;
  psize = pst->nak_pac_opt_off ; 
  if ( n > 1 )
  {
    uint16_t len ; 
    len = 4*(n+1) ; /* (4+4+4*(n-1)) */
    phn->options   |= rInfo->PGM_OPT_PRESENT|rInfo->PGM_OPT_NET_SIG ;
    pop = (pgmOptHeader *)(packet + pst->nak_pac_opt_off) ;
    pop->option_type = PGM_OPT_LENGTH ;
    pop->option_len  = 4 ;
    pop->option_other= htons(len) ;
    psize += len ;
    len -= 4 ; 
    pop++ ;
    pop->option_type = PGM_OPT_END|PGM_OPT_NAK_LIST ;
    pop->option_len  = (uint8_t)len ;
    pop->option_other= htons(len) ;
    pop++ ;
    pi = (int *)pop ;
    pi-- ; /* so that pi[1] would point to the right place */
    for ( i=1 ; i<n ; i++ )
      pi[i] = htonl(sqn_list[i]) ;
  }

  bptr = buff ; 
  NputInt(bptr,psize) ; 
  BB_lock(pst->cInfo->sendNacksQ) ; 
  while ( BB_put_buff(pst->cInfo->sendNacksQ,buff,0) == NULL )
  {
    if ( rInfo->GlobalSync.goDN )
    {
      BB_unlock(pst->cInfo->sendNacksQ);
      MM_put_buff(gInfo->nackBuffsQ,buff) ;
      return RMM_FAILURE ;
    }
    BB_waitF(pst->cInfo->sendNacksQ);
  }
  BB_signalE(pst->cInfo->sendNacksQ);
  BB_unlock(pst->cInfo->sendNacksQ);

  if(llmIsTraceAllowed(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9294)))
  {
    if ( rmmGetNameInfo((SA *)&pst->sa_nk_uni,daddr,sizeof(daddr),dport,sizeof(dport),&rc,errMsg) == RMM_SUCCESS )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(9294),"%d%s%s%s",
        "Get Name Info: packets={0}, address={1}, port={2}.errMsg={3}", n,daddr,dport,errMsg );
    }
  }
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5297),"%d%u%u%s",
    "The RUM receiver sent a NAK for {0} packets (corresponding to packet sequence numbers {1} through {2}) for stream {3}.",n,sqn_list[0],sqn_list[n-1],pst->sid_str );


  pst->TotNacks++ ;
  pst->nak_stat[5] += n ;
  return RMM_SUCCESS ;

}

void SnapShot(rmmReceiverRec *rInfo)
{
#if MAKE_HIST
  int i;
#endif
  uint64_t n1,n2,n3,n4,n5,n6,n7,n8,tb;
  rStreamInfoRec *pst ;
  WhyBadRec *wb ;
  char timestr[32], *sptr;
  TCHandle tcHandle = rInfo->tcHandle;
  TBHandle tbh = llmCreateTraceBuffer(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5296));
  if(tbh == NULL) return;
  wb = &rInfo->why_bad ;
  rmmTime(NULL,timestr);
  for (sptr=timestr ; *sptr ; sptr++ ) if (*sptr == '\n') *sptr = ' ';
  llmAddTraceData(tbh,"%s","RUM Receiver Snapshot Report ({0}): \n",timestr);
  llmAddTraceData(tbh,"%d%d%d%d%d%d%d%d",
    "Topics: {0} , Conns: {1} , Streams: {2} , Rejected: {3} , PPthreads: {4} , MAthreads: {5} , ma_idle: {6} , pr_wfp: {7} \n",
    rInfo->rNumTopics,rInfo->gInfo->nConnections,rInfo->rNumStreams,rInfo->nbad,rInfo->NumPPthreads,rInfo->NumMAthreads,
               rInfo->ma_wfb,rInfo->pr_wfb) ;
  
  llmAddTraceData(tbh,"%d%d%d%d",
    "TaskTimer: nTasks: {0} , NominalSleep: {1} , ActualSleep: {2} , nUpdated: {3} \n",
   rInfo->rTasks.nTasks,rInfo->rTasks.reqSleepTime,rInfo->rTasks.actSleepTime,rInfo->rTasks.updSleepTime) ;
  

  n1 = rInfo->TotSelects ;
  n2 = rInfo->TotReads ; 
  n3 = rInfo->TotPacksRecv[0] ; 
  tb = rInfo->TotBytesRecv[0] ; 
  n4 = (ulong)(tb/0x40000000) ;
  n5 = (ulong)(tb%0x40000000) ;
  n6 =  n2 ? (ulong)(tb/n2) : 0 ; 
  n7 = rInfo->PacksRateIn  ;
  n8 = rInfo->BytesRateIn  ;

  llmAddTraceData(tbh,"%llu%llu%llu%llu%llu%llu%llu%llu%llu",
    "Throuput: nSelect: {0}, nReads: {1} {8}, nPacks: {2}, nBytes: {3}GB + {4}, ReadSize: {5}, Rate: {6} pps {7} kbps \n",
   UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4),UINT_64_TO_LLU(n5),
   UINT_64_TO_LLU(n6),UINT_64_TO_LLU(n7),UINT_64_TO_LLU(n8),UINT_64_TO_LLU(rInfo->TotCalls)) ;
  

 
  n1 = rInfo->TotPacksRecv[0] ;
  n2 = rInfo->TotPacksIn ;
  n3 = rInfo->TotPacksOut ;
  n1 = rInfo->TotSelects ;
  n2 = rInfo->TotReads ;
  n3 = rInfo->TotPacksOut ;
  n3 = rInfo->TotPacksRecv[0];
  n4 = rInfo->TotPacksIn-rInfo->TotPacksOut ;
  n5 = rInfo->TotPacksRecv[0] ? (ulong)(rInfo->TotBytesRecv[0]/rInfo->TotPacksRecv[0]) : 0 ;

   llmAddTraceData(tbh,"%llu%llu%llu%llu%llu",
     "Thruput: TotPacksIn: {0} {1} , TotPackOut: {2} , TotInQs: {3} , PackSize: {4} \n",
   UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4),UINT_64_TO_LLU(n5)) ;
  
 
  n2 = MM_get_nBuffs(rInfo->recvBuffsQ) ;
  n3 = LL_get_nBuffs(rInfo->sockQ,0) ;
  n1 = n2 + n3 ;
  n4 = MM_get_buf_size(rInfo->recvBuffsQ) ;
  n5 = MM_get_max_size(rInfo->recvBuffsQ) ;
  n6 = BB_get_nBuffs(rInfo->rmevQ,0) + BB_get_nBuffs(rInfo->lgevQ,0) ;

   llmAddTraceData(tbh,"%llu%llu%llu%llu%llu%llu",
     "RecvBuffs  : Allocated : {0} , Idle: {1} , InUse: {2} , BuffSize: {3} , MaxBuffs: {4} , evntQ: {5} \n",
   UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4),UINT_64_TO_LLU(n5),UINT_64_TO_LLU(n6)) ;
  
 
  
  n1 = MM_get_cur_size(rInfo->dataBuffsQ) ;
  n2 = MM_get_nBuffs(rInfo->dataBuffsQ) ;
  n3 = n1 - n2 ;
  n4 = MM_get_buf_size(rInfo->dataBuffsQ) ;
  n5 = MM_get_max_size(rInfo->dataBuffsQ) ;

  
   llmAddTraceData(tbh,"%llu%llu%llu%llu%llu",
     "DataBuffs  : Allocated : {0} , Idle: {1} , InUse: {2} , BuffSize: {3} , MaxBuffs: {4} \n",
   UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4),UINT_64_TO_LLU(n5)) ;

 
  n2 = MM_get_nBuffs(rInfo->nackStrucQ) ;
  n3 = 0 ;
  for ( pst=rInfo->firstStream ; pst ; pst=pst->next )
  {
    n3 += pst->nakQn ;
  }
  n1 = n2 + n3 ;
  n4 = MM_get_buf_size(rInfo->nackStrucQ) ;
  n5 = MM_get_max_size(rInfo->nackStrucQ) ;
  
   llmAddTraceData(tbh,"%llu%llu%llu%llu%llu",
     "NackElmnts : Allocated : {0} , Idle: {1} , InUse: {2} , BuffSize: {3} , MaxBuffs : {4} \n",
   UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4),UINT_64_TO_LLU(n5)) ;
  {
    n1 = rInfo->TotPacks[PGM_TYPE_SPM] ;
    n2 = rInfo->TotPacks[PGM_TYPE_ODATA] ;
    n3 = rInfo->TotPacks[PGM_TYPE_RDATA] ;
    n4 = rInfo->TotPacks[PGM_TYPE_NAK  ] ;
    n5 = rInfo->TotPacks[PGM_TYPE_NCF  ] ;
   
  llmAddTraceData(tbh,"%llu%llu%llu%llu%llu%llu",
     "PackCount: SPM: {0} , ODATA: {1} , RDATA: {2} , NAK: {3} , NCF: {4} , Tot: {5} \n",
   UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4),UINT_64_TO_LLU(n5),
   UINT_64_TO_LLU((n1+n2+n3+n4+n5))) ;
  
  }

 
 #if MAKE_HIST
  
  llmAddTraceData(tbh,"","\nReadSize Histogram:\n") ;
  for ( i=0 ; i<rInfo->hist_size ; i++ )
  {
    if ( rInfo->hist[i] )
   
  llmAddTraceData(tbh,"%d%d","{0} {1} \n",i,rInfo->hist[i]) ;
  
  }
  llmAddTraceData(tbh,"","\n") ;  

 #endif
  llmAddTraceData(tbh,"","\nDiagnostic Events: \n") ;
  get_why_bad_msgs(wb,tbh) ; 
  
  if ( rInfo->EmptyPackQ || rInfo->FullPackQ )
  {
    int i;
    rTopicInfoRec  *ptp;
    
  llmAddTraceData(tbh,"","\nRecvPacks Stats: \n") ;
    n1 = rInfo->FullPackQ ;
    n2 = rInfo->EmptyPackQ;
    for ( i=0,n3=0 ; i<rInfo->rNumTopics ; i++ )
    {
      if ( (ptp = rInfo->rTopics[i]) == NULL )
        continue ;
      if ( ptp->packQ == NULL )
        continue ;
      n3 += LL_get_nBuffs(ptp->packQ,0);
    }
  llmAddTraceData(tbh,"%llu%llu%llu",
    "AppPackQ: Full: {0} , Empty: {1} , InQ: {2} \n",
    UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3)) ;

    n1 = MM_get_cur_size(rInfo->packStrucQ) ;
    n2 = MM_get_nBuffs(rInfo->packStrucQ) ;
    n3 = n1 - n2 ;
    n4 = MM_get_buf_size(rInfo->packStrucQ) ;
    n5 = MM_get_max_size(rInfo->packStrucQ) ;
    llmAddTraceData(tbh,"%llu%llu%llu%llu%llu",
    "PackStrucs : Allocated : {0} , Idle: {1} , InUse: {2} , BuffSize: {3} , MaxBuffs: {4} \n",
   UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4),UINT_64_TO_LLU(n5)) ;
  }

  if ( rInfo->aConfig.PrintStreamInfo_rx )
  for ( pst=rInfo->firstStream ; pst ; pst=pst->next )
  {
    llmAddTraceData(tbh,"%s%s%s",
    "\nStreamID: {0} Conn: {1} Topic: {2} \n",pst->sid_str,pst->cInfo->req_addr,pst->topicName) ;

    n1 = pst->TotPacksIn ;
    n2 = pst->TotPacksOut ;
    n3 = pst->TotVisits  ;
    n4 = pst->TotMaSigs ;
    n5 = pst->TotPacksLost ;
    n6 = pst->ma_last_time ;
   
  llmAddTraceData(tbh,"%llu%llu%llu%llu%llu%llu",
    "Thruput: TotPacksIn/Out: {0}/{1} , TotMaVisits/Signals: {2}/{3} , TotPacksLost: {4} , LastTime: {5} \n",
   UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4),
   UINT_64_TO_LLU(n5),UINT_64_TO_LLU(n6)) ;

    n1 = (pst->TotMsgsOut) ;
    n2 = (pst->TotBytesOut/0x40000000) ;
    n3 = (pst->TotBytesOut%0x40000000) ;
   
  llmAddTraceData(tbh,"%llu%llu%llu",
    "Thruput: TotMsgsRecvd: {0} , TotBytesRecvd: {1}GB + {2} \n",
    UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3)) ;

    n1 = pst->TotNacks ;
    if ( n1 )
      n2 = pst->nak_stat[5]/pst->TotNacks ;
    else
      n2 = 0 ;
    n3 = pst->RdataPacks ;
    n4 = pst->DupPacks ;
    n5 = pst->nak_stat[6] ;
    n6 = pst->ng_last_time ;
   
  llmAddTraceData(tbh,"%llu%llu%llu%llu%llu%llu",
    "Thruput: NAKs: {0} , ChunkSize: {1} , RdataPacks: {2}, DupPacks: {3}, ReTrans: {4} , LastTime: {5} \n",
     UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4),
     UINT_64_TO_LLU(n5),UINT_64_TO_LLU(n6)) ;

    n1 = pst->rxw_trail ;
    n2 = pst->rxw_lead ;
    n3 = pst->rxw_lead-pst->rxw_trail ;
    n4 = pst->last_spm ;
    llmAddTraceData(tbh,"%llu%llu%llu%llu",
    "tWindow : Trail: {0} , Lead: {1} , Size: {2}, SPM: {3} \n",
     UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4));

    n1 = SQ_get_tailSN(pst->dataQ,0) ;
    n2 = SQ_get_headSN(pst->dataQ,0) ;
    n3 = SQ_get_tailSN(pst->nakSQ,0) ;
    llmAddTraceData(tbh,"%llu%llu%llu%llu",
    "rWindow : Tail : {0} , Head: {1} , Size: {2} , nackTail: {3} \n",
    UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU((n2-n1)),UINT_64_TO_LLU(n3)) ;

    n1 = SQ_get_nBuffs(pst->dataQ) ;
    n2 = pst->nakQn ;
    n3 = LL_get_nBuffs(pst->fragQ,0) ;
  
  llmAddTraceData(tbh,"%llu%llu%llu","Queues  : Data : {0} , NAKs: {1} , Frag: {2} \n",
    UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3)) ;

    n1 = pst->nak_stat[0] ;
    n2 = pst->nak_stat[1] ;
    n3 = pst->nak_stat[2] ;
    n4 = pst->NackTimeoutNCF ;
    n5 = pst->NackTimeoutData;
    
  llmAddTraceData(tbh,"%llu%llu%llu%llu%llu","Nacks   : Created: {0} , dataOK: {1} , Trail: {2} , TO_ncf: {3} , TO_data: {4} \n",
   UINT_64_TO_LLU(n1),UINT_64_TO_LLU(n2),UINT_64_TO_LLU(n3),UINT_64_TO_LLU(n4),UINT_64_TO_LLU(n5)) ;

  llmAddTraceData(tbh,"","\n");
  }
  
  llmAddTraceData(tbh,"","\nRUM Receiver SnapShot Report End\n");
  llmCompositeTraceInvoke(tbh);
}



/** !!!!!!!!!!!!!!!!!!!!!! */
/** !!!!!!!!!!!!!!!!!!!!!! */

static void wakeMA(rmmReceiverRec *rInfo, rStreamInfoRec *pst)
{
  if ( pst )
    pst->TotMaSigs++ ;
  put_pst_inQ(rInfo,pst) ;
}
void put_pst_inQ(rmmReceiverRec *rInfo,rStreamInfoRec *pst)
{
  LL_lock(rInfo->mastQ) ;
  if ( pst && !pst->inMaQ )
  {
    pst->inMaQ = 1 ;
    LL_put_buff(rInfo->mastQ,pst,0) ; 
  }
  LL_signalE(rInfo->mastQ) ;
  LL_unlock(rInfo->mastQ) ;
}
/** !!!!!!!!!!!!!!!!!!!!!! */


/**-------------------------------------**/


