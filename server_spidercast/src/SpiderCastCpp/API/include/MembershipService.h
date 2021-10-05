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
 * MembershipService.h
 *
 *  Created on: 28/01/2010
 */

#include <boost/cstdint.hpp>
#include "Definitions.h"
#include "SpiderCastLogicError.h"
#include "SpiderCastRuntimeError.h"
#include "AttributeValue.h"
#include "NodeID.h"

#ifndef MEMBERSHIP_H_
#define MEMBERSHIP_H_

namespace spdr
{

/**
 * An interface providing access to the membership service.
 *
 * This membership service allows the user to receive membership events and to set its own attributes.
 *
 * An implementation of the MembershipService interface is created using the SpiderCast instance,
 * by calling SpiderCast#createMemebershipService(). Only a single instance of the MembershipService
 * can be created from an instance of SpiderCast.
 *
 * <b>Node Membership</b><p>
 *
 * The membership service allows a user to compile a view of the live nodes in the overlay,
 * and to detect the failure or departure of nodes. The MembershipService works by notifying the application,
 * via the MembershipListener, with membership events.
 *
 * The first event after creation of the service is ViewChangeEvent. This event delivers the node membership
 * at the current time, in the form of a map in which the keys are NodeIDs of live nodes and the values are
 * the respective MetaData. This map is defined as the ViewMap.
 *
 * Following the first view change event, the membership service proceeds to report view changes
 * differentially, by delivering NodeJoinEvent, NodeLeaveEvent, and ChangeOfMetaDataEvent.
 *
 * <b>The Attribute Service</b><p>
 *
 * An attribute is a key-value pair, in which the key is a string and the value is a byte buffer.
 * Each node has an attribute table, which can be written by it only.
 * The attribute table of every node is replicated to all other nodes and delivered in membership events,
 * as a field in the MetaData. Thus, every node can read the attribute map of every other node,
 * but write only its own attribute map.
 *
 * The keys must start with an alpha-numeric character.
 * The keys are trimmed - leading and trailing whitespace is ignored.
 *
 * <b>Foreign Zone Membership</b><p>
 *
 * When hierarchy is enabled, supervisors in the management zone maintain the membership view of
 * base-zones they are guarding. Supervisors disseminate the identity and node count of zones they
 * are guarding to all other supervisors.
 *
 * The API allows the user to get the zone-census, and query for the membership of any base-zone.
 * The ZoneCensus includes the names of all zones, and their node counts.
 * A query for the membership view of a foreign-zone will return a full member view of that zone.
 *
 * A request for a foreign zone membership may be fulfilled by the current node sending a request
 * to a remote node. Therefore, a request might time-out if a reply is not received in time. The
 * value of the time-out is defined in the properties parameter when creating the MembershipService.
 *
 * @see SpiderCast
 * @see NodeID
 * @see MetaData
 * @see ViewChangeEvent
 * @see ViewMap
 * @see NodeJoinEvent
 * @see NodeLeaveEvent
 * @see ChangeOfMetaDataEvent
 * @see ZoneCensusEvent
 * @see ForeignZoneMembershipEvent
 *
 */
class MembershipService
{
protected:
	MembershipService();
public:
	virtual ~MembershipService();

	/**
	 * Close the service.
	 *
	 * This closes the current instance of the membership service, stopping event notifications
	 * and disabling the ability to manipulate the attributes.
	 *
	 * Note that this does NOT clear the attributes, nor does it mean leaving the overlay.
	 * The node's attribute table is still kept internally and replicated, and the node's presence
	 * in the overlay is maintained.
	 *
	 * Once a membership service is closed it cannot be restarted, a new instance must be created.
	 */
	virtual void close() = 0;

	/**
	 * Is the service closed?
	 * @return
	 */
	virtual bool isClosed() = 0;

	/**
	 * Mark/unmark this node as a high priority monitor.
	 *
	 * The delivery of leave events and failure suspicions will be expedited to this node.
	 * This call has effect only if the configuration enables it, and UDP discovery is enabled.
	 *
	 * @param value
	 * @return the state of high priority monitoring after the call.
	 */
	virtual bool setHighPriorityMonitor(bool value) = 0;

	/**
	 * Whether this node is a high priority monitor.
	 *
	 * @return
	 */
	virtual bool isHighPriorityMonitor() = 0;

	//=== Attribute manipulation ===

	/**
	 * A constant representing an empty attribute value.
	 *
	 * A pair with:
	 * first (length) = 0;
	 * second (pointer to buffer) = NULL.
	 */
	static const Const_Buffer	EmptyAttributeValue; 	// (0,NULL)

	/**
	 * Set (and overwrite) a key-value pair.
	 *
	 * An attribute value is a raw byte buffer.
	 * First member is the length, second is a pointer to the buffer.
	 * A zero length is legal and means a key mapped to an empty value, the pointer is NULL.
	 *
	 * The buffer is copied, so the pointer remains under responsibility of the caller.
	 *
	 * @param key
	 * 		Must start with an alphanumeric character, trailing whitespace is trimmed.
	 * @param value
	 * 		A pair in which the first member is the length, second is a pointer to the buffer.
	 * 		A zero length is legal and means a key mapped to an empty value, the pointer must be NULL.
	 * 		Negative length is illegal.
	 *
	 * @return true if the key is new, map size changed
	 *
	 * @throw IllegalStateException if called when service is closed.
	 * @throw IllegalArgumentException if illegal key or value is given.
	 */
	virtual bool setAttribute(const String& key, Const_Buffer value) = 0;

	/**
	 * Get the value mapped to a key.
	 *
	 * First member of the returned pair holds an AttributeValue,
	 * which holds a shared pointer to the internal immutable buffer, and the length.
	 * Do not write to the buffer or bad things will happen.
	 *
	 * Second member holds an indication whether the key was found in the map.
	 * If the key was missing, the return value has zero length and NULL pointer.
	 *
	 * Note that a zero length value is legal and means a key mapped to an empty value, the pointer is NULL.
	 *
	 * @param key
	 * @return a pair holding the value, and an indication whether it was found on the map.
	 *
	 * @throw IllegalStateException if called when service is closed.
	 *
	 * @see AttributeValue
	 *
	 */
	virtual std::pair<event::AttributeValue,bool> getAttribute(const String& key) = 0;

	/**
	 * Remove a key-value pair.
	 *
	 * @param key
	 * @return true if removed, map size changed
	 *
	 * @throw IllegalStateException if called when service is closed.
	 */
	virtual bool removeAttribute(const String& key) = 0;

	/**
	 * Contains a key?
	 *
	 * @param key
	 * @return true if the map contains the key
	 *
	 * @throw IllegalStateException if called when service is closed.
	 */
	virtual bool containsAttribute(const String& key) = 0;


	/**
	 * Clear all key-value attributes.
	 *
	 * @throw IllegalStateException if called when service is closed.
	 */
	virtual void clearAttributeMap() = 0;

	/**
	 * Is the attribute map empty?
	 * @return true if attribute map is empty
	 *
	 * @throw IllegalStateException if called when service is closed.
	 */
	virtual bool isEmptyAttributeMap() = 0;

	/**
	 * Size of attribute map.
	 * @return size of attribute map
	 *
	 * @throw IllegalStateException if called when service is closed.
	 */
	virtual size_t sizeOfAttributeMap() = 0;

	/**
	 * Get the attribute key-set.
	 * @return
	 *
	 * @throw IllegalStateException if called when service is closed.
	 */
	virtual StringSet getAttributeKeySet() = 0;

	//=== Retained Attributes ===

	/**
	 * Clear the retained attributes from a target node that is no longer in view.
	 *
	 * If this node has retained attributes for a node that is no longer in view,
	 * it will clear them and signal every member of the view to clear those attributes as well.
	 *
	 * If the target node is in view this operation will fail.
	 *
	 * TODO RetainAtt
	 *
	 * @param target
	 * @param incarnation
	 * @return true if the operation succeeded
	 */
	virtual bool clearRemoteNodeRetainedAttributes(NodeID_SPtr target, int64_t incarnation) = 0;

	//=== Foreign Zone Membership ===

	/**
	 * Get the membership view of a single base-zone.
	 *
	 * Asynchronously retrieve the membership view of a base-zone.
	 *
	 * The view is delivered via the MembershipListener in a ForeignZoneMembershipEvent.
	 * When a request is issued for a single base-zone ForeignZoneMembershipEvent::isLastEvent()
	 * always returns true.
	 *
	 * Note that this request may fail immediately with an exception, or by returning a
	 * ForeignZoneMembershipEvent in which ForeignZoneMembershipEvent::isError() returns true.
	 * In case of an error, the events carries an error code and an error text message.
	 *
	 * @param zoneBusName
	 * 		The base-zone bus-name.
	 * @param includeAttributes
	 * 		Whether to include attributes or not
	 *
	 * @return A request number. This number is included in the resulting event.
	 *
	 * @throw IllegalStateException if called when service is closed;
	 * 			or if called on a node that belongs to a base-zone.
	 * @throw IllegalArgumentException if the bus-name is ill-formed.
	 *
	 * @see event::ForeignZoneMembershipEvent
	 */
	virtual int64_t getForeignZoneMembership(const String& zoneBusName, bool includeAttributes)
		throw(SpiderCastLogicError, SpiderCastRuntimeError) = 0;

	/**
	 * Get the zone census.
	 *
	 * Asynchronously retrieve the zone-census.
	 *
	 * The zone-census is delivered via the MembershipListener in a ZoneCensusEvent.
	 *
	 * @return A request number. This number is included in the resulting event.
	 *
	 * @throw IllegalStateException if called when service is closed;
	 * 			or if called on a node that belongs to a base-zone.
	 *
	 * @see event::ZoneCensusEvent
	 */
	virtual int64_t getZoneCensus()
		throw(SpiderCastLogicError, SpiderCastRuntimeError) = 0;

};

typedef boost::shared_ptr<MembershipService> MembershipService_SPtr;

}

#endif /* MEMBERSHIP_H_ */
