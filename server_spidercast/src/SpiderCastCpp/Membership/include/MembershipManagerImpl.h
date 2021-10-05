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
 * MembershipManagerImpl.h
 *
 *  Created on: 16/03/2010
 */

#ifndef MEMBERSHIPMANAGERIMPL_H_
#define MEMBERSHIPMANAGERIMPL_H_

#include <vector>
#include <deque>

#include <boost/unordered_set.hpp>
#include <boost/utility.hpp>

#include "MembershipManager.h"
#include "MembershipDefinitions.h"
#include "AttributeManager.h"
#include "SpiderCastConfigImpl.h"
#include "CoreInterface.h"
#include "NodeVersion.h"
#include "MembershipService.h"
#include "MembershipServiceImpl.h"
#include "SpiderCastLogicError.h"
#include "MembershipListener.h"
#include "BootstrapSet.h"
#include "NodeHistorySet.h"
#include "NodeHistoryPruneTask.h"
#include "NodeInfo.h"
#include "Suspicion.h"
#include "NodeIDCache.h"
#include "VirtualIDCache.h"
#include "UpdateDatabase.h"
#include "NeighborChangeTask.h"
#include "RefreshSuccessorListTask.h"
#include "HighPriorityMonitor.h"
#include "LeaderElectionService.h"
#include "LeaderElectionListener.h"
#include "LEViewKeeper.h"
#include "LECandidate.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"


namespace spdr
{

class MembershipManagerImpl: public MembershipManager, public ScTraceContext, boost::noncopyable
{
private:
	static ScTraceComponent* tc_;

public:
	MembershipManagerImpl(const String& instID, const SpiderCastConfigImpl& config,
			NodeIDCache& nodeIDCache, VirtualIDCache& vidCache, CoreInterface& coreInterface);

	virtual ~MembershipManagerImpl();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void start();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual bool terminate(bool soft, bool removeRetained, int timeout_millis);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual NodeIDImpl_SPtr getRandomNode();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual std::pair<NodeIDImpl_SPtr,int> getRandomizedStructuredNode();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual std::pair<NodeIDImpl_SPtr, bool> getDiscoveryNode();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual bool isFromHistorySet(NodeIDImpl_SPtr discoveryNode);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void reportSuspect(NodeIDImpl_SPtr suspectedNode);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual bool reportSuspicionDuplicateRemoteNode(NodeIDImpl_SPtr suspectedNode, int64_t incarnationNumber);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void newNeighbor(NodeIDImpl_SPtr node);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void disconnectedNeighbor(NodeIDImpl_SPtr node);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void processIncomingMembershipMessage(SCMessage_SPtr membershipMsg);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual NodeVersion getMyNodeVersion() const;

	/*
	 * @see spdr::MembershipManager
	 */
	virtual bool processIncomingMulticastDiscoveryNodeView(NodeIDImpl_SPtr peerID, const NodeVersion& ver, bool isRequest, bool isBootstrap);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void getDiscoveryView(SCMessage_SPtr discoveryMsg);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void processIncomingDiscoveryView(SCMessage_SPtr discoveryMsg, bool isRequest, bool isBootstrap);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void getDiscoveryViewPartial(SCMessage_SPtr discoveryMsg, int num_id = 0);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual int getDiscoveryViewMultipart(std::vector<SCMessage_SPtr>& discoveryMsgMultipart);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void destroyMembershipService();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual AttributeControl& getAttributeControl()
	{
		return attributeManager;
	}

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void registerInternalMembershipConsumer(
				boost::shared_ptr<SCMembershipListener> listener,
				InternalMembershipConsumer who);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void getDelegateFullView(SCMessage_SPtr viewupdateMsg, bool includeAttributes);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void destroyLeaderElectionService();

	//=== Tasks Interface ===

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void periodicTask();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void terminationTask();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void terminationGraceTask();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void clearRetainAttrTask();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void firstViewDeliveryTask();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void neighborChangeTask();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void changeOfMetadataDeliveryTask();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void refreshSuccessorListTask();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void scheduleChangeOfMetadataDeliveryTask();

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void notifyForeignZoneMembership(
		const int64_t requestID, const String& zoneBusName,
		boost::shared_ptr<event::ViewMap > view, const bool lastEvent);


	/*
	 * @see spdr::MembershipManager
	 */
	virtual void notifyForeignZoneMembership(
		const int64_t requestID, const String& zoneBusName,
		event::ErrorCode errorCode, const String& errorMessage, bool lastEvent);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual void notifyZoneCensus(
		const int64_t requestID, event::ZoneCensus_SPtr census, const bool full);

	/*
	 * @see spdr::MembershipManager
	 */
	virtual int getViewSize();


	/*
	 * @see spdr::MembershipManager
	 */
	virtual bool clearRemoteNodeRetainedAttributes(NodeID_SPtr target, int64_t incarnation);

	//===============================================================

	MembershipService_SPtr createMembershipService(
				const PropertyMap& properties,
				MembershipListener& membershipListener)
				throw (SpiderCastLogicError);

	leader_election::LeaderElectionService_SPtr createLeaderElectionService(
				leader_election::LeaderElectionListener& electionListener,
				bool candidate,
				const PropertyMap& properties)
	throw (SpiderCastRuntimeError,SpiderCastLogicError);

	/**
	 * Create cross-references between the components.
	 * To be called right after construction but before start().
	 */
	void init();

	/**
	 * Destroy cross-references between the components.
	 * To be called right before destruction.
	 */
	void destroyCrossRefs();

	void reportStats(boost::posix_time::ptime time, bool labels);

private:
	//static const String intMemConsumerName[];

	const String _instID;
	const SpiderCastConfigImpl& _config;
	CoreInterface& _coreInterface;
	/*
	 * Contains the IDs of nodes that are in view, and nodes in history with retained attributes
	 */
	NodeIDCache& _nodeIDCache;
	VirtualIDCache& _nodeVirtualIDCache;

	/* Signals the component was started
	 * Threading: App-thread
	 */
	volatile bool _started;

	/* set by core to signal orderly shutdown
	 * Threading: App-thread, Delivery-thread, any failing thread.
	 */
	volatile bool _closed;
	volatile bool _close_soft;
	volatile bool _close_remove_retained_att;

	volatile bool _close_done;
	volatile bool _close_remove_retained_att_ack;
	boost::condition_variable_any _close_done_condition_var;

	bool _first_periodic_task;

	/* Threading: MemTopo-thread only */
	TopologyManager_SPtr topoMgr_SPtr;

	/* Threading: MemTopo, App, etc lock the internal mutex */
	NeighborTable_SPtr neighborTable_SPtr;

	/* Threading: any, thread-safe */
	TaskSchedule_SPtr taskSchedule_SPtr;

	/* Threading: MemTopo-thread only */
	AbstractTask_SPtr periodicTask_SPtr;

	/* Threading: MemTopo-thread only */
	AbstractTask_SPtr historyPruneTask_SPtr;

	/* Threading: created/destroyed by App-thread, used by Membership-thread */
	boost::shared_ptr<MembershipServiceImpl> membershipServiceImpl_SPtr;

	/* Lock when using multi-threaded variables */
	boost::recursive_mutex membership_mutex;

	//=========================================================================

	/* The set of bootstrap nodes
	 * Threading: MemTopo-thread only */
	boost::shared_ptr<Bootstrap> bootstrap;
	//BootstrapSet bootstrapSet;
	boost::posix_time::ptime bootstrapStart;

	/* A set of nodes that left the overlay
	 * Threading: MemTopo-thread only */
	NodeHistorySet nodeHistorySet;

	/* Threading: MemTopo-thread only */
	NodeIDImpl_SPtr myID;

	/* Threading: MemTopo-thread only */
	util::VirtualID_SPtr myVID;

	/* Threading: MemTopo-thread only */
	NodeVersion myVersion;

	/* Threading: MemTopo-thread only */
	SCMessage_SPtr outgoingMemMessage;
	/* Threading: MemTopo-thread only */
	SCMessage_SPtr outgoingAttMessage;

	/* The membership view
	 * A hash map, (unordered_map)
	 * Threading: MemTopo-thread only */
	MembershipViewMap viewMap;

	/* The membership ring
	 * A tree map, ordered by the VID, value is NodeID
	 * Threading: MemTopo-thread only */
	MembershipRing ringSet;

	/* Update data-base, Alive, Suspected, and Left sets.
	 * Threading: MemTopo-thread only.
	 */
	UpdateDatabase updateDB;

	/*
	 * true for add, false for disconnect
	 */
	typedef std::deque<std::pair<NodeIDImpl_SPtr,bool> > NeighborsChangeQ;

	/* New neighbors queue.
	 * Threading: MemTopo-thread only.
	 */
	NeighborsChangeQ neighborsChangeQ;

	/*
	 * Threading: MemTopo-thread, App.-thread (through MembershipService).
	 */
	AttributeManager attributeManager;

	typedef std::deque<std::pair<NodeIDImpl_SPtr,int64_t> > ClearRetainAttrQ;
	ClearRetainAttrQ clearRetainAttrQ_;

	/* High priority monitoring
	 * Threading: MemTopo-thread only.
	 */
	HighPriorityMonitor_SPtr highPriorityMonitor_SPtr;

	//=== Leader election =====================================================

	/* Internal membership consumer.
	 * Registered when instance is created.
	 */
	leader_election::LEViewKeeper_SPtr leaderElectionViewKeeper_SPtr;

	/*
	 *
	 */
	leader_election::LECandidate_SPtr leaderElectionCandidate_SPtr;

	//=== Internal membership consumers =======================================

	/*
	 * Threading - written during construction, read by MemTopoThread after
	 * start, no need to lock.
	 *
	 * First: an array of pointer to listener
	 * Second: is first view delivered?
	 */
	std::vector<boost::shared_ptr<SCMembershipListener> > intMemConsumer_Vec;
	bool intMemConsumer_FirstViewDelivered;

	//=== private methods =====================================================

	void newNeighborTask(NodeIDImpl_SPtr id);
	void disconnectedNeighborTask(NodeIDImpl_SPtr id);

	//=== membership event notification ===
	void notifyLeave(NodeIDImpl_SPtr id, const NodeVersion& ver, spdr::event::NodeStatus status, AttributeTable_SPtr attributeTable);
	void notifyJoin(NodeIDImpl_SPtr id, int64_t incarnationNumber, event::AttributeMap_SPtr attrMap);
	void notifyChangeOfMetadata();
	void notifyViewChange();


	//=== protocol ===
	/**
	 * Process a leave message.
	 *
	 * @param name
	 * @param ver
	 * @param status
	 * @return true if view changed
	 */
	bool processMsgLeave(StringSPtr name, const NodeVersion& ver, spdr::event::NodeStatus node_status);

	/**
	 *
	 * @param membershipMsg
	 * @return true if view changed
	 */
	bool processMsgNodeUpdate(SCMessage_SPtr membershipMsg);

	bool viewProcessSuspicion(
			StringSPtr reportingName,
			StringSPtr suspectName,
			NodeVersion suspectVer);
	bool viewMergeAliveNode(NodeIDImpl_SPtr id, const NodeVersion& ver);
	bool viewAddNode(NodeIDImpl_SPtr id, const NodeVersion& ver);
	bool viewRemoveNode(NodeIDImpl_SPtr id, const NodeVersion& ver, spdr::event::NodeStatus status);
	bool historyProcessRetain(NodeIDImpl_SPtr id, NodeVersion ver, spdr::event::NodeStatus status);

	void refreshSuccessorList();

	/*
	 * Send leave to all neighbors.
	 */
	void sendLeaveMsg(int32_t exitCode);

	void sendLeaveAckMsg(NodeIDImpl_SPtr target);

	/*
	 * Prepare the format of a leave message
	 */
	void prepareLeaveMsg(SCMessage_SPtr msg, int32_t exitCode);

	/*
	 * Prepare a full view message (for a new neighbor)
	 */
	void prepareFullViewMsg(SCMessage_SPtr msg);

	/*
	 * Prepare an update view message (for an old neighbor)
	 */
	void prepareUpdateViewMsg(SCMessage_SPtr msg);

	void verifyCRC(SCMessage_SPtr msg, const String& method, const String& desc);

};

} //namespace spdr

#endif /* MEMBERSHIPMANAGERIMPL_H_ */
