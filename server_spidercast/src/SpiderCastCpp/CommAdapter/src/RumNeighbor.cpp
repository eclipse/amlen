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
 * RumNeighbor.cpp
 *
 *  Created on: Apr 10, 2012
 *      Author:
 *
 * Version     : $Revision: 1.6 $
 *-----------------------------------------------------------------
 * $Log: RumNeighbor.cpp,v $
 * Revision 1.6  2016/01/07 14:41:18 
 * bug fix: closeStream with virgin RumNeighbor
 *
 * Revision 1.5  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.4  2015/11/12 10:28:16 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.3  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.2  2015/10/09 06:56:11 
 * bug fix - when neighbor in the table but newNeighbor not invoked yet, don't send to all neighbors but only to routable neighbors. this way the node does not get a diff mem/metadata before the full view is received.
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:07 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2012/07/02 12:38:10 
 * fix 64bit compile errors
 *
 * Revision 1.1  2012/06/27 12:38:57 
 * Seperate generic COMM from RUM
 *
 */

#include <iostream>
#include <sstream>
#include "RumNeighbor.h"

using namespace std;

namespace spdr
{

RumNeighbor::RumNeighbor(
		const rumConnection& connection,
		rumQueueT_SPtr tx,
		rumStreamID_t sid,
		const String& targetName,
		const String& myName,
		const String& instID) :
	Neighbor(myName, instID,
	targetName),
	_tx(tx),
	_connection(connection)
{
	Trace_Entry(this, "RumNeighbor()");
	_closed = false;
	_reciever = 0;
	_sid = static_cast<uint64_t>(sid);
	_virgin = false;
	int rc;
	Trace_Event(this, "RumNeighbor()", "before rumInitStructureParameters");
	if (rumInitStructureParameters(RUM_SID_TxMESSAGE, &msg, RUMCAPI_VERSION,
			&rc) == RUM_FAILURE)
	{
		ostringstream oss;
		oss << "failed to init the RUM message. rc: " << rc;
		Trace_Event(this, "Neighbor()", oss.str());

		throw SpiderCastRuntimeError(
				"Neighbor::Neighbor, failed to init the RUM message. rc: ");
	}
	Trace_Event(this, "RumNeighbor()", "after rumInitStructureParameters");
	Trace_Exit(this, "RumNeighbor()");

}

RumNeighbor::RumNeighbor(const rumConnection& connection, const String& myName, const String& instID, const String& targetName ):
		Neighbor(myName, instID, targetName),
		_tx(),
		_connection(connection)
{
}

int RumNeighbor::sendMessage(SCMessage_SPtr scMsg)
{
	Trace_Entry(this, "sendMessage()", toString());

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		if (_closed)
			return -1;
		boost::shared_ptr<ByteBuffer> buf = (*scMsg).getBuffer();

		msg.msg_buf = (char *) (*buf).getBuffer();
		msg.msg_len = (*buf).getDataLength();
		int rc;
		//Trace_Event(this, "Neighbor()", "RUMTimeStamp before rumTSubmitMessage");
		//	ostringstream oss;
		//	oss << _connection.connection_id;
		//	Trace_Event(this, "sendMessage()", "target", _targetName, "connection", oss.str());
		if (rumTSubmitMessage(_tx.get(), &msg, &rc) != RUM_SUCCESS)
		{
			ostringstream oss;
			oss << "failed, rc: " << rc;
			Trace_Event(this, "sendMessage()", oss.str());
			return rc;
		}
	}

	//Trace_Event(this, "Neighbor()", "RUMTimeStamp after rumTSubmitMessage");
	Trace_Exit(this, "sendMessage()");
	return 0;
}

RumNeighbor::~RumNeighbor()
{
	Trace_Entry(this, "~RumNeighbor");
}

uint64_t RumNeighbor::getConnectionId() const
{
	return getConnection().connection_id;
}

String RumNeighbor::toString() const
{
	stringstream tmpOut;
	if (!isVirgin())
	{
		tmpOut << "Neighbor: target=" << _targetName << ", con="
				<< _connection.connection_id << ", tx-sid=" << getSid()
				<< ", receiver-id=" << getReceiverId() << ", closed="
				<< std::boolalpha << _closed << endl;
	}
	else
	{
		tmpOut << "Neighbor: Virgin target=" << _targetName << ", con="
				<< _connection.connection_id << ", closed="
				<< std::boolalpha << _closed << endl;

	}
	return tmpOut.str();

}

bool RumNeighbor::operator==(Neighbor& rhs) const
{
	RumNeighbor *rumNeighbor = static_cast<RumNeighbor *>(&rhs);

	if (this->_tx && rumNeighbor->_tx)
	{
		if (this->_tx->instance != rumNeighbor->_tx->instance)
			return false;

		if (this->_tx->handle != rumNeighbor->_tx->handle)
			return false;
	}
	else if ((this->_tx && !rumNeighbor->_tx)
			|| (!this->_tx && rumNeighbor->_tx))
	{
		return false;
	}

	if (this->_sid != rumNeighbor->_sid)
		return false;

	if (this->_connection.connection_id
			!= rumNeighbor->_connection.connection_id)
		return false;

	if (this->_reciever != rumNeighbor->_reciever)
		return false;

	if (this->_myName != rumNeighbor->_myName)
		return false;

	if (this->_targetName != rumNeighbor->_targetName)
		return false;

	return true;
}

bool RumNeighbor::operator!=(Neighbor& rhs) const
{
	return !(*this == rhs);
}

}

