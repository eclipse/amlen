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

#ifndef  H_rumPrivate_H
#define  H_rumPrivate_H

#define USE_EA 1

/* macros */

#define  milli2nano(x)          (1000000*(x))
#define  nano2milli(x)          ((x)/1000000)
#define  CONNCTION_STREAMS_ARRAY_SIZE  128

#define RMM_EVENT_ID  111
#define RUM_EVENT_ID  111
#define LOG_EVENT_ID  222
#define CON_EVENT_ID  333
#define FCB_EVENT_ID  555
#define ONE_PRM_SIZE  512
#define TOT_PRM_SIZE 8192
#define MAX_NUM_PRMS    8

#define T_INVALID  0x01
#define R_INVALID  0x02
#define P_INVALID  0x04
#define C_INVALID  0x08
#define A_INVALID  0x10

#if USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>


typedef struct
{
  struct ssl_ctx_st *sslCtx[2] ; // i = init_here 
} sslGlobInfo_t ; 

typedef struct
{
  struct ssl_ctx_st *sslCtx ; 
  SSL               *ssl ; 
  BIO               *bio ; 
  char              *func ;
  pthread_mutex_t    lock[1] ; 
} sslConnInfo_t ; 
#endif


typedef struct
{
  rumConnectionEvent ev ; 
  rum_on_connection_event_t on_event ; 
  void * user ; 
  char  *prm[MAX_NUM_PRMS] ; 
  double prms[TOT_PRM_SIZE/8] ; 
} ConEventInfo ; 

typedef struct
{
  rumEvent ev ; 
  rum_on_event_t on_event ; 
  void * user ; 
  char  *prm[MAX_NUM_PRMS] ; 
  double prms[TOT_PRM_SIZE/8] ; 
} RumEventInfo ; 

typedef struct
{
  rumLogEvent ev ; 
  rum_on_log_event_t on_event ; 
  void *user ; 
  char  *prm[MAX_NUM_PRMS] ; 
  double prms[TOT_PRM_SIZE/8] ; 
} LogEventInfo ; 

typedef struct
{
  rum_free_callback_t free_callback ; 
  void *user ; 
} FcbEventInfo ; 

typedef struct
{
  int evType ;
  union
  {
    RumEventInfo rumEv ; 
    LogEventInfo logEv ; 
    ConEventInfo conEv ; 
    FcbEventInfo fcbEv ; 
  } u ; 
} EventInfo ; 

/* data structures */


/****************************************************/
/*   GENERAL                                        */
/****************************************************/


typedef struct   /* private Basic RUM configuration parameters */
{
        int    Protocol ;                      /* RUM_TCP                                    */
        int    IPVersion ;                     /* RUM_IPver_[IPv4,IPv6, ANY, BOTH]           */
        int    ServerSocketPort ;
        int    LogLevel ;                      /* RUM_[NO, BASIC, MAXIMAL]_LOG               */
        int    LimitTransRate ;                /* RUM_[DISABLED, STATIC, DYNAMIC]_RATE_LIMIT */
        int    TransRateLimitKbps ; 
        int    PacketSizeBytes ;
        int    TransportDirection ;            /* RUM_[Tx_Rx, Tx_ONLY, Rx_ONLY]         */       
        int    MaxMemoryAllowedMBytes ;
        int    RxMaxMemoryAllowedMBytes ;
        int    TxMaxMemoryAllowedMBytes ;
        int    MinimalHistoryMBytes ;
        int    SocketReceiveBufferSizeKBytes ;
        int    SocketSendBufferSizeKBytes ;
        char   RxNetworkInterface[RUM_ADDR_STR_LEN] ;        /* "None" or a valid IP address          */
        char   TxNetworkInterface[RUM_ADDR_STR_LEN] ;        /* "None" or a valid IP address          */
        char   AdvanceConfigFile[RUM_FILE_NAME_LEN] ;       /* "None" or a valid file name           */ 
}  rumBasicConfig ;



typedef struct   /* Advanced Transmitter configuration parameters */
{
  int         MaxPendingQueueKBytes ;
  int         MaxStreamsPerTransmitter ;
  int         MaxFireOutThreads ;         /* FireOut Thread parameters */
  int         StreamsPerFireout ;
  int         FireoutPollMicro ;
  int         PacketsPerRound ;
  int         PacketsPerRoundWhenCleaning ;
  int         RdataSendPercent ;
  int         CleaningMarkPercent ;
  int         MinTrimSize ;
  int         MaintenanceLoop ;
  int         HeartbeatTimeoutMilli ;
  int         InterHeartbeatAmbientMilli ;     /* SPM Thread parameters */
  int         InterHeartbeatMinMilli ;
  int         InterHeartbeatMaxMilli ;
  int         SnapshotCycleMilli_tx ;
  int         PrintStreamInfo_tx ;
  int         BatchingMode ;
  int         BatchYield ;
  int         BatchGlobal ;
  int         MinBatchingMicro;
  int         MaxBatchingMicro;
  int         StreamReportIntervalMilli;
  int         DefaultLogLevel ;
  uint8_t     opt_present ;
  uint8_t     opt_net_sig ;
  char        apiBatchMode[14] ;
  int         ReuseAddress ;
  int         UseNoMA ; 
  int         NumExthreads ;
  int         NumMAthreads ;
  int         ThreadPerTopic ;
  int         DataPacketSize ;
  int         recvBuffsQsize ;
  int         nackBuffsQsize ;
  int         nackQsize      ;
  int         recvQsize      ;
  int         rsrvQsize      ;
  int         fragQsize      ;
  int         evntQsize      ;
  int         packQsize      ;
  int         BaseWaitMili ;
  int         LongWaitMili ;
  int         BaseWaitNano ;
  int         LongWaitNano ;
  int         SnapshotCycleMilli_rx ; 
  int         PrintStreamInfo_rx ;
  int         MemoryCrisisCycle ;
  int         MemoryAlertPctHi ;
  int         MemoryAlertPctLo ;
  int         TaskTimerCycle ;
  int         NackGenerCycle ;
  int         NackTimeoutBOF ;
  int         NackTimeoutNCF ;
  int         NackTimeoutData;
  int         NackRetriesNCF ;
  int         NackRetriesData;
  int         BindRetryTime  ; 
  int         MaxNacksPerCycle ;
  int         MaxSqnPerNack ;
  int         LogLevel ;
  char        LogFile[RUM_FILE_NAME_LEN] ;
  int         ReceiverConnectionEstablishTimeoutMilli ;
  int         RecvPacketNpoll ;
  int         ThreadPriority_tx ; 
  int         ThreadPriority_rx ; 
  rmm_uint64  ThreadAffinity_tx ; 
  rmm_uint64  ThreadAffinity_rx ; 
  int         ThreadStackSize   ; 
  int         LatencyMonSampleRate ;
  int         MonSampleRate ;
  int         UsePerConnInQ;
}  rumAdvanceConfig;



typedef struct 
{
  uint32_t    buflen ; 
  uint32_t    reqlen ; 
  uint32_t    offset ; 
  int         need_free;
  char       *bptr ; 
  char       *buffer ; 
} ioInfo ; 

typedef struct 
{
  int                       uid ;
  int                       is_valid ; 
  int                       n_active ; 
  int                       n_cip ;
  rum_on_connection_event_t on_event ; 
  void                     *user ; 
} ConnListenerInfo ;

typedef struct 
{
  void **buffs ; 
  int    n ; 
  int    size ; 
} TempPool ; 

typedef struct ConnInfo_
{
  rumConnectionID_t   connection_id ;
  double              ccp_header[8] ;  /* up to 48+4 bytes are needed  */
  char                tx_str_rep_packet[64+CONNCTION_STREAMS_ARRAY_SIZE*8] ;
  char                rx_str_rep_packet[64+CONNCTION_STREAMS_ARRAY_SIZE*8] ;
  char                rx_str_rep_rx_packet[64+CONNCTION_STREAMS_ARRAY_SIZE*8] ;
  char                rx_str_rep_tx_packet[64+CONNCTION_STREAMS_ARRAY_SIZE*8] ;
  void               *tx_streams[CONNCTION_STREAMS_ARRAY_SIZE] ; /* StreamInfoRec_T */
  int                 n_tx_streams ; 
  int                 last_stream_index ; 
  int                 ccp_header_len ;
  int                 use_ib ; 
  int                 use_shm; 
  int                 use_ssl; 
  pthread_mutex_t     mutex ; 
  int                 str_rep_rx_pending ; 
  int                 str_rep_tx_pending ; 
  uint32_t            ccp_sqn ;
  uint32_t            inQ[2] ;
  int                 hold_it ; 
  int                 init_here ; 
  char                req_addr[RUM_HOSTNAME_STR_LEN];
  int                 req_port ; 
  int                 method ; 
  int                 msg_len ; 
  char               *msg_buf ; 
  ConnListenerInfo   *conn_listener ; 
  int                 cip_to ; 
  int                 hb_to ; 
  int                 hb_skip ; 
  int                 hb_interval ; 
  int                 one_way_hb ; 
  int                 to_detected ; 
  rum_uint64          last_r_time ; 
  rum_uint64          last_t_time ; 
  rum_uint64          next_tx_str_rep_time ; 
  rum_uint64          next_rx_str_rep_time ; 
  rum_uint64          nBytes ; 
  rum_uint64          nCalls ; 
  unsigned long       nReads ; 
  unsigned long       nPacks ; 
  SOCKET              sfd ; 
  int                 sock_af ; 
  int                 ind ; 
  SAS                 rmt_sas ; 
  SAS                 lcl_sas ; 
  SA                 *rmt_sa  ; 
  SA                 *lcl_sa  ; 
  ipFlat              rmt_addr ; 
  ipFlat              lcl_addr ; 
  int                 rmt_port ; 
  int                 lcl_port ; 
  ioInfo              rdInfo ; 
  ioInfo              wrInfo ; 
  int                 io_state ; 
  int                 state ; 
  int                 is_invalid ; 
  int                 ppIn ; 
  int                 iPoll; 
  int                 ev_sent ; 
  rumConnection       apiInfo ; 
  BuffBoxRec         *sendNacksQ ; 
  TempPool           *tempPool ; 
  struct ConnInfo_   *next ; 
  struct ConnInfo_   *ll_next ; 
  #if USE_SSL
   sslConnInfo_t      sslInfo[1];
  #endif
  void               *gInfo;   /* rumInstanceRec */
}ConnInfoRec ;

typedef struct
{
  uint32_t ip ; 
  uint16_t port ; 
  uint16_t count ; 
} ConnIdRec ; 

#define EV_MSG_SIZE 128

typedef struct
{
  int          nConnsInProg ; 
  int          cidInit;
  ConnInfoRec *firstConnInProg ;
  ConnIdRec    ConnId ; 
 #if USE_POLL
  struct pollfd *fds ; 
  nfds_t        nfds ; 
  int           lfds ; 
 #else
  fd_set       rfds , wfds , efds ; 
  SOCKET       np1 ; 
 #endif
  char         ev_msg[EV_MSG_SIZE] , *ev_dbg[2] ; 
} cipInfoRec ; 
  

/* RUM instance structure. */

typedef struct {
  
  int                   instance;
  rumInstance           rum_instance;
  int                   instance_r;
  void                  *rInfo;   /* rmmReceiverRec */
  int                   instance_t;
  void                  *tInfo;   /* rmmTransmitterRec */

  int                   initialized ;
  
  rumConfig             apiConfig ;     /* API config params                         */
  rumBasicConfig        basicConfig ;   /* instance config params (private)          */
  rumAdvanceConfig      advanceConfig ; /* instance advanced config params (private) */
 
  rmm_uint64            CurrentTime ;   /* internal clock                            */
  pthread_mutex_t       ClockMutex ; 

  uint32_t              gsi_high ;      /* in nbo - set by the transmitter           */
  uint16_t              port ;          /* in nbo - set to the ServerSocketPort      */
  ipFlat                addr ; 

  ifInfo                RxlocalIf ;
  ifInfo                TxlocalIf ;
  int                   rxRunning ;
  int                   txRunning ;
  FILE                 *fp_log ;
  void                 *log_user ; 
  void                 *alert_user ; 
  rum_on_log_event_t    on_log_event ; 
  rum_on_event_t        on_alert_event ; 
  rum_free_callback_t   free_callback ; 

  cipInfoRec            cipInfo ; 
 #if USE_POLL
  struct pollfd        *rfds ; 
  nfds_t               nrfds ; 
  int                  lrfds ; 
  struct pollfd        *wfds ; 
  nfds_t               nwfds ; 
  int                  lwfds ; 
 #endif

  int                   nextCL ;
  int                  nConnectionListeners ;
  ConnListenerInfo     *ConnectionListeners[RUM_MAX_CONNECTION_LISTENERS] ; 
  pthread_mutex_t       ConnectionListenersMutex ; 

  int                  nConnections ; 
  ConnInfoRec          *firstConnection ; 
  LinkedListRec        *connReqQ ; 
  BuffBoxRec           *recvNacksQ ; 
  MemManRec            *nackBuffsQ ;
  int                   connReqQsize ; 
  int                   connUpd;
  rmm_rwlock_t          ConnSyncRW ; 
  int                   ibLoaded ; 
  int                   use_ssl; 
  char                  *instanceName;
  TCHandle              tcHandles[3];
  #if USE_SSL
   sslGlobInfo_t            sslInfo[1];
  #endif
} rumInstanceRec ;

static int  PutRumEvent(rumInstanceRec *rumInfo, rumEvent *ev, rum_on_event_t on_event, void *user) ; 
static void PutConEvent(rumInstanceRec *rumInfo, rumConnectionEvent *ev, rum_on_connection_event_t on_event, void *user) ; 
static void PutFcbEvent(rumInstanceRec *rumInfo, rum_free_callback_t free_callback, void *user) ; 
static int rmm_write(ConnInfoRec *cInfo);
static int rmm_read(ConnInfoRec *cInfo, char *buf, int len, int copy, int *errCode, char *errMsg);

#endif
