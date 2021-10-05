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
 * MutableSharedBuffer.cpp
 *
 *  Created on: Apr 13, 2011
 */

#include "MutableSharedBuffer.h"

namespace spdr
{

MutableSharedBuffer::MutableSharedBuffer() :
		length_(0), bufferSPtr_()
{
}

MutableSharedBuffer::MutableSharedBuffer(uint32_t length, char* buffer) :
		length_(length), bufferSPtr_(buffer)
{
}

MutableSharedBuffer::MutableSharedBuffer(uint32_t length, boost::shared_array<char> bufferSPtr) :
		length_(length), bufferSPtr_(bufferSPtr)
{
}

MutableSharedBuffer::~MutableSharedBuffer()
{
}

MutableSharedBuffer::MutableSharedBuffer(const MutableSharedBuffer& other) :
		length_(other.length_), bufferSPtr_(other.bufferSPtr_)
{
}

MutableSharedBuffer& MutableSharedBuffer::operator=(const MutableSharedBuffer& other)
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
