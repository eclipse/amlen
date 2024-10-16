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
#include "rmmConstants.h"
#include "rum2rmm.h"
#include "rmmWinPthreads.h"
#include "SortedQmgm.h"
#include "BuffBoxMgm.h"
#include "rmmPacket.h"
#include "rmmCutils.h"
#include "SockMgm.h"
#include "MemMan.h"
#include "rumPrivate.h"
#include "rmmTprivate.h"
#include "rumRprivate.h"

/* Global Variables */
static int                  rumIrunning  = 0 ;
static int                  lastInstance = 0 ;
static rumInstanceRec      *rumIrec[MAX_RUM_INSTANCES] ;

#ifdef RUM_UNIX
static rmm_mutex_t      rumImutex = PTHREAD_MUTEX_INITIALIZER ;
#else
static rmm_mutex_t      rumImutex ;
static LONG volatile        rumIinitialized = 0 ;
static uint32_t             WinThreadID ;
#endif
static int global_log_level = RUM_LOGLEV_GLOBAL ;
static rum_on_log_event_t tmp_on_log_event ;
static void *tmp_log_user ;

#if USE_SSL
extern void ism_ssl_init(int useFips, int useBufferPool);
extern int ism_config_getFIPSConfig(void);
extern int ism_config_getSslUseBufferPool(void);
#endif

/*  Internal Routines  */
static int get_default_config(rumConfig *rum_config) ;
static int rum_init(rumInstanceRec *rumInfo, char* instanceName,const char *myName, int *error_code) ;
static int  alloc_instance_memory(rumInstanceRec *rumInfo) ;
static void free_instance_memory(rumInstanceRec *rumInfo) ;
static int process_config_params(rumInstanceRec *rumInfo, rumConfig *config, rumBasicConfig *basicConfig) ;
static int parse_ancillary_params(rumInstanceRec *rumInfo, rumConfig *config, rumBasicConfig *basicConfig) ;
static int read_advance_config(rumInstanceRec *rumInfo, char *file , rumAdvanceConfig *config) ;
static int read_config_line(int init, rumInstanceRec *rumInfo, char *line, rumAdvanceConfig *config) ;
static int check_configuration_parameters(rumInstanceRec *rumInfo) ;

static void send_FO_signal(rumInstanceRec *rumInfo, int iLock) ;
static void send_PR_signal(rumInstanceRec *rumInfo, int iLock) ;

static int  rum_EstablishConnection(rumInstanceRec *gInfo, const char *address,  int port,  int method, int heartbeat_timeout_milli,
                            int heartbeat_interval_milli, int one_way_heartbeat, int establish_timeout_milli,int msg_len,
                            const unsigned char *msg, rum_on_connection_event_t on_connection_event, void *user, int use_shm, int *error_code) ;
static int rum_CloseConnection(rumInstanceRec *gInfo,rumConnection *rum_connection, int *error_code) ;
static ConnInfoRec *rum_find_connection(rumInstanceRec *gInfo,rumConnectionID_t cid, int try_ind, int iLock);

static int rum_AddConnectionListener(rumInstanceRec *gInfo, rum_on_connection_event_t on_connection_event,
                              void *user, int *error_code) ;
static int rum_RemoveConnectionListener(rumInstanceRec *gInfo,void *user, int *error_code) ;
/*--------------------------------------------------------*/
static void rumImutex_lock(void)
{
  while ( rmm_mutex_trylock(&rumImutex) != 0 )
    tsleep(0,5000000) ;
}
static void rumImutex_unlock(void)
{
  tmp_on_log_event = NULL ;
  rmm_mutex_unlock(&rumImutex) ;
}
/**-------------------------------------**/
#define SET_ERROR_CODE( pErrorCode, errorCode ) if ((pErrorCode) != NULL ) { *(pErrorCode) = errorCode; }

#define LOG_BUSY_TOPIC(topicName) { \
  llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4001),"%s%s",   \
  "The queue {0} was busy when the {1} function was called.",topicName,methodName);  \
}



#define _checkNull(_arg)\
{\
  if ( _arg == NULL )\
  {\
    rumImutex_unlock() ;                 \
    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3001),"%s%s",    \
      "The {0} parameter is NULL when calling to the {1} function.",#_arg,methodName);  \
    if ( ec ) *ec = RMM_L_E_BAD_PARAMETER ;\
    return RMM_FAILURE ;                        \
  }\
}

#define _checkInst()\
{\
  if ( inst < 0 || inst >= MAX_RUM_INSTANCES || (rumInfo = rumIrec[inst]) == NULL ||\
       rumIrunning == 0 || rumInfo->initialized != RUM_TRUE )\
  {\
    if ( ec ) *ec = RUM_L_E_INSTANCE_INVALID ;\
    rumImutex_unlock() ;\
    return RUM_FAILURE ;\
  }\
  tcHandle = rumInfo->tcHandles[0];\
  LOG_METHOD_ENTRY()\
}
/*--------------------------------------------------------*/

/*  Rx and Tx code */
#include "llmLogUtil.c"
#include "RumReceiverApi.c"
#include "RumTransmitterApi.c"


static int rumInstanceCounter = 1;
static char* createRUMInstanceName(void){
  char  result[64];
  snprintf(result, sizeof(result), "RUM.%d", rumInstanceCounter);
  rumInstanceCounter++;
  return rmm_strdup(result);
}

static void registerRUMTraceComponents(const char* instanceName, TCHandle tcHandles[3]){
  memset(tcHandles,0,3*sizeof(TCHandle));  
    llmRegisterTraceComponent(instanceName,300,"RUM_API",&(tcHandles[0]),NULL);
    llmRegisterTraceComponent(instanceName,310,"RUM_RX",&(tcHandles[1]),NULL);
    llmRegisterTraceComponent(instanceName,320,"RUM_TX",&(tcHandles[2]),NULL);
}
static void unregisterRUMTraceComponents(TCHandle tcHandles[3]){
  int i;
  for(i = 0; i < 3; i++){
    llmUnregisterTraceComponent(tcHandles[i],NULL);
  }
  memset(tcHandles,0,3*sizeof(TCHandle));  
}

/*****************************************************************************************/
/*   API: Transmitter                                                                    */
/*****************************************************************************************/

/***********************   0 0 0 0   *****************************************************/
RUMAPI_DLL int rumIsSSLSupported(int *fSupported)
{
  if ( fSupported )
  {
    *fSupported = USE_SSL ; 
     return RUM_SUCCESS ;
  }
  return RUM_FAILURE ;
}


/***********************   0 0 0 0   *****************************************************/
RUMAPI_DLL int rumGetVersion(char *version, int *ec)
{
  if ( ec ) *ec = 0 ;
  if ( strlcpy(version,RMM_API_VERSION,64) > 0 )
    return RUM_SUCCESS ;
  if ( ec ) *ec = RUM_L_E_BAD_PARAMETER ;
  return RUM_FAILURE ;
}

/***********************   0 0 0 0   *****************************************************/
RUMAPI_DLL int  rumGetErrorDescription(int error_code, char* desc, int max_length)
{
  return get_error_description(error_code, desc, max_length) ;
}

/***********************   1 1 1 1   *****************************************************/
RUMAPI_DLL int  rumInit(rumInstance *rum_instance, const rumConfig *rum_config, rum_on_log_event_t on_log_event, void *log_user, int *ec)
{
  rumInstanceRec *rumInfo;
  int inst=-1, rc, error_code=0 ;
  int logRC;
  const char* myName = "rumInit";
  char* instanceName;
  TCHandle  tcHandles[3] = {NULL};
  TCHandle  tcHandle = NULL;
  if((rum_instance == NULL) || (rum_config==NULL)){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;

  if ( ec ) *ec = 0 ;

  if ( rum_config->rum_reserved != RMM_INIT_SIG )
  {
    if ( ec ) *ec = RUM_L_E_STRUCT_INIT ;
    rumImutex_unlock() ;
    return RUM_FAILURE ;
  }

  /* initial values for rum_instance indicate failure */
    rum_instance->instance = -1;


  if ( rumIrunning == 0 )
  {
    rmm_is_big = (1==ntohl(1)) ;
    for (inst=0; inst<MAX_RUM_INSTANCES; inst++)     rumIrec[inst] = NULL;
    rmmInitTime() ; /* init the internal static BaseTimeSecs under mutex */
    rumIrunning = 1;
  }
  if(rum_config->instanceName != NULL){
    instanceName = rmm_strdup(rum_config->instanceName);
  }else{
    instanceName = createRUMInstanceName();
  }
  logRC = setLogConfig(instanceName,21,rum_config->LogLevel,
      (llm_on_log_event_t)on_log_event,log_user,0,ec);
  if(logRC != 0) {
       free(instanceName);
       rumImutex_unlock() ;
       return RMM_FAILURE;
  }
  registerRUMTraceComponents(instanceName,tcHandles);
  tcHandle = tcHandles[0];

  /* search for a free place in instances array. place 0 reserved for default */
  for (inst=lastInstance+1; inst<MAX_RUM_INSTANCES && rumIrec[inst] != NULL; inst++); /* empty body */
  if ( inst >= MAX_RUM_INSTANCES )
  for (inst=             1; inst<MAX_RUM_INSTANCES && rumIrec[inst] != NULL; inst++); /* empty body */
  if ( inst >= MAX_RUM_INSTANCES )
  {
    if ( ec ) *ec = RUM_L_E_TOO_MANY_INSTANCES ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_FATAL, MSG_KEY(2441), "%d", "An instance failed to initialize.  There are too many instances currently running. The maximum number of instances is {0}.", MAX_RUM_INSTANCES);
    unregisterRUMTraceComponents(tcHandles);
    unsetLogConfig(instanceName,21);
    free(instanceName);
    rumImutex_unlock() ;
    return RUM_FAILURE ;
  }

  if ( (rumIrec[inst] = (rumInstanceRec *)malloc(sizeof(rumInstanceRec))) == NULL  )
  {
    if ( ec ) *ec = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
    LOG_MEM_ERR(tcHandle, "rumInit", sizeof(rumInstanceRec));
    unregisterRUMTraceComponents(tcHandles);
    unsetLogConfig(instanceName,21);
    free(instanceName);
    rumImutex_unlock() ;
    return RUM_FAILURE ;
  }
  memset(rumIrec[inst], 0, sizeof(rumInstanceRec));
  rumInfo = rumIrec[inst];
  rumInfo->instance=inst ;

  memcpy(&rumInfo->apiConfig,rum_config, sizeof(rumConfig)) ;
  if ( rumInfo->apiConfig.LogLevel == RUM_LOGLEV_GLOBAL )
  {
    if ( global_log_level != RUM_LOGLEV_GLOBAL )
      rumInfo->apiConfig.LogLevel = global_log_level ;
    else
      rumInfo->apiConfig.LogLevel = RUM_LOGLEV_INFO ;
  }

  if ( rum_config->SupplementalPorts && rum_config->SupplementalPortsLength > 0 )
  {
    if ( !(rumInfo->apiConfig.SupplementalPorts = malloc(rum_config->SupplementalPortsLength*sizeof(rumPort))) )
    {
      if ( ec ) *ec = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
      LOG_MEM_ERR(tcHandle, "rumInit", rum_config->SupplementalPortsLength*sizeof(rumPort));
    unregisterRUMTraceComponents(tcHandles);
    unsetLogConfig(instanceName,21);
    free(instanceName);
      rumImutex_unlock() ;
      return RUM_FAILURE ;
    }
    memcpy(rumInfo->apiConfig.SupplementalPorts,rum_config->SupplementalPorts,rum_config->SupplementalPortsLength*sizeof(rumPort)) ;
  }
  else
    rumInfo->apiConfig.SupplementalPortsLength = 0 ;

  rumInfo->on_log_event   = on_log_event;
  rumInfo->on_alert_event = rumInfo->apiConfig.on_event;
  rumInfo->log_user       = log_user ;
  rumInfo->alert_user     = rumInfo->apiConfig.event_user ;
  rumInfo->free_callback  = rumInfo->apiConfig.free_callback ;
  rumInfo->fp_log         = NULL ;
  memcpy(rumInfo->tcHandles,tcHandles,3*sizeof(TCHandle));
  rc = rum_init(rumInfo,instanceName,myName,&error_code);
  if ( rc == RUM_FAILURE )
  {
    if ( ec ) *ec = error_code ;
    free(rumIrec[inst]);
    rumIrec[inst] = NULL;
  unregisterRUMTraceComponents(tcHandles);
  unsetLogConfig(instanceName,21);
  free(instanceName);
  }
  else
  {
    rum_instance->instance = inst;
    lastInstance = inst ;
    rumInfo->initialized = RMM_TRUE;
  rumInfo->instanceName = instanceName;
    /* make sure we're not holding pointers to app space (the content is copied in rum_init) */
    rumInfo->apiConfig.Nparams = 0 ;
    rumInfo->apiConfig.AncillaryParams = NULL ;
    if ( rum_config->SupplementalPorts && rum_config->SupplementalPortsLength > 0 )
      memcpy(rum_config->SupplementalPorts,rumInfo->apiConfig.SupplementalPorts,rum_config->SupplementalPortsLength*sizeof(rumPort)) ;
  }


  rumImutex_unlock() ;
  return rc;
}


/***********************   3 3 3 3   *****************************************************/
RUMAPI_DLL int rumStop(rumInstance *rum_instance, int linger_time_milli, int *ec)
{
  rumInstanceRec *rumInfo;
  rmmReceiverRec *rInfo=NULL ;
  int rc, inst=-1, wait_time_milli ;

  int tx_error_code=0, rx_error_code=0;
  const char* methodName = "rumStop";
  TCHandle tcHandle = NULL;
  if(rum_instance == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;

  inst = rum_instance->instance;
  _checkInst() ;
  if ( ec ) *ec = 0 ;

  rInfo = (rmmReceiverRec *)rumInfo->rInfo ;
  tmp_on_log_event = rInfo->on_log_event ;
  tmp_log_user = rInfo->log_user ;

  rumInfo->initialized = RUM_FALSE;

  rc = RUM_SUCCESS;

  /* both rumT_Stop and rumR_Stop are blocking - wait only on Tx */
  /* Tx must be stopped first to allow the use of EventAnnouncer */
  rumT_Stop(rumInfo, linger_time_milli, &tx_error_code);
  rumR_Stop(rumInfo, 0, &rx_error_code);

  /* wait until both Rx and Tx announced termination */
  wait_time_milli = 1000;
  while ( wait_time_milli > 0 )
  {
    if ( !rumInfo->rxRunning && !rumInfo->txRunning )
      break;
    tsleep(0, 100000000);
    wait_time_milli -= 100;
  }
  if ( wait_time_milli <= 0 ) /* NIR - TODO what else can be done */
  {
    rc = RUM_FAILURE;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8443), "%d",
        "Attempt to stop RUM instance {0} failed.",inst);

    if ( rumInfo->rxRunning )
    {
      if ( ec ) *ec = rx_error_code ;
      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8444), "%d",
          "RX did not stop properly, (error_code = {0}). ", rx_error_code);
    }
    if ( rumInfo->txRunning )
    {
      if ( ec ) *ec = tx_error_code ;
      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8445), "%d",
          "TX did not stop properly, (error_code = {0}). ", tx_error_code);
    }
  }

  /* report method exit before closing log file */


  /* free memory */
  free_instance_memory(rumInfo) ;
  unregisterRUMTraceComponents(rumInfo->tcHandles);
  unsetLogConfig(rumInfo->instanceName,21);
  unsetLogConfig(rumInfo->instanceName,22);
  free(rumInfo->instanceName);
  if ( rumInfo->fp_log != NULL && rumInfo->fp_log != stdout ) fclose(rumInfo->fp_log);
  free(rumInfo);
  rumIrec[inst] = NULL;
  rum_instance->instance = -1;

  rumImutex_unlock() ;

  return rc;
}



/***********************   4 4 4 4   *****************************************************/
RUMAPI_DLL int  rumEstablishConnection(const rumInstance *rum_instance, const rumEstablishConnectionParams *c_params, int *ec)
{
  rumInstanceRec *rumInfo;
  int rc, inst=-1 , error_code=0 ;

  const char* methodName = "rumEstablishConnection";
  TCHandle tcHandle = NULL;
  if(rum_instance == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;

  inst = rum_instance->instance;
  _checkInst() ;
  _checkNull(c_params) ;
  _checkNull(c_params->on_connection_event) ;
  if ( ec ) *ec = 0 ;

  if ( c_params->rum_reserved != RMM_INIT_SIG )
  {
    if ( ec ) *ec = RUM_L_E_STRUCT_INIT ;
    LOG_STRUCT_NOT_INIT("rumEstablishConnectionParams");

    rumImutex_unlock() ;
    return RUM_FAILURE ;
  }

  if ( c_params->use_shared_memory )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(2446), "",
        "The shared memory feature is not supported on this build.");
    if ( ec ) *ec = RUM_L_E_BAD_PARAMETER ;
    rumImutex_unlock() ;
    return RUM_FAILURE ;
  }

  {
    TBHandle tbh = NULL;
    tbh = llmCreateTraceBuffer(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5525));
    if(tbh != NULL)
    {
      llmAddTraceData(tbh, "", "rumEstablishConnectionParams: ");
      llmAddTraceData(tbh, "%s", "address = {0}, ", c_params->address);
      llmAddTraceData(tbh, "%d", "port = {0}, ", c_params->port);
      llmAddTraceData(tbh, "%d", "connect_method = {0}, ", c_params->connect_method);
      llmAddTraceData(tbh, "%d", "heartbeat_timeout_milli = {0}, ", c_params->heartbeat_timeout_milli);
      llmAddTraceData(tbh, "%d", "heartbeat_interval_milli = {0}, ", c_params->heartbeat_interval_milli);
      llmAddTraceData(tbh, "%d", "establish_timeout_milli = {0}, ", c_params->establish_timeout_milli);
      llmAddTraceData(tbh, "%llx", "on_connection_event = {0}, ", LLU_VALUE(c_params->on_connection_event));
      llmAddTraceData(tbh, "%llx", "on_connection_user = {0}, ", LLU_VALUE(c_params->on_connection_user));
      llmAddTraceData(tbh, "%d", "connect_msg_len = {0}, ", c_params->connect_msg_len);
      if ( c_params->connect_msg_len > 0 )
      {
        char *tstr = malloc(2*(c_params->connect_msg_len+1)) ; 
        if ( tstr )
        {
          b2h(tstr, c_params->connect_msg, c_params->connect_msg_len);
          llmAddTraceData(tbh, "%s", "connect_msg = {0}, ", tstr);
          free(tstr) ; 
        }
      }
      llmAddTraceData(tbh, "%d", "one_way_heartbeat = {0}, ", c_params->one_way_heartbeat);
      llmCompositeTraceInvoke(tbh);
    }
  }

  rc = rum_EstablishConnection(rumInfo, c_params->address, c_params->port, c_params->connect_method,
                             c_params->heartbeat_timeout_milli,c_params->heartbeat_interval_milli,c_params->one_way_heartbeat,
                             c_params->establish_timeout_milli,c_params->connect_msg_len,c_params->connect_msg,
                             c_params->on_connection_event, c_params->on_connection_user, c_params->use_shared_memory, &error_code) ;
  if ( rc == RUM_FAILURE )
  {
    if ( ec ) *ec = error_code ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8447), "",
        "Unable to create request | establish connection.");
  }

  LOG_METHOD_EXIT();

  rumImutex_unlock() ;

  return rc;
}

/***********************   5 5 5 5   *****************************************************/
RUMAPI_DLL int  rumCloseConnection(rumConnection *rum_connection, int *ec)
{

  rumInstanceRec *rumInfo;
  int rc, inst=-1 , error_code=0 ;
  const char* methodName = "rumCloseConnection";
  TCHandle tcHandle = NULL;
  if(rum_connection == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;
  inst = rum_connection->rum_instance;
  _checkInst() ;
  if ( ec ) *ec = 0 ;

/* rumImutex_unlock() ; */
  rc = rum_CloseConnection(rumInfo, rum_connection,&error_code) ;
/* pthread_mutex_lock(&rumImutex) ; */

  if ( rc == RUM_FAILURE )
  {
    if ( ec ) *ec = error_code ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8448), "",
        "Attempt to close connection failed.");
  }

  LOG_METHOD_EXIT();

  rumImutex_unlock() ;

  return rc;
}

/***********************   6 6 6 6   *****************************************************/
RUMAPI_DLL int  rumAddConnectionListener(const rumInstance *rum_instance, rum_on_connection_event_t on_conn_event, void *event_user, int *ec)
{

  rumInstanceRec *rumInfo;
  int rc, inst=-1 , error_code=0 ;
  const char* methodName = "rumAddConnectionListener";
  TCHandle tcHandle = NULL;
  if(rum_instance == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;

  inst = rum_instance->instance;
  _checkInst() ;

  if ( ec ) *ec = 0 ;
  _checkNull(on_conn_event) ;


  rc = rum_AddConnectionListener(rumInfo,on_conn_event, event_user,&error_code) ;
  if ( rc == RMM_FAILURE )
  {
    if ( ec ) *ec = error_code ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8449), "%d",
        "Unable to add connection listener, there are too many listeners.  The maximum allowed number of listeners is {0}.", RUM_MAX_CONNECTION_LISTENERS);
  }

  LOG_METHOD_EXIT();

  rumImutex_unlock() ;

  return rc;
}


/***********************   7 7 7 7   *****************************************************/
RUMAPI_DLL int  rumRemoveConnectionListener(const rumInstance *rum_instance, void *event_user, int *ec)
{

  rumInstanceRec *rumInfo;
  int rc, inst=-1 , error_code=0 ;
  const char* methodName = "rumRemoveConnectionListener";
  TCHandle tcHandle = NULL;
  if(rum_instance == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;


  inst = rum_instance->instance;
  _checkInst() ;

  if ( ec ) *ec = 0 ;
  rc = rum_RemoveConnectionListener(rumInfo, event_user,&error_code) ;
  if ( rc == RMM_FAILURE )
  {
    if ( ec ) *ec = error_code ? error_code : RUM_L_E_GENERAL_ERROR ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8450), "",
        "Specified listener not found");
  }

  LOG_METHOD_EXIT();

  rumImutex_unlock() ;

  return rc;
}

/***********************   0 0 0 0   *****************************************************/

/***********************   0 0 0 0   *****************************************************/
RUMAPI_DLL int  rumChangeLogLevel(int new_log_level, int *ec)
{
  rumInstanceRec *rumInfo;
  int inst, rc, error_code=0;

  rumImutex_lock() ;
  if ( ec ) *ec = 0 ;

  if ( new_log_level < RUM_LOGLEV_NONE ||
       new_log_level > RUM_LOGLEV_XTRACE )
  {
    if ( ec ) *ec = RUM_L_E_BAD_PARAMETER ;
    rumImutex_unlock() ;
    return RUM_FAILURE ;
  }
  global_log_level = new_log_level ;

  if ( rumIrunning == 0 )
  {
    rumImutex_unlock() ;
    return RUM_SUCCESS ;
  }

  rc = RUM_SUCCESS ;
  for (inst=0; inst<MAX_RUM_INSTANCES; inst++)
  {
    if ( (rumInfo=rumIrec[inst]) == NULL )
      continue ;
    rumInfo->apiConfig.LogLevel = new_log_level ;
    rumInfo->basicConfig.LogLevel = new_log_level ;
    {
      llmInstanceLogCfg_t logConfig;
      memset(&logConfig,0,sizeof(llmInstanceLogCfg_t));
      logConfig.filterID = 21;
      logConfig.instanceName = rumInfo->instanceName;
      if(llmGetInstanceLogConfig(&logConfig,ec) == 0)
      {
        logConfig.allowedLogLevelDefault = new_log_level;
        if(llmSetInstanceLogConfig(&logConfig,1,ec) != 0)
          rc = RUM_FAILURE;
      }
      else
        rc = RUM_FAILURE;
    }
    if ( rumT_ChangeLogLevel(rumInfo,new_log_level,&error_code) != RUM_SUCCESS )
      rc = RUM_FAILURE ;
    if ( rumR_ChangeLogLevel(rumInfo,new_log_level,&error_code) != RUM_SUCCESS )
      rc = RUM_FAILURE ;
    if ( rc == RUM_FAILURE && ec && error_code ) *ec = error_code ;
  }
  rumImutex_unlock() ;
  return rc ;
}

/***********************   0 0 0 0   *****************************************************/


/***********************   8 8 8 8   *****************************************************/
RUMAPI_DLL int rumInitStructureParameters(RUM_STRUCTURE_ID_t structure_id, void *structure, int api_version, int *ec)
{
  int rc ;
  rc = RUM_SUCCESS ;
  switch ( structure_id )
  {
    case RUM_SID_CONFIG :
    {
      rumConfig *conf ;
      conf = (rumConfig *)structure ;
      memset(conf,0,sizeof(rumConfig)) ;
      get_default_config(conf) ;
      conf->rum_reserved = RUM_INIT_SIG ;
      break ;
    }
    case RUM_SID_TxQUEUEPARAMETERS :
    {
      rumTxQueueParameters *qt_params ;
      qt_params = (rumTxQueueParameters *)structure ;
      memset(qt_params,0,sizeof(rumTxQueueParameters)) ;
      qt_params->reliability  = RUM_RELIABLE_NO_HISTORY ;
      qt_params->rum_reserved = RUM_INIT_SIG ;
      break ;
    }
    case RUM_SID_RxQUEUEPARAMETERS :
    {
      rumRxQueueParameters *qr_params ;
      qr_params = (rumRxQueueParameters *)structure ;
      memset(qr_params,0,sizeof(rumRxQueueParameters)) ;
      qr_params->reliability  = RUM_RELIABLE_RCV ;
      qr_params->rum_reserved = RUM_INIT_SIG ;
      break ;
    }
    case RUM_SID_ESTABLISHCONNPARAMS :
    {
      rumEstablishConnectionParams *c_params ;
      c_params = (rumEstablishConnectionParams *)structure ;
      memset(c_params,0,sizeof(rumEstablishConnectionParams)) ;
      c_params->connect_method = RUM_CREATE_NEW ;
      c_params->heartbeat_timeout_milli = 10000 ;
      c_params->heartbeat_interval_milli=  1000 ;
      c_params->establish_timeout_milli = 10000 ;
      c_params->rum_reserved = RUM_INIT_SIG ;
      break ;
    }
    case RUM_SID_TxMESSAGE :
    {
      rumTxMessage *msg ;
      msg = (rumTxMessage *)structure ;
      memset(msg,0,sizeof(rumTxMessage)) ;
      msg->rum_reserved = RUM_INIT_SIG ;
      break ;
    }
    case RUM_SID_MSGPROPERTY :
    {
      rumMsgProperty *prop ;
      prop = (rumMsgProperty *)structure ;
      memset(prop,0,sizeof(rumMsgProperty)) ;
      prop->type = RUM_MSG_PROP_INT32 ;
      prop->rum_reserved = RUM_INIT_SIG ;
      break ;
    }
    case RUM_SID_MSGSELECTOR :
    {
      rumMsgSelector *sel ;
      sel = (rumMsgSelector *)structure ;
      memset(sel,0,sizeof(rumMsgSelector)) ;
      sel->rum_reserved = api_version ;
      sel->rum_reserved = RUM_INIT_SIG ;
      break ;
    }
    case RUM_SID_LATENCYMONPARAMS :
    {
      rumLatencyMonParams *lmp ;
      lmp = (rumLatencyMonParams *)structure ;
      memset(lmp,0,sizeof(rumLatencyMonParams));
      lmp->rum_reserved = api_version ;
      lmp->rum_reserved = RUM_INIT_SIG ;
      break ;
    }
    default :
      if ( ec ) *ec = RMM_L_E_BAD_PARAMETER ;
      rc = RUM_FAILURE ;
  }
  ((llmCommonInitStruct *)structure)->llm_api_version = api_version;
  return rc ;
}

/***********************   8 8 8 8   *****************************************************/

  /*----    TRANSMITTER API functions    ----*/

/***********************   8 8 8 8   *****************************************************/
RUMAPI_DLL int  rumTCreateQueue(const rumInstance *rum_instance, rumTxQueueParameters *queue_params, rumQueueT *queue_t, int *ec)
{

  rumInstanceRec *rumInfo;
  int rc, inst=-1 , error_code=0 ;
  const char* methodName = "rumTCreateQueue";
  TCHandle tcHandle = NULL;
  if(rum_instance == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;

  inst = rum_instance->instance;
  _checkInst() ;
  _checkNull(queue_params) ;
  _checkNull(queue_t) ;

  if ( ec ) *ec = 0 ;

  if ( queue_params->rum_reserved != RMM_INIT_SIG )
  {
    if ( ec ) *ec = RUM_L_E_STRUCT_INIT ;
    LOG_STRUCT_NOT_INIT("rumTxQueueParameters");

    rumImutex_unlock() ;
    return RUM_FAILURE ;
  }

  if ( queue_params->rum_length > 2100 )
    rc = rumT_CreateQueue(rumInfo, queue_params->queue_name, queue_params->reliability,&queue_params->rum_connection,
                        queue_params->on_event, queue_params->event_user, 
                        queue_params->is_late_join, queue_t,queue_params->stream_id,queue_params->app_controlled_batch, &error_code);
  else
    rc = rumT_CreateQueue(rumInfo, queue_params->queue_name, queue_params->reliability,&queue_params->rum_connection,
                        queue_params->on_event, queue_params->event_user, 
                        queue_params->is_late_join, queue_t,queue_params->stream_id,                                 0, &error_code);

  if ( rc == RUM_FAILURE )
  {
    if ( ec ) *ec = error_code ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8354), "",
        "Creation of QueueT failed.");
  }

  LOG_METHOD_EXIT();

  rumImutex_unlock() ;

  return rc;
}


/***********************   9 9 9 9   *****************************************************/
RUMAPI_DLL int  rumTCloseQueue(rumQueueT *queue_t, int linger_time_milli, int *ec)
{

  rumInstanceRec *rumInfo;
  int rc, inst=-1 , error_code=0 ;
  const char* methodName = "rumTCloseQueue";
  TCHandle tcHandle = NULL;
  if(queue_t == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;


  inst = queue_t->instance;
  _checkInst() ;

  if ( ec ) *ec = 0 ;
  rc = rumT_CloseQueue(rumInfo, queue_t, linger_time_milli,&error_code);

  if ( rc == RUM_FAILURE )
  {
    if ( ec ) *ec = error_code ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8355), "",
        "Failed to close QueueT.");
  }

  LOG_METHOD_EXIT();

  rumImutex_unlock() ;

  return rc;
}

/***********************   11 11 11 11   *************************************************/

RUMAPI_DLL int  rumTSubmitMsg(const rumQueueT *queue_t, const char *msg,  int msg_len, int *ec)
{
  int inst , rc, error_code=0;

  if ( queue_t == NULL || (inst=queue_t->instance) < 0 || inst >= MAX_RUM_INSTANCES ||
       rumIrec[inst] == NULL || rumIrec[inst]->initialized == RMM_FALSE)
  {
    error_code = RUM_L_E_INSTANCE_INVALID ;
    rc = RUM_FAILURE;
  }
  else
    rc = rumT_SubmitMessage(rumIrec[inst], queue_t->handle, msg, msg_len, NULL, &error_code);

  if ( ec ) *ec = error_code ;
  return rc ;
}
/***********************   11.1 11.1 11.1   *************************************************/
RUMAPI_DLL int  rumTSubmitMessage(const rumQueueT *queue_t, const rumTxMessage *msg_params, int *ec)
{
  int inst , rc, error_code=0;

  if ( queue_t == NULL || msg_params == NULL || (inst=queue_t->instance) < 0 || inst >= MAX_RUM_INSTANCES ||
       rumIrec[inst] == NULL || rumIrec[inst]->initialized == RMM_FALSE)
  {
    error_code = RUM_L_E_INSTANCE_INVALID ;
    rc = RUM_FAILURE;
  }
  else
  if ( msg_params->rum_reserved != RMM_INIT_SIG )
  {
    error_code = RUM_L_E_STRUCT_INIT ;
    rc = RUM_FAILURE ;
  }
  else
    rc = rumT_SubmitMessage(rumIrec[inst], queue_t->handle, msg_params->msg_buf, msg_params->msg_len, msg_params, &error_code);
  if ( ec ) *ec = error_code ;
  return rc ;
}


/***********************   19   *************************************************/
RUMAPI_DLL int  rumTGetStreamID(const rumQueueT *queue_t, rumStreamID_t *stream_id, int *ec)
{
  int rc, inst=-1 , error_code=0 ;
  rumInstanceRec *rumInfo;
  const char* methodName = "rumTGetStreamID";
  TCHandle tcHandle = NULL;
  if(queue_t == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;

  inst = queue_t->instance;
  _checkInst() ;
  _checkNull(stream_id) ;
  if ( ec ) *ec = 0 ;

  rc = rumT_GetStreamID(queue_t, stream_id, &error_code);

  rumImutex_unlock() ;
  if ( ec ) *ec = error_code ;
  return rc;
}


/*****************************************************************************************/

/*----    RECEIVER functions    ----*/

/***********************   16 16 16 16   *************************************************/
RUMAPI_DLL int  rumRCreateQueue(const rumInstance *rum_instance, const rumRxQueueParameters *queue_params, rumQueueR *queue_r, int *ec)
{

  rumInstanceRec *rumInfo;
  int rc, inst=-1 , error_code=0 ;
  const char* methodName = "rumRCreateQueue";
  TCHandle tcHandle = NULL;
  if(rum_instance == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;

  inst = rum_instance->instance;
  _checkInst() ;
  _checkNull(queue_params) ;
  _checkNull(queue_r) ;
  if ( ec ) *ec = 0 ;

  if ( queue_params->rum_reserved != RMM_INIT_SIG )
  {
    if ( ec ) *ec = RUM_L_E_STRUCT_INIT ;
    LOG_STRUCT_NOT_INIT("rumRxQueueParameters");

    rumImutex_unlock() ;
    return RUM_FAILURE ;
  }

  rc = rumR_CreateQueue(rumInfo, queue_params, queue_r, &error_code);
  if ( rc == RUM_FAILURE )
  {
    if ( ec ) *ec = error_code ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8463), "",
        "Failed to create QueueR");
  }

  LOG_METHOD_EXIT();

  rumImutex_unlock() ;

  return rc;
}


/***********************   18 18 18 18   *************************************************/
RUMAPI_DLL int  rumRCloseQueue(rumQueueR *queue_r, int *ec)
{
  rumInstanceRec *rumInfo;
  int rc, inst=-1 , error_code=0 ;
  const char* methodName = "rumRCloseQueue";
  TCHandle tcHandle = NULL;
  if(queue_r == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;

  inst = queue_r->instance;
  _checkInst() ;
  if ( ec ) *ec = 0 ;

  rc = rumR_CloseQueue(rumInfo, queue_r->handle, &error_code);
  if ( rc == RUM_FAILURE )
  {
    if ( ec ) *ec = error_code ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8464), "",
        "Failed to close QueueR");
  }

  LOG_METHOD_EXIT();

  rumImutex_unlock() ;

  return rc;
}



/***********************   21 21 21 21   *************************************************/
RUMAPI_DLL int  rumRRemoveStream(const rumInstance *rum_instance, rumStreamID_t stream_id, int *ec)
{

  rumInstanceRec *rumInfo;
  int rc, inst=-1 , error_code=0 ;
  const char* methodName = "rumRRemoveStream";
  TCHandle tcHandle = NULL;
  if(rum_instance == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;

  inst = rum_instance->instance;
  _checkInst() ;
  if ( ec ) *ec = 0 ;

  rc = rumR_RemoveStream(rumInfo, stream_id,&error_code);

  if ( rc == RUM_FAILURE )
  {
    if ( ec ) *ec = error_code ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8470), "",
        "Failed to reject stream. ");
  }

  LOG_METHOD_EXIT();

  rumImutex_unlock() ;

  return rc;
}


/***********************   20 20 20 20   *************************************************/
RUMAPI_DLL int  rumRClearRejectedStreams(const rumInstance *rum_instance, int *ec)
{
  rumInstanceRec *rumInfo;
  int rc, inst=-1 ,error_code=0 ;
  const char* methodName = "rumRClearRejectedStreams";
  TCHandle tcHandle = NULL;
  if(rum_instance == NULL){
    SET_ERROR_CODE(ec,RUM_L_E_BAD_PARAMETER);
    return RUM_FAILURE;
  }
  rumImutex_lock() ;

  inst = rum_instance->instance;
  _checkInst() ;
  if ( ec ) *ec = 0 ;

  rc = rumR_ClearRejectedStreams(rumInfo,&error_code);
  if ( rc == RUM_FAILURE )
  {
    if ( ec ) *ec = error_code ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8471), "",
        "Failed to clear rejected streams");
  }

  LOG_METHOD_EXIT();

  rumImutex_unlock() ;

  return rc;
}

/* ------  Internal functions ---------------- */

  /***********************   1 1 1 1   *****************************************************/
int rum_init(rumInstanceRec *rumInfo, char* instanceName, const char *myName, int *error_code)
{
  int rc , tmp_err ;
  int inst ;
  TCHandle tcHandle = rumInfo->tcHandles[0];
  char errMsg[ERR_MSG_SIZE];

  inst = rumInfo->instance ;

  rc = RUM_SUCCESS;

  /* process configuration */
  read_advance_config(rumInfo, rumInfo->apiConfig.AdvanceConfigFile, &rumInfo->advanceConfig);
  process_config_params(rumInfo, &rumInfo->apiConfig, &rumInfo->basicConfig) ;
  if ( check_configuration_parameters(rumInfo) == RUM_FAILURE )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8484), "",
        "Initialization failed due to invalid configuration.  Please check configuration parameters. ");

    *error_code = RUM_L_E_CONFIG_ENTRY ;
    return RUM_FAILURE ;
  }
  if(rumInfo->advanceConfig.DefaultLogLevel > LLM_LOGLEV_NONE){
      int logRC;
      char fileName[32];
      strlcpy(fileName, rumInfo->advanceConfig.LogFile,10);
      upper(fileName);

      if((fileName[0] == 0)||(strcmp(fileName,"STD") == 0)){
          rumInfo->fp_log = stdout;
      }else{
          rumInfo->fp_log = llm_fopen(rumInfo->advanceConfig.LogFile,"w",&rc);
          if(rumInfo->fp_log == NULL){
            llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8487), "%s%d",
              "Initialization failed due to invalid configuration.  Failed to open advanced log file {0}. The error code is {1}",
                rumInfo->advanceConfig.LogFile,rc);
            *error_code = RUM_L_E_CONFIG_ENTRY ;
            return RUM_FAILURE ;
          }
      }
      logRC = setLogConfig(instanceName,22,rumInfo->advanceConfig.DefaultLogLevel,myLogger,rumInfo->fp_log,0,error_code);
      if(logRC != 0) return RUM_FAILURE;
  }

  rumInfo->initialized = RUM_FALSE;

  /* set "random" source port to ServerPort */
  rumInfo->port = htons((uint16_t)rumInfo->basicConfig.ServerSocketPort);

  rumInfo->use_ssl = (rumInfo->apiConfig.Protocol == RUM_SSL) ; 
  if ( rumInfo->use_ssl )
 #if USE_SSL
  {
    ism_ssl_init(ism_config_getFIPSConfig(),ism_config_getSslUseBufferPool());
  }
 #else
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(1129), "",
        "The SSL feature is not supported on this build.");
    *error_code = RUM_L_E_BAD_PARAMETER ;
    return RUM_FAILURE ;
  }
 #endif

  if ( alloc_instance_memory(rumInfo) != RMM_SUCCESS )
  {
    free_instance_memory(rumInfo);
    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ; 
    return RUM_FAILURE ;
  }

  /* NIR - ConnectionHandler and EventAnnouncer threads will be started and stopped by the receiver */
/*
  Function use and update rumInfo directly
  int  rumT_Init(rumInstanceRec *rumInfo,int *error_code);
  int  rumR_Init(rumInstanceRec *rumInfo,int *error_code);
*/

  /* Initialize Receiver and Transmitter */
  /* The Receiver initialyze the ConnectionHandler which */
  /* requires the Transmitter to be initialized          */
  if ( rc == RUM_SUCCESS )
  {
    rc = rumT_Init(rumInfo, error_code);
  }
  if ( rc == RUM_SUCCESS )
  {
    rc = rumR_Init(rumInfo, error_code);
    if ( rc != RUM_SUCCESS )
      rumT_Stop(rumInfo, 0, &tmp_err);
  }

  /* call again just to print parameters */
  if ( rc == RUM_SUCCESS )
  {
 // set_source_nla(inst,0) ; 
    rumInfo->gsi_high = rmmTRec[inst]->gsi_high;
    rmmTRec[inst]->port = rumInfo->port;

    check_configuration_parameters(rumInfo);


  }




  if ( rc == RMM_FAILURE )
  {
    free_instance_memory(rumInfo);
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5488), "%d%s%s",
      "The RUM instance {0} failed to initialize (Version {1}) running on {2}.", inst, RMM_API_VERSION, RMM_OS_TYPE);

  }else{
    rumInfo->initialized = RUM_TRUE;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5489), "%d%s%s",
          "The RUM instance {0} was initialized (Version {1}) running on {2}.", inst, RMM_API_VERSION, RMM_OS_TYPE);
  }


  get_OS_info(errMsg,sizeof(errMsg));
  llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5490), "%s",
      "{0}", errMsg);



  return rc ;
}

/*****************************************************************************************/
int alloc_instance_memory(rumInstanceRec *rumInfo)
{
  int rc; //,inst ;
  TCHandle tcHandle = rumInfo->tcHandles[0];
  //inst = rumInfo->instance ;

  if ( (rc=pthread_mutex_init(&rumInfo->ClockMutex, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(5491), "%d",
        "Failed to init mutex variable (ClockMutex)(rc = {0}).  This should not happen!", rc);
    return RMM_FAILURE ;
  }

  if ( (rc=pthread_mutex_init(&rumInfo->ConnectionListenersMutex, NULL)) != 0 )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(5492), "%d",
        "Failed to init mutex variable (ConnectionListenersMutex)(rc = {0}).  This should not happen!", rc);
    return RMM_FAILURE ;
  }

  if ( (rc=rmm_rwlock_init(&rumInfo->ConnSyncRW)) != RMM_SUCCESS )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(5493), "%d",
        "Failed to init rwlock variable (ConnSyncRW)(rc = {0}). This should not happen!", rc);
    return RMM_FAILURE ;
  }
  rmm_rwlock_rdlock(&rumInfo->ConnSyncRW) ;
  rmm_rwlock_getnupd(&rumInfo->ConnSyncRW) ;
  if ( rmm_rwlock_rdunlockif(&rumInfo->ConnSyncRW) != RMM_SUCCESS )
  rmm_rwlock_rdunlock(&rumInfo->ConnSyncRW) ;

  rumInfo->connReqQsize = 128 ;
  rumInfo->connReqQsize = offsetof(ConnInfoRec,ll_next) ; 
  if ( (rumInfo->connReqQ = LL_alloc(rumInfo->connReqQsize,0) ) == NULL )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8494), "",
        "Failed to allocate LinkedList (connReqQ) causing init failure.  This should not happen!");
    return RMM_FAILURE ;
  }

  if ( (rumInfo->nackBuffsQ = MM_alloc(NACK_BUFF_SIZE,rumInfo->advanceConfig.nackBuffsQsize,0,0,0)) == NULL )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8495), "",
        "Failed to allocate BufferPool (nackBuffsQ) causing init failure.  This should not happen.  ");
    return RMM_FAILURE ;
  }

  if ( (rumInfo->recvNacksQ = BB_alloc(rumInfo->advanceConfig.nackBuffsQsize,0) ) == NULL )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_TRACE, MSG_KEY(8496), "",
        "Failure to allocate BufferBox (recnNacksQ) has caused init to fail.  This should not happen!");
    return RMM_FAILURE ;
  }

  return RMM_SUCCESS ;
}
/*****************************************************************************************/
void free_instance_memory(rumInstanceRec *rumInfo)
{
  int n;

  if ( rumInfo->connReqQ )   LL_free(rumInfo->connReqQ,1) ;
  if ( rumInfo->nackBuffsQ ) MM_free(rumInfo->nackBuffsQ,1) ;
  if ( rumInfo->recvNacksQ ) BB_free(rumInfo->recvNacksQ,1) ;

  rmm_rwlock_destroy(&rumInfo->ConnSyncRW) ;
  pthread_mutex_destroy(&rumInfo->ConnectionListenersMutex) ;
  pthread_mutex_destroy(&rumInfo->ClockMutex) ;

  for ( n=0 ; n<rumInfo->nConnectionListeners ; n++ )
  {
    if ( rumInfo->ConnectionListeners[n] == NULL )
      continue;
    if ( rumInfo->free_callback != NULL )
      rumInfo->free_callback(rumInfo->ConnectionListeners[n]->user);
    free(rumInfo->ConnectionListeners[n]);
  }

  if ( rumInfo->apiConfig.SupplementalPorts )
    free (rumInfo->apiConfig.SupplementalPorts) ;

 #if USE_POLL
  if ( rumInfo->cipInfo.fds )
    free (rumInfo->cipInfo.fds) ;

  if ( rumInfo->rfds )
    free(rumInfo->rfds) ; 

  if ( rumInfo->wfds )
    free(rumInfo->wfds) ; 
 #endif

  return ;
}


/*****************************************************************************************/
int process_config_params(rumInstanceRec *rumInfo, rumConfig *config, rumBasicConfig *basicConfig)
{
  int pending_size;

  /* copy common parameters to basicConfig */
  basicConfig->Protocol = config->Protocol ;
  basicConfig->IPVersion = config->IPVersion ;
  basicConfig->ServerSocketPort = config->ServerSocketPort ;
  basicConfig->LogLevel = config->LogLevel ;
  basicConfig->LimitTransRate = config->LimitTransRate ;
  basicConfig->TransRateLimitKbps = config->TransRateLimitKbps ;
  basicConfig->PacketSizeBytes = config->PacketSizeBytes ;
  basicConfig->TransportDirection = config->TransportDirection ;
  basicConfig->MaxMemoryAllowedMBytes = config->MaxMemoryAllowedMBytes ;
  basicConfig->MinimalHistoryMBytes = config->MinimalHistoryMBytes ;
  basicConfig->SocketReceiveBufferSizeKBytes = config->SocketReceiveBufferSizeKBytes ;
  basicConfig->SocketSendBufferSizeKBytes = config->SocketSendBufferSizeKBytes ;
  strlcpy(basicConfig->RxNetworkInterface,config->RxNetworkInterface,RUM_ADDR_STR_LEN) ;
  strlcpy(basicConfig->TxNetworkInterface,config->TxNetworkInterface,RUM_ADDR_STR_LEN) ;
  strlcpy(basicConfig->AdvanceConfigFile,config->AdvanceConfigFile,RUM_FILE_NAME_LEN) ;

  /* insert ancillary parameters into basicConfig */
  parse_ancillary_params(rumInfo, config, basicConfig);

  /* set unique basicConfig parameters */

  /* memory allocation  */
  pending_size = 4000000 ;  /* maximal value 4M */
  if ( rumInfo->advanceConfig.MaxPendingQueueKBytes > pending_size )
    rumInfo->advanceConfig.MaxPendingQueueKBytes = pending_size ;
  if ( rumInfo->advanceConfig.MaxPendingQueueKBytes >= config->MaxMemoryAllowedMBytes * 512 )
    rumInfo->advanceConfig.MaxPendingQueueKBytes = config->MaxMemoryAllowedMBytes * 512;
  if ( rumInfo->advanceConfig.MaxPendingQueueKBytes == 0 )
    pending_size = rmmMin(20,config->MaxMemoryAllowedMBytes/10);
  else
    pending_size = (rumInfo->advanceConfig.MaxPendingQueueKBytes+1023) / 1024;
  if ( pending_size <= 0 )  pending_size = 1 ;

  rumInfo->advanceConfig.MaxPendingQueueKBytes = 1024 * pending_size ;

  basicConfig->TxMaxMemoryAllowedMBytes = 12*(config->MinimalHistoryMBytes + pending_size)/10 ;
  basicConfig->RxMaxMemoryAllowedMBytes = config->MaxMemoryAllowedMBytes - basicConfig->TxMaxMemoryAllowedMBytes ;

  if ( config->SupplementalPorts && config->SupplementalPortsLength > 0 )
  {
    int i;
    for ( i=0 ; i<config->SupplementalPortsLength ; i++ )
    {
      if ( !config->SupplementalPorts[i].networkInterface[0] )
        strlcpy(config->SupplementalPorts[i].networkInterface, "ALL", RUM_ADDR_STR_LEN);
      else
        upper(config->SupplementalPorts[i].networkInterface) ;
    }
  }

  return RMM_SUCCESS ;

}

/*****************************************************************************************/
int parse_ancillary_params(rumInstanceRec *rumInfo, rumConfig *config, rumBasicConfig *basicConfig)
{
  int i;
  char line[MAX_ANCILLARY_PARAM_SIZE+1] ;
  int params = config->Nparams;
  TCHandle tcHandle = rumInfo->tcHandles[0];

  if (params < 0 )
    return RUM_FAILURE;

  for (i=0; i<params; i++)
  {
    if ( strllen(config->AncillaryParams[i],MAX_ANCILLARY_PARAM_SIZE) >= MAX_ANCILLARY_PARAM_SIZE )
    {
      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_DEBUG, MSG_KEY(7498), "",
          "Bad Ancillary Parameter");

      continue ;
    }

    strlcpy(line, config->AncillaryParams[i],sizeof(line));
    if ( read_config_line(1,rumInfo, line, &rumInfo->advanceConfig) == RMM_FAILURE )
    {
      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_WARN, MSG_KEY(4417), "%s",
          "There is an unrecognized ancillary parameter found at line {0}.", line);

    }
  }

  return RUM_SUCCESS;
}

/*****************************************************************************************/

int get_default_config(rumConfig *rum_config)
{
  if ( !rum_config )
    return RUM_FAILURE ;
  memset(rum_config,0,sizeof(rumConfig)) ;

  rum_config->rum_reserved                    = RUM_INIT_SIG ;
  rum_config->Protocol                      = RUM_TCP ;
  rum_config->IPVersion                     = RUM_IPver_IPv4 ;
  rum_config->ServerSocketPort              = 35353 ;
  rum_config->LogLevel                      = RUM_LOGLEV_INFO ;
  rum_config->LimitTransRate                = RUM_DISABLED_RATE_LIMIT ;
  rum_config->TransRateLimitKbps            = 100000 ;
  rum_config->PacketSizeBytes               = 8000 ;
  rum_config->TransportDirection            = RUM_Tx_Rx ;
  rum_config->MaxMemoryAllowedMBytes        = 200 ;
  rum_config->MinimalHistoryMBytes          = 0 ;
  rum_config->SocketReceiveBufferSizeKBytes = 1024 ;
  rum_config->SocketSendBufferSizeKBytes    = 64 ;
  rum_config->Nparams                       = 0 ;
  rum_config->AncillaryParams              = NULL ;
  rum_config->on_event                      = NULL ;
  rum_config->event_user                    = NULL ;
  rum_config->free_callback                 = NULL ;
  memcpy(rum_config->RxNetworkInterface,"NONE",5) ;
  memcpy(rum_config->TxNetworkInterface,"NONE",5) ;
  memcpy(rum_config->AdvanceConfigFile,"NONE",5) ;
  return RUM_SUCCESS ;
}


/*****************************************************************************************/
int read_advance_config(rumInstanceRec *rumInfo, char *file , rumAdvanceConfig *config)
{
  char line[LINE_SIZE] ;
  FILE *fp ;
  char file_none[8];
  int using_default = 1;
  TCHandle tcHandle = rumInfo->tcHandles[0];

  memset(config,0,sizeof(rumAdvanceConfig)) ;
  config->MaxPendingQueueKBytes        = 0 ;                    /* if not defined will be according to MaxMemory */
  config->MaxStreamsPerTransmitter     = RMM_MAX_TX_TOPICS/2 ;  /* 1024 */
  config->FireoutPollMicro             = 0 ;
  config->PacketsPerRound              = 10 ;
  config->SnapshotCycleMilli_tx        = 0;
  config->PrintStreamInfo_tx           = 1 ;
  config->StreamReportIntervalMilli    = 10000 ;
  config->MinBatchingMicro             = 100 ;
  config->MaxBatchingMicro             = 1000 ;
  strlcpy(config->apiBatchMode,"Disabled",sizeof(config->apiBatchMode));

  config->DefaultLogLevel              = 0 ;                  /* LogLevel and DefaultLogLevel are equal */
  config->LogLevel                     = 0 ;
  memcpy(config->LogFile,"std",4) ;


  /* the following Tx params are not exposed in the RUM documentation since they have minor impact and the defaults should be used.  */
  config->MaxFireOutThreads            = 1 ;
  config->StreamsPerFireout            = 1 ;
  config->PacketsPerRoundWhenCleaning  = 5 ;
  config->RdataSendPercent             = 90 ;
  config->CleaningMarkPercent          = 80 ;
  config->MinTrimSize                  = 0 ;
  config->MaintenanceLoop              = 10 ;
  config->HeartbeatTimeoutMilli        = 0 ;
  config->BatchingMode                 = 0 ;
  config->BatchingMode                 = 0 ;
  config->BatchYield                   = 0 ;
  config->BatchGlobal                  = 0 ;
  config->opt_present = (unsigned char)PGM_OPT_PRESENT_CSC ;
  config->opt_net_sig = (unsigned char)(PGM_OPT_NET_SIG_CSC|PGM_OPT_PRESENT_CSC) ;

  /* the following Tx params are not used in RUM */
  config->InterHeartbeatAmbientMilli   = 5000 ;
  config->InterHeartbeatMinMilli       = 500 ;
  config->InterHeartbeatMaxMilli       = 5000 ;


  config->ReuseAddress                 = 1 ;
  config->UseNoMA                      = 1 ;
  config->NumMAthreads                 = 1 ;
  config->NumExthreads                 = 0 ;  /* newly added */
  config->ThreadPerTopic               = 0 ;  /* depricated, not used */
  config->recvBuffsQsize               = 128 ;  /* piggyback for nackStrucQsize and evntStrucQsize */
  config->rsrvQsize                    = 1024 ;
  config->fragQsize                    = 1024 ;
  config->BaseWaitMili                 = 9 ;
  config->LongWaitMili                 = 256 ; /* not used */
  config->SnapshotCycleMilli_rx        = 0 ;
  config->PrintStreamInfo_rx           = 1 ;
  config->MemoryCrisisCycle            = 100 ; /* not used */
  config->MemoryAlertPctHi             = 0 ;
  config->MemoryAlertPctLo             = 0 ;
  config->NackGenerCycle               = 32 ;
  config->TaskTimerCycle               = 2 ;
  config->BindRetryTime                = 30000 ;
  config->RecvPacketNpoll              = 1 ;
  config->ThreadPriority_tx            = 0 ;
  config->ThreadPriority_rx            = 0 ;
  config->ThreadAffinity_tx            = 0 ;
  config->ThreadAffinity_rx            = 0 ;
  config->ThreadStackSize              = 0 ;
  config->LatencyMonSampleRate         = 0 ;                 /* user may use LatencyMonSampleRate[_tx|_rx] */
  config->MonSampleRate                = 0 ;
  config->ReceiverConnectionEstablishTimeoutMilli = DEFAULT_CON_ESTABLISH_TIMEOUT_MILLI;

  /* the following Rx params are not exposed in the RUM documentation since they have minor impact and the defaults should be used.  */
  config->DataPacketSize     = 8192 ;                        /* Deprecated. set to PacketSizeBytes */
  config->nackBuffsQsize     =  64 ;
  config->nackQsize          = 2048 ;
  config->recvQsize          = 1024 ;
  config->evntQsize          = 1024 ;
  config->packQsize          = 0 ;
  config->BaseWaitNano       = milli2nano(config->BaseWaitMili) ;
  config->LongWaitNano       = milli2nano(config->LongWaitMili) ; /* not used */
  config->NackTimeoutBOF     =     0 ;
  config->NackTimeoutNCF     =   200 ;
  config->NackTimeoutData    =   500 ;
  config->NackRetriesNCF     =   256 ;
  config->NackRetriesData    =   512 ;
  config->MaxNacksPerCycle   =   512 ;
  config->MaxSqnPerNack      =    63 ;
  config->UsePerConnInQ      =   0 ; 


  if ( !rumInfo->apiConfig.AdvanceConfigFile[0] )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5504), "",
        "RUM is using default values for the advanced configuration parameters. ");

    return RUM_SUCCESS;
  }

  strlcpy(file_none, rumInfo->apiConfig.AdvanceConfigFile,8);
  upper(strip(file_none));

  if ( strcmp(file_none, "NONE") == 0 )
  {
    memcpy(rumInfo->apiConfig.AdvanceConfigFile, "NONE",5);  /* make sure upper case is used */
    /* check for RUM_ADVANCE_CONFIG_FILE in current directory */
    fp = llm_fopen(RUM_ADVANCE_CONFIG_FILE,"r",NULL);
    if ( fp == NULL )
    {
      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5505), "%s",
          "The default advanced configuration file {0} was not found. The RUM instance is using the default advanced configuration parameters.",
          RUM_ADVANCE_CONFIG_FILE);

      return RUM_SUCCESS;
    }
  }
  else
  {
    if ( (fp = llm_fopen(rumInfo->apiConfig.AdvanceConfigFile,"r",NULL)) == NULL )
    {
      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3506), "%s",
          "The user defined AdvancedConfigFile {0} was not found. RUM is using the default values.", rumInfo->apiConfig.AdvanceConfigFile);

      return RUM_FAILURE ;
    }
    else
      using_default = 0;
  }

  if ( fp == NULL )
    return RMM_FAILURE;
  else
  {
    if ( using_default )
      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5507), "%s",
      "RUM is using the default AdvancedConfigFile {0}.", RUM_ADVANCE_CONFIG_FILE);
    else
      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_INFO, MSG_KEY(5508), "%s",
      "RUM is using the user defined AdvancedConfigFile {0}.", rumInfo->apiConfig.AdvanceConfigFile);
  }

  /* read advanced configuration parameters */
  while ((fgets(line, LINE_SIZE, fp) != NULL) )
  {
    line[LINE_SIZE-1] = 0 ;
    if ( read_config_line(1,rumInfo, line, config) == RMM_FAILURE )
    {
      llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_WARN, MSG_KEY(4509), "%s",
        "The RUM advanced configuration file contains a bad parameter at line {0}", line);
    }
  }
  if ( config->ThreadPerTopic )
  {
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_WARN, MSG_KEY(4510), "",
      "The advanced configuration parameter 'ThreadPerTopic' is deprecated.");
  }

  fclose(fp) ;

  return RMM_SUCCESS ;
}

/*****************************************************************************************/
int read_config_line(int init, rumInstanceRec *rumInfo, char *line, rumAdvanceConfig *config)
{
  char org_line[LINE_SIZE], *ip ;
  int inst=rumInfo->instance ;

  if ( line[0] == '#' || line[0] =='\n' )
    return RMM_SUCCESS ;

  strlcpy(org_line,line,LINE_SIZE);
  upper(strip(line)) ;
  for ( ip=line ; ip[0]!='=' && !isEOL(ip[0]) ; ip++) ; /* empty body */
  if ( ip[0] != '=' )
    return RMM_SUCCESS;                            /* avoid printing warning on space line */

  ip[0] = 0 ;
  ip++ ;
  strip(line) ;
  strip(ip) ;

  /* the parameters below can be changed dynamically */
  {
    if ( strcmp(line,"SNAPSHOTCYCLEMILLI_TX") == 0 || strcmp(line,"SNAPSHOTCYCLEMILLITX") == 0 )
    {
      config->SnapshotCycleMilli_tx = atoi(ip) ;
      if ( config->SnapshotCycleMilli_tx < 0 )
        config->SnapshotCycleMilli_tx = 0;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"PRINTSTREAMINFO_TX") == 0 || strcmp(line,"PRINTSTREAMINFOTX") == 0 )
    {
      config->PrintStreamInfo_tx = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"SNAPSHOTCYCLEMILLI_RX") == 0 || strcmp(line,"SNAPSHOTCYCLEMILLIRX") == 0 )
    {
      config->SnapshotCycleMilli_rx = atoi(ip) ;
      if ( config->SnapshotCycleMilli_rx < 0 )
        config->SnapshotCycleMilli_rx = 0;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"SNAPSHOTCYCLE") == 0 || strcmp(line,"SNAPSHOTCYCLEMILLI") == 0 ) /* if _TX or _RX not defined set both */
    {
      if ( config->SnapshotCycleMilli_rx == 0 )
      {
        config->SnapshotCycleMilli_rx = atoi(ip) ;
        if ( config->SnapshotCycleMilli_rx < 0 )
          config->SnapshotCycleMilli_rx = 0;
      }

      if ( config->SnapshotCycleMilli_tx == 0 )
      {
        config->SnapshotCycleMilli_tx = atoi(ip) ;
        if ( config->SnapshotCycleMilli_tx < 0 )
          config->SnapshotCycleMilli_tx = 0;
      }

      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"PRINTSTREAMINFO_RX") == 0 || strcmp(line,"PRINTSTREAMINFORX") == 0 )
    {
      config->PrintStreamInfo_rx = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"RECVPACKETNPOLL") == 0 )
    {
      config->RecvPacketNpoll = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"LOGLEVEL") == 0 || strcmp(line,"DEFAULTLOGLEVEL") == 0 )
    {
      upper(ip) ;
      if ( strcmp(ip,"NONE") == 0  )
        config->LogLevel = RUM_LOGLEV_NONE ;
      else if ( strcmp(ip,"BASIC") == 0 )
        config->LogLevel = RUM_LOGLEV_EVENT ;
      else if ( strcmp(ip,"MAXIMAL") == 0 )
        config->LogLevel = RUM_LOGLEV_XTRACE ;
      else
      {
        config->LogLevel = atoi(ip) ;
        if ( config->LogLevel < RUM_LOGLEV_NONE )
          config->LogLevel = RUM_LOGLEV_NONE ;
        if ( config->LogLevel > RUM_LOGLEV_XTRACE )
          config->LogLevel = RUM_LOGLEV_XTRACE ;
      }
      config->DefaultLogLevel = config->LogLevel ;
      return RMM_SUCCESS ;
    }
  }

  /* the parameters below can NOT be changed dynamically */
  if ( init )
  {
    if ( strcmp(line,"MAXPENDINGQUEUEKBYTES") == 0 )
    {
      config->MaxPendingQueueKBytes = atoi(ip) ;
      if ( config->MaxPendingQueueKBytes < 0 )
        config->MaxPendingQueueKBytes = 0;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MAXSTREAMSPERTRANSMITTER") == 0 )
    {
      config->MaxStreamsPerTransmitter = atoi(ip) ;
      if ( config->MaxStreamsPerTransmitter < 1 )
        config->MaxStreamsPerTransmitter = rmmMax(10, RMM_MAX_TX_TOPICS/5) ;
      if ( config->MaxStreamsPerTransmitter > RMM_MAX_TX_TOPICS )
        config->MaxStreamsPerTransmitter = RMM_MAX_TX_TOPICS ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MAXFIREOUTTHREADS") == 0 )
    {
      config->MaxFireOutThreads = atoi(ip) ;
      if ( config->MaxFireOutThreads < 1 )
        config->MaxFireOutThreads = 1;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"STREAMSPERFIREOUT") == 0 )
    {
      config->StreamsPerFireout = atoi(ip) ;
      if ( config->StreamsPerFireout < 1 )
        config->StreamsPerFireout = 1;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"FIREOUTPOLLMICRO") == 0 )
    {
      config->FireoutPollMicro = atoi(ip) ;
      if ( config->FireoutPollMicro < 0 )
      config->FireoutPollMicro = 0;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"PACKETSPERROUND") == 0 )
    {
      config->PacketsPerRound = atoi(ip) ;
      if ( config->PacketsPerRound < 1 )
        config->PacketsPerRound = 10;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"PACKETSPERROUNDWHENCLEANING") == 0 )
    {
      config->PacketsPerRoundWhenCleaning = atoi(ip) ;
      if ( config->PacketsPerRoundWhenCleaning < 1 )
        config->PacketsPerRoundWhenCleaning = 5;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"RDATASENDPERCENT") == 0 )
    {
      config->RdataSendPercent = atoi(ip) ;
      if ( config->RdataSendPercent < 0 || config->RdataSendPercent > 100 )
        config->RdataSendPercent = 80;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"CLEANINGMARKPERCENT") == 0 )
    {
      config->CleaningMarkPercent = atoi(ip) ;
      if ( config->CleaningMarkPercent < 0 || config->CleaningMarkPercent > 100 )
        config->CleaningMarkPercent = 80;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MINTRIMSIZE") == 0 )
    {
      config->MinTrimSize = atoi(ip) ;
      if ( config->MinTrimSize < 0 )
        config->MinTrimSize = 0;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MAINTENANCELOOP") == 0 )
    {
      config->MaintenanceLoop = atoi(ip) ;
      if ( config->MaintenanceLoop < 0 )
        config->MaintenanceLoop = 10;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"HEARTBEATTIMEOUTMILLI") == 0 )
    {
      config->HeartbeatTimeoutMilli = atoi(ip) ;
      if ( config->HeartbeatTimeoutMilli < 0 )
        config->HeartbeatTimeoutMilli = 0;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"INTERHEARTBEATAMBIENTMILLI") == 0 )
    {
      config->InterHeartbeatAmbientMilli = atoi(ip) ;
      if ( config->InterHeartbeatAmbientMilli < 0 )
        config->InterHeartbeatAmbientMilli = 5000;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"INTERHEARTBEATMINMILLI") == 0 )
    {
      config->InterHeartbeatMinMilli = atoi(ip) ;
      if ( config->InterHeartbeatMinMilli < 0 )
        config->InterHeartbeatMinMilli = 500;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"INTERHEARTBEATMAXMILLI") == 0 )
    {
      config->InterHeartbeatMaxMilli = atoi(ip) ;
      if ( config->InterHeartbeatMaxMilli < 0 )
        config->InterHeartbeatMaxMilli = 5000;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MONITORPRINTINTERVALSEC") == 0 )
    {
      if ( config->SnapshotCycleMilli_tx == 0 )
      {
        config->SnapshotCycleMilli_tx = 1000*atoi(ip) ;
        if ( config->SnapshotCycleMilli_tx < 0 )
          config->SnapshotCycleMilli_tx = 0;
      }
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MINBATCHINGMICRO") == 0 )
    {
      config->MinBatchingMicro = atoi(ip) ;
      if ( config->MinBatchingMicro < 0 )
        config->MinBatchingMicro = 0;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MAXBATCHINGMICRO") == 0 )
    {
      config->MaxBatchingMicro = atoi(ip) ;
      if ( config->MaxBatchingMicro < 0 )
        config->MaxBatchingMicro = 0;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"ADAPTIVEPACKETRATE") == 0 )
    {
      strlcpy(config->apiBatchMode,ip,sizeof(config->apiBatchMode));
      config->BatchYield  = 1 ;
      config->BatchGlobal = 0 ;

      if ( strcmp(ip,"TX_FEEDBACK") == 0 || strcmp(ip,"TXFEEDBACK") == 0 )
      {
        config->BatchingMode = 1;
        return RMM_SUCCESS ;
      }
      if ( strcmp(ip,"TX_SMOOTH") == 0 || strcmp(ip,"TXSMOOTH") == 0 )
      {
        config->BatchingMode = 1;
        config->BatchYield = 0;
        return RMM_SUCCESS ;
      }
      if ( strcmp(ip,"RX_FEEDBACK") == 0 || strcmp(ip,"RXFEEDBACK") == 0 )
      {
        config->BatchingMode = -1;
        return RMM_SUCCESS ;
      }
      if ( strcmp(ip,"RX_SMOOTH") == 0 || strcmp(ip,"RXSMOOTH") == 0 )
      {
        config->BatchingMode = -1;
        config->BatchYield = 0;
        return RMM_SUCCESS ;
      }
      if ( strcmp(ip,"GLOBAL") == 0 )
      {
        config->BatchingMode = 1;
        config->BatchGlobal  = 1;
        return RMM_SUCCESS ;
      }
      if ( strcmp(ip,"DISABLED") == 0 )
      {
        config->BatchingMode = 0;
        return RMM_SUCCESS ;
      }

      return RMM_FAILURE ;
    }
    if ( strcmp(line,"STREAMREPORTINTERVALMILLI") == 0 )
    {
      config->StreamReportIntervalMilli = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"DEFAULTDEBUGLEVEL") == 0 )
    {
      return RMM_SUCCESS ;
    }

    /*--- Receiver related -----*/

    if ( strcmp(line,"REUSEADDRESS") == 0 )
    {
      config->ReuseAddress = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"USENOMA") == 0 )
    {
      config->UseNoMA = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"NUMMATHREADS") == 0 )
    {
      config->NumMAthreads = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"NUMEXTHREADS") == 0 )
    {
      config->NumExthreads = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"THREADPERTOPIC") == 0 )
    {
      config->ThreadPerTopic = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"DATAPACKETSIZE") == 0 || strcmp(line,"RECVPACKETSIZE") == 0 )
    {
      config->DataPacketSize = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"RECVBUFFSQSIZE") == 0 )
    {
      config->recvBuffsQsize = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"NACKBUFFSQSIZE") == 0 )
    {
      config->nackBuffsQsize = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"NACKQSIZE") == 0 )
    {
      config->nackQsize      = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"RECVQSIZE") == 0 )
    {
      config->recvQsize      = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"RSRVQSIZE") == 0 )
    {
      config->rsrvQsize      = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"FRAGQSIZE") == 0 )
    {
      config->fragQsize      = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"EVNTQSIZE") == 0 )
    {
      config->evntQsize      = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"PACKQSIZE") == 0 )
    {
      config->packQsize      = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"BASEWAITMILI") == 0 || strcmp(line,"BASEWAITMILLI") == 0 )
    {
      config->BaseWaitMili = atoi(ip) ;
      if ( config->BaseWaitMili < 0 )
        config->BaseWaitMili = 0 ;
      config->BaseWaitNano = milli2nano(config->BaseWaitMili) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"LONGWAITMILI") == 0 || strcmp(line,"LONGWAITMILLI") == 0 )
    {
      config->LongWaitMili = atoi(ip) ;
      if ( config->LongWaitMili < 0 )
        config->LongWaitMili = 0 ;
      config->LongWaitNano = milli2nano(config->LongWaitMili) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MEMORYCRISISCYCLE") == 0 )
    {
      config->MemoryCrisisCycle = atoi(ip) ;
      if ( config->MemoryCrisisCycle < 0 )
        config->MemoryCrisisCycle = 0 ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MEMORYALERTPCTHIGH") == 0 )
    {
      config->MemoryAlertPctHi = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MEMORYALERTPCTLOW") == 0 )
    {
      config->MemoryAlertPctLo = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"NACKGENERCYCLE") == 0 )
    {
      config->NackGenerCycle = atoi(ip) ;
      if ( config->NackGenerCycle < 0 )
        config->NackGenerCycle = 0 ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"TASKTIMERCYCLE") == 0 )
    {
      config->TaskTimerCycle = atoi(ip) ;
      if ( config->TaskTimerCycle < 0 )
        config->TaskTimerCycle = 0 ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"NACKTIMEOUTBOF") == 0 )
    {
      config->NackTimeoutBOF = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"NACKTIMEOUTNCF") == 0 )
    {
      config->NackTimeoutNCF = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"NACKTIMEOUTDATA") == 0 )
    {
      config->NackTimeoutData = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"NACKRETRIESNCF") == 0 )
    {
      config->NackRetriesNCF = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"NACKRETRIESDATA") == 0 )
    {
      config->NackRetriesData = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"BINDRETRYTIME") == 0 )
    {
      config->BindRetryTime = atoi(ip) ;
      if ( config->BindRetryTime < 0 )
        config->BindRetryTime = 0 ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MAXNACKSPERCYCLE") == 0 )
    {
      config->MaxNacksPerCycle = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"MAXSQNPERNACK") == 0 )
    {
      config->MaxSqnPerNack = atoi(ip) ;
      return RMM_SUCCESS ;
    }
    if ( strcmp(line,"LOGFILE") == 0 )
    {
      if ( strcmp(ip,"NONE") == 0 )
      {
        if ( inst != 0 ) rumInfo->fp_log = NULL;
        return RMM_SUCCESS;
      }
      if ( strcmp(ip,"STD") == 0 )
      {
        if ( inst != 0 || rumInfo->fp_log == NULL )
          rumInfo->fp_log = stdout;
      }
      else
      {
        strip(org_line);
        for ( ip=org_line ; ip[0]!='=' ; ip++) ;
        strip(++ip);
        strlcpy(config->LogFile,ip,RUM_FILE_NAME_LEN) ;
      }
      return RMM_SUCCESS ;
    }

    if ( strcmp(line,"RECEIVERCONNECTIONESTABLISHTIMEOUTMILLI") == 0 )
    {
      config->ReceiverConnectionEstablishTimeoutMilli = atoi(ip) ;
      return RMM_SUCCESS;
    }

    if ( strcmp(line,"THREADPRIORITY_TX") == 0 || strcmp(line,"THREADPRIORITYTX") == 0 )
    {
      int val = atoi(ip) ;
      if ( val < 0 )
        val = 0 ;
      else if ( val > 100 )
        val = 100 ;
      config->ThreadPriority_tx = val ;
      return RMM_SUCCESS ;
    }

    if ( strcmp(line,"THREADPRIORITY_RX") == 0 || strcmp(line,"THREADPRIORITYRX") == 0 )
    {
      int val = atoi(ip) ;
      if ( val < 0 )
        val = 0 ;
      else if ( val > 100 )
        val = 100 ;
      config->ThreadPriority_rx = val ;
      return RMM_SUCCESS ;
    }

    if ( strcmp(line,"THREADSTACKSIZE") == 0 )
    {
      int val = atoi(ip) ;
      if ( val < 0 )
        val = 0 ;
      config->ThreadStackSize = val ;
      return RMM_SUCCESS ;
    }

    if ( strcmp(line,"THREADAFFINITY_TX") == 0 || strcmp(line,"THREADAFFINITYTX") == 0 )
    {
      config->ThreadAffinity_tx = strtoull(ip,NULL,0) ;
      return RMM_SUCCESS ;
    }

    if ( strcmp(line,"THREADAFFINITY_RX") == 0 || strcmp(line,"THREADAFFINITYRX") == 0 )
    {
      config->ThreadAffinity_rx = strtoull(ip,NULL,0) ;
      return RMM_SUCCESS ;
    }

    if ( strcmp(line,"LIMITPENDINGPACKETS") == 0 )
    {
      config->UsePerConnInQ = atoi(ip) ;
      return RMM_SUCCESS ;
    }
  }

  /* Deprecated parameters that should not issue a warning */
  if ( strcmp(line,"DEBUGLEVEL") == 0 || strcmp(line,"NUMPRTHREADS") == 0 )
  {
    return RMM_SUCCESS ;
  }

  /* no match was found */
  return RMM_FAILURE;
}

int check_configuration_parameters(rumInstanceRec *rumInfo)
{
    int i,n;
    FILE *fp;
    int inst = rumInfo->instance;
    char timestr[32];
    char  string_param[8];
    int rc = RUM_SUCCESS;
    rumBasicConfig *config = &rumInfo->basicConfig;
    rumAdvanceConfig *adv_config = &rumInfo->advanceConfig; 
    TCHandle tcHandle = rumInfo->tcHandles[0];
    TBHandle tbh = llmCreateTraceBuffer(tcHandle,LLM_LOGLEV_INFO,MSG_KEY(5510));

    get_time_string(timestr);

    llmAddTraceData(tbh,"%s%d",
        "The RUM (Version {0}) instance {1} is created using the following configuration parameters: Basic configuration=[",
        RMM_API_VERSION,inst);

#if   USE_SSL
    if ( config->Protocol != RUM_TCP && config->Protocol != RUM_SSL){ 
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3051),"%s%d%d%s%d%s",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are {2}({3}) and {4}({5}).",
            "Protocol",config->Protocol,RUM_TCP,"RUM_TCP",RUM_SSL,"RUM_SSL");
        rc = RUM_FAILURE;   
    }
#else
    if ( config->Protocol != RUM_TCP ){ 
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3053),"%s%d%d%s",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid value is {2}({3}).",
            "Protocol",config->Protocol,RUM_TCP,"RUM_TCP");
        rc = RUM_FAILURE;   
    }

#endif
    else  llmAddTraceData(tbh,"%d","Protocol={0}, ", config->Protocol);

    if ( config->IPVersion != RUM_IPver_IPv4  && config->IPVersion != RUM_IPver_IPv6  &&
        config->IPVersion != RUM_IPver_ANY  && config->IPVersion != RUM_IPver_BOTH ) {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3054),"%s%d%d%s%d%s%d%s%d%s",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are {2}({3}), {4}({5}), {6}({7}), or {8}({9}).",
            "IPVersion",config->IPVersion,RUM_IPver_IPv4,"IPv4",RUM_IPver_IPv6,"IPv6",RUM_IPver_ANY,"ANY",RUM_IPver_BOTH,"BOTH");
        rc = RUM_FAILURE;
    }else  llmAddTraceData(tbh,"%d","IPVersion={0}, ", config->IPVersion);

    if ( rumInfo->apiConfig.SupplementalPorts && rumInfo->apiConfig.SupplementalPortsLength > 0 ){
        int port[2] ;
        rumPort *rp ;

        port[0] = config->ServerSocketPort ;
        if ( port[0] < 0 || port[0] > 0xffff)   {
            llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
                "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
                "ServerSocketPort",config->ServerSocketPort,0,0xffff);
            rc = RUM_FAILURE;   
        } else llmAddTraceData(tbh,"%d","ServerSocketPort={0}, ", port[0]);


        rp = rumInfo->apiConfig.SupplementalPorts ;
        for ( i=0 ; i<rumInfo->apiConfig.SupplementalPortsLength ; i++ , rp++)
        {
            port[0] = rp->port_number ;
            if ( port[0] < 0 || port[0] > 0xffff)
            {
                llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
                    "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
                    "SupplementalPorts.port_number",rp->port_number,0,65535);
                rc = RUM_FAILURE;   }
            if ( port[0] == 0 )
            {
                port[0] = rp->port_range_low ;
                port[1] = rp->port_range_high;
                if ( port[1] != 0 && !(port[0] > 0 && port[0] <= port[1] && port[1] <= 0xffff) )
                {
                    llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3052),"%s%d%d",
                        "The RUM configuration parameter {0} is not valid. The requested values are {1} {2}. The valid values are [0 65535] and (port_range_low < port_range_high).",
                        "SupplementalPorts.port_range",port[0], port[1]);
                    rc = RUM_FAILURE;   
                }  else { 
                    if ( port[1] == 0 ) {
                        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4052),
                            "%d%d%d%d%d","The dynamic port range ({0} {1}) is not valid for SupplementalPorts [{2}]. RUM will use the following default values: ({3} {4}).",
                            port[0],port[1],i,PORT_RANGE_LO,PORT_RANGE_HI);
                    }
                }
            }
            if ( rp->networkInterface[0] )
            {
                char *ni = rp->networkInterface ; 
                if ( memcmp(ni,"NONE",5) && memcmp(ni,"ALL",4) ) 
                {
                    ipFlat tt[1] ; 
                    if ( rmmGetAddr(AF_UNSPEC,ni,tt) == AF_UNSPEC )
                    {
                        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3059),"%s%d",
                            "The network interface ({0}) is not valid for SupplementalPorts[{1}]. The valid values are NONE, ALL, or a valid IP address.",
                            ni,i);
                        rc = RUM_FAILURE;
                    }
                }
            }
        }
    } else {
        if ( config->ServerSocketPort < 1 || config->ServerSocketPort > 0xffff)   {
            llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
                "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
                "ServerSocketPort",config->ServerSocketPort,1,0xffff);
            rc = RUM_FAILURE;   
        } else llmAddTraceData(tbh,"%d","ServerSocketPort={0}, ",config->ServerSocketPort);
    }
    llmAddTraceData(tbh,"%d","SupplementalPortsLength={0}, ",rumInfo->apiConfig.SupplementalPortsLength);
    if(rumInfo->apiConfig.SupplementalPorts && rumInfo->apiConfig.SupplementalPortsLength > 0){
        rumPort *rp = rumInfo->apiConfig.SupplementalPorts;
        llmAddTraceData(tbh,"","SupplementalPorts=( ");
        for(i = 0; i < rumInfo->apiConfig.SupplementalPortsLength; i++, rp++){
            llmAddTraceData(tbh,"%s%d%d%d","[{0}, {1}, {2}, {3}] ",
                rp->networkInterface,rp->port_number,rp->port_range_low,rp->port_range_high);
        }
        llmAddTraceData(tbh,"","), ");
    }

    if ( config->LogLevel < RUM_LOGLEV_NONE || config->LogLevel > RUM_LOGLEV_XTRACE )
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
            "LogLevel",config->LogLevel,RUM_LOGLEV_NONE,RUM_LOGLEV_XTRACE);
        rc = RUM_FAILURE;
    }  else  llmAddTraceData(tbh,"%d","LogLevel={0}, ",config->LogLevel);

    if ( config->LimitTransRate != RUM_DISABLED_RATE_LIMIT && config->LimitTransRate != RUM_STATIC_RATE_LIMIT &&  config->LimitTransRate != RUM_DYNAMIC_RATE_LIMIT)
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3056),"%s%d%d%d%d",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are {2}, {3}, or {4}.",
            "LimitTransRate",config->LimitTransRate,RUM_DISABLED_RATE_LIMIT,RUM_STATIC_RATE_LIMIT, RUM_DYNAMIC_RATE_LIMIT);
        rc = RUM_FAILURE;
    } else llmAddTraceData(tbh,"%d","LimitTransRate={0}, ", config->LimitTransRate);

    if ( config->LimitTransRate != RUM_DISABLED_RATE_LIMIT && config->TransRateLimitKbps < 8 )
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
            "TransRateLimitKbps",config->TransRateLimitKbps,8,0x7FFFFFFF);
        rc = RUM_FAILURE;   
    } else  llmAddTraceData(tbh,"%d","TransRateLimitKbps={0}, ", config->TransRateLimitKbps);

    if ( config->PacketSizeBytes < 300 || config->PacketSizeBytes > MAX_PACKET_SIZE  )
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
            "PacketSizeBytes",config->PacketSizeBytes,300,MAX_PACKET_SIZE);
        rc = RUM_FAILURE;   
    } else llmAddTraceData(tbh,"%d","PacketSizeBytes={0}, ", config->PacketSizeBytes );

    if ( config->TransportDirection < RUM_Tx_Rx || config->TransportDirection > RUM_Rx_ONLY  ) {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
            "TransportDirection",config->TransportDirection,RUM_Tx_Rx,RUM_Rx_ONLY);
        rc = RUM_FAILURE;   
    } else llmAddTraceData(tbh,"%d","TransportDirection={0}, ",config->TransportDirection );

    i = (sizeof(int)==sizeof(void *)) ? 4000 : 1000000 ;  /* 4K on 32bit and M on 64 bit */
    if ( config->MaxMemoryAllowedMBytes < MIN_INSTANCE_MEMORY || config->MaxMemoryAllowedMBytes > i )
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
            "MaxMemoryAllowedMBytes",config->MaxMemoryAllowedMBytes,MIN_INSTANCE_MEMORY,i);
        rc = RUM_FAILURE;
    }
    else  
    {
      llmAddTraceData(tbh,"%d","MaxMemoryAllowedMBytes={0}, ",config->MaxMemoryAllowedMBytes );
    }

    if ((config->MinimalHistoryMBytes < 0) || (config->MinimalHistoryMBytes > i))
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
            "MinimalHistoryMBytes",config->MinimalHistoryMBytes,0,i);
        rc = RUM_FAILURE;
    }
    else 
    {
        llmAddTraceData(tbh,"%d","MinimalHistoryMBytes={0}, ", config->MinimalHistoryMBytes);
    }

    if ((config->RxMaxMemoryAllowedMBytes < 1) || (config->RxMaxMemoryAllowedMBytes > config->MaxMemoryAllowedMBytes))
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
            "RxMaxMemoryAllowedMBytes",config->RxMaxMemoryAllowedMBytes,1,config->MaxMemoryAllowedMBytes);
        rc = RUM_FAILURE;
    }  else {
        llmAddTraceData(tbh,"%d","RxMaxMemoryAllowedMBytes={0}, ", 
            config->RxMaxMemoryAllowedMBytes);
    }

    if ((config->TxMaxMemoryAllowedMBytes < 1) || (config->TxMaxMemoryAllowedMBytes > config->MaxMemoryAllowedMBytes))
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
            "TxMaxMemoryAllowedMBytes",config->TxMaxMemoryAllowedMBytes,1,config->MaxMemoryAllowedMBytes);
        rc = RUM_FAILURE;
    }  else {
        llmAddTraceData(tbh,"%d","TxMaxMemoryAllowedMBytes={0}, ", 
            config->TxMaxMemoryAllowedMBytes);
    }

    if ((config->RxMaxMemoryAllowedMBytes + config->TxMaxMemoryAllowedMBytes) > config->MaxMemoryAllowedMBytes )
    {
        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(1390),"%s%d%d%d",
            "The RUM configuration parameter {0} is not valid. The requested value is {1}. The valid values are [{2} {3}].",
            "RxMaxMemoryAllowedMBytes+TxMaxMemoryAllowedMBytes",
            (config->RxMaxMemoryAllowedMBytes + config->TxMaxMemoryAllowedMBytes),2,config->MaxMemoryAllowedMBytes);
        rc = RUM_FAILURE;
    } 

    if ( config->SocketReceiveBufferSizeKBytes < 0 )
        config->SocketReceiveBufferSizeKBytes = 0;
    llmAddTraceData(tbh,"%d","SocketReceiveBufferSizeKBytes={0}, ",config->SocketReceiveBufferSizeKBytes);

    if ( config->SocketSendBufferSizeKBytes < 0 )
        config->SocketSendBufferSizeKBytes = 0;
    llmAddTraceData(tbh,"%d","SocketSendBufferSizeKBytes={0}, ",config->SocketSendBufferSizeKBytes);


    llmAddTraceData(tbh,"%llx","on_event={0}, ",LLU_VALUE(rumInfo->apiConfig.on_event));
    llmAddTraceData(tbh,"%llx","event_user={0}, ",LLU_VALUE(rumInfo->apiConfig.event_user));
    llmAddTraceData(tbh,"%llx","free_callback={0}, ",LLU_VALUE(rumInfo->apiConfig.free_callback));
    llmAddTraceData(tbh,"%d","Nparams={0}, ",rumInfo->apiConfig.Nparams);
    llmAddTraceData(tbh,"","AncillaryParams=( ");
    for(i = 0; i < rumInfo->apiConfig.Nparams; i++){
        llmAddTraceData(tbh,"%s","{0} ",rumInfo->apiConfig.AncillaryParams[i]);
    }
    llmAddTraceData(tbh,"","), ");

    llmAddTraceData(tbh,"%s","RxNetworkInterface={0}, ",config->RxNetworkInterface);
    strlcpy(string_param, config->RxNetworkInterface,sizeof(string_param));
    upper(string_param);
    if ( strcmp(string_param, "NONE") == 0 || strcmp(config->RxNetworkInterface, "ALL") == 0 )
        upper(rumInfo->basicConfig.RxNetworkInterface);


    llmAddTraceData(tbh,"%s","TxNetworkInterface={0}, ",config->TxNetworkInterface);
    strlcpy(string_param, config->TxNetworkInterface,sizeof(string_param));
    upper(string_param);
    if ( strcmp(string_param, "NONE") == 0 || strcmp(string_param, "ALL") == 0 )
        upper(rumInfo->basicConfig.TxNetworkInterface);


    llmAddTraceData(tbh,"%s","AdvanceConfigFile={0}], ",config->AdvanceConfigFile);
    strlcpy(string_param, config->AdvanceConfigFile,sizeof(string_param));
    upper(string_param);
    if ( string_param[0] && strcmp(string_param, "NONE") != 0 )
    {
        if ( (fp = llm_fopen(rumInfo->basicConfig.AdvanceConfigFile,"r",NULL)) == NULL )
        {
            llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_ERROR,MSG_KEY(3055),"%s",
                "The AdvancedConfigFile configuration parameter {0} is not valid. The value must be NONE, empty or a valid file name.",
                rumInfo->basicConfig.AdvanceConfigFile);
            rc = RUM_FAILURE ;
        }
        else fclose(fp);
    }
    else upper(rumInfo->basicConfig.AdvanceConfigFile);

    llmAddTraceData(tbh,"","Advanced configuration=[");

    /* check advanced configuration parameters */

    if ( adv_config->MemoryAlertPctHi < 0 ) adv_config->MemoryAlertPctHi = 0 ;
    if ( adv_config->MemoryAlertPctHi >100) adv_config->MemoryAlertPctHi = 100;
    if ( adv_config->MemoryAlertPctLo < 0 ) adv_config->MemoryAlertPctLo = 0 ;
    n = 4*adv_config->MemoryAlertPctHi/5 ;
    if ( adv_config->MemoryAlertPctLo > n ) adv_config->MemoryAlertPctLo = n ;

    n=0 ; 

    llmAddTraceData(tbh,"%d","MaxPendingQueueKBytes={0}, ",adv_config->MaxPendingQueueKBytes);
    llmAddTraceData(tbh,"%d","MaxStreamsPerTransmitter={0}, ",adv_config->MaxStreamsPerTransmitter);
    llmAddTraceData(tbh,"%d","MaxFireOutThreads={0}, ", adv_config->MaxFireOutThreads);
    llmAddTraceData(tbh,"%d","StreamsPerFireout={0}, ", adv_config->StreamsPerFireout);
    llmAddTraceData(tbh,"%d","FireoutPollMicro={0}, ",adv_config->FireoutPollMicro);
    llmAddTraceData(tbh,"%d","PacketsPerRound={0}, ",adv_config->PacketsPerRound);
    llmAddTraceData(tbh,"%d","PacketsPerRoundWhenCleaning={0}, ",adv_config->PacketsPerRoundWhenCleaning);
    llmAddTraceData(tbh,"%d","RdataSendPercent={0}, ", adv_config->RdataSendPercent);
    llmAddTraceData(tbh,"%d","CleaningMarkPercent={0}, ", adv_config->CleaningMarkPercent);
    llmAddTraceData(tbh,"%d","MinTrimSize={0}, ", adv_config->MinTrimSize);
    llmAddTraceData(tbh,"%d","MaintenanceLoop={0}, ",adv_config->MaintenanceLoop);
    llmAddTraceData(tbh,"%d","HeartbeatTimeoutMilli={0}, ",adv_config->HeartbeatTimeoutMilli);
    llmAddTraceData(tbh,"%d","SnapshotCycleMilli_tx={0}, ",adv_config->SnapshotCycleMilli_tx);
    llmAddTraceData(tbh,"%d","PrintStreamInfo_tx={0}, ",adv_config->PrintStreamInfo_tx);
    llmAddTraceData(tbh,"%s%d%d%d%d","AdaptivePacketRate=[{0} {1} + {2} ({3} {4})], ",adv_config->apiBatchMode,adv_config->BatchingMode,adv_config->BatchYield,adv_config->MinBatchingMicro,adv_config->MaxBatchingMicro);
    llmAddTraceData(tbh,"%d","StreamReportIntervalMilli={0}, ", adv_config->StreamReportIntervalMilli);
    llmAddTraceData(tbh,"%d","DefaultLogLevel={0}, ", adv_config->DefaultLogLevel);
    llmAddTraceData(tbh,"%d","ReuseAddress={0}, ", adv_config->ReuseAddress);
    llmAddTraceData(tbh,"%d","UseNoMA={0}, ",adv_config->UseNoMA);
    llmAddTraceData(tbh,"%d","NumMAthreads={0}, ",adv_config->NumMAthreads);
    llmAddTraceData(tbh,"%d","NumExthreads={0}, ",adv_config->NumExthreads);
    llmAddTraceData(tbh,"%d","ThreadPerTopic={0}, ",adv_config->ThreadPerTopic);

    if ( adv_config->DataPacketSize != config->PacketSizeBytes )
    {

        llmSimpleTraceInvoke(tcHandle,LLM_LOGLEV_WARN,MSG_KEY(4053),"%d%d",
            "The value of DataPacketSize ({0}) has been adjusted to the value specified by configuration parameter PacketSizeBytes (Tx) ({1}).",
            adv_config->DataPacketSize, config->PacketSizeBytes);
        adv_config->DataPacketSize = config->PacketSizeBytes ;
    }
    llmAddTraceData(tbh,"%d","DataPacketSize={0}, ",adv_config->DataPacketSize);
    llmAddTraceData(tbh,"%d","recvBuffsQsize={0}, ", adv_config->recvBuffsQsize);
    llmAddTraceData(tbh,"%d","nackBuffsQsize={0}, ", adv_config->nackBuffsQsize);
    llmAddTraceData(tbh,"%d","nackQsize={0}, ", adv_config->nackQsize);
    llmAddTraceData(tbh,"%d","recvQsize={0}, ", adv_config->recvQsize);
    llmAddTraceData(tbh,"%d","rsrvQsize={0}, ", adv_config->rsrvQsize);
    llmAddTraceData(tbh,"%d","fragQsize={0}, ", adv_config->fragQsize);
    llmAddTraceData(tbh,"%d","evntQsize={0}, ", adv_config->evntQsize);
    llmAddTraceData(tbh,"%d","packQsize={0}, ", adv_config->packQsize);
    llmAddTraceData(tbh,"%d","BaseWaitMili={0}, ", adv_config->BaseWaitMili);
    llmAddTraceData(tbh,"%d","LongWaitMili={0}, ", adv_config->LongWaitMili);

    llmAddTraceData(tbh,"%d","SnapshotCycleMilli_rx={0}, ", adv_config->SnapshotCycleMilli_rx);
    llmAddTraceData(tbh,"%d","PrintStreamInfo_rx={0}, ",  adv_config->PrintStreamInfo_rx);
    llmAddTraceData(tbh,"%d","MemoryCrisisCycle={0}, ", adv_config->MemoryCrisisCycle);

    llmAddTraceData(tbh,"%d","MemoryAlertPctHigh={0}, ", adv_config->MemoryAlertPctHi);
    llmAddTraceData(tbh,"%d","MemoryAlertPctLow={0}, ",  adv_config->MemoryAlertPctLo);
    llmAddTraceData(tbh,"%d","NackGenerCycle={0}, ", adv_config->NackGenerCycle);


    llmAddTraceData(tbh,"%d","TaskTimerCycle={0}, ", adv_config->TaskTimerCycle);
    llmAddTraceData(tbh,"%d","BindRetryTime={0}, ",  adv_config->BindRetryTime);
    llmAddTraceData(tbh,"%d","RecvPacketNpoll={0}, ", adv_config->RecvPacketNpoll);

    llmAddTraceData(tbh,"%d","ReceiverConnectionEstablishTimeoutMilli={0}, ",adv_config->ReceiverConnectionEstablishTimeoutMilli);
    llmAddTraceData(tbh,"%d","ThreadPriority_tx={0}, ",  adv_config->ThreadPriority_tx);
    llmAddTraceData(tbh,"%d","ThreadPriority_rx={0}, ",  adv_config->ThreadPriority_rx);

    llmAddTraceData(tbh,"%llx","ThreadAffinity_tx={0}, ",UINT_64_TO_LLU(adv_config->ThreadAffinity_tx));
    llmAddTraceData(tbh,"%llx","ThreadAffinity_rx={0}, ",UINT_64_TO_LLU(adv_config->ThreadAffinity_rx));
    llmAddTraceData(tbh,"%d","ThreadStackSize={0}, ",  adv_config->ThreadStackSize);
    llmAddTraceData(tbh,"%d","LatencyMonSampleRate_tx={0}, ",   adv_config->LatencyMonSampleRate);
    llmAddTraceData(tbh,"%d","LatencyMonSampleRate_rx={0}, ",    adv_config->MonSampleRate);
    llmAddTraceData(tbh,"%d","LimitPendingPackets={0}, ",    adv_config->UsePerConnInQ);

    llmCompositeTraceInvoke(tbh);
    return rc;
}

/*****************************************************************************************/

int  rum_EstablishConnection(rumInstanceRec *gInfo, const char *address,  int port,  int method, int heartbeat_timeout_milli,
                            int heartbeat_interval_milli, int one_way_heartbeat, int establish_timeout_milli,int msg_len,
                            const unsigned char *msg, rum_on_connection_event_t on_connection_event,void *user, int use_shm, int *error_code)
{
  int l;
  ConnInfoRec *cInfo ;
  ConnListenerInfo *cl ;

  const char* methodName = "rum_EstablishConnection";
  TCHandle tcHandle = gInfo->tcHandles[0];

  l = (63*1024) ;
  if ( msg_len > l || msg_len < 0 )
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3511), "%d%d",
      "The message length {0} is not valid. The value must be a positive number, and the maximum is {1}.", msg_len, l);
    return RMM_FAILURE ;
  }

  if ( port < 1 || port > 65535 )
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3512), "%d",
      "The port ({0}) is out of range. The value must be between 1 and 65535.", port);
    return RMM_FAILURE ;
  }

  if ( strllen(address,1024) > 1023  )
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3513), "%d",
      "The address length {0} is not valid . The maximum value is {1}.", (int)strllen(address,1024));
    return RMM_FAILURE ;
  }

  if ( heartbeat_timeout_milli < heartbeat_interval_milli && heartbeat_timeout_milli > 0 && heartbeat_interval_milli > 0 )
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3514), "%d%d",
      "The heartbeat_interval_milli parameter value {0} is not valid. The value must be less than the heartbeat_timeout_milli value {1}.",heartbeat_interval_milli,heartbeat_timeout_milli);
    return RMM_FAILURE ;
  }

  if ( heartbeat_timeout_milli < 0 )
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3515), "%d",
      "The heartbeat_timeout_milli parameter value {0} is not valid. The value must be a positive number.",heartbeat_timeout_milli);
    return RMM_FAILURE ;
  }

  if ( heartbeat_interval_milli < 0 )
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3516), "%d",
      "The heartbeat_interval_milli parameter value {0} is not valid. The value must be a positive number.",heartbeat_interval_milli);
    return RMM_FAILURE ;
  }

  if ( establish_timeout_milli < 0 )
  {
    *error_code = RUM_L_E_BAD_PARAMETER ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3517), "%d",
      "The establish_timeout_milli parameter value {0} is not valid. The value must be a positive number.",establish_timeout_milli);
    return RMM_FAILURE ;
  }

  if ( (cInfo=(ConnInfoRec *)malloc(sizeof(ConnInfoRec))) == NULL )
  {
    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
    LOG_MEM_ERR(tcHandle,methodName,(sizeof(ConnInfoRec)));

    return RMM_FAILURE ;
  }
  memset(cInfo,0,sizeof(ConnInfoRec)) ;
  cInfo->gInfo = gInfo ; 

  if ( (cl=(ConnListenerInfo *)malloc(sizeof(ConnListenerInfo))) == NULL )
  {
    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
    LOG_MEM_ERR(tcHandle,methodName,(sizeof(ConnListenerInfo)));

    free(cInfo) ;
    return RMM_FAILURE ;
  }
  memset(cl,0,sizeof(ConnListenerInfo)) ;

  cl->uid      = gInfo->nextCL++ ;
  cl->is_valid = 1 ;
  cl->n_cip    = 1 ;
  cl->on_event = on_connection_event ;
  cl->user     = user ;
  cInfo->conn_listener = cl ;

  cInfo->init_here = 1 ;
  strlcpy(cInfo->req_addr,address,sizeof(cInfo->req_addr)) ;
  cInfo->req_port = port ;
  cInfo->method = method ;
  if ( msg_len )
  {
    cInfo->msg_len = msg_len ;
    if ( (cInfo->msg_buf=(char *)malloc(cInfo->msg_len)) == NULL )
    {
      *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
      LOG_MEM_ERR(tcHandle,methodName,(cInfo->msg_len));

      free(cl) ;
      free(cInfo) ;
      return RMM_FAILURE ;
    }
    memcpy(cInfo->msg_buf,msg,msg_len) ;
  }
  cInfo->cip_to = establish_timeout_milli ;
  cInfo->hb_to  = heartbeat_timeout_milli ;
  cInfo->hb_interval = heartbeat_interval_milli ;
  cInfo->one_way_hb  = one_way_heartbeat ;
  cInfo->use_shm     = use_shm ;

  LL_put_buff(gInfo->connReqQ,cInfo,1) ;

  return RMM_SUCCESS ;
}

/****************************************************/

int rum_CloseConnection(rumInstanceRec *gInfo,rumConnection *rum_connection, int *error_code)
{
  int i=0 ,n ;
  ConnInfoRec *cInfo ;

  TCHandle tcHandle = gInfo->tcHandles[0];

  rmm_rwlock_rdlock(&gInfo->ConnSyncRW) ;
  for ( cInfo=gInfo->firstConnection ; cInfo ; cInfo=cInfo->next )
    if ( cInfo->connection_id == rum_connection->connection_id )
      break ;
  if ( !cInfo )
  {
    char cid_str[20] ;
    rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ;
    b2h(cid_str,(uint8_t *)&rum_connection->connection_id,sizeof(rumConnectionID_t)) ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3521), "%s",
      "The connection {0} cannot be found during RUM connection closing.",cid_str );

    return RMM_SUCCESS ;
  }

  cInfo->is_invalid |= A_INVALID ;
  rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ;
  n = 250 ;
  while ( (cInfo=rum_find_connection(gInfo,rum_connection->connection_id,i,1)) && n-- )
    tsleep(0,10000000) ;

  if ( cInfo )
  {
    char cid_str[20] ;
    b2h(cid_str,(uint8_t *)&rum_connection->connection_id,sizeof(rumConnectionID_t)) ;
    *error_code = RUM_L_E_GENERAL_ERROR ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_WARN, MSG_KEY(4522), "%s",
      "The connection {0} has yet to be removed after 2500 milliseconds.",cid_str );

    return RMM_FAILURE ;
  }
  return RMM_SUCCESS ;
}

/****************************************************/

ConnInfoRec *rum_find_connection(rumInstanceRec *gInfo,rumConnectionID_t cid, int try_ind, int iLock)
{
  ConnInfoRec *cInfo ;

  if ( iLock ) rmm_rwlock_rdlock(&gInfo->ConnSyncRW) ;
  for ( cInfo=gInfo->firstConnection ; cInfo ; cInfo=cInfo->next )
  {
    if ( cInfo->connection_id == cid )
      break ; 
  }
  if ( iLock ) rmm_rwlock_rdunlock(&gInfo->ConnSyncRW) ;
  return cInfo;
}

/****************************************************/

int rum_AddConnectionListener(rumInstanceRec *gInfo,
                              int  (*on_connection_event)(rumConnectionEvent *connection_event, void *user),
                              void *user, int *error_code)
{
  ConnListenerInfo *cl ;

  const char* methodName = "rum_AddConnectionListener";
  TCHandle tcHandle = gInfo->tcHandles[0];


  if ( (cl=(ConnListenerInfo *)malloc(sizeof(ConnListenerInfo))) == NULL )
  {
    *error_code = RUM_L_E_MEMORY_ALLOCATION_ERROR ;
  LOG_MEM_ERR(tcHandle,methodName,(sizeof(ConnListenerInfo)));
    return RMM_FAILURE ;
  }
  memset(cl,0,sizeof(ConnListenerInfo)) ;

  cl->uid      = gInfo->nextCL++ ;
  cl->is_valid = 1 ;
  cl->on_event = on_connection_event ;
  cl->user     = user ;

  pthread_mutex_lock(&gInfo->ConnectionListenersMutex) ;
  if ( gInfo->nConnectionListeners >= RUM_MAX_CONNECTION_LISTENERS )
  {
    *error_code = RUM_L_E_INTERNAL_LIMIT ;
    llmSimpleTraceInvoke(tcHandle, LLM_LOGLEV_ERROR, MSG_KEY(3524), "%d",
      "The maximum number of connection listeners has been reached. The maximum number of connection listeners is {0}.", RUM_MAX_CONNECTION_LISTENERS );

    free(cl) ;
    pthread_mutex_unlock(&gInfo->ConnectionListenersMutex) ;
    return RMM_FAILURE ;
  }
  gInfo->ConnectionListeners[gInfo->nConnectionListeners++] = cl ;
  pthread_mutex_unlock(&gInfo->ConnectionListenersMutex) ;

  return RMM_SUCCESS ;
}

/****************************************************/

int rum_RemoveConnectionListener(rumInstanceRec *gInfo,void *user, int *error_code)
{
  int n , rc=RMM_FAILURE ; 
  ConnListenerInfo *cl ;

  pthread_mutex_lock(&gInfo->ConnectionListenersMutex) ;
  for ( n=0 ; n<gInfo->nConnectionListeners ; n++ )
  {
    if ( (cl=gInfo->ConnectionListeners[n])->user == user )
    {
      gInfo->ConnectionListeners[n] = gInfo->ConnectionListeners[--gInfo->nConnectionListeners] ;
      cl->is_valid = 0 ;
      if ( !(cl->n_active > 0 || cl->n_cip > 0) )
      {
        if ( gInfo->free_callback )
          PutFcbEvent(gInfo, gInfo->free_callback, cl->user) ;
        PutFcbEvent(gInfo, ea_free_ptr, cl) ;
      }
      rc = RMM_SUCCESS ;
      break ; 
    }
  }
  pthread_mutex_unlock(&gInfo->ConnectionListenersMutex) ;

  return rc ; 
}

/****************************************************/

void send_FO_signal(rumInstanceRec *rumInfo, int iLock)
{
  rmmTransmitterRec *tInfo ;

  if ( !rumInfo )
    return ;

  tInfo = (rmmTransmitterRec *)rumInfo->tInfo ;

  if ( !tInfo )
    return ;
  rmm_cond_signal(&tInfo->FO_CondWait,iLock) ;

  return ;

}

void send_PR_signal(rumInstanceRec *rumInfo, int iLock)
{
  rmmReceiverRec *rInfo ;

  if ( !rumInfo )
    return ;

  rInfo = (rmmReceiverRec *)rumInfo->rInfo ;

  if ( !rInfo )
    return ;

  rmm_cond_signal(&rInfo->prWcw,iLock) ;

  return ;

}
