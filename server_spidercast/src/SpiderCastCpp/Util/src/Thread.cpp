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
 * Thread.cpp
 *
 *  Created on: 01/03/2010
 */

//#include <iostream>

#include "Thread.h"

namespace spdr
{

ScTraceComponent* Thread::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Core,
		trace::ScTrConstants::Layer_ID_Core,
		"Thread",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

Thread::Thread(const String& instID) :
		ctx_(tc_,instID,""),
		bThread(),
		rMutex(),
		conditionVar(),
		finishFlag(false)
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
		boost::recursive_mutex::scoped_lock lock(rMutex);
		if (!bThread.joinable()) // don't start the same thread twice
		{
			boost::thread temp(boost::ref(*this)); //this calls operator()() and starts the thread
			bThread.swap(temp); //deposit the thread in the local field, temp gets Not-a-Thread and dies
		}
		else
		{
			throw SpiderCastRuntimeError("Thread already started");
		}
	}

	Trace_Exit(&ctx_, "start()");
}

void Thread::finish()
{
	Trace_Entry(&ctx_, "finish()");

	{
		boost::recursive_mutex::scoped_lock lock(rMutex);
		finishFlag = true;
	}

	//bThread.interrupt();
	Trace_Event(&ctx_, "finish()", "Before notify all" );
	conditionVar.notify_all();
	Trace_Event(&ctx_, "finish()", "After notify all" );
	Trace_Exit(&ctx_, "finish()");
}

bool Thread::isFinish()
{
	bool rv = false;

	{
		boost::recursive_mutex::scoped_lock lock(rMutex);
		rv = finishFlag;
	}

	return rv;
}

void Thread::interrupt()
{
	bThread.interrupt();
}

boost::thread::id Thread::getID()
{
	return bThread.get_id();
}

}
