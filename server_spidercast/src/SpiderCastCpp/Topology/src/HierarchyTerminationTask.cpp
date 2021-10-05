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
 * HierarchyTerminationTask.cpp
 *
 *  Created on: Oct 21, 2010
 */

#include "HierarchyTerminationTask.h"

namespace spdr
{

using namespace trace;

ScTraceComponent* HierarchyTerminationTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy,
		"HierarchyTerminationTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchyTerminationTask::HierarchyTerminationTask(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),"")
{
	Trace_Entry(this, "HierarchyTerminationTask()");
	hierMngr_SPtr = core.getHierarchyManager();
}

HierarchyTerminationTask::~HierarchyTerminationTask()
{
	Trace_Entry(this, "~HierarchyTerminationTask()");
}

void HierarchyTerminationTask::run()
{
	if (hierMngr_SPtr)
	{
		hierMngr_SPtr->terminationTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from HierarchyTerminationTask::run()");
	}
}

String HierarchyTerminationTask::toString() const
{
	String str("HierarchyTerminationTask ");
	str.append(AbstractTask::toString());
	return str;
}


}
