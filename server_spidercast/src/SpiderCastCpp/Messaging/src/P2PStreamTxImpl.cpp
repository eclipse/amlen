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
 * P2PStreamTxImpl.cpp
 *
 *  Created on: Jul 2, 2012
 *      Author:
 *
  * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamTxImpl.cpp,v $
 * Revision 1.3  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:14 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:17 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2012/10/10 15:33:36 
 * *** empty log message ***
 *
 * Revision 1.1  2012/09/30 12:41:24 
 * P2P initial version
 *
 */

#include "P2PStreamTxImpl.h"

namespace spdr
{

namespace messaging
{

ScTraceComponent* P2PStreamTxImpl::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Msgn,
		trace::ScTrConstants::Layer_ID_Msgn,
		"P2PStreamTxImpl",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);


P2PStreamTxImpl::P2PStreamTxImpl(
		const String& instID,
		const SpiderCastConfigImpl& config,
		NodeIDCache& nodeIDCache,
		CoreInterface& coreInterface,
		P2PStreamEventListener& eventListener,
		const PropertyMap& propMap,
		StreamID_SPtr streamID,
		NodeID_SPtr target,
		Neighbor_SPtr neighbor) :
		P2PStreamTx(),
		ScTraceContext(_tc, instID, config.getMyNodeID()->getNodeName()),
		_instID(instID),
		_config(config),
		_nodeIDCache(nodeIDCache),
		_coreInterface(coreInterface),
		_eventListener(eventListener),
		_propMap(propMap),
		_streamID(streamID),
		_target(target),
		_neighbor(neighbor),
		_mutex(),
		_closed(false),
		_init_done(false),
		messagingManager_SPtr(coreInterface.getMessagingManager()),
		_next_msg_num(0),
		_outgoingDataMsg(new SCMessage),
		_header_size(0)

{
	Trace_Entry(this, "P2PStreamTxImpl()");
	_outgoingDataMsg->setBuffer(ByteBuffer::createByteBuffer(1024));
	ByteBuffer_SPtr bb(_outgoingDataMsg->getBuffer());

	//prepare the headers
	//H1
	_outgoingDataMsg->writeH1Header(SCMessage::Type_Data_Direct);

	bb->writeStreamID(static_cast<StreamIDImpl&>(*streamID));
	bb->writeString(config.getMyNodeID()->getNodeName());

	_header_size = bb->getPosition();

	Trace_Exit(this, "P2PStreamTxImpl()");
}


P2PStreamTxImpl::~P2PStreamTxImpl()
{
	Trace_Entry(this, "~P2PStreamTxImpl()");

	messagingManager_SPtr.reset();

	Trace_Exit(this, "~P2PStreamTxImpl()");
}

NodeID_SPtr P2PStreamTxImpl::getTarget() const
{
	return _target;
}

StreamID_SPtr P2PStreamTxImpl::getStreamID() const
{
	return _streamID;
}

int64_t P2PStreamTxImpl::submitMessage(const TxMessage& message)
{

	Trace_Entry(this,"submitMessage()");

	int64_t msg_num = -1;

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		if (_closed)
		{
			throw IllegalStateException("Service is closed.");
		}

		ByteBuffer_SPtr bb = _outgoingDataMsg->getBuffer();
		bb->setPosition(_header_size);
		bb->writeLong(_next_msg_num);
		bb->writeInt(message.getBuffer().first);
		bb->writeByteArray(message.getBuffer().second, message.getBuffer().first);
		_outgoingDataMsg->updateTotalLength();

		try
		{
			if (_neighbor->sendMessage(_outgoingDataMsg))
			{
				Trace_Event(this, "submitMessage()",
						"couldn't send a message to", "node",
						_neighbor->getName());
			}
			else
			{
				Trace_Event(this, "submitMessage()",
						"sent message to", _neighbor->getName());
			}

			msg_num = _next_msg_num++;
		}
		catch (SpiderCastRuntimeError& re)
		{
			Trace_Event(this,"submitMessage()","Failed to send message",
					"what",re.what(),
					"trace", re.getStackTrace());
			throw;
		}
	}

	Trace_Exit<int64_t>(this,"submitMessage()",msg_num);
	return msg_num;

}

void P2PStreamTxImpl::close()
{
	Trace_Entry(this, "close()");

	bool do_close = false;

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		do_close = !_closed;
		_closed = true;
	}

	if (do_close)
	{
		_neighbor->close();
		_coreInterface.getCommAdapter()->disconnect(_neighbor);
	}

	Trace_Exit(this, "close()");
}

bool P2PStreamTxImpl::isOpen() const
{
	Trace_Entry(this, "isOpen()");

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		return !_closed;
	}

	Trace_Exit(this, "isOpen()");
}


std::string P2PStreamTxImpl::toString() const
{
	std::string s("P2PStreamTxImpl: ");
	s = s + "my name: " + _config.getMyNodeID()->getNodeName() + "; target=" +_target->toString() + " sid=" + _streamID->toString() + "; Neighbor: " + _neighbor->toString();
	return s;
}


}

}

