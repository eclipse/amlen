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
 * StopInitialDiscoveryPeriodTask.cpp
 *
 *  Created on: Apr 12, 2010
 *      Author:
 * Version     : $Revision: 1.4 $
 *-----------------------------------------------------------------
 * $Log: StopInitialDiscoveryPeriodTask.cpp,v $
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
 * Revision 1.1  2010/06/14 15:55:39 
 * Initial version
 *
 */

#include "StopInitialDiscoveryPeriodTask.h"

namespace spdr
{

StopInitialDiscoveryPeriodTask::StopInitialDiscoveryPeriodTask(CoreInterface& core)
{
	topoMgr_SPtr = core.getTopologyManager();
}

StopInitialDiscoveryPeriodTask::~StopInitialDiscoveryPeriodTask()
{
	// TODO Auto-generated destructor stub
}

void StopInitialDiscoveryPeriodTask::run()
{
	if (topoMgr_SPtr)
	{
		topoMgr_SPtr->stopFrequentDiscoveryTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from StopInitialDiscoveryPeriodTask::run()");
	}
}

String StopInitialDiscoveryPeriodTask::toString() const
{
	String str("StopInitialDiscoveryPeriodTask ");
	str.append(AbstractTask::toString());
	return str;
}

}//namespace spdr
