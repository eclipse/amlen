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
 * HierarchySupervisor.h
 *
 *  Created on: 15/03/2010
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisor.h,v $
 * Revision 1.3  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:15 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.20  2015/01/26 12:38:14 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.19  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.18  2012/07/17 09:19:04 
 * add pub/sub bridges
 *
 * Revision 1.17  2012/02/27 13:40:44 
 * typedef AbstractTask_SPtr, forward declare in CoreInterface
 *
 * Revision 1.16  2011/08/07 12:13:35 
 * Publish attributes only if changed since last time around
 *
 * Revision 1.15  2011/03/03 06:56:32 
 * Add support for foreign zone membership requests timeout
 *
 * Revision 1.14  2011/02/28 18:12:40 
 * Implement foreign zone remote requests
 *
 * Revision 1.13  2011/02/20 13:52:36 
 * FZM request timeout, No-attributes error code
 *
 * Revision 1.12  2011/02/20 12:01:01 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.11  2011/02/16 20:08:34 
 * foreign zone membership task - take 1
 *
 * Revision 1.10  2011/02/16 08:45:02 
 * Integrate the hierarchy view keeper
 *
 * Revision 1.9  2011/02/14 13:52:33 
 * Add the zone census task
 *
 * Revision 1.8  2011/02/13 13:20:54 
 * Foreign zone membership
 *
 * Revision 1.7  2011/02/07 14:52:33 
 * Add setActiveDelegatesTask, and processing of additional messages from the delegate
 *
 * Revision 1.6  2011/01/27 11:43:56 
 * Add a ByteBuffer to be used by attribute values
 *
 * Revision 1.5  2011/01/25 13:02:08 
 * *** empty log message ***
 *
 * Revision 1.4  2011/01/17 13:52:31 
 * Make data staructures deal with guarded zones seperately
 *
 * Revision 1.3  2011/01/13 10:17:54 
 * Initial mostly implemented version
 *
 *
 */

#ifndef HIERARCHYSUPERVISOR_H_
#define HIERARCHYSUPERVISOR_H_

#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>

#include "SpiderCastConfigImpl.h"
#include "NodeIDCache.h"
#include "CoreInterface.h"
#include "HierarchyManager.h"
#include "HierarchySupervisorTaskInterface.h"
#include "HierarchySupervisorSetActiveDelegatesTask.h"
#include "HierarchySupervisorZoneCensusTask.h"
#include "HierarchySupervisorForeignZoneMembershipTask.h"
#include "HierarchySupervisorForeignZoneMembershipTOTask.h"
#include "SupervisorNeighborTable.h"
#include "RoutingManager.h"

#include "MembershipEvent.h"
#include "HierarchyViewKeeper.h"

namespace spdr
{

class HierarchySupervisor: boost::noncopyable,
		public HierarchySupervisorTaskInterface,
		public ScTraceContext
{
private:
	static ScTraceComponent* _tc;

	static const String supervisorState_AttributeKey; // ".hier.sup.state"
	static const String supervisorGuardedBaseZone_AttributeKeyPrefix; // ".hier.sup.gbz:" {base-zone-name} appended

public:
	HierarchySupervisor(const String& instID,
			const SpiderCastConfigImpl& config,
			NodeIDCache& nodeIDCache,
			CoreInterface& coreInterface,
			boost::shared_ptr<HierarchyViewKeeper> viewKeeper);

	virtual ~HierarchySupervisor();

	void init();

	void terminate();

	void destroyCrossRefs();

	void processIncomingHierarchyMessage(SCMessage_SPtr inHierarchyMsg);

	void processIncomingCommEventMsg(SCMessage_SPtr incomingCommEvent);

	void sendLeaveMsg();

	/*
	 * Threading: App. thread, internal threads
	 */
	int64_t queueForeignZoneMembershipRequest(
			BusName_SPtr zoneBusName, bool includeAttributes, int timeoutMillis);

	/*
	 * Threading: App. thread, internal threads
	 */
	int64_t queueZoneCensusRequest();

	/*
	 * @see spdr::HierarchySupervisorTaskInterface
	 */
	void zoneCensusTask();

	/*
	 * @see spdr::HierarchySupervisorTaskInterface
	 */
	void setActiveDelegatesTask();

	/*
	 * @see spdr::HierarchySupervisorTaskInterface
	 */
	void foreignZoneMembershipTask(int64_t reqId, BusName_SPtr zoneBusName, bool includeAttributes);

	/*
	 * @see spdr::HierarchySupervisorTaskInterface
	 */
	void foreignZoneMembershipTOTask(int64_t reqId, String zoneBusName);


private:
	const String& _instID;
	const SpiderCastConfigImpl& _config;
	NodeIDCache& _nodeIDCache;
	CoreInterface& _coreInterface;
	boost::shared_ptr<HierarchyViewKeeper> _hierarchyViewKeeper_SPtr;

	mutable boost::recursive_mutex _mutex;
	volatile bool _closed;

	TaskSchedule_SPtr _taskSchedule_SPtr;
	AbstractTask_SPtr _setActiveDelegatesTask_SPtr;
	AbstractTask_SPtr _zoneCensusTask_SPtr;
	CommAdapter_SPtr _commAdapter_SPtr;
	MembershipManager_SPtr _memManager_SPtr;


	SCMessage_SPtr _outgoingHierMessage;
	ByteBuffer_SPtr _attrValBuffer;

	typedef boost::unordered_map<String, SupervisorNeighborTable_SPtr > Zone2DelegateTable_Map;
	Zone2DelegateTable_Map _delegatesTablesMap;

	boost::unordered_map<NodeID_SPtr, std::vector<boost::tuples::tuple<int64_t, String, bool> > > _remoteForeignZoneMemebershipRequestsMap;

	boost::unordered_map<int64_t, pair<String, AbstractTask_SPtr > > _pendingRemoteForeignZoneMemebershipRequestsMap;

	boost::unordered_map<String, pair<int32_t, int32_t> > _publishedZoneStats; // zone name -> numDelegates, numMember

	bool _setActiveDelegatesTaskScheduled;
	bool _zoneCensusTaskScheduled;

	int64_t _requestId;
	std::vector<int64_t> _pendingZoneCensusRequestIds;

	int32_t _publishedNumBaseZones;

	bool isClosed() const;

	void addDelegate(Neighbor_SPtr delegate, BusName_SPtr busName);
	void removeDelegate(NodeIDImpl_SPtr delegate, BusName_SPtr busName);
	void setAttributes(BusName_SPtr busName);

	void processIncomingConnectRequestMsg(SCMessage_SPtr incomingHierMsg);
	void processIncomingDisconnectRequestMsg(SCMessage_SPtr incomingHierMsg);

	void processIncomingConnectReplyMsg(SCMessage_SPtr incomingHierMsg);
	void processIncomingDisconnectReplyMsg(SCMessage_SPtr incomingHierMsg);

	void processIncomingNodeLeaveMsg(SCMessage_SPtr incomingHierMsg);

	void processIncomingViewUpdate(SCMessage_SPtr incomingHierMsg);
	void processIncomingStartMembershipPushReplyMsg(SCMessage_SPtr incomingHierMsg);

	void processIncomingForeignZoneMembershipRequestMsg(SCMessage_SPtr incomingHierMsg);
	void processIncomingForeignZoneMembershipReplyMsg(SCMessage_SPtr incomingHierMsg);

	void processOnBreakEvent(SCMessage_SPtr incomingCommEvent);
	void processOnSuccessEvent(SCMessage_SPtr incomingCommEvent);

	String delegatesTablesMapToString();

	void scheduleSetActiveDelegatesTask(int delayMillis);
	void scheduleZoneCensusTask(int delayMillis);
	void scheduleforeignZoneMembershipTask(int delayMillis, int64_t reqId, BusName_SPtr zoneBusName, bool includeAttributes);
	void scheduleforeignZoneMembershipTOTask(int delayMillis, int64_t reqId, String zoneBusName);

	void sendActivateMsg(String targetBus, NodeIDImpl_SPtr targetName);

	void obtainRemoteForeignZoneMembership(NodeIDImpl_SPtr activeSupervisor, int64_t reqId, BusName_SPtr zoneBusName, bool includeAttributes);

};

}

#endif /* HIERARCHYSUPERVISOR_H_ */
