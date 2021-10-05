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

#ifndef MCP_VIEWKEEPER_H_
#define MCP_VIEWKEEPER_H_

#include <list>
#include <map>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "cluster.h"
#include "MCPTypes.h"
#include <MembershipListener.h>
#include "SubCoveringFilterEventListener.h"
#include "LocalSubManager.h"
#include "EngineEventCallback.h"
#include "ForwardingControl.h"
#include "RemoteServerStatus.h"
#include "ByteBuffer.h"
#include "RemoteServerInfo.h"
#include "MCPConfig.h"
#include "FatalErrorHandler.h"
#include "TaskExecutor.h"
#include "ControlManager.h"
#include "RemovedServers.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * A forward declaration of the engine thread init
 */
XAPI void ism_engine_threadInit(uint8_t isStoreCrit);


#ifdef __cplusplus
}
#endif

namespace mcp
{

/*
 *
 * ViewKeeper created at init()
 * MembershipService is created at recoveryCompleted()
 */
class ViewKeeper: public spdr::MembershipListener,
		public FilterTags,
		public spdr::ScTraceContext
{
private:
	static spdr::ScTraceComponent* tc_;

public:
	ViewKeeper(
			const std::string& inst_ID,
			const MCPConfig& mcpConfig,
			spdr::NodeID_SPtr myNodeID,
			SubCoveringFilterEventListener& updatelistener,
			LocalSubManager& subscriptionStatsListener,
			TaskExecutor& taskExecutor,
			ControlManager& controlManager);
	virtual ~ViewKeeper();

	MCPReturnCode registerEngineEventCallback(EngineEventCallback* engineEventCallBack);

	MCPReturnCode registerProtocolEventCallback(ForwardingControl* forwardingControl);

	void setFatalErrorHandler(FatalErrorHandler* pFatalErrorHandler);

	MCPReturnCode restoreRemoteServers(
				const ismCluster_RemoteServerData_t *pServersData, int numServers, int64_t& storedIncarnationNumber);

	MCPReturnCode recoveryCompleted(int64_t incNum);

	virtual void onMembershipEvent(spdr::event::MembershipEvent_SPtr event);

	MCPReturnCode nodeForwardingConnected(const ismCluster_RemoteServerHandle_t phServerHandle);

	MCPReturnCode nodeForwardingDisconnected(const ismCluster_RemoteServerHandle_t phServerHandle);

	MCPReturnCode adminDeleteNode(const ismCluster_RemoteServerHandle_t phServerHandle, spdr::NodeID_SPtr& nodeID, int64_t* incarnation_num);

    MCPReturnCode adminDeleteNodeFromList(const std::string& nodeUID,  spdr::NodeID_SPtr& nodeID, int64_t* incarnation_num);


	MCPReturnCode storeSubscriptionPatterns(
			const std::vector<SubscriptionPattern_SPtr>& patterns);

	bool isReconciliationFinished() const;

	MCPReturnCode getStatistics(ismCluster_Statistics_t *pStatistics) const;

	/*
	 * Call the Engine callback under the view_mutex.
	 * Thread: TaskExecutor
	 */
	int reportEngineStatistics(ismCluster_EngineStatistics_t *pEngineStatistics);


	MCPReturnCode notifyTerm();     //when doing adminDetach (remove from cluster)
	MCPReturnCode close();          //When closing normally

	MCPReturnCode getView(ismCluster_ViewInfo_t **pView);

	static MCPReturnCode freeView(ismCluster_ViewInfo_t *pView);

	static String toString_RSViewInfo(ismCluster_RSViewInfo_t* rsViewInfo);

	static String toString_ViewInfo(ismCluster_ViewInfo_t* viewInfo);

	void getRemovedServers(RemoteServerVector& serverVector) const;

	void getRestoredNotInViewServers(RemoteServerVector& serverVector) const;

private:
	const MCPConfig& mcpConfig_;
	spdr::NodeID_SPtr my_nodeID;
	std::string my_ServerName;
	std::string my_ServerUID;
	std::string my_ClusterName;
	SubCoveringFilterEventListener& filterUpdatelistener;
	LocalSubManager& subscriptionStatsListener;
	TaskExecutor& taskExecutor_;
	ControlManager& controlManager_;
	FatalErrorHandler* fatalErrorHandler_;
	mutable boost::recursive_mutex view_mutex;

	MCPState state_;

	EngineEventCallback* engineServerRegisteration;
	ForwardingControl* forwardingControl;
	int64_t incarnationNumber;

	ServerIndex serverIndex_max;
	std::set<ServerIndex> serverIndex_gaps;
	ByteBuffer_SPtr storeRecoveryState_ByteBuffer_;

	typedef std::map<spdr::NodeID_SPtr, RemoteServerStatus_SPtr,
			spdr::SPtr_Less<spdr::NodeID> > ServerRegistryMap;
	/* Thread:
	 * the SpiderCast.OnMembershipEvent() thread,
	 * the forwarding thread that delivers connected/disconnected,
	 * the internal task thread
	 * Admin thread on getView/getStatistics
	 */
	ServerRegistryMap serverRegistryMap;

	/*
	 * Persistent cluster-wide removal, with server fencing
	 * Map: ServerUID -> IncarnationNumber
	 */
	RemovedServers removedServers_;

	/*
	 * for safe garbage collection of RemoteServerStatus objects
	 */
	typedef std::list< std::pair<RemoteServerStatus_SPtr, boost::posix_time::ptime> > DeletedNodesList;
	DeletedNodesList  deletedNodes;

	struct RecoveryFilterState
	{
		int64_t  incarnation_number;
		uint64_t bf_exact_lastUpdate;
		uint64_t bf_wildcard_lastUpdate;
		uint64_t bf_wcsp_lastUpdate;
		uint64_t rcf_lastUpdate;

		RecoveryFilterState();

		bool isNonZero() const;

		std::string toString() const;
	};

	typedef std::map<ServerIndex, RecoveryFilterState> RecoveryFilterState_Map;
	RecoveryFilterState_Map recoveryFilterState_Map_;

	ByteBuffer_SPtr storeFilterState;

	std::vector<SubscriptionPattern_SPtr> storePatterns_;
	bool storePatternsPending_;
	bool storeFirstTime_;
	bool storeRemovedServersPending_;
	boost::recursive_mutex storeSelfRecord_mutex_;

	ismCluster_RemoteServer_t selfNode_ClusterHandle_;

    bool selfNodePrev_;
	std::string selfNodePrev_UID_;
	std::string selfNodePrev_Name_;


	boost::shared_array<char*> pSubs_array_;
	std::size_t pSubs_array_length_;

	//=========================================================================

	void onViewChangeEvent(spdr::event::MembershipEvent_SPtr event);
	void onNodeJoinEvent(spdr::event::MembershipEvent_SPtr event);
	void onNodeLeaveEvent(spdr::event::MembershipEvent_SPtr event);
	void onChangeOfMetadataEvent(spdr::event::MembershipEvent_SPtr event);


	typedef std::map<uint64_t, spdr::event::AttributeValue> SortedBF_BaseUpdate_Map;

	int deliver_BF_Base(
			ismCluster_RemoteServerHandle_t clusterHandle,
			const spdr::event::AttributeValue& attrVal,
			const std::string& filterTag);
	int deliver_BF_Update(
			ismCluster_RemoteServerHandle_t clusterHandle,
			const spdr::event::AttributeValue& attrVal,
			const std::string& filterTag);

	int deliver_RCF_Base(
			RemoteServerStatus_SPtr status,
			const spdr::event::AttributeValue& attrVal);

	int deliver_RCF_Update(
			RemoteServerStatus_SPtr status,
			const spdr::event::AttributeValue& attrVal);

	/*
	 * Deliver a sequence of either adds or removes
	 */
    int deliver_RCF_Sequence(
            RemoteServerStatus_SPtr status,
            const RemoteServerStatus::RCF_Map& rcf_map,
            int action);

	int deliver_WCSP_Base(
			ismCluster_RemoteServerHandle_t clusterHandle,
			const spdr::event::AttributeValue& attrVal);

	int deliver_WCSP_Update(
			ismCluster_RemoteServerHandle_t clusterHandle,
			const spdr::event::AttributeValue& attrVal);

	//===
	enum FilterValueType
	{
		VALUE_TYPE_BF_E_BASE	= 1,
		VALUE_TYPE_BF_E_UPDT,
		VALUE_TYPE_BF_W_BASE,
		VALUE_TYPE_BF_W_UPDT,
		VALUE_TYPE_WCSP_BASE,
		VALUE_TYPE_WCSP_UPDT,
		VALUE_TYPE_RCF_BASE,
		VALUE_TYPE_RCF_UPDT
	};

	typedef std::map<uint64_t, std::pair<FilterValueType,spdr::event::AttributeValue> > Sorted_FilterValue_Map;

	bool filterSort_attribute_map(
			const spdr::event::AttributeMap& attr_map,
			uint64_t bf_exact_last_update,
			uint64_t bf_wildcard_last_update,
			uint64_t bf_wcsp_last_update,
			uint64_t rcf_last_update,
			uint64_t& bf_exact_last_update_new,
			uint64_t& bf_wildcard_last_update_new,
			uint64_t& bf_wcsp_last_update_new,
			uint64_t& rcf_last_update_new,
			Sorted_FilterValue_Map& base_update_sorted_map);

	uint64_t getSQN_from_BFAttVal(const spdr::event::AttributeValue& attVal);

	int deliver_filter_changes(
			spdr::event::AttributeMap_SPtr attr_map,
			RemoteServerStatus_SPtr status,
			RecoveryFilterState& recoveryState);

	//===

	int deliver_retained_changes(
	        spdr::event::AttributeMap_SPtr attr_map,
	        RemoteServerStatus_SPtr status);

	void deliver_monitoring_changes(
	        spdr::event::AttributeMap_SPtr attr_map,
	        RemoteServerStatus_SPtr status);

	int deliver_RemovedServersList_changes(
	        spdr::event::AttributeMap_SPtr attr_map,
            RemoteServerStatus_SPtr status);

	int deliver_RestoredNotInView_changes(
	            spdr::event::AttributeMap_SPtr attr_map,
	            RemoteServerStatus_SPtr status);

	//===

	int addUpdate_RestoredNotInView(RemoteServerRecord_SPtr server);

	ServerIndex allocate_ServerIndex();
	void free_ServerIndex(ServerIndex index);

	bool extractFwdEndPoint(const spdr::event::AttributeMap& attr_map, std::string& addr, uint16_t& port, uint8_t& fUseTLS);

	bool extractServerInfo(const spdr::event::AttributeMap& attr_map, uint16_t& protoVerSupported,  uint16_t& protoVerUsed, std::string& serverName);

	int deliver_WCSubStats_Update(const spdr::event::AttributeMap& attr_map, RemoteServerStatus_SPtr status);

	MCPReturnCode deleteNode(ServerRegistryMap::iterator pos, bool recovery);

	int addToRemovedServersMap(const std::string& uid, int64_t incNum);

	void cleanDeletedNodesList();

	/**
	 * The recovery state for a remote server.
	 *
	 * The server data contains:
	 *
	 * uint16_t store-version
	 * int64_t  incarnation number
	 * uint64_t exact BF last update SQN
	 * uint64_t wild-card BF last update SQN
	 * uint64_t wild-card pattern last update SQN
	 * uint64_t RCF last update SQN
	 */
	MCPReturnCode readRecoveryFilterState(const char* uid, const char* pData, uint32_t dataLength,
			RecoveryFilterState& recoveryState);

	/**
	 * The recovery state for a local server.
	 *
	 * The server data contains:
	 *
	 * uint16_t store-version
	 * int8_t record type
	 * int64_t incarnation number
	 * uint32_t num_patterns
	 * SubscriptionPattern x num_patterns
	 * Optional:
	 * uint32_t num_records
	 * {
	 *     String  uid
	 *     int64_t incNum
	 * } x num_records
	 *
	 * @param pData input data
	 * @param dataLength input data length
	 * @param incNum output
	 * @param recoveredPatterns output
	 * @return
	 */
	int readRecoverySelfRecord(const char* pData, uint32_t dataLength,
	        int64_t& incNum,
	        std::vector<SubscriptionPattern_SPtr>& recoveredPatterns, RemovedServers& removedServers, std::string& clusterName);

	/**
	 * The recovery state for a remote server
	 *
	 * @param status
	 * @param filter_sqn_vector
	 * @return
	 */
	int storeRecoveryFilterState(RemoteServerStatus_SPtr status, RecoveryFilterState& filter_sqn_vector );

	int storeRecoverySelfRecord();

    /*
     * Reconciliation is over for a node when the filter versions received
     * from the network are equal or higher than the ones in the recovery
     * filter state. When that happens, the "route-all" flag will be
     * turned off.
     */
    int reconcileRecoveryState(RemoteServerStatus_SPtr status);

    RecoveryFilterState getRecoveryFilterState(RemoteServerStatus_SPtr status);


    /*
     * Never call with view_mutex (or any other mutex) taken
     */
	int onFatalError(const std::string& component, const std::string& errorMessage, int rc);
};

} /* namespace mcp */

#endif /* MCP_VIEWKEEPER_H_ */
