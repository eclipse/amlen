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
#include <ismrc.h>
#include <ismutil.h>
#include <ldap.h>
#include <errno.h>
#include <ctype.h>
#include <config.h>
#include <ldaputil.h>
#include <threadpool.h>
#include <pthread.h>
#include <admin.h>
#include <transport.h>
#include <stdint.h>
#include <configInternal.h>

#define WEBUI_SEARCH_BASE "ou=groups,ou=webui,dc=ism.ibm,dc=com"
#define MESSAGING_SEARCH_BASE "ou=groups,ou=messaging,dc=ism.ibm,dc=com"

#define CN_DEFAULT_LEN 64

#define DN_BACKSLASH '\\'
#define SEPARATOR(c) (c == ',' || c == ';' || c == '+')
#define SPACE(c) (c == ' ' || c == '\n')
#define NEEDSESCAPE(c) (c == DN_BACKSLASH || c == '"')
#define B4TYPE  0
#define INTYPE  1
#define B4EQUAL  2
#define B4VALUE  3
#define INVALUE  4
#define INQUOTEDVALUE 5
#define B4SEPARATOR 6

XAPI int auditLogControl;
XAPI ismAuthToken_t * ism_security_getSecurityContextAuthToken(
        ismSecurity_t *sContext);
XAPI ismLDAPConfig_t *ism_security_getEnabledLDAPConfig(void);
XAPI int ism_security_LDAPInitLD(LDAP ** ld);
XAPI int ism_security_getContextCheckGroup(ismSecurity_t *sContext);
XAPI int ism_admin_getLDAPDN(ismLDAPConfig_t *ldapobj, const char* username,
        int username_len, char ** pDN, int * DNLen, bool isGroup, int *dnInHeap);
XAPI int ism_admin_getLDAPDNWithLD(LDAP **ld, ismLDAPConfig_t *ldapobj,
        const char* username, int username_len, char ** pDN, int * DNLen,
        bool isGroup, int *dnInHeap);
XAPI void ism_security_getMemberGroupsInternal(LDAP * ld, char *memberdn,
        ismAuthToken_t * token, int level);
XAPI int ism_admin_ldapExtraLen(char * str, int len, int isFilterStr);
XAPI void ism_admin_ldapEscape(char ** new, char * from, int len,
        int isFilterStr);
XAPI void ism_admin_ldapHexEscape(char ** new, char * from, int len);
XAPI int ism_admin_ldapHexExtraLen(char * str, int len);
XAPI int ism_admin_ldapSearchFilterExtra(char * str);
XAPI void ism_admin_ldapSearchFilterEscape(char ** new, char * from, int len);
XAPI void ism_security_LDAPTermLD(LDAP * ld);
XAPI ism_transport_t * ism_security_getTransFromSecContext(ismSecurity_t *sContext);
XAPI void ism_security_setLDAPGlobalEnv(ismLDAPConfig_t * ldapConfig);
XAPI char * ism_security_context_getOAuthGroup(ismSecurity_t *sContext);
XAPI char * ism_security_context_getOAuthGroupDelimiter(ismSecurity_t *sContext);
XAPI int ism_admin_ldapNeedEscape(char *str, int len);
XAPI ismLDAPConfig_t *ism_security_getLDAPConfig(void);

static void ism_security_invalidateAndCleanAuthCache(void);

extern ism_trclevel_t * ism_defaultTrace;

ismLDAPConfig_t * _ldapConfig;
ismLDAPConfig_t * _localLdapConfig;
int isAuthenticationCacheDisabled = 0;
int isCachingGroupInfoDuringAuth = 0;
int enabledCache = 1;
int cacheTTL = 10;
int enabledGroupCache = 1;
int groupCacheTTL = 300;
ism_time_t ldapCfgObj_Changed_Time = 0;

static const char * g_keystore;
static int g_keystore_len;
char * ldapSSLCertFile;

static pthread_mutex_t authTokenLock;
static pthread_mutex_t dnLock;

ismHashMap * ismAuthCacheTokenMap;
ismHashMap * ismSecurityDNMap;
static int isLDAPUtilInited = 0;

LDAP *getDNLDsession = NULL;
static pthread_mutex_t dnLDsessionLock;

typedef struct ism_groupName {
    char name[4096];
    int len;
    char cname[4096];
    int clen;
    int level;
} ism_groupName_t;

#define FNV_OFFSET_BASIS_32 0x811C9DC5
#define FNV_PRIME_32 0x1000193

/* This function is same as in server_utils/src/map.c file
 * TODO: In next release remove this function from here, get the utils function exported and
 * use the function in this file
 */
uint64_t ism_security_memhash_fnv1a_32(const void * in, size_t *len) {
    uint32_t hash = FNV_OFFSET_BASIS_32;
    uint8_t * in8 = (uint8_t *) in;
    size_t length = *len;

    if (!length) {
        uint8_t b;
        do {
            b = *(in8++);
            hash ^= b;
            hash *= FNV_PRIME_32;
            length++;
        } while (b);
        *len = length;
        return hash;
    }
    if (length == 4) {
        uint32_t ival;
        memcpy(&ival, in, 4); /* Make sure it is aligned */
        hash = ival;
        hash = hash << 7 | hash >> 25; /* rotate left 7 bit */
        return hash / 127;
    }
    if (length == 8) {
        uint64_t lval;
        memcpy(&lval, in, 8); /* Make sure it is aligned */
        uint64_t val = lval;
        val = val << 7 | val >> 57; /* rotate left 7 bits */
        return val / 127;
    }

    if (__builtin_expect((length > 0), 1)) {
        while (length--) {
            hash ^= *(in8++);
            hash *= FNV_PRIME_32;
        }
    }
    return hash;
}

void tolowerDN_ASCII(char * dn) {
    char *p = dn;
    char c;

    while (p && *p != '\0') {
        c = (char) tolower((int) *p);
        *p = c;
        p++;
    }
}

/* Lowercase the dn of a validated UTF-8 String */
char * tolowerDN(const char * dn) {

    int rc = ISMRC_OK;
    char * utf8str;

    if (!dn || *dn == '\0') {
        return NULL;
    }

    /* convert to lower case */
    rc = ism_common_lowerCaseUTF8(&utf8str, dn);
    if (rc)
        return NULL;

    return utf8str;
}

/* remove leading spaces */
void string_strip_leading(char * dn) {
    char *p = NULL;
    if (dn == NULL || *dn == '\0')
        return;

    p = dn;
    while (SPACE(*p))
        p++;
    if (p != dn) {
        int i;
        for (i = 0; p[i] != '\0'; i++)
            dn[i] = p[i];
        dn[i] = p[i];
    }
    return;
}

/* remove training spaces */
void string_strip_trailing(char * dn) {
    int len = 0;

    if (dn == NULL || *dn == '\0')
        return;

    len = strlen(dn);
    while (len > 0)
        if (!SPACE(dn[len - 1]))
            break;
        else if (len > 1 && dn[len - 2] == '\\')
            break;
        else
            --len;

    dn[len] = '\0';
    return;
}

/*
 * dn_normalize - put dn into a canonical format.  the dn is
 * normalized in place, as well as returned.
 */

void dn_normalize(char * dn) {

    char *d, *s = NULL;
    int state, gotesc = 0;
    if ((dn == NULL) || (strlen(dn) == 0))
        return;

    gotesc = 0;
    state = B4TYPE;
    for (d = s = dn; *s; s++) {
        switch (state) {
        case B4TYPE:
            if (!SPACE(*s)) {
                state = INTYPE;
                *d++ = *s;
            }
            break;
        case INTYPE:
            if (*s == '=') {
                state = B4VALUE;
                *d++ = *s;
            } else if (SPACE(*s)) {
                state = B4EQUAL;
            } else {
                *d++ = *s;
            }
            break;
        case B4EQUAL:
            if (*s == '=') {
                state = B4VALUE;
                *d++ = *s;
            } else if (!SPACE(*s)) {
                *d++ = *s;
            }
            break;
        case B4VALUE:
            if (*s == '"') {
                state = INQUOTEDVALUE;
                *d++ = *s;
            } else if (!SPACE(*s)) {
                state = INVALUE;
                *d++ = *s;
            }
            break;
        case INVALUE:
            if (!gotesc && SEPARATOR(*s)) {
                while (SPACE(*(d - 1)))
                    d--;
                state = B4TYPE;
                if (*s == '+') {
                    *d++ = *s;
                    //plus_exists = 1; /* D46345 */
                } else {
                    *d++ = ',';
                }
            } else if (gotesc && !NEEDSESCAPE(*s)) { //47076
                *--d = *s;
                d++;
            } else {
                *d++ = *s;
            }
            break;
        case INQUOTEDVALUE:
            if (!gotesc && *s == '"') {
                state = B4SEPARATOR;
                *d++ = *s;
            } else if (gotesc && !NEEDSESCAPE(*s)) {
                *--d = *s;
                d++;
            } else {
                *d++ = *s;
            }
            break;
        case B4SEPARATOR:
            if (SEPARATOR(*s)) {
                state = B4TYPE;
                /*if ( *s == '+' )
                 plus_exists = 1;*/
                *d++ = *s;
            }
            break;
        default:
            break;
        }
        if (*s == DN_BACKSLASH && gotesc == 0) { // 47076
            gotesc = 1;
        } else {
            gotesc = 0;
        }
    }
    *d = '\0';

    /*if (plus_exists)
     dn = dn_normalize_compound_RDN(dn);*/

    //return ( dn );
}

void ism_security_setLDAPConfig(ismLDAPConfig_t * ldapConfig) {
    pthread_spin_lock(&ldapconfiglock);
    _ldapConfig = ldapConfig;
    ldapCfgObj_Changed_Time = ism_common_currentTimeNanos();
    ism_security_setLDAPGlobalEnv(_ldapConfig);
    pthread_spin_unlock(&ldapconfiglock);

    if (_ldapConfig->EnableCache == true) {
        enabledCache = 1;
    } else {
        enabledCache = 0;
    }
    cacheTTL = _ldapConfig->CacheTTL;
    groupCacheTTL = _ldapConfig->GroupCacheTTL;

    /*LDAP COnfiguration change. CLean the cache*/
    ism_security_invalidateAndCleanAuthCache();

    /* When this function is called during initialisation,
     * the mutex wont have been initialised yet. The auth
     * worker threads will also not have started, and so
     * we shouldn't need to reset this connection, as it
     * wont have been initialised yet.*/
    if (isLDAPUtilInited) {
        pthread_mutex_lock(&dnLDsessionLock);
        /*Terminate GetDNLD object*/
        if (getDNLDsession != NULL) {
            ism_security_LDAPTermLD(getDNLDsession);
            getDNLDsession = NULL;
        }
        pthread_mutex_unlock(&dnLDsessionLock);
    }
}


void ism_security_setLDAPSConfig(ismLDAPConfig_t * ldapConfig) {
	if ( ldapConfig == NULL ) {
		ldapConfig = ism_security_getLDAPConfig();
	}
    if (ldapConfig && ldapConfig->URL && strncmp(ldapConfig->URL, "ldaps", 5) == 0) {
        char *errStr = NULL;
        int allow = LDAP_OPT_X_TLS_DEMAND;
        if ( ldapConfig->CheckServerCert == ismSEC_SERVER_CERT_DISABLE_VERIFY ) {
            allow = LDAP_OPT_X_TLS_ALLOW;
        }

        int rc = ldap_set_option(NULL, LDAP_OPT_X_TLS_REQUIRE_CERT, &allow);
        if (rc != LDAP_SUCCESS) {
            errStr = ldap_err2string(rc);
            TRACE(3, "set LDAP_OPT_X_TLS_REQUIRE_CERT: rc=%d error=%s\n", rc, errStr?errStr:"UNKNOWN");
        }

        if ( ldapConfig->Certificate != NULL ) {
            if ( ldapSSLCertFile == NULL ) {
                g_keystore = ism_common_getStringConfig("LDAPCertificateDir");
                if (!g_keystore)
                    g_keystore = ".";
                g_keystore_len = (int) strlen(g_keystore);
                ldapSSLCertFile = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,181), g_keystore_len + strlen(ldapConfig->Certificate) + 2); /* BEAM suppression: operating on NULL */
                strcpy(ldapSSLCertFile, g_keystore);
                strcat(ldapSSLCertFile, "/");
                strcat(ldapSSLCertFile, ldapConfig->Certificate); /* BEAM suppression: operating on NULL */
                if (ldapConfig->FullCertificate != NULL) {
                	ism_common_free(ism_memory_admin_misc,ldapConfig->FullCertificate);
                }
                ldapConfig->FullCertificate = ldapSSLCertFile; /* BEAM suppression: operating on NULL */
            }
            TRACE(5, "Use CACERTFILE=%s CheckServerCert=%d REQUIRE_CERT=%d\n",
                ldapSSLCertFile?ldapSSLCertFile:"", ldapConfig->CheckServerCert, allow);
            rc = ldap_set_option(NULL, LDAP_OPT_X_TLS_CACERTFILE, (const char*) ldapSSLCertFile);
            if (rc != LDAP_SUCCESS) {
                errStr = ldap_err2string(rc);
                TRACE(3, "set LDAP_OPT_X_TLS_CACERTFILE: rc=%d error=%s\n", rc, errStr?errStr:"UNKNOWN");
            }
        }
    }
}

void ism_security_setLDAPGlobalEnv(ismLDAPConfig_t * ldapConfig) {
    int version = LDAP_VERSION3;
    const char *ldap_server = NULL;

    if (!ldapConfig) {
        TRACE(4, "setLDAPEnv: LDAPCOnfig is NULL.\n");
        return;
    }

    ldap_server = ldapConfig->URL; /* BEAM suppression: operating on NULL */
    if (ldap_server == NULL) {
        ldap_server = "ldap://127.0.0.1/";
    }
    ldap_set_option(NULL, LDAP_OPT_PROTOCOL_VERSION, &version);
    ldap_set_option(NULL, LDAP_OPT_URI, ldap_server);
    TRACE(8, "LDAP Server URL: %s\n", ldap_server);

    //Set Timeout & Time Limit for the Synchronous Operation
    int timeoutval = ldapConfig->Timeout; /* BEAM suppression: operating on NULL */
    ldap_set_option(NULL, LDAP_OPT_TIMELIMIT, &timeoutval);
    struct timeval timeout;
    timeout.tv_sec = ldapConfig->Timeout; /* BEAM suppression: operating on NULL */
    timeout.tv_usec = 0;
    ldap_set_option(NULL, LDAP_OPT_TIMEOUT, &timeout);
    ldap_set_option(NULL, LDAP_OPT_NETWORK_TIMEOUT, &timeout);

    ism_security_setLDAPSConfig(ldapConfig);
}

ismLDAPConfig_t *ism_security_getLDAPConfig(void) {
    if (_ldapConfig) {
        pthread_spin_lock(&_ldapConfig->lock);
        if (_ldapConfig->deleted == false && _ldapConfig->Enabled == true) {
            pthread_spin_unlock(&_ldapConfig->lock);
            return _ldapConfig;
        }
        pthread_spin_unlock(&_ldapConfig->lock);
    }
    pthread_spin_lock(&ldapconfiglock);
    _ldapConfig = ism_security_getEnabledLDAPConfig();
    if (_ldapConfig == NULL || _ldapConfig->Enabled == false) {
        _ldapConfig = _localLdapConfig;
    } else {
        ism_security_setLDAPGlobalEnv(_ldapConfig);
    }

    pthread_spin_unlock(&ldapconfiglock);

    return _ldapConfig;
}

void ism_security_ldapUtilInit(void) {
    if (!isLDAPUtilInited) {
        ismAuthCacheTokenMap = ism_common_createHashMap(2048, HASH_BYTE_ARRAY);
        ismSecurityDNMap = ism_common_createHashMap(512, HASH_STRING);
        pthread_mutex_init(&authTokenLock, 0);
        pthread_mutex_init(&dnLock, 0);
        pthread_mutex_init(&dnLDsessionLock, 0);

        pthread_mutex_lock(&dnLDsessionLock);
        int ldInitRC = ism_security_LDAPInitLD(&getDNLDsession);
        pthread_mutex_unlock(&dnLDsessionLock);
        if (ldInitRC != ISMRC_OK) {
            TRACE(5, "Failed to initialize the getDNLDSession object.\n");
        }
        isLDAPUtilInited = 1;

    }
}

/**
 * Delete the DN from the map
 * @param cn the common name of an user or group
 * @return 0 for the value can't be found or still had references. 1 means that the value is found and freed.
 */
XAPI int ism_security_deleteLDAPDNFromMap(char * cn) {
    int ret = 0;
    char * retDN = NULL;

    if (cn != NULL && ismSecurityDNMap != NULL) {

        pthread_mutex_lock(&dnLock);

        retDN = (char *) ism_common_getHashMapElement(ismSecurityDNMap,
                (void *) cn, 0);
        if (retDN != NULL) {
            /*Check ref count */
            int * ref_p = (int*) retDN;
            if (*ref_p > 1) {
                /*There are still references. Just minus 1 for reference and unlock and return*/
                *ref_p = *ref_p - 1;
                TRACE(8, "The DN reference for %s is decreased to %d\n", cn, *ref_p);
                pthread_mutex_unlock(&dnLock);
                return ret;
            } else {
                /*Remove the map*/
                void * retDN2 = ism_common_removeHashMapElement(ismSecurityDNMap, cn, 0);
                TRACE(8, "Removed the DN for %s. The removed DN: %s.\n", cn, (char * )retDN2? (char * )retDN2: "");
                ret = 1;
                if ( retDN2 != NULL ) ism_common_free(ism_memory_admin_misc,retDN2);

            }
        }
        pthread_mutex_unlock(&dnLock);

    }
    return ret;
}

/**
 * Get the LDAP DN from the map.
 * @param cn the group or user common name or uid
 * @param
 */
char * ism_security_getLDAPDNFromMap(char * cn) {
    char * retDN = NULL;
    if (cn != NULL) {
        pthread_mutex_lock(&dnLock);
        retDN = (char *) ism_common_getHashMapElement(ismSecurityDNMap,
                (void *) cn, 0);
        if (retDN != NULL) {
            retDN += sizeof(int);
        }
        pthread_mutex_unlock(&dnLock);
    }

    return retDN;
}

/**
 * Put the DN into the map for the common name of user or group
 * @param cn the user or group common name or uid
 * @param isGroup 1 for if the cn is group dn. Otherwise false.
 * @return the DN for the user or group
 */
char * ism_security_putLDAPDNToMap(char * cn, int isGroup) {
    char * retDN = NULL;
    int * ref_p = NULL;
    char * retDN_p = NULL;
    if (cn != NULL) {

        ismLDAPConfig_t * ldapConfig = ism_security_getLDAPConfig();

        char * ldapCN = cn;
        int dnInHeap = 0;

        int rc = ISMRC_OK;

        if (isGroup) {
            int dnLen = ldapConfig->GroupDNMaxLen + strlen(cn);
            /* hex escape the full group string if necessary*/
            char *tmpDN = (char *) alloca(dnLen + 1);
            int tmpDNLen = dnLen + 1;
            snprintf(tmpDN, tmpDNLen, "%s=%s,%s", ldapConfig->GroupIdPrefix,
                    ldapCN, ldapConfig->GroupSuffix);
            int hexLen = ism_admin_ldapHexExtraLen(tmpDN, tmpDNLen - 1);
            if (hexLen > 0) {
                int retHexDNLen = tmpDNLen + hexLen;
                /*Malloc the first 4 bytes for ref count and the hex escaped len*/
                retDN = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,183),sizeof(int) + retHexDNLen);
                /*The string will be returned. So increase the ref count.*/
                ref_p = (int*) retDN;
                *ref_p = 1;
                retDN_p = retDN + sizeof(int);
                ism_admin_ldapHexEscape(&retDN_p, tmpDN, dnLen);
                retDN_p[retHexDNLen - 1] = '\0';
            } else {
                /*Malloc the first 4 bytes for ref count and the default len*/
                retDN = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,184),sizeof(int) + tmpDNLen);
                /*The string will be returned. So increase the ref count.*/
                ref_p = (int*) retDN;
                *ref_p = 1;
                retDN_p = retDN + sizeof(int);
                snprintf(retDN_p, tmpDNLen, "%s=%s,%s",
                        ldapConfig->GroupIdPrefix, ldapCN,
                        ldapConfig->GroupSuffix);
            }
        } else {
            /* Hex Escape Special Characters for LDAP CN for userid.
             * Group is already escaped during policy creation time.
             */
            if (ldapConfig->SearchUserDN == false) {
                int cnHexExtraLen = 0;
                int cn_len = strlen(cn);
                int dnLen = ldapConfig->UserDNMaxLen + cn_len;

                char *tmpDN = alloca(dnLen + 1);
                snprintf(tmpDN, dnLen + 1, "%s=%s,%s", ldapConfig->UserIdPrefix,
                        ldapCN, ldapConfig->UserSuffix);
                cnHexExtraLen = ism_admin_ldapHexExtraLen(tmpDN, dnLen);

                if (cnHexExtraLen > 0) {
                    int newsize = cnHexExtraLen + dnLen + 1;
                    retDN = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,185),sizeof(int) + newsize);
                    /*The string will be returned. So increase the ref count.*/
                    ref_p = (int*) retDN;
                    *ref_p = 1;
                    retDN_p = retDN + sizeof(int);
                    ism_admin_ldapHexEscape(&retDN_p, tmpDN, dnLen);
                    retDN_p[newsize - 1] = '\0';
                } else {
                    retDN = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,186),sizeof(int) + dnLen + 1);
                    /*The string will be returned. So increase the ref count.*/
                    ref_p = (int *) retDN;
                    *ref_p = 1;
                    retDN_p = retDN + sizeof(int);
                    snprintf(retDN_p, dnLen + 1, "%s", tmpDN);
                }

            } else {
                int tmpDNLen = 0;
                char * tmpDN = NULL;

                rc = ism_admin_getLDAPDN(ldapConfig, ldapCN, strlen(ldapCN),
                        &tmpDN, &tmpDNLen, false, &dnInHeap);
                if (rc != ISMRC_OK) {
                    TRACE(5,
                            "Failed to obtain the user DN from the Directory Server. CN: %s. RC: %d\n",
                            cn, rc);
                    if (tmpDN != NULL)
                        ism_common_free(ism_memory_admin_misc,tmpDN);
                    return NULL;
                }
                retDN = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,188),sizeof(int) + tmpDNLen + 1);
                /*The string will be returned. So increase the ref count.*/
                ref_p = (int*) retDN;
                *ref_p = 1;
                retDN_p = retDN + sizeof(int);
                snprintf(retDN_p, tmpDNLen + 1, "%s", tmpDN); /* BEAM suppression: passing null object */

            }
        }

        if (rc == ISMRC_OK) {
            pthread_mutex_lock(&dnLock);

            void * mretDN = NULL;

            ism_common_putHashMapElement(ismSecurityDNMap, (void *) cn, 0,
                    (void *) retDN, &mretDN);

            /*Object already existed. Increase ref count*/
            if (mretDN != NULL) {
                *ref_p = *((int *) mretDN) + 1;
                ism_common_free(ism_memory_admin_misc,mretDN);
                mretDN = NULL;
            }

            pthread_mutex_unlock(&dnLock);

            return retDN_p;
        } else {
            TRACE(5, "Failed to set the DN into the map. CN: %s\n", cn);
            if (retDN)
                ism_common_free(ism_memory_admin_misc,retDN);
            return NULL;
        }
    }
    return retDN;

}

/* Free entry of admin action list */
void ism_admin_free(void *data) {
    ism_common_free(ism_memory_admin_misc,data);
}

void ism_security_initAuthToken(ismAuthToken_t * authToken) {
    pthread_spin_init(&authToken->lock, 0);
    ism_common_list_init(&authToken->gCacheList, 0, (void*)ism_admin_free);

}

void ism_security_destroyAuthToken(ismAuthToken_t * token) {
    if (token != NULL) {
        pthread_spin_lock(&token->lock);
        pthread_spin_unlock(&token->lock);
        pthread_spin_destroy(&token->lock);
        ism_common_list_destroy(&token->gCacheList);
        token = NULL;
    }
}
static void ism_security_initAuthCacheToken(ismAuthCacheToken_t * authToken) {
    pthread_spin_init(&authToken->lock, 0);
    ism_common_list_init(&authToken->gCacheList, 0, (void*)ism_admin_free);

}

void ism_security_destroyAuthCacheToken(ismAuthCacheToken_t * token) {
    if (token != NULL) {
        TRACE(8, "Destroy Authentication Cache Token. user: %s\n",
                token->username);
        if (token->username_inheap) {
            ism_common_free(ism_memory_admin_misc,token->username);
        }
        pthread_spin_destroy(&token->lock);
        ism_common_list_destroy(&token->gCacheList);
        ism_common_free(ism_memory_admin_misc,token);
        token = NULL;
    }
}

void ism_security_cleanAuthCache(void) {
    TRACE(9, "Enter cleanAuthCache.\n");
    int serverstate = ism_admin_get_server_state();

    if ((serverstate == ISM_SERVER_RUNNING
            || serverstate == ISM_MESSAGING_STARTED)
            && ismAuthCacheTokenMap != NULL && isLDAPUtilInited == 1) {

        TRACE(8, "Performing Authentication Cache Clean Up Task.\n");
        ismHashMapEntry ** array;
        ismAuthCacheToken_t * token = NULL;
        int i = 0;

        if (ismAuthCacheTokenMap != NULL) {

            if (pthread_mutex_trylock(&authTokenLock) != 0)
                return;

            serverstate = ism_admin_get_server_state();
            /*Ensure that the server state is still running after obtain the lock*/
            if (serverstate != ISM_SERVER_RUNNING
                    && serverstate != ISM_MESSAGING_STARTED) {
                pthread_mutex_unlock(&authTokenLock);
                return;
            }

            array = ism_common_getHashMapEntriesArray(ismAuthCacheTokenMap);

            while (array[i] != ((void*) -1)) {
                token = (ismAuthCacheToken_t *) array[i]->value;
                if (token->authExpireTime > ism_common_currentTimeNanos()) {
                    token =
                            (ismAuthCacheToken_t *) ism_common_removeHashMapElement(
                                    ismAuthCacheTokenMap, token->username,
                                    token->username_len);
                    TRACE(8,
                            "Removed the Cache Authentication Token for user: %s\n",
                            token->username);
                    ism_security_destroyAuthCacheToken(token);
                }
                i++;

                if (i >= 2000)
                    break;
            }
            ism_common_freeHashMapEntriesArray(array);

            pthread_mutex_unlock(&authTokenLock);
        }
    }
    TRACE(9, "Exit cleanAuthCache.\n");
}

static void ism_security_invalidateAndCleanAuthCache(void) {
    TRACE(9, "Enter ism_security_invalidateAndCleanAuthCache.\n");
    int serverstate = ism_admin_get_server_state();

    if ((serverstate == ISM_SERVER_RUNNING
            || serverstate == ISM_MESSAGING_STARTED)
            && ismAuthCacheTokenMap != NULL && isLDAPUtilInited == 1) {

        TRACE(8, "Performing Authentication Cache Clean Up Task.\n");
        ismHashMapEntry ** array;
        ismAuthCacheToken_t * token = NULL;
        int i = 0;

        if (ismAuthCacheTokenMap != NULL) {

            pthread_mutex_lock(&authTokenLock);

            array = ism_common_getHashMapEntriesArray(ismAuthCacheTokenMap);

            while (array[i] != ((void*) -1)) {
                token = (ismAuthCacheToken_t *) array[i]->value;
                token = (ismAuthCacheToken_t *) ism_common_removeHashMapElement(
                        ismAuthCacheTokenMap, token->username,
                        token->username_len);
                TRACE(8,
                        "Removed the Cache Authentication Token for user: %s\n",
                        token->username);
                ism_security_destroyAuthCacheToken(token);
                i++;

                if (i >= 2000)
                    break;
            }
            ism_common_freeHashMapEntriesArray(array);

            pthread_mutex_unlock(&authTokenLock);
        }
    }
    TRACE(9, "Exit ism_security_invalidateAndCleanAuthCache.\n");
}

void ism_security_ldapUtilDestroy(void) {
    if (isLDAPUtilInited) {

        if (ismAuthCacheTokenMap != NULL) {
            ismHashMapEntry ** array;
            ismAuthCacheToken_t * token = NULL;
            int i = 0;

            pthread_mutex_lock(&authTokenLock);
            array = ism_common_getHashMapEntriesArray(ismAuthCacheTokenMap);

            while (array[i] != ((void*) -1)) {
                token = (ismAuthCacheToken_t *) array[i]->value;
                token = (ismAuthCacheToken_t *) ism_common_removeHashMapElement(
                        ismAuthCacheTokenMap, token->username,
                        token->username_len);
                ism_security_destroyAuthCacheToken(token);
                i++;
            }
            ism_common_freeHashMapEntriesArray(array);

            pthread_mutex_unlock(&authTokenLock);

            ism_common_destroyHashMap(ismAuthCacheTokenMap);
            ismAuthCacheTokenMap = NULL;

        }

        if (ismSecurityDNMap != NULL) {
            ismHashMapEntry ** array;
            char * dn = NULL;
            int i = 0;
            array = ism_common_getHashMapEntriesArray(ismSecurityDNMap);
            while (array[i] != ((void*) -1)) {
                dn = (char *) array[i]->value;
                ism_common_free(ism_memory_admin_misc,dn);
                i++;
            }
            ism_common_freeHashMapEntriesArray(array);
            ism_common_destroyHashMap(ismSecurityDNMap);
        }

        pthread_spin_lock(&ldapconfiglock);
        _ldapConfig = NULL;
        pthread_spin_unlock(&ldapconfiglock);

        pthread_mutex_destroy(&authTokenLock);
        pthread_mutex_destroy(&dnLock);
        pthread_mutex_destroy(&dnLDsessionLock);

        // No longer initialized
        isLDAPUtilInited = 0;
    }
}

int ism_security_groupComparator(const void *data1, const void *data2) {
    ism_groupName_t *d1 = (ism_groupName_t *) data1;
    ism_groupName_t *d2 = (ism_groupName_t *) data2;
    int i = d1->len;
    int j = d2->len;
    if (i < j)
        return 1;
    if (i > j)
        return -1;
    return 0;
}

void ism_security_cacheAuthToken(ismAuthToken_t *authToken) {

    pthread_mutex_lock(&authTokenLock);

    ismAuthCacheToken_t * cacheAuthToken =
            (ismAuthCacheToken_t *) ism_common_getHashMapElement(
                    ismAuthCacheTokenMap, (void *) authToken->username,
                    authToken->username_len);

    if (cacheAuthToken == NULL) {
        TRACE(8, "Creating new authentication cache token.\n");
        cacheAuthToken = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,194),1, sizeof(ismAuthCacheToken_t));
        ism_security_initAuthCacheToken(cacheAuthToken);

        cacheAuthToken->username = cacheAuthToken->lusername;
        cacheAuthToken->username_len = cacheAuthToken->username_alloc_len =
                sizeof(cacheAuthToken->lusername);

        if (authToken->username_len > cacheAuthToken->username_alloc_len) {
            if (cacheAuthToken->username_inheap)
                ism_common_free(ism_memory_admin_misc,cacheAuthToken->username);
            cacheAuthToken->username = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,196),authToken->username_len);
            cacheAuthToken->username_len = authToken->username_len;
            cacheAuthToken->username_alloc_len = authToken->username_len;
            cacheAuthToken->username_inheap = 1;
        } else {
            cacheAuthToken->username_len = authToken->username_len;
        }

        memcpy(cacheAuthToken->username, authToken->username,
                authToken->username_len);

        cacheAuthToken->authExpireTime = ism_common_currentTimeNanos()
                + (cacheTTL * SECOND_TO_NANOS);
        cacheAuthToken->hash_code = authToken->hash_code;

        /*Copy the group cache over*/

        if (ism_common_list_size(&authToken->gCacheList) > 0) {
            ism_common_listIterator iter;
            ism_common_list_iter_init(&iter, &authToken->gCacheList);

            while (ism_common_list_iter_hasNext(&iter)) {
                ism_common_list_node * node = ism_common_list_iter_next(&iter);
                ism_groupName_t *igrp = (ism_groupName_t *) node->data;
                if (igrp->len > 4096) {
                    TRACE(5,
                            "Group name %s length exceeded the limit of 4096. Group ignored\n",
                            igrp->name);
                    continue;
                }
                ism_groupName_t *tgrp = (ism_groupName_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,197),1,
                        sizeof(ism_groupName_t));
                memcpy(tgrp->name, igrp->name, igrp->len);
                tgrp->len = igrp->len;
                memcpy(tgrp->cname, igrp->cname, igrp->clen);
                tgrp->clen = igrp->clen;
                tgrp->level = igrp->level;
                if (tgrp != NULL) {
                    ism_common_list_insert_ordered(&cacheAuthToken->gCacheList,
                            (void *) tgrp, ism_security_groupComparator);
                }

            }
            ism_common_list_iter_destroy(&iter);

            cacheAuthToken->gCacheExpireTime = authToken->gCacheExpireTime;

            TRACE(8, "Cache Token: Copy Group Cache: Size: %d\n",
                    ism_common_list_size(&authToken->gCacheList));
        }

        ism_common_putHashMapElement(ismAuthCacheTokenMap,
                (void *) cacheAuthToken->username, cacheAuthToken->username_len,
                (void *) cacheAuthToken, NULL);

    } else {
        cacheAuthToken->authExpireTime = ism_common_currentTimeNanos()
                + (cacheTTL * SECOND_TO_NANOS);
        cacheAuthToken->hash_code = authToken->hash_code;
    }

    /*Put the username into the DN Map*/
    char * tuser_name = (char *) alloca(cacheAuthToken->username_len + 1);
    memcpy(tuser_name, cacheAuthToken->username, cacheAuthToken->username_len);
    tuser_name[cacheAuthToken->username_len] = '\0';
    ism_security_putLDAPDNToMap(tuser_name, 0);

    pthread_mutex_unlock(&authTokenLock);
}

int ism_security_authenticateFromCache(ismAuthToken_t * token,
        uint64_t hash_code) {
    int rc = ISMRC_NotAuthenticated;
    ismAuthCacheToken_t * cacheToken = NULL;
    if (token != NULL) {
        pthread_mutex_lock(&authTokenLock);

        cacheToken = (ismAuthCacheToken_t *) ism_common_getHashMapElement(
                ismAuthCacheTokenMap, (void *) token->username,
                token->username_len);

        if (cacheToken == NULL) {
            pthread_mutex_unlock(&authTokenLock);
            TRACE(8, "The authentication cache token doesn't exist. User: %s\n",
                    token->username);
            return rc;
        }

        if (cacheToken->authExpireTime > ism_common_currentTimeNanos()) {
            TRACE(8, "Cache Token is valid.\n");
            if (cacheToken->hash_code == hash_code) {
                TRACE(8, "authenticated user from cache\n");
                rc = ISMRC_OK;
                /*Update Expiration time*/
                cacheToken->authExpireTime = ism_common_currentTimeNanos()
                        + (cacheTTL * SECOND_TO_NANOS);

                /*If there are groups caching, copy it over*/
                if (ism_common_list_size(&cacheToken->gCacheList) > 0) {
                    ism_common_listIterator iter;
                    ism_common_list_iter_init(&iter, &cacheToken->gCacheList);

                    while (ism_common_list_iter_hasNext(&iter)) {
                        ism_common_list_node * node = ism_common_list_iter_next(
                                &iter);
                        ism_groupName_t *igrp = (ism_groupName_t *) node->data;
                        if (igrp->len > 4096) {
                            TRACE(5,
                                    "Group name %s length exceeded the limit of 4096. Group ignored\n",
                                    igrp->name);
                            continue;
                        }
                        ism_groupName_t *tgrp = (ism_groupName_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,198),1,
                                sizeof(ism_groupName_t));
                        memcpy(tgrp->name, igrp->name, igrp->len);
                        tgrp->len = igrp->len;
                        memcpy(tgrp->cname, igrp->cname, igrp->clen);
                        tgrp->clen = igrp->clen;
                        tgrp->level = igrp->level;
                        if (tgrp != NULL) {
                            pthread_spin_lock(&token->lock);
                            ism_common_list_insert_ordered(&token->gCacheList,
                                    (void *) tgrp,
                                    ism_security_groupComparator);
                            pthread_spin_unlock(&token->lock);
                        }

                    }
                    ism_common_list_iter_destroy(&iter);

                    pthread_spin_lock(&token->lock);
                    token->gCacheExpireTime = cacheToken->gCacheExpireTime;
                    pthread_spin_unlock(&token->lock);
                }
                TRACE(8,
                        "Authenticate From Cache: Copy Group Cache: Size: %d\n",
                        ism_common_list_size(&cacheToken->gCacheList));
                pthread_mutex_unlock(&authTokenLock);
                return rc;
            }
        }

        pthread_mutex_unlock(&authTokenLock);

    }

    return rc;
}
/**
 * Check if the Group is existed in the list
 */
static int isGroupExisted(ism_common_list *groupList, char * groupDN) {

    if (groupList == NULL || groupDN == NULL) {
        return 0;
    }

    ism_common_listIterator iter;
    ism_common_list_iter_init(&iter, groupList);

    while (ism_common_list_iter_hasNext(&iter)) {
        ism_common_list_node * node = ism_common_list_iter_next(&iter);
        char * igrp = (char *) node->data;

        if (!strcmp(igrp, groupDN)) {
            return 1;
        }
    }
    ism_common_list_iter_destroy(&iter);
    return 0;
}

/**
 * Search all the groups that the input member belong to.
 * This function will get the groups from cache if it is available. If not,
 * it will retrieve the group membership from the LDAP Server
 * @return list which contains the groups for the member. NULL if error.
 */
ism_common_list * ism_security_getMemberGroupsFromLDAP(LDAP * ld, char *memberdn, ism_common_list *groupList, int *nestedSearch) {

    int rc = 0;

    LDAPMessage * ldapmsg = NULL;
    LDAPMessage * result;
    ismLDAPConfig_t * ldapConfig = ism_security_getLDAPConfig();

    char * bind_error_dn = NULL;
    char * bind_error_str = NULL;
    int errorcode;
    int needUnbind = 0;

    int len = 2 * DN_DEFAULT_LEN;
    char finalFilter[len];
    char * pFinalFilter = finalFilter;

    pFinalFilter = stpcpy(finalFilter, ldapConfig->GroupMemberIdPrefix);
    stpcpy(pFinalFilter, "=");
    pFinalFilter += 1;

    /* Escape the name before perform the search but only if at base level of nested group search for group names or if the username
     * has not already been escaped i.e. retrieved from the hash map. For usernames we just check for '\\' */
    int memberLen = strlen(memberdn);
    int memberExtra = ism_admin_ldapSearchFilterExtra((char *) memberdn);
    char * memberEx = (char *) memberdn;

    if (groupList->rsrv == 1 && memberExtra > 0) {
        if (strstr(memberdn, "\\") == NULL) {
            int newsize = memberExtra + memberLen + 1;
            memberEx = (char *) alloca(newsize);
            ism_admin_ldapSearchFilterEscape(&memberEx, (char *) memberdn,
                    memberLen);
            memberEx[newsize - 1] = '\0';
        }
    }
    strcpy(pFinalFilter, memberEx);

    TRACE(8, "LDAP Filter: %s\n", finalFilter);

    if (ldapConfig->NestedGroupSearch == true) {
        *nestedSearch = 1;
    }

    if (ld == NULL) {
        rc = ldap_initialize(&ld, ldapConfig->URL);
        if (rc != LDAP_SUCCESS) {
            char * errStr = ldap_err2string(rc);
            TRACE(7, "Couldn't create LDAP session: server=%s error=%s : rc=%d\n",
                    ldapConfig->URL, errStr ? errStr : "UNKNOWN", rc);
            return 0;
        }
        needUnbind = 1;

        ism_security_setLDAPSConfig(ldapConfig);

        char *baseDN = ldapConfig->BindDN;
        struct berval cred;
        cred.bv_val = (ldapConfig->BindPassword == NULL) ?  NULL : (char*) ldapConfig->BindPassword;
        cred.bv_len = (ldapConfig->BindPassword == NULL) ?  0 : strlen(ldapConfig->BindPassword);
        LDAP * myLD = ld;
        rc = ldap_sasl_bind_s(myLD, baseDN, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
        if (rc != LDAP_SUCCESS) {
            char * errStr = ldap_err2string(rc);
            TRACE(7, "LDAP bind server=%s error=%s (rc=%d)\n", ldapConfig->URL, errStr ? errStr : "Unknown", rc);
            if (needUnbind) {
                ldap_unbind_ext_s(myLD, NULL, NULL);
            }
            return 0;
        }
    }

    LDAP * myLD = ld;

    /*If the cache is timeout or invalid. Contact LDAP*/
    //Search Group
    //char *attributes[] = {"dn", NULL};
    char *attrs[] = { LDAP_NO_ATTRS, NULL };
    if ((rc = ldap_search_ext_s(myLD, ldapConfig->GroupSuffix,
            LDAP_SCOPE_SUBTREE, finalFilter, attrs, 0, NULL, NULL, NULL, 0,
            &result)) != LDAP_SUCCESS) {
        char * pszErrStr = ldap_err2string(rc);
        TRACE(2, "Failed to search: %s\n", pszErrStr);
        if (needUnbind) {
            ldap_unbind_ext_s(myLD, NULL, NULL);
        }
        return NULL;;
    }
    int count = ldap_count_entries(myLD, result);

    if (count > 0) {
        ldapmsg = ldap_first_message(myLD, result);
        while (ldapmsg != NULL) {

            ldap_parse_result(myLD, ldapmsg, &errorcode, &bind_error_dn,
                    &bind_error_str, NULL, NULL, 0);

            if (bind_error_dn != NULL) {
                TRACE(2, "LDAP: Result: bind_error_dn: %s\n", bind_error_dn);
                ldap_memfree(bind_error_dn);
            }
            if (bind_error_str != NULL) {
                TRACE(2, "LDAP: Result: bind_error_dn: %s\n", bind_error_str);
                ldap_memfree(bind_error_str);
            }
            char * groupdn = ldap_get_dn(ld, ldapmsg);
            char * utf8str = NULL;

            if (groupdn != NULL && strcmp(groupdn, "")) {
                int extraLen = 0;
                TRACE(8, "Found group level=%d: %s for member %s.\n", groupList->rsrv, groupdn, memberdn);
                int gnlen = strlen(groupdn);
                extraLen = ism_admin_ldapHexExtraLen(groupdn, gnlen);
                if ((gnlen + extraLen) > 4096) {
                    TRACE(5, "Group name %s length exceeded the limit of 4096. Group ignored\n", groupdn);
                } else {
                    ism_groupName_t *gname = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,199),1, sizeof(ism_groupName_t));
                    /* make list of groups normalized, special chars escaped, and in lower case */
                    dn_normalize(groupdn);
                    char *cname = gname->cname;
                    int clen = strlen(groupdn);
                    memcpy(cname, groupdn, clen);
                    gname->clen = clen;
                    utf8str = tolowerDN(groupdn);
                    if (utf8str) {
                        char * hexgname = gname->name;
                        ism_admin_ldapHexEscape(&hexgname, utf8str, strlen(utf8str));
                        gnlen += extraLen;
                        gname->len = gnlen;
                        gname->level = groupList->rsrv;
                        ism_common_list_insert_ordered(groupList, (void *) gname, ism_security_groupComparator);
                        ism_common_free(ism_memory_utils_to_lower, utf8str);
                    }
                }
            }

            if ( groupdn != NULL )
                ldap_memfree(groupdn);

            ldapmsg = ldap_next_message(myLD, ldapmsg);
        }
    }

    ldap_msgfree(result);

    if (needUnbind) {
        ldap_unbind_ext_s(ld, NULL, NULL);
    }

    return groupList;

}

/**
 * Search all the groups that the input member belong to.
 * This function will get the groups from cache if it is available. If not,
 * it will retrieve the group membership from the LDAP Server
 * @return list which contains the groups for the member. NULL if error.
 */
XAPI void ism_security_getMemberGroupsInternal(LDAP * ld, char *memberdn,
        ismAuthToken_t * token, int level) {
    ism_common_list tmpGroupList;
    ism_common_list_init(&tmpGroupList, 0, NULL);

    //Get groups from cache if available. If noT,  request LDAP Server.
    //Also check if it is already timeout
    //Add check if the checkGroup global flag of Security Context is enabled.
    if (token != NULL  && ism_security_getContextCheckGroup(token->sContext) == 1) {

        char * memberdnEx = memberdn;

        level += 1;
        tmpGroupList.rsrv = level;
        int nestedSearch = 0;
        ism_security_getMemberGroupsFromLDAP(ld, memberdnEx, &tmpGroupList, &nestedSearch);

        ism_common_listIterator iter;
        ism_common_list_iter_init(&iter, &tmpGroupList);

        while (ism_common_list_iter_hasNext(&iter)) {
            ism_common_list_node * node = ism_common_list_iter_next(&iter);
            void * igrp = (char *) node->data;

            pthread_spin_lock(&token->lock);
            if (!isGroupExisted(&token->gCacheList, igrp)) {
                ism_common_list_insert_ordered(&token->gCacheList, igrp,
                        ism_security_groupComparator);

                /* For each DN of Groups for this member,
                 * Ensure that nested group is taken into account.
                 */
                pthread_spin_unlock(&token->lock);

                if (nestedSearch == 1) {
                    char *mdn = ((ism_groupName_t *) igrp)->name;
                    if ( mdn ) {
                        ism_security_getMemberGroupsInternal(ld, mdn, token, level);
                    }
                }
            } else {

                /*free this pointer because it has no reference from the token gCacheList.*/
                if (igrp != NULL)
                    ism_common_free(ism_memory_admin_misc,igrp);
                pthread_spin_unlock(&token->lock);
            }

        }
        ism_common_list_iter_destroy(&iter);

    }

    /*Free the resource for this list*/
    ism_common_list_destroy(&tmpGroupList);

}
/**
 * Check member in group from the member group list
 */
static int checkMemberInGroup(ism_common_list *gCacheList, char * polGroupDN) {
    int retcount = 0;
    ism_common_listIterator iter;
    ism_common_list_iter_init(&iter, gCacheList);

    TRACE(9, "SearchGroup: polGroupDN: %s\n", polGroupDN);
    int polGroupDNLen = strlen(polGroupDN);
    char *polGroupEscaped = NULL;
    /* Defect 198596: The Group list cache data is always escaped but the policy group may not be */
    if (ism_admin_ldapNeedEscape(polGroupDN, polGroupDNLen)) {
        int hexLen = ism_admin_ldapHexExtraLen(polGroupDN, polGroupDNLen);
        int newLen = polGroupDNLen + hexLen;
        polGroupEscaped = (char *) alloca(newLen + 1);
        char *hexPtr = polGroupEscaped;
        ism_admin_ldapHexEscape(&hexPtr, polGroupDN, polGroupDNLen);
        hexPtr[newLen] = '\0';
    } else {
        polGroupEscaped = polGroupDN;
    }
    while (ism_common_list_iter_hasNext(&iter)) {
        ism_common_list_node * node = ism_common_list_iter_next(&iter);
        ism_groupName_t *gn = (ism_groupName_t *) node->data;
        char *igrp = gn->name;

        TRACE(8, "SearchGroup: Group from List: %s\n", igrp);

        if (ism_common_match(igrp, polGroupEscaped)) {
            //Found the group which the member belonged to
            TRACE(8, "Found matched Group: %s\n", polGroupDN);
            retcount++;
            break;
        }

    }
    ism_common_list_iter_destroy(&iter);

    return retcount;
}
/**
 * Validate the member in group.
 * This function will get the groups from cache if it is available. If not,
 * it will retrieve the group membership from the LDAP Server
 * @return list which contains the groups for the member. NULL if error.
 */
int ism_security_validateMemberGroupsInternal(ismSecurity_t *sContext,
        char *memberdn, char *polGroupDN) {

    int found = 0;
    int retcount = 0;

    ismAuthToken_t * token = ism_security_getSecurityContextAuthToken(sContext);
    //Get groups from cache if available. If noT,  request LDAP Server.
    //Also check if it is already timeout
    if (token != NULL) {
        pthread_spin_lock(&token->lock);

        if (token->gCacheExpireTime >= ism_common_currentTimeNanos()
                && ism_common_list_size(&token->gCacheList) > 0) {

            retcount = checkMemberInGroup(&token->gCacheList, polGroupDN);

            TRACE(8, "Membership: Got Member Groups from Cache\n");
            found = 1;
        }
        pthread_spin_unlock(&token->lock);
        /*If the cache is timeout or invalid. Contact LDAP*/
        if (!found) {
            /*Clean out the current cache*/
            pthread_spin_lock(&token->lock);
            if (ism_common_list_size(&token->gCacheList) > 0) {
                ism_common_list_destroy(&token->gCacheList);
            }
            pthread_spin_unlock(&token->lock);

            /*Call to get the current group membership*/
            if (memberdn) {
                ism_security_getMemberGroupsInternal(NULL, memberdn, token, 0);
                TRACE(8, "Membership: Got Member Groups LDAP\n");
                //Update the timeout
                pthread_spin_lock(&token->lock);

                token->gCacheExpireTime = ism_common_currentTimeNanos()
                        + (groupCacheTTL * SECOND_TO_NANOS);

                retcount = checkMemberInGroup(&token->gCacheList, polGroupDN);

                pthread_spin_unlock(&token->lock);
            }

        }

    }

    return retcount;

}

/**
 * Check if the member is belonged to the group.
 * This function also handle the case of nested group.
 * @return 0 for not valid. Any other number is valid.
 */
int ism_security_isMemberBelongsToGroup(ismSecurity_t *sContext, char *uid,
        char *group) {
    char * groupDN;
    char * memberDN;

    memberDN = ism_security_getLDAPDNFromMap(uid);
    if (memberDN == NULL) {
        memberDN = ism_security_putLDAPDNToMap(uid, 0);
    }
    groupDN = ism_security_getLDAPDNFromMap(group);
    if (groupDN == NULL) {
        groupDN = ism_security_putLDAPDNToMap(group, 1);
    }
    int result = ism_security_validateMemberGroupsInternal(sContext, memberDN,
            groupDN);

    return result;
}

int ism_security_ldap_authentication(LDAP ** ld, ismAuthToken_t * authobj) {
    int rc = ISMRC_OK;
    const char *username = authobj->username;
    const char *password = authobj->password;
    int password_len = authobj->password_len;
    int username_len = authobj->username_len;

    uint32_t connID = 0;
    char *clientID = NULL;
    char *clientAddr = NULL;
    char *endpoint = NULL;
    ism_transport_t * transport = NULL;
    ism_trclevel_t *trclevel = ism_defaultTrace;

    if (authobj->sContext) {
        transport = ism_security_getTransFromSecContext(authobj->sContext);
        if (transport) {

            /*Check if transport is in closing or closed state. If yes. set error code and return.*/
            if (UNLIKELY(transport->state != ISM_TRANST_Open)) {
                rc = ISMRC_Closed;
                return rc;
            }

            if (transport->trclevel)
                trclevel = transport->trclevel;
            connID = transport->index;
            clientID = (char *) transport->clientID;
            clientAddr = (char *) transport->client_addr;
            if (transport->listener)
                endpoint = (char *) transport->listener->name;
        }
    }

    ismLDAPConfig_t * ldapConfig = ism_security_getLDAPConfig();

    char dnBuf[DN_DEFAULT_LEN];
    char * DN = dnBuf;
    int dn_len = DN_DEFAULT_LEN;
    int dnBuf_inheap = 0;

    char * ldapUserName = (char *) username;
    int ldapUserName_len = username_len;

    /*Hex Escape Username for special characters for LDAP DN*/
    if (ldapConfig->SearchUserDN == false) {
        int ldapHexExtraLen = ism_admin_ldapHexExtraLen((char *) username,
                username_len);

        if (ldapHexExtraLen > 0) {
            ldapUserName_len = ldapHexExtraLen + username_len;
            ldapUserName = (char *) alloca(ldapUserName_len);
            ism_admin_ldapHexEscape(&ldapUserName, (char *) username,
                    username_len);
        }

        /*If username length and length of user suffix and ID greater than
         * the default length (512), Recalculate size.
         */
        if (ldapConfig->UserDNMaxLen + username_len + ldapHexExtraLen
                + 1>DN_DEFAULT_LEN) {
            dn_len = ldapConfig->UserDNMaxLen + username_len + ldapHexExtraLen;
            DN = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,201),dn_len + 1);
            if (DN == NULL) {
                return ISMRC_AllocateError;
            }
            dnBuf_inheap = 1;
        }
    }

    char * pDN = DN;

    if (ldapConfig->SearchUserDN == false) {
        pDN = stpcpy(DN, ldapConfig->UserIdPrefix);
        stpcpy(pDN, "=");
        pDN += 1;
        memcpy(pDN, ldapUserName, ldapUserName_len);
        pDN += ldapUserName_len;
        strcpy(pDN, ",");
        pDN += 1;
        strcpy(pDN, ldapConfig->UserSuffix);
    } else {
        /*Perform Search to get the user DN*/
        rc = ism_admin_getLDAPDNWithLD(ld, ldapConfig, ldapUserName,
                ldapUserName_len, &DN, &dn_len, false, &dnBuf_inheap);
        /*If failed to obtain LDAP, log the error.*/
        if (rc != ISMRC_OK) {
            if (auditLogControl > 0) {
                char errBuffer[256];

                ism_common_getErrorStringByLocale(rc, ism_common_getLocale(),
                        errBuffer, 255);
                LOG(NOTICE, Security, 6107, "%u%-s%-s%-s%-s%-s%d",
                        "User authentication failed: ConnectionID={0}, UserID={1}, Endpoint={2}, ClientID={3}, ClientAddress={4}, Error={5}, RC={6}.",
                        connID, username, endpoint ? endpoint : " ",
                        clientID ? clientID : " ",
                        clientAddr ? clientAddr : " ", errBuffer, rc);
            }
            if (dnBuf_inheap)
                ism_common_free(ism_memory_admin_misc,DN);
            return rc;
        }
    }

    TRACEL(8, trclevel, "ism_security_ldap_authentication: UserDN: %s.\n", DN);
    TRACE(8, "ldaps config items: Cert=%s CheckServerCert=%d\n", ldapConfig->FullCertificate?ldapConfig->FullCertificate:"", ldapConfig->CheckServerCert);

    struct berval *servercredp;
    struct berval cred;
    cred.bv_val = (char*) password;
    cred.bv_len = password_len;
    rc = ldap_sasl_bind_s(*ld, DN, LDAP_SASL_SIMPLE, &cred, NULL, NULL,
            &servercredp);

    if (rc != LDAP_SUCCESS) {
        char * errStr = ldap_err2string(rc);
        if (auditLogControl > 0) {
            LOG(NOTICE, Security, 6107, "%u%-s%-s%-s%-s%-s%d",
                    "User authentication failed: ConnectionID={0}, UserID={1}, Endpoint={2}, ClientID={3}, ClientAddress={4}, Error={5}, RC={6}.",
                    connID, username, endpoint ? endpoint : " ",
                    clientID ? clientID : " ", clientAddr ? clientAddr : " ",
                    errStr ? errStr : " ", rc);
        }

        /*If failed to connect to server (RC=-1).
         * Try to Re-initialize the LD object.
         */
        if (rc == -1) {
            ism_security_LDAPTermLD(*ld);
            TRACEL(5, trclevel, "Reinitialize the ld object.\n");
            int ldInitRC = ism_security_LDAPInitLD(ld);
            if (ldInitRC != ISMRC_OK) {
                TRACEL(5, trclevel, "Failed to reinitialize the ld object.\n");
                return ldInitRC;
            }
            ism_security_setLDAPSConfig(ldapConfig);
            /*Trying to bind one more time. If failed. Return errror*/
            rc = ldap_sasl_bind_s(*ld, DN, LDAP_SASL_SIMPLE, &cred, NULL, NULL,
                    &servercredp);
            if (rc != LDAP_SUCCESS) {
                errStr = ldap_err2string(rc);
                if (auditLogControl > 0) {
                    LOG(NOTICE, Security, 6107, "%u%-s%-s%-s%-s%-s%d",
                            "User authentication failed: ConnectionID={0}, UserID={1}, Endpoint={2}, ClientID={3}, ClientAddress={4}, Error={5}, RC={6}.",
                            connID, username, endpoint ? endpoint : " ",
                            clientID ? clientID : " ",
                            clientAddr ? clientAddr : " ",
                            errStr ? errStr : " ", rc);
                }
            }
        }

        if (rc != LDAP_SUCCESS) {
            if (dnBuf_inheap)
                ism_common_free(ism_memory_admin_misc,DN);
            return ISMRC_Error;
        }
    }
    if (auditLogControl > 1) {
        LOG(INFO, Security, 6108, "%u%-s%-s%-s%-s",
                "User authentication succeeded: ConnectionID={0}, UserID={1}, Endpoint={2}, ClientID={3}, ClientAddress={4}.",
                connID, username, endpoint ? endpoint : " ",
                clientID ? clientID : " ", clientAddr ? clientAddr : " ");
    }

    /*Pull the Group Membership */
    ism_security_getMemberGroupsInternal(*ld, DN, authobj, 0);

    pthread_spin_lock(&authobj->lock);
    authobj->gCacheExpireTime = ism_common_currentTimeNanos()
            + (groupCacheTTL * SECOND_TO_NANOS);
    pthread_spin_unlock(&authobj->lock);

    if (dnBuf_inheap)
        ism_common_free(ism_memory_admin_misc,DN);

    return rc;
}
/**
 * Get the LDAP ID Prefix
 */
int ism_security_getLDAPIdPrefix(char * idMap, char *idPrefix) {
    if (idMap == NULL || idPrefix == NULL)
        return 1;

    char *optnexttoken = NULL;

    int len = strlen(idMap);
    char *ioption = (char *) alloca(len + 1);
    memcpy(ioption, idMap, len);
    ioption[len] = 0;

    strtok_r(ioption, ":", &optnexttoken);

    if (optnexttoken != NULL && strcmp(optnexttoken, "")) {
        strcpy(idPrefix, optnexttoken);
    } else {
        strcpy(idPrefix, idMap);
    }

    return 0;
}

/*
 * Validate the LDAP configuration object.
 */
int ism_security_validateLDAPConfig(ismLDAPConfig_t *ldapobj, int isVerify, int newCert, int tryCount) {

    LDAP * ld = NULL;
    int rc = ISMRC_OK;
    char * tmpldapSSLCertFile = NULL;
    int ldBound = 0;

    if (ldapobj->URL == NULL) {
        /* If Enabled is false, no need to validate the object */
        if (ldapobj->Enabled == false && isVerify == 0) {
            goto VALIDATEEND;
        }
        ism_common_setError(ISMRC_InvalidLDAPURL);
        return ISMRC_InvalidLDAPURL;
    }

    const char *ldap_server = ldapobj->URL;
    int version = LDAP_VERSION3;

    if (strncmp(ldapobj->URL, "ldaps", 5) == 0 &&
        ldapobj->CheckServerCert == ismSEC_SERVER_CERT_TRUST_STORE ) {

        if (ldapobj->Certificate == NULL) {
            ldapobj->Certificate = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"ldap.pem");
        }

        const char * keystore = ism_common_getStringConfig("LDAPCertificateDir");
        if (newCert) {
            keystore = IMA_SVR_DATA_PATH "/userfiles";
        }
        if (!keystore)
            keystore = ".";
        int keystore_len = (int) strlen(keystore);
        tmpldapSSLCertFile = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,205),keystore_len + strlen(ldapobj->Certificate) + 2);
        strcpy(tmpldapSSLCertFile, keystore);
        strcat(tmpldapSSLCertFile, "/");
        strcat(tmpldapSSLCertFile, ldapobj->Certificate);
        TRACE(5, "CA Certificate Path: %s\n", tmpldapSSLCertFile);

        /* Validate the Certificate Existence */
        if (ism_common_validate_PEM(tmpldapSSLCertFile)) {
            rc = ISMRC_InvalidLDAPCert;
            ism_common_setError(rc);
            goto VALIDATEEND;
        }
    }

    /* If Enabled is false, no need to validate the object */
    if (ldapobj->Enabled == false && isVerify == 0) {
        goto VALIDATEEND;
    }

    int timeoutval = ldapobj->Timeout;

    /* Retry within 1 sec.
     * in case server fails to bind due to the LDAP server is not up yet.
     */
    if (tryCount == 0)
        tryCount = 10;

    int k = 0;
    char * errStr = NULL;
    for (k = 0; k < tryCount; k++) {

        rc = ldap_initialize(&ld, ldap_server);
        if (rc != LDAP_SUCCESS) {
            errStr = ldap_err2string(rc);
            TRACE(7, "Couldn't create LDAP session: error=%s : rc=%d\n",
                    errStr ? errStr : "UNKNOWN", rc);
            rc = ISMRC_InvalidLDAPURL;
            ism_common_setError(rc);
            goto VALIDATEEND;
        }

        struct timeval timeout;
        timeout.tv_sec = ldapobj->Timeout;
        timeout.tv_usec = 0;

        ldap_set_option(ld, LDAP_OPT_TIMELIMIT, &timeoutval);
        ldap_set_option(ld, LDAP_OPT_TIMEOUT, &timeout);
        ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &timeout);
        ldap_set_option(ld, LDAP_OPT_URI, ldap_server);
        ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);

        if (ldapobj->URL && strncmp(ldapobj->URL, "ldaps", 5) == 0) { /* BEAM suppression: operating on NULL */
            int newCtx = 0;
            int allow = LDAP_OPT_X_TLS_DEMAND;
            if ( ldapobj->CheckServerCert == ismSEC_SERVER_CERT_DISABLE_VERIFY ) {
                allow = LDAP_OPT_X_TLS_ALLOW;
            }

            TRACE(5, "Use CACERTFILE=%s CheckServerCert=%d REQUIRE_CERT=%d\n",
                tmpldapSSLCertFile?tmpldapSSLCertFile:"", ldapobj->CheckServerCert, allow);

            rc = ldap_set_option(ld, LDAP_OPT_X_TLS_REQUIRE_CERT, &allow);
            if (rc != LDAP_SUCCESS) {
                errStr = ldap_err2string(rc);
                TRACE(3, "set LDAP_OPT_X_TLS_REQUIRE_CERT: rc=%d error=%s\n", rc, errStr?errStr:"UNKNOWN");
            }

            if (ldapobj->Certificate != NULL) {
                rc = ldap_set_option(ld, LDAP_OPT_X_TLS_CACERTFILE, (const char*) tmpldapSSLCertFile);
                if (rc != LDAP_SUCCESS) {
                    errStr = ldap_err2string(rc);
                    TRACE(3, "set LDAP_OPT_X_TLS_CACERTFILE: rc=%d error=%s\n", rc, errStr?errStr:"UNKNOWN");
                }
            }

            rc = ldap_set_option(ld, LDAP_OPT_X_TLS_NEWCTX, &newCtx);
            if (rc != LDAP_SUCCESS) {
                errStr = ldap_err2string(rc);
                TRACE(3, "set LDAP_OPT_X_TLS_NEWCTX: rc=%d error=%s\n", rc, errStr?errStr:"UNKNOWN");
            }
        }


        /* Set BindDN and BIND Password for connection binding */
        struct berval cred;
        cred.bv_val = (ldapobj->BindPassword == NULL) ?  NULL : (char*) ldapobj->BindPassword;
        cred.bv_len = (ldapobj->BindPassword == NULL) ?  0 : strlen(ldapobj->BindPassword);

        rc = ldap_sasl_bind_s(ld, ldapobj->BindDN, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
        if (rc != LDAP_SUCCESS) {
            errStr = ldap_err2string(rc);
            TRACE(5, "Failed to bind with LDAP: error=%s : rc=%d\n", errStr ? errStr : "UNKNOWN", rc);

            ldap_destroy(ld);
            ld = NULL;

            if (rc == -1 && k < 9) {
                /* wait for some time - network services may not be fully functional yet. 1 sec*/
                TRACE(6,
                        "Wait for 1 second before next retry of ldap bind. Count=%d\n",
                        k);
                ism_common_sleep(1000000);
                continue;
            }

            ism_common_setErrorData(ISMRC_FailedToBindLDAP, "%s%d", errStr, rc);
            rc = ISMRC_FailedToBindLDAP;
            goto VALIDATEEND;
        } else {
            ldBound = 1;
            TRACE(6, "Validate LDAP: ldap_sasl_bind succeeded\n");
        }

        if (ldapobj->FullCertificate && tmpldapSSLCertFile) {
            ism_common_free(ism_memory_admin_misc,ldapobj->FullCertificate);
            ldapobj->FullCertificate = tmpldapSSLCertFile;
        }

        /* Validate BaseDN */
        if (ldapobj->BaseDN) {
            char *attrs[] = { LDAP_NO_ATTRS, NULL };
            LDAPMessage *result = NULL;
            if ((rc = ldap_search_ext_s(ld, ldapobj->BaseDN, LDAP_SCOPE_BASE,
                    NULL, attrs, 0, NULL, NULL, NULL, 0, &result))
                    != LDAP_SUCCESS) {
                char * pszErrStr = ldap_err2string(rc);
                TRACE(2, "Invalid BaseDN specified: %s, error: %s\n",
                        ldapobj->BaseDN ? ldapobj->BaseDN : "", pszErrStr);
                rc = ISMRC_InvalidBaseDN;
                ism_common_setErrorData(rc, "%s%s", ldapobj->BaseDN, pszErrStr);
                goto VALIDATEEND;
            }

            if (result) {
                ldap_msgfree(result);
            }
        }
        break;
    }

    VALIDATEEND:

    if (rc != ISMRC_OK) {
        ism_common_free(ism_memory_admin_misc,tmpldapSSLCertFile);
        if (rc == -1) {
            ism_common_setErrorData(ISMRC_FailedToBindLDAP, "%s%d", errStr, rc);
            rc = ISMRC_FailedToBindLDAP;
        }
    }

    if (ldBound == 1)
        ldap_destroy(ld);

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

int ism_security_submitLDAPEvent(ismAuthEvent_t * authent) {

    ism_worker_t * worker = NULL;
    int count = 0, worker_valid = 0;
    /*
     * Insert the auth item at the tail of the auth queue
     */

    for (count = 0; count < ism_security_getWorkerCount(); count++) {
        worker = ism_security_getWorker(authent->ltpaAuth);

        pthread_mutex_lock(&worker->authLock);

        if (worker->status == WORKER_STATUS_ACTIVE) {
            pthread_mutex_unlock(&worker->authLock);
            worker_valid = 1;
            break;
        }
        pthread_mutex_unlock(&worker->authLock);
    }

    if (worker_valid == 1) {
        pthread_mutex_lock(&worker->authLock);
        if (worker->authTail) {
            worker->authTail->next = authent;
            worker->authTail = authent;
        } else {
            worker->authTail = authent;
            worker->authHead = authent;
        }

        ism_transport_t *tport = NULL;
        ism_trclevel_t *trclevel = ism_defaultTrace;
        if (authent->token && authent->token->sContext) {
            tport = ism_security_getTransFromSecContext(
                    authent->token->sContext);
            if (tport && tport->trclevel)
                trclevel = tport->trclevel;
        }

        TRACEL(8, trclevel, "Scheduled a job: WorkerID: %d Status: %d\n",
                worker->id, worker->status);
        pthread_cond_signal(&worker->authCond);
        pthread_mutex_unlock(&worker->authLock);
        return 0;
    }

    return 1;

}

XAPI int ism_admin_getLDAPDNWithLD(LDAP **ld, ismLDAPConfig_t *ldapobj,
        const char* name, int name_len, char **pDN, int * DNLen, bool isGroup,
        int *dnInHeap) {
    LDAPMessage *pMsg = NULL, *entry = NULL;
    int err, count;
    char efilter[1024], *p;
    int ret = ISMRC_OK;

    if (ld != NULL && *ld != NULL) {

        do {

            TRACE(8, "Binding ldsession session\n");
            struct berval cred;
            cred.bv_val =
                    (ldapobj->BindPassword == NULL) ?
                            NULL : (char*) ldapobj->BindPassword;
            cred.bv_len =
                    (ldapobj->BindPassword == NULL) ?
                            0 : strlen(ldapobj->BindPassword);
            err = ldap_sasl_bind_s(*ld, ldapobj->BindDN, LDAP_SASL_SIMPLE,
                    &cred, NULL, NULL, NULL);

            if (err != LDAP_SUCCESS) {
                /*If failed to connect to server (RC=-1).
                 * Try to Re-initialize the LD object.
                 */
                if (err == -1) {

                    ism_security_LDAPTermLD(*ld);
                    TRACE(9, "Reinitialize the ld object.\n");
                    int ldInitRC = ism_security_LDAPInitLD(ld);

                    if (ldInitRC != ISMRC_OK) {
                        TRACE(5, "Failed to initialize the ld object.\n");
                        *ld = NULL;
                        return ldInitRC;
                    }

                    ism_security_setLDAPSConfig(ldapobj);

                    /*Trying to bind one more time. If failed. Return error*/
                    err = ldap_sasl_bind_s(*ld, ldapobj->BindDN,
                            LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
                }
                if (err != LDAP_SUCCESS) {
                    char * pszErrStr = ldap_err2string(err);
                    TRACE(8, "Could not bind to ldap server (%d): %s", err,
                            (pszErrStr ? pszErrStr : ""));
                    ism_common_setErrorData(ISMRC_FailedToBindLDAP, "%s%d",
                            pszErrStr, err);
                    ret = ISMRC_FailedToBindLDAP;
                    break;
                }
            }

            /*Escape the name before perform the search*/
            int nameExtra = ism_admin_ldapSearchFilterExtra((char *) name);
            char * nameEx = (char *) name;

            if (nameExtra > 0) {
                int newsize = nameExtra + name_len + 1;
                nameEx = (char *) alloca(newsize);
                ism_admin_ldapSearchFilterEscape(&nameEx, (char *) name,
                        name_len);
                nameEx[newsize - 1] = '\0';
            }

            /*
             * Search for the user in the md5 directories.
             */
            if ((err =
                    snprintf(efilter, 1024, "(%s=%s)",
                            (isGroup==true)?ldapobj->GroupIdPrefix:ldapobj->UserIdPrefix,
                            nameEx)) < 0) {
                ret = ISMRC_AllocateError;
                break;
            }

            if ((err = ldap_search_ext_s(*ld, ldapobj->BaseDN,
                    LDAP_SCOPE_SUBTREE, efilter, NULL, 0, NULL, NULL, NULL, 0,
                    &pMsg)) != LDAP_SUCCESS) {
                char * pszErrStr = ldap_err2string(err);
                TRACE(8, "ldap_search_ext_s() error: %s",
                        (pszErrStr ? pszErrStr : ""));
                ret = ISMRC_FailedToObtainLDAPDN;
                break;
            }

            if (pMsg == NULL) {
                TRACE(8, "No LDAP entry found for %s", name);
                ret = ISMRC_FailedToObtainLDAPDN;
                break;
            }

            /*
             * Count the number of entries found in our search.  We want only one.
             * So we only look at the first entry returned, after suitable checking,
             * of course.  We also do the ldsession connection to get the dn record.
             */
            count = ldap_count_entries(*ld, pMsg);
            if (count <= 0 || count > 1) {
                if (count == 0) {
                    TRACE(8, "No records found for %s", name);
                    ret = ISMRC_FailedToObtainLDAPDN;
                } else {
                    TRACE(8, "Invalid number of records, %d, found for %s",
                            count, name);
                    ret = ISMRC_FailedToObtainLDAPDN;
                }
                break;
            }

            if ((entry = ldap_first_entry(*ld, pMsg)) == NULL) {
                TRACE(8, "Error getting ldap entry");
                ret = ISMRC_FailedToObtainLDAPDN;
                break;
            }

            TRACE(8, "Doing ldap_get_dn(): ldsession: %lx, entry: %lx",
                    (uint64_t )*ld, (uint64_t )entry);
            if ((p = ldap_get_dn(*ld, entry)) == NULL) {
                TRACE(8, "Error getting ldap dn record");
                ret = ISMRC_FailedToObtainLDAPDN;
                break;
            }

            if (p != NULL) {

                int pLen = strlen(p);

                if (pLen < *DNLen) {
                    strcpy(*pDN, p);
                } else {
                    *pDN = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,208),pLen + 1);

                    if (*pDN == NULL) {
                        TRACE(8, "Error getting ldap dn record");
                        ret = ISMRC_FailedToObtainLDAPDN;
                    } else {
                        strcpy(*pDN, p);
                        *dnInHeap = 1;
                        *DNLen = pLen;
                    }
                    break;
                }

            }

        } while (0);

        if (pMsg) {
            ldap_msgfree(pMsg);
        }

    } else {
        /*Set the return code.*/
        ret = ISMRC_InvalidLDAP;
    }

    return ret;
}

/**
 * Assume that the name is already escaped for the LDAP special characters.
 */
XAPI int ism_admin_getLDAPDN(ismLDAPConfig_t *ldapobj, const char* name,
        int name_len, char **pDN, int * DNLen, bool isGroup, int *dnInHeap) {
    TRACE(6, "Using shared getDNLDSession object to look up DN.\n");
    int rc = ISMRC_OK;
    /* lock the shared LDAP session */
    pthread_mutex_lock(&dnLDsessionLock);

    /**
     * Try to initialize if getDNLDsession is NULL
     */
    if (getDNLDsession == NULL) {
        int ldInitRC = ism_security_LDAPInitLD(&getDNLDsession);
        if (ldInitRC != ISMRC_OK) {
            TRACE(5, "Failed to initialize the getDNLDSession object.\n");
            getDNLDsession = NULL;
            pthread_mutex_unlock(&dnLDsessionLock);
            return ldInitRC;
        }
        ism_security_setLDAPSConfig(ldapobj);
    }

    // call into the 'real' getLDAPDN function with the shared LDAP connection
    rc = ism_admin_getLDAPDNWithLD(&getDNLDsession, ldapobj, name, name_len,
            pDN, DNLen, isGroup, dnInHeap);

    pthread_mutex_unlock(&dnLDsessionLock);
    return rc;
}

/* Simple check to see if a backslash is present indicating some form of escape (hex or regular) */
XAPI int ism_admin_ldapNeedEscape(char *str, int len) {
    int needEscape = 1;

    for (int i = 0; i < len; i++) {
        uint8_t ch = (uint8_t) *str++;
        if (ch == '\\') {
            needEscape = 0;
            break;
        }
    }

    return needEscape;
}
/*
 * Compute the extra length needed for JSON escapes of a string.
 * Control characters and the quote and backslash are escaped.
 * No mulitbyte characters are escaped.
 */
XAPI int ism_admin_ldapExtraLen(char * str, int len, int isFilterStr) {
    int extra = 0;
    int i;
    for (i = 0; i < len; i++) {
        uint8_t ch = (uint8_t) *str++;
        if (ch >= ' ') { /* Normal characters */
            if (isFilterStr == 1) {
                /*
                 * If this is for filter string, only count escape backsplash
                 * Assumed other characters had been escaped to backsplash and hex
                 * for special chacters.
                 */
                if (ch == '\\')
                    extra++;
            } else {
                if (ch == '"' || ch == '\\' || ch == ',' || ch == '#'
                        || ch == '+' || ch == '<' || ch == '>' || ch == ';'
                        || ch == ' ')
                    extra++;
            }
        } else
            return -1;
    }
    return extra;
}

/*
 * Copy a byte array with JSON escapes.
 * Escape the doublequote, backslash, and all control characters.
 * Do not escape any multibyte character.
 */
XAPI void ism_admin_ldapEscape(char ** new, char * from, int len,
        int isFilterStr) {
    int i;

    char *to = *new;
    for (i = 0; i < len; i++) {
        uint8_t ch = (uint8_t) *from++;
        if (ch >= ' ') {
            if (isFilterStr == 1) {
                /*
                 * If this is for filter string, only escape backsplash
                 * Assumed other characters had been escaped to backsplash and hex
                 * for special chacters.
                 */
                if (ch == '\\')
                    *to++ = '\\';

            } else {
                if (ch == '"' || ch == '\\' || ch == ',' || ch == '#'
                        || ch == '+' || ch == '<' || ch == '>' || ch == ';'
                        || ch == ' ')
                    *to++ = '\\';
            }
            *to++ = ch;
        }
    }

}

static char hexDigit(unsigned n) {
    if (n < 10) {
        return n + '0';
    } else {
        return (n - 10) + 'A';
    }
}

/*
 * Compute the extra length needed for JSON escapes of a string.
 * Control characters and the quote and backslash are escaped.
 * No mulitbyte characters are escaped.
 */
XAPI int ism_admin_ldapHexExtraLen(char * str, int len) {
    int extra = 0;
    int i;
    for (i = 0; i < len; i++) {
        uint8_t ch = (uint8_t) *str++;
        if (ch >= ' ') { /* Normal characters */
            if (ch == '"' || ch == '\\' || ch == ',' || ch == '#' || ch == '+'
                    || ch == '<' || ch == '>' || ch == ';' || ch == ' ')
                extra += 2;
        } else
            return -1;
    }
    return extra;
}

/*
 * Copy a byte array with JSON escapes.
 * Escape the doublequote, backslash, and all control characters.
 * Do not escape any multibyte character.
 */
XAPI void ism_admin_ldapHexEscape(char ** new, char * from, int len) {
    int i;

    char *to = *new;
    for (i = 0; i < len; i++) {
        uint8_t ch = (uint8_t) *from++;
        if (ch >= ' ') {
            if (ch == '"' || ch == '\\' || ch == ',' || ch == '#' || ch == '+'
                    || ch == '<' || ch == '>' || ch == ';' || ch == ' ') {
                *to++ = '\\';
                *to++ = hexDigit(ch / 0x10);
                *to++ = hexDigit(ch % 0x10);

            } else {
                *to++ = ch;
            }
        }
    }
}

/*
 * Computer the lenghth for Escaping special characters
 * for search filter (RFC 4515)
 */
XAPI int ism_admin_ldapSearchFilterExtra(char * str) {
    int extra = 0;
    while (*str) {
        uint8_t ch = (uint8_t) *str++;
        if (ch >= ' ') { /* Normal characters */
            if (ch == '!' || ch == '&' || ch == '*' || ch == ':' || ch == '|'
                    || ch == '~' || ch == '(' || ch == ')' || ch == '\\')
                extra += 2;
        } else
            return -1;
    }
    return extra;
}

/*
 * Escaping special characters for search filter (RFC 4515).
 * The value will in the splash and 2 digits hex
 */
XAPI void ism_admin_ldapSearchFilterEscape(char ** new, char * from, int len) {
    int i;

    char *to = *new;
    for (i = 0; i < len; i++) {
        uint8_t ch = (uint8_t) *from++;
        if (ch >= ' ') {
            if (ch == '!' || ch == '&' || ch == '*' || ch == ':' || ch == '|'
                    || ch == '~' || ch == '(' || ch == ')' || ch == '\\') {
                *to++ = '\\';
                *to++ = hexDigit(ch / 0x10);
                *to++ = hexDigit(ch % 0x10);
            } else {
                *to++ = ch;
            }
        }
    }
    *to = '\0';
}

/* Escape string */
static char *escapeString(char *str, int noCaseCheck, int *hexLen) {
    int strLen = 0;
    int strHexLen = 0;
    char *hexStr = NULL;
    char *hexStrPtr = NULL;
    char *tmpstr = NULL;

    if (str) {
        if ( noCaseCheck == 1 ) {
            tmpstr = tolowerDN(str);
        } else {
            tmpstr = str;
        }

        strLen = strlen(tmpstr);
        strHexLen = ism_admin_ldapHexExtraLen(tmpstr, strLen);
        if (strHexLen > 0) {
            hexStr = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,209),1, (strLen + strHexLen + 1));
            hexStrPtr = hexStr;
            ism_admin_ldapHexEscape(&hexStrPtr, tmpstr, strLen);
        }

        *hexLen = strHexLen;
        if ( noCaseCheck == 1 && tmpstr ) {
        	//tolowerDN will allocate a new string with lower case so free it
        	ism_common_free(ism_memory_utils_to_lower, tmpstr);
        }
    } else {
        *hexLen = 0;
    }

    return hexStr;
}

/* Get groupname from DN, and validate destination */
int ism_security_getGroupAndValidateDestination(char *uid, char *igrp, char * grpSuffix, int grpSuffixHexLen, char * hexGrpPtr, const char *objectName,
        const char *destination, ima_transport_info_t *tport, int noCaseCheck)
{
    char *groupname = NULL;
    char *groupID[1];
    int groupCount = 0;
    int destCheck = 1;
        int normalizeIgrp = 1;

    /* sanity check */
    if ( !igrp || *igrp == '\0' ) {
        TRACE(3, "Group entry in group cache is NULL or empty\n");
        return destCheck;
    }

    if (strchr(igrp, '\\')) {
        TRACE(9, "No need to escape GroupID entry: %s\n", igrp);
        normalizeIgrp = 0;
    } else {
        TRACE(9, "Escape GroupID entry: %s\n", igrp);
    }

    /* if groupIDMapHasStar - skip to first = sign */
    igrp = strstr(igrp, "=");
    if (igrp == NULL) {
        /* invalid group entry in cache */
        TRACE(2,     "Group entry is invalid in cache (missing =)\n");
        return destCheck;
    }

    igrp = igrp + 1;

    char *grpSuffixPos = NULL;
    char *igrpHex = NULL;

    if (grpSuffix) {
        if (grpSuffixHexLen > 0) {
            /* check if we need to normlize igrp */
            if ( normalizeIgrp == 1 ) {
                /* normalize igrp before trying to get group suffix position */
                int igrpHexLen = 0;
                igrpHex = escapeString(igrp, 1, &igrpHexLen);
                TRACE(9, "Escaped (len=%d) GroupID Entry: %s\n", igrpHexLen, igrpHex?igrpHex:"");
                if (igrpHex) {
                    grpSuffixPos = strstr(igrpHex, hexGrpPtr);
                } else {
                    grpSuffixPos = strstr(igrp, hexGrpPtr);
                }
            } else {
                grpSuffixPos = strstr(igrp, hexGrpPtr);
            }

            /* subtract the two hex chars representing the comma */
            if (grpSuffixPos) grpSuffixPos = grpSuffixPos - 2;
        } else {
            grpSuffixPos = strstr(igrp, grpSuffix);
        }
    }

    if (!grpSuffixPos) {
        groupname = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),igrp);
    } else {
        int groupLen = 0;
        if ( igrpHex ) {
            groupLen = grpSuffixPos - igrpHex - 1; /* subtract the comma if not escaped or the backslash if so */
            groupname = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,210),groupLen + 1);
            memcpy(groupname, igrp, groupLen);
            groupname[groupLen] = 0;

        } else {
            groupLen = grpSuffixPos - igrp - 1; /* subtract the comma if not escaped or the backslash if so */
            groupname = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,211),groupLen + 1);
            memcpy(groupname, igrp, groupLen);
            groupname[groupLen] = 0;
        }
    }

    if ( igrpHex ) ism_common_free(ism_memory_admin_misc,igrpHex);

    TRACE(9, "GroupName (grpSuffixPos=%s) from GroupID entry: %s\n", grpSuffixPos, groupname);

    groupID[groupCount] = groupname;

    destCheck = ism_common_matchWithVars(objectName, destination, (ima_transport_info_t *)tport, groupID, 1, noCaseCheck);
    if (destCheck == 0) {
        TRACE(9, "Validate DestWithGrp passed: rc=%d object=%s dest=%s userID=%s groupID=%s\n",
            destCheck, objectName, destination, uid ? uid : "NULL", groupname);
    } else {
        TRACE(7, "Validate DestWithGrp falied: rc=%d object=%s dest=%s userID=%s groupID=%s\n",
            destCheck, objectName, destination, uid ? uid : "NULL", groupname);
    }

    ism_common_free(ism_memory_admin_misc,groupname);

    return destCheck;
}

/*
 * Validate destination with ${GroupID} as a variable substitution
 */
#define MAX_GROUPS 200

XAPI int ism_security_validateDestinationWithGroupID(const char *objectName,
        ismSecurity_t *sContext, int ngrp, const char *destination)
{
    char *ID = NULL;
    ism_transport_t *tport = NULL;
    int noGroupIDs = ngrp;
    char *groupID[MAX_GROUPS];
    int destCheck = 1;
        int noCaseCheck = 0;

    if (sContext && noGroupIDs) {
        tport = ism_security_getTransFromSecContext(sContext);
        if (tport) {
            ID = (char *) tport->userid;
            if (ID == NULL) {
                TRACE(7, "User ID in transport is NULL\n");
                return ISMRC_UserNotAuthorized;
            }
        } else {
            TRACE(7, "Invalid or NULL transport object in security context.\n");
            return ISMRC_UserNotAuthorized;
        }
    } else {
        TRACE(7, "Invalid or NULL security context or no ${GroupID} in destination: %s\n",
                destination ? destination : "NULL");
        return ISMRC_UserNotAuthorized;
    }

    /* Use oauthGroup if set in security context, otherwise get group information from LDAP */
    char *oauthGroup = ism_security_context_getOAuthGroup(sContext);
    if (oauthGroup) {
        char *groupDelimiter = ism_security_context_getOAuthGroupDelimiter(sContext);
        TRACE(9, "Validate user %s for OAuth group %s - use delimiter [%s]\n", ID, oauthGroup, groupDelimiter);
        int len = strlen(oauthGroup);
        char *tmpstr = (char *) alloca(len + 1);
        memcpy(tmpstr, oauthGroup, len);
        tmpstr[len] = 0;
        len = strlen(groupDelimiter);
        char *tmpdel = (char *)alloca(len+1);
        memcpy(tmpdel, groupDelimiter, len);
        tmpdel[len] = 0;
        char *token, *nexttoken = NULL;
        for (token = strtok_r(tmpstr, tmpdel, &nexttoken); token != NULL; token =
                strtok_r(NULL, tmpdel, &nexttoken)) {
            groupID[0] = token;
            destCheck = ism_common_matchWithVars(objectName, destination, (ima_transport_info_t *) tport, groupID, 1, 0);
            if (destCheck == 0) {
                break;
            } else {
                TRACE(9, "OAuth destination validation failed: rc=%d objectName=%s destination=%s groupID=%s\n",
                        destCheck, objectName, destination, token);
            }
        }
        if (destCheck == 1) {
            TRACE(7, "OAuth destination validation failed: rc=%d objectName=%s destination=%s groupID=%s\n",
                    destCheck, objectName, destination, oauthGroup);
        }

    } else {

        /* Get group from LDAP */
        ismAuthToken_t * token = ism_security_getSecurityContextAuthToken(sContext);
        if (token == NULL) {
            TRACE(7, "Authorization token is NULL for user ID=%s\n", ID);
            return ISMRC_UserNotAuthorized;
        }

        ismLDAPConfig_t * ldapConfig = ism_security_getLDAPConfig();
        char *grpSuffix = ldapConfig->GroupSuffix;
        char *grpIpMap = ldapConfig->GroupIdMap;

        int grpSuffixLen = 0;
        int grpSuffixHexLen = 0;
        char *hexGrpSuffix = NULL;
        char *hexGrpPtr = NULL;

        /*
        if (ldapConfig->IgnoreCase == true) {
            noCaseCheck = 1;
        }
        */

        /* Allocate memory for grpSuffix manupulation */
        if (grpSuffix) {
            /* need to escape the group suffix */
            grpSuffixLen = strlen(grpSuffix);
            grpSuffixHexLen = ism_admin_ldapHexExtraLen(grpSuffix, grpSuffixLen);
            if (grpSuffixHexLen > 0) {
                hexGrpSuffix = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,214),1, (grpSuffixLen + grpSuffixHexLen + 1));
                hexGrpPtr = hexGrpSuffix;
                ism_admin_ldapHexEscape(&hexGrpPtr, grpSuffix, grpSuffixLen);
            }
        }

        if (ngrp < 0) {
            TRACE(9, "Validate user %s for %d GroupID vars: GroupSuffix:%s GroupIdMap:%s\n",
                    ID, noGroupIDs, grpSuffix ? grpSuffix : "NULL", grpIpMap ? grpIpMap : "NULL");
        } else {
            TRACE(9, "Validate user %s for %d GroupIDn vars: GroupSuffix:%s GroupIdMap:%s\n",
                    ID, noGroupIDs, grpSuffix ? grpSuffix : "NULL", grpIpMap ? grpIpMap : "NULL");
        }

        pthread_spin_lock(&token->lock);

        int dirtyCache = 1;
        if (token->gCacheExpireTime >= ism_common_currentTimeNanos())
            dirtyCache = 0;

        if (dirtyCache == 0 && ism_common_list_size(&token->gCacheList) > 0) {

            ism_common_list *gCacheList = &token->gCacheList;
            ism_common_listIterator iter;
            ism_common_list_iter_init(&iter, gCacheList);

            if (gCacheList->size > MAX_GROUPS) {
                TRACE(5, "User is in %d groups, max allowed is %d\n",
                        gCacheList->size, MAX_GROUPS);
                pthread_spin_unlock(&token->lock);
                if (grpSuffixHexLen > 0 && hexGrpSuffix != NULL)
                    ism_common_free(ism_memory_admin_misc,hexGrpSuffix);
                return ISMRC_UserNotAuthorized;
            }

            TRACE(9, "The user %s is in %d cached group(s).\n", ID, gCacheList->size);

            while (ism_common_list_iter_hasNext(&iter)) {
                ism_common_list_node * node = ism_common_list_iter_next(&iter);
                char *igrp = NULL;
                if (node) {
                    ism_groupName_t *gn = (ism_groupName_t *) node->data;
                    if (gn) {
                        igrp = gn->cname;
                    }
                } else {
                    TRACE(3, "Invalid or NULL group cache list node.\n");
                    return ISMRC_UserNotAuthorized;
                }

                TRACE(9, "GetGroupName: GroupEntry= %s grpSuffix= %s hexLen= %d hexGrpPtr= %s\n",
                        igrp? igrp:"NULL", grpSuffix? grpSuffix:"NULL", grpSuffixHexLen, hexGrpPtr? hexGrpPtr:"NULL");

                if (igrp == NULL || *igrp == '\0') {
                    TRACE(3, "Group entry in group cache list is NULL or empty.\n");
                    return ISMRC_UserNotAuthorized;
                }

                destCheck = ism_security_getGroupAndValidateDestination(ID, igrp, grpSuffix, grpSuffixHexLen, hexGrpPtr, objectName,
                        destination, (ima_transport_info_t *)tport, noCaseCheck);

                if ( destCheck == 0 ) break;

            }
            ism_common_list_iter_destroy(&iter);
        }

        pthread_spin_unlock(&token->lock);

        /* If the cache is timeout or invalid - check LDAP */
        if (dirtyCache == 1) {

            /*Clean out the current cache*/
            pthread_spin_lock(&token->lock);
            if (ism_common_list_size(&token->gCacheList) > 0) {
                ism_common_list_destroy(&token->gCacheList);
            }
            pthread_spin_unlock(&token->lock);

            /* Call to get the current group membership */
            char * memberdn = ism_security_getLDAPDNFromMap(ID);
            if (memberdn == NULL) {
                memberdn = ism_security_putLDAPDNToMap(ID, 0);
                if (memberdn == NULL) {
                    TRACE(5, "User ID: %s is not member of any group\n", ID ? ID : "NULL");
                    if (grpSuffixHexLen > 0 && hexGrpSuffix != NULL)
                        ism_common_free(ism_memory_admin_misc,hexGrpSuffix);
                    return ISMRC_UserNotAuthorized;
                }
            }

            ism_security_getMemberGroupsInternal(NULL, memberdn, token, 0);
            TRACE(9, "Got group from LDAP:%s\n", memberdn ? memberdn : "NULL");

            //Update the timeout
            pthread_spin_lock(&token->lock);
            token->gCacheExpireTime = ism_common_currentTimeNanos() + (groupCacheTTL * SECOND_TO_NANOS);

            ism_common_list *gCacheList = &token->gCacheList;
            ism_common_listIterator iter;
            ism_common_list_iter_init(&iter, gCacheList);

            if (gCacheList->size > MAX_GROUPS) {
                TRACE(5, "User is in %d groups, max allowed is %d\n", gCacheList->size, MAX_GROUPS);
                pthread_spin_unlock(&token->lock);
                if (grpSuffixHexLen > 0 && hexGrpSuffix != NULL)
                    ism_common_free(ism_memory_admin_misc,hexGrpSuffix);
                return ISMRC_UserNotAuthorized;
            }

            TRACE(9, "The user %s is in %d group(s).\n", ID, gCacheList->size);

            while (ism_common_list_iter_hasNext(&iter)) {
                ism_common_list_node * node = ism_common_list_iter_next(&iter);
                char *igrp = NULL;
                if (node) {
                    ism_groupName_t *gn = (ism_groupName_t *) node->data;
                    if (gn) {
                        igrp = gn->cname;
                    }
                } else {
                    TRACE(3, "Invalid or NULL group cache list node.\n");
                    return ISMRC_UserNotAuthorized;
                }

                TRACE(9, "GetGroupName: GroupEntry= %s grpSuffix= %s hexLen= %d hexGrpPtr= %s\n",
                        igrp? igrp:"NULL", grpSuffix? grpSuffix:"NULL", grpSuffixHexLen, hexGrpPtr? hexGrpPtr:"NULL");

                if (igrp == NULL || *igrp == '\0') {
                    TRACE(3, "Group entry in group cache list is NULL or empty.\n");
                    return ISMRC_UserNotAuthorized;
                }

                destCheck = ism_security_getGroupAndValidateDestination(ID, igrp, grpSuffix, grpSuffixHexLen, hexGrpPtr, objectName,
                        destination, (ima_transport_info_t *)tport, noCaseCheck);

                if ( destCheck == 0 ) break;

            }
            ism_common_list_iter_destroy(&iter);

            pthread_spin_unlock(&token->lock);
        }

        if (grpSuffixHexLen > 0 && hexGrpSuffix != NULL)
            ism_common_free(ism_memory_admin_misc,hexGrpSuffix);

    }


    return destCheck;
}
