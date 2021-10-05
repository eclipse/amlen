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
 * LocalNeighbor.cpp
 *
 *  Created on: May 19, 2011
 *      Author:
 */

#include "LocalNeighbor.h"

namespace spdr
{

LocalNeighbor::LocalNeighbor(NodeIDImpl_SPtr myNodeId, const String& instID,
		IncomingMsgQ_SPtr incomingMsgQ, BusName_SPtr myBusName) :
	Neighbor(myNodeId->getNodeName(), instID, myNodeId->getNodeName()),
			_incomingMsgQ(incomingMsgQ), _myNodeId(myNodeId), _myBusName(
					myBusName)
{
	//_targetName = myNodeId->getNodeName();
	//	_myName = myNodeId->getNodeName();
	//	_instID = instID;
	_sid = 0;
	_virgin = false;

}

LocalNeighbor::~LocalNeighbor()
{
	// TODO Auto-generated destructor stub
	//std::cout << "~LocalNeighbor" << std::endl;
}

uint64_t LocalNeighbor::getConnectionId() const
{
	Trace_Entry(this, "LocalNeighbor::getConnectionId");
	return 0;
}

uint64_t LocalNeighbor::getSid() const
{
	Trace_Entry(this, "LocalNeighbor::getSid");
	return 0;
}

int LocalNeighbor::sendMessage(SCMessage_SPtr msg)
{
	boost::recursive_mutex::scoped_lock lock(_mutex);
	Trace_Entry(this, "LocalNeighbor::sendMessage");

	boost::shared_ptr<ByteBuffer> buf = (*msg).getBuffer();

	boost::shared_ptr<ByteBuffer> buffer(ByteBuffer::createReadOnlyByteBuffer(
			(char *) (*buf).getBuffer(), (*buf).getDataLength(), false));

	SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
	(*scMsg).setBuffer(buffer);

	(*scMsg).setSender(_myNodeId);
	(*scMsg).setStreamId(0);
	(*scMsg).setBusName(_myBusName);

	_incomingMsgQ->onMessage(scMsg);
	Trace_Exit(this, "LocalNeighbor::sendMessage");

	return 0;

}

String LocalNeighbor::toString() const
{
	return "LocalNeighbor";
}

}
