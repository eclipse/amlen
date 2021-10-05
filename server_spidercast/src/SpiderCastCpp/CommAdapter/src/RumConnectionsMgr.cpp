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
 * RumConnectionMgr.cpp
 *
 *  Created on: Feb 15, 2010
 *      Author:
 *
 * Version     : $Revision: 1.22 $
 *
 * Version     : $Revision: 1.22 $
 *-------------------------------
 * $Log: RumConnectionsMgr.cpp,v $
 * Revision 1.22  2016/12/05 14:27:50 
 * Fix mem leak in Comm; Improve trace for stats.
 *
 * Revision 1.21  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.19.2.3  2016/01/27 09:07:16 
 * nameless tcp discovery
 *
 * Revision 1.19.2.2  2016/01/25 14:50:03 
 * bug fix: allow removed nodes in view change
 * bug fix: invalid iterator
 * allow $ANY prefix in connect message target
 *
 * Revision 1.20  2016/01/24 12:13:44 
 * bug fix: invalid iterator
 *
 *
 * Version     : $Revision: 1.22 $
 *-------------------------------
 * $Log: RumConnectionsMgr.cpp,v $
 * Revision 1.22  2016/12/05 14:27:50 
 * Fix mem leak in Comm; Improve trace for stats.
 *
 * Revision 1.21  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.20  2016/01/24 12:13:44 
 * bug fix: invalid iterator
 *
 * Revision 1.19.2.3  2016/01/27 09:07:16 
 * nameless tcp discovery
 *
 * Revision 1.19.2.2  2016/01/25 14:50:03 
 * bug fix: allow removed nodes in view change
 * bug fix: invalid iterator
 * allow $ANY prefix in connect message target
 *
 * Revision 1.19.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.19  2016/01/19 13:50:24 
 * protect _ctx with mutex
 *
 * Revision 1.18  2016/01/07 14:41:18 
 * bug fix: closeStream with virgin RumNeighbor
 *
 * Revision 1.17  2015/12/22 11:56:40 
 * print typeid in catch std::exception; fix toString of sons of AbstractTask.
 *
 * Revision 1.16  2015/12/02 09:31:47 
 * code cleanup
 *
 * Revision 1.15  2015/11/30 09:42:00 
 * prevent double leave on a duplicate node suspicion
 *
 * Revision 1.14  2015/11/26 09:52:11 
 * Bug fix: handle RUM_CONNECTION_ESTABLISH_TIMEOUT on incoming pending connections
 *
 * Revision 1.13  2015/11/24 13:19:50 
 * improve trace
 *
 * Revision 1.12  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.11  2015/11/12 10:28:16 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.10  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.9  2015/10/09 06:56:11 
 * bug fix - when neighbor in the table but newNeighbor not invoked yet, don't send to all neighbors but only to routable neighbors. this way the node does not get a diff mem/metadata before the full view is received.
 *
 * Revision 1.8  2015/08/27 10:53:53 
 * change RUM log level dynamically
 *
 * Revision 1.7  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.6  2015/07/29 09:26:11 
 * split brain
 *
 * Revision 1.5  2015/07/13 08:26:21 
 * *** empty log message ***
 *
 * Revision 1.4  2015/06/30 13:26:45 
 * Trace 0-8 levels, trace levels by component
 *
 * Revision 1.3  2015/06/25 10:26:45 
 * readiness for version upgrade
 *
 * Revision 1.2  2015/05/05 12:48:26 
 * Safe connect
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.69  2015/01/26 12:38:14 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.68  2012/10/25 10:11:07 
 * Added copyright and proprietary notice
 *
 * Revision 1.67  2012/06/27 12:38:57 
 * Seperate generic COMM from RUM
 *
 * Revision 1.66  2012/03/01 13:19:50 
 * Ensure that when there are different pending request for a connection that each gets a different neighbor
 *
 * Revision 1.65  2012/02/14 12:43:32 
 * change comment. no functional changes
 *
 * Revision 1.64  2011/07/10 18:03:14 
 * Remove compilation warnings
 *
 * Revision 1.63  2011/07/10 09:23:52 
 * Create an RUM Tx on a seperate thread (not thr RUM thread)
 *
 * Revision 1.62  2011/06/20 11:39:37 
 * If a tx creation on an existing connetcion fails, turn to async connection creation
 *
 * Revision 1.61  2011/06/19 13:48:06 
 * *** empty log message ***
 *
 * Revision 1.60  2011/06/01 13:36:28 
 * Add traces to find a deadlock
 *
 * Revision 1.59  2011/05/30 17:45:42 
 * Streamline mutex locking
 *
 * Revision 1.58  2011/05/22 12:52:35 
 * Introduce a local neighbor
 *
 * Revision 1.57  2011/04/04 13:11:46 
 * Change onBreak not to close RUM resources; leave them around to be closed by topo calling disconect()
 *
 * Revision 1.55  2011/03/22 09:19:41 
 * Remove entry from outPending in case the connetion creation process failed
 *
 * Revision 1.54  2011/03/16 13:37:37 
 * Add connectivity event; fix a couple of bugs in topology
 *
 * Revision 1.53  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.52  2011/02/01 13:56:36 
 * In case of a failure to create tx on an existing connection only return false and not pass an onFailure to the corresponding listener
 *
 * Revision 1.51  2011/01/25 13:02:08 
 * *** empty log message ***
 *
 * Revision 1.50  2011/01/17 13:51:37 
 * Add busName to commEventInfo onBreak
 *
 * Revision 1.49  2011/01/10 12:26:28 
 * Remove context from connectOnExisting() method
 *
 * Revision 1.48  2010/12/26 07:46:41 
 * Fix spelling getreceiverId()
 *
 * Revision 1.47  2010/12/22 13:35:01 
 * Simplify closeAllConnections()
 *
 * Revision 1.46  2010/12/22 12:09:10 
 * closeStream() - simplify and fix a couple of bugs
 *
 * Revision 1.45  2010/12/14 10:47:40 
 * Comm changes - receiver stream id in each message and neighbor, disconnect reject stream, if known and counts number tx and rcv before closing connection.
 *
 * Revision 1.44  2010/11/25 16:15:05 
 * After merge of comm changes back to head
 *
 * Revision 1.42.2.5  2010/11/25 14:42:11 
 * minor changes after code review and merge with HEAD
 *
 * Revision 1.42.2.4  2010/11/23 14:12:05 
 * merged with HEAD branch
 *
 * Revision 1.42.2.3  2010/11/15 12:46:32 
 * Hopefully,last bug fixed
 *
 * Revision 1.42.2.2  2010/10/21 18:01:49 
 * adding context to On_Connection_Success event and fixing bug in connection manager
 *
 * Revision 1.42.2.1  2010/10/21 16:10:42 
 * Massive comm changes
 *
 * Revision 1.42  2010/09/18 17:04:55 
 * Hierarhical bus name support
 *
 * Revision 1.41  2010/08/31 14:02:35 
 * More debug info in createConnection
 *
 * Revision 1.40  2010/08/23 15:26:42 
 * Update tx data structures at the correct location
 *
 * Revision 1.39  2010/08/23 09:21:54 
 * Initialize some RUM connection properties from the config object
 *
 * Revision 1.38  2010/08/22 13:22:08 
 * Switch onSuccess to receive as a paramtere CSP(Neighbor) rather than (Neighbor *)
 *
 * Revision 1.37  2010/08/19 12:41:13 
 * Enhanced logging
 *
 * Revision 1.36  2010/08/17 13:29:46 
 * Remove the RUM config param, as it should be done seperately for each call; enhanced logging
 *
 * Revision 1.35  2010/08/15 11:48:45 
 * Eliminate compilation warnings, no functional change
 *
 * Revision 1.34  2010/08/12 15:31:04 
 * Log clarity' no functional change
 *
 * Revision 1.33  2010/08/11 09:07:06 
 * Synchronization issues - move call to createtX inside onSuccess() outside the mutex
 *
 * Revision 1.32  2010/08/10 09:52:19 
 * Update tx map when a new tx is created on an existing connection
 *
 * Revision 1.31  2010/08/09 13:45:25 
 * Add timrstampe before and after RUM closing of queues and connections
 *
 * Revision 1.30  2010/08/08 12:54:04 
 * Change code such that no Comm locks are held while calling RUM
 *
 * Revision 1.29  2010/08/05 11:04:51 
 * Handle RUM connections closing more carefully
 *
 * Revision 1.28  2010/08/05 09:26:05 
 * Init the RUM connection config structure only once
 *
 * Revision 1.27  2010/08/04 12:37:21 
 * Always place  onFailure calls in the incoming msg Q
 *
 * Revision 1.26  2010/08/03 13:58:33 
 * Add support for on_success and on_failure connection events to be passed in through the incoming msg Q + add RUM time tracing
 *
 * Revision 1.25  2010/08/02 17:13:04 
 * Use myNodeId in order to select an appropriate arget IP address
 *
 * Revision 1.24  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.23  2010/07/01 18:32:09 
 * Change createConnection to call asyncCompletionListener->onFailure when failing immediately, and not already in the outpending list
 *
 * Revision 1.22  2010/06/30 09:14:27 
 * Handle termination better
 *
 * Revision 1.21  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.20  2010/06/27 14:00:16 
 * Rename print to toString and have it return a string rather than print
 *
 * Revision 1.19  2010/06/27 12:02:52 
 * Move all implementations from header files
 *
 * Revision 1.18  2010/06/27 08:23:55 
 * Pass RUM error code to ConnectionsAsyncCompletionLIstener::onFailure()
 *
 * Revision 1.17  2010/06/23 14:19:16 
 * No functional chnage
 *
 * Revision 1.16  2010/06/23 12:36:35 
 * Added error printout in case received onBreak on a not-known connection
 *
 * Revision 1.15  2010/06/21 07:40:34 
 * Close queueT only if bool flag=true
 *
 * Revision 1.14  2010/06/20 15:59:12 
 * Change neighbor's map into queueT map, close qt, upon connection closure
 *
 * Revision 1.13  2010/06/20 14:42:13 
 * Change the connection info to consist of the connection id; handle termination better
 *
 * Revision 1.12  2010/06/17 11:44:17 
 * Add connection info to topology events. For debugging purposes
 *
 * Revision 1.11  2010/06/14 15:53:59 
 * Avoid a deadlock between local mutex and topology mutex; handle in and out pending lists better
 *
 * Revision 1.10  2010/06/03 14:43:22 
 * Arrange print statmenets
 *
 * Revision 1.9  2010/06/03 12:27:41 
 * No functional change
 *
 * Revision 1.8  2010/05/30 15:31:44 
 * close connection on stream creation failure; implement onBreak
 *
 * Revision 1.7  2010/05/27 09:33:23 
 * Enahnced logging + new source event on connection rather thanstream
 *
 * Revision 1.6  2010/05/20 09:23:22 
 * change createConnection to take a nodeIdImpl
 *
 * Revision 1.5  2010/05/10 08:31:09 
 * Use a new interface: ConnectionsAsyncCompletionListener rather than an asyncCompletionListener template
 *
 * Revision 1.4  2010/05/06 09:34:02 
 * Declare mutexes as mutable so they can be grabbed in const methods
 *
 * Revision 1.3  2010/04/12 09:22:24 
 * Add a mutex for roper synchronization
 *
 * Revision 1.2  2010/04/07 13:54:58 
 * Make it compile on Linux
 *
 * Revision 1.1  2010/03/07 10:44:46 
 * Intial Version of the Comm Adapter
 *
 *
 *
 */

#include <boost/algorithm/string/predicate.hpp>

#include "RumConnectionsMgr.h"

#ifdef RUM_UNIX
#include <string.h>
#endif

#include <iostream>
#include <sstream>
#include <boost/lexical_cast.hpp>

using namespace std;

extern "C"
{

//static int onConnectionEvent(rumConnectionEvent *connection_event, void *user)
//{
//	return spdr::RumConnectionsMgr::on_connection_event(connection_event, user);
//}

static int onConnectionEvent(rumConnectionEvent *connection_event, void *user)
{
	//------------------------------------------------------------------------------
	// Parse context
	//------------------------------------------------------------------------------
	spdr::RumCnContext* rumCnCtx = (spdr::RumCnContext*) user;
	spdr::RumConnectionsMgr* cnMgr = rumCnCtx->getConMgr();
	int event_context = rumCnCtx->getContext();

	return cnMgr->onRumConnectionEvent(connection_event, event_context);
}

}

namespace spdr
{

using namespace trace;

ScTraceComponent* RumConnectionsMgr::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Comm,
		trace::ScTrConstants::Layer_ID_CommAdapter, "RumConnectionsMgr",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

RumConnectionsMgr::RumConnectionsMgr(BusName_SPtr thisBusName,
		const char* thisMemberName, rumInstance& rum, NodeIDCache& cache,
		IncomingMsgQ_SPtr incomingMsgQ, const String& instID,
		NodeIDImpl_SPtr myNodeId, const SpiderCastConfigImpl& config, int64_t incarnationNumber) :
	ScTraceContext(_tc, instID, thisMemberName),
	_rum(rum),
	_thisMemberName(thisMemberName),
	_thisBusName(thisBusName),
	_txMgr(_thisBusName,_thisMemberName, _rum, instID, static_cast<CommRumTxMgrListener&>(*this)),
	_nodesConnectionsMap(),
	_outPendingMap(),
	_inPendingMap(),
	_txsMap(),
	_receivedStreams(),
	_connectionBusName(),
	_ctx(0),
	_nodeIdCache(cache),
	_incomingMsgQ(incomingMsgQ),
	_started(false),
	_closed(false),
	_instID(instID),
	_myNodeId(myNodeId),
	_incarnationNumber(incarnationNumber),
	_connectMessage_buffer(),
	_connectMessage_prefix_pos(0),
	_connectMessage_min_size(sizeof(config::SPIDERCAST_VERSION)*2 + 8 + 8 + 2),
	_config(config)
{
	Trace_Entry(this, "RumConnectionsMgr()");

	_connectMessage_buffer = ByteBuffer::createByteBuffer(1024);
	//In the base version the Supported-version is always equal to the Used-version
	_connectMessage_buffer->writeShort(static_cast<int16_t>(config::SPIDERCAST_VERSION));//Supported
	_connectMessage_buffer->writeShort(static_cast<int16_t>(config::SPIDERCAST_VERSION));//Used
	_connectMessage_buffer->writeString(_thisBusName->toOrgString());
	_connectMessage_buffer->writeString(String(_thisMemberName));
	_connectMessage_buffer->writeLong(incarnationNumber);
	_connectMessage_prefix_pos = _connectMessage_buffer->getPosition();

	_rumcnCtx = new RumCnContext(this, -1);

	Trace_Exit(this, "RumConnectionsMgr()");
}

RumConnectionsMgr::~RumConnectionsMgr()
{
	Trace_Entry(this, "~RumConnectionsMgr()");
	delete _rumcnCtx;
	Trace_Exit(this, "~RumConnectionsMgr()");
}

void RumConnectionsMgr::terminate(bool grace)
{
	Trace_Entry(this, "terminate()");
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		if (_closed)
			return;

		_closed = true;
	}

	if (_started)
	{
		closeAllConnections();
	}
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		clear();
		_txMgr.terminate();
	}
	Trace_Exit(this, "terminate()");
}

bool RumConnectionsMgr::addConnection(const String& node,
		const rumConnection& connection)
{
	Trace_Entry(this, "addConnection()", "node", node, "conn-id", spdr::stringValueOf(connection.connection_id));

	boost::recursive_mutex::scoped_lock lock(_mutex);

	if (_nodesConnectionsMap.count(node) > 0)
	{
		Trace_Event(this, "addConnection()",
				"already contains a connection for", "node", node);
		Trace_Exit(this, "addConnection()", "false");
		return false;
	}

	_nodesConnectionsMap.insert(make_pair(node, connection));
	Trace_Exit(this, "addConnection()", "true");
	return true;
}

void RumConnectionsMgr::clear()
{
	Trace_Entry(this, "clear()");
	boost::recursive_mutex::scoped_lock lock(_mutex);

	_nodesConnectionsMap.clear();
	_outPendingMap.clear();
	_inPendingMap.clear();
	_txsMap.clear();

	Trace_Exit(this, "clear()");
}

const rumConnection& RumConnectionsMgr::getAConnection(const String& node) const
{
	Trace_Entry(this, "getAConnection()");

	boost::recursive_mutex::scoped_lock lock(_mutex);

	return _nodesConnectionsMap.find(node)->second;
}

bool RumConnectionsMgr::removeOutPending(int context)
{
	ostringstream oss;
	oss << "context: " << context;
	Trace_Entry(this, "removeOutPending()", oss.str());

	boost::recursive_mutex::scoped_lock lock(_mutex);

	return (_outPendingMap.erase(context) == 1);
}

bool RumConnectionsMgr::removeOutPending(const String & node)
{
	Trace_Entry(this, "removeOutPending()", "node", node);

	boost::recursive_mutex::scoped_lock lock(_mutex);

	OutPending_Map::const_iterator iter = _outPendingMap.begin();
	while (iter != _outPendingMap.end())
	{
		if (!iter->second->getTargetName().compare(node))
		{
			_outPendingMap.erase(iter->first);
			Trace_Exit(this, "removeOutPending()");
			return true;
		}
		iter++;
	}
	Trace_Exit(this, "removeOutPending()");
	return false;
}

bool RumConnectionsMgr::removeInPending(const rumConnection& con)
{
	ostringstream oss;
	oss << "connection: " << con.connection_id;
	Trace_Entry(this, "removeInPending()", oss.str());

	boost::recursive_mutex::scoped_lock lock(_mutex);

	return _inPendingMap.erase(con.connection_id) == 1;
}

bool RumConnectionsMgr::containsOutNodePending(const String & node) const
{
	Trace_Entry(this, "containsOutNodePending()", "node", node);

	boost::recursive_mutex::scoped_lock lock(_mutex);
	OutPending_Map::const_iterator iter = _outPendingMap.begin();
	while (iter != _outPendingMap.end())
	{
		if (!iter->second->getTargetName().compare(node))
		{
			Trace_Exit(this, "containsOutNodePending()");
			return true;
		}
		iter++;
	}
	Trace_Exit(this, "containsOutNodePending()");
	return false;
}

bool RumConnectionsMgr::containsInNodePending(const String & node) const
{
	Trace_Entry(this, "containsInNodePending()", "node", node);

	boost::recursive_mutex::scoped_lock lock(_mutex);
	map<rumConnectionID_t, InPendingInfo_SPtr>::const_iterator iter =
			_inPendingMap.begin();
	while (iter != _inPendingMap.end())
	{
		if (!iter->second->getSourceName().compare(node))
		{
			Trace_Exit(this, "containsInNodePending()");
			return true;
		}
		iter++;
	}
	Trace_Exit(this, "containsInNodePending()");
	return false;
}

bool RumConnectionsMgr::containsOutNodePending(int context) const
{
	Trace_Entry(this, "containsOutNodePending()");
	boost::recursive_mutex::scoped_lock lock(_mutex);
	return (_outPendingMap.find(context) != _outPendingMap.end());
}

bool RumConnectionsMgr::addOutPending(NodeIDImpl_SPtr node, int context,
		ConnectionsAsyncCompletionListener* asyncCompletionListener,
		ConnectionContext connCtx)
{
	String nodeName = node->getNodeName();
	ostringstream oss;
	oss << nodeName << "; context: " << context;
	Trace_Entry(this, "addOutPending()", oss.str());

	boost::recursive_mutex::scoped_lock lock(_mutex);

	if (_nodesConnectionsMap.count(nodeName) > 0)
	{
		Trace_Event(this, "addOutPending()",
				"exit badly, contains connection to", "node", nodeName);
		return false;
	}

	if (containsOutNodePending(nodeName))
	{
		Trace_Event(this, "addOutPending()",
				"exit badly, contains out pending to", "node", nodeName);
		return false;
	}

	OutPendingInfo *info = new OutPendingInfo(node, asyncCompletionListener,
			connCtx);
	_outPendingMap.insert(make_pair(context, OutPendingInfo_SPtr(info)));

	Trace_Exit(this, "addOutPending()");

	return true;
}

void RumConnectionsMgr::start()
{
	Trace_Entry(this, "start()");

	boost::recursive_mutex::scoped_lock lock(_mutex);
	int error_code;
	//RumCnContext* rumcnCtx = new RumCnContext(this, -1);

	int res = rumAddConnectionListener(&_rum, onConnectionEvent, _rumcnCtx,
			&error_code);
	if (res != RUM_SUCCESS)
	{
		char errorDesc[1024];
		rumGetErrorDescription(error_code, errorDesc, 1024);
		String error("Failed to add connection listener to RUM : ");
		error += errorDesc;
		Trace_Event(this, "start()", error);
		//TODO: throw Exception
	}

	_txMgr.start();
	_started = true;
	Trace_Exit(this, "start()");
}

String RumConnectionsMgr::toString() const
{
	Trace_Entry(this, "toString()");

	boost::recursive_mutex::scoped_lock lock(_mutex);

	stringstream tmpOut;
	tmpOut << "_nodesConnectionsMap: " << endl;
	for (map<String, rumConnection>::const_iterator iter =
			_nodesConnectionsMap.begin(); iter != _nodesConnectionsMap.end(); ++iter)
	{
		tmpOut << " target: " << iter->first << " ; connection: "
				<< iter->second.connection_id << endl;
	}

	tmpOut << "outPending Map: " << endl;
	for (OutPending_Map::const_iterator iter =
			_outPendingMap.begin(); iter != _outPendingMap.end(); ++iter)
	{
		tmpOut << "context: " << iter->first << " ; target: "
				<< iter->second->getTargetName() << endl;
	}

	tmpOut << "inPending Map: " << endl;
	for (map<rumConnectionID_t, InPendingInfo_SPtr>::const_iterator iter =
			_inPendingMap.begin(); iter != _inPendingMap.end(); ++iter)
	{
		tmpOut << "connection : " << iter->first << " ; target: "
				<< iter->second->getSourceName() << endl;
	}

	tmpOut << "QT map: " << endl;
	for (map<rumConnectionID_t, rumQueueT_SPtr>::const_iterator iter =
			_txsMap.begin(); iter != _txsMap.end(); ++iter)
	{
		tmpOut << "connection: " << iter->first << " ; qt handle: "
				<< iter->second->handle << endl;
	}

	tmpOut << "received streams  map: " << endl;
	for (StreamID2ConnID_Map::const_iterator iter =
			_receivedStreams.begin(); iter != _receivedStreams.end(); ++iter)
	{
		tmpOut << "sid: " << iter->first << "; connection: " << iter->second
				<< endl;
	}

	Trace_Exit(this, "toString()");

	return tmpOut.str();
}

bool RumConnectionsMgr::createConnection(NodeIDImpl_SPtr targetNode, ConnectionContext ctx)
{
	//return createConnection(targetNode, (&(*_incomingMsgQ)), ctx);
	return createConnection(targetNode, static_cast<ConnectionsAsyncCompletionListener*>(_incomingMsgQ.get()), ctx);
}

bool RumConnectionsMgr::createConnection(NodeIDImpl_SPtr targetNode,
		ConnectionsAsyncCompletionListener* asyncCompletionListener, ConnectionContext ctx)
{
	Trace_Entry(this, "createConnection()", "target", targetNode->getNodeName());

	bool contains = false;
	rumConnection con;
	String targetName(targetNode->getNodeName());

	if (!targetNode->getNodeName().compare(_thisMemberName))
	{
		Neighbor_SPtr localNeighbor = Neighbor_SPtr (new LocalNeighbor(targetNode, _instID,
				_incomingMsgQ, _thisBusName));
		asyncCompletionListener->onSuccess(localNeighbor, ctx);
		return true;
	}

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		if (_closed)
		{
			Trace_Debug(this, "createConnection()", "already closed");
			return false;
		}

		// If already exists in connections data structure, simply return
		if (_nodesConnectionsMap.count(targetName)>0)
		{
			contains = true;
			con = getAConnection(targetName);
		}
	}

	if (contains)
	{
		Trace_Debug(this, "createConnection()", "already exists", "node",
				targetName);
		rumQueueT_SPtr qt(new rumQueueT);
		rumStreamID_t sid;
		int errorCode;
		bool created = _txMgr.createTx(targetName, con, qt.get(), &sid, &errorCode);

		if (created)
		{
			Trace_Debug(
					this,
					"createConnection()",
					"Transmitter created, creating neighbor and sending it to caller using listener");

			Neighbor_SPtr neighbor = Neighbor_SPtr (new RumNeighbor(con, qt, sid, targetName,
					String(_thisMemberName), _instID));
			asyncCompletionListener->onSuccess(neighbor, ctx);
			{
				boost::recursive_mutex::scoped_lock lock(_mutex);
				_txsMap.insert(make_pair(con.connection_id, qt));
			}
			Trace_Exit(this, "createConnection()");

			return created;
		}
		else
		{
			Trace_Debug(this, "createConnection()",
					"Transmitter creation failed, sending async connect request instead");
			//			asyncCompletionListener->onFailure(
			//					targetNode->getNodeName(),
			//					errorCode,
			//					"RumConnectionsMgr::createConnection Failed immediately to create connection",
			//					ctx);
		}
	}

	RumCnContext_SPtr rumcnCtx;

	{ // no connection exist or connection in creation
		boost::recursive_mutex::scoped_lock lock(_mutex);

		if (containsOutNodePending(targetName))
		{
			Trace_Debug(this, "createConnection()",
					"already exists in out pending", "node", targetName);
			OutPending_Map::const_iterator outPendingIter =
					_outPendingMap.begin();
			while (outPendingIter != _outPendingMap.end())
			{
				if (!outPendingIter->second->getTargetName().compare(targetName))
				{
					outPendingIter->second->addListener(
							asyncCompletionListener, ctx);
					Trace_Exit(this, "createConnection()");
					return true;
				}
				outPendingIter++;
			}
		}

		if (containsInNodePending(targetName))
		{
			Trace_Debug(this, "createConnection()",
					"already exists in inPending", "node", targetName);

			map<rumConnectionID_t, InPendingInfo_SPtr>::const_iterator
					inPendingIter = _inPendingMap.begin();
			while (inPendingIter != _inPendingMap.end())
			{
				if (!inPendingIter->second->getSourceName().compare(targetName))
				{
					inPendingIter->second->addListener(asyncCompletionListener,
							ctx);
					Trace_Exit(this, "createConnection()");
					return true;
				}
				inPendingIter++;
			}
		}

		if (!addOutPending(targetNode, _ctx, asyncCompletionListener, ctx))
		{
			Trace_Debug(this, "createConnection()", "failed to add pending",
					"target", targetName);

			asyncCompletionListener->onFailure(
					targetName,
					1,
					"RumConnectionsMgr::createConnection Failed immediately Failed to add node to outPending",
					ctx);

			Trace_Exit(this, "createConnection()");
			return false;
		}

		rumcnCtx.reset(new RumCnContext(this, _ctx++)); //_ctx protected by mutex
	}

	//RumCnContext* rumcnCtx = new RumCnContext(this, _ctx++); //FIXME _ctx not protected by mutex

	NetworkEndpoints addresses = targetNode->getNetworkEndpoints();
	int targetPort = addresses.getPort();

	Trace_Debug(this, "createConnection()", "Looking for scope", "target",
			targetNode->toString(), "source", _myNodeId->toString());
	String targetIP = comm::endpointScopeMatch(
			_myNodeId->getNetworkEndpoints().getAddresses(),
			addresses.getAddresses());


	if (!targetIP.compare(""))
	{
		Trace_Debug(this, "createConnection()",
				"failed to locate appropriate target address", "target",
				targetName, "targetId", NodeIDImpl::stringValueOf(targetNode));
		removeOutPending(targetNode->getNodeName());
		asyncCompletionListener->onFailure(
				targetNode->getNodeName(),
				1,
				"RumConnectionsMgr::createConnection Failed immediately failed to locate appropriate target address",
				ctx);

		Trace_Exit(this, "createConnection()");
		return false;
	}
	else
	{
		Trace_Debug(this, "createConnection()",
				"established the target IP address", "target", targetName,
				"target IP", targetIP);

	}

	rumEstablishConnectionParams config;
	int rc;
	Trace_Debug(this, "createConnection()", "before rumInitStructureParameters");
	int res = rumInitStructureParameters(RUM_SID_ESTABLISHCONNPARAMS, &config,
			RUMCAPI_VERSION, &rc);
	Trace_Debug(this, "createConnection()", "after rumInitStructureParameters");
	if (res != RUM_SUCCESS)
	{
		//delete rumCtx;
		char errorDesc[2048];
		rumGetErrorDescription(rc, errorDesc, 2048);
		Trace_Event(this, "createConnection()", "failed", "target", targetName,
				"error", errorDesc);
		removeOutPending(targetNode->getNodeName());
		asyncCompletionListener->onFailure(targetName, rc,
				"Failed immediately to create connection to: " + targetName
						+ "; " + errorDesc, ctx);
		Trace_Exit(this, "createConnection()");
		return false;
	}
	strcpy(config.address, targetIP.c_str());
	config.port = targetPort;

	//config.connect_msg = (unsigned char*) _connectMessage.c_str();
	//config.connect_msg_len = _connectMessage.length();

	_connectMessage_buffer->setPosition(_connectMessage_prefix_pos);
	_connectMessage_buffer->writeString(targetName);
	config.connect_msg = reinterpret_cast<unsigned char*>(const_cast<char*>(_connectMessage_buffer->getBuffer()));
	config.connect_msg_len = static_cast<int>(_connectMessage_buffer->getDataLength());

	config.heartbeat_timeout_milli = _config.getHeartbeatTimeoutMillis();
	config.heartbeat_interval_milli = _config.getHeartbeatIntervalMillis();
	config.establish_timeout_milli
			= _config.getConnectionEstablishTimeoutMillis(); //3000;
	config.on_connection_event = onConnectionEvent;
	config.on_connection_user = rumcnCtx.get();
	//	//config.connect_method = RUM_CREATE_NEW;

	Trace_Debug(this, "createConnection()",
			"RUMTimeStamp before rumEstablishConnection");
	res = rumEstablishConnection(&_rum, &config, &rc);
	Trace_Debug(this, "createConnection()",
			"RUMTimeStamp after rumEstablishConnection");
	//------------------------------------------------------------------------------
	// In case the RUM function failed, delete context. Return the result of the
	// function.
	//------------------------------------------------------------------------------
	if (res != RUM_SUCCESS)
	{
		//delete rumcnCtx;
		removeOutPending(targetName);
		char errorDesc[2048];
		rumGetErrorDescription(rc, errorDesc, 2048);
		Trace_Event(this, "createConnection", "Failed to establish connection",
				"target", targetIP, "error", errorDesc);
		removeOutPending(targetNode->getNodeName());
		asyncCompletionListener->onFailure(targetNode->getNodeName(), rc,
				errorDesc, ctx);

		Trace_Exit(this, "createConnection()");
		return false;
	}
	else
	{
		_rumCnCtxList.push_back(rumcnCtx);
		ostringstream oss;
		oss << targetIP << ":" << targetPort;
		Trace_Debug(this, "createConnection()",
				"Succeeded to send an async connection request", "target",
				oss.str());
		Trace_Exit(this, "createConnection()");
		return true;
	}
}

Neighbor_SPtr RumConnectionsMgr::connectOnExisting(
		NodeIDImpl_SPtr targetNode)
{
	Trace_Entry(this, "connectOnExisting()", "target",
			targetNode->getNodeName());

	bool contains = false;
	rumConnection con;
	String targetName(targetNode->getNodeName());

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		if (_closed)
		{
			Trace_Debug(this, "connectOnExisting()", "already closed");
			Trace_Exit(this, "connectOnExisting()");
			return Neighbor_SPtr ();
		}

		// If already exists in connections data structure, simply return
		if (_nodesConnectionsMap.count(targetName)>0)
		{
			contains = true;
			con = getAConnection(targetName);
		}
	}
	if (contains)
	{
		Trace_Debug(this, "connectOnExisting()", "already exists", "node",
				targetName);
		rumQueueT_SPtr qt(new rumQueueT);
		rumStreamID_t sid;
		int errorCode;
		bool created = _txMgr.createTx(targetName, con, qt.get(), &sid, &errorCode);

		if (created)
		{
			Trace_Event(this, "connectOnExisting()",
					"Transmitter created, creating neighbor and sending it to caller");
			Neighbor_SPtr neighbor = Neighbor_SPtr (new RumNeighbor(con, qt, sid, targetName,
					String(_thisMemberName), _instID));
			{
				boost::recursive_mutex::scoped_lock lock(_mutex);
				_txsMap.insert(make_pair(con.connection_id, qt));
			}
			Trace_Exit(this, "connectOnExisting()");
			return neighbor;
		}
		else
		{
			Neighbor_SPtr neighbor = Neighbor_SPtr();
			Trace_Debug(this, "connectOnExisting()",
					"Transmitter creation failed, returning", "target",
					targetNode->getNodeName());
			Trace_Exit(this, "connectOnExisting()");
			return neighbor;
		}
	}

	Neighbor_SPtr neighbor = Neighbor_SPtr ();
	Trace_Debug(this, "connectOnExisting()",
			"Connection does not exist, returning", "target",
			targetNode->getNodeName());
	Trace_Exit(this, "connectOnExisting()");
	return neighbor;
}

int RumConnectionsMgr::onRumConnectionEvent(rumConnectionEvent *connection_event, int event_context)
{
	Trace_Entry(this, __FUNCTION__,
			"type", (connection_event ? spdr::stringValueOf(connection_event->type) : "NULL"),
			"context", spdr::stringValueOf(event_context));

	if (!connection_event)
	{
		Trace_Error(this, __FUNCTION__, "Error: rumConnectionEvent is NULL)");
		return RUM_ON_CONNECTION_NULL;
	}

	if (ScTraceBuffer::isDebugEnabled(_tc))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, __FUNCTION__);
		buffer->addProperty("event", toString(connection_event));
		buffer->addProperty("context", event_context);
		buffer->invoke();
	}


	int rc = RUM_ON_CONNECTION_NULL;
	//------------------------------------------------------------------------------
	// Handle event according to its type
	//------------------------------------------------------------------------------
	switch (connection_event->type)
	{

	case RUM_CONNECTION_ESTABLISH_SUCCESS:
	{
		onSuccess(connection_event->connection_info, event_context);
		rc = RUM_ON_CONNECTION_NULL;
		break;
	}

	case RUM_CONNECTION_ESTABLISH_FAILURE:
	case RUM_CONNECTION_ESTABLISH_IN_PROCESS: //TODO separate from above
	{
		onFailureOutgoing(connection_event->connection_info, event_context);
		rc = RUM_ON_CONNECTION_NULL;
		break;
	}

	//This event can be on an outgoing-pending AND incoming-pending (after RUM_NEW_CONNECTION)
	case RUM_CONNECTION_ESTABLISH_TIMEOUT:
	{
		if (event_context < 0)
			onFailureIncoming(connection_event->connection_info, event_context);
		else
			onFailureOutgoing(connection_event->connection_info, event_context);

		rc = RUM_ON_CONNECTION_NULL;
		break;
	}

	case RUM_NEW_CONNECTION:
	{
		bool accepted = onNewConnection(connection_event);
		rc = accepted ? RUM_ON_CONNECTION_ACCEPT : RUM_ON_CONNECTION_REJECT;
		break;
	}


	case RUM_CONNECTION_BROKE: //This may happen on an inPending as well
	{
		onBreak(connection_event->connection_info);
		onBreakInPending(connection_event->connection_info);
		rc = RUM_ON_CONNECTION_NULL;
		break;
	}

	case RUM_CONNECTION_HEARTBEAT_TIMEOUT:
	case RUM_CONNECTION_CLOSED:
	{
		onBreak(connection_event->connection_info);
		rc = RUM_ON_CONNECTION_NULL;
		break;
	}

	case RUM_CONNECTION_READY:
	{
		onReady(connection_event->connection_info);
		rc = RUM_ON_CONNECTION_NULL;
		break;
	}
	}

	Trace_Exit(this, __FUNCTION__, rc);
	return rc;
}

void RumConnectionsMgr::onSuccess(rumConnection & con, int event_context)
{
	Trace_Entry(this, "onSuccess()",
			"conn-id", spdr::stringValueOf(con.connection_id),
			"context",spdr::stringValueOf(event_context));

	//	bool created;
	//	int errorCode;
	//	rumQueueT qt;
	//	rumStreamID_t sid;
	OutPendingInfo_SPtr target;
	OutPending_Map::iterator iter;

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		if (_closed)
		{
			Trace_Event(this, "onSuccess()", "already closed. returning");

			return;
		}
		iter = _outPendingMap.find(event_context);
		if (iter == _outPendingMap.end())
		{
			int pRC = 666;
			//rumCloseConnection(&con, &pRC);
			ostringstream oss2;
			oss2 << "couldn't find context: " << event_context
					<< " in the outPendingMap" << "closing connection: " << pRC;
			Trace_Event(this, "onSuccess()", oss2.str());
			throw SpiderCastLogicError(oss2.str());
			Trace_Exit(this, "onSuccess()", "failed");
			return;
		}
		addConnection(iter->second->getTargetName(), con);
		target = iter->second;
		_outPendingMap.erase(iter);
	}

	ConnCompletionListenerContext_List listeners = target->getListeners();
	ConnCompletionListenerContext_List::iterator listenerIter = listeners.begin();
	if (listenerIter == listeners.end())
	{
		Trace_Event(this, "onSuccess()",
				"no need to create transmitters, no waiting listeners");
	}

	while (listenerIter != listeners.end())
	{
		ConnectionContext currCtx = (*listenerIter).second;
		ConnectionsAsyncCompletionListener* listener = (*listenerIter).first;
		Trace_Event(this, "onSuccess()",
				"Creating virgin neighbor and sending it to caller using listener");
		Neighbor_SPtr neighbor =
				Neighbor_SPtr (new RumNeighbor(con, String(_thisMemberName),
						_instID, target->getTargetName()));

		listener->onSuccess(neighbor, currCtx);
		listenerIter++;
	}
	Trace_Exit(this, "onSuccess()");
}

void RumConnectionsMgr::onFailureOutgoing(rumConnection & con, int ctx)
{
	Trace_Entry(this, "onFailureOutgoing()");

	OutPendingInfo_SPtr target;
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		ostringstream oss;
		oss << "context=" << ctx;
		Trace_Event(this, "onFailureOutgoing()", oss.str());

		if (_closed)
		{
			Trace_Event(this, "onFailureOutgoing()", "already closed. returning");

			return;
		}
		OutPending_Map::iterator iter = _outPendingMap.find(ctx);
		if (iter == _outPendingMap.end())
		{
			Trace_Event(this, "onFailureOutgoing()",
					"couldn't find context in the outPendingMap", "context",
					oss.str());

			Trace_Exit(this, "onFailureOutgoing()");
			return;
		}
		target = iter->second;
		_outPendingMap.erase(iter);

	}

	ConnCompletionListenerContext_List listeners =
			target->getListeners();
	ConnCompletionListenerContext_List::iterator
			listenerIter = listeners.begin();
	while (listenerIter != listeners.end())
	{
		// try in case a connection was established in the mean time
		Neighbor_SPtr myNeighbor = connectOnExisting(
				target->getTarget());

		ConnectionContext currCtx = (*listenerIter).second;
		ConnectionsAsyncCompletionListener* listener = (*listenerIter).first;
		if (!myNeighbor)
		{
			listener->onFailure(target->getTargetName(), -1,
					"Failed to create connection", currCtx);
		}
		else
		{
			Trace_Event(this, "onFailureOutgoing", "success after all", "node",
					target->getTargetName());
			listener->onSuccess(myNeighbor, currCtx);

			RumNeighbor *rumNeighbor =
					static_cast<RumNeighbor *> (myNeighbor.get());
			//ConcurrentSharedPtr<RumNeighbor> rumTarget = ConcurrentSharedPtr<
				//	RumNeighbor> (rumNeighbor);
			boost::recursive_mutex::scoped_lock lock(_mutex);

			_txsMap.insert(make_pair(myNeighbor->getConnectionId(),
					rumNeighbor->getTx()));
		}
		listenerIter++;
	}

	Trace_Exit(this, "onFailureOutgoing()");
}

void RumConnectionsMgr::onFailureIncoming(rumConnection & con, int ctx)
{
	Trace_Entry(this, __FUNCTION__, "connection", toString(con), "context", spdr::stringValueOf(ctx));

	InPendingInfo_SPtr inPendingInfo;

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		if (_closed)
		{
			Trace_Event(this, __FUNCTION__,  "already closed. returning");
			return;
		}

		InPending_Map::iterator pos = _inPendingMap.find(con.connection_id);
		if (pos != _inPendingMap.end())
		{
			inPendingInfo = pos->second;
			_inPendingMap.erase(pos);
		}
		else
		{
			Trace_Event(this, __FUNCTION__,  "Alert: Could not find connection-id in map, returning",
					"connection", toString(con), "context", spdr::stringValueOf(ctx));
			return;
		}
	}


	ConnCompletionListenerContext_List listeners = inPendingInfo->getListeners();
	String sourceName = inPendingInfo->getSourceName();
	if (!listeners.empty())
	{
		for (ConnCompletionListenerContext_List::iterator it = listeners.begin(); it != listeners.end(); ++it)
		{
			it->first->onFailure(sourceName,1,
					"Failed to create connection, RUM_CONNECTION_ESTABLISH_TIMEOUT on incoming pending connection from the target",
					it->second);
			Trace_Event(this, __FUNCTION__, "Notified onFailure() to outgoing ConnectionsAsyncCompletionListener",
					"target", sourceName,
					"context", spdr::stringValueOf(it->second));
		}
	}
	else
	{
		Trace_Event(this, __FUNCTION__, "No outgoing ConnectionsAsyncCompletionListener",
				"source", sourceName);
	}

	Trace_Exit(this, __FUNCTION__);
}


std::string RumConnectionsMgr::toString(const rumConnection& connection_info)
{
	std::ostringstream oss;

	oss << "connection-id=" << connection_info.connection_id
			<< "; remote-addr=" << connection_info.remote_addr
			<< " remote-connection-port=" << connection_info.remote_connection_port
			<< " remote-server-port=" << connection_info.remote_server_port
			<< "; local-addr=" << connection_info.local_addr
			<< " local-connection-port=" << connection_info.local_connection_port
			<< " local-server-port=" << connection_info.local_server_port << ";";

	return oss.str();
}


std::string RumConnectionsMgr::toString(rumConnectionEvent *connection_event)
{
	std::ostringstream oss;
	if (connection_event != NULL)
	{
		oss << " type=" << connection_event->type << "; ";
		oss << toString(connection_event->connection_info);
		oss << " connect-msg-length=" << connection_event->msg_len << ";";
	}
	else
	{
		oss << "Null";
	}

	return oss.str();
}

void  RumConnectionsMgr::deliver_warning_event(const String& errMsg, spdr::event::ErrorCode errCode, rumConnectionID_t conn_id)
{
	try
	{	// create the warning event for topology
		SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
		scMsg->setSender(_nodeIdCache.getOrCreate(String("Not Available")));
		boost::shared_ptr<CommEventInfo> eventInfo = boost::shared_ptr<CommEventInfo>(
				new CommEventInfo(CommEventInfo::Warning_Connection_Refused, conn_id));
		eventInfo->setErrCode(errCode);
		eventInfo->setErrMsg(errMsg);
		scMsg->setCommEventInfo(eventInfo);
		_incomingMsgQ->onMessage(scMsg);
	}
	catch (SpiderCastRuntimeError& e)
	{
		Trace_Warning(this, "deliver_warning_event()", "SpiderCastRuntimeError, ignored", "what",e.what());
	}
}

//SplitBrain
void RumConnectionsMgr::deliver_dup_node_suspicion_event(const String& errMsg, spdr::event::ErrorCode errCode,
		rumConnectionID_t conn_id, const String& node, int64_t incomingIncNum)
{
	try
	{	// create the suspicion event for topology
		SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
		scMsg->setSender(_nodeIdCache.getOrCreate(node));
		boost::shared_ptr<CommEventInfo> eventInfo = boost::shared_ptr<CommEventInfo>(
				new CommEventInfo(CommEventInfo::Suspicion_Duplicate_Remote_Node, conn_id));
		eventInfo->setErrCode(errCode);
		eventInfo->setErrMsg(errMsg);
		eventInfo->setIncNum(incomingIncNum);
		scMsg->setCommEventInfo(eventInfo);
		_incomingMsgQ->onMessage(scMsg);
	}
	catch (SpiderCastRuntimeError& e)
	{
		Trace_Warning(this, "deliver_dup_node_suspicion_event()", "SpiderCastRuntimeError, ignored", "what",e.what());
	}
}

// at the receiver side when a connection first comes up through RUM
bool RumConnectionsMgr::onNewConnection(rumConnectionEvent *connection_event)
{
	Trace_Entry(this, "onNewConnection()");
	boost::recursive_mutex::scoped_lock lock(_mutex);

	const rumConnection & con = connection_event->connection_info;
	const char* connectionMsg = connection_event->connect_msg;
	int connectionMsgLength = connection_event->msg_len;

	if (_closed)
	{
		Trace_Event(this, "onNewConnection()", "already closed. returning");
		Trace_Exit(this, "onNewConnection()",false);
		return false;
	}

	if (connectionMsg == NULL || static_cast<std::size_t>(connectionMsgLength) < _connectMessage_min_size)
	{
		String errMsg("Connection refused: cannot parse handshake message. ");
		errMsg += toString(connection_event);
		Trace_Event(this, "onNewConnection()", errMsg, "why","length shorter then minimal size");
		deliver_warning_event(errMsg, event::Connection_Refused_Handshake_Parse_Error, con.connection_id);
		Trace_Exit(this, "onNewConnection()",false);
		return false;
	}

	uint16_t suppVersion = 0;
	uint16_t usedVersion = 0;
	String busName;
	String memberName;
	int64_t memberIncNum = 0;
	String targetName;

	try
	{
		ByteBuffer_SPtr bb = ByteBuffer::createReadOnlyByteBuffer(connectionMsg, connectionMsgLength, false);
		suppVersion = static_cast<uint16_t>( bb->readShort() );
		usedVersion = static_cast<uint16_t>( bb->readShort() );
		//In the base version, this is all we need to check
		if (usedVersion != config::SPIDERCAST_VERSION)
		{
			std::ostringstream errMsg;
			errMsg << "Connection refused: remote SpiderCast used-version "<< usedVersion << " is incompatible with local version " << config::SPIDERCAST_VERSION << ". ";
			errMsg << ", " << toString(connection_event);
			Trace_Event(this, "onNewConnection()", errMsg.str(), "remote supported-version",suppVersion);
			deliver_warning_event(errMsg.str(), event::Connection_Refused_Incompatible_Version, con.connection_id);
			Trace_Exit(this, "onNewConnection()",false);
			return false;
		}

		busName = bb->readString();
		memberName = bb->readString();
		memberIncNum = bb->readLong();
		targetName = bb->readString();

	}
	catch (SpiderCastRuntimeError& re)
	{
		String errMsg("Connection refused: cannot parse handshake message. ");
		errMsg += toString(connection_event);
		Trace_Event(this, "onNewConnection()",errMsg, "what", String(re.what()));
		deliver_warning_event(errMsg, event::Connection_Refused_Handshake_Parse_Error, con.connection_id);
		Trace_Exit(this, "onNewConnection()",false);
		return false;
	}
	catch (std::exception& e)
	{
		String errMsg("Connection refused: cannot parse handshake message. ");
		errMsg += toString(connection_event);
		Trace_Event(this, "onNewConnection()",errMsg, "what", String(e.what()), "typeid", typeid(e).name());
		deliver_warning_event(errMsg, event::Connection_Refused_Handshake_Parse_Error, con.connection_id);
		Trace_Exit(this, "onNewConnection()",false);
		return false;
	}
	catch (...)
	{
		String errMsg("Connection refused: cannot parse handshake message. ");
		errMsg += toString(connection_event);
		Trace_Event(this, "onNewConnection()",errMsg, "what", String("untyped exception (...)"));
		deliver_warning_event(errMsg, event::Connection_Refused_Handshake_Parse_Error, con.connection_id);
		Trace_Exit(this, "onNewConnection()",false);
		return false;
	}

	BusName_SPtr busNameObj = BusName_SPtr(new BusName(busName.c_str()));

	Trace_Event(this, "onNewConnection()", "details",
			"bus", busName, "member", memberName,
			"incarnation", boost::lexical_cast<std::string>(memberIncNum));

	if (busNameObj->comparePrefix(*_thisBusName) == 0)
	{
		std::ostringstream errMsg;
		errMsg << "Connection refused: remote bus-name='" << busName
				<<"', received from node name='" << memberName
				<< "', incompatible with local bus-name='" << _thisBusName->toOrgString() << "'. ";
		errMsg << toString(connection_event);
		Trace_Event(this, "onNewConnection()", errMsg.str());
		deliver_warning_event(errMsg.str(), event::Connection_Refused_Incompatible_BusName, con.connection_id);
		Trace_Exit(this, "onNewConnection()",false);
		return false;
	}


	if (!boost::starts_with(targetName, NodeID::NAME_ANY) && (targetName != String(_thisMemberName)))
	{
		std::ostringstream errMsg;
		errMsg << "Connection refused: remote target name='" << targetName
				<< "', received from node name='" << memberName
				<<"', different than local name='" << _thisMemberName << "'. ";
		errMsg << toString(connection_event);
		Trace_Event(this, "onNewConnection()", errMsg.str());
		deliver_warning_event(errMsg.str(), event::Connection_Refused_Wrong_TargetName, con.connection_id);
		Trace_Exit(this, "onNewConnection()",false);
		return false;
	}

	String sourceName;
	if (boost::starts_with(targetName, NodeID::NAME_ANY))
	{
		Trace_Debug(this, "onNewConnection()", "received a nameless discovery connection", "source", memberName);
		sourceName = NodeID::RSRV_NAME_DISCOVERY + "@" + memberName;
	}
	else
	{
		sourceName = memberName;
	}

	if (_nodesConnectionsMap.count(sourceName)>0)
	{
		std::ostringstream errMsg;
		errMsg << "Connection refused: source node name='" << sourceName << "' already connected. ";
		errMsg << toString(connection_event);
		Trace_Event(this, "onNewConnection()", errMsg.str());

		//Figure out whether to deliver by adding the existing incarnation number to the Node2Connection map? (currently, no).
		//SplitBrain
		deliver_dup_node_suspicion_event(errMsg.str(), event::Duplicate_Remote_Node_Suspicion,
				con.connection_id, sourceName, memberIncNum);

		Trace_Exit(this, "onNewConnection()",false);
		return false;
	}

	if (containsOutNodePending(sourceName))
	{
		//When this node and the other node try to connect at the same time
		//Break symmetry, the node with the name ordered first wins
		bool res = String(_thisMemberName).compare(sourceName) > 0;
		Trace_Event(this, "onNewConnection()", "after containsOutNodePending",
				"target", sourceName, "result", spdr::stringValueOf(res));

		if (!res)
		{
			Trace_Exit(this, "onNewConnection()", res);
			return res;
		}
	}

	if (containsInNodePending(sourceName))
	{
		Trace_Event(this, "onNewConnection", "in pending already exists",
				"member", sourceName);

		//Figure out if the following statement is needed
		Trace_Exit(this, "onNewConnection()", false);
		return false;
	}

	Trace_Event(this, "onNewConnection()", "accepting the connection request");

	_inPendingMap.insert(make_pair(con.connection_id, InPendingInfo_SPtr(
			new InPendingInfo(sourceName))));

	_connectionBusName.insert(make_pair(con.connection_id, busNameObj));

	Trace_Exit(this, "onNewConnection()", true);
	return true;
}

void RumConnectionsMgr::onBreak(const rumConnection & con)
{
	Trace_Entry(this, "onBreak()", "connection", spdr::stringValueOf(con.connection_id));

	boost::recursive_mutex::scoped_lock lock(_mutex);

	if (_closed)
	{
		Trace_Event(this, "onBreak()", "already closed. returning",
				"connection", spdr::stringValueOf(con.connection_id));
		Trace_Exit(this, "onBreak()");
		return;
	}

	String sourceName;
	bool found = false;
	map<String, rumConnection>::iterator iter = _nodesConnectionsMap.begin();
	while (iter != _nodesConnectionsMap.end())
	{
		if (iter->second.connection_id == con.connection_id)
		{
			found = true;
			sourceName = iter->first;
			break;
		}
		else
		{
			iter++;
		}
	}

	BusName_SPtr busName;
	ConnID2BusName_Map::iterator busNameIter;
	busNameIter = _connectionBusName.find(con.connection_id);
	if (busNameIter != _connectionBusName.end())
	{
		busName = busNameIter->second;
	}
	else
	{
		Trace_Event(this, "onBreak()", "Couldn't find corresponding BusName",
				"connection", "connection", spdr::stringValueOf(con.connection_id));
	}

	closeConnection(con, false);

	if (found)
	{
		try
		{
			// create the  connection broken event for topology
			SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
			(*scMsg).setSender(_nodeIdCache.getOrCreate(sourceName));
			(*scMsg).setBusName(busName);
			boost::shared_ptr<CommEventInfo> eventInfo = boost::shared_ptr<
					CommEventInfo>(
							new CommEventInfo(CommEventInfo::On_Break,
									con.connection_id));
			(*scMsg).setCommEventInfo(eventInfo);
			_incomingMsgQ->onMessage(scMsg);
		} catch (SpiderCastRuntimeError& e)
		{
			Trace_Warning(this, "onBreak()", "SpiderCastRuntimeError, ignored",
					"what", e.what());
		}
	}
	else
	{
		Trace_Event(this, "onBreak()",
				"Couldn't find corresponding connection", "connection",
				"connection", spdr::stringValueOf(con.connection_id));
	}
	Trace_Exit(this, "onBreak()");
}


void RumConnectionsMgr::onBreakInPending(const rumConnection & con)
{
	Trace_Entry(this, __FUNCTION__, "connection", spdr::stringValueOf(con.connection_id));

	InPendingInfo_SPtr inPendingInfo;

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		if (_closed)
		{
			Trace_Event(this, __FUNCTION__,  "already closed. returning");
			return;
		}

		InPending_Map::iterator pos = _inPendingMap.find(con.connection_id);
		if (pos != _inPendingMap.end())
		{
			inPendingInfo = pos->second;
			_inPendingMap.erase(pos);
		}
		else
		{
			Trace_Debug(this, __FUNCTION__, "Could not find connection-id in map, returning",
					"connection", toString(con));
			return;
		}
	}

	ConnCompletionListenerContext_List listeners = inPendingInfo->getListeners();
	String sourceName = inPendingInfo->getSourceName();
	if (!listeners.empty())
	{
		for (ConnCompletionListenerContext_List::iterator it = listeners.begin(); it != listeners.end(); ++it)
		{
			it->first->onFailure(sourceName,1,
					"Failed to create connection, RUM_CONNECTION_BROKE on incoming pending connection from the target",
					it->second);
			Trace_Event(this, __FUNCTION__, "Notified onFailure() to outgoing ConnectionsAsyncCompletionListener",
					"target", sourceName,
					"context", spdr::stringValueOf(it->second));
		}
	}
	else
	{
		Trace_Event(this, __FUNCTION__, "No outgoing ConnectionsAsyncCompletionListener",
				"source", sourceName);
	}

	Trace_Exit(this, __FUNCTION__);
}

// called on the receiver side when the connection is ready to work with
void RumConnectionsMgr::onReady(rumConnection & con)
{
	ostringstream oss;
	oss << con.connection_id;
	Trace_Entry(this, "onReady()", oss.str());

	//	bool created;
	//	int errorCode;
	//	rumQueueT qt;
	//	rumStreamID_t sid;
	InPendingInfo_SPtr source;
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		if (_closed)
		{
			Trace_Event(this, "onReady()", "already closed. returning");
			Trace_Exit(this, "onReady()");
			return;
		}

		map<rumConnectionID_t, InPendingInfo_SPtr>::iterator iter =
				_inPendingMap.find(con.connection_id);
		if (iter == _inPendingMap.end())
		{
			int pRC = 666;
			//rumCloseConnection(&con, &pRC);
			ostringstream oss2;
			oss2 << "connection: " << con.connection_id
					<< " in the inPendingMap" << "closing connection: " << pRC;
			Trace_Event(this, "onReady()", oss2.str());
			throw SpiderCastLogicError(oss2.str());
			Trace_Exit(this, "onReady()", "failed");
			return;
		}
		source = iter->second;

		addConnection(source->getSourceName(), con);

		_inPendingMap.erase(iter);
	}

	Trace_Event(this, "onReady()", "Sending NewSource event");
	try
	{
		// create the new connection event for topology
		SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
		Trace_Event(this, "onReady()", "Before getOrCreate");
		(*scMsg).setSender(_nodeIdCache.getOrCreate(source->getSourceName()));
		Trace_Event(this, "onReady()", "After getOrCreate");
		boost::shared_ptr<CommEventInfo> eventInfo = boost::shared_ptr<
				CommEventInfo>(new CommEventInfo(CommEventInfo::New_Source,
				con.connection_id));
		(*scMsg).setCommEventInfo(eventInfo);
		_incomingMsgQ->onMessage(scMsg);
	} catch (SpiderCastRuntimeError& e)
	{
		Trace_Warning(this, "onReady()", "SpiderCastRuntimeError, ignored", "what",e.what());
	}
	Trace_Event(this, "onReady()", "NewSource event sent");

	//Creating streams and neighbors for all listeners and event about this for topology
	ConnCompletionListenerContext_List listeners =
			source->getListeners();
	ConnCompletionListenerContext_List::iterator
			listenerIter = listeners.begin();
	if (listenerIter == listeners.end())
	{
		Trace_Event(this, "onReady()",
				"no need to create transmitters, no waiting listeners");
	}
	while (listenerIter != listeners.end())
	{

		ConnectionContext currCtx = (*listenerIter).second;
		ConnectionsAsyncCompletionListener* listener = (*listenerIter).first;

		Trace_Event(this, "onReady()",
				"Creating virgin neighbor and sending it to caller using listener");
		Neighbor_SPtr neighbor =
				Neighbor_SPtr (new RumNeighbor(con, String(_thisMemberName),
						_instID, source->getSourceName()));
		listener->onSuccess(neighbor, currCtx);
		listenerIter++;
	}

	Trace_Exit(this, "onReady()");
}

void RumConnectionsMgr::closeAllConnections()
{
	Trace_Entry(this, "closeAllConnections()");

	multimap<String, rumConnection>::iterator iter;

	for (iter = _nodesConnectionsMap.begin(); iter
			!= _nodesConnectionsMap.end(); iter++)
	{
		int rc;
		ostringstream oss;
		oss << iter->second.connection_id;
		Trace_Event(this, "closeAllConnections()", "closing", "connection",
				oss.str());

		Trace_Event(this, "closeAllConnections()",
				"RUMTimeStamp before rumCloseConnection");
		int ret = rumCloseConnection(&(iter->second), &rc);
		Trace_Event(this, "closeAllConnections()",
				"RUMTimeStamp after rumCloseConnection");
		if (ret != RUM_SUCCESS)
		{
			char errorDesc[1024];
			rumGetErrorDescription(rc, errorDesc, 1024);
			String error("Failed to close Connection : ");
			error += errorDesc;
			Trace_Event(this, "closeAllConnections()", error);
		}
		else
		{
			Trace_Event(this, "closeConnection()", "closed", "connection",
					oss.str());
		}
	}
}

bool RumConnectionsMgr::closeConnection(rumConnection con, bool flag)
{
	ostringstream oss;
	oss << ": " << con.connection_id << " flag: " << flag;
	Trace_Entry(this, "closeConnection()", oss.str());

	vector<rumQueueT_SPtr> qtsToRemove;
	bool removed = false;
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		if (flag)
		{
			multimap<rumConnectionID_t, rumQueueT_SPtr>::iterator iter2;
			iter2 = _txsMap.find(con.connection_id);
			while (iter2 != _txsMap.end())
			{
				if (flag)
				{
					qtsToRemove.push_back(iter2->second);
				}

				_txsMap.erase(iter2);
				iter2 = _txsMap.find(con.connection_id);
			}
		}

		map<String, rumConnection>::iterator iter =
				_nodesConnectionsMap.begin();
		while (iter != _nodesConnectionsMap.end())
		{
			if (iter->second.connection_id == con.connection_id)
			{
				Trace_Debug(this, "closeConnection()", "removed source", "node", iter->first);
				removed = true;
				_nodesConnectionsMap.erase(iter);
				break;
			}
			else
			{
				iter++;
			}
		}

		_inPendingMap.erase(con.connection_id);
		_connectionBusName.erase(con.connection_id);
	}

	if (flag)
	{
		int rc;
		for (unsigned int counter = 0; counter < qtsToRemove.size(); counter++)
		{
			Trace_Event(this, "closeConnection()",
					"RUMTimeStamp before rumTCloseQueue");
			int res = rumTCloseQueue(qtsToRemove[counter].get(), 0, &rc);
			Trace_Event(this, "closeConnection()",
					"RUMTimeStamp after rumTCloseQueue");
			if (res != RUM_SUCCESS)
			{
				char errorDesc[1024];
				rumGetErrorDescription(rc, errorDesc, 1024);
				String error("Failed to close Queue : ");
				error += errorDesc;
				Trace_Event(this, "closeConnection()", error);
			}
			else
			{
				Trace_Event(this, "closeConnection()", "closed queueT");
			}
		}
		Trace_Event(this, "closeConnection()",
				"RUMTimeStamp before rumCloseConnection");
		int res = rumCloseConnection(&con, &rc);
		Trace_Event(this, "closeConnection()",
				"RUMTimeStamp after rumCloseConnection");
		if (res != RUM_SUCCESS)
		{
			char errorDesc[1024];
			rumGetErrorDescription(rc, errorDesc, 1024);
			String error("Failed to close Connection : ");
			error += errorDesc;
			Trace_Event(this, "closeConnection", error);
		}
		else
		{
			Trace_Event(this, "closeConnection()", "closed connection");
		}
	}

	Trace_Exit(this, "closeConnection()");

	return removed;
}

bool RumConnectionsMgr::closeStream(Neighbor_SPtr target)
{
	Trace_Entry(this, "closeStream()", "target", spdr::toString(target));

	rumQueueT_SPtr qtToRemove;
	bool removedQT = false;
	bool removeConnection = false;

	RumNeighbor_SPtr rumTarget = boost::static_pointer_cast<RumNeighbor, Neighbor>(target);

	if (target->isVirgin())
	{
		Trace_Event(this, "closeStream()", "Target RumNeighbor is Virgin, no transmitter", "target", spdr::toString(target));
		rumConnection con = rumTarget->getConnection();

		if (_txsMap.count(con.connection_id) == 0) //no more rumQueueT's on the connection
		{
			removeConnection = true;
			for (StreamID2ConnID_Map::iterator recieversIter = _receivedStreams.begin();
					recieversIter != _receivedStreams.end(); ++recieversIter)
			{
				if (recieversIter->second == con.connection_id) //there are still incoming streams on the connection
				{
					removeConnection = false;
					break;
				}
			}
		}

		if (removeConnection)
		{
			Trace_Debug(this, "closeStream()","Virgin neighbor: connection has no more Tx/Rcv, going to close",
					"conn-id", spdr::stringValueOf(con.connection_id));
			closeConnection(con, true);
		}
	}
	else
	{
		rumConnection con = rumTarget->getConnection();
		rumQueueT_SPtr transmitter = rumTarget->getTx();
		streamId_t reciever = rumTarget->getReceiverId();

		{
			boost::recursive_mutex::scoped_lock lock(_mutex);

			multimap<rumConnectionID_t, rumQueueT_SPtr>::iterator iter2;
			pair< multimap<rumConnectionID_t, rumQueueT_SPtr>::iterator,
			multimap<rumConnectionID_t, rumQueueT_SPtr>::iterator > range;

			range = _txsMap.equal_range(con.connection_id);
			for (iter2 = range.first; iter2 != range.second; ++iter2)
			{
				rumQueueT_SPtr qt = iter2->second;
				if (transmitter)
				{
					if (qt->handle == transmitter->handle)
					{
						qtToRemove = qt;
						_txsMap.erase(iter2);
						removedQT = true;
						break;
					}
				}
			}

			_receivedStreams.erase(reciever);

			if (_txsMap.count(con.connection_id) == 0) //no more rumQueueT's on the connection
			{
				removeConnection = true;
				for (StreamID2ConnID_Map::iterator recieversIter = _receivedStreams.begin();
						recieversIter != _receivedStreams.end(); ++recieversIter)
				{
					if (recieversIter->second == con.connection_id) //there are still incoming streams on the connection
					{
						removeConnection = false;
						Trace_Debug(this, "closeStream()", "Additional receiver streams exist",
								"removeConnecition", spdr::stringValueOf(removeConnection));
						break;
					}
				}
			}
			else
			{
				Trace_Debug(this, "closeStream()", "Additional transmitter streams exist",
						"removeConnecition", spdr::stringValueOf(removeConnection));
			}
		}

		if (removedQT)
		{
			int rc;
			Trace_Event(this, "closeStream()", "before rumTCloseQueue");
			int res = rumTCloseQueue(qtToRemove.get(), 0, &rc);
			Trace_Event(this, "closeStream()",
					"after rumTCloseQueue");
			if (res != RUM_SUCCESS)
			{
				char errorDesc[1024];
				rumGetErrorDescription(rc, errorDesc, 1024);
				String error("Failed to close Queue : ");
				error += errorDesc;
				Trace_Event(this, "closeStream()", error);
			}
			else
			{
				Trace_Event(this, "closeStream()", "closed queueT");
			}
		}

		if (removeConnection)
		{
			closeConnection(con, true);
		}
	}

	Trace_Exit(this, "closeStream()");

	return removedQT;
}

//int RumConnectionsMgr::on_connection_event(
//		rumConnectionEvent *connection_event, void *user)
//{
//	int type = connection_event->type;
//	//------------------------------------------------------------------------------
//	// Parse context
//	//------------------------------------------------------------------------------
//	RumCnContext* rumCnCtx = (RumCnContext*) user;
//	RumConnectionsMgr* cnMgr = rumCnCtx->getConMgr();
//	int event_context = rumCnCtx->getContext();
//
//	int rc = RUM_ON_CONNECTION_NULL;
//	//------------------------------------------------------------------------------
//	// Handle event according to its type
//	//------------------------------------------------------------------------------
//	switch (type)
//	{
//
//	case RUM_CONNECTION_ESTABLISH_SUCCESS:
//	{
//		cnMgr->onSuccess(connection_event->connection_info, event_context);
//		rc = RUM_ON_CONNECTION_NULL;
//		break;
//	}
//
//	case RUM_CONNECTION_ESTABLISH_FAILURE:
//	case RUM_CONNECTION_ESTABLISH_TIMEOUT:
//	case RUM_CONNECTION_ESTABLISH_IN_PROCESS:
//	{
//		cnMgr->onFailure(event_context);
//		rc = RUM_ON_CONNECTION_NULL;
//		break;
//	}
//	case RUM_NEW_CONNECTION:
//	{
//		bool accepted = cnMgr->onNewConnection(connection_event);
//		rc = accepted ? RUM_ON_CONNECTION_ACCEPT : RUM_ON_CONNECTION_REJECT;
//		break;
//	}
//
//	case RUM_CONNECTION_HEARTBEAT_TIMEOUT:
//	case RUM_CONNECTION_BROKE:
//	case RUM_CONNECTION_CLOSED:
//	{
//		//TODO: pass specific reason
//		cnMgr->onBreak(connection_event->connection_info);
//		rc = RUM_ON_CONNECTION_NULL;
//		break;
//	}
//
//	case RUM_CONNECTION_READY:
//	{
//		cnMgr->onReady(connection_event->connection_info);
//		rc = RUM_ON_CONNECTION_NULL;
//		break;
//	}
//	}
//	return rc;
//}

std::string RumConnectionsMgr::onNewStreamReceived(rumStreamID_t sid, String nodeName,
		rumConnectionID_t con)
{
	ostringstream oss;
	oss << " sid: " << sid << ", conn: " << con << ", name: " << nodeName;
	Trace_Entry(this, "onNewStreamReceived()", oss.str());

	boost::recursive_mutex::scoped_lock lock(_mutex);

	std::string sender_local_name;

	if (_closed)
	{
		Trace_Exit(this, "onNewStreamReceived()", "already closed. returning");
		return sender_local_name;
	}

	if (_receivedStreams.find(sid) != _receivedStreams.end())
	{
		Trace_Exit(this, "onNewStreamReceived()",
				"received the same sid twice;");
		return sender_local_name;
	}
	_receivedStreams.insert(make_pair(sid, con));


	for (NodesConnectionsMap::const_iterator it =  _nodesConnectionsMap.begin(); it != _nodesConnectionsMap.end(); ++it)
	{
		if (it->second.connection_id == con)
		{
			sender_local_name = it->first;
			break;
		}
	}

	if (sender_local_name != nodeName)
	{
		Trace_Debug(this, "onNewStreamReceived()", "sender local name != remote name", "local", sender_local_name, "remote", nodeName);
	}

	Trace_Dump(this, "onNewStreamReceived()", toString());

	return sender_local_name;
}

void RumConnectionsMgr::onStreamBreak(rumStreamID_t sid)
{
	boost::recursive_mutex::scoped_lock lock(_mutex);

	if (_closed)
	{
		Trace_Event(this, "onStreamBreak()", "already closed. returning" , "sid", boost::lexical_cast<string>(sid));
		return;
	}

	if (_receivedStreams.erase(sid) > 0)
	{
		Trace_Debug(this, "onStreamBreak()", "removed", "sid", boost::lexical_cast<string>(sid));
	}
	else
	{
		Trace_Event(this, "onStreamBreak()", "did not find the stream", "sid", boost::lexical_cast<string>(sid));
	}
}

void RumConnectionsMgr::onStreamNotPresent(rumStreamID_t sid)
{
	Trace_Entry(this, "onStreamNotPresent()", "sid", toHexString(sid));

	bool found = false;
	rumQueueT_SPtr qt;
	rumConnectionID_t cid = 0;
	bool removeConnection = false;

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		if (_closed)
		{
			Trace_Event(this, "onStreamNotPresent()", "already closed, returning");
			return;
		}

		for (multimap<rumConnectionID_t, rumQueueT_SPtr>::iterator it = _txsMap.begin();
				it != _txsMap.end(); ++it)
		{
			int rc;
			rumStreamID_t sid_tmp;
			if (RUM_SUCCESS != rumTGetStreamID(it->second.get(), &sid_tmp, &rc))
			{
				char errorDesc[1024];
				rumGetErrorDescription(rc, errorDesc, 1024);
				String error("Unable to get rumStreamID_t from rumQueueT, ignoring: ");
				error += errorDesc;
				Trace_Event(this, "onStreamNotPresent()", error);
			}
			else if (sid_tmp == sid)
			{
				Trace_Event(this, "onStreamNotPresent()", "found, removing from transmitters map", //TODO check the code
						"sid", toHexString(sid),
						"cid", toHexString(it->first));
				qt = it->second;
				cid = it->first;
				_txsMap.erase(it);
				found = true;
				break;
			}
		}

		if (found && _txsMap.count(cid) == 0) //no more rumQueueT's on the connection
		{
			Trace_Debug(this, __FUNCTION__, "No more transmitters on connection", "cid", toHexString(cid));
			removeConnection = true;
			for (StreamID2ConnID_Map::iterator recieversIter = _receivedStreams.begin();
					recieversIter != _receivedStreams.end(); ++recieversIter)
			{
				if (recieversIter->second == cid) //there are still incoming streams on the connection
				{
					Trace_Debug(this, __FUNCTION__, "Found receiver on connection",
							"cid", toHexString(cid), "sid", toHexString(recieversIter->first));
					removeConnection = false;
					break;
				}
			}
		}
	}

	if (found)
	{
		int rc;
		int res = rumTCloseQueue(qt.get(), 0, &rc);
		if (res != RUM_SUCCESS)
		{
			char errorDesc[1024];
			rumGetErrorDescription(rc, errorDesc, 1024);
			String error("Unable to close Queue : ");
			error += errorDesc;
			Trace_Event(this, "onStreamNotPresent()", error);
		}
		else
		{
			Trace_Event(this, "onStreamNotPresent()", "closed rumQueueT", "sid", toHexString(sid));
		}

		//TODO
		//Avoided the following in order to preserve stability. May result in an orphan connection in rare cases.
		//Close connection if there are no additional transmitters or receivers on it
		if (removeConnection)
		{
			Trace_Event(this, "onStreamNotPresent()", "connection has no remaining streams, will not close",
					"cid",spdr::toHexString(cid));
		}
	}
	else
	{
		Trace_Event(this, "onStreamNotPresent()", "stream not found", "sid", toHexString(sid));
	}

	Trace_Exit(this, "onStreamNotPresent()", toString());
}

BusName_SPtr RumConnectionsMgr::lookupBusName(
		Neighbor_SPtr neighbor)
{
	Trace_Entry(this, "lookupBusName()", "neighbor", neighbor->toString());

	boost::recursive_mutex::scoped_lock lock(_mutex);
	if (neighbor)
	{
		ConnID2BusName_Map::iterator
				busNameIter = _connectionBusName.find(
						neighbor->getConnectionId());
		if (busNameIter != _connectionBusName.end())
		{
			Trace_Exit(this, "lookupBusName()", busNameIter->second->toString());
			return busNameIter->second;
		}
		else
		{
			Trace_Exit(this, "lookupBusName()", "Nothing found");
			return BusName_SPtr ();
		}
	}
	else
	{
		Trace_Exit(this, "lookupBusName()", "Invalid neighbor, nothing found");
		return BusName_SPtr ();
	}
}

RumCnContext * RumConnectionsMgr::getRumContext()
{
	return _rumcnCtx;

}
}
