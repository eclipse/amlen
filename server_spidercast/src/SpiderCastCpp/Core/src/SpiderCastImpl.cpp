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
 * SpiderCastImpl.cpp
 *
 *  Created on: 02/02/2010
 */

#include <iostream>

#include "SpiderCastImpl.h"

namespace spdr
{
using namespace std;
using namespace trace;

ScTraceComponent* SpiderCastImpl::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Core,
		trace::ScTrConstants::Layer_ID_Core,
		"SpiderCastImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

SpiderCastImpl::SpiderCastImpl(
		const String instID,
		SpiderCastConfig& config,
		SpiderCastEventListener& listener) throw (SpiderCastRuntimeError) :
	ScTraceContext(tc_,instID,""),
	instanceID(instID),
	configImpl(config),
	eventListener(listener),
	state(SpiderCast::Init),
	memTopoThread_SPtr(),
	commAdapter_SPtr(),
	topoMngr_SPtr(),
	topoMngrImpl_ptr(NULL),
	memMngr_SPtr(),
	//memMngrImpl_ptr(NULL),
	hierMngr_SPtr(),
	hierMngrImpl_ptr(NULL),
	routingManagerImpl_SPtr(),
	messagingManagerImpl_SPtr(),
	routingThread_SPtr(),
	memTopoTaskSchedule_CSptr(),
	incarnationNumber(-1),
	nodeIDCache(),
	nodeVIDCache(),
	statisticsTask_SPtr()
{
	using namespace boost::posix_time;
	using namespace boost::gregorian;

	Trace_Entry(this,"SpiderCastImpl()", "config", config.toString());

	printRefCount();

	//get the incarnation number
	//startup time as micro-sec since 1-Jan-1970
	ptime time = boost::get_system_time();
	ptime epoch(date(1970,1,1), microseconds(0));
	time_duration diff = time - epoch;
	incarnationNumber = diff.ticks() / (time_duration::ticks_per_second()/1000000); //micro, not nano

	if (configImpl.getChooseIncarnationNumberHigherThen() >=0)
	{
		if (configImpl.getChooseIncarnationNumberHigherThen() >= incarnationNumber)
		{
			Trace_Event(this,"SpiderCastImpl()", "Incarnation number adjusted to be higher than",
					"config-higher-than", boost::lexical_cast<std::string>(configImpl.getChooseIncarnationNumberHigherThen()),
					"time", boost::lexical_cast<std::string>(incarnationNumber));
			incarnationNumber = configImpl.getChooseIncarnationNumberHigherThen()+1;
		}
	}
	else if (configImpl.getForceIncarnationNumber() >= 0)
	{
		Trace_Event(this,"SpiderCastImpl()", "Incarnation forced to configuration value",
				"config-force", boost::lexical_cast<std::string>(configImpl.getForceIncarnationNumber()),
				"time", boost::lexical_cast<std::string>(incarnationNumber));
		incarnationNumber = configImpl.getForceIncarnationNumber();
	}

	try
	{
		commAdapter_SPtr = CommAdapter::getInstance(instanceID, configImpl, nodeIDCache, incarnationNumber);
	}
	catch (SpiderCastRuntimeError& e)
	{
		String what("CommAdapter creation failed; ");
		what += e.what();

		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "SpiderCastImpl()","CommAdapter creation failed");
		tb->addProperty("SpiderCastRuntimeError",e.what());
		tb->invoke();

		throw;
	}
	catch (std::exception& ex)
	{
		String what("CommAdapter creation failed; ");
		what += ex.what();

		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "SpiderCastImpl()","CommAdapter creation failed");
		tb->addProperty("what",ex.what());
		tb->addProperty("typeid", typeid(ex).name());
		tb->invoke();

		throw SpiderCastRuntimeError(ex.what());
	}

	{
		memTopoThread_SPtr = MemTopoThread_SPtr(
				new MemTopoThread(instanceID, configImpl, *this));

		//TODO number of routing threads
		routingThread_SPtr = boost::shared_ptr<route::RoutingThread>(
				new route::RoutingThread(instID,configImpl,*this));

		commAdapter_SPtr->getIncomingMessageQ()->registerReaderThread(
				memTopoThread_SPtr.get(), IncomingMsgQ::MembershipQ);
		commAdapter_SPtr->getIncomingMessageQ()->registerReaderThread(
				memTopoThread_SPtr.get(), IncomingMsgQ::TopologyQ);
	}

	//memMngrImpl_ptr = new MembershipManagerImpl(instanceID, configImpl, nodeIDCache, nodeVIDCache, *this);
	memMngr_SPtr.reset(new MembershipManagerImpl(instanceID, configImpl, nodeIDCache, nodeVIDCache, *this));

	topoMngrImpl_ptr = new TopologyManagerImpl(instanceID, configImpl, nodeIDCache, *this);
	topoMngr_SPtr = TopologyManager_SPtr(topoMngrImpl_ptr);

	hierMngrImpl_ptr = new HierarchyManagerImpl(instanceID, configImpl, nodeIDCache, nodeVIDCache, *this);
	hierMngr_SPtr = HierarchyManager_SPtr(hierMngrImpl_ptr);

	routingManagerImpl_SPtr.reset(new route::RoutingManagerImpl(
			instanceID, configImpl, nodeIDCache, *this, nodeVIDCache, commAdapter_SPtr->getIncomingMessageQ()));

	commAdapter_SPtr->getIncomingMessageQ()->registerReaderThread(
							routingManagerImpl_SPtr.get(), IncomingMsgQ::DataQ);

	messagingManagerImpl_SPtr.reset(new messaging::MessagingManagerImpl(
				instanceID, configImpl, nodeIDCache, nodeVIDCache, *this));

	memTopoTaskSchedule_CSptr = TaskSchedule_SPtr(
			new TaskSchedule( *memTopoThread_SPtr ));

	//init solves the problem of cross referencing of these two units
	memMngr_SPtr->init();
	topoMngrImpl_ptr->init();

	if (configImpl.isHierarchyEnabled())
	{
		hierMngrImpl_ptr->init();
	}

	if (configImpl.isRoutingEnabled())
	{
		routingManagerImpl_SPtr->init();
		messagingManagerImpl_SPtr->init();
	}

	Trace_Config(this,"SpiderCastImpl()", "SpiderCast Created",
			"Config", config.toString(),
			"IncarnationNumber", boost::lexical_cast<std::string>(incarnationNumber));

	printRefCount();

	Trace_Exit(this,"SpiderCastImpl()");
}

SpiderCastImpl::~SpiderCastImpl()
{
	Trace_Entry(this,"~SpiderCastImpl()");

	memTopoTaskSchedule_CSptr->cancel();
	memMngr_SPtr->destroyCrossRefs();
	topoMngrImpl_ptr->destroyCrossRefs();
	hierMngrImpl_ptr->destroyCrossRefs();
	routingManagerImpl_SPtr->destroyCrossRefs();
	messagingManagerImpl_SPtr->destroyCrossRefs();

	printRefCount();

	Trace_Exit(this,"~SpiderCastImpl()");
}

void SpiderCastImpl::printRefCount()
{
	ostringstream oss;
	oss << instanceID << ": SpiderCastImpl" <<
			" memMngr:" << memMngr_SPtr.use_count() <<
			" topoMngr:" << topoMngr_SPtr.use_count() <<
			" memTopoThread:" << memTopoThread_SPtr.use_count() <<
			" routingThread: " << routingThread_SPtr.use_count() <<
			" commAdapter:" << commAdapter_SPtr.use_count() <<
			" hierMngr:" << hierMngr_SPtr.use_count() <<
			" routingMngr:" << routingManagerImpl_SPtr.use_count() <<
			" messagingMngr:" << messagingManagerImpl_SPtr.use_count() <<
			" memTopoTaskSchedule:" << memTopoTaskSchedule_CSptr.use_count() <<
			endl;
	if (memTopoTaskSchedule_CSptr)
	{
		oss << " memTopoTaskSchedule(size):" << memTopoTaskSchedule_CSptr->size() << endl;
	}

	Trace_Debug(this,"printRefCount()",oss.str());
}

void SpiderCastImpl::start() throw (SpiderCastRuntimeError, SpiderCastLogicError)
{
	Trace_Entry(this,"start()");

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state != SpiderCast::Init)
		{
			String what = "SpiderCast instance in state " + SpiderCast::nodeStateName[state];
			Trace_Exit(this, "start()", "SpiderCastLogicError", what);
			throw SpiderCastLogicError(what);
		}

		try
		{
			commAdapter_SPtr->start();
		}
		catch (SpiderCastRuntimeError& e)
		{

			state = SpiderCast::Error;

			memMngr_SPtr->terminate(false,false,0);
			topoMngr_SPtr->terminate(false);

			String what("Failed to start CommAdapter; ");
			what += e.what();
			std::cerr << what << std::endl;

			Trace_Exit(this,"start()", "SpiderCastRuntimeError", what);

			throw;
		}

		//start order is important - start threads last
		memMngr_SPtr->start();
		topoMngr_SPtr->start();

		if (configImpl.isHierarchyEnabled())
		{
			hierMngr_SPtr->start();
		}

		memTopoThread_SPtr->start();

		//TODO number of routing threads
		if (configImpl.isRoutingEnabled())
		{
			routingThread_SPtr->start();
		}

		if (configImpl.isStatisticsEnabled())
		{
			statisticsTask_SPtr = AbstractTask_SPtr(new StatisticsTask(*this));
			memTopoTaskSchedule_CSptr->scheduleDelay(
					statisticsTask_SPtr, TaskSchedule::ZERO_DELAY);
		}

		state = SpiderCast::Started;
	}

	Trace_Event(this,"start()", "SpiderCast started.");

	Trace_Exit(this,"start()");
}

/*
 * Called only by App thread (or delivery thread), under normal conditions.
 */
void SpiderCastImpl::close(bool soft) throw (SpiderCastRuntimeError)
{
	//TODO improve: threading, thread failure, repeated calls, state

	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "close()");
		buffer->addProperty<bool>("soft",soft);
		buffer->invoke();
	}

	bool do_close = false;

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state == SpiderCast::Started || state == SpiderCast::Init)
		{
			state = SpiderCast::Closed;
			do_close = true;
		}
	}

	if (do_close)
	{
		int time_out = 0;
		if (configImpl.isRetainAttributesOnSuspectNodesEnabled())
		{
			time_out = (soft ? (2*configImpl.getMembershipGossipIntervalMillis()) : 0);
		}

		internalClose(soft, false, false, time_out );
	}

	Trace_Exit(this,"close()");
}

/*
 * Called only by App thread (or delivery thread), under normal conditions.
 */
bool SpiderCastImpl::closeAndRemove(int timeout_millis) throw (SpiderCastRuntimeError)
{
	//TODO improve: threading, thread failure, repeated calls, state

	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "closeAndRemove()");
		buffer->invoke();
	}

	bool do_close = false;

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state == SpiderCast::Started || state == SpiderCast::Init)
		{
			state = SpiderCast::Closed;
			do_close = true;
		}
	}

	bool ack_rcv = false;

	if (do_close)
	{
		ack_rcv = internalClose(true, true, false, timeout_millis);
	}

	//TODO wait for result
	Trace_Exit<bool>(this,"closeAndRemove()", ack_rcv);
	return ack_rcv;
}

/*
 * Close the layers from top to bottom.
 * If onFailure, assume closing thread is one of SpiderCast threads
 */
bool SpiderCastImpl::internalClose(bool soft, bool removeRetained, bool onFailure, int timeout_millis)
{
	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "internalClose()");
		buffer->addProperty<bool>("soft",soft);
		buffer->addProperty<bool>("onFailure",onFailure);
		buffer->invoke();
	}

	if (ScTraceBuffer::isDebugEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "internalClose()","calling thread");
		buffer->addProperty<boost::thread::id>("id",boost::this_thread::get_id());
		buffer->invoke();
	}

	//Signaling terminate to all components
	bool ack_rcv = memMngr_SPtr->terminate(soft,removeRetained, timeout_millis);
	Trace_Event(this, "internalClose", "after membership termination","ack_rcv",ack_rcv);

	topoMngr_SPtr->terminate(soft);
	Trace_Event(this, "internalClose", "after topo termination");
	if (configImpl.isHierarchyEnabled())
	{
		Trace_Event(this, "internalClose", "Calling hier terminate");
		hierMngr_SPtr->terminate(soft);
	}

	if (configImpl.isRoutingEnabled())
	{
		Trace_Event(this, "internalClose", "Calling routing terminate");
		routingManagerImpl_SPtr->terminate(soft);
		messagingManagerImpl_SPtr->terminate(soft);
	}

	//Closing all threads

	if (memTopoThread_SPtr)
	{
		Trace_Event(this, "internalClose", "Before call to finish");
		memTopoThread_SPtr->finish();
		Trace_Event(this, "internalClose", "After call to finish");
		if (boost::this_thread::get_id() != memTopoThread_SPtr->getID())
		{
			Trace_Debug(this, "internalClose()", "joining MemTopoThread",
					"thread-id",
					ScTraceBuffer::stringValueOf<boost::thread::id>(memTopoThread_SPtr->getID()));
			memTopoThread_SPtr->join();
		}
		else
		{
			Trace_Event(this, "internalClose()", "closing thread is MemTopoThread");
		}
	}

	Trace_Event(this, "internalClose()", "After MemTopoThread join");

	if (routingThread_SPtr)
	{
		routingThread_SPtr->finish();
		routingManagerImpl_SPtr->wakeUp(1);

		if (boost::this_thread::get_id() != routingThread_SPtr->getID())
		{
			Trace_Debug(this, "internalClose()", "joining RoutingThread",
					"thread-id",
					ScTraceBuffer::stringValueOf<boost::thread::id>(routingThread_SPtr->getID()));
			routingThread_SPtr->join();
		}
		else
		{
			Trace_Event(this, "internalClose()", "closing thread is RoutingThread");
		}
	}

	Trace_Event(this, "internalClose()", "After RoutingThread join");

	//closing CommAdapter is the last thing we do, and the only one that might fail (do we really need this?).
	try
	{
		commAdapter_SPtr->terminate(soft);
	}
	catch (SpiderCastRuntimeError& e)
	{
		{
			boost::recursive_mutex::scoped_lock lock(state_mutex);
			state = SpiderCast::Error;
		}

		String desc("Failed to terminate CommAdapter; ");
		desc.append("SpiderCastRuntimeError ");
		desc.append(e.what());

		//TRACE error
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "internalClose()", desc);
		buffer->invoke();

		throw SpiderCastRuntimeError(desc);
	}

	Trace_Event(this, "internalClose()", "After CommAdapetr terminate");

	Trace_Exit(this,"internalClose()", ack_rcv);
	return ack_rcv;
}

SpiderCast::NodeState SpiderCastImpl::getNodeState() const
{
	boost::recursive_mutex::scoped_lock lock(state_mutex);

	return state;
}

String SpiderCastImpl::toString() const
{
	String s = "SpiderCast: " + instanceID + ": state=";

	{//synch block
		boost::recursive_mutex::scoped_lock lock(state_mutex);
		s.append(nodeStateName[state]);
	}

	s.append(", Config: \n").append(configImpl.toString());

	return s;
}

NodeID_SPtr SpiderCastImpl::getNodeID() const
{
	return boost::static_pointer_cast<NodeID>(configImpl.getMyNodeID());
}

String SpiderCastImpl::getBusName() const
{
	return configImpl.getBusName();
}

/*
 * @see spdr::SpiderCast
 */
MembershipService_SPtr SpiderCastImpl::createMembershipService(
			const PropertyMap& properties,
			MembershipListener& membershipListener)
			throw (SpiderCastRuntimeError, SpiderCastLogicError)
{
	Trace_Entry(this,"createMembershipService()", "prop", properties.toString());

	MembershipService_SPtr service;

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state == SpiderCast::Error || state == SpiderCast::Closed)
		{
			String what = "SpiderCast instance in state " + SpiderCast::nodeStateName[state];
			Trace_Exit(this, "createMembershipService()", "SpiderCastLogicError", what);
			throw SpiderCastLogicError(what);
		}

		if (memMngr_SPtr)
		{
			service = memMngr_SPtr->createMembershipService(properties, membershipListener);
		}
		else
		{
			String what = "MembershipManager is null";
			Trace_Exit(this, "createMembershipService()", "NullPointerException", what);
			throw NullPointerException(what);
		}

	}

	Trace_Exit(this, "createMembershipService()");
	return service;
}

messaging::Topic_SPtr SpiderCastImpl::createTopic(
		const String& name,
		const PropertyMap& properties)
	throw (SpiderCastRuntimeError,SpiderCastLogicError)
{
	using namespace messaging;

	BasicConfig topic_config(properties);
	bool global = topic_config.getOptionalBooleanProperty(
			config::TopicGlobalScope_Prop_Key,
			configImpl.isTopicGlobalScope());
	Topic_SPtr topic(new TopicImpl(name,global));
	return topic;
}

messaging::TopicPublisher_SPtr SpiderCastImpl::createTopicPublisher(
		messaging::Topic_SPtr topic,
		messaging::PubSubEventListener& eventListener,
		const PropertyMap& config)
throw (SpiderCastRuntimeError,SpiderCastLogicError)
{
	using namespace messaging;

	Trace_Entry(this,"createTopicPublisher()",
			"topic", spdr::toString<Topic>(topic),
			"config", config.toString());

	TopicPublisher_SPtr publisher;

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state == SpiderCast::Error || state == SpiderCast::Closed)
		{
			String what = "SpiderCast instance in state " + SpiderCast::nodeStateName[state];
			Trace_Exit(this, "createTopicPublisher()", "SpiderCastLogicError", what);
			throw SpiderCastLogicError(what);
		}

		if (!topic)
		{
			String what = "Topic can not be null";
			Trace_Exit(this, "createTopicPublisher()", "IllegalArgumentException", what);
			throw IllegalArgumentException(what);
		}

		if (!configImpl.isRoutingEnabled())
		{
			String what = "SpiderCast instance config has " + config::RoutingEnabled_PROP_KEY + "=false; Messaging services are unavailable.";
			Trace_Exit(this, "createTopicPublisher()", "SpiderCastLogicError", what);
			throw SpiderCastLogicError(what);
		}

		publisher = messagingManagerImpl_SPtr->createTopicPublisher(
				topic,eventListener,config);
	}

	Trace_Exit(this, "createTopicPublisher()",
			spdr::toString<TopicPublisher>(publisher));
	return publisher;
}

/*
 * @see spdr::SpiderCast
 */
messaging::TopicSubscriber_SPtr SpiderCastImpl::createTopicSubscriber(
		messaging::Topic_SPtr topic,
		messaging::MessageListener& messageListener,
		messaging::PubSubEventListener& eventListener,
		const PropertyMap& config)
	throw (SpiderCastRuntimeError,SpiderCastLogicError)
{

	using namespace messaging;

	Trace_Entry(this,"createTopicSubscriber()",
			"topic", spdr::toString<Topic>(topic),
			"config", config.toString());

	TopicSubscriber_SPtr subscriber;

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state == SpiderCast::Error || state == SpiderCast::Closed)
		{
			String what = "SpiderCast instance in state " + SpiderCast::nodeStateName[state];
			Trace_Exit(this, "createTopicPublisher()", "SpiderCastLogicError", what);
			throw SpiderCastLogicError(what);
		}

		if (!topic)
		{
			String what = "Topic can not be null";
			Trace_Exit(this, "createTopicSubscriber()", "IllegalArgumentException", what);
			throw IllegalArgumentException(what);
		}

		if (!configImpl.isRoutingEnabled())
		{
			String what = "SpiderCast instance config has " + config::RoutingEnabled_PROP_KEY + "=false; Messaging services are unavailable.";
			Trace_Exit(this, "createTopicSubscriber()", "SpiderCastLogicError", what);
			throw SpiderCastLogicError(what);
		}

		subscriber = messagingManagerImpl_SPtr->createTopicSubscriber(
				topic,messageListener,eventListener,config);
	}

	Trace_Exit(this, "createTopicSubscriber()",
			spdr::toString<TopicSubscriber>(subscriber));
	return subscriber;
}

leader_election::LeaderElectionService_SPtr SpiderCastImpl::createLeaderElectionService(
		leader_election::LeaderElectionListener& electionListener,
		bool candidate,
		const PropertyMap& properties)
	throw (SpiderCastRuntimeError,SpiderCastLogicError)
{
	Trace_Entry(this, "createLeaderElectionService()");

	using namespace leader_election;

	LeaderElectionService_SPtr les;

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state == SpiderCast::Error || state == SpiderCast::Closed)
		{
			String what = "SpiderCast instance in state " + SpiderCast::nodeStateName[state];
			Trace_Exit(this, "createLeaderElectionService()", "SpiderCastLogicError", what);
			throw SpiderCastLogicError(what);
		}

		if (!configImpl.isLeaderElectionEnabled())
		{
			String what = "SpiderCast instance config has " + config::LeaderElectionEnabled_PROP_KEY + "=false; Leader election service is unavailable.";
			Trace_Exit(this, "createLeaderElectionService()", "SpiderCastLogicError", what);
			throw SpiderCastLogicError(what);
		}

		les = memMngr_SPtr->createLeaderElectionService(electionListener,candidate,properties);
	}

	Trace_Exit(this, "createLeaderElectionService()");
	return les;
}


messaging::P2PStreamTx_SPtr SpiderCastImpl::createP2PStreamTx(
		NodeID_SPtr target,
		messaging::P2PStreamEventListener& p2PStreamTxEventListener,
		const PropertyMap& properties) throw (SpiderCastRuntimeError,
				SpiderCastLogicError)
{

	using namespace messaging;

	Trace_Entry(this, "createP2PStreamTx()");

	P2PStreamTx_SPtr tx;

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state == SpiderCast::Error || state == SpiderCast::Closed)
		{
			String what = "SpiderCast instance in state "
					+ SpiderCast::nodeStateName[state];
			Trace_Exit(this, "createP2PStreamTx()", "SpiderCastLogicError",
					what);
			throw SpiderCastLogicError(what);
		}

		tx = messagingManagerImpl_SPtr->createP2PStreamTx(target,
				p2PStreamTxEventListener, properties);
	}

	Trace_Exit(this, "createP2PStreamTx()", spdr::toString<P2PStreamTx>(
			tx));
	return tx;

				}

messaging::P2PStreamRcv_SPtr SpiderCastImpl::createP2PStreamRcv(
		messaging::MessageListener & p2PStreamMsgListener,
		messaging::P2PStreamEventListener& p2PStreamRcvEventListener,
		const PropertyMap& properties) throw (SpiderCastRuntimeError,
		SpiderCastLogicError)
{
	using namespace messaging;

	Trace_Entry(this, "createP2PStreamRcv()");

	P2PStreamRcv_SPtr receiver;

	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state == SpiderCast::Error || state == SpiderCast::Closed)
		{
			String what = "SpiderCast instance in state "
					+ SpiderCast::nodeStateName[state];
			Trace_Exit(this, "createP2PStreamRcv()", "SpiderCastLogicError",
					what);
			throw SpiderCastLogicError(what);
		}

		receiver = messagingManagerImpl_SPtr->createP2PStreamRcv(
				p2PStreamMsgListener, p2PStreamRcvEventListener, properties);
	}

	Trace_Exit(this, "createP2PStreamRcv()", spdr::toString<P2PStreamRcv>(
			receiver));
	return receiver;
}

MembershipManager_SPtr SpiderCastImpl::getMembershipManager()
{
	return boost::static_pointer_cast<MembershipManager>(memMngr_SPtr);
}

TopologyManager_SPtr SpiderCastImpl::getTopologyManager()
{
	return topoMngr_SPtr;
}

CommAdapter_SPtr SpiderCastImpl::getCommAdapter()
{
	return commAdapter_SPtr;
}

HierarchyManager_SPtr SpiderCastImpl::getHierarchyManager()
{
	return hierMngr_SPtr;
}

boost::shared_ptr<route::RoutingManager> SpiderCastImpl::getRoutingManager()
{
	return boost::static_pointer_cast<route::RoutingManager>(routingManagerImpl_SPtr);
}

boost::shared_ptr<messaging::MessagingManager> SpiderCastImpl::getMessagingManager()
{
	return boost::static_pointer_cast<messaging::MessagingManager>(
			messagingManagerImpl_SPtr);
}

TaskSchedule_SPtr SpiderCastImpl::getTopoMemTaskSchedule()
{
	return memTopoTaskSchedule_CSptr;
}

int64_t SpiderCastImpl::getIncarnationNumber() const
{
	return incarnationNumber;
}

/*
 * report a fatal error and close
 */
void SpiderCastImpl::threadFailure(const String& threadName, std::exception& ex)
{
	Trace_Entry(this, "threadFailure()");

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "threadFailure()", "Unexpected thread failure");
		buffer->addProperty<boost::thread::id>("thread-id",boost::this_thread::get_id());
		buffer->addProperty("thread-name", threadName);
		buffer->addProperty("what", String(ex.what()));
		buffer->invoke();
	}

	//failures may happen on a close sequence too, we do not want to call internalClose twice
	bool do_close = false;
	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state == SpiderCast::Started || state == SpiderCast::Init)
		{
			do_close = true;
		}
		state = SpiderCast::Error;
	}

	eventListener.onEvent( event::SpiderCastEvent_SPtr(
			new event::FatalErrorEvent(
					event::Thread_Exit_Abnormally,
					"Thread exit abnormally, shutting down",
					boost::shared_ptr<Exception>( new SpiderCastRuntimeError(ex.what())))) );

	if (do_close)
	{
		internalClose(false, false, true, 0);
	}

	Trace_Exit(this,"threadFailure");
}

/*
 * report a fatal error and close
 */
void SpiderCastImpl::componentFailure(const String& errMsg, event::ErrorCode errCode)
{
	Trace_Entry(this, "componentFailure()");

	if (ScTraceBuffer::isErrorEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::error(this, "componentFailure()", "Unexpected failure");
		buffer->addProperty<boost::thread::id>("thread-id",boost::this_thread::get_id());
		buffer->addProperty("errMsg", errMsg);
		buffer->addProperty("errCode", event::SpiderCastEvent::errorCodeName[static_cast<int>(errCode)]);
		buffer->invoke();
	}

	//failures may happen on a close sequence too, we do not want to call internalClose twice
	bool do_close = false;
	{
		boost::recursive_mutex::scoped_lock lock(state_mutex);

		if (state == SpiderCast::Started || state == SpiderCast::Init)
		{
			do_close = true;
		}
		state = SpiderCast::Error;
	}

	eventListener.onEvent( event::SpiderCastEvent_SPtr(
			new event::FatalErrorEvent(
					errCode,
					"Unexpected failure, shutting down: " + errMsg)));

	if (do_close)
	{
		internalClose(false, false, true, 0);
	}

	Trace_Exit(this,"componentFailure");
}

const String& SpiderCastImpl::getInstanceID() const
{
	return instanceID;
}

void SpiderCastImpl::submitExternalEvent(event::SpiderCastEvent_SPtr event)
{
	TRACE_ENTRY2(this, "submitExternalEvent", event->toString());

	eventListener.onEvent(event);

	Trace_Exit(this, "submitExternalEvent");
}

void SpiderCastImpl::reportStats(bool labels)
{

	boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
	commAdapter_SPtr->getIncomingMessageQ()->reportStats(time,labels);
	memTopoThread_SPtr->reportStats(time,labels);
	memMngr_SPtr->reportStats(time,labels);

	//TODO event stats - report-suspect, etc
	//TODO routing stats

	memTopoTaskSchedule_CSptr->scheduleDelay(
			statisticsTask_SPtr,
			boost::posix_time::milliseconds(configImpl.getStatisticsPeriodMillis()));
}

} //namespace spdr
