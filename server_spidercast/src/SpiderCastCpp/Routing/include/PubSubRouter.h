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
 * PubSubRouter.h
 *
 *  Created on: Jan 31, 2012
 */

#ifndef PUBSUBROUTER_H_
#define PUBSUBROUTER_H_

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>

#include "SpiderCastConfigImpl.h"
#include "TopicImpl.h"
#include "SCMessage.h"
#include "CoreInterface.h"
#include "IncomingMsgQ.h"
#include "MessagingManager.h"
#include "VirtualIDCache.h"
#include "RoutingTableLookup.h"
#include "PubSubViewKeeper.h"
#include "PubSubBridge.h"


#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace route
{

class PubSubRouter : boost::noncopyable, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	PubSubRouter(
			const String& instID,
			const SpiderCastConfigImpl& config,
			CoreInterface& coreInterface,
			VirtualIDCache& vidCache,
			RoutingTableLookup& routingTable,
			PubSubBridge& pubSubBridge,
			boost::shared_ptr<PubSubViewKeeper> pubsubViewKeeper);

	virtual ~PubSubRouter();

	void init();

	/**
	 * Route a message sent by this node.
	 *
	 * Routes to external nodes as well as to the local node.
	 * If there is a local subscriber, the message will be copied to the
	 * local incoming message queue, with the LCL flag set.
	 *
	 * @param topic
	 * @param message
	 * @return whether sent to at least one neighbor
	 *
	 * @throw SpiderCastRuntimeError
	 */
	bool send(messaging::TopicImpl_SPtr topic, SCMessage_SPtr message)
		throw (SpiderCastRuntimeError);

	/**
	 * Route a message taken from the incoming message queue.
	 *
	 * Route to the network and deliver to local subscribers.
	 *
	 * Buffer location is right after H2 header.
	 *
	 * Messages taken from the incoming message queue are either from the network
	 * (with LCL flag=0, or from the local node with LCL=1).
	 *
	 * Route to the network and deliver to local subscribers.
	 *
	 * @param message
	 * @param h2
	 * @param h1
	 * @return true if it is a local message, false if from the network
	 */
	bool route(SCMessage_SPtr message,
			const SCMessage::H2Header& h2, const SCMessage::H1Header& h1)
		throw (SpiderCastRuntimeError);

	/**
	 * Route a message taken from the incoming message queue, coming from the bridge (different zone).
	 *
	 * This means the range is reset as if this node is the origin.
	 * Route to the network and deliver to local subscribers.
	 *
	 * Buffer location is right after H2 header.
	 *
	 * Messages taken from the incoming message queue are either from the network
	 * (with LCL flag=0, or from the local node with LCL=1).
	 *
	 * @param message
	 * @param h2
	 * @param h1
	 * @return true if sent to at least one neighbor
	 */
	bool route_FromBridge(SCMessage_SPtr message,
			const SCMessage::H2Header& h2, const SCMessage::H1Header& h1)
		throw (SpiderCastRuntimeError);

	void addLocalSubscriber(int32_t topicHash);
	void removeLocalSubscriber(int32_t topicHash);
	bool isLocalSubscriber(int32_t topicHash) const;

private:
	const String& instID_;
	const SpiderCastConfigImpl& config_;
	CoreInterface& coreInterface_;
	VirtualIDCache& vidCache_;
	RoutingTableLookup& routingTable_;
	PubSubBridge& pubSubBridge_;
	boost::shared_ptr<PubSubViewKeeper> pubsubViewKeeper_;

	util::VirtualID_SPtr myVID_;
	IncomingMsgQ_SPtr incomingMsgQ_;
	messaging::MessagingManager_SPtr messagingManager_;

	mutable boost::mutex filterMutex_;
	typedef boost::unordered_map<int32_t,int> TopicHash2Count_Map;
	TopicHash2Count_Map localSubscriptionFilter_;

	int sendToRange(
			SCMessage_SPtr message,
			int32_t topicID,
			const SCMessage::H2Header& h2,
			const SCMessage::H1Header& h1,
			const util::VirtualID& upperBound)
		throw (SpiderCastRuntimeError);

};

}

}

#endif /* PUBSUBROUTER_H_ */
