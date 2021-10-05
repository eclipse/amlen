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
 * CommRumReceiver.cpp
 *
 *  Created on: Feb 16, 2010
 *      Author:
 *
 * Version     : $Revision: 1.8 $
 *-----------------------------------------------------------------
 * $Log: CommRumReceiver.cpp,v $
 * Revision 1.8  2016/12/05 14:27:50 
 * Fix mem leak in Comm; Improve trace for stats.
 *
 * Revision 1.7  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.6.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.6  2015/12/22 11:56:40 
 * print typeid in catch std::exception; fix toString of sons of AbstractTask.
 *
 * Revision 1.5  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.4  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.3  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.2  2015/07/02 10:29:12 
 * clean trace, byte-buffer
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.33  2012/10/25 10:11:07 
 * Added copyright and proprietary notice
 *
 * Revision 1.32  2012/04/02 10:48:29 
 * UDP on IPv6 resolve bug, remove clear-all-streams
 *
 * Revision 1.31  2012/02/20 15:45:04 
 * use R/W byte-buffers in incoming messages
 *
 * Revision 1.30  2011/06/02 12:07:08 
 * Take into consideration that an incoming message might belong to a stream already cleared from our data structures
 *
 * Revision 1.29  2011/06/01 13:36:08 
 * Add traces to find a deadlock
 *
 * Revision 1.28  2011/05/30 17:44:32 
 * Streamline mutex locking; fix potential deadlock with RUM on termination
 *
 * Revision 1.27  2011/04/11 08:28:19 
 * Ensure that a stream to be closed is first rejected by RUM before being deleted from spdr internal data structures
 *
 * Revision 1.26  2011/03/24 08:46:27 
 * verify CRC on message reception
 *
 * Revision 1.25  2011/02/27 12:20:20 
 * Refactoring, header file organization
 *
 * Revision 1.24  2010/12/22 12:04:11 
 * fix typos; no functional change
 *
 * Revision 1.23  2010/12/14 10:47:40 
 * Comm changes - receiver stream id in each message and neighbor, disconnect reject stream, if known and counts number tx and rcv before closing connection.
 *
 * Revision 1.22  2010/12/09 13:39:42 
 * Add synchronization to _cahcedStreams and enhanced tracing
 *
 * Revision 1.21  2010/11/29 11:41:11 
 * Added reciever stream id to SCMessage
 *
 * Revision 1.20  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.18.2.2  2010/11/25 10:04:18 
 * before merge with head - all chages from head inside
 *
 * Revision 1.18.2.1  2010/11/23 14:12:05 
 * merged with HEAD branch
 *
 * Revision 1.18  2010/09/18 17:04:55 
 * Hierarhical bus name support
 *
 * Revision 1.17  2010/08/17 13:28:34 
 * Add getStreamConnection() method; enhanced logging
 *
 * Revision 1.16  2010/08/08 12:54:04 
 * Change code such that no Comm locks are held while calling RUM
 *
 * Revision 1.15  2010/08/03 13:57:31 
 * Add RUM time tracing
 *
 * Revision 1.14  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.13  2010/06/30 09:13:20 
 * Handle termination better
 *
 * Revision 1.12  2010/06/27 12:02:52 
 * Move all implementations from header files
 *
 * Revision 1.11  2010/06/23 14:18:21 
 * No functional chnage
 *
 * Revision 1.10  2010/06/23 12:04:41 
 * Put a message on the incoming queue only if managed to identify the peer it came from
 *
 * Revision 1.9  2010/06/03 14:43:21 
 * Arrange print statmenets
 *
 * Revision 1.8  2010/05/27 09:30:35 
 * Enahnced logging + new source event on connection rather thanstream
 *
 * Revision 1.6  2010/05/23 14:11:51 
 * Insert correct source node name to the internal data structures in acceptStream
 *
 * Revision 1.5  2010/05/20 09:16:59 
 * Pass a new source event to the incoming msg queue
 *
 * Revision 1.4  2010/05/10 08:30:51 
 * Handle termination better
 *
 * Revision 1.3  2010/04/14 09:16:21 
 * Add some print statemnets - no funcional changes
 *
 * Revision 1.2  2010/04/12 09:21:34 
 * Add a mutex for roper synchronization + misc. non-funcational changes
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 *
 *
 *
 */
#include <iostream>

#include <boost/crc.hpp>

#include "CommRumAdapter.h"
#include "CommRumReceiver.h"

using namespace std;

extern "C"
{
static void on_message(rumRxMessage* rumMsg, void *msg_user)
{
	spdr::RumReceiverContext* rrud = (spdr::RumReceiverContext*) msg_user;
	rrud->_receiver->onMessage(rumMsg);
}
static void on_event(const rumEvent* event, void *event_user)
{
	spdr::RumReceiverContext* rrud = (spdr::RumReceiverContext*) event_user;
	rrud->_receiver->onEvent(event);
}
static int accept_stream(rumStreamParameters *stream_params, void *user)
{
	spdr::RumReceiverContext* rrud = (spdr::RumReceiverContext*) user;
	return rrud->_receiver->acceptStream(stream_params);
}
}

namespace spdr
{

using namespace trace;

ScTraceComponent* CommRumReceiver::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Comm,
		trace::ScTrConstants::Layer_ID_CommAdapter, "CommRumReceiver",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

void CommRumReceiver::start()
{
	Trace_Entry(this, "start()");
	int errorCode;
	int res = createRumReceiver(&errorCode);
	if (res != RUM_SUCCESS)
	{
		char errorDesc[1024];
		rumGetErrorDescription(errorCode, errorDesc, 1024);
		string error("CommRumReceiver::start Failed to create RUM Receiver : ");
		error += errorDesc;
		Trace_Event(this, "start()", error);
		throw SpiderCastRuntimeError(error);

	}
	ostringstream oss;
	oss << _queueR.handle;
	_started = true;
	Trace_Exit(this, "start()", "success", oss.str());
}

void CommRumReceiver::onMessage(rumRxMessage* rumMsg)
{
	Trace_Entry(this, "onMessage()");

	if (_closed)
		return;

	rumStreamID_t sid = rumMsg->stream_info->stream_id;

	if (ScTraceBuffer::isDebugEnabled(_tc))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "onMessage()");
		buffer->addProperty<uint64_t> ("sid", sid);
		buffer->addProperty<int> ("length", rumMsg->msg_len);
		buffer->invoke();
	}

	uint32_t rumCRC = 0;
	if (_crcMemTopoEnabled)
	{
		//CRC on raw RUM buffer
		boost::crc_32_type crcCode;
		crcCode.process_bytes(rumMsg->msg_buf, rumMsg->msg_len - 4);
		rumCRC = crcCode.checksum();
	}

	boost::shared_ptr<ByteBuffer> buffer(ByteBuffer::cloneByteBuffer(
				rumMsg->msg_buf, rumMsg->msg_len)); //allocate and copy into a R/W buffer

	SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
	(*scMsg).setBuffer(buffer);

	try
	{
//		String sender = getStreamSender(sid);
//		BusName_SPtr busName = getStreamBusName(sid);
		String sender;
		BusName_SPtr busName;
		StringSPtr senderLocalName;
		bool found = getStreamNames(sid,sender,busName,senderLocalName);
		if (!found || sender.empty() || !busName)
		{
			Trace_Event(this, "onMessage", "Discarding message; origin not found internally",
					"sid", spdr::stringValueOf(sid));
			return;
		}
		scMsg->setSender(_nodeIdCache.getOrCreate(sender));
		scMsg->setStreamId(sid);
		scMsg->setBusName(busName);
		scMsg->setSenderLocalName(senderLocalName);
		Trace_Debug(this, "onMessage()", "",
				"sender", sender,
				"bus", busName->toOrgString(),
				"senderLocalName", (senderLocalName ? *senderLocalName : "null"),
				"sid", stringValueOf<streamId_t> ((*scMsg).getStreamId()));

		if (_crcMemTopoEnabled)
		{
			scMsg->verifyCRCchecksum(); //may throw
		}

		_incomingMsgQ->onMessage(scMsg);
	} catch (SpiderCastRuntimeError& e)
	{
		e.printStackTrace(); //to stderr

		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "onMessage()",
				"Exception");
		buffer->addProperty<uint64_t> ("sid", sid);
		buffer->addProperty<int> ("length", rumMsg->msg_len);
		buffer->addProperty<uint32_t> ("rumCRC", rumCRC);
		buffer->addProperty("what", e.what());
		buffer->addProperty("bt", e.getStackTrace());
		buffer->invoke();
		throw;
	}

	Trace_Exit(this, "onMessage()");
}

void CommRumReceiver::onEvent(const rumEvent* event)
{

	ostringstream oss;
	oss << event->type;
	Trace_Entry(this, "onEvent()", "Received", "queue name", event->queue_name,
			oss.str());

	if (_closed)
		return;
	// process the event

	int type = event->type;

	switch (type)
	{
	// Failures
	case RUM_PACKET_LOSS:
	case RUM_HEARTBEAT_TIMEOUT:
	case RUM_VERSION_CONFLICT:
	case RUM_RELIABILITY:
	case RUM_CLOSED_TRANS:
	case RUM_STREAM_ERROR:
	case RUM_STREAM_BROKE:
	case RUM_STREAM_NOT_PRESENT:
	case RUM_NEW_SOURCE_FAILED:
	case RUM_RECEIVE_QUEUE_TRIMMED:
	case RUM_MESSAGE_LOSS:
	case RUM_REPAIR_DELAY:
	case RUM_RELIABILITY_CHANGED:
		onBreak(event->stream_id);
		break;
		// new source
	case RUM_NEW_SOURCE:
	{
		String queueName(event->queue_name);
		Trace_Event(this, "onEvent()", "new source received from",
				"queue", queueName);
		String nodeName = queueName.substr(queueName.find_last_of(",") + 1);

		RumReceiverInfo_SPtr receiverInfo;

		{
			boost::recursive_mutex::scoped_lock lock(_mapMutex);
			CachedStreams_Map::iterator iter = _cachedStreams.find(event->stream_id);
			if (iter != _cachedStreams.end())
			{
				receiverInfo = iter->second;
			}
		}

		if (!receiverInfo)
		{
			Trace_Event(this, "onEvent()",
					"Warning: Received an RUM_NEW_SOURCE event, can't find stream in map",
					"sid", spdr::stringValueOf(event->stream_id));
		}
		else
		{
			rumConnectionID_t connectionId = receiverInfo->getConnection();
			std::string senderLocalName = _listener->onNewStreamReceived(event->stream_id, nodeName, connectionId);

			{
				boost::recursive_mutex::scoped_lock lock(_mapMutex);
				receiverInfo->setSenderLocalName(senderLocalName);
			}
			Trace_Debug(this, "onEvent()", "RUM_NEW_SOURCE, updated",
					"sid", spdr::stringValueOf(event->stream_id), "receiverInfo", receiverInfo->toString());
		}
	}

		break;
	case RUM_FIRST_MESSAGE:
	case RUM_LATE_JOIN_FAILURE:
		Trace_Event(this, "onEvent()", "ignoring event", "type", oss.str());
		break;
	default:
		Trace_Event(this, "onEvent()", "received unknown event", "type",
				oss.str());
		break;
	}
	Trace_Exit(this, "onEvent()");
}

int CommRumReceiver::acceptStream(rumStreamParameters *stream_params)
{
	Trace_Entry(this, "acceptStream()");

	if (_closed)
		return RUM_STREAM_REJECT;

	if (stream_params->event->queue_name == NULL)
	{
		Trace_Warning(this,"acceptStream()","Warning: NULL queue name, rejecting stream");
		Trace_Exit(this, "acceptStream()", "REJECT");
		return RUM_STREAM_REJECT;
	}

	String queueName(stream_params->event->queue_name);
	try
	{
		rumStreamID_t sid = stream_params->stream_id;

		size_t first = queueName.find_first_of(",");
		size_t last = queueName.find_last_of(",");
		if (first == std::string::npos || last == std::string::npos || first >= last)
		{
			Trace_Warning(this,"acceptStream()","Warning: illegal queue name, rejecting stream", "queue-name",queueName);
			Trace_Exit(this, "acceptStream()", "REJECT");
			return RUM_STREAM_REJECT;
		}

		String prefix = queueName.substr(0,first);
		String busName = queueName.substr(first+1, last-first-1);
		String senderName = queueName.substr(last+1);
		BusName busNameObj(busName.c_str());

		Trace_Event(this, "acceptStream()", "Details",
				"queue-name", queueName,
				"bus name", busName,
				"sender", senderName,
				"sid", boost::lexical_cast<String>(sid));

		// do accept stream logic
		if (prefix != CommRumTxMgr::queueNamePrefix)
		{
			Trace_Exit(this, "acceptStream()", "REJECT (bad prefix)");
			return RUM_STREAM_REJECT;
		}

		if (busNameObj.comparePrefix(*_thisBusName) > 0)
		{
			addStream(sid, senderName, busName,
					stream_params->rum_connection->connection_id);
			Trace_Exit(this, "acceptStream()", "ACCEPT");
			return RUM_STREAM_ACCEPT;
		}
		else
		{
			Trace_Exit(this, "acceptStream()", "REJECT (bad bus)");
			return RUM_STREAM_REJECT;
		}
	}
	catch (std::exception& e)
	{
		Trace_Warning(this,"acceptStream()","Warning: failure to parse queue name, rejecting stream",
				"queue-name", queueName, "what",e.what(), "typeid",typeid(e).name());
		Trace_Exit(this, "acceptStream()", "REJECT (parse error)");
		return RUM_STREAM_REJECT;
	}
}

int CommRumReceiver::createRumReceiver(int* p_error_code)
{
	Trace_Entry(this, "createRumReceiver()");

	rumRxQueueParameters config;

	Trace_Event(this, "createRumReceiver()",
			"before rumInitStructureParameters");

	int res = rumInitStructureParameters(RUM_SID_RxQUEUEPARAMETERS, &config,
			RUMCAPI_VERSION, p_error_code);
	Trace_Event(this, "createRumReceiver()", "after rumInitStructureParameters");
	if (res == RUM_SUCCESS)
	{
		_rumReceiverContext_SPtr.reset(new RumReceiverContext(this));
		config.reliability = RUM_RELIABLE_RCV;
		config.accept_stream = accept_stream;
		config.accept_user = _rumReceiverContext_SPtr.get();
		config.on_event = on_event;
		config.event_user = _rumReceiverContext_SPtr.get();
		config.on_message = on_message;
		config.user = _rumReceiverContext_SPtr.get();
		Trace_Event(this, "createRumReceiver()",
				"RUMTimeStamp before rumRCreateQueue");
		int rc = rumRCreateQueue(&_rum, &config, &_queueR, p_error_code);
		Trace_Event(this, "createRumReceiver()",
				"RUMTimeStamp after rumRCreateQueue");
		return rc;
	}
	ostringstream oss;
	oss << res;
	Trace_Exit(this, "createRumReceiver()", "res", oss.str());
	return res;
}

int CommRumReceiver::rejectStream(rumStreamID_t sid, int *p_rc, bool rum_reject)
{
	Trace_Entry(this, "rejectStream()", "streamId",
			stringValueOf<rumStreamID_t> (sid));

	if (_closed)
		return RUM_FAILURE;

	int res = RUM_SUCCESS;

	if (rum_reject)
	{
		int rc;
		res = rumRRemoveStream(&_rum, sid, &rc);
		if (res != RUM_SUCCESS)
		{
			char errorDesc[1024];
			rumGetErrorDescription(rc, errorDesc, 1024);
			String error("Unable to remove receiver: ");
			error += errorDesc;
			error += " (benign event)";
			Trace_Event(this, "rejectStream()", error);
		}
		else
		{
			Trace_Event(this, "rejectStream()", "removed receiver");
		}
	}

	removeStream(sid);
	Trace_Exit<int> (this, "rejectStream()", res);
	return res;
}

void CommRumReceiver::removeStream(rumStreamID_t sid)
{
	Trace_Entry(this, "removeStream()");

	boost::recursive_mutex::scoped_lock lock(_mapMutex);
	if (_closed)
		return;

	CachedStreams_Map::iterator it =
			_cachedStreams.find(sid);
	if (it != _cachedStreams.end())
	{
		Trace_Event(this, "removeStream()", "removed receiver", "streamId",
				stringValueOf<rumStreamID_t> (sid));
		_cachedStreams.erase(it);
	}
	else
	{
		Trace_Event(this, "removeStream()", "can't find stream", "streamId",
				stringValueOf<rumStreamID_t> (sid), "_cachedStreams",
				stringValueOf<size_t> (_cachedStreams.size()));
	}
	Trace_Exit(this, "removeStream()");
}

//void CommRumReceiver::clearRejectedStreams(void)
//{
//	Trace_Entry(this, "clearRejectedStreams()");
//
//	if (_closed)
//		return;
//	int errorCode;
//
//	int rc = rumRClearRejectedStreams(&_rum, &errorCode);
//	if (rc != RUM_SUCCESS)
//	{
//		// LOG
//	}
//	Trace_Exit(this, "clearRejectedStreams()");
//}

void CommRumReceiver::onBreak(rumStreamID_t sid)
{
	ostringstream oss;
	oss << sid;
	Trace_Entry(this, "onBreak()", "sid", oss.str());
	if (_closed)
		return;

	// notify upwards
	removeStream(sid);
	_listener->onStreamBreak(sid);
	Trace_Exit(this, "onBreak()");
}

void CommRumReceiver::terminate()
{
	Trace_Entry(this, "terminate()");
	CachedStreams_Map _cachedStreamsCopy;
	{
		boost::recursive_mutex::scoped_lock lock(_mapMutex);

		_closed = true;

		_cachedStreamsCopy = _cachedStreams;
		_cachedStreams.clear();
	}

	for (CachedStreams_Map::iterator iter =
			_cachedStreamsCopy.begin(); iter != _cachedStreamsCopy.end(); ++iter)
	{
		int rc;
		int res = rumRRemoveStream(&_rum, iter->first, &rc);
		if (res != RUM_SUCCESS)
		{
			char errorDesc[1024];
			rumGetErrorDescription(rc, errorDesc, 1024);
			String error("Failed to close RStream : ");
			error += errorDesc;
			Trace_Event(this, "terminate()", error);
		}
		else
		{
			ostringstream oss;
			oss << iter->first;
			Trace_Event(this, "terminate()", "removed", "sid", oss.str());
		}
	}

	//_cachedStreams.clear();

	int error_code;
	if (_started)
	{
		int res = rumRCloseQueue(&_queueR, &error_code);
		if (res != RUM_SUCCESS)
		{
			char errorDesc[1024];
			rumGetErrorDescription(error_code, errorDesc, 1024);
			String error("Failed to close QueueR : ");
			error += errorDesc;
			Trace_Event(this, "terminate()", error);
		}
		else
		{
			Trace_Event(this, "terminate()", "removed queueR");
		}
	}
	Trace_Exit(this, "terminate()");
}


bool CommRumReceiver::getStreamNames(rumStreamID_t sid, String& sender, BusName_SPtr& busName, StringSPtr& senderLocalName) const
{
	Trace_Entry(this, "getStreamNames()","sid", spdr::stringValueOf(sid));

	boost::recursive_mutex::scoped_lock lock(_mapMutex);
	if (_closed)
	{
		Trace_Exit(this, "getStreamNames()","(closed)");
		return false;
	}

	CachedStreams_Map::const_iterator iter =
			_cachedStreams.find(sid);
	if (iter != _cachedStreams.end())
	{
		Trace_Debug(this, "getStreamNames()", "found",
				"sid", spdr::stringValueOf(sid),
				"sender", iter->second->getSender(),
				"bus", iter->second->getBusName()->toOrgString(),
				"senderLocalName", (iter->second->getSenderLocalName() ? *(iter->second->getSenderLocalName()) : "null"));
		sender = iter->second->getSender();
		busName = iter->second->getBusName();
		senderLocalName = iter->second->getSenderLocalName();

		return true;
	}
	else
	{
		Trace_Event(this, "getStreamNames()", "not found", "sid", spdr::stringValueOf(sid));
		return false;
	}
}

//const String CommRumReceiver::getStreamSender(rumStreamID_t sid) const
//{
//	Trace_Entry(this, "getStreamSender()","sid", spdr::stringValueOf(sid));
//
//	boost::recursive_mutex::scoped_lock lock(_mapMutex);
//	if (_closed)
//	{
//		Trace_Exit(this, "getStreamSender()","(closed)");
//		return "";
//	}
//
//	CachedStreams_Map::const_iterator iter =
//			_cachedStreams.find(sid);
//	if (iter != _cachedStreams.end())
//	{
//		Trace_Debug(this, "getStreamSender()", "",
//				"sid", spdr::stringValueOf(sid), "sender", iter->second->getSender());
//		return iter->second->getSender();
//	}
//	else
//	{
//		Trace_Event(this, "getStreamSender()", "not found", "sid", spdr::stringValueOf(sid));
//		return "";
//	}
//}
//
//const BusName_SPtr CommRumReceiver::getStreamBusName(
//		rumStreamID_t sid) const
//{
//	Trace_Entry(this, "getStreamBusName()", "sid", spdr::stringValueOf(sid));
//
//	boost::recursive_mutex::scoped_lock lock(_mapMutex);
//	if (_closed)
//	{
//		Trace_Exit(this, "getStreamBusName()", "(closed)");
//		return BusName_SPtr ();
//	}
//
//	ostringstream oss;
//	oss << sid;
//	CachedStreams_Map::const_iterator iter =
//			_cachedStreams.find(sid);
//	if (iter != _cachedStreams.end())
//	{
//		Trace_Debug(this, "getStreamBusName()", "",
//				"sid", spdr::stringValueOf(sid),
//				"bus", iter->second->getBusName()->toOrgString());
//		return iter->second->getBusName();
//	}
//	else
//	{
//		Trace_Event(this, "getStreamBusName()","not found",
//				"sid", spdr::stringValueOf(sid));
//		return BusName_SPtr ();
//	}
//}

const rumConnectionID_t CommRumReceiver::getStreamConnection(rumStreamID_t sid) const
{
	Trace_Entry(this, "getStreamConnection()");
	boost::recursive_mutex::scoped_lock lock(_mapMutex);
	if (_closed)
		return -1;
	ostringstream oss;

	CachedStreams_Map::const_iterator iter =
			_cachedStreams.find(sid);
	if (iter != _cachedStreams.end())
	{
		oss << "sid=" << sid << "; sender=" << iter->second->getSender()
				<< "; busName=" << iter->second->getBusName()->toOrgString()
				<< "; connection=" << iter->second->getConnection();
		Trace_Event(this, "getStreamConnection()", oss.str());
		//TODO: no need to create a new string
		Trace_Exit(this, "getStreamConnection()");
		return iter->second->getConnection();
	}
	else
	{
		oss << sid;
		Trace_Event(this, "getStreamConnection() not found", "for", "sid",
				oss.str());
		Trace_Exit(this, "getStreamConnection()");
		/*throw SpiderCastRuntimeError(
				_instID
						+ ": CommRumReceiver::getStreamConnection Couldn't find corresponding stream connection");*/
		return -1;
	}

}

void CommRumReceiver::addStream(rumStreamID_t sid, const String& sender,
		const String& busName, rumConnectionID_t con)
{
	Trace_Entry(this, "addStream()", "node", sender, "streamId", stringValueOf<
			rumStreamID_t> (sid));

	boost::recursive_mutex::scoped_lock lock(_mapMutex);
	if (_closed)
		return;

	CachedStreams_Map::iterator it =
			_cachedStreams.find(sid);
	if (it == _cachedStreams.end())
	{
		RumReceiverInfo_SPtr streamInfo(new RumReceiverInfo(sender, busName, con));
		_cachedStreams.insert(std::make_pair(sid, streamInfo));
	}
	else
	{
		Trace_Event(this, "addStream()", "already found in the map");
	}
	Trace_Exit(this, "addStream()", "node", sender);
}

CommRumReceiver::~CommRumReceiver()
{
	Trace_Entry(this, "~CommRumReceiver");
}

}
