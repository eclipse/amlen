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

#ifndef MCPROUTING_H_
#define MCPROUTING_H_

#include <stdexcept>
#include <vector>

#include <PropertyMap.h>
#include <NodeID.h>

#include "RouteLookup.h"
#include "LocalSubscriptionEvents.h"
#include "LocalForwardingEvents.h"
#include "EngineEventCallback.h"
#include "ForwardingControl.h"
#include "MCPReturnCode.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

/**
 *
 */
class MCPRouting: public RouteLookup, public LocalSubscriptionEvents, public LocalForwardingEvents
{
private:
	static spdr::ScTraceComponent* tc_;
	static spdr::ScTraceContextImpl tcntx_;
public:
	virtual ~MCPRouting();

	/**
	 * Create an MCP instance.
	 *
	 * @param mcpProps
	 * @param spidercastProps
	 * @param spidercastBootstrapSet
	 * @param mcpRouting
	 * @return
	 */
	static MCPReturnCode create(
			const spdr::PropertyMap& mcpProps,
			const spdr::PropertyMap& spidercastProps,
			const std::vector<spdr::NodeID_SPtr>& spidercastBootstrapSet,
			MCPRouting** mcpRouting);

	/**
	 * Start of recovery, threads, start control, join the overlay.
	 *
	 * @return
	 */
	virtual MCPReturnCode start() = 0;

	virtual MCPReturnCode registerEngineEventCallback(EngineEventCallback* engineEventCallBack) = 0;

	virtual MCPReturnCode registerProtocolEventCallback(ForwardingControl* forwardingControl) = 0;

	/**
	 * Restore remote nodes during recovery.
	 *
	 * Must be called after start() and before recoveryCompleted().
	 *
	 * @param pServersData
	 * @param numServers
	 * @return
	 */
	virtual MCPReturnCode restoreRemoteServers(
			const ismCluster_RemoteServerData_t *pServersData,
	        int numServers) = 0;

	virtual MCPReturnCode recoveryCompleted() = 0;

	virtual MCPReturnCode startMessaging() = 0;

	virtual MCPReturnCode adminDeleteNode(const ismCluster_RemoteServerHandle_t phServerHandle) = 0;

	virtual MCPReturnCode adminDetachFromCluster() = 0;

	virtual MCPReturnCode stop() = 0;

	virtual MCPReturnCode getStatistics(const ismCluster_Statistics_t *pStatistics) = 0;

	virtual MCPReturnCode updateRetainedStats(const char *pServerUID,
            void *pData, uint32_t dataLength) = 0;

	virtual MCPReturnCode lookupRetainedStats(const char *pServerUID,
            ismCluster_LookupRetainedStatsInfo_t **pLookupInfo) = 0;

	static MCPReturnCode freeRetainedStats(
	        ismCluster_LookupRetainedStatsInfo_t *pLookupInfo);

	virtual MCPReturnCode getView(ismCluster_ViewInfo_t **pView) = 0;

	static MCPReturnCode freeView(ismCluster_ViewInfo_t *pView);

	static String toStringView(ismCluster_ViewInfo_t *pView);

    virtual MCPReturnCode setHealthStatus(ismCluster_HealthStatus_t  healthStatus) = 0;

    virtual MCPReturnCode setHaStatus(ismCluster_HaStatus_t haStatus) = 0;

    static spdr::log::Level ismLogLevel_to_spdrLogLevel(int ismLogLevel);

protected:
	MCPRouting();

};

} /* namespace mcp */

#endif /* MCPROUTING_H_ */
