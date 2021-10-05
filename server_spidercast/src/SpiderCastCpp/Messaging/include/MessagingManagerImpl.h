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
 * MessagingManagerImpl.h
 *
 *  Created on: Feb 1, 2012
 */

#ifndef MESSAGINGMANAGERIMPL_H_
#define MESSAGINGMANAGERIMPL_H_

#include <boost/noncopyable.hpp>

#include "MessagingManager.h"
#include "NodeIDCache.h"
#include "VirtualIDCache.h"
#include "SpiderCastConfigImpl.h"
#include "RoutingManager.h"
#include "PubSubRouter.h"
#include "CoreInterface.h"

#include "StreamIDImpl.h"
#include "TopicPublisherImpl.h"
#include "TopicSubscriberImpl.h"
#include "P2PStreamRcvImpl.h"
#include "P2PStreamTxImpl.h"
//#include "Neighbor.h"
#include "RumNeighbor.h"
#include "CommEventInfo.h"
#include "P2PStreamSyncCreationAdapter.h"



namespace spdr
{

namespace messaging
{

class MessagingManagerImpl : boost::noncopyable, public MessagingManager, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	//const static String topicKey_Prefix; //".sub."

	MessagingManagerImpl(
			const String& instID,
			const SpiderCastConfigImpl& config,
			NodeIDCache& nodeIDCache,
			VirtualIDCache& vidCache,
			CoreInterface& coreInterface);

	virtual ~MessagingManagerImpl();

	/*
	 * @see MessagingManager
	 */
	void removePublisher(StreamID_SPtr publisherSID);

	/*
	 * @see MessagingManager
	 */
	void removeSubscriber(Topic_SPtr topic);

	void removeP2PRcv();

	/*
	 * @see MessagingManager
	 */
	void processIncomingDataMessage(SCMessage_SPtr message);

	/**
	 * Create cross-references between the components.
	 * To be called right after construction but before start().
	 */
	void init();

	/**
	 * Terminate.
	 *
	 * Close all publishers, close all subscribers, etc.
	 *
	 * @param soft
	 */
	void terminate(bool soft);

	/**
	 * Destroy cross-references between the components.
	 * To be called right before destruction.
	 */
	void destroyCrossRefs();

	/**
	 *
	 * @param topic
	 * @param eventListener
	 * @param configProp
	 * @return
	 */
	TopicPublisher_SPtr createTopicPublisher(
			Topic_SPtr topic,
			PubSubEventListener& eventListener,
			const PropertyMap& configProp)
		throw (SpiderCastRuntimeError,SpiderCastLogicError);

	/**
	 *
	 * @param topic
	 * @param eventListener
	 * @param config
	 * @return
	 */
	TopicSubscriber_SPtr createTopicSubscriber(
			Topic_SPtr topic,
			MessageListener& messageListener,
			PubSubEventListener& eventListener,
			const PropertyMap& configProp)
		throw (SpiderCastRuntimeError,SpiderCastLogicError);


	P2PStreamRcv_SPtr createP2PStreamRcv(
			MessageListener & p2PStreamMsgListener,
			P2PStreamEventListener& p2PStreamRcvEventListener,
			const PropertyMap& properties)
		throw (SpiderCastRuntimeError,SpiderCastLogicError);

	P2PStreamTx_SPtr createP2PStreamTx(
			NodeID_SPtr target,
			P2PStreamEventListener& p2PStreamTxEventListener,
			const PropertyMap& properties)
	throw (SpiderCastRuntimeError,SpiderCastLogicError);


private:
	const String& instID_;
	const SpiderCastConfigImpl& config_;
	NodeIDCache& nodeIDCache_;
	VirtualIDCache& nodeVirtualIDCache_;
	CoreInterface& coreInterface_;
	MembershipManager_SPtr membershipManager_;
	route::RoutingManager_SPtr routingManager_;
	CommAdapter_SPtr commManager_;

	//=== stream ID ===
	boost::recursive_mutex sidMutex_;
	const uint64_t sidPrefix_; //lower 64 bit of VID
	uint64_t sidSuffix_; //upper 32 - incarnation number (start time uS), lower 32 - 0.

	//=== PubSub ============================================================
	boost::recursive_mutex pubsubMutex_;

	//=== Publishers ===
	typedef boost::unordered_map<StreamID_SPtr,TopicPublisherImpl_SPtr,
			spdr::SPtr_Hash<StreamID>, spdr::SPtr_Equals<StreamID> > StreamID2Publisher_Map;
	StreamID2Publisher_Map publishers_by_StreamID_;

	typedef std::set<TopicPublisherImpl_SPtr, spdr::SPtr_Less<TopicPublisherImpl> > TopicPublisherSet;
	typedef boost::unordered_map<String,TopicPublisherSet > TopicName2TopicPublisherSet_Map;
	TopicName2TopicPublisherSet_Map publisherSet_by_Topic_;

	bool closedPub_;

	//=== Subscribers ===
	typedef boost::unordered_map<String,TopicSubscriberImpl_SPtr > TopicName2Subscriber_Map;
	TopicName2Subscriber_Map subscriber_by_Topic_;

	bool closedSub_;

	//=== P2P receiver ===
	P2PStreamRcvImpl_SPtr _receiver;

	//=== private methods ====================================================

	StreamID_SPtr getNextStreamID();

	void addPublisher(TopicPublisherImpl_SPtr publisher);

	void addSubscriber(TopicSubscriberImpl_SPtr subscriber);

	void setP2PRcv(P2PStreamRcvImpl_SPtr receiver);


	/**
	 * Buffer location is after H3-start.
	 *
	 * @param message
	 * @param h3start
	 */
	void processIncomingPubSubDataMessage(SCMessage_SPtr message,
			const SCMessage::H3HeaderStart& h3start);

	void addSubscriber_Attribute(const String& topicName, bool global);

	void removeSubscriber_Attribute(const String& topicName);

	void addPublisher_Attribute(const String& topicName, bool global);

	void removePublisher_Attribute(const String& topicName);


};

}

}

#endif /* MESSAGINGMANAGERIMPL_H_ */
