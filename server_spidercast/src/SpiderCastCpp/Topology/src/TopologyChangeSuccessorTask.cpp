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
 * TopologyChangeSuccessorTask.cpp
 *
 *  Created on: Aug 26, 2010
 *      Author:
 *
 * Version     : $Revision: 1.4 $
 *-----------------------------------------------------------------
 * $Log: TopologyChangeSuccessorTask.cpp,v $
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
 * Revision 1.2  2012/10/25 10:11:12 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2010/08/26 08:34:03 
 * Make "change successor" a seperate and independent task
 *
 */


#include "TopologyChangeSuccessorTask.h"

namespace spdr
{

TopologyChangeSuccessorTask::TopologyChangeSuccessorTask(CoreInterface& core)
{
	topoMgr_SPtr = core.getTopologyManager();
}

TopologyChangeSuccessorTask::~TopologyChangeSuccessorTask()
{
	// TODO Auto-generated destructor stub
}

void TopologyChangeSuccessorTask::run()
{
	if (topoMgr_SPtr)
	{
		topoMgr_SPtr->changeSuccessorTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from TopologyChangeSuccessorTask::run()");
	}
}

String TopologyChangeSuccessorTask::toString() const
{
	String str("TopologyChangeSuccessorTask ");
	str.append(AbstractTask::toString());
	return str;
}

}//namespace spdr
