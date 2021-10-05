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
 * MembershipListener.h
 *
 *  Created on: 21/03/2010
 */

#ifndef MEMBERSHIPLISTENER_H_
#define MEMBERSHIPLISTENER_H_

#include <boost/shared_ptr.hpp>

#include "MembershipEvent.h"

namespace spdr
{

/**
 * An interface for receiving membership events.
 *
 * The application using the MembershipService must implement this interface in order to
 * receive membership events.
 *
 * @see MembershipService
 * @see MembershipEvent
 */
class MembershipListener
{
public:
	MembershipListener();
	virtual ~MembershipListener();

	/**
	 * Delivers a membership event.
	 *
	 * Use SpiderCastEvent#getType() to get the event type, and boost's
	 * <p>
	 * 		template <class T,class U>
	 * 		shared_ptr<T> static_pointer_cast(const shared_ptr<U>& r);
	 * <p>
	 * to cast to the correct type.
	 *
	 * <p>
	 * <b>Note:</b> The event and all the objects pointed by it are in the
	 * responsibility and ownership of the application.
	 *
	 * @param event
	 *
	 * @see MembershipEvent
	 */
	virtual void onMembershipEvent(event::MembershipEvent_SPtr event) = 0;

};


}

#endif /* MEMBERSHIPLISTENER_H_ */
