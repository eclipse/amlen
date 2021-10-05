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
 * NodeHistoryPruneTask2.h
 *
 *  Created on: 21/04/2010
 */

#ifndef NODEHISTORYPRUNETASK_H_
#define NODEHISTORYPRUNETASK_H_

#include <boost/date_time/posix_time/posix_time.hpp>

#include "AbstractTask.h"
//#include "ConcurrentSharedPtr.h"
#include "NodeHistorySet.h"
#include "TaskSchedule.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class NodeHistoryPruneTask: public AbstractTask, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

	NodeHistorySet& historySet_;
	TaskSchedule_SPtr sched_;
	int32_t retentionTime_;
	AbstractTask_SPtr& self_ref_;

public:
	NodeHistoryPruneTask(
			const String& instID,
			NodeHistorySet& historySet,
			TaskSchedule_SPtr sched,
			int32_t retentionTime,
			AbstractTask_SPtr& self_ref);

	virtual ~NodeHistoryPruneTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;
};

}

#endif /* NODEHISTORYPRUNETASK_H_ */
