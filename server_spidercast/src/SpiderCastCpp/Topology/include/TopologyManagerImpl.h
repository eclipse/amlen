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
 * TopologyManagerImpl.h
 *
 *  Created on: 16/03/2010
 *      Author:
 *
 * Version     : $Revision: 1.5 $
 *-----------------------------------------------------------------
 * $Log: TopologyManagerImpl.h,v $
 * Revision 1.5  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.4.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.4  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.3  2015/08/06 06:59:15 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.2  2015/08/04 12:19:11 
 * split brain bug fix
 *
 * Revision 1.1  2015/03/22 12:29:18 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.53  2015/01/26 12:38:15 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.52  2014/11/04 09:51:25 
 * Added multicast discovery
 *
 * Revision 1.51  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.50  2012/02/27 13:41:09 
 * typedef AbstractTask_SPtr, forward declare in CoreInterface
 *
 * Revision 1.49  2012/02/14 14:32:24 
 * Fix a bug in which successor was reported as suspect due to bi-directional  simultaneous connection attempts
 *
 * Revision 1.48  2012/02/14 12:45:18 
 * Add structured topology
 *
 * Revision 1.47  2011/06/22 11:12:28 
 * *** empty log message ***
 *
 * Revision 1.46  2011/06/19 07:56:36 
 * Add support for UDP discovery
 *
 * Revision 1.45  2011/05/30 11:01:31 
 * Call reportSuspect the first time a failure to connect to the successor is encountered
 *
 * Revision 1.44  2011/03/22 09:20:14 
 * Add a list for neighbors waiting to be disconnected
 *
 * Revision 1.43  2011/03/16 13:37:36 
 * Add connectivity event; fix a couple of bugs in topology
 *
 * Revision 1.42  2011/03/03 11:24:56 
 * Add a predecessor to setSuccessor()
 *
 * Revision 1.41  2011/02/20 12:01:01 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.40  2011/01/09 09:12:58 
 * Hierarchy branch merged in to HEAD
 *
 * Revision 1.39  2010/12/26 07:47:44 
 * Remove ReturningCompletionListener, and _incomingConnectionRequests
 *
 * Revision 1.38  2010/12/14 09:57:23 
 * Remove discovery disconnect and disconnect OK msgs and periodic task
 *
 * Revision 1.37  2010/12/05 19:03:27 
 * Close discovery connection right after receiving and processing the reply; Don't send nodeIds on topo msgs, just target string to check by the other side; Save a list to whom I sent a disconnect request, and don't send again before I receive a response
 *
 * Revision 1.36  2010/12/01 18:53:30 
 * Add discovery reply send task
 *
 * Revision 1.35  2010/11/29 14:10:07 
 * change std::map to boost::unordered_map
 *
 * Revision 1.34  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.32.2.3  2010/11/25 14:42:11 
 * minor changes after code review and merge with HEAD
 *
 * Revision 1.32.2.2  2010/11/23 14:12:05 
 * merged with HEAD branch
 *
 * Revision 1.33  2010/11/22 12:52:18 
 * Refined locking such that no lock is held while calling COMM; Deal with recently disconnected neighbors; enhanced tracing
 *
 * Revision 1.32.2.1  2010/10/21 16:10:43 
 * Massive comm changes
 *
 * Revision 1.32  2010/08/26 18:59:35 
 * Make "random disconnect" and "update degree" seperate and independent tasks. Eliminat etopology periodic task
 *
 * Revision 1.31  2010/08/26 11:15:57 
 * Make "random connect" a seperate and independent task
 *
 * Revision 1.30  2010/08/26 08:34:03 
 * Make "change successor" a seperate and independent task
 *
 * Revision 1.29  2010/08/24 13:54:49 
 * Enable multiple connection creations to successors (different successors)
 *
 * Revision 1.28  2010/08/23 09:22:47 
 * Grab the number of random connections aimed and slack from the config object
 *
 * Revision 1.27  2010/08/22 13:22:07 
 * Switch onSuccess to receive as a paramtere CSP(Neighbor) rather than (Neighbor *)
 *
 * Revision 1.26  2010/08/18 10:14:37 
 * Discovery -  runs as independent task, runs in two modes - normal and frequent discovery.
 *
 * Revision 1.25  2010/08/11 09:07:32 
 * Rename recentlyDisconnected to recentlyrejected
 *
 * Revision 1.24  2010/08/03 13:58:44 
 * Add support for on_success and on_failure connection events to be passed in through the incoming msg Q
 *
 * Revision 1.23  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.22  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.21  2010/06/27 12:02:31 
 * No functional changes
 *
 * Revision 1.20  2010/06/22 12:25:14 
 * Re-factor
 *
 * Revision 1.19  2010/06/20 14:42:56 
 * Handle termination better. Add grace parameter
 *
 * Revision 1.18  2010/06/17 08:20:28 
 * Add disconnect type and remove entry from future candidates map
 *
 * Revision 1.17  2010/06/16 08:13:40 
 * Add neighbors degree related information
 *
 * Revision 1.16  2010/06/14 15:55:03 
 * Add stopFrequentDiscovery task
 *
 * Revision 1.15  2010/06/08 15:46:25 
 * changed instanceID to String&
 *
 * Revision 1.14  2010/06/06 12:45:41 
 * Report suspect when several connection attempts to the successor fail
 *
 * Revision 1.13  2010/06/03 12:28:05 
 * No functional change
 *
 * Revision 1.12  2010/06/03 11:36:29 
 * Use a single scMessage and buffer for outgoing topology messages
 *
 * Revision 1.11  2010/06/03 10:01:45 
 * Make sure that msg.updateTotalLength is always called
 *
 *
 */

#ifndef TOPOLOGYMANAGERIMPL_H_
#define TOPOLOGYMANAGERIMPL_H_

#include "TopologyManager.h"
#include "SpiderCastConfigImpl.h"
#include "CoreInterface.h"
#include "NodeIDCache.h"
#include "SCMessage.h"
#include "StopInitialDiscoveryPeriodTask.h"
#include "TopologyTerminationTask.h"
#include "DiscoveryPeriodicTask.h"
#include "TopologyChangeSuccessorTask.h"
#include "TopologyRandomConnectTask.h"
#include "TopologyStructuredConnectTask.h"
#include "TopologyStructuredRefreshTask.h"
#include "TopologyRandomDisconnectTask.h"
#include "TopologyUpdateDegreeTask.h"
#include "TopologyDiscoveryReplySendTask.h"
#include "ConnectivityEvent.h"
#include "OutgoingStructuredNeighborTable.h"
#include "RoutingManager.h"
#include "HierarchyManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

//#include <map>
#include <boost/unordered_map.hpp>
#include <list>

namespace spdr
{
class TopologyManagerImpl: public TopologyManager,
		ScTraceContext
{
public:
	TopologyManagerImpl(const String& instID,
			const SpiderCastConfigImpl& config, NodeIDCache& cache,
			CoreInterface& coreInterface);

	virtual ~TopologyManagerImpl();

	void start();

	void terminate(bool soft);

	/*
	 * @see spdr::TopologyManager
	 */
	void setSuccessor(NodeIDImpl_SPtr successor, NodeIDImpl_SPtr predecessor);

	/*
	 * @see spdr::TopologyManager
	 */
	NeighborTable_SPtr getNeighborTable();

	/*
	 * @see spdr::TopologyManager
	 */
	void discoveryTask();

	void stopFrequentDiscoveryTask();

	void terminationTask();

	void changeSuccessorTask();

	void randomTopologyConnectTask();

	void randomTopologyDisconnectTask();

	void structuredTopologyConnectTask();

	void structuredTopologyRefreshTask();

	void changedDegreeTask();

	void discoveryReplySendTask();

	void multicastDiscoveryReplySendTask();

	//===============================================================

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

	void processIncomingTopologyMessage(SCMessage_SPtr topologyMsg);

	// Methods from ConnectionsAsyncCompletionListener
	//void onSuccess(Neighbor *result);
	void onSuccess(Neighbor_SPtr result, int ctx);
	void onFailure(String failedTargetName, int rc, String message, int ctx);

	void startFrequentDiscovery();
	void stopFrequentDiscovery();
	bool isInFrequentDiscovery();

private:

	static ScTraceComponent* _tc;
	const String _instID;
	const SpiderCastConfigImpl& _config;
	NodeIDImpl_SPtr _myNodeId;
	CoreInterface& _coreInterface;

	NodeIDCache& _nodeIdCache;
	NeighborTable_SPtr _neighborTable;

	NeighborTable_SPtr _incomingStructuredNeighborTable;
	boost::shared_ptr<OutgoingStructuredNeighborTable> _outgoingStructuredNeighborTable;

	/* set by core to signal orderly shutdown
	 * Threading: App-thread, Mem-thread
	 */
	volatile State _state;

	MembershipManager_SPtr _memMgr_SPtr;
	boost::shared_ptr<route::RoutingManager> _routingMgr_SPtr;
	CommAdapter_SPtr _commAdapter_SPtr;
	HierarchyManager_SPtr _hierarchyMngr_SPtr;
	TaskSchedule_SPtr _taskSchedule_SPtr;
	AbstractTask_SPtr _discoveryTask_SPtr;
	AbstractTask_SPtr _stopFrequentDiscoveryTask_SPtr;
	AbstractTask_SPtr _changeSuccessorTask_SPtr;
	AbstractTask_SPtr _randomConnectTask_SPtr;
	AbstractTask_SPtr _randomDisConnectTask_SPtr;
	AbstractTask_SPtr _updateDegreeTask_SPtr;
	AbstractTask_SPtr _discoveryReplySendTask_SPtr;
	AbstractTask_SPtr _structuredConnectTask_SPtr;
	AbstractTask_SPtr _structuredRefreshTask_SPtr;



	/* lock when using multi-threaded variables
	 */
	boost::recursive_mutex topo_mutex;

	boost::posix_time::time_duration _frequentDiscoveryPeriod;
	boost::posix_time::time_duration _normalDiscoveryPeriod;
	boost::posix_time::time_duration _frequentDiscoveryDuration;

	SCMessage_SPtr _topoMessage;
	std::vector<SCMessage_SPtr> _udpDiscoveryMessage;

	enum DisconnectType
	{
		SUCCESSOR = 0, RANDOM, STRUCTURED
	};

	NeighborTable_SPtr _discoveryMap;
	NeighborTable_SPtr _outgoingRandomConnectionRequests;
	NodeIDImpl_SPtr _currentSuccessor;
	NodeIDImpl_SPtr _setSuccessor;
	NodeIDImpl_SPtr _currentPredecessor;
	int _numRandomNeighbors;
	int _numFailedSetSuccessorAttempts;
	static const int numFailedSuccessorAttemptAllowed = 1;
	bool _frequentDiscovery;

	//typedef std::map<NodeIDImpl_SPtr , short, NodeIDImpl_SPtr::Less> NeighborsDegreeMap;
	typedef boost::unordered_map<NodeIDImpl_SPtr, short, NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals> NeighborsDegreeMap;
	NeighborsDegreeMap _neighborsDegreeMap;

	bool _myDegreeChanged;
	std::list<NodeIDImpl_SPtr >
			_candidatesForFutureRandomConnectionAttempts;
	bool _softClosed;
	bool _changeSuccessorTaskScheduled;
	bool _randomConnectTaskScheduled;
	bool _randomDisConnectTaskScheduled;
	bool _updateDegreeTaskScheduled;
	bool _discoveryReplySendTaskScheduled;
	bool _multicastDiscoveryReplySendTaskScheduled;
	bool _structuredConnectTaskScheduled;
	bool _structuredRefreshTaskScheduled;

	std::list<Neighbor_SPtr > _discoveryReplySendListTCP;
	std::list<NodeIDImpl_SPtr>  _discoveryReplySendListUDP;
	int _discoveryReplySendCountMulticast;

	std::list<NodeIDImpl_SPtr >
				_recentlyDisconnected;

	std::list<NodeIDImpl_SPtr > _recentlySentDisconnectRequest;

	std::list <Neighbor_SPtr > _waitingConnectionBreakList;

	short _numRingNeighborsReported;
	short _numRandomNeighborsReported;
	short _numOutgoingStructuredNeighborsReported;
	short _numIncomingStructuredNeighborsReported;

	//TCP, UDP discovery are mutually exclusive
	bool _isTCPDiscovery;
	bool _isUDPDiscovery;
	//Multicast discovery works together with p2p discovery
	bool _isMulticastDiscovery;

	void addEntryToNeighborsDegreeMap(NodeIDImpl_SPtr nodeName,
			short degree);
	void removeEntryFromNeighborsDegreeMap(
			NodeIDImpl_SPtr nodeName);
	void removeEntryFromCandidatesForFutureRandomConnectionAttemptsList(
			NodeIDImpl_SPtr nodeName);
	void removeEntryFromRecentlyDisconnectedList(
				NodeIDImpl_SPtr nodeName);
	void removeEntryFromRecentlySentDisconnectRequestList(
				NodeIDImpl_SPtr nodeName);

	void removeEntryFromWaitingConnectionBreakList(NodeIDImpl_SPtr nodeName);

	void sendLeaveMsg();

	void discoveryTaskImpl();

	void discoveryMulticastTaskImpl();

	void processIncomingDiscoveryRequestMsg(SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);
	void processIncomingDiscoveryReplyMsg(SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);

	void processIncomingDiscoveryRequestMulticastMsg(
			SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName);
	void processIncomingDiscoveryReplyMulticastMsg(
			SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName);

	void processIncomingDiscoveryRequestUDPMsg(SCMessage_SPtr incomingTopologyMsg,
				NodeIDImpl_SPtr peerName);
	void processIncomingDiscoveryReplyUDPMsg(SCMessage_SPtr incomingTopologyMsg,
				NodeIDImpl_SPtr peerName);


	void processIncomingConnectSuccessorMsg(SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);
	void processIncomingConnectSuccessorOkMsg(
			SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);
	void processIncomingDisconnectRequestMsg(
			SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);
	void processIncomingDisconnectReplyMsg(SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);
	void processIncomingNodeLeaveMsg(SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);
	void processIncomingConnectRequestMsg(SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);
	void processIncomingConnectReplyMsg(SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);
	void processIncomingConnectStructuredRequestMsg(SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);
	void processIncomingConnectStructuredReplyMsg(SCMessage_SPtr incomingTopologyMsg,
				NodeIDImpl_SPtr peerName);
	void processIncomingDegreeChangedMsg(SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);
	void processIncomingCommEventMsg(SCMessage_SPtr incomingTopologyMsg,
			NodeIDImpl_SPtr peerName);

	void myDegreeChanged();
	bool isRecentlyDisconnected(NodeIDImpl_SPtr target);
	bool isInRecentlySentDisconnectRequestList(NodeIDImpl_SPtr target);

	void submitConnectivityEvent();

	bool structuredTopoRefreshNeeded();
};

}

#endif /* TOPOLOGYMANAGERIMPL_H_ */
