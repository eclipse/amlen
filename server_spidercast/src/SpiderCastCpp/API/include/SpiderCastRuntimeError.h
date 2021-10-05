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
 * SpiderCastRuntimeError.h
 *
 *  Created on: 22/02/2010
 */

#ifndef SPIDERCASTRUNTIMEERROR_H_
#define SPIDERCASTRUNTIMEERROR_H_

#include <stdexcept>

#include "Definitions.h"
#include "StackBackTrace.h"
#include "SpiderCastEvent.h"

namespace spdr
{

/**
 * Super-class of all SpiderCast runtime error type exceptions.
 *
 * This class inherits from std::runtime_error.
 *
 * It is thrown when the error could not have been avoided by inspecting the input,
 * for example when a null pointer is encountered.
 *
 * This class extracts the stack back trace when it is constructed, and stores it internally.
 * The stack back trace can then be printed or otherwise manipulated later on,
 * when the exception is caught, for example.
 *
 * The stack back trace is only supported on some platforms (currently only Linux),
 * and only if the code was compiled accordingly.
 *
 * @see StackBackTrace
 */
class SpiderCastRuntimeError : public std::runtime_error
{
private:
	boost::shared_ptr<StackBackTrace> stackBackTrace_;


public:
	/**
	 * Construct with a "what" string.
	 * @param whatString
	 * @return
	 */
	explicit SpiderCastRuntimeError(const String& whatString);
	/**
	 * Copy constructor.
	 * @param other
	 * @return
	 */
	SpiderCastRuntimeError(const SpiderCastRuntimeError& other);
	/**
	 * Assignment operator.
	 * @param other
	 * @return
	 */
	SpiderCastRuntimeError& operator=(const SpiderCastRuntimeError& other);

	/**
	 * Destructor.
	 * @return
	 */
	virtual ~SpiderCastRuntimeError() throw(); //throw nothing

	/**
	 * Get a string representation of the stack back-trace.
	 *
	 * @return
	 */
	virtual String getStackTrace() const;

	/**
	 * Print the stack back trace to the standard error.
	 */
	virtual void printStackTrace();

	/**
	 * Print the stack back trace to the output stream.
	 *
	 * @param out output stream
	 */
	virtual void printStackTrace(std::ostream& out);
};

//===================================================================

/**
 * Thrown when an NULL pointer (or boost::shared_ptr) is used inappropriately.
 */
class NullPointerException : public SpiderCastRuntimeError
{
public:
	/**
	 * Construct with a "what" string.
	 * @param whatString
	 * @return
	 */
	NullPointerException(const String& whatString);
	/**
	 * Destructor.
	 * @return
	 */
	virtual ~NullPointerException() throw (); //throw nothing
};
//===================================================================

/**
 * Thrown when memory allocation fails.
 */
class OutOfMemoryException : public SpiderCastRuntimeError
{
public:
	/**
	 * Construct with a "what" string.
	 * @param whatString
	 * @return
	 */
	OutOfMemoryException(const String& whatString = "Out of memory");
	/**
	 * Destructor.
	 * @return
	 */
	virtual ~OutOfMemoryException() throw (); //throw nothing
};
//===================================================================

/**
 * Thrown when message unmarshaling fails.
 */
class MessageUnmarshlingException : public SpiderCastRuntimeError
{
public:
	/**
	 * Construct with a "what" string.
	 * @param whatString
	 * @return
	 */
	MessageUnmarshlingException(const String& whatString, event::ErrorCode error_code = event::Error_Code_Unspecified);
	/**
	 * Destructor.
	 * @return
	 */
	virtual ~MessageUnmarshlingException() throw (); //throw nothing

	event::ErrorCode getErrorCode() const
	{
		return errc;
	}

private:
	const event::ErrorCode errc;
};

//===================================================================

/**
 * Thrown when message marshaling fails.
 */
class MessageMarshlingException : public SpiderCastRuntimeError
{
public:
	/**
	 * Construct with a "what" string.
	 * @param whatString
	 * @return
	 */
	MessageMarshlingException(const String& whatString);
	/**
	 * Destructor.
	 * @return
	 */
	virtual ~MessageMarshlingException() throw (); //throw nothing
};

//===================================================================

/**
 * Signals that an error occurred while attempting to bind a
 * socket to a local address and port.  Typically, the port is
 * in use, or the requested local address could not be assigned.
 */
class BindException : public SpiderCastRuntimeError
{
public:
	/**
	 * Construct with a "what" string.
	 * @param whatString
	 * @return
	 */
	BindException(const String& whatString, event::ErrorCode error_code, const std::string& address, int port);
	/**
	 * Destructor.
	 * @return
	 */
	virtual ~BindException() throw (); //throw nothing

	event::ErrorCode getErrorCode() const
	{
		return errc;
	}

	const std::string& getAddress() const
	{
		return address;
	}

	const int getPort() const
	{
		return port;
	}

private:
	const event::ErrorCode errc;
	const std::string address;
	const int port;
};

//===================================================================

} //namespace spdr

#endif /* SPIDERCASTRUNTIMEERROR_H_ */
