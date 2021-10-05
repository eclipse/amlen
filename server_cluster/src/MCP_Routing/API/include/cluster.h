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

/*********************************************************************/
/*                                                                   */
/* Module Name: cluster.h                                            */
/*                                                                   */
/* Description: Cluster component external header file               */
/*                                                                   */
/*********************************************************************/
#ifndef __CLUSTER_API_DEFINED
#define __CLUSTER_API_DEFINED

#ifdef MCC_PROTOTYPE
 #ifndef XAPI
 #define XAPI extern
 #endif
#else
#include <ismutil.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*********************************************************************/
/* Cluster External Configuration Parameters                         */
/*********************************************************************/

/* Cluster.EnableClusterMembership                                             */
/* Enable or disable cluster services.                                         */
/*                                                                             */
/* A value of 0 means disabled.                                                */
/* A value of 1 means enabled.                                                 */
/*                                                                             */
/* The type of the parameter is uint8_t.                                       */
/* The default value is 0                                                      */
#define ismCLUSTER_CFG_ENABLECLUSTER       "Cluster.EnableClusterMembership"

/* Cluster.ClusterName                                                         */
/* The name of the cluster.                                                    */
/* Only servers with the same ClusterName are grouped in the same cluster.     */
/*                                                                             */
/* The ClusterName must not have leading or trailing spaces and cannot contain */
/* control characters, commas, double quotation marks, backslashes, slashes,   */
/* or equal signs. The first character must not be a number or any of the      */
/* following special characters: ! # $ % & ' ( ) * + - . : ; < > ? @           */
/*                                                                             */
/* The type of the parameter is string.                                        */
/* The default value is MessageSightCluster                                    */
#define ismCLUSTER_CFG_CLUSTERNAME         "Cluster.ClusterName"

/* Cluster.DiscoveryServerList                                                 */
/* The bootstrap set that is used to discover remote servers in the cluster.   */
/* The DiscoveryServerList is provided as a list of comma separated pairs of   */
/* the control IP address and control port of a remote server in the cluster.  */
/* The format of a pair is [address]:Port (the square brackets are optional    */
/* for Ipv4 addresses)                                                         */
/* For example: "[10.10.10.1]:2001, 10.10.10.2:20002".                         */
/*                                                                             */
/* In case a remote server is using a Cluster.ControlExternalAddress and/or    */ 
/* Cluster.ControlExternalPort, the respective entry in the                    */ 
/* discovery server list should be the external address/port.                  */
/*                                                                             */
/* The type of the parameter is string.                                        */
/* The default value is NULL                                                   */
#define ismCLUSTER_CFG_BOOTSTRAPSET        "Cluster.DiscoveryServerList"

/* Cluster.DiscoveryPort                                                       */
/* Port number for Cluster multicast discovery.                                */
/* This is a well known port that is used by all cluster members.              */
/* Range 0 - 65535                                                             */
/*                                                                             */
/* The type of the parameter is uint16_t.                                      */
/* The default value is 9091.                                                  */
#define ismCLUSTER_CFG_DISCOVERYPORT       "Cluster.DiscoveryPort"

/* Cluster.DiscoveryExternalPort                                               */
/* The external port number for Cluster multicast discovery.                   */
/* This is a well known port that is used by all cluster members.              */
/* A value of zero means the value of Cluster.DiscoveryPort should be used.    */
/* Range 0 - 65535                                                             */
/*                                                                             */
/* The type of the parameter is uint16_t.                                      */
/* The default value is 0.                                                     */
#define ismCLUSTER_CFG_DISCOVERYPORTEXT    "Cluster.DiscoveryExternalPort"

/* Cluster.ControlPort                                                         */
/* Port number for Cluster control communication.                              */
/* Range 0 - 65535                                                             */
/*                                                                             */
/* The type of the parameter is uint16_t.                                      */
/* The default value is 9102.                                                  */
#define ismCLUSTER_CFG_CONTROLPORT         "Cluster.ControlPort"

/* Cluster.ControlExternalPort                                                 */
/* The external port number to be used for Cluster control communication.      */
/* A value of zero means the value of Cluster.ControlPort should be used.      */
/* Range 0 - 65535                                                             */
/*                                                                             */
/* The type of the parameter is uint16_t.                                      */
/* The default value is 0.                                                     */
#define ismCLUSTER_CFG_CONTROLPORTEXT      "Cluster.ControlExternalPort"

/* Cluster.ControlAddress                                                      */
/* The network address for Cluster control communication.                      */
/* A value of NULL means that an address should be selected internally.        */
/*                                                                             */
/* An IP address, format specified in Cluster.DiscoveryServerList above.       */
/*                                                                             */
/* The type of the parameter is string.                                        */
/* The default value is NULL                                                   */
#define ismCLUSTER_CFG_CONTROLADDR         "Cluster.ControlAddress"

/* Cluster.ControlExternalAddress                                              */
/* The external network address for Cluster control communication.             */
/* A value of NULL means the value of Cluster.ControlAddress should be used.   */
/*                                                                             */
/* An IP address, format specified in Cluster.DiscoveryServerList above.       */
/*                                                                             */
/* The type of the parameter is string.                                        */
/* The default value is NULL                                                   */
#define ismCLUSTER_CFG_CONTROLADDREXT      "Cluster.ControlExternalAddress"

/* Cluster.MulticastDiscoveryTTL                                               */
/* TTL to use for multicast discovery.                                         */
/*                                                                             */
/* The type of the parameter is uint8_t.                                       */
/* The default value is 1.                                                     */
#define ismCLUSTER_CFG_MCASTTTL            "Cluster.MulticastDiscoveryTTL"

/* Cluster.UseMulticastDiscovery                                               */
/* A flag indicating whether multicast discovery should be enabled.            */
/* Multicast discovery can be used with or without a bootstarp set.            */
/*                                                                             */
/* A value of 0 means multicast discovery is disabled.                         */
/* A value of 1 means multicast discovery is enabled.                          */
/*                                                                             */
/* The type of the parameter is uint8_t.                                       */
/* The default value is 1                                                      */
#define ismCLUSTER_CFG_MULTICASTDISCOVERY  "Cluster.UseMulticastDiscovery"

/* Cluster.DiscoveryTime                                                       */
/* The time, in seconds, that Cluster spends during server start up to         */
/* discover other servers in the cluster and get updated information from them.*/
/* The local server becomes an active member of the cluster after it either    */
/* obtains updated information on all the remote servers or that the discovery */
/* time expired.                                                               */
/*                                                                             */
/* The type of the parameter is uint32_t.                                      */
/* The default value is 10.                                                    */
#define ismCLUSTER_CFG_DISCOVERYTIME       "Cluster.DiscoveryTime"

/* Cluster.MulticastAddressIPv4                                                */
/* Specifies the IPv4 multicast group address used when multicast discovery    */
/* is enabled. A value of NULL indicates the default value is used.            */
/*                                                                             */
/* The type of the parameter is a string.                                      */
/* The default value is 239.192.97.105.                                        */
#define ismCluster_CFG_IPV4_MCAST_ADDR     "Cluster.MulticastAddressIPv4"

/* Cluster.MulticastAddressIPv6                                                */
/* Specifies the IPv6 multicast group address used when multicast discovery    */
/* is enabled. A value of NULL indicates the default value is used.            */
/*                                                                             */
/* The type of the parameter is a string.                                      */
/* The default value is ff18::6169.                                            */
#define ismCluster_CFG_IPV6_MCAST_ADDR     "Cluster.MulticastAddressIPv6"

/* Cluster.ControlTLSPolicy                                                    */
/*                                                                             */
/* Specifies the usage of TLS for cluster control channel connections.         */
/* A value of 1 means "Always On" (default).                                   */
/* A value of 2 means "Always Off".                                            */
/* A value of 3 means "Follow Messaging", i.e. use the value supplied by       */
/* the call to ism_cluster_setLocalForwardingInfo().                           */
/*                                                                             */
/* The type of the parameter is integer.                                       */
/* The default value is 1 (Always On).                                         */
#define ismCluster_CFG_CONTROL_TLS_POLICY  "Cluster.ControlTLSPolicy"

/*********************************************************************/
/* Cluster Internal Configuration Parameters                         */
/*********************************************************************/
/* Cluster.ControlHeartbeatTimeout                                             */
/* The heartbeat timeout in seconds for the control connections.               */
/*                                                                             */
/* The type of the parameter is uint32_t.                                      */
/* The default value is 20.                                                    */
#define ismCLUSTER_CFG_CONTROLHBTO         "Cluster.ControlHeartbeatTimeout"

/* Cluster.BFMaxAttributes                                                     */
/* The maximal number of attributes dedicated to Bloom filter updates before a */
/* new base is published.                                                      */
/*                                                                             */
/* The type of the parameter is uint32_t.                                      */
/* The default value is 1000.                                                  */
#define ismCLUSTER_CFG_BF_MAX_ATTRIBUTES           "Cluster.BFMaxAttributes"

/* Cluster.BFPublishTaskIntervalMillis                               */
/* The interval in milliseconds of the task that publishes local filters,      */
/* bloom filters and otherwise.                                                */
/*                                                                             */
/* The type of the parameter is uint32_t.                                      */
/* The default value is 100.                                                   */
#define ismCLUSTER_CFG_BF_PUBLISH_INTERVAL_MILLIS  "Cluster.BFPublishTaskIntervalMillis"


/*********************************************************************/
/* Cluster Stand Alone Tests Configuration Parameters                */
/*********************************************************************/
/* Cluster.TestStandAlone                                                      */
/* Indicates if Cluster is running in stand alone mode (i.e., outside of       */ 
/* MessageSight).                                                              */
/*                                                                             */
/* The type of the parameter is uint8_t.                                       */
/* The default value is 0                                                      */
#define ismCLUSTER_CFG_TESTSTANDALONE      "Cluster.TestStandAlone"

/* Cluster.ConfigFile                                                          */
/* A configuration file for tuning internal parameters.                        */
/*                                                                             */
/* The type of the parameter is string.                                        */
/* The default value is NULL                                                   */
#define ismCLUSTER_CFG_CONFIGFILE          "Cluster.ConfigFile"

/* Cluster.LogFile                                                             */
/* Log file to use in Cluster stand alone tests.                               */
/*                                                                             */
/* The type of the parameter is string.                                        */
/* The default value is NULL                                                   */
#define ismCLUSTER_CFG_LOGFILE             "Cluster.LogFile"

/* Cluster.LogLevel                                                            */
/* Log level to use in Cluster stand alone tests.                              */
/*                                                                             */
/* The type of the parameter is uint8_t.                                       */
/* The default value is 0                                                      */
#define ismCLUSTER_CFG_LOGLEVEL            "Cluster.LogLevel"

/* Cluster.TestNIC                                                             */
/* NIC to use in Cluster stand alone tests.                                    */
/*                                                                             */
/* The type of the parameter is string.                                        */
/* The default value is eth0                                                   */
#define ismCLUSTER_CFG_TESTNIC             "Cluster.TestNIC"


/*********************************************************************/
/* Cluster API                                                       */
/*********************************************************************/

typedef struct ismCluster_RemoteServer_t  * ismCluster_RemoteServerHandle_t;
typedef struct ismEngine_RemoteServer_t   * ismEngine_RemoteServerHandle_t;
typedef struct ismEngine_RemoteServer_PendingUpdate_t  * ismEngine_RemoteServer_PendingUpdateHandle_t;
typedef struct ismProtocol_RemoteServer_t * ismProtocol_RemoteServerHandle_t;

#define ismENGINE_NULL_REMOTESERVER_HANDLE NULL

/*
 * A structure representing cumulative statistics of incoming messages
 * from the forwarding channel.
 */
typedef struct
{
    uint64_t numFwdIn;            /* The total number of forwarded incoming messages,
                                     not including retained messages.                 */
    uint64_t numFwdInNoConsumer;  /* The number of forwarded incoming messages,
                                     not including retained messages,
                                     that have no local consumers.                    */
    uint64_t numFwdInRetained;    /* The total number of forwarded incoming messages
                                     that are retained messages.                      */
} ismCluster_EngineStatistics_t;

/* Remote Server event types delivered to the Engine                           */
/* ENGINE_RS_CREATE_LOCAL:                                                     */
/* Create an entry for the local server.                                       */
/* The local server entry is used only to allow Cluster to persist local       */
/* server information in the Store.                                            */
/*                                                                             */
/* ENGINE_RS_CREATE:                                                           */
/* Create a remote server definition for a newly discovered server.            */
/* This event is used to inform the Engine of a new remote server in the       */
/* cluster.                                                                    */
/* This event is used only for remote servers that the Engine is not yet       */
/* aware of.                                                                   */
/*                                                                             */
/* ENGINE_RS_CONNECT:                                                          */
/* This event is used to inform the Engine that a remote server is connected   */
/* to the cluster.                                                             */
/* This connect indication is similar to a connect event of a local client.    */
/*                                                                             */
/* ENGINE_RS_DISCONNECT:                                                       */
/* This event is used to inform the Engine that a remote server has been       */
/* disconnected from the cluster.                                              */
/* This disconnect indication is similar to a connect event of a local client. */
/*                                                                             */
/* ENGINE_RS_REMOVE:                                                           */
/* This event is used to inform the Engine that a remote server should be      */
/* removed. All data related to the remote server should be discarded.         */
/*                                                                             */
/* ENGINE_RS_UPDATE:                                                           */
/* This event is used by the Cluster component to persist remote server data   */
/* using the Store. The Engine maintains the data provided by Cluster along    */
/* with the other information it maintains for this remote server and          */
/* reconstructs the data during server recovery. When recovery is completed    */
/* the Engine delivers the data to the Cluster component.                      */
/*                                                                             */
/* ENGINE_RS_ADD_SUB:                                                          */
/* Add (wildcard) subscriptions to the Engine's topic tree for a remote        */
/* server.                                                                     */
/*                                                                             */
/* ENGINE_RS_DEL_SUB:                                                          */
/* Delete (wildcard) subscriptions from the Engine's topic tree for a remote   */
/* server.                                                                     */
/*                                                                             */
/* ENGINE_RS_ROUTE:                                                            */
/* This event is used by the Cluster component to report the route-all status  */
/* of a remote server.                                                         */
/*                                                                             */
/* ENGINE_RS_REPORT_STATS                                                      */
/* This event is used by the Cluster component in order to get cumulative      */
/* engine statistics of incoming messages from the forwarding channel. This is */
/* invoked periodically at most every 10 seconds.  The cluster uses these      */
/* statistics to evaluate the false positive (error-rate) performance of the   */
/* Bloom filters it publishes to the rest of the cluster.                      */
/*                                                                             */
/* ENGINE_RS_TERM:                                                             */
/* This event is used by the Cluster component to indicate that the local      */
/* server has been removed from the cluster and the Cluster component is about */
/* to shut down.                                                               */
/* The Engine should clean all cluster related resources and either avoid      */
/* making new calls to server_cluster or expect ISMRC_ClusterDisabled to be    */
/* returned by future calls.                                                   */
typedef enum
{
   ENGINE_RS_CREATE_LOCAL = 1,           /* Create a local server entry        */
   ENGINE_RS_CREATE       = 2,           /* Create a new remote server         */
   ENGINE_RS_CONNECT      = 3,           /* Remote server is connected         */
   ENGINE_RS_DISCONNECT   = 4,           /* Remote server is Disconnect        */
   ENGINE_RS_REMOVE       = 5,           /* Remove a remote server             */
   ENGINE_RS_UPDATE       = 6,           /* Update data for a remote server    */
   ENGINE_RS_ADD_SUB      = 7,           /* Add a (wildcard) subscription for  */
                                         /* the remote server                  */
   ENGINE_RS_DEL_SUB      = 8,           /* Delete a (wildcard) subscription   */
                                         /* for the remote server              */
   ENGINE_RS_ROUTE        = 9,           /* Report route-all status            */
   ENGINE_RS_REPORT_STATS = 10,          /* Report engine statistics           */
   ENGINE_RS_TERM         = 99           /* Terminate/disable cluster          */
}ENGINE_RS_EVENT_TYPE_t ;

/*******************************************************************************/
/* A callback to deliver remote server events to the Engine.                   */
/*                                                                             */
/* @param[in]  eventType               Event type                              */
/* @param[in]  hRemoteServer           Engine handle for the remote server.    */
/*                                     The handle is NULL for events of type   */
/*                                     CREATE and must be a valid handle for   */
/*                                     all other event types                   */
/* @param[in]  hClusterHandle          Cluster handle for the remote server.   */
/* @param[in]  pServerName             Remote server name                      */
/* @param[in]  pServerUID              Remote server unique ID                 */
/* @param[in]  pRemoteServerData       Pointer to remote server data.          */
/*                                     Used only for events of type UPDATE     */
/* @param[in]  remoteServerDataLength  Length of pRemoteServerData in bytes    */
/* @param[in]  pSubscriptions          Array of subscriptions to add/delete    */
/*                                     to/from the Engine topic tree.          */
/*                                     Used only in ENGINE_RS_ADD_SUB and      */
/*                                     ENGINE_RS_DEL_SUB event types           */
/* @param[in]  subscriptionsLength     Length of the pSubscriptions array      */
/* @param[in]  fIsRouteAll             A flag indicating if the remote server  */
/*                                     is currently in route-all state         */
/* @param[in]  fCommitUpdate           Used to instruct the engine to commit   */
/*                                     pending UPDATE requests to the store    */
/*                                     including the current UPDATE.           */
/* @param[in/out] phPendingUpdateHandle Engine pending update handle must      */
/*                                     contain NULL pointer on first call to   */
/*                                     UPDATE and is set to non-NULL value     */
/*                                     if fCommitUpdate is set specified.      */
/* @param/in/out] pEngineStatistics    A pointer to a structure that the       */
/*                                     engine has to fill with statistics when */
/*                                     event type is ENGINE_RS_REPORT_STATS.   */
/*                                     The pointer may be NULL on all other    */
/*                                     events.                                 */
/* @param[in]  pContext                Pointer to the caller's context         */
/* @param[out] phEngineHandle          Returned Engine remote server handle.   */
/*                                     Used only of events of type CREATE      */
/*                                                                             */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.               */
/*******************************************************************************/
typedef int32_t (*ismEngine_RemoteServerCallback_t)(
    ENGINE_RS_EVENT_TYPE_t             eventType,
    ismEngine_RemoteServerHandle_t     hRemoteServer,
    ismCluster_RemoteServerHandle_t    hClusterHandle,
    const char                        *pServerName,
    const char                        *pServerUID,
    void                              *pRemoteServerData,
    size_t                             remoteServerDataLength,
    char                             **pSubscriptions,
    size_t                             subscriptionsLength,
    uint8_t                            fIsRoutAll,
    uint8_t                            fCommitUpdate,
    ismEngine_RemoteServer_PendingUpdateHandle_t  *phPendingUpdateHandle,
    ismCluster_EngineStatistics_t     *pEngineStatistics,
    void                              *pContext,
    ismEngine_RemoteServerHandle_t    *phEngineHandle);

/*******************************************************************************/
/* Register an Engine remote server event callback                             */
/*                                                                             */
/* @param callback      Engine callback function                               */
/* @param pContext      Context to associate with the callback function        */
/*                                                                             */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.               */
/*******************************************************************************/
XAPI int32_t ism_cluster_registerEngineEventCallback(ismEngine_RemoteServerCallback_t callback, void *pContext);

/* Remote Server event types delivered to the Protocol                         */
/*                                                                             */
/* PROTOCOL_RS_CREATE:                                                         */
/* Create a remote server definition for a newly discovered server and         */
/* provides the destination address and port Protocol should use to connect    */
/* to the node                                                                 */
/* This event is used to inform the Protocol of a new remote server in the     */
/* cluster.                                                                    */
/* This event is used only for remote servers that the Protocol is not yet     */
/* aware of.                                                                   */
/*                                                                             */
/* PROTOCOL_RS_DISCONNECT:                                                     */
/* This event is used to inform the Protocol that a remote server has been     */
/* detected to be disconnected by the Cluster control component.               */
/*                                                                             */
/* PROTOCOL_RS_REMOVE:                                                         */
/* This event is used to inform the Protocol that a remote server should be    */
/* removed. All data related to the remote server should be discarded.         */
/*                                                                             */
/* PROTOCOL_RS_CONNECT:                                                        */
/* This event is used by the Cluster component to inform Protocol that a       */
/* remote server has been connected by the Cluster control component.          */
/* The event provides the Protocol parameters of the remote server including   */
/* the destination address and port.                                           */
/*                                                                             */
/* PROTOCOL_RS_TERM:                                                           */
/* This event is used by the Cluster component to indicate that the local      */
/* server has been removed from the cluster and the Cluster component is about */
/* to shut down.                                                               */
/* Protocol should clean all cluster related resources and either avoid making */
/* new calls to server_cluster or expect ISMRC_ClusterDisabled to be returned  */
/* by future calls.                                                            */
typedef enum
{
   PROTOCOL_RS_CREATE      = 1,         /* Create a new remote server          */
   PROTOCOL_RS_CONNECT     = 2,         /* Remote server is connected          */
   PROTOCOL_RS_DISCONNECT  = 3,         /* Remote server is disconnect         */
   PROTOCOL_RS_REMOVE      = 4,         /* Remove a remote server              */
   PROTOCOL_RS_TERM        = 99         /* Terminate/disable cluster           */
}PROTOCOL_RS_EVENT_TYPE_t ;

/*******************************************************************************/
/* A callback to deliver remote server events to the Protocol.                 */
/*                                                                             */
/* @param[in]  eventType               Protocol type                           */
/* @param[in]  hRemoteServer           Protocol handle for the remote server.  */
/*                                     The handle is NULL for events of type   */
/*                                     CREATE and must be a valid handle for   */
/*                                     all other event types                   */
/* @param[in]  pServerName             Remote server name                      */
/* @param[in]  pServerUID              Remote server unique ID                 */
/* @param[in]  pRemoteServerAddress    Address used by the remote server.      */
/*                                     Used only for events of type CREATE and */
/*                                     UPDATE                                  */
/* @param[in]  remoteServerPort        Port used by the remote server.         */
/*                                     Used only for events of type CREATE and */
/*                                     UPDATE                                  */
/* @param[in]  fUseTLS                 A flag indicating whether the remote    */
/*                                     server uses TLS for forwarding.         */
/* @param[in]  hClusterHandle          Cluster handle for the remote server.   */
/* @param[in]  hEngineHandle           Engine handle for the remote server.    */
/* @param[in]  pContext                Pointer to the caller's context         */
/* @param[out] phProtocolHandle        Returned Protocol remote server handle. */
/*                                     Used only of events of type CREATE      */
/*                                                                             */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.               */
/*******************************************************************************/
typedef int32_t (*ismProtocol_RemoteServerCallback_t)(
    PROTOCOL_RS_EVENT_TYPE_t           eventType,
    ismProtocol_RemoteServerHandle_t   hRemoteServer,
    const char                        *pServerName,
    const char                        *pServerUID,
    const char                        *pRemoteServerAddress,
    int                                remoteServerPort,
    uint8_t                            fUseTLS,
    ismCluster_RemoteServerHandle_t    hClusterHandle,
    ismEngine_RemoteServerHandle_t     hEngineHandle,
    void                              *pContext,
    ismProtocol_RemoteServerHandle_t  *phProtocolHandle);

/*******************************************************************************/
/* Register a Protocol remote server event callback                            */
/*                                                                             */
/* @param callback      Protocol callback function                             */
/* @param pContext      Context to associate with the callback function        */
/*                                                                             */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.               */
/*******************************************************************************/
XAPI int32_t ism_cluster_registerProtocolEventCallback(ismProtocol_RemoteServerCallback_t callback, void *pContext);


/* A structure representing subscription information.                          */
/* The structure is used by the Engine to update the clustering component      */
/* about changes to local subscriptions.                                       */
typedef struct
{
   char             *pSubscription;  /* Subscription string.                   */
   uint8_t           fWildcard;      /* A flag indicating a wildcard           */
                                     /* subscription.                          */
} ismCluster_SubscriptionInfo_t;

/* A structure used to hold route lookup results.                              */
/* The structure is used by the Engine to obtain lookup results for a given    */
/* topic string                                                                */
typedef struct
{
   char             *pTopic;         /* Topic name on which to perform the     */
                                     /* lookup.                                */
   size_t            topicLen;       /* Length of pTopic (as returned by       */
                                     /* strlen).                               */
   ismEngine_RemoteServerHandle_t *phDests; /* Array to hold the lookup        */
                                     /* results of remote server handles.      */
   int               destsLen;       /* Length of the phDests array            */
   int               numDests;       /* On input: Number of destinations in    */
                                     /* the array already matched by the       */
                                     /* Engine (via the topic tree lookup).    */
                                     /* On output: Total number of             */
                                     /* destinations now in the  phDests array */
                                     /* phDests array.                         */
                                     /* If the lookup results is more than     */
                                     /* destsLen destinations numDests is set  */
                                     /* to -1 to indicate the lookup           */
                                     /* should be retried with a larger array. */
   ismCluster_RemoteServerHandle_t *phMatchedServers; /* Array that holds the  */
                                     /* Cluster handles for matched remote     */
                                     /* servers. The Cluster handles must be   */
                                     /* those corresponding to the Engine      */
                                     /* handles provided in phDests on input.  */
                                     /* The number of handles in the array is  */
                                     /* numDests.                              */
} ismCluster_LookupInfo_t;

/* A structure representing clustering data associated with a remote server or */
/* a local server.                                                             */
/* The Engine provides this structure to the Cluster during server recovery    */
/* to allow the Cluster to reconstruct the routing information associated with */
/* each remote server that was known to the server when it last ran.           */
/* The local server recovery record is used to ensure a monotonically          */
/* increasing incarnation number across restarts, store routing information,   */
/* and detect restart of the server with a different UID.                      */

typedef struct
{
   ismEngine_RemoteServerHandle_t hServerHandle; /* The server's Engine handle.*/
   char             *pRemoteServerName;          /* The remote server name.    */
   char             *pRemoteServerUID;           /* The remote server UID.     */
   const void       *pData;          /* A pointer to the remote server data.   */
   uint32_t          dataLength;     /* Data length in bytes.                  */
   ismCluster_RemoteServerHandle_t *phClusterHandle; /* Output: Returned       */
                                     /* Cluster remote server handle           */
   uint8_t           fLocalServer;   /* 0 if recored created with              */
                                     /* ENGINE_RS_CREATE, >0 if record created */
                                     /* with ENGINE_RS_CREATE_LOCAL.           */
} ismCluster_RemoteServerData_t;

/*********************************************************************/
/* Cluster Init                                                      */
/*                                                                   */
/* Initialize the Cluster component.                                 */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_init(void);

/*********************************************************************/
/* Cluster Start                                                     */
/*                                                                   */
/* Start up the Cluster component.                                   */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_start(void);

/*********************************************************************/
/* Cluster termination                                               */
/*                                                                   */
/* Bring down the Cluster component.                                 */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_term(void);

/*********************************************************************/
/* Allow Cluster to complete discovery before starting messaging     */
/*                                                                   */
/* This function blocks until Cluster exists discovery mode and the  */
/* server is ready to start messaging.                               */
/* Discovery time is configured to Cluster.DiscoveryTime             */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_startMessaging(void);

/*********************************************************************/
/* Recovery completed.                                               */
/*                                                                   */
/* The Engine calls this function after it performed recovery and    */
/* completed the following tasks:                                    */
/* 1. Restored all remote servers and called                         */
/*    ism_cluster_restoreRemoteServers.                              */
/* 2. Restored all local subscriptions and called                    */
/*    ism_cluster_addSubscription                                    */
/* 3. Engine is ready to receive remote server events.               */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_recoveryCompleted(void); 

/*********************************************************************/
/* Register new subscriptions.                                       */
/*                                                                   */
/* This function is used by the Engine to inform Cluster of new      */
/* subscriptions.                                                    */
/* The Engine maintains a reference count for its local              */
/* subscriptions and only calls this function when a subscription    */
/* is made the first time.                                           */
/* For efficiency the function receives an array of subscriptions so */
/* that multiple subscriptions could be added with one call.         */
/*                                                                   */
/* @param pSubInfo     Pointer to the subscription information array */
/* @param numSubs      Number of subscriptions in the pSubInfo array */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_addSubscriptions(const ismCluster_SubscriptionInfo_t *pSubInfo,
                                          int numSubs);

/*********************************************************************/
/* Removes existing subscriptions.                                   */
/*                                                                   */
/* This function is used by the Engine to inform Cluster that a      */
/* subscription has been removed.                                    */
/* The Engine maintains a reference count for its local subscription */
/* and only calls this function when the count for a subscription    */
/* goes down to zero.                                                */
/* For efficiency the function receives an array of subscriptions so */
/* that multiple subscriptions could be removed with one call.       */
/*                                                                   */
/* @param pSubInfo     Pointer to the subscription information array */
/* @param numSubs      Number of subscriptions in the pSubInfo array */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_removeSubscriptions(const ismCluster_SubscriptionInfo_t *pSubInfo,
                                             int numSubs);

/*********************************************************************/
/* Obtain route lookup results.                                      */
/*                                                                   */
/* This function is used by the Engine to obtain route information   */
/* for a specific topic, that is, the list of remote servers that    */
/* are subscribed to the topic.                                      */
/* The Engine is expected to allocate the phDests array and          */
/* indicate the array size in destsLen. On successful return the     */
/* Cluster provides the Engine handles of the remote servers in the  */
/* phDests array and sets numDests to indicate the number of         */
/* destinations.                                                     */
/* If the lookup results in more than destsLen destinations the      */
/* function returns ISMRC_ArrayTooSmall and the Engine should try    */
/* again with a larger phDest array.                                 */
/*                                                                   */
/* @param pLookupInfo   Pointer to the lookup information            */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_routeLookup(ismCluster_LookupInfo_t *pLookupInfo);

/*********************************************************************/
/* Restore servers information during server recovery.               */
/*                                                                   */
/* When the Engine performs recovery it constructs the information   */
/* about remote servers from data saved in the Store. Once recovery  */
/* is completed and all the clustering information is available the  */
/* Engine invokes this function to deliver the information to the    */
/* Cluster component in the pServersData array.                      */
/*                                                                   */
/* @param pServersData  Pointer to an array of structures each       */
/*                      holding the stored data of a single remote   */
/*                      server                                       */
/* @param numServers    Number of servers in the pServersData array  */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_restoreRemoteServers(const ismCluster_RemoteServerData_t *pServersData, 
                                              int numServers);

/*********************************************************************/
/* Set the forwarding information of the local server                */
/*                                                                   */
/* During Protocol start the local forwarding endpoint is created.   */
/* The Protocol component then provides Cluster the forwarding       */
/* information of the local server and Cluster delivers this         */
/* information to remote servers.                                    */
/*                                                                   */
/* @param pServerName    Pointer to the (display) server name        */
/* @param pServerUID     Pointer to the unique server ID             */
/* @param pServerAddress Pointer to the forwarding external IP       */
/*                       address used by the local server            */
/* @param serverPort     The external forwarding port                */
/* @param fUseTLS        A flag indicating whether TLS is used       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_setLocalForwardingInfo(const char *pServerName, const char *pServerUID,
                                                const char *pServerAddress, int serverPort, 
                                                uint8_t fUseTLS);

/*********************************************************************/
/* Indicate that a remote server is connected.                       */
/*                                                                   */
/* This is invoked by Forwarding to informs Cluster that the remote  */
/* server has been connected.                                        */
/*                                                                   */
/* @param phServerHandle  Cluster handle for the remote server       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_remoteServerConnected(const ismCluster_RemoteServerHandle_t phServerHandle);

/*********************************************************************/
/* Indicate that a remote server is disconnected.                    */
/*                                                                   */
/* This is invoked by Forwarding to inform Cluster that the remote   */
/* server has been disconnected.                                     */
/*                                                                   */
/* @param phServerHandle  Cluster handle for the remote server       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_remoteServerDisconnected(const ismCluster_RemoteServerHandle_t phServerHandle);

/*********************************************************************/
/* Monitoring and Control Interface                                  */
/*********************************************************************/

typedef enum
{
  /* Local Server States */
   ISM_CLUSTER_LS_STATE_INIT     = 1,  /* The Cluster component on the local   */
                                       /* server was initialized but not yet   */
                                       /* started                              */
   ISM_CLUSTER_LS_STATE_DISCOVER = 2,  /* The local server is in the initial   */
                                       /* state of discovering other remote    */
                                       /* servers in the cluster.              */
   ISM_CLUSTER_LS_STATE_ACTIVE   = 3,  /* The local server is an active member */
                                       /* of the cluster.                      */
   ISM_CLUSTER_LS_STATE_STANDBY  = 4,  /* The server has been initialized as a */
                                       /* standby node in an HA-Pair           */
   ISM_CLUSTER_LS_STATE_REMOVED  = 5,  /* The local server has been removed    */
                                       /* from the cluster.                    */
   ISM_CLUSTER_LS_STATE_ERROR    = 8,  /* The local server encountered a fatal */
                                       /* error and can not function.          */
   ISM_CLUSTER_LS_STATE_DISABLED = 9,  /* Cluster services are disabled.       */

   /* Remote Server States */
   ISM_CLUSTER_RS_STATE_ACTIVE      = 100, /* The remote server is an active   */
                                           /* member of the cluster with both  */
                                           /* control and forwarding connected */
   ISM_CLUSTER_RS_STATE_CONNECTING  = 101, /* The remote server is an active   */
                                           /* member of the cluster and is in  */
                                           /* the process of connecting.       */
   ISM_CLUSTER_RS_STATE_INACTIVE    = 102, /* The remote server is an inactive */
                                           /* member of the cluster. It is not */
                                            /* connected to the local server.  */
} ismCluster_State_t;

typedef enum
{
   ISM_CLUSTER_HEALTH_UNKNOWN    = 0,  /* Server health indicator is unknown  */
   ISM_CLUSTER_HEALTH_GREEN      = 1,  /* Server health indicator is green    */
   ISM_CLUSTER_HEALTH_YELLOW     = 2,  /* Server health indicator is yellow   */
   ISM_CLUSTER_HEALTH_RED        = 3   /* Server health indicator is red      */
} ismCluster_HealthStatus_t;

typedef enum
{
   ISM_CLUSTER_HA_UNKNOWN        = 0,  /* HA status is unknown                */
   ISM_CLUSTER_HA_DISABLED       = 1,  /* HA is disabled                      */
   ISM_CLUSTER_HA_PRIMARY_SINGLE = 2,  /* Primary without a sync standby      */
   ISM_CLUSTER_HA_PRIMARY_PAIR   = 3,  /* Primary with sync standby           */
   ISM_CLUSTER_HA_STANDBY        = 4,  /* Standby node                        */
   ISM_CLUSTER_HA_ERROR          = 9   /* Server in HA error state            */
} ismCluster_HaStatus_t;

/* A structure to hold Cluster statistics                                      */
typedef struct
{
  ismCluster_State_t state;            /* Current state of the local server    */
  ismCluster_HealthStatus_t  healthStatus; /* Local server health status       */
  ismCluster_HaStatus_t      haStatus; /* Local server HA status               */
   const char       *pClusterName;     /* Name of the cluster                  */
   const char       *pServerName;      /* Name of this local server            */
   const char       *pServerUID;       /* UID of this local server             */
   int               connectedServers; /* Number of connected remote servers   */
   int               disconnectedServers; /* Number of disconnected remote     */
                                       /* servers                              */
} ismCluster_Statistics_t;

/* A structure to hold view information for a remote server                    */
typedef struct
{
  ismCluster_RemoteServerHandle_t phServerHandle;  /* Remote server handle     */
  ismCluster_State_t state;            /* Current state of the remote server   */
  ism_time_t         stateChangeTime;  /* Time at which state was last changed */
  ismCluster_HealthStatus_t  healthStatus; /* Server health status             */
  ismCluster_HaStatus_t      haStatus; /* Server HA status                     */
   const char       *pServerName;      /* Name of the remote server            */
   const char       *pServerUID;       /* ServerUID of the remote server       */
} ismCluster_RSViewInfo_t;

/* A structure to hold view information for a remote server                    */
typedef struct
{
  ismCluster_RSViewInfo_t *pLocalServer;   /* Local server information         */
  ismCluster_RSViewInfo_t *pRemoteServers; /* An array to hold the information */
                                           /* of remote servers in the view    */
  int                   numRemoteServers;  /* Number of remote servers         */
} ismCluster_ViewInfo_t;

/*********************************************************************/
/* Remove a remote server from the cluster.                          */
/*                                                                   */
/* This function is used by the cluster admin to remove a remote     */
/* server from the cluster.                                          */
/* The local server on which this command is invoked must be an      */
/* active member of the cluster. The remote server can be either     */
/* connected or disconnected from the cluster.                       */
/*                                                                   */
/* @param phServerHandle  Cluster handle for the remote server       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_removeRemoteServer(const ismCluster_RemoteServerHandle_t phServerHandle);

/*********************************************************************/
/* Remove the local server from the cluster.                         */
/*                                                                   */
/* This function is used by the admin to remove the local server     */
/* from the cluster.                                                 */
/* The local server on which this command is invoked must be an      */
/* active member of the cluster.                                     */
/* After the local server is removed from the cluster it cannot      */
/* rejoin the cluster without performing server restart.             */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_removeLocalServer();

/*********************************************************************/
/* Get cluster statistics                                            */
/*                                                                   */
/* Provides local server Cluster related information                 */
/*                                                                   */
/* @param pStatistics     Pointer to the statistics structure        */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_getStatistics(const ismCluster_Statistics_t *pStatistics);

/*********************************************************************/
/* Get the current cluster view                                      */
/*                                                                   */
/* Provides information about remote servers in the cluster.         */
/* Cluster is responsible to allocate the view information and the   */
/* caller is expected to later free it by calling                    */
/* ism_cluster_freeView                                              */
/*                                                                   */
/* @param pView        Pointer to where the view information should  */
/*                     be written to by Cluster.                     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_getView(ismCluster_ViewInfo_t **pView);

/*********************************************************************/
/* Free the memory of a cluster view structure                       */
/*                                                                   */
/* This function is used to free the memory that was allocated by    */
/* Cluster when ism_cluster_getView was called.                      */
/*                                                                   */
/* @param pView        Pointer to the view structure to free         */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_freeView(ismCluster_ViewInfo_t *pView);

/*********************************************************************/
/* Sets the health status of the local server                        */
/*                                                                   */
/* The health status is distributed to other members in the cluster. */
/*                                                                   */
/* @param healthStatus New health status value to set                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_setHealthStatus(ismCluster_HealthStatus_t  healthStatus);

/*********************************************************************/
/* Sets the HA status of the local server                            */
/*                                                                   */
/* The HA status is distributed to other members in the cluster.     */
/*                                                                   */
/* @param haStatus New HA status value to set                        */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_setHaStatus(ismCluster_HaStatus_t haStatus);

/*********************************************************************/
/* Support for retained messages                                     */
/*********************************************************************/
/*********************************************************************/
/* Update retained statistics.                                       */
/*                                                                   */
/* This function is used by the Engine to update the retained stats  */
/* structure of the local server on retained messages received from  */
/* a server with specified UID.                                      */
/*                                                                   */
/* The Cluster keeps this information for a serverUID (which may or  */
/* may not represent a current cluster member) and sends it around   */
/* the cluster.                                                      */
/*                                                                   */
/* This data should replace any data already stored for the          */
/* specified serverUID.                                              */
/*                                                                   */
/* If the data pointer is NULL and the data length is zero,          */
/* the entry for the respective server UID will be removed.          */
/*                                                                   */
/* @param [in] pServerUID     ServerUID string                       */
/* @param [in] pData          Stats data to store for this server    */
/* @param [in] dataLength     Length of the stats data               */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_updateRetainedStats(const char *pServerUID,
                                             void *pData, uint32_t dataLength);

/* A structure representing retained stats information                         */
/* The structure is used by the Engine to determine which servers know the     */
/* most up-to-date retained messages from other servers.                       */
typedef struct
{
   char             *pServerUID;     /* Server UID for timestamp               */
   void             *pData;          /* Stats data for this server             */
   uint32_t          dataLength;     /* Length of the stats data               */
} ismCluster_RetainedStats_t;

/* A structure used to hold retained stats lookup results.                     */
/* The structure is used by the Engine to obtain lookup results for a given    */
/* remote server UID (for a given topic string)                                */
typedef struct
{
   ismCluster_RetainedStats_t *pStats; /* Array to hold the stats for a        */
                                       /* specified remote server UID.         */
   int               numStats;         /* Number of stats records in the       */
                                       /* pStats array.                        */
} ismCluster_LookupRetainedStatsInfo_t;

/*********************************************************************/
/* Retrieve a set of retained stats                                  */
/*                                                                   */
/* This function is used by the Engine to get an array of retained   */
/* stats data structures for a particular serverUID. The array lists */
/* other serverUID values and the current stats stored that the      */
/* requested server UID has kept.                                    */
/*                                                                   */
/* Cluster is responsible to allocate the lookup information and the */
/* Engine is expected to later free it by calling                    */
/* ism_cluster_freeRetainedStats                                     */
/*                                                                   */
/* If the serverUID is not found, the pointer to the lookup          */
/* information is set to NULL, and the return value is ISMRC_OK.     */
/*                                                                   */
/* @param pServerUID   ServerUID's array to be requested             */
/* @param pLookupInfo  Pointer to where the lookup information       */
/*                     should be written to by Cluster; or NULL      */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_lookupRetainedStats(const char *pServerUID,
                                             ismCluster_LookupRetainedStatsInfo_t **pLookupInfo);

/*********************************************************************/
/* Free the memory of a retained stats structure                     */
/*                                                                   */
/* This function is used by the Engine to free the memory that was   */
/* allocated by Cluster for a set of retained stats (during a call   */
/* to ism_cluster_lookupRetainedStats).                              */
/*                                                                   */
/* @param pLookupInfo  Pointer to the lookup structure to free       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_cluster_freeRetainedStats(ismCluster_LookupRetainedStatsInfo_t *pLookupInfo);

/*********************************************************************/
/* End of cluster.h                                                  */
/*********************************************************************/


#ifdef __cplusplus
}
#endif

#endif /* __CLUSTER_API_DEFINED */
