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
 * RxMessageImpl.cpp
 *
 *  Created on: Feb 8, 2012
 */

#include "RxMessageImpl.h"

namespace spdr
{

namespace messaging
{

RxMessageImpl::RxMessageImpl() :
	RxMessage()
{
}

RxMessageImpl::RxMessageImpl(const RxMessageImpl& other) :
	RxMessage(other)

{
}

RxMessageImpl& RxMessageImpl::operator=(const RxMessageImpl& other)
{
	if (this != &other)
	{
		RxMessage::operator =(other);
	}
	return *this;
}

RxMessageImpl::~RxMessageImpl()
{
}

void RxMessageImpl::setBuffer(const int32_t length, const char* buff)
{
	buffer_.first = length;
	buffer_.second = buff;
}

void RxMessageImpl::setBuffer(const Const_Buffer& buffer)
{
	buffer_ = buffer;
}

void RxMessageImpl::setMessageID(const int64_t messageID)
{
	messageID_ = messageID;
}

void RxMessageImpl::setStreamID(const StreamID_SPtr& sid)
{
	streamID_ = sid;
}

void RxMessageImpl::setTopic(const Topic_SPtr& topic)
{
	topic_ = topic;
}

void RxMessageImpl::setSource(const NodeID_SPtr& source)
{
	source_ = source;
}

void RxMessageImpl::reset()
{
	buffer_.first = -1;
	buffer_.second = NULL;
	messageID_ = -1;
	streamID_.reset();
	topic_.reset();
	source_.reset();
}

}

}
