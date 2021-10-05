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
 * RoutingManager.h
 *
 *  Created on: Nov 7, 2011
 */

#ifndef ROUTINGMANAGER_H_
#define ROUTINGMANAGER_H_

#include "NodeIDImpl.h"
#include "Neighbor.h"
#include "P2PStreamRcvImpl.h"

namespace spdr
{

namespace route
{

class P2PRouter;
class BroadcastRouter;
class PubSubRouter;

class RoutingManager
{
public:
	RoutingManager();
	virtual ~RoutingManager();

	/**
	 *
	 * Note: The same ID can appear twice: as incoming and outgoing, with two different Neighbors.
	 *
	 * Threading: called by MemTopoThread
	 *
	 * @param id
	 * @param neighbor
	 */
	virtual void addRoutingNeighbor(NodeIDImpl_SPtr id, Neighbor_SPtr neighbor) = 0;

	/**
	 *
	 * Note: The same ID can appear twice: as incoming and outgoing, with two different Neighbors.
	 *
	 * Threading: called by MemTopoThread
	 *
	 * @param id
	 * @param neighbor
	 */
	virtual void removeRoutingNeighbor(NodeIDImpl_SPtr id, Neighbor_SPtr neighbor) = 0;

	virtual P2PRouter& getP2PRouter() = 0;

	virtual BroadcastRouter& getBroadcastRouter() = 0;

	virtual PubSubRouter& getPubSubRouter() = 0;

	/**
	 * Entry point of the routing thread
	 * @param workPending
	 * @return
	 */
	virtual bool runRoutingTask(bool workPending) = 0;

	virtual void setP2PRcv(messaging::P2PStreamRcvImpl_SPtr receiver) = 0;

	/**
	 * (1) When the delegate is the chosen active delegate in the zone, or
	 * (2) When (1) and target changed
	 *
	 * Thread: MemTopoThread
	 *
	 * @param targetSupervisor
	 */
	virtual void startDelegatePubSubBridge(Neighbor_SPtr targetSupervisor) = 0;

	/**
	 * When the delegate is no longer the chosen active delegate in the zone
	 *
	 * Thread: MemTopoThread
	 */
	virtual void stopDelegatePubSubBridge() = 0;

	/**
	 * When the supervisor activates a delegate.
	 *
	 * Thread: MemTopoThread
	 */
	virtual void supervisorPubSubBridge_add_active(
			const String& busName,
			NodeIDImpl_SPtr id,
			Neighbor_SPtr neighbor) = 0;

	/**
	 * When an active delegate is dis-activated, disconnects, or leaves
	 *
	 * Thread: MemTopoThread
	 */
	virtual void supervisorPubSubBridge_remove_active(
			BusName_SPtr bus,
			NodeIDImpl_SPtr id)  = 0;

	/**
	 * MemTopo-thread
	 *
	 * @param msg
	 */
	virtual void processIncomingDelegatePubSubBridgeControlMessage(SCMessage_SPtr msg) = 0;

	/**
	 * MemTopo-thread
	 * @param msg
	 */
	virtual void processIncomingSupervisorPubSubBridgeControlMessage(SCMessage_SPtr msg) = 0;

	/**
	 * Push interest to the S-Bridge
	 *
	 * MemTopo-thread
	 */
	virtual void runDelegateBridgeUpdateInterestTask() = 0;


};

typedef boost::shared_ptr<RoutingManager> RoutingManager_SPtr;

}

}

#endif /* ROUTINGMANAGER_H_ */
