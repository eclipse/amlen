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
 * CommRumReceiver.h
 *
 *  Created on: Feb 16, 2010
 *      Author:
 *
 * Version     : $Revision: 1.7 $
 *-----------------------------------------------------------------
 * $Log: CommRumReceiver.h,v $
 * Revision 1.7  2016/12/05 14:27:50 
 * Fix mem leak in Comm; Improve trace for stats.
 *
 * Revision 1.6  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.5.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.5  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.4  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.3  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.2  2015/05/07 09:51:28 
 * Support for internal/external endpoints
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.21  2015/01/26 12:38:13 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.20  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.19  2012/04/02 10:48:29 
 * UDP on IPv6 resolve bug, remove clear-all-streams
 *
 * Revision 1.18  2011/03/31 13:57:16 
 * Convert a couple of pointers to shared pointers
 *
 * Revision 1.17  2011/03/24 08:45:29 
 * added config to constructor in support of CRC
 *
 * Revision 1.16  2011/02/27 12:20:21 
 * Refactoring, header file organization
 *
 * Revision 1.15  2010/12/14 10:47:40 
 * Comm changes - receiver stream id in each message and neighbor, disconnect reject stream, if known and counts number tx and rcv before closing connection.
 *
 * Revision 1.14  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.13.2.1  2010/10/21 16:10:43 
 * Massive comm changes
 *
 * Revision 1.13  2010/09/18 17:04:55 
 * Hierarhical bus name support
 *
 * Revision 1.12  2010/08/17 13:26:01 
 * Add getStreamConnection() method
 *
 * Revision 1.11  2010/08/15 11:48:45 
 * Eliminate compilation warnings, no functional change
 *
 * Revision 1.10  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.9  2010/06/30 09:11:24 
 * Re-factor - no funcational changes
 *
 * Revision 1.8  2010/06/27 12:02:52 
 * Move all implementations from header files
 *
 * Revision 1.7  2010/06/23 14:16:28 
 * No functional chnage
 *
 * Revision 1.6  2010/06/02 08:59:30 
 * *** empty log message ***
 *
 * Revision 1.5  2010/05/27 09:27:07 
 * Enahnced logging
 *
 * Revision 1.4  2010/05/20 09:09:39 
 * Add nodeIdCache, incomingMsgQ,
 *
 * Revision 1.3  2010/05/06 09:34:01 
 * Declare mutexes as mutable so they can be grabbed in const methods
 *
 * Revision 1.2  2010/04/12 09:17:51 
 * Add a mutex for roper synchronization + misc. non-funcational changes
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 *
 *
 */

#ifndef COMMRUMRECEIVER_H_
#define COMMRUMRECEIVER_H_

#ifdef __MINGW__
#include <stdint.h>
#include <_mingw.h>
#endif

#include <boost/shared_ptr.hpp>

#include "rumCapi.h"

#include "CommAdapterDefinitions.h"
#include "CommRumReceiverListener.h"
#include "BusName.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

#include <map>
#include <iterator>

#include <boost/thread/recursive_mutex.hpp>

using namespace std;

namespace spdr
{

struct RumReceiverInfo
{
private:
	String _sender;
	BusName_SPtr _busName;
	rumConnectionID_t _con;
	StringSPtr _senderLocalName;

public:
	RumReceiverInfo(String sender, String busName, const rumConnectionID_t con) :
		_sender(sender), _busName(new BusName(busName.c_str())), _con(con), _senderLocalName()
	{
	}

	virtual ~RumReceiverInfo(void)
	{
	}

	rumConnectionID_t getConnection() const
	{
		return _con;
	}

	string getSender() const
	{
		return _sender;
	}

	BusName_SPtr getBusName() const
	{
		return _busName;
	}

	StringSPtr getSenderLocalName() const
	{
		return _senderLocalName;
	}

	void setSenderLocalName(const std::string& senderLocalName)
	{
		_senderLocalName.reset(new String(senderLocalName));
	}

	string toString() const
	{
		ostringstream oss;
		oss << "con=" << _con << " sender=" << _sender << " bus=" << spdr::toString(_busName) << " senderLocalName=" << (_senderLocalName? *_senderLocalName : "null");
		return oss.str();
	}
};

struct RumReceiverContext;
typedef boost::shared_ptr<RumReceiverInfo> RumReceiverInfo_SPtr;

class CommRumReceiver: public ScTraceContext
{

private:
	static ScTraceComponent* _tc;
	rumQueueR _queueR;
	rumInstance& _rum;
	boost::shared_ptr<CommRumReceiverListener> _listener;
	mutable boost::recursive_mutex _mapMutex;
	const String _instID;
	const bool _crcMemTopoEnabled;
	boost::shared_ptr<RumReceiverContext> _rumReceiverContext_SPtr;

	int createRumReceiver(int* p_error_code);
	void onBreak(rumStreamID_t sid);

protected:
	const char* _thisMemberName;
	BusName_SPtr _thisBusName;
	volatile bool _closed;
	volatile bool _started;

	typedef map<rumStreamID_t, RumReceiverInfo_SPtr> CachedStreams_Map;
	CachedStreams_Map _cachedStreams;
	NodeIDCache& _nodeIdCache;
	IncomingMsgQ_SPtr _incomingMsgQ;

	/**
	 *
	 * @param sid input
	 * @param sender output, extracted from queue name
	 * @param busName output, extracted from queue name
	 * @param senderLocalName output, target/sender local name as attached to connection
	 * @return found
	 */
	bool getStreamNames(rumStreamID_t sid, String& sender, BusName_SPtr& busName, StringSPtr& senderLocalName) const;

//	const String getStreamSender(rumStreamID_t sid) const;
//	const BusName_SPtr getStreamBusName(rumStreamID_t sid) const;

	const rumConnectionID_t getStreamConnection(rumStreamID_t sid) const;

	void addStream(rumStreamID_t sid, const String& sender, const String& busName,
			rumConnectionID_t con);
	void removeStream(rumStreamID_t sid);

public:

	CommRumReceiver(
			BusName_SPtr thisBusName,
			const char* thisMemberName,
			rumInstance& rum,
			boost::shared_ptr<CommRumReceiverListener> listener,
			NodeIDCache& cache,
			IncomingMsgQ_SPtr incomingMsgQ,
			const String& instID,
			const SpiderCastConfigImpl& config) :
				ScTraceContext(_tc, instID, thisMemberName),
				_queueR(),
				_rum(rum),
				_listener(listener),
				_instID(instID),
				_crcMemTopoEnabled(config.isCRCMemTopoMsgEnabled()),
				_thisMemberName(thisMemberName),
				_thisBusName(thisBusName),
				_closed(false),
				_started(false),
				_cachedStreams(),
				_nodeIdCache(cache),
				_incomingMsgQ(incomingMsgQ)
{
		Trace_Entry(this, "CommRumReceiver()");
}

	~CommRumReceiver();

	void terminate();
	void start();
	//TODO: remove?
	//void clearRejectedStreams();

	void onMessage(rumRxMessage* rumMsg);
	void onEvent(const rumEvent *event);
	int acceptStream(rumStreamParameters *stream_params);
	int rejectStream(rumStreamID_t sid, int *p_error_code, bool flag);

	/*
	const char* getBusName() const
	{
		return _thisBusName;
	}
	const char* getMemberName() const
	{
		return _thisMemberName;
	}
	*/
};

struct RumReceiverContext: public RumContext
{
	CommRumReceiver* _receiver;
	RumReceiverContext(CommRumReceiver* receiver) :
		_receiver(receiver)
	{
	}
	virtual ~RumReceiverContext(void)
	{
	}
};

}

#endif /* COMMRUMRECEIVER_H_ */

