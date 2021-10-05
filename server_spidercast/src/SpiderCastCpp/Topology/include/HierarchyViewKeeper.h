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
 * HierarchyViewKeeper.h
 *
 *  Created on: Oct 7, 2010
 */

#ifndef HIERARCHYVIEWKEEPER_H_
#define HIERARCHYVIEWKEEPER_H_

#include <set>

#include <boost/noncopyable.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "SCMembershipListener.h"
#include "SpiderCastConfigImpl.h"
#include "HierarchyUtils.h"
#include "HierarchyViewListener.h"
#include "SCMessage.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

/**
 * This class maintains a view of nodes that have ".hier.*" attribute keys.
 *
 * Threading: access by MemTopoThread only.
 */
class HierarchyViewKeeper: boost::noncopyable,
		public SCMembershipListener,
		public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:

	HierarchyViewKeeper(const String& instID,
			const SpiderCastConfigImpl& config,
			HierarchyViewListener& hierarchyManager);
	virtual ~HierarchyViewKeeper();

	/*
	 * @see spdr::SCMembershipListener
	 */
	void onMembershipEvent(const SCMembershipEvent& event);

	/**
	 * The state of a delegate is the set of supervisors it is connected to,
	 * and whether it is active w.r.t. that supervisor. That is:
	 *
	 * Key = supervisor-name
	 * Value = is-active
	 */
	typedef std::map<String, bool> DelegateState;

	/**
	 * Contains all the delegates in the zone and their state.
	 * That is:
	 *
	 * Key = delegate-ID
	 * Value = delegate-state
	 */
	typedef std::map<NodeIDImpl_SPtr, DelegateState, NodeIDImpl::SPtr_Less>
			ZoneDelegatesStateMap;

	/**
	 * The report of a supervisor consists of the number of view members in the guarded base zone
	 *
	 * Key = supervisor-name
	 * Value = pair {int number of view members, bool includeAttributes)
	 */
	typedef std::map<NodeIDImpl_SPtr, std::pair<int, bool> > SupervisorZoneReport;

	/**
	 * Contains all the zones and their respective reports
	 * That is:
	 *
	 * Key = zone name
	 * Value = supervisor report
	 */
	typedef std::map<String, SupervisorZoneReport> SupervisorReportMap;

	inline int64_t getHierarchyViewVersion()
	{
		return hierViewVersion_;
	}

	const ZoneDelegatesStateMap& getZoneDelegatesStateMap() const
	{
		return delegateView_;
	}

	const SupervisorReportMap& getSupervisorReportMap() const
	{
		return supervisorsView_;
	}

	String toStringSupervisorReportMap() const;

	String toStringZoneDelegatesStateMap() const;

	int getBaseZone_NumDelegates() const;

	int getBaseZone_NumConnectedDelegates() const;

	int getBaseZone_NumConnectedSupervisors() const;

	NodeIDImpl_Set getBaseZone_ActiveDelegates() const;

	inline void setMembershipPushActive(bool active)
	{
		membershipPush_active_ = active;
		if (!active)
		{
			membershipPush_Queue_.clear();
		}
	}

	inline bool isMembershipPushActive() const
	{
		return membershipPush_active_;
	}

	inline int64_t getMembershipPushViewID() const
	{
		return membershipPush_viewID_;
	}

	/**
	 * Write the events in the Q to the message and clear the Q.
	 *
	 * writes:
	 * - view-ID
	 * - num-events
	 * - a series of events
	 *
	 * Note that view-ID and num-events are written when num-events==0 as well.
	 *
	 * @param outgoingViewupdateMsg
	 * @param includeAttributes
	 * @param clearQ
	 * @return number of events written.
	 */
	int writeMembershipPushQ(SCMessage_SPtr outgoingViewupdateMsg,
			bool includeAttributes, bool clearQ);

	NodeIDImpl_SPtr getActiveSupervisor(String zone, bool includeAttributes);

private:
	int64_t hierViewVersion_;
	HierarchyViewListener& hierarchyManager_;

	/*
	 * Contains nodes with .hier.* attribute keys.
	 * Filters out all attributes but .hier.* attributes.
	 */
	SCViewMap hierarchyView_;

	/*
	 * Contains all the delegates in the zone and their state.
	 */
	ZoneDelegatesStateMap delegateView_;

	/*
	 * Contains all the zones and their supervisors report
	 */
	SupervisorReportMap supervisorsView_;

	/*
	 * Accumulate membership events into the into the queue
	 */
	bool membershipPush_active_;
	/*
	 * A queue of membership events ready to be written to a view update message.
	 */
	std::vector<SCMembershipEvent> membershipPush_Queue_;

	int64_t membershipPush_viewID_;

	//=== methods ===

	//	/**
	//	 * Remove all attributes but those with the ".hier." prefix
	//	 * @param eventMap
	//	 * @return a changed map
	//	 */
	//	void filterAttributeMap(event::AttributeMap_SPtr eventMap);

	/**
	 * Return a new map with all attributes starting with ".hier." prefix
	 * @param eventMap
	 * @return new map with ".hier." attributes only, or null
	 */
	event::AttributeMap_SPtr filter2AttributeMap(
			const event::AttributeMap_SPtr& eventMap);

	/**
	 * Remove a node from the delegate view.
	 *
	 * The node may or may not already exist in the delegateView.
	 *
	 * @param node
	 */
	void delegateViewRemove(NodeID_SPtr node);

	/**
	 * Update the state of a node in the delegate view.
	 *
	 * Assumes that the attribute map has some keys with HierarchyUtils::hierarchy_AttributeKeyPrefix in them.
	 * The node may or may not already exist in the delegateView.
	 * This call may result with a node removed from the delegate view.
	 *
	 * @param node
	 * @param attr_map
	 */
	void delegateViewUpdate(NodeID_SPtr node,
			event::AttributeMap_SPtr attr_map);

	/**
	 * Remove a node from the supervisor view.
	 *
	 * The node may or may not already exist in the supervisorView.
	 *
	 * @param node
	 */
	void supervisorViewRemove(NodeID_SPtr node);

	/**
	 * Update the supervisor report map based on the info of a member in the mgmt zone
	 *
	 * Assumes that the attribute map has some keys with HierarchyUtils::hierarchy_AttributeKeyPrefix in them.
	 * The node may or may not already exist in the supervisor map.
	 *
	 * @param node
	 * @param attr_map
	 */
	void supervisorViewUpdate(NodeID_SPtr node,
			event::AttributeMap_SPtr attr_map);
};
}
#endif /* HIERARCHYVIEWKEEPER_H_ */
