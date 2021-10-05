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
 * HierarchyViewKeeper.cpp
 *
 *  Created on: Oct 7, 2010
 */

#include "HierarchyViewKeeper.h"

namespace spdr
{
using namespace event;

ScTraceComponent* HierarchyViewKeeper::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy, "HierarchyViewKeeper",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchyViewKeeper::HierarchyViewKeeper(const String& instID,
		const SpiderCastConfigImpl& config,
		HierarchyViewListener& hierarchyManager) :
	ScTraceContext(tc_, instID, config.getMyNodeID()->getNodeName()),
			hierViewVersion_(0),
			hierarchyManager_(hierarchyManager),
			hierarchyView_(),
			delegateView_(),
			supervisorsView_(),
			membershipPush_active_(false),
			membershipPush_Queue_(),
			membershipPush_viewID_(0)
{
	Trace_Entry(this, "HierarchyViewKeeper()");
}

HierarchyViewKeeper::~HierarchyViewKeeper()
{
	Trace_Entry(this, "~HierarchyViewKeeper()");
}

void HierarchyViewKeeper::onMembershipEvent(const SCMembershipEvent& event)
{
	using namespace event;

	TRACE_ENTRY3(this, "onMembershipEvent()", "SCMembershipEvent",	event.toString());

	membershipPush_viewID_++;

	if (membershipPush_active_)
	{
		membershipPush_Queue_.push_back(event);
		hierarchyManager_.globalViewChanged();
	}

	bool hierachyViewChanged = false;

	switch (event.getType())
	{
	case SCMembershipEvent::View_Change:
	{
		//Assumes this is the first event, that comes only once

		hierarchyView_.clear();
		delegateView_.clear();
		supervisorsView_.clear();

		SCViewMap_SPtr view = event.getView();
		for (SCViewMap::iterator it = view->begin(); it != view->end(); ++it)
		{
			//put every node with .hier.* attributes in the hierarchyView
			if (it->second) //MetaData valid
			{
				AttributeMap_SPtr attr_map_SPtr = it->second->getAttributeMap();
				if (attr_map_SPtr)
				{
					AttributeMap_SPtr filteredMap = filter2AttributeMap(
							attr_map_SPtr);
					if (filteredMap && !filteredMap->empty())
					{
						hierarchyView_.insert(std::make_pair(it->first, MetaData_SPtr(
								new MetaData(
										filteredMap,
										it->second->getIncarnationNumber(),
										it->second->getNodeStatus()))));
						hierachyViewChanged = true;

						delegateViewUpdate(it->first, filteredMap);
						supervisorViewUpdate(it->first, filteredMap);
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
				AttributeMap_SPtr filteredMap = filter2AttributeMap(
						attr_map_SPtr);
				if (filteredMap && !filteredMap->empty())
				{
					hierarchyView_.insert( std::make_pair(
									event.getNodeID(),
									MetaData_SPtr( new MetaData(
										filteredMap,
										event.getMetaData()->getIncarnationNumber(),
										event.getMetaData()->getNodeStatus()))));
					hierachyViewChanged = true;
					delegateViewUpdate(event.getNodeID(), filteredMap);
					supervisorViewUpdate(event.getNodeID(), filteredMap);
				}
			}
		}
	}
		break;

	case SCMembershipEvent::Node_Leave:
	{
		hierachyViewChanged = hierarchyView_.erase(event.getNodeID()) > 0;
		delegateViewRemove(event.getNodeID());
		supervisorViewRemove(event.getNodeID());
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
					AttributeMap_SPtr filteredMap = filter2AttributeMap(
							attr_map_SPtr);
					if (filteredMap && !filteredMap->empty())
					{
						MetaData_SPtr filteredMeta(new MetaData(filteredMap,
								it->second->getIncarnationNumber(),
								it->second->getNodeStatus()));
						//insert / replace if changed
						std::pair<SCViewMap::iterator, bool> res =
								hierarchyView_.insert(std::make_pair(it->first,
										filteredMeta));
						if (!res.second)
						{
							if (*(res.first->second) != *(filteredMeta))
							{
								res.first->second = filteredMeta;
								hierachyViewChanged = true;
								delegateViewUpdate(it->first, filteredMap);
								supervisorViewUpdate(it->first, filteredMap);
							}
						}
						else
						{
							hierachyViewChanged = true;
							delegateViewUpdate(it->first, filteredMap);
							supervisorViewUpdate(it->first, filteredMap);
						}
					}
					else
					{
						if (hierarchyView_.erase(it->first) > 0)
						{
							hierachyViewChanged = true;
							delegateViewRemove(it->first);
							supervisorViewRemove(it->first);
						}
					}
				}
				else
				{
					if (hierarchyView_.erase(it->first) > 0)
					{
						hierachyViewChanged = true;
						delegateViewRemove(it->first);
						supervisorViewRemove(it->first);
					}
				}
			}
			else
			{
				if (hierarchyView_.erase(it->first) > 0)
				{
					hierachyViewChanged = true;
					delegateViewRemove(it->first);
					supervisorViewRemove(it->first);
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

	if (hierachyViewChanged)
	{
		hierViewVersion_++;
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this,
					"onMembershipEvent()", "hierarchy view changed");
			buffer->addProperty<uint64_t> ("hierViewVersion", hierViewVersion_);
			buffer->addProperty("view", SCMembershipEvent::viewMapToString(
					hierarchyView_, true));
			buffer->invoke();
		}
		hierarchyManager_.hierarchyViewChanged();
	}

	Trace_Exit(this, "onMembershipEvent()");
}

//void HierarchyViewKeeper::filterAttributeMap(event::AttributeMap_SPtr eventMap)
//{
//	event::AttributeMap::iterator it = eventMap->begin();
//	while (it != eventMap->end())
//	{
//		if (!boost::starts_with(it->first, HierarchyUtils::hierarchy_AttributeKeyPrefix))
//		{
//			event::AttributeMap::iterator pos2erase = it++;
//			eventMap->erase(pos2erase);
//		}
//		else
//		{
//			++it;
//		}
//	}
//}

event::AttributeMap_SPtr HierarchyViewKeeper::filter2AttributeMap(
		const event::AttributeMap_SPtr& eventMap)
{
	event::AttributeMap_SPtr filteredMap;

	event::AttributeMap::const_iterator it;
	for (it = eventMap->begin(); it != eventMap->end(); ++it)
	{
		if (boost::starts_with(it->first,
				HierarchyUtils::hierarchy_AttributeKeyPrefix))
		{
			if (!filteredMap)
			{
				filteredMap.reset(new AttributeMap);
			}
			filteredMap->insert(*it);
		}
	}

	return filteredMap;
}

void HierarchyViewKeeper::delegateViewRemove(NodeID_SPtr node)
{
	delegateView_.erase(boost::static_pointer_cast<NodeIDImpl>(node));
}



void HierarchyViewKeeper::delegateViewUpdate(NodeID_SPtr node, AttributeMap_SPtr attr_map)
{
	event::AttributeMap::const_iterator it;
	bool insert = false;
	DelegateState delegateState;
	for (it = attr_map->begin(); it != attr_map->end(); ++it)
	{
		if (boost::starts_with(it->first,
				HierarchyUtils::delegateSupervisor_AttributeKeyPrefix))
		{
			insert = true;
			size_t prefix_size = HierarchyUtils::delegateSupervisor_AttributeKeyPrefix.size();
			String supervisor_name(it->first.substr(prefix_size,
					it->first.size() - prefix_size));
			bool active = (it->second.getBuffer().get())[0];
			//TODO do something with the NodeID as well
			delegateState.insert(std::make_pair(supervisor_name, active));
		}
		else if (boost::starts_with(it->first,
				HierarchyUtils::delegateState_AttributeKey))
		{
			insert = true;
		}
	}

	if (insert)
	{
		std::pair<ZoneDelegatesStateMap::iterator, bool> result =
				delegateView_.insert(std::make_pair(
						boost::static_pointer_cast<NodeIDImpl>(node), delegateState));
		if (!result.second)
		{
			result.first->second.swap(delegateState);
		}
	}
	else
	{
		delegateView_.erase(boost::static_pointer_cast<NodeIDImpl>(node));
	}
}

void HierarchyViewKeeper::supervisorViewRemove(NodeID_SPtr node)
{
	Trace_Entry(this, "supervisorViewRemove()", "node", node->getNodeName());

	SupervisorReportMap::iterator it = supervisorsView_.begin();
	while ( it != supervisorsView_.end() )
	{
		it->second.erase(boost::static_pointer_cast<NodeIDImpl>(node));

		if (it->second.empty())
		{
			SupervisorReportMap::iterator pos2erase = it++;
			supervisorsView_.erase(pos2erase);
		}
		else
		{
			++it;
		}
	}

	Trace_Exit(this, "supervisorViewRemove()");
}

void HierarchyViewKeeper::supervisorViewUpdate(NodeID_SPtr node,
		event::AttributeMap_SPtr attr_map)
{
	using namespace event;

	Trace_Entry(this, "supervisorViewUpdate()",
			"node", node->getNodeName(),
			"Attr", attr_map->toString(event::AttributeValue::BIN));

	//build a zone report
	//std::map<String, int> zoneReport; //of the given node, key=zone-name, value=view-size
	std::map<String, pair<int,bool> > zoneReport; //of the given node, key=zone-name, value=view-size

	for (AttributeMap::const_iterator attr_it = attr_map->begin();
			attr_it != attr_map->end(); ++attr_it)
	{
		if (boost::starts_with(attr_it->first,
				HierarchyUtils::supervisorGuardedBaseZone_AttributeKeyPrefix))
		{
			size_t prefix_size = HierarchyUtils::supervisorGuardedBaseZone_AttributeKeyPrefix.size();
			String zone_name(attr_it->first.substr(prefix_size, attr_it->first.size() - prefix_size));
			ByteBuffer_SPtr buffer = ByteBuffer::createReadOnlyByteBuffer(
					attr_it->second.getBuffer().get(), attr_it->second.getLength(), false); //copy
			buffer->readInt(); //int numDelegates, skipped
			int numMembers = buffer->readInt();
			bool includeAttrs = buffer->readBoolean();
			zoneReport.insert(std::make_pair(zone_name, std::make_pair(numMembers, includeAttrs)));

			if (ScTraceBuffer::isDebugEnabled(tc_))
			{
				ScTraceBufferAPtr buffer = ScTraceBuffer::debug(
						this, "supervisorViewUpdate()","inserted to zoneReport");
				buffer->addProperty("zone", zone_name);
				buffer->addProperty<int>("num", numMembers);
				buffer->addProperty<bool>("includeAttrs", includeAttrs);
				buffer->invoke();
			}
		}
	}

	if (zoneReport.empty())
	{
		supervisorViewRemove(node);
	}
	else
	{
		{ //scan all existing zones for updates
			SupervisorReportMap::iterator view_it = supervisorsView_.begin();
			while (view_it != supervisorsView_.end())
			{
				if (zoneReport.count(view_it->first) > 0)
				{ //update or add {supervisor : count} to current zone
					Trace_Debug(this,"supervisorViewUpdate()","before update map");

					std::pair<SupervisorZoneReport::iterator,bool> res = view_it->second.insert(
							std::make_pair(boost::static_pointer_cast<NodeIDImpl>(node), zoneReport[view_it->first]));

					if (!res.second)
					{
						res.first->second = zoneReport[view_it->first];
					}
					++view_it;

					Trace_Debug(this,"supervisorViewUpdate()","after update map");
				}
				else if (view_it->second.count(boost::static_pointer_cast<NodeIDImpl>(node))>0)
				{//remove supervisor from current zone
					Trace_Debug(this,"supervisorViewUpdate()","before remove from map");

					view_it->second.erase(boost::static_pointer_cast<NodeIDImpl>(node)); //remove supervisor
					if (view_it->second.empty()) //zone has no more supervisors, remove it too
					{
						SupervisorReportMap::iterator pos2erase = view_it++;
						supervisorsView_.erase(pos2erase);
					}
					else
					{
						++view_it;
					}

					Trace_Debug(this,"supervisorViewUpdate()","after remove from map");
				}
				else
				{
					++view_it;
				}
			}
		}

		//handle all new zones
		for (std::map<String, std::pair<int, bool> >::const_iterator zr_it = zoneReport.begin();
				zr_it != zoneReport.end(); ++zr_it)
		{
			Trace_Debug(this,"supervisorViewUpdate()","before new zone test");

			std::pair<SupervisorReportMap::iterator,bool> res = supervisorsView_.insert(
				std::make_pair(zr_it->first, SupervisorZoneReport()));
			if (res.second) //a new zone
			{
				res.first->second.insert(std::make_pair(boost::static_pointer_cast<NodeIDImpl>(node), zr_it->second));
			}

			Trace_Debug(this,"supervisorViewUpdate()","after new zone test");
		}
	}

	Trace_Exit(this, "supervisorViewUpdate()");
}

NodeIDImpl_SPtr HierarchyViewKeeper::getActiveSupervisor(String zone, bool includeAttributes)
{
	Trace_Entry(this, "getActiveSupervisor", zone);

	NodeIDImpl_SPtr nodeId;

	SupervisorReportMap::iterator iter = supervisorsView_.find(zone);
	if (iter != supervisorsView_.end())
	{
		Trace_Event(this, "getActiveSupervisor", "found zone", "zone", iter->first);
		for (SupervisorZoneReport::iterator inner_iter = iter->second.begin(); inner_iter != iter->second.end(); inner_iter++)
		{
			// typedef std::map<NodeIDImpl_SPtr, std::pair<int, bool> > SupervisorZoneReport;
			int numMembers = inner_iter->second.first;
			bool hasAttrs = inner_iter->second.second;
			std::ostringstream oss;
			oss << "supervisor: " << inner_iter->first->getNodeName() << "; numMembers: " << inner_iter->second.first << "; has attrs: " << inner_iter->second.second << std::endl;

			Trace_Event(this, "getActiveSupervisor", oss.str());

			if (numMembers > -1 && (! includeAttributes || includeAttributes == hasAttrs))
			{
				nodeId = inner_iter->first;
				break;
			}

		}
	}
	else
	{
		Trace_Event(this, "getActiveSupervisor", "could not find zone", "zone", zone);
	}

	Trace_Exit(this, "getActiveSupervisor", nodeId ? nodeId->getNodeName(): "NULL");

	return nodeId;

}


String HierarchyViewKeeper::toStringZoneDelegatesStateMap() const
{
	std::ostringstream oss;
	if (delegateView_.empty())
	{
		oss << "empty";
	}
	else
	{
		oss << "size=" << delegateView_.size() << "; ";
		ZoneDelegatesStateMap::const_iterator it;
		for (it = delegateView_.begin(); it != delegateView_.end(); ++it)
		{
			oss << std::endl << "Del=" << it->first->getNodeName() << ", Sup={";
			DelegateState::const_iterator sup_it;
			for (sup_it = it->second.begin(); sup_it != it->second.end(); ++sup_it)
			{
				DelegateState::const_iterator next = sup_it;
				oss << sup_it->first << (sup_it->second ? " A" : " P")
						<< ((++next != it->second.end()) ? ", " : "");
			}
			oss << "};";
		}
	}
	return oss.str();
}

String HierarchyViewKeeper::toStringSupervisorReportMap() const
{
	std::ostringstream oss;
	if (supervisorsView_.empty())
	{
		oss << "empty";
	}
	else
	{
		oss << "size=" << supervisorsView_.size() << "; ";
		SupervisorReportMap::const_iterator it;
		for (it = supervisorsView_.begin(); it != supervisorsView_.end(); ++it)
		{
			oss << std::endl << "Zone=" << it->first << ", SupReport={";
			SupervisorZoneReport::const_iterator sup_it;
			for (sup_it = it->second.begin(); sup_it != it->second.end(); ++sup_it)
			{
				SupervisorZoneReport::const_iterator next = sup_it;
				oss << sup_it->first << ": " << sup_it->second.first << "; " << sup_it->second.second << std::endl;
			}
			oss << "};";
		}
	}
	return oss.str();

}

int HierarchyViewKeeper::getBaseZone_NumConnectedDelegates() const
{
	int count = 0;

	HierarchyViewKeeper::ZoneDelegatesStateMap::const_iterator it;
	for (it = delegateView_.begin(); it != delegateView_.end(); ++it)
	{
		if (!(it->second.empty()))
		{
			count++;
		}
	}
	return count;
}

NodeIDImpl_Set HierarchyViewKeeper::getBaseZone_ActiveDelegates() const
{
	NodeIDImpl_Set set;

	for (HierarchyViewKeeper::ZoneDelegatesStateMap::const_iterator it = delegateView_.begin();
			it != delegateView_.end(); ++it)
	{
		if (!(it->second.empty()))
		{
			for (DelegateState::const_iterator ds_it = it->second.begin();
					ds_it != it->second.end(); ++ds_it)
			{
				if (ds_it->second) //<supervisor-name, active>
				{
					set.insert(it->first); //active delegate
				}
			}
		}
	}

	return set;
}

int HierarchyViewKeeper::getBaseZone_NumDelegates() const
{
	return (int) delegateView_.size();
}

int HierarchyViewKeeper::getBaseZone_NumConnectedSupervisors() const
{
	std::set<String> supervisor_set;

	HierarchyViewKeeper::ZoneDelegatesStateMap::const_iterator it;
	for (it = delegateView_.begin(); it != delegateView_.end(); ++it)
	{
		DelegateState::const_iterator state_it;
		for (state_it = it->second.begin(); state_it != it->second.end(); ++state_it)
		{
			supervisor_set.insert(state_it->first);
		}
	}

	return (int) supervisor_set.size();
}

int HierarchyViewKeeper::writeMembershipPushQ(
		SCMessage_SPtr outgoingViewupdateMsg, bool includeAttributes,
		bool clearQ)
{
	int size = membershipPush_Queue_.size();
	int written = 0;
	ByteBuffer_SPtr bb = outgoingViewupdateMsg->getBuffer();
	bb->writeLong(membershipPush_viewID_);
	size_t num_events_pos = bb->getPosition();
	bb->writeInt(written);
	if (size > 0)
	{
		for (int i = 0; i < size; ++i)
		{
			if (includeAttributes)
			{
				outgoingViewupdateMsg->writeSCMembershipEvent(
						membershipPush_Queue_[i], true);
				written++;
			}
			else if (membershipPush_Queue_[i].getType()
					!= SCMembershipEvent::Change_of_Metadata)
			{
				outgoingViewupdateMsg->writeSCMembershipEvent(
						membershipPush_Queue_[i], false);
				written++;
			}
		}
	}

	if (written > 0)
	{
		size_t last_pos = bb->getPosition();
		bb->setPosition(num_events_pos);
		bb->writeInt(written);
		bb->setPosition(last_pos);
	}

	if (clearQ)
	{
		membershipPush_Queue_.clear();
	}

	return written;
}
}
