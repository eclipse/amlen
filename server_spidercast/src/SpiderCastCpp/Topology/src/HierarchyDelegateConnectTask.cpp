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
 * HierarchyDelegateConnectTask.cpp
 *
 *  Created on: Oct 21, 2010
 */

#include "HierarchyDelegateConnectTask.h"

namespace spdr
{
ScTraceComponent* HierarchyDelegateConnectTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy,
		"HierarchyDelegateConnectTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchyDelegateConnectTask::HierarchyDelegateConnectTask(
		const String& instID,
		HierarchyDelegateTaskInterface& delegateTaskInterface) :
		ScTraceContext(tc_,instID,""),
        delegateTaskInterface_(delegateTaskInterface)
{
	Trace_Entry(this, "HierarchyDelegateConnectTask()");
}

HierarchyDelegateConnectTask::~HierarchyDelegateConnectTask()
{
	Trace_Entry(this, "~HierarchyDelegateConnectTask()");
}

void HierarchyDelegateConnectTask::run()
{
	delegateTaskInterface_.connectTask();
}

String HierarchyDelegateConnectTask::toString() const
{
	String str("HierarchyDelegateConnectTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
