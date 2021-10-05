/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#ifndef LOCALSUBMANAGER_H_
#define LOCALSUBMANAGER_H_

#include <boost/shared_ptr.hpp>

#include "LocalSubscriptionEvents.h"
#include "RemoteSubscriptionStatsListener.h"

namespace mcp
{

class LocalSubManager: public RemoteSubscriptionStatsListener
{
public:
	LocalSubManager();
	virtual ~LocalSubManager();

	virtual MCPReturnCode publishLocalBFTask() = 0;

	virtual MCPReturnCode publishRetainedTask() = 0;

	virtual MCPReturnCode publishMonitoringTask() = 0;

	virtual MCPReturnCode schedulePublishLocalBFTask(int delayMillis) = 0;

	virtual MCPReturnCode schedulePublishRetainedTask(int delayMillis) = 0;

	virtual MCPReturnCode schedulePublishMonitoringTask(int delayMillis) = 0;

	virtual MCPReturnCode restoreSubscriptionPatterns(const std::vector<SubscriptionPattern_SPtr>& patterns) = 0;
};

typedef boost::shared_ptr<LocalSubManager> LocalSubManager_SPtr;

} /* namespace mcp */

#endif /* LOCALSUBMANAGER_H_ */
