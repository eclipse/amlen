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
 * TopologyStructuredRefreshTask.cpp
 *
 *  Created on: Feb 9, 2012
 *      Author:
 */

#include "TopologyStructuredRefreshTask.h"

namespace spdr
{
TopologyStructuredRefreshTask::TopologyStructuredRefreshTask(
		CoreInterface& core)
{
	topoMgr_SPtr = core.getTopologyManager();
}

TopologyStructuredRefreshTask::~TopologyStructuredRefreshTask()
{
	// TODO Auto-generated destructor stub
}

void TopologyStructuredRefreshTask::run()
{
	if (topoMgr_SPtr)
	{
		topoMgr_SPtr->structuredTopologyRefreshTask();
	}
	else
	{
		throw NullPointerException(
				"NullPointerException from TopologyStructuredRefreshTask::run()");
	}
}

String TopologyStructuredRefreshTask::toString()
{
	String str("TopologyStructuredRefreshTask ");
	str.append(AbstractTask::toString());
	return str;
}

}
