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
 * File        : ScTrWriter.h
 * Author      :
 * Version     : $Revision: 1.7 $
 * Last updated: $Date: 2015/11/18 08:37:01 $
 * Lab         : Haifa Research Lab, IBM Israel
 *-----------------------------------------------------------------
 * $Log: ScTrWriter.h,v $
 * Revision 1.7  2015/11/18 08:37:01 
 * Update copyright headers
 *
 * Revision 1.6  2015/09/09 06:38:57 
 * clean code
 *
 * Revision 1.5  2015/08/13 14:44:47
 * remove comment in #else for gcc 5
 *
 * Revision 1.4  2015/07/06 12:20:30 
 * improve trace
 *
 * Revision 1.3  2015/06/30 13:26:45 
 * Trace 0-8 levels, trace levels by component
 *
 * Revision 1.2  2015/03/22 14:43:20 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:19 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.4  2012/10/25 10:11:16 
 * Added copyright and proprietary notice
 *
 * Revision 1.3  2012/07/01 06:48:02 
 * Add trace interface to API
 *
 * Revision 1.2  2010/06/17 14:07:56 
 * node attributes implementation
 *
 * Revision 1.1  2010/05/16 07:13:34
 * Tracer - first version
 *
 *
 * Function: Trace writer declaration
 */

#ifndef SC_TR_WRITER_H
#define SC_TR_WRITER_H

#include <stdarg.h>
#include <fstream>

#include "ScTraceComponent.h"
#include "LogListener.h"

//
//#ifdef WIN32
//#   ifndef   int32_t
//#   define   int32_t __int32
//#   endif
//#else
//#   ifndef   int32_t
//#   define   int32_t   int
//#   endif
//#endif
//
//#ifdef WIN32
//#   ifndef   uint64_t
//#   define   uint64_t unsigned __int64
//#   endif
//#else
//#   ifndef   uint64_t
//#   define   uint64_t unsigned long long
//#   endif
//#endif
//

#define SC_LOGLEV int

namespace spdr
{
typedef enum LOG_LEVEL
{
	//L_AUDIT = 1,
	L_FATAL_ERROR = spdr::log::Level_ERROR, //1
	L_ERROR = spdr::log::Level_ERROR, //1
	L_WARNING = spdr::log::Level_WARNING, //2
	L_INFO = spdr::log::Level_INFO, //3,
	L_CONFIG = spdr::log::Level_CONFIG, //4
	L_EVENT = spdr::log::Level_EVENT, //5,
	L_DEBUG = spdr::log::Level_DEBUG, //6,
	L_ENTRY = spdr::log::Level_ENTRY, //7,
	L_EXIT = spdr::log::Level_ENTRY, //7,
	L_DUMP = spdr::log::Level_DUMP, //8,
	L_XTRACE = spdr::log::Level_DUMP //8,
} LogLevel;

// DON'T NEED THIS. WAS FOR RCMS?
extern "C" typedef void (*scLogListener)(void *user, int log_level,
		const char *id, const char *message);
/*
 extern "C" typedef void (* scLogListener)(void * userData, const SC_LOGLEV level, const char * id, const char * msg,
 int32_t category, const char * types, const uint64_t * replacement);
 */

class DefaultLogListener
{
private:
	std::auto_ptr<std::ofstream> _out;
	void print(int log_level, const char *id, const char *message);
	//std::auto_ptr<Mutex>           _mutex;
public:
	DefaultLogListener(const char* logFileName = NULL);

	~DefaultLogListener()
	{
		_out->flush();
		_out->close();
	}

	static void logListener(void *user, int log_level, const char *id,
			const char *message)
	{
		DefaultLogListener* listener = (DefaultLogListener*) user;
		listener->print(log_level, id, message);
	}
};

class ScTrWriter
{
protected:
	scLogListener _logListener;
	void* _userInfo;

public:
	ScTrWriter(scLogListener logListener, void* userInfo) :
		_logListener(logListener), _userInfo(userInfo)
	{
	}

	virtual ~ScTrWriter()
	{
	}

	virtual void writeLineToTrace(const ScTraceComponent* tc,
			LogLevel logLevel, const std::string &line) = 0;
	virtual void writeLineToNLS(const ScTraceComponent* tc, LogLevel logLevel,
			const std::string &key, const char* formatMsg, va_list args) = 0;
	virtual void writeRumTrace(int log_level, int32_t category,
			const char *msgKey, const char *message) = 0;
	//static std::auto_ptr<ScTrWriter> createTrWriter(scLogListener  logListener, void* userInfo);
	static ScTrWriter
			* createTrWriter(scLogListener logListener, void* userInfo);
	static void updateTrWriter(std::auto_ptr<ScTrWriter>& writer,
			scLogListener logListener, void* userInfo);
	std::pair<scLogListener, void*> getLogListener(void) const
	{
		return std::pair<scLogListener, void*>(_logListener, _userInfo);
	}
	void updateLogListener(scLogListener logListener, void* userInfo);
};

//typedef std::auto_ptr<ScTrWriter> ScTrWriterRCSPtr;
typedef ScTrWriter* ScTrWriterRCSPtr;

}

#endif
