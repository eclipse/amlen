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
 * RxMessage.cpp
 *
 *  Created on: Jan 29, 2012
 */

#include "RxMessage.h"

namespace spdr
{

namespace messaging
{

RxMessage::RxMessage() :
	buffer_(-1, NULL), messageID_(-1), streamID_(), topic_(), source_()
{
}

RxMessage::RxMessage(const RxMessage& other) :
	buffer_(other.buffer_),
	messageID_(other.messageID_),
	streamID_(other.streamID_),
	topic_(other.topic_),
	source_(other.source_)
{
}

RxMessage& RxMessage::operator=(const RxMessage& other)
{
	if (this != &other)
	{
		buffer_ = other.buffer_;
		messageID_ = other.messageID_;
		streamID_ = other.streamID_;
		topic_ = other.topic_;
		source_ = other.source_;
	}
	return *this;
}

RxMessage::~RxMessage()
{
}

const Const_Buffer& RxMessage::getBuffer() const
{
	return buffer_;
}

int64_t RxMessage::getMessageID() const
{
	return messageID_;
}

StreamID_SPtr RxMessage::getStreamID() const
{
	return streamID_;
}

Topic_SPtr RxMessage::getTopic() const
{
	return topic_;
}

NodeID_SPtr RxMessage::getSource() const
{
	return source_;
}

String RxMessage::toString() const
{
	String s("Topic=");
	s.append(spdr::toString(topic_));
	//TODO
	return s;
}

}

}
