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
 * StackBackTrace.h
 *
 *  Created on: 20/07/2010
 */

#ifndef STACKBACKTRACE_H_
#define STACKBACKTRACE_H_

#include <iosfwd>
#include <string>

#ifdef SPDR_LINUX
#include <cstdio>
#include <cstdlib>
#include <execinfo.h>
#endif

namespace spdr
{

/**
 * Extracts and holds the stack back trace.
 *
 * Currently supported on Linux only.
 * Compile with debug information (-g) and linker option (-rdynamic).
 * On Linux set environment variable SPDR_LINUX.
 *
 * Allocates heap memory to hold the function name strings.
 */
class StackBackTrace
{
private:
	static const int DEPTH = 100;
	void* tracePtrs[DEPTH];
	int count;
	char** funcNames;

	StackBackTrace();
	StackBackTrace(const StackBackTrace&);
	StackBackTrace& operator=(const StackBackTrace&);

public:

	/**
	 * Destructor.
	 *
	 * @return
	 */
	virtual ~StackBackTrace();

	/**
	 * Create the stack trace of the calling thread.
	 *
	 * This method allocates heap memory to store the function name strings.
	 *
	 * @return pointer to a new stack back trace object
	 */
	static StackBackTrace* getStackBackTrace();

	/**
	 * Print the stack trace to the standard error.
	 */
	void print();

	/**
	 * Print the stack trace to the output stream.
	 */
	void print(std::ostream& out);

	/**
	 * Get the stack trace in a string.
	 * @return
	 */
	std::string toString() const;
};

}

#endif /* STACKBACKTRACE_H_ */
