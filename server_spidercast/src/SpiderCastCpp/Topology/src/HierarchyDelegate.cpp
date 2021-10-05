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
 * HierarchyDelegate.cpp
 *
 *  Created on: Oct 13, 2010
 */

#include "HierarchyDelegate.h"

namespace spdr
{

ScTraceComponent* HierarchyDelegate::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy,
		"HierarchyDelegate",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchyDelegate::HierarchyDelegate(const String& instID,
		const SpiderCastConfigImpl& config,
		NodeIDCache& nodeIDCache,
		VirtualIDCache& vidCache,
		CoreInterface& coreInterface,
		boost::shared_ptr<HierarchyViewKeeper> viewKeeper) :
		ScTraceContext(tc_,instID,config.getMyNodeID()->getNodeName()),
		instID_(instID),
		config_(config),
		nodeIDCache_(nodeIDCache),
		coreInterface_(coreInterface),
		hierarchyViewKeeper_SPtr(viewKeeper),
		closed_(false),
		supervisorBootstrapSet_(instID_,config_.getSupervisorBootstrapSet(),config_.getMyNodeID(),vidCache,false),
		outgoingLogicalConnectRequests_(config_.getNodeName(), "LogicalConnectRequests", instID_),
		supervisorNeighborTable_(config_.getNodeName(), "SupervisorTable", instID_),
		supervisorStateTable_(),
		quarantineConnectTargets_(),
		disconnectRequested_(),
		taskSchedule_SPtr(),
		commAdapter_SPtr(),
		connectTask_SPtr(),
		hierarchyManager_SPtr(),
		membershipManager_SPtr(),
		connectTaskScheduled_(false),
		firstConnectTask_(true),
		outgoingHierMessage_(),
		attrValBuffer_(),
		viewUpdateTaskScheduled_(false),
		viewUpdateTask_SPtr(),
		pubsubBridgeTaskScheduled_(false),
		pubsubBridgeTask_SPtr(),
		pubsubBridge_target_()

{
	Trace_Entry(this, "HierarchyDelegate()");

	outgoingHierMessage_ = SCMessage_SPtr(new SCMessage);
	outgoingHierMessage_->setBuffer(ByteBuffer::createByteBuffer(128));
	attrValBuffer_ = ByteBuffer::createByteBuffer(128);
}

HierarchyDelegate::~HierarchyDelegate()
{
	Trace_Entry(this, "~HierarchyDelegate()");
}

void HierarchyDelegate::init()
{
	Trace_Entry(this, "init()");

	taskSchedule_SPtr = coreInterface_.getTopoMemTaskSchedule();
	commAdapter_SPtr = coreInterface_.getCommAdapter();
	connectTask_SPtr = AbstractTask_SPtr(
			new HierarchyDelegateConnectTask(instID_,*this));
	hierarchyManager_SPtr = coreInterface_.getHierarchyManager();
	membershipManager_SPtr = coreInterface_.getMembershipManager();
	viewUpdateTask_SPtr = AbstractTask_SPtr(
			new HierarchyDelegateViewUpdateTask(instID_,*this));
	pubsubBridgeTask_SPtr = AbstractTask_SPtr(
			new HierarchyDelegatePubSubBridgeTask(instID_,*this));
}

void HierarchyDelegate::terminate()
{
	Trace_Entry(this, "terminate()");

	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		closed_ = true;
	}
}

void HierarchyDelegate::destroyCrossRefs()
{
	Trace_Entry(this, "destroyCrossRefs()");
	taskSchedule_SPtr.reset();
	commAdapter_SPtr.reset();
	connectTask_SPtr.reset();
	hierarchyManager_SPtr.reset();
	membershipManager_SPtr.reset();
	viewUpdateTask_SPtr.reset();
	pubsubBridgeTask_SPtr.reset();
}

void HierarchyDelegate::sendLeave2All()
{
	Trace_Entry(this, "sendLeave2All()");

	NeighborTable::NeighborTableMap::const_iterator it;
	for (it = supervisorNeighborTable_._table.begin();
			it != supervisorNeighborTable_._table.end(); ++it)
	{
		sendLeave(it->second.neighbor);
	}

	for (it = outgoingLogicalConnectRequests_._table.begin();
			it != outgoingLogicalConnectRequests_._table.end(); ++it)
	{
		sendLeave(it->second.neighbor);
	}

	Trace_Exit(this, "sendLeave2All()");
}

bool HierarchyDelegate::isClosed() const
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	return closed_;
}

void HierarchyDelegate::processIncomingHierarchyMessage(SCMessage_SPtr inHierarchyMsg)
{
	Trace_Entry(this,"processIncomingHierarchyMessage");

	if (isClosed())
	{
		Trace_Exit(this,"processIncomingHierarchyMessage", "closed");
		return;
	}

	SCMessage::H1Header h1 = inHierarchyMsg->readH1Header();
	NodeIDImpl_SPtr sender = inHierarchyMsg->getSender();
	BusName_SPtr sender_bus = inHierarchyMsg->getBusName();

	//TODO verify sender bus is L1

	ByteBuffer_SPtr buffer = inHierarchyMsg->getBuffer();
	{
		String senderName = buffer->readString();
		String targetName = buffer->readString();
		coreInterface_.getHierarchyManager()->verifyIncomingMessageAddressing(
				senderName, sender->getNodeName(), targetName); //FAIL-FAST, throws SpiderCastRuntimeError
	}

	Trace_Event(this, "processIncomingHierarchyMessage",
			"msg", toString<SCMessage>(inHierarchyMsg));

	switch (h1.get<1>()) //message type
	{

	case SCMessage::Type_Hier_Connect_Reply:
	{
		Neighbor_SPtr neighbor = outgoingLogicalConnectRequests_.getNeighbor(sender);
		if (!neighbor)
		{
			//should not happen FAIL FAST
			Trace_Event(this,"processIncomingHierarchyMessage",
					"Received Type_Hier_Connect_Reply, but no valid Neighbor");
			std::ostringstream what;
			what << "Received Type_Hier_Connect_Reply, but no valid Neighbor: sender="
					<< NodeIDImpl::stringValueOf(sender);
			Trace_Event(this,"processIncomingHierarchyMessage", what.str());
			throw SpiderCastRuntimeError(what.str());
		}

		int reply = buffer->readShort();

		if (reply == HierarchyUtils::ACCEPT)
		{
			Trace_Event(this,"processIncomingHierarchyMessage", "Type_Hier_Connect_Reply - ACCEPT");
			outgoingLogicalConnectRequests_.removeEntry(sender);
			supervisorNeighborTable_.addEntry(sender,neighbor);
			supervisorStateTable_.insert(std::make_pair(sender, boost::tuples::make_tuple(false,false,false)));
			neighbor->setReceiverId(inHierarchyMsg->getStreamId()); //associate RcvSID w. neighbor
			addSupervisor(sender); //set attributes, etc;
		}
		else if (reply == HierarchyUtils::REJECT)
		{
			Trace_Event(this,"processIncomingHierarchyMessage", "Type_Hier_Connect_Reply - REJECT");
			outgoingLogicalConnectRequests_.removeEntry(sender);
			commAdapter_SPtr->disconnect(neighbor);
			quarantineSupervisorCandidate(sender);
			rescheduleConnectTask(0);
		}
		else if (reply == HierarchyUtils::REDIRECT)
		{
			Trace_Event(this,"processIncomingHierarchyMessage", "Type_Hier_Connect_Reply - REDIRECT");
			int num_targets = buffer->readShort();
			std::vector<NodeIDImpl_SPtr> targets;
			for (int i=0; i<num_targets; ++i)
			{
				NodeIDImpl_SPtr target = inHierarchyMsg->readNodeID();
				targets.push_back(target);
			}

			if (targets.size() > 0)
			{
				//TODO add target list to supervisor-bootstrap
			}

			outgoingLogicalConnectRequests_.removeEntry(sender);
			commAdapter_SPtr->disconnect(neighbor);
			quarantineSupervisorCandidate(sender);
			rescheduleConnectTask(0);
		}
		else
		{
			std::ostringstream what;
			what << "Type_Hier_Connect_Reply Unexpected reply value: " << reply;
			Trace_Event(this,"processIncomingHierarchyMessage", what.str());
			throw SpiderCastRuntimeError(what.str());
		}
	}
	break;

	case SCMessage::Type_Hier_Disconnect_Request:
	{
		Neighbor_SPtr neighbor = supervisorNeighborTable_.getNeighbor(sender);
		if (neighbor)
		{
			Trace_Event(this, "processIncomingHierarchyMessage",
				"received disconnect request, in supervisor table",
				"sender", NodeIDImpl::stringValueOf(sender));

			supervisorNeighborTable_.removeEntry(sender);
			supervisorStateTable_.erase(sender);
			sendDisconnectReply(neighbor,true);
			quarantineSupervisorCandidate(sender);
			removeSupervisor(sender);
		}
		else
		{
			neighbor = outgoingLogicalConnectRequests_.getNeighbor(sender);
			if (neighbor)
			{
				Trace_Event(this, "processIncomingHierarchyMessage",
					"received disconnect request, in outgoing-logical",
					"sender", NodeIDImpl::stringValueOf(sender));

				outgoingLogicalConnectRequests_.removeEntry(sender);
				sendDisconnectReply(neighbor,true);
				supervisorBootstrapSet_.setInView(sender,false);
			}
			else
			{
				//This should not happen
				supervisorBootstrapSet_.setInView(sender,false);
				Trace_Event(this, "processIncomingHierarchyMessage",
						"Warning: received disconnect request but neighbor not found, ignoring");
			}
		}

		rescheduleConnectTask(0);
	}
	break;

	case SCMessage::Type_Hier_Disconnect_Reply:
	{
		int reply = buffer->readShort();
		bool requested = (disconnectRequested_.erase(sender) > 0);

		if (requested)
		{
			if (reply == HierarchyUtils::ACCEPT) //accept
			{
				Neighbor_SPtr neighbor = supervisorNeighborTable_.getNeighbor(sender);
				if (neighbor)
				{
					Trace_Event(this, "processIncomingHierarchyMessage",
							"received disconnect reply, in supervisor-table",
							"sender", NodeIDImpl::stringValueOf(sender));

					supervisorNeighborTable_.removeEntry(sender);
					supervisorStateTable_.erase(sender);
					commAdapter_SPtr->disconnect(neighbor);
					supervisorBootstrapSet_.setInView(sender,false);
					removeSupervisor(sender);
				}
				else
				{
					neighbor = outgoingLogicalConnectRequests_.getNeighbor(sender);
					if (neighbor)
					{
						Trace_Event(this, "processIncomingHierarchyMessage",
								"Warning: received disconnect reply, in outgoing-logical",
								"sender", NodeIDImpl::stringValueOf(sender));

						outgoingLogicalConnectRequests_.removeEntry(sender);
						commAdapter_SPtr->disconnect(neighbor);
						supervisorBootstrapSet_.setInView(sender,false);
					}
					else
					{
						supervisorBootstrapSet_.setInView(sender,false);
						Trace_Event(this, "processIncomingHierarchyMessage",
								"Warning: received disconnect reply but neighbor not found, ignoring",
								"sender", NodeIDImpl::stringValueOf(sender));
					}
				}
			}
			else if (reply == HierarchyUtils::REJECT)			//rejected
			{
				Trace_Event(this, "processIncomingHierarchyMessage",
						"Disconnect request rejected",
						"sender", NodeIDImpl::stringValueOf(sender));
			}
			else
			{
				std::ostringstream what;
				what << "Type_Hier_Disconnect_Reply from:" << NodeIDImpl::stringValueOf(sender)
					<< ", Unexpected reply value: " << reply;
				Trace_Event(this,"processIncomingHierarchyMessage", what.str());
				throw SpiderCastRuntimeError(what.str());
			}
		}
		else
		{
			Trace_Event(this, "processIncomingHierarchyMessage",
					"Warning: received disconnect reply but a disconnect was not requested, ignoring",
					"sender", NodeIDImpl::stringValueOf(sender));
		}

		rescheduleConnectTask(0);
	}
	break;

	case SCMessage::Type_Hier_Leave:
	{
		Neighbor_SPtr neighbor = supervisorNeighborTable_.getNeighbor(sender);
		if (neighbor)
		{
			Trace_Event(this, "processIncomingHierarchyMessage",
				"received Leave, in supervisor-table",
				"sender", NodeIDImpl::stringValueOf(sender));

			supervisorNeighborTable_.removeEntry(sender);
			supervisorStateTable_.erase(sender);
			supervisorBootstrapSet_.setInView(sender,false);
			removeSupervisor(sender);
		}
		else
		{
			neighbor = outgoingLogicalConnectRequests_.getNeighbor(sender);
			if (neighbor)
			{
				Trace_Event(this, "processIncomingHierarchyMessage",
					"received Leave, in outgoing-logical",
					"sender", NodeIDImpl::stringValueOf(sender));

				outgoingLogicalConnectRequests_.removeEntry(sender);
				supervisorBootstrapSet_.setInView(sender,false);
			}
			else
			{
				//This should not happen
				supervisorBootstrapSet_.setInView(sender,false);
				Trace_Event(this, "processIncomingHierarchyMessage",
					"Warning: received Leave but neighbor not found, ignoring");
			}
		}

		rescheduleConnectTask(0);
	}
	break;

	case SCMessage::Type_Hier_DelOp_Request_StartMembershipPush:
	{
		bool includeAttributes = buffer->readBoolean();
		int aggregationDelayMillis = buffer->readShort();
		startMembershipPush(sender, includeAttributes, aggregationDelayMillis);
	}
	break;

	case SCMessage::Type_Hier_DelOp_Request_StopMembershipPush:
	{
		stopMembershipPush(sender);
	}
	break;

	case SCMessage::Type_Hier_SupOp_Reply_ViewUpdate:
	{
		//currently nothing to do
	}
	break;

	case SCMessage::Type_Hier_PubSubBridge_BaseZoneInterest_Reply:
	{
		coreInterface_.getRoutingManager()->processIncomingDelegatePubSubBridgeControlMessage(
				inHierarchyMsg);
		//pass to D-Bridge
	}
	break;

	default:
	{
		String what("Unexpected message type: ");
		what.append(inHierarchyMsg ? inHierarchyMsg->toString() : "null");
		throw SpiderCastRuntimeError(what);
	}

	}//switch

	Trace_Exit(this,"processIncomingHierarchyMessage");
}

void HierarchyDelegate::connectTask()
{
	Trace_Entry(this, "connectTask()");

	connectTaskScheduled_ = false;

	if (isClosed())
	{
		Trace_Exit(this,"connectTask()", "closed");
		return;
	}

	if (firstConnectTask_)
	{
		firstConnectTask_ = false;
		initAttributes();
	}

	int num_connected_delegates = hierarchyViewKeeper_SPtr->getBaseZone_NumConnectedDelegates();
	bool try2connect = (num_connected_delegates < config_.getNumberOfDelegates()) ||
			(supervisorNeighborTable_.size() > 0 && num_connected_delegates <= config_.getNumberOfDelegates());

	int num_missing_supervisors = config_.getNumberOfSupervisors() - supervisorNeighborTable_.size();

	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "connectTask()", "Before connect loop");
		buffer->addProperty<int>("#connected_delegates", num_connected_delegates);
		buffer->addProperty<bool>("try2connect", try2connect);
		buffer->addProperty<int>("#missing_supervisors", num_missing_supervisors);
		buffer->addProperty<int>("#candidates", supervisorBootstrapSet_.getNumNotInView());
		buffer->addProperty<int>("#supervisors", supervisorNeighborTable_.size());
		buffer->addProperty<int>("#logical-Req", outgoingLogicalConnectRequests_.size());
		buffer->addProperty<int>("#physical-req", outgoingPhysicalConnectRequests_.size());
		buffer->addProperty<int>("#disconnect-req", disconnectRequested_.size());
		buffer->invoke();
	}

	//try to connect
	//not enough connections, and there is something we can do about it
	if (	try2connect &&
			(num_missing_supervisors > 0) &&
			(supervisorBootstrapSet_.getNumNotInView() > 0))
	{
		int max_attempts = supervisorBootstrapSet_.getNumNotInView(); //scan once

		while ( getNumSupervisorsAndRequests() < config_.getNumberOfSupervisors() &&
				max_attempts > 0)
		{
			//initiate another physical connection
			NodeIDImpl_SPtr target = supervisorBootstrapSet_.getNextNode_NotInView();
			Trace_Event(this, "connectTask()", "target-supervisor", NodeIDImpl::stringValueOf(target));
			if (target)
			{
				bool ok = commAdapter_SPtr->connect(target, HierarchyCtx); //return to incoming-msg-Q
				if (ok)
				{
					Trace_Event(this, "connectTask()", "asynch connect to supervisor initiated",
							"target", NodeIDImpl::stringValueOf(target));
					outgoingPhysicalConnectRequests_.insert(target);
					supervisorBootstrapSet_.setInView(target, true);
				}
				else
				{
					Trace_Event(this, "connectTask()", "Failed to asynch connect to supervisor",
							"target", NodeIDImpl::stringValueOf(target));
				}
				max_attempts--;
			}
			else
			{
				break; //no more nodes that are not-in-view
			}
		}

		rescheduleConnectTask(config_.getHierarchyConnectIntervalMillis());
	}
	else
	{
		Trace_Event(this,"connectTask()", "Skipped connect loop");
	}

	//try to back-off, if indeed connected
	if (num_connected_delegates > config_.getNumberOfDelegates())
	{
		if (supervisorNeighborTable_.size() > 0)
		{
			const HierarchyViewKeeper::ZoneDelegatesStateMap& map = hierarchyViewKeeper_SPtr->getZoneDelegatesStateMap();
			HierarchyViewKeeper::ZoneDelegatesStateMap::const_iterator pos = map.find(config_.getMyNodeID());
			if (pos != map.end())
			{
				int order = 1;
				while (pos != map.begin())
				{
					--pos;
					if (!(pos->second.empty()))
					{
						++order; // count connected delegates only
					}
				}

				if (ScTraceBuffer::isEventEnabled(tc_))
				{
					ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "connectTask()", "try to back-off");
					buffer->addProperty<int>("order", order);
					buffer->addProperty("ZoneDelegatesStateMap", hierarchyViewKeeper_SPtr->toStringZoneDelegatesStateMap());
					buffer->invoke();
				}

				//the delegates with the lower IDs stay connected, others back-off
				if (order > config_.getNumberOfDelegates())
				{
					disconnectFromSupervisors();
				}
			}
		}

		rescheduleConnectTask(config_.getHierarchyConnectIntervalMillis());
	}

	Trace_Exit(this, "connectTask()");
}

void HierarchyDelegate::disconnectFromSupervisors()
{
	Trace_Entry(this,"disconnectFromSupervisors()");

	//leave it in the supervisor table until the reply?

	NeighborTable::NeighborTableMap::iterator it;
	for (it = supervisorNeighborTable_._table.begin(); it != supervisorNeighborTable_._table.end(); ++it)
	{
		if (disconnectRequested_.count(it->first) == 0)
		{
			sendDisconnectRequest(it->second.neighbor);
			disconnectRequested_.insert(it->first);
		}
	}

	Trace_Exit(this,"disconnectFromSupervisors()");
}

void HierarchyDelegate::processIncomingCommEventMsg(SCMessage_SPtr incomingCommEventMsg)
{
	Trace_Entry(this,"processIncomingCommEventMsg()");

	if (isClosed())
	{
			Trace_Exit(this,"processIncomingCommEventMsg()", "closed");
			return;
	}

	boost::shared_ptr<CommEventInfo> inEvent = incomingCommEventMsg->getCommEventInfo();
	NodeIDImpl_SPtr peerName = incomingCommEventMsg->getSender();

	Trace_Event(this, "processIncomingCommEventMsg()",
			"peer", NodeIDImpl::stringValueOf(peerName),
			"event", (inEvent?inEvent->toString():"null"));

	switch (inEvent->getType())
	{

	case CommEventInfo::New_Source:
	{
		//Nothing to do, this should not happen in the delegate
	}
	break;

	case CommEventInfo::On_Break:
	{
		bool in_physical = (outgoingPhysicalConnectRequests_.count(peerName) > 0); //on-failure will follow

		bool removed_logical = outgoingLogicalConnectRequests_.removeEntry(peerName);
		bool removed_super = supervisorNeighborTable_.removeEntry(peerName);
		supervisorStateTable_.erase(peerName);

		if (removed_logical || removed_super)
		{
			if (ScTraceBuffer::isEventEnabled(tc_))
			{
				ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "processIncomingCommEventMsg()",
						"On_Break, removed entry");
				buffer->addProperty("node", NodeIDImpl::stringValueOf(peerName));
				buffer->addProperty<bool>("in_physical(not-removed)", in_physical);
				buffer->addProperty<bool>("logical", removed_logical);
				buffer->addProperty<bool>("super", removed_super);
				buffer->invoke();
			}

			supervisorBootstrapSet_.setInView(peerName,false);
			if (removed_super)
			{
				removeSupervisor(peerName);
			}

			rescheduleConnectTask(0);
		}
		else
		{
			//can happen after a leave or disconnect
			if (ScTraceBuffer::isEventEnabled(tc_))
			{
				ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "processIncomingCommEventMsg()",
						"On_Break, nothing to do");
				buffer->addProperty("node", NodeIDImpl::stringValueOf(peerName));
				buffer->addProperty<bool>("in_physical(not-removed)", in_physical);
				buffer->invoke();
			}
		}
	}
	break;

	case CommEventInfo::On_Connection_Success:
	{
		Trace_Event(this, "processIncomingCommEventMsg", "On_Connection_Success");
		Neighbor_SPtr neighbor = (*inEvent).getNeighbor();

		if (neighbor->isVirgin())
		{
			neighbor = commAdapter_SPtr->connectOnExisting(peerName);
			if (!neighbor)
			{
				Trace_Event(this, "processIncomingCommEventMsg()",
						"couldn't connect to", "node",
						peerName->getNodeName());
				// TODO: figure out what to do
				return;

			}
		}

		size_t num = outgoingPhysicalConnectRequests_.erase(peerName);
		if (num > 0)
		{
			bool ok = sendConnectRequest(neighbor);
			if (ok)
			{
				outgoingLogicalConnectRequests_.addEntry(peerName,neighbor);
			}
			else
			{
				commAdapter_SPtr->disconnect(neighbor);
				supervisorBootstrapSet_.setInView(peerName,false);
			}
		}
		else
		{
			// should not happen, FAIL FAST
			Trace_Event( this, "processIncomingCommEventMsg()",
					"On_Connection_Success, new neighbor has no match in outgoingPhysicalConnectRequests_",
					"peer", spdr::toString(neighbor));
			std::ostringstream oss;
			oss << "HierarchyDelegate.processIncomingCommEventMsg(), On_Connection_Success, "
					<< " new neighbor has no match in outgoingPhysicalConnectRequests_: "
					<< spdr::toString(neighbor);
			throw SpiderCastRuntimeError(oss.str());
		}
	}
	break;

	case CommEventInfo::On_Connection_Failure:
	{
		size_t num = outgoingPhysicalConnectRequests_.erase(peerName);
		if (num > 0)
		{
			supervisorBootstrapSet_.setInView(peerName,false);

		}
		else
		{
			// this may happen if the async connection attempt fails immediately
			Trace_Debug( this, "processIncomingCommEventMsg()",
					"On_Connection_Failure, failed connection target has no match in outgoingPhysicalConnectRequests_",
					"peer", NodeIDImpl::stringValueOf(peerName));
		}

		rescheduleConnectTask(0);
	}
	break;

	default:
	{
		Trace_Event( this, "processIncomingCommEventMsg()","Unexpected event type");
		String what("Unexpected event type");
		throw SpiderCastRuntimeError(what);
	}

	} //switch

	Trace_Exit(this,"processIncomingCommEventMsg()");
}

bool HierarchyDelegate::sendConnectRequest(Neighbor_SPtr target)
{
	outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_Connect_Request);
	ByteBuffer_SPtr buffer = outgoingHierMessage_->getBuffer();
	buffer->writeString(config_.getNodeName());
	buffer->writeString(target->getName());
	outgoingHierMessage_->updateTotalLength();
	if (config_.isCRCMemTopoMsgEnabled())
	{
		outgoingHierMessage_->writeCRCchecksum();
	}

	bool ok = (target->sendMessage(outgoingHierMessage_) == 0);

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
			ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,"sendConnectRequest");
			buffer->addProperty("target", target->getName());
			buffer->addProperty<bool>("ok", ok);
			buffer->invoke();
	}

	return ok;
}

bool HierarchyDelegate::sendDisconnectRequest(Neighbor_SPtr target)
{
	outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_Disconnect_Request);
	ByteBuffer_SPtr buffer = outgoingHierMessage_->getBuffer();
	buffer->writeString(config_.getNodeName());
	buffer->writeString(target->getName());
	outgoingHierMessage_->updateTotalLength();
	if (config_.isCRCMemTopoMsgEnabled())
	{
		outgoingHierMessage_->writeCRCchecksum();
	}

	int ok = (target->sendMessage(outgoingHierMessage_) == 0);

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,"sendDisconnectRequest");
		buffer->addProperty("target", target->getName());
		buffer->addProperty<bool>("ok", ok);
		buffer->invoke();
	}

	return ok;
}

bool HierarchyDelegate::sendDisconnectReply(Neighbor_SPtr target, bool accept)
{
	outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_Disconnect_Reply);
	ByteBuffer_SPtr buffer = outgoingHierMessage_->getBuffer();
	buffer->writeString(config_.getNodeName());
	buffer->writeString(target->getName());
	buffer->writeShort( (accept ? (int16_t)HierarchyUtils::ACCEPT : (int16_t)HierarchyUtils::REJECT) );
	outgoingHierMessage_->updateTotalLength();
	if (config_.isCRCMemTopoMsgEnabled())
	{
		outgoingHierMessage_->writeCRCchecksum();
	}

	int ok = (target->sendMessage(outgoingHierMessage_) == 0);

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,"sendDisconnectReply");
		buffer->addProperty("target", target->getName());
		buffer->addProperty<bool>("accept", accept);
		buffer->addProperty<bool>("ok", ok);
		buffer->invoke();
	}

	return ok;
}

bool HierarchyDelegate::sendLeave(Neighbor_SPtr target)
{
	outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_Leave);
	ByteBuffer_SPtr buffer = outgoingHierMessage_->getBuffer();
	buffer->writeString(config_.getNodeName());
	buffer->writeString(target->getName());
	outgoingHierMessage_->updateTotalLength();
	if (config_.isCRCMemTopoMsgEnabled())
	{
		outgoingHierMessage_->writeCRCchecksum();
	}

	int ok = (target->sendMessage(outgoingHierMessage_) == 0);

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,"sendLeave");
		buffer->addProperty("target", target->getName());
		buffer->addProperty<bool>("ok", ok);
		buffer->invoke();
	}

	return ok;
}

bool HierarchyDelegate::sendReply_StartMembershipPush(Neighbor_SPtr target, bool accept)
{
	outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_DelOp_Reply_StartMembershipPush);
	ByteBuffer_SPtr buffer = outgoingHierMessage_->getBuffer();
	buffer->writeString(config_.getNodeName());
	buffer->writeString(target->getName());
	buffer->writeBoolean(accept);
	outgoingHierMessage_->updateTotalLength();
	if (config_.isCRCMemTopoMsgEnabled())
	{
		outgoingHierMessage_->writeCRCchecksum();
	}

	int ok = (target->sendMessage(outgoingHierMessage_) == 0);

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,"sendReply_StartMembershipPush");
		buffer->addProperty("target", target->getName());
		buffer->addProperty<bool>("accept", accept);
		buffer->addProperty<bool>("ok", ok);
		buffer->invoke();
	}

	return ok;
}

bool HierarchyDelegate::sendReply_StopMembershipPush(Neighbor_SPtr target, bool accept)
{
	outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_DelOp_Reply_StopMembershipPush);
	ByteBuffer_SPtr buffer = outgoingHierMessage_->getBuffer();
	buffer->writeString(config_.getNodeName());
	buffer->writeString(target->getName());
	buffer->writeBoolean(accept);
	outgoingHierMessage_->updateTotalLength();
	if (config_.isCRCMemTopoMsgEnabled())
	{
		outgoingHierMessage_->writeCRCchecksum();
	}

	int ok = (target->sendMessage(outgoingHierMessage_) == 0);

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,"sendReply_StopMembershipPush");
		buffer->addProperty("target", target->getName());
		buffer->addProperty<bool>("accept", accept);
		buffer->addProperty<bool>("ok", ok);
		buffer->invoke();
	}

	return ok;
}

int HierarchyDelegate::getNumSupervisorsAndRequests()
{
	Trace_Entry(this, "getNumSupervisorsAndRequests()");
	int count = outgoingLogicalConnectRequests_.size()
			+ outgoingPhysicalConnectRequests_.size() + supervisorNeighborTable_.size();
	Trace_Exit<int> (this, "getNumSupervisorsAndRequests()", count);
	return count;
}

void HierarchyDelegate::addSupervisor(NodeIDImpl_SPtr supervisor)
{
	Trace_Entry(this,"addSupervisor()",
		"node", NodeIDImpl::stringValueOf(supervisor));

	//updateAttributes
	AttributeControl& attrCtrl = coreInterface_.getMembershipManager()->getAttributeControl();

	if (supervisorNeighborTable_.size() == 1) //first supervisor
	{
		//state
		char connected = (char) HierarchyUtils::delegateState_Connected;
		attrCtrl.setAttribute(HierarchyUtils::delegateState_AttributeKey, std::make_pair(1, &connected));
	}

	//key: prefix + {name}
	String key(HierarchyUtils::delegateSupervisor_AttributeKeyPrefix);
	key.append(supervisor->getNodeName());
	//value: <is-active, node-id>
	attrValBuffer_->reset();
	//outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_Leave); //Bogus, just using the buffer
	//ByteBuffer_SPtr buff = outgoingHierMessage_->getBuffer();
	attrValBuffer_->writeBoolean(false);
	attrValBuffer_->writeNodeID(supervisor);
	std::pair<const int, const char*> value = std::make_pair(
			(int)attrValBuffer_->getPosition(),
			(const char*)attrValBuffer_->getBuffer());

	attrCtrl.setAttribute(key, value);

	Trace_Exit(this,"addSupervisor()");
}

void HierarchyDelegate::removeSupervisor(NodeIDImpl_SPtr supervisor)
{
	Trace_Entry(this,"removeSupervisor()",
		"node", NodeIDImpl::stringValueOf(supervisor));

	//updateAttributes
	AttributeControl& attrCtrl = coreInterface_.getMembershipManager()->getAttributeControl();

	if (supervisorNeighborTable_.size() == 0) //last supervisor
	{
		//state
		char connected = (char) HierarchyUtils::delegateState_Candidate;
		attrCtrl.setAttribute(HierarchyUtils::delegateState_AttributeKey, std::make_pair(1, &connected));
	}

	//key: prefix + {name}
	String key(HierarchyUtils::delegateSupervisor_AttributeKeyPrefix);
	key.append(supervisor->getNodeName());

	attrCtrl.removeAttribute(key);

	Trace_Exit(this,"removeSupervisor()");
}

void HierarchyDelegate::updateSupervisorActive(NodeIDImpl_SPtr supervisor, bool active)
{
	Trace_Entry(this,"updateSupervisorActive()",
			"node", NodeIDImpl::stringValueOf(supervisor));

	//updateAttributes
	AttributeControl& attrCtrl = coreInterface_.getMembershipManager()->getAttributeControl();

	//key: prefix + {name}
	String key(HierarchyUtils::delegateSupervisor_AttributeKeyPrefix);
	key.append(supervisor->getNodeName());
	//value: <is-active, node-id>
	attrValBuffer_->reset();
	attrValBuffer_->writeBoolean(active);
	attrValBuffer_->writeNodeID(supervisor);
	std::pair<const int, const char*> value = std::make_pair(
			(int)attrValBuffer_->getPosition(),
			(const char*)attrValBuffer_->getBuffer());

	attrCtrl.setAttribute(key, value);

	Trace_Exit(this,"updateSupervisorActive()");
}

int HierarchyDelegate::getNumActiveSupervisors()
{
	int k=0;
	for (SuprvisorStateMap::const_iterator it = supervisorStateTable_.begin();
			it != supervisorStateTable_.end(); ++it)
	{
		if (it->second.get<0>())
		{
			k++;
		}
	}
	return k;
}

void HierarchyDelegate::initAttributes()
{
	Trace_Debug(this,"initAttributes()","initialize delegate attributes");

	AttributeControl& attrCtrl = coreInterface_.getMembershipManager()->getAttributeControl();

	if (supervisorBootstrapSet_.size() > 0) //is candidate
	{
		if (supervisorNeighborTable_.size() == 0)
		{
			//state
			char cand = (char) HierarchyUtils::delegateState_Candidate;
			attrCtrl.setAttribute(HierarchyUtils::delegateState_AttributeKey, std::make_pair(1, &cand));
		}
		else
		{
			//state
			char connected = (char) HierarchyUtils::delegateState_Connected;
			attrCtrl.setAttribute(HierarchyUtils::delegateState_AttributeKey, std::make_pair(1, &connected));
		}
	}
	else
	{
		attrCtrl.removeAttribute(HierarchyUtils::delegateState_AttributeKey);
	}
}

void HierarchyDelegate::quarantineSupervisorCandidate(NodeIDImpl_SPtr peer)
{
	quarantineConnectTargets_.insert(peer);
	taskSchedule_SPtr->scheduleDelay( AbstractTask_SPtr(
			new HierarchyDelegateUnquarantineTask(instID_,*this,peer)),
			boost::posix_time::milliseconds( config_.getHierarchyQuarantineIntervalMillis()));

	Trace_Debug(this,"quarantineSupervisorCandidate()","",
				"peer", NodeIDImpl::stringValueOf(peer));
}

void HierarchyDelegate::unquarantineSupervisorCandidate(NodeIDImpl_SPtr peer)
{
	if (isClosed())
	{
			Trace_Exit(this,"unquarantineSupervisorCandidate()", "closed");
			return;
	}

	quarantineConnectTargets_.erase(peer);
	supervisorBootstrapSet_.setInView(peer, false);

	Trace_Debug(this,"unquarantineSupervisorCandidate()","",
				"peer", NodeIDImpl::stringValueOf(peer));

	rescheduleConnectTask(0);
}

void HierarchyDelegate::pubsubBridgeTask()
{
	Trace_Entry(this,"pubsubBridgeTask()");

	if (isClosed())
	{
		Trace_Exit(this,"pubsubBridgeTask()", "closed");
		return;
	}

	pubsubBridgeTaskScheduled_ = false;

	//check if I am a chosen bridge
	//if chosen bridge, check whether new => create and activate bridge
	//if existing, check if current target is active or need to update target
	//if stopped being a chosen bridge, stop it

	NodeIDImpl_Set set = hierarchyViewKeeper_SPtr->getBaseZone_ActiveDelegates();
	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this,"pubsubBridgeTask()");
		std::ostringstream oss;
		for (NodeIDImpl_Set::const_iterator it = set.begin(); it != set.end(); ++it)
		{
			oss << (*it)->getNodeName() << " ";
		}
		buffer->addProperty("active-delegates",oss.str());
		buffer->invoke();
	}

	if (!set.empty())
	{
		//delegate with lowest NodeID is the D-Bridge
		if ((*set.begin())->getNodeName() == config_.getMyNodeID()->getNodeName() )
		{
			Trace_Debug(this, "pubsubBridgeTask()", "chosen D-Bridge");

			if (pubsubBridge_target_.first)
			{
				SuprvisorStateMap::const_iterator pos = supervisorStateTable_.find(pubsubBridge_target_.first);
				if (pos != supervisorStateTable_.end() && pos->second.get<0>())
				{
					//keep same target
					Trace_Debug(this, "pubsubBridgeTask()", "existing D-Bridge, keep same target");
				}
				else
				{
					//choose a new target
					pubsubBridge_target_ = chooseActiveSupervisor();
					coreInterface_.getRoutingManager()->startDelegatePubSubBridge(pubsubBridge_target_.second);
					Trace_Debug(this, "pubsubBridgeTask()", "update D-Bridge",
							"target", spdr::toString<NodeIDImpl>(pubsubBridge_target_.first));
				}
			}
			else
			{
				pubsubBridge_target_ = chooseActiveSupervisor();
				coreInterface_.getRoutingManager()->startDelegatePubSubBridge(pubsubBridge_target_.second);
				Trace_Debug(this, "pubsubBridgeTask()", "create D-Bridge",
						"target", spdr::toString<NodeIDImpl>(pubsubBridge_target_.first));
			}
		}
		else
		{
			if (pubsubBridge_target_.first)
			{
				Trace_Debug(this, "pubsubBridgeTask()", "not chosen D-Bridge, stop D-Bridge");
				coreInterface_.getRoutingManager()->stopDelegatePubSubBridge();
				pubsubBridge_target_.first.reset();
				pubsubBridge_target_.second.reset();
			}
			else
			{
				Trace_Debug(this, "pubsubBridgeTask()", "not chosen D-Bridge, no D-Bridge");
			}
		}
	}
	else
	{
		if (pubsubBridge_target_.first)
		{
			Trace_Debug(this, "pubsubBridgeTask()", "no active delegates, stop D-Bridge");
			coreInterface_.getRoutingManager()->stopDelegatePubSubBridge();
			pubsubBridge_target_.first.reset();
			pubsubBridge_target_.second.reset();

		}
	}

	Trace_Exit(this,"pubsubBridgeTask()");
}

std::pair<NodeIDImpl_SPtr,Neighbor_SPtr> HierarchyDelegate::chooseActiveSupervisor()
{
	Trace_Entry(this,"chooseActiveSupervisor()");
	std::pair<NodeIDImpl_SPtr,Neighbor_SPtr> target;

	for (SuprvisorStateMap::const_iterator it = supervisorStateTable_.begin();
			it != supervisorStateTable_.end(); ++it)
	{
		//first active from map.
		//TODO randomize selection
		if (it->second.get<0>())
		{
			target.first = it->first;
			target.second = supervisorNeighborTable_.getNeighbor(it->first);
			break;
		}
	}

	Trace_Debug(this,"chooseActiveSupervisor()", "",
			"ID", spdr::toString<NodeIDImpl>(target.first),
			"neighbor", spdr::toString(target.second));

	Trace_Exit(this,"chooseActiveSupervisor()");
	return target;
}

void HierarchyDelegate::supervisorViewUpdate()
{
	Trace_Entry(this,"supervisorViewUpdate()");

	viewUpdateTaskScheduled_ = false;

	if (isClosed())
	{
		Trace_Exit(this,"supervisorViewUpdate()", "closed");
		return;
	}

	int num_active = getNumActiveSupervisors();
	if (!hierarchyViewKeeper_SPtr->isMembershipPushActive() && num_active > 0)
	{
		hierarchyViewKeeper_SPtr->setMembershipPushActive(true);
	}
	else if (hierarchyViewKeeper_SPtr->isMembershipPushActive() && num_active == 0)
	{
		hierarchyViewKeeper_SPtr->setMembershipPushActive(false);
	}

	if (num_active >0)
	{
		// 1) existing supervisors
		// 1a) send updates, with attributes
		bool msg_ready = false;
		int num_updates = 0;
		for (SuprvisorStateMap::const_iterator it = supervisorStateTable_.begin();
				it != supervisorStateTable_.end(); ++it)
		{
			if (it->second.get<0>() && it->second.get<1>() && it->second.get<2>()) //active, with attributes, first view delivered
			{
				if (!msg_ready)
				{
					outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_SupOp_Request_ViewUpdate);
					num_updates = hierarchyViewKeeper_SPtr->writeMembershipPushQ(
							outgoingHierMessage_, true, (num_active == 1)); //clear Q only if the only active supervisor
					outgoingHierMessage_->updateTotalLength();
					if (config_.isCRCMemTopoMsgEnabled())
					{
						outgoingHierMessage_->writeCRCchecksum();
					}
					msg_ready = true;
					Trace_Debug(this,"supervisorViewUpdate()", "prepared a differential ViewUpdate, with attributes");
				}

				if (num_updates>0)
				{
					Neighbor_SPtr neighbor = supervisorNeighborTable_.getNeighbor(it->first);
					if (neighbor)
					{
						neighbor->sendMessage(outgoingHierMessage_);
					}
					else
					{
						Trace_Event(this, "supervisorViewUpdate", "Error: supervisor state and neighbor table inconsistent.");
						throw SpiderCastRuntimeError("Error: supervisor state and neighbor table inconsistent.");
					}
				}
			}
		}

		// 1b) send updates, without attributes
		msg_ready = false;
		num_updates = 0;
		for (SuprvisorStateMap::const_iterator it = supervisorStateTable_.begin();
				it != supervisorStateTable_.end(); ++it)
		{
			if (it->second.get<0>() && !it->second.get<1>() && it->second.get<2>()) //active, without attributes, first view delivered
			{
				if (!msg_ready)
				{
					outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_SupOp_Request_ViewUpdate);
					num_updates = hierarchyViewKeeper_SPtr->writeMembershipPushQ(
							outgoingHierMessage_, false, true); //clear Q
					outgoingHierMessage_->updateTotalLength();
					if (config_.isCRCMemTopoMsgEnabled())
					{
						outgoingHierMessage_->writeCRCchecksum();
					}
					msg_ready = true;
					Trace_Debug(this,"supervisorViewUpdate()", "prepared a differential ViewUpdate, without attributes");
				}

				if (num_updates>0)
				{
					Neighbor_SPtr neighbor = supervisorNeighborTable_.getNeighbor(it->first);
					if (neighbor)
					{
						neighbor->sendMessage(outgoingHierMessage_);
					}
					else
					{
						Trace_Event(this, "supervisorViewUpdate", "Error: supervisor state and neighbor table inconsistent.");
						throw SpiderCastRuntimeError("Error: supervisor state and neighbor table inconsistent.");
					}
				}
			}
		}

		// 2) new supervisors
		// 2a) send full view, with attributes
		msg_ready = false;
		for (SuprvisorStateMap::iterator it = supervisorStateTable_.begin();
				it != supervisorStateTable_.end(); ++it)
		{
			if (it->second.get<0>() && it->second.get<1>() && !it->second.get<2>()) //active, with attributes, first view not delivered
			{
				if (!msg_ready)
				{
					outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_SupOp_Request_ViewUpdate);
					outgoingHierMessage_->getBuffer()->writeLong(hierarchyViewKeeper_SPtr->getMembershipPushViewID());
					outgoingHierMessage_->getBuffer()->writeInt(1); //one view change
					outgoingHierMessage_->getBuffer()->writeChar((char)SCMembershipEvent::View_Change);
					membershipManager_SPtr->getDelegateFullView(outgoingHierMessage_,true);
					outgoingHierMessage_->updateTotalLength();
					if (config_.isCRCMemTopoMsgEnabled())
					{
						outgoingHierMessage_->writeCRCchecksum();
					}
					msg_ready = true;
					Trace_Debug(this,"supervisorViewUpdate()", "prepared a full ViewUpdate, with attributes");
				}

				Neighbor_SPtr neighbor = supervisorNeighborTable_.getNeighbor(it->first);
				if (neighbor)
				{
					neighbor->sendMessage(outgoingHierMessage_);
				}
				else
				{
					Trace_Event(this, "supervisorViewUpdate", "Error: supervisor state and neighbor table inconsistent.");
					throw SpiderCastRuntimeError("Error: supervisor state and neighbor table inconsistent.");
				}
				it->second.get<2>() = true; //first view delivered
			}
		}

		// 2b) send full view, without attributes
		msg_ready = false;
		for (SuprvisorStateMap::iterator it = supervisorStateTable_.begin();
				it != supervisorStateTable_.end(); ++it)
		{
			if (it->second.get<0>() && !it->second.get<1>() && !it->second.get<2>()) //active, without attributes, first view not delivered
			{
				if (!msg_ready)
				{
					outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_SupOp_Request_ViewUpdate);
					outgoingHierMessage_->getBuffer()->writeLong(hierarchyViewKeeper_SPtr->getMembershipPushViewID());
					outgoingHierMessage_->getBuffer()->writeInt(1); //one view change
					outgoingHierMessage_->getBuffer()->writeChar((char)SCMembershipEvent::View_Change);
					membershipManager_SPtr->getDelegateFullView(outgoingHierMessage_,false);
					outgoingHierMessage_->updateTotalLength();
					if (config_.isCRCMemTopoMsgEnabled())
					{
						outgoingHierMessage_->writeCRCchecksum();
					}
					msg_ready = true;
					Trace_Debug(this,"supervisorViewUpdate()", "prepared a full ViewUpdate, without attributes");
				}

				Neighbor_SPtr neighbor = supervisorNeighborTable_.getNeighbor(it->first);
				if (neighbor)
				{
					neighbor->sendMessage(outgoingHierMessage_);
				}
				else
				{
					Trace_Event(this, "supervisorViewUpdate", "Error: supervisor state and neighbor table inconsistent.");
					throw SpiderCastRuntimeError("Error: supervisor state and neighbor table inconsistent.");
				}
				it->second.get<2>() = true; //first view delivered
			}
		}
	}

	Trace_Exit(this,"supervisorViewUpdate()");
}

void HierarchyDelegate::globalViewChanged()
{
	if (getNumActiveSupervisors() > 0)
	{
		rescheduleViewUpdateTask();
	}
}

void HierarchyDelegate::rescheduleViewUpdateTask()
{
	if (!viewUpdateTaskScheduled_)
	{
		taskSchedule_SPtr->scheduleDelay(viewUpdateTask_SPtr,
				boost::posix_time::milliseconds(
						config_.getHierarchyMemberhipUpdateAggregationInterval()));
		viewUpdateTaskScheduled_ = true;
		Trace_Debug(this,"rescheduleViewUpdateTask()","rescheduled task");
	}
	else
	{
		Trace_Debug(this,"rescheduleViewUpdateTask()","task already scheduled");
	}
}

void HierarchyDelegate::rescheduleConnectTask(int delayMillis)
{
	if (!connectTaskScheduled_)
	{
		taskSchedule_SPtr->scheduleDelay(connectTask_SPtr,
				boost::posix_time::milliseconds(delayMillis));
		connectTaskScheduled_ = true;
		Trace_Debug(this,"rescheduleConnectTask()","rescheduled");
	}
	else
	{
		Trace_Debug(this,"rescheduleConnectTask()","already scheduled");
	}
}

void HierarchyDelegate::reschedulePubSubBridgeTask(int delayMillis)
{
	if (!pubsubBridgeTaskScheduled_)
	{
		taskSchedule_SPtr->scheduleDelay(pubsubBridgeTask_SPtr,
				boost::posix_time::milliseconds(delayMillis));
		pubsubBridgeTaskScheduled_ = true;
		Trace_Debug(this,"reschedulePubSubBridgeTask()","rescheduled");
	}
	else
	{
		Trace_Debug(this,"reschedulePubSubBridgeTask()","already scheduled");
	}
}

void HierarchyDelegate::startMembershipPush(NodeIDImpl_SPtr supervisorID, bool includeAttributes, int aggregationDelayMillis)
{
	Trace_Entry(this, "startMembershipPush");
	Neighbor_SPtr supervisor = supervisorNeighborTable_.getNeighbor(supervisorID);
	if (supervisor)
	{
		bool ok = sendReply_StartMembershipPush(supervisor, true);
		if (ok)
		{
			SuprvisorStateMap::iterator pos = supervisorStateTable_.find(supervisorID);
			if (pos != supervisorStateTable_.end())
			{
				pos->second.get<0>() = true; //active
				pos->second.get<1>() = includeAttributes;
				pos->second.get<2>() = false; //first view delivered
			}
			else
			{
				Trace_Event(this, "startMembershipPush",
						"Error: supervisor state and neighbor table inconsistent.");
				throw SpiderCastRuntimeError("Error: supervisor state and neighbor table inconsistent.");
			}
			updateSupervisorActive(supervisorID,true); //change attributes
			rescheduleViewUpdateTask();
			if (config_.isRoutingEnabled())
			{
				reschedulePubSubBridgeTask(0);
			}
		}
		else
		{
			Trace_Event(this, "startMembershipPush","Warning: send failed, nothing to do");
		}
	}
	else
	{
		Trace_Event(this, "startMembershipPush",
				"Warning: received StartMembershipPush but neighbor not found, ignoring");
	}
	Trace_Exit(this, "startMembershipPush");
}

void HierarchyDelegate::stopMembershipPush(NodeIDImpl_SPtr supervisorID)
{

	Neighbor_SPtr supervisor = supervisorNeighborTable_.getNeighbor(supervisorID);
	if (supervisor)
	{
		bool ok = sendReply_StopMembershipPush(supervisor, true);
		if (!ok)
		{
			Trace_Event(this, "stopMembershipPush","Warning: send reply failed, stopping anyway");
		}

		SuprvisorStateMap::iterator pos = supervisorStateTable_.find(supervisorID);
		if (pos != supervisorStateTable_.end())
		{
			pos->second.get<0>() = false; //active
			pos->second.get<1>() = false; //include attributes
			pos->second.get<2>() = false; //first view delivered
		}
		else
		{
			Trace_Event(this, "stopMembershipPush",
					"Error: supervisor state and neighbor table inconsistent.");
			throw SpiderCastRuntimeError("Error: supervisor state and neighbor table inconsistent.");
		}
		updateSupervisorActive(supervisorID,false); //change attributes
		rescheduleViewUpdateTask();
		if (config_.isRoutingEnabled())
		{
			reschedulePubSubBridgeTask(0);
		}
	}
	else
	{
		Trace_Event(this, "stopMembershipPush",
				"Warning: received StopMembershipPush but neighbor not found, ignoring");
	}
}

}
