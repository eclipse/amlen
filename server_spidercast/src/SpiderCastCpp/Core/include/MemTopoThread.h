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
 * MemTopoThread.h
 *
 *  Created on: 01/03/2010
 */

#ifndef MEMTOPOTHREAD_H_
#define MEMTOPOTHREAD_H_

#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

#include "Thread.h"
#include "ThreadControl.h"
#include "CoreInterface.h"
//#include "ConcurrentSharedPtr.h"
#include "TaskSchedule.h"
#include "IncomingMsgQ.h"
#include "EnumCounter.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class MemTopoThread;
typedef boost::shared_ptr<MemTopoThread> MemTopoThread_SPtr;

class MemTopoThread : public Thread, public ThreadControl, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:

	MemTopoThread(const String& instID, SpiderCastConfigImpl& config, CoreInterface& coreIfc);
	virtual ~MemTopoThread();

	/**
	 * @see ThreadControl
	 *
	 * Bit 0 - unspecified work, check all
	 * Bit 1 - task schedule
	 * Bit 2 - incoming membership Q
	 * Bit 3 - incoming topology Q
	 *
	 * @param mask
	 */
	void wakeUp(uint32_t mask = (uint32_t)1 );

	/*
	 * @see Thread
	 */
	const String getName() const;

	/*
	 * @see Thread
	 */
	void operator()();

//	/*
//	 * Override ScTraceContext from Thread.
//	 */
//	const ScTraceComponent* getTraceComponent() const
//	{
//		return tc_;
//	}

	/**
	 * Print a statistics report to the log.
	 *
	 * @param time
	 * @param labels Labels or counters
	 */
	void reportStats(boost::posix_time::ptime time, bool labels = false);

private:
	const String instanceID;
	SpiderCastConfigImpl& configImpl;
	CoreInterface&  coreInterface;
	const String name_;
	uint32_t workPending; //=0 No, >0 Yes, bit may mark type of work

	TaskSchedule_SPtr taskSchedule_SPtr;
	MembershipManager_SPtr memMngr_SPtr;
	TopologyManager_SPtr topoMngr_SPtr;
	IncomingMsgQ_SPtr incomingMsgQ_SPtr;

	const int msgsChunkSize;

	EnumCounter<TaskSchedule::TaskSchedStats, int64_t> taskSchedStatsCounter_;
	uint64_t numTasks_sinceLastReport_;
	uint64_t numMemMsgs_sinceLastReport_;
	uint64_t numTopoMsgs_sinceLastReport_;
	boost::posix_time::ptime lastReportTime_;
	boost::posix_time::time_duration reportPeriod_;

	boost::recursive_mutex mutexStats_;

	boost::shared_ptr<SpiderCastRuntimeError> e_sptr;

	//=== methods ===

	/**
	 * @return time to next task
	 */
	boost::posix_time::time_duration processTaskSchedule();

	/**
	 * Process a batch of membership messages
	 * @return true if the Q was partially processed, i.e. batch-size < Q-size
	 */
	bool processIncomingMembershipMessages();

	/**
	 * Process a batch of topology messages
	 * @return true if the Q was partially processed, i.e. batch-size < Q-size
	 */
	bool processIncomingTopologyMessages();

	void internalClose();

};

}

#endif
