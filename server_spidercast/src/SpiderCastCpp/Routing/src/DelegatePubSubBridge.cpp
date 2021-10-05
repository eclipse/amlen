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
 * DelegatePubSubBridge.cpp
 *
 *  Created on: Jul 8, 2012
 */

#include "DelegatePubSubBridge.h"

namespace spdr
{

namespace route
{

ScTraceComponent* DelegatePubSubBridge::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Route,
		trace::ScTrConstants::Layer_ID_Route,
		"DelegatePubSubBridge",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

DelegatePubSubBridge::DelegatePubSubBridge(
		const String& instID,
		const SpiderCastConfigImpl& config,
		boost::shared_ptr<PubSubViewKeeper> pubsubViewKeeper,
		CoreInterface& coreInterface) :
		ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
		closed_(false),
		config_(config),
		pubsubViewKeeper_(pubsubViewKeeper),
		attributeControl_(coreInterface.getMembershipManager()->getAttributeControl()),
		routingManager_(coreInterface.getRoutingManager()),
		memTopoTaskScheduler_(coreInterface.getTopoMemTaskSchedule()),
		updateInterestTask_(new DBridgePubSubInterestTask(coreInterface.getRoutingManager())),
		updateInterestTaskScheduled_(false),
		outgoingHierMessage_()
{
	Trace_Entry(this, "DelegatePubSubBridge()");
	outgoingHierMessage_ = SCMessage_SPtr(new SCMessage);
	outgoingHierMessage_->setBuffer(ByteBuffer::createByteBuffer(128));
}

DelegatePubSubBridge::~DelegatePubSubBridge()
{
	Trace_Entry(this, "~DelegatePubSubBridge()");

//	if (!closed_)
//	{
//		close();
//	}

}

void DelegatePubSubBridge::init()
{
	StringSet set = pubsubViewKeeper_->getGlobalTopicPublications();
	for (StringSet::const_iterator it = set.begin(); it != set.end(); ++it)
	{
		globalPub_add(*it);
	}

	set = pubsubViewKeeper_->getGlobalTopicSubscriptions();
	for (StringSet::const_iterator it = set.begin(); it != set.end(); ++it)
	{
		globalSub_add(*it);
	}
}

//TODO add a parameter speciying if its an instance closure, then we dont need to clear attributes,
// otherwise is a role change, when we clear attributes
void DelegatePubSubBridge::close(bool shutdown)
{
	closed_ = true;

	if (!shutdown)
	{
		StringSet set = pubsubViewKeeper_->getGlobalTopicPublications();
		for (StringSet::const_iterator it = set.begin(); it != set.end(); ++it)
		{
			globalPub_remove(*it);
		}

		set = pubsubViewKeeper_->getGlobalTopicSubscriptions();
		for (StringSet::const_iterator it = set.begin(); it != set.end(); ++it)
		{
			globalSub_remove(*it);
		}
	}
}

void DelegatePubSubBridge::setTargetSupervisor(Neighbor_SPtr targetSupervisor)
{
	targetSupervisor_ = targetSupervisor;
	rescheduleInterestUpdateTask();
}

//void DelegatePubSubBridge::processIncomingDataMessage(SCMessage_SPtr msg)
//{
//	Trace_Entry(this, "processIncomingDataMessage()","msg", toString<SCMessage>(msg));
//
//
//
//	Trace_Exit(this, "processIncomingDataMessage()");
//}

void DelegatePubSubBridge::processIncomingControlMessage(SCMessage_SPtr ctrlMsg)
{
	Trace_Entry(this, "processIncomingControlMessage()");

	SCMessage::H1Header h1 = (*ctrlMsg).readH1Header();
	Trace_Debug(this, "processIncomingControlMessage()", " ",
			"sender", spdr::toString<NodeIDImpl>(ctrlMsg->getSender()),
			"type", SCMessage::getMessageTypeName(h1.get<1>()));

	switch (h1.get<1>()) //message type
	{
	case SCMessage::Type_Hier_PubSubBridge_BaseZoneInterest_Reply:
	{
		//nothing to do right now
	}
	break;

	default:
	{
		String what("Unexpected message type ");
		what.append(SCMessage::getMessageTypeName(h1.get<1>()));
		Trace_Exit(this,"processIncomingDataMessage()","SpiderCastRuntimeError",what);

		throw SpiderCastRuntimeError(what);
	}

	}

	Trace_Exit(this, "processIncomingControlMessage()");
}

bool DelegatePubSubBridge::sendMessage(SCMessage_SPtr msg,
		const SCMessage::H2Header& h2,
		const SCMessage::H1Header& h1)
{
	if (targetSupervisor_)
	{
		return (targetSupervisor_->sendMessage(msg) == 0);
	}

	return false;
}

/**
 * When global publications change.
 * form bridge subscriptions.
 */
void DelegatePubSubBridge::globalPub_add(String topic_name)
{
	using namespace std;
	using namespace spdr::messaging;

	String topicKey(MessagingManager::topicKey_Prefix);
	topicKey.append(topic_name);
	pair<event::AttributeValue,bool> res = attributeControl_.getAttribute(topicKey);

	char flags = 0x0;

	if (res.second)
	{
		if (res.first.getLength() > 0)
		{
			flags = (res.first.getBuffer())[0];
		}
		else
		{
			//Error, FAIL FAST
			String what("Error: DelegatePubSubBridge::globalPub_add() empty value on key ");
			what.append(topicKey);
			throw SpiderCastRuntimeError(what);
		}
	}

	flags = MessagingManager::addBridgeSub_Flags(flags);

	attributeControl_.setAttribute(
			topicKey, std::pair<int32_t,char*>(1,&flags));
}

/**
 * When global publications change.
 * remove bridge subscription.
 */
void DelegatePubSubBridge::globalPub_remove(String topic_name)
{
	using namespace std;
	using namespace spdr::messaging;

	String topicKey(MessagingManager::topicKey_Prefix);
	topicKey.append(topic_name);
	pair<event::AttributeValue,bool> res = attributeControl_.getAttribute(topicKey);

	if (res.second)
	{
		if (res.first.getLength() > 0)
		{
			char flags = (res.first.getBuffer())[0];
			flags = MessagingManager::removeBridgeSub_Flags(flags);

			if (flags > 0)
			{
				attributeControl_.setAttribute(
					topicKey,std::pair<int32_t,char*>(1,&flags));
			}
			else
			{
				attributeControl_.removeAttribute(topicKey);
			}
		}
		else
		{
			//Error, FAIL FAST
			String what("Error: DelegatePubSubBridge::globalPub_remove() empty value on key ");
			what.append(topicKey);
			throw SpiderCastRuntimeError(what);
		}
	}
	else
	{
		//Error, FAIL FAST
		String what("Error: DelegatePubSubBridge::globalPub_remove() missing value on key ");
		what.append(topicKey);
		throw SpiderCastRuntimeError(what);
	}
}

/**
 * When global subscriptions change.
 * Get global subscriptions and send them to the supervisor over the bridge.
 */
void DelegatePubSubBridge::globalSub_add(String topic_name)
{
	Trace_Entry(this,"globalSub_add()", "topic", topic_name);

	rescheduleInterestUpdateTask();

	Trace_Exit(this,"globalSub_add()");
}

/**
 * When global subscriptions change.
 * Get global subscriptions and send them to the supervisor over the bridge.
 */
void DelegatePubSubBridge::globalSub_remove(String topic_name)
{
	Trace_Entry(this,"globalSub_remove()", "topic", topic_name);

	rescheduleInterestUpdateTask();

	Trace_Exit(this,"globalSub_remove()");
}

void DelegatePubSubBridge::rescheduleInterestUpdateTask()
{
	if (!updateInterestTaskScheduled_)
	{
		memTopoTaskScheduler_->scheduleDelay(
				updateInterestTask_,
				TaskSchedule::ZERO_DELAY);
		updateInterestTaskScheduled_ = true;

		Trace_Debug(this,"rescheduleInterestUpdateTask()","rescheduled task");
	}
	else
	{
		Trace_Debug(this,"rescheduleInterestUpdateTask()","task already scheduled");
	}
}

void DelegatePubSubBridge::updatePubSubInterest()
{
	updateInterestTaskScheduled_ = false;

	if (targetSupervisor_)
	{
		outgoingHierMessage_->writeH1Header(SCMessage::Type_Hier_PubSubBridge_BaseZoneInterest);

		ByteBuffer_SPtr buffer = outgoingHierMessage_->getBuffer();
		buffer->writeString(config_.getNodeName());
		buffer->writeString(targetSupervisor_->getName());

		StringSet subs = pubsubViewKeeper_->getGlobalTopicSubscriptions();
		buffer->writeInt(static_cast<int32_t>(subs.size()));
		for (StringSet::const_iterator it = subs.begin(); it != subs.end(); ++it)
		{
			buffer->writeString(*it);
		}

		outgoingHierMessage_->updateTotalLength();
		if (config_.isCRCMemTopoMsgEnabled())
		{
			outgoingHierMessage_->writeCRCchecksum();
		}

		bool ok = (targetSupervisor_->sendMessage(outgoingHierMessage_) == 0);

		if (ScTraceBuffer::isEventEnabled(tc_))
		{
			ScTraceBufferAPtr buff = ScTraceBuffer::event(this,"updatePubSubInterest()","sent");
			buff->addProperty("target", targetSupervisor_->getName());
			buff->addProperty<size_t>("size",subs.size());
			buff->addProperty<bool>("ok", ok);
			buff->invoke();
		}
	}
	else
	{
		Trace_Event(this,"updatePubSubInterest()", "NULL target supervisor");
	}
}

}

}
