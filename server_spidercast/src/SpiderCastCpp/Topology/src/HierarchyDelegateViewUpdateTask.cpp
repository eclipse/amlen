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
 * HierarchyDelegateViewUpdateTask.cpp
 *
 *  Created on: Feb 3, 2011
 */

#include "HierarchyDelegateViewUpdateTask.h"

namespace spdr
{
ScTraceComponent* HierarchyDelegateViewUpdateTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy,
		"HierarchyDelegateViewUpdateTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchyDelegateViewUpdateTask::HierarchyDelegateViewUpdateTask(const String& instID,
		HierarchyDelegateTaskInterface& delegateTaskInterface):
		ScTraceContext(tc_,instID,""),
        delegateTaskInterface_(delegateTaskInterface)
{
	Trace_Entry(this, "HierarchyDelegateViewUpdateTask()");
}

HierarchyDelegateViewUpdateTask::~HierarchyDelegateViewUpdateTask()
{
	Trace_Entry(this, "HierarchyDelegateViewUpdateTask()");
}

void HierarchyDelegateViewUpdateTask::run()
{
	delegateTaskInterface_.supervisorViewUpdate();
}

String HierarchyDelegateViewUpdateTask::toString() const
{
	String str("HierarchyDelegateViewUpdateTask ");
	str.append(AbstractTask::toString());
	return str;
}
}
