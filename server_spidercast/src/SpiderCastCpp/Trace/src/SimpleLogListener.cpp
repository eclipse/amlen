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
 * SimpleLogListener.cpp
 *
 *  Created on: 17/06/2010
 */

#include <fstream>
#include <memory>
#include <iostream>
#include <sstream>


#include <boost/date_time/posix_time/posix_time.hpp>

#include "ScTraceComponent.h"
#include "ScTrWriter.h"

#include "SimpleLogListener.h"

namespace spdr
{

SimpleLogListener::SimpleLogListener(const char* logFileName)
{
	if (logFileName == NULL)
	{
		trFileName = "trace_";
		trFileName.append(boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time()));
		trFileName.append(".log");
	}
	else
	{
		trFileName.append(logFileName);
	}

	_out = std::auto_ptr<std::ofstream>(new std::ofstream(trFileName.c_str()));

	//_mutex = std::auto_ptr<Mutex>(Mutex::createMutex(mutexName+trFileName));
}

SimpleLogListener::~SimpleLogListener()
{
	std::cout << "> ~SimpleLogListener()" << std::endl;

	_out->flush();
	_out->close();

	std::cout << "< ~SimpleLogListener()" << std::endl;
}

void SimpleLogListener::print(int logLevel, const char *id, const char *message)
{
	std::ostringstream strBuff;

	{	//start_synchronized_block
		boost::recursive_mutex::scoped_lock lock(_mutex);

		strBuff << "[";

		boost::posix_time::ptime time =
				boost::posix_time::microsec_clock::local_time();
		strBuff << boost::posix_time::to_iso_extended_string(time);

		strBuff << "] ";

		strBuff << boost::this_thread::get_id() << " ";
		strBuff.fill(' ');
		char c;
		switch (logLevel)
		{

		case L_ERROR:
			c = 'E';
			break;
		case L_WARNING:
			c = 'W';
			break;
		case L_INFO:
			c = 'I';
			break;
		case L_CONFIG:
			c = 'C';
			break;

		case L_ENTRY:
			c = 'x';
			break;
		case L_EVENT:
			c = 'e';
			break;
		case L_DEBUG:
			c = 'd';
			break;
		case L_DUMP:
			c = 'p';
			break;
		default:
			c = '?';
			break;
		}
		strBuff << id << ' ' << c << ' ' << message << std::endl;// << std::ends;
		*_out << strBuff.str();
	} 	//end_synchronized_block
}
} //namespace spdr

