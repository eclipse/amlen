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

#ifndef MCP_LOCALSUBMANAGERIMPL_H_
#define MCP_LOCALSUBMANAGERIMPL_H_

#include <stdexcept>
#include <set>
#include <map>
#include <vector>


#include <boost/thread/recursive_mutex.hpp>
#include <boost/unordered_set.hpp>
#include <boost/tuple/tuple.hpp>

#include "LocalSubManager.h"
#include "MCPConfig.h"
#include "SubCoveringFilterPublisher.h"

#include "BloomFilter.h"
#include "CountingBloomFilter.h"
#include "SubscriptionPattern.h"
#include "TaskExecutor.h"
#include "PublishLocalBFTask.h"
#include "PublishRetainedTask.h"
#include "PublishMonitoringTask.h"

#include "LocalExactSubManager.h"
#include "LocalWildcardSubManager.h"
#include "LocalRetainedStatsManager.h"
#include "LocalMonitoringManager.h"
#include "FatalErrorHandler.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

/**
 * Keep a set for all subscribed topics
 *
 * boost::unordered_set or std::set(tree-based impl)
 *
 */
class LocalSubManagerImpl: public LocalSubManager, public LocalSubscriptionEvents, public spdr::ScTraceContext
{
private:
	static spdr::ScTraceComponent* tc_;
public:

	LocalSubManagerImpl(
			const std::string& inst_ID,
			const MCPConfig& mcpConfig,
			const std::string& server_name,
			TaskExecutor& taskExecutor,
			ControlManager& controlManager);

	/**
	 * check the state
	 *
	 * need to close first before destructor
	 *
	 */
	virtual ~LocalSubManagerImpl();

	/*
	 * @see LocalSubscriptionEvents
	 */
	virtual MCPReturnCode addSubscriptions(
			const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs);

	/*
	 * @see LocalSubscriptionEvents
	 */
	virtual MCPReturnCode removeSubscriptions(
			const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs);

	/*
	 * @see RemoteSubscriptionStatsListener
	 */
	virtual int update(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID,
			const RemoteSubscriptionStats& stats);
	/*
	 * @see RemoteSubscriptionStatsListener
	 */
	virtual int disconnected(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID);
	/*
	 * @see RemoteSubscriptionStatsListener
	 */
	virtual int connected(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID);
	/*
	 * @see RemoteSubscriptionStatsListener
	 */
	virtual int remove(
			ismCluster_RemoteServerHandle_t hClusterHandle,
			const char* pServerUID);


	MCPReturnCode setSubCoveringFilterPublisher(SubCoveringFilterPublisher_SPtr subCoveringFilterPublisher);

	boost::shared_ptr<RemoteSubscriptionStatsListener> getRemoteSubscriptionStatsListener();

	MCPReturnCode updateRetainedStats(const char *pServerUID,
	               void *pData, uint32_t dataLength);

	MCPReturnCode setHealthStatus(ismCluster_HealthStatus_t  healthStatus);

	MCPReturnCode setHaStatus(ismCluster_HaStatus_t haStatus);

	ismCluster_HealthStatus_t getHealthStatus();

	ismCluster_HaStatus_t getHaStatus();

	/**
	 * Start the unit.
	 *
	 * Start the unit, starting threads if any, etc.
	 *
	 * @return return code from MCPReturnCode
	 *
	 * @see MCPReturnCode
	 */
	MCPReturnCode start();

	/*
	 * @see LocaSubManager
	 */
	virtual MCPReturnCode restoreSubscriptionPatterns(const std::vector<SubscriptionPattern_SPtr>& patterns);

	MCPReturnCode recoveryCompleted();

	/**
	 * Store and process the Engine's incoming forwarded message statistics.
	 *
	 * @param pEngineStatistics
	 * @return
	 */
	int reportEngineStatistics(ismCluster_EngineStatistics_t *pEngineStatistics);

	/**
	 *
	 * Closing in a top-down manner
	 *
	 * update the state
	 * reset shared pointer
	 * release memory
	 *
	 * Close the unit.
	 *
	 * Close the unit, stopping threads if any, releasing resources, etc.
	 * This method is idempotent, i.e. may be called multiple times with the same effect.
	 *
	 * @return return code from MCPReturnCode
	 *
	 * @see MCPReturnCode
	 */
	MCPReturnCode close(bool leave_state_error = false);

	/*
	 * @see LocalSubManager
	 */
	virtual MCPReturnCode publishLocalBFTask();

	/*
	 * @see LocalSubManager
	 */
	virtual MCPReturnCode schedulePublishLocalBFTask(int delayMillis);

    /*
     * @see LocalSubManager
     */
	virtual MCPReturnCode publishRetainedTask();

    /*
     * @see LocalSubManager
     */
	virtual MCPReturnCode schedulePublishRetainedTask(int delayMillis);

    /*
     * @see LocalSubManager
     */
	virtual MCPReturnCode publishMonitoringTask();

    /*
     * @see LocalSubManager
     */
	virtual MCPReturnCode schedulePublishMonitoringTask(int delayMillis);


	void setFatalErrorHandler(FatalErrorHandler* pFatalErrorHandler);

private:
	const MCPConfig& config;
	TaskExecutor& taskExecutor;

	//State
	bool m_started;
	bool m_closed;
	bool m_recovered;
	bool m_error;
	boost::recursive_mutex m_stateMutex;

	boost::shared_ptr<PublishLocalBFTask> publishTask;
	bool publishTask_scheduled;

	boost::shared_ptr<PublishRetainedTask> retainTask;
	bool retainTask_scheduled;

    boost::shared_ptr<PublishMonitoringTask> monitoringTask;
    bool monitoringTask_scheduled;

	//=== Exact subscriptions ===
	boost::shared_ptr<LocalExactSubManager> exactManager;

	//=== Wild-card subscriptions ===
	boost::shared_ptr<LocalWildcardSubManager> wildcardManager;

	//=== Retained message version vector manager ===
	boost::shared_ptr<LocalRetainedStatsManager> retainedManager;

	//=== Monitoring Manager ===
	boost::shared_ptr<LocalMonitoringManager> monitoringManager;

	FatalErrorHandler* fatalErrorHandler_;

	std::deque<ismCluster_EngineStatistics_t> engineStatistics_;
	const unsigned int engineStatsNumPeriods_;

	int onFatalError(const std::string& component, const std::string& errorMessage, int rc);

};


} /* namespace mcp */

#endif /* MCP_LOCALSUBMANAGERIMPL_H_ */
