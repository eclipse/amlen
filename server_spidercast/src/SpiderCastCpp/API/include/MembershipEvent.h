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
 * MembershipEvent.h
 *
 *  Created on: 21/03/2010
 */

#ifndef MEMBERSHIPEVENT_H_
#define MEMBERSHIPEVENT_H_

#include <map>

#include "SpiderCastEvent.h"
#include "NodeID.h"
#include "MetaData.h"

namespace spdr
{

namespace event
{

/**
 * A map that holds the node membership view.
 *
 * The ViewMap is an std::map, in which the keys are shared-pointers to NodeID,
 * and the values are shared-pointers to MetaData.
 * The map is sorted by increasing NodeID using the function object NodeID::SPtr_Less.
 * NodeIDs are ordered by name.
 *
 * @see NodeID
 * @see MetaData
 */
typedef std::map< NodeID_SPtr, MetaData_SPtr, NodeID::SPtr_Less > 	ViewMap;
//typedef boost::unordered_map< NodeID_SPtr, MetaData_SPtr, NodeID::SPtr_Hash, NodeID::SPtr_Equals > 	ViewMap;

/**
 * A boost::shared_ptr to a ViewMap.
 */
typedef boost::shared_ptr<ViewMap > 								ViewMap_SPtr;

typedef std::set<NodeID_SPtr, NodeID::SPtr_Less >					NodeIDSet;

typedef boost::shared_ptr<NodeIDSet >								NodeIDSet_SPtr;

/**
 * The super-class of all membership events.
 * <p>
 * All membership events have a single common ancestor, the MembershipEvent class.
 * Use the SpiderCastEvent#getType() method to select to correct sub-type to cast
 * to in order to get event details.
 * <p>
 * It is recommended to use:
 * <p>
 * boost::shared_ptr<X> p_sub_type = boost::static_pointer_cast<X>(boost::shared_ptr<Y> p_ancestor);
 * <p>
 * For example:
 * <UL>
 * 	<LI> MembershipEvent_SPtr event = ...;
 * 	<LI> NodeJoinEvent_SPtr njp = boost::static_pointer_cast<NodeJoinEvent>(event);
 * </UL>
 */
class MembershipEvent : public SpiderCastEvent
{
public:
	/**
	 * Distructor.
	 * @return
	 */
	virtual ~MembershipEvent() {}

protected:
	/**
	 * Constructor.
	 * @param eventType
	 * @return
	 */
	MembershipEvent(EventType eventType) :
		SpiderCastEvent(eventType)
	{
	}
};

/**
 * Node join event.
 *
 * Carries the NodeID of the joining node and its MetaData.
 *
 * Event type: EventType#Node_Join.
 *
 * @see NodeID
 * @see MetaData
 */
class NodeJoinEvent : public MembershipEvent
{

public:
	/**
	 * Constructor.
	 * @param nodeID
	 * @param metaData
	 * @return
	 */
	NodeJoinEvent(
			NodeID_SPtr nodeID,
			MetaData_SPtr metaData);
	/**
	 * Distructor.
	 * @return
	 */
	virtual ~NodeJoinEvent();

	/**
	 * Get NodeID.
	 * @return
	 */
	const NodeID_SPtr getNodeID() const;

	/**
	 * Get MetaData.
	 * @return
	 */
	const MetaData_SPtr getMetaData() const;

	/**
	 * A string representation of the event.
	 * @return
	 */
	String toString() const;

private:
	NodeID_SPtr _nodeID;
	MetaData_SPtr _metaData;
};

/**
 * Node leave event.
 *
 * Carries the NodeID of the leaving node.
 *
 * Event type: EventType#Node_Leave.
 *
 * @see NodeID
 */
class NodeLeaveEvent : public MembershipEvent
{

public:
	/**
	 * Constructor.
	 *
	 * @param nodeID
	 * @return
	 */
	NodeLeaveEvent(
			NodeID_SPtr nodeID,
			MetaData_SPtr metaData);

	/**
	 * Distructor.
	 *
	 * @return
	 */
	virtual ~NodeLeaveEvent();

	/**
	 * Get the NodeID.
	 * @return
	 */
	const NodeID_SPtr getNodeID() const;

	/**
	 * Get MetaData.
	 * @return
	 */
	const MetaData_SPtr getMetaData() const;

	/**
	 * A string representation of the event.
	 * @return
	 */
	String toString() const;

private:
	NodeID_SPtr _nodeID;
	MetaData_SPtr _metaData;
};

/**
 * View change event.
 *
 * Carries a map that contains the current membership view. The view is represented in the
 * form of a ViewMap.
 *
 * Event type: EventType#View_Change.
 *
 * @see ViewMap
 * @see MetaData
 */
class ViewChangeEvent : public MembershipEvent
{

public:
	/**
	 * Constructor.
	 * @param view
	 * @return
	 */
	ViewChangeEvent(ViewMap_SPtr view);

	/**
	 * Distructor.
	 * @return
	 */
	virtual ~ViewChangeEvent();

	/**
	 * Get the view.
	 * @return
	 */
	ViewMap_SPtr getView();

	/**
	 * A string representation of the event.
	 * @return
	 */
	String toString() const;

	/**
	 * A detailed string representation of the view.
	 *
	 * Includes the nodes (keys), the incarnation number & status, but does NOT include the attributes.
	 *
	 * @param view
	 * @return
	 *
	 * @see ViewMap
	 * @see ChangeOfMetaDataEvent#viewMapToString()
	 */
	static String viewMapToString(ViewMap_SPtr view);

	static String viewMapEPToString(ViewMap_SPtr view);

private:
	boost::shared_ptr<ViewMap > _view;
};

/**
 * Change of meta-data event.
 *
 * Delivers the updated meta-data of alive nodes whose meta-data changed.
 * That is, the encapsulated ViewMap may contain a subset of the full view.
 *
 * Event type: EventType#Change_of_Metadata.
 *
 * @see ViewMap
 * @see MetaData
 */
class ChangeOfMetaDataEvent : public MembershipEvent
{
public:
	/**
	 * Constructor.
	 * @param view
	 * @return
	 */
	ChangeOfMetaDataEvent(ViewMap_SPtr view);

	/**
	 * Distructor.
	 * @return
	 */
	virtual ~ChangeOfMetaDataEvent();

	/**
	 * Get the view.
	 * @return
	 */
	ViewMap_SPtr getView();

	/**
	 * A short string representation of the event, does not print the view.
	 * @return
	 */
	String toString() const;

	/**
	 * A detailed string representation of the view.
	 *
	 * Includes the nodes and the meta-data (with the attribute maps).
	 *
	 * @param view
	 * @param mode how to print attribute values.
	 * @return
	 *
	 * @see ViewMap
	 * @see AttributeValue
	 * @see ViewChangeEvent#viewMapToString()
	 */
	static String viewMapToString(ViewMap_SPtr view, AttributeValue::ToStringMode mode);

private:
	boost::shared_ptr<ViewMap > _view;
};

/**
 * Foreign zone membership event.
 *
 * Delivers a foreign zone membership event, in response to a foreign-zone membership request.
 *
 * Event type: EventType#Foreign_Zone_Membership.
 *
 */
class ForeignZoneMembershipEvent : public MembershipEvent
{
public:
	/**
	 * Constructor of a successful request.
	 *
	 * @param requestID
	 * @param zoneBusName
	 * @param view
	 * @param lastEvent
	 * @return
	 */
	ForeignZoneMembershipEvent(
			const int64_t requestID, const String& zoneBusName,
			boost::shared_ptr<ViewMap > view, const bool lastEvent);

	/**
	 * Constructor of a failed request.
	 *
	 * @param requestID
	 * @param zoneBusName
	 * @param errorCode
	 * @param errorMessage
	 * @param lastEvent
	 * @return
	 */
	ForeignZoneMembershipEvent(
			const int64_t requestID, const String& zoneBusName,
			ErrorCode errorCode, const String& errorMessage, bool lastEvent);

	/**
	 * Distructor.
	 * @return
	 */
	virtual ~ForeignZoneMembershipEvent();

	int64_t getRequestID() const;

	const String& getZoneBusName() const;

	/**
	 * Get the view.
	 * @return
	 */
	ViewMap_SPtr getView();

	/**
	 * Is this the last event in series?
	 * @return
	 */
	bool isLastEvent() const;

	/**
	 * What is the error code?
	 * @return Error_Code_None if no error, or a specific error code.
	 */
	ErrorCode getErrorCode() const;

	/**
	 * A text error message.
	 * @return empty if no error, or a specific descriptive error message.
	 */
	const String& getErrorMessage() const;

	/**
	 * Is there an error?
	 * @return false if Error_Code_None, true otherwise
	 */
	bool isError() const;

	/**
	 * A short string representation of the event, does not print the view.
	 * @return
	 */
	String toString() const;

private:
	const int64_t requestID_;
	const String zoneBusName_;
	boost::shared_ptr<ViewMap > _view;
	const bool lastEvent_;
	ErrorCode errorCode_;
	const String errorMessage_;
};

/**
 *  Zone census values are node counts.
 */
typedef int32_t ZoneCensusData;

/**
 * A map where the keys are zone-bus-names and the values are zone census data.
 *
 * Zone census values are node counts.
 *
 * The map contains the following entries, in order:
 * The value associated with "/" has the total node count.
 * The value associated with "/{L1}" has the management zone node count.
 * The value associated with "/{L1}/{L2}" has the base zone node count, for each base-zone, respectively.
 */
typedef std::map<String,ZoneCensusData> ZoneCensus;

/**
 * A boost::shared_ptr to ZoneCensus
 */
typedef boost::shared_ptr<ZoneCensus> ZoneCensus_SPtr;

/**
 * Zone census event.
 *
 * Delivers a zone census event, in response to a zone-census request.
 *
 * Event type: EventType#Zone_Census.
 *
 */
class ZoneCensusEvent : public MembershipEvent
{
public:
	/**
	 * Constructor.
	 *
	 * @param requestID
	 * @param census
	 * @param full
	 * @return
	 */
	ZoneCensusEvent(const int64_t requestID, ZoneCensus_SPtr census, const bool full);

	/**
	 * Distructor.
	 * @return
	 */
	virtual ~ZoneCensusEvent();

	int64_t getRequestID() const;

	/**
	 * Get the census.
	 * @return
	 */
	ZoneCensus_SPtr getZoneCensus();

	/**
	 * Is this a full census or a differential update?
	 * @return
	 */
	bool isFullCensus() const;

	/**
	 * A short string representation of the event, does not print the census.
	 * @return
	 */
	String toString() const;

	/**
	 * Creates a string representation of the zone-census.
	 * @param census
	 * @return
	 */
	static String zoneCensusToString(ZoneCensus_SPtr census);

private:
	const int64_t requestID_;
	ZoneCensus_SPtr census_;
	const bool full_;
};

//===================================================================
/**
 * A boost::shared_ptr to a MembershipEvent.
 */
typedef boost::shared_ptr<event::MembershipEvent> 			MembershipEvent_SPtr;
/**
 * A boost::shared_ptr to a NodeJoinEvent.
 */
typedef boost::shared_ptr<event::NodeJoinEvent> 			NodeJoinEvent_SPtr;
/**
 * A boost::shared_ptr to a NodeLeaveEvent.
 */
typedef boost::shared_ptr<event::NodeLeaveEvent> 			NodeLeaveEvent_SPtr;
/**
 * A boost::shared_ptr to a ViewChangeEvent.
 */
typedef boost::shared_ptr<event::ViewChangeEvent> 			ViewChangeEvent_SPtr;
/**
 * A boost::shared_ptr to a ChangeOfMetaDataEvent.
 */
typedef boost::shared_ptr<event::ChangeOfMetaDataEvent> 	ChangeOfMetaDataEvent_SPtr;
/**
 * A boost::shared_ptr to a ForeignZoneMembershipEvent.
 */
typedef boost::shared_ptr<ForeignZoneMembershipEvent> 		ForeignZoneMembershipEvent_SPtr;
/**
 * A boost::shared_ptr to a ZoneCensusEvent.
 */
typedef boost::shared_ptr<ZoneCensusEvent> 					ZoneCensusEvent_SPtr;

} //namespace event
} //namespace spdr

#endif /* MEMBERSHIPEVENT_H_ */
