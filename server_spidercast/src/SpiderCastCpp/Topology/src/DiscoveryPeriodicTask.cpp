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
 * DiscoveryPeriodicTask.cpp
 *
 *  Created on: Aug 11, 2010
 *      Author:
 */

#include "DiscoveryPeriodicTask.h"

namespace spdr
{

DiscoveryPeriodicTask::DiscoveryPeriodicTask(CoreInterface& core)
{
	topoMgr_SPtr = core.getTopologyManager();
}

DiscoveryPeriodicTask::~DiscoveryPeriodicTask()
{
}

void DiscoveryPeriodicTask::run()
{
	if (topoMgr_SPtr)
	{
		topoMgr_SPtr->discoveryTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from DiscoveryPeriodicTask::run()");
	}
}


String DiscoveryPeriodicTask::toString()
{
	String str("DiscoveryPeriodicTask ");
	str.append(AbstractTask::toString());
	return str;
}


}
