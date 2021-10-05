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
 * MembershipTerminationTask.cpp
 *
 *  Created on: 11/04/2010
 */

#include "MembershipTerminationTask.h"

namespace spdr
{

ScTraceComponent* MembershipTerminationTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"MembershipTerminationTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

MembershipTerminationTask::MembershipTerminationTask(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),"")
{
	Trace_Entry(this,"MembershipTerminationTask()");
	memMngr_SPtr = core.getMembershipManager();
}

MembershipTerminationTask::~MembershipTerminationTask()
{
	Trace_Entry(this,"~MembershipTerminationTask()");
}

void MembershipTerminationTask::run()
{
	if (memMngr_SPtr)
	{
		memMngr_SPtr->terminationTask();
	}
	else
	{
		throw NullPointerException(
				"NullPointerException from MembershipTerminationTask::run()");
	}
}

String MembershipTerminationTask::toString() const
{
	String str("MembershipTerminationTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
