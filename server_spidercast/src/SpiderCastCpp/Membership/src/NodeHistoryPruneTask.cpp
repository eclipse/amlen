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
 * NodeHistoryPruneTask.cpp
 *
 *  Created on: 21/04/2010
 */

#include "NodeHistoryPruneTask.h"

namespace spdr
{

ScTraceComponent* NodeHistoryPruneTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"NodeHistoryPruneTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

NodeHistoryPruneTask::NodeHistoryPruneTask(
		const String& instID,
		NodeHistorySet& historySet,
		TaskSchedule_SPtr sched,
		int32_t retentionTime,
		AbstractTask_SPtr& self_ref) :

		ScTraceContext(tc_,instID,""),
		historySet_(historySet),
		sched_(sched),
		retentionTime_(retentionTime),
		self_ref_(self_ref)
{
	Trace_Entry(this,"NodeHistoryPruneTask()");
}

NodeHistoryPruneTask::~NodeHistoryPruneTask()
{
	Trace_Entry(this,"~NodeHistoryPruneTask()");
}

void NodeHistoryPruneTask::run()
{

	using namespace boost::posix_time;

	ptime now = microsec_clock::universal_time();
	ptime threshold = now - seconds(retentionTime_);
	int num = historySet_.prune(threshold);

	Trace_Event(this, "run()", "executed",
			"#pruned", ScTraceBuffer::stringValueOf<int>(num));;

	//next task
	if (sched_)
	{
		sched_->scheduleDelay(self_ref_, milliseconds((1000 * retentionTime_) / 2));
	}
	else
	{
		throw NullPointerException(
				"NullPointerException from NodeHistoryPruneTask::run()");
	}
}

String NodeHistoryPruneTask::toString() const
{
	String str("NodeHistoryPruneTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
