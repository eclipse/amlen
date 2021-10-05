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
 * CommAdapter.h
 *
 *  Created on: Feb 8, 2010
 *      Author:
 *
 * Version     : $Revision: 1.7 $
 *-----------------------------------------------------------------
 * $Log: CommAdapter.h,v $
 * Revision 1.7  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.6.2.1  2016/01/27 09:07:16 
 * nameless tcp discovery
 *
 * Revision 1.6  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.5  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.4  2015/07/29 09:26:11 
 * split brain
 *
 * Revision 1.3  2015/05/20 11:11:01 
 * support for IPv6 in multicast discovery
 *
 * Revision 1.2  2015/05/05 12:48:25 
 * Safe connect
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.28  2015/01/26 12:38:13 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.27  2014/11/04 09:51:24 
 * Added multicast discovery
 *
 * Revision 1.26  2014/10/30 15:46:25 
 * Implemented the CommUDPMulticast, towards multicast discovery
 *
 * Revision 1.25  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.24  2011/06/20 11:38:23 
 * Add commUdp component
 *
 * Revision 1.22  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.21  2011/01/10 12:23:37 
 * Remove context from connectOnExisting() method
 *
 * Revision 1.20  2011/01/09 14:04:34 
 * *** empty log message ***
 *
 * Revision 1.19  2011/01/09 09:12:58 
 * Hierarchy branch merged in to HEAD
 *
 * Revision 1.18  2010/12/14 10:47:40 
 * Comm changes - receiver stream id in each message and neighbor, disconnect reject stream, if known and counts number tx and rcv before closing connection.
 *
 * Revision 1.17  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.16.4.1  2011/01/04 14:50:14 
 * Merge HEAD into Hierarchy branch
 *
 * Revision 1.16.2.2  2010/11/15 12:46:32 
 * Hopefully,last bug fixed
 *
 * Revision 1.16.2.1  2010/10/21 16:10:43 
 * Massive comm changes
 *
 * Revision 1.16  2010/09/18 17:04:55 
 * Hierarhical bus name support
 *
 * Revision 1.15  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.14  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.13  2010/06/23 14:15:38 
 * No functional chnage
 *
 * Revision 1.12  2010/06/08 15:46:25 
 * changed instanceID to String&
 *
 * Revision 1.11  2010/06/02 08:59:30 
 * *** empty log message ***
 *
 * Revision 1.10  2010/05/27 09:28:01 
 * Add disconnect() method
 *
 * Revision 1.9  2010/05/20 08:56:55 
 * Add nodeIdCache, incomingMsgQ, and the use of nodeId on the connect rather than a string
 *
 * Revision 1.8  2010/05/13 09:06:11 
 * added NodeIDCache to constructor/factory
 *
 * Revision 1.7  2010/05/10 08:27:46 
 * Use a new interface: ConnectionsAsyncCompletionListener rather than an asyncCompletionListener template; remove the connect() method without a completion listener
 *
 * Revision 1.6  2010/05/04 11:35:21 
 * Added IncomingMsgQ
 *
 * Revision 1.5  2010/04/12 09:16:31 
 * Use concurrentSharedPtr with getInstance() + misc. non-funcational changes
 *
 * Revision 1.4  2010/04/07 13:53:24 
 * Add CVS log
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 */

#ifndef COMMADAPTER_H_
#define COMMADAPTER_H_

#include "IncomingMsgQ.h"
#include "ConnectionsAsyncCompletionListener.h"
#include "Neighbor.h"
#include "SpiderCastConfigImpl.h"
#include "NodeIDCache.h"
#include "BusName.h"
#include "CommUDP.h"
#include "CommUDPMulticast.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class CommAdapter;
//typedef ConcurrentSharedPtr<CommAdapter> CommAdapter_SPtr;
typedef boost::shared_ptr<CommAdapter> CommAdapter_SPtr;

class CommAdapter
{

private:
	static ScTraceComponent* tc_;
	ScTraceContextImpl ctx_;
	boost::shared_ptr<CommUDP> _commUDP;
	boost::shared_ptr<CommUDPMulticast> _commUDPMulticast;

	const CommAdapter& operator=(const CommAdapter& commAdapter)
	{
		return *this;
	}

protected:
	volatile bool _started;
	BusName_SPtr _thisBusName;
	char* _thisMemberName;
	String _instID;
	bool _isUdpDiscovery;
	bool _isMulticastDiscovery;
	volatile bool _terminated;
	NodeIDCache& _nodeIdCache;
	IncomingMsgQ_SPtr _incomingMsgQueue;
	NodeIDImpl_SPtr _myNodeId;

	CommAdapter(const String& instID, const SpiderCastConfigImpl& config,
			NodeIDCache& cache, int64_t incarnationNum);

public:

	virtual ~CommAdapter();

	static CommAdapter_SPtr getInstance(const String& instID,
			const SpiderCastConfigImpl& config, NodeIDCache& cache, int64_t incarnationNumber);

	virtual IncomingMsgQ_SPtr getIncomingMessageQ();

	virtual bool connect(NodeIDImpl_SPtr target,
			ConnectionsAsyncCompletionListener* asyncCompletionListener,
			ConnectionContext ctx) = 0;

	/**
	 *
	 * @param target
	 * @param ctx
	 * @return true if request was created successfully
	 */
	virtual bool connect(NodeIDImpl_SPtr target, ConnectionContext ctx) = 0;

	virtual Neighbor_SPtr connectOnExisting(
			NodeIDImpl_SPtr target) = 0;

	virtual bool disconnect(Neighbor_SPtr target) = 0;

	virtual void start();
	virtual void terminate(bool grace);

	/**
	 * Send the message to the target.
	 *
	 * Uses UDP, message must be no longer then a single packet.
	 *
	 * @param target
	 * @param msg
	 * @return true if message was sent successfully
	 *
	 * @throws SpiderCastRuntimeError if UDP discovery is disabled
	 *
	 *
	 */
	bool sendTo(NodeIDImpl_SPtr target, SCMessage_SPtr msg);

	/**
	 * Send the message bundle to the target, each message must be no longer then a single packet.
	 *
	 * Uses UDP, message must be no longer then a single packet.
	 *
	 * @param target
	 * @param msgBundle
	 * @param numMsgs
	 * @return true if message was sent successfully
	 *
	 * @throws SpiderCastRuntimeError if UDP discovery is disabled
	 *
	 */
	bool sendTo(NodeIDImpl_SPtr target,
			std::vector<SCMessage_SPtr> & msgBundle, int numMsgs);

	/**
	 * Send the message to the discovery multicast group.
	 *
	 * Uses UDP, message must be no longer then a single packet.
	 *
	 * @param target
	 * @param msg
	 * @return true if message was sent successfully
	 *
	 * @throws SpiderCastRuntimeError if Multicast discovery is disabled
	 *
	 *
	 */
	bool sendToMCgroup(SCMessage_SPtr msg);

	/**
	 * Send the message bundle to the discovery multicast group.
	 *
	 * Uses UDP, each message must be no longer then a single packet.
	 *
	 * @param target
	 * @param msgBundle
	 * @param numMsgs
	 * @return true if message was sent successfully
	 *
	 * @throws SpiderCastRuntimeError if Multicast discovery is disabled
	 *
	 */
	bool sendToMCgroup(	std::vector<SCMessage_SPtr> & msgBundle, int numMsgs);


};

}

#endif /* COMMADAPTER_H_ */
