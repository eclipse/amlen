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
 * MembershipTermination2Task.cpp
 *
 *  Created on: 11/04/2010
 */

#include "MembershipTermination2Task.h"

namespace spdr
{

ScTraceComponent* MembershipTermination2Task::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"MembershipTermination2Task",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

MembershipTermination2Task::MembershipTermination2Task(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),"")
{
	Trace_Entry(this,"MembershipTermination2Task()");
	memMngr_SPtr = core.getMembershipManager();
}

MembershipTermination2Task::~MembershipTermination2Task()
{
	Trace_Entry(this,"~MembershipTermination2Task()");
}

void MembershipTermination2Task::run()
{
	if (memMngr_SPtr)
	{
		memMngr_SPtr->terminationGraceTask();
	}
	else
	{
		throw NullPointerException(
				"NullPointerException from MembershipTermination2Task::run()");
	}
}

String MembershipTermination2Task::toString() const
{
	String str("MembershipTermination2Task ");
	str.append(AbstractTask::toString());
	return str;
}

}
