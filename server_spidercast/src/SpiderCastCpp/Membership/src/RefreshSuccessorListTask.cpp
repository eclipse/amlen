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
 * RefreshSuccessorListTask.cpp
 *
 *  Created on: Aug 4, 2010
 */

#include "RefreshSuccessorListTask.h"

namespace spdr
{

ScTraceComponent* RefreshSuccessorListTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"RefreshSuccessorListTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

RefreshSuccessorListTask::RefreshSuccessorListTask(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),"")
{
	Trace_Entry(this,"RefreshSuccessorListTask()");
	memMngr_SPtr = core.getMembershipManager();
}

RefreshSuccessorListTask::~RefreshSuccessorListTask()
{
	Trace_Entry(this,"~RefreshSuccessorListTask()");
}

void RefreshSuccessorListTask::run()
{
	if (memMngr_SPtr)
	{
		memMngr_SPtr->refreshSuccessorListTask();
	}
	else
	{
		throw NullPointerException(
				"NullPointerException from RefreshSuccessorListTask::run()");
	}
}

String RefreshSuccessorListTask::toString() const
{
	String str("RefreshSuccessorListTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
