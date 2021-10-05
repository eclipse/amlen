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
 * HierarchySupervisorForeignZoneMembershipTOTask.cpp
 *
 *  Created on: Mar 2, 2011
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisorForeignZoneMembershipTOTask.cpp,v $
 * Revision 1.2  2015/11/18 08:36:58 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:13 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:12 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2011/03/03 06:56:31 
 * Add support for foreign zone membership requests timeout
 *
 */

#include "HierarchySupervisorForeignZoneMembershipTOTask.h"

namespace spdr
{
ScTraceComponent* HierarchySupervisorForeignZoneMembershipTOTask::_tc =
		ScTr::enroll(trace::ScTrConstants::ScTr_Component_Name,
				trace::ScTrConstants::ScTr_SubComponent_Hier,
				trace::ScTrConstants::Layer_ID_Hierarchy,
				"HierarchySupervisorForeignZoneMembershipTOTask",
				trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchySupervisorForeignZoneMembershipTOTask::HierarchySupervisorForeignZoneMembershipTOTask(
		const String& instID,
		HierarchySupervisorTaskInterface& supervisorTaskInterface,
		int64_t reqId, String zoneBusName) :
	ScTraceContext(_tc, instID, ""), _supervisorTaskInterface(
			supervisorTaskInterface), _zoneBusName(zoneBusName), _reqId(reqId)
{
	Trace_Entry(this, "HierarchySupervisorForeignZoneMembershipTOTask()");

}

HierarchySupervisorForeignZoneMembershipTOTask::~HierarchySupervisorForeignZoneMembershipTOTask()
{
	// TODO Auto-generated destructor stub
}

void HierarchySupervisorForeignZoneMembershipTOTask::run()
{
	_supervisorTaskInterface.foreignZoneMembershipTOTask(_reqId, _zoneBusName);
}

String HierarchySupervisorForeignZoneMembershipTOTask::toString() const
{
	String str("HierarchySupervisorForeignZoneMembershipTOTask ");
	str.append(AbstractTask::toString());
	return str;
}

}

