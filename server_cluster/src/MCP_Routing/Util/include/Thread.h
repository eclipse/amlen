/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#ifndef MCP_THREAD_H_
#define MCP_THREAD_H_

#include <string>
#include <boost/cstdint.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "MCPExceptions.h"
#include "MCPConfig.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace mcp
{

/**
 * Common interface for MCP threads.
 *
 * TODO refrain from exceptions
 */
class Thread
{
private:
	static spdr::ScTraceComponent* tc_;
	spdr::ScTraceContextImpl ctx_;

public:
	Thread(const std::string& instID);
	virtual ~Thread();

	/**
	 * @throws MCPRuntimeError if thread already started
	 */
	virtual void start();

	virtual void finish();

	virtual bool isFinish();

	virtual void interrupt();

	virtual void join();

	virtual boost::thread::id getThreadID();

	virtual const std::string getThreadName() const = 0;

	virtual void operator()() = 0;

protected:
	boost::thread bThread;
	boost::recursive_mutex mutex;
	boost::condition_variable_any conditionVar;
	bool volatile finishFlag;

};

}

#endif /* MCP_THREAD_H_ */
