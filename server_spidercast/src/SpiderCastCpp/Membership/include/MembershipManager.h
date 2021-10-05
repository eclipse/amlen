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
 * MembershipManager.h
 *
 *  Created on: 01/03/2010
 */

#ifndef MEMBERSHIPMANAGER_H_
#define MEMBERSHIPMANAGER_H_

//#include "ConcurrentSharedPtr.h"
#include "NodeIDImpl.h"
#include "SCMessage.h"
#include "AttributeControl.h"
#include "SCMembershipListener.h"
#include "MembershipEvent.h"

namespace spdr
{

class MembershipManager;
typedef boost::shared_ptr<MembershipManager> MembershipManager_SPtr;

/**
 *
 */
class MembershipManager
{
public:

	/**
	 * Identity of internal membership consumers
	 *
	 * Note: when changed, change also the name array:
	 * MembershipManager::intMemConsumerName
	 */
	enum InternalMembershipConsumer
	{
		IntMemConsumer_Hierarchy = 0,
		IntMemConsumer_PubSub,
		IntMemConsumer_HighPriorityMonitor,
		IntMemConsumer_LeaderElection,
		IntMemConsumer_Max
	};

protected:
	static const String intMemConsumerName[];

public:
	MembershipManager();
	virtual ~MembershipManager();

	/**
	 * Start the component.
	 *
	 * Threading: App
	 */
	virtual void start() = 0;

	/**
	 * Close the membership layer.
	 *
	 * Threading: App
	 *
	 * @param soft If true, sends LEAVE messages to neighbors to gracefully leave the overlay.
	 * @param removeRetained perform the leave with remove handshake
	 * @param timeout_millis timeout when waiting for LeaveAck
	 * @return whether ack was received
	 */
	virtual bool terminate(bool soft, bool removeRetained, int timeout_millis) = 0;

	/**
	 * Get a random node.
	 *
	 * Used by the topology manager to form a random overlay.
	 *
	 * Threading: MemTopoThread
	 *
	 * @return 	A random node from the view, i.e. nodes that are believed to be alive.
	 * 			Never returns the NodeID of itself. May return NULL if view is empty.
	 *
	 */
	virtual NodeIDImpl_SPtr getRandomNode() = 0;

	/**
	 * Get a randomized structured node.
	 *
	 * Used by the topology manager to form the randomized structured overlay.
	 * Returns a node according to the harmonic distribution, starting from this node;
	 * i.e. approx. 1/x.
	 *
	 * Threading: MemTopoThread
	 *
	 * @return 	A pair:
	 * 			1) A node from the view, i.e. nodes that are believed to be alive.
	 * 			Never returns the NodeID of itself. May return NULL if view is too small.
	 * 			2) Current view-size
	 *
	 */
	virtual std::pair<NodeIDImpl_SPtr,int> getRandomizedStructuredNode() = 0;

	/**
	 * Get a discovery node.
	 *
	 * Used by the topology manager during bootstrap, and periodically to heal partitions.
	 *
	 * Threading: MemTopoThread
	 *
	 * @return 	A pair containing:
	 * 			1) a node from the bootstrap set or the history,
	 * 			which is not necessarily alive. Never returns the NodeID of itself.
	 * 			May return NULL if view is empty. And,
	 * 			2) a boolean, with true if the node is from bootstrap, and false if from history.
	 */
	virtual std::pair<NodeIDImpl_SPtr, bool> getDiscoveryNode() = 0;

	/**
	 * Whether the given node is in the history set.
	 *
	 * @param discoveryNode
	 * @return
	 */
	virtual bool isFromHistorySet(NodeIDImpl_SPtr discoveryNode) = 0;

	/**
	 * Report a failure suspicion on a neighbor node.
	 *
	 * The topology manager will report nodes that caused communication failures
	 * such as: heartbeat-timeouts, failure to connect, etc.
	 *
	 * Threading: MemTopoThread
	 *
	 * @param suspectedNode
	 */
	virtual void reportSuspect(NodeIDImpl_SPtr suspectedNode) = 0;

	/**
	 * A suspicion on duplicate remote node.
	 *
	 * The topology manager will report a suspicion on duplicate remote node
	 * (two nodes running with the same name), stemming from a connect attempt
	 * in which a connection already exists to a node with the same name.
	 *
	 * Threading: MemTopoThread
	 *
	 * @param suspectedNode only name, no end-points
	 * @param incarnationNumber from the connect message
	 * @return whether the suspicion is true or false
	 */
	virtual bool reportSuspicionDuplicateRemoteNode(NodeIDImpl_SPtr suspectedNode, int64_t incarnationNumber) = 0;

	/**
	 * Report a new neighbor node.
	 *
	 * The topology manager will report new neighbors in order to expedite the delivery
	 * of full view to them.
	 *
	 *  Threading: MemTopoThread
	 *
	 * @param suspectedNode
	 */
	virtual void newNeighbor(NodeIDImpl_SPtr node) = 0;

	/**
	 * Report a disconnected neighbor node.
	 *
	 * The topology manager will report disconnected neighbors in order to
	 * purge and resubmit attribute requests sent to them.
	 *
	 * Threading: MemTopoThread
	 *
	 * @param suspectedNode
	 */
	virtual void disconnectedNeighbor(NodeIDImpl_SPtr node) = 0;


	/**
	 * Processes a incoming membership messages.
	 *
	 * Threading: MemTopoThread
	 *
	 * @param msg An incoming membership message.
	 *
	 */
	virtual void processIncomingMembershipMessage(
			SCMessage_SPtr membershipMsg) = 0;


	/**
	 * Used by Topology to prepare an outgoing multicast discovery message.
	 *
	 * Threading: MemTopoThread
	 *
	 * @return a copy of the current NodeVersion
	 */
	virtual NodeVersion getMyNodeVersion() const = 0;

	/**
	 * Used by topology to get a view in order to compose an outgoing discovery message.
	 *
	 * Writes the full view into the message.
	 *
	 * The message may be long, in particular longer than a UDP packet, this form is used
	 * with TCP discovery.
	 *
	 * Threading: MemTopoThread
	 *
	 * @param discoveryMsg
	 * 		an outgoing discovery message into which the discovery view of this node,
	 * 		the full view of the current node, will be written.
	 *
	 * @throw MessageMarshlingException if marshaling fails
	 */
	virtual void getDiscoveryView(SCMessage_SPtr discoveryMsg) = 0;

	/**
	 * Used by topology to get a partial view in order to compose an outgoing discovery message.
	 *
	 * Writes the current node-ID and the ID's of num_id other nodes.
	 * That is, the partial view size is (num_id+1).
	 *
	 * The message will never be longer than a UDP packet, therefore this form can be used
	 * with TCP or UDP discovery.
	 *
	 * Threading: MemTopoThread
	 *
	 * @param discoveryMsg
	 * 		an outgoing discovery message into which the discovery view of this node,
	 * 		a partial view of the current node, will be written.
	 * @param num_id
	 * 		the number of additional node ID's
	 *
	 * @throw MessageMarshlingException if marshaling fails
	 */
	virtual void getDiscoveryViewPartial(SCMessage_SPtr discoveryMsg, int num_id = 0) = 0;

	/**
	 * Used by topology to get a view in order to compose an outgoing discovery message.
	 *
	 * Writes the full view into the message vector.
	 * Each message is "self-contained", and contains a legal header and body.
	 *
	 * The first message in the vector must exist and contain a header, which is copied
	 * to the other messages as needed.
	 * The number of valid messages in the vector is returned.
	 * The vector is appended
	 *
	 * Each individual message will never be longer than a UDP packet,
	 * therefore this form can be used with UDP discovery.
	 *
	 * Each message is "finalized", i.e. total length and CRC (if applicable) are already written.
	 *
	 * Threading: MemTopoThread
	 *
	 * @param discoveryMsgMultipart
	 * 		an outgoing multi-part discovery message into which
	 * 		a full view of the current node, will be written.
	 *
	 * @return the number of valid messages in the vector.
	 *
	 * @throw MessageMarshlingException if marshaling fails
	 */
	 virtual int getDiscoveryViewMultipart(std::vector<SCMessage_SPtr>& discoveryMsgMultipart) = 0;

	/**
	 * Used by topology to inform membership of a discovery view received from a discovery peer.
	 *
	 * Threading: MemTopoThread
	 *
	 * @param discoveryMsg
	 * 		an incoming discovery message from which the discovery view of the sending node,
	 * 		generally the full view of that node, will be extracted.
	 * @param isRequest whether this is a request or reply
	 * @param isBootstrap whether the target node (this node) was in bootstrap or history of source node
	 *
	 * @throw MessageUnmarshlingException if unmarshaling fails
	 */
	virtual void processIncomingDiscoveryView(SCMessage_SPtr discoveryMsg, bool isRequest, bool isBootstrap) = 0;

	/**
	 * Used by topology to inform membership of a peer received by multicast discovery (either request or reply)
	 *
	 * Threading: MemTopoThread
	 *
	 * @param peerID
	 * @param ver
	 * @param isRequest
	 * @param isBoostrap
	 * @return if the view changed
	 */
	virtual bool processIncomingMulticastDiscoveryNodeView(NodeIDImpl_SPtr peerID, const NodeVersion& ver, bool isRequest, bool isBootstrap) = 0;


	/**
	 * Used by the membership service to manipulate attributes.
	 * That is, read, write, delete etc. of self node attributes.
	 *
	 * Threading: MemTopoThread, App
	 *
	 * @return
	 */
	virtual AttributeControl& getAttributeControl() = 0;

	/**
	 * Call by MembershipService upon close upon close of membership-service.
	 *
	 * Threading: MemTopoThread, App
	 */
	virtual void destroyMembershipService() = 0;

	virtual void destroyLeaderElectionService() = 0;

	/**
	 * Register an internal membership consumer.
	 *
	 * Threading: App, upon init.
	 *
	 * @param listener
	 * @param who
	 */
	virtual void registerInternalMembershipConsumer(
			boost::shared_ptr<SCMembershipListener> listener,
			InternalMembershipConsumer who) = 0;

	/**
	 * Used by HierarchyDelegate to get a full view in order to send a
	 * first view-update to the supervisor.
	 *
	 * The format is that of SCMessage::writeSCMembershipEvent(), with event-type View-Change.
	 *
	 * Threading: MemTopoThread
	 *
	 * @param viewupdateMsg
	 * 		an outgoing view-update message into which the full view of this node will be written.
	 * @param includeAttributes
	 * 		whether to include attributes in the view.
	 *
	 * @throw MessageMarshlingException if marshaling fails
	 */
	virtual void getDelegateFullView(SCMessage_SPtr viewupdateMsg, bool includeAttributes) = 0;

	/**
	 * The view size of the current node.
	 *
	 * Threading: MemTopoThread
	 *
	 * @return
	 */
	virtual int getViewSize() = 0;


	/**
	 * The reply of a Foreign-Zone-Membership request, on-success
	 *
	 * Threading: MemTopoThread
	 *
	 * @param requestID
	 * @param zoneBusName
	 * @param view
	 * @param lastEvent
	 */
	virtual void notifyForeignZoneMembership(
			const int64_t requestID, const String& zoneBusName,
			boost::shared_ptr<event::ViewMap > view, const bool lastEvent) = 0;

	/**
	 * The reply of a Foreign-Zone-Membership request, on-failure
	 *
	 * Threading: MemTopoThread
	 *
	 * @param requestID
	 * @param zoneBusName
	 * @param errorCode
	 * @param errorMessage
	 * @param lastEvent
	 */
	virtual void notifyForeignZoneMembership(
			const int64_t requestID, const String& zoneBusName,
			event::ErrorCode errorCode, const String& errorMessage, bool lastEvent) = 0;

	/**
	 * The reply of a Zone-Census request
	 *
	 * Threading: MemTopoThread
	 *
	 * @param requestID
	 * @param census
	 * @param full
	 */
	virtual void notifyZoneCensus(
			const int64_t requestID, event::ZoneCensus_SPtr census, const bool full) = 0 ;

	//=== tasks ==
	// Threading of all tasks: MemTopoThread

	/**
	 * Schedule a task to notify the application with metadata changes.
	 */
	virtual void scheduleChangeOfMetadataDeliveryTask() = 0;

	/**
	 * The periodic task, to be executed by the "run()" method of the task.
	 */
	virtual void periodicTask() = 0;

	/**
	 * The termination task, to be executed by the "run()" method of the task.
	 */
	virtual void terminationTask() = 0;

	/**
	 * The 2nd phase of the termination task, to be executed by the "run()" method of the task.
	 */
	virtual void terminationGraceTask() = 0;

	/**
	 * Clear retain attributes on a remote node
	 */
	virtual void clearRetainAttrTask() = 0;

	/**
	 * The first view delivery (to membership service) task, to be executed by the "run()" method of the task.
	 * Scheduled when the membership service is created.
	 */
	virtual void firstViewDeliveryTask() = 0;

	/**
	 * The neighbor change task, to be executed by the "run()" method of the task.
	 */
	virtual void neighborChangeTask() = 0;

	/**
	 * The Change of Metadata delivery task, to be executed by the "run()" method of the task.
	 */
	virtual void changeOfMetadataDeliveryTask() = 0;

	/**
	 * The refresh successor list task, to be executed by the "run()" method of the task.
	 *
	 * Scheduled when a report-suspect causes a view change,
	 * to avoid setting the successor from within the reportSuspect call.
	 */
	virtual void refreshSuccessorListTask() = 0;

	virtual bool clearRemoteNodeRetainedAttributes(NodeID_SPtr target, int64_t incarnation) = 0;


};

}

#endif /* MEMBERSHIPMANAGER_H_ */
