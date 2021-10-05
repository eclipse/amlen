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

#ifndef MCP_TASKEXECUTOR_H_
#define MCP_TASKEXECUTOR_H_

#include <queue>

#include <boost/cstdint.hpp>
#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "ismutil.h"
#include "Definitions.h"

#include "MCPConfig.h"
#include "AbstractTask.h"
#include "Thread.h"
#include "SpiderCastLogicError.h"
#include "SpiderCastRuntimeError.h"
#include "FatalErrorHandler.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*
 * A forward declaration of the engine thread init
 */
XAPI void ism_engine_threadInit(uint8_t isStoreCrit);


#ifdef __cplusplus
}
#endif

namespace mcp
{

	using boost::posix_time::ptime;
	using boost::posix_time::time_duration;


/**
 * TODO get rid of the Thread inheritance and embed all functionality here.
 *
 * Uses Boost.Thread.
 *
 * Time concepts use Boost.Date_Time:
 * boost::posix_time::time_duration for delay, and
 * boost::posix_time::ptime for absolute time.
 *
 * TODO move implementation to boost.Chrono
 *
 */
class TaskExecutor: public Thread, public spdr::ScTraceContext
{
private:
	static spdr::ScTraceComponent* tc_;

public:

	const static boost::posix_time::time_duration ZERO_DELAY;
	const static boost::posix_time::time_duration WAIT_DELAY;

	enum TaskSchedStats
	{
		TSS_Null = 0,
		TSS_NumTasks,
		TSS_AvgTardiness,
		TSS_MaxTardiness,
		TSS_NumTardinessEvents, //above threshold
		TSS_Max
	};

	static const char* const taskSchedStatsName[];

	TaskExecutor(const std::string& inst_id, const std::string& name);

	virtual ~TaskExecutor();


    void setFatalErrorHandler(FatalErrorHandler* pFatalErrorHandler);

	/**
	 * Schedule a task to be executed after a delay relative to current time.
	 *
	 * Sets task state to scheduled.
	 *
	 * @param task
	 * @param delay
	 *
	 * @throw IllegalArgumentException if negative delay
	 */
	void scheduleDelay(AbstractTask_SPtr task, time_duration delay);

	/**
	 * Schedule a task to be executed at a the specified time.
	 *
	 * Sets task state to scheduled.
	 *
	 * @param task
	 * @param delay
	 */
	void schedule(AbstractTask_SPtr task, ptime execTime);

	/**
	 * Cancel all tasks in the queue.
	 */
	void cancel();

	/**
	 * The number of tasks, including canceled tasks.
	 *
	 * @return
	 */
	uint32_t size();

	/**
	 * @see Thread
	 *
	 * Execute the tasks.
	 */
	void operator()();


	/*
	 * @see Thread
	 */
	const std::string getThreadName() const;


	// Reschedule not supported - not efficient with priority_queue.

	//	/**
	//	 * Remove canceled tasks from the queue.
	//	 *
	//	 */
	//	void purge();


private:
	static int handleCounter; //provides an identifier for TaskSchedule instances
	const int handle;
	const std::string name_;

	std::priority_queue< AbstractTask_SPtr, std::vector<AbstractTask_SPtr >, spdr::SPtr_Greater<AbstractTask> > taskHeap;
	//boost::recursive_mutex heapMutex;

	bool workPending;

	FatalErrorHandler* fatalErrorHandler_;

	void wakeUp();

	/**
	 * Retrieve but does not remove the task at the top of the schedule.
	 *
	 * @return the task at the top, or null if schedule is empty.
	 */
	AbstractTask_SPtr getMin();


	/**
	 * Is there a task with execution time smaller or equal to specified current
	 * time?
	 *
	 * @return
	 */
	bool isPendingTask(ptime currentTime);

	/**
	 * Unconditionally retrieve and remove the task at the top of the schedule.
	 *
	 * Sets task state to executed. Never returns tasks that were canceled.
	 *
	 * @return the task at the top, or null if schedule is empty.
	 */
	AbstractTask_SPtr removeMin();

	/**
	 * Retrieve and remove the task at the top of the schedule, only if its
	 * execution time is smaller or equal from the specified current time.
	 *
	 * Sets task state to executed. Never returns tasks that were canceled.
	 *
	 * @param currentTime
	 * @return a pending task, or null
	 */
	AbstractTask_SPtr removeMin(ptime currentTime);

	/**
	 * The time remaining until the scheduled execution time of the next task on
	 * the queue.
	 *
	 * @return (t.scheduledExecutionTime() - currentTime) where t is the
	 *         task at the top of the queue, or (365*24) hours if the queue is
	 *         empty. May return negative values.
	 */
	time_duration timeToNextTask(ptime currentTime);

	/**
	 * @return time to next task
	 */
	boost::posix_time::time_duration processTaskSchedule();


	/** Wake up the consumer thread */
	void consumerNotify();

};

typedef boost::shared_ptr<TaskExecutor> TaskExecutor_SPtr;

}

#endif /* MCP_TASKEXECUTOR_H_ */
