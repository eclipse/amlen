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
 * SpiderCast.h
 *
 *  Created on: 28/01/2010
 */

#ifndef SPDR_SPIDERCAST_H_
#define SPDR_SPIDERCAST_H_

#include "Definitions.h"

#include "SpiderCastLogicError.h"
#include "SpiderCastRuntimeError.h"
#include "MembershipService.h"
#include "MembershipListener.h"
#include "Topic.h"
#include "TopicPublisher.h"
#include "TopicSubscriber.h"
#include "PubSubEventListener.h"
#include "MessageListener.h"
#include "LeaderElectionListener.h"
#include "LeaderElectionService.h"
#include "P2PStreamTx.h"
#include "P2PStreamRcv.h"
#include "P2PStreamEventListener.h"

#include "PropertyMap.h"

namespace spdr {

/**
 * SpiderCast represents a node in the overlay and creates membership and communication services.
 *
 * SpiderCast is centered around the concept of a node. A node is a unique entity in the overlay,
 * that has a unique name and network endpoints. The node's identifier is the NodeID, which carries the
 * node name (String) and NetworkEndpoints.
 *
 * A node also belongs to a bus, which is identified by a string bus-name.
 * The bus defines an administrative zone that provides full membership.
 * Nodes with different bus names do not connect to each other.
 *
 * A SpiderCast instance is created using the SpiderCastFactory#createSpiderCast() method.
 *
 * In order to properly create a node some mandatory configuration parameters need to be supplied.
 * Those are encapsulated in the SpiderCastConfig, which is created using
 * the SpiderCastFactory#createSpiderCastConfig() method. The SpiderCastConfig class also allows additional optional
 * configuration parameters to be supplied.
 *
 * When a node is created it stays passive in an initialized state until it is started.
 * When the created instance of a SpiderCast node is started, it starts a phase in which it discovers
 * overlay peers and connects to a subset of them. Peers to which a node is connected to are called
 * neighbors. The node then proceeds with a gossip protocol that implements the membership and attribute
 * services, and establishes an internal representation of the overlay node membership.
 *
 * The services that a SpiderCast node provides, like the membership service, are all created by
 * factory methods from the SpiderCast instance.
 *
 * In general, the factory methods will throw a SpiderCastRuntimeError if creation fails due to
 * some unexpected errors, and a SpiderCastLogicError if creation fails due to errors that can
 * be inferred from the input parameters or node state.

 */
class SpiderCast
{
public:

	/**
	 * The local node state.
	 */
	enum NodeState
	{
		/**
		 * After creation, before start.
		 */
		Init,

		/**
		 * After start, working normally.
		 */
		Started,

		/**
		 * After close.
		 */
		Closed,

		/**
		 * In an error state (closed).
		 */
		Error
	};

	virtual ~SpiderCast();

	/**
	 * Start SpiderCast, join the overlay.
	 *
	 * @throw SpiderCastRuntimeError if fails to start because of some unexpected error
	 * @throw SpiderCastLogicError if start() is invoked while node state is not 'INIT'.
	 */
	virtual void start() throw(SpiderCastRuntimeError, SpiderCastLogicError) = 0;

	/**
	 * Close SpiderCast, leave the overlay.
	 *
	 * A closed instance cannot be restarted. Pending messages waiting for
     * transmission or delivery may be discarded.
     *
     * A soft close attempts to gracefully close communication services,
     * by trying to send messages in the internal buffers and execute close protocols.
     * A soft close may take several seconds, depending on the configuration of the
     * underlying communication infrastructure.
     *
     * A hard close will not execute the close protocols of transport entities
     * and does not wait for full transmission of its internal buffers.
     *
	 * @param soft
	 * 		if true, does a soft close, otherwise a hard close.
	 *
	 * @throw SpiderCastRuntimeError if fails to close because of some unexpected error
	 */
	virtual void close(bool soft) throw(SpiderCastRuntimeError) = 0;

	/**
	 * A soft close that signals the peers that they may remove all retained attributes of this node.
	 * Will block until the first acknowledgement from a neighbor, or until a timeout.
	 *
	 * @return whether an acknowledgement was received.
	 */
	virtual bool closeAndRemove(int timeout_millis) throw(SpiderCastRuntimeError) = 0;

	/**
	 * Create the membership service.
	 *
	 * Only a single MembershipService instance can be created per SpiderCast instance.
	 *
	 * <b>Configuration parameters: </b><p>
	 * <UL>
	 *  <LI> ForeignZoneMembershipTimeoutMillis - optional, integer, default = 10000.
	 * </UL>
	 *
	 *
	 * @param properties
	 * 		Configuration parameters for the membership service.
	 * @param membershipListener
	 * 		An implementation of the MembershipListener interface for receiving membership events.
	 * @return A shared pointer to an implementation of the MembershipService interface.
	 *
	 * @throw SpiderCastRuntimeError If fails to create because of some unexpected error
	 * @throw SpiderCastLogicError If invoked while node state is not NodeState#Started, or if the service already exists.
	 *
	 * @see MembershipEvent
	 * @see
	 */
	virtual MembershipService_SPtr createMembershipService(
			const PropertyMap& properties,
			MembershipListener& membershipListener)
		throw (SpiderCastRuntimeError, SpiderCastLogicError) = 0;

	/**
	 * Create a Topic.
	 *
	 * A topic can be created from a SpiderCast instance even before it is started, or after it is closed.
	 *
	 * @param name
	 * @param properties
	 * @return A boost::shared_ptr to a Topic implementation
	 */
	virtual messaging::Topic_SPtr createTopic(
			const String& name,
			const PropertyMap& properties = PropertyMap())
		throw (SpiderCastRuntimeError,SpiderCastLogicError)= 0;

	/**
	 * Create a TopicPublisher.
	 *
	 * A TopicPublisher is a source for a stream of messages on a specific topic,
	 * that maintains a specific quality of service.
	 *
	 * For example a publisher may maintain message ordering, where messages sent
	 * from a certain TopicPublisher are received in the same order in which they were sent.
     *
     * It is legal to create several publishers on the same Topic, in the same node.
     * The different publishers may even have different quality of service parameters.
     * For example one publisher may maintain reliable ordered delivery, whereas another
     * may maintain best-effort delivery.
     *
     * Messages sent from different publishers, on the same Topic, even if the publishers
     * have the same QoS, are not necessarily received in the order they were sent.
     *
     * Each topic publisher is uniquely identified by a StreamID.
     *
	 * @param topic
	 * 			The Topic object.
	 * @param eventListener
	 * 			The event listener delivers life-cycle events
     *          about the TopicPublisher.
	 * @param properties
	 * 			Configures the properties of the publisher.
	 *
	 * @return The TopicPublisher object.
	 *
	 * @throw SpiderCastRuntimeError if creation fails due to unexpected errors
	 * @throw SpiderCastLogicError If invoked while node state is not NodeState#Started; if the configuration properties are wrong or inconsistent.
	 *
	 * @see Topic
	 * @see StreamID
	 * @see TopicPublisher
	 */
	virtual messaging::TopicPublisher_SPtr createTopicPublisher(
			messaging::Topic_SPtr topic,
			messaging::PubSubEventListener& eventListener,
			const PropertyMap& properties = PropertyMap())
	throw (SpiderCastRuntimeError,SpiderCastLogicError)= 0;

	/**
	 * Create a TopicSubscriber.
	 *
	 * @param topic
	 * @param messageListener
	 * @param eventListener
	 * @param properties
	 * @return
	 */
	virtual messaging::TopicSubscriber_SPtr createTopicSubscriber(
			messaging::Topic_SPtr topic,
			messaging::MessageListener& messageListener,
			messaging::PubSubEventListener& eventListener,
			const PropertyMap& properties = PropertyMap())
	throw (SpiderCastRuntimeError,SpiderCastLogicError)= 0;

	/**
	 * Create the leader election service.
	 *
	 * @param electionListener a callback interface for election results
	 * @param candidate whether a candidate or an observer
	 * @param properties configuration properties
	 * @return
	 *
	 * @throw SpiderCastLogicError in case the service already exists, or the properties are wrong
	 */
	virtual leader_election::LeaderElectionService_SPtr createLeaderElectionService(
			leader_election::LeaderElectionListener& electionListener,
			bool candidate,
			const PropertyMap& properties = PropertyMap())
	throw (SpiderCastRuntimeError,SpiderCastLogicError)= 0;


	virtual messaging::P2PStreamTx_SPtr createP2PStreamTx(
			NodeID_SPtr target,
			messaging::P2PStreamEventListener& p2PStreamTxEventListener,
			const PropertyMap& properties = PropertyMap())
		throw (SpiderCastRuntimeError, SpiderCastLogicError) =  0;

	virtual messaging::P2PStreamRcv_SPtr createP2PStreamRcv(
			messaging:: MessageListener & p2PStreamMsgListener,
			messaging::P2PStreamEventListener& p2PStreamRcvEventListener,
				const PropertyMap& properties = PropertyMap())
		throw (SpiderCastRuntimeError,SpiderCastLogicError)= 0;

	/**
	 * Get the node's state.
	 *
	 * @return an enum of type NodeState.
	 */
	virtual NodeState getNodeState() const = 0;

	/**
	 * A string representation of the node identity and state.
	 *
	 * @return A string representation of the node identity and state.
	 */
	virtual String toString() const = 0;

	/**
	 * Get the node's NodeID, carrying the node name and network end-points.
	 *
	 * @return a shared pointer to a NodeID.
	 *
	 * @see NodeID
	 */
	virtual NodeID_SPtr getNodeID() const = 0;

	/**
	 * Get the node's bus-name.
	 *
	 * @return a string
	 *
	 */
	virtual String getBusName() const = 0;

	/**
	 * Get the node's incarnation number.
	 *
	 * The incarnation number is determined at creation time.
	 *
	 * @return The incarnation number.
	 */
	virtual int64_t getIncarnationNumber() const = 0;

	/**
	 * Convert the NodeState enum value to a string.
	 *
	 * @param nodeState
	 * @return a string representing the NodeState enum value.
	 */
	static String getNodeStateName(NodeState nodeState);

protected:
	/**
	 * Default constructor.
	 * @return
	 */
	SpiderCast();
	/**
	 * NodeState name array.
	 */
	static const String nodeStateName[];
};

/**
 * A shared pointer to a SpiderCast instance.
 */
typedef boost::shared_ptr<SpiderCast> 			SpiderCast_SPtr;

}

#endif /* SPDR_SPIDERCAST_H_ */
