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
 * SupervisorNeighborTable.cpp
 *
 *  Created on: Feb 2, 2011
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: SupervisorNeighborTable.cpp,v $
 * Revision 1.2  2015/11/18 08:36:58 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:14 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.10  2012/10/25 10:11:11 
 * Added copyright and proprietary notice
 *
 * Revision 1.9  2012/07/17 09:19:05 
 * add pub/sub bridges
 *
 * Revision 1.8  2011/05/30 17:46:05 
 * Streamline mutex locking
 *
 * Revision 1.7  2011/03/31 14:01:22 
 * Make a single active delegate. Turn SupervisorViewKeeper into a shared pointer. Initialize supervisorViewKeeper whenever a new active delegate is chosen
 *
 * Revision 1.6  2011/02/28 18:14:10 
 * Implement foreign zone remote requests
 *
 * Revision 1.5  2011/02/21 07:55:49 
 * Add includeAttributes to the supervisor attributes
 *
 * Revision 1.4  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.3  2011/02/16 20:08:34 
 * foreign zone membership task - take 1
 *
 * Revision 1.2  2011/02/10 13:28:54 
 * Added supervisor view keeper issues
 *
 * Revision 1.1  2011/02/07 14:58:21 
 * Initial version
 *
 */

#include "SupervisorNeighborTable.h"

namespace spdr
{

SupervisorNeighborTable::SupervisorNeighborTable(String myName,
		String tableName, const String& instID) :
	NeighborTable(myName, tableName, instID), _myName(myName), _activeDelegate()
{

	_viewKeeper = boost::shared_ptr<SupervisorViewKeeper>(new SupervisorViewKeeper(_myName));
}

SupervisorNeighborTable::~SupervisorNeighborTable()
{
	// TODO Auto-generated destructor stub
}

int SupervisorNeighborTable::getNumActiveDelegates()
{
	//return _activeDelegate.size();
	return (_activeDelegate ? 1 : 0);
}

NodeIDImpl_SPtr SupervisorNeighborTable::getAnActiveDelegateCandidate()
{
	Trace_Entry(this, "getAnActiveDelegateCandidate");
	NodeIDImpl_SPtr ret;

	boost::recursive_mutex::scoped_lock lock(_mutex);

	NeighborTable::NeighborTableMap::iterator iter;
	for (iter = _table.begin(); iter != _table.end(); ++iter)
	{
		if (_activeDelegate)
		{
			if ((*_activeDelegate) != (*iter->first))
			{
				ret = iter->first;
				break;
			}
		}
		else
		{
			ret = iter->first;
			break;
		}
	}

	Trace_Exit(this, "getAnActiveDelegateCandidate", "returning",
			NodeIDImpl::stringValueOf(ret));
	return ret;

}

void SupervisorNeighborTable::setActiveDelegate(NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "setActiveDelegate",
			NodeIDImpl::stringValueOf(targetName));

	boost::recursive_mutex::scoped_lock lock(_mutex);
	_activeDelegate = targetName;
	_viewKeeper = boost::shared_ptr<SupervisorViewKeeper>(new SupervisorViewKeeper(_myName));

	Trace_Exit(this, "setActiveDelegate");
}
bool SupervisorNeighborTable::setInactiveDelegate(NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "setInactiveDelegate", NodeIDImpl::stringValueOf(
			targetName));

	bool hit = false;

	boost::recursive_mutex::scoped_lock lock(_mutex);
	if (_activeDelegate)
	{
		if ((*_activeDelegate) == (*targetName))
		{
			_activeDelegate = NodeIDImpl_SPtr();
			hit = true;
		}
	}

	Trace_Exit<bool>(this, "setInactiveDelegate",hit);
	return hit;
}

void SupervisorNeighborTable::processViewEvent(const SCMembershipEvent& event)
{
	Trace_Entry(this, "processViewEvent");

	_viewKeeper->onMembershipEvent(event);

	Trace_Exit(this, "processViewEvent");
}

int SupervisorNeighborTable::getViewSize()
{
	Trace_Entry(this, "getViewSize");

	int size =  _viewKeeper->getViewSize();

	Trace_Exit<int> (this, "getViewSize", size);

	return size;
}

SCViewMap_SPtr SupervisorNeighborTable::getSCView() const
{
	Trace_Entry(this, "getSCView");

	return _viewKeeper->getSCView();
}

boost::shared_ptr<event::ViewMap> SupervisorNeighborTable::getView(
		bool includeAttributes)
{
	Trace_Entry(this, "getView");

	return _viewKeeper->getView(includeAttributes);

}

bool SupervisorNeighborTable::hasActiveDelegate()
{
	Trace_Entry(this, "hasActiveDelegate");

	bool ret = (_activeDelegate ? true : false);

	Trace_Exit<bool> (this, "hasActiveDelegate", ret);

	return ret;
}

bool SupervisorNeighborTable::isActiveDelegate(NodeIDImpl_SPtr targetName)
{
	Trace_Entry(this, "isActiveDelegate");

	bool ret = _activeDelegate ? ((*_activeDelegate) == (*targetName)) : false;

	Trace_Exit<bool> (this, "isActiveDelegate", ret);

	return ret;
}

}
