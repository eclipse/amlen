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
 * BroadcastRouter.cpp
 *
 *  Created on: Feb 1, 2012
 */

#include "BroadcastRouter.h"

namespace spdr
{

namespace route
{

ScTraceComponent* BroadcastRouter::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Route,
		trace::ScTrConstants::Layer_ID_Route, "BroadcastRouter",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

BroadcastRouter::BroadcastRouter(
		const String& instID,
		const SpiderCastConfigImpl& config,
		CoreInterface& coreInterface,
		VirtualIDCache& vidCache,
		RoutingTableLookup& routingTable,
		PubSubBridge& pubSubBridge) :
	ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
	instID_(instID),
	config_(config),
	coreInterface_(coreInterface),
	vidCache_(vidCache),
	routingTable_(routingTable),
	pubsubBridge_(pubSubBridge),
	myVID_(vidCache_.get(config_.getMyNodeID()->getNodeName())),
	incomingMsgQ_(),
	messagingManager_()
{
	Trace_Entry(this,"BroadcastRouter()");
}
BroadcastRouter::~BroadcastRouter()
{
	Trace_Entry(this,"~BroadcastRouter()");
}

void BroadcastRouter::init()
{
	incomingMsgQ_ = coreInterface_.getCommAdapter()->getIncomingMessageQ();
	messagingManager_ = coreInterface_.getMessagingManager();
}

bool BroadcastRouter::send(SCMessage_SPtr message)
		throw (SpiderCastRuntimeError)
{
	Trace_Entry(this, "send()", "msg", spdr::toString<SCMessage>(message));

	ByteBuffer_SPtr bb = message->getBuffer();
	const std::size_t data_length = bb->getDataLength();

	const SCMessage::H1Header h1 = message->readH1Header();
	const SCMessage::H2Header h2 = message->readH2Header();

	if (data_length != (std::size_t)h1.get<2>())
	{
		throw SpiderCastRuntimeError("Total length different than data length");
	}

	//Send local copy
	ByteBuffer_SPtr buf_copy = ByteBuffer::cloneByteBuffer(
			(char *) bb->getBuffer(), data_length);

	SCMessage_SPtr localMsg = SCMessage_SPtr(new SCMessage());
	localMsg->setBuffer(buf_copy);

	uint8_t flags = h2.get<1> () | SCMessage::Message_H2_Flags_LCL_Mask;
	localMsg->writeH2Header(h2.get<0>(), flags, h2.get<2>());
	buf_copy->setPosition(data_length);

	localMsg->setSender(config_.getMyNodeID());
	localMsg->setStreamId(0);
	localMsg->setBusName(config_.getBusName_SPtr());

	incomingMsgQ_->onMessage(localMsg);
	Trace_Debug(this, "send()", "sent local copy");

	//global within zone routing
	bb->setPosition(data_length);
	int count = sendToRange(message, h2, h1, *myVID_);

	if ((flags & SCMessage::Message_H2_Flags_GLB_Mask) > 0)
	{
		//if base-zone - send to D-Bridge
		//if mngt-zone - send to S-Bridge
		pubsubBridge_.sendOverBridge(message, h2, h1);
	}

	Trace_Exit<bool> (this, "send()", (count>0));
	return (count>0);

}

bool BroadcastRouter::route(
		SCMessage_SPtr message,
		const SCMessage::H2Header& h2,
		const SCMessage::H1Header& h1)
	throw (SpiderCastRuntimeError)
{
	Trace_Entry(this, "route()");
	bool local = false;

	ByteBuffer_SPtr bb = message->getBuffer();
	util::VirtualID upper_bound_VID(bb->readVirtualID());
	bb->readInt(); //skip topic hash

	if ((h2.get<1> () & SCMessage::Message_H2_Flags_LCL_Mask) > 0)
	{
		//A message from a local transmitter, no need to route
		Trace_Debug(this, "route()","A message from a local transmitter, no need to send out");
		messagingManager_->processIncomingDataMessage(message);
		local = true;
	}
	else
	{
		//A message from a remote publisher
		Trace_Debug(this, "route()","A message from a remote transmitter");
		//check TTL
		uint8_t ttl = h2.get<2>()-(uint8_t)1;
		if (ttl > 0)
		{
			message->writeH2Header(h2.get<0>(),h2.get<1>(),--ttl);
			sendToRange(message, h2, h1, upper_bound_VID);
		}
		else
		{
			Trace_Event(this, "route()","TTL==0, no need to route");
		}

		//send to local consumer
		messagingManager_->processIncomingDataMessage(message);
	}

	Trace_Exit<bool>(this, "route()",local);
	return local;
}

bool BroadcastRouter::route_FromBridge(SCMessage_SPtr message,
		const SCMessage::H2Header& h2, const SCMessage::H1Header& h1)
throw (SpiderCastRuntimeError)
{
	Trace_Entry(this, "route_FromBridge()");

	bool sent = false;

	ByteBuffer_SPtr bb = message->getBuffer();
	util::VirtualID upper_bound_VID(bb->readVirtualID()); //skip VID
	int32_t topicHash = bb->readInt();

	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "route_FromBridge()");
		buffer->addProperty<int32_t>("tid", topicHash);
		buffer->invoke();
	}

	//check TTL
	uint8_t ttl = h2.get<2>()-(uint8_t)1;
	if (ttl > 0)
	{
		//global within the zone, as source
		message->writeH2Header(h2.get<0>(),h2.get<1>(),ttl);
		int num = sendToRange(message,h2,h1,*myVID_);
		sent = (num > 0);
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "route_FromBridge()", "sent");
			buffer->addProperty<uint8_t>("TTL", ttl);
			buffer->addProperty<int>("#sent", num);
			buffer->invoke();
		}
	}
	else
	{
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "route_FromBridge()",
					"TTL==0, no need to route");
			buffer->invoke();
		}
	}

	messagingManager_->processIncomingDataMessage(message);

	Trace_Exit<bool>(this, "route_FromBridge()", sent);
	return sent;
}

int BroadcastRouter::sendToRange(
		SCMessage_SPtr message,
		const SCMessage::H2Header& h2,
		const SCMessage::H1Header& h1,
		const util::VirtualID& upperBound)
	throw (SpiderCastRuntimeError)
{
	Trace_Entry(this, "sendToRange()");

	int num_sent = 0;
	Next2HopsBroadcast next2Hops = routingTable_.next2Hops_Broadcast(upperBound);

	if (next2Hops.firstHop) //successor in range
	{
		ByteBuffer_SPtr bb = message->getBuffer();
		bb->setPosition(SCMessage::Message_H2_O2M_Offset);
		bb->writeVirtualID( next2Hops.firstHopUpperBound); //upper bound for successor
		bb->setPosition(h1.get<2>()); //set position to end of message
		int res1 = next2Hops.firstHop->sendMessage(message);

		if (res1==0)
		{
			++num_sent;
			Trace_Debug(this, "sendToRange()", "sent to successor");
		}
		else
		{
			Trace_Debug(this, "sendToRange()", "send to successor failed");
		}

		if (next2Hops.secondHop)
		{
			bb->setPosition(SCMessage::Message_H2_O2M_Offset);
			bb->writeVirtualID(upperBound); //upper bound for mid-range
			bb->setPosition(h1.get<2>()); //set position to end of message
			int res2 = next2Hops.secondHop->sendMessage(message);

			if (res2==0)
			{
				++num_sent;
				Trace_Debug(this, "sendToRange()", "sent to mid-range");
			}
			else
			{
				Trace_Debug(this, "sendToRange()", "send to mid-range failed");
			}
		}
		else
		{
			Trace_Debug(this, "sendToRange()", "mid-range empty, no one to send to");
		}
	}
	else
	{
		Trace_Debug(this, "sendToRange()", "successor empty, no one to send to");
	}

	Trace_Exit<int>(this, "sendToRange()",num_sent);
	return num_sent;
}

}
}
