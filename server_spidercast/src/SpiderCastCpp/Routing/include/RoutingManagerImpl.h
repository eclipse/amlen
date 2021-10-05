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
 * RoutingManagerImpl.h
 *
 *  Created on: Nov 7, 2011
 */

#ifndef ROUTINGMANAGERIMPL_H_
#define ROUTINGMANAGERIMPL_H_

#include <boost/noncopyable.hpp>

#include "RoutingManager.h"
#include "P2PRouter.h"
#include "BroadcastRouter.h"
#include "PubSubRouter.h"
#include "RoutingTable.h"
#include "CoreInterface.h"
#include "NodeIDCache.h"
#include "SpiderCastConfigImpl.h"
#include "ThreadControl.h"
#include "IncomingMsgQ.h"
#include "PubSubViewKeeper.h"
#include "MembershipManager.h"
#include "DelegatePubSubBridge.h"
#include "SupervisorPubSubBridge.h"
#include "PubSubViewListener.h"
#include "HierarchyManager.h"
#include "PubSubBridge.h"
#include "P2PStreamRcvImpl.h"

namespace spdr
{

class VirtualIDCache;

namespace route
{

class RoutingManagerImpl : boost::noncopyable, public RoutingManager, public ThreadControl,
	public PubSubViewListener, public PubSubBridge, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	RoutingManagerImpl(
			const String& instID,
			const SpiderCastConfigImpl& config,
			NodeIDCache& nodeIDCache,
			CoreInterface& coreInterface,
			VirtualIDCache& vidCache,
			IncomingMsgQ_SPtr incomingMsgQ);

	virtual ~RoutingManagerImpl();


	/*
	 * @see: RoutingManager
	 */
	void addRoutingNeighbor(NodeIDImpl_SPtr id, Neighbor_SPtr neighbor);

	/*
	 * @see: RoutingManager
	 */
	void removeRoutingNeighbor(NodeIDImpl_SPtr id, Neighbor_SPtr neighbor);

	/*
	 * @see: RoutingManager
	 */
	P2PRouter& getP2PRouter();

	/*
	 * @see: RoutingManager
	 */
	BroadcastRouter& getBroadcastRouter();

	/*
	 * @see: RoutingManager
	 */
	PubSubRouter& getPubSubRouter();

	/*
	 * @see: RoutingManager
	 *
	 * MemTopo
	 */
	void startDelegatePubSubBridge(Neighbor_SPtr targetSupervisor);

	/*
	 * @see: RoutingManager
	 */
	void stopDelegatePubSubBridge();

	/*
	 * @see: RoutingManager
	 */
	void supervisorPubSubBridge_add_active(
			const String& busName,
			NodeIDImpl_SPtr id,
			Neighbor_SPtr neighbor) ;

	/*
	 * @see: RoutingManager
	 */
	void supervisorPubSubBridge_remove_active(
			BusName_SPtr bus,
			NodeIDImpl_SPtr id) ;

	/*
	 * @see: RoutingManager
	 */
	void processIncomingDelegatePubSubBridgeControlMessage(SCMessage_SPtr msg);

	/*
	 * @see: RoutingManager
	 */
	void processIncomingSupervisorPubSubBridgeControlMessage(SCMessage_SPtr msg);

	/*
	 * @see: RoutingManager
	 *
	 * Push global subscriptions to S-Bridge
	 */
	void runDelegateBridgeUpdateInterestTask();

	//===

	/*
	 * @see RoutingTask
	 */
	bool runRoutingTask(bool workPending);


	void setP2PRcv(messaging::P2PStreamRcvImpl_SPtr receiver);
	//===

	/*
	 * @see ThreadControl
	 */
	void wakeUp(uint32_t mask);

	//===

	/*
	 * @see PubSubViewListener
	 */
	void globalPub_add(const String& topic_name);

	/*
	 * @see PubSubViewListener
	 */
	void globalPub_remove(const String& topic_name);

	/*
	 * @see PubSubViewListener
	 */
	void globalSub_add(const String& topic_name);

	/*
	 * @see PubSubViewListener
	 */
	void globalSub_remove(const String& topic_name);

	//===

	/*
	 * @see PubSubBridge
	 */
	void sendOverBridge(
			SCMessage_SPtr message,
			const SCMessage::H2Header& h2,
			const SCMessage::H1Header& h1);


	//===

	/**
	 * Create cross-references between the components.
	 * To be called right after construction but before start().
	 */
	void init();

	/**
	 * Destroy cross-references between the components.
	 * To be called right before destruction.
	 */
	void destroyCrossRefs();

	//void start();

	void terminate(bool soft);

private:
	const String& instID_;
	const SpiderCastConfigImpl& config_;
	NodeIDCache& nodeIDCache_;
	CoreInterface& coreInterface_;

	RoutingTable routingTable_;

	P2PRouter p2pRouter_;
	BroadcastRouter broadcastRouter_;
	boost::shared_ptr<PubSubViewKeeper> pubsubViewKeeper_;
	PubSubRouter pubsubRouter_;

	boost::recursive_mutex routingWorkMutex_;
	boost::condition_variable_any routingWorkCondVar_;
	uint32_t routingWorkPending_;

	bool closed_;

	IncomingMsgQ_SPtr incomingMsgQ_SPtr;

	const std::size_t msgsChunkSize;

	//=== hierarchical pub/sub ===
	boost::recursive_mutex pubsubBridgeMutex_;
	SupervisorPubSubBridge_SPtr supPubSubBridge_;
	/*
	 * Created, maintained, stopped, by: MemTopo thread
	 * Data sent by: MemTopoThread, Routing Thread
	 */
	DelegatePubSubBridge_SPtr delPubSubBridge_;

	messaging::P2PStreamRcvImpl_SPtr _receiver;

	/**
	 * Read a batch of messages from the incoming message queue and process them one by one.
	 *
	 * @return partial processing of the Q.
	 */
	bool processIncomingDataMessages();

	/**
	 * Process a single incoming data message.
	 *
	 * @param message
	 * @return
	 */
	void processIncomingDataMessage(SCMessage_SPtr message);

	void processIncomingP2PDataMessage(SCMessage_SPtr message);
	
	void sendOverSBridge(
			SCMessage_SPtr message,
			const SCMessage::H2Header& h2,
			const SCMessage::H1Header& h1);
};

}

}

#endif /* ROUTINGMANAGERIMPL_H_ */
