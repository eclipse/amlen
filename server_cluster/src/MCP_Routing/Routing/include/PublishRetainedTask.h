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

#ifndef PUBLISHRETAINEDTASK_H_
#define PUBLISHRETAINEDTASK_H_

#include <AbstractTask.h>
#include "LocalSubManager.h"

namespace mcp
{

class PublishRetainedTask: public mcp::AbstractTask
{
public:
    PublishRetainedTask(LocalSubManager& localSubManager);
	virtual ~PublishRetainedTask();

	void run()
	{
		localSubManager.publishRetainedTask();
	}

private:

	LocalSubManager& localSubManager;
};

} /* namespace mcp */

#endif /* PUBLISHRETAINEDTASK_H_ */
