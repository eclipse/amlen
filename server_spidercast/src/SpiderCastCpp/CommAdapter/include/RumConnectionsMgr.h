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
 * RumConnectionMgr.h
 *
 *  Created on: Feb 15, 2010
 *      Author:
 *
 * Version     : $Revision: 1.14 $

 *-----------------------------------------------------------------
 * $Log: RumConnectionsMgr.h,v $
 * Revision 1.14  2016/12/05 14:27:50 
 * Fix mem leak in Comm; Improve trace for stats.
 *
 * Revision 1.13  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.12.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.12  2016/01/07 14:41:18 
 * bug fix: closeStream with virgin RumNeighbor
 *
 * Revision 1.11  2015/12/02 09:31:47 
 * code cleanup
 *
 * Revision 1.10  2015/11/26 09:52:11 
 * Bug fix: handle RUM_CONNECTION_ESTABLISH_TIMEOUT on incoming pending connections
 *
 * Revision 1.9  2015/11/24 13:19:50 
 * improve trace
 *
 * Revision 1.8  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.7  2015/11/12 10:28:16 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.6  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.5  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.4  2015/07/29 09:26:11 
 * split brain
 *
 * Revision 1.3  2015/06/25 10:26:45 
 * readiness for version upgrade
 *
 * Revision 1.2  2015/05/05 12:48:25 
 * Safe connect
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.34  2015/01/26 12:38:14 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.33  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.32  2012/06/27 12:38:57 
 * Seperate generic COMM from RUM
 *
 * Revision 1.31  2011/07/10 09:23:37 
 * Create an RUM Tx on a seperate thread (not thr RUM thread)
 *
 * Revision 1.30  2011/05/22 12:52:14 
 * Introduce a local neighbor
 *
 * Revision 1.29  2011/03/16 13:37:36 
 * Add connectivity event; fix a couple of bugs in topology
 *
 * Revision 1.28  2011/02/27 12:20:20 
 * Refactoring, header file organization
 *
 * Revision 1.27  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.26  2011/01/10 12:25:35 
 * Remove context from connectOnExisting() method
 *
 * Revision 1.25  2010/12/14 10:47:40 
 * Comm changes - receiver stream id in each message and neighbor, disconnect reject stream, if known and counts number tx and rcv before closing connection.
 *
 * Revision 1.24  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.23.2.1  2010/10/21 16:10:43 
 * Massive comm changes
 *
 * Revision 1.23  2010/09/18 17:04:55 
 * Hierarhical bus name support
 *
 * Revision 1.22  2010/08/23 09:19:17 
 * Pass in the config object
 *
 * Revision 1.21  2010/08/17 13:28:07 
 * Remove the RUM config param, as it should be done seperately for each call
 *
 * Revision 1.20  2010/08/05 09:25:41 
 * Init the RUM connection config structure only once
 *
 * Revision 1.19  2010/08/02 17:12:02 
 * Pass in myNodeId in order to sekect an appropriate arget IP address
 *
 * Revision 1.18  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.17  2010/06/30 09:11:58 
 * Handle termination better
 *
 * Revision 1.16  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.15  2010/06/27 13:59:40 
 * Rename print to toString and have it return a string rather than print
 *
 * Revision 1.14  2010/06/27 12:02:52 
 * Move all implementations from header files
 *
 * Revision 1.13  2010/06/23 14:34:19 
 * Close all connections upon destruction
 *
 * Revision 1.12  2010/06/23 14:17:02 
 * No functional chnage
 *
 * Revision 1.11  2010/06/20 15:59:12 
 * Change neighbor's map into queueT map, close qt, upon connection closure
 *
 * Revision 1.10  2010/06/20 14:40:46 
 * Handle termination better. Add grace parameter
 *
 * Revision 1.9  2010/06/14 15:51:00 
 * No functional change
 *
 * Revision 1.8  2010/06/02 08:59:29 
 * *** empty log message ***
 *
 * Revision 1.7  2010/05/27 09:28:01 
 * Add disconnect() method
 *
 * Revision 1.6  2010/05/20 09:12:53 
 * change createConnection to take a nodeIdImpl
 *
 * Revision 1.5  2010/05/10 08:29:33 
 * Use a new interface: ConnectionsAsyncCompletionListener rather than an asyncCompletionListener template
 *
 * Revision 1.4  2010/05/06 09:34:00 
 * Declare mutexes as mutable so they can be grabbed in const methods
 *
 * Revision 1.3  2010/04/14 09:16:21 
 * Add some print statemnets - no funcional changes
 *
 * Revision 1.2  2010/04/12 09:20:00 
 * Add a mutex for roper synchronization + misc. non-funcational changes
 *
 * Revision 1.1  2010/03/07 10:44:44 
 * Intial Version of the Comm Adapter
 *
 *
 */

#ifndef RUMCONNECTIONMGR_H_
#define RUMCONNECTIONMGR_H_

#ifdef __MINGW__
#include <stdint.h>
#include <_mingw.h>
#endif

#include <boost/shared_ptr.hpp>
#include "rumCapi.h"

#include "CommRumTxMgr.h"
#include "BusName.h"
#include "LocalNeighbor.h"
#include "RumNeighbor.h"
#include "CommRumReceiverListener.h"
#include "CommRumTxMgrListener.h"
#include "ConnectionsAsyncCompletionListener.h"
#include "NodeIDImpl.h"
#include "NodeIDCache.h"
#include "IncomingMsgQ.h"
#include "SpiderCastConfigImpl.h"
#include "CommAdapterDefinitions.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

#include <map>
#include <set>
#include <list>

#include <boost/thread/recursive_mutex.hpp>

using namespace std;

namespace spdr
{

class OutPendingInfo;
typedef boost::shared_ptr<OutPendingInfo> OutPendingInfo_SPtr;

class InPendingInfo;
typedef boost::shared_ptr<InPendingInfo> InPendingInfo_SPtr;

class RumConnectionsMgr;

typedef list<pair<ConnectionsAsyncCompletionListener*, ConnectionContext> > ConnCompletionListenerContext_List;

class OutPendingInfo
{
private:
	NodeIDImpl_SPtr _target;
	ConnCompletionListenerContext_List _listeners;

public:
	OutPendingInfo(NodeIDImpl_SPtr target,
			ConnectionsAsyncCompletionListener* listener, ConnectionContext ctx) :
		_target(target), _listeners()
	{
		_listeners.push_back(make_pair(listener, ctx));
	}

	virtual ~OutPendingInfo()
	{
	}

	ConnCompletionListenerContext_List getListeners() const
	{
		return _listeners;
	}

	void addListener(ConnectionsAsyncCompletionListener* listener, ConnectionContext ctx)
	{
		_listeners.push_back(make_pair(listener, ctx));
	}

	String getTargetName() const
	{
		return _target->getNodeName();
	}

	NodeIDImpl_SPtr getTarget() const
	{
		return _target;
	}
};

class InPendingInfo
{
private:
	String _sourceName;
	ConnCompletionListenerContext_List _listeners;

public:
	InPendingInfo(String sourceName,
			ConnectionsAsyncCompletionListener* listener, ConnectionContext ctx) :
		_sourceName(sourceName), _listeners()
	{
		_listeners.push_back(make_pair(listener, ctx));
	}

	InPendingInfo(String sourceName) :
		_sourceName(sourceName), _listeners()
	{
	}

	virtual ~InPendingInfo()
	{
	}

	ConnCompletionListenerContext_List getListeners() const
	{
		return _listeners;
	}

	void addListener(ConnectionsAsyncCompletionListener* listener, ConnectionContext ctx)
	{
		_listeners.push_back(make_pair(listener, ctx));
	}

	String getSourceName() const
	{
		return _sourceName;
	}
};

class RumCnContext: public RumContext
{

public:

	RumCnContext(RumConnectionsMgr * conMgr, int eventContext) :
		_conMgr(conMgr), _eventContext(eventContext)
	{
	}
	virtual ~RumCnContext()
	{
	}
	RumConnectionsMgr * getConMgr(void)
	{
		return _conMgr;
	}
	int getContext(void) const
	{
		return _eventContext;
	}

private:
	RumConnectionsMgr * _conMgr;
	int _eventContext;

};

typedef boost::shared_ptr<RumCnContext> RumCnContext_SPtr;

class RumConnectionsMgr: public CommRumReceiverListener, public CommRumTxMgrListener, ScTraceContext
{
private:
	static ScTraceComponent* _tc;
	rumInstance& _rum;
	const char* _thisMemberName;
	BusName_SPtr _thisBusName;
	CommRumTxMgr _txMgr;

	// node name -> connection
	typedef multimap<String, rumConnection> NodesConnectionsMap;
	NodesConnectionsMap _nodesConnectionsMap;

	// ctx -> (target_node, waiting clients)
	typedef map<int, OutPendingInfo_SPtr> OutPending_Map;
	OutPending_Map _outPendingMap;

	// connection -> (initiator_name, waiting clients)
	typedef map<rumConnectionID_t, InPendingInfo_SPtr> InPending_Map;
	InPending_Map _inPendingMap;

	typedef multimap<rumConnectionID_t, rumQueueT_SPtr> ConnID2QueueT_MMap;
	ConnID2QueueT_MMap _txsMap; // multi

	typedef map<rumStreamID_t, rumConnectionID_t> StreamID2ConnID_Map;
	StreamID2ConnID_Map _receivedStreams;

	typedef map<rumConnectionID_t, BusName_SPtr > ConnID2BusName_Map;
	ConnID2BusName_Map _connectionBusName;

	int _ctx;
	std::vector<RumCnContext_SPtr> _rumCnCtxList;

	mutable boost::recursive_mutex _mutex;

	NodeIDCache& _nodeIdCache;
	IncomingMsgQ_SPtr _incomingMsgQ;
	bool _started;
	bool _closed;
	const String _instID;
	NodeIDImpl_SPtr _myNodeId;
	int64_t _incarnationNumber;

	ByteBuffer_SPtr _connectMessage_buffer;
	std::size_t _connectMessage_prefix_pos;
	const std::size_t _connectMessage_min_size;

	SpiderCastConfigImpl _config;

	RumCnContext *_rumcnCtx;

	void closeAllConnections();

	static std::string toString(const rumConnection& connection_info);
	static std::string toString(rumConnectionEvent *connection_event);

	void deliver_warning_event(const String& errMsg, spdr::event::ErrorCode errCode, rumConnectionID_t conn_id);

	void deliver_dup_node_suspicion_event(const String& errMsg, spdr::event::ErrorCode errCode,
			rumConnectionID_t conn_id, const String& node, int64_t incomingIncNum);

protected:

public:

	RumConnectionsMgr(BusName_SPtr thisBusName,
			const char* thisMemberName, rumInstance& rum,
			NodeIDCache& cache, IncomingMsgQ_SPtr incomingMsgQ,
			const String& instID, NodeIDImpl_SPtr myNodeId,
			const SpiderCastConfigImpl& config, int64_t incarnationNumber);

	virtual ~RumConnectionsMgr();

	void start();

	bool createConnection(NodeIDImpl_SPtr targetNode,
			ConnectionsAsyncCompletionListener* asyncCompletionListener,
			ConnectionContext ctx);

	bool createConnection(NodeIDImpl_SPtr targetNode, ConnectionContext ctx);

	Neighbor_SPtr
	connectOnExisting(NodeIDImpl_SPtr targetNode);

	bool closeConnection(rumConnection con, bool flag);

	bool closeStream(Neighbor_SPtr target);

	int onRumConnectionEvent(rumConnectionEvent *connection_event, int event_context);

	void onSuccess(rumConnection& con, int event_context);
	void onFailureOutgoing(rumConnection& con, int ctx);
	void onFailureIncoming(rumConnection& con, int ctx);

	bool onNewConnection(rumConnectionEvent *connection_event);

	void onBreak(const rumConnection& con);
	void onReady(rumConnection& con);

	void onBreakInPending(const rumConnection& con);

	/*
	 * @see CommRumReceiverListener
	 */
	virtual std::string onNewStreamReceived(rumStreamID_t sid, String nodeName,
			rumConnectionID_t con);
	/*
	 * @see CommRumReceiverListener
	 */
	virtual void onStreamBreak(rumStreamID_t sid);

	/*
	 * @see CommRumTxMgrListener
	 */
	virtual void onStreamNotPresent(rumStreamID_t sid);

	bool addConnection(const String& node, const rumConnection& connection);
	bool addOutPending(NodeIDImpl_SPtr node, int context,
			ConnectionsAsyncCompletionListener* asyncCompletionListener,
			ConnectionContext connCtx);
	bool removeOutPending(int context);
	bool removeOutPending(const String& node);
	bool removeInPending(const rumConnection& con);

	void clear();

	bool containsOutNodePending(const String& node) const;
	bool containsOutNodePending(int context) const;
	bool containsInNodePending(const String& node) const;
	const rumConnection& getAConnection(const String& node) const;
	String toString() const;
	void terminate(bool grace);

	BusName_SPtr lookupBusName(
			Neighbor_SPtr neighbor);

	RumCnContext *getRumContext();

//	static int on_connection_event(rumConnectionEvent* connection_event,
//			void *user);

};

}

#endif /* RUMCONNECTIONMGR_H_ */

