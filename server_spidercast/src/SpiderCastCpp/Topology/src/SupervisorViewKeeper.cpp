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
 * SupervisorViewKeeper.cpp
 *
 *  Created on: 09/02/2011
 *      Author:
 *
 * 	Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: SupervisorViewKeeper.cpp,v $
 * Revision 1.3  2016/11/27 11:28:33 
 * macros fro trace
 *
 * Revision 1.2  2015/11/18 08:36:58 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:14 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.10  2015/01/25 07:32:49 
 * Retain attributes on node leave
 *
 * Revision 1.9  2012/10/25 10:11:11 
 * Added copyright and proprietary notice
 *
 * Revision 1.8  2012/02/14 12:44:19 
 * remove comment. no functional changes
 *
 * Revision 1.7  2011/03/31 14:02:43 
 * Throw runTimeError in order to catch NPes in action
 *
 * Revision 1.6  2011/02/28 18:14:34 
 * Implement foreign zone remote requests
 *
 * Revision 1.5  2011/02/21 09:47:51 
 * Refactoring: NodeIDimpl_SPtr to NodeID_SPtr
 *
 * Revision 1.4  2011/02/21 07:55:49 
 * Add includeAttributes to the supervisor attributes
 *
 * Revision 1.3  2011/02/20 12:01:02 
 * Got rid of ConcurrentSharedPointer in API & NodeID
 *
 * Revision 1.2  2011/02/16 20:08:34 
 * foreign zone membership task - take 1
 *
 * Revision 1.1  2011/02/10 13:29:01 
 * Initial version
 *
 */

#include "SupervisorViewKeeper.h"

namespace spdr
{

ScTraceComponent* SupervisorViewKeeper::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Topo,
		trace::ScTrConstants::Layer_ID_TEST, "SupervisorViewKeeper",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

SupervisorViewKeeper::SupervisorViewKeeper(String myNodeName) :
	ScTraceContext(_tc, "", myNodeName), _myNodeName(myNodeName),
			_baseZoneView()
{
	Trace_Entry(this, "SupervisorCViewKeeper()");
}

SupervisorViewKeeper::~SupervisorViewKeeper()
{
	Trace_Entry(this, "~SupervisorCViewKeeper()");
}

void SupervisorViewKeeper::onMembershipEvent(const SCMembershipEvent& event)
{
	TRACE_ENTRY2(this, "onMembershipEvent()", event.toString());
// TODO: add timing issues
	switch (event.getType())
	{
	case SCMembershipEvent::View_Change:
	{
		//Assumes this is the first event, that comes only once
		boost::recursive_mutex::scoped_lock lock(_mutex);
		if (_baseZoneView)
		{
			String errMsg("!_baseZoneView->empty() before");
			Trace_Event(this, "onMembershipEvent()", errMsg);
			throw SpiderCastRuntimeError(errMsg);
		}

		_baseZoneView = event.getView();

		if (!_baseZoneView || _baseZoneView->empty())
		{
			String errMsg("_baseZoneView->empty() r NULL after");
			Trace_Event(this, "onMembershipEvent()", errMsg);
			throw SpiderCastRuntimeError(errMsg);
		}
	}
		break;

	case SCMembershipEvent::Node_Leave:
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		if (!_baseZoneView)
		{
			String errMsg("!_baseZoneView");
			Trace_Event(this, "onMembershipEvent()", errMsg);
			throw SpiderCastRuntimeError(errMsg);
		}
		int removed = _baseZoneView->erase(event.getNodeID());
		if (removed != 1)
		{
			ScTraceBufferAPtr tb = ScTraceBuffer::event(this,
					"onMembershipEvent()",
					"Warning - node leave, but not in the map");
			tb->invoke();
		}
	}
		break;

	case SCMembershipEvent::Change_of_Metadata:
	{
		SCViewMap_SPtr view = event.getView();

		if (!view)
		{
			String what(
					"Change_of_Metadata, received NULL view. going to explode");
			Trace_Event(this, "onMembershipEvent", what);
			throw SpiderCastRuntimeError(what);
		}

		{
			boost::recursive_mutex::scoped_lock lock(_mutex);

			for (SCViewMap::iterator it = view->begin(); it != view->end(); ++it)
			{
				if (!_baseZoneView)
				{
					String
							what(
									"Change_of_Metadata, WARNING _baseZoneView is NULL");
					Trace_Event(this, "onMembershipEvent", what);
					throw SpiderCastRuntimeError(what);
				}

				SCViewMap::iterator res = _baseZoneView->find(it->first);
				if (res != _baseZoneView->end())

				{
					res->second = it->second;
				}
				else
				{
					ScTraceBufferAPtr tb = ScTraceBuffer::event(this,
							"onMembershipEvent()",
							"Warning - change of metadata, but not in the map");
					tb->invoke();
				}
			}
		}
	}
		break;

	case SCMembershipEvent::Node_Join:
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);

		if (!_baseZoneView)
		{
			String errMsg("!_baseZoneView");
			Trace_Event(this, "onMembershipEvent()", errMsg);
			throw SpiderCastRuntimeError(errMsg);
		}

		if (!_baseZoneView->insert(std::make_pair(event.getNodeID(),
				event.getMetaData())).second)
		{
			ScTraceBufferAPtr tb = ScTraceBuffer::event(this,
					"onMembershipEvent()",
					"Warning - node join, but already in the map");
			tb->invoke();
		}
	}
		break;

	default:
	{

		String errMsg("Unexpected event type in onMembershipEvent()");
		Trace_Event(this, "onMembershipEvent()", errMsg);
		throw SpiderCastRuntimeError(errMsg);

	}
	}

	Trace_Exit(this, "onMembershipEvent()");
}

SCViewMap_SPtr SupervisorViewKeeper::getSCView() const
{
	Trace_Entry(this, "getSCView()");

	SCViewMap_SPtr copy;

	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		if (_baseZoneView)
		{
			copy.reset(new SCViewMap);
			copy->insert(_baseZoneView->begin(), _baseZoneView->end());
		}
	}

	Trace_Exit(this, "getSCView()");
	return copy;
}

// typedef std::map< NodeID_SPtr, MetaData_SPtr, NodeID::SPtr_Less > 	ViewMap;
//typedef boost::unordered_map< NodeIDImpl_SPtr, event::MetaData_SPtr, NodeIDImpl_SPtr::Hash, NodeIDImpl_SPtr::Equals > 	SCViewMap;
event::ViewMap_SPtr SupervisorViewKeeper::getView(bool includeAttributes)
{
	using namespace event;

	Trace_Entry(this, "getView()");

	ViewMap_SPtr copy = ViewMap_SPtr(new ViewMap());
	SCViewMap::iterator iter;
	{
		boost::recursive_mutex::scoped_lock lock(_mutex);
		for (iter = _baseZoneView->begin(); iter != _baseZoneView->end(); iter++)
		{
			if (includeAttributes)
			{
				(*copy)[iter->first] = iter->second;
			}
			else
			{
				(*copy)[iter->first] = MetaData_SPtr(new MetaData(
						AttributeMap_SPtr(),
						iter->second->getIncarnationNumber(),
						iter->second->getNodeStatus()));
			}
		}
	}
	Trace_Exit(this, "getView()", event::ViewChangeEvent::viewMapToString(copy));
	return copy;
}

boost::shared_ptr<event::ViewMap> SupervisorViewKeeper::convertSCtoViewMap(
		SCViewMap_SPtr incomingView)
{
	using namespace event;

	ViewMap_SPtr copy = ViewMap_SPtr(new ViewMap());
	SCViewMap::iterator iter;
	for (iter = incomingView->begin(); iter != incomingView->end(); iter++)
	{

		(*copy)[iter->first] = iter->second;

	}
	return copy;
}

int SupervisorViewKeeper::getViewSize() const
{
	Trace_Entry(this, "getViewSize()");

	int ret = -1;

	boost::recursive_mutex::scoped_lock lock(_mutex);

	if (_baseZoneView)
	{
		ret = _baseZoneView->size();
	}

	Trace_Exit<int> (this, "getViewSize()", ret);

	return ret;
}
}
