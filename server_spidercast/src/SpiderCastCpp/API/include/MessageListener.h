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
 * MessageListener.h
 *
 *  Created on: Jan 26, 2012
 */

#ifndef MESSAGELISTENER_H_
#define MESSAGELISTENER_H_

#include "RxMessage.h"

namespace spdr
{

namespace messaging
{

class MessageListener
{
public:
	MessageListener();
	virtual ~MessageListener();

	virtual void onMessage(const RxMessage& message) = 0;

private:
	String name_;
};

}
}

#endif /* MESSAGELISTENER_H_ */
