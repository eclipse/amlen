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

#ifndef SCMEMBERSHIPEVENT_H_
#define SCMEMBERSHIPEVENT_H_

#include <set>
#include <boost/unordered_map.hpp>

#include "SpiderCastEvent.h"
//#include "ConcurrentSharedPtr.h"
#include "NodeIDImpl.h"
#include "MetaData.h"

namespace spdr
{

/**
 * A map that holds the internal node membership view.
 * Delivered in internal membership events.
 */
//typedef std::map< NodeIDImpl_SPtr, event::MetaData_SPtr, NodeIDImpl_SPtr::Less > 	SCViewMap;
typedef boost::unordered_map< NodeID_SPtr, event::MetaData_SPtr,
		NodeID::SPtr_Hash, NodeID::SPtr_Equals > 	SCViewMap;

/**
 * A boost::shared_ptr to a ViewMap.
 */
typedef boost::shared_ptr<SCViewMap > 	SCViewMap_SPtr;

/**
 * Internal membership events.
 *
 * The type field determines which members are valid:
 * Join: node-ID + metadata,
 * Leave: node-ID,
 * View-change & change-of-metadata: view.
 *
 * The copy constructor and assignment operators are default synthesized.
 */
class SCMembershipEvent
{
public:
	enum Type
	{
		View_Change,
		Node_Join,
		Node_Leave,
		Change_of_Metadata
	};

	/**
	 * Distructor.
	 * @return
	 */
	virtual ~SCMembershipEvent() {}

	/**
	 * Constructor (Join / Leave).
	 * @param eventType
	 * @return
	 */
	SCMembershipEvent(Type eventType,
			NodeID_SPtr nodeID,
			event::MetaData_SPtr metaData = event::MetaData_SPtr());

	/**
	 * Constructor (View-change / change-of-metadata).
	 * @param eventType
	 * @return
	 */
	SCMembershipEvent(Type eventType, SCViewMap_SPtr view);

	Type getType() const;

	NodeID_SPtr getNodeID() const;

	event::MetaData_SPtr getMetaData() const;

	SCViewMap_SPtr getView() const;

	/**
	 * A string representation.
	 *
	 * Note that since the view is based on an unordered map which does not guaranty iteration order,
	 * the string representation may change from invocation to invocation even if the map is the same.
	 *
	 * @return
	 */
	String toString() const;

	static String viewMapToString(
			const SCViewMap& view,
			bool withMetadata = false,
			event::AttributeValue::ToStringMode mode = event::AttributeValue::BIN);

	friend
	bool operator==(const SCMembershipEvent& lhs, const SCMembershipEvent& rhs);

	friend
	bool operator!=(const SCMembershipEvent& lhs, const SCMembershipEvent& rhs);


private:
	Type type_;
	NodeID_SPtr nodeID_;
	event::MetaData_SPtr metadata_;
	SCViewMap_SPtr view_;
};

/**
 * A non-member function to filter an AttributeMap using a key-prefix.
 *
 * @param map
 * @param keyPrefix
 * @return A pointer to a newly allocated map with the attributes that contain the key-prefix.
 * 		Null if no matching attributes were found.
 */
event::AttributeMap_SPtr filter2AttributeMap(const event::AttributeMap& map,
		const String& keyPrefix);


bool operator==(const SCViewMap& lhs, const SCViewMap& rhs);

bool operator!=(const SCViewMap& lhs, const SCViewMap& rhs);

} //namespace spdr

#endif /* SCMEMBERSHIPEVENT_H_ */
