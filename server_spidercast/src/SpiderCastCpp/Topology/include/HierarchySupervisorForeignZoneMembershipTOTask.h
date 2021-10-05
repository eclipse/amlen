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
 * HierarchySupervisorForeignZoneMembershipTOTask.h
 *
 *  Created on: Mar 2, 2011
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisorForeignZoneMembershipTOTask.h,v $
 * Revision 1.2  2015/11/18 08:37:00 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2011/03/03 06:56:32 
 * Add support for foreign zone membership requests timeout
 *
 */

#ifndef HIERARCHYSUPERVISORFOREIGNZONEMEMBERSHIPTOTASK_H_
#define HIERARCHYSUPERVISORFOREIGNZONEMEMBERSHIPTOTASK_H_

#include "AbstractTask.h"
#include "HierarchySupervisorTaskInterface.h"
#include "BusName.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class HierarchySupervisorForeignZoneMembershipTOTask: public spdr::AbstractTask,
		public ScTraceContext
{
private:
	static ScTraceComponent* _tc;
	HierarchySupervisorTaskInterface& _supervisorTaskInterface;
	String _zoneBusName;
	int64_t _reqId;

public:
	HierarchySupervisorForeignZoneMembershipTOTask(const String& instID,
			HierarchySupervisorTaskInterface& supervisorTaskInterface,
			int64_t reqId, String zoneBusName);

	virtual ~HierarchySupervisorForeignZoneMembershipTOTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;

};

}

#endif /* HIERARCHYSUPERVISORFOREIGNZONEMEMBERSHIPTOTASK_H_ */

