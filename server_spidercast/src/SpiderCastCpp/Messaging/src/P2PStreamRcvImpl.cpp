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
 * P2PStreamRcvImpl.cpp
 *
 *  Created on: Jun 28, 2012
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamRcvImpl.cpp,v $
 * Revision 1.2  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:15 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:16 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2012/10/10 15:33:36 
 * *** empty log message ***
 *
 * Revision 1.1  2012/09/30 12:41:24 
 * P2P initial version
 *
 */

#include "P2PStreamRcvImpl.h"

namespace spdr
{

namespace messaging
{

ScTraceComponent* P2PStreamRcvImpl::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Msgn,
		trace::ScTrConstants::Layer_ID_Msgn, "P2PStreamRcvImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

P2PStreamRcvImpl::P2PStreamRcvImpl(
		const String& instID,
		const SpiderCastConfigImpl& config,
		NodeIDCache& nodeIDCache,
		CoreInterface& coreInterface,
		MessageListener& messageListener,
		P2PStreamEventListener& eventListener,
		const PropertyMap& propMap) :

		ScTraceContext(_tc, instID, config.getMyNodeID()->getNodeName()),
		_instID(instID),
			_config(config),
			_nodeIDCache(nodeIDCache),
			_coreInterface(coreInterface),
			_messageListener(messageListener),
			_eventListener(eventListener),
			_propMap(propMap), _mutex(),
			_closed(false), messagingManager_SPtr(
					coreInterface.getMessagingManager())
{
	Trace_Entry(this, "P2PStreamRcvImpl()");

	// TODO: register with COMM

	Trace_Exit(this, "P2PStreamRcvImpl()");
}

P2PStreamRcvImpl::~P2PStreamRcvImpl()
{
	Trace_Entry(this, "~P2PStreamRcvImpl()");
	messagingManager_SPtr.reset();
}

void P2PStreamRcvImpl::close()
{
	//TODO: de-register from COMM
	Trace_Entry(this, "close()");
	bool do_close = false;
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		do_close = !_closed;
		_closed = true;
	}

	if (do_close)
	{
		messagingManager_SPtr->removeP2PRcv();
	}

	Trace_Exit(this, "close()");
}

bool P2PStreamRcvImpl::isOpen()
{
	boost::recursive_mutex::scoped_lock lock(_mutex);
	return !_closed;
}

std::string P2PStreamRcvImpl::toString() const
{
	std::ostringstream oss;
	oss << "P2PStreamRcvImpl; node: " << _config.getMyNodeID()->getNodeName() << "; closed: " << _closed << std::endl;

	return oss.str();
}

void P2PStreamRcvImpl::processIncomingDataMessage(SCMessage_SPtr message)
{
	// TODO: on a first message from a new stream raise a new stream event
	Trace_Entry(this, "processIncomingDataMessage()");

	if (!isOpen())
	{
		Trace_Exit(this, "processIncomingDataMessage()", "Closed");
		return;
	}

	ByteBuffer_SPtr bb = message->getBuffer();

	StreamID_SPtr sid = bb->readStreamID_SPtr();
	String  sourceName = bb->readString();
	int64_t mid = bb->readLong();

	if (ScTraceBuffer::isDebugEnabled(_tc))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this,
				"processIncomingDataMessage()");
		buffer->addProperty("sid", sid->toString());
		buffer->addProperty<int64_t> ("msgID", mid);

		buffer->invoke();
	}

	RxMessageImpl& rx_msg = message->getRxMessage();
	rx_msg.setStreamID(sid);
	rx_msg.setMessageID(mid);
	rx_msg.setSource(_nodeIDCache.getOrCreate(sourceName));

	int32_t length = bb->readInt();
	if (length > 0)
	{
		rx_msg.setBuffer(length, (bb->getBuffer() + bb->getPosition()));
	}
	else
	{
		rx_msg.setBuffer(0, NULL);
	}

	Trace_Debug(this, "processIncomingDataMessage()","before delivery");

	_messageListener.onMessage(rx_msg);

	Trace_Exit(this, "processIncomingDataMessage()");
}

void P2PStreamRcvImpl::rejectStream(StreamID& sid)
{
	Trace_Entry(this, "rejectStream()");

	Trace_Exit(this, "rejectStream()");

}

}

}
