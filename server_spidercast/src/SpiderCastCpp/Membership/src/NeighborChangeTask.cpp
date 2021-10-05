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
 * NeighborChangeTask.cpp
 *
 *  Created on: 18/05/2010
 */

#include <iostream>

#include "NeighborChangeTask.h"

namespace spdr
{

ScTraceComponent* NeighborChangeTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"NeighborChangeTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

NeighborChangeTask::NeighborChangeTask(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),"")
{
	Trace_Entry(this,"NeighborChangeTask()");
	memMngr_SPtr = core.getMembershipManager();

}

NeighborChangeTask::~NeighborChangeTask()
{
	Trace_Entry(this,"~NeighborChangeTask()");
}

void NeighborChangeTask::run()
{
	if (memMngr_SPtr)
	{
		memMngr_SPtr->neighborChangeTask();
	}
	else
	{
		throw NullPointerException(
				"NullPointerException from NeighborChangeTask::run()");
	}
}

String NeighborChangeTask::toString()
{
	String str("NeighborChangeTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
