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
 * RoutingThread.cpp
 *
 *  Created on: Feb 9, 2012
 */

#include <sys/prctl.h>
#include "RoutingThread.h"

namespace spdr
{

namespace route
{

ScTraceComponent* RoutingThread::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Route,
		trace::ScTrConstants::Layer_ID_Route, "RoutingThread",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

RoutingThread::RoutingThread(const String& instID,
		SpiderCastConfigImpl& config, CoreInterface& coreIfc) :
	Thread(instID),
	ScTraceContext(tc_, instID,	config.getMyNodeID()->getNodeName()),
	instID_(instID),
	configImpl_(config),
	coreInterface_(coreIfc),
	name_(instID + ".RoutingThread"),
	routingTask_()
{
	Trace_Entry(this, "RoutingThread()");
}

RoutingThread::~RoutingThread()
{
	Trace_Entry(this, "~RoutingThread()");
}

const String RoutingThread::getName() const
{
	return name_;
}

void RoutingThread::operator()()
{
    ism_common_makeTLS(512,NULL);
	Trace_Entry(this, "operator()()");

	string name16;
	if (name_.size()>15)
		name16 = name_.substr(0,15);
	else
		name16 = name_;

	int rc = prctl( PR_SET_NAME,(uintptr_t)name16.c_str());
	if (rc != 0)
	{
		int errsv = errno;
		Trace_Warning(this, "operator()", "prctl PR_SET_NAME failed",
				"errno", spdr::stringValueOf(errsv));
	}
	else
	{
		Trace_Debug(this, "operator()", "thread name set to", "name", name16);
	}



	routingTask_ = coreInterface_.getRoutingManager();

	try
	{
		if (ScTraceBuffer::isEventEnabled(tc_))
		{
			ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "operator()()",
					"Starting");
			tb->addProperty("name", name_);
			tb->addProperty<boost::thread::id> ("id",
					boost::this_thread::get_id());
			tb->invoke();
		}

		bool workPending = true;
		while (!isFinish())
		{
			workPending = routingTask_->runRoutingTask(workPending);
		}

		if (ScTraceBuffer::isEventEnabled(tc_))
		{
			ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "operator()()",
					"Ending");
			tb->addProperty("name", name_);
			tb->addProperty<boost::thread::id> ("id",
					boost::this_thread::get_id());
			tb->invoke();
		}

	} catch (SpiderCastRuntimeError& re)
	{
		//Unexpected exception, call thread failure and shut down
		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "operator()()",
				"Error: unexpected SpiderCastRuntimeError");
		tb->addProperty(re);
		tb->addProperty(re.getStackTrace());
		tb->invoke();

		//Report fatal-error(thread failure) and orderly shutdown (internal close)
		coreInterface_.threadFailure(name_, re);

	} catch (SpiderCastLogicError& le)
	{
		//Unexpected exception, call thread failure and shut down
		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "operator()()",
				"Error: unexpected SpiderCastLogicError");
		tb->addProperty(le);
		tb->addProperty(le.getStackTrace());
		tb->invoke();

		//Report fatal-error(thread failure) and orderly shutdown (internal close)
		coreInterface_.threadFailure(name_, le);
	} catch (std::exception& e)
	{
		//Unexpected exception, call thread failure and shut down
		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "operator()()",
				"Error: unexpected exception");
		tb->addProperty("what",e.what());
		tb->addProperty("typeid", typeid(e).name());
		tb->invoke();

		//Report fatal-error(thread failure) and orderly shutdown (internal close)
		coreInterface_.threadFailure(name_, e);
	} catch (...)
	{
		//Unexpected exception, call thread failure and shut down
		std::ostringstream oss;
		oss << name_ << " unexpected exception: id="
				<< boost::this_thread::get_id()
				<< "; Not a descendant of std::exception";

		//Report fatal-error(thread failure) and orderly shutdown (internal close)
		boost::shared_ptr<SpiderCastRuntimeError> e_sptr = boost::shared_ptr<
				SpiderCastRuntimeError>(new SpiderCastRuntimeError(oss.str()));

		ScTraceBufferAPtr tb = ScTraceBuffer::event(this, "operator()()",
				"Error: unexpected unknown exception");
		tb->addProperty(*e_sptr);
		tb->invoke();

		coreInterface_.threadFailure(name_, *e_sptr);
	}

	routingTask_.reset();

	Trace_Exit(this, "operator()()");
	ism_common_freeTLS();
}

}

}
