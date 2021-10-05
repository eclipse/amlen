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
 * HierarchySupervisorForeignZoneMembershipTask.cpp
 *
 *  Created on: Feb 16, 2011
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisorForeignZoneMembershipTask.cpp,v $
 * Revision 1.3  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:15 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:13 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:11 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2011/02/16 20:08:34 
 * foreign zone membership task - take 1
 *
 */

#include "HierarchySupervisorForeignZoneMembershipTask.h"

namespace spdr
{

ScTraceComponent* HierarchySupervisorForeignZoneMembershipTask::_tc =
		ScTr::enroll(trace::ScTrConstants::ScTr_Component_Name,
				trace::ScTrConstants::ScTr_SubComponent_Hier,
				trace::ScTrConstants::Layer_ID_Hierarchy,
				"HierarchySupervisorForeignZoneMembershipTask",
				trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchySupervisorForeignZoneMembershipTask::HierarchySupervisorForeignZoneMembershipTask(
		const String& instID,
		HierarchySupervisorTaskInterface& supervisorTaskInterface,
		int64_t reqId, BusName_SPtr zoneBusName, bool includeAttributes) :
	ScTraceContext(_tc, instID, ""), _supervisorTaskInterface(
			supervisorTaskInterface), _zoneBusName(zoneBusName),
			_includeAttributes(includeAttributes), _reqId(reqId)
{
	Trace_Entry(this, "HierarchySupervisorForeignZoneMembershipTask()");

}

HierarchySupervisorForeignZoneMembershipTask::~HierarchySupervisorForeignZoneMembershipTask()
{
	Trace_Entry(this, "~HierarchySupervisorForeignZoneMembershipTask()");
}

void HierarchySupervisorForeignZoneMembershipTask::run()
{
	_supervisorTaskInterface.foreignZoneMembershipTask(_reqId, _zoneBusName,
			_includeAttributes);
}

String HierarchySupervisorForeignZoneMembershipTask::toString() const
{
	String str("HierarchySupervisorForeignZoneMembershipTask ");
	str.append(AbstractTask::toString());
	return str;
}

}

