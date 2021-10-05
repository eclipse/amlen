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
 * TopologyRandomDisconnectTask.cpp
 *
 *  Created on: Aug 26, 2010
 *      Author:
 *
 * Version     : $Revision: 1.4 $
 *-----------------------------------------------------------------
 * $Log: TopologyRandomDisconnectTask.cpp,v $
 * Revision 1.4  2015/12/22 11:56:40 
 * print typeid in catch std::exception; fix toString of sons of AbstractTask.
 *
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
 * Revision 1.1  2010/08/26 18:59:35 
 * Make "random disconnect" and "update degree" seperate and independent tasks. Eliminat etopology periodic task
 *
 */

#include "TopologyRandomDisconnectTask.h"

namespace spdr
{

TopologyRandomDisconnectTask::TopologyRandomDisconnectTask(CoreInterface& core)
{
	topoMgr_SPtr = core.getTopologyManager();
}

TopologyRandomDisconnectTask::~TopologyRandomDisconnectTask()
{
	// TODO Auto-generated destructor stub
}

void TopologyRandomDisconnectTask::run()
{
	if (topoMgr_SPtr)
	{
		topoMgr_SPtr->randomTopologyDisconnectTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from TopologyRandomConnectTask::run()");
	}
}

String TopologyRandomDisconnectTask::toString() const
{
	String str("TopologyRandomDisconnectTask ");
	str.append(AbstractTask::toString());
	return str;
}

}//namespace spdr
