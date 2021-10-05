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
 * MemTopoThread.cpp
 *
 *  Created on: 01/03/2010
 */

#include <iostream>
#include <sys/prctl.h>

#include "MemTopoThread.h"

namespace spdr
{

ScTraceComponent* MemTopoThread::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Core,
		trace::ScTrConstants::Layer_ID_Core,
		"MemTopoThread",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

MemTopoThread::MemTopoThread(
		const String& instID,
		SpiderCastConfigImpl& config,
		CoreInterface& coreIfc) :
	Thread(instID),
	ThreadControl(),
	ScTraceContext(tc_,instID,config.getMyNodeID()->getNodeName()),
	instanceID(instID),
	configImpl(config),
	coreInterface(coreIfc),
	name_(instanceID + ".MemTopoThread"),
	workPending((uint32_t)0),
	taskSchedule_SPtr(),
	memMngr_SPtr(),
	topoMngr_SPtr(),
	incomingMsgQ_SPtr(),
	msgsChunkSize(20),
	taskSchedStatsCounter_(TaskSchedule::TSS_Max, TaskSchedule::taskSchedStatsName),
	numTasks_sinceLastReport_(0),
	numMemMsgs_sinceLastReport_(0),
	numTopoMsgs_sinceLastReport_(0),
	lastReportTime_(boost::get_system_time()),
	reportPeriod_(boost::posix_time::seconds(10))
{
	Trace_Entry(this,"MemTopoThread()");
}

MemTopoThread::~MemTopoThread()
{
	Trace_Entry(this,"~MemTopoThread()");
}

void MemTopoThread::operator()()
{
    ism_common_makeTLS(512,NULL);
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
		Trace_Warning(this, "operator()", "Warning: prctl PR_SET_NAME failed",
				"errno", spdr::stringValueOf(errsv));
	}
	else
	{
		Trace_Debug(this, "operator()", "thread name set to", "name", name16);
	}


	taskSchedule_SPtr = coreInterface.getTopoMemTaskSchedule();
	memMngr_SPtr = coreInterface.getMembershipManager();
	topoMngr_SPtr = coreInterface.getTopologyManager();
	incomingMsgQ_SPtr = coreInterface.getCommAdapter()->getIncomingMessageQ();

	try
	{
		if (ScTraceBuffer::isEventEnabled(tc_))
		{
			ScTraceBufferAPtr tb = ScTraceBuffer::event(this,"operator()()", "Starting");
			tb->addProperty<boost::thread::id>("id", boost::this_thread::get_id());
			tb->invoke();
		}

		boost::posix_time::time_duration time2next = zero_millis;
		bool partialQueueProcessing = false;

		while (!isFinish())
		{
			{
				boost::recursive_mutex::scoped_lock lock(rMutex);
				if ((workPending==(uint32_t)0) && (time2next>zero_millis) && !partialQueueProcessing)
				{
					try
					{
						boost::posix_time::time_duration actual_wait = wait_millis;
						if (time2next < wait_millis)
							actual_wait = time2next;

						conditionVar.timed_wait(lock, actual_wait);
					}
					catch (boost::thread_interrupted& ex)
					{
						Trace_Debug(this, "operator()()", "Interrupted",
								"id",
								ScTraceBuffer::stringValueOf<boost::thread::id>(boost::this_thread::get_id()));
					}
				}
				//currentWorkSpec = workPending;
				workPending = (uint32_t)0;
			}

			//execute tasks first. Do not change order, initialization code.
			processTaskSchedule();
			//process a batch of messages
			partialQueueProcessing = processIncomingMembershipMessages();
			//execute tasks
			processTaskSchedule();
			//process a batch of messages
			bool tmp = processIncomingTopologyMessages();
			partialQueueProcessing = tmp || partialQueueProcessing;
			//processTaskSchedule();

			//and tasks again to get the accurate wait time
			time2next = processTaskSchedule();

			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				boost::posix_time::ptime now = boost::get_system_time();
				boost::posix_time::time_duration statsPeriod = now - lastReportTime_;
				if ( statsPeriod > reportPeriod_)
				{
					ScTraceBufferAPtr tb = ScTraceBuffer::dump(this,"operator()()", "Stats:");
					tb->addProperty("period",boost::posix_time::to_iso_string(statsPeriod));
					tb->addProperty<uint64_t>("#tasks",numTasks_sinceLastReport_);
					tb->addProperty<uint64_t>("#mem-msgs",numMemMsgs_sinceLastReport_);
					tb->addProperty<uint64_t>("#topo-msgs",numTopoMsgs_sinceLastReport_);
					tb->invoke();

					lastReportTime_ = now;
					numTasks_sinceLastReport_ = 0;
					numMemMsgs_sinceLastReport_ = 0;
					numTopoMsgs_sinceLastReport_ = 0;
				}

			}
		}

		Trace_Event(this,"operator()()", "Ending");
	}
	catch (SpiderCastRuntimeError& re)
	{
		//Unexpected exception, call thread failure and shut down
		ScTraceBufferAPtr tb = ScTraceBuffer::event(
				this,"operator()()", "Error: unexpected SpiderCastRuntimeError");
		tb->addProperty("what",re.what());
		tb->addProperty("stacktrace",re.getStackTrace());
		tb->invoke();

		//Report fatal-error(thread failure) and orderly shutdown (internal close)
		coreInterface.threadFailure(name_,re);

	}
	catch (SpiderCastLogicError& le)
	{
		//Unexpected exception, call thread failure and shut down
		ScTraceBufferAPtr tb = ScTraceBuffer::event(
				this,"operator()()", "Error: unexpected SpiderCastLogicError");
		tb->addProperty("what",le.what());
		tb->addProperty("stacktrace",le.getStackTrace());
		tb->invoke();

		//Report fatal-error(thread failure) and orderly shutdown (internal close)
		coreInterface.threadFailure(name_,le);
	}
	catch (std::bad_alloc& ba)
	{
		//Unexpected bad_alloc, call thread failure and shut down
		SpiderCastRuntimeError re(ba.what());
		ScTraceBufferAPtr tb = ScTraceBuffer::error(
				this,"operator()()", "Error: unexpected bad_alloc");
		tb->addProperty("what", ba.what());
		tb->addProperty("stacktrace",re.getStackTrace());
		tb->invoke();

		//Report fatal-error(thread failure) and orderly shutdown (internal close)
		coreInterface.threadFailure(name_,ba);
	}
	catch (std::exception& e)
	{
		//Unexpected exception, call thread failure and shut down
		SpiderCastRuntimeError re(e.what());
		ScTraceBufferAPtr tb = ScTraceBuffer::event(
				this,"operator()()", "Error: unexpected exception");
		tb->addProperty("what", e.what());
		tb->addProperty("typeid", typeid(e).name());
		tb->addProperty("stacktrace",re.getStackTrace());
		tb->invoke();

		//Report fatal-error(thread failure) and orderly shutdown (internal close)
		coreInterface.threadFailure(name_,e);
	}
	catch (...)
	{
		//Unexpected exception, call thread failure and shut down
		std::ostringstream oss;
		oss << name_ << " unexpected exception: id=" << boost::this_thread::get_id()
		<< "; Not a descendant of std::exception";

		//Report fatal-error(thread failure) and orderly shutdown (internal close)
		e_sptr.reset(new SpiderCastRuntimeError(oss.str()));

		ScTraceBufferAPtr tb = ScTraceBuffer::event(
				this,"operator()()", "Error: unexpected unknown exception");
		tb->addProperty("what", oss.str());
		tb->invoke();

		coreInterface.threadFailure(name_, *e_sptr);
	}

	internalClose();

	Trace_Exit(this,"operator()()");
	ism_common_freeTLS();
}

void MemTopoThread::internalClose()
{
	Trace_Entry(this,"internalClose()");
	taskSchedule_SPtr.reset();
	memMngr_SPtr.reset();
	topoMngr_SPtr.reset();
	Trace_Exit(this,"internalClose()");
}

boost::posix_time::time_duration MemTopoThread::processTaskSchedule()
{
	//Trace_Entry(this,"processTaskSchedule()");

	// ensures this loop does not go forever
	boost::posix_time::ptime now = boost::get_system_time();
	boost::posix_time::time_duration tardiness_threshold = boost::posix_time::milliseconds(
			configImpl.getStatisticsTaskTardinessThresholdMillis());
	int num_tardiness_threshold = 0;
	boost::posix_time::time_duration tardiness_max = boost::posix_time::seconds(0);

	int i = 0;
	while (taskSchedule_SPtr->isPendingTask(now))
	{
		AbstractTask_SPtr task = taskSchedule_SPtr->removeMin(now);

		if (task)
		{
			boost::posix_time::time_duration tardiness = now - task->scheduledExecutionTime();

			{
				boost::recursive_mutex::scoped_lock lock(mutexStats_);

				taskSchedStatsCounter_.increment(TaskSchedule::TSS_NumTasks);
				taskSchedStatsCounter_.increment(TaskSchedule::TSS_AvgTardiness, tardiness.total_milliseconds());
				taskSchedStatsCounter_.set2max(TaskSchedule::TSS_MaxTardiness, tardiness.total_milliseconds());

				if (tardiness > tardiness_threshold)
				{
					taskSchedStatsCounter_.increment(TaskSchedule::TSS_NumTardinessEvents);
					num_tardiness_threshold++;
				}

				++numTasks_sinceLastReport_;
			}

			if (tardiness > tardiness_max)
			{
				tardiness_max = tardiness;
			}

			try
			{
				task->run();
			}
			catch (std::exception& e)
			{
				ScTraceBufferAPtr tb = ScTraceBuffer::error(this,"processTaskSchedule()", "Error: exception in task->run()");
				tb->addProperty("what", e.what());
				tb->addProperty("typeid", typeid(*task).name());
				tb->addProperty("task", task->toString());
				tb->invoke();
				throw;
			}

			i++;
		}
	}

	if (i>0)
	{
		if (num_tardiness_threshold > 0)
		{
			if (ScTraceBuffer::isEventEnabled(tc_))
			{
				ScTraceBufferAPtr tb = ScTraceBuffer::event(this,"processTaskSchedule()", "Warning: Tardiness-Threshold-Violation");
				tb->addProperty("some tasks are over the tardiness threshold, possible CPU starvation");
				tb->addProperty<int>("#tasks",i);
				tb->addProperty("Tardiness-Max", boost::posix_time::to_iso_string(tardiness_max));
				tb->addProperty<int>("#violations",num_tardiness_threshold);
				tb->invoke();
			}
		}
	}

	now = boost::get_system_time();
	boost::posix_time::time_duration time2next = taskSchedule_SPtr->timeToNextTask(now);

	//Trace_Exit(this, "processTaskSchedule()", boost::posix_time::to_simple_string(time2next));
	return time2next;
}

bool MemTopoThread::processIncomingMembershipMessages()
{
	//Trace_Entry(this, "processIncomingMembershipMessages()");

	bool partial = false;

	if (incomingMsgQ_SPtr)
	{
		size_t q_size = incomingMsgQ_SPtr->getQSize(IncomingMsgQ::MembershipQ);
		partial = (q_size > (size_t)msgsChunkSize);
		int num_msgs_to_process = partial ? msgsChunkSize : (int)q_size ;

		if (num_msgs_to_process > 0)
		{
			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				ScTraceBufferAPtr tb = ScTraceBuffer::dump(this,"processIncomingMembershipMessages()");
				tb->addProperty<size_t>("Q-size",q_size);
				tb->addProperty<int>("num_msgs_to_process", num_msgs_to_process);
				tb->invoke();
			}

			boost::recursive_mutex::scoped_lock lock(mutexStats_);
			numMemMsgs_sinceLastReport_ +=  num_msgs_to_process;
		}

		while (num_msgs_to_process > 0)
		{
			--num_msgs_to_process;
			SCMessage_SPtr msg_sptr = incomingMsgQ_SPtr->pollQ(IncomingMsgQ::MembershipQ);
			if (msg_sptr)
			{
				try
				{
					memMngr_SPtr->processIncomingMembershipMessage(msg_sptr);
				}
				catch (std::exception& e)
				{
					ScTraceBufferAPtr tb = ScTraceBuffer::error(this,"processIncomingMembershipMessages()", "Error: exception in memMngr_SPtr->processIncomingMembershipMessage()");
					tb->addProperty("what", e.what());
					tb->addProperty("typeid", typeid(e).name());
					tb->addProperty("msg", spdr::toString(msg_sptr));
					tb->invoke();
					throw;
				}
			}
		}


	}

	//Trace_Exit<bool>(this, "processIncomingMembershipMessages()",partial);
	return partial;
}

bool MemTopoThread::processIncomingTopologyMessages()
{
	//Trace_Entry(this, "processIncomingTopologyMessages()");

	bool partial = false;

	if (incomingMsgQ_SPtr)
	{
		size_t numTopoMsgs = incomingMsgQ_SPtr->getQSize(IncomingMsgQ::TopologyQ);
		partial = (numTopoMsgs > (size_t)msgsChunkSize);
		int num_msgs_to_process = partial  ? msgsChunkSize : (int)numTopoMsgs;

		if (num_msgs_to_process > 0)
		{
			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				ScTraceBufferAPtr tb = ScTraceBuffer::dump(this,"processIncomingTopologyMessages()");
				tb->addProperty<size_t>("Q-size",numTopoMsgs);
				tb->addProperty<int>("num_msgs_to_process", num_msgs_to_process);
				tb->invoke();
			}

			boost::recursive_mutex::scoped_lock lock(mutexStats_);
			numTopoMsgs_sinceLastReport_ +=  num_msgs_to_process;
		}

		for (int counter = 0; counter < (int) num_msgs_to_process; counter++)
		{
			SCMessage_SPtr msg = incomingMsgQ_SPtr->pollQ(IncomingMsgQ::TopologyQ);
			if (msg)
			{
				try
				{
					topoMngr_SPtr->processIncomingTopologyMessage(msg);
				}
				catch (std::exception& e)
				{
					ScTraceBufferAPtr tb = ScTraceBuffer::error(this,"processIncomingMembershipMessages()", "Error: exception in topoMngr_SPtr->processIncomingTopologyMessage()");
					tb->addProperty("what", e.what());
					tb->addProperty("typeid", typeid(e).name());
					tb->addProperty("msg", spdr::toString(msg));
					tb->invoke();
					throw;
				}
			}

		}
	}

	//Trace_Exit<bool>(this, "processIncomingTopologyMessages()",partial);
	return partial;
}

void MemTopoThread::wakeUp(uint32_t mask)
{
	if (mask == (uint32_t)0)
		throw IllegalArgumentException("Mask must be >0");

	{
		boost::recursive_mutex::scoped_lock lock(rMutex);
		workPending = workPending | mask; //bitwise
	}

	conditionVar.notify_all();

	Trace_Dump(this, "wakeUp", "notify_all");
}

const String MemTopoThread::getName() const
{
	return name_;
}

void MemTopoThread::reportStats(boost::posix_time::ptime time, bool labels)
{

	if (ScTraceBuffer::isConfigEnabled(tc_))
	{

		EnumCounter<TaskSchedule::TaskSchedStats, int64_t> taskSchedStatsCounter;

		{
			boost::recursive_mutex::scoped_lock lock(mutexStats_);

			taskSchedStatsCounter = taskSchedStatsCounter_;
			taskSchedStatsCounter_.reset();
		}

		int64_t numTasks = taskSchedStatsCounter.get(TaskSchedule::TSS_NumTasks);
		if (numTasks > 0) //if there were no tasks, TaskSchedule::TSS_AvgTardiness is zero
		{
			int64_t avgTardiness = taskSchedStatsCounter.get(TaskSchedule::TSS_AvgTardiness) / numTasks;
			taskSchedStatsCounter.set(TaskSchedule::TSS_AvgTardiness, avgTardiness);
		}

		std::string time_str(boost::posix_time::to_iso_extended_string(time));

		std::ostringstream oss;
		oss << std::endl;
		if (!labels)
		{
			oss << instanceID << ", " << time_str << ", SC_Stats_Core_MemTopoTasks, "
					<< taskSchedStatsCounter.toCounterString() << std::endl;
		}
		else
		{
			oss << instanceID << ", " << time_str << ", SC_Stats_Core_MemTopoTasks, "
					<< taskSchedStatsCounter.toLabelString() << std::endl;
		}

		ScTraceBufferAPtr buffer = ScTraceBuffer::config(this, "reportStats()", oss.str());
		buffer->invoke();
	}
}


}

