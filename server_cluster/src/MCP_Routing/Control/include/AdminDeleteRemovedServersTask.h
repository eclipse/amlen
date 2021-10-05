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

#ifndef ADMINDELETEREMOVEDSERVERSTASK_H_
#define ADMINDELETEREMOVEDSERVERSTASK_H_

#include <AbstractTask.h>
#include "ControlManager.h"

namespace mcp
{

class AdminDeleteRemovedServersTask : public mcp::AbstractTask
{
public:
    AdminDeleteRemovedServersTask(ControlManager& controlManager, RemoteServerVector& pendingRemovals);
    virtual ~AdminDeleteRemovedServersTask();

    void run()
    {
        controlManager_.executeAdminDeleteRemovedServersTask(pendingRemovals_);
    }

private:
    ControlManager& controlManager_;
    RemoteServerVector pendingRemovals_;
};

} /* namespace mcp */

#endif /* ADMINDELETEREMOVEDSERVERSTASK_H_ */
