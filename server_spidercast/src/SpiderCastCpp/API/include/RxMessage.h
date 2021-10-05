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
 * RxMessage.h
 *
 *  Created on: Jan 29, 2012
 */

#ifndef RXMESSAGE_H_
#define RXMESSAGE_H_

#include "Definitions.h"
#include "StreamID.h"
#include "Topic.h"
#include "NodeID.h"

namespace spdr
{

namespace messaging
{

/**
 * An incoming message.
 *
 * This class represents an incoming message.
 * The message contains two parts: the body, and associated meta-data.
 *
 * The body is accessed by the getBuffer() call.
 *
 * The meta-data includes the message sequence number, topic, stream ID, and source ID.
 *
 * TODO add getter for BusName
 */
class RxMessage
{
public:
	RxMessage();
	RxMessage(const RxMessage& other);
	RxMessage& operator=(const RxMessage& other);

	virtual ~RxMessage();

	/**
	 * Get the message content.
	 *
	 * This buffer belongs to the SpiderCast instance. It should not be freed or written to.
	 * It can be read safely only within the scope of the onMessage() call that delivered
	 * the current message. If the message need to be accesses later it needs to be copied.
	 * The length of the body can be zero in which case the pointer is NULL.
	 *
	 * @return
	 */
	virtual const Const_Buffer& getBuffer() const;

	/**
	 * The message sequence number.
	 *
	 * @return
	 */
	virtual int64_t getMessageID() const;

	/**
	 * The transport stream ID.
	 *
	 * @return
	 */
	virtual StreamID_SPtr getStreamID() const;

	/**
	 * The topic, if applicable.
	 *
	 * @return the topic, or null.
	 */
	virtual Topic_SPtr getTopic() const;

	/**
	 * The source node.
	 *
	 * @return the source NodeID.
	 */
	virtual NodeID_SPtr getSource() const;

	/**
	 * A string representation.
	 *
	 * @return
	 */
	virtual String toString() const;

protected:
	Const_Buffer buffer_;
	int64_t messageID_;
	StreamID_SPtr streamID_;
	Topic_SPtr topic_;
	NodeID_SPtr source_;
};

}

}

#endif /* RXMESSAGE_H_ */
