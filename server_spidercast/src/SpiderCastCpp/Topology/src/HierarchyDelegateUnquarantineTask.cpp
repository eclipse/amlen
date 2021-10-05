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
 * HierarchyDelegateUnquarantineTask.cpp
 *
 *  Created on: Nov 4, 2010
 */

#include "HierarchyDelegateUnquarantineTask.h"

namespace spdr
{

ScTraceComponent* HierarchyDelegateUnquarantineTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy,
		"HierarchyDelegateUnquarantineTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchyDelegateUnquarantineTask::HierarchyDelegateUnquarantineTask(
		const String& instID,
		HierarchyDelegateTaskInterface& delegateTaskInterface,
		NodeIDImpl_SPtr peer) :
		ScTraceContext(tc_,instID,""),
        delegateTaskInterface_(delegateTaskInterface),
		peer_(peer)
{
	Trace_Entry(this, "HierarchyDelegateUnquarantineTask()");
}

HierarchyDelegateUnquarantineTask::~HierarchyDelegateUnquarantineTask()
{
	Trace_Entry(this, "~HierarchyDelegateUnquarantineTask()");
}

void HierarchyDelegateUnquarantineTask::run()
{
	delegateTaskInterface_.unquarantineSupervisorCandidate(peer_);
}

String HierarchyDelegateUnquarantineTask::toString() const
{
	String str("HierarchyDelegateUnquarantineTask ");
	str.append(AbstractTask::toString());
	return str;
}


}
