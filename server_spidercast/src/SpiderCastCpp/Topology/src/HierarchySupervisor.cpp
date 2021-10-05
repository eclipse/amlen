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
 * HierarchySupervisor.cpp
 *
 *  Created on: 15/03/2010
 *      Author:
 *
 * Version     : $Revision: 1.6 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisor.cpp,v $
 * Revision 1.6  2016/11/27 11:28:33 
 * macros fro trace
 *
 * Revision 1.5  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.4.2.1  2016/01/21 14:52:47 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.4  2015/11/18 08:36:58 
 * Update copyright headers
 *
 * Revision 1.3  2015/08/27 10:53:53 
 * change RUM log level dynamically
 *
 * Revision 1.2  2015/08/06 06:59:15 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:13 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.32  2012/10/25 10:11:12 
 * Added copyright and proprietary notice
 *
 * Revision 1.31  2012/07/30 14:05:35 
 * add PubSub_Bridge control messages and pass them to bridges
 *
 * Revision 1.30  2012/07/17 09:19:05 
 * add pub/sub bridges
 *
 * Revision 1.29  2012/02/27 13:41:46 
 * typedef AbstractTask_SPtr
 *
 * Revision 1.28  2012/02/19 09:07:11 
 * Add a "routbale" field to the neighbor tables
 *
 * Revision 1.27  2011/08/07 12:13:58 
 * Publish attributes only if changed since last time around
 *
 * Revision 1.26  2011/07/10 09:24:18 
 * Create an RUM Tx on a seperate thread (not thr RUM thread)
 *
 * Revision 1.25  2011/05/02 12:10:00 
 * Update the way we compute attribute size
 *
 * Revision 1.24  2011/05/01 08:38:59 
 * Move connection context from TopologyManager to Definitions.h
 *
 * Revision 1.23  2011/03/31 14:00:05 
 * Ensure that view update messages are accepted only from the active delegate
 *
 * Revision 1.22  2011/03/24 08:41:53 
 * write CRC after on each message
 *
 * Revision 1.21  2011/03/03 06:56:31 
 * Add support for foreign zone membership requests timeout
 *
 * Revision 1.20  2011/02/28 18:13:42 
 * Implement foreign zone remote requests
 *
 * Revision 1.19  2011/02/27 12:20:20 
 * Refactoring, header file organization
 *
 * Revision 1.18  2011/02/21 07:55:49 
 * Add includeAttributes to the supervisor attributes
 *
 * Revision 1.16  2011/02/20 13:12:29 
 * Add config parameters: includeAttributes and agregation interval for membership updates from delegate to supervisor
 *
 * Revision 1.15  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.14  2011/02/16 20:08:34 
 * foreign zone membership task - take 1
 *
 * Revision 1.13  2011/02/16 08:46:23 
 * Integrate the hierarchy view keeper; and use it for the census task
 *
 * Revision 1.12  2011/02/14 15:00:50 
 * bug fix - schedule tasks
 *
 * Revision 1.11  2011/02/14 13:54:46 
 * Add the zone census task
 *
 * Revision 1.10  2011/02/13 13:20:54 
 * Foreign zone membership
 *
 * Revision 1.9  2011/02/10 13:28:44 
 * Added supervisor view keeper issues
 *
 * Revision 1.8  2011/02/07 14:57:37 
 * Add setActiveDelegatesTask, and processing of additional messages from the delegate
 *
 * Revision 1.7  2011/02/01 13:18:40 
 * *** empty log message ***
 *
 * Revision 1.6  2011/01/27 11:44:27 
 * Add a ByteBuffer to be used by attribute values
 *
 * Revision 1.5  2011/01/25 13:02:08 
 * *** empty log message ***
 *
 * Revision 1.4  2011/01/17 13:53:18 
 * Make data staructures deal with guarded zones seperately
 *
 * Revision 1.3  2011/01/13 10:19:42 
 * Initial mostly implemented version
 *
 *
 */

#include <sstream>

#include "HierarchySupervisor.h"

using namespace std;

namespace spdr
{

ScTraceComponent* HierarchySupervisor::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy, "HierarchySupervisor",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchySupervisor::HierarchySupervisor(const String& instID,
		const SpiderCastConfigImpl& config, NodeIDCache& nodeIDCache,
		CoreInterface& coreInterface,
		boost::shared_ptr<HierarchyViewKeeper> viewKeeper) :
	ScTraceContext(_tc, instID, config.getMyNodeID()->getNodeName()), _instID(
			instID), _config(config), _nodeIDCache(nodeIDCache),
			_coreInterface(coreInterface),
			_hierarchyViewKeeper_SPtr(viewKeeper), _closed(false),
			_taskSchedule_SPtr(), _setActiveDelegatesTask_SPtr(),
			_zoneCensusTask_SPtr(), _commAdapter_SPtr(), _memManager_SPtr(),
			_outgoingHierMessage(), _attrValBuffer(), _delegatesTablesMap(),
			_remoteForeignZoneMemebershipRequestsMap(),
			_pendingRemoteForeignZoneMemebershipRequestsMap(),
			_publishedZoneStats(), _setActiveDelegatesTaskScheduled(false),
			_zoneCensusTaskScheduled(false), _requestId(0),
			_pendingZoneCensusRequestIds(), _publishedNumBaseZones(-1)
{
	Trace_Entry(this, "HierarchySupervisor()");

	_outgoingHierMessage = SCMessage_SPtr(new SCMessage);
	_outgoingHierMessage->setBuffer(ByteBuffer::createByteBuffer(128));
	_attrValBuffer = ByteBuffer::createByteBuffer(8);

}

HierarchySupervisor::~HierarchySupervisor()
{
	Trace_Entry(this, "~HierarchySupervisor()");
}

void HierarchySupervisor::init()
{
	Trace_Entry(this, "init()");

	_taskSchedule_SPtr = _coreInterface.getTopoMemTaskSchedule();
	_commAdapter_SPtr = _coreInterface.getCommAdapter();
	_memManager_SPtr = _coreInterface.getMembershipManager();
	_setActiveDelegatesTask_SPtr = AbstractTask_SPtr (
			new HierarchySupervisorSetActiveDelegatesTask(_instID, *this));
	_zoneCensusTask_SPtr = AbstractTask_SPtr (
			new HierarchySupervisorZoneCensusTask(_instID, *this));
}

void HierarchySupervisor::destroyCrossRefs()
{
	Trace_Entry(this, "destroyCrossRefs()");
	_taskSchedule_SPtr.reset();
	_commAdapter_SPtr.reset();
	_memManager_SPtr.reset();
	_setActiveDelegatesTask_SPtr.reset();
	_zoneCensusTask_SPtr.reset();
}

void HierarchySupervisor::processIncomingHierarchyMessage(
		SCMessage_SPtr inHierarchyMsg)
{

	Trace_Entry(this, "processIncomingHierarchyMessage");

	if (isClosed())
	{
		Trace_Exit(this, "processIncomingHierarchyMessage", "closed");
		return;
	}

	SCMessage::H1Header h1 = inHierarchyMsg->readH1Header();
	NodeIDImpl_SPtr sender = inHierarchyMsg->getSender();
	BusName_SPtr sender_bus = inHierarchyMsg->getBusName();

	if (h1.get<1> () != SCMessage::Type_Hier_SupOp_Request_ViewUpdate &&
			h1.get<1> () != SCMessage::Type_Hier_SupOp_Request_ForeignZoneMembership &&
			h1.get<1> () != SCMessage::Type_Hier_SupOp_Reply_ForeignZoneMemberships)
	{
		ByteBuffer_SPtr buffer = inHierarchyMsg->getBuffer();
		String senderName = buffer->readString();
		String targetName = buffer->readString();
		_coreInterface.getHierarchyManager()->verifyIncomingMessageAddressing(
				senderName, sender->getNodeName(), targetName);
	}

	Trace_Event(this, "processIncomingHierarchyMessage", "msg",
			(inHierarchyMsg ? inHierarchyMsg->toString() : "null"));

	switch (h1.get<1> ())
	//message type
	{

	case SCMessage::Type_Hier_Connect_Request:
	{
		processIncomingConnectRequestMsg(inHierarchyMsg);
	}

		break;

	case SCMessage::Type_Hier_Disconnect_Request:
	{
		processIncomingDisconnectRequestMsg(inHierarchyMsg);
	}
		break;

	case SCMessage::Type_Hier_Disconnect_Reply:
	{
		processIncomingDisconnectReplyMsg(inHierarchyMsg);

	}
		break;

	case SCMessage::Type_Hier_Leave:
	{
		processIncomingNodeLeaveMsg(inHierarchyMsg);
	}
		break;

	case SCMessage::Type_Hier_SupOp_Request_ViewUpdate:
	{
		processIncomingViewUpdate(inHierarchyMsg);
	}
		break;

	case SCMessage::Type_Hier_DelOp_Reply_StartMembershipPush:
	{
		processIncomingStartMembershipPushReplyMsg(inHierarchyMsg);
	}
		break;

	case SCMessage::Type_Hier_SupOp_Request_ForeignZoneMembership:
	{
		processIncomingForeignZoneMembershipRequestMsg(inHierarchyMsg);
	}
		break;

	case SCMessage::Type_Hier_SupOp_Reply_ForeignZoneMemberships:
	{
		processIncomingForeignZoneMembershipReplyMsg(inHierarchyMsg);
	}
		break;

	case SCMessage::Type_Hier_PubSubBridge_BaseZoneInterest:
	{
		_coreInterface.getRoutingManager()->processIncomingSupervisorPubSubBridgeControlMessage(
				inHierarchyMsg);
		//pass to S-Bridge
	}
	break;

	default:
	{
		String what("Unexpected message type: ");
		what.append(inHierarchyMsg ? inHierarchyMsg->toString() : "null");
		throw SpiderCastRuntimeError(what);
	}

	}//switch

	Trace_Exit(this, "processIncomingHierarchyMessage");

}

void HierarchySupervisor::processIncomingCommEventMsg(
		SCMessage_SPtr incomingCommEvent)
{

	Trace_Entry(this, "processIncomingCommEventMsg()");

	if (isClosed())
	{
		Trace_Exit(this, "processIncomingCommEventMsg()", "closed");
		return;
	}

	boost::shared_ptr<CommEventInfo> inEvent =
			incomingCommEvent->getCommEventInfo();
	//NodeIDImpl_SPtr peerName = incomingCommEvent->getSender();

	Trace_Event(this, "processIncomingCommEventMsg", "event",
			inEvent->toString());

	switch (inEvent->getType())
	{

	case CommEventInfo::New_Source:
	{
		Trace_Event(this, "processIncomingCommEventMsg()",
				"received a New Source Event msg");
	}
		break;

	case CommEventInfo::On_Break:
	{

		processOnBreakEvent(incomingCommEvent);
	}
		break;

	case CommEventInfo::On_Connection_Success:
	{
		processOnSuccessEvent(incomingCommEvent);
	}
		break;

	case CommEventInfo::On_Connection_Failure:
	{
		Trace_Event(this, "processIncomingCommEventMsg",
				"Warning: On_Connection_Failure. Should not happen at the supervisor");
	}
		break;

	default:
	{
		Trace_Event(this, "processIncomingCommEventMsg()",
				"Unexpected event type");
		String what("Unexpected event type");
		throw SpiderCastRuntimeError(what);
	}

	} //switch

	Trace_Exit(this, "processIncomingCommEventMsg()");

}

void HierarchySupervisor::processOnSuccessEvent(
		SCMessage_SPtr incomingCommEvent)
{
	//TODO: who gets rid of this connection?
	Trace_Entry(this, "processOnSuccessEvent()");

	BusName_SPtr busName = incomingCommEvent->getBusName();
	boost::shared_ptr<CommEventInfo> inEvent =
			incomingCommEvent->getCommEventInfo();
	NodeIDImpl_SPtr peerName = incomingCommEvent->getSender();

	boost::unordered_map<NodeID_SPtr, std::vector<boost::tuples::tuple<int64_t,
			String, bool> > >::iterator iter =
			_remoteForeignZoneMemebershipRequestsMap.find(peerName);
	if (iter != _remoteForeignZoneMemebershipRequestsMap.end())
	{
		std::vector<boost::tuples::tuple<int64_t, String, bool> >
				pendingRequests = iter->second;
		for (int counter = 0; counter < (int) pendingRequests.size(); counter++)
		{
			boost::tuples::tuple<int64_t, String, bool> entry =
					pendingRequests.back();
			int64_t reqId = entry.get<0> ();
			String zoneName = entry.get<1> ();
			bool includeAttrs = entry.get<2> ();

			Trace_Event(this, "processOnSuccessEvent",
					"about to send a request", "supervisor",
					peerName->getNodeName(), "requested zone", zoneName);

			(*_outgoingHierMessage).writeH1Header(
					SCMessage::Type_Hier_SupOp_Request_ForeignZoneMembership);
			ByteBuffer& bb = *(*_outgoingHierMessage).getBuffer();
			bb.writeNodeID(_config.getMyNodeID());
			bb.writeLong(reqId);
			bb.writeString(zoneName);
			bb.writeBoolean(includeAttrs);
			(*_outgoingHierMessage).updateTotalLength();
			if (_config.isCRCMemTopoMsgEnabled())
			{
				(*_outgoingHierMessage).writeCRCchecksum();
			}

			Neighbor_SPtr neighbor = (*inEvent).getNeighbor();
//#ifndef RUM_Termination_Bug
			if (neighbor->isVirgin())
			{
				neighbor = _commAdapter_SPtr->connectOnExisting(peerName);
				if (!neighbor)
				{
					Trace_Event(this, "processOnSuccessEvent()",
							"couldn't connect to", "node",
							peerName->getNodeName());
					// TODO: figure out what to do
					return;

				}
			}
//#endif
			if (neighbor->sendMessage(_outgoingHierMessage) != 0)
			{
				{
					Trace_Event(this, "processOnSuccessEvent()",
							"couldn't send a message to", "node",
							peerName->getNodeName());
					// TODO: figure out what to do
				}
			}

			scheduleforeignZoneMembershipTOTask(
					_config.getHierarchyForeignZoneMemberhipTimeOut(), reqId,
					zoneName);

			pendingRequests.pop_back();
		}
		_remoteForeignZoneMemebershipRequestsMap.erase(peerName);
	}

	Trace_Exit(this, "processOnSuccessEvent()");
}

void HierarchySupervisor::processOnBreakEvent(SCMessage_SPtr incomingCommEvent)
{
	Trace_Entry(this, "processOnBreakEvent()");

	BusName_SPtr busName = incomingCommEvent->getBusName();
	boost::shared_ptr<CommEventInfo> inEvent =
			incomingCommEvent->getCommEventInfo();
	NodeIDImpl_SPtr peerName = incomingCommEvent->getSender();

	if (busName)
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		Zone2DelegateTable_Map::iterator delegatesTable =
				_delegatesTablesMap.find(busName->toString());
		if (delegatesTable != _delegatesTablesMap.end())
		{
			bool removed_delegate = delegatesTable->second->removeEntry(
					peerName);
			bool hit = delegatesTable->second->setInactiveDelegate(peerName);
			if (hit)
			{
				//update S-Bridge
				_coreInterface.getRoutingManager()->supervisorPubSubBridge_remove_active(
						busName, peerName);
			}

			if (delegatesTable->second->size() == 0)
			{
				_delegatesTablesMap.erase(busName->toString());
				Trace_Event(this, "processIncomingCommEventMsg",
						"removing zone from delegates table", "zone",
						busName->toString());
			}

			if (removed_delegate)
			{
				removeDelegate(peerName, busName);
			}
			else
			{
				//can happen after a leave or disconnect
				Trace_Event(
						this,
						"processIncomingCommEventMsg()",
						"On_Break, couldn't find node in delegates table. Nothing to do",
						"peer", NodeIDImpl::stringValueOf(peerName));
			}
		}
		else
		{
			Trace_Event(
					this,
					"processIncomingCommEventMsg()",
					"On_Break, couldn't find zone in delegates table. Nothing to do",
					"zone", spdr::toString(busName));
		}
	}
	else
	{
		Trace_Event(
				this,
				"processIncomingCommEventMsg()",
				"On_Break, couldn't find zone cause bus-name is empty. Nothing to do",
				"eventInfo", inEvent->toString());
	}

	Trace_Exit(this, "processOnBreakEvent()");

}

void HierarchySupervisor::terminate()
{
	Trace_Entry(this, "terminate()");
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		_closed = true;
	}
}

//TODO use this method to send the resulting event, using MemTopoThread
//_MemManager_SPtr->notifyZoneCensus(...);
void HierarchySupervisor::zoneCensusTask()
{
	Trace_Entry(this, "zoneCensusTask()");

	_zoneCensusTaskScheduled = false;

	int totalNodeCount = 0;
	std::vector<int64_t> copyPendingZoneCensusRequestIds;

	event::ZoneCensus_SPtr zcInfo = event::ZoneCensus_SPtr(
			new event::ZoneCensus());

	int numLocalNodes = _memManager_SPtr->getViewSize();
	zcInfo->insert(std::make_pair(_config.getBusName(), numLocalNodes));
	totalNodeCount += numLocalNodes;

	Trace_Event<int> (this, "zoneCensusTask", "numLocalNodes", "num",
			numLocalNodes);

	const HierarchyViewKeeper::SupervisorReportMap& map =
			_hierarchyViewKeeper_SPtr->getSupervisorReportMap();
	for (HierarchyViewKeeper::SupervisorReportMap::const_iterator zone =
			map.begin(); zone != map.end(); zone++)
	{
		String zone_name = zone->first;
		HierarchyViewKeeper::SupervisorZoneReport zoneReport = zone->second;

		for (HierarchyViewKeeper::SupervisorZoneReport::const_iterator
				zoneInfo = zoneReport.begin(); zoneInfo != zoneReport.end(); zoneInfo++)
		{
			int viewMembers = zoneInfo->second.first;
			if (viewMembers != -1)
			{
				zcInfo->insert(std::make_pair(zone_name, viewMembers));
				totalNodeCount += viewMembers;
				Trace_Event<int> (this, "zoneCensusTask", "numBaseZoneNodes",
						zone_name, viewMembers);
				break;
			}

		}
	}

	zcInfo->insert(std::make_pair("/", totalNodeCount));

	Trace_Event<int> (this, "zoneCensusTask", "total node count", "num",
			totalNodeCount);

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		copyPendingZoneCensusRequestIds = _pendingZoneCensusRequestIds;
		_pendingZoneCensusRequestIds.clear();
	}

	event::ZoneCensus::iterator iter = zcInfo->begin();
	while (iter != zcInfo->end())
	{
		Trace_Event<int> (this, "zoneCensusTask", "total data structure",
				iter->first, iter->second);
		iter++;
	}

	for (int counter = 0; counter
			< (int) copyPendingZoneCensusRequestIds.size(); counter++)
	{
		_memManager_SPtr->notifyZoneCensus(
				_pendingZoneCensusRequestIds[counter], zcInfo, true);
	}

	Trace_Exit(this, "zoneCensusTask()");
}

void HierarchySupervisor::setActiveDelegatesTask()
{
	Trace_Entry(this, "setActiveDelegatesTask()");

	_setActiveDelegatesTaskScheduled = false;

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		Zone2DelegateTable_Map::iterator delegatesTable =
				_delegatesTablesMap.begin();

		while (delegatesTable != _delegatesTablesMap.end())
		{
			bool more = true;
			while (more)
			{
				if (delegatesTable->second->getNumActiveDelegates()
						< _config.getNumberOfActiveDelegates()
						&& delegatesTable->second->getNumActiveDelegates()
								< delegatesTable->second->size())
				{
					NodeIDImpl_SPtr
							activate =
									delegatesTable->second->getAnActiveDelegateCandidate();
					if (activate)
					{
						sendActivateMsg(delegatesTable->first, activate);
					}
					else
					{
						more = false;
					}

				}
				else
				{
					more = false;
				}
			}
			delegatesTable++;
		}
	}
}

//TODO use this method to send the resulting event, using MemTopoThread
//_MemManager_SPtr->notifyForeignZoneMembership(...);
void HierarchySupervisor::foreignZoneMembershipTask(int64_t reqId,
		BusName_SPtr zoneBusName, bool includeAttributes)
{
	Trace_Entry(this, "foreignZoneMembershipTask()", zoneBusName->toString());
	event::ViewMap_SPtr view;

	{
		// get view from local supervisor in case it has an active delegate and the includeAtributes param can be serviced
		if (!includeAttributes || (includeAttributes
				&& _config.getIncludeAttributes()))
		{
			boost::recursive_mutex::scoped_lock lock(_mutex);
			Zone2DelegateTable_Map::iterator delegatesTable =
					_delegatesTablesMap.find(zoneBusName->toString());
			if (delegatesTable != _delegatesTablesMap.end())
			{
				if (delegatesTable->second->hasActiveDelegate())
				{
					view = delegatesTable->second->getView(includeAttributes);
				}
			}
		}

	}
	if (view)
	{
		_memManager_SPtr->notifyForeignZoneMembership(reqId,
				zoneBusName->toString(), view, true);
	}
	else
	{
		Trace_Event(this, "foreignZoneMembershipTask",
				"looking for an active supervisor");

		NodeIDImpl_SPtr activeSupervisor =
				_hierarchyViewKeeper_SPtr->getActiveSupervisor(
						zoneBusName->toString(), includeAttributes);

		if (activeSupervisor)
		{
			obtainRemoteForeignZoneMembership(activeSupervisor, reqId,
					zoneBusName, includeAttributes);
		}
		else
		{
			Trace_Event(this, "foreignZoneMembershipTask",
					"Failed to find appropriate active supervisor");

			_memManager_SPtr->notifyForeignZoneMembership(reqId,
					zoneBusName->toString(),
					event::Foreign_Zone_Membership_Unreachable, "", true);
		}
	}

	Trace_Exit(this, "foreignZoneMembershipTask()");
}

void HierarchySupervisor::foreignZoneMembershipTOTask(int64_t reqId,
		String zoneBusName)
{
	Trace_Entry(this, "foreignZoneMembershipTOTask()", zoneBusName);

	_pendingRemoteForeignZoneMemebershipRequestsMap.erase(reqId);

	_memManager_SPtr->notifyForeignZoneMembership(reqId, zoneBusName,
			event::Foreign_Zone_Membership_Request_Timeout,
			"Request timed out", true);

	Trace_Exit(this, "foreignZoneMembershipTOTask()");

}

void HierarchySupervisor::obtainRemoteForeignZoneMembership(
		NodeIDImpl_SPtr activeSupervisor, int64_t reqId,
		BusName_SPtr zoneBusName, bool includeAttributes)
{
	Trace_Entry(this, "obtainRemoteForeignZoneMembership");

	boost::unordered_map<NodeID_SPtr, std::vector<boost::tuples::tuple<int64_t,
			String, bool> > >::iterator iter =
			_remoteForeignZoneMemebershipRequestsMap.find(activeSupervisor);
	if (iter == _remoteForeignZoneMemebershipRequestsMap.end())
	{
		bool ok = _commAdapter_SPtr->connect(activeSupervisor, HierarchyCtx); //return to incoming-msg-Q
		if (ok)
		{
			Trace_Event(this, "obtainRemoteForeignZoneMembership()",
					"connected to active supervisor", "activeSupervisor",
					NodeIDImpl::stringValueOf(activeSupervisor));
			std::vector<boost::tuples::tuple<int64_t, String, bool> > entry;
			entry.push_back(boost::tuples::tuple<int64_t, String, bool>(reqId,
					zoneBusName->toString(), includeAttributes));
			_remoteForeignZoneMemebershipRequestsMap.insert(make_pair(
					activeSupervisor, entry));
		}
		else
		{
			Trace_Event(this, "obtainRemoteForeignZoneMembership()",
					"Failed to connect to active supervisor",
					"activeSupervisor", NodeIDImpl::stringValueOf(
							activeSupervisor));
		}
	}
	else
	{
		Trace_Event(
				this,
				"obtainRemoteForeignZoneMembership()",
				"Found entry table. Adding another request for the same supervisor",
				"activeSupervisor", NodeIDImpl::stringValueOf(activeSupervisor));
		iter->second.push_back(boost::tuples::tuple<int64_t, String, bool>(
				reqId, zoneBusName->toString(), includeAttributes));

	}

	Trace_Exit(this, "obtainRemoteForeignZoneMembership");

}

bool HierarchySupervisor::isClosed() const
{
	boost::recursive_mutex::scoped_lock lock(_mutex);
	return _closed;
}

void HierarchySupervisor::addDelegate(Neighbor_SPtr delegate,
		BusName_SPtr busName)
{

	Trace_Entry(this, "addDelegate()", "node", delegate->getName(), "zone",
			busName->toString());

	setAttributes(busName);
	scheduleSetActiveDelegatesTask(0);

	Trace_Exit(this, "addDelegate()");

}

void HierarchySupervisor::removeDelegate(NodeIDImpl_SPtr delegate,
		BusName_SPtr busName)
{

	Trace_Entry(this, "removeDelegate()", "node", NodeIDImpl::stringValueOf(
			delegate), "zone", busName->toString());

	setAttributes(busName);
	scheduleSetActiveDelegatesTask(0);

	Trace_Exit(this, "removeSupervisor()");

}

void HierarchySupervisor::setAttributes(BusName_SPtr busName)
{
	Trace_Entry(this, "setAttributes()", "zone", busName->toString());

	//updateAttributes
	AttributeControl& attrCtrl =
			_coreInterface.getMembershipManager()->getAttributeControl();

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		int32_t numBaseZones = _delegatesTablesMap.size();
		if (numBaseZones != _publishedNumBaseZones)
		{
			_publishedNumBaseZones = numBaseZones;
			if (numBaseZones > 0)
			{
				(*_attrValBuffer).reset();
				(*_attrValBuffer).writeInt(numBaseZones);

				std::pair<const int, const char*> value = std::make_pair(
						(int) _attrValBuffer->getPosition(),
						(const char*) _attrValBuffer->getBuffer());

				attrCtrl.setAttribute(
						HierarchyUtils::supervisorState_AttributeKey, value);

				Trace_Event(this, "setAttributes", "setting attribute: ",
						HierarchyUtils::supervisorState_AttributeKey);
			}
			else
			{
				attrCtrl.removeAttribute(
						HierarchyUtils::supervisorState_AttributeKey);
				Trace_Event(this, "setAttributes",
						"Deleting supervisor attributes. No guarded zones");
			}
		}
		else
		{
			Trace_Event(this, "setAttributes", "skipping setting attribute: ",
					HierarchyUtils::supervisorState_AttributeKey);
		}

		//key: prefix + {name}
		String
				key(
						HierarchyUtils::supervisorGuardedBaseZone_AttributeKeyPrefix);
		key.append(busName->toString());

		Zone2DelegateTable_Map::iterator delegatesTable =
				_delegatesTablesMap.find(busName->toString());
		if (delegatesTable != _delegatesTablesMap.end())

		{
			//value: <num of delegates for this zone>

			int32_t numDelegates = delegatesTable->second->size();
			int32_t numMembers = delegatesTable->second->getViewSize();
			bool publish = false;
			boost::unordered_map<String, pair<int32_t, int32_t> >::iterator
					previousStats = _publishedZoneStats.find(
							busName->toString());
			if (previousStats != _publishedZoneStats.end())
			{
				if (previousStats->second.first != numDelegates
						|| previousStats->second.second != numMembers)
				{
					publish = true;
					previousStats->second.first = numDelegates;
					previousStats->second.second = numMembers;
				}
			}
			else
			{
				_publishedZoneStats.insert(make_pair(busName->toString(),
						make_pair(numDelegates, numMembers)));
				publish = true;
			}

			if (publish)
			{
				(*_attrValBuffer).reset();
				(*_attrValBuffer).writeInt(numDelegates);
				(*_attrValBuffer).writeInt(numMembers);
				(*_attrValBuffer).writeBoolean(_config.getIncludeAttributes());

				std::pair<const int, const char*> value = std::make_pair(
						(int) _attrValBuffer->getPosition(),
						(const char*) _attrValBuffer->getBuffer());

				attrCtrl.setAttribute(key, value);

				Trace_Event(this, "setAttributes", "setting attribute: ", key);
			}
			else
			{
				Trace_Event(this, "setAttributes",
						"skipping setting attribute: ", key);
			}
		}
		else
		{
			bool publish = false;
			boost::unordered_map<String, pair<int32_t, int32_t> >::iterator
					previousStats = _publishedZoneStats.find(
							busName->toString());
			if (previousStats != _publishedZoneStats.end())
			{
				_publishedZoneStats.erase(busName->toString());
				publish = true;
			}
			if (publish)
			{
				attrCtrl.removeAttribute(key);
				Trace_Event(this, "setAttributes", "Deleting zone attribute: ",
						busName->toString());
			}
			else
			{
				Trace_Event(this, "setAttributes",
						"Skipping Deleting zone attribute: ",
						busName->toString());

			}
		}
	}
	Trace_Exit(this, "setAttributes()");
}

void HierarchySupervisor::processIncomingConnectRequestMsg(
		SCMessage_SPtr incomingHierMsg)

{
	Trace_Entry(this, "processIncomingConnectRequestMsg");

	NodeIDImpl_SPtr peerName = incomingHierMsg->getSender();
	BusName_SPtr busName = incomingHierMsg->getBusName();
	Neighbor_SPtr myNeighbor =
			_commAdapter_SPtr->connectOnExisting(peerName);
	if (!myNeighbor)
	{
		Trace_Event(this, "processIncomingConnectRequestMsg",
				"Warning: connectOnExisting() failed");
	}
	else
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		if (myNeighbor->getReceiverId() != 0 && myNeighbor->getReceiverId()
				!= incomingHierMsg->getStreamId())
		{
			Trace_Event(
					this,
					"processIncomingConnectRequestMsg()",
					"Warning: assert(myNeighbor->getRecieverId() == 0 || myNeighbor->getReceiverId() == inHierarchyMsg->getStreamId()); failed");
		}
		myNeighbor->setReceiverId(incomingHierMsg->getStreamId());

		Zone2DelegateTable_Map::iterator delegatesTable =
				_delegatesTablesMap.find(busName->toString());
		int numCurrentZoneDelegates = delegatesTable
				== _delegatesTablesMap.end() ? 0
				: delegatesTable->second->size();

		int num_missing_delegates = _config.getNumberOfDelegates()
				- numCurrentZoneDelegates;
		short response =
				(num_missing_delegates > 0) ? (int) HierarchyUtils::ACCEPT
						: (int) HierarchyUtils::REJECT;

		if (response == HierarchyUtils::ACCEPT)
		{
			if (delegatesTable == _delegatesTablesMap.end())
			{
				SupervisorNeighborTable_SPtr _delegatesTable =
						SupervisorNeighborTable_SPtr (
								new SupervisorNeighborTable(
										_config.getNodeName(),
										"delegatesTable", _instID));
				_delegatesTable->addEntry(peerName, myNeighbor);
				bool res = _delegatesTablesMap.insert(make_pair(
						busName->toString(), _delegatesTable)).second;
				if (res)
				{
					Trace_Event(this, "processIncomingConnectRequestMsg()",
							"A new entry was added to _delegatesTablesMap: ",
							"zone", busName->toString());
				}
				else
				{
					Trace_Event(
							this,
							"processIncomingConnectRequestMsg()",
							"Warning: A new entry was NOT added to _delegatesTablesMap: ",
							busName->toString());
				}
			}
			else
			{
				delegatesTable->second->addEntry(peerName, myNeighbor);
			}
			addDelegate(myNeighbor, busName); //set attributes, etc

			Trace_Event(this, "processIncomingConnectRequestMsg()",
					"Hier Connectivity event; added delegate", "Complete map",
					delegatesTablesMapToString());
		}

		Trace_Event(this, "processIncomingConnectRequestMsg()",
				"Sending Type_Hier_Connect_Reply: ",
				HierarchyUtils::ReplyTypeName[response]);

		(*_outgoingHierMessage).writeH1Header(
				SCMessage::Type_Hier_Connect_Reply);
		ByteBuffer& bb = *(*_outgoingHierMessage).getBuffer();
		bb.writeString(_config.getNodeName());
		bb.writeString(peerName->getNodeName());
		bb.writeShort(response);
		(*_outgoingHierMessage).updateTotalLength();
		if (_config.isCRCMemTopoMsgEnabled())
		{
			(*_outgoingHierMessage).writeCRCchecksum();
		}

		if (myNeighbor->sendMessage(_outgoingHierMessage))
		{
			Trace_Event(this, "processIncomingConnectRequestMsg",
					"couldn't send a connect reply message to", "node",
					myNeighbor->getName());
			// TODO: decide what to do
		}

	}
	Trace_Exit(this, "processIncomingConnectRequestMsg");
}

void HierarchySupervisor::processIncomingDisconnectRequestMsg(
		SCMessage_SPtr incomingHierMsg)
{
	Trace_Entry(this, "processIncomingDisconnectRequestMsg");
	short reply = 0;
	NodeIDImpl_SPtr peerName = incomingHierMsg->getSender();
	BusName_SPtr busName = incomingHierMsg->getBusName();
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		Zone2DelegateTable_Map::iterator delegatesTable =
				_delegatesTablesMap.find(busName->toString());
		if (delegatesTable != _delegatesTablesMap.end())
		{
			Neighbor_SPtr myNeighbor =
					delegatesTable->second->getNeighbor(peerName);
			if (myNeighbor)
			{
				Trace_Event(this, "processIncomingDisconnectRequestMsg",
						"received disconnect request, in delegates table",
						"sender", NodeIDImpl::stringValueOf(peerName));

				//reply = _delegatesTable.size() > _config.getNumberOfDelegates();
				reply = HierarchyUtils::ACCEPT;
				if (reply == HierarchyUtils::ACCEPT)
				{
					delegatesTable->second->removeEntry(peerName);
					bool hit = delegatesTable->second->setInactiveDelegate(peerName);
					if (hit)
					{
						//update S-Bridge
						_coreInterface.getRoutingManager()->supervisorPubSubBridge_remove_active(
								busName, peerName);
					}

					if (delegatesTable->second->size() == 0)
					{
						_delegatesTablesMap.erase(busName->toString());
						Trace_Event(this,
								"processIncomingDisconnectRequestMsg",
								"removing zone from delegates table", "zone",
								busName->toString());
					}
					removeDelegate(peerName, busName);
				}
				(*_outgoingHierMessage).writeH1Header(
						SCMessage::Type_Hier_Disconnect_Reply);
				ByteBuffer& bb = *(*_outgoingHierMessage).getBuffer();
				bb.writeString(_config.getNodeName());
				bb.writeString(peerName->getNodeName());
				bb.writeShort(reply);
				(*_outgoingHierMessage).updateTotalLength();
				if (_config.isCRCMemTopoMsgEnabled())
				{
					(*_outgoingHierMessage).writeCRCchecksum();
				}

				if (myNeighbor->sendMessage(_outgoingHierMessage))
				{
					Trace_Event(this, "processIncomingDisconnectRequestMsg",
							"couldn't send a connect reply message to", "node",
							myNeighbor->getName());
					// TODO: decide what to do
				}

			}
			else
			{
				//This should not happen
				Trace_Event(
						this,
						"processIncomingDisconnectRequestMsg",
						"Warning: received disconnect request but neighbor not found, ignoring: ",
						NodeIDImpl::stringValueOf(peerName));

			}
		}
		else
		{
			//This should not happen
			Trace_Event(
					this,
					"processIncomingDisconnectRequestMsg",
					"Warning: received disconnect request but zone not found, ignoring: ",
					busName->toString());
		}
	}
	Trace_Exit<short> (this, "processIncomingDisconnectRequestMsg", reply);
}

void HierarchySupervisor::processIncomingConnectReplyMsg(
		SCMessage_SPtr incomingHierMsg)
{
	Trace_Entry(this, "processIncomingConnectReplyMsg",
			"Warning. Should not happen at the supervisor");

}

void HierarchySupervisor::processIncomingDisconnectReplyMsg(
		SCMessage_SPtr incomingHierMsg)
{
	Trace_Entry(this, "processIncomingDisconnectReplyMsg",
			"Warning. Should not happen at the supervisor");
}

void HierarchySupervisor::processIncomingNodeLeaveMsg(
		SCMessage_SPtr incomingHierMsg)

{
	Trace_Entry(this, "processIncomingNodeLeaveMsg");
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		NodeIDImpl_SPtr peerName = incomingHierMsg->getSender();
		BusName_SPtr busName = incomingHierMsg->getBusName();
		Zone2DelegateTable_Map::iterator delegatesTable =
				_delegatesTablesMap.find(busName->toString());
		if (delegatesTable != _delegatesTablesMap.end())
		{
			Neighbor_SPtr myNeighbor =
					delegatesTable->second->getNeighbor(peerName);
			if (myNeighbor)
			{
				Trace_Event(this, "processIncomingNodeLeaveMsg",
						"received Leave, in delegates-table", "sender",
						NodeIDImpl::stringValueOf(peerName));

				delegatesTable->second->removeEntry(peerName);
				bool hit = delegatesTable->second->setInactiveDelegate(peerName);
				if (hit)
				{
					//update S-Bridge
					_coreInterface.getRoutingManager()->supervisorPubSubBridge_remove_active(
									busName, peerName);
				}

				if (delegatesTable->second->size() == 0)
				{
					_delegatesTablesMap.erase(busName->toString());
					Trace_Event(this, "processIncomingNodeLeaveMsg",
							"removing zone from delegates table", "zone",
							busName->toString());
				}
				removeDelegate(peerName, busName);
			}
			else
			{

				Trace_Event(
						this,
						"processIncomingNodeLeaveMsg",
						"Warning: received Leave but neighbor not found, ignoring",
						NodeIDImpl::stringValueOf(peerName));
			}
		}
		else
		{
			Trace_Event(this, "processIncomingNodeLeaveMsg",
					"Warning: received Leave but zone not found, ignoring: ",
					busName->toString());
		}
	}
	Trace_Exit(this, "processIncomingNodeLeaveMsg");
}

void HierarchySupervisor::processIncomingViewUpdate(
		SCMessage_SPtr incomingHierMsg)
{
	Trace_Entry(this, "processIncomingViewUpdate");

	ByteBuffer_SPtr buffer = incomingHierMsg->getBuffer();
	long viewId = buffer->readLong();
	int numEvents = buffer->readInt();

	std::ostringstream oss;
	oss << "viewId: " << viewId << "; numEvents: " << numEvents;

	Trace_Event(this, "processIncomingViewUpdate", oss.str());

	boost::recursive_mutex::scoped_lock lock(_mutex);
	BusName_SPtr busName = incomingHierMsg->getBusName();
	Zone2DelegateTable_Map::iterator delegatesTable = _delegatesTablesMap.find(busName->toString());
	if (delegatesTable != _delegatesTablesMap.end())
	{

		if (delegatesTable->second->isActiveDelegate(
				incomingHierMsg->getSender()))
		{
			for (int counter = 0; counter < numEvents; counter++)
			{
				SCMembershipEvent event =
						incomingHierMsg->readSCMembershipEvent();
				TRACE_EVENT2(this, "processIncomingViewUpdate", event.toString());
				delegatesTable->second->processViewEvent(event);
			}
			setAttributes(busName);
		}
		else
		{
			Trace_Event(this, "processIncomingViewUpdate",
					"Received message from a non active delegate. Ignoring");
		}
	}

	Trace_Exit(this, "processIncomingViewUpdate");
}

void HierarchySupervisor::processIncomingStartMembershipPushReplyMsg(
		SCMessage_SPtr incomingHierMsg)
{
	Trace_Entry(this, "processIncomingStartMembershipPushReplyMsg");

	ByteBuffer_SPtr buffer = incomingHierMsg->getBuffer();
	bool accept = buffer->readBoolean();

	if (accept)
	{
		Trace_Event(this, "processIncomingStartMembershipPushReplyMsg",
				"accepted");
	}
	else
	{
		BusName_SPtr busName = incomingHierMsg->getBusName();
		String targetBus = busName->toString();
		NodeIDImpl_SPtr sender = incomingHierMsg->getSender();
		Zone2DelegateTable_Map::iterator delegatesTable =
				_delegatesTablesMap.find(targetBus);
		if (delegatesTable != _delegatesTablesMap.end())
		{
			bool hit = delegatesTable->second->setInactiveDelegate(sender);
			if (hit)
			{
				//update S-Bridge
				_coreInterface.getRoutingManager()->supervisorPubSubBridge_remove_active(
						busName, sender);
			}
		}
		Trace_Event(this, "processIncomingStartMembershipPushReplyMsg",
				"rejected");
		scheduleSetActiveDelegatesTask(0);
	}

	Trace_Exit(this, "processIncomingStartMembershipPushReplyMsg");

}

void HierarchySupervisor::processIncomingForeignZoneMembershipRequestMsg(
		SCMessage_SPtr incomingHierMsg)
{
	Trace_Entry(this, "processIncomingForeignZoneMembershipRequestMsg");

	ByteBuffer_SPtr buffer = incomingHierMsg->getBuffer();
	NodeIDImpl_SPtr orgSenderId = buffer->readNodeID();
	int64_t reqId = buffer->readLong();
	String zoneName = buffer->readString();
	bool includeAttrs = buffer->readBoolean();

	SCViewMap_SPtr view;

	{
		// get view from local supervisor in case it has an active delegate and the includeAtributes param can be serviced
		if (!includeAttrs || _config.getIncludeAttributes())
		{
			boost::recursive_mutex::scoped_lock lock(_mutex);
			Zone2DelegateTable_Map::iterator delegatesTable =
					_delegatesTablesMap.find(zoneName);
			if (delegatesTable != _delegatesTablesMap.end())
			{
				if (delegatesTable->second->hasActiveDelegate())
				{
					view = delegatesTable->second->getSCView();
				}
			}
		}

	}

	(*_outgoingHierMessage).writeH1Header(
			SCMessage::Type_Hier_SupOp_Reply_ForeignZoneMemberships);
	ByteBuffer& bb = *(*_outgoingHierMessage).getBuffer();
	bb.writeNodeID(_config.getMyNodeID());
	bb.writeLong(reqId);
	bb.writeString(zoneName);
	if (view)
	{
		bb.writeChar(HierarchyUtils::ACCEPT);
		(*_outgoingHierMessage).writeSCMembershipEvent(SCMembershipEvent(
				SCMembershipEvent::View_Change, view), includeAttrs);

	}
	else
	{
		bb.writeChar(HierarchyUtils::REJECT);
		bb.writeString("Information could not be found locally");
	}
	(*_outgoingHierMessage).updateTotalLength();
	if (_config.isCRCMemTopoMsgEnabled())
	{
		(*_outgoingHierMessage).writeCRCchecksum();
	}

	NodeIDImpl_SPtr peerName = incomingHierMsg->getSender();

	Neighbor_SPtr myNeighbor =
			_commAdapter_SPtr->connectOnExisting(peerName);
	if (myNeighbor)
	{
		if (myNeighbor->sendMessage(_outgoingHierMessage))
		{
			Trace_Event(this, "processIncomingForeignZoneMembershipRequestMsg",
					"couldn't send a reply message to", "node",
					myNeighbor->getName());
			// TODO: decide what to do
		}

	}

	Trace_Exit(this, "processIncomingForeignZoneMembershipRequestMsg");
}

void HierarchySupervisor::processIncomingForeignZoneMembershipReplyMsg(
		SCMessage_SPtr incomingHierMsg)
{
	Trace_Entry(this, "processIncomingForeignZoneMembershipReplyMsg");

	//	Original-Source					NodeID
	//	Request-ID					int64_t
	//	Zone-name					string
	//	Reply						enum ReplyType, char
	//	If (Reply == ACCEPT)
	//	View 					a single MemEvent of type View change (VC)
	//	if (Reply == REJECT)
	//		Message 				string, descriptive text
	//	If (Reply==REDIRECT)
	//		Redirect-address			NodeID

	NodeIDImpl_SPtr peerName = incomingHierMsg->getSender();

	ByteBuffer_SPtr buffer = incomingHierMsg->getBuffer();
	NodeIDImpl_SPtr orgSenderId = buffer->readNodeID();
	int64_t reqId = buffer->readLong();
	String zoneName = buffer->readString();
	char reply = buffer->readChar();

	boost::unordered_map<int64_t, pair<String,
			AbstractTask_SPtr > >::iterator iter =
			_pendingRemoteForeignZoneMemebershipRequestsMap.find(reqId);
	if (iter != _pendingRemoteForeignZoneMemebershipRequestsMap.end())
	{
		AbstractTask_SPtr task = iter->second.second;
		if (task)
		{
			task->cancel();
		}
		else
		{
			Trace_Event(this, "processIncomingForeignZoneMembershipReplyMsg",
					"Warning: Found invalid task in data structure", "zone",
					iter->second.first);
		}
		_pendingRemoteForeignZoneMemebershipRequestsMap.erase(iter);

		switch (reply)
		{
		case HierarchyUtils::ACCEPT:
		{
			SCMembershipEvent event = incomingHierMsg->readSCMembershipEvent();

			if (event.getType() != SCMembershipEvent::View_Change)
			{
				String
						what(
								"Error while unmarshaling reply message (unexpected event type) from ");
				what.append(peerName->getNodeName());
				Trace_Event(this,
						"processIncomingForeignZoneMembershipReplyMsg", what);
				throw MessageUnmarshlingException(what);
			}

			SCViewMap_SPtr scView = event.getView();
			boost::shared_ptr<event::ViewMap> view =
					SupervisorViewKeeper::convertSCtoViewMap(scView);

			_memManager_SPtr->notifyForeignZoneMembership(reqId, zoneName,
					view, true);
		}

			break;
		case HierarchyUtils::REJECT:
		{
			String errMsg = buffer->readString();
			_memManager_SPtr->notifyForeignZoneMembership(reqId, zoneName,
					event::Foreign_Zone_Membership_Unreachable, errMsg, true);
		}
			break;
		case HierarchyUtils::REDIRECT:
			break;
		default:
			break;
		}
	}
	else
	{
		_pendingRemoteForeignZoneMemebershipRequestsMap.erase(reqId);
		Trace_Event(this, "processIncomingForeignZoneMembershipReplyMsg",
				"Response discarded since it came after the timeout has expired");
	}

	Trace_Exit(this, "processIncomingForeignZoneMembershipReplyMsg");
}

void HierarchySupervisor::sendLeaveMsg()
{
	Trace_Entry(this, "sendLeaveMsg");

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		Zone2DelegateTable_Map::iterator tablesIter;
		for (tablesIter = _delegatesTablesMap.begin(); tablesIter
				!= _delegatesTablesMap.end(); ++tablesIter)
		{
			SupervisorNeighborTable::NeighborTableMap::iterator iter;
			for (iter = tablesIter->second->_table.begin(); iter
					!= tablesIter->second->_table.end(); ++iter)
			{
				_outgoingHierMessage->writeH1Header(SCMessage::Type_Hier_Leave);
				ByteBuffer_SPtr buffer = _outgoingHierMessage->getBuffer();
				buffer->writeString(_config.getNodeName());
				buffer->writeString(iter->second.neighbor->getName());
				_outgoingHierMessage->updateTotalLength();
				if (_config.isCRCMemTopoMsgEnabled())
				{
					_outgoingHierMessage->writeCRCchecksum();
				}

				if (iter->second.neighbor->sendMessage(_outgoingHierMessage))
				{
					Trace_Event(this, "sendLeaveMsg",
							"Warning: failed to send message to: ", "target",
							iter->second.neighbor->getName());
				}
			}
		}
	}
	Trace_Exit(this, "sendLeaveMsg()");
}

//TODO use this method to send the resulting event, using MemTopoThread
//_MemManager_SPtr->notifyForeignZoneMembership(...);
int64_t HierarchySupervisor::queueForeignZoneMembershipRequest(
		BusName_SPtr zoneBusName, bool includeAttributes, int timeoutMillis)
{
	Trace_Entry(this, "queueForeignZoneMembershipRequest()",
			zoneBusName->toString());

	int64_t id;
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		id = _requestId++;
		scheduleforeignZoneMembershipTask(0, id, zoneBusName, includeAttributes);
	}

	Trace_Exit<int64_t> (this, "queueForeignZoneMembershipRequest()", id);

	return id;
}

//TODO use this method to send the resulting event, using MemTopoThread
//_MemManager_SPtr->notifyZoneCensus(...);
int64_t HierarchySupervisor::queueZoneCensusRequest()
{
	Trace_Entry(this, "queueZoneCensusRequest()");

	int64_t id;
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		id = _requestId;
		_pendingZoneCensusRequestIds.push_back(_requestId++);
		scheduleZoneCensusTask(0);
	}

	Trace_Exit<int64_t> (this, "queueZoneCensusRequest()", id);
	return id;
}

String HierarchySupervisor::delegatesTablesMapToString()
{
	std::ostringstream oss;
	Zone2DelegateTable_Map::iterator
			delegatesTable;
	for (delegatesTable = _delegatesTablesMap.begin(); delegatesTable
			!= _delegatesTablesMap.end(); delegatesTable++)
	{
		oss << delegatesTable->first << ": "
				<< delegatesTable->second->toString();
	}
	return oss.str();
}

void HierarchySupervisor::scheduleSetActiveDelegatesTask(int delayMillis)
{
	Trace_Entry(this, "scheduleSetActiveDelegatesTask");

	if (!_setActiveDelegatesTaskScheduled)
	{
		_taskSchedule_SPtr->scheduleDelay(_setActiveDelegatesTask_SPtr,
				boost::posix_time::milliseconds(delayMillis));
		_setActiveDelegatesTaskScheduled = true;
		Trace_Debug(this, "scheduleSetActiveDelegatesTask()", "rescheduled");
	}
	else
	{
		Trace_Debug(this, "scheduleSetActiveDelegatesTask()",
				"already scheduled");
	}
	Trace_Exit(this, "scheduleSetActiveDelegatesTask");
}

void HierarchySupervisor::scheduleZoneCensusTask(int delayMillis)
{
	Trace_Entry(this, "scheduleZoneCensusTask");

	if (!_zoneCensusTaskScheduled)
	{
		_taskSchedule_SPtr->scheduleDelay(_zoneCensusTask_SPtr,
				boost::posix_time::milliseconds(delayMillis));
		_zoneCensusTaskScheduled = true;
		Trace_Debug(this, "scheduleZoneCensusTask()", "scheduled");
	}
	else
	{
		Trace_Debug(this, "scheduleZoneCensusTask()", "already scheduled");
	}
	Trace_Exit(this, "scheduleZoneCensusTask");
}

void HierarchySupervisor::scheduleforeignZoneMembershipTask(int delayMillis,
		int64_t reqId, BusName_SPtr zoneBusName, bool includeAttributes)
{
	Trace_Entry(this, "scheduleforeignZoneMembershipTask");

	_taskSchedule_SPtr->scheduleDelay(AbstractTask_SPtr (
			new HierarchySupervisorForeignZoneMembershipTask(_instID, *this,
					reqId, zoneBusName, includeAttributes)),
			boost::posix_time::milliseconds(delayMillis));

	Trace_Exit(this, "scheduleforeignZoneMembershipTask", "scheduled for",
			zoneBusName->toString());
}

void HierarchySupervisor::scheduleforeignZoneMembershipTOTask(int delayMillis,
		int64_t reqId, String zoneBusName)
{
	Trace_Entry(this, "scheduleforeignZoneMembershipTOTask");

	AbstractTask_SPtr task =
			AbstractTask_SPtr (
					new HierarchySupervisorForeignZoneMembershipTOTask(_instID,
							*this, reqId, zoneBusName));
	_taskSchedule_SPtr->scheduleDelay(task, boost::posix_time::milliseconds(
			delayMillis));

	_pendingRemoteForeignZoneMemebershipRequestsMap.insert(make_pair(reqId,
			make_pair(zoneBusName, task)));

	Trace_Exit(this, "scheduleforeignZoneMembershipTOTask", "scheduled for",
			zoneBusName);
}

void HierarchySupervisor::sendActivateMsg(String targetBus,
		NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "sendActivateMsg");

	boost::recursive_mutex::scoped_lock lock(_mutex);

	Zone2DelegateTable_Map::iterator
			delegatesTable = _delegatesTablesMap.find(targetBus);
	if (delegatesTable != _delegatesTablesMap.end())
	{
		Neighbor_SPtr myNeighbor =
				delegatesTable->second->getNeighbor(targetName);
		if (myNeighbor)
		{
			(*_outgoingHierMessage).writeH1Header(
					SCMessage::Type_Hier_DelOp_Request_StartMembershipPush);
			ByteBuffer& bb = *(*_outgoingHierMessage).getBuffer();
			bb.writeString(_config.getNodeName());
			bb.writeString(targetName->getNodeName());
			bb.writeBoolean(_config.getIncludeAttributes());
			bb.writeShort(
					_config.getHierarchyMemberhipUpdateAggregationInterval());

			(*_outgoingHierMessage).updateTotalLength();
			if (_config.isCRCMemTopoMsgEnabled())
			{
				(*_outgoingHierMessage).writeCRCchecksum();
			}

			if (myNeighbor->sendMessage(_outgoingHierMessage) != 0)
			{
				Trace_Event(this, "sendActivateMsg",
						"couldn't send an activate message to", "node",
						myNeighbor->getName());
				// TODO: decide what to do
			}
			else
			{
				delegatesTable->second->setActiveDelegate(targetName);
				//update the S-Bridge
				_coreInterface.getRoutingManager()->supervisorPubSubBridge_add_active(
									targetBus, targetName, myNeighbor);
			}
		}
		else
		{
			Trace_Event(this, "sendActivateMsg()", "could not find entry",
					"bus", targetBus, "target", NodeIDImpl::stringValueOf(
							targetName));
		}

	}

	Trace_Exit(this, "sendActivateMsg");

}

}
