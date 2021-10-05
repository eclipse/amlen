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
/*-----------------------------------------------------------------
 * File        : ScTr.cpp
 * Author      :
 * Version     : $Revision: 1.5 $
 * Last updated: $Date: 2015/11/18 08:37:03 $
 * Lab         : Haifa Research Lab, IBM Israel
 *-----------------------------------------------------------------
 * $Log: ScTrWriter.cpp,v $
 * Revision 1.5  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.4  2015/08/27 10:53:53 
 * change RUM log level dynamically
 *
 * Revision 1.3  2015/07/06 12:20:30 
 * improve trace
 *
 * Revision 1.2  2015/06/30 13:26:45 
 * Trace 0-8 levels, trace levels by component
 *
 * Revision 1.1  2015/03/22 12:29:19 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.7  2012/10/25 10:11:19 
 * Added copyright and proprietary notice
 *
 * Revision 1.6  2012/07/01 06:48:02 
 * Add trace interface to API
 *
 * Revision 1.5  2010/08/15 11:48:45 
 * Eliminate compilation warnings, no functional change
 *
 * Revision 1.4  2010/07/06 08:59:23 
 * *** empty log message ***
 *
 * Revision 1.3  2010/07/04 14:40:50 
 * Make it compile on Linux; remove on Linux implementaion of getCurrenTtime
 *
 * Revision 1.2  2010/06/17 14:07:57 
 * node attributes implementation
 *
 * Revision 1.1  2010/05/16 07:13:33 
 * Tracer - first version
 *
 *
 * Function: Trace writer implementation
 */

#include "TraceUtils.h"
#include "ScTrWriter.h"

#include <time.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <stdio.h> 
//#include "Thread.h"
//#include "MiscUtils.h"
//#include "DCSConstants.h"
#ifdef WIN32
#include <windows.h>
#endif

#ifdef RUM_UNIX
#include <cstring>
#endif

using namespace std;


namespace spdr
{

static const double CLOCKS_PER_MSEC = 0.001*CLOCKS_PER_SEC;
static const int CATEGORY_MASK = 0x060000;

#ifdef WIN32

typedef signed __int64		sc_int64_t;
typedef unsigned __int64	sc_uint64_t;
static ULARGE_INTEGER			  llmBaseTime;
#else

typedef int64_t				sc_int64_t;
typedef uint64_t			sc_uint64_t;
#endif
/*
#ifdef WIN32
static void initTimeBase(void) {
    SYSTEMTIME systime;
    FILETIME   filetime;
    memset(&systime, 0, sizeof(SYSTEMTIME));
    systime.wYear  = 1970;
    systime.wMonth = 1;
    systime.wDay   = 1;
    SystemTimeToFileTime(&systime, &filetime);
	llmBaseTime.HighPart = filetime.dwHighDateTime;
	llmBaseTime.LowPart = filetime.dwLowDateTime;
}
#else
#include <sys/time.h>
static void initTimeBase(void) {
}
#endif
*/


//sc_uint64_t GetCurrentTimeMillis(void){
//
////#ifdef WIN32
////    FILETIME   filetime;
////	ULARGE_INTEGER currTime;
////	GetSystemTimeAsFileTime(&filetime);
////	currTime.HighPart = filetime.dwHighDateTime;
////	currTime.LowPart = filetime.dwLowDateTime;
////	return ((currTime.QuadPart - llmBaseTime.QuadPart)/10000);
////#else
////    struct		timeval tv;
////	uint64_t	result = 1000;
////    gettimeofday(&tv, NULL);
////	result *= tv.tv_sec;
////	result += tv.tv_usec/1000;
////    return result;
////#endif
//
//	return 0;
//}

/*****************************************************************************/
class timePrintClass {
private:
    static  const int MAX_TIME_STRING_SIZE = 128;
    char _timeString[MAX_TIME_STRING_SIZE];
    long        _lastTimeMsec; 
    time_t      _currTimeSec;

public:
    timePrintClass(); 
    void writeTimeString(ostream &oStream);
};

timePrintClass::timePrintClass() {
    _lastTimeMsec = GetCurrentTimeMillis();
    struct tm *pTime;
    time( &_currTimeSec );
	pTime = localtime(&_currTimeSec);

    strftime(_timeString,MAX_TIME_STRING_SIZE,"%m:%d:%y %H:%M:%S",pTime);
}

void timePrintClass::writeTimeString(ostream &out) {
    unsigned int msecTime = GetCurrentTimeMillis();
    unsigned int diffMsec = msecTime - _lastTimeMsec;
    unsigned int diffSec = diffMsec / 1000;
    unsigned int remMsec = diffMsec % 1000;
    if (diffSec != 0) { 
        _lastTimeMsec = msecTime - remMsec;
        _currTimeSec += diffSec;
// Important note: Because we take time again (using a different function), 
// it may be that 'time' writes an uncorrect time (might be bigger by 1 second
// than that of '_secTime'). 
// It is better to leave this function as is, as this problem should not 
// happen often (if at all), and it is only verbose. 
	    struct tm *pTime;

		pTime = localtime(&_currTimeSec);

		strftime(_timeString,MAX_TIME_STRING_SIZE,"%m:%d:%y %H:%M:%S",pTime);
    }
    out << _timeString;
    out << ":";
    out.width(3);
    out.fill('0');
    out << remMsec;
}

//static timePrintClass globalTime;
/*****************************************************************************/

class ScBasicTrWriter : public ScTrWriter {
public:
    ScBasicTrWriter(scLogListener   logListener, void* userInfo):ScTrWriter(logListener,userInfo){}
    virtual ~ScBasicTrWriter() {
    }

    virtual void    writeLineToTrace(const ScTraceComponent* tc, LogLevel logLevel,
        const string &line);
    virtual void    writeLineToNLS(const ScTraceComponent* tc, LogLevel logLevel,
        const string &key, const char* formatMsg, va_list args);
	virtual void	writeRumTrace(int log_level, int32_t category, const char*, const char*);
};


void ScBasicTrWriter::writeLineToTrace(const ScTraceComponent* tc, LogLevel logLevel,
                                    const string &line) {
    char    msgId[10];

    msgId[0] = 'S';
    msgId[1] = 'P';
    msgId[2] = 'D';
    msgId[3] = 'R';
    msgId[4] = msgId[5] = msgId[6] = msgId[7] = '0';

    switch(logLevel){
    case L_ERROR:
    	msgId[8] = 'E';
    	break;

    case L_WARNING:
    	msgId[8] = 'W';
    	break;

    case L_INFO:
    	msgId[8] = 'I';
    	break;

    case L_CONFIG:
    	msgId[8] = 'C';
    	break;

    case L_EVENT:
    	msgId[8] = 'V';
    	break;

    case L_DEBUG:
    	msgId[8] = 'D';
    	break;

    case L_ENTRY:
    	msgId[8] = 'X';
    	break;

    case L_DUMP:
    	msgId[8] = 'P';
    	break;

    default:
    	msgId[8] = '?';
    }
    msgId[9] = '\0';

    _logListener(_userInfo, logLevel, msgId, line.c_str());//, category, NULL, NULL);
}

void ScBasicTrWriter::writeLineToNLS(const ScTraceComponent* tc, LogLevel logLevel,
        const string &key, const char *formatMsg, va_list args){
    
#ifdef WIN32
  //size_t len = _vscprintf( formatMsg, args ) + 1;
	size_t len = 1024;
  auto_ptr<char>  buffer(new char[len]);

  vsprintf( buffer.get(), formatMsg, args );
#else
  int len = 10000;
  auto_ptr<char>  buffer(new char[len]);
  vsnprintf( buffer.get(), len, formatMsg, args );
#endif
  //int32_t category = CATEGORY_MASK | tc->getSubcomponentID();
  _logListener(_userInfo, logLevel, key.c_str(), buffer.get());//, category, NULL, NULL);
  delete [](buffer.release());
}

void	ScBasicTrWriter::writeRumTrace(int logLevel, int32_t category, const char *msgKey, const char *message)
{
  _logListener(_userInfo, logLevel, msgKey, message);//, category, NULL, NULL);
}

//=== DefaultLogListener ==============================================

DefaultLogListener::DefaultLogListener(const char* logFileName)
{
	char trFileName[1024];
	if (logFileName == NULL)
	{
		sprintf(trFileName, "trace%u.log", (unsigned int)time(NULL));
	}
	else
	{
		strcpy(trFileName, logFileName);
	}
	//string mutexName("DefaultLogListener:");
	//_mutex = std::auto_ptr<Mutex>(Mutex::createMutex(mutexName+trFileName));
	_out = auto_ptr<ofstream> (new ofstream(trFileName));
}

void DefaultLogListener::print(int logLevel, const char *id, const char *message){
    ostringstream   strBuff;

    //start_synchronized_block(_mutex.get())

    strBuff << "["; 
    globalTime.writeTimeString(strBuff);
    strBuff << "] ";
    strBuff << setw(8);
    strBuff.fill('0');
    //strBuff << Thread::getCurrentThreadId() << " ";
    strBuff.fill(' ');
    char c;
    switch(logLevel){
//        case L_AUDIT:
//            c = 'A';
//            break;
        case L_ERROR:
        //case L_FATAL_ERROR:
            c = 'E';
            break;
        case L_WARNING:
            c = 'W';
            break;
        case L_INFO:
            c = 'I';
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
//        case L_DUMP:
//            c = '#';
//            break;
        default:
            c = '?';
            break;
    }
    strBuff << id << ' ' << c << ' ' << message << endl;// << ends;
    *_out << strBuff.str();
//    end_synchronized_block(_mutex.get())
}

//=== DefaultLogListener ==============================================

/*
auto_ptr<ScTrWriter> ScTrWriter::createTrWriter(scLogListener logListener, void* userInfo){
    return auto_ptr<ScTrWriter>(new ScBasicTrWriter(logListener,userInfo));
}
*/

ScTrWriter* ScTrWriter::createTrWriter(scLogListener logListener, void* userInfo){
    return new ScBasicTrWriter(logListener,userInfo);
}

void ScTrWriter::updateTrWriter(auto_ptr<ScTrWriter>& writer, scLogListener logListener, void* userInfo){
    //if(writer.isNullPtr()){
	if(writer.get() == NULL){
        writer.reset(new ScBasicTrWriter(logListener,userInfo));
        return;
    }
    writer->updateLogListener(logListener,userInfo);
}

void ScTrWriter::updateLogListener(scLogListener logListener, void* userInfo){
    _logListener = logListener;
    _userInfo = userInfo;
}
}
