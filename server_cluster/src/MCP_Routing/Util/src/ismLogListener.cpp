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

//#define TRACE_COMP SpiderCast

#include <iostream>
#include <ismLogListener.h>


namespace mcp
{

ismLogListener::ismLogListener()
{
	//std::cout << "ismLogListener()" << std::endl;
}
ismLogListener::~ismLogListener()
{
	//std::cout << "~ismLogListener()" << std::endl;
}

void ismLogListener::print(spdr::log::Level log_level, const char *id, const char *message)
{
//	std::cout << ">>> in ismLogListener::print1 " << id << std::endl;
//	int len = strlen(message);
//	std::cout << ">>> in ismLogListener::print2 " << len << std::endl;
//
//	if (len>1024)
//	{
//		std::cout << ">>> in ismLogListener::print3 " << strlen(message) << std::endl;
//	}

	using namespace spdr;
	char c;
	int ism_log_level = 9;

	switch (log_level)
	{
	case log::Level_NONE:
	    c = '-';
	    ism_log_level = 0;
	    break;

	case log::Level_ERROR:
		c = 'E';
		ism_log_level = 1;
		break;

	case log::Level_WARNING:
		ism_log_level = 2;
		c = 'W';
		break;

	case log::Level_INFO:
		ism_log_level = 3;
		c = 'I';
		break;

	case log::Level_CONFIG:
		ism_log_level = 4;
		c = 'C';
		break;

	case log::Level_EVENT:
		c = 'e';
		ism_log_level = 6;
		break;

	case log::Level_DEBUG:
		c = 'd';
		ism_log_level = 7;
		break;

	case log::Level_ENTRY:
		c = 'x';
		ism_log_level = 9;
		break;

	case log::Level_DUMP:
	    c = 'p';
	    ism_log_level = 9;
	    break;

	default:
		c = '?';
		break;
	}

	TRACE(ism_log_level," %s %c %s\n", ((id)?(id):"nil"), c, ((message)?(message):"nil"));
}

}
