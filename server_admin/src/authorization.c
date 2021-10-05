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

#include "securityInternal.h"
#include "validateInternal.h"
#include <security.h>


/* Suppress the warning about an empty gnu_printf format string caused by a LOG message with no inserts */
#pragma GCC diagnostic ignored "-Wformat-zero-length"

extern void ism_security_setLDAPConfig(ismLDAPConfig_t * ldapConfig);
extern int ism_security_getLDAPIdPrefix(char * idMap, char *idPrefix);
extern void ism_security_initAuthToken(ismAuthToken_t * authToken);
extern void  ism_security_destroyAuthToken(ismAuthToken_t * token);
extern int32_t ism_config_deleteOAuthKeyFile(char * name);
extern ism_trclevel_t * ism_defaultTrace;
extern transGetSecProfile_f transGetSecProfile;
extern ismLDAPConfig_t *    _localLdapConfig;
extern int userIDMapHasStar;
extern int groupIDMapHasStar;
extern int criticalConfigError;
extern int32_t auditLogControl;
extern pthread_rwlock_t policylock;
extern ismSecPolicies_t *policies;
extern ismSecLDAPObjects_t *ldapobjects;

int doAuthorize = 1;

//static const char *policyDir = NULL;
ism_config_t * conhandle = NULL;

/* Spin lock to update LTPA config object */
extern pthread_spinlock_t ltpaconfiglock;

extern ismLTPAObjects_t *ltpaobjects;

static ismLTPAProfile_t *getLTPAProfileByName(const char *name);


/* Spin lock to update OAuth config object */
extern pthread_spinlock_t oauthconfiglock;

extern ismOAuthObjects_t *oauthobjects;

static ismOAuthProfile_t *getOAuthProfileByName(const char *name);

extern void dn_normalize(char * dn);
extern char * tolowerDN(const char * dn);
extern void tolowerDN_ASCII(char * dn);
/**
 * This callback will be invoked by the Transport component when the
 * connection is expired.
 */
int ism_security_connectionExpiredCallback(ism_transport_t * transport)
{
    if(transport!=NULL)
    {
        /* TODO:
         * For now, just close the connection.
         * Need to look into a policy to determine whether to disconnect or not
         */

        transport->expireTime = 0;
        transport->close(transport, ISMRC_ConnectionExpired, 0, "Connection expired");

        return 0;
    }

    return 1;
}


static void freeLDAPConfig(ismLDAPConfig_t *ldapconfig) {
    if ( !ldapconfig )
        return;

    pthread_spin_lock(&ldapconfig->lock);
    if ( ldapconfig->name ) ism_common_free(ism_memory_admin_misc,(void *)ldapconfig->name);
    if ( ldapconfig->URL ) ism_common_free(ism_memory_admin_misc,(void *)ldapconfig->URL);
    if ( ldapconfig->Certificate ) ism_common_free(ism_memory_admin_misc,(void *)ldapconfig->Certificate);
    if ( ldapconfig->BaseDN ) ism_common_free(ism_memory_utils_to_lower,(void *)ldapconfig->BaseDN);
    if ( ldapconfig->BindDN ) ism_common_free(ism_memory_utils_to_lower,(void *)ldapconfig->BindDN);
    if ( ldapconfig->BindPassword ) ism_common_free(ism_memory_admin_misc,(void *)ldapconfig->BindPassword);
    if ( ldapconfig->UserSuffix ) ism_common_free(ism_memory_utils_to_lower,(void *)ldapconfig->UserSuffix);
    if ( ldapconfig->GroupSuffix ) ism_common_free(ism_memory_utils_to_lower,(void *)ldapconfig->GroupSuffix);
    if ( ldapconfig->UserIdMap ) ism_common_free(ism_memory_admin_misc,(void *)ldapconfig->UserIdMap);
    if ( ldapconfig->GroupIdMap ) ism_common_free(ism_memory_admin_misc,(void *)ldapconfig->GroupIdMap);
    if ( ldapconfig->GroupMemberIdMap ) ism_common_free(ism_memory_admin_misc,(void *)ldapconfig->GroupMemberIdMap);
    if ( ldapconfig->FullCertificate ) ism_common_free(ism_memory_admin_misc,(void *)ldapconfig->FullCertificate);

    pthread_spin_unlock(&ldapconfig->lock);
    pthread_spin_destroy(&ldapconfig->lock);
    ism_common_free(ism_memory_admin_misc,ldapconfig);
    ldapconfig = NULL;
}

/* process protocol family */
static void processProtoFamily(ismPolicyRule_t *policy) {
   int i;
   int protocount=0;

        for (i=0; i<MAX_PROTO_FAM; i++){
                  policy->protofam[i] = 0;
                  policy->protofam_count=0;
        }

        if ( !policy->Protocol || (policy->Protocol && *policy->Protocol == '\0') ) {
                return;
        }

        int len = strlen(policy->Protocol);
        if(len==1 && *policy->Protocol=='*'){
                /*Protocol filter is all. So no need to set*/
                return;
        }

        char *tmpstr = (char *)alloca(len+1);
        memcpy(tmpstr, policy->Protocol, len);
        tmpstr[len] = 0;

        char *token, *nexttoken = NULL;
        for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL; token
                = strtok_r(NULL, ",", &nexttoken)) {
                int protoid =  ism_admin_getProtocolId(token);
                policy->protofam[protocount++] = protoid;
                TRACE(9, "Set Protocol family rule (%s). ID=%d\n",
                                policy->Protocol, protoid);

        }
        policy->protofam_count=protocount;
        TRACE(9, "Protocol family rule count=%d\n", policy->protofam_count);

    return;
}

static int addLDAPObject(ismLDAPConfig_t * ldapobj) {
    int i;

    if (ldapobjects->count == ldapobjects->nalloc) {
        ismLDAPConfig_t ** tmp = NULL;
        int firstSlot = ldapobjects->nalloc;
        ldapobjects->nalloc = ldapobjects->nalloc == 0 ? 64 : ldapobjects->nalloc * 2;
        tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_admin_misc,423),ldapobjects->ldapobjs, sizeof(ismLDAPConfig_t *) * ldapobjects->nalloc);
        if (tmp == NULL) {
            return ISMRC_AllocateError;
        }
        ldapobjects->ldapobjs = tmp;
        for (i = firstSlot; i < ldapobjects->nalloc; i++)
            ldapobjects->ldapobjs[i] = NULL;
        ldapobjects->slots = ldapobjects->count;
    }
    if (ldapobjects->count == ldapobjects->slots) {
        ldapobjects->ldapobjs[ldapobjects->count] = ldapobj;
        ldapobjects->id = ldapobjects->count;
        ldapobjects->count++;
        ldapobjects->slots++;
    } else {
        for (i = 0; i < ldapobjects->slots; i++) {
            if (!ldapobjects->ldapobjs[i]) {
                ldapobjects->ldapobjs[i] = ldapobj;
                ldapobjects->id = i;
                ldapobjects->count++;
                break;
            }
        }
    }



    return ISMRC_OK;
}

static void updateLDAPDNMaxLen(ismLDAPConfig_t * ldapconfig)
{
    /*Calculate the length of suffix + idMap for User and Group*/
    int uidmapLen = ldapconfig->UserIdMap?strlen(ldapconfig->UserIdMap):0;
    int gidmaplen = ldapconfig->GroupIdMap?strlen(ldapconfig->GroupIdMap):0;
    if (ldapconfig->UserSuffix)  ldapconfig->UserDNMaxLen = strlen(ldapconfig->UserSuffix) + uidmapLen + 2 ;
    if (ldapconfig->GroupSuffix) ldapconfig->GroupDNMaxLen = strlen(ldapconfig->GroupSuffix) + gidmaplen +2  ;
}

/* Update or add a LDAP object in the list */
static int updateLDAPConfig(ismLDAPConfig_t * ldapconfig) {
    int i;
    int rc = ISMRC_OK;
    if (!ldapconfig || !ldapconfig->name){
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "ldapconfig");
        return ISMRC_ArgNotValid;
    }

    TRACE(9, "Process LDAP %s to add/update.\n", ldapconfig->name);

    /* if rule exist in the list - update else add policy in the list */
    pthread_spin_lock(&ldapconfiglock);
    int found = 0;
    for (i = 0; i < ldapobjects->count; i++) {
        ismLDAPConfig_t *ldapobj = ldapobjects->ldapobjs[i];
        if (strcmp(ldapconfig->name, ldapobj->name) == 0) {
            TRACE(9, "Updating LDAP %s\n", ldapconfig->name);
            found = 1;

            /* update policy fields */

            ldapobj->IgnoreCase = ldapconfig->IgnoreCase;
            ldapobj->SearchUserDN = ldapconfig->SearchUserDN;
            ldapobj->NestedGroupSearch = ldapconfig->NestedGroupSearch;
            ldapobj->GroupCacheTTL = ldapconfig->GroupCacheTTL;
            ldapobj->Timeout = ldapconfig->Timeout;
            ldapobj->EnableCache = ldapconfig->EnableCache;
            ldapobj->CacheTTL = ldapconfig->CacheTTL;
            ldapobj->MaxConnections = ldapconfig->MaxConnections;
            ldapobj->Enabled = ldapconfig->Enabled;
            ldapobj->CheckServerCert = ldapconfig->CheckServerCert;
            if(ldapconfig->URL){
                if(ldapobj->URL) ism_common_free(ism_memory_admin_misc,ldapobj->URL);
                ldapobj->URL = ldapconfig->URL;
            }
            if(ldapconfig->Certificate){
                if(ldapobj->Certificate) ism_common_free(ism_memory_admin_misc,ldapobj->Certificate);
                ldapobj->Certificate = ldapconfig->Certificate;
            }
            if(ldapconfig->BaseDN){
                if(ldapobj->BaseDN) ism_common_free(ism_memory_utils_to_lower,ldapobj->BaseDN);
                ldapobj->BaseDN = ldapconfig->BaseDN;
            }
            if(ldapconfig->BindDN){
                if(ldapobj->BindDN) ism_common_free(ism_memory_utils_to_lower,ldapobj->BindDN);
                ldapobj->BindDN = ldapconfig->BindDN;
            }
            if(ldapconfig->BindPassword){
                if(ldapobj->BindPassword) ism_common_free(ism_memory_admin_misc,ldapobj->BindPassword);
                ldapobj->BindPassword = ism_security_decryptAdminUserPasswd(ldapconfig->BindPassword);
            }
            if(ldapconfig->UserSuffix){
                if(ldapobj->UserSuffix) ism_common_free(ism_memory_utils_to_lower,ldapobj->UserSuffix);
                ldapobj->UserSuffix = ldapconfig->UserSuffix;
            }
            if(ldapconfig->GroupSuffix){
                if(ldapobj->GroupSuffix) ism_common_free(ism_memory_utils_to_lower,ldapobj->GroupSuffix);
                ldapobj->GroupSuffix = ldapconfig->GroupSuffix;
            }
            if(ldapconfig->UserIdMap){
                char *uPtr;
                char *tmpstr;
                char *val;

                if(ldapobj->UserIdMap) ism_common_free(ism_memory_admin_misc,ldapobj->UserIdMap);
                ldapobj->UserIdMap = ldapconfig->UserIdMap;

                /* check for * in UserIdMap and GroupIdMap */
                uPtr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ldapobj->UserIdMap);
                tmpstr = uPtr;
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
            if(ldapconfig->GroupIdMap){
                char *gPtr;
                char *tmpstr;
                char *val;

                if(ldapobj->GroupIdMap) ism_common_free(ism_memory_admin_misc,ldapobj->GroupIdMap);
                ldapobj->GroupIdMap = ldapconfig->GroupIdMap;

                gPtr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ldapobj->GroupIdMap);
                tmpstr = gPtr;
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
            if(ldapconfig->GroupMemberIdMap){
                if(ldapobj->GroupMemberIdMap) ism_common_free(ism_memory_admin_misc,ldapobj->GroupMemberIdMap);
                ldapobj->GroupMemberIdMap = ldapconfig->GroupMemberIdMap;
            }

            ism_security_getLDAPIdPrefix(ldapobj->UserIdMap, ldapobj->UserIdPrefix);
            ism_security_getLDAPIdPrefix(ldapobj->GroupIdMap, ldapobj->GroupIdPrefix);
            ism_security_getLDAPIdPrefix(ldapobj->GroupMemberIdMap, ldapobj->GroupMemberIdPrefix);
            updateLDAPDNMaxLen(ldapconfig);
            TRACE(8, "LDAP %s is updated\n", ldapobj->name);
            break;
        }
    }
    if (found == 0) {
        ism_security_getLDAPIdPrefix(ldapconfig->UserIdMap, ldapconfig->UserIdPrefix);
        ism_security_getLDAPIdPrefix(ldapconfig->GroupIdMap, ldapconfig->GroupIdPrefix);
        ism_security_getLDAPIdPrefix(ldapconfig->GroupMemberIdMap, ldapconfig->GroupMemberIdPrefix);
        updateLDAPDNMaxLen(ldapconfig);

        rc = addLDAPObject(ldapconfig);
        if ( rc == ISMRC_OK )
            TRACE(8, "LDAP %s is added\n", ldapconfig->name);
    }
    pthread_spin_unlock(&ldapconfiglock);

    pthread_spin_lock(&ldapconfig->lock);
    if(ldapconfig->deleted==false && ldapconfig->Enabled)
    {
        ism_security_setLDAPConfig(ldapconfig);
    }else{
        if(_localLdapConfig!=NULL)
            ism_security_setLDAPConfig(_localLdapConfig);
    }
    pthread_spin_unlock(&ldapconfig->lock);

    return rc;
}


static ismLDAPConfig_t *getLDAPByName(char *name) {
    int i = 0;
    for (i = 0; i < ldapobjects->count; i++) {
        ismLDAPConfig_t *ldapobj = ldapobjects->ldapobjs[i];
        if (name && ldapobj->name && strcmp(ldapobj->name, name) == 0) {
            return ldapobj;
        }
    }
    return NULL;
}

/**
 * Get the first Enabled LDAP configuration
 */
ismLDAPConfig_t *ism_security_getEnabledLDAPConfig(void) {
    int i = 0;
    for (i = 0; i < ldapobjects->count; i++) {
        ismLDAPConfig_t *ldapobj = ldapobjects->ldapobjs[i];
        if (ldapobj->Enabled == true&&ldapobj->deleted==false) {
            return ldapobj;
        }
    }
    return NULL;
}


/* create props for one ldap */
static ism_prop_t * createLDAPProp(ism_prop_t *props, char *ldapname, char *cfgname, int pos) {
    char *value;
    ism_field_t f;
    int found = 0;
    ism_prop_t *newProps = ism_common_newProperties(24);


    sprintf(cfgname + pos, "Name.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Name", &f);
        found = 1;
    }
    sprintf(cfgname + pos, "URL.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "URL", &f);
        found = 1;
    }
    sprintf(cfgname + pos, "Certificate.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Certificate", &f);
        found = 1;
    }
    sprintf(cfgname + pos, "CheckServerCert.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "CheckServerCert", &f);
        found = 1;
    }
    sprintf(cfgname + pos, "IgnoreCase.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "IgnoreCase", &f);
        found = 1;
    }
    sprintf(cfgname + pos, "NestedGroupSearch.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "NestedGroupSearch", &f);
        found = 1;
    }
    sprintf(cfgname + pos, "BaseDN.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "BaseDN", &f);
        found = 1;
    }
     sprintf(cfgname + pos, "BindDN.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "BindDN", &f);
        found = 1;
    }
     sprintf(cfgname + pos, "BindPassword.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "BindPassword", &f);
        found = 1;
    }
     sprintf(cfgname + pos, "UserSuffix.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "UserSuffix", &f);
        found = 1;
    }
      sprintf(cfgname + pos, "GroupSuffix.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "GroupSuffix", &f);
        found = 1;
    }
      sprintf(cfgname + pos, "GroupCacheTimeout.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "GroupCacheTimeout", &f);
        found = 1;
    }
      sprintf(cfgname + pos, "UserIdMap.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "UserIdMap", &f);
        found = 1;
    }
      sprintf(cfgname + pos, "GroupIdMap.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "GroupIdMap", &f);
        found = 1;
    }
     sprintf(cfgname + pos, "GroupMemberIdMap.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "GroupMemberIdMap", &f);
        found = 1;
    }
    sprintf(cfgname + pos, "Timeout.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Timeout", &f);
        found = 1;
    }
      sprintf(cfgname + pos, "EnableCache.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "EnableCache", &f);
        found = 1;
    }
       sprintf(cfgname + pos, "CacheTimeout.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "CacheTimeout", &f);
        found = 1;
    }
       sprintf(cfgname + pos, "MaxConnections.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "MaxConnections", &f);
        found = 1;
    }
        sprintf(cfgname + pos, "Enabled.%s", ldapname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Enabled", &f);
        found = 1;
    }



    if ( found == 0 ) {
        ism_common_freeProperties(newProps);
        return NULL;
    }

    return newProps;
}

extern void ism_ssl_locksReset(void);
/* Create security ldap from buffer */
static int createLDAPFromProps(ism_prop_t *ldapprops, char *ldapname, char *cfgname, int pos) {
    int rc = ISMRC_OK;
    ism_prop_t *props = ldapprops;
    char            *name=NULL;
    char            *URL=NULL;
    char            *Certificate=NULL;
    char            *CheckServerCertStr=NULL;
    char            *IgnoreCaseStr=NULL;
    char            *NestedGroupSearchStr=NULL;
    char            *BaseDN=NULL;
    char            *BindDN=NULL;
    char            *BindPassword=NULL;
    char            *UserSuffix=NULL;
    char            *GroupSuffix=NULL;
    char            *GroupCacheTTLStr;
    char            *UserIdMap=NULL;
    char            *GroupIdMap=NULL;
    char            *GroupMemberIdMap=NULL;
    char            *TimeoutStr=NULL;
    char            *EnableCacheStr=NULL;
    char            *CacheTTLStr=NULL;
    char            *MaxConnections=NULL;
    char            *EnabledStr;
    int             needFreeProps=0;
    ism_prop_t *newProps = NULL;

    if (!props) {
        TRACE(5, "Can not create ldap configuration - LDAP buffer and props NULL\n");
        return ISMRC_InvalidPolicy;
    }

    if (cfgname) {
        newProps = createLDAPProp(props, ldapname, cfgname, pos);
        if (newProps == NULL) {
            TRACE(5, "Could not find valid policy properties: %s\n", cfgname);
            return ISMRC_InvalidPolicy;
        }
        needFreeProps=1;
        props = newProps;
    }

    name = (char *) ism_common_getStringProperty(props, "Name");
    if (!name) {
        TRACE(5, "Can not create LDAP Object - invalid name\n");
        return ISMRC_InvalidPolicy;
    }


    ismLDAPConfig_t *ldapconfig = NULL;

    ldapconfig = (ismLDAPConfig_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,436),1, sizeof(ismLDAPConfig_t));
    pthread_spin_init(&ldapconfig->lock, 0);
    ldapconfig->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);

    URL = (char *) ism_common_getStringProperty(props, "URL");
    Certificate = (char *) ism_common_getStringProperty(props, "Certificate");
    CheckServerCertStr = (char *) ism_common_getStringProperty(props, "CheckServerCert");
    IgnoreCaseStr = (char *) ism_common_getStringProperty(props, "IgnoreCase");
    NestedGroupSearchStr = (char *) ism_common_getStringProperty(props, "NestedGroupSearch");
    BaseDN = (char *) ism_common_getStringProperty(props, "BaseDN");
    BindDN = (char *) ism_common_getStringProperty(props, "BindDN");

    /* decrypt BindPassword */
    const char *encryptedBindPwd = (char *) ism_common_getStringProperty(props, "BindPassword");
    BindPassword = ism_security_decryptAdminUserPasswd((char *)encryptedBindPwd);
    UserSuffix = (char *) ism_common_getStringProperty(props, "UserSuffix");
    GroupSuffix = (char *) ism_common_getStringProperty(props, "GroupSuffix");
    GroupCacheTTLStr = (char *) ism_common_getStringProperty(props, "GroupCacheTimeout");
    UserIdMap = (char *) ism_common_getStringProperty(props, "UserIdMap");
    GroupIdMap = (char *) ism_common_getStringProperty(props, "GroupIdMap");
    GroupMemberIdMap = (char *) ism_common_getStringProperty(props, "GroupMemberIdMap");
    TimeoutStr = (char *) ism_common_getStringProperty(props, "Timeout");
    EnableCacheStr = (char *) ism_common_getStringProperty(props, "EnableCache");
    CacheTTLStr = (char *) ism_common_getStringProperty(props, "CacheTimeout");
    MaxConnections = (char *) ism_common_getStringProperty(props, "MaxConnections");
    EnabledStr = (char *) ism_common_getStringProperty(props, "Enabled");

    if (URL && *URL != '\0')
        ldapconfig->URL = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),URL);

    if ( Certificate && *Certificate != '\0')
        ldapconfig->Certificate = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),Certificate);

    if (CheckServerCertStr && *CheckServerCertStr != '\0'){
        ldapconfig->CheckServerCert = ismSEC_SERVER_CERT_DISABLE_VERIFY;
        if (!strcmpi(CheckServerCertStr, "TrustStore")) {
            ldapconfig->CheckServerCert = ismSEC_SERVER_CERT_TRUST_STORE;
        } else if (!strcmpi(CheckServerCertStr, "PublicTrust")) {
            ldapconfig->CheckServerCert = ismSEC_SERVER_CERT_PUBLIC_TRUST;
        }
    } else {
        ldapconfig->CheckServerCert=ismSEC_SERVER_CERT_DISABLE_VERIFY;
    }

    if (IgnoreCaseStr && *IgnoreCaseStr != '\0'){
        if(!strcmpi(IgnoreCaseStr, "true")) {
            ldapconfig->IgnoreCase = true;
        } else {
            ldapconfig->IgnoreCase = false;
        }
    }else {
        ldapconfig->IgnoreCase=false;
    }

    if (NestedGroupSearchStr && *NestedGroupSearchStr != '\0') {
        if(!strcmpi(NestedGroupSearchStr, "true")) {
            ldapconfig->NestedGroupSearch = true;
        } else {
            ldapconfig->NestedGroupSearch = false;
        }
    }else {
        ldapconfig->NestedGroupSearch=false;
    }


    if (BaseDN && *BaseDN != '\0') {
        char * dn = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),BaseDN);
        dn_normalize(dn);
        ldapconfig->BaseDN = tolowerDN(dn);
        ism_common_free(ism_memory_admin_misc,dn);
    }

    if (BindDN && *BindDN != '\0') {
        char * dn = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),BindDN);
        dn_normalize(dn);
        ldapconfig->BindDN = tolowerDN(dn);
        ism_common_free(ism_memory_admin_misc,dn);
    }

    /* BindPassword returned by ism_security_decryptAdminUserPasswd() is already malloced memory - no need to strdup */
    if (BindPassword && *BindPassword != '\0')
        ldapconfig->BindPassword = BindPassword;

    if (UserSuffix && *UserSuffix != '\0') {
        char * dn = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),UserSuffix);
        dn_normalize(dn);
        ldapconfig->UserSuffix = tolowerDN(dn);
        ldapconfig->SearchUserDN=false;
        ism_common_free(ism_memory_admin_misc,dn);
    } else {
        ldapconfig->SearchUserDN=true;
    }

    if (GroupSuffix && *GroupSuffix != '\0') {
        char * dn = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),GroupSuffix);
        dn_normalize(dn);
        ldapconfig->GroupSuffix = tolowerDN(dn);
        ism_common_free(ism_memory_admin_misc,dn);
    }

    if (GroupCacheTTLStr && *GroupCacheTTLStr != '\0')
        ldapconfig->GroupCacheTTL = atoi(GroupCacheTTLStr);

    if (UserIdMap && *UserIdMap != '\0') {
        ldapconfig->UserIdMap = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),UserIdMap);
        tolowerDN_ASCII(ldapconfig->UserIdMap);
    }

    if (GroupIdMap && *GroupIdMap != '\0') {
        ldapconfig->GroupIdMap = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),GroupIdMap);
        tolowerDN_ASCII(ldapconfig->GroupIdMap);
    }

    if (GroupMemberIdMap && *GroupMemberIdMap != '\0') {
        ldapconfig->GroupMemberIdMap = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),GroupMemberIdMap);
        tolowerDN_ASCII(ldapconfig->GroupMemberIdMap);
    }

    if (TimeoutStr && *TimeoutStr != '\0')
        ldapconfig->Timeout = atoi(TimeoutStr);

    if (EnableCacheStr && *EnableCacheStr != '\0') {
        if (!strcmpi(EnableCacheStr, "true")) {
            ldapconfig->EnableCache = true;
        } else {
            ldapconfig->EnableCache = false;
        }
    } else {
        ldapconfig->EnableCache=false;
    }

    if (CacheTTLStr && *CacheTTLStr != '\0')
        ldapconfig->CacheTTL = atoi(CacheTTLStr);

    if (MaxConnections && *MaxConnections != '\0')
        ldapconfig->MaxConnections = atoi(MaxConnections);

    if (EnabledStr && *EnabledStr != '\0') {
        if ( !strcmpi(EnabledStr, "true")) {
            ldapconfig->Enabled = true;
        } else {
            ldapconfig->Enabled = false;
        }
    } else {
        ldapconfig->Enabled=false;
    }


    int state=ism_admin_get_server_state();

    /*Validate LDAP Configuration during start up.
     * Any changes in LDAP configuration will be validated
     * from CLI and GUI.
     */
    if(state !=ISM_SERVER_RUNNING && state!=ISM_MESSAGING_STARTED) {
        /* Validate the ldap configuration object */
        int tryCount = 1;
        rc = ism_security_validateLDAPConfig(ldapconfig, 0, 0, tryCount);
    }


    /* If the object is initialized during startup, go ahead and set the config.
     * This allows the update to the LDAP config object even though the object
     * is not valid.
     */
    if ( ldapconfig && ( (rc == ISMRC_OK) || (state != ISM_SERVER_RUNNING && state != ISM_MESSAGING_STARTED) ) ) {
        /* Validate LDAP config object before doing the actual update */
        updateLDAPConfig(ldapconfig);
        ism_ssl_locksReset();
        if(ldapconfig->Enabled!=true && (state != ISM_SERVER_RUNNING && state != ISM_MESSAGING_STARTED) )
            rc = ISMRC_OK;
    } else {
        TRACE(6,"Failed to update the external LDAP Configuration. Error Code: %d\n", rc);
        if(ldapconfig) freeLDAPConfig(ldapconfig);                      /* BEAM suppression: constant condition */
    }

    if( (needFreeProps == 1) && (props != NULL) ) {
        ism_common_freeProperties(props);
    }



    return rc;
}


/**
 * Creates Security context
 */
XAPI int32_t ism_security_create_context(ismSecurityPolicyType_t type,
        ism_transport_t * transport, ismSecurity_t * * sContext) {

    int authExtras_len = 256;
    int totalsize = sizeof(ismSecurity_t) + authExtras_len;
    ismSecurity_t * secContext;
    *sContext = secContext = (ismSecurity_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,441),1, totalsize);

    if (*sContext == NULL)
        return ISMRC_AllocateError;

    secContext->type = type;
    secContext->useCount = 1;  /* Transport create this context, set the use count to 1 */
    secContext->transport = transport;
    secContext->reValidatePolicy = 1;

    secContext->authToken.username = secContext->username;
    secContext->authToken.username_alloc_len = sizeof(secContext->username);
    secContext->authToken.password = secContext->password;
    secContext->authToken.password_alloc_len = sizeof(secContext->password);

    ism_security_initAuthToken(&secContext->authToken);
    secContext->authToken.inited=1;
    secContext->authToken.isCancelled=0;
    secContext->authToken.sContext=*sContext;
    secContext->authent.next=NULL;

    /*Note: authExtras is used to store the pContext of the AuthToken*/
    secContext->authExtras =  (char *)(secContext + 1);
    secContext->authExtras_len =  authExtras_len;
    secContext->authToken.pContext = secContext->authExtras;

    /*set the default value first*/
    secContext->AllowDurable = 1;
    secContext->AllowPersistentMessages =1;
    secContext->ExpectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
    secContext->InFlightMsgLimit = 0;
    secContext->ClientStateExpiry = UINT32_MAX;
    secContext->MaxSessionExpiry = UINT32_MAX;

    TRACE(9, "ism_security_create_context: initial AllowDurable and AllowPersistentMessages to True\n");

    if ( transport && transport->listener ) {
        if ( (transport->listener->protomask & PMASK_Admin) && (transport->listener->transmask & TMASK_AnyTrans) == TMASK_AnyTrans ) {
            if ((transport->server_addr && (!strcmp(transport->server_addr, "127.0.0.1") || !strcmp(transport->server_addr, "localhost"))) ||
                (transport->endpoint_name && (*transport->endpoint_name == '!'))) {
                secContext->adminPort = 2;
            } else {
                secContext->adminPort = 1;
            }
        }
    }

    /* set allownullpassword, LTPA Profile and OSuth Profile, if security profile is configured with */
    const char *secprof = NULL;
    if ( transport && transport->listener )
        secprof = transport->listener->secprof;
    if ( secprof ) {
        if ( transGetSecProfile ) {
            ism_secprof_t * secprofptr = (ism_secprof_t *)transGetSecProfile(secprof);

            /* Get allownullpassword and add to context */
            if ( secprofptr )
                secContext->allowNullPassword = secprofptr->allownullpassword;

            if ( secprofptr && secprofptr->ltpaprof ) {
                /* Get secretkey and add in the context */
                ismLTPAProfile_t * ltpaProfile = getLTPAProfileByName(secprofptr->ltpaprof);
                if ( ltpaProfile != NULL ) {
                    secContext->secretKey = ltpaProfile->secretKey;
                    secContext->ltpaTokenExpirationTime = 0;
                    /*Set the callback when connection is expired.*/
                    transport->expire = ism_security_connectionExpiredCallback;
                }

            }
            if ( secprofptr && secprofptr->oauthprof ) {
                /* Get url and add in the context */
                ismOAuthProfile_t * oauthProfile = getOAuthProfileByName(secprofptr->oauthprof);
                if ( oauthProfile != NULL ) {
                    secContext->url = oauthProfile->url;
                    secContext->oauthTokenExpirationTime = 0;

                    /*Set OAuth Properties into AuthToken for later use*/
                    secContext->authToken.oauth_url = oauthProfile->url;
                    secContext->authToken.oauth_userName = oauthProfile->userName;
                    secContext->authToken.oauth_userPassword = oauthProfile->userPassword;
                    secContext->authToken.oauth_userInfoUrl = oauthProfile->userInfoUrl;
                    secContext->authToken.oauth_usernameProp = oauthProfile->usernameProp;
                    secContext->authToken.oauth_groupnameProp = oauthProfile->groupnameProp;
                    secContext->authToken.oauth_groupDelimiter = oauthProfile->groupDelimiter;
                    secContext->authToken.oauth_keyFileName = oauthProfile->keyFileName;
                    secContext->authToken.oauth_fullKeyFilePath = oauthProfile->fullKeyFilePath;
                    secContext->authToken.oauth_tokenNameProp = oauthProfile->tokenNameProp;
                    secContext->authToken.oauth_isURLsEqual = oauthProfile->isURLsEqual;
                    secContext->authToken.oauth_urlProcessMethod = oauthProfile->urlProcessMethod;
                    secContext->authToken.oauth_tokenSendMethod = oauthProfile->tokenSendMethod;
                    secContext->authToken.oauth_checkServerCert = oauthProfile->checkServerCert;
                    secContext->authToken.oauth_securedConnection = 0;
                    if (oauthProfile->url && strncmpi(oauthProfile->url, "https://", 8) == 0) {
                        secContext->authToken.oauth_securedConnection = 1;
                    }

                    /*Set the callback when connection is expired.*/
                    transport->expire = ism_security_connectionExpiredCallback;
                }
            }
        }
    }

    pthread_spin_init(&(secContext->lock), 0);

    return ISMRC_OK;
}

XAPI char * ism_security_context_getUserID(ismSecurity_t *sContext)
{
    if ( !sContext || !sContext->transport )
        return NULL;
    return (char *)sContext->transport->userid;
}

XAPI ism_trclevel_t *ism_security_context_getTrcLevel(ismSecurity_t *sContext)
{
    if (sContext != NULL && sContext->transport != NULL && sContext->transport->trclevel != NULL)
    {
        return sContext->transport->trclevel;
    }
    else
    {
        return ism_defaultTrace;
    }
}

/**
 * Increase the useCount of the Security Context
 */
XAPI uint32_t ism_security_context_use_increment(ismSecurity_t *sContext) {

        if(sContext==NULL){
                return 0;
        }
        return  __sync_add_and_fetch(&(sContext->useCount), 1);
}
/**
 * Decrease the useCount of the Security Context
 */
XAPI uint32_t ism_security_context_use_decrement(ismSecurity_t *sContext) {

        if(sContext==NULL){
                return 0;
        }
        return __sync_sub_and_fetch(&(sContext->useCount), 1);
}

/* Destroy security context */
XAPI int32_t ism_security_destroy_context(ismSecurity_t *sContext) {

        if (sContext == NULL)
        return ISMRC_NotFound;

        pthread_spin_lock(&sContext->lock);

    char threadName[64];
    memset(threadName, '\0', sizeof(threadName));
    ism_common_getThreadName(threadName, 64);

    uint32_t contextUseCount= ism_security_context_use_decrement(sContext);

    /*If there is a component still use the context just return.*/
    if(contextUseCount>0){
        pthread_spin_unlock(&sContext->lock);
        return ISMRC_OK;
    }

    pthread_spin_lock(&sContext->authToken.lock);
    if(sContext->authToken.isCancelled==0 || (sContext->authToken.isCancelled==2 && *threadName=='S') ){
         pthread_spin_unlock(&sContext->authToken.lock);

         ism_security_deleteLDAPDNFromMap(sContext->authToken.username);

         if (sContext->authToken.username_inheap) {
             ism_common_free(ism_memory_admin_misc,sContext->authToken.username);
             sContext->authToken.username_inheap = 0;
         }
         if (sContext->authToken.userDN_inheap) {
             ism_common_free(ism_memory_admin_misc,sContext->authToken.userDNPtr);
             sContext->authToken.userDN_inheap = 0;
         }
         if (sContext->authToken.password_inheap) {
             ism_common_free(ism_memory_admin_misc,sContext->authToken.password);
             sContext->authToken.password_inheap = 0;
         }
         if (sContext->authToken.pContext_inheap) {
             ism_common_free(ism_memory_admin_misc,sContext->authToken.pContext);
             sContext->authToken.pContext_inheap = 0;
         }
         if (sContext->policy_inheap) {
             ism_common_free(ism_memory_admin_misc,sContext->policies);
             sContext->policy_inheap = 0;
         }
         ism_security_destroyAuthToken(&sContext->authToken);
         if ( sContext->oauthGroup)
             ism_common_free(ism_memory_admin_misc,sContext->oauthGroup);

         pthread_spin_unlock(&sContext->lock);
         ism_common_free(ism_memory_admin_misc,sContext);
         sContext=NULL;

    } else {
        pthread_spin_unlock(&sContext->authToken.lock);
        pthread_spin_unlock(&sContext->lock);
    }

    return ISMRC_OK;
}

XAPI ismLTPA_t * ism_security_context_getLTPASecretKey(ismSecurity_t *sContext)
{
    if ( !sContext )
        return NULL;
    return sContext->secretKey;

}

XAPI char * ism_security_context_getOAuthURL(ismSecurity_t *sContext)
{
    if ( !sContext )
        return NULL;
    return sContext->url;

}

typedef ism_time_t (*transportSetConnectionExpire_f)(ism_transport_t * transport, ism_time_t expire);
static transportSetConnectionExpire_f transportSetConnectionExpire = NULL;

XAPI int ism_security_context_setLTPAExpirationTime(ismSecurity_t *sContext, unsigned long tmval) {
    if ( !sContext )
        return ISMRC_Error;
    sContext->ltpaTokenExpirationTime = tmval;

    /*Set the Expiration in Transport*/
    ism_transport_t *tport = sContext->transport;
    transportSetConnectionExpire = (transportSetConnectionExpire_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_setConnectionExpire_fnptr", 0L);

    transportSetConnectionExpire(tport, tmval);
    TRACE(9, "LTPA Authentication: Set Connection Timeout: %s. Timeout: %llu\n", tport->userid, (ULL)tport->expireTime);


    return ISMRC_OK;
}

XAPI int ism_security_context_setOAuthExpirationTime(ismSecurity_t *sContext, unsigned long tmval) {
    if ( !sContext )
        return ISMRC_Error;
    sContext->oauthTokenExpirationTime = tmval;

    /*Set the Expiration in Transport*/
    ism_transport_t *tport = sContext->transport;
    transportSetConnectionExpire = (transportSetConnectionExpire_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_setConnectionExpire_fnptr", 0L);

    transportSetConnectionExpire(tport, tmval);
    TRACE(9, "OAuth Authentication: Set Connection Timeout: %s. Timeout: %llu\n", tport->userid, (ULL)tport->expireTime);


    return ISMRC_OK;
}



/* Check if member belongs to OAuth Group */

static int updLDAP(char *cfgname, int offset, ism_prop_t *props, const char * name, ismLDAPConfig_t *ldapconfig) {
    const char * newValue;
    int rc = ISMRC_OK;

    int changedURL=0;
    int changedCertificate=0;
    int changedBaseDN=0;
    int changedBindDN=0;
    int changedBindPassword=0;
    int changedUserSuffix=0;
    int changedGroupSuffix=0;
    int changedUserIdMap=0;
    int changedGroupIdMap=0;
    int changedGroupMemberIdMap=0;

    ismLDAPConfig_t tmpldapconfig;
    memset(&tmpldapconfig, 0, sizeof(ismLDAPConfig_t));
    memcpy(&tmpldapconfig, ldapconfig, sizeof(ismLDAPConfig_t));

    sprintf(cfgname + offset, "URL.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        changedURL=1;
        tmpldapconfig.URL = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);;
    }

    sprintf(cfgname + offset, "Certificate.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        changedCertificate=1;
        tmpldapconfig.Certificate = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);;
    }

    sprintf(cfgname + offset, "CheckServerCert.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        tmpldapconfig.CheckServerCert = ismSEC_SERVER_CERT_DISABLE_VERIFY;
        if (!strcmpi(newValue, "TrustStore")) {
            tmpldapconfig.CheckServerCert = ismSEC_SERVER_CERT_TRUST_STORE;
        } else if (!strcmpi(newValue, "PublicTrust")) {
            tmpldapconfig.CheckServerCert = ismSEC_SERVER_CERT_PUBLIC_TRUST;
        }
    }

    sprintf(cfgname + offset, "IgnoreCase.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        if(!strcmpi(newValue, "true")){
            tmpldapconfig.IgnoreCase = true;
        }else{
            tmpldapconfig.IgnoreCase = false;
        }

    }

    sprintf(cfgname + offset, "NestedGroupSearch.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        if(!strcmpi(newValue, "true")){
            tmpldapconfig.NestedGroupSearch = true;
        }else{
            tmpldapconfig.NestedGroupSearch = false;
        }

    }

    sprintf(cfgname + offset, "BaseDN.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        changedBaseDN=1;
        char * dn = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
        dn_normalize(dn);
        tmpldapconfig.BaseDN = tolowerDN(dn);
        ism_common_free(ism_memory_admin_misc,dn);
    }


    sprintf(cfgname + offset, "BindDN.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
       changedBindDN=1;
       char * dn = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
       dn_normalize(dn);
       tmpldapconfig.BindDN = tolowerDN(dn);
       ism_common_free(ism_memory_admin_misc,dn);
    }else if( newValue && *newValue == '\0'){
        /*If value is empty, set the BindDN to NULL*/
        tmpldapconfig.BindDN = NULL;
    }

    sprintf(cfgname + offset, "BindPassword.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        changedBindPassword=1;
        tmpldapconfig.BindPassword = (char *)ism_security_decryptAdminUserPasswd(newValue);
    }else if( newValue && *newValue == '\0'){
        /*If value is empty, set the BindPassword to NULL*/
        tmpldapconfig.BindPassword = NULL;
    }

    sprintf(cfgname + offset, "UserSuffix.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
       changedUserSuffix=1;
       char * dn = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
       dn_normalize(dn);
       tmpldapconfig.UserSuffix = tolowerDN(dn);
       tmpldapconfig.SearchUserDN = false;
       ism_common_free(ism_memory_admin_misc,dn);
    } else if(newValue && *newValue == '\0'){
        changedUserSuffix=1;
        tmpldapconfig.UserSuffix = NULL;
        tmpldapconfig.SearchUserDN = true;
    }

    sprintf(cfgname + offset, "GroupSuffix.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        changedGroupSuffix=1;
        char * dn = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
        dn_normalize(dn);
        tmpldapconfig.GroupSuffix = tolowerDN(dn);
        ism_common_free(ism_memory_admin_misc,dn);
    }

    sprintf(cfgname + offset, "GroupCacheTimeout.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        tmpldapconfig.GroupCacheTTL = atoi(newValue);
    }
    sprintf(cfgname + offset, "UserIdMap.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        changedUserIdMap=1;
        tmpldapconfig.UserIdMap = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
        tolowerDN_ASCII(tmpldapconfig.UserIdMap);
    }

    sprintf(cfgname + offset, "GroupIdMap.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        changedGroupIdMap=1;
        tmpldapconfig.GroupIdMap = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
        tolowerDN_ASCII(tmpldapconfig.GroupIdMap);
    }

    sprintf(cfgname + offset, "GroupMemberIdMap.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        changedGroupMemberIdMap=1;
        tmpldapconfig.GroupMemberIdMap = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
        tolowerDN_ASCII(tmpldapconfig.GroupMemberIdMap);
    }

    sprintf(cfgname + offset, "Timeout.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
       tmpldapconfig.Timeout = atoi(newValue);
    }

    sprintf(cfgname + offset, "EnableCache.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        if(!strcmpi(newValue, "true")){
            tmpldapconfig.EnableCache = true;
        }else{
            tmpldapconfig.EnableCache = false;
        }
    }

    sprintf(cfgname + offset, "CacheTimeout.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {

       tmpldapconfig.CacheTTL = atoi(newValue);
    }

    sprintf(cfgname + offset, "MaxConnections.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {

       tmpldapconfig.MaxConnections = atoi(newValue);
    }

    int changedEnabled=0;
    sprintf(cfgname + offset, "Enabled.%s", name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( newValue && *newValue != '\0' ) {
        changedEnabled=1;
        if(!strcmpi(newValue, "true")){
            tmpldapconfig.Enabled = true;
        }else{
            tmpldapconfig.Enabled = false;
        }

    }

    /* Copy to the actual object */
    if(changedURL && ldapconfig->URL) ism_common_free(ism_memory_admin_misc,ldapconfig->URL);
    if(changedBaseDN && ldapconfig->BaseDN) ism_common_free(ism_memory_utils_to_lower,ldapconfig->BaseDN);
    if(changedBindDN && ldapconfig->BindDN) ism_common_free(ism_memory_utils_to_lower,ldapconfig->BindDN);
    if(changedBindPassword && ldapconfig->BindPassword) ism_common_free(ism_memory_admin_misc,ldapconfig->BindPassword);
    if(changedUserSuffix && ldapconfig->UserSuffix) ism_common_free(ism_memory_utils_to_lower,ldapconfig->UserSuffix);
    if(changedGroupSuffix && ldapconfig->GroupSuffix) ism_common_free(ism_memory_utils_to_lower,ldapconfig->GroupSuffix);
    if(changedUserIdMap && ldapconfig->UserIdMap) ism_common_free(ism_memory_admin_misc,ldapconfig->UserIdMap);
    if(changedGroupIdMap && ldapconfig->GroupIdMap) ism_common_free(ism_memory_admin_misc,ldapconfig->GroupIdMap);
    if(changedGroupMemberIdMap && ldapconfig->GroupMemberIdMap) ism_common_free(ism_memory_admin_misc,ldapconfig->GroupMemberIdMap);
    if(changedCertificate && ldapconfig->Certificate) ism_common_free(ism_memory_admin_misc,ldapconfig->Certificate);

    memcpy(ldapconfig, &tmpldapconfig, sizeof(ismLDAPConfig_t));


    pthread_spin_lock(&ldapconfig->lock);

    ism_security_getLDAPIdPrefix(ldapconfig->UserIdMap, ldapconfig->UserIdPrefix);
    ism_security_getLDAPIdPrefix(ldapconfig->GroupIdMap, ldapconfig->GroupIdPrefix);
    ism_security_getLDAPIdPrefix(ldapconfig->GroupMemberIdMap, ldapconfig->GroupMemberIdPrefix);
    updateLDAPDNMaxLen(ldapconfig);

    if(ldapconfig->deleted==false)
    {
        if(ldapconfig->Enabled){
            ism_security_setLDAPConfig(ldapconfig);
        }else if(changedEnabled){
            /*Fallback to local ldap when LDAP is disabled.*/
            if(_localLdapConfig!=NULL)
                ism_security_setLDAPConfig(_localLdapConfig);
        }

    }else{
        if(_localLdapConfig!=NULL)
            ism_security_setLDAPConfig(_localLdapConfig);
    }
    pthread_spin_unlock(&ldapconfig->lock);

    return rc;     /* BEAM suppression: memory leak */
}

XAPI int ism_security_ldap_update(ism_prop_t *props, char *oldname, int flag) {
    int rc = ISMRC_OK;

    if (!props)
        return ISMRC_NullPointer;

    /* loop thru the list and create ldap objects */
    TRACE(9,"Process ldap configuration items.\n");
    const char * propertyName = NULL;
    int i = 0;
    for (i = 0; ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++) {
        char *cfgname = NULL;
        const char *name = NULL;
        int pos = 0;
        int proplen = strlen(propertyName);

        if (proplen >= 10 && !memcmp(propertyName,
                "LDAP.Name.",10)){
            name = propertyName + 10;
            cfgname = alloca(strlen(name) + 32);
            sprintf(cfgname, "LDAP.");
            pos = 5;
        }
        if (pos > 0) {
            TRACE(5,"Process (flag=%d) security LDAP configuration item %s\n",
                flag, propertyName);
            /* get name */
            sprintf(cfgname + pos, "Name.%s", name);
            const char *value = ism_common_getStringProperty(props, cfgname);
            if (!value && flag != ISM_CONFIG_CHANGE_NAME) {
                TRACE(5, "Name is NULL. Can not update LDAP configuration. CFG:%s\n",
                        cfgname);
                rc = ISMRC_InvalidLDAP;
                break;
            }

            /* get current policy */
            ismLDAPConfig_t *ldapobj = getLDAPByName((char *)value);

            /* Add or update property */
            if (flag == ISM_CONFIG_CHANGE_PROPS) {
                /* add ldap */
                if (!ldapobj) {
                    rc = createLDAPFromProps(props, (char *)name, cfgname, pos);
                    if (rc != ISMRC_OK) {
                        TRACE(5, "Failed to add LDAP object %s.\n", name);
                        break;
                    }
                    continue;
                }

                /* Update LDAP */
                ldapobj->deleted=false; /* Mark this object not to be deleted. */
                rc = updLDAP(cfgname, pos, props, name, ldapobj);
                if (rc != ISMRC_OK) {
                    TRACE(3, "Failed to update LDAP Configuration: %s\n", ldapobj->name);
                }
                continue;
            }

            /* Delete LDAP */
            if (flag == ISM_CONFIG_CHANGE_DELETE) {
                if (!ldapobj) {
                    TRACE(5, "Trying to delete a non-existing LDAP configuration: %s\n",
                            cfgname);
                    rc = ISMRC_InvalidLDAP;
                } else {
                    pthread_spin_lock(&ldapconfiglock);
                    ldapobj->deleted = 1;
                    if(strncmp(ldapobj->URL, "ldaps",5)==0 && ldapobj->FullCertificate!=NULL){

                        if(remove(ldapobj->FullCertificate)==0){
                            TRACE(5, "The LDAP Certificate is removed for %s\n",
                            ldapobj->URL);
                        }else{
                            TRACE(5, "Failed to remove the LDAP Certificate for %s\n",
                            ldapobj->URL);
                        }

                    }
                    pthread_spin_unlock(&ldapconfiglock);

                    ism_security_setLDAPConfig(_localLdapConfig);
                }
                continue;
            }

            /* Rename policy */
            if (flag == ISM_CONFIG_CHANGE_NAME) {
                /* get policy from old name */
                ismLDAPConfig_t *ldapconfig = getLDAPByName(oldname);
                if (!ldapconfig) {
                    TRACE( 2, "Can not rename a non-existing LDAP. CFG:%s\n", cfgname);
                    rc = ISMRC_InvalidLDAP;
                } else {
                    pthread_spin_lock(&ldapconfiglock);
                    if ( ldapconfig->name )
                        ism_common_free(ism_memory_admin_misc, ldapconfig->name );
                    ldapconfig->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);

                    pthread_spin_unlock(&ldapconfiglock);
                }
            }
        }
    }
    return rc;
}

/* ------------------------------------------------------------------------ */
/* Functions to maintain LTPA Profiles */

static int addLTPAPObject(ismLTPAProfile_t * ltpaprofile) {
    int i;

    if (ltpaobjects->count == ltpaobjects->nalloc) {
        ismLTPAProfile_t ** tmp = NULL;
        int firstSlot = ltpaobjects->nalloc;
        ltpaobjects->nalloc = ltpaobjects->nalloc == 0 ? 64 : ltpaobjects->nalloc * 2;
        tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_admin_misc,454),ltpaobjects->ltpaprofiles, sizeof(ismLTPAProfile_t *)
                * ltpaobjects->nalloc);
        if (tmp == NULL) {
            return ISMRC_AllocateError;
        }
        ltpaobjects->ltpaprofiles = tmp;
        for (i = firstSlot; i < ltpaobjects->nalloc; i++)
            ltpaobjects->ltpaprofiles[i] = NULL;
        ltpaobjects->slots = ltpaobjects->count;
    }
    if (ltpaobjects->count == ltpaobjects->slots) {
        ltpaobjects->ltpaprofiles[ltpaobjects->count] = ltpaprofile;
        ltpaobjects->id = ltpaobjects->count;
        ltpaobjects->count++;
        ltpaobjects->slots++;
    } else {
        for (i = 0; i < ltpaobjects->slots; i++) {
            if (!ltpaobjects->ltpaprofiles[i]) {
                ltpaobjects->ltpaprofiles[i] = ltpaprofile;
                ltpaobjects->id = i;
                ltpaobjects->count++;
                break;
            }
        }
    }
    return ISMRC_OK;
}

static ismLTPAProfile_t *getLTPAProfileByName(const char *name) {
    int i = 0;
    for (i = 0; i < ltpaobjects->count; i++) {
        ismLTPAProfile_t *ltpaobj = ltpaobjects->ltpaprofiles[i];
        if (name && ltpaobj && ltpaobj->name && strcmp(ltpaobj->name, name) == 0) {
            return ltpaobj;
        }
    }
    return NULL;
}

static int createUpdateLTPAProfileFromProps(ism_prop_t * props, char * name, char * cfgname, int pos, ismLTPAProfile_t * ltpaprofile) {
    int rc = ISMRC_OK;

    /* this function assumes that the data sent is already verified */
    /* Get name and password from props */
    sprintf(cfgname + pos, "KeyFileName.%s", name);
    const char *keyfilename = ism_common_getStringProperty(props, cfgname);
    if ( !keyfilename || *keyfilename == '\0' ) {
        TRACE(5, "Invalid LTPAProfile configuration: keyfilename is NULL\n");
        rc = ISMRC_NullArgument;
        ism_common_setError(ISMRC_NullArgument);
        return rc;
    }
    sprintf(cfgname + pos, "Password.%s", name);
    const char *encryptedPWD = ism_common_getStringProperty(props, cfgname);

    //decrypt the ltpa password
    char *password = (char *)ism_security_decryptAdminUserPasswd((char *)encryptedPWD);
    if ( !password || *password == '\0' ) {
        TRACE(5, "Invalid LTPAProfile configuration: password is NULL\n");
        rc = ISMRC_NullArgument;
        ism_common_setError(ISMRC_NullArgument);
        return rc;
    }

    char *keyfname = NULL;
    const char *ltpaKeyStore = ism_common_getStringProperty(ism_common_getConfigProperties(), "LTPAKeyStore");
    int keypathlen = 0;
    if ( ltpaKeyStore ) {
        keypathlen = strlen(keyfilename) + strlen(ltpaKeyStore) + 32;
    } else {
        keypathlen = strlen(keyfilename) + 1024;
    }
    keyfname = (char *)alloca(keypathlen);
    sprintf(keyfname, "%s/%s", ltpaKeyStore?ltpaKeyStore:"", keyfilename);
    TRACE(5, "LTPAKey file: %s\n", keyfname);

    ismLTPA_t *secretKey=NULL;
    rc = ism_security_ltpaReadKeyfile(keyfname, password, &secretKey);

    if (password)  ism_common_free(ism_memory_admin_misc,password);
    if ( rc != ISMRC_OK )
        return rc;

    /*
    int secretKey_len = 0;
    unsigned char *secretKey = ism_security_get_ltpaSecretKey(keyfname, password, &secretKey_len, &rc);
    if ( rc != ISMRC_OK )
        return rc;

    unsigned char *secretKey = ltpaKeys->des_key;
    secretKey_len = ltpaKeys->des_key_len;

    int i=0, slen=0;
    char pkey[1024];
    memset(pkey, 0, 1024);
    unsigned char *tl = secretKey;
    for (i=0;i<secretKey_len;i++) {
        snprintf(pkey+slen, 1024-slen,  "%02x", tl[i]);
        slen += 2;
    }
    pkey[slen] = 0;
    TRACE(5, "LTPAKey: %s\n", pkey);
    */


    pthread_spin_lock(&ltpaconfiglock);
    if ( ltpaprofile ) {
        /* Update entry */
        /* update LTPA profile structure */
        if ( ltpaprofile->keyfilename ) ism_common_free(ism_memory_admin_misc,(void *)ltpaprofile->keyfilename);
        ltpaprofile->keyfilename = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),keyfilename);
        if ( ltpaprofile->secretKey ) ism_security_ltpaDeleteKey(ltpaprofile->secretKey);
        ltpaprofile->secretKey = secretKey;
        ltpaprofile->deleted = 0;
    } else {
        /* create and add entry */
        ismLTPAProfile_t *ltpaprof = (ismLTPAProfile_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,457),1, sizeof(ismLTPAProfile_t));
        ltpaprof->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);
        ltpaprof->keyfilename = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),keyfilename);
        ltpaprof->secretKey = secretKey;
        ltpaprof->deleted = 0;

        /* Add entry */
        rc = addLTPAPObject(ltpaprof);
        if ( rc != ISMRC_OK ) {
            TRACE(5, "Failed to add LTPAProfile in the object list\n");
            ism_common_setError(rc);
            ism_common_free(ism_memory_admin_misc,(void *)ltpaprof->name);
            ism_common_free(ism_memory_admin_misc,(void *)ltpaprof->keyfilename);
            ism_security_ltpaDeleteKey(ltpaprof->secretKey);
            ism_common_free(ism_memory_admin_misc,(void *)ltpaprof);
        }
    }
    pthread_spin_unlock(&ltpaconfiglock);

    return rc;
}

XAPI int ism_security_ltpa_update(ism_prop_t * props, char * oldName, ism_ConfigChangeType_t flag) {
    int rc = ISMRC_OK;

    if (!props)
        return ISMRC_NullPointer;

    /* loop thru the list and create LTPA objects */
    TRACE(9,"Process LTPA Profile configuration items.\n");
    const char * propertyName = NULL;
    int i = 0;
    for (i = 0; ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++) {
        char *cfgname = NULL;
        const char *name = NULL;
        int pos = 0;
        int proplen = strlen(propertyName);

        if (proplen >= 17 && !memcmp(propertyName,
                "LTPAProfile.Name.",17)){
            name = propertyName + 17;
            cfgname = alloca(strlen(name) + 32);
            sprintf(cfgname, "LTPAProfile.");
            pos = 12;
        }
        if (pos > 0) {
            TRACE(5,"Process (flag=%d) LTPAProfile configuration item %s\n",
                flag, propertyName);
            /* get name */
            sprintf(cfgname + pos, "Name.%s", name);
            const char *value = ism_common_getStringProperty(props, cfgname);
            if (!value && flag != ISM_CONFIG_CHANGE_NAME) {
                TRACE(5, "Name is NULL. Can not update LTPAProfile configuration. CFG:%s\n",
                        cfgname);
                rc = ISMRC_Error;   /* TODO: create an error */
                break;
            }

            /* check if profile exist */
            ismLTPAProfile_t *ltpaprofile = getLTPAProfileByName(value);

            /* Add or update property */
            if (flag == ISM_CONFIG_CHANGE_PROPS) {
                rc = createUpdateLTPAProfileFromProps(props, (char *)name, cfgname, pos, ltpaprofile);
                if (rc != ISMRC_OK) {
                    if ( ltpaprofile ) {
                        TRACE(5, "Failed to update LTPAProfile %s.\n", name);
                    } else {
                        TRACE(5, "Failed to add LTPAProfile %s.\n", name);
                    }
                    break;
                }
                continue;
            }

            /* Delete LDAP */
            if (flag == ISM_CONFIG_CHANGE_DELETE) {
                if (!ltpaprofile) {
                    TRACE(5, "Trying to delete a non-existing LTPAProfile configuration: %s\n", name);
                    rc = ISMRC_NotFound;
                    ism_common_setError(ISMRC_NotFound);
                } else {
                    pthread_spin_lock(&ltpaconfiglock);
                    ltpaprofile->deleted = 1;
                    pthread_spin_unlock(&ltpaconfiglock);
                    TRACE(5, "LTPAProfile is deleted: %s\n", name);
                }
                continue;
            }

            /* Rename policy */
            if (flag == ISM_CONFIG_CHANGE_NAME) {
                rc = ISMRC_NotImplemented;
                ism_common_setError(ISMRC_NotImplemented);
            }
        }
    }

    return rc;
}

/* ------------------------------------------------------------------------ */
/* Functions to maintain OAuth Profiles */

static int addOAuthObject(ismOAuthProfile_t * oauthprofile) {
    int i;

    if (oauthobjects->count == oauthobjects->nalloc) {
        ismOAuthProfile_t ** tmp = NULL;
        int firstSlot = oauthobjects->nalloc;
        oauthobjects->nalloc = oauthobjects->nalloc == 0 ? 64 : oauthobjects->nalloc * 2;
        tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_admin_misc,461),oauthobjects->oauthprofiles, sizeof(ismOAuthProfile_t *)
                * oauthobjects->nalloc);
        if (tmp == NULL) {
            return ISMRC_AllocateError;
        }
        oauthobjects->oauthprofiles = tmp;
        for (i = firstSlot; i < oauthobjects->nalloc; i++)
            oauthobjects->oauthprofiles[i] = NULL;
        oauthobjects->slots = oauthobjects->count;
    }
    if (oauthobjects->count == oauthobjects->slots) {
        oauthobjects->oauthprofiles[oauthobjects->count] = oauthprofile;
        oauthobjects->id = oauthobjects->count;
        oauthobjects->count++;
        oauthobjects->slots++;
    } else {
        for (i = 0; i < oauthobjects->slots; i++) {
            if (!oauthobjects->oauthprofiles[i]) {
                oauthobjects->oauthprofiles[i] = oauthprofile;
                oauthobjects->id = i;
                oauthobjects->count++;
                break;
            }
        }
    }
    return ISMRC_OK;
}

static ismOAuthProfile_t *getOAuthProfileByName(const char *name) {
    int i = 0;
    for (i = 0; i < oauthobjects->count; i++) {
        ismOAuthProfile_t *oauthobj = oauthobjects->oauthprofiles[i];
        if (name && oauthobj && oauthobj->name && strcmp(oauthobj->name, name) == 0) {
            return oauthobj;
        }
    }
    return NULL;
}

static int createUpdateOAuthProfileFromProps(ism_prop_t * props, char * name, char * cfgname, int pos, ismOAuthProfile_t * oauthprofile) {
    int rc = ISMRC_OK;

    /* this function assumes that the data sent is already verified */
    sprintf(cfgname + pos, "ResourceURL.%s", name);
    const char *url = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname + pos, "UserName.%s", name);
    const char *userName = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname + pos, "UserPassword.%s", name);
    const char *userPassword = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname + pos, "KeyFileName.%s", name);
    const char *keyfilename = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname + pos, "AuthKey.%s", name);
    const char *tokenname = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname + pos, "UserInfoURL.%s", name);
    const char *userInfoUrl = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname + pos, "UserInfoKey.%s", name);
    const char *usernameProp = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname + pos, "GroupInfoKey.%s", name);
    const char *groupnameProp = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname + pos, "GroupDelimiter.%s", name);
    const char *groupDelimiter = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname + pos, "TokenSendMethod.%s", name);
    const char *tokenSendMethodProp = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname + pos, "CheckServerCert.%s", name);
    const char *checkServerCertProp = ism_common_getStringProperty(props, cfgname);

    char * tmpoauthSSLCertFile =NULL;
    if(keyfilename!=NULL && *keyfilename!='\0'){
                const char * keystore = ism_common_getStringConfig("OAuthCertificateDir");
                if (!keystore)
                        keystore = IMA_SVR_INSTALL_PATH "/certificates/OAuth";
                int keystore_len = (int)strlen(keystore);
                tmpoauthSSLCertFile = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,462),keystore_len + strlen(keyfilename) + 2);
                strcpy(tmpoauthSSLCertFile, keystore);
                strcat(tmpoauthSSLCertFile, "/");
                strcat(tmpoauthSSLCertFile, keyfilename);
        }

    /* Get OAuth profile data */

    pthread_spin_lock(&oauthconfiglock);
    if ( oauthprofile ) {
        /* Update entry */

        /* update OAuth profile structure */

        oauthprofile->tokenSendMethod = ISM_OAUTH_TOKENSEND_URL_PARAM;
        if (tokenSendMethodProp !=NULL && !strcmp("HTTPHeader",tokenSendMethodProp)) {
            oauthprofile->tokenSendMethod = ISM_OAUTH_TOKENSEND_HTTP_HEADER;
        } else if (tokenSendMethodProp !=NULL && !strcmp("HTTPPost",tokenSendMethodProp)) {
            oauthprofile->tokenSendMethod = ISM_OAUTH_TOKENSEND_HTTP_POST;
        }

        oauthprofile->checkServerCert = ismSEC_SERVER_CERT_DISABLE_VERIFY;
        if (checkServerCertProp !=NULL && !strcmp("TrustStore",checkServerCertProp)) {
            oauthprofile->checkServerCert = ismSEC_SERVER_CERT_TRUST_STORE;
        } else if (checkServerCertProp !=NULL && !strcmp("PublicTrust",checkServerCertProp)) {
            oauthprofile->checkServerCert = ismSEC_SERVER_CERT_PUBLIC_TRUST;
        }

        if(url != NULL && *url !='\0'){
            if ( oauthprofile->url ) ism_common_free(ism_memory_admin_misc,oauthprofile->url);
                oauthprofile->url = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),url);

            if (oauthprofile->tokenSendMethod == ISM_OAUTH_TOKENSEND_HTTP_HEADER) {
              	oauthprofile->urlProcessMethod = ISM_OAUTH_URL_PROCESS_HEADER;
            } else if (oauthprofile->tokenSendMethod == ISM_OAUTH_TOKENSEND_HTTP_POST ) {
                oauthprofile->urlProcessMethod = ISM_OAUTH_URL_PROCESS_POST;
            } else {
                /* set OAuth URL process method */
                oauthprofile->urlProcessMethod = ISM_OAUTH_URL_PROCESS_NORMAL;
                int p = 0;
                int l = strlen(url);

                p = strcspn(url, "?");
                if ( p != l ) oauthprofile->urlProcessMethod = ISM_OAUTH_URL_PROCESS_QUERY;
                p = strcspn(url, "&");
                if ( p != l ) oauthprofile->urlProcessMethod = ISM_OAUTH_URL_PROCESS_QUERY_WITH_AND;
           }
        }

        if (userName!=NULL && *userName != '\0'){
                if ( oauthprofile->userName ) ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->userName);
                oauthprofile->userName= ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),userName);
        } else if( userName!=NULL && *userName=='\0') {
                if ( oauthprofile->userName ){
                        ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->userName);
                        oauthprofile->userName=NULL;
                }
        }

        if (userPassword!=NULL && *userPassword != '\0'){
                if ( oauthprofile->userPassword ) ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->userPassword);
                oauthprofile->userPassword= ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),userPassword);
        } else if( userPassword!=NULL && *userPassword=='\0') {
                if ( oauthprofile->userPassword ){
                        ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->userPassword);
                        oauthprofile->userPassword=NULL;
                }
        }

        if(userInfoUrl!=NULL && *userInfoUrl !='\0'){
                if ( oauthprofile->userInfoUrl ) ism_common_free(ism_memory_admin_misc,oauthprofile->userInfoUrl);
                oauthprofile->userInfoUrl = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),userInfoUrl);
        }else{
                if ( oauthprofile->userInfoUrl ) {
                        ism_common_free(ism_memory_admin_misc,oauthprofile->userInfoUrl);
                        oauthprofile->userInfoUrl=NULL;
                }

        }

        if(keyfilename!=NULL && *keyfilename!='\0'){
                if ( oauthprofile->keyFileName ) ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->keyFileName);
                oauthprofile->keyFileName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),keyfilename);
                if(oauthprofile->fullKeyFilePath) ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->fullKeyFilePath);
                        oauthprofile->fullKeyFilePath = tmpoauthSSLCertFile;
        }else if( keyfilename!=NULL && *keyfilename=='\0') {
                if ( oauthprofile->keyFileName ) {
                        /*Remove KeyFile since KeyFileName is set to empty*/
                        ism_config_deleteOAuthKeyFile(oauthprofile->keyFileName);

                        ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->keyFileName);
                        oauthprofile->keyFileName=NULL;
                }
                if (oauthprofile->fullKeyFilePath){
                        ism_common_free(ism_memory_admin_misc,(void *) oauthprofile->fullKeyFilePath);
                        oauthprofile->fullKeyFilePath=NULL;
                }

        }

        if(tokenname!=NULL && *tokenname !='\0'){
                        if ( oauthprofile->tokenNameProp ) ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->tokenNameProp);
                        oauthprofile->tokenNameProp = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tokenname);
        }

        if (usernameProp!=NULL && *usernameProp != '\0'){
                if ( oauthprofile->usernameProp ) ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->usernameProp);
                oauthprofile->usernameProp= ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),usernameProp);
        } else if( usernameProp!=NULL && *usernameProp=='\0') {
                if ( oauthprofile->usernameProp ){
                        ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->usernameProp);
                        oauthprofile->usernameProp=NULL;
                }

        }

        if (groupnameProp!=NULL && *groupnameProp != '\0'){
                if ( oauthprofile->groupnameProp ) ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->groupnameProp);
                oauthprofile->groupnameProp= ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),groupnameProp);
        } else if( groupnameProp!=NULL && *groupnameProp=='\0') {
                if ( oauthprofile->groupnameProp ){
                        ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->groupnameProp);
                        oauthprofile->groupnameProp=NULL;
                }

        }

        if (groupDelimiter!=NULL && *groupDelimiter != '\0'){
                if ( oauthprofile->groupDelimiter ) ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->groupDelimiter);
                oauthprofile->groupDelimiter= ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),groupDelimiter);
        } else if( groupDelimiter!=NULL && *groupDelimiter=='\0') {
                if ( oauthprofile->groupDelimiter ){
                        ism_common_free(ism_memory_admin_misc,(void *)oauthprofile->groupDelimiter);
                        oauthprofile->groupDelimiter=NULL;
                }

        }

        /*Determine if Resource URL and UserInfo URL are equal*/
        oauthprofile->isURLsEqual=0;
        if(oauthprofile->userInfoUrl!=NULL &&
                        !strcmp(oauthprofile->url, oauthprofile->userInfoUrl)){
                oauthprofile->isURLsEqual=1;
        }

        oauthprofile->deleted = 0;

    } else {
        /* create and add entry */
        ismOAuthProfile_t *oauthprof = (ismOAuthProfile_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,475),1, sizeof(ismOAuthProfile_t));
        oauthprof->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);

        if (url) oauthprof->url = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),url);
        if (userName) oauthprof->userName = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),userName);
        if (userPassword) oauthprof->userPassword = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),userPassword);
        if (userInfoUrl && *userInfoUrl) oauthprof->userInfoUrl = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),userInfoUrl);
        if (usernameProp) oauthprof->usernameProp= (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),usernameProp);
        if (groupnameProp) oauthprof->groupnameProp= (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),groupnameProp);
        if (groupDelimiter) oauthprof->groupDelimiter= (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),groupDelimiter);

        oauthprof->tokenSendMethod = ISM_OAUTH_TOKENSEND_URL_PARAM;
        if (tokenSendMethodProp !=NULL && !strcmp("HTTPHeader",tokenSendMethodProp)) {
            oauthprof->tokenSendMethod = ISM_OAUTH_TOKENSEND_HTTP_HEADER;
        } else if (tokenSendMethodProp !=NULL && !strcmp("HTTPPost",tokenSendMethodProp)) {
            oauthprof->tokenSendMethod = ISM_OAUTH_TOKENSEND_HTTP_POST;
        }

        oauthprof->checkServerCert = ismSEC_SERVER_CERT_DISABLE_VERIFY;
        if (checkServerCertProp !=NULL && !strcmp("TrustStore",checkServerCertProp)) {
            oauthprof->checkServerCert = ismSEC_SERVER_CERT_TRUST_STORE;
        } else if (checkServerCertProp !=NULL && !strcmp("PublicTrust",checkServerCertProp)) {
            oauthprof->checkServerCert = ismSEC_SERVER_CERT_PUBLIC_TRUST;
        }

        if (keyfilename) {
                oauthprof->keyFileName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),keyfilename);
                oauthprof->fullKeyFilePath = tmpoauthSSLCertFile;
        }
        if (tokenname) oauthprof->tokenNameProp = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tokenname);
        oauthprof->deleted = 0;
        /*Determine if Resource URL and UserInfo URL are equal*/
        oauthprof->isURLsEqual=0;
        if (oauthprof->userInfoUrl!=NULL &&
                        !strcmp(oauthprof->url, oauthprof->userInfoUrl)){
                oauthprof->isURLsEqual=1;
        }

        if ( url ) {
            if (  oauthprof->tokenSendMethod == ISM_OAUTH_TOKENSEND_HTTP_HEADER ) {
                oauthprof->urlProcessMethod = ISM_OAUTH_URL_PROCESS_HEADER;
            } else if (  oauthprof->tokenSendMethod == ISM_OAUTH_TOKENSEND_HTTP_POST ) {
                oauthprof->urlProcessMethod = ISM_OAUTH_URL_PROCESS_POST;
            } else {
                /* set OAuth URL process method */
                oauthprof->urlProcessMethod = ISM_OAUTH_URL_PROCESS_NORMAL;
                int p = 0;
                int l = strlen(url);

                p = strcspn(url, "?");
                if ( p != l ) oauthprof->urlProcessMethod = ISM_OAUTH_URL_PROCESS_QUERY;
                p = strcspn(url, "&");
                if ( p != l ) oauthprof->urlProcessMethod = ISM_OAUTH_URL_PROCESS_QUERY_WITH_AND;
            }
        }

        /* Add entry */
        rc = addOAuthObject(oauthprof);
        if ( rc != ISMRC_OK ) {
            TRACE(5, "Failed to add OAuthProfile in the object list\n");
            ism_common_setError(rc);
            ism_common_free(ism_memory_admin_misc,(void *)oauthprof->name);
            if (oauthprof->url) ism_common_free(ism_memory_admin_misc,(void *)oauthprof->url);
            if (oauthprof->userName) ism_common_free(ism_memory_admin_misc,(void *)oauthprof->userName);
            if (oauthprof->userPassword) ism_common_free(ism_memory_admin_misc,(void *)oauthprof->userPassword);
            if (oauthprof->keyFileName) ism_common_free(ism_memory_admin_misc,(void *)oauthprof->keyFileName);
            if(oauthprof->fullKeyFilePath) ism_common_free(ism_memory_admin_misc,(void *)oauthprof->fullKeyFilePath);
            if (oauthprof->tokenNameProp) ism_common_free(ism_memory_admin_misc,(void *)oauthprof->tokenNameProp);
            if (oauthprof->userInfoUrl) ism_common_free(ism_memory_admin_misc,(void *)oauthprof->userInfoUrl);
            if (oauthprof->usernameProp) ism_common_free(ism_memory_admin_misc,(void *)oauthprof->usernameProp);
            if (oauthprof->groupnameProp) ism_common_free(ism_memory_admin_misc,(void *)oauthprof->groupnameProp);
            if (oauthprof->groupDelimiter) ism_common_free(ism_memory_admin_misc,(void *)oauthprof->groupDelimiter);
            ism_common_free(ism_memory_admin_misc,(void *)oauthprof);
        }
    }
    pthread_spin_unlock(&oauthconfiglock);

    return rc;
}

XAPI int ism_security_oauth_update(ism_prop_t * props, char * oldName, ism_ConfigChangeType_t flag) {
    int rc = ISMRC_OK;

    if (!props)
        return ISMRC_NullPointer;

    /* loop thru the list and create OAuth objects */
    TRACE(9,"Check and process OAuth Profile configuration items.\n");
    const char * propertyName = NULL;
    int i = 0;
    for (i = 0; ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++) {
        char *cfgname = NULL;
        const char *name = NULL;
        int pos = 0;
        int proplen = strlen(propertyName);

        if (proplen >= 18 && !memcmp(propertyName,
                "OAuthProfile.Name.",18)){
            name = propertyName + 18;
            cfgname = alloca(strlen(name) + 32);
            sprintf(cfgname, "OAuthProfile.");
            pos = 13;
        }
        if (pos > 0) {
            TRACE(5,"Process (flag=%d) OAuthProfile configuration item %s\n",
                flag, propertyName);
            /* get name */
            sprintf(cfgname + pos, "Name.%s", name);
            const char *value = ism_common_getStringProperty(props, cfgname);
            if (!value && flag != ISM_CONFIG_CHANGE_NAME) {
                TRACE(5, "Name is NULL. Can not update OAuthProfile configuration. CFG:%s\n",
                        cfgname);
                rc = ISMRC_Error;   /* TODO: create an error */
                break;
            }

            /* check if profile exist */
            ismOAuthProfile_t *oauthprofile = getOAuthProfileByName(value);

            /* Add or update property */
            if (flag == ISM_CONFIG_CHANGE_PROPS) {
                rc = createUpdateOAuthProfileFromProps(props, (char *)name, cfgname, pos, oauthprofile);
                if (rc != ISMRC_OK) {
                    if ( oauthprofile ) {
                        TRACE(5, "Failed to update OAuthProfile %s.\n", name);
                    } else {
                        TRACE(5, "Failed to add OAuthProfile %s.\n", name);
                    }
                    break;
                }
                continue;
            }

            /* Delete OAuth profile */
            if (flag == ISM_CONFIG_CHANGE_DELETE) {
                if (!oauthprofile) {
                    TRACE(5, "Trying to delete a non-existing OAuthProfile configuration: %s\n", name);
                    rc = ISMRC_NotFound;
                    ism_common_setError(ISMRC_NotFound);
                } else {
                    pthread_spin_lock(&oauthconfiglock);
                    oauthprofile->deleted = 1;
                    pthread_spin_unlock(&oauthconfiglock);
                    TRACE(5, "OAuthProfile is deleted: %s\n", name);
                }
                continue;
            }

            /* Rename policy */
            if (flag == ISM_CONFIG_CHANGE_NAME) {
                rc = ISMRC_NotImplemented;
                ism_common_setError(ISMRC_NotImplemented);
            }
        }
    }

    return rc;
}

/**
 * Get the authentication token from the security context
 * @param sContext the security context
 * @return the authentication token.
 */
ismAuthToken_t * ism_security_getSecurityContextAuthToken(ismSecurity_t *sContext)
{
    return (ismAuthToken_t *)&sContext->authToken;
}


/**
 * Get the Authentication event object from the security context
 * @param sContext the security context
 * @return the authentication event object
 */
ismAuthEvent_t * ism_security_getSecurityContextAuthEvent(ismSecurity_t *sContext)
{
    return &sContext->authent;
}

/**
 * Get the AllowNullPassword from the security context
 * @param sContext the security context
 * @return AllowNullPassword setting
 */
int ism_security_getSecurityContextAllowNullPassword(ismSecurity_t *sContext)
{
    return sContext->allowNullPassword;
}

/**
 * Set the AuditLogControl variable
 * @param value the control log value
 */
XAPI void ism_security_setAuditControlLog(int value) {
    auditLogControl = value;
}

/**
 * Get value the checkGroup global setting.
 * @param sContext the Security Context
 * @return 1 for True. Otherwise, it is false.
 */
int ism_security_getContextCheckGroup(ismSecurity_t *sContext)
{
    return sContext->checkGroup;
}

/**
 * Return criticalConfigError value *
 */
int ism_config_criticalConfigError(void) {
    return criticalConfigError;
}

/*Set header LTPA token into security context*/
XAPI int32_t ism_security_setLTPAToken(
        ism_transport_t * transport,ismSecurity_t *sContext, char * ltpaToken, int ltpaToken_len)
{
    int rc = 1;
    int lptaprofileexisted=0;

    if(ltpaToken!=NULL && ltpaToken_len>0)
    {
        TRACE(9, "Set header LTPA token: %s\n", ltpaToken);

        /* Determine if the LTPAProfile existed. If not, no need to set the LTPA Token*/
        const char *secprof = NULL;
        if ( transport && transport->listener )
            secprof = transport->listener->secprof;
        if ( secprof ) {
            if ( transGetSecProfile ) {
                ism_secprof_t * secprofptr = (ism_secprof_t *)transGetSecProfile(secprof);
                if ( secprofptr && secprofptr->ltpaprof ) {
                    /* Get secretkey and add in the context */
                    ismLTPAProfile_t * ltpaProfile = getLTPAProfileByName(secprofptr->ltpaprof);
                    if ( ltpaProfile != NULL ) {
                         lptaprofileexisted=1;
                    }
                }
            }
        }

        /* if LTPAProfile doesn't exist for this Endpoint.
         * No need to set the LTPA Token into security context
         */
        if(lptaprofileexisted==1){
            /*Copy Password to Authenticatino Token*/
            ismAuthEvent_t * authent = &sContext->authent;
            authent->token =(ismAuthToken_t *)&sContext->authToken;
            if(ltpaToken_len > authent->token->password_alloc_len){
                if(authent->token->password_inheap)
                    ism_common_free(ism_memory_admin_misc,authent->token->password);
                authent->token->password = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,486),1, ltpaToken_len+1);
                authent->token->password_len=ltpaToken_len;
                authent->token->password_alloc_len=ltpaToken_len;
                authent->token->password_inheap=1;
            }else{
                authent->token->password_len=ltpaToken_len;
            }
            memcpy(authent->token->password, ltpaToken, ltpaToken_len);

            /*Copy IMA_LTPA_AUTH user name*/
            memcpy(authent->token->username, "IMA_LTPA_AUTH", 13 );
            authent->token->username_len=13;

            authent->ltpaTokenSet=1;

            rc = 0;
        }


    }

    return rc;

}

int32_t ism_security_testClientAuthority(
    char *testObject,
    ism_json_parse_t *json,
    concat_alloc_t *output_buffer )
{
    return ISMRC_OK;
}

/**
 * Get the Security Context Transport object
 * @param sContext the Security Context
 * @return the transport object.
 */
XAPI ism_transport_t * ism_security_getTransFromSecContext(ismSecurity_t *sContext) {
    ism_transport_t * transport = NULL;

    if ( sContext && sContext->transport )
        transport = sContext->transport;

    return transport;
}


XAPI int ism_security_updatePoliciesProtocol(void)
{
        int i =0;
        pthread_rwlock_wrlock(&policylock);

        for (i = 0; i < policies->count; i++) {
                ismPolicyRule_t *policy = policies->rules[i];
                processProtoFamily(policy);
        }

        pthread_rwlock_unlock(&policylock);

        return 0;
}

XAPI int ism_security_context_getAllowDurable(ismSecurity_t *sContext)
{
        if (sContext != NULL)
        {
                return sContext->AllowDurable;
        }else {
            return -1;
        }
}

XAPI int ism_security_context_getAllowPersistentMessages(ismSecurity_t *sContext)
{
        if (sContext != NULL)
        {
                return sContext->AllowPersistentMessages;
        }else {
            return -1;
        }
}

XAPI int ism_security_context_isLTPA(ismSecurity_t *sContext) {
    if (sContext->secretKey != NULL)
        return 1;
    return 0;
}

XAPI int ism_security_context_isOAuth(ismSecurity_t *sContext) {
    if (sContext->url != NULL)
        return 1;
    return 0;
}


XAPI int ism_security_context_getCheckGroup(ismSecurity_t *sContext)
{
        if (sContext != NULL)
        {
                return sContext->checkGroup;
        }else {
            return -1;
        }
}

XAPI char * ism_security_context_getOAuthGroup(ismSecurity_t *sContext)
{
        if (sContext != NULL)
        {
                return sContext->oauthGroup;
        } else {
            return NULL;
        }
}

XAPI char * ism_security_context_getOAuthGroupDelimiter(ismSecurity_t *sContext)
{
        if (sContext != NULL)
        {
                if (sContext->authToken.oauth_groupDelimiter != NULL) {
                        return (char *)sContext->authToken.oauth_groupDelimiter;
        }
            }
            return ",";
}

XAPI int ism_security_context_setOAuthGroup(ismSecurity_t *sContext, char *group)
{
        if (sContext != NULL && group != NULL)
        {
                sContext->oauthGroup = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),group);
                return 0;
        } else {
            return -1;
        }
}


XAPI int ism_security_context_isSuperUser(ismSecurity_t *sContext)
{
        if (sContext != NULL && sContext->superUser == 1 )
        {
                return 1;
        }
        return 0;
}

XAPI int ism_security_context_isAdminListener(ismSecurity_t *sContext)
{
    if ( sContext && sContext->transport && sContext->transport->listener && ( sContext->transport->listener->isAdmin == 1 )) {
        return 1;
    }
    return 0;
}

XAPI void ism_security_context_setSuperUser(ismSecurity_t *sContext)
{
    if ( sContext ) {
        sContext->superUser = 1;
    }
}

