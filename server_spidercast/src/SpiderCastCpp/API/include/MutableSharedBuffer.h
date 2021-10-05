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
 * MutableSharedBuffer.h
 *
 *  Created on: Apr 13, 2011
 */

#ifndef MUTABLESHAREDBUFFER_H_
#define MUTABLESHAREDBUFFER_H_

#include <boost/cstdint.hpp>
#include <boost/shared_array.hpp>

namespace spdr
{

/**
 *
 *
 */
class MutableSharedBuffer
{
	//TODO implement clone
public:

	/**
	 * Zero length buffer, with NULL pointer.
	 * @return
	 */
	MutableSharedBuffer();

	/**
	 * Constructor of a buffer from a raw pointer.
	 *
	 * The pointer is wrapped by a shared_array and managed by it.
	 * Pointer must be allocated with new char[], as it is destroyed with delete[].
	 *
	 * @param length the length of the buffer
	 * @param buffer
	 * @return
	 */
	MutableSharedBuffer(uint32_t length, char* buffer);

	/**
	 * Constructor of a buffer from a shared_array, shallow copy.
	 *
	 * @param length
	 * @param bufferSPtr
	 * @return
	 */
	MutableSharedBuffer(uint32_t length, boost::shared_array<char> bufferSPtr);

	virtual ~MutableSharedBuffer();

	/**
	 * Shallow copy.
	 *
	 * @param other
	 * @return
	 */
	MutableSharedBuffer(const MutableSharedBuffer& other);

	MutableSharedBuffer& operator=(const MutableSharedBuffer& other);

	boost::shared_array<char> getBuffer() const
	{
		return bufferSPtr_;
	}

	uint32_t getLength() const
	{
		return length_;
	}

private:
	/**
	 * Length of the buffer.
	 *
	 * A zero length is legal and means an empty value, the pointer is NULL.
	 */
	uint32_t length_;

	/**
	 * Shared pointer to a buffer.
	 * Pointer must be allocated with new char[], as it is destroyed with delete[].
	 */
	boost::shared_array<char> bufferSPtr_;
};

}

#endif /* MUTABLESHAREDBUFFER_H_ */
