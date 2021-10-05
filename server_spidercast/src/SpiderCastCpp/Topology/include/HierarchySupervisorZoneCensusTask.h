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
 * HierarchySupervisorZoneCensusTask.h
 *
 *  Created on: Feb 14, 2011
 *      Author:
 *
 * Version     : $Revision: 1.2 $
 *-----------------------------------------------------------------
 * $Log: HierarchySupervisorZoneCensusTask.h,v $
 * Revision 1.2  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:09 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2011/02/14 13:53:21 
 * Initial version
 *
 */

#ifndef HIERARCHYSUPERVISORZONECENSUSTASK_H_
#define HIERARCHYSUPERVISORZONECENSUSTASK_H_

#include "AbstractTask.h"
#include "HierarchySupervisorTaskInterface.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class HierarchySupervisorZoneCensusTask: public AbstractTask,
		public ScTraceContext
{

private:
	static ScTraceComponent* _tc;
	HierarchySupervisorTaskInterface& _supervisorTaskInterface;

public:
	HierarchySupervisorZoneCensusTask(const String& instID,
			HierarchySupervisorTaskInterface& supervisorTaskInterface);

	virtual ~HierarchySupervisorZoneCensusTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString() const;

};

}
#endif /* HIERARCHYSUPERVISORZONECENSUSTASK_H_ */
