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
 * LocalNeighbor.h
 *
 *  Created on: May 19, 2011
 *      Author:
 *
 * Version     : $Revision: 1.4 $
 *-----------------------------------------------------------------
 * $Log: LocalNeighbor.h,v $
 * Revision 1.4  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.3  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.2  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:16 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.5  2015/01/26 12:38:13 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.4  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.3  2012/06/27 12:38:57 
 * Seperate generic COMM from RUM
 *
 * Revision 1.2  2012/02/01 15:56:31 
 * Messaging manager skeleton
 *
 * Revision 1.1  2011/05/22 12:52:14 
 * Introduce a local neighbor
 *
 */

#ifndef LOCALNEIGHBOR_H_
#define LOCALNEIGHBOR_H_

#include "Neighbor.h"
#include "IncomingMsgQ.h"

namespace spdr
{

class LocalNeighbor: public spdr::Neighbor
{
public:
	LocalNeighbor(NodeIDImpl_SPtr myNodeId, const String& instID,
			IncomingMsgQ_SPtr incomingMsgQ,
			BusName_SPtr myBusName);

	virtual ~LocalNeighbor();

	/**
	 * Deliver locally.
	 *
	 * Copies the message and submits it to the local incoming message queue.
	 *
	 * @param msg
	 * @return
	 */
	virtual int sendMessage(SCMessage_SPtr msg);

	virtual uint64_t getConnectionId() const;

	virtual uint64_t getSid() const;

	String toString() const;

private:
	IncomingMsgQ_SPtr _incomingMsgQ;
	NodeIDImpl_SPtr _myNodeId;
	BusName_SPtr _myBusName;

};

}

#endif /* LOCALNEIGHBOR_H_ */
