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
 * ConstSharedBuffer.cpp
 *
 *  Created on: Apr 13, 2011
 */

#include "ConstSharedBuffer.h"

namespace spdr
{

ConstSharedBuffer::ConstSharedBuffer() :
		length_(0), bufferSPtr_()
{
}

ConstSharedBuffer::ConstSharedBuffer(uint32_t length, const char* buffer) :
		length_(length), bufferSPtr_(buffer)
{
}

ConstSharedBuffer::ConstSharedBuffer(uint32_t length, boost::shared_array<const char> bufferSPtr) :
		length_(length), bufferSPtr_(bufferSPtr)
{
}

ConstSharedBuffer::~ConstSharedBuffer()
{
}

ConstSharedBuffer::ConstSharedBuffer(const ConstSharedBuffer& other) :
		length_(other.length_), bufferSPtr_(other.bufferSPtr_)
{
}

ConstSharedBuffer& ConstSharedBuffer::operator=(const ConstSharedBuffer& other)
{
	if (this == &other)
	{
		return *this;
	}

	length_ = other.length_;
	bufferSPtr_ = other.bufferSPtr_;

	return *this;
}

}
