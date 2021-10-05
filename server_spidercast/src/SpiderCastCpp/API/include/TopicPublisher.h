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
 * TopicPublisher.h
 *
 *  Created on: Jan 29, 2012
 */

#ifndef TOPICPUBLISHER_H_
#define TOPICPUBLISHER_H_

#include <boost/cstdint.hpp>

#include "StreamID.h"
#include "Topic.h"
#include "TxMessage.h"

namespace spdr
{

namespace messaging
{

/**
 * A TopicPublisher is a source for a stream of messages on a specific topic,
 * that maintains a specific quality of service.
 * <p>
 * The counterpart of the TopicPublisher  is the TopicSubscriber.
 * Together with the Topic, they provide the basic means for publish subscribe
 * communications over the overlay.
 * <p>
 * It is legal to create several publishers on the same Topic, in the same node,
 * and on different nodes. Each topic publisher is uniquely identified by a StreamID.
 * The different publishers may even have different quality of service parameters.
 * For example one publisher may maintain reliable ordered deliver, whereas another
 * may maintain best-effort delivery. That is, the QoS is maintained per stream, not per topic.
 * <p>
 * Messages sent from different publishers, on the same Topic, even if the publishers
 * have the same QoS, are not necessarily received in the order they were sent.
 * <p>
 * A TopicPublisher implementation must be created using the factory method:
 * SpiderCast#createTopicPublisher(). The TopicPublisher is configured by
 * providing a PropertyMap object to the factory method that generates the publisher.
 * <p>
 * The quality of service currently supported is: Best Effort.
 *
 * @see Topic
 * @see StreamID
 * @see TxMessage
 */
class TopicPublisher
{
public:
	virtual ~TopicPublisher();

	virtual void close() = 0;

	virtual bool isOpen() const = 0;

	virtual Topic_SPtr getTopic() const = 0;

	virtual StreamID_SPtr getStreamID() const = 0;

	virtual int64_t publishMessage(const TxMessage& message) = 0;

	virtual std::string toString() const = 0;

protected:
	TopicPublisher();
};

typedef boost::shared_ptr<TopicPublisher> TopicPublisher_SPtr;

}

}

#endif /* TOPICPUBLISHER_H_ */
