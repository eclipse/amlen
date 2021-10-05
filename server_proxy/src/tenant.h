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


#ifndef TENANT_H
#define TENANT_H
#include <ismjson.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <selector.h>
#include <pxrouting.h>
#include <pxtransport.h>

#define ISM_TENANT_RMPOLICY_REQSECURE    0x0001
#define ISM_TENANT_RMPOLICY_REQCERT      0x0002
#define ISM_TENANT_RMPOLICY_REQUSER      0x0004
#define ISM_TENANT_RMPOLICY_REQSNI       0x0008

#define LICENSE_None          0
#define LICENSE_Developers    1
#define LICENSE_NonProduction 2
#define LICENSE_Production    3

typedef struct ism_tenant_t ism_tenant_t;
typedef struct ism_user_t   ism_user_t;
typedef struct ism_server_t ism_server_t;
typedef struct ism_topicrule_t ism_topicrule_t;
typedef struct ism_mhub_t   ism_mhub_t;


/*
 * Server cipher selected by UseTLS
 */
enum server_cipher_e {
    Server_Cipher_None = 0,
    Server_Cipher_Fast = 1,
    Server_Cipher_Best = 2
};

/*
 * Tenant auth setting
 */
enum tenant_auth_e {
    Tenant_Auth_None         = 0,
    Tenant_Auth_Token        = 1,
    Tenant_Auth_CertOrToken  = 2,
    Tenant_Auth_CertAndToken = 3
};

/*
 * Fair use values
 */
typedef struct tenant_fairuse_t {
    int          mups_units;   /* Unit size */
    uint8_t      log;          /* 0=false, 1=true, 2=only */
    uint8_t      enabled;
    uint8_t      resv1[2];
    float        mups_d;       /* Device */
    float        mups_g;       /* Gateway */
    float        mups_a;       /* Application */
    float        mups_A;       /* Shared application */
//  ismRule_t *  route;        /* Compiled routing rule */
} tenant_fairuse_t;


/*
 * Structure for a tenant.
 * This is temporary.
 * Note: Keep the padding for 8 bytes
 */
struct ism_tenant_t {
    struct ism_tenant_t * next;
    char         structid[8];
    uint32_t     hash;            /* Used for lookup bucket                    */
    int          namelen;         /* Length of name (not including terminator) */
    const char * name;            /* Null terminated tenant name               */
    const char * configname;      /* Name of the config file                   */
    pthread_spinlock_t  lock;
    ism_user_t * users;
    ism_server_t * server;        /* Server object, can be subclass of server  */
    const char * serverstr;       /* Name of the MessageSight server           */
    int          max_connects;    /* Max connections for all proxies           */
    int          rc;
    ism_topicrule_t * topicrule;  /*                                           */
    uint8_t      enabled;         /* Tenant is enabled                         */
    uint8_t      allow_durable;   /* Allow a durable connection                */
    uint8_t      allow_systopic;  /* Allow subscribe to system topics          */
    uint8_t      max_qos;         /* Max QoS for publish and subscribe         */
    uint8_t      require_secure;  /* Require a secure connection               */
    uint8_t      require_cert;    /* Require a user certificate                */
    uint8_t      require_user;    /* Require a userid even if there is a cert  */
    uint8_t      check_user;      /* Check the userid in the proxy             */
    uint8_t      remove_user;     /* Remove userid and pw in the proxy         */
    uint8_t      allow_anon;      /* Allow anonymous connect                   */
    uint8_t      allow_shared;    /* Allow shared subscriptions                */
    uint8_t      allow_retain;    /* Allow retained messages                   */
    uint8_t      user_is_clientid;/* The clientid is used as the userid        */
    uint8_t      useSNI;
    uint8_t      sgEnabled;
    uint8_t      serverless;      /* The connection is serverless */
    uint32_t     maxMsgSize;
    uint32_t     maxSessionExpire;
    uint8_t		 rlacAppResourceGroupEnabled;		/* RLAC Enabled for App.*/
	uint8_t		 rlacAppDefaultGroup; 				/* RLAC Default Group for App. */
	uint8_t      authenticate;
    uint8_t      modified;                          /* Tenant modified */
	int 		 messageRouting;                    /* Message Routing */
	const char * serverCert;                        /* Server certificate */
	const char * serverKey;                         /* Server private key */
	const char * keyPassword;                       /* Server private key password (obfuscated) */
	const char * caCerts;                           /* Trust store CAs */
    const char * plan;                              /* The plan name 1-31 chars, no spaces */
	const char * fairUsePolicy;						/* fairUsePolicy */
	ism_mhub_t * mhublist;                          /* List of MessageHub entries */
	ism_timer_t  mhubtimer;                         /* Timer for MessageHub */
    tenant_fairuse_t fairuse;
    ism_routing_config_t * routeConfig;
    int          rmPolicies;
    uint8_t      disableCRL;
    uint8_t      pxactEnabled;                      /* Client activity tracking                  */
    uint8_t      checkSessionUser;
    uint8_t      willTopicValidationPolicy;
};


/*
 * User structure.  This is used for internal users
 */
struct ism_user_t {
    struct ism_user_t * next;
    char         structid[8];
    const char * name;
    const char * password;
    uint32_t     role;
    uint32_t     resv;
};

typedef struct ima_server_address_t {
	char 		* ipString;
    struct addrinfo * ip;
    char		ipInfo[68];
    uint32_t    useCount;
    uint16_t	port;
    uint16_t    resv;
    int32_t     node;
} ima_server_address_t ;

typedef enum {
    TCP_DISCONNECTED,
    TCP_DISCONNECTING,
    TCP_CON_IN_PROCESS,
    TCP_CONNECTED,
    PROTOCOL_CON_IN_PROCESS,
    PROTOCOL_CONNECTED
} ism_serverConnection_State;

typedef struct serverConnection_t {
    struct ima_pxtransport_t *  transport;
    pthread_spinlock_t          lock;
    ism_serverConnection_State  state;
    uint8_t                     useCount;
    uint8_t                     index;
    volatile uint8_t            disabled;
    uint8_t                     rsrv[5];
} serverConnection_t;

typedef struct ima_pxtransport_t ism_transport_t;


/**
 * Callback for the completion of name resolution
 *
 * There are several objects which subclass server and allow an outbound connection.
 * @param transport  A transport object
 * @param rc         An ISMRC or GAI return value
 * @param address    An addrinfo structure (which might be a list)
 *
 * The address might well be a list containing multiple resolutions.  The implementor of this
 * callback should choose one of them.  The numeric string form of the address can only be
 * created after the choice of which resolution to use.
 *
 * If the rc is negative it is an GAI error code.  If it >=100 it is an ISMRC.  The value 0 means
 * there is no error, and the values 0-99 are available so use by the implementor.
 *
 */
typedef void (* ism_gotAddress_f)(ism_transport_t * transport, int rc, struct addrinfo * addrinfo);

struct ism_server_t;

/*
 * Call to request an async name resolution from the server
 *
 * There are several objects which subclass server and allow an outbound connection.
 * @param server     A server object or an object which subclasses server
 * @param transport  A transport object
 * @param gotAddress The callback for the async completion of the getAddress method
 * @return  An ISMRC return code for synchronous errors.
 *
 * The server and gotAddress fields are stored in the transport object during the async operation.
 * If the parameters to this call are NULL, the values already in the transport object are used.
 * There must be specified by one of these two methods.
 */
typedef int (* ism_getAddress_f)(struct ism_server_t * server, ism_transport_t * transport, ism_gotAddress_f gotAddress);

/*
 * Server object
 */
struct ism_server_t {
    char         structid[8];
    struct ism_server_t * next;
    const char * name;
    uint8_t      last_good;          /* 0=ipaddr, 1=backup */
    uint8_t      useTLS;             /* 0=none, 1=fast, 2=best */
    uint8_t      serverKind;         /* 0=MQTT, 1=Kafka, 2=SQS, 3=bridge, 5=mhub */
    uint8_t      disabled;
    int          port;
    pthread_spinlock_t  lock;
    struct ssl_ctx_st * tlsCTX;
    ism_getAddress_f  getAddress;
    /* The fields before this match common alternate server objects */

    int          ipaddr_count;
    int          backup_count;
    uint8_t      mqttProxyProtocol;
    uint8_t      monitor_connect;
    uint8_t      monitor_retain;
    uint8_t      monitor_qos;
    uint8_t      resv [3];
    uint8_t      need_start;

    const char * metering_topic;
    const char * monitor_topic;
    const char * monitor_topic_alt;
    const char * address [32];
    const char * nodeaddr [32];         /* Node addresses used by kafka */
    int          a_port [32];           /* Ports used by kafka */
    int          n_port [32];           /* Node ports used by kafka */
    int          node [32];             /* Node ID used by kafka */
    struct ima_pxtransport_t *  monitor;
    serverConnection_t          mqttCon;
    struct serverConnection_t * mux;
    uint32_t     maxMsgSize;
    uint32_t     resvi;
    /*AWS SQS*/
    const char * awssqs_url;
};

typedef struct px_server_stats_t {
    const char * name;
    const char * primaryIPs[32];
    const char * backupIPs[32];
    uint32_t     primaryUseCount[32];
    uint32_t     backupUseCount[32];
    uint16_t     port;
    uint16_t     pendingHTTPRequests;
    uint8_t      usePrimary;
    uint8_t      primaryCount;
    uint8_t      backupCount;
    uint8_t      connectionState;
} px_server_stats_t;

typedef struct ism_devicestatus_t {
	const char *  clientID;
	const char * clientAddress;
	int port;
	const char * protocol;
	const char * userID;
	const char * action;
	char * timestamp;
}ism_devicestatus_t;

#define RULEEND  0xff
#define RULEVAR  0xfe
#define RULEWILD 0xfd

#define RULEEND_Any     0x01
#define RULEEND_None    0x02
#define RULEVAR_Org     0x01
#define RULEVAR_Id      0x02
#define RULEVAR_Type    0x03
#define RULEWILD_Plus   0x01
#define RULEWILD_NoWild 0x02
#define RULEWILD_Any    0x03
#define RULETYPE_Topic  0x01
#define RULETYPE_Class  0x02
#define RULETYPE_Convert 0x03

/*
 * An individual rule entry
 */
typedef struct ism_pxrule_t {
    struct ism_pxrule_t * next;
    char         class;
    uint8_t      endrule;
    uint8_t      after;
    uint8_t      ruletype;
    int          auth;
    int          option;
    int          rulelen;
    uint8_t *    rule;
} ism_pxrule_t;


/*
 * Client class object
 */
typedef struct ism_clientclass_t {
    struct ism_clientclass_t * next;
    char         structID[8];
    const char * name;
    int          namelen;
    ism_pxrule_t * classlist;
    ism_pxrule_t * deflt;
} ism_clientclass_t;

/*
 * Topic rule object
 */
struct ism_topicrule_t {
    struct ism_topicrule_t * next;
    struct ism_topicrule_t * child;
    char         structID[8];
    const char * name;
    int          namelen;
    uint8_t      clientclass;
    uint8_t      nohash;           /* Do not allow # wildcard */
    uint8_t      flags;
    uint8_t      remove_count;     /* Elements to remove on outbound conversion */
    int          publish_count;
    int          subscribe_count;
    ism_pxrule_t * * publish;
    ism_pxrule_t * * subscribe;
    ism_pxrule_t * convert;
};

/*
 * Find the tenant by name
 */
XAPI ism_tenant_t * ism_tenant_getTenant(const char * name);

/*
 * Lock the tenant config
 */
XAPI void ism_tenant_lock_int(const char * file, int line);
#define ism_tenant_lock() ism_tenant_lock_int(__FILE__, __LINE__);

/*
 * Unlock the tenant config
 */
XAPI void ism_tenant_unlock(void);



/*
 * Find the user by name
 */
XAPI ism_user_t * ism_tenant_getUser(const char * name, ism_tenant_t * tenant, int only);

/*
 * Find the tenant by name
 */
XAPI ism_server_t * ism_tenant_getServer(const char * name);

/*
 * Configure a tenant
 */
XAPI int ism_tenant_config_json(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing);
/*
 * Create an obfuscated password
 */
XAPI int  ism_tenant_createObfus(const char * user, const char * pw, char * buf, int buflen, int otype);


/*
 * Check obfuscated password.
 * - Type 0 is a plain text password, and if it starts with a character < 0x30
 *          it must have a preceding backslash.
 * - Type 1 is a SHA256 password in base64 encoding, and it has a leading equals (=)
 *          to indicate the type.
 * - Other starter characters less than 0x30 ('0') are reserved for other password types.
 * @return 0=does not match, 1=matches
 */
XAPI int ism_tenant_checkObfus(const char * user, const char * pw, const char * obfus);

int ism_proxy_complexConfig(ism_json_parse_t * parseobj, int complex, int checkonly, int keepgoing);
int ism_tenant_makeTenant(ism_json_parse_t * parseobj, int where, const char * name);
int ism_tenant_makeUser(ism_json_parse_t * parseobj, int where, const char * name, ism_tenant_t * tenant, int checkonly, int keepgoing);
int ism_tenant_makeServer(ism_json_parse_t * parseobj, int where, const char * name);
int ism_proxy_makeEndpoint(ism_json_parse_t * parseobj, int where, const char * name, int checkonly, int keepgoing);
int ism_tenant_getTenantJson(ism_tenant_t * tenant, ism_json_t * jobj, const char * name);
int ism_tenant_getServerJson(ism_server_t * server, ism_json_t * jobj, const char * name);
int ism_tenant_getUserJson(ism_user_t * user, ism_json_t * jobj, const char * name);
int ism_tenant_getTenantList(const char * match, ism_json_t * jobj, int json, const char * name);
int ism_tenant_getServerList(const char * match, ism_json_t * jobj, int json, const char * name);
int ism_tenant_getUserList(const char * match, ism_tenant_t * tenant, ism_json_t * jobj, int json, const char * name);
int ism_tenant_getEndpointStats(const char * match, ism_json_t * jobj);
int ism_tenant_getServerStats(const char * match, ism_json_t * jobj);

/*
 * Command to print tenants
 */
XAPI void ism_tenant_printTenants(const char * pattern);

/*
 * Command to print users
 */
XAPI void ism_tenant_printUsers(const char * pattern);

/*
 * Command to print users
 */
XAPI void ism_tenant_printTenantUsers(const char * pattern);

/*
 * Command to print servers
 */
XAPI void ism_tenant_printServers(const char * pattern);

XAPI void ism_server_setLastGoodAddress(ism_server_t * server, int isBackup);

XAPI void ism_server_setLastBadAddress(ism_server_t * server, int isBackup);

XAPI int ism_proxy_getServersStats(px_server_stats_t * stats, int * pCount);


/*
 * Check if a device id is known
 */
uint64_t ism_proxy_devknown(const char * org, const char * devtype, const char * devid);

/*
 * Add a device to the known list
 */
int ism_proxy_addknown(const char * org, const char * devtype, const char * devid);

#endif
