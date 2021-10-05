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
 * WarningEvent.h
 *
 *  Created on: 03/05/2015
 */

#ifndef SPDR_WARNINGEVENT_H_
#define SPDR_WARNINGEVENT_H_

#include <boost/shared_ptr.hpp>
#include "Definitions.h"
#include "SpiderCastEvent.h"

namespace spdr
{

namespace event
{

/**
 * A warning event.
 *
 * Event types:
 * (1) EventType#Warning_Connection_Refused.
 * (2) EventType#Warning_Message_Refused.
 *
 * <p>
 *
 * Warning_Connection_Refused
 * <p>
 * This event is delivered when a SpiderCast node rejects an incoming connection
 * due to any one of several reasons. For example: (1) the bus name does not match,
 * (2) the target name does not match the recipient's node name, or (3) the connect message
 * cannot be parsed.
 *
 * These events indicate a TCP connection was established to the recipient node, but that some
 * of the data received in the connect handshake did not match the expected format/protocol.
 *
 * These events may indicate a bad configuration on one one (or both) of the peers, or some
 * other networking activity that collides  with SpiderCast.
 *
 * <p>
 * Warning_Message_Refused
 * <p>
 * This event is delivered when a SpiderCast node drops an incoming UDP message
 * due to any one of several reasons. For example: (1) the bus name does not match,
 * (2) the target name does not match the recipient's node name, or (3) the message
 * cannot be parsed.
 *
 * These events indicate a UDP message was received by node, but that some
 * of the data received in the handshake did not match the expected format/protocol.
 *
 * <p>
 * These events may indicate a bad configuration on one (or both) of the peers, or some
 * other networking activity that collides  with SpiderCast.
 *
 */
class WarningEvent: public SpiderCastEvent
{

public:

	/**
	 * Constructor.
	 *
	 * @param errorCode
	 * @param info
	 */
	WarningEvent(
			const EventType type,
			const ErrorCode errorCode,
			const String& info) :
		SpiderCastEvent(type), _errorCode(errorCode), _info(info)
	{
	}

	/**
	 * Distructor.
	 *
	 * @return
	 */
	virtual ~WarningEvent()
	{
	}

	/**
	 * Get the error code.
	 * @return
	 */
	virtual ErrorCode getErrorCode() const
	{
		return _errorCode;
	}

	/**
	 * Get additional text information.
	 * @return
	 */
	virtual String getInfo() const
	{
		return _info;
	}

	/**
	 * A string representation of the error.
	 * @return
	 */
	virtual String toString() const
	{
		String s = SpiderCastEvent::toString()
			+ " ErrorCode=" + errorCodeName[_errorCode]
		    + ", Info=" + _info;

		return s;
	}



private:
	const ErrorCode _errorCode;
	const String _info;
};

/**
 * A boost::shared_ptr to a WarningEvent.
 */
typedef boost::shared_ptr<event::WarningEvent> WarningEvent_SPtr;

} //namespace event

} //namespace spdr

#endif /* SPDR_WARNINGEVENT_H_ */

