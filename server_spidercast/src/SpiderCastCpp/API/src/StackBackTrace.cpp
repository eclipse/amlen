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
 * StackBackTrace.cpp
 *
 *  Created on: 20/07/2010
 */

#include <iostream>
#include <sstream>

#include "StackBackTrace.h"

namespace spdr
{

StackBackTrace::StackBackTrace() :
		count(0),
		funcNames(NULL)
{
	//std::cout << "StackBackTrace()" << std::endl;
}

StackBackTrace::~StackBackTrace()
{
	//std::cout << "> ~StackBackTrace()" << std::endl;

#ifdef SPDR_LINUX
	if (count>0)
	{
		// Free the string pointers
		free( funcNames );
	}
#endif

	//std::cout << "< ~StackBackTrace()" << std::endl;
}

StackBackTrace* StackBackTrace::getStackBackTrace()
{
	StackBackTrace* trc = new StackBackTrace;

#ifdef SPDR_LINUX
	trc->count = backtrace( trc->tracePtrs, StackBackTrace::DEPTH );
	if (trc->count > 0)
	{
		trc->funcNames = backtrace_symbols( trc->tracePtrs, trc->count );
	}
#endif

	return trc;
}

void StackBackTrace::print()
{
	print(std::cerr);
}

void StackBackTrace::print(std::ostream& out)
{
	out << "=== StackBackTrace:" << std::endl;

#ifdef SPDR_LINUX

	if (count <= 0)
	{
		out << "Empty stack trace." << std::endl;
	}
	else
	{
		if (funcNames != NULL)
		{
			// Print the stack trace
			for ( int ii = 0; ii < count; ii++ )
			{
				out << funcNames[ii] << std::endl;
			}
		}
		else
		{
			out << "Error getting stack trace symbols, #frames=" <<  count << std::endl;
		}
	}

#else
	out << "Stack trace not supported on non-Linux platforms." << std::endl;
#endif
	out << "===" << std::endl;
}


std::string StackBackTrace::toString() const
{
	std::ostringstream trc;

	trc << "=== StackBackTrace:" << std::endl;

#ifdef SPDR_LINUX

	if (count <= 0)
	{
		trc << "Empty stack trace." << std::endl;
	}
	else
	{
		if (funcNames != NULL)
		{
			// Print the stack trace
			for( int ii = 0; ii < count; ii++ )
			{
				trc << funcNames[ii] << std::endl;
			}
		}
		else
		{
			trc << "Error getting stack trace symbols, #frames=" <<  count << std::endl;
		}
	}

#else
	trc << "Stack trace not supported on non-Linux platforms" << std::endl;
#endif

	return trc.str();
}

}
