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


#ifndef  H_rmmRprivate_H
#define  H_rmmRprivate_H

/*   Data Structures   */


#define HASH_SIZE 65536
#define HASH_MASK 65535

#define MAX_WHY_BAD 100
#define LEN_WHY_BAD  64
typedef struct
{
       int num_why_bad_msgs   ;
       int  last_why_bad   ;
       int  why_bad     [MAX_WHY_BAD] ;
       int  how_bad_isit[MAX_WHY_BAD] ;
       char why_bad_msgs[MAX_WHY_BAD][LEN_WHY_BAD] ;
} WhyBadRec ; 

typedef struct
{
  int type ;
  RMM_LOGLEV_t severity ;
  int nargs ;
  char args[4][LOG_MSG_SIZE] ;
  char msg[LOG_MSG_SIZE] ;
} rmmLogMsgRec ;

typedef struct 
{
   int              goDN ;
   int              prUP ;
   int              ppUP ;
   int              ngUP ;
   int              ntUP ;
   int              maUP ;
   int              eaUP ;
   int              chUP ;
   int              chEC ;
   pthread_mutex_t  mutex ;
   rmm_rwlock_t     rw ; 
}  GlobalSyncRec ;

typedef struct 
{
   int                  type ;
   rumStreamID_t        sid ;
   rumConnectionID_t    cid ; 
}  RejectedRec ;

typedef struct _streamInfo rStreamInfoRec ; /* forward declaration */
typedef struct _topicInfo rTopicInfoRec ; /* forward declaration */

typedef int (*pt2parseMsgs)(rTopicInfoRec *ptp, rStreamInfoRec *pst, int nMsgs, char *bptr, char *eop,rmmMessageID_t msg_cur, char *packet, rumPacket *rPack);

typedef struct _hsi_
{
   rmmStreamID_t         sid ;
   struct _hsi_         *hsi_next ;
   struct _hsi_         *bad_next ;
} HashStreamInfo ;

struct _streamInfo
{
   rumStreamID_t         sid ;
   HashStreamInfo       *hsi_next ;
   struct _streamInfo   *prev ;
   struct _streamInfo   *next ;
   struct _streamInfo   *ma_next ;
   ConnInfoRec          *cInfo ; 
   unsigned char         sid_chr[8] ;
   char                  sid_str[20] ;
   char                  sIP_str[RUM_ADDR_STR_LEN] ;
   int                   instance_id ;
   int                   topic_id ;
   int                   reliability ;
   int                   reliable ;
   int                   active   ;
   int                   killed   ;
   int                   ns_event ; 
   int                   keepHistory ; 
   int                   ChunkSize ;
   int                   inMaQ ;
   int                   maIn  ;
   int                   nfo ;
   int                   isTf ; 
   int                   isPr ;
   int                   ha_bw_req ; 
   rmm_uint64            TotMsgsOut, TotMsgsOut_;
   rmm_uint64            TotMsgsDel, TotMsgsDel_;
   rmm_uint64            TotBytesOut, TotBytesOut_;
   rmm_uint64            TotPacksIn,  TotPacksIn_;
   rmm_uint64            TotPacksOut, TotPacksOut_;
   rmm_uint64            TotPacksLost, TotPacksLost_;
   rmm_uint64            RdataPacks;
   rmm_uint64            DupPacks, DupPacks_ ;
   rmm_uint64            TotNacks ;
   rmm_uint64            TotVisits;
   rmm_uint64            TotMaSigs ;
   rmm_uint64            nak_stat[8] ;

   pgm_seq               rxw_trail ;
   pgm_seq               rxw_tail ;
   pgm_seq               rxw_lead ;
   pgm_seq               last_spm ;
   int                   have_lj ; 
   int                   have_st ; 
   int                   mtl_offset ;
   int                   msn_offset ;
   uli                   msn_trail ;
   uli                   msn_tail ;
   uli                   msn_lead ;
   rmm_uint64            ma_last_time ;
   rmm_uint64            ng_last_time ;
   rmm_uint64            pp_last_time ;
   rmm_uint64            close_time ;
   pgm_seq               ng_last;
   uint32_t              ng_last_tpk ;
   pgm_seq               ng_last_spm ;
   rmm_uint64            ng_last_spm_time ;
   int                   ng_timer_set ;
   int                   ng_num_dead ;
   uint32_t              ng_sum_toN ;
   uint32_t              ng_num_toN ;
   uint32_t              ng_sum_toD ;
   uint32_t              ng_num_toD ;
   int                   NackTimeoutNCF ;
   int                   NackTimeoutData;
   int                   num_glbs ; 
   pgm_seq               psn_glbs[MAX_GLB_2KEEP+1] ; 
   rmmMessageID_t        msn_glbs[MAX_GLB_2KEEP+1] ; 
   pgm_seq               psn_glb ; 
   rmmMessageID_t        msn_glb ; 
   rmmMessageID_t        msn_gob ; 

   uint16_t              src_port ;  /*  Note:                */
   uint16_t              dest_port;  /*    These              */
   uint16_t              gsi_low  ;  /*      are              */
   uint32_t              gsi_high ;  /*        kept in        */
   ipFlat                path_nla ;  /*          network      */
   ipFlat                 src_nla ;  /*          network      */
   ipFlat                mc_group ;  /*             byte      */
   SAS                   sa_nk_uni ; /*               order   */
   SAS                   sa_nk_mcg ; /*                       */
   void                 *sa_nk_ip  ; 
   socklen_t             sa_nk_len ; 

   SortedQRec           *dataQ ;
   SortedQRec           *nakSQ ;
   int                   nakQn ; 
   LinkedListRec        *uordQ ;
   LinkedListRec        *fragQ ;

   pt2parseMsgs          parseMsgs;
   rumStreamInfo         si ; 
   rumRxMessage          msg ;
   rumPacket            *pck ;
   rumEvent              ev ;
   void                 *om_user ;
   int                   topicLen ;
   char                  topicName[RUM_MAX_QUEUE_NAME] ; 
   uint32_t              nak_pac_opt_off ; 
   pthread_mutex_t       ppMutex  ;
};

typedef struct
{
   rmm_uint64           lag_time ; 
   rmm_uint64           chk_time ; 
   rmmMessageID_t       msn_next[3] ; 

}  GobackInfoRec ; 

typedef int (*pt2parseMTL)(rStreamInfoRec *pst, char *packet);

struct _topicInfo
{
   int                  instance_id ;
   int                  topic_id ;
   int                  reliability ;
   int                  reliable ;
   int                  ordered  ;
   int                  failover ;
   int                  isTf ; 
   int                  isPr     ;
   int                  byStream ;
   int                  closed ;
   rmmMessageID_t       msn_next; 
   rmm_uint64           nextSortTime ;
   rmm_uint64           TotMsgsLost, TotMsgsLost_;
   rmm_uint64           TotPacksIn ;
   rmm_uint64           TotPacksDrop, TotPacksDrop_;
   rmm_uint64           OldPacksDrop ;
   rmm_uint64           TotMsgsOut ;
   rmm_uint64           TotMsgsDel ;
   GobackInfoRec        goback ; 
   int                  maIn ;
   int                  maOut ;
   int                  odEach ;
   pthread_t            maThreadID ;
   pthread_mutex_t      maMutex  ;
   pthread_mutex_t      tfMutex  ;
   rum_on_message_t     on_message ;
   rum_on_packet_t      on_packet ;
   rum_on_data_t        on_data ;
   rum_on_event_t       on_event   ;
   rum_accept_stream_t  accept_stream ;
      pt2parseMTL       parseMTL;
   rumEvent             ev ;
   void                *om_user ;
   void                *ov_user ;
   void                *ss_user ;
   LinkedListRec       *packQ ;
   char                *BitMap ; 
   int                 *BitMapPos ; 
   int                  BitMapLen ; 
   int                  maxPackInQ ; 
   int                  maxTimeInQ ; 
   int                  need_trim ; 
   int                  stream_join_backtrack ;
   int                  topicLen ;
   char                 topicName[RUM_MAX_QUEUE_NAME] ; 
} ;

typedef struct 
{
   rumStreamID_t         sid ;
   rumConnectionID_t     cid ; 
   int                   reliability ;
   int                   keepHistory ; 
   int                   isTf ; 
   int                   isPr ; 
   int                   have_lj ; 
   pgm_seq               rxw_trail ;
   pgm_seq               rxw_lead ;
   pgm_seq               late_join ;
   ipFlat                src_nla ; 
   uint16_t              src_port ;
   int                   topicLen ;
   char                  topicName[RUM_MAX_QUEUE_NAME] ; 
} scpInfoRec ; 

typedef struct 
{
   int                  instance ;
   rumInstanceRec      *gInfo ; 
   int                  rNumTopics ;
   int                  rNumStreams ;
   rmm_uint64           TotPacks[256] ;
   rmm_uint64           TotPacksRecv[2] ;
   rmm_uint64           TotSelects;
   rmm_uint64           TotReads ;
   rmm_uint64           TotCalls ;
   rmm_uint64           TotPacksIn ;
   rmm_uint64           TotPacksOut ;
   rmm_uint64           PacksRateIn ;
   rmm_uint64           BytesRateIn ;
   rmm_uint64           FullPackQ ;
   rmm_uint64           EmptyPackQ ;
   rmm_uint64           TotBytesRecv[2]  ;
   rmm_uint64           TotMsgsOut ;
   rmm_uint64           CurrentTime ;
   rmm_uint64           NakAlertCount ;
   rmm_uint64           NextSlowAlert ;
   rmm_uint64           NextNakTime ;
   rmm_uint64           LastNakAlertCount ;
   rmm_uint64           MemoryCrisisTime ;
   int                  MemoryCrisisMode ;
   int                  MemoryAlertLevel ;
   int                  haveGoback ; 
   int                  nbad ;
   pthread_mutex_t      nbMutex ; 
   int                  nNeedTrim ;  

   uintptr_t            MaxMemoryAllowed ;
   unsigned int         MaxPacketsAllowed ;
   unsigned int         PerConnInQwm[2] ; 
   int                  DataPacketSize ;
   int                  MaxSqnPerNack ;

   int                  recvBuffsQsize ;
   int                  dataBuffsQsize ;
   int                  nackStrucQsize ;
   int                  evntStrucQsize ;
   int                  packStrucQsize ;
   int                  dataQsize ;
   int                  nackQsize ;
   int                  recvQsize ;
   int                  rsrvQsize ;
   int                  evntQsize ;
   int                  fragQsize ;
   int                  packQsize ;
   int                  prIn ;
   int                  ppIn ;
   int                  prOut ; 
   int                  tryLock ; 
   int                  pr_wfb ;
   int                  pp_wfb ;
   int                  ma_wfb ;
   int                  tp_wma ;
   int                  pp_signaled ;
   int                  ThreadPerTopic ;
   int                  UseNoMA ;
   int                  NumPPthreads ;
   int                  NumPRthreads ;
   int                  NumMAthreads ;
   pthread_t            prThreadID[MAX_THREADS] ; /* PacketReceiver  ThreadID */
   pthread_t            ppThreadID[MAX_THREADS] ; /* PacketProcessor   ThreadID */
   pthread_t            ngThreadID ; /* NAKGenerator      ThreadID */
   pthread_t            ntThreadID ; /* NAKTimer          ThreadID */
   pthread_t            eaThreadID ; /* EventAnnouncer    ThreadID */
   pthread_t            chThreadID ; /* ConnectionHandler ThreadID */
   pthread_t            maThreadID[MAX_THREADS] ; /* MessageAnnouncer ThreadIDs */
   pthread_mutex_t      maMutex ; 
   pthread_mutex_t      ppMutex ; 
   pthread_mutex_t      prMutex ; 
   TimerTasks           rTasks ; 
   int                  pp_wns ; 

   HashStreamInfo      *hash_table[HASH_SIZE] ;
   rStreamInfoRec      *last_pst ;

   unsigned char        PGM_OPT_PRESENT ;
   unsigned char        PGM_OPT_NET_SIG ;

   ifInfo               HostIF ;
   ipInfo               HostIP ;
   char                 HostAddress[RUM_ADDR_STR_LEN] ;
   char                 HostName[256]   ;
   const char          *cr_string ; 

   MemManRec           *recvBuffsQ ;
   MemManRec           *dataBuffsQ ;
   MemManRec           *nackStrucQ ;
   MemManRec           *evntStrucQ ;
   MemManRec           *packStrucQ ;

   LinkedListRec       *sockQ      ;
   BuffBoxRec          *rsrvQ      ;
   BuffBoxRec          *rmevQ      ;
   BuffBoxRec          *lgevQ      ;
   LinkedListRec       *mastQ      ;

   CondWaitRec          nakTcw  ;
   CondWaitRec          prWcw;
   CondWaitRec          eaWcw;

   GlobalSyncRec        GlobalSync ;

  rmm_get_external_time_t  extTime ; 
   void *                  extTime_param ;

   rum_on_log_event_t   on_log_event ;
   rum_on_event_t       on_alert_event ; 
   void                *log_user ;
   void                *alert_user ;
   rumBasicConfig       rConfig ;
   rumAdvanceConfig     aConfig ;
   WhyBadRec            why_bad ;
   pthread_mutex_t      wb_mutex ;
   FILE                *fp_log ;
  rTopicInfoRec        *rTopics[MAX_TOPICS] ;
                         
   pthread_mutex_t      pstsQlock ; 
  rStreamInfoRec       *pstsQfirst ;
   pthread_mutex_t      deadQlock ; 
  rStreamInfoRec       *deadQfirst ;
  rStreamInfoRec       *deadQlast  ;
  rStreamInfoRec       *firstStream ;
  rStreamInfoRec       *lastStream ;
   int                  nSCPs ; 
  scpInfoRec           *SCPs[MAX_STREAMS+1] ;
   RejectedRec          RejectedStreams[MAX_STREAMS+1] ;
  #if MAKE_HIST
   int                  hist_size;
   int                 *hist;
  #endif
  ConnInfoRec          *ConnOk[MAX_READY_CONNS] ; 
  TCHandle             tcHandle;
} rmmReceiverRec ;

typedef struct
{
   pgm_seq       pSN  ;
   int           state ;
   rmm_uint64    timer ;
   rmm_uint64    NewNakNcfTime ;
   rmm_uint64    OldNakNcfTime ;
    int32_t      NcfRetryCount ;
    int32_t      DataRetryCount ;
   rmm_uint64    rtt_time ;
   unsigned char flag ;

}  NackInfoRec ;

typedef struct
{
     rmm_uint64    eTime ; 
     void         *ll_next ; 
     char *        buffer ;
     char *        packet ;
     pgm_seq       msgSN ;
     int           msgID ;
     int           totLen ;
     int           curLen ;
     int           curNum ;

}  FragMsgInfoRec ;

#endif
