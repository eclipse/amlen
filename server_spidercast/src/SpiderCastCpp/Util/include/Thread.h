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
 * Thread.h
 *
 *  Created on: 01/03/2010
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <boost/cstdint.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "Definitions.h"
#include "ThreadControl.h"
#include "SpiderCastRuntimeError.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef XAPI
#define XAPI
#endif
/*
 * A forward declaration of the engine thread init
 */
XAPI void ism_common_makeTLS(int need, const char* name);
XAPI void ism_common_freeTLS();

#ifdef __cplusplus
}
#endif

namespace spdr
{

/**
 * Common interface for SpiderCast threads.
 */
class Thread
{
private:
	static ScTraceComponent* tc_;
	ScTraceContextImpl ctx_;

public:
	Thread(const String& instID);
	virtual ~Thread();

	virtual void start();
	virtual void finish();
	virtual bool isFinish();
	virtual void interrupt();
	virtual void join();
	virtual boost::thread::id getID();
	virtual const String getName() const = 0;

	/**
	 * Wake a thread to do some work, bit-mask may signal what type of work is pending.
	 *
	 * Bit 0 - unspecified type.
	 * Bit 1-31 - TBD, implementation specific.
	 *
	 * @param mask marks the type of work pending. must be >0.
	 * @throw IllegalArgumentException if mask==0.
	 */
	//virtual void wakeUp(uint32_t mask = (uint32_t) 1) = 0;

	virtual void operator()() = 0;

protected:
	boost::thread bThread;
	boost::recursive_mutex rMutex;
	boost::condition_variable_any conditionVar;
	bool volatile finishFlag;

};

}

#endif /* THREAD_H_ */
