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
 * StreamIDImpl.cpp
 *
 *  Created on: Jan 29, 2012
 */

#include <sstream>
#include "StreamIDImpl.h"

namespace spdr
{

namespace messaging
{

StreamIDImpl::StreamIDImpl() :
		StreamID()
{
}

StreamIDImpl::~StreamIDImpl()
{
}

StreamIDImpl::StreamIDImpl(uint64_t prefix, uint64_t suffix) :
	StreamID(prefix, suffix)
{
}

StreamIDImpl::StreamIDImpl(const StreamIDImpl& other) :
	StreamID(other.streamID_.first,other.streamID_.second)
{
}

const StreamIDImpl& StreamIDImpl::operator=(const StreamIDImpl& other)
{
	if (this == &other)
	{
		return *this;
	}
	else
	{
		streamID_ = other.streamID_;
		return *this;
	}
}

std::size_t StreamIDImpl::hash_value() const
{
	return ((std::size_t) streamID_.first) + ((std::size_t) streamID_.second);
}

bool StreamIDImpl::equalsPrefix(const StreamID& other) const
{
	return streamID_.first == static_cast<const StreamIDImpl&>(other).streamID_.first;
}

bool StreamIDImpl::operator==(const StreamID& other) const
{
	return (streamID_ == static_cast<const StreamIDImpl&>(other).streamID_);
}

bool StreamIDImpl::operator!=(const StreamID& other) const
{
	return (streamID_ != static_cast<const StreamIDImpl&>(other).streamID_);
}

bool StreamIDImpl::operator<(const StreamID& other) const
{
	return (streamID_ < static_cast<const StreamIDImpl&>(other).streamID_);
}

bool StreamIDImpl::operator<=(const StreamID& other) const
{
	return (streamID_ <= static_cast<const StreamIDImpl&>(other).streamID_);
}

bool StreamIDImpl::operator>(const StreamID& other) const
{
	return (streamID_ > static_cast<const StreamIDImpl&>(other).streamID_);
}

bool StreamIDImpl::operator>=(const StreamID& other) const
{
	return (streamID_ >= static_cast<const StreamIDImpl&>(other).streamID_);
}

std::string StreamIDImpl::toString() const
{
	std::ostringstream oss;
	oss << std::hex << streamID_.first << ":" << streamID_.second;
	return oss.str();
}

}

}
