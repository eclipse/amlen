/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

//****************************************************************************
/// @file  exportClientState.h
/// @brief Functions to export clientState information
//****************************************************************************
#ifndef __ISM_EXPORTCLIENTSTATE_DEFINED
#define __ISM_EXPORTCLIENTSTATE_DEFINED

#include "engineInternal.h"
#include "engineHashTable.h"
#include "exportResources.h"
#include "exportCrypto.h"

//****************************************************************************
/// @brief Information exported about a client state.
///
/// @remark Following after this structure in an exported file will be, in order:
///
/// - char[]      ClientId                  - Client Identifier for this clientState
/// - char[]      UserId                    - UserId associated with this clientState
/// - char[]      WillTopic                 - Will topic (implies WillMsgId != 0)
/// - uint32_t[]  Unreleased Delivery Ids   - UMSCount *UNALIGNED* delivery Ids
///
/// ClientIdLength should be non-zero, if any other lengths are zero, the field
/// is ommitted.
///
/// Character arrays (and lengths) include a NULL terminator.
///
/// Assume that unreleased delivery Ids are *NOT* aligned.
//****************************************************************************
typedef struct tag_ieieClientStateInfo_t
{
    uint32_t                    Version;              ///< The version of clientState import/export information
    bool                        Durable;              ///< Is this a durable client state?
    uint32_t                    ClientIdLength;       ///< Length of optional client ID
    uint32_t                    ProtocolId;           ///< Protocol identifier
    uint32_t                    LastConnectedTime;    ///< Last connected time (in ExpireTime form)
    uint32_t                    UserIdLength;         ///< Length of associated user-id
    uint64_t                    WillMsgId;            ///< Will message identifier (pointer when exported)
    uint32_t                    WillMsgTimeToLive;    ///< Will Message time to live
    uint32_t                    WillTopicNameLength;  ///< Length of will-topic name
    uint32_t                    UMSCount;             ///< Count of unreleased message states
    // Version 2+
    uint32_t                    ExpiryInterval;       ///< Expiry interval for this clientState
    // Version 3+
    uint32_t                    WillDelay;            ///< Will Delay for this clientState
} ieieClientStateInfo_t;

#define ieieCLIENTSTATE_VERSION_1           1
#define ieieCLIENTSTATE_VERSION_2           2
#define ieieCLIENTSTATE_VERSION_3           3
#define ieieCLIENTSTATE_CURRENT_VERSION     ieieCLIENTSTATE_VERSION_3

/// @brief Version 1 of the exported client state information
typedef struct tag_ieieClientStateInfo_V1_t
{
    uint32_t                    Version;              ///< The version of clientState import/export information
    bool                        Durable;              ///< Is this a durable client state?
    uint32_t                    ClientIdLength;       ///< Length of optional client ID
    uint32_t                    ProtocolId;           ///< Protocol identifier
    uint32_t                    LastConnectedTime;    ///< Last connected time (in ExpireTime form)
    uint32_t                    UserIdLength;         ///< Length of associated user-id
    uint64_t                    WillMsgId;            ///< Will message identifier (pointer when exported)
    uint32_t                    WillMsgTimeToLive;    ///< Will Message time to live
    uint32_t                    WillTopicNameLength;  ///< Length of will-topic name
    uint32_t                    UMSCount;             ///< Count of unreleased message states
} ieieClientStateInfo_V1_t;

/// @brief Version 2 of the exported client state information
typedef struct tag_ieieClientStateInfo_V2_t
{
    uint32_t                    Version;              ///< The version of clientState import/export information
    bool                        Durable;              ///< Is this a durable client state?
    uint32_t                    ClientIdLength;       ///< Length of optional client ID
    uint32_t                    ProtocolId;           ///< Protocol identifier
    uint32_t                    LastConnectedTime;    ///< Last connected time (in ExpireTime form)
    uint32_t                    UserIdLength;         ///< Length of associated user-id
    uint64_t                    WillMsgId;            ///< Will message identifier (pointer when exported)
    uint32_t                    WillMsgTimeToLive;    ///< Will Message time to live
    uint32_t                    WillTopicNameLength;  ///< Length of will-topic name
    uint32_t                    UMSCount;             ///< Count of unreleased message states
    // Version 2+
    uint32_t                    ExpiryInterval;       ///< Expiry interval for this clientState
} ieieClientStateInfo_V2_t;

//****************************************************************************
/// @brief  Fill in the passed hash table with all of the clientIds that match
///         the specified regular expression.
///
/// @param[in]     control        ieieExportResourceControl_t for this export
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_getMatchingClientIds(ieutThreadData_t *pThreadData,
                                  ieieExportResourceControl_t *control);

//****************************************************************************
/// @internal
///
/// @brief  Export the information for client states whose clientId matches
///         the specified regex.
///
/// @param[in]     control        ieieExportResourceControl_t for this export
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_exportClientStates(ieutThreadData_t *pThreadData,
                                ieieExportResourceControl_t *control);

//****************************************************************************
/// @internal
///
/// @brief  Check that we can import this client state and add it to the
///         validated set & global set of clientIds being imported.
///
/// @param[in]     control        ieieImportResourceControl_t for this import
/// @param[in]     dataId         dataId (The dataId of the client)
/// @param[in]     data           ieieClientStateInfo_t
/// @param[in]     dataLen        Length of the clientStateInfo
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieieValidateClientStateImport(ieutThreadData_t *pThreadData,
                                      ieieImportResourceControl_t *control,
                                      uint64_t dataId,
                                      uint8_t *data,
                                      size_t dataLen);

//****************************************************************************
/// @internal
///
/// @brief  Release clientIds that have been marked as being imported
///
/// @param[in]     key            ClientId
/// @param[in]     keyHash        Hash of the key value
/// @param[in]     value          DataId of the ClientId (for information only)
/// @param[in]     context        For convenience, the address of the
///                               activeImportClientIdTable.
///
/// @remark Note that once the clientId has been removed from the
/// activeImportClientIdTable the key is freed (rendering it invalid for
/// future queries)
//****************************************************************************
void ieie_releaseValidatedClientId(ieutThreadData_t *pThreadData,
                                   char *key,
                                   uint32_t keyHash,
                                   void *value,
                                   void *context);

//****************************************************************************
/// @internal
///
/// @brief  Release ClientStates that were added
///
/// @param[in]     key            DataId
/// @param[in]     keyHash        Hash of the key value
/// @param[in]     value          pClient imported
/// @param[in]     context        unused
///
/// @remark Note we decrement the use count for this client, we do not release
/// it, thus leaving a zombie with 0 useCount in the clientState table
//****************************************************************************
void ieie_releaseImportedClientState(ieutThreadData_t *pThreadData,
                                     char *key,
                                     uint32_t keyHash,
                                     void *value,
                                     void *context);

//****************************************************************************
/// @brief Find the dataId for a client that is actively being imported
///
/// @param[in]     clientId       ClientId being checked
/// @param[in]     clientIdHash   Hash calculated for the clientId
///
/// @remark The clientState table (ismEngine_serverGlobal.Mutex) should be held
/// when this is being called.
///
/// @return The dataId of the import, or zero if it is not being imported.
//****************************************************************************
uint64_t ieie_findActiveImportClientDataId(ieutThreadData_t *pThreadData,
                                           const char *clientId,
                                           uint32_t clientIdHash);

//****************************************************************************
/// @internal
///
/// @brief Check whether the specified clientId is part of a set being imported
///
/// @param[in]     clientId       ClientId being checked
/// @param[in]     clientIdHash   Hash calculated for the clientId
///
/// @remark The clientState table (ismEngine_serverGlobal.Mutex) should be held
/// when this is being checked.
///
/// @return true if the specified clientId is being imported, otherwise false.
//****************************************************************************
bool ieie_isClientIdBeingImported(ieutThreadData_t *pThreadData,
                                  const char *clientId,
                                  uint32_t clientIdHash);

//****************************************************************************
/// @internal
///
/// @brief  Find an imported ClientState by dataId
///
/// @param[in]     control              Import control structure
/// @param[in]     dataId               Identifier given to the clientState on export
/// @param[out]    ppClient             Client found
///
/// @remark If the client is found, it does NOT have its use count increased.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_findImportedClientState(ieutThreadData_t *pThreadData,
                                     ieieImportResourceControl_t *control,
                                     uint64_t dataId,
                                     ismEngine_ClientState_t **ppClient);

//****************************************************************************
/// @internal
///
/// @brief  Find an imported ClientState by ClientId string
///
/// @param[in]     control       Import control structure
/// @param[in]     clientId      ClientId string
/// @param[out]    ppClient      Client found
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_findImportedClientStateByClientId(ieutThreadData_t *pThreadData,
                                               ieieImportResourceControl_t *control,
                                               const char *clientId,
                                               ismEngine_ClientState_t **ppClient);

//****************************************************************************
/// @internal
///
/// @brief  Find the imported ClientState which owns the dataId of a queueHandle
///
/// @param[in]     control       Import control structure
/// @param[in]     dataId        DataId of the queue handle being sought
/// @param[out]    ppClient      Client found
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_findImportedClientStateByQueueDataId(ieutThreadData_t *pThreadData,
                                                  ieieImportResourceControl_t *control,
                                                  uint64_t dataId,
                                                  ismEngine_ClientState_t **ppClient);

//****************************************************************************
/// @internal
///
/// @brief  Import a client state
///
/// @param[in]     control        ieieImportResourceControl_t for this import
/// @param[in]     dataId         dataId (the dataId for this client)
/// @param[in]     data           ieieClientStateInfo_t
/// @param[in]     dataLen        sizeof(ieieClientStateInfo_t plus data)
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_importClientState(ieutThreadData_t *pThreadData,
                               ieieImportResourceControl_t *control,
                               uint64_t dataId,
                               uint8_t *data,
                               size_t dataLen);

#endif
