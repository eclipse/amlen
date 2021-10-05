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

//#include <iostream>

#include <error.h>
#include "sys/prctl.h"

#include "TaskExecutor.h"

namespace mcp
{
spdr::ScTraceComponent* TaskExecutor::tc_ = spdr::ScTr::enroll(
		mcp::trace::Component_Name,
		mcp::trace::SubComponent_Core,
		spdr::trace::ScTrConstants::Layer_ID_App,
		"TaskExecutor",
		spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

const boost::posix_time::time_duration TaskExecutor::ZERO_DELAY = boost::posix_time::milliseconds(0);
const boost::posix_time::time_duration TaskExecutor::WAIT_DELAY = boost::posix_time::milliseconds(5000);

const char* const TaskExecutor::taskSchedStatsName[] = {
		"Null",
		"NumTasks",
		"AvgTard",
		"MaxTard",
		"NumTardThreshX", //above threshold
		"Max"
};

int TaskExecutor::handleCounter = 0;

TaskExecutor::TaskExecutor(const std::string& inst_id, const std::string& name) :
		Thread(inst_id),
		spdr::ScTraceContext(tc_,inst_id,""),
		handle(++handleCounter),
		name_(inst_id + "." + name),
		workPending(false),
		fatalErrorHandler_(NULL)
{
	Trace_Entry(this, "TaskExecutor()", name_);
}

TaskExecutor::~TaskExecutor()
{
	Trace_Entry(this, "~TaskExecutor()", name_);
	finish();
	join();
}

void TaskExecutor::scheduleDelay(AbstractTask_SPtr task,
		boost::posix_time::time_duration delay)
{
	if (delay < ZERO_DELAY)
		throw spdr::IllegalArgumentException("Negative execution delay.");

	schedule(task, boost::get_system_time() + delay);
}

void TaskExecutor::schedule(AbstractTask_SPtr task,
		ptime execTime)
{
	if (execTime.is_special())
		throw spdr::IllegalArgumentException("Illegal execution time.");

	if (!task)
		throw spdr::NullPointerException("Null pointer to AbstractTask");

	bool notify;

	{
		boost::recursive_mutex::scoped_lock lock_heap(mutex);

		{
			boost::recursive_mutex::scoped_lock lock_task(task->mutex);

			if (task->state == AbstractTask::Scheduled || task->state
					== AbstractTask::Canceled)
			{
				throw spdr::SpiderCastRuntimeError("Task scheduled or canceled");
			}

			task->executionTime = execTime;
			task->taskScheduleHandle = handle;
			task->state = AbstractTask::Scheduled;
			taskHeap.push(task);
		}

		AbstractTask_SPtr top_task = taskHeap.top();
		notify = (top_task->equals(*task));
		if (notify)
		{
			workPending = notify;
		}
	}

	if (notify)
	{
		conditionVar.notify_all();
	}
}

void TaskExecutor::cancel()
{
	//synchronized (heap)
	{
		boost::recursive_mutex::scoped_lock lock_heap(mutex);

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

		workPending = true;
	}

	conditionVar.notify_all();
}

//    /**
//     * Remove canceled tasks from the queue.
//     *
//     */
//    public void purge()
//    {
//        synchronized (heap)
//        {
//            for (int i = heap.size(); i > 0; i--)
//            {
//                AbstractTask task = heap.array()[i];
//
//                synchronized (task.lock)
//                {
//                    if (task.state == AbstractTask.State.Canceled)
//                    {
//                        heap.remove(task);
//                        task.state = AbstractTask.State.Executed;
//                        task.schedule = null;
//                    }
//                }
//            }
//        }
//
//    }

AbstractTask_SPtr TaskExecutor::getMin()
{
	boost::recursive_mutex::scoped_lock lock_heap(mutex);
	return taskHeap.top();
}

uint32_t TaskExecutor::size()
{
	boost::recursive_mutex::scoped_lock lock_heap(mutex);
	return taskHeap.size();
}

bool TaskExecutor::isPendingTask(ptime currentTime)
{
	bool pending = false;

	{
		boost::recursive_mutex::scoped_lock lock_heap(mutex);

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

				{
					boost::recursive_mutex::scoped_lock lock_task(task->mutex);

					if (task->state == AbstractTask::Canceled)
					{
						taskHeap.pop();
						task->state = AbstractTask::Executed;
						task->taskScheduleHandle = 0;
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

AbstractTask_SPtr TaskExecutor::removeMin()
{
	AbstractTask_SPtr task = removeMin(boost::get_system_time()
			+ boost::posix_time::hours(24 * 365));

	return task;
}

AbstractTask_SPtr TaskExecutor::removeMin(ptime currentTime)
{
	AbstractTask_SPtr task;

	//synchronized (heap)
	{
		boost::recursive_mutex::scoped_lock lock_heap(mutex);

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

time_duration TaskExecutor::timeToNextTask(ptime currentTime)
{
	time_duration time = boost::posix_time::hours(24 * 365);
	AbstractTask_SPtr task;

	//synchronized (heap)
	{
		boost::recursive_mutex::scoped_lock lock_heap(mutex);

		while (!taskHeap.empty())
		{
			task = taskHeap.top();

			{
				boost::recursive_mutex::scoped_lock lock_task(task->mutex);

				if (task->state == AbstractTask::Canceled)
				{
					//skip over canceled tasks
					taskHeap.pop();
					task->state = AbstractTask::Executed;
					task->taskScheduleHandle = 0;
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

const std::string TaskExecutor::getThreadName() const
{
	return name_;
}

void TaskExecutor::setFatalErrorHandler(FatalErrorHandler* pFatalErrorHandler)
{
    if (pFatalErrorHandler == NULL)
    {
        std::string what = "TaskExecutor::setFatalErrorHandler Null handler";
        throw MCPRuntimeError(what, ISMRC_NullArgument);
    }

    fatalErrorHandler_ = pFatalErrorHandler;
}

void TaskExecutor::operator()()
{
    using namespace std;
    using namespace spdr;

    Trace_Entry(this,"operator()()");

    string name16;
    if (name_.size()>15)
        name16 = name_.substr(0,15);
    else
        name16 = name_;

    int rc = prctl( PR_SET_NAME,(uintptr_t)name16.c_str());
    if (rc != 0)
    {
        int errsv = errno;
        Trace_Warning(this, "operator()", "prctl PR_SET_NAME failed",
                "errno", spdr::stringValueOf(errsv));
    }
    else
    {
        Trace_Debug(this, "operator()", "thread name set to", "name", name16);
    }

//    {
//        char tn[16] ;
//        rmm_strlcpy(tn,__FUNCTION__,16);
//        prctl( PR_SET_NAME,(uintptr_t)tn);
//      }

    try
    {
        if (ScTraceBuffer::isEventEnabled(tc_))
        {
            ScTraceBufferAPtr tb = ScTraceBuffer::event(this,"operator()()", "Starting");
            tb->addProperty<boost::thread::id>("id", getThreadID());
            tb->invoke();
        }

        ism_engine_threadInit(0);

        boost::posix_time::time_duration time2next = ZERO_DELAY;

        while (!isFinish())
        {
            {
                boost::recursive_mutex::scoped_lock lock(mutex);
                if (!workPending && (time2next>ZERO_DELAY))
                {
                    try
                    {
                        boost::posix_time::time_duration actual_wait = WAIT_DELAY;
                        if (time2next < WAIT_DELAY)
                        {
                            actual_wait = time2next;
                        }

                        conditionVar.timed_wait(lock, actual_wait);
                    }
                    catch (boost::thread_interrupted& ex)
                    {
                        Trace_Debug(this, "operator()()", "Interrupted",
                                "id", ScTraceBuffer::stringValueOf<boost::thread::id>(getThreadID()));
                    }
                }

                workPending = false;
            }

            //and tasks again to get the accurate wait time
            time2next = processTaskSchedule();
        }

        Trace_Event(this,"operator()()", "Ending");
    }
    catch (MCPRuntimeError& mcp_re)
    {
        //Unexpected exception, call thread failure and shut down
        Trace_Error(this,"operator()()", "Error: unexpected MCPRuntimeError",
                "wnat",mcp_re.what(),"RC",
                boost::lexical_cast<std::string>(mcp_re.getReturnCode()),
                "stacktrace",mcp_re.getStackTrace());

        //Report fatal-error(thread failure) and orderly shutdown (internal close)
        if (fatalErrorHandler_)
        {
            fatalErrorHandler_->onFatalError("MCP TaskExecutor", "unexpected MCPRuntimeError", mcp_re.getReturnCode());
        }
        else
        {
            throw mcp_re;
        }
    }
    catch (MCPLogicError& mcp_le)
    {
        //Unexpected exception, call thread failure and shut down
        Trace_Error(this,"operator()()", "Error: unexpected MCPLogicError",
                "what",mcp_le.what(),
                "RC",boost::lexical_cast<std::string>(mcp_le.getReturnCode()),
                "stacktrace", mcp_le.getStackTrace());

        //Report fatal-error(thread failure) and orderly shutdown (internal close)
        if (fatalErrorHandler_)
        {
            fatalErrorHandler_->onFatalError("MCP TaskExecutor", "unexpected MCPLogicError", mcp_le.getReturnCode());
        }
        else
        {
            throw mcp_le;
        }
    }
    catch (spdr::SpiderCastRuntimeError& re)
    {
        //Unexpected exception, call thread failure and shut down
        Trace_Error(this,"operator()()", "Error: unexpected SpiderCastRuntimeError",
                "what",re.what(),"stacktrace",re.getStackTrace());

        //Report fatal-error(thread failure) and orderly shutdown (internal close)
        if (fatalErrorHandler_)
        {
            fatalErrorHandler_->onFatalError("MCP TaskExecutor", "unexpected SpiderCastRuntimeError", ISMRC_ClusterInternalError);
        }
        else
        {
            throw re;
        }

    }
    catch (spdr::SpiderCastLogicError& le)
    {
        //Unexpected exception, call thread failure and shut down
        Trace_Error(this,"operator()()", "Error: unexpected SpiderCastLogicError","what",le.what(),"stacktrace",le.getStackTrace());

        //Report fatal-error(thread failure) and orderly shutdown (internal close)
        if (fatalErrorHandler_)
        {
            fatalErrorHandler_->onFatalError("MCP TaskExecutor", "unexpected SpiderCastLogicError", ISMRC_ClusterInternalError);
        }
        else
        {
            throw le;
        }
    }
    catch (std::bad_alloc& ba)
    {
        //Unexpected exception, call thread failure and shut down
        Trace_Error(this,"operator()()", "Error: unexpected std::bad_alloc","what",ba.what());

        //Report fatal-error(thread failure) and orderly shutdown (internal close)
        if (fatalErrorHandler_)
        {
            fatalErrorHandler_->onFatalError("MCP TaskExecutor", "unexpected std::bad_alloc", ISMRC_AllocateError);
        }
        else
        {
            throw ba;
        }
    }
    catch (std::exception& e)
    {
        //Unexpected exception, call thread failure and shut down
        Trace_Error(this,"operator()()", "Error: unexpected exception","what",e.what());

        //Report fatal-error(thread failure) and orderly shutdown (internal close)
        if (fatalErrorHandler_)
        {
            fatalErrorHandler_->onFatalError("MCP TaskExecutor", "unexpected exception", ISMRC_ClusterInternalError);
        }
        else
        {
            throw e;
        }
    }
    catch (...)
    {
        //Unexpected exception, call thread failure and shut down
        Trace_Error(this,"operator()()", "Error: unexpected untyped (...) exception");

        //Report fatal-error(thread failure) and orderly shutdown (internal close)
        if (fatalErrorHandler_)
        {
            fatalErrorHandler_->onFatalError("MCP TaskExecutor", "unexpected untyped (..) exception", ISMRC_ClusterInternalError);
        }
        else
        {
            throw MCPRuntimeError("unexpected untyped (..) exception");
        }
    }

    Trace_Exit(this,"operator()()");
}

boost::posix_time::time_duration TaskExecutor::processTaskSchedule()
{
    using namespace spdr;

	Trace_Entry(this,"processTaskSchedule()");

	// ensures this loop does not go forever
	boost::posix_time::ptime now = boost::get_system_time();
	boost::posix_time::time_duration tardiness_threshold = boost::posix_time::milliseconds(500); //TODO configure
	int num_tardiness_threshold = 0;
	boost::posix_time::time_duration tardiness_max = boost::posix_time::seconds(0);

	int i = 0;
	while (isPendingTask(now))
	{
		AbstractTask_SPtr task = removeMin(now);

		if (task)
		{
			boost::posix_time::time_duration tardiness = now - task->scheduledExecutionTime();

			if (tardiness > tardiness_threshold)
			{
				num_tardiness_threshold++;
			}

			if (tardiness > tardiness_max)
			{
				tardiness_max = tardiness;
			}

			task->run();
			i++;
		}
	}

	if (i>0)
	{
		if (num_tardiness_threshold > 0)
		{
			//TODO throw event
			if (ScTraceBuffer::isEventEnabled(tc_))
			{
				ScTraceBufferAPtr tb = ScTraceBuffer::debug(this,"processTaskSchedule()", "Warning: Tardiness-Threshold-Violation");
				tb->addProperty("some tasks are over the tardiness threshold, possible CPU starvation");
				tb->addProperty<int>("#tasks",i);
				tb->addProperty("Tardiness-Max", boost::posix_time::to_iso_string(tardiness_max));
				tb->addProperty<int>("#violations",num_tardiness_threshold);
				tb->invoke();
			}
		}
	}

	now = boost::get_system_time();
	boost::posix_time::time_duration time2next = timeToNextTask(now);

	Trace_Exit(this, "processTaskSchedule()", boost::posix_time::to_simple_string(time2next));
	return time2next;
}

void TaskExecutor::wakeUp()
{
    using namespace spdr;

	Trace_Entry(this, "wakeUp", "Entry");

	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		workPending = true;
	}

	conditionVar.notify_all();

	Trace_Exit(this, "wakeUp", "Exit");
}

}//namespace spdr
