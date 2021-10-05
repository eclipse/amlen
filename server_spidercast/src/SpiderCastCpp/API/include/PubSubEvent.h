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
 * PubSubEvent.h
 *
 *  Created on: Jan 31, 2012
 */

#ifndef PUBSUBEVENT_H_
#define PUBSUBEVENT_H_

#include "SpiderCastEvent.h"

namespace spdr
{

namespace event
{

class PubSubEvent : public SpiderCastEvent
{
public:
	PubSubEvent(EventType type);
	virtual ~PubSubEvent();
};

typedef boost::shared_ptr<PubSubEvent> PubSubEvent_SPtr;

}

}

#endif /* PUBSUBEVENT_H_ */
