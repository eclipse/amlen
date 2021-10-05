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
 * TopologyTerminationTask.cpp
 *
 *  Created on: Jun 17, 2010
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: TopologyTerminationTask.cpp,v $
 * Revision 1.3  2015/11/18 08:36:58 
 * Update copyright headers
 *
 * Revision 1.2  2015/08/06 06:59:15 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.1  2015/03/22 12:29:13 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:12 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.1  2010/06/20 14:47:01 
 * Initial version
 *
 */

#include "TopologyTerminationTask.h"

using namespace std;

namespace spdr
{

TopologyTerminationTask::TopologyTerminationTask(CoreInterface& core)
{
	_topo_SPtr = core.getTopologyManager();
}

TopologyTerminationTask::~TopologyTerminationTask()
{
}

void TopologyTerminationTask::run()
{
	if (_topo_SPtr)
	{
		_topo_SPtr->terminationTask();
	}
	else
	{
		throw NullPointerException(
				"NullPointerException from TopologyTerminationTask::run()");
	}
}

String TopologyTerminationTask::toString()
{
	String str("TopologyTerminationTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
