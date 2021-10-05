/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#include "Thread.h"

namespace mcp
{

spdr::ScTraceComponent* Thread::tc_ = spdr::ScTr::enroll(
        mcp::trace::Component_Name,
        mcp::trace::SubComponent_Core,
        spdr::trace::ScTrConstants::Layer_ID_Core, "Thread",
		spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

Thread::Thread(const std::string& instID) :
		ctx_(tc_, instID, ""), bThread(), mutex(), conditionVar(), finishFlag(
				false)
{
	Trace_Entry(&ctx_, "Thread()");
}

Thread::~Thread()
{
	Trace_Entry(&ctx_, "~Thread()");
}

void Thread::join()
{
	Trace_Entry(&ctx_, "join()");
	bThread.join();
	Trace_Exit(&ctx_, "join()");
}

void Thread::start()
{
	Trace_Entry(&ctx_, "start()");

	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		if (!bThread.joinable()) // don't start the same thread twice
		{
			boost::thread temp(boost::ref(*this)); //this calls operator()() and starts the thread
			bThread.swap(temp); //deposit the thread in the local field, temp gets Not-a-Thread and dies
		}
		else
		{
			throw MCPRuntimeError("Thread already started");
		}
	}

	Trace_Exit(&ctx_, "start()");
}

void Thread::finish()
{
	Trace_Entry(&ctx_, "finish()");

	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		finishFlag = true;
	}

	//bThread.interrupt();
	conditionVar.notify_all();
	Trace_Exit(&ctx_, "finish()");
}

bool Thread::isFinish()
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	return finishFlag;
}

void Thread::interrupt()
{
	bThread.interrupt();
}

boost::thread::id Thread::getThreadID()
{
	return bThread.get_id();
}

}
