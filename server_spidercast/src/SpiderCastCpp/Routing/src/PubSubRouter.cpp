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
 * PubSubRouter.cpp
 *
 *  Created on: Jan 31, 2012
 */

#include <sstream>

#include "PubSubRouter.h"

namespace spdr
{

namespace route
{

ScTraceComponent* PubSubRouter::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Route,
		trace::ScTrConstants::Layer_ID_Route, "PubSubRouter",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

PubSubRouter::PubSubRouter(
		const String& instID,
		const SpiderCastConfigImpl& config,
		CoreInterface& coreInterface,
		VirtualIDCache& vidCache,
		RoutingTableLookup& routingTable,
		PubSubBridge& pubSubBridge,
		boost::shared_ptr<PubSubViewKeeper> pubsubViewKeeper) :
	ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
	instID_(instID),
	config_(config),
	coreInterface_(coreInterface),
	vidCache_(vidCache),
	routingTable_(routingTable),
	pubSubBridge_(pubSubBridge),
	pubsubViewKeeper_(pubsubViewKeeper),
	myVID_(vidCache_.get(config_.getMyNodeID()->getNodeName()))
{
	Trace_Entry(this,"PubSubRouter()");
}

PubSubRouter::~PubSubRouter()
{
	Trace_Entry(this,"~PubSubRouter()");
}

void PubSubRouter::init()
{
	incomingMsgQ_ = coreInterface_.getCommAdapter()->getIncomingMessageQ();
	messagingManager_ = coreInterface_.getMessagingManager();
}

bool PubSubRouter::send(messaging::TopicImpl_SPtr topic, SCMessage_SPtr message)
		throw (SpiderCastRuntimeError)
		{
	Trace_Entry(this,"send()",
			"topic", spdr::toString<messaging::TopicImpl>(topic),
			"msg", spdr::toString<SCMessage>(message));

	ByteBuffer_SPtr bb = message->getBuffer();
	const std::size_t data_length = bb->getDataLength();

	const SCMessage::H1Header h1 = message->readH1Header();
	const SCMessage::H2Header h2 = message->readH2Header();

	if (data_length != (std::size_t)h1.get<2>())
	{
		throw SpiderCastRuntimeError("Total length different then data length");
	}

	//Local routing
	int32_t topicID = static_cast<int32_t>(topic->hash_value());
	bool local = isLocalSubscriber(topicID);
	if (local)
	{
		ByteBuffer_SPtr buf_copy = ByteBuffer::cloneByteBuffer(
				(char *) bb->getBuffer(), data_length);

		SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
		scMsg->setBuffer(buf_copy);

		std::size_t last_pos = buf_copy->getPosition();

		uint8_t flags = h2.get<1> () | SCMessage::Message_H2_Flags_LCL_Mask;
		scMsg->writeH2Header(h2.get<0>(),flags,h2.get<2>());
		buf_copy->setPosition(last_pos);

		scMsg->setSender(config_.getMyNodeID());
		scMsg->setStreamId(0);
		scMsg->setBusName(config_.getBusName_SPtr());

		incomingMsgQ_->onMessage(scMsg);

		Trace_Debug(this,"send()","sent local copy");
	}

	//within zone routing
	int count = sendToRange(message,topicID,h2,h1,*myVID_);

	if ((h2.get<1>() & SCMessage::Message_H2_Flags_GLB_Mask) > 0)
	{
		//if base-zone - send to D-Bridge
		//if mngt-zone - send to S-Bridge
		pubSubBridge_.sendOverBridge(message, h2, h1);
	}

	Trace_Exit<int>(this,"send()",count);

	return (count>0);
}


bool PubSubRouter::route(SCMessage_SPtr message,
		const SCMessage::H2Header& h2, const SCMessage::H1Header& h1)
	throw (SpiderCastRuntimeError)
{
	Trace_Entry(this, "route()");
	bool local = false;

	ByteBuffer_SPtr bb = message->getBuffer();
	util::VirtualID upper_bound_VID(bb->readVirtualID());
	int32_t topicHash = bb->readInt();

	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "route()");
		buffer->addProperty("ub", upper_bound_VID.toString());
		buffer->addProperty<int32_t>("tid", topicHash);
		buffer->invoke();
	}

	if ((h2.get<1>() &  SCMessage::Message_H2_Flags_LCL_Mask) > 0)
	{
		Trace_Event(this, "route()", "Message from local publisher, no need to route");
		local = true;
		if (isLocalSubscriber(topicHash))
		{
			messagingManager_->processIncomingDataMessage(message);
		}
		else
		{
			Trace_Event(this, "route()", "Local message but no local subscriber, ignored");
		}
	}
	else
	{
		//A message from a remote publisher
		Trace_Event(this, "route()", "Message from remote publisher");
		//check TTL
		uint8_t ttl = h2.get<2>()-(uint8_t)1;
		if (ttl > 0)
		{
			message->writeH2Header(h2.get<0>(),h2.get<1>(),ttl);
			//global routing
			sendToRange(message,topicHash,h2,h1,upper_bound_VID);
		}
		else
		{
			Trace_Event(this, "route()","TTL==0, no need to route");
		}

		if (isLocalSubscriber(topicHash))
		{
			messagingManager_->processIncomingDataMessage(message);
		}
	}

	Trace_Exit<bool>(this, "route()",local);
	return local;
}

bool PubSubRouter::route_FromBridge(SCMessage_SPtr message,
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
		int num = sendToRange(message,topicHash,h2,h1,*myVID_);
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

	if (isLocalSubscriber(topicHash))
	{
		messagingManager_->processIncomingDataMessage(message);
	}

	Trace_Exit<bool>(this, "route_FromBridge()", sent);
	return sent;
}

void PubSubRouter::addLocalSubscriber(const int32_t topicHash)
{
	using std::pair;

	boost::mutex::scoped_lock lock(filterMutex_);

	pair<TopicHash2Count_Map::iterator,bool> res = localSubscriptionFilter_.insert(
			pair<int32_t,int32_t>(topicHash,1));
	if (!res.second)
	{
		++(res.first->second);
	}
}
void PubSubRouter::removeLocalSubscriber(const int32_t topicHash)
{
	boost::mutex::scoped_lock lock(filterMutex_);

	TopicHash2Count_Map::iterator pos = localSubscriptionFilter_.find(topicHash);
	if (pos != localSubscriptionFilter_.end())
	{
		if (pos->second == 1)
		{
			localSubscriptionFilter_.erase(topicHash);
		}
		else
		{
			--(pos->second);
		}
	}
	else
	{
		std::ostringstream what;
		what << "Unmatched PubSubRouter::removeLocalSubscriber, topicHash=" << topicHash;
		throw SpiderCastRuntimeError(what.str());
	}
}

bool PubSubRouter::isLocalSubscriber(const int32_t topicHash) const
{
		boost::mutex::scoped_lock lock(filterMutex_);

		return (localSubscriptionFilter_.count(topicHash)>0);
}

int PubSubRouter::sendToRange(
		SCMessage_SPtr message,
		int32_t topicID,
		const SCMessage::H2Header& h2,
		const SCMessage::H1Header& h1,
		const util::VirtualID& upperBound)
	throw (SpiderCastRuntimeError)
{
	Trace_Entry(this, "sendToRange()");

	int num_sent = 0;

	std::pair<NodeIDImpl_SPtr, util::VirtualID_SPtr> closest_sub =
			pubsubViewKeeper_->getClosestSubscriber(topicID);

	if (closest_sub.first)
	{
		Next2HopsBroadcast next2Hops = routingTable_.next2Hops_PubSub_CSH(
				*(closest_sub.second),upperBound);

		if (next2Hops.firstHop) //1st-hop in range
		{
			ByteBuffer_SPtr bb = message->getBuffer();
			bb->setPosition(SCMessage::Message_H2_O2M_Offset);
			bb->writeVirtualID(next2Hops.firstHopUpperBound); //upper bound for 1st-hop
			bb->setPosition(h1.get<2>()); //set position to end of message
			int res1 = next2Hops.firstHop->sendMessage(message);

			if (res1==0)
			{
				++num_sent;
				Trace_Debug(this, "sendToRange()", "sent to 1st-hop",
						"node",next2Hops.firstHop->getName(),
						"ub",next2Hops.firstHopUpperBound.toString());
			}
			else
			{
				Trace_Debug(this, "sendToRange()", "send to 1st-hop failed");
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
					Trace_Debug(this, "sendToRange()", "sent to mid-range",
							"node",next2Hops.secondHop->getName(),
							"ub",upperBound.toString());

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
			Trace_Debug(this, "sendToRange()", "1st-hop empty, no one to send to");
		}
	}
	else
	{
		Trace_Debug(this, "sendToRange()", "closest-subscriber empty, no one to send to");
	}

	Trace_Exit<int>(this, "sendToRange()",num_sent);
	return num_sent;
}


}

}
