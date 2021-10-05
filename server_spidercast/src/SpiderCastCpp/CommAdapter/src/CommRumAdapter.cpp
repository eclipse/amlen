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
 * CommRumAdapter.cpp
 *
 *  Created on: Feb 9, 2010
 *      Author:
 *
 * Version     : $Revision: 1.16 $
 *-----------------------------------------------------------------
 * $Log: CommRumAdapter.cpp,v $
 * Revision 1.16  2016/01/07 14:41:18 
 * bug fix: closeStream with virgin RumNeighbor
 *
 * Revision 1.15  2016/01/06 10:24:59 
 * With new property UseSSL that sets RUM protocol to RUM_SSL
 *
 * Revision 1.14  2015/12/24 10:22:49 
 * make RUM trace no higher than CommRumAdapter
 *
 * Revision 1.13  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.12  2015/11/11 15:20:50 
 * *** empty log message ***
 *
 * Revision 1.11  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.10  2015/08/27 10:53:53 
 * change RUM log level dynamically
 *
 * Revision 1.9  2015/08/09 09:47:40 
 * *** empty log message ***
 *
 * Revision 1.8  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.7  2015/07/29 09:26:11 
 * split brain
 *
 * Revision 1.6  2015/07/26 09:15:54 
 * change debug-fail-fast to default false
 *
 * Revision 1.5  2015/05/20 11:35:12 
 * comments, no functional change
 *
 * Revision 1.4  2015/05/20 11:11:01 
 * support for IPv6 in multicast discovery
 *
 * Revision 1.3  2015/05/07 09:51:28 
 * Support for internal/external endpoints
 *
 * Revision 1.2  2015/05/05 12:48:26 
 * Safe connect
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.49  2015/01/26 12:38:14 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.48  2012/10/25 10:11:07 
 * Added copyright and proprietary notice
 *
 * Revision 1.47  2012/06/27 12:38:57 
 * Seperate generic COMM from RUM
 *
 * Revision 1.46  2011/07/10 09:23:52 
 * Create an RUM Tx on a seperate thread (not thr RUM thread)
 *
 * Revision 1.45  2011/06/20 11:38:54 
 * Add commUdp component
 *
 * Revision 1.44  2011/05/30 17:43:59 
 * Streamline mutex locking
 *
 * Revision 1.43  2011/05/22 12:52:35 
 * Introduce a local neighbor
 *
 * Revision 1.42  2011/03/31 13:57:43 
 * Convert a couple of pointers to shared pointers
 *
 * Revision 1.41  2011/03/24 08:45:29 
 * added config to constructor in support of CRC
 *
 * Revision 1.40  2011/03/13 09:23:18 
 * Set RUM trace level in config only
 *
 * Revision 1.39  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.38  2011/01/10 12:26:15 
 * Remove context from connectOnExisting() method
 *
 * Revision 1.37  2011/01/09 09:59:00 
 * Align RUm logging
 *
 * Revision 1.36  2011/01/02 13:26:48 
 * No functional change
 *
 * Revision 1.35  2010/12/29 09:49:20 
 * Instructu RUM to use IPV4 rathar than "any" (per Nir's recommendation)
 *
 * Revision 1.34  2010/12/26 07:45:59 
 * Fix spelling getreceiverId()
 *
 * Revision 1.33  2010/12/22 13:34:32 
 * Align RUM log level to ours (more or less)
 *
 * Revision 1.32  2010/12/14 10:47:40 
 * Comm changes - receiver stream id in each message and neighbor, disconnect reject stream, if known and counts number tx and rcv before closing connection.
 *
 * Revision 1.31  2010/11/25 16:15:05 
 * After merge of comm changes back to head
 *
 * Revision 1.29.2.3  2010/11/23 14:12:05 
 * merged with HEAD branch
 *
 * Revision 1.29.2.2  2010/11/15 12:46:32 
 * Hopefully,last bug fixed
 *
 * Revision 1.29.2.1  2010/10/21 16:10:42 
 * Massive comm changes
 *
 * Revision 1.29  2010/09/18 17:04:55 
 * Hierarhical bus name support
 *
 * Revision 1.28  2010/08/24 10:45:04 
 * Clsoe s neighbor while holding the mutex, when disconnecting a neighbor
 *
 * Revision 1.27  2010/08/23 13:33:05 
 * Embed RUM log within ours
 *
 * Revision 1.26  2010/08/23 09:21:13 
 * Initialize some RUM properties from the config object
 *
 * Revision 1.25  2010/08/09 12:35:48 
 * resrict RUM memory to 20MB
 *
 * Revision 1.24  2010/08/08 12:54:04 
 * Change code such that no Comm locks are held while calling RUM
 *
 * Revision 1.23  2010/08/03 13:57:18 
 * Add RUM time tracing
 *
 * Revision 1.22  2010/08/02 17:12:39 
 * Pass in myNodeId to RumCOnnectionsMgr in order to sekect an appropriate arget IP address
 *
 * Revision 1.21  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.20  2010/07/01 11:55:07 
 * Remove setRumConfig as a sepeate method
 *
 * Revision 1.19  2010/06/30 09:12:51 
 * Bypass RUM creation failure issue due  to instanceName
 *
 * Revision 1.18  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.17  2010/06/27 12:38:48 
 * *** empty log message ***
 *
 * Revision 1.16  2010/06/23 12:35:59 
 * Stop RUM earlier during the close process
 *
 * Revision 1.15  2010/06/22 12:14:11 
 * Add mutex protection
 *
 * Revision 1.14  2010/06/20 14:41:49 
 * Handle termination better. Add grace parameter. Check for terminated in (dis)connect
 *
 * Revision 1.13  2010/06/14 15:51:31 
 * Remove unused print() method + some comments clean-up
 *
 * Revision 1.12  2010/06/08 15:46:25 
 * changed instanceID to String&
 *
 * Revision 1.11  2010/06/03 14:43:21 
 * Arrange print statmenets
 *
 * Revision 1.10  2010/06/01 13:50:40 
 * incomingMsgQ was created twice. Create only in parent class
 *
 * Revision 1.9  2010/05/27 09:29:38 
 * Add disconnect() method + enhanced logging
 *
 * Revision 1.8  2010/05/20 09:15:52 
 * Add nodeIdCache, incomingMsgQ, and the use of nodeId on the connect rather than a string
 *
 * Revision 1.7  2010/05/10 11:31:30 
 * Handle termination better
 *
 * Revision 1.6  2010/05/10 08:30:40 
 * Use a new interface: ConnectionsAsyncCompletionListener rather than an asyncCompletionListener template; remove the connect() method without a completion listener; handle termination better; handle initialization failues better
 *
 * Revision 1.5  2010/04/15 10:08:51 
 * *** empty log message ***
 *
 * Revision 1.4  2010/04/14 09:16:21 
 * Add some print statemnets - no funcional changes
 *
 * Revision 1.3  2010/04/12 09:21:06 
 * Implement getInstance with proper config object + use mutex for synchronization
 *
 * Revision 1.2  2010/04/07 13:54:12 
 * Add CVS log and a print() method and make it compile on Linux
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 */

#include "CommRumAdapter.h"
#include <sstream>
#ifdef RUM_UNIX
#include <string.h>
#include <stdlib.h>
#endif // RUM_UNIX
#include <cstdio>

using namespace std;

extern "C"
{

static void onRumLogEvent(const llmLogEvent_t *log_event, void *user)
{
	spdr::CommRumAdapter::on_rum_log_event(log_event, user);
}

}


namespace spdr
{
using namespace trace;

ScTraceComponent* CommRumAdapter::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Comm,
		trace::ScTrConstants::Layer_ID_CommAdapter, "CommRumAdapter",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

CommRumAdapter::CommRumAdapter(const String& instID,
		const SpiderCastConfigImpl& config, NodeIDCache& cache, int64_t incarnationNumber) :
	CommAdapter(instID, config, cache, incarnationNumber),
	ScTraceContext(_tc, instID,	config.getNodeName())
{
	Trace_Entry(this, "CommRumAdapter()");
	_instID = instID;

	_thisBusName = BusName_SPtr(new BusName(config.getBusName().c_str()));

	_thisMemberName = new char[config.getNodeName().length() + 1];
	strcpy(_thisMemberName, config.getNodeName().c_str());

	if (ScTraceBuffer::isEventEnabled(_tc))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "CommRumAdapter()", "init");
		ostringstream oss2;
		oss2 << "BusName=" << _thisBusName->toOrgString()
			 << "; MemberName=" << _thisMemberName
			 << "; MemberBindPort=" << (int) config.getBindTcpRceiverPort()
			 << "; MemberBindInterface=" << config.getBindNetworkAddress()
			 << "; BindAll=" << config.getBindAllInterfaces()
			 << "; My NodeID=" << config.getMyNodeID()->toString() << ";";

		buffer->addProperty("config", oss2.str());
		buffer->invoke();
	}

	//_myNodeId = config.getMyNodeID();

	//-------------------------------------------------------------------------
	// Set RUM config
	//-------------------------------------------------------------------------
	rumConfig rumconfig;
	int rum_error_code;
	Trace_Event(this, "CommRumAdapter()", "before rumInitStructureParameters");
	int res = rumInitStructureParameters(RUM_SID_CONFIG, &rumconfig,
			RUMCAPI_VERSION, &rum_error_code);
	Trace_Event(this, "CommRumAdapter()", "after rumInitStructureParameters");
	if (res != RUM_SUCCESS)
	{
		char errorDesc[1024];
		rumGetErrorDescription(rum_error_code, errorDesc, 1024);
		String error(_thisMemberName);
		error += " Failed to init RUM config params:";
		error += errorDesc;
		Trace_Event(this, "CommRumAdapter()", error);
		throw SpiderCastRuntimeError(error);
	}

	rumconfig.IPVersion = (RUM_IPVERSION_t) RUM_IPver_ANY; //RUM_IPver_IPv4;//RUM_IPver_BOTH;
	rumconfig.TransportDirection = RUM_Tx_Rx;
	rumconfig.LogLevel = (RUM_LOGLEV_t) config.getRumLogLevel(); //default is RUM_LOGLEV_GLOBAL => INFO

	if (config.getBindAllInterfaces())
	{
		strcpy(rumconfig.RxNetworkInterface, "ALL"); //MUST be uppercase
	}
	else
	{
		strcpy(rumconfig.RxNetworkInterface, config.getBindNetworkAddress().c_str());
	}
	strcpy(rumconfig.TxNetworkInterface, rumconfig.RxNetworkInterface);
	rumconfig.ServerSocketPort = config.getBindTcpRceiverPort();

	ostringstream oss;
	oss << "SC.RUM." << _thisBusName->toShortString().c_str() << "." << _thisMemberName << "."
			<< _instID;
	String rumInstanceName = oss.str();
	rumconfig.instanceName = rumInstanceName.c_str();

	String logFileParam("LogFile=");
	logFileParam.append(rumInstanceName);

	rumconfig.Nparams = 1;
	char* ancillary_params[rumconfig.Nparams + 1];
	rumconfig.AncillaryParams = reinterpret_cast<char**>(&ancillary_params);
	rumconfig.AncillaryParams[0] = const_cast<char*>(logFileParam.c_str());
	rumconfig.AncillaryParams[1] = NULL;
	//config.LimitTransRate = (RUM_RATE_LIMIT_t)RUM_DYNAMIC_RATE_LIMIT;
	//config.TransRateLimitKbps = 1024;
	rumconfig.MaxMemoryAllowedMBytes = config.getMaxMemoryAllowedMBytes(); //20;

	if (config.isUseSSL())
	{
		rumconfig.Protocol = RUM_SSL;
	}
	else
	{
		rumconfig.Protocol = RUM_TCP;
	}

	// TODO check if necessary and config
	//config.PacketSizeBytes = ;
	//config.SocketReceiveBufferSizeKBytes ;
	//config.SocketSendBufferSizeKBytes
	//strcpy(config.AdvanceConfigFile, ...);
	//config.on_event = NULL;
	//config.event_user = this;
	//config.free_callback = NULL;

	//-------------------------------------------------------------------------
	// Create RUM instance
	//-------------------------------------------------------------------------
	Trace_Event(this, "CommRumAdapter()", "before rumInit");
	int rc = rumInit(&_rum, &rumconfig, onRumLogEvent, //
			reinterpret_cast<void*>(const_cast<char*>(_instID.c_str())), //TODO use this in on_log_event()
			&rum_error_code);
	Trace_Event(this, "CommRumAdapter()", "after rumInit");

	if (rc != RUM_SUCCESS)
	{
		char errorDesc[1024];
		rumGetErrorDescription(rum_error_code, errorDesc, 1024);
		std::ostringstream what;
		what << _thisMemberName << ": Failed to init RUM instance:" << errorDesc;
		Trace_Event(this, "CommRumAdapter()", what.str());

		if (rum_error_code == RUM_L_E_PORT_BUSY)
		{
			what << "; endpoint: [" << rumconfig.RxNetworkInterface << "]:" << rumconfig.ServerSocketPort;
			throw BindException(what.str(), event::Bind_Error_Port_Busy,
					std::string(rumconfig.RxNetworkInterface), rumconfig.ServerSocketPort);
		}
		else if (rum_error_code == RUM_L_E_LOCAL_INTERFACE)
		{
			what << "; endpoint: [" << rumconfig.RxNetworkInterface << "]:" <<  rumconfig.ServerSocketPort;
			throw BindException(what.str(), event::Bind_Error_Cannot_Assign_Local_Interface,
					std::string(rumconfig.RxNetworkInterface), rumconfig.ServerSocketPort);
		}
		else if (rum_error_code == RUM_L_E_THREAD_ERROR)
		{
			what << "; Failed to bind to listening port; endpoint: [" << rumconfig.RxNetworkInterface << "]:" << rumconfig.ServerSocketPort;
			throw BindException(what.str(), event::Bind_Error_Port_Busy,
					std::string(rumconfig.RxNetworkInterface), rumconfig.ServerSocketPort);
		}
		else if (rum_error_code == RUM_L_E_CONFIG_ENTRY) //May happen when SSL is requested but is not supported by RUM build
		{
			throw IllegalConfigException(what.str());
		}
		else
		{
			throw SpiderCastRuntimeError(what.str());
		}
	}

	_connMgr = boost::shared_ptr<RumConnectionsMgr>(new RumConnectionsMgr(_thisBusName, _thisMemberName, _rum,
			_nodeIdCache, _incomingMsgQueue, _instID, _myNodeId, config, incarnationNumber));
	_rumReceiver = boost::shared_ptr<CommRumReceiver>(new CommRumReceiver(_thisBusName, _thisMemberName, _rum,
			_connMgr, _nodeIdCache, _incomingMsgQueue, _instID, config));

	Trace_Exit(this, "CommRumAdapter()");
}

IncomingMsgQ_SPtr CommRumAdapter::getIncomingMessageQ()
{
	return _incomingMsgQueue;
}

void CommRumAdapter::terminate(bool grace)
{
	Trace_Entry(this, "terminate()");
	{
		boost::recursive_mutex::scoped_lock lock(_commMutex);

		if (_terminated)
			return;

		_terminated = true;
	}
	//#ifdef RUM_TERMINATION
	if (_started)
	{
		_connMgr->terminate(grace);
		_rumReceiver->terminate();
	}
	//#endif // #ifdef RUM_TERMINATION

	//-------------------------------------------------------------------------
	// Rum is killed after all listeners, queues and connection have been closed
	//-------------------------------------------------------------------------
	killRum(grace);

	CommAdapter::terminate(grace);

	Trace_Event(this, "terminate()", "Done");

	Trace_Exit(this, "terminate()");
}

void CommRumAdapter::killRum(bool grace)
{
	ostringstream oss;
	oss << grace;
	Trace_Entry(this, "killRum", "grace", oss.str());

	int error_code = 0;

	//#ifdef RUM_TERMINATION
	int gracePeriod = grace ? 1000 : 0;
	/*
	 #else
	 int gracePeriod = 0;
	 #endif
	 */

	int res = rumRemoveConnectionListener(&_rum, _connMgr->getRumContext(), &error_code);
	if (res == RUM_SUCCESS)
	{
		Trace_Event(this, "killRum()", "rumRemoveConnectionListener succeeded");
	}
	else
	{
		char errorDesc[1024];
		rumGetErrorDescription(error_code, errorDesc, 1024);
		Trace_Event(this, "killRum()", "rumRemoveConnectionListener failed", "error", errorDesc);
	}

	res = rumStop(&_rum, gracePeriod, &error_code);
	if (res == RUM_SUCCESS)
	{
		Trace_Event(this, "killRum()", "rumStop succeeded");
	}
	else
	{
		char errorDesc[1024];
		rumGetErrorDescription(error_code, errorDesc, 1024);
		Trace_Event(this, "killRum()", "rumStop failed", "error", errorDesc);
	}
	Trace_Exit(this, "killRum()");
}

void CommRumAdapter::start()
{
	Trace_Entry(this, "start()");
	{
		boost::recursive_mutex::scoped_lock lock(_commMutex);

		_connMgr->start();
		_rumReceiver->start();

		CommAdapter::start();
	}
	Trace_Exit(this, "start()");
}

CommRumAdapter::~CommRumAdapter()
{
	Trace_Entry(this, "~CommRumAdapter()");

	if (_thisMemberName != NULL)
	{
		delete[] _thisMemberName;
	}

	terminate(true);

	Trace_Exit(this, "~CommRumAdapter()");
}

bool CommRumAdapter::connect(NodeIDImpl_SPtr target,
		ConnectionsAsyncCompletionListener* asyncCompletionListener, ConnectionContext ctx)
{
	Trace_Entry(this, "connect()");
	{
		boost::recursive_mutex::scoped_lock lock(_commMutex);
		Trace_Entry(this, "connect()", "target", target->getNodeName());
		if (_terminated)
		{
			Trace_Event(this, "connect()", "failed. terminated");
			return false;
		}
	}
	if (!target->getNodeName().compare(_thisMemberName))
	{
		Trace_Event(this, "connect()",
				"Creating connection to myself");
		//return false;
	}
	bool res = _connMgr->createConnection(target, asyncCompletionListener, ctx);
	Trace_Exit<bool>(this, "connect()", res);
	return res;
}

bool CommRumAdapter::connect(NodeIDImpl_SPtr target, ConnectionContext ctx)
{
	Trace_Entry(this, "connect()");
	{
		boost::recursive_mutex::scoped_lock lock(_commMutex);
		Trace_Entry(this, "connect()", "target", target->getNodeName());
		if (_terminated)
		{
			Trace_Event(this, "connect()", "failed. terminated");
			return false;
		}
	}
	if (!target->getNodeName().compare(_thisMemberName))
	{
		Trace_Event(this, "connect()",
				"Creating connection to myself");
		//return false;
	}
	bool res = _connMgr->createConnection(target, ctx);
	Trace_Exit(this, "connect()");
	return res;
}

Neighbor_SPtr CommRumAdapter::connectOnExisting(NodeIDImpl_SPtr target)
{
	Trace_Entry(this, "connectOnExisting()", "target", target->getNodeName());
	{
		boost::recursive_mutex::scoped_lock lock(_commMutex);
		if (_terminated)
		{
			Trace_Event(this, "connectOnExisting()", "failed. terminated");
			return Neighbor_SPtr();
		}
	}
	if (!target->getNodeName().compare(_thisMemberName))
	{
		Trace_Event(this, "connectOnExisting()",
				"failed. Not creating connection to myself");
		return Neighbor_SPtr();
	}
	Neighbor_SPtr res = _connMgr->connectOnExisting(target);
	Trace_Exit(this, "connectOnExisting()", res ? "Succeeded" : "Failed");
	return res;
}


bool CommRumAdapter::disconnect(Neighbor_SPtr target)
{
	Trace_Entry(this, "disconnect()", "from neighbor", spdr::toString(target));

	{
		boost::recursive_mutex::scoped_lock lock(_commMutex);

		if (_terminated)
		{
			Trace_Event(this, "disconnect()", "failed. terminated");
			return false;
		}
		target->close();
	}

	if (!target->getName().compare(_thisMemberName))
	{
		Trace_Event(this, "disconnect()", "Local neighbor; ignoring");
		return true;
	}

	int rc;
	if (target->getReceiverId() != 0)
	{
		Trace_Debug(this, "disconnect()", "before rejectStream");
		_rumReceiver->rejectStream(target->getReceiverId(),&rc,true);
		Trace_Debug(this, "disconnect()", "after rejectStream");
	}
	else
	{
		Trace_Event(this, "disconnect()", "no associated receiver");
	}

	bool res = _connMgr->closeStream(target);
	Trace_Exit(this, "disconnect()", res);
	return res;
}

void CommRumAdapter::on_rum_log_event(const llmLogEvent_t *log_event,
		void *user)
{
//	 /** \brief Log Levels     */
//	typedef enum LLM_LOGLEV {
//	    LLM_LOGLEV_NONE   = 0,     /**<  N = none            */
//	    LLM_LOGLEV_STATUS = 1,     /**< S = Status           */
//	    LLM_LOGLEV_FATAL  = 2,     /**< F = Fatal            */
//	    LLM_LOGLEV_ERROR  = 3,     /**< E = Error            */
//	    LLM_LOGLEV_WARN   = 4,     /**< W = Warning          */
//	    LLM_LOGLEV_INFO   = 5,     /**< I = Informational    */
//	    LLM_LOGLEV_EVENT  = 6,     /**< V = eVent            */
//	    LLM_LOGLEV_DEBUG  = 7,     /**< D = Debug            */
//	    LLM_LOGLEV_TRACE  = 8,     /**< T = Trace            */
//	    LLM_LOGLEV_XTRACE = 9      /**< X = eXtended trace   */
//	} LLM_LOGLEV_t;

//	if (!_tc->isEventEnabled() && log_event->log_level >= LLM_LOGLEV_EVENT)
//		return;
//	if (!_tc->isEntryEnabled() && log_event->log_level >= LLM_LOGLEV_WARN)
//		return;


	//RUM log level is never higher that CommRumAdapter
	//RUM log level is 0-9, SpiderCast 0-8
	if (_tc->isLevelEnabled( (log_event->log_level>8) ? 8 : log_event->log_level) )
	{
		char msgKey[32];
		//sprintf(msgKey, "FMDU%04d %s", log_event->msg_key, "");
		std::string inst_id = std::string(user ? (char *)user : "null") + ":";
		sprintf(msgKey, "FMDU%04d %s", log_event->msg_key, inst_id.c_str());

		const char* logMsg;
		if ((log_event->nparams > 0) && (log_event->event_params != NULL))
		{
			logMsg = (const char*) (log_event->event_params[0]);
		}
		else
		{
			logMsg = "Undefined log message";
		}

		ScTr::rumTrace(log_event->log_level, RUM_LOGGING_CATEGORY, msgKey, logMsg);
	}
}
}
