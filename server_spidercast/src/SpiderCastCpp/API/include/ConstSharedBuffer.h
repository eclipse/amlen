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
 * ConstSharedBuffer.h
 *
 *  Created on: Apr 13, 2011
 */

#ifndef CONSTSHAREDBUFFER_H_
#define CONSTSHAREDBUFFER_H_

#include <boost/cstdint.hpp>
#include <boost/shared_array.hpp>

namespace spdr
{

/**
 *
 *
 */
class ConstSharedBuffer
{
	//TODO implement clone
public:

	/**
	 * Zero length buffer, with NULL pointer.
	 * @return
	 */
	ConstSharedBuffer();

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
	ConstSharedBuffer(uint32_t length, const char* buffer);

	/**
	 * Constructor of a buffer from a shared_array.
	 *
	 * @param length
	 * @param bufferSPtr
	 * @return
	 */
	ConstSharedBuffer(uint32_t length, boost::shared_array<const char> bufferSPtr);

	virtual ~ConstSharedBuffer();

	ConstSharedBuffer(const ConstSharedBuffer& other);

	ConstSharedBuffer& operator=(const ConstSharedBuffer& other);

	boost::shared_array<const char> getBuffer() const
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
	 * Shared pointer to a const buffer.
	 * Pointer must be allocated with new char[], as it is destroyed with delete[].
	 */
	boost::shared_array<const char> bufferSPtr_;
};
}
#endif /* CONSTSHAREDBUFFER_H_ */
