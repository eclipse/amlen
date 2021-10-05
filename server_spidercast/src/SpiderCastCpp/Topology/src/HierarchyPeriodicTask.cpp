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
 * HierarchyPeriodicTask.cpp
 *
 *  Created on: Oct 5, 2010
 */

#include "HierarchyPeriodicTask.h"

namespace spdr
{
using namespace trace;

ScTraceComponent* HierarchyPeriodicTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy,
		"HierarchyPeriodicTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchyPeriodicTask::HierarchyPeriodicTask(CoreInterface& core) :
		ScTraceContext(tc_,core.getInstanceID(),"")
{
	Trace_Entry(this, "HierarchyPeriodicTask()");
	hierMngr_SPtr = core.getHierarchyManager();
}

HierarchyPeriodicTask::~HierarchyPeriodicTask()
{
	Trace_Entry(this, "~HierarchyPeriodicTask()");
}

void HierarchyPeriodicTask::run()
{
	if (hierMngr_SPtr)
	{
		hierMngr_SPtr->periodicTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from HierarchyPeriodicTask::run()");
	}
}

String HierarchyPeriodicTask::toString() const
{
	String str("HierarchyPeriodicTask ");
	str.append(AbstractTask::toString());
	return str;
}


}
