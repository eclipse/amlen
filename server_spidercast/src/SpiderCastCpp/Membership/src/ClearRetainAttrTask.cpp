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
 * ClearRetainAttrTask.cpp
 *
 *  Created on: 11/04/2010
 */

#include "ClearRetainAttrTask.h"

namespace spdr
{

ScTraceComponent* ClearRetainAttrTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"ClearRetainAttrTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

ClearRetainAttrTask::ClearRetainAttrTask(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),"")
{
	Trace_Entry(this,"ClearRetainAttrTask()");
	memMngr_SPtr = core.getMembershipManager();
}

ClearRetainAttrTask::~ClearRetainAttrTask()
{
	Trace_Entry(this,"~ClearRetainAttrTask()");
}

void ClearRetainAttrTask::run()
{
	if (memMngr_SPtr)
	{
		memMngr_SPtr->clearRetainAttrTask();
	}
	else
	{
		throw NullPointerException(
				"NullPointerException from ClearRetainAttrTask::run()");
	}
}

String ClearRetainAttrTask::toString() const
{
	String str("ClearRetainAttrTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
