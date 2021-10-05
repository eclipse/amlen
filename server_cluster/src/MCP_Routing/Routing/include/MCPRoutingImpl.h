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

#ifndef MCPROUTINGIMPL_H_
#define MCPROUTINGIMPL_H_

#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/shared_ptr.hpp>

#include "SpiderCastLogicError.h"
#include "SpiderCastRuntimeError.h"
#include "SpiderCastFactory.h"

#include "MCPExceptions.h"
#include "MCPReturnCode.h"
#include "MCPRouting.h"
#include "MCPConfig.h"
#include "LocalSubManagerImpl.h"
#include "GlobalSubManagerImpl.h"
#include "ControlManagerImpl.h"
#include "TaskExecutor.h"
#include "FatalErrorHandler.h"
#include "RoutingTasksHandler.h"
#include "DiscoveryTimeoutTask.h"
#include "TraceLevelMonitorTask.h"
#include "EngineStatisticsTask.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

class MCPRoutingImpl: public MCPRouting, public FatalErrorHandler, public RoutingTasksHandler, public spdr::ScTraceContext
{
private:
	static spdr::ScTraceComponent* tc_;

public:
	MCPRoutingImpl(
			const std::string& inst_ID,
			const spdr::PropertyMap& mcpProps,
			const spdr::PropertyMap& spidercastProps,
			const std::vector<spdr::NodeID_SPtr>& bootstrapSet)
	throw (MCPRuntimeError,spdr::SpiderCastLogicError, spdr::SpiderCastRuntimeError);

	virtual ~MCPRoutingImpl();

	/*
	 * @see MCPRouting
	 */
	virtual MCPReturnCode start();

	/*
	 * @see MCPRouting
	 */
	virtual MCPReturnCode registerEngineEventCallback(EngineEventCallback* engineEventCallBack);

	/*
	 * @see MCPRouting
	 */
	virtual MCPReturnCode registerProtocolEventCallback(ForwardingControl* forwardingControl);


	/*
	 * @see MCPRouting
	 */
	virtual MCPReturnCode restoreRemoteServers(
				const ismCluster_RemoteServerData_t *pServersData,
		        int numServers);

	/*
	 * @see MCPRouting
	 */
	virtual MCPReturnCode recoveryCompleted();

	/*
	 * @see MCPRouting
	 */
	virtual MCPReturnCode startMessaging();

	/*
	 * @see MCPRouting
	 */
	virtual MCPReturnCode stop();

	/*
	 * @see MCPRouting
	 */
	virtual MCPReturnCode getStatistics(const ismCluster_Statistics_t *pStatistics);

	//===================================================================

	/*
	 * @see LocalForwardingEvents
	 */
	virtual int setLocalForwardingInfo(
				const char *pServerName,
				const char *pServerUID,
				const char *pServerAddress,
				int serverPort,
				uint8_t fUseTLS);

	/*
	 * @see LocalForwardingEvents
	 */
	virtual int nodeForwardingConnected(const ismCluster_RemoteServerHandle_t phServerHandle);

	/*
	 * @see LocalForwardingEvents
	 */
	virtual int nodeForwardingDisconnected(const ismCluster_RemoteServerHandle_t phServerHandle);

	//===================================================================

	/*
	 * @see RouteLookup
	 */
	virtual MCPReturnCode lookup(
			ismCluster_LookupInfo_t* pLookupInfo);

	//===================================================================

	/*
	 * @see LocalSubscriptionEvents
	 */
	MCPReturnCode addSubscriptions(
			const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs);
	/*
	 * @see LocalSubscriptionEvents
	 */
	MCPReturnCode removeSubscriptions(
			const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs);

	//===================================================================

	/*
	 * @see FatalErrorHandler
	 */
	virtual int onFatalError(const std::string& component, const std::string& errorMessage, int rc);

	/*
	 * @see FatalErrorHandler
	 */
	virtual int onFatalError_MaintenanceMode(const std::string& component, const std::string& errorMessage, int rc, int restartFlag);

	//===================================================================

	/*
	 * @see RoutingTasksHandler
	 */
	virtual void discoveryTimeoutTask();
    /*
     * @see RoutingTasksHandler
     */
	virtual void traceLevelMonitorTask();
    /*
     * @see RoutingTasksHandler
     */
	virtual void engineStatisticsTask();

	//===================================================================

	virtual spdr::NodeID_SPtr getNodeID();

	/*
	 * Administrative deletion of a node.
	 * @param nodeIndex
	 */
	virtual MCPReturnCode adminDeleteNode(const ismCluster_RemoteServerHandle_t phServerHandle);


	/*
	 * Administrative deletion of the local node.
	 * @param nodeIndex
	 */
	virtual MCPReturnCode adminDetachFromCluster();

	//=== Retained messages ==============================================

	virtual MCPReturnCode updateRetainedStats(const char *pServerUID,
	            void *pData, uint32_t dataLength);

	virtual MCPReturnCode lookupRetainedStats(const char *pServerUID,
	            ismCluster_LookupRetainedStatsInfo_t **pLookupInfo);

	//=== Admin View / Monitoring ============================================

    virtual MCPReturnCode getView(ismCluster_ViewInfo_t **pView);

    virtual MCPReturnCode setHealthStatus(ismCluster_HealthStatus_t  healthStatus);

    virtual MCPReturnCode setHaStatus(ismCluster_HaStatus_t haStatus);


private:
	MCPReturnCode internalClose(bool remove_self = false, bool leave_state_error = false);

private:
	const static boost::posix_time::time_duration DISCOVERY_TIMEOUT_TASK_INTERVAL_MS;
	const static boost::posix_time::time_duration TRACE_LEVEL_MONITOR_TASK_INTERVAL_MS;

	MCPConfig mcpConfig_;
	std::string my_ClusterName;
	std::string my_ServerName;
	std::string my_ServerUID;

	MCPState state_;
	ism_time_t stateChangeTime_;
	bool stateFailure_;

	boost::recursive_mutex state_mutex;

	boost::shared_ptr<LocalSubManagerImpl> localSubManager_SPtr;
	boost::shared_ptr<GlobalSubManagerImpl> globalSubManager_SPtr;
	boost::shared_ptr<ControlManagerImpl> controlManager_SPtr;
	boost::shared_ptr<TaskExecutor> taskExecutor_SPtr;

	boost::shared_ptr<DiscoveryTimeoutTask> discoveryTimeoutTask_;
	boost::posix_time::ptime discoveryTimeoutTask_deadline_;

	boost::shared_ptr<TraceLevelMonitorTask> traceLevelMonitorTask_;
	int cluster_trace_level_;
	int spidercast_trace_level_;

	boost::shared_ptr<EngineStatisticsTask> engineStatisticsTask_;
};

} /* namespace mcp */

#endif /* MCPROUTINGIMPL_H_ */
