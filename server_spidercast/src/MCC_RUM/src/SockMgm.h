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


#ifndef  H_SockMgm_H
#define  H_SockMgm_H

 #define USE_MSGHDR 1

 #define TCP_MIN_CHUNK INT_SIZE

#if EAGAIN == EWOULDBLOCK
 #define eWB(x) ((x)==EAGAIN)
#else
 #define eWB(x) (((x)==EAGAIN) || ((x)==EWOULDBLOCK))
#endif

/* The IP header */
typedef struct
{
        uint8_t   ver_len ;        /* Type of service */
        uint8_t   tos;             /* Type of service */
        uint16_t  total_len;      /* total length of the packet */
        uint16_t  ident;          /* unique identifier */
        uint16_t  frag_and_flags; /* flags */
        uint8_t   ttl;
        uint8_t   proto;           /* protocol (TCP, UDP etc) */
        uint16_t  checksum;       /* IP checksum */

        uint32_t  sourceIP;
        uint32_t  destIP;

}IpHeader ;

typedef struct
{
        uint32_t  ver_plus ;
        uint16_t  payload;      /* payload length of the packet */
        uint8_t   next_hdr ;      /* next header */
        uint8_t   hops ;
        uint8_t   src_addr[16] ;
        uint8_t   dst_addr[16] ;

}Ip6Header;

#ifndef AI
#define AI struct addrinfo
#endif

#ifndef IA4
#define IA4 struct in_addr
#endif

#ifndef IA6
#define IA6 struct in6_addr
#endif

typedef struct
{
  int length ;
  union
  {
    uint8_t bytes[sizeof(struct in6_addr)] ;
    IA6 ia6;
    IA4 ia4;
  };
} ipFlat ;

enum
{
   TRY_THIS_ADDR=1,
   TRY_THIS_INDEX,
   TRY_THIS_NAME,
   TRY_THIS_ALL,
   TRY_THIS_NONE
} ;

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 128
#endif
typedef struct ifInfo_
{
  char user_name[RMM_HOSTNAME_STR_LEN] ;
  char name[IF_NAMESIZE] ;
  int  index ;
  int  flags ;
  int  af_req ;
  int  have_af;
  int  have_addr ;
  int  iTTT ;
  struct in_addr ipv4_addr ;
  struct in6_addr link_addr ;
  struct in6_addr site_addr ;
  struct in6_addr glob_addr ;
  uint32_t link_scope ;
  uint32_t site_scope ;
  uint32_t glob_scope ;
#define IF_HAVE_IPV4(x) (((x)->have_addr&0x1))
#define IF_HAVE_IPV6(x) (((x)->have_addr&0xe))
#define IF_HAVE_LINK(x) (((x)->have_addr&0x2))
#define IF_HAVE_SITE(x) (((x)->have_addr&0x4))
#define IF_HAVE_GLOB(x) (((x)->have_addr&0x8))
} ifInfo ;

#ifndef SA
#define SA struct sockaddr
#endif

#ifndef SA4
#define SA4 struct sockaddr_in
#endif

#ifndef SA6
#define SA6 struct sockaddr_in6
#endif

typedef  union
{
  SA  sa  ;
  SA4 sa4 ;
  SA6 sa6 ;
} SAA ;

typedef  union
{
  SA  sa  ;
  SA4 sa4 ;
  SA6 sa6 ;
} SAS ;

#define AI_AS_IPV4(x) ((struct sockaddr_in  *)(x)->ai_addr)
#define AI_AS_IPV6(x) ((struct sockaddr_in6 *)(x)->ai_addr)

#ifndef TCP_INFO_REC
#define TCP_INFO_REC
typedef struct TcpInfoRec TcpInfoRec ; 
#endif

typedef struct SockInfo_ SockInfo ; 

typedef int  (*pt2exit)(void *) ;
typedef int (*recv_packet_t) (SockInfo *si, SA *src_addr, char *packet, int len, pt2exit goDown , void *gdArg, int *errCode, char *errMsg, void **recv_si);

struct SockInfo_
{
  struct SockInfo_             *sq_next ;
  struct SockInfo_             *next ;
  void                         *inst_ctx ; 
  recv_packet_t                 RecvPacket;
  int                           req_af ;
  int                           type ;
  int                           proto ;
  in_port_t                     port, nbo_port ; 
  int                           is1st;
  uint32_t                      ncall ;
  uint32_t                      ngood ;
  uint32_t                      nselect ;
  PollInfo                      pi[1] ; 
  MUTEX_TYPE                    dLock ;
  pthread_mutex_t               qLock ;
  pthread_mutex_t               pLock ;
  SortedQRec                   *condSQ ;
  char                         *qFirst ;
  char                         *qLast ;
  int                           inQ ;
  int                           ppIn ;
  int                           pWait;
  uint32_t                      NextSqn ;
  unsigned long                 TotPacks[16] ;
  int                           bs[2] ;
  short                         mc_all[2] ; 
  int                           isuni;
  int                           ni ;
  int                           nr ;
  int                           ns ;
  SOCKET                        fd[2] ;
  SOCKET                        np1 ;
  SOCKET                        sd[2] ;
  int                           af[2] ;
  socklen_t                     sl[2] ;
  int                           counter ;
 #if USE_POLL
  struct pollfd                 fds[2] ; 
  nfds_t                        nfds ; 
 #else
  fd_set                        fds ;
 #endif
  TcpInfoRec                   *tcp ;
  ifInfo                       *ifi ;
 #if USE_MSGHDR
  struct msghdr                 msg[2] ;
  struct iovec                  iov[1] ;
 #endif
} ;


typedef struct
{
  AI ai ;
  SAS bytes ;
  char chars[64] ;
#define IP_INIT(x)    {memset(x,0,sizeof(ipInfo));\
                       (x)->ai.ai_addr=(SA *)&(x)->bytes;\
                       (x)->ai.ai_canonname=(x)->chars;\
                       (x)->ai.ai_addrlen=sizeof((x)->bytes);}

#define IP_AF(x)      ((x)->ai.ai_family)
#define IP_LEN(x)     ((x)->ai.ai_addrlen)
#define IP_SA(x)      ((x)->ai.ai_addr)
#define IP_SA4(x)      ((SA4 *)(x)->ai.ai_addr)
#define IP_SA6(x)      ((SA6 *)(x)->ai.ai_addr)
#define IP_IA4(x)      (((SA4 *)(x)->ai.ai_addr)->sin_addr)
#define IP_IA6(x)      (((SA6 *)(x)->ai.ai_addr)->sin6_addr)
} ipInfo ;

 #define closesocket(sfd) close(sfd)

typedef void (*pt2Log)   (void *dbgArg, char *msg) ;

#define ERR_MSG_SIZE 512


static int rmmGetLocalIf(char *try_this, int use_ib, int no_any, int req_mc, int req_af0, ifInfo *localIf, int *errCode, char *errMsg, TBHandle tbHandle) ;

static int rmmGetAddr(int af_req, const char *src, ipFlat *ip) ;
static int rmmGetAddrInfo(char *addr, char *port, int family, int stype, int proto, int flags, AI *result, int *errCode, char *errMsg) ;
static int rmmGetNameInfo(SA *sa, char *addr, size_t a_len, char *port, size_t p_len, int *errCode, char *errMsg) ;

static int llm_addr_scope(const void *a, int l) ; 
static int rmm_ntop(int af, void *src, char *dst, size_t size) ;
static const char *rmmErrStr(int code);

#endif
