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
 * OutgoingStructuredNeighborTable.cpp
 *
 *  Created on: Feb 1, 2012
 *      Author:
 *
 *       Version     : $Revision: 1.4 $
 *-----------------------------------------------------------------
 * $Log: OutgoingStructuredNeighborTable.cpp,v $
 * Revision 1.4  2015/11/18 08:36:58 
 * Update copyright headers
 *
 * Revision 1.3  2015/08/06 06:59:15 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.2  2015/07/02 10:29:12 
 * clean trace, byte-buffer
 *
 * Revision 1.1  2015/03/22 12:29:14 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:11 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2012/02/19 09:07:12 
 * Add a "routbale" field to the neighbor tables
 *
 * Revision 1.1  2012/02/14 12:45:18 
 * Add structured topology
 *
 */

#include "OutgoingStructuredNeighborTable.h"

using namespace std;

namespace spdr
{

using namespace trace;

ScTraceComponent* OutgoingStructuredNeighborTable::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Topo,
		trace::ScTrConstants::Layer_ID_Topology,
		"OutgoingStructuredNeighborTable",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

OutgoingStructuredNeighborTable::OutgoingStructuredNeighborTable(String myName,
		String tableName, const String& instID) :
	ScTraceContext(_tc, instID, myName), _myNodeName(myName), _tableName(
			tableName), _instID(instID)
{

}

OutgoingStructuredNeighborTable::~OutgoingStructuredNeighborTable()
{
	// TODO Auto-generated destructor stub
}

bool OutgoingStructuredNeighborTable::addEntry(NodeIDImpl_SPtr targetName,
		Neighbor_SPtr neighbor, int viewSize)
{
	Trace_Entry(this, "addEntry()", "TableName", _tableName, "adding",
			targetName->getNodeName());

	bool res = true;

	if (neighbor && targetName->getNodeName().compare(
			neighbor->getName()))
	{
		String what =
				"Bogus insertion to NeighborTable - nodeId name and neighbor name not equal";
		Trace_Event(this, "addEntry()", what, "TableName", _tableName,
				"NodeID", targetName->getNodeName(), "Neighbor",
				neighbor->getName());
		throw SpiderCastRuntimeError(what);
	}

	boost::recursive_mutex::scoped_lock lock(_mutex);
	OutgoingStructuredNeighborTableMap::iterator iter = _table.find(targetName);
	if (iter != _table.end())
	{
		if (viewSize != -1)
		{
			Trace_Error(this, "addEntry()", "Error: view-size must be -1 at this point.");
			throw SpiderCastRuntimeError("Error: OutgoingStructuredNeighborTable::addEntry view-size must be -1 at this point.");
			//assert(viewSize == -1);
		}
		iter->second = boost::tuples::make_tuple(neighbor,
				iter->second.get<1> (), false);
	}
	else
	{
		res = _table.insert(make_pair(targetName, boost::tuples::make_tuple(
				neighbor, viewSize, false))).second;
	}
	Trace_Exit<bool> (this, "addEntry()", res);
	return res;

}

Neighbor_SPtr OutgoingStructuredNeighborTable::getNeighbor(
		NodeIDImpl_SPtr targetName)
{

	Trace_Entry(this, "getNeighbor()", "TableName", _tableName, "asking for",
			targetName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(_mutex);
	OutgoingStructuredNeighborTableMap::iterator iter = _table.find(targetName);
	if (iter == _table.end())
	{
		Trace_Event(this, "getNeighbor", "could not find corresponding entry",
				"", "");

		return Neighbor_SPtr ();
	}
	else
	{
		return iter->second.get<0> ();
	}

}

int OutgoingStructuredNeighborTable::getViewSize(NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "getViewSize()", "TableName", _tableName, "asking for",
			targetName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(_mutex);
	OutgoingStructuredNeighborTableMap::iterator iter = _table.find(targetName);
	if (iter == _table.end())
	{
		Trace_Event(this, "getNeighbor", "could not find corresponding entry",
				"", "");

		return -1;
	}
	else
	{
		return iter->second.get<1> ();
	}

}

bool OutgoingStructuredNeighborTable::removeEntry(NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "removeEntry()", "TableName", _tableName, "removing",
			targetName->getNodeName());
	boost::recursive_mutex::scoped_lock lock(_mutex);

	bool res = _table.erase(targetName);

	Trace_Exit<bool> (this, "removeEntry()", res);
	return res;

}

bool OutgoingStructuredNeighborTable::contains(NodeIDImpl_SPtr targetName)
{

	Trace_Entry(this, "contains()", "TableName", _tableName, "asking for",
			targetName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(_mutex);

	OutgoingStructuredNeighborTableMap::iterator iter = _table.find(targetName);

	bool res = iter != _table.end();

	ostringstream oss;
	oss << "response: " << res;
	Trace_Exit(this, "contains(); TableName", _tableName, oss.str());

	return res;

}

int OutgoingStructuredNeighborTable::size()
{
	Trace_Entry(this, "size()", "TableName", _tableName);

	boost::recursive_mutex::scoped_lock lock(_mutex);
	int size = (int) _table.size();

	ostringstream oss;
	oss << "size: " << size;
	Trace_Exit(this, "size(); TableName: ", _tableName, oss.str());

	return size;
}

String OutgoingStructuredNeighborTable::toString()
{
	boost::recursive_mutex::scoped_lock lock(_mutex);
	ostringstream oss;

	OutgoingStructuredNeighborTableMap::iterator iter;
	for (iter = _table.begin(); iter != _table.end(); ++iter)
	{
		oss << (iter->first)->getNodeName() << "; ";
	}

	return oss.str();
}

/**
 * Send a message to the target
 * @param target
 * @param msg
 * @return true if success, false if failed / not found in neighbor table
 */
bool OutgoingStructuredNeighborTable::sendToNeighbor(NodeIDImpl_SPtr target,
		SCMessage_SPtr msg)
{
	Trace_Entry(this, "sendToNeighbor()");

	bool success = false;

	boost::recursive_mutex::scoped_lock lock(_mutex);

	OutgoingStructuredNeighborTableMap::iterator it = _table.find(target);

	if (it != _table.end())
	{
		int rc = (it->second.get<0> ())->sendMessage(msg);
		if (rc == 0)
		{
			Trace_Event(this, "sendToNeighbor()", "sent", "target",
					NodeIDImpl::stringValueOf(target), "msg", msg->toString());
			success = true;
		}
		else
		{
			std::ostringstream oss;
			oss << "send failed, rc=" << rc;
			Trace_Event(this, "sendToNeighbor()", oss.str(), "target",
					NodeIDImpl::stringValueOf(target), "msg", msg->toString());

		}
	}
	else
	{
		Trace_Event(this, "sendToNeighbor()", "failed, neighbor not found",
				"target", NodeIDImpl::stringValueOf(target), "msg",
				msg->toString());
	}

	Trace_Exit<bool> (this, "sendToNeighbor()", success);
	return success;
}

bool OutgoingStructuredNeighborTable::refreshNeeded(int viewSize)
{
	Trace_Entry(this, "refreshNeeded");

	bool ret = false;
	boost::recursive_mutex::scoped_lock lock(_mutex);

	OutgoingStructuredNeighborTableMap::iterator iter;
	for (iter = _table.begin(); iter != _table.end(); ++iter)
	{
		if (iter->second.get<1> () > viewSize * 2 || iter->second.get<1> ()
				< viewSize / 2)
		{
			ret = true;
			break;
		}
	}

	Trace_Exit<bool> (this, "refreshNeeded", ret);
	return ret;
}

vector<NodeIDImpl_SPtr> OutgoingStructuredNeighborTable::structuredLinksToRefresh(
		int viewSize)
{
	Trace_Entry(this, "structuredLinksToRefresh");

	vector<NodeIDImpl_SPtr> ret;

	boost::recursive_mutex::scoped_lock lock(_mutex);

	OutgoingStructuredNeighborTableMap::iterator iter;
	for (iter = _table.begin(); iter != _table.end(); ++iter)
	{
		if (iter->second.get<1> () > viewSize * 2 || iter->second.get<1> ()
				< viewSize / 2)
		{
			ret.push_back(iter->first);
		}
	}

	Trace_Exit(this, "structuredLinksToRefresh");
	return ret;

}

void OutgoingStructuredNeighborTable::setRoutable(NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "setRoutabele()", "TableName", _tableName, "asking for",
			targetName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(_mutex);
	OutgoingStructuredNeighborTableMap::iterator iter = _table.find(targetName);
	if (iter == _table.end())
	{
		Trace_Event(this, "setRoutabele", "could not find corresponding entry",
				"", "");
		//should never happen
		String desc("Error: setRoutabele could not find corresponding entry");

		ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "setRoutabele()",
				desc);
		buffer->invoke();

		String what("NeighborTable::setRoutabele ");
		what.append(desc);
		throw SpiderCastRuntimeError(what);
	}

	else
	{
		iter->second = boost::tuples::make_tuple(iter->second.get<0> (), iter->second.get<1> (), true);
	}

}

bool OutgoingStructuredNeighborTable::getRoutable(NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "getRoutable()", "TableName", _tableName, "asking for",
			targetName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(_mutex);
	OutgoingStructuredNeighborTableMap::iterator iter = _table.find(targetName);
	if (iter == _table.end())
	{
		Trace_Event(this, "getRoutable", "could not find corresponding entry",
				"", "");

		return false;
	}
	else
	{
		return iter->second.get<2> ();
	}

}


}
