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

#ifndef MCP_CONTROLMANAGERIMPL_H_
#define MCP_CONTROLMANAGERIMPL_H_

#include <vector>
#include <deque>

#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "NodeID.h"
#include "SpiderCastConfig.h"
#include "SpiderCastFactory.h"

#include "ControlManager.h"
#include "MCPConfig.h"
#include "SubCoveringFilterEventListener.h"
#include "MCPExceptions.h"
#include "ViewKeeper.h"
#include "SubCoveringFilterPublisherImpl.h"
#include "LocalForwardingEvents.h"
#include "LocalSubManager.h"
#include "FatalErrorHandler.h"
#include "TaskExecutor.h"
#include "ViewNotifyEvent.h"
#include "ViewNotifyTask.h"

#include "cluster.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

class ControlManagerImpl: public ControlManager,
		public spdr::SpiderCastEventListener,
		public LocalForwardingEvents,
		public spdr::ScTraceContext
{
private:
	static spdr::ScTraceComponent* tc_;

	const static int adminDetachFromCluster_timeoutMillis_;

public:
	ControlManagerImpl(const std::string& insd_ID, const MCPConfig& mcp_config,
			const spdr::PropertyMap& spidercastProps,
			const std::vector<spdr::NodeID_SPtr>& bootstrapSet,
			TaskExecutor& taskExecutor)
					throw (spdr::SpiderCastLogicError,
					spdr::SpiderCastRuntimeError);

	virtual ~ControlManagerImpl();

	virtual SubCoveringFilterPublisher_SPtr getSubCoveringFilterPublisher();

	MCPReturnCode registerEngineEventCallback(
			EngineEventCallback* engineEventCallBack);

	MCPReturnCode registerProtocolEventCallback(
			ForwardingControl* forwardingControl);

	/*
	 * @see ControlManager
	 */
	virtual MCPReturnCode storeSubscriptionPatterns(
				const std::vector<SubscriptionPattern_SPtr>& patterns);

	void setSubCoveringFilterEventListener(
			boost::shared_ptr<SubCoveringFilterEventListener> listener)
					throw (MCPRuntimeError);

	void setLocalSubManager(
			boost::shared_ptr<LocalSubManager> listener)
					throw (MCPRuntimeError);

	void setFatalErrorHandler(FatalErrorHandler* pFatalErrorHandler);

	void start() throw (spdr::SpiderCastLogicError,
			spdr::SpiderCastRuntimeError, MCPIllegalStateError, MCPRuntimeError);

	MCPReturnCode restoreRemoteServers(
			const ismCluster_RemoteServerData_t *pServersData, int numServers);

	MCPReturnCode recoveryCompleted();

	/*
	 * For testing only
	 */
	int64_t getRecoveredIncarnationNumber() const;
    /*
     * For testing only
     */
	int64_t getCurrentIncarnationNumber() const;

	MCPReturnCode notifyTerm();

	void close(bool soft = true) throw (spdr::SpiderCastRuntimeError);

	spdr::NodeID_SPtr getNodeID()
	{
		return nodeID;
	}

	/*
	 * @see spdr::SpiderCastEventListener
	 */
	void onEvent(spdr::event::SpiderCastEvent_SPtr event);

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
	int nodeForwardingConnected(
			const ismCluster_RemoteServerHandle_t phServerHandle);

	/*
	 * @see LocalForwardingEvents
	 */
	int nodeForwardingDisconnected(
			const ismCluster_RemoteServerHandle_t phServerHandle);

	MCPReturnCode adminDeleteNode(const ismCluster_RemoteServerHandle_t phServerHandle);

	MCPReturnCode adminDetachFromCluster();

	MCPReturnCode getStatistics(ismCluster_Statistics_t *pStatistics);

	bool isReconciliationFinished() const;

	MCPReturnCode getView(ismCluster_ViewInfo_t **pView);

	/*
     * @see ControlManager
	 */
	virtual void executeViewNotifyTask();

	/*
	 * @see ControlManager
	 */
	virtual void executePublishRemovedServersTask();

	/*
	 * @see ControlManager
	 */
	virtual void executePublishRestoredNotInViewTask();

    /*
     * @see ControlManager
     */
	virtual void executeAdminDeleteRemovedServersTask(RemoteServerVector& pendingRemovals);

	/*
	 * @see ControlManager
	 */
	virtual void executeRequestAdminMaintenanceModeTask(int errorRC, int restartFlag);

    /*
     * @see ControlManager
     */
	virtual void onTaskFailure(const std::string& component, const std::string& errorMessage, int rc);

	int reportEngineStatistics(ismCluster_EngineStatistics_t *pEngineStatistics);

private:
	const MCPConfig& mcpConfig;
	spdr::PropertyMap spidercastProperties;
	std::vector<spdr::NodeID_SPtr> spidercastBootstrapSet;
	TaskExecutor& taskExecutor;
	spdr::SpiderCastConfig_SPtr spidercastConfig;

	bool closed;
	bool started;
	bool recovered;
	mutable boost::recursive_mutex control_mutex;

	spdr::SpiderCast_SPtr spidercast;
	spdr::NodeID_SPtr nodeID;
	boost::shared_ptr<SubCoveringFilterEventListener> filterUpdatelistener;
	boost::shared_ptr<LocalSubManager> localSubManager_;
	boost::shared_ptr<ViewKeeper> viewKeeper;
	spdr::MembershipService_SPtr membershipService;
	boost::shared_ptr<SubCoveringFilterPublisherImpl> filterPublisher;
	int64_t recoveredIncarnationNumber;

	std::string forwardingAddress;
	int32_t forwardingPort;
	uint8_t forwardingUseTLS;

	FatalErrorHandler* fatalErrorHandler_;

    std::deque<ViewNotifyEvent_SPtr> viewNotifyEventQ_;
    bool viewNotifyTask_scheduled_;
    mutable boost::recursive_mutex viewNotifyEventQ_mutex_;

	int onFatalError(const std::string& component, const std::string& errorMessage, int rc);
	int onFatalError_MaintenanceMode(const std::string& component, const std::string& errorMessage, int rc, int restartFlag);
};

} /* namespace mcp */

#endif /* CONTROLMANAGERIMPL_H_ */
