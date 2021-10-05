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
 * SupervisorPubSubBridge.h
 *
 *  Created on: Jul 8, 2012
 */

#ifndef SUPERVISORPUBSUBBRIDGE_H_
#define SUPERVISORPUBSUBBRIDGE_H_

#include <boost/shared_ptr.hpp>
#include <boost/unordered_set.hpp>

#include "SpiderCastConfigImpl.h"
#include "PubSubViewKeeper.h"
#include "AttributeControl.h"
#include "Neighbor.h"
#include "TopicImpl.h"

namespace spdr
{

namespace route
{

/**
 * 1. receive the base zone subscriber interest
 * 2. declare base zone subscriber interest as own interest in supervisor zone - create "bridge-subscriptions"
 * 3. get the publication list from base zone B (pub_B)
 * 4. reply with the intersection of pub_B with all the subscriptions in the supervisor zone,
 *
 */
class SupervisorPubSubBridge: public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	SupervisorPubSubBridge(
			const String& instID,
			const SpiderCastConfigImpl& config,
			boost::shared_ptr<PubSubViewKeeper> pubsubViewKeeper,
			AttributeControl& attributeControl);

	virtual ~SupervisorPubSubBridge();

	void close();


	void add_active(
			const String& busName,
			NodeIDImpl_SPtr id,
			Neighbor_SPtr neighbor) ;


	void remove_active(
			BusName_SPtr bus,
			NodeIDImpl_SPtr id) ;

	/**
	 * Send over S-Bridge data messages coming from the In-Q.
	 *
	 * Send to local delegates, according to subscription.
	 * Messages coming from the base-zone - exclude the source bus
	 * Messages coming from the mngt-zone - send all delegates
	 *
	 *
	 * @param msg
	 * @param h2
	 * @param h1
	 * @param exclude_bus may be NULL
	 * @return number of successful sends
	 */
	int sendToActiveDelegates(
			SCMessage_SPtr msg,
			const int32_t tid,
			const SCMessage::H2Header& h2,
			const SCMessage::H1Header& h1,
			BusName_SPtr exclude_bus);

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

private:
	const SpiderCastConfigImpl& config_;
	boost::shared_ptr<PubSubViewKeeper> pubsubViewKeeper_;
	AttributeControl& attributeControl_;

	typedef boost::unordered_set<int32_t> TopicID_HashSet;

	/**
	 * The datum associated with the active delegate of a base zone.
	 */
	struct DBridgeState
	{
		NodeIDImpl_SPtr delegate;
		Neighbor_SPtr neighbor;
		StringSet subscriptions; 			//topic names
		TopicID_HashSet subscription_TIDs;	//topic ID's
	};

	/**
	 * A map from bus name (base-zone) to a DBridgeState.
	 *
	 * Entries are added and removed by the add_active / remove_active calls.
	 * The subscriptions are updated by incoming Type_Hier_PubSubBridge_BaseZoneInterest messages.
	 */
	typedef std::map<String, DBridgeState > DBridgeStateTable;
	DBridgeStateTable dBridgeState_table_;

	/**
	 * A map from topic-name to a reference count.
	 * A table that counts the number of base-zones which subscribe to a topic.
	 * Derived from DBridgeStateTable.
	 */
	typedef std::map<String,int> BridgeSubsRefCountTable;
	BridgeSubsRefCountTable bridgeSubsRefCount_;

	/**
	 * The sets can overlap.
	 * First add the add_set to the ref-count. Add an attribute to new entries.
	 * Then remove the remove_set from the ref-count. Remove the attribute of entries with 0 refs.
	 *
	 * @param add_set
	 * @param remove_set
	 */
	void updateBridgeSubsAttributes(
			const StringSet& add_set, const StringSet& remove_set);

};

typedef boost::shared_ptr<SupervisorPubSubBridge> SupervisorPubSubBridge_SPtr;


}

}

#endif /* SUPERVISORPUBSUBBRIDGE_H_ */
