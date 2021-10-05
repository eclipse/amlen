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
 * SCMembershipListener.h
 *
 *  Created on: 21/03/2010
 */

#ifndef SCMEMBERSHIPLISTENER_H_
#define SCMEMBERSHIPLISTENER_H_

#include <boost/shared_ptr.hpp>

#include "SCMembershipEvent.h"

namespace spdr
{

/**
 * An interface for receiving internal membership events.
 *
 * The an internal component consuming membership events must implement this interface in order to
 * receive membership events.
 *
 * @see SCMembershipEvent
 */
class SCMembershipListener
{
public:
	SCMembershipListener();
	virtual ~SCMembershipListener();

	/**
	 * Delivers a membership event.
	 *
	 * <b>Note:</b> The event can be copied. The mutable objects pointed by it are in the
	 * responsibility and ownership of the recipient. The immutable objects are shared.
	 *
	 * @param event
	 *
	 * @see SCMembershipEvent
	 */
	virtual void onMembershipEvent(const SCMembershipEvent& event) = 0;

};


}

#endif /* SCMEMBERSHIPLISTENER_H_ */
