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
#ifndef TRACE_COMP
#define TRACE_COMP Transport
#endif

#include <ismutil.h>
#include <ismjson.h>
#include <pxtransport.h>
#include "pxtcp.h"
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <openssl/opensslv.h>

int g_useBufferPool = 1;

extern int ism_transport_getTcpMax(void);
extern int ism_pxactivity_startMessaging();
int ism_common_isBridge(void) ;

extern void ism_proxy_sqs_term(void);
extern int g_need_dyn_write;

static int                 FIPSmode = 0;
static int                 g_messaging_started = 0; // 0=not started, 1=started, 2=Do not start

/*
 * TLS methods enumeration
 */
static ism_enumList enum_methods [] = {
#if OPENSSL_VERSION_NUMBER < 0x10101000L
    { "Method",    5,                 },
#else
    { "Method",    6,                 },
#endif
    { "SSLv3",     SecMethod_SSLv3,   },
    { "TLSv1",     SecMethod_TLSv1,   },
    { "TLSv1.1",   SecMethod_TLSv1_1, },
    { "TLSv1.2",   SecMethod_TLSv1_2, },
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
    { "TLSv1.3",   SecMethod_TLSv1_3, },
#endif
    { "None",      0,                 },
};


/*
 * Cipher enumeration
 */
static ism_enumList enum_ciphers [] = {
    { "Ciphers",    4,                  },
    { "Best",       CipherType_Best,   },
    { "Fast",       CipherType_Fast,   },
    { "Medium",     CipherType_Medium, },
    { "None",       0,                 },
};
/*
 * Cipher enumeration
 */
static ism_enumList enum_ciphers_out [] = {
    { "Ciphers",    5,                  },
    { "Best",       CipherType_Best,   },
    { "Fast",       CipherType_Fast,   },
    { "Medium",     CipherType_Medium, },
    { "Custom",     CipherType_Custom, },
    { "None",       0,                 },
};

/*
 * Authentication types
 */
static ism_enumList enum_auth [] = {
    { "Authentication",  4,                 },
    { "Username",        AuthType_Username, },
    { "Basic",           AuthType_Basic,    },
    { "Cert",            AuthType_Cert,     },
    { "None",            AuthType_None,     },
};

/*
 * Dummy endpoint so we do not segfault
 */
static ism_endpoint_t nullEndpoint = {
    .name        = "!None",
    .ipaddr      = "",
    .transport_type = "TCP",
    .protomask   = PMASK_Internal,
    .thread_count = 1,
};

static ism_endstat_t outStat = { 0 };
/*
 * Dummy endpoint so we do not segfault
 */
static ism_endpoint_t outEndpoint = {
    .name        = "!Proxy",
    .ipaddr      = "",
    .transport_type = "TCP",
    .protomask   = PMASK_Internal,
    .thread_count = 1,
    .stats = &outStat,
};

/*
 * Define a list of protocol handlers.
 * When we need to find a protocol, we call each one until we get one which accepts
 * the protocol string in the transport.
 */
struct protocol_chain {
    struct protocol_chain *   next;
    ism_transport_register_t  regcall;
};
static struct protocol_chain * protocols = NULL;


/*
 * Chain of endpoints
 */
static ism_endpoint_t * endpoints = NULL;
static int endpoint_count = 0;

/*
 * Old endpoints which are no longer used.  We do not delete these as objects may have pointers to them
 */
static ism_endpoint_t * old_endpoints = NULL;


#define TOBJ_INIT_SIZE  1536
static ism_byteBufferPool tObjPool = NULL;
#ifdef HAS_BRIDGE
extern int g_need_dyn_write;
#endif

/*
 * Forward references
 */
static void linkEndpoint(ism_endpoint_t * endpoint);
static int unlinkEndpoint(const char * name);
int ism_transport_config_json(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing);
extern char ism_common_getNumericSeparator(void);
extern void ism_ssl_init(int useFips, int useBufferPool);
extern void ism_tenant_init(void);
extern void ism_transport_ackwaitInit(void);
static int getUnitSize(const char * ssize);
static const char * makeCiphers(int ciphertype, int method, char * buf, int len);
ism_topicrule_t * ism_proxy_getTopicRule(const char * name);
int ism_tenant_initServerTLS(void);
void ism_transport_closeAllConnections(int notAdmin, int notKafka);
const char * ism_transport_makepw(const char * data, char * buf, int len, int dir);

/*
 * Init for the WebSockets framer
 */
void ism_transport_wsframe_init(void);

/*
 * Return if we are in FIPS mode.
 * If FIPS mode is modified, the server must be restarted.
 */
int ism_transport_getFIPSmode(void) {
    return FIPSmode;
}

/*
 * strcmp with NULL handling
 */
xUNUSED static int mystrcmp(const char * s1, const char * s2) {
    if (s1 == NULL && s2 == NULL)
        return 0;
    if (s1 == NULL)
        return -1;
    if (s2 == NULL)
        return 1;
    return strcmp(s1, s2);
}

int ism_transport_initTransportBufferPool(void) {
	tObjPool = ism_common_createBufferPool(TOBJ_INIT_SIZE, 10240, 1024*1024,"TransportBufferPool");
	return 0;
}

/*
 * Initialize the transport component
 */
int ism_transport_init(void) {
    int sslUseBufferPool;

    /* Init monitoring */
    ism_tenant_init();
#ifndef NO_PROXY
    g_useBufferPool = ism_common_getBooleanConfig("UseBufferPool", 1);
#else
    g_useBufferPool = ism_common_getBooleanConfig("UseBufferPool", 1);
#endif
    if (g_useBufferPool) {
    		ism_transport_initTransportBufferPool();
    }

    /*
     * Initial set of FIPS mode
     */
    FIPSmode = ism_common_getBooleanConfig("FIPS", 0);
    sslUseBufferPool = ism_common_getBooleanConfig("TlsUseBufferPool", 0);
    TRACE(3, "Initialize transport. FIPS=%u\n", FIPSmode);
    ism_ssl_init(FIPSmode, sslUseBufferPool);
    ism_transport_ackwaitInit();

    /*
     * Initialize TCP
     */
    ism_transport_initTCP();
    return 0;
}


/*
 * Get the TLS method from the transport object
 */
enum ism_SSL_Methods ism_transport_getTLSMethod(ism_transport_t * transport) {
    int method = 0;
    int version;
    if (!transport || !transport->ssl)
        return 0;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    version = transport->ssl->version;
#else
    version = SSL_version(transport->ssl);
#endif
    switch(version) {
    case TLS1_2_VERSION:  method = SecMethod_TLSv1_2;   break;
    case 0x0304  :        method = SecMethod_TLSv1_3;   break;
    case TLS1_VERSION:    method = SecMethod_TLSv1;     break;
    case TLS1_1_VERSION:  method = SecMethod_TLSv1_1;   break;
    }
    return method;
}


/*
 * The the TLS method as a string from the transport object
 */
const char * ism_transport_getTLSMethodName(ism_transport_t * transport) {
    int method = (int)ism_transport_getTLSMethod(transport);
    return ism_common_enumName(enum_methods, method);
}


/*
 * Configuration callback for transport
 */
int ism_transport_config_json(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing) {
    int   rc = 0;
    ism_json_entry_t * ent;
    int endloc;

    ism_tenant_lock();

    if (!parseobj || where > parseobj->ent_count) {
        TRACE(2, "Transport config JSON not correct\n");
        ism_tenant_unlock();
        return 1;
    }
    ent = parseobj->ent+where;
    if (!ent->name || strcmp(ent->name, "Endpoint") || ent->objtype != JSON_Object) {
        TRACE(2, "Transport config JSON invoked for config which is not an endpoint\n");
        ism_tenant_unlock();
        return 2;
    }
    endloc = where + ent->count;
    where++;
    ent++;
    while (where <= endloc) {
        int xrc = ism_proxy_makeEndpoint(parseobj, where, NULL, checkonly, keepgoing);
        if (rc == 0)
            rc = xrc;
        ent = parseobj->ent+where;
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
 * Notify transport that the connection is ready for processing
 */
void ism_transport_connectionReady(ism_transport_t * transport) {
    /* Log the connection now we know the client ID and user ID */
	if(ism_common_conditionallyLogged(NULL, ISM_LOGLEV(INFO), ISM_MSG_CAT(Connection), 1117, transport->trclevel, transport->name, transport->client_addr, NULL) == 0){
		LOG(INFO, Connection, 1117, "%u%-s%-s%-s%s%-s%s%u",
			"Create {4} connection: ConnectionID={0} ClientID={1} Endpoint={2} UserID={3} CommonName={5} From={6}:{7}.",
						transport->index, transport->name, transport->endpoint->name,
						transport->userid ? transport->userid : "",
						transport->protocol_family,
						transport->cert_name ? transport->cert_name : "",
						transport->client_host, transport->clientport);
	}
}


/*
 * Simple lower case (ASCII7 only)
 */
static char * my_strlwr(char * cp) {
    char * buf = cp;
    if (!cp)
        return NULL;

    while (*cp) {
        if (*cp >= 'A' && *cp <= 'Z')
            *cp = (char)(*cp|0x20);
        cp++;
    }
    return buf;
}

/*
 * Parse a protocol string
 */
uint32_t parseProtocols(const char * str) {
    int count = 0;
    uint32_t ret = 0;
    if (!str)
        return 0;
    char * s = alloca(strlen(str)+1);
    strcpy(s, str);
    my_strlwr(s);
    char * token = ism_common_getToken(s, " \t,", " \t,", &s);
    while (token) {
        if (!strcmp(token, "mqtt")) {
            count++;
            ret |= PMASK_MQTT;
        } else if (!strcmp(token, "admin")) {
            count++;
            ret |= PMASK_Internal;
        }
        token = ism_common_getToken(s, " \t,", " \t,", &s);
    }
#ifndef HAS_BRIDGE
    if (count == 0) {
        ret = PMASK_MQTT;
    }
#endif
    return ret;
}

/*
 * Parse a transport string.
 */
uint32_t parseTransports(const char * str) {
     int count = 0;
     uint32_t ret = 0;
     if (!str)
         return 0;
     char * s = alloca(strlen(str)+1);
     strcpy(s, str);
     my_strlwr(s);
     char * token = ism_common_getToken(s, " \t,", " \t,", &s);
     while (token) {
         if (!strcmp(token, "tcp"))  {
             count++;
             ret |= TMASK_TCP;
         } else if (!strcmp(token, "websockets")) {
             count++;
             ret |= TMASK_WebSockets;
         }
         token = ism_common_getToken(s, " \t,", " \t,", &s);
     }
     if (count == 0) {
         ret = TMASK_AnyTrans;
     }
     return ret;
}

/*
 * Replace a string
 */
static void replaceString(const char * * oldval, const char * val) {
    if (*oldval) {
        char * freeval = (char *)*oldval;
        if (!strcmp(freeval, val))
            return;
        *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_utils,1000),val);
        ism_common_free(ism_memory_proxy_utils,freeval);
    } else {
        *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_utils,1000),val);
    }
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

/* In Bridge we strictly define the case of the Endpoint object items */
#ifdef NO_PROXY
#define strcmpix strcmp
#else
#define strcmpix strcasecmp
#endif

/*
 * Make a endpoint object from the configuration.
 * TODO: validate the values
 */
int ism_proxy_makeEndpoint(ism_json_parse_t * parseobj, int where, const char * name, int checkonly, int keepgoing) {
    int endloc;
    ism_endpoint_t * endpoint;
    int  port = 0;
    int  rc = 0;
    int  created = 0;
    char xbuf[1024];
    int  needlog = 1;

    if (!parseobj || where > parseobj->ent_count)
        return 1;
    ism_json_entry_t * ent = parseobj->ent+where;
    endloc = where + ent->count;
    where++;
    if (!name)
        name = ent->name;
    endpoint = ism_transport_getEndpoint(name);
    if (endpoint) {
        if (ent->objtype != JSON_Object) {
            endpoint->enabled = 0;
            endpoint->needed = ENDPOINT_NEED_ALL;
            if (g_messaging_started == 1)
                ism_transport_startTCPEndpoint(endpoint);
            endpoint->needed = 0;
            unlinkEndpoint(name);
            return 0;
        } else {
            port = endpoint->port;
        }
    } else {
        if (ent->objtype == JSON_Object) {
            endpoint = ism_transport_createEndpoint(name, 1);
            endpoint->enabled = 1;
            endpoint->secure = 2;
            endpoint->clientcipher = 2;
            endpoint->clientcert = 2;
            endpoint->enableAbout = 2;
#ifdef HAS_BRIDGE
            endpoint->protomask = PMASK_Admin;
#else
            endpoint->protomask = PMASK_MQTT;
#endif
            endpoint->transmask = TMASK_AnyTrans;
            created = 1;
        } else {
            if (ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Endpoint", name);
                return ISMRC_BadPropertyValue;
            }
            TRACE(4, "Endpoint does not exist: %s\n", name);
            return 1;
        }
    }
    int savewhere = where;
    while (where <= endloc) {
        ent = parseobj->ent + where;
        if (!strcmpix(ent->name, "Port")) {
            if (ent->objtype != JSON_Integer || ent->count <= 0 || ent->count > 65535) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Port", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            } else {
                port = ent->count;
            }
        } else if (!strcmpix(ent->name, "Interface")) {
            if (ent->objtype != JSON_String) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Interface", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
#ifndef NO_PROXY
        } else if (!strcmpi(ent->name, "DomainSeparator")) {
            if ((ent->objtype != JSON_String || strlen(ent->value)>1) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "DomainSeparator", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "ClientClass")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ClientClass", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
#endif
        } else if (!strcmpix(ent->name, "Secure")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Secure", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpix(ent->name, "Enabled")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Enabled", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpix(ent->name, "UseClientCipher")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "UseClientCipher", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
#ifndef NO_PROXY
        } else if (!strcmpi(ent->name, "UseClientCertificate")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "UseClientCertificate", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
#endif
        } else if (!strcmpix(ent->name, "Ciphers")) {
            if (ent->objtype != JSON_String ||
                    (ism_common_enumValue(enum_ciphers, ent->value)==INVALID_ENUM && *ent->value != ':') ) {
                if (ent->objtype != JSON_Null) {
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Ciphers", getJsonValue(ent));
                    rc = ISMRC_BadPropertyValue;
                }
            }
        } else if (!strcmpix(ent->name, "Method")) {
            if (ent->objtype != JSON_String || ism_common_enumValue(enum_methods, ent->value)==INVALID_ENUM) {
                if (ent->objtype != JSON_Null) {
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Method", getJsonValue(ent));
                    rc = ISMRC_BadPropertyValue;
                }
            }
        } else if (!strcmpix(ent->name, "Transport")) {
            if ((ent->objtype != JSON_String || parseTransports(ent->value)==0) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Transport", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpix(ent->name, "Protocol")) {
            if ((ent->objtype != JSON_String || parseProtocols(ent->value)==0) && ent->objtype != JSON_Null ) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Protocol", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpix(ent->name, "MaxMessageSize")) {
            if (ent->objtype != JSON_Integer) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Interface", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpix(ent->name, "Authentication")) {
            if (ent->objtype != JSON_String || ism_common_enumValue(enum_auth, ent->value)==INVALID_ENUM) {
                if (ent->objtype != JSON_Null) {
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Authentication", getJsonValue(ent));
                    rc = ISMRC_BadPropertyValue;
                }
            }
        } else if (!strcmpix(ent->name, "EnableAbout")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "EnableAbout", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpix(ent->name, "Certificate")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Certificate", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpix(ent->name, "Key")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Key", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpix(ent->name, "KeyPassword")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KeyPassword", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else {
            LOG(ERROR, Server, 925, "%s%-s", "Unknown endpoint property: Endpoint={0} Property={1}",
                    endpoint->name, ent->name);
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
    if (!port) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Port", getJsonValue(ent));
        rc = ISMRC_BadPropertyValue;
    }

    if (rc == 0) {
        where = savewhere;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            if (!strcmpix(ent->name, "Port")) {
                endpoint->port = ent->count;
            } else if (!strcmpix(ent->name, "Interface")) {
                replaceString(&endpoint->ipaddr, ent->value);
            } else if (!strcmpix(ent->name, "DomainSeparator")) {
                endpoint->separator = ent->objtype != JSON_Null ? ent->value[0] : 0;
            } else if (!strcmpix(ent->name, "ClientClass")) {
                replaceString(&endpoint->clientclass, ent->value);
            } else if (!strcmpix(ent->name, "Secure")) {
                endpoint->secure = ent->objtype == JSON_Null ? 2 : ent->objtype == JSON_True;
            } else if (!strcmpix(ent->name, "Enabled")) {
                endpoint->enabled = ent->objtype != JSON_False;
            } else if (!strcmpix(ent->name, "UseClientCipher")) {
                endpoint->clientcipher = ent->objtype == JSON_Null ? 2 : ent->objtype == JSON_True;
                if (endpoint->clientcipher == 1)
                    endpoint->sslop &= !SSL_OP_CIPHER_SERVER_PREFERENCE;
                else
                    endpoint->sslop |= SSL_OP_CIPHER_SERVER_PREFERENCE;
            } else if (!strcmpix(ent->name, "UseClientCertificate")) {
                endpoint->clientcert = ent->objtype == JSON_Null ? 2 : ent->objtype == JSON_True;
            } else if (!strcmpix(ent->name, "Ciphers")) {
                if (ent->objtype == JSON_Null) {
                    endpoint->ciphertype = 0;
                } else if (*ent->value == ':') {
                    endpoint->ciphertype = CipherType_Custom;
                    replaceString(&endpoint->ciphers, ent->value);
                } else {
                    endpoint->ciphertype = ism_common_enumValue(enum_ciphers, ent->value);
                }
            } else if (!strcmpix(ent->name, "Method")) {
                if (ent->objtype == JSON_Null)
                    endpoint->tls_method = 0;
                else
                    endpoint->tls_method = ism_common_enumValue(enum_methods, ent->value);
            } else if (!strcmpix(ent->name, "Transport")) {
                if (ent->objtype == JSON_Null)
                    endpoint->transmask = TMASK_AnyTrans;
                else
                    endpoint->transmask = parseTransports(ent->value);
            } else if (!strcmpix(ent->name, "Protocol")) {
                if (ent->objtype == JSON_Null) {
#ifdef HAS_BRIDGE
                    endpoint->protomask = PMASK_Admin;
#else
                    endpoint->protomask = PMASK_MQTT;
#endif
                } else {
                    endpoint->protomask = parseProtocols(ent->value);
                }
            } else if (!strcmpix(ent->name, "MaxMessageSize")) {
                endpoint->maxMsgSize = ent->objtype == JSON_Null ? 0 : ent->count;
            } else if (!strcmpix(ent->name, "Authentication")) {
                endpoint->authorder[0] = ent->objtype == JSON_Null? 0 : ism_common_enumValue(enum_auth, ent->value);
            } else if (!strcmpix(ent->name, "EnableAbout")) {
                endpoint->enableAbout = ent->objtype == JSON_Null ? 2 : ent->objtype == JSON_True;
            } else if (!strcmpix(ent->name, "Certificate")) {
                replaceString(&endpoint->cert, ent->value);
            } else if (!strcmpix(ent->name, "Key")) {
                replaceString(&endpoint->key, ent->value);
            } else if (!strcmpix(ent->name, "KeyPassword")) {
                if (ent->value && *ent->value != '!') {
                    int    obfuslen = strlen(ent->value)*2 + 48;
                    char * obfusbuf = alloca(obfuslen);
                    ism_transport_makepw(ent->value, obfusbuf, obfuslen, 0);
                    replaceString(&endpoint->keypw, obfusbuf);
                    g_need_dyn_write = 1;
                } else {
                    replaceString(&endpoint->keypw, ent->value);
                }
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
    }
    /*
     * For compatibility, if the separator='2' set the clientclass=iot2
     */
    if (endpoint->separator == '2' && endpoint->clientclass == NULL) {
        replaceString(&endpoint->clientclass, "iot2");
#ifndef NO_PROXY
        ism_proxy_getTopicRule("iot2");   /* Force load of default topic rules */
#endif
    }
    if (endpoint->ciphertype != CipherType_Custom) {
        makeCiphers(endpoint->ciphertype, endpoint->tls_method, xbuf, sizeof xbuf);
        replaceString(&endpoint->ciphers, xbuf);
    }

    /*
     * Set the values for the default CTX from the first secure endpoint
     */
    if (!rc && !ism_common_isBridge() && endpoint->secure==1 && !ism_common_getStringConfig("tlsCiphers")) {
        ism_field_t f;
        f.type = VT_String;
        f.val.s = xbuf;
        ism_common_setProperty(ism_common_getConfigProperties(), "tlsCiphers", &f);
        f.val.s = (char *)ism_common_enumName(enum_methods, endpoint->tls_method);
        ism_common_setProperty(ism_common_getConfigProperties(), "tlsMethod", &f);
        f.val.s = (char *)endpoint->cert;
        ism_common_setProperty(ism_common_getConfigProperties(), "tlsCertName", &f);
        f.val.s = (char *)endpoint->key;
        ism_common_setProperty(ism_common_getConfigProperties(), "tlsKeyName", &f);
        if (endpoint->keypw) {
            f.val.s = (char *)endpoint->keypw;
            ism_common_setProperty(ism_common_getConfigProperties(), "tlsKeyPW", &f);
        }
        f.type = VT_Integer;
        f.val.i = endpoint->sslop;
        ism_common_setProperty(ism_common_getConfigProperties(), "tlsOptions", &f);
    }

    if (rc) {
        endpoint->enabled = 0;
        if (needlog) {
            ism_common_formatLastError(xbuf, sizeof xbuf);
            LOG(ERROR, Server, 955, "%s%u%-s", "Endpoint configuration error: Endpoint={0} Error={2} Code={1}",
                                endpoint->name, ism_common_getLastError(), xbuf);
        }
    }

    if (!created || !rc) {
        if (created) {
            linkEndpoint(endpoint);
        }
        endpoint->rc = rc;
        endpoint->needed = ENDPOINT_NEED_ALL;
        if (g_messaging_started == 1) {
            rc = ism_transport_startTCPEndpoint(endpoint);
            if (!rc)
                endpoint->needed = 0;
        }
        ism_transport_dumpEndpoint(5, endpoint, "make", 0);
    }
    return endpoint->rc;
}


/*
 * Start the transport.
 * This is called near the beginning of the server startup and creates the necessary
 * threads and endpoint objects, but only starts the admin endpoint.
 */
int ism_transport_start(void) {
    int rc;

    TRACE(4, "Start transport\n");
    /* Init WebSockets frame.  We do this here to be after all other initialization */
    ism_transport_wsframe_init();

#ifndef NO_PROXY
    ism_tenant_initServerTLS();
#endif

    /* Start TCP */
    rc = ism_transport_startTCP();

    return rc;
}

/*
 * Create the forwarder client context here, as we know the cert
 */
XAPI struct ssl_ctx_st * ism_transport_clientTlsContext(const char * name, const char * tlsversion, const char * cipher) {
    return ism_common_create_ssl_ctx(name, tlsversion, cipher,
        NULL, NULL, 0, 0x010003FF, 0, name, NULL, NULL);
}

/*
 * Get count of endpoints waiting for LB authstats
 */
XAPI int ism_transport_lbcount(void) {
    ism_endpoint_t * endpoint;
    int waitcount = 0;
    ism_tenant_lock();
    endpoint = endpoints;
    while (endpoint) {
        if (endpoint->enabled && endpoint->lb_count)
            waitcount++;
        endpoint = endpoint->next;
    }
    ism_tenant_unlock();
    return waitcount;
}

extern int pxactEnabled;
/*
 * Start messaging.
 *
 * This is called after all recovery is done, and starts up the rest of the messaage
 * endpoints.
 */
int ism_transport_startMessaging(void) {
    ism_endpoint_t * endpoint;
    int   rc;

    if (g_messaging_started)
        return 1;

    TRACE(4, "Start messaging\n");
    g_messaging_started = 1;
#ifndef NO_PXACT
    if (pxactEnabled)
        ism_pxactivity_startMessaging();
#endif

    /*
     * Call all registered protocols to tell them messaging is starting
     */
    TRACE(6, "Inform registered protocols of start messaging\n");
    ism_transport_t transp;
    transp.protocol = "*start*";
    ism_transport_findProtocol(&transp);

    /*
     * Start endpoints
     */
    TRACE(6, "Start all endpoints\n");
    ism_tenant_lock();
    TRACE(6, "Start all endpoints 2\n");
    endpoint = endpoints;
    while (endpoint) {
        TRACE(7, "Start endpoint name=%s need=%d\n", endpoint->name, endpoint->needed);
        rc = ism_transport_startTCPEndpoint(endpoint);
        if (!rc)
            endpoint->needed = 0;
        endpoint = endpoint->next;
    }
    ism_tenant_unlock();
    return 0;
}




/*
 * Link a new endpoint profile
 * If there is already one by this name, move it to the old chain
 *
 * This is called with the tenantlock held
 */
static void linkEndpoint(ism_endpoint_t * endpoint) {
    ism_endpoint_t * lp = (ism_endpoint_t *)&endpoints;
    while (lp->next) {
        if (!strcmp(endpoint->name, lp->next->name)) {
            ism_endpoint_t * oldendp = lp->next;
            endpoint->next = lp->next->next;
            lp->next = endpoint;
            /* Link onto the old chain */
            oldendp->next = old_endpoints;
            old_endpoints = oldendp;
            return;
        }
        lp = lp->next;
    }
    endpoint->next = NULL;
    lp->next = endpoint;
    endpoint_count++;
}


/*
 * Unlink a certificate profile
 */
static int unlinkEndpoint(const char * name) {
    ism_endpoint_t * lp = (ism_endpoint_t *)&endpoints;
    while (lp->next) {
        if (!strcmp(name, lp->next->name)) {
            ism_endpoint_t * oldendp = lp->next;
            lp->next = lp->next->next;
            /* Link onto the old chain.  We never delete endpoints */
            oldendp->next = old_endpoints;
            old_endpoints = oldendp;
            endpoint_count--;
            return 0;
        }
        lp = lp->next;
    }
    ism_common_setErrorData(ISMRC_NotFound, "%s", name);
    return ISMRC_NotFound;
}

/*
 * Look up an endpoint by name
 */
ism_endpoint_t * ism_transport_getEndpoint(const char * name) {
    ism_endpoint_t * ret = endpoints;
    if (!name)
        return NULL;
    while (ret) {
        if (!strcmp(name, ret->name))
            break;
        ret = ret->next;
    }
    return ret;
}

/*
 * Put out a endpoint list in JSON form
 */
int ism_transport_getEndpointList(const char * match, ism_json_t * jobj, int json, const char * name) {
    ism_endpoint_t * endpoint;
     if (json)
        ism_json_startObject(jobj, name);
    else
        ism_json_startArray(jobj, name);

    ism_tenant_lock();
    endpoint = endpoints;
    while (endpoint) {
        if (ism_common_match(endpoint->name, match)) {
            if (json) {
                ism_tenant_getEndpointJson(endpoint, jobj, endpoint->name);
            } else {
                ism_json_putStringItem(jobj, NULL, endpoint->name);
            }
        }
        endpoint = endpoint->next;
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
int ism_tenant_getEndpointStats(const char * match, ism_json_t * jobj) {
    ism_endpoint_t * endpoint;
    uint64_t count;
    int i;

    ism_tenant_lock();
    ism_json_startObject(jobj, NULL);
    ism_json_startObject(jobj, "Endpoint");

    endpoint = endpoints;
    while (endpoint) {
        if (ism_common_match(endpoint->name, match)) {
            ism_json_startObject(jobj, endpoint->name);
            ism_json_putBooleanItem(jobj, "Enabled", endpoint->enabled);
            ism_json_putIntegerItem(jobj, "RC", endpoint->rc);
            ism_json_putIntegerItem(jobj, "ActiveConnections", endpoint->stats->connect_active);
            ism_json_putIntegerItem(jobj, "TotalConnections", endpoint->stats->connect_count);
            ism_json_putIntegerItem(jobj, "BadConnections", endpoint->stats->bad_connect_count);
            for (count = 0, i = 0; i < endpoint->thread_count; i++) {
                count += endpoint->stats->count[i].read_bytes;
            }
            ism_json_putLongItem(jobj, "ReadBytes", count);

            int read_msg=0, qos0_read_msg=0, qos1_read_msg=0, qos2_read_msg=0;
            for (count = 0, i = 0; i < endpoint->thread_count; i++) {
            	read_msg += endpoint->stats->count[i].read_msg;
            }
            ism_json_putLongItem(jobj, "ReadMsg", read_msg);

            for (count = 0, i = 0; i < endpoint->thread_count; i++) {
            	qos1_read_msg += endpoint->stats->count[i].qos1_read_msg;
			}

			for (count = 0, i = 0; i < endpoint->thread_count; i++) {
				qos2_read_msg += endpoint->stats->count[i].qos2_read_msg;
			}
			qos0_read_msg = read_msg - (qos1_read_msg + qos2_read_msg);

			if (endpoint->protomask & PMASK_MQTT) {
			    ism_json_putLongItem(jobj, "QoS0ReadMsg", qos0_read_msg);
			    ism_json_putLongItem(jobj, "QoS1ReadMsg", qos1_read_msg);
			    ism_json_putLongItem(jobj, "QoS2ReadMsg", qos2_read_msg);
			}

            for (count = 0, i = 0; i < endpoint->thread_count; i++) {
                count += endpoint->stats->count[i].write_bytes;
            }
            ism_json_putLongItem(jobj, "WriteBytes", count);
            for (count = 0, i = 0; i < endpoint->thread_count; i++) {
                count += endpoint->stats->count[i].write_msg;
            }
            ism_json_putLongItem(jobj, "WriteMsg", count);
            ism_json_endObject(jobj);
        }
        endpoint = endpoint->next;
    }
    ism_tenant_unlock();
    ism_json_endObject(jobj);
    ism_json_endObject(jobj);
    return 0;
}

/*
 * Stop all non-internal endpoints
 */
int ism_transport_stopMessaging(void) {
    ism_endpoint_t * endpoint;

    if (!g_messaging_started) {
    	g_messaging_started = 2;
    	return 0;
    }

    ism_tenant_lock();
    endpoint = endpoints;
    while (endpoint) {
        if (endpoint->enabled && ((endpoint->protomask & PMASK_Internal) == 0)) {
            endpoint->enabled = 0;
            endpoint->needed = ENDPOINT_NEED_SOCKET;
            ism_transport_startTCPEndpoint(endpoint);
        }
        endpoint = endpoint->next;
    }
    ism_tenant_unlock();
    return 0;
}

/*
 * Terminate the transport
 */
int ism_transport_term(void) {
    ism_endpoint_t * endpoint;

    /*
     * Disable all endpoints
     */
    ism_tenant_lock();
    endpoint = endpoints;
    while (endpoint) {
        if (endpoint->enabled) {
            endpoint->enabled = 0;
            endpoint->needed = ENDPOINT_NEED_SOCKET;
            ism_transport_startTCPEndpoint(endpoint);
        }
        endpoint = endpoint->next;
    }
    ism_tenant_unlock();
    ism_transport_closeAllConnections(1, 1);

    /*
     * Terminate TCP
     */
    usleep(10000);
    ism_transport_termTCP();
    usleep(10000);
    if (tObjPool)
        ism_common_destroyBufferPool(tObjPool);
#ifndef NO_SQS
    ism_proxy_sqs_term();
#endif
    return 0;
}


/*
 * Make a transport object.
 * Allocate a transport object.
 * @param endpoint  The endpoint object
 * @param tobjsize  Space allocated for transport specific object
 */
ism_transport_t * ism_transport_newTransport(ism_endpoint_t * endpoint, int tobjSize, int fromPool) {
    int size;
    ism_transport_t * transport;
    if (tobjSize < 0) {
        tobjSize = 0;
    }

    if (!endpoint)
        endpoint = &nullEndpoint;
    size = tobjSize + sizeof(ism_transport_t);
    int allocSize = TOBJ_INIT_SIZE;
    if (size >= TOBJ_INIT_SIZE) {
        allocSize = (size+1024);
        fromPool = 0;
    }

    ism_byteBuffer buff = (tObjPool && fromPool) ? ism_common_getBuffer(tObjPool,1) : ism_allocateByteBuffer(allocSize);
    transport = (ism_transport_t*) (buff->buf);
    memset(transport, 0, allocSize);
    transport->suballoc.size = allocSize - sizeof(ism_transport_t);
    transport->suballoc.pos  = 0;
    if (tobjSize)
        transport->tobj = (struct ism_transobj *)ism_transport_allocBytes(transport, tobjSize, 1);
    transport->state = ISM_TRANST_Opening;
    transport->domain = &ism_defaultDomain;
    transport->trclevel = ism_defaultTrace;
    transport->name = "";
    transport->clientID = "";
    transport->endpoint = endpoint;
    transport->endpoint_name = endpoint->name;
    transport->protocol = "unknown";                       /* The protocol is not yet known */
    transport->protocol_family = "";
    transport->connect_time = ism_common_currentTimeNanos();
    pthread_spin_init(&transport->lock,0);
    transport->lastAccessTime = ism_common_readTSC();
    return transport;
}


/*
 * Make a transport object.
 * Allocate a transport object.
 * @param endpoint  The endpoint object
 * @param tobjsize  Space allocated for transport specific object
 */
ism_transport_t * ism_transport_newOutgoing(ism_endpoint_t * endpoint, int fromPool) {
    int size;
    ism_transport_t * transport;

    if (!endpoint)
        endpoint = &outEndpoint;
    size = sizeof(ism_transport_t);
    int allocSize = TOBJ_INIT_SIZE;
    if (size >= TOBJ_INIT_SIZE) {
        allocSize = (size+1024);
        fromPool = 0;
    }
    ism_byteBuffer buff = (tObjPool && fromPool) ? ism_common_getBuffer(tObjPool,1) : ism_allocateByteBuffer(allocSize);
    transport = (ism_transport_t*) (buff->buf);
    memset(transport, 0, allocSize);
    transport->suballoc.size = allocSize - sizeof(ism_transport_t);
    transport->suballoc.pos  = 0;
    transport->state = ISM_TRANST_Opening;
    transport->domain = &ism_defaultDomain;
    transport->trclevel = ism_defaultTrace;
    transport->name = "";
    transport->clientID = "";
    transport->endpoint = endpoint;
    transport->endpoint_name = endpoint->name;
    transport->protocol = "unknown";                       /* The protocol is not yet known */
    transport->protocol_family = "";
    transport->originated = 1;
    transport->connect_time = ism_common_currentTimeNanos();
    pthread_spin_init(&transport->lock,0);
    transport->lastAccessTime = ism_common_readTSC();
    return transport;
}

/*
 * Free a transport object
 * Free all memory associated with the transport object.  The invoker must guarantee that there
 * are no references to the transport object before calling this function.
 */
void ism_transport_freeTransport(ism_transport_t * transport) {
    ism_byteBuffer buff = (ism_byteBuffer)transport;
    struct suballoc_t * suba = transport->suballoc.next;
    while (suba) {
        struct suballoc_t * freesub = suba;
        suba = suba->next;
        freesub->next = NULL;
        ism_common_free(ism_memory_proxy_transport,freesub);
    }
    buff--;

    ism_common_returnBuffer(buff, __FILE__, __LINE__);
}

/*
 * Check for name validity
 */
int ism_transport_validName(const char * name) {
    int count;
    int len;
    int i;

    /* Check for NULL */
    if (!name)
        return 0;
    /* Check valid UTF8 */
    len = strlen(name);
    count = ism_common_validUTF8(name, len);
    if (count<1)
        return 0;
    /* Check for starting char */
    if ((uint8_t)*name < 0x40 && *name != '!')
        return 0;
    /* Check for control characters */
    for (i=0; i<len; i++) {
        if ((uint8_t)name[i]<' ' || name[i]=='=')
            return 0;
    }
    /* Do not allow trailing space */
    if (name[len-1] == ' ')
        return 0;
    return 1;
}

#define MY_AES128 "ECDHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-SHA:DHE-RSA-AES128-SHA:"

#define MY_AES256 "ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-SHA:DHE-RSA-AES256-SHA:"

/*
 * Make the cipher string from the method
 */
static const char * makeCiphers(int ciphertype, int method, char * buf, int len) {
    char * ciphers;

    /*
     * Default ciphers if not specified
     *
     * NIST 800-131a prohibit the use of a number of some ciphers including the use of MD5 as an HMAC.
     * If TLSv1.2 is used as the protocol and best is selected, do not use ciphers with the SHA1 HMAC.
     */
    if (FIPSmode) {
        switch (ciphertype) {
        case CipherType_Best:    /* Best security */
            ciphers = MY_AES256 MY_AES128 "AESGCM:FIPS:!SRP:!PSK:!ADH:!AECDH:!EXP:!RC4";
            break;
        default:
        case CipherType_Fast:    /* Optimize for speed with high security */
            ciphers = MY_AES128 MY_AES256 "AES128-GCM-SHA256:AESGCM:AES128:FIPS:!SRP:!PSK:!ADH:!AECDH:!EXP:!RC4";
            break;
        case CipherType_Medium:  /* Optimize for speed and allow medium security */
            ciphers = "AES128-GCM-SHA256:AES128:FIPS:!SRP:!PSK:!EXP:!RC4";
            break;
        }
    } else {
        switch (ciphertype) {
        case CipherType_Best:    /* Best security */
            ciphers = MY_AES256 MY_AES128 "AESGCM:AES:!SRP:!ADH:!AECDH:!EXP:!RC4";
            break;
        default:
        case CipherType_Fast:    /* Optimize for speed with high security */
            ciphers = MY_AES128 MY_AES256 "AES128-GCM-SHA256:AESGCM:AES:!SRP:!ADH:!AECDH:!EXP:!RC4";
            break;
        case CipherType_Medium:  /* Optimize for speed and allow medium security */
            ciphers = "AES128-GCM-SHA256:AES128:HIGH:MEDIUM:!eNULL:!SRP:!EXP:!RC4";
        }
    }
    ism_common_strlcpy(buf, ciphers, len);
    return buf;
}


/*
 * Create a endpoint object
 */
ism_endpoint_t * ism_transport_createEndpoint(const char * name, int mkstats) {
    ism_endpoint_t * ret;
    int extralen;
    char * cp;

    /* Check the name */
    if (!ism_transport_validName(name)) {
        ism_common_setErrorData(ISMRC_BadConfigName, "%s", name);
        return NULL;
    }

    extralen = strlen(name)+1;

    /* Create the object */
    ret = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_transport_endpoint,1),1, sizeof(ism_endpoint_t) + extralen);
    cp = (char *)(ret+1);
    if (mkstats) {
        ret->stats = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_transport_endpoint,2),1, sizeof(ism_endstat_t));
    }
    ret->name = (const char *)cp;
    strcpy(cp, name);

    strcpy(ret->transport_type, "tcp");
    ret->sslop = 0x034203FF;
    TRACE(5, "Create endpoint: name =%s tlsopt=%08x\n", name, ret->sslop);
    return ret;
}


/*
 * Register a transport
 */
int ism_transport_registerProtocol(ism_transport_register_t regcall) {
    /* Put the new one at the start of the chain */
    struct protocol_chain * newprot = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_transport,29),sizeof(struct protocol_chain));
    newprot->next = protocols;
    newprot->regcall = regcall;
    protocols = newprot;
    return 0;
}


/*
 * Call registered protocols until we get one which works
 */
int ism_transport_findProtocol(ism_transport_t * transport) {
    struct protocol_chain * prot = protocols;
    while (prot) {
        int rc = prot->regcall(transport);
        if (rc == 0) {
            transport->state = ISM_TRANST_Open;
            return 0;
        }
        prot = prot->next;
    }
    return 1;
}




/*
 * Allocate bytes
 */
char * ism_transport_allocBytes(ism_transport_t * transport, int len, int align) {
    struct suballoc_t * suba = &transport->suballoc;
    int  pad = 0;
    for (;;) {
        if (align) {
            pad = 8-(suba->pos&3);
            if (pad == 8)
                pad = 0;
        }
        if (suba->size-suba->pos > len+pad) {
            char * ret = ((char *)(suba+1))+suba->pos+pad;
            suba->pos += len+pad;
            return ret;
        }
        if (!suba->next) {
            int newlen = ((len+1200) & ~0x3ff);
            suba->next = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_transport,30),newlen);
            suba->next->next = NULL;
            suba->next->size = newlen-sizeof(struct suballoc_t);
            suba->next->pos  = 0;
        }
        suba = suba->next;
    }
    return NULL;
}


/*
 * Put a string into the transport object.
 *
 * This allocates space inside the transport object which is freed when the tranaport object
 * is freed.  The various strings in the transport object can either be allocated in this way,
 * or can be constants.
 * @param transport  The transport object
 * @param str   The string to copy
 * @return The copy of the string within the transport object.
 */
const char * ism_transport_putString(ism_transport_t * transport, const char * str) {
    int    len = (int)strlen(str);
    char * bp = ism_transport_allocBytes(transport, len+1, 0);
    strcpy(bp, str);
    return (const char *)bp;
}



/*
 * Dump fields from a transport object
 */
void ism_transport_dumpTransport(int level, ism_transport_t * transport, const char * where, int full) {
    if (!where)
        where = "object";
    TRACEL(level, transport->trclevel,
          "Transport %s index=%u name=%s addr=%p\n"
          "    client_addr=%s client_port=%u server_addr=%s server_port=%u\n"
          "    protocol=%s userid=%s clientID=%s cert_name=%s\n"
          "    readbytes=%llu readmsg=%llu writebytes=%llu writemsg=%llu sendQueueSize=%d\n",
          where, transport->index, transport->name, transport,
          transport->client_addr, transport->clientport, transport->server_addr, transport->serverport,
          transport->protocol, transport->userid ? transport->userid : "", transport->clientID,
          transport->cert_name?transport->cert_name:"",
          (ULL)transport->read_bytes, (ULL)transport->read_msg,
          (ULL)transport->write_bytes, (ULL)transport->write_msg, transport->sendQueueSize);
}


/*
 * Dump a endpoint object
 */
void ism_transport_dumpEndpoint(int level, ism_endpoint_t * endpoint, const char * where, int full) {
    int   i;
    uint64_t read_msg = 0;
    uint64_t read_bytes = 0;
    uint64_t write_msg = 0;
    uint64_t write_bytes = 0;
    uint64_t lost_msg = 0;
    char rmsgcnt[32];
    char rbytecnt[32];
    char wmsgcnt[32];
    char wbytecnt[32];
    if (!where)
        where = "object";

    /* Ignore thread problems for now */
    for (i=0; i<endpoint->thread_count; i++) {
        read_msg += endpoint->stats->count[i].read_msg;
        read_bytes += endpoint->stats->count[i].read_bytes;
        write_msg += endpoint->stats->count[i].write_msg;
        write_bytes += endpoint->stats->count[i].write_bytes;
        lost_msg += endpoint->stats->count[i].lost_msg;
    }
    ism_common_ltoa_ts(read_msg, rmsgcnt, ism_common_getNumericSeparator());
    ism_common_ltoa_ts(read_bytes, rbytecnt, ism_common_getNumericSeparator());
    ism_common_ltoa_ts(write_msg, wmsgcnt, ism_common_getNumericSeparator());
    ism_common_ltoa_ts(write_bytes, wbytecnt, ism_common_getNumericSeparator());

    TRACE(level,
          "Endpoint %s name=%s enabled=%u rc=%d ipaddr=%s port=%u addr=%p need=%d\n"
          "    secure=%u ciphers=%s method=%s clientcert=%u clientciphers=%u clientclass=%s\n"
          "    protomask=%x transmask=%x sock=%p maxsize=%u active=%llu count=%llu failed=%llu\n"
          "    read_msg=%s read_bytes=%s write_msg=%s write_msg=%s lost_msg=%llu\n",
            where, endpoint->name, endpoint->enabled, endpoint->rc, endpoint->ipaddr,
            endpoint->port, endpoint, endpoint->needed,
            endpoint->secure, ism_common_enumName(enum_ciphers_out, endpoint->ciphertype),
            ism_common_enumName(enum_methods, endpoint->tls_method), endpoint->clientcert, endpoint->clientcipher,
            endpoint->clientclass ? endpoint->clientclass : "", endpoint->protomask, endpoint->transmask,
            (void *)(uintptr_t)endpoint->sock, endpoint->maxMsgSize,
            (ULL)endpoint->stats->connect_active, (ULL)endpoint->stats->connect_count,
            (ULL)endpoint->stats->bad_connect_count, rmsgcnt, rbytecnt, wmsgcnt, wbytecnt, (ULL)lost_msg);
}

void ism_transport_printEndpoints(const char * pattern){
    int   i;
    uint64_t read_msg = 0;
    uint64_t read_bytes = 0;
    uint64_t write_msg = 0;
    uint64_t write_bytes = 0;
    uint64_t lost_msg = 0;
    char rmsgcnt[32];
    char rbytecnt[32];
    char wmsgcnt[32];
    char wbytecnt[32];
    ism_endpoint_t * endpoint;

    if (!pattern)
        pattern = "*";
    endpoint = endpoints;
    while (endpoint) {
        if (ism_common_match(endpoint->name, pattern)) {
            read_msg = 0;
            read_bytes = 0;
            write_msg = 0;
            write_bytes = 0;
            lost_msg = 0;
            /* Ignore thread problems for now */
            for (i=0; i<endpoint->thread_count; i++) {
                read_msg += endpoint->stats->count[i].read_msg;
                read_bytes += endpoint->stats->count[i].read_bytes;
                write_msg += endpoint->stats->count[i].write_msg;
                write_bytes += endpoint->stats->count[i].write_bytes;
                lost_msg += endpoint->stats->count[i].lost_msg;
            }
            ism_common_ltoa_ts(read_msg, rmsgcnt, ism_common_getNumericSeparator());
            ism_common_ltoa_ts(read_bytes, rbytecnt, ism_common_getNumericSeparator());
            ism_common_ltoa_ts(write_msg, wmsgcnt, ism_common_getNumericSeparator());
            ism_common_ltoa_ts(write_bytes, wbytecnt, ism_common_getNumericSeparator());

            printf("Endpoint %s enabled=%u rc=%d ipaddr=%s port=%u addr=%p need=%d\n"
                   "    secure=%u ciphers=%s method=%s clientcert=%u clientciphers=%u\n"
                   "    protomask=%x transmask=%x sock=%p maxsize=%u clientclass=%s\n"
                   "    active=%llu count=%llu failed=%llu\n"
                   "    read_msg=%s read_bytes=%s write_msg=%s write_bytes=%s lost_msg=%llu\n",
                    endpoint->name, endpoint->enabled, endpoint->rc, endpoint->ipaddr ? endpoint->ipaddr : "(null)",
                    endpoint->port, endpoint, endpoint->needed,
                    endpoint->secure, ism_common_enumName(enum_ciphers_out, endpoint->ciphertype),
                    ism_common_enumName(enum_methods, endpoint->tls_method), endpoint->clientcert, endpoint->clientcipher,
                    endpoint->protomask, endpoint->transmask,
                    (void *)(uintptr_t)endpoint->sock, endpoint->maxMsgSize,
                    endpoint->clientclass ? endpoint->clientclass : "",
                    (ULL)endpoint->stats->connect_active, (ULL)endpoint->stats->connect_count,
                    (ULL)endpoint->stats->bad_connect_count, rmsgcnt, rbytecnt, wmsgcnt, wbytecnt, (ULL)lost_msg);
        }
        endpoint = endpoint->next;
    }
}


/*
 * Get a size with K and M suffix
 */
xUNUSED static int getUnitSize(const char * ssize) {
    int val;
    char * eos;

    if (!ssize)
        return 0;
    val = strtoul(ssize, &eos, 10);
    if (eos) {
        while (*eos == ' ')
            eos++;
        if (*eos == 'k' || *eos == 'K')
            val *= 1024;
        else if (*eos == 'm' || *eos == 'M')
            val *= (1024 * 1024);
        else if (*eos)
            val = 0;
    }
    return val;
}

ism_transport_t * ism_transport_getPhysicalTransport(ism_transport_t * transport) {
    if(transport->virtualSid)
        transport = (ism_transport_t *) transport->tobj;
    return transport;
}

/**
 * Set HTTP or MQTT Message Count with Size
 */
int ism_proxy_setMsgSizeStats(px_msgsize_stats_t * msgSizeStat, int size, int originated) {
    int result = 0;
    if (msgSizeStat) {
        if (!originated) {
            if (size <= 512) {
                result = __sync_add_and_fetch(&msgSizeStat->C2P512BMsgReceived, 1);
            } else if (size <= 1024) {
                result = __sync_add_and_fetch(&msgSizeStat->C2P1KBMsgReceived, 1);
            } else if (size <= 4096) {
                result = __sync_add_and_fetch(&msgSizeStat->C2P4KBMsgReceived, 1);
            } else if (size <= 16384) {
                result = __sync_add_and_fetch(&msgSizeStat->C2P16KBMsgReceived, 1);
            } else if (size <= 65536) {
                result = __sync_add_and_fetch(&msgSizeStat->C2P64KBMsgReceived, 1);
            } else if (size > 65536) {
                result = __sync_add_and_fetch(&msgSizeStat->C2PLargeMsgReceived, 1);
            }
        } else {
            if (size <= 512) {
                result = __sync_add_and_fetch(&msgSizeStat->P2S512BMsgReceived, 1);
            } else if (size <= 1024) {
                result = __sync_add_and_fetch(&msgSizeStat->P2S1KBMsgReceived, 1);
            } else if (size <= 4096) {
                result = __sync_add_and_fetch(&msgSizeStat->P2S4KBMsgReceived, 1);
            } else if (size <= 16384) {
                result = __sync_add_and_fetch(&msgSizeStat->P2S16KBMsgReceived, 1);
            } else if (size <= 65536) {
                result = __sync_add_and_fetch(&msgSizeStat->P2S64KBMsgReceived, 1);
            } else if (size > 65536) {
                result = __sync_add_and_fetch(&msgSizeStat->P2SLargeMsgReceived, 1);
            }
        }
    }
    return result;
}

typedef struct tr_ackwait_t {
    uint64_t  waitID;
    ism_transport_t * transport;
} tr_ackwait_t;

static tr_ackwait_t * ackwait_list;
static pthread_mutex_t ackwait_lock;
static int  ackwait_alloc = 0;
static int  ackwait_avail = 1;

/*
 * Init the ackwait methods
 *
 * The purpose of the ackwait table is to give a point to respond with an ACK when the
 * transport object may have been closed while waiting.  The waitID consists of an index
 * into the ackwait table and a random value to guarantee that this slot is still current.
 */
void ism_transport_ackwaitInit(void) {
    pthread_mutex_init(&ackwait_lock, 0);
}

/*
 * Generate a waitID
 */
static uint64_t genWaitID(int i) {
    uint64_t nanos = ism_common_currentTimeNanos();
    uint64_t ret = (uint64_t)i;
    ret <<= 32;
    ret |= ism_common_crc32c(0, (char *)&nanos, 8);
    return ret;
}

/*
 * Get the WaitID for a transport object.
 * If the transport object already has a WaitID then return it, otheriwse assign one.
 * @param  The transport object
 * @return the waitID object
 */
uint64_t ism_transport_getWaitID(ism_transport_t * transport) {
    uint32_t i;
    uint64_t ret = 0;

    if (transport->waitID)           /* Check before locking if we already have a waitID */
        return transport->waitID;

    pthread_mutex_lock(&ackwait_lock);
    if (!transport->waitID) {        /* Check inside the lock as we might now have a waitID */
        /* Use an empty spot if we find one */
        for (i=ackwait_avail; i<ackwait_alloc; i++) {
            if (ackwait_list[i].waitID == 0) {
                ackwait_list[i].waitID = ret = genWaitID(i);
                ackwait_list[i].transport = transport;
                ackwait_avail = i+1;
                break;
            }
        }
        /* Extend the list if necessary */
        if (i == ackwait_alloc) {
            int newcount =  ackwait_alloc ? ackwait_alloc * 16 : 1024;
            tr_ackwait_t * newlist = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_transport,31),ackwait_list, newcount*sizeof(tr_ackwait_t));
            if (newlist) {
                memset(newlist+ackwait_alloc, 0, (newcount-ackwait_alloc)*sizeof(tr_ackwait_t));
                ackwait_alloc = newcount;
                ackwait_list = newlist;
                ackwait_list[i].waitID = ret = genWaitID(i);
                ackwait_list[i].transport = transport;
                ackwait_avail = i+1;
            }
        }
        transport->waitID = ret;
    }
    pthread_mutex_unlock(&ackwait_lock);
    return transport->waitID;
}

/*
 * Free the waitID associated with a transport object
 * @param The transport object
 */
void ism_transport_freeWaitID(ism_transport_t * transport) {
    uint32_t i;
    if (transport->waitID) {
        pthread_mutex_lock(&ackwait_lock);
        i = (int)(transport->waitID>>32);
        if (i && i < ackwait_alloc) {
            ackwait_list[i].waitID = 0;
            ackwait_list[i].transport = NULL;
        }
        pthread_mutex_unlock(&ackwait_lock);
    }
}


/*
 * Acknowledge something being waited on
 * @param waitid  The wait ID assigned to the transport object
 * @param waitval The wait value
 * @param rc      The reason code for the failure (or 0 for good)
 * @param reason  A reason string or NULL to indicate no reason
 */
void ism_transport_ack(uint64_t waitid, int waitval, int rc, const char * reason) {
    uint32_t i;
    ism_transport_t * transport = NULL;
    if (waitid) {
        pthread_mutex_lock(&ackwait_lock);
        i = (int)(transport->waitID>>32);
        if (i && i < ackwait_alloc) {
            if (ackwait_list[i].waitID == waitid) {
                transport = ackwait_list[i].transport;
            }
        }
    }
    pthread_mutex_unlock(&ackwait_lock);
    if (transport) {
        transport->ack(transport, waitval, rc, reason);
    }
}
