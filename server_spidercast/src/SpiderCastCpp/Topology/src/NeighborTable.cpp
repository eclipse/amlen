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
 * NeighborTable.cpp
 *
 *  Created on: 15/03/2010
 *      Author:
 *
 * Version     : $Revision: 1.6 $
 *-----------------------------------------------------------------
 * $Log: NeighborTable.cpp,v $
 * Revision 1.6  2016/02/08 12:49:08 
 * after merge with branch Nameless_TCP_Discovery
 *
 * Revision 1.5.2.3  2016/02/07 19:51:47 
 * trace better
 *
 * Revision 1.5.2.2  2016/01/27 09:06:23 
 * trace better
 *
 * Revision 1.5.2.1  2016/01/21 14:52:47 
 * add senderLocalName on every incoming message, as attached to the connection
 *
 * Revision 1.5  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.4  2015/10/09 06:56:11 
 * bug fix - when neighbor in the table but newNeighbor not invoked yet, don't send to all neighbors but only to routable neighbors. this way the node does not get a diff mem/metadata before the full view is received.
 *
 * Revision 1.3  2015/08/06 06:59:15 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.2  2015/07/13 08:26:21 
 * *** empty log message ***
 *
 * Revision 1.1  2015/03/22 12:29:14 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.28  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.27  2012/06/27 12:38:57 
 * Seperate generic COMM from RUM
 *
 * Revision 1.26  2012/02/19 09:07:11 
 * Add a "routbale" field to the neighbor tables
 *
 * Revision 1.25  2011/08/07 12:14:12 
 * Obtain neighbors names
 *
 * Revision 1.24  2011/05/30 17:45:58 
 * Streamline mutex locking
 *
 * Revision 1.23  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.22  2010/12/22 12:09:37 
 * Enhanced tracing; no functional change
 *
 * Revision 1.21  2010/12/01 18:53:56 
 * Trace fix; no functional changes
 *
 * Revision 1.20  2010/10/06 10:29:30 
 * Bug fix - (1) send to all when 0 neighbors undetected, (2) increase version when discovered
 *
 * Revision 1.19  2010/08/18 18:54:24 
 * renamed method sent -> send
 *
 * Revision 1.18  2010/08/17 13:30:21 
 * Add a couple of additional checks on the ta validity
 *
 * Revision 1.17  2010/08/16 14:37:30 
 * throw when key does not match value
 *
 * Revision 1.16  2010/08/16 13:38:40 
 * *** empty log message ***
 *
 * Revision 1.15  2010/08/16 13:21:05 
 * *** empty log message ***
 *
 * Revision 1.14  2010/08/16 11:24:34 
 * *** empty log message ***
 *
 * Revision 1.13  2010/08/12 15:31:39 
 * Use correct table classes on all iterators
 *
 * Revision 1.12  2010/08/10 09:52:43 
 * Add toString()
 *
 * Revision 1.11  2010/07/08 09:53:29 
 * Switch from cout to trace
 *
 * Revision 1.10  2010/07/01 11:55:47 
 * Rename NeighborTable::exists() to contains()
 *
 * Revision 1.9  2010/06/21 13:46:10 
 * In getNeighbor ensure that the neighbor is valid before accessing it (in this case for printing purposes)
 *
 * Revision 1.8  2010/06/14 15:55:30 
 * Change insert to change the value if the key already exists
 *
 * Revision 1.7  2010/06/08 10:33:17 
 * Add a size() method
 *
 * Revision 1.6  2010/06/06 12:47:17 
 * Add table name; no functional chage
 *
 * Revision 1.5  2010/06/03 14:43:20 
 * Arrange print statmenets
 *
 * Revision 1.4  2010/05/30 15:34:15 
 * getNeighbor to return non-valid CSP rather than throw exception
 *
 * Revision 1.3  2010/05/27 09:34:59 
 * Topology initial implementation
 *
 * Revision 1.2  2010/05/20 09:43:24 
 * Initial version
 *
 *
 */

#include <boost/lexical_cast.hpp>
#include "NeighborTable.h"

using namespace std;

namespace spdr
{

using namespace trace;

ScTraceComponent* NeighborTable::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Topo,
		trace::ScTrConstants::Layer_ID_Topology, "NeighborTable",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

NeighborTable::Value::Value() : neighbor(), routable(false)
{
}

NeighborTable::Value::Value(Neighbor_SPtr neighbor, bool routable) :
		neighbor(neighbor), routable(routable)
{
}

NeighborTable::NeighborTable(String myName, String tableName,
		const String& instID) :
	ScTraceContext(_tc, instID, myName), _myNodeName(myName), _tableName(
			tableName), _instID(instID)
{
}

NeighborTable::~NeighborTable()
{
}

bool NeighborTable::addEntry(NodeIDImpl_SPtr targetName, Neighbor_SPtr neighbor)
{
	Trace_Entry(this, "addEntry()", "TableName", _tableName, "adding",
			targetName->getNodeName());

	bool res = true;

	if (neighbor && targetName->getNodeName().compare(
			neighbor->getName()))
	{
		String what =
				"Inconsistent insertion to NeighborTable - nodeId name and neighbor name not equal";
		Trace_Event(this, "addEntry()", what, "TableName", _tableName,
				"NodeID", targetName->getNodeName(), "Neighbor",
				neighbor->getName());
		throw SpiderCastRuntimeError(what);
	}

	boost::recursive_mutex::scoped_lock lock(_mutex);
	NeighborTableMap::iterator iter = _table.find(targetName);
	if (iter != _table.end())
	{
		iter->second = Value(neighbor, false);
	}
	else
	{
		if (targetName->getNetworkEndpoints().getNumAddresses() == (size_t) 0)
		{
			String what = "Bogus NodeIDImpl insertion to NeighborTable, no endpoints";
			Trace_Event(this, "addEntry()", what, "TableName", _tableName,
					"adding", targetName->getNodeName());
			//throw SpiderCastRuntimeError(what);
		}
		res = _table.insert(make_pair(targetName, Value(neighbor, false))).second;
	}
	Trace_Exit<bool> (this, "addEntry()", res);
	return res;
}

Neighbor_SPtr NeighborTable::getNeighbor(
		NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "getNeighbor()", "TableName", _tableName, "asking for",
			targetName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(_mutex);
	NeighborTableMap::iterator iter = _table.find(targetName);
	if (iter == _table.end())
	{
		Trace_Debug(this, "getNeighbor", "could not find corresponding entry",
				"target", spdr::toString(targetName));

		return Neighbor_SPtr ();
	}
	else
	{
		ostringstream oss;
		oss << "returning target: "
				<< (iter->second.neighbor ? iter->second.neighbor->getName()
						: "") << "; cid: "
				<< (iter->second.neighbor ? iter->second.neighbor->getConnectionId()
						: 0) << "; tx sid: "
				<< (iter->second.neighbor ? iter->second.neighbor->getSid()
						: 0);

		if ((iter->second.neighbor)
				&& (targetName->getNodeName().compare(
						(iter->second.neighbor)->getName()))
				&& (iter->first->getNodeName().compare(
						iter->second.neighbor->getName())))
		{
			String what =
					"Error: Bogus entry in NeighborTable - nodeId name and neighbor name not equal";
			Trace_Error(this, "getNeighbor()", what,
					"TableName", _tableName,
					"NodeID", targetName->getNodeName(),
					"Neighbor", (iter->second.neighbor)->getName(),
					"Stored NodeId", iter->first->getNodeName());
			throw SpiderCastRuntimeError(what);
		}

		Trace_Debug(this, "getNeighbor()", oss.str());

		return iter->second.neighbor;
	}
}

bool NeighborTable::removeEntry(NodeIDImpl_SPtr targetName)
{

	Trace_Entry(this, "removeEntry()", "TableName", _tableName, "removing",
			targetName->getNodeName());
	boost::recursive_mutex::scoped_lock lock(_mutex);

	bool res = _table.erase(targetName);

	Trace_Exit<bool> (this, "removeEntry()", res);
	return res;
}

bool NeighborTable::contains(NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "contains()", "TableName", _tableName, "asking for",
			targetName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(_mutex);

	NeighborTableMap::iterator iter = _table.find(targetName);

	bool res = iter != _table.end();

	if (res && (iter->second.neighbor)
			&& (targetName->getNodeName().compare(
					(iter->second.neighbor)->getName()))
			&& (iter->first->getNodeName().compare(
					iter->second.neighbor->getName())))
	{
		String what =
				"Error: Bogus entry in NeighborTable - nodeId name and neighbor name not equal";
		Trace_Error(this, "addEntry()", what,
				"TableName", _tableName,
				"NodeID", targetName->getNodeName(),
				"Neighbor",	(iter->second.neighbor)->getName(),
				"Stored NodeId", iter->first->getNodeName());

		throw SpiderCastRuntimeError(what);
	}

	Trace_Exit(this, "contains(); TableName", _tableName, ScTraceBuffer::stringValueOf(res));

	return res;
}

int NeighborTable::size()
{
	boost::recursive_mutex::scoped_lock lock(_mutex);
	int size = (int) _table.size();

	Trace_Dump(this, "size()", _tableName, "size", spdr::stringValueOf(size));

	return size;
}

String NeighborTable::toString() const
{
	boost::recursive_mutex::scoped_lock lock(_mutex);
	ostringstream oss;

	NeighborTable::NeighborTableMap::const_iterator iter;
	for (iter = _table.begin(); iter != _table.end(); ++iter)
	{
		oss << (iter->first)->getNodeName() << "; ";

		if ((iter->second.neighbor)
				&& (iter->first->getNodeName().compare(
						iter->second.neighbor->getName())))
		{
			String what = "Error: Bogus entry in NeighborTable - stored nodeId name and neighbor name not equal";
			Trace_Error(this, "toString()", what,
					"TableName", _tableName,
					"Neighbor", (iter->second.neighbor)->getName(),
					"Stored NodeId", iter->first->getNodeName());

			throw SpiderCastRuntimeError(what);
		}
	}

	return oss.str();
}


String NeighborTable::toStringDump() const
{
	boost::recursive_mutex::scoped_lock lock(_mutex);
	ostringstream oss;

	NeighborTable::NeighborTableMap::const_iterator iter;
	for (iter = _table.begin(); iter != _table.end(); ++iter)
	{
		oss << (iter->first)->getNodeName() << " RT=" << iter->second.routable << ", " << spdr::toString(iter->second.neighbor) << "; ";
	}

	return oss.str();
}

String NeighborTable::getNeighborsStr()
{
	bool empty = true;
	String names;

	NeighborTable::NeighborTableMap::iterator iter;
	for (iter = _table.begin(); iter != _table.end(); ++iter)
	{
		names = names + (iter->first)->getNodeName() + " ";
		empty = false;
	}
	if (empty)
	{
		names = ("empty");
	}
	return names;
}

/**
 * Send a message to the target
 * @param target
 * @param msg
 * @return true if success, false if failed / not found in neighbor table
 */
bool NeighborTable::sendToNeighbor(NodeIDImpl_SPtr target, SCMessage_SPtr msg)
{
	Trace_Entry(this, "sendToNeighbor()");

	bool success = false;

	boost::recursive_mutex::scoped_lock lock(_mutex);

	NeighborTable::NeighborTableMap::iterator it = _table.find(target);

	if (it != _table.end())
	{
		int rc = (it->second.neighbor)->sendMessage(msg);
		if (rc == 0)
		{
			Trace_Debug(this, "sendToNeighbor()", "sent",
					"target", NodeIDImpl::stringValueOf(target),
					"msg", msg->toString());
			success = true;
		}
		else
		{
			std::ostringstream oss;
			oss << "send failed, rc=" << rc;
			Trace_Debug(this, "sendToNeighbor()", oss.str(),
					"target", NodeIDImpl::stringValueOf(target),
					"msg", msg->toString());

		}
	}
	else
	{
		Trace_Debug(this, "sendToNeighbor()", "failed, neighbor not found",
				"target", NodeIDImpl::stringValueOf(target),
				"msg", msg->toString());
	}

	Trace_Exit<bool> (this, "sendToNeighbor()", success);
	return success;
}

std::pair<int, int> NeighborTable::sendToAllNeighbors(SCMessage_SPtr msg)
{
	Trace_Entry(this, "sendToAllNeighbors()");

	int failed = 0;
	int success = 0;
	NeighborTableMap activeNeighbors;
	NeighborTable::NeighborTableMap::iterator it;

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		for (it = _table.begin(); it != _table.end(); ++it)
		{
			activeNeighbors.insert((*it));
		}
	}

	for (it = activeNeighbors.begin(); it != activeNeighbors.end(); ++it)
	{
		int rc = (it->second.neighbor)->sendMessage(msg); //TODO try/catch?
		if (rc != 0)
		{
			failed++;
			std::ostringstream oss;
			oss << "send to 1 neighbor failed, rc=" << rc;
			Trace_Debug(this, "sendToAllNeighbors()", oss.str(),
					"target", NodeIDImpl::stringValueOf(it->first),
					"msg", msg->toString());
		}
		else
		{
			success++;
		}

	}

	std::pair<int, int> res = std::make_pair(success, success + failed);

	std::ostringstream succ_vs_total;
	succ_vs_total << success << "/" << (success + failed);

	Trace_Debug(this, "sendToAllNeighbors()", succ_vs_total.str(), "msg",
			msg->toString());

	Trace_Exit(this, "sendToAllNeighbors()");

	return res;
}


std::pair<int, int> NeighborTable::sendToAllRoutableNeighbors(SCMessage_SPtr msg)
{
	Trace_Entry(this, "sendToAllRoutableNeighbors()");

	int failed = 0;
	int success = 0;
	NeighborTableMap routableNeighbors;
	NeighborTable::NeighborTableMap::iterator it;

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		for (it = _table.begin(); it != _table.end(); ++it)
		{
			if (it->second.routable)
				routableNeighbors.insert((*it));
		}
	}

	for (it = routableNeighbors.begin(); it != routableNeighbors.end(); ++it)
	{
		int rc = (it->second.neighbor)->sendMessage(msg); //TODO try/catch?
		if (rc != 0)
		{
			failed++;
			std::ostringstream oss;
			oss << "send to 1 neighbor failed, rc=" << rc;
			Trace_Debug(this, "sendToAllRoutableNeighbors()", oss.str(),
					"target", NodeIDImpl::stringValueOf(it->first),
					"msg", msg->toString());
		}
		else
		{
			success++;
		}

	}

	std::pair<int, int> res = std::make_pair(success, success + failed);

	std::ostringstream succ_vs_total;
	succ_vs_total << success << "/" << (success + failed);

	Trace_Debug(this, "sendToAllRoutableNeighbors()", succ_vs_total.str(), "msg",
			msg->toString());
	Trace_Dump(this, "sendToAllRoutableNeighbors()", "",
				"#routable", boost::lexical_cast<std::string>(routableNeighbors.size()),
				"table", toStringDump());

	Trace_Exit(this, "sendToAllRoutableNeighbors()");

	return res;
}

void NeighborTable::setRoutable(NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "setRoutable()", "TableName", _tableName, "asking for",
			targetName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(_mutex);
	NeighborTableMap::iterator iter = _table.find(targetName);
	if (iter == _table.end())
	{
		//should never happen
		String desc("Error: setRoutabele could not find corresponding entry");
		Trace_Error(this, "setRoutable()",desc, "target", NodeIDImpl::stringValueOf(targetName));
		String what("NeighborTable::setRoutable ");
		what.append(desc);
		throw SpiderCastRuntimeError(what);
	}

	else
	{
		iter->second.routable = true;
	}

}

bool NeighborTable::getRoutable(NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "getRoutable()", "TableName", _tableName, "asking for",
			targetName->getNodeName());

	boost::recursive_mutex::scoped_lock lock(_mutex);
	NeighborTableMap::iterator iter = _table.find(targetName);
	if (iter == _table.end())
	{
		Trace_Debug(this, "getRoutable", "could not find corresponding entry",
				"target", NodeIDImpl::stringValueOf(targetName));

		return false;
	}
	else
	{
		return iter->second.routable;
	}

}

}
