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
 * TopicPublisherImpl.cpp
 *
 *  Created on: Jan 31, 2012
 */

#include "TopicPublisherImpl.h"

namespace spdr
{

namespace messaging
{

ScTraceComponent* TopicPublisherImpl::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Msgn,
		trace::ScTrConstants::Layer_ID_Msgn,
		"TopicPublisherImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

TopicPublisherImpl::TopicPublisherImpl(
		const String& instID,
		const SpiderCastConfigImpl& config,
		NodeIDCache& nodeIDCache,
		CoreInterface& coreInterface,
		Topic_SPtr topic,
		PubSubEventListener& eventListener,
		const PropertyMap& propMap,
		StreamID_SPtr streamID) :
		ScTraceContext(tc_,instID,config.getMyNodeID()->getNodeName()),
		instID_(instID),
		config_(config),
		nodeIDCache_(nodeIDCache),
		coreInterface_(coreInterface),
		topic_(boost::static_pointer_cast<TopicImpl>(topic)),
		eventListener_(eventListener),
		propMap_(propMap),
		streamID_(streamID),
		mutex_(),
		closed_(false),
		init_done_(false),
		messagingManager_SPtr(coreInterface.getMessagingManager()),
		routingManager_SPtr(coreInterface.getRoutingManager()),
		broadcastRouter_(routingManager_SPtr->getBroadcastRouter()),
		pubsubRouter_(routingManager_SPtr->getPubSubRouter()),
		next_msg_num_(0),
		outgoingDataMsg_(new SCMessage),
		header_size_(0),
		routingProtocol_(SCMessage::RoutingProto_PubSub)

{
	Trace_Entry(this, "TopicPublisherImpl()");
	outgoingDataMsg_->setBuffer(ByteBuffer::createByteBuffer(1024));
	ByteBuffer_SPtr bb(outgoingDataMsg_->getBuffer());

	// search properties for configuration overrides
	BasicConfig pub_config(propMap_);
	String pub_config_RP = pub_config.getOptionalProperty(
			config::PublisherRoutingProtocol_PROP_KEY,
			config_.getPublisherRoutingProtocol());
	SpiderCastConfigImpl::validatePublisherRoutingProtocol(pub_config_RP);
	if (pub_config_RP == config::RoutingProtocol_Broadcast_VALUE)
	{
		routingProtocol_ = SCMessage::RoutingProto_Broadcast;
	}
	else
	{
		routingProtocol_ = SCMessage::RoutingProto_PubSub;
	}

	//prepare the headers
	//H1
	outgoingDataMsg_->writeH1Header(SCMessage::Type_Data_PubSub);

	//H2
	uint8_t flags = SCMessage::Message_H2_Flags_Default;
	if (topic_->isGlobalScope())
	{
		flags = flags | SCMessage::Message_H2_Flags_GLB_Mask;
	}
	outgoingDataMsg_->writeH2Header(
			routingProtocol_,
			flags,
			SCMessage::Message_H2_TTL_Default);

	//H2-O2M
	bb->writeVirtualID(util::VirtualID::MinValue); //skip, router will update
	boost::hash<std::string> string_hash;
	bb->writeInt((int32_t)string_hash(topic_->getName())); //Topic-name hash

	//H3
	outgoingDataMsg_->writeH3HeaderStart(SCMessage::TransProto_PubSub,
			SCMessage::ReliabilityMode_BestEffort);
	bb->writeStreamID(static_cast<StreamIDImpl&>(*streamID));
	bb->writeLong(next_msg_num_);
	bb->writeString(topic_->getName());
	//TODO add bus-name to wire-format
	bb->writeString(config_.getMyNodeID()->getNodeName()); //TODO change to NodeID
	header_size_ = bb->getPosition();

	Trace_Exit(this, "TopicPublisherImpl()");
}

TopicPublisherImpl::~TopicPublisherImpl()
{
	Trace_Entry(this, "~TopicPublisherImpl()");
	messagingManager_SPtr.reset();
	routingManager_SPtr.reset();

	Trace_Exit(this, "~TopicPublisherImpl()");
}

/*
 * @see TopicPublisher
 */
void TopicPublisherImpl::close()
{
	Trace_Entry(this, "close()");

	bool do_close = false;

	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		do_close = !closed_;
		closed_ = true;
	}

	if (do_close)
	{
		messagingManager_SPtr->removePublisher(streamID_);
	}

	Trace_Exit(this, "close()");
}

/*
 * @see TopicPublisher
 */
bool TopicPublisherImpl::isOpen() const
{
	Trace_Entry(this, "isOpen()");

	{
		boost::recursive_mutex::scoped_lock lock(mutex_);
		return !closed_;
	}

	Trace_Exit(this, "isOpen()");
}

/*
 * @see TopicPublisher
 */
Topic_SPtr TopicPublisherImpl::getTopic() const
{
	return boost::static_pointer_cast<Topic>(topic_);
}

/*
 * @see TopicPublisher
 */
StreamID_SPtr TopicPublisherImpl::getStreamID() const
{
	return streamID_;
}

/*
 * @see TopicPublisher
 */
int64_t TopicPublisherImpl::publishMessage(const TxMessage& message)
{
	Trace_Entry(this,"publishMessage()");

	int64_t msg_num = -1;

	{
		boost::recursive_mutex::scoped_lock lock(mutex_);

		if (closed_)
		{
			throw IllegalStateException("Service is closed.");
		}

		ByteBuffer_SPtr bb = outgoingDataMsg_->getBuffer();
		bb->setPosition(SCMessage::Message_H3_MessageNumber_Offset);
		bb->writeLong(next_msg_num_);
		bb->setPosition(header_size_);
		bb->writeInt(message.getBuffer().first);
		bb->writeByteArray(message.getBuffer().second, message.getBuffer().first);
		outgoingDataMsg_->updateTotalLength();

		try
		{
			if (routingProtocol_ == SCMessage::RoutingProto_PubSub)
			{
				pubsubRouter_.send(topic_, outgoingDataMsg_);
			}
			else
			{
				broadcastRouter_.send(outgoingDataMsg_);
			}
			msg_num = next_msg_num_++;
		}
		catch (SpiderCastRuntimeError& re)
		{
			Trace_Event(this,"publishMessage()","Failed to publish message",
					"what",re.what(),
					"trace", re.getStackTrace());
			throw;
		}
	}

	Trace_Exit<int64_t>(this,"publishMessage()",msg_num);
	return msg_num;
}

std::string TopicPublisherImpl::toString() const
{
	std::string s("TopicPublisher: ");
	s = s + "topic=" +topic_->toString() + " sid=" + streamID_->toString();
	return s;
}

//friend
bool operator<(const TopicPublisherImpl& lhs, const TopicPublisherImpl& rhs)
{
	return (*lhs.getStreamID()) < (*rhs.getStreamID());
}

//friend
bool operator==(const TopicPublisherImpl& lhs, const TopicPublisherImpl& rhs)
{
	return (*lhs.getStreamID()) == (*rhs.getStreamID());
}

}

}
