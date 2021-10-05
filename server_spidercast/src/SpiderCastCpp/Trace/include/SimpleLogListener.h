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
 * SimpleLogListener.h
 *
 *  Created on: 17/06/2010
 */

#ifndef SIMPLELOGLISTENER_H_
#define SIMPLELOGLISTENER_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "LogListener.h"

namespace spdr
{

class SimpleLogListener : public log::LogListener
{
private:
	std::string trFileName;
//	std::auto_ptr<std::ofstream> _out;
	boost::shared_ptr<std::ofstream> _out;
	void print(int log_level, const char *id, const char *message);

	boost::recursive_mutex _mutex;

public:
	SimpleLogListener(const char* logFileName = NULL);
	virtual ~SimpleLogListener();

	std::string getFileName() const
	{
		return trFileName;
	}

	/*
	 * @see spdr::LogListener
	 */
	void onLogEvent(log::Level log_level, const char *id, const char *message)
	{
		print(log_level,id,message);
	}

	static void logListener(void *user, int log_level, const char *id,
			const char *message)
	{
		SimpleLogListener* listener = (SimpleLogListener*) user;
		listener->print(log_level, id, message);
	}
};

}
#endif /* SIMPLELOGLISTENER_H_ */
