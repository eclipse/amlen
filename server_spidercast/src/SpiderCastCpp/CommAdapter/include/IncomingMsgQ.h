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
 * IncomingMsgQ.h
 *
 *  Created on: 04/05/2010
 *      Author:
 *
 * Version     : $Revision: 1.4 $
 *-----------------------------------------------------------------
 * $Log: IncomingMsgQ.h,v $
 * Revision 1.4  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.3.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.3  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.14  2015/01/26 12:38:13 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.13  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.12  2012/02/13 13:08:22 
 * typedef
 *
 * Revision 1.11  2011/06/29 11:46:46 
 * *** empty log message ***
 *
 * Revision 1.10  2011/05/01 08:27:32 
 * Add a DHT queue
 *
 * Revision 1.9  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.8  2010/12/09 12:15:36 
 * SC_Stats improvements
 *
 * Revision 1.7  2010/12/08 10:48:03 
 * Added statistics support
 *
 * Revision 1.6  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.5.2.1  2010/10/21 16:10:42 
 * Massive comm changes
 *
 * Revision 1.5  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.4  2010/06/23 14:16:41 
 * No functional chnage
 *
 */

#ifndef INCOMINGMSGQ_H_
#define INCOMINGMSGQ_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "SCMessage.h"
#include "NodeIDCache.h"
#include "ThreadControl.h"
//#include "ConcurrentSharedPtr.h"
#include "NodeIDImpl.h"
#include "ConnectionsAsyncCompletionListener.h"
#include "EnumCounter.h"


#include "ScTr.h"
#include "ScTraceBuffer.h"

#include <deque>

namespace spdr
{

class IncomingMsgQ;
typedef boost::shared_ptr<IncomingMsgQ> IncomingMsgQ_SPtr;

class IncomingMsgQ: public ScTraceContext, public ConnectionsAsyncCompletionListener
{
public:

	/*
	 * When adding queues, remember to update the qTypeName array, and reportStats() method.
	 */
	enum QType
	{
		NullQ = 0,   //!< NullQ
		TopologyQ,   //!< TopologyQ
		MembershipQ, //!< MembershipQ
		DHTQ, 		 //!< DHTQ
		DataQ,       //!< DataQ
		LastPositionQ//!< LastPositionQ
	};

	IncomingMsgQ(const String& instanceID,
			NodeIDImpl_SPtr myNodeId,
			NodeIDCache& cache);
	virtual ~IncomingMsgQ();

	void registerReaderThread(ThreadControl* threadControl, QType qtype);

	void onMessage(SCMessage_SPtr msg);

	SCMessage_SPtr pollQ(QType qtype);

	bool isQEmpty(QType qtype);

	size_t getQSize(QType qtype);

	virtual void onSuccess(Neighbor_SPtr result, ConnectionContext ctx);
	virtual void onFailure(String FailedTargetName, int rc, String message, ConnectionContext ctx);

	/**
	 * Print a statistics report to the log.
	 *
	 * @param time
	 * @param labels Labels or counters
	 */
	void reportStats(boost::posix_time::ptime time, bool labels = false);

private:
	static ScTraceComponent* _tc;

	static const char* const qTypeNames[];

	std::deque<SCMessage_SPtr> _messageQueues[LastPositionQ];

	String _instId;

	ThreadControl* _readerThreads[LastPositionQ];

	NodeIDImpl_SPtr _myNodeID;

	//boost::recursive_mutex _mutex;

	boost::recursive_mutex _mutexTopo;
	boost::recursive_mutex _mutexMem;
	boost::recursive_mutex _mutexDHT;
	boost::recursive_mutex _mutexData;

	boost::recursive_mutex _mutexStats;

	NodeIDCache& _nodeIdCache;

	EnumCounter<SCMessage::MessageType, int32_t> inMsgCount_;
	EnumCounter<SCMessage::MessageType, int32_t> inMsgBytes_;
	EnumCounter<SCMessage::MessageGroup, int32_t> inMsgGroup_;
	EnumCounter<QType, int32_t> inQueueSize_;

};

}//namespace spdr

#endif /* INCOMINGMSGQ_H_ */
