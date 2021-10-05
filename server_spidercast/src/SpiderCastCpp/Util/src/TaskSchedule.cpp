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
 * TaskSchedule.cpp
 *
 *  Created on: 03/03/2010
 */

//#include <iostream>

#include "TaskSchedule.h"

namespace spdr
{

const boost::posix_time::time_duration TaskSchedule::ZERO_DELAY = boost::posix_time::milliseconds(0);

const char* const TaskSchedule::taskSchedStatsName[] = {
		"Null",
		"NumTasks",
		"AvgTard",
		"MaxTard",
		"NumTardThreshX", //above threshold
		"Max"
};

int TaskSchedule::handleCounter = 0;

TaskSchedule::TaskSchedule(ThreadControl& consumerThread) :
	handle(++handleCounter), thread(consumerThread)
{
}

TaskSchedule::~TaskSchedule()
{
}

void TaskSchedule::scheduleDelay(AbstractTask_SPtr task,
		boost::posix_time::time_duration delay)
{
	static time_duration zeroDelay = boost::posix_time::microseconds(0);

	if (delay < zeroDelay)
		throw IllegalArgumentException("Negative execution delay.");

	schedule(task, boost::get_system_time() + delay);
}

void TaskSchedule::schedule(AbstractTask_SPtr task,
		ptime execTime)
{
	if (execTime.is_special())
		throw IllegalArgumentException("Illegal execution time.");

	if (!task)
		throw NullPointerException("Null pointer to AbstractTask");

	bool notify;

	//synchronized (heap)
	{
		boost::recursive_mutex::scoped_lock lock_heap(heapMutex);

		//synchronized (task.lock)
		{
			boost::recursive_mutex::scoped_lock lock_task(task->mutex);

			if (task->state == AbstractTask::Scheduled || task->state
					== AbstractTask::Canceled)
			{
				throw SpiderCastRuntimeError("Task scheduled or canceled");
			}

			task->executionTime = execTime;
			task->taskScheduleHandle = handle;
			task->state = AbstractTask::Scheduled;
			taskHeap.push(task);
		}

		AbstractTask_SPtr top_task = taskHeap.top();
		notify = (top_task.get() == task.get());
	}

	if (notify)
	{
		thread.wakeUp();
	}
}


void TaskSchedule::cancel()
{
	//synchronized (heap)
	{
		boost::recursive_mutex::scoped_lock lock_heap(heapMutex);

		while (!taskHeap.empty())
		{
			AbstractTask_SPtr task = taskHeap.top();
			taskHeap.pop();

			//synchronized (task.lock)
			{
				boost::recursive_mutex::scoped_lock lock_task(task->mutex);

				task->state = AbstractTask::Executed;
				task->taskScheduleHandle = 0;
			}
		}
	}

	thread.wakeUp();
}

AbstractTask_SPtr TaskSchedule::getMin()
{
	boost::recursive_mutex::scoped_lock lock_heap(heapMutex);

	return taskHeap.top();

}

uint32_t TaskSchedule::size()
{
	//synchronized (heap)
	{
		boost::recursive_mutex::scoped_lock lock_heap(heapMutex);

		return taskHeap.size();
	}
}

bool TaskSchedule::isPendingTask(ptime currentTime)
{
	bool pending = false;

	//synchronized (heap)
	{
		boost::recursive_mutex::scoped_lock lock_heap(heapMutex);

		if (taskHeap.size() == 0)
		{
			pending = false;
		}
		else
		{
			bool foundValidTask = false; //skip over canceled tasks

			while (!foundValidTask && !taskHeap.empty())
			{
				AbstractTask_SPtr task = taskHeap.top();
				//synchronized (task.lock)
				{
					boost::recursive_mutex::scoped_lock lock_task(task->mutex);

					if (task->state == AbstractTask::Canceled)
					{
						taskHeap.pop();
					}
					else
					{
						pending = (task->executionTime <= currentTime);
						foundValidTask = true;
					}
				}
			}
		}
	}

	return pending;
}

AbstractTask_SPtr TaskSchedule::removeMin()
{
	AbstractTask_SPtr task = removeMin(boost::get_system_time()
			+ boost::posix_time::hours(24 * 365));

	return task;
}

AbstractTask_SPtr TaskSchedule::removeMin(ptime currentTime)
{
	AbstractTask_SPtr task;

	//synchronized (heap)
	{
		boost::recursive_mutex::scoped_lock lock_heap(heapMutex);

		while (!taskHeap.empty())
		{
			task = taskHeap.top();

			//synchronized (task.lock)
			{
				boost::recursive_mutex::scoped_lock lock_task(task->mutex);

				if (task->state == AbstractTask::Canceled)
				{
					taskHeap.pop();
					task->state = AbstractTask::Executed;
					task->taskScheduleHandle = 0;
					task.reset(); //Never return a canceled task
				}
				else if (task->executionTime <= currentTime)
				{
					taskHeap.pop();
					task->state = AbstractTask::Executed;
					task->taskScheduleHandle = 0;
					break;
				}
				else
				{
					task = AbstractTask_SPtr ();
					break;
				}
			}
		}

	}
	return task;
}

time_duration TaskSchedule::timeToNextTask(ptime currentTime)
{
	time_duration time = boost::posix_time::hours(24 * 365);
	AbstractTask_SPtr task;

	//synchronized (heap)
	{
		boost::recursive_mutex::scoped_lock lock_heap(heapMutex);

		while (!taskHeap.empty())
		{
			task = taskHeap.top();

			{
				boost::recursive_mutex::scoped_lock lock_task(task->mutex);

				if (task->state == AbstractTask::Canceled)
				{
					//skip over canceled tasks
					taskHeap.pop();
				}
				else
				{
					time = task->executionTime - currentTime;
					break;
				}
			}
		}

	}

	return time;
}

}//namespace spdr
