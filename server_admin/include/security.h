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

#ifndef __SECURITY_DEFINED
#define __SECURITY_DEFINED


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#include <ismutil.h>
#include <transport.h>
#include <admin.h>
#include <config.h>


/**
 * @brief Maximum number of policies allowed per security context
 */
#define MAX_POLICIES_PER_SECURITY_CONTEXT 100


/**
 * @brief Security context
 */
typedef struct ismSecurity_t ismSecurity_t;


/**
 * @brief LTPA Key data opaque structure
 */
typedef struct ismLTPA_t ismLTPA_t;


/**
 * @brief Security context object types
 */
typedef enum {
    ismSEC_CONTEXT_CLIENT_ID       = 0, /** Client ID */
    ismSEC_CONTEXT_USERID          = 1, /** User ID */
    ismSEC_CONTEXT_PROTOCOL        = 2, /** Client Protocol name */
    ismSEC_CONTEXT_COMMON_NAME     = 3, /** SSL Certificate distinguished/common names */
    ismSEC_CONTEXT_ADDRESS         = 4, /** Client IP Address as string */
    ismSEC_CONTEXT_TOPIC           = 6, /** Topic */
    ismSEC_CONTEXT_POLICIES        = 7, /** List of policies assigned to a Listener */
    ismSEC_CONTEXT_CONPOLICIES     = 8, /** List of connection policies assigned to a Listener */
    ismSEC_CONTEXT_MSGPOLICIES     = 9, /** List of messaging policies assigned to a Listener */
    ismSEC_CONTEXT_DESTINATION     = 10,/** Destination */
    ismSEC_CONTEXT_LAST            = 11
} ismSecurityContextObjectType_t;

/**
 * @brief Security Authorization Object types
 */
typedef enum {
    ismSEC_AUTH_TOPIC           = 0, /** Topic name        */
    ismSEC_AUTH_QUEUE           = 1, /** Queue name        */
    ismSEC_AUTH_USER            = 2, /** User  name        */
    ismSEC_AUTH_SUBSCRIPTION    = 3, /** Subscription name */
    ismSEC_AUTH_LAST            = 4
} ismSecurityAuthObjectType_t;

/**
 * @brief Security policy types
 */
typedef enum {
    ismSEC_POLICY_CONFIG        = 0, /** Configuration policy */
    ismSEC_POLICY_CONNECTION    = 1, /** Connection Policy */
    ismSEC_POLICY_MESSAGING     = 2, /** Messaging action policy - Deprecated */
    ismSEC_POLICY_TOPIC         = 3, /** Messaging policy for Topic */
    ismSEC_POLICY_QUEUE         = 4, /** Messaging policy for Queue */
    ismSEC_POLICY_SUBSCRIPTION  = 5, /** Messaging policy for Subscription */
    ismSEC_POLICY_LAST          = 6
} ismSecurityPolicyType_t;

/**
 * @brief Security policy validation (authorization) action types
 */
typedef enum {
    /* admin actions */
    ismSEC_AUTH_ACTION_GET          = 0,  /** get config data */
    ismSEC_AUTH_ACTION_SET          = 1,  /** set config data */
    ismSEC_AUTH_ACTION_START        = 2,  /** start server */
    ismSEC_AUTH_ACTION_STOP         = 3,  /** stop server */
    ismSEC_AUTH_ACTION_BACKUP       = 4,  /** backup */
    ismSEC_AUTH_ACTION_RESTORE      = 5,  /** restore */
    ismSEC_AUTH_ACTION_DIAG         = 6,  /** tracedump, coredump */
    ismSEC_AUTH_ACTION_APPLY        = 7,  /** apply */
    ismSEC_AUTH_ACTION_IMPORT       = 8,  /** import */
    ismSEC_AUTH_ACTION_EXPORT       = 9,  /** export */
    ismSEC_AUTH_ACTION_STATUS       = 10, /** server status */
    ismSEC_AUTH_ACTION_VERSION      = 11, /** server version */

    /* connection action */
    ismSEC_AUTH_ACTION_CONNECT      = 12, /** connect */

    /* messaging actions */
    ismSEC_AUTH_ACTION_SEND         = 13, /** send - Queue */
    ismSEC_AUTH_ACTION_RECEIVE      = 14, /** receive - Queue */
    ismSEC_AUTH_ACTION_BROWSE       = 15, /** browse - Queue */
    ismSEC_AUTH_ACTION_PUBLISH      = 16, /** publish - Topic */
    ismSEC_AUTH_ACTION_SUBSCRIBE    = 17, /** subscribe - Topic */
    ismSEC_AUTH_ACTION_CONTROL      = 18, /** control - Subscription */

    /* Configuration Policy actions */
    ismSEC_AUTH_ACTION_CONFIGURE    = 19, /** monitor server */
    ismSEC_AUTH_ACTION_VIEW         = 20, /** monitor server */
    ismSEC_AUTH_ACTION_MONITOR      = 21, /** monitor server */
    ismSEC_AUTH_ACTION_MANAGE       = 22, /** monitor server */

    ismSEC_AUTH_ACTION_LAST         = 23
} ismSecurityAuthActionType_t;


/**
 * @brief Configuration policy authorization types
 */
typedef enum {
    ismSEC_CONFIG_AUTH_CONFIGURE = 0,    /** Authorized to configure */
    ismSEC_CONFIG_AUTH_VIEW      = 1,    /** Authorized to view      */
    ismSEC_CONFIG_AUTH_MONITOR   = 2,    /** Authorized to monitor   */
    ismSEC_CONFIG_AUTH_MANAGE    = 3,    /** Authorized to manage    */
    ismSEC_CONFIG_AUTH_NONE      = 4
} ismSecurityConfigAuthType_t;

/**
 * @brief LDAP and OAuth Server Certificate Verification options
 */
typedef enum {
    ismSEC_SERVER_CERT_DISABLE_VERIFY       = 0, /** Server Certificate is not verified     */
    ismSEC_SERVER_CERT_TRUST_STORE          = 1, /** Verify using user provided certificate */
    ismSEC_SERVER_CERT_PUBLIC_TRUST         = 2, /** Verify using publicly trusted root CAs */
    ismSEC_SERVER_CERT_LAST                 = 3
} ismSecurityVerifyCert_t;


/**
 * Admin/Monitoring action authorization lookup
 */
static const int configuration_policy_action_lookup[] = {
    [ISM_ADMIN_SET]                       = ismSEC_AUTH_ACTION_CONFIGURE,     /** Set                               */
    [ISM_ADMIN_GET]                       = ismSEC_AUTH_ACTION_VIEW,          /** Get                               */
    [ISM_ADMIN_STOP]                      = ismSEC_AUTH_ACTION_MANAGE,        /** Stop                              */
    [ISM_ADMIN_APPLY]                     = ismSEC_AUTH_ACTION_CONFIGURE,     /** Apply                             */
    [ISM_ADMIN_IMPORT]                    = ismSEC_AUTH_ACTION_MANAGE,        /** Import                            */
    [ISM_ADMIN_EXPORT]                    = ismSEC_AUTH_ACTION_MANAGE,        /** Export                            */
    [ISM_ADMIN_TRACE]                     = ismSEC_AUTH_ACTION_CONFIGURE,     /** Trace                             */
    [ISM_ADMIN_VERSION]                   = ismSEC_AUTH_ACTION_VIEW,          /** Version                           */
    [ISM_ADMIN_STATUS]                    = ismSEC_AUTH_ACTION_VIEW,          /** Status                            */
    [ISM_ADMIN_SETMODE]                   = ismSEC_AUTH_ACTION_CONFIGURE,     /** Admin Mode                        */
    [ISM_ADMIN_HAROLE]                    = ismSEC_AUTH_ACTION_VIEW,          /** HA Node Role                      */
    [ISM_ADMIN_CERTSYNC]                  = ismSEC_AUTH_ACTION_CONFIGURE,     /** Synchronize certificates          */
    [ISM_ADMIN_TEST]                      = ismSEC_AUTH_ACTION_CONFIGURE,     /** Test connection                   */
    [ISM_ADMIN_PURGE]                     = ismSEC_AUTH_ACTION_CONFIGURE,     /** Purge - store/log                 */
    [ISM_ADMIN_ROLLBACK]                  = ismSEC_AUTH_ACTION_MANAGE,        /** Rollback Transaction              */
    [ISM_ADMIN_COMMIT]                    = ismSEC_AUTH_ACTION_CONFIGURE,     /** Commit Transaction                */
    [ISM_ADMIN_FORGET]                    = ismSEC_AUTH_ACTION_CONFIGURE,     /** Forget Transaction                */
    [ISM_ADMIN_SYNCFILE]                  = ismSEC_AUTH_ACTION_CONFIGURE,     /** Sync file with Standby            */
    [ISM_ADMIN_CLOSE]                     = ismSEC_AUTH_ACTION_CONFIGURE,     /** Close action                      */
    [ISM_ADMIN_NOTIFY]                    = ismSEC_AUTH_ACTION_CONFIGURE,     /** Notification from other component */
    [ISM_ADMIN_RUNMSSHELLSCRIPT]          = ismSEC_AUTH_ACTION_CONFIGURE,     /** Run a script                      */
    [ISM_ADMIN_CREATE]                    = ismSEC_AUTH_ACTION_CONFIGURE,     /** Create object configuration       */
    [ISM_ADMIN_UPDATE]                    = ismSEC_AUTH_ACTION_CONFIGURE,     /** Update object configuration       */
    [ISM_ADMIN_DELETE]                    = ismSEC_AUTH_ACTION_CONFIGURE,     /** Delete object configuration       */
    [ISM_ADMIN_RESTART]                   = ismSEC_AUTH_ACTION_MANAGE,        /** Restart server or component       */
    [ISM_ADMIN_MONITOR]                   = ismSEC_AUTH_ACTION_MONITOR,       /** Monitoring requests - statistics  */
    [ISM_ADMIN_LAST]                      = ismSEC_AUTH_ACTION_LAST
};


typedef struct security_stat_t {
    uint64_t  authen_passed;
    uint64_t  authen_failed;
    uint64_t  author_conn_passed;
    uint64_t  author_conn_failed;
    uint64_t  author_msgn_passed;
    uint64_t  author_msgn_failed;
#ifdef AUTH_TIME_STAT_ENABLED
    double    author_total_time;
#endif
} security_stat_t;


/**
 * @brief  Create Security context
 *
 * Called by Transport to create initial security context.
 *
 * The security context is passed to other components (protocol, engine) APIs
 * for authentication and policy validation (authorization).
 *
 * @param[in]    type         Type of security context
 * @param[in]    transport    Pointer to transport structure
 * @param[out]   newSContext  The new Security context that has been created.
 *
 * Returns ISMRC_Success on success or ISM_RC_*
 *
 */
XAPI int32_t ism_security_create_context(ismSecurityPolicyType_t type, ism_transport_t * transport, ismSecurity_t **newSContext);

/**
 * @brief  Returns UserID associated with the Security context
 *
 * Called by Engine to get UserID.
 *
 * @param[in]   sContext     Created security context
 *
 * Returns a userid on success or NULL
 *
 */
XAPI char * ism_security_context_getUserID(ismSecurity_t *sContext);

/**
 * @brief  Returns TrcLevel associated with the Security context or the default
 *
 * Called by Engine to get TrcLevel
 *
 * @param[in]   sContext     Created security context
 *
 * Returns an ism_trclevel_t on success or NULL
 *
 */
XAPI ism_trclevel_t *ism_security_context_getTrcLevel(ismSecurity_t *sContext);

/**
 * @brief  Destroy Security context
 *
 * Called by Transport when client connection is closed.
 *
 * @param[in]   sContext     Currently created security context
 *
 * Returns ISMRC_Success on success or ISM_RC_*
 */
XAPI int32_t ism_security_destroy_context(ismSecurity_t *newSContext);


/**
 * @brief  Validate Policy
 *
 * ISM supports policy based authorization. The policies are created using WebUI or CLI based on one or
 * more of the following attributes associated with a client:
 * - Client ID
 * - Connection related data like client IP, domain name
 * - SSL certificate Distinguished/common names
 * - Group the user belongs to
 * - Topic or policy name
 * This API is called by Transport, protocol and engine to authorize a client/connection/user to take
 * an action on a specified object. Basically there are two types of authorization:
 * - Connection level
 * - Messaging level (on Topic/Queue)
 * Connection level authorization can be controlled only with ismSecurityAuthActionType. No object name
 * is required because required objects data is available in security context and
 * this API can use this data to validate connection level policies set by the users.
 * However for messaging level authorization, a topic/queue name is required.  *
 *
 * @param[in]    sContext     Currently created security context
 * @param[in]    objectType   Object type
 * @param[in]    objectName   Object name
 * @param[in]    actionType   Action type
 * @param[in]    compType     Component type
 * @param[out]   context      Stored context for requesting component.
 *
 * Returns ISMRC_Success on success or ISM_RC_*
 */
XAPI int32_t ism_security_validate_policy(ismSecurity_t *secContext, ismSecurityAuthObjectType_t objectType, const char * objectName, ismSecurityAuthActionType_t actionType, ism_ConfigComponentType_t compType, void ** context);



/**
 * @brief  Authenticate user
 *
 * Called by Transport and/or protocol to authenticate user.
 * A ISM/MQTT client has two options to specify username and password.
 * - Initial handshake e.g. connecting from a web browser
 *   - For such cases this API will be called from Transport
 * - Set username/assword in MQTT or JMS connection object.
 *   - For such cases this API will be called from Protocol.
 * An error code is returned to the caller, if the user can not be authenticated
 * successfully.
 *
 * @param[in]    username     User name
 * @param[in]    password     Password
 *
 *
 * Returns ISMRC_Success on success or ISM_RC_*
 */
XAPI int32_t ism_security_authenticate_user(ismSecurity_t *sContext, const char *username, int username_len, const char *password, int password_len);


/**
 * @brief  Add a Policy
 *
 * Add a policy to the configured policy list. A policy can be added from a JSON file or
 * a JSON string.
 *
 * @param[in]    policyFileName     Policy file name
 * @param[in]    polstr             Policy string
 *
 * Returns ISMRC_Success on success or ISM_RC_*
 */
XAPI int32_t ism_security_addPolicy(const char *policyFileName, char *polstr);


/**
 * @brief  Initialize security service
 *
 * Returns ISMRC_Success on success or ISM_RC_*
 */
XAPI int32_t ism_security_init(void);

/**
 * @brief Initialize authentication objects
 *
 * Returns ISMRC_Success on success or ISM_RC_*
 */
XAPI int ism_authentication_init(void);


/**
 * @brief  Terminate security service
 *
 * Returns ISMRC_Success on success or ISM_RC_*
 */
XAPI int32_t ism_security_term(void);


/**
 * @brief Authentication Callback
 */
typedef void (*ismSecurity_AuthenticationCallback_t)(int rc, void * callbackParam);

/*Default DN length*/
#define DN_DEFAULT_LEN 64

/**
 * Security Authentication Token
 */
 typedef enum ismAuthenticationStatus_t {
    AUTH_STATUS_INACTIVE     = 0,
    AUTH_STATUS_IN_Q         = 1,
    AUTH_STATUS_IN_PROGRESS  = 2,
    AUTH_STATUS_IN_CALLBACK  = 3,
    AUTH_STATUS_COMPLETED    = 4,
    AUTH_STATUS_CANCELLED    = 5,

} ismAuthenticationStatus_t;

/* Where/how  we put the token in the http request */
typedef enum {
    ISM_OAUTH_TOKENSEND_URL_PARAM   = 0,  /* Include the token in the URL params of GET Method    */
    ISM_OAUTH_TOKENSEND_HTTP_HEADER = 1,  /* Include the token in the HTTP headers of GET Method  */
    ISM_OAUTH_TOKENSEND_HTTP_POST   = 2   /* Include the token in the HTTP headers of POST Method */
} ism_oauth_token_send_method_e;


struct ismAuthToken {
    int                       msgid;
    int                       username_len;
    char *                    username;
    int                       username_alloc_len;
    int                       username_inheap;
    char *                    password;
    int                       password_len;
    int                       password_alloc_len;
    int                       password_inheap;
    volatile short            isCancelled;
    pthread_spinlock_t        lock;
    ismAuthenticationStatus_t status;
    int                       inited;
    int                       isAuthenticated;
    uint64_t                  hash_code; // password hashcode

    ism_time_t                authExpireTime;
    ism_time_t                gCacheExpireTime;
    ism_common_list           gCacheList;

    ismSecurity_AuthenticationCallback_t     pCallbackFn;
    void  *                   pContext;
    int                       pContext_size;
    int                       pContext_inheap;

    ismSecurity_t             *sContext;
    char                      userDN[DN_DEFAULT_LEN];
    char *                    userDNPtr;
    int                       userDN_inheap;

    /*OAuth Properties*/
    char *                          oauth_url;
    char *                          oauth_userName;
    char *                          oauth_userPassword;
    char *                          oauth_userInfoUrl;
    char *                          oauth_usernameProp;
    char *                          oauth_keyFileName;
    char *                          oauth_fullKeyFilePath;
    char *                          oauth_tokenNameProp;
    int                             oauth_isURLsEqual;   //If ResourceURL and UserInfoURL are equal.
    int                             oauth_usePost;
    char *                          oauth_groupnameProp;
    char *                          oauth_groupDelimiter;
    int                             oauth_urlProcessMethod;
    ism_oauth_token_send_method_e   oauth_tokenSendMethod;
    int                             oauth_securedConnection;
    ismSecurityVerifyCert_t         oauth_checkServerCert;

};

typedef struct ismAuthToken ismAuthToken_t;



typedef enum ismSecurityLDAPEventType_t {
    SECURITY_LDAP_AUTH_EVENT  = 0,
    SECURITY_LDAP_GROUP_SEARCH_EVENT = 1
} ismSecurityEventType_t;


typedef struct ismAuthEvent_t {
    struct ismAuthEvent_t * next;
    ismAuthToken_t *        token;
    ismSecurityEventType_t  type;
    int                     authnRequired;
    int                     ltpaAuth;
    int                     ltpaTokenSet;
    int                     oauth;
} ismAuthEvent_t;

/**
 * Security Authentication.
 * The token object will be destroyed once the authentication process is completed
 * @param username            user name
 * @param username_len        user name length
 * @param password            password
 * @param password_len        password length
 * @param pContext            Context for the callback
 * @param pContext_size       size of pContext
 * @param pCallbackFn         function pointer of the authentication callback
 * @param authnRequired       is authentication required
 * @return an authentication handle
 */
XAPI void ism_security_authenticate_user_async( ismSecurity_t *sContext,
                                                const char *username, int username_len,
                                                const char *password, int password_len,
                                                void *pContext,int pContext_size,
                                                ismSecurity_AuthenticationCallback_t pCallbackFn,
                                                int authnRequired,
                                                ism_time_t delay);

/**
 * Cancel the authentication process for this token.
 * @param token the authentication token.
 */
XAPI void ism_security_cancelAuthentication(void * handle);

XAPI void ism_security_returnAuthHandle(void * handle);

XAPI security_stat_t * ism_security_getStat(void);

XAPI int ism_security_ltpaReadKeyfile(char *keyfile_path, char *keyfile_password, ismLTPA_t **ltpaKey);

XAPI int ism_security_ltpaDeleteKey(ismLTPA_t *key);

XAPI int ism_security_ltpaV1DecodeToken(ismLTPA_t *keys, char *token_data, size_t token_len, char **user_name,
                                        char **realm, time_t *expiration);

XAPI int ism_security_ltpaV2DecodeToken(ismLTPA_t *keys, char *token_data, size_t token_len, char **user_name,
                                        char **realm, time_t *expiration);

XAPI int32_t ism_security_setLTPAToken(
        ism_transport_t * transport,ismSecurity_t *sContext, char * ltpaToken, int ltpaToken_len);

/**
 * Decrease the useCount of the Security Context
 * @param sContext Security Context
 * @return the use count after the decrement.
 */
XAPI uint32_t ism_security_context_use_decrement(ismSecurity_t *sContext) ;

/**
 * Increase the useCount of the Security Context
 * @param sContext Security Context
 * @return the use count after the decrement.
 */
XAPI uint32_t ism_security_context_use_increment(ismSecurity_t *sContext) ;


XAPI int ism_security_updatePoliciesProtocol(void);

/**
 * @brief  Sets context to be returned by ism_security_validate_policy()
 *
 * Stores some opaque data to be retrieved when a policy has been used
 * to authorise an action
 *
 * @param[in]     name                   name to set the context for
 * @param[in]     policyType             type of policy
 * @param[in]     compType               Component type
 * @param[in]     newContext             Context to store
 *
 * NOTE: At the moment compType is asserted to be ISM_CONFIG_COMP_ENGINE
 * only.
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI int32_t ism_security_set_policyContext(const char *name, ismSecurityPolicyType_t policyType, ism_ConfigComponentType_t compType, void *newContext);

/**
 * @brief  Get context of a policy
 *
 * @param[in]     name                   name to set the context for
 * @param[in]     compType               Component type
 *
 * NOTE: At the moment compType is asserted to be ISM_CONFIG_COMP_ENGINE
 * only.
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI void *ism_security_get_policyContextByName(char *name, ism_ConfigComponentType_t compType);

XAPI int ism_security_context_getAllowDurable(ismSecurity_t *sContext);

XAPI int ism_security_context_getAllowPersistentMessages(ismSecurity_t *sContext);

XAPI int ism_security_context_isLTPA(ismSecurity_t *sContext);

XAPI int ism_security_context_isOAuth(ismSecurity_t *sContext);

XAPI ExpectedMsgRate_t ism_security_context_getExpectedMsgRate(ismSecurity_t *sContext);

XAPI ExpectedMsgRate_t ism_security_context_setExpectedMsgRate(ismSecurity_t *sContext, ExpectedMsgRate_t newRate);

XAPI uint32_t ism_security_context_getInflightMsgLimit(ismSecurity_t *sContext);

XAPI uint32_t  ism_security_context_setInflightMsgLimit(ismSecurity_t *sContext, uint32_t newLimit);

/**
 * @brief  Get the maximum session expiry interval in place for this security context
 *
 * @param[in]     sContext               Security context
 *
 * @remark When a security context is created, this field is initalized with the value
 * pulled from the connection policy.
 *
 * Note that the value is *not* updated if the connection policy is updated, it keeps
 * the value that was in place when the security context was first created.
 *
 * @return UINT32_MAX if no maximum is in place, or a value from 1 to (UINT32_MAX-1) for
 * the maximum number of seconds to allow for session expiry.
 */
XAPI uint32_t ism_security_context_getMaxSessionExpiryInterval(ismSecurity_t *sContext);

/**
 * @brief  Set the actual clientState expiry to use into the security context
 *
 * @param[in]     sContext               Security context
 * @param[in]     clientStateExpiry      Expiry interval in seconds
 *
 * @remark When a clientState is being created, if it is durable we want to be able to say
 * how long after disconnecting the clientState should be remembered. A value of UINT32_MAX
 * means that the session should not expire, otherwise it is the number of seconds after a
 * client disconnects when the clientState is discarded.
 *
 * This is used for MQTT session expiry.
 */
XAPI void ism_security_context_setClientStateExpiry(ismSecurity_t *sContext, uint32_t clientStateExpiry);

/**
 * @brief  Get the actual clientState expiry from the security context
 *
 * @param[in]     sContext               Security context
 *
 * @remark The clientStateExpiry value in the security context defaults to UINT32_MAX meaning
 * that the clientState should not expire. It can be overridden with a call to
 * ism_security_setClientStateExpiry.
 *
 * @return The current clientStateExpiry value from the specified security context.
 *
 * @see ism_security_context_setClientStateExpiry
 */
XAPI uint32_t ism_security_context_getClientStateExpiry(ismSecurity_t *sContext);

#ifdef __cplusplus
}
#endif

#endif


