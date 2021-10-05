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
 * AttributeMap.h
 *
 *  Created on: 28/06/2010
 */

#ifndef ATTRIBUTEMAP1_H_
#define ATTRIBUTEMAP1_H_

#include "Definitions.h"
#include "SpiderCastRuntimeError.h"
#include "AttributeValue.h"

namespace spdr
{

namespace event
{

class AttributeMap;
/**
 * A shared pointer to an AttributeMap.
 */
typedef boost::shared_ptr<AttributeMap >						AttributeMap_SPtr;
/**
 * A shared pointer to a const AttributeMap.
 */
typedef boost::shared_ptr<const AttributeMap >					ConstAttributeMap_SPtr;

/**
 * An attribute map.
 *
 * The AttributeMap is a std::map with String keys and AttributeValue values.
 * In addition, it supports conversion to a string representation with toString(),
 * and supports equality operators == and !=.
 *
 * The attribute map is included in the MetaData, which is delivered in some
 * MembershipEvent sub-types, as part of the MembershipService.
 */
class AttributeMap :
	public std::map<String, AttributeValue> //, public boost::equality_comparable<AttributeMap>
{
public:

	/**
	 * Default constructor.
	 *
	 * Creates an empty map.
	 *
	 * @return an empty map.
	 */
	AttributeMap();

	/**
	 * Destructor.
	 *
	 * Destroys the map and everything in it.
	 */
	virtual ~AttributeMap();

	/**
	 * A string representation of the map.
	 *
	 * @param mode Determines how to convert the AttributeValue values to a string.
	 * @return
	 */
	String toString(AttributeValue::ToStringMode mode) const;

	/**
	 * Deep copy, allocate storage on the heap.
	 *
	 * @param other
	 * @return
	 */
	static AttributeMap_SPtr clone(AttributeMap_SPtr other);

	/**
	 * Compares two AttributeMap objects for equality.
	 *
	 * Deep compare.
	 *
	 * @param map1
	 * @param map2
	 * @return true if the two maps are equal.
	 */
	friend
	bool operator==(const AttributeMap& map1, const AttributeMap& map2);

	/**
	 * Compares two AttributeMap objects for inequality.
	 *
	 * Deep compare.
	 *
	 * @param map1
	 * @param map2
	 * @return true if the two maps are equal.
	 */
	friend
	bool operator!=(const AttributeMap& map1, const AttributeMap& map2);
};

}
}

#endif /* ATTRIBUTEMAP1_H_ */
