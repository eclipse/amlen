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
 * HierarchySupervisorSetActiveDelegatesTask.h
 *
 *  Created on: Feb 3, 2011
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisorSetActiveDelegatesTask.h,v $
 * Revision 1.2  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2011/02/14 13:52:55 
 * comments; no functional change
 *
 */

#ifndef HIERARCHYSUPERVISORSETACTIVEDELEGATESTASK_H_
#define HIERARCHYSUPERVISORSETACTIVEDELEGATESTASK_H_

#include "AbstractTask.h"
#include "HierarchySupervisorTaskInterface.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class HierarchySupervisorSetActiveDelegatesTask: public AbstractTask,
		public ScTraceContext
{
private:
	static ScTraceComponent* _tc;
	HierarchySupervisorTaskInterface& _supervisorTaskInterface;

public:
	HierarchySupervisorSetActiveDelegatesTask(const String& instID,
			HierarchySupervisorTaskInterface& supervisorTaskInterface);

	virtual ~HierarchySupervisorSetActiveDelegatesTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;
};

}

#endif /* HIERARCHYSUPERVISORSETACTIVEDELEGATESTASK_H_ */
