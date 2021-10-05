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
 * CommEventInfo.h
 *
 *  Created on: 28/03/2010
 *      Author:
 *
 * Version     : $Revision: 1.6 $
 *-----------------------------------------------------------------
 * $Log: CommEventInfo.h,v $
 * Revision 1.6  2015/11/18 08:37:03 
 * Update copyright headers
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
 * Revision 1.15  2015/01/26 12:38:14 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.14  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.13  2012/09/30 12:35:15 
 * P2P initial version
 *
 * Revision 1.12  2012/06/27 12:38:57 
 * Seperate generic COMM from RUM
 *
 * Revision 1.11  2012/02/14 12:45:18 
 * Add structured topology
 *
 * Revision 1.10  2011/10/10 09:51:08 
 * *** empty log message ***
 *
 * Revision 1.9  2011/01/10 12:24:31 
 * Add toString()
 *
 * Revision 1.8  2011/01/09 09:12:58 
 * Hierarchy branch merged in to HEAD
 *
 * Revision 1.7  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.6.4.1  2010/10/26 14:43:25 
 * *** empty log message ***
 *
 * Revision 1.6.2.2  2010/10/27 13:25:14 
 * Some comments about types of events to CommEventInfo
 *
 * Revision 1.6.2.1  2010/10/21 16:10:42 
 * Massive comm changes
 *
 * Revision 1.6  2010/08/03 13:56:48 
 * Add support for on_success and on_failure connection events to be passed in through the incoming msg Q
 *
 * Revision 1.5  2010/06/20 14:39:55 
 * Change the connection info to consist of the connection id
 *
 * Revision 1.4  2010/06/17 11:43:36 
 * Add connection info. For debugging purposes
 *
 * Revision 1.3  2010/05/20 09:06:18 
 * Initial version
 *
 */
#ifndef COMMEVENTINFO_H_
#define COMMEVENTINFO_H_

#ifdef __MINGW__
#include <stdint.h>
#include <_mingw.h>
#endif
//#include "rumCapi.h"

//#include "ConcurrentSharedPtr.h"
#include "NodeIDImpl.h"
//#include "Neighbor.h"

namespace spdr
{
class Neighbor;
typedef boost::shared_ptr<Neighbor> Neighbor_SPtr;

enum ConnectionContext
{
	UnspecifiedCtx = 0,

	DiscoveryCtx = 1,

	RingCtx,

	RandomCtx,

	StructuredCtx,

	HierarchyCtx,

	DHTProxyCtx,

	DHTServerCtx,

	P2PCtx
};

class CommEventInfo
{
public:
	enum EventType
	{
		/**
		 * This message type send if connection to node was initiated by other side
		 */
		New_Source,

		/**
		 * Message send if RUM connection was broken, time-outed or closed (by other side)
		 */
		On_Break,

		/**
		 * Should be send if node successful created connection/transmitter to other side.
		 */
		On_Connection_Success,

		/**
		 * Event send if node failed to open connection or transmitter or initiate RUM, etc
		 * Actually, opposite to <b>On_Connection_Success</b> message
		 */
		On_Connection_Failure,

		/**
		 * This event is sent when this node rejected an incoming connection from a remote node,
		 * due the version, bus or target names being incompatible, or failure to parse the connect message.
		 */
		Warning_Connection_Refused,

		/**
		 * This event is sent when this node rejected an incoming UDP message from a remote node,
		 * due the version, bus or target names being incompatible, or failure to parse the message.
		 */
		Warning_UDP_Message_Refused,

		/**
		 * This event is sent when this node rejects a connection request from a remote node,
		 * because a connection already exists from a node with the same name, but the
		 * incarnation number of the node issuing the new request is higher than the
		 * incarnation number of the node already connected.
		 */
		Suspicion_Duplicate_Remote_Node,

		/**
		 * This event is sent when there is a fatal error in the CommAdapter components.
		 */
		Fatal_Error
	};



	CommEventInfo(EventType eventType, uint64_t connectionId,
			Neighbor_SPtr neighbor = Neighbor_SPtr());
	virtual ~CommEventInfo();

	EventType getType() const;
	uint64_t getConnectionId() const;
	Neighbor_SPtr getNeighbor() const;
	int getContext() const;
	int getErrCode() const;
	String getErrMsg() const;
	int64_t getIncNum() const;

	void setErrCode(int code);
	void setErrMsg(String msg);
	void setContext(int context);
	void setIncNum(int64_t incNum);


	/**
	 * A string representation of the event.
	 * @return
	 */
	String toString() const;
	static const String EventTypeName[];

protected:
	CommEventInfo();

private:


	EventType _eType;
	int _context;
	uint64_t _connectionId;
	Neighbor_SPtr _neighbor;
	int _errCode;
	String _errMsg;
	int64_t _incNum;
};

}

#endif /* COMMEVENTINFO_H_ */
