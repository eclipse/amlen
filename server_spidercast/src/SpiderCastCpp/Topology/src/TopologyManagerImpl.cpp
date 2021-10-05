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
 * TopologyManagerImpl.cpp
 *
 *  Created on: 16/03/2010
 *      Author:
 *
 * Version     : $Revision: 1.17 $
 *-----------------------------------------------------------------
 * $Log: TopologyManagerImpl.cpp,v $
 * Revision 1.17  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.16.2.3  2016/02/07 19:50:25 
 * degree to high, ignore for now, fix later
 *
 * Revision 1.16.2.2  2016/01/27 09:06:53 
 * nameless tcp discovery
 *
 * Revision 1.16.2.1  2016/01/21 14:52:47 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.16  2016/01/07 14:41:18 
 * bug fix: closeStream with virgin RumNeighbor
 *
 * Revision 1.15  2015/11/30 09:42:00 
 * prevent double leave on a duplicate node suspicion
 *
 * Revision 1.14  2015/11/18 08:36:58 
 * Update copyright headers
 *
 * Revision 1.13  2015/10/09 06:56:11 
 * bug fix - when neighbor in the table but newNeighbor not invoked yet, don't send to all neighbors but only to routable neighbors. this way the node does not get a diff mem/metadata before the full view is received.
 *
 * Revision 1.12  2015/09/02 11:33:39 
 * nameless udp discovery
 *
 * Revision 1.11  2015/08/27 10:53:53 
 * change RUM log level dynamically
 *
 * Revision 1.10  2015/08/06 06:59:15 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.9  2015/08/04 12:19:11 
 * split brain bug fix
 *
 * Revision 1.8  2015/07/30 12:14:35 
 * split brain
 *
 * Revision 1.7  2015/07/29 09:26:12 
 * split brain
 *
 * Revision 1.6  2015/07/20 06:47:33 
 * multiple EP in bootstrap
 *
 * Revision 1.5  2015/07/13 08:26:21 
 * *** empty log message ***
 *
 * Revision 1.4  2015/07/02 10:29:12 
 * clean trace, byte-buffer
 *
 * Revision 1.3  2015/06/25 10:26:46 
 * readiness for version upgrade
 *
 * Revision 1.2  2015/05/05 12:48:26 
 * Safe connect
 *
 * Revision 1.1  2015/03/22 12:29:13 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.122  2015/03/01 15:08:09 
 * fix bug in TopologyManagerImpl, avoid throwing when memManager pointer is null, it is legal during termination
 *
 * Revision 1.121  2015/01/26 12:38:15 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.120  2014/11/04 09:51:25 
 * Added multicast discovery
 *
 * Revision 1.119  2012/10/25 10:11:11 
 * Added copyright and proprietary notice
 *
 * Revision 1.118  2012/03/14 09:35:25 
 * comment on low priority bug
 *
 * Revision 1.117  2012/02/27 13:41:47 
 * typedef AbstractTask_SPtr
 *
 * Revision 1.116  2012/02/27 12:44:12 
 * Solve a bug which caused the same neighbor to be reported to the routing manager twice
 *
 * Revision 1.115  2012/02/23 09:40:16 
 * Ensure that the check for isROutable of neighbors within neighbor tables is made before the entry is removed from the table
 *
 * Revision 1.114  2012/02/19 09:07:12 
 * Add a "routbale" field to the neighbor tables
 *
 * Revision 1.113  2012/02/15 15:37:05 
 * Add a missing call to addRoutingNeighbor()
 *
 * Revision 1.112  2012/02/14 14:32:25 
 * Fix a bug in which successor was reported as suspect due to bi-directional  simultaneous connection attempts
 *
 * Revision 1.111  2012/02/14 12:45:18 
 * Add structured topology
 *
 * Revision 1.110  2011/08/07 12:14:53 
 * Report susepct when failing to send a message to the successor; add neighbor names list to connectivity event
 *
 * Revision 1.109  2011/07/10 18:04:29 
 * Increase minor version only when discovered from another nodes history
 *
 * Revision 1.108  2011/07/10 09:24:18 
 * Create an RUM Tx on a seperate thread (not thr RUM thread)
 *
 * Revision 1.107  2011/06/29 11:46:46 
 * *** empty log message ***
 *
 * Revision 1.106  2011/06/29 07:39:20 
 * *** empty log message ***
 *
 * Revision 1.105  2011/06/22 11:12:28 
 * *** empty log message ***
 *
 * Revision 1.104  2011/06/21 11:33:48 
 * Upon changing a successor, disconnect from the previous successor
 *
 * Revision 1.103  2011/06/20 11:40:24 
 * Add commUdp component; re-schedule a connect successor task in case of a connection failure
 *
 * Revision 1.102  2011/06/19 07:56:36 
 * Add support for UDP discovery
 *
 * Revision 1.101  2011/06/13 13:36:42 
 * Increase numver of discovery attempts
 *
 * Revision 1.100  2011/06/09 07:44:23 
 * Fix trace; no functional changes
 *
 * Revision 1.99  2011/06/01 13:36:42 
 * Add traces to find a deadlock
 *
 * Revision 1.98  2011/05/30 17:46:57 
 * Go through scheduleDelay delay parameters
 *
 * Revision 1.97  2011/05/30 11:01:31 
 * Call reportSuspect the first time a failure to connect to the successor is encountered
 *
 * Revision 1.96  2011/05/02 10:40:57 
 * Handle general comm events
 *
 * Revision 1.95  2011/05/01 08:39:00 
 * Move connection context from TopologyManager to Definitions.h
 *
 * Revision 1.94  2011/04/11 08:29:25 
 * Ensure that entries are inserted into the neighbors degree map only if a corresponding entry exists in the neighbor table
 *
 * Revision 1.93  2011/04/04 13:13:02 
 * Clean up neighbors, which were destined to be disconnected, once an "onBreak" is received
 *
 * Revision 1.92  2011/04/03 12:05:43 
 * In case a positie disconect reply received for the current successor, schedule a change successor task
 *
 * Revision 1.91  2011/03/31 14:05:06 
 * Fix NodeIdImplcomparisons; ensure the right action if the successor is about o be kicked out of the neighbor table
 *
 * Revision 1.90  2011/03/24 08:41:54 
 * write CRC after on each message
 *
 * Revision 1.89  2011/03/23 19:59:15 
 * fix an NPE
 *
 * Revision 1.88  2011/03/23 13:36:04 
 * Ensure that we send a complete nodeid when proposing to another node alternate nodes to connect to
 *
 * Revision 1.87  2011/03/22 09:20:31 
 * Add a list for neighbors waiting to be disconnected
 *
 * Revision 1.86  2011/03/16 13:37:36 
 * Add connectivity event; fix a couple of bugs in topology
 *
 * Revision 1.85  2011/03/03 11:24:56 
 * Add a predecessor to setSuccessor()
 *
 * Revision 1.84  2011/02/21 15:37:15 
 * Const AttributeValue and buffer within
 *
 * Revision 1.83  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.82  2011/02/07 14:58:44 
 * Take the correct mutex (topo rather than neighbor table)
 *
 * Revision 1.81  2011/02/02 10:08:30 
 * Convert asserts to runtimeExceptions
 *
 * Revision 1.80  2011/02/01 13:18:40 
 * *** empty log message ***
 *
 * Revision 1.79  2011/01/27 13:00:47 
 * Ensure that incoming connect request messages belong to the exact same zone (bus name)
 *
 * Revision 1.78  2011/01/10 12:26:42 
 * Remove context from connectOnExisting() method
 *
 * Revision 1.77  2011/01/09 14:03:15 
 * Added HierarchyCtx to connection-context
 *
 * Revision 1.76  2011/01/09 09:12:58 
 * Hierarchy branch merged in to HEAD
 *
 * Revision 1.75  2011/01/02 13:27:04 
 * Clean up code; no functional changes
 *
 * Revision 1.74  2010/12/29 13:21:15 
 * Add connect() context types; use in calls to connect() and return via onSuccess and onFailure
 *
 * Revision 1.73  2010/12/29 09:49:58 
 * Ignore the recentlyDisconnected list, as COMM should deal with these situations
 *
 * Revision 1.72  2010/12/27 13:25:27 
 * Allow connections to the same target be opened by different tasks
 *
 * Revision 1.71  2010/12/26 07:49:16 
 * Replace _incomingConnecionRequetsts by createOnExisting, and finalize setting correct streamId in neighbor objects such that connections may be properly closed by Comm
 *
 * Revision 1.70  2010/12/22 13:37:02 
 * Don't perform automatic neighbor creation on detection of a new source, rather use connectOnExisting()
 *
 * Revision 1.69  2010/12/19 14:45:05 
 * Changes to Topology - assiciation between transmiter and receiver inside neighbor
 *
 * Revision 1.68  2010/12/14 10:17:15 
 * send the same leave msg to all, so that the sendToAll method can be called
 *
 * Revision 1.67  2010/12/14 09:58:36 
 * Remove discovery disconnect and disconnect OK msgs and periodic task; consolidate some mutex lock calls
 *
 * Revision 1.66  2010/12/09 14:05:31 
 * Send update degree msg with no source and target info such that the same msg could be sent to all
 *
 * Revision 1.65  2010/12/05 19:03:28 
 * Close discovery connection right after receiving and processing the reply; Don't send nodeIds on topo msgs, just target string to check by the other side; Save a list to whom I sent a disconnect request, and don't send again before I receive a response
 *
 * Revision 1.64  2010/12/01 18:54:35 
 * Add discovery reply send task
 *
 * Revision 1.63  2010/11/29 14:10:07 
 * change std::map to boost::unordered_map
 *
 * Revision 1.62  2010/11/29 09:41:18 
 * Limit the amount of outgoing discovery links open at any point in time
 *
 * Revision 1.61  2010/11/28 18:43:16 
 * Take into consideration the outgoingRandomMap as well while deciding whethr to attempt to connect to additional neighbors or whether to accept an incoming connect request
 *
 * Revision 1.60  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.56.2.5  2010/11/25 14:42:11 
 * minor changes after code review and merge with HEAD
 *
 * Revision 1.56.2.4  2010/11/25 10:04:18 
 * before merge with head - all chages from head inside
 *
 * Revision 1.56.2.3  2010/11/23 14:12:05 
 * merged with HEAD branch
 *
 * Revision 1.59  2010/11/22 17:48:23 
 * getDiscoveryView to behave differently depending on the context it's called from
 *
 * Revision 1.58  2010/11/22 12:52:18 
 * Refined locking such that no lock is held while calling COMM; Deal with recently disconnected neighbors; enhanced tracing
 *
 * Revision 1.57  2010/10/27 12:42:31 
 * Close a loop hole in which a node rejected a peer as a random peer even though it was already a neighbor (or was in the process of asking its peer to be a random neighbor)
 *
 * Revision 1.56.2.2  2010/11/08 08:47:50 
 * fix
 *
 * Revision 1.56.2.1  2010/10/21 16:10:43 
 * Massive comm changes
 *
 * Revision 1.56  2010/10/12 18:13:48 
 * Support for hierarchy
 *
 * Revision 1.55  2010/10/10 09:36:20 
 * Reschedule the changeSuccessor task in case waiting for connection to close.
 *
 * Revision 1.54  2010/08/31 14:03:29 
 * more debug info in discoveryTaskImpl
 *
 * Revision 1.53  2010/08/26 18:59:35 
 * Make "random disconnect" and "update degree" seperate and independent tasks. Eliminat etopology periodic task
 *
 * Revision 1.52  2010/08/26 11:15:57 
 * Make "random connect" a seperate and independent task
 *
 * Revision 1.51  2010/08/26 08:34:04 
 * Make "change successor" a seperate and independent task
 *
 * Revision 1.50  2010/08/25 13:56:20 
 * Enable several successor connections to be performed in parallel
 *
 * Revision 1.49  2010/08/24 13:54:49 
 * Enable multiple connection creations to successors (different successors)
 *
 * Revision 1.48  2010/08/24 13:23:18 
 * Newly used sendToNeighbor has the opposite return code meaning than the previosu sendMessage
 *
 * Revision 1.47  2010/08/24 10:44:24 
 * While processing incoming connection requests consider only members of the neighbor table as current active neighbors
 *
 * Revision 1.46  2010/08/23 09:27:31 
 * Grab the number of random connections aimed and slack from the config object, as well as the periodic task period
 *
 * Revision 1.45  2010/08/22 14:23:37 
 * Use neighborTable->sendToNeighbor rather than neighbor->sendMessage
 *
 * Revision 1.44  2010/08/22 13:22:07 
 * Switch onSuccess to receive as a paramtere CSP(Neighbor) rather than (Neighbor *)
 *
 * Revision 1.43  2010/08/19 09:22:46 
 * Implement the sequence of neighborTable->adEntry and calls to newNeighbor() as agreed lately to make sure the attributes module works correctly
 *
 * Revision 1.42  2010/08/18 10:14:38 
 * Discovery -  runs as independent task, runs in two modes - normal and frequent discovery.
 *
 * Revision 1.41  2010/08/17 13:31:22 
 * Send and check node id fo source and target on each topology message
 *
 * Revision 1.40  2010/08/12 15:33:03 
 * Call newNeighbor from onSUccess of the new successor rather than from the processing of connect_successor_ok
 *
 * Revision 1.39  2010/08/11 09:08:21 
 * Rename recentlyDisconnected to recentlyrejected; ensure that a recently rejected node doesn't interfere with membership
 *
 * Revision 1.38  2010/08/10 09:53:17 
 * Add "neighbor table changed" trace messages
 *
 * Revision 1.37  2010/08/09 09:20:27 
 * While looking for discovery node potentials, don't count entries existing in other categories
 *
 * Revision 1.36  2010/08/04 13:18:18 
 * Hopefully solve NPE, decouple reportSuspect from setSuccessor
 *
 * Revision 1.35  2010/08/04 12:37:54 
 * Don't set _setSuccessor to Null after a successor failure
 *
 * Revision 1.34  2010/08/04 08:53:34 
 * Hopefully avoid a NPE
 *
 * Revision 1.33  2010/08/03 13:59:08 
 * Add support for on_success and on_failure connection events to be passed in through the incoming msg Q
 *
 * Revision 1.32  2010/08/02 17:13:33 
 * Some improvements to randomTopologyConnectTask()
 *
 * Revision 1.30  2010/07/15 09:01:32 
 * Call membership's disconnectedNeighbor() whenevr a node is removed from the neighbor table
 *
 * Revision 1.29  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.28  2010/07/01 18:32:33 
 * Change createConnection to call asyncCompletionListener->onFailure when failing immediately, and not already in the outpending list
 *
 * Revision 1.27  2010/07/01 11:55:47 
 * Rename NeighborTable::exists() to contains()
 *
 * Revision 1.26  2010/06/22 12:25:30 
 * Re-factor
 *
 * Revision 1.25  2010/06/21 13:14:52 
 * Add topology leave msg
 *
 * Revision 1.24  2010/06/21 07:40:57 
 * Currently disable recentlyDisconnected list
 *
 * Revision 1.23  2010/06/20 15:10:12 
 * Deal with case in which successor has changed by the time we receive onSuccess() on the previous successor. Close the connection
 *
 * Revision 1.22  2010/06/20 14:44:25 
 * Handle termination better.
 *
 * Revision 1.21  2010/06/17 11:45:29 
 * Add connection info to topology events. For debugging purposes
 *
 * Revision 1.20  2010/06/17 08:21:52 
 * Add a disconnect stage in the periodic task; handle disconnect better
 *
 * Revision 1.19  2010/06/16 08:14:22 
 * accepts K + H incoming connection request + handle neighbors degree information
 *
 * Revision 1.18  2010/06/14 17:43:47 
 * Initial random topology implementation
 *
 * Revision 1.17  2010/06/14 15:57:40 
 * Too many changes at once: frequent discovery task; discovery disconnect message; and initial random topology
 *
 * Revision 1.16  2010/06/08 15:46:26 
 * changed instanceID to String&
 *
 * Revision 1.15  2010/06/06 12:50:23 
 * Report suspect when several connection attempts to the successor fail; send disconnect message when a successor has changed within ::periodicTask
 *
 * Revision 1.14  2010/06/03 14:43:20 
 * Arrange print statmenets
 *
 * Revision 1.13  2010/06/03 12:28:44 
 * No functional change
 *
 * Revision 1.12  2010/06/03 11:36:29 
 * Use a single scMessage and buffer for outgoing topology messages
 *
 * Revision 1.11  2010/06/03 10:01:46 
 * Make sure that msg.updateTotalLength is always called
 *
 *
 */

#include <boost/algorithm/string/predicate.hpp>

#include "TopologyManagerImpl.h"
#include "WarningEvent.h"

using namespace std;

namespace spdr
{

using namespace trace;

ScTraceComponent* TopologyManagerImpl::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Topo,
		trace::ScTrConstants::Layer_ID_Topology, "TopologyManagerImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

TopologyManagerImpl::TopologyManagerImpl(const String& instID,
		const SpiderCastConfigImpl& config, NodeIDCache& cache,
		CoreInterface& coreInterface) :
	ScTraceContext(_tc, instID, config.getMyNodeID()->getNodeName()), _instID(
			instID), _config(config), _myNodeId(_config.getMyNodeID()),
			_coreInterface(coreInterface), _nodeIdCache(cache), _state(
					TopologyManager::Init), _udpDiscoveryMessage(),
			_currentSuccessor(), _setSuccessor(), _currentPredecessor(),
			_numRandomNeighbors(0), _numFailedSetSuccessorAttempts(0),
			_frequentDiscovery(true), _neighborsDegreeMap(), _myDegreeChanged(
					false), _candidatesForFutureRandomConnectionAttempts(),
			_softClosed(true), _changeSuccessorTaskScheduled(false),
			_randomConnectTaskScheduled(false), _randomDisConnectTaskScheduled(
					false), _updateDegreeTaskScheduled(false),
			_discoveryReplySendTaskScheduled(false),
			_multicastDiscoveryReplySendTaskScheduled(false),
			_structuredConnectTaskScheduled(false),
			_structuredRefreshTaskScheduled(false),
			_discoveryReplySendListTCP(),
			_discoveryReplySendListUDP(),
			_discoveryReplySendCountMulticast(0),
			_recentlyDisconnected(), _recentlySentDisconnectRequest(),
			_waitingConnectionBreakList(), _numRingNeighborsReported(-1),
			_numRandomNeighborsReported(-1),
			_numOutgoingStructuredNeighborsReported(-1),
			_numIncomingStructuredNeighborsReported(-1),
			_isTCPDiscovery(_config.isTCPDiscovery()),
			_isUDPDiscovery(_config.isUDPDiscovery()),
			_isMulticastDiscovery(_config.isMulticastDiscovery())
{
	Trace_Entry(this, "TopologyManagerImpl()");
	_neighborTable = NeighborTable_SPtr (new NeighborTable(
			_myNodeId->getNodeName(), "NeighborTable", _instID));
	_incomingStructuredNeighborTable = NeighborTable_SPtr (
			new NeighborTable(_myNodeId->getNodeName(),
					"incomingStructuredNeighborTable", _instID));
	_outgoingStructuredNeighborTable.reset(new OutgoingStructuredNeighborTable(
			_myNodeId->getNodeName(), "OutgoingStructuredNeighborTable",
			_instID));

	_discoveryMap = NeighborTable_SPtr (new NeighborTable(
			_myNodeId->getNodeName(), "DiscoveryMap", _instID));
	_outgoingRandomConnectionRequests = NeighborTable_SPtr (
			new NeighborTable(_myNodeId->getNodeName(),
					"OutgoingRandomConnectionRequests", _instID));
	_topoMessage = SCMessage_SPtr(new SCMessage);
	(*_topoMessage).setBuffer(ByteBuffer::createByteBuffer(1024));

	SCMessage_SPtr _udpMessage = SCMessage_SPtr(new SCMessage);
	(*_udpMessage).setBuffer(ByteBuffer::createByteBuffer(1024));

	_udpDiscoveryMessage.push_back(_udpMessage);

	_frequentDiscoveryPeriod = boost::posix_time::milliseconds(
			_config.getFrequentDiscoveryIntervalMillis());
	_normalDiscoveryPeriod = boost::posix_time::milliseconds(
			_config.getNormalDiscoveryIntervalMillis());
	_frequentDiscoveryDuration = boost::posix_time::milliseconds(
			_config.getFrequentDiscoveryMinimalDurationMillis());

	Trace_Exit(this, "TopologyManagerImpl");
	//do not call method on coreInterface in ctor
}

TopologyManagerImpl::~TopologyManagerImpl()
{
	Trace_Entry(this, "~TopologyManagerImpl");
}

void TopologyManagerImpl::start()
{
	Trace_Entry(this, "start()");
	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		if (_state != TopologyManager::Init)
		{
			String what = "TopologyManagerImpl in state "
					+ TopologyManager::nodeStateName[_state]
					+ " while calling start()";
			throw SpiderCastLogicError(what);
		}

		_state = TopologyManager::Discovery;
	}

	_taskSchedule_SPtr->scheduleDelay(_stopFrequentDiscoveryTask_SPtr,
			_frequentDiscoveryDuration);
	_taskSchedule_SPtr->scheduleDelay(_discoveryTask_SPtr,
			TaskSchedule::ZERO_DELAY);
	Trace_Exit(this, "start");
}

void TopologyManagerImpl::terminate(bool soft)
{
	Trace_Entry(this, "terminate()");

	if (ScTraceBuffer::isDebugEnabled(_tc))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "terminate()");
		buffer->addProperty<bool> ("soft", soft);
		buffer->invoke();
	}

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		if (_state == TopologyManagerImpl::Closed)
		{
			String errMsg("Unexpected state in terminate(): closed");
			Trace_Event(this, "terminate()", errMsg);
			throw SpiderCastRuntimeError(errMsg);
		}

		Trace_Event(this, "terminate", "About to schedule a termination task");
		_softClosed = soft;
		if (_state != TopologyManagerImpl::Init)
		{
			//schedule a termination task to send leave messages and clean-up
			_taskSchedule_SPtr->scheduleDelay(
					AbstractTask_SPtr (
							new TopologyTerminationTask(_coreInterface)),
					TaskSchedule::ZERO_DELAY);
		}
		Trace_Event(this, "terminate", "after scheduling a termination task");

		_state = TopologyManager::Closed;
	}

	Trace_Exit(this, "terminate()");
}

void TopologyManagerImpl::setSuccessor(NodeIDImpl_SPtr successor,
		NodeIDImpl_SPtr predecessor)
{
	Trace_Entry(this, "setSuccessor()", "successor", toString(successor),
			"predecessor", toString(predecessor));

	boost::recursive_mutex::scoped_lock lock(topo_mutex);
	if (_state == TopologyManagerImpl::Closed)
	{
		Trace_Event(this, "setSuccessor()",
				"returning immediately because closed");
		return;
	}

	_currentPredecessor = predecessor;

	if (successor)
	{
		Trace_Event(this, "setSuccessor()", "Setting new successor", "node",
				successor->getNodeName());
		_state = TopologyManager::Normal;
		if (_setSuccessor)
		{
			if (_setSuccessor->getNodeName().compare(successor->getNodeName()))
			{
				_numFailedSetSuccessorAttempts = 0;
			}
		}
		else
		{
			_numFailedSetSuccessorAttempts = 0;
		}
	}
	else
	{
		Trace_Event(this, "setSuccessor()", "Setting to NULL");
		startFrequentDiscovery();
		//		_state = TopologyManager::Discovery;
		_numFailedSetSuccessorAttempts = 0;
		_currentSuccessor = NodeIDImpl_SPtr();
	}
	_setSuccessor = successor;

	bool schedule = true;
	if (_currentSuccessor && _setSuccessor)
	{
		if ((*_setSuccessor) == (*_currentSuccessor))
		{
			schedule = false;
			Trace_Event(
					this,
					"setSuccessor()",
					"_setSuccessor == _currentSuccessor, skip scheduling a change successor task",
					"set", toString<NodeIDImpl> (_setSuccessor), "current",
					toString<NodeIDImpl> (_currentSuccessor));
		}
	}

	if (_setSuccessor && schedule && !_changeSuccessorTaskScheduled && _state
			!= TopologyManager::Closed)
	{
		_taskSchedule_SPtr->scheduleDelay(_changeSuccessorTask_SPtr,
				TaskSchedule::ZERO_DELAY);
		_changeSuccessorTaskScheduled = true;
		Trace_Event(this, "setSuccessor()",
				"scheduling a change successor task");
	}
	else
	{
		Trace_Event(this, "setSuccessor()", "skipping a change successor task");
	}

	if (_setSuccessor && !_currentSuccessor && !_randomConnectTaskScheduled
			&& (_neighborTable->size() < _config.getRandomDegreeTarget() + 2)
			&& _state != TopologyManager::Closed)
	{
		_taskSchedule_SPtr->scheduleDelay(_randomConnectTask_SPtr,
				TaskSchedule::ZERO_DELAY);
		_randomConnectTaskScheduled = true;
		Trace_Event(this, "setSuccessor()", "scheduling a random connect task");
	}
	else
	{
		Trace_Event(this, "setSuccessor()", "skipping a random connect task");
	}

	if (_setSuccessor && !_currentSuccessor && !_structuredConnectTaskScheduled
			&& _outgoingStructuredNeighborTable->size()
					< _config.getStructDegreeTarget() && _state
			!= TopologyManager::Closed && _config.isStructTopoEnabled())
	{
		_taskSchedule_SPtr->scheduleDelay(_structuredConnectTask_SPtr,
				TaskSchedule::ZERO_DELAY);
		_structuredConnectTaskScheduled = true;
		Trace_Event(this, "setSuccessor()",
				"scheduling a structured connect task");
	}
	else
	{
		Trace_Event(this, "setSuccessor()",
				"skipping a structured connect task");
	}

	if (!_structuredRefreshTaskScheduled && structuredTopoRefreshNeeded()
			&& _state != TopologyManager::Closed
			&& _config.isStructTopoEnabled())
	{
		_taskSchedule_SPtr->scheduleDelay(_structuredRefreshTask_SPtr,
				TaskSchedule::ZERO_DELAY);
		_structuredRefreshTaskScheduled = true;
		Trace_Event(this, "setSuccessor()",
				"scheduling a structured refresh task");
	}

	submitConnectivityEvent();

	Trace_Exit(this, "setSuccessor()");
}

NeighborTable_SPtr TopologyManagerImpl::getNeighborTable()
{
	return _neighborTable;
}

void TopologyManagerImpl::init()
{
	_memMgr_SPtr = _coreInterface.getMembershipManager();
	_routingMgr_SPtr = _coreInterface.getRoutingManager();
	_commAdapter_SPtr = _coreInterface.getCommAdapter();
	_hierarchyMngr_SPtr = _coreInterface.getHierarchyManager();
	_taskSchedule_SPtr = _coreInterface.getTopoMemTaskSchedule();
	_stopFrequentDiscoveryTask_SPtr = AbstractTask_SPtr (
			new StopInitialDiscoveryPeriodTask(_coreInterface));
	_discoveryTask_SPtr = AbstractTask_SPtr (
			new DiscoveryPeriodicTask(_coreInterface));
	_changeSuccessorTask_SPtr = AbstractTask_SPtr (
			new TopologyChangeSuccessorTask(_coreInterface));
	_randomConnectTask_SPtr = AbstractTask_SPtr (
			new TopologyRandomConnectTask(_coreInterface));
	_randomDisConnectTask_SPtr = AbstractTask_SPtr (
			new TopologyRandomDisconnectTask(_coreInterface));
	_updateDegreeTask_SPtr = AbstractTask_SPtr (
			new TopologyUpdateDegreeTask(_coreInterface));
	_discoveryReplySendTask_SPtr = AbstractTask_SPtr (
			new TopologyDiscoveryReplySendTask(_coreInterface));
	_structuredConnectTask_SPtr = AbstractTask_SPtr (
			new TopologyStructuredConnectTask(_coreInterface));
	_structuredRefreshTask_SPtr = AbstractTask_SPtr (
			new TopologyStructuredRefreshTask(_coreInterface));
}

//reset all shared pointers to break cycles.
void TopologyManagerImpl::destroyCrossRefs()
{
	Trace_Entry(this, "destroyCrossRefs()");

	_memMgr_SPtr.reset();
	_routingMgr_SPtr.reset();
	_commAdapter_SPtr.reset();
	_hierarchyMngr_SPtr.reset();
	_taskSchedule_SPtr.reset();
	_stopFrequentDiscoveryTask_SPtr.reset();
	_discoveryTask_SPtr.reset();
	_changeSuccessorTask_SPtr.reset();
	_randomConnectTask_SPtr.reset();
	_randomDisConnectTask_SPtr.reset();
	_updateDegreeTask_SPtr.reset();
	_discoveryReplySendTask_SPtr.reset();
	_structuredConnectTask_SPtr.reset();
	_structuredRefreshTask_SPtr.reset();
}

void TopologyManagerImpl::discoveryTask()
{
	Trace_Entry(this, "discoveryTask()");
	discoveryTaskImpl(); //Only p2p (TCP/UDP)
	discoveryMulticastTaskImpl(); //Only Multicast

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		if (_state == TopologyManager::Discovery || _frequentDiscovery)
		{
			Trace_Debug(this, "discoveryTask()",
					"Scheduling discovery task as frequent", "Task",
					_discoveryTask_SPtr->toString());
			_taskSchedule_SPtr->scheduleDelay(_discoveryTask_SPtr,
					_frequentDiscoveryPeriod);
		}
		else
		{
			Trace_Debug(this, "discoveryTask()",
					"Scheduling discovery task as normal", "Task",
					_discoveryTask_SPtr->toString());
			_taskSchedule_SPtr->scheduleDelay(_discoveryTask_SPtr,
					_normalDiscoveryPeriod);
		}
	}
	Trace_Exit(this, "discoveryTask()");
}

void TopologyManagerImpl::discoveryTaskImpl()
{
	Trace_Entry(this, "discoveryTaskImpl()");

	NodeIDImpl_SPtr firstNodeId;

	int max_discovery_attempts = std::max(3, _config.getRandomDegreeTarget() * _config.getRandomDegreeTarget());
	int num_discovery_attempts = max_discovery_attempts - _discoveryMap->size();
	for (int counter = 0; counter < num_discovery_attempts; counter++)
	{
		if (!_memMgr_SPtr)
		{
			Trace_Event(this, "discoveryTaskImpl()", "invalid _memMgr_SPtr");
			break;
		}

		std::pair<NodeIDImpl_SPtr, bool> discoveryNode =
				_memMgr_SPtr->getDiscoveryNode();

		if (!firstNodeId)
		{
			firstNodeId = discoveryNode.first;
		}
		else if (discoveryNode.first)
		{
			if (firstNodeId->deepEquals(*(discoveryNode.first)))
			{
				Trace_Debug(this, "discoveryTaskImpl()",
						"Done. Received twice the same discovery node");
				break;
			}
		}

		if (!discoveryNode.first)
		{
			Trace_Debug(this, "discoveryTaskImpl()", "skipping a NULL discovery node");
			continue;
		}

		if (_isTCPDiscovery)
		{
			boost::recursive_mutex::scoped_lock lock(topo_mutex);

			if (!discoveryNode.first->isNameANY())
			{
				if (_discoveryMap->contains(discoveryNode.first))
				{
					Trace_Debug(this, "discoveryTaskImpl()", "skipping a discovery node, pending request",
							"candidate", discoveryNode.first->toString());
					counter--;
					continue;
				}
				else
				{
					Trace_Debug(this, "discoveryTaskImpl()", "adding a discovery node",
							"candidate", discoveryNode.first->toString());
					_discoveryMap->addEntry(discoveryNode.first, Neighbor_SPtr ());

					if (!_commAdapter_SPtr->connect(discoveryNode.first, DiscoveryCtx))
					{
						Trace_Event(this, "discoveryTaskImpl()", "connect request failed, removing",
								"target", discoveryNode.first->toString());
						_discoveryMap->removeEntry(discoveryNode.first);
					}
					else
					{
						Trace_Debug(this, "discoveryTaskImpl()", "connect request OK",
								"target", discoveryNode.first->toString());
					}
				}
			}
			else
			{
				//TODO Nameless_TCP_Discovery
				String namelessTargetName = NodeID::NAME_ANY + "@" + discoveryNode.first->getNetworkEndpoints().toNameString();
				NodeIDImpl_SPtr namelessTargetID(new NodeIDImpl(namelessTargetName,discoveryNode.first->getNetworkEndpoints()));
				if (_discoveryMap->contains(namelessTargetID))
				{
					Trace_Debug(this, "discoveryTaskImpl()", "skipping a nameless discovery node, pending request",
							"candidate", namelessTargetID->toString());
					counter--;
					continue;
				}
				else
				{
					Trace_Debug(this, "discoveryTaskImpl()", "adding a nameless discovery node",
							"candidate", namelessTargetID->toString());
					_discoveryMap->addEntry(namelessTargetID, Neighbor_SPtr ());

					if (!_commAdapter_SPtr->connect(namelessTargetID, DiscoveryCtx))
					{
						Trace_Event(this, "discoveryTaskImpl()", "connect request failed, removing",
								"target", namelessTargetID->toString());
						_discoveryMap->removeEntry(namelessTargetID);
					}
					else
					{
						Trace_Debug(this, "discoveryTaskImpl()", "connect request OK",
								"target", namelessTargetID->toString());
					}
				}
			}
		}



		if (_isUDPDiscovery)
		{
			Trace_Debug(this, "discoveryTaskImpl()",
					"sending a Type_Topo_Discovery_Request_UDP to", "node",
					discoveryNode.first->toString());

			_udpDiscoveryMessage[0]->writeH1Header(
					SCMessage::Type_Topo_Discovery_Request_UDP);

			ByteBuffer& bb = *(*(_udpDiscoveryMessage[0])).getBuffer();

			bb.writeString(_config.getBusName());
			bb.writeString(_myNodeId->getNodeName());
			bb.writeLong(_coreInterface.getIncarnationNumber());
			//TODO SplitBrain
			bb.writeBoolean(discoveryNode.second); //isBootstrap
			_memMgr_SPtr->getDiscoveryViewPartial(_udpDiscoveryMessage[0], 0);
			_udpDiscoveryMessage[0]->updateTotalLength();
			if (_config.isCRCMemTopoMsgEnabled())
			{
				_udpDiscoveryMessage[0]->writeCRCchecksum();
			}
			if (!_commAdapter_SPtr->sendTo(discoveryNode.first,
					_udpDiscoveryMessage[0]))
			{
				Trace_Debug(this, "discoveryTaskImpl()",
						"couldn't send a message to",
						"node",spdr::toString(discoveryNode.first));
			}
		}
	} //for (num_discovery_attemps)
	Trace_Exit(this, "discoveryTaskImpl()");
}

void TopologyManagerImpl::discoveryMulticastTaskImpl()
{
	Trace_Entry(this, "discoveryMulticastTaskImpl()");

	if (_isMulticastDiscovery)
	{
		Trace_Debug(this, "discoveryMulticastTaskImpl()",
				"sending a Type_Topo_Discovery_Request_Multicast");

		bool bootstrap = false;

		{
			boost::recursive_mutex::scoped_lock lock(topo_mutex);
			bootstrap = (_state == TopologyManager::Discovery || _frequentDiscovery);
		}

		_udpDiscoveryMessage[0]->writeH1Header(
				SCMessage::Type_Topo_Discovery_Request_Multicast);

		ByteBuffer& bb = *(*(_udpDiscoveryMessage[0])).getBuffer();

		bb.writeString(_config.getBusName());
		bb.writeNodeID(_myNodeId);
		bb.writeNodeVersion(_memMgr_SPtr->getMyNodeVersion());
		bb.writeBoolean(bootstrap);

		_udpDiscoveryMessage[0]->updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			_udpDiscoveryMessage[0]->writeCRCchecksum();
		}
		if (!_commAdapter_SPtr->sendToMCgroup(
				_udpDiscoveryMessage[0]))
		{
			Trace_Event(this, "discoveryMulticastTaskImpl()",
					"couldn't send a message to Multicast group");
		}
	}

	Trace_Exit(this, "discoveryTaskMulticastImpl()");
}

void TopologyManagerImpl::changeSuccessorTask()
{
	Trace_Entry(this, "changeSuccessorTask()");
	// Deal with successor now
	bool changedSuccessor = false;
	// or a bit later
	bool reschedule = false;
	bool changeSuccessor = true;
	NodeIDImpl_SPtr target;
	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		_changeSuccessorTaskScheduled = false;

		if (_currentSuccessor && _setSuccessor)
		{
			if (_setSuccessor->getNodeName().compare(
					_currentSuccessor->getNodeName()) != 0)
			{
				changedSuccessor = true;
			}
		}
		else if (_setSuccessor)
		{
			changedSuccessor = true;
		}

		if (_setSuccessor && changedSuccessor)
		{
			Trace_Event(this, "changeSuccessorTask()", "",
					"changing successor to", _setSuccessor->getNodeName(),
					"current successor", NodeIDImpl::stringValueOf(
							_currentSuccessor));

			if (changeSuccessor && _outgoingRandomConnectionRequests->contains(
					_setSuccessor))
			{
				Trace_Event(
						this,
						"changeSuccessorTask()",
						"Skipping target already in _outgoingRandomConnectionRequests",
						"node", _setSuccessor->getNodeName());

				changeSuccessor = false;
				reschedule = true;
			}

			if (changeSuccessor && _neighborTable->contains(_setSuccessor))
			{
				Trace_Event(this, "changeSuccessorTask()",
						"already a neighbor", "node",
						_setSuccessor->getNodeName());

				changeSuccessor = false;

				bool changedConnectivity = false;
				if (_setSuccessor != _currentSuccessor)
				{
					changedConnectivity = true;
				}
				_currentSuccessor = _setSuccessor;
				if (changedConnectivity)
				{
					submitConnectivityEvent();
				}
				_numFailedSetSuccessorAttempts = 0;

				// schedule a connect random task

				if (!_randomConnectTaskScheduled)
				{
					_taskSchedule_SPtr->scheduleDelay(
							_randomConnectTask_SPtr, TaskSchedule::ZERO_DELAY);
					_randomConnectTaskScheduled = true;
					Trace_Event(this, "changeSuccessorTask()",
							"scheduling a random connect task");
				}

			}
			if (changeSuccessor)
			{
				target = _setSuccessor;
			}

			if (!changeSuccessor && reschedule)
			{
				_taskSchedule_SPtr->scheduleDelay(
						_changeSuccessorTask_SPtr,
						boost::posix_time::milliseconds(
								_config.getTopologyPeriodicTaskIntervalMillis()));
				_changeSuccessorTaskScheduled = true;
				Trace_Event(this, "changeSuccessorTask()",
						"rescheduling a change successor task");
			}
		}
		else
		{
			changeSuccessor = false;
		}
	}

	if (changeSuccessor)
	{
		bool failed = !_commAdapter_SPtr->connect(target, RingCtx);
		if (failed)
		{
			Trace_Event(this, "changeSuccessorTask()",
					"Failed to connect to successor", "target",
					NodeIDImpl::stringValueOf(target), "_setSuccessor",
					NodeIDImpl::stringValueOf(_setSuccessor));
			//_outgoingRandomConnectionRequests->removeEntry(target);
			// schedule a change successor task
			{
				boost::recursive_mutex::scoped_lock lock(topo_mutex);
				if (!_changeSuccessorTaskScheduled)
				{
					_taskSchedule_SPtr->scheduleDelay(
							_changeSuccessorTask_SPtr,
							boost::posix_time::milliseconds(
									_config.getTopologyPeriodicTaskIntervalMillis()));
					_changeSuccessorTaskScheduled = true;
					Trace_Event(this, "changeSuccessorTask()",
							"rescheduling a change successor task");
				}
			}
		}
		else
		{
			_outgoingRandomConnectionRequests->addEntry(target,
					Neighbor_SPtr ());
			Trace_Event(this, "changeSuccessorTask()",
					"Succeeded to send async connect request to successor",
					"target", NodeIDImpl::stringValueOf(target),
					"_setSuccessor", NodeIDImpl::stringValueOf(_setSuccessor));
		}
	}

	Trace_Exit(this, "changeSuccessorTask()");
}

void TopologyManagerImpl::structuredTopologyConnectTask()
{

	Trace_Entry(this, "structuredTopologyConnectTask()");

	_structuredConnectTaskScheduled = false;
	NodeIDImpl_SPtr firstNodeId;

	int numCurrentConnections = _outgoingStructuredNeighborTable->size();

	for (int numStructuredConnections = numCurrentConnections; numStructuredConnections
			< _config.getStructDegreeTarget(); numStructuredConnections++)
	{
		bool connect = false;
		pair<NodeIDImpl_SPtr, int> nodeId;
		{
			boost::recursive_mutex::scoped_lock lock(topo_mutex);

			if (!_memMgr_SPtr)
			{

				Trace_Event(this, "structuredTopologyConnectTask()",
						"invalid _memMgr_SPtr. Leaving");
				continue;
			}
			nodeId = _memMgr_SPtr->getRandomizedStructuredNode();
			if (!firstNodeId)
			{
				firstNodeId = nodeId.first;
			}
			else if (nodeId.first)
			{
				if ((*firstNodeId) == (*(nodeId.first)))
				{
					Trace_Debug(this, "structuredTopologyConnectTask()",
							"Leaving. Received twice the same random node");
					break;
				}
			}

			if (!nodeId.first)
			{
				Trace_Debug(this, "structuredTopologyConnectTask()",
						"skipping a NULL random node");

				continue;
			}
			if (_outgoingStructuredNeighborTable->contains(nodeId.first))
			{
				Trace_Debug(
						this,
						"structuredTopologyConnectTask()",
						"Skipping an existing entry in _outgoingStructuredNeighborTable table as a structured node",
						"node", nodeId.first->getNodeName());
				numStructuredConnections--;
				continue;
			}
			else
			{
				Trace_Debug(this, "structuredTopologyConnectTask()",
						", adding a structured node", "node",
						nodeId.first->getNodeName());
				connect = true;
			}
		}
		if (connect)
		{
			if (!_commAdapter_SPtr->connect(nodeId.first, StructuredCtx))
			{
				Trace_Debug(this, "structuredTopologyConnectTask()",
						", Failed immediately to connect to", "node",
						nodeId.first->getNodeName());
			}
			_outgoingStructuredNeighborTable->addEntry(nodeId.first,
					Neighbor_SPtr (), nodeId.second);
		}
	} // end structured connect task

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		if (!_structuredConnectTaskScheduled
				&& (_outgoingStructuredNeighborTable->size()
						< _config.getStructDegreeTarget()) && _state
				!= TopologyManager::Closed)
		{
			_taskSchedule_SPtr->scheduleDelay(_structuredConnectTask_SPtr,
					boost::posix_time::milliseconds(
							_config.getTopologyPeriodicTaskIntervalMillis()));
			_structuredConnectTaskScheduled = true;
			Trace_Debug(this, "structuredTopologyConnectTask()",
					"scheduling a structured connect task");
		}
	}
	Trace_Exit(this, "structuredTopologyConnectTask()");

}

void TopologyManagerImpl::structuredTopologyRefreshTask()
{

	Trace_Entry(this, "structuredTopologyRefreshTask()");

	boost::recursive_mutex::scoped_lock lock(topo_mutex);

	_structuredRefreshTaskScheduled = false;

	vector<NodeIDImpl_SPtr> candidates =
			_outgoingStructuredNeighborTable->structuredLinksToRefresh(
					_memMgr_SPtr->getViewSize());

	vector<NodeIDImpl_SPtr>::iterator iter;

	for (iter = candidates.begin(); iter < candidates.end(); iter++)
	{
		NodeIDImpl_SPtr node = *iter;

		Neighbor_SPtr disconnectNeighbor =
				_outgoingStructuredNeighborTable->getNeighbor(node);
		if (disconnectNeighbor)
		{
			(*_topoMessage).writeH1Header(
					SCMessage::Type_Topo_Disconnect_Request);
			ByteBuffer& bb = *(*_topoMessage).getBuffer();
			bb.writeString(node->getNodeName());
			bb.writeInt((int) TopologyManagerImpl::STRUCTURED);
			(*_topoMessage).updateTotalLength();
			if (_config.isCRCMemTopoMsgEnabled())
			{
				(*_topoMessage).writeCRCchecksum();
			}

			if (!_outgoingStructuredNeighborTable->sendToNeighbor(node,
					_topoMessage))
			{
				Trace_Debug(this, "structuredTopologyRefreshTask()",
						"failed to send disconnect message to", "node",
						disconnectNeighbor->getName());
			}
			else
			{
				Trace_Debug(this, "structuredTopologyRefreshTask()",
						"sent a disconnect message to", "node",
						disconnectNeighbor->getName());
			}
		}
		else
		{
			Trace_Debug(this, "structuredTopologyRefreshTask()",
					"skipping an invalid neighbor", "node", node->getNodeName());

		}

	}

	Trace_Exit(this, "structuredTopologyRefreshTask()");

}

void TopologyManagerImpl::randomTopologyConnectTask()
{
	Trace_Entry(this, "randomTopologyConnectTask()");

	ostringstream oss;
	oss << _neighborTable->size();
	ostringstream oss3;
	oss3 << _outgoingRandomConnectionRequests->size();
	oss3 << "; " << _outgoingRandomConnectionRequests->toString();

	Trace_Debug(this, "randomTopologyConnectTask()", "Entry",
			"neighbor table size", oss.str(),
			"_outgoingRandomConnectionRequests->size()", oss3.str());

	_randomConnectTaskScheduled = false;
	NodeIDImpl_SPtr firstNodeId;

	int numCurrentConnections = _neighborTable->size()
			+ _outgoingRandomConnectionRequests->size();

	for (int numRandomConnections = numCurrentConnections; numRandomConnections
			< _config.getRandomDegreeTarget() + 2 /* currently 1 successor and 1 predecessor*/; numRandomConnections++)
	{
		ostringstream oss;
		oss << numCurrentConnections;
		ostringstream oss2;
		oss2 << numRandomConnections;
		ostringstream oss3;
		oss3 << _config.getRandomDegreeTarget();
		Trace_Debug(this, "randomTopologyConnectTask()", "In the loop",
				"numCurrentConnections", oss.str(), "numRandomConnections",
				oss2.str(), "numRandConnectionsAimed", oss3.str());
		bool connect = false;
		NodeIDImpl_SPtr nodeId;
		{
			boost::recursive_mutex::scoped_lock lock(topo_mutex);
			if (_candidatesForFutureRandomConnectionAttempts.empty())
			{
				if (!_memMgr_SPtr)
				{

					Trace_Event(this, "randomTopologyConnectTask()",
							"invalid _memMgr_SPtr. Leaving");
					continue;
				}
				nodeId = _memMgr_SPtr->getRandomNode();
				if (!firstNodeId)
				{
					firstNodeId = nodeId;
				}
				else if (nodeId)
				{
					if ((*firstNodeId) == (*nodeId))
					{
						Trace_Debug(this, "randomTopologyConnectTask()",
								"Leaving. Received twice the same random node");
						break;
					}
				}
			}
			else
			{
				nodeId = _candidatesForFutureRandomConnectionAttempts.back();
				_candidatesForFutureRandomConnectionAttempts.pop_back();
				Trace_Debug(this, "randomTopologyConnectTask",
						"adding node from candidates", "node",
						nodeId->toString());
			}

			if (!nodeId)
			{
				Trace_Debug(this, "randomTopologyConnectTask()",
						"skipping a NULL random node");

				continue;
			}
			if (_neighborTable->contains(nodeId))
			{
				Trace_Debug(
						this,
						"randomTopologyConnectTask()",
						"Skipping an existing entry in neighbor table as a random node",
						"node", nodeId->getNodeName());
				numRandomConnections--;
				continue;
			}

			if (_outgoingRandomConnectionRequests->contains(nodeId))
			{
				Trace_Debug(this, "randomTopologyConnectTask()",
						"Skipping a redundant random node", "node",
						nodeId->getNodeName());
				numRandomConnections--;
				continue;
			}
			else
			{
				Trace_Debug(this, "randomTopologyConnectTask()",
						", adding a random node", "node", nodeId->getNodeName());
				connect = true;
			}
		}
		if (connect)
		{
			if (!_commAdapter_SPtr->connect(nodeId, RandomCtx))
			{
				Trace_Debug(this, "randomTopologyConnectTask()",
						", Failed immediately to connect to", "node",
						nodeId->getNodeName());
				//_outgoingRandomConnectionRequests->removeEntry(nodeId);
			}
			else
			{
				_outgoingRandomConnectionRequests->addEntry(nodeId,
						Neighbor_SPtr ());
			}
		}
	} // end random connect task

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		_candidatesForFutureRandomConnectionAttempts.clear(); // TODO: figure out whether a good place for this
		if (!_randomConnectTaskScheduled && (_neighborTable->size()
				< _config.getRandomDegreeTarget() + 2) && _state
				!= TopologyManager::Closed)
		{
			_taskSchedule_SPtr->scheduleDelay(_randomConnectTask_SPtr,
					boost::posix_time::milliseconds(
							_config.getTopologyPeriodicTaskIntervalMillis()));
			_randomConnectTaskScheduled = true;
			Trace_Debug(this, "randomTopologyConnectTask()",
					"scheduling a random connect task");
		}
	}
	Trace_Exit(this, "randomTopologyConnectTask()");
}

void TopologyManagerImpl::randomTopologyDisconnectTask()
{
	Trace_Entry(this, "randomTopologyDisconnectTask()");

	boost::recursive_mutex::scoped_lock lock(topo_mutex);

	_randomDisConnectTaskScheduled = false;

	list<NodeIDImpl_SPtr> lowesttNodes;

	//FIXME it may happen that all other nodes have degree-vacancy 0, but this one has a degree too high. this creates a stalemate.

	std::ostringstream oss_deg;
	std::ostringstream oss_low;
	for (NeighborsDegreeMap::const_iterator iter = _neighborsDegreeMap.begin();
			iter != _neighborsDegreeMap.end(); iter++)
	{
		if (ScTraceBuffer::isDebugEnabled(_tc))
		{
			oss_deg << iter->first->getNodeName() << " d=" << iter->second << "; ";
		}

		if (iter->second < 0 && (*(iter->first) > *_myNodeId)
				&& !isInRecentlySentDisconnectRequestList(iter->first))
		{
			lowesttNodes.push_back(iter->first);
			if (ScTraceBuffer::isDebugEnabled(_tc))
			{
				oss_low << iter->first->getNodeName() << "; ";
			}
		}
	}

	Trace_Debug(this,"randomTopologyDisconnectTask()", "neighbor node degrees",
			"all", oss_deg.str(), "lowest", oss_low.str());

	for (int counter = _config.getRandomDegreeTarget() + 2; counter
			< _neighborTable->size(); counter++)
	{
		if (lowesttNodes.empty())
		{
			Trace_Debug(this, "randomTopologyDisconnectTask", "Empty disconnect candidate set");
			break;
		}
		else
		{
			NodeIDImpl_SPtr disconnectCandidate = lowesttNodes.back();
			lowesttNodes.pop_back();
			if (disconnectCandidate)
			{
				if (_currentSuccessor)
				{
					if ((*disconnectCandidate) == (*_currentSuccessor))
					{
						Trace_Debug(this, "randomTopologyDisconnectTask",
								"skipping disconnecting the successor:",
								"node", toString(disconnectCandidate));
						counter--;
						continue;
					}
				}
				Neighbor_SPtr disconnectNeighbor =
						_neighborTable->getNeighbor(disconnectCandidate);
				if (disconnectNeighbor)
				{
					_recentlySentDisconnectRequest.push_back(
							disconnectCandidate);
					(*_topoMessage).writeH1Header(
							SCMessage::Type_Topo_Disconnect_Request);
					ByteBuffer& bb = *(*_topoMessage).getBuffer();
					bb.writeString(disconnectCandidate->getNodeName());
					bb.writeInt((int) TopologyManagerImpl::RANDOM);
					bb.writeShort(_config.getRandomDegreeTarget() + 2
							- _neighborTable->size());
					(*_topoMessage).updateTotalLength();
					if (_config.isCRCMemTopoMsgEnabled())
					{
						(*_topoMessage).writeCRCchecksum();
					}

					if (!_neighborTable->sendToNeighbor(disconnectCandidate,
							_topoMessage))
					{
						Trace_Debug(this, "randomTopologyDisconnectTask()",
								"failed to send disconnect message to", "node",
								disconnectNeighbor->getName());
					}
					else
					{
						Trace_Debug(this, "randomTopologyDisconnectTask()",
								"sent a disconnect message to", "node",
								disconnectNeighbor->getName());
					}
				}
				else
				{
					removeEntryFromNeighborsDegreeMap(disconnectCandidate);
				}
			}
		}
	}

	if (!_randomDisConnectTaskScheduled && (_neighborTable->size()
			> _config.getRandomDegreeTarget() + 2) && _state
			!= TopologyManager::Closed)
	{
		_taskSchedule_SPtr->scheduleDelay(_randomDisConnectTask_SPtr,
				boost::posix_time::milliseconds(
						_config.getTopologyPeriodicTaskIntervalMillis()));
		_randomDisConnectTaskScheduled = true;
		Trace_Debug(this, "randomTopologyDisconnectTask()",
				"scheduling a random disconnect task");
	}

	Trace_Exit(this, "randomTopologyDisconnectTask()");
}

void TopologyManagerImpl::changedDegreeTask()
{
	Trace_Entry(this, "changedDegreeTask()");

	boost::recursive_mutex::scoped_lock lock(topo_mutex);

	_updateDegreeTaskScheduled = false;
	// Send degree updates to all current neighbors
	if (_myDegreeChanged)
	{
		if (_neighborTable)
		{
			(*_topoMessage).writeH1Header(SCMessage::Type_Topo_Degree_Changed);
			ByteBuffer& bb = *(*_topoMessage).getBuffer();
			bb.writeShort(_config.getRandomDegreeTarget() + 2
					- _neighborTable->size());
			(*_topoMessage).updateTotalLength();
			if (_config.isCRCMemTopoMsgEnabled())
			{
				(*_topoMessage).writeCRCchecksum();
			}
			_neighborTable->sendToAllNeighbors(_topoMessage);

		}
		_myDegreeChanged = false;
	}
	Trace_Exit(this, "changedDegreeTask()");
}

void TopologyManagerImpl::discoveryReplySendTask()
{
	Trace_Entry(this, "discoveryReplySendTask()");
	boost::recursive_mutex::scoped_lock lock(topo_mutex);
	_discoveryReplySendTaskScheduled = false;

	if (_isTCPDiscovery && !_discoveryReplySendListTCP.empty())
	{
		list<Neighbor_SPtr >::iterator iter;

		(*_topoMessage).writeH1Header(SCMessage::Type_Topo_Discovery_Reply_TCP);

		_memMgr_SPtr->getDiscoveryView(_topoMessage);

		(*_topoMessage).updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			(*_topoMessage).writeCRCchecksum();
		}

		for (iter = _discoveryReplySendListTCP.begin(); iter
				!= _discoveryReplySendListTCP.end(); iter++)
		{
			Neighbor_SPtr myNeighbor = (*iter);

			if (myNeighbor)
			{
				if (myNeighbor->sendMessage(_topoMessage))
				{
					Trace_Debug(this, "discoveryReplySendTask()",
							"couldn't send a message to", "node",
							myNeighbor->getName());
				}
				else
				{
					Trace_Debug(this, "discoveryReplySendTask()",
							"sent message to", "node", myNeighbor->getName());
				}

				_waitingConnectionBreakList.push_back(myNeighbor);
				Trace_Debug(this, "discoveryReplySendTask()",
						"Added node to _waitingConnectionBreakSet", "node",
						myNeighbor->getName());
			}
			else
			{
				Trace_Debug(this, "discoveryReplySendTask()",
						"Warning: couldn't find entry in incoming map", "node",
						myNeighbor->getName());

			}
		}
		_discoveryReplySendListTCP.clear();
	}

	if (_isUDPDiscovery && !_discoveryReplySendListUDP.empty())// UDP discovery
	{

		_udpDiscoveryMessage[0]->writeH1Header(
				SCMessage::Type_Topo_Discovery_Reply_UDP);

		ByteBuffer& bb = *(*(_udpDiscoveryMessage[0])).getBuffer();
		bb.writeString(_config.getBusName());
		bb.writeString(_myNodeId->getNodeName());
		bb.writeLong(_coreInterface.getIncarnationNumber());
		//TODO SplitBrain
		int numMsgs = _memMgr_SPtr->getDiscoveryViewMultipart(
				_udpDiscoveryMessage);

		Trace_Debug(this, "discoveryReplySendTask()",
				"Going to send message list",
				"size", ScTraceBuffer::stringValueOf(_discoveryReplySendListUDP.size()));

		for (list<NodeIDImpl_SPtr>::iterator iter =
				_discoveryReplySendListUDP.begin(); iter
				!= _discoveryReplySendListUDP.end(); iter++)
		{
			NodeIDImpl_SPtr fullNodeToSend = _nodeIdCache.getOrCreate(
					(*iter)->getNodeName());

			Trace_Debug(this, "discoveryReplySendTask()",
					"Going to send message to", "node", toString<NodeIDImpl> (
							fullNodeToSend));

			bool ret = _commAdapter_SPtr->sendTo(fullNodeToSend,
					_udpDiscoveryMessage, numMsgs);
			if (!ret)
			{
				Trace_Debug(this, "discoveryReplySendTask()",
						"Failed to send message to", "node",
						fullNodeToSend->getNodeName());
			}
		}
		_discoveryReplySendListUDP.clear();

		Trace_Debug(this, "discoveryReplySendTask()", "Sent to UDP list");
	}

	//TODO move this to a task dedicated to Multicast replies
	multicastDiscoveryReplySendTask();

	Trace_Exit(this, "discoveryReplySendTask()");
}

void TopologyManagerImpl::multicastDiscoveryReplySendTask()
{
	Trace_Entry(this, "multicastDiscoveryReplySendTask()");

	if (_isMulticastDiscovery && _discoveryReplySendCountMulticast > 0)
	{
		_udpDiscoveryMessage[0]->writeH1Header(
				SCMessage::Type_Topo_Discovery_Reply_Multicast);

		ByteBuffer& bb = *(*(_udpDiscoveryMessage[0])).getBuffer();
		bb.writeString(_config.getBusName());
		bb.writeNodeID(_myNodeId);
		bb.writeNodeVersion(_memMgr_SPtr->getMyNodeVersion());

		_udpDiscoveryMessage[0]->updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			_udpDiscoveryMessage[0]->writeCRCchecksum();
		}

		Trace_Debug(this, "multicastDiscoveryReplySendTask()","Going to send reply Multicast group",
				"#requests",ScTraceBuffer::stringValueOf(_discoveryReplySendCountMulticast));

		bool ret = _commAdapter_SPtr->sendToMCgroup(_udpDiscoveryMessage[0]);
		if (!ret)
		{
			Trace_Debug(this, "discoveryReplySendTask()", "Failed to send message to Multicast group");
		}
		_discoveryReplySendCountMulticast = 0;
	}

	Trace_Exit(this, "multicastDiscoveryReplySendTask()");
}

void TopologyManagerImpl::stopFrequentDiscoveryTask()
{
	Trace_Entry(this, "stopFrequentDiscoveryTask()");
	boost::recursive_mutex::scoped_lock lock(topo_mutex);
	_frequentDiscovery = false;
	Trace_Exit(this, "stopFrequentDiscoveryTask()");
}

void TopologyManagerImpl::terminationTask()
{
	Trace_Event(this, "terminationTask(), Entry");

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		Trace_Event(this, "terminationTask(), with the lock");

		//cancel all tasks
		_stopFrequentDiscoveryTask_SPtr->cancel();
		_discoveryTask_SPtr->cancel();
		_changeSuccessorTask_SPtr->cancel();
		_randomConnectTask_SPtr->cancel();
		_randomDisConnectTask_SPtr->cancel();
		_updateDegreeTask_SPtr->cancel();
		_structuredConnectTask_SPtr->cancel();

		if (_state != TopologyManagerImpl::Closed)
		{
			String errMsg("Unexpected state in terminate(): ");
			errMsg.append(TopologyManager::nodeStateName[_state]);
			Trace_Event(this, "terminationTask()", errMsg);
			throw SpiderCastRuntimeError(errMsg);
		}
	}

	if (_softClosed)
	{
		sendLeaveMsg();
	}

	Trace_Event(this, "terminationTask(), Exit");
}

void TopologyManagerImpl::sendLeaveMsg()
{
	Trace_Entry(this, "sendLeaveMsg()");
	boost::recursive_mutex::scoped_lock lock(topo_mutex);
	if (_neighborTable)
	{
		(*_topoMessage).writeH1Header(SCMessage::Type_Topo_Node_Leave);
		(*_topoMessage).updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			(*_topoMessage).writeCRCchecksum();
		}
		_neighborTable->sendToAllNeighbors(_topoMessage);

	}
	else
	{
		Trace_Event(this, "sendLeaveMsg", "neighborTable is null, ignoring");
	}
	Trace_Exit(this, "sendLeaveMsg()");
}

void TopologyManagerImpl::processIncomingTopologyMessage(
		SCMessage_SPtr incomingTopologyMsg)
{
	Trace_Entry(this, "processIncomingTopologyMessage");

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		if (_state == TopologyManagerImpl::Closed)
		{
			Trace_Event(this, "processIncomingTopologyMessage()",
					"returning immediately because closed.");
			return;
		}
	}

	Trace_Debug(this, "processIncomingTopologyMessage()", "", "source",
			NodeIDImpl::stringValueOf(incomingTopologyMsg->getSender()),
			"source-bus", spdr::toString(
					incomingTopologyMsg->getBusName()), "message",
			incomingTopologyMsg->toString());

	SCMessage::H1Header h1 = (*incomingTopologyMsg).readH1Header();

	NodeIDImpl_SPtr peerName = (*incomingTopologyMsg).getSender();

	if (h1.get<0> () == SCMessage::Group_Topology) //message group
	{
		//Sanity check Fail-Fast
		if (h1.get<1> () != SCMessage::Type_Topo_Comm_Event
				&& h1.get<1> () != SCMessage::Type_Topo_Discovery_Reply_TCP
				&& h1.get<1> () != SCMessage::Type_Topo_Discovery_Request_TCP
				&& h1.get<1> () != SCMessage::Type_Topo_Degree_Changed
				&& h1.get<1> () != SCMessage::Type_Topo_Node_Leave
				&& h1.get<1> () != SCMessage::Type_Topo_Discovery_Reply_UDP
				&& h1.get<1> () != SCMessage::Type_Topo_Discovery_Request_UDP
				&& h1.get<1> () != SCMessage::Type_Topo_Discovery_Reply_Multicast
				&& h1.get<1> () != SCMessage::Type_Topo_Discovery_Request_Multicast)
		{
			boost::shared_ptr<ByteBuffer> bb =
					(*incomingTopologyMsg).getBuffer();
			String receiver;
			try
			{
				receiver = (*bb).readString();
			}
			catch (IndexOutOfBoundsException& e)
			{
				String what("Error: ");
				what.append(e.what());
				what.append(", Exception while unmarshaling topology message from ");
				what.append(peerName->getNodeName());
				Trace_Error(this, "processIncomingTopologyMessage", what);
				throw MessageUnmarshlingException(what);
			}

			Trace_Event(this, "processIncomingTopologyMessage()", "details",
					"target", receiver);

			if (receiver != _myNodeId->getNodeName())
			{
				String errMsg("Error: Wrong recipient: intended: ");
				errMsg.append(receiver);
				errMsg.append("; My Name: ");
				errMsg.append(_myNodeId->getNodeName());
				Trace_Error(this, "processIncomingTopologyMessage()", errMsg);
				throw SpiderCastRuntimeError(errMsg);
			}
		}

		switch (h1.get<1> ())
		//message type

		{
		case SCMessage::Type_Topo_Discovery_Request_TCP:
		{
			processIncomingDiscoveryRequestMsg(incomingTopologyMsg, peerName);
		}
			break;

		case SCMessage::Type_Topo_Discovery_Reply_TCP:
		{
			processIncomingDiscoveryReplyMsg(incomingTopologyMsg, peerName);
		}
			break;

		case SCMessage::Type_Topo_Discovery_Request_UDP:
		{
			processIncomingDiscoveryRequestUDPMsg(incomingTopologyMsg, peerName);
		}
			break;

		case SCMessage::Type_Topo_Discovery_Reply_UDP:
		{
			processIncomingDiscoveryReplyUDPMsg(incomingTopologyMsg, peerName);
		}
			break;

		case SCMessage::Type_Topo_Discovery_Request_Multicast:
		{
			//In case of Multicast, the peerName is full, unlike UDP
			processIncomingDiscoveryRequestMulticastMsg(incomingTopologyMsg, peerName);
		}
			break;

		case SCMessage::Type_Topo_Discovery_Reply_Multicast:
		{
			//In case of Multicast, the perName is full, unlike UDP
			processIncomingDiscoveryReplyMulticastMsg(incomingTopologyMsg, peerName);
		}
			break;

		case SCMessage::Type_Topo_Connect_Successor:
		{
			processIncomingConnectSuccessorMsg(incomingTopologyMsg, peerName);
		}
			break;
		case SCMessage::Type_Topo_Connect_Successor_OK:
		{
			processIncomingConnectSuccessorOkMsg(incomingTopologyMsg, peerName);
		}
			break;
		case SCMessage::Type_Topo_Disconnect_Request:
		{
			processIncomingDisconnectRequestMsg(incomingTopologyMsg, peerName);
		}
			break;
		case SCMessage::Type_Topo_Disconnect_Reply:
		{
			processIncomingDisconnectReplyMsg(incomingTopologyMsg, peerName);
		}
			break;
		case SCMessage::Type_Topo_Node_Leave:
		{
			processIncomingNodeLeaveMsg(incomingTopologyMsg, peerName);
		}
			break;
		case SCMessage::Type_Topo_Connect_Random_Request:
		{
			processIncomingConnectRequestMsg(incomingTopologyMsg, peerName);
		}
			break;
		case SCMessage::Type_Topo_Connect_Random_Reply:
		{
			processIncomingConnectReplyMsg(incomingTopologyMsg, peerName);
		}
			break;
		case SCMessage::Type_Topo_Connect_Structured_Request:
		{
			processIncomingConnectStructuredRequestMsg(incomingTopologyMsg,
					peerName);
		}
			break;
		case SCMessage::Type_Topo_Connect_Structured_Reply:
		{
			processIncomingConnectStructuredReplyMsg(incomingTopologyMsg,
					peerName);
		}
			break;

		case SCMessage::Type_Topo_Degree_Changed:
		{
			processIncomingDegreeChangedMsg(incomingTopologyMsg, peerName);
		}
			break;
		case SCMessage::Type_Topo_Comm_Event:
		{
			processIncomingCommEventMsg(incomingTopologyMsg, peerName);
		}
			break;
		default:
		{
			String errMsg("Error: Unexpected message type ");
			errMsg.append(SCMessage::getMessageTypeName(h1.get<1> ()));
			Trace_Error(this, "processIncomingTopologyMessage()", errMsg);
			throw SpiderCastRuntimeError(errMsg);
		}
		}
	}
	else if (h1.get<0> () == SCMessage::Group_GeneralCommEvents)
	{
		switch (h1.get<1> ())
		//message type

		{
		case SCMessage::Type_General_Comm_Event:
		{
			processIncomingCommEventMsg(incomingTopologyMsg, peerName);
		}
			break;

		default:
		{
			String errMsg("Error: Unexpected message type ");
			errMsg.append(SCMessage::getMessageTypeName(h1.get<1> ()));
			Trace_Error(this, "processIncomingTopologyMessage()", errMsg);
			throw SpiderCastRuntimeError(errMsg);
		}
		}
	}
	else
	{
		//send to hierarchy manager
		_hierarchyMngr_SPtr->processIncomingHierarchyMessage(
				incomingTopologyMsg);
	}

	Trace_Exit(this, "processIncomingTopologyMessage");
}

void TopologyManagerImpl::processIncomingDiscoveryRequestMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingDiscoveryRequestMsg()");

	if (incomingTopologyMsg->getBusName()->toString().compare(
			_config.getBusName()) != 0)
	{
		String errMsg("Error: Wrong bus name: intended: ");
		errMsg.append(incomingTopologyMsg->getBusName()->toString());
		errMsg.append("; My Bus Name: ");
		errMsg.append(_config.getBusName());
		Trace_Error(this, "processIncomingDiscoveryRequestMsg()", errMsg);
		throw SpiderCastRuntimeError(errMsg);
	}

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		if (!_memMgr_SPtr)
		{
			Trace_Event(this, "processIncomingDiscoveryRequestMsg()",
					"invalid _memMgr_SPtr. returning");

			return;
		}

		bool isBootstrap = incomingTopologyMsg->getBuffer()->readBoolean();

		_memMgr_SPtr->processIncomingDiscoveryView(incomingTopologyMsg, true,
				isBootstrap);

		if (!_discoveryReplySendTaskScheduled)
		{
			_discoveryReplySendTaskScheduled = true;
			_taskSchedule_SPtr->scheduleDelay(_discoveryReplySendTask_SPtr,
					TaskSchedule::ZERO_DELAY);
			Trace_Debug(this, "processIncomingDiscoveryRequestMsg()",
					"Scheduled a discovery reply send task");
		}
	}

	Neighbor_SPtr myNeighbor;
	StringSPtr senderLocalName = incomingTopologyMsg->getSenderLocalName();
	if (senderLocalName && boost::starts_with(*senderLocalName, NodeID::RSRV_NAME_DISCOVERY))
	{
		Trace_Debug(this, "processIncomingDiscoveryRequestMsg()", "nameless discovery request",
				"sender", spdr::toString(peerName), "senderLocalName", *senderLocalName );
		myNeighbor = _commAdapter_SPtr->connectOnExisting(_nodeIdCache.getOrCreate(*senderLocalName));
	}
	else
	{
		myNeighbor = _commAdapter_SPtr->connectOnExisting(peerName);
	}

	if (myNeighbor)
	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		_discoveryReplySendListTCP.push_back(myNeighbor);
		if (myNeighbor->getReceiverId() != 0 && myNeighbor->getReceiverId()
				!= incomingTopologyMsg->getStreamId())
		{
			Trace_Debug(this,"processIncomingDiscoveryRequestMsg()",
					"Warning: assert(myNeighbor->getRecieverId() == 0 || myNeighbor->getReceiverId() == incomingTopologyMsg->getStreamId()); failed");
		}
		myNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());
		Trace_Debug(this, "processIncomingDiscoveryRequestMsg()",
				"Set receiver stream id inside neighbor", "neighbor", spdr::toString(myNeighbor));
	}
	else
	{
		Trace_Debug(this, "processIncomingDiscoveryRequestMsg()",
				"Warning: connectOnExisting() failed");

	}

	Trace_Exit(this, "processIncomingDiscoveryRequestMsg()");
}

void TopologyManagerImpl::processIncomingDiscoveryRequestUDPMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingDiscoveryRequestUDPMsg()");

	boost::shared_ptr<ByteBuffer> bb = (*incomingTopologyMsg).getBuffer();
	String sourceBus = bb->readString();
	String sourceName = bb->readString();
	int64_t inc_num = bb->readLong();

	Trace_Debug(this, "processIncomingDiscoveryRequestUDPMsg()", "details",
			"bus", sourceBus, "source", sourceName, "inc", boost::lexical_cast<String>(inc_num));

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		if (!_memMgr_SPtr)
		{
			Trace_Event(this, "processIncomingDiscoveryRequestUDPMsg()",
					"invalid _memMgr_SPtr. returning");

			return;
		}

		bool isBootstrap = incomingTopologyMsg->getBuffer()->readBoolean();

		_memMgr_SPtr->processIncomingDiscoveryView(incomingTopologyMsg, true,
				isBootstrap);

		if (!_discoveryReplySendTaskScheduled)
		{
			_discoveryReplySendTaskScheduled = true;
			_taskSchedule_SPtr->scheduleDelay(_discoveryReplySendTask_SPtr,
					TaskSchedule::ZERO_DELAY);
			Trace_Debug(this, "processIncomingDiscoveryRequestUDPMsg()",
					"Scheduled a discovery reply send task");
		}

		_discoveryReplySendListUDP.push_back(peerName);
	}

	Trace_Exit(this, "processIncomingDiscoveryRequestUDPMsg()");
}

void TopologyManagerImpl::processIncomingDiscoveryReplyMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingDiscoveryReplyMsg()");

	if (!_memMgr_SPtr)
	{
		Trace_Event(this, "processIncomingDiscoveryReplyMsg()",
				"invalid _memMgr_SPtr. returning");
		return;
	}

	ByteBuffer_SPtr buffer = incomingTopologyMsg->getBuffer();

	_memMgr_SPtr->processIncomingDiscoveryView(incomingTopologyMsg, false,
			true);

	StringSPtr senderLocalName = incomingTopologyMsg->getSenderLocalName();
	Neighbor_SPtr myNeighbor;

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		//TODO Nameless_TCP_Discovery
		if (senderLocalName && (senderLocalName->substr(0,NodeID::NAME_ANY.size()) == NodeID::NAME_ANY))
		{
			NodeIDImpl_SPtr senderLocalID = _nodeIdCache.getOrCreate(*senderLocalName);
			myNeighbor = _discoveryMap->getNeighbor(senderLocalID);
			_discoveryMap->removeEntry(senderLocalID);
		}
		else
		{
			myNeighbor = _discoveryMap->getNeighbor(peerName);
			_discoveryMap->removeEntry(peerName);
		}
	}

	if (myNeighbor)
	{
		if (myNeighbor->getReceiverId() != 0 && myNeighbor->getReceiverId()
				!= incomingTopologyMsg->getStreamId())
		{
			Trace_Debug(this,"processIncomingDiscoveryReplyMsg()",
					"Warning: assert(myNeighbor->getRecieverId() == 0 || myNeighbor->getReceiverId() != incomingTopologyMsg->getStreamId()); failed");
		}
		myNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());
		Trace_Debug(this, "processIncomingDiscoveryReplyMsg()",
				"Set receiver stream id inside neighbor");

		_commAdapter_SPtr->disconnect(myNeighbor);
	}
	else
	{
		Trace_Event(this, "processIncomingDiscoveryReplyMsg()",	"Warning: could not find discovery node",
				"sender", peerName->getNodeName(),
				"senderLocalName", (senderLocalName ? *senderLocalName : "null"));
	}

	Trace_Exit(this, "processIncomingDiscoveryReplyMsg()");
}

void TopologyManagerImpl::processIncomingDiscoveryReplyUDPMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingDiscoveryReplyUDPMsg()");

	if (!_memMgr_SPtr)
	{
		Trace_Event(this, "processIncomingDiscoveryReplyUDPMsg()",
				"invalid _memMgr_SPtr. returning");
		return;
	}

	ByteBuffer_SPtr buffer = incomingTopologyMsg->getBuffer();
	buffer->skipString(); //bus
	buffer->skipString(); //sender
	buffer->readLong(); //Incarnation SplitBrain

	_memMgr_SPtr->processIncomingDiscoveryView(incomingTopologyMsg, false,
			true);

	Trace_Exit(this, "processIncomingDiscoveryReplyUDPMsg()");
}

void TopologyManagerImpl::processIncomingDiscoveryRequestMulticastMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingDiscoveryRequestMulticastMsg()");

	boost::shared_ptr<ByteBuffer> bb = (*incomingTopologyMsg).getBuffer();
	bb->skipString(); //bus name
	NodeIDImpl_SPtr nodeID = bb->readNodeID();
	NodeVersion ver = bb->readNodeVersion();
	bool isBootstrap = bb->readBoolean();

	Trace_Debug(this, "processIncomingDiscoveryRequestMulticastMsg()", "details",
			"source", spdr::toString(peerName), "ver", ver.toString());

	if (!_memMgr_SPtr)
	{
		Trace_Event(this, "processIncomingDiscoveryRequestUDPMsg()",
				"invalid _memMgr_SPtr. throwing");
		throw NullPointerException("Null pointer to MembershipManager");
	}

	//Unlike UDP, the peerName is full (including endpoint addresses)
	//call membership with NodeID + NodeVersion, to process "alive" message,
	//reply if the view changed, or if the view did not change but it is not in view (filtered by history)
	bool reply = _memMgr_SPtr->processIncomingMulticastDiscoveryNodeView(nodeID,ver,true,isBootstrap);

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		if (reply)
		{
			//TODO schedule a dedicated multicast discovery reply task, with batching (not zero delay)
			if (!_discoveryReplySendTaskScheduled)
			{
				_discoveryReplySendTaskScheduled = true;
				_taskSchedule_SPtr->scheduleDelay(_discoveryReplySendTask_SPtr,
						TaskSchedule::ZERO_DELAY);
				Trace_Debug(this, "processIncomingDiscoveryRequestMulticastMsg()",
						"Scheduled a discovery reply send task");
			}
			_discoveryReplySendCountMulticast++;
		}
	}

	Trace_Exit(this, "processIncomingDiscoveryRequestMulticastMsg()");
}

void TopologyManagerImpl::processIncomingDiscoveryReplyMulticastMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingDiscoveryReplyMulticastMsg()");

	ByteBuffer_SPtr buffer = incomingTopologyMsg->getBuffer();
	buffer->skipString(); //bus name
	NodeIDImpl_SPtr nodeID = buffer->readNodeID();
	NodeVersion ver = buffer->readNodeVersion();

	if (!_memMgr_SPtr)
	{
		Trace_Event(this, "processIncomingDiscoveryRequestUDPMsg()",
				"invalid _memMgr_SPtr. throwing");
		throw NullPointerException("Null pointer to MembershipManager");
	}

	//Unlike UDP, the peerName is full (including endpoint addresses)
	//call membership with NodeID + NodeVersion, to process "alive" message
	_memMgr_SPtr->processIncomingMulticastDiscoveryNodeView(nodeID,ver,false,false);

	Trace_Exit(this, "processIncomingDiscoveryReplyMulticastMsg()");
}

void TopologyManagerImpl::processIncomingConnectSuccessorMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingConnectSuccessorMsg()");
	Neighbor_SPtr myNeighbor;

	if (incomingTopologyMsg->getBusName()->toString().compare(
			_config.getBusName()) != 0)
	{
		String errMsg("Error: Wrong bus name: intended: ");
		errMsg.append(incomingTopologyMsg->getBusName()->toString());
		errMsg.append("; My Bus Name: ");
		errMsg.append(_config.getBusName());
		Trace_Error(this, "processIncomingConnectSuccessorMsg()", errMsg);
		throw SpiderCastRuntimeError(errMsg);
	}

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		if (_neighborTable->contains(peerName))
		{
			if (_memberName.compare(peerName->getNodeName()) > 0)
			{
				Trace_Debug(this, "processIncomingConnectSuccessorMsg()",
						"Ignoring request, waiting for response from",
						"Target", peerName->getNodeName());
				return;
			}
			else
			{
				myNeighbor = _neighborTable->getNeighbor(peerName);
				if (!myNeighbor)
				{
					String errMsg("Error: Invalid entry in the neighbor table: ");
					errMsg.append(peerName->getNodeName());
					Trace_Error(this, "processIncomingConnectSuccessorMsg()",
							errMsg);
					throw SpiderCastRuntimeError(errMsg);
				}
				if (myNeighbor->getReceiverId() != 0
						&& myNeighbor->getReceiverId()
								!= incomingTopologyMsg->getStreamId())
				{
					ostringstream oss;
					oss
							<< "Warning: assert(myNeighbor->getRecieverId() == 0 || myNeighbor->getReceiverId() == incomingTopologyMsg->getStreamId()); failed; myNeighbor id: "
							<< myNeighbor->getReceiverId() << "; message id: "
							<< incomingTopologyMsg->getStreamId();
					Trace_Debug(this, "processIncomingConnectSuccessorMsg()",
							oss.str());
				}

				myNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());
				Trace_Debug(this, "processIncomingConnectSuccessorMsg()",
						"Responding with existing neighbor", "Target",
						peerName->getNodeName());
			}
		}
		if (_outgoingRandomConnectionRequests->contains(peerName))
		{
			if (_memberName.compare(peerName->getNodeName()) > 0)
			{
				Trace_Debug(
						this,
						"processIncomingConnectSuccessorMsg()",
						"Ignoring request, waiting for outgoing connection attempt",
						"Target", peerName->getNodeName());
				return;
			}
		}
	}

	if (!myNeighbor)
	{
		myNeighbor = _commAdapter_SPtr->connectOnExisting(peerName);
	}

	if (!myNeighbor)
	{
		Trace_Debug(this, "processIncomingConnectSuccessorMsg()",
				"Warning: connectOnExisting() failed", "target",
				peerName->getNodeName());
	}
	else
	{

		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		(*_topoMessage).writeH1Header(SCMessage::Type_Topo_Connect_Successor_OK);
		ByteBuffer& bb = *(*_topoMessage).getBuffer();
		bb.writeString(peerName->getNodeName());
		bb.writeShort(_config.getRandomDegreeTarget() + 2
				- _neighborTable->size());
		(*_topoMessage).updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			(*_topoMessage).writeCRCchecksum();
		}

		if (myNeighbor->getReceiverId() != 0 && myNeighbor->getReceiverId()
				!= incomingTopologyMsg->getStreamId())
		{
			Trace_Debug(
					this,
					"processIncomingConnectSuccessorMsg()",
					"Warning: assert(myNeighbor->getRecieverId() == 0) || myNeighbor->getRecieverId() != incomingTopologyMsg->getStreamId failed");
		}
		myNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());
		Trace_Debug(this, "processIncomingConnectSuccessorMsg()",
				"Set receiver stream id inside neighbor");

		if (!_neighborTable->contains(peerName))
		{
			_neighborTable->addEntry(peerName, myNeighbor);

			ostringstream oss;
			oss << _neighborTable->size();
			Trace_Event(this, "processIncomingConnectSuccessorMsg()",
					"Connectivity event; added pre-decessor", "table",
					_neighborTable->toString(), "size", oss.str());
		}

		if (!_neighborTable->sendToNeighbor(peerName, _topoMessage))
		{
			Trace_Debug(this, "processIncomingConnectSuccessorMsg()",
					"couldn't send a connect_OK message to", "node",
					myNeighbor->getName());
			// TODO: decide what to do
		}

		if (_memMgr_SPtr)
		{
			_memMgr_SPtr->newNeighbor(peerName);
			_routingMgr_SPtr->addRoutingNeighbor(peerName, myNeighbor);
			_neighborTable->setRoutable(peerName);
		}
		else
		{
			Trace_Event(this, "processIncomingConnectSuccessorMsg()",
					"invalid _memMgr_SPtr");
		}

		if (_setSuccessor)
		{
			if ((*peerName) == (*_setSuccessor))
			{
				_currentSuccessor = _setSuccessor;
			}
		}

		boost::shared_ptr<ByteBuffer> inbb = (*incomingTopologyMsg).getBuffer();
		short degree;
		try
		{
			degree = (*inbb).readShort();
		} catch (IndexOutOfBoundsException& e)
		{
			String what("Error: ");
			what.append(e.what());
			what.append(
					", Exception while unmarshaling Type_Topo_Connect_Successor message from ");
			what.append(peerName->getNodeName());
			Trace_Error(this, "processIncomingConnectSuccessorMsg", what);
			throw MessageUnmarshlingException(what);
		}
		addEntryToNeighborsDegreeMap(peerName, degree);
		submitConnectivityEvent();
		if (!_randomDisConnectTaskScheduled && (_neighborTable->size()
				> _config.getRandomDegreeTarget() + 2) && _state
				!= TopologyManager::Closed)
		{
			_taskSchedule_SPtr->scheduleDelay(_randomDisConnectTask_SPtr,
					boost::posix_time::milliseconds(
							_config.getTopologyPeriodicTaskIntervalMillis()));
			_randomDisConnectTaskScheduled = true;
			Trace_Debug(this, "processIncomingConnectSuccessorMsg()",
					"scheduling a random disconnect task");
		}
	}

	Trace_Exit(this, "processIncomingConnectSuccessorMsg()");
}

void TopologyManagerImpl::processIncomingConnectSuccessorOkMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingConnectSuccessorOkMsg()");

	boost::recursive_mutex::scoped_lock lock(topo_mutex);

	if (_memMgr_SPtr)
	{
		_memMgr_SPtr->newNeighbor(peerName);
	}
	else
	{
		Trace_Debug(this, "processIncomingConnectSuccessorOkMsg()",
				"invalid _memMgr_SPtr");
	}

	boost::shared_ptr<ByteBuffer> bb = (*incomingTopologyMsg).getBuffer();
	short degree;
	try
	{
		degree = (*bb).readShort();
	} catch (IndexOutOfBoundsException& e)
	{
		String what("Error: ");
		what.append(e.what());
		what.append(
				", Exception while unmarshaling Type_Topo_Connect_Successor_OK message from ");
		what.append(peerName->getNodeName());
		Trace_Error(this, "processIncomingConnectSuccessorOkMsg()", what);
		throw MessageUnmarshlingException(what);
	}
	addEntryToNeighborsDegreeMap(peerName, degree);
	_outgoingRandomConnectionRequests->removeEntry(peerName);

	Neighbor_SPtr myNeighbor = _neighborTable->getNeighbor(
			peerName);
	if (myNeighbor)
	{
		if (myNeighbor->getReceiverId() != 0 && myNeighbor->getReceiverId()
				!= incomingTopologyMsg->getStreamId())
		{
			Trace_Debug(
					this,
					"processIncomingConnectSuccessorOkMsg()",
					"Warning: assert(myNeighbor->getRecieverId() == 0 || myNeighbor->getReceiverId() != incomingTopologyMsg->getStreamId()); failed");
		}
		//		assert(myNeighbor->getRecieverId() == 0);
		myNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());
		Trace_Debug(this, "processIncomingConnectSuccessorOkMsg()",
				"Set receiver stream id inside neighbor");
		_routingMgr_SPtr->addRoutingNeighbor(peerName, myNeighbor);
		_neighborTable->setRoutable(peerName);
	}
	else
	{
		Trace_Debug(this, "processIncomingConnectSuccessorOkMsg()",
				"Warning: couldn't find entry in neighbor table");
	}

	Trace_Exit(this, "processIncomingConnectSuccessorOkMsg()");
}

void TopologyManagerImpl::processIncomingDisconnectRequestMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingDisconnectRequestMsg()");

	boost::recursive_mutex::scoped_lock lock(topo_mutex);

	boost::shared_ptr<ByteBuffer> bb = (*incomingTopologyMsg).getBuffer();
	int type;
	short degree = 0;
	try
	{
		type = (*bb).readInt();
		if (type != (int) TopologyManagerImpl::STRUCTURED)
		{
			degree = (*bb).readShort();
		}
	} catch (IndexOutOfBoundsException& e)
	{
		String what("Error: ");
		what.append(e.what());
		what.append(
				", Exception while unmarshaling Type_Topo_Disconnect_Request message from ");
		what.append(peerName->getNodeName());
		Trace_Error(this, "processIncomingDisconnectRequestMsg()", what);
		throw MessageUnmarshlingException(what);
	}

	ostringstream oss;
	oss << "link type: " << type;
	Trace_Debug(this, "processIncomingDisconnectRequestMsg()", oss.str());

	//FIXME first remove from table/s, then reply, to make sure the reply is the last message

	bool reply = true;

	if (type == (int) TopologyManagerImpl::STRUCTURED)
	{
		Neighbor_SPtr myNeighbor =
				_incomingStructuredNeighborTable->getNeighbor(peerName);
		if (myNeighbor)
		{

			(*_topoMessage).writeH1Header(SCMessage::Type_Topo_Disconnect_Reply);
			ByteBuffer& bb = *(*_topoMessage).getBuffer();
			bb.writeString(peerName->getNodeName());
			bb.writeInt(type);
			bb.writeBoolean(reply);
			(*_topoMessage).updateTotalLength();
			if (_config.isCRCMemTopoMsgEnabled())
			{
				(*_topoMessage).writeCRCchecksum();
			}

			if (!_incomingStructuredNeighborTable->sendToNeighbor(peerName,
					_topoMessage))
			{
				Trace_Debug(this, "processIncomingDisconnectRequestMsg()",
						"failed to send a Disconnect_reply msg to", "node",
						myNeighbor->getName());
			}

			if (!_incomingStructuredNeighborTable->getRoutable(peerName))
			{
				Trace_Error(this, "processIncomingDisconnectRequestMsg()", "Error: inconsistent state of incomingStructuredNeighborTable",
						"peer", spdr::toString(peerName));
				throw SpiderCastRuntimeError("Error: inconsistent state of incomingStructuredNeighborTable");
				//assert(_incomingStructuredNeighborTable->getRoutable(peerName));
			}

			if (_incomingStructuredNeighborTable->removeEntry(peerName))
			{
				submitConnectivityEvent();
				_routingMgr_SPtr->removeRoutingNeighbor(peerName, myNeighbor);
			}
		}
		else
		{
			Trace_Debug(this, "processIncomingDisconnectRequestMsg()",
					"Warning: could not find neighbor table node", "node",
					peerName->getNodeName());
		}

	}
	else
	{
		if (_currentSuccessor)
		{
			if (!_currentSuccessor->getNodeName().compare(
					peerName->getNodeName()))
			{
				reply = false;
			}
		}

		if (reply && type == (int) TopologyManagerImpl::RANDOM)
		{
			if (_neighborTable->size() <= _config.getRandomDegreeTarget() + 2)
			{
				reply = false;
				addEntryToNeighborsDegreeMap(peerName, degree);
			}
		}

		Neighbor_SPtr myNeighbor = _neighborTable->getNeighbor(
				peerName);
		if (myNeighbor)
		{
			if (myNeighbor->getReceiverId() == 0)
			{
				Trace_Debug(this, "processIncomingDisconnectRequestMsg()",
						"Warning: assert (myNeighbor->getRecieverId() != 0); failed");
			}
			(*_topoMessage).writeH1Header(SCMessage::Type_Topo_Disconnect_Reply);
			ByteBuffer& bb = *(*_topoMessage).getBuffer();
			bb.writeString(peerName->getNodeName());
			bb.writeInt(type);
			bb.writeBoolean(reply);
			if (!reply)
			{
				bb.writeShort(_config.getRandomDegreeTarget() + 2
						- _neighborTable->size());
			}
			(*_topoMessage).updateTotalLength();
			if (_config.isCRCMemTopoMsgEnabled())
			{
				(*_topoMessage).writeCRCchecksum();
			}

			if (!_neighborTable->sendToNeighbor(peerName, _topoMessage))
			{
				Trace_Debug(this, "processIncomingDisconnectRequestMsg()",
						"failed to send a Disconnect_reply msg to", "node",
						myNeighbor->getName());
			}
		}
		else
		{
			Trace_Event(this, "processIncomingDisconnectRequestMsg()",
					"Warning: could not find neighbor table node", "node",
					peerName->getNodeName());
		}
		if (reply)
		{
			if (myNeighbor)
			{
				_waitingConnectionBreakList.push_back(myNeighbor);
				Trace_Debug(this, "processIncomingDisconnectRequestMsg()",
						"Added node to _waitingConnectionBreakSet", "node",
						myNeighbor->getName());
			}
			bool wasRoutable = _neighborTable->getRoutable(peerName);
			if (_neighborTable->removeEntry(peerName))
			{
				ostringstream oss;
				oss << _neighborTable->size();
				Trace_Event(this, "processIncomingDisconnectRequestMsg()",
						"Connectivity event; removed random", "table",
						_neighborTable->toString(), "size", oss.str());
				submitConnectivityEvent();
				if (_memMgr_SPtr)
				{
					_memMgr_SPtr->disconnectedNeighbor(peerName);
					if (myNeighbor && wasRoutable)
					{
						_routingMgr_SPtr->removeRoutingNeighbor(peerName,
								myNeighbor);
					}
				}
				if (!_randomConnectTaskScheduled && (_neighborTable->size()
						< _config.getRandomDegreeTarget() + 2) && _state
						!= TopologyManager::Closed)
				{
					_taskSchedule_SPtr->scheduleDelay(
							_randomConnectTask_SPtr, TaskSchedule::ZERO_DELAY);
					_randomConnectTaskScheduled = true;
					Trace_Event(this, "processIncomingDisconnectRequestMsg()",
							"scheduling a random connect task");
				}
			}
			removeEntryFromNeighborsDegreeMap(peerName);
			_recentlyDisconnected.push_back(peerName);
		}
	}
	Trace_Exit(this, "processIncomingDisconnectRequestMsg()");
}

void TopologyManagerImpl::processIncomingDisconnectReplyMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingDisconnectReplyMsg()");

	boost::shared_ptr<ByteBuffer> bb = (*incomingTopologyMsg).getBuffer();
	int type;
	bool res;
	short degree = 0;
	try
	{
		type = (*bb).readInt();
		res = (*bb).readBoolean();
		if (!res)
		{
			degree = (*bb).readShort();
		}
	} catch (IndexOutOfBoundsException& e)
	{
		String what("Error: ");
		what.append(e.what());
		what.append(
				", Exception while unmarshaling Type_Topo_Disconnect_Reply message from ");
		what.append((*incomingTopologyMsg).getSender()->getNodeName());
		Trace_Error(this, "processIncomingDisconnectReplyMsg()", what);
		throw MessageUnmarshlingException(what);
	}

	ostringstream oss;
	oss << res;
	Trace_Debug(this, "processIncomingDisconnectReplyMsg()", "received",
			"response", oss.str());

	if (res)
	{
		if (type == (int) TopologyManagerImpl::STRUCTURED)
		{
			Neighbor_SPtr myNeighbor;
			{
				boost::recursive_mutex::scoped_lock lock(topo_mutex);
				myNeighbor = _outgoingStructuredNeighborTable->getNeighbor(
						peerName);
				bool wasRoutable =
						_outgoingStructuredNeighborTable->getRoutable(peerName);
				if (_outgoingStructuredNeighborTable->removeEntry(peerName))
				{
					submitConnectivityEvent();
					if (myNeighbor && wasRoutable)
					{
						_routingMgr_SPtr->removeRoutingNeighbor(peerName,
								myNeighbor);
					}
				}
			}
			if (myNeighbor)
			{
				_commAdapter_SPtr->disconnect(myNeighbor);
			}
			else
			{
				Trace_Event(
						this,
						"processIncomingDisconnectReplyMsg()",
						"Warning could not find in _outgoingStructuredNeighborTable table",
						"node", peerName->getNodeName());
			}

		}
		else
		{
			Neighbor_SPtr myNeighbor;
			{
				boost::recursive_mutex::scoped_lock lock(topo_mutex);
				myNeighbor = _neighborTable->getNeighbor(peerName);
				bool wasRoutbale = _neighborTable->getRoutable(peerName);

				if (_neighborTable->removeEntry(peerName))
				{
					ostringstream oss;
					oss << _neighborTable->size();
					Trace_Event(this, "processIncomingDisconnectReplyMsg()",
							"Connectivity event; removed random", "table",
							_neighborTable->toString(), "size", oss.str());

					if (_currentSuccessor)
					{
						if ((*peerName) == (*_currentSuccessor))
						{
							_currentSuccessor = NodeIDImpl_SPtr();
							Trace_Debug(this,
									"processIncomingDisconnectReplyMsg()",
									"Warning; removed _currentSUccessor",
									"node", NodeIDImpl::stringValueOf(peerName));

							if (!_changeSuccessorTaskScheduled && _state
									!= TopologyManager::Closed)
							{
								_taskSchedule_SPtr->scheduleDelay(
										_changeSuccessorTask_SPtr,
										TaskSchedule::ZERO_DELAY);
								_changeSuccessorTaskScheduled = true;
								Trace_Debug(this,
										"processIncomingDisconnectReplyMsg()",
										"scheduling a change successor task");
							}
						}
					}

					submitConnectivityEvent();
					if (_memMgr_SPtr)
					{
						_memMgr_SPtr->disconnectedNeighbor(peerName);
						if (myNeighbor && wasRoutbale)
						{
							_routingMgr_SPtr->removeRoutingNeighbor(peerName,
									myNeighbor);
						}
					}
					if (!_randomConnectTaskScheduled && (_neighborTable->size()
							< _config.getRandomDegreeTarget() + 2) && _state
							!= TopologyManager::Closed)
					{
						_taskSchedule_SPtr->scheduleDelay(
								_randomConnectTask_SPtr,
								TaskSchedule::ZERO_DELAY);
						_randomConnectTaskScheduled = true;
						Trace_Debug(this,
								"processIncomingDisconnectReplyMsg()",
								"scheduling a random connect task");
					}
				}
				removeEntryFromNeighborsDegreeMap(peerName);
			}
			if (myNeighbor)
			{
				if (myNeighbor->getReceiverId() == 0)
				{
					Trace_Event(this, "processIncomingDisconnectReplyMsg()",
							"Warning: assert(myNeighbor->getRecieverId() != 0); failed");
				}
				//			assert(myNeighbor->getRecieverId() != 0);
				_commAdapter_SPtr->disconnect(myNeighbor);
			}
			else
			{
				Trace_Event(this, "processIncomingDisconnectReplyMsg()",
						"Warning could not find in neighbor table", "node",
						peerName->getNodeName());
			}
		}
	}
	else
	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		addEntryToNeighborsDegreeMap(peerName, degree);
	}
	if (type != (int) TopologyManagerImpl::STRUCTURED)
	{
		removeEntryFromRecentlySentDisconnectRequestList(peerName);
	}
	Trace_Exit(this, "processIncomingDisconnectReplyMsg()");
}

void TopologyManagerImpl::processIncomingNodeLeaveMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingNodeLeaveMsg()");
	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		Neighbor_SPtr myNeighbor = _neighborTable->getNeighbor(
				peerName);
		if (myNeighbor)
		{
			if (myNeighbor->getReceiverId() == 0)
			{
				Trace_Event(this, "processIncomingNodeLeaveMsg()",
						"Warning: assert(myNeighbor->getRecieverId() != 0); failed");
			}
			//			assert(myNeighbor->getRecieverId() != 0);

			_waitingConnectionBreakList.push_back(myNeighbor);
			Trace_Debug(this, "processIncomingNodeLeaveMsg()",
					"Added node to _waitingConnectionBreakSet", "node",
					myNeighbor->getName());
		}

		bool wasRoutable = _neighborTable->getRoutable(peerName);
		if (_neighborTable->removeEntry(peerName))
		{
			ostringstream oss;
			oss << _neighborTable->size();
			Trace_Event(this, "processIncomingNodeLeaveMsg()",
					"Connectivity event; removed peer", "table",
					_neighborTable->toString(), "size", oss.str());
			if (_currentSuccessor)
			{
				if ((*peerName) == (*_currentSuccessor))
				{
					_currentSuccessor = NodeIDImpl_SPtr();
					Trace_Debug(this, "processIncomingNodeLeaveMsg()",
							"Warning; removed _currentSUccessor", "node",
							stringValueOf(peerName));
					if (!_changeSuccessorTaskScheduled && _state
							!= TopologyManager::Closed)
					{
						_taskSchedule_SPtr->scheduleDelay(
								_changeSuccessorTask_SPtr,
								TaskSchedule::ZERO_DELAY);
						_changeSuccessorTaskScheduled = true;
						Trace_Debug(this, "processIncomingNodeLeaveMsg()",
								"scheduling a change successor task");
					}
				}
			}
			submitConnectivityEvent();
			if (_memMgr_SPtr)
			{
				_memMgr_SPtr->disconnectedNeighbor(peerName);
				if (myNeighbor && wasRoutable)
				{
					_routingMgr_SPtr->removeRoutingNeighbor(peerName,
							myNeighbor);
				}
			}
			if (!_randomConnectTaskScheduled && (_neighborTable->size()
					< _config.getRandomDegreeTarget() + 2) && _state
					!= TopologyManager::Closed)
			{
				_taskSchedule_SPtr->scheduleDelay(_randomConnectTask_SPtr,
						TaskSchedule::ZERO_DELAY);
				_randomConnectTaskScheduled = true;
				Trace_Debug(this, "processIncomingNodeLeaveMsg()",
						"scheduling a random connect task");
			}
		}
		wasRoutable = _outgoingStructuredNeighborTable->getRoutable(peerName);
		myNeighbor = _outgoingStructuredNeighborTable->getNeighbor(peerName);
		if (_outgoingStructuredNeighborTable->removeEntry(peerName))
		{
			ostringstream oss;
			oss << _outgoingStructuredNeighborTable->size();
			Trace_Event(this, "processIncomingNodeLeaveMsg()",
					"Connectivity event; removed outgoing structured peer",
					"table", _outgoingStructuredNeighborTable->toString(),
					"size", oss.str());
			if (myNeighbor && wasRoutable)
			{
				_routingMgr_SPtr->removeRoutingNeighbor(peerName, myNeighbor);
			}
			submitConnectivityEvent();
			if (!_structuredConnectTaskScheduled
					&& (_outgoingStructuredNeighborTable->size()
							< _config.getStructDegreeTarget())
					&& _config.isStructTopoEnabled() && _state
					!= TopologyManager::Closed)
			{
				_taskSchedule_SPtr->scheduleDelay(
						_structuredConnectTask_SPtr, TaskSchedule::ZERO_DELAY);
				_structuredConnectTaskScheduled = true;
				Trace_Debug(this, "processIncomingNodeLeaveMsg()",
						"scheduling a structured connect task");
			}
		}

		myNeighbor = _incomingStructuredNeighborTable->getNeighbor(peerName);
		if (myNeighbor)
		{
			if (!_incomingStructuredNeighborTable->getRoutable(peerName))
			{
				Trace_Error(this, "processIncomingNodeLeaveMsg()", "Error: inconsistent state of _incomingStructuredNeighborTable",
						"peer",spdr::toString(peerName));
				throw SpiderCastRuntimeError("Error: inconsistent state of _incomingStructuredNeighborTable");
				//assert(_incomingStructuredNeighborTable->getRoutable(peerName));
			}
		}
		if (_incomingStructuredNeighborTable->removeEntry(peerName))
		{
			ostringstream oss;
			oss << _incomingStructuredNeighborTable->size();
			Trace_Event(this, "processIncomingNodeLeaveMsg()",
					"Connectivity event; removed incoming structured peer",
					"table", _incomingStructuredNeighborTable->toString(),
					"size", oss.str());
			if (myNeighbor)
			{
				_routingMgr_SPtr->removeRoutingNeighbor(peerName, myNeighbor);
			}
			submitConnectivityEvent();
		}

		removeEntryFromNeighborsDegreeMap(peerName);
		removeEntryFromCandidatesForFutureRandomConnectionAttemptsList(peerName);
		_recentlyDisconnected.push_back(peerName);
	}
	Trace_Exit(this, "processIncomingNodeLeaveMsg()");
}

void TopologyManagerImpl::processIncomingConnectRequestMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingConnectRequestMsg");

	if (incomingTopologyMsg->getBusName()->toString().compare(
			_config.getBusName()) != 0)
	{
		String errMsg("Error: Wrong bus name: intended: ");
		errMsg.append(incomingTopologyMsg->getBusName()->toString());
		errMsg.append("; My Bus Name: ");
		errMsg.append(_config.getBusName());
		Trace_Error(this, "processIncomingConnectRequestMsg()", errMsg);
		throw SpiderCastRuntimeError(errMsg);
	}

	Neighbor_SPtr myNeighbor;

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		if (_neighborTable->contains(peerName))
		{
			if (_memberName.compare(peerName->getNodeName()) > 0)
			{
				Trace_Debug(this, "processIncomingConnectRequestMsg()",
						"Ignoring request, waiting for response from",
						"Target", peerName->getNodeName());
				return;
			}
			else
			{
				myNeighbor = _neighborTable->getNeighbor(peerName);

				if (!myNeighbor)
				{
					String errMsg("Error: Invalid entry in the neighbor table: ");
					errMsg.append(peerName->getNodeName());
					Trace_Error(this, "processIncomingConnectRequestMsg()",
							errMsg);
					throw SpiderCastRuntimeError(errMsg);
				}
				if (myNeighbor->getReceiverId() != 0
						&& myNeighbor->getReceiverId()
								!= incomingTopologyMsg->getStreamId())
				{
					ostringstream oss;
					oss
							<< "Warning: assert(myNeighbor->getRecieverId() == 0 || myNeighbor->getReceiverId() == incomingTopologyMsg->getStreamId()); failed; myNeighbor id: "
							<< myNeighbor->getReceiverId() << "; message id: "
							<< incomingTopologyMsg->getStreamId();
					Trace_Event(this, "processIncomingConnectRequestMsg()",
							oss.str());
				}
				//				if (myNeighbor->getReceiverId() != 0)
				//				{
				//					ostringstream oss;
				//					oss
				//							<< "Neighbor receiver ID should have been 0 instead it is: ";
				//					oss << myNeighbor->getReceiverId();
				//					Trace_Event(this, "processIncomingConnectRequestMsg()",
				//							oss.str());
				//					throw SpiderCastRuntimeError(oss.str());
				//				}

				myNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());
				Trace_Debug(this, "processIncomingConnectRequestMsg()",
						"using existing neighbor", "Target",
						peerName->getNodeName());
			}
		}

		if (_outgoingRandomConnectionRequests->contains(peerName))
		{
			if (_memberName.compare(peerName->getNodeName()) > 0)
			{
				Trace_Debug(
						this,
						"processIncomingConnectRequestMsg()",
						"Ignoring request, waiting for outgoing connection attempt",
						"Target", peerName->getNodeName());
				return;
			}
		}
	}

	if (!myNeighbor)
	{
		myNeighbor = _commAdapter_SPtr->connectOnExisting(peerName);
	}

	if (!myNeighbor)
	{
		Trace_Event(this, "processIncomingConnectRequestMsg()",
				"Warning: connectOnExisting() failed", "target",
				peerName->getNodeName());
	}
	else
	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		int numCurrentConnections = _neighborTable->size()
				+ _outgoingRandomConnectionRequests->size();

		bool reply = _neighborTable->contains(peerName);

		if (!reply)
		{
			if (_setSuccessor)
			{
				reply = ((*_setSuccessor) == (*peerName));
				if (reply)
				{
					_currentSuccessor = _setSuccessor;

				}
			}
		}

		if (!reply)
			reply = _outgoingRandomConnectionRequests->contains(peerName);

		if (!reply)
			reply = numCurrentConnections >= _config.getRandomDegreeTarget()
					+ _config.getRandomDegreeMargin() + 2 ? false : true;

		(*_topoMessage).writeH1Header(SCMessage::Type_Topo_Connect_Random_Reply);
		ByteBuffer& bb = *(*_topoMessage).getBuffer();
		bb.writeString(peerName->getNodeName());
		bb.writeBoolean(reply);
		if (reply)
		{
			// send local current degree (num of connections this node still lacks)
			bb.writeShort(_config.getRandomDegreeTarget() + 2
					- _neighborTable->size());
		}
		else
		{
			// propose another node to attempt to connect to
			short highest = 0;
			NodeIDImpl_SPtr highestNode;
			NeighborsDegreeMap::iterator iter;
			for (iter = _neighborsDegreeMap.begin(); iter
					!= _neighborsDegreeMap.end(); iter++)
			{
				if ((*iter).second > highest)
				{
					highest = (*iter).second;
					highestNode = (*iter).first;
				}
			}
			if (highestNode)
			{
				if (_neighborTable->contains(highestNode))
				{
					// ensure that we have network endpoints in the nodeid we intend to send
					if (highestNode->getNetworkEndpoints().getNumAddresses()
							== (size_t) 0)
					{
						highestNode = _nodeIdCache.getOrCreate(
								highestNode->getNodeName());
						if (highestNode->getNetworkEndpoints().getNumAddresses()
								== (size_t) 0)
						{
							highest = -1; // cause the node not to be sent
						}
						else
						{
							removeEntryFromNeighborsDegreeMap(highestNode);
							addEntryToNeighborsDegreeMap(highestNode, highest);
						}
						Trace_Debug(this, "processIncomingConnectRequestMsg",
								"bogus", "node", highestNode->toString());
					}
				}
				else
				{
					removeEntryFromNeighborsDegreeMap(highestNode);
					highest = -1;
				}
			}
			if (highest > 0 && highestNode)
			{
				bb.writeBoolean(true);
				(*_topoMessage).writeNodeID(highestNode);
			}
			else
			{
				bb.writeBoolean(false);
			}
		}
		(*_topoMessage).updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			(*_topoMessage).writeCRCchecksum();
		}

		if (myNeighbor->getReceiverId() != 0 && myNeighbor->getReceiverId()
				!= incomingTopologyMsg->getStreamId())
		{
			Trace_Event(
					this,
					"processIncomingConnectRequestMsg()",
					"Warning: assert(myNeighbor->getRecieverId() == 0) || myNeighbor->getRecieverId()=!= incomingTopologyMsg->getSender(); failed");
		}
		myNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());
		Trace_Debug(this, "processIncomingConnectRequestMsg()",
				"Set receiver stream id inside neighbor");

		if (myNeighbor->sendMessage(_topoMessage) != 0)
		{
			Trace_Debug(this, "processIncomingConnectRequestMsg",
					"couldn't send a connect_OK message to", "node",
					myNeighbor->getName());
			// TODO: decide what to do
		}

		if (reply)
		{
			_neighborTable->addEntry(peerName, myNeighbor);
			ostringstream oss;
			oss << _neighborTable->size();
			Trace_Event(this, "processIncomingConnectRequestMsg()",
					"Connectivity event; added random", "table",
					_neighborTable->toString(), "size", oss.str());
			_memMgr_SPtr->newNeighbor(peerName);
			_routingMgr_SPtr->addRoutingNeighbor(peerName, myNeighbor);
			_neighborTable->setRoutable(peerName);
			submitConnectivityEvent();
			boost::shared_ptr<ByteBuffer> bb =
					(*incomingTopologyMsg).getBuffer();
			short degree;
			try
			{
				degree = (*bb).readShort();
			} catch (IndexOutOfBoundsException& e)
			{
				String what("Error: ");
				what.append(e.what());
				what.append(
						", Exception while unmarshaling Type_Topo_Connect_Request message from ");
				what.append(peerName->getNodeName());
				Trace_Error(this, "processIncomingConnectRequestMsg()", what);
				throw MessageUnmarshlingException(what);
			}
			addEntryToNeighborsDegreeMap(peerName, degree);
			if (!_randomDisConnectTaskScheduled && (_neighborTable->size()
					> _config.getRandomDegreeTarget() + 2) && _state
					!= TopologyManager::Closed)
			{
				_taskSchedule_SPtr->scheduleDelay(
						_randomDisConnectTask_SPtr,
						boost::posix_time::milliseconds(
								_config.getTopologyPeriodicTaskIntervalMillis()));
				_randomDisConnectTaskScheduled = true;
				Trace_Debug(this, "processIncomingConnectRequestMsg()",
						"scheduling a random disconnect task");
			}
		}
	}

	Trace_Exit(this, "processIncomingConnectRequestMsg");
}

void TopologyManagerImpl::processIncomingConnectStructuredRequestMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingConnectStructuredRequestMsg");

	if (incomingTopologyMsg->getBusName()->toString().compare(
			_config.getBusName()) != 0)
	{
		String errMsg("Error: Wrong bus name: intended: ");
		errMsg.append(incomingTopologyMsg->getBusName()->toString());
		errMsg.append("; My Bus Name: ");
		errMsg.append(_config.getBusName());
		Trace_Error(this, "processIncomingConnectStructuredRequestMsg()",
				errMsg);
		throw SpiderCastRuntimeError(errMsg);
	}

	Neighbor_SPtr myNeighbor;

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		if (_incomingStructuredNeighborTable->contains(peerName))
		{

			myNeighbor
					= _incomingStructuredNeighborTable->getNeighbor(peerName);

			if (!myNeighbor)
			{
				String errMsg("Error: Invalid entry in the neighbor table: ");
				errMsg.append(peerName->getNodeName());
				Trace_Error(this,
						"processIncomingConnectStructuredRequestMsg()", errMsg);
				throw SpiderCastRuntimeError(errMsg);
			}
			if (myNeighbor->getReceiverId() != 0 && myNeighbor->getReceiverId()
					!= incomingTopologyMsg->getStreamId())
			{
				ostringstream oss;
				oss
						<< "Warning: assert(myNeighbor->getRecieverId() == 0 || myNeighbor->getReceiverId() == incomingTopologyMsg->getStreamId()); failed; myNeighbor id: "
						<< myNeighbor->getReceiverId() << "; message id: "
						<< incomingTopologyMsg->getStreamId();
				Trace_Debug(this, "processIncomingConnectRequestMsg()",
						oss.str());
			}

			myNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());
			Trace_Debug(this, "processIncomingConnectStructuredRequestMsg()",
					"using existing neighbor", "Target",
					peerName->getNodeName());

		}
	}

	if (!myNeighbor)
	{
		myNeighbor = _commAdapter_SPtr->connectOnExisting(peerName);
	}

	if (!myNeighbor)
	{
		Trace_Event(this, "processIncomingConnectStructuredRequestMsg()",
				"Warning: connectOnExisting() failed", "target",
				peerName->getNodeName());
	}
	else
	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		int numCurrentConnections = _incomingStructuredNeighborTable->size();

		bool reply = _incomingStructuredNeighborTable->contains(peerName);

		if (!reply)
		{
			reply = numCurrentConnections >= _config.getStructDegreeTarget()
					* 2 ? false : true;
		}

		(*_topoMessage).writeH1Header(
				SCMessage::Type_Topo_Connect_Structured_Reply);
		ByteBuffer& bb = *(*_topoMessage).getBuffer();
		bb.writeString(peerName->getNodeName());
		bb.writeBoolean(reply);

		(*_topoMessage).updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			(*_topoMessage).writeCRCchecksum();
		}

		if (myNeighbor->getReceiverId() != 0 && myNeighbor->getReceiverId()
				!= incomingTopologyMsg->getStreamId())
		{
			Trace_Event(
					this,
					"processIncomingConnectStructuredRequestMsg()",
					"Warning: assert(myNeighbor->getRecieverId() == 0) || myNeighbor->getRecieverId()=!= incomingTopologyMsg->getSender(); failed");
		}
		myNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());
		Trace_Debug(this, "processIncomingConnectStructuredRequestMsg()",
				"Set receiver stream id inside neighbor");

		if (myNeighbor->sendMessage(_topoMessage) != 0)
		{
			Trace_Debug(this, "processIncomingConnectStructuredRequestMsg",
					"couldn't send a connect_Structured_Reply message to",
					"node", myNeighbor->getName());
			// TODO: decide what to do
		}
		if (reply)
		{
			_incomingStructuredNeighborTable->addEntry(peerName, myNeighbor);
			_routingMgr_SPtr->addRoutingNeighbor(peerName, myNeighbor);
			_incomingStructuredNeighborTable->setRoutable(peerName);
			submitConnectivityEvent();
		}
	}

	Trace_Exit(this, "processIncomingConnectStructuredRequestMsg");
}

void TopologyManagerImpl::processIncomingConnectReplyMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{

	Trace_Entry(this, "processIncomingConnectReplyMsg");

	boost::shared_ptr<ByteBuffer> bb = (*incomingTopologyMsg).getBuffer();
	bool res;
	bool connectElswhere = false;
	NodeIDImpl_SPtr otherNodeToConnect;
	short degree = 0;
	try
	{
		res = (*bb).readBoolean();
		if (res)
		{
			degree = (*bb).readShort();
		}
		else
		{
			connectElswhere = (*bb).readBoolean();
			if (connectElswhere)
			{
				otherNodeToConnect = (*incomingTopologyMsg).readNodeID();
			}
		}
	} catch (IndexOutOfBoundsException& e)
	{
		String what("Error: ");
		what.append(e.what());
		what.append(
				", Exception while unmarshaling Type_Topo_Connect_Reply message from ");
		what.append((*incomingTopologyMsg).getSender()->getNodeName());
		Trace_Error(this, "processIncomingConnectReplyMsg()", what);
		throw MessageUnmarshlingException(what);
	}
	ostringstream oss;
	oss << res;
	Trace_Debug(this, "processIncomingConnectReplyMsg()", "received",
			"response", oss.str());
	Neighbor_SPtr peerNeighbor;
	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		peerNeighbor = _neighborTable->getNeighbor(peerName);
	}
	if (!peerNeighbor)
	{
		Trace_Debug(this, "processIncomingConnectReplyMsg()",
				"Warning could not find in neighbor table", "node",
				peerName->getNodeName());
	}
	else
	{
		if (peerNeighbor->getReceiverId() != 0 && peerNeighbor->getReceiverId()
				!= incomingTopologyMsg->getStreamId())
		{
			Trace_Debug(
					this,
					"processIncomingConnectReplyMsg()",
					"Warning: assert(peerNeighbor->getRecieverId() == 0 || peerNeighbor->getRecieverId() == incomingTopologyMsg->getStreamId())); failed");
		}
		peerNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());

		Trace_Debug(this, "processIncomingConnectReplyMsg()",
				"Set receiver stream id inside neighbor");

		if (res)
		{
			boost::recursive_mutex::scoped_lock lock(topo_mutex);
			// Was already added to the neighbor table when sent the connect request msg
			_memMgr_SPtr->newNeighbor(peerName);
			addEntryToNeighborsDegreeMap(peerName, degree);
			_routingMgr_SPtr->addRoutingNeighbor(peerName, peerNeighbor);
			_neighborTable->setRoutable(peerName);
		}
		else
		{
			{
				boost::recursive_mutex::scoped_lock lock(topo_mutex);
				if (_neighborTable->removeEntry(peerName)) // was inserted when sent the message

				{
					ostringstream oss;
					oss << _neighborTable->size();
					Trace_Event(this, "processIncomingConnectReplyMsg()",
							"Connectivity event; removed random", "table",
							_neighborTable->toString(), "size", oss.str());
					if (!_randomConnectTaskScheduled && (_neighborTable->size()
							< _config.getRandomDegreeTarget() + 2) && _state
							!= TopologyManager::Closed)
					{
						_taskSchedule_SPtr->scheduleDelay(
								_randomConnectTask_SPtr,
								TaskSchedule::ZERO_DELAY);
						_randomConnectTaskScheduled = true;
						Trace_Debug(this, "processIncomingConnectReplyMsg()",
								"scheduling a random connect task");
					}
					submitConnectivityEvent();
				}
				_memMgr_SPtr->disconnectedNeighbor(peerName);
				//_routingMgr_SPtr->removeRoutingNeighbor(peerName, peerNeighbor);
				removeEntryFromNeighborsDegreeMap(peerName);
				if (connectElswhere && otherNodeToConnect)
				{
					Trace_Debug(
							this,
							"processIncomingConnectReplyMsg()",
							"adding to _candidatesForFutureRandomConnectionAttempts",
							"node", otherNodeToConnect->getNodeName());
					boost::recursive_mutex::scoped_lock lock(topo_mutex);
					_candidatesForFutureRandomConnectionAttempts.push_front(
							otherNodeToConnect);
				}
				if (_currentSuccessor)
				{
					if ((*peerName) == (*_currentSuccessor))
					{
						if (!_changeSuccessorTaskScheduled)
						{
							_taskSchedule_SPtr->scheduleDelay(
									_changeSuccessorTask_SPtr,
									TaskSchedule::ZERO_DELAY);
							_changeSuccessorTaskScheduled = true;
							Trace_Debug(this,
									"processIncomingConnectReplyMsg()",
									"scheduling a change successor task");
						}
					}
				}
			}
			_commAdapter_SPtr->disconnect(peerNeighbor);
		}
	}

	Trace_Exit(this, "processIncomingConnectReplyMsg");
}

void TopologyManagerImpl::processIncomingConnectStructuredReplyMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{

	Trace_Entry(this, "processIncomingConnectStructuredReplyMsg");

	boost::shared_ptr<ByteBuffer> bb = (*incomingTopologyMsg).getBuffer();
	bool res;
	try
	{
		res = (*bb).readBoolean();
	} catch (IndexOutOfBoundsException& e)
	{
		String what("Error: ");
		what.append(e.what());  //TODO continue from here <<<<
		what.append(
				", Exception while unmarshaling Type_Topo_Connect_Structured_Reply message from ");
		what.append((*incomingTopologyMsg).getSender()->getNodeName());
		Trace_Event(this, "processIncomingConnectStructuredReplyMsg()", what);
		throw MessageUnmarshlingException(what);
	}
	ostringstream oss;
	oss << res;
	Trace_Event(this, "processIncomingConnectStructuredReplyMsg()", "received",
			"response", oss.str());
	Neighbor_SPtr peerNeighbor;
	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);
		peerNeighbor = _outgoingStructuredNeighborTable->getNeighbor(peerName);
	}
	if (!peerNeighbor)
	{
		Trace_Event(this, "processIncomingConnectStructuredReplyMsg()",
				"Warning could not find in neighbor table", "node",
				peerName->getNodeName());
	}
	else
	{
		if (peerNeighbor->getReceiverId() != 0 && peerNeighbor->getReceiverId()
				!= incomingTopologyMsg->getStreamId())
		{
			Trace_Event(
					this,
					"processIncomingConnectStructuredReplyMsg()",
					"Warning: assert(peerNeighbor->getRecieverId() == 0 || peerNeighbor->getRecieverId() == incomingTopologyMsg->getStreamId())); failed");
		}
		peerNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());

		Trace_Event(this, "processIncomingConnectStructuredReplyMsg()",
				"Set receiver stream id inside neighbor");

		if (res)
		{
			boost::recursive_mutex::scoped_lock lock(topo_mutex);
			// Was already added to the neighbor table when sent the connect request msg
			_routingMgr_SPtr->addRoutingNeighbor(peerName, peerNeighbor);
			_outgoingStructuredNeighborTable->setRoutable(peerName);
		}
		else
		{
			{
				boost::recursive_mutex::scoped_lock lock(topo_mutex);
				if (_outgoingStructuredNeighborTable->removeEntry(peerName)) // was inserted when sent the message

				{
					if (!_structuredConnectTaskScheduled
							&& (_outgoingStructuredNeighborTable->size()
									< _config.getStructDegreeTarget())
							&& _state != TopologyManager::Closed)
					{
						_taskSchedule_SPtr->scheduleDelay(
								_structuredConnectTask_SPtr,
								TaskSchedule::ZERO_DELAY);
						_structuredConnectTaskScheduled = true;
						Trace_Event(this,
								"processIncomingConnectStructuredReplyMsg()",
								"scheduling a strcutured connect task");
					}
				}
			}
			_commAdapter_SPtr->disconnect(peerNeighbor);
		}
		submitConnectivityEvent();
	}

	Trace_Exit(this, "processIncomingConnectStructuredReplyMsg");
}

void TopologyManagerImpl::processIncomingDegreeChangedMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingDegreeChangedMsg()");
	boost::recursive_mutex::scoped_lock lock(topo_mutex);
	boost::shared_ptr<ByteBuffer> bb = (*incomingTopologyMsg).getBuffer();
	short degree = 0;
	try
	{
		degree = (*bb).readShort();
	} catch (IndexOutOfBoundsException& e)
	{
		String what(e.what());
		what.append(
				", Exception while unmarshaling Type_Topo_Degree_Changed message from: ");
		what.append(peerName->getNodeName());
		Trace_Event(this, "processIncomingDegreeChangedMsg()", what);
		throw MessageUnmarshlingException(what);
	}
	addEntryToNeighborsDegreeMap(peerName, degree);
	Neighbor_SPtr myNeighbor = _neighborTable->getNeighbor(
			peerName);
	if (myNeighbor)
	{
		if (myNeighbor->getReceiverId() != 0 && myNeighbor->getReceiverId()
				!= incomingTopologyMsg->getStreamId())
		{
			Trace_Event(
					this,
					"processIncomingDegreeChangedMsg()",
					"Warning: assert(myNeighbor->getRecieverId() == 0 || myNeighbor->getRecieverId() == incomingTopologyMsg->getStreamId()); failed");
		}
		myNeighbor->setReceiverId(incomingTopologyMsg->getStreamId());
		Trace_Event(this, "processIncomingDegreeChangedMsg()",
				"Set receiver stream id inside neighbor");

	}
	Trace_Exit(this, "processIncomingDegreeChangedMsg()");
}

void TopologyManagerImpl::processIncomingCommEventMsg(
		SCMessage_SPtr incomingTopologyMsg, NodeIDImpl_SPtr peerName)
{
	Trace_Entry(this, "processIncomingCommEventMsg()");

	//TODO deal events to Hierarchy manager too.
	// CommEventInfo::On_Connection_*: by the connection context.
	// CommEventInfo::New_Source: by bus-name
	// CommEventInfo::On_Break: by bus-name, or forward all breaks to hierarchy too

	boost::shared_ptr<CommEventInfo> inEvent =
			incomingTopologyMsg->getCommEventInfo();

	Trace_Event(this, "processIncomingCommEventMsg()",
			"event",spdr::toString(inEvent), "peer", spdr::toString(peerName));

	switch ((*inEvent).getType())
	{

	case CommEventInfo::New_Source:
	{
		Trace_Event(this, "processIncomingCommEventMsg()",
				"received a New Source Event msg");
	}
		break;

	case CommEventInfo::On_Break:
	{
		Trace_Event(this, "processIncomingCommEventMsg()",
				"received onBreak msg");

		//Let Discovery, Random & Ring process it

		{
			boost::recursive_mutex::scoped_lock lock(topo_mutex);

			if (_discoveryMap->removeEntry(peerName))
			{
				Trace_Event(this, "processIncomingCommEventMsg()",
						"removed entry from _discoveryMap", "node",
						peerName->getNodeName());
			}

			if (_outgoingRandomConnectionRequests->removeEntry(peerName))
			{
				Trace_Event(this, "processIncomingCommEventMsg()",
						"removed entry from _outgoingRandomConnectionRequests ");
			}

			if (_neighborTable->removeEntry(peerName))
			{
				Trace_Event(this, "processIncomingCommEventMsg()",
						"removed entry from _neighborTable");
				ostringstream oss;
				oss << _neighborTable->size();
				Trace_Event(this, "processIncomingCommEventMsg()",
						"Connectivity event; removed peer", "table",
						_neighborTable->toString(), "size", oss.str());

				Trace_Event(this, "processIncomingCommEventMsg()",
						"reporting suspect", "node", peerName->getNodeName());
				_memMgr_SPtr->reportSuspect(peerName);
				removeEntryFromNeighborsDegreeMap(peerName);
				if (!_randomConnectTaskScheduled && (_neighborTable->size()
						< _config.getRandomDegreeTarget() + 2) && _state
						!= TopologyManager::Closed)
				{
					_taskSchedule_SPtr->scheduleDelay(
							_randomConnectTask_SPtr, TaskSchedule::ZERO_DELAY);
					_randomConnectTaskScheduled = true;
					Trace_Event(this, "processIncomingCommEventMsg()",
							"scheduling a random connect task");
				}
				if (_currentSuccessor)
				{
					if ((*peerName) == (*_currentSuccessor))
					{
						_currentSuccessor = NodeIDImpl_SPtr();
						Trace_Event(this, "processIncomingCommEventMsg()",
								"Warning; removed _currentSUccessor", "node",
								toString(peerName));
						if (!_changeSuccessorTaskScheduled && _state
								!= TopologyManager::Closed)
						{
							_taskSchedule_SPtr->scheduleDelay(
									_changeSuccessorTask_SPtr,
									TaskSchedule::ZERO_DELAY);
							_changeSuccessorTaskScheduled = true;
							Trace_Event(this, "processIncomingCommEventMsg()",
									"scheduling a change successor task");
						}

					}
				}
				submitConnectivityEvent();
			}

			Neighbor_SPtr myNeighbor =
					_outgoingStructuredNeighborTable->getNeighbor(peerName);
			bool wasRoutable = _outgoingStructuredNeighborTable->getRoutable(
					peerName);

			if (_outgoingStructuredNeighborTable->removeEntry(peerName))
			{
				Trace_Event(this, "processIncomingCommEventMsg()",
						"removed entry from _outgoingStructuredNeighborTable");
				ostringstream oss;
				oss << _outgoingStructuredNeighborTable->size();
				Trace_Event(this, "processIncomingCommEventMsg()",
						"Connectivity event; removed outgoing structured peer",
						"table", _outgoingStructuredNeighborTable->toString(),
						"size", oss.str());
				if (myNeighbor && wasRoutable)
				{
					_routingMgr_SPtr->removeRoutingNeighbor(peerName,
							myNeighbor);
				}
				if (!_structuredConnectTaskScheduled
						&& (_outgoingStructuredNeighborTable->size()
								< _config.getStructDegreeTarget()) && _state
						!= TopologyManager::Closed)
				{
					_taskSchedule_SPtr->scheduleDelay(
							_structuredConnectTask_SPtr,
							TaskSchedule::ZERO_DELAY);
					_structuredConnectTaskScheduled = true;
					Trace_Event(this, "processIncomingCommEventMsg()",
							"scheduling a structured connect task");
				}
				submitConnectivityEvent();
			}

			myNeighbor
					= _incomingStructuredNeighborTable->getNeighbor(peerName);
			if (myNeighbor)
			{
				if (!_incomingStructuredNeighborTable->getRoutable(peerName))
				{
					Trace_Error(this, "processIncomingCommEventMsg()", "Error: inconsistent state of incomingStructuredNeighborTable",
							"peer", spdr::toString(peerName));
					throw SpiderCastRuntimeError("Error: inconsistent state of incomingStructuredNeighborTable");
					//assert(_incomingStructuredNeighborTable->getRoutable(peerName));
				}
			}

			if (_incomingStructuredNeighborTable->removeEntry(peerName))
			{
				Trace_Event(this, "processIncomingCommEventMsg()",
						"removed entry from _incomingStructuredNeighborTable");
				ostringstream oss;
				oss << _incomingStructuredNeighborTable->size();
				Trace_Event(this, "processIncomingCommEventMsg()",
						"Connectivity event; removed incoming structured peer",
						"table", _incomingStructuredNeighborTable->toString(),
						"size", oss.str());
				if (myNeighbor)
				{
					_routingMgr_SPtr->removeRoutingNeighbor(peerName,
							myNeighbor);
				}

				submitConnectivityEvent();
			}

			removeEntryFromCandidatesForFutureRandomConnectionAttemptsList(
					peerName);
			removeEntryFromRecentlyDisconnectedList(peerName);
			removeEntryFromRecentlySentDisconnectRequestList(peerName);

		}
		removeEntryFromWaitingConnectionBreakList(peerName);
		//Let Hierarchy process it
		_hierarchyMngr_SPtr->processIncomingCommEventMsg(incomingTopologyMsg);

	}
		break;

	case CommEventInfo::On_Connection_Success:
	{
		Trace_Event(this, "processIncomingCommEventMsg()",
				"received an on connection success Event msg");
		//deal event to topo or hierarchy using context
		int ctx = inEvent->getContext();
		if (ctx == RandomCtx || ctx == DiscoveryCtx || ctx == RingCtx || ctx
				== StructuredCtx)
		{
			onSuccess((*inEvent).getNeighbor(), (*inEvent).getContext());
		}
		else if (ctx == HierarchyCtx)
		{
			_hierarchyMngr_SPtr->processIncomingCommEventMsg(
					incomingTopologyMsg);
		}
		else
		{
			//Fail-Fast
			ostringstream oss;
			oss << "Error: CommEvent with an unknown context, ctx=" << ctx << "; event: " << inEvent->toString();
			Trace_Error(this, "processIncomingCommEventMsg()", oss.str());
			throw SpiderCastRuntimeError(oss.str());
		}
	}
		break;

	case CommEventInfo::On_Connection_Failure:
	{
		Trace_Event(this, "processIncomingCommEventMsg()",
				"received an on connection failure Event msg");
		//deal event to topo or hierarchy using context
		int ctx = inEvent->getContext();
		if (ctx == RandomCtx || ctx == DiscoveryCtx || ctx == RingCtx || ctx
				== StructuredCtx)
		{
			onFailure(peerName->getNodeName(), (*inEvent).getErrCode(),
					(*inEvent).getErrMsg(), (*inEvent).getContext());
		}
		else if (ctx == HierarchyCtx)
		{
			_hierarchyMngr_SPtr->processIncomingCommEventMsg(
					incomingTopologyMsg);
		}
		else
		{
			//Fail-Fast
			ostringstream oss;
			oss << "Error: CommEvent with an unknown context, ctx=" << ctx << "; event: " << inEvent->toString();
			Trace_Error(this, "processIncomingCommEventMsg()", oss.str());
			throw SpiderCastRuntimeError(oss.str());
		}
	}
		break;

	case CommEventInfo::Warning_Connection_Refused:
	{
		_coreInterface.submitExternalEvent(event::SpiderCastEvent_SPtr(
				new event::WarningEvent(event::Warning_Connection_Refused,
						static_cast<event::ErrorCode>(inEvent->getErrCode()),
						inEvent->getErrMsg())));
	}
		break;

	case CommEventInfo::Warning_UDP_Message_Refused:
	{
		_coreInterface.submitExternalEvent(event::SpiderCastEvent_SPtr(
				new event::WarningEvent(event::Warning_Message_Refused,
						static_cast<event::ErrorCode>(inEvent->getErrCode()),
						inEvent->getErrMsg())));
	}
	break;

	case CommEventInfo::Suspicion_Duplicate_Remote_Node:
	{
		bool res = _memMgr_SPtr->reportSuspicionDuplicateRemoteNode(peerName, inEvent->getIncNum());
		if (res)
		{
			Trace_Warning(this, "processIncomingCommEventMsg()", "Warning: Duplicate remote node suspicion (AKA 'split brain'). Details:",
						"event",spdr::toString(inEvent), "peer", spdr::toString(peerName));
		}
	}
	break;

	case CommEventInfo::Fatal_Error:
	{
		_coreInterface.componentFailure(
				inEvent->getErrMsg(),
				static_cast<event::ErrorCode>(inEvent->getErrCode()));
	}
	break;

		//TODO deal additional events w/o context to both topo and hierarchy

	default:
	{
		//Fail-Fast
		ostringstream oss;
		oss << (*inEvent).getType();
		Trace_Event(this, "processIncomingCommEventMsg()",
				"received an unknown event type", "type", oss.str());
		throw SpiderCastRuntimeError(oss.str());
	}
		break;
	}
	Trace_Exit(this, "processIncomingCommEventMsg()");
}

bool TopologyManagerImpl::isInRecentlySentDisconnectRequestList(
		NodeIDImpl_SPtr target)
{
	Trace_Entry(this, "isInRecentlySentDisconnectRequestList()", "node",
			target->getNodeName());
	bool ret = false;
	list<NodeIDImpl_SPtr>::iterator iter;

	boost::recursive_mutex::scoped_lock lock(topo_mutex);

	for (iter = _recentlySentDisconnectRequest.begin(); iter
			!= _recentlySentDisconnectRequest.end(); iter++)
	{
		if (!((*iter)->getNodeName().compare(target->getNodeName())))
		{
			ret = true;
			break;
		}
	}

	Trace_Exit<bool> (this, "isInRecentlySentDisconnectRequestList", ret);

	return ret;

}

bool TopologyManagerImpl::isRecentlyDisconnected(NodeIDImpl_SPtr target)
{
	Trace_Entry(this, "isRecentlyDisconnected()", "node", target->getNodeName());
	bool ret = false;
	list<NodeIDImpl_SPtr>::iterator iter;

	boost::recursive_mutex::scoped_lock lock(topo_mutex);

	for (iter = _recentlyDisconnected.begin(); iter
			!= _recentlyDisconnected.end(); iter++)
	{
		if (!((*iter)->getNodeName().compare(target->getNodeName())))
		{
			ret = true;
			break;
		}
	}

	Trace_Exit<bool> (this, "isRecentlyDisconnected()", ret);

	return ret;

}

void TopologyManagerImpl::removeEntryFromRecentlySentDisconnectRequestList(
		NodeIDImpl_SPtr nodeName)
{
	Trace_Entry(this, "removeEntryFromRecentlyDisconnectedList()", "node",
			nodeName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(topo_mutex);

	bool found = false;
	list<NodeIDImpl_SPtr>::iterator iter =
			_recentlySentDisconnectRequest.begin();
	while (iter != _recentlySentDisconnectRequest.end())
	{
		if (!((*iter)->getNodeName().compare(nodeName->getNodeName())))
		{
			_recentlySentDisconnectRequest.erase(iter);
			found = true;
			break;
		}
		iter++;
	}

	Trace_Exit<bool> (this,
			"removeEntryFromCandidatesForFutureRandomConnectionAttemptsList()",
			found);

}

void TopologyManagerImpl::removeEntryFromRecentlyDisconnectedList(
		NodeIDImpl_SPtr nodeName)
{

	Trace_Entry(this, "removeEntryFromRecentlyDisconnectedList()", "node",
			nodeName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(topo_mutex);

	bool found = false;
	list<NodeIDImpl_SPtr>::iterator iter = _recentlyDisconnected.begin();
	while (iter != _recentlyDisconnected.end())
	{
		if (!((*iter)->getNodeName().compare(nodeName->getNodeName())))
		{
			_recentlyDisconnected.erase(iter);
			found = true;
			break;
		}
		iter++;
	}

	Trace_Exit<bool> (this,
			"removeEntryFromCandidatesForFutureRandomConnectionAttemptsList()",
			found);

}

void TopologyManagerImpl::removeEntryFromCandidatesForFutureRandomConnectionAttemptsList(
		NodeIDImpl_SPtr nodeName)
{
	Trace_Entry(this,
			"removeEntryFromCandidatesForFutureRandomConnectionAttemptsList()",
			"node", nodeName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(topo_mutex);

	bool found = false;
	list<NodeIDImpl_SPtr>::iterator iter =
			_candidatesForFutureRandomConnectionAttempts.begin();
	while (iter != _candidatesForFutureRandomConnectionAttempts.end())
	{
		if (!((*iter)->getNodeName().compare(nodeName->getNodeName())))
		{
			_candidatesForFutureRandomConnectionAttempts.erase(iter);
			found = true;
			break;
		}
		iter++;
	}
	ostringstream oss;
	oss << found;
	Trace_Exit(this,
			"removeEntryFromCandidatesForFutureRandomConnectionAttemptsList()",
			"found", oss.str());
}

void TopologyManagerImpl::removeEntryFromWaitingConnectionBreakList(
		NodeIDImpl_SPtr nodeName)
{
	Trace_Entry(this, "removeEntryFromWaitingConnectionBreakList()", "node",
			nodeName->getNodeName());

	list<Neighbor_SPtr > toDisconnectList;
	list<Neighbor_SPtr >::iterator iter;

	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		bool scan = true;
		while (scan)
		{
			scan = false;
			iter = _waitingConnectionBreakList.begin();
			while (iter != _waitingConnectionBreakList.end())
			{
				String name = (*iter) ? (*iter)->getName() : "NULL";
				Trace_Event(this, "removeEntryFromWaitingConnectionBreakList",
						"In the loop", "node", name);
				if (!((*iter)->getName().compare(nodeName->getNodeName())))
				{
					toDisconnectList.push_back(*iter);
					_waitingConnectionBreakList.erase(iter);
					scan = true;
					break;
				}
				iter++;
			}
		}
	}

	iter = toDisconnectList.begin();
	while (iter != toDisconnectList.end())
	{
		_commAdapter_SPtr->disconnect(*iter);
		iter++;
	}

	int numErased = toDisconnectList.size();
	ostringstream oss;
	oss << numErased;
	Trace_Exit(this, "removeEntryFromWaitingConnectionBreakList()", "found",
			oss.str());

}

void TopologyManagerImpl::addEntryToNeighborsDegreeMap(
		NodeIDImpl_SPtr nodeName, short degree)
{
	ostringstream oss;
	oss << degree;
	Trace_Entry(this, "addEntryToNeighborsDegreeMap()", "node",
			nodeName->getNodeName(), "degree-vacancy", oss.str());

	boost::recursive_mutex::scoped_lock lock(topo_mutex);
	if (_neighborTable->contains(nodeName))
	{
		NeighborsDegreeMap::iterator iter = _neighborsDegreeMap.find(nodeName);
		if (iter == _neighborsDegreeMap.end())
		{
			_neighborsDegreeMap.insert(make_pair(nodeName, degree));
			myDegreeChanged();
		}
		else
		{
			iter->second = degree;
		}
	}
	else
	{
		Trace_Event(this, "addEntryToNeighborsDegreeMap",
				"skipping since entry not found in neighbors table");
	}
	Trace_Exit(this, "addEntryToNeighborsDegreeMap()");
}

void TopologyManagerImpl::myDegreeChanged()
{
	_myDegreeChanged = true;
	if (!_updateDegreeTaskScheduled && _state != TopologyManager::Closed)
	{
		_taskSchedule_SPtr->scheduleDelay(_updateDegreeTask_SPtr,
				boost::posix_time::milliseconds(
						_config.getTopologyPeriodicTaskIntervalMillis()));
		_updateDegreeTaskScheduled = true;
		Trace_Event(this, "myDegreeChanged()",
				"scheduling an update degree task");
	}
}

bool TopologyManagerImpl::structuredTopoRefreshNeeded()
{
	Trace_Entry(this, "structuredTopoRefreshNeeded");

	int currentViewSize = _memMgr_SPtr->getViewSize();

	bool ret = _outgoingStructuredNeighborTable->refreshNeeded(currentViewSize);

	Trace_Exit<bool> (this, "structuredTopoRefreshNeeded", ret);
	return ret;
}

void TopologyManagerImpl::removeEntryFromNeighborsDegreeMap(
		NodeIDImpl_SPtr nodeName)
{
	if (_neighborsDegreeMap.erase(nodeName))
	{
		Trace_Event(this, "removeEntryFromNeighborsDegreeMap()", "removing",
				"node", nodeName->getNodeName());
		myDegreeChanged();
	}
}

void TopologyManagerImpl::onSuccess(Neighbor_SPtr result,
		int ctx)
{
	Trace_Entry(this, "onSuccess()",
			"result", spdr::toString(result),
			"ctx", boost::lexical_cast<std::string>(ctx));

	bool disconnect = false;
	bool skip = false;
	String peer = result->getName();
	{
		boost::recursive_mutex::scoped_lock lock(topo_mutex);

		if (_state == TopologyManagerImpl::Closed)
		{
			Trace_Event(this, "onSuccess()",
					"returning immediately because closed");

			return;
		}

		// Successor
		if (ctx == RingCtx)
		{
			if (_setSuccessor)
			{
				if (!peer.compare(_setSuccessor->getNodeName()))
				{
					Trace_Event(this, "onSuccess()", "a new successor", "node",
							peer);

					if (_currentSuccessor)
					{
						Neighbor_SPtr currentSuccessor =
								_neighborTable->getNeighbor(_currentSuccessor);
						if (currentSuccessor)
						{
							Trace_Event(
									this,
									"onSuccess()",
									"Sending disconnect request to old successor",
									"node", currentSuccessor->getName());

							_recentlySentDisconnectRequest.push_back(
									_currentSuccessor);
							(*_topoMessage).writeH1Header(
									SCMessage::Type_Topo_Disconnect_Request);
							ByteBuffer& bb = *(*_topoMessage).getBuffer();
							bb.writeString(_currentSuccessor->getNodeName());
							bb.writeInt((int) TopologyManagerImpl::SUCCESSOR);
							bb.writeShort(_config.getRandomDegreeTarget() + 2
									- _neighborTable->size());
							(*_topoMessage).updateTotalLength();
							if (_config.isCRCMemTopoMsgEnabled())
							{
								(*_topoMessage).writeCRCchecksum();
							}

							if (!_neighborTable->sendToNeighbor(
									_currentSuccessor, _topoMessage))
							{
								Trace_Event(this, "onSuccess()",
										"failed to send disconnect message to",
										"node", currentSuccessor->getName());
							}
							else
							{
								Trace_Event(this, "onSuccess()",
										"sent a disconnect message to", "node",
										currentSuccessor->getName());
							}

						}
					}

					_currentSuccessor = _setSuccessor;
					_numFailedSetSuccessorAttempts = 0;
					if (_neighborTable->contains(_nodeIdCache.getOrCreate(peer)))
					{
						Trace_Event(this, "onSuccess()",
								"Neighbor table already contains node",
								"table", _neighborTable->toString());
						disconnect = true;
					}
					if (!disconnect)
					{
						//#ifndef RUM_Termination_Bug
						//TODO: createOnExisting
						if (result->isVirgin())
						{
							result = _commAdapter_SPtr->connectOnExisting(
									_nodeIdCache.getOrCreate(peer));
							if (!result)
							{
								return onFailure(peer, -1,
										"connectOnExisting failed", RingCtx);
							}
						}
						//#endif
						_neighborTable->addEntry(
								_nodeIdCache.getOrCreate(peer), result);

						_outgoingRandomConnectionRequests->removeEntry(
								_nodeIdCache.getOrCreate(peer));

						ostringstream oss;
						oss << _neighborTable->size();
						Trace_Event(this, "onSuccess()",
								"Connectivity event; added successor", "table",
								_neighborTable->toString(), "size", oss.str());

						Trace_Event(this, "onSuccess()",
								"created a new SCMessage: Type_Topo_Connect_Successor",
								"node", peer);

						(*_topoMessage).writeH1Header(SCMessage::Type_Topo_Connect_Successor);
						ByteBuffer& bb = *(*_topoMessage).getBuffer();
						bb.writeString(peer);
						bb.writeShort(_config.getRandomDegreeTarget() + 2
								- _neighborTable->size());
						(*_topoMessage).updateTotalLength();
						if (_config.isCRCMemTopoMsgEnabled())
						{
							(*_topoMessage).writeCRCchecksum();
						}

						if (!_neighborTable->sendToNeighbor(
								_nodeIdCache.getOrCreate(peer), _topoMessage))
						{
							Trace_Event(this, "onSuccess()",
									"couldn't send a message to", "node", peer);
							_neighborTable->removeEntry(
									_nodeIdCache.getOrCreate(peer));
							_memMgr_SPtr->reportSuspect(
									_nodeIdCache.getOrCreate(peer));
						}
						if (!_randomDisConnectTaskScheduled
								&& (_neighborTable->size()
										> _config.getRandomDegreeTarget() + 2)
										&& _state != TopologyManager::Closed)
						{
							_taskSchedule_SPtr->scheduleDelay(
									_randomDisConnectTask_SPtr,
									boost::posix_time::milliseconds(
											_config.getTopologyPeriodicTaskIntervalMillis()));
							_randomDisConnectTaskScheduled = true;
							Trace_Event(this, "onSuccess()",
									"scheduling a random disconnect task");
						}

						//TODO call newNeighbor???
						// newNeighbor called in the connect_ok processing
					}
					submitConnectivityEvent();
					skip = true;
				}
			}
		}

		if (ctx == DiscoveryCtx)
		{
			if (!skip && _discoveryMap->contains(_nodeIdCache.getOrCreate(peer)))
			{
				if (result->isVirgin())
				{
					result = _commAdapter_SPtr->connectOnExisting(
							_nodeIdCache.getOrCreate(peer));
					if (!result)
					{
						return onFailure(peer, -1, "connectOnExisting failed",
								DiscoveryCtx);
					}
				}

				// update the Neighbor entry in the _discoveryMap
				NodeIDImpl_SPtr discoveryNode = _nodeIdCache.getOrCreate(peer);
				_discoveryMap->addEntry(discoveryNode, result); //Will not replace the key
				// discovery
				// send discovery request
				if (_memMgr_SPtr)
				{
					Trace_Event(this, "onSuccess()",
							"sending a Type_Topo_Discovery_Request to", "node",
							peer);

					(*_topoMessage).writeH1Header(
							SCMessage::Type_Topo_Discovery_Request_TCP);
					ByteBuffer& bb = *(*_topoMessage).getBuffer();

					//TODO Nameless_TCP_Discovery: if $ANY prefix it is from bootstrap
					if (discoveryNode->isNameStartsWithANY())
					{
						bb.writeBoolean(true);
					}
					else
					{
						bb.writeBoolean(!_memMgr_SPtr->isFromHistorySet(
							discoveryNode)); //isBootstrap
					}
					_memMgr_SPtr->getDiscoveryViewPartial(_topoMessage, 0);
					(*_topoMessage).updateTotalLength();
					if (_config.isCRCMemTopoMsgEnabled())
					{
						(*_topoMessage).writeCRCchecksum();
					}

					if (!_discoveryMap->sendToNeighbor(
							discoveryNode, _topoMessage))
					{
						//there will never be a reply, so just remove it
						Trace_Event(this, "onSuccess()", "couldn't send a message, removing from DiscoveryMap",
								"target", peer);
						_discoveryMap->removeEntry(discoveryNode);
					}
				}
				skip = true;
			}
		}

		// random, or a previous call to successor that changed by the time the async connection completed
		if (ctx == RandomCtx || ctx == RingCtx)
		{
			if (!skip && _outgoingRandomConnectionRequests->contains(
					_nodeIdCache.getOrCreate(peer)))
			{
				if (_neighborTable->contains(_nodeIdCache.getOrCreate(peer)))
				{
					Trace_Event(this, "onSuccess()",
							"Neighbor table already contains node", "table",
							_neighborTable->toString());
					disconnect = true;
				}
				// random
				// send random connections request
				if (!disconnect)
				{
					//#ifndef RUM_Termination_Bug
					if (result->isVirgin())
					{
						// TODO: createOnExisting
						result = _commAdapter_SPtr->connectOnExisting(
								_nodeIdCache.getOrCreate(peer));
						if (!result)
						{
							return onFailure(peer, -1,
									"connectOnExisting failed", RingCtx);
						}
					}
					//#endif
					_neighborTable->addEntry(_nodeIdCache.getOrCreate(peer),
							result);
					//TODO call membership newNeighbor???
					ostringstream oss;
					oss << _neighborTable->size();
					Trace_Event(this, "onSuccess()",
							"Connectivity event; added random", "table",
							_neighborTable->toString(), "size", oss.str());

					// might have become the candidate successor after we sent a random discovery request
					bool successor = false;
					if (_setSuccessor)
					{
						if (!peer.compare(_setSuccessor->getNodeName()))
						{
							successor = true;
						}
					}

					if (!successor)
					{
						Trace_Event(this, "onSuccess()",
								"sending a Type_Topo_Connect_Random_Request to",
								"node", peer);
					}
					else
					{
						Trace_Event(this, "onSuccess()",
								"sending a Type_Topo_Connect_Successor to",
								"node", peer);
					}

					(*_topoMessage).writeH1Header(
							successor ? SCMessage::Type_Topo_Connect_Successor
									: SCMessage::Type_Topo_Connect_Random_Request);
					ByteBuffer& bb = *(*_topoMessage).getBuffer();
					bb.writeString(peer);
					bb.writeShort(_config.getRandomDegreeTarget() + 2
							- _neighborTable->size());
					(*_topoMessage).updateTotalLength();
					if (_config.isCRCMemTopoMsgEnabled())
					{
						(*_topoMessage).writeCRCchecksum();
					}

					if (!_neighborTable->sendToNeighbor(
							_nodeIdCache.getOrCreate(peer), _topoMessage))
					{
						Trace_Event(this, "onSuccess()",
								"couldn't send a message to", "node", peer);
						// TODO: decide what to do
					}

					_outgoingRandomConnectionRequests->removeEntry(
							_nodeIdCache.getOrCreate(peer));
					if (!_randomDisConnectTaskScheduled
							&& (_neighborTable->size()
									> _config.getRandomDegreeTarget() + 2)
									&& _state != TopologyManager::Closed)
					{
						_taskSchedule_SPtr->scheduleDelay(
								_randomDisConnectTask_SPtr,
								boost::posix_time::milliseconds(
										_config.getTopologyPeriodicTaskIntervalMillis()));
						_randomDisConnectTaskScheduled = true;
						Trace_Event(this, "onSuccess()",
								"scheduling a random disconnect task");
					}
				}
				submitConnectivityEvent();
				skip = true;
			}
		}

		if (!skip && ctx == StructuredCtx)
		{
			Trace_Event(this, "onSuccess()",
					"Received a structured random onSuccess");
			if (!_outgoingStructuredNeighborTable->contains(
					_nodeIdCache.getOrCreate(peer)))
			{
				Trace_Event(
						this,
						"onSuccess()",
						"Neighbor cannot be found in outgoing structured table",
						"table", _outgoingStructuredNeighborTable->toString());
			}
			else
			{
				// send structured connections request

				//#ifndef RUM_Termination_Bug
				if (result->isVirgin())
				{
					result = _commAdapter_SPtr->connectOnExisting(
							_nodeIdCache.getOrCreate(peer));
					if (!result)
					{
						return onFailure(peer, -1, "connectOnExisting failed",
								StructuredCtx);
					}
				}
				//#endif
				if (!_outgoingStructuredNeighborTable->getNeighbor(
						_nodeIdCache.getOrCreate(peer)))
				{
					_outgoingStructuredNeighborTable->addEntry(
							_nodeIdCache.getOrCreate(peer), result, -1);

					(*_topoMessage).writeH1Header(
							SCMessage::Type_Topo_Connect_Structured_Request);
					ByteBuffer& bb = *(*_topoMessage).getBuffer();
					bb.writeString(peer);
					(*_topoMessage).updateTotalLength();
					if (_config.isCRCMemTopoMsgEnabled())
					{
						(*_topoMessage).writeCRCchecksum();
					}

					if (!_outgoingStructuredNeighborTable->sendToNeighbor(
							_nodeIdCache.getOrCreate(peer), _topoMessage))
					{
						Trace_Event(this, "onSuccess()",
								"couldn't send a message to", "node", peer);
						// TODO: decide what to do
					}
					submitConnectivityEvent();
				}
				else
				{
					disconnect = true;
				}
				skip = true;
			}
		}
		else if (!skip)// didn't find entry anywhere //TODO why else if? and not if?
		{
			Trace_Event(
					this,
					"onSuccess()",
					"failed to find entry in _discoveryMap and _outgoingRandomConnectionRequests. Closing the connection to",
					"node", peer);
			disconnect = true;
		}
	}

	if (disconnect)
	{
		Trace_Event(this, "onSuccess()", "Closing the connection to", "node",
				peer);
		_outgoingRandomConnectionRequests->removeEntry(
				_nodeIdCache.getOrCreate(peer));
		_commAdapter_SPtr->disconnect(result);
	}

	Trace_Exit(this, "onSuccess()");
}

void TopologyManagerImpl::onFailure(String failedTargetName, int rc,
		String message, int ctx)
{
	Trace_Entry(this, "onFailure()");
	boost::recursive_mutex::scoped_lock lock(topo_mutex);
	if (_state == TopologyManagerImpl::Closed)
	{
		Trace_Event(this, "onFailure()", "returning immediately because closed");

		return;
	}
	ostringstream oss;
	oss << "ctx: " << ctx << "; rc: " << rc;
	Trace_Entry(this, "onFailure()", "failed to connect to node",
			failedTargetName, "details", oss.str(), "message", message);

	if (ctx == DiscoveryCtx)
	{
		_discoveryMap->removeEntry(_nodeIdCache.getOrCreate(failedTargetName));
	}
	else
	{
		_outgoingRandomConnectionRequests->removeEntry(
				_nodeIdCache.getOrCreate(failedTargetName));
		//TODO remove from outgoingStructured?
	}

	if (_setSuccessor)
	{
		if (!failedTargetName.compare(_setSuccessor->getNodeName()))
		{
			Trace_Event(this, "onFailure()", "a new successor", "node",
					failedTargetName);
			if (++_numFailedSetSuccessorAttempts
					> numFailedSuccessorAttemptAllowed)
			{
				//prevent report-suspect from calling set-successor in this context
				_memMgr_SPtr->reportSuspect(_nodeIdCache.getOrCreate(
						failedTargetName));
				_numFailedSetSuccessorAttempts = 0;

				//this caused a NPE

				//_setSuccessor = NodeIDImpl_SPtr ();
			}
			else
			{
				if (!_changeSuccessorTaskScheduled)
				{
					_taskSchedule_SPtr->scheduleDelay(
							_changeSuccessorTask_SPtr,
							boost::posix_time::milliseconds(0));
					_changeSuccessorTaskScheduled = true;
					Trace_Event(this, "onFailure()",
							"scheduling a change successor task");
				}
			}
		}
	}
	Trace_Exit(this, "onFailure()");
}

void TopologyManagerImpl::startFrequentDiscovery()
{
	Trace_Entry(this, "startFrequentDiscovery()");
	if (_state != TopologyManager::Closed)
	{
		// Reschedule stop task
		AbstractTask_SPtr oldStopTask =
				_stopFrequentDiscoveryTask_SPtr;
		_stopFrequentDiscoveryTask_SPtr = AbstractTask_SPtr (
				new StopInitialDiscoveryPeriodTask(_coreInterface));
		oldStopTask->cancel();
		_taskSchedule_SPtr->scheduleDelay(_stopFrequentDiscoveryTask_SPtr,
				_frequentDiscoveryDuration);

		// Set state
		_frequentDiscovery = true;
		_state = TopologyManager::Discovery;

		//Reschedule discovery task
		if ((_discoveryTask_SPtr->scheduledExecutionTime()
				- boost::posix_time::microsec_clock::universal_time())
				> _frequentDiscoveryPeriod)
		{
			AbstractTask_SPtr oldDiscoveryTask =
					_discoveryTask_SPtr;
			_discoveryTask_SPtr = AbstractTask_SPtr (
					new DiscoveryPeriodicTask(_coreInterface));
			_taskSchedule_SPtr->scheduleDelay(_discoveryTask_SPtr,
					_frequentDiscoveryPeriod);
			oldDiscoveryTask->cancel();
		}
	}
	else
	{
		Trace_Event(this, "startFrequentDiscovery()",
				"Can't start frequent discovery - topology already closed");
	}
	Trace_Exit(this, "startFrequentDiscovery()");
}

void TopologyManagerImpl::stopFrequentDiscovery()
{
	Trace_Entry(this, "stopFrequentDiscovery()");
	if (_state != TopologyManager::Closed)
	{
		// No need to cancel stop task, it can run.
		// No need to schedule discovery task at new time - is should be scheduled already as frequent discovery
		// and will reschedule itself to normal discovery during next run.
		_frequentDiscovery = false;
		_state = TopologyManager::Normal;
	}
	else
	{
		Trace_Event(this, "stopFrequentDiscovery()",
				"Can't stop frequent discovery - topology already closed");
	}
	Trace_Exit(this, "stopFrequentDiscovery()");
}

bool TopologyManagerImpl::isInFrequentDiscovery()
{
	return _frequentDiscovery;
}

void TopologyManagerImpl::submitConnectivityEvent()
{
	Trace_Entry(this, "submitConnectivityEvent");

	short numRingNeighbors;
	short numRandomNeighbors;
	short numOutgoingStructuredNeighbors;
	short numIncomingStructuredNeighbors;

	numRingNeighbors = (_currentSuccessor && _neighborTable->contains(
			_currentSuccessor)) ? 1 : 0;

	Trace_Event(this, "submitConnectivityEvent", "details: Ring+Random", "successor",
			toString(_currentSuccessor), "predecessor", toString(
					_currentPredecessor), "neighbor table",
			_neighborTable->toString());

	Trace_Event(this, "submitConnectivityEvent", "details: Structured",
			"outgoingStructured", _outgoingStructuredNeighborTable->toString(),
			"incomingStructured", _incomingStructuredNeighborTable->toString());

	if (_currentPredecessor)
	{
		if (!_currentSuccessor)
		{
			numRingNeighbors
					+= _neighborTable->contains(_currentPredecessor) ? 1 : 0;
		}
		else if (*_currentSuccessor != *_currentPredecessor)
		{
			numRingNeighbors
					+= _neighborTable->contains(_currentPredecessor) ? 1 : 0;
		}
	}
	numRandomNeighbors = _neighborTable->size() - numRingNeighbors;
	numOutgoingStructuredNeighbors = _outgoingStructuredNeighborTable->size();
	numIncomingStructuredNeighbors = _incomingStructuredNeighborTable->size();

	if (_numRandomNeighborsReported != numRandomNeighbors
			|| _numRingNeighborsReported != numRingNeighbors
			|| _numOutgoingStructuredNeighborsReported
					!= numOutgoingStructuredNeighbors
			|| _numIncomingStructuredNeighborsReported
					!= numIncomingStructuredNeighbors)
	{

		bool withinRange = (numRandomNeighbors >= 0) && (numRandomNeighbors
				<= _config.getRandomDegreeTarget()
						+ _config.getRandomDegreeMargin());
		if (withinRange)
		{
			withinRange = numOutgoingStructuredNeighbors
					<= _config.getStructDegreeTarget();
		}
		if (withinRange)
		{
			withinRange = numIncomingStructuredNeighbors <= 2
					* _config.getStructDegreeTarget();
		}
		if (!withinRange)
		{
			ostringstream oss;
			oss << "Warning: outside range; numRandom: " << numRandomNeighbors
					<< "; numRing: " << numRingNeighbors
					<< "; numOutgoingStruct: "
					<< numOutgoingStructuredNeighbors
					<< "; numIncomingStruct: "
					<< numIncomingStructuredNeighbors;
			Trace_Event(this, "submitConnectivityEvent", oss.str());
		}

		_coreInterface.submitExternalEvent(event::SpiderCastEvent_SPtr(
				new event::ConnectivityEvent(numRandomNeighbors,
						numRingNeighbors, _neighborTable->getNeighborsStr(),
						numOutgoingStructuredNeighbors,
						_outgoingStructuredNeighborTable->toString(),
						numIncomingStructuredNeighbors,
						_incomingStructuredNeighborTable->toString() )));

		_numRingNeighborsReported = numRingNeighbors;
		_numRandomNeighborsReported = numRandomNeighbors;
		_numOutgoingStructuredNeighborsReported
				= numOutgoingStructuredNeighbors;
		_numIncomingStructuredNeighborsReported
				= numIncomingStructuredNeighbors;
	}
	else
	{
		Trace_Event(this, "submitConnectivityEvent",
				"skipping since the numbers have not changed");
	}
	Trace_Exit(this, "submitConnectivityEvent");
}
} //namespace
