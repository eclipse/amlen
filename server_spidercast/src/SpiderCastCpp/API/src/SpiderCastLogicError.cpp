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
 * SpiderCastLogicError.cpp
 *
 *  Created on: Jul 22, 2010
 */
#include <iostream>

#include "SpiderCastLogicError.h"

namespace spdr
{
//=== SpiderCastLogicError ===
SpiderCastLogicError::SpiderCastLogicError(const String& whatString) :
	logic_error(whatString)
{
	stackBackTrace_ = boost::shared_ptr<StackBackTrace>(StackBackTrace::getStackBackTrace());
}

SpiderCastLogicError::~SpiderCastLogicError() throw () //throw nothing
{
}

SpiderCastLogicError::SpiderCastLogicError(const SpiderCastLogicError& other) :
		std::logic_error(other.what()), stackBackTrace_(other.stackBackTrace_)
{
}

SpiderCastLogicError& SpiderCastLogicError::operator=(const SpiderCastLogicError& other)
{
	if (this == &other)
	{
		return *this;
	}
	else
	{
		logic_error::operator=(other);
		this->stackBackTrace_ = other.stackBackTrace_;
		return *this;
	}
}

String SpiderCastLogicError::getStackTrace() const
{
	if (stackBackTrace_)
	{
		return stackBackTrace_->toString();
	}
	else
		return "null";
}

void SpiderCastLogicError::printStackTrace()
{
	if (stackBackTrace_)
	{
		stackBackTrace_->print();
	}
	else
	{
		std::cerr << "StackBackTrace: null" << std::endl;
	}
}

void SpiderCastLogicError::printStackTrace(std::ostream& out)
{
	if (stackBackTrace_)
	{
		stackBackTrace_->print(out);
	}
	else
	{
		out << "StackBackTrace: null" << std::endl;
	}
}


//=== IllegalArgumentException ===


IllegalArgumentException::IllegalArgumentException(const String& whatString) :
	SpiderCastLogicError(whatString)
{
}

IllegalArgumentException::~IllegalArgumentException() throw () //throw nothing
{
}

//=== IllegalStateException ===

IllegalStateException::IllegalStateException(const String& whatString) :
	SpiderCastLogicError(whatString)
{
}

IllegalStateException::~IllegalStateException() throw () //throw nothing
{
}

//=== IllegalConfigException ===

IllegalConfigException::IllegalConfigException(const String& whatString) :
	SpiderCastLogicError(whatString)
{
}

IllegalConfigException::~IllegalConfigException() throw () //throw nothing
{
}

//=== BufferNotWriteableException ===

BufferNotWriteableException::BufferNotWriteableException(
		const String& whatString) :
	SpiderCastLogicError(whatString)
{
}

BufferNotWriteableException::~BufferNotWriteableException() throw () //throw nothing
{
}

//=== IndexOutOfBoundsException ===
IndexOutOfBoundsException::IndexOutOfBoundsException(const String& whatString) :
	SpiderCastLogicError(whatString)
{
}

IndexOutOfBoundsException::~IndexOutOfBoundsException() throw () //throw nothing
{
}

} //namespace spdr
