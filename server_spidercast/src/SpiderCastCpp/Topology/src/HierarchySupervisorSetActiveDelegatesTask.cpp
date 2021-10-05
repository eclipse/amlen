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
 * HierarchySupervisorSetActiveDelegatesTask.cpp
 *
 *  Created on: Feb 3, 2011
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisorSetActiveDelegatesTask.cpp,v $
 * Revision 1.2  2015/11/18 08:36:58 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:14 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:11 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2011/02/14 13:55:01 
 * comments; no functional change
 *
 */

#include "HierarchySupervisorSetActiveDelegatesTask.h"

namespace spdr
{

ScTraceComponent* HierarchySupervisorSetActiveDelegatesTask::_tc =
		ScTr::enroll(trace::ScTrConstants::ScTr_Component_Name,
				trace::ScTrConstants::ScTr_SubComponent_Hier,
				trace::ScTrConstants::Layer_ID_Hierarchy,
				"HierarchySupervisorSetActiveDelegatesTask",
				trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchySupervisorSetActiveDelegatesTask::HierarchySupervisorSetActiveDelegatesTask(
		const String& instID,
		HierarchySupervisorTaskInterface& supervisorTaskInterface) :
	ScTraceContext(_tc, instID, ""), _supervisorTaskInterface(
			supervisorTaskInterface)
{
	Trace_Entry(this, "HierarchySupervisorSetActiveDelegatesTask()");
}

HierarchySupervisorSetActiveDelegatesTask::~HierarchySupervisorSetActiveDelegatesTask()
{
	Trace_Entry(this, "~HierarchySupervisorSetActiveDelegatesTask()");
}

void HierarchySupervisorSetActiveDelegatesTask::run()
{
	_supervisorTaskInterface.setActiveDelegatesTask();
}

String HierarchySupervisorSetActiveDelegatesTask::toString() const
{
	String str("HierarchySupervisorSetActiveDelegatesTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
