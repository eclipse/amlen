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
 * TopicSubscriber.h
 *
 *  Created on: Jan 29, 2012
 */

#ifndef TOPICSUBSCRIBER_H_
#define TOPICSUBSCRIBER_H_

#include "Topic.h"

namespace spdr
{

namespace messaging
{

/**
 * This interface provides the means for subscribing to a Topic.
 * <p>
 * The counterpart of the TopicSubscriber is the
 * TopicPublisher. Together with the Topic, they
 * provide the basic means for publish subscribe communications over the
 * overlay.
 * <p>
 * The intended use is for a SpiderCast instance to create a subscriber, providing a
 * topic identifier, message listener and an event listener. Messages published
 * on that topic are received by the subscriber.
 * <p>
 * An implementation of the TopicSubscriber interface is created
 * using the factory method: SpiderCast#createTopicSubscriber(). The TopicSubscriber is configured by
 * providing a PropertyMap object to the factory method that generates the subscriber.
 * <p>
 * Incoming messages are delivered to the application by the
 * MessageListener interface.
 * <p>
 * Life-cycle and abnormal events are delivered to the application by the
 * PubSubEventListener. The events delivered are all
 * encapsulated in a PubSubEvent object. See further documentation in the respective classes.
 * <p>
 * Opening a subscriber indicates the willingness to accept message streams from
 * TopicPublishers. It is the responsibility of the TopicPublisher to detect the
 * subscriber and initiate a handshake with it. The semantics and
 * quality-of-service (QoS) of any single-source stream is defined by the
 * TopicPublisher. Different publishers on the same topic may have different semantics and QoS settings.
 * <p>
 *
 * @see TopicPublisher
 */
class TopicSubscriber
{
public:
	virtual ~TopicSubscriber();

	virtual void close() = 0;

	virtual bool isOpen() = 0;

	virtual Topic_SPtr getTopic() = 0;

	virtual std::string toString() const = 0;

protected:
	TopicSubscriber();
};

typedef boost::shared_ptr<TopicSubscriber> TopicSubscriber_SPtr;

}

}

#endif /* TOPICSUBSCRIBER_H_ */
