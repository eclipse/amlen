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
 * DBridgePubSubInterestTask.cpp
 *
 *  Created on: Jul 17, 2012
 */

#include "DBridgePubSubInterestTask.h"

namespace spdr
{

namespace route
{

DBridgePubSubInterestTask::DBridgePubSubInterestTask(RoutingManager_SPtr routingManager) :
		routingManager_(routingManager)
{
}

DBridgePubSubInterestTask::~DBridgePubSubInterestTask()
{
}


void DBridgePubSubInterestTask::run()
{
	if (routingManager_)
	{
		routingManager_->runDelegateBridgeUpdateInterestTask();
	}
	else
	{
		throw NullPointerException("NullPointerException from DBridgePubSubInterestTask::run()");
	}
}


String DBridgePubSubInterestTask::toString() const
{
	String str("DBridgePubSubInterestTask ");
	str.append(AbstractTask::toString());
	return str;
}

}

}
