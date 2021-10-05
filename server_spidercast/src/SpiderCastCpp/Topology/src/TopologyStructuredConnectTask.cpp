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
 * TopologyStructuredConnecttask.cpp
 *
 *  Created on: Feb 2, 2012
 *      Author:
 *
 * Version     : $Revision: 1.3 $
 *-----------------------------------------------------------------
 * $Log: TopologyStructuredConnectTask.cpp,v $
 * Revision 1.3  2015/11/18 08:36:58 
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
 * Revision 1.1  2012/02/14 12:45:19 
 * Add structured topology
 *
 */

#include "TopologyStructuredConnectTask.h"

namespace spdr
{

TopologyStructuredConnectTask::TopologyStructuredConnectTask(CoreInterface& core)
{
	topoMgr_SPtr = core.getTopologyManager();
}

TopologyStructuredConnectTask::~TopologyStructuredConnectTask()
{
	// TODO Auto-generated destructor stub
}

void TopologyStructuredConnectTask::run()
{
	if (topoMgr_SPtr)
	{
		topoMgr_SPtr->structuredTopologyConnectTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from TopologyStructuredConnectTask::run()");
	}
}

String TopologyStructuredConnectTask::toString()
{
	String str("TopologyStructuredConnectTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
