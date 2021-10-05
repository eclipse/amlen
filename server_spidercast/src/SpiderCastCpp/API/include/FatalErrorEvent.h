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
 * FatalErrorEvent.h
 *
 *  Created on: 10/03/2010
 */

#ifndef FATALERROREVENT_H_
#define FATALERROREVENT_H_

#include <boost/shared_ptr.hpp>
#include "Definitions.h"
#include "SpiderCastEvent.h"

namespace spdr
{

namespace event
{

/**
 * A fatal error event.
 *
 * Event type: EventType#Fatal_Error.
 *
 * This event is delivered when a SpiderCast encounters a fatal error.
 * The SpiderCast instance closes down internally and frees all resources.
 * The state of the instance after this event is SpiderCast#NodeState#Error.
 *
 */
class FatalErrorEvent: public SpiderCastEvent
{

public:

	/**
	 * Constructor.
	 *
	 * @param errorCode
	 * @param info
	 * @param cause
	 */
	FatalErrorEvent(
			const ErrorCode errorCode,
			const String& info,
			boost::shared_ptr<Exception> cause = boost::shared_ptr<Exception>()) :
		SpiderCastEvent(Fatal_Error), _errorCode(errorCode), _info(info), _cause(cause)
	{
	}

	/**
	 * Distructor.
	 *
	 * @return
	 */
	virtual ~FatalErrorEvent()
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
	 * Get the exception that caused the failure, if applicable.
	 * @return
	 */
	virtual boost::shared_ptr<Exception> getCause() const
	{
		return _cause;
	}

	/**
	 * A string representation of the error.
	 * @return
	 */
	virtual String toString() const
	{
		String s = SpiderCastEvent::toString()
			+ " ErrorCode=" + errorCodeName[_errorCode]
		    + ", Info=" + _info
		    + ", Cause=" + (_cause ? _cause->what() : "");

		return s;
	}



private:
	const ErrorCode _errorCode;
	const String _info;
	boost::shared_ptr<std::exception> _cause;
};

/**
 * A boost::shared_ptr to a FatalErrorEvent.
 */
typedef boost::shared_ptr<event::FatalErrorEvent> FatalErrorEvent_SPtr;

} //namespace event

} //namespace spdr

#endif /* FATALERROREVENT_H_ */

