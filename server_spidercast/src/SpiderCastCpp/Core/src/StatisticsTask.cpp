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
 * StatisticsTask.cpp
 *
 *  Created on: Dec 7, 2010
 */

#include "StatisticsTask.h"

namespace spdr
{

ScTraceComponent* StatisticsTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Core,
		trace::ScTrConstants::Layer_ID_Core,
		"StatisticsTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

StatisticsTask::StatisticsTask(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),""),
		core_(core),
		first_(true)
{
	Trace_Entry(this,"StatisticsTask()");
}

StatisticsTask::~StatisticsTask()
{
	Trace_Entry(this,"~StatisticsTask()");
}

void StatisticsTask::run()
{
	if (first_)
	{
		core_.reportStats(true);
		first_ = false;
	}
	else
	{
		core_.reportStats();
	}
}

String StatisticsTask::toString() const
{
	String str("StatisticsTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
