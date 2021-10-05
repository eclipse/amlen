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

#define TRACE_COMP Security

#include <security.h>
#include <dirent.h>
#include <ismrc.h>
#include <ismutil.h>
#include <ldap.h>
#include <pwd.h>
#include <errno.h>
#include <ctype.h>
#include <config.h>
#include <authentication.h>
#include <securityInternal.h>
#include <threadpool.h>
#include <ldaputil.h>
#include <string.h>
#include <ltpautils.h>
#include <transport.h>
#include <curl/curl.h>
#include <throttle.h>
#include <openssl/evp.h>

extern int auditLogControl;
extern int isAuthenticationCacheDisabled;
extern int isCachingGroupInfoDuringAuth;
extern int enabledCache;
extern int enabledGroupCache;
extern int    groupCacheTTL;
extern ismLDAPConfig_t *    _localLdapConfig;

extern int ism_security_ldap_authentication(LDAP ** ld, ismAuthToken_t * authobj) ;
extern int ism_security_authenticateFromCache(ismAuthToken_t * token, uint64_t hash_code);
extern uint64_t ism_security_memhash_fnv1a_32(const void * in, size_t *len);
extern ismAuthToken_t * ism_security_getSecurityContextAuthToken(ismSecurity_t *sContext);
extern void ism_security_cacheAuthToken(ismAuthToken_t *authToken);
extern int ism_security_getLDAPIdPrefix(char * idMap, char *idPrefix);
extern ismLDAPConfig_t *ism_security_getLDAPConfig(void);
extern ismLTPA_t *ism_security_context_getLTPASecretKey(ismSecurity_t *sContext);
extern int ism_security_context_setLTPAExpirationTime(ismSecurity_t *sContext, unsigned long tmval);
extern void ism_security_getMemberGroupsInternal(LDAP * ld, char *memberdn, ismAuthToken_t * token, int level);
void ism_security_reValidatePoliciesInSecContext(ismSecurity_t *secContext);
extern ism_transport_t * ism_security_getTransFromSecContext(ismSecurity_t *sContext);
extern void ism_security_setLDAPGlobalEnv(ismLDAPConfig_t * ldapConfig);
extern int ism_security_context_getCheckGroup(ismSecurity_t *sContext);
extern char *ism_config_getAdminUserName(void);
extern char *ism_config_getAdminUserPassword(void);
extern int ism_security_context_isSuperUser(ismSecurity_t *sContext);
extern int ism_security_context_isAdminListener(ismSecurity_t *sContext);
extern void ism_security_context_setSuperUser(ismSecurity_t *sContext);
extern int ism_security_getSecurityContextAllowNullPassword(ismSecurity_t *sContext);
extern void ism_security_setLDAPSConfig(ismLDAPConfig_t * ldapConfig);


extern ism_trclevel_t * ism_defaultTrace;
extern ism_time_t ldapCfgObj_Changed_Time;
extern ism_threadkey_t curlHandleTLSKey;

security_stat_t *statCount = NULL;
int userIDMapHasStar = 0;
int groupIDMapHasStar = 0;

static int disableAuth=0;

#define LTPA_AUTH_USERNAME 		"IMA_LTPA_AUTH"
#define LTPA_AUTH_USERNAME_LEN  13


pthread_mutex_t    authLock;
pthread_cond_t     authCond;
ism_threadh_t      authThread;
ismAuthEvent_t *    authHead=NULL;
ismAuthEvent_t *    authTail=NULL;

extern ismAuthEvent_t * ism_security_getSecurityContextAuthEvent(ismSecurity_t *sContext);
extern int ism_throttle_startDelayTableCleanUpTimerTask(void);

int32_t ism_security_authenticateInternal(LDAP **ld,ismAuthToken_t * authToken);
static int ism_validate_ltpa_token(ismSecurity_t *sContext, ismAuthToken_t * token);

XAPI security_stat_t * ism_security_getStat(void) {
    return statCount;
}

int ism_security_LDAPInitLD(LDAP ** ld)
{
    int rc = ISMRC_OK;
    pthread_mutex_lock(&authLock);
    rc = ldap_initialize(ld, NULL);
    if (rc != LDAP_SUCCESS) {
        char * errStr = ldap_err2string(rc);
        TRACE(2, "Couldn't create LDAP session: error=%s : rc=%d\n",
                errStr ? errStr : "", rc);
        rc = ISMRC_FailedToBindLDAP;
        ism_common_setErrorData(rc, "%s%d", errStr, rc);
    }
    pthread_mutex_unlock(&authLock);
    return rc;
}

void ism_security_LDAPTermLD(LDAP * ld)
{
    //Unbind
    if(ld!=NULL)
        ldap_unbind_ext_s(ld, NULL, NULL);
   
}
static void ism_security_destroyInternalLDAPObject(void)
{
    pthread_spin_destroy(&_localLdapConfig->lock);
    ism_common_free(ism_memory_admin_misc,_localLdapConfig);
}
static void ism_security_initInternalLDAPObject(void)
{
    _localLdapConfig = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,488),1, sizeof(ismLDAPConfig_t));
    pthread_spin_init(&_localLdapConfig->lock, 0);
    _localLdapConfig->name="ldapconfig";
    _localLdapConfig->URL="ldap://127.0.0.1";
    _localLdapConfig->Certificate=NULL;
    _localLdapConfig->IgnoreCase=true;
    _localLdapConfig->BaseDN="dc=ism.ibm,dc=com";
    _localLdapConfig->BindDN="cn=Directory Manager,dc=ism.ibm,dc=com";
    _localLdapConfig->BindPassword="secret";
    _localLdapConfig->UserSuffix="ou=people,ou=messaging,dc=ism.ibm,dc=com";
    _localLdapConfig->GroupSuffix="ou=groups,ou=messaging,dc=ism.ibm,dc=com";
    _localLdapConfig->GroupCacheTTL=300;
    _localLdapConfig->UserIdMap="*:cn";
    _localLdapConfig->GroupIdMap="*:cn";
    _localLdapConfig->GroupMemberIdMap="member";
    _localLdapConfig->Timeout=10;
    _localLdapConfig->EnableCache=true;
    _localLdapConfig->CacheTTL=10;
    _localLdapConfig->MaxConnections=10;
    _localLdapConfig->Enabled=true;
    _localLdapConfig->deleted=false;
    _localLdapConfig->NestedGroupSearch=true;
    _localLdapConfig->CheckServerCert=ismSEC_SERVER_CERT_DISABLE_VERIFY;
    
    ism_security_getLDAPIdPrefix(_localLdapConfig->UserIdMap, _localLdapConfig->UserIdPrefix);
    ism_security_getLDAPIdPrefix(_localLdapConfig->GroupIdMap, _localLdapConfig->GroupIdPrefix);
    ism_security_getLDAPIdPrefix(_localLdapConfig->GroupMemberIdMap, _localLdapConfig->GroupMemberIdPrefix);

    /* check for * in UserIdMap and GroupIdMap */
    if (_localLdapConfig->UserIdMap) {
        char *uPtr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),_localLdapConfig->UserIdMap);
        char *tmpstr = uPtr;
        char *val;
        userIDMapHasStar = 2;  /* Indicates UserIdMap is not NULL */
        while ( tmpstr != NULL ) {
        	val= ism_common_getToken(tmpstr, ":" , ":", &tmpstr);
        	if ( *val == '*' ) {
        		userIDMapHasStar = 1;
        		break;
        	}
        }
        ism_common_free(ism_memory_admin_misc,uPtr);
    }
    if (_localLdapConfig->GroupIdMap) {
        char *gPtr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),_localLdapConfig->GroupIdMap);
        char *tmpstr = gPtr;
        char *val;
        groupIDMapHasStar = 2;  /* Indicates GroupIdMap is not NULL */
        while ( tmpstr != NULL ) {
        	val= ism_common_getToken(tmpstr, ":" , ":", &tmpstr);
        	if ( *val == '*' ) {
        		groupIDMapHasStar = 1;
        		break;
        	}
        }
        ism_common_free(ism_memory_admin_misc,gPtr);
    }
    
    /*Calculate the length of suffix + idMap for User and Group*/
    if(_localLdapConfig->UserSuffix) _localLdapConfig->UserDNMaxLen = strlen(_localLdapConfig->UserSuffix) +strlen(_localLdapConfig->UserIdMap)  ;
    if(_localLdapConfig->GroupSuffix) _localLdapConfig->GroupDNMaxLen = strlen(_localLdapConfig->GroupSuffix) +strlen(_localLdapConfig->GroupIdMap)  ;
}

/* Authenticate AdminUser */
static int ism_security_authenticateAdminUser(char *username, char *password) {
    int rc = ISMRC_NotAuthenticated;

    /* check if username is valid AdminUserName */
    char *adminUserName = ism_config_getAdminUserName();
    char *adminUserPasswd = ism_config_getAdminUserPassword();

    if ( username && adminUserName && strcmp(username, adminUserName) == 0 ) {
        if ( adminUserPasswd && password && strcmp(adminUserPasswd, password) == 0 ) {
            rc = ISMRC_OK;
        }
    }
    if ( adminUserPasswd ) ism_common_free(ism_memory_admin_misc,adminUserPasswd);
    return rc;
}

/* Terminate authentication service */
int ism_security_termAuthentication(void)
{
    int rc = ISMRC_OK;

    ism_security_termWorkers();

    ism_security_ldapUtilDestroy();
    
    ism_security_destroyInternalLDAPObject();
    
    return rc;
}

/* Init authentication service */
int ism_security_initAuthentication(ism_prop_t *props)
{

	pthread_mutex_init(&authLock, 0);

    ism_security_initInternalLDAPObject();
    
    ismLDAPConfig_t * ldapConfig = ism_security_getLDAPConfig();
    
    ism_security_setLDAPGlobalEnv(ldapConfig);
    
    disableAuth = ism_common_getBooleanConfig("DisableAuthentication", false);
    
    ism_common_createThreadKey(&curlHandleTLSKey);

    /*Getting Cache Configuration*/
    isAuthenticationCacheDisabled = ism_common_getIntConfig("SecurityAuthCacheDisabled",0);
    isCachingGroupInfoDuringAuth  = ism_common_getIntConfig("SecurityCacheGroupInfoDuringAuth",0);
    enabledGroupCache  = ism_common_getBooleanConfig("EnabledGroupCache",1);
     

    int threadpoolsize = ism_common_getIntConfig("SecurityThreadPoolSize",2);

    //LTPA thread pool is a subpool of security threads so will be reduced to less than
    //threadpoolsize (by default we turn off dedicated threads for LTPA and they use
    //the general ones
    int lptathreadpoolsize = ism_common_getIntConfig("SecurityLTPAThreadPoolSize",0);
    ism_security_initThreadPool(threadpoolsize, lptathreadpoolsize);
    ism_security_startWorkers();


    ism_security_ldapUtilInit();
    
    /* init stat struct */
    statCount = (security_stat_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,492),1, sizeof(security_stat_t));

    /*Initialize throttling feature*/
    /*Initialize the Delay Process*/
   ism_throttle_initThrottle();

   /*Parse throttle configuration*/
   ism_throttle_parseThrottleConfiguration();

   /*Start Task to clean up the client table.*/
   if(ism_throttle_isEnabled())
	ism_throttle_startDelayTableCleanUpTimerTask();

   //TODO: Handle init error
    return 0;
}

XAPI int32_t ism_security_authenticateInternal(LDAP ** ld, ismAuthToken_t * authToken) {
    int rc = ISMRC_NotAuthenticated;
    
    /* Sanity check for username and password */
    if (!authToken || !authToken->username || !authToken->password ) {
        return rc;
    }
    /*Revalidate the Rule so we can set checkGroup (for getting groups from LDAP)*/
    ism_security_reValidatePoliciesInSecContext(authToken->sContext);

    rc = ism_security_ldap_authentication(ld, authToken);
    
    return rc;
}



XAPI int32_t ism_security_authenticate_user(ismSecurity_t *sContext, const char *username, int username_len, const char *password, int password_len)
{
    ismAuthToken_t * token;
    int rc = ISMRC_NotAuthenticated;
    int ldInitRC=ISMRC_OK;
    size_t passLen = password_len;
      
    /* Sanity check for username and password */
    if ( !username || !password ) {
        __sync_add_and_fetch(&statCount->authen_failed, 1);
        return rc;
    }
   
    

    uint64_t hash_code = ism_security_memhash_fnv1a_32(password, &passLen);
    
    token= ism_security_getSecurityContextAuthToken(sContext);
    ism_transport_t *tport = ism_security_getTransFromSecContext(sContext);
    ism_trclevel_t *trclevel = ism_defaultTrace;
    if ( tport && tport->trclevel ) trclevel = tport->trclevel;
    
    if(username_len > token->username_alloc_len){
        if(token->username_inheap)
            ism_common_free(ism_memory_admin_misc,token->username);
        token->username = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,494),username_len);
        token->username_len=username_len;
        token->username_alloc_len=username_len;
        token->username_inheap=1;
    }else{
        token->username_len=username_len;
    }
    memcpy(token->username, username, username_len );
    
    if(password_len > token->password_alloc_len){
        if(token->password_inheap)
            ism_common_free(ism_memory_admin_misc,token->password);
        token->password = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,496),password_len);
        token->password_len=password_len;
        token->password_alloc_len=password_len;
        token->password_inheap=1;
    }else{
        token->password_len=password_len;
    }
    memcpy(token->password, password, password_len);
       
       token->hash_code = hash_code;
       
       
    if ( !strcasecmp(username, "IMA_LTPA_AUTH")) {
        rc = ism_validate_ltpa_token(sContext, token);
        if ( rc != ISMRC_OK ) {
        	__sync_add_and_fetch(&statCount->authen_failed, 1);
        } else {
        	__sync_add_and_fetch(&statCount->authen_passed, 1);
        }
        return rc;
    }
    
     if ( !strcasecmp(username, "IMA_OAUTH_ACCESS_TOKEN")) {
        rc = ism_validate_oauth_token(sContext, token);
         if ( rc != ISMRC_OK ) {
        	__sync_add_and_fetch(&statCount->authen_failed, 1);
        } else {
        	__sync_add_and_fetch(&statCount->authen_passed, 1);
        }
        return rc;
    }
    
    /*Check if authentication is valid in cache.*/
    if(enabledCache){
        TRACEL(8, trclevel, "Authenticating User from Cache: username=%s\n", token->username);
        if((rc = ism_security_authenticateFromCache(token, hash_code))==ISMRC_OK)
        {
            TRACEL(8, trclevel, "Cache Authentication Status : username=%s. Status: %d\n", token->username, rc);
            if ( rc != ISMRC_OK ) {
            	__sync_add_and_fetch(&statCount->authen_failed, 1);
            } else {
            	__sync_add_and_fetch(&statCount->authen_passed, 1);
            }
            return rc;
        }
    }
    
    LDAP *ld=NULL;
    ldInitRC = ism_security_LDAPInitLD(&ld);
    if(ldInitRC!=ISMRC_OK) return rc;
    ism_security_setLDAPSConfig(NULL);
    rc = ism_security_authenticateInternal(&ld, token);
    if(rc==ISMRC_OK && enabledCache){
        //Cache the Auth
        TRACEL(8, trclevel, "Caching token: username=%s\n", token->username);
        ism_security_cacheAuthToken(token);
    }
    ism_security_LDAPTermLD(ld);

    if ( rc == ISMRC_OK )
        __sync_add_and_fetch(&statCount->authen_passed, 1);
    else
        __sync_add_and_fetch(&statCount->authen_failed, 1);

    return rc;
}







static void ism_security_processAuthEvent(LDAP ** ld, ismAuthEvent_t * authent)
{
    int rc=ISMRC_NotAuthenticated;
    int needCaching = 0;
    if(authent->token==NULL){
    	return;
    }
    
    pthread_spin_lock(&authent->token->lock);
    authent->token->status = AUTH_STATUS_IN_PROGRESS;
    if(authent->token->isCancelled!=1){
        pthread_spin_unlock(&authent->token->lock);
    
        if ( authent->authnRequired == 1 ) {
            //if ( !strcasecmp(authent->token->username, "IMA_LTPA_AUTH")) {
            if ( authent->ltpaAuth==1) {
                
                rc = ism_validate_ltpa_token(authent->token->sContext, authent->token);
               
                needCaching = 0;
               
                /*Pull the Group Membership */
                if(rc==ISMRC_OK){
                    ism_security_getMemberGroupsInternal(*ld, authent->token->userDNPtr, authent->token, 0);
                    pthread_spin_lock(&authent->token->lock);        
                    authent->token->gCacheExpireTime = ism_common_currentTimeNanos() + (groupCacheTTL * SECOND_TO_NANOS);
                    pthread_spin_unlock(&authent->token->lock);
                }
                
            }  else if ( authent->oauth==1) {        /* username is IMA_OAUTH_ACCESS_TOKEN */

                rc = ism_validate_oauth_token(authent->token->sContext, authent->token);

                needCaching = 0;

                /*Pull the Group Membership */
                if(rc==ISMRC_OK){
                     ism_security_getMemberGroupsInternal(*ld, authent->token->userDNPtr, authent->token, 0);
                     pthread_spin_lock(&authent->token->lock);
                     authent->token->gCacheExpireTime = ism_common_currentTimeNanos() + (groupCacheTTL * SECOND_TO_NANOS);
                     pthread_spin_unlock(&authent->token->lock);
                }

            } else if ( ism_security_context_isSuperUser(authent->token->sContext) == 1 ) {
                /* Authenticate AdminUser */
                rc = ism_security_authenticateAdminUser(authent->token->username, authent->token->password);
            } else {
              if(enabledCache ){

                if ((rc=ism_security_authenticateFromCache(authent->token, authent->token->hash_code))!=ISMRC_OK)
                {
                    rc =ism_security_authenticateInternal(ld, authent->token);
                    needCaching=1;
                }
              } else {

                rc =ism_security_authenticateInternal(ld, authent->token);
                needCaching = 1;
              }
            }
        } else {
            rc = ISMRC_OK;
            __sync_add_and_fetch(&statCount->authen_passed, 1);
        }

        if ( rc != ISMRC_OK && rc!=ISMRC_Closed) {
            rc = ISMRC_NotAuthenticated;
            __sync_add_and_fetch(&statCount->authen_failed, 1);
        }
        
        if(authent->token!=NULL){
	        pthread_spin_lock(&authent->token->lock);
	        if(authent->token->isCancelled!=1){
	            authent->token->status = AUTH_STATUS_IN_CALLBACK;
	            pthread_spin_unlock(&authent->token->lock);
	
	            if ( rc == ISMRC_OK ) {
	                rc = ism_security_validate_policy(authent->token->sContext,
	                    ismSEC_AUTH_USER, NULL, ismSEC_AUTH_ACTION_CONNECT, ISM_CONFIG_COMP_SECURITY, NULL);
	                if ( rc != ISMRC_OK )
	                    rc = ISMRC_NotAuthorized;
	            }
	
	            if(authent->token!=NULL && authent->token->pCallbackFn!=NULL)
	                authent->token->pCallbackFn(rc, authent->token->pContext);
	            
	            if(authent->token!=NULL){
		            pthread_spin_lock(&authent->token->lock);
		            authent->token->status = AUTH_STATUS_COMPLETED;
		            if ( needCaching==1 && rc==ISMRC_OK && enabledCache ) {
		                //Cache the Auth
		                ism_security_cacheAuthToken(authent->token);
		            }

		            /*Authentication is done. Set isCancelled 2 so the Sec context can be destroyed if the isCancelled is 1.*/
		            if(authent->token->isCancelled==1 || rc == ISMRC_Closed){
		            	 authent->token->isCancelled=2;
		            }
	            }
	        }else{
	            authent->token->isCancelled=2;
	            authent->token->status = AUTH_STATUS_CANCELLED;
	        }
        }
    }else{
        authent->token->status = AUTH_STATUS_CANCELLED;
        authent->token->isCancelled=2;
    }

    pthread_spin_unlock(&authent->token->lock);		/* BEAM suppression: operating on NULL */
    if(authent->token!=NULL){
    	ism_security_destroy_context(authent->token->sContext);
    }

}


/**
 * Authentication processing method
 */
void * ism_security_ldapthreadfpool(void * param, void * context, int value) {
    ism_worker_t * worker = (ism_worker_t *) param;
    ismAuthEvent_t * authent;
    int ldIniRC=ISMRC_OK;

    TRACE(8, "Authentication Thread (id=%d) is started.\n", worker->id);
    ism_time_t lastLDAPCfgObjChangedTime= ldapCfgObj_Changed_Time;
    LDAP *ld=NULL;
    
    ldIniRC = ism_security_LDAPInitLD(&ld);
    if(ldIniRC!=ISMRC_OK){
        TRACE(8, "Failed to initialize LDAP. The worker thread %d is not started.\n", worker->id);
        pthread_mutex_lock(&worker->authLock);
        worker->status = WORKER_STATUS_SHUTDOWN;
        pthread_mutex_unlock(&worker->authLock);
        return NULL;
    }

    ism_security_setLDAPSConfig(NULL);

    pthread_mutex_lock(&worker->authLock);
    worker->status = WORKER_STATUS_ACTIVE;
    pthread_mutex_unlock(&worker->authLock);
        
    for (;;) {
        ism_common_backHome();
        pthread_mutex_lock(&worker->authLock);

        /* Wait for work */
        while (worker->authHead == NULL && worker->status==WORKER_STATUS_ACTIVE) {      /* BEAM suppression: infinite loop */
            pthread_cond_wait(&worker->authCond, &worker->authLock);
        }
        
        /*Check if the worker still active*/
        if(worker->status!=WORKER_STATUS_ACTIVE){
            pthread_mutex_unlock(&worker->authLock);
            break;
        }
    
        /* Select the first item */
        authent = worker->authHead;
        SA_ASSUME(authent != NULL);
        worker->authHead = authent->next;
        if (worker->authHead == NULL)
            worker->authTail = NULL;
        pthread_mutex_unlock(&worker->authLock);
        ism_common_going2work();
        if(authent->type==SECURITY_LDAP_AUTH_EVENT){
            /*Ensure that the LD object is valid*/
        	if(lastLDAPCfgObjChangedTime!=ldapCfgObj_Changed_Time){
        		ism_security_LDAPTermLD(ld);
        		ldIniRC = ism_security_LDAPInitLD(&ld);
        		if ( ldIniRC == ISMRC_OK ) {
        	        ism_security_setLDAPSConfig(NULL);
        		}
        		lastLDAPCfgObjChangedTime=ldapCfgObj_Changed_Time;
        	}
            ism_security_processAuthEvent(&ld, authent);
        }

    }
    
    pthread_mutex_lock(&worker->authLock);
    TRACE(8, "Authentication Thread (id=%d) is shutdown.\n", worker->id);
    worker->status = WORKER_STATUS_SHUTDOWN;
    pthread_mutex_unlock(&worker->authLock);
    
    ism_security_LDAPTermLD(ld);
    
    return NULL;
}




void ism_security_returnAuthHandle(void * handle)
{    
    TRACE(8, "Returning Authentication Handle\n");
    if(handle!=NULL){
        ismSecurity_t *sContext = (ismSecurity_t *)handle;
        ismAuthToken_t * token = ism_security_getSecurityContextAuthToken(sContext);
        if(token->inited){ 
            pthread_spin_lock(&token->lock);
            if(token->status==AUTH_STATUS_IN_Q ||
                token->status==AUTH_STATUS_IN_PROGRESS){
                TRACE(8, "Canceling the Authentication process\n");
                /*Submit request to cancel*/
                token->isCancelled=1;
                
                
            }else if(token->status==AUTH_STATUS_IN_CALLBACK){
                /*If Auth is in the application callback, let that complete first.*/
                TRACE(8, "The Authentication is processing the application callback. Waiting to finish\n");
                /*
                while(token->status!=AUTH_STATUS_CANCELLED && 
                        token->status!=AUTH_STATUS_COMPLETED){
                    pthread_spin_unlock(&token->lock);
                    ism_common_sleep(1000);
                    pthread_spin_lock(&token->lock);
                }
                */
                token->isCancelled=1;
                TRACE(8, "The Authentication is finished with the callback\n");
            }else{
            	/*If the token is not in use. Set it to zero so the Security Context can be destroyed. */
            	token->isCancelled=0;

            }
            
            
            pthread_spin_unlock(&token->lock);
        }
        TRACE(8, "Free the Authentication handle\n");
        
    
        
    }
        
}

void ism_security_cancelAuthentication(void * handle)
{    
    
    if(handle!=NULL){
        ismAuthToken_t * token = (ismAuthToken_t *)    handle;
        TRACE(8,"Cancelling the authentication request.\n");
        pthread_spin_lock(&token->lock);
        token->isCancelled=1;
        pthread_spin_unlock(&token->lock);
    }
        
}


static int submitAuthEvent(ismAuthEvent_t * authent)
{
	int rc = ISMRC_NotAuthenticated;
	int submitrc = ism_security_submitLDAPEvent(authent);
	if(submitrc){
		/*If Submit failed.*/
		authent->token->pCallbackFn(rc, authent->token->pContext);
		/**
		 * Decrement the useCount of Context.
		 * */
		if(authent->token->sContext!=NULL){
			authent->token->isCancelled=0;
			ism_security_destroy_context(authent->token->sContext);
		}
	}
	return submitrc;
}


static int delayAuth(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
	ismAuthEvent_t * authent = (ismAuthEvent_t *)userdata;
	TRACE(7, "delayAuth invoked: time=%llu\n", (ULL) timestamp);
	submitAuthEvent(authent);
	ism_common_cancelTimer(key);
	return 0;
}

void ism_security_authenticate_user_async(ismSecurity_t *sContext,
        const char *username, int username_len, 
        const char *password, int password_len, 
        void *pContext, int pContext_size,
        ismSecurity_AuthenticationCallback_t pCallbackFn, int authnRequired, ism_time_t delay)
{
    ismAuthEvent_t * authent;
   
    if (disableAuth) {
        pCallbackFn(ISMRC_OK, pContext);
        return ;
    }

    if(sContext==NULL){
    	pCallbackFn(ISMRC_NotAuthenticated, pContext);
    	return;
    }


	authent= ism_security_getSecurityContextAuthEvent(sContext);
	authent->next = 0;
	authent->oauth = 0;

	authent->token= ism_security_getSecurityContextAuthToken(sContext);
	if(authent->token!=NULL && authent->token->inited){
		pthread_spin_lock(&authent->token->lock);
		authent->token->status = AUTH_STATUS_IN_Q;
		pthread_spin_unlock(&authent->token->lock);
	}
    authent->type = SECURITY_LDAP_AUTH_EVENT;
    authent->authnRequired = authnRequired;

    /* check if this is an admin user - only when connection is on admin endpoint */
    if ( ism_security_context_isAdminListener(sContext) == 1 ) {
        char *admin = ism_config_getAdminUserName();
        if ( username && admin && strcmp(username, admin ) == 0 ) {
            ism_security_context_setSuperUser(sContext);
        }
    }

    /*Increase useCount of Security Context.*/
    ism_security_context_use_increment(sContext) ;
    
    const char * tusername = username;
    int tusername_len = username_len;
    const char * tpassword = password;
    int tpassword_len = password_len;
    if(authent->ltpaTokenSet==1){
    	tusername= authent->token->username;						/* BEAM suppression: operating on NULL */
    	tusername_len = authent->token->username_len;
    	tpassword = authent->token->password;
    	tpassword_len = authent->token->password_len;
    	
    }

    int allowNullPassword = ism_security_getSecurityContextAllowNullPassword(sContext);
    if((authnRequired == 1) && (!tusername || (allowNullPassword == 0 && !tpassword)))
    {
        pCallbackFn(ISMRC_NotAuthenticated, pContext);
        pthread_spin_lock(&authent->token->lock);
        authent->token->status = AUTH_STATUS_COMPLETED;
        pthread_spin_unlock(&authent->token->lock);
        ism_security_destroy_context(sContext);
        return ;
    }
    
    if (tusername!=NULL && tusername_len==13 && memcmp(tusername, "IMA_LTPA_AUTH", 13)==0) {
        authent->ltpaAuth=1;
        TRACE(9, "Set LTPA token: %s\n", authent->token->password);
    } else if (tusername != NULL && tusername_len == 22 && memcmp(tusername, "IMA_OAUTH_ACCESS_TOKEN", 22) == 0) {
        authent->oauth=1;
        TRACE(9, "Set OAuth token: %s\n", authent->token->password);
    }

    /* Bypass authentication step, if authentication is required, but
     * - password is NULL
     * - AllowNullPassword is set
     * - LTPA or OAuth authentication is not enabled
     */
    if ( authnRequired == 1 && allowNullPassword == 1 && !tpassword && authent->ltpaAuth == 0 && authent->oauth == 0) {
        pCallbackFn(ISMRC_OK, pContext);
        return ;
    }

    if ( authnRequired == 1 ) {
    	/*If the token is already set. No need to copy again*/
    	if(authent->ltpaTokenSet!=1){
	        if(tusername_len > authent->token->username_alloc_len){			/* BEAM suppression: operating on NULL */
	            if(authent->token->username_inheap)
	                ism_common_free(ism_memory_admin_misc,authent->token->username);
	            authent->token->username = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,498),1, tusername_len+1);
	            authent->token->username_len=tusername_len;
	            authent->token->username_alloc_len=tusername_len;
	            authent->token->username_inheap=1;
	        }else{
	            authent->token->username_len=tusername_len;
	        }
	        memcpy(authent->token->username, tusername, tusername_len );
	
	        if(tpassword_len > authent->token->password_alloc_len){
	            if(authent->token->password_inheap)
	                ism_common_free(ism_memory_admin_misc,authent->token->password);
	            authent->token->password = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,500),1, tpassword_len+1);
	            authent->token->password_len=tpassword_len;
	            authent->token->password_alloc_len=tpassword_len;
	            authent->token->password_inheap=1;
	        }else{
	            authent->token->password_len=tpassword_len;
	        }
	        memcpy(authent->token->password, tpassword, tpassword_len);
		}
		
        size_t passLen = tpassword_len;
        uint64_t hash_code = ism_security_memhash_fnv1a_32(tpassword, &passLen);
        authent->token->hash_code = hash_code;
    }

    if(pContext_size>0 && pContext!=NULL){
    	if(pContext_size > sContext->authExtras_len){
        	authent->token->pContext = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,501),pContext_size);				/* BEAM suppression: operating on NULL */
        	authent->token->pContext_inheap = 1;
        }
        memcpy(authent->token->pContext, pContext,pContext_size );			/* BEAM suppression: operating on NULL */
    }
    authent->token->pCallbackFn=pCallbackFn;			/* BEAM suppression: operating on NULL */
    

    
    /* By pass authentication if not required */
	if ( authnRequired == 0 &&  authent->ltpaTokenSet == 0 && (!username || *username == '\0')) {
		/* Check connection authorization */
        int rc = ism_security_validate_policy(authent->token->sContext,
            ismSEC_AUTH_USER, NULL, ismSEC_AUTH_ACTION_CONNECT, ISM_CONFIG_COMP_SECURITY, NULL);
        if ( rc != ISMRC_OK )
            rc = ISMRC_NotAuthorized;

        pCallbackFn(rc, pContext);

		/**
		 * Decrement the useCount of Context.
		 * */
        if ( sContext != NULL ) {
	        pthread_spin_lock(&authent->token->lock);
            authent->token->isCancelled=0;
	        authent->token->status = AUTH_STATUS_COMPLETED;
	        pthread_spin_unlock(&authent->token->lock);
			ism_security_destroy_context(sContext);
    	}

	    return;
	}


    if(delay>0){
    	/*Put the request in timer thread.*/
    	ism_common_setTimerOnce(ISM_TIMER_HIGH, delayAuth, authent, delay);

    }else{
    	submitAuthEvent(authent);
    }
    
    return;
}

/*
 * --------------- LTPA Support functions -------------------
 */

typedef char * (*transportAllocBytes_f)(ism_transport_t * transport, int len, int align);
static transportAllocBytes_f transportAllocBytes = NULL;

static int ism_validate_ltpa_token(ismSecurity_t *sContext, ismAuthToken_t * token) {

  /* Check if LTPA is configured */
  if ( !sContext ) {
      TRACE(5, "Security context is not set.\n");
      return ISMRC_NotAuthenticated;
  }

  int rc = ISMRC_OK;
  uint32_t connID = 0;
  char *clientID = NULL;
  char *clientAddr = NULL;
  char *endpoint = NULL;
  ism_trclevel_t *trclevel = ism_defaultTrace;

  ism_transport_t * transport = ism_security_getTransFromSecContext(sContext);
  if ( transport ) {
	  /*Check if transport is in closing or closed state. If yes. set error code and return.*/
	  if(UNLIKELY(transport->state != ISM_TRANST_Open)){
		  rc = ISMRC_Closed;
		  goto MOD_EXIT;
	  }

      if ( transport->trclevel ) trclevel = transport->trclevel;
      connID = transport->index;
      clientID = (char *)transport->clientID;
      clientAddr = (char *)transport->client_addr;
      if ( transport->listener )
          endpoint = (char *)transport->listener->name;
  }


  const char *password = token->password;
  int password_len = token->password_len;

  ismLTPA_t *keys = ism_security_context_getLTPASecretKey(sContext);

  if ( !keys ) {
      TRACEL(5, trclevel, "LTPA profile is not configured.\n");
      rc = ISMRC_NotFound;
      goto MOD_EXIT;
  }

  /* Guess LTPA version based on the token size */
  int tryVersion[2];
  tryVersion[0] = 2;
  tryVersion[1] = 1;
  if ( password_len < 512 ) {
      tryVersion[0] = 1;
      tryVersion[1] = 2;
  }

  char *user = NULL;
  time_t expiration;
  int i;
  for (i=0; i<2; i++) {
      int version = tryVersion[i];
      char *buf = (char *)password;

      char *decode = NULL;
      char *realm = NULL;

      user = NULL;
      expiration = 0;
      rc = ISMRC_OK;

      TRACEL(9, trclevel, "LTPA Token: %s\n", password);
      if ( version == 2 ) {
          rc = ism_security_ltpaV2DecodeToken(keys, buf, password_len, &user, &realm, &expiration);
      } else {
          rc = ism_security_ltpaV1DecodeToken(keys, buf, password_len, &user, &realm, &expiration);
      }
      if ( rc != ISMRC_OK ) {
          TRACEL(7, trclevel, "LTPA Token (ver:%d) Decode failure: %d\n", version, rc);
      } else {
          TRACEL(9, trclevel, "LTPA Token (ver:%d) decode success: User='%s', realm='%s', expiration='%ld'\n", version, user, realm, (long) expiration);
      }

      if (realm) {
          ism_common_free(ism_memory_admin_misc,realm);
      }
      if (decode) {
          ism_common_free(ism_memory_admin_misc,decode);
      }

      if ( rc != ISMRC_OK ) {
          if ( rc == ISMRC_LTPASigVerifyError ) {
              break;
          }
          if (user) {
              ism_common_free(ism_memory_admin_misc,user);
          }
          user = NULL;
      } else {
          break;
      }
  }

  if ( rc != ISMRC_OK ) {
      goto MOD_EXIT;
  }

  /* Copy user data into transport object */
  if ( rc == ISMRC_OK && user != NULL ) {
      /* Validate expireTime is in seconds from EPOC */
      if ( expiration < time(NULL) ) {
          rc = ISMRC_LTPATokenExpired;
          goto MOD_EXIT;
      }

      /* Set Expiration time in Security Context
       * Since expTime is in seconds, need to convert to nanoseconds for Transport ExpireTime
       * which is in nanoseconds
       */
      ism_security_context_setLTPAExpirationTime(sContext, expiration * 1000000000.00);

      ismLDAPConfig_t * ldapConfig = ism_security_getLDAPConfig();


      char * userIdCfg = alloca(strlen(ldapConfig->UserIdPrefix)+2); //+2 for = and null terminated char
      sprintf(userIdCfg, "%s=",ldapConfig->UserIdPrefix);

      char *uid = strstr(user, userIdCfg);

      /*Copy user uid into the the IMA AuthToken*/
      if ( uid != NULL ) {
          size_t uid_len= strlen(uid);

          if ( uid_len<DN_DEFAULT_LEN ) {
              token->userDNPtr = token->userDN;
              token->userDN_inheap = 0;
          } else {
              token->userDNPtr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,505),uid_len+1);
              if (token->userDNPtr==NULL) {
                  rc = ISMRC_AllocateError;
                  goto MOD_EXIT;
              }
              token->userDN_inheap = 1;
          }
          strcpy(token->userDNPtr, uid);

          /* Need to extract just the common name or the user id and set it to the
           * transport->userid for later use for authorization
           */
          char * userid, *more=NULL;
          userid= ism_common_getToken(uid, " \t\r\n", "=\r\n", &more );

          if ( more != NULL )
              userid = ism_common_getToken(more, " \t\r\n", ",\r\n", &more );

          size_t userid_len = strlen(userid);

          ism_transport_t *tport = ism_security_getTransFromSecContext(sContext);

          /* Use the pre-allocated memory in transport if the len of the userid is less than the current one */
          transportAllocBytes = (transportAllocBytes_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_allocBytes_fnptr", 0L);
          tport->userid = transportAllocBytes(tport, userid_len+1, 0);
          memcpy((char *) tport->userid, userid, userid_len );
          ((char *) tport->userid)[userid_len] = 0;
      } else {
          /* Make authentication fail if GroupID filter in Messaging/Connection policy is set for this connection */
          if ( ism_security_context_getCheckGroup(sContext) == 1 ) {
              rc = ISMRC_NotAuthenticated;
              goto MOD_EXIT;
          }
      }

      if ( auditLogControl > 1 )
      {
              char userInfo[2048];
              if ( uid ) {
                  sprintf(userInfo, "IMA_LTPA_AUTH:%s", uid);
              } else {
                      sprintf(userInfo, "IMA_LTPA_AUTH");
              }
              LOG(INFO, Security, 6108, "%u%-s%-s%-s%-s", "User authentication succeeded: ConnectionID={0}, UserID={1}, Endpoint={2}, ClientID={3}, ClientAddress={4}.",
                      connID, userInfo,  endpoint?endpoint:" ", clientID?clientID:" ", clientAddr?clientAddr:" ");
      }
  }

MOD_EXIT:

  if ( user ) ism_common_free(ism_memory_admin_misc,user);
  if ( rc != ISMRC_OK )
  {
      if ( auditLogControl > 0 )
      {
          char errBuffer[256];
          /*This is for Logging. Use System Locale*/
          ism_common_getErrorStringByLocale(rc, ism_common_getLocale(), errBuffer, 255);
          LOG(NOTICE, Security, 6107, "%u%-s%-s%-s%-s%-s%d", "User authentication failed: ConnectionID={0}, UserID={1}, Endpoint={2}, ClientID={3}, ClientAddress={4}, Error={5}, RC={6}.",
              connID, "IMA_LTPA_AUTH", endpoint?endpoint:" ", clientID?clientID:" ", clientAddr?clientAddr:" ", errBuffer, rc);
      }
      if(rc != ISMRC_Closed)
       rc = ISMRC_NotAuthenticated;
  }

  return rc;
}


/*
 * Encryt AdminUserPassword
 */
#define BUFLEN 1024

static int ism_config_hex2int(const char h){
    if(isdigit(h))
        return h - '0';
    else
        return toupper(h) - 'A' + 10;
}

/* DO not change key in this function. Changing will cause backward compatibility issue */
XAPI char * ism_security_encryptAdminUserPasswd(const char * src) {
    unsigned char *key = (unsigned char *)"pDm99d30ccF3W8+8ak5CN4jrnCSBh+ML";
    int inLen = strlen(src) + 1;
    unsigned char out[2 * inLen];
    int outLen = 0;
    int len = 0;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if ( ctx == NULL ) {
        TRACE(2, "EVP_CIPHER_CTX_new() failed: %s\n", ERR_reason_error_string(ERR_get_error()));
        return NULL;
    }

    /* Encrypt */
    unsigned char *inpSrc = (unsigned char *)alloca(inLen* sizeof(char));
    memcpy(inpSrc, src, inLen);
    memset(out, 0, 2 * inLen);

    if ( EVP_EncryptInit_ex(ctx, EVP_des_ede3_ecb(), NULL, key, NULL) != 1 ) {
       TRACE(2, "EVP_EncryptInit_ex() Failed: %s\n", ERR_reason_error_string(ERR_get_error()));
       EVP_CIPHER_CTX_free(ctx);
       return NULL;
    }

    if ( EVP_EncryptUpdate(ctx, out, &outLen, inpSrc, inLen) != 1 ) {
        TRACE(2, "EVP_EncryptUpdate() failed: %s\n", ERR_reason_error_string(ERR_get_error()));
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    len = outLen;

   if ( EVP_EncryptFinal_ex(ctx, out + outLen, &outLen) != 1 ) {
       TRACE(2, "EVP_EncryptFinal_ex() failed: %s\n", ERR_reason_error_string(ERR_get_error()));
       EVP_CIPHER_CTX_free(ctx);
       return NULL;
    }

    len += outLen;

    int i = 0;
    int slen = 0;
    char hkey[BUFLEN];
    memset(hkey, 0, BUFLEN);
    unsigned char *tl = out;
    for (i=0;i<len;i++) {
        snprintf(hkey+slen, BUFLEN - slen,  "%02x", tl[i]);
        slen += 2;
    }
    hkey[slen] = 0;

    EVP_CIPHER_CTX_free(ctx);

    return (ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),hkey));
}

/*
* Decrypt AdminUserPassword
* DO not change key in this function. Changing will cause backward compatibility issue.
*/

XAPI char * ism_security_decryptAdminUserPasswd(const char * src) {
    unsigned char *key = (unsigned char *)"pDm99d30ccF3W8+8ak5CN4jrnCSBh+ML";
    unsigned char *out;
    int ol = 0;
    int declen = 0;

    if ( !src ) return NULL;

    char hkey[BUFLEN];
    memset(hkey, 0, BUFLEN);
    int inLen = snprintf(hkey, BUFLEN, "%s", src);
    int outLen = inLen / 2;
    unsigned char *keyint = (unsigned char* )alloca(outLen);
    int i;

    for (i = 0; i < outLen; i++) {
        keyint[i] = ism_config_hex2int(hkey[i * 2])*16 + ism_config_hex2int(hkey[i * 2 + 1]);
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if ( ctx == NULL ) {
        TRACE(2, "EVP_CIPHER_CTX_new() failed: %s\n", ERR_reason_error_string(ERR_get_error()));
        return NULL;
    }

    out = (unsigned char *)alloca(outLen * sizeof(char));
    memset(out, 0, outLen);
    if (EVP_DecryptInit_ex(ctx, EVP_des_ede3_ecb(), NULL, key, NULL) != 1) {
        TRACE(2, "EVP_DecryptInit_ex() failed: %s\n", ERR_reason_error_string(ERR_get_error()));
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    if ( EVP_DecryptUpdate(ctx, out, &ol, keyint, outLen) != 1 ) {
        TRACE(2, "EVP_DecryptUpdate() failed: %s\n", ERR_reason_error_string(ERR_get_error()));
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    declen = ol;

    if (1 != EVP_DecryptFinal_ex(ctx, out + ol, &ol)) {
        TRACE(2, "EVP_DecryptFinal_ex() failed: %s\n", ERR_reason_error_string(ERR_get_error()));
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    declen += ol;

    char *retval = (char* )ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,508),declen+1);
    memcpy(retval, out, declen);
    retval[declen] = 0;

    EVP_CIPHER_CTX_free(ctx);

    return retval;
}
