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
 * TraceUtils.cpp
 *
 *  Created on: Apr 6, 2010
 *      Author:
 */


#include <set>
#include <list>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

#include <string>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "TraceUtils.h"

using namespace std;

namespace spdr
{

uint64_t GetCurrentTimeMillis(void){
#ifdef WIN32
    FILETIME   filetime;
	ULARGE_INTEGER currTime;
	GetSystemTimeAsFileTime(&filetime);
	currTime.HighPart = filetime.dwHighDateTime;
	currTime.LowPart = filetime.dwLowDateTime;
	return ((currTime.QuadPart - llmBaseTime.QuadPart)/10000);
#else
    struct		timeval tv;
	uint64_t	result = 1000;
    gettimeofday(&tv, NULL);
	result *= tv.tv_sec;
	result += tv.tv_usec/1000;
    return result;
#endif
}

MT_timePrintClass::MT_timePrintClass() {
    _secTime = (int) (GetCurrentTimeMillis()/1000);
    struct tm *currTime;
    time_t ltime;
    time( &ltime );
    currTime = localtime(&ltime);
    strftime(_timeString,MAX_TIME_STRING_SIZE,"%c",currTime);
#ifndef WIN32
    _timeString[19] = 0;
#endif
}

void MT_timePrintClass::writeTimeString(std::ostream &out) {
    unsigned int msecTime = (int) (GetCurrentTimeMillis());

    if (_secTime != (msecTime / 1000)) {
        _secTime = msecTime / 1000;
        struct tm *currTime;
        time_t ltime;
// Important note: Because we take time again (using a different function),
// it may be that 'time' writes an uncorrect time (might be bigger by 1 second
// than that of '_secTime').
// It is better to leave this function as is, as this problem should not
// happen often, and it is only verbose.
        time( &ltime );
        currTime = localtime(&ltime);
        strftime(_timeString,MAX_TIME_STRING_SIZE,"%c",currTime);
#ifndef WIN32
        _timeString[19] = 0;
#endif
    }
    out << _timeString;
    out << ":";
    out.width(3);
    out.fill('0');
    out << (msecTime % 1000);
}

spdr::MT_timePrintClass globalTime;

void logListener(void *user, int log_level, const char *id, const char *message)
{
	std::ostringstream verbose;

	boost::posix_time::ptime time = boost::posix_time::microsec_clock::universal_time();
//	verbose << boost::posix_time::to_simple_string(time);
	verbose << boost::posix_time::to_iso_extended_string(time);

//	globalTime.writeTimeString(verbose);

	verbose << "\t" << boost::this_thread::get_id();
	verbose << "\t" << string(message);
	cout << verbose.str() << endl;
	//    safePrintErr(verbose.str());
}

int scSetLogListener(void *userInfo, scLogListener logListener)
{
	ScTrWriter* scWriter = ScTrWriter::createTrWriter(logListener, userInfo);
	if (ScTr::init(scWriter))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

/**
 * Use spdr::trace::ScTrConstants::Level_Mask_* to compose the log level.
 *
 * @param layers
 * @param logLevel
 * @return
 */
int scSetTraceLevel(unsigned int layers, int logLevel)
{
	ScTr::updateConfig(logLevel);
	return 1;
}

}
