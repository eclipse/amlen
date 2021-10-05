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
 * RoutingManagerImpl.cpp
 *
 *  Created on: Nov 7, 2011
 */

#include "RoutingManagerImpl.h"

namespace spdr
{

namespace route
{

ScTraceComponent* RoutingManagerImpl::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Route,
		trace::ScTrConstants::Layer_ID_Route, "RoutingManagerImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

RoutingManagerImpl::RoutingManagerImpl(const String& instID,
		const SpiderCastConfigImpl& config,
		NodeIDCache& nodeIDCache,
		CoreInterface& coreInterface,
		VirtualIDCache& vidCache,
		IncomingMsgQ_SPtr incomingMsgQ) :
	ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
	instID_(instID),
	config_(config),
	nodeIDCache_(nodeIDCache),
	coreInterface_(coreInterface),
	routingTable_(instID,config, vidCache, config_.isDebugFailFastEnabled()),
	p2pRouter_(),
	broadcastRouter_(instID,config, coreInterface, vidCache, routingTable_, *this),
	pubsubViewKeeper_(new PubSubViewKeeper(instID,config,vidCache,*this)),
	pubsubRouter_(instID,config, coreInterface,vidCache, routingTable_, *this, pubsubViewKeeper_),
	routingWorkMutex_(),
	routingWorkCondVar_(),
	routingWorkPending_(true),
	closed_(false),
	incomingMsgQ_SPtr(),
	msgsChunkSize(100),
	pubsubBridgeMutex_(),
	supPubSubBridge_(),
	delPubSubBridge_(),
	_receiver()
{
	Trace_Entry(this, "RoutingManagerImpl()");
}

RoutingManagerImpl::~RoutingManagerImpl()
{
	Trace_Entry(this, "~RoutingManagerImpl()");
}

void RoutingManagerImpl::init()
{
	Trace_Entry(this, "init()");

	incomingMsgQ_SPtr = coreInterface_.getCommAdapter()->getIncomingMessageQ();
	MembershipManager_SPtr memMngr_SPtr =
			coreInterface_.getMembershipManager();
	memMngr_SPtr->registerInternalMembershipConsumer(
			boost::static_pointer_cast<SCMembershipListener>(pubsubViewKeeper_),
			MembershipManager::IntMemConsumer_PubSub);

	broadcastRouter_.init();
	pubsubRouter_.init();

	if (config_.isHierarchyEnabled())
	{
		if (coreInterface_.getHierarchyManager()->isManagementZone())
		{
			supPubSubBridge_.reset( new SupervisorPubSubBridge(
					instID_, config_, pubsubViewKeeper_,
					coreInterface_.getMembershipManager()->getAttributeControl()));
		}
	}

	Trace_Exit(this, "init()");
}

void RoutingManagerImpl::destroyCrossRefs()
{
	Trace_Entry(this, "destroyCrossRefs()");

	incomingMsgQ_SPtr.reset();
	supPubSubBridge_.reset();
	delPubSubBridge_.reset();

	Trace_Exit(this, "destroyCrossRefs()");
}

//void RoutingManagerImpl::start()
//{
//	Trace_Entry(this, "start()");
//
//
//
//	Trace_Exit(this, "start()");
//}

void RoutingManagerImpl::terminate(bool soft)
{
	Trace_Entry(this, "terminate()");

	{
		boost::recursive_mutex::scoped_lock lock(routingWorkMutex_);
		closed_ = true;
	}


	routingWorkCondVar_.notify_all();

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);

		if (supPubSubBridge_)
		{
			supPubSubBridge_->close();
		}

		if (delPubSubBridge_)
		{
			delPubSubBridge_->close(true);
		}
	}



	Trace_Exit(this, "terminate()");
}

/*
 * @see: RoutingManager
 */
void RoutingManagerImpl::addRoutingNeighbor(NodeIDImpl_SPtr id,
		Neighbor_SPtr neighbor)
{
	Trace_Entry(this, "addRoutingNeighbor()");

	routingTable_.addRoutingNeighbor(id, neighbor);

	Trace_Exit(this, "addRoutingNeighbor()");
}

/*
 * @see: RoutingManager
 */
void RoutingManagerImpl::removeRoutingNeighbor(NodeIDImpl_SPtr id,
		Neighbor_SPtr neighbor)
{
	Trace_Entry(this, "removeRoutingNeighbor()");

	routingTable_.removeRoutingNeighbor(id, neighbor);

	Trace_Exit(this, "removeRoutingNeighbor()");
}

void RoutingManagerImpl::setP2PRcv(messaging::P2PStreamRcvImpl_SPtr receiver)
{
	Trace_Entry(this, "setP2PRcv()");

	_receiver = receiver;

	Trace_Exit(this, "setP2PRcv()");
}

/*
 * @see: RoutingManager
 */
P2PRouter& RoutingManagerImpl::getP2PRouter()
{
	return p2pRouter_;
}

/*
 * @see: RoutingManager
 */
BroadcastRouter& RoutingManagerImpl::getBroadcastRouter()
{
	return broadcastRouter_;
}

/*
 * @see: RoutingManager
 */
PubSubRouter& RoutingManagerImpl::getPubSubRouter()
{
	return pubsubRouter_;
}

/*
 * @see: RoutingManager
 */
void RoutingManagerImpl::startDelegatePubSubBridge(Neighbor_SPtr targetSupervisor)
{
	Trace_Entry(this,"startDelegatePubSubBridge",
			"target", spdr::toString(targetSupervisor));

	if (targetSupervisor)
	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);

		if (delPubSubBridge_)
		{
			Trace_Debug(this,"startDelegatePubSubBridge","DBridge exists, setting new target");
			delPubSubBridge_->setTargetSupervisor(targetSupervisor);
		}
		else
		{
			Trace_Debug(this,"startDelegatePubSubBridge","new DBridge");
			delPubSubBridge_.reset( new DelegatePubSubBridge(
					instID_, config_,
					pubsubViewKeeper_,
					coreInterface_));
			delPubSubBridge_->setTargetSupervisor(targetSupervisor);
		}

		delPubSubBridge_->init();
	}

	Trace_Exit(this,"startDelegatePubSubBridge");
}

/*
 * @see: RoutingManager
 */
void RoutingManagerImpl::stopDelegatePubSubBridge()
{
	Trace_Entry(this,"stopDelegatePubSubBridge");

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);

		if (delPubSubBridge_)
		{
			delPubSubBridge_->close(false);
			delPubSubBridge_.reset();
		}
	}

	Trace_Exit(this,"stopDelegatePubSubBridge");
}

/*
 * @see: RoutingManager
 */
void RoutingManagerImpl::supervisorPubSubBridge_add_active(
		const String& busName,
		NodeIDImpl_SPtr id,
		Neighbor_SPtr neighbor)
{
	Trace_Entry(this, "supervisorPubSubBridge_add_active()");

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);
		if (supPubSubBridge_)
		{
			supPubSubBridge_->add_active(busName,id,neighbor);
		}
	}

	Trace_Exit(this, "supervisorPubSubBridge_add_active()");
}


/*
 * @see: RoutingManager
 */
void RoutingManagerImpl::supervisorPubSubBridge_remove_active(
		BusName_SPtr bus,
		NodeIDImpl_SPtr id)
{
	Trace_Entry(this, "supervisorPubSubBridge_remove_active()");

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);
		if (supPubSubBridge_)
		{
			supPubSubBridge_->remove_active(bus,id);
		}
	}

	Trace_Exit(this, "supervisorPubSubBridge_remove_active()");
}

void RoutingManagerImpl::processIncomingDelegatePubSubBridgeControlMessage(SCMessage_SPtr msg)
{
	Trace_Entry(this, "processIncomingDelegatePubSubBridgeControlMessage()");

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);
		if (delPubSubBridge_)
		{
			delPubSubBridge_->processIncomingControlMessage(msg);
		}
		else
		{
			Trace_Event(this, "processIncomingDelegatePubSubBridgeControlMessage()","No D-Bridge");
		}
	}

	Trace_Exit(this, "processIncomingDelegatePubSubBridgeControlMessage()");
}

void RoutingManagerImpl::processIncomingSupervisorPubSubBridgeControlMessage(SCMessage_SPtr msg)
{
	Trace_Entry(this, "processIncomingSupervisorPubSubBridgeControlMessage()");

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);
		if (supPubSubBridge_)
		{
			supPubSubBridge_->processIncomingControlMessage(msg);
		}
		else
		{
			Trace_Event(this, "processIncomingSupervisorPubSubBridgeControlMessage()","No S-Bridge");
		}
	}

	Trace_Exit(this, "processIncomingSupervisorPubSubBridgeControlMessage()");
}

/*
 * @see RoutingTask
 */
bool RoutingManagerImpl::runRoutingTask(bool workPending)
{
	Trace_Entry(this, "runRoutingTask()", "workPending", workPending?"true":"false");

	{
		boost::recursive_mutex::scoped_lock lock(routingWorkMutex_);

		if ((routingWorkPending_==(uint32_t)0) && !workPending)
		{
			try
			{
				routingWorkCondVar_.timed_wait(lock, wait_millis);
			}
			catch (boost::thread_interrupted& ex)
			{
				Trace_Debug(this, "operator()()", "Interrupted", "id",
						ScTraceBuffer::stringValueOf<boost::thread::id>(boost::this_thread::get_id()));
			}
		}

		routingWorkPending_ = (uint32_t)0;
	}

	//process a batch of messages
	bool partialQueueProcessing = processIncomingDataMessages();

	Trace_Exit<bool>(this, "runRoutingTask()", partialQueueProcessing);
	return partialQueueProcessing;
}



bool RoutingManagerImpl::processIncomingDataMessages()
{

	Trace_Entry(this, "processIncomingDataMessages()");

	bool partial = false;

	if (incomingMsgQ_SPtr)
	{
		size_t q_size = incomingMsgQ_SPtr->getQSize(IncomingMsgQ::DataQ);
		partial = (q_size > (size_t)msgsChunkSize);
		int num_msgs_to_process = partial ? msgsChunkSize : (int)q_size ;

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr tb = ScTraceBuffer::debug(this,"processIncomingDataMessages()");
			tb->addProperty<size_t>("Q-size",q_size);
			tb->addProperty<int>("num_msgs_to_process", num_msgs_to_process);
			tb->invoke();
		}

		while (num_msgs_to_process > 0)
		{
			--num_msgs_to_process;
			SCMessage_SPtr msg_sptr = incomingMsgQ_SPtr->pollQ(IncomingMsgQ::DataQ);
			if (msg_sptr)
			{
				processIncomingDataMessage(msg_sptr);
			}
		}
	}

	Trace_Exit<bool>(this, "processIncomingDataMessages()",partial);
	return partial;
}

void RoutingManagerImpl::processIncomingDataMessage(SCMessage_SPtr message)
{
	Trace_Entry(this, "processIncomingDataMessage()");

	SCMessage::H1Header h1 = (*message).readH1Header();
	Trace_Debug(this, "processIncomingDataMessage()", " ", "sender",
			spdr::toString<NodeIDImpl>(message->getSender()), "type",
			SCMessage::getMessageTypeName(h1.get<1> ()));

	bool skip_closed = false;
	{
		boost::recursive_mutex::scoped_lock lock(routingWorkMutex_);
		if (closed_)
		{
			skip_closed = true;
		}
	}

	if (skip_closed)
	{
		Trace_Exit(this, "processIncomingDataMessage", "skip-closed");
		return;
	}

	switch (h1.get<1> ())
	//message type
	{
	case SCMessage::Type_Data_PubSub:
	{
		SCMessage::H2Header h2 = message->readH2Header();

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingDataMessage()", "H2");
			buffer->addProperty("RP", SCMessage::messageRoutingProtocolName[h2.get<0>()]);
			buffer->addProperty<int>("Flags", h2.get<1>());
			buffer->addProperty<int>("TTL", h2.get<2>());
			buffer->invoke();
		}

		BusName_SPtr bus = message->getBusName();
		if (bus->isEqual(*config_.getBusName_SPtr()))
		{
			//routing within the zone, including local subscribers, message coming from same zone
			bool local_pub = false;
			if (h2.get<0>() == SCMessage::RoutingProto_PubSub)
			{
				local_pub = pubsubRouter_.route(message, h2, h1);
			}
			else if (h2.get<0>() == SCMessage::RoutingProto_Broadcast)
			{
				local_pub = broadcastRouter_.route(message, h2, h1);
			}
			else
			{
				String what("Unexpected routing protocol; type=");
				what.append(SCMessage::getMessageTypeName(h1.get<1>()));
				what.append(", RP=");
				what.append(SCMessage::messageRoutingProtocolName[h2.get<0>()]);
				Trace_Exit(this,"processIncomingDataMessage()","SpiderCastRuntimeError",what);
				throw SpiderCastRuntimeError(what);
			}

			//route over bridge
			if (!local_pub && ((h2.get<1>() & SCMessage::Message_H2_Flags_GLB_Mask) > 0))
			{
				uint8_t ttl = h2.get<2>()-(uint8_t)1;
				if (ttl > 0)
				{
					message->writeH2Header(h2.get<0>(),h2.get<1>(),--ttl);
					sendOverBridge(message, h2, h1);
				}
				else
				{
					Trace_Event(this, "route()","TTL==0, no need to send-over-bridge");
				}
			}
		}
		else if ((h2.get<1>() & SCMessage::Message_H2_Flags_GLB_Mask) > 0)
		{
			//routing within the zone, including local subscribers, of message coming from bridge
			//route as source
			if (h2.get<0>() == SCMessage::RoutingProto_PubSub)
			{
				pubsubRouter_.route_FromBridge(message, h2, h1);
			}
			else if (h2.get<0>() == SCMessage::RoutingProto_Broadcast)
			{
				broadcastRouter_.route_FromBridge(message, h2, h1);
			}
			else
			{
				//FAIL FAST
				String what("Unexpected routing protocol; type=");
				what.append(SCMessage::getMessageTypeName(h1.get<1>()));
				what.append(", RP=");
				what.append(SCMessage::messageRoutingProtocolName[h2.get<0>()]);
				Trace_Event(this,"processIncomingDataMessage()","SpiderCastRuntimeError",what);
				throw SpiderCastRuntimeError(what);
			}

			//if supervisor, route over bridge to other delegates
			{
				uint8_t ttl = h2.get<2>()-(uint8_t)1;
				if (ttl > 0)
				{
					message->writeH2Header(h2.get<0>(),h2.get<1>(),--ttl);
					sendOverSBridge(message, h2, h1);
				}
				else
				{
					Trace_Event(this, "route()","TTL==0, no need to send-over-bridge");
				}
			}
		}
		else
		{
//FAIL FAST
			String what("Unexpected routing flags: message from different bus with GLB=0; type=");
			what.append(SCMessage::getMessageTypeName(h1.get<1>()));
			what.append(", RP=");
			what.append(SCMessage::messageRoutingProtocolName[h2.get<0>()]);
			what.append(", Bus=");
			what.append(bus->toString());
			Trace_Event(this,"processIncomingDataMessage()","SpiderCastRuntimeError",what);
			throw SpiderCastRuntimeError(what);		}

	}
		break;

	case SCMessage::Type_Data_Direct:
	{
		processIncomingP2PDataMessage(message);
	}

		//TODO: add P2P msg type
		break;

	default:
	{
		String what("Unexpected message type ");
		what.append(SCMessage::getMessageTypeName(h1.get<1>()));
		Trace_Exit(this,"processIncomingDataMessage()","SpiderCastRuntimeError",what);

		throw SpiderCastRuntimeError(what);
	}

	}


	Trace_Exit(this, "processIncomingDataMessage()");
}

void RoutingManagerImpl::processIncomingP2PDataMessage(SCMessage_SPtr message)
{
	Trace_Entry(this, "processIncomingP2PDataMessage()");

	if (_receiver)
	{
		_receiver->processIncomingDataMessage(message);
	}
	else
	{
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this,
					"processIncomingP2PDataMessage()",
					"no receiver, dropping message");
			buffer->invoke();
		}
	}

	Trace_Exit(this, "processIncomingP2PDataMessage()");
}

/*
 * @see ThreadControl
 */
void RoutingManagerImpl::wakeUp(uint32_t mask)
{
	Trace_Entry(this, "wakeUp()");

	if (mask == (uint32_t)0)
		throw IllegalArgumentException("Mask must be >0");

	{
		boost::recursive_mutex::scoped_lock lock(routingWorkMutex_);
		routingWorkPending_ = routingWorkPending_ | mask; //bitwise
	}

	routingWorkCondVar_.notify_all();

	Trace_Exit(this, "wakeUp()", "Exit");
}


/*
 * @see PubSubViewListener
 */
void RoutingManagerImpl::globalPub_add(const String& topic_name)
{
	Trace_Entry(this, "globalPub_add()","topic",topic_name);

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);
		if (delPubSubBridge_)
		{
			delPubSubBridge_->globalPub_add(topic_name);
		}
	}

	Trace_Exit(this, "globalPub_add()");
}

/*
 * @see PubSubViewListener
 */
void RoutingManagerImpl::globalPub_remove(const String& topic_name)
{
	Trace_Entry(this, "globalPub_remove()","topic",topic_name);

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);
		if (delPubSubBridge_)
		{
			delPubSubBridge_->globalPub_remove(topic_name);
		}
	}

	Trace_Exit(this, "globalPub_remove()");
}

/*
 * @see PubSubViewListener
 */
void RoutingManagerImpl::globalSub_add(const String& topic_name)
{
	Trace_Entry(this, "globalSub_add()","topic",topic_name);

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);
		if (delPubSubBridge_)
		{
			delPubSubBridge_->globalSub_add(topic_name);
		}
	}

	Trace_Exit(this, "globalSub_add()");
}

/*
 * @see PubSubViewListener
 */
void RoutingManagerImpl::globalSub_remove(const String& topic_name)
{
	Trace_Entry(this, "globalSub_remove()","topic",topic_name);

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);
		if (delPubSubBridge_)
		{
			delPubSubBridge_->globalSub_remove(topic_name);
		}
	}

	Trace_Exit(this, "globalSub_remove()");
}


/*
 * @see PubSubBridge
 */
void RoutingManagerImpl::sendOverBridge(
			SCMessage_SPtr message,
			const SCMessage::H2Header& h2,
			const SCMessage::H1Header& h1)
{
	Trace_Entry(this, "sendOverBridge()");

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);

		if (delPubSubBridge_)
		{
			//Always send to S-Bridge
			ByteBuffer_SPtr bb = message->getBuffer();
			bb->setPosition(h1.get<2>());
			bool ok = delPubSubBridge_->sendMessage(message,h2,h1);
			Trace_Debug(this, "sendOverBridge()", (ok?"D-to-S-Bridge, OK":"D-to-S-Bridge, Fail"));
		}
		else if (supPubSubBridge_)
		{
			ByteBuffer_SPtr bb = message->getBuffer();
			bb->setPosition(SCMessage::Message_H2_O2M_TID_Offset);
			const int32_t tid = bb->readInt();
			bb->setPosition(h1.get<2>());

			//send to active delegates, but not to the bus the message is from
			int num = supPubSubBridge_->sendToActiveDelegates(message,tid,h2,h1,message->getBusName());
			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "sendOverBridge()", "S-to-D-Bridge");
				buffer->addProperty<int>("#sent", num);
				buffer->invoke();
			}
		}
		else
		{
			Trace_Debug(this, "sendOverBridge()", "not a bridge, skipping");
		}
	}

	Trace_Exit(this, "sendOverBridge()");
}


void RoutingManagerImpl::sendOverSBridge(
		SCMessage_SPtr message,
		const SCMessage::H2Header& h2,
		const SCMessage::H1Header& h1)
{
	Trace_Entry(this, "sendOverSBridge()");

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);

		if (supPubSubBridge_)
		{
			ByteBuffer_SPtr bb = message->getBuffer();
			bb->setPosition(SCMessage::Message_H2_O2M_TID_Offset);
			const int32_t tid = bb->readInt();
			bb->setPosition(h1.get<2>());

			//send to active delegates, but not to the bus the message is from
			int num = supPubSubBridge_->sendToActiveDelegates(message,tid,h2,h1,message->getBusName());
			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "sendOverSBridge()", "S-to-D-Bridge");
				buffer->addProperty<int>("#sent", num);
				buffer->invoke();
			}
		}
		else
		{
			Trace_Debug(this, "sendOverSBridge()", "not a bridge, skipping");
		}
	}

	Trace_Exit(this, "sendOverSBridge()");
}

void RoutingManagerImpl::runDelegateBridgeUpdateInterestTask()
{
	Trace_Entry(this, "runDelegateBridgeUpdateInterestTask()");

	{
		boost::recursive_mutex::scoped_lock lock(pubsubBridgeMutex_);
		if (delPubSubBridge_)
		{
			delPubSubBridge_->updatePubSubInterest();
		}
	}

	Trace_Exit(this, "runDelegateBridgeUpdateInterestTask()");
}



}

}
