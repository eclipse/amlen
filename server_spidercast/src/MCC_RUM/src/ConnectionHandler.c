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

#include <spidercast_certificates.h>

#define CIP_DBG 0

#if CIP_DBG
 #define DBG_PRT(x) x
#else
 #define DBG_PRT(x)
#endif

static THREAD_RETURN_TYPE ConnectionHandler(void *arg) ;

#define BASE_CCP_SIZE  sizeof(rumHeaderCCP)

typedef enum
{
   CIP_IO_STATE_ACC = 0   ,
   CIP_IO_STATE_CONN      ,
   CIP_IO_STATE_WRITE     ,
   CIP_IO_STATE_READ      ,
   CIP_IB_STATE_CREQ      ,
   CIP_IB_STATE_CEST      ,
   CIP_IB_STATE_ADDR      ,
   CIP_IB_STATE_ROUTE     ,
   CIP_IO_STATE_SSL_HS    ,
   CIP_IO_STATE_NOP         
} cip_io_state_t ; 

#define CIP_STATE_S_CFP_REQ   10
#define CIP_STATE_S_CFP_REP   11
#define CIP_STATE_S_CFP_ACK   12
#define CIP_STATE_R_CFP_REQ0  13
#define CIP_STATE_R_CFP_REQ   14
#define CIP_STATE_R_CFP_REP0  15
#define CIP_STATE_R_CFP_REP   16
#define CIP_STATE_R_CFP_ACK0  17
#define CIP_STATE_R_CFP_ACK   18


static void cip_init_conn_id(rumInstanceRec *gInfo) ;
static void cip_next_conn_id(rumInstanceRec *gInfo) ;
static int  cip_set_low_delay(SOCKET sfd, int nodelay, int lwm) ; 
static int  cip_set_socket_buffer_size(SOCKET sfd, int buffer_size, int opt) ; 
static int  cip_create_accept_conns(rumInstanceRec *gInfo) ; 
static int  cip_handle_conn_req(rumInstanceRec *gInfo, ConnInfoRec *req) ; 
static int  cip_set_local_endpoint(rmmReceiverRec *rInfo, ConnInfoRec *cInfo)  ; 
static int  cip_set_remote_endpoint(rmmReceiverRec *rInfo, ConnInfoRec *cInfo)  ; 
static int  cip_check_to(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static int  cip_remove_conn(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static int  cip_build_new_incoming_conn(rumInstanceRec *gInfo, SOCKET sfd)  ; 
static int  cip_conn_failed(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
           
static int  cip_prepare_s_cfp_req(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static int  cip_prepare_s_cfp_rep(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static int  cip_prepare_s_cfp_ack(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
           
static int  cip_prepare_ccp_header(rumInstanceRec *gInfo, ConnInfoRec *cInfo, char *buffer, char type) ; 
           
static int  cip_prepare_r_cfp_0(rumInstanceRec *gInfo, ConnInfoRec *cInfo, uint32_t size) ; 
static int  cip_prepare_r_cfp_1(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static int  cip_process_r_cfp_req(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static int  cip_process_r_cfp_rep(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static int  cip_process_r_cfp_ack(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static int  cip_make_conn_ready(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static void cip_remove_all_conns(rumInstanceRec *gInfo) ; 
static int  cip_update_conn_list(rumInstanceRec *gInfo, ConnInfoRec *cInfo, int add) ; 
static int  cip_set_conn_buffers(rumInstanceRec *gInfo, ConnInfoRec *cInfo) ; 
static int cip_ssl_handshake(rumInstanceRec *gInfo, ConnInfoRec *cInfo);

#if USE_SSL
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
#endif
/*
 * SSL methods enumeration
 */
#if 0
ism_enumList enum_methods2 [] = {
    { "Method",    4,                 },
    { "SSLv3",     SecMethod_SSLv3,   },
    { "TLSv1",     SecMethod_TLSv1,   },
    { "TLSv1.1",   SecMethod_TLSv1_1, },
    { "TLSv1.2",   SecMethod_TLSv1_2, },
};
#endif

typedef struct x509_store_ctx_st X509StoreCtx;
typedef int (*ism_verifySSLCert)(int, X509StoreCtx *);
/**
 * Create a security context.
 *
 * Any of the string parameters can be NULL to indicate that the default should be used.
 */
extern struct ssl_ctx_st * ism_common_create_ssl_ctx(const char * objname, const char * methodName, const char * ciphers,
        const char * certFile, const char * keyFile, int isServer, int sslopt, int useClientCert, const char * profileName,
        ism_verifySSLCert verifyCallback, const char * serverName);
extern const char * ism_common_getStringConfig(const char * name);
#endif


THREAD_RETURN_TYPE ConnectionHandler(void *arg)
{
  SOCKET sfd ; 
  int nr , rc , use_ib , use_ssl, n_acc_conns , ec; 
  int iBad_select , iBad_accept , iBad_state ;
  socklen_t sas_len ;
 #if !USE_POLL
  struct timeval tv ;
 #endif
  WhyBadRec *wb ;
  ConnInfoRec *cInfo, *nextC ; 
  ConnInfoRec *req ; 
  rumInstanceRec *gInfo ;
  rmmReceiverRec *rInfo ; 
  socklen_t err , err_len; 
  struct sigaction sigact ; 
  char tmpstr[4] ; 
  const char* myName = "ConnectionHandler";
  char errMsg[ERR_MSG_SIZE];
  TCHandle tcHandle = NULL;

  memset(&sigact,0,sizeof(struct sigaction)) ; 
  sigact.sa_handler = SIG_IGN ; 
  sigaction(SIGPIPE,&sigact,NULL) ; 
 #if USE_PRCTL
  {
    char tn[16] ; 
    rmm_strlcpy(tn,__FUNCTION__,16);
    prctl( PR_SET_NAME,(uintptr_t)tn);
  }
 #endif

  gInfo = (rumInstanceRec *)arg ;
  rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
  rInfo->GlobalSync.chUP =-1 ; 
  tcHandle = rInfo->tcHandle;
  use_ib = (gInfo->basicConfig.Protocol==RUM_IB) ; 
  use_ssl= (gInfo->basicConfig.Protocol==RUM_SSL); 

  strlcpy(gInfo->cipInfo.ev_msg,"_!!!_",EV_MSG_SIZE) ; 
  gInfo->cipInfo.ev_dbg[0] = gInfo->cipInfo.ev_msg ; 

  wb = &rInfo->why_bad ;
  pthread_mutex_lock(&rInfo->wb_mutex) ;
  iBad_select   = add_why_bad_msg(wb,0,myName,"select failed") ;
  iBad_accept   = add_why_bad_msg(wb,0,myName,"accept failed") ;
  iBad_state    = add_why_bad_msg(wb,0,myName,"invalid state") ;
  pthread_mutex_unlock(&rInfo->wb_mutex) ;

 #if !USE_POLL
  llmFD_ZERO(&gInfo->cipInfo.rfds) ;
  llmFD_ZERO(&gInfo->cipInfo.wfds) ;
  gInfo->cipInfo.np1 = 0 ; 
 #endif

 #if USE_SSL
  if ( use_ssl )
  {
    int i;

    FILE * f_cert;
    FILE * f_key;
    char * cert;
    char * key;
    cert = ha_certdata;
    key = ha_keydata;
    const char * dir = ism_common_getStringConfig("KeyStore");

    if ( dir ) {
        char * cert_file = alloca(18 + strlen(dir));
        strcpy(cert_file,dir);
        strcat(cert_file, "/Cluster_cert.pem");
        f_cert = fopen(cert_file, "rb");
        char * key_file = alloca(18 + strlen(dir) );
        strcpy(key_file,dir);
        strcat(key_file, "/Cluster_key.pem");
        f_key = fopen(key_file, "rb");

        if (f_cert && f_key) {

            cert = "Cluster_cert.pem";
            key = "Cluster_key.pem";
            fclose(f_key);
            fclose(f_cert);
        }
    }

    for ( i=0 ; i<2 ; i++ )
    {
      char *nm ; 
      nm = i ? "RUM_Outgoing" : "RUM_Incoming" ; 
      gInfo->sslInfo->sslCtx[i] = ism_common_create_ssl_ctx(nm, "TLSv1.2", "ECDHE-ECDSA-AES128-GCM-SHA256", cert, key, 1-i, 0x010003FF, 1, nm, NULL, "Cluster");
      if (!gInfo->sslInfo->sslCtx[i] )
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1127),"%s",
            "The SSL/TLS context could not be created for {0}", nm);
        goto exitThread ;
      }
    }
  }
 #endif
  
  if ( cip_create_accept_conns(gInfo) != RMM_SUCCESS )
  {
    pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
    rInfo->GlobalSync.chEC = RUM_L_E_PORT_BUSY ; 
    pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3107),"",
        "The RUM receiver failed to create the accept socket.");
    goto exitThread ;
  }
  n_acc_conns = gInfo->cipInfo.nConnsInProg ; 

  pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
  rInfo->GlobalSync.chUP = 1 ;
  pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5069),"%s%llu%d",
    "The {0} thread is running. Thread id: {1} ({2}).",
    "ConnectionHandler",LLU_VALUE(my_thread_id()),
#ifdef OS_Linuxx
    (int)gettid());
#else
    (int)my_thread_id());
#endif


  while ( 1 )
  {
    if ( (req = (ConnInfoRec *)LL_get_buff(gInfo->connReqQ,1)) != NULL )
    {
      cip_handle_conn_req(gInfo,req) ; 
    }
   #if USE_POLL
    nr = poll(gInfo->cipInfo.fds, gInfo->cipInfo.nfds, (gInfo->cipInfo.nConnsInProg>n_acc_conns) ? 1 : 100) ; 
   #else
    tv.tv_sec  = 0 ; 
    tv.tv_usec = (gInfo->cipInfo.nConnsInProg>n_acc_conns) ? 1 : 100000 ; 
    nr = select((int)gInfo->cipInfo.np1,&gInfo->cipInfo.rfds,&gInfo->cipInfo.wfds,&gInfo->cipInfo.efds,&tv) ;
   #endif
    if ( nr < 0 )
    {
      if ( (rc = rmmErrno ) != EINTR )
      {
        set_why_bad(wb,iBad_select) ;
      }
      llmFD_ZERO(&gInfo->cipInfo.rfds) ;
      llmFD_ZERO(&gInfo->cipInfo.wfds) ;
      llmFD_ZERO(&gInfo->cipInfo.efds) ;
    }
    if ( rInfo->GlobalSync.goDN )
      break ;  

    for ( cInfo=gInfo->cipInfo.firstConnInProg ; cInfo ; cInfo=nextC )
    {
      nextC = cInfo->next ; 
      if ( cInfo->io_state != CIP_IO_STATE_ACC && cInfo->io_state != CIP_IB_STATE_CREQ )
      {
  //    DBG_PRT(printf(" i=%d , io_state=%d , stat=%d\n",i,cInfo->io_state,cInfo->state)) ;
        if ( cip_check_to(gInfo,cInfo) == RMM_FAILURE )
        {
       // DBG_PRT(printf(" cip_check_to failed for i= %d\n",i)) ; 
          continue ; 
        }
      }
      switch (cInfo->io_state)
      {
        case CIP_IO_STATE_ACC :
         #if USE_POLL
          if ( gInfo->cipInfo.fds[cInfo->ind].revents & POLLIN ) 
         #else
          if ( FD_ISSET(cInfo->sfd,&gInfo->cipInfo.rfds) )
         #endif
          {
            DBG_PRT(printf(" CIP_IO_STATE_ACC... %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
            sas_len = sizeof(SAS) ; 
            if ( (sfd = accept_(cInfo->sfd, cInfo->rmt_sa, &sas_len)) == INVALID_SOCKET )
            {
              DBG_PRT(printf(" accept failed: err= %d (%s)\n",rmmErrno,rmmErrStr(rmmErrno))) ; 
              set_why_bad(wb,iBad_accept) ;
            }
            else
            {
              DBG_PRT(printf(" accept ok! calling build_new_incoming_conn!\n")) ; 
              cip_build_new_incoming_conn(gInfo,sfd) ; /* this includes 'add_conn() and 'prepare_r_cfp_req0()' */
            }
          }
         #if !USE_POLL
          else
            FD_SET(cInfo->sfd,&gInfo->cipInfo.rfds) ;
         #endif
          break ; 
        case CIP_IO_STATE_CONN :
          DBG_PRT(printf(" CIP_IO_STATE_CONN.. %s|%d\n",cInfo->req_addr,cInfo->req_port)) ; 

         #if USE_POLL
          if ( (gInfo->cipInfo.fds[cInfo->ind].revents & (POLLIN|POLLOUT)) ) 
         #else
          if ( FD_ISSET(cInfo->sfd,&gInfo->cipInfo.rfds) || 
               FD_ISSET(cInfo->sfd,&gInfo->cipInfo.wfds) )
         #endif
          {
            err_len = sizeof(err) ; 
            err = 0 ; 
            if ( getsockopt(cInfo->sfd,SOL_SOCKET,SO_ERROR,&err,&err_len) != 0 )
              err = rmmErrno ; 
            if ( err != 0 )
            {
              DBG_PRT(printf(" ConnFailed: err= %d\n",err)) ; 
              snprintf(gInfo->cipInfo.ev_msg,EV_MSG_SIZE," connect() failed: err= %d (%s)\n",err,rmmErrStr(err)) ; 
              cip_conn_failed(gInfo,cInfo) ; 
              break ; 
            }
            if ( read(cInfo->sfd, tmpstr, 0) != 0 )
            {
              DBG_PRT(printf(" ConnFailed: read 0 -> rc!=0\n")) ; 
              strlcpy(gInfo->cipInfo.ev_msg,"read 0 -> rc!=0",EV_MSG_SIZE) ; 
              cip_conn_failed(gInfo,cInfo) ; 
              break ; 
            }
          }
          else
          {
           #if !USE_POLL
            FD_SET(cInfo->sfd,&gInfo->cipInfo.rfds) ;
            FD_SET(cInfo->sfd,&gInfo->cipInfo.wfds) ;
           #endif
            break ; 
          }
          cInfo->io_state = CIP_IO_STATE_SSL_HS ;
          break ; 
        case CIP_IO_STATE_SSL_HS :
          DBG_PRT(printf(" CIP_IO_STATE_SSL_HS.. %s|%d\n",cInfo->req_addr,cInfo->req_port)) ; 
          rc = cip_ssl_handshake(gInfo,cInfo) ; 
          DBG_PRT(printf(" After cip_ssl_handshake for %s|%d , rc=%d\n",cInfo->req_addr,cInfo->req_port,rc)) ; 
          if ( rc > 0 )
          {
            if ( cInfo->init_here )
            {
              if ( cip_set_local_endpoint(rInfo,cInfo) == RMM_FAILURE  )
              {
                DBG_PRT(printf(" ConnFailed: set_local_endpoint failed\n")) ; 
                strlcpy(gInfo->cipInfo.ev_msg,"set_local_endpoint failed",EV_MSG_SIZE) ; 
                cip_conn_failed(gInfo,cInfo) ; 
                break ; 
              }
              if ( cip_set_remote_endpoint(rInfo,cInfo) == RMM_FAILURE )
              {
                DBG_PRT(printf(" ConnFailed: cip_set_remote_endpoint failed\n")) ; 
                strlcpy(gInfo->cipInfo.ev_msg,"cip_set_remote_endpoint",EV_MSG_SIZE) ; 
                cip_conn_failed(gInfo,cInfo) ; 
                break ; 
              }
              if ( cip_prepare_s_cfp_req(gInfo, cInfo) == RMM_FAILURE )
              {
                DBG_PRT(printf(" ConnFailed: cip_prepare_s_cfp_req failed\n")) ; 
                strlcpy(gInfo->cipInfo.ev_msg,"cip_prepare_s_cfp_req",EV_MSG_SIZE) ; 
                cip_conn_failed(gInfo,cInfo) ; 
                break ; 
              }
              cInfo->state    = CIP_STATE_S_CFP_REQ ; 
              cInfo->io_state = CIP_IO_STATE_WRITE ; 
             #if USE_POLL
              gInfo->cipInfo.fds[cInfo->ind].events = POLLOUT ;  
             #else
              FD_SET(cInfo->sfd,&gInfo->cipInfo.wfds) ;
             #endif
            }
            else
            {
              if ( cip_prepare_r_cfp_0(gInfo,cInfo,65536) == RMM_FAILURE )
                break ; 
          
              cInfo->io_state = CIP_IO_STATE_READ ; 
              cInfo->state    = CIP_STATE_R_CFP_REQ0 ; 
            }
          }
          else
          if ( rc < 0 )
          {
            cip_conn_failed(gInfo,cInfo) ; 
            DBG_PRT(printf("cip_ssl_handshake() failed for conn |%s|\n",cInfo->req_addr));
          }
          break ; 
        case CIP_IO_STATE_WRITE :
          DBG_PRT(printf(" CIP_IO_STATE_WRITE.. %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
          if ( use_ib ||
             #if USE_POLL
              (gInfo->cipInfo.fds[cInfo->ind].revents & POLLOUT) ) 
             #else
               FD_ISSET(cInfo->sfd,&gInfo->cipInfo.wfds) )
             #endif
          {
            rc = rmm_write(cInfo) ; 
            if ( rc > 0 )
            {
              DBG_PRT(printf(" write ok , rc=%d %d %d\n",rc,cInfo->wrInfo.reqlen,cInfo->wrInfo.offset)) ; 
              cInfo->wrInfo.offset += rc ; 
              if ( cInfo->wrInfo.offset == cInfo->wrInfo.reqlen )
              {
               #if USE_POLL
                gInfo->cipInfo.fds[cInfo->ind].events = POLLIN ;  
               #else
                FD_CLR(cInfo->sfd,&gInfo->cipInfo.wfds) ;
                FD_SET(cInfo->sfd,&gInfo->cipInfo.rfds) ;
               #endif
                cInfo->io_state = CIP_IO_STATE_READ ; 
                switch (cInfo->state)
                {
                  case CIP_STATE_S_CFP_REQ :
                    if ( cip_prepare_r_cfp_0(gInfo,cInfo,BASE_CCP_SIZE) == RMM_FAILURE )
                    {
                      DBG_PRT(printf(" ConnFailed: cip_prepare_r_cfp_0 at CIP_STATE_S_CFP_REQ failed\n")) ; 
                      strlcpy(gInfo->cipInfo.ev_msg,"cip_prepare_r_cfp_0 CIP_STATE_S_CFP_REQ",EV_MSG_SIZE) ; 
                      cip_conn_failed(gInfo,cInfo) ; 
                      break ; 
                    }
                    cInfo->state = CIP_STATE_R_CFP_REP0 ; 
                    break ; 
                  case CIP_STATE_S_CFP_REP : 
                    if ( cip_prepare_r_cfp_0(gInfo,cInfo,BASE_CCP_SIZE) == RMM_FAILURE )
                    {
                      DBG_PRT(printf(" ConnFailed: cip_prepare_r_cfp_0 at CIP_STATE_S_CFP_REP failed\n")) ; 
                      strlcpy(gInfo->cipInfo.ev_msg,"cip_prepare_r_cfp_0 CIP_STATE_S_CFP_REP",EV_MSG_SIZE) ; 
                      cip_conn_failed(gInfo,cInfo) ; 
                      break ; 
                    }
                    cInfo->state = CIP_STATE_R_CFP_ACK0 ; 
                    break ; 
                  case CIP_STATE_S_CFP_ACK :
                    cip_make_conn_ready(gInfo,cInfo) ;  
                    break ; 
                  default :
                    DBG_PRT(printf(" bad_state_0: %s|%d , state=%d \n",cInfo->req_addr,cInfo->req_port,cInfo->state)) ; 
                    set_why_bad(wb,iBad_state) ;
                }
              }
              else
                cInfo->wrInfo.bptr += rc ; 
            }
            else
            if ( rc < 0 )
            {
              DBG_PRT(printf(" write failed, rc=%d %d %d\n",rc,cInfo->wrInfo.reqlen,cInfo->wrInfo.offset)) ; 
              snprintf(gInfo->cipInfo.ev_msg,EV_MSG_SIZE,"write failed, rc=%d ",rc) ; 
              cip_conn_failed(gInfo,cInfo) ; 
            }
          }
          else
          {
        //  DBG_PRT(printf(" write not FD_ISSET !! %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
           #if !USE_POLL
            FD_SET(cInfo->sfd,&gInfo->cipInfo.wfds) ;
           #endif
          }
          break ; 
        case CIP_IO_STATE_READ :
          DBG_PRT(printf(" CIP_IO_STATE_READ.. %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
          if ( use_ib || use_ssl ||
             #if USE_POLL
              (gInfo->cipInfo.fds[cInfo->ind].revents & POLLIN) ) 
             #else
               FD_ISSET(cInfo->sfd,&gInfo->cipInfo.rfds) )
             #endif
          {
            rc = rmm_read(cInfo, cInfo->rdInfo.bptr, (cInfo->rdInfo.reqlen-cInfo->rdInfo.offset), 1, &ec, errMsg) ; 
            if ( rc > 0 )
            {
              DBG_PRT(printf("read ok! rc=%d %d %d\n ",rc,cInfo->rdInfo.reqlen,cInfo->rdInfo.offset)) ; 
              cInfo->rdInfo.offset += rc ; 
              if ( cInfo->rdInfo.offset == cInfo->rdInfo.reqlen )
              {
                switch (cInfo->state)
                {
                  case CIP_STATE_R_CFP_REQ0 : 
                    DBG_PRT(printf(" CIP_STATE_R_CFP_REQ0 %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
                    if ( cip_prepare_r_cfp_1(gInfo,cInfo) == RMM_FAILURE )
                    {
                      DBG_PRT(printf(" ConnFailed: cip_prepare_r_cfp_1 at CIP_STATE_R_CFP_REQ0 failed\n")) ; 
                      strlcpy(gInfo->cipInfo.ev_msg,"cip_prepare_r_cfp_1 CIP_STATE_R_CFP_REQ0",EV_MSG_SIZE) ; 
                      cip_conn_failed(gInfo,cInfo) ; 
                      break ; 
                    }
                    cInfo->state = CIP_STATE_R_CFP_REQ ; 
                    break ; 
                  case CIP_STATE_R_CFP_REQ :
                    DBG_PRT(printf(" CIP_STATE_R_CFP_REQ  %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
                    if ( cip_process_r_cfp_req(gInfo,cInfo) == RMM_FAILURE )
                    {
                      DBG_PRT(printf(" ConnFailed: cip_process_r_cfp_req at CIP_STATE_R_CFP_REQ failed\n")) ; 
                      strlcpy(gInfo->cipInfo.ev_msg,"cip_process_r_cfp_req CIP_STATE_R_CFP_REQ",EV_MSG_SIZE) ; 
                      cip_conn_failed(gInfo,cInfo) ; 
                      break ; 
                    }
                    if ( cip_prepare_s_cfp_rep(gInfo, cInfo) == RMM_FAILURE )
                    {
                      DBG_PRT(printf(" ConnFailed: cip_prepare_s_cfp_rep at CIP_STATE_R_CFP_REQ failed\n")) ; 
                      strlcpy(gInfo->cipInfo.ev_msg,"cip_prepare_s_cfp_rep CIP_STATE_R_CFP_REQ",EV_MSG_SIZE) ; 
                      cip_conn_failed(gInfo,cInfo) ; 
                      break ; 
                    }
                    cInfo->state   = CIP_STATE_S_CFP_REP ; 
                    cInfo->io_state= CIP_IO_STATE_WRITE ; 
                   #if USE_POLL
                    gInfo->cipInfo.fds[cInfo->ind].events = POLLOUT ; 
                   #else
                    FD_CLR(cInfo->sfd,&gInfo->cipInfo.rfds) ;
                    FD_SET(cInfo->sfd,&gInfo->cipInfo.wfds) ;
                   #endif
                    break ; 
                  case CIP_STATE_R_CFP_REP0 : 
                    DBG_PRT(printf(" CIP_STATE_R_CFP_REP0 %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
                    if ( cip_prepare_r_cfp_1(gInfo,cInfo) == RMM_FAILURE )
                    {
                      DBG_PRT(printf(" ConnFailed: cip_prepare_r_cfp_1 at CIP_STATE_R_CFP_REP0 failed\n")) ; 
                      strlcpy(gInfo->cipInfo.ev_msg,"cip_prepare_r_cfp_1  CIP_STATE_R_CFP_REP0",EV_MSG_SIZE) ; 
                      cip_conn_failed(gInfo,cInfo) ; 
                      break ; 
                    }
                    cInfo->state = CIP_STATE_R_CFP_REP ; 
                    break ; 
                  case CIP_STATE_R_CFP_REP : 
                    DBG_PRT(printf(" CIP_STATE_R_CFP_REP  %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
                    if ( cip_process_r_cfp_rep(gInfo,cInfo) == RMM_FAILURE )
                    {
                      DBG_PRT(printf(" ConnFailed: cip_process_r_cfp_rep at CIP_STATE_R_CFP_REP failed\n")) ; 
                      strlcpy(gInfo->cipInfo.ev_msg,"cip_process_r_cfp_rep CIP_STATE_R_CFP_REP",EV_MSG_SIZE) ; 
                      cip_conn_failed(gInfo,cInfo) ; 
                      break ; 
                    }
                    if ( cip_prepare_s_cfp_ack(gInfo,cInfo) == RMM_FAILURE )
                    {
                      DBG_PRT(printf(" ConnFailed: cip_prepare_s_cfp_ack at CIP_STATE_R_CFP_REP failed\n")) ; 
                      strlcpy(gInfo->cipInfo.ev_msg,"cip_prepare_s_cfp_ack CIP_STATE_R_CFP_REP",EV_MSG_SIZE) ; 
                      cip_conn_failed(gInfo,cInfo) ; 
                      break ; 
                    }
                    cInfo->state   = CIP_STATE_S_CFP_ACK ; 
                    cInfo->io_state= CIP_IO_STATE_WRITE ; 
                   #if USE_POLL
                    gInfo->cipInfo.fds[cInfo->ind].events = POLLOUT ; 
                   #else
                    FD_CLR(cInfo->sfd,&gInfo->cipInfo.rfds) ;
                    FD_SET(cInfo->sfd,&gInfo->cipInfo.wfds) ;
                   #endif
                    break ; 
                  case CIP_STATE_R_CFP_ACK0 :
                    DBG_PRT(printf(" CIP_STATE_R_CFP_ACK0 %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
                    if ( cip_prepare_r_cfp_1(gInfo,cInfo) == RMM_FAILURE )
                    {
                      DBG_PRT(printf(" ConnFailed: cip_prepare_r_cfp_1 at CIP_STATE_R_CFP_ACK0 failed\n")) ; 
                      strlcpy(gInfo->cipInfo.ev_msg,"cip_prepare_r_cfp_1  CIP_STATE_R_CFP_ACK0",EV_MSG_SIZE) ; 
                      cip_conn_failed(gInfo,cInfo) ; 
                      break ; 
                    }
                    cInfo->state = CIP_STATE_R_CFP_ACK ; 
                    break ; 
                  case CIP_STATE_R_CFP_ACK :
                    DBG_PRT(printf(" CIP_STATE_R_CFP_ACK  %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
                    if ( cip_process_r_cfp_ack(gInfo,cInfo) == RMM_FAILURE )
                    {
                      DBG_PRT(printf(" ConnFailed: cip_process_r_cfp_ack at CIP_STATE_R_CFP_ACK failed\n")) ; 
                      strlcpy(gInfo->cipInfo.ev_msg,"cip_process_r_cfp_ack CIP_STATE_R_CFP_ACK",EV_MSG_SIZE) ; 
                      cip_conn_failed(gInfo,cInfo) ; 
                      break ; 
                    }
                    cip_make_conn_ready(gInfo,cInfo) ;  
                    break ; 
                  default :
                    DBG_PRT(printf(" bad_state_1: %s|%d, state=%d \n",cInfo->req_addr,cInfo->req_port,cInfo->state)) ; 
                    set_why_bad(wb,iBad_state) ;
                }
              }
              else
                cInfo->rdInfo.bptr += rc ; 
            }
            else
            if ( rc < 0 )
            {
              DBG_PRT(printf("read failed! rc=%d %d %d\n ",rc,cInfo->rdInfo.reqlen,cInfo->rdInfo.offset)) ;
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(1127),"%s",
                 "rmm_read failed, errMsg: {0}", errMsg);
              snprintf(gInfo->cipInfo.ev_msg,EV_MSG_SIZE,"read failed, rc=%d ",rc) ; 
              cip_conn_failed(gInfo,cInfo) ; 
            }
          }
          else
          {
        //  DBG_PRT(printf(" read not FD_ISSET !! %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
           #if !USE_POLL
            FD_SET(cInfo->sfd,&gInfo->cipInfo.rfds) ;
           #endif
          }
          break ; 
        default :
          DBG_PRT(printf(" bad_state_2: %s|%d, state=%d \n",cInfo->req_addr,cInfo->req_port,cInfo->io_state)) ;
          set_why_bad(wb,iBad_state) ;
      }
    }
  }
  exitThread:

  cip_remove_all_conns(gInfo) ; 

  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5067),"%s%llu",
    "The {0}({1}) thread was stopped.",
    "ConnectionHandler", THREAD_ID(pthread_self()));

  pthread_mutex_lock(&rInfo->GlobalSync.mutex) ; 
  rInfo->GlobalSync.chUP = 0 ; 
  pthread_mutex_unlock(&rInfo->GlobalSync.mutex) ; 

  rmm_thread_exit() ;
}

/**********************************/

int cip_ssl_handshake(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
 #if USE_SSL
  int rc;

  if ( gInfo->use_ssl )
  {
    SSL * ssl = cInfo->sslInfo->ssl ; 
    BIO * bio = cInfo->sslInfo->bio ; 

    if (!ssl )
    {
      cInfo->sslInfo->sslCtx = gInfo->sslInfo->sslCtx[cInfo->init_here] ; 
      ssl = SSL_new(cInfo->sslInfo->sslCtx);
      if (!ssl )
      {
        cInfo->sslInfo->func = "SSL_new" ;
        sslTraceErr(cInfo, 0, __FILE__, __LINE__);
      }
      else 
      {
        if ( cInfo->init_here )
          SSL_set_connect_state(ssl);
        else
          SSL_set_accept_state(ssl);
        bio = BIO_new_socket(cInfo->sfd, BIO_NOCLOSE);
        if (!bio)
        {
          cInfo->sslInfo->func = "BIO_new_socket" ; 
          sslTraceErr(cInfo, 0, __FILE__, __LINE__);
        }
      }
      if (!ssl || !bio) 
      {
        if (ssl)
          SSL_free(ssl);
        return -1;
      }
   
      SSL_set_bio(ssl, bio, bio);
      cInfo->sslInfo->ssl = ssl ; 
      cInfo->sslInfo->bio = bio ; 
      pthread_mutex_init(cInfo->sslInfo->lock,NULL) ;
      cInfo->use_ssl = 1 ; 
    }

    // sslHandshake

    if ( cInfo->init_here )
    {
      cInfo->sslInfo->func = "SSL_connect" ; 
      rc = SSL_connect(ssl);
    }
    else
    {
      cInfo->sslInfo->func = "SSL_accept" ; 
      rc = SSL_accept(ssl);
    }
    if (rc <= 0) 
    {
      int ec = SSL_get_error(ssl, rc);
      if ( ec == SSL_ERROR_WANT_WRITE )
      {
        gInfo->cipInfo.fds[cInfo->ind].events = POLLOUT ; 
        return 0 ; 
      }
      else
      if ( ec == SSL_ERROR_WANT_READ )
      {
        gInfo->cipInfo.fds[cInfo->ind].events = POLLIN ; 
        return 0 ; 
      }
      else
      {
        sslTraceErr(cInfo, ec, __FILE__, __LINE__);
        return -1;
      }
    } 
    else 
    {
      if ( cInfo->init_here )
      {
        /*
         * On a completed TLS outgoing connection
         */
        rc = SSL_get_verify_result(ssl);
        if ((rc != X509_V_OK) && (rc != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)) 
        {
          char cid_str[20] ; 
          TCHandle tcHandle;
          rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
          tcHandle = rInfo->tcHandle ; 
          const char * sslErr = X509_verify_cert_error_string(rc);
          b2h(cid_str,(unsigned char *)&cInfo->connection_id,sizeof(rumConnectionID_t)) ;
          llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1128),"%s%d%s%s",
            "Certificate verification failed: conn={0}:{1} id={2} error={3}.",
                    cInfo->req_addr,cInfo->req_port,cid_str,sslErr);
          return -1;
        }
      }
      else
      {
        /*
         * On a completed TLS incoming connection
         */
      }
    }
  }
 #endif

  return 1 ; 
}

/**********************************/

int  cip_update_conn_list(rumInstanceRec *gInfo, ConnInfoRec *cInfo, int add)
{
  int rc;
  if ( add )
  {
    rc = update_conn_list(&gInfo->cipInfo.firstConnInProg, cInfo, &gInfo->cipInfo.nConnsInProg, 1) ;  
   #if USE_POLL
    if ( gInfo->cipInfo.lfds < gInfo->cipInfo.nConnsInProg )
    {
      int l = gInfo->cipInfo.lfds + 64 ; 
      struct pollfd *new ;
      if ( !(new = realloc(gInfo->cipInfo.fds, l*sizeof(struct pollfd))) )
      {
        char *methodName = "cip_update_conn_list";
        TCHandle tcHandle = NULL;
        rmmReceiverRec *rInfo ; 
        rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
        tcHandle = rInfo->tcHandle ; 
        LOG_MEM_ERR(tcHandle,methodName,(l*sizeof(struct pollfd)));
        return -1 ; 
      }
      gInfo->cipInfo.fds = new ; 
      memset(gInfo->cipInfo.fds+gInfo->cipInfo.lfds, 0, (l-gInfo->cipInfo.lfds)*sizeof(struct pollfd)) ; 
      gInfo->cipInfo.lfds = l ; 
    }
   #endif
  }
  else
  {
    rc = update_conn_list(&gInfo->cipInfo.firstConnInProg, cInfo, &gInfo->cipInfo.nConnsInProg, 0) ;  
   #if USE_POLL
    if ( !rc )
    {
      int i;
      int ind = cInfo->ind ; 
      for ( i=ind ; i<gInfo->cipInfo.nConnsInProg ; i++ )
        gInfo->cipInfo.fds[i] = gInfo->cipInfo.fds[i+1] ; 
    }
   #endif
  }
 #if USE_POLL
  gInfo->cipInfo.nfds = gInfo->cipInfo.nConnsInProg ;
 #endif
  return rc ; 
}

/**********************************/

void cip_init_conn_id(rumInstanceRec *gInfo)
{
  time_t sec ; 

  memset(&gInfo->cipInfo.ConnId,0,sizeof(ConnIdRec)) ; 
  gInfo->cipInfo.ConnId.ip   = gInfo->gsi_high ; 
  gInfo->cipInfo.ConnId.port = gInfo->port ;
  gInfo->cipInfo.ConnId.count= (uint16_t)rmmTime(&sec,NULL) ; 
  gInfo->cipInfo.ConnId.count += (uint16_t)sec ; 
  gInfo->cipInfo.cidInit = 1 ; 
}

void cip_next_conn_id(rumInstanceRec *gInfo)
{
  rumConnectionID_t cid ; 

  if ( !gInfo->cipInfo.cidInit )
    cip_init_conn_id(gInfo);
  do
  {
    gInfo->cipInfo.ConnId.count++ ; 
    memcpy(&cid,&gInfo->cipInfo.ConnId,sizeof(ConnIdRec)) ; 
  } while ( rum_find_connection(gInfo,cid,0,1) ) ; 
}

/**********************************/

int  cip_set_low_delay(SOCKET sfd, int nodelay, int lwm)
{
  int ival ;
  int rc = RMM_SUCCESS ; 

  if ( nodelay )
  {
    ival=1;
    if (setsockopt(sfd,IPPROTO_TCP,TCP_NODELAY,(rmm_poptval)&ival, sizeof(ival)) < 0 )
      rc = RMM_FAILURE ;
  }

  if ( lwm )
  {
    ival=lwm;
    if (setsockopt(sfd,SOL_SOCKET,SO_RCVLOWAT,(rmm_poptval)&ival, sizeof(ival)) < 0 )
      rc = RMM_FAILURE ;
  }

  return rc ; 
}
/**********************************/

int  cip_set_conn_buffers(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  int i;
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo; 
  TCHandle tcHandle = rInfo->tcHandle;

  i = 1024*gInfo->basicConfig.SocketReceiveBufferSizeKBytes ;
  if ( cip_set_socket_buffer_size(cInfo->sfd, i, SO_RCVBUF) == RMM_FAILURE )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3433),"%d",
      "The RUM connection handler failed to set the receive buffer size to {0}.",i);
    return RMM_FAILURE ; 
  }

  i = 1024*gInfo->basicConfig.SocketSendBufferSizeKBytes ;
  if ( cip_set_socket_buffer_size(cInfo->sfd, i, SO_SNDBUF) == RMM_FAILURE )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3434),"%d",
      "The RUM connection handler failed to set the send buffer size to {0}.",i);
    return RMM_FAILURE ; 
  }
  return RMM_SUCCESS ; 
}

/**********************************/

int cip_set_socket_buffer_size(SOCKET sfd, int buffer_size, int opt)
{
  int ival ; 


  if (buffer_size > 0)
  {
    ival = buffer_size;
    while (ival > 0)
    {
      if (setsockopt(sfd,SOL_SOCKET,opt,(rmm_poptval)&ival, sizeof(ival)) < 0 )
        ival = 7*ival/8;
      else break;
    }
    if (ival == 0) /* total failure */
      return RMM_FAILURE ;
  }
  return RMM_SUCCESS ; 
}

/**********************************/

int cip_create_accept_conns(rumInstanceRec *gInfo)
{
  int  i, j, n, rc, ival , iok , ok4=0, ok6=0 ; 
  int k, port, ports[2] , reuse_opt;
  int use_ib ; 
  char *netIf ; 
  SOCKET sfd , mfd=0 ; 
  socklen_t len ;
  rum_uint64 etime ; 
  SA4    *sa4 ; 
  SA6    *sa6 ; 
  ipFlat baddr[1];
  int iaddr ; 
  ConnInfoRec *cInfo=NULL ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo; 
  TCHandle tcHandle = rInfo->tcHandle;
  const char* methodName = "cip_create_accept_conns";
  char errMsg[ERR_MSG_SIZE];

  reuse_opt = SO_REUSEADDR ; 

  use_ib = (gInfo->basicConfig.Protocol==RUM_IB) ; 
  etime = gInfo->CurrentTime + rInfo->aConfig.BindRetryTime ;
  for ( k=-1 ; k<gInfo->apiConfig.SupplementalPortsLength ; k++ )
  {
    if ( k < 0 )
    {
      ports[0] = ports[1] = gInfo->basicConfig.ServerSocketPort ; 
      netIf = gInfo->basicConfig.RxNetworkInterface ; 
      if ( !ports[0] )
        continue ; 
    }
    else
    {
      rumPort *rp = &gInfo->apiConfig.SupplementalPorts[k] ; 
      if ( rp->port_number > 0 )
        ports[0] = ports[1] = rp->port_number ; 
      else
      {
        if (!rp->port_range_high )
        {
          ports[0] = PORT_RANGE_LO ; 
          ports[1] = PORT_RANGE_HI ; 
        }
        else
        {
          ports[0] = rp->port_range_low ; 
          ports[1] = rp->port_range_high; 
        }
      }
      netIf = rp->networkInterface ; 
    }
    if ( !(ports[0]>0 && ports[0]<=ports[1] && ports[1]<65536) )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3400),"%d%d%d",
        "The RUM connection handler tried to bind to the port ({0} {1} {2}) that is not valid.",
        k,ports[0],ports[1]);
      return RMM_FAILURE ; 
    }

    for ( port=ports[0] ; port<=ports[1] ; port++ )
    {
      for ( j=-1 ; j<k ; j++ )
      {
        if ((j == -1 && port == gInfo->basicConfig.ServerSocketPort) ||
            (j >=  0 && port == gInfo->apiConfig.SupplementalPorts[j].assigned_port) )
          break ; 
      }
      if ( j<k )
        continue ; 
      baddr->length = 0 ; 
      if ( gInfo->basicConfig.IPVersion != RMM_IPver_IPv6 )
      {
        do
        {
          iok = 0 ; 
          if ( (sfd=socket_(AF_INET, SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET )
          {
            if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3402),"%d",
                "The RUM connection handler failed to create an IPv4 socket. The error code is {0}.",
                rmmErrno);
            }else{
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4402),"%d",
                "The RUM connection handler failed to create an IPv4 socket. The error code is {0}.",
                rmmErrno);
            }
            break ; 
          }
          iok++ ; 
          if ( rmm_set_nb(sfd, &rc, errMsg, methodName) == RMM_FAILURE )
          {
            if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3403),"%s",
                "The RUM connection handler failed to set the IPv4 socket options. The error is: {0}",
                errMsg);
            }else{
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4403),"%s",
                "The RUM connection handler failed to set the IPv4 socket options. The error is: {0}",
                errMsg);
            }
            break ; 
          }
          if ( !use_ib && gInfo->advanceConfig.ReuseAddress )
          {
            ival = 1 ;
            if ( setsockopt(sfd, SOL_SOCKET, reuse_opt, (rmm_poptval)&ival, sizeof(ival)) < 0 )
            {
              if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3404),"%d",
                  "The RUM connection handler failed to set the SO_REUSEADDR option for the IPv4 socket. The error code is {0}.",
                  rmmErrno);
              }else{
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4404),"%d",
                  "The RUM connection handler failed to set the SO_REUSEADDR option for the IPv4 socket. The error code is {0}.",
                  rmmErrno);
              }
              break ; 
            }
          }
      
          if ( (cInfo=(ConnInfoRec *)malloc(sizeof(ConnInfoRec))) == NULL )
          {
            LOG_MEM_ERR(tcHandle,methodName,(sizeof(ConnInfoRec)));
            break ; 
          }
          iok++ ; 
          memset(cInfo,0,sizeof(ConnInfoRec)) ; 
          cInfo->gInfo = gInfo ; 
          cInfo->sfd = sfd ; 
          cInfo->sock_af = AF_INET ; 
          cInfo->lcl_sa = (SA  *)&cInfo->lcl_sas ; 
          cInfo->lcl_sa->sa_family = AF_INET ; 
          sa4 = (SA4 *)cInfo->lcl_sa ; 
          sa4->sin_port = htons((uint16_t)port) ;
          if ( memcmp(netIf,"ALL",4) != 0 )
          {
            if ( k < 0 || memcmp(netIf,"NONE",5) == 0 )
            {
              if ( IF_HAVE_IPV4(&gInfo->RxlocalIf) )
              {
                memcpy(&sa4->sin_addr,&gInfo->RxlocalIf.ipv4_addr,sizeof(IA4)) ; 
                memcpy(baddr->bytes  ,&gInfo->RxlocalIf.ipv4_addr,sizeof(IA4)) ; 
                baddr->length = sizeof(IA4) ; 
              }
              else
              {
                if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3406),"",
                    "The RUM connection handler was unable to find a local IPv4 address to bind a server socket to.");
                }else{
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4406),"",
                    "The RUM connection handler was unable to find a local IPv4 address to bind a server socket to.");
                }
                break ; 
              }
            }
            else
            {
              ipFlat addr;
              if ( rmmGetAddr(AF_INET, netIf, &addr) == AF_INET )
              {
                memcpy(&sa4->sin_addr,addr.bytes,sizeof(IA4)) ; 
                memcpy(baddr->bytes  ,addr.bytes,sizeof(IA4)) ; 
                baddr->length = sizeof(IA4) ; 
              }
              else
              {
                if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3406),"",
                    "The RUM connection handler was unable to find a local IPv4 address to bind a server socket to.");
                }else{
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4406),"",
                    "The RUM connection handler was unable to find a local IPv4 address to bind a server socket to.");
                }
                break ; 
              }
            }
          }
          len = sizeof(SA4) ; 
          do
          {
            rc = bind(cInfo->sfd, cInfo->lcl_sa, len) ; 
            if ( rc == 0 )
              break ; 
            rc = rmmErrno ; 
            if ( rc != EADDRINUSE )
              break ; 
            if ( rInfo->GlobalSync.goDN )
              break ; 
            if ( ports[0]<ports[1] )
              break ; 
            llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4400),"%d%d%d",
              "The RUM connection handler failed (errno={0}) to bind its IPv4 server socket to port {1}. The RUM connection handler will retry in 1 second and will stop bind attempt after {2} milliseconds.",
              rc, port,rInfo->aConfig.BindRetryTime);

            tsleep(1,0) ; 
          } while ( gInfo->CurrentTime < etime ) ; /* BEAM suppression: infinite loop */
          if ( rc != 0 )
          {
            if ( port == ports[1] )
            {
              if ( rc == EADDRINUSE ){
                if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3408),"%d%d",
                    "The RUM connection handler failed (errno={0}) to bind its IPv4 server socket to port {1}. The port is in use. Verify that the address and port are not in use by another process.",
                    rc, port);
                }else{
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4408),"%d%d",
                    "The RUM connection handler failed (errno={0}) to bind its IPv4 server socket to port {1}. The port is in use. Verify that the address and port are not in use by another process.",
                    rc, port);
                }
              } else {
                if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3409),"%d%d",
                    "The RUM connection handler failed (errno={0}) to bind its IPv4 server socket to port {1}.",
                    rc, port);
                }else{
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4409),"%d%d",
                    "The RUM connection handler failed (errno={0}) to bind its IPv4 server socket to port {1}.",
                    rc, port);
                }
              }
            }
            break ; 
          }
    
          {
            if ( cip_set_conn_buffers(gInfo, cInfo) ==  RMM_FAILURE )
              break ; 
          if ( listen(cInfo->sfd, 64) == SOCKET_ERROR)
          {
            if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3411),"%d",
                "The RUM connection handler failed to listen for the IPv4 socket. The error code is {0}.",rmmErrno);
            }else{
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4411),"%d",
                "The RUM connection handler failed to listen for the IPv4 socket. The error code is {0}.",rmmErrno);
            }
            break ; 
          }
          }

          cip_update_conn_list(gInfo, cInfo, 1) ;  
          DBG_PRT(printf(" cip_create_accept_conns: ipv4 con added for port %d, gInfo->cipInfo.nConnsInProg=%d\n",port,gInfo->cipInfo.nConnsInProg)) ; 
    
          memset(&cInfo->last_r_time,0xff,sizeof(cInfo->last_r_time)) ; 
          cInfo->lcl_addr.length = sizeof(IA4) ; 
          memcpy(cInfo->lcl_addr.bytes,&sa4->sin_addr,sizeof(IA4)) ; 
          cInfo->lcl_port = port ;
          cInfo->io_state = (use_ib) ? CIP_IB_STATE_CREQ : CIP_IO_STATE_ACC ;
         #if USE_POLL
          gInfo->cipInfo.fds[cInfo->ind].fd = cInfo->sfd ; 
          gInfo->cipInfo.fds[cInfo->ind].events = POLLIN ; 
         #else
          if ( mfd < cInfo->sfd )
               mfd = cInfo->sfd ;
          FD_SET(cInfo->sfd,&gInfo->cipInfo.rfds) ; 
         #endif
          rmm_ntop(AF_INET,cInfo->lcl_addr.bytes,cInfo->req_addr,RUM_ADDR_STR_LEN) ;
          cInfo->req_port = cInfo->lcl_port ; 
          iok = 4 ; 
        } while ( 0 ) ; 
        if ( iok < 4 )
        {
          if ( iok > 1 )
          {
            free(cInfo) ; 
          }
          if ( iok > 0 )
          {
            closesocket(sfd) ; 
          }
          if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY )
          {
            continue ; 
          }
          else
          {
          }
        }
        ok4 = ( iok == 4 ) ; 
      }
    
      if ( !use_ib && gInfo->basicConfig.IPVersion != RMM_IPver_IPv4 )
      {
        ok6 = 1 ; 
        if ( memcmp(netIf,"ALL",4) == 0 )
          n = 1 ; 
        else
        if ( k < 0 || memcmp(netIf,"NONE",5) == 0 )
          n = 3 ; 
        else
          n = 1 ; 

        for ( i=0 ; i<n ; i++ )
        {
          do
          {
            iok = 0 ; 
            if ( (sfd=socket_(AF_INET6, SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET )
            {
              if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3412),"%d",
                  "The RUM connection handler failed to create an IPv6 socket. The error code is {0}.",
                  rmmErrno);
              }else{
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4412),"%d",
                  "The RUM connection handler failed to create an IPv6 socket. The error code is {0}.",
                  rmmErrno);
              }
              break ; 
            }
            iok++ ; 
            if ( rmm_set_nb(sfd, &rc, errMsg, methodName) == RMM_FAILURE ){
              if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3413),"%s",
                  "The RUM connection handler failed to set the IPv6 socket options. The error is: {0}",
                  errMsg);
              }else{
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4413),"%s",
                  "The RUM connection handler failed to set the IPv6 socket options. The error is: {0}",
                  errMsg);
              }
              break ; 
            }
      
            if ( !use_ib && gInfo->advanceConfig.ReuseAddress )
            {
              ival = 1 ;
              if ( setsockopt(sfd, SOL_SOCKET, reuse_opt, (rmm_poptval)&ival, sizeof(ival)) < 0 )
              {
                if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3414),"%d",
                    "The RUM connection handler failed to set the SO_REUSEADDR option for the IPv6 socket. The error code is {0}.",
                    rmmErrno);
                }else{
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4414),"%d",
                    "The RUM connection handler failed to set the SO_REUSEADDR option for the IPv6 socket. The error code is {0}.",
                    rmmErrno);
                }
                break ; 
              }
            }
    
           #ifdef IPV6_V6ONLY
            DBG_PRT(printf("_debug_0 IPV6_V6ONLY !! \n")) ;
            ival = 1 ;
            if ( setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, (rmm_poptval)&ival, sizeof(ival)) < 0 )
            {
              if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3474),"%d",
                  "The RUM connection handler failed to set the IPV6_V6ONLY option for the IPv6 socket. The error code is {0}.",
                  rmmErrno);
              }else{
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4474),"%d",
                  "The RUM connection handler failed to set the IPV6_V6ONLY option for the IPv6 socket. The error code is {0}.",
                  rmmErrno);
              }
              break ; 
            }
            DBG_PRT(printf("_debug_ IPV6_V6ONLY !! \n")) ;
           #endif
        
            if ( (cInfo=(ConnInfoRec *)malloc(sizeof(ConnInfoRec))) == NULL )
            {
        LOG_MEM_ERR(tcHandle,methodName,(sizeof(ConnInfoRec)));
              break ; 
            }
            iok++ ; 
            memset(cInfo,0,sizeof(ConnInfoRec)) ; 
            cInfo->gInfo = gInfo ; 
            cInfo->sfd = sfd ; 
            cInfo->sock_af = AF_INET6 ; 
            cInfo->lcl_sa = (SA  *)&cInfo->lcl_sas ; 
            cInfo->lcl_sa->sa_family = AF_INET6 ; 
            sa6 = (SA6 *)cInfo->lcl_sa ; 
            sa6->sin6_port = htons((uint16_t)port) ;
            iaddr = 0 ; 
            if ( memcmp(netIf,"ALL",4) != 0 )
            {
              if ( i == 0 )
              {
                if ( n == 1 )
                {
                  ipFlat addr;
                  if ( rmmGetAddr(AF_INET6, netIf, &addr) == AF_INET6 )
                  {
                    iaddr = 1 ; 
                    memcpy(&sa6->sin6_addr,addr.bytes,sizeof(IA6)) ; 
                    if ( IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr) )
                      sa6->sin6_scope_id = gInfo->RxlocalIf.link_scope ; 
                    else
                    if ( IN6_IS_ADDR_SITELOCAL(&sa6->sin6_addr) )
                      sa6->sin6_scope_id = gInfo->RxlocalIf.site_scope ; 
                    else
                      sa6->sin6_scope_id = gInfo->RxlocalIf.glob_scope ; 
                  }
                  else
                  {
                    if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3416),"",
                        "The RUM connection handler was unable to find a local IPv6 address to bind a server socket to.");
                    }else{
                      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4416),"",
                        "The RUM connection handler was unable to find a local IPv6 address to bind a server socket to.");
                    }
                    i = 3 ; 
                  }
                }
                else
                if ( IF_HAVE_LINK(&gInfo->RxlocalIf) )
                {
                  iaddr = 1 ; 
                  memcpy(&sa6->sin6_addr,&gInfo->RxlocalIf.link_addr,sizeof(IA6)) ; 
                  sa6->sin6_scope_id = gInfo->RxlocalIf.link_scope ; 
                }
                else
                  i++ ; 
              }
              if ( i == 1 )
              {
                if ( IF_HAVE_SITE(&gInfo->RxlocalIf) )
                {
                  iaddr = 1 ; 
                  memcpy(&sa6->sin6_addr,&gInfo->RxlocalIf.site_addr,sizeof(IA6)) ; 
                  sa6->sin6_scope_id = gInfo->RxlocalIf.site_scope ; 
                }
                else
                  i++ ; 
              }
              if ( i == 2 )
              {
                if ( IF_HAVE_GLOB(&gInfo->RxlocalIf) )
                {
                  iaddr = 1 ; 
                  memcpy(&sa6->sin6_addr,&gInfo->RxlocalIf.glob_addr,sizeof(IA6)) ; 
                  sa6->sin6_scope_id = gInfo->RxlocalIf.glob_scope ; 
                }
                else
                  i++ ; 
              }
              if ( i > 2 )
                break ; 
            }
            len = sizeof(SA6) ; 
            if ( !baddr->length && iaddr )
            {
              memcpy(baddr->bytes,&sa6->sin6_addr,sizeof(IA6)) ; 
              baddr->length = sizeof(IA6) ; 
            }
            do
            {
              if ( (rc=bind(cInfo->sfd, cInfo->lcl_sa, len)) == 0 )
                break ; 
              rc = rmmErrno ; 
              if ( rc != EADDRINUSE )
                break ; 
              if ( rInfo->GlobalSync.goDN )
                break ; 
              if ( !ok4 && ports[0]<ports[1] )
                break ; 
              llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4415),"%d%d%d",
                "The RUM connection handler failed (errno={0}) to bind its IPv6 server socket to port {1}. The RUM connection handler will retry in 1 second and will stop bind attempt after {2} milliseconds.",
                rc, port,rInfo->aConfig.BindRetryTime);
              tsleep(1,0) ; 
            } while ( gInfo->CurrentTime < etime ) ; 
            if ( rc != 0 )
            {
              if ( rc == EADDRINUSE ){
                if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3418),"%d%d",
                    "The RUM connection handler failed (errno={0}) to bind its IPv6 server socket to port {1}. The port is in use. Verify that the address and port are not used by another process.",
                    rc, port);
                }else{
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4418),"%d%d",
                    "The RUM connection handler failed (errno={0}) to bind its IPv6 server socket to port {1}. The port is in use. Verify that the address and port are not used by another process.",
                    rc, port);
                }
              } else {
                if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3419),"%d%d",
                    "The RUM connection handler failed (errno={0}) to bind its IPv6 server socket to port {1}.",
                    rc, port);
                }else{
                  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4419),"%d%d",
                    "The RUM connection handler failed (errno={0}) to bind its IPv6 server socket to port {1}.",
                    rc, port);
                }
              }
              break ; 
            }
      
            if ( listen(cInfo->sfd, 64) == SOCKET_ERROR)
            {
              if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY ){
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3421),"%d",
                  "The RUM connection handler failed to listen for the IPv6 socket. The error code is {0}.",rmmErrno);
              }else{
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4435),"%d",
                  "The RUM connection handler failed to listen for the IPv6 socket. The error code is {0}.",rmmErrno);
              }
              break ; 
            }

            cip_update_conn_list(gInfo, cInfo, 1) ;  
            DBG_PRT(printf(" cip_create_accept_conns: ipv6 con added for port %d, gInfo->cipInfo.nConnsInProg=%d\n",port,gInfo->cipInfo.nConnsInProg)) ; 
      
            memset(&cInfo->last_r_time,0xff,sizeof(cInfo->last_r_time)) ; 
            cInfo->lcl_addr.length = sizeof(IA6) ; 
            memcpy(cInfo->lcl_addr.bytes,&sa6->sin6_addr,sizeof(IA6)) ; 
            cInfo->lcl_port = port ;
            cInfo->io_state = CIP_IO_STATE_ACC ;
           #if USE_POLL
            gInfo->cipInfo.fds[cInfo->ind].fd = cInfo->sfd ; 
            gInfo->cipInfo.fds[cInfo->ind].events = POLLIN ; 
           #else
            if ( mfd < cInfo->sfd )
                 mfd = cInfo->sfd ;
            FD_SET(cInfo->sfd,&gInfo->cipInfo.rfds) ; 
           #endif
            rmm_ntop(AF_INET6,cInfo->lcl_addr.bytes,cInfo->req_addr,RUM_ADDR_STR_LEN) ;
            cInfo->req_port = cInfo->lcl_port ; 
            iok = 4 ; 
          } while ( 0 ) ; 
          if ( iok < 4 )
          {
            if ( iok > 0 )
              closesocket(sfd) ; 
            if ( iok > 1 )
              free(cInfo) ; 
            if ( gInfo->basicConfig.IPVersion != RMM_IPver_ANY )
            {
              continue ; 
            }
            else
            {
            }
          }
          ok6 &= ( iok == 4 ) ; /* BEAM suppression: not boolean */
        }
      }
      if ( mfd ) 
        mfd++ ; 
     #if !USE_POLL
      if ( gInfo->cipInfo.np1 < mfd )
           gInfo->cipInfo.np1 = mfd ;
     #endif
    
      if ((gInfo->basicConfig.IPVersion == RMM_IPver_IPv4 && !ok4)       ||
          (gInfo->basicConfig.IPVersion == RMM_IPver_IPv6 && !ok6)       ||
          (gInfo->basicConfig.IPVersion == RMM_IPver_ANY  && !(ok4|ok6)) ||
          (gInfo->basicConfig.IPVersion == RMM_IPver_BOTH && !(ok4&ok6)) )
        continue ; 
      if ( k >= 0 )
        gInfo->apiConfig.SupplementalPorts[k].assigned_port = port ; 
      if ( gInfo->basicConfig.ServerSocketPort <= 0 )
      {
        gInfo->basicConfig.ServerSocketPort = port ; 
        gInfo->port = htons((uint16_t)port) ; 
      }
      if ( baddr->length && !gInfo->addr.length )
        gInfo->addr = baddr[0] ; 
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5400),"%d%d%d",
      "The RUM instance {0} listens on port {1} for index {2}.",gInfo->instance,port,k);

      break ; 
    }
    if ( port > ports[1] )
      return RMM_FAILURE ; 
  }

  return RMM_SUCCESS ; 
}

/*******************************/

/*******************************/

int cip_handle_conn_req(rumInstanceRec *gInfo, ConnInfoRec *cInfo) 
{
  int ok, rc , errCode , place=0  ; 
  SOCKET   sfd=0 ; 
  ipInfo   tmp_ipi ; 
  rumConnectionEvent ev ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
 #if CIP_DBG
  char addr[RUM_ADDR_STR_LEN] ; 
 #endif
  char port[8] , errMsg[ERR_MSG_SIZE];
  TCHandle tcHandle = rInfo->tcHandle;
  const char* methodName = "cip_handle_conn_req";


  cInfo->apiInfo.rum_instance = gInfo->instance ; 
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5401),"%d%s%d",
    "The RUM instance {0} is handling a connection request to {1}|{2}.",gInfo->instance,cInfo->req_addr,cInfo->req_port);


  place = 0 ; 
  ok = 0 ; 
  do
  {
    SA4 *sa4 ; 
    SA6 *sa6 ; 
    int af , allif ; 
    snprintf(port,8,"%d",cInfo->req_port) ; 
    IP_INIT(&tmp_ipi) ; 
    allif = ( memcmp(gInfo->basicConfig.TxNetworkInterface,"ALL" ,4) == 0 ||
              memcmp(gInfo->basicConfig.TxNetworkInterface,"NONE",5) == 0 ) ; 
    af = allif ? AF_UNSPEC : gInfo->TxlocalIf.have_af ; 
    if ( rmmGetAddrInfo(cInfo->req_addr,port,af,SOCK_STREAM,IPPROTO_TCP,0,&tmp_ipi.ai,&errCode,errMsg) != RMM_SUCCESS )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3423),"%s%s%s",
        "RUM failed to resolve an IP address {0}|{1}. The error is: {2}.",
        cInfo->req_addr,port,errMsg);
      break ; 
    }
    place++ ; 

    if ( (sfd=socket_(tmp_ipi.ai.ai_family,SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3430),"%d",
        "The RUM connection handler failed to create a socket. The error code is {0}.",rmmErrno);
      break ; 
    }
    ok++ ; 
    cInfo->sfd = sfd ; 
    place++ ; 
    if ( rmm_set_nb(sfd, &rc, errMsg, methodName) == RMM_FAILURE )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3432),"%s",
        "The RUM connection handler failed to set the socket options. The error is: {0}.",errMsg);
      break;
    }
    place++ ; 
  
    if ( cip_set_conn_buffers(gInfo, cInfo) ==  RMM_FAILURE )
    {
      break ; 
    }
    place++ ; 

    if ( cip_set_low_delay(sfd, 1, 0) == RMM_FAILURE )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4420),"%d",
        "The RUM connection handler failed to set the socket TCP_NODELAY mode. The reason code is {0}.",rmmErrno);
      /* break ; we can leave without it */
    }
    place++ ; 

    if ( cip_set_low_delay(sfd, 0, 1) == RMM_FAILURE )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(4421),"%d",
        "The RUM connection handler failed to set the socket SO_RCVLOWAT to 1. The reason code is {0}.",rmmErrno);
      /* break ; we can leave without it */
    }
    place++ ; 
  
    cInfo->init_here = 1 ; 
    cInfo->last_r_time  = gInfo->CurrentTime + cInfo->cip_to ; 
    cInfo->sock_af = tmp_ipi.ai.ai_family ; 
    if ( cInfo->sock_af == AF_INET6 )
    {
      IA6 *ia6 ; 
      sa6 = (SA6 *)tmp_ipi.ai.ai_addr ; 
      ia6 = &sa6->sin6_addr ; 
      if ( IN6_IS_ADDR_LINKLOCAL(ia6) && IF_HAVE_LINK(&gInfo->TxlocalIf) )
      {
        sa6->sin6_scope_id = gInfo->TxlocalIf.link_scope ;
      }
    }

    if (!allif )
    {
      socklen_t len ; 
      cInfo->lcl_sa = (SA *)&cInfo->lcl_sas ; 
      if ( cInfo->sock_af == AF_INET )
      {
        sa4 = (SA4 *)cInfo->lcl_sa ; 
        memset(sa4,0,sizeof(SA4)) ; 
        memcpy(&sa4->sin_addr,&gInfo->TxlocalIf.ipv4_addr,sizeof(IA4)) ; 
        len = sizeof(SA4) ; 
      }
      else
      {
        IA6 *ia6=NULL ; 
        sa6 = (SA6 *)cInfo->lcl_sa ; 
        memset(sa6,0,sizeof(SA6)) ; 
        if ( IF_HAVE_GLOB(&gInfo->TxlocalIf) )
          ia6 = &gInfo->TxlocalIf.glob_addr ; 
        else
        if ( IF_HAVE_SITE(&gInfo->TxlocalIf) )
          ia6 = &gInfo->TxlocalIf.site_addr ; 
        else
        {
          ia6 = &gInfo->TxlocalIf.link_addr ; 
          sa6->sin6_scope_id = gInfo->TxlocalIf.link_scope ;
        }
        if ( ia6 )
          memcpy(&sa6->sin6_addr,ia6,sizeof(IA6)) ; 
        len = sizeof(SA6) ; 
      }
      cInfo->lcl_sa->sa_family = cInfo->sock_af ; 
      if ( bind(sfd, cInfo->lcl_sa, len) )
      {
        rc = rmmErrno ; 
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3435),"%d",
          "The RUM connection handler failed to bind a socket to the local TxInterface. The error code is {0}.",rc);
        break ; 
      }
    }

   #if CIP_DBG
    if ( rmmGetNameInfo(tmp_ipi.ai.ai_addr,addr,64,port,8,&errCode,errMsg) == RMM_SUCCESS )
      printf("connecting to: %s|%d (%s|%s)\n",cInfo->req_addr,cInfo->req_port,addr,port) ; 
    else
      printf("connecting to: %s|%d (%s|%s)\n",cInfo->req_addr,cInfo->req_port,"?","?") ; 
   #endif

    cip_update_conn_list(gInfo, cInfo, 1) ;  
    DBG_PRT(printf(" cip_handle_conn_req: con added , gInfo->cipInfo.nConnsInProg=%d\n",gInfo->cipInfo.nConnsInProg)) ; 

   #if USE_POLL
    gInfo->cipInfo.fds[cInfo->ind].fd = cInfo->sfd ; 
   #else
    if ( gInfo->cipInfo.np1 < (sfd+1) )
      gInfo->cipInfo.np1 = (sfd+1) ; 
   #endif
  
    if ( connect(sfd,tmp_ipi.ai.ai_addr,(socklen_t)tmp_ipi.ai.ai_addrlen) == 0 )
    {
      cInfo->io_state = CIP_IO_STATE_SSL_HS ;
    }
    else
    {
    place++ ; 
      rc = rmmErrno ;
      if ( rc == EINPROGRESS || rc == EWOULDBLOCK )
      {
        DBG_PRT(printf(" rmmErrno == EINPROGRESS\n")) ; 
        cInfo->io_state = CIP_IO_STATE_CONN ; 
       #if USE_POLL
        gInfo->cipInfo.fds[cInfo->ind].events = (POLLIN|POLLOUT) ; 
       #else
        FD_SET(cInfo->sfd,&gInfo->cipInfo.rfds) ;
        FD_SET(cInfo->sfd,&gInfo->cipInfo.wfds) ;
       #endif
      }
      else
      {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6436),"%d",
          "RUM connection handler failed to connect the socket. The error is: {0}.",rmmErrno);
        cip_update_conn_list(gInfo, cInfo, 0) ;  
        break ; 
      }
    place++ ; 
    }
    ok++ ; 
  } while ( 0 ) ; 
  if ( ok > 1 ) 
    return RMM_SUCCESS ; 
  else
  if ( ok > 0 ) 
    closesocket(sfd) ; 

  memset(&ev,0,sizeof(ev)) ; 
  ev.type = RUM_CONNECTION_ESTABLISH_FAILURE ; 

  ev.nparams = 1 ; 
  ev.event_params = (void *)gInfo->cipInfo.ev_dbg ; 
  snprintf(gInfo->cipInfo.ev_msg,EV_MSG_SIZE,"end of cip_handle_conn_req %d %d",place,ok) ; 

  ev.connection_info.rum_instance = gInfo->instance ; 
  ev.connection_info.connection_state = RUM_CONNECTION_STATE_CLOSED ; 
  ev.connection_info.remote_server_port = cInfo->req_port ; 
  strlcpy(ev.connection_info.remote_addr,cInfo->req_addr,RUM_ADDR_STR_LEN);

  cInfo->conn_listener->on_event(&ev,cInfo->conn_listener->user) ; 
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_EVENT,MSG_KEY(6438),"%s%d",
    "RUM connection handler failed to establish connection to {0}|{1}.",
    cInfo->req_addr,cInfo->req_port);

  if ( gInfo->free_callback )
    gInfo->free_callback(cInfo->conn_listener->user) ; 
  free(cInfo->conn_listener) ; 
  if ( cInfo->msg_len && cInfo->msg_buf )
    free(cInfo->msg_buf) ; 
  free(cInfo) ; 
  return RMM_FAILURE ; 
}

/******************************/

/******************************/

int cip_set_local_endpoint(rmmReceiverRec *rInfo,ConnInfoRec *cInfo) 
{
  socklen_t len ;
  SA4 *sa4 ; 
  SA6 *sa6 ; 
  TCHandle tcHandle = rInfo->tcHandle;

  cInfo->lcl_sa = (SA *)&cInfo->lcl_sas ; 
  len = sizeof(SAS) ; 
  if ( getsockname(cInfo->sfd,cInfo->lcl_sa,&len) == -1 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3450),"%d",
      "The RUM connection handler call to the getsockname function has failed. The error code is {0}.",
     rmmErrno);
    return RMM_FAILURE ; 
  }
  if ( cInfo->lcl_sa->sa_family == AF_INET )
  {
    sa4 = (SA4 *)cInfo->lcl_sa ; 
    cInfo->lcl_addr.length = sizeof(IA4) ; 
    memcpy(cInfo->lcl_addr.bytes,&sa4->sin_addr,sizeof(IA4)) ; 
    cInfo->lcl_port = ntohs(sa4->sin_port) ; 
  }
  else
  {
    sa6 = (SA6 *)cInfo->lcl_sa ; 
    cInfo->lcl_addr.length = sizeof(IA6) ; 
    memcpy(cInfo->lcl_addr.bytes,&sa6->sin6_addr,sizeof(IA6)) ; 
    cInfo->lcl_port = ntohs(sa6->sin6_port) ; 
  }
  return RMM_SUCCESS ; 
}

/******************************/

int cip_set_remote_endpoint(rmmReceiverRec *rInfo,ConnInfoRec *cInfo) 
{
  socklen_t len ;
  SA4 *sa4 ; 
  SA6 *sa6 ; 
  TCHandle tcHandle = rInfo->tcHandle;

  cInfo->rmt_sa = (SA *)&cInfo->rmt_sas ; 
  len = sizeof(SAS) ; 
  if ( getpeername(cInfo->sfd,cInfo->rmt_sa,&len) == -1 )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3451),"%d",
      "The RUM connection handler call to the getpeername function has failed. The error code is {0}.",
     rmmErrno);
    return RMM_FAILURE ; 
  }
  if ( cInfo->rmt_sa->sa_family == AF_INET )
  {
    sa4 = (SA4 *)cInfo->rmt_sa ; 
    cInfo->rmt_addr.length = sizeof(IA4) ; 
    memcpy(cInfo->rmt_addr.bytes,&sa4->sin_addr,sizeof(IA4)) ; 
    cInfo->rmt_port = ntohs(sa4->sin_port) ; 
  }
  else
  {
    sa6 = (SA6 *)cInfo->rmt_sa ; 
    cInfo->rmt_addr.length = sizeof(IA6) ; 
    memcpy(cInfo->rmt_addr.bytes,&sa6->sin6_addr,sizeof(IA6)) ; 
    cInfo->rmt_port = ntohs(sa6->sin6_port) ; 
  }
  return RMM_SUCCESS ; 
}

/******************************/

int cip_prepare_s_cfp_req(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  int n ; 
  uint16_t m ; 
  uint8_t cval ; 
  char *bptr ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
  TCHandle tcHandle = rInfo->tcHandle;
  const char* methodName = "cip_prepare_s_cfp_req";


  n = 4 + sizeof(rumHeaderCCP) ; 
  if ( cInfo->msg_len )
    n += 8 + cInfo->msg_len ; 
  if ( n < 512 )
    n = (n+63)/64*64 ;
  else
    n = (n+1023)/1024*1024 ; 
  if ( (cInfo->wrInfo.buffer = malloc(n)) == NULL )
  {     
    LOG_MEM_ERR(tcHandle,methodName,n);
    return RMM_FAILURE ; 
  }
  cInfo->wrInfo.buflen = n ; 
  cInfo->wrInfo.need_free = 1 ; 

  cip_next_conn_id(gInfo) ; 
  memcpy(&cInfo->connection_id,&gInfo->cipInfo.ConnId,sizeof(ConnIdRec)) ;
  n = cip_prepare_ccp_header(gInfo,cInfo,cInfo->wrInfo.buffer,RUM_CFP_REQ) ; 

  bptr = cInfo->wrInfo.buffer + 9 ; 
  NputChar(bptr,(cInfo->msg_len>0)) ; /* have_opts? */
  bptr = cInfo->wrInfo.buffer + n ; 

  if ( cInfo->msg_len )
  {
    NputChar(bptr,PGM_OPT_LENGTH) ; 
    NputChar(bptr,4) ; 
    m = 8 + cInfo->msg_len ; 
    NputShort(bptr,m) ; 
    cval = (PGM_OPT_END|RUM_OPT_CONNECTION_MSG) ; 
    DBG_PRT(printf("in cip_prepare_s_cfp_req: cval= %2.2x\n",cval)) ; 
    NputChar(bptr,cval) ; 
    NputChar(bptr,0) ; 
    m = 4 + cInfo->msg_len ; 
    NputShort(bptr,m) ; 
    memcpy(bptr,cInfo->msg_buf,cInfo->msg_len) ; 
    bptr += cInfo->msg_len ; 
  }
  n = (int)(bptr - cInfo->wrInfo.buffer) - 4 ; 
  bptr = cInfo->wrInfo.buffer ; 
  NputInt(bptr,n) ; 

  cInfo->wrInfo.reqlen = n + 4 ; 
  cInfo->wrInfo.offset = 0 ; 
  cInfo->wrInfo.bptr   = cInfo->wrInfo.buffer ; 

  return RMM_SUCCESS ; 
}

/******************************/

int cip_prepare_s_cfp_rep(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  int n ; 

  n = sizeof(cInfo->ccp_header) ; 
  cInfo->wrInfo.buffer = (char *)cInfo->ccp_header ; 
  cInfo->wrInfo.buflen = n ; 

  n = cip_prepare_ccp_header(gInfo,cInfo,cInfo->wrInfo.buffer, RUM_CFP_REP) ; 
  cInfo->wrInfo.buflen = n ; 
  cInfo->wrInfo.reqlen = n ; 
  cInfo->wrInfo.offset = 0 ; 
  cInfo->wrInfo.bptr   = cInfo->wrInfo.buffer ; 

  return RMM_SUCCESS ; 
}

/******************************/

int cip_prepare_s_cfp_ack(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  int n ; 

  n = cip_prepare_ccp_header(gInfo,cInfo,cInfo->wrInfo.buffer,RUM_CFP_ACK) ; 

  cInfo->wrInfo.reqlen = n ; 
  cInfo->wrInfo.offset = 0 ; 
  cInfo->wrInfo.bptr   = cInfo->wrInfo.buffer ; 

  return RMM_SUCCESS ; 
}

/*******************************/

int cip_prepare_r_cfp_0(rumInstanceRec *gInfo, ConnInfoRec *cInfo, uint32_t n)
{
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
  TCHandle tcHandle = rInfo->tcHandle;
  const char* methodName = "cip_prepare_r_cfp_0";


  if ( n <= cInfo->rdInfo.buflen ) 
  {
    n = cInfo->rdInfo.buflen ;
  }
  else
  if ( n <= sizeof(cInfo->ccp_header) ) 
  {
    cInfo->rdInfo.buffer = (char *)cInfo->ccp_header ; 
    n = sizeof(cInfo->ccp_header) ; 
  }
  else
  {
    if ( (cInfo->rdInfo.buffer = malloc(n)) == NULL )
    {
    LOG_MEM_ERR(tcHandle,methodName,n);
      return RMM_FAILURE ; 
    }
    cInfo->rdInfo.need_free = 1 ; 
  }
  cInfo->rdInfo.buflen = n ; 

  cInfo->rdInfo.reqlen = 4 ; 
  cInfo->rdInfo.offset = 0 ; 
  cInfo->rdInfo.bptr   = cInfo->rdInfo.buffer ; 

  cInfo->io_state = CIP_IO_STATE_READ ; 
 #if USE_POLL
  gInfo->cipInfo.fds[cInfo->ind].events = POLLIN ;  
 #else
  FD_SET(cInfo->sfd,&gInfo->cipInfo.rfds) ;
 #endif

  return RMM_SUCCESS ; 
}

/*******************************/

int cip_prepare_r_cfp_1(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  uint32_t n ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ;
  TCHandle tcHandle = rInfo->tcHandle;
  const char* methodName = "cip_prepare_r_cfp_1";

  cInfo->rdInfo.bptr   = cInfo->rdInfo.buffer ; 
  NgetInt(cInfo->rdInfo.bptr,n) ; 

  if ( cInfo->rdInfo.buflen < n+4 ) 
  {
    char *p ; 
    if ( cInfo->rdInfo.need_free ) 
      p = realloc(cInfo->rdInfo.buffer,(n+4)) ; 
    else
      p = malloc(n+4) ; 
    if ( !p )
    {       
    LOG_MEM_ERR(tcHandle,methodName,(n+4));
      return RMM_FAILURE ; 
    }
    cInfo->rdInfo.buffer = p ; 
    cInfo->rdInfo.bptr   = p + 4 ; 
    cInfo->rdInfo.need_free = 1 ; 
    cInfo->rdInfo.buflen = n+4 ; 
  }

  cInfo->rdInfo.reqlen = n ; 
  cInfo->rdInfo.offset = 0 ; 

  cInfo->io_state = CIP_IO_STATE_READ ; 
 #if USE_POLL
  gInfo->cipInfo.fds[cInfo->ind].events = POLLIN ;  
 #else
  FD_SET(cInfo->sfd,&gInfo->cipInfo.rfds) ;
 #endif

  return RMM_SUCCESS ; 
}


/******************************/

int cip_check_to(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  rumConnectionEvent ev ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ;
  TCHandle tcHandle = rInfo->tcHandle;

  if ( cInfo->last_r_time >= gInfo->CurrentTime )
    return RMM_SUCCESS ; 

  memset(&ev,0,sizeof(ev)) ; 
  ev.type = RUM_CONNECTION_ESTABLISH_TIMEOUT ; 

  if ( cInfo->init_here )
  {
    ev.connection_info.rum_instance = gInfo->instance ; 
    ev.connection_info.remote_server_port = cInfo->req_port ; 
    strlcpy(ev.connection_info.remote_addr,cInfo->req_addr,RUM_ADDR_STR_LEN);
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4430),"%s%d",
      "The RUM_CONNECTION_ESTABLISH_TIMEOUT was raised on an outgoing connection to {0}|{1}.",
      cInfo->req_addr,cInfo->req_port);
  }
  else
  {
    char cid_str[20] ; 
    memcpy(&ev.connection_info,&cInfo->apiInfo,sizeof(rumConnection)) ;
    b2h(cid_str,(unsigned char *)&cInfo->connection_id,sizeof(rumConnectionID_t)) ;
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4431),"%s",
      "The RUM_CONNECTION_ESTABLISH_TIMEOUT was raised on incoming connection {0}.",
      cid_str);
  }
  ev.connection_info.connection_state = RUM_CONNECTION_STATE_CLOSED ; 
  if ( cInfo->conn_listener && cInfo->conn_listener->is_valid) 
    cInfo->conn_listener->on_event(&ev,cInfo->conn_listener->user) ;
  cip_remove_conn(gInfo,cInfo) ; 
  return RMM_FAILURE ; 
}

/******************************/

int cip_remove_conn(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
 #if !USE_POLL
  SOCKET mfd=0; 
 #endif

  if ( cip_update_conn_list(gInfo, cInfo, 0) < 0 )
    return RMM_FAILURE ; 

  DBG_PRT(printf(" cip_remove_conn: gInfo->cipInfo.nConnsInProg=%d\n",gInfo->cipInfo.nConnsInProg)) ; 


 #if !USE_POLL
  FD_CLR(cInfo->sfd,&gInfo->cipInfo.wfds) ;
  FD_CLR(cInfo->sfd,&gInfo->cipInfo.rfds) ;
  FD_CLR(cInfo->sfd,&gInfo->cipInfo.efds) ;
 #endif
 #if USE_SSL
  if ( gInfo->use_ssl && cInfo->sslInfo->ssl )
  {
    SSL_shutdown(cInfo->sslInfo->ssl) ; 
    SSL_free    (cInfo->sslInfo->ssl) ; 
    pthread_mutex_destroy(cInfo->sslInfo->lock) ; 
  }
 #endif
  closesocket(cInfo->sfd) ; 
  if ( cInfo->msg_len && cInfo->msg_buf )
    free(cInfo->msg_buf) ; 
  if ( cInfo->rdInfo.buffer && cInfo->rdInfo.need_free )
    free(cInfo->rdInfo.buffer) ; 
  if ( cInfo->wrInfo.buffer && cInfo->wrInfo.need_free )
    free(cInfo->wrInfo.buffer) ; 
  if ( cInfo->init_here )
  {
    if ( gInfo->free_callback )
      gInfo->free_callback(cInfo->conn_listener->user) ; 
    free(cInfo->conn_listener) ; 
  }
  else
  if ( cInfo->conn_listener )
  {
    pthread_mutex_lock(&gInfo->ConnectionListenersMutex) ; 
    cInfo->conn_listener->n_cip-- ; 
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
  free(cInfo) ; 

 #if !USE_POLL
  for ( cInfo=gInfo->cipInfo.firstConnInProg ; cInfo ; cInfo=cInfo->next )
  {
    if ( mfd < cInfo->sfd+1 )
         mfd = cInfo->sfd+1 ;
  }
  gInfo->cipInfo.np1 = mfd ; 
 #endif

  return RMM_SUCCESS ; 
}

/******************************/

void cip_remove_all_conns(rumInstanceRec *gInfo)
{
  ConnInfoRec *cInfo=NULL ; 

  while ( gInfo->cipInfo.firstConnInProg )
  {
    cInfo = gInfo->cipInfo.firstConnInProg; 
    cip_conn_failed(gInfo,cInfo) ; 
    DBG_PRT(printf(" cip_remove_all_conns: con removed , gInfo->cipInfo.nConnsInProg=%d\n",gInfo->cipInfo.nConnsInProg)) ; 
  }

  return ; 
}

/*******************************/

int cip_build_new_incoming_conn(rumInstanceRec *gInfo, SOCKET sfd) 
{
  int ok, rc , place=0 ; 
  ConnInfoRec *cInfo=NULL ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ;
  TCHandle tcHandle = rInfo->tcHandle;
  const char* methodName = "cip_build_new_incoming_conn";
  char errMsg[ERR_MSG_SIZE];

  ok = 0 ; 
  do
  {
    place++ ; 
    if ( rmm_set_nb(sfd, &rc, errMsg, methodName) == RMM_FAILURE )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3445),"%s",
        "The RUM connection handler failed to set the socket options. The error is: {0}.",errMsg);
      break;
    }
  
    if ( cip_set_low_delay(sfd, 1, 0) == RMM_FAILURE )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4420),"%d",
        "The RUM connection handler failed to set the socket TCP_NODELAY mode. The reason code is {0}.",rmmErrno);
      /* break ; we can leave without it */
    }
    place++ ; 

    if ( cip_set_low_delay(sfd, 0, 1) == RMM_FAILURE )
    {
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_DEBUG,MSG_KEY(4421),"%d",
        "The RUM connection handler failed to set the socket SO_RCVLOWAT to 1. The reason code is {0}.",rmmErrno);
      /* break ; we can leave without it */
    }
    place++ ; 

    place++ ; 
    if ( (cInfo=(ConnInfoRec *)malloc(sizeof(ConnInfoRec))) == NULL )
    {
    LOG_MEM_ERR(tcHandle,methodName,(sizeof(ConnInfoRec)));
      break ; 
    }
    ok++ ; 
    memset(cInfo,0,sizeof(ConnInfoRec)) ; 
    cInfo->gInfo = gInfo ; 

    cInfo->last_r_time  = gInfo->CurrentTime + gInfo->advanceConfig.ReceiverConnectionEstablishTimeoutMilli ;
    cInfo->sfd = sfd ; 

    place++ ; 
    if ( cip_set_local_endpoint(rInfo,cInfo) == RMM_FAILURE )
      break ; 

    place++ ; 
    if ( cip_set_remote_endpoint(rInfo,cInfo) == RMM_FAILURE )
      break ; 

    cInfo->sock_af = cInfo->lcl_sa->sa_family ;
  
    place++ ; 

    cip_update_conn_list(gInfo, cInfo, 1) ; 
    DBG_PRT(printf(" cip_build_new_incoming_conn: con added , gInfo->cipInfo.nConnsInProg=%d\n",gInfo->cipInfo.nConnsInProg)) ; 


   #if USE_POLL
    gInfo->cipInfo.fds[cInfo->ind].fd = cInfo->sfd ; 
    gInfo->cipInfo.fds[cInfo->ind].events = POLLIN ; 
   #else
    FD_SET(cInfo->sfd,&gInfo->cipInfo.rfds) ;
    if ( gInfo->cipInfo.np1 < (sfd+1) )
      gInfo->cipInfo.np1 = (sfd+1) ; 
   #endif

    rmm_ntop(cInfo->sock_af,cInfo->rmt_addr.bytes,cInfo->req_addr,RUM_ADDR_STR_LEN) ;
    cInfo->req_port = cInfo->rmt_port ; 

    cInfo->io_state = CIP_IO_STATE_SSL_HS ;
    ok++ ; 
  } while ( 0 ) ; 
  DBG_PRT(printf("cip_build_new_incoming_conn: rc= %d , place= %d\n",ok,place)) ; 
  if ( ok > 1 ) 
    return RMM_SUCCESS ; 
  else
  if ( ok > 0 ) 
    free(cInfo) ; 

  closesocket(sfd) ; 

  return RMM_FAILURE ; 
}

/*******************************/

int cip_conn_failed(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  rumConnectionEvent ev ; 
  TCHandle tcHandle = gInfo->tcHandles[1];

  if ( cInfo->conn_listener && cInfo->conn_listener->is_valid )
  {
    memset(&ev,0,sizeof(ev)) ; 
    if ( cInfo->init_here )
      ev.type = RUM_CONNECTION_ESTABLISH_FAILURE ; 
    else
      ev.type = RUM_CONNECTION_BROKE ; 

    ev.nparams = 1 ; 
    ev.event_params = (void *)gInfo->cipInfo.ev_dbg ; 
  
    if ( cInfo->init_here )
    {
      ev.connection_info.rum_instance = cInfo->apiInfo.rum_instance ; 
      ev.connection_info.remote_server_port = cInfo->req_port ; 
      strlcpy(ev.connection_info.remote_addr,cInfo->req_addr,RUM_ADDR_STR_LEN);
    }
    else
      memcpy(&ev.connection_info,&cInfo->apiInfo,sizeof(rumConnection)) ;

    ev.connection_info.connection_state = RUM_CONNECTION_STATE_CLOSED ; 
    cInfo->conn_listener->on_event(&ev,cInfo->conn_listener->user) ; 
  }
  if ( cInfo->init_here )
  {
    b2h(cInfo->apiInfo.remote_addr,(unsigned char *)&cInfo->connection_id,sizeof(cInfo->connection_id)) ; 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5402),"%d%s%d%s%s",
      "The RUM instance {0} was unable to establish a connection to {1}|{2}. Additional information: cid={3}, ev_msg={4}",
      gInfo->instance,cInfo->req_addr,cInfo->req_port,cInfo->apiInfo.remote_addr,gInfo->cipInfo.ev_msg);
  }
  return cip_remove_conn(gInfo,cInfo) ; 
}

/*******************************/

int cip_process_r_cfp_req(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  int have_opts , af , n , rc ; 
  uint8_t  cval ; 
  uint16_t sval ; 
  uint32_t ival ; 
  char *bptr ; 
  rumConnectionEvent ev ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ;
  ConnListenerInfo *cl ;
  TCHandle tcHandle = rInfo->tcHandle;

  DBG_PRT(printf(" entering cip_process_r_cfp_req...\n")) ;    

  memset(&ev,0,sizeof(ev)) ; 
  ev.connection_info.rum_instance  = 17 ; /* gInfo->instance ; */
  bptr = cInfo->rdInfo.buffer + 4 ; 

  NgetShort(bptr,sval) ;
  ev.connection_info.remote_server_port = sval ; 
  cInfo->req_port = sval ; 

  NgetShort(bptr,sval) ;
  ev.connection_info.remote_connection_port = sval ; 

  ev.connection_info.local_server_port = gInfo->basicConfig.ServerSocketPort ;
  ev.connection_info.local_connection_port = cInfo->lcl_port ; 
  rmm_ntop(cInfo->sock_af,cInfo->lcl_addr.bytes,ev.connection_info.local_addr,RUM_ADDR_STR_LEN) ; 


  NgetChar(bptr,cval) ; 
  if ( cval != RUM_CFP_REQ )
  {
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3455),"%d%d",
      "The CFP_REQ packet received by the RUM connection handler does not have a not valid type ({0} {1}).",
       cval,RUM_CFP_REQ);
    return RMM_FAILURE ; 
  }

  NgetChar(bptr,cval) ; 
  have_opts = cval ; 

  bptr += 2 ; 

  memcpy(&cInfo->connection_id,bptr,sizeof(rumConnectionID_t)) ;
  ev.connection_info.connection_id = cInfo->connection_id ; 
  bptr += sizeof(rumConnectionID_t) ; 

  NgetInt(bptr,ival) ; 
  NgetInt(bptr,ival) ; 
  cInfo->hb_to = (ival&0x7fffffff) ; 
  cInfo->one_way_hb = ((ival&0x80000000)!=0) ; 

  NgetShort(bptr,sval) ; 
  bptr += 2 ; 
  af = (sval == NLA_AFI) ? AF_INET : AF_INET6 ; 
  rmm_ntop(af,bptr,ev.connection_info.remote_addr,sizeof(ev.connection_info.remote_addr)) ; 
  if ( af == AF_INET )
    bptr += 4 ; 
  else
    bptr += 16; 

  if ( have_opts )
  {
    char *opt_end ; 
    uint8_t opt_type ; 
    uint16_t option_len ; 
    pgmOptHeader php[1] ; 

    memcpy(php,bptr,sizeof(pgmOptHeader)) ;
    opt_type = (php->option_type&~PGM_OPT_END) ; 
    if ( opt_type != PGM_OPT_LENGTH )
    { 
      llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3456),"%d%d",
        "The CFP_REQ packet received by the RUM connection handler does not have a valid option type ({0} {1}).",
         cval,PGM_OPT_LENGTH);
      return RMM_FAILURE ; 
    }
    opt_end = bptr + ntohs(php->option_other) ;
    option_len = php->option_len ; 
    while ( option_len > 0 && (bptr += option_len) < opt_end && !(php->option_type&PGM_OPT_END) )
    {
      memcpy(php,bptr,sizeof(pgmOptHeader)) ;
      opt_type = (php->option_type&~PGM_OPT_END) ; 
      option_len = ntohs(php->option_other) ; 
      if ( opt_type == RUM_OPT_CONNECTION_MSG )
      {
        ev.msg_len = option_len-4 ; 
        ev.connect_msg  = bptr+4 ; 
        break ; 
      }
    }
  }

  ev.type = RUM_NEW_CONNECTION ; 
  DBG_PRT(printf(" trying to call %d connListeners!\n",gInfo->nConnectionListeners)) ; 
  rc = RMM_FAILURE ; 
  pthread_mutex_lock(&gInfo->ConnectionListenersMutex) ; 
  for ( n=0 ; n<gInfo->nConnectionListeners ; n++ )
  {
    cl = gInfo->ConnectionListeners[n] ; 
    if ( cl->on_event(&ev,cl->user) == RUM_ON_CONNECTION_ACCEPT )
    {
      DBG_PRT(printf(" connAccepted! n= %d\n",n)) ; 
      cInfo->conn_listener = cl ; 
      cInfo->conn_listener->n_cip++ ; 
      memcpy(&cInfo->apiInfo,&ev.connection_info,sizeof(rumConnection)) ;
      rc = RMM_SUCCESS ; 
      break ; 
    }
    else
    {
      DBG_PRT(printf(" connRejected! n= %d\n",n)) ; 
    }
  }
  pthread_mutex_unlock(&gInfo->ConnectionListenersMutex) ; 

  return rc ; 
}

/*******************************/

int cip_process_r_cfp_rep(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  uint16_t sval ; 
  uint8_t  cval ; 
  char *bptr , cid_str0[20] , cid_str1[20] ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
  TCHandle tcHandle = rInfo->tcHandle;

  bptr = cInfo->rdInfo.buffer + 4 ; 

  NgetShort(bptr,sval) ;
  cInfo->apiInfo.remote_server_port = sval ; 
  NgetShort(bptr,sval) ;

  NgetChar(bptr,cval) ; 
  if ( cval != RUM_CFP_REP )
  { 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3457),"%d%d",
      "The CFP_REP packet received by the RUM connection handler does not have a valid type ({0} {1}).",
       cval,RUM_CFP_REP);
    return RMM_FAILURE ; 
  }

  NgetChar(bptr,cval) ; 

  bptr += 2 ; 

  if ( memcmp(&cInfo->connection_id,bptr,sizeof(ConnIdRec)) )
  { 
    b2h(cid_str0,(unsigned char *)&cInfo->connection_id,sizeof(rumConnectionID_t)) ;
    b2h(cid_str1,(unsigned char *)bptr                 ,sizeof(rumConnectionID_t)) ;
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3458),"%s%s",
      "The CFP_REP packet received by the RUM connection handler does not have a connection_id ({0} {1}).",
       cid_str0,cid_str1);
    return RMM_FAILURE ; 
  }

  return RMM_SUCCESS ; 
}

/*******************************/

int cip_process_r_cfp_ack(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  uint8_t  cval ;
  char *bptr , cid_str0[20] , cid_str1[20] ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
  TCHandle tcHandle = rInfo->tcHandle;

  bptr = cInfo->rdInfo.buffer + 4 ; 

  bptr += 4 ; /* skip ports */

  NgetChar(bptr,cval) ; 
  if ( cval != RUM_CFP_ACK )
  { 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3459),"%d%d",
      "The CFP_ACK packet received by the RUM connection handler does not have a valid type ({0} {1}).",
       cval,RUM_CFP_ACK);
    return RMM_FAILURE ; 
  }

  NgetChar(bptr,cval) ; 

  bptr += 2 ; 

  if ( memcmp(&cInfo->connection_id,bptr,sizeof(ConnIdRec)) )
  { 
    b2h(cid_str0,(unsigned char *)&cInfo->connection_id,sizeof(rumConnectionID_t)) ;
    b2h(cid_str1,(unsigned char *)bptr                 ,sizeof(rumConnectionID_t)) ;
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3460),"%s%s",
      "The CFP_ACK packet received by the RUM connection handler does not have a connection_id ({0} {1}).",
       cid_str0,cid_str1);
    return RMM_FAILURE ; 
  }

  return RMM_SUCCESS ; 
}

/******************************/

int cip_prepare_ccp_header(rumInstanceRec *gInfo, ConnInfoRec *cInfo, char *buffer, char type)
{
  int n ; 
  uint32_t ival ; 
  uint16_t m=0 ; 
  char *bptr ; 

  bptr = buffer + 4 ; 
  NputShort(bptr,(uint16_t)gInfo->basicConfig.ServerSocketPort) ;
  NputShort(bptr,(uint16_t)cInfo->lcl_port) ;
  NputChar(bptr,type) ; 
  NputChar(bptr,0) ; 
  NputShort(bptr,m) ;

  memcpy(bptr,&cInfo->connection_id,sizeof(ConnIdRec)) ; bptr += sizeof(ConnIdRec) ; 
  NputInt(bptr,1) ; 
  ival = (cInfo->one_way_hb) ? (cInfo->hb_to|0x80000000) : cInfo->hb_to ; 
  NputInt(bptr,ival) ; 
  m = ( cInfo->lcl_sa->sa_family == AF_INET ) ? NLA_AFI : NLA_AFI6 ; 
  NputShort(bptr,m) ; 
  memset(bptr,0,2) ; bptr += 2 ; 
  memcpy(bptr,cInfo->lcl_addr.bytes,cInfo->lcl_addr.length) ; bptr += cInfo->lcl_addr.length ; 
  n = (int)(bptr - buffer) - 4 ; 
  bptr = buffer ; 
  NputInt(bptr,n) ; 

  return n+4 ; 
}

/*******************************/

int cip_make_conn_ready(rumInstanceRec *gInfo, ConnInfoRec *cInfo)
{
  int i  ; 
  rumConnectionEvent ev ; 
  rumConnectionID_t cid ; 
  rmmReceiverRec *rInfo = (rmmReceiverRec *)gInfo->rInfo ; 
  TCHandle tcHandle = rInfo->tcHandle;

  DBG_PRT(printf("entering cip_make_conn_ready...\n")) ; 

  if ( cInfo->msg_len && cInfo->msg_buf )
    free(cInfo->msg_buf) ; 
  cInfo->msg_buf = NULL ; 
  cInfo->msg_len = 0 ; 
  if ( cInfo->rdInfo.buffer && cInfo->rdInfo.need_free )
    free(cInfo->rdInfo.buffer) ; 
  if ( cInfo->wrInfo.buffer && cInfo->wrInfo.need_free )
    free(cInfo->wrInfo.buffer) ; 
  cInfo->rdInfo.buflen = cInfo->rdInfo.reqlen = cInfo->rdInfo.offset = cInfo->rdInfo.need_free = 0 ; 
  cInfo->rdInfo.bptr = cInfo->rdInfo.buffer = NULL ; 
  cInfo->rdInfo.reqlen = INT_SIZE ; 
  cInfo->wrInfo.buflen = cInfo->wrInfo.reqlen = cInfo->wrInfo.offset = cInfo->wrInfo.need_free = 0 ; 
  cInfo->wrInfo.bptr = cInfo->wrInfo.buffer = NULL ; 

  pthread_mutex_lock(&gInfo->ConnectionListenersMutex) ; 
  if ( !cInfo->conn_listener->is_valid )
  {
    DBG_PRT(printf(" cip_make_conn_ready: conn_listener->is_valid==0 !! con not added!!\n")) ; 
    pthread_mutex_unlock(&gInfo->ConnectionListenersMutex) ; 
    cip_conn_failed(gInfo,cInfo);
    return RMM_FAILURE ; 
  }
  cInfo->conn_listener->n_cip-- ; 
  cInfo->conn_listener->n_active++ ; 
  pthread_mutex_unlock(&gInfo->ConnectionListenersMutex) ; 

  if ( cInfo->hb_interval <= 0 )
    cInfo->hb_interval = (cInfo->hb_to<200) ? 20 : cInfo->hb_to/10 ; 

  pthread_mutex_init(&cInfo->mutex,NULL) ;

  cInfo->ccp_header_len = cip_prepare_ccp_header(gInfo,cInfo,(char *)cInfo->ccp_header,RUM_CON_HB) ; 
                          cip_prepare_ccp_header(gInfo,cInfo,(char *)cInfo->tx_str_rep_packet,RUM_TX_STR_REP) ; 
                          cip_prepare_ccp_header(gInfo,cInfo,(char *)cInfo->rx_str_rep_tx_packet,RUM_RX_STR_REP) ; 
  DBG_PRT(printf("winthin  cip_make_conn_ready <%d>...\n",cInfo->ccp_header_len)) ; 
   
  if ( (cInfo->sendNacksQ = BB_alloc(gInfo->advanceConfig.nackBuffsQsize,0) ) == NULL )
  {
    DBG_PRT(printf(" cip_make_conn_ready: failed due to failure to allocate BufferBox (sendNacksQ). This should not happand!\n")) ;
    cip_conn_failed(gInfo,cInfo);
    return RMM_FAILURE ;
  }

  cid = cInfo->connection_id ;
  cInfo->apiInfo.connection_id     = cInfo->connection_id ;
  cInfo->apiInfo.rum_instance      = gInfo->instance ; 
  cInfo->apiInfo.connection_state  = RUM_CONNECTION_STATE_ESTABLISHED ; 
  cInfo->apiInfo.local_server_port = gInfo->basicConfig.ServerSocketPort ;
  cInfo->apiInfo.local_connection_port = cInfo->lcl_port ; 
  rmm_ntop(cInfo->sock_af,cInfo->lcl_addr.bytes,cInfo->apiInfo.local_addr,RUM_ADDR_STR_LEN) ; 
  cInfo->apiInfo.remote_connection_port = cInfo->rmt_port ; 
  rmm_ntop(cInfo->sock_af,cInfo->rmt_addr.bytes,cInfo->apiInfo.remote_addr,RUM_ADDR_STR_LEN) ;

  cInfo->last_t_time = gInfo->CurrentTime;
  cInfo->last_r_time = gInfo->CurrentTime;

  cip_update_conn_list(gInfo, cInfo, 0) ; 
  DBG_PRT(printf(" cip_make_conn_ready: con removed , gInfo->cipInfo.nConnsInProg=%d\n",gInfo->cipInfo.nConnsInProg)) ; 

 #if !USE_POLL
  {
    ConnInfoRec *cI;
    SOCKET mfd=0 ; 
    FD_CLR(cInfo->sfd,&gInfo->cipInfo.wfds) ;
    FD_CLR(cInfo->sfd,&gInfo->cipInfo.rfds) ;
    for ( mfd=0, cI=gInfo->cipInfo.firstConnInProg ; cI ; cI=cI->next )
    {
      if ( mfd < (cI->sfd+1) )
           mfd = (cI->sfd+1) ;
    }
    gInfo->cipInfo.np1 = mfd ; 
  }
 #endif
   
  b2h(cInfo->req_addr,(unsigned char *)&cInfo->connection_id,sizeof(cInfo->connection_id)) ; 
  i = 2*sizeof(cInfo->connection_id) ; 
  snprintf(cInfo->req_addr+i,sizeof(cInfo->req_addr)-i," %s %s|%d",(cInfo->init_here)?"->":"<-",cInfo->apiInfo.remote_addr,cInfo->apiInfo.remote_server_port) ; 

  cInfo->state = RUM_CONNECTION_STATE_IN_PROGRESS ; 

  DBG_PRT(printf("winthin  cip_make_conn_ready calling add_ready_connection...\n")) ; 
  add_ready_connection(gInfo,cInfo) ; 
  DBG_PRT(printf("winthin  cip_make_conn_ready after   add_ready_connection...\n")) ; 

  memset(&ev,0,sizeof(ev)) ; 
  memcpy(&ev.connection_info,&cInfo->apiInfo,sizeof(rumConnection)) ;
  if ( cInfo->init_here )
    ev.type = RUM_CONNECTION_ESTABLISH_SUCCESS ; 
  else
    ev.type = RUM_CONNECTION_READY ; 
  DBG_PRT(printf("winthin  cip_make_conn_ready calling on_event...\n")) ; 
  cInfo->conn_listener->on_event(&ev,cInfo->conn_listener->user) ; 
  DBG_PRT(printf("winthin  cip_make_conn_ready after   on_event...\n")) ; 

  rmm_rwlock_rdlock(&gInfo->ConnSyncRW) ; 
  if ( cInfo == rum_find_connection(gInfo,cid,0,0) ) 
  {
    cInfo->state = RUM_CONNECTION_STATE_ESTABLISHED ; 
    send_FO_signal(gInfo,1) ; 
    send_PR_signal(gInfo,1) ; 
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5405),"%s%d%d%d%d%d",
    "The RUM instance has established a connection to {0}|{1} with the following parameters: init_here={2}, hb_to={3}, hb_interval={4}, local port={5}.",
    cInfo->req_addr,cInfo->rmt_port, cInfo->init_here, cInfo->hb_to, cInfo->hb_interval, cInfo->lcl_port);
  }
  else
  {
    char cid_str[20] ; 
    b2h(cid_str,(uint8_t *)&cid,sizeof(cid)) ;
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5406),"%s",
    "The ready connection ({0}) has already been closed.",cid_str);
  }
  rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ; 
  

  return RMM_SUCCESS ; 
}
