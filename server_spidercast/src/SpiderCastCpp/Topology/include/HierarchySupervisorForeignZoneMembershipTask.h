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
 * HierarchySupervisorForeignZoneMembershipTask.h
 *
 *  Created on: Feb 16, 2011
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisorForeignZoneMembershipTask.h,v $
 * Revision 1.3  2015/11/18 08:37:00 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:16 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:18 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2011/02/16 20:08:34 
 * foreign zone membership task - take 1
 *
 */

#ifndef HIERARCHYSUPERVISORFOREIGNZONEMEMBERSHIPTASK_H_
#define HIERARCHYSUPERVISORFOREIGNZONEMEMBERSHIPTASK_H_

#include "AbstractTask.h"
#include "HierarchySupervisorTaskInterface.h"
#include "BusName.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class HierarchySupervisorForeignZoneMembershipTask: public AbstractTask,
		public ScTraceContext
{

private:
	static ScTraceComponent* _tc;
	HierarchySupervisorTaskInterface& _supervisorTaskInterface;
	BusName_SPtr _zoneBusName;
	bool _includeAttributes;
	int64_t _reqId;

public:
	HierarchySupervisorForeignZoneMembershipTask(const String& instID,
			HierarchySupervisorTaskInterface& supervisorTaskInterface,
			int64_t reqId, BusName_SPtr zoneBusName, bool includeAttributes);

	virtual ~HierarchySupervisorForeignZoneMembershipTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;

};

}
#endif /* HIERARCHYSUPERVISORFOREIGNZONEMEMBERSHIPTASK_H_ */
