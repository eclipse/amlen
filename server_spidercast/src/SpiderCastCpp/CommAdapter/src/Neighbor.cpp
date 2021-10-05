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
 * Neighbor.cpp
 *
 *  Created on: Feb 25, 2010
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: Neighbor.cpp,v $
 * Revision 1.3  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.2  2015/11/12 10:28:16 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.32  2012/10/25 10:11:07 
 * Added copyright and proprietary notice
 *
 * Revision 1.31  2012/07/02 12:38:10 
 * fix 64bit compile errors
 *
 * Revision 1.30  2012/06/27 12:38:57 
 * Seperate generic COMM from RUM
 *
 * Revision 1.29  2012/02/20 15:44:37 
 * add const to toString(), add hash_value()
 *
 * Revision 1.28  2012/02/16 09:14:40 
 * implement Neighbor equality operator
 *
 * Revision 1.27  2011/07/10 09:23:52 
 * Create an RUM Tx on a seperate thread (not thr RUM thread)
 *
 * Revision 1.26  2011/05/30 17:45:19 
 * Streamline mutex locking
 *
 * Revision 1.25  2011/05/22 12:52:36 
 * Introduce a local neighbor
 *
 * Revision 1.24  2011/03/24 08:47:00 
 * *** empty log message ***
 *
 * Revision 1.23  2011/01/13 10:14:20 
 * Changed toStaring(); no functional change
 *
 * Revision 1.22  2011/01/02 13:10:28 
 * Random selection from bootstrap
 *
 * Revision 1.21  2010/12/26 07:46:23 
 * Fix spelling and move implementation to .cpp file (get / set receiverId)
 *
 * Revision 1.20  2010/12/22 12:05:16 
 * Fix compiler warning; logging changes
 *
 * Revision 1.19  2010/12/14 10:47:40 
 * Comm changes - receiver stream id in each message and neighbor, disconnect reject stream, if known and counts number tx and rcv before closing connection.
 *
 * Revision 1.18  2010/11/25 16:15:05 
 * After merge of comm changes back to head
 *
 * Revision 1.16.2.1  2010/11/23 14:12:05 
 * merged with HEAD branch
 *
 * Revision 1.16  2010/08/16 07:33:43 
 * Add log entry on sendMEssage
 *
 * Revision 1.15  2010/08/04 12:36:31 
 * Remove RUMtime tracing
 *
 * Revision 1.14  2010/08/03 13:58:00 
 * Add RUM time tracing
 *
 * Revision 1.13  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.12  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.11  2010/06/27 13:59:53 
 * Rename print to toString and have it return a string rather than print
 *
 * Revision 1.10  2010/06/23 14:19:06 
 * No functional chnage
 *
 * Revision 1.9  2010/06/14 15:52:19 
 * Add target on destructor print
 *
 * Revision 1.8  2010/06/08 10:29:06 
 * Add target name to print out
 *
 * Revision 1.7  2010/06/03 14:43:21 
 * Arrange print statmenets
 *
 * Revision 1.6  2010/05/27 09:32:11 
 * Enahnced logging + close() method
 *
 * Revision 1.5  2010/05/20 09:22:36 
 * Change sendMessage to send an SCMessage
 *
 * Revision 1.4  2010/04/14 09:16:21 
 * Add some print statemnets - no funcional changes
 *
 * Revision 1.3  2010/04/12 09:22:09 
 * Add a mutex for roper synchronization
 *
 * Revision 1.2  2010/04/07 13:55:16 
 * Add CVS log
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 */

#include <iostream>
#include <sstream>
#include "Neighbor.h"

using namespace std;

namespace spdr
{

using namespace trace;

ScTraceComponent* Neighbor::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Comm,
		trace::ScTrConstants::Layer_ID_CommAdapter, "Neighbor",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

Neighbor::Neighbor(String myName, const String& instID, const String& targetName) :
	ScTraceContext(_tc, instID, myName),
	_reciever(0),
	_closed(false),
	_sid(0),
	_instID(instID),
	_virgin(true),
	_targetName(targetName),
	_myName(myName)
{

}

Neighbor::~Neighbor()
{
	Trace_Entry(this, "~Neighbor");
}

void Neighbor::close()
{
	Trace_Entry(this, "close");

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		_closed = true;
	}
	Trace_Exit(this, "close");
}

bool Neighbor::isVirgin() const
{
	return _virgin;
}

void Neighbor::setReceiverId(uint64_t streamId)
{
	_reciever = streamId;

	if (ScTraceBuffer::isExitEnabled(_tc))
	{
		stringstream oss;
		oss << _reciever;
		ScTraceBufferAPtr buffer = ScTraceBuffer::exit(this, "setReceiverId",
				oss.str());
		buffer->invoke();
	}
}

String Neighbor::getName() const
{
	return _targetName;
}


uint64_t Neighbor::getReceiverId() const
{
	if (ScTraceBuffer::isEntryEnabled(_tc))
	{
		stringstream oss;
		oss << _reciever;
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "getRecieverId",
				oss.str());
		buffer->invoke();
	}

	return _reciever;
}

bool Neighbor::operator==(Neighbor& rhs) const
{
	//std::cout << "operator==Neighbor" << std::endl;
	if (this->_myName != rhs._myName)
		return false;

	if (this->_targetName != rhs._targetName)
		return false;

	return true;
}

bool Neighbor::operator!=(Neighbor& rhs) const
{
	//std::cout << "operator!=Neighbor" << std::endl;
	return !((*this) == rhs);
}

}
