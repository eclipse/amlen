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
 * TopicSubscriberImpl.h
 *
 *  Created on: Jan 31, 2012
 */

#ifndef TOPICSUBSCRIBERIMPL_H_
#define TOPICSUBSCRIBERIMPL_H_

#include <boost/thread/recursive_mutex.hpp>

#include "TopicSubscriber.h"
#include "PubSubEventListener.h"
#include "MessageListener.h"
#include "PropertyMap.h"
#include "SpiderCastConfigImpl.h"
#include "NodeIDCache.h"
#include "CoreInterface.h"
#include "MessagingManager.h"
#include "TopicRxBestEffort.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace messaging
{

class TopicSubscriberImpl:  boost::noncopyable, public TopicSubscriber, public ScTraceContext
{
	static ScTraceComponent* tc_;

public:
	TopicSubscriberImpl(
			const String& instID,
			const SpiderCastConfigImpl& config,
			NodeIDCache& nodeIDCache,
			CoreInterface& coreInterface,
			Topic_SPtr topic,
			MessageListener& messageListener,
			PubSubEventListener& eventListener,
			const PropertyMap& propMap);

	virtual ~TopicSubscriberImpl();

	/*
	 * @see TopicSubscriber
	 */
	void close();

	/*
	 * @see TopicSubscriber
	 */
	bool isOpen();

	/*
	 * @see TopicSubscriber
	 */
	Topic_SPtr getTopic();

	/*
	 * @see TopicSubscriber
	 */
	std::string toString() const;

	//===

	/**
	 * Process an incoming pub/sub message on the given topic.
	 *
	 * The message arrives with the buffer location just after the H3 header.
	 * The StreamID, messageID, topicName, and sourceName were read and set into the RxMessage.
	 *
	 * @param message
	 */
	void processIncomingDataMessage(SCMessage_SPtr message, const SCMessage::H3HeaderStart& h3start);

private:
	const String& instID_;
	const SpiderCastConfigImpl& config_;
	NodeIDCache& nodeIDCache_;
	CoreInterface& coreInterface_;
	Topic_SPtr topic_;
	MessageListener& messageListener_;
	PubSubEventListener& eventListener_;
	const PropertyMap& propMap_;

	mutable boost::recursive_mutex mutex_;
	bool closed_;

	MessagingManager_SPtr messagingManager_SPtr;

	TopicRxBestEffort topicRxBestEffort_;

};

typedef boost::shared_ptr<TopicSubscriberImpl> TopicSubscriberImpl_SPtr;

}

}

#endif /* TOPICSUBSCRIBERIMPL_H_ */
