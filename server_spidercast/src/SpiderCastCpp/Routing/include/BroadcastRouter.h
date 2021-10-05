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
 * BroadcastRouter.h
 *
 *  Created on: Feb 1, 2012
 */

#ifndef BROADCASTROUTER_H_
#define BROADCASTROUTER_H_


#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>

#include "SpiderCastConfigImpl.h"
#include "TopicImpl.h"
#include "SCMessage.h"
#include "CoreInterface.h"
#include "VirtualIDCache.h"
#include "RoutingTableLookup.h"
#include "IncomingMsgQ.h"
#include "MessagingManager.h"
#include "PubSubBridge.h"


#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace route
{

class BroadcastRouter : boost::noncopyable, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	BroadcastRouter(
			const String& instID,
			const SpiderCastConfigImpl& config,
			CoreInterface& coreInterface,
			VirtualIDCache& vidCache,
			RoutingTableLookup& routingTable,
			PubSubBridge& pubSubBridge);

	virtual ~BroadcastRouter();

	void init();

	/**
	 * Route a message sent by this node.
	 *
	 * Routes to external nodes as well as to the local node.
	 * For the local receiver, the message will be copied to the
	 * local incoming message queue, with the LCL flag set.
	 *
	 * @param message
	 * @return whether sent to at least one neighbor
	 *
	 * @throw SpiderCastRuntimeError
	 */
	bool send(SCMessage_SPtr message) throw (SpiderCastRuntimeError);

	/**
	 * Route a message taken from the incoming message queue, coming from same zone.
	 *
	 * Buffer location is right after H2 header.
	 *
	 * Messages taken from the incoming message queue are either from the network
	 * (with LCL flag=0, or from the local node with LCL=1).
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
	 * route a message taken from the incoming message queue, coming from the bridge (different zone).
	 *
	 * This means the range is reset as if this node is the origin.
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


private:
	const String& instID_;
	const SpiderCastConfigImpl& config_;
	CoreInterface& coreInterface_;
	VirtualIDCache& vidCache_;
	RoutingTableLookup& routingTable_;
	PubSubBridge& pubsubBridge_;

	util::VirtualID_SPtr myVID_;
	IncomingMsgQ_SPtr incomingMsgQ_;
	messaging::MessagingManager_SPtr messagingManager_;

	/**
	 * Send to the successor and the neighbor closest to half range.
	 *
	 * Sets the buffer position to the data-length according to H1Header.
	 *
	 * @param message
	 * @param h2
	 * @param upperBound
	 * @return number of message sent
	 */
	int sendToRange(
			SCMessage_SPtr message,
			const SCMessage::H2Header& h2,
			const SCMessage::H1Header& h1,
			const util::VirtualID& upperBound)
		throw (SpiderCastRuntimeError);

};

}

}
#endif /* BROADCASTROUTER_H_ */
