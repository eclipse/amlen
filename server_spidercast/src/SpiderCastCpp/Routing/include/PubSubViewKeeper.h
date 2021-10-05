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
 * PubSubViewKeeper.h
 *
 *  Created on: Feb 26, 2012
 */

#ifndef PUBSUBVIEWKEEPER_H_
#define PUBSUBVIEWKEEPER_H_

#include <set>
#include <algorithm>
#include <sstream>

#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/unordered_map.hpp>

#include "SCMembershipListener.h"
#include "SpiderCastConfigImpl.h"
#include "Topic.h"
#include "VirtualIDCache.h"
#include "MessagingManager.h"
#include "PubSubViewListener.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace route
{
//TODO prepare a topic-name cache

class PubSubViewKeeper: boost::noncopyable,
		public SCMembershipListener,
		public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	PubSubViewKeeper(
			const String& instID,
			const SpiderCastConfigImpl& config,
			VirtualIDCache& vidCache,
			PubSubViewListener& listener);

	virtual ~PubSubViewKeeper();

	/*
	 * @see spdr::SCMembershipListener
	 */
	void onMembershipEvent(const SCMembershipEvent& event);

	/**
	 * Get the closest node, CW on the ring, that is subscribed to the topic.
	 *
	 * The topic is identified by the topic ID, the boost::string_hash of the topic name.
	 *
	 * Never returns the node itself (me).
	 *
	 * @param topicID
	 * @return
	 */
	std::pair<NodeIDImpl_SPtr, util::VirtualID_SPtr> getClosestSubscriber(int32_t topicID) const;

	/**
	 * A set of topic ID's.
	 */
	typedef std::set<int32_t> TopicID_Set;

	/**
	 * A set of Node ID's.
	 */
	typedef std::set<NodeIDImpl_SPtr,spdr::SPtr_Less<NodeIDImpl> > NodeSet;

	/**
	 *
	 * @param node
	 * @return
	 */
	TopicID_Set getNodeSubscriptions(NodeIDImpl_SPtr node) const;

	/**
	 *
	 * @param topicID
	 * @return
	 */
	 NodeSet getTopicSubscribers(int32_t topicID) const;

	 /**
	  * The set of global topics with subscribers in the zone.
	  *
	  * Excludes bridge subscriptions which includes:
	  * (1) bridge subscriptions for base zones on supevisor
	  * (2) bridge subscriptions for global publeshers on delegates
	  * @return
	  */
	 StringSet getGlobalTopicSubscriptions() const;

	 /**
	  * The set of global topics with publishers in the zone
	  * @return
	  */
	 StringSet getGlobalTopicPublications() const;

	/**
	 * Put into the output map all topic hashs, taken from attributes starting with ".tpc." prefix
	 *
	 * @param eventMap input
	 * @param topicSet output
	 *
	 */


	 /**
	  * Get all subscriptions TopicIDs, all global subscriptions, and all global publishers.
	  *
	  * Note that bridge subscriptions are not included in the global subscriptions.
	  *
	  * @param eventMap
	  * @param all_subscriptions
	  * 	all topics with topicFlags_SubscriberMask on;
	  * @param global_subscriptions
	  * 	all topics with topicFlags_SubscriberMask and topicFlags_GlobalMask on
	  * @param global_publishers
	  * 	all topics with topicFlags_PublisherMask on
	  */
	void filter2TopicSet(
			const event::AttributeMap_SPtr& eventMap,
			TopicID_Set& all_subscriptions,
			StringSet& global_subscriptions,
			StringSet& global_publishers,
			StringSet& bridge_subscriptions) const;


private:

	mutable boost::mutex mutex_;

	const SpiderCastConfigImpl& config_;
	VirtualIDCache& vidCache_;
	PubSubViewListener& viewListener_;
	NodeIDImpl_SPtr myNodeID_;
	util::VirtualID_SPtr myVID_;

	typedef boost::unordered_map<NodeIDImpl_SPtr, TopicID_Set,
			SPtr_Hash<NodeIDImpl> , SPtr_Equals<NodeIDImpl> >
			NodeID2TopicIDSet_Map;
	/* All topics
	 * The subscription list (interest) of every node
	 * NodeID to a set of TopicIDs
	 */
	NodeID2TopicIDSet_Map node2subscriptionList_;

	typedef boost::unordered_map<int32_t, NodeSet > TopicID2NodeSet_Map;
	/* All topics
	 * The subscribers of every topic
	 * TopicID to all the subscribers
	 */
	TopicID2NodeSet_Map topicID2NodeSet_;

	typedef boost::unordered_map<int32_t,
		std::pair<NodeIDImpl_SPtr,util::VirtualID_SPtr> >
		TopicID2Subscriber_Map;
	/* All topics
	 * TopicID to closest subscriber NodeID
	 */
	TopicID2Subscriber_Map topicID2ClosestSubscriber_;

	typedef std::map<String,NodeSet > TopicName2NodeSet_Map;
	/* Global topics
	 * the global subscription list, all the global topics with subscribers in the zone
	 */
	TopicName2NodeSet_Map globalSubscriptionMap_;
	/* Global topics
	 * The global publication list, all the global topics with publishers in the zone
	 */
	TopicName2NodeSet_Map globalPublicationMap_;

	typedef boost::unordered_map<NodeIDImpl_SPtr, StringSet,
				SPtr_Hash<NodeIDImpl> , SPtr_Equals<NodeIDImpl> >
				NodeID2TopicNameSet_Map;
	/* Global topics
	 * the set of global topic subscriber for every node
	 */
	NodeID2TopicNameSet_Map globalSub_Node2Topic_;
	NodeID2TopicNameSet_Map globalPub_Node2Topic_;

	void addSubscription(NodeIDImpl_SPtr node, int32_t tid);
	void removeSubscription(NodeIDImpl_SPtr node, int32_t tid);


	void addGlobalSub(String topic_name, NodeIDImpl_SPtr node);
	void removeGlobalSub(String topic_name, NodeIDImpl_SPtr node);

	void addGlobalPub(String topic_name, NodeIDImpl_SPtr node);
	void removeGlobalPub(String topic_name, NodeIDImpl_SPtr node);

//	std::vector<int32_t> setDiffTopicID(const TopicID_Set& a, const TopicID_Set& b) const;
//	std::vector<String > setDiffString(const StringSet& a, const StringSet& b) const;

};

std::vector<int32_t> setDiffTopicID(const PubSubViewKeeper::TopicID_Set& a, const PubSubViewKeeper::TopicID_Set& b);
std::vector<String > setDiffString(const StringSet& a, const StringSet& b);

typedef boost::shared_ptr<PubSubViewKeeper> PubSubViewKeeper_SPtr;

}

}

#endif /* PUBSUBVIEWKEEPER_H_ */
