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
 * TaskSchedule.h
 *
 *  Created on: 03/03/2010
 */

#ifndef TASKSCHEDULE_H_
#define TASKSCHEDULE_H_

#include <queue>

#include <boost/cstdint.hpp>
#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Definitions.h"
#include "ThreadControl.h"
#include "AbstractTask.h"
#include "SpiderCastLogicError.h"
#include "SpiderCastRuntimeError.h"
//#include "ConcurrentSharedPtr.h"

namespace spdr
{

using boost::posix_time::ptime;
using boost::posix_time::time_duration;

class TaskSchedule;
typedef boost::shared_ptr<TaskSchedule> TaskSchedule_SPtr;


/**
 *
 * Uses Boost.Thread.
 *
 * Time concepts use Boost.Date_Time:
 * boost::posix_time::time_duration for delay, and
 * boost::posix_time::ptime for absolute time.
 *
 */
class TaskSchedule
{

public:

	const static boost::posix_time::time_duration ZERO_DELAY;


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

	TaskSchedule(ThreadControl& consumerThread);

	virtual ~TaskSchedule();

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

// Reschedule not supported - not efficient with priority_queue.
//	/**
//	 * Reschedule a task.
//	 *
//	 * If the task was ON the queue - still not executed, or canceled - it is
//	 * rescheduled. If the task was OFF the queue - never submitted or already
//	 * executed - it is submitted again.
//	 *
//	 * @param task
//	 * @param delay
//	 *            execution delay
//	 * @return true if the operation results in a future task added, false when
//	 *         a queued (pending, not canceled) task is rescheduled.
//	 */
//	bool rescheduleDelay(AbstractTask task, long delay);
//
//	/**
//	 * Reschedule a task.
//	 *
//	 * If the task was ON the queue - still not executed, or canceled - it is
//	 * rescheduled. If the task was OFF the queue - never submitted or already
//	 * executed - it is submitted again.
//	 *
//	 * @param task
//	 * @param execTime
//	 *            scheduled execution time
//	 *
//	 * @return true if the operation results in a future task added, false when
//	 *         a queued (pending, not canceled) task is rescheduled.
//	 */
//	bool reschedule(AbstractTask task, long execTime);

	/**
	 * Cancel all tasks in the queue.
	 */
	void cancel();

//	/**
//	 * Remove canceled tasks from the queue.
//	 *
//	 */
//	void purge();

	/**
	 * Retrieve but does not remove the task at the top of the schedule.
	 *
	 * @return the task at the top, or null if schedule is empty.
	 */
	AbstractTask_SPtr getMin();

	/**
	 * The number of tasks, including canceled tasks.
	 *
	 * @return
	 */
	uint32_t size();

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

private:
	static int handleCounter; //provides an identifier for TaskSchedule instances

	const int handle;
	ThreadControl& thread;
	std::priority_queue< AbstractTask_SPtr, std::vector<AbstractTask_SPtr >, spdr::SPtr_Greater<AbstractTask> > taskHeap;
	boost::recursive_mutex heapMutex;

	/** Wake up the consumer thread */
	void consumerNotify();

};

}

#endif /* TASKSCHEDULE_H_ */
