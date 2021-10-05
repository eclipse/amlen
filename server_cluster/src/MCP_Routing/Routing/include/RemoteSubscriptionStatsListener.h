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

#ifndef MCP_REMOTESUBSCRIPTIONSTATSLISTENER_H_
#define MCP_REMOTESUBSCRIPTIONSTATSLISTENER_H_

#include "RemoteServerInfo.h"
#include "RemoteSubscriptionStats.h"

namespace mcp
{

/**
 * An interface used to convey wild card subscription statistics information
 * from the ControlManager to the LocalSubManager.
 */
class RemoteSubscriptionStatsListener
{
public:
	RemoteSubscriptionStatsListener();
	virtual ~RemoteSubscriptionStatsListener();

	virtual int update(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID,
			const RemoteSubscriptionStats& stats) = 0;

	virtual int disconnected(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID) = 0;

	virtual int connected(
				ismCluster_RemoteServerHandle_t hClusterHandle,
				const char* pServerUID) = 0;

	virtual int remove(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID) = 0;
};

} /* namespace mcp */

#endif /* MCP_REMOTESUBSCRIPTIONSTATSLISTENER_H_ */
