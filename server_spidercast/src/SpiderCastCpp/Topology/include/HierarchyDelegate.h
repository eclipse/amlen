// Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
/*
 * HierarchyDelegate.h
 *
 *  Created on: Oct 13, 2010
 */

#ifndef HIERARCHYDELEGATE_H_
#define HIERARCHYDELEGATE_H_

#include <set>
#include <sstream>
#include <vector>

#include <boost/noncopyable.hpp>

#include "SpiderCastConfigImpl.h"
#include "NodeIDCache.h"
#include "CoreInterface.h"
#include "HierarchyManager.h"
#include "NeighborTable.h"
#include "BootstrapSet.h"
#include "HierarchyUtils.h"
#include "HierarchyDelegateConnectTask.h"
#include "HierarchyDelegateUnquarantineTask.h"
#include "HierarchyDelegateViewUpdateTask.h"
#include "HierarchyDelegateTaskInterface.h"
#include "HierarchyViewKeeper.h"
#include "HierarchyDelegatePubSubBridgeTask.h"
#include "RoutingManager.h"

namespace spdr
{

class VirtualIDCache;

/*
 * TODO
 * 2. handle redirect messages, add supervisor candidates to supervisor-BSS
 * 11. handle disconnect reply
 */
class HierarchyDelegate : boost::noncopyable,
	public HierarchyDelegateTaskInterface, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	HierarchyDelegate(const String& instID,
			const SpiderCastConfigImpl& config,
			NodeIDCache& nodeIDCache,
			VirtualIDCache& vidCache,
			CoreInterface& coreInterface,
			boost::shared_ptr<HierarchyViewKeeper> viewKeeper);

	virtual ~HierarchyDelegate();

	void init();

	void terminate();

	void destroyCrossRefs();

	void sendLeave2All();

	void processIncomingHierarchyMessage(SCMessage_SPtr inHierarchyMsg);

	void processIncomingCommEventMsg(SCMessage_SPtr incomingCommEvent);

	/**
	 * Reschedule the connect task to 'delayMillis' in the future,
	 * only if it is not already scheduled.
	 *
	 * @param delayMillis
	 */
	void rescheduleConnectTask(int delayMillis);

	/**
	 * Reschedule the pub sub bridge task to 'delayMillis' in the future,
	 * only if it is not already scheduled.
	 *
	 * @param delayMillis
	 */
	void reschedulePubSubBridgeTask(int delayMillis);

	void globalViewChanged();

	/*
	 * @see HierarchyDelegateTaskInterface
	 */
	void connectTask();

	/*
	 * @see HierarchyDelegateTaskInterface
	 */
	void unquarantineSupervisorCandidate(NodeIDImpl_SPtr peer);

	/*
	 * @see HierarchyDelegateTaskInterface
	 */
	void supervisorViewUpdate();

	/*
	 * @see HierarchyDelegateTaskInterface
	 */
	void pubsubBridgeTask();

private:
	const String& instID_;
	const SpiderCastConfigImpl& config_;
	NodeIDCache& nodeIDCache_;
	CoreInterface& coreInterface_;
	boost::shared_ptr<HierarchyViewKeeper> hierarchyViewKeeper_SPtr;

	mutable boost::recursive_mutex mutex_;
	volatile bool closed_;

	/* The set of bootstrap nodes. In-view are nodes that are in:
	 * (1) a request (physical/logical) or (2) in the supervisor-table, or
	 * (3) in quarantine, or (4) in disconnect-request table.
	 * Not-in-view nodes are eligible candidates.
	 */
	BootstrapSet 	supervisorBootstrapSet_;

	/* Supervisor connect state
	 */
	typedef std::set<NodeIDImpl_SPtr, NodeIDImpl::SPtr_Less> NodeIDSet;
	NodeIDSet 		outgoingPhysicalConnectRequests_;
	NeighborTable 	outgoingLogicalConnectRequests_;


	NeighborTable 	supervisorNeighborTable_;

	/* A data structure that includes whether the candidate is active w.r.t. the supervisor.
	 *
	 * first - active
	 * second - includeAttributes
	 * third - firstViewDelivered
	 */
	typedef boost::tuple<bool,bool,bool> SupervisorState;
	typedef boost::unordered_map<NodeIDImpl_SPtr, SupervisorState,
				NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals > SuprvisorStateMap;
	//typedef std::map<NodeIDImpl_SPtr, SupervisorState, spdr::SPtr_Less<NodeIDImpl> > SuprvisorStateMap;
	SuprvisorStateMap supervisorStateTable_;

	/* Rejected connect requests are quarantined here
	 */
	NodeIDSet		quarantineConnectTargets_;

	/* Disconnect request was sent to these, waiting for reply
	 */
	NodeIDSet 	disconnectRequested_;

	TaskSchedule_SPtr taskSchedule_SPtr;
	CommAdapter_SPtr commAdapter_SPtr;
	AbstractTask_SPtr connectTask_SPtr;
	HierarchyManager_SPtr hierarchyManager_SPtr;
	MembershipManager_SPtr membershipManager_SPtr;

	bool connectTaskScheduled_;
	bool firstConnectTask_;

	SCMessage_SPtr outgoingHierMessage_;
	ByteBuffer_SPtr attrValBuffer_;

	bool viewUpdateTaskScheduled_;
	AbstractTask_SPtr viewUpdateTask_SPtr;

	bool pubsubBridgeTaskScheduled_;
	AbstractTask_SPtr pubsubBridgeTask_SPtr;
	std::pair<NodeIDImpl_SPtr,Neighbor_SPtr> pubsubBridge_target_;

	//=== methods ===

	bool sendConnectRequest(Neighbor_SPtr target);

	bool sendDisconnectRequest(Neighbor_SPtr target);

	bool sendDisconnectReply(Neighbor_SPtr target, bool accept);

	bool sendLeave(Neighbor_SPtr target);

	bool sendReply_StartMembershipPush(Neighbor_SPtr target, bool accept);

	bool sendReply_StopMembershipPush(Neighbor_SPtr target, bool accept);

//	/*
//	 * Verify the message sender & target fields, throw a SpiderCastRuntimeError if wrong
//	 */
//	void verifyIncomingMessageAddressing(
//			const String& msgSenderName, const String& connSenderName, const String& msgTargetName);

	int getNumSupervisorsAndRequests();

	void addSupervisor(NodeIDImpl_SPtr supervisor);

	void removeSupervisor(NodeIDImpl_SPtr supervisor);

	void updateSupervisorActive(NodeIDImpl_SPtr supervisor, bool active);

	/**
	 * Init the attributes to reflect the state of the delegate.
	 */
	void initAttributes();

	/**
	 * Quarantine a supervisor candidate.
	 */
	void quarantineSupervisorCandidate(NodeIDImpl_SPtr peer);

	void disconnectFromSupervisors();

	bool isClosed() const;

	void startMembershipPush(NodeIDImpl_SPtr supervisor, bool includeAttributes, int aggregationDelayMillis);

	void stopMembershipPush(NodeIDImpl_SPtr supervisor);

	void rescheduleViewUpdateTask();

	int getNumActiveSupervisors();

	std::pair<NodeIDImpl_SPtr,Neighbor_SPtr> chooseActiveSupervisor();

};

}

#endif /* HIERARCHYDELEGATE_H_ */
