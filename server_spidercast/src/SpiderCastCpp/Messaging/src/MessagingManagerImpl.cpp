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
 * MessagingManagerImpl.cpp
 *
 *  Created on: Feb 1, 2012
 */

#include "MessagingManagerImpl.h"

namespace spdr
{

namespace messaging
{

ScTraceComponent* MessagingManagerImpl::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Msgn,
		trace::ScTrConstants::Layer_ID_Msgn,
		"MessagingManagerImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

//const String MessagingManagerImpl::topicKey_Prefix = ".sub.";

MessagingManagerImpl::MessagingManagerImpl(
		const String& instID,
		const SpiderCastConfigImpl& config,
		NodeIDCache& nodeIDCache,
		VirtualIDCache& vidCache,
		CoreInterface& coreInterface) :
		ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
		instID_(instID),
		config_(config),
		nodeIDCache_(nodeIDCache),
		nodeVirtualIDCache_(vidCache),
		coreInterface_(coreInterface),
		membershipManager_(),
		routingManager_(),
		commManager_(),
		sidMutex_(),
		sidPrefix_(vidCache.get(config.getMyNodeID()->getNodeName())->getLower64()),
		sidSuffix_(0),
		pubsubMutex_(),
		publishers_by_StreamID_(),
		closedPub_(false),

		subscriber_by_Topic_(),
		closedSub_(false),
		_receiver()
{
	Trace_Entry(this, "MessagingManagerImpl()");

	sidSuffix_ = (uint64_t)coreInterface_.getIncarnationNumber();
	sidSuffix_ = sidSuffix_ << 32;
}

MessagingManagerImpl::~MessagingManagerImpl()
{
	Trace_Entry(this, "~MessagingManagerImpl()");
}

void MessagingManagerImpl::init()
{
	Trace_Entry(this, "init()");

	membershipManager_ = coreInterface_.getMembershipManager();
	routingManager_ = coreInterface_.getRoutingManager();
	commManager_ = coreInterface_.getCommAdapter();

	Trace_Exit(this, "init()");
}

void MessagingManagerImpl::terminate(bool soft)
{
	Trace_Entry(this, "terminate()", "soft", (soft ? "true" : "false"));

	std::list<TopicPublisherImpl_SPtr> pub_list;

	{
		boost::recursive_mutex::scoped_lock lock(pubsubMutex_);
		closedPub_ = true;

		for (StreamID2Publisher_Map::iterator pub_it = publishers_by_StreamID_.begin();
				pub_it != publishers_by_StreamID_.end(); ++pub_it)
		{
			pub_list.push_back(pub_it->second);
		}

		publishers_by_StreamID_.clear();
	}

	for (std::list<TopicPublisherImpl_SPtr>::iterator it1 = pub_list.begin(); it1 != pub_list.end(); ++it1)
	{
		(*it1)->close();
	}

	Trace_Debug(this, "terminate()", "closed all publishers");

	std::list<TopicSubscriberImpl_SPtr> sub_list;

	{
		boost::recursive_mutex::scoped_lock lock(pubsubMutex_);

		closedSub_ = true;

		for (TopicName2Subscriber_Map::iterator sub_it = subscriber_by_Topic_.begin();
				sub_it != subscriber_by_Topic_.end(); ++sub_it)
		{
			sub_list.push_back(sub_it->second);
		}

		subscriber_by_Topic_.clear();
	}

	for (std::list<TopicSubscriberImpl_SPtr>::iterator it2 = sub_list.begin(); it2 != sub_list.end(); ++it2)
	{
		(*it2)->close();
	}

	Trace_Debug(this, "terminate()", "closed all subscribers");

	Trace_Exit(this, "terminate()");
}

void MessagingManagerImpl::destroyCrossRefs()
{
	Trace_Entry(this, "destroyCrossRefs()");

	membershipManager_.reset();
	routingManager_.reset();
	commManager_.reset();

	Trace_Exit(this, "destroyCrossRefs()");
}

TopicPublisher_SPtr MessagingManagerImpl::createTopicPublisher(
		Topic_SPtr topic,
		PubSubEventListener& eventListener,
		const PropertyMap& configProp)
	throw (SpiderCastRuntimeError,SpiderCastLogicError)
{
	Trace_Entry(this, "createTopicPublisher()");

	TopicPublisherImpl_SPtr publisher(
			new TopicPublisherImpl(
					instID_, config_, nodeIDCache_,	coreInterface_,
					topic, eventListener, configProp,
					getNextStreamID()));

	addPublisher(publisher);

	Trace_Exit(this, "createTopicPublisher()");
	return boost::static_pointer_cast<TopicPublisher>(publisher);
}

TopicSubscriber_SPtr MessagingManagerImpl::createTopicSubscriber(
		Topic_SPtr topic,
		MessageListener& messageListener,
		PubSubEventListener& eventListener,
		const PropertyMap& configProp)
	throw (SpiderCastRuntimeError,SpiderCastLogicError)
{
	Trace_Entry(this, "createTopicSubscriber()");

	TopicSubscriberImpl_SPtr subscriber(new TopicSubscriberImpl(
			instID_,config_,nodeIDCache_,coreInterface_,
			topic,messageListener,eventListener,configProp));

	addSubscriber(subscriber);

	Trace_Exit(this, "createTopicSubscriber()");
	return boost::static_pointer_cast<TopicSubscriber>(subscriber);
}

P2PStreamRcv_SPtr MessagingManagerImpl::createP2PStreamRcv(
		MessageListener & p2PStreamMsgListener,
		P2PStreamEventListener& p2PStreamRcvEventListener,
		const PropertyMap& properties) throw (SpiderCastRuntimeError,
		SpiderCastLogicError)
{
	Trace_Entry(this, "createP2PStreamRcv()");

	if (_receiver)
	{
		String what("P2P receiver already exists ");
		throw SpiderCastLogicError(what);
	}

	P2PStreamRcvImpl_SPtr receiver(new P2PStreamRcvImpl(instID_, config_,
			nodeIDCache_, coreInterface_, p2PStreamMsgListener,
			p2PStreamRcvEventListener, properties));

	setP2PRcv(receiver);

	Trace_Exit(this, "createP2PStreamRcv()");
	return boost::static_pointer_cast<P2PStreamRcv>(receiver);

}

P2PStreamTx_SPtr MessagingManagerImpl::createP2PStreamTx(NodeID_SPtr target,
		P2PStreamEventListener& p2PStreamTxEventListener,
		const PropertyMap& properties) throw (SpiderCastRuntimeError,
		SpiderCastLogicError)
{

	Trace_Entry(this, "createP2PStreamTx()");

	NodeIDImpl_SPtr fullTarget =
			nodeIDCache_.getOrCreate(target->getNodeName());
	boost::shared_ptr<P2PStreamSyncCreationAdapter> adapter(new P2PStreamSyncCreationAdapter(
			_instanceID, config_.getMyNodeID()->getNodeName()));

	commManager_->connect(fullTarget, adapter.get(), P2PCtx);

	Neighbor_SPtr neighbor = adapter->waitForCompletion();

	//
	P2PStreamTxImpl_SPtr transmitter(new P2PStreamTxImpl(instID_, config_,
			nodeIDCache_, coreInterface_, p2PStreamTxEventListener, properties,
			getNextStreamID(), target, neighbor));


	Trace_Exit(this, "createP2PStreamTx()");
	return boost::static_pointer_cast<P2PStreamTx>(transmitter);

}

StreamID_SPtr MessagingManagerImpl::getNextStreamID()
{
	boost::recursive_mutex::scoped_lock lock(sidMutex_);
	return StreamID_SPtr(new StreamIDImpl(sidPrefix_,++sidSuffix_));
}

void MessagingManagerImpl::addPublisher(TopicPublisherImpl_SPtr publisher)
{
	Trace_Entry(this, "addPublisher()", spdr::toString<TopicPublisherImpl>(publisher));

	{
		boost::recursive_mutex::scoped_lock lock(pubsubMutex_);

		if (!closedPub_)
		{
			std::pair<StreamID2Publisher_Map::iterator,bool> res = publishers_by_StreamID_.insert(
					std::make_pair(publisher->getStreamID(),publisher));
			if (!res.second)
			{
				String what("Cannot add publisher: ");
				what.append(publisher->toString());
				throw SpiderCastRuntimeError(what);
			}

			TopicName2TopicPublisherSet_Map::iterator it = publisherSet_by_Topic_.find(
					publisher->getTopic()->getName());
			if (it != publisherSet_by_Topic_.end())
			{
				it->second.insert(publisher);
			}
			else
			{
				TopicPublisherSet set;
				set.insert(publisher);
				publisherSet_by_Topic_.insert(std::make_pair(publisher->getTopic()->getName(), set));
			}

			if (publisher->getTopic()->isGlobalScope())
			{
				addPublisher_Attribute(
					publisher->getTopic()->getName(),true);
			}

			Trace_Event(this, "addPublisher()", "ok"
				"TopicPublisher", spdr::toString<TopicPublisherImpl>(publisher));
		}
	}

	Trace_Exit(this, "addPublisher()");
}

void MessagingManagerImpl::removePublisher(StreamID_SPtr publisherSID)
{
	Trace_Entry(this, "removePublisher()",
			"StreamID", spdr::toString<StreamID>(publisherSID));

	{
		boost::recursive_mutex::scoped_lock lock(pubsubMutex_);

		if (!closedPub_)
		{
			StreamID2Publisher_Map::iterator pos = publishers_by_StreamID_.find(publisherSID);
			if (pos != publishers_by_StreamID_.end())
			{
				TopicPublisherImpl_SPtr pub = pos->second;
				publishers_by_StreamID_.erase(pos);

				//TODO remove from topic to set<publisher> map
				TopicName2TopicPublisherSet_Map::iterator it = publisherSet_by_Topic_.find(
						pub->getTopic()->getName());
				if (it != publisherSet_by_Topic_.end())
				{
					if (it->second.size() == 1)
					{
						publisherSet_by_Topic_.erase(it);
					}
					else
					{
						it->second.erase(pub);
					}
				}
				else
				{
					String what("Remove publisher failed (TopicName2TopicPublisherSet_Map): ");
					what.append(spdr::toString<TopicPublisherImpl>(pub));
					throw SpiderCastRuntimeError(what);
				}

				if (pub->getTopic()->isGlobalScope())
				{
					removePublisher_Attribute(pub->getTopic()->getName());
				}

				Trace_Event(this, "removePublisher()", "ok"
						"TopicPublisher", spdr::toString<TopicPublisherImpl>(pub));
			}
			else
			{
				String what("Remove publisher failed (find): ");
				what.append(spdr::toString<StreamID>(publisherSID));
				throw SpiderCastRuntimeError(what);
			}
		}
	}

	Trace_Exit(this, "removePublisher()");
}


void MessagingManagerImpl::addSubscriber(TopicSubscriberImpl_SPtr subscriber)
{
	Trace_Entry(this, "addSubscriber()", spdr::toString<TopicSubscriberImpl>(subscriber));

	{
		boost::recursive_mutex::scoped_lock lock(pubsubMutex_);

		if (closedSub_)
		{
			Trace_Exit(this, "addSubscriber()","closed");
			return;
		}

		std::pair<TopicName2Subscriber_Map::iterator,bool> res = subscriber_by_Topic_.insert(
				std::make_pair(subscriber->getTopic()->getName(),subscriber));

		if (!res.second)
		{
			String what("Subscriber already exists on Topic=");
			what.append(subscriber->getTopic()->getName());
			throw SpiderCastLogicError(what);
		}

		//update interest aware membership
		addSubscriber_Attribute(
				subscriber->getTopic()->getName(),
				subscriber->getTopic()->isGlobalScope());
	}

	//Update PubSubRouter
	routingManager_->getPubSubRouter().addLocalSubscriber(
			(int32_t)subscriber->getTopic()->hash_value());


	Trace_Exit(this, "addSubscriber()");
}

void MessagingManagerImpl::removeSubscriber(Topic_SPtr topic)
{
	Trace_Entry(this, "removeSubscriber()", spdr::toString<Topic>(topic));

	{
		boost::recursive_mutex::scoped_lock lock(pubsubMutex_);

		if (closedSub_)
		{
			Trace_Exit(this, "removeSubscriber()","closed");
			return;
		}

		std::size_t count = subscriber_by_Topic_.erase(topic->getName());

		if (count == 0)
		{
			String what("Subscriber does not exists on Topic=");
			what.append(topic->getName());
			throw SpiderCastRuntimeError(what);
		}

		//update interest aware membership
		removeSubscriber_Attribute(topic->getName());
	}



	//Update PubSubRouter
	routingManager_->getPubSubRouter().removeLocalSubscriber(
			(int32_t)topic->hash_value());

	Trace_Exit(this, "removeSubscriber()");
}

void MessagingManagerImpl::setP2PRcv(P2PStreamRcvImpl_SPtr receiver)
{
	Trace_Entry(this, "setP2PRcv()");
	_receiver = receiver;
	routingManager_->setP2PRcv(receiver);
	Trace_Exit(this, "setP2PRcv()");
}

void MessagingManagerImpl::removeP2PRcv()
{
	Trace_Entry(this, "removeP2PRcv()");
	_receiver = P2PStreamRcvImpl_SPtr();
	setP2PRcv(_receiver);
	Trace_Exit(this, "removeP2PRcv()");
}

void MessagingManagerImpl::processIncomingDataMessage(SCMessage_SPtr message)
{
	Trace_Entry(this, "processIncomingDataMessage()");

	SCMessage::H3HeaderStart h3start = message->readH3HeaderStart();

	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingDataMessage()");
		buffer->addProperty("TP", SCMessage::messageTransProtocolName[h3start.get<0>()]);
		buffer->addProperty("RM", SCMessage::messageReliabilityModeName[h3start.get<1>()]);
		buffer->invoke();
	}

	switch (h3start.get<0>()) //TransportProtocol
	{
	case SCMessage::TransProto_PubSub:
	{
		processIncomingPubSubDataMessage(message, h3start);
	}
	break;

	default:
	{
		//throw not supported
		String what("Not supported: TransportProtocol=");
		what.append(SCMessage::messageTransProtocolName[h3start.get<0>()]);
		throw SpiderCastRuntimeError(what);
	}

	}//switch

	Trace_Exit(this, "processIncomingDataMessage()");
}

void MessagingManagerImpl::processIncomingPubSubDataMessage(SCMessage_SPtr message,
		const SCMessage::H3HeaderStart& h3start)
{
	Trace_Entry(this, "processIncomingPubSubDataMessage()");

	ByteBuffer_SPtr bb = message->getBuffer();

	StreamID_SPtr sid = bb->readStreamID_SPtr();
	int64_t mid = bb->readLong();
	String topicName = bb->readString();
	String sourceName = bb->readString();

	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
	        ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingPubSubDataMessage()");
	        buffer->addProperty("sid", sid->toString());
	        buffer->addProperty<int64_t>("msgID", mid);
	        buffer->addProperty("topic", topicName);
	        buffer->addProperty("source", sourceName);
	        buffer->invoke();
	}

	TopicSubscriberImpl_SPtr sub;

	{
		boost::recursive_mutex::scoped_lock lock(pubsubMutex_);

		if (!closedSub_)
		{
			TopicName2Subscriber_Map::const_iterator pos = subscriber_by_Topic_.find(topicName);

			if (pos != subscriber_by_Topic_.end())
			{
				sub = pos->second;
			}
		}
	}

	if (sub)
	{
		RxMessageImpl& rx_msg = message->getRxMessage();
		rx_msg.setStreamID(sid);
		rx_msg.setMessageID(mid);
		rx_msg.setTopic(sub->getTopic());
		rx_msg.setSource(nodeIDCache_.getOrCreate(sourceName));

		sub->processIncomingDataMessage(message,h3start);
	}
	else
	{
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "processIncomingPubSubDataMessage()",
					"no subscriber, dropping message");
			buffer->invoke();
		}
	}

	Trace_Exit(this, "processIncomingPubSubDataMessage()");
}

void MessagingManagerImpl::addSubscriber_Attribute(const String& topicName, bool global)
{
	using namespace std;

	String topicKey(topicKey_Prefix);
	topicKey.append(topicName);
	pair<event::AttributeValue,bool> res = membershipManager_->getAttributeControl().getAttribute(topicKey);

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
			String what("Error: addSubscriber_Attribute() empty value on key ");
			what.append(topicKey);
			throw SpiderCastRuntimeError(what);
		}
	}

	flags = MessagingManager::addSub_Flags(flags,global);
	membershipManager_->getAttributeControl().setAttribute(
			topicKey,std::pair<int32_t,char*>(1,&flags));
}

void MessagingManagerImpl::removeSubscriber_Attribute(const String& topicName)
{
	String topicKey(topicKey_Prefix);
	topicKey.append(topicName);
	pair<event::AttributeValue,bool> res = membershipManager_->getAttributeControl().getAttribute(topicKey);

	if (res.second)
	{
		if (res.first.getLength() > 0)
		{
			char flags = (res.first.getBuffer())[0];
			flags = MessagingManager::removeSub_Flags(flags);

			if (flags > 0)
			{
				membershipManager_->getAttributeControl().setAttribute(
						topicKey,std::pair<int32_t,char*>(1,&flags));
			}
			else
			{
				membershipManager_->getAttributeControl().removeAttribute(topicKey);
			}
		}
		else
		{
			//Error, FAIL FAST
			String what("Error: removeSubscriber_Attribute() empty value on key ");
			what.append(topicKey);
			throw SpiderCastRuntimeError(what);
		}
	}
	else
	{
		//Error, FAIL FAST
		String what("Error: removeSubscriber_Attribute() missing value on key ");
		what.append(topicKey);
		throw SpiderCastRuntimeError(what);
	}
}


void MessagingManagerImpl::addPublisher_Attribute(const String& topicName, bool global)
{
	using namespace std;

	String topicKey(topicKey_Prefix);
	topicKey.append(topicName);
	pair<event::AttributeValue,bool> res = membershipManager_->getAttributeControl().getAttribute(topicKey);

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
			String what("Error: addPublisher_Attribute() empty value on key ");
			what.append(topicKey);
			throw SpiderCastRuntimeError(what);
		}
	}

	flags = MessagingManager::addPub_Flags(flags);

	membershipManager_->getAttributeControl().setAttribute(
			topicKey,std::pair<int32_t,char*>(1,&flags));
}

void MessagingManagerImpl::removePublisher_Attribute(const String& topicName)
{
	String topicKey(topicKey_Prefix);
	topicKey.append(topicName);
	pair<event::AttributeValue,bool> res = membershipManager_->getAttributeControl().getAttribute(topicKey);

	if (res.second)
	{
		if (res.first.getLength() > 0)
		{
			char flags = (res.first.getBuffer())[0];
			flags = MessagingManager::removePub_Flags(flags);

			if (flags > 0)
			{
				membershipManager_->getAttributeControl().setAttribute(
						topicKey,std::pair<int32_t,char*>(1,&flags));
			}
			else
			{
				membershipManager_->getAttributeControl().removeAttribute(topicKey);
			}
		}
		else
		{
			//Error, FAIL FAST
			String what("Error: removePublisher_Attribute() empty value on key ");
			what.append(topicKey);
			throw SpiderCastRuntimeError(what);
		}
	}
	else
	{
		//Error, FAIL FAST
		String what("Error: removePublisher_Attribute() missing value on key ");
		what.append(topicKey);
		throw SpiderCastRuntimeError(what);
	}
}


}

}
