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


#ifndef  H_SockMgm_C
#define  H_SockMgm_C

#include "SockMgm.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#if FW2IP
static int fw2ip;
#endif

#if DEBUG
 #define DBG(x) x
#else
 #define DBG(x)
#endif

#define TRACE_MSG_SIZE 1024

  #define TRDBG(x) { char traceMsg[TRACE_MSG_SIZE]; x ; trace_msg(tbHandle, traceMsg); }
  #define TRACE_DEBUG 1


static void trace_msg(TBHandle tbHandle, char *traceMsg)
{
#if DEBUG
 fputs(traceMsg,stdout);
#endif
#if TRACE_DEBUG

    if ( tbHandle != NULL )
    llmAddTraceData(tbHandle,"",traceMsg);

#endif
}

#define RDMA_ERR(x) (((x)==-1)?rmmErrno:((x)<0?(-x):(x)))

#define MAX_INLINE_IB 400
#define MAX_INLINE_IW 64



/**************************************************/
/**************************************************/
static int rmm_pton(int af, const char *src, void *dst) ;
static int rmm_set_nb(SOCKET fd, int *errCode, char *errMsg, const char *myName);
/**************************************************/

#define MAX_ERR_STR  2048
#define ERR_STR_SIZE  64
static int err2str_init=0 ;
static char const *err2str[MAX_ERR_STR] ;
static void init_err2str(void);
static void add_err2str(int code, const char *txt);

/**************************************************/

static int socket_(int domain, int type, int protocol)
{
 #ifdef SOCK_CLOEXEC
  type |= SOCK_CLOEXEC ; 
  return socket(domain, type, protocol);
 #else
  int fd = socket(domain, type, protocol);
  if ( fd >= 0 ) set_CLOEXEC(fd) ; 
  return fd ; 
 #endif
}

static int accept_(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
 #ifdef __USE_GNU
  return accept4(sockfd, addr, addrlen, SOCK_CLOEXEC);
 #else
  int fd = accept(sockfd, addr, addrlen) ; 
  if ( fd >= 0 ) set_CLOEXEC(fd) ; 
  return fd ; 
 #endif
}
/**************************************************/
#if USE_SSL
/*
 * Names for TLS errors
 */
static const char * SSL_ERRORS[9] = {
    "SSL_ERROR_NONE",
    "SSL_ERROR_SSL",
    "SSL_ERROR_WANT_READ",
    "SSL_ERROR_WANT_WRITE",
    "SSL_ERROR_WANT_X509_LOOKUP",
    "SSL_ERROR_SYSCALL",
    "SSL_ERROR_ZERO_RETURN",
    "SSL_ERROR_WANT_CONNECT",
    "SSL_ERROR_WANT_ACCEPT"
};

/*
 * Trace any errors from openssl
 */

static void sslTraceErr(ConnInfoRec *cInfo, uint32_t  rc, const char * file, int line) 
{
    int          flags;
    const char * data;
    char         mbuf[1024];
    char *       pos;
    int          err = errno;
    TCHandle tcHandle;
    rmmReceiverRec *rInfo ; 
    rumInstanceRec *gInfo = (rumInstanceRec *)cInfo->gInfo ; 
    rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
    tcHandle = rInfo->tcHandle ; 

  //if (SHOULD_TRACE(4)) 
    {
        const char *func = cInfo->sslInfo->func ? cInfo->sslInfo->func : "Unknown" ; 
        if (rc) {
            const char * errStr = (rc < 9) ? SSL_ERRORS[rc] : "SSL_UNKNOWN_ERROR";
            if (err) {
                data = strerror_r(err, mbuf, 1024);
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1130),"%s%s%d%s%s%d%s%d",
                    "openssl error_1: func={0}, conn= |{1}|, error({2}): {3} : errno is |{4}| ({5}), at {6}.{7}",
                    func,cInfo->req_addr, rc, errStr, data, err, file, line);
            } else {
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1131),"%s%s%d%s%s%d",
                    "openssl error_2: func={0}, conn= |{1}|, error({2}): {3} , at {4}.{5}",
                    func,cInfo->req_addr, rc, errStr, file, line);
            }
        }
        for (;;) {
            rc = (uint32_t)ERR_get_error_line_data(&file, &line, &data, &flags);
            if (rc == 0)
                break;
            ERR_error_string_n(rc, mbuf, sizeof mbuf);
            pos = strchr(mbuf, ':');
            if (!pos)
                pos = mbuf;
            else
                pos++;
            if (flags&ERR_TXT_STRING) {
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1132),"%s%s%d%s%s%s%d",
                    "openssl error_3: func={0}, conn= |{1}|, error({2}): {3} : data= |{4}| , at {5}.{6}",
                    func,cInfo->req_addr, rc, pos, data, file, line);
            } else {
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1133),"%s%s%d%s%s%d",
                    "openssl error_4: func={0}, conn= |{1}|, error({2}): {3} , at {4}.{5}",
                    func,cInfo->req_addr, rc, pos, file, line);
            }
        }
    }
}
#endif
/**************************************************/

int rmm_write(ConnInfoRec *cInfo)
{
  char  *buf = cInfo->wrInfo.bptr ; 
  size_t len = cInfo->wrInfo.reqlen-cInfo->wrInfo.offset ; 

 #if USE_SSL
  if ( cInfo->use_ssl )
  {
    int ret,rc;
    struct pollfd pfd ; 
    pfd.fd = cInfo->sfd ; 
    pthread_mutex_lock(cInfo->sslInfo->lock) ;
    for(;;)
    {
      ret = SSL_write(cInfo->sslInfo->ssl, buf, len) ;
      rc  = SSL_get_error(cInfo->sslInfo->ssl, ret) ; 
      if ( rc == SSL_ERROR_NONE )
      {
        rc = ret ; 
        break ; 
      }
      else
      if ( rc == SSL_ERROR_WANT_READ )
      {
        pfd.events = POLLIN ; 
      }
      else
      if ( rc == SSL_ERROR_WANT_WRITE)
      {
        rc = 0 ; 
        break ; 
      }
      else
      {
        if ( rc == SSL_ERROR_SYSCALL && ret == -1 && eWB(errno) )
        {
          rc = 0 ; 
        }
        else
        {
          cInfo->sslInfo->func = "SSL_write" ;
          sslTraceErr(cInfo, rc, __FILE__, __LINE__);
          rc = -1 ; 
        }
        break ; 
      }
      poll(&pfd, 1, 1) ; 
    }
    pthread_mutex_unlock(cInfo->sslInfo->lock) ;
    return rc ; 
  }
 #endif

  return write(cInfo->sfd, buf, len   ) ;
}
/**************************************************/
int rmm_read(ConnInfoRec *cInfo, char *buf, int len, int copy, int *errCode, char *errMsg)
{
  int ret;

  errMsg[0]=0;

 #if USE_SSL
  if ( cInfo->use_ssl )
  {
    int rc;
    struct pollfd pfd ; 
  
    pfd.fd = cInfo->sfd ; 
    pfd.events = POLLOUT ; 
    pthread_mutex_lock(cInfo->sslInfo->lock) ; 
    for(;;)
    {
      ret = SSL_read(cInfo->sslInfo->ssl, buf, len) ;
      if ( ret > 0 )
      {
        rc = ret ; 
        break ; 
      }
      rc  = SSL_get_error(cInfo->sslInfo->ssl, ret) ; 
      if ( rc == SSL_ERROR_NONE )
      {
        rc = ret ; 
        break ; 
      }
      else
      if ( rc == SSL_ERROR_WANT_READ )
      {
        rc = 0 ; 
        break ; 
      }
      else
      if ( rc == SSL_ERROR_WANT_WRITE)
      {
        pfd.events = POLLOUT; 
        poll(&pfd, 1, 1) ; 
      }
      else
      {
        if ( rc == SSL_ERROR_SYSCALL && ret == -1 && eWB(errno) )
          rc = 0 ; 
        else
        {
          cInfo->sslInfo->func = "SSL_read" ;
          sslTraceErr(cInfo, rc, __FILE__, __LINE__);
          snprintf(errMsg,ERR_MSG_SIZE,"SockMgm(%s):  conn_read failed: ret=%d, SSL_rc=%d, errno=%d (%s)",__func__, ret, rc, errno, strerror(errno));
          rc = -1 ; 
        }
        break ; 
      }
    }
    pthread_mutex_unlock(cInfo->sslInfo->lock) ; 
    return rc ; 
  }
 #endif

  ret = read(cInfo->sfd, buf, len  ) ;
  if ( ret > 0 ) return ret ; 
  if ( ret < 0 )
  {
    int rc = rmmErrno ; 
    if ( eWB(rc) )
      return 0 ; 
    else
    {
      *errCode = rc ;
      snprintf(errMsg,ERR_MSG_SIZE,"SockMgm(%s):  conn_read failed: %d (%s)",__func__, *errCode,rmmErrStr(*errCode));
      return -1 ; 
    }
  }
  if ( len <= 0 )
    return 0 ; 
  snprintf(errMsg,ERR_MSG_SIZE,"SockMgm(%s):  conn_read failed: nget=0 => EOF detected => other side closed connection.",__func__);
  return -2 ; 
}

/******************************************************************/
/******************************************************************/

int rmmGetNameInfo(SA *sa, char *addr, size_t a_len, char *port, size_t p_len, int *errCode, char *errMsg)
{
  int af , rc , iport;
  SA4 *sa4 ;
  SA6 *sa6 ;
  void *a_p ;

  *errCode = 0 ;
  if ( addr && a_len>0 ) addr[0] = 0 ; 
  else a_len = 0 ; 
  if ( port && p_len>0 ) port[0] = 0 ; 
  else p_len = 0 ; 

  af = sa->sa_family ;
  if ( af == AF_INET )
  {
    sa4 = (SA4 *)sa ;
    a_p = &sa4->sin_addr ;
    iport = ntohs(sa4->sin_port) ;
  }
  else
  if ( af == AF_INET6 )
  {
    sa6 = (SA6 *)sa ;
    a_p = &sa6->sin6_addr ;
    iport = ntohs(sa6->sin6_port) ;
  }
  else
  {
   #ifdef EAFNOSUPPORT
    *errCode = EAFNOSUPPORT ;
   #endif
    snprintf(errMsg,ERR_MSG_SIZE," Failed to convert address, unknown af: %d ",af) ;
    return RMM_FAILURE ;
  }

  if ( port!=NULL && p_len>0 )
  {
    snprintf(port,p_len,"%u",iport) ;
    port[p_len-1] = 0 ;
  }
  rc = rmm_ntop(af, a_p, addr, a_len) ;
  if ( rc != RMM_SUCCESS )
  {
    *errCode = rmmErrno ;
    snprintf(errMsg,ERR_MSG_SIZE," Failed to convert address using rmm_ntop, error %d (%s)",*errCode,rmmErrStr(*errCode)) ;
    return RMM_FAILURE ;
  }
  else
    return RMM_SUCCESS ;
}

/******************************************************************/
/******************************************************************/
/******************************************************************/
static int IN4_IS_ADDR_LINKLOCAL(const void *a)
{
  unsigned char *v = (unsigned char *)a ;
  return (v[0]==169 && v[1]==254);
}
/******************************************************************/
int llm_addr_scope(const void *a, int l)
{
  if ( !a ) return 0 ; 
  if ( l == sizeof(IA4) )
  {
    int i ; 
    unsigned char *v = (unsigned char *)a ;
    if ( v[0] == 0x7f ) return 1 ;                           /* loopback */
    if ( IN4_IS_ADDR_LINKLOCAL(a) ) return 2 ;               /* 169.254.x.x */
    if ( v[0] == 0x0a ) return 3 ;                           /* 10.x.x.x    */
    if ( v[0] == 0xac && (v[1]&0xf0) == 0x10 ) return 3 ;    /* 172.16.x.x-172.31.x.x */ 
    if ( v[0] == 192 && v[1] == 168 ) return 3 ;             /* 192.168.x.x */
    memcpy(&i,v,4) ; 
    if ( i == 0 ) return 0 ; 
    return 4 ; 
  }
  if ( l == sizeof(IA6) )
  {
    const IA6 *v = (IA6 *)a ; 
    if ( IN6_IS_ADDR_UNSPECIFIED(v) ) return 0 ; 
    if ( IN6_IS_ADDR_LOOPBACK(v)    ) return 1 ; 
    if ( IN6_IS_ADDR_LINKLOCAL(v)   ) return 2 ; 
    if ( IN6_IS_ADDR_SITELOCAL(v)   ) return 3 ; 
    return 4 ; 
  }
  return 0 ; 
}
/******************************************************************/

static int is_ifi_ok(ifInfo *cur_ifi, ifInfo *tmp_ifi, ipFlat *all_ips, int nall, int req_af, int req_af0, TBHandle tbHandle)
{
  int ia ;

  if ( IF_HAVE_GLOB(cur_ifi) && (IN6_IS_ADDR_V4MAPPED(&cur_ifi->glob_addr) || IN6_IS_ADDR_V4COMPAT(&cur_ifi->glob_addr)) )
  {
    for ( ia = 0 ; ia<nall ; ia++ )
    {
      if ( all_ips[ia].length != sizeof(IA6) ) continue ;
      if ( IN6_IS_ADDR_LINKLOCAL(&all_ips[ia].ia6) ) continue ;
      if ( IN6_IS_ADDR_SITELOCAL(&all_ips[ia].ia6) ) continue ;
      if ( IN6_IS_ADDR_V4MAPPED( &all_ips[ia].ia6) ) continue ;
      if ( IN6_IS_ADDR_V4COMPAT( &all_ips[ia].ia6) ) continue ;
      memcpy(&cur_ifi->glob_addr,all_ips[ia].bytes,sizeof(IA6)) ;
      break ;
    }
  }

  if ( IF_HAVE_IPV4(tmp_ifi) && memcmp(&tmp_ifi->ipv4_addr,&cur_ifi->ipv4_addr,sizeof(IA4)) )
  {
    for ( ia = 0 ; ia<nall ; ia++ )
    {
      if ( all_ips[ia].length != sizeof(IA4) ) continue ;
      if ( memcmp(&tmp_ifi->ipv4_addr,all_ips[ia].bytes,sizeof(IA4)) ) continue ;
      memcpy(&cur_ifi->ipv4_addr,all_ips[ia].bytes,sizeof(IA4)) ;
      break ;
    }
    if ( ia >= nall )
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_5.0: ipv4 address mismatch\n")) ;
      return 0 ;
    }
  }
  if ( IF_HAVE_LINK(tmp_ifi) && memcmp(&tmp_ifi->link_addr,&cur_ifi->link_addr,sizeof(IA6)) )
  {
    for ( ia = 0 ; ia<nall ; ia++ )
    {
      if ( all_ips[ia].length != sizeof(IA6) ) continue ;
      if (!IN6_IS_ADDR_LINKLOCAL(&all_ips[ia].ia6) ) continue ;
      if ( memcmp(&tmp_ifi->link_addr,all_ips[ia].bytes,sizeof(IA6)) ) continue ;
      memcpy(&cur_ifi->link_addr,all_ips[ia].bytes,sizeof(IA6)) ;
      break ;
    }
    if ( ia >= nall )
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_5.1: link address mismatch\n")) ;
      return 0 ;
    }
  }
  if ( IF_HAVE_SITE(tmp_ifi) && memcmp(&tmp_ifi->site_addr,&cur_ifi->site_addr,sizeof(IA6)) )
  {
    for ( ia = 0 ; ia<nall ; ia++ )
    {
      if ( all_ips[ia].length != sizeof(IA6) ) continue ;
      if (!IN6_IS_ADDR_SITELOCAL(&all_ips[ia].ia6) ) continue ;
      if ( memcmp(&tmp_ifi->site_addr,all_ips[ia].bytes,sizeof(IA6)) ) continue ;
      memcpy(&cur_ifi->site_addr,all_ips[ia].bytes,sizeof(IA6)) ;
      break ;
    }
    if ( ia >= nall )
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_5.2: site address mismatch\n")) ;
      return 0 ;
    }
  }
  if ( IF_HAVE_GLOB(tmp_ifi) && memcmp(&tmp_ifi->glob_addr,&cur_ifi->glob_addr,sizeof(IA6)) )
  {
    for ( ia = 0 ; ia<nall ; ia++ )
    {
      if ( all_ips[ia].length != sizeof(IA6) ) continue ;
      if ( IN6_IS_ADDR_LINKLOCAL(&all_ips[ia].ia6) ) continue ;
      if ( IN6_IS_ADDR_SITELOCAL(&all_ips[ia].ia6) ) continue ;
      if ( memcmp(&tmp_ifi->glob_addr,all_ips[ia].bytes,sizeof(IA6)) ) continue ;
      memcpy(&cur_ifi->glob_addr,all_ips[ia].bytes,sizeof(IA6)) ;
      break ;
    }
    if ( ia >= nall )
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_5.3: glob address mismatch\n")) ;
      return 0 ;
    }
  }

  if ( !tmp_ifi->have_addr && IF_HAVE_IPV4(cur_ifi) && IN4_IS_ADDR_LINKLOCAL(&cur_ifi->ipv4_addr) )
  {
    TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_5.4: ipv4 addr is linklocal (169.254.x.x)\n")) ;
    return 0 ;
  }

  if ( !cur_ifi->have_addr ||
      (req_af==AF_INET  && !IF_HAVE_IPV4(cur_ifi)) ||
      (req_af==AF_INET6 && !IF_HAVE_IPV6(cur_ifi)) ||
      (req_af0==RMM_IPver_BOTH && (!IF_HAVE_IPV4(cur_ifi) || !IF_HAVE_IPV6(cur_ifi)) ) )
  {
    TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_6: no addresses found!\n")) ;
    return 0 ;
  }

  return 1 ;
}
/******************************************************************/
/******************************************************************/
/******************************************************************/

/******************************************************************/

int rmmGetLocalIf(char *try_this, int use_ib, int no_any, int req_mc, int req_af0, ifInfo *localIf, int *errCode, char *errMsg, TBHandle tbHandle)
{
  int iok , req_af , af ;
  int lo_ok=0 , iTTT;
  ifInfo tmp_ifi, *cur_ifi;
  ipFlat tmp_ipf ;
  ipFlat all_ips[256] ;
  int nall;
  char delim[256] ;
  char tmpstr[RMM_HOSTNAME_STR_LEN] , cmd[RMM_HOSTNAME_STR_LEN] ;
  uint8_t *p ;
  SOCKET sfd4=0, sfd6=0 ;
  int rc ;
  struct if_nameindex *ifNI0, *ifNI ;
  struct ifreq        tifr ;
  struct ifreq        *qIfr ;
#ifdef SIOCGLIFADDR
  #define LIFREQ    lifreq
  #define LIFCONF   lifconf
  #define LIFC_LEN  lifc_len
  #define LIFC_BUF  lifc_buf
  #define LIFC_REQ  lifc_req
  #define LIFC_FAM  lifc_family
  #define LIFR_ADDR lifr_addr
  #define LIFR_NAME lifr_name
  #define LIFR_FLAGS lifr_flags
  #define LIFR_INDEX lifr_index
  char                 lIfcBuf[256*sizeof(struct LIFREQ)] ;
  struct LIFCONF        lIfc ;
  struct LIFREQ        tlifr ;
  struct LIFREQ       *qlIfr ;
  struct LIFREQ        *pIfr ;
  struct LIFREQ        *eIfr ;
#else
  FILE *fp ;
  char line[RMM_HOSTNAME_STR_LEN] ;
#endif
  char myName[] = {"rmmGetLocalIf"} ;

   TRDBG(snprintf(traceMsg, sizeof(traceMsg),"%s: entering get_local_if...%s uib %d\n",myName, (try_this)?try_this:"nil",use_ib)) ;

  *errCode = 0 ;
  snprintf(errMsg,ERR_MSG_SIZE,"%s","_not_set_yet_") ;
  cmd[0] = 0 ;

   if ( req_af0 == RMM_IPver_IPv4 )
        req_af  =  AF_INET   ;
   else
   if ( req_af0 == RMM_IPver_IPv6 )
        req_af  =  AF_INET6  ;
   else
        req_af  =  AF_UNSPEC ;

  memset(delim,0,sizeof(delim)) ;
  delim[0] = delim[' '] = delim['\t'] = delim['\n'] = delim['/'] = delim['%'] = 1 ;
  memset(&tmp_ifi,0,sizeof(tmp_ifi)) ;
  if ( try_this == NULL || try_this[0] == 0 )
    strlcpy(tmpstr,"NONE",RMM_HOSTNAME_STR_LEN) ;
  else
  {
    strlcpy(tmpstr,try_this,RMM_HOSTNAME_STR_LEN) ;
    strip(tmpstr) ;
  }
  strlcpy(cmd,tmpstr,RMM_HOSTNAME_STR_LEN) ;
  upper(cmd) ;
  if ( strcmp(cmd,"LOOPBACK") == 0 || strcmp(cmd,"LOCALHOST") == 0 ||
      (req_af != AF_INET  && strcmp(tmpstr,"::1") == 0) ||
      (req_af != AF_INET6 && strcmp(tmpstr,"127.0.0.1") == 0) )
  {
    lo_ok = 1 ;
    req_mc = 0 ;
    if ( req_af == AF_INET6 )
      strlcpy(tmpstr,"::1",RMM_HOSTNAME_STR_LEN) ;
    else
      strlcpy(tmpstr,"127.0.0.1",RMM_HOSTNAME_STR_LEN) ;
  }
  if ( no_any || (strcmp(cmd,"NONE") && strcmp(cmd,"ANY") && strcmp(cmd,"ALL")) )
  {
    TRDBG(snprintf(traceMsg, sizeof(traceMsg)," Checking whether |%s| is an address\n",tmpstr)) ;
    if ( (af=rmmGetAddr(AF_UNSPEC,tmpstr,&tmp_ipf)) != AF_UNSPEC )
    {
      if ( af != req_af && req_af != AF_UNSPEC )
      {
        *errCode = 999 ;
        snprintf(errMsg,ERR_MSG_SIZE," Required interface (%s) is inconsistent with required IPversion (%s)",
                tmpstr,(req_af == AF_INET6)?"IPv6":"IPv4") ;
        return RMM_FAILURE ;
      }
      iTTT = TRY_THIS_ADDR ; /* try_this is an address */
      if ( af == AF_INET )
      {
        memcpy(&tmp_ifi.ipv4_addr,tmp_ipf.bytes,sizeof(IA4)) ; /* BEAM suppression: uninitialized */
        tmp_ifi.have_addr |= 1 ;
       #if TRACE_DEBUG
        rmm_ntop(AF_INET ,&tmp_ifi.ipv4_addr,tmpstr,sizeof(tmpstr)) ;
        TRDBG(snprintf(traceMsg, sizeof(traceMsg),"              ipv4: %s\n",tmpstr)) ;
       #endif
      }
      else
      if ( IN6_IS_ADDR_LINKLOCAL(&tmp_ipf.ia6) )  /* BEAM suppression: uninitialized */
      {
        memcpy(&tmp_ifi.link_addr,tmp_ipf.bytes,sizeof(IA6)) ;
        tmp_ifi.have_addr |= 2 ;
       #if TRACE_DEBUG
        rmm_ntop(AF_INET6,&tmp_ifi.link_addr,tmpstr,sizeof(tmpstr)) ;
        TRDBG(snprintf(traceMsg, sizeof(traceMsg),"              link: %s\n",tmpstr)) ;
       #endif
      }
      else
      if ( IN6_IS_ADDR_SITELOCAL(&tmp_ipf.ia6) )
      {
        memcpy(&tmp_ifi.site_addr,tmp_ipf.bytes,sizeof(IA6)) ;
        tmp_ifi.have_addr |= 4 ;
       #if TRACE_DEBUG
        rmm_ntop(AF_INET6,&tmp_ifi.site_addr,tmpstr,sizeof(tmpstr)) ;
        TRDBG(snprintf(traceMsg, sizeof(traceMsg),"              site: %s\n",tmpstr)) ;
       #endif
      }
      else
      {
        memcpy(&tmp_ifi.glob_addr,tmp_ipf.bytes,sizeof(IA6)) ;
        tmp_ifi.have_addr |= 8 ;
       #if TRACE_DEBUG
        rmm_ntop(AF_INET6,&tmp_ifi.glob_addr,tmpstr,sizeof(tmpstr)) ;
        TRDBG(snprintf(traceMsg, sizeof(traceMsg),"              glob: %s\n",tmpstr)) ;
       #endif
      }
    }
    else
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg)," Checking whether |%s| is an interface index\n",tmpstr)) ;
      p = (uint8_t *)tmpstr ;
      while ( isdigit(*p) ) p++ ;
      if ( delim[*p] )
      {
        iTTT = TRY_THIS_INDEX ; /* try_this is an index */
        tmp_ifi.index = rmm_atoi(tmpstr) ;
        TRDBG(snprintf(traceMsg, sizeof(traceMsg)," rmmGetAddrInfo:  tmp_ifi.index: %d\n",tmp_ifi.index)) ;
      }
      else
      {
        iTTT = TRY_THIS_NAME ; /* try_this is a name */
        TRDBG(snprintf(traceMsg, sizeof(traceMsg)," Checking whether |%s| is an interface name\n",tmpstr)) ;
        strlcpy(tmp_ifi.name,tmpstr,sizeof(tmp_ifi.name)) ;
        TRDBG(snprintf(traceMsg, sizeof(traceMsg)," rmmGetAddrInfo:  tmp_ifi.name: %s\n",tmp_ifi.name)) ;
      }
    }
  }
  else
  {
    if ( strcmp(cmd,"ALL") == 0 )
      iTTT = TRY_THIS_ALL ; /* try_this is ALL */
    else
      iTTT = TRY_THIS_NONE; /* try_this is "NONE" or "ANY" */
  }

#ifdef OS_Linuxx
{
 #include <ifaddrs.h>
  struct ifaddrs *Ifaddr , *savptr ;
  if ( getifaddrs(&Ifaddr) == 0 )
  {
    int ec;
    char port[8] , addr[64] ;
    SA *sa;
    savptr = Ifaddr ;
    while ( Ifaddr )
    {
      sa = Ifaddr->ifa_addr ;
      if ( !sa ) continue ; 
      strlcpy(addr,"rmmGetNameInfo failed",64) ; 
      rmmGetNameInfo(sa,addr,64,port,8,&ec,tmpstr) ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"rmmGetAddrInfo: getifaddrs_: name: %s , addr: %d %d %d %s\n",Ifaddr->ifa_name,sa->sa_family,AF_INET,AF_INET6,addr));
      Ifaddr = Ifaddr->ifa_next ;
    }
    freeifaddrs(savptr);
  }
}
#endif /* of ifdef OS_Linuxx */

  do
  {
    sfd4 = socket_(AF_INET , SOCK_DGRAM, 0) ;
    sfd6 = socket_(AF_INET6, SOCK_DGRAM, 0) ;
    TRDBG(snprintf(traceMsg, sizeof(traceMsg),"%d: sfd4= %d\n",__LINE__,sfd4));
    TRDBG(snprintf(traceMsg, sizeof(traceMsg),"%d: sfd6= %d\n",__LINE__,sfd6));
    if ( sfd4 < 0 && sfd6 < 0 )
    {
      *errCode = rmmErrno ;
      snprintf(errMsg,ERR_MSG_SIZE," Failed to create temp socket (rc=%d)",rmmErrno) ;
      return RMM_FAILURE ;
    }

    iok = 0 ;
    qIfr = &tifr ;
    ifNI0 = ifNI = if_nameindex() ;
    if (!ifNI0 )
    {
      *errCode = rmmErrno ;
      snprintf(errMsg,ERR_MSG_SIZE," if_nameindex() failed (rc=%d %s)",rmmErrno,rmmErrStr(rmmErrno)) ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"if_nameindex() failed (rc=%d %s)\n",rmmErrno,rmmErrStr(rmmErrno)));
    }

    /*@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

    cur_ifi = localIf ;
    for ( ; ifNI && ifNI->if_index  ; ifNI++ )
    {
      nall = 0 ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"checking interface ->  name: %s , index: %d \n",ifNI->if_name,ifNI->if_index)) ;
      if ( tmp_ifi.name[0] )
      {
        strlcpy(tmpstr,ifNI->if_name,RMM_HOSTNAME_STR_LEN) ;
        upper(strip(tmpstr)) ;
        strlcpy(cmd,tmp_ifi.name,RMM_HOSTNAME_STR_LEN) ;
        upper(strip(cmd)) ;
        if ( strcmp(cmd,tmpstr) )
        {
          TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_1: name mismatch: _%s_%s_\n",cmd,tmpstr)) ;
          continue ;
        }
      }
      if ( tmp_ifi.index && tmp_ifi.index != ifNI->if_index )
      {
        TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_2: index mismatch: _%d_%d_\n",tmp_ifi.index,ifNI->if_index)) ;
        continue ;
      }

      memset(cur_ifi,0,sizeof(ifInfo)) ;
      strlcpy(cur_ifi->name,ifNI->if_name,IF_NAMESIZE) ;
      strlcpy(cur_ifi->user_name,try_this,RMM_HOSTNAME_STR_LEN) ;
      cur_ifi->index = ifNI->if_index ;
      rc = 1 ;
#ifdef SIOCGLIFADDR
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"trying to obtain if_flags using ioctl(sfd, SIOCGLIFFLAGS, qlIfr)...\n")) ;
      qlIfr = &tlifr ;
      strlcpy(qlIfr->LIFR_NAME,ifNI->if_name,IF_NAMESIZE) ;
      if ((sfd6==-1 || ioctl(sfd6, SIOCGLIFFLAGS, qlIfr) < 0) &&
          (sfd4==-1 || ioctl(sfd4, SIOCGLIFFLAGS, qlIfr) < 0) )
      {
        TRDBG(snprintf(traceMsg, sizeof(traceMsg),"could not obtain if_flags using ioctl(sfd, SIOCGLIFFLAGS, qlIfr), rc= %d (%s)\n",errno,rmmErrStr(errno))) ;
      }
      else
      {
        if ( !(qlIfr->LIFR_FLAGS & IFF_UP) ||
             ( req_mc && !(qlIfr->LIFR_FLAGS & IFF_MULTICAST)) ||
             (!lo_ok && (qlIfr->LIFR_FLAGS & IFF_LOOPBACK)) )
        {
          TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_3: flags mismatch\n")) ;
          continue ;
        }
        cur_ifi->flags = qlIfr->LIFR_FLAGS ;
        rc = 0 ;
      }
#endif  /* of #ifdef SIOCGLIFADDR  */
      if ( rc )
      {
        TRDBG(snprintf(traceMsg, sizeof(traceMsg),"trying to obtain if_flags using ioctl(sfd, SIOCGIFFLAGS, qIfr)...\n")) ;
        rc=strlcpy(qIfr->ifr_name,ifNI->if_name,IF_NAMESIZE) ;
        if ((sfd6==-1 || ioctl(sfd6, SIOCGIFFLAGS, qIfr) < 0) &&
            (sfd4==-1 || ioctl(sfd4, SIOCGIFFLAGS, qIfr) < 0) )
        {
          TRDBG(snprintf(traceMsg, sizeof(traceMsg),"could not obtain if_flags using ioctl(sfd, SIOCGIFFLAGS, qIfr), rc= %d (%s)\n",errno,rmmErrStr(errno))) ;
          TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_4.0: no flags\n")) ;
          continue ;
        }
        if ( !(qIfr->ifr_flags & IFF_UP) ||
             ( req_mc && !(qIfr->ifr_flags & IFF_MULTICAST)) ||
              (!lo_ok && (qIfr->ifr_flags & IFF_LOOPBACK)) )
        {
          TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_4: flags mismatch\n")) ;
          continue ;
        }
        cur_ifi->flags = qIfr->ifr_flags ;
      }

#ifdef SIOCGLIFADDR
    do
    {
      SA  *sa ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"trying to obtain addresses using ioctl(sfd, SIOCGLIFCONF, &lIfc)...\n")) ;
      lIfc.LIFC_FAM = req_af ;
      lIfc.lifc_flags  = 0 ;
      lIfc.LIFC_LEN    = sizeof(lIfcBuf);
      lIfc.LIFC_BUF = (char *)lIfcBuf;
      if ((sfd6==-1 || ioctl(sfd6, SIOCGLIFCONF, &lIfc) < 0) &&
          (sfd4==-1 || ioctl(sfd4, SIOCGLIFCONF, &lIfc) < 0) )
      {
        TRDBG(snprintf(traceMsg, sizeof(traceMsg),"could not obtain addresses using ioctl(sfd, SIOCGLIFCONF, &lIfc)...\n")) ;
        break ;
      }
      pIfr = lIfc.LIFC_REQ ;
      eIfr = (struct LIFREQ *)(lIfc.LIFC_BUF+lIfc.LIFC_LEN) ;
      for ( ; pIfr<eIfr ; pIfr++ )
      {
        memcpy(eIfr,pIfr,sizeof(struct LIFREQ)) ;
        if ((sfd6==-1 || ioctl(sfd6, SIOCGLIFINDEX, eIfr) < 0) &&
            (sfd4==-1 || ioctl(sfd4, SIOCGLIFINDEX, eIfr) < 0) )
          continue ;
        if (  eIfr->LIFR_INDEX != cur_ifi->index  )
          continue ;

        memcpy(eIfr,pIfr,sizeof(struct LIFREQ)) ;
        sa = (SA *)&eIfr->LIFR_ADDR ;
        if ( sa->sa_family == AF_INET && req_af != AF_INET6 )
        {
          SA4 *sa4;
          sa4 = (SA4 *)sa ;
          memcpy(&cur_ifi->ipv4_addr,&sa4->sin_addr,sizeof(IA4)) ;
          cur_ifi->have_addr |= 1 ;
          memcpy(all_ips[nall].bytes,&sa4->sin_addr,sizeof(IA4)) ;
          all_ips[nall++].length = sizeof(IA4) ;
        }
        else
        if ( sa->sa_family == AF_INET6 && req_af != AF_INET  )
        {
          SA6 *sa6;
          IA6 *ia6 ;
          sa6 = (SA6 *)sa ;
          ia6 = &sa6->sin6_addr ;
          if ( IN6_IS_ADDR_LINKLOCAL(ia6) )
          {
            memcpy(&cur_ifi->link_addr,ia6,sizeof(IA6)) ;
            cur_ifi->link_scope = sa6->sin6_scope_id ;
            cur_ifi->have_addr |= 2 ;
          }
          else
          if ( IN6_IS_ADDR_SITELOCAL(ia6) )
          {
            memcpy(&cur_ifi->site_addr,ia6,sizeof(IA6)) ;
            cur_ifi->site_scope = sa6->sin6_scope_id ;
            cur_ifi->have_addr |= 4 ;
          }
          else
          {
            memcpy(&cur_ifi->glob_addr,ia6,sizeof(IA6)) ;
            cur_ifi->glob_scope = sa6->sin6_scope_id ;
            cur_ifi->have_addr |= 8 ;
          }
          memcpy(all_ips[nall].bytes,ia6,sizeof(IA6)) ;
          all_ips[nall++].length = sizeof(IA6) ;
        }
      }
    } while ( 0 ) ;
#else
    {
     #include <ifaddrs.h>
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"trying to obtain addresses using getifaddrs(&Ifaddr)...\n")) ;
      struct ifaddrs *Ifaddr , *savptr ;
      if ( getifaddrs(&Ifaddr) == 0 )
      {
        SA  *sa ;
        struct ifreq  pIfr[1] ;
        char addr[64], port[8];
        int ec, idx ; 
        size_t nl ; 
        nl = strllen(cur_ifi->name,IF_NAMESIZE) ;
        savptr = Ifaddr ;
        for ( ; Ifaddr ; Ifaddr = Ifaddr->ifa_next )
        {
          strlcpy(pIfr->ifr_name,Ifaddr->ifa_name,IF_NAMESIZE) ;
          if ( ioctl(sfd4,SIOCGIFINDEX, pIfr) < 0 )
          {
            TRDBG(snprintf(traceMsg, sizeof(traceMsg),"rmmGetAddrInfo:  ioctl(sfd, SIOCGIFINDEX, qIfr) failed for %s, sfd=%d ,errno= %d\n",pIfr->ifr_name,sfd4,errno)) ;
            continue ;
          }
          idx = pIfr->ifr_ifindex ; 
          if ( cur_ifi->index != idx &&
               memcmp(cur_ifi->name,Ifaddr->ifa_name,nl+1) )
          {
            TRDBG(snprintf(traceMsg, sizeof(traceMsg),"diff name/index: %s %s %d %d\n",Ifaddr->ifa_name,cur_ifi->name,idx, cur_ifi->index)) ;
            continue ;
          }
          sa = (SA *)Ifaddr->ifa_addr ;
          if ( !sa )
          {
            TRDBG(snprintf(traceMsg, sizeof(traceMsg),"ifa_addr is NULL for %s\n",Ifaddr->ifa_name)) ;
            continue ; 
          }
          rmmGetNameInfo(sa,addr,64,port,8,&ec,tmpstr) ;
          TRDBG(snprintf(traceMsg, sizeof(traceMsg),"got ifa_addr->sa_family %d : %s %d\n",sa->sa_family,addr,idx)) ;
          if ( sa->sa_family == AF_INET && req_af != AF_INET6 )
          {
            SA4 *sa4;
            sa4 = (SA4 *)sa ;
            memcpy(&cur_ifi->ipv4_addr,&sa4->sin_addr,sizeof(IA4)) ;
            cur_ifi->have_addr |= 1 ;
            memcpy(all_ips[nall].bytes,&sa4->sin_addr,sizeof(IA4)) ;
            all_ips[nall++].length = sizeof(IA4) ;
          }
          else
          if ( sa->sa_family == AF_INET6 && req_af != AF_INET  )
          {
            SA6 *sa6;
            IA6 *ia6 ;
            sa6 = (SA6 *)sa ;
            ia6 = &sa6->sin6_addr ;
            if ( IN6_IS_ADDR_LINKLOCAL(ia6) )
            {
              memcpy(&cur_ifi->link_addr,ia6,sizeof(IA6)) ;
              cur_ifi->link_scope = sa6->sin6_scope_id ;
              cur_ifi->have_addr |= 2 ;
            }
            else
            if ( IN6_IS_ADDR_SITELOCAL(ia6) )
            {
              memcpy(&cur_ifi->site_addr,ia6,sizeof(IA6)) ;
              cur_ifi->site_scope = sa6->sin6_scope_id ;
              cur_ifi->have_addr |= 4 ;
            }
            else
            {
              memcpy(&cur_ifi->glob_addr,ia6,sizeof(IA6)) ;
              cur_ifi->glob_scope = sa6->sin6_scope_id ;
              cur_ifi->have_addr |= 8 ;
            }
            memcpy(all_ips[nall].bytes,ia6,sizeof(IA6)) ;
            all_ips[nall++].length = sizeof(IA6) ;
          }
        }
        freeifaddrs(savptr);
      }
    }
#endif
      if ( req_af != AF_INET && !IF_HAVE_IPV6(cur_ifi) )
      {
        int err;
        TRDBG(snprintf(traceMsg, sizeof(traceMsg),"trying to obtain ipv6 addresses using /proc/net/if_inet6...\n")) ;
        strlcpy(cmd,"/proc/net/if_inet6",RMM_HOSTNAME_STR_LEN) ;
        if ( (fp=llm_fopen(cmd,"r",&err)) )
        {
          union
          {
          IA6 ia6;
            unsigned char ad6[16];
          } u ; 
          unsigned i,n, ind , scope ;
          while ( fgets(line,RMM_HOSTNAME_STR_LEN,fp) )
          {
            sscanf(line+33,"%2x",&ind) ;
            if ( ind != cur_ifi->index )
              continue ;
            for ( i=0 ; i<16 ; i++ )
            {
              sscanf(line+(2*i),"%2x",&n) ;
              u.ad6[i] = n ;
            }
            sscanf(line+39,"%2x",&scope) ;
            if ( IN6_IS_ADDR_LINKLOCAL(&u.ia6) )
            {
              memcpy(&cur_ifi->link_addr,u.ad6,sizeof(struct in6_addr)) ;
              cur_ifi->link_scope = scope ;
              cur_ifi->have_addr |= 2 ;
            }
            else
            if ( IN6_IS_ADDR_SITELOCAL(&u.ia6) )
            {
              memcpy(&cur_ifi->site_addr,u.ad6,sizeof(struct in6_addr)) ;
              cur_ifi->site_scope = scope ;
              cur_ifi->have_addr |= 4 ;
            }
            else
            {
              memcpy(&cur_ifi->glob_addr,u.ad6,sizeof(struct in6_addr)) ;
              cur_ifi->glob_scope = scope ;
              cur_ifi->have_addr |= 8 ;
            }
            memcpy(all_ips[nall].bytes,u.ad6,sizeof(IA6)) ;
            all_ips[nall++].length = sizeof(IA6) ;
          }
          fclose(fp) ;
        }
      }
      if ( !IF_HAVE_IPV4(cur_ifi) && req_af != AF_INET6 )
      {
        TRDBG(snprintf(traceMsg, sizeof(traceMsg),"trying to obtain ipv4 addresses using ioctl(sfd, SIOCGIFADDR, qIfr)...\n")) ;
        rc=strlcpy(qIfr->ifr_name,ifNI->if_name,IF_NAMESIZE) ;
        if (ioctl(sfd4,SIOCGIFADDR, qIfr) < 0 &&
            ioctl(sfd6,SIOCGIFADDR, qIfr) < 0)
        {
          TRDBG(snprintf(traceMsg, sizeof(traceMsg),"could not obtain ipv4 addresses using ioctl(sfd, SIOCGIFADDR, qIfr), rc= %d (%s)\n",errno,rmmErrStr(errno))) ;
        }
        else
        {
          if ( qIfr->ifr_addr.sa_family == AF_INET )
          {
            SA4 *sa4 ;
            sa4 = (SA4 *)&qIfr->ifr_addr ;
            memcpy(&cur_ifi->ipv4_addr,&(sa4->sin_addr),sizeof(struct in_addr)) ;
            cur_ifi->have_addr |= 1 ;
            memcpy(all_ips[nall].bytes,&(sa4->sin_addr),sizeof(IA4)) ;
            all_ips[nall++].length = sizeof(IA4) ;
          }
        }
      }
      if ( !is_ifi_ok(cur_ifi, &tmp_ifi, all_ips, nall, req_af, req_af0,tbHandle) )
        continue ;

      cur_ifi->af_req = req_af ;
      iok = 1 ;
        break ;
    }
    if ( ifNI0 )
      if_freenameindex(ifNI0)  ;
  } while ( 0 ) ;

  if ( sfd6 > 0 )
    closesocket(sfd6);
  if ( sfd4 > 0 )
    closesocket(sfd4);

  if ( !iok )
  {
    if ( strcmp(errMsg,"_not_set_yet_") == 0 )
      snprintf(errMsg,ERR_MSG_SIZE," rmmGetLocalIf: failed to obtain local NIC for %s",(try_this)?try_this:"") ;
    return RMM_FAILURE ;
  }

  cur_ifi=localIf ;
  {
    if ( IF_HAVE_IPV4(cur_ifi) )
      cur_ifi->af_req = AF_INET ;
    else
      cur_ifi->af_req = AF_INET6;

    if ( IF_HAVE_IPV4(cur_ifi) && IF_HAVE_IPV6(cur_ifi) )
      cur_ifi->have_af = AF_UNSPEC ;
    else
    if ( IF_HAVE_IPV4(cur_ifi) )
      cur_ifi->have_af = AF_INET ;
    else
      cur_ifi->have_af = AF_INET6;
  }

#if TRACE_DEBUG
  TRDBG(snprintf(traceMsg, sizeof(traceMsg)," rmmGetLocalIf: chosen NIC:\n") );
  cur_ifi=localIf ;
  {
    TRDBG(snprintf(traceMsg, sizeof(traceMsg),"          name: %s\n",cur_ifi->name)) ;
    TRDBG(snprintf(traceMsg, sizeof(traceMsg),"          index: %d\n",cur_ifi->index)) ;

    if ( IF_HAVE_IPV4(cur_ifi) )
    {
      rmm_ntop(AF_INET ,&cur_ifi->ipv4_addr,tmpstr,sizeof(tmpstr)) ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"              ipv4: %s\n",tmpstr)) ;
    }
    if ( IF_HAVE_LINK(cur_ifi) )
    {
      rmm_ntop(AF_INET6,&cur_ifi->link_addr,tmpstr,sizeof(tmpstr)) ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"         ipv6_link: %s , scope=%u\n",tmpstr,cur_ifi->link_scope)) ;
    }
    if ( IF_HAVE_SITE(cur_ifi) )
    {
      rmm_ntop(AF_INET6,&cur_ifi->site_addr,tmpstr,sizeof(tmpstr)) ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"         ipv6_site: %s , scope=%u\n",tmpstr,cur_ifi->site_scope) );
    }
    if ( IF_HAVE_GLOB(cur_ifi) )
    {
      rmm_ntop(AF_INET6,&cur_ifi->glob_addr,tmpstr,sizeof(tmpstr)) ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"         ipv6_glob: %s , scope=%u\n",tmpstr,cur_ifi->glob_scope)) ;
    }
  }
#endif

  localIf->iTTT = iTTT ; 

  return RMM_SUCCESS ;
}

/****************************************************************/
/******************************************************************/
int rmmGetAddrInfo(char *addr0, char *port0, int family, int stype, int proto, int flags, AI *result, int *errCode, char *errMsg)
{
  int rc ;
  AI hints ;
  SA *tmpsa ;
  AI *res , *pai ;
  char port[16] , *pp ;
  char addr[RMM_ADDR_STR_LEN] ;
  char *pa ;

  *errCode = 0 ;

  if ( addr0 == NULL || addr0[0]==0 )
    pa = NULL ;
  else
  {
    strlcpy(addr,addr0,RMM_ADDR_STR_LEN);
    strip(upper(addr)) ;
    if ( (strcmp(addr,"NONE") && strcmp(addr,"ANY") && strcmp(addr,"ALL")) )
    {
      strlcpy(addr,addr0,RMM_ADDR_STR_LEN);
      pa = addr ;
    }
    else
      pa = NULL ;
  }

  if ( port0 == NULL || port0[0]==0 )
    pp = NULL ;
  else
  {
    strlcpy(port,port0,sizeof(port)) ;
    pp = port ;
  }
  if ( !pa && !(flags&AI_PASSIVE) )
  {
    snprintf(errMsg,ERR_MSG_SIZE,"Destination address must be fully specified (_%s_)",(addr0==NULL)?"null":addr0) ;
    *errCode = 0 ;
    return RMM_FAILURE ;
  }
  if ( !pa && !pp )
  {
    snprintf(errMsg,ERR_MSG_SIZE,"%s","Both host and serv are NULL") ;
    *errCode = EAI_NONAME ;
    return RMM_FAILURE ;
  }

  memset(&hints,0,sizeof(AI)) ;
  hints.ai_flags    = flags ;
  hints.ai_family   = family ;
  hints.ai_socktype = stype ;
  hints.ai_protocol = proto ;
  if ( (rc=getaddrinfo(pa,pp,&hints, &res)) != 0)
  {
    snprintf(errMsg,ERR_MSG_SIZE," getaddrinfo failed for _%s_%s_ (error=%s)\n",
      (addr0==NULL?"null":addr0),(port0==NULL?"null":port0),gai_strerror(rc)) ;
    *errCode = rc ;
    return RMM_FAILURE ;
  }
  if ( result->ai_addrlen < res->ai_addrlen )
  {
    snprintf(errMsg,ERR_MSG_SIZE," getaddrinfo: not enought space to hols result address! %d %d\n",
       (int)result->ai_addrlen , (int)res->ai_addrlen )  ;
    freeaddrinfo(res) ;
    *errCode = 0 ;
    return RMM_FAILURE ;
  }
  for ( pai=res ; pai ; pai=pai->ai_next )
  {
    if ( pai->ai_family != AF_INET &&
         pai->ai_family != AF_INET6 )
    {
      continue ;
    }
    if ( family != AF_UNSPEC &&
         pai->ai_family != family )
    {
      continue ;
    }
    tmpsa = result->ai_addr ;
    memcpy(result,pai,sizeof(AI)) ;
    result->ai_addr = tmpsa ;
    memcpy(result->ai_addr,pai->ai_addr,pai->ai_addrlen) ;
    break ;
  }
  if ( !pai )
  {
    snprintf(errMsg,ERR_MSG_SIZE," getaddrinfo failed? for _%s_%s_ (error=%s)\n",
      (addr==NULL?"":addr),(port==NULL?"":port),gai_strerror(rc)) ;
    freeaddrinfo(res) ;
    *errCode = 0 ;
    return RMM_FAILURE ;
  }

  if ( result->ai_family != result->ai_addr->sa_family )
  {
    result->ai_addr->sa_family = result->ai_family ;
  }

  freeaddrinfo(res) ;

  return RMM_SUCCESS ;
}

/****************************************************************/

/****************************************************************/
/******************************************************************/
int rmmGetAddr(int af, const char *src, ipFlat *ip)
{
  if ( !src )
    return AF_UNSPEC ;

  if ( af == AF_INET )
  {
    if ( rmm_pton(AF_INET,src,ip->bytes) == RMM_SUCCESS )
    {
      ip->length = sizeof(IA4) ;
      return AF_INET ;
    }
    else
      return AF_UNSPEC ;
  }
  else
  if ( af == AF_INET6 )
  {
    if ( rmm_pton(AF_INET6,src,ip->bytes) == RMM_SUCCESS )
    {
      ip->length = sizeof(IA6) ;
      return AF_INET6 ;
    }
    else
      return AF_UNSPEC ;
  }
  else
  {
    if ( rmm_pton(AF_INET,src,ip->bytes) == RMM_SUCCESS )
    {
      ip->length = sizeof(IA4) ;
      return AF_INET ;
    }
    else
    if ( rmm_pton(AF_INET6,src,ip->bytes) == RMM_SUCCESS )
    {
      ip->length = sizeof(IA6) ;
      return AF_INET6 ;
    }
    else
      return AF_UNSPEC ;
  }
}

int rmm_pton(int af, const char *src, void *dst)
{
  int rc ;

  if ( !src )
    return RMM_FAILURE ;
  rc = inet_pton(af, src, dst) ;
  if ( rc > 0 )
    return RMM_SUCCESS ;
  else
    return RMM_FAILURE ;
}
/******************************************************************/
int rmm_ntop(int af, void *src, char *dst, size_t size)
{
  if ( inet_ntop(af,src,dst,size) == NULL )
    return RMM_FAILURE ;
  else
    return RMM_SUCCESS ;
}

/******************************************************************/
/******************************************************************/
int rmm_set_nb(SOCKET fd, int *errCode, char *errMsg, const char *myName)
{

  int    ival ;
  if ( (int)(ival = fcntl(fd,F_GETFL)) == -1 )
  {
    *errCode = rmmErrno ;
    snprintf(errMsg,ERR_MSG_SIZE,"SockMgm(%s):  fcntl F_GETFL failed (fd=%d) with error %d (%s)",myName, fd, *errCode,rmmErrStr(*errCode));
    return RMM_FAILURE ;
  }

  if ( !(ival&O_NONBLOCK) && fcntl(fd,F_SETFL,ival|O_NONBLOCK) == -1 )
  {
    *errCode = rmmErrno ;
    snprintf(errMsg,ERR_MSG_SIZE,"SockMgm(%s):  fcntl F_SETFL failed with error %d (%s)",myName, *errCode,rmmErrStr(*errCode));
    return RMM_FAILURE ;
  }
  return RMM_SUCCESS ;
}
/******************************************************************/
/******************************************************************/

const char *rmmErrStr(int code)
{
  static char oor[32];
  if (!err2str_init )
    init_err2str() ;

  if (code >= 0 && code < MAX_ERR_STR && err2str[code] )
    return err2str[code] ;

  snprintf(oor,sizeof(oor),"Unrecognized (%d)",code) ; 
  return oor ; 
}

void add_err2str(int code, const char *txt)
{
  if ( (code >= 0 && code < MAX_ERR_STR) )
    err2str[code] = txt;
}

void init_err2str(void)
{
#ifdef ECLONEME
  add_err2str(ECLONEME,"ECLONEME");
#endif
#ifdef EDEADLOCK
  add_err2str(EDEADLOCK,"EDEADLOCK");
#endif
#ifdef EWOULDBLOCK
  add_err2str(EWOULDBLOCK,"EWOULDBLOCK");
#endif
#ifdef EDESTADDREQ
  add_err2str(EDESTADDREQ,"EDESTADDREQ");
#endif
#ifdef EPERM
  add_err2str(EPERM,"EPERM");
#endif
#ifdef ENOENT
  add_err2str(ENOENT,"ENOENT");
#endif
#ifdef ESRCH
  add_err2str(ESRCH,"ESRCH");
#endif
#ifdef EINTR
  add_err2str(EINTR,"EINTR");
#endif
#ifdef EIO
  add_err2str(EIO,"EIO");
#endif
#ifdef ENXIO
  add_err2str(ENXIO,"ENXIO");
#endif
#ifdef E2BIG
  add_err2str(E2BIG,"E2BIG");
#endif
#ifdef ENOEXEC
  add_err2str(ENOEXEC,"ENOEXEC");
#endif
#ifdef EBADF
  add_err2str(EBADF,"EBADF");
#endif
#ifdef ECHILD
  add_err2str(ECHILD,"ECHILD");
#endif
#ifdef EAGAIN
  add_err2str(EAGAIN,"EAGAIN");
#endif
#ifdef ENOMEM
  add_err2str(ENOMEM,"ENOMEM");
#endif
#ifdef EACCES
  add_err2str(EACCES,"EACCES");
#endif
#ifdef EFAULT
  add_err2str(EFAULT,"EFAULT");
#endif
#ifdef ENOTBLK
  add_err2str(ENOTBLK,"ENOTBLK");
#endif
#ifdef EBUSY
  add_err2str(EBUSY,"EBUSY");
#endif
#ifdef EEXIST
  add_err2str(EEXIST,"EEXIST");
#endif
#ifdef EXDEV
  add_err2str(EXDEV,"EXDEV");
#endif
#ifdef ENODEV
  add_err2str(ENODEV,"ENODEV");
#endif
#ifdef ENOTDIR
  add_err2str(ENOTDIR,"ENOTDIR");
#endif
#ifdef EISDIR
  add_err2str(EISDIR,"EISDIR");
#endif
#ifdef EINVAL
  add_err2str(EINVAL,"EINVAL");
#endif
#ifdef ENFILE
  add_err2str(ENFILE,"ENFILE");
#endif
#ifdef EMFILE
  add_err2str(EMFILE,"EMFILE");
#endif
#ifdef ENOTTY
  add_err2str(ENOTTY,"ENOTTY");
#endif
#ifdef ETXTBSY
  add_err2str(ETXTBSY,"ETXTBSY");
#endif
#ifdef EFBIG
  add_err2str(EFBIG,"EFBIG");
#endif
#ifdef ENOSPC
  add_err2str(ENOSPC,"ENOSPC");
#endif
#ifdef ESPIPE
  add_err2str(ESPIPE,"ESPIPE");
#endif
#ifdef EROFS
  add_err2str(EROFS,"EROFS");
#endif
#ifdef EMLINK
  add_err2str(EMLINK,"EMLINK");
#endif
#ifdef EPIPE
  add_err2str(EPIPE,"EPIPE");
#endif
#ifdef EDOM
  add_err2str(EDOM,"EDOM");
#endif
#ifdef ERANGE
  add_err2str(ERANGE,"ERANGE");
#endif
#ifdef EDEADLK
  add_err2str(EDEADLK,"EDEADLK");
#endif
#ifdef ENAMETOOLONG
  add_err2str(ENAMETOOLONG,"ENAMETOOLONG");
#endif
#ifdef ENOLCK
  add_err2str(ENOLCK,"ENOLCK");
#endif
#ifdef ENOSYS
  add_err2str(ENOSYS,"ENOSYS");
#endif
#ifdef ENOTEMPTY
  add_err2str(ENOTEMPTY,"ENOTEMPTY");
#endif
#ifdef ELOOP
  add_err2str(ELOOP,"ELOOP");
#endif
#ifdef ENOMSG
  add_err2str(ENOMSG,"ENOMSG");
#endif
#ifdef EIDRM
  add_err2str(EIDRM,"EIDRM");
#endif
#ifdef ECHRNG
  add_err2str(ECHRNG,"ECHRNG");
#endif
#ifdef EL2NSYNC
  add_err2str(EL2NSYNC,"EL2NSYNC");
#endif
#ifdef EDEADLK
  add_err2str(EDEADLK,"EDEADLK");
#endif
#ifdef EL3HLT
  add_err2str(EL3HLT,"EL3HLT");
#endif
#ifdef ENOTREADY
  add_err2str(ENOTREADY,"ENOTREADY");
#endif
#ifdef ECANCELED
  add_err2str(ECANCELED,"ECANCELED");
#endif
#ifdef EL3RST
  add_err2str(EL3RST,"EL3RST");
#endif
#ifdef EWRPROTECT
  add_err2str(EWRPROTECT,"EWRPROTECT");
#endif
#ifdef ELNRNG
  add_err2str(ELNRNG,"ELNRNG");
#endif
#ifdef ENOTSUP
  add_err2str(ENOTSUP,"ENOTSUP");
#endif
#ifdef EFORMAT
  add_err2str(EFORMAT,"EFORMAT");
#endif
#ifdef EUNATCH
  add_err2str(EUNATCH,"EUNATCH");
#endif
#ifdef ENOLCK
  add_err2str(ENOLCK,"ENOLCK");
#endif
#ifdef ENOCSI
  add_err2str(ENOCSI,"ENOCSI");
#endif
#ifdef ENOCONNECT
  add_err2str(ENOCONNECT,"ENOCONNECT");
#endif
#ifdef EL2HLT
  add_err2str(EL2HLT,"EL2HLT");
#endif
#ifdef EBADE
  add_err2str(EBADE,"EBADE");
#endif
#ifdef ESTALE
  add_err2str(ESTALE,"ESTALE");
#endif
#ifdef EBADR
  add_err2str(EBADR,"EBADR");
#endif
#ifdef EDIST
  add_err2str(EDIST,"EDIST");
#endif
#ifdef EXFULL
  add_err2str(EXFULL,"EXFULL");
#endif
#ifdef EWOULDBLOCK
  add_err2str(EWOULDBLOCK,"EWOULDBLOCK");
#endif
#ifdef ENOANO
  add_err2str(ENOANO,"ENOANO");
#endif
#ifdef EINPROGRESS
  add_err2str(EINPROGRESS,"EINPROGRESS");
#endif
#ifdef EBADRQC
  add_err2str(EBADRQC,"EBADRQC");
#endif
#ifdef EALREADY
  add_err2str(EALREADY,"EALREADY");
#endif
#ifdef EBADSLT
  add_err2str(EBADSLT,"EBADSLT");
#endif
#ifdef ENOTSOCK
  add_err2str(ENOTSOCK,"ENOTSOCK");
#endif
#ifdef EOWNERDEAD
  add_err2str(EOWNERDEAD,"EOWNERDEAD");
#endif
#ifdef EDESTADDRREQ
  add_err2str(EDESTADDRREQ,"EDESTADDRREQ");
#endif
#ifdef EBFONT
  add_err2str(EBFONT,"EBFONT");
#endif
#ifdef ENOTRECOVERABLE
  add_err2str(ENOTRECOVERABLE,"ENOTRECOVERABLE");
#endif
#ifdef EMSGSIZE
  add_err2str(EMSGSIZE,"EMSGSIZE");
#endif
#ifdef EPROTOTYPE
  add_err2str(EPROTOTYPE,"EPROTOTYPE");
#endif
#ifdef ENOPROTOOPT
  add_err2str(ENOPROTOOPT,"ENOPROTOOPT");
#endif
#ifdef EPROTONOSUPPORT
  add_err2str(EPROTONOSUPPORT,"EPROTONOSUPPORT");
#endif
#ifdef ESOCKTNOSUPPORT
  add_err2str(ESOCKTNOSUPPORT,"ESOCKTNOSUPPORT");
#endif
#ifdef ENONET
  add_err2str(ENONET,"ENONET");
#endif
#ifdef EOPNOTSUPP
  add_err2str(EOPNOTSUPP,"EOPNOTSUPP");
#endif
#ifdef ENOPKG
  add_err2str(ENOPKG,"ENOPKG");
#endif
#ifdef EPFNOSUPPORT
  add_err2str(EPFNOSUPPORT,"EPFNOSUPPORT");
#endif
#ifdef EREMOTE
  add_err2str(EREMOTE,"EREMOTE");
#endif
#ifdef EAFNOSUPPORT
  add_err2str(EAFNOSUPPORT,"EAFNOSUPPORT");
#endif
#ifdef ENOLINK
  add_err2str(ENOLINK,"ENOLINK");
#endif
#ifdef EADDRINUSE
  add_err2str(EADDRINUSE,"EADDRINUSE");
#endif
#ifdef EADV
  add_err2str(EADV,"EADV");
#endif
#ifdef EADDRNOTAVAIL
  add_err2str(EADDRNOTAVAIL,"EADDRNOTAVAIL");
#endif
#ifdef ESRMNT
  add_err2str(ESRMNT,"ESRMNT");
#endif
#ifdef ENETDOWN
  add_err2str(ENETDOWN,"ENETDOWN");
#endif
#ifdef ECOMM
  add_err2str(ECOMM,"ECOMM");
#endif
#ifdef ENETUNREACH
  add_err2str(ENETUNREACH,"ENETUNREACH");
#endif
#ifdef ENETRESET
  add_err2str(ENETRESET,"ENETRESET");
#endif
#ifdef ELOCKUNMAPPED
  add_err2str(ELOCKUNMAPPED,"ELOCKUNMAPPED");
#endif
#ifdef EMULTIHOP
  add_err2str(EMULTIHOP,"EMULTIHOP");
#endif
#ifdef ECONNABORTED
  add_err2str(ECONNABORTED,"ECONNABORTED");
#endif
#ifdef EDOTDOT
  add_err2str(EDOTDOT,"EDOTDOT");
#endif
#ifdef ENOTACTIVE
  add_err2str(ENOTACTIVE,"ENOTACTIVE");
#endif
#ifdef ECONNRESET
  add_err2str(ECONNRESET,"ECONNRESET");
#endif
#ifdef ENOBUFS
  add_err2str(ENOBUFS,"ENOBUFS");
#endif
#ifdef EOVERFLOW
  add_err2str(EOVERFLOW,"EOVERFLOW");
#endif
#ifdef EISCONN
  add_err2str(EISCONN,"EISCONN");
#endif
#ifdef ENOTUNIQ
  add_err2str(ENOTUNIQ,"ENOTUNIQ");
#endif
#ifdef ENOTCONN
  add_err2str(ENOTCONN,"ENOTCONN");
#endif
#ifdef EBADFD
  add_err2str(EBADFD,"EBADFD");
#endif
#ifdef ESHUTDOWN
  add_err2str(ESHUTDOWN,"ESHUTDOWN");
#endif
#ifdef EREMCHG
  add_err2str(EREMCHG,"EREMCHG");
#endif
#ifdef ETIMEDOUT
  add_err2str(ETIMEDOUT,"ETIMEDOUT");
#endif
#ifdef ELIBACC
  add_err2str(ELIBACC,"ELIBACC");
#endif
#ifdef ECONNREFUSED
  add_err2str(ECONNREFUSED,"ECONNREFUSED");
#endif
#ifdef ELIBBAD
  add_err2str(ELIBBAD,"ELIBBAD");
#endif
#ifdef EHOSTDOWN
  add_err2str(EHOSTDOWN,"EHOSTDOWN");
#endif
#ifdef ELIBSCN
  add_err2str(ELIBSCN,"ELIBSCN");
#endif
#ifdef EHOSTUNREACH
  add_err2str(EHOSTUNREACH,"EHOSTUNREACH");
#endif
#ifdef ELIBMAX
  add_err2str(ELIBMAX,"ELIBMAX");
#endif
#ifdef ERESTART
  add_err2str(ERESTART,"ERESTART");
#endif
#ifdef ELIBEXEC
  add_err2str(ELIBEXEC,"ELIBEXEC");
#endif
#ifdef EPROCLIM
  add_err2str(EPROCLIM,"EPROCLIM");
#endif
#ifdef EILSEQ
  add_err2str(EILSEQ,"EILSEQ");
#endif
#ifdef EUSERS
  add_err2str(EUSERS,"EUSERS");
#endif
#ifdef ERESTART
  add_err2str(ERESTART,"ERESTART");
#endif
#ifdef ESTRPIPE
  add_err2str(ESTRPIPE,"ESTRPIPE");
#endif
#ifdef ENOTEMPTY
  add_err2str(ENOTEMPTY,"ENOTEMPTY");
#endif
#ifdef ENOTSOCK
  add_err2str(ENOTSOCK,"ENOTSOCK");
#endif
#ifdef EDQUOT
  add_err2str(EDQUOT,"EDQUOT");
#endif
#ifdef EDESTADDRREQ
  add_err2str(EDESTADDRREQ,"EDESTADDRREQ");
#endif
#ifdef ECORRUPT
  add_err2str(ECORRUPT,"ECORRUPT");
#endif
#ifdef EMSGSIZE
  add_err2str(EMSGSIZE,"EMSGSIZE");
#endif
#ifdef EPROTOTYPE
  add_err2str(EPROTOTYPE,"EPROTOTYPE");
#endif
#ifdef ENOPROTOOPT
  add_err2str(ENOPROTOOPT,"ENOPROTOOPT");
#endif
#ifdef EREMOTE
  add_err2str(EREMOTE,"EREMOTE");
#endif
#ifdef ENETDOWN
  add_err2str(ENETDOWN,"ENETDOWN");
#endif
#ifdef ENETUNREACH
  add_err2str(ENETUNREACH,"ENETUNREACH");
#endif
#ifdef ENETRESET
  add_err2str(ENETRESET,"ENETRESET");
#endif
#ifdef ECONNABORTED
  add_err2str(ECONNABORTED,"ECONNABORTED");
#endif
#ifdef ECONNRESET
  add_err2str(ECONNRESET,"ECONNRESET");
#endif
#ifdef ENOBUFS
  add_err2str(ENOBUFS,"ENOBUFS");
#endif
#ifdef EISCONN
  add_err2str(EISCONN,"EISCONN");
#endif
#ifdef ENOTCONN
  add_err2str(ENOTCONN,"ENOTCONN");
#endif
#ifdef ESHUTDOWN
  add_err2str(ESHUTDOWN,"ESHUTDOWN");
#endif
#ifdef ETOOMANYREFS
  add_err2str(ETOOMANYREFS,"ETOOMANYREFS");
#endif
#ifdef ENOSYS
  add_err2str(ENOSYS,"ENOSYS");
#endif
#ifdef ETIMEDOUT
  add_err2str(ETIMEDOUT,"ETIMEDOUT");
#endif
#ifdef EMEDIA
  add_err2str(EMEDIA,"EMEDIA");
#endif
#ifdef ECONNREFUSED
  add_err2str(ECONNREFUSED,"ECONNREFUSED");
#endif
#ifdef ESOFT
  add_err2str(ESOFT,"ESOFT");
#endif
#ifdef EHOSTDOWN
  add_err2str(EHOSTDOWN,"EHOSTDOWN");
#endif
#ifdef ENOATTR
  add_err2str(ENOATTR,"ENOATTR");
#endif
#ifdef EHOSTUNREACH
  add_err2str(EHOSTUNREACH,"EHOSTUNREACH");
#endif
#ifdef ESAD
  add_err2str(ESAD,"ESAD");
#endif
#ifdef EALREADY
  add_err2str(EALREADY,"EALREADY");
#endif
#ifdef ENOTRUST
  add_err2str(ENOTRUST,"ENOTRUST");
#endif
#ifdef EINPROGRESS
  add_err2str(EINPROGRESS,"EINPROGRESS");
#endif
#ifdef ETOOMANYREFS
  add_err2str(ETOOMANYREFS,"ETOOMANYREFS");
#endif
#ifdef ESTALE
  add_err2str(ESTALE,"ESTALE");
#endif
#ifdef EILSEQ
  add_err2str(EILSEQ,"EILSEQ");
#endif
#ifdef EUCLEAN
  add_err2str(EUCLEAN,"EUCLEAN");
#endif
#ifdef ECANCELED
  add_err2str(ECANCELED,"ECANCELED");
#endif
#ifdef ENOSR
  add_err2str(ENOSR,"ENOSR");
#endif
#ifdef ENOTNAM
  add_err2str(ENOTNAM,"ENOTNAM");
#endif
#ifdef ENAVAIL
  add_err2str(ENAVAIL,"ENAVAIL");
#endif
#ifdef ETIME
  add_err2str(ETIME,"ETIME");
#endif
#ifdef EBADMSG
  add_err2str(EBADMSG,"EBADMSG");
#endif
#ifdef EISNAM
  add_err2str(EISNAM,"EISNAM");
#endif
#ifdef EPROTONOSUPPORT
  add_err2str(EPROTONOSUPPORT,"EPROTONOSUPPORT");
#endif
#ifdef EPROTO
  add_err2str(EPROTO,"EPROTO");
#endif
#ifdef EREMOTEIO
  add_err2str(EREMOTEIO,"EREMOTEIO");
#endif
#ifdef ESOCKTNOSUPPORT
  add_err2str(ESOCKTNOSUPPORT,"ESOCKTNOSUPPORT");
#endif
#ifdef EDQUOT
  add_err2str(EDQUOT,"EDQUOT");
#endif
#ifdef ENODATA
  add_err2str(ENODATA,"ENODATA");
#endif
#ifdef EOPNOTSUPP
  add_err2str(EOPNOTSUPP,"EOPNOTSUPP");
#endif
#ifdef ENOMEDIUM
  add_err2str(ENOMEDIUM,"ENOMEDIUM");
#endif
#ifdef ENOSTR
  add_err2str(ENOSTR,"ENOSTR");
#endif
#ifdef EPFNOSUPPORT
  add_err2str(EPFNOSUPPORT,"EPFNOSUPPORT");
#endif
#ifdef EAFNOSUPPORT
  add_err2str(EAFNOSUPPORT,"EAFNOSUPPORT");
#endif
#ifdef EMEDIUMTYPE
  add_err2str(EMEDIUMTYPE,"EMEDIUMTYPE");
#endif
#ifdef ENOTSUP
  add_err2str(ENOTSUP,"ENOTSUP");
#endif
#ifdef EADDRINUSE
  add_err2str(EADDRINUSE,"EADDRINUSE");
#endif
#ifdef EMULTIHOP
  add_err2str(EMULTIHOP,"EMULTIHOP");
#endif
#ifdef EADDRNOTAVAIL
  add_err2str(EADDRNOTAVAIL,"EADDRNOTAVAIL");
#endif
#ifdef ENOLINK
  add_err2str(ENOLINK,"ENOLINK");
#endif
#ifdef EOVERFLOW
  add_err2str(EOVERFLOW,"EOVERFLOW");
#endif
#ifdef WSAEINTR
  add_err2str(WSAEINTR-WSABASEERR,"WSAEINTR");
#endif
#ifdef WSAEBADF
  add_err2str(WSAEBADF-WSABASEERR,"WSAEBADF");
#endif
#ifdef WSAEACCES
  add_err2str(WSAEACCES-WSABASEERR,"WSAEACCES");
#endif
#ifdef WSAEFAULT
  add_err2str(WSAEFAULT-WSABASEERR,"WSAEFAULT");
#endif
#ifdef WSAEINVAL
  add_err2str(WSAEINVAL-WSABASEERR,"WSAEINVAL");
#endif
#ifdef WSAEMFILE
  add_err2str(WSAEMFILE-WSABASEERR,"WSAEMFILE");
#endif
#ifdef WSAEWOULDBLOCK
  add_err2str(WSAEWOULDBLOCK-WSABASEERR,"WSAEWOULDBLOCK");
#endif
#ifdef WSAEINPROGRESS
  add_err2str(WSAEINPROGRESS-WSABASEERR,"WSAEINPROGRESS");
#endif
#ifdef WSAEALREADY
  add_err2str(WSAEALREADY-WSABASEERR,"WSAEALREADY");
#endif
#ifdef WSAENOTSOCK
  add_err2str(WSAENOTSOCK-WSABASEERR,"WSAENOTSOCK");
#endif
#ifdef WSAEDESTADDRREQ
  add_err2str(WSAEDESTADDRREQ-WSABASEERR,"WSAEDESTADDRREQ");
#endif
#ifdef WSAEMSGSIZE
  add_err2str(WSAEMSGSIZE-WSABASEERR,"WSAEMSGSIZE");
#endif
#ifdef WSAEPROTOTYPE
  add_err2str(WSAEPROTOTYPE-WSABASEERR,"WSAEPROTOTYPE");
#endif
#ifdef WSAENOPROTOOPT
  add_err2str(WSAENOPROTOOPT-WSABASEERR,"WSAENOPROTOOPT");
#endif
#ifdef WSAEPROTONOSUPPORT
  add_err2str(WSAEPROTONOSUPPORT-WSABASEERR,"WSAEPROTONOSUPPORT");
#endif
#ifdef WSAESOCKTNOSUPPORT
  add_err2str(WSAESOCKTNOSUPPORT-WSABASEERR,"WSAESOCKTNOSUPPORT");
#endif
#ifdef WSAEOPNOTSUPP
  add_err2str(WSAEOPNOTSUPP-WSABASEERR,"WSAEOPNOTSUPP");
#endif
#ifdef WSAEPFNOSUPPORT
  add_err2str(WSAEPFNOSUPPORT-WSABASEERR,"WSAEPFNOSUPPORT");
#endif
#ifdef WSAEAFNOSUPPORT
  add_err2str(WSAEAFNOSUPPORT-WSABASEERR,"WSAEAFNOSUPPORT");
#endif
#ifdef WSAEADDRINUSE
  add_err2str(WSAEADDRINUSE-WSABASEERR,"WSAEADDRINUSE");
#endif
#ifdef WSAEADDRNOTAVAIL
  add_err2str(WSAEADDRNOTAVAIL-WSABASEERR,"WSAEADDRNOTAVAIL");
#endif
#ifdef WSAENETDOWN
  add_err2str(WSAENETDOWN-WSABASEERR,"WSAENETDOWN");
#endif
#ifdef WSAENETUNREACH
  add_err2str(WSAENETUNREACH-WSABASEERR,"WSAENETUNREACH");
#endif
#ifdef WSAENETRESET
  add_err2str(WSAENETRESET-WSABASEERR,"WSAENETRESET");
#endif
#ifdef WSAECONNABORTED
  add_err2str(WSAECONNABORTED-WSABASEERR,"WSAECONNABORTED");
#endif
#ifdef WSAECONNRESET
  add_err2str(WSAECONNRESET-WSABASEERR,"WSAECONNRESET");
#endif
#ifdef WSAENOBUFS
  add_err2str(WSAENOBUFS-WSABASEERR,"WSAENOBUFS");
#endif
#ifdef WSAEISCONN
  add_err2str(WSAEISCONN-WSABASEERR,"WSAEISCONN");
#endif
#ifdef WSAENOTCONN
  add_err2str(WSAENOTCONN-WSABASEERR,"WSAENOTCONN");
#endif
#ifdef WSAESHUTDOWN
  add_err2str(WSAESHUTDOWN-WSABASEERR,"WSAESHUTDOWN");
#endif
#ifdef WSAETOOMANYREFS
  add_err2str(WSAETOOMANYREFS-WSABASEERR,"WSAETOOMANYREFS");
#endif
#ifdef WSAETIMEDOUT
  add_err2str(WSAETIMEDOUT-WSABASEERR,"WSAETIMEDOUT");
#endif
#ifdef WSAECONNREFUSED
  add_err2str(WSAECONNREFUSED-WSABASEERR,"WSAECONNREFUSED");
#endif
#ifdef WSAELOOP
  add_err2str(WSAELOOP-WSABASEERR,"WSAELOOP");
#endif
#ifdef WSAENAMETOOLONG
  add_err2str(WSAENAMETOOLONG-WSABASEERR,"WSAENAMETOOLONG");
#endif
#ifdef WSAEHOSTDOWN
  add_err2str(WSAEHOSTDOWN-WSABASEERR,"WSAEHOSTDOWN");
#endif
#ifdef WSAEHOSTUNREACH
  add_err2str(WSAEHOSTUNREACH-WSABASEERR,"WSAEHOSTUNREACH");
#endif
#ifdef WSAENOTEMPTY
  add_err2str(WSAENOTEMPTY-WSABASEERR,"WSAENOTEMPTY");
#endif
#ifdef WSAEPROCLIM
  add_err2str(WSAEPROCLIM-WSABASEERR,"WSAEPROCLIM");
#endif
#ifdef WSAEUSERS
  add_err2str(WSAEUSERS-WSABASEERR,"WSAEUSERS");
#endif
#ifdef WSAEDQUOT
  add_err2str(WSAEDQUOT-WSABASEERR,"WSAEDQUOT");
#endif
#ifdef WSAESTALE
  add_err2str(WSAESTALE-WSABASEERR,"WSAESTALE");
#endif
#ifdef WSAEREMOTE
  add_err2str(WSAEREMOTE-WSABASEERR,"WSAEREMOTE");
#endif
#ifdef WSAEDISCON
  add_err2str(WSAEDISCON-WSABASEERR,"WSAEDISCON");
#endif
#ifdef WSAENOMORE
  add_err2str(WSAENOMORE-WSABASEERR,"WSAENOMORE");
#endif
#ifdef WSAECANCELLED
  add_err2str(WSAECANCELLED-WSABASEERR,"WSAECANCELLED");
#endif
#ifdef WSAEINVALIDPROCTABLE
  add_err2str(WSAEINVALIDPROCTABLE-WSABASEERR,"WSAEINVALIDPROCTABLE");
#endif
#ifdef WSAEINVALIDPROVIDER
  add_err2str(WSAEINVALIDPROVIDER-WSABASEERR,"WSAEINVALIDPROVIDER");
#endif
#ifdef WSAEPROVIDERFAILEDINIT
  add_err2str(WSAEPROVIDERFAILEDINIT-WSABASEERR,"WSAEPROVIDERFAILEDINIT");
#endif
#ifdef WSAEREFUSED
  add_err2str(WSAEREFUSED-WSABASEERR,"WSAEREFUSED");
#endif
#ifdef WSANOTINITIALISED
  add_err2str(WSANOTINITIALISED-WSABASEERR,"WSANOTINITIALISED");
#endif
  err2str_init = 1 ;
}

/**************  IBV_EVENT listener thread  ***************************/

#endif
