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
 * HighPriorityMonitor.cpp
 *
 *  Created on: May 7, 2012
 */

#include "HighPriorityMonitor.h"

namespace spdr
{


ScTraceComponent* HighPriorityMonitor::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership, "HighPriorityMonitor",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HighPriorityMonitor::HighPriorityMonitor(
		const String& instID,
		const SpiderCastConfigImpl& config,
		CommAdapter_SPtr commAdapter) :
		ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
		hpmKey_(".hpm"),
		commAdapter_(commAdapter),
		highPriorityMonitors_()
{
	Trace_Entry(this, "HighPriorityMonitor()");
}

HighPriorityMonitor::~HighPriorityMonitor()
{
	Trace_Entry(this, "~HighPriorityMonitor()");
}


/*
 * @see spdr::SCMembershipListener
 */
void HighPriorityMonitor::onMembershipEvent(const SCMembershipEvent& event)
{
	using namespace event;

	TRACE_ENTRY3(this, "onMembershipEvent()", "SCMembershipEvent",
			event.toString());

	bool hpm_view_changed = false;

	switch (event.getType())
	{
	case SCMembershipEvent::View_Change:
	{
		//Assumes this is the first event, that comes only once
		highPriorityMonitors_.clear();

		SCViewMap_SPtr view = event.getView();
		for (SCViewMap::iterator it = view->begin(); it != view->end(); ++it)
		{
			//put every node with .hpm attribute in the view
			if (it->second) //MetaData valid
			{
				AttributeMap_SPtr attr_map_SPtr = it->second->getAttributeMap();
				if (attr_map_SPtr)
				{
					bool hpm = attr_map_SPtr->count(hpmKey_) > 0;

					if (hpm)
					{
						highPriorityMonitors_.insert(it->first);
						hpm_view_changed = true;
					}
				}
			}
		}
	}
		break;

	case SCMembershipEvent::Node_Join:
	{
		if (event.getMetaData()) //MetaData valid
		{
			AttributeMap_SPtr attr_map_SPtr =
					event.getMetaData()->getAttributeMap();
			if (attr_map_SPtr)
			{
				bool hpm = attr_map_SPtr->count(hpmKey_) > 0;

				if (hpm)
				{
					highPriorityMonitors_.insert(event.getNodeID());
					hpm_view_changed = true;
				}
			}
		}
	}
		break;

	case SCMembershipEvent::Node_Leave:
	{
		hpm_view_changed = highPriorityMonitors_.erase(event.getNodeID()) > 0;
	}
		break;

	case SCMembershipEvent::Change_of_Metadata:
	{
		SCViewMap_SPtr view = event.getView();
		for (SCViewMap::iterator it = view->begin(); it != view->end(); ++it)
		{
			if (it->second) //MetaData valid
			{
				AttributeMap_SPtr attr_map_SPtr = it->second->getAttributeMap();
				if (attr_map_SPtr)
				{
					bool hpm = attr_map_SPtr->count(hpmKey_) > 0;

					if (hpm)
					{
						std::pair<NodeIDSet::iterator,bool> res = highPriorityMonitors_.insert(it->first);
						hpm_view_changed = res.second;
					}
					else
					{
						if (highPriorityMonitors_.erase(it->first) > 0)
						{
							hpm_view_changed = true;
						}
					}
				}
				else
				{
					if (highPriorityMonitors_.erase(it->first) > 0)
					{
						hpm_view_changed = true;
					}
				}
			}
			else
			{
				if (highPriorityMonitors_.erase(it->first) > 0)
				{
					hpm_view_changed = true;
				}
			}
		}
	}
		break;

	default:
	{
		String what("unexpected event type: ");
		what.append(event.toString());
		throw SpiderCastRuntimeError(what);
	}
	}

	if (hpm_view_changed)
	{

		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this,
					"onMembershipEvent()", "HPM view changed");
			buffer->addProperty<std::size_t>("size", highPriorityMonitors_.size());

			std::ostringstream oss;
			oss << "{";
			for (NodeIDSet::const_iterator it = highPriorityMonitors_.begin();
					it != highPriorityMonitors_.end(); ++it)
			{
				oss << it->get()->getNodeName() << " ";
			}
			oss << "}";

			buffer->addProperty("view", oss.str());
			buffer->invoke();
		}
	}

	Trace_Exit(this, "onMembershipEvent()");
}

void HighPriorityMonitor::send2Monitors(SCMessage_SPtr message)
{
	Trace_Entry(this, "send2Monitors()");

	int succ = 0;
	for (NodeIDSet::const_iterator it = highPriorityMonitors_.begin();
						it != highPriorityMonitors_.end(); ++it)
	{
		NodeIDImpl_SPtr target = boost::static_pointer_cast<NodeIDImpl>(*it);
		bool ok = commAdapter_->sendTo(target,message);

		Trace_Debug(this, "send2Monitors()", (ok?"OK":"Failed"),
				"target", spdr::toString<NodeIDImpl>(target));

		if (ok)
		{
			succ++;
		}
	}

	if (ScTraceBuffer::isEventEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,
				"send2Monitors()", "Sent");

		buffer->addProperty<int>("sent", succ);
		buffer->addProperty<std::size_t>("hpm-size", highPriorityMonitors_.size());
		buffer->invoke();
	}

	Trace_Exit(this, "send2Monitors()");
}

}
