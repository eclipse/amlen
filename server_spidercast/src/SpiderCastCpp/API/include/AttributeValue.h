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
 * AttributeValue.h
 *
 *  Created on: Feb 22, 2011
 */

#ifndef ATTRIBUTEVALUE_H_
#define ATTRIBUTEVALUE_H_

#include <boost/shared_array.hpp>

#include "Definitions.h"
#include "SpiderCastRuntimeError.h"

namespace spdr
{

namespace event
{

/**
 * An AttributeValue holds a read only dynamically allocated byte buffer, with a specified length, using a smart pointer.
 *
 * The AttributeValue has two members. First member is the length, second is a shared pointer to the buffer.
 * A zero length is legal and means a key mapped to an empty value, the pointer is NULL.
 *
 * Note that the AttributeValue will free memory when the last copy goes out of scope.
 *
 * The AttributeValue can be compared for equality using operators == and !=.
 *
 * Copy constructor and assignment operator perform a shallow copy.
 * For deep copy use clone().
 *
 * The buffer pointed to by the AttributeValue is shared; Do not write to it, or bad things will happen!
 *
 * @see AttributeMap
 */
class AttributeValue //: public boost::equality_comparable<AttributeValue>
{
public:
	/**
	 * Constructor of an arbitrary AttributeValue.
	 *
	 * The pointer is wrapped by a shared-pointer and managed by it.
	 *
	 * @param len
	 * @param buffer
	 * @return
	 */
	AttributeValue(const int32_t len, const char* buffer) :
		length(len), bufferSPtr(buffer)
	{
	}

	/**
	 * Constructor of an arbitrary AttributeValue.
	 *
	 * @param len
	 * @param bufferSPtr
	 * @return
	 */
	AttributeValue(const int32_t len, const boost::shared_array<const char> & bufferSPtr) :
		length(len), bufferSPtr(bufferSPtr)
	{
	}

	/**
	 * Default constructor.
	 *
	 * length = 0; pointer = NULL.
	 *
	 * @return
	 */
	AttributeValue() :
		length(0), bufferSPtr()
	{
	}

	int32_t getLength() const
	{
		return length;
	}

	boost::shared_array<const char> getBuffer() const
	{
		return bufferSPtr;
	}

	/**
	 * Determines how to convert the buffer to a string.
	 */
	enum ToStringMode
	{
		STR, 	/**< Treat like an std::string */
		CSTR,   /**< Treat like a C-string, expect a null terminated string  */
		BIN		/**< Treat like a binary buffer, print the hex value of every byte */
	};

	/**
	 * Convert to a string.
	 *
	 * The buffer length is always respected.
	 *
	 * @param mode Determines how to convert the AttributeValue to a string.
	 * @return A string representation of the buffer.
	 */
	String toString(ToStringMode mode) const;

	/**
	 * A utility to compare the equality of the buffer's string representation to a string.
	 *
	 * @param s
	 * @param mode
	 * @return true if the buffer's  string representation equals the given string.
	 */
	bool equalsString(String s, ToStringMode mode) const;

	/**
	 * Deep copy, allocate storage on the heap.
	 *
	 * @param other
	 * @return
	 */
	static AttributeValue clone(const AttributeValue& other);

	/**
	 * Compare two AttributeValue objects for equality.
	 *
	 * @param lhs
	 * @param rhs
	 * @return true if the two objects are equal.
	 */
	friend
	bool operator==(const AttributeValue& lhs, const AttributeValue& rhs);

	/**
	 * Compare two AttributeValue objects for inequality.
	 *
	 * @param lhs
	 * @param rhs
	 * @return true if the two objects are equal.
	 */
	friend
	bool operator!=(const AttributeValue& lhs, const AttributeValue& rhs);

private:
	/**
	 * Length of the buffer.
	 *
	 * A zero length is legal and means a key mapped to an empty value, the pointer is NULL.
     * A (-1) length means a null entry, and represents a missing key-value pair, the pointer is NULL.
	 */
	int32_t length;

	/**
	 * Shared pointer to a buffer.
	 */
	boost::shared_array<const char> bufferSPtr;
};
}//event
}//spdr
#endif /* ATTRIBUTEVALUE_H_ */
