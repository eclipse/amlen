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
 * TopologyTerminationTask.h
 *
 *  Created on: Jun 17, 2010
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: TopologyTerminationTask.h,v $
 * Revision 1.3  2015/11/18 08:36:59 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:15 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:10 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2010/06/20 14:43:10 
 * Initial version
 *
 */

#ifndef TOPOLOGYTERMINATIONTASK_H_
#define TOPOLOGYTERMINATIONTASK_H_

#include "AbstractTask.h"
#include "CoreInterface.h"
#include "TopologyManager.h"

namespace spdr
{

class TopologyTerminationTask: public spdr::AbstractTask
{
public:
	TopologyManager_SPtr _topo_SPtr;

	TopologyTerminationTask(CoreInterface& core);
	virtual ~TopologyTerminationTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	/*
	 * @see spdr::AbstractTask
	 */
	String toString();
};

}

#endif /* TOPOLOGYTERMINATIONTASK_H_ */

