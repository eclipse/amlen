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
 * ChangeOfMetadataDeliveryTask.cpp
 *
 *  Created on: 15/06/2010
 */

#include "ChangeOfMetadataDeliveryTask.h"

namespace spdr
{
using namespace trace;

ScTraceComponent* ChangeOfMetadataDeliveryTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"ChangeOfMetadataDeliveryTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

ChangeOfMetadataDeliveryTask::ChangeOfMetadataDeliveryTask(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),"")
{
	Trace_Entry(this,"ChangeOfMetadataDeliveryTask()");
	memMngr_SPtr = core.getMembershipManager();
}

ChangeOfMetadataDeliveryTask::~ChangeOfMetadataDeliveryTask()
{
	Trace_Entry(this,"~ChangeOfMetadataDeliveryTask()");
}

void ChangeOfMetadataDeliveryTask::run()
{
	if (memMngr_SPtr)
	{
		memMngr_SPtr->changeOfMetadataDeliveryTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from ChangeOfMetadataDeliveryTask::run()");
	}
}

String ChangeOfMetadataDeliveryTask::toString() const
{
	String str("ChangeOfMetadataDeliveryTask ");
	str.append(AbstractTask::toString());
	return str;
}
}
