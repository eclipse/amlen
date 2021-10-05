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

#ifndef MCP_ENGINESTATISTICSTASK_H_
#define MCP_ENGINESTATISTICSTASK_H_

#include <AbstractTask.h>
#include "RoutingTasksHandler.h"

namespace mcp
{

class EngineStatisticsTask : public mcp::AbstractTask
{
public:
    EngineStatisticsTask(RoutingTasksHandler& routing);
	virtual ~EngineStatisticsTask();

	void run()
	{
		routing_.engineStatisticsTask();
	}

private:

	RoutingTasksHandler& routing_;
};

} /* namespace mcp */

#endif /* MCP_ENGINESTATISTICSTASK_H_ */
