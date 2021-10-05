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
 * SpiderCastImpl.h
 *
 *  Created on: 02/02/2010
 */

#ifndef SPDR_SPIDERCASTIMPL_H_
#define SPDR_SPIDERCASTIMPL_H_

#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "Definitions.h"
#include "SpiderCast.h"
#include "CoreInterface.h"
#include "SpiderCastConfigImpl.h"
#include "SpiderCastEventListener.h"
#include "MemTopoThread.h"
#include "SpiderCastRuntimeError.h"
#include "MembershipManagerImpl.h"
#include "TopologyManagerImpl.h"
#include "NodeIDCache.h"
#include "VirtualIDCache.h"
#include "HierarchyManagerImpl.h"
#include "RoutingManagerImpl.h"
#include "MessagingManagerImpl.h"
#include "StatisticsTask.h"
#include "Topic.h"
#include "RoutingThread.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class SpiderCastImpl: boost::noncopyable,
		public SpiderCast,
		public CoreInterface,
		public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	SpiderCastImpl(const String instID, SpiderCastConfig& spiderCastConfig,
			SpiderCastEventListener& listener) throw (SpiderCastRuntimeError);

	virtual ~SpiderCastImpl();

	/*
	 * @see spdr::SpiderCast
	 */
	void start() throw (SpiderCastRuntimeError, SpiderCastLogicError);

	/*
	 * @see spdr::SpiderCast
	 */
	void close(bool soft) throw (SpiderCastRuntimeError);

	/*
	 * @see spdr::SpiderCast
	 */
	bool closeAndRemove(int timeout_millis) throw (SpiderCastRuntimeError);

	/*
	 * @see spdr::SpiderCast
	 */
	String toString() const;

	/*
	 * @see spdr::SpiderCast
	 */
	MembershipService_SPtr createMembershipService(
			const PropertyMap& properties,
			MembershipListener& membershipListener)
		throw (SpiderCastRuntimeError, SpiderCastLogicError);

	/*
	 * @see spdr::SpiderCast
	 */
	messaging::Topic_SPtr createTopic(
			const String& name,
			const PropertyMap& properties)
			throw (SpiderCastRuntimeError, SpiderCastLogicError);

	/*
	 * @see spdr::SpiderCast
	 */
	messaging::TopicPublisher_SPtr createTopicPublisher(
			messaging::Topic_SPtr topic,
			messaging::PubSubEventListener& eventListener,
			const PropertyMap& config)
	throw (SpiderCastRuntimeError,SpiderCastLogicError);

	/*
	 * @see spdr::SpiderCast
	 */
	messaging::TopicSubscriber_SPtr createTopicSubscriber(
			messaging::Topic_SPtr topic,
			messaging::MessageListener& messageListener,
			messaging::PubSubEventListener& eventListener,
			const PropertyMap& config)
	throw (SpiderCastRuntimeError,SpiderCastLogicError);

	/*
	 * @see spdr::SpiderCast
	 */
	leader_election::LeaderElectionService_SPtr createLeaderElectionService(
				leader_election::LeaderElectionListener& electionListener,
				bool candidate,
				const PropertyMap& properties)
	throw (SpiderCastRuntimeError,SpiderCastLogicError);


	messaging::P2PStreamTx_SPtr createP2PStreamTx(NodeID_SPtr target,
			messaging::P2PStreamEventListener& p2PStreamTxEventListener,
			const PropertyMap& properties)
	throw (SpiderCastRuntimeError, SpiderCastLogicError);

	messaging::P2PStreamRcv_SPtr createP2PStreamRcv(
			messaging:: MessageListener & p2PStreamMsgListener,
			messaging::P2PStreamEventListener& p2PStreamRcvEventListener,
			const PropertyMap& properties)
	throw (SpiderCastRuntimeError,SpiderCastLogicError);
	/*
	 * @see spdr::SpiderCast
	 */
	SpiderCast::NodeState getNodeState() const;

	/*
	 * @see spdr::SpiderCast
	 */
	NodeID_SPtr getNodeID() const;

	/*
	 * @see spdr::SpiderCast
	 */
	String getBusName() const;

	/*
	 * @see spdr::CoreInterface
	 */
	MembershipManager_SPtr getMembershipManager();

	/*
	 * @see spdr::CoreInterface
	 */
	TopologyManager_SPtr getTopologyManager();

	/*
	 * @see spdr::CoreInterface
	 */
	CommAdapter_SPtr getCommAdapter();

	/*
	 * @see spdr::CoreInterface
	 */
	HierarchyManager_SPtr getHierarchyManager();

	/*
	 * @see spdr::CoreInterface
	 */
	boost::shared_ptr<route::RoutingManager> getRoutingManager();

	/*
	 * @see spdr::CoreInterface
	 */
	boost::shared_ptr<messaging::MessagingManager> getMessagingManager();

	/*
	 * @see spdr::CoreInterface
	 */
	TaskSchedule_SPtr getTopoMemTaskSchedule();

	/*
	 * @see spdr::CoreInterface
	 */
	int64_t getIncarnationNumber() const;

	/*
	 * @see spdr::CoreInterface
	 */
	void threadFailure(const String& threadName, std::exception& ex);

	/*
	 * @see spdr::CoreInterface
	 */
	void componentFailure(const String& errMsg, event::ErrorCode errCode);

	/*
	 * @see spdr::CoreInterface
	 */
	const String& getInstanceID() const;

	/*
	 * @see spdr::CoreInterface
	 */
	void reportStats(bool labels);

	/*
	 * @see spdr::CoreInterface
	 */
	void submitExternalEvent(event::SpiderCastEvent_SPtr event);
	/*
	 * @see ScTraceContext
	 */
	const ScTraceComponent* getTraceComponent() const
	{
		return tc_;
	}

private:

	const String instanceID;

	SpiderCastConfigImpl configImpl;
	SpiderCastEventListener& eventListener;

	mutable boost::recursive_mutex state_mutex;
	NodeState state;

	MemTopoThread_SPtr memTopoThread_SPtr;

	CommAdapter_SPtr commAdapter_SPtr;

	TopologyManager_SPtr topoMngr_SPtr;
	TopologyManagerImpl* topoMngrImpl_ptr; //deleted by topoMngr_SPtr

	boost::shared_ptr<MembershipManagerImpl> memMngr_SPtr;
	//MembershipManagerImpl* memMngrImpl_ptr; //deleted by memMngr_SPtr

	HierarchyManager_SPtr hierMngr_SPtr;
	HierarchyManagerImpl* hierMngrImpl_ptr; //deleted by hierMngr_SPtr;

	boost::shared_ptr<route::RoutingManagerImpl> routingManagerImpl_SPtr;

	boost::shared_ptr<messaging::MessagingManagerImpl> messagingManagerImpl_SPtr;

	boost::shared_ptr<route::RoutingThread> routingThread_SPtr;

	TaskSchedule_SPtr memTopoTaskSchedule_CSptr;

	int64_t incarnationNumber;

	NodeIDCache nodeIDCache;

	VirtualIDCache nodeVIDCache; //for node-VIDs

	AbstractTask_SPtr statisticsTask_SPtr;

	//=== private methods ===
	void printRefCount();
	bool internalClose(bool soft, bool removeRetained, bool onFailure, int timeout_millis);
};

} // namespace spdr

#endif /* SPDR_SPIDERCASTIMPL_H_ */
