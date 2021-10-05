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
 * SpiderCastRuntimeError.cpp
 *
 *  Created on: Jul 22, 2010
 */

#include <iostream>
#include "SpiderCastRuntimeError.h"

namespace spdr
{

//=== SpiderCastRuntimeError ===

SpiderCastRuntimeError::SpiderCastRuntimeError(const String& whatString) :
	runtime_error(whatString)
{
	//std::cout << "> SpiderCastRuntimeError()" << std::endl;
	stackBackTrace_ = boost::shared_ptr<StackBackTrace>(StackBackTrace::getStackBackTrace());
}

SpiderCastRuntimeError::SpiderCastRuntimeError(
		const SpiderCastRuntimeError& other) :
	runtime_error(other.what()), stackBackTrace_(other.stackBackTrace_)
{
	//std::cout << "SpiderCastRuntimeError(other)" << std::endl;
}

SpiderCastRuntimeError& SpiderCastRuntimeError::operator=(const SpiderCastRuntimeError& other)
{
	if (this == &other)
	{
		return *this;
	}
	else
	{
		runtime_error::operator=(other);
		this->stackBackTrace_ = other.stackBackTrace_;
		return *this;
	}
}

SpiderCastRuntimeError::~SpiderCastRuntimeError() throw () //throw nothing
{
	//std::cout << "~SpiderCastRuntimeError()" << std::endl;
}

String SpiderCastRuntimeError::getStackTrace() const
{
	if (stackBackTrace_)
	{
		return stackBackTrace_->toString();
	}
	else
		return "null";
}

void SpiderCastRuntimeError::printStackTrace()
{
	if (stackBackTrace_)
	{
		stackBackTrace_->print();
	}
	else
	{
		std::cerr << "null" << std::endl;
	}
}


void SpiderCastRuntimeError::printStackTrace(std::ostream& out)
{
	if (stackBackTrace_)
	{
		stackBackTrace_->print(out);
	}
	else
	{
		out << "null" << std::endl;
	}
}


//=== NullPointerException ===
NullPointerException::NullPointerException(const String& whatString) :
	SpiderCastRuntimeError(whatString)
{
}
NullPointerException::~NullPointerException() throw () //throw nothing
{
}

//=== OutOfMemoryException ===
OutOfMemoryException::OutOfMemoryException(const String& whatString) :
	SpiderCastRuntimeError(whatString)
{
}

OutOfMemoryException::~OutOfMemoryException() throw () //throw nothing
{
}

//=== MessageUnmarshlingException ===
MessageUnmarshlingException::MessageUnmarshlingException(
		const String& whatString, event::ErrorCode error_code) :
	SpiderCastRuntimeError(whatString), errc(event::Error_Code_Unspecified)
{
}

MessageUnmarshlingException::~MessageUnmarshlingException() throw () //throw nothing
{
}

//=== MessageMarshlingException ===
MessageMarshlingException::MessageMarshlingException(const String& whatString) :
	SpiderCastRuntimeError(whatString)
{
}

MessageMarshlingException::~MessageMarshlingException() throw () //throw nothing
{
}

//=== BindException ===
BindException::BindException(const String& whatString,
		event::ErrorCode error_code, const std::string& address, int port) :
		SpiderCastRuntimeError(whatString), errc(error_code),
		address(address), port(port)
{
}

BindException::~BindException() throw () //throw nothing
{
}

} //namespace spdr
