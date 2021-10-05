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
 * SpiderCastEventListener.h
 *
 *  Created on: 10/03/2010
 */

#ifndef SPIDERCASTEVENTLISTENER_H_
#define SPIDERCASTEVENTLISTENER_H_

#include <boost/shared_ptr.hpp>

#include "SpiderCastEvent.h"
#include "FatalErrorEvent.h"

namespace spdr
{

/**
 * An interface for receiving life-cycle events from the SpiderCast instance.
 *
 * An implementation of this interface is registered into the SpiderCast upon creation.
 * The onEvent() method is invoked when an event has to be delivered,
 * for example a event::FatalErrorEvent.
 *
 * Note:
 * <UL>
 * 	<LI> All events reside in the spdr::event namespace and are sub-types of event::SpiderCastEvent.
 * 	<LI> Membership events (sub-types of event::MembershipEvent) are not delivered to this listener.
 * </UL>
 */
class SpiderCastEventListener
{
public:
	SpiderCastEventListener();

	virtual ~SpiderCastEventListener();

	/**
	 * Delivers a SpiderCast life-cycle event.
	 *
	 * Use event::SpiderCastEvent#getType() to get the event type, and boost's
	 * \code
	 * template <class T,class U>
	 * shared_ptr<T> static_pointer_cast(const shared_ptr<U>& r);
	 * \endcode
	 * to cast to the correct type. For example:
	 * \code
	 * event::SpiderCastEvent_SPtr event = ...;
	 * if (event->getType() == event::Fatal_Error)
	 *   FatalErrorEvent_SPtr fee = boost::static_pointer_cast<event::FatalErrorEvent>( event );
	 * \endcode
	 * The event is in the responsibility of the application.
	 *
	 * @param event
	 */
	virtual void onEvent(event::SpiderCastEvent_SPtr event) = 0;
};

}

#endif /* SPIDERCASTEVENTLISTENER_H_ */
