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

/*********************************************************************/
/*                                                                   */
/* Module Name: ha.h                                                 */
/*                                                                   */
/* Description: Store High Availability external header file         */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_HA_DEFINED
#define __ISM_HA_DEFINED

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************/
/* HA External Configuration                                                  */
/******************************************************************************/
/* HA.EnableHA                                                                 *
 * Defines whether High Availability (HA) is enabled or disabled.              *
 *                                                                             *
 * A value of 0 indicates HA is disabled.                                      *
 * A value of 1 indicates HA is enabled.                                       *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismHA_CFG_ENABLEHA               "HA.EnableHA"

/* HA.StartupMode                                                              *
 * The high availability mode at which this node of the ISM server should      *
 * start. Possible values are                                                  *
 * AutoDetect                                                                  *
 * StandAlone                                                                  *
 *                                                                             *
 * AutoDetect means that at start up the nodes of the HA-Pair detect one       *
 * another and automatically establish the HA-Pair. If the node cannot detect  *
 * the other node in the HA-Pair it will not start.                            *
 *                                                                             *
 * StandAlone means that the node will attempt to start as a stand alone       *
 * Primary node. However, an additional node could be started later to form an *
 * HA-Pair. If while starting up another node is detected the node will not    *
 * be able to start.                                                           *
 *                                                                             *
 * The type of the parameter is String.                                        *
 * The default value is AutoDetect                                             */
#define ismHA_CFG_STARTUPMODE            "HA.StartupMode"

/* HA.PreferredPrimary                                                         *
 * Provides a hint to prefer setting this node to act as a Primary in case     *
 * the two nodes in the HA-Pair are equally eligible to act as Primary.        *
 *                                                                             *
 * A value of 0 indicates no preference                                        *
 * A value of 1 indicates preference for acting as Primary                     *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismHA_CFG_PREFERREDPRIMARY       "HA.PreferredPrimary"

/* HA.RemoteDiscoveryNIC                                                       *
 * The IPv4 address, IPv6 address, or DNS name of the discovery interface on   *
 * the remote node in the high availability pair.                              *
 * To use automatic discovery set this parameter to an empty string (default)  *
 *                                                                             *
 * The type of the parameter is String.                                        *
 * The default value is empty string (indicating automatic discovery).         */
#define ismHA_CFG_REMOTEDISCOVERYNIC     "HA.RemoteDiscoveryNIC"

/* HA.LocalReplicationNIC                                                      *
 * The IP address of the NIC that should be used for HA replication.           *
 * To use automatic discovery set this parameter to an empty string (default)  *
 *                                                                             *
 * The type of the parameter is String.                                        *
 * The default value is empty string (indicating automatic discovery).         */
#define ismHA_CFG_LOCALREPLICATIONNIC    "HA.LocalReplicationNIC"

/* HA.LocalDiscoveryNIC                                                        *
 * The IP address of the NIC that is used for HA discovery and secondary       *
 * heartbeat channel.                                                          *
 * To use automatic discovery set this parameter to an empty string (default)  *
 *                                                                             *
 * The type of the parameter is String.                                        *
 * The default value is empty string (indicating automatic discovery).         */
#define ismHA_CFG_LOCALDISCOVERYNIC      "HA.LocalDiscoveryNIC"

/* HA.DiscoveryTimeout                                                         *
 * When a node is started with HA enabled and AutoDetect StartupMode the node  *
 * will attempt to communicate with the other node in the HA-Pair for          *
 * HA.DiscoveryTimeout seconds. If no other node is found after                *
 * HA.DiscoveryTimeout seconds the node will fail to start.                    *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 600.                                                   */
#define ismHA_CFG_DISCOVERYTIMEOUT       "HA.DiscoveryTimeout"

/* HA.HeartbeatTimeout                                                         *
 * The heartbeat timeout in seconds used to detect that the other node in the  *
 * HA-Pair has failed.                                                         *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 10.                                                    */
#define ismHA_CFG_HEARTBEATTIMEOUT       "HA.HeartbeatTimeout"

/* HA.Group                                                                    *
 * An identifier to define nodes that may establish an HA-Pair together.       *
 * Only nodes with the same Group may form an HA-Pair.                         *
 * By default Group is empty meaning that the node may form an HA-Pair only    *
 * with another node that did not define a Group. In such a case after the     *
 * HA-Pair is established the Store will generate a unique Group identifier    *
 * for the two nodes and save it in the configuration.                         *
 *                                                                             *
 * The type of the parameter is String.                                        *
 * The default value is an empty string which means an undefined Group.        */
#define ismHA_CFG_GROUP                  "HA.Group"  

/* HA.UseSecuredConnections                                                    *
 * Indicates whether the connections between the nodes of the HA-Pair should   *
 * be secured.                                                                 *
 *                                                                             *
 * The type of the parameter is uint_8.                                        *
 * The default value is 0 (false) indicating secured connections are not used. */
#define ismHA_CFG_USE_SECUREDCONN        "HA.UseSecuredConnections"

/* HA.RequireCertificates                                                      *
 * Indicates whether the connections between the nodes of the HA-Pair should   *
 * require a certificates to be passed.                                        *
 *                                                                             *
 * The type of the parameter is uint_8.                                        *
 * The default value is 1 (true) indicating certificates will be required.     */
#define ismHA_CFG_REQUIRE_CERTIFICATES   "HA.RequireCertificates"

/******************************************************************************/
/* HA support for address/port mapping (NAT)                                  */
/******************************************************************************/
/* HA.ExternalReplicationNIC                                                   *
 * The external IP address of the NIC that should be used for HA replication.  *
 *                                                                             *
 * If not set defaults to HA.LocalReplicationNIC.                              *
 * This parameter is ignored if automatic discovery is enabled.                *
 *                                                                             *
 * The type of the parameter is String.                                        *
 * The default value is empty string (use the value of HA.LocalReplicationNIC) */
#define ismHA_CFG_EXTERNALREPLICATIONNIC "HA.ExternalReplicationNIC"

/* HA.RemoteDiscoveryPort                                                      *
 * The port number used for HA discovery on the remote node in the HA-Pair.    *
 * Range 1024 - 65535                                                          *
 *                                                                             *
 * If not set or set to 0 (default) the value of HA.DiscoveryPort is used.     *
 * This parameter is ignored if automatic discovery is enabled.                *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 0 (value of HA.DiscoveryPort is used).                 */
#define ismHA_CFG_REMOTEDISCOVERYPORT    "HA.RemoteDiscoveryPort"

/* HA.ReplicationPort                                                          *
 * The port number used for HA replication on the local node in the HA-Pair.   *
 * Range 1024 - 65535                                                          *
 *                                                                             *
 * If not set or set to 0 (default) a free port is selected automatically.     *
 * This parameter is ignored if automatic discovery is enabled.                *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value 0 (automatically select a random port).                   */
#define ismHA_CFG_RPLICATIONPORT         "HA.ReplicationPort"

/* HA.ExternalReplicationPort                                                  *
 * The external port number used for HA replication on the local node in the   *
 * HA-Pair. When this parameter is set HA.ReplicationPort must also be set.    *
 * Range 1024 - 65535                                                          *
 *                                                                             *
 * If not set or set to 0 (default) the value of HA.ReplicationPort is used.   *
 * This parameter is ignored if automatic discovery is enabled.                *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value 0 (value of HA.ReplicationPort is used).                  */
#define ismHA_CFG_EXTERNALRPLICATIONPORT "HA.ExternalReplicationPort"

/******************************************************************************/
/* HA Internal Configuration                                                  */
/******************************************************************************/
/* HA.AckingPolicy                                                             *
 * Defines the time at which the standby node can send an ACK to the primary.  *
 * The options are either to ACK immediately after the data has been received  *
 * from the primary or only after the data has been written to NVRAM in the    *
 * standby.                                                                    *
 * A value of 0 indicates ACK after data is received.                          *
 * A value of 1 indicates ACK after data is written to NVRAM                   *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 1                                                      */
#define ismHA_CFG_ACKINGPOLICY           "HA.AckingPolicy"

/* HA.DiscoveryPort                                                            *
 * Port number to be used for high availability control communication.         *
 * Range 1024 - 65535                                                          *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 9084.                                                  */
#define ismHA_CFG_PORT                   "HA.DiscoveryPort"

/* HA.ReplicationProtocol                                                      *
 * The protocol to use for HA replication. Options are TCP or RDMA RC.         *
 * Note that RDMA RC only works on the physical appliance.                     *
 * A value of 0 indicates RDMA RC protocol.                                    *
 * A value of 1 indicates TCP protocol.                                        *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 1.                                                     */
#define ismHA_CFG_REPLICATIONPROTOCOL    "HA.ReplicationProtocol"

/* HA.RecoveryMemStandbyMB                                                     *
 * The maximal amount of recovery data in Megabytes that a standby node may    *
 * hold in main memory.                                                        *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 80000.                                                 */
#define ismHA_CFG_RECOVERYMEMSTANDBYMB   "HA.RecoveryMemStandbyMB"

/* HA.LockedMemSizeMB                                                          *
 * The amount in Megabytes of locked memory that each RDMA connection may use. *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 16.                                                    */
#define ismHA_CFG_LOCKEDMEMSIZEMB        "HA.LockedMemSizeMB"

/* HA.SocketBufferSize                                                         *
 * The socket buffer size (in bytes) to use for HA TCP connections.            *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 1M.                                                    */
#define ismHA_CFG_SOCKETBUFFERSIZE       "HA.SocketBufferSize"

/* HA.SyncMemSizeMB                                                            *
 * The maximal amount of data in Megabytes used for the new node               *
 * synchronization procedure that a node may hold in main memory.              *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 8192 (8GB)                                             */
#define ismHA_CFG_SYNCMEMSIZEMB          "HA.SyncMemSizeMB"

/* HA.AllowSingleNIC                                                           *
 * For testing purposes allows LocalDiscoveryNIC and LocalReplicationNIC       *
 * to point to the same NIC.                                                   *
 * A value of 0 indicates two different NICs must be used.                     *
 * A value of 1 indicates a single NIC may be used.                            *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismHA_CFG_ALLOWSINGLENIC         "HA.AllowSingleNIC"

/* HA.UseForkInit                                                              *
 * A value of 0 indicates NOT to call ibv_fork_init().                         *
 * A value of 1 indicates     to call ibv_fork_init().                         *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 1.                                                     */
#define ismHA_CFG_USEFORKINIT            "HA.UseForkInit"

/* HA.RoleValidation                                                           *
 * Defines whether HA role validation is enabled or disabled.                  *
 * If HA RoleValidation is enabled, the store verifies that the content was    *
 * created by a primary node, otherwise the ism_store_start API fails with an  *
 * ISMRC_StoreUnrecoverable error code.                                        *
 *                                                                             *
 * A value of 0 indicates HA startup validation is disabled.                   *
 * A value of 1 indicates HA startup validation is enabled.                    *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 1.                                                     */
#define ismHA_CFG_ROLEVALIDATION         "HA.RoleValidation"

/* HA.FlowControl                                                              *
 * Defines whether HA flow control is enabled or disabled.                     *
 * A value of 0 indicates that the flow control is disabled.                   *
 * A value of 1 indicates that the flow control is enabled.                    *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 1.                                                     */
#define ismHA_CFG_FLOWCONTROL            "HA.FlowControl"

/* HA.DisableAutoResync                                                        *
 * This flag is used to disable a server that is automatically restarted from  *
 * synchronizing (as a Standby) with a running Primary node.                   *
 * The flag is expected to be set only if two conditions are met               *
 *  1. The policy is to prevent the server from resynchronizing automatically. *
 *  2. The server was automatically restarted (information provided by Admin). *
 * If the flag is set the Store will prevent this node from synchronizing with *
 * an existing node that is already acting as a Primary.                       *
 * A value of 0 indicates that (re)synchronization is allowed.                 *
 * A value of 1 indicates that (re)synchronization is not allowed.             *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismHA_CFG_DISABLEAUTORESYNC      "HA.DisableAutoResync"

/* HA.SplitBrainPolicy                                                         *
 * The split-brain policy defines the actions that should be taken when nodes  *
 * in an HA-Pair detect a split-brain situation (i.e., both nodes are acting   *
 * as primary).                                                                *
 * In case of a mismatch in the policies on the two nodes the default policy   *
 * (stop both servers) is used.
 * The supported policies are the following:                                   *
 * 0 - Stop both servers (current implementation)                              *
 * 1 - Keep the server with the largest number of connections running and      *
 *     restart the other server with a clean store (will become standby)       *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismHA_CFG_SPLITBRAINPOLICY       "HA.SplitBrainPolicy"

/* HA.NumResyncAttemps                                                         *
 * Number of time to retry to sync in case a sync attempt fails on the standby *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 2.                                                     */
#define ismHA_CFG_NUMRESYNCATTEMPS       "HA.NumResyncAttemps"

/* HA.SyncPolicy                                                               *
 * Defines the action to take in case a standby fails to synchronize           *
 * A value of 0 means go to maintenance                                        *
 * A value of 1 means kill the server                                          *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismHA_CFG_SYNCPOLICY             "HA.SyncPolicy"

/****************************************************************************************************/
/* HA Store-Admin interface                                                                         */
/****************************************************************************************************/

typedef enum
{
   ISM_HA_ROLE_UNSYNC         = 0,        /* Node is out of synchronization with the Primary.
                                           * An attempt will be made to synchronize the node with
                                           * the primary                                            */
   ISM_HA_ROLE_PRIMARY        = 1,        /* Node is acting as the Primary                          */
   ISM_HA_ROLE_STANDBY        = 2,        /* Node is acting as a Standby                            */
   ISM_HA_ROLE_TERM           = 3,        /* Node terminated by the Primary (coordinated shutdown)  */
   ISM_HA_ROLE_ERROR          = 90,       /* Node got out of synchronization due to a fatal error.
                                           * NO attempt will be made to synchronize the node again.
                                           * Store can no longer function                           */
   ISM_HA_ROLE_DISABLED       = 99        /* High-Availability mode is disabled                     */
} ismHA_Role_t;

typedef enum
{
   ISM_HA_REASON_OK           = 0,        /* The view is OK.                                        */
   ISM_HA_REASON_CONFIG_ERROR = 1,        /* The Store configurations of the two nodes does not 
                                           * allow them to work as an HA-Pair. 
                                           * In this case, the name of the mismatched configuration  
                                           * parameter is stored in pReasonParam. The name is taken   
                                           * from the parameters that appear in store.h or ha.h, 
                                           * e.g., ismSTORE_CFG_GRANULE_SIZE.                       */
   ISM_HA_REASON_DISC_TIMEOUT = 2,        /* Discovery time expired. Failed to communicate with
                                           * the other node in the HA-pair.                         
                                           * In case of automatic discovery, the Group identifier 
                                           * of the last remote node that was discovered is stored 
                                           * in pReasonParam.                                       */
   ISM_HA_REASON_SPLIT_BRAIN  = 3,        /* Two Primary nodes have been identified (split-brain).
                                           * The node should be terminated.                         */
   ISM_HA_REASON_SYNC_ERROR   = 4,        /* Two unsynchronized nodes have been identified.
                                           * The node should be terminated.                         */
   ISM_HA_REASON_UNRESOLVED   = 5,        /* The two nodes have non-empty Stores on startup.
                                           * Unable to resolve which Store should be used.          */
   ISM_HA_REASON_NO_RESYNC    = 6,        /* The node was started with HA.DisableAutoResync
                                           * and attempted to join a running Primary.               */
   ISM_HA_REASON_DIFF_GROUP   = 7,        /* The node failed to pair with the remote node since the 
                                           * two nodes have a different Group identifier.
                                           * In this case, the Group identifier of the remote node
                                           * is stored in pReasonParam.                             */
   ISM_HA_REASON_SPLIT_BRAIN_RESTART = 8, /* Two Primary nodes have been identified (split-brain).
                                           * The node should be restarted.                          */
   ISM_HA_REASON_SYSTEM_ERROR = 9         /* A High-Availability system or internal error occurred. 
                                           * The node should be terminated.                         */
} ismHA_ViewReason_t;

typedef struct
{
   ismHA_Role_t       NewRole;            /* Current role of this node                              */
   ismHA_Role_t       OldRole;            /* Role of this node before the view changed              */
   uint16_t           ActiveNodesCount;   /* Number of active nodes in the HA-Pair                  */
   uint16_t           SyncNodesCount;     /* Number of synchronized nodes in the HA-Pair            */
   ismHA_ViewReason_t ReasonCode;         /* A reason code.
                                           * If it is not ISM_HA_REASON_OK then an error occurred   */
   const char        *pReasonParam;       /* In case of ISM_HA_REASON_[CONFIG_ERROR| DISC_TIMEOUT|
                                           * DIFF_GROUP], pReasonParam points to additional 
                                           * information as described in the description of the 
                                           * respective ISM_HA_REASON definitions.                  */
   const char       *LocalReplicationNIC;  /* The address of the HA local replication NIC           */
   const char       *LocalDiscoveryNIC;    /* The address of the HA local discovery NIC             */
   const char       *RemoteDiscoveryNIC;   /* The address of the HA remote discovery NIC            */
} ismHA_View_t;

/* The maximum length in bytes of an Admin response data                       */
#define ismHA_MAX_ADMIN_RESPONSE_LENGTH   10240

typedef struct
{
   const char        *pData;              /* Pointer to the buffer of the data                      */
   uint32_t           DataLength;         /* Number of bytes in the data buffer (pData)             */
   char              *pResBuffer;         /* Pointer to the buffer where the response data will be
                                           * copied. The buffer is allocated/freed by the caller.   */
   uint32_t           ResBufferLength;    /* Size in bytes of the response buffer (pResBuffer)      */
   uint32_t           ResLength;          /* Length in bytes of the response data length (output). 
                                           * The length must not exceed ResBufferLength and
                                           * ismHA_MAX_ADMIN_RESPONSE_LENGTH (whichever is smaller).*/
} ismHA_AdminMessage_t;

/*********************************************************************/
/* View changed information                                          */
/*                                                                   */
/* Called by the Store to notify the Admin that the HA-Pair status   */
/* has changed.                                                      */
/* The changes that require actions are                              */
/* 1. Standby becomes Primary (oldRole=STANDBY newRole=PRIMARY)      */
/*    Action: Perform Standby to Primary procedure                   */
/* 2. Node enters ERROR state (newRole=ERROR)                        */
/*    Action: Terminate the Store (Store can no longer function)     */
/* 3. Primary detects a new sync Standby (oldRole=newRole=PRIMARY    */
/*    ActiveNodesCount=2 SyncNodesCount=2)                           */
/*    Action: Start performing Admin replicating to the Standby      */
/* 4. Primary detects Standby failed (oldRole=newRole=PRIMARY        */
/*    ActiveNodesCount=1 SyncNodesCount=1)                           */
/*    Action: Stop performing Admin replicating to the Standby       */
/* 5. Standby terminated by Primary (oldRole=STANDBY newRole=TERM)   */
/*    Action: Terminate the ISM server (including the Store)         */
/*                                                                   */
/* @param view          New view information                         */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
int ism_ha_admin_viewChanged(ismHA_View_t *pView);

/*********************************************************************/
/* Admin instructed to transfer state                                */
/*                                                                   */
/* Called by the Store to notify the Admin that it should transfer   */
/* its state as part of the a new node synchronization procedure.    */
/* This call is also used to indicate that the Store established the */
/* Admin channel and the Admin can start using the SPI calls for     */
/* replicating Admin data.                                           */
/* In case the Store generated the Group name for the HA-Pair the    */
/* Group name is provided in the generatedGroup parameter. If the    */
/* Store did not generate the Group name generatedGroup is NULL      */
/* This function is invoked on the Primary node.                     */
/*                                                                   */
/* @param generatedGroup   Name of the Group generated by the Store  */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
int ism_ha_admin_transfer_state(const char *generatedGroup);

/*********************************************************************/
/* Report state transfer completed                                   */
/*                                                                   */
/* Called by the Admin to notify the Store that it completed         */
/* transferring its state as part of the new node synchronization    */
/* procedure.                                                        */
/* This function is invoked on the Primary node.                     */
/* After ism_ha_admin_transfer_state has been called the Admin must  */
/* call ism_ha_store_transfer_state_completed to allow the new node  */
/* synchronization procedure to complete                             */
/*                                                                   */
/* @param adminRC       Return code from admin sync functions        */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
int ism_ha_store_transfer_state_completed(int adminRC);

/*********************************************************************/
/* Send Admin message                                                */
/*                                                                   */
/* Called by the Admin to send a message from the Primary node to    */
/* the Standby node. The send is synchronous, which means that the   */
/* function is blocked until the message is received and processed   */
/* by the Standby node.                                              */
/* Once the function returns successfully, the response buffer       */
/* (pResBuffer) may contain a response from the Standby node.        */
/* The function can only be used on the Primary node.                */
/*                                                                   */
/* @param pAdminMsg     Pointer to the Admin message structure       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
int ism_ha_store_send_admin_msg(ismHA_AdminMessage_t *pAdminMsg);

/*********************************************************************/
/* Process Admin message                                             */
/*                                                                   */
/* Called by the Store on the Standby node to deliver an Admin       */
/* message that was received from the Primary node of the HA-Pair.   */
/*                                                                   */
/* The Standby node can send back a response to the Primary node by  */
/* putting the response data in the pResBuffer and setting the       */
/* ResLength to the response data length.                            */
/* Note that the maximum length of the response is bounded by the    */
/* ismHA_MAX_ADMIN_RESPONSE_LENGTH constant value.                   */
/*                                                                   */
/* Once the function returns, an ACK (including the response data)   */
/* is sent back to the Primary node; the ACK releases the            */
/* ism_ha_store_send_admin_msg function casing it to return.         */
/* The pAdminMsg should not be accessed after the function returns.  */
/*                                                                   */
/* @param pAdminMsg     Pointer to the Admin message structure       */
/*********************************************************************/
void ism_ha_admin_process_admin_msg(ismHA_AdminMessage_t *pAdminMsg);

/*********************************************************************/
/* Transfer Admin file                                               */
/*                                                                   */
/* Called by the Admin to transfer a file from the Primary node to   */
/* the Standby node. The transfer is synchronous, which means that   */
/* the function is blocked until the file is fully stored on the     */
/* Standby disk.                                                     */
/* The function can only be used on the Primary node.                */
/*                                                                   */
/* @param pPath               Path of the file to transfer           */
/* @param pFilename           Name of the file to transfer           */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
int ism_ha_store_transfer_file(char *pPath, char *pFilename);

/*********************************************************************/
/* Get HA view of the node                                           */
/*                                                                   */
/* Called by the Admin to get the current HA view of the node.       */
/* NewRole indicates the current role of the node.                   */
/*                                                                   */
/* @param pView  Pointer to the view (output)                        */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
int ism_ha_store_get_view(ismHA_View_t *pView);

/*********************************************************************/
/* Controlled termination of both nodes                              */
/*                                                                   */
/* Controlled termination of both nodes in the HA-Pair.              */
/* Used to allow the two nodes to start later without having to      */
/* synchronize their state.                                          */
/* The function can only be used on the Primary node.                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
int ism_ha_store_term(void);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_HA_DEFINED */

/*********************************************************************/
/* End of ha.h                                                       */
/*********************************************************************/
