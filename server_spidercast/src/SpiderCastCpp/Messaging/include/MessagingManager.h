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
 * MessagingManager.h
 *
 *  Created on: Feb 1, 2012
 */

#ifndef MESSAGINGMANAGER_H_
#define MESSAGINGMANAGER_H_

#include <boost/shared_ptr.hpp>

#include "Topic.h"
#include "StreamID.h"
#include "SCMessage.h"

namespace spdr
{

namespace messaging
{

class MessagingManager
{
public:
	const static String topicKey_Prefix; //".tpc."
	const static char topicFlags_SubscriberMask = 	0x01;
	const static char topicFlags_PublisherMask = 	0x02;
	const static char topicFlags_GlobalMask = 		0x04;
	const static char topicFlags_BridgeMask = 		0x08;

	MessagingManager();
	virtual ~MessagingManager();

	/**
	 * Remove publisher from internal data-structures.
	 *
	 * @param publisherSID
	 */
	virtual void removePublisher(StreamID_SPtr publisherSID) = 0;

	/**
	 * Remove subscriber from internal data-structures.
	 *
	 * @param topic
	 */
	virtual void removeSubscriber(Topic_SPtr topic) = 0;

	virtual void removeP2PRcv() = 0;
	/**
	 *
	 * @param message
	 */
	virtual void processIncomingDataMessage(SCMessage_SPtr message) = 0;


	static char addBridgeSub_Flags(char current_flags);

	static char removeBridgeSub_Flags(char current_flags);

	static char addSub_Flags(char current_flags, bool global);

	static char removeSub_Flags(char current_flags);

	static char addPub_Flags(char current_flags);

	static char removePub_Flags(char current_flags);


};

typedef boost::shared_ptr<MessagingManager> MessagingManager_SPtr;

}

}

#endif /* MESSAGINGMANAGER_H_ */
