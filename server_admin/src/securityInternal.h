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

#ifndef __ISM_SECINTERNAL_DEFINED
#define __ISM_SECINTERNAL_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <dirent.h>
#include <ldap.h>
#include <pwd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <selector.h>
#include <transport.h>
#include <ismrc.h>
#include <ismutil.h>
#include <ismjson.h>
#include <config.h>
#include <ldaputil.h>
#include <threadpool.h>
#include <admin.h>
#include <ltpautils.h>

#include "adminInternal.h"
#include "configInternal.h"


#define MAX_ADDR_PAIRS 100
#define MAX_PROTO_FAM  MAX_PROTOCOL
#define PROTOJMS       0
#define PROTOMQTT      1
#define MAXPOL_PEREP   30
#define MAXLDAP_PEREP  30


/* validation ldap objects */
typedef struct {
    ismLDAPConfig_t ** ldapobjs;
    int                id;
    int                count;
    int                nalloc;
    int                slots;
} ismSecLDAPObjects_t;

/* validation rules for a policy */
typedef struct {
    char               *name;
    int                deleted;
    int                type;
    pthread_spinlock_t lock;
    const char         *UserID;
    const char         *GroupID;
    const char         *CommonNames;
    const char         *ClientID;
    const char         *ClientAddress;
    const char         *Protocol;
    const char         *Destination;
    int                destType;
    int                protofam[MAX_PROTOCOL];
    int                protofam_count;
    uint32_t           low4[MAX_ADDR_PAIRS];
    uint32_t           high4[MAX_ADDR_PAIRS];
    int                ipv4Pairs;
    struct in6_addr    low6[MAX_ADDR_PAIRS];
    struct in6_addr    high6[MAX_ADDR_PAIRS];
    int                ipv6Pairs;
    int                varID;
    int                varCL;
    int                varCN;
    int                varGR;
    int                varIDinCL;
    int                varCNinCL;
    int                actions[ismSEC_AUTH_ACTION_LAST];
    void               *context;
    int                validate;
    char *             GroupLDAPEx;
    int                DisconnectedClientNotification;
    int                AllowDurable;
    int                AllowPersistentMessages;
    ExpectedMsgRate_t  ExpMsgRate;
    uint32_t           MaxSessionExpiry;
} ismPolicyRule_t;

/* validation policies */
typedef struct {
    int                type;
    ismPolicyRule_t ** rules;
    int                id;
    int                count;
    int                nalloc;
    int                slots;
    int                noConfPols;
    int                noConnPols;
    int                noTopicPols;
    int                noQueuePols;
    int                noSubsPols;        
} ismSecPolicies_t;

#define SEC_CTX_POLICY_SLOTS 8
/* security context */
struct ismSecurity_t {
    int                 type;
    volatile uint32_t   useCount;
    ism_transport_t *   transport;
    pthread_spinlock_t  lock;
    uint8_t             reValidatePolicy;
    uint8_t             checkGroup;
    uint8_t             policy_inheap;
    uint8_t             resv;
    int                 adminPort;
    ismPolicyRule_t * * conPolicyPtr;
    ismPolicyRule_t * * topicPolicyPtr;
    ismPolicyRule_t * * queuePolicyPtr;
    ismPolicyRule_t * * subsPolicyPtr;
    uint32_t            noConPolicies;
    uint32_t            noTopicPolicies;
    uint32_t            noQueuePolicies;
    uint32_t            noSubsPolicies;
    uint32_t            policy_slot_pos;
    uint32_t            policy_slot_alloc;
    ismPolicyRule_t * * policies;
    ismPolicyRule_t *   policy_slots[SEC_CTX_POLICY_SLOTS];

    ismAuthToken_t      authToken;
    char                username[32];
    char                password[32];

    ismAuthEvent_t      authent;
    char *              authExtras;
    int                 authExtras_len;
    int                 authExtras_inheap;

    ismLTPA_t *         secretKey;
    unsigned long       ltpaTokenExpirationTime;

    unsigned long       oauthTokenExpirationTime;
    char *              oauthGroup;
    char *              url;

    int                 AllowDurable;
    int                 AllowPersistentMessages;

    ExpectedMsgRate_t   ExpectedMsgRate;
    uint32_t            InFlightMsgLimit;

    uint32_t            MaxSessionExpiry;
    uint32_t            ClientStateExpiry;

    int                 superUser;
    int                 allowNullPassword;
};



/* LTPAProfile configuration object */
typedef struct {
    const char *        name;
    const char *        keyfilename;
    ismLTPA_t  *        secretKey;
    int                 deleted;
} ismLTPAProfile_t;

/* LTPA Profile object */
typedef struct {
    ismLTPAProfile_t ** ltpaprofiles;
    int                 id;
    int                 count;
    int                 nalloc;
    int                 slots;
} ismLTPAObjects_t;

/* OAuth URL process methods */
typedef enum {
    ISM_OAUTH_URL_PROCESS_NORMAL          = 0,   /* URL with no query parameters     */
    ISM_OAUTH_URL_PROCESS_QUERY           = 1,   /* URL with query parameters        */
    ISM_OAUTH_URL_PROCESS_QUERY_WITH_AND  = 3,   /* URL with query parameters with & */
    ISM_OAUTH_URL_PROCESS_HEADER          = 4,   /* AuthInfo in header not in URL    */
    ISM_OAUTH_URL_PROCESS_POST            = 5    /* AuthInfo in header and use POST  */
} ism_oauth_url_process_method_e;

/* OAuthProfile configuration object */
typedef struct {
    const char                *        name;
    char                      *    url;
    char                      *    userName;
    char                      *    userPassword;
    char                      *    userInfoUrl;
    char                      *    usernameProp;
    char                      *    keyFileName;
    char                      *    fullKeyFilePath;
    char                      *    tokenNameProp;
    char                      *    groupnameProp;
    char                      *    groupDelimiter;
    int                            isURLsEqual;   //If ResourceURL and UserInfoURL are equal.
    int                            deleted;
    int                            urlProcessMethod;
    ism_oauth_token_send_method_e  tokenSendMethod;
    ismSecurityVerifyCert_t        checkServerCert;
} ismOAuthProfile_t;

/* OAuth Profile object */
typedef struct {
    ismOAuthProfile_t ** oauthprofiles;
    int                 id;
    int                 count;
    int                 nalloc;
    int                 slots;
} ismOAuthObjects_t;


/* variables to handle build order issues */
typedef int32_t (*disconnClientNotificationThread_f)(void);
extern disconnClientNotificationThread_f disconnClientNotificationThread;

typedef ism_secprof_t * (*transGetSecProfile_f)(const char *name);
extern transGetSecProfile_f transGetSecProfile;

/* Private function protypes */
XAPI int ism_security_createPolicyFromProps(ism_prop_t *polprops, int type, char *polname, char *cfgname, int pos );
XAPI int ism_security_getActionType(char *name);
XAPI int ism_security_getValidationType(int type);
XAPI char * ism_security_getActionName(int type);


#ifdef __cplusplus
}
#endif

#endif
