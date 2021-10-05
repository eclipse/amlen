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

#ifndef ISMLOGLISTENER_H0_
#define ISMLOGLISTENER_H0_

#define TRACE_COMP Cluster

#include "LogListener.h"

#include <ismutil.h>

namespace mcp
{

/**
 * Log listener that writes to a file, with support for cyclic log files of limited size.
 *
 */
class ismLogListener : public spdr::log::LogListener
{
public:
	/**
	 * Log listener that writes to the messageSight global trace file.
	 *
	 *
	 * @param logFileNameBase e.g. "trace_node1"
	 * @param logFileNameSuffix e.g. "log"
	 * @param numFiles
	 * @param maxFileSizeKB
	 * @return
	 */
	ismLogListener();

	virtual ~ismLogListener();


	/*
	 * @see spdr::LogListener
	 */
	void onLogEvent(spdr::log::Level log_level, const char *id, const char *message)
	{
		print(log_level,id,message);
	}


private:

	void print(spdr::log::Level log_level, const char *id, const char *message);
};
}
#endif /* ISMLOGLISTENER_H0_ */
