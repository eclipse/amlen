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
 * CommRumTxMgr.cpp
 *
 *  Created on: Feb 17, 2010
 *      Author:
 *
 * Version     : $Revision: 1.7 $
 *-----------------------------------------------------------------
 * $Log: CommRumTxMgr.cpp,v $
 * Revision 1.7  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.6  2015/11/12 10:28:16 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.5  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.4  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.3  2015/07/02 10:29:12 
 * clean trace, byte-buffer
 *
 * Revision 1.2  2015/06/11 12:16:33 
 * *** empty log message ***
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.22  2012/10/25 10:11:07 
 * Added copyright and proprietary notice
 *
 * Revision 1.21  2011/05/30 17:44:41 
 * Streamline mutex locking
 *
 * Revision 1.20  2011/02/27 12:20:20 
 * Refactoring, header file organization
 *
 * Revision 1.19  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.17.2.2  2010/11/23 14:12:05 
 * merged with HEAD branch
 *
 * Revision 1.17.2.1  2010/10/21 16:10:42 
 * Massive comm changes
 *
 * Revision 1.17  2010/09/18 17:04:55 
 * Hierarhical bus name support
 *
 * Revision 1.16  2010/08/23 15:25:06 
 * Remove commented code. No functional change
 *
 * Revision 1.15  2010/08/17 13:29:14 
 * Remove the RUM config param, as it should be done seperately for each call; enhanced logging
 *
 * Revision 1.14  2010/08/15 11:48:45 
 * Eliminate compilation warnings, no functional change
 *
 * Revision 1.13  2010/08/05 09:25:23 
 * Init the RUM TX config structure only once
 *
 * Revision 1.12  2010/08/03 13:57:43 
 * Add RUM time tracing
 *
 * Revision 1.11  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.10  2010/06/30 09:13:40 
 * Handle termination better
 *
 * Revision 1.9  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.8  2010/06/27 12:02:52 
 * Move all implementations from header files
 *
 * Revision 1.7  2010/06/27 08:23:32 
 * Pass RUM error code to ConnectionsAsyncCompletionLIstener::onFailure()
 *
 * Revision 1.6  2010/06/23 14:18:38 
 * No functional chnage
 *
 * Revision 1.5  2010/05/27 09:31:16 
 * Enahnced logging
 *
 * Revision 1.4  2010/04/14 09:16:21 
 * Add some print statemnets - no funcional changes
 *
 * Revision 1.3  2010/04/12 09:21:53 
 * Add a mutex for roper synchronization + misc. non-funcational changes
 *
 * Revision 1.2  2010/04/07 13:54:35 
 * Add CVS log and make it compile on Linux
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 */
#ifdef RUM_UNIX
#include <string.h>
#endif

#include <iostream>
#include <string.h>

#include <boost/lexical_cast.hpp>

#include "CommRumTxMgr.h"

using namespace std;

namespace spdr
{
extern "C"
{
static void onEvent(const rumEvent *ev, void *user)
{
	CommRumTxMgr::on_event(ev, user);
}
}

using namespace trace;

ScTraceComponent* CommRumTxMgr::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Comm,
		trace::ScTrConstants::Layer_ID_CommAdapter, "CommRumTxMgr",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

ScTraceContextImpl CommRumTxMgr::_tcntx(_tc,"static","static");

const String CommRumTxMgr::queueNamePrefix = "SPDR";
const String CommRumTxMgr::queueNameSeparator = ",";

void CommRumTxMgr::terminate()
{
	boost::recursive_mutex::scoped_lock lock(_mutex);
	Trace_Entry(this, "terminate()");
	_closed = true;
}

CommRumTxMgr::CommRumTxMgr(BusName_SPtr thisBusName, const char* thisMemberName,
		rumInstance& rum, const String& instID, CommRumTxMgrListener& connMgr) :
	ScTraceContext(_tc, instID, thisMemberName),
	_rum(rum),
	_closed(false),
	_instID(instID),
	_connMgr(connMgr),
	_thisMemberName(thisMemberName),
	_thisBusName(thisBusName)
{
	Trace_Entry(this, "CommRumTxMgr()");
	_queueName = queueNamePrefix;
	_queueName.append(queueNameSeparator);
	_queueName.append(_thisBusName->toOrgString());
	_queueName.append(queueNameSeparator);
	_queueName.append(_thisMemberName);
}

void CommRumTxMgr::start()
{
}

CommRumTxMgr::~CommRumTxMgr()
{
	Trace_Entry(this, "~CommRumTxMgr");
}

bool CommRumTxMgr::createTx(
		const String& target,
		const rumConnection& connection,
		rumQueueT* pQueueT,
		rumStreamID_t* pSID,
		int *pRC)
{
	if (_closed)
	{
		Trace_Event(this, "createTx()", "returning immediately. closed.");
		return false;
	}

	Trace_Entry(this, "createTx()",
			"target", target, "connection", spdr::toHexString(connection.connection_id));

	/*-------------------------------------------------------------------------
	 Create a QueueT handle, and return the result of its opening attempt.
	 -------------------------------------------------------------------------*/
	rumTxQueueParameters config;
	int res = rumInitStructureParameters(RUM_SID_TxQUEUEPARAMETERS, &config,
			RUMCAPI_VERSION, pRC);
	if (res != RUM_SUCCESS)
	{
		char errorDesc[1024];
		rumGetErrorDescription(*pRC, errorDesc, 1024);
		ostringstream oss2;
		oss2 << "Failed to init rumQueue config error code: " << *pRC
				<< ", description: " << errorDesc << "target: " << target
				<< "connection: " << std::hex << connection.connection_id;
		Trace_Event(this, "createTx()", oss2.str());
		Trace_Exit(this, "createTx", false);
		return false;
	}

	config.reliability = RUM_RELIABLE;
	config.on_event = onEvent;
	config.event_user = this;
	strcpy(config.queue_name, _queueName.c_str());
	config.stream_id = pSID;
	config.rum_connection = connection;

	Trace_Dump(this, "createTx()", "before rumTCreateQueue");
	res = rumTCreateQueue(&_rum, &config, pQueueT, pRC);
	if (res == RUM_SUCCESS)
	{
		Trace_Event(this, "createTx()", "succeeded",
				"sid", spdr::toHexString(*pSID), "cid", spdr::toHexString(connection.connection_id));
		Trace_Exit(this, "createTx", true);
		return true;
	}
	else
	{
		char errorDesc[1024];
		rumGetErrorDescription(*pRC, errorDesc, 1024);
		ostringstream oss5;
		oss5 << "Failed to create QueueT error code " << *pRC
				<< ", description: " << errorDesc << "target: " << target
				<< "connection: " << std::hex << connection.connection_id;
		Trace_Event(this, "createTx()", oss5.str());
		Trace_Exit(this, "createTx", false);
		return false;
	}
}

void CommRumTxMgr::onStreamNotPresent(rumStreamID_t sid)
{
	Trace_Entry(this, "onStreamNotPresent()", spdr::toHexString(sid));
	_connMgr.onStreamNotPresent(sid);
	Trace_Exit(this, "onStreamNotPresent()");
}

void CommRumTxMgr::on_event(const rumEvent *event, void *event_user)
{
	CommRumTxMgr* txMgr = (CommRumTxMgr*) event_user;
	const char* eventDescription;
	if ((event->nparams > 0) && (event->event_params[0] != NULL))
	{
		eventDescription = (const char*) event->event_params[0];
	}
	else
	{
		eventDescription = "NULL";
	}

	Trace_Debug(&_tcntx, "on_event()", eventDescription);

	int type = event->type;
	switch (type)
	{

	case RUM_STREAM_NOT_PRESENT:
	{
		txMgr->onStreamNotPresent(event->stream_id);
		break;
	}
	}

}

}

