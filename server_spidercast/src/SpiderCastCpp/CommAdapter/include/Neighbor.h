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
 * Neighbor.h
 *
 *  Created on: Feb 25, 2010
 *      Author:
 *
 * Version     : $Revision: 1.4 $
 *-----------------------------------------------------------------
 * $Log: Neighbor.h,v $
 * Revision 1.4  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.3  2015/11/12 10:28:16 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.2  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.22  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.21  2012/07/02 12:38:10 
 * fix 64bit compile errors
 *
 * Revision 1.20  2012/06/27 12:38:57 
 * Seperate generic COMM from RUM
 *
 * Revision 1.19  2012/02/20 15:44:37 
 * add const to toString(), add hash_value()
 *
 * Revision 1.18  2012/02/16 09:14:40 
 * implement Neighbor equality operator
 *
 * Revision 1.17  2011/07/10 09:23:37 
 * Create an RUM Tx on a seperate thread (not thr RUM thread)
 *
 * Revision 1.16  2011/05/22 12:52:14 
 * Introduce a local neighbor
 *
 * Revision 1.15  2011/05/17 06:14:01 
 * *** empty log message ***
 *
 * Revision 1.14  2011/01/09 09:12:58 
 * Hierarchy branch merged in to HEAD
 *
 * Revision 1.13  2010/12/26 07:45:17 
 * Fix spelling and move implementation to .cpp file (get / set receiverId)
 *
 * Revision 1.12  2010/12/14 10:47:40 
 * Comm changes - receiver stream id in each message and neighbor, disconnect reject stream, if known and counts number tx and rcv before closing connection.
 *
 * Revision 1.11  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.10.2.1  2010/10/21 16:10:43 
 * Massive comm changes
 *
 * Revision 1.10  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.9  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.8  2010/06/27 13:59:29 
 * Rename print to toString and have it return a string rather than print
 *
 * Revision 1.7  2010/06/27 12:01:46 
 * No functional changes
 *
 * Revision 1.6  2010/06/23 14:16:52 
 * No functional chnage
 *
 * Revision 1.5  2010/05/27 09:27:35 
 * Enahnced logging + close() method
 *
 * Revision 1.4  2010/05/20 09:12:20 
 * Change sendMessage to send an SCMessage
 *
 * Revision 1.3  2010/04/12 09:19:37 
 * Add a mutex for roper synchronization
 *
 * Revision 1.2  2010/04/07 13:53:24 
 * Add CVS log
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 */

#ifndef NEIGHBOR_H_
#define NEIGHBOR_H_

#ifdef __MINGW__
#include <stdint.h>
#include <_mingw.h>
#endif

#include "rumCapi.h"

#include "Definitions.h"
#include "SpiderCastRuntimeError.h"
#include "SCMessage.h"
#include "BusName.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

#include <boost/thread/recursive_mutex.hpp>

namespace spdr
{

class Neighbor;
typedef boost::shared_ptr<Neighbor> 	Neighbor_SPtr;

class Neighbor: public ScTraceContext
{
private:
	static ScTraceComponent* _tc;

public:
	virtual ~Neighbor();

	/**
	 *
	 * @param msg
	 * @return 0 for success
	 */
	virtual int sendMessage(SCMessage_SPtr msg) = 0;

	virtual uint64_t getConnectionId() const = 0;

	//	rumConnection getConnection() const
	//	{
	//		return _connection;
	//	}

	virtual uint64_t getSid() const = 0;
	//	{
	//		return _sid;
	//	}

	//	rumQueueT getTx() const
	//	{
	//		return _tx;
	//	}

	virtual uint64_t getReceiverId() const;

	virtual void setReceiverId(uint64_t streamId);

	virtual String getName() const;

	virtual String toString() const = 0;

	virtual void close();

	virtual bool isVirgin() const;

	virtual bool operator==(Neighbor& rhs) const;


	virtual bool operator!=(Neighbor& rhs) const;

	friend
	std::size_t hash_value(const Neighbor& n);

protected:

	//Neighbor(const rumConnection& connection, const rumQueueT& tx,
	//	rumStreamID_t sid, String targetName, String myName, const String& instID);

	Neighbor(String myName, const String& instID, const String& targetName );

	//rumQueueT _tx;

	uint64_t _reciever;

	//rumTxMessage msg;

	bool _closed;

	//rumConnection _connection;

	uint64_t _sid;

	boost::recursive_mutex _mutex;

	String _instID;

	bool _virgin;

	const String _targetName;

	const String _myName;


};

}

#endif /* NEIGHBOR_H_ */
