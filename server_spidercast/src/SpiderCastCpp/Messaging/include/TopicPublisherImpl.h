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
 * TopicPublisherImpl.h
 *
 *  Created on: Jan 31, 2012
 */

#ifndef TOPICPUBLISHERIMPL_H_
#define TOPICPUBLISHERIMPL_H_

#include <boost/noncopyable.hpp>
#include <boost/operators.hpp>

#include "TopicPublisher.h"
#include "PubSubEventListener.h"
#include "PropertyMap.h"
#include "SpiderCastConfigImpl.h"
#include "NodeIDCache.h"
#include "CoreInterface.h"
#include "TopicImpl.h"
#include "RoutingManager.h"
#include "PubSubRouter.h"
#include "BroadcastRouter.h"
#include "MessagingManager.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace messaging
{

class TopicPublisherImpl: boost::noncopyable, public TopicPublisher, public ScTraceContext,
	public boost::equality_comparable<TopicPublisherImpl>, public boost::less_than_comparable<TopicPublisherImpl>
{
private:
	static ScTraceComponent* tc_;

public:
	TopicPublisherImpl(
			const String& instID,
			const SpiderCastConfigImpl& config,
			NodeIDCache& nodeIDCache,
			CoreInterface& coreInterface,
			Topic_SPtr topic,
			PubSubEventListener& eventListener,
			const PropertyMap& propMap,
			StreamID_SPtr streamID);

	virtual ~TopicPublisherImpl();

	//=== TopicPublisher ==================================
	/*
	 * @see TopicPublisher
	 */
	void close();

	/*
	 * @see TopicPublisher
	 */
	bool isOpen() const;

	/*
	 * @see TopicPublisher
	 */
	Topic_SPtr getTopic() const;

	/*
	 * @see TopicPublisher
	 */
	StreamID_SPtr getStreamID() const;

	/*
	 * @see TopicPublisher
	 */
	int64_t publishMessage(const TxMessage& message);

	/*
	 * @see TopicPublisher
	 */
	std::string toString() const;

	friend
	bool operator<(const TopicPublisherImpl& lhs, const TopicPublisherImpl& rhs);

	friend
	bool operator==(const TopicPublisherImpl& lhs, const TopicPublisherImpl& rhs);

	//=====================================================

protected:

	const String& instID_;
	const SpiderCastConfigImpl& config_;
	NodeIDCache& nodeIDCache_;
	CoreInterface& coreInterface_;
	TopicImpl_SPtr topic_;
	PubSubEventListener& eventListener_;
	const PropertyMap& propMap_;
	StreamID_SPtr streamID_;

	mutable boost::recursive_mutex mutex_;
	bool closed_;
	bool init_done_;

	MessagingManager_SPtr messagingManager_SPtr;
	route::RoutingManager_SPtr routingManager_SPtr;
	route::BroadcastRouter& broadcastRouter_;
	route::PubSubRouter& pubsubRouter_;

	int64_t next_msg_num_;
	SCMessage_SPtr outgoingDataMsg_;
	std::size_t header_size_;

	SCMessage::MessageRoutingProtocol routingProtocol_;


};

typedef boost::shared_ptr<TopicPublisherImpl> TopicPublisherImpl_SPtr;

}

}

#endif /* TOPICPUBLISHERIMPL_H_ */
