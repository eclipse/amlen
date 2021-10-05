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
 * PubSubEventListener.h
 *
 *  Created on: Jan 31, 2012
 */

#ifndef PUBSUBEVENTLISTENER_H_
#define PUBSUBEVENTLISTENER_H_

#include "PubSubEvent.h"

namespace spdr
{

namespace messaging
{

class PubSubEventListener
{
public:
	PubSubEventListener();
	virtual ~PubSubEventListener();

	virtual void onEvent(event::PubSubEvent_SPtr event) = 0;
};

}

}

#endif /* PUBSUBEVENTLISTENER_H_ */
