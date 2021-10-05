/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

#ifndef TRACE_COMP
#define TRACE_COMP Security
#endif

#include <auth.h>
#include <ismutil.h>
#include <ismregex.h>
#include <javaconfig.h>
#include <pxtransport.h>
#include <tenant.h>
#include <throttle.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#define MQTT_CONST_ONLY
#include <pxmqtt.h>

#define BILLION        1000000000L

/**
 * This table will keep the gwKnowDeviceMap object and the key is
 * the Gateway Client ID
 */
static ismHashMap *       g_deviceCache;

/**
 * Lock for the global Gateway Device Cache
 */
static pthread_mutex_t g_deviceCacheLock ;

static int authInited=0;
extern int g_authenticator;

extern ism_regex_t devIDMatch;
extern int         doDevIDMatch;
extern ism_regex_t devTypeMatch;
extern int         doDevTypeMatch;
extern uint64_t g_gwCleanupTime;
static void destroyDeviceInfo(void * ptr);
int ism_ssl_waitPendingCRL(ima_transport_info_t * transport, const char * org, void * callback, void * data);
void ism_proxy_auth_now(ism_transport_t * transport, double starttime_tsc);
#ifdef DEBUG
int ism_ssl_isDisableCRL(ima_transport_info_t * transport);
#endif
int disconnectDeletedDevice = 1;

/*
 * Binary im message or event entry.
 * This is used to save a metering message from when it is produced until it is sent.
 */
typedef struct DeviceUpdateObj {
    char * clientID;
    char * orgID;
    char * devType;
    char * devID;
    short  isDeleted;
} DeviceUpdateObj;

/*
 * Submit a authenticator job to the timer thread
 */
int ism_proxy_check_authenticator(ism_transport_t * transport, int which, const char * userID, const char * passwd,
            ism_proxy_AuthCallback_t callback, void * callbackParam) {
    if (g_authenticator) {
        authAction_t * action;
        int useridLen = strlen(userID) + 1;
        int pwdLen;
        if(ism_throttle_setConnectReqInQ(transport->clientID, 1)){
            /*CONNECT request for this client is in the timer. Terminate this one*/
             return ISMRC_TooManyAuthRequests;
        }
        if (!passwd)
            passwd = "";
        pwdLen = strlen(passwd) + 1;
        transport->channel = ism_transport_allocBytes(transport, pwdLen, 0);
        action = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_auth,1),sizeof(authAction_t) + useridLen);
        if(UNLIKELY(action == NULL))
            return ISMRC_AllocateError;
        action->transport = transport;
        action->callback = callback;
        action->callbackParam = callbackParam;
        action->which = which;
        action->throttled = 0;
        action->userID = (char*)(action+1);
        action->password = transport->channel;
        strcpy((char*)action->userID, userID);
        strcpy((char*)action->password, passwd);

        ism_time_t delay_nanos= 1;
        if(ism_throttle_isEnabled()){
            int limit = ism_throttle_getThrottleLimit(transport->clientID, THROTTLET_HIGHEST_COUNT);
            if(limit>0){
                delay_nanos = ism_throttle_getDelayTimeInNanos(limit);
                if(delay_nanos<=0) delay_nanos=1;
                action->throttled = 1;
                TRACE(6, "Throttling: ClientID=%s delay_limit=%d delay_time=%ldd\n", transport->clientID,limit, delay_nanos  );
            }
        }
        TRACE(9, "ism_proxy_check_authenticator: connection=%u\n", transport->index);
        ism_common_setTimerOnce(ISM_TIMER_HIGH, ism_proxyAuthenticate, action, delay_nanos);

        return ISMRC_AsyncCompletion;
    } else {
        return 0;
    }
}

typedef struct callbackInfo_t {
    ism_proxy_AuthCallback_t callback;
    void * callbackParam;
} callbackInfo_t;

/*
 * Delay ContinueAuthTask.
 */
static int delayContinueAuthTask(ism_timer_t key, ism_time_t timestamp, void * userdata){
    callbackInfo_t * cbInfo = (callbackInfo_t *) userdata;
    cbInfo->callback(0, cbInfo->callbackParam);
    ism_common_cancelTimer(key);
    ism_common_free(ism_memory_proxy_auth,cbInfo);
    return 0;
}

/*
 * Structure to save parameters to doAuthenticate to allow it to be restarted
 */
typedef struct doAuthInfo_t {
    ism_transport_t * transport;
    int user_len;
    int pw_len;
    ism_proxy_AuthCallback_t callback;
    void * callbackParam;
} doAuthInfo_t;


/*
 * Reply from a pending CRL Check
 */
void replyCRLCheck(int rc, void * userdata) {
    int  xrc;
    doAuthInfo_t * daInfo = (doAuthInfo_t *) userdata;
    const char * userName = (const char *)(daInfo+1);
    const char * pwd = userName + daInfo->user_len + 1;
    ism_transport_t * transport = daInfo->transport;
    /* TODO: Do we need to move to IOP ? */
    if (rc) {
        transport->crlStatus = ism_proxy_mapCrlReturnCode(rc);
        const char * sslErr = X509_verify_cert_error_string(rc);
        TRACE(4, "Certificate verification failed: connect=%d from=%s:%d error=%s (%d)\n",
                     transport->index, transport->client_addr, transport->clientport, sslErr, rc);
        ism_common_setErrorData(ISMRC_CertificateNotValid, "%s", sslErr);
        char * lastError = alloca(512);
        ism_common_formatLastError(lastError, 512);
        daInfo->callback(ISMRC_CertificateNotValid, daInfo->callbackParam);
        transport->close(transport, ISMRC_CertificateNotValid, 0, lastError);
        return;
    }
    TRACE(7, "replyCRLCheck: connect=%d rc=%d userName=%s pwd=%s\n", transport->index, transport->crlStatus, userName, pwd);

    /* Continue with authorization */
    ism_common_setError(0);
    transport->crtChckStatus = 0;
    xrc = ism_proxy_doAuthenticate(transport, userName, pwd, daInfo->callback, daInfo->callbackParam);
    if (xrc != ISMRC_AsyncCompletion)
        daInfo->callback(xrc, daInfo->callbackParam);
}

/*Authenticate using Local Table*/

static int ism_proxyLocalAuthenticate(ism_timer_t key, ism_time_t timestamp, void * userdata){
    /* Verify password from internal tables */
    authAction_t * action = (authAction_t *)userdata;
    ism_transport_t * transport = action->transport;
    ism_user_t * user = action->user;
    if (!ism_tenant_checkObfus(user->name, action->password, user->password)) {
      TRACE(6, "User authentication failed: user=%s connect=%u\n", user->name, transport->index);
      action->callback(ISMRC_NotAuthorized, action->callbackParam);
    }else{
      transport->auth_permissions = user->role;
      TRACE(6, "User authentication succeeded: user=%s connect=%u role=%d\n", user->name, transport->index, transport->auth_permissions);
      action->callback(0, action->callbackParam);
    }
    return 0;
}
/*
 * Submit a local authenticator job to the timer thread
 */
int ism_proxy_localauthenticator(ism_transport_t * transport, ism_user_t * user,  int which, const char * userID, const char * passwd,
            ism_proxy_AuthCallback_t callback, void * callbackParam) {
    authAction_t * action;
    int useridLen = strlen(userID) + 1;
    int pwdLen;
    if(ism_throttle_setConnectReqInQ(transport->clientID, 1)){
        /*CONNECT request for this client is in the timer. Terminate this one*/
         return ISMRC_TooManyAuthRequests;
    }
    if (!passwd)
        passwd = "";
    pwdLen = strlen(passwd) + 1;
    transport->channel = ism_transport_allocBytes(transport, pwdLen, 0);
    action = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_auth,1),sizeof(authAction_t) + useridLen);
    if(UNLIKELY(action == NULL))
        return ISMRC_AllocateError;
    action->transport = transport;
    action->callback = callback;
    action->callbackParam = callbackParam;
    action->which = which;
    action->throttled = 0;
    action->userID = (char*)(action+1);
    action->password = transport->channel;
    strcpy((char*)action->userID, userID);
    strcpy((char*)action->password, passwd);

    action->user = user;

    ism_time_t delay_nanos= 1;
    if(ism_throttle_isEnabled()){
        int limit = ism_throttle_getThrottleLimit(transport->clientID, THROTTLET_HIGHEST_COUNT);
        if(limit>0){
            delay_nanos = ism_throttle_getDelayTimeInNanos(limit);
            if(delay_nanos<=0) delay_nanos=1;
            action->throttled = 1;
            TRACE(6, "Throttling: ClientID=%s delay_limit=%d delay_time=%ldd\n", transport->clientID,limit, delay_nanos  );
        }
    }
    TRACE(9, "ism_proxy_localauthenticator: connection=%u\n", transport->index);
    ism_common_setTimerOnce(ISM_TIMER_HIGH, ism_proxyLocalAuthenticate, action, delay_nanos);

    return ISMRC_AsyncCompletion;

}

/*
 * Authenticate a user
 */
int ism_proxy_doAuthenticate(ism_transport_t * transport, const char * userName, const char * pwd, ism_proxy_AuthCallback_t callback, void * callbackParam) {
    int which_user = 0;
    ism_tenant_t *    tenant = transport->tenant;
    double startAuth = ism_common_readTSC();

    TRACE(8, "doAuthenticate connect=%d crl=%d\n", transport->index, transport->crtChckStatus);
    /* If a CRL download is pending, go async */
    if (transport->crtChckStatus == 9) {
        int user_len = userName ? strlen(userName) : 0;
        int pw_len = pwd ? strlen(pwd) : 0;
        //FIXME: this currently leaks
        doAuthInfo_t * daInfo = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_auth,2),sizeof(doAuthInfo_t) + user_len + pw_len + 2);
        char * data = (char *)(daInfo+1);
        if (user_len) {
            memcpy(data, userName, user_len);
            data += user_len;
        }
        *data++ = 0;
        if (pw_len) {
            memcpy(data, pwd, pw_len);
            data += pw_len;
        }
        *data++ = 0;
        daInfo->transport = transport;
        daInfo->user_len = user_len;
        daInfo->pw_len = pw_len;
        daInfo->callback = callback;
        daInfo->callbackParam = callbackParam;
        TRACE(7, "waitPendingCRL: connect=%d org=%s\n", transport->index, transport->tenant->name);
        if (!ism_ssl_waitPendingCRL((ima_transport_info_t *)transport, transport->tenant->name, replyCRLCheck, daInfo))
            return ISMRC_AsyncCompletion;
    }

    /*
     * Check if this is a secure connection if required
     */
    if (tenant->require_secure==1 && !transport->secure) {
        TRACE(6, "Connection is not secure: connect=%u\n", transport->index);
        ism_common_setErrorData(ISMRC_NotAuthorized, "%s", "A secure connection in required");
        return ISMRC_NotAuthorized;
    }

    if (tenant->require_user==1) {
        if (transport->pwIsRequired) {   /* Used for MQTTv5 */
            if (!pwd) {
                TRACE(6, "A password is required and not specified: connect=%u\n", transport->index);
                ism_common_setErrorData(ISMRC_NotAuthorized, "%s", "A password is required");
                return ISMRC_NotAuthorized;
            }
        } else {
            if (!userName) {
                TRACE(6, "A user name is reequired and not specified: connect=%u\n", transport->index);
                ism_common_setErrorData(ISMRC_NotAuthorized, "%s", "A user name is required");
                return ISMRC_NotAuthorized;
            }
        }
    }

    if (transport->secure) {
        /* This is a secure connection */
        X509 * cert = (transport->usePSK) ? NULL : SSL_get_peer_certificate(transport->ssl);
        if (cert) {
            if (!transport->cert_name || !*transport->cert_name) {
                /* If there is a client cert, get its common name */
                X509_NAME * name = X509_get_subject_name(cert);
                if (name) {
                    char commonName [2048];
                    if (X509_NAME_get_text_by_NID(name, NID_commonName, commonName, sizeof(commonName)) != -1) {
                        int len = strlen(commonName) + 1;
                        transport->cert_name = ism_transport_allocBytes(transport, len, 0);
                        memcpy((char *)transport->cert_name, commonName, len);
                    }
                }
            }
            X509_free(cert);
        } else {
            /* There is no valid client's certificate - authentication fails if certificate is required */
            if (tenant->require_cert) {
                TRACE(6, "Client certificate is not available: connect=%u\n", transport->index);
                /* Do not return error (Work item 155487) */
                //return ISMRC_NoCertificate;
            }
            if (transport->usePSK) {
                const char * identity = SSL_get_psk_identity(transport->ssl);
                int len = strlen(identity) + 1;
                transport->cert_name = ism_transport_allocBytes(transport, len, 0);
                memcpy((char *)transport->cert_name, identity, len);
            }
        }
    } else {
        if (tenant->require_cert) {
            TRACE(6, "Client certificate is not available for non-secure connection: connect=%u\n", transport->index);
        }
    }

    /* Validate for Device ID or App id */
	if (doDevIDMatch) {
		/* Make sure clientID matches regex */
		if (ism_regex_match(devIDMatch, transport->deviceID)) {
            if (transport->client_class=='A' || transport->client_class=='a') {
                TRACE(6, "The application ID is not valid: AppID=%s ClientID=%s\n", transport->deviceID, transport->clientID);
                ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", "The application ID is not valid");
            } else {
			    TRACE(6, "The device ID is not valid: DeviceID=%s ClientID=%s\n", transport->deviceID, transport->clientID);
                ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", "The device ID is not valid");
            }
			return CRC_BadIdentifier;
		}
	}

	/* Validate the Device Type or App instance */
	if (transport->typeID && *transport->typeID && doDevTypeMatch) {
		/* Make sure clientID matches regex */
		if (ism_regex_match(devTypeMatch, transport->typeID)) {
		    if (transport->client_class=='A' || transport->client_class=='a') {
	            TRACE(6, "The application instance is not valid: Instance=%s ClientID=%s\n", transport->typeID, transport->clientID);
                ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", "The application instance is not valid");
		    } else {
			    TRACE(6, "The device type is not valid: DeviceType=%s ClientID=%s\n", transport->typeID, transport->clientID);
                ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", "The device type is not valid");
		    }
			return CRC_BadIdentifier;
		}
	}

    /*
     * If user name is NULL use CN from the certificate
     */
    if (!userName) {
        userName = transport->cert_name;
        which_user = 2;
    } else if (!strcmp(userName, transport->clientID)) {
        which_user = 1;
    }

    /*
     * Check for a user name if requested
     */
    if (tenant->allow_anon != 1 && !userName) {
        TRACE(6, "Anonymous connections was not allowed: connect=%u\n", transport->index);
        ism_common_setErrorData(ISMRC_NotAuthorized, "%s", "UserName");
        return CRC_NotAuthorized;
    }

    /*This is consider quickstart. Let it pass thru and not going to authentication process*/
    if (tenant->allow_anon==1 && tenant->check_user){
        /*Check if the CONNECT request from this ClientID in the auth timer*/
        if (ism_throttle_setConnectReqInQ(transport->clientID, 1)){
            /*CONNECT request for this client is in the timer. Terminate this one*/
            ism_common_setError(ISMRC_TooManyAuthRequests);
            return ISMRC_TooManyAuthRequests;
        }
        ism_time_t delay_nanos= 1;
        if(ism_throttle_isEnabled()){
            /*Get delay time if any.*/
            int limit = ism_throttle_getThrottleLimit(transport->clientID, THROTTLET_HIGHEST_COUNT);
            if (limit>0){
                delay_nanos = ism_throttle_getDelayTimeInNanos(limit);
                if (delay_nanos<=0) delay_nanos=1;
                TRACE(6, "Throttling: ClientID=%s delay_limit=%d delay_time=%ldd\n", transport->clientID,limit, delay_nanos  );
            }
        }
        /*Set this clientID is in auth timer. If next one come in and this clientID is in timer. Disconnect it.*/
        callbackInfo_t * cbInfo = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_auth,3),sizeof(callbackInfo_t));
        cbInfo->callback = callback;
        cbInfo->callbackParam = callbackParam;
        ism_common_setTimerOnce(ISM_TIMER_HIGH, (ism_attime_t)delayContinueAuthTask, cbInfo, delay_nanos);
        return ISMRC_AsyncCompletion;
    }


    /* Check userid/passwd if requested */
    if (tenant->check_user) {
        /* Find user in local tables */
        ism_user_t * user = ism_tenant_getUser(strcmp(userName, "use-token-auth") ? userName : transport->clientID, tenant, 0);
        if (!user) {
            /*
             * Use external authenticator.  If this exists it will return async unless there is
             * no external authenticator in which case it will return 0
             */
            int rc = ism_proxy_check_authenticator(transport, which_user, userName, pwd, callback, callbackParam);
            if (rc == ISMRC_AsyncCompletion) {
                return rc;
            }
            if (rc == ISMRC_TooManyAuthRequests) {
				/* CONNECT request for this client is in the timer. Terminate this one */
               TRACE(6, "The connection or CONNECT request is in the Auth pending Queue : connect=%u\n", transport->index);
			   ism_common_setError(ISMRC_TooManyAuthRequests);
			   return ISMRC_TooManyAuthRequests;
		    }
            TRACE(6, "Failed to authenticate user: user=%s connect=%u\n", userName, transport->index);
            ism_common_setError(ISMRC_NotAuthorized);
            return ISMRC_NotAuthorized;
        }

        //Do Local Authentication
        int rc = ism_proxy_localauthenticator(transport, user, which_user, userName, pwd, callback, callbackParam);
        if (rc == ISMRC_AsyncCompletion) {
            return rc;
        }
        if (rc == ISMRC_TooManyAuthRequests) {
            /* CONNECT request for this client is in the timer. Terminate this one */
            TRACE(6, "The connection or CONNECT request is in the Auth pending Queue : connect=%u\n", transport->index);
            ism_common_setError(ISMRC_TooManyAuthRequests);
            return ISMRC_TooManyAuthRequests;
        }
        TRACE(6, "Failed to authenticate user: user=%s connect=%u\n", userName, transport->index);
        ism_common_setError(ISMRC_NotAuthorized);
        return ISMRC_NotAuthorized;

    } else {
        if (ism_throttle_setConnectReqInQ(transport->clientID, 1)) {
             /* CONNECT request for this client is in the timer. Terminate this one */
            ism_common_setError(ISMRC_TooManyAuthRequests);
            return ISMRC_TooManyAuthRequests;
        }
        if (ism_throttle_isEnabled()) {
            int limit = ism_throttle_getThrottleLimit(transport->clientID, THROTTLET_HIGHEST_COUNT);
            ism_time_t delay_nanos= ism_throttle_getDelayTimeInNanos(limit);
            if (delay_nanos>0){
                callbackInfo_t * cbInfo = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_auth,4),sizeof(callbackInfo_t));
                cbInfo->callback = callback;
                cbInfo->callbackParam = callbackParam;
                TRACE(6, "Throttling: ClientID=%s delay_limit=%d delay_time=%ldd\n", transport->clientID,limit, delay_nanos  );
                ism_common_setTimerOnce(ISM_TIMER_HIGH, (ism_attime_t)delayContinueAuthTask, cbInfo, delay_nanos);
                return ISMRC_AsyncCompletion;
            }
        }else{
            //if throttle is enabled, setConnectReqInQ to zero.
            ism_throttle_setConnectReqInQ(transport->clientID, 0);
        }
    }
    ism_proxy_auth_now(transport, startAuth);
    return ISMRC_OK;
}


/*
 * Complete an authorize publish or subscribe
 */
int ism_proxy_completeAuthorize(void * correlate, int rc, const char * reason) {
    asyncauth_t * async = (asyncauth_t *) correlate;
    async->callback(rc, reason, async->callbackParam);
    ism_common_setError(rc);
    return 0;
}

const char * ism_tenant_getTenantAlias(const char * org, const char * deviceID);

/*
 * Check if the tenant is valid including doing the alias lookup
 */
ism_tenant_t * ism_proxy_getValidTenant(ism_transport_t * transport) {
    if (transport->tenant == NULL) {
        const char * org = (transport->org) ? transport->org : "";
        ism_tenant_t * tenant;

        ism_tenant_lock();
        tenant = ism_tenant_getTenant(org);
        /* Check Alias. If any, use the alias to get the org/tenant */
		const char * alias = ism_tenant_getTenantAlias(transport->org, transport->deviceID);
		if (alias) {
			char * pos;
			tenant = ism_tenant_getTenant(alias);
			ism_tenant_unlock();
			if (tenant) {
				TRACE(5, "Tenant alias used: connect=%d ClientID=%s old=%s new=%s\n", transport->index, transport->name?transport->name:"", org, alias);
				pos = strstr(transport->name, org);
				if (pos)
					memcpy(pos, alias, strlen(alias));   /* Replace org in ClientID */
				strcpy((char *)transport->org, alias);   /* Replace org             */
			} else {
				TRACE(4, "Tenant alias cannot be used because the new tenant is not known: connect=%d ClientID=%s old=%s new=%s\n",
				        transport->index, transport->name?transport->name:"", org, alias);
			}
		} else {
			ism_tenant_unlock();
		}
		if (!tenant) {
			TRACE(5, "Unknown organization: org=\"%s\" clientID=\"%s\" client_addr=%s connect=%u\n",
					org, transport->name, transport->client_addr, transport->index);
			if(ism_common_conditionallyLogged(NULL, ISM_LOGLEV(INFO), ISM_MSG_CAT(Server), 922, TRACE_DOMAIN, transport->name, transport->client_addr, NULL) == 0){
				LOG(INFO, Server, 922, "%-s%u%s%u", "The organization is not known: ClientID={0} Connect={1} From={2}:{3}.",
						transport->name, transport->index, transport->client_addr, transport->clientport);
			}
			return NULL;
		}

        if (!tenant->enabled) {
            TRACE(5, "The organization config is not valid: org=\"%s\" clientID=\"%s\" client_addr=%s, connect=%u\n",
                    org, transport->name, transport->client_addr, transport->index);
            if(ism_common_conditionallyLogged(NULL, ISM_LOGLEV(INFO), ISM_MSG_CAT(Server), 923, TRACE_DOMAIN, transport->name, transport->client_addr, NULL) ==0){
                LOG(INFO, Server, 923, "%-s%u%s%u", "The organization config is not valid: ClientID={0} Connect={1} From={2}:{3}.",
                        transport->name, transport->index, transport->client_addr, transport->clientport);
            }
            return NULL;
        }
#ifdef DEBUG
        TRACE(8, "disableCRL: client=%s sni=%s disableCRL=%u\n", transport->name,
                transport->sniName ? transport->sniName : "",
                ism_ssl_isDisableCRL((ima_transport_info_t *)transport));
#endif

        transport->tenant = tenant;
        transport->server = tenant->server;
    }
    return transport->tenant;
}



/*
 * Check that the certificate subject name matches the type and id from the ClientID
 * @return 0=no match, 1=partial match, 3=exact match
 */
static int matchCertName(ism_transport_t * transport, const char * subject) {
    if (subject && *subject && subject[1] == ':') {
        /* For device the format is d:devtyp:devid where the devid can be empty to indicate any */
        if (*subject == 'd' || *subject == 'g') {
            if (transport->client_class == *subject) {
                char * colon = strchr(subject+2, ':');
                if (colon) {
                    if (colon-subject-2 == strlen(transport->typeID) &&
                        !memcmp(subject+2, transport->typeID, colon-subject-2)) {
                        if (!strchr(colon+1, ':')) {
                            if (!colon[1])
                                return 1;
                            else
                                return strcmp(colon+1, transport->deviceID)?0:3;
                        }
                    }
                }
            }
        }

        /* For application the format is a:appid where the appid can be empty to indicate any */
        else if (*subject == 'a' || *subject == 'A') {
            if (transport->client_class == 'a' || transport->client_class == 'A') {
                if (!strchr(subject+2, ':')) {
                    if (!subject[2])
                        return 1;
                    else
                        return strcmp(subject+2, transport->deviceID)?0:3;
                }
            }
        }
    }
    return 0;
}

struct ssl_st * ism_transport_getSSL(ism_transport_t * transport);
/*
 * Match common name and SubjectAltName from certificate
 */
int ism_proxy_matchCertNames(ism_transport_t * transport) {
    int name_match = matchCertName(transport, transport->cert_name);
    int best_match = name_match;
    if (best_match < 3) {
        struct ssl_st * ssl = ism_transport_getSSL(transport);
        if (ssl) {
            char * xxbuf = alloca(512);
            int  ii;
            concat_alloc_t sanbuf = {xxbuf, 512};
#ifdef CUNIT
            int sancount = 0;
            if (transport->tlsReadBytes == 0) {
                memcpy(sanbuf.buf, transport->client_host, transport->clientport);
                sanbuf.used = transport->clientport;
                sancount = transport->serverport;
            }
#else
            int  sancount = ism_ssl_getSubjectAltNames(ssl, &sanbuf);
#endif
            char * san = sanbuf.buf;
            for (ii=0; ii<sancount; ii++) {
                name_match = matchCertName(transport, san);
                if (name_match)
                    best_match = name_match;
                if (best_match == 3)
                    break;
                san += (strlen(san)+1);
            }
            if (sanbuf.inheap)
                ism_common_freeAllocBuffer(&sanbuf);
        }
    }
    return best_match;
}


/*
 * Gernerate a device cache key
 */
static inline int generateGWkey(char * buf, int buflen, const char * org, const char * devtype, const char * devid) {
    return snprintf(buf, buflen, "%s:%s:%s", ((org) ? org : ""), ((devtype) ? devtype : ""), ((devid) ? devid : ""));
}

/*Get Gateway Device Info. The device info object will be locked when returned
 *Need to use ism_proxy_unlockDeviceInfo to return the device info
 * */
dev_info_t *  ism_proxy_getDeviceAuthInfo(const char * org, const char * devtype, const char * devid)
{
	dev_info_t * deviceInfo = NULL;

	if (g_deviceCache) {

		char * key = alloca(1024);
		int keyLen = generateGWkey(key, 1024, org, devtype, devid);
		if (keyLen >= 1024) {
			key = alloca(keyLen + 32);
			keyLen = generateGWkey(key, (keyLen + 32), org, devtype, devid);
		}

		pthread_mutex_lock(&g_deviceCacheLock);
		if(g_deviceCache){
			deviceInfo = ism_common_getHashMapElement(g_deviceCache, key, keyLen);

			if(deviceInfo){
				TRACE(6, "getDeviceInfo. deviceInfo=%p org=%s devtype=%s devid=%s\n",deviceInfo, org, devtype, devid );
				/**
				 * Lock device Info Object before return.
				 * Caller need to call ism_proxy_unlockDeviceInfo API
				 * to unlock the object
				 */
				pthread_spin_lock(&deviceInfo->lock);
			}
		}
		pthread_mutex_unlock(&g_deviceCacheLock);


	}
	return deviceInfo;
}


/*Set Gateway Device Info*/
int  ism_proxy_setDeviceAuthInfo(const char * org, const char * devtype, const char * devid, dev_info_t ** outDevInfo)
{
	dev_info_t * deviceInfo = NULL;
	int rc = 0;

	if (g_deviceCache) {

		char * key = alloca(1024);
		int keyLen = generateGWkey(key, 1024, org, devtype, devid);
		if (keyLen >= 1024) {
			key = alloca(keyLen + 32);
			keyLen = generateGWkey(key, (keyLen + 32), org, devtype, devid);
		}
		pthread_mutex_lock(&g_deviceCacheLock);
		if(g_deviceCache){
			deviceInfo = ism_common_getHashMapElement(g_deviceCache, key, keyLen);
			if(!deviceInfo){
				deviceInfo = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_device_auth,1),1, sizeof(dev_info_t));

				 //This table contains the link list of async object for each transport
				deviceInfo->pendingAuthRequestTable = ism_common_createHashMap(128,HASH_STRING);
				pthread_spin_init(&deviceInfo->lock, 0);
				ism_common_putHashMapElement(g_deviceCache, key, keyLen, deviceInfo, NULL);
				deviceInfo->devid = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_device_auth,1000),devid);
				deviceInfo->devtype = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_device_auth,1000),devtype);
				TRACE(6, "setDeviceInfo. deviceInfo=%p org=%s devtype=%s devid=%s\n", deviceInfo, org, devtype, devid );
			}else{
				TRACE(6, "setDeviceInfo. Device Info is already existed. deviceInfo=%p org=%s devtype=%s devid=%s\n", deviceInfo, org, devtype, devid);
			}
			if(outDevInfo!=NULL){
				pthread_spin_lock(&deviceInfo->lock);
				*outDevInfo = deviceInfo;
			}
		}else{
			rc=1;
		}
		pthread_mutex_unlock(&g_deviceCacheLock);


	}else{
		return 1;
	}
	return rc;
}

/**
 * Search and Delete the Device info
 */
int ism_proxy_deleteDeviceAuthInfo(const char * org, const char * devtype, const char * devid)
{
	if (g_deviceCache) {

		char * key = alloca(1024);
		int keyLen = generateGWkey(key, 1024, org, devtype, devid);
		if (keyLen >= 1024) {
			key = alloca(keyLen + 32);
			keyLen = generateGWkey(key, (keyLen + 32), org, devtype, devid);
		}
		pthread_mutex_lock(&g_deviceCacheLock);
		if(g_deviceCache){
			dev_info_t * deviceInfo = ism_common_getHashMapElement(g_deviceCache, key, keyLen);
			if(deviceInfo!=NULL){
				TRACE(5, "deleteDeviceAuthInfo: deviceInfo=%p org=%s devtype=%s devid=%s\n", deviceInfo, org, devtype, devid );
				//Remove from the table.
				ism_common_removeHashMapElement(g_deviceCache, key, keyLen);
				destroyDeviceInfo(deviceInfo);
			}
		}

		pthread_mutex_unlock(&g_deviceCacheLock);
	}
	return 0;
}

XAPI int ism_proxy_setDeviceAuthPendingRequest(dev_info_t * deviceInfo, asyncauth_t * async, const char * name)
{
	int rc = 0;
	rc = ism_common_putHashMapElement(deviceInfo->pendingAuthRequestTable, name, strlen(name), async, NULL);
	TRACE(6, "setDeviceAuthPendingRequest. deviceInfo=%p async=%p name=%s\n", deviceInfo, async, name );
	return rc;

}

XAPI asyncauth_t * ism_proxy_getDeviceAuthPendingRequest(dev_info_t * deviceInfo, const char *name)
{

	asyncauth_t * pendingAuthRequest = ism_common_getHashMapElement(deviceInfo->pendingAuthRequestTable, name, strlen(name));
	TRACE(6, "getDeviceAuthPendingRequest. devInfo=%p async=%p name=%s\n", deviceInfo, pendingAuthRequest, name );
	return pendingAuthRequest;
}

XAPI asyncauth_t * ism_proxy_setDeviceAuthComplete(dev_info_t * deviceInfo, int authRC, int createRC, uint64_t authtime, const char *name)
{
	asyncauth_t * pendingAuthRequest=NULL;
	/*Remove the pending auth request for a transport, and drain out all auth async*/
	pendingAuthRequest = ism_common_removeHashMapElement(deviceInfo->pendingAuthRequestTable, name, strlen(name));
	deviceInfo->authRC = authRC;
	deviceInfo->createRC = createRC;
	deviceInfo->authTime = authtime;
	TRACE(6, "setDeviceAuthComplete. devInfo=%p authRC=%d createRC=%d authtime=%llu\n", deviceInfo, authRC, createRC,  (ULL)authtime );
	return pendingAuthRequest;

}

XAPI int ism_proxy_checkDeviceAuth(dev_info_t * deviceInfo, uint64_t currTime, const char * name)
{
	int rc=-1;
	uint64_t currentTime = currTime;

	if(currentTime==0){
		currentTime = (uint64_t)(ism_common_readTSC() * 1000.0);
	}

	asyncauth_t * pendingAuthRequest = ism_common_getHashMapElement(deviceInfo->pendingAuthRequestTable, name, strlen(name));
	if (pendingAuthRequest == NULL) {
		if (deviceInfo->authRC || deviceInfo->createRC) {
			if ((currentTime - deviceInfo->authTime) < g_gwCleanupTime) {
				rc = ISMRC_NotAuthorized;
				ism_common_setErrorData(rc, "%s%s%s", "Device not registered:", deviceInfo->devtype, deviceInfo->devid);
			} else {
				rc=-1;
				deviceInfo->authRC = 0;
				deviceInfo->createRC = 0;
				deviceInfo->authTime = 0;
			}
		} else {
			rc = 0;//Already authorized
		}
	}

	TRACE(6, "checkDeviceAuth. devInfo=%p rc=%d authtime=%llu authrc=%d createrc=%d\n", deviceInfo, rc,  (ULL)currentTime, deviceInfo->authRC,deviceInfo->createRC );
	return rc;
}

int ism_proxy_initAuth(void)
{

	if (!authInited){

		pthread_mutex_init(&g_deviceCacheLock, 0);
		g_deviceCache = ism_common_createHashMap(128,HASH_STRING);

		disconnectDeletedDevice = ism_common_getIntConfig("DisconnectDeletedDevice", 1);

		authInited=1;

		TRACE(5, "Proxy Auth is initialized.\n");
	}
	return 0;
}

static void destroyDeviceInfo(void * ptr)
{
	dev_info_t * deviceInfo = (dev_info_t *)ptr;
	TRACE(6, "destroyDeviceInfo: deviceInfo=%p", deviceInfo);
	pthread_spin_lock(&deviceInfo->lock);

	if(deviceInfo->devid != NULL){
		ism_common_free(ism_memory_proxy_device_auth,deviceInfo->devid);
	}
	if(deviceInfo->devtype!=NULL){
		ism_common_free(ism_memory_proxy_device_auth,deviceInfo->devtype);
	}
	//Destroy authasync table
	if(deviceInfo->pendingAuthRequestTable!=NULL)
		ism_common_destroyHashMap(deviceInfo->pendingAuthRequestTable);
	pthread_spin_unlock(&deviceInfo->lock);
	pthread_spin_destroy(&deviceInfo->lock);
	ism_common_free(ism_memory_proxy_device_auth,deviceInfo);

}

/*
 * Terminate auth
 */
int ism_proxy_termAuth(void) {
	if (authInited) {
		authInited=0;

		pthread_mutex_lock(&g_deviceCacheLock);

		ism_common_destroyHashMapAndFreeValues(g_deviceCache, destroyDeviceInfo);

		pthread_mutex_unlock(&g_deviceCacheLock);


		TRACE(5, "Proxy Auth is terminated.\n");

	}

	return 0;
}


/*
 * Return a string from a JSON entry even for things without a value.
 */
const char * getJsonValue(ism_json_entry_t * ent) {
    if (!ent)
        return "";
    switch (ent->objtype) {
    case JSON_True:    return "true";
    case JSON_False:   return "false";
    case JSON_Null:    return "null";
    case JSON_Object:  return "object";
    case JSON_Array:   return "array";
    case JSON_Integer:
    case JSON_String:
    case JSON_Number:
        return ent->value;
    }
    return "";
}



/*
 * This function will update cache or disconnect the connection for deleted device.
 */
int ism_deviceUpdateTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
	DeviceUpdateObj* deviceUpdateObj = (DeviceUpdateObj *)userdata;
	char * orgId = deviceUpdateObj->orgID;
	char * deviceType = deviceUpdateObj->devType;
	char * deviceId = deviceUpdateObj->devID;
	char * clientID = deviceUpdateObj->clientID;
	short deleted = deviceUpdateObj->isDeleted;

	 if (key)
		 ism_common_cancelTimer(key);

	TRACE(5, "DeviceUpdateTimer: ClientID=%s Org=%s devType=%s devId=%s deleted=%d\n",clientID, orgId, deviceType, deviceId, deleted);

	if(deleted){
		TRACE(7, "DeviceUpdateTimer: deleteDeviceAuthInfo: Name=%s Org=%s devType=%s devId=%s deleted=%d\n",clientID, orgId, deviceType, deviceId, deleted);
		ism_proxy_deleteDeviceAuthInfo(orgId, deviceType, deviceId);

		/*Close the device connection also*/
		if(disconnectDeletedDevice){
			TRACE(7, "DeviceUpdateTimer: closeConnection: Name=%s Org=%s devType=%s devId=%s deleted=%d\n",clientID, orgId, deviceType, deviceId, deleted);
			ism_transport_closeConnection(clientID, NULL, NULL,NULL, NULL, NULL, 0);
		}
	}

	if(deviceUpdateObj->clientID!=NULL)
		ism_common_free(ism_memory_proxy_device_auth,deviceUpdateObj->clientID);
	if(deviceUpdateObj->devID!=NULL)
		ism_common_free(ism_memory_proxy_device_auth,deviceUpdateObj->devID);
	if(deviceUpdateObj->devType!=NULL)
		ism_common_free(ism_memory_proxy_device_auth,deviceUpdateObj->devType);
	if(deviceUpdateObj->orgID!=NULL)
		ism_common_free(ism_memory_proxy_device_auth,deviceUpdateObj->orgID);

	ism_common_free(ism_memory_proxy_device_auth,deviceUpdateObj);

	return 0;
}

/**
 * Runtime updates for Gateway device
 */
int ism_proxy_deviceUpdate(ism_json_parse_t * parseobj, int where, const char * name) {

	int rc=0;
	int endloc;
	const char * orgId = NULL;
	const char * deviceType = NULL;
	const char * deviceId = NULL;
	int deleted=0;

	if (!parseobj || where > parseobj->ent_count)
		return 1;
	ism_json_entry_t * ent = parseobj->ent+where;
	endloc = where + ent->count;
	where++;
	if (!name)
		name = ent->name;

	while (where <= endloc) {
		ent = parseobj->ent + where;

		if (!strcmpi(ent->name, "deleted")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "deleted", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }else{
            	deleted = ent->objtype == JSON_True;
            }
        }else if (!strcmpi(ent->name, "deviceId")) {
			 if (ent->objtype != JSON_String) {
				ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "deviceId", getJsonValue(ent));
				rc = ISMRC_BadPropertyValue;
			} else {
				deviceId = ent->value;
			}

		} else if (!strcmpi(ent->name, "deviceType")) {
			 if (ent->objtype != JSON_String) {
				ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "deviceType", getJsonValue(ent));
				rc = ISMRC_BadPropertyValue;
			} else {
				deviceType = ent->value;
			}

		}  else if (!strcmpi(ent->name, "Org")) {
			 if (ent->objtype != JSON_String) {
				ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Org", getJsonValue(ent));
				rc = ISMRC_BadPropertyValue;
			} else {
				orgId = ent->value;
			}

		} else {
			/* Having an unknown config item is not an error to allow for system upgrade */
			 LOG(WARN, Server, 991, "%-s", "Unknown Device property: Property={1}",
						  ent->name);
		}
		if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
			where += ent->count + 1;
		else
			where++;
	}




	TRACE(5, "DeviceUpdate: Name=%s Org=%s devType=%s devId=%s deleted=%d rc=%d\n",name, orgId, deviceType, deviceId, deleted, rc);

	if(rc==ISMRC_OK ){

		DeviceUpdateObj * deviceUpdateObj = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_device_auth,2),1, sizeof(DeviceUpdateObj));
		deviceUpdateObj->clientID = (char *) ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_device_auth,1000),name);
		deviceUpdateObj->isDeleted= deleted;
		if(orgId!=NULL)
			deviceUpdateObj->orgID = (char *) ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_device_auth,1000),orgId);
		if(deviceType!=NULL)
			deviceUpdateObj->devType = (char *) ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_device_auth,1000),deviceType);
		if(deviceType!=NULL)
				deviceUpdateObj->devID = (char *) ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_device_auth,1000),deviceId);

		ism_common_setTimerOnce(ISM_TIMER_LOW, ism_deviceUpdateTimer, deviceUpdateObj, 1000);
	}

	return rc;
}



/*
 * Make the ACL no longer the active ACL
 */
void ism_proxy_unlockDeviceInfo(dev_info_t * deviceInfo)
{
    if (deviceInfo) {
        pthread_spin_unlock(&deviceInfo->lock);
    }
}

/*Get the ACL Key*/
const char * ism_proxy_getACLKey(ism_transport_t * transport){
	const char * aclKey;
	//If quickstart case. return NULL
	if (transport->tenant->allow_anon==1 && transport->tenant->check_user){
		return NULL;
	}
	if(transport->client_class=='a' || transport->client_class=='A'){
		aclKey=transport->userid;
	}else{
		aclKey=transport->name;
	}
	return aclKey;
}
