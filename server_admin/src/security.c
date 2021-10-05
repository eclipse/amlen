/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

#include "securityInternal.h"
#include "authentication.h"
#include <security.h>

/*
 * security.c - Contains security init and terminate functions
 *
 */

extern void ism_ssl_init(int useFips, int useBufferPool);
extern int ism_security_createPolicyFromProps(ism_prop_t *polprops, int type, char *polname, char *cfgname, int pos );
extern ism_prop_t * ism_security_createPolicyPropsFromJson(const char *policyBuf, int *type);
extern int ism_security_policy_update(ism_prop_t *props, char *oldname, int flag);
extern int ism_security_ldap_update(ism_prop_t *props, char *oldname, int flag);
extern int ism_security_ltpa_update(ism_prop_t * props, char * oldName, ism_ConfigChangeType_t flag);
extern int ism_sceurity_oauth_update(ism_prop_t * props, char * oldName, ism_ConfigChangeType_t flag);
extern void ism_security_cleanAuthCache(void);
extern ism_prop_t * ism_config_json_getProperties(ism_config_t *compHandle, const char *objectType, const char *objectName, int *doesObjExist, int mode);
extern void ism_admin_setAdminInitErrorExternalLDAP(int rc);

static ism_timer_t cleanup_timer = 0;
static int ism_security_cacheCleanupTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata);

/* Define internal policies */
#define MAX_INTERNAL_POLICIES 3
const char *internalPolStrings[MAX_INTERNAL_POLICIES] = {
    "{ \"Item\":\"ConnectionPolicy\",\"Name\":\"!MQConnectivityConnectionPolicy\",\"Protocol\":\"MQ\" }",
    "{ \"Item\":\"QueuePolicy\",\"Name\":\"!MQConnectivityQueuePolicy\",\"Queue\":\"*\",\"ActionList\":\"Send,Receive\" }",
    "{ \"Item\":\"TopicPolicy\",\"Name\":\"!MQConnectivityTopicPolicy\",\"Topic\":\"*\",\"ActionList\":\"Publish,Subscribe\" }"
};

/* Security locks */
pthread_spinlock_t ltpaconfiglock;
pthread_spinlock_t ldapconfiglock;
pthread_spinlock_t oauthconfiglock;
pthread_rwlock_t   policylock;

int securityLocksInited = 0;
int criticalConfigError = 0;
int auditLogControl = 0;

ismSecPolicies_t    *policies    = NULL;
ismSecLDAPObjects_t *ldapobjects = NULL;
ismLTPAObjects_t    *ltpaobjects = NULL;
ismOAuthObjects_t   *oauthobjects = NULL;

int ldapstatus = ISMRC_OK;


/* variables to handle build order issues */
disconnClientNotificationThread_f disconnClientNotificationThread = NULL;
transGetSecProfile_f transGetSecProfile = NULL;


/* Auth action string
 * NOTE: Type order should match with ismSecurityAuthActionType_t ENUM defined in security.h
 */
struct {
    int type;
    int validationType;
    char *name;
} secActionTypes[] = {
    { ismSEC_AUTH_ACTION_GET,       ismSEC_POLICY_CONFIG,       "get"         },
    { ismSEC_AUTH_ACTION_SET,       ismSEC_POLICY_CONFIG,       "set"         },
    { ismSEC_AUTH_ACTION_START,     ismSEC_POLICY_CONFIG,       "start"       },
    { ismSEC_AUTH_ACTION_STOP,      ismSEC_POLICY_CONFIG,       "stop"        },
    { ismSEC_AUTH_ACTION_BACKUP,    ismSEC_POLICY_CONFIG,       "backup"      },
    { ismSEC_AUTH_ACTION_RESTORE,   ismSEC_POLICY_CONFIG,       "restore"     },
    { ismSEC_AUTH_ACTION_DIAG,      ismSEC_POLICY_CONFIG,       "diag"        },
    { ismSEC_AUTH_ACTION_APPLY,     ismSEC_POLICY_CONFIG,       "apply"       },
    { ismSEC_AUTH_ACTION_IMPORT,    ismSEC_POLICY_CONFIG,       "import"      },
    { ismSEC_AUTH_ACTION_EXPORT,    ismSEC_POLICY_CONFIG,       "export"      },
    { ismSEC_AUTH_ACTION_STATUS,    ismSEC_POLICY_CONFIG,       "status"      },
    { ismSEC_AUTH_ACTION_VERSION,   ismSEC_POLICY_CONFIG,       "version"     },
    { ismSEC_AUTH_ACTION_CONNECT,   ismSEC_POLICY_CONNECTION,   "connect"     },
    { ismSEC_AUTH_ACTION_SEND,      ismSEC_POLICY_QUEUE,        "send"        },
    { ismSEC_AUTH_ACTION_RECEIVE,   ismSEC_POLICY_QUEUE,        "receive"     },
    { ismSEC_AUTH_ACTION_BROWSE,    ismSEC_POLICY_QUEUE,        "browse"      },
    { ismSEC_AUTH_ACTION_PUBLISH,   ismSEC_POLICY_TOPIC,        "publish"     },
    { ismSEC_AUTH_ACTION_SUBSCRIBE, ismSEC_POLICY_TOPIC,        "subscribe"   },
    { ismSEC_AUTH_ACTION_CONTROL,   ismSEC_POLICY_SUBSCRIPTION, "control"     },
    { ismSEC_AUTH_ACTION_CONFIGURE, ismSEC_POLICY_CONFIG,       "configure"   },
    { ismSEC_AUTH_ACTION_VIEW,      ismSEC_POLICY_CONFIG,       "view"        },
    { ismSEC_AUTH_ACTION_MONITOR,   ismSEC_POLICY_CONFIG,       "monitor"     },
    { ismSEC_AUTH_ACTION_MANAGE,    ismSEC_POLICY_CONFIG,       "manage"      },
    { ismSEC_AUTH_ACTION_LAST,      ismSEC_POLICY_LAST,         "unsupported" }
};


/* Timer task to periodically check if the cache for authentication or group cache are expired */
static int ism_security_cacheCleanupTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    /*Clean up the authencation cache*/
    ism_security_cleanAuthCache();
    return 1;
}

/* Configuration callback for security objects */
int ism_security_config(char * object, char * name, ism_prop_t * props, ism_ConfigChangeType_t flag) {
//    char * oldname = NULL;
    int rc = ISMRC_InvalidPolicy;

    bool isLDAPObj = false;
    bool isLTPAObj = false;
    bool isOAuthObj= false;

    if ( !object || *object == '\0' || !props ) {
        rc = ISMRC_NullPointer;
        goto CONFCALLBACK_END;
    }

    // set object type
    if ( !strcmp(object, "LDAP") ) {
        isLDAPObj = true;
    } else if ( !strcmp(object, "LTPAProfile") ) {
        isLTPAObj = true;
    } else if ( !strcmp(object, "OAuthProfile") ) {
        isOAuthObj = true;
    }

    switch (flag) {

    /* For rename is not implemented yet - return error */
    case ISM_CONFIG_CHANGE_NAME:
        rc = ISMRC_NotImplemented;
        break;

    /* Process update or delete cases */
    case ISM_CONFIG_CHANGE_PROPS:
    case ISM_CONFIG_CHANGE_DELETE:
         if ( isLDAPObj == true ) {
            rc = ism_security_ldap_update(props, name, flag);
        } else if ( isLTPAObj == true ) {
            rc = ism_security_ltpa_update(props, name, flag);
        } else if ( isOAuthObj == true ) {
            rc = ism_security_oauth_update(props, name, flag);
        } else {
            rc = ism_security_policy_update(props, name, flag);
        }
        break;

    default:
        break;
    }

CONFCALLBACK_END:

    if ( rc != ISMRC_OK && rc != ISMRC_PolicyInUse){
    	if(rc==ISMRC_ClientAddrMaxError)
			ism_common_setErrorData(rc,"%d", MAX_ADDR_PAIRS);
		else
			ism_common_setError(rc);
    }

    return rc;
}


/**
 * Initialize security locks, structures and required function pointers
 */
XAPI int ism_security_init_locks(void) {
    int rc = ISMRC_OK;

    if ( securityLocksInited == 0 ) {
        TRACE(4, "Security: Initialize locks\n");
        pthread_rwlockattr_t rwlockattr_init;

        securityLocksInited = 1;

        /* initialize security policies */
        pthread_rwlockattr_init(&rwlockattr_init);
        pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
        pthread_rwlock_init(&policylock, &rwlockattr_init);

        pthread_spin_init(&ldapconfiglock, 0);
        pthread_spin_init(&ltpaconfiglock, 0);
        pthread_spin_init(&oauthconfiglock, 0);
    }

    return rc;
}


/*
 * Initialize security service
 */
XAPI int32_t ism_security_init(void) {
    int32_t rc = ISMRC_OK;
    int nopolicies = 0;

    /* Init locks */
    ism_security_init_locks();

    /*Init SSL with FIPS*/
    int isFIPSEnabled = ism_config_getFIPSConfig();
    int sslUseBufferPool = ism_config_getSslUseBufferPool();
    ism_ssl_init(isFIPSEnabled, sslUseBufferPool);
    TRACE(5, "Security: Initialized SSL with FIPS %s.\n", (isFIPSEnabled)?"enabled":"disabled");

    /* Setup function pointers to handle build order */

    /* get function pointer to ism_transport_getSecProfile() function */
    transGetSecProfile = (transGetSecProfile_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_getSecProfile_fnptr", 0L);

    /*Get monitoring startDisconnectedClientNotificationThread function pointer*/
    disconnClientNotificationThread = (disconnClientNotificationThread_f)(uintptr_t)ism_common_getLongConfig("_ism_monitoring_startDisconnectedClientNotificationThread", 0L);

    /* Initialize policy data structure */
    TRACE(4, "Security: Initialize Policy\n");
    policies = (ismSecPolicies_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,47),1, sizeof(ismSecPolicies_t));
    if (!policies) {
        TRACE(2, "Unable to initialize policies\n");
        return ISMRC_AllocateError;
    }

    /* Process internal policies */
    int i = 0;
    for (i=0; i<MAX_INTERNAL_POLICIES; i++) {
        char *polstr = (char *)internalPolStrings[i];
        int type;
        ism_prop_t * props = ism_security_createPolicyPropsFromJson(polstr, &type);
        if (props == NULL) {
            TRACE(3, "Security: Invalid Internal policy: %s\n", polstr);
            continue;
        }

        int rc1 = ism_security_createPolicyFromProps(props, type, NULL, NULL, 0);
        if (rc1 == 0)
            nopolicies += 1;
        else
            TRACE(3, "Security: Could not create static policy: RC=%d: %s\n", rc1, polstr);

        ism_common_freeProperties(props);
    }

    /* Register for the security callbacks. */
    ism_config_t * confhandle = NULL;
    ism_config_register(ISM_CONFIG_COMP_SECURITY, NULL, ism_security_config, &confhandle);

    /* Get the server dynamic config and run it */
    int doesObjExist = 0;
    ism_prop_t *props = ism_config_json_getProperties(confhandle, NULL, NULL, &doesObjExist, 0);

    /* Defect 241356: we do not need to get auditLogControl here and because of the way we're
         * retrieving the properties in ism_security_init, the object we're querying has no
         * information on SecurityLog. To avoid getting auditLogControl set to the default value
         * at startup we're just going to comment this out.
        auditLogControl = ism_config_getSecurityLog(); */

    ism_security_policy_update(props, NULL, ISM_CONFIG_CHANGE_PROPS);
    TRACE(4, "Security: Policies are loaded\n");

    LOG(NOTICE, Security, 6109, NULL, "Security in " IMA_PRODUCTNAME_FULL " is initialized.");

    if ( props )
        ism_common_freeProperties(props);

    return rc;
}

/* Initialize authentication objects */
XAPI int ism_authentication_init(void) {
    int rc = ISMRC_OK;
    /* initialize authentication objects */
    if ( ism_admin_getServerProcType() == ISM_PROTYPE_SERVER ) {
        /* Register for the security callbacks. */
        ism_config_t * confhandle = ism_config_getHandle(ISM_CONFIG_COMP_SECURITY, NULL);

        /* Get the server dynamic config and run it */
        int doesObjExist = 0;
        ism_prop_t *props = ism_config_json_getProperties(confhandle, NULL, NULL, &doesObjExist, 0);

        if ( !props ) {
            rc = ISMRC_ObjectNotFound;
            ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "Security", "Properties");
            goto SECINITEND;
        }

        ldapobjects = (ismSecLDAPObjects_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,48),1, sizeof(ismSecLDAPObjects_t));
        if (!ldapobjects) {
            TRACE(2, "Security: Unable to initialize ldap objects\n");
            if ( props )
                ism_common_freeProperties(props);
            rc = ISMRC_AllocateError;
            goto SECINITEND;
        }
        ldapstatus = ism_security_ldap_update(props, NULL, ISM_CONFIG_CHANGE_PROPS);
        TRACE(4, "Security: External LDAP support is initialized. status=%d\n", ldapstatus);

        /* Initialize LTPA object */
        ltpaobjects = (ismLTPAObjects_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,49),1, sizeof(ismLTPAObjects_t));
        if (!ltpaobjects) {
            TRACE(2, "Security: Unable to initialize LTPA objects\n");
            if ( props )
                ism_common_freeProperties(props);
            rc = ISMRC_AllocateError;
            goto SECINITEND;
        }
        ism_security_ltpa_update(props, NULL, ISM_CONFIG_CHANGE_PROPS);
        TRACE(4, "Security: LTPA support is initialized.\n");

        /* Initialize OAuth object */
        oauthobjects = (ismOAuthObjects_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,50),1, sizeof(ismOAuthObjects_t));
        if (!oauthobjects) {
            TRACE(2, "Security: Unable to initialize OAuth objects\n");
            if ( props )
                ism_common_freeProperties(props);
            rc = ISMRC_AllocateError;
            goto SECINITEND;
        }
        ism_security_oauth_update(props, NULL, ISM_CONFIG_CHANGE_PROPS);
        TRACE(4, "Security: OAuth support is initialized.\n");

        /* Initialize the Authentication service */
        ism_security_initAuthentication(props);
        TRACE(4, "Security: Authentication service is initialized.\n");

        if ( getenv("CUNIT") == NULL ) {
            TRACE(9, "Security: Set Security Timer Task: cleanup_timer=%llu\n", (ULL)cleanup_timer);
            if (!cleanup_timer) {
                /*Set the timer to wake up every 5 minutes */
                cleanup_timer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) ism_security_cacheCleanupTimerTask, NULL, 120, 120, TS_SECONDS);
            }
        }

        if (props)
            ism_common_freeProperties(props);
    }

SECINITEND:

    if ( ldapstatus != ISMRC_OK ) {
        ism_admin_setAdminInitErrorExternalLDAP(ldapstatus);
    }

    return rc;
}

/* Terminate security */
XAPI int32_t ism_security_term(void) {
    int rc = ISMRC_OK;
    rc = ism_security_termAuthentication();
    return rc;
}


/* Returns Security action type */
XAPI int ism_security_getActionType(char *name) {
    int ret = ismSEC_AUTH_ACTION_LAST;
    int i = 0;
    if ( !name )
        return ret;
    for (i = 0; i < ismSEC_AUTH_ACTION_LAST; i++ ) {
        if ( strcasecmp(name, secActionTypes[i].name) == 0 )
            return secActionTypes[i].type;
    }
    return ret;
}

/* Returns Security validation type */
XAPI int ism_security_getValidationType(int type) {
    if ( type < 0 || type >= ismSEC_AUTH_ACTION_LAST )
        return ismSEC_POLICY_LAST;

    return secActionTypes[type].validationType;
}

/* Returns Security action name */
XAPI char * ism_security_getActionName(int type) {
    if ( type < 0 || type >= ismSEC_AUTH_ACTION_LAST )
        return NULL;

    return secActionTypes[type].name;
}

/* Returning 0 is indication of NULL security context */
XAPI ExpectedMsgRate_t ism_security_context_getExpectedMsgRate(ismSecurity_t *sContext) {
    if (sContext) {
        return sContext->ExpectedMsgRate;
    } else
        return EXPECTEDMSGRATE_UNDEFINED;
}

/* Returning 0 is indication of an error */
XAPI ExpectedMsgRate_t ism_security_context_setExpectedMsgRate(ismSecurity_t *sContext, ExpectedMsgRate_t newRate) {
    if (!sContext) {
        return EXPECTEDMSGRATE_UNDEFINED;
    }

    switch(newRate) {
    case EXPECTEDMSGRATE_LOW:
    case EXPECTEDMSGRATE_DEFAULT:
    case EXPECTEDMSGRATE_HIGH:
    case EXPECTEDMSGRATE_MAX:
        pthread_spin_lock(&sContext->lock);
        sContext->ExpectedMsgRate = newRate;
        pthread_spin_unlock(&sContext->lock);
        break;
    default:
        return EXPECTEDMSGRATE_UNDEFINED;
    }
    return newRate;
}

/* 0 here is not indicative of an error */
XAPI uint32_t ism_security_context_getInflightMsgLimit(ismSecurity_t *sContext) {
    if (sContext) {
        return sContext->InFlightMsgLimit;
    } else
        return 0;
}

/* O indicates an error */
XAPI uint32_t  ism_security_context_setInflightMsgLimit(ismSecurity_t *sContext, uint32_t newLimit) {
    if (sContext && (newLimit > 0)) {
        pthread_spin_lock(&sContext->lock);
        sContext->InFlightMsgLimit = newLimit;
        pthread_spin_unlock(&sContext->lock);
        return newLimit;
    } else
        return 0;
}

/* Return MaxSessionExpiry interval (UINT32_MAX meaning no maximum) */
XAPI uint32_t ism_security_context_getMaxSessionExpiryInterval(ismSecurity_t *sContext) {
    if (sContext) {
        return sContext->MaxSessionExpiry;
    }
    return UINT32_MAX;
}

/* Sets the max clientState expiry interval to use */
XAPI void ism_security_context_setMaxSessionExpiry(ismSecurity_t *sContext, uint32_t maxExpiry) {
    if (sContext) {
        pthread_spin_lock(&sContext->lock);
        sContext->MaxSessionExpiry = maxExpiry;
        pthread_spin_unlock(&sContext->lock);
    }
}

/* Sets the actual clientState expiry interval to use */
XAPI void ism_security_context_setClientStateExpiry(ismSecurity_t *sContext, uint32_t clientStateExpiry) {
    if (sContext) {
        pthread_spin_lock(&sContext->lock);
        sContext->ClientStateExpiry = clientStateExpiry;
        pthread_spin_unlock(&sContext->lock);
    }
}

/* Return the actual clientState expiry interval to use (0 is indication of NULL Security context) */
XAPI uint32_t ism_security_context_getClientStateExpiry(ismSecurity_t *sContext) {
    if (sContext) {
        return sContext->ClientStateExpiry;
    } else
        return 0;
}
