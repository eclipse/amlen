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
 * HierarchyDelegatePubSubBridgeTask.cpp
 *
 *  Created on: Jul 9, 2012
 */

#include "HierarchyDelegatePubSubBridgeTask.h"

namespace spdr
{


ScTraceComponent* HierarchyDelegatePubSubBridgeTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy,
		"HierarchyDelegatePubSubBridgeTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchyDelegatePubSubBridgeTask::HierarchyDelegatePubSubBridgeTask(
		const String& instID,
		HierarchyDelegateTaskInterface& delegateTaskInterface):
		ScTraceContext(tc_,instID,""),
		delegateTaskInterface_(delegateTaskInterface)
{
	Trace_Entry(this, "HierarchyDelegatePubSubBridgeTask()");

}

HierarchyDelegatePubSubBridgeTask::~HierarchyDelegatePubSubBridgeTask()
{
	Trace_Entry(this, "~HierarchyDelegatePubSubBridgeTask()");
}

void HierarchyDelegatePubSubBridgeTask::run()
{
	delegateTaskInterface_.pubsubBridgeTask();
}

String HierarchyDelegatePubSubBridgeTask::toString() const
{
	String str("HierarchyDelegatePubSubBridgeTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
