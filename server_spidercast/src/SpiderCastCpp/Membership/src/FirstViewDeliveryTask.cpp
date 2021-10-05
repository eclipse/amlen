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
 * FirstViewDeliveryTask.cpp
 *
 *  Created on: 09/05/2010
 */

#include "FirstViewDeliveryTask.h"

namespace spdr
{

ScTraceComponent* FirstViewDeliveryTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"FirstViewDeliveryTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

FirstViewDeliveryTask::FirstViewDeliveryTask(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),"")
{
	Trace_Entry(this,"FirstViewDeliveryTask()");
	memMngr_SPtr = core.getMembershipManager();
}

FirstViewDeliveryTask::~FirstViewDeliveryTask()
{
	Trace_Entry(this,"~FirstViewDeliveryTask()");
}

void FirstViewDeliveryTask::run()
{
	if (memMngr_SPtr)
	{
		memMngr_SPtr->firstViewDeliveryTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from FirstViewDeliveryTask::run()");
	}
}

String FirstViewDeliveryTask::toString() const
{
	String str("FirstViewDeliveryTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
