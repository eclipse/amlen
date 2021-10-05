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
 * TopologyStructuredConnecttask.h
 *
 *  Created on: Feb 2, 2012
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: TopologyStructuredConnectTask.h,v $
 * Revision 1.3  2015/11/18 08:37:00 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:16 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:08 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2012/02/14 12:45:17 
 * Add structured topology
 *
 */

#ifndef TOPOLOGYSTRUCTUREDCONNECTTASK_H_
#define TOPOLOGYSTRUCTUREDCONNECTTASK_H_

#include "AbstractTask.h"
#include "CoreInterface.h"
#include "TopologyManager.h"

namespace spdr
{

class TopologyStructuredConnectTask: public spdr::AbstractTask
{
private:
	TopologyManager_SPtr topoMgr_SPtr;

public:
	TopologyStructuredConnectTask(CoreInterface& core);
	virtual ~TopologyStructuredConnectTask();

	/*
	 * @see spdr::AbstractTask
	 */
	void run();

	String toString();
};

}

#endif /* TOPOLOGYSTRUCTUREDCONNECTTASK_H_ */
