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
 * MetaData.h
 *
 *  Created on: 21/03/2010
 */

#ifndef METADATA_H_
#define METADATA_H_

#include <sstream>

#include "Definitions.h"
#include "AttributeMap.h"

namespace spdr
{
namespace event
{

enum NodeStatus
{
	/**
	 * The node is alive
	 */
	STATUS_ALIVE 		= 0,

	/**
	 * The node is suspected as crashed or behind a network partition
	 */
	STATUS_SUSPECT  	= 1,

	/**
	 * The node orderly left the overlay with an intention to rejoin,
	 * the application may keep state associated with this node.
	 */
	STATUS_LEAVE 		= 2,

	/**
	 * The node orderly left the overlay with no intention to rejoin,
	 * the application may delete state associated with this node.
	 */
	STATUS_REMOVE 		= 3,

	/**
	 * The node is suspected of having another node with the same name
	 * running at the same time (AKA "Split Brain").
	 */
	STATUS_SUSPECT_DUPLICATE_NODE = 4
};

class MetaData;
/**
 * A shared pointer to a MetaData.
 */
typedef boost::shared_ptr<MetaData> 							MetaData_SPtr;

/**
 * Holds the node's meta-data.
 *
 * The node's meta-data is composed of
 * <UL>
 * 	<LI> The AttributeMap - a map holding key-value attributes.
 * 	<LI> The incarnation number - an integer identifier that increases every
 * 	     time a node starts.
 * 	<LI> The node status - an indicator of whether the node is alive or not,
 * 	     and if not, what were the circumstances of its departure from
 * 	     the overlay.
 * </UL>
 *
 * The meta-data can be compared for equality using operators == and !=.
 * Equality means that all the elements of the meta-data are equal
 * - deep compare.
 *
 * Copy constructor and assignment operator are default synthesized (shallow
 * copy). For deep copy use clone().
 *
 */
class MetaData
{
public:
	/**
	 * Constructor.
	 *
	 *
	 *
	 * @param attributeMap a shared pointer to the attribute map associated with the node.
	 * @param incarnationNumber an integer identifier that increases every time a node starts.
	 * @return
	 */
	MetaData(AttributeMap_SPtr attributeMap, int64_t incarnationNumber, NodeStatus status);

	/**
	 * Destructor.
	 *
	 * @return
	 */
	virtual ~MetaData();

	/**
	 * Get the attribute map.
	 * @return a shared pointer to the attribute map.
	 */
	AttributeMap_SPtr getAttributeMap();

	/**
	 * Get the incarnation number.
	 *
	 * @return the incarnation number, or -1 if not applicable.
	 */
	int64_t getIncarnationNumber() const;

	NodeStatus getNodeStatus() const;

	/**
	 * A string representation of the MetaData.
	 *
	 * @param mode determines the mode of converting the AttributeValue to a string.
	 * @return
	 */
	String toString(AttributeValue::ToStringMode mode) const;

	/**
	 * Deep copy, allocated on the heap.
	 *
	 * @param other
	 * @return
	 */
	static MetaData_SPtr clone(MetaData_SPtr other);

	/**
	 * Compare two MetaData objects for equality.
	 *
	 * @param lhs
	 * @param rhs
	 * @return
	 */
	friend
	bool operator==(const MetaData& lhs, const MetaData& rhs);

	/**
	 * Compare two MetaData objects for inequality.
	 *
	 * @param lhs
	 * @param rhs
	 * @return
	 */
	friend
	bool operator!=(const MetaData& lhs, const MetaData& rhs);

private:
	AttributeMap_SPtr attributeMap_;
	int64_t incarnationNumber_;
	NodeStatus nodeStatus_;
};

} //namespace event
} //namespace spdr

#endif /* METADATA_H_ */
