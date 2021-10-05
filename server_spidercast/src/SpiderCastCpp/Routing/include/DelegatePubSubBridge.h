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
 * DelegatePubSubBridge.h
 *
 *  Created on: Jul 8, 2012
 */

#ifndef DELEGATEPUBSUBBRIDGE_H_
#define DELEGATEPUBSUBBRIDGE_H_

#include <boost/shared_ptr.hpp>

#include "SpiderCastConfigImpl.h"
#include "PubSubViewKeeper.h"
#include "Neighbor.h"
#include "CoreInterface.h"
#include "AttributeControl.h"
#include "RoutingManager.h"
#include "TaskSchedule.h"
#include "DBridgePubSubInterestTask.h"

namespace spdr
{
namespace route
{

/**
 *
 * Thread - unsafe, lock from without.
 */
class DelegatePubSubBridge : public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	DelegatePubSubBridge(
			const String& instID,
			const SpiderCastConfigImpl& config,
			boost::shared_ptr<PubSubViewKeeper> pubsubViewKeeper,
			CoreInterface& coreInterface);

	virtual ~DelegatePubSubBridge();

	/**
	 * Add all current global subscriptions and publications
	 *
	 * Thread: MemTopo
	 */
	void init();

	/**
	 * Remove all current global subscriptions and publications
	 *
	 * Thread: MemTopo
	 */
	void close(bool shutdown);

	/**
	 * Update the target supervisor.
	 *
	 * Tread: MemTopo
	 *
	 * @param targetSupervisor
	 */
	void setTargetSupervisor(Neighbor_SPtr targetSupervisor);

//	/**
//	 * Process messages coming from the In-Q.
//	 *
//	 * Those coming from the base-zone - transfer over bridge if needed.
//	 * Those coming from the mngt-zone - send to base-zone if needed.
//	 *
//	 * Thread: Routing-thread
//	 *
//	 * @param msg
//	 */
//	void processIncomingDataMessage(SCMessage_SPtr msg);

	/**
	 * Process control messages coming from In-Q.
	 *
	 * manage bridge subscriptions
	 *
	 * Thread: MemTopo-thread
	 *
	 * @param ctrlMsg
	 */
	void processIncomingControlMessage(SCMessage_SPtr ctrlMsg);

	/**
	 * Send message over the bridge to the supervisor.
	 *
	 * Thread: MemTopo, Routing
	 *
	 * @param msg
	 * @return true if send OK
	 */
	bool sendMessage(SCMessage_SPtr msg,
			const SCMessage::H2Header& h2,
			const SCMessage::H1Header& h1);

	/**
	 * When global publications change.
	 * form bridge subscription.
	 *
	 * MemTopoThread
	 */
	void globalPub_add(String topic_name);

	/**
	 * When global publications change.
	 * remove bridge subscription.
	 *
	 * MemTopoThread
	 */
	void globalPub_remove(String topic_name);

	/**
	 * When global subscriptions change.
	 * Get global subscriptions and send them to the supervisor over the bridge.
	 *
	 * MemTopoThread
	 */
	void globalSub_add(String topic_name);

	/**
	 * When global subscriptions change.
	 * Get global subscriptions and send them to the supervisor over the bridge.
	 *
	 * MemTopoThread
	 */
	void globalSub_remove(String topic_name);

	/**
	 * Push global subscriptions to S-Bridge
	 *
	 * MemTopoThread
	 *
	 * @see DBridgeTaskInterface
	 */
	void updatePubSubInterest();


private:
	bool closed_;
	const SpiderCastConfigImpl& config_;
	PubSubViewKeeper_SPtr pubsubViewKeeper_;
	AttributeControl& attributeControl_;
	Neighbor_SPtr targetSupervisor_;
	RoutingManager_SPtr routingManager_;
	TaskSchedule_SPtr memTopoTaskScheduler_;
	AbstractTask_SPtr updateInterestTask_;
	bool updateInterestTaskScheduled_;
	SCMessage_SPtr outgoingHierMessage_;

	void rescheduleInterestUpdateTask();

};

typedef boost::shared_ptr<DelegatePubSubBridge> DelegatePubSubBridge_SPtr;

}

}

#endif /* DELEGATEPUBSUBBRIDGE_H_ */
