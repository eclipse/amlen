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

#ifndef MCP_ABSTRACTTASK_H_
#define MCP_ABSTRACTTASK_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace mcp
{

/**
 * The base class of tasks that can be executed by a TaskExecutor.
 *
 * @see TaskExecutor
 */
class AbstractTask
{
	friend class TaskExecutor;
	//friend class TaskExecutor2;

public:

	AbstractTask() :
		state(Virgin), taskScheduleHandle(0)
	{
	}

	virtual ~AbstractTask()
	{
	}

	/**
	 * The action to be performed by this task.
	 */
	virtual void run() = 0;

	/**
	 * Cancel this task.
	 *
	 * The task remains on the queue, but silently drops when it was supposed to be executed.
	 *
	 * @return true if cancellation prevented future execution.
	 */
	bool cancel()
	{
		bool prevent_exec;

		{
			boost::recursive_mutex::scoped_lock lock(mutex);
			prevent_exec = (state == Scheduled);
			if (prevent_exec)
				state = Canceled;
		}

		return prevent_exec;
	}

	/**
	 * Scheduled execution time.
	 *
	 * @return
	 */
	boost::posix_time::ptime scheduledExecutionTime()
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		return executionTime;
	}

	/**
	 * Convert to string.
	 *
	 * @return
	 */
	virtual std::string toString() const
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		std::string s = stateName[state] + " " + boost::posix_time::to_simple_string(
				executionTime) + " h=" + boost::lexical_cast<std::string>(taskScheduleHandle);
		return s;
	}

	bool operator<(const mcp::AbstractTask& task) const
	{
		return executionTime < task.executionTime;
	}

	bool operator>(const mcp::AbstractTask& task) const
	{
		return executionTime > task.executionTime;
	}

	/**
	 * Equal objects, by address.
	 *
	 * @param task
	 * @return
	 */
	bool equals(const AbstractTask& task)
	{
		return (this == &task);
	}

protected:

	mutable boost::recursive_mutex mutex;

	enum State
	{
		/** Created, not scheduled (not in the Q) */
		Virgin = 1,
		/** Scheduled, not yet executed (in the Q) */
		Scheduled,
		/** executed or currently executing, not canceled (not in the Q) */
		Executed,
		/** canceled, but in the heap (in the Q) */
		Canceled
	};

	/** Initialized to Virgin */
	State state;

	/** Scheduled execution time (point in time, microsecond resolution, 64 bit) */
	boost::posix_time::ptime executionTime;

	/**
	 * An identifier of the associated schedule (Q) of the task manager.
	 * When this in not zero, the task is in Q, i.e. either Scheduled or Canceled.
	 */
	int taskScheduleHandle;

private:
	static const std::string stateName[];


};

typedef boost::shared_ptr<AbstractTask> AbstractTask_SPtr;

}//namespace mcp

#endif /* MCP_ABSTRACTTASK_H_ */
