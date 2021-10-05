
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
#include <admin.h>
#include <ismregex.h>
#include <transport.h>
#include <config.h>
#include "tcp.h"
#include <unistd.h>
#include <cluster.h>
#include <transport_certificates.h>
#include <openssl/opensslv.h>

#ifndef _WIN32
#include <pthread.h>
#endif

extern int ism_transport_startAdminIOP(void);
extern int ism_transport_getTcpMax(void);
XAPI void ism_server_initt(ism_prop_t * props);
extern int ism_config_set_fromJSONStr(const char *jsonStr, const char *object, int validate);
extern void ism_common_startMessaging(void);
extern void ism_engine_threadInit(uint8_t isStoreCrit);
extern void ism_transport_closeAllConnections(int bypassAdminEPCheck);

static ism_transport_t * * monitorlist;
static int                 monitor_alloc;
static int                 monitor_used;
static int                 monitor_free_head;
static int                 monitor_free_count;
static int                 monitor_free_limit;
static int                 monitor_free_tail;
static pthread_mutex_t     monitorlock;
static pthread_mutex_t     endpointlock;
static ism_endpoint_t *    adminEndpoint;
static int                 FIPSmode = 0;
static int                 g_messaging_started = 0;
static ism_config_t *      transporthandle = NULL;
extern int                 ism_transport_forwarder_active;

#define IS_VALID_MON(x) ((monitorlist[x]) && !(((uintptr_t)(monitorlist[x]))&1))


/*
 * SSL methods enumeration
 */
static ism_enumList enum_methods [] = {
#if OPENSSL_VERSION_NUMBER < 0x10101000L
    { "Method",    4,                 },
#else
    { "Method",    5,                 },
#endif
    { "SSLv3",     SecMethod_SSLv3,   },
    { "TLSv1",     SecMethod_TLSv1,   },
    { "TLSv1.1",   SecMethod_TLSv1_1, },
    { "TLSv1.2",   SecMethod_TLSv1_2, },
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
    { "TLSv1.3",   SecMethod_TLSv1_3, },
#endif
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

/*
 * Dummpy endpoint so we do not segfault
 */
static ism_endpoint_t nullEndpoint = {
    .name        = "!None",
    .ipaddr      = "",
    .secprof     = "",           /* Security profile name                  */
    .msghub      = "!None",      /* Message hub name                       */
    .conpolicies = "",           /* Connection policies                    */
    .topicpolicies = "",         /* Topic policies                         */
    .qpolicies   = "",           /* Queue policies                         */
    .subpolicies = "",           /* Subscripton policies                   */
    .transport_type = "none",
    .protomask   = PMASK_Internal,
    .thread_count = 1,
};

/*
 * Define a list of protocol handlers.
 * When we need to find a protocol, we call each one until we get one which accepts
 * the protocol string in the transport.
 */
typedef struct protocol_chain {
    struct protocol_chain *             next;
    ism_transport_onConnection_t        onConnection;
    ism_transport_onStartMessaging_t    onStart;
} protocol_chain;

static protocol_chain * protocols = NULL;

/*
 * Define a list of frame handlers.
 * When we need to find a frame, we call each one until we get one which accepts
 * the handshake.
 */
struct framer_chain {
    struct framer_chain *     next;
    ism_transport_registerf_t regcall;
};

static struct framer_chain * frames = NULL;

/*
 * Chain of endpoints
 */
static ism_endpoint_t * endpoints = NULL;
static int endpoint_count = 0;

/*
 * Old endpoints which are no longer used.  We do not delete these as objects may have pointers to them
 */
static ism_endpoint_t * old_endpoints = NULL;


/*
 * Chain of security profiles
 */
static ism_secprof_t * secprofiles = NULL;
static int secprofile_count = 0;

/*
 * Chain of cetificate profiles
 */
static ism_certprof_t * certprofiles = NULL;
static int certprofile_count = 0;

/*
 * Chain of msghub
 */
static ism_msghub_t * msghubs = NULL;
static ism_msghub_t * old_msghubs = NULL;
static int msghub_count = 0;

#define TOBJ_INIT_SIZE  1536
static ism_byteBufferPool tObjPool = NULL;

/*
 * Forward references
 */
static int makeSecurityProfile(const char * name, ism_prop_t * props);
static int makeEndpoint(const char * name, ism_prop_t * props, ism_endpoint_t * * created_endpoint);
static int makeCertProfile(const char * name, ism_prop_t * props);
static int makeMsgHub(const char * name, ism_prop_t * props);
static void linkSecProfile(ism_secprof_t * secprof);
static void linkEndpoint(ism_endpoint_t * endpoint);
static void linkCertProfile(ism_certprof_t * cert);
static void linkMsgHub(ism_msghub_t * msghub);
static int unlinkEndpoint(const char * name);
static int unlinkSecProfile(const char * name);
static int unlinkCertProfile(const char * name);
static int unlinkMsgHub(const char * name);
static ism_endpoint_t * getEndpoint(const char * name);
ism_secprof_t * ism_transport_getSecProfile(const char * name);
ism_msghub_t * ism_transport_getMsgHub(const char * name);
int ism_transport_config(char * object, char * name, ism_prop_t * props, ism_ConfigChangeType_t flag);
extern char ism_common_getNumericSeparator(void);
extern const char * ism_common_getStringConfig(const char * name);
extern void ism_ssl_init(int useFips, int useBufferPool);
static int getUnitSize(const char * ssize);
static void setProp(ism_prop_t * props, const char * obj, const char * name, const char * item, const char * value);

/*
 * Init for the WebSockets framer
 */
void ism_transport_wsframe_init(void);
void ism_transport_frame_init(void);

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
static int mystrcmp(const char * s1, const char * s2) {
    if (s1 == NULL && s2 == NULL)
        return 0;
    if (s1 == NULL)
        return -1;
    if (s2 == NULL)
        return 1;
    return strcmp(s1, s2);
}

/* Declare these here to overcome an order problem of admin and utils */
XAPI int ism_server_config(char * object, char * name, ism_prop_t * props, ism_ConfigChangeType_t flag);
XAPI void ism_server_initt(ism_prop_t * props);
static void pskNotify(int count);
XAPI void ism_common_setDynamicConfig(void *);
XAPI int ism_common_initServerName(void);

/*
 * Second round of server init.
 * This includes a couple of calls which cannot be done from utils.
 */
int ism_server_init2(void) {
    ism_prop_t * props;
    ism_config_t * conhandle = ism_config_getHandle(ISM_CONFIG_COMP_SERVER, NULL);

    /* Get the server dynamic config and run it */
    props = ism_config_getProperties(conhandle, NULL, NULL);
    ism_server_initt(props);
    ism_server_config(NULL, NULL, props, 0);
    if (props)
        ism_common_freeProperties(props);
    ism_common_setDynamicConfig((void *)ism_config_set_fromJSONStr);
    ism_common_initServerName();
    return 0;
}

/*
 * Initialize the transport component
 */
int ism_transport_init(void) {
    ism_endpoint_t * endpoint = NULL;
    const char * cfgname;
    int sslUseBufferPool;
    int minPool;
    ism_prop_t * transprops;

    /* Init monitoring */
    pthread_mutex_init(&monitorlock, NULL);
    pthread_mutex_init(&endpointlock, NULL);
    minPool = ism_common_getIntConfig("TransportMinPool", 10240);
    tObjPool = ism_common_createBufferPool(TOBJ_INIT_SIZE, minPool, 1024*1024,"TransportObjectPool");

    /*
     * Initial set of FIPS mode
     */
    FIPSmode = ism_config_getFIPSConfig();
    sslUseBufferPool = ism_config_getSslUseBufferPool();
    ism_common_setPSKnotify(pskNotify);
    ism_ssl_init(FIPSmode, sslUseBufferPool);

    /*
     * Initialize TCP
     */
    ism_transport_initTCP();

    /*
     * Initialize connection monitoring.
     * The monitor ID is assigned on a fifo of free slots, but with at least
     * monitor_Free_limit entries in the fifo before an ID is reused.
     */
    monitor_alloc = ism_transport_getTcpMax();
    monitor_free_limit = ism_common_getIntConfig("TCPMonitorLimit", 0);
    if (monitor_free_limit <= 0)
        monitor_free_limit = monitor_alloc/20;
    monitor_alloc += monitor_free_limit;
    monitorlist = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 18),monitor_alloc, sizeof(ism_transport_t *));
    monitor_used  = 1;       /* Skip zero */
    monitor_free_head = 0;
    monitor_free_tail = 0;
    monitor_free_count = 0;


    /* Register to get all transport callbacks */
    ism_config_register(ISM_CONFIG_COMP_TRANSPORT, NULL, ism_transport_config, &transporthandle);
    transprops = ism_config_getProperties(transporthandle, NULL, NULL);

    /*
     * Create the Admin endpoint
     */
    cfgname = "AdminEndpoint.Interface.AdminEndpoint";
    const char *adminHost = ism_common_getStringProperty(transprops, cfgname);
    if ( !adminHost || *adminHost == '\0')
        adminHost = "All";
    cfgname = "AdminEndpoint.Port.AdminEndpoint";
    int adminPort = ism_common_getIntProperty(transprops, cfgname, 9089);
    cfgname = "AdminEndpoint.SecurityProfile.AdminEndpoint";
    const char * secProfile = ism_common_getStringProperty(transprops, cfgname);
    if ( secProfile && ( *secProfile == ' ' || *secProfile == '\0' ) )
        secProfile = NULL;
    /* For AdminEndpoint, there are no connection policies, conpolicies field in ism_endpoint_t is used for configuration policies */
    cfgname = "AdminEndpoint.ConfigurationPolicies.AdminEndpoint";
    const char * confPols = ism_common_getStringProperty(transprops, cfgname);
    if ( confPols && ( *confPols == ' ' || *confPols == '\0' ))
        confPols = NULL;
    pthread_mutex_lock(&endpointlock);
    endpoint = ism_transport_createEndpoint("AdminEndpoint", NULL, "tcp", adminHost, secProfile, confPols, NULL, NULL, NULL, 1);
    endpoint->port = adminPort;
    endpoint->enabled = 1;
    endpoint->isAdmin = 1;
    endpoint->isInternal = 1;
    endpoint->enableAbout = 1;
    endpoint->protomask = PMASK_Admin;
    endpoint->transmask = TMASK_AnyTrans;
    endpoint->needed = ENDPOINT_NEED_ALL;
    endpoint->config_time = ism_common_currentTimeNanos();
    TRACE(5, "Create Admin Endpoint: Interface:%s Port:%d SecurityProfile:%s\n", adminHost, adminPort, secProfile?secProfile:"NULL");
    linkEndpoint(endpoint);

    pthread_mutex_unlock(&endpointlock);
    if (transprops)
        ism_common_freeProperties(transprops);
    return 0;
}


static int g_pskCount = 0;

int ism_transport_getPSKCount(void) {
    return g_pskCount;
}


/*
 * PSK change notification.
 * For all endpoints with an SSL context, update the PSK callback
 */
static void pskNotify(int count) {
    int  i;
    ism_endpoint_t * endpoint;
    pthread_mutex_lock(&endpointlock);
    g_pskCount = count;
    endpoint = endpoints;
    while (endpoint) {
        if (endpoint->sslCTX) {
            if (count > 0)
                endpoint->sslctx_count = 1;
            TRACE(8, "Update PSK for endpoint \"%s\": count=%u\n", endpoint->name, count);
            for (i=0; i<endpoint->sslctx_count; i++) {
                ism_ssl_update_psk(endpoint->sslCTX[i], count);
            }
        }
        endpoint = endpoint->next;
    }
    pthread_mutex_unlock(&endpointlock);
}


/*
 * Configuration callback for transport
 */
int ism_transport_config(char * object, char * name, ism_prop_t * props, ism_ConfigChangeType_t flag) {
    int   rc = 0;
    int   xrc;
    ism_endpoint_t * endpoint;

    if (name == NULL) {
        if (!strcmp(object, "MQConnectivityEnabled")) {
            name = "!MQConnectivityEndpoint";
        } else {
            /* Take no action on any other singleton properties */
            return 0;
        }
    }
    pthread_mutex_lock(&endpointlock);
    switch (flag) {
    case ISM_CONFIG_CHANGE_NAME:
        ism_common_setError(ISMRC_Error);
        rc = ISMRC_Error;
        break;
    /*
     * Process modification of properties
     */
    case ISM_CONFIG_CHANGE_PROPS:
        if (!strcmp(object, "Endpoint")) {
            rc = makeEndpoint(name, props, NULL);
            if (!rc) {
                endpoint = getEndpoint(name);
                if (endpoint) {
                    if (g_messaging_started) {
                        xrc = ism_transport_startTCPEndpoint(endpoint);
                        if (!xrc)
                            endpoint->needed = 0;
                        else
                            if (!endpoint->oldendp)
                                unlinkEndpoint(name);
                    }
                }
            }
        }
        else if (!strcmp(object, "SecurityProfile")) {
            rc = makeSecurityProfile(name, props);
            /* Restart all endpoints which use this security profile */
            if (!rc) {
                endpoint = endpoints;
                while (endpoint) {
                    if (endpoint->secprof && !strcmp(name, endpoint->secprof)) {
                        if (endpoint->enabled || endpoint->rc) {
                            endpoint->enabled = 1;
                            endpoint->rc = 0;
                            endpoint->needed |= ENDPOINT_NEED_SECURE;
                            if (!strcmp(endpoint->name, "AdminEndpoint") || g_messaging_started) {
                                xrc = ism_transport_startTCPEndpoint(endpoint);
                                if (xrc) {
                                    endpoint->rc = xrc;
                                    endpoint->enabled = 0;
                                } else {
                                    endpoint->needed = 0;
                                }
                            }
                        }
                    }
                    endpoint = endpoint->next;
                }
            }
        }
        else if (!strcmp(object, "MessageHub")) {
            rc = makeMsgHub(name, props);
        }

        else if (!strcmp(object, "CertificateProfile")) {
            rc = makeCertProfile(name, props);
            /* Restart all endpoints with a security profile which uses this certificate profile */
            if (!rc) {
                endpoint = endpoints;
                while (endpoint) {
                    if (endpoint->secprof) {
                        ism_secprof_t * secprof = ism_transport_getSecProfile(endpoint->secprof);
                        if (secprof && secprof->certprof && !strcmp(name, secprof->certprof)) {
                            if (endpoint->enabled || endpoint->rc) {
                                endpoint->enabled = 1;
                                endpoint->rc = 0;
                                endpoint->needed |= ENDPOINT_NEED_SECURE;
                                if (g_messaging_started) {
                                    xrc = ism_transport_startTCPEndpoint(endpoint);
                                    if (xrc) {
                                        endpoint->rc = xrc;
                                        endpoint->enabled = 0;
                                    } else {
                                        endpoint->needed = 0;
                                    }
                                }
                            }
                        }
                    }
                    endpoint = endpoint->next;
                }
            }
        }

        /*
         * Process the admin endpoint
         */
        else if (!strcmp(object, "AdminEndpoint")) {
            adminEndpoint = NULL;
            /*
             * The admin endpoint is treated specially by makeEndpoint in that instead of linking
             * it into the endpoint chain, it is returned as adminEndpoint.
             */
            rc = makeEndpoint(name, props, &adminEndpoint);
            if (!adminEndpoint) {
                rc = ISMRC_Error;
                TRACE(2, "Unable to make admin endpoint\n");
            }
            if (!rc) {
                endpoint = adminEndpoint;
                rc = ism_transport_startTCPEndpoint(endpoint);
                if (!rc) {
                    linkEndpoint(endpoint);
                    endpoint->needed = 0;
                } else {
                    TRACE(2, "Unable to modify AdminEndpoint.  Revert to previous definition\n");
                    endpoint = getEndpoint(name);
                    endpoint->sslCTX = NULL;   /* was destroyed by the previous startTCPEndpoint */
                    endpoint->needed = ENDPOINT_NEED_ALL;
                    xrc = ism_transport_startTCPEndpoint(endpoint);
                    if (xrc) {
                        TRACE(2, "Unable to revert AdminEndpoint to previous defintion\n");
                        /* TODO: shutdown */
                    }
                }
            }
        }

        else if (!strcmp(object, "MQConnectivityEnabled")) {
            int enabled = ism_common_getBooleanProperty(props, "MQConnectivityEnabled", 0);
            if(!enabled)
                ism_admin_stop_mqc_channel();
            else
                ism_admin_start_mqc_channel();
        }
        break;

    /*
     * Process deletion of object
     */
    case ISM_CONFIG_CHANGE_DELETE:
        if (!strcmp(object, "Endpoint")) {
            endpoint = getEndpoint(name);
            if (endpoint && endpoint->enabled) {
                endpoint->enabled = 0;
                endpoint->needed = ENDPOINT_NEED_ALL;
                ism_transport_startTCPEndpoint(endpoint);
                endpoint->needed = 0;
            }
            unlinkEndpoint(name);
        }
        else if (!strcmp(object, "SecurityProfile")) {
            rc = unlinkSecProfile(name);
            endpoint = endpoints;
            while (endpoint) {
                if (endpoint->secprof) {
                    if (endpoint->enabled && !strcmp(name, endpoint->secprof)) {
                        endpoint->rc = ISMRC_NoSecProfile;
                        endpoint->enabled = 0;
                        endpoint->needed = ENDPOINT_NEED_ALL;
                        ism_transport_startTCPEndpoint(endpoint);
                        endpoint->needed = 0;
                    }
                }
                endpoint = endpoint->next;
            }
        }

        else if (!strcmp(object, "CertificateProfile")) {
            rc = unlinkCertProfile(name);
            endpoint = endpoints;
            while (endpoint) {
                if (endpoint->secprof) {
                    ism_secprof_t * secprof = ism_transport_getSecProfile(endpoint->secprof);
                    if (endpoint->enabled && secprof && secprof->certprof &&
                            !strcmp(name, secprof->certprof)) {
                        endpoint->rc = ISMRC_NoCertProfile;
                        endpoint->enabled = 0;
                        endpoint->needed = ENDPOINT_NEED_ALL;
                        ism_transport_startTCPEndpoint(endpoint);
                        endpoint->needed = 0;
                    }
                }
                endpoint = endpoint->next;
            }
        }
        else if (!strcmp(object, "MessageHub")) {
            rc = unlinkMsgHub(name);
            /* On rename disable any endpoints using this hub */
            endpoint = endpoints;
            while (endpoint) {
                if (endpoint->enabled && endpoint->msghub && !strcmp(name, endpoint->msghub)) {
                    endpoint->rc = ISMRC_MessageHub;
                    endpoint->enabled = 0;
                    endpoint->needed = ENDPOINT_NEED_ALL;
                    ism_transport_startTCPEndpoint(endpoint);
                    endpoint->needed = 0;
                }
                endpoint = endpoint->next;
            }
        }

        break;
    }
    pthread_mutex_unlock(&endpointlock);
    return rc;
}

/*
 * Notify transport that the connection is ready for processing
 */
void ism_transport_connectionReady(ism_transport_t * transport) {
    /* Log the connection now we know the client ID and user ID */
    if (!ism_transport_noLog(transport)) {
        LOG(INFO, Connection, 1117, "%u%-s%-s%-s%s%-s%d",
            "Create {4} connection: ConnectionID={0} ClientID={1} Endpoint={2} UserID={3} CommonName={5} Durable={6}.",
                        transport->index, transport->name, transport->endpoint_name,
                        transport->userid ? transport->userid : "",
                        transport->protocol_family,
                        transport->cert_name ? transport->cert_name : "", transport->durable);
    } else {
        TRACE(6, "Create %s connection: ConnectionID=%u ClientID=%s Endpoint=%s UserID=%s CommonName=%s",
                transport->protocol_family,
                transport->index, transport->name, transport->endpoint_name,
                transport->userid ? transport->userid : "",
                transport->cert_name ? transport->cert_name : "");
    }
}


/*
 * Set a property when constructing transport config objects
 */
static void setProp(ism_prop_t * props, const char * obj, const char * name, const char * item, const char * value) {
    char   cfgname [1024];
    ism_field_t f = {0};

    if (value) {
        sprintf(cfgname, "%s.%s.%s", obj, item, name);
        f.type = VT_String;
        f.val.s = (char *)value;
        ism_common_setProperty(props, cfgname, &f);
    }
}

static int expire_min = 1;
static int expire_max = 0;

/**
 * Set connection expiration.
 *
 * At some some after the expiration time, the expire() method on transport is called if it is non-null.
 * If it is null the connection is closed.
 * If the expiration time is zero, the connection does not expire.
 *
 * @param transport  The connection
 * @param expire     The expiration time
 * @return The previous expiration time
 */
ism_time_t ism_transport_setConnectionExpire(ism_transport_t * transport, ism_time_t expire) {
    ism_time_t ret = 0;
    pthread_mutex_lock(&monitorlock);
    ret = transport->expireTime;
    transport->expireTime = expire;
    if (expire) {
        if (transport->monitor_id == 0) {
            expire_min = 1;
            expire_max = monitor_used;
        } else {
            if (transport->monitor_id < expire_min)
                expire_min = transport->monitor_id;
            if (transport->monitor_id > expire_max-1)
                expire_max = transport->monitor_id;
        }
    }
    pthread_mutex_unlock(&monitorlock);
    return ret;
}


/*
 * Do the actual close on the delivery thread
 */
static int doExpire(ism_transport_t * transport, void * userdata, int flags) {
    if (transport->expire) {
        transport->expire(transport);
    } else {
        transport->expireTime = 0;
        ism_common_setError(ISMRC_ConnectionExpired);
        transport->close(transport, ISMRC_ConnectionExpired, 0, "Connection expired");
    }
    return 0;
}

/*
 * Timer task to periodically check the connection has expired
 */
static int checkExpire(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    int   i;
    ism_transport_t * transport;

    pthread_mutex_lock(&monitorlock);
    int newmin = 0;
    int newmax = 0;
    for (i=expire_min; i<=expire_max; i++) {
        if (IS_VALID_MON(i)) {
            transport = monitorlist[i];
            if (transport->expireTime) {
                if (timestamp >= transport->expireTime) {
                    TRACEL(6, transport->trclevel, "Expire connection: id=%d index=%u client=%s\n", transport->monitor_id, transport->index, transport->name);
                    transport->addwork(transport, doExpire, NULL);
                }
                newmax = transport->monitor_id;
                if (newmin == 0)
                    newmin = transport->monitor_id;
            }
        }
    }
    if (newmin == 0)
        newmin = 1;
    expire_min = newmin;
    expire_max = newmax;
    pthread_mutex_unlock(&monitorlock);
    return 1;
}

/*
 * setendpoint command
 */
int ism_transport_setEndpoint(char * args) {
    const char * name = NULL;
    const char * enabled = "1";
    const char * interface = NULL;
    const char * port = NULL;
    const char * protocol = NULL;
    const char * transp   = NULL;
    const char * security = NULL;
    const char * secprof = NULL;
    const char * conpol = NULL;
    const char * topicpol = NULL;
    const char * subpol = NULL;
    const char * qpol = NULL;
    const char * about = NULL;
    const char * msghub = NULL;
    const char * maxmsgsize = NULL;
    int    op = ISM_CONFIG_CHANGE_PROPS;
    char * keyword;
    char * cp;
    char * value;
    int    rc = 0;
    char   errstr [64];

    TRACE(5, "setendpoint: %s\n", args ? args : "");
    ism_prop_t * props = ism_common_newProperties(20);
    while (args && *args) {
        keyword = ism_common_getToken(args, " \t\r\n", "=\r\n", &args);
        if (keyword && *keyword) {
            cp = keyword+strlen(keyword);     /* trim trailing spaces */
            while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                cp--;
            *cp = 0;
            value   = ism_common_getToken(args, " =\t\r\n", ";\r\n", &args);
            if (value && * value) {
                cp = value+strlen(value);     /* trim trailing spaces */
                while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                    cp--;
                *cp = 0;
            }
       //     printf("! name=[%s] value=[%s]\n", keyword, value);
            if (!strcmpi(keyword, "name"))
                name = value;
            else if (!strcmpi(keyword, "enabled"))
                enabled = value;
            else if (!strcmpi(keyword, "interface") || !strcmpi(keyword, "ipaddr"))
                interface = value;
            else if (!strcmpi(keyword, "port"))
                port = value;
            else if (!strcmpi(keyword, "protocol"))
                protocol = value;
            else if (!strcmpi(keyword, "transport"))
                transp = value;
            else if (!strcmpi(keyword, "security"))
                security = value;
            else if (!strcmpi(keyword, "maxmsgsize") || !strcmpi(keyword, "maxmessagesize"))
                maxmsgsize = value;
            else if (!strcmpi(keyword, "secprof") || !strcmpi(keyword, "securityprofile"))
                secprof = value;
            else if (!strcmpi(keyword, "conpolicies") || !strcmpi(keyword, "connectionpolicies"))
                conpol = value;
            else if (!strcmpi(keyword, "topicpolicies"))
                topicpol = value;
            else if (!strcmpi(keyword, "qpolicies") || !strcmpi(keyword, "queuepolicies"))
                qpol = value;
            else if (!strcmpi(keyword, "subpolicies"))
                subpol = value;
            else if (!strcmpi(keyword, "about"))
                about = value;
            else if (!strcmpi(keyword, "hub") || !strcmpi(keyword, "messagehub") || !strcmpi(keyword, "msghub"))
                msghub = value;
            else if (!strcmpi(keyword, "delete")) {
                op = ISM_CONFIG_CHANGE_DELETE;
            } else {
                printf("Keyword not known: %s\n", keyword);
            }
        }
    }
    if (!name) {
        printf("Name is required\n");
    } else {
        const char * endtype = "Endpoint";
        if (!strcmp(name, "AdminEndpoint")) {
            endtype = "AdminEndpoint";
        }
        setProp(props, endtype, name,  "Enabled", enabled);
        setProp(props, endtype, name,  "Interface", interface);
        setProp(props, endtype, name,  "Port", port);
        setProp(props, endtype, name,  "Protocol", protocol);
        setProp(props, endtype, name,  "Transport", transp);
        setProp(props, endtype, name,  "Security", security);
        setProp(props, endtype, name,  "ConnectionPolicies", conpol);
        setProp(props, endtype, name,  "TopicPolicies", topicpol);
        setProp(props, endtype, name,  "QueuePolicies", qpol);
        setProp(props, endtype, name,  "SubscriptionPolicies", subpol);
        setProp(props, endtype, name,  "SecurityProfile", secprof);
        setProp(props, endtype, name,  "MessageHub", msghub);
        setProp(props, endtype, name,  "MaxMessageSize", maxmsgsize);
        setProp(props, endtype, name,  "EnableAbout", about);
        rc = ism_transport_config((char *)endtype, (char *)name, props, op);
        if (rc) {
            printf("Unable to set endpoint: error=%s (%d)\n",
                    ism_common_getErrorString(rc, errstr, sizeof(errstr)), rc);
        } else {
            ism_transport_printEndpoints(name);
        }
    }
    return 0;
}


/*
 * setsecprof command
 */
XAPI int ism_transport_setSecProf(char * args) {
    const char * name = NULL;
    const char * cert = NULL;
    const char * method = NULL;
    const char * ciphers = NULL;
    const char * clientcert = NULL;
    const char * clientcipher = NULL;
    const char * usepassword = NULL;
    const char * allownullpassword = NULL;
    int    op = ISM_CONFIG_CHANGE_PROPS;
    char * keyword;
    char * cp;
    char * value;
    int    rc = 0;
    char   errstr [64];

    TRACE(5, "setsecprof: %s\n", args ? args : "");
    ism_prop_t * props = ism_common_newProperties(20);
    while (args && *args) {
        keyword = ism_common_getToken(args, " \t\r\n", "=\r\n", &args);
        if (keyword && *keyword) {
            cp = keyword+strlen(keyword);     /* trim trailing spaces */
            while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                cp--;
            *cp = 0;
            value   = ism_common_getToken(args, " =\t\r\n", ";\r\n", &args);
            if (value && * value) {
                cp = value+strlen(value);     /* trim trailing spaces */
                while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                    cp--;
                *cp = 0;
            }
            if (!strcmpi(keyword, "name"))
                name = value;
            else if (!strcmpi(keyword, "certprof"))
                cert = value;
            else if (!strcmpi(keyword, "method"))
                method = value;
            else if (!strcmpi(keyword, "ciphers"))
                ciphers = value;
            else if (!strcmpi(keyword, "clientcipher") || !strcmpi(keyword, "useclientcipher"))
                clientcipher = value;
            else if (!strcmpi(keyword, "clientcert") || !strcmpi(keyword, "useclientcertificate"))
                clientcert = value;
            else if (!strcmpi(keyword, "usepassword"))
                usepassword = value;
            else if (!strcmpi(keyword, "allownullpassword"))
                allownullpassword = value;
            else if (!strcmpi(keyword, "delete")) {
                op = ISM_CONFIG_CHANGE_DELETE;
            } else {
                printf("Keyword not known: %s\n", keyword);
            }
        }
    }
    if (!name) {
        printf("Name is required\n");
    } else {
        setProp(props, "SecurityProfile", name, "CertificateProfile", cert);
        setProp(props, "SecurityProfile", name, "MinimumProtocolMethod", method);
        setProp(props, "SecurityProfile", name, "Ciphers", ciphers);
        setProp(props, "SecurityProfile", name, "UseClientCertificate", clientcert);
        setProp(props, "SecurityProfile", name, "UseClientCipher", clientcipher);
        setProp(props, "SecurityProfile", name, "UsePasswordAuthentication", usepassword);
        setProp(props, "SecurityProfile", name, "AllowNullPassword", allownullpassword);
        rc = ism_transport_config("SecurityProfile", (char *)name, props, op);
        if (rc) {
            printf("Unable to set SecurityProfile: error=%s (%d)\n",
                    ism_common_getErrorString(rc, errstr, sizeof(errstr)), rc);
        } else {
            ism_transport_printCertProfile(name);
        }
    }
    return 0;
}

/*
 * setcertprof command
 */
XAPI int ism_transport_setCertProf(char * args) {
    const char * name = NULL;
    const char * cert = NULL;
    const char * key = NULL;
    int    op = ISM_CONFIG_CHANGE_PROPS;
    char * keyword;
    char * cp;
    char * value;
    int    rc = 0;
    char   errstr [64];

    TRACE(5, "setcertprof: %s\n", args ? args : "");
    ism_prop_t * props = ism_common_newProperties(20);
    while (args && *args) {
        keyword = ism_common_getToken(args, " \t\r\n", "=\r\n", &args);
        if (keyword && *keyword) {
            cp = keyword+strlen(keyword);     /* trim trailing spaces */
            while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                cp--;
            *cp = 0;
            value   = ism_common_getToken(args, " =\t\r\n", ";\r\n", &args);
            if (value && * value) {
                cp = value+strlen(value);     /* trim trailing spaces */
                while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                    cp--;
                *cp = 0;
            }
            if (!strcmpi(keyword, "name"))
                name = value;
            else if (!strcmpi(keyword, "cert") || !strcmpi(keyword, "certificate"))
                cert = value;
            else if (!strcmpi(keyword, "key"))
                key = value;
            else if (!strcmpi(keyword, "delete")) {
                op = ISM_CONFIG_CHANGE_DELETE;
            } else {
                printf("Keyword not known: %s\n", keyword);
            }
        }
    }
    if (!name) {
        printf("Name is required\n");
    } else {
        setProp(props, "CertificateProfile", name,  "Certificate", cert);
        setProp(props, "CertificateProfile", name,  "Key", key);
        rc = ism_transport_config("CertificateProfile", (char *)name, props, op);
        if (rc) {
            printf("Unable to set CertificateProfile: error=%s (%d)\n",
                    ism_common_getErrorString(rc, errstr, sizeof(errstr)), rc);
        } else {
            ism_transport_printCertProfile(name);
        }
    }
    return 0;
}

/*
 * setmsghub command
 */
XAPI int ism_transport_setMsgHub(char * args) {
    const char * name = NULL;
    const char * descr = NULL;
    int    op = ISM_CONFIG_CHANGE_PROPS;
    char * keyword;
    char * cp;
    char * value;
    int    rc = 0;
    char   errstr [64];

    TRACE(5, "setmsghub: %s\n", args ? args : "");
    ism_prop_t * props = ism_common_newProperties(20);
    while (args && *args) {
        keyword = ism_common_getToken(args, " \t\r\n", "=\r\n", &args);
        if (keyword && *keyword) {
            cp = keyword+strlen(keyword);     /* trim trailing spaces */
            while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                cp--;
            *cp = 0;
            value   = ism_common_getToken(args, " =\t\r\n", ",\r\n", &args);
            if (value && * value) {
                cp = value+strlen(value);     /* trim trailing spaces */
                while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                    cp--;
                *cp = 0;
            }
            if (!strcmpi(keyword, "name"))
                name = value;
            else if (!strcmpi(keyword, "descr") || !strcmpi(keyword, "description"))
                descr = value;
            else if (!strcmpi(keyword, "delete")) {
                op = ISM_CONFIG_CHANGE_DELETE;
            } else {
                printf("Keyword not known: %s\n", keyword);
            }
        }
    }
    if (!name) {
        printf("Name is required\n");
    } else {
        if (!descr)
            descr = "";
        setProp(props, "MessageHub", name,  "Description", descr);
        setProp(props, "MessageHub", name,  "Name",        name);
        rc = ism_transport_config("MessageHub", (char *)name, props, op);
        if (rc) {
            printf("Unable to set MessageHub: error=%s (%d)\n",
                    ism_common_getErrorString(rc, errstr, sizeof(errstr)), rc);
        } else {
            ism_transport_printMsgHub(name);
        }
    }
    return 0;
}


/*
 * Look up a SecurityProfile by name
 * This must be called with the endpointlock held
 */
ism_secprof_t * ism_transport_getSecProfile(const char * name) {
    ism_secprof_t * ret = secprofiles;
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
 * Look up a Endpoint by name
 */
static ism_endpoint_t * getEndpoint(const char * name) {
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
 * External version to call from protocol
 */
ism_endpoint_t * ism_transport_findEndpoint(const char * name) {
    return getEndpoint(name);
}

/*
 * Look up a CertificateProfiles by name
 * This must be called with the endpointlock held
 */
ism_certprof_t * ism_transport_getCertProfile(const char * name) {
    ism_certprof_t * ret = certprofiles;
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
 * Look up a message hub by name
 * This must be called with the endpointlock held
 */
ism_msghub_t * ism_transport_getMsgHub(const char * name) {
    ism_msghub_t * ret = msghubs;
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
 * Make a security policy object from the configuration
 */
static int makeSecurityProfile(const char * name, ism_prop_t * props) {
    ism_secprof_t * secprofile;
    ism_secprof_t * oldsecprof;
    char *       cfgname;
    const char * methodstr;
    const char * cipherstr;
    const char * ciphers = NULL;
    const char * certprofile = NULL;
    const char * ltpaprof = NULL;
    const char * oauthprof = NULL;
    int8_t  method  = SecMethod_TLSv1_2;
    int8_t  cipher  = CipherType_Fast;
    int8_t  clientcert = 0;
    int8_t  passwordauth=0;
    int8_t  clientcipher = 0;
    int8_t  tlsenabled = 1;
    int8_t  allownullpassword = 0;

    TRACE(7, "MakeSecurityProfile [%s]\n", name);
    /*
     * Make sure the name is valid
     */
    if (!ism_transport_validName(name)) {
        ism_common_setErrorData(ISMRC_BadConfigName,"%s",name);
        return ISMRC_BadConfigName;
    }

    oldsecprof = ism_transport_getSecProfile(name);
    if (oldsecprof) {
        ismHashMap * allowedClientsMap = oldsecprof->allowedClientsMap;
        if(allowedClientsMap)
            ism_ssl_cleanAllowedClientCerts(allowedClientsMap);
        oldsecprof->allowedClientsMap = NULL;
        cipher = oldsecprof->ciphertype;
        method = oldsecprof->method;
        certprofile = oldsecprof->certprof;
        clientcipher = oldsecprof->clientcipher;
        clientcert = oldsecprof->clientcert;
        passwordauth =oldsecprof->passwordauth;
        ltpaprof = oldsecprof->ltpaprof;
        oauthprof = oldsecprof->oauthprof;
        tlsenabled = oldsecprof->tlsenabled;
        allownullpassword = oldsecprof->allownullpassword;
    }

    cfgname = alloca(strlen(name)+64);
    sprintf(cfgname, "SecurityProfile.MinimumProtocolMethod.%s", name);
    methodstr = ism_common_getStringProperty(props, cfgname);
    if (methodstr && ism_common_enumValue(enum_methods, methodstr) != INVALID_ENUM)
        method = (uint8_t)ism_common_enumValue(enum_methods, methodstr);


    sprintf(cfgname, "SecurityProfile.Ciphers.%s", name);
    cipherstr = ism_common_getStringProperty(props, cfgname);

    if (cipherstr && ism_common_enumValue(enum_ciphers, cipherstr) != INVALID_ENUM)
        cipher = (uint8_t)ism_common_enumValue(enum_ciphers, cipherstr);
    if (cipherstr && *cipherstr == ':') {
        cipher = CipherType_Custom;
        ciphers = cipherstr;
    }

    sprintf(cfgname, "SecurityProfile.UseClientCertificate.%s", name);
    clientcert = (uint8_t)ism_common_getBooleanProperty(props, cfgname, clientcert);

    sprintf(cfgname, "SecurityProfile.UsePasswordAuthentication.%s", name);
    passwordauth = (uint8_t)ism_common_getBooleanProperty(props, cfgname, passwordauth);

    sprintf(cfgname, "SecurityProfile.AllowNullPassword.%s", name);
    allownullpassword = (uint8_t)ism_common_getBooleanProperty(props, cfgname, allownullpassword);

    sprintf(cfgname, "SecurityProfile.UseClientCipher.%s", name);
    clientcipher = (uint8_t)ism_common_getBooleanProperty(props, cfgname, clientcipher);

    sprintf(cfgname, "SecurityProfile.CertificateProfile.%s", name);
    if (ism_common_getStringProperty(props, cfgname))
        certprofile = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname, "SecurityProfile.LTPAProfile.%s", name);
    if (ism_common_getStringProperty(props, cfgname)) {
        ltpaprof = ism_common_getStringProperty(props, cfgname);
        /* If LTPA Profile is specified, turn on Password Authentication. */
        if (ltpaprof != NULL && *ltpaprof!='\0')
            passwordauth = 1;
    }

    sprintf(cfgname, "SecurityProfile.OAuthProfile.%s", name);
    if (ism_common_getStringProperty(props, cfgname)) {
        oauthprof = ism_common_getStringProperty(props, cfgname);
        /* If OAuth Profile is specified, turn on Password Authentication. */
        if (oauthprof != NULL && *oauthprof!='\0')
            passwordauth = 1;
    }

    sprintf(cfgname, "SecurityProfile.TLSEnabled.%s", name);
    tlsenabled = (uint8_t)ism_common_getBooleanProperty(props, cfgname, tlsenabled);

    secprofile = ism_transport_createSecProfile(name, method, cipher, ciphers, certprofile, ltpaprof, oauthprof);
    secprofile->clientcert = clientcert;
    secprofile->passwordauth = passwordauth;
    secprofile->allownullpassword = allownullpassword;
    secprofile->clientcipher = clientcipher;
    if (!clientcipher)
        secprofile->sslop |= SSL_OP_CIPHER_SERVER_PREFERENCE;
    secprofile->tlsenabled = tlsenabled;
    secprofile->allowedClientsMap = ism_ssl_initAllowedClientCerts(secprofile->name);

    linkSecProfile(secprofile);
    ism_transport_dumpSecProfile(8, secprofile, "make", 0);
    return 0;
}


/*
 * Make a security policy object from the configuration
 */
static int makeCertProfile(const char * name, ism_prop_t * props) {
    ism_certprof_t * certprofile;
    ism_certprof_t * oldcertprof;
    char *       cfgname;
    const char * cert = NULL;
    const char * key  = NULL;

    TRACE(7, "MakeCertProfile [%s]\n", name);

    /*
     * Make sure the name is valid
     */
    if (!ism_transport_validName(name)) {
        ism_common_setErrorData(ISMRC_BadConfigName,"%s",name);
        return ISMRC_BadConfigName;
    }

    oldcertprof = ism_transport_getCertProfile(name);
    if (oldcertprof) {
        cert = oldcertprof->cert;
        key  = oldcertprof->key;
    }

    cfgname = alloca(strlen(name)+48);
    sprintf(cfgname, "CertificateProfile.Certificate.%s", name);
    if (ism_common_getStringProperty(props, cfgname))
        cert = ism_common_getStringProperty(props, cfgname);

    sprintf(cfgname, "CertificateProfile.Key.%s", name);
    if (ism_common_getStringProperty(props, cfgname))
        key = ism_common_getStringProperty(props, cfgname);

    certprofile = ism_transport_createCertProfile(name, cert, key);
    linkCertProfile(certprofile);
    ism_transport_dumpCertProfile(8, certprofile, "make", 0);
    return 0;
}

/*
 * Make a message hub
 */
static int makeMsgHub(const char * name, ism_prop_t * props) {
    ism_msghub_t * msghub;
    char *       cfgname;
    char *       data;
    const char * descr = NULL;

    TRACE(7, "MakeMsgHub [%s]\n", name);
    /*
     * Make sure the name is valid
     */
    if (!ism_transport_validName(name)) {
        ism_common_setErrorData(ISMRC_BadConfigName,"%s",name);
        return ISMRC_BadConfigName;
    }

    cfgname = alloca(strlen(name)+48);
    sprintf(cfgname, "MessageHub.Description.%s", name);
    if (ism_common_getStringProperty(props, cfgname))
        descr = ism_common_getStringProperty(props, cfgname);
    if (!descr)
        descr = "";

    msghub = ism_common_malloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 5),sizeof(ism_msghub_t) + strlen(name) + strlen(descr) + 2);
    data = (char *)(msghub+1);
    msghub->name = data;
    strcpy(data, name);
    data += strlen(data)+1;
    msghub->descr = data;
    strcpy(data, descr);
    linkMsgHub(msghub);
    return 0;
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

typedef struct plugin_mask_t {
    struct plugin_mask_t * next;
    uint64_t mask;
    char name[1];
} plugin_mask_t;

static plugin_mask_t * plugin_masks;
static int plugin_proto_count = 0;

/*
 * Find the mask for a protocol, creating it if necessary
 */
uint64_t ism_transport_pluginMask(const char * protocol, int make) {
    plugin_mask_t * plugin;
    plugin_mask_t * last = NULL;
    uint64_t ret;

    if (!strcmpi(protocol, "mqtt"))
        return PMASK_MQTT;
    if (!strcmpi(protocol, "jms"))
        return PMASK_JMS;
    if (!strcmpi(protocol, "rmsg"))
        return PMASK_RMSG;

    plugin = plugin_masks;
    while (plugin) {
        if (!strcmpi(plugin->name, protocol))
            return plugin->mask;
        last = plugin;
        plugin = plugin->next;
    }
    if (make) {
        ret = 0x10000;
        ret <<= plugin_proto_count;
        if (plugin_proto_count < 45)
            plugin_proto_count++;
        plugin = (plugin_mask_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 22),1, sizeof(plugin_mask_t) + strlen(protocol));
        plugin->mask = ret;
        strcpy(plugin->name, protocol);
        if (last)
            last->next = plugin;
        else
            plugin_masks = plugin;
    } else {
        ret = 0;
    }
    return ret;
}

/*
 * Parse a protocol string
 */
uint64_t parseProtocols(const char * str) {
     int count = 0;
     uint64_t ret = 0;
     char * s = alloca(strlen(str)+1);
     strcpy(s, str);
     my_strlwr(s);
     char * token = ism_common_getToken(s, " \t,", " \t,", &s);
     while (token) {
         if (!strcmp(token, "jms")) {
             count++;
             ret |= PMASK_JMS;
         } else if (!strcmp(token, "mqtt")) {
             count++;
             ret |= PMASK_MQTT;
         } else if (!strcmp(token, "admin")) {
             count++;
             ret |= PMASK_Internal;
         } else if (!strcmp(token, "mq")) {
             count++;
             ret |= PMASK_MQConn;
         } else if (!strcmp(token, "rmsg")) {
             count++;
             ret |= PMASK_RMSG;
         } else if (!strcmp(token, "all")) {
             count++;
             ret |= PMASK_AnyProtocol;
         } else {
             plugin_mask_t * plugin = plugin_masks;
             while (plugin) {
                 if (!strcmpi(plugin->name, token)) {
                     ret |= plugin->mask;
                     break;
                 }
                 plugin = plugin->next;
             }
         }
         token = ism_common_getToken(s, " \t,", " \t,", &s);
     }
     if (count == 0) {
         ret = PMASK_AnyProtocol;
     }
     return ret;
}

/*
 * Parse a transport string.
 */
uint32_t parseTransports(const char * str) {
     int count = 0;
     uint32_t ret = 0;
     char * s = alloca(strlen(str)+1);
     strcpy(s, str);
     my_strlwr(s);
     char * token = ism_common_getToken(s, " \t,", " \t,", &s);
     while (token) {
         if (!strcmp(token, "tcp") || !(strcmp(token, "reliable"))) {
             count++;
             ret |= TMASK_TCP;
         } else if (!strcmp(token, "!tcp") || !(strcmp(token, "!reliable"))) {
             count++;
             ret &= ~TMASK_TCP;
         } else if (!strcmp(token, "udp") || !(strcmp(token, "unreliable"))) {
             count++;
             ret |= TMASK_UDP;
         } else if (!strcmp(token, "!udp")|| !(strcmp(token, "!unreliable"))) {
             count++;
             ret &= ~TMASK_UDP;
         } else if (!strcmp(token, "websockets")) {
             count++;
             ret |= TMASK_WebSockets;
         } else if (!strcmp(token, "!websockets")) {
             count++;
             ret &= ~TMASK_WebSockets;
         } else if (!strcmp(token, "all")) {
             count++;
             ret |= TMASK_AnyTrans;
         }
         token = ism_common_getToken(s, " \t,", " \t,", &s);
     }
     if (count == 0) {
         ret = TMASK_AnyTrans;
     }
     return ret;
}


/*
 * Do a basic check if the port is in use
 */
static int checkInUse(ism_endpoint_t * oldendp, const char * interface, int port) {
    ism_endpoint_t * endpoint = endpoints;
    while (endpoint) {
        if (endpoint != oldendp) {
            if (endpoint->enabled) {
                if (interface == NULL || endpoint->ipaddr == NULL || endpoint->ipaddr == interface) {
                    if (port == endpoint->port) {
                        ism_common_setError(ISMRC_PortInUse);
                        return ISMRC_PortInUse;
                    }
                }
            }
        }
        endpoint = endpoint->next;
    }
    return 0;
}


/*
 * Make a endpoint object from the configuration.
 * We can either just reuse the existing endpoint object, or create a new one.
 * In the case of the admin endpoint we always create a new one.
 */
static int makeEndpoint(const char * name, ism_prop_t * props, ism_endpoint_t * * created_endpoint) {
    ism_endpoint_t * endpoint;
    ism_endpoint_t * oldendp;
    char * cfgname;
    const char * secprofile = NULL;
    const char * interface = NULL;
    const char * msghub = NULL;
    const char * conpolicies = NULL;
    const char * topicpolicies = NULL;
    const char * subpolicies = NULL;
    const char * qpolicies = NULL;
    int32_t  port = -1;
    int32_t  secure = -1;
    int32_t  maxmsgsize = 4096*1024;
    uint16_t maxSendSize = 0;
    uint8_t  doNotBatch = 0;
    uint8_t  enableAbout = 0;
    int32_t  msize;
    int32_t  enabled = 1;
    int32_t  explicit_enable = 1;
    uint64_t protomask = PMASK_AnyProtocol;
    uint32_t transmask = TMASK_AnyTrans;
    uint32_t isAdmin = 0;
    uint32_t isInternal = 0;
    const char * pvalue;
    int      fixloc = 9;

    TRACE(7, "MakeEndpoint [%s]\n", name);

    /*
     * Make sure the name is valid
     */
    if (!ism_transport_validName(name)) {
        ism_common_setErrorData(ISMRC_BadConfigName,"%s",name);
        return ISMRC_BadConfigName;
    }

    cfgname = alloca(strlen(name)+48);
    oldendp = getEndpoint(name);

    if (oldendp && oldendp->isAdmin) {
        sprintf(cfgname, "AdminEndpoint.");
        fixloc = 14;
        explicit_enable = 1;
    } else {
        sprintf(cfgname, "Endpoint.");
        sprintf(cfgname+fixloc, "Enabled.%s", name);
        explicit_enable = ism_common_getBooleanProperty(props, cfgname, 9);
    }

    if (oldendp) {
        secprofile = oldendp->secprof;
        conpolicies = oldendp->conpolicies;
        topicpolicies = oldendp->topicpolicies;
        subpolicies = oldendp->subpolicies;
        qpolicies = oldendp->qpolicies;
        interface = oldendp->ipaddr;
        protomask = oldendp->protomask;
        transmask = oldendp->transmask;
        port = oldendp->port;
        secure = oldendp->secure;
        enabled = oldendp->enabled;
        msghub = oldendp->msghub;
        maxmsgsize = oldendp->maxMsgSize;
        maxSendSize = oldendp->maxSendSize;
        doNotBatch = oldendp->doNotBatch;
        isAdmin = oldendp->isAdmin;
        isInternal = oldendp->isInternal;
        enableAbout = oldendp->enableAbout;
        /*
         * Force disconnect on explicit disable
         */
        if (explicit_enable == 0 && !isAdmin) {
            oldendp->enabled = 0;
            oldendp->needed = ENDPOINT_NEED_ALL;
            ism_transport_startTCPEndpoint(oldendp);
            oldendp->needed = 0;
            ism_common_sleep(1000);
            TRACE(6, "Explicit disable of endpoint %s: disconnect all connections\n", oldendp->name);
            int count = ism_transport_disconnectEndpoint(-2, "The connection was closed because the endpoint was disabled.", oldendp->name);
            TRACE(5, "Explicit disable of endpoint %s: disconnected %d connections\n", oldendp->name, count);
        }
    }

    sprintf(cfgname+fixloc, "Port.%s", name);
    port = ism_common_getIntProperty(props, cfgname, port);
    TRACE(7, "makeEndpoint port=%u %s\n", port, ism_common_getStringProperty(props, cfgname));

    if (explicit_enable != 9)
        enabled = explicit_enable;

    sprintf(cfgname+fixloc, "Interface.%s", name);
    pvalue = ism_common_getStringProperty(props, cfgname);
    if (pvalue)
        interface = pvalue;
    if (interface && !*interface)
        interface = NULL;

    sprintf(cfgname+fixloc, "MessageHub.%s", name);
    pvalue = ism_common_getStringProperty(props, cfgname);
    if (pvalue)
        msghub = pvalue;
    if (msghub && !*msghub)
        msghub = NULL;

    sprintf(cfgname+fixloc, "SecurityProfile.%s", name);
    pvalue = ism_common_getStringProperty(props, cfgname);
    if (pvalue)
        secprofile = pvalue;
    if (secprofile && !*secprofile)
        secprofile = NULL;

    sprintf(cfgname+fixloc, "ConnectionPolicies.%s", name);
    pvalue = ism_common_getStringProperty(props, cfgname);
    if (pvalue)
        conpolicies = pvalue;
    if (conpolicies && !*conpolicies)
        conpolicies = NULL;

    sprintf(cfgname+fixloc, "TopicPolicies.%s", name);
    pvalue= ism_common_getStringProperty(props, cfgname);
    if (!pvalue) {
        sprintf(cfgname+fixloc, "MessagingPolicies.%s", name);
        pvalue= ism_common_getStringProperty(props, cfgname);
    }
    if (pvalue)
        topicpolicies = pvalue;

    sprintf(cfgname+fixloc, "SubscriptionPolicies.%s", name);
    pvalue= ism_common_getStringProperty(props, cfgname);
    if (pvalue)
        subpolicies = pvalue;

    sprintf(cfgname+fixloc, "QueuePolicies.%s", name);
    pvalue= ism_common_getStringProperty(props, cfgname);
    if (pvalue)
        qpolicies = pvalue;

    sprintf(cfgname+fixloc, "Protocol.%s", name);
    pvalue = ism_common_getStringProperty(props, cfgname);
    if (pvalue)
        protomask = parseProtocols(pvalue);

    sprintf(cfgname+fixloc, "Transport.%s", name);
    pvalue = ism_common_getStringProperty(props, cfgname);
    if (pvalue)
        transmask = parseTransports(pvalue);

    sprintf(cfgname+fixloc, "MaxMessageSize.%s", name);
    pvalue = ism_common_getStringProperty(props, cfgname);
    if (pvalue) {
        msize = getUnitSize(pvalue);
        if (msize == 0)
            msize = maxmsgsize;
        if (msize < 1024)
            msize = 1024;
        maxmsgsize = msize;
    }
    sprintf(cfgname+fixloc, "MaxSendSize.%s", name);
    pvalue = ism_common_getStringProperty(props, cfgname);
    if (pvalue) {
        msize = getUnitSize(pvalue);
        if (msize && (msize < 128))
            msize = 128;
        maxSendSize = msize;
    }
    sprintf(cfgname+fixloc, "BatchMessages.%s", name);
    pvalue = ism_common_getStringProperty(props, cfgname);
    if (pvalue) {
        doNotBatch = (ism_common_getIntProperty(props, cfgname, 1)) ? 0 : 1;
    }

    sprintf(cfgname+fixloc, "EnableAbout.%s", name);
    pvalue = ism_common_getStringProperty(props, cfgname);
    if (pvalue) {
        enableAbout = (ism_common_getIntProperty(props, cfgname, 1)) ? 1 : 0;
    }

    if (interface && (*interface==0 || !strcmpi(interface, "all")))
        interface = NULL;

    /* Check for port in use */
    if (enabled && checkInUse(oldendp, interface, port)) {
        ism_common_setError(ISMRC_PortInUse);
        return ISMRC_PortInUse;
    }


    /* If we are only changing simple things, just change the structure and redo */
    if (oldendp &&  !created_endpoint &&
            !mystrcmp(interface, oldendp->ipaddr) &&
            !mystrcmp(secprofile, oldendp->secprof) &&
            !mystrcmp(conpolicies, oldendp->conpolicies) &&
            !mystrcmp(topicpolicies, oldendp->topicpolicies) &&
            !mystrcmp(qpolicies, oldendp->qpolicies) &&
            !mystrcmp(subpolicies, oldendp->subpolicies) &&
            !mystrcmp(msghub, oldendp->msghub)) {
        endpoint = oldendp;
        if (endpoint->port != port || endpoint->rc ||
            endpoint->enabled != enabled || endpoint->secure != secure) {
            TRACE(5, "Endpoint simple stop and start: name=%s needed=%d enabled=%d:%d port=%d:%d secure=%d:%d\n",
                    endpoint->name, endpoint->needed, enabled, oldendp->enabled, port, oldendp->port,
                    secure, oldendp->secure);
            endpoint->needed = ENDPOINT_NEED_ALL;
        } else {
            TRACE(5, "Endpoint simple update: name=%s needed=%d enabled=%d:%d port=%d:%d secure=%d:%d\n",
                    endpoint->name, endpoint->needed, enabled, oldendp->enabled, port, oldendp->port,
                    secure, oldendp->secure);
        }
        endpoint->port = port;
        endpoint->enabled = enabled;
        endpoint->secure = secure;
        if (!secprofile) {
            endpoint->usePasswordAuth=0;
        }
        endpoint->protomask = protomask;
        endpoint->transmask = transmask;
        endpoint->maxMsgSize = maxmsgsize;
        endpoint->maxSendSize = maxSendSize;
        endpoint->doNotBatch = doNotBatch;
        endpoint->enableAbout = enableAbout;
        endpoint->oldendp = 1;
        endpoint->rc = 0;
        endpoint->isAdmin = isAdmin;
        endpoint->isInternal = isInternal;
    } else {
        endpoint = ism_transport_createEndpoint(name, msghub, "tcp", interface, secprofile,
                conpolicies, topicpolicies, qpolicies, subpolicies, !oldendp);
        endpoint->port = port;
        endpoint->enabled = enabled;
        endpoint->secure = secure;
        if (!secprofile) {
            endpoint->usePasswordAuth = 0;
            endpoint->secure = 0;
        }
        endpoint->protomask = protomask;
        endpoint->transmask = transmask;
        endpoint->maxMsgSize = maxmsgsize;
        endpoint->maxSendSize = maxSendSize;
        endpoint->doNotBatch = doNotBatch;
        endpoint->enableAbout = enableAbout;
        endpoint->isAdmin = isAdmin;
        endpoint->isInternal = isInternal;
        endpoint->config_time = ism_common_currentTimeNanos();
        endpoint->rc = 0;
        if (oldendp) {
            endpoint->oldendp = 1;
            endpoint->sock = oldendp->sock;

            endpoint->sslCTX1 = oldendp->sslCTX1;
            if (oldendp->sslCTX == &oldendp->sslCTX1)
                endpoint->sslCTX = &endpoint->sslCTX1;
            else
                endpoint->sslCTX = oldendp->sslCTX;
            oldendp->sslCTX = NULL;
            endpoint->sslctx_count = oldendp->sslctx_count;
            endpoint->useClientCert = oldendp->useClientCert;
            if (secprofile) {
                endpoint->usePasswordAuth = oldendp->usePasswordAuth;
            }
            endpoint->needed = oldendp->needed;
            endpoint->stats = oldendp->stats;
            if (!endpoint->sock || endpoint->needed || endpoint->enabled != oldendp->enabled ||
                endpoint->port != oldendp->port || mystrcmp(endpoint->ipaddr, oldendp->ipaddr) ||
                endpoint->secure != oldendp->secure || mystrcmp(endpoint->secprof, oldendp->secprof)) {
                endpoint->needed = ENDPOINT_NEED_ALL;
                TRACE(5, "Endpoint stop and start: name=%s needed=%d enabled=%d:%d port=%d:%d ipaddr=%s:%s secure=%d:%d secprof=%s:%s\n",
                        endpoint->name, endpoint->needed, endpoint->enabled, oldendp->enabled, endpoint->port, oldendp->port,
                        endpoint->ipaddr ? endpoint->ipaddr : "(null)", oldendp->ipaddr ? oldendp->ipaddr : "(null)",
                        endpoint->secure, oldendp->secure,
                        endpoint->secprof ? endpoint->secprof : "(null)", oldendp->secprof ? oldendp->secprof : "(null)");
            } else {
                /* Copy stats */
                endpoint->config_time      = oldendp->config_time;        /* Time of configuration                  */
                endpoint->thread_count = oldendp->thread_count;
                TRACE(5, "Endpoint modified without restarting: name=%s needed=%d enabled=%d port=%d\n",
                        endpoint->name, endpoint->needed, enabled, port);
                if (g_messaging_started) {
                    if (ism_transport_updateTCPEndpoint(endpoint)) {
                        TRACE(5, "Endpoint update failed, do full start/stop: name=%s\n", endpoint->name);
                        endpoint->needed = ENDPOINT_NEED_ALL;
                    }
                }
            }
        } else {
            endpoint->needed = ENDPOINT_NEED_ALL;
            TRACE(5, "Create a new endpoint: name=%s needed=%d enabled=%d port=%d\n",
                    endpoint->name, endpoint->needed, enabled, port);
        }
        if (created_endpoint) {
            *created_endpoint = endpoint;
        } else {
            linkEndpoint(endpoint);
        }
    }
    ism_transport_dumpEndpoint(7, endpoint, "make", 0);
    return 0;
}

XAPI void ism_engine_threadInit(uint8_t isStoreCrit);

/*
 * Initialize the engine thread local storage in the timer thread
 */
static int tls_init_timer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_engine_threadInit(0); /* BEAM suppression: no effect */
    ism_common_cancelTimer(key);
    return 0;
}

/*
 * Start the transport.
 * This is called near the beginning of the server startup and creates the necessary
 * threads and endpoint objects, but only starts the admin and cluster messaging endpoints.
 */
int ism_transport_start(void) {
    int rc = 0;
    int i;
    int proplen;
    ism_endpoint_t * endpoint;
    ism_prop_t * props;
    ism_prop_t * cprops;
    const char * propertyName = NULL;
    ism_config_t * clusterhandle = NULL;

    TRACE(4, "Start transport\n");

    /* Initialize the engine thread local storage for timer threads */
    ism_common_setTimerOnce(ISM_TIMER_LOW, tls_init_timer, NULL, 1);
    ism_common_setTimerOnce(ISM_TIMER_HIGH, tls_init_timer, NULL, 1);

    /* Get the dynamic config */
    props = ism_config_getProperties(transporthandle, NULL, NULL);
    clusterhandle = ism_config_getHandle(ISM_CONFIG_COMP_CLUSTER, NULL);
    cprops = ism_config_getProperties(clusterhandle, NULL, NULL);

    /*
     * Make objects from dynamic config.
     * We need to make these in the correct order
     */
    pthread_mutex_lock(&endpointlock);

    /*
     * If cluster is enabled
     */
    const char * msgLocalAddr = ism_common_getStringProperty(cprops, "ClusterMembership.MessagingAddress.cluster");
    const char * ctlLocalAddr = ism_common_getStringProperty(cprops, "ClusterMembership.ControlAddress.cluster");
    int          msgLocalPort = ism_common_getIntProperty(cprops, "ClusterMembership.MessagingPort.cluster", 0);
    int          secure       = ism_common_getIntProperty(cprops, "ClusterMembership.MessagingUseTLS.cluster",
                                    ism_common_getIntConfig("ClusterDataUseTLS", 0));
    int          requireCerts = ism_common_getIntProperty(cprops, "ClusterMembership.RequireCertificates.cluster",1)? 1 : 2;
    int          cunit        = ism_common_getIntConfig("ClusterCUnit", 0);
    const char * msgExtAddr   = ism_common_getStringProperty(cprops, "ClusterMembership.MessagingExternalAddress.cluster");
    int          msgExtPort   = ism_common_getIntProperty(cprops, "ClusterMembership.MessagingExternalPort.cluster", msgLocalPort);


    /*
     * Default values as necessary
     */
    if (msgLocalAddr == NULL)
        msgLocalAddr = ism_common_getStringConfig("ClusterDataLocalAddr");   /* temp static config */
    if (msgLocalAddr == NULL  || *msgLocalAddr == 0)
        msgLocalAddr = ctlLocalAddr;
    if (msgLocalAddr == NULL)
        msgLocalAddr = "All";
    if (msgLocalPort == 0)
        msgLocalPort = ism_common_getIntConfig("ClusterDataLocalPort", 0);    /* temp static config */
    if (msgLocalPort == 0)
        msgLocalPort = 9103;
    if (msgExtAddr == NULL || *msgExtAddr == 0)
        msgExtAddr = msgLocalAddr;
    if (msgExtAddr == NULL || !strcmpi(msgExtAddr, "all")) {
        msgExtAddr = ctlLocalAddr;
    }
    if (msgExtPort == 0)
        msgExtPort = msgLocalPort;


    if (!cunit) {
        rc = ism_cluster_setLocalForwardingInfo(ism_common_getServerName(), ism_common_getServerUID(),
                msgExtAddr, msgExtPort, secure);
    } else {
        TRACE(4, "Forwarder CUnit mode = %u\n", cunit);
    }

    /*
     * If cluster is active, create the forwarder endpoint
     */
    if (rc == 0) {
        if (msgLocalPort == 0)
            return ISMRC_ClusterDisabled;

        ism_transport_forwarder_active = 1;
        TRACE(4, "Enable forwarder endpoint: SeverName=%s ServerUID=%s MessagingAddress=%s MessagingPort=%u"
                 " MessagingUseTLS=%u MessagingExternalAddress=%s MessagingExternalPort=%u\n",
                 ism_common_getServerName(), ism_common_getServerUID(), msgLocalAddr, msgLocalPort,
                 secure, msgExtAddr, msgExtPort);

        endpoint = ism_transport_createEndpoint("!Forwarder", NULL, "tcp", msgLocalAddr, NULL, NULL, NULL, NULL, NULL, 1);
        endpoint->port = msgLocalPort;
        endpoint->enabled = 1;
        endpoint->protomask = PMASK_Cluster;
        endpoint->transmask = TMASK_AnyTrans | TMASK_AnySecure;
        endpoint->needed = ENDPOINT_NEED_ALL;
        endpoint->config_time = ism_common_currentTimeNanos();
        endpoint->secure = 2;
        endpoint->isInternal = 2;
        endpoint->maxSendBufferSize = 8*1024*1024;
        endpoint->maxRecvBufferSize = 8*1024*1024;

        FILE * f_cert;
        FILE * f_key;
        char * cert;
        char * key;
        cert = fwd_certdata;
        key = fwd_keydata;


        char const * dir = ism_common_getStringConfig("KeyStore");

        if ( dir ) {
            char * cert_file = alloca(18 + strlen(dir));
            strcpy(cert_file,dir);
            strcat(cert_file, "/Cluster_cert.pem");
            f_cert = fopen(cert_file, "rb");
            char * key_file = alloca(18 + strlen(dir));
            strcpy(key_file,dir);
            strcat(key_file, "/Cluster_key.pem");
            f_key = fopen(key_file, "rb");

            if (f_cert && f_key) {

                cert = "Cluster_cert.pem";
                key = "Cluster_key.pem";
                TRACE(5, "Using user provided credentials for Clustering\n");
                fclose(f_key);
                fclose(f_cert);
                requireCerts = 1;
            }
        }

        endpoint->sslCTX1 = ism_common_create_ssl_ctx(endpoint->name, "TLSv1.2", "ECDHE-ECDSA-AES128-GCM-SHA256",
            cert, key, 1, 0x010003FF, requireCerts, endpoint->name, NULL, "Cluster");

        if (endpoint->sslCTX1 == NULL) {
            endpoint->enabled = 0;
            endpoint->rc = ISMRC_CreateSSLContext;
            ism_common_setError(endpoint->rc);
            TRACE(1, "The SSL/TLS context could not be created for endpoint %s\n", endpoint->name);
            /* This is logged in ism_common_create_ssl_ctl() */
            return endpoint->rc;
        }
        endpoint->sslCTX = &endpoint->sslCTX1;
        endpoint->sslctx_count = 1;
        linkEndpoint(endpoint);
        TRACE(5, "Create cluster messaging endpoint: ipaddr=%s port=%d\n", msgLocalAddr, msgLocalPort);
        TRACE(5, "Set local cluster info: name=%s uid=%s addr=%s port=%d secure=%d\n",
                ism_common_getServerName(), ism_common_getServerUID(),
                msgExtAddr, msgExtPort, secure);
    }

    /* Create message hubs */
    for (i = 0; ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++) {
        proplen = strlen(propertyName);
        if (proplen >= 16 && !memcmp(propertyName, "MessageHub.Name.", 16)) {
            makeMsgHub(propertyName+16, props);
        }
    }

    /* Create certificate profiles */
    for (i = 0; ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++) {
        proplen = strlen(propertyName);
        if (proplen >= 31 && !memcmp(propertyName, "CertificateProfile.Certificate.", 31)) {
            makeCertProfile(propertyName+31, props);
        }
    }

    /* Create security profiles */
    for (i = 0; ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++) {
        proplen = strlen(propertyName);
        if (proplen >= 21 &&  !memcmp(propertyName, "SecurityProfile.Name.", 21)) {
            makeSecurityProfile(propertyName+21, props);
        }
    }

    /* Create endpoints */
    for (i = 0; ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++) {
        proplen = strlen(propertyName);
        if (proplen >= 14 && !memcmp(propertyName, "Endpoint.Name.", 14)) {
            makeEndpoint(propertyName+14, props, NULL);
        }
    }
    pthread_mutex_unlock(&endpointlock);

    /* Init WebSockets frame.  We do this here to be after all other initialization */
    ism_transport_frame_init();
    ism_transport_wsframe_init();

    /* Start TCP */
    rc = ism_transport_startTCP();

    /* Start Admin IOP */
    ism_transport_startAdminIOP();

    /* Start the admin endpoint */
    endpoint = getEndpoint("AdminEndpoint");
    if (endpoint) {
        rc = ism_transport_startTCPEndpoint(endpoint);
    } else {
        TRACE(1, "No admin endpoint is defined\n");
        rc = ISMRC_Error;
    }

    /* Start the MQConnectivityEndpoint and start MQC channel if it is enabled */
    endpoint = getEndpoint("!MQConnectivityEndpoint");
    if (endpoint) {
        rc = ism_transport_startTCPEndpoint(endpoint);
        if (!rc) {
            if (ism_config_getMQConnEnabled()) {
                ism_admin_start_mqc_channel();
            }
        }
    } else {
        TRACE(3, "Warning: No MQConnectivity endpoint is defined.\n");
    }

    if (props)
        ism_common_freeProperties(props);
    if (cprops)
        ism_common_freeProperties(cprops);
    return rc;
}

/*
 * Create the forwarder client context here, as we know the cert
 */
XAPI struct ssl_ctx_st * ism_transport_clientTlsContext(const char * name, const char * tlsversion, const char * cipher) {
    return ism_common_create_ssl_ctx(name, tlsversion, cipher,
        fwd_certdata, fwd_keydata, 0, 0x010003FF, 0, name, NULL, NULL);
}


/*
 * Allow out of order linkage
 */
fwd_stat_f  getFwdStat = NULL;
fwd_monstat_f getFwdMonStat = NULL;
XAPI void ism_transport_registerFwdStat(fwd_stat_f fwdstat, fwd_monstat_f fwdmonstat) {
    getFwdStat = fwdstat;
    getFwdMonStat = fwdmonstat;
}

/* Thunk for getFowarderStats */
int  ism_fwd_getForwarderStats(concat_alloc_t * buf, int option) {
    if (getFwdStat)
        return getFwdStat(buf, option);
    return 0;
}

/* Thunk for getForwarderMonitorStats */
int ism_fwd_getMonitoringStats(fwd_monstat_t * monstat, int option) {
    if (getFwdMonStat)
        return getFwdMonStat(monstat, option);
    memset(monstat, 0, sizeof(fwd_monstat_f));
    return ISMRC_Error;
}

/*
 * Start messaging.
 *
 * This is called after all recovery is done, and starts up the rest of the messaage
 * endpoints.
 */
int ism_transport_startMessaging(void) {
    ism_endpoint_t * endpoint;
    int   rc;

    TRACE(4, "Transport start messaging\n");
    g_messaging_started = 1;
    ism_common_startMessaging();

    /* Start the cluster forwader endpoint */
    endpoint = getEndpoint("!Forwarder");
    if (endpoint) {
        rc = ism_transport_startTCPEndpoint(endpoint);
    }


    /*
     * Call all registered protocols to tell them messaging is starting
     */
    protocol_chain * prot = protocols;
    while (prot) {
        if (prot->onStart) {
            xUNUSED int rc1 = prot->onStart();
            /* TODO: Handle errors  */
        }
        prot = prot->next;
    }

    /*
     * Start endpoints
     */
    pthread_mutex_lock(&endpointlock);
    endpoint = endpoints;
    while (endpoint) {
        TRACE(6, "Start endpoint name=%s need=%d\n", endpoint->name, endpoint->needed);
        rc = ism_transport_startTCPEndpoint(endpoint);
        if (!rc) {
            endpoint->needed = 0;
        }
        endpoint = endpoint->next;
    }
    pthread_mutex_unlock(&endpointlock);

    /*
     * Set the expire timer
     */
    int expireRate = ism_common_getIntConfig("ExpireRate", 30);
    ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t)checkExpire, NULL, 7, expireRate, TS_SECONDS);
    return 0;
}

/*
 * Link a new security profile
 * If there is already one by this name, free it
 *
 * This is called with the endpointlock held
 */
static void linkSecProfile(ism_secprof_t * secprof) {
    ism_secprof_t * sp = (ism_secprof_t *)&secprofiles;
    while (sp->next) {
        if (!strcmp(secprof->name, sp->next->name)) {
            ism_secprof_t * oldprof = sp->next;
            secprof->next = sp->next->next;
            sp->next = secprof;
            ism_common_free(ism_memory_transportProfile,oldprof);
            return;
        }
        sp = sp->next;
    }
    secprof->next = NULL;
    sp->next = secprof;
    secprofile_count++;
}

static int unlinkSecProfile(const char * name) {
    ism_secprof_t * sp = (ism_secprof_t *)&secprofiles;
    while (sp->next) {
        if (!strcmp(name, sp->next->name)) {
            ism_secprof_t * oldprof = sp->next;
            sp->next = sp->next->next;
            if(oldprof->allowedClientsMap)
                ism_ssl_cleanAllowedClientCerts(oldprof->allowedClientsMap);
            ism_common_free(ism_memory_transportProfile,oldprof);
            secprofile_count--;
            return 0;
        }
        sp = sp->next;
    }
    ism_common_setErrorData(ISMRC_NotFound, "%s", name);
    return ISMRC_NotFound;
}
/*
 * Link a new endpoint profile
 * If there is already one by this name, move it to the old chain
 *
 * This is called with the endpointlock held
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
    uintptr_t xptr = (uintptr_t)&endpoints;         /* Overcome gcc 4.4 bug fixed in later versions */
    ism_endpoint_t * lp = (ism_endpoint_t *)xptr;
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
 * Link a new security profile
 * If there is already one by this name, free it
 *
 * This is called with the endpointlock held
 */
static void linkCertProfile(ism_certprof_t * certprof) {
    ism_certprof_t * cp = (ism_certprof_t *)&certprofiles;
    while (cp->next) {
        if (!strcmp(certprof->name, cp->next->name)) {
            ism_certprof_t * oldcert = cp->next;
            certprof->next = cp->next->next;
            cp->next = certprof;
            ism_common_free(ism_memory_transportProfile,oldcert);
            return;
        }
        cp = cp->next;
    }
    certprof->next = NULL;
    cp->next = certprof;
    certprofile_count++;
}

/*
 * Unlink a certificate profile
 */
static int unlinkCertProfile(const char * name) {
    ism_certprof_t * cp = (ism_certprof_t *)&certprofiles;
    while (cp->next) {
        if (!strcmp(name, cp->next->name)) {
            ism_certprof_t * oldcert = cp->next;
            cp->next = cp->next->next;
            ism_common_free(ism_memory_transportProfile,oldcert);
            certprofile_count--;
            return 0;
        }
        cp = cp->next;
    }
    ism_common_setErrorData(ISMRC_NotFound, "%s", name);
    return ISMRC_NotFound;
}

/*
 * Link a new message hub
 * If there is already one by this name, free it
 *
 * This is called with the endpointlock held
 */
static void linkMsgHub(ism_msghub_t * msghub) {
    ism_msghub_t * cp = (ism_msghub_t *)&msghubs;
    while (cp->next) {
        if (!strcmp(msghub->name, cp->next->name)) {
            ism_msghub_t * oldhub = cp->next;
            msghub->next = cp->next->next;
            cp->next = msghub;
            /* Link onto the old chain.  We never delete msghubs */
            oldhub->next = old_msghubs;
            old_msghubs = oldhub;
            return;
        }
        cp = cp->next;
    }
    msghub->next = NULL;
    cp->next = msghub;
    msghub_count++;
}

/*
 * Unlink a message hub
 */
static int unlinkMsgHub(const char * name) {
    ism_msghub_t * cp = (ism_msghub_t *)&msghubs;
    while (cp->next) {
        if (!strcmp(name, cp->next->name)) {
            ism_msghub_t * oldhub = cp->next;
            cp->next = cp->next->next;
            /* Link onto the old chain.  We never delete msghubs */
            oldhub->next = old_msghubs;
            old_msghubs = oldhub;
            msghub_count--;
            return 0;
        }
        cp = cp->next;
    }
    ism_common_setErrorData(ISMRC_NotFound, "%s", name);
    return ISMRC_NotFound;
}


/*
 * Print statistics
 */
void ism_transport_printStats(const char * names) {
    ism_connect_mon_t * moncon = NULL;
    ism_connect_mon_t * transport;
    int  position = 0;
    int  count;
    int  i;
    const char * epNames = NULL;

    if (names && strlen(names)>9 && !memcmp(names, "endpoint=", 9)) {
        epNames = names+9;
        names = NULL;
    }
    count = ism_transport_getConnectionMonitor(&moncon, 100, &position, 0,
            names, NULL, epNames, NULL, NULL, 0, 0xffff, ISM_INCOMING_CONNECTION | ISM_OUTGOING_CONNECTION);
    transport = moncon;
    for (i=0; i<count; i++) {
        printf("Connection id=%d name=%s from=%s readbytes=%llu readmsg=%llu writebytes=%llu writemsg=%llu lost=%llu\n",
                transport->index, transport->name, transport->client_addr, (ULL) transport->read_bytes,
                (ULL) transport->read_msg, (ULL) transport->write_bytes, (ULL) transport->write_msg,
                (ULL) transport->lost_msg);
        transport++;
    }
    if (count == 0) {
        printf("There are no active connections.\n");
    } else {
        ism_transport_freeConnectionMonitor(moncon);
    }
}

int ism_transport_term_endpoints(void) {
    ism_endpoint_t * endpoint;

    /*
     * Disable Admin endpoints
     */
    pthread_mutex_lock(&endpointlock);
    endpoint = endpoints;
    while (endpoint) {
        if ( strcmp(endpoint->name, "AdminEndpoint")) {
            endpoint->enabled = 0;
            endpoint->needed = ENDPOINT_NEED_SOCKET;
            ism_transport_startTCPEndpoint(endpoint);
        }
        endpoint = endpoint->next;
    }
    pthread_mutex_unlock(&endpointlock);

    /* Close all connections */
    ism_transport_closeAllConnections(1);

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
    pthread_mutex_lock(&endpointlock);
    endpoint = endpoints;
    while (endpoint) {
        if ( !strcmp(endpoint->name, "AdminEndpoint")) {
            endpoint->enabled = 0;
            endpoint->needed = ENDPOINT_NEED_SOCKET;
            ism_transport_startTCPEndpoint(endpoint);
        }
        endpoint = endpoint->next;
    }
    pthread_mutex_unlock(&endpointlock);

    /*
     * Terminate TCP
     */
    usleep(10000);
    ism_transport_termTCP();
    usleep(10000);
    if (tObjPool)
        ism_common_destroyBufferPool(tObjPool);
    return 0;
}

/*
 * For testing only
 */
void ism_transport_destoryBufferPool(void) {
    if (tObjPool)
        ism_common_destroyBufferPool(tObjPool);
}


/*
 * Make a transport object.
 * Allocate a transport object.
 * @param endpoint  The endpoint object
 * @param tobjsize  Space allocated for transport specific object
 */
MALLOC ism_transport_t * ism_transport_newTransport(ism_endpoint_t * endpoint, int tobjSize, int fromPool) {
    int size;
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
//    size = sizeof(ism_transport_t) + extrasize + tobjsize + 8;
//    ism_transport_t * trans =ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 23),size, 1);
    ism_byteBuffer buff = (fromPool) ? ism_common_getBuffer(tObjPool,1) : ism_allocateByteBuffer(allocSize);
    ism_transport_t * transport = (ism_transport_t*) (buff->buf);
    memset(transport, 0, allocSize);
    transport->suballoc.size = allocSize - sizeof(ism_transport_t);
    transport->suballoc.pos  = 0;
    if(tobjSize)
        transport->tobj = (struct ism_transobj *)ism_transport_allocBytes(transport, tobjSize, 1);
    transport->state = ISM_TRANST_Opening;
    transport->domain = &ism_defaultDomain;
    transport->trclevel = ism_defaultTrace;
    transport->name = "";
    transport->clientID = "";
    transport->listener = endpoint;
    transport->endpoint_name = endpoint->name;
    transport->protocol = "unknown";             /* The protocol is not yet known */
    transport->protocol_family = "";
    transport->connect_time = ism_common_currentTimeNanos();
    pthread_spin_init(&transport->lock,0);
    transport->lastAccessTime = (uint64_t)ism_common_readTSC();
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

    size = sizeof(ism_transport_t);
    int allocSize = TOBJ_INIT_SIZE;
    if (size >= TOBJ_INIT_SIZE) {
        allocSize = (size+1024);
        fromPool = 0;
    }
    ism_byteBuffer buff = (fromPool) ? ism_common_getBuffer(tObjPool,1) : ism_allocateByteBuffer(allocSize);
    ism_transport_t * transport = (ism_transport_t*) (buff->buf);
    memset(transport, 0, allocSize);
    transport->suballoc.size = allocSize - sizeof(ism_transport_t);
    transport->suballoc.pos  = 0;
    transport->state = ISM_TRANST_Opening;
    transport->domain = &ism_defaultDomain;
    transport->trclevel = ism_defaultTrace;
    transport->name = "";
    transport->clientID = "";
    transport->listener = endpoint;
    transport->endpoint_name = endpoint->name;
    transport->protocol = "unknown";                       /* The protocol is not yet known */
    transport->protocol_family = "";
    transport->originated = 1;
    transport->connect_time = ism_common_currentTimeNanos();
    pthread_spin_init(&transport->lock,0);
    transport->lastAccessTime = (uint64_t)ism_common_readTSC();
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
        ism_common_free(ism_memory_transportBuffers,freesub);
    }
    buff--;

    ism_common_returnBuffer(buff, __FILE__, __LINE__);
}

/**
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

/**
 * Create a security profile.
 *
 * This method does not link in the secuirty profile, it only creates the object.
 * Once created, it should not be modified.
 *
 * @param name     The name of the profile
 * @param methods  The methods string
 * @param ciphertype  The enumeration of cipher type
 * @param ciphers  The string cipher list
 * @param certprof The certificate profile
 * @param ltpaprof The LTPA Profile name
 *
 */
XAPI ism_secprof_t * ism_transport_createSecProfile(const char * name, uint32_t method,
        uint32_t ciphertype, const char * ciphers, const char * certprof,  const char * ltpaprof, const char * oauthprof) {
    int  extralen;
    ism_secprof_t * ret;
    char * cp;

    /* Check the name */
    if (!ism_transport_validName(name)) {
        ism_common_setErrorData(ISMRC_BadConfigName,"%s",name);
        return NULL;
    }

    /*
     * Default ciphers if not specified
     *
     * NIST 800-131a prohibit the use of a number of some ciphers including the use of MD5 as an HMAC.
     * If TLSv1.2 is used as the protocol and best is selected, do not use ciphers with the SHA1 HMAC.
     */
    if (ciphers == NULL) {
        if (FIPSmode) {
            switch (ciphertype) {
            case CipherType_Best:    /* Best security */
                ciphers = "AESGCM:FIPS:!SRP:!PSK:!aNULL";
                break;
            default:
            case CipherType_Fast:    /* Optimize for speed with high security */
                ciphers = "AESGCM:AES128:FIPS:!SRP:!PSK:!aNULL";
                break;
            case CipherType_Medium:  /* Optimize for speed and allow non-authorizing ciphers */
                ciphers = "AES128-GCM-SHA256:AES128:FIPS:!SRP:!PSK";
                break;
            }
        } else {
            switch (ciphertype) {
            case CipherType_Best:    /* Best security */
                ciphers = "AESGCM:AES:!MD5:!SRP:!aNULL:!ADH:!AECDH";
                break;
            default:
            case CipherType_Fast:    /* Optimize for speed with high security */
                ciphers = "AES128-GCM-SHA256:AES128:AESGCM:HIGH:!MD5:!SRP:!aNULL:!3DES";
                break;
            case CipherType_Medium:  /* Optimize for speed and allow non-authorizing ciphers */
                ciphers = "AES128-GCM-SHA256:AES128:AESGCM:HIGH:MEDIUM:!MD5:!SRP";
                break;
            }
        }
    }

    /* Compute the length */
    extralen = strlen(name) + 4;
    if (ciphers && *ciphers!='\0')
        extralen += strlen(ciphers);
    if (certprof && *certprof!='\0')
        extralen += strlen(certprof);
    if (ltpaprof && *ltpaprof!='\0')
        extralen += strlen(ltpaprof);
     if (oauthprof && *oauthprof!='\0')
        extralen += strlen(oauthprof);

    /* Create the object */
    ret =ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportProfile, 23),1, sizeof(ism_secprof_t) + extralen);
    cp = (char *)(ret+1);
    ret->name = (const char *)cp;
    strcpy(cp, name);
    cp += strlen(cp)+1;
    if (ciphers && *ciphers!='\0') {
        ret->ciphers = (const char *)cp;
        strcpy(cp, ciphers);
        cp += strlen(cp)+1;
    }
    if (certprof && *certprof!='\0') {
        ret->certprof = (const char *)cp;
        strcpy(cp, certprof);
        cp += strlen(cp)+1;
    }
    if (ltpaprof && *ltpaprof!='\0') {
        ret->ltpaprof = (const char *)cp;
        strcpy(cp, ltpaprof);
        cp += strlen(cp)+1;
    }
    if (oauthprof && *oauthprof!='\0') {
        ret->oauthprof = (const char *)cp;
        strcpy(cp, oauthprof);
        cp += strlen(cp)+1;
    }

    ret->method = method;
    ret->ciphertype = ciphertype;
    ret->sslop = 0x010003FF;       /* Default sslop, disable SSLv2 and common bug fixes */
    return ret;
}


/**
 * Create a endpoint object
 */
ism_endpoint_t * ism_transport_createEndpoint(const char * name, const char * msghub, const char * transport_type,
        const char * ipaddr, const char * secprofile, const char * conpolicies,
        const char * topicpolicies, const char * qpolicies, const char * subpolicies, int mkstats) {
    int  extralen;
    ism_endpoint_t * ret;
    char * cp;
    int statlen = 0;

    /* Check the name */
    if (!ism_transport_validName(name)) {
        ism_common_setErrorData(ISMRC_BadConfigName,"%s",name);
        return NULL;
    }
    TRACE(6, "Transport create endpoint: name=%s msghub=%s transport_type=%s ipaddr=%s secprofile=%s conpolicies=%s "
            "topicolicies=%s qpolicies=%s subpolicies=%s\n",
            name, (msghub ? msghub : ""),(transport_type ? transport_type : ""),
            (ipaddr ? ipaddr : ""),(secprofile ? secprofile : ""), (conpolicies ? conpolicies :""),
            (topicpolicies ? topicpolicies :""), (qpolicies ? qpolicies :""), (subpolicies ? subpolicies :""));
    /* Compute the length */
    extralen = strlen(name) + 8;
    if (mkstats) {
        int iops = ism_tcp_getIOPcount() + 1;    /* Extra for admin */
        statlen = sizeof(ism_endstat_t);
        if (iops > MAX_STAT_THREADS)
            statlen += (iops-MAX_STAT_THREADS) * sizeof(msg_stat_t);
        extralen += statlen;

    }
    if (ipaddr)
        extralen += strlen(ipaddr);
    if (secprofile)
        extralen += strlen(secprofile);
    if (conpolicies)
        extralen += strlen(conpolicies);
    if (topicpolicies)
        extralen += strlen(topicpolicies);
    if (qpolicies)
        extralen += strlen(qpolicies);
    if (subpolicies)
        extralen += strlen(subpolicies);
    if (msghub)
        extralen += strlen(msghub);

    /* Create the object */
    ret =ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 24),1, sizeof(ism_endpoint_t) + extralen);
    cp = (char *)(ret+1);
    if (mkstats) {
        ret->stats = (ism_endstat_t *)cp;
        cp += statlen;
    }
    ret->name = (const char *)cp;
    strcpy(cp, name);
    cp += strlen(cp)+1;
    if (ipaddr) {
        ret->ipaddr = (const char *)cp;
        strcpy(cp, ipaddr);
        cp += strlen(cp)+1;
    }
    if (secprofile) {
        ret->secprof = (const char *)cp;
        strcpy(cp, secprofile);
        cp += strlen(cp)+1;
    }
    if (conpolicies) {
        ret->conpolicies = (const char *)cp;
        strcpy(cp, conpolicies);
        cp += strlen(cp)+1;
    }
    if (topicpolicies) {
        ret->topicpolicies = (const char *)cp;
        strcpy(cp, topicpolicies);
        cp += strlen(cp)+1;
    }
    if (qpolicies) {
        ret->qpolicies = (const char *)cp;
        strcpy(cp, qpolicies);
        cp += strlen(cp)+1;
    }
    if (subpolicies) {
        ret->subpolicies = (const char *)cp;
        strcpy(cp, subpolicies);
        cp += strlen(cp)+1;
    }
    if (msghub) {
        ret->msghub = (const char *)cp;
        strcpy(cp, msghub);
        cp += strlen(cp)+1;
    }
    if (transport_type == NULL)
        transport_type = "tcp";
    ism_common_strlcpy(ret->transport_type, transport_type, sizeof(ret->transport_type));
    ret->maxMsgSize = 4096*1024;
    ret->maxSendBufferSize = 128*1024;
    ret->maxRecvBufferSize = 8*1024*1024;
    return ret;
}


/**
 * Create a certificate profile.
 *
 * This method does not link in the security profile, it only creates the object.
 * Once created, it should not be modified.
 *
 * @param name     The name of the profile
 */
ism_certprof_t * ism_transport_createCertProfile(const char * name, const char * cert, const char * key) {
    int  extralen;
    ism_certprof_t * ret;
    char * cp;

    /* Check the name */
    if (!ism_transport_validName(name)) {
        ism_common_setErrorData(ISMRC_BadConfigName,"%s",name);
        return NULL;
    }

    /* Compute the length */
    extralen = strlen(name) + 16;
    if (cert)
        extralen += strlen(cert);
    if (key)
        extralen += strlen(key);

    /* Create the object */
    ret =ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportProfile, 25),1, sizeof(ism_certprof_t) + extralen);
    cp = (char *)(ret+1);
    ret->name = (const char *)cp;
    strcpy(cp, name);
    cp += strlen(cp)+1;
    if (cert) {
        ret->cert = (const char *)cp;
        strcpy(cp, cert);
        cp += strlen(cp)+1;
    }
    if (key) {
        ret->key = (const char *)cp;
        strcpy(cp, key);
        cp += strlen(cp)+1;
    }
    return ret;
}


/*
 * Register a transport
 */
int ism_transport_registerProtocol(ism_transport_onStartMessaging_t onStart, ism_transport_onConnection_t onConnection) {
    /* Put the new one at the start of the chain */
    protocol_chain * newprot = ism_common_malloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 6), sizeof(struct protocol_chain));
    newprot->next = protocols;
    newprot->onStart = onStart;
    newprot->onConnection = onConnection;
    protocols = newprot;
    return 0;
}


/*
 * Call registered protocols until we get one which works
 */
int ism_transport_findProtocol(ism_transport_t * transport) {
    protocol_chain * prot = protocols;
    while (prot) {
        int rc = prot->onConnection(transport);
        if (!rc)
            return 0;
        prot = prot->next;
    }
    return 1;
}


/*
 * Register a framer.
 * When a new connection is established, the transport calls all registered framers until it
 * finds one which accepts the handshake.
 * @param callback  The method to invoke when a new connection is found
 * @return A return code, 0=good
 */
int ism_transport_registerFramer(ism_transport_registerf_t callback) {
    /* Put the new one at the start of the chain */
    struct framer_chain * newframe = ism_common_malloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 3),sizeof(struct framer_chain));
    newframe->next = frames;
    newframe->regcall = callback;
    frames = newframe;
    return 0;
}


/*
 * Call registered frame checkers until we get one which accepts the connection.
 */
int ism_transport_findFramer(ism_transport_t * transport, char * buffer, int buflen, int * used) {
    struct framer_chain * frame = frames;
    while (frame) {
        int rc = frame->regcall(transport, buffer, buflen, used);
        if (rc != -1)
            return rc;
        frame = frame->next;
    }
    return -1;
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
            suba->next = ism_common_malloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 2),newlen);
            suba->next->next = NULL;
            suba->next->size = newlen-sizeof(struct suballoc_t);
            suba->next->pos  = 0;
        }
        suba = suba->next;
    }
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
 *
 */
typedef struct ism_delitem_t {
    struct ism_delitem_t * next;           /* Next delivery item in work or free chain */
    ism_transport_t *      transport;      /* The associated transport object          */
    ism_transport_onDelivery_t ondelivery; /* Callback for the work item               */
    void *                 userdata;       /* Userdata for the work item               */
} ism_delitem_t;

#define DELITEM_LIST_SIZE   2048
typedef struct ism_delitem_list_t {
    ism_delitem_t   list[DELITEM_LIST_SIZE];
    struct ism_delitem_list_t * next;
    int             inUse;
}ism_delitem_list_t;

struct ism_delivery_t {
    pthread_mutex_t lock;             /* Lock for all fields in this structure */
    pthread_cond_t  cond;             /* Condition variable for wait */
    struct ism_delitem_t * first;     /* First item in work list (read position) */
    struct ism_delitem_t * last;      /* Last item in work list (write position) */
    struct ism_delitem_t * free;      /* List of free entries                    */
    ism_delitem_list_t   * alloclist; /* List of all allocation blocks (used for free) */
    ism_threadh_t   delthread;        /* Thread handle */
    int             totalItemsAllocated;  /* Total allocated items                   */
    int             totalItemsInUse;      /* Total allocated items                   */
    char            state;
    char            resv[7];
};


/*
 * Worker thread for delivery.
 *
 * If there is no work we do a condition wait, but if there are work items which cannot be done,
 * we will go into a hot loop.
 *
 */
static void * transDelivery(void * data, void * context, int value) {
    ism_delivery_t * delivery = (ism_delivery_t *) data;

    /* Initialize engine thread local storage in the delivery thread */
    ism_engine_threadInit(1);

    pthread_mutex_lock(&delivery->lock);

    /* Loop to do work */
    while (delivery->state == ISM_TRANST_Open) {            /* BEAM suppression: infinite loop */
        ism_delitem_t * ditem = delivery->first;
        ism_delitem_t * freeListHead = NULL;
        ism_delitem_t * freeListTail = NULL;
        ism_delitem_t * reschedListHead = NULL;
        ism_delitem_t * reschedListTail = NULL;
        int reschedCount = 0;

        /* If nothing to do, wait for some work */
        ism_common_backHome();
        if (ditem == NULL) {
            pthread_cond_wait(&delivery->cond, &delivery->lock);
            continue;
        }
        ism_common_going2work();
        delivery->first = delivery->last = NULL;
        if (delivery->first == NULL)
            delivery->last = NULL;
        delivery->totalItemsInUse = 0;
        pthread_mutex_unlock(&delivery->lock);
        do {
            int rc = 0;
            ism_delitem_t * nextItem = ditem->next;
            rc = ditem->ondelivery(ditem->transport, ditem->userdata, 0);
            ditem->next = NULL;
            if ((rc == 0) ||
                (rc != 99 && ditem->transport->state == ISM_TRANST_Closed)) { /* Remove */
                if (ditem->transport)
                    __sync_sub_and_fetch(&ditem->transport->workCount, 1);
                if (freeListTail) {
                    freeListTail->next = ditem;
                } else {
                    freeListHead = ditem;
                }
                freeListTail = ditem;
            } else { /* Reschedule */
                reschedCount++;
                if (reschedListTail) {
                    reschedListTail->next = ditem;
                } else {
                    reschedListHead = ditem;
                }
                reschedListTail = ditem;
            }
            ditem = nextItem;
        } while (ditem != NULL);
        pthread_mutex_lock(&delivery->lock);
        /* Add rescheduled items back to the tasks list */
        if (reschedCount) {
            delivery->totalItemsInUse += reschedCount;
            if (delivery->last) {
                delivery->last->next = reschedListHead;
            } else {
                delivery->first = reschedListHead;
            }
            delivery->last = reschedListTail;
        }
        /* Add freed items back to the free list */
        if (freeListHead) {
            freeListTail->next = delivery->free;
            delivery->free = freeListHead;
        }
    }
    pthread_mutex_unlock(&delivery->lock);
    return NULL;
}


/**
 * Create a delivery object.
 *
 * This is created as a common service by transport.  A transport is not required to use this mechanism
 * and therefore it should never be directly called by the protocol.
 *
 * The delivery object represents a work queue which serially delivers work to a callback.  If there is
 * no work available it waits for work to be added.
 *
 * This is normally only called early in the start processing.  It should be considered a fatal error
 * if it fails.
 *
 * @param name  A name of 1 to 16 characters which is suitable for use as a thread name.
 * @return A delivery object or NULL to indicate an error.
 */
ism_delivery_t * ism_transport_createDelivery(const char * name) {
    char tname[18];
    ism_delivery_t * delivery =ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 26),1, sizeof(ism_delivery_t));
    ism_common_strlcpy(tname, name, 17);

    pthread_mutex_init(&delivery->lock, 0);
    pthread_cond_init(&delivery->cond, 0);

    delivery->state = ISM_TRANST_Open;
    delivery->alloclist = ism_common_malloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 1),sizeof(ism_delitem_list_t));
    delivery->alloclist->inUse = 0;
    delivery->alloclist->next = NULL;
    delivery->totalItemsAllocated = DELITEM_LIST_SIZE;

    ism_common_startThread(&delivery->delthread, transDelivery, delivery, NULL, 0,
                ISM_TUSAGE_NORMAL, 0, tname, "Transport delivery thread");
    return delivery;
}


/*
 * Add a work item to the delivery object.
 *
 * @param delivery   The delivery object
 * @param ondelivery The method to invoke for work
 * @param userdata   The data context for the work item
 * @return A return code: 0=good
 */
XAPI int  ism_transport_addDelivery(ism_delivery_t * delivery, ism_transport_t * transport,
        ism_transport_onDelivery_t ondelivery, void * userdata) {
    ism_delitem_t * ditem;
    if (transport)
        __sync_add_and_fetch(&transport->workCount, 1);
    pthread_mutex_lock(&delivery->lock);
    if (delivery->free) {
        ditem = delivery->free;
        delivery->free = ditem->next;
    } else {
        if (delivery->alloclist->inUse == DELITEM_LIST_SIZE) {
            ism_delitem_list_t * newList = ism_common_malloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 4),sizeof(ism_delitem_list_t));
            if (newList == NULL) {
                pthread_mutex_unlock(&delivery->lock);
                return ISMRC_AllocateError;
            }
            newList->next = delivery->alloclist;
            newList->inUse = 0;
            delivery->alloclist = newList;
            delivery->totalItemsAllocated += DELITEM_LIST_SIZE;
        }
        ditem = &(delivery->alloclist->list[delivery->alloclist->inUse++]);
    }
    ditem->next = NULL;
    ditem->transport = transport;
    ditem->ondelivery = ondelivery;
    ditem->userdata = userdata;
    if (delivery->last)
        delivery->last->next = ditem;
    else{
        delivery->first = ditem;
        pthread_cond_signal(&delivery->cond);
    }
    delivery->last = ditem;
    delivery->totalItemsInUse++;
    pthread_mutex_unlock(&delivery->lock);
    return 0;
}


/*
 * Get a transport pointer from an id
 */
ism_transport_t * ism_transport_getTransport(int id) {
    ism_transport_t * transport = NULL;
    if ((id > 0) && (id < monitor_alloc) && (IS_VALID_MON(id))) {
        transport = monitorlist[id];
    }
    return transport;
}


/*
 * Delayed add of monitor.
 *
 * Wait for the monitorlock to be available while doing whatever other work is available.
 *
 * Return 0 when the work is done, and 1 if it must be rescheduled.
 */
static int delayAddMonitor(ism_transport_t * transport, void * userdata, int flags) {
    if (pthread_mutex_trylock(&monitorlock) ) {
        return 99;    /* Reschedule even if closed */
    } else {
        int  rc = -1;
        if (transport->closestate[0] == 2 || transport->monitor_id) {
            rc = 0;
        } else {
            if (monitor_free_count > monitor_free_limit) {
                rc = monitor_free_head;
                monitor_free_head = (int)(((uintptr_t)monitorlist[rc]) >> 1);
                monitorlist[rc] = transport;
                monitor_free_count--;
            } else {
                if (monitor_used < monitor_alloc)
                    rc = monitor_used++;
            }
            if (rc > 0) {
                monitorlist[rc] = transport;
                transport->monitor_id = rc;
            }
        }
        pthread_mutex_unlock(&monitorlock);
        TRACEL(8, transport->trclevel, "Add transport to monitoring (delayed): transport=%d monitor=%d addr=%p\n", transport->index, rc, transport);
        return 0;
    }
}

/*
 * Add a transport to the array of monitored transport objects.
 *
 * @param transport   A pointer to a transport object to monitor
 * @return            An index of the transport object in the monitored
 *                    array
 */
int ism_transport_addMonitor(ism_transport_t * transport) {
    int  rc = -1;

    if (pthread_mutex_trylock(&monitorlock)) {
        transport->addwork(transport, delayAddMonitor, NULL);
        rc = 0;
    } else {
        if (monitor_free_count > monitor_free_limit) {
            rc = monitor_free_head;
            monitor_free_head = (int)(((uintptr_t)monitorlist[rc]) >> 1);
            monitorlist[rc] = transport;
            monitor_free_count--;
        } else {
            if (monitor_used < monitor_alloc)
                rc = monitor_used++;
        }
        if (rc > 0) {
            monitorlist[rc] = transport;
            transport->monitor_id = rc;
        }
        pthread_mutex_unlock(&monitorlock);
        TRACEL(8, transport->trclevel, "Add transport to monitoring: transport=%d monitor=%d addr=%p\n", transport->index, rc, transport);
    }
    return rc;
}

/*
 * Add the transport object to monitoring, waiting if required.
 */
int ism_transport_addMonitorNow(ism_transport_t * transport) {
    int  rc = -1;
    pthread_mutex_lock(&monitorlock);
    if (transport->monitor_id == 0) {
        if (monitor_free_count > monitor_free_limit) {
            rc = monitor_free_head;
            monitor_free_head = (int)(((uintptr_t)monitorlist[rc]) >> 1);
            monitorlist[rc] = transport;
            monitor_free_count--;
        } else {
            if (monitor_used < monitor_alloc)
                rc = monitor_used++;
        }
        if (rc > 0) {
            monitorlist[rc] = transport;
            transport->monitor_id = rc;
        }
    }
    pthread_mutex_unlock(&monitorlock);
    TRACEL(8, transport->trclevel, "Add transport to monitoring: transport=%d monitor=%d addr=%p\n", transport->index, rc, transport);
    return rc;
}


/*
 * Delayed remove of monitor.
 *
 * Wait for the monitorlock to be available while doing whatever other work is available.
 *
 * Return 0 when the work is done, and 1 if it must be rescheduled.
 */
static int delayRemoveMonitor(ism_transport_t * transport, void * userdata, int flags) {
    TRACEL(8, transport->trclevel, "Delayed monitor remove: id=%d index=%d equals=%d\n", transport->monitor_id, (int)transport->index,
            monitorlist[transport->monitor_id] == transport);
    if (pthread_mutex_trylock(&monitorlock) ) {
        return 99;   /* Reschedule even if closed */
    } else {
        if (transport->workCount <= 1) {
            if (transport->monitor_id > 0 && transport->monitor_id < monitor_used &&
                monitorlist[transport->monitor_id] == transport) {
                if (!monitor_free_head) {
                    monitor_free_head = monitor_free_tail = transport->monitor_id;
                    monitorlist[transport->monitor_id] = 0;
                    monitor_free_count = 1;
                } else {
                    monitorlist[monitor_free_tail] = (ism_transport_t *)(uintptr_t)((transport->monitor_id<<1) | 1);
                    monitorlist[transport->monitor_id] = 0;
                    monitor_free_tail = transport->monitor_id;
                    monitor_free_count++;
                }
            }
            transport->closestate[0] = 2;
            transport->monitor_id = 0;
        }
        pthread_mutex_unlock(&monitorlock);
        return 0;
    }
}

/*
 * Remove a transport from the list of monitored transport objects
 *
 * @param transport   A pointer to a transport object to stop monitoring
 * @param freeit      A flag indicating whether to free this transport or not
 * @return            1, if transport removed; 0 otherwise
 */
int ism_transport_removeMonitor(ism_transport_t * transport, int freeit) {
    int  rc = 1;
    TRACEL(8, transport->trclevel, "Remove transport index=%d monitor=%d addr=%p\n", transport->index, transport->monitor_id, transport);
    if (pthread_mutex_trylock(&monitorlock) ) {
        transport->addwork(transport, delayRemoveMonitor, NULL);
        rc = 0;
    } else {
        if (transport->workCount == 0) {
            if (transport->monitor_id > 0 && transport->monitor_id < monitor_used &&
                monitorlist[transport->monitor_id] == transport) {
                if (!monitor_free_head) {
                    monitor_free_head = monitor_free_tail = transport->monitor_id;
                    monitorlist[transport->monitor_id] = 0;
                    monitor_free_count = 1;
                } else {
                    monitorlist[monitor_free_tail] = (ism_transport_t *)(uintptr_t)((transport->monitor_id<<1) | 1);
                    monitorlist[transport->monitor_id] = 0;
                    monitor_free_tail = transport->monitor_id;
                    monitor_free_count++;
                }
                rc = 1;
            }
            transport->closestate[0] = 2;
            transport->monitor_id = 0;
        }
        pthread_mutex_unlock(&monitorlock);
    }
    return rc;
}

/*
 * Remove a transport from the list of monitored transport objects without delay
 *
 * The normal form of this method will schedule work if it cannot get the monitor lock.
 * This form is used when delaying the removal is not acceptable.
 *
 * @param transport   A pointer to a transport object to stop monitoring
 */
void ism_transport_removeMonitorNow(ism_transport_t * transport) {
    TRACEL(8, transport->trclevel, "Remove transport index=%d monitor=%d addr=%p\n", transport->index, transport->monitor_id, transport);
    pthread_mutex_lock(&monitorlock);
    if (transport->monitor_id > 0 && transport->monitor_id < monitor_used &&
        monitorlist[transport->monitor_id] == transport) {
        if (!monitor_free_head) {
            monitor_free_head = monitor_free_tail = transport->monitor_id;
            monitorlist[transport->monitor_id] = 0;
            monitor_free_count = 1;
        } else {
            monitorlist[monitor_free_tail] = (ism_transport_t *)(uintptr_t)((transport->monitor_id<<1) | 1);
            monitorlist[transport->monitor_id] = 0;
            monitor_free_tail = transport->monitor_id;
            monitor_free_count++;
        }
    }
    transport->closestate[0] = 2;
    transport->monitor_id = 0;
    pthread_mutex_unlock(&monitorlock);
}

/*
 * Do the actual close on the delivery thread
 */
static int deliverClose(ism_transport_t * transport, void * userdata, int flags) {
    int  rc = 0;
    char xbuf[256];
    if (userdata != NULL && *(char *)userdata) {
        rc = atoi(userdata);
    }
    if (rc == 0)
        rc = ISMRC_EndpointDisabled;
    ism_common_getErrorString(rc, xbuf, sizeof xbuf);
    transport->close(transport, ISMRC_EndpointDisabled, 0, xbuf);
    return 0;
}


/**
 * Force the disconnect of connections for specified endpoints.
 *
 * @param rc        Specify the return code to use for the disconnect.  This is put into the trace.
 *
 * @param reason    A reason string which can be NULL.  This is put in the log if it is present
 *                  as the reason for disconnect.  The default is "Forced disconnect".
 * @param endpoint  The endpoint name which can contain asterisks as wildcard characters.
 *                  Use the value "*" to disconnect all connections.
 * @param
 *
 */
int ism_transport_disconnectEndpoint(int rc, const char * reason, const char * endpoint) {
    ism_transport_t * transport;
    int  i;
    int  count = 0;

    if (rc == 0)
        rc = -1;
    if (!reason)
        reason = "Force disconnect";
    if (!endpoint || !*endpoint)
        return -1;

    pthread_mutex_lock(&monitorlock);
    for (i=1; i<monitor_used; i++) {
        if (IS_VALID_MON(i)) {
            transport = monitorlist[i];
            if (transport->name && *transport->name && transport->listener && *transport->listener->name > '!') {
                if ((rc < -1) ? !strcmp(transport->listener->name, endpoint) : ism_common_match(transport->listener->name, endpoint)) {
                    if (rc == 99)
                        printf("disconnect %s\n", transport->name);
                    TRACEL(8, transport->trclevel, "Force disconnect: client=%s rc=%d reason=%s\n", transport->name, rc, reason);
                    transport->addwork(transport, deliverClose, (void *)reason);
                    count++;
                }
            } else {
            //  printf("not disconnected %s\n", transport->name);
            }
        }
    }
    pthread_mutex_unlock(&monitorlock);
    return count;
}

/*
 * Select connection
 */
inline static int selectConnection(ism_transport_t * transport, const char * clientID, const char * userID, const char * client_addr, const char * endpoint) {
    if (transport->name[0] == '_' && transport->name[1] == '_')
        return 0;
    if (clientID) {
        if (!ism_common_match(transport->name, clientID)) {
            return 0;
        }
    }
    if (userID) {
        if (transport->userid == NULL) {
            if (*userID)
                return 0;
        } else {
            if (!ism_common_match(transport->userid, userID))
                return 0;
        }
    }
    if (client_addr && transport->client_addr) {
        if (!ism_common_match(transport->client_addr, client_addr))
            return 0;
    }
    if (endpoint) {
        if (!ism_common_match(transport->endpoint_name, endpoint))
            return 0;
    }
    return 1;
}

/*
 * For Unit test
 */
int ism_testSelect(ism_transport_t * transport, const char * clientid, const char * userid, const char * client_addr, const char * endpoint) {
    return selectConnection(transport, clientid, userid, client_addr, endpoint);
}

/*
 * Force a disconnection.
 *
 * Any parameter which is null does not participate in the match.
 * Note that this method will return as soon as we have scheduled a close of the
 * connection, which is probably before it is done.
 *
 * @param clientID  The clientID which can contain asterisks as wildcard characters.
 * @param userID    The userID which can contain asterisks as wildcard characters.
 * @param client_addr  The client address and an IP address which can contain asterisks.
 *                  If this is an IPv6 address it will be surrounded by brackets.
 * @param endpoint  The endpoint name which can contain asterisks as wildcard characters.
 * @return  The count disconnected
 *
 */
int ism_transport_closeConnection(const char * clientID, const char * userID, const char * client_addr, const char * endpoint) {
    ism_transport_t * transport;
    int  i;
    int  count = 0;

    if (!clientID && !userID && !client_addr && !endpoint)
        return 0;
    pthread_mutex_lock(&monitorlock);
    for (i=1; i<monitor_used; i++) {
        if (IS_VALID_MON(i)) {
            transport = monitorlist[i];
            /* Ignore handshake, internal, and admin connections   */
            if (transport->adminCloseConn == 0 && transport->name && *transport->name && transport->endpoint_name && *transport->endpoint_name != '!') {
                if (selectConnection(transport, clientID, userID, client_addr, endpoint)) {
                     TRACEL(6, transport->trclevel, "Force disconnect: client=%s From=%s:%u user=%s endpoint=%s\n",
                            transport->name, transport->client_addr, transport->clientport, transport->userid, transport->endpoint_name);
                    if (transport->addwork)
                        transport->addwork(transport, deliverClose, (void *)"");
                    transport->adminCloseConn = 1;
                    count++;
                }
            }
        }
    }
    pthread_mutex_unlock(&monitorlock);
    return count;
}



/*
 * Monitor list header.  This is put just before the list returned to the user
 */
typedef struct mon_list_header_t {
    void *    more;
    uint32_t  count;
    uint32_t  size;
} mon_list_header_t;

/**
 * Get a list of endpoint monitoring objects.
 *
 * Return a monitoring object for each of the endpoints which match the selection.
 * If a pattern is specified, only those which match this name will be returned.
 * The pattern is a string with an asterisk to match zero or more characters.
 *
 * @param monlis   The output with a list of endpoint monitoring objects.  This must be freed using
 *                 ism_transport_freeEndpointMonitor.
 * @param names    A pattern to match against the endpoint names or NULL to match all.
 * @return         The count of endpoint monitoring objects returned
 */
XAPI int ism_transport_getEndpointMonitor(ism_endpoint_mon_t * * monlis, const char * names) {
    mon_list_header_t * ret = NULL;
    ism_endpoint_t * * list;
    ism_endpoint_mon_t * endpmon;
    ism_endpoint_t * endpoint;
    int   count = 0;
    int   i;
    int   j;
    int   internal;
    uint64_t read_msg;
    uint64_t read_bytes;
    uint64_t write_msg;
    uint64_t write_bytes;
    uint64_t lost_msg;
    uint64_t warn_msg;

    *monlis = NULL;

    internal = names && names[0]=='*' && names[1]=='*';
    pthread_mutex_lock(&endpointlock);
    list = alloca(endpoint_count * sizeof(ism_endpoint_t));

    /*
     * Select the endpoints to monitor
     */
    endpoint = endpoints;
    while (endpoint) {
        if ((!endpoint->isAdmin) || internal) {
            if (!names || ism_common_match(endpoint->name, names)) {
                list[count++] = endpoint;
            }
        }
        endpoint = endpoint->next;
    }

    /*
     * Allocate the memory and fill in the structure
     */
    if (count) {
        int size = sizeof(*ret) + count * sizeof(ism_endpoint_mon_t);
        ret = ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 19),1, size);
        if (ret == NULL) {
            pthread_mutex_unlock(&endpointlock);
            return 0;
        }
        ret->count = count;
        ret->size  = size;
        endpmon = (ism_endpoint_mon_t *)(ret+1);
        for (i=0; i<count; i++) {
            endpoint = list[i];
            read_msg = 0;
            read_bytes = 0;
            write_msg = 0;
            write_bytes = 0;
            lost_msg = 0;
            warn_msg = 0;
            for (j=0; j<endpoint->thread_count; j++) {
                read_msg    += endpoint->stats->count[j].read_msg;
                read_bytes  += endpoint->stats->count[j].read_bytes;
                write_msg   += endpoint->stats->count[j].write_msg;
                write_bytes += endpoint->stats->count[j].write_bytes;
                lost_msg    += endpoint->stats->count[j].lost_msg;
                warn_msg    += endpoint->stats->count[j].warn_msg;
            }
            endpmon->name              = endpoint->name;            /* Endpoint name                    */
            endpmon->ipaddr            = endpoint->ipaddr;          /* IP address or null for any       */
            endpmon->enabled           = endpoint->enabled;         /* Is enabled                       */
            endpmon->secure            = endpoint->secure;          /* Security type                    */
            endpmon->port              = endpoint->port;            /* Port                             */
            endpmon->errcode           = endpoint->rc;              /* Last error code                  */
            endpmon->config_time       = endpoint->config_time;     /* Time of configuration            */
            endpmon->reset_time        = endpoint->reset_time;      /* Time of statistics reset         */
            endpmon->connect_active    = endpoint->stats->connect_active;  /* Currently active connections     */
            endpmon->connect_count     = endpoint->stats->connect_count;   /* Connection count since reset     */
            endpmon->bad_connect_count = endpoint->stats->bad_connect_count; /* Count of connections which have failed to connect since reset */
            endpmon->read_msg_count    = read_msg;                  /* Read message count since reset   */
            endpmon->read_bytes_count  = read_bytes;                /* Read bytes count since reset     */
            endpmon->write_msg_count   = write_msg;                 /* Write message count since reset  */
            endpmon->write_bytes_count = write_bytes;               /* Write bytes count since reset    */
            endpmon->lost_msg_count    = lost_msg;                  /* Lost message count since reset   */
            endpmon->warn_msg_count    = warn_msg;                  /* Partially successful message publish count since reset   */
            endpmon++;
        }
        *monlis = (ism_endpoint_mon_t *)(ret+1);
    }
    pthread_mutex_unlock(&endpointlock);
    return count;
}


/**
 * Free a list of endpoint monitoring objects
 */
XAPI void ism_transport_freeEndpointMonitor(ism_endpoint_mon_t * monlis) {
    mon_list_header_t * hdr;
    if (monlis) {
        hdr = ((mon_list_header_t *)monlis)-1;
        ism_common_free(ism_memory_transportBuffers,hdr);
    }
}

/*
 * Get a list of connection monitoring objects.
 *
 * Return a monitoring object for each of the connections which match the selections.
 * If a pattern is specified, only those which match this name will be returned.
 * The pattern is a string with an asterisk to match zero or more characters.  If NULL
 * is specified for a selector, all connections match.  If multiple selector strings are
 * specified, the connection must match all of them.
 *
 * Since the number of connections can be very large, you can specify a maximum count
 * to return.  If the count of connections returned is equal to the maxcount, the function can
 * be reinvoked with the updated position to get additional monitoring objects.
 *
 * @param moncon   The output with a list of connection monitoring objects.  This must be freed using
 *                 ism_transport_freeConnectionMonitor.
 * @param maxcount The maximum number of connections to return.
 * @param position This is an input and output parameter which indicates where we are in the set of connection.
 *                 On first call this should be set to zero.  On subsequent calls the updated value should
 *                 be used.
 * @param options  Options.  Set to zero if no options are known.
 * @param names    A pattern to match against the connection names or NULL to match all.  The name of the
 *                 connection is normally the clientID.
 * @param procools A pattern to match against the protocol family name, or NULL to match all.
 * @param endpoints A pattern to match against the endpoint name, or NULL to match all.
 * @param userids   A pattern to match against the username, or NULL to match all.
 * @param clientips A pattern to match against the client IP address.  If the client is connected using IPv6, this
 *                 address is surrounded by brackets.  Thus "[*" will match all IPv6 client addresses.
 * @return         The count of connection monitoring objects returned
 */
XAPI int ism_transport_getConnectionMonitor(ism_connect_mon_t * * moncon, int maxcount, int * position, int options,
        const char * names, const char * protFilter, const char * endpFilter, const char * userids, const char * clientip,
        uint16_t minPort, uint16_t maxPort, int connectionDirection) {
    mon_list_header_t * ret = NULL;
    int  i;
    int  pos;
    int  count;
    int  extra = 256;       /* Give some extra for cases of differentces between the two passes */
    int  freeit = 0;
    char * data;
    ism_transport_t * * con;
    int   internal;

    internal = endpFilter && endpFilter[0]=='!';

    *moncon = NULL;
    if (maxcount > 2000) {
        con =ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 20),maxcount, sizeof(ism_transport_t *));
        freeit = 1;
    } else {
        con = alloca(maxcount * sizeof(ism_transport_t *));
    }

    pos = *position;
    pthread_mutex_lock(&monitorlock);

    /*
     * Find transport objects to monitor
     */
    for (count = 0; (count < maxcount) && (pos < monitor_used); pos++) {
        if (IS_VALID_MON(pos)) {
            ism_transport_t * transport = monitorlist[pos];
            /*
             * Make sure we have finalized the transport name and userid.
             * The protocol should not set both name and ClientID until userid
             * is set.
             */
            if (!*transport->name || !transport->clientID)
                continue;
            if (names && !ism_common_match(transport->name, names))
                continue;
            if (protFilter && !ism_common_match(transport->protocol_family, protFilter))
                continue;
            if (endpFilter) {
                if (!internal && transport->listener->name[0]=='!' && transport->listener->name[1]!='M' )
                    continue;
                if (!ism_common_match(transport->listener->name, endpFilter))
                    continue;
            } else {
                if (transport->listener->name[0]=='!' && transport->listener->name[1]!='M' )
                    continue;
            }
            if (userids && !ism_common_match(transport->userid, userids))
                continue;
            if (clientip && !ism_common_match(transport->client_addr, clientip))
                continue;
            if ((transport->clientport < minPort) || (transport->clientport > maxPort))
                continue;
            if (transport->originated) {
                if ((connectionDirection & ISM_OUTGOING_CONNECTION) == 0)
                    continue;
            } else {
                if((connectionDirection & ISM_INCOMING_CONNECTION) == 0)
                    continue;
            }
            con[count++] = transport;
            extra += 3;
            if (transport->name)
                extra += strlen(transport->name);
            if (transport->userid)
                extra += strlen(transport->userid);
            if (transport->client_addr)
                extra += strlen(transport->client_addr);
        }
    }

    /*
     * Allocate the memory and fill in the monitoring structure
     */
    retry:
    if (count) {
        ism_time_t now = ism_common_currentTimeNanos();
        ism_connect_mon_t * tmon;
        int size = sizeof(*ret) + extra + count * sizeof(ism_connect_mon_t);
        int left = extra;
        ret =ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 27),1, size);
        if (ret != NULL) {
            ret->count = count;
            ret->size  = size;
            tmon = (ism_connect_mon_t *)(ret+1);
            data = (char *)(tmon+count);
        } else {
            count = 0;
        }
        for (i=0; i<count; i++) {
            ism_transport_t * transport = con[i];
            int  ilen;
            if (transport->name) {
                tmon->name      = data;             /* Connection name.  This is used for trace.  The clientID is commonly used    */
                ilen = strlen(transport->name);
                if (ilen >= left) {
                    ism_common_free(ism_memory_transportBuffers,ret);
                    extra += 1024;
                    goto retry;
                }
                memcpy(data, transport->name, ilen+1);
                data += ilen+1;
                left -= ilen+1;
            } else {
                tmon->name = "";
            }
            if (transport->client_addr) {
                tmon->client_addr  = data;             /* Client address */
                ilen = strlen(transport->client_addr);
                if (ilen >= left) {
                    ism_common_free(ism_memory_transportBuffers,ret);
                    extra += 1024;
                    goto retry;
                }
                memcpy(data, transport->client_addr, ilen+1);
                data += ilen+1;
                left -= ilen+1;
            } else {
                tmon->client_addr = "";
            }
            if (transport->userid) {
                tmon->userid     = data;             /* Primary userid                                 */
                ilen = strlen(transport->userid);
                if (ilen >= left) {
                    ism_common_free(ism_memory_transportBuffers,ret);
                    extra += 1024;
                    goto retry;
                }
                memcpy(data, transport->userid, ilen+1);
                data += ilen+1;
                left -= ilen+1;
            } else {
                tmon->userid = "";
            }
            tmon->protocol     = transport->protocol_family;    /* The name of the protocol family                */
            tmon->listener     = transport->listener->name;     /* The endpoint object                            */
            tmon->port         = transport->serverport;         /* The server port                                */
            tmon->index        = transport->index;              /* The index used in trace                        */
            tmon->connect_time = transport->connect_time;       /* Connection time                                */
            tmon->reset_time   = transport->reset_time;         /* The time of the last statistics reset          */
            tmon->read_bytes   = transport->read_bytes;         /* The bytes read since reset                     */
            tmon->read_msg     = transport->read_msg;           /* The messages read since reset                  */
            tmon->write_bytes  = transport->write_bytes;        /* The bytes written since reset                  */
            tmon->write_msg    = transport->write_msg;          /* The messages written since reset               */
            tmon->lost_msg     = transport->lost_msg;
            tmon->warn_msg     = transport->warn_msg;
            tmon->sendQueueSize = transport->sendQueueSize;     /* The current size of the send queue             */
            tmon->isSuspended  = transport->suspended;        /* Is connection suspended                        */
            tmon->duration     = now - transport->connect_time; /* The connection duration in nanoseconds         */
            ism_transport_getSocketInfo(transport, &tmon->socketInfoTcp);
            tmon++;
        }
        *moncon = (ism_connect_mon_t *)(ret+1);
    }
    pthread_mutex_unlock(&monitorlock);

    *position = pos;
    if (freeit) {
        ism_common_free(ism_memory_transportBuffers,con);
    }
    return count;
}


/*
 * Free a list of connection
 */
XAPI void ism_transport_freeConnectionMonitor(ism_connect_mon_t * moncon) {
    mon_list_header_t * hdr;
    if (moncon) {
        hdr = ((mon_list_header_t *)moncon)-1;
        ism_common_free(ism_memory_transportBuffers,hdr);
    }
}


/**
 * Get number of active connections
 */
XAPI int ism_transport_getNumActiveConns(void) {
    int n;
    ism_endpoint_t * endpoint;
    pthread_mutex_lock(&endpointlock);
    for ( n=0, endpoint=endpoints; endpoint ; endpoint = endpoint->next ) if ( endpoint->stats ) n += endpoint->stats->connect_active;  /* Currently active connections     */
    pthread_mutex_unlock(&endpointlock);
    return n;
}

/*
 * Get a list of transport objects.
 * * DEPRECATED *
 * This is an internal call, so we give the actual transport pointers to the monitor.
 * However, we do prevent the transport objects from being deleted while monitoring
 * is in progress.
 *
 * @param translist   A pointer to an array of transport objects to fill in
 * @param count       A number of entries in the translist array
 * @param start       An index of a first monitor to get
 * @return            Total number of monitors returned
 */
int ism_transport_getMonitor(ism_transport_t * * translist, int count, int start) {
    return 0;
}


/*
 * Free a list of transport objects.
 * * DEPRECATED *
 * The count for free should be the lesser of the original count and the
 * return count.
 *
 * @param translist   A pointer to an array of transport objects to free
 * @param count       A number of entries in the translist array
 */
void ism_transport_freeMonitor(ism_transport_t * * translist, int count) {
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
    uint64_t warn_msg = 0;
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
        warn_msg += endpoint->stats->count[i].warn_msg;
    }
    ism_common_ltoa_ts(read_msg, rmsgcnt, ism_common_getNumericSeparator());
    ism_common_ltoa_ts(read_bytes, rbytecnt, ism_common_getNumericSeparator());
    ism_common_ltoa_ts(write_msg, wmsgcnt, ism_common_getNumericSeparator());
    ism_common_ltoa_ts(write_bytes, wbytecnt, ism_common_getNumericSeparator());

    TRACE(level,
          "Endpoint %s name=%s enabled=%u rc=%d ipaddr=%s port=%u transport=%s addr=%p need=%d\n"
          "    hub=%s secure=%u secprof=%s conpolicies=%s topicpolicies=%s qpolicies=%s subpolicies=%s\n"
          "    protomask=%lx transmask=%x sock=%p maxsize=%u active=%llu count=%llu failed=%llu\n"
          "    read_msg=%s read_bytes=%s write_msg=%s write_msg=%s lost_msg=%llu warn_msg=%llu\n",
            where, endpoint->name, endpoint->enabled, endpoint->rc, endpoint->ipaddr ? endpoint->ipaddr : "(null)",
            endpoint->port, endpoint->transport_type, endpoint, endpoint->needed,
            endpoint->msghub ? endpoint->msghub : "", endpoint->secure, endpoint->secprof ? endpoint->secprof : "",
            endpoint->conpolicies ? endpoint->conpolicies : "", endpoint->topicpolicies ? endpoint->topicpolicies : "",
            endpoint->qpolicies ? endpoint->qpolicies : "", endpoint->subpolicies ? endpoint->subpolicies : "",
            endpoint->protomask, endpoint->transmask,
            (void *)(uintptr_t)endpoint->sock, endpoint->maxMsgSize, (ULL)endpoint->stats->connect_active, (ULL)endpoint->stats->connect_count,
            (ULL)endpoint->stats->bad_connect_count, rmsgcnt, rbytecnt, wmsgcnt, wbytecnt, (ULL)lost_msg, (ULL)warn_msg);
}

/*
 * Print the endpoints
 */
void ism_transport_printEndpoints(const char * pattern) {
    int  i;
    char rmsgcnt [32];
    char rbytecnt [32];
    char wmsgcnt [32];
    char wbytecnt [32];
    ism_endpoint_t * endpoint;


    pthread_mutex_lock(&endpointlock);
    endpoint = endpoints;
    while (endpoint) {
        if (ism_common_match(endpoint->name, pattern)) {
            uint64_t read_msg = 0;
            uint64_t read_bytes = 0;
            uint64_t write_msg = 0;
            uint64_t write_bytes = 0;
            uint64_t lost_msg = 0;
            uint64_t warn_msg = 0;
            for (i=0; i<endpoint->thread_count; i++) {
                read_msg += endpoint->stats->count[i].read_msg;
                read_bytes += endpoint->stats->count[i].read_bytes;
                write_msg += endpoint->stats->count[i].write_msg;
                write_bytes += endpoint->stats->count[i].write_bytes;
                lost_msg += endpoint->stats->count[i].lost_msg;
                warn_msg += endpoint->stats->count[i].warn_msg;
            }
            ism_common_ltoa_ts(read_msg, rmsgcnt, ism_common_getNumericSeparator());
            ism_common_ltoa_ts(read_bytes, rbytecnt, ism_common_getNumericSeparator());
            ism_common_ltoa_ts(write_msg, wmsgcnt, ism_common_getNumericSeparator());
            ism_common_ltoa_ts(write_bytes, wbytecnt, ism_common_getNumericSeparator());
            printf("Endpoint name=%s enabled=%u rc=%d ipaddr=%s port=%u transport=%s\n"
                   "    hub=%s secure=%u secprof=%s clientcert=%u usepassword=%u about=%u\n"
                   "    conpolicies=%s topicpolicies=%s qpolicies=%s subpolicies=%s\n"
                   "    protomask=%lx transmask=%x maxmsgsize=%u maxsendsize=%u doNotBatch=%s\n"
                   "    active=%llu count=%llu bad=%llu\n"
                   "    read_msg=%s read_bytes=%s write_msg=%s write_bytes=%s lost_msg=%llu warn_msg=%llu\n",
                    endpoint->name, endpoint->enabled,  endpoint->rc, endpoint->ipaddr ? endpoint->ipaddr : "*",
                    endpoint->port, endpoint->transport_type, endpoint->msghub ? endpoint->msghub : "",
                    endpoint->secure, endpoint->secprof ? endpoint->secprof : "",
                    endpoint->useClientCert, endpoint->usePasswordAuth, endpoint->enableAbout,
                    endpoint->conpolicies ? endpoint->conpolicies : "",
                    endpoint->topicpolicies ? endpoint->topicpolicies : "",
                    endpoint->qpolicies ? endpoint->qpolicies : "",
                    endpoint->subpolicies ? endpoint->subpolicies : "",
                    endpoint->protomask, endpoint->transmask, endpoint->maxMsgSize,
                    endpoint->maxSendSize, ((endpoint->doNotBatch) ? "true" : "false"),
                    (ULL)endpoint->stats->connect_active, (ULL)endpoint->stats->connect_count,
                    (ULL)endpoint->stats->bad_connect_count, rmsgcnt, rbytecnt, wmsgcnt, wbytecnt,
                    (ULL)lost_msg, (ULL)warn_msg);
        }
        endpoint = endpoint->next;
    }
    pthread_mutex_unlock(&endpointlock);
}


/*
 * Trace a security profile
 */
void ism_transport_dumpSecProfile(int level, ism_secprof_t * secprof, const char * where, int full) {
    if (!where)
        where = "";
    TRACE(level,
          "SecProfile %s name=%s method=%s sslop=%08x certprof=%s addr=%p\n"
          "    ciphertype=%s ciphers=\"%s\" clientcert=%d clientcipher=%d\n",
          where, secprof->name, ism_common_enumName(enum_methods, secprof->method), secprof->sslop,
          secprof->certprof, secprof,
          ism_common_enumName(enum_ciphers, secprof->ciphertype), secprof->ciphers,
          secprof->clientcert, secprof->clientcipher);
}

/*
 * Print the security profiles
 */
void ism_transport_printSecProfile(const char * pattern) {
    ism_secprof_t * secprof;

    if (!pattern)
        pattern = "*";
    pthread_mutex_lock(&endpointlock);

    secprof = secprofiles;
    while (secprof) {
        /* Ignore thread problems for now */
        if (ism_common_match(secprof->name, pattern)) {
            printf(
                "SecProfile name=%s method=%s sslop=%08x certprof=%s ciphertype=%s\n"
                "    ciphers=\"%s\"\n"
                "    clientcert=%d clientcipher=%d usepassword=%d tlsenable=%u\n",
                secprof->name, ism_common_enumName(enum_methods, secprof->method), secprof->sslop,
                secprof->certprof, ism_common_enumName(enum_ciphers, secprof->ciphertype), secprof->ciphers,
                secprof->clientcert, secprof->clientcipher, secprof->passwordauth, secprof->tlsenabled);
        }
        secprof = secprof->next;
    }
    pthread_mutex_unlock(&endpointlock);
}



/*
 * Trace a certificate profile
 */
void ism_transport_dumpCertProfile(int level, ism_certprof_t * certprof, const char * where, int full) {
    if (!where)
        where = "";
    TRACE(level, "CertProfile %s name=%s cert=%s key=%s addr=%p\n",
          where, certprof->name, certprof->cert, certprof->key, certprof);
}

/*
 * Print the security profiles
 */
void ism_transport_printCertProfile(const char * pattern) {
    ism_certprof_t * certprof;

    if (!pattern)
        pattern = "*";
    pthread_mutex_lock(&endpointlock);

    certprof = certprofiles;
    while (certprof) {
        /* Ignore thread problems for now */
        if (ism_common_match(certprof->name, pattern)) {
            printf("CertProfile name=%s cert=%s key=%s\n",
                 certprof->name, certprof->cert, certprof->key);
        }
        certprof = certprof->next;
    }
    pthread_mutex_unlock(&endpointlock);
}

/*
 * Trace a message hub
 */
void ism_transport_dumpMsgHub(int level, ism_msghub_t * msghub, const char * where, int full) {
    if (!where)
        where = "";
    TRACE(level, "MessageHub %s name=\"%s\" description=\"%s\"\n", where, msghub->name, msghub->descr);
}

/*
 * Print the security profiles
 */
void ism_transport_printMsgHub(const char * pattern) {
    ism_msghub_t * msghub;

    if (!pattern)
        pattern = "*";
    pthread_mutex_lock(&endpointlock);

    msghub = msghubs;
    while (msghub) {
        /* Ignore thread problems for now */
        if (ism_common_match(msghub->name, pattern)) {
            printf("MessageHub name=%s description=\"%s\"\n",
                 msghub->name, msghub->descr);
        }
        msghub = msghub->next;
    }
    pthread_mutex_unlock(&endpointlock);
}


/*
 * Get a size with K and M suffix
 */
static int getUnitSize(const char * ssize) {
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
/*
 * List of disabled client sets.
 * We assume that this is normally quite small
 */
typedef struct disableClient_t {
    struct disableClient_t * next;
    const char * regex_str;
    ism_regex_t  regex;
    int          count;
    int          rc;
} disableClient_t;

static disableClient_t * disableClients = NULL;


/*
 * Close connections based on a regex match to the clientID
 * 
 * @return The number of connections that are not closed (clients that have not yet 
 * checked their clientId against the regex are excluded as they will close themselves when
 * they perform the check in  ism_transport_clientAllowed() )
 */
static int ism_transport_closeClientSet(ism_regex_t regex) {
    int  i;
    ism_transport_t * transport;
    int  count = 0;

    pthread_mutex_lock(&monitorlock);
    for (i=1; i<monitor_used; i++) {
        if (IS_VALID_MON(i)) {
            transport = monitorlist[i];
            /* Ignore handshake, internal, and admin connections   */
            if (transport->adminCloseConn == 0 && transport->name && *transport->name && transport->endpoint_name &&
                    *transport->endpoint_name != '!' &&  (strcmp(transport->protocol, "mux") != 0)) {
                if (ism_regex_match(regex, transport->name) == 0) {

                    if (transport->enabled_checked)
                    {
                        TRACEL(6, transport->trclevel, "Force connection close: client=%s From=%s:%u user=%s endpoint=%s\n",
                                transport->name, transport->client_addr, transport->clientport, transport->userid, transport->endpoint_name);
                        if (transport->addwork)
                            transport->addwork(transport, deliverClose, (void *)"");
                        transport->adminCloseConn = 1;
                        count++;
                    }
                    else
                    {
                        TRACEL(7, transport->trclevel, "Skipping close of new connection: client=%s From=%s:%u user=%s endpoint=%s\n",
                                transport->name, transport->client_addr, transport->clientport, transport->userid, transport->endpoint_name);
                    }
                    
                }
            } else {
                /* not fully closed and passed the enabled initial check*/
                if (transport->adminCloseConn && !transport->enabled_checked)
                    count++;
            }
        }
    }
    pthread_mutex_unlock(&monitorlock);
    return count;
}

/*
 * Disable the ability to connect from a set of clientIDs.
 *
 * If there are currently any connections from a matching clientID, the connection is closed.
 * Connections from a metching clientID is not allowed until the same string is used for an
 * ism_transport_enableClientSet, or the server restarts.  If it is necessary to disable
 * connection at server start, this method must be called before messaging is started.
 *
 * If a client set is disabled multiple times, it must be enabled the same number of times.
 *
 * @param regex_str     A regular expression to match the clientIDs to be disabled
 * @param disallowedrc  The return code to set when a connection is attempted
 * @return A return code. 0=good
 */
int  ism_transport_disableClientSet(const char * regex_str, int disallowedrc) {
    int  xrc;
    char xbuf [256];
    disableClient_t * dp;
    disableClient_t * disable;
    char * rgStr;
    int doDisconnect = 0;
    int rc = 0;

    if (regex_str == NULL) {
        TRACE(4, "Disable client with a null client set.\n");
        return ISMRC_ClientSetNotValid;
    }
    disable =ism_common_calloc(ISM_MEM_PROBE(ism_memory_transportBuffers, 21),1, sizeof(disableClient_t)+strlen(regex_str)+1);
    disable->regex_str = rgStr = (char*) (disable+1);
    strcpy(rgStr,regex_str);
    xrc = ism_regex_compile(&disable->regex, regex_str);
    if (xrc) {
        TRACE(4, "Disable client not valid: regex=%s disrc=%d error=%s\n", regex_str, disallowedrc,
                ism_regex_getError(rc, xbuf, sizeof xbuf, disable->regex));
        ism_common_free(ism_memory_transportBuffers,disable);
        return ISMRC_ClientSetNotValid;
    }
    pthread_mutex_lock(&endpointlock);
    dp = disableClients;
    while (dp) {
        if (!strcmp(regex_str, dp->regex_str))
            break;
        dp = dp->next;
    }
    if (dp) {
        dp->count++;
        ism_regex_free(disable->regex);
        ism_common_free(ism_memory_transportBuffers,disable);
        disable = dp;
    } else {
        disable->count = 1;
        disable->rc = disallowedrc;
        disable->next = disableClients;
        disableClients = disable;
        doDisconnect = 1;
    }
    pthread_mutex_unlock(&endpointlock);

    TRACE(5, "Disable client set: %s\n", regex_str);

    /*
     * If we want to force a disconnect.
     * We actually expect that in normal usage this operation will be done when matching
     * clients are assumed to be already disconnected, but if not we guarantee they will
     * be disconnected.
     */
    if (doDisconnect) {
        uint32_t waited_millis = 0;
        bool allclosed = false;

        //Wait up to 3 minutes trying to disconnect
        while (waited_millis < 180000) {
            uint32_t connectioncount = 0;
            int wait_micros = connectioncount * 20 + 20000;
            ism_common_sleep(wait_micros);

            waited_millis += wait_micros/1000;
            connectioncount = ism_transport_closeClientSet(disable->regex);
            TRACE(6, "Close connections for client set: set=%s count=%d\n", regex_str, connectioncount);

            if (connectioncount == 0) {
                //We're done
                allclosed = true;
                break;
            }
        }

        if (!allclosed) {
            //The connections aren't closing properly... we'll fail with an error and undo
            //the addition of the regex to the blocked list
            int rc2 = ism_transport_enableClientSet(regex_str);
            TRACE(4, "Disable client set timed out: set=%s renablerc=%d\n", regex_str, rc2);

            rc = ISMRC_TimeOut;
        }
    }
    return rc;
}


/*
 * Enable the ability to connect from a set of clientIDs.
 *
 * If connection from a client set has been disabled, then enable it again.  The string used for
 * enable must exactly match the string used for disable.  If a client set is disabled multiple times,
 * it must be enabled the same number of times.  If the string is not found in the list of disabled
 * client states, no action is taken.
 *
 * @param regex_str  A regular expression to match the clietnIds to be enabled
 * @return A return code. 0=good, ISMRC_NotFound=a matching disable was not found
 */
int  ism_transport_enableClientSet(const char * regex_str) {
    int  rc = ISMRC_NotFound;
    disableClient_t * dp;
    disableClient_t * last = NULL;

    pthread_mutex_lock(&endpointlock);
    dp = disableClients;
    while (dp) {
        if (!strcmp(regex_str, dp->regex_str)) {
            rc = ISMRC_OK;
            if (dp->count > 1) {
                dp->count--;
            } else {
                if (last) {
                    last->next = dp->next;
                } else {
                    disableClients = NULL;
                }
                ism_regex_free(dp->regex);
                ism_common_free(ism_memory_transportBuffers,dp);
            }
            break;
        }
        last = dp;
        dp = dp->next;
    }
    pthread_mutex_unlock(&endpointlock);
    return rc;
}


/*
 * Check if this client is allowed by the disable rules.
 *
 * Check if a connection from a client has been disabled.  This support is commonly used for tp
 * temporarily stop connections from a set of clientIDs while some processing is being done.
 *
 * @param clienID  The clientID to check
 * @return A return code, 0=allowed, otherwise the return code specified when disabled.
 */
int  ism_transport_clientAllowed(ism_transport_t * transport) {
    int  rc = 0;
    const char * clientID = transport->name;
    disableClient_t * dp;

    /*
     * Check the disableClients outside a lock.  At most times this value is NULL and we
     * do not want to lock in that case.  The operation to set the value involves multiple
     * locks and a sleep so this should never be a problem.
     */
    if (disableClients) {
        pthread_mutex_lock(&endpointlock);
        transport->enabled_checked = true;


        dp = disableClients;
        while (dp) {
            if (ism_regex_match(dp->regex, clientID) == 0) {
                rc = dp->rc;
                ism_common_setError(rc);
                break;
            }
            dp = dp->next;
        }
        pthread_mutex_unlock(&endpointlock);
    } else {
        transport->enabled_checked = true;
    }
    return rc;
}

ism_transport_t * ism_transport_getPhysicalTransport(ism_transport_t * transport) {
    if(transport->virtualSid)
        transport = (ism_transport_t *) transport->tobj;
    return transport;
}

/*
 * Do the actual revalidation on the delivery thread
 */
static int revalidateCRL(ism_transport_t * transport, void * userdata, int flags) {
    char xbuf[256];
    int revalCode = ism_ssl_revalidateCert(transport->ssl);
    if ( revalCode == 0 ) {
        transport->close(transport, ISMRC_ConnectNotAuthorized, 0, xbuf);
    }
    return 0;
}

/**
 * Revoke connections based on CRL revocation list for specified endpoint
 *
 * @param endpoint  The endpoint name
 *
 */
int ism_transport_revokeConnectionsEndpoint(const char * endpoint) {
    ism_transport_t * transport;
    int  i;
    int  count = 0;

    if (!endpoint || !*endpoint)
        return -1;

    pthread_mutex_lock(&monitorlock);
    for (i=1; i<monitor_used; i++) {
        if (IS_VALID_MON(i)) {
            transport = monitorlist[i];
            if (transport->name && *transport->name && transport->listener && *transport->listener->name > '!') {
                if (!strcmp(transport->listener->name, endpoint)) {
                    TRACEL(8, transport->trclevel, "Check CRL revocation: endpoint=%s clientID=%s\n", transport->name, transport->clientID);
                    transport->addwork(transport, revalidateCRL, NULL);
                    count++;
                }
            }
        }
    }
    pthread_mutex_unlock(&monitorlock);
    return count;
}
