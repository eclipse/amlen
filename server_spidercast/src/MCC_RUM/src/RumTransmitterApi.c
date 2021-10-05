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

#include "rmmSystem.h"
#include "rumCapi.h"
#include "rmmWinPthreads.h"
#include "rmmConstants.h"

#include "rmmCutils.h"
#include "SockMgm.h"
#include "BuffBoxMgm.h"
#include "MemMan.h"

#include "rmmTprivate.h"


/* Global Transmitter Variables */
static int                  rmmTrunning   = 0 ;
static int                  tNumInstances = 0 ;
static rmmTransmitterRec   *rmmTRec[MAX_Tx_INSTANCES] ;

static rmm_mutex_t      rmmTmutex = PTHREAD_MUTEX_INITIALIZER ;

/*  Internal Routines  */
/*--------------------*/
static int  rumT_Init(rumInstanceRec *rumInfo,int *error_code) ;
static int  rumT_ChangeLogLevel(rumInstanceRec *rumInfo, int new_log_level, int *error_code) ;
static int  rumT_CreateQueue(rumInstanceRec *rumInfo, const char *queue_name, int reliability_level, rumConnection *rum_connection, rum_on_event_t on_event, void *event_user,
                 int is_late_join, rumQueueT *queue_t, rumStreamID_t *stream_id, int app_con_batch, int *error_code) ;
static int  rumT_CloseQueue(rumInstanceRec *rumInfo, rumQueueT *queue_t, int timeout_milli, int *error_code) ;
static int  rumT_SubmitMessage(rumInstanceRec *rumInfo, int q_handle, const char *msg,  int msg_len, const rumTxMessage *msg_params, int *error_code) ;
static int  rumT_Stop(rumInstanceRec *rumInfo, int timeout_milli, int *error_code) ;
static int  create_queue_T(int inst, StreamInfoRec_T *pSinf, ConnInfoRec *cInf, uint16_t counter, const char *queue_name, int reliable, rumConnection *rum_connection,
                          rum_on_event_t on_event, void *event_user, int is_late_join, pgm_seq lj_mark, int app_con_batch, int *error_code) ;
static int  rumT_GetStreamID(const rumQueueT *queue_t, rumStreamID_t *stream_id, int *error_code) ;

static int rum_transmitter_init(int inst, int *error_code) ;
static void free_transmitter_memory(int inst) ;
static int  start_transmitter_threads(int inst) ;
static int  set_timer_tasks(int inst);
static int  stop_transmitter_threads(int inst) ;
static int  rumT_Create_Queue(rumInstanceRec *rumInfo, const char *queue_name, int reliability_level, rumConnection *rum_connection,
                              rum_on_event_t on_event, void *event_user,rumQueueT *queue_t, rumStreamID_t *stream_id,
                              int is_late_join, pgm_seq lj_mark, int app_con_batch, int *error_code) ;
static int  construct_odata_options(StreamInfoRec_T *pSinf) ;
static void free_stream_memory(StreamInfoRec_T *pSinf) ;
static int  BuildSpmPacket(StreamInfoRec_T *pSinf, int close_linger) ;
static int  checkT_configuration_parameters(int inst) ;
static int detach_trans_thread(int inst, const char *name, pthread_t id);


/*--------------------*/


/*  Threads and utility functions Implementation  */
#include "rmmCutils.c"
#include "SockMgm.c"
#include "rmmTutils.c"
#include "TokenBucket.c"
#include "BuffBoxMgm.c"
#include "MemMan.c"
#include "RepairThread.c"
#include "rumFireOutThread.c"
#include "MonitorThread.c"


/*****************************************************************************************/
static int rumT_chk_instance(int inst, TCHandle *tcHandle, int *ec)
{
  if ( inst < 0 || inst >= MAX_Tx_INSTANCES )
  {
    if ( ec ) *ec = RUM_L_E_INSTANCE_INVALID ;

    return -1;
  }


  rmmTmutex_lock() ;

  if ( rmmTRec[inst] == NULL || rmmTrunning == 0 || rmmTRec[inst]->initialized == RMM_FALSE )
  {
    if ( ec ) *ec = RUM_L_E_INSTANCE_INVALID ;
    rmmTmutex_unlock() ;
    return -1 ;
  }
  *tcHandle = rmmTRec[inst]->tcHandle;
  return inst ;
}

static int rumT_chk_topic(int inst, int counter, const char* methodName,int *ec)
{
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;
  if ( counter < 0 || counter >= rmmTRec[inst]->T_advance.MaxStreamsPerTransmitter ||
    rmmTRec[inst]->all_streams[counter] == NULL )
  {
    if ( ec ) *ec = RUM_L_E_QUEUE_INVALID ;

    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3489), "%d%d%s", "The RUM transmitter instance {1} did not find the RUM transmitter queue handle {0} when function {2} was called.", 
    counter, inst, methodName);

    rmmTmutex_unlock() ;
    return -1 ;
  }
  return counter ;
}

/*****************************************************************************************/
/*****************************************************************************************/
/*   API: Transmitter                                                                    */
/*****************************************************************************************/


/***********************   1 1 1 1   *****************************************************/
int rumT_Init(rumInstanceRec *rumInfo,int *error_code)
{
  int inst, rc;
  TCHandle tcHandle = rumInfo->tcHandles[2];
  rumInfo->instance_t = -1;
  rumInfo->tInfo = NULL;


  rmmTmutex_lock() ;


  if ( rmmTrunning == 0 )
  {
    for (inst=0; inst<MAX_Tx_INSTANCES; inst++)     rmmTRec[inst] = NULL;
    rmmInitTime() ; /* init the internal static BaseTimeSecs under mutex */
    rmmTrunning = 1;
  }


  inst = rumInfo->instance ;
  if ( inst < 0 || inst >= MAX_Tx_INSTANCES || rmmTRec[inst] != NULL )
  {
    rmm_fprintf(stderr," RumApi(rmmTInit): Initialization error - too many Transmitter instances running!!\n") ;
    rumInfo->instance_t = -1;
    rmmTmutex_unlock() ;
    *error_code = RUM_L_E_TOO_MANY_INSTANCES ;
    return RMM_FAILURE ;
  }
  if ( (rmmTRec[inst] = (rmmTransmitterRec *)malloc(sizeof(rmmTransmitterRec))) == NULL  )
  {
    rmm_fprintf(stderr," RumApi(rmmTInit): Failed to allocate rmmTransmitterRec\n") ;
    rumInfo->instance_t = -1;
    rmmTmutex_unlock() ;
    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
    return RMM_FAILURE ;
  }
  memset(rmmTRec[inst], 0, sizeof(rmmTransmitterRec));

  /* let the transmitter know the RUM instance */
  rmmTRec[inst]->rumInfo = rumInfo;
  rmmTRec[inst]->tcHandle = tcHandle;
  rc = rum_transmitter_init(inst, error_code);

  if ( rc == RMM_FAILURE )
  {
    free(rmmTRec[inst]);
    rmmTRec[inst] = NULL;
  }
  else
  {
    rumInfo->instance_t = inst;
    rumInfo->tInfo = rmmTRec[inst];
    rumInfo->txRunning = 1;
    if ( ++tNumInstances >= MAX_Tx_INSTANCES ) tNumInstances = MAX_Tx_INSTANCES-1;
  }

  rmmTmutex_unlock() ;
  return rc;
}

/***********************   2 2 2 2   *****************************************************/

int  rumT_ChangeLogLevel(rumInstanceRec *rumInfo, int log_level, int *error_code)
{
  int inst ;
  const char* methodName = "rumT_ChangeLogLevel";
  TCHandle tcHandle;
  if ( (inst = rumT_chk_instance(rumInfo->instance_t, &tcHandle, error_code)) < 0 )
    return RMM_FAILURE ;
  LOG_METHOD_ENTRY();

  if ( log_level >= RMM_LOGLEV_NONE && log_level <= RMM_LOGLEV_XTRACE )
    rmmTRec[inst]->T_config.LogLevel = log_level;
  else
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3309), "%d",
        "The specified log level ({0}) is not valid. ", log_level);

    rmmTmutex_unlock() ;
    return RMM_FAILURE ;
  }

  rmmTmutex_unlock() ;

  LOG_METHOD_ENTRY();

  return RMM_SUCCESS ;

}


/***********************   3 3 3 3   *****************************************************/
int  rumT_CreateQueue(rumInstanceRec *rumInfo, const char *queue_name, int reliability_level, rumConnection *rum_connection,
                      rum_on_event_t on_event, void *event_user, int is_late_join,
                      rumQueueT *queue_t, rumStreamID_t *stream_id, int app_con_batch, int *error_code)
{
  return rumT_Create_Queue(rumInfo, queue_name, reliability_level, rum_connection, on_event, event_user, queue_t, stream_id,
                           is_late_join, 1, app_con_batch, error_code) ;
}

/***********************   4 4 4 4   *****************************************************/
int  rumT_CloseQueue(rumInstanceRec *rumInfo, rumQueueT *queue_t, int timeout_milli, int *error_code)
{
  StreamInfoRec_T *pSinf ;
  int inst, linger ;
  int counter ;
  const char* methodName = "rumT_CloseQueue";
  TCHandle tcHandle;
  if ( (inst = rumT_chk_instance(rumInfo->instance_t, &tcHandle, error_code)) < 0 )
    return RMM_FAILURE ;
  LOG_METHOD_ENTRY();
  if ( (counter = rumT_chk_topic(inst, queue_t->handle, methodName, error_code)) < 0 )
    return RMM_FAILURE ;

  pSinf = rmmTRec[inst]->all_streams[counter] ;

  if ( pSinf->active == RMM_TRUE )
    pSinf->active = RMM_FALSE ;
  else
  {
    *error_code = RUM_L_E_QUEUE_ERROR ;
    rmmTmutex_unlock() ;
    return RMM_FAILURE ;
  }

  linger = rmmMax(50, timeout_milli) ;
  /* announce that queue is about to close by including opt_fin in spm */
  pthread_mutex_lock(&pSinf->spm_mutex);
  pSinf->spm_pending = RMM_FALSE;
  BuildSpmPacket(pSinf, linger);  /* construct a new spm packet with opt_fin included */
  pSinf->spm_pending = RMM_TRUE;
  pthread_mutex_unlock(&pSinf->spm_mutex);
  rmm_signal_fireout(inst,1,pSinf);

  pSinf->close_time = rmmTRec[inst]->rumInfo->CurrentTime + linger;
  pSinf->closed = RMM_FALSE;    /* queue will be closed after heartbeat timeout */

  rmmTmutex_unlock() ;

  LOG_METHOD_EXIT();

  return RMM_SUCCESS;
}

/***********************   5 5 5 5   *****************************************************/
int  rumT_SubmitMessage(rumInstanceRec *rumInfo, int q_handle, const char *msg_buf,  int msg_length, const rumTxMessage *msg_params, int *ec)
{
  int msg_len ;
  StreamInfoRec_T *pSinf ;
  int inst = rumInfo->instance_t;
  TCHandle tcHandle = NULL;

  if ( rmmTRec[inst] == NULL || q_handle < 0 || q_handle >= rmmTRec[inst]->T_advance.MaxStreamsPerTransmitter ||
    (pSinf = rmmTRec[inst]->all_streams[q_handle]) == NULL )
  {
    *ec = RUM_L_E_QUEUE_INVALID ;

    return RMM_FAILURE ;
  }
  tcHandle = rmmTRec[inst]->tcHandle;

  if ( pSinf->active == RMM_FALSE || pSinf->conn_invalid == RMM_TRUE )
  {
    if ( pSinf->conn_invalid == RMM_TRUE )
    {
      *ec = RUM_L_E_CONN_INVALID ;
      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3303), "%s",
          "The connection for queue {0} is closed. A connection must be established and "
          "valid for message submission", pSinf->topic_name);

    }
    else
    {
      *ec = RUM_L_E_QUEUE_CLOSED ;

      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3304), "%s",
          "The queue {0} is closed and unable to submit a message. ", pSinf->topic_name);

    }

    return RMM_FAILURE ;
  }

 /* max_debug(inst,RMM_D_METHOD_ENTRY,0,NULL,"rmmTSubmitMessage (API)"); */

  msg_len = 0 ;
  if ( msg_params )
  {
    if ( msg_params->num_frags <= 0 )
    {
      if ( msg_params->msg_buf )
      {
        msg_len = msg_params->msg_len ;
        msg_buf = msg_params->msg_buf ;
      }
    }
    else
    {
      int i ;
      for ( i=0, msg_len=0 ; i<msg_params->num_frags ; i++ )
      {
        if ( msg_params->msg_frags[i] == NULL || msg_params->frags_lens[i] <= 0)
        {
          msg_len = 0 ;
          break ;
        }
        msg_len += msg_params->frags_lens[i] ;
      }
      msg_buf = NULL ;
    }
  }
  else
  {
    if ( msg_buf )
      msg_len = msg_length ;
  }

  if ( msg_len <= 0 )
  {
    *ec = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3305), "%d%d%s", "The RUM instance {1} is unable to submit a message on queue {2}. The message has either no content or has a length ({0}) which is less than or equal to 0. ", msg_len, inst, pSinf->topic_name);

    return RMM_FAILURE ;
  }

  return rmmT_submit_message(pSinf, msg_buf, msg_len, 0, NULL, msg_params, ec) ;
}


/***********************   6 6 6 6   *****************************************************/
int rumT_Stop(rumInstanceRec *rumInfo, int timeout_milli, int *error_code)
{
  int i, linger ;
  StreamInfoRec_T  *copy_of_all_streams[RMM_MAX_TX_TOPICS] ;
  StreamInfoRec_T *pSinf ;
  int inst ;
  const char* methodName = "rumT_Stop";
  TCHandle tcHandle;
  if ( (inst = rumT_chk_instance(rumInfo->instance_t, &tcHandle, error_code)) < 0 )
    return RMM_FAILURE ;
  LOG_METHOD_ENTRY();


  if ( rmmTRec[inst]->initialized == RMM_TRUE )
    rmmTRec[inst]->initialized = RMM_FALSE;
  else
  {
    *error_code = RUM_L_E_INSTANCE_INVALID   ;
    rmmTmutex_unlock() ;
    return RMM_FAILURE;
  }

  linger = rmmMax(1, timeout_milli) ;
  /* prevents submission of new messages on all streams */
  for (i=0; i < rmmTRec[inst]->max_stream_index ; i++)
  {
    if ( (pSinf = rmmTRec[inst]->all_streams[i]) != NULL ) pSinf->active = RMM_FALSE;
  }

  for (i=0; i < rmmTRec[inst]->max_stream_index ; i++)
  {
    if ( (pSinf = rmmTRec[inst]->all_streams[i]) == NULL ) continue;

    /* announce that queue is about to close by including opt_fin in spm */
    pthread_mutex_lock(&pSinf->spm_mutex);
    pSinf->spm_pending = RMM_FALSE;
    BuildSpmPacket(pSinf, linger);           /* construct a new spm packet with opt_fin included */
    pSinf->spm_pending = RMM_TRUE;
    pthread_mutex_unlock(&pSinf->spm_mutex);
    rmm_signal_fireout(inst,1,pSinf);
  }

  rmmTmutex_unlock();     /* release global mutex during sleep */

  tsleep(linger/1000, 1000000*(linger % 1000));

  rmmTmutex_lock() ;

  if ( stop_transmitter_threads(inst) != RMM_SUCCESS )
  {
    *error_code = RUM_L_E_THREAD_ERROR ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3311), "%d", "The transmitter threads for RUM instance {0} were unable to stop.", inst);

  }

  Sleep(5*BASE_SLEEP);

  for (i=0; i<RMM_MAX_TX_TOPICS; i++)
  {
    copy_of_all_streams[i] = rmmTRec[inst]->all_streams[i];
    rmmTRec[inst]->all_streams[i] = NULL;
  }

  Sleep(5*BASE_SLEEP);

  /* free memory */
  for (i=0; i < RMM_MAX_TX_TOPICS ; i++)
  {
    if ( (pSinf=copy_of_all_streams[i]) != NULL )
    {
      free_stream_memory(pSinf);
      free(pSinf);
    }
    if ( (pSinf=rmmTRec[inst]->closed_streams[i]) != NULL )
    {
      free_stream_memory(pSinf);
      free(pSinf);
    }
  }

  /* report method exit before closing log file */

  free_transmitter_memory(inst) ;
  rumInfo->txRunning = 0;
  rumInfo->instance_t = -1;
  free(rmmTRec[inst]);
  rmmTRec[inst] = NULL;

  rmmTmutex_unlock() ;

  LOG_METHOD_EXIT();

  return RMM_SUCCESS;
}


/***********************   --  *****************************************************/

/***********************   7 7 7 7   *****************************************************/
int  rumT_GetStreamID(const rumQueueT *queue_t, rumStreamID_t *stream_id, int *error_code)
{
  StreamInfoRec_T *pSinf ;
  int inst = queue_t->instance;
  const char* methodName = "rumT_GetStreamID";
  TCHandle tcHandle = NULL;

  if ( inst < 0 || inst >= MAX_Tx_INSTANCES )
  {
    *error_code = RUM_L_E_INSTANCE_INVALID ;
    return RMM_FAILURE;
  }

  tcHandle = rmmTRec[inst]->tcHandle;
  LOG_METHOD_ENTRY();

  memset(stream_id, 0, sizeof(rumStreamID_t));
  if ( rmmTRec[inst] == NULL ||   rmmTRec[inst]->initialized == RMM_FALSE || queue_t->handle < 0 ||
    queue_t->handle >= rmmTRec[inst]->T_advance.MaxStreamsPerTransmitter )
  {
    *error_code = RUM_L_E_QUEUE_INVALID ;
    return RMM_FAILURE;
  }

  if ( (pSinf = rmmTRec[inst]->all_streams[queue_t->handle]) == NULL )
  {
    *error_code = RUM_L_E_QUEUE_INVALID ;
    return RMM_FAILURE;
  }

  memcpy(stream_id, &pSinf->stream_id, sizeof(rumStreamID_t));

  LOG_METHOD_EXIT();

  return RMM_SUCCESS;
}

/* -------  not supported --------------- */


/***********************   1 1 1 1   *****************************************************/
int rum_transmitter_init(int inst, int *error_code)
{
  rumInstanceRec *rumInfo;
  int32_t DataPoolSize, ControlPoolSize, data_buf_size;
  int i, rc, err_code;
  uint16_t port;
  uint32_t rate;
  char caddr[RUM_ADDR_STR_LEN], caddr6[RUM_ADDR_STR_LEN] ;
  char my_host_name[512];
  char error_msg[ERR_MSG_SIZE];
  const char* methodName = "rum_transmitter_init";
  TCHandle tcHandle = NULL;
  TBHandle tbHandle = NULL; 
  if ( inst < 0 || inst >= MAX_Tx_INSTANCES )
  {
    *error_code = RUM_L_E_INSTANCE_INVALID ;
    return RMM_FAILURE;
  }
  tcHandle = rmmTRec[inst]->tcHandle;

  rc = RMM_SUCCESS;

  rumInfo = rmmTRec[inst]->rumInfo;

  /* put the on_log and on_denug functions */

  rmmTRec[inst]->fp_log = rumInfo->fp_log;
  rmmTRec[inst]->log_user = rumInfo->log_user;

  rmmTRec[inst]->initialized = RMM_FALSE;
  rmmTRec[inst]->DataBuffPool = NULL ;
  rmmTRec[inst]->CtrlBuffPool = NULL ;
  rmmTRec[inst]->global_token_bucket = NULL ;

  rmmTRec[inst]->number_of_streams = 0;
  rmmTRec[inst]->max_stream_index = 0 ;
  rmmTRec[inst]->counter = 0 ;

  /* select a time based starting point for streamID */
  {
    time_t sec;
    rmmTRec[inst]->streamID_counter = (uint16_t)rmmTime(&sec,NULL) ; 
    rmmTRec[inst]->streamID_counter+= (uint16_t)sec ; 
  }

  for (i=0; i< RMM_MAX_TX_TOPICS; i++)
  {
    rmmTRec[inst]->all_streams[i] = NULL ;
    rmmTRec[inst]->closed_streams[i] = NULL ;
  }

  rmmTRec[inst]->T_config  = rumInfo->basicConfig;
  rmmTRec[inst]->T_advance = rumInfo->advanceConfig;

  if ( checkT_configuration_parameters(inst) == RMM_FAILURE )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2315), "",
        "RUM transmitter initialization failed due to configuration errors. ");

    *error_code = RUM_L_E_CONFIG_ENTRY ;
    rc = RMM_FAILURE ;
  }
  /* indicates if instance maintaines History and can answer NAKs */
  if ( rmmTRec[inst]->T_config.MinimalHistoryMBytes > 0 )
    rmmTRec[inst]->keepHistory = 1;
  else
    rmmTRec[inst]->keepHistory = 0;

  /* find interfaces and IP adresses used in the host */
    tbHandle = llmCreateTraceBuffer(tcHandle,LLM_LOGLEV_XTRACE,MSG_KEY(1386));
  if ( rmmGetLocalIf(rmmTRec[inst]->T_config.TxNetworkInterface, (rmmTRec[inst]->T_config.Protocol==RUM_IB), 0, 0, rmmTRec[inst]->T_config.IPVersion,
    &rmmTRec[inst]->localIf[0], &err_code, error_msg,tbHandle) != RMM_SUCCESS )
  {
    if(tbHandle != NULL) llmCompositeTraceInvoke(tbHandle);
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2316), "%s",
        "The RUM transmitter is unable to acquire a local network interface. ({0}).", error_msg);
    *error_code = RUM_L_E_LOCAL_INTERFACE ;
    rc = RMM_FAILURE ;
  }else{
    if(tbHandle != NULL) llmCompositeTraceInvoke(tbHandle);
  }
  /* set the instance primary IPv4 and IPv6 addresses; also sets gsi_high */
  rmmTRec[inst]->num_interfaces = 1 ; 
  if ( set_source_nla(inst) != RMM_SUCCESS )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2317), "", "The RUM transmitter is unable to set a local IP address.");

    *error_code = RUM_L_E_LOCAL_INTERFACE ;
    rc = RMM_FAILURE ;
  }

  /* set gsi_high and get "random" port */
  rumInfo->gsi_high = rmmTRec[inst]->gsi_high;
  rumInfo->TxlocalIf = rmmTRec[inst]->localIf[0];
  rmmTRec[inst]->port = rumInfo->port;

  /* print host name and address information */
  port = ntohs(rumInfo->port);
  gethostname(my_host_name, 500);
  if ( rmmTRec[inst]->source_nla[0].length > 0 )
    rmm_ntop(AF_INET,&rmmTRec[inst]->source_nla[0].bytes,caddr,sizeof(caddr)) ;
  else
    strlcpy(caddr,"None",sizeof(caddr)) ;
  if ( rmmTRec[inst]->source_nla[1].length > 0 )
    rmm_ntop(AF_INET6,&rmmTRec[inst]->source_nla[1].bytes,caddr6,sizeof(caddr6)) ;
  else
    strlcpy(caddr6,"None",sizeof(caddr6)) ;

  llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_STATUS, MSG_KEY(1318), "%d%s%s%d%s%d",
      "Transmitter (instance {0} Network Details:\n IPv4 Address = {1}\n IPv6 address = {2}\n Port Number = {3}\n Hostname = {4}\n streamID_counter = {5}",
      inst, caddr, caddr6, port, my_host_name, rmmTRec[inst]->streamID_counter);


  rmmTRec[inst]->min_batching_time = (double)rmmTRec[inst]->T_advance.MinBatchingMicro/1e6 ;
  rmmTRec[inst]->max_batching_time = (double)rmmTRec[inst]->T_advance.MaxBatchingMicro/1e6 ;
  if ( rmmTRec[inst]->min_batching_time > 0e0 )
    rmmTRec[inst]->max_batching_packets = 2 * (int)(1e0/rmmTRec[inst]->min_batching_time) ;
  else
    rmmTRec[inst]->max_batching_packets = 10000 ;

  rmmTRec[inst]->extTime = sysTOD ;

  /* initialize threads status */
  rmmTRec[inst]->FireOutStatus.status = THREAD_INIT ;
  rmmTRec[inst]->RepairStatus.status  = THREAD_INIT ;
  rmmTRec[inst]->SpmStatus.status     = THREAD_INIT ;
  rmmTRec[inst]->MonitorStatus.status = THREAD_INIT ;
  rmmTRec[inst]->FireOutStatus.thread_id = 0 ;
  rmmTRec[inst]->RepairStatus.thread_id  = 0 ;
  rmmTRec[inst]->SpmStatus.thread_id     = 0 ;
  rmmTRec[inst]->MonitorStatus.thread_id = 0 ;
  rmmTRec[inst]->FireOutStatus.loops = 0 ;
  rmmTRec[inst]->RepairStatus.loops  = 0 ;
  rmmTRec[inst]->SpmStatus.loops     = 0 ;
  rmmTRec[inst]->MonitorStatus.loops = 0 ;


  rmmTRec[inst]->DataPacketSize          = 8*((rmmTRec[inst]->T_config.PacketSizeBytes+INT_SIZE+7) / 8) ;
  rmmTRec[inst]->packet_header_offset    = rmmTRec[inst]->DataPacketSize ;
  rmmTRec[inst]->MaxPacketsAllowed       = (uint32_t)((rmm_uint64)1024*1024*rmmTRec[inst]->T_config.TxMaxMemoryAllowedMBytes / rmmTRec[inst]->DataPacketSize)  ; 
  rmmTRec[inst]->MinHistoryPackets       = (uint32_t)((rmm_uint64)1024*1024*rmmTRec[inst]->T_config.MinimalHistoryMBytes / rmmTRec[inst]->DataPacketSize) ;
  rmmTRec[inst]->MaxPendingPackets       = (uint32_t)((rmm_uint64)1024*rmmTRec[inst]->T_advance.MaxPendingQueueKBytes / rmmTRec[inst]->DataPacketSize) ;
  rmmTRec[inst]->MaxPendingStreamPackets = rmmTRec[inst]->MaxPendingPackets;

  data_buf_size = rmmTRec[inst]->DataPacketSize+sizeof(rmmTxPacketHeader)+16 ; /* header + 4 bytes length + small extra */
  DataPoolSize  = rmmTRec[inst]->MaxPacketsAllowed ;
  if ( (rmmTRec[inst]->DataBuffPool = MM_alloc2(data_buf_size, (DataPoolSize+100), 0, 0, 0, rmmTRec[inst]->packet_header_offset, (int)sizeof(rmmTxPacketHeader)) ) == NULL )
  {
    LOG_MEM_ERR(tcHandle, methodName, (sizeof(MemManRec) + (sizeof(void *)*(DataPoolSize+100))) );
    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
    rc = RMM_FAILURE ;
  }

  /* no memory limit imposed on control packets */
  ControlPoolSize = rmmMax(MIN_QUEUE_SIZE, rmmTRec[inst]->MaxPacketsAllowed/10) ;
  i = CONTROL_PACKET_BUF_SIZE-2*sizeof(void *) ;
  if ( (rmmTRec[inst]->CtrlBuffPool = MM_alloc2(CONTROL_PACKET_BUF_SIZE, (ControlPoolSize+100), 0,0,0, i, (int)sizeof(void *)) ) == NULL )
  {
  LOG_MEM_ERR(tcHandle, methodName, (sizeof(MemManRec) + (sizeof(void *)*(ControlPoolSize+100))) );


    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
    rc = RMM_FAILURE ;
  }

  for (i=0; i < FIREOUT_ARRAY_SIZE ; i++) rmmTRec[inst]->FO_Thrds[i] = THREAD_INIT ;

  /* initialize FO_CondWait */
  if ( rmm_cond_init(&rmmTRec[inst]->FO_CondWait, rmmTRec[inst]->T_advance.FireoutPollMicro) == RMM_FAILURE ||
       rmm_cond_init(&rmmTRec[inst]->FO_CondSleep,rmmTRec[inst]->T_advance.FireoutPollMicro) == RMM_FAILURE  )
  {

  llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2318), "", "The FireOut thread condition wait variable failed to initialize.");

    *error_code = RUM_L_E_THREAD_ERROR ;
    rc = RMM_FAILURE ;
  }

  /* transmitter rate limit */
  if ( rmmTRec[inst]->T_config.LimitTransRate != RMM_DISABLED_RATE_LIMIT )
  {
    rate = 128*rmmTRec[inst]->T_config.TransRateLimitKbps;
    /* init_bucket(uint32_t trans_rate, int min_sleep_time) */
    if ( (rmmTRec[inst]->global_token_bucket = init_bucket(rate, 9)) == NULL )
    {

      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2319), "", "The rate control feature failed to initialize.");

      *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
      rc = RMM_FAILURE ;
    }
    else
      rmmTRec[inst]->bucket_rate = rmmTRec[inst]->T_config.TransRateLimitKbps;
  }

  rmmTRec[inst]->MemoryAlert = MEMORY_ALERT_OFF ;
  rmmTRec[inst]->partial_pending = RMM_FALSE;
  rmmTRec[inst]->packet_rate = 0;

  if ( (pthread_mutex_init(&rmmTRec[inst]->MemoryAlert_mutex, NULL) != 0)  ||
    (pthread_mutex_init(&rmmTRec[inst]->Gprps_mutex, NULL) != 0)        ||
    (pthread_mutex_init(&rmmTRec[inst]->Fireout_mutex, NULL) !=0 ) )
  {

  llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2320), "",
          "The pthread mutex variable failed to initialize.");

    *error_code = RUM_L_E_THREAD_ERROR ;
    rc = RMM_FAILURE ;
  }

  if ( rc != RMM_FAILURE )
  {
    if ( start_transmitter_threads(inst) != RMM_SUCCESS )
    {

      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2321), "",
          "The RUM transmitter instance threads failed to initialize.");

      *error_code = RUM_L_E_THREAD_ERROR ;
      rc = RMM_FAILURE ;
    }
  }

  if ( rc == RMM_FAILURE ) free_transmitter_memory(inst);
  else rmmTRec[inst]->initialized = RMM_TRUE;

  return rc ;
}

/*****************************************************************************************/
void free_transmitter_memory(int inst)
{
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;

  if ( rmmTRec[inst]->global_token_bucket != NULL )
  {
    if ( destroy_bucket(rmmTRec[inst]->global_token_bucket) == RMM_SUCCESS )
    {

    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5322), "%d", "The Token Bucket was freed for instance {0}.", inst);
    }
    else
    {

    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5323), "%d", "The Token Bucket was not freed for instance {0}.", inst);
    }

  }
  if ( rmmTRec[inst]->CtrlBuffPool != NULL ) MM_free(rmmTRec[inst]->CtrlBuffPool, 1) ;
  if ( rmmTRec[inst]->DataBuffPool != NULL ) MM_free(rmmTRec[inst]->DataBuffPool, 1) ;

  pthread_mutex_destroy(&rmmTRec[inst]->MemoryAlert_mutex) ;
  pthread_mutex_destroy(&rmmTRec[inst]->Gprps_mutex) ;
  pthread_mutex_destroy(&rmmTRec[inst]->Fireout_mutex) ;
  rmm_cond_destroy(&rmmTRec[inst]->FO_CondWait) ;
  rmm_cond_destroy(&rmmTRec[inst]->FO_CondSleep) ;
  if ( rmmTRec[inst]->tTasks.goDown == 1 )
    pthread_mutex_destroy(&rmmTRec[inst]->tTasks.visiMutex) ;

}


/*****************************************************************************************/
int start_transmitter_threads(int inst)
{

  pthread_t tid;
  int n, fo_id = 0;  /* id assigned to main FireOut thread */
  int instance = inst;
  int rc = RMM_SUCCESS;
  int fo_params[2];
  int attrInit;
  pthread_attr_t attr_rt , *pattr_rt=NULL;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;
char errMsg[ERR_MSG_SIZE];
  /* FireOut Thread */

  attrInit = (pthread_attr_init(&attr_rt)) ? 0 : 1;
  if ( attrInit && (rmmTRec[inst]->T_advance.ThreadPriority_tx > 0) )
  {
  if ( rmm_set_thread_priority(&attr_rt, SCHED_RR,rmmTRec[inst]->T_advance.ThreadPriority_tx, errMsg, ERR_MSG_SIZE) == RMM_FAILURE ){
     llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3181),"%d%s",
     "RUM failed to set the thread priority to {0}. The error is: {1}", rmmTRec[inst]->T_advance.ThreadPriority_tx,errMsg);
  }
    else
      pattr_rt = &attr_rt ;
  }
  if ( attrInit && (rmmTRec[inst]->T_advance.ThreadAffinity_tx > 0) )
  {
#if USE_AFF
    if ( rmm_set_thread_affinity(&attr_rt, 0, rmmTRec[inst]->T_advance.ThreadAffinity_tx, errMsg, ERR_MSG_SIZE) == RMM_FAILURE )
     {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(5182),"%llx%s",
               "RUM failed to set the thread affinity to {0}. The error is: {1}", 
         rmmTRec[inst]->T_advance.ThreadAffinity_tx,errMsg);
     }
    else
      pattr_rt = &attr_rt ;
#else
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_WARN, MSG_KEY(5324), "",
        "The thread affinity support is not available.");
#endif
  }

  fo_params[0] = instance;
  fo_params[1] = fo_id;
  if ( pthread_create(&tid, pattr_rt, FireOutThread, (void *)&fo_params) == 0 || (pattr_rt != NULL &&
       pthread_create(&tid, NULL    , FireOutThread, (void *)&fo_params) == 0 ) )
  {
    rmmTRec[inst]->FireOutStatus.thread_id = tid ;
  }
  else
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2325), "",
        "The FireOut thread failed to initialize. ");

    rc = RMM_FAILURE ;
  }

  /* Repair Thread */
  if ( pthread_create(&tid, NULL, RumRepairThread, (void *)&instance) == 0 )
  {
    rmmTRec[inst]->RepairStatus.thread_id = tid ;
  }
  else
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2326), "",
        "The Repair thread failed to initialize. ");
    rc = RMM_FAILURE ;
  }

  /* Monitor Thread (TaskTimer) */
  /* if ( pthread_create(&tid, NULL, MonitorThread,(void *)&instance) == 0 ) */
  if ( set_timer_tasks(inst) == RMM_SUCCESS )
  {
    if ( pthread_create(&tid, NULL, TaskTimer,(void *)&rmmTRec[inst]->tTasks) == 0 )
    {
      rmmTRec[inst]->MonitorStatus.thread_id = tid ;
    }
    else
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2327), "",
        "The Monitor (TaskTimer) thread failed to initialize. ");
      rc = RMM_FAILURE ;
    }
  }
  else
    rc = RMM_FAILURE ;


  /* allow threads to initialize and report status of THREAD_RUNNING  */
  n = 100;
  while ( n > 0 )
  {
    if ( rmmTRec[inst]->FireOutStatus.status == THREAD_RUNNING &&
      rmmTRec[inst]->RepairStatus.status == THREAD_RUNNING  &&
      /*rmmTRec[inst]->SpmStatus.status == THREAD_RUNNING     &&*/
      rmmTRec[inst]->tTasks.ThreadUP == 1 ) break;

    if ( rmmTRec[inst]->FireOutStatus.status == THREAD_EXIT ||
      rmmTRec[inst]->RepairStatus.status == THREAD_EXIT )
      /* || rmmTRec[inst]->SpmStatus.status == THREAD_EXIT )*/
    {
      n = 0;
      break;
    }

    Sleep(100);
    n--;
  }
  /* report TaskTimer up since thread does not report */
  if ( rmmTRec[inst]->tTasks.ThreadUP == 1 )
  {
   llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5186),"%s%llu","The {0} thread was created. Thread id: {1}",
    "Monitor",LLU_VALUE(rmmTRec[inst]->tTasks.threadId));

  }

  if ( n <= 0 ) rc = RMM_FAILURE;
  if ( rc == RMM_FAILURE ) stop_transmitter_threads(inst);

  if(attrInit)
    pthread_attr_destroy(&attr_rt);

  return rc;
}

/*****************************************************************************************/
int set_timer_tasks(int inst)
{
  int i,rc;
  rmm_uint64 current_time;
  TaskInfo task ;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;

  memset(&rmmTRec[inst]->tTasks,0,sizeof(rmmTRec[inst]->tTasks)) ;

  if ( (rc=pthread_mutex_init(&rmmTRec[inst]->tTasks.visiMutex, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2329), "%d",
        "The tTasks.visiMutex failed to initialize. The error code is {0}.", rc);
    return RMM_FAILURE ;
  }

  current_time = rmmTime(NULL,NULL);
  i=0 ;
  memset(&task,0,sizeof(TaskInfo)) ;

  /* Task 1: remove_closed_queues ; Interval - 250 */
  task.nextTime = current_time + 100 ;
  task.taskParm[0] = inst ;
  task.taskData = NULL ;
  task.taskCode = call_remove_closed_topics ;
  i |= TT_add_task(&rmmTRec[inst]->tTasks,&task) ;

  /* Task 2: monitor_all_streams ; Interval - MONITOR_INTERVAL_MILLI (250) */
  task.nextTime = current_time + 100 ;
  task.taskParm[0] = inst ;
  task.taskData = NULL ;
  task.taskCode = call_monitor_all_streams ;
  i |= TT_add_task(&rmmTRec[inst]->tTasks,&task) ;


  /* Task 3: print_snapshot ; Interval - SnapshotCycleMilli_tx */
  if ( rmmTRec[inst]->T_advance.SnapshotCycleMilli_tx > 0 )
  {
    task.nextTime = current_time + rmmTRec[inst]->T_advance.SnapshotCycleMilli_tx/2 ;
    task.taskParm[0] = inst ;
    task.taskData = NULL ;
    task.taskCode = call_print_snapshot ;
    i |= TT_add_task(&rmmTRec[inst]->tTasks,&task) ;
  }

  /* Task 4: update_batch_time ; Interval - BATCH_TIME_UPDATE_SEC*1000 (10000) */
  if ( rmmTRec[inst]->T_advance.BatchingMode != 0 )
  {
    task.nextTime = current_time + 1000 ;
    task.taskParm[0] = inst ;
    task.taskData = NULL ;
    task.taskCode = call_update_batch_time ;
    i |= TT_add_task(&rmmTRec[inst]->tTasks,&task) ;
  }

  /* Task 5: update_dynamic_rate ; Interval - DYNAMIC_RATE_UPDATE_MILLI (20) */
  if ( rmmTRec[inst]->T_config.LimitTransRate == RMM_DYNAMIC_RATE_LIMIT )
  {
    task.nextTime = current_time + 100 ;
    task.taskParm[0] = inst ;
    task.taskData = NULL ;
    task.taskCode = call_update_dynamic_rate ;
    i |= TT_add_task(&rmmTRec[inst]->tTasks,&task) ;
  }
  /* Task 6: update_token_bucket ; Interval - bucket->sleep_time (9) */
  if ( rmmTRec[inst]->T_config.LimitTransRate != RMM_DISABLED_RATE_LIMIT )
  {
    task.nextTime = current_time + 10 ;
    task.taskParm[0] = inst ;
    task.taskData = NULL ;
    task.taskCode = update_token_bucket ;
    /* change status of token bucket to running */
    rmmTRec[inst]->global_token_bucket->status = THREAD_RUNNING ;
    i |= TT_add_task(&rmmTRec[inst]->tTasks,&task) ;
  }

  /* Task 7: signal_fireout ; Interval - 50 */
  task.nextTime = current_time + 100 ;
  task.taskParm[0] = inst ;
  task.taskData = NULL ;
  task.taskCode = signal_fireout ;
  i |= TT_add_task(&rmmTRec[inst]->tTasks,&task) ;
  if ( i )
  {
    TT_del_all_tasks(&rmmTRec[inst]->tTasks) ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2330), "", "The RUM transmitter failed to add tasks to the TimerTask thread.");

    return RMM_FAILURE ;
  }
  rmmTRec[inst]->tTasks.reqSleepTime = SHORT_SLEEP ;

  return RMM_SUCCESS;
}

int detach_trans_thread(int inst, const char *name, pthread_t id)
{
  int rc = 0 ;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;

  if (id){
   rc=pthread_detach(id);
  }
  if(rc){
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3080),"%s%d",
    "RUM failed to detach thread {0}. The error code is {1}.",
    name,rc);
    return RUM_FAILURE;
  }else{
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6106),"%s%llu",
      "Thread {0} with thread id {1} was detached.", name,THREAD_ID(id));
    return RMM_SUCCESS;
  }
}



/*****************************************************************************************/
int stop_transmitter_threads(int inst)
{
  int n, res, rc = RMM_SUCCESS;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;

  pthread_t zid;
  void *result ;

  pthread_mutex_lock(&rmmTRec[inst]->Gprps_mutex);

  if ( rmmTRec[inst]->FireOutStatus.status != THREAD_EXIT )
    rmmTRec[inst]->FireOutStatus.status = THREAD_KILL;
  if ( rmmTRec[inst]->RepairStatus.status != THREAD_EXIT )
    rmmTRec[inst]->RepairStatus.status  = THREAD_KILL;
    /*  if ( rmmTRec[inst]->SpmStatus.status    != THREAD_EXIT )
  rmmTRec[inst]->SpmStatus.status     = THREAD_KILL;      */
  rmmTRec[inst]->tTasks.goDown = 1 ;
  if ( rmmTRec[inst]->global_token_bucket != NULL )
  {
    if ( rmmTRec[inst]->global_token_bucket->status != THREAD_EXIT )
      rmmTRec[inst]->global_token_bucket->status = THREAD_KILL;
  }

  pthread_mutex_unlock(&rmmTRec[inst]->Gprps_mutex);
  memset(&zid,0,sizeof(pthread_t)) ;

  /* allow threads to exit and report status of THREAD_EXIT  */
  n = 20;
  while ( n > 0 )
  {
    rmm_cond_signal(&rmmTRec[inst]->FO_CondWait,1); /* wake up Fireout */
    rmm_cond_signal(&rmmTRec[inst]->FO_CondSleep,1); /* wake up Fireout */
    if ( rmmTRec[inst]->rumInfo->recvNacksQ )
      BB_signalE(rmmTRec[inst]->rumInfo->recvNacksQ) ; /* wake up Repair */

    if ( rmmTRec[inst]->FireOutStatus.status == THREAD_EXIT &&
      rmmTRec[inst]->RepairStatus.status == THREAD_EXIT  &&
      /*rmmTRec[inst]->SpmStatus.status == THREAD_EXIT     && */
      rmmTRec[inst]->tTasks.ThreadUP == 0 ) break;
    Sleep(100);
    n--;

    if ( rmmTRec[inst]->global_token_bucket != NULL )
      pthread_cond_broadcast(&rmmTRec[inst]->global_token_bucket->waiting);

  }

  /* report TaskTimer exit since thread does not report */
  if ( rmmTRec[inst]->tTasks.ThreadUP == 0 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5067),"%s%llu",
      "The {0}({1}) thread was stopped.",
      "Monitor", LLU_VALUE(rmmTRec[inst]->tTasks.threadId));
  }

  if ( n <= 0 )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_WARN, MSG_KEY(4185), "%d",
        "Not all threads stopped gracefully for RUM instance {0}.  The remaining threads will now be terminated.",
        inst);

  }

  Sleep(BASE_SLEEP);

  /* FireOut Thread */
  if ( rmmTRec[inst]->FireOutStatus.status != THREAD_EXIT && memcmp(&rmmTRec[inst]->FireOutStatus.thread_id,&zid,sizeof(pthread_t)) )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5336), "", "The RUM transmitter is attempting to terminate the FireOut thread");


    if ( (res=pthread_cancel(rmmTRec[inst]->FireOutStatus.thread_id)) != 0 )
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3337), "%d",
        "The RUM transmitter attempt to terminate the FireOut thread failed. The error code is {0}.", res);

      rc = RMM_FAILURE ;
    }
    if ( (res=pthread_join(rmmTRec[inst]->FireOutStatus.thread_id, &result)) != 0 )
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3338), "%d",
        "The RUM transmitter failed to join the FireOut thread. The error code is {0}.", res);
      rc = RMM_FAILURE ;
    }
    if ( result != PTHREAD_CANCELED )
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3339), "%llx", "The RUM transmitter failed to cancel the FireOut thread. The error code is {0}.", LLU_VALUE(result));
      rc = RMM_FAILURE ;
    }
    else
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5340), "",
        "The FireOut thread was canceled.");

    }
  } else {
    if ( (res=detach_trans_thread(inst,"FireOut",rmmTRec[inst]->FireOutStatus.thread_id)) != 0 )
    {
      rc = RMM_FAILURE ;
    }
  }



  /* Repair Thread */
  if ( rmmTRec[inst]->RepairStatus.status != THREAD_EXIT && memcmp(&rmmTRec[inst]->RepairStatus.thread_id,&zid,sizeof(pthread_t)) )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5341), "", "The RUM transmitter is attempting to terminate the Repair thread.");


    if ( (res=pthread_cancel(rmmTRec[inst]->RepairStatus.thread_id)) != 0 )
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3342), "%d",
        "The RUM transmitter attempt to terminate the Repair thread failed. The error code is {0}.", res);

      rc = RMM_FAILURE ;
    }
    if ( (res=pthread_join(rmmTRec[inst]->RepairStatus.thread_id, &result)) != 0 )
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3343), "%d",
        "The RUM transmitter failed to join the Repair thread. The error code is {0}.", res);

      rc = RMM_FAILURE ;
    }
    if ( result != PTHREAD_CANCELED )
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3344), "%llx", "The RUM transmitter failed to cancel the Repair thread. The error code is {0}.", LLU_VALUE(result));

      rc = RMM_FAILURE ;
    }
    else
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5345), "",
        "The Repair thread was canceled.");

    }
  } else {
    if ( (res=detach_trans_thread(inst,"Repair",rmmTRec[inst]->RepairStatus.thread_id)) != 0 )
    {
      rc = RMM_FAILURE ;
    }
  }

  /* Monitor Thread */
  if ( rmmTRec[inst]->tTasks.ThreadUP > 0 && memcmp(&rmmTRec[inst]->MonitorStatus.thread_id,&zid,sizeof(pthread_t)))
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5346), "",
        "The RUM transmitter is attempting to terminate the Monitor thread.");


    if ( (res=pthread_cancel(rmmTRec[inst]->MonitorStatus.thread_id)) != 0 )
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3347), "%d",
        "The RUM transmitter attempt to terminate the Monitor thread failed. The error code is {0}.", res);

      rc = RMM_FAILURE ;
    }
    if ( (res=pthread_join(rmmTRec[inst]->MonitorStatus.thread_id, &result)) != 0 )
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3348), "%d",
        "The RUM transmitter failed to join the Monitor thread. The error code is {0}.", res);

      rc = RMM_FAILURE ;
    }
    if ( result != PTHREAD_CANCELED )
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3349), "%llx",
         "The RUM transmitter failed to cancel the Monitor thread. The error code is {0}.", LLU_VALUE(result));

      rc = RMM_FAILURE ;
    }
    else
    {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5350), "",
        "The Monitor thread was canceled.");

    }
  } else {
    if ( (res=detach_trans_thread(inst,"Monitor",rmmTRec[inst]->MonitorStatus.thread_id)) != 0 )
    {
      rc = RMM_FAILURE;
    }
  }

  return rc;
}

/***********************   3 3 3 3   *****************************************************/
int  rumT_Create_Queue(rumInstanceRec *rumInfo, const char *queue_name, int reliability_level, rumConnection *rum_connection,
                       rum_on_event_t on_event, void *event_user,rumQueueT *queue_t, rumStreamID_t *stream_id,
                       int is_late_join, pgm_seq lj_mark, int app_con_batch, int *error_code)
{
  StreamInfoRec_T *pSinf ;
  ConnInfoRec *cInfo;
  int inst ;
  const char* methodName = "rumT_Create_Queue";
  TCHandle tcHandle;
  if ( (inst = rumT_chk_instance(rumInfo->instance_t, &tcHandle, error_code)) < 0 )
    return RMM_FAILURE ;
  LOG_METHOD_ENTRY();

  queue_t->handle = queue_t->instance = -1;

  if ( rum_connection->rum_instance != rumInfo->instance )
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3351), "",
      "Conflicting parameters were passed to the rumTCreateQueue API. The rumQueueT and the rumConnection it is created on must both be on the same rumInstance.");

    rmmTmutex_unlock() ;
    return RMM_FAILURE ;
  }

  if (queue_name == NULL)
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    LOG_NULL_PARAM("queue_name");

    rmmTmutex_unlock() ;
    return RMM_FAILURE ;
  }

  if ( strllen(queue_name,RUM_MAX_QUEUE_NAME) + 1 > RUM_MAX_QUEUE_NAME)
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3352), "%d%s",
        "The queue name {1} is too long.  The maximum name length is {0} characters, including a null terminator.",
        RUM_MAX_QUEUE_NAME, queue_name);

    rmmTmutex_unlock() ;
    return RMM_FAILURE ;
  }

  if ( reliability_level != RUM_UNRELIABLE &&
       reliability_level != RUM_RELIABLE &&
       reliability_level != RUM_RELIABLE_NO_HISTORY )
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3353), "%d",
        "The specified reliability parameter ({0}) is not valid.  A value from the RUM_Tx_RELIABILITY_t enumeration must be used.", reliability_level);

    rmmTmutex_unlock() ;
    return RMM_FAILURE ;
  }

  /* find connection record */
  rmm_rwlock_rdlock(&rumInfo->ConnSyncRW) ;
  cInfo = rum_find_connection(rumInfo, rum_connection->connection_id, 0, 0);
  if ( cInfo == NULL )
  {
    *error_code = RUM_L_E_CONN_INVALID ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3354), "%s",
        "The queue {0} cannot be created because a valid rumConnection was not found.", queue_name);
    rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ;
    rmmTmutex_unlock() ;
    return RMM_FAILURE ;
  }

  if ( !is_connT_valid(cInfo) || cInfo->n_tx_streams >= CONNCTION_STREAMS_ARRAY_SIZE )
  {
    *error_code = RUM_L_E_CONN_INVALID ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3355), "%d%d%d%s",
        "The queue {3} cannot be created because the rumConnection is not valid or has too many streams ({0} {1} {2}).", is_connT_valid(cInfo), cInfo->n_tx_streams, CONNCTION_STREAMS_ARRAY_SIZE, queue_name);
    rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ;
    rmmTmutex_unlock() ;
    return RMM_FAILURE ;
  }


  /* assign position in all_streams array */
  if ( (rmmTRec[inst]->counter >= rmmTRec[inst]->T_advance.MaxStreamsPerTransmitter) ||
    (rmmTRec[inst]->all_streams[rmmTRec[inst]->counter] != NULL)  )
  {
    for (rmmTRec[inst]->counter=0;
      rmmTRec[inst]->counter < rmmTRec[inst]->T_advance.MaxStreamsPerTransmitter &&
        rmmTRec[inst]->all_streams[rmmTRec[inst]->counter] != NULL;
      rmmTRec[inst]->counter++);
  }
  if (rmmTRec[inst]->all_streams[rmmTRec[inst]->counter] != NULL)
  {
    *error_code = RUM_L_E_TOO_MANY_STREAMS  ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3356), "%s%d",
        "The queue {0} cannot be created because the maximum number of streams per transmitter {1} has been exceeded.", queue_name, rmmTRec[inst]->T_advance.MaxStreamsPerTransmitter);

    rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ;
    rmmTmutex_unlock() ;
    return RMM_FAILURE;
  }

  if ( (pSinf = (StreamInfoRec_T *)malloc(sizeof(StreamInfoRec_T))) == NULL)
  {
    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
    LOG_MEM_ERR(tcHandle, methodName, sizeof(StreamInfoRec_T));

    rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ;
    rmmTmutex_unlock() ;
    return RMM_FAILURE;
  }
  memset(pSinf, 0, sizeof(StreamInfoRec_T));

  /* initialize stream parameters in stream's record */
  if ( create_queue_T(inst, pSinf, cInfo, rmmTRec[inst]->counter, queue_name, reliability_level, rum_connection, on_event, event_user, is_late_join, lj_mark, app_con_batch, error_code) != RMM_SUCCESS)
  {
    *error_code = RUM_L_E_QUEUE_ERROR ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3357), "%s",
        "The queue {0} cannot be created.", queue_name);

    free_stream_memory(pSinf);
    free(pSinf);
    rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ;
    rmmTmutex_unlock() ;
    return RMM_FAILURE;
  }

   /* keep FO_mutex as long as possible to prevent FO from removing the stream before it is properly installed */
  pthread_mutex_lock(&rmmTRec[inst]->Fireout_mutex);
  /* add stream to connection */
  if ( update_stream_array(cInfo, pSinf, inst, 0, 0, 0) == RMM_FAILURE )
  {
    *error_code = RUM_L_E_QUEUE_ERROR ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3358), "%s",
        "The queue {0} cannot be created because the call to function update_stream_array failed.", queue_name);

    free_stream_memory(pSinf);
    free(pSinf);
    pthread_mutex_unlock(&rmmTRec[inst]->Fireout_mutex);
    rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ;
    rmmTmutex_unlock() ;
    return RMM_FAILURE;
  }

  rmmTRec[inst]->number_of_streams++ ;
  if ( rmmTRec[inst]->max_stream_index < rmmTRec[inst]->T_advance.MaxStreamsPerTransmitter )
    rmmTRec[inst]->max_stream_index++ ;
  rmmTRec[inst]->MaxPendingStreamPackets = rmmTRec[inst]->MaxPendingPackets / rmmTRec[inst]->number_of_streams;
  rmmTRec[inst]->MaxPendingStreamPackets = rmmMax(2,rmmTRec[inst]->MaxPendingStreamPackets) ;

  /* create entry in all_streams array  */
  rmmTRec[inst]->all_streams[rmmTRec[inst]->counter] = pSinf ;

  rmm_signal_fireout(inst,1,pSinf);

  if ( stream_id )
    *stream_id = pSinf->stream_id ;

llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5359), "%s%s%d%d%d%d%d",
    "The RUM transmitter created queue {0} (sid={1}, counter={2}, src_port={3}, reliability={4}, is_late_join={5}, app_con_batch={6}).",
     queue_name, pSinf->stream_id_str, rmmTRec[inst]->counter, pSinf->src_port, pSinf->reliability, is_late_join, app_con_batch );


  queue_t->handle = rmmTRec[inst]->counter;
  queue_t->instance = inst;
  rmmTRec[inst]->counter++ ;
  pthread_mutex_unlock(&rmmTRec[inst]->Fireout_mutex);
  rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ;
  rmmTmutex_unlock() ;

  LOG_METHOD_EXIT();

  return RMM_SUCCESS;
}

/***********************   3 3 3 3   *****************************************************/
int create_queue_T(int inst, StreamInfoRec_T *pSinf, ConnInfoRec *cInfo, uint16_t counter, const char *queue_name, int reliable, rumConnection *rum_connection,
                   rum_on_event_t on_event, void *event_user,
                   int is_late_join, pgm_seq lj_mark, int app_con_batch, int *error_code)
{
  int base, size, ierror;
  uint16_t id;
  char *bptr ;
  StreamInfoRec_T *tmp_pSinf ;
  int i, stream, id_exist=0;
  const char* methodName = "create_queue_T";
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;

  pSinf->Odata_options = pSinf->mtl_buff = pSinf->Spm_P = NULL;
  pSinf->History_Q = NULL;
  pSinf->Rdata_Q = pSinf->Ncf_Q = pSinf->Odata_Q = NULL;

  pSinf->inst_id = inst;
  pSinf->rmmT.handle = counter;
  pSinf->rmmT.instance = inst;
  pSinf->reliability = reliable;
  pSinf->new_reliability = pSinf->reliability ;
  pSinf->reliable    = (pSinf->reliability != RMM_UNRELIABLE) ;

  pSinf->is_failover = 0 ; 
  pSinf->is_backup   = 0 ; 

  pSinf->send_msn = 0 ; 
  pSinf->app_controlled_batch = (app_con_batch != 0) ;
  pSinf->no_partial = (pSinf->app_controlled_batch || pSinf->is_backup)  ;

  /* maintain History for queue? */
  if ( rmmTRec[inst]->keepHistory && reliable != RUM_UNRELIABLE && reliable != RUM_RELIABLE_NO_HISTORY )
    pSinf->keepHistory = 1;
  else
    pSinf->keepHistory = 0;


  /* connection info */
  pSinf->connection_id = cInfo->connection_id;
  pSinf->conn_invalid = RMM_FALSE ;   /* indicates that a connection has reference to the stream */
  pSinf->FO_in_use = RMM_TRUE ;
  pSinf->conn_invalid_time = 0 ;

  pSinf->LimitTransRate = rmmTRec[inst]->T_config.LimitTransRate;
  pSinf->active = RMM_TRUE ;
  pSinf->create_time = rmmTRec[inst]->rumInfo->CurrentTime ;
  pSinf->close_time = 0 ;
  pSinf->remove_time = 0 ;
  pSinf->closed = RMM_FALSE;


  pSinf->txw_trail = 1 ;
  pSinf->txw_lead = pSinf->txw_trail-1  ;   /* lead is incremented before use so first packet sqn will be 1 */
  pSinf->spm_seq_num = 1;

  for (i=0; i<65536; i++)
  {
    id = htons((uint16_t)(rmmTRec[inst]->streamID_counter + i) );
    if ( id == 0 ) continue;

    /* make sure no other stream is using this ID */
    id_exist = 0;
    for (stream = 0; stream < rmmTRec[inst]->max_stream_index; stream++)
    {
      if ( (tmp_pSinf = rmmTRec[inst]->all_streams[stream]) == NULL )   continue;
      if ( tmp_pSinf->src_port == id )
      {
        id_exist = 1;
        break;
      }
    }
    if ( !id_exist )
    {
      pSinf->src_port = id;
      rmmTRec[inst]->streamID_counter += i+1;
      break;
    }
  }
  if ( id_exist ) /* free stream ID was not found */
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3361), "", "The RUM transmitter could not assign a stream ID.");

    return RMM_FAILURE ;
  }
  pSinf->dest_port = htons((short)rmmTRec[inst]->T_config.ServerSocketPort) ;
  pSinf->gsi_high = rmmTRec[inst]->gsi_high ;
  pSinf->gsi_low = rmmTRec[inst]->port ;

  bptr = (char *)&pSinf->stream_id ;
  memcpy(bptr,&pSinf->gsi_high,4); bptr += 4 ;
  memcpy(bptr,&pSinf->gsi_low,2);  bptr += 2 ;
  memcpy(bptr,&pSinf->src_port,2);
  b2h(pSinf->stream_id_str,(uint8_t *)&pSinf->stream_id,sizeof(rumStreamID_t)) ;
  b2h(pSinf->conn_id_str,(uint8_t *)&pSinf->connection_id,sizeof(rumConnectionID_t)) ;

  pSinf->on_event = on_event ;
  pSinf->event_user = event_user ;

  pSinf->is_late_join = is_late_join ;
  pSinf->late_join_mark = lj_mark;
  pSinf->late_join_offset_odata = 0 ;
  pSinf->late_join_offset_spm = 0 ;

  if ( cInfo->sock_af == AF_INET6 )  /* IPv6 address */
  {
    /* set IPv6 source_nla */
    pSinf->source_nla_afi = NLA_AFI6 ;
    pSinf->source_nla.length = sizeof(IA6) ;
    memcpy(pSinf->source_nla.bytes,&rmmTRec[inst]->source_nla[1].bytes,rmmTRec[inst]->source_nla[1].length) ;
  }
  else  /* IPv4 address */
  {
    /* set IPv4 source_nla */
    pSinf->source_nla_afi = NLA_AFI ;
    pSinf->source_nla.length = sizeof(IA4) ;
    memcpy(pSinf->source_nla.bytes,&rmmTRec[inst]->source_nla[0].bytes,rmmTRec[inst]->source_nla[0].length) ;
  }

  strlcpy(pSinf->topic_name,queue_name,RUM_MAX_QUEUE_NAME);
  /* queue_name_len = (int)strlcpy(pSinf->topic_name,queue_name,RUM_MAX_QUEUE_NAME); */

  if ( construct_odata_options(pSinf) == RMM_FAILURE )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3363), "",
        "The RUM transmitter failed to construct ODATA options.");

    return RMM_FAILURE ;
  }


  /* construct PGM header */
  bptr = pSinf->pgm_header;
  memcpy(bptr, &pSinf->src_port, 2);
  bptr += 2;
  memcpy(bptr, &pSinf->dest_port, 2);
  bptr += 2;
  NputChar(bptr, (char)PGM_TYPE_ODATA);                       /* type = ODATA                              */
  if ( pSinf->Odata_options_size > 0 )
  {
    NputChar(bptr, rmmTRec[inst]->T_advance.opt_present);  /* options in packet not network significant */
  }
  else
  {
    NputChar(bptr, 0) ;                                    /* no options in RUM Odata                   */
  }
  NputShort(bptr, 0);                            /* zero checksum                             */
  memcpy(bptr, &pSinf->gsi_high, 4);
  bptr += 4;
  memcpy(bptr, &pSinf->gsi_low, 2);
  bptr += 2;
  NputShort(bptr, 0);               /* TSDU = 0             */

  {
    pSinf->sqn_offset = 20 ;
    /* msg_opt_offse variables are set when constructing odata_options and SPM */
  }

  pSinf->spm_pending = RMM_TRUE;
  pSinf->spm_fo_generate = RMM_FALSE;

  pSinf->Spm_on     = RMM_FALSE ;
  pSinf->Repair_on  = RMM_FALSE;
  pSinf->Monitor_on = RMM_FALSE;
  pSinf->FireOut_on = RMM_FALSE ;

  /* initialize all stats to zero */
  memset(&pSinf->stats, 0, sizeof(stream_stats_t));
  /*
  pSinf->stats.bytes_transmitted = 0;
  pSinf->stats.bytes_retransmitted = 0;
  pSinf->stats.messages_sent = 0;
  pSinf->stats.naks_received = 0;
  pSinf->stats.rdata_packets = 0;
  */

  pSinf->mtl_lead_sqn = pSinf->txw_lead;
  /* pSinf->mtl_buffsize = rmmTRec[inst]->T_config.PacketSizeBytes + INT_SIZE ; */
        pSinf->mtl_buffsize = rmmTRec[inst]->T_config.PacketSizeBytes - SHORT_SIZE;   /* sub. short size to simplify checks in submit_message */
  if ( (pSinf->mtl_buff = (char *)MM_get_buff(rmmTRec[inst]->DataBuffPool, 100, &ierror)) == NULL )
  {
    LOG_MEM_ERR(tcHandle, methodName, (rmmTRec[inst]->DataBuffPool->iBufSize));

    return RMM_FAILURE ;
  }
  pSinf->mtl_offset = INT_SIZE + ODATA_HEADER_LENGTH + pSinf->Odata_options_size;
  pSinf->mtl_currsize = pSinf->mtl_offset + FIXED_MTL_HEADER_SIZE;
  pSinf->mtl_messages = 0;
  pSinf->msg_sqn = 1 ;
  pSinf->msg_packet_trail = pSinf->msg_sqn ;
  pSinf->msg_packet_lead = pSinf->msg_sqn;


  /* initialize mutexes                      */
  if ( (pthread_mutex_init(&pSinf->zero_delay_mutex, NULL) != 0) ||
    (pthread_mutex_init(&pSinf->spm_mutex, NULL) != 0)         ||
    (pthread_mutex_init(&pSinf->rdata_mutex, NULL) !=0 )       ||
    (pthread_mutex_init(&pSinf->fireout_mutex, NULL) !=0 ) )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3365), "",
        "The pthread mutex variable failed to initialize.");

    return RMM_FAILURE ;
  }

  /* construct an SPM packet including options  */
  if ( BuildSpmPacket(pSinf, 0) != RMM_SUCCESS )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3367), "", "The RUM transmitter failed to create an SPM Packet.");

    return RMM_FAILURE ;
  }

  /* construct NCF buffer */
  pSinf->Nak_header_length = NAK_HEADER_FIX_LENGTH + 4 + pSinf->source_nla.length + 4 + pSinf->mcast_nla.length;

  base = pSinf->txw_trail; /* sqn of first packet in queue */
  if ( pSinf->reliability != RMM_UNRELIABLE ) size = 1;
  else size = 0;
  if ( ((pSinf->Ncf_Q = LL_alloc(CONTROL_PACKET_BUF_SIZE-2*sizeof(void *), base)) == NULL)     ||
    ((pSinf->Rdata_Q = LL_alloc(rmmTRec[inst]->packet_header_offset+offsetof(rmmTxPacketHeader,ll_next), base)) == NULL) ||
    ((pSinf->History_Q = BB_alloc(size*rmmTRec[inst]->MaxPacketsAllowed, base)) == NULL) ||
    ((pSinf->Odata_Q = LL_alloc(rmmTRec[inst]->packet_header_offset+offsetof(rmmTxPacketHeader,ll_next), base)) == NULL)  )
  {
    {
    LOG_MEM_ERR(tcHandle, methodName, (sizeof(void*)*2*rmmTRec[inst]->MaxPendingPackets));
    }

    return RMM_FAILURE ;
  }

  pSinf->monitor_needed.trail = 0;
  pSinf->monitor_needed.lead = 0;
  pSinf->monitor_needed.rdata_sqn = 0;

  if ( rmmTRec[inst]->T_advance.BatchingMode > 0 )
    pSinf->batching_time = rmmTRec[inst]->max_batching_time;
  else
    pSinf->batching_time = 0 ;
  pSinf->batching_stamp = 0;

  pSinf->mtl_version = (char)PGM_VERSION ;

  return RMM_SUCCESS ;

}

/*****************************************************************************************/
int construct_odata_options(StreamInfoRec_T *pSinf)
{
  char opt_end=0, *popt;
  int inst = pSinf->inst_id;
  uint16_t Odata_options_size ;
  const char* methodName = "construct_odata_options";
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;

  /* only HA and late join options in ODATA packets */
  Odata_options_size = 0;
  if ( pSinf->is_late_join == RMM_TRUE )
  {
    Odata_options_size += 8 ;  /* OPT_JOIN length is 8 bytes */
    opt_end = PGM_OPT_JOIN ;
  }

  if ( Odata_options_size > 0 )
  {
    Odata_options_size += 4 ;
    if ( pSinf->Odata_options == NULL || Odata_options_size > pSinf->Odata_options_size )
    {
      char *opts ;
      if ( (opts = (char *)malloc(Odata_options_size)) == NULL )
      {
    LOG_MEM_ERR(tcHandle, methodName, (Odata_options_size));

        return RMM_FAILURE ;
      }
      if ( pSinf->Odata_options )
        free(pSinf->Odata_options) ;
      pSinf->Odata_options = opts ;
      pSinf->Odata_options_size = Odata_options_size ;
      pSinf->pgm_header[5] = rmmTRec[inst]->T_advance.opt_present ;
    }
  }
  else
  {
    if ( pSinf->Odata_options )
      free(pSinf->Odata_options) ;
    pSinf->Odata_options = NULL ;
    pSinf->Odata_options_size = Odata_options_size ;
    pSinf->pgm_header[5] = 0 ;
    return RMM_SUCCESS ;
  }

  memset(pSinf->Odata_options, 0, pSinf->Odata_options_size);
  popt = pSinf->Odata_options ;

  /* PGM opt_length header  */
  NputChar(popt, PGM_OPT_LENGTH);
  NputChar(popt, 4);                                    /* opt_length                         */
  NputShort(popt, pSinf->Odata_options_size); /*length of all options               */

  /* Construct Options. RMM_OPT_MSG_INFO MUST is the first option                             */

  /* RMM Option: messsage information  msg_sqn_trail(8) + msg_sqn_lead(8) + reliability(1)    */

  /* PGM OPT_JOIN (late join)  */
  if ( pSinf->is_late_join == RMM_TRUE )
  {
    if ( opt_end == PGM_OPT_JOIN )
    {
      NputChar(popt, (char)(PGM_OPT_JOIN | PGM_OPT_END));                 /* type of RMM OPT_MSG_INFO                 */
    }
    else
    {
      NputChar(popt, PGM_OPT_JOIN);                 /* type of RMM OPT_MSG_INFO                 */
    }
    NputChar(popt, 0);                            /* opt length in RUM is a short              */
    NputShort(popt, 8);                 /* OPT_JOIN is 8 bytes                 */
    pSinf->late_join_offset_odata = (int)(popt - pSinf->Odata_options);
    NputInt(popt, pSinf->late_join_mark); /* currrent late join mark                  */
  }


  return RMM_SUCCESS ;
}


/*****************************************************************************************/
void free_stream_memory(StreamInfoRec_T *pSinf)
{
  char *packet;
  int inst = pSinf->inst_id;

  if ( pSinf->Odata_options != NULL ) free(pSinf->Odata_options);
  if ( pSinf->mtl_buff != NULL ) MM_put_buff(rmmTRec[inst]->DataBuffPool, pSinf->mtl_buff) ;
  if ( pSinf->Spm_P != NULL ) free(pSinf->Spm_P);

  /* Free NCF Queue */
  if ( pSinf->Ncf_Q != NULL )
  {
    while ( (packet = LL_get_buff(pSinf->Ncf_Q, 1)) != NULL )
      MM_put_buff(rmmTRec[inst]->CtrlBuffPool, packet);
    LL_free(pSinf->Ncf_Q, 1) ;
  }
  /* Free Rdata Queue ignoring packets in the queue */
  if ( pSinf->Rdata_Q != NULL ) LL_free(pSinf->Rdata_Q, 0) ;

  /* Free Odata Queue */
  if ( pSinf->Odata_Q != NULL )
  {
    while ( (packet = LL_get_buff(pSinf->Odata_Q, 1)) != NULL )
      MM_put_buff(rmmTRec[inst]->DataBuffPool, packet);
    LL_free(pSinf->Odata_Q, 1) ;
  }

  /* Free History Queue */
  if ( pSinf->History_Q != NULL )
  {
    while ( (packet = BB_get_buff(pSinf->History_Q, 1)) != NULL )
      MM_put_buff(rmmTRec[inst]->DataBuffPool, packet);
    BB_free(pSinf->History_Q, 1) ;
  }

  pthread_mutex_destroy(&pSinf->zero_delay_mutex) ;
  pthread_mutex_destroy(&pSinf->spm_mutex) ;
  pthread_mutex_destroy(&pSinf->rdata_mutex) ;
  pthread_mutex_destroy(&pSinf->fireout_mutex) ;

}

/***********************   3 3 3 3   *****************************************************/
int BuildSpmPacket(StreamInfoRec_T *pSinf, int close_linger)
{
  char *pspm;
  uint16_t options_length, cs;
  unsigned char rel_opt;
  uint16_t      control_opt_len, fixed_opt_len;
  int total_length, malloc_size ;
  int topic_name_len = (int)strllen(pSinf->topic_name,sizeof(pSinf->topic_name));
  int inst = pSinf->inst_id;
  const char* methodName = "BuildSpmPacket";
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;

  /* option length includes OPT_LENGTH (4) + OPT_CONTROL (4+8) + OPT_TOPIC_NAME (4 + topic len) */
  /*  should be FIXED_OPT_LEN = 16                                                              */
  control_opt_len = 12;
  fixed_opt_len = control_opt_len + 8;
  /* if ( append_fin ) fixed_opt_len += 4; part of RUM_OPT_CONTROL */
  if ( pSinf->is_late_join == RMM_TRUE ) fixed_opt_len += 8;

  options_length = fixed_opt_len + topic_name_len;
  total_length = SPM_HEADER_FIX_LENGTH + 4 + pSinf->source_nla.length + options_length;
  malloc_size = 64 + (total_length/64 + 1)*64 ; /* leave space for fin etc. and round to 64 */
  if ( pSinf->Spm_P == NULL )
  {
    pSinf->Spm_P = (char *)malloc(malloc_size);
    if (pSinf->Spm_P == NULL)
    {
    LOG_MEM_ERR(tcHandle, methodName, malloc_size);

      return RMM_FAILURE;
    }
    memset(pSinf->msg2pac_option, 0, MSG2PAC_OPT_SIZE);
  }

  pspm = pSinf->Spm_P;
  /* copy the general fields into the spm packet */
  memset(pspm, 0, total_length + INT_SIZE);
  NputInt(pspm, total_length);       /* total length prefix in nbo         */
  /*memcpy(pspm, &total_length, INT_SIZE);
    pspm += INT_SIZE;*/
  memcpy(pspm, &pSinf->src_port, 2);
  pspm += 2;
  memcpy(pspm, &pSinf->dest_port, 2);
  pspm +=2;
  NputChar(pspm, (char)RUM_TYPE_SCP);
  NputChar(pspm, rmmTRec[inst]->T_advance.opt_present); /* set options to present but not network significant */
  pspm += 2;                      /* skip checksum = 0   */
  memcpy(pspm, &pSinf->gsi_high, 4);
  pspm += 4;
  memcpy(pspm, &pSinf->gsi_low, 2);
  pspm += 4;                     /* skip gsi_low + TSDU  */
  NputInt(pspm, pSinf->spm_seq_num);
  NputInt(pspm, pSinf->txw_trail);
  NputInt(pspm, pSinf->txw_lead);
  NputShort(pspm, pSinf->source_nla_afi);
  pspm += 2;                      /* skip reserved field */
  memcpy(pspm, &pSinf->source_nla.bytes, pSinf->source_nla.length);
  pspm += pSinf->source_nla.length;

  /* Append Options. RMM_OPT_MSG_INFO is the first option after OPT_LENGTH              */

  /* PGM opt_length header  */
  NputChar(pspm, PGM_OPT_LENGTH);
  NputChar(pspm, 4);                         /* opt_length                              */
  NputShort(pspm, options_length); /* length of all options                   */

  /* RMM Option: messsage information  msg_sqn_trail(8) + msg_sqn_lead(8) + reliability(1)   */
  /*   this option is first on the list to allow easy updates of msg sqn                     */

  /* RMM Option: RMM_OPT_MSG_TO_PAC information. only for HA replay QoS, MSG2PAC_OPT_SIZE (80) bytes  */
  memset(pSinf->msg2pac_option, 0, MSG2PAC_OPT_SIZE);
  pSinf->msg2pac_offset_spm = 0;

  /* PGM OPT_JOIN (late join)  */
  if ( pSinf->is_late_join == RMM_TRUE )
  {
    NputChar(pspm, PGM_OPT_JOIN);                 /* type of RMM OPT_MSG_INFO                 */
    NputChar(pspm, 0);                            /* OPT_JOIN is 8 bytes                      */
    NputShort(pspm, 8);                 /* rest of the fields are 0                 */
    pSinf->late_join_offset_spm = (int)(pspm - pSinf->Spm_P);
    NputInt(pspm, pSinf->late_join_mark); /* currrent late join mark                  */
  }

  /* RUM Control Option:  reliability(1)+isActive(1)+ keep_history(1)+reserved(1)+close_linger(4)  */
  /* hb_timeout = (uint16_t)(rmmTRec[inst]->T_advance.HeartbeatTimeoutMilli/1000); rmmTRec[inst]->T_config.HeartbeatTimeoutSec;*/
  NputChar(pspm, RUM_OPT_CONTROL);            /* type of RMM OPT_ADDR                          */
  NputChar(pspm, 0);                          /* not used                                      */
  NputShort(pspm, control_opt_len); /* opt_length                                    */
  rel_opt = pSinf->reliability ; 
  NputChar(pspm, rel_opt);                    /* reliability information                       */
  NputChar(pspm, (close_linger==0));
  NputChar(pspm, pSinf->keepHistory);         /* keepHistory                                   */
  NputChar(pspm, 0); //pSinf->enable_msg_properties);
  NputInt(pspm, close_linger);


  /* RMM topic name option   */
  NputChar(pspm, (char)(RUM_OPT_QUEUE_NAME | PGM_OPT_END)); /* type of RMM OPT_TOPIC_NAME      */
  NputChar(pspm, 0);
  cs = (topic_name_len+4) ;
  NputShort(pspm, cs);            /* opt_length                      */
  memcpy(pspm, pSinf->topic_name, topic_name_len);

  return RMM_SUCCESS;
}


/*****************************************************************************************/
int checkT_configuration_parameters(int inst)
{
  char timestr[32];
  int rc = RMM_SUCCESS;
  TCHandle tcHandle = rmmTRec[inst]->tcHandle;
  TBHandle tbh = NULL;
  rumBasicConfig config = rmmTRec[inst]->T_config;
  rumAdvanceConfig *adv_config = &rmmTRec[inst]->T_advance;

  get_time_string(timestr);

  tbh = llmCreateTraceBuffer(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5380));

  if (tbh != NULL) llmAddTraceData(tbh, "", "The RUM transmitter is being created using the following configuration parameters: Basic: [");

#if   USE_SSL
  if ( config.Protocol != RUM_TCP && config.Protocol != RUM_SSL)
  { 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3381),"%s%d%d%s%d%s",
        "The RUM transmitter configuration parameter {0} is not valid. "
        "The requested value is {1}. The valid values are {2}({3}) or {4}({5}).",
        "Protocol",config.Protocol,RUM_TCP,"RUM_TCP",RUM_SSL,"RUM_SSL");
    rc = RUM_FAILURE;   
  }
#else
  if ( config.Protocol != RUM_TCP )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3382), "%s%d%d%s",
        "The RUM transmitter configuration parameter {0} is not valid.  "
        "The requested value is {1}. The valid value is {2}({3}).",
        "Protocol", config.Protocol, RUM_TCP, "RUM_TCP");
    rc = RMM_FAILURE;
  }
#endif
  if (tbh != NULL) llmAddTraceData(tbh, "%d", "Protocol={0}, ", config.Protocol);

  if ( config.IPVersion != RUM_IPver_IPv4 && config.IPVersion != RUM_IPver_IPv6 &&
      config.IPVersion != RUM_IPver_ANY && config.IPVersion != RUM_IPver_BOTH )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3383), "%s%d%d%s%d%s%d%s%d%s",
        "The RUM transmitter configuration parameter {0} is not valid.  "
        "The requested value is {1}. "
        "The valid values are {2}({3}) or {4}({5}) or {6}({7}) or {8}({9}).",
        "IPVersion", config.IPVersion, RUM_IPver_IPv4, "RUM_IPver_IPv4",
        RUM_IPver_IPv6, "RUM_IPver_IPv6", RUM_IPver_ANY, "RUM_IPver_ANY",
        RUM_IPver_BOTH, "RUM_IPver_BOTH");
    rc = RMM_FAILURE;
  }

  if(tbh != NULL) llmAddTraceData(tbh,"%d","IPVersion={0}, ", config.IPVersion);

  if ( config.ServerSocketPort < 0 || config.ServerSocketPort > 0xffff)
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3385),"%s%d%d%d",
        "The RUM transmitter configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
        "ServerSocketPort",config.ServerSocketPort,0,0xffff);
    rc = RMM_FAILURE;
  }
  if(tbh != NULL) llmAddTraceData(tbh,"%d","ServerSocketPort={0}, ", config.ServerSocketPort);

  if ( config.LogLevel < RUM_LOGLEV_NONE || config.LogLevel > RUM_LOGLEV_XTRACE )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3386),"%s%d%d%d",
        "The RUM transmitter configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
        "LogLevel",config.LogLevel,RUM_LOGLEV_NONE,RUM_LOGLEV_XTRACE);
    rc = RMM_FAILURE;
  }
  if(tbh != NULL) llmAddTraceData(tbh,"%d","LogLevel={0}, ", config.LogLevel);

  if ( config.LimitTransRate != RUM_DISABLED_RATE_LIMIT && config.LimitTransRate != RUM_STATIC_RATE_LIMIT && config.LimitTransRate != RUM_DYNAMIC_RATE_LIMIT)
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3387),"%s%d%d%s%d%s%d%s",
        "The RUM transmitter configuration parameter {0} is not valid. The requested value is {1}. The valid values are {2}({3}), {4}({5}), or {6}({7}).",
        "LimitTransRate",config.LimitTransRate,RUM_DISABLED_RATE_LIMIT,"Disabled",RUM_STATIC_RATE_LIMIT,"Static",RUM_DYNAMIC_RATE_LIMIT,"Dynamic");
    rc = RMM_FAILURE;
  }
  if(tbh != NULL) llmAddTraceData(tbh,"%d","LimitTransRate={0}, ", config.LimitTransRate);

  if ( config.LimitTransRate != RUM_DISABLED_RATE_LIMIT && config.TransRateLimitKbps < 8 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3388),"%s%d%d%d",
        "The RUM transmitter configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
        "TransRateLimitKbps",config.TransRateLimitKbps,8,0x7FFFFFFF);
    rc = RMM_FAILURE;
  }
  if(tbh != NULL) llmAddTraceData(tbh,"%d","TransRateLimitKbps={0}, ", config.TransRateLimitKbps);

  if ( config.PacketSizeBytes < MIN_PACKET_SIZE || config.PacketSizeBytes > MAX_PACKET_SIZE )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3389),"%s%d%d%d",
        "The RUM transmitter configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
        "PacketSizeBytes",config.PacketSizeBytes,MIN_PACKET_SIZE,MAX_PACKET_SIZE);
    rc = RMM_FAILURE;
  }
  if(tbh != NULL) llmAddTraceData(tbh,"%d","PacketSizeBytes={0}, ", config.PacketSizeBytes);

  if ( config.TxMaxMemoryAllowedMBytes < 0 ||
      config.MinimalHistoryMBytes < 0 ||
      config.TxMaxMemoryAllowedMBytes < 12*(config.MinimalHistoryMBytes + rmmTRec[inst]->T_advance.MaxPendingQueueKBytes/1024)/10 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3390),"%s%d%s%d%d",
        "The RUM transmitter configuration parameter {0} must be larger than 1.2*({2}+MaxPendingQueueKBytes/1024) which has a value of ({4}). "
        "The requested value is {1}. "
        "Additionally, the configuration parameter {2} must be greater than 0. "
        "The requested value is {3}.",
        "TxMaxMemoryAllowedMBytes", config.TxMaxMemoryAllowedMBytes,
        "MinimalHistoryMBytes", config.MinimalHistoryMBytes,
        (12*(config.MinimalHistoryMBytes + rmmTRec[inst]->T_advance.MaxPendingQueueKBytes/1024)/10));
    rc = RMM_FAILURE;
  }
  if(tbh != NULL) llmAddTraceData(tbh,"%d","TxMaxMemoryAllowedMBytes={0}, ", config.TxMaxMemoryAllowedMBytes);
  if(tbh != NULL) llmAddTraceData(tbh,"%d","MinimalHistoryMBytes={0}, ", config.MinimalHistoryMBytes);


  if ( config.SocketSendBufferSizeKBytes < 0 )
    config.SocketSendBufferSizeKBytes = 0;
  if(tbh != NULL) llmAddTraceData(tbh,"%d","SocketSendBufferSizeKBytes={0}, ", config.SocketSendBufferSizeKBytes);

  if(tbh != NULL) llmAddTraceData(tbh,"%s","TxNetworkInterface = {0}], ", config.TxNetworkInterface);
  if ( strcmp(upper(config.TxNetworkInterface), "NONE") == 0 )
    upper(rmmTRec[inst]->T_config.TxNetworkInterface);


  /* check advanced configuration parameters */
  if(tbh != NULL) llmAddTraceData(tbh,"","Advanced: [");
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "MaxPendingQueueKBytes = {0}, ", adv_config->MaxPendingQueueKBytes);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "MaxStreamsPerTransmitter = {0}, ", adv_config->MaxStreamsPerTransmitter);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "MaxFireOutThreads = {0}, ", adv_config->MaxFireOutThreads);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "StreamsPerFireout = {0}, ", adv_config->StreamsPerFireout);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "FireoutPollMicro = {0}, ", adv_config->FireoutPollMicro);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "PacketsPerRound = {0}, ", adv_config->PacketsPerRound);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "PacketsPerRoundWhenCleaning = {0}, ", adv_config->PacketsPerRoundWhenCleaning);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "RdataSendPercent = {0}, ", adv_config->RdataSendPercent);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "CleaningMarkPercent = {0}, ", adv_config->CleaningMarkPercent);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "MinTrimSize = {0}, ", adv_config->MinTrimSize);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "MaintenenceLoop = {0}, ", adv_config->MaintenanceLoop);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "HeartbeatTimeoutMilli = {0}, ", adv_config->HeartbeatTimeoutMilli);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "SnapshotCycleMilli_tx = {0}, ", adv_config->SnapshotCycleMilli_tx);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "PrintStreamInfo_tx = {0}, ", adv_config->PrintStreamInfo_tx);
  if(tbh != NULL) llmAddTraceData(tbh, "%d%d%d", "AdaptivePacketRate = {0} ({1} {2}), ", adv_config->BatchingMode,adv_config->MinBatchingMicro,adv_config->MaxBatchingMicro);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "StreamReportIntervalMilli = {0}, ", adv_config->StreamReportIntervalMilli);
  if(tbh != NULL) llmAddTraceData(tbh, "%d", "DefaultLogLevel = {0}, ", adv_config->DefaultLogLevel);
  if(tbh != NULL) llmAddTraceData(tbh, "%s", "LogFile = {0}]", adv_config->LogFile);

  if ( rc != RMM_FAILURE)
  {
    if(tbh != NULL) llmCompositeTraceInvoke(tbh);
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5391),"%s%d%d%s",
        "RUM Transmitter Version {0}({1}): Instance {2} is running on {3}",
        RMM_API_VERSION, PGM_VERSION, inst, RMM_OS_TYPE);
  }else{
    if(tbh != NULL) llmReleaseTraceBuffer(tbh);
  }

  return rc;
}



/*****************************************************************************************/


