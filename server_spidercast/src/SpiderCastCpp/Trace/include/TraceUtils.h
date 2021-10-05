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
 * TraceUtils.h
 *
 *  Created on: Apr 6, 2010
 *      Author:
 */

#ifndef TRACEUTILS_H_
#define TRACEUTILS_H_

#include <boost/cstdint.hpp>

#include <ScTr.h>
#include <ScTrWriter.h>
#include <ScTraceBuffer.h>
#include <ScTraceComponent.h>
#include <ScTraceContext.h>
#include <ScTrConstants.h>

namespace spdr
{

#ifdef WIN32
#include <windows.h>
//typedef signed __int64		sc_int64_t;
//typedef unsigned __int64	sc_uint64_t;
static ULARGE_INTEGER			  llmBaseTime;
#else

//typedef int64_t				sc_int64_t;
//typedef uint64_t			sc_uint64_t;
#endif

uint64_t GetCurrentTimeMillis(void);

class MT_timePrintClass {
private:
    static  const int MAX_TIME_STRING_SIZE = 128;
    char _timeString[MAX_TIME_STRING_SIZE];
    unsigned int _secTime;

public:
    MT_timePrintClass();
    void writeTimeString(std::ostream &oStream);
};

extern MT_timePrintClass globalTime;

void logListener(void *user, int log_level, const char *id, const char *message);

int scSetLogListener(void *userInfo, scLogListener logListener);

int scSetTraceLevel(unsigned int layers, int logLevel);

//extern const ScTraceComponent* TC;

}
#endif /* TRACEUTILS_H_ */
