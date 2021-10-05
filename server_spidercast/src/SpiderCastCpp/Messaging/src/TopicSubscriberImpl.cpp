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
 * TopicSubscriberImpl.cpp
 *
 *  Created on: Jan 31, 2012
 */

#include "TopicSubscriberImpl.h"

namespace spdr
{

namespace messaging
{


ScTraceComponent* TopicSubscriberImpl::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Msgn,
		trace::ScTrConstants::Layer_ID_Msgn,
		"TopicSubscriberImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

TopicSubscriberImpl::TopicSubscriberImpl(
		const String& instID,
		const SpiderCastConfigImpl& config,
		NodeIDCache& nodeIDCache,
		CoreInterface& coreInterface,
		Topic_SPtr topic,
		MessageListener& messageListener,
		PubSubEventListener& eventListener,
		const PropertyMap& propMap) :
		ScTraceContext(tc_,instID,config.getMyNodeID()->getNodeName()),
		instID_(instID),
		config_(config),
		nodeIDCache_(nodeIDCache),
		coreInterface_(coreInterface),
		topic_(topic),
		messageListener_(messageListener),
		eventListener_(eventListener),
		propMap_(propMap),
		mutex_(),
		closed_(false),
		messagingManager_SPtr(coreInterface.getMessagingManager()),
		topicRxBestEffort_(instID, config, messageListener, topic)
{
	Trace_Entry(this, "TopicSubscriberImpl()");
}

TopicSubscriberImpl::~TopicSubscriberImpl()
{
	Trace_Entry(this, "~TopicSubscriberImpl()");
	messagingManager_SPtr.reset();
}


/*
 * @see TopicSubscriber
 */
void TopicSubscriberImpl::close()
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
		messagingManager_SPtr->removeSubscriber(topic_);
	}

	Trace_Exit(this, "close()");
}

/*
 * @see TopicSubscriber
 */
bool TopicSubscriberImpl::isOpen()
{
	boost::recursive_mutex::scoped_lock lock(mutex_);
	return !closed_;
}

/*
 * @see TopicSubscriber
 */
Topic_SPtr TopicSubscriberImpl::getTopic()
{
	return topic_;
}

std::string TopicSubscriberImpl::toString() const
{
	std::string s("TopicSubscriber: ");
	s = s + "topic=" +topic_->toString();
	return s;
}

void TopicSubscriberImpl::processIncomingDataMessage(SCMessage_SPtr message,
		const SCMessage::H3HeaderStart& h3start)
{
	Trace_Entry(this, "processIncomingDataMessage()");

	if (!isOpen())
	{
		Trace_Exit(this, "processIncomingDataMessage()","Closed");
		return;
	}

	switch (h3start.get<1>()) //ReliabilityMode
	{
	case SCMessage::ReliabilityMode_BestEffort:
	{
		topicRxBestEffort_.processIncomingDataMessage(message);
	}
	break;

	default:
	{
		//throw not supported
		String what("Not supported: ReliabilityMode=");
		what.append(SCMessage::messageReliabilityModeName[h3start.get<1>()]);
		throw SpiderCastRuntimeError(what);
	}

	}

	Trace_Exit(this, "processIncomingDataMessage()");
}

}

}
