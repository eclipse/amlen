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
/* Module Name: storeHighAvailability.c                              */
/*                                                                   */
/* Description: Store high-availability Component of ISM             */
/*                                                                   */
/*********************************************************************/
#define TRACE_COMP Store
#define USE_IB 0
#define USE_SSL 1
#define DEBUG_LEVEL 9
#define POLL_TO  16
//================================
#include  "storeHAinternal.h"
#include  "storeUtils.h"
#include  <ismutil.h>
#include  <adminHA.h>
#include  <store_certificates.h>
//===========================================================================================================

// External data
extern ismStore_global_t ismStore_global ; 
extern ismStore_memGlobal_t ismStore_memGlobal ; 
extern int ism_transport_getNumActiveConns(void);
double viewTime;


// Internal data
static haGlobalInfo gInfo_[1] ; 
static const char *StartUpName[2] = {"AutoDetect","StandAlone"} ; 
//================================

// Internal function declarations

static char *ip2str(ipFlat *ip, char *str, int len);
static const char *RT_SCOPE(int scope);
static int buildIfList(errInfo *ei);
static int buildIpList(int allowNoAddrs, double retryTO);;

#if USE_IB
static int conn_read_ib(ConnInfoRec *cInfo);
#endif
#if USE_SSL
static int conn_read_ssl(ConnInfoRec *cInfo);
#endif
static int conn_read_tcp(ConnInfoRec *cInfo);

static int conn_write(ConnInfoRec *cInfo);
static int doSelect(ConnInfoRec *cInfo);
static int extractPacket(ConnInfoRec *cInfo, char **buff, uint32_t *bufflen);
static int getLocalIf(const char *try_this, int use_ib, int req_mc, ifInfo *ifi, int nono_ind);
static int getMyRole(void);
static int getViewCount(void);
static int viewBreak(haGlobalInfo *gInfo);
static int breakView_(haGlobalInfo *gInfo,int line);
#define breakView(x) breakView_(x,__LINE__);
static int haGetAddr(int af, int *dns, const char *src, ipFlat *ip);
static int haGetNameInfo(SA *sa, char *addr, size_t a_len, char *port, size_t p_len, errInfo *ei);
static int ha_set_nb(int fd, errInfo *ei, const char *myName);
static void *ism_store_haThread(void *arg, void * context, int value);
static void build_server_id(void);
static int ha_raise_event(ConnInfoRec *cInfo, int event_type);
static int update_conn_list(ConnInfoRec **seed, ConnInfoRec *cInfo, int *nConns, int add);
static int update_chnn_list(haGlobalInfo *gInfo, ChannInfo *ch, int add);
static int cip_update_conn_list(haGlobalInfo *gInfo, ConnInfoRec *cInfo, int add);
static int cip_set_socket_buffer_size(int sfd, int buffer_size, int opt);
static int cip_set_low_delay(int sfd, int nodelay, int lwm);
static int cip_set_local_endpoint(ConnInfoRec *cInfo) ;
static int cip_set_remote_endpoint(ConnInfoRec *cInfo) ;
static int cip_create_accept_conns(haGlobalInfo *gInfo);
static int cip_check_req_msgs(haGlobalInfo *gInfo, int *role);
static int cip_check_res_msgs(haGlobalInfo *gInfo, int *role);
static int cip_prepare_req_msg(haGlobalInfo *gInfo);
static int cip_prepare_res_msg(haGlobalInfo *gInfo);
static int cip_prepare_ack_msg(haGlobalInfo *gInfo);
static int cip_prepare_s_cfp_cid(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int cip_prepare_s_cfp_hbt(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int cip_prepare_s_cfp_req(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int cip_prepare_s_cfp_res(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int cip_prepare_s_cfp_ack(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int cip_prepare_s_cfp_trm(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int cip_prepare_s_ack_trm(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int cip_conn_failed_(haGlobalInfo *gInfo, ConnInfoRec *cInfo, int ec, int ln);
#define cip_conn_failed(x,y,z) cip_conn_failed_(x,y,z,__LINE__)
static int free_conn(ConnInfoRec *cInfo);
static int cip_restart_discovery_(haGlobalInfo *gInfo,int line);
#define cip_restart_discovery(x) cip_restart_discovery_(x,__LINE__)
static int cip_raise_view_(haGlobalInfo *gInfo, int type, int line);
#define cip_raise_view(x,y) cip_raise_view_(x,y,__LINE__)
static ConnInfoRec *cip_prepare_conn_req(haGlobalInfo *gInfo, int is_ha, ChannInfo *channel);
static int cip_handle_conn_est(haGlobalInfo *gInfo, ConnInfoRec *cInfo) ;
static int cip_set_buffers(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int cip_handle_conn_req(haGlobalInfo *gInfo, ConnInfoRec *cInfo) ;
static int cip_build_new_incoming_conn(haGlobalInfo *gInfo, ConnInfoRec *acInfo);
static int cip_handle_async_connect(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int cip_conn_ready(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static void ha_set_pfds(ConnInfoRec *cInfo);
static void cip_remove_conns(haGlobalInfo *gInfo, int all, int type);
static void wait4channs(haGlobalInfo *gInfo, double wt, int chUp);
static void build_nfds(haGlobalInfo *gInfo);
static int cip_ssl_handshake(haGlobalInfo *gInfo, ConnInfoRec *cInfo);

#if USE_IB
static const char *rdma_ev_desc(int type);
static int clearSendQ(ibInfo *si, errInfo *ei);
static int loadIbLib(errInfo *ei);
static int unloadIbLib(void);
static int buildIfIbInfo(ifInfo *cur_ifi, int req_af, int port_space, errInfo *ei);
static void freeIfIbInfo(ifInfo *ifi);
static int  cip_prepare_ib_stuff(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static void  cip_destroy_ib_stuff(ConnInfoRec *cInfo);
static int  cip_handle_conn_req_ev(haGlobalInfo *gInfo, ConnInfoRec *acInfo);
static int  cip_handle_addr_res_ev(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int  cip_handle_rout_res_ev(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int  cip_handle_conn_est_ev(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
static int  cip_check_disconn_ev(haGlobalInfo *gInfo, ConnInfoRec *cInfo);
#endif
static int cip_create_discovery_conns(haGlobalInfo *gInfo) ; 
static int cip_handle_discovery_conns(haGlobalInfo *gInfo) ; 

static char *b2h(char *dst,unsigned char *src,int len);

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
    int          flags, i=1;
    const char * data;
    const char * fn;
    char         mbuf[1024];
    char *       pos;
    int          err = errno;

    if (SHOULD_TRACE(4)) {
        const char *func = cInfo->sslInfo->func ? cInfo->sslInfo->func : "Unknown" ; 
        if (rc) {
            const char * errStr = (rc < 9) ? SSL_ERRORS[rc] : "SSL_UNKNOWN_ERROR";
            if (err) {
                data = strerror_r(err, mbuf, 1024);
                traceFunction(3, TOPT_WHERE, file, line, "openssl func=%s, conn= |%s|, error(%d): %s : errno is \"%s\"(%d)\n",
                        func,cInfo->req_addr, rc, errStr, data, err);
            } else {
                traceFunction(3, TOPT_WHERE, file, line, "openssl func=%s, conn= |%s|, error(%d): %s\n",
                        func,cInfo->req_addr, rc, errStr);
            }
            i=0;
        }
        for (;;) {
            rc = (uint32_t)ERR_get_error_all(&file, &line, &fn, &data, &flags);
            if (rc == 0)
                break;
            ERR_error_string_n(rc, mbuf, sizeof mbuf);
            pos = strchr(mbuf, ':');
            if (!pos)
                pos = mbuf;
            else
                pos++;
            if (flags&ERR_TXT_STRING) {
                traceFunction(3, TOPT_WHERE, file, line, "openssl func=%s, conn= |%s|, error(%d): %s: data=\"%s\"\n",
                        func,cInfo->req_addr, rc, pos, data);
            } else {
                traceFunction(3, TOPT_WHERE, file, line, "openssl func=%s, conn= |%s|, error(%d): %s\n",
                        func,cInfo->req_addr, rc, pos);
            }
            i=0;
        }
        if (i)
        {
          traceFunction(3, TOPT_WHERE, file, line, "openssl conn= |%s| no errors!!!\n",cInfo->req_addr);
        }
    }
}
#endif

/*-------------------------------------------------*/
/*-------------------------------------------------*/
/*-------------------------------------------------*/

static inline int checkChannel(void *hChannel)
{
  ChannInfo *ch ;
  ConnInfoRec *cInfo ;

  ch = (ChannInfo *)hChannel ; 
  if ( !ch )
  {
    TRACE(1,"ism_storeHA_API called with a NULL hChannel\n") ; 
    return StoreRC_SystemError ; 
  }

  cInfo = ch->cInfo ; 
  if ( !cInfo )
  {
    TRACE(1,"ism_storeHA_API called without a connection\n") ; 
    return StoreRC_SystemError ; 
  }
  if ( cInfo->is_invalid )
  {
    if (!ch->closing && !ch->broken )
    {
      TRACE(1,"ism_storeHA_API called with a broken connection\n") ; 
      ch->broken = 1 ; 
    }
    return StoreRC_HA_ConnectionBroke ; 
  }
  return StoreRC_OK ; 
}
#if USE_IB
#if 0
static void cip_query_qp(struct ibv_qp *qp)
{
  struct ibv_qp_attr a[1];
  struct ibv_qp_init_attr i[1];
  int rc;
  if ( (rc=ibv_query_qp(qp,a,-1,i)) )
  {
    TRACE(1,"ibv_quey_qp failed, rc= %d (%s)\n",rc,strerror(rc));
    return;
  }
  TRACE(1,"QP ATTR: qp_state=%d, cur_qp_state=%d, path_mtu=%d, path_mig_state=%d, qkey=%u, rq_psn=%u, sq_psn=%u, dest_qp_num=%u, qp_access_flags=%d,"
          "max_send_wr=%u, max_recv_wr=%u, max_send_sge=%u, max_recv_sge=%u, max_inline_data=%u, pkey_index=%hu, alt_pkey_index=%hu,en_sqd_async_notify=%x,"
          "sq_draining=%x, max_rd_atomic=%x, max_dest_rd_atomic=%x, min_rnr_timer=%x, port_num=%x, timeout=%x,retry_cnt=%x, rnr_retry=%x, alt_port_num=%x,"
          "alt_timeout=%x, qp_context=%p, send_cq=%p, recv_cq=%p, srq=%p, qp_type=%d, sq_sig_all=%d, xrc_domain=%p\n",
           a->qp_state, a->cur_qp_state, a->path_mtu, a->path_mig_state, a->qkey, a->rq_psn, a->sq_psn, a->dest_qp_num, a->qp_access_flags, 
           a->cap.max_send_wr, a->cap.max_recv_wr, a->cap.max_send_sge, a->cap.max_recv_sge, a->cap.max_inline_data, a->pkey_index, a->alt_pkey_index, 
           a->en_sqd_async_notify, a->sq_draining, a->max_rd_atomic, a->max_dest_rd_atomic, a->min_rnr_timer, a->port_num, a->timeout, 
           a->retry_cnt, a->rnr_retry, a->alt_port_num, a->alt_timeout, i->qp_context, i->send_cq, i->recv_cq, i->srq, i->qp_type, i->sq_sig_all, i->xrc_domain);
}
#endif
#endif
//================================
//  API functions
//================================

/*********************************************************************/
/* Callbcak to get HA configuration changes from Admin               */
/*                                                                   */
/* HA user settable configuration items are not runtime settable     */
/* items. ISM server restart is necessary for these items to be      */
/* effective.                                                        */
/* This callack always return ISMRC_OK. It is added to satisfy       */
/* configuration service.                                            */
/*********************************************************************/
int ism_store_HAConfigCallback(char *pObject, char *pName, ism_prop_t *pProps, ism_ConfigChangeType_t flag)
{
  haGlobalInfo *gInfo=gInfo_;
  TRACE(9, "HA Configuration callback is invoked.\n");
  if (flag == ISM_CONFIG_CHANGE_PROPS)
  {
    int i ; 
    const char *name ; 

    for ( i=0 ; ism_common_getPropertyIndex(pProps, i, &name) == 0 ; i++ )
    {
      TRACE(9, "HA Configuration callback: process %s\n", name);
      if (!strcmp(name,"HighAvailability.Group.haconfig"))
      {
        char *Group = su_strdup(ism_common_getStringProperty(pProps, name)) ; 
        if ( Group )
        {
          if ( su_strllen(Group, GROUP_MAX_LEN+1) > GROUP_MAX_LEN )
          {
            TRACE(1," The value of config param %s (%s) is not valid\n",ismHA_CFG_GROUP,Group) ; 
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ismHA_CFG_GROUP,Group) ;
            return ISMRC_BadPropertyValue;
          }
          pthread_mutex_lock(gInfo->haLock) ; 
          gInfo->config->Group_ = Group ; 
          pthread_mutex_unlock(gInfo->haLock) ; 
          TRACE(5,"%s - HA Group has been dynamically changed to %s (old val: %s).\n",__FUNCTION__,Group,gInfo->config->Group);
        }
        break ; 
      }
    }
  }
   return ISMRC_OK;
}

int ism_store_initHAConfig(void)
{
   int i, rc = ISMRC_OK;
   ism_prop_t *pProps, *pCfgProps;
   ism_config_t *hConfig = NULL;
   ism_field_t f;
   char haPropName[256];
   const char *pName, *p1, *p2;

   TRACE(9, "Entry: %s\n", __FUNCTION__);
   if ((rc = ism_config_register(ISM_CONFIG_COMP_HA, NULL, ism_store_HAConfigCallback, &hConfig)) != ISMRC_OK)
   {
      return rc;
   }

   if ((pProps = ism_config_getPropertiesDynamic(hConfig, NULL, NULL)) != NULL)
   {
      snprintf(haPropName, 256, "HA.");
      pCfgProps = ism_common_getConfigProperties();
      for (i = 0; ism_common_getPropertyIndex(pProps, i, &pName) == 0; i++)
      {
         if (ism_common_getProperty(pProps, pName, &f))
         {
            continue;
         }
         else
         {
            for (p1=pName; *p1!=0 && *p1!='.'; p1++); // empty body
            if (*p1 != '.') continue;
            p2 = ++p1;
            for (; *p1!=0 && *p1!='.'; p1++ ); // empty body
            if ( *p1 != '.' ) continue;
            memcpy(haPropName+3, p2, (p1-p2));
            haPropName[3+(p1-p2)] = 0;
         }
         ism_common_setProperty(pCfgProps, haPropName, &f);
      }
   }

   if (NULL != pProps) ism_common_freeProperties(pProps);

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}
/*-------------------------------------------------*/
int ism_storeHA_init(ismStore_HAParameters_t *pHAParameters)
{
  const char *tstr ; 
  int rc = StoreRC_OK ; 
  haGlobalInfo *gInfo=gInfo_;

  memset(gInfo,0,sizeof(haGlobalInfo)) ; 
  pthread_mutex_init(gInfo->haLock,NULL) ;
  if (!(pHAParameters &&
        pHAParameters->ViewChanged &&
        pHAParameters->ChannelCreated) )
    return StoreRC_SystemError ; 
  memcpy(gInfo->params, pHAParameters, sizeof(ismStore_HAParameters_t)) ; 
  gInfo->config->EnableHA                 = ism_common_getIntConfig(ismHA_CFG_ENABLEHA, 0) ; 
  gInfo->config->Group                    = ism_common_getStringConfig(ismHA_CFG_GROUP) ; 
  tstr                                    = ism_common_getStringConfig(ismHA_CFG_STARTUPMODE) ; 
  gInfo->config->PreferredPrimary         = ism_common_getIntConfig(ismHA_CFG_PREFERREDPRIMARY,0) ; 
  gInfo->config->NodesAddr                = ism_common_getStringConfig(ismHA_CFG_REMOTEDISCOVERYNIC) ; 
  gInfo->config->ReplicationNIC           = ism_common_getStringConfig(ismHA_CFG_LOCALREPLICATIONNIC) ; 
  gInfo->config->ExtReplicationNIC             = ism_common_getStringConfig(ismHA_CFG_EXTERNALREPLICATIONNIC) ; 
  gInfo->config->DiscoveryNIC             = ism_common_getStringConfig(ismHA_CFG_LOCALDISCOVERYNIC) ; 
  gInfo->config->DiscoveryTimeout         = ism_common_getIntConfig(ismHA_CFG_DISCOVERYTIMEOUT,600) ; 
  gInfo->config->HeartbeatTimeout         = ism_common_getIntConfig(ismHA_CFG_HEARTBEATTIMEOUT,10) ; 
                                         
  gInfo->config->AckingPolicy             = ism_common_getIntConfig(ismHA_CFG_ACKINGPOLICY,1) ; 
  gInfo->config->ServerName               = ism_common_getStringConfig("Name") ; 
  if (!su_strllen(gInfo->config->ServerName,1) )
    gInfo->config->ServerName             = "NoName" ; 
  gInfo->config->Port                     = ism_common_getIntConfig(ismHA_CFG_PORT,9084) ; 
  gInfo->config->ExtPort                  = ism_common_getIntConfig(ismHA_CFG_REMOTEDISCOVERYPORT,0) ; 
  gInfo->config->haPort                   = ism_common_getIntConfig(ismHA_CFG_RPLICATIONPORT,0) ; 
  gInfo->config->ExtHaPort                = ism_common_getIntConfig(ismHA_CFG_EXTERNALRPLICATIONPORT,0) ; 
  gInfo->config->ReplicationProtocol      = ism_common_getIntConfig(ismHA_CFG_REPLICATIONPROTOCOL,1) ; 
  gInfo->config->RecoveryMemStandbyMB     = ism_common_getIntConfig(ismHA_CFG_RECOVERYMEMSTANDBYMB,0);
  gInfo->config->LockedMemSizeMB          = ism_common_getIntConfig(ismHA_CFG_LOCKEDMEMSIZEMB,16) ; 
  gInfo->config->SocketBufferSize         = ism_common_getIntConfig(ismHA_CFG_SOCKETBUFFERSIZE,(1<<22)) ; 
  gInfo->config->SyncMemSizeMB            = ism_common_getIntConfig(ismHA_CFG_SYNCMEMSIZEMB,0);
  gInfo->config->AllowSingleNIC           = ism_common_getIntConfig(ismHA_CFG_ALLOWSINGLENIC,0) ; 
  gInfo->config->UseForkInit              = ism_common_getIntConfig(ismHA_CFG_USEFORKINIT,1) ; 
  gInfo->config->RoleValidation           = ism_common_getIntConfig(ismHA_CFG_ROLEVALIDATION,1) ;
  gInfo->config->FlowControl              = ism_common_getIntConfig(ismHA_CFG_FLOWCONTROL,1) ;
  gInfo->config->DisableAutoResync        = ism_common_getIntConfig(ismHA_CFG_DISABLEAUTORESYNC,0) ; 
  gInfo->config->UseSecuredConnections    = ism_common_getIntConfig(ismHA_CFG_USE_SECUREDCONN,0) ; 
  gInfo->config->SplitBrainPolicy         = ism_common_getIntConfig(ismHA_CFG_SPLITBRAINPOLICY,0) ;
  gInfo->config->RequireCertificates      = ism_common_getIntConfig(ismHA_CFG_REQUIRE_CERTIFICATES,1) ;

  if ( !tstr || !*tstr ) tstr = StartUpName[CIP_STARTUP_AUTO_DETECT] ;
  if ( !strcasecmp(tstr, StartUpName[CIP_STARTUP_AUTO_DETECT]) )
    gInfo->config->StartupMode = CIP_STARTUP_AUTO_DETECT ; 
  else
  if ( !strcasecmp(tstr, StartUpName[CIP_STARTUP_STAND_ALONE]) )
    gInfo->config->StartupMode = CIP_STARTUP_STAND_ALONE ; 
  else
    gInfo->config->StartupMode = CIP_STARTUP_AUTO_DETECT - 1 ; 


  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_ENABLEHA,gInfo->config->EnableHA) ; 
  TRACE(5," The value of config param %s is %s\n",ismHA_CFG_STARTUPMODE,tstr) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_PREFERREDPRIMARY,gInfo->config->PreferredPrimary) ; 
//TRACE(5," The value of config param %s is %d\n",ismHA_CFG_AUTOCONFIG,gInfo->config->AutoConfig) ;
  TRACE(5," The value of config param %s is %s\n",ismHA_CFG_GROUP,gInfo->config->Group) ;
  TRACE(5," The value of config param %s is %s\n",ismHA_CFG_REMOTEDISCOVERYNIC,gInfo->config->NodesAddr) ; 
  TRACE(5," The value of config param %s is %s\n",ismHA_CFG_LOCALREPLICATIONNIC,gInfo->config->ReplicationNIC) ; 
  TRACE(5," The value of config param %s is %s\n",ismHA_CFG_EXTERNALREPLICATIONNIC,gInfo->config->ExtReplicationNIC) ; 
  TRACE(5," The value of config param %s is %s\n",ismHA_CFG_LOCALDISCOVERYNIC,gInfo->config->DiscoveryNIC) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_DISCOVERYTIMEOUT,gInfo->config->DiscoveryTimeout) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_HEARTBEATTIMEOUT,gInfo->config->HeartbeatTimeout) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_ACKINGPOLICY,gInfo->config->AckingPolicy) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_PORT,gInfo->config->Port) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_REMOTEDISCOVERYPORT,gInfo->config->ExtPort) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_RPLICATIONPORT,gInfo->config->haPort) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_EXTERNALRPLICATIONPORT,gInfo->config->ExtHaPort) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_REPLICATIONPROTOCOL, gInfo->config->ReplicationProtocol) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_RECOVERYMEMSTANDBYMB, gInfo->config->RecoveryMemStandbyMB) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_LOCKEDMEMSIZEMB, gInfo->config->LockedMemSizeMB) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_SOCKETBUFFERSIZE, gInfo->config->SocketBufferSize) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_SYNCMEMSIZEMB, gInfo->config->SyncMemSizeMB) ;
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_ALLOWSINGLENIC, gInfo->config->AllowSingleNIC) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_USEFORKINIT, gInfo->config->UseForkInit) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_ROLEVALIDATION, gInfo->config->RoleValidation) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_FLOWCONTROL, gInfo->config->FlowControl) ;
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_DISABLEAUTORESYNC, gInfo->config->DisableAutoResync) ; 
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_USE_SECUREDCONN,gInfo->config->UseSecuredConnections) ;
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_SPLITBRAINPOLICY,gInfo->config->SplitBrainPolicy) ;
  TRACE(5," The value of config param %s is %d\n",ismHA_CFG_REQUIRE_CERTIFICATES,gInfo->config->RequireCertificates) ;
  

  if ( gInfo->config->EnableHA != 0 && gInfo->config->EnableHA != 1 ) 
  {
    TRACE(1," The value of config param %s (%d) is not valid\n",ismHA_CFG_ENABLEHA,gInfo->config->EnableHA) ; 
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%u", ismHA_CFG_ENABLEHA,gInfo->config->EnableHA) ;
  }
#if 0
  if ( su_strlen(gInfo->config->NodesAddr) + su_strlen(gInfo->config->ReplicationNIC) + su_strlen(gInfo->config->DiscoveryNIC) == 0 )
  {
    gInfo->config->AutoConfig = 1 ;
    TRACE(5," AutoConfig is turned on since the HA network has not been configured explicitly.\n");
    gInfo->config->ExtReplicationNIC = NULL ; 
    gInfo->config->ExtPort = 0 ; 
    gInfo->config->haPort = 0 ; 
    gInfo->config->ExtHaPort = 0 ; 
  }
  else
    gInfo->config->AutoConfig          = 0 ; // ism_common_getIntConfig(ismHA_CFG_AUTOCONFIG, 0) ; 
#endif
  gInfo->config->AutoConfig = 0 ; // As of v2.0 AutoConfig is no longer supported 
  if ( gInfo->config->AckingPolicy != 0 && gInfo->config->AckingPolicy != 1 )
  {
    TRACE(1," The value of config param %s (%d) is not valid\n",ismHA_CFG_ACKINGPOLICY,gInfo->config->AckingPolicy) ; 
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%u", ismHA_CFG_ACKINGPOLICY,gInfo->config->AckingPolicy) ;
  }

  if ( gInfo->config->StartupMode != CIP_STARTUP_AUTO_DETECT &&
       gInfo->config->StartupMode != CIP_STARTUP_STAND_ALONE )
  {
    TRACE(1," The value of config param %s (%s) is not valid\n",ismHA_CFG_STARTUPMODE,tstr);
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ismHA_CFG_STARTUPMODE,tstr);
  }

  if ( gInfo->config->AutoConfig )
  {
    const char *nic ; 
    nic = "ha0" ; 
    gInfo->config->DiscoveryNIC = ism_common_ifmap(nic, IFMAP_FROM_SYSTEM) ; 
    if ( !gInfo->config->DiscoveryNIC || gInfo->config->DiscoveryNIC == nic )
    {
      TRACE(1," The internal NIC mapping service: ism_common_ifmap(%s, IFMAP_FROM_SYSTEM) failed.\n",nic);
      return StoreRC_SystemError ; 
    }
    nic = "ha1" ; 
    gInfo->config->ReplicationNIC = ism_common_ifmap(nic, IFMAP_FROM_SYSTEM) ; 
    if ( !gInfo->config->ReplicationNIC || gInfo->config->ReplicationNIC == nic )
    {
      TRACE(1," The internal NIC mapping service: ism_common_ifmap(%s, IFMAP_FROM_SYSTEM) failed.\n",nic);
      return StoreRC_SystemError ; 
    }
    TRACE(5," The value of detected param %s is %s\n",ismHA_CFG_LOCALREPLICATIONNIC,gInfo->config->ReplicationNIC) ; 
    TRACE(5," The value of detected param %s is %s\n",ismHA_CFG_LOCALDISCOVERYNIC,gInfo->config->DiscoveryNIC) ; 
    if ( gInfo->config->Group && su_strllen(gInfo->config->Group, GROUP_MAX_LEN+1) > GROUP_MAX_LEN )
    {
      TRACE(1," The value of config param %s (%s) is not valid\n",ismHA_CFG_GROUP,gInfo->config->Group) ; 
      rc = StoreRC_BadParameter ;
      ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ismHA_CFG_GROUP,gInfo->config->Group) ;
    }
  }
  else
  {
    if ( gInfo->config->NodesAddr && gInfo->config->NodesAddr[0] )
    {
//    ipFlat tip[1] ; 
//    if ( haGetAddr(AF_UNSPEC, 0, gInfo->config->NodesAddr, tip) == AF_UNSPEC )
//    {
//      TRACE(1," The value of config param %s (%s) is not valid\n",ismHA_CFG_REMOTEDISCOVERYNIC, gInfo->config->NodesAddr) ; 
//      rc = StoreRC_BadParameter ;
//    }
    }
    else
    {
      TRACE(1," The config param %s must be specified\n",ismHA_CFG_REMOTEDISCOVERYNIC);
      rc = StoreRC_BadParameter ;
      ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ismHA_CFG_REMOTEDISCOVERYNIC,"(nil)");
    }
  
    if ( gInfo->config->ReplicationNIC && gInfo->config->ReplicationNIC[0] )
    {
//      ipFlat tip[1] ; 
//      if ( haGetAddr(AF_UNSPEC, 0, gInfo->config->ReplicationNIC, tip) == AF_UNSPEC )
//      {
//        TRACE(1," The value of config param %s (%s) is not valid\n",ismHA_CFG_LOCALREPLICATIONNIC, gInfo->config->ReplicationNIC) ; 
//        rc = StoreRC_BadParameter ;
//        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ismHA_CFG_LOCALREPLICATIONNIC, gInfo->config->ReplicationNIC) ;
//      }
    }
    else
    {
      TRACE(1," The config param %s must be specified\n",ismHA_CFG_LOCALREPLICATIONNIC) ; 
      rc = StoreRC_BadParameter ;
      ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ismHA_CFG_LOCALREPLICATIONNIC,"(nil)");
    }
  
    if ( gInfo->config->ExtReplicationNIC && gInfo->config->ExtReplicationNIC[0] )
    {
      int dns=1 ; 
      ipFlat tip[1] ; 
      if ( haGetAddr(AF_UNSPEC, &dns, gInfo->config->ExtReplicationNIC, tip) == AF_UNSPEC )
      {
        TRACE(1," The value of config param %s (%s) is not valid\n",ismHA_CFG_EXTERNALREPLICATIONNIC,gInfo->config->ExtReplicationNIC);
        rc = StoreRC_BadParameter ;
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ismHA_CFG_EXTERNALREPLICATIONNIC,gInfo->config->ExtReplicationNIC);
      }
    }
  
    if ( gInfo->config->DiscoveryNIC && gInfo->config->DiscoveryNIC[0] )
    {
//      ipFlat tip[1] ; 
//      if ( haGetAddr(AF_UNSPEC, 0, gInfo->config->DiscoveryNIC, tip) == AF_UNSPEC )
//      {
//        TRACE(1," The value of config param %s (%s) is not valid\n",ismHA_CFG_LOCALDISCOVERYNIC, gInfo->config->DiscoveryNIC) ; 
//        rc = StoreRC_BadParameter ;
//        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ismHA_CFG_LOCALDISCOVERYNIC, gInfo->config->DiscoveryNIC) ;
//      }
    }
    else
    {
      TRACE(1," The config param %s must be specified\n",ismHA_CFG_LOCALDISCOVERYNIC) ; 
      rc = StoreRC_BadParameter ;
      ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ismHA_CFG_LOCALDISCOVERYNIC,"(nil)");
    }
  }


  if ( gInfo->config->Port < 1024 || gInfo->config->Port > 65535 ) 
  {
    TRACE(1," The value of config param %s (%d) is not valid\n",ismHA_CFG_PORT,gInfo->config->Port);
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%u", ismHA_CFG_PORT,gInfo->config->Port);
  }
  if ( gInfo->config->ExtPort && (gInfo->config->ExtPort < 1024 || gInfo->config->ExtPort > 65535) ) 
  {
    TRACE(1," The value of config param %s (%d) is not valid\n",ismHA_CFG_REMOTEDISCOVERYPORT,gInfo->config->ExtPort);
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%u", ismHA_CFG_REMOTEDISCOVERYPORT,gInfo->config->ExtPort);
  }
  if ( gInfo->config->haPort && (gInfo->config->haPort < 1024 || gInfo->config->haPort > 65535) ) 
  {
    TRACE(1," The value of config param %s (%d) is not valid\n",ismHA_CFG_RPLICATIONPORT,gInfo->config->haPort);
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%u", ismHA_CFG_RPLICATIONPORT,gInfo->config->haPort);
  }
  if ( gInfo->config->ExtHaPort && (gInfo->config->ExtHaPort < 1024 || gInfo->config->ExtHaPort > 65535) ) 
  {
    TRACE(1," The value of config param %s (%d) is not valid\n",ismHA_CFG_EXTERNALRPLICATIONPORT,gInfo->config->ExtHaPort);
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%u", ismHA_CFG_EXTERNALRPLICATIONPORT,gInfo->config->ExtHaPort);
  }
  if ( gInfo->config->ExtHaPort && !gInfo->config->haPort )
  {
    TRACE(1," The value of config param %s (%d) is not valid: it must be specified when %s is specified\n",ismHA_CFG_RPLICATIONPORT,gInfo->config->haPort,ismHA_CFG_EXTERNALRPLICATIONPORT);
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%u", ismHA_CFG_RPLICATIONPORT,gInfo->config->haPort);
  }
  if ( gInfo->config->ReplicationProtocol != 0 && gInfo->config->ReplicationProtocol != 1 ) 
  {
    TRACE(1," The value of config param %s (%d) is not valid\n",ismHA_CFG_REPLICATIONPROTOCOL,gInfo->config->ReplicationProtocol) ; 
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%u", ismHA_CFG_REPLICATIONPROTOCOL,gInfo->config->ReplicationProtocol) ;
  }
  if ( gInfo->config->ReplicationProtocol == 0 && gInfo->config->UseSecuredConnections )
  {
    TRACE(1," The value of config param %s (%d) is not valid, it conficts with the value of %s (%d)\n",
          ismHA_CFG_USE_SECUREDCONN, gInfo->config->UseSecuredConnections, ismHA_CFG_REPLICATIONPROTOCOL,gInfo->config->ReplicationProtocol) ;
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%u", ismHA_CFG_USE_SECUREDCONN, gInfo->config->UseSecuredConnections) ;
  }
  if ( gInfo->config->SplitBrainPolicy != 0 && gInfo->config->SplitBrainPolicy != 1 ) 
  {
    TRACE(1," The value of config param %s (%d) is not valid\n",ismHA_CFG_SPLITBRAINPOLICY,gInfo->config->SplitBrainPolicy) ;
    rc = StoreRC_BadParameter ;
    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%u", ismHA_CFG_SPLITBRAINPOLICY,gInfo->config->SplitBrainPolicy) ;
  }
  if ( rc != StoreRC_OK )
    return rc ; 

  if (!gInfo->config->ExtPort ) gInfo->config->ExtPort = gInfo->config->Port ; 

  gInfo->use_ib = (gInfo->config->ReplicationProtocol == 0) ; 
  if ( gInfo->use_ib )
 #if USE_IB
  {
    errInfo ei[1];
    ei->errLen = sizeof(ei->errMsg) ; 
    if ( (rc=loadIbLib(ei)) != StoreRC_OK )
    {
      TRACE(1," loadIbLib failed with errMsg: %s\n",ei->errMsg) ; 
    }
    else
    if ( gInfo->config->UseForkInit && (rc = Xibv_fork_init()) )
    {
      TRACE(1," ibv_fork_init failed, rc=%d (%s)\n",rc,strerror(rc));
    }
  }
 #else
  {
    TRACE(1,"Support for RDMA RC ReplicationProtocol was not compiled in for this version\n") ; 
    rc = StoreRC_BadParameter ;
  }
 #endif

  gInfo->use_ssl = gInfo->config->UseSecuredConnections ; 
  gInfo->require_certificates = gInfo->config->RequireCertificates ;
  if ( gInfo->use_ssl )
 #if USE_SSL
  {
    int sslUseBufferPool;
    gInfo->sslInfo->FIPSmode = ism_config_getFIPSConfig();
    sslUseBufferPool = ism_config_getSslUseBufferPool();
    ism_ssl_init(gInfo->sslInfo->FIPSmode, sslUseBufferPool);
  }
 #else
  {
    TRACE(1,"Support for secure HA communication was not compiled in for this version\n") ; 
    rc = StoreRC_BadParameter ;
  }
 #endif
  if ( rc != StoreRC_OK )
    return rc ; 

  if ( (tstr = su_strdup(gInfo->config->ServerName)) ) gInfo->config->ServerName = tstr ; 
  if ( (tstr = su_strdup(gInfo->config->NodesAddr )) ) gInfo->config->NodesAddr  = tstr ; 
  if ( (tstr = su_strdup(gInfo->config->ReplicationNIC)) ) gInfo->config->ReplicationNIC = tstr ; 
  if ( (tstr = su_strdup(gInfo->config->ExtReplicationNIC)) ) gInfo->config->ExtReplicationNIC = tstr ; 
  if ( (tstr = su_strdup(gInfo->config->DiscoveryNIC)) ) gInfo->config->DiscoveryNIC = tstr ; 
  if ( (tstr = su_strdup(gInfo->config->Group)) ) gInfo->config->Group = tstr ; 
  gInfo->lastView->OldRole = gInfo->lastView->NewRole = gInfo->myRole = ISM_HA_ROLE_UNSYNC ; 
  gInfo->haTimeOut[0] = 1e1 ; 
  gInfo->haTimeOut[1] = (double)gInfo->config->DiscoveryTimeout ; 
  gInfo->hbTimeOut = (double)gInfo->config->HeartbeatTimeout ; 
  gInfo->hbInterval= gInfo->hbTimeOut / 2e1 ; 
  gInfo->hbTOloops = gInfo->config->HeartbeatTimeout * 1000 / POLL_TO / 8 ; 
  gInfo->recvNpoll = gInfo->use_ib ? 10000000 : (gInfo->use_ssl ? 1 : 1000) ; 
  gInfo->SockBuffSizes[0][0] = gInfo->config->SocketBufferSize ;  //1 << 16 ; 
  gInfo->SockBuffSizes[0][1] = gInfo->config->SocketBufferSize ;  //1 << 16 ;
  gInfo->SockBuffSizes[1][0] = gInfo->config->LockedMemSizeMB << 20 ;
  gInfo->SockBuffSizes[1][1] = gInfo->config->LockedMemSizeMB << 20 ;
  if ( ismStore_memGlobal.fEnablePersist )
  {
    gInfo->SockBuffSizes[2][0] = ismStore_memGlobal.PersistBuffSize ;
    gInfo->SockBuffSizes[2][1] = ismStore_memGlobal.PersistBuffSize ;
  }
  else
  {
    gInfo->SockBuffSizes[2][0] = gInfo->config->SocketBufferSize ;  //1 << 16 ; 
    gInfo->SockBuffSizes[2][1] = gInfo->config->SocketBufferSize ;  //1 << 16 ;
  }
  gInfo->mcInfo->grp_len = su_strlen(gInfo->config->Group) ; 
  gInfo->config->gUpd[0]++ ; 

  return rc ; 
}
/*-------------------------------------------------*/
int ism_storeHA_start(void)
{
  int rc ; 
  double et;
  errInfo ei[1];
  ei->errLen = sizeof(ei->errMsg) ; 
  haGlobalInfo *gInfo=gInfo_;

  if ( buildIfList(ei) < 0 )
  {
    TRACE(1,"ism_storeHA_start: failed to detect local NICs, error: %s\n",ei->errMsg);
    return StoreRC_SystemError ; 
  }
  if ( (rc = getLocalIf(gInfo->config->ReplicationNIC, gInfo->use_ib, gInfo->config->AutoConfig, gInfo->haIf,0)) < 0 )
  {
    const char *nic = gInfo->config->AutoConfig ? "ha1" : gInfo->config->ReplicationNIC ; 
    TRACE(1,"ism_storeHA_start: failed to attach to the given %s (%s) using the given %s (%d)\n",ismHA_CFG_LOCALREPLICATIONNIC,nic,ismHA_CFG_REPLICATIONPROTOCOL,gInfo->config->ReplicationProtocol);
    //ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s%s", ismHA_CFG_LOCALREPLICATIONNIC, gInfo->config->ReplicationNIC,"Address provided for the HA LocalReplicationNIC is not valid or cannot be used");
    if ( gInfo->config->AutoConfig )
    {
      ism_common_setErrorData(ISMRC_StoreHABadNicConfig, "%s", nic);
      return StoreRC_BadNIC; 
    }
    else
    {
      if ( rc == -2 )
      {
        char str[1024] ; 
        snprintf(str, 1024, "%s (resolved to the LoopBack interface)", gInfo->config->ReplicationNIC);
        ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s", ismHA_CFG_LOCALREPLICATIONNIC, str);
      }
      else
        ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s", ismHA_CFG_LOCALREPLICATIONNIC, gInfo->config->ReplicationNIC);
      return StoreRC_BadParameter; 
    }
  }
  if ( (rc = getLocalIf(gInfo->config->DiscoveryNIC, 0, 0, gInfo->hbIf, 0)) < 0 )
  {
    const char *nic = gInfo->config->AutoConfig ? "ha0" : gInfo->config->DiscoveryNIC ; 
    TRACE(1,"ism_storeHA_start: failed to attach to the given %s (%s) \n",ismHA_CFG_LOCALDISCOVERYNIC,nic) ; 
    //ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s%s", ismHA_CFG_LOCALDISCOVERYNIC, gInfo->config->DiscoveryNIC,"Address provided for HA LocalDiscoveryNIC is not valid or cannot be used");
    if ( gInfo->config->AutoConfig )
    {
      ism_common_setErrorData(ISMRC_StoreHABadNicConfig, "%s", nic);
      return StoreRC_BadNIC; 
    }
    else
    {
      if ( rc == -2 )
      {
        char str[1024] ; 
        snprintf(str, 1024, "%s (resolved to the LoopBack interface)", gInfo->config->DiscoveryNIC);
        ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s", ismHA_CFG_LOCALDISCOVERYNIC, str);
      }
      else
        ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s", ismHA_CFG_LOCALDISCOVERYNIC, gInfo->config->DiscoveryNIC);
      return StoreRC_BadParameter; 
    }
  }
  if ( gInfo->haIf->index == gInfo->hbIf->index && !gInfo->config->AllowSingleNIC )
  {
    TRACE(1,"ism_storeHA_start: HA configuration is not valid. %s (%s) and %s (%s) are configured to use the same NIC (index %d)\n",ismHA_CFG_LOCALDISCOVERYNIC,gInfo->config->DiscoveryNIC,ismHA_CFG_LOCALREPLICATIONNIC,gInfo->config->ReplicationNIC,gInfo->haIf->index) ; 
    //ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s%s", ismHA_CFG_LOCALDISCOVERYNIC, gInfo->config->DiscoveryNIC,"HA LocalDiscoveryNIC is configured to use the same NIC as HA LocalReplicationNIC (must use different NICs)");
    ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s", ismHA_CFG_LOCALDISCOVERYNIC, gInfo->config->DiscoveryNIC);
    return StoreRC_BadParameter; 
  }
  su_strlcpy(gInfo->lastView->LocalReplicationNIC, gInfo->haIf->addr_name, ADDR_STR_LEN) ; 
  su_strlcpy(gInfo->lastView->LocalDiscoveryNIC  , gInfo->hbIf->addr_name, ADDR_STR_LEN) ; 

  if (!gInfo->config->AutoConfig && (rc = buildIpList(0, gInfo->haTimeOut[1]/2e0)) != StoreRC_OK )
  {
    TRACE(1,"ism_storeHA_start: failed to translate the given %s (%s) into an internal form\n",ismHA_CFG_REMOTEDISCOVERYNIC,gInfo->config->NodesAddr);
    return rc ; 
  }
  build_server_id() ; 

 #if USE_SSL
  if ( gInfo->use_ssl )
  {
    int i;

    FILE * f_cert;
    FILE * f_key;
    int require_cert = gInfo->require_certificates ? 1 : 2;
    char * cert;
    char * key;
    cert = ha_certdata;
    key = ha_keydata;

    char const * dir = ism_common_getStringConfig("KeyStore");

    if ( dir ) {
        char * cert_file = alloca(12 + strlen(dir) + 1);
        strcpy(cert_file,dir);
        strcat(cert_file, "/HA_cert.pem");
        f_cert = fopen(cert_file, "rb");
        char * key_file = alloca(11 + strlen(dir) + 1);
        strcpy(key_file,dir);
        strcat(key_file, "/HA_key.pem");
        f_key = fopen(key_file, "rb");

        if (f_cert && f_key) {

            cert = "HA_cert.pem";
            key = "HA_key.pem";
            TRACE(5, "Using user provided credentials for High Availability Pairing\n");
            fclose(f_key);
            fclose(f_cert);
            require_cert = 1;
        }
    }

    for ( i=0 ; i<2 ; i++ )
    {
      char *nm ; 
      nm = i ? "HA_Outgoing" : "HA_Incoming" ; 
      // specify HA_Incoming as the serverName to allow both connections to use a common CAFile and CAPath
      gInfo->sslInfo->sslCtx[i] = ism_common_create_ssl_ctx(nm, "TLSv1.2", "ECDHE-ECDSA-AES128-GCM-SHA256", cert, key, 1-i, 0x010003FF, require_cert , nm, NULL, "HA");
      if (!gInfo->sslInfo->sslCtx[i] )
      {
        rc = ISMRC_CreateSSLContext ; 
        ism_common_setError(rc);
        TRACE(1, "The SSL/TLS context could not be created for %s\n", nm);
        return rc ; 
      }
    }
  }
 #endif

  gInfo->cipInfo->chUP =-1 ; 
  if ( ism_common_startThread(&gInfo->haId, ism_store_haThread, gInfo, NULL, 0, ISM_TUSAGE_NORMAL,0, "haControl", "Handle HA communication with partner node") )
  {
    TRACE(1,"ism_storeHA_start: ism_common_startThread failed.\n");
    return StoreRC_SystemError ; 
  }
  et = su_sysTime() + 2e1 ; 
  do
  {
    pthread_mutex_lock(gInfo->haLock) ; 
    rc = gInfo->cipInfo->chUP ;
    pthread_mutex_unlock(gInfo->haLock) ; 
    if ( rc != -1 )
      break ; 
    su_sleep(8,SU_MIL) ; 
  } while ( et > su_sysTime() ) ; 
  if ( rc != 1 )
  {
    TRACE(1,"ism_storeHA_start: ism_store_haThread failed to start.\n");
    if ( gInfo->failedNIC )
    {
      if ( gInfo->config->AutoConfig )
      {
        ism_common_setErrorData(ISMRC_StoreHABadNicConfig, "%s", gInfo->failedNIC);
        return StoreRC_BadNIC; 
      }
      else
      {
        if ( memcmp(gInfo->failedNIC,"ha0",4) )
          ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s", ismHA_CFG_LOCALREPLICATIONNIC, gInfo->config->ReplicationNIC);
        else
          ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s", ismHA_CFG_LOCALDISCOVERYNIC, gInfo->config->DiscoveryNIC);
        return StoreRC_BadParameter; 
      }
    }
    return StoreRC_SystemError ; 
  }

  return StoreRC_OK ; 
}
/*-------------------------------------------------*/
int ism_storeHA_term(void)
{
  haGlobalInfo *gInfo=gInfo_;
  gInfo->goDown = 1 ; 
  breakView(gInfo) ; 
  wait4channs(gInfo, 1e1, 0) ; 
 #if USE_IB
  {
    unloadIbLib() ; 
  }
 #endif
  // TODO
  return StoreRC_OK ; 
}
/*-------------------------------------------------*/
const ismStore_HAConfig_t *ism_storeHA_getConfig(void)
{
  haGlobalInfo *gInfo=gInfo_;
  return gInfo->config ;
}
/*-------------------------------------------------*/
/*********************************************************************/
/* Return a new HASessionId                                          */
/*                                                                   */
/*                                                                   */
/* @param pHASessionId  Pointer to a buffer of 10 bytes where the    */
/*                      new HASessionId will be copied (output)      */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                         */
/*********************************************************************/
int ism_storeHA_allocateSessionId(unsigned char *pHASessionId)
{
  haGlobalInfo *gInfo=gInfo_;
  if ( pHASessionId )
  {
    time_t t;
    unsigned char *p = pHASessionId ;
    memcpy(p, gInfo->hbIf->ni->haddr->bytes,6) ; p += 6 ; 
    t = time(NULL);
    memcpy(p, &t,4) ;
    return StoreRC_OK ; 
  }
  return StoreRC_SystemError ;
}
/*-------------------------------------------------*/
int ism_storeHA_createChannel(int32_t channelId, uint8_t flags, void **phChannel)
{
  int rc ; 
  uint8_t fHighPerf ;
  ChannInfo *ch ; 
  ConnInfoRec *cInfo ; 
  eventInfo   *ev=NULL ; 
  haGlobalInfo *gInfo=gInfo_;

  if (!(ch = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,152),sizeof(ChannInfo))) ) 
  {
    TRACE(1,"ism_storeHA_createChannel: malloc failed for %lu bytes\n",sizeof(ChannInfo)) ; 
    breakView(gInfo) ; 
    return StoreRC_SystemError ; 
  }
  memset(ch, 0, sizeof(ChannInfo)) ; 
  pthread_mutex_init(ch->lock,NULL) ; 
  pthread_cond_init (ch->cond,NULL) ; 
  ch->channel_id = channelId ; 
  ch->flags = flags ; 
  ch->use_lock = (flags&2) ; 

  if ( viewBreak(gInfo) || !(gInfo->dInfo->state & DSC_HAVE_PAIR) )
  {
    ism_common_free(ism_memory_store_misc,ch) ; 
    return StoreRC_HA_ConnectionBroke ; 
  }

  fHighPerf = flags&1 ; 
  rc = StoreRC_OK ; 
  if ( !(cInfo = cip_prepare_conn_req(gInfo, 1, ch)) )
  {
    rc = StoreRC_SystemError ; 
    breakView(gInfo) ; 
  }
  else
  {
    update_chnn_list(gInfo, ch, 1) ; 
    pthread_mutex_lock(gInfo->haLock) ; 
    for(;;)
    {
      if ( (ev = ch->events) )
      {
        ch->events = ev->next ; 
        break ; 
      }
      if ( gInfo->viewBreak )
        break ; 
      pthread_cond_wait(ch->cond, gInfo->haLock) ; 
    }
    pthread_mutex_unlock(gInfo->haLock) ; 
    DBG_PRINT("_dbg_%d: %p %d\n",__LINE__,ev,(ev?ev->event_type:-1)) ; 
    if ( ev )
    {
      int type = ev->event_type ; 
      ism_common_free(ism_memory_store_misc,ev) ; 
      if ( type == CH_EV_CON_READY )
      {
        *phChannel = ch ;
        if ( fHighPerf )
          ch->cInfo->nPoll = gInfo->recvNpoll ; 
        if ( ch->cInfo->nPoll < 1 ) 
             ch->cInfo->nPoll = 1 ; 
        rc = StoreRC_OK ; 
      }
      else
      if ( type == CH_EV_CON_BROKE )
      {
        rc = StoreRC_HA_ConnectionBroke ; 
        breakView(gInfo) ; 
      }
      else
      if ( type == CH_EV_SYS_ERROR || type == CH_EV_SSL_ERROR )
      {
        rc = StoreRC_SystemError ; 
        breakView(gInfo) ; 
      }
      else
      if ( type == CH_EV_NEW_VIEW  )
      {
        if ( rc == StoreRC_OK )
          rc = StoreRC_HA_ViewChanged ; 
      }
      else
      {
        // CH_EV_TERMINATE
        rc = StoreRC_SystemError ; 
      }
    }
    else
      rc = StoreRC_HA_ConnectionBroke ; 
  }
  if ( rc != StoreRC_OK )
  {
    update_chnn_list(gInfo, ch, 0) ; 
    pthread_mutex_lock(gInfo->haLock) ; 
    while (ch->events)
    {
      ev = ch->events ; 
      ch->events = ev->next ; 
      ism_common_free(ism_memory_store_misc,ev) ; 
    }
    pthread_mutex_unlock(gInfo->haLock) ; 
    pthread_mutex_destroy(ch->lock) ;
    pthread_cond_destroy (ch->cond) ; 
    ism_common_free(ism_memory_store_misc,ch) ; 
  }
    
  return rc ; 
}
/*-------------------------------------------------*/
int ism_storeHA_closeChannel(void *hChannel, uint8_t fPending)
{
  int rc ; 
  ChannInfo *ch ;
  eventInfo   *ev ; 
  haGlobalInfo *gInfo=gInfo_;

  if ( (rc = checkChannel(hChannel)) == StoreRC_SystemError )
    return rc ; 

  ch = (ChannInfo *)hChannel ; 
  if ( fPending )
  {
    ch->closing = 1 ; 
    ch->cInfo->nPoll = 1 ; 
    return StoreRC_OK ; 
  }

  update_chnn_list(gInfo, ch, 0);
  free_conn(ch->cInfo) ; 
  pthread_mutex_lock(gInfo->haLock) ; 
  while (ch->events)
  {
    ev = ch->events ; 
    ch->events = ev->next ; 
    ism_common_free(ism_memory_store_misc,ev) ; 
  }
  pthread_mutex_unlock(gInfo->haLock) ; 
  pthread_mutex_destroy(ch->lock) ;
  pthread_cond_destroy (ch->cond) ; 
  ism_common_free(ism_memory_store_misc,ch) ; 

  return StoreRC_OK ;
}
/*-------------------------------------------------*/
int ism_storeHA_getChannelTxQDepth(void *hChannel, uint32_t *pTxQDepth)
{
   if ( !hChannel || !pTxQDepth )
   {
      return StoreRC_SystemError;
   }
#if USE_IB
   int rc ;
   ChannInfo *ch ;
   ConnInfoRec *cInfo ;
   ioInfo *si ;

   if ( (rc = checkChannel(hChannel)) != StoreRC_OK )
     return rc ;

   ch = (ChannInfo *)hChannel ;
   cInfo = ch->cInfo ;
   if ( cInfo->use_ib )
   {
      si = &cInfo->wrInfo ;
      *pTxQDepth = si->reg_len * 2;
   }
   else
   {
      *pTxQDepth = 1;
   }
#else
   *pTxQDepth = 1;
#endif
   return StoreRC_OK;
}
/*-------------------------------------------------*/
int ism_storeHA_allocateBuffer(void *hChannel,char **pBuffer,uint32_t *pBufferLength)
{
  int rc ; 
  ChannInfo *ch ;
  ConnInfoRec *cInfo ;
  ioInfo *si ;

  if ( (rc = checkChannel(hChannel)) != StoreRC_OK )
    return rc ; 

  ch = (ChannInfo *)hChannel ; 
  cInfo = ch->cInfo ; 
  si = &cInfo->wrInfo ;

  if ( ch->use_lock ) pthread_mutex_lock(ch->lock) ; 

 #if USE_IB
  if ( cInfo->use_ib )
  {
    struct ibv_send_wr *wr ;

    if ( !si->wr1st )
    {
      errInfo ei[1] ; 
      ei->errLen = sizeof(ei->errMsg) ; 
      for(;;)
      {
        if ( (rc=clearSendQ((ibInfo *)&si->ibv_ctx, ei)) != StoreRC_OK )
        {
          TRACE(1,"%s\n",ei->errMsg);
        }
        else
        if ( si->wr1st )
          break ; 
        else
        if ( cInfo->is_invalid )
          rc = StoreRC_HA_ConnectionBroke ; 
        else
        {
          if ( ch->use_lock ) pthread_mutex_unlock(ch->lock) ; 
          sched_yield() ; 
          if ( ch->use_lock ) pthread_mutex_lock(ch->lock) ; 
          continue ; 
        }
        if ( ch->use_lock ) pthread_mutex_unlock(ch->lock) ; 
        if ( !ch->closing )
          breakView(gInfo) ; 
        return rc ;
      }
    }
    wr = si->wr1st ;
    si->wr1st = wr->next ;
    *pBuffer = (char *)wr->sg_list->addr ; 
    *pBufferLength = si->ib_bs ; 
  }
  else
 #endif
  {
    if ( ch->use_lock )
    {
      while ( si->inUse && !cInfo->is_invalid )  /* BEAM suppression: infinite loop */
      {
        pthread_mutex_unlock(ch->lock) ; 
        sched_yield() ; 
        pthread_mutex_lock(ch->lock) ; 
      }
      if ( cInfo->is_invalid )
      {
        pthread_mutex_unlock(ch->lock) ; 
        return StoreRC_HA_ConnectionBroke ; 
      }
      else
      {
        si->inUse++ ;
        *pBuffer = si->buffer ; 
      }
    }
    else
      *pBuffer = si->buffer ; 
    *pBufferLength = si->buflen ; 
  }

  if ( ch->use_lock ) pthread_mutex_unlock(ch->lock) ; 
  return StoreRC_OK ; 
}
/*-------------------------------------------------*/
int ism_storeHA_sendMessage(void *hChannel,char *pData,uint32_t dataLength)
{
  int rc, ret ; 
  ChannInfo *ch ;
  ConnInfoRec *cInfo ;
  ioInfo *si ;

  if ( (rc = checkChannel(hChannel)) != StoreRC_OK )
    return rc ; 

  ch = (ChannInfo *)hChannel ; 
  cInfo = ch->cInfo ; 
  si = &cInfo->wrInfo ;

  if ( ch->use_lock ) pthread_mutex_lock(ch->lock) ; 

 #if USE_IB
  if ( cInfo->use_ib )
  {
    struct ibv_send_wr *wr , *wrb ;

    wr = *((struct ibv_send_wr **)(pData + si->ib_bs)) ; 
    si->inUse++ ;
    wr->next = NULL ;
    wr->sg_list->length = dataLength ; 
    wr->send_flags = si->send_flags[(dataLength <= si->ifi->ib_max_inline)] ;
    wrb = NULL ;
    if ( (ret = ibv_post_send(si->ib_qp, wr, &wrb)) || wrb )
    {
      TRACE(1,"ism_storeHA_sendMessage:  ibv_post_send failed with error %d (%s)",ret,strerror(ret));
      rc = StoreRC_HA_ConnectionBroke ; 
    }
    else
    {
      errInfo ei[1] ; 
      ei->errLen = sizeof(ei->errMsg) ; 
      cInfo->wrInfo.nBytes += dataLength ; 
      cInfo->wrInfo.nPacks++ ; 
     #if 1
      if ( si->inUse > si->reg_len && (rc=clearSendQ((ibInfo *)&si->ibv_ctx, ei)) != StoreRC_OK )
      {
        TRACE(1,"%s\n",ei->errMsg);
      }
     #endif
    }
  }
  else
 #endif
  {
    struct pollfd pfd ; 
    pfd.fd = cInfo->sfd ; 
    pfd.events = POLLOUT ; 
    si->reqlen = dataLength ;
    si->offset = 0;
    si->bptr   = pData ;
   #if USE_SSL
    if ( cInfo->use_ssl )
    {
      pthread_mutex_lock(cInfo->sslInfo->lock) ; 
      for(;;)
      {
        ret = SSL_write(cInfo->sslInfo->ssl, si->bptr, (si->reqlen-si->offset)) ;
        if ( ret > 0 )
        {
          cInfo->wrInfo.nBytes += ret ; 
          si->offset += ret ;
          si->bptr += ret;
          if ( si->offset == si->reqlen )
          {
            cInfo->wrInfo.nPacks++ ; 
            si->inUse-- ;
            rc = StoreRC_OK ; 
            break ;
          }
          else
            pfd.events = POLLOUT ; 
        }
        else
        {
        rc  = SSL_get_error(cInfo->sslInfo->ssl, ret) ; 
      //TRACE(9,"SSL_write:\t ret= %d, rc=%s, conn=%s\n",ret,SSL_ERRORS[rc],cInfo->req_addr);
        if ( LIKELY(rc == SSL_ERROR_NONE) )
        {
          if ( ret > 0 )
          {
            cInfo->wrInfo.nBytes += ret ; 
            si->offset += ret ;
            si->bptr += ret;
            if ( si->offset == si->reqlen )
            {
              cInfo->wrInfo.nPacks++ ; 
              si->inUse-- ;
              rc = StoreRC_OK ; 
              break ;
            }
            else
              pfd.events = POLLOUT ; 
          }
        }
        else
        if ( rc == SSL_ERROR_WANT_READ )
          pfd.events = POLLIN ; 
        else
        if ( rc == SSL_ERROR_WANT_WRITE)
          pfd.events = POLLOUT; 
        else
        {
          if ( rc == SSL_ERROR_SYSCALL && ret == -1 && eWB(errno) )
            pfd.events = POLLOUT; 
          else
          {
            cInfo->sslInfo->func = "SSL_write" ;
            sslTraceErr(cInfo, rc, __FILE__, __LINE__);
            rc = StoreRC_HA_ConnectionBroke ; 
            break ; 
          }
        }
        }
        poll(&pfd, 1, 1) ; 
      }
      pthread_mutex_unlock(cInfo->sslInfo->lock) ; 
    }
    else
   #endif
    for(;;)
    {
      ret = write(cInfo->sfd, si->bptr, (si->reqlen-si->offset)) ;
      if ( ret >= 0 )
      {
        cInfo->wrInfo.nBytes += ret ; 
        si->offset += ret ;
        si->bptr += ret;
        if ( si->offset == si->reqlen )
        {
          cInfo->wrInfo.nPacks++ ; 
          si->inUse-- ;
          break ;
        }
      }
      else /* ret == 0 doesn't indicate a failure */
      {
        ret = errno ; 
        if ( cInfo->is_invalid || !eWB(ret) )
        {
          rc = StoreRC_HA_ConnectionBroke ; 
          break ; 
        }
      }
      poll(&pfd, 1, 1) ; 
    }
  }

  if ( ch->use_lock ) pthread_mutex_unlock(ch->lock) ; 
  if ( rc != StoreRC_OK && !ch->closing )
    breakView(gInfo_) ; 
  return rc ; 
}
/*-------------------------------------------------*/
int ism_storeHA_receiveMessage(void *hChannel,char **pData,uint32_t *pDataLength,uint8_t fNonBlocking)
{
  int rc ; 
  ChannInfo *ch ;
  ConnInfoRec *cInfo ;

  if ( (rc = checkChannel(hChannel)) != StoreRC_OK )
    return rc ; 

  ch = (ChannInfo *)hChannel ; 
  cInfo = ch->cInfo ; 

  for ( ; ; )
  {
    rc = extractPacket(cInfo, pData, pDataLength) ; 
    if ( rc > 0 )
    {
      cInfo->iPoll = 0 ; 
      return StoreRC_OK ; 
    }
    else
    if ( rc < 0 )
    {
      cInfo->is_invalid |= P_INVALID ; 
      TRACE(5,"HA Connection marked as invalid: %s \n",cInfo->req_addr);
      rc = StoreRC_HA_ConnectionBroke ; 
      break ; 
    }
    else
    {
      if ( cInfo->is_invalid )
      {
        rc = StoreRC_HA_ConnectionBroke ; 
        break ; 
      }
      if ( fNonBlocking )
        return StoreRC_HA_WouldBlock ; 
      if ( cInfo->iPoll < cInfo->nPoll )
      {
        cInfo->iPoll++ ; 
       #if USE_IB
        if ( cInfo->use_ib && !cInfo->rdInfo.inUse && cInfo->iPoll >= cInfo->nPoll )
        {
          ibv_req_notify_cq(cInfo->rdInfo.ib_cq,0) ;
          cInfo->rdInfo.inUse++ ; 
          cInfo->iPoll-- ; 
        }
       #endif
      }
    }
    if ( cInfo->iPoll >= cInfo->nPoll )
    {
      rc = doSelect(cInfo) ; 
      if ( rc <= 0 )
      {
        rc = StoreRC_HA_ConnectionBroke ; 
        break ; 
      }
    }
    else
      sched_yield() ; 
  }
  if ( !ch->closing )
    breakView(gInfo_) ; 
  return rc ; 
}
/*-------------------------------------------------*/
int ism_storeHA_returnBuffer(void *hChannel, char *pBuffer)
{
  int rc ; 

  if ( (rc = checkChannel(hChannel)) != StoreRC_OK )
    return rc ; 

 #if USE_IB
  ChannInfo *ch ;
  ConnInfoRec *cInfo ;
  ch = (ChannInfo *)hChannel ; 
  cInfo = ch->cInfo ; 
  if ( cInfo->use_ib )
  {
    int ret ;
    ioInfo *si ;
    struct ibv_recv_wr *wr, *wr_bad ;

    si = &cInfo->rdInfo ;
    wr = *((struct ibv_recv_wr **)(pBuffer + si->ib_bs)) ; 
    if ( (ret = ibv_post_recv(si->ib_qp, wr, &wr_bad)) )
    {
      TRACE(1," failed to post_recv! (rc=%d %d) (n=%d)\n",ret,errno,(int)wr->wr_id);
      if ( !ch->closing )
        breakView(gInfo_) ; 
      return StoreRC_HA_ConnectionBroke ; 
    }
  }
  else
 #endif
  {
    // currently no action seems to be needed here
  }

  return StoreRC_OK ; 
}

/*-------------------------------------------------*/

int ism_storeHA_sbError(void)
{
  haGlobalInfo *gInfo=gInfo_;
  if ( (gInfo->dInfo->state & DSC_HAVE_PAIR) && gInfo->lastView->NewRole == ISM_HA_ROLE_STANDBY )
  {
    gInfo->sbError = 1 ; 
    breakView(gInfo) ; 
    return StoreRC_OK ; 
  }
  return StoreRC_SystemError ; 
}                           
/*-------------------------------------------------*/

//================================
//  Internal functions
//================================


/*-------------------------------------------------*/

void wait4channs(haGlobalInfo *gInfo, double wt, int chUp)
{
  int ok=0 ; 
  double et = su_sysTime() + wt ; 
  do
  {
    pthread_mutex_lock(gInfo->haLock) ; 
    ok = (gInfo->cipInfo->chUP <= chUp && gInfo->chnHead == NULL) ; 
    pthread_mutex_unlock(gInfo->haLock) ; 
    su_sleep(8,SU_MIL);
  } while (!ok && et > su_sysTime() ) ; 
}
/*-------------------------------------------------*/


void build_server_id(void)
{
  haGlobalInfo *gInfo=gInfo_;
  uint32_t pid ; 
  char *p = (char *)gInfo->server_id ; 
  memset(gInfo->server_id, 0, sizeof(ismStore_HANodeID_t)) ; 
 #if 1
  su_strlcpy(p, gInfo->config->ServerName,7) ; p += 6 ; 
 #else
  if ( (pid=su_strlcpy(p, gInfo->config->ServerName,6)) < 6 ) ;
  {
    gethostname(p+pid,6-pid) ; 
  }
  p += 6 ; 
 #endif
  memcpy(p, gInfo->hbIf->ni->haddr->bytes,6) ; p += 6 ; 
  pid = getpid() ; 
  memcpy(p, &pid, 4) ; 
}

/*-------------------------------------------------*/

int getMyRole(void)
{
  int r ; 
  haGlobalInfo *gInfo=gInfo_;
  pthread_mutex_lock(gInfo->haLock);
  r = (int)gInfo->myRole ; 
  pthread_mutex_unlock(gInfo->haLock);
  return r ; 
}

/*-------------------------------------------------*/

int getViewCount(void)
{
  int c ; 
  haGlobalInfo *gInfo=gInfo_;
  pthread_mutex_lock(gInfo->haLock);
  c = gInfo->viewCount ; 
  pthread_mutex_unlock(gInfo->haLock);
  return c ; 
}

/*-------------------------------------------------*/

int  viewBreak(haGlobalInfo *gInfo)
{
  int c ; 
  pthread_mutex_lock(gInfo->haLock);
  c = gInfo->viewBreak ; 
  pthread_mutex_unlock(gInfo->haLock);
  return c ; 
}

/*-------------------------------------------------*/

int  breakView_(haGlobalInfo *gInfo, int line)
{
  ChannInfo *ch ; 
  pthread_mutex_lock(gInfo->haLock);
  if ( !gInfo->viewBreak )
  {
    TRACE(5,"breakView_ called from line %d\n",line);
    gInfo->viewBreak = 1 ; 
    for ( ch=gInfo->chnHead ; ch ; ch = ch->next )
    {
      if ( ch->cInfo ) ch->cInfo->is_invalid |= C_INVALID ; 
      pthread_cond_signal(ch->cond) ; 
    }
  }
  pthread_mutex_unlock(gInfo->haLock);
  return 0 ; 
}

/*-------------------------------------------------*/
void build_nfds(haGlobalInfo *gInfo)
{
  ChannInfo *ch ; 
  if (!ismStore_memGlobal.fEnablePersist ) return ; 
  pthread_mutex_lock(gInfo->haLock);
  if ( gInfo->nblu != gInfo->nchu )
  {
    int ok=1 ; 
    if ( gInfo->nchs > gInfo->sfds )
    {
      int s = (gInfo->nchs+31)&(~31) ; 
      void *ptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,159),gInfo->pfds, s*sizeof(*gInfo->pfds)) ; 
      if ( !ptr ) 
      {
        pthread_mutex_unlock(gInfo->haLock);
        return ; 
      }
      gInfo->pfds = ptr ; 
      gInfo->sfds = s ; 
    }
    gInfo->nfds = 0 ; 
    for ( ch=gInfo->chnHead ; ch ; ch = ch->next )
    {
      if ( ch->cInfo )
      {
        gInfo->pfds[gInfo->nfds].fd = ch->cInfo->sfd ; 
        gInfo->pfds[gInfo->nfds].events = POLLIN ; 
        gInfo->nfds++ ; 
      }
      else
        ok = 0 ; 
    }
    if ( ok ) gInfo->nblu = gInfo->nchu ; 
  }
  pthread_mutex_unlock(gInfo->haLock);
}
/*-------------------------------------------------*/
int ism_storeHA_pollOnAllChanns(int milli)
{
  haGlobalInfo *gInfo=gInfo_;
  build_nfds(gInfo) ;
  return poll(gInfo->pfds, gInfo->nfds, milli) ; 
}
/*-------------------------------------------------*/

const char *RT_SCOPE(int scope)
{
  static char unkn[32] ; 
  switch(scope)
  {
   case RT_SCOPE_UNIVERSE  : return "global";
   case RT_SCOPE_SITE      : return "site";
   case RT_SCOPE_LINK      : return "link";
   case RT_SCOPE_HOST      : return "host";
   case RT_SCOPE_NOWHERE   : return "none";
   default : {snprintf(unkn,32,"Unknown scope: %d",scope) ; return unkn ; }
  }
}

/*-------------------------------------------------*/
char *b2h(char *dst,unsigned char *src,int len)
{
  char H[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'} ;
  char *rc = dst ;

  while ( len-- )
  {
    *dst = H[(*src)>>4] ; dst++ ;
    *dst = H[(*src)&15] ; dst++ ;
    src++ ;
  }
  *dst = 0 ;
  return rc ;
}

/*-------------------------------------------------*/

char *ip2str(ipFlat *ip, char *str, int len)
{
  char H[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'} ;
  char s64[ADDR_STR_LEN] ; 
  if ( ip->length == sizeof(IA4) )
    inet_ntop(AF_INET, ip->bytes, s64, ADDR_STR_LEN) ; 
  else
  if ( ip->length == sizeof(IA6) )
    inet_ntop(AF_INET6,ip->bytes, s64, ADDR_STR_LEN) ; 
  else
  {
    char *p;
    uint8_t *b = (uint8_t *)ip->bytes ; 
    uint8_t *e = b + ip->length ; 
    for ( p=s64 ; b<e ; b++ )
    {
      *p++ = H[(*b)>>4] ; 
      *p++ = H[(*b)&15] ; 
      *p++ = ':' ; 
    }
    *(--p) = 0 ; 
  }
  snprintf(str,len,"%s %s %s",s64,ip->label,RT_SCOPE(ip->scope));
  return str;
}

/*-------------------------------------------------*/

static int buildIfList_if(errInfo *ei)
{
  nicInfo *ifh=NULL, *ift=NULL, *ifi ; 
  int ret=-1 ; 
  haGlobalInfo *gInfo=gInfo_;
  struct ifaddrs *ifa0, *ifa ; 
  char s96[96] ; 

  pthread_mutex_lock(gInfo->haLock) ; 
  if ( gInfo->niHead )
  {
    pthread_mutex_unlock(gInfo->haLock) ;
    return 0 ; 
  }

  if ( getifaddrs(&ifa0) )
  { 
    snprintf(ei->errMsg, ei->errLen,"system call %s() failed with error %d (%s)","getifaddrs",errno,strerror(errno)) ; 
    goto err ; 
  }

  for ( ifa=ifa0 ; ifa ; ifa=ifa->ifa_next )
  {
    if ( ifa->ifa_addr->sa_family==AF_PACKET )
    {
      struct sockaddr_ll *sal = (struct sockaddr_ll *)ifa->ifa_addr ; 
      for ( ifi=ifh ; ifi && ifi->index != sal->sll_ifindex ; ifi=ifi->next ) ; // empty body
      if ( !ifi )
      {
        if (!(ifi = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,160),sizeof(ifInfo))) )
          goto err ; 
        memset(ifi,0,sizeof(ifInfo)) ; 
        if ( ift ) ift->next = ifi ; 
        else ifh = ifi ; 
        ift = ifi ; 
        ifi->index = sal->sll_ifindex ; 
        ifi->type  = sal->sll_hatype ;
        ifi->flags = ifa->ifa_flags ; 
        su_strlcpy(ifi->name, ifa->ifa_name, IF_NAMESIZE) ; 
        ifi->haddr->length = sal->sll_halen ; 
        memcpy(ifi->haddr->bytes, sal->sll_addr, ifi->haddr->length);
      }
    }
  }
  for ( ifa=ifa0 ; ifa ; ifa=ifa->ifa_next )
  {
    ipFlat ip;
    memset(&ip,0,sizeof(ip));
    if ( ifa->ifa_addr->sa_family==AF_INET )
    {
      SA4 *sa4 = (SA4 *)ifa->ifa_addr ; 
      ip.length = sizeof(IA4) ; 
      memcpy(ip.bytes,&sa4->sin_addr,ip.length) ; 
    }
    else
    if ( ifa->ifa_addr->sa_family==AF_INET6)
    {
      SA6 *sa6 = (SA6 *)ifa->ifa_addr ; 
      ip.length = sizeof(IA6) ; 
      memcpy(ip.bytes,&sa6->sin6_addr,ip.length) ; 
    }
    if ( ip.length )
    {
      char nm[IF_NAMESIZE+8];
      strncpy(ip.label, ifa->ifa_name, IF_NAMESIZE) ; 
      strncpy(nm, ifa->ifa_name, IF_NAMESIZE) ; 
      char *p = strstr(nm,":");
      if ( p ) *p=0;
      for ( ifi=ifh ; ifi && strncmp(nm,ifi->name,IF_NAMESIZE) ; ifi=ifi->next ) ; // empty body
      if ( ifi )
      {
        ipFlat *ipi, *ipp;
        for ( ipp=NULL,ipi=ifi->addrs ; ipi ; ipp=ipi,ipi=ipi->next )
          if ( ipi->length == ip.length && !memcmp(ipi->bytes,ip.bytes,ip.length) ) break ; 
        if ( !ipi )
        {
          if ( !(ipi=ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,161),sizeof(ipFlat))) )
            goto err ; 
          memcpy(ipi,&ip,sizeof(ip)) ; 
          ifi->naddr++ ; 
          if ( ipp ) ipp->next = ipi ; 
          else ifi->addrs = ipi ; 
        }
        else
        {
          TRACE(5,"interface not found for address %s\n",ip2str(&ip,s96,96)) ; 
        }
      }
    }
  }
  freeifaddrs(ifa0);

#if 1 || CIP_DBG
  for ( ifi=ifh ; ifi ; ifi=ifi->next ) 
  {
    ipFlat *ipi ; 
    TRACE(1," name=%s, index=%d, mtu=%d, flags=%x\n",ifi->name,ifi->index,ifi->mtu,ifi->flags) ; 
    TRACE(1,"\t hwaddr: %s\n",ip2str(ifi->haddr,s96, 96));
    for ( ipi=ifi->addrs ; ipi ; ipi=ipi->next )
    {
      TRACE(1,"\t addr: %s\n",ip2str(ipi,s96,96));
  }
  }
#endif
  gInfo->niHead = ifh ; 
  ret = 0 ; 
 err:
  if ( ret )
  {
    ipFlat *ipi ; 
    while ( ifh ) 
    {
      ifi = ifh ; 
      ifh = ifi->next ; 
      while ( ifi->addrs )
      {
        ipi = ifi->addrs ;
        ifi->addrs = ipi->next ; 
        ism_common_free(ism_memory_store_misc,ipi) ; 
      }
      ism_common_free(ism_memory_store_misc,ifi) ; 
    }
  }
  pthread_mutex_unlock(gInfo->haLock) ;
  return ret ;
}

/*-------------------------------------------------*/

static int buildIfList_nl(errInfo *ei)
{
  nicInfo *ifh=NULL, *ift=NULL, *ifi ; 
  int fd=0, rc, i, seq=0, ret=-1 ; 
  ssize_t len0, len1, blen=0;
  struct sockaddr_nl nl;
  haGlobalInfo *gInfo=gInfo_;

  struct 
  {
    struct nlmsghdr nh;
    union
    {
      struct ifinfomsg fi;
      struct ifaddrmsg fa;
    } u;
  } req ; 

  struct nlmsghdr *nh;
  struct ifinfomsg *fi;
  struct ifaddrmsg *fa;
  struct rtattr *rta;
  char s96[96] ; 
  char *buff=NULL;

  pthread_mutex_lock(gInfo->haLock) ; 
  if ( gInfo->niHead )
  {
    pthread_mutex_unlock(gInfo->haLock) ;
    return 0 ; 
  }

  fd = socket_(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
  if ( fd == -1) 
  {
    snprintf(ei->errMsg, ei->errLen,"system call %s() failed with error %d (%s)","socket",errno,strerror(errno)) ; 
    goto err ; 
  }

  memset(&nl, 0, sizeof(nl));
  nl.nl_family = AF_NETLINK;
  nl.nl_pid = 0 ; //getpid() ; 
  rc = bind(fd, (SA *)&nl, sizeof(nl));
  if (rc == -1) 
  {
    snprintf(ei->errMsg, ei->errLen,"system call %s() failed with error %d (%s)","bind",errno,strerror(errno)) ; 
    goto err ; 
  }
  nl.nl_pid = 0 ; // dest

  memset(&req, 0, sizeof(req));
  req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.nh.nlmsg_type = RTM_GETLINK;
  req.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
  req.nh.nlmsg_pid = 0 ; //getpid();
  for ( i=0 ; i<2 ; i++ )
  {
    int multi=0;
    req.nh.nlmsg_seq = seq++ ; 
    req.u.fi.ifi_family = (i&1) ? AF_INET6 : AF_INET ;   
    len0 = req.nh.nlmsg_len ; //sizeof(req) ; 
    //print_nlmsg(&req.nh,__LINE__);
    len1 = sendto(fd, &req, len0, 0, (SA *)&nl, sizeof(nl));
    if ( len1 != len0 )
    {
      snprintf(ei->errMsg, ei->errLen,"system call %s() failed with error %d (%s)","send",errno,strerror(errno)) ; 
      goto err ; 
    }
    do
    {
      len1 = recv(fd,s96,1, MSG_PEEK|MSG_TRUNC) ; 
      if ( len1 < 0 )
      {
        snprintf(ei->errMsg, ei->errLen,"system call %s() failed with error %d (%s)","recv",errno,strerror(errno)) ; 
        goto err ; 
      }
      if ( len1 > blen )
      {
        char *tmp ; 
        len0 = (len1+0xfff)&(~0xfff) ; 
        if (!(tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,164),buff,len0)) )
        {
          snprintf(ei->errMsg, ei->errLen,"system call %s() failed with error %d (%s), req size: %lu","realloc",errno,strerror(errno),len0) ; 
          goto err ; 
        }
        buff = tmp ; 
        blen = len0 ; 
      }
      len1 = recv(fd,buff,blen,0) ; 
      DBG_PRINT("_dbg_%d: recv len1=%lu, i=%d\n",__LINE__,len1,i);
      if ( len1 < 0 )
      {
        snprintf(ei->errMsg, ei->errLen,"system call %s() failed with error %d (%s)","recv",errno,strerror(errno)) ; 
        goto err ; 
      }
      nh = (struct nlmsghdr *)buff ; 
      if (nh->nlmsg_type == NLMSG_ERROR) 
      {
        struct nlmsgerr *nlme;
        nlme = NLMSG_DATA(nh);
        snprintf(ei->errMsg, ei->errLen,"Got error msg %d.", nlme->error);
        goto err ; 
      }
  
      len0 = len1;
      for (;NLMSG_OK(nh, len0); nh = NLMSG_NEXT(nh, len0)) 
      {
        int new = 0;
        multi = ((nh->nlmsg_flags&NLM_F_MULTI) && nh->nlmsg_type != NLMSG_DONE) ; 
        //print_nlmsg(nh,__LINE__);
        if ( NLMSG_PAYLOAD(nh,0) < sizeof(struct ifinfomsg) )
          continue ; 
        fi = (struct ifinfomsg *)NLMSG_DATA(nh);
        for ( ifi=ifh ; ifi && ifi->index != fi->ifi_index ; ifi=ifi->next ) ; // empty body
        if ( !ifi )
        {
          if (!(ifi = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,165),sizeof(nicInfo))) )
            goto err ; 
          new++ ; 
          memset(ifi,0,sizeof(nicInfo)) ; 
          if ( ift ) ift->next = ifi ; 
          else ifh = ifi ; 
          ift = ifi ; 
          ifi->index = fi->ifi_index ;
          ifi->type  = fi->ifi_type  ;
        }
        ifi->flags |= fi->ifi_flags ;
  
        rta = IFLA_RTA(fi);
        len1 = IFLA_PAYLOAD(nh);
        for (; RTA_OK(rta, len1); rta = RTA_NEXT(rta, len1)) 
        {
          if ( new )
          {
            if (rta->rta_type == IFLA_IFNAME) 
            {
              su_strlcpy(ifi->name, (char *)RTA_DATA(rta), IF_NAMESIZE) ; 
            }
            else
            if (rta->rta_type == IFLA_ADDRESS )
            {
              ifi->haddr->length = RTA_PAYLOAD(rta) ; 
              ENSURE_LEN(ifi->haddr) ; 
              memcpy(ifi->haddr->bytes, (char *)RTA_DATA(rta), ifi->haddr->length);
            }
            else
            if (rta->rta_type == IFLA_MTU) 
            {
              ifi->mtu = *((int *)RTA_DATA(rta)) ; 
            }
          }
        }
        DBG_PRT(if ( new ) printf("=> new interface: %s , %d\n",ifi->name,ifi->index));
        if (nh->nlmsg_type == NLMSG_DONE) 
          break;
      }
    } while(multi) ; 
  }

  memset(&req, 0, sizeof(req));
  req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
  req.nh.nlmsg_type = RTM_GETADDR;
  req.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
  req.nh.nlmsg_pid = 0 ; //getpid();


  for ( i=0 ; i<2 ; i++ )
  {
    int multi=0 ; 
    req.nh.nlmsg_seq = seq++ ; 
    req.u.fa.ifa_family = (i&1) ? AF_INET6 : AF_INET ;   
    len0 = req.nh.nlmsg_len ; //sizeof(req) ; 
    //print_nlmsg(&req.nh,__LINE__);
    len1 = sendto(fd, &req, len0, 0, (SA *)&nl, sizeof(nl));
    if ( len1 != len0 )
    {
      perror("send");
      snprintf(ei->errMsg, ei->errLen,"system call %s() failed with error %d (%s)","send",errno,strerror(errno)) ; 
      goto err ; 
    }

    do
    {
      len1 = recv(fd, s96,1, MSG_PEEK|MSG_TRUNC) ; 
      if ( len1 < 0 )
      {
        snprintf(ei->errMsg, ei->errLen,"system call %s() failed with error %d (%s)","recv",errno,strerror(errno)) ; 
        goto err ; 
      }
      if ( len1 > blen )
      {
        char *tmp ; 
        len0 = (len1+0xfff)&(~0xfff) ; 
        if (!(tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,166),buff,len0)) )
        {
          snprintf(ei->errMsg, ei->errLen,"_at_%d: realloc failed for %lu bytes",__LINE__,len0) ; 
          goto err ; 
        }
        buff = tmp ; 
        blen = len0 ; 
      }
      len1 = recv(fd,buff,blen,0) ; 
      DBG_PRINT("_dbg_%d: recv len1=%lu, i=%d\n",__LINE__,len1,i);
      if ( len1 < 0 )
      {
        snprintf(ei->errMsg, ei->errLen,"system call %s() failed with error %d (%s)","recv",errno,strerror(errno)) ; 
        goto err ; 
      }
      nh = (struct nlmsghdr *)buff ; 
      if (nh->nlmsg_type == NLMSG_ERROR) 
      {
        struct nlmsgerr *nlme;
        nlme = NLMSG_DATA(nh);
        snprintf(ei->errMsg, ei->errLen, "Got error msg %d.", nlme->error);
        goto err ; 
      }
  
      len0 = len1;
      for (;NLMSG_OK(nh, len0); nh = NLMSG_NEXT(nh, len0)) 
      {
        ipFlat ip , iq ;
        //print_nlmsg(nh,__LINE__);
        multi = ((nh->nlmsg_flags&NLM_F_MULTI) && nh->nlmsg_type != NLMSG_DONE) ; 
        if ( NLMSG_PAYLOAD(nh,0) < sizeof(struct ifaddrmsg) )
          continue ; 
        fa = (struct ifaddrmsg *)NLMSG_DATA(nh);
        rta = (struct rtattr *)IFA_RTA(fa);
  
          DBG_PRINT("Interface: index= %d, family=%2.2x, pref=%2.2x, flags=%2.2x, scope=%2.2x\n",
                    fa->ifa_index,fa->ifa_family,fa->ifa_prefixlen,fa->ifa_flags,fa->ifa_scope);
        for ( ifi=ifh ; ifi && ifi->index != fa->ifa_index ; ifi=ifi->next ) ; // empty body
        memset(&ip,0,sizeof(ip)) ; 
        memset(&iq,0,sizeof(iq)) ; 
        ip.scope = fa->ifa_scope ; 
  
        len1 = IFA_PAYLOAD(nh);
        for (; RTA_OK(rta, len1); rta = RTA_NEXT(rta, len1)) 
        {
          //DBG_PRINT(" rta->rta_type= %d\n",rta->rta_type);
          if(rta->rta_type == IFA_ADDRESS)
          {
            iq.length = RTA_PAYLOAD(rta) ; 
            ENSURE_LEN(&iq) ; 
            memcpy(iq.bytes, RTA_DATA(rta), iq.length) ; 
          }
          else
          if(rta->rta_type == IFA_LOCAL )
          {
            ip.length = RTA_PAYLOAD(rta) ; 
            ENSURE_LEN(&ip) ; 
            memcpy(ip.bytes, RTA_DATA(rta), ip.length) ; 
          }
          else
          if(rta->rta_type == IFA_LABEL)
          {
            size_t mxl = RTA_PAYLOAD(rta)+1 ; 
            if ( mxl > sizeof(ip.label) )
                 mxl = sizeof(ip.label) ;
            su_strlcpy(ip.label, RTA_DATA(rta), mxl) ; 
          }
        }
        if ( !ip.length && iq.length )
        {
          memcpy(ip.bytes,iq.bytes,iq.length);
          ip.length = iq.length ; 
        }
        if ( ip.length )
        {
          ipFlat *ipi, *ipp;
          if ( ifi )
          {
            for ( ipp=NULL,ipi=ifi->addrs ; ipi ; ipp=ipi,ipi=ipi->next )
              if ( ipi->length == ip.length && !memcmp(ipi->bytes,ip.bytes,ip.length) ) break ; 
            if ( !ipi )
            {
              if ( !(ipi=ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,167),sizeof(ipFlat))) )
                goto err ; 
              memcpy(ipi,&ip,sizeof(ip)) ; 
              ifi->naddr++ ; 
              if ( ipp ) ipp->next = ipi ; 
              else ifi->addrs = ipi ; 
            }
          }
          else
          {
            TRACE(5,"interface not found for address %s\n",ip2str(&ip,s96,96)) ; 
          }
        }
      }
    } while(multi) ;
  }

#if CIP_DBG
  for ( ifi=ifh ; ifi ; ifi=ifi->next ) 
  {
    ipFlat *ipi ; 
    printf(" name=%s, index=%d, mtu=%d, flags=%x\n",ifi->name,ifi->index,ifi->mtu,ifi->flags) ; 
    printf("\t hwaddr: %s\n",ip2str(ifi->haddr,s96, 96));
    for ( ipi=ifi->addrs ; ipi ; ipi=ipi->next )
      printf("\t addr: %s\n",ip2str(ipi,s96,96));
  }
#endif
  gInfo->niHead = ifh ; 
  ret = 0 ; 
 err:
  if ( fd ) close(fd) ;
  if ( buff ) ism_common_free(ism_memory_store_misc,buff) ; 
  if ( ret )
  {
    ipFlat *ipi ; 
    while ( ifh ) 
    {
      ifi = ifh ; 
      ifh = ifi->next ; 
      while ( ifi->addrs )
      {
        ipi = ifi->addrs ;
        ifi->addrs = ipi->next ; 
        ism_common_free(ism_memory_store_misc,ipi) ; 
      }
      ism_common_free(ism_memory_store_misc,ifi) ; 
    }
  }
  pthread_mutex_unlock(gInfo->haLock) ;
  return ret ;
}

//--------------------------------------------

int buildIfList(errInfo *ei)
{
  int rc ; 
  if ( (rc = buildIfList_nl(ei)) < 0 )
        rc = buildIfList_if(ei) ;
  return rc ; 
}

//--------------------------------------------

#if USE_IB
const char *rdma_ev_desc(int type)
{
  switch (type )
  {
    case RDMA_CM_EVENT_ADDR_RESOLVED :       return "RDMA_CM_EVENT_ADDR_RESOLVED";   
    case RDMA_CM_EVENT_ADDR_ERROR :          return "RDMA_CM_EVENT_ADDR_ERROR";      
    case RDMA_CM_EVENT_ROUTE_RESOLVED :      return "RDMA_CM_EVENT_ROUTE_RESOLVED";  
    case RDMA_CM_EVENT_ROUTE_ERROR :         return "RDMA_CM_EVENT_ROUTE_ERROR";     
    case RDMA_CM_EVENT_CONNECT_REQUEST :     return "RDMA_CM_EVENT_CONNECT_REQUEST"; 
    case RDMA_CM_EVENT_CONNECT_RESPONSE :    return "RDMA_CM_EVENT_CONNECT_RESPONSE";
    case RDMA_CM_EVENT_CONNECT_ERROR :       return "RDMA_CM_EVENT_CONNECT_ERROR";   
    case RDMA_CM_EVENT_UNREACHABLE :         return "RDMA_CM_EVENT_UNREACHABLE";     
    case RDMA_CM_EVENT_REJECTED :            return "RDMA_CM_EVENT_REJECTED";        
    case RDMA_CM_EVENT_ESTABLISHED :         return "RDMA_CM_EVENT_ESTABLISHED";     
    case RDMA_CM_EVENT_DISCONNECTED :        return "RDMA_CM_EVENT_DISCONNECTED";    
    case RDMA_CM_EVENT_DEVICE_REMOVAL :      return "RDMA_CM_EVENT_DEVICE_REMOVAL";  
    case RDMA_CM_EVENT_MULTICAST_JOIN :      return "RDMA_CM_EVENT_MULTICAST_JOIN";  
    case RDMA_CM_EVENT_MULTICAST_ERROR :     return "RDMA_CM_EVENT_MULTICAST_ERROR"; 
    case (RDMA_CM_EVENT_MULTICAST_ERROR+1) : return "RDMA_CM_EVENT_ADDR_CHANGE";     
    case (RDMA_CM_EVENT_MULTICAST_ERROR+2) : return "RDMA_CM_EVENT_TIMEWAIT_EXIT";   
    default :return "Unknown";
  }
}

//--------------------------------------------

int clearSendQ(ibInfo *si, errInfo *ei)
{
  int rc = StoreRC_OK ;
  char myName[]={"clearSendQ"} ;

  if ( si->ibv_ctx && si->inUse )
  {
   #define WC_SIZE 256
    struct ibv_wc wcs[WC_SIZE] , *wc ;
    struct ibv_send_wr *wr , *wrb ;
    int ret ;

    wr = (struct ibv_send_wr *)si->wr_mem ;
    do
    {
      ret = ibv_poll_cq(si->ib_cq,WC_SIZE, wcs) ;
      if ( ret > 0 )
      {
        int i ;
        for ( i=0 ; i<ret ; i++ )
        {
          wc = &wcs[i] ;
          wrb = &wr[wc->wr_id] ;
          wrb->next = si->wr1st ;
          si->wr1st = wrb ;
          si->inUse-- ;
          if ( wc->status != IBV_WC_SUCCESS )
          {
            if ( ei ) 
            {
              ei->errCode = wc->status ;
              snprintf(ei->errMsg,ei->errLen,"SockMgm(%s):  ibv_poll_cq: bad wc->status: %d , %d %d %d)",myName, wc->status,(int)wc->wr_id,wc->opcode,wc->vendor_err);
              DBG_PRINT("_ibv_%d: %s\n",__LINE__,ei->errMsg);
            }
            rc = StoreRC_SystemError ;
          }
          else
          if ( wc->wr_id != wrb->wr_id )
          {
            if ( ei ) 
            {
              ei->errCode = 1 ;
              snprintf(ei->errMsg,ei->errLen,"SockMgm(%s):  wc->wr_id != wr[wc->wr_id].wr_id: %d %d",myName, (int)wc->wr_id,(int)wrb->wr_id);
              DBG_PRINT("_ibv_%d: %s\n",__LINE__,ei->errMsg);
            }
            rc = StoreRC_SystemError ;
          }
        }
      }
      else
      if ( ret < 0 )
      {
        if ( ei ) 
        {
          ei->errCode = -ret ;
          snprintf(ei->errMsg,ei->errLen,"SockMgm(%s):  ibv_poll_cq failed with error %d (%s)",myName, -ret,strerror(-ret));
          DBG_PRINT("_ibv_%d: %s\n",__LINE__,ei->errMsg);
        }
        rc = StoreRC_SystemError ;
      }
    } while ( ret == WC_SIZE && rc == StoreRC_OK ) ;
  }
  return rc ;
}
#endif

//--------------------------------------------


int conn_write(ConnInfoRec *cInfo)
{
  size_t len ; 
  char *buf ; 

  len = (cInfo->wrInfo.reqlen-cInfo->wrInfo.offset) ;
  buf = cInfo->wrInfo.bptr ;
 #if USE_IB
  if ( cInfo->use_ib )
  {
    int ret ;
    ioInfo *si ;
    struct ibv_send_wr *wr , *wrb ;

    si = &cInfo->wrInfo ;
    if ( !si->wr1st )
    {
      errInfo ei[1] ; 
      ei->errLen = sizeof(ei->errMsg) ; 
      if ( (ret=clearSendQ((ibInfo *)&si->ibv_ctx, ei)) != StoreRC_OK )
      {
        TRACE(1,"%s\n",ei->errMsg);
        return -1 ;
      }
      if ( !si->wr1st )
        return 0 ;
    }

    wr = si->wr1st ;
    si->wr1st = wr->next ;
    si->inUse++ ;
    wr->next = NULL ;
    wr->sg_list->length = len ;
    wr->send_flags = si->send_flags[(len <= si->ifi->ib_max_inline)] ;
    memcpy((void *)(uintptr_t)wr->sg_list->addr,buf,len) ;
    wrb = NULL ;
    if ( (ret = ibv_post_send(si->ib_qp, wr, &wrb)) || wrb )
    {
      TRACE(1,"ism_storeHA_conn_write:  ibv_post_send failed with error %d (%s)",ret,strerror(ret));
      return -1 ;
    }
    return len ;
  }
 #endif

 #if USE_SSL
  if ( cInfo->use_ssl )
  {
    int ret,rc;
    struct pollfd pfd ; 
    pfd.fd = cInfo->sfd ; 
    pfd.events = POLLOUT ; 
    pthread_mutex_lock(cInfo->sslInfo->lock) ;
    for(;;)
    {
      ret = SSL_write(cInfo->sslInfo->ssl, buf, len) ;
      if ( ret > 0 )
      {
        rc = ret ; 
        break ; 
      }
      else
      {
      rc  = SSL_get_error(cInfo->sslInfo->ssl, ret) ; 
    //TRACE(9,"SSL_write:\t ret= %d, rc=%s, conn=%s\n",ret,SSL_ERRORS[rc],cInfo->req_addr);
      if ( LIKELY(rc == SSL_ERROR_NONE) )
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
        pfd.events = POLLOUT; 
      }
      else
      {
        if ( rc == SSL_ERROR_SYSCALL && ret == -1 && eWB(errno) )
        {
          pfd.events = POLLOUT; 
        }
        else
        {
          cInfo->sslInfo->func = "SSL_write" ;
          sslTraceErr(cInfo, rc, __FILE__, __LINE__);
          rc = -1 ; 
          break ; 
        }
      }
      }
      poll(&pfd, 1, 1) ; 
    }
    pthread_mutex_unlock(cInfo->sslInfo->lock) ;
    return rc ; 
  }
  else
 #endif
  return write(cInfo->sfd, buf, len) ;
}

//--------------------------------------------

#define INT_SIZE 4
#define NgetInt(b,i) {memcpy(&i,b,INT_SIZE); b+=INT_SIZE;}

#if USE_IB
int conn_read_ib(ConnInfoRec *cInfo)
{
  ioInfo *si ;
  int  ret ;
  struct ibv_wc wc[1] ;
  struct ibv_recv_wr *wr ;

  si = &cInfo->rdInfo ;

  ret = ibv_poll_cq(si->ib_cq, 1, wc) ;
  if ( ret > 0 )
  {
    if ( wc->status != IBV_WC_SUCCESS )
    {
      TRACE(1,"storeHA(%s):  ibv_poll_cq: bad wc->status: %d %d %d (%s)\n",__func__, wc->status,wc->vendor_err,(int)wc->wr_id,strerror(wc->status));
      return -1 ;
    }
    wr = (struct ibv_recv_wr *)si->wr_mem ;
    if ( wc->wr_id != wr[wc->wr_id].wr_id )
    {
      TRACE(1,"storeHA(%s):  wc->wr_id != wr[wc->wr_id].wr_id! \n",__func__) ; 
      return -1 ;
    }
    si->wr2nd = &wr[wc->wr_id] ;
    si->buffer = si->bptr = (char *)(uintptr_t)si->wr2nd->sg_list->addr ;
    si->reqlen = INT_SIZE ; 
    si->offset = 0 ;
    return wc->byte_len ;
  }
  if ( ret < 0 )
  {
    TRACE(1,"storeHA(%s):  ibv_poll_cq failed with error %d (%s)\n",__func__, -ret,strerror(-ret));
    return -1 ;
  }
  return 0 ;
}
#endif

#if USE_SSL
int conn_read_ssl(ConnInfoRec *cInfo)
{
  char *buf = cInfo->rdInfo.buffer+cInfo->rdInfo.offset ; 
  size_t len = cInfo->rdInfo.buflen-cInfo->rdInfo.offset ; 
  int ret,rc;
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
    else
    {
      rc  = SSL_get_error(cInfo->sslInfo->ssl, ret) ; 
    //TRACE(9,"SSL_read :\t ret= %d, rc=%s, conn=%s\n",ret,SSL_ERRORS[rc],cInfo->req_addr);
      if ( LIKELY(rc == SSL_ERROR_NONE) )
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
          TRACE(1,"conn_read failed: ret=%d, SSL_rc=%d, errno=%d (%s)\n",ret,rc,errno,strerror(errno));
          rc = -1 ; 
        }
        break ; 
      }
    }
  }
  pthread_mutex_unlock(cInfo->sslInfo->lock) ; 
  return rc ; 
}
#endif

int conn_read_tcp(ConnInfoRec *cInfo)
{
  char *buf = cInfo->rdInfo.buffer+cInfo->rdInfo.offset ; 
  size_t len = cInfo->rdInfo.buflen-cInfo->rdInfo.offset ; 

  int ret = read(cInfo->sfd, buf, len  ) ;
  if ( ret > 0 ) return ret ; 
  if ( ret < 0 )
  {
    int rc = errno ; 
    if ( eWB(rc) )
      return 0 ; 
    else
    {
      TRACE(1,"conn_read failed: rc=%d, (%s)\n",rc,strerror(rc));
      return -1 ; 
    }
  }
  if ( len <= 0 )
    return 0 ; 
  TRACE(5,"conn_read failed: nget=0 => EOF detected => other side closed connection.\n");
  return -1 ; 
}
//--------------------------------------------

int extractPacket(ConnInfoRec *cInfo, char **buff, uint32_t *bufflen)
{
  int nask , nget ; 
  uint32_t packlen ; 

  for ( ; ; )
  {
    if ( cInfo->rdInfo.offset >= cInfo->rdInfo.reqlen )
    {
      NgetInt(cInfo->rdInfo.bptr,packlen) ;
      cInfo->rdInfo.reqlen += packlen ; 
      if ( cInfo->rdInfo.offset >= cInfo->rdInfo.reqlen )
      {
        cInfo->rdInfo.reqlen += INT_SIZE ; 
        *buff = cInfo->rdInfo.bptr - INT_SIZE ; 
        *bufflen = packlen + INT_SIZE ; 
        cInfo->rdInfo.bptr += packlen ; 
        cInfo->rdInfo.nPacks++ ; 
        return 1 ; 
      }
      else
      {
       #if 0&&USE_SSL
        if (!(gInfo_->dInfo->state & DSC_HAVE_PAIR) && !cInfo->use_ssl && packlen > sizeof(haConAllMsg) )
        {
          uint8_t *p = (uint8_t *)(cInfo->rdInfo.bptr - INT_SIZE) ; 
          if ( p[0] == 0x16 && p[1] == 0x03 && p[2] < 0x02 )
          {
            char tmp[256] ; 
            b2h(tmp,p, 16);
            TRACE(1,"%s: An incoming message seems to be an SSL handshake!!! 1st 16 bytes: %s\n",__func__,tmp);
            gInfo_->dInfo->state |= DSC_SSL_ERR ; 
            return -1 ; 
          }
        }
       #endif
        if ( cInfo->rdInfo.buflen < cInfo->rdInfo.reqlen )
        {
          if ( packlen+INT_SIZE > cInfo->rdInfo.buflen )
          {
            if ( 1 || cInfo->use_ib || !cInfo->rdInfo.need_free )
            {
              TRACE(1,"storeHA(%s): received a packet larger (%u) than the configured receive buffer (%u)\n",__FUNCTION__,packlen ,cInfo->rdInfo.buflen);
              return -1 ; 
            }
            else
            {
              void *ptr ; 
              uint32_t bs = (packlen+INT_SIZE+4095)&(~4095) ; 
              if (!(ptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,171),cInfo->rdInfo.buffer,bs)) )
              {
                TRACE(1,"storeHA(%s): received a packet larger (%u) than the configured receive buffer (%u), realloc failed for size of %u\n",__FUNCTION__,packlen ,cInfo->rdInfo.buflen,bs);
                return -1 ; 
              }
              TRACE(5,"%s: rdInfo.buffer has been reallocated from %u to %u, packlen=%u \n",__FUNCTION__,cInfo->rdInfo.buflen,bs,packlen);
              cInfo->rdInfo.bptr = (char *)ptr + (cInfo->rdInfo.bptr - cInfo->rdInfo.buffer) ; 
              cInfo->rdInfo.buffer = ptr ; 
              cInfo->rdInfo.buflen = bs ;
            }
          }
        }
        cInfo->rdInfo.bptr -= INT_SIZE ; 
        cInfo->rdInfo.reqlen -= packlen ; 
      }
    }
    if ( !cInfo->use_ib )
    {
      nask = cInfo->rdInfo.buflen-cInfo->rdInfo.offset ; 
      if ( 2*nask < cInfo->rdInfo.buflen )
      {
        int n = (int)(cInfo->rdInfo.bptr - cInfo->rdInfo.buffer) ; 
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

    nget = cInfo->conn_read(cInfo) ; 
    if ( nget > 0 )
    {
      cInfo->rdInfo.nBytes += nget ; 
      cInfo->rdInfo.offset += nget ; 
      continue ; 
    }
    return nget ; 
  }
}

//--------------------------------------------

int doSelect(ConnInfoRec *cInfo)
{
  int nOk=0 ; 

  do
  {
   #if USE_IB
    if ( cInfo->use_ib && !cInfo->rdInfo.inUse )
    {
      if ( !ibv_req_notify_cq(cInfo->rdInfo.ib_cq,0) )
        cInfo->rdInfo.inUse++ ; 
    }
   #endif

    poll(cInfo->pfds, cInfo->nfds, 10) ; 

   #if USE_IB
    if ( cInfo->use_ib && cInfo->init_here )
    {
      if ( (cInfo->pfds[1].revents&POLLIN) )
      {
        int rc ; 
        struct rdma_cm_event *cm_ev ; 
        do
        {
          if ( (rc = Xrdma_get_cm_event(cInfo->cm_id->channel , &cm_ev)) )
          {
            rc = errno ; 
            if ( !eWB(rc) )
            {
              TRACE(1,"The TCP receiver call to the InfiniBand verbs API (rmda_get_cm_event) failed. The error is %d (%s).\n",rc,strerror(rc) );
              cInfo->is_invalid |= C_INVALID ; 
              TRACE(5,"HA Connection marked as invalid: %s \n",cInfo->req_addr);
            }
            break ; 
          }
          if (!cm_ev )
          {
            TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) return a value of NULL for cm_ev.\n");
            cInfo->is_invalid |= C_INVALID ; 
            TRACE(5,"HA Connection marked as invalid: %s \n",cInfo->req_addr);
            break ; 
          }
          if ( cm_ev->event == RDMA_CM_EVENT_DISCONNECTED ||
               cm_ev->event == RDMA_CM_EVENT_ADDR_CHANGE  )
          {
            cInfo->is_invalid |= C_INVALID ; 
            TRACE(5,"HA Connection marked as invalid: %s \n",cInfo->req_addr);
          }
          else
          {
            TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) returned an event of type %s status %d, while expecting an event of type %s.\n", 
              rdma_ev_desc(cm_ev->event),cm_ev->status,rdma_ev_desc(RDMA_CM_EVENT_DISCONNECTED));
          }
          Xrdma_ack_cm_event(cm_ev) ; 
        } while (0) ; 
      }
    }
   #endif
    if ( (cInfo->pfds[0].revents&POLLIN) )
    {
     #if USE_IB
      if ( cInfo->use_ib )
      {
        struct ibv_cq *cq ; 
        void *cq_ctx ; 
        while ( !Xibv_get_cq_event(cInfo->rdInfo.ib_ch, &cq, &cq_ctx) )
        {
          cInfo->rdInfo.inUse = 0 ; 
          Xibv_ack_cq_events(cq,1) ; 
        }
      }
     #endif
      nOk++ ; 
    }
   #if USE_IB
    if ( cInfo->use_ib && cInfo->rdInfo.inUse<8 )
    {
   // cInfo->rdInfo.inUse++ ; 
      nOk++ ; 
    }
   #endif
  } while ( !nOk && !viewBreak(gInfo_) && !cInfo->is_invalid ) ;
  return nOk ; 
}

//--------------------------------------------

#if USE_IB
static int ibLoadCnt=0;
static LIBTYPE iblib[2]={NULL,NULL} ;
int loadIbLib(errInfo *ei)
{
  int i;
  const char *ibname[3] = {"libibverbs.so","libibverbs.so.1","libibverbs.so.2"};
  const char *rdname[3] = {"librdmacm.so", "librdmacm.so.1", "librdmacm.so.2"};

  pthread_mutex_lock(gInfo->haLock) ;
  if ( ibLoadCnt )
  {
    ibLoadCnt++ ;
    pthread_mutex_unlock(gInfo->haLock) ;
    return StoreRC_OK ; 
  }

  for ( i=0 ; i<3 ; i++ )
    if ( (iblib[0] = (LIBTYPE) dlopen(ibname[i] DLLOAD_OPT)) )
      break ; 

  if ( !iblib[0] )
  {
    if (ei && ei->errLen>0)
      dlerror_string(ei->errMsg, ei->errLen);
    pthread_mutex_unlock(gInfo->haLock) ;
    return StoreRC_SystemError ; 
  }

  if ( !GETSYM(iblib[0], ibv_ack_cq_events               ) ||
       !GETSYM(iblib[0], ibv_alloc_pd                    ) ||
       !GETSYM(iblib[0], ibv_create_ah                   ) ||
       !GETSYM(iblib[0], ibv_create_comp_channel         ) ||
       !GETSYM(iblib[0], ibv_create_cq                   ) ||
       !GETSYM(iblib[0], ibv_dealloc_pd                  ) ||
       !GETSYM(iblib[0], ibv_dereg_mr                    ) ||
       !GETSYM(iblib[0], ibv_destroy_ah                  ) ||
       !GETSYM(iblib[0], ibv_destroy_comp_channel        ) ||
       !GETSYM(iblib[0], ibv_destroy_cq                  ) ||
       !GETSYM(iblib[0], ibv_get_cq_event                ) ||
       !GETSYM(iblib[0], ibv_modify_qp                   ) ||
       !GETSYM(iblib[0], ibv_query_port                  ) ||
       !GETSYM(iblib[0], ibv_query_qp                    ) ||
       !GETSYM(iblib[0], ibv_attach_mcast                ) ||
       !GETSYM(iblib[0], ibv_detach_mcast                ) ||
       !GETSYM(iblib[0], ibv_query_device                ) ||
      #if USE_IBEL
       !GETSYM(iblib[0], ibv_get_async_event             ) ||
       !GETSYM(iblib[0], ibv_ack_async_event             ) ||
      #endif
       !GETSYM(iblib[0], ibv_fork_init                   ) ||
       !GETSYM(iblib[0], ibv_create_qp                   ) ||
       !GETSYM(iblib[0], ibv_destroy_qp                  ) ||
       !GETSYM(iblib[0], ibv_reg_mr                      )  )
  {
    dlclose((LIBTYPE)iblib[0]);
    su_strlcpy(ei->errMsg, "error resolving ibv symbols", ei->errLen);
    pthread_mutex_unlock(gInfo->haLock) ;
    return StoreRC_SystemError ; 
  }

  for ( i=0 ; i<3 ; i++ )
    if ( (iblib[1] = (LIBTYPE) dlopen(rdname[i] DLLOAD_OPT)) )
      break ; 

  if ( !iblib[1] )
  {
    if (ei->errMsg && (ei->errLen>0))
      dlerror_string(ei->errMsg, ei->errLen);
    dlclose((LIBTYPE)iblib[0]);
    pthread_mutex_unlock(gInfo->haLock) ;
    return StoreRC_SystemError ; 
  }

  if ( !GETSYM(iblib[1], rdma_accept                     ) ||
       !GETSYM(iblib[1], rdma_ack_cm_event               ) ||
       !GETSYM(iblib[1], rdma_bind_addr                  ) ||
       !GETSYM(iblib[1], rdma_connect                    ) ||
       !GETSYM(iblib[1], rdma_disconnect                 ) ||
       !GETSYM(iblib[1], rdma_create_event_channel       ) ||
       !GETSYM(iblib[1], rdma_create_id                  ) ||
       !GETSYM(iblib[1], rdma_create_qp                  ) ||
       !GETSYM(iblib[1], rdma_destroy_qp                 ) ||
       !GETSYM(iblib[1], rdma_destroy_event_channel      ) ||
       !GETSYM(iblib[1], rdma_destroy_id                 ) ||
       !GETSYM(iblib[1], rdma_get_cm_event               ) ||
       !GETSYM(iblib[1], rdma_join_multicast             ) ||
       !GETSYM(iblib[1], rdma_leave_multicast            ) ||
       !GETSYM(iblib[1], rdma_listen                     ) ||
       !GETSYM(iblib[1], rdma_resolve_addr               ) ||
       !GETSYM(iblib[1], rdma_resolve_route              )  )
  {
    dlclose((LIBTYPE)iblib[0]);
    dlclose((LIBTYPE)iblib[1]);
    su_strlcpy(ei->errMsg, "error resolving rdma symbols", ei->errLen);
    pthread_mutex_unlock(gInfo->haLock) ;
    return StoreRC_SystemError ; 
  }

  ibLoadCnt++ ;
  pthread_mutex_unlock(gInfo->haLock) ;
  return StoreRC_OK ; 
}

/*-------------------------------------------------*/


int unloadIbLib(void)
{
  pthread_mutex_lock(gInfo->haLock) ;
  ibLoadCnt-- ;
  if ( ibLoadCnt )
  {
    if ( ibLoadCnt < 0 )
      ibLoadCnt = 0 ;
  }
  else
  {
    dlclose((LIBTYPE)iblib[0]);
    dlclose((LIBTYPE)iblib[1]);
  }

  pthread_mutex_unlock(gInfo->haLock) ;
  return StoreRC_OK ; 
}
#endif

//--------------------------------------------

int haGetAddr(int af, int *dns, const char *src, ipFlat *ip)
{
  int rc = AF_UNSPEC ;
  if ( src && *src )
  {
    if ( af == AF_INET )
    {
      if ( inet_pton(AF_INET,src,ip->bytes) > 0 )
      {
        ip->length = sizeof(IA4) ;
        rc = AF_INET ;
      }
    }
    else
    if ( af == AF_INET6 )
    {
      if ( inet_pton(AF_INET6,src,ip->bytes) > 0 )
      {
        ip->length = sizeof(IA6) ;
        rc = AF_INET6 ;
      }
    }
    else
    {
      if ( inet_pton(AF_INET,src,ip->bytes) > 0 )
      {
        ip->length = sizeof(IA4) ;
        rc = AF_INET ;
      }
      else
      if ( inet_pton(AF_INET6,src,ip->bytes) > 0 )
      {
        ip->length = sizeof(IA6) ;
        rc = AF_INET6 ;
      }
    }

    if ( dns && *dns )
    {
      if ( rc != AF_UNSPEC ) 
        *dns = 0 ;
      else
      {
        double et;
        struct addrinfo h, *r, *ri ; 
        int ec ; 
        memset(&h, 0, sizeof(struct addrinfo)) ; 
        h.ai_family = af ;
        et = su_sysTime() + 9e1 ; 
        do
        {
          r = NULL ; 
          ec = getaddrinfo(src,NULL, &h, &r) ;
          if ( ec != EAI_AGAIN && ec != EAI_NODATA && ec != EAI_NONAME )
            break ; 
          su_sleep(10,SU_MIL) ; 
        } while ( et > su_sysTime() ) ; 
        if ( ec == 0 )
        {
          int scope=0 ; 
          for ( ri=r ; ri ; ri=ri->ai_next )
          {
            if ( SHOULD_TRACE(9) )
            {
              char addr[ADDR_STR_LEN], pstr[8] ;
              errInfo ei[1];
              ei->errLen = sizeof(ei->errMsg) ; 
              haGetNameInfo(ri->ai_addr,addr,ADDR_STR_LEN,pstr,8,ei) ;
              TRACE(9," haGetAddr: addr %s has returned for %s\n",addr,src) ; 
            }
            if ( ri->ai_family == AF_INET )
            {
              ip->length = sizeof(IA4) ; 
              memcpy(ip->bytes,&(((SA4 *)ri->ai_addr)->sin_addr),ip->length) ; 
              rc = AF_INET ; 
              break ; 
            }
            else
            if ( ri->ai_family == AF_INET6 )
            {
              int sc = 0 ; 
              if ( IN6_IS_ADDR_LINKLOCAL(&(((SA6 *)ri->ai_addr)->sin6_addr)) )
                sc = 1 ;
              else
              if ( IN6_IS_ADDR_SITELOCAL(&(((SA6 *)ri->ai_addr)->sin6_addr)) )
                sc = 2 ;
              else
                sc = 3 ;
              if ( sc > scope )
              {
                ip->length = sizeof(IA6) ; 
                memcpy(ip->bytes,&(((SA6 *)ri->ai_addr)->sin6_addr),ip->length) ; 
                rc = AF_INET6 ; 
                scope = sc ;
              }
            }
            else
              continue ; 
          } 
          if ( r )
            freeaddrinfo(r) ; 
        }
      }
    }
  }
  return rc ; 
}

//--------------------------------------------

int haGetNameInfo(SA *sa, char *addr, size_t a_len, char *port, size_t p_len, errInfo *ei)
{
  int af , iport;
  SA4 *sa4 ;
  SA6 *sa6 ;
  void *a_p ;

  ei->errCode = 0 ;
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
    ei->errCode = EAFNOSUPPORT ;
   #endif
    snprintf(ei->errMsg,ei->errLen," Failed to convert address, unknown af: %d ",af) ;
    return -1 ;
  }

  if ( port!=NULL && p_len>0 )
  {
    snprintf(port,p_len,"%u",iport) ;
    port[p_len-1] = 0 ;
  }
  if ( addr && !inet_ntop(af, a_p, addr, a_len) )
  {
    ei->errCode = errno ;
    snprintf(ei->errMsg,ei->errLen," Failed to convert address using inet_ntop, error %d (%s)",ei->errCode,strerror(ei->errCode)) ;
    return -1 ;
  }
  else
    return 0 ;
}

//--------------------------------------------

int ha_set_nb(int fd, errInfo *ei, const char *myName)
{
  int    ival ;
  if ( (int)(ival = fcntl(fd,F_GETFL)) == -1 )
  {
    ei->errCode = errno ;
    snprintf(ei->errMsg,ei->errLen,"SockMgm(%s):  fcntl F_GETFL failed with error %d (%s)",myName, ei->errCode,strerror(ei->errCode));
    return -1 ;
  }

  if ( !(ival&O_NONBLOCK) && fcntl(fd,F_SETFL,ival|O_NONBLOCK) == -1 )
  {
    ei->errCode = errno ;
    snprintf(ei->errMsg,ei->errLen,"SockMgm(%s):  fcntl F_SETFL failed with error %d (%s)",myName, ei->errCode,strerror(ei->errCode));
    return -1 ;
  }
  return 0 ;
}
//--------------------------------------------

#if USE_IB
static void freeIfIbInfo(ifInfo *ifi)
{
  if ( ifi->ib_pd ) {Xibv_dealloc_pd(ifi->ib_pd) ; ifi->ib_pd=NULL ;}
  if ( ifi->cm_id ) 
  {
    struct rdma_event_channel *ev_ch ;
    ev_ch = ifi->cm_id->channel ;
    Xrdma_destroy_id(ifi->cm_id) ; ifi->cm_id = NULL ;
    if ( ev_ch ) Xrdma_destroy_event_channel(ev_ch) ;
  }
}

/*-------------------------------------------------*/

static int buildIfIbInfo(ifInfo *cur_ifi, int req_af, int port_space, errInfo *ei)
{
  int rc ; 
  struct ibv_device_attr ibv_attr ;
  struct ibv_port_attr port_attr ;
  struct rdma_event_channel *ev_ch ;
  SAS sa_s ;
  SA  *sa_ ;
  SA4 *sa_4 ;
  SA6 *sa_6 ;
  int ret = -1 ; 
  const char *myName = __FUNCTION__ ; 

  freeIfIbInfo(cur_ifi);
  do
  {
    TRDBG(snprintf(traceMsg, sizeof(traceMsg),"trying to bind to IB device cm_id...\n")) ;
    if ( !(ev_ch = Xrdma_create_event_channel()) )
    {
      ei->errCode = -1 ;
      snprintf(ei->errMsg,ei->errLen," rdma_create_event_channel failed (rc=%d)",ei->errCode) ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_7: _rdma_%d: rdma_create_event_channel \n",__LINE__));
      continue ;
    }
    if ( ha_set_nb(ev_ch->fd, ei, myName) != 0 )
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_14: could not ha_set_nb! (rc=%d)\n",ei->errCode));
      continue ;
    }
    if ( (rc = Xrdma_create_id(ev_ch, &cur_ifi->cm_id, cur_ifi, port_space)) )
    {
      ei->errCode = rc ;
      snprintf(ei->errMsg,ei->errLen," rdma_create_id failed (rc=%d)",ei->errCode) ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_8: _ibv_%d: rdma_create_id (rc=%d)\n",__LINE__,rc));
      continue ;
    }
    sa_  = (SA *)&sa_s ;
    sa_4 = (SA4 *)sa_ ;
    sa_6 = (SA6 *)sa_ ;
    memset(sa_,0,sizeof(SAS)) ;
    if ( cur_ifi->tt_addr )
    {
      ipFlat *ip = cur_ifi->tt_addr ; 
      if ( ip->length == sizeof(IA4) )
      {
        sa_->sa_family = AF_INET ;
        memcpy(&sa_4->sin_addr,ip->bytes,sizeof(IA4)) ;
      }
      else
      {
        sa_->sa_family = AF_INET6 ;
        if ( IN6_IS_ADDR_LINKLOCAL((IA6 *)ip->bytes) )
          sa_6->sin6_scope_id = cur_ifi->link_scope ;
        memcpy(&sa_6->sin6_addr,ip->bytes,sizeof(IA6)) ;
      }
    }
    else
    if ( req_af==AF_INET && IF_HAVE_IPV4(cur_ifi) )
    {
      sa_->sa_family = AF_INET ;
      memcpy(&sa_4->sin_addr,&cur_ifi->ipv4_addr,sizeof(IA4)) ;
    }
    else
    if ( req_af==AF_INET6 && IF_HAVE_LINK(cur_ifi) )
    {
      sa_->sa_family = AF_INET6 ;
      sa_6->sin6_scope_id = cur_ifi->link_scope ;
      memcpy(&sa_6->sin6_addr,&cur_ifi->link_addr,sizeof(IA6)) ;
    }
    else
    if ( req_af==AF_INET6 && IF_HAVE_SITE(cur_ifi) )
    {
      sa_->sa_family = AF_INET6 ;
      sa_6->sin6_scope_id = cur_ifi->site_scope ;
      memcpy(&sa_6->sin6_addr,&cur_ifi->site_addr,sizeof(IA6)) ;
    }
    else
    if ( req_af==AF_INET6 && IF_HAVE_GLOB(cur_ifi) )
    {
      sa_->sa_family = AF_INET6 ;
      sa_6->sin6_scope_id = cur_ifi->glob_scope ;
      memcpy(&sa_6->sin6_addr,&cur_ifi->glob_addr,sizeof(IA6)) ;
    }
    else
    if ( IF_HAVE_IPV4(cur_ifi) )
    {
      sa_->sa_family = AF_INET ;
      memcpy(&sa_4->sin_addr,&cur_ifi->ipv4_addr,sizeof(IA4)) ;
    }
    else
    if ( IF_HAVE_LINK(cur_ifi) )
    {
      sa_->sa_family = AF_INET6 ;
      sa_6->sin6_scope_id = cur_ifi->link_scope ;
      memcpy(&sa_6->sin6_addr,&cur_ifi->link_addr,sizeof(IA6)) ;
    }
    else
    if ( IF_HAVE_SITE(cur_ifi) )
    {
      sa_->sa_family = AF_INET6 ;
      sa_6->sin6_scope_id = cur_ifi->site_scope ;
      memcpy(&sa_6->sin6_addr,&cur_ifi->site_addr,sizeof(IA6)) ;
    }
    else
    if ( IF_HAVE_GLOB(cur_ifi) )
    {
      sa_->sa_family = AF_INET6 ;
      sa_6->sin6_scope_id = cur_ifi->glob_scope ;
      memcpy(&sa_6->sin6_addr,&cur_ifi->glob_addr,sizeof(IA6)) ;
    }
    else
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_9: no addresses found!\n"));
      continue ;
    }
   #if TRACE_DEBUG
    {
      char addr[ADDR_STR_LEN], port[8] ;
      haGetNameInfo(sa_,addr,ADDR_STR_LEN,port,8,ei) ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"cm_id_bind_%d binding to _%s_%s_ \n",__LINE__,addr,port)) ;
    }
   #endif
    if ( (rc = Xrdma_bind_addr(cur_ifi->cm_id, sa_)) )
    {
      char addr[ADDR_STR_LEN], port[8] ;
      haGetNameInfo(sa_,addr,ADDR_STR_LEN,port,8,ei) ;
      rc = errno ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_10: could not bind to _%s_%s_ (rc=%d %s)\n",addr,port,rc,strerror(rc))) ;
      continue ;
    }
    if ( !cur_ifi->cm_id->verbs )
    {
      char addr[ADDR_STR_LEN], port[8] ;
      haGetNameInfo(sa_,addr,ADDR_STR_LEN,port,8,ei) ;
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_10.1: rdma bound to _%s_%s_ without attaching to an HCA\n",addr,port,rc)) ;
      continue ;
    }
    cur_ifi->ibv_ctx = cur_ifi->cm_id->verbs ;
    cur_ifi->ib_port = cur_ifi->cm_id->port_num ;
    cur_ifi->ib_iwarp= 0 ; //(cur_ifi->ibv_ctx->device->transport_type==IBV_TRANSPORT_IWARP) ;
    cur_ifi->ib_max_inline = cur_ifi->ib_iwarp ? MAX_INLINE_IW : MAX_INLINE_IB ;
    if ( (rc = Xibv_query_device(cur_ifi->ibv_ctx, &ibv_attr)) )
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_11: could not query device! (rc=%d)\n",rc)) ;
      continue ;
    }
    cur_ifi->ib_max_wr = ibv_attr.max_qp_wr ;

    cur_ifi->ib_pd = Xibv_alloc_pd(cur_ifi->ibv_ctx) ;
    if ( !cur_ifi->ib_pd )
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_12: could not allocate a protection domain! \n")) ;
      continue ;
    }
    if ( (rc = Xibv_query_port(cur_ifi->ibv_ctx, cur_ifi->ib_port, &port_attr)) )
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"drop_place_13: could not query port! (rc=%d)\n",rc));
      continue ;
    }
    cur_ifi->ib_mtu = 0x80<<port_attr.active_mtu ;
    cur_ifi->ib_state = port_attr.state ; 
    TRDBG(snprintf(traceMsg, sizeof(traceMsg),"_ibv_port_details: port= %d ; state= %d ; space= %x ; max_mtu= %d ; act_mtu= %d ; msg_size= %u ; lid= %d ; gids= %d ; max_wr= %d\n",cur_ifi->ib_port, port_attr.state,port_space,port_attr.max_mtu,0x80<<port_attr.active_mtu,port_attr.max_msg_sz,port_attr.lid,port_attr.gid_tbl_len,cur_ifi->ib_max_wr));
    if ( port_attr.state != IBV_PORT_ACTIVE )
    {
      TRDBG(snprintf(traceMsg, sizeof(traceMsg),"_ibv_port_warning: ib_port %d is not in the expected 'IBV_PORT_ACTIVE' (%d) state!\n",cur_ifi->ib_port,IBV_PORT_ACTIVE)); 
    }
    ret = 0 ;  
  } while(0) ; 
  if (ret) 
    freeIfIbInfo(cur_ifi);
  return ret; 
}
#endif

//--------------------------------------------

int getLocalIf(const char *try_this, int use_ib, int req_mc, ifInfo *ifi, int nono_ind)
{
  int iTTT, ll , req_iff, lb=0 ; 
  ipFlat ip[1] , *iq=NULL , *fq ; 
  nicInfo *ni=NULL ; 
  int alias=0, dns=1 ; 
  errInfo ei[1];
  ei->errLen = sizeof(ei->errMsg) ; 
  haGlobalInfo *gInfo=gInfo_;

  memset(ip,0,sizeof(ipFlat)) ; 
  if ( !try_this || !(*try_this) || !strcasecmp(try_this,"none") )
    iTTT = TRY_THIS_NONE ; 
  else
  if ( haGetAddr(AF_UNSPEC, &dns, try_this, ip) != AF_UNSPEC )
    iTTT = TRY_THIS_ADDR ; 
  else
  {
    iTTT = TRY_THIS_NAME ; 
    alias = (strchr(try_this,':') != NULL) ; 
  }
  req_iff = req_mc ? (IFF_UP|IFF_MULTICAST) : IFF_UP ; 
  for ( ni=gInfo->niHead ; ni ; ni=ni->next )
  {
    if (!ni->addrs ) continue ; 
    if ( nono_ind && nono_ind == ni->index ) continue ; 
    if ( (ni->flags&req_iff) != req_iff ) continue ; 
    iq = NULL ; 
    if ( iTTT == TRY_THIS_ADDR )
    {
      for ( iq=ni->addrs ; iq ; iq=iq->next )
      {
        if ( ip->length == iq->length && !memcmp(ip->bytes,iq->bytes,ip->length) ) break ; 
      }
      if (!iq ) continue ; 
      if ( dns && (ni->flags&IFF_LOOPBACK) )
      {
        lb = 1 ; 
        break ; 
      }
    }
    else
    if ( iTTT == TRY_THIS_NAME )
    {
      if ( alias )
      {
        for ( iq=ni->addrs ; iq ; iq=iq->next )
        {
          if (!strcasecmp(try_this,iq->label) ) break ; 
        }
        if (!iq ) continue ; 
      }
      else
        if ( strcasecmp(try_this,ni->name) ) continue ; 
    }
    else
    if ( ni->type != ARPHRD_ETHER && ni->type != ARPHRD_INFINIBAND ) continue ; 

    memset(ifi,0,sizeof(ifInfo)) ; 
    ifi->ni = ni ; 
    su_strlcpy(ifi->user_name,try_this,HOSTNAME_STR_LEN) ; 
    if ( alias && iq )
      su_strlcpy(ifi->name,iq->label,IF_NAMESIZE) ; 
    else
      su_strlcpy(ifi->name,ni->name,IF_NAMESIZE) ; 
    ifi->index = ni->index ; 
    ifi->flags = ni->flags ; 
    ifi->iTTT = iTTT ; 
    ifi->tt_addr = iq ; 
    fq = iq ; 
    ll = 0 ; 
    for ( iq=ni->addrs ; iq ; )
    {
      if ( iq->length == sizeof(IA4) )
      {
        memcpy(&ifi->ipv4_addr, iq->bytes, iq->length) ; 
        ifi->have_addr |= 0x1 ; 
      }
      else
      {
        if ( IN6_IS_ADDR_LINKLOCAL((IA6 *)iq->bytes) )
        {
          memcpy(&ifi->link_addr, iq->bytes, iq->length) ; 
          ifi->have_addr |= 0x2 ; 
        //ifi->link_scope = iq->scope ; 
          ifi->link_scope = ifi->index ;
        }
        else
        if ( IN6_IS_ADDR_SITELOCAL((IA6 *)iq->bytes) )
        {
          memcpy(&ifi->site_addr, iq->bytes, iq->length) ; 
          ifi->have_addr |= 0x4 ; 
          ifi->site_scope = iq->scope ; 
        }
        else
        {
          memcpy(&ifi->glob_addr, iq->bytes, iq->length) ; 
          ifi->have_addr |= 0x8 ; 
          ifi->glob_scope = iq->scope ; 
        }
      }
      if ( ll ) break ; 
      iq = iq->next ; 
      if ( !iq )
      {
        if ( !fq ) break ; 
        iq = fq ; 
        ll = 1 ; 
      }
    }
   #if USE_IB
    if ( use_ib && buildIfIbInfo(ifi, AF_UNSPEC, RDMA_PS_TCP, ei) < 0 )
    {
      if ( ifi->iTTT == TRY_THIS_NONE ) continue ; 
      TRACE(1,"storeHA(getLocalIf): buildIfIbInfo failed for the given HA.ReplicationNIC (|%s|)\n",try_this);
      return -1 ; 
    }
   #endif
    if ( ifi->tt_addr )
    {
      iq = ifi->tt_addr ; 
      if ( iq->length == sizeof(IA4) )
        inet_ntop(AF_INET, iq->bytes, ifi->addr_name, ADDR_STR_LEN) ; 
      else
        inet_ntop(AF_INET6,iq->bytes, ifi->addr_name, ADDR_STR_LEN) ; 
    }
    else
    if ( IF_HAVE_IPV4(ifi) )
      inet_ntop(AF_INET, &ifi->ipv4_addr, ifi->addr_name, ADDR_STR_LEN) ; 
    else
    if ( IF_HAVE_GLOB(ifi) )
      inet_ntop(AF_INET6,&ifi->glob_addr, ifi->addr_name, ADDR_STR_LEN) ; 
    else
    if ( IF_HAVE_SITE(ifi) )
      inet_ntop(AF_INET6,&ifi->site_addr, ifi->addr_name, ADDR_STR_LEN) ; 
    else
    if ( IF_HAVE_LINK(ifi) )
      inet_ntop(AF_INET6,&ifi->link_addr, ifi->addr_name, ADDR_STR_LEN) ; 
    else
      su_strlcpy(ifi->addr_name, "Unknown", ADDR_STR_LEN);
    return 0 ; 
  }

  if ( lb )
  {
    TRACE(1,"storeHA(getLocalIf): the given NIC (|%s|) resolved to the loopback interface\n",try_this);
    return -2 ; 
  }
  else
  {
    TRACE(1,"storeHA(getLocalIf): no local interface found for the given NIC (|%s|)\n",try_this);
    return -1 ; 
  }
}

//--------------------------------------------

int buildIpList(int allowNoAddrs, double retryTO)
{
  ipInfo *ipH=NULL, *ipT=NULL ; 
  char *p, *q , port[8] , *tstr ; 
  int rc, loc=0, none=0;
  haGlobalInfo *gInfo=gInfo_;

  if ( !(tstr = su_strdup(gInfo->config->NodesAddr)) )
  {
    TRACE(1,"ism_storeHA_start: malloc failed for %lu bytes\n",su_strlen(gInfo->config->NodesAddr));
    return StoreRC_SystemError ; 
  }
  snprintf(port,8,"%d",gInfo->config->ExtPort) ; 
  rc = StoreRC_OK ; 
  for ( p=tstr, q=NULL ; ; p++ )
  {
    if (!q && !isspace(*p) )
      q = p ; 
    if ( *p == ',' || !(*p) )
    {
      if ( q < p )
      {
        double et;
        ipFlat ti[1] , *tj ; 
        nicInfo *ni ; 
        struct addrinfo h, *r, *ri ; 
        int ec ; 
        char s=(*p) ; 
        *p = 0 ; 
        if ( !strcasecmp(q, "none") )
          none = 1 ; 
        else
        {
          memset(ti,0,sizeof(ipFlat)) ; 
          memset(&h,0,sizeof(h)) ; 
          h.ai_flags = AI_NUMERICSERV ; 
          h.ai_family = AF_UNSPEC ; 
          h.ai_socktype = SOCK_STREAM ; 
          h.ai_protocol = IPPROTO_TCP ; 
          et = su_sysTime() + retryTO ; 
          do
          {
            r = NULL ; 
            ec = getaddrinfo(q, port, &h, &r) ; 
            if ( ec == 0 || ec == EAI_ADDRFAMILY || ec == EAI_FAMILY )
              break ; 
            if ( ec != EAI_AGAIN && ec != EAI_NODATA && ec != EAI_NONAME )
            {
              TRACE(1,"ism_storeHA_start: getaddrinfo failed for %s:%s, ec= %d (%s)\n",q,port,ec,gai_strerror(ec)) ; 
              rc = StoreRC_SystemError ; 
              goto exit ; 
            }
            su_sleep(10,SU_MIL) ; 
          } while ( et > su_sysTime() ) ; 
          if ( ec == 0 )
          {
            ipInfo *ip ; 
            for ( ri=r ; ri ; ri=ri->ai_next )
            {
              if ( ri->ai_family == AF_INET )
              {
                ti->length = sizeof(IA4) ; 
                memcpy(ti->bytes,&(((SA4 *)ri->ai_addr)->sin_addr),ti->length) ; 
              }
              else
              {
                ti->length = sizeof(IA6) ; 
                memcpy(ti->bytes,&(((SA6 *)ri->ai_addr)->sin6_addr),ti->length) ; 
              }
              for ( ni=gInfo->niHead ; ni ; ni=ni->next )
              {
                for ( tj=ni->addrs ; tj ; tj=tj->next )
                {
                  if ( ti->length == tj->length && !memcmp(ti->bytes,tj->bytes,ti->length) )
                  {
                    loc++ ; 
                    break ; 
                  }
                }
                if ( tj )
                  break ; 
              }
              if ( ni )
                break ; 
              if (!(ip = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,172),sizeof(ipInfo))) )
              {
                TRACE(1,"ism_storeHA_start: malloc failed for %lu bytes\n",sizeof(ipInfo)) ; 
                rc = StoreRC_SystemError ; 
                goto exit ; 
              }
              IP_INIT(ip) ; 
              IP_COPY(ip,ri) ; 
              if ( ipT )
                ipT->next = ip ; 
              else
                ipH = ip ; 
              if ( SHOULD_TRACE(9) )
              {
                char addr[ADDR_STR_LEN], pstr[8] ;
                errInfo ei[1];
                ei->errLen = sizeof(ei->errMsg) ; 
                haGetNameInfo(IP_SA(ip),addr,ADDR_STR_LEN,pstr,8,ei) ;
                TRACE(9," buildIpList: addr %s added for %s\n",addr,q) ; 
              }
              ipT = ip ; 
            } 
            if ( r )
              freeaddrinfo(r) ; 
          }
          else
          {
            TRACE(1,"ism_storeHA_start: getaddrinfo failed for %s:%s, ec= %d (%s)\n",q,port,ec,gai_strerror(ec)) ; 
          }        
        }        
        *p = s ; 
      }
      q = NULL ; 
    }
    if ( !(*p) )
      break ; 
  }
 exit:
  ism_common_free(ism_memory_store_misc,tstr) ; 
  if ( !ipH )
  {
    if ( none )
    {
      if ( !allowNoAddrs )
      {
        TRACE(1," 'NONE' is detected for the %s param %s.\n",ismHA_CFG_REMOTEDISCOVERYNIC,gInfo->config->NodesAddr);
      }
    }
    else
    if ( allowNoAddrs )
    {
      TRACE(6," No addresses found for the %s param %s.\n",ismHA_CFG_REMOTEDISCOVERYNIC,gInfo->config->NodesAddr);
    }
    else
    {
      if ( loc )
      {
        TRACE(1," No addresses found for the %s param %s. The address provided is a local address.\n",ismHA_CFG_REMOTEDISCOVERYNIC,gInfo->config->NodesAddr);
        //ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s%s", ismHA_CFG_REMOTEDISCOVERYNIC, gInfo->config->NodesAddr,"Address provided for the HA RemoteDiscoveryNIC is a local address");
        ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s", ismHA_CFG_REMOTEDISCOVERYNIC, gInfo->config->NodesAddr);
      }
      else
      {
        TRACE(1," No addresses found for the %s param %s.\n",ismHA_CFG_REMOTEDISCOVERYNIC,gInfo->config->NodesAddr);
        //ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s%s", ismHA_CFG_REMOTEDISCOVERYNIC, gInfo->config->NodesAddr,"Address provided for the HA RemoteDiscoveryNIC is not valid or cannot be used");
        ism_common_setErrorData(ISMRC_StoreHABadConfigValue, "%s%s", ismHA_CFG_REMOTEDISCOVERYNIC, gInfo->config->NodesAddr);
      }
      rc = StoreRC_BadParameter ;
    }
  }
  else
  if ( rc == StoreRC_OK )
  {
    ipInfo *ip ; 
    if ( !gInfo->ipHead )
      gInfo->ipHead = ipH ; 
    else
    {
      while( ipH )
      {
        for ( ip=gInfo->ipHead ; ip ; ip=ip->next )
          if ( ip->ai.ai_addrlen == ipH->ai.ai_addrlen && !memcmp(ip->ai.ai_addr, ipH->ai.ai_addr, ip->ai.ai_addrlen) )
            break ; 
        if ( ip )
        {
          ip = ipH->next ; 
          ism_common_free(ism_memory_store_misc,ipH) ; 
          ipH = ip ; 
        }
        else
        {
          ip = ipH->next ; 
          ipH->next = gInfo->ipHead ; 
          gInfo->ipHead = ipH ; 
          ipH = ip ; 
        }
      }
    }
  }
  else
  while( ipH )
  {
    ipInfo *ip = ipH->next ; 
    ism_common_free(ism_memory_store_misc,ipH) ; 
    ipH = ip ; 
  }
  return rc ; 
}

//--------------------------------------------

void ha_set_pfds(ConnInfoRec *cInfo)
{
 #if USE_IB
  if ( cInfo->use_ib )
  {
    cInfo->pfds[0].fd = cInfo->rdInfo.ib_ch->fd ;
    cInfo->pfds[0].events = POLLIN ; 
    cInfo->nfds = 1 ; 
    if ( cInfo->init_here )
    {
      cInfo->pfds[1].fd = cInfo->sfd ; 
      cInfo->pfds[1].events = POLLIN ; 
      cInfo->nfds++ ; 
    }
  }
  else
 #endif
  {
    cInfo->pfds[0].fd = cInfo->sfd ; 
    cInfo->pfds[0].events = POLLIN ; 
    cInfo->nfds = 1 ; 
  }
}
//--------------------------------------------

int ha_raise_event(ConnInfoRec *cInfo, int event_type)
{
  int rc=0 ; 
  haGlobalInfo *gInfo=gInfo_;
  if ( cInfo->channel )
  {
    eventInfo *ev, *evi ; 
    ChannInfo *ch ; 
    ha_set_pfds(cInfo) ; 
    pthread_mutex_lock(gInfo->haLock) ; 
    if ( !gInfo->viewBreak )
    {
      // make sure the channel is still valid
      for ( ch=gInfo->chnHead ; ch && ch != cInfo->channel ; ch=ch->next ) ; // empty body
      if ( ch )
      {
      if ( (ev = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,176),sizeof(eventInfo))) )
      {
        memset(ev,0,sizeof(eventInfo)) ; 
        ev->event_type = event_type ; 
        ch = (ChannInfo *)cInfo->channel ; 
        for ( evi=ch->events ; evi && evi->next ; evi=evi->next ) ; // empty body
        if ( evi )
          evi->next = ev ; 
        else
          ch->events = ev ; 
        pthread_cond_signal(ch->cond) ; 
      }
      else
        rc = -1 ; 
    }
      else
        rc = -1 ; 
    }
    pthread_mutex_unlock(gInfo->haLock) ; 
  }
  return rc ; 
}

//--------------------------------------------

int update_chnn_list(haGlobalInfo *gInfo, ChannInfo *ch, int add)
{
  pthread_mutex_lock(gInfo->haLock) ; 
  gInfo->nchu++ ; 
  if ( add )
  {
    ch->next = gInfo->chnHead ; 
    gInfo->chnHead = ch ; 
    if ( gInfo->viewBreak && ch->cInfo ) ch->cInfo->is_invalid |= C_INVALID ; 
    gInfo->nchs++ ; 
  }
  else
  {
    ChannInfo *chi ; 
    for ( chi=gInfo->chnHead ; chi ; chi=chi->next )
    {
      if ( chi->next == ch )
      {
        chi->next = ch->next ; 
        break ; 
      }
      else
      if ( chi == ch )
      {
        gInfo->chnHead = ch->next ; 
        break ; 
      }
    }
    if ( chi )
    {
      ch->next = NULL ; 
      gInfo->nchs-- ; 
    }
  }
  pthread_mutex_unlock(gInfo->haLock) ; 
  return 0 ; 
}

//--------------------------------------------

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

//--------------------------------------------

int  cip_update_conn_list(haGlobalInfo *gInfo, ConnInfoRec *cInfo, int add)
{
  int rc;
  if ( add )
  {
    rc = update_conn_list(&gInfo->cipInfo->firstConnInProg, cInfo, &gInfo->cipInfo->nConnsInProg, 1) ;  
    if ( gInfo->cipInfo->lfds < gInfo->cipInfo->nConnsInProg )
    {
      int l = gInfo->cipInfo->lfds + 64 ; 
      struct pollfd *new ;
      if ( !(new = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,177),gInfo->cipInfo->fds, l*sizeof(struct pollfd))) )
      {
        TRACE(1," failed to allocate %lu bytes.\n",l*sizeof(struct pollfd));
        return -1 ; 
      }
      gInfo->cipInfo->fds = new ; 
      memset(gInfo->cipInfo->fds+gInfo->cipInfo->lfds, 0, (l-gInfo->cipInfo->lfds)*sizeof(struct pollfd)) ; 
      gInfo->cipInfo->lfds = l ; 
    }
  }
  else
  {
    rc = update_conn_list(&gInfo->cipInfo->firstConnInProg, cInfo, &gInfo->cipInfo->nConnsInProg, 0) ;  
    if ( !rc )
    {
      int i;
      int ind = cInfo->ind ; 
      for ( i=ind ; i<gInfo->cipInfo->nConnsInProg ; i++ )
        gInfo->cipInfo->fds[i] = gInfo->cipInfo->fds[i+1] ; 
    }
  }
  gInfo->cipInfo->nfds = gInfo->cipInfo->nConnsInProg ;
  return rc ; 
}

//--------------------------------------------

int cip_set_socket_buffer_size(int sfd, int buffer_size, int opt)
{
  socklen_t ival ; 

  if (buffer_size > 0)
  {
    ival = buffer_size;
    while (ival > 0)
    {
      if (setsockopt(sfd,SOL_SOCKET,opt,(void *)&ival, sizeof(ival)) < 0 )
        ival = 7*ival/8;
      else break;
    }
    if (ival != buffer_size)
    {
      TRACE(3," Set %s socket buffer size to %d instead of %d\n",(opt==SO_SNDBUF)?"SEND":"RECV",ival,buffer_size);
    }
    if (ival == 0) /* total failure */
      return -1 ;
  }
  return 0 ; 
}

//--------------------------------------------


int  cip_set_low_delay(int sfd, int nodelay, int lwm)
{
  socklen_t ival ;
  int rc = 0 ; 

  if ( nodelay )
  {
    ival=1;
    if (setsockopt(sfd,IPPROTO_TCP,TCP_NODELAY,(void *)&ival, sizeof(ival)) < 0 )
      rc = -1 ;
  }

  if ( lwm )
  {
    ival=lwm;
    if (setsockopt(sfd,SOL_SOCKET,SO_RCVLOWAT,(void *)&ival, sizeof(ival)) < 0 )
      rc = -1 ;
  }

  return rc ; 
}

//--------------------------------------------


int cip_set_local_endpoint(ConnInfoRec *cInfo) 
{
  socklen_t len ;
  SA4 *sa4 ; 
  SA6 *sa6 ; 

  cInfo->lcl_sa = (SA *)&cInfo->lcl_sas ; 
  len = sizeof(SAS) ; 
 #if USE_IB
  if ( cInfo->use_ib ) 
  {
    SA *sa = rdma_get_local_addr(cInfo->cm_id) ; 
    len = sa->sa_family == AF_INET6 ? sizeof(SA6) : sizeof(SA4) ; 
    memcpy(cInfo->lcl_sa,sa,len) ; 
  }
  else
 #endif
  if ( getsockname(cInfo->sfd,cInfo->lcl_sa,&len) == -1 )
  {
    TRACE(1," failed to getsockname, the error code is %d (%s).\n",errno,strerror(errno)) ; 
    return -1 ; 
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
  return 0 ; 
}

/******************************/

int cip_set_remote_endpoint(ConnInfoRec *cInfo) 
{
  socklen_t len ;
  SA4 *sa4 ; 
  SA6 *sa6 ; 

  cInfo->rmt_sa = (SA *)&cInfo->rmt_sas ; 
  len = sizeof(SAS) ; 
 #if USE_IB
  if ( cInfo->use_ib ) 
  {
    SA *sa = rdma_get_peer_addr(cInfo->cm_id) ; 
    len = sa->sa_family == AF_INET6 ? sizeof(SA6) : sizeof(SA4) ; 
    memcpy(cInfo->rmt_sa,sa,len) ; 
  }
  else
 #endif
  if ( getpeername(cInfo->sfd,cInfo->rmt_sa,&len) == -1 )
  {
    TRACE(1," failed to getpeername, the error code is %d (%s).\n",errno,strerror(errno)) ; 
    return -1 ; 
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
  return 0 ; 
}

//--------------------------------------------

int cip_create_discovery_conns(haGlobalInfo *gInfo)
{
  ifInfo *ifi ; 
  SAS sas[1] ; 
  SA  *sa ; 
  SA4 *sa4 ; 
  SA6 *sa6 ; 
  ipFlat *ip, tip[1] ; 
  socklen_t vlen ; 
  int i, sfd, iok, port, nok, ival ; 
  char cval ; 
  socklen_t sa_len ; 
  const char *myName = "storeHA_cip_create_discovery_conns" ;
  errInfo ei[1];
  ei->errLen = sizeof(ei->errMsg) ; 

  sa  = (SA  *)sas ; 
  sa4 = (SA4 *)sas ; 
  sa6 = (SA6 *)sas ; 

  ifi = gInfo->haIf ; 
  port = gInfo->config->Port ; 
  nok = 0 ; 
  for ( i=0 ; i<2 ; i++ )
  {
    gInfo->mcInfo->sfd[i] = -1 ; 
    memset(sas,0,sizeof(SAS)) ; 
    memset(tip,0,sizeof(ipFlat)) ; 
    ip = tip ; 
    iok = 0 ; 
    do
    {
      if ( i )
      {
        if (!IF_HAVE_IPV6(ifi) )
          continue ; 
        if ( haGetAddr(AF_INET6, 0, ismHA_CFG_IPV6_MCAST_ADDR, ip) == AF_UNSPEC )
        {
          TRACE(1," The value of config param ismHA_CFG_IPV6_MCAST_ADDR (%s) is not valid\n",ismHA_CFG_IPV6_MCAST_ADDR) ; 
          break ;
        }
        sa->sa_family = AF_INET6 ; 
        sa6->sin6_port = htons((in_port_t)port) ; 
        memcpy(&sa6->sin6_addr, ip->bytes, ip->length) ; 
        if ( IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr) )
          sa6->sin6_scope_id = ifi->index ; 
        sa_len = sizeof(SA6) ; 
      }
      else
      {
        if (!IF_HAVE_IPV4(ifi) )
          continue ; 
        if ( haGetAddr(AF_INET , 0, ismHA_CFG_IPV4_MCAST_ADDR, ip) == AF_UNSPEC )
        {
          TRACE(1," The value of config param ismHA_CFG_IPV4_MCAST_ADDR (%s) is not valid\n",ismHA_CFG_IPV4_MCAST_ADDR) ; 
          break ;
        }
        sa->sa_family = AF_INET ; 
        sa4->sin_port = htons((in_port_t)port) ; 
        memcpy(&sa4->sin_addr, ip->bytes, ip->length) ; 
        sa_len = sizeof(SA4) ; 
      }
      if ( (sfd=socket_(sa->sa_family, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
      {
        TRACE(1," failed to create socket, rc=%d (%s)\n",errno,strerror(errno)) ; 
        break ; 
      }
      iok++ ; 
      #ifndef IP_MULTICAST_ALL
       #define IP_MULTICAST_ALL 49
      #endif
      ival = 0 ;
      vlen = sizeof(ival) ;
      if ( setsockopt(sfd, IPPROTO_IP, IP_MULTICAST_ALL, (void *)&ival, vlen) < 0 )
      {
        TRACE(4," failed to set socket option to IP_MULTICAST_ALL, rc=%d (%s)\n",errno,strerror(errno)) ; 
      }
      ival = 1 ;
      if ( setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (void *)&ival, vlen) < 0 )
      {
        TRACE(4," failed to set socket option to SO_REUSEADDR, rc=%d (%s)\n",errno,strerror(errno)) ; 
      }
      ival = 0 ;
      vlen = sizeof(ival) ;
      getsockopt(sfd,SOL_SOCKET,SO_SNDBUF,(void *)&ival, &vlen) ; 
      vlen = sizeof(ival) ;
      if ( ival < (1<<16) )
      {
        ival = (1<<16) ; 
        if ( setsockopt(sfd,SOL_SOCKET,SO_SNDBUF,(void *)&ival, vlen) < 0 )
        {
          TRACE(1," failed to set socket option to SO_SNDBUF, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break ; 
        }
      }
      ival = (1<<16) ; 
      if ( setsockopt(sfd,SOL_SOCKET,SO_RCVBUF,(void *)&ival, vlen) < 0 )
      {
        TRACE(1," failed to set socket option to SO_RCVBUF, rc=%d (%s)\n",errno,strerror(errno)) ; 
        break ; 
      }

      if ( i == 0 )
      {
        IA4 * iaddr ;
        struct ip_mreq    mreq ;
        cval = 0 ;
        vlen = sizeof(cval) ;
        if ( setsockopt(sfd, IPPROTO_IP, IP_MULTICAST_LOOP, (void *)&cval, vlen) < 0 )
        {
          TRACE(4," failed to set socket option to IP_MULTICAST_LOOP, rc=%d (%s)\n",errno,strerror(errno)) ; 
        }
        cval = ismHA_CFG_MCAST_TTL ; 
        vlen = sizeof(cval) ;
        if ( setsockopt(sfd, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&cval, vlen) < 0 )
        {
          TRACE(1," failed to set socket option to IP_MULTICAST_IF, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break ; 
        }
        iaddr = &ifi->ipv4_addr;
        vlen = sizeof(IA4) ;
        if (setsockopt(sfd,IPPROTO_IP,IP_MULTICAST_IF,(void *)iaddr, vlen) < 0 )
        {
          TRACE(1," failed to set socket option to IP_MULTICAST_IF, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break ; 
        }
        memset(&mreq, 0, sizeof(mreq));
        memcpy(&mreq.imr_multiaddr,ip->bytes, ip->length) ;
        memcpy(&mreq.imr_interface,iaddr,sizeof(IA4)) ;
        vlen = sizeof(mreq) ;
        if ( setsockopt(sfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, vlen) < 0 )
        {
          TRACE(1," failed to set socket option to IP_ADD_MEMBERSHIP, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break ; 
        }
      }
      else
      {
        struct ipv6_mreq  mreq6;
        ival = 0 ;
        vlen = sizeof(ival) ;
        if ( setsockopt(sfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (void *)&ival, vlen) < 0 )
        {
          TRACE(4," failed to set socket option to IPV6_MULTICAST_LOOP, rc=%d (%s)\n",errno,strerror(errno)) ; 
          //break ; 
        }
        ival = ifi->index ; 
        if (setsockopt(sfd,IPPROTO_IPV6,IPV6_MULTICAST_IF,(void *)&ival,vlen) < 0 )
        {
          TRACE(1," failed to set socket option to IPV6_MULTICAST_IF, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break ; 
        }
        ival = ismHA_CFG_MCAST_TTL ;
        vlen = sizeof(ival) ;
        if ( setsockopt(sfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (void *)&ival, vlen) < 0 )
        {
          TRACE(1," failed to set socket option to IPV6_MULTICAST_HOPS, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break ; 
        }
       #ifdef IPV6_V6ONLY
        ival = 1 ;
        vlen = sizeof(ival) ;
        if ( setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&ival, vlen) < 0 )
        {
          TRACE(1," failed to set socket option to IPV6_V6ONLY, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break ; 
        }
       #endif
        memset(&mreq6, 0, sizeof(mreq6));
        memcpy(&mreq6.ipv6mr_multiaddr,ip->bytes, ip->length) ;
        mreq6.ipv6mr_interface = ifi->index ;
        vlen = sizeof(mreq6) ;
        if ( setsockopt(sfd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (void *)&mreq6, vlen) < 0 )
        {
          TRACE(1," failed to set socket option to IPV6_ADD_MEMBERSHIP, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break ; 
        }
      }
  
      if ( bind(sfd, sa, sa_len) < 0 )
      {
        char addr[ADDR_STR_LEN], pstr[8] ;
        haGetNameInfo(sa,addr,ADDR_STR_LEN,pstr,8,ei) ;
        TRACE(1," failed to bind UDP socket to %s|%s, rc=%d (%s)\n",addr,pstr,errno,strerror(errno)) ; 
        break ; 
      }

      if ( ha_set_nb(sfd, ei, myName) == -1 )
      {
        TRACE(1," failed to set socket to non-blocking, rc=%d (%s)\n",errno,strerror(errno)) ; 
        break ; 
      }

      gInfo->mcInfo->sfd[i] = sfd ; 
      memcpy(gInfo->mcInfo->sas+i, sas, sizeof(SAS)) ; 
      gInfo->mcInfo->sa4 = (SA4 *)&gInfo->mcInfo->sas[0] ; 
      gInfo->mcInfo->sa6 = (SA6 *)&gInfo->mcInfo->sas[1] ; 
      gInfo->mcInfo->fds[gInfo->mcInfo->nfds].fd = sfd ; 
      gInfo->mcInfo->fds[gInfo->mcInfo->nfds].events = POLLIN ; 
      gInfo->mcInfo->nfds++ ; 
        
      iok++ ; 
      nok++ ; 
    } while ( 0 ) ; 
    if ( iok < 2 )
    {
      if ( iok > 0 )
      {
        close(sfd) ; 
      }
    }
  }
  if ( !nok )
  {
    TRACE(1," failed to create udp socket for HA AutoConfig discovery %d.\n",port) ; 
    gInfo->failedNIC = "ha1" ; 
    return -1 ; 
  }
  ifi = gInfo->hbIf ; 
  iok = 0 ; 
  do
  {
    size_t len, ll;
    haConDisMsg *msg ; 
    char s64[ADDR_STR_LEN], *p, *q ; 

    len = (1<<16) ; 
    if (!(gInfo->mcInfo->iMsg = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,178),len)) )
    {
      TRACE(1," failed to allocate memory of size %lu.\n",len) ; 
      break ; 
    }
    iok++ ; 
    memset(gInfo->mcInfo->iMsg, 0, len);
    gInfo->mcInfo->iLen = len ; 
    len = (sizeof(haConDisMsg) + (ifi->ni->naddr * ADDR_STR_LEN) + GROUP_MAX_LEN + 7) & (~7) ; 
    if ( len > (1<<16) )
         len = (1<<16) ;
    if (!(gInfo->mcInfo->oMsg = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,179),len)) )
    {
      TRACE(1," failed to allocate memory of size %lu.\n",len) ; 
      break ; 
    }
    iok++ ; 
    memset(gInfo->mcInfo->oMsg, 0, len);
    msg = gInfo->mcInfo->oMsg ; 
    msg->msg_len  = len ; 
    msg->version  = HA_WIRE_VERSION ; 
    msg->msg_type = HA_MSG_TYPE_DIS_REQ ; 
    msg->id_len = sizeof(ismStore_HANodeID_t) ; 
    memcpy(msg->source_id, gInfo->server_id, msg->id_len) ; 
    p = msg->data ; 
    q = (char *)msg + msg->msg_len ; 
    for ( ip=ifi->ni->addrs ; ip ; ip=ip->next )
    {
      if ( ip->length == sizeof(IA4) )
        inet_ntop(AF_INET, ip->bytes, s64, ADDR_STR_LEN) ; 
      else
      if ( ip->length == sizeof(IA6) && (IF_ONLY_LINK(ifi) || !IN6_IS_ADDR_LINKLOCAL((IA6 *)ip->bytes)) )
      {
        inet_ntop(AF_INET6,ip->bytes, s64, ADDR_STR_LEN) ; 
      }
      else
        continue ; 
      len = q - p ; 
      ll = su_strlcpy(p, s64, len) ;
      if ( ll >= len )
        break ; 
      p += ll ; 
      *p++ = ',' ; 
    }
    *(p-1) = 0 ;
    msg->adr_len = p - msg->data ; 
    gInfo->mcInfo->grp_len = su_strlen(gInfo->config->Group) ; 
    msg->grp_len = gInfo->mcInfo->grp_len ;
    memcpy(p, gInfo->config->Group, msg->grp_len) ; 
    p += msg->grp_len ; 
    msg->msg_len = p - (char *)msg ; 
    iok++ ; 
  } while(0) ; 
  if ( iok < 3 )
  {
    if ( iok > 0 )
      ism_common_free(ism_memory_store_misc,gInfo->mcInfo->iMsg) ; 
    if ( iok > 1 )
      ism_common_free(ism_memory_store_misc,gInfo->mcInfo->oMsg) ; 
    return -1 ; 
  }
  return 0 ;
}

//--------------------------------------------

int cip_handle_discovery_conns(haGlobalInfo *gInfo)
{
  int i, do_ac ; 
  ssize_t nb ; 
  autoConfInfo *acI = gInfo->mcInfo ; 

  do_ac = ( !(gInfo->dInfo->state & DSC_HAVE_PAIR) ) ;
  if ( poll(acI->fds, acI->nfds, 0) > 0 )
  {
    SAS sas[1] ; 
    socklen_t len[1] ; 
    for ( i=0 ; i<acI->nfds && i<2 ; i++ )
    {
      if ( acI->fds[i].revents & POLLIN )
      {
        memset(acI->iMsg, 0, acI->iLen) ;
        nb = recvfrom(acI->fds[i].fd, (void *)acI->iMsg, acI->iLen, 0, (SA *)sas, len) ; 
        if ( nb > 0 && do_ac )
        {
          do
          {
            if ( acI->iMsg->msg_type != HA_MSG_TYPE_DIS_REQ ) break ; 
            if ( acI->iMsg->adr_len + acI->iMsg->grp_len + sizeof(haConDisMsg) < acI->iLen )
              gInfo->lastView->pReasonParam = acI->iMsg->data + acI->iMsg->adr_len ;
            if ( acI->iMsg->msg_len < sizeof(haConDisMsg) + acI->oMsg->grp_len ) break ; 
            if ( acI->iMsg->id_len == acI->oMsg->id_len && !memcmp(acI->iMsg->source_id,acI->oMsg->source_id,acI->iMsg->id_len) ) break ; 
            if ( (acI->iMsg->grp_len != acI->oMsg->grp_len || (acI->oMsg->grp_len && memcmp(acI->iMsg->data+acI->iMsg->adr_len,acI->oMsg->data+acI->oMsg->adr_len,acI->iMsg->grp_len))) ) break ;
            gInfo->config->NodesAddr = acI->iMsg->data ; 
            if ( buildIpList(0, 1) != StoreRC_OK )
            {
              TRACE(1,"cip_handle_discovery_conns: failed to translate the received addresses (%s) into an internal form\n",gInfo->config->NodesAddr);
            }
          } while(0) ; 
        }
      }
    }
  }
  if ( do_ac && su_sysTime() > acI->nextS )
  {
    if ( gInfo->config->gUpd[1] != gInfo->config->gUpd[0] )
    {
      gInfo->config->gUpd[1] = gInfo->config->gUpd[0] ;
      haConDisMsg *msg ; 
      char *p ; 
      msg = acI->oMsg ; 
      p = msg->data + msg->adr_len ; 
      msg->grp_len = gInfo->mcInfo->grp_len ;
      memcpy(p, gInfo->config->Group, msg->grp_len) ; 
      p += msg->grp_len ; 
      msg->msg_len = p - (char *)msg ; 
    }
    int ok=0 ;
    if ( acI->sfd[0] != -1 && (nb = sendto(acI->sfd[0], (void *)acI->oMsg, acI->oMsg->msg_len, 0, (SA *)acI->sa4, sizeof(SA4))) == acI->oMsg->msg_len )
      ok = 1 ; 
    if ( acI->sfd[1] != -1 && (nb = sendto(acI->sfd[1], (void *)acI->oMsg, acI->oMsg->msg_len, 0, (SA *)acI->sa6, sizeof(SA6))) == acI->oMsg->msg_len )
      ok = 1 ; 
    if ( ok )
      acI->nextS = su_sysTime() + ismHA_CFG_MCAST_INTERVAL ;
    else
    {
      TRACE(4,"Failed to send HA discovery message, errno= %d (%s).\n",errno,strerror(errno));
      acI->nextS = su_sysTime() + 1e0 ; 
    }
  }
  return 0 ; 
}

//--------------------------------------------

int cip_create_accept_conns(haGlobalInfo *gInfo)
{
  nicInfo *ni  ; 
  ifInfo *ifi ; 
  ConnInfoRec *cInfo ; 
  SAS sas[1] ; 
  SA  *sa ; 
  SA4 *sa4 ; 
  SA6 *sa6 ; 
  ipFlat *ip, tip[1] ; 
  int i, sfd, iok, port, nok, use_ib, rc ; 
  socklen_t sa_len ; 
  double etime ; 
 #if USE_IB
  int ret ; 
  struct rdma_event_channel *ev_ch=NULL ;
 #endif
  const char *myName = "storeHA_cip_create_accept_conns" ; 
  errInfo ei[1];
  ei->errLen = sizeof(ei->errMsg) ; 

  sa  = (SA  *)sas ; 
  sa4 = (SA4 *)sas ; 
  sa6 = (SA6 *)sas ; 

  for ( i=0 ; i<2 ; i++ )
  {
    ifi = i ? gInfo->haIf : gInfo->hbIf ; 
    ni = ifi->ni ; 
    port = i ? gInfo->config->haPort : gInfo->config->Port ; 
    nok = 0 ; 
    ip=ni->addrs ;
    use_ib = (i && gInfo->use_ib) ; 
   #if USE_IB
    if ( use_ib )
    {
      memset(tip,0,sizeof(ipFlat)) ; 
      sa = rdma_get_local_addr(ifi->cm_id) ; 
      if ( sa->sa_family == AF_INET )
      {
        sa4 = (SA4 *)sa ; 
        ip = tip ; 
        ip->length = sizeof(IA4) ; 
        memcpy(ip->bytes, &sa4->sin_addr, ip->length) ; 
      }
      else
      if ( sa->sa_family == AF_INET6 )
      {
        sa6 = (SA6 *)sa ; 
        ip = tip ; 
        ip->length = sizeof(IA6) ; 
        memcpy(ip->bytes, &sa6->sin6_addr, ip->length) ; 
      }
      else
      {
        TRACE(1," Internal error: cm_id of haIf is not bound properly.\n") ; 
        return -1 ; 
      }
      sa  = (SA  *)sas ; 
      sa4 = (SA4 *)sas ; 
      sa6 = (SA6 *)sas ; 
    }
    else
   #endif
    if ( ifi->tt_addr )
    {
      ip = tip ; 
      memcpy(ip, ifi->tt_addr, sizeof(ipFlat)) ; 
      ip->next = NULL ; 
    }
    else
    {
      if ( haGetAddr(AF_UNSPEC, 0, ifi->addr_name, tip) != AF_UNSPEC )
      {
        ip = tip ; 
        ip->next = NULL ; 
      }
    }
    for ( ; ip ; ip = ip->next )
    {
      memset(sas,0,sizeof(SAS)) ; 
      if ( ip->length == sizeof(IA4) )
      {
        sa->sa_family = AF_INET ; 
        sa4->sin_port = htons((in_port_t)port) ; 
        memcpy(&sa4->sin_addr, ip->bytes, ip->length) ; 
        sa_len = sizeof(SA4) ; 
      }
      else
      if ( ip->length == sizeof(IA6) )
      {
        sa->sa_family = AF_INET6 ; 
        sa6->sin6_port = htons((in_port_t)port) ; 
        memcpy(&sa6->sin6_addr, ip->bytes, ip->length) ; 
        if ( IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr) )
          sa6->sin6_scope_id = ifi->index ; 
        sa_len = sizeof(SA6) ; 
      }
      else
        continue ; 
      do
      {
        iok = 0 ; 
       #if USE_IB
        if ( use_ib )
        {
          if ( !(ev_ch = Xrdma_create_event_channel()) )
          {
            TRACE(1," failed to create an RDMA event channel. The error code is %d (%s).\n",errno,strerror(errno)) ; 
            break ; 
          }
          sfd = ev_ch->fd ; 
        }
        else
       #endif
        if ( (sfd=socket_(sa->sa_family, SOCK_STREAM,IPPROTO_TCP)) == -1 )
        {
          TRACE(1," failed to create socket, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break ; 
        }
        iok++ ; 
        if ( ha_set_nb(sfd, ei, myName) == -1 )
        {
          TRACE(1," failed to set socket to non-blocking, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break ; 
        }
        if (!(use_ib) )
        {
          socklen_t ival = 1 ;
          if ( setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (void *)&ival, sizeof(ival)) < 0 )
          {
            TRACE(1," failed to set socket option to SO_REUSEADDR, rc=%d (%s)\n",errno,strerror(errno)) ; 
            break ; 
          }
        }
    
        if ( (cInfo=(ConnInfoRec *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,182),sizeof(ConnInfoRec))) == NULL )
        {
          TRACE(1," failed to allocate memory of size %lu.\n",sizeof(ConnInfoRec)) ; 
          break ; 
        }
        iok++ ; 
        memset(cInfo,0,sizeof(ConnInfoRec)) ; 
        cInfo->sfd = sfd ; 
        cInfo->use_ib = use_ib ; 
        cInfo->sock_af = sa->sa_family ; 
        cInfo->lcl_sa = (SA  *)&cInfo->lcl_sas ; 
        memcpy(&cInfo->lcl_sas, sas, sizeof(SAS)) ; 
       #if USE_IB
        if ( use_ib )
        {
          if ( (ret = Xrdma_create_id(ev_ch, &cInfo->cm_id, cInfo, RDMA_PS_TCP)) )
          {
            TRACE(1," failed to create the RDMA ID. The error code is %d.",ret);
            break ;
          }
        }
       #endif
        etime = su_sysTime() + gInfo->haTimeOut[1]/2 ; 
        do
        {
         #if USE_IB
          if ( use_ib )
            rc = Xrdma_bind_addr(cInfo->cm_id, cInfo->lcl_sa) ; 
          else
         #endif
          rc = bind(cInfo->sfd, cInfo->lcl_sa, sa_len) ; 
          if ( rc == 0 )
            break ; 
          rc = errno ; 
          if ( rc != EADDRINUSE )
            break ; 
          if ( gInfo->goDown )
            break ; 
          TRACE(5," failed to bind socket to port %d (EADDRINUSE), will retry in 1 second and will stop bind attempt after 16 seconds.\n",port) ; 
          su_sleep(1,SU_SEC) ;
        } while ( su_sysTime() < etime ) ;
        if ( rc != 0 )
        {
          char addr[ADDR_STR_LEN], pstr[8] ;
          haGetNameInfo(cInfo->lcl_sa,addr,ADDR_STR_LEN,pstr,8,ei) ;
          TRACE(1," failed to bind socket to %s|%s, rc=%d (%s)\n",addr,pstr,errno,strerror(errno)) ; 
          gInfo->failedNIC = i ? "ha1" : "ha0" ; 
          break ; 
        }
        if ( !port )
        {
         #if USE_IB
          if ( use_ib )
          {
            sa = rdma_get_local_addr(cInfo->cm_id) ; 
          }
          else
         #endif
          {
            if ( getsockname(sfd, sa,&sa_len) < 0 )
            {
              rc = errno ; 
              TRACE(1," failed to getsockname of bound socket, errno=%d (%s).\n",rc,strerror(rc)) ; 
              break ; 
            }
          }
          memcpy(&cInfo->lcl_sas, sa, sa_len) ; 
          port = (sa->sa_family == AF_INET) ? sa4->sin_port : sa6->sin6_port ; 
          port = ntohs(port) ; 
        }
       #if USE_IB
        if ( use_ib )
        {
          if ( (ret = Xrdma_listen(cInfo->cm_id, 64)) )
          {
            TRACE(1," failed to invoke the rdma_listen API. The error code is %d.",ret);
            break ; 
          }
        }
        else
       #endif
        {
          if ( ifi == gInfo->haIf )
          {
            rc = gInfo->SockBuffSizes[0][0] ;
            if ( cip_set_socket_buffer_size(cInfo->sfd, rc, SO_SNDBUF) == -1 )
            {
              TRACE(1," failed to set the accept socket send buffer size to %d.\n",rc);
            }
            else
            {
              TRACE(5," Accept send socket buffer size set to %d.\n",rc);
            }
            rc = gInfo->SockBuffSizes[0][1] ;
            if ( cip_set_socket_buffer_size(cInfo->sfd, rc, SO_RCVBUF) == -1 )
            {
              TRACE(1," failed to set the accept sokcet receive buffer size to %d.\n",rc);
            }
            else
            {
              TRACE(5," Accept receive socket buffer size set to %d.\n",rc);
            }
          }
          if ( cip_set_low_delay(cInfo->sfd, 1, 0) == -1 )
          {
            TRACE(5," failed to set the socket TCP_NODELAY to 1. The reason code is %d.\n",errno);
            /* break ; we can leave without it */
          }
          if ( cip_set_low_delay(cInfo->sfd, 0, 1) == -1 )
          {
            TRACE(5," failed to set the socket SO_RCVLOWAT to 1. The reason code is %d.\n",errno);
            /* break ; we can leave without it */
          }

          if ( listen(cInfo->sfd, 64) == -1)
          {
            rc = errno ; 
            TRACE(1," failed to listen on socket, errno=%d (%s).\n",rc,strerror(rc)) ; 
            break ; 
          }
        }
        cip_update_conn_list(gInfo, cInfo, 1) ;  
        DBG_PRT(printf(" cip_create_accept_conns: con added for port %d, gInfo->cipInfo->nConnsInProg=%d\n",port,gInfo->cipInfo->nConnsInProg)) ; 
    
        memcpy(&cInfo->lcl_addr, ip, sizeof(ipFlat)) ; 
        cInfo->lcl_port = port ;
        cInfo->io_state = (use_ib) ? CIP_IB_STATE_CREQ : CIP_IO_STATE_ACC ;
        cInfo->state    = CIP_STATE_LISTEN ; 
        cInfo->use_ib   = use_ib ; 
        cInfo->is_ha    = i ; 
        gInfo->cipInfo->fds[cInfo->ind].fd = cInfo->sfd ; 
        gInfo->cipInfo->fds[cInfo->ind].events = POLLIN ; 
        inet_ntop(sa->sa_family,cInfo->lcl_addr.bytes,cInfo->req_addr,HOSTNAME_STR_LEN) ;
        cInfo->req_port = cInfo->lcl_port ; 
        cInfo->conn_id = ++gInfo->conn_id ; 
        TRACE(5, " Listening on endpoint: %s|%u for %s NIC\n",cInfo->req_addr,cInfo->req_port,i?"Replication":"Discovery") ; 
        iok = 4 ; 
        nok++ ; 
      } while ( 0 ) ; 
      if ( iok < 4 )
      {
        if ( iok > 1 )
        {
         #if USE_IB
          if ( use_ib && cInfo->cm_id )
            Xrdma_destroy_id(cInfo->cm_id) ; 
         #endif
          ism_common_free(ism_memory_store_misc,cInfo) ; 
        }
        if ( iok > 0 )
        {
         #if USE_IB
          if ( use_ib )
            Xrdma_destroy_event_channel(ev_ch) ; 
          else
         #endif
          close(sfd) ; 
        }
      }
      else
      if ( i && nok )
      {
        gInfo->dInfo->cIha = cInfo ; 
        IP_INIT(gInfo->ipLcl) ; 
        gInfo->ipLcl->ai.ai_family = cInfo->sock_af ; 
        gInfo->ipLcl->ai.ai_addrlen = sa_len ; 
        memcpy(gInfo->ipLcl->ai.ai_addr,cInfo->lcl_sa, sa_len) ; 

        IP_INIT(gInfo->ipExt) ; 
        if ( gInfo->config->ExtReplicationNIC && gInfo->config->ExtReplicationNIC[0] )
        {
          int dns=1;
          ip = tip ; 
          haGetAddr(AF_UNSPEC, &dns, gInfo->config->ExtReplicationNIC, ip) ; 
          memset(sas,0,sizeof(SAS)) ; 
          if ( gInfo->config->ExtHaPort ) port = gInfo->config->ExtHaPort ; 
          if ( ip->length == sizeof(IA4) )
          {
            sa->sa_family = AF_INET ; 
            sa4->sin_port = htons((in_port_t)port) ; 
            memcpy(&sa4->sin_addr, ip->bytes, ip->length) ; 
            sa_len = sizeof(SA4) ; 
          }
          else
          {
            sa->sa_family = AF_INET6 ; 
            sa6->sin6_port = htons((in_port_t)port) ; 
            memcpy(&sa6->sin6_addr, ip->bytes, ip->length) ; 
            if ( IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr) )
              sa6->sin6_scope_id = ifi->index ; 
            sa_len = sizeof(SA6) ; 
          }

          gInfo->ipExt->ai.ai_family = sa->sa_family ; 
          gInfo->ipExt->ai.ai_addrlen = sa_len ; 
          memcpy(gInfo->ipExt->ai.ai_addr,sa,sa_len) ; 
        }
        else
        {
          gInfo->ipExt->ai.ai_family = cInfo->sock_af ; 
          gInfo->ipExt->ai.ai_addrlen = sa_len ; 
          memcpy(gInfo->ipExt->ai.ai_addr,cInfo->lcl_sa, sa_len) ; 
          if ( gInfo->config->ExtHaPort && gInfo->config->ExtHaPort != port )
          {
            port = gInfo->config->ExtHaPort ;
            if ( cInfo->sock_af == AF_INET )
            {
              IP_SA4(gInfo->ipExt)->sin_port = htons((in_port_t)port) ;
            }
            else
            {
              IP_SA6(gInfo->ipExt)->sin6_port = htons((in_port_t)port) ;
            }
          }
        }
        break ; 
      }
    }
    if ( !nok )
    {
      TRACE(1," failed to create listen socket for HA mgmt connection on port %d.\n",port) ; 
      return -1 ; 
    }
  }
  return 0 ; 
}

  
//--------------------------------------------
static int cip_cmp_sid(ismStore_HASessionID_t s1, uint32_t c1, ismStore_HASessionID_t s2, uint32_t c2)
{
  ismStore_HASessionID_t zid;
  memset(zid,0,sizeof(ismStore_HASessionID_t)); 
  if ( !memcmp(s1,zid,sizeof(ismStore_HASessionID_t)) ||
       !memcmp(s2,zid,sizeof(ismStore_HASessionID_t)) ||
        memcmp(s1,s2,sizeof(ismStore_HASessionID_t)) ||
        c1 == c2 )
    return 0 ; 
  return ( (c1 - c2) & (1<<15) ) ? -1 : 1 ; 
}
//--------------------------------------------
static void cip_get_req_extra_params(haConReqMsg *msg, haReqFlags *f)
{
  char *p0, *p1 ; 
  p0 = msg->data + msg->grp_len + 1 ; 
  p1 = (char *)msg + msg->msg_len + INT_SIZE ; 
  if ( p1 >= p0 + INT_SIZE )
  {
    memcpy(&f->SplitBrainPolicy, p0, INT_SIZE) ; 
    p0 += INT_SIZE ; 
  }
  if ( p1 >= p0 + INT_SIZE )
  {
    memcpy(&f->numActiveConns, p0, INT_SIZE) ; 
    p0 += INT_SIZE ; 
  }
}
//--------------------------------------------

int cip_check_req_msgs(haGlobalInfo *gInfo, int *role)
{
  int rc, dif , lrole, rrole , line;
  haConReqMsg *lmsg = &gInfo->dInfo->req_msg[0] ; 
  haConReqMsg *rmsg = &gInfo->dInfo->req_msg[1] ; 
  haReqFlags L={0}, R={0} ; 

  L.is_primary     = lmsg->flags & HA_MSG_FLAG_IS_PRIM ; 
  L.is_standalone  = lmsg->flags & HA_MSG_FLAG_STANDALONE ; 
  L.can_be_primary = lmsg->flags & HA_MSG_FLAG_CAN_BE_PRIM ; 
  L.have_data      = lmsg->flags & HA_MSG_FLAG_HAVE_DATA ; 
  L.was_primary    = lmsg->flags & HA_MSG_FLAG_WAS_PRIM ; 
  L.pref_prim      = lmsg->flags & HA_MSG_FLAG_PREF_PRIM ; 
  L.use_comp       = lmsg->flags & HA_MSG_FLAG_USE_COMPACT ; 
  L.no_resync      = lmsg->flags & HA_MSG_FLAG_NO_RESYNC ; 
  L.is_cluster     = lmsg->flags & HA_MSG_FLAG_IS_CLUSTER; 

  R.is_primary     = rmsg->flags & HA_MSG_FLAG_IS_PRIM ; 
  R.is_standalone  = rmsg->flags & HA_MSG_FLAG_STANDALONE ; 
  R.can_be_primary = rmsg->flags & HA_MSG_FLAG_CAN_BE_PRIM ; 
  R.have_data      = rmsg->flags & HA_MSG_FLAG_HAVE_DATA ; 
  R.was_primary    = rmsg->flags & HA_MSG_FLAG_WAS_PRIM ; 
  R.pref_prim      = rmsg->flags & HA_MSG_FLAG_PREF_PRIM ; 
  R.use_comp       = rmsg->flags & HA_MSG_FLAG_USE_COMPACT ; 
  R.no_resync      = rmsg->flags & HA_MSG_FLAG_NO_RESYNC ; 
  R.is_cluster     = rmsg->flags & HA_MSG_FLAG_IS_CLUSTER; 

  dif = memcmp(lmsg->source_id, rmsg->source_id, sizeof(ismStore_HANodeID_t)) ; 

  if ( !dif                                                    ||
    // lmsg->msg_len                   != rmsg->msg_len        ||
    // lmsg->version                   != rmsg->version        ||
    // lmsg->store_version             != rmsg->store_version  ||
       lmsg->msg_type                  != rmsg->msg_type       ) 
  {
    rc = ISM_HA_REASON_SYSTEM_ERROR ; 
    line = __LINE__;
  }
  else
  if ( lmsg->TotalMemSizeMB            != rmsg->TotalMemSizeMB            ||
       lmsg->MgmtSmallGranuleSizeBytes != rmsg->MgmtSmallGranuleSizeBytes ||
       lmsg->MgmtGranuleSizeBytes      != rmsg->MgmtGranuleSizeBytes      ||
       lmsg->GranuleSizeBytes          != rmsg->GranuleSizeBytes          ||
       lmsg->MgmtMemPercent            != rmsg->MgmtMemPercent            ||
       lmsg->InMemGensCount            != rmsg->InMemGensCount            ||
       lmsg->ReplicationProtocol       != rmsg->ReplicationProtocol       ||
       L.is_cluster                    != R.is_cluster                    ) 
  {
    rc = ISM_HA_REASON_CONFIG_ERROR ;
    line = __LINE__;
  }
  else
  if ( rmsg->msg_len > offsetof(haConReqMsg,grp_len) && 
      (lmsg->grp_len != rmsg->grp_len || (lmsg->grp_len && memcmp(lmsg->data,rmsg->data,lmsg->grp_len))) )
  {
    TRACE(1,"haChoosePrimary: Different Group names detected (local %s vs remote %s).\n",lmsg->data,rmsg->data);
    rc = ISM_HA_REASON_DIFF_GROUP ; 
    line = __LINE__;
  } 
  else
    rc = ISM_HA_REASON_OK ;

  do
  {
    // both active primary --> split brain
    if ( L.is_primary && R.is_primary )  
    {
      lrole = rrole = ISM_HA_ROLE_ERROR ; 
      rc = ISM_HA_REASON_SPLIT_BRAIN ; 
      line = __LINE__;
      if ( ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE )
      {
        int d=0;
        cip_get_req_extra_params(lmsg,&L);
        cip_get_req_extra_params(rmsg,&R);
        if ( L.SplitBrainPolicy == 1 && R.SplitBrainPolicy == 1 )
        {
          rc = ISM_HA_REASON_SPLIT_BRAIN_RESTART ; 
          d = R.numActiveConns - L.numActiveConns ; 
          if ( d < 0 )
          {
            lrole = ISM_HA_ROLE_PRIMARY ; 
            rrole = ISM_HA_ROLE_ERROR ; 
            line = __LINE__;
            snprintf(gInfo->dInfo->reasonParm,sizeof(gInfo->dInfo->reasonParm),"Two Primaries: numConns=(local %d , remote %d) ; restarting remote",
                     L.numActiveConns,R.numActiveConns) ; 
          }
          else
          if ( d > 0 )
          {
            rrole = ISM_HA_ROLE_PRIMARY ; 
            lrole = ISM_HA_ROLE_ERROR ; 
            line = __LINE__;
            snprintf(gInfo->dInfo->reasonParm,sizeof(gInfo->dInfo->reasonParm),"Two Primaries: numConns=(local %d , remote %d) ; restarting local",
                     L.numActiveConns,R.numActiveConns) ; 
          }
          else
          {
            d = cip_cmp_sid(lmsg->session_id, lmsg->session_count, rmsg->session_id, rmsg->session_count) ; 
            if ( d < 0 )
            {
              lrole = ISM_HA_ROLE_PRIMARY ; 
              rrole = ISM_HA_ROLE_ERROR ; 
              line = __LINE__;
              snprintf(gInfo->dInfo->reasonParm,sizeof(gInfo->dInfo->reasonParm),"Two Primaries: numConns=(local %d , remote %d) ; session_count=(local %d, remote %d) restarting remote",
                       L.numActiveConns,R.numActiveConns,lmsg->session_count,rmsg->session_count);
            }
            else
            if ( d > 0 )
            {
              rrole = ISM_HA_ROLE_PRIMARY ; 
              lrole = ISM_HA_ROLE_ERROR ; 
              line = __LINE__;
              snprintf(gInfo->dInfo->reasonParm,sizeof(gInfo->dInfo->reasonParm),"Two Primaries: numConns=(local %d , remote %d) ; session_count=(local %d, remote %d) restarting local",
                       L.numActiveConns,R.numActiveConns,lmsg->session_count,rmsg->session_count);
            }
            else
            {
              char lsid[1+2*sizeof(ismStore_HASessionID_t)] ; 
              char rsid[1+2*sizeof(ismStore_HASessionID_t)] ; 
              b2h(lsid,lmsg->session_id,sizeof(ismStore_HASessionID_t));
              b2h(rsid,rmsg->session_id,sizeof(ismStore_HASessionID_t));
              TRACE(1,"!!! Different sessionId, or, equal sessionCount !!! local: %s | %u , remote: %s | %u\n",lsid,lmsg->session_count,rsid,rmsg->session_count);
              lrole = rrole = ISM_HA_ROLE_ERROR ; 
              line = __LINE__;
              rc = ISM_HA_REASON_SPLIT_BRAIN ; 
            }
          }
        }
      }
      else
      {
        rrole = ISM_HA_ROLE_PRIMARY ;
        lrole = ISM_HA_ROLE_ERROR ; 
        line = __LINE__;
        gInfo->dInfo->state |= DSC_SB_TERM ; 
        if ( gInfo->config->SplitBrainPolicy == 1 )
          rc = ISM_HA_REASON_SPLIT_BRAIN_RESTART ; 
      }
      break ; 
    }
    
    // Local already primary && Remote new --> If Remote is valid it is set to Standby otherwise Error
    // note that unlike before bringing up a node in standalone will not take the primary down if the problem is detected on time (i.e., before the standalone becomes is_primary) 
    if ( L.is_primary && !R.is_primary )  
    {
      lrole = ISM_HA_ROLE_PRIMARY ;
      if ( rc != ISM_HA_REASON_OK || R.is_standalone || (!R.use_comp) || R.no_resync || lmsg->version > rmsg->version )
      {
        rrole = ISM_HA_ROLE_ERROR ; 
        line = __LINE__;
        if ( R.is_standalone )
          rc = ISM_HA_REASON_SPLIT_BRAIN ; 
        else
        if ( !R.use_comp || lmsg->version > rmsg->version )
          rc = ISM_HA_REASON_CONFIG_ERROR ; 
        else
        if ( R.no_resync )
          rc = ISM_HA_REASON_NO_RESYNC ; 
      }
      else
      {
        rrole = ISM_HA_ROLE_STANDBY ; 
        line = __LINE__;
      }
      break ; 
    }
    
    // like above with reverse roles (two sections can be combined)
    if ( !L.is_primary && R.is_primary )  
    {
      rrole = ISM_HA_ROLE_PRIMARY ;
      if ( rc != ISM_HA_REASON_OK || L.is_standalone || L.no_resync )
      {
        lrole = ISM_HA_ROLE_ERROR ; 
        line = __LINE__;
        if ( L.is_standalone )
          rc = ISM_HA_REASON_SPLIT_BRAIN ; 
        if ( L.no_resync )
          rc = ISM_HA_REASON_NO_RESYNC ; 
      }
      else
      {
        lrole = ISM_HA_ROLE_STANDBY ; 
        line = __LINE__;
      }
      break ; 
    }
    
    // If we get here it means both nodes are new
    //first check for obvious errors
    if ( (L.is_standalone || R.is_standalone) || rc != ISM_HA_REASON_OK || (!L.can_be_primary && !R.can_be_primary) || (L.use_comp != R.use_comp) )
    {
      lrole = rrole = ISM_HA_ROLE_ERROR ; 
      line = __LINE__;
      if ( L.is_standalone || R.is_standalone )
        rc = ISM_HA_REASON_SPLIT_BRAIN ; 
      if ( !L.can_be_primary && !R.can_be_primary )
        rc = ISM_HA_REASON_SYNC_ERROR ; 
      if ( L.use_comp != R.use_comp )
        rc = ISM_HA_REASON_CONFIG_ERROR ;
      break ;
    }
    
    // see if one of the nodes cannot act as primary
    if ( L.can_be_primary != R.can_be_primary )  // cannot be both
    {
      lrole = L.can_be_primary ? ISM_HA_ROLE_PRIMARY : ISM_HA_ROLE_STANDBY ; 
      rrole = R.can_be_primary ? ISM_HA_ROLE_PRIMARY : ISM_HA_ROLE_STANDBY ; 
      line = __LINE__;
      break ;
    }   

    // New check for two non-empty Stores on startup
    if ( L.have_data && R.have_data && 
         (memcmp(lmsg->shutdown_token,rmsg->shutdown_token,sizeof(ismStore_memGenToken_t)) || lmsg->shutdown_token->Timestamp == 0)  )
    {
      // if we cannot prefer one based on was_primay we set both to error 
      if ( L.was_primary == R.was_primary || (!L.was_primary && L.pref_prim) || (!R.was_primary && R.pref_prim) )
      {
        int d = cip_cmp_sid(lmsg->session_id, lmsg->session_count, rmsg->session_id, rmsg->session_count) ; 
        if ( d < 0 )
        {
          line = __LINE__;
          rrole = ISM_HA_ROLE_PRIMARY ; 
          lrole = ISM_HA_ROLE_STANDBY ; 
        }
        else
        if ( d > 0 )
        {
          line = __LINE__;
          lrole = ISM_HA_ROLE_PRIMARY ; 
          rrole = ISM_HA_ROLE_STANDBY ; 
        }
        else
        {
          line = __LINE__;
          lrole = rrole = ISM_HA_ROLE_ERROR ; 
          rc = ISM_HA_REASON_UNRESOLVED ; //new reason for two non-empty stores
        }
        break ; 
      }
      else
      {
        // choose the one with was_primary as primary other to standby 
        lrole = L.was_primary ? ISM_HA_ROLE_PRIMARY : ISM_HA_ROLE_STANDBY ;
        rrole = R.was_primary ? ISM_HA_ROLE_PRIMARY : ISM_HA_ROLE_STANDBY ;
        line = __LINE__;
      }
      break ;
    }   
    
    // at this point all error conditions have been checked so one node will be Primary and the other Standby 
    if ( L.have_data != R.have_data )  // prefer node with have_data
    {
      lrole = L.have_data ? ISM_HA_ROLE_PRIMARY : ISM_HA_ROLE_STANDBY ;
      rrole = R.have_data ? ISM_HA_ROLE_PRIMARY : ISM_HA_ROLE_STANDBY ;
      line = __LINE__;
      break ;
    }
    
    // select based on user hint
    if ( L.pref_prim != R.pref_prim )  // prefer node with pref_prim
    {
      lrole = L.pref_prim ? ISM_HA_ROLE_PRIMARY : ISM_HA_ROLE_STANDBY ;
      rrole = R.pref_prim ? ISM_HA_ROLE_PRIMARY : ISM_HA_ROLE_STANDBY ;
      line = __LINE__;
      break ;
    }
    
    // no preference, select based on node_id
    lrole = dif > 0 ? ISM_HA_ROLE_PRIMARY : ISM_HA_ROLE_STANDBY ;
    rrole = dif < 0 ? ISM_HA_ROLE_PRIMARY : ISM_HA_ROLE_STANDBY ;
    line = __LINE__;
    break ;
  } while(0) ; 

  gInfo->lastView->pReasonParam = NULL ; 
  if ( lmsg->version > rmsg->version && lrole == ISM_HA_ROLE_PRIMARY && !L.is_primary )
  {
    lrole = rrole = ISM_HA_ROLE_ERROR ; 
    line = __LINE__;
    rc = ISM_HA_REASON_CONFIG_ERROR ;
    gInfo->lastView->pReasonParam = "Server version conflict" ; 
  }

  role[0] = lrole ; 
  role[1] = rrole ; 

  gInfo->lastView->ReasonCode = rc ; 
  if ( rc != ISM_HA_REASON_OK )
  {
    if ( !dif )
    {
      TRACE(1,"haChoosePrimary: peer rejected: same server id.\n") ; 
    }
    if (
     //  lmsg->msg_len                   != rmsg->msg_len        ||
     //  lmsg->version                   != rmsg->version        ||
     //  lmsg->store_version             != rmsg->store_version  ||
         L.use_comp                      != R.use_comp           ||
         lmsg->version                    > rmsg->version        ||
         lmsg->msg_type                  != rmsg->msg_type ) 
    {
      TRACE(1,"haChoosePrimary: peer rejected: not same server version or internal error.\n") ; 
    }
    if ( rc == ISM_HA_REASON_SPLIT_BRAIN )
    {
      if ( gInfo->config->StartupMode == CIP_STARTUP_STAND_ALONE )
      {
        TRACE(1,"haChoosePrimary: peer rejected: attempting to start in StandAlone mode.\n");
      }
      else
      {
        TRACE(1,"haChoosePrimary: peer rejected: both already primary.\n");
      }
    }
    if ( L.is_primary && L.use_comp && !R.use_comp )
    {
      TRACE(1,"haChoosePrimary: peer rejected: incompatible server version: primary use compaction while peer can not.\n");
    }
    if ( lmsg->TotalMemSizeMB            != rmsg->TotalMemSizeMB )
    {
      TRACE(1,"haChoosePrimary: peer rejected: incompatible %s value (%u vs %u).\n",ismSTORE_CFG_TOTAL_MEMSIZE_MB,lmsg->TotalMemSizeMB,rmsg->TotalMemSizeMB);
      gInfo->lastView->pReasonParam = ismSTORE_CFG_TOTAL_MEMSIZE_MB ; 
      if ( !L.is_primary && rrole == ISM_HA_ROLE_PRIMARY && !L.have_data && !ismStore_global.fRestarting &&
           ismStore_global.MemType != ismSTORE_MEMTYPE_NVRAM && rmsg->TotalMemSizeMB <= (ismStore_global.MachineMemoryBytes>>22) )
      {
        TRACE(1,"haChoosePrimary: setting PrimaryMemSizeBytes to %lu (%lu) on standby node before restarting store.\n",(((uint64_t)rmsg->TotalMemSizeMB)<<20), ismStore_global.PrimaryMemSizeBytes);
        ismStore_global.PrimaryMemSizeBytes = ((uint64_t)rmsg->TotalMemSizeMB)<<20 ;
      }
      else if (!L.is_primary && !R.is_primary && !L.have_data && !R.have_data && !ismStore_global.fRestarting )
      {
        // support for two clean nodes started at the same time: restart both nodes with the store size set to the smaller of the two
        ismStore_global.PrimaryMemSizeBytes = (lmsg->TotalMemSizeMB < rmsg->TotalMemSizeMB ? lmsg->TotalMemSizeMB : rmsg->TotalMemSizeMB) ; 
        ismStore_global.PrimaryMemSizeBytes <<= 20 ; 
        TRACE(1,"haChoosePrimary: setting PrimaryMemSizeBytes to %lu (%luMB) on both nodes before restarting store.\n",ismStore_global.PrimaryMemSizeBytes, (ismStore_global.PrimaryMemSizeBytes>>20));
      }        
    }
    if ( lmsg->MgmtSmallGranuleSizeBytes != rmsg->MgmtSmallGranuleSizeBytes )
    {
      TRACE(1,"haChoosePrimary: peer rejected: incompatible %s value (%u vs %u).\n",ismSTORE_CFG_MGMT_SMALL_GRANULE,lmsg->MgmtSmallGranuleSizeBytes,rmsg->MgmtSmallGranuleSizeBytes);
      gInfo->lastView->pReasonParam = ismSTORE_CFG_MGMT_SMALL_GRANULE ; 
    }
    if ( lmsg->MgmtGranuleSizeBytes      != rmsg->MgmtGranuleSizeBytes )
    {
      TRACE(1,"haChoosePrimary: peer rejected: incompatible %s value (%u vs %u).\n",ismSTORE_CFG_MGMT_GRANULE_SIZE,lmsg->MgmtGranuleSizeBytes,rmsg->MgmtGranuleSizeBytes);
      gInfo->lastView->pReasonParam = ismSTORE_CFG_MGMT_GRANULE_SIZE ; 
    }
    if ( lmsg->GranuleSizeBytes          != rmsg->GranuleSizeBytes )
    {
      TRACE(1,"haChoosePrimary: peer rejected: incompatible %s value (%u vs %u).\n",ismSTORE_CFG_GRANULE_SIZE,lmsg->GranuleSizeBytes,rmsg->GranuleSizeBytes);
      gInfo->lastView->pReasonParam = ismSTORE_CFG_GRANULE_SIZE ; 
    }
    if ( lmsg->MgmtMemPercent            != rmsg->MgmtMemPercent )
    {
      TRACE(1,"haChoosePrimary: peer rejected: incompatible %s value (%u vs %u).\n",ismSTORE_CFG_MGMT_MEM_PCT,lmsg->MgmtMemPercent,rmsg->MgmtMemPercent);
      gInfo->lastView->pReasonParam = ismSTORE_CFG_MGMT_MEM_PCT ; 
    }
    if ( lmsg->InMemGensCount            != rmsg->InMemGensCount )
    {
      TRACE(1,"haChoosePrimary: peer rejected: incompatible %s value (%u vs %u).\n",ismSTORE_CFG_INMEM_GENS_COUNT,lmsg->InMemGensCount,rmsg->InMemGensCount);
      gInfo->lastView->pReasonParam = ismSTORE_CFG_INMEM_GENS_COUNT ; 
    }
    if ( lmsg->ReplicationProtocol       != rmsg->ReplicationProtocol )
    {
      TRACE(1,"haChoosePrimary: peer rejected: incompatible %s value (%u vs %u).\n",ismHA_CFG_REPLICATIONPROTOCOL,lmsg->ReplicationProtocol,rmsg->ReplicationProtocol);
      gInfo->lastView->pReasonParam = ismHA_CFG_REPLICATIONPROTOCOL ; 
    }
    if ( L.is_cluster                    != R.is_cluster              ) 
    {
      TRACE(1,"haChoosePrimary: peer rejected: incompatible %s value (%u vs %u).\n",ismCLUSTER_CFG_ENABLECLUSTER,L.is_cluster/HA_MSG_FLAG_IS_CLUSTER,R.is_cluster/HA_MSG_FLAG_IS_CLUSTER);
      gInfo->lastView->pReasonParam = ismCLUSTER_CFG_ENABLECLUSTER ; 
    }
    if ( rc == ISM_HA_REASON_DIFF_GROUP )
    {
      TRACE(1,"haChoosePrimary: peer rejected: incompatible %s value (%s vs %s).\n","HAGroup",lmsg->data,rmsg->data);
      gInfo->lastView->pReasonParam = rmsg->data ; 
    }
    if ( rc == ISM_HA_REASON_SPLIT_BRAIN_RESTART )
    {
      TRACE(1,"haChoosePrimary: %s\n",gInfo->dInfo->reasonParm);
      gInfo->lastView->pReasonParam = gInfo->dInfo->reasonParm ; 
    }
  }

  TRACE(1,"haChoosePrimary: lrole=%d, rrole=%d, line=%d\n",lrole,rrole,line);
  return rc ; 
}

//--------------------------------------------

int cip_check_res_msgs(haGlobalInfo *gInfo, int *role)
{
  int rc ; //, dif ; 
  haConResMsg *lmsg = &gInfo->dInfo->res_msg[0] ; 
  haConResMsg *rmsg = &gInfo->dInfo->res_msg[1] ; 

  // dif = memcmp(lmsg->source_id, rmsg->source_id, sizeof(ismStore_HANodeID_t)) ; 

  if ( lmsg->msg_len                   != rmsg->msg_len  ||
    // lmsg->version                   != rmsg->version  ||
       lmsg->msg_type                  != rmsg->msg_type ||
       lmsg->conn_rejected             != rmsg->conn_rejected ||
       lmsg->reject_reason             != rmsg->reject_reason ||
       lmsg->role_local                != rmsg->role_remote   ||
       lmsg->role_remote               != rmsg->role_local    ||
       memcmp(lmsg->source_id, rmsg->destination_id, sizeof(ismStore_HANodeID_t)) ||
       memcmp(lmsg->destination_id, rmsg->source_id, sizeof(ismStore_HANodeID_t))  )
  {
    if ((!lmsg->conn_rejected) && rmsg->conn_rejected )
    {
      rc = ISM_HA_REASON_CONFIG_ERROR ;
      gInfo->lastView->pReasonParam = "Configuration mismatch or server version conflict" ; 
    }
    else
    rc = ISM_HA_REASON_SYSTEM_ERROR ; 
    role[0] = gInfo->dInfo->req_msg[0].flags&HA_MSG_FLAG_IS_PRIM ? lmsg->role_local : ISM_HA_ROLE_ERROR  ;
    role[1] = gInfo->dInfo->req_msg[1].flags&HA_MSG_FLAG_IS_PRIM ? rmsg->role_local : ISM_HA_ROLE_ERROR  ;
  }
  else
  {
    rc = lmsg->reject_reason ; 
    role[0] = lmsg->role_local ; 
    role[1] = lmsg->role_remote ; 
  }

  IP_INIT(gInfo->ipRmt) ; 
  if ( rmsg->ha_addr_len == sizeof(IA4) )
  {
    SA4 *sa4 = IP_SA4(gInfo->ipRmt) ; 
    sa4->sin_family = gInfo->ipRmt->ai.ai_family = AF_INET ; 
    sa4->sin_port = rmsg->ha_port ; 
    memcpy(&sa4->sin_addr, rmsg->ha_nic, sizeof(IA4)) ; 
    gInfo->ipRmt->ai.ai_addrlen = sizeof(SA4) ; 
  }
  else
  {
    SA6 *sa6 = IP_SA6(gInfo->ipRmt) ; 
    sa6->sin6_family = gInfo->ipRmt->ai.ai_family = AF_INET6 ; 
    sa6->sin6_port = rmsg->ha_port ; 
    memcpy(&sa6->sin6_addr, rmsg->ha_nic, sizeof(IA6)) ; 
    gInfo->ipRmt->ai.ai_addrlen = sizeof(SA6) ; 
  }

  return rc ; 
}

//--------------------------------------------
static int cip_can_be_prim(haGlobalInfo *gInfo)
{
  ismStore_memMgmtHeader_t *pMgmHeader;
  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  return (gInfo->viewCount || pMgmHeader->Role==ismSTORE_ROLE_PRIMARY || (gInfo->config->RoleValidation==0 && pMgmHeader->Role==ismSTORE_ROLE_STANDBY)) ; 
}
static int cip_was_prim(haGlobalInfo *gInfo)
{
  ismStore_memMgmtHeader_t *pMgmHeader;
  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  return pMgmHeader->WasPrimary ; 
}
static int cip_store_have_data(haGlobalInfo *gInfo)
{
  ismStore_memMgmtHeader_t *pMgmHeader;
  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  return (gInfo->viewCount || pMgmHeader->HaveData) ;  //TODO do we need gInfo->viewCount here
}
static ismStore_memGenToken_t *cip_get_shutdown_token(void)
{
  ismStore_memMgmtHeader_t *pMgmHeader;
  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  return &pMgmHeader->GenToken ; 
}
static uint32_t cip_get_session_count(void)
{
  ismStore_memMgmtHeader_t *pMgmHeader;
  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  return pMgmHeader->SessionCount ; 
}
static void *cip_get_session_id(void)
{
  ismStore_memMgmtHeader_t *pMgmHeader;
  pMgmHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
  return &pMgmHeader->SessionId ; 
}
//--------------------------------------------

int cip_prepare_req_msg(haGlobalInfo *gInfo)
{
  int numActiveConns;
  char *p ; 
  ismCluster_Statistics_t cs[1];
  cs->connectedServers=cs->disconnectedServers=0;
  cs->pClusterName=cs->pServerName=cs->pServerUID=NULL;
  cs->healthStatus=ISM_CLUSTER_HEALTH_UNKNOWN;
  cs->haStatus=ISM_CLUSTER_HA_UNKNOWN;
  haConReqMsg *msg = &gInfo->dInfo->req_msg[0] ; 
  memset(msg, 0, sizeof(haConReqMsg)) ; 
//msg->msg_len  = sizeof(haConReqMsg) - INT_SIZE ; 
  msg->version  = HA_WIRE_VERSION ; 
  msg->msg_type = HA_MSG_TYPE_CON_REQ ; 
  msg->flags |= gInfo->viewCount ? HA_MSG_FLAG_IS_PRIM : 0 ; 
  msg->flags |= gInfo->config->PreferredPrimary ? HA_MSG_FLAG_PREF_PRIM : 0 ; 
  msg->flags |= cip_can_be_prim(gInfo) ? HA_MSG_FLAG_CAN_BE_PRIM : 0 ; 
  msg->flags |= cip_was_prim(gInfo) ? HA_MSG_FLAG_WAS_PRIM : 0 ; 
  msg->flags |= cip_store_have_data(gInfo) ? HA_MSG_FLAG_HAVE_DATA : 0 ; 
  msg->flags |= gInfo->config->StartupMode==CIP_STARTUP_STAND_ALONE ? HA_MSG_FLAG_STANDALONE : 0 ; 
  msg->flags |= HA_MSG_FLAG_USE_COMPACT ; 
  msg->flags |= gInfo->config->DisableAutoResync && !gInfo->viewCount ? HA_MSG_FLAG_NO_RESYNC : 0 ; 
//msg->flags |= ism_common_getBooleanConfig(ismCLUSTER_CFG_ENABLECLUSTER, 0) ? HA_MSG_FLAG_IS_CLUSTER : 0 ; 
  msg->flags |=(ism_cluster_getStatistics(cs)==ISMRC_ClusterDisabled)? 0 : HA_MSG_FLAG_IS_CLUSTER ; 
  msg->TotalMemSizeMB = (int)(ismStore_memGlobal.TotalMemSizeBytes>>20) ; 
  msg->MgmtSmallGranuleSizeBytes = ismSTORE_CFG_MGMT_SMALL_GRANULE_ORG ; // ismStore_memGlobal.MgmtSmallGranuleSizeBytes ; 
  msg->MgmtGranuleSizeBytes      = ismSTORE_CFG_MGMT_GRANULE_SIZE_ORG  ; // ismStore_memGlobal.MgmtGranuleSizeBytes ; 
  msg->GranuleSizeBytes = ismStore_memGlobal.GranuleSizeBytes ; 
  msg->MgmtMemPercent = ismStore_memGlobal.MgmtMemPct ; 
  msg->InMemGensCount = ismStore_memGlobal.InMemGensCount ; 
  msg->ReplicationProtocol = gInfo->config->ReplicationProtocol ; 
  msg->id_len = sizeof(ismStore_HANodeID_t) ; 
  memcpy(msg->source_id, gInfo->server_id, sizeof(ismStore_HANodeID_t)) ; 
  memcpy(msg->shutdown_token, cip_get_shutdown_token(), sizeof(ismStore_memGenToken_t)) ; 
  msg->hbto = gInfo->config->HeartbeatTimeout ; 
  msg->session_count = cip_get_session_count() ; 
  memcpy(msg->session_id, cip_get_session_id(), sizeof(ismStore_HASessionID_t)) ; 
//if ( gInfo->config->gUpd[2] != gInfo->config->gUpd[0] )
  {
    gInfo->config->gUpd[2] = gInfo->config->gUpd[0] ;
    msg->grp_len = gInfo->mcInfo->grp_len ;
    memcpy(msg->data, gInfo->config->Group, msg->grp_len) ; 
  }

  p =  msg->data + msg->grp_len + 1 ; 
  memcpy(p, &gInfo->config->SplitBrainPolicy, sizeof(int)) ; p += INT_SIZE ; 

  numActiveConns = (gInfo->viewCount && gInfo->config->SplitBrainPolicy == 1) ? ism_transport_getNumActiveConns() : -1 ; 
  memcpy(p, &numActiveConns, sizeof(int)) ; p += INT_SIZE ; 

  msg->msg_len = p - (char *)msg - INT_SIZE ; 
  return 0 ; 
}

//--------------------------------------------

int cip_prepare_res_msg(haGlobalInfo *gInfo)
{
  int rc, role[2] ; 
  haConResMsg *msg = &gInfo->dInfo->res_msg[0] ; 

  rc = cip_check_req_msgs(gInfo, role) ; 

  memset(msg, 0, sizeof(haConResMsg)) ; 
  msg->msg_len  = sizeof(haConResMsg) - INT_SIZE ; 
  msg->version  = HA_WIRE_VERSION ; 
  msg->msg_type = HA_MSG_TYPE_CON_RES ; 
  msg->conn_rejected = (rc != ISM_HA_REASON_OK );
  msg->reject_reason = rc ; 
  msg->role_local  = role[0] ; 
  msg->role_remote = role[1] ; 
  if ( IP_AF(gInfo->ipExt) == AF_INET )
  {
    SA4 *sa4 = IP_SA4(gInfo->ipExt) ; 
    msg->ha_port = sa4->sin_port ; 
    msg->ha_addr_len = sizeof(IA4) ; 
    memcpy(msg->ha_nic,&sa4->sin_addr, sizeof(IA4)) ; 
  }
  else
  {
    SA6 *sa6 = IP_SA6(gInfo->ipExt) ; 
    msg->ha_port = sa6->sin6_port ; 
    msg->ha_addr_len = sizeof(IA6) ; 
    memcpy(msg->ha_nic,&sa6->sin6_addr, sizeof(IA6)) ; 
  }
  msg->id_len = sizeof(ismStore_HANodeID_t) ; 
  memcpy(msg->source_id, gInfo->server_id, sizeof(ismStore_HANodeID_t)) ; 
  memcpy(msg->destination_id, gInfo->dInfo->req_msg[1].source_id, sizeof(ismStore_HANodeID_t)) ; 
  return 0 ; 
}

//--------------------------------------------

int cip_prepare_ack_msg(haGlobalInfo *gInfo)
{
  int rc, role[2] ; 
  haConAckMsg *msg = &gInfo->dInfo->ack_msg[0] ; 

  rc = cip_check_res_msgs(gInfo, role) ; 

  memset(msg, 0, sizeof(haConAckMsg)) ; 
  msg->msg_len  = sizeof(haConAckMsg) - INT_SIZE ; 
  msg->version  = HA_WIRE_VERSION ; 
  msg->msg_type = HA_MSG_TYPE_CON_ACK ; 
  msg->conn_rejected = (rc != ISM_HA_REASON_OK) ; 
  msg->reject_reason = rc ; 
  msg->role_local  = role[0] ; 
  msg->role_remote = role[1] ; 
  msg->id_len = sizeof(ismStore_HANodeID_t) ; 
  memcpy(msg->source_id, gInfo->server_id, sizeof(ismStore_HANodeID_t)) ; 
  memcpy(msg->destination_id, gInfo->dInfo->req_msg[1].source_id, sizeof(ismStore_HANodeID_t)) ; 
  return 0 ; 
}

//--------------------------------------------

int cip_prepare_s_cfp_cid(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int l = sizeof(haConCidMsg) ; 
  haConCidMsg msg[1] ; 
  memset(msg, 0, l) ; 
  msg->msg_len  = l - INT_SIZE ; 
  msg->version  = HA_WIRE_VERSION ; 
  msg->msg_type = HA_MSG_TYPE_CON_CID ; 
  msg->chn_id   = cInfo->channel->channel_id | (cInfo->channel->flags<<24) ; 

  memcpy(cInfo->wrInfo.buffer, msg, l) ; 
  cInfo->wrInfo.bptr = cInfo->wrInfo.buffer ; 
  cInfo->wrInfo.reqlen = l ; 
  cInfo->wrInfo.offset = 0 ; 

  return 0 ; 
}

//--------------------------------------------

int cip_prepare_s_cfp_hbt(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int l = sizeof(haConHbtMsg) ; 
  haConHbtMsg msg[1] ; 
  memset(msg, 0, l) ; 
  msg->msg_len  = l - INT_SIZE ; 
  msg->version  = HA_WIRE_VERSION ; 
  msg->msg_type = HA_MSG_TYPE_CON_HBT ; 
  msg->hbt_count= ++cInfo->hb_count ;  

  memcpy(cInfo->wrInfo.buffer, msg, l) ; 
  cInfo->wrInfo.bptr = cInfo->wrInfo.buffer ; 
  cInfo->wrInfo.reqlen = l ; 
  cInfo->wrInfo.offset = 0 ; 

  return 0 ; 
}

//--------------------------------------------

int cip_prepare_s_cfp_req(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int l ;
  haConReqMsg *msg = &gInfo->dInfo->req_msg[0] ; 
  cip_prepare_req_msg(gInfo) ; 
  l = msg->msg_len + INT_SIZE ; 
  memcpy(cInfo->wrInfo.buffer, msg, l) ; 
  cInfo->wrInfo.bptr = cInfo->wrInfo.buffer ; 
  cInfo->wrInfo.reqlen = l ; 
  cInfo->wrInfo.offset = 0 ; 

  return 0 ; 
}

//--------------------------------------------

int cip_prepare_s_cfp_res(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int l ;
  haConResMsg *msg = &gInfo->dInfo->res_msg[0] ; 
  cip_prepare_res_msg(gInfo) ; 
  l = msg->msg_len + INT_SIZE ; 
  memcpy(cInfo->wrInfo.buffer, msg, l) ; 
  cInfo->wrInfo.bptr = cInfo->wrInfo.buffer ; 
  cInfo->wrInfo.reqlen = l ; 
  cInfo->wrInfo.offset = 0 ; 

  gInfo->dInfo->sendBit = RES_MSG_SENT ; 
//cInfo->state    = CIP_STATE_S_CFP_RES ; 
  cInfo->io_state = CIP_IO_STATE_WRITE ; 
  gInfo->cipInfo->fds[cInfo->ind].events = POLLOUT ; 

  return 0 ; 
}

//--------------------------------------------

int cip_prepare_s_cfp_ack(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int l ;
  haConAckMsg *msg = &gInfo->dInfo->ack_msg[0] ; 
  cip_prepare_ack_msg(gInfo) ; 
  l = msg->msg_len + INT_SIZE ; 
  memcpy(cInfo->wrInfo.buffer, msg, l) ; 
  cInfo->wrInfo.bptr = cInfo->wrInfo.buffer ; 
  cInfo->wrInfo.reqlen = l ; 
  cInfo->wrInfo.offset = 0 ; 

  gInfo->dInfo->sendBit = ACK_MSG_SENT ; 
//cInfo->state    = CIP_STATE_S_CFP_ACK ; 
  cInfo->io_state = CIP_IO_STATE_WRITE ; 
  gInfo->cipInfo->fds[cInfo->ind].events = POLLOUT ; 

  return 0 ; 
}

//--------------------------------------------

int cip_prepare_s_cfp_trm(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int l = sizeof(haConComMsg) ; 
  haConComMsg msg[1] ; 
  memset(msg, 0, l) ; 
  msg->msg_len  = l - INT_SIZE ; 
  msg->version  = HA_WIRE_VERSION ; 
  msg->msg_type = HA_MSG_TYPE_NOD_TRM ; 

  memcpy(cInfo->wrInfo.buffer, msg, l) ; 
  cInfo->wrInfo.bptr = cInfo->wrInfo.buffer ; 
  cInfo->wrInfo.reqlen = l ; 
  cInfo->wrInfo.offset = 0 ; 

  gInfo->dInfo->sendBit = TRM_MSG_SENT ; 
//cInfo->state    = CIP_STATE_S_CFP_ACK ; 
  cInfo->io_state = CIP_IO_STATE_WRITE ; 
  gInfo->cipInfo->fds[cInfo->ind].events = POLLOUT ; 

  return 0 ; 
}

//--------------------------------------------

int cip_prepare_s_ack_trm(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int l = sizeof(haConComMsg) ; 
  haConComMsg msg[1] ; 
  memset(msg, 0, l) ; 
  msg->msg_len  = l - INT_SIZE ; 
  msg->version  = HA_WIRE_VERSION ; 
  msg->msg_type = HA_MSG_TYPE_ACK_TRM ; 

  memcpy(cInfo->wrInfo.buffer, msg, l) ; 
  cInfo->wrInfo.bptr = cInfo->wrInfo.buffer ; 
  cInfo->wrInfo.reqlen = l ; 
  cInfo->wrInfo.offset = 0 ; 

  return 0 ; 
}

//--------------------------------------------

int free_conn(ConnInfoRec *cInfo)
{
  haGlobalInfo *gInfo=gInfo_;

  if ( cInfo->state == CIP_STATE_ESTABLISH )
  {
    TRACE(5,"%s: Connection closed: conn= %s\n",__func__,cInfo->req_addr) ; 
  }

 #if USE_IB
  if ( cInfo->use_ib )
  {
    if ( cInfo->use_ib && cInfo->cm_id && cInfo->cm_id->qp )
    {
      Xrdma_disconnect(cInfo->cm_id) ; 
      su_sleep(8,SU_MIL) ; 
    }
    cip_destroy_ib_stuff(cInfo) ;
  }
  else
 #endif
 #if USE_SSL
  if ( gInfo->use_ssl && cInfo->sslInfo->ssl )
  {
    SSL_shutdown(cInfo->sslInfo->ssl) ; 
    SSL_free    (cInfo->sslInfo->ssl) ; 
    pthread_mutex_destroy(cInfo->sslInfo->lock) ; 
  }
 #endif
  close(cInfo->sfd) ; 

  if ( cInfo->rdInfo.buffer && cInfo->rdInfo.need_free )
    ism_common_free(ism_memory_store_misc,cInfo->rdInfo.buffer) ; 
  if ( cInfo->wrInfo.buffer && cInfo->wrInfo.need_free )
    ism_common_free(ism_memory_store_misc,cInfo->wrInfo.buffer) ; 
  ism_common_free(ism_memory_store_misc,cInfo) ; 

  return 0 ; 
}
//--------------------------------------------

int cip_conn_failed_(haGlobalInfo *gInfo, ConnInfoRec *cInfo, int ec, int ln)
{
  if ( !cInfo )
    return 0 ; 
  TRACE(((cInfo->state == CIP_STATE_ESTABLISH) ? 5 : 9),"cip_conn_failed called from line %d for conn: %s, cInfo %p, channel %p, ec %d\n",
         ln,cInfo->req_addr,cInfo,cInfo->channel,ec) ; 

  if ( cInfo->channel )
    ha_raise_event(cInfo, ec) ; 
  else
  {
    if ( gInfo->dInfo->cIhb[0] == cInfo )
    {
         gInfo->dInfo->cIhb[0] = NULL ; 
         gInfo->dInfo->state |= DSC_RESTART ;
    }
    if ( gInfo->dInfo->cIlc == cInfo )
    {
         gInfo->dInfo->cIlc = NULL ; 
      if (!gInfo->dInfo->cIhb[0] && (gInfo->dInfo->state & REQ_SENT_LCL) )
        gInfo->dInfo->state &= ~(REQ_MSG_SENT | REQ_SENT_LCL) ; 
    }
    if ( gInfo->dInfo->cIrm == cInfo )
    {
         gInfo->dInfo->cIrm = NULL ; 
      if (!gInfo->dInfo->cIhb[0] && (gInfo->dInfo->state & REQ_RECV_RMT) )
        gInfo->dInfo->state &= ~(REQ_MSG_RECV | REQ_RECV_RMT) ; 
    }
    if ( gInfo->dInfo->cIhb[1] == cInfo )
         gInfo->dInfo->cIhb[1] = NULL ; 
  }

  cip_update_conn_list(gInfo, cInfo, 0) ;
  free_conn(cInfo) ; 
  su_sleep(8,SU_MIL) ; 

  return 0 ; 
}

//--------------------------------------------


void cip_remove_conns(haGlobalInfo *gInfo, int all, int type)
{
  ConnInfoRec  *cInfo, *nextC ; 
  for ( cInfo=gInfo->cipInfo->firstConnInProg ; cInfo ; cInfo=nextC )
  {
    nextC = cInfo->next ; 
    if ( all || cInfo->state != CIP_STATE_LISTEN )
      cip_conn_failed(gInfo, cInfo, type) ; 
  }
  if ( all && gInfo->config->AutoConfig )
  {
    ipFlat ip[1] ; 
    socklen_t vlen ; 
    if ( gInfo->mcInfo->sfd[0] != -1 )
    {
      struct ip_mreq mreq ;
      haGetAddr(AF_INET , 0, ismHA_CFG_IPV4_MCAST_ADDR, ip) ; 
      memset(&mreq, 0, sizeof(mreq));
      memcpy(&mreq.imr_multiaddr,ip->bytes, ip->length) ;
      memcpy(&mreq.imr_interface,&gInfo->haIf->ipv4_addr,sizeof(IA4)) ;
      vlen = sizeof(mreq) ;
      if ( setsockopt(gInfo->mcInfo->sfd[0], IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *)&mreq, vlen) < 0 )
      {
        TRACE(1," failed to set socket option to IP_DROP_MEMBERSHIP, rc=%d (%s)\n",errno,strerror(errno)) ; 
      }
      close(gInfo->mcInfo->sfd[0]) ; 
      gInfo->mcInfo->sfd[0] = -1 ; 
    }
    if ( gInfo->mcInfo->sfd[1] != -1 )
    {
      struct ipv6_mreq mreq6 ;
      haGetAddr(AF_INET6 , 0, ismHA_CFG_IPV6_MCAST_ADDR, ip) ; 
      memset(&mreq6, 0, sizeof(mreq6));
      memcpy(&mreq6.ipv6mr_multiaddr,ip->bytes, ip->length) ;
      mreq6.ipv6mr_interface = gInfo->haIf->index ;
      vlen = sizeof(mreq6) ;
      if ( setsockopt(gInfo->mcInfo->sfd[1], IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (void *)&mreq6, vlen) < 0 )
      {
        TRACE(1," failed to set socket option to IPV6_DROP_MEMBERSHIP, rc=%d (%s)\n",errno,strerror(errno)) ; 
      }
      close(gInfo->mcInfo->sfd[1]) ; 
      gInfo->mcInfo->sfd[1] = -1 ; 
    }
    if ( gInfo->mcInfo->iMsg )
    {
      ism_common_free(ism_memory_store_misc,gInfo->mcInfo->iMsg) ; 
      gInfo->mcInfo->iMsg = NULL ; 
    }
    if ( gInfo->mcInfo->oMsg )
    {
      ism_common_free(ism_memory_store_misc,gInfo->mcInfo->oMsg) ; 
      gInfo->mcInfo->oMsg = NULL ; 
    }
  }
}
//--------------------------------------------

int cip_restart_discovery_(haGlobalInfo *gInfo,int line)
{
  ipInfo *ipHead;
  TRACE(8,"cip_restart_discovery_ called from line %d\n",line);
  cip_remove_conns(gInfo, 0, CH_EV_NEW_VIEW) ; 
  gInfo->dInfo->state = DSC_IS_ON | (gInfo->dInfo->state&DSC_WORK_SOLO) ; // resetting all other bits
  gInfo->dInfo->bad[0] = gInfo->dInfo->bad[1] = 0 ; 
  gInfo->viewBreak = 0 ;
  ipHead = gInfo->ipHead ; 
  gInfo->ipHead = NULL ; 
  buildIpList(1, 1) ; 
  if ( gInfo->ipHead )
  {
    ipInfo *ip ;  
    while( ipHead )
    {
      ip = ipHead ; 
      ipHead = ip->next ; 
      ism_common_free(ism_memory_store_misc,ip) ; 
    }
  }
  else
    gInfo->ipHead = ipHead ; 
  gInfo->ipCur = gInfo->ipHead ; 
  return 0 ; 
}

//--------------------------------------------

int cip_raise_view_(haGlobalInfo *gInfo, int type, int line)
{
  int amc = gInfo->lastView->ActiveNodesCount ; 
  gInfo->lastView->OldRole = gInfo->lastView->NewRole ;
  gInfo->dInfo->bad[0] = gInfo->dInfo->bad[1] = 0 ; 

  TRACE(1,"RaiseView called from line %d with type %d\n",line,type);

  if ( type == VIEW_CHANGE_TYPE_DISC )
  {
    if ( (gInfo->dInfo->state & DSC_SSL_ERR) )
    {
      gInfo->lastView->ReasonCode = ISM_HA_REASON_CONFIG_ERROR ;
      gInfo->lastView->pReasonParam = "Configuration mismatch: HA.UseSecuredConnections." ; 
      gInfo->lastView->NewRole = ISM_HA_ROLE_ERROR ; 
    }
    else
    if ( (gInfo->dInfo->state & DSC_TIMEOUT) )
    {
      if ( gInfo->config->StartupMode == CIP_STARTUP_AUTO_DETECT )
      {
        gInfo->lastView->ReasonCode = ISM_HA_REASON_DISC_TIMEOUT; 
        gInfo->lastView->NewRole = ISM_HA_ROLE_ERROR ; 
      }
      else
        gInfo->lastView->NewRole = ISM_HA_ROLE_PRIMARY ; 
      gInfo->lastView->ActiveNodesCount = 1 ; 
    }
    else
    {
      gInfo->lastView->NewRole = gInfo->dInfo->ack_msg[0].role_local ; 
      if ( gInfo->dInfo->ack_msg[0].conn_rejected || gInfo->dInfo->ack_msg[1].conn_rejected )
      {
        gInfo->lastView->ActiveNodesCount = 1 ; 
        gInfo->lastView->ReasonCode = gInfo->dInfo->ack_msg[0].reject_reason ; 
      }
      else
        gInfo->lastView->ActiveNodesCount = 2 ; 
    }
    memcpy(gInfo->lastView->ActiveNodeIds[1], gInfo->dInfo->ack_msg[1].source_id, sizeof(ismStore_HANodeID_t)) ;
    if ( gInfo->lastView->ActiveNodesCount > 1 )
    {
      ipFlat *ip = &gInfo->dInfo->cIhb[0]->rmt_addr ; 
      haConReqMsg *lmsg = &gInfo->dInfo->req_msg[0] ; 
      haConReqMsg *rmsg = &gInfo->dInfo->req_msg[1] ; 
      if ( rmsg->msg_len > offsetof(haConReqMsg,grp_len) && !lmsg->grp_len )
      {
        char *p , cp, cs ; 
        unsigned char *pn, *sn, *pu ; 
        if ( gInfo->lastView->NewRole == ISM_HA_ROLE_PRIMARY )
        {
          pn = lmsg->source_id ; 
          sn = rmsg->source_id ; 
          pu = lmsg->source_id + 6 ; 
        }
        else
        {
          pn = rmsg->source_id ; 
          sn = lmsg->source_id ; 
          pu = rmsg->source_id + 6 ; 
        }
        p = gInfo->config->autoHAGroup ; 
        cp = pn[6] ; pn[6] = 0 ; 
        cs = sn[6] ; sn[6] = 0 ; 
        p += snprintf(p, ADDR_STR_LEN,"%s-%s_",pn,sn) ; 
        pn[6] = cp ; 
        sn[6] = cs ; 
        b2h(p, pu, 6) ; 
        gInfo->lastView->autoHAGroup = gInfo->config->autoHAGroup ;
        gInfo->config->Group = gInfo->lastView->autoHAGroup ; 
        gInfo->mcInfo->grp_len = su_strlen(gInfo->config->Group) ; 
        gInfo->config->gUpd[0]++ ; 
        TRACE(5,"Constructed HAGroup: |%s|\n",gInfo->config->Group);
      }
      if ( ip->length == sizeof(IA4) )
        inet_ntop(AF_INET, ip->bytes, gInfo->lastView->RemoteDiscoveryNIC, ADDR_STR_LEN) ; 
      else
        inet_ntop(AF_INET6,ip->bytes, gInfo->lastView->RemoteDiscoveryNIC, ADDR_STR_LEN) ; 

      gInfo->dInfo->state |= DSC_VU_SENT ;
      if ( gInfo->dInfo->req_msg[1].hbto > 0 && gInfo->dInfo->req_msg[1].hbto < gInfo->dInfo->req_msg[0].hbto )
      {
        TRACE(5,"HighAvailability HeartbeatTimeout is reduced from %u to %u to mach the other side.\n",gInfo->dInfo->req_msg[0].hbto,gInfo->dInfo->req_msg[1].hbto);
        gInfo->hbTimeOut = (double)gInfo->dInfo->req_msg[1].hbto ; 
      }
      else
        gInfo->hbTimeOut = (double)gInfo->config->HeartbeatTimeout ; 
    }
    else
      gInfo->dInfo->state = DSC_WORK_SOLO ; // resetting al other bits
    if ( gInfo->lastView->NewRole == ISM_HA_ROLE_ERROR )
    {
      gInfo->dInfo->state = 0 ; 
      gInfo->goDown = 1 ; 
    }
  }
  else
  if ( type == VIEW_CHANGE_TYPE_PRIM_SOLO )
  {
    cip_remove_conns(gInfo, 0, CH_EV_NEW_VIEW) ; 
    gInfo->lastView->NewRole = ISM_HA_ROLE_PRIMARY ; 
    gInfo->lastView->ActiveNodesCount = 1 ; 
    gInfo->dInfo->state = DSC_WORK_SOLO ; // resetting al other bits
  }
  else
  if ( type == VIEW_CHANGE_TYPE_BKUP_PRIM )
  {
    cip_remove_conns(gInfo, 0, CH_EV_NEW_VIEW) ; 
    gInfo->lastView->NewRole = ISM_HA_ROLE_PRIMARY ; 
    gInfo->lastView->ActiveNodesCount = 1 ; 
    gInfo->dInfo->state = DSC_WORK_SOLO ; // resetting al other bits
  }
  else
  if ( type == VIEW_CHANGE_TYPE_BKUP_TERM )
  {
    cip_remove_conns(gInfo, 1, CH_EV_TERMINATE) ; 
    gInfo->lastView->NewRole = ISM_HA_ROLE_TERM ; 
    gInfo->lastView->ActiveNodesCount = 1 ; 
    gInfo->dInfo->state = 0 ; 
    gInfo->goDown = 1 ; 
  }
  else
  if ( type == VIEW_CHANGE_TYPE_BKUP_ERR  )
  {
    if ( gInfo->sbError )
      gInfo->lastView->ReasonCode = ISM_HA_REASON_SYSTEM_ERROR ; 
    else if ( gInfo->config->SplitBrainPolicy == 1 )
      gInfo->lastView->ReasonCode = ISM_HA_REASON_SPLIT_BRAIN_RESTART ; 
    else
      gInfo->lastView->ReasonCode = ISM_HA_REASON_SPLIT_BRAIN ; 
    cip_remove_conns(gInfo, 1, CH_EV_TERMINATE) ; 
    gInfo->lastView->NewRole = ISM_HA_ROLE_ERROR; 
    gInfo->lastView->ActiveNodesCount = 1 ; 
    gInfo->dInfo->state = 0 ; 
    gInfo->goDown = 1 ; 
  }

  if ( amc > 1 && gInfo->lastView->OldRole == ISM_HA_ROLE_STANDBY )
    wait4channs(gInfo, 5e0, 1) ; 

  if ( gInfo->lastView->OldRole != gInfo->lastView->NewRole ||
       gInfo->lastView->ActiveNodesCount != amc )
  {
    memcpy(gInfo->lastView->ActiveNodeIds[0], gInfo->server_id, sizeof(ismStore_HANodeID_t)) ;
    gInfo->viewTime = viewTime = su_sysTime() ; 
    gInfo->lastView->ViewId++ ; 
    gInfo->viewCount = gInfo->lastView->ViewId ; 
    if ( gInfo->params->ViewChanged(gInfo->lastView, gInfo->params->pContext) == StoreRC_OK )
    {
      gInfo->myRole = gInfo->lastView->NewRole ; 
      gInfo->viewBreak = 0 ;
    }
    else
    {
      TRACE(1,"ViewChanged returned with error.  The haControl thread is going down.\n");
      gInfo->goDown |= 1 ; 
    }
  }
  DBG_PRINT("_dbg_%d: cip_raise_view called: ; %d %d %d %d %d\n",__LINE__,amc, gInfo->lastView->OldRole, gInfo->lastView->NewRole, gInfo->lastView->ActiveNodesCount,gInfo->lastView->ViewId); 

  return 0 ; 
}
//--------------------------------------------

ConnInfoRec *cip_prepare_conn_req(haGlobalInfo *gInfo, int is_ha, ChannInfo *channel)
{
  ConnInfoRec *cInfo, *last ; 

  if (!(cInfo = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,190),sizeof(ConnInfoRec))) )
  {
    TRACE(1," failed to allocate memory of size %lu.\n",sizeof(ConnInfoRec)) ; 
    return NULL ; 
  }
  memset(cInfo, 0, sizeof(ConnInfoRec)) ; 
  cInfo->init_here = 1 ; 
  cInfo->is_ha = is_ha ; 
  cInfo->channel = channel ; 

  pthread_mutex_lock(gInfo->haLock) ; 
  for ( last=gInfo->connReqQ ; last && last->next ; last = last->next ) ; // empty body
  if ( last )
    last->next = cInfo ; 
  else
    gInfo->connReqQ = cInfo ; 
  pthread_mutex_unlock(gInfo->haLock) ; 
  return cInfo ; 
}
//--------------------------------------------

int cip_ssl_handshake(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int rc;

 #if USE_SSL
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
     #if 0
      SSL_set_app_data(ssl, (char *)transport);
      SSL_set_info_callback(ssl, ism_common_sslInfoCallback);
      SSL_set_msg_callback(ssl, ism_common_sslProtocolDebugCallback);
      SSL_set_msg_callback_arg(ssl, transport);
     #endif
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
        gInfo->cipInfo->fds[cInfo->ind].events = POLLOUT ; 
        return 0 ; 
      }
      else
      if ( ec == SSL_ERROR_WANT_READ )
      {
        gInfo->cipInfo->fds[cInfo->ind].events = POLLIN ; 
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
          const char * sslErr = X509_verify_cert_error_string(rc);
          TRACE(4, "Certificate verification failed: conn=%s:%d id=%lu error=%s",
                    cInfo->req_addr,cInfo->req_port,cInfo->conn_id,sslErr);
          return -1;
        }
      }
      else
      {
        /*
         * A completed TLS incoming connection
         */
      }
    }
  }
 #endif

  return 1 ; 
}
//--------------------------------------------

int cip_conn_ready(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  char a[2][ADDR_STR_LEN], p[2][8] ; 
  errInfo ei[1] ; 
  ei->errLen = sizeof(ei->errMsg) ; 
  haGetNameInfo(cInfo->lcl_sa, a[0], ADDR_STR_LEN, p[0], 8, ei) ; 
  haGetNameInfo(cInfo->rmt_sa, a[1], ADDR_STR_LEN, p[1], 8, ei) ; 
  cInfo->conn_id = ++gInfo->conn_id ; 
  cInfo->next_r_time  = su_sysTime() + gInfo->hbTimeOut ; 
  snprintf(cInfo->req_addr, HOSTNAME_STR_LEN, "%s|%s %s %s|%s (%lu)",a[0],p[0],(cInfo->init_here?"->":"<-"),a[1],p[1],cInfo->conn_id) ; 
//TRACE(5," established connection: %s. (%d %p %p) \n",cInfo->req_addr,cInfo->is_ha,gInfo->dInfo->cIhb[1],cInfo->channel);
  TRACE(5," established connection: %s. (%s NIC)\n",cInfo->req_addr,cInfo->is_ha?"Replication":"Discovery");

 #if USE_IB
  if ( cInfo->use_ib )
    cInfo->conn_read = conn_read_ib ; 
  else
 #endif
 #if USE_SSL
  if ( cInfo->use_ssl )
    cInfo->conn_read = conn_read_ssl; 
  else
 #endif
    cInfo->conn_read = conn_read_tcp; 

  return 0 ; 
}

//--------------------------------------------
int cip_handle_conn_est(haGlobalInfo *gInfo, ConnInfoRec *cInfo) 
{
  if ( cip_set_local_endpoint(cInfo) == -1  )
    return -1 ; 
  if ( cip_set_remote_endpoint(cInfo) == -1 )
    return -1 ; 
  if ( cip_conn_ready(gInfo, cInfo) == -1 )
    return -1 ; 
  cInfo->next_r_time  = su_sysTime() + gInfo->hbTimeOut ; 
  if ( cInfo->channel )
  {
    if ( cip_prepare_s_cfp_cid(gInfo, cInfo) == -1 )
      return -1 ; 
  //cInfo->state    = CIP_STATE_S_CFP_CID ; 
  }
  else
  if ( cInfo->is_ha )
  {
    if ( cip_prepare_s_cfp_hbt(gInfo, cInfo) == -1 )
      return -1 ; 
  //cInfo->state    = CIP_STATE_S_CFP_HBT ; 
  }
  else
  if ( !(gInfo->dInfo->state & REQ_MSG_RECV) )
  {
    if ( cip_prepare_s_cfp_req(gInfo, cInfo) == -1 )
      return -1 ; 
  //cInfo->state    = CIP_STATE_S_CFP_REQ ; 
    gInfo->dInfo->sendBit = (REQ_MSG_SENT | REQ_SENT_LCL) ; 
  }
  else
  {
    TRACE(5," connection to %s dropped since REQ_MSG already RECV.\n",cInfo->req_addr);
    return -1 ; // not an error, just this conn is not needed anymore
  }
  cInfo->state = CIP_STATE_ESTABLISH ; 
  cInfo->io_state = CIP_IO_STATE_WRITE ; 
  gInfo->cipInfo->fds[cInfo->ind].events = POLLOUT ; 


  return 0 ;
}
//--------------------------------------------

int cip_set_buffers(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int bs ; 
  if ( cInfo->channel ) // i.e. gonna be used for heavy trafic
  {
    if ( cInfo->init_here )
    {
      bs = gInfo->SockBuffSizes[0][1] ;
      if ( cip_set_socket_buffer_size(cInfo->sfd, bs, SO_RCVBUF) == -1 )
      {
        TRACE(1," failed to set the socket receive buffer size to %d. ec= %d (%s)\n",bs,errno,strerror(errno));
        return -1 ; 
      }
    }
   #if 0
    else
    // This is done on the accept socket
   #endif
    bs = gInfo->SockBuffSizes[2][1] ;
  }
  else
    bs = 0x10000 ; 
  if (!(cInfo->rdInfo.buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,191),bs)) )
  {
    TRACE(1," failed to allocate receive buffer of size %d.\n",bs);
    return -1 ; 
  }
  cInfo->rdInfo.buflen = bs ; 
  cInfo->rdInfo.need_free = 1 ; 
  cInfo->rdInfo.bptr = cInfo->rdInfo.buffer ; 
  cInfo->rdInfo.reqlen = INT_SIZE ; 

  if ( cInfo->channel && cInfo->init_here ) // i.e. gonna be used for heavy trafic
  {
    if ( cInfo->init_here )
    {
      bs = gInfo->SockBuffSizes[0][0] ;
      if ( cip_set_socket_buffer_size(cInfo->sfd, bs, SO_SNDBUF) == -1 )
      {
        TRACE(1," failed to set the socket send buffer size to %d. ec= %d (%s)\n",bs,errno,strerror(errno));
        return -1 ; 
      }
    }
   #if 0
    else
    // This is done on the accept socket
   #endif
    bs = gInfo->SockBuffSizes[2][0] ;
  }
  else
    bs = 0x10000 ; 
  if (!(cInfo->wrInfo.buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,192),bs)) )
  {
    TRACE(1," failed to allocate send buffer of size %d.\n",bs);
    return -1 ; 
  }
  cInfo->wrInfo.buflen = bs ; 
  cInfo->wrInfo.need_free = 1 ; 

  return 0 ; 
}
//--------------------------------------------

int cip_handle_conn_req(haGlobalInfo *gInfo, ConnInfoRec *cInfo) 
{
  int ok, rc , use_ib ; 
  int   sfd=0 ; 
  ipInfo *ipi ; 
  ifInfo *ifi ; 
  SA6 *sa6 ; 
 #if USE_IB
  SA4 *sa4 ; 
  int ret ; 
  struct rdma_event_channel *ev_ch ;
 #endif
  char port[8] ; 
  errInfo ei[1] ; 
  const char* myName = "storeHA_cip_handle_conn_req";
  ei->errLen = sizeof(ei->errMsg) ; 

  ok = 0 ; 
  do
  {
    if ( cInfo->is_ha )
    {
      ipi = gInfo->ipRmt ; 
      ifi = gInfo->haIf ; 
    }
    else
    {
      if ( gInfo->ipCur )
        gInfo->ipCur = gInfo->ipCur->next ; 
      if (!gInfo->ipCur )
      {
        if ( gInfo->nextResolve < su_sysTime() )
        {
          buildIpList(1, 1) ; 
          gInfo->nextResolve = su_sysTime() +  gInfo->haTimeOut[0]/2 ; 
        }
        gInfo->ipCur = gInfo->ipHead ; 
      }
      if (!gInfo->ipCur )
        break ; 
      ipi = gInfo->ipCur ;
      ifi = gInfo->hbIf ; 
    }
    haGetNameInfo(ipi->ai.ai_addr, cInfo->req_addr,HOSTNAME_STR_LEN, port,8, ei) ; 
    cInfo->req_port = atoi(port) ; 
    TRACE(9," start handling a connection request to %s|%s.\n",cInfo->req_addr,port);

    cInfo->use_ib = use_ib = (cInfo->is_ha && gInfo->use_ib) ; 

   #if USE_IB
    if ( use_ib )
    {
      SA  *sa  ; 
      do
      {
        if ( !(ev_ch = Xrdma_create_event_channel()) )
        {
          rc = errno ; 
          TRACE(1,"_$s_$d: failed to create an RDMA event channel, error %d (%s)\n.",rc,strerror(rc)) ; 
          break ; 
        }
        ok++ ; 
        sfd = ev_ch->fd ; 
        if ( ha_set_nb(sfd, ei, myName) == -1 )
        {
          TRACE(1," failed to set socket to non-blocking, rc=%d (%s)\n",errno,strerror(errno)) ; 
          break;
        }

        if ( (ret = Xrdma_create_id(ev_ch, &cInfo->cm_id, cInfo, RDMA_PS_TCP)) )
        {
          TRACE(1," failed to create the RDMA ID. The error code is %d.",ret);
          break ; 
        }
        ok++ ; 

        cInfo->init_here = 1 ; 
        cInfo->sfd = sfd ; 
        cInfo->sock_af = ipi->ai.ai_family ; 
        if ( cInfo->sock_af == AF_INET6 )
        {
          IA6 *ia6 ; 
          sa6 = (SA6 *)ipi->ai.ai_addr ; 
          ia6 = &sa6->sin6_addr ; 
          if ( IN6_IS_ADDR_LINKLOCAL(ia6) )
            sa6->sin6_scope_id = ifi->index ; 
        }

        sa = rdma_get_local_addr(ifi->cm_id) ; 
        cInfo->lcl_sa = (SA *)&cInfo->lcl_sas ; 
        if ( sa->sa_family == AF_INET )
        {
          memcpy(cInfo->lcl_sa,sa,sizeof(SA4)) ;
          sa4 = (SA4 *)cInfo->lcl_sa ; 
          sa4->sin_port = 0 ; 
        }
        else
        {
          memcpy(cInfo->lcl_sa,sa,sizeof(SA6)) ;
          sa6 = (SA6 *)cInfo->lcl_sa ; 
          sa6->sin6_port = 0 ; 
        }
        sa = cInfo->lcl_sa ; 
        if ( (rc = Xrdma_resolve_addr(cInfo->cm_id , sa, ipi->ai.ai_addr , 16)) )
        {
          TRACE(1,"_$s_$d: failed to resolve an RDMA address, error %d (%s)\n.",rc,strerror(rc)) ; 
          break ; 
        }

        cip_update_conn_list(gInfo, cInfo, 1) ;  
        DBG_PRT(printf(" cip_handle_conn_req: con added , gInfo->cipInfo->nConnsInProg=%d\n",gInfo->cipInfo->nConnsInProg)) ; 

        gInfo->cipInfo->fds[cInfo->ind].fd = cInfo->sfd ; 
        gInfo->cipInfo->fds[cInfo->ind].events = POLLIN ; 

        cInfo->io_state = CIP_IB_STATE_ADDR ; 
        return 0 ; 
      } while (0) ; 
      if ( ok == 1 )
        Xrdma_destroy_event_channel(ev_ch) ; 

      cip_conn_failed(gInfo, cInfo, CH_EV_SYS_ERROR) ;  
      return -1 ; 
    }
   #endif

    // below is TCP only, no IB
    if ( (sfd=socket_(ipi->ai.ai_family,SOCK_STREAM,IPPROTO_TCP)) == -1 )
    {
      TRACE(1," failed to create socket, rc=%d (%s)\n",errno,strerror(errno)) ; 
      break ; 
    }
    ok++ ; 
    if ( ha_set_nb(sfd, ei, myName) == -1 )
    {
      TRACE(1," failed to set socket to non-blocking, rc=%d (%s)\n",errno,strerror(errno)) ; 
      break;
    }
  
    cInfo->sfd = sfd ; 
    cInfo->init_here = 1 ; 
    memset(&cInfo->rdInfo,0,sizeof(ioInfo)) ; 
    memset(&cInfo->wrInfo,0,sizeof(ioInfo)) ; 
    if ( cip_set_buffers(gInfo, cInfo) < 0 )
      break ; 

    if ( cip_set_low_delay(sfd, 1, 0) == -1 )
    {
      TRACE(5," failed to set the socket TCP_NODELAY mode. The reason code is %d.\n",errno);
      /* break ; we can leave without it */
    }

    if ( cip_set_low_delay(sfd, 0, 1) == -1 )
    {
      TRACE(5," failed to set the socket SO_RCVLOWAT to 1. The reason code is %d.\n",errno);
      /* break ; we can leave without it */
    }
  
    cInfo->sock_af = ipi->ai.ai_family ; 
    if ( cInfo->sock_af == AF_INET6 )
    {
      IA6 *ia6 ; 
      sa6 = (SA6 *)ipi->ai.ai_addr ; 
      ia6 = &sa6->sin6_addr ; 
      if ( IN6_IS_ADDR_LINKLOCAL(ia6) )
        sa6->sin6_scope_id = ifi->index ;
    }

    cip_update_conn_list(gInfo, cInfo, 1) ;  
    DBG_PRT(printf(" cip_handle_conn_req: con added , gInfo->cipInfo->nConnsInProg=%d\n",gInfo->cipInfo->nConnsInProg)) ; 

    gInfo->cipInfo->fds[cInfo->ind].fd = cInfo->sfd ; 
  
    if ( connect(sfd,ipi->ai.ai_addr,(socklen_t)ipi->ai.ai_addrlen) == 0 )
    {
      cInfo->io_state = CIP_IO_STATE_SSL_HS ; 
    }
    else
    {
      rc = errno ;
      if ( rc == EINPROGRESS || rc == EWOULDBLOCK )
      {
        DBG_PRT(printf(" errno == EINPROGRESS\n")) ; 
        cInfo->io_state = CIP_IO_STATE_CONN ; 
        gInfo->cipInfo->fds[cInfo->ind].events = (POLLIN|POLLOUT) ; 
      }
      else
      {
        TRACE(1," failed to connect the socket. The error is: %d.\n",rc);
        break ; 
      }
    }
    return 0 ; 
  } while ( 0 ) ; 

  cip_conn_failed(gInfo, cInfo, CH_EV_SYS_ERROR) ;  
  return -1 ; 
}

//--------------------------------------------

int cip_build_new_incoming_conn(haGlobalInfo *gInfo, ConnInfoRec *acInfo)
{
  int ok, place=0, sfd ; 
  socklen_t sa_len ; 
  SAS sas[1] ; 
  SA *sa ; 
  ConnInfoRec *cInfo=NULL ; 
  const char* myName = "storeHA_cip_build_new_incoming_conn";
  errInfo ei[1] ; 
  ei->errLen = sizeof(ei->errMsg) ; 

  ok = 0 ; 
  do
  {
    sa = (SA *)sas ; 
    sa_len = sizeof(SAS) ; 
    if ( (sfd = accept_(acInfo->sfd, sa, &sa_len)) == -1 )
    {
      TRACE(5," accept failed: err= %d (%s)\n",errno,strerror(errno)) ; 
      break ; 
    }

    if ( acInfo->is_ha && !((gInfo->dInfo->state&ACK_MSG_SENT) && gInfo->dInfo->ack_msg[0].role_local == ISM_HA_ROLE_STANDBY) )
    {
      char addr[ADDR_STR_LEN], port[8] ; 
      haGetNameInfo(sa, addr, ADDR_STR_LEN, port, 8, ei);
      TRACE(5," unexpected incoming conn on HA NIC (from: %s|%s)\n",addr,port) ; 
      break ; 
    }

    if (!acInfo->is_ha && (gInfo->dInfo->cIrm || gInfo->dInfo->cIhb[0]) )
    {
      char addr[ADDR_STR_LEN], port[8] ; 
      haGetNameInfo(sa, addr, ADDR_STR_LEN, port, 8, ei);
      TRACE(7," redundant incoming conn on ADM NIC (from: %s|%s), it will be closed.\n",addr,port) ; 
      break ; 
    }

    place++ ; 
    if ( ha_set_nb(sfd, ei, myName) == -1 )
    {
      TRACE(1," failed to set socket to non-blocking, rc=%d (%s)\n",errno,strerror(errno)) ; 
      break;
    }

   #if 0
    // MUST be set on the accept socket
    if ( cip_set_low_delay(sfd, 1, 0) == -1 )
    {
      TRACE(5," failed to set the socket TCP_NODELAY to 1. The reason code is %d.\n",errno);
      /* break ; we can leave without it */
    }
    place++ ; 

    if ( cip_set_low_delay(sfd, 0, 1) == -1 )
    {
      TRACE(5," failed to set the socket SO_RCVLOWAT to 1. The reason code is %d.\n",errno);
      /* break ; we can leave without it */
    }
    place++ ; 
   #endif

    place++ ; 
    if ( (cInfo=(ConnInfoRec *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,193),sizeof(ConnInfoRec))) == NULL )
    {
      TRACE(1," failed to allocate send buffer of size %lu.\n",sizeof(ConnInfoRec));
      break ; 
    }
    ok++ ; 
    memset(cInfo,0,sizeof(ConnInfoRec)) ; 

    cInfo->sfd = sfd ; 
    cInfo->is_ha = acInfo->is_ha ; 
    if ( cInfo->is_ha )
    {
      if ( gInfo->dInfo->cIhb[1] )
      {
        ChannInfo *ch ; 
        if (!(ch = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,194),sizeof(ChannInfo))) )
        {
          TRACE(1," failed to allocate channel struct of size %lu.\n",sizeof(ChannInfo));
          break ; 
        }
        memset(ch, 0, sizeof(ChannInfo)) ; 
        pthread_mutex_init(ch->lock,NULL) ; 
        pthread_cond_init (ch->cond,NULL) ; 
        cInfo->channel = ch ; 
      }
      else
        gInfo->dInfo->cIhb[1] = cInfo ; 
    }
    else
    {
      gInfo->dInfo->cIrm = cInfo ; 
      gInfo->dInfo->etime[0] = su_sysTime() + (gInfo->config->StartupMode==CIP_STARTUP_STAND_ALONE ? 10 : gInfo->haTimeOut[0]) ; 
    }
    ok++ ; 
    if ( cip_set_buffers(gInfo, cInfo) < 0 )
      break ; 
    ok++ ; 

    cInfo->next_r_time  = su_sysTime() + gInfo->hbTimeOut ; 

    place++ ; 
    if ( cip_set_local_endpoint(cInfo) == -1 )
      break ; 

    place++ ; 
    if ( cip_set_remote_endpoint(cInfo) == -1 )
      break ; 

    cInfo->sock_af = cInfo->lcl_sa->sa_family ;
  
    place++ ; 

    cip_update_conn_list(gInfo, cInfo, 1) ;  
    DBG_PRT(printf(" cip_build_new_incoming_conn: con added , gInfo->cipInfo->nConnsInProg=%d\n",gInfo->cipInfo->nConnsInProg)) ; 
    ok++ ; 

    gInfo->cipInfo->fds[cInfo->ind].fd = cInfo->sfd ; 
    gInfo->cipInfo->fds[cInfo->ind].events = POLLIN ; 

    inet_ntop(cInfo->sock_af,cInfo->rmt_addr.bytes,cInfo->req_addr,HOSTNAME_STR_LEN) ;
    cInfo->req_port = cInfo->rmt_port ; 

    cInfo->io_state = CIP_IO_STATE_SSL_HS ; 
    ok++ ; 
  } while ( 0 ) ; 
  DBG_PRT(printf("cip_build_new_incoming_conn: rc= %d , place= %d\n",ok,place)) ; 
  if ( ok > 4 ) 
    return 0 ; 
  if ( ok > 3 ) 
    cip_update_conn_list(gInfo, cInfo, 0) ;  
  if ( ok > 2 ) 
  {
    ism_common_free(ism_memory_store_misc,cInfo->rdInfo.buffer) ; 
    ism_common_free(ism_memory_store_misc,cInfo->wrInfo.buffer) ; 
  }
  if ( ok > 1 ) 
  {
    if ( cInfo->channel ) ism_common_free(ism_memory_store_misc,cInfo->channel) ; 
    else if ( cInfo == gInfo->dInfo->cIhb[1] ) gInfo->dInfo->cIhb[1] = NULL ; 
    else if ( cInfo == gInfo->dInfo->cIrm    ) gInfo->dInfo->cIrm    = NULL ; 
  }
  if ( ok > 0 ) 
    ism_common_free(ism_memory_store_misc,cInfo) ; 

  if ( sfd > 0 ) close(sfd) ; 

  return -1 ; 
}

//--------------------------------------------

int cip_handle_async_connect(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  if ( (gInfo->cipInfo->fds[cInfo->ind].revents & (POLLIN|POLLOUT)) ) 
  {
    socklen_t err , err_len; 
    char tmpstr[4] ; 
    errInfo ei[1] ; 
    ei->errLen = sizeof(ei->errMsg) ; 
    err_len = sizeof(err) ; 
    err = 0 ; 
    if ( getsockopt(cInfo->sfd,SOL_SOCKET,SO_ERROR,&err,&err_len) != 0 )
      err = errno ; 
    if ( err != 0 )
    {
      DBG_PRT(printf(" ConnFailed: err= %d\n",err)) ; 
      snprintf(gInfo->cipInfo->ev_msg,EV_MSG_SIZE," connect() failed: err= %d (%s)\n",err,strerror(err)) ; 
      DBG_PRINT("_dbg_: %s\n",gInfo->cipInfo->ev_msg);
      cip_conn_failed(gInfo,cInfo, CH_EV_CON_BROKE) ; 
      return -1 ; 
    }
    if ( read(cInfo->sfd, tmpstr, 0) != 0 )
    {
      DBG_PRT(printf(" ConnFailed: read 0 -> rc!=0\n")) ; 
      su_strlcpy(gInfo->cipInfo->ev_msg,"read 0 -> rc!=0",EV_MSG_SIZE) ; 
      cip_conn_failed(gInfo,cInfo, CH_EV_CON_BROKE) ; 
      return -1 ; 
    }
  }
  else
  {
    return 0 ; 
  }

  cInfo->io_state = CIP_IO_STATE_SSL_HS ; 
  return 0 ; 
}

/******************************/
#if USE_IB
#define IB_BUFF_SIZE (1<<13)
int  cip_prepare_ib_stuff(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int rc , n, ps ; 
  int iok=0 ; 
  struct ibv_qp_init_attr init_qp_attr ; 
  struct ibv_recv_wr *wr_bad ; 
  struct ibv_sge     *sg ; 
  int nbuffs , bsize, ret , buffer_size ; 
  size_t mem_len ; 
  ioInfo *si ; 
  const char* methodName = "cip_prepare_ib_stuff";
  errInfo ei[1];
  ei->errLen = sizeof(ei->errMsg) ; 

  cInfo->use_ib = 1 ; 
  cInfo->is_ha  = 1 ; 
  do
  {
    cInfo->rdInfo.ifi = gInfo->haIf ;
    cInfo->wrInfo.ifi = gInfo->haIf ;
    cInfo->ibv_ctx = cInfo->cm_id->verbs ;
    cInfo->ib_pd = gInfo->haIf->ib_pd ; 
    iok++ ; // 1

    { // RECV side
      struct ibv_recv_wr *wr ; 
      si = &cInfo->rdInfo ; 
      si->reqlen = INT_SIZE ; 
      si->ibv_ctx = cInfo->ibv_ctx ; 
      if ( (si->ib_ch = Xibv_create_comp_channel(cInfo->ibv_ctx)) == NULL )
      {
        rc = errno ; 
        TRACE(1,"ibv_create_comp_channel failed. The error is %d (%s).\n",rc,strerror(rc) );
        break ; 
      }
      iok++ ; // 2
      if ( ha_set_nb(si->ib_ch->fd, ei, methodName) == -1 )
      {
        TRACE(1," failed to set si->ib_ch->fd to non-blocking, rc=%d (%s)\n",errno,strerror(errno)) ; 
        break;
      }
  
      ps = getpagesize() ; 
      if ( cInfo->channel && !cInfo->init_here )  // recv of bkup
      {
        buffer_size = gInfo->SockBuffSizes[1][1] ; 
        bsize = IB_BUFF_SIZE ; 
        nbuffs= buffer_size / bsize ; 
        nbuffs *= 2 ; 
      }
      else
      {
        bsize = 2*ps ; 
        nbuffs = 64 ; 
        buffer_size = bsize*nbuffs;
      }
      if ( nbuffs > si->ifi->ib_max_wr )
        nbuffs = si->ifi->ib_max_wr ;
      DBG_PRT(printf("_ibv_%d: nbuffs %d %d %d %d\n",__LINE__,nbuffs,buffer_size ,si->ifi->ib_mtu,bsize)) ; 
      TRACE(5,"_recv: nbuffs %d %d %d %d\n",nbuffs,buffer_size ,si->ifi->ib_mtu,bsize) ; 
  
      if ( (si->ib_cq = Xibv_create_cq(cInfo->ibv_ctx, nbuffs, si, si->ib_ch, 0)) == NULL )
      {
        rc = errno ; 
        TRACE(1,"ibv_create_cq failed. The error is %d (%s).\n",rc,strerror(rc) );
        break ; 
      }
      si->reg_len = nbuffs/2 ; 
      iok++ ; // 3
  
      for ( mem_len=nbuffs*bsize ; mem_len ; mem_len = (7*mem_len/8)/bsize*bsize )
      {
        if ( (rc=posix_memalign(&si->reg_mem,ps,mem_len)) )
        {
          TRACE(1," failed to allocate reg_mem of size %lu.\n",mem_len);
          continue ; 
        }
        if ( (si->ib_mr = Xibv_reg_mr(cInfo->ib_pd, si->reg_mem, mem_len, (IBV_ACCESS_LOCAL_WRITE))) == NULL )
        {
          rc = errno ; 
          TRACE(1," ibv_reg_mr failed for size %lu, rc=%d (%s).\n",mem_len,rc,strerror(rc)) ; 
          ism_common_free(ism_memory_store_misc,si->reg_mem) ; 
          si->reg_mem = NULL ; 
          continue ; 
        }
        break ; 
      }
      nbuffs = mem_len/bsize ; 
      if ( !nbuffs )
      {
        TRACE(1," failed to Allocate or register memory! (nbuffs=%d)\n",nbuffs);
        break ; 
      }
      iok++ ; // 4
  
      mem_len = nbuffs * (sizeof(struct ibv_recv_wr) + sizeof(struct ibv_sge)) ; 
      if ( (si->wr_mem = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,200),mem_len)) == NULL )
      {
        TRACE(1," failed to allocate wr_mem of size %lu.\n",mem_len);
        break ; 
      }
      iok++ ; // 5
      
      memset(si->wr_mem,0,mem_len) ; 
      wr = si->wr_mem ; 
      sg = (struct ibv_sge *)((uintptr_t)si->wr_mem + (nbuffs * sizeof(struct ibv_recv_wr))) ;
      for ( n = 0 ; n<nbuffs ; n++ , wr++ , sg++ )
      {
        wr->wr_id = n ; 
        wr->next = wr+1 ; 
        wr->sg_list = sg ; 
        wr->num_sge = 1 ; 
        sg->addr = (uintptr_t)si->reg_mem + (n * bsize) ; 
        sg->length = bsize-sizeof(void *) ; 
        *((void **)((uintptr_t)sg->addr+sg->length)) = wr ; 
        sg->lkey = si->ib_mr->lkey ; /* BEAM suppression: operating on NULL */
      }
      (--wr)->next = NULL ; 
      si->ib_bs = bsize-sizeof(void *) ;
    }

    { // SEND side
      struct ibv_send_wr *wr ; 
      si = &cInfo->wrInfo ; 
      if ( si->ifi->ib_iwarp )
      {
        si->send_flags[0] = IBV_SEND_SIGNALED ; 
        si->send_flags[1] = IBV_SEND_SIGNALED | IBV_SEND_INLINE ; 
      }
      else
      {
        si->send_flags[0] = 0 ; 
        si->send_flags[1] = IBV_SEND_INLINE ; 
      }
      si->ibv_ctx = cInfo->ibv_ctx ; 
      if ( (si->ib_ch = Xibv_create_comp_channel(cInfo->ibv_ctx)) == NULL )
      {
        rc = errno ; 
        TRACE(1,"ibv_create_comp_channel failed. The error is %d (%s).\n",rc,strerror(rc) );
        break ; 
      }
      iok++ ; // 6
      if ( ha_set_nb(si->ib_ch->fd, ei, methodName) == -1 )
      {
        TRACE(1," failed to set si->ib_ch->fd to non-blocking, rc=%d (%s)\n",errno,strerror(errno)) ; 
        break;
      }

      ps = getpagesize() ; 
      if ( cInfo->channel && cInfo->init_here )  // send of primary
      {
        buffer_size = gInfo->SockBuffSizes[1][0] ; 
        bsize = IB_BUFF_SIZE ; 
        nbuffs= buffer_size / bsize ; 
      }
      else
      {
        bsize = 2*ps ; 
        nbuffs = 64 ; 
        buffer_size = bsize*nbuffs;
      }
      if ( nbuffs > si->ifi->ib_max_wr )
        nbuffs = si->ifi->ib_max_wr ;
      DBG_PRT(printf("_ibv_%d: nbuffs %d %d %d %d\n",__LINE__,nbuffs,buffer_size ,si->ifi->ib_mtu,bsize)) ; 
      TRACE(5,"_send: nbuffs %d %d %d %d\n",nbuffs,buffer_size ,si->ifi->ib_mtu,bsize) ; 
  
      if ( (si->ib_cq = Xibv_create_cq(cInfo->ibv_ctx, nbuffs, cInfo, si->ib_ch, 0)) == NULL )
      {
        rc = errno ; 
        TRACE(1,"ibv_create_cq failed. The error is %d (%s).\n",rc,strerror(rc) );
        break ; 
      }
      si->reg_len = nbuffs/2 ; 
      iok++ ; // 7
  
      for ( mem_len=nbuffs*bsize ; mem_len ; mem_len = (7*mem_len/8)/bsize*bsize )
      {
        if ( (rc=posix_memalign(&si->reg_mem,ps,mem_len)) )
        {
          TRACE(1," failed to allocate reg_mem of size %lu.\n",mem_len);
          continue ; 
        }
        if ( (si->ib_mr = Xibv_reg_mr(cInfo->ib_pd, si->reg_mem, mem_len, (IBV_ACCESS_LOCAL_WRITE))) == NULL )
        {
          rc = errno ; 
          TRACE(1," ibv_reg_mr failed for size %lu, rc=%d (%s).\n",mem_len,rc,strerror(rc)) ; 
          ism_common_free(ism_memory_store_misc,si->reg_mem) ; 
          si->reg_mem = NULL ; 
          continue ; 
        }
        break ; 
      }
      nbuffs = mem_len/bsize ; 
      if ( !nbuffs )
      {
        TRACE(1," failed to allocate or register memory! (nbuffs=%d)\n",nbuffs);
        break ; 
      }
      iok++ ; // 8
  
      mem_len = nbuffs * (sizeof(struct ibv_send_wr) + sizeof(struct ibv_sge)) ; 
      if ( (si->wr_mem = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,202),mem_len)) == NULL )
      {
        TRACE(1," failed to allocate wr_mem of size %lu.\n",mem_len);
        break ; 
      }
      iok++ ; // 9
  
      memset(si->wr_mem,0,mem_len) ; 
      wr = si->wr_mem ; 
      si->wr1st = wr ; 
      sg = (struct ibv_sge *)((uintptr_t)si->wr_mem + (nbuffs * sizeof(struct ibv_send_wr))) ;
      for ( n = 0 ; n<nbuffs ; n++ , wr++ , sg++ )
      {
        wr->wr_id = n ; 
        wr->next = wr+1 ; 
        wr->sg_list = sg ; 
        wr->num_sge = 1 ; 
        wr->opcode = IBV_WR_SEND ; 
        wr->send_flags = 0 ; 
        wr->imm_data = 0 ; 
        sg->addr = (uintptr_t)si->reg_mem + (n * bsize) ; 
        sg->length = bsize-sizeof(void *) ; 
        *((void **)((uintptr_t)sg->addr+sg->length)) = wr ; 
        sg->lkey = si->ib_mr->lkey ; /* BEAM suppression: operating on NULL */
      }
      (--wr)->next = NULL ; 
      si->ib_bs = bsize-sizeof(void *) ;
    }

   #if 0
  //for ( init_qp_attr.cap.max_inline_data=(1<<20) ; init_qp_attr.cap.max_inline_data>0 ; init_qp_attr.cap.max_inline_data/=2 )
    for ( buffer_size=(1<<12) ; buffer_size>0 ; buffer_size=buffer_size/2 )
    {
    memset(&init_qp_attr, 0, sizeof(struct ibv_qp_init_attr)) ; 
    init_qp_attr.qp_context = cInfo ; 
    init_qp_attr.send_cq = cInfo->wrInfo.ib_cq ;
    init_qp_attr.recv_cq = cInfo->rdInfo.ib_cq ; 
    init_qp_attr.cap.max_send_wr = cInfo->wrInfo.reg_len*2 ; 
    init_qp_attr.cap.max_recv_wr = cInfo->rdInfo.reg_len*2 ; 
    init_qp_attr.cap.max_send_sge = 1;
    init_qp_attr.cap.max_recv_sge = 1;
    init_qp_attr.cap.max_inline_data = buffer_size ; 
    init_qp_attr.qp_type = IBV_QPT_RC;
    init_qp_attr.sq_sig_all = 1;
      struct ibv_qp *tt = Xibv_create_qp(cInfo->ib_pd, &init_qp_attr) ; 
      if ( tt )
      {
        Xibv_destroy_qp(tt) ; 
        break ; 
      }
    }
    si->ifi->ib_max_inline = buffer_size ; 
    TRACE(5,"Detected QP max_inline_data = %u\n",si->ifi->ib_max_inline);
   #endif
    memset(&init_qp_attr, 0, sizeof(struct ibv_qp_init_attr)) ; 
    init_qp_attr.qp_context = cInfo ; 
    init_qp_attr.send_cq = cInfo->wrInfo.ib_cq ;
    init_qp_attr.recv_cq = cInfo->rdInfo.ib_cq ; 
    init_qp_attr.cap.max_send_wr = cInfo->wrInfo.reg_len*2 ; 
    init_qp_attr.cap.max_recv_wr = cInfo->rdInfo.reg_len*2 ; 
    init_qp_attr.cap.max_send_sge = 1;
    init_qp_attr.cap.max_recv_sge = 1;
    init_qp_attr.cap.max_inline_data = si->ifi->ib_max_inline ; 
    init_qp_attr.qp_type = IBV_QPT_RC;
    init_qp_attr.sq_sig_all = 1;
    if ( (ret = Xrdma_create_qp(cInfo->cm_id, cInfo->ib_pd, &init_qp_attr)) )
    {
      rc = errno ; 
      TRACE(1,"ibv_create_qp failed. The error is %d (%s).\n",rc,strerror(rc) );
      break ; 
    }
    si->ifi->ib_max_inline = init_qp_attr.cap.max_inline_data ; 
    cInfo->ib_qp = cInfo->cm_id->qp ; 
    cInfo->rdInfo.ib_qp = cInfo->cm_id->qp ; 
    cInfo->wrInfo.ib_qp = cInfo->cm_id->qp ; 
    iok++ ; // 10

    {
      struct ibv_recv_wr *wr, *nwr ; 
      si = &cInfo->rdInfo ; 
      for ( wr=si->wr_mem ; wr ; wr=nwr )
      {
        nwr = wr->next ; 
        wr->next = NULL ; 
        wr_bad = NULL ; 
        if ( (ret = ibv_post_recv(cInfo->ib_qp, wr, &wr_bad)) || wr_bad )
        {
          rc = ret ; 
          TRACE(1,"ibv_post_recv failed. The error is %d (%s).\n",rc,strerror(rc) );
          break ; 
        }
      }
    }

   #if CIP_DBG
    {
      struct ibv_qp_attr qp_attr ; 
      memset(&qp_attr, 0, sizeof(struct ibv_qp_attr)) ; 
      if ( (ret = Xibv_query_qp(si->ib_qp, &qp_attr, 0, &init_qp_attr)) )
      {
        DBG_PRT(printf("_ibv_%d: failed to query QP! (rc=%d) \n",__LINE__,ret));
      }
      DBG_PRT(printf("_ibv_%d: qpnum= %8x, qpkey= %8x %8x, sl= %x, nbuffs= %d\n",__LINE__,si->ib_qp->qp_num,si->ib_qp->handle,qp_attr.qkey,qp_attr.ah_attr.sl,nbuffs)) ; 
    }
   #endif
    
    iok++ ; // 11
  } while(0) ; 
  if ( iok >= 11 )
    return 0 ; 
  cip_destroy_ib_stuff(cInfo) ; 
  return -1 ; 
}

/******************************/

void  cip_destroy_ib_stuff(ConnInfoRec *cInfo)
{
  if ( cInfo->cm_id )
  {
    struct rdma_event_channel *ev_ch = cInfo->cm_id->channel ;
    if ( cInfo->cm_id->qp ) Xrdma_destroy_qp(cInfo->cm_id) ;
    Xrdma_destroy_id(cInfo->cm_id) ; 
    cInfo->cm_id = NULL ; 
    if ( cInfo->init_here && ev_ch ) Xrdma_destroy_event_channel(ev_ch) ;
  }

  if ( cInfo->wrInfo.wr_mem ) { ism_common_free(ism_memory_store_misc,cInfo->wrInfo.wr_mem) ; cInfo->wrInfo.wr_mem = NULL ; }
  if ( cInfo->wrInfo.ib_mr ) { Xibv_dereg_mr(cInfo->wrInfo.ib_mr) ; cInfo->wrInfo.ib_mr = NULL ; }
  if ( cInfo->wrInfo.reg_mem ) { ism_common_free(ism_memory_store_misc,cInfo->wrInfo.reg_mem) ; cInfo->wrInfo.reg_mem = NULL ; }
  if ( cInfo->wrInfo.ib_cq ) { Xibv_destroy_cq(cInfo->wrInfo.ib_cq) ; cInfo->wrInfo.ib_cq = NULL ; }
  if ( cInfo->wrInfo.ib_ch ) { Xibv_destroy_comp_channel(cInfo->wrInfo.ib_ch) ; cInfo->wrInfo.ib_ch = NULL ; }

  if ( cInfo->rdInfo.wr_mem ) { ism_common_free(ism_memory_store_misc,cInfo->rdInfo.wr_mem) ; cInfo->rdInfo.wr_mem = NULL ; }
  if ( cInfo->rdInfo.ib_mr ) { Xibv_dereg_mr(cInfo->rdInfo.ib_mr) ; cInfo->rdInfo.ib_mr = NULL ; }
  if ( cInfo->rdInfo.reg_mem ) { ism_common_free(ism_memory_store_misc,cInfo->rdInfo.reg_mem) ; cInfo->rdInfo.reg_mem = NULL ; }
  if ( cInfo->rdInfo.ib_cq ) { Xibv_destroy_cq(cInfo->rdInfo.ib_cq) ; cInfo->rdInfo.ib_cq = NULL ; }
  if ( cInfo->rdInfo.ib_ch ) { Xibv_destroy_comp_channel(cInfo->rdInfo.ib_ch) ; cInfo->rdInfo.ib_ch = NULL ; }

  if ( cInfo->ib_pd                             &&
       cInfo->ib_pd != cInfo->wrInfo.ifi->ib_pd &&
       cInfo->ib_pd != cInfo->rdInfo.ifi->ib_pd )
  {
    Xibv_dealloc_pd(cInfo->ib_pd) ; 
    cInfo->ib_pd = NULL ; 
  }
}

/******************************/

int  cip_handle_conn_req_ev(haGlobalInfo *gInfo, ConnInfoRec *acInfo)
{
  int rc ; 
  int iok=0 ; 
  struct rdma_cm_event *cm_ev ; 
  ConnInfoRec *cInfo=NULL ; 
  const char* myName = "cip_handle_conn_req_ev";

  if ( (rc = Xrdma_get_cm_event(acInfo->cm_id->channel , &cm_ev)) )
  {
    rc = errno ; 
    if ( !eWB(rc) )
    {
      TRACE(1,"The TCP receiver call to the InfiniBand verbs API (rmda_get_cm_event) failed. The error is %d (%s).\n",rc,strerror(rc) );
      return -1 ; 
    }
    return 0 ; 
  }
  if (!cm_ev )
  {
    TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) return a value of NULL for cm_ev.\n");
    return -1 ; 
  }
 #if 1
  if ( cm_ev->event == RDMA_CM_EVENT_ESTABLISHED )
  {
    cInfo = (ConnInfoRec *)cm_ev->id->context ; 
    if ( cInfo )
      cInfo->cm_ev = cm_ev ; 
    else
    {
      DBG_PRT(printf(" cip_handle_conn_req_ev: cInfo = (ConnInfoRec *)cm_ev->id->context is NULL\n")) ; 
      Xrdma_ack_cm_event(cm_ev) ; 
    }
    return 0 ; 
  }
  if ( cm_ev->event == RDMA_CM_EVENT_DISCONNECTED )
  {
    cInfo = (ConnInfoRec *)cm_ev->id->context ; 
    if ( cInfo )
    {
      if ( cInfo->channel ) pthread_mutex_lock(cInfo->channel->lock) ; 
      if (!cInfo->is_invalid )
      {
        cInfo->is_invalid |= C_INVALID ; 
        TRACE(5,"HA Connection marked as invalid: %s \n",cInfo->req_addr);
      }
      if ( cInfo->channel ) pthread_mutex_unlock(cInfo->channel->lock) ; 
    }
    Xrdma_ack_cm_event(cm_ev) ; 
    return 0 ; 
  }
 #endif
  if ( cm_ev->event != RDMA_CM_EVENT_CONNECT_REQUEST )
  {
    TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) returned an event of type %s status %d, while expecting an event of type %s.\n", 
           rdma_ev_desc(cm_ev->event),cm_ev->status,rdma_ev_desc(RDMA_CM_EVENT_CONNECT_REQUEST)) ; 
    Xrdma_ack_cm_event(cm_ev) ; 
    return 0 ; 
  }

  do
  {
    errInfo ei[1];
    ei->errLen = sizeof(ei->errMsg) ; 
    int ret ; 

    if ( acInfo->is_ha && !((gInfo->dInfo->state&ACK_MSG_SENT) && gInfo->dInfo->ack_msg[0].role_local == ISM_HA_ROLE_STANDBY) )
    {
      SA *sa = rdma_get_peer_addr(cm_ev->id) ; 
      char addr[ADDR_STR_LEN], port[8] ; 
      haGetNameInfo(sa, addr, ADDR_STR_LEN, port, 8, ei);
      TRACE(5," unexpected incoming conn on HA NIC (from: %s|%s)\n",addr,port) ; 
      break ; 
    }

    if (!acInfo->is_ha && (gInfo->dInfo->cIrm || gInfo->dInfo->cIhb[0]) )
    {
      SA *sa = rdma_get_peer_addr(cm_ev->id) ; 
      char addr[ADDR_STR_LEN], port[8] ; 
      haGetNameInfo(sa, addr, ADDR_STR_LEN, port, 8, ei);
      TRACE(5," unexpected incoming conn on ADM NIC (from: %s|%s)\n",addr,port) ; 
      break ; 
    }

   #if 0
    if ( !(ev_ch = Xrdma_create_event_channel()) )
    {
      rc = errno ; 
      TRACE(1,"_$s_$d: failed to create an RDMA event channel, error %d (%s)\n.",myName,__LINE__,rc,strerror(rc)) ; 
      break ; 
    }
    iok++ ; // 1
    if ( ha_set_nb(ev_ch->fd, ei, myName) == -1 )
    {
      break ; 
    }
   #else
    iok++ ; // 1
   #endif
    if ( (cInfo=(ConnInfoRec *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,207),sizeof(ConnInfoRec))) == NULL )
    {
      TRACE(1,"%s_%d: malloc failed for %lu bytes\n",myName,__LINE__,sizeof(ConnInfoRec)) ; 
      break ; 
    }
    iok++ ; // 2
    memset(cInfo,0,sizeof(ConnInfoRec)) ; 
    cInfo->use_ib = 1 ; 
    cInfo->cm_id = cm_ev->id ; 
    cInfo->cm_id->context = cInfo ; 
   #if 0
    cInfo->cm_id->channel = ev_ch ; 
   #endif
    cInfo->next_r_time  = su_sysTime() + gInfo->hbTimeOut ; 
    cInfo->sfd = cInfo->cm_id->channel->fd ; 

    if ( cip_set_local_endpoint(cInfo) == -1 )
      break ; 

    if ( cip_set_remote_endpoint(cInfo) == -1 )
      break ; 

    cInfo->sock_af = cInfo->lcl_sa->sa_family ;

    cInfo->is_ha = acInfo->is_ha ; 
    if ( cInfo->is_ha )
    {
      if ( gInfo->dInfo->cIhb[1] )
      {
        ChannInfo *ch ; 
        if (!(ch = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,208),sizeof(ChannInfo))) )
        {
          TRACE(1," failed to allocate channel struct of size %lu.\n",sizeof(ChannInfo));
          break ; 
        }
        memset(ch, 0, sizeof(ChannInfo)) ; 
        pthread_mutex_init(ch->lock,NULL) ; 
        pthread_cond_init (ch->cond,NULL) ; 
        cInfo->channel = ch ; 
      }
      else
        gInfo->dInfo->cIhb[1] = cInfo ; 
    }

    if ( cip_prepare_ib_stuff(gInfo, cInfo) != 0 )
      break ; 
    iok++ ; // 3
    
    if ( (ret = Xrdma_accept(cInfo->cm_id, &cm_ev->param.conn)) )
    {
      rc = ret ; 
      TRACE(1," rdma_accept failed, error %d (%s)\n.",rc,strerror(rc)) ; 
      break ; 
    }
    cip_update_conn_list(gInfo, cInfo, 1) ;  
    DBG_PRT(printf(" cip_handle_conn_req_ev: con added , gInfo->cipInfo->nConnsInProg=%d\n",gInfo->cipInfo->nConnsInProg)) ; 
    iok++ ; // 4
  } while(0) ; 
  Xrdma_ack_cm_event(cm_ev) ; 
  if ( iok >= 4 )
  {
    cInfo->io_state = CIP_IB_STATE_CEST ; 

    gInfo->cipInfo->fds[cInfo->ind].fd = cInfo->sfd ; 
    gInfo->cipInfo->fds[cInfo->ind].events = POLLIN ; 
    return 1 ; 
  }
  if ( iok >= 3 )
    cip_destroy_ib_stuff(cInfo) ;
  else
    Xrdma_destroy_id(cm_ev->id) ; 
  if ( iok >= 2 )
    ism_common_free(ism_memory_store_misc,cInfo) ; 
 #if 0
  if ( iok >= 1 )
    Xrdma_destroy_event_channel(ev_ch) ; 
 #endif

  return -1 ; 
}

/******************************/

int  cip_handle_addr_res_ev(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int rc ; 
  struct rdma_cm_event *cm_ev ; 

  if ( (rc = Xrdma_get_cm_event(cInfo->cm_id->channel , &cm_ev)) )
  {
    rc = errno ; 
    if ( !eWB(rc) )
    {
      TRACE(1,"The TCP receiver call to the InfiniBand verbs API (rmda_get_cm_event) failed. The error is %d (%s).\n",rc,strerror(rc) );
      return -1 ; 
    }
    return 0 ; 
  }
  if (!cm_ev )
  {
    TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) return a value of NULL for cm_ev.\n");
    return -1 ; 
  }
  if ( cm_ev->event != RDMA_CM_EVENT_ADDR_RESOLVED )
  {
    TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) returned an event of type %s status %d, while expecting an event of type %s.\n", 
      rdma_ev_desc(cm_ev->event),cm_ev->status,rdma_ev_desc(RDMA_CM_EVENT_ADDR_RESOLVED));
    Xrdma_ack_cm_event(cm_ev) ; 
    return -1; 
  }

  if ( (rc = Xrdma_resolve_route(cInfo->cm_id , 16)) )
  {
    TRACE(1,"The TCP receiver call to the InfiniBand verbs API (rdma_resolve_route) failed. The error is %d (%s).\n",rc,strerror(rc) );
    Xrdma_ack_cm_event(cm_ev) ; 
    return -1 ; 
  }

  Xrdma_ack_cm_event(cm_ev) ; 
  cInfo->io_state = CIP_IB_STATE_ROUTE; 
  return 1 ; 

}

/******************************/

int  cip_handle_rout_res_ev(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int rc , iok=0 ; 
  struct rdma_cm_event *cm_ev ; 
  struct rdma_conn_param cp ; 

  if ( (rc = Xrdma_get_cm_event(cInfo->cm_id->channel , &cm_ev)) )
  {
    rc = errno ; 
    if ( !eWB(rc) )
    {
      TRACE(1,"The TCP receiver call to the InfiniBand verbs API (rmda_get_cm_event) failed. The error is %d (%s).\n",rc,strerror(rc) );
      return -1 ; 
    }
    return 0 ; 
  }
  if (!cm_ev )
  {
    TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) return a value of NULL for cm_ev.\n");
    return -1 ; 
  }
  if ( cm_ev->event != RDMA_CM_EVENT_ROUTE_RESOLVED )
  {
    TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) returned an event of type %s status %d, while expecting an event of type %s.\n", 
      rdma_ev_desc(cm_ev->event),cm_ev->status,rdma_ev_desc(RDMA_CM_EVENT_ROUTE_RESOLVED));
    Xrdma_ack_cm_event(cm_ev) ; 
    return -1; 
  }
  Xrdma_ack_cm_event(cm_ev) ; 

  DBG_PRT(printf("rdma_id_pkey: %d\n",cInfo->cm_id->route.addr.addr.ibaddr.pkey));
  do
  {
    if ( cip_set_local_endpoint(cInfo) == -1 )
      break ; 

    if ( cip_set_remote_endpoint(cInfo) == -1 )
      break ; 

    cInfo->sock_af = cInfo->lcl_sa->sa_family ;

    if ( cip_prepare_ib_stuff(gInfo, cInfo) != 0 )
      break ; 
    iok++ ; 

    memset(&cp,0,sizeof(cp)) ; 
    cp.retry_count = 7 ; 
    cp.rnr_retry_count = 7 ; 
//  cp.responder_resources = 1 ; 
//  cp.initiator_depth = 1 ;  
//  cp.retry_count = 10; 
    if ( (rc = Xrdma_connect(cInfo->cm_id ,&cp)) )
    {
      rc = errno ; 
      TRACE(1,"The TCP receiver call to the InfiniBand verbs API (rdma_connect) failed. The error is %d (%s).\n",rc,strerror(rc) );
      break ; 
    }
    iok++ ; 
  } while (0) ; 
  if ( iok >= 2 )
  {
    cInfo->io_state = CIP_IB_STATE_CEST ; 
    return 1 ; 
  }

  return -1 ; 
}

/******************************/

int  cip_handle_conn_est_ev(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int rc ; 
  struct rdma_cm_event *cm_ev ; 

  if ( cInfo->init_here )
  {
    if ( (rc = Xrdma_get_cm_event(cInfo->cm_id->channel , &cm_ev)) )
    {
      rc = errno ; 
      if ( !eWB(rc) )
      {
        TRACE(1,"The TCP receiver call to the InfiniBand verbs API (rmda_get_cm_event) failed. The error is %d (%s).\n",rc,strerror(rc) );
        return -1 ; 
      }
      return 0 ; 
    }
    if (!cm_ev )
    {
      TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) return a value of NULL for cm_ev.\n");
      return -1 ; 
    }
  }
  else
  {
    if (!cInfo->cm_ev )
      return 0 ; 
    cm_ev = cInfo->cm_ev ; 
    cInfo->cm_ev = NULL ; 
  }
  if ( cm_ev->event != RDMA_CM_EVENT_ESTABLISHED )
  {
    TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) returned an event of type %s status %d, while expecting an event of type %s.\n", 
      rdma_ev_desc(cm_ev->event),cm_ev->status,rdma_ev_desc(RDMA_CM_EVENT_ESTABLISHED));
    Xrdma_ack_cm_event(cm_ev) ; 
    return -1; 
  }
  Xrdma_ack_cm_event(cm_ev) ; 

  //if (!cInfo->channel )
  {
    size_t bs = 0x10000 ; 
    if (!(cInfo->wrInfo.buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,210),bs)) )
    {
      TRACE(1," failed to allocate send buffer of size %lu.\n",bs);
      return -1 ; 
    }
    cInfo->wrInfo.buflen = bs ; 
    cInfo->wrInfo.need_free = 1 ; 
  }

  if ( cInfo->init_here )
  {
    if ( cInfo->channel )
    {
      if ( cip_prepare_s_cfp_cid(gInfo, cInfo) == -1 )
        return -1 ; 
    //cInfo->state    = CIP_STATE_S_CFP_CID ; 
    }
    else
    {
    //cInfo->state = CIP_STATE_ESTABLISH ; 
      if ( cip_prepare_s_cfp_hbt(gInfo, cInfo) == -1 )
        return -1 ; 
    //cInfo->state    = CIP_STATE_S_CFP_HBT ; 
    }
    cInfo->io_state = CIP_IO_STATE_WRITE ; 
  }
  else
  {
    cInfo->io_state = CIP_IO_STATE_READ ; 
  }
  cInfo->state = CIP_STATE_ESTABLISH ; 
  if ( cip_conn_ready(gInfo, cInfo) == -1 )
    return -1 ; 

  return 1 ; 
}

/******************************/

int  cip_check_disconn_ev(haGlobalInfo *gInfo, ConnInfoRec *cInfo)
{
  int rc ; 
  struct rdma_cm_event *cm_ev ; 

  if ( (rc = Xrdma_get_cm_event(cInfo->cm_id->channel , &cm_ev)) )
  {
    rc = errno ; 
    if ( !eWB(rc) )
    {
      TRACE(1,"The TCP receiver call to the InfiniBand verbs API (rmda_get_cm_event) failed. The error is %d (%s).\n",rc,strerror(rc) );
      return -1 ; 
    }
    return 0 ; 
  }
  if (!cm_ev )
  {
    TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) return a value of NULL for cm_ev.\n");
    return -1 ; 
  }
  if ( cm_ev->event == RDMA_CM_EVENT_DISCONNECTED )
  {
    cInfo->is_invalid |= C_INVALID ; 
    TRACE(5,"HA Connection marked as invalid: %s \n",cInfo->req_addr);
    Xrdma_ack_cm_event(cm_ev) ; 
    return 1 ; 
  }
  else
  {
    TRACE(1,"The InfiniBand verbs API (rmda_get_cm_event) returned an event of type %s status %d, while expecting an event of type %s.\n", 
           rdma_ev_desc(cm_ev->event),cm_ev->status,rdma_ev_desc(RDMA_CM_EVENT_DISCONNECTED)) ; 
    Xrdma_ack_cm_event(cm_ev) ; 
    return 0 ; 
  }
}
#endif

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

void *ism_store_haRecvThread(void *arg, void * context, int value)
{
  int rc, rc1;
  char    *msg_buf ; 
  uint32_t msg_len ; 
  ChannInfo *ch = (ChannInfo *)arg ; 
  haGlobalInfo *gInfo=gInfo_;

  pthread_mutex_lock(gInfo->haLock) ; 
  ch->thUp = 1 ; 
  pthread_mutex_unlock(gInfo->haLock) ; 
  update_chnn_list(gInfo, ch, 1) ; 
  rc = rc1 = StoreRC_OK ; 
  for(;;)
  {
    if ( ch->closing  ||
        (rc = ism_storeHA_receiveMessage(ch, &msg_buf, &msg_len, 0)) != StoreRC_OK ||
        (rc = rc1 = ch->MessageReceived(ch, msg_buf, msg_len, ch->uCtx))   != StoreRC_OK ||
        (rc = ism_storeHA_returnBuffer(ch, msg_buf))                 != StoreRC_OK )
      break ; 
  }
  if ( rc != StoreRC_OK && rc1 != StoreRC_HA_CloseChannel )
    breakView(gInfo) ; 
  ch->ChannelClosed(ch, ch->uCtx) ; 
    
  pthread_mutex_lock(gInfo->haLock) ; 
  ch->thUp = 0 ; 
  pthread_mutex_unlock(gInfo->haLock) ; 

  update_chnn_list(gInfo, ch, 0);
  free_conn(ch->cInfo) ; 

  ism_common_detachThread(ch->thId) ; 
  pthread_mutex_destroy(ch->lock) ; 
  pthread_cond_destroy (ch->cond) ; 
  ism_common_free(ism_memory_store_misc,ch) ; 

  if ( rc1 == StoreRC_HA_StoreTerm )
    gInfo->goDown = 2 ; 
  else
  if ( rc1 == StoreRC_SystemError )
    gInfo->sbError = 1 ; 

  return NULL ;
}

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

void *ism_store_haThread(void *arg, void * context, int value)
{
  int pstate=0;
  int npf=0 ; 
  int nr , rc ; 
  uint64_t nBytes[2]={0,0}, nPacks[2]={0,0} ; 
  double nsTime=0e0 ; 
  ConnInfoRec *cInfo, *nextC ; 
  ConnInfoRec *req ; 
  haGlobalInfo *gInfo ;
  struct sigaction sigact ; 
  const char* myName = "ism_store_haThread";
  errInfo ei[1] ; 
  ei->errLen = sizeof(ei->errMsg) ; 

  memset(&sigact,0,sizeof(struct sigaction)) ; 
  sigact.sa_handler = SIG_IGN ; 
  sigaction(SIGPIPE,&sigact,NULL) ; 

  gInfo = (haGlobalInfo *)arg ;

  su_strlcpy(gInfo->cipInfo->ev_msg,"_!!!_",EV_MSG_SIZE) ; 
  gInfo->cipInfo->ev_dbg[0] = gInfo->cipInfo->ev_msg ; 

  gInfo->failedNIC = NULL ; 
  if ( gInfo->config->AutoConfig && cip_create_discovery_conns(gInfo) != 0 )
  {
    gInfo->cipInfo->chUP = 0 ; 
    TRACE(1,"_%s: failed to create the discovery sockets.\n",myName);
    goto exitThread ;
  }

  gInfo->dInfo->etime[1] = su_sysTime() + (gInfo->config->StartupMode==CIP_STARTUP_STAND_ALONE ? 10 : gInfo->haTimeOut[1]) ; 
  if ( cip_create_accept_conns(gInfo) != 0 )
  {
    gInfo->cipInfo->chUP = 0 ; 
    TRACE(1,"_%s: failed to create the accept sockets.\n",myName);
    goto exitThread ;
  }

  pthread_mutex_lock(gInfo->haLock) ; 
  gInfo->cipInfo->chUP = 1 ;
  pthread_mutex_unlock(gInfo->haLock) ; 
  TRACE(5, "The %s thread is started\n", myName);

  getMyRole() ; 
  getViewCount() ; 

  gInfo->dInfo->etime[0] = su_sysTime() + (gInfo->config->StartupMode==CIP_STARTUP_STAND_ALONE ? 10 : gInfo->haTimeOut[0]) ; 
  gInfo->dInfo->state = DSC_IS_ON ; 
  gInfo->dInfo->bad[0] = gInfo->dInfo->bad[1] = 0 ; 

  for(;;)
  {
    ism_common_going2work();
    if ( pstate != gInfo->dInfo->state )
    {
      TRACE(7,"HA Discovery state changed from %x to %x (%x)\n",pstate,gInfo->dInfo->state,pstate^gInfo->dInfo->state);
      pstate = gInfo->dInfo->state ; 
    }
    if ( (gInfo->dInfo->state & DSC_VU_SENT) )
    {
      if ( (gInfo->dInfo->state & DSC_HAVE_PAIR) )
      {
        int i,j,bad[2] ; 
        double ct = su_sysTime() ; 
        for ( i=0,j=0 ; i<2 ; i++ )
        {
          bad[i] = 0 ; 
          if ( !gInfo->dInfo->cIhb[i] )  bad[i] |= 1 ; 
          else
          if ( gInfo->dInfo->cIhb[i]->is_invalid ) bad[i] |= 2 ; 
          else
          if ( gInfo->dInfo->cIhb[i]->next_r_time < ct)
          {
            double ttt = gInfo->dInfo->cIhb[1-i] ? gInfo->dInfo->cIhb[1-i]->next_r_time : 0e0 ; 
            TRACE(1,"_hbt_ on %s NIC : next_r_time < ct (%g < %g), HBTO: %f, loops: %u %u, otherNIC %f \n",(i?"Replication":"Discovery"),
                  gInfo->dInfo->cIhb[i]->next_r_time,  ct, gInfo->hbTimeOut, gInfo->dInfo->cIhb[i]->noHBloops , gInfo->hbTOloops, ttt);
            if ( gInfo->dInfo->cIhb[i]->noHBloops > gInfo->hbTOloops )
              bad[i] |= 4 ; 
          }
          if ( viewBreak(gInfo) ) bad[i] |= 8 ; 
          if ( bad[i] != gInfo->dInfo->bad[i] ) j++ ; 
        }
        if ( bad[0]|bad[1] )  //  HBT or VIEW_BREAK!!
        {
          if ( j )
          {
            TRACE(5,"HA connectivity state changed: from %x to %x (%x) on Discovery NIC, from %x to %x (%x) on Replication NIC\n",
                     gInfo->dInfo->bad[0],bad[0],bad[0]^gInfo->dInfo->bad[0], gInfo->dInfo->bad[1],bad[1],bad[1]^gInfo->dInfo->bad[1]);
          }
          for ( i=0 ; i<2 ; i++ )
          {
            gInfo->dInfo->bad[i] = bad[i] ; 
            bad[i] = (bad[i]&7) ? 1 : 0 ; 
          }
          breakView(gInfo) ; 
          if (!bad[0] && !bad[1] )
          {
            cip_conn_failed(gInfo,gInfo->dInfo->cIhb[1], CH_EV_CON_BROKE) ; 
          }
          else
          if ( bad[0] && bad[1] )
          {
            if ( gInfo->myRole == ISM_HA_ROLE_PRIMARY )
            {
              // TODO Primary to Primary standalone
              cip_raise_view(gInfo, VIEW_CHANGE_TYPE_PRIM_SOLO) ; 
            }
            else
            if ( (gInfo->dInfo->state&TRM_MSG_RECV) || gInfo->sbError )
            {
              // TODO Standby to terminate
              cip_raise_view(gInfo, VIEW_CHANGE_TYPE_BKUP_ERR) ;
            }
            else
            {
              // TODO Standby to Primary standalone
              cip_raise_view(gInfo, VIEW_CHANGE_TYPE_BKUP_PRIM) ; 
            }
          }
          else
          if ( gInfo->myRole == ISM_HA_ROLE_PRIMARY )
          {
            if (!(gInfo->dInfo->state&TRM_MSG_SENT) ) 
            {
              cInfo = gInfo->dInfo->cIhb[bad[0]] ; 
              cip_prepare_s_cfp_trm(gInfo, cInfo) ;
            //cInfo->state    = CIP_STATE_S_CFP_HBT ; 
              cInfo->io_state = CIP_IO_STATE_WRITE ; 
              gInfo->cipInfo->fds[cInfo->ind].events = POLLOUT ; 
            }
            else
            if ( (gInfo->dInfo->state&ACK_TRM_RECV) ) 
            {
              // TODO Primary to Primary standalone
              cip_raise_view(gInfo, VIEW_CHANGE_TYPE_PRIM_SOLO) ; 
            }
          }
          else
          if ( gInfo->dInfo->state&ACK_TRM_SENT ) 
          {
            su_sleep(16,SU_MIL) ; 
            // TODO Standby to terminate
            cip_raise_view(gInfo, VIEW_CHANGE_TYPE_BKUP_ERR) ; 
          }
        }
        else
        {
          double dt ; 
          if ( gInfo->dInfo->cIhb[0]->next_s_time < ct )
          {
            cInfo = gInfo->dInfo->cIhb[0] ; 
            cip_prepare_s_cfp_hbt(gInfo, cInfo) ;
          //cInfo->state    = CIP_STATE_S_CFP_HBT ; 
            cInfo->io_state = CIP_IO_STATE_WRITE ; 
            gInfo->cipInfo->fds[cInfo->ind].events = POLLOUT ; 
          }
          if ( gInfo->dInfo->cIhb[1]->next_s_time < ct )
          {
            cInfo = gInfo->dInfo->cIhb[1] ; 
            cip_prepare_s_cfp_hbt(gInfo, cInfo) ;
          //cInfo->state    = CIP_STATE_S_CFP_HBT ; 
            cInfo->io_state = CIP_IO_STATE_WRITE ; 
            gInfo->cipInfo->fds[cInfo->ind].events = POLLOUT ; 
          }
          if (!nsTime ) nsTime = su_sysTime() + 1e1 ; 
          else
          if ( (dt = su_sysTime() - nsTime) > 0e0 )
          {
            uint64_t nB[2]={0,0}, nP[2]={0,0} ; 
            ChannInfo *ch ; 
            pthread_mutex_lock(gInfo->haLock);
            for ( ch=gInfo->chnHead ; ch ; ch = ch->next )
            {
              if ( ch->cInfo )
              {
                nB[0] += ch->cInfo->rdInfo.nBytes ; 
                nB[1] += ch->cInfo->wrInfo.nBytes ; 
                nP[0] += ch->cInfo->rdInfo.nPacks ; 
                nP[1] += ch->cInfo->wrInfo.nPacks ; 
              }
            }
            pthread_mutex_unlock(gInfo->haLock);
            nB[0] -= nBytes[0] ; 
            nB[1] -= nBytes[1] ; 
            nP[0] -= nPacks[0] ; 
            nP[1] -= nPacks[1] ; 
            dt += 1e1 ; 
         //   printf("_haLayer: Network stats: read => %.1f MBytes/sec, %.0f Msgs/sec ; write => %.1f MBytes/sec, %.0f Msgs/sec \n",
         //          (double)nB[0]/dt/(double)(1<<20),(double)nP[0]/dt,(double)nB[1]/dt/(double)(1<<20),(double)nP[1]/dt);
            nBytes[0]  += nB[0]; 
            nBytes[1]  += nB[1]; 
            nPacks[0]  += nP[0]; 
            nPacks[1]  += nP[1]; 
            nsTime = su_sysTime() + 1e1 ;
          }
        }
      }
    }
    else
    {
      if ( (gInfo->dInfo->state & DSC_RESTART) )
      {
        cip_restart_discovery(gInfo) ; 
        continue ; 
      }
      if ( !gInfo->dInfo->cIlc && !(gInfo->dInfo->state & REQ_MSG_RECV) )
      {
        if ( (!(gInfo->dInfo->state & DSC_WORK_SOLO) || gInfo->dInfo->etime[0] < su_sysTime()) )
        {
          if (!gInfo->ipHead )
          {
            buildIpList(1, 1) ; 
            su_sleep(100,SU_MIL) ; 
          }
          if ( gInfo->ipHead )
          {
            gInfo->dInfo->etime[0] = su_sysTime() + (gInfo->config->StartupMode==CIP_STARTUP_STAND_ALONE ? 10 : gInfo->haTimeOut[0]) ; 
            gInfo->dInfo->cIlc = cip_prepare_conn_req(gInfo, 0, NULL) ; 
          }
        }
      }
      else
      if ( !gInfo->dInfo->cIhb[0] )
      {
        if ( (gInfo->dInfo->state & REQ_MSG_RECV) && (gInfo->dInfo->state & REQ_MSG_SENT) )
        {
          if ( (gInfo->dInfo->state&REQ_SENT_LCL) && 
              !(gInfo->dInfo->state&REQ_RECV_RMT)  ) 
            gInfo->dInfo->cIhb[0] = gInfo->dInfo->cIlc ; 
          else
          if (!(gInfo->dInfo->state&REQ_SENT_LCL) && 
               (gInfo->dInfo->state&REQ_RECV_RMT)  ) 
            gInfo->dInfo->cIhb[0] = gInfo->dInfo->cIrm ; 
          else
          {
            int dif = memcmp(gInfo->dInfo->req_msg[0].source_id, gInfo->dInfo->req_msg[1].source_id, sizeof(ismStore_HANodeID_t)) ; 
            if ( dif < 0 )
              gInfo->dInfo->cIhb[0] = gInfo->dInfo->cIrm ; 
            else
            if ( dif > 0 )
              gInfo->dInfo->cIhb[0] = gInfo->dInfo->cIlc ; 
            else
            {
              cip_restart_discovery(gInfo) ; 
              continue ; 
            }
          }
          if ( !gInfo->dInfo->cIhb[0] )
          {
            cip_restart_discovery(gInfo) ; 
            continue ; 
          }
          DBG_PRINT("_dbg_%d: gInfo->dInfo->cIhb[0] set to %s\n",__LINE__,gInfo->dInfo->cIhb[0]->req_addr);
          cip_prepare_s_cfp_res(gInfo, gInfo->dInfo->cIhb[0]) ; 
        }
      }
      else
      if (!(gInfo->dInfo->state&ACK_MSG_SENT) && 
           (gInfo->dInfo->state&RES_MSG_SENT) && 
           (gInfo->dInfo->state&RES_MSG_RECV)  ) 
      {
        if ( gInfo->dInfo->cIhb[0] != gInfo->dInfo->cIlc && gInfo->dInfo->cIlc )
          cip_conn_failed(gInfo,gInfo->dInfo->cIlc, CH_EV_CON_BROKE) ; 
        if ( gInfo->dInfo->cIhb[0] != gInfo->dInfo->cIrm && gInfo->dInfo->cIrm )
          cip_conn_failed(gInfo,gInfo->dInfo->cIrm, CH_EV_CON_BROKE) ; 
        cip_prepare_s_cfp_ack(gInfo, gInfo->dInfo->cIhb[0]) ; 
      }
      else
      if ( (gInfo->dInfo->state&ACK_MSG_SENT) && 
           (gInfo->dInfo->state&ACK_MSG_RECV) && 
          !(gInfo->dInfo->state&DSC_VU_SENT )  ) 
      {
        if (!(gInfo->dInfo->state & DSC_HAVE_PAIR) )
        {
          if ( gInfo->dInfo->ack_msg[0].conn_rejected || gInfo->dInfo->ack_msg[1].conn_rejected )
            cip_raise_view(gInfo, VIEW_CHANGE_TYPE_DISC) ; 
          else
          {
            gInfo->dInfo->state |= DSC_HAVE_PAIR ;
            gInfo->dInfo->state &=~DSC_WORK_SOLO ; 
            if (gInfo->config->AutoConfig )
            {
              ipInfo *ip ; 
              while( gInfo->ipHead )
              {
                ip = gInfo->ipHead ; 
                gInfo->ipHead = ip->next ; 
                ism_common_free(ism_memory_store_misc,ip) ; 
              }
              gInfo->ipCur = NULL ; 
            }
          }
        }
        else
        if ( !gInfo->dInfo->cIhb[1] )
        {
          if ( gInfo->dInfo->ack_msg[0].role_local == ISM_HA_ROLE_PRIMARY )
            gInfo->dInfo->cIhb[1] = cip_prepare_conn_req(gInfo, 1, NULL) ; 
        }
        else
        if ( gInfo->dInfo->cIhb[1]->state == CIP_STATE_ESTABLISH ||
             gInfo->dInfo->cIhb[1]->state == CIP_STATE_S_CFP_HBT  )
        {
          cip_raise_view(gInfo, VIEW_CHANGE_TYPE_DISC) ; 
        }
      }

      if ( (gInfo->dInfo->state & DSC_SB_TERM) )
      {
        cip_raise_view(gInfo, VIEW_CHANGE_TYPE_BKUP_ERR) ; 
      }
      else
      if ( (gInfo->dInfo->state & DSC_IS_ON) )
      {
        if (!gInfo->viewCount && gInfo->dInfo->etime[1] < su_sysTime() )
        {
          gInfo->dInfo->state |= DSC_TIMEOUT ; 
          cip_raise_view(gInfo, VIEW_CHANGE_TYPE_DISC) ; 
        }
        else
        if ( gInfo->dInfo->etime[0] < su_sysTime() )
        {
          cip_restart_discovery(gInfo) ; 
        }
      }
    }


    pthread_mutex_lock(gInfo->haLock) ; 
    if ( (req = gInfo->connReqQ) )
      gInfo->connReqQ = req->next ; 
    pthread_mutex_unlock(gInfo->haLock) ; 
    if ( req )
    {
      req->next = NULL ; 
      if ( cip_handle_conn_req(gInfo,req) < 0 )
      {
        if ( gInfo->dInfo->cIlc == req )
             gInfo->dInfo->cIlc = NULL ; 
        if ( gInfo->dInfo->cIhb[1] == req )
             gInfo->dInfo->cIhb[1] = NULL ; 
      }
    }

    nr = poll(gInfo->cipInfo->fds, gInfo->cipInfo->nfds, POLL_TO) ; 
    if ( gInfo->goDown )
      break ;  
    if ( nr < 0 )
    {
      if ( (rc = errno ) != EINTR )
      {
        if ( !(npf&0x3ff) )
        {
          TRACE(1,"_%s: poll failed with error %d (%s)\n",myName,errno,strerror(errno)) ; 
        }
        npf++ ; 
      }
    }

    pthread_mutex_lock(gInfo->haLock) ; 
    if ( gInfo->config->Group_ )
    {
      gInfo->config->Group = gInfo->config->Group_ ; 
      gInfo->config->Group_ = NULL ; 
      gInfo->mcInfo->grp_len = su_strlen(gInfo->config->Group) ; 
      gInfo->config->gUpd[0]++ ; 
    }
    pthread_mutex_unlock(gInfo->haLock) ; 
      
    if ( gInfo->config->AutoConfig && cip_handle_discovery_conns(gInfo) != 0 )
    {
      TRACE(1,"_%s: cip_handle_discovery_conns failed.\n",myName) ; 
    }

    for ( cInfo=gInfo->cipInfo->firstConnInProg ; cInfo ; cInfo=nextC )
    {
      nextC = cInfo->next ; 
      switch (cInfo->io_state)
      {
        case CIP_IO_STATE_ACC :
          if ( gInfo->cipInfo->fds[cInfo->ind].revents & POLLIN ) 
          {
            DBG_PRT(printf(" CIP_IO_STATE_ACC... %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
            cip_build_new_incoming_conn(gInfo, cInfo) ;
          }
          break ; 
        case CIP_IO_STATE_CONN :
          DBG_PRT(printf(" CIP_IO_STATE_CONN.. %s|%d\n",cInfo->req_addr,cInfo->req_port)) ; 
          cip_handle_async_connect(gInfo, cInfo) ;  // failures are handled there
          break ; 
        case CIP_IO_STATE_SSL_HS :
          DBG_PRT(printf(" CIP_IO_STATE_SSL_HS.. %s|%d\n",cInfo->req_addr,cInfo->req_port)) ; 
          rc = cip_ssl_handshake(gInfo,cInfo) ; 
          if ( rc > 0 )
          {
            if ( cInfo->init_here )
            {
              rc = cip_handle_conn_est(gInfo, cInfo) ;
            }
            else
            {
              cInfo->io_state = CIP_IO_STATE_READ ; 
              cInfo->state = CIP_STATE_ESTABLISH ;
              gInfo->cipInfo->fds[cInfo->ind].events = POLLIN ; 
          
              rc = cip_conn_ready(gInfo, cInfo) ;
            }
            if ( rc == -1 )
              cip_conn_failed(gInfo,cInfo, CH_EV_CON_BROKE) ; 
          }
          else
          if ( rc < 0 )
          {
            cip_conn_failed(gInfo,cInfo, CH_EV_SSL_ERROR) ; 
            TRACE(5,"cip_ssl_handshake() failed for conn |%s|\n",cInfo->req_addr);
            if (0 && !(gInfo->dInfo->state&DSC_WORK_SOLO) ) // This "feature" is disabled for now
            {
              gInfo->dInfo->state |= DSC_SSL_ERR ;
              cip_raise_view(gInfo, VIEW_CHANGE_TYPE_DISC) ; 
            }
          }
          break ; 
        case CIP_IO_STATE_WRITE :
          DBG_PRT(printf(" CIP_IO_STATE_WRITE.. %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
          if ( cInfo->use_ib || (gInfo->cipInfo->fds[cInfo->ind].revents & POLLOUT) ) 
          {
            rc = conn_write(cInfo) ; 
            if ( rc > 0 )
            {
              DBG_PRT(printf(" write ok , rc=%d %d %d\n",rc,cInfo->wrInfo.reqlen,cInfo->wrInfo.offset)) ; 
              cInfo->wrInfo.offset += rc ; 
              if ( cInfo->wrInfo.offset == cInfo->wrInfo.reqlen )
              {
                DBG_PRINT("_dbg_%d: SENT msg %d on conn %s\n",__LINE__,((haConAllMsg *)cInfo->wrInfo.buffer)->com.msg_type,cInfo->req_addr) ; 
                if ( cInfo->channel )
                {
                  ChannInfo *ch = cInfo->channel ; 
                  ch->cInfo = cInfo ;  
                  cip_update_conn_list(gInfo, cInfo, 0) ;  
                  ha_raise_event(cInfo, CH_EV_CON_READY) ; 
                  break ; 
                }
                gInfo->dInfo->state |= gInfo->dInfo->sendBit ; 
                gInfo->dInfo->sendBit = 0 ; 
                gInfo->cipInfo->fds[cInfo->ind].events = POLLIN ;  
                cInfo->io_state = CIP_IO_STATE_READ ; 
                cInfo->next_s_time = su_sysTime() + gInfo->hbInterval ; 
                TRACE(DEBUG_LEVEL,"_dbg_hbt: Setting next_s_time to %g for conn %s\n",cInfo->next_s_time,cInfo->req_addr);
              }
              else
                cInfo->wrInfo.bptr += rc ; 
            }
            else
            if ( !cInfo->use_ib || rc < 0 )
            {
              DBG_PRT(printf(" write failed, rc=%d %d %d\n",rc,cInfo->wrInfo.reqlen,cInfo->wrInfo.offset)) ; 
              snprintf(gInfo->cipInfo->ev_msg,EV_MSG_SIZE,"write failed, rc=%d ",rc) ; 
              cip_conn_failed(gInfo,cInfo, CH_EV_CON_BROKE) ; 
            }
          }
          else
          {
        //  DBG_PRT(printf(" write not FD_ISSET !! %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
          }
          break ; 
        case CIP_IO_STATE_READ :
          DBG_PRT(printf(" CIP_IO_STATE_READ.. %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
         #if USE_IB
          if ( cInfo->use_ib && cInfo->init_here && (gInfo->cipInfo->fds[cInfo->ind].revents & POLLIN) && cip_check_disconn_ev(gInfo, cInfo) < 0 )
          {
            cip_conn_failed(gInfo,cInfo, CH_EV_CON_BROKE) ; 
            break ; 
          }
         #endif
          cInfo->noHBloops++ ; 
          if ( cInfo->use_ib || (gInfo->cipInfo->fds[cInfo->ind].revents & POLLIN) || cInfo->rdInfo.offset >= cInfo->rdInfo.reqlen)
          {
            char *tbuf ; 
            uint32_t tlen ; 
            rc = extractPacket(cInfo, &tbuf, &tlen) ; 
            if ( rc > 0 )
            {
              ChannInfo tch[1] ; 
              tch->cInfo = cInfo ; 
              DBG_PRT(printf("read ok! rc=%d %d %d\n ",rc,cInfo->rdInfo.reqlen,cInfo->rdInfo.offset)) ; 
              if ( tlen < sizeof(haConComMsg) ) //|| tlen > sizeof(haConAllMsg) )
              {
                DBG_PRT(printf(" ConnFailed: bad msg len: %d %d %d\n",tlen, sizeof(haConComMsg), sizeof(haConAllMsg))) ; 
                ism_storeHA_returnBuffer(tch, tbuf) ; 
                cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
              }
              else
              {
                haConAllMsg msg[1] ; 
                memset(msg, 0, sizeof(haConAllMsg)) ; 
                if ( tlen > sizeof(haConAllMsg) )
                     tlen = sizeof(haConAllMsg) ;
                memcpy(msg, tbuf, tlen) ; 
                ism_storeHA_returnBuffer(tch, tbuf) ; 
                DBG_PRINT("_dbg_%d: RECV msg %d on conn %s\n",__LINE__,msg->com.msg_type,cInfo->req_addr) ; 
                if ( cInfo->channel )
                {
                  if ( msg->com.msg_type == HA_MSG_TYPE_CON_CID )
                  {
                    ismStore_HAChParameters_t chp[1] ; 
                    ChannInfo *ch = cInfo->channel ; 
                    ch->channel_id = msg->cid.chn_id & 0xffffff ; 
                    ch->flags      = msg->cid.chn_id >> 24 ; 
                    ch->cInfo = cInfo ;  
                    ha_set_pfds(cInfo);
                    cip_update_conn_list(gInfo, cInfo, 0) ;  
                    if ( (rc = gInfo->params->ChannelCreated(ch->channel_id, ch->flags, ch, chp, gInfo->params->pContext)) != StoreRC_OK )
                    {
                      DBG_PRT(printf(" ConnFailed: ChannelCreated callback failed (%d)\n",rc)) ; 
                      cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
                    }
                    else
                    {
                      ch->ChannelClosed = chp->ChannelClosed ; 
                      ch->MessageReceived = chp->MessageReceived;
                      ch->use_lock = chp->fMultiSend ; 
                      ch->uCtx = chp->pChContext ; 
                      if ( ch->flags&1 )
                        ch->cInfo->nPoll = gInfo->recvNpoll ; 
                      if ( ch->cInfo->nPoll < 1 ) 
                           ch->cInfo->nPoll = 1 ; 
                      if ( ism_common_startThread(&ch->thId, ism_store_haRecvThread, ch, NULL, 0, ISM_TUSAGE_NORMAL,0, chp->ChannelName, "Handle HA channel communication on SBY node") )
                      {
                        TRACE(1,"ism_common_startThread failed.\n");
                        ch->ChannelClosed(ch, ch->uCtx) ; 
                        cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
                      }
                    }
                  }
                  else
                  {
                    DBG_PRT(printf(" ConnFailed: non-cid msg recv on channel conn (%d)\n",msg->com.msg_type)) ; 
                    cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
                  }
                }
                else
                if ( msg->com.msg_type == HA_MSG_TYPE_CON_HBT )
                {
                  cInfo->next_r_time = su_sysTime() + gInfo->hbTimeOut ; 
                  cInfo->noHBloops = 0 ; 
                  TRACE(DEBUG_LEVEL,"_dbg_hbt: Setting next_r_time to %g for conn %s\n",cInfo->next_r_time,cInfo->req_addr);
                }
                else
                if ( msg->com.msg_type == HA_MSG_TYPE_NOD_TRM )
                {
                  gInfo->dInfo->state |= TRM_MSG_RECV ; 
                  cip_prepare_s_ack_trm(gInfo, cInfo) ;
                  cInfo->io_state = CIP_IO_STATE_WRITE ; 
                  gInfo->cipInfo->fds[cInfo->ind].events = POLLOUT ; 
                  gInfo->dInfo->state |= ACK_TRM_SENT ; 
                }
                else
                if ( msg->com.msg_type == HA_MSG_TYPE_ACK_TRM )
                {
                  gInfo->dInfo->state |= ACK_TRM_RECV ; 
                }
                else
                if ( cInfo->is_ha )
                {
                  DBG_PRT(printf(" ConnFailed: non-hbt msg recv on ha-hb conn (%d)\n",msg->com.msg_type)) ; 
                  cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
                }
                else
                if ( (gInfo->dInfo->state & DSC_HAVE_PAIR) )
                {
                  DBG_PRT(printf(" ConnFailed: non-hbt msg recv on mg-hb conn (%d)\n",msg->com.msg_type)) ; 
                  cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
                }
                else
                if ( msg->com.msg_type == HA_MSG_TYPE_CON_REQ )
                {
                  if ( (gInfo->dInfo->state & REQ_MSG_RECV) )
                  {
                    DBG_PRT(printf(" ConnFailed: req msg recv more than once (%d)\n",msg->com.msg_type)) ; 
                    cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
                  }
                  else
                  {
                    memcpy(&gInfo->dInfo->req_msg[1], &msg->req, sizeof(haConReqMsg)) ; 
                    gInfo->dInfo->state |= REQ_MSG_RECV ; 
                    if ( !cInfo->init_here )
                      gInfo->dInfo->state |= REQ_RECV_RMT ; 
                    if ( !(gInfo->dInfo->state & REQ_MSG_SENT) )
                    {
                      if ( cip_prepare_s_cfp_req(gInfo, cInfo) == -1 )
                      {
                        DBG_PRT(printf(" ConnFailed: cip_prepare_s_cfp_req failed\n")) ; 
                        cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
                      }
                      else
                      {
                      //cInfo->state    = CIP_STATE_S_CFP_REQ ; 
                        gInfo->dInfo->sendBit = REQ_MSG_SENT ; 
                        cInfo->io_state = CIP_IO_STATE_WRITE ; 
                        gInfo->cipInfo->fds[cInfo->ind].events = POLLOUT ; 
                      }
                    }
                  }
                }
                else
                if ( msg->com.msg_type == HA_MSG_TYPE_CON_RES )
                {
                  if ( (gInfo->dInfo->state & RES_MSG_RECV) )
                  {
                    DBG_PRT(printf(" ConnFailed: res msg recv more than once (%d)\n",msg->com.msg_type)) ; 
                    cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
                  }
                  else
                  {
                    memcpy(&gInfo->dInfo->res_msg[1], &msg->res, sizeof(haConResMsg)) ; 
                    gInfo->dInfo->state |= RES_MSG_RECV ; 
                  }
                }
                else
                if ( msg->com.msg_type == HA_MSG_TYPE_CON_ACK )
                {
                  if ( (gInfo->dInfo->state & ACK_MSG_RECV) )
                  {
                    DBG_PRT(printf(" ConnFailed: ack msg recv more than once (%d)\n",msg->com.msg_type)) ; 
                    cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
                  }
                  else
                  {
                    memcpy(&gInfo->dInfo->ack_msg[1], &msg->ack, sizeof(haConAckMsg)) ; 
                    gInfo->dInfo->state |= ACK_MSG_RECV ; 
                  }
                }
                else
                {
                  DBG_PRT(printf(" ConnFailed: unrecognized msg recv on mg-hb conn (%d)\n",msg->com.msg_type)) ; 
                  cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ; 
                }
              }
            }
            else
            if ( rc < 0 )
            {
              DBG_PRT(printf("read failed! rc=%d %d %d\n ",rc,cInfo->rdInfo.reqlen,cInfo->rdInfo.offset)) ; 
              snprintf(gInfo->cipInfo->ev_msg,EV_MSG_SIZE,"read failed, rc=%d ",rc) ; 
              cip_conn_failed(gInfo,cInfo, CH_EV_CON_BROKE) ; 
              if ( (gInfo->dInfo->state & DSC_SSL_ERR) )
                cip_raise_view(gInfo, VIEW_CHANGE_TYPE_DISC) ; 
            }
          }
          break ; 
       #if USE_IB
        case CIP_IB_STATE_CREQ :
          if ( gInfo->cipInfo->fds[cInfo->ind].revents & POLLIN ) 
          {
            DBG_PRT(printf(" CIP_IB_STATE_CREQ... %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
            rc = cip_handle_conn_req_ev(gInfo, cInfo) ; 
            if ( rc < 0 )
            {
              DBG_PRT(printf(" cip_handle_conn_req_ev failed: err= %d\n",errno)) ; 
            }
          }
          break ; 
        case CIP_IB_STATE_CEST :
          if ( !cInfo->init_here || (gInfo->cipInfo->fds[cInfo->ind].revents & POLLIN) ) 
          {
            DBG_PRT(printf(" CIP_IB_STATE_CEST... %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
            rc = cip_handle_conn_est_ev(gInfo, cInfo) ; 
            if ( rc < 0 )
            {
              DBG_PRT(printf(" cip_handle_conn_est_ev failed: err= %d\n",errno)) ; 
              cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ;
            }
          }
          break ; 
        case CIP_IB_STATE_ADDR :
          if ( gInfo->cipInfo->fds[cInfo->ind].revents & POLLIN ) 
          {
            DBG_PRT(printf(" CIP_IB_STATE_ADDR... %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
            rc = cip_handle_addr_res_ev(gInfo, cInfo) ; 
            if ( rc < 0 )
            {
              DBG_PRT(printf(" cip_handle_addr_res_ev failed: err= %d\n",errno)) ; 
              cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ;
            }
          }
          break ; 
        case CIP_IB_STATE_ROUTE:
          if ( gInfo->cipInfo->fds[cInfo->ind].revents & POLLIN ) 
          {
            DBG_PRT(printf(" CIP_IB_STATE_ROUTE... %s|%d\n",cInfo->req_addr,cInfo->req_port)) ;
            rc = cip_handle_rout_res_ev(gInfo, cInfo) ; 
            if ( rc < 0 )
            {
              DBG_PRT(printf(" cip_handle_rout_res_ev failed: err= %d\n",errno)) ; 
              cip_conn_failed(gInfo,cInfo, CH_EV_SYS_ERROR) ;
            }
          }
          break ; 
       #endif
        default :
          DBG_PRT(printf(" bad_state_2: %s|%d, state=%d \n",cInfo->req_addr,cInfo->req_port,cInfo->io_state)) ;
      }
    }
  }
  exitThread:
  ism_common_backHome();

  cip_remove_conns(gInfo, 1, CH_EV_TERMINATE) ;
  breakView(gInfo) ; 

  TRACE(5, "The %s thread is stopped\n", myName);

  pthread_mutex_lock(gInfo->haLock) ; 
  gInfo->cipInfo->chUP = 0 ; 
  pthread_mutex_unlock(gInfo->haLock) ; 

  if ( gInfo->goDown > 1 )
  {
    ism_storeHA_term();
    cip_raise_view(gInfo, VIEW_CHANGE_TYPE_BKUP_TERM) ; 
  }

  ism_common_detachThread(gInfo->haId) ; 
  return NULL ;
}
