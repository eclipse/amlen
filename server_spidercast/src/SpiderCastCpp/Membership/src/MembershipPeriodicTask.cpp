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
 * MembershipPeriodicTask.cpp
 *
 *  Created on: 06/04/2010
 */

#include "MembershipPeriodicTask.h"

namespace spdr
{

ScTraceComponent* MembershipPeriodicTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"MembershipPeriodicTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

MembershipPeriodicTask::MembershipPeriodicTask(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),"")
{
	Trace_Entry(this, "MembershipPeriodicTask()");
	memMngr_SPtr = core.getMembershipManager();
}

MembershipPeriodicTask::~MembershipPeriodicTask()
{
	Trace_Entry(this, "~MembershipPeriodicTask()");
}

void MembershipPeriodicTask::run()
{
	if (memMngr_SPtr)
	{
		memMngr_SPtr->periodicTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from MembershipPeriodicTask::run()");
	}
}

String MembershipPeriodicTask::toString() const
{
	String str("MembershipPeriodicTask ");
	str.append(AbstractTask::toString());
	return str;
}

}//namespace spdr
