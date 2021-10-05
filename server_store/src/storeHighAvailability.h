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
/* Module Name: storeHighAvailability.h                              */
/*                                                                   */
/* Description: Store high-availability header file                  */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_STORE_HA_DEFINED
#define __ISM_STORE_HA_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <ha.h>

typedef unsigned char   ismStore_HANodeID_t[16];
typedef unsigned char   ismStore_HASessionID_t[10];
#ifndef ADDR_STR_LEN
#define ADDR_STR_LEN  64
#endif

typedef struct
{
   uint32_t             ViewId;
   ismHA_Role_t         NewRole;
   ismHA_Role_t         OldRole;
   uint16_t             ActiveNodesCount;
   ismHA_ViewReason_t   ReasonCode;       /* A reason code.
                                           * If it is not ISM_HA_REASON_OK then an error occurred   */
   const char          *pReasonParam;     /* In case of ISM_HA_REASON_CONFIG_ERROR, pReasonParam
                                           * points to the name of the mismatched configuration
                                           * parameter. The name is the one that appears in store.h
                                           * or ha.h, e.g., ismSTORE_CFG_GRANULE_SIZE.              */
   ismStore_HANodeID_t  ActiveNodeIds[2];
   char                 LocalReplicationNIC[ADDR_STR_LEN];  /* The address of the HA local replication NIC               */
   char                 LocalDiscoveryNIC[ADDR_STR_LEN];    /* The address of the HA local discovery NIC                 */
   char                 RemoteDiscoveryNIC[ADDR_STR_LEN];   /* The address of the HA remote discovery NIC                 */
   const char          *autoHAGroup;
} ismStore_HAView_t;

#define ismHA_CFG_IPV4_MCAST_ADDR "239.192.97.104"
#define ismHA_CFG_IPV6_MCAST_ADDR "ff18::6168"
#define ismHA_CFG_MCAST_TTL       1
#define ismHA_CFG_MCAST_INTERVAL  5e0

typedef struct
{
  const char  *Group ;
  const char  *Group_;
  const char  *ServerName ;
  const char  *NodesAddr ; 
  const char  *ReplicationNIC ;
  const char  *ExtReplicationNIC ;
  const char  *DiscoveryNIC ;
  const char  *McastAddr4 ;
  const char  *McastAddr6 ;
  char         autoHAGroup[ADDR_STR_LEN];
  uint8_t      gUpd[4] ; 
  int          EnableHA ;
  int          AutoConfig ; 
  int          StartupMode ; // 0,1 => AutoDetect, StandAlone
  int          PreferredPrimary ;
  int          AckingPolicy ;
  int          Port ;
  int          ExtPort ;
  int          haPort ;
  int          ExtHaPort ;
  int          DiscoveryTimeout ;
  int          HeartbeatTimeout ;
  int          ReplicationProtocol ;
  int          RecoveryMemStandbyMB ;
  int          SyncMemSizeMB ;
  int          LockedMemSizeMB ;
  int          SocketBufferSize ;
  int          AllowSingleNIC ;
  int          UseForkInit ;
  int          RoleValidation ;
  int          FlowControl ;
  int          DisableAutoResync ; 
  int          UseSecuredConnections ; 
  int          SplitBrainPolicy ; 
  int          RequireCertificates ;
} ismStore_HAConfig_t;

/*********************************************************************/
/* Callback function and arguments used to inform the store at the   */
/* Standby server that a channel has been closed by the primary      */
/* server.                                                           */
/*                                                                   */
/* @param hChannel    Channel handle                                 */
/* @param pChContext  Context provided in ismStore_HAChParameters_t  */
/*                                                                   */
/* @return StoreRC_OK on successful completion or an            */
/* ismSTOREHARC_ value on failure.                                   */
/*********************************************************************/
typedef int (*ismPSTOREHACHCLOSED)(void *hChannel, void *pChContext);

/*********************************************************************/
/* Callback function and arguments used to inform the store at the   */
/* Standby server that a new message has been received on a          */
/* specified channel.                                                */
/*                                                                   */
/* @param hChannel    Channel handle                                 */
/* @param pData       Pointer to the buffer of the data. When the    */
/* callback returns, the buffer can be re-used by the caller.        */
/* @param dataLength  Data length                                    */
/* @param pChContext  Context provided in ismStore_HAChParameters_t  */
/*                                                                   */
/* @return StoreRC_OK on successful completion or an            */
/* StoreRC_HA_CloseChannel if the channel should be closed        */
/* gracefully or an ismSTOREHARC_ value on failure.                  */
/*********************************************************************/
typedef int (*ismPSTOREHAMSGRECEIVED)(void *hChannel,
                                      char *pData,
                                      uint32_t dataLength,
                                      void *pChContext);

typedef struct
{
   uint8_t                 fMultiSend;         /* Indicates if multiple threads can send data on
                                                * the channel                                      */
   char                    ChannelName[16];    /* Channel name                                     */
   ismPSTOREHACHCLOSED     ChannelClosed;      /* ChannelClosed callback                           */
   ismPSTOREHAMSGRECEIVED  MessageReceived;    /* MessageReceived callback                         */
   void                   *pChContext;         /* Context to associate with the channel callbacks  */
} ismStore_HAChParameters_t;

/*********************************************************************/
/* Callback function and arguments used to inform the store that the */
/* view of the nodes has been changed.                               */
/*                                                                   */
/* @param pView       View information                               */
/* @param pContext    Context provided in ismStore_HAParameters_t    */
/*                                                                   */
/* @return StoreRC_OK on successful completion or an            */
/* ismSTOREHARC_ value on failure.                                   */
/*********************************************************************/
typedef int (*ismPSTOREHAVIEWCHANGED)(ismStore_HAView_t *pView,
                                      void *pContext);

/*********************************************************************/
/* Callback function and arguments used to inform the store at the   */
/* Standby server that a new channel has been created by the primary */
/* server.                                                           */
/*                                                                   */
/* @param channelId   Channel Id                                     */
/* @param hChannel    Channel handle                                 */
/* @param pChParams   Channel parameters to associate with this      */
/* channel (output)                                                  */
/* @param pContext    Context provided in ismStore_HAParameters_t    */
/*                                                                   */
/* @return StoreRC_OK on successful completion or an            */
/* ismSTOREHARC_ value on failure.                                   */
/*********************************************************************/
typedef int (*ismPSTOREHACHCREATED)(int32_t channelId,
                                    uint8_t flags,
                                    void *hChannel,
                                    ismStore_HAChParameters_t *pChParams,
                                    void *pContext);

typedef struct
{
   ismPSTOREHAVIEWCHANGED  ViewChanged;        /* ViewChanged callback                             */
   ismPSTOREHACHCREATED    ChannelCreated;     /* ChannelCreated callback (Standby only)           */
   void                   *pContext;           /* Context to associate with the callbacks          */
} ismStore_HAParameters_t;

/*********************************************************************/
/* Store HA Init                                                     */
/*                                                                   */
/* Performs store HA init tasks                                      */
/*                                                                   */
/* @param pHAParameters   Store HA parameters                        */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_init(ismStore_HAParameters_t *pHAParameters);

/*********************************************************************/
/* Store HA Start                                                    */
/*                                                                   */
/* Performs store HA start tasks                                     */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_start(void);

/*********************************************************************/
/* Store HA Shutdown                                                 */
/*                                                                   */
/* Performs shutdown tasks                                           */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_term(void);

/*********************************************************************/
/* Create Channel                                                    */
/*                                                                   */
/* Creates a channel for sending and receiving messages.             */
/*                                                                   */
/* @param channelId     Channel Id                                   */
/* @param flags         Flags                                        */
/* @param phChannel     Handle of newly opened channel               */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_createChannel(int32_t channelId, uint8_t flags, void **phChannel);

/*********************************************************************/
/* Close Channel                                                     */
/*                                                                   */
/* Closes a channel.                                                 */
/*                                                                   */
/* @param hChannel      Channel handle                               */
/* @param fPending      If set (value != 0) the channel is marked as */
/*                      close pending, and will wait for an Ack from */
/*                      the standby node. Any error on this channel  */
/*                      will not cause a viewChanged.                */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_closeChannel(void *hChannel, uint8_t fPending);

/*********************************************************************/
/* Return the transmit queue depth of the channel                    */
/*                                                                   */
/* @param hChannel      Channel handle                               */
/* @param pTxQDepth     Pointer to the transmit queue depth          */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_getChannelTxQDepth(void *hChannel, uint32_t *pTxQDepth);

/*********************************************************************/
/* Allocate Buffer for sending a message                             */
/*                                                                   */
/* Returns a buffer from the underlying transport pool, in which the */
/* data to be sent will be copied.                                   */
/*                                                                   */
/* @param hChannel      Channel handle                               */
/* @param pBuffer       Pointer to the allocated buffer (output)     */
/* @param pBufferLength Pointer to the buffer length (output)        */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_allocateBuffer(void *hChannel,
                               char **pBuffer,
                               uint32_t *pBufferLength);

/*********************************************************************/
/* Send Message                                                      */
/*                                                                   */
/* Sends a message on a specified channel.                           */
/*                                                                   */
/* @param hChannel      Channel handle                               */
/* @param pData         Pointer to the buffer of the data. After the */
/*                      method returns, the buffer is no longer used */
/*                      by the caller (can be re-used).              */
/* @param dataLength    Data length                                  */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_sendMessage(void *hChannel,
                            char *pData,
                            uint32_t dataLength);

/*********************************************************************/
/* Receive Message                                                   */
/*                                                                   */
/* Receives a message on a specified channel.                        */
/* If no messages are available at the channel, the receive call     */
/* wait for a message to arrive, unless the fNonBLocking is set.     */
/*                                                                   */
/* @param hChannel      Channel handle                               */
/* @param pData         Pointer to the buffer of the data (output)   */
/* @param pDataLength   Pointer to the data length (output)          */
/* @param fNonBlocking  Indicates that the receive is non-blocking   */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_receiveMessage(void *hChannel,
                               char **pData,
                               uint32_t *pDataLength,
                               uint8_t fNonBlocking);

/*********************************************************************/
/* Return Buffer to the transport pool                               */
/*                                                                   */
/* Returns a buffer which was received by ism_storeHA_receiveMessage */
/* to the underlying transport pool.                                 */
/*                                                                   */
/* @param hChannel      Channel handle                               */
/* @param pBuffer       Pointer to the buffer                        */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_returnBuffer(void *hChannel, char *pBuffer);

/*********************************************************************/
/* Unsync a running SB and set to Error                              */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                    */
/*********************************************************************/
int ism_storeHA_sbError(void);

/*********************************************************************/
/* Return the HA configuration                                       */
/*********************************************************************/
const ismStore_HAConfig_t *ism_storeHA_getConfig(void);

/*********************************************************************/
/* Return a new HASessionId                                          */
/*                                                                   */
/*                                                                   */
/* @param pHASessionId  Pointer to a buffer of 10 bytes where the    */
/*                      new HASessionId will be copied (output)      */
/*                                                                   */
/* @return A return code, StoreRC_OK=success                         */
/*********************************************************************/
int ism_storeHA_allocateSessionId(unsigned char *pHASessionId);


int ism_storeHA_pollOnAllChanns(int milli);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_STORE_HA_DEFINED */

/*********************************************************************/
/* End of storeHighAvailability.h                                    */
/*********************************************************************/
