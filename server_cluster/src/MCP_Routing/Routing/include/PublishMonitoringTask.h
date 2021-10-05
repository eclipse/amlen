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

#ifndef PUBLISHMONITORINGTASK_H_
#define PUBLISHMONITORINGTASK_H_

#include <AbstractTask.h>
#include "LocalSubManager.h"

namespace mcp
{

class PublishMonitoringTask: public mcp::AbstractTask
{
public:
    PublishMonitoringTask(LocalSubManager& localSubManager);
	virtual ~PublishMonitoringTask();

	void run()
	{
		localSubManager.publishMonitoringTask();
	}

private:

	LocalSubManager& localSubManager;
};

} /* namespace mcp */

#endif /* PUBLISHMONITORINGTASK_H_ */
