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

#ifndef MCP_LOCALFORWARDINGEVENTS_H_
#define MCP_LOCALFORWARDINGEVENTS_H_

#include "MCPTypes.h"

namespace mcp
{

/**
 * The interface with which the local forwarding component of MCP informs the
 * routing component on:
 * 1) what is the local forwarding address, and
 * 2) whether the local node is connected (disconnected) to the forwarding address
 *    of a remote node.
 */
class LocalForwardingEvents
{
public:
	LocalForwardingEvents();
	virtual ~LocalForwardingEvents();

	virtual int setLocalForwardingInfo(
			const char *pServerName,
			const char *pServerUID,
			const char *pServerAddress,
			int serverPort,
			uint8_t fUseTLS) = 0;
	/**
	 *
	 * @param nodeIndex
	 * @param nodeName C-string
	 * @return
	 */
	virtual int nodeForwardingConnected(
			const ismCluster_RemoteServerHandle_t phServerHandle) = 0;

	/**
	 *
	 * @param nodeIndex
	 * @param nodeName C-string
	 * @return
	 */
	virtual int nodeForwardingDisconnected(
			const ismCluster_RemoteServerHandle_t phServerHandle) = 0;

};

} /* namespace mcp */

#endif /* MCP_LOCALFORWARDINGEVENTS_H_ */
