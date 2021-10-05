/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
 
/*********************************************************************/
/*                                                                   */
/* Module Name: storeHAinternal.h                                    */
/*                                                                   */
/* Description: Store high-availability internal header file         */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_STORE_HA_INTERNAL_DEFINED
#define __ISM_STORE_HA_INTERNAL_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#define HA_WIRE_VERSION  1001

#define ERR_STR_LEN 256
#define HOSTNAME_STR_LEN  256
#define ADDR_STR_LEN  64
#define GROUP_MAX_LEN  512

#include        <stddef.h>
#include        <sched.h>
#include        <sys/types.h>   /* basic system data types                */
#include        <pthread.h>
#include        <stdlib.h>
#include        <stdarg.h>
#include        <stdint.h>
#include        <stdio.h>
#include        <malloc.h>
#include        <ctype.h>
#include        <string.h>
#include        <sys/utsname.h>
#include        <math.h>
#include        <sys/socket.h>  /* basic socket definitions               */
#include        <sys/time.h>    /* timeval{} for select()                 */
#include        <sys/timeb.h>   /* timeb for ftime                        */
#include        <time.h>        /* timespec{} for pselect()               */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <netinet/ip6.h>  /* sockaddr_in{} and other Internet defns */
#include        <netinet/tcp.h>
#include        <sys/ioctl.h>
#include        <errno.h>
#include        <fcntl.h>       /* for nonblocking                        */
#include        <netdb.h>
#include        <signal.h>
#include        <sys/stat.h>    /* for S_xxx file mode constants          */
#include        <sys/uio.h>     /* for iovec{} and readv/writev           */
#include        <unistd.h>
#include        <sys/wait.h>
#include        <arpa/inet.h>   /* inet(3) functions                      */
#include        <net/if.h>
#include        <poll.h>
#include        <ifaddrs.h>
#include        <net/if_arp.h>

#if USE_IB
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#endif

#if USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

//================================
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
//================================
#include  "storeMemory.h"
#include  "ismutil.h"
#include  "config.h"
#include  "ha.h"
#include  "storeHighAvailability.h"
#include <cluster.h>

#ifndef IF_NAMESIZE
#define IF_NAMESIZE IFNAMSIZ
#endif

//================================
#if USE_IB
#define MAX_INLINE_IB 512
#define MAX_INLINE_IW 64

#define DEFSYM(ret, name, parm) typedef ret (* t_##name)parm; static t_##name X##name
#define GETSYM(lib, name) (X##name = (t_##name) dlsym(lib, #name))

DEFSYM(void, ibv_ack_cq_events, (struct ibv_cq *cq, unsigned int nevents));
DEFSYM(struct ibv_pd *, ibv_alloc_pd, (struct ibv_context *context));
DEFSYM(struct ibv_ah *, ibv_create_ah, (struct ibv_pd *pd, struct ibv_ah_attr *attr));
DEFSYM(struct ibv_comp_channel *, ibv_create_comp_channel, (struct ibv_context *context));
DEFSYM(struct ibv_cq *, ibv_create_cq, (struct ibv_context *context, int cqe,void *cq_context,struct ibv_comp_channel *channel,int comp_vector));
DEFSYM(int, ibv_dealloc_pd, (struct ibv_pd *pd));
DEFSYM(int, ibv_dereg_mr, (struct ibv_mr *mr));
DEFSYM(int, ibv_destroy_ah, (struct ibv_ah *ah));
DEFSYM(int, ibv_destroy_comp_channel, (struct ibv_comp_channel *channel));
DEFSYM(int, ibv_destroy_cq, (struct ibv_cq *cq));
DEFSYM(int, ibv_get_cq_event, (struct ibv_comp_channel *channel,struct ibv_cq **cq, void **cq_context));
DEFSYM(int, ibv_modify_qp, (struct ibv_qp *qp, struct ibv_qp_attr *attr,enum ibv_qp_attr_mask attr_mask));
DEFSYM(int, ibv_query_port, (struct ibv_context *context, uint8_t port_num,struct ibv_port_attr *port_attr));
DEFSYM(int, ibv_query_qp, (struct ibv_qp *qp, struct ibv_qp_attr *attr,enum ibv_qp_attr_mask attr_mask,struct ibv_qp_init_attr *init_attr));
DEFSYM(int, ibv_attach_mcast, (struct ibv_qp *qp, union ibv_gid *gid,uint16_t lid));
DEFSYM(int, ibv_detach_mcast, (struct ibv_qp *qp, union ibv_gid *gid,uint16_t lid));
DEFSYM(int, ibv_query_device, (struct ibv_context *context, struct ibv_device_attr *device_attr));
DEFSYM(struct ibv_mr *, ibv_reg_mr, (struct ibv_pd *pd, void *addr,size_t length, enum ibv_access_flags access));
DEFSYM(struct ibv_qp *, ibv_create_qp, (struct ibv_pd *pd,struct ibv_qp_init_attr *qp_init_attr));
DEFSYM(int, ibv_destroy_qp, (struct ibv_qp *qp));
#if USE_IBEL
DEFSYM(int, ibv_get_async_event, (struct ibv_context *context,struct ibv_async_event *event));
DEFSYM(void, ibv_ack_async_event, (struct ibv_async_event *event));
#endif
DEFSYM(int, ibv_fork_init, (void));

DEFSYM(int, rdma_accept, (struct rdma_cm_id *id, struct rdma_conn_param *conn_param));
DEFSYM(int, rdma_ack_cm_event, (struct rdma_cm_event *event));
DEFSYM(int, rdma_bind_addr, (struct rdma_cm_id *id, struct sockaddr *addr));
DEFSYM(int, rdma_connect, (struct rdma_cm_id *id, struct rdma_conn_param *conn_param));
DEFSYM(int, rdma_disconnect, (struct rdma_cm_id *id));
DEFSYM(struct rdma_event_channel *, rdma_create_event_channel, (void));
DEFSYM(int, rdma_create_id, (struct rdma_event_channel *channel,struct rdma_cm_id **id, void *context,enum rdma_port_space ps));
DEFSYM(int, rdma_create_qp, (struct rdma_cm_id *id, struct ibv_pd *pd,struct ibv_qp_init_attr *qp_init_attr));
DEFSYM(void, rdma_destroy_qp, (struct rdma_cm_id *id));
DEFSYM(void, rdma_destroy_event_channel, (struct rdma_event_channel *channel));
DEFSYM(int, rdma_destroy_id, (struct rdma_cm_id *id));
DEFSYM(int, rdma_get_cm_event, (struct rdma_event_channel *channel,struct rdma_cm_event **event));
DEFSYM(int, rdma_join_multicast, (struct rdma_cm_id *id, struct sockaddr *addr,void *context));
DEFSYM(int, rdma_leave_multicast, (struct rdma_cm_id *id, struct sockaddr *addr));
DEFSYM(int, rdma_listen, (struct rdma_cm_id *id, int backlog));
DEFSYM(int, rdma_resolve_addr, (struct rdma_cm_id *id, struct sockaddr *src_addr,struct sockaddr *dst_addr, int timeout_ms));
DEFSYM(int, rdma_resolve_route, (struct rdma_cm_id *id, int timeout_ms));
#endif
//================================

#if USE_SSL
extern void ism_ssl_init(int useFips, int useBufferPool);
// Already defined in transport.h which is included imlicitly via config.h
#if 0
enum ism_SSL_Methods {
    SecMethod_SSLv3     =  1,
    SecMethod_TLSv1     =  2,
    SecMethod_TLSv1_1   =  3,
    SecMethod_TLSv1_2   =  4
};

enum ism_Cipher_Methods {
    CipherType_Best      = 1,      /* Best security */
    CipherType_Fast      = 2,      /* Optimize for speed with high security */
    CipherType_Medium    = 3,      /* Deprecated, now the same as Fast */
    CipherType_Custom    = 4
};
/*
 * SSL methods enumeration
 */
ism_enumList enum_methods2 [] = {
    { "Method",    4,                 },
    { "SSLv3",     SecMethod_SSLv3,   },
    { "TLSv1",     SecMethod_TLSv1,   },
    { "TLSv1.1",   SecMethod_TLSv1_1, },
    { "TLSv1.2",   SecMethod_TLSv1_2, },
};
#endif


typedef struct
{
  struct ssl_ctx_st *sslCtx[2] ; // i = init_here 
  int      FIPSmode ; 
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


typedef enum
{
    HA_MSG_TYPE_CON_REQ       = 1,    /**< Connect Request                                               */
    HA_MSG_TYPE_CON_RES          ,    /**< Connect Response                                              */
    HA_MSG_TYPE_CON_ACK          ,    /**< Connect Ack                                                   */
    HA_MSG_TYPE_CON_HBT          ,    /**< Connect Heartbeat                                             */
    HA_MSG_TYPE_CON_CID          ,    /**< Channel id                                                    */
    HA_MSG_TYPE_NOD_TRM          ,    /**< STONITH                                                       */
    HA_MSG_TYPE_ACK_TRM          ,    /**< STONITH ack                                                   */
    HA_MSG_TYPE_DIS_REQ               /**< Discovery Request                                             */
} HA_MSG_TYPE_t ;

#define HA_MSG_FLAG_IS_PRIM      0x001
#define HA_MSG_FLAG_PREF_PRIM    0x002
#define HA_MSG_FLAG_CAN_BE_PRIM  0x004
#define HA_MSG_FLAG_HAVE_DATA    0x008
#define HA_MSG_FLAG_STANDALONE   0x010
#define HA_MSG_FLAG_WAS_PRIM     0x020
#define HA_MSG_FLAG_USE_COMPACT  0x040
#define HA_MSG_FLAG_NO_RESYNC    0x080
#define HA_MSG_FLAG_IS_CLUSTER   0x100

typedef struct
{
  int is_primary ; 
  int is_standalone ; 
  int can_be_primary ; 
  int have_data ; 
  int was_primary ; 
  int pref_prim ; 
  int use_comp ; 
  int no_resync ; 
  int is_cluster; 
  int SplitBrainPolicy;
  int numActiveConns;
} haReqFlags ; 

typedef struct haConComMsg
{
   uint32_t                  msg_len ;                    /**< Total message length                                           */
   uint32_t                  version ;                    /**< HA version                                                     */
   uint32_t                  msg_type ;                   /**< Message type                                                   */
}haConComMsg ;

typedef struct haConDisMsg
{
   uint32_t                  msg_len ;                    /**< Total message length                                           */
   uint32_t                  version ;                    /**< HA version                                                     */
   uint32_t                  msg_type ;                   /**< Message type HA_MSG_TYPE_DIS_REQ                               */
   uint32_t                  flags  ;                     /**< Not used yet                                                   */
   uint32_t                  id_len ;                     /**< Length of node ID                                              */
   ismStore_HANodeID_t       source_id ;                  /**< ID of source node                                              */
   uint32_t                  port ;                       /**< Discovery port                                                 */
   uint32_t                  grp_len ;                    /**< Length of Group                                                */
   uint32_t                  adr_len ;                    /**< Length of Addresses                                            */
   char                      data[1] ;                    /**< Group + Addresses of DiscoveryNIC (comma separated, NULL term) */
}haConDisMsg ;

typedef struct haConReqMsg
{
   uint32_t                  msg_len ;                    /**< Total message length                                           */
   uint32_t                  version ;                    /**< HA version                                                     */
   uint32_t                  msg_type ;                   /**< Message type HA_MSG_TYPE_CON_REQ                               */
   uint32_t                  flags  ;                     /**< New | pref | can_be | clean | alone                            */
   uint32_t                  TotalMemSizeMB ;             /**< Store config TotalMemSizeMB                                    */
   uint32_t                  MgmtSmallGranuleSizeBytes ;  /**< Store config MgmtSmallGranuleSizeBytes                         */
   uint32_t                  MgmtGranuleSizeBytes ;       /**< Store config MgmtGranuleSizeBytes                              */
   uint32_t                  GranuleSizeBytes ;           /**< Store config GranuleSizeBytes                                  */
   uint32_t                  MgmtMemPercent ;             /**< Store config MgmtMemPercent                                    */
   uint32_t                  InMemGensCount ;             /**< Store config InMemGensCount                                    */
   uint32_t                  ReplicationProtocol ;        /**< HA config ReplicationProtocol                                  */
   uint32_t                  id_len ;                     /**< Length of node ID                                              */
   ismStore_HANodeID_t       source_id ;                  /**< ID of source node                                              */
   ismStore_HANodeID_t       destination_id ;             /**< ID of destination node. Zero if unknown                        */
   ismStore_memGenToken_t    shutdown_token[1];           /**< ShutDown token                                                 */
   uint32_t                  hbto ; 
   uint32_t                  session_count ; 
   ismStore_HASessionID_t    session_id ; 
   uint32_t                  grp_len ;                    /**< Length of Group                                                */
   char                      data[GROUP_MAX_LEN+1] ;      /**< Group                                                          */
   uint32_t                  pad[2] ;                     /** SplitBrainPolicy + numActiveConns                               */
}haConReqMsg ;

typedef struct haConResMsg
{
   uint32_t                  msg_len ;                    /**< Total message length                                           */
   uint32_t                  version ;                    /**< HA version                                                     */
   uint32_t                  msg_type ;                   /**< Message type HA_MSG_TYPE_CON_RES                               */
   uint32_t                  conn_rejected ;              /**< Is the connection rejected (cannot act as HA-Pair)             */
   uint32_t                  reject_reason ;              /**< In case of reject a reason code                                */
   uint32_t                  role_local ;                 /**< The expected role of the local node (Primary or Standby)       */
   uint32_t                  role_remote ;                /**< The expected role of the remote node (Primary or Standby)      */
   uint16_t                  ha_port ;                    /**< Port used in HA NIC                                            */
   uint16_t                  ha_addr_len ;                /**< Address length of HA NIC                                              */
   uint8_t                   ha_nic[16] ;                 /**< Address of HA NIC (internal form, NOT string)                  */
   uint32_t                  id_len ;                     /**< Length of node ID                                              */
   ismStore_HANodeID_t       source_id ;                  /**< ID of source node                                              */
   ismStore_HANodeID_t       destination_id ;             /**< ID of destination node. Zero if unknown                        */
}haConResMsg ;

typedef struct haConAckMsg
{
   uint32_t                  msg_len ;                    /**< Total message length                                           */
   uint32_t                  version ;                    /**< HA version                                                     */
   uint32_t                  msg_type ;                   /**< Message type HA_MSG_TYPE_CON_ACK                               */
   uint32_t                  conn_rejected ;              /**< Is the connection rejected (cannot act as HA-Pair)             */
   uint32_t                  reject_reason ;              /**< In case of reject a reason code                                */
   uint32_t                  role_local ;                 /**< The expected role of the local node (Primary or Standby)       */
   uint32_t                  role_remote ;                /**< The expected role of the remote node (Primary or Standby)      */
   uint32_t                  id_len ;                     /**< Length of node ID                                              */
   ismStore_HANodeID_t       source_id ;                  /**< ID of source node                                              */
   ismStore_HANodeID_t       destination_id ;             /**< ID of destination node. Zero if unknown                        */
}haConAckMsg ;

typedef struct haConHbtMsg
{
   uint32_t                  msg_len ;                    /**< Total message length                                           */
   uint32_t                  version ;                    /**< HA version                                                     */
   uint32_t                  msg_type ;                   /**< Message type HA_MSG_TYPE_CON_HBT                               */
   uint32_t                  hbt_count ;                  /**< HB count                                                       */
}haConHbtMsg ;

typedef struct haConCidMsg
{
   uint32_t                  msg_len ;                    /**< Total message length                                           */
   uint32_t                  version ;                    /**< HA version                                                     */
   uint32_t                  msg_type ;                   /**< Message type HA_MSG_TYPE_CON_CID                               */
   uint32_t                  chn_id ;                     /**< Channel ID                                                     */
}haConCidMsg ;

typedef union
{
  haConReqMsg req ; 
  haConResMsg res ; 
  haConAckMsg ack ; 
  haConHbtMsg hbt ; 
  haConCidMsg cid ; 
  haConComMsg com ; 
} haConAllMsg ; 


//================================

#define T_INVALID  0x01
#define R_INVALID  0x02
#define P_INVALID  0x04
#define C_INVALID  0x08
#define A_INVALID  0x10



#if 0
#define DBG_PRINT(fmt...) printf(fmt)
#define TRDBG(x) { char traceMsg[ERR_STR_LEN]; x ; fputs(traceMsg,stdout); }
#else
#define DBG_PRINT(fmt...)
#define TRDBG(x)
#endif


#define CIP_DBG 0

#if CIP_DBG
 #define DBG_PRT(x) x
#else
 #define DBG_PRT(x)
#endif

#if EAGAIN == EWOULDBLOCK
 #define eWB(x) ((x)==EAGAIN)
#else
 #define eWB(x) (((x)==EAGAIN) || ((x)==EWOULDBLOCK))
#endif

typedef struct 
{
  int   errCode ; 
  int   errLen  ; 
  char  errMsg[ERR_STR_LEN] ; 
} errInfo ; 

#define SA  struct sockaddr
#define SA4 struct sockaddr_in
#define SA6 struct sockaddr_in6
#define SAl struct sockaddr_ll

typedef  union
{
  SA  sa  ;
  SA4 sa4 ;
  SA6 sa6 ;
} SAA, SAS ;

#define IA4 struct in_addr
#define IA6 struct in6_addr

typedef union
{
  IA4 ia4 ; 
  IA6 ia6 ; 
} IAA ; 

typedef struct ipFlat_
{
  struct ipFlat_ *next ; 
  int             length ;
  int             scope ; 
  IAA             bytes[2] ;
  char            label[IF_NAMESIZE] ;
} ipFlat ;
#define ENSURE_LEN(x) if((x)->length>sizeof((x)->bytes))(x)->length=sizeof((x)->bytes) 

typedef struct nicInfo_
{
  struct nicInfo_ *next ; 
  char            name[IF_NAMESIZE] ; 
  int             mtu   ; 
  int             flags ; 
  int             index ; 
  int             type  ; 
  int             naddr ; 
  ipFlat         *addrs ;
  ipFlat          haddr[1];
} nicInfo ; 

enum
{
   TRY_THIS_ADDR=1,
   TRY_THIS_INDEX,
   TRY_THIS_NAME,
   TRY_THIS_ALL,
   TRY_THIS_NONE
} ;

typedef struct ifInfo_
{
  struct ifInfo_ *next ;
  nicInfo *ni ; 
  ipFlat  *tt_addr ;
  char user_name[HOSTNAME_STR_LEN] ;
  char addr_name[ADDR_STR_LEN] ;
  char name[IF_NAMESIZE] ;
  int  index ;
  int  flags ;
  int  have_addr ;
  int  iTTT ;
  struct in_addr  ipv4_addr ;
  struct in6_addr link_addr ;
  struct in6_addr site_addr ;
  struct in6_addr glob_addr ;
  uint32_t link_scope ;
  uint32_t site_scope ;
  uint32_t glob_scope ;
 #if USE_IB
  struct ibv_context *ibv_ctx ;
  struct ibv_pd      *ib_pd ;
  struct rdma_cm_id  *cm_id ;
  int    ib_ps ;
  int    ib_port ;
  int    ib_state;
  int    ib_mtu ;
  int    ib_iwarp ;
  int    ib_max_wr;
  int    ib_max_inline;
 #if USE_IBEL
  ibel_evInfo ibel_info[1];
  ibel_evInfo bond_info[1];
 #endif
 #endif
#define IF_HAVE_IPV4(x) (((x)->have_addr&0x1))
#define IF_HAVE_IPV6(x) (((x)->have_addr&0xe))
#define IF_HAVE_LINK(x) (((x)->have_addr&0x2))
#define IF_HAVE_SITE(x) (((x)->have_addr&0x4))
#define IF_HAVE_GLOB(x) (((x)->have_addr&0x8))
#define IF_ONLY_LINK(x) (((x)->have_addr&0xf)==0x2)
} ifInfo ;


#if USE_IB
typedef struct
{
  struct ibv_context           *ibv_ctx ;
  struct ibv_cq                *ib_cq ;
  struct ibv_qp                *ib_qp ; 
  struct ibv_comp_channel      *ib_ch ;
  struct ibv_mr                *ib_mr ;
  struct ibv_send_wr           *wr1st ;
  struct ibv_recv_wr           *wr2nd ;
  void                         *wr_mem ;
  void                         *reg_mem ;
  int                           inUse ;
} ibInfo ; 
#endif

#define AI struct addrinfo
#define AI_AS_IPV4(x) ((struct sockaddr_in  *)(x)->ai_addr)
#define AI_AS_IPV6(x) ((struct sockaddr_in6 *)(x)->ai_addr)

typedef struct ipInfo_
{
  struct ipInfo_  *next ; 
  AI               ai ;
  SAS              bytes ;
  char             chars[ADDR_STR_LEN] ;
#define IP_INIT(x)    {memset(x,0,sizeof(ipInfo));\
                       (x)->ai.ai_addr=(SA *)&(x)->bytes;\
                       (x)->ai.ai_canonname=(x)->chars;\
                       (x)->ai.ai_addrlen=sizeof((x)->bytes);}
#define IP_COPY(x,y)  {memcpy(&((x)->ai),y,sizeof(AI));\
                       (x)->ai.ai_addr=(SA *)&(x)->bytes;\
                       (x)->ai.ai_canonname=(x)->chars;\
                       memcpy((x)->ai.ai_addr, (y)->ai_addr, (y)->ai_addrlen);\
                       (x)->ai.ai_next=NULL;}
#define IP_AF(x)      ((x)->ai.ai_family)
#define IP_LEN(x)     ((x)->ai.ai_addrlen)
#define IP_SA(x)      ((x)->ai.ai_addr)
#define IP_SA4(x)      ((SA4 *)(x)->ai.ai_addr)
#define IP_SA6(x)      ((SA6 *)(x)->ai.ai_addr)
#define IP_IA4(x)      (((SA4 *)(x)->ai.ai_addr)->sin_addr)
#define IP_IA6(x)      (((SA6 *)(x)->ai.ai_addr)->sin6_addr)
} ipInfo ;

typedef uint64_t connectionID_t ;
typedef struct 
{
  char         *bptr ; 
  char         *buffer ; 
  uint32_t      buflen ; 
  uint32_t      reqlen ; 
  uint32_t      offset ; 
  int           need_free;
 #if USE_IB
  /*---> start ibInfo struct map  <---*/
  struct ibv_context      *ibv_ctx ; 
  struct ibv_cq           *ib_cq ; 
  struct ibv_qp           *ib_qp ; 
  struct ibv_comp_channel *ib_ch ; 
  struct ibv_mr           *ib_mr ; 
  struct ibv_send_wr      *wr1st ; 
  struct ibv_recv_wr      *wr2nd ; 
  void                    *wr_mem ; 
  void                    *reg_mem ; 
  int                      inUse ; 
  /*---> end   ibInfo struct map  <---*/
  int                      ib_bs ; 
  int                      send_flags[2] ; 
  size_t                   reg_len ; 
  ifInfo                  *ifi ; 
 #else
  int                      inUse ; 
 #endif
  uint64_t            nBytes ; 
  uint64_t            nPacks ; 
} ioInfo ; 

typedef struct ChannInfo_ ChannInfo ; 

typedef struct ConnInfo_ ConnInfoRec ; 
typedef int (*conn_read_f)(ConnInfoRec *cInfo) ; 

struct ConnInfo_
{
  struct ConnInfo_   *next ; 
 #if USE_IB
  struct rdma_cm_id       *cm_id ; 
  struct rdma_cm_event    *cm_ev ; 
  struct ibv_context      *ibv_ctx ; 
  struct ibv_pd           *ib_pd ; 
  struct ibv_qp           *ib_qp ; 
 #endif
  connectionID_t      conn_id ;
  ChannInfo          *channel ; 
  conn_read_f         conn_read ; 
  int                 use_ib ; 
  int                 use_ssl ; 
  int                 requireCertificates ;
  uint32_t            ccp_sqn ;
  uint32_t            inQ[2] ;
  int                 hold_it ; 
  int                 init_here ; 
  int                 is_ha ; 
  int                 is_hb ; 
  int                 noHBloops ; 
  char                req_addr[HOSTNAME_STR_LEN];
  int                 req_port ; 
  int                 msg_len ; 
  char               *msg_buf ; 
  void               *event_ctx ; 
  double              next_r_time ; 
  double              next_s_time ; 
  int                 hb_count ; 
  int                 hb_interval ; 
  int                 one_way_hb ; 
  int                 to_detected ; 
  int                 sfd ; 
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
  int                 iPoll; 
  int                 nPoll; 
  int                 ev_sent ; 
  nfds_t              nfds ; 
  struct pollfd       pfds[2] ; 
  #if USE_SSL
   sslConnInfo_t      sslInfo[1];
  #endif
} ;

typedef enum
{
  CH_EV_CON_READY=1,
  CH_EV_CON_BROKE  ,
  CH_EV_SYS_ERROR  ,
  CH_EV_SSL_ERROR  ,
  CH_EV_NEW_VIEW   ,
  CH_EV_TERMINATE   
} chEventType_t ; 

typedef struct eventInfo_
{
  struct eventInfo_ *next ; 
  void              *event_ctx ; 
  int                event_type ; 
} eventInfo ; 


struct ChannInfo_
{
  struct ChannInfo_        *next ; 
  ConnInfoRec              *cInfo ; 
  eventInfo                *events ; 
  void                     *uCtx ; 
  ismPSTOREHACHCLOSED       ChannelClosed ; 
  ismPSTOREHAMSGRECEIVED    MessageReceived ; 
  pthread_t                 thId ; 
  pthread_mutex_t           lock[1] ; 
  pthread_cond_t            cond[1] ; 
  int                       thUp ; 
  int                       use_lock ; 
  int                       closing ; 
  int                       broken ; 
  uint32_t                  channel_id ; 
  uint32_t                  flags ; 
} ; 

#define EV_MSG_SIZE 128

typedef struct
{
  int            nConnsInProg ; 
  int            cidInit;
  ConnInfoRec   *firstConnInProg ;
//ConnIdRec      ConnId ; 
  struct pollfd *fds ; 
  nfds_t         nfds ; 
  int            lfds ; 
  int            chUP ; 
  int            chEC ; 
  char           ev_msg[EV_MSG_SIZE] , *ev_dbg[2] ; 
} cipInfoRec ; 

#define DSC_IS_ON     0x00001
#define DSC_TIMEOUT   0x00002
#define DSC_HAVE_PAIR 0x00004
#define DSC_WORK_SOLO 0x00008
#define REQ_MSG_SENT  0x00010
#define REQ_MSG_RECV  0x00020
#define RES_MSG_SENT  0x00040
#define RES_MSG_RECV  0x00080
#define ACK_MSG_SENT  0x00100
#define ACK_MSG_RECV  0x00200
#define REQ_RECV_RMT  0x00400
#define REQ_SENT_LCL  0x00800
#define DSC_VU_SENT   0x01000
#define DSC_RESTART   0x02000
#define TRM_MSG_SENT  0x04000
#define TRM_MSG_RECV  0x08000
#define ACK_TRM_SENT  0x10000
#define ACK_TRM_RECV  0x20000
#define DSC_SB_TERM   0x40000
#define DSC_SSL_ERR   0x80000

#define HA_STATE_BEGIN  0
#define HA_STATE_DISCN  0

typedef enum
{
   CIP_IO_STATE_ACC = 1   ,
   CIP_IO_STATE_CONN      ,
   CIP_IO_STATE_WRITE     ,
   CIP_IO_STATE_READ      ,
   CIP_IO_STATE_SSL_HS    ,
   CIP_IB_STATE_CREQ      ,
   CIP_IB_STATE_CEST      ,
   CIP_IB_STATE_ADDR      ,
   CIP_IB_STATE_ROUTE     ,
   CIP_IO_STATE_NOP         
} cip_io_state_t ; 

typedef enum
{
   CIP_STATE_S_CFP_REQ =1,
   CIP_STATE_S_CFP_RES   ,
   CIP_STATE_S_CFP_ACK   , 
   CIP_STATE_S_CFP_HBT   , 
   CIP_STATE_S_CFP_CID   , 
   CIP_STATE_LISTEN      , 
   CIP_STATE_ESTABLISH   , 
   CIP_STATE_BROKEN        
} cip_state_t ;

typedef enum
{
   CIP_STARTUP_AUTO_DETECT=0,
   CIP_STARTUP_STAND_ALONE
} cip_startup_t ; 

typedef enum
{
  VIEW_CHANGE_TYPE_DISC =1,
  VIEW_CHANGE_TYPE_PRIM_SOLO ,
  VIEW_CHANGE_TYPE_BKUP_TERM ,
  VIEW_CHANGE_TYPE_BKUP_ERR  ,
  VIEW_CHANGE_TYPE_BKUP_PRIM 
} viewChangeType ; 

typedef struct 
{
  int            sfd[2] ; 
  SAS            sas[2] ; 
  SA4           *sa4 ; 
  SA6           *sa6 ; 
  struct pollfd  fds[2] ; 
  nfds_t         nfds ; 
  int            grp_len ; 
  haConDisMsg   *iMsg ; 
  haConDisMsg   *oMsg ; 
  size_t         iLen ; 
  double         nextS ;
} autoConfInfo ; 

typedef struct
{
  ConnInfoRec  *cIha ;     // acc conn on ha If
  ConnInfoRec  *cIhb[2] ;  // HB conns on adm & rep NICs
  ConnInfoRec  *cIlc ; 
  ConnInfoRec  *cIrm ; 
  haConReqMsg  req_msg[2] ; 
  haConResMsg  res_msg[2] ; 
  haConAckMsg  ack_msg[2] ; 
  double       etime[2] ; 
  int          state ; 
  int          sendBit ; 
  int          bad[2] ; 
  char         reasonParm[128];
} dscInfoRec ; 

typedef struct
{
   pthread_t                haId ; 
   pthread_mutex_t          haLock[1] ; 
   nicInfo                 *niHead ; 
   ifInfo                   haIf[1], hbIf[1] ; 
   ipInfo                  *ipHead, *ipCur, ipRmt[1] , ipLcl[1], ipExt[1] ; 
   double                   haTimeOut[2] ; 
   double                   hbTimeOut ; 
   double                   hbInterval; 
   double                   nextResolve;
   int                      hbTOloops ; 
   int                      recvNpoll ; 
   int                      sbError ; 
   ismHA_Role_t             myRole ; 
   int                      viewBreak ;  
   int                      viewCount ;  
   double                   viewTime ; 
   ismStore_HAView_t        lastView[1] ; 
   int                      goDown ;  
   int                      use_ib ; 
   int                      use_ssl ; 
   int                      require_certificates;
   ismStore_HAConfig_t      config[1];
   ismStore_HAParameters_t  params[1];
   cipInfoRec               cipInfo[1] ; 
   dscInfoRec               dInfo[1] ; 
   autoConfInfo             mcInfo[1] ; 
   ConnInfoRec             *connReqQ ;
   ChannInfo               *chnHead ; 
   int                      SockBuffSizes[3][2] ; 
   ismStore_HANodeID_t      server_id ; 
   connectionID_t           conn_id ;
   char                    *failedNIC;
   int                      nchu;
   int                      nblu;
   int                      nchs ; 
   int                      sfds ; 
   nfds_t                   nfds ; 
   struct pollfd           *pfds ; 
  #if USE_SSL
   sslGlobInfo_t            sslInfo[1];
  #endif
} haGlobalInfo ; 



#ifdef __cplusplus
}
#endif

#endif /* __ISM_STORE_HA_INTERNAL_DEFINED */
