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

#ifndef FORWARDINGCONTROL_H_
#define FORWARDINGCONTROL_H_

#include "MCPTypes.h"

namespace mcp
{

class ForwardingControl
{
public:
	ForwardingControl();
	virtual ~ForwardingControl();

	virtual int add(
			const char                        *pServerName,
			const char                        *pServerUID,
			const char                        *pRemoteServerAddress,
			int                                remoteServerPort,
			uint8_t                            fUseTLS,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			ismEngine_RemoteServerHandle_t     hEngineHandle,
			ismProtocol_RemoteServerHandle_t  *phProtocolHandle) = 0;

	virtual int connect(
			ismProtocol_RemoteServerHandle_t   hRemoteServer,
			const char                        *pServerName,
			const char                        *pServerUID,
			const char                        *pRemoteServerAddress,
			int                                remoteServerPort,
			uint8_t                            fUseTLS,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			ismEngine_RemoteServerHandle_t     hEngineHandle) = 0;

	virtual int disconnect(
			ismProtocol_RemoteServerHandle_t   hRemoteServer,
			const char                        *pServerName,
			const char                        *pServerUID,
			const char                        *pRemoteServerAddress,
			int                                remoteServerPort,
			uint8_t                            fUseTLS,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			ismEngine_RemoteServerHandle_t     hEngineHandle) = 0;


	virtual int remove(
			ismProtocol_RemoteServerHandle_t   hRemoteServer,
			const char                        *pServerName,
			const char                        *pServerUID,
			const char                        *pRemoteServerAddress,
			int                                remoteServerPort,
			uint8_t                            fUseTLS,
			ismCluster_RemoteServerHandle_t    hClusterHandle,
			ismEngine_RemoteServerHandle_t     hEngineHandle) = 0;

	virtual int term() = 0;

};

} /* namespace mcp */

#endif /* FORWARDINGCONTROL_H_ */
