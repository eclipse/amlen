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
 * HierarchySupervisorZoneCensusTask.cpp
 *
 *  Created on: Feb 14, 2011
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisorZoneCensusTask.cpp,v $
 * Revision 1.2  2015/11/18 08:36:58 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:13 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:11 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2011/02/14 13:55:10 
 * Initial version
 *
 */

#include "HierarchySupervisorZoneCensusTask.h"

namespace spdr
{

ScTraceComponent* HierarchySupervisorZoneCensusTask::_tc = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Hier,
		trace::ScTrConstants::Layer_ID_Hierarchy,
		"HierarchySupervisorZoneCensusTask",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

HierarchySupervisorZoneCensusTask::HierarchySupervisorZoneCensusTask(
		const String& instID,
		HierarchySupervisorTaskInterface& supervisorTaskInterface) :
	ScTraceContext(_tc, instID, ""), _supervisorTaskInterface(
			supervisorTaskInterface)
{
	Trace_Entry(this, "HierarchySupervisorZoneCensusTask()");
}

HierarchySupervisorZoneCensusTask::~HierarchySupervisorZoneCensusTask()
{
	Trace_Entry(this, "~HierarchySupervisorZoneCensusTask()");
}

void HierarchySupervisorZoneCensusTask::run()
{
	_supervisorTaskInterface.zoneCensusTask();
}

String HierarchySupervisorZoneCensusTask::toString() const
{
	String str("HierarchySupervisorZoneCensusTask ");
	str.append(AbstractTask::toString());
	return str;
}

}

