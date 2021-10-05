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
 * LEWarmupTask.cpp
 *
 *  Created on: Jun 19, 2012
 */

#include "LEWarmupTask.h"

namespace spdr
{

namespace leader_election
{

ScTraceComponent* LEWarmupTask::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"LEWarmupTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

LEWarmupTask::LEWarmupTask(
		const String& instID, LEViewKeeper_SPtr viewKeeper, LECandidate_SPtr candidate) :
		ScTraceContext(tc_,instID,""),
		viewKeeper_(viewKeeper),
		candidate_(candidate)
{
	Trace_Entry(this, "LEWarmupTask()");
}

LEWarmupTask::~LEWarmupTask()
{
	Trace_Entry(this, "~LEWarmupTask()");
}

/*
 * @see spdr::AbstractTask
 */
void LEWarmupTask::run()
{
	Trace_Entry(this, "run()");
	candidate_->warmupExpired();
	viewKeeper_->firstViewDelivery();
	Trace_Exit(this, "run()");
}

String LEWarmupTask::toString() const
{
	String str("LEWarmupTask ");
	str.append(AbstractTask::toString());
	return str;
}

}

}
