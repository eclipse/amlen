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
 * IncomingMsgQ.cpp
 *
 *  Created on: 04/05/2010
 *      Author:
 *
 * Version     : $Revision: 1.10 $
 *-----------------------------------------------------------------
 * $Log: IncomingMsgQ.cpp,v $
 * Revision 1.10  2016/12/05 14:27:50 
 * Fix mem leak in Comm; Improve trace for stats.
 *
 * Revision 1.9  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.8.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.8  2016/01/07 14:41:18 
 * bug fix: closeStream with virgin RumNeighbor
 *
 * Revision 1.7  2015/12/24 10:21:53 
 * reportStats at level Info
 *
 * Revision 1.6  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.5  2015/09/08 07:11:33 
 * adjust trace levels
 *
 * Revision 1.4  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.3  2015/07/13 08:26:21 
 * *** empty log message ***
 *
 * Revision 1.2  2015/06/30 13:26:45 
 * Trace 0-8 levels, trace levels by component
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.27  2014/11/24 15:37:33 
 * *** empty log message ***
 *
 * Revision 1.26  2012/10/25 10:11:07 
 * Added copyright and proprietary notice
 *
 * Revision 1.25  2012/06/27 12:38:57 
 * Seperate generic COMM from RUM
 *
 * Revision 1.24  2011/07/10 09:23:52 
 * Create an RUM Tx on a seperate thread (not thr RUM thread)
 *
 * Revision 1.23  2011/06/29 11:46:46 
 * *** empty log message ***
 *
 * Revision 1.22  2011/06/06 12:04:49 
 * Add some traces to attempt to find a deadlock
 *
 * Revision 1.21  2011/06/01 13:36:18 
 * Add traces to find a deadlock
 *
 * Revision 1.20  2011/05/30 17:44:58 
 * Streamline mutex locking
 *
 * Revision 1.19  2011/05/02 10:38:42 
 * Deal with general comm events; place them both in topo and dht queue
 *
 * Revision 1.18  2011/05/01 08:28:24 
 * Add a DHT queue
 *
 * Revision 1.17  2011/03/24 08:45:56 
 * *** empty log message ***
 *
 * Revision 1.16  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.15  2010/12/09 12:15:36 
 * SC_Stats improvements
 *
 * Revision 1.14  2010/12/08 10:48:03 
 * Added statistics support
 *
 * Revision 1.13  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.11.2.3  2010/11/23 14:12:05 
 * merged with HEAD branch
 *
 * Revision 1.11.2.2  2010/10/21 18:01:49 
 * adding context to On_Connection_Success event and fixing bug in connection manager
 *
 * Revision 1.11.2.1  2010/10/21 16:10:42 
 * Massive comm changes
 *
 * Revision 1.11  2010/10/12 18:13:49 
 * Support for hierarchy
 *
 * Revision 1.10  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.9  2010/06/23 14:18:51 
 * No functional chnage
 *
 * Revision 1.8  2010/06/14 15:51:57 
 * Add mutex lock on registerReaderThread
 *
 * Revision 1.7  2010/06/08 15:46:25 
 * changed instanceID to String&
 *
 * Revision 1.6  2010/06/03 14:43:22 
 * Arrange print statmenets
 *
 * Revision 1.5  2010/06/01 13:51:34 
 * Set all entries in readerThreads to NULL upon creation
 *
 * Revision 1.4  2010/05/30 15:29:06 
 * Add print statements in onMessage
 *
 * Revision 1.3  2010/05/27 09:31:40 
 * Enahnced logging
 *
 * Revision 1.2  2010/05/20 09:22:05 
 * Initial implemented version
 *
 */

#include "IncomingMsgQ.h"

using namespace std;

namespace spdr
{

using namespace trace;

ScTraceComponent* IncomingMsgQ::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Comm,
		trace::ScTrConstants::Layer_ID_CommAdapter, "IncomingMsgQ",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

const char* const IncomingMsgQ::qTypeNames[] =
{ "NullQ", "TopologyQ", "MembershipQ", "DHTQ", "DataQ", "LastPositionQ" };

IncomingMsgQ::IncomingMsgQ(const String& instanceID, NodeIDImpl_SPtr myNodeId,
		NodeIDCache& cache) :
	ScTraceContext(_tc, instanceID, myNodeId->getNodeName()),
	_instId(instanceID), _myNodeID(myNodeId), _nodeIdCache(cache),
	inMsgCount_(SCMessage::Type_Max, SCMessage::messageTypeName),
	inMsgBytes_(SCMessage::Type_Max, SCMessage::messageTypeName),
	inMsgGroup_(SCMessage::Group_Max, SCMessage::messageGroupName),
	inQueueSize_(IncomingMsgQ::LastPositionQ, IncomingMsgQ::qTypeNames)
{
	Trace_Entry(this, "IncomingMsgQ()");
	for (int counter = 0; counter < LastPositionQ; counter++)
		_readerThreads[counter] = NULL;
	Trace_Exit(this, "IncomingMsgQ()");
}

IncomingMsgQ::~IncomingMsgQ()
{
	Trace_Entry(this, "~IncomingMsgQ()");
}

void IncomingMsgQ::registerReaderThread(ThreadControl* threadControl,
		QType qtype)
{
	ostringstream oss;
	oss << "qtype=" << qtype;
	Trace_Entry(this, "registerReaderThread()", oss.str());

	//boost::recursive_mutex::scoped_lock lock(_mutex);

	if (_readerThreads[qtype] != NULL)
	{
		ostringstream oss2;
		oss2 << "was not NULL for qtype: " << qtype;
		Trace_Event(this, "registerReaderThread()", oss2.str());
	}
	_readerThreads[qtype] = threadControl;

	Trace_Exit(this, "registerReaderThread()");
}

void IncomingMsgQ::onMessage(SCMessage_SPtr msg)
{
	Trace_Debug(this, "onMessage()",  "Entry");

	SCMessage::H1Header h1 = (*msg).readH1Header();

	{
		boost::recursive_mutex::scoped_lock(_mutexStats);
		Trace_Debug(this, "onMessage()",  "With stats mutex");
		inMsgGroup_.increment(h1.get<0> ());
		inMsgCount_.increment(h1.get<1> ());
		inMsgBytes_.increment(h1.get<1> (), h1.get<2> ());
	}

	switch (h1.get<0> ())
	//message group
	{
	case SCMessage::Group_Membership:
	{
		{
			boost::recursive_mutex::scoped_lock lock(_mutexMem);
			Trace_Debug(this, "onMessage()",  "membership message");
			_messageQueues[MembershipQ].push_front(msg);
		}

		if (_readerThreads[MembershipQ] != NULL)
		{
			Trace_Debug(this, "onMessage()",  "Calling wakeup");
			_readerThreads[MembershipQ]->wakeUp();
		}
	}
		break;

	case SCMessage::Group_Topology:
	case SCMessage::Group_Hierarchy:
	{
		{
			boost::recursive_mutex::scoped_lock lock(_mutexTopo);
			Trace_Debug(this, "onMessage()",  "topo / hier message");
			_messageQueues[TopologyQ].push_front(msg);
		}

		if (_readerThreads[TopologyQ] != NULL)
		{
			Trace_Debug(this, "onMessage()",  "Calling wakeup");
			_readerThreads[TopologyQ]->wakeUp();
		}
	}
		break;

	case SCMessage::Group_DHT:
	{
//Disable DHT
//		{
//			boost::recursive_mutex::scoped_lock lock(_mutexDHT);
//			Trace_Debug(this, "onMessage()",  "DHT message");
//			_messageQueues[DHTQ].push_front(msg);
//		}
//
//		if (_readerThreads[DHTQ] != NULL)
//		{
//			Trace_Debug(this, "onMessage()",  "Calling wakeup");
//			_readerThreads[DHTQ]->wakeUp();
//		}
	}
		break;

	case SCMessage::Group_GeneralCommEvents:
	{
		Trace_Debug(this, "onMessage()",  "general message");

		{
			boost::recursive_mutex::scoped_lock lock(_mutexTopo);
			_messageQueues[TopologyQ].push_front(msg);
		}

		if (_readerThreads[TopologyQ] != NULL)
		{
			Trace_Debug(this, "onMessage()",  "Calling wakeup");
			_readerThreads[TopologyQ]->wakeUp();
		}

//Disable DHT
//		{
//			boost::recursive_mutex::scoped_lock lock(_mutexDHT);
//			_messageQueues[DHTQ].push_front(msg);
//		}
//
//		if (_readerThreads[DHTQ] != NULL)
//		{
//			Trace_Debug(this, "onMessage()",  "Calling wakeup");
//			_readerThreads[DHTQ]->wakeUp();
//		}
	}
		break;

	case SCMessage::Group_Data:
	{
		Trace_Debug(this, "onMessage()",  "data message");

		{
			boost::recursive_mutex::scoped_lock lock(_mutexData);
			_messageQueues[DataQ].push_front(msg);
		}

		if (_readerThreads[DataQ] != NULL)
		{
			Trace_Debug(this, "onMessage()",  "Calling wakeup");
			_readerThreads[DataQ]->wakeUp();
		}
	}
		break;

	default:
		ostringstream oss;
		oss << "Warning: received an unknown message group: " << h1.get<0> ();
		Trace_Warning(this, "onMessage()", oss.str());
		break;
	}

	Trace_Debug(this, "onMessage()",  "Exit");
}

SCMessage_SPtr IncomingMsgQ::pollQ(QType qtype)
{
	Trace_Entry<int>(this, "pollQ()", "type", qtype);

	SCMessage_SPtr msg;

	switch (qtype)
	{
	case MembershipQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexMem);
		 msg = _messageQueues[MembershipQ].back();
		_messageQueues[MembershipQ].pop_back();
	}
	break;

	case TopologyQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexTopo);
		 msg = _messageQueues[TopologyQ].back();
		_messageQueues[TopologyQ].pop_back();
	}
	break;

	case DHTQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexDHT);
		 msg = _messageQueues[DHTQ].back();
		_messageQueues[DHTQ].pop_back();
	}
	break;

	case DataQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexData);
		 msg = _messageQueues[DataQ].back();
		_messageQueues[DataQ].pop_back();
	}
	break;

	default:
	{
		Trace_Error(this, "pollQ", "Error: Unexpected Q type", "type",qtype);
		throw SpiderCastRuntimeError("Unexpected Queue type");
	}

	}//switch

	Trace_Exit(this, "pollQ()");
	return msg;
}

bool IncomingMsgQ::isQEmpty(QType qtype)
{
	Trace_Entry<int>(this, "isQEmpty()", "type", qtype);
	bool empty = true;

	switch (qtype)
	{
	case MembershipQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexMem);
		empty = _messageQueues[MembershipQ].empty();
	}
	break;

	case TopologyQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexTopo);
		empty = _messageQueues[TopologyQ].empty();
	}
	break;

	case DHTQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexDHT);
		empty = _messageQueues[DHTQ].empty();
	}
	break;

	case DataQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexData);
		empty = _messageQueues[DataQ].empty();
	}
	break;

	default:
	{
		Trace_Error(this, "isQEmpty", "Unexpected Q type", "type", qtype);
		throw SpiderCastRuntimeError("Unexpected Queue type");
	}

	}//switch

	Trace_Exit<bool>(this, "isQEmpty()", empty);

	return empty;
}

size_t IncomingMsgQ::getQSize(QType qtype)
{
	//Trace_Entry<int>(this, "getQSize()", "type", qtype);

	size_t size = 0;

	switch (qtype)
	{
	case MembershipQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexMem);
		size = _messageQueues[MembershipQ].size();
	}
	break;

	case TopologyQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexTopo);
		size = _messageQueues[TopologyQ].size();
	}
	break;

	case DHTQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexDHT);
		size = _messageQueues[DHTQ].size();
	}
	break;

	case DataQ:
	{
		boost::recursive_mutex::scoped_lock lock(_mutexData);
		size = _messageQueues[DataQ].size();
	}
	break;

	default:
	{
		Trace_Error(this, "getQSize", "Unexpected Q type", "type", qtype);
		throw SpiderCastRuntimeError("Unexpected Queue type");
	}

	}//switch

	if (size > 0)
	{
		Trace_Dump(this, "getQSize()", "",
				"type", boost::lexical_cast<std::string>(qtype),
				"size", boost::lexical_cast<std::string>(size));
	}

	//Trace_Exit<size_t>(this, "getQSize()", size);
	return size;
}

void IncomingMsgQ::onSuccess(Neighbor_SPtr result, ConnectionContext ctx)
{
	Trace_Entry(this, "onSuccess()", "neighbor", result->toString());
	try
	{
		// create the new connection event for topology
		SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
		(*scMsg).setSender(_nodeIdCache.getOrCreate(result->getName()));
		boost::shared_ptr<CommEventInfo> eventInfo = boost::shared_ptr<
				CommEventInfo>(new CommEventInfo(
						CommEventInfo::On_Connection_Success,
						result->isVirgin() ? 0 : result->getConnectionId(), result));
		(*eventInfo).setContext(ctx);
		(*scMsg).setCommEventInfo(eventInfo);
		this->onMessage(scMsg);
	} catch (SpiderCastRuntimeError& e)
	{
		Trace_Warning(this, "onSuccess()", "SpiderCastRuntimeError, ignored", "what", e.what());
	}
	Trace_Exit(this, "onSuccess()");

}

void IncomingMsgQ::onFailure(String FailedTargetName, int rc, String message,
		ConnectionContext ctx)
{
	Trace_Entry(this, "onFailure()", "target", FailedTargetName);
	try
	{
		SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
		(*scMsg).setSender(_nodeIdCache.getOrCreate(FailedTargetName));
		boost::shared_ptr<CommEventInfo> eventInfo = boost::shared_ptr<
				CommEventInfo>(new CommEventInfo(
						CommEventInfo::On_Connection_Failure, 0));
		(*eventInfo).setErrCode(rc);
		(*eventInfo).setErrMsg(message);
		(*eventInfo).setContext(ctx);
		(*scMsg).setCommEventInfo(eventInfo);
		this->onMessage(scMsg);
	} catch (SpiderCastRuntimeError& e)
	{
		Trace_Warning(this, "onFailure()", "SpiderCastRuntimeError, ignored", "what", e.what());
	}
	Trace_Exit(this, "onFailure()");
}

void IncomingMsgQ::reportStats(boost::posix_time::ptime time, bool labels)
{
	if (ScTraceBuffer::isConfigEnabled(_tc))
	{

		EnumCounter<SCMessage::MessageType, int32_t> inMsgCount;
		EnumCounter<SCMessage::MessageType, int32_t> inMsgBytes;
		EnumCounter<SCMessage::MessageGroup, int32_t> inMsgGroup;

		{
			boost::recursive_mutex::scoped_lock lock(_mutexStats);

			inMsgCount = inMsgCount_;
			inMsgCount_.reset();
			inMsgBytes = inMsgBytes_;
			inMsgBytes_.reset();
			inMsgGroup = inMsgGroup_;
			inMsgGroup_.reset();
		}

		inQueueSize_.set(TopologyQ, getQSize(TopologyQ));
		inQueueSize_.set(MembershipQ, getQSize(MembershipQ));
		inQueueSize_.set(DHTQ, getQSize(DHTQ));
		inQueueSize_.set(DataQ, getQSize(DataQ));

		std::string time_str(boost::posix_time::to_iso_extended_string(time));

		std::ostringstream oss;
		oss << endl;
		if (!labels)
		{
			oss << _instId << ", " << time_str << ", SC_Stats_Comm_InMsgCount, "
					<< inMsgCount.toCounterString() << endl;
			oss << _instId << ", " << time_str << ", SC_Stats_Comm_InMsgBytes, "
					<< inMsgBytes.toCounterString() << endl;
			oss << _instId << ", " << time_str << ", SC_Stats_Comm_InMsgGroup, "
					<< inMsgGroup.toCounterString() << endl;
			oss << _instId << ", " << time_str << ", SC_Stats_Comm_InQsize, "
					<< inQueueSize_.toCounterString() << endl;
		}
		else
		{
			oss << _instId << ", " << time_str << ", SC_Stats_Comm_InMsgCount, "
					<< inMsgCount.toLabelString() << endl;
			oss << _instId << ", " << time_str << ", SC_Stats_Comm_InMsgBytes, "
					<< inMsgBytes.toLabelString() << endl;
			oss << _instId << ", " << time_str << ", SC_Stats_Comm_InMsgGroup, "
					<< inMsgGroup.toLabelString() << endl;
			oss << _instId << ", " << time_str << ", SC_Stats_Comm_InQsize, "
					<< inQueueSize_.toLabelString() << endl;
		}

		ScTraceBufferAPtr buffer = ScTraceBuffer::config(this, "reportStats()", oss.str());
		buffer->invoke();
	}
}

} //namespace
