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
#ifndef __ISM_LDAPUTILH_DEFINED
#define __ISM_LDAPUTILH_DEFINED
#include <security.h>
#include <ldap.h>
#include <stdbool.h>

/*Spin lock to update ldap config object*/
extern pthread_spinlock_t ldapconfiglock;

/*LDAP configuration object*/
typedef struct {
    int                       deleted;
    pthread_spinlock_t        lock;
    char                     *name;
    char                     *URL;
    char                     *Certificate;
    ismSecurityVerifyCert_t   CheckServerCert;
    bool                      IgnoreCase;
    char                     *BaseDN;
    char                     *BindDN;
    char                     *BindPassword;
    char                     *UserSuffix;
    char                     *GroupSuffix;
    int                       GroupCacheTTL;
    char                     *UserIdMap;
    char                     *GroupIdMap;
    char                     *GroupMemberIdMap;
    int                       Timeout;
    bool                      EnableCache;
    int                       CacheTTL;
    int                       MaxConnections;
    bool                      Enabled;

    char                      UserIdPrefix[32];
    char                      GroupIdPrefix[32];
    char                      GroupMemberIdPrefix[32];
    char                     *FullCertificate;

    bool                      SearchUserDN;
    bool                      NestedGroupSearch;
    int                       UserDNMaxLen;
    int                       GroupDNMaxLen;
} ismLDAPConfig_t;

#define SECOND_TO_NANOS (1.0 * 1000000000.0)
#define DEFAULT_AUTH_EXPIRE_TIME 30
#define DEFAULT_CACHE_EXPIRE_TIME 300

/**
 * @brief LDAP Result Callback
 */
typedef void (*ismSecurity_LDAPResultCallback_t)(void * ldap_result, void * callbackParam);

typedef struct ismAuthCacheToken_t{
	
	
	char 				lusername[64];
	int 				username_len;
	char 				*username;
	int 				username_alloc_len;
	int					username_inheap;
	ism_time_t			authExpireTime;
	pthread_spinlock_t   lock;
	uint64_t			hash_code; // password hashcode
	
	ism_time_t 								gCacheExpireTime;
	ism_common_list 						gCacheList;
	
}ismAuthCacheToken_t;


/**
 * Initialize LDAP Util
 */
XAPI void ism_security_ldapUtilInit(void);
/**
 * Destroy LDAP Util
 */
XAPI void ism_security_ldapUtilDestroy(void);
/**
 * Validdate if the member belong to group 
 */
XAPI int ism_security_isMemberBelongsToGroup(ismSecurity_t *sContext, char *uid, char *group);

/**
 * Submit an authentication event.
 */
int ism_security_submitLDAPEvent(ismAuthEvent_t * authent)	;

/**
 * Delete the DN from the map 
 * @param cn the common name of an user or group
 * @return 0 for the value can't be found. 1 means that the value is found and freed.
 */
XAPI int ism_security_deleteLDAPDNFromMap(char * cn);

XAPI int ism_security_validateLDAPConfig(ismLDAPConfig_t *ldapobj, int isVerify, int newCert, int tryCount);


#endif
