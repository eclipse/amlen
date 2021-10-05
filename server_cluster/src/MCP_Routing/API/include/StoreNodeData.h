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

#ifndef MCP_STORENODEDATA_H_
#define MCP_STORENODEDATA_H_

#include "MCPTypes.h"

namespace mcp
{

/**
 * The interface which MCP uses to store persistent data.
 *
 * MCP uses this interface to store persistent data on each of the remote
 * nodes it discovered (for example, versions of Bloom filters).
 * The data persisted with this interface is given to the MCP component
 * upon recovery, using the recovery API.
 */
class StoreNodeData
{
public:
	StoreNodeData();
	virtual ~StoreNodeData();


	/**
	 * Store node data.
	 *
	 * @param hRemoteServer
	 * @param hClusterHandle
	 * @param pServerName
	 * @param pRemoteServerData
	 * @param remoteServerDataLength
	 * @return
	 */
	virtual int update(
			ismEngine_RemoteServerHandle_t     hRemoteServer,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			const char                        *pServerName,
			const char                        *pServerUID,
			void                              *pRemoteServerData,
			size_t                             remoteServerDataLength,
			uint8_t                            fCommitUpdate,
			ismEngine_RemoteServer_PendingUpdateHandle_t  *phPendingUpdateHandle) = 0;
};

} /* namespace mcp */

#endif /* MCP_STORENODEDATA_H_ */
