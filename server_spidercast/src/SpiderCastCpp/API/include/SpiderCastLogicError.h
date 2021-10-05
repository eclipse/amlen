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
 * SpiderCastLogicError.h
 *
 *  Created on: 07/02/2010
 */

#ifndef SPIDERCASTLOGICERROR_H_
#define SPIDERCASTLOGICERROR_H_

#include <stdexcept>

#include "Definitions.h"
#include "StackBackTrace.h"

namespace spdr
{

/**
 * Super-class of all SpiderCast logic errors type exceptions.
 *
 * This class inherits from std::logic_error.
 *
 * It is thrown when the error could have been avoided by inspecting the input,
 * for example when an illegal argument is passed to a method.
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
class SpiderCastLogicError : public std::logic_error
{
private:
	boost::shared_ptr<StackBackTrace> stackBackTrace_;

public:

	/**
	 * Construct with a "what" string.
	 *
	 * @param whatString
	 * @return
	 */
	SpiderCastLogicError(const String& whatString);

	/**
	 * Copy constructor.
	 *
	 * @param other
	 * @return
	 */
	SpiderCastLogicError(const SpiderCastLogicError& other);

	/**
	 * Assignment operator.
	 *
	 * @param other
	 * @return
	 */
	SpiderCastLogicError& operator=(const SpiderCastLogicError& other);

	/**
	 * Destructor.
	 *
	 * @return
	 */
	virtual ~SpiderCastLogicError() throw(); //throw nothing

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
 * Thrown when an illegal argument is passed to method.
 */
class IllegalArgumentException: public SpiderCastLogicError
{
public:
	/**
	 * Construct with a "what" string.
	 * @param whatString
	 * @return
	 */
	IllegalArgumentException(const String& whatString);

	/**
	 * Destructor.
	 * @return
	 */
	virtual ~IllegalArgumentException() throw (); //throw nothing

};
//===================================================================

/**
 * Thrown when an operation is performed on an object with illegal state.
 */
class IllegalStateException: public SpiderCastLogicError
{
public:
	/**
	 * Construct with a "what" string.
	 * @param whatString
	 * @return
	 */
	IllegalStateException(const String& whatString);
	/**
	 * Destructor.
	 * @return
	 */
	virtual ~IllegalStateException() throw (); //throw nothing
};
//===================================================================

/**
 * Thrown when an attempt to create a configuration object fails due
 * to wrong parameters or relations between them.
 */
class IllegalConfigException: public SpiderCastLogicError
{
public:
	/**
	 * Construct with a "what" string.
	 * @param whatString
	 * @return
	 */
	IllegalConfigException(const String& whatString);
	/**
	 * Destructor.
	 * @return
	 */
	virtual ~IllegalConfigException() throw (); //throw nothing
};
//===================================================================

/**
 * Thrown when a read-only ByteBuffer is tried to be written.
 */
class BufferNotWriteableException: public SpiderCastLogicError
{
public:
	/**
	 * Construct with a "what" string.
	 *
	 * @param whatString
	 * @return
	 */
	BufferNotWriteableException(const String& whatString = String(
			"BufferNotWriteableException"));
	/**
	 * Destructor.
	 * @return
	 */
	virtual ~BufferNotWriteableException() throw (); //throw nothing
};
//===================================================================

/**
 * Thrown when a ByteBuffer (or any other sequence container that supports this)
 * is read or written past its capacity.
 */
class IndexOutOfBoundsException: public SpiderCastLogicError
{
public:
	/**
	 * Construct with a "what" string.
	 * @param whatString
	 * @return
	 */
	IndexOutOfBoundsException(const String& whatString);
	/**
	 * Destructor.
	 * @return
	 */
	virtual ~IndexOutOfBoundsException() throw (); //throw nothing
};

}//namespace spdr

#endif /* SPIDERCASTLOGICERROR_H_ */
