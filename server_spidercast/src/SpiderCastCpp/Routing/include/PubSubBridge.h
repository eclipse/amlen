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
 * PubSubBridge.h
 *
 *  Created on: Jul 23, 2012
 */

#ifndef PUBSUBBRIDGE_H_
#define PUBSUBBRIDGE_H_

#include "SCMessage.h"

namespace spdr
{

namespace route
{

class PubSubBridge
{
public:
	PubSubBridge();
	virtual ~PubSubBridge();

	/**
	 * Send a Pub/Sub message over the bridge.
	 *
	 * When a local publisher sends a message on a global topic, the broadcast/pubsub routers
	 * will use this to send over the S/D-bridges
	 *
	 * When routing a message coming from the incoming-message-Q, the routing manager
	 * will use this to send over the bridge.
	 *
	 * Sets the buffer position to data-length according to H1Header.
	 *
	 * @param message
	 * @param h2
	 * @param h1
	 */
	virtual void sendOverBridge(
			SCMessage_SPtr message,
			const SCMessage::H2Header& h2,
			const SCMessage::H1Header& h1) = 0;

};

}

}

#endif /* PUBSUBBRIDGE_H_ */
