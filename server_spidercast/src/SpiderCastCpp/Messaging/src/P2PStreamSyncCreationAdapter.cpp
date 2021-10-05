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
 * P2PStreamSyncCreationAdapter.cpp
 *
 *  Created on: Aug 6, 2012
 *      Author:
 *
 * Version     : $Revision: 1.4 $
 *-----------------------------------------------------------------
 * $Log: P2PStreamSyncCreationAdapter.cpp,v $
 * Revision 1.4  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.3.2.1  2016/01/21 14:52:48 
 * add senderLocalName on every incoming message, as attached to the connection
 *
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

#include "P2PStreamSyncCreationAdapter.h"

namespace spdr
{

namespace messaging
{

using namespace trace;

ScTraceComponent* P2PStreamSyncCreationAdapter::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Topo,
		trace::ScTrConstants::Layer_ID_Msgn, "P2PStreamSyncCreationAdapter",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

P2PStreamSyncCreationAdapter::P2PStreamSyncCreationAdapter(
		const String& instID, const String& nodeName) :
	ScTraceContext(_tc, instID, nodeName),
	ConnectionsAsyncCompletionListener(),
	_instID(instID), _name(nodeName),
			_neighbor(), _rc(-1), _msg(), _condVar(), _mutex()
{

}

P2PStreamSyncCreationAdapter::~P2PStreamSyncCreationAdapter()
{
}

void P2PStreamSyncCreationAdapter::onSuccess(
		Neighbor_SPtr result, ConnectionContext ctx)
{
	_neighbor = result;

	boost::recursive_mutex::scoped_lock lock(_mutex);
	_condVar.notify_all();
}

void P2PStreamSyncCreationAdapter::onFailure(String FailedTargetName, int rc,
		String message, ConnectionContext ctx)
{
	std::ostringstream oss;
	oss << "Failed to create Tx to " << FailedTargetName << " rc: " << rc
			<< " msg: " << message;
	_msg = oss.str();
	boost::recursive_mutex::scoped_lock lock(_mutex);
	_condVar.notify_all();

}

Neighbor_SPtr P2PStreamSyncCreationAdapter::waitForCompletion()
{
	Trace_Entry(this, "waitForCompletion");
	boost::recursive_mutex::scoped_lock lock(_mutex);

	while (notDone())
	{
		_condVar.wait(lock);
	}

	return _neighbor;

}

String P2PStreamSyncCreationAdapter::getErrorMessage()
{
	return _msg;
}

bool P2PStreamSyncCreationAdapter::notDone()
{

	if (_msg.empty())
		return false;

	if (_neighbor)
		return false;

	else
		return true;

}

}

}
