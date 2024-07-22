/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
#define TRACE_COMP Transport
#include <ismutil.h>
#include <pxtransport.h>
#include <ismjson.h>
#include <openssl/opensslv.h>
#include <openssl/evp.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <selector.h>

#include <tenant.h>

#ifndef XSTR
#define XSTR(s) STR(s)
#define STR(s) #s
#endif

#define JSON_OPTION_CLEARNULL 0x80

xUNUSED static char * versionString = "version_string: imaproxy " XSTR(ISM_VERSION) " " XSTR(BUILD_LABEL) " " XSTR(ISMDATE) " " XSTR(ISMTIME);

ism_topicrule_t * ism_proxy_getTopicRule(const char * name);
const char * ism_proxy_getTopicRuleName(ism_topicrule_t * topicrule);
static pthread_mutex_t     tenantlock;
static pthread_spinlock_t  fairuselock;

xUNUSED static int httpProxyEnabled;

#ifdef NO_PROXY
#define g_tenant_buckets 1
#else
#define g_tenant_buckets 256
#endif
#define g_user_buckets 1
ism_tenant_t * ismTenants [g_tenant_buckets] = {0};
ism_user_t *   ismUsers [g_user_buckets] = {0};
ism_server_t * ismServers = NULL;
xUNUSED static int     ismServersCount = 0;
extern int g_need_dyn_write;

extern int  ism_monitor_startServerMonitoring(ism_server_t * server);
extern int ism_server_startServerConnection(ism_server_t * server);
struct ssl_ctx_st * ism_transport_clientTlsContext(const char * name, const char * tlsversion, const char * cipher);
int  ism_monitor_startServerMonitoring(ism_server_t * server);
int  ism_transport_startMuxConnections(ism_server_t * server);
int ism_transport_crlVerify(int gppd, X509StoreCtx * ctx);
void ism_proxy_changeMsgRouting(ism_tenant_t * tenant, int old_msgRouting);
int ism_proxy_parseFairUse(tenant_fairuse_t * fairuse, const char * fairUsePolicy, const char * tenant);
tenant_fairuse_t * ism_proxy_makeFairUse(const char * name, const char * value);
static int  replaceString(const char * * oldval, const char * val);
int ism_server_getAddress(ism_server_t * server, ism_transport_t * transport, ism_gotAddress_f gotAddress);
static void linkTenant(ism_tenant_t * rtenant);
extern int ism_proxy_termAuth(void);
extern int ism_proxy_initAuth(void);
int ism_rlac_deleteACL(const char * name);
//AWS SQS Functions
extern void ism_proxy_sqs_init(ism_server_t * server);
extern bool ism_mqtt_isValidWillPolicy(int willPolicy) ;


/*
 * TLS methods enumeration
 */
static ism_enumList enum_methods [] = {
    { "Method",    5,                 },
    { "SSLv3",     SecMethod_SSLv3,   },
    { "TLSv1",     SecMethod_TLSv1,   },
    { "TLSv1.1",   SecMethod_TLSv1_1, },
    { "TLSv1.2",   SecMethod_TLSv1_2, },
    { "TLSv1.3",   SecMethod_TLSv1_3, },
};

/*
 * Cipher enumeration
 */
static ism_enumList enum_ciphers [] = {
    { "Ciphers",   3,                  },
    { "Best",       CipherType_Best,   },
    { "Fast",       CipherType_Fast,   },
    { "Medium",     CipherType_Medium, },
};

#ifndef NO_PROXY
/*
 * TLS enumeration
 */
static ism_enumList enum_server_tls [] = {
    { "TLS",        3,                  },
    { "None",       Server_Cipher_None, },
    { "Fast",       Server_Cipher_Fast, },
    { "Best",       Server_Cipher_Best, },
};

/*
 * FairUsePolicy log setting
 */
static ism_enumList enum_fairuse_log [] = {
    { "FairUseLog", 3 },
    { "false",      0 },
    { "true",       1 },
    { "only",       2 },
};

/*
 * Authenticate type
 */
static ism_enumList enum_tenant_auth [] = {
    { "Authenticate",  4, },
    { "None",          Tenant_Auth_None,         },   /* Anonymous only */
    { "Token",         Tenant_Auth_Token,        },   /* User/Password authentication */
    { "CertOrToken",   Tenant_Auth_CertOrToken,  },   /* Certificate OR token */
    { "CertAndToken",  Tenant_Auth_CertAndToken, }    /* Certificate AND token */
};

/*
 * Fair use by plan structure;
 */
typedef struct fairuse_plan_t {
    char name [64];
    struct fairuse_plan_t * next;
    tenant_fairuse_t fairuse;
} fairuse_plan_t;


#ifndef HAS_BRIDGE
/*
 * Dummy event Tenant for the
 */
ism_tenant_t eventTenant = {
    .name        = "!event",
	.enabled = 1
};
/*
 * Dummy Meter Tenant for the
 */
ism_tenant_t meterTenant = {
    .name        = "!meter",
	.enabled = 1
};

#endif

static const char * bestCipher;
static const char * fastCipher;
static int allowLocalCertFiles = 0;
int ism_proxy_setTenantAlias(const char * str);
static tenant_fairuse_t g_fairuse = {0};
static fairuse_plan_t * g_fairuse_head = NULL;
#endif

/*
 * Initialization for tenants
 */
void ism_tenant_init(void) {
    static int inited = 0;
    if (inited)
        return;
    inited = 1;

    pthread_mutex_init(&tenantlock, NULL);

#ifndef NO_PROXY
    int ix;
    pthread_spin_init(&fairuselock, 0);
    httpProxyEnabled = ism_common_getIntConfig("httpProxyEnabled", 0);

    ism_proxy_setTenantAlias(ism_common_getStringConfig("TenantAlias"));

    /* For testing */
    allowLocalCertFiles = ism_common_getIntConfig("allowLocalCertFiles", 1);

    /*
     * Read default fair use policy
     */
    for (ix=0;;ix++) {
        const char * name;
        int   namelen;
        ism_field_t f;
        ism_prop_t * cprops = ism_common_getConfigProperties();
        if (ism_common_getPropertyIndex(cprops, ix, &name))
            break;
        if (name) {
            namelen = strlen(name);
            if (namelen >= 13 && !memcmp(name, "FairUsePolicy", 13)) {
                ism_common_getProperty(cprops, name, &f);
                if (f.type == VT_String) {
                    if (namelen == 13) {
                        ism_proxy_parseFairUse(&g_fairuse, f.val.s, "_FairUsePolicy");
                    } else if (name[13]=='.') {
                        if (namelen > 45) {
                            TRACE(5, "The FairUsePolicy name is too long: %s\n", name);
                        } else {
                            tenant_fairuse_t * fup = ism_proxy_makeFairUse(name+14, f.val.s);
                            if (!fup)
                                TRACE(3, "The static FairUsePolicy.%s is not set\n", name+14);
                        }
                    }
                }
            }
        }
    }

    /*Init Auth*/
    ism_proxy_initAuth();

    /*Set the proxy internal Tenant*/
  	TRACE(8, "Link Event Tenant");
   	linkTenant(&eventTenant);

    /*Set the proxy meter internal Tenant*/
  	TRACE(8, "Link Meter Tenant");
  	linkTenant(&meterTenant);

#endif
}

void ism_tenant_term(void) {
    ism_server_t * pServer;
    ism_tenant_lock();
    for(pServer = ismServers; pServer != NULL; pServer = pServer->next) {
        pServer->mqttCon.disabled = 1;
    }
    ism_tenant_unlock();

}


/*
 * Lock the tenant
 */
void ism_tenant_lock_int(const char * file, int line) {
    // if (SHOULD_TRACE(8))
    //    traceFunction(8, 0, file, line, "Lock tenant\n");
    pthread_mutex_lock(&tenantlock);
}

/*
 * Unlock the tenant
 */
void ism_tenant_unlock(void) {
    // TRACE(8, "Unlock tenant\n");
    pthread_mutex_unlock(&tenantlock);
}

/*
 * Get the version of libimaproxy.so
 */
const char * ism_proxy_getVersion(void) {
    return versionString;
}

#define FNV_OFFSET_BASIS_32 0x811C9DC5
#define FNV_PRIME_32 0x1000193

/*
 * Hash function
 */
uint32_t ism_proxy_hash(const char * in) {
    uint32_t hash = FNV_OFFSET_BASIS_32;
    uint8_t * in8 = (uint8_t *) in;

    while (*in8) {
        hash ^= *(in8++);
        hash *= FNV_PRIME_32;
    }
    return hash;
}


/*
 * Find the tenant by name.
 * The caller should be holding the tenantlock.
 */
ism_tenant_t * ism_tenant_getTenant(const char * name) {
    uint32_t hash;
    ism_tenant_t * ret;
    if (!name)
        return NULL;
    hash = ism_proxy_hash(name);
    ret = ismTenants[hash % g_tenant_buckets];
    while (ret) {
        if (!strcmp(name, ret->name))
            break;
        ret = ret->next;
    }
    return ret;
}
#ifndef NO_PROXY

static int tlsInited = 0;
/*
 * Initialize Server TLS contexts
 */
int ism_tenant_initServerTLS(void) {
    ism_server_t * server;

    /*
     * Configure cipher lists
     */
    bestCipher = ism_common_getStringConfig("BestCipher");
    if (!bestCipher || !*bestCipher)
        bestCipher = "AES128+AESGCM:!aNULL";
    fastCipher = ism_common_getStringConfig("FastCipher");
    if (!fastCipher || !*fastCipher)
        fastCipher = "AES128-GCM-SHA256:AES128+AESGCM:!aNULL";
    TRACE(5, "bestCipher=%s\n", bestCipher);
    TRACE(5, "fastCipher=%s\n", fastCipher);

    /*
     * Create all servers at startup
     */
    ism_tenant_lock();
    tlsInited = 1;
    server = ismServers;
    while (server) {
        if (server->useTLS) {
            server->tlsCTX = ism_transport_clientTlsContext("server", "TLSv1.2",
                    server->useTLS == Server_Cipher_Best ? bestCipher : fastCipher);
            if (!server->tlsCTX) {
                TRACE(2, "Unable to create server TLS context: server=%s\n", server->name);
            } else {
                TRACE(5, "Create init TLS context: server=%s method=%s ciphers=%s\n",
                        server->name, "TLSv1.2", ism_common_enumName(enum_server_tls, server->useTLS));
            }
        }
        server = server->next;
    }
    ism_tenant_unlock();

    return 0;
}


/*
 * Find the server by name
 */
ism_server_t * ism_tenant_getServer(const char * name) {
    ism_server_t * ret = ismServers;
    if (!name)
        return NULL;
    while (ret) {
        if (!strcmp(name, ret->name))
            break;
        ret = ret->next;
    }
    return ret;
}
#endif

/*
 * Link this tenant to the end of the list
 */
static void linkTenant(ism_tenant_t * rtenant) {
    int bucket;
    rtenant->hash = ism_proxy_hash(rtenant->name);
    bucket = rtenant->hash % g_tenant_buckets;
    rtenant->next = NULL;
    if (!ismTenants[bucket]) {
        ismTenants[bucket] = rtenant;
    } else {
        ism_tenant_t * tenant = ismTenants[bucket];
        while (tenant->next) {
            tenant = tenant->next;
        }
        tenant->next = rtenant;
    }
}

/* External interface */
void ism_tenant_likeTenant(ism_tenant_t * tenant) {
    ism_tenant_lock();
    linkTenant(tenant);
    ism_tenant_unlock();
}
#ifndef NO_PROXY

/*
 * Free a tenant object
 */
static void freeTenant(ism_tenant_t * tenant) {
    if (tenant) {
        if (tenant->useSNI) {
            ism_ssl_removeSNIConfig(tenant->name);
            tenant->useSNI = 0;
        }
        if (tenant->serverstr) {
            ism_common_free(ism_memory_proxy_tenant,(char *)tenant->serverstr);
            tenant->serverstr = NULL;
        }
        if (tenant->name) {
            ism_common_free(ism_memory_proxy_tenant,(char *)tenant->name);
            tenant->name = NULL;
        }
        ism_common_free(ism_memory_proxy_tenant,tenant);
    }
}


/*
 * Unlink this tenant
 */
static void unlinkTenant(ism_tenant_t * rtenant) {
    int bucket = rtenant->hash % g_tenant_buckets;
    if (ismTenants[bucket] == NULL)
        return;
    if (ismTenants[bucket] == rtenant) {
        ismTenants[bucket] = ismTenants[bucket]->next;
    } else {
        ism_tenant_t * tenant = ismTenants[bucket];
        while (tenant->next) {
            if (tenant->next == rtenant) {
                tenant->next = rtenant->next;
                break;
            }
            tenant = tenant->next;
        }
    }
}


/*
 * Link this tenant to the end of the list
 */
static void linkServer(ism_server_t * rserver) {
    rserver->next = NULL;
    ismServersCount++;
    if (!ismServers) {
        ismServers = rserver;
    } else {
        ism_server_t * server = ismServers;
        while (server->next) {
            server = server->next;
        }
        server->next = rserver;
    }
}


const char * * g_tenant_alias       = NULL;
char *         g_tenant_alias_str   = NULL;
int            g_tenant_alias_count = 0;

/*
 * Process a tenant alias.
 * The original and alias names must be the same length
 */
int ism_proxy_setTenantAlias(const char * str) {
    int   rc = 0;
    const char * token;
    char * more;
    int   count;
    const char * org;
    const char * alias_org;
    ism_tenant_lock();
    if (!str)
        str = "";
    count = ism_common_countTokens(str, " ,");
    if (g_tenant_alias) {
        ism_common_free(ism_memory_proxy_tenant,g_tenant_alias);
        g_tenant_alias = NULL;
        g_tenant_alias_count=0;
        /* Do not free the string as we return a pointer into it */
    }
    if (count) {
        g_tenant_alias_str = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_tenant,1000),str);
        g_tenant_alias = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tenant,5),count*3, sizeof(const char *)); //0 position=incoming org, 1 position= alias org, 2 postion=devID prefix
        g_tenant_alias_count = count = 0;
        more = (char *)g_tenant_alias_str;
        token = ism_common_getToken(more, " ,", " ,", &more);
        while (token) {
        	 	char * devID=NULL;
        		char * pos = strchr(token, '=');
            int    tokenlen = strlen(token);
            char * paren_open_pos = strchr(token, '(');
            char * paren_close_pos = strchr(token, ')');
            int length_check=0;

            //Perform length check to ensure original and alias length are the same
            if(paren_open_pos && paren_close_pos){
            		length_check = (((paren_open_pos-token)*2)+((paren_close_pos - paren_open_pos)+2 ))==tokenlen; //Ensure org length are valid
            }else{
            		length_check = ((pos-token)*2+1)==tokenlen;
            }

            if (pos && pos > token && length_check) {
            		//Parse if deviceID filter is in the token
            		if(paren_open_pos && paren_close_pos){
            			devID = paren_open_pos+1;
            			*paren_close_pos=0;
            			alias_org=token;
            			*paren_open_pos=0;
            			pos++;
            			org = pos;

            		}else{
            			//No DevID prefix, set the incoming org
            			alias_org=token;
            			*pos++=0;
            			org = pos;

            		}

                g_tenant_alias[count*3] = alias_org;
                g_tenant_alias[count*3+1] = org;
                g_tenant_alias[count*3+2] = devID;

                if(devID != NULL){
                		TRACE(3, "Set tenant alias %u: %s=%s with deviceIDPrefix=%s\n", count, g_tenant_alias[count*3], g_tenant_alias[count*3+1], g_tenant_alias[count*3+2]);
                }else{
                		TRACE(3, "Set tenant alias %u: %s=%s\n", count, g_tenant_alias[count*3], g_tenant_alias[count*3+1]);
                }

                count++;
            } else {
                TRACE(3, "The TenantAlias definition is not valid: %s\n", token);
                rc = ISMRC_ArgNotValid;
                ism_common_setErrorData(rc, "%s", "TenantAlias");
                break;
            }
            token = ism_common_getToken(more, " ,", " ,", &more);
        }
        g_tenant_alias_count = count;
    }

    ism_tenant_unlock();
    return rc;
}


/*
 * Look up a tenant alias
 * If devID filter is available, validate the device ID as well
 * This should be called with the tenantlock held
 */
const char * ism_tenant_getTenantAlias(const char * org, const char * deviceID) {
    int  i;
    const char * alias = NULL;
    for (i=0; i<g_tenant_alias_count; i++) {
        if (!strcmp(org, g_tenant_alias[i*3])){
        		if(g_tenant_alias[i*3 + 2] != NULL && deviceID != NULL){
        			//Filter by deviceID Prefix as well
        			const char * f_devID = g_tenant_alias[i*3+2];
        			int flen = strlen(f_devID);
        			int devIDlen = strlen(deviceID);
        			if(devIDlen >= flen && strncmp(deviceID, f_devID, flen) == 0){
        				alias = g_tenant_alias[i*3+1];
        			}else {
        				alias = NULL;
        			}

        		}else{
        			alias =  g_tenant_alias[i*3+1];
        		}
        }
        if(alias != NULL){
        		//Found the first alias, break the loop
        		break;
        }
    }
    return alias;
}


/*
 * Free a server object
 */
void freeServer(ism_server_t * server) {
    int i;
    if (server) {
        for (i=0; i<16; i++) {
            replaceString(server->address+i, NULL);
        }
        if (server->name) {
            ism_common_free(ism_memory_proxy_tenant,(char *)server->name);
            server->name = NULL;
        }
        pthread_spin_destroy(&server->lock);
        ism_common_free(ism_memory_proxy_tenant,server);
    }
}

/*
 * Unlink this tenant
 */
void unlinkServer(ism_server_t * rserver) {
    if (ismServers && rserver) {
        rserver->disabled = 1;
        ismServersCount--;
        if (ismServers == rserver) {
            ismServers = ismServers->next;
        } else {
            ism_server_t * server = ismServers;
            while (server->next) {
                if (server->next == rserver) {
                    server->next = rserver->next;
                    break;
                }
                server = server->next;
            }
        }
    }
}
#endif


/*
 * Compute SHA256.
 * Low level API call to digest is forbidden in FIPS mode.
 * Use high level digest APIs.
 */
XAPI uint32_t ism_ssl_SHA256(const void *key, size_t keyLen, uint8_t * mdBuf) {
    uint32_t mdLen = 0;
    const EVP_MD *md = EVP_sha256();

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX mdCtx;
    EVP_MD_CTX_init(&mdCtx);
    EVP_DigestInit_ex(&mdCtx, md, NULL);
    EVP_DigestUpdate(&mdCtx, key, keyLen);
    EVP_DigestFinal_ex(&mdCtx, mdBuf, &mdLen);
    EVP_MD_CTX_cleanup(&mdCtx);
#else
    EVP_MD_CTX * mdCtx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdCtx, md, NULL);
    EVP_DigestUpdate(mdCtx, key, keyLen);
    EVP_DigestFinal_ex(mdCtx, mdBuf, &mdLen);
    EVP_MD_CTX_free(mdCtx);
#endif
    return mdLen;
}


/*
 * A user can be either global or local to a tenant.
 * @param user    The user to find
 * @parma tenant  The optional tenant to look in for the user
 */
XAPI ism_user_t * ism_tenant_getUser(const char * username, ism_tenant_t * tenant, int only) {
    ism_user_t * user = NULL;

    if (username) {
        if (tenant) {
            user = tenant->users;
            while (user) {
                if (!strcmp(username, user->name))
                    break;
                user = user->next;
            }
        }
        if (!only) {
            if (!user) {
                int bucket = ism_proxy_hash(username) % g_user_buckets;
                user = ismUsers[bucket];
                while (user) {
                    if (!strcmp(username, user->name))
                        return user;
                    user = user->next;
                }
            }
        }
    }
    return user;
}


/*
 * Create an obfuscated password
 */
int  ism_tenant_createObfus(const char * user, const char * pw, char * buf, int buflen, int otype) {
    int  len;
    const char * salt1 = "\xD4\xB3\x25\x01\x7c\x99\x53\x37\xD3\xDE\x61\x17\x08\x7e\x92\xc7";
    const char * salt;
    char  * xbuf;
    char  sbuf[64];
    char  obuf[64];
    int   pwlen;
    int   userlen;
    int   saltlen;
    if (!user)
        user = "";
    if (!pw)
        pw = "";
    userlen = strlen(user);
    pwlen = strlen(pw);

    switch (otype) {

    /* Plain text password - length is not limited */
    case 0:
        len = pwlen;
        if (*pw && ((uint8_t)*pw < 0x30 || *pw == '=')) {
            len++;
            if (buflen) {
                *buf = '\\';
                ism_common_strlcpy(buf+1, pw, buflen);
            }
        } else {
            ism_common_strlcpy(buf, pw, buflen);
        }
        break;

    /* SHA256 digest password - length is 45 bytes */
    case 1:
        salt = salt1;
        saltlen = 16;
        saltlen -= (userlen&7);
        xbuf = alloca(saltlen + pwlen + userlen);
        memcpy(xbuf, user, userlen);
        memcpy(xbuf+userlen, salt, saltlen);
        memcpy(xbuf+userlen+saltlen, pw, pwlen);
        len = ism_ssl_SHA256(xbuf, (size_t)saltlen + pwlen + userlen, (uint8_t *)sbuf);
        obuf[0] = '=';
        len = ism_common_toBase64(sbuf, obuf+1, len);
        obuf[len+1] = 0;
        ism_common_strlcpy(buf, obuf, buflen);
        break;

    /* Other types are not yet defined */
    default:
        len = -1;
        if (buflen)
            *buf = 0;
        break;
    }
    return len+1;
}

/*
 * Check obfuscated password.
 * - Type 0 is a plain text password, and if it starts with a character < 0x30
 *          it must have a preceeding backslash.
 * - Type 1 is a SHA256 password in base64 encoding, and it has a leading equals (=)
 *          to indicate the type.
 * - Other starter characters less than 0x30 ('0') are reserved for other password types.
 * @return 0=does not match, 1=matches
 */
int ism_tenant_checkObfus(const char * user, const char * pw, const char * obfus) {
    int otype = 0;
    char obuf [64];

    /* Treat null and empty string the same */
    if (!obfus)
        obfus = "=";
    if (!user)
        user = "";
    if (!pw)
        pw = "";

    /* Determine type from leading character */
    if (*obfus == '\\') {
        obfus++;
        otype = 0;
    } else {
        if (((uint8_t)*obfus) < 0x30 || *obfus == '=') {
            switch (*obfus) {
            case '=':
                otype = 1;
                break;
            default:
                return 0;
            }
        }
    }

    if (otype == 0) {
        return !strcmp(pw, obfus);
    } else {
        ism_tenant_createObfus(user, pw, obuf, sizeof obuf, otype);
        return !strcmp(obfus, obuf);
    }

}


/*
 * Link this user to the end of the list
 */
void linkUser(ism_user_t * ruser, ism_tenant_t * tenant) {
    ruser->next = NULL;
    if (tenant) {
        if (!tenant->users) {
            tenant->users = ruser;
        } else {
            ism_user_t * user = tenant->users;
            while (user->next) {
                user = user->next;
            }
            user->next = ruser;
        }
    } else {
        int bucket = ism_proxy_hash(ruser->name) % g_user_buckets;
        if (!ismUsers[bucket]) {
            ismUsers[bucket] = ruser;
        } else {
            ism_user_t * user = ismUsers[bucket];
            while (user->next) {   /* Put at end */
                user = user->next;
            }
            user->next = ruser;
        }
    }
}

/*
 * Unlink a user
 */
void unlinkUser(ism_user_t * ruser, ism_tenant_t * tenant) {
    if (tenant) {
        if (tenant->users == NULL)
            return;
        if (tenant->users == ruser) {
            tenant->users = tenant->users->next;
        } else {
            ism_user_t * user = tenant->users;
            while (user->next) {
                if (user->next == ruser) {
                    user->next = ruser->next;
                    break;
                }
                user = user->next;
            }
        }
    } else {
        int bucket = ism_proxy_hash(ruser->name) % g_user_buckets;
        if (ismUsers[bucket] == NULL)
            return;
        if (ismUsers[bucket] == ruser) {
            ismUsers[bucket] = ismUsers[bucket]->next;
        } else {
            ism_user_t * user = ismUsers[bucket];
            while (user->next) {
                if (user->next == ruser) {
                    user->next = ruser->next;
                    break;
                }
                user = user->next;
            }
        }
    }
}

/*
 * Configure a tenant
 */
int ism_tenant_config_json(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing) {
    int   rc = 0;
    int   xrc;
    int   endloc;
    ism_json_entry_t * ent;
    int isTenant;
    int isUser;
    int isServer;

    ism_tenant_lock();

    if (!parseobj || where > parseobj->ent_count) {
        TRACE(2, "Tenant config JSON not correct\n");
        ism_tenant_unlock();
        return 1;
    }
    ent = parseobj->ent+where;
    isTenant = !strcmp(ent->name, "Tenant");
    isUser   = !strcmp(ent->name, "User");
    isServer = !strcmp(ent->name, "Server");
    if (!ent->name || (!isTenant && !isUser && !isServer) || ent->objtype != JSON_Object) {
        TRACE(2, "Tenant config JSON invoked for config which is not a server, tenant, or user\n");
        ism_tenant_unlock();
        return 2;
    }
    endloc = where + ent->count;
    where++;
    ent++;
    while (where <= endloc) {
        if (isUser) {
            xrc = ism_tenant_makeUser(parseobj, where, NULL, NULL, checkonly, keepgoing);
            if (rc == 0)
                rc = xrc;
        }
#ifndef NO_PROXY
        if (isTenant) {
            xrc = ism_tenant_makeTenant(parseobj, where, NULL);
            if (rc == 0)
                rc = xrc;
        }
        if (isServer) {
            xrc = ism_tenant_makeServer(parseobj, where, NULL);
            if (rc == 0)
                rc = xrc;
        }
#endif
        ent = parseobj->ent+where;
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }
    ism_tenant_unlock();
    return rc;
}

/*
 * Do a strcmp allowing for NULLs
 */
static int my_strcmp(const char * str1, const char * str2) {
    if (str1 == NULL || str2 == NULL) {
        if (str1 == NULL && str2 == NULL)
            return 0;
        return str1 ? 1 : -1;
    }
    return strcmp(str1, str2);
}

/*
 * Replace a string
 * Return 1 if the value is changed, 0 if it has not changed
 */
static int  replaceString(const char * * oldval, const char * val) {
    if (!my_strcmp(*oldval, val))
        return 0;
    if (*oldval) {
        char * freeval = (char *)*oldval;
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_utils,1000),val);
        else
            *oldval = NULL;
        ism_common_free(ism_memory_proxy_utils,freeval);
    } else {
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_utils,1000),val);
        else
            *oldval = NULL;
    }
    return 1;
}


/*
 * Return a string from a JSON entry even for things without a value.
 */
static const char * getJsonValue(ism_json_entry_t * ent) {
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

#ifndef NO_PROXY
/*
 * Parse a valid value for a fair use policy item.
 * We allow two decimal places so always use a multiplier of 100
 */
static float validVal(const char * tenant, const char * svalue, const char * name, double defval, int minval, int maxval, int * prc) {
    char * eos;
    double value = strtod(svalue, &eos);
    if (*eos || value<(double)minval || value>(double)maxval) {
        TRACE(3, "Invalid fair use value: tenant=%s item=%s value=%s\n", tenant, name, svalue);
        if (prc)
            *prc = ISMRC_BadPropertyValue;
        return defval;
    }
    return (float)value;
}

#if 0
static int parseRouteSelect(const char * tenant, char * rulestr, ismRule_t * * rules, char * * xmore) {
    char * more;
    int rc;
    ismRule_t * rule;

    /*
     * If the rule string starts with a left paren, the rule goes until
     * a matching right paren.
     */
    if (*rulestr == '(') {
        rulestr++;
        int level = 1;
        int inquote = 0;
        char * rp=rulestr;
        while (*rp) {
            if (*rp == '\'') {
                inquote ^= 1;
            } else {
                if (!inquote) {
                    if (*rp == '(') {
                        level++;
                    } else if (*rp == ')') {
                        level--;
                        if (level == 0)
                            break;
                    }
                }
            }
            rp++;
        }
        if (*rp)
            *rp++ = 0;
        more = rp;
    } else {
        more = rulestr + strlen(rulestr);
    }

    rc = ism_common_compileSelectRuleOpt(&rule, NULL, rulestr, SELOPT_Internal);
    if (rc == 0) {
        *rules = rule;
    }
    if (xmore)
        *xmore = more;
    return rc;
}
#endif

/*
 * Parse the fair use policy.
 * The fair use policy consists of a comma and/or space separated list of items.
 * Each item is a name=value pair where the value is a decimal integer.  No spaces are allowed
 * within the name=value since spaces are separators between items.
 * unit=#   The number of message bytes which makes a unit (defualt = 4096)
 * log      The log setting as true, false, only
 * mups_d   The message units per second for devices
 * mups_g   The message units per second for gateways
 * mups_a   The message units per second for applicaitons
 * mups_A   The message units per second for shared applications
 * route     The routing rule
 */
int ism_proxy_parseFairUse(tenant_fairuse_t * xfairuse, const char * fairUsePolicy, const char * tenant) {
    int rc = 0;
    tenant_fairuse_t fairuse = {0};
    if (fairUsePolicy) {
        char * more;
        char * name;
        fairuse.mups_units = 4096;
        fairuse.log = 1;      /* Default to log=true */
        char * tempfu = alloca(strlen(fairUsePolicy)+1);
        strcpy(tempfu, fairUsePolicy);
        TRACE(7, "Fair use policy for tenant %s : %s\n", tenant, tempfu);
        name = ism_common_getToken(tempfu, ", ", ", ", &more);
        while (name && rc==0) {
            char * value = strchr(name, '=');
            if (value) {
                *value++ = 0;
                if (!strcmp(name, "unit")) {
                    fairuse.mups_units = validVal(tenant, value, name, fairuse.mups_units, 1, 0x1000000, &rc);
                    if (!rc && (fairuse.mups_units == 10)) {
                    	TRACE(5, "Fair use policy for tenant %s has unit=10. Default fair use policy will be used.\n", tenant);
                    	fairuse = g_fairuse;
                    	break;
                    }
                } else if (!strcmp(name, "log")) {
                    int logval = ism_common_enumValue(enum_fairuse_log, value);
                    if (logval == INVALID_ENUM) {
                        rc = ISMRC_BadPropertyValue;
                        TRACE(3, "Invalid Fair use log value: tenant=%s value=%s\n", tenant, value);
                    }
                    fairuse.log = logval;
                } else if (!strcmp(name, "mups_a")) {
                    fairuse.mups_a = validVal(tenant, value, name, fairuse.mups_a, 0, 1000000, &rc);
                } else if (!strcmp(name, "mups_A")) {
                    fairuse.mups_A = validVal(tenant, value, name, fairuse.mups_A, 0, 1000000, &rc);
                } else if (!strcmp(name, "mups_d")) {
                    fairuse.mups_d = validVal(tenant, value, name, fairuse.mups_d, 0, 1000000, &rc);
                } else if (!strcmp(name, "mups_g")) {
                    fairuse.mups_g = validVal(tenant, value, name, fairuse.mups_g, 0, 1000000, &rc);
#if 0
                } else if (!strcmp(name, "route")) {
                    char * pos = value+strlen(value);
                    *pos = fairUsePolicy[pos-tempfu];        /* restore the original character */
                    rc = parseRouteSelect(tenant, value, &fairuse.route, &more);
#endif
                } else {
                    TRACE(3, "Unknown fair use property: tenant=%s property=%s\n", tenant, name);
                    rc = ISMRC_BadPropertyName;
                }
            } else {
                TRACE(3, "Invalid fair use rule: tenant=%s rule=%s\n", tenant, name);
                rc = ISMRC_BadPropertyName;
            }
            name = ism_common_getToken(more, ", ", ", ", &more);
        }
        if (!rc) {
            fairuse.enabled = 1;  /* Use this policy */
            TRACE(4, "Set fair use policy: name=%s units=%u mups_a=%0.1f mups_A=%0.1f mups_d=%0.1f mups_g=%0.1f log=%s\n",
                    tenant, fairuse.mups_units, fairuse.mups_a, fairuse.mups_A, fairuse.mups_d, fairuse.mups_g,
                    ism_common_enumName(enum_fairuse_log, fairuse.log));
        } else {
            memset(&fairuse, 0, sizeof fairuse);
            TRACE(4, "The fair use policy is not set: name=%s\n", tenant);
        }
        *xfairuse = fairuse;
        return rc;
    }
    *xfairuse = fairuse;
    return 1;
}

/*
 * Make or update a fairuse plan
 */
tenant_fairuse_t * ism_proxy_makeFairUse(const char * name, const char * value) {
    tenant_fairuse_t fairuse;
    fairuse_plan_t * fup;
    int  rc;
    rc = ism_proxy_parseFairUse(&fairuse, value, name);
    if (!rc) {
        pthread_spin_lock(&fairuselock);
        fup = g_fairuse_head;
        while (fup && strcmp(fup->name, name)) {
            fup = fup->next;
        }
        if (!fup) {
            fup = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tenant,9),1, sizeof(fairuse_plan_t));
            fup->next = g_fairuse_head;
            ism_common_strlcpy(fup->name, name, sizeof fup->name);
            g_fairuse_head = fup;
            TRACE(8, "Create fair use policy: %s\n", name);
        } else {
            TRACE(8, "Update fair use policy: %s\n", name);
        }
        fup->fairuse = fairuse;
        pthread_spin_unlock(&fairuselock);
        return &fup->fairuse;
    }
    return NULL;
}

/*
 * Return MUPS value from client class
 */
static inline double getMUPS(tenant_fairuse_t * fairuse, char cc) {
    if (cc=='d')
        return fairuse->mups_d;
    if (cc=='a')
        return fairuse->mups_a;
    if (cc=='A')
        return fairuse->mups_A;
    if (cc=='g')
        return fairuse->mups_g;
    return 0.0;
}

/*
 * Get the fairuse policy on connect.
 * If the tenant has a fairuse policy use it
 * Else if the tenant has a plan and the plan has a fairuse policy use it
 * Otherwise use the default fairuse policy
 *
 * The plan or tenant fairuse policies can be set dynamically.  The default
 * fairuse policy is set in static config only.
 *
 * TODO: raise trace levels
 */
tenant_fairuse_t * ism_proxy_getFairUse(ism_transport_t * transport) {
    fairuse_plan_t * fup;
    if (!transport || ! transport->tenant)
        return NULL;
    if (transport->tenant->fairuse.enabled) {
        tenant_fairuse_t * fairuse = &transport->tenant->fairuse;
        TRACE(7, "Fair use policy from tenant: connect=%u client=%s tenant=%s log=%s mups=%0.1f\n",
           transport->index, transport->name, transport->tenant->name,
           ism_common_enumName(enum_fairuse_log, fairuse->log), getMUPS(fairuse, transport->client_class));
        return fairuse;
    }
    if (transport->tenant->plan) {
        pthread_spin_lock(&fairuselock);
        fup = g_fairuse_head;
        while (fup) {
            if (!strcmp(fup->name, transport->tenant->plan)) {
                pthread_spin_unlock(&fairuselock);
                tenant_fairuse_t * fairuse = &fup->fairuse;
                TRACE(7, "Fair use policy from plan: connect=%u client=%s plan=%s log=%s mups=%0.1f\n",
                   transport->index, transport->name, transport->tenant->plan,
                   ism_common_enumName(enum_fairuse_log, fairuse->log), getMUPS(fairuse, transport->client_class));
                return fairuse;
            }
            fup = fup->next;
        }
        pthread_spin_unlock(&fairuselock);
    }
    if (g_fairuse.mups_units) {
        TRACE(8, "Fair use policy from default: connect=%u client=%s log=%s mups=%0.1f\n",
            transport->index, transport->name,
            ism_common_enumName(enum_fairuse_log, g_fairuse.log), getMUPS(&g_fairuse, transport->client_class));
    }
    return &g_fairuse;
}

/*
 * Fix up legacy auth bits and authenticate to match
 */
static void setAuthBits(int needAuth, ism_tenant_t * tenant) {
    if (needAuth == 1) {
        if (tenant->require_cert) {
            if (tenant->require_user)
                tenant->authenticate = Tenant_Auth_CertAndToken;
            else
                tenant->authenticate = Tenant_Auth_CertOrToken;
        } else {
            if (tenant->allow_anon)
                tenant->authenticate = Tenant_Auth_None;
            else
                tenant->authenticate = Tenant_Auth_Token;
        }
    }
    if (needAuth == 2) {
        switch (tenant->authenticate) {
        case Tenant_Auth_None:
            tenant->allow_anon =   1;
            tenant->require_cert = 0;
            tenant->require_user = 0;
            break;
        case Tenant_Auth_Token:
            tenant->allow_anon =   0;
            tenant->require_cert = 0;
            tenant->require_user = 0;
            break;
        case Tenant_Auth_CertOrToken:
            tenant->allow_anon =   0;
            tenant->require_cert = 1;
            tenant->require_user = 0;
            break;
        case Tenant_Auth_CertAndToken:
            tenant->allow_anon =   0;
            tenant->require_cert = 1;
            tenant->require_user = 1;
            break;
        }
    }
}

/*
 * Generate a trystate boolean from a JSON boolean
 * 0 = false
 * 1 = true
 * 2 = unknown
 */
static inline int tristate(ism_json_entry_t * ent) {
    return ent->objtype == JSON_True ? 1 : (ent->objtype == JSON_False ? 0 : 2);
}

/*
 * Make a tenant object from the configuration.
 * TODO: additional validation of the values
 */
int ism_tenant_makeTenant(ism_json_parse_t * parseobj, int where, const char * name) {
    int endloc;
    char  xbuf[1024];
    ism_server_t * server = NULL;
    ism_tenant_t * tenant;
    int  rc = 0;
    int  created = 0;
    int  xrc;
    int  needAuth = 0;          /* Need an Authenticate update */
    int  cleanCerts = 0;
    int  rlacdisabled = -1;


    if (!parseobj || (uint32_t)where > parseobj->ent_count)
        return 1;

    /* Assume clean certificates when coming from javaconfig */
    if (parseobj->options & JSON_OPTION_CLEARNULL)
        cleanCerts = 1;

    ism_json_entry_t * ent = parseobj->ent+where;
    endloc = where + ent->count;
    where++;
    if (!name)
        name = ent->name;
    tenant = ism_tenant_getTenant(name);
    if (tenant) {
        if (ent->objtype != JSON_Object) {
            unlinkTenant(tenant);
            if (tenant->useSNI) {
                ism_ssl_removeSNIConfig(tenant->name);
                tenant->useSNI = 0;
            }

            /*
             * Do not free up the tenant as it might still be in use.
             * We accept that there is a meamory leak but deleting tenants
             * is not a common activity.
             */
            return 0;
        }
    } else {
        if (ent->objtype == JSON_Object) {
            tenant = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tenant,10),1, sizeof(ism_tenant_t));
            strcpy(tenant->structid, "IoTTEN");
            tenant->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_tenant,1000),name);
            tenant->namelen = (int)strlen(name);
            tenant->check_user = 2;      /* Defaults true */
            tenant->remove_user = 2;     /* Defaults true */
            tenant->enabled = 1;         /* Always set */
            tenant->allow_durable = 2;   /* Allow a durable connection                */
            tenant->allow_systopic = 2;  /* Allow subscribe to system topics          */
            tenant->require_secure = 2;  /* Require a secure connection               */
            tenant->authenticate = Tenant_Auth_Token;
            tenant->allow_shared = 2;    /* Allow shared subscriptions                */
            tenant->allow_retain = 2;    /* Allow retained messages                   */
            tenant->user_is_clientid = ism_common_getBooleanConfig("UserIsClientID", 2);
            tenant->rlacAppResourceGroupEnabled = 2;
            tenant->rlacAppDefaultGroup = 2;
            tenant->sgEnabled = 2;
#ifdef PX_CLIENTACTIVITY            
            tenant->pxactEnabled = 2;
#endif
            tenant->disableCRL = 2;
            tenant->checkSessionUser = 2;
            tenant->willTopicValidationPolicy = 0xff; //0xff value means not set
            pthread_spin_init(&tenant->lock, 0);
            created = 1;
        } else {
            if (ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Tenant", name);
                return ISMRC_BadPropertyValue;
            }
            TRACE(2, "Tenant does not exist: %s\n", name);
            return 1;
        }
    }
    int old_msgRouting = tenant->messageRouting;
    int savewhere = where;
    while (where <= endloc) {
        ent = parseobj->ent + where;

        if (!strcmpi(ent->name, "Server")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Server", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "Enabled")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Enabled", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "Authenticate")) {
            if ((ent->objtype != JSON_String || ism_common_enumValue(enum_tenant_auth, ent->value)==INVALID_ENUM) &&
                 ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "RequireSecure")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ReqireSecure", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "RequireCertificate")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "RequireCertificate", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "SGEnabled")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "SGEnabled", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
#ifdef PX_CLIENTACTIVITY
        } else if (!strcmpi(ent->name, "ClientActivityEnabled")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ClientActivityEnabled", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
#endif
        } else if (!strcmpi(ent->name, "CACertificates")) {
            if (ent->objtype != JSON_String) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "CACertificates", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "ServerCertificate")) {
            if (ent->objtype != JSON_String) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ServerCertificate", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "ServerCertificateKey") || !strcmpi(ent->name, "ServerKey")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ServerKey", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "KeyPassword")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ServerKey", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "RequireUser")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "RequireUser", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "RemoveUser")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "RemoveUser", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "CheckUser")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "CheckUser", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "UserIsClientID")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "UserIsClientID", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "AllowDurable")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "AllowDurable", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "AllowSysTopic")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "AllowRetain")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "AllowShared")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "AllowAnonymous")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "AllowAnonymous", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "DisableCRL")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "DisableCRL", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "CheckSessionUser")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "CheckSessionUser", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MaxQoS")) {
            if ((ent->objtype != JSON_Integer && ent->objtype != JSON_Null) || ent->count < 0 || ent->count > 2) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MaxQoS", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MaxConnections")) {
            if ((ent->objtype != JSON_Integer && ent->objtype != JSON_Null) || ent->count < 0) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MaxConnectionsce", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MaxMessageSize")) {
            if ((ent->objtype != JSON_Integer && ent->objtype != JSON_Null) || ent->count < 0) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MaxMessageSize", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MaxSessionExpiryInterval")) {
            if ((ent->objtype != JSON_Integer  && ent->objtype != JSON_Null) || ent->count < 0) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MaxMessageExpiryInterval", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "WillTopicValidationPolicy")) {
            if ((ent->objtype != JSON_Integer && ent->objtype != JSON_Null) || ent->count < 0 || !ism_mqtt_isValidWillPolicy(ent->count) ) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "WillTopicValidationPolicy", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "RLACAppResourceGroupEnabled")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "RLACAppResourceGroupEnabled", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "RLACAppDefaultGroup")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "RLACAppDefaultGroup", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "FairUsePolicy")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "FairUsePolicy", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "Plan")) {
            if ((ent->objtype != JSON_String && ent->objtype != JSON_Null) || strchr(getJsonValue(ent), ' ') != NULL) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Plan", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "FairUseMethod")) {
            /* No longer used */
        } else if (!strcmpi(ent->name, "MessageRouting")) {
            if (ent->objtype != JSON_Integer || ent->count < 0) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MessageRouting", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "CheckTopic")) {
            if (ent->objtype != JSON_String || !ism_proxy_getTopicRule(ent->value)) {
                if (ent->objtype != JSON_Null && ent->value && *ent->value && strcmpi(ent->value, "none")) {
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "CheckTopic", getJsonValue(ent));
                    rc = ISMRC_BadPropertyValue;
                }
            }
        } else if (!strcmpi(ent->name, "User")) {
            if (ent->objtype != JSON_Object) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "User", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "User")) {
            int thisend = where + ent->count;
            int thisloc = where+1;
            while (thisloc < thisend) {
                ent = parseobj->ent + thisloc;
                xrc = ism_tenant_makeUser(parseobj, thisloc, NULL, tenant, 1, 0);
                if (rc == 0)
                    rc = xrc;
                thisloc += ent->count + 1;
            }
        } else if (!strcmpi(ent->name, "Port")) {
            /* No longer used */
        } else if (!strcmpi(ent->name, "MaxDeviceLimit")) {
            /* No longer used */
        } else if (!strcmpi(ent->name, "MaxMessagePolicy")) {
            /* No longer used */
        } else if (!strcmpi(ent->name, "ChangeTopic")) {
            /* No longer used */
        } else if (!strcmpi(ent->name, "HasCACertificates")) {
            /* No longer used */
        } else if (!strcmpi(ent->name, "ServerCertificateId")) {
            /* No longer used */
        } else if (!strcmpi(ent->name, "RLACAppEnabled")) {
            /* No longer used */
        } else {
            /* Having an unknown config item is not an error to allow for system upgrade */
             LOG(WARN, Server, 931, "%-s%-s", "Unknown tenant property: Tenant={0} Property={1}",
                          tenant->name, ent->name);
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }

    if (rc == 0) {
        where = savewhere;
        tenant->rmPolicies=0;
        if (cleanCerts) {
            replaceString(&tenant->serverCert, NULL);
            replaceString(&tenant->serverKey, NULL);
            replaceString(&tenant->keyPassword, NULL);
        }
        while (where <= endloc) {
            ent = parseobj->ent + where;
            if (!strcmpi(ent->name, "Server")) {
                replaceString(&tenant->serverstr, ent->value);
            } else if (!strcmpi(ent->name, "Enabled")) {
                tenant->enabled = tristate(ent);
            } else if (!strcmpi(ent->name, "RequireSecure")) {
                tenant->require_secure = tristate(ent);
            } else if (!strcmpi(ent->name, "SGEnabled")) {
                tenant->sgEnabled = tristate(ent);
#ifdef PX_CLIENTACTIVITY                
            } else if (!strcmpi(ent->name, "ClientActivityEnabled")) {
                tenant->pxactEnabled = tristate(ent);
#endif                
            } else if (!strcmpi(ent->name, "Authenticate")) {
                tenant->authenticate = ent->objtype == JSON_String ? ism_common_enumValue(enum_tenant_auth, ent->value) : 1;
                needAuth = 2;
            } else if (!strcmpi(ent->name, "RequireCertificate")) {
                tenant->require_cert = ent->objtype == JSON_True;
                needAuth = 1;
            } else if (!strcmpi(ent->name, "RequireUser")) {
                tenant->require_user = ent->objtype == JSON_True;
                needAuth = 1;
            } else if (!strcmpi(ent->name, "RemoveUser")) {
                tenant->remove_user = tristate(ent);
            } else if (!strcmpi(ent->name, "CheckUser")) {
                tenant->check_user = tristate(ent);
            } else if (!strcmpi(ent->name, "UserIsClientID")) {
                tenant->user_is_clientid = tristate(ent);
            } else if (!strcmpi(ent->name, "AllowAnonymous")) {
                tenant->allow_anon = ent->objtype == JSON_True;
                needAuth = 1;
            } else if (!strcmpi(ent->name, "DisableCRL")) {
                tenant->disableCRL = tristate(ent);
            } else if (!strcmpi(ent->name, "CheckSessionUser")) {
                tenant->checkSessionUser = tristate(ent);
            } else if (!strcmpi(ent->name, "AllowDurable")) {
                tenant->allow_durable = tristate(ent);
            } else if (!strcmpi(ent->name, "AllowShared")) {
                tenant->allow_shared = tristate(ent);
            } else if (!strcmpi(ent->name, "AllowSysTopic")) {
                tenant->allow_systopic = tristate(ent);
            } else if (!strcmpi(ent->name, "AllowRetain")) {
                tenant->allow_retain = tristate(ent);
            } else if (!strcmpi(ent->name, "MaxQoS")) {
                tenant->max_qos = ent->objtype == JSON_Null ? 2 : ent->count;
            } else if (!strcmpi(ent->name, "MaxConnections")) {
                tenant->max_connects = ent->count;
            } else if (!strcmpi(ent->name, "MaxMessageSize")) {
                tenant->maxMsgSize = ent->count;
            } else if (!strcmpi(ent->name, "WillTopicValidationPolicy")) {
                tenant->willTopicValidationPolicy = ent->count;
            }else if (!strcmpi(ent->name, "MaxSessionExpiryInterval")) {
                tenant->maxSessionExpire = ent->count;
            } else if (!strcmpi(ent->name, "RLACAppResourceGroupEnabled")) {
                int oldrlac = tenant->rlacAppResourceGroupEnabled;
                tenant->rlacAppResourceGroupEnabled = tristate(ent);
                if (oldrlac != tenant->rlacAppResourceGroupEnabled)
                    rlacdisabled = tenant->rlacAppResourceGroupEnabled;
            } else if (!strcmpi(ent->name, "RLACAppDefaultGroup")) {
                tenant->rlacAppDefaultGroup = tristate(ent);
            } else if (!strcmpi(ent->name, "FairUsePolicy")) {
            	replaceString(&tenant->fairUsePolicy, ent->value);
            	ism_proxy_parseFairUse(&tenant->fairuse, tenant->fairUsePolicy, tenant->name);
            } else if (!strcmpi(ent->name, "Plan")) {
                replaceString(&tenant->plan, ent->value);
            } else if (!strcmpi(ent->name, "ServerCertificate")) {
                replaceString(&tenant->serverCert, ent->value);
            } else if (!strcmpi(ent->name, "ServerKey") || !strcmpi(ent->name, "ServerCertificateKey")) {
                replaceString(&tenant->serverKey, ent->value);
            } else if (!strcmpi(ent->name, "KeyPassword")) {
                replaceString(&tenant->keyPassword, ent->value);
                /* TODO: obfuscate keypw */
            } else if (!strcmpi(ent->name, "CACertificates")) {
                replaceString(&tenant->caCerts, ent->value);
            } else if (!strcmpi(ent->name, "MessageRouting")) {
            	tenant->messageRouting = ent->count;
            } else if (!strcmpi(ent->name, "CheckTopic")) {
                tenant->topicrule = ism_proxy_getTopicRule(ent->value);
            } else if (!strcmpi(ent->name, "User")) {
                /* TODO: check above */
                int thisend = where + ent->count;
                int thisloc = where+1;
                while (thisloc < thisend) {
                    ent = parseobj->ent + thisloc;
                    xrc = ism_tenant_makeUser(parseobj, thisloc, NULL, tenant, 0, 0);
                    if (rc == 0)
                        rc = xrc;
                    thisloc += ent->count + 1;
                }
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
    }
    
    /*
     * Set up both authenticate and legacy bits
     */
    if (rc==0 && needAuth) {
        setAuthBits(needAuth, tenant);
    }

    /*
     * if rlac disabled delete application acls
     */
    if (rlacdisabled == 0) {
        ism_rlac_deleteACL(name);
    }

    /*
     * Check if serverless is allowed
     */
    if (!tenant->serverstr) {
        if (ism_common_getBooleanConfig("AllowServerless", 0)) {
            tenant->serverless = 1;
        } else {
            tenant->enabled = 2;
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Server", "");
            /* TODO: Log warning */
        }
    } else {
        tenant->serverless = 0;
        server = ism_tenant_getServer(tenant->serverstr);
        if (!server) {
            tenant->enabled = 2;
            /* TODO: log warning */
            TRACE(2, "Server not found for: tenant=%s server=%s\n", name, tenant->serverstr);
            ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s%s", "Server", name, tenant->serverstr);
            // rc = ISMRC_ObjectNotFound;
        }
        tenant->server = server;
    }
    if (!rc) {
        tenant->rmPolicies = 0;
        if (tenant->require_user==1)
            tenant->rmPolicies |= ISM_TENANT_RMPOLICY_REQUSER;
        if (tenant->require_cert==1)
            tenant->rmPolicies |= ISM_TENANT_RMPOLICY_REQCERT;
        if (tenant->require_secure==1)
            tenant->rmPolicies |= ISM_TENANT_RMPOLICY_REQSECURE;
        if (tenant->require_cert==1 || tenant->serverCert) {
            int reqcert = tenant->require_cert==1;
            if (reqcert && tenant->require_user==1)
                reqcert = 3;     /* RequireCert and RequireUser */
            int snirc = ism_ssl_setSNIConfig(tenant->name, tenant->serverCert, tenant->serverKey,
                    tenant->keyPassword, tenant->caCerts, reqcert,
                    reqcert ? ism_transport_crlVerify: NULL);
            if (snirc) {
                /* log SNI config failure */
            		 ism_common_formatLastError(xbuf, sizeof xbuf);
            	 	 LOG(ERROR, Server, 992, "%s%-s%u", "Failed to set the SNI configuration: Org={0} Error={1} Code={2}",
            	                        tenant->name, xbuf, ism_common_getLastError());
            } else {
                tenant->useSNI = 1;
                tenant->rmPolicies |= ISM_TENANT_RMPOLICY_REQSNI;
            }
        } else {
            if (tenant->useSNI) {
                ism_ssl_removeSNIConfig(tenant->name);
                tenant->useSNI = 0;
            }
        }
    }

    /* Send ACL change when message routing changes */
    if (old_msgRouting != tenant->messageRouting && rc==0) {
        ism_proxy_changeMsgRouting(tenant, old_msgRouting);
    }

    tenant->rc = rc;
    if (rc) {
        tenant->enabled = 0;
        ism_common_formatLastError(xbuf, sizeof xbuf);
        LOG(ERROR, Server, 932, "%s%-s%u", "Organization configuration error.  No connections will be allowed: Org={0} Error={1} Code={2}",
                        tenant->name, xbuf, ism_common_getLastError());
    }
    if (!created || !rc) {
        if (created) {
            linkTenant(tenant);
        }
    } else {
        freeTenant(tenant);
    }
    return rc;
}
#endif

/*
 * Free a user object
 */
static void freeUser(ism_user_t * user) {
    if (user) {
        if (user->password) {
            ism_common_free(ism_memory_proxy_tenant,(char *)user->password);
            user->password = NULL;
        }
        if (user->name) {
            ism_common_free(ism_memory_proxy_tenant,(char *)user->name);
            user->name = NULL;
        }
    }
}

/*
 * Make a user object from the configuration.
 * TODO: validate the values
 */
int ism_tenant_makeUser(ism_json_parse_t * parseobj, int where, const char * name, ism_tenant_t * tenant, int checkonly, int keepgoing) {
    int endloc;
    ism_user_t * user;
    int  rc = 0;
    int  created = 0;
    char xbuf[1024];
    int needlog = 1;

    if (!parseobj || where > parseobj->ent_count)
        return 1;
    ism_json_entry_t * ent = parseobj->ent+where;
    endloc = where + ent->count;
    where++;
    if (!name)
        name = ent->name;
    user = ism_tenant_getUser(name, tenant, 0);
    if (user) {
        if (ent->objtype != JSON_Object) {
            unlinkUser(user, tenant);
            freeUser(user);
            return 0;
        }
    } else {
        if (ent->objtype == JSON_Object) {
            user = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tenant,13),1, sizeof(ism_tenant_t));
            strcpy(user->structid, "IoTUSR");
            user->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_tenant,1000),name);
            user->role = 0xffffff;
            created = 1;
        } else {
            if (ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "User", name);
                return ISMRC_BadPropertyValue;
            }
            TRACE(4, "User does not exist: %s\n", name);
            return 1;
        }
    }
    int savewhere = where;
    while (where <= endloc) {
        ent = parseobj->ent + where;
        if (!strcmpi(ent->name, "Password")) {
            if (ent->objtype != JSON_String) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Password", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "Role")) {
            if (ent->objtype != JSON_String) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Role", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else {
            LOG(ERROR, Server, 937, "%s%-s", "Unknown user property: User={0} Property={1}",
                    user->name, ent->name);
            needlog = 0;
            if (!keepgoing) {
                rc = ISMRC_BadPropertyName;
                ism_common_setErrorData(rc, "%s", ent->name);
            }
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }

    if (rc == 0) {
        where = savewhere;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            if (!strcmpi(ent->name, "Password")) {
                replaceString(&user->password, ent->value);
#if HAS_BRIDGE
                if (ent->value && *ent->value != '=')
                     g_need_dyn_write = 1;
#endif
            } else if (!strcmpi(ent->name, "Role")) {
                char * eos;
                uint32_t role = strtoul(ent->value, &eos, 16);
                if (!*eos)
                    user->role = role;
                else
                    TRACE(4, "Invalid user role: user=%s role=%s\n", user->name, ent->value);
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
#ifdef HAS_BRIDGE
        if (!user->password) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Password", "");
            rc = ISMRC_BadPropertyValue;
        }
#endif
    }

    if (rc) {
        if (needlog) {
            ism_common_formatLastError(xbuf, sizeof xbuf);
            LOG(ERROR, Server, 954, "%s%u%-s", "User configuration error: User={0} Error={2} Code={1}",
                                user->name, ism_common_getLastError(), xbuf);
        }
    }

    if (rc == 0) {
        if (created) {
            linkUser(user, tenant);
        }
    } else {
        if (!created) {
            unlinkUser(user, tenant);
        }
        freeUser(user);
    }
    return rc;
}

#ifndef NO_PROXY
/*
 * Replace an array of strings.
 * Type and count checking should already have been done.
 */
void replaceArray(const char * * values, ism_json_parse_t * parseobj, int where, int count) {
    int i;
    ism_json_entry_t * ent = parseobj->ent+where;
    for (i=0; i<count; i++) {
        replaceString(values+i, ent->value);
        ent++;
    }
}


/*
 * Check the monitor topic for validity.
 */
int checkMonitorTopic(const char * topic) {
    const char * cp = topic;
    int  slashes = 0;
    while (*cp) {
        char ch = *cp++;
        if (ch=='/') {
            if (slashes++ > 30)
                return 1;
        }
        if (ch == '#' || ch == '+') {
            return 1;
        }
    }
    return 0;
}


extern int ism_tcp_getIOPcount(void);

/*
 * Make a tenant object from the configuration.
 *
 * We allow the Address and Backup items to have up to 32 entries, but we no longer use
 * more than 2 entries.  The new preferred config is to use up to 2 Address entries and
 * not use Backup. More that this is allowed for legacy reasons.
 */
int ism_tenant_makeServer(ism_json_parse_t * parseobj, int where, const char * name) {
    int endloc;
    int endarray;
    int awhere;
    ism_server_t * server;
    int  rc = 0;
    int  created = 0;
    char xbuf[1024];
    ism_json_entry_t * aent;
    int need_restart = 0;
    int need_tls = 0;
    int need;
    int ipaddr_count = -1;
    int backup_count = -1;

    if (!parseobj || where > parseobj->ent_count)
        return 1;
    ism_json_entry_t * ent = parseobj->ent+where;
    endloc = where + ent->count;
    where++;
    if (!name)
        name = ent->name;
    server = ism_tenant_getServer(name);
    if (server) {
        if (ent->objtype != JSON_Object) {
            unlinkServer(server);
            /*
             * We do not free up the server, so this memory will be seen as lost.
             * The assumption is that servers are not deleted often, and we do
             * not delete it as it might still be in used.  At some point we can
             * add in code to check if the server is in use and free it when it is
             * not.
             */
            return 0;
        }
    } else {
        if (ent->objtype == JSON_Object) {
            server = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tenant,14),1, sizeof(ism_server_t));
            strcpy(server->structid, "IoTSRV");
            server->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_tenant,1000),name);
            server->getAddress = (ism_getAddress_f)ism_server_getAddress;
            created = 1;
            pthread_spin_init(&server->lock,0);
            pthread_spin_init(&server->mqttCon.lock, 0);
            server->monitor_qos = 0x10;
            server->monitor_retain = 0x10;
            server->monitor_connect = 0x10;
        } else {
            if (ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Server", name);
                return ISMRC_BadPropertyValue;
            }
            TRACE(2, "Server does not exist: %s\n", name);
            return 1;
        }
    }
    int savewhere = where;
    while (where <= endloc) {
        ent = parseobj->ent + where;
        if (!strcmpi(ent->name, "Address")) {
            if (ent->objtype == JSON_Null) {
                ipaddr_count = 0;
            } else {
                if (ent->objtype != JSON_Array || ent->count > 32) {
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Address", getJsonValue(ent));
                    rc = ISMRC_BadPropertyValue;
                } else {
                    ipaddr_count = ent->count;
                    awhere = where+1;
                    endarray = awhere + ent->count;
                    while (awhere < endarray) {
                        aent = parseobj->ent + awhere;
                        if (aent->objtype != JSON_String) {
                            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Address", getJsonValue(ent));
                            rc = ISMRC_BadPropertyValue;
                        }
                        awhere++;
                    }
                }
            }
        } else if (!strcmpi(ent->name, "Backup")) {
            if (ent->objtype == JSON_Null) {
                backup_count = 0;
            } else {
                if (ent->objtype != JSON_Array  || ent->count > 32) {
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Backup", getJsonValue(ent));
                    rc = ISMRC_BadPropertyValue;
                } else {
                    backup_count = ent->count;
                    awhere = where+1;
                    endarray = awhere + ent->count;
                    while (awhere < endarray) {
                        aent = parseobj->ent + awhere;
                        if (aent->objtype != JSON_String) {
                            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Backup", getJsonValue(ent));
                            rc = ISMRC_BadPropertyValue;
                        }
                        awhere++;
                    }
                }
            }
        } else if (!strcmpi(ent->name, "Port")) {
            if (ent->objtype != JSON_Integer || ent->count <= 0 || ent->count > 65535) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Port", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MaxMessageSize")) {
            if (ent->objtype != JSON_Integer || ent->count < 0) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MaxMessageSize", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "Monitor")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Monitor", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MeteringTopic")) {
            if (ent->objtype != JSON_String || checkMonitorTopic(ent->value)) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MeteringTopic", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MonitorTopic")) {
            if (ent->objtype != JSON_String || checkMonitorTopic(ent->value)) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MonitorTopic", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MonitorTopicAlt")) {
            if (ent->objtype != JSON_String || checkMonitorTopic(ent->value)) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MonitorTopicAlt", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MonitorQoS")) {
            if (ent->objtype != JSON_Integer || ent->count <= 0 || ent->count > 1) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MonitorQoS", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MonitorRetain")) {
            if (ent->objtype != JSON_Integer || ent->count <= 0 || ent->count > 2) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MonitorRetain", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "TLS")) {
            if (ent->objtype != JSON_String || ism_common_enumValue(enum_server_tls, ent->value)==INVALID_ENUM) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "TLS", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        }  else if (!strcmpi(ent->name, "SQSQueueURL")) {
            if (ent->objtype != JSON_String) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "SQSQueueURL", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MqttProxyProtocol")) {
            /* No longer used */
        } else {
            LOG(WARN, Server, 933, "%s%-s", "Unknown server property: Server={0} Property={1}",
                    server->name, ent->name);
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }

    if (rc == 0) {
        where = savewhere;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            if (!strcmpi(ent->name, "Port")) {
                if (server->port != ent->count)
                    need_restart = 1;
                server->port = ent->count;
            } else if (!strcmpi(ent->name, "Address")) {
                if (ipaddr_count > 2) {
                    ipaddr_count = 2;
                    /* TODO: log */
                }
                if (ipaddr_count > 0) {
                    aent = parseobj->ent + (where + 1);
                    need = replaceString(&server->address[0], aent->value);
                    if (need && server->last_good == 0)
                        need_restart = 1;
                    if (ipaddr_count == 2) {
                        aent++;
                        need = replaceString(&server->address[1], aent->value);
                        if (need && server->last_good == 1)
                            need_restart = 1;
                        backup_count = -1;
                    }
                } else {
                    if (ipaddr_count == 0) {
                        need = replaceString(&server->address[0], NULL);
                        if (need && server->last_good == 1)
                            need_restart = 1;
                    }
                }
                server->ipaddr_count = ipaddr_count;
                server->backup_count = 0;
            } else if (!strcmpi(ent->name, "Backup") && ent->objtype != JSON_Null) {
                if (backup_count > 1) {
                    backup_count = 1;
                }
                if (backup_count > 0) {
                		aent = parseobj->ent + (where + 1);
                    need = replaceString(&server->address[1], aent->value);
                    if (need && server->last_good == 1)
                        need_restart = 1;
                } else {
                    if (backup_count == 0) {
                        if (ipaddr_count == 1) {
                            need = replaceString(&server->address[1], NULL);
                            if (need && server->last_good == 1)
                                need_restart = 1;
                        }
                    }
                }
                if(backup_count > 0)
                		server->ipaddr_count+=backup_count;
                server->backup_count = 0;
            } else if (!strcmpi(ent->name, "MaxMessageSize")) {
                server->maxMsgSize = ent->count;
            } else if (!strcmpi(ent->name, "Monitor")) {
                uint8_t newmonitor = (ent->objtype == JSON_True) ? 3 : 0;
                if (newmonitor != server->monitor_connect)
                    need_restart = 1;
                server->monitor_connect = newmonitor;
            } else if (!strcmpi(ent->name, "MonitorRetain")) {
                server->monitor_retain = (uint8_t)ent->count;
            } else if (!strcmpi(ent->name, "MonitorQoS")) {
                server->monitor_qos = (uint8_t)ent->count;
            } else if (!strcmpi(ent->name, "MeteringTopic")) {
                replaceString(&server->metering_topic, ent->value);
            } else if (!strcmpi(ent->name, "MonitorTopic")) {
                replaceString(&server->monitor_topic, ent->value);
            } else if (!strcmpi(ent->name, "MonitorTopicAlt")) {
                replaceString(&server->monitor_topic_alt, ent->value);
            } else if (!strcmpi(ent->name, "TLS")) {
                uint8_t newtls = ism_common_enumValue(enum_server_tls, ent->value);
                if (newtls != server->useTLS)
                    need_tls = 1;
                server->useTLS = newtls;
            } else if (!strcmpi(ent->name, "SQSQueueURL")) {
            	server->serverKind = 2;
                replaceString(&server->awssqs_url, ent->value);
                TRACE(5, "SQS URL is specified url=%s\n", server->awssqs_url);
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
    } else {
        ism_common_formatLastError(xbuf, sizeof xbuf);
        LOG(ERROR, Server, 951, "%s%-s%u", "Server configuration error: Server={0} Error={1} Code={2}",
                            server->name, xbuf, ism_common_getLastError());
    }

    if (rc == 0) {
        if (tlsInited) {
            if (need_tls) {
                if (server->tlsCTX) {
                    // ism_common_destroy_ssl_ctx(server->tlsCTX);
                    server->tlsCTX = NULL;
                }
                if (server->useTLS) {
                    server->tlsCTX = ism_transport_clientTlsContext("server", "TLSv1.2",
                            server->useTLS == Server_Cipher_Best ? bestCipher : fastCipher);
                    if (!server->tlsCTX) {
                        TRACE(2, "Unable to create server TLS context: server=%s\n", server->name);
                    } else {
                        TRACE(5, "Create TLS context: server=%s method=%s ciphers=%s\n",
                                 server->name, "TLSv1.2", ism_common_enumName(enum_server_tls, server->useTLS));
                    }
                }
            }
            if (created) {
                linkServer(server);
                if (need_restart) {
                    server->need_start = 1;
                    need_restart = 0;
                }
            }
            if (need_restart) {
                ism_transport_closeServerConnection(server->name);
            } else {
                if (server->need_start && server->address[0] && server->port) {
                    /* Start server */
                    if (server->serverKind == 0) {
                        if (server->monitor_topic || server->metering_topic)
                            ism_monitor_startServerMonitoring(server);
                        if (ism_common_getIntConfig("MqttUseMux", 1)) {
                            int i;
                            int iopCount = ism_tcp_getIOPcount();
                            server->mux = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tenant,15),iopCount, sizeof(serverConnection_t));
                            for(i = 0; i < iopCount; i++)
                                pthread_spin_init(&server->mux[i].lock,0);
                            ism_transport_startMuxConnections(server);
                        }
                        if (httpProxyEnabled)
                            ism_server_startServerConnection(server);
#ifndef NO_SQS
                    } else if(server->serverKind==2) {
                        //AWS SQS Server
                        ism_proxy_sqs_init(server);
#endif
                    }
                }
            }
        }
    } else {
        if (!created) {
            unlinkServer(server);
        } else {
            freeServer(server);
        }
    }
    return rc;
}

/*
 * Command to print tenants
 */
void ism_tenant_printTenants(const char * pattern) {
    ism_tenant_lock();
    int i;
    for (i=0; i<g_tenant_buckets; i++) {
        ism_tenant_t * tenant = ismTenants[i];
        int nullonly = 0;
        if (!pattern)
            pattern = "*";
        if (pattern[0]=='.' && pattern[1]==0)
            nullonly = 1;
        while (tenant) {
            int selected = 0;
            if (nullonly) {
                if (tenant->name[0]==0)
                    selected = 1;
            } else {
                if (ism_common_match(tenant->name, pattern))
                    selected = 1;
            }
            if (selected) {
                printf("Tenant \"%s\" server=%s enabled=%u rc=%d topicrule=%s\n"
                       "    user_is_clientid=%d durable=%d shared=%d retain=%d maxqos=%d maxconnect=%u\n"
                       "    require_secure=%d require_cert=%d reqire_user=%d allow_anon=%d maxmsg=%d\n"
                       "    plan=%s fairuse=%s routing=%x maxsessionexpire=%d\n",
                   tenant->name, tenant->serverstr, tenant->enabled, tenant->rc, ism_proxy_getTopicRuleName(tenant->topicrule),
                   tenant->user_is_clientid, tenant->allow_durable, tenant->allow_shared, tenant->allow_retain, tenant->max_qos,
                   tenant->max_connects, tenant->require_secure, tenant->require_cert, tenant->require_user,
                   tenant->allow_anon, tenant->maxMsgSize, tenant->plan ? tenant->plan : "",
                   tenant->fairUsePolicy ? tenant->fairUsePolicy : "", tenant->messageRouting,
                   tenant->maxSessionExpire);
            }
            tenant = tenant->next;
        }
    }
    ism_tenant_unlock();
}
#endif

/*
 * Command to print users
 */
void ism_tenant_printUsers(const char * pattern) {
    int i;
    ism_tenant_lock();
    for (i=0; i<g_user_buckets; i++) {
        ism_user_t * user = ismUsers[i];
        if (!pattern)
            pattern = "*";
        while (user) {
            if (ism_common_match(user->name, pattern)) {
                const char * pw = "";
                if (user->password) {
                    if (*user->password == '=')
                        pw = user->password;
                    else
                        pw = "********";
                }
                printf("User \"%s\" password=\"%s\"", user->name, pw);
                if (user->role != 0xffffff) {
                    printf(" role=%x", user->role);
                }
                printf("\n");
            }
            user = user->next;
        }
    }
    ism_tenant_unlock();
}

#ifndef NO_PROXY
/*
 * Command to print users
 */
void ism_tenant_printTenantUsers(const char * args) {
    ism_user_t * user;
    ism_tenant_t * tenant;
    char * pattern;
    const char * tenantstr = ism_common_getToken((char *)args, " \t\r\n", " \t\r\n", &pattern);
    if (!pattern || !*pattern)
        pattern = "*";

    if (tenantstr && tenantstr[0]=='.' && tenantstr[1]==0)
        tenantstr = "";
    ism_tenant_lock();
    tenant = ism_tenant_getTenant(tenantstr);
    if (!tenant) {
    	ism_tenant_unlock();
        printf("Tenant is not found: '%s'\n", tenantstr ? tenantstr : "");
        return;
    }
    printf("Tenant \"%s\"\n", tenant->name);
    user = tenant->users;
    while (user) {
        if (ism_common_match(user->name, pattern)) {
            const char * pw = "";
            if (user->password) {
                if (*user->password == '=')
                    pw = user->password;
                else
                    pw = "********";
            }
            if (user->role == 0xffffff) {
                printf("User \"%s\" password=\"%s\"\n", user->name, pw);
            } else {
                printf("User \"%s\" password=\"%s\" role=%x\n", user->name, pw, user->role);
            }
        }
        user = user->next;
    }
    ism_tenant_unlock();
}

/*
 * Command to print server
 */
void ism_tenant_printServers(const char * pattern) {
    int i;

    ism_tenant_lock();
    ism_server_t * server = ismServers;
    if (!pattern)
        pattern = "*";
    while (server) {
        if (ism_common_match(server->name, pattern)) {
            printf("Server \"%s\" port=%u monitor=%s mon_retain=%d maxmsg=%u tls=%s\n",
                    server->name, server->port, server->monitor_connect ? "true" : "false",
                    server->monitor_retain&0x0F,  server->maxMsgSize,
                    ism_common_enumName(enum_server_tls, server->useTLS));
            if (server->monitor_topic) {
                printf("    monitor_topic=\"%s\"\n", server->monitor_topic);
            }
            if (server->monitor_topic_alt) {
                printf("    monitor_topic_alt=\"%s\"\n", server->monitor_topic_alt);
            }
            if (server->ipaddr_count) {
                printf("    address = [ ");
                for (i=0; i<server->ipaddr_count; i++) {
                    printf("%s ", server->address[i]);
                }
                printf("]\n");
            }
        }
        server = server->next;
    }
    ism_tenant_unlock();
}


/*
 * Callback at completion of getaddrinfo_a
 */
#ifdef GAI_SIG
static int addrinfo_callback(void * xtransport) {
    ism_transport_t * transport = (ism_transport_t *)xtransport;
    struct gaicb * req = (struct gaicb *)transport->getAddrCB;
    struct addrinfo * info = req->ar_result;

    int grc = gai_error(req);
    TRACE(9, "Server addrinfo_callback: connect=%u name=%s grc=%d\n", transport->index, transport->name, grc);
    if (grc != EAI_INPROGRESS) {
        if (grc == 0)
            grc = transport->slotused;
        transport->gotAddress(transport, grc, info);
        freeaddrinfo(info);
        ism_common_free(ism_memory_proxy_tenant,req);
        return -1;
    }
    return 0;
#else
static void addrinfo_callback(union sigval xtransport) {
    ism_transport_t * transport = (ism_transport_t *)xtransport.sival_ptr;
    struct gaicb * req = (struct gaicb *)transport->getAddrCB;
    struct sigevent * sigevt = sigevt = (struct sigevent *)(req+1);
    struct addrinfo * info = req->ar_result;

    int grc = gai_error(req);
    if (grc == 0)
        grc = transport->slotused;
    transport->gotAddress(transport, grc, info);
    freeaddrinfo(info);
    ism_common_free(ism_memory_proxy_tenant,req);           /* This includes the hints and sigevent */
#endif
}

/*
 * Get a server address from the ipaddr or backup lists
 */
int ism_server_getAddress(ism_server_t * server, ism_transport_t * transport, ism_gotAddress_f gotAddress) {
    int rc;
    struct gaicb * req = {0};
    struct sigevent * sigevt;
    struct addrinfo * hints;
    ism_handler_t handler;

    if (server)
        transport->server = server;
    else
        server = transport->server;
    if (gotAddress)
        transport->gotAddress = gotAddress;
    if (!server || !transport->gotAddress) {
        ism_common_setError(ISMRC_Error);
        return ISMRC_Error;
    }

    /* Put any info we need at completion of the address resolution into the transport object */
	pthread_spin_lock(&server->lock);
	int tryserver = (server->ipaddr_count > 1) ? server->last_good : 0;
    transport->server_addr = ism_transport_putString(transport, server->address[tryserver]);
    transport->serverport = server->port;
    pthread_spin_unlock(&server->lock);

    req = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_tenant,18),1, sizeof(*req)+sizeof(*sigevt)+sizeof(*hints)+16);
    sigevt = (struct sigevent *)(req+1);
    hints = (struct addrinfo *)(sigevt+1);
    transport->getAddrCB = req;

    hints->ai_family = AF_INET6;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_V4MAPPED;
    req->ar_name = transport->server_addr;
    req->ar_request = hints;
    req->__return = EAI_INPROGRESS;
    transport->slotused = server->last_good;
#ifdef GAI_SIG
    sigevt->sigev_notify = SIGEV_SIGNAL;
    sigevt->sigev_signo = ism_common_userSignal();
    handler = ism_common_addUserHandler(addrinfo_callback, transport);
#else
    sigevt->sigev_notify = SIGEV_THREAD;
    sigevt->sigev_value.sival_ptr = (void *)transport;
    sigevt->sigev_notify_function = addrinfo_callback;
#endif
    rc = getaddrinfo_a(GAI_NOWAIT, &req, 1, sigevt);
    TRACE(9, "ism_server_getAddress: connect=%u name=%s server=%s addr=%s rc=%d grc=%d handler=%p\n",
            transport->index, transport->name, server->name, req->ar_name, rc, gai_error(req), handler);

    if (rc) {
#ifdef GAI_SIG
        ism_common_removeUserHandler(handler);
#endif
        ism_common_free(ism_memory_proxy_tenant,transport->getAddrCB);
        transport->getAddrCB = NULL;
        ism_common_setErrorData(ISMRC_Error, "%s%i", "getaddrinfo_a", rc);
        return ISMRC_Error;
    }
    return 0;
}
#endif


/*
 * Set the last good
 */
XAPI void ism_server_setLastGoodAddress(ism_server_t * server, int isBackup) {
#ifndef NO_PROXY
    //This function valid for the following server:
    // * MessageSight Server. Server Kind for MessageSight server is zero.
    //Note: For Kafka, the kind is 1, and has more than 1 servers.
    //The assign of last_good will come ism_kafka_getAddress function
    if (server->serverKind < 1 && server->ipaddr_count > 1) {
        pthread_spin_lock(&server->lock);
        int oldGood = server->last_good;
        server->last_good = isBackup;
        pthread_spin_unlock(&server->lock);
        if (oldGood != isBackup) {
            TRACE(4, "Set last good server: server=%s oldGood=%d newGood=%d\n", server->name, oldGood, isBackup);
        }
    }
#endif
}


/*
 *
 */
XAPI void ism_server_setLastBadAddress(ism_server_t * server, int isBackup) {
#ifndef NO_PROXY
    //This function valid for the following server:
    // * MessageSight Server. Server Kind for MessageSight server is zero.
    //Note: For Kafka, the kind is 1, and has more than 1 servers.
    //The assign of last_good will come ism_kafka_getAddress function
    if (server->serverKind < 1 && server->ipaddr_count > 1) {
        pthread_spin_lock(&server->lock);
        int oldGood = server->last_good;
        server->last_good = !isBackup;
        pthread_spin_unlock(&server->lock);
        if (oldGood == isBackup) {
            TRACE(4, "Set last bad server: server=%s oldGood=%d newGood=%d\n", server->name, oldGood, !isBackup);
        }
    }
#endif
}


#ifndef NO_PROXY
/*
 * Put out a tenant in JSON form
 */
int ism_tenant_getTenantJson(ism_tenant_t * tenant, ism_json_t * jobj, const char * name) {
    ism_json_startObject(jobj, name);

    ism_json_putBooleanItem(jobj, "Enabled", tenant->enabled);
    if (tenant->serverstr)
        ism_json_putStringItem(jobj, "Server", tenant->serverstr);
    if (tenant->require_secure != 2)
        ism_json_putBooleanItem(jobj, "RequireSecure", tenant->require_secure);
    ism_json_putStringItem(jobj, "Authenticate", ism_common_enumName(enum_tenant_auth, tenant->authenticate));
    if (tenant->remove_user != 2)
        ism_json_putBooleanItem(jobj, "RemoveUser", tenant->remove_user);
    if (tenant->check_user != 2)
        ism_json_putBooleanItem(jobj, "CheckUser", tenant->check_user);
    if (tenant->user_is_clientid != 2)
        ism_json_putBooleanItem(jobj, "UserIsClientID", tenant->user_is_clientid);
    if (tenant->disableCRL != 2)
        ism_json_putBooleanItem(jobj, "DisableCRL", tenant->disableCRL);
    if (tenant->checkSessionUser != 2)
        ism_json_putBooleanItem(jobj, "CheckSessionUser", tenant->checkSessionUser);
#ifdef PX_CLIENTACTIVITY        
    if (tenant->pxactEnabled != 2)
    	ism_json_putBooleanItem(jobj, "ClientActivityEnabled", tenant->pxactEnabled);
#endif
    if (tenant->sgEnabled != 2)
        ism_json_putBooleanItem(jobj, "SGEnabled", tenant->sgEnabled);
    if (tenant->allow_durable != 2)
        ism_json_putBooleanItem(jobj, "AllowDurable", tenant->allow_durable);
    if (tenant->allow_shared != 2)
        ism_json_putBooleanItem(jobj, "AllowShared", tenant->allow_shared);
    if (tenant->allow_systopic != 2)
        ism_json_putBooleanItem(jobj, "AllowSysTopic", tenant->allow_systopic);
    if (tenant->allow_retain != 2)
        ism_json_putBooleanItem(jobj, "AllowRetain", tenant->allow_retain);
    ism_json_putIntegerItem(jobj, "MaxQoS", tenant->max_qos);
    if (tenant->max_connects)
        ism_json_putIntegerItem(jobj, "MaxConnections", tenant->max_connects);
    if (tenant->maxMsgSize)
        ism_json_putIntegerItem(jobj, "MaxMessageSize", tenant->maxMsgSize);
    if (tenant->rlacAppResourceGroupEnabled == 1)
        ism_json_putBooleanItem(jobj, "RLACAppEnabled", tenant->rlacAppResourceGroupEnabled);
    if (tenant->rlacAppDefaultGroup == 1)
        ism_json_putBooleanItem(jobj, "RLACAppDefaultGroup", tenant->rlacAppDefaultGroup);
    if (tenant->messageRouting)
        ism_json_putIntegerItem(jobj, "MessageRouting", tenant->messageRouting);
    if (tenant->plan)
        ism_json_putStringItem(jobj, "Plan", tenant->plan);
    if (tenant->fairUsePolicy)
		ism_json_putStringItem(jobj, "FairUsePolicy", tenant->fairUsePolicy);
    if (tenant->topicrule)
        ism_json_putStringItem(jobj, "CheckTopic", ism_proxy_getTopicRuleName(tenant->topicrule));
    if (tenant->serverCert)
        ism_json_putStringItem(jobj, "ServerCertificate", tenant->serverCert);
    if (tenant->serverKey)
        ism_json_putStringItem(jobj, "ServerKey",tenant->serverKey);
    if (tenant->keyPassword)
        ism_json_putStringItem(jobj, "KeyPassword", tenant->keyPassword);
    if (tenant->caCerts)
        ism_json_putStringItem(jobj, "CACertificates", tenant->caCerts);
    if (tenant->users) {
        ism_tenant_getUserList("*", tenant, jobj, 2, "User");
    }
    ism_json_endObject(jobj);
    return 0;
}


/*
 * Put out a tenant list in JSON form
 */
int ism_tenant_getTenantList(const char * match, ism_json_t * jobj, int json, const char * name) {
    int   i;
    ism_tenant_t * tenant;

    if (json)
        ism_json_startObject(jobj, name);
    else
        ism_json_startArray(jobj, name);

    ism_tenant_lock();
    for (i=0; i<g_tenant_buckets; i++) {
        tenant = ismTenants[i];
        while (tenant) {
            if (ism_common_match(tenant->name, match) &&
                (tenant->name[0] != '!' || *match == '!')) {
                if (json) {
                    ism_tenant_getTenantJson(tenant, jobj, tenant->name);
                } else {
                    ism_json_putStringItem(jobj, NULL, tenant->name);
                }
            }
            tenant = tenant->next;
        }

    }
    ism_tenant_unlock();

    if (json)
        ism_json_endObject(jobj);
    else
        ism_json_endArray(jobj);
    return 0;
}

#endif


/*
 * Put out an endpoint in JSON form
 */
int ism_tenant_getEndpointJson(ism_endpoint_t * endpoint, ism_json_t * jobj, const char * name) {
    char protos [256];


    *protos = 0;
    if (endpoint->protomask & PMASK_JMS) {
        strcat(protos, "JMS");
    }
    if (endpoint->protomask & PMASK_MQTT) {
        if (*protos)
            strcat(protos, ",");
        strcat(protos, "MQTT");
    }
    if (endpoint->protomask & PMASK_Admin) {
        if (*protos)
            strcat(protos, ",");
        strcat(protos, "Admin");
    }

    ism_json_startObject(jobj, name);

    ism_json_putIntegerItem(jobj, "Port", endpoint->port);
    if (endpoint->ipaddr) {
        ism_json_putStringItem(jobj, "Interface", endpoint->ipaddr);
    }
    ism_json_putBooleanItem(jobj, "Enabled", endpoint->enabled);
    if (endpoint->separator  && endpoint->separator != ':' && !endpoint->clientclass) {
        char xx [2];
        xx[0] = endpoint->separator;
        xx[1] = 0;
        ism_json_putStringItem(jobj, "DomainSeparator", xx);
    }
    if (endpoint->clientclass)
        ism_json_putStringItem(jobj, "ClientClass", endpoint->clientclass);
    if (endpoint->secure < 2)
         ism_json_putBooleanItem(jobj, "Secure", endpoint->secure);
    ism_json_putStringItem(jobj, "Protocol", protos);
    if (endpoint->clientcipher < 2)
        ism_json_putBooleanItem(jobj, "UseClientCipher", endpoint->clientcipher);
    if (endpoint->authorder[0]==AuthType_Basic)
        ism_json_putStringItem(jobj, "Authentication", "basic");
#ifndef NO_PROXY
    if (endpoint->clientcert < 2)
        ism_json_putBooleanItem(jobj, "UseClientCertificate", endpoint->clientcert);
#endif
    if (endpoint->ciphertype) {
        if (endpoint->ciphertype == CipherType_Custom) {
            ism_json_putStringItem(jobj, "Ciphers", endpoint->ciphers);
        } else {
            ism_json_putStringItem(jobj, "Ciphers", ism_common_enumName(enum_ciphers, endpoint->ciphertype));
        }
    }
    if (endpoint->tls_method)
        ism_json_putStringItem(jobj, "Method", ism_common_enumName(enum_methods, endpoint->tls_method));
    if (endpoint->maxMsgSize)
        ism_json_putIntegerItem(jobj, "MaxMessageSize",  endpoint->maxMsgSize);
    if (endpoint->enableAbout < 2)
        ism_json_putBooleanItem(jobj, "EnableAbout", endpoint->enableAbout);
    if (endpoint->cert)
        ism_json_putStringItem(jobj, "Certificate", endpoint->cert);
    if (endpoint->key)
        ism_json_putStringItem(jobj, "Key", endpoint->key);
    if (endpoint->keypw)
        ism_json_putStringItem(jobj, "KeyPassword", endpoint->keypw);
    ism_json_endObject(jobj);
    return 0;
}

#ifndef NO_PROXY
/*
 * Put out an server in JSON form
 */
int ism_tenant_getServerJson(ism_server_t * server, ism_json_t * jobj, const char * name) {
    int  i;

    ism_json_startObject(jobj, name);
    ism_json_putIntegerItem(jobj, "Port", server->port);
    ism_json_putStringItem(jobj, "TLS", ism_common_enumName(enum_server_tls, server->useTLS));
    if (server->monitor_connect < 0x10)
        ism_json_putBooleanItem(jobj, "Monitor", server->monitor_connect);
    if (server->monitor_retain < 0x10)
        ism_json_putIntegerItem(jobj, "MonitorRetain", server->monitor_retain);
    if (server->monitor_qos < 3)
        ism_json_putIntegerItem(jobj, "MonitorQoS", server->monitor_qos);
    if (server->monitor_topic)
        ism_json_putStringItem(jobj, "MonitorTopic", server->monitor_topic);
    if (server->monitor_topic_alt)
        ism_json_putStringItem(jobj, "MonitorTopicAlt", server->monitor_topic_alt);
    if (server->metering_topic)
       ism_json_putStringItem(jobj, "MeteringTopic", server->metering_topic);
    if (server->maxMsgSize)
        ism_json_putIntegerItem(jobj, "MaxMessageSize", server->maxMsgSize);

    if (server->ipaddr_count) {
        ism_json_startArray(jobj, "Address");
        for (i=0; i<server->ipaddr_count; i++) {
            ism_json_putStringItem(jobj, NULL, server->address[i]);
        }
        ism_json_endArray(jobj);
    }
    ism_json_endObject(jobj);
    return 0;
}


/*
 * Put out a tenant list in JSON form
 */
int ism_tenant_getServerList(const char * match, ism_json_t * jobj, int json, const char * name) {
    ism_server_t * server;

    if (json)
        ism_json_startObject(jobj, name);
    else
        ism_json_startArray(jobj, name);

    ism_tenant_lock();
    server = ismServers;
    while (server) {
        if (ism_common_match(server->name, match)) {
            if (json) {
                ism_tenant_getServerJson(server, jobj, server->name);
            } else {
                ism_json_putStringItem(jobj, NULL, server->name);
            }
        }
        server = server->next;
    }
    ism_tenant_unlock();

    if (json)
        ism_json_endObject(jobj);
    else
        ism_json_endArray(jobj);
    return 0;
}

/*
 * Get the server statistics
 */
int ism_tenant_getServerStats(const char * match, ism_json_t * jobj) {
    ism_server_t * server;
    char tbuf[64];

    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_tenant_lock();
    ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 7, ISM_TFF_ISO8601);
    ism_common_closeTimestamp(ts);

    ism_json_startObject(jobj, NULL);
    ism_json_putStringItem(jobj, "Timestamp", tbuf);
    ism_json_startObject(jobj, "Server");

    server = ismServers;
    while (server) {
        if (ism_common_match(server->name, match)) {
            ism_json_startObject(jobj, server->name);
            ism_json_putBooleanItem(jobj, "Backup", 0); /* TODO: HA support */
            ism_json_putIntegerItem(jobj, "AddressCount", server->ipaddr_count);
            ism_json_putBooleanItem(jobj, "MonitorConnected", server->monitor && server->monitor->state == ISM_TRANST_Open);
            ism_json_endObject(jobj);
        }
        server = server->next;
    }
    ism_tenant_unlock();
    ism_json_endObject(jobj);
    ism_json_endObject(jobj);
    return 0;
}
#endif

/*
 * Put out an user in JSON form
 */
int ism_tenant_getUserJson(ism_user_t * user, ism_json_t * jobj, const char * name) {
    char hexbuf [32];
    int  saveindent;

    ism_json_startObject(jobj, name);
    saveindent = jobj->indent;
    jobj->indent = 0;
    if (user->password && *user->password != '=') {
        int obfus_len = strlen(user->password) * 3 + 64;
        char * obfus = alloca(obfus_len);
        ism_tenant_createObfus(user->name, user->password, obfus, obfus_len-1, 1);
        ism_json_putStringItem(jobj, "Password", obfus);
    } else {
        ism_json_putStringItem(jobj, "Password", user->password);
    }

    if (user->role != 0xffffff) {
        sprintf(hexbuf, "%X", user->role);
        ism_json_putStringItem(jobj, "Role", hexbuf);
    }
    ism_json_endObject(jobj);
    jobj->indent = saveindent;
    return 0;
}

/*
 * Put out a user list in JSON form
 */
int ism_tenant_getUserList(const char * match, ism_tenant_t * tenant, ism_json_t * jobj, int json, const char * name) {
    int   i;
    ism_user_t * user;
    if (json)
        ism_json_startObject(jobj, name);
    else
        ism_json_startArray(jobj, name);

    if (json != 2)
        ism_tenant_lock();
    if (tenant) {
        user = tenant->users;
        while (user) {
            if (ism_common_match(user->name, match)) {
                if (json) {
                    ism_tenant_getUserJson(user, jobj, user->name);
                } else {
                    ism_json_putStringItem(jobj, NULL, user->name);
                }
            }
            user = user->next;
        }
    } else {
        for (i=0; i<g_user_buckets; i++) {
            user = ismUsers[i];
            while (user) {
                if (ism_common_match(user->name, match)) {
                    if (json) {
                        ism_tenant_getUserJson(user, jobj, user->name);
                    } else {
                        ism_json_putStringItem(jobj, NULL, user->name);
                    }
                }
                user = user->next;
            }
        }
    }

    if (json != 2)
        ism_tenant_unlock();

    if (json)
        ism_json_endObject(jobj);
    else
        ism_json_endArray(jobj);
    return 0;
}

#ifndef NO_PROXY
extern uint16_t ism_server_pendingHTTPRequestCount(ism_server_t * server);
extern int ima_monitor_getMonitorState(ism_server_t* server);
int ism_proxy_getServersStats(px_server_stats_t * stats, int * pCount) {
    ism_server_t * pServer;
    int count = 0;
    ism_tenant_lock();
    if(*pCount < ismServersCount){
        *pCount = ismServersCount;
        ism_tenant_unlock();
        return 1;
    }
    for(pServer = ismServers; pServer != NULL; pServer = pServer->next) {
        /* Only stats for MQTT servers which are started */
        if (pServer->serverKind  || pServer->need_start || !pServer->ipaddr_count)
            continue;

        stats[count].name = pServer->name;
        stats[count].primaryIPs[0] = pServer->address[0];
        stats[count].primaryUseCount[0] = 0;  /* TODO */
        stats[count].primaryCount = 1;
        if (pServer->ipaddr_count > 1) {
            stats[count].backupIPs[0] = pServer->address[1];
            stats[count].backupUseCount[0] = 0;   /* TODO */
            stats[count].backupCount = 1;
        }
        stats[count].usePrimary = !pServer->last_good;
        stats[count].connectionState = (ima_monitor_getMonitorState(pServer) << 4) & 0xf0;
        if (pServer->mqttCon.disabled) {
            stats[count].connectionState += 0x0f;
            stats[count].pendingHTTPRequests = 0;
        } else {
            stats[count].connectionState += pServer->mqttCon.state;
            stats[count].pendingHTTPRequests = ism_server_pendingHTTPRequestCount(pServer);
        }
        stats[count++].port = pServer->port;
    }
    *pCount = count;
    ism_tenant_unlock();
    return 0;
}
#endif
