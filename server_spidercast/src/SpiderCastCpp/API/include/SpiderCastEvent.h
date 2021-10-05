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
 * SpiderCastEvent.h
 *
 *  Created on: 10/03/2010
 */

#ifndef SPIDERCASTEVENT_H_
#define SPIDERCASTEVENT_H_

#include "Definitions.h"

namespace spdr
{

/**
 * All SpiderCast events are defined under this namespace.
 *
 * All SpiderCast events are defined under the spdr::event namespace,
 * and all have a common ancestor, the class SpiderCastEvent.
 * The event class hierarchy reflects the functional division of events to functional
 * units, such as error events, membership events, pub/sub, and so on.
 *
 */
namespace event
{

//=========================================================

/**
 * Event type.
 *
 * Every concrete event has a unique event type.
 * Given a pointer of a super-class, use this enum to select the correct concrete type to cast to.
 */
enum EventType
{
	Event_Type_None = 0,

	// === Error, Warning & Information Events ===
	// These events are delivered to the SpiderCastEventListener

	Fatal_Error = 1, 						//!< Fatal Error (error events)

	Warning_Connection_Refused,    			//!< Warning, connection refused

	Warning_Message_Refused,    			//!< Warning, message refused

	Connectivity,							//!< Connectivity, an informational event


	// === Membership Events ===
	// These events are delivered to the MembershipListener
	View_Change,							//!< View Change (membership events)
	Node_Join,								//!< Node_Join (membership events)
	Node_Leave,								//!< Node_Leave (membership events)
	Change_of_Metadata,						//!< Change_of_Metadata (membership events)
	Foreign_Zone_Membership, 				//!< Foreign_Zone_Membership (membership events)
	Zone_Census,							//!< Zone_Census (membership events)

	//=== P2P ===
	// These events are delivered to the P2PStreamEventListener
	STERAM_BROKE,             				//!<< P2P stream broke

	//=== Range guard ===
	Event_Type_Max					//!< Event_Type_Max (range guard)
} ;

//=========================================================

/**
 * Error code.
 *
 * Error and warning events carry an error code, enumerated here.
 */
enum ErrorCode
{
		Error_Code_Unspecified = 0,

		//=== General errors ===
		Thread_Exit_Abnormally = 1,     //!< Thread_Exit_Abnormally
		Network_Failure,           		//!< Network_Failure
		Component_Failure,         		//!< Component_Failure
		Duplicate_Local_Node_Detected,	//!< Duplicate_Local_Node_Detected

		//=== Networking related errors ===
		Bind_Error_Port_Busy,						//!< Failed to bind to listening port
		Bind_Error_Cannot_Assign_Local_Interface,  	//!< An error occurred while trying to set the local interface

		//=== Warnings ===
		Connection_Refused_Handshake_Parse_Error,
		Connection_Refused_Incompatible_Version,
		Connection_Refused_Incompatible_BusName,
		Connection_Refused_Wrong_TargetName,

		Message_Refused_Parse_Error,
		Message_Refused_Incompatible_Version,
		Message_Refused_Incompatible_BusName,
		Message_Refused_Incompatible_TargetName,

		Duplicate_Remote_Node_Suspicion,


		//=== Foreign Zone Membership ===
		Foreign_Zone_Membership_Request_Timeout, //!< Foreign_Zone_Membership_Request_Timeout
		Foreign_Zone_Membership_Unreachable, //!< Foreign_Zone_Membership_Unreachable
		Foreign_Zone_Membership_No_Attributes, //!< Foreign_Zone_Membership_No_Attributes

		//=== Range guard ===
		Error_Code_Max             //!< Error_Code_Max (range guard)
};

//=========================================================

/**
 * Superclass of event hierarchy.
 *
 * All SpiderCast events have a single common ancestor, the class SpiderCastEvent.
 * The event class hierarchy reflects the functional division of events to functional
 * units, such as error events, membership events, and so on.
 *
 * Use the SpiderCastEvent#getType() method to select to correct sub-type to cast
 * to in order to get event details.
 *
 * It is recommended to use:
 * <p>
 * boost::shared_ptr<X> p_sub_type = boost::static_pointer_cast<X>(boost::shared_ptr<Y> p_ancestor);
 * <p>
 * For example:
 * <UL>
 * 	<LI> SpiderCastEvent_SPtr event = ...;
 * 	<LI> FatalErrorEvent_SPtr fep = boost::static_pointer_cast<FatalErroeEvent>(event);
 * </UL>
 *
 */
class SpiderCastEvent
{

public:

	/**
	 * Event name array.
	 *
	 * Convert the EventType enum to a string using this array.
	 */
	static const String eventTypeName[];

	/**
	 * Error code name array.
	 *
	 * Convert the ErrorCode enum to a string using this array.
	 */
	static const String errorCodeName[];

	/**
	 * Destructor.
	 *
	 * @return
	 */
	virtual ~SpiderCastEvent() = 0;

	/**
	 * Get event type.
	 *
	 * @return
	 */
	virtual const EventType getType() const
	{
		return type;
	}

	/**
	 * Get a string representation of the event.
	 *
	 * @return
	 */
	virtual String toString() const
	{
		return "Event[" + eventTypeName[type] + "]";
	}



protected:
	/**
	 * Constructor.
	 * @param eventType
	 * @return
	 */
	SpiderCastEvent(EventType eventType) :
		type(eventType)
	{
	}

	/**
	 * Event type.
	 */
	const EventType type;
};

/**
 * A boost::shared_ptr to a SpiderCastEvent.
 */
typedef boost::shared_ptr<event::SpiderCastEvent> SpiderCastEvent_SPtr;

}//namespace event
}//namespace spdr

#endif /* SPIDERCASTEVENT_H_ */
