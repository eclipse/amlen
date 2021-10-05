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


#ifndef  H_rmmTprivate_H
#define  H_rmmTprivate_H

/* macros */

/* constants indicating (ack based) stream memory alert. must maintain this order! */
#define FO_STR_MEM_ALERT_OFF  0   /* must be zero! */
#define FO_STR_MEM_ALERT_LOW  1
#define FO_STR_MEM_ALERT_HIGH 2

#define MAX_FO_ATTN  64

#define  milli2nano(x)          (1000000*(x))
#define  nano2milli(x)          ((x)/1000000)

/* data structures */


/****************************************************/
/*   GENERAL                                        */
/****************************************************/



typedef struct   /* Advanced Transmitter configuration parameters */
{
       int    RecvSocketBufferSizeKbytes[3] ;
       int    SendSocketBufferSizeKbytes[3] ;
       int    MaxPendingQueueKBytes ;
       int    MaxStreamsPerTransmitter ;
       int    MaxFireOutThreads ;         /* FireOut Thread parameters */
       int    MaxFireOutQueues ;
       int    StreamsPerFireout ;
       int    FireoutPollMicro ;
       int    ThreadPriority ;
  rmm_uint64  ThreadAffinity ;
       int    PacketsPerRound ;
       int    PacketsPerRoundWhenCleaning ;
       int    RdataSendPercent ;
       int    CleaningMarkPercent ;
       int    MinTrimSize ;
       int    MaintenanceLoop ;
       int    InterHeartbeatAmbientMilli ;     /* SPM Thread parameters */
       int    InterHeartbeatMinMilli ;
       int    InterHeartbeatMaxMilli ;
       int    SnapshotCycleMilli_tx ;
       int    PrintStreamInfo_tx ;
       int    RepairAlertRateHigh ;
       int    RepairAlertRateLow ;
       int    FixedRepairPort ;
       int    BatchingMode ;
       int    MinBatchingMicro;
       int    MaxBatchingMicro;
       int    BatchYield;
       int    BatchGlobal ;
       int    LatencyMonSampleRate ;
       int    HistoryTrimmingIntervalMilli ;
       int    evntStrucQsize;
       int    evntQsize;
       int    DefaultLogLevel ;
       int    RdataRateLimit ;
       int    RepairPortRangeLow ;
       int    RepairPortRangeHigh ;
       int    RepairSuppressionMicro ; 
       int    SyncEventLevel ; 
       unsigned char opt_present ;
       unsigned char opt_net_sig ;
       char   apiBatchMode[14] ;
       char   LogFile[RMM_FILE_NAME_LEN] ;
       char   OsInformation[RMM_FILE_NAME_LEN] ;
       char   StreamIdManagementPath[RMM_FILE_NAME_LEN] ;
       int    LBThreadPollMicro ;
       int    LBThreadPriority ;
  rmm_uint64  LBThreadAffinity ;
       int    ThreadTerminationTimeoutMilli ; 

}  rmmTAdvanceConfig;

/* Toke Bucket structure. All values are in Kbit(/sec)               */
typedef struct _token_bucket
{
  int              size;
  int              tokens;
  int              sleep_time;     /* how long to sleep between updates        */
  pthread_mutex_t  mutex;
  pthread_t        updater;        /* a thread that updates the tokens         */
  pthread_cond_t   waiting;        /* conditional waiting for the token        */
  int              tokens_per_milli;
  int              tokens_per_milli_min;
  int              tokens_per_milli_max;
  int              status;
} token_bucket;

typedef struct
{
  int              status;
  pthread_t        thread_id;
  uint32_t         loops;
  int              tPos;
} ThreadStatus ;

typedef struct
{
  int trail;
  int lead;
  int rdata_sqn;
} MonitorChecks ;

/****************************************************/
/*   TRANSMITTER                                    */
/****************************************************/

/* The following structures are kept per-stream.                     */

typedef struct
{
  rmm_uint64  bytes_transmitted ;       
  rmm_uint64  last_bytes_transmitted ;
  rmm_uint64  bytes_retransmitted ;     
  rmm_uint64  messages_sent ;
  rmm_uint64  packets_sent ;            

  rmm_uint64  reset_messages_sent ;
  rmm_uint64  reset_bytes_transmitted ;
  rmm_uint64  reset_packets_sent ;    
  rmm_uint64  reset_bytes_retransmitted ;     
  uint32_t    reset_rdata_packets ;

  uint32_t  ncfs_sent ;
  uint32_t  naks_received ;             
  uint32_t  reset_naks_received ;             
  uint32_t  acks_received ;             
  uint32_t  rdata_packets ;             
  uint32_t  naks_filtered ;
  uint32_t  last_nak_count;
  uint32_t  last_txw_lead ;
  uint32_t  partial_packets ;
  uint32_t  partial_2fast ;
  uint32_t  partial_trylock ;
  int       packets_per_sec ;
  int       rate_kbps ;
} stream_stats_t ;

/* forward declaration */
typedef struct StreamInfoRec_T StreamInfoRec_T ; 

typedef struct
{
  double         submit_time ;
  rmm_uint64     history_time ;
  void          *ll_next ; 
} rmmTxPacketHeader ;


/* Data structure for a single stream at the Transmitter             */
struct StreamInfoRec_T
{
  int                      inst_id ;         /* Transmitter instance ID                   */
  signed char              reliable ;
  signed char              reliability ;
  signed char              new_reliability ;
  char                     send_msn ;
  char                     is_failover ;
  char                     is_backup ;
  char                     no_partial ;
  char                     dont_batch ; 
  char                     must_batch ; 
  char                     app_controlled_batch ; /* means no partial           */
  char                     LimitTransRate ;
  char                     direct_send ;
  char                     active ;          /* indicate if stream is about to be closed  */
  char                     closed ;
  char                     has_ordering_info;
  char                     rcms_is_control;
  char                     enable_msg_properties;
  char                     mtl_version ;
  char                     has_htm ;
  char                     disable_snapshot ;
  char                     tcp_blocked ; 
  char                     has_bond ; 
                         
  rmm_uint64               create_time ;
  rmm_uint64               close_time ;
  rmm_uint64               remove_time ;
  int                      release_thread ;
                         
  int                      keepHistory ;
  rumConnectionID_t        connection_id ;
  int                      conn_invalid ;    /* indicates that no connection has reference to the stream */
  rmm_uint64               conn_invalid_time ;
  int                      FO_in_use ;
  int                      SCP_tries ;
  char                     topic_name[RUM_MAX_QUEUE_NAME+8] ;
  rumQueueT                rmmT ;            /* fields exposed to the API                 */
  rumStreamID_t            stream_id ;
  /* pt2onRumEvent            on_event ;  void                 *event_user ;  */
  void                    *lb_parent_info ;
  void                    *lb_child_info ;
  int                      is_lb_child ;
  int                      FO_attn ;
                         
  char                     msg2pac_option[(MSG2PAC_OPT_SIZE+7)/8*8] ;
  int                      msg2pac_offset_spm ;
  int                      msg2pac_interval ;
                         
  char                     stream_id_str[24] ;
  char                     conn_id_str[24] ;
  pgm_seq                  txw_trail ;
  pgm_seq                  txw_lead ;
  pgm_seq                  spm_seq_num ;
  pgm_seq                  txw_trail_nbo ;
  int                      spm_size ;
                         
  uint16_t                 src_port ;        /* nbo */
  uint16_t                 dest_port ;       /* nbo */
  uint32_t                 gsi_high ;        /* nbo */
  uint16_t                 gsi_low ;         /* nbo */
                         
  uint16_t                 ip_header_sqn ; 
                         
  char                     is_late_join ;
  pgm_seq                  late_join_mark ;
  int                      late_join_offset_odata ;
  int                      late_join_offset_spm ;
                         
                         
  char                     pgm_header[PGM_HEADER_LENGTH] ;
  uint16_t                 Odata_options_size ;
  char                    *Odata_options ;
  char                     Odata_options_changed ;
  int                      sqn_offset ;
                         
  rmm_uint64               next_send_time ;
  rmm_uint64               last_send_time ;
  char                     spm_pending ;    /* indicates FO an spm packet is ready        */
  char                     spm_fo_generate; /* indicates FO to generate and send an spm   */
  char                     request_ack ;
  char                     request_spm ;
  unsigned char            ack_drop_prob ;
  rmm_uint64               last_spm_time ;
  rmm_uint64               last_ack_time ;
  rmm_uint64               last_hbt_time ;
  rmm_uint64               last_hbt_time_ev ;
  pgm_seq                  last_ack_sqn ;
  pgm_seq                  last_ack_to_ev;
  signed char              handshake_mode ; /* handshke=0, active=1, reject=-1            */
  char                     handshake_hbto_delivered ;
  char                     ack_alert_on ;
  signed char              mem_alert ;
  char                     wait1_ack_updated ;
                         
  int                      Spm_on ;         /* threads indicate activity on the stream    */
  int                      Repair_on ;      /* to allow safe topic close                  */
  int                      Monitor_on ;
  int                      FireOut_on ;
                         
  stream_stats_t           stats ;          /* stream statistics                          */
                         
  pgm_seq                  mtl_lead_sqn ;   /* sqn assigned in mtl before packet is send  */
  int                      mtl_buffsize ;
  char                    *mtl_buff ;       /* MTL buffer used for batching messages      */
  int                      mtl_buff_init ;
  int                      mtl_currsize ;
  int                      mtl_offset ;     /* start of MTL in packets = PGM+PTL headers  */
  int                      mtl_messages ;   /* number of messages in mtl buffer           */
  int                      free_bytes_in_packet ; 
                         
  pthread_mutex_t          zero_delay_mutex ; /* to allow sending partial packets         */
  pthread_mutex_t          spm_mutex ;
  pthread_mutex_t          mbu_mutex ;
  pthread_mutex_t          rdata_mutex ;      /* protect Rdata_Q when cleaning history    */
  pthread_mutex_t          fireout_mutex ;    /* protect stream when FireOut is active    */
  pthread_mutex_t          repair_mutex ;
  int                      mutex_init ;
  SockInfo                *DirectSocket ;
  char                    *Spm_P ;           /* only one SPM packet is used. no spm queue */
  char                     NcfPacket[CONTROL_PACKET_BUF_SIZE] ;
  int                      Nak_header_length ;
  LinkedListRec           *Ncf_Q ;
  LinkedListRec           *Rdata_Q ;
  LinkedListRec           *Odata_Q ;
  BuffBoxRec              *History_Q ;
                         
  MonitorChecks            monitor_needed ;
                         
  double                   batching_time ;
  double                   batching_stamp ;
                         
  rmm_uint64               msg_sqn ;
  rmm_uint64               msg_packet_trail ;
  rmm_uint64               msg_packet_lead ;
  rmm_uint64               last_set_msg_sqn ;
  int                      msg_opt_offset_spm ;      /* offset of msg option in SPM           */
  int                      req_ack_offset_spm;      /* offset of unicast control option in spm */
  int                      msg_opt_offset_data ;     /* offset of msg option in packet        */
  int                      msg_odata_optins_offset;  /* offset of msg option in odata_options */
                         
  ipFlat                   source_nla ;      /* nbo */
  ipFlat                   mcast_nla ;       /* nbo */
  short                    source_nla_afi ;
  short                    mcast_nla_afi ;
  char                     dest_addr_str[RMM_ADDR_STR_LEN] ;
  int                      socket_index ;
  int                      include_src_option;
                         
  char                     is_unicast ;
  int                      transport ;                     /* RMM_TRANSPORT_MULTICAST / RMM_TRANSPORT_UDP_UNICAST    */
  int                      feedback_mode ;                 /* RMM_FEEDBACK_ACK / RMM_FEEDBACK_NAK           */
  int                      heartbeat_timeout_milli ;
  int                      heartbeat_interval_min ;
  int                      heartbeat_interval_max ;
  int                      spm_backoff ;
  int                      ack_timeout_milli ;
  int                      ack_interval_milli ;
  int                      ack_interval_packets ;
  uint32_t                 max_unacked_packets ;
  int                      ack_probe_milli ;
  int                      fast_ack_notification ; 
  int                      fast_ack_offset ; 
  int                      handshake_timeout_milli ;
  int                      max_ack_retries ;
  int                      ack_retries ;
  pgm_seq                  ack_retries_sqn ;
  int                      history_size_packets ;
  int                      history_time_milli ;
  int                      history_cleaning_policy ;
  rmm_uint64               next_trim_time ;
  double                   next_ncf_time ; 
  int                      dest_data_port ;                /* if value <=0 will use DataPort specified in configuration   */
                         
  void                    *event_user;
  rum_on_event_t           on_event ;
} ;


typedef struct
{
  double    etime ; 
  uint16_t  active ; 
  uint16_t  value ; 
  char      rsrv[4] ; 
} SrcPortInfo ; 

typedef struct
{
  double        ntime[2] ; 
  SrcPortInfo  *ids ; 
  FILE         *fp ; 
  TCHandle      tcHandle;
  char          vals[0x10000] ; 
  int           fd ; 
  int           size[2] ; 
} UniqueIdInfo ; 


/* Global transmitter structure.
Streams are stored in an array where the array index is the stream's counter.  */

typedef struct {

  rumInstanceRec       *rumInfo ; /* RUM instance */
  rumBasicConfig        T_config ;
  rumAdvanceConfig      T_advance ;
  int                   keepHistory ;
  pthread_mutex_t       Fireout_mutex ;    /* protect connection stream list  */

  char                  global_id_str[24] ;
  int                   initialized ;
  int                   number_of_streams ;
  uint16_t              max_stream_index ;  /* highest index used in array all_streams */
  uint16_t              counter ;           /* to assign counter for stream IDs        */
  uint16_t              streamID_counter ;  /* to assign stream IDs                    */
  StreamInfoRec_T      *all_streams[RMM_MAX_TX_TOPICS] ;
  StreamInfoRec_T      *closed_streams[RMM_MAX_TX_TOPICS] ;

  rmm_uint64            CurrentTime ;     /* internal clock                            */

  int                   protos ; 
  int                   num_interfaces ;      /* there are at most 8 interfaces ( 1 prim + 7 sup) */
  ifInfo                localIf[1] ;
  int                   source_nla_afi[2] ;
  ipFlat                source_nla[2] ;
  int                   source_nla_is_global[1] ;
  uint32_t              gsi_high ;          /* nbo */
  uint16_t              port ;              /* nbo */

  char                  addr4_string[RMM_ADDR_STR_LEN];
  char                  addr6_string[RMM_ADDR_STR_LEN];
  char                  host_info[RMM_FILE_NAME_LEN];
  ThreadStatus          FireOutStatus ;     /* indicates status of threads */
  ThreadStatus          RepairStatus ;
  ThreadStatus          TcpStatus ;
  ThreadStatus          SpmStatus ;
  ThreadStatus          MonitorStatus ;
  ThreadStatus          EAStatus ;

  TimerTasks            tTasks ;

  uint32_t              DataPacketSize ;
  uint32_t              MaxPacketsAllowed ;
  uint32_t              MinHistoryPackets ;
  uint32_t              MinHistoryPackets0;
  uint32_t              MaxPendingPackets ;
  uint32_t              MaxPendingStreamPackets ;
  uint32_t              sqn_offset ;
  uint32_t              packet_header_offset ;


  MemManRec            *DataBuffPool ;
  MemManRec            *CtrlBuffPool ;
  int                   FO_Thrds[FIREOUT_ARRAY_SIZE];
  CondWaitRec           FO_CondWait ;
  CondWaitRec           FO_CondSleep ;
  double                FO_wakeupTime;
  double                min_batching_time ;
  double                max_batching_time ;
  double                batching_stamp ;
  double                rdata_supp_sec ; 
  int                   max_batching_packets ;
  token_bucket         *global_token_bucket ;
  int                   bucket_rate;

  SockInfo             *UdpSocket ;
  SockInfo             *TcpSocket ;
  SockInfo             *NakSocket ;
  SockInfo             *DirectSocket ;
  SockInfo             *SocketHead ; 
  int                   MemoryAlert ;
  int                   partial_pending ;
  pthread_mutex_t       MemoryAlert_mutex ;
  pthread_mutex_t       Gprps_mutex ;

  FILE                 *fp_log ;
  void                 *log_user ;
  rum_on_log_event_t    on_log ;
  rum_on_event_t        tx_on_event ;                             /**< callback to process events                 */
  void                 *event_user ;               /**< user to associate with on_event callbacks  */

  int                   packet_rate ;
  int                   rate_kbps ;
  unsigned int          history_clean_mark ;
  unsigned int          maximal_packet_rate ;
  unsigned int          maximal_rate_kbps ;
  unsigned int          naks_received ;
  unsigned int          acks_received ;
  unsigned int          repair_packets ;
  rmm_uint64            total_repair_packets ;
  rmm_uint64            total_repair_packets_closed ;
  rmm_uint64            packets_sent ;
  rmm_uint64            total_packets_sent ;
  rmm_uint64            total_packets_sent_closed ;
  rmm_uint64            msgs_sent ;
  rmm_uint64            total_msgs_sent ;        /* keeps count of closed streams + count of msgs_sent at last reset */
  rmm_uint64            total_msgs_sent_closed ;        /* keeps count of closed streams + count of msgs_sent at last reset */
  rmm_uint64            bytes_sent ;
  rmm_uint64            total_bytes_sent ;
  rmm_uint64            total_bytes_sent_closed ;
  rmm_uint64            last_fo_time ;
  char                  repair_alert_on ;

  rmm_get_external_time_t   extTime ;
  void *                    extTime_param ;

  char                  mon_slow_rec ;

  char*                 instanceName;
  TCHandle              tcHandle;
  uint32_t              total_history_size_packets ;
  uint16_t              next_dest_id ;
} rmmTransmitterRec ;


#endif
