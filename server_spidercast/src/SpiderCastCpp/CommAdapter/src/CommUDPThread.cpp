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
 * CommUDPThread.cpp
 *
 *  Created on: Jun 19, 2011
 */

#include <sys/prctl.h>
#include "CommUDPThread.h"

namespace spdr
{

ScTraceComponent* CommUDPThread::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Comm,
		trace::ScTrConstants::Layer_ID_CommAdapter,
		"CommUDPThread",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

CommUDPThread::CommUDPThread(const String& instID, const String name, boost::asio::io_service& io_service) :
	Thread(instID),
	ScTraceContext(tc_, instID, ""),
	name_(instID +"." + name),
	io_service_(io_service)
{
	Trace_Entry(this,"CommUDPThread()");

}

CommUDPThread::~CommUDPThread()
{
	Trace_Entry(this,"~CommUDPThread()");
}

const String CommUDPThread::getName() const
{
	return name_;
}

void CommUDPThread::operator()()
{
    ism_common_makeTLS(512,NULL);
	Trace_Entry(this,"operator()", "CommUDPThread Starting");

	std::string name16;
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


	Trace_Event(this,"operator()", "CommUDPThread Starting");
	try
	{
		while (!isFinish())
		{
			io_service_.run();
		}
	}
	catch (boost::system::system_error& se)
	{
		Trace_Error(this, "operator()", "Error: UDP thread (Unicast or Multicast) encountered a system_error and shut-down.",
				"name", name_, "id", spdr::stringValueOf(boost::this_thread::get_id()),
				"what", se.what(), "code", spdr::stringValueOf(se.code()));
		//throw;
		//TODO orderly shut down
	}
	catch (std::exception& e)
	{
		Trace_Error(this, "operator()", "Error: UDP thread (Unicast & Multicast) encountered an exception and shut-down.",
				"name", name_, "id", spdr::stringValueOf(boost::this_thread::get_id()),
				"what", e.what(), "typeid", typeid(e).name());
		//throw;
		//TODO orderly shut down
	}

	Trace_Exit(this,"operator()");
	ism_common_freeTLS();
}

}
